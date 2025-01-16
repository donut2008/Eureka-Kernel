	  __le32 *rhf_addr, struct qib_message_header *rhdr)
{
	u32 ret = 0;

	if (eflags & (QLOGIC_IB_RHF_H_ICRCERR | QLOGIC_IB_RHF_H_VCRCERR))
		ret = 1;
	else if (eflags == QLOGIC_IB_RHF_H_TIDERR) {
		/* For TIDERR and RC QPs premptively schedule a NAK */
		struct qib_ib_header *hdr = (struct qib_ib_header *) rhdr;
		struct qib_other_headers *ohdr = NULL;
		struct qib_ibport *ibp = &ppd->ibport_data;
		struct qib_qp *qp = NULL;
		u32 tlen = qib_hdrget_length_in_bytes(rhf_addr);
		u16 lid  = be16_to_cpu(hdr->lrh[1]);
		int lnh = be16_to_cpu(hdr->lrh[0]) & 3;
		u32 qp_num;
		u32 opcode;
		u32 psn;
		int diff;

		/* Sanity check packet */
		if (tlen < 24)
			goto drop;

		if (lid < QIB_MULTICAST_LID_BASE) {
			lid &= ~((1 << ppd->lmc) - 1);
			if (unlikely(lid != ppd->lid))
				goto drop;
		}

		/* Check for GRH */
		if (lnh == QIB_LRH_BTH)
			ohdr = &hdr->u.oth;
		else if (lnh == QIB_LRH_GRH) {
			u32 vtf;

			ohdr = &hdr->u.l.oth;
			if (hdr->u.l.grh.next_hdr != IB_GRH_NEXT_HDR)
				goto drop;
			vtf = be32_to_cpu(hdr->u.l.grh.version_tclass_flow);
			if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
				goto drop;
		} else
			goto drop;

		/* Get opcode and PSN from packet */
		opcode = be32_to_cpu(ohdr->bth[0]);
		opcode >>= 24;
		psn = be32_to_cpu(ohdr->bth[2]);

		/* Get the destination QP number. */
		qp_num = be32_to_cpu(ohdr->bth[1]) & QIB_QPN_MASK;
		if (qp_num != QIB_MULTICAST_QPN) {
			int ruc_res;

			qp = qib_lookup_qpn(ibp, qp_num);
			if (!qp)
				goto drop;

			/*
			 * Handle only RC QPs - for other QP types drop error
			 * packet.
			 */
			spin_lock(&qp->r_lock);

			/* Check for valid receive state. */
			if (!(ib_qib_state_ops[qp->state] &
			      QIB_PROCESS_RECV_OK)) {
				ibp->n_pkt_drops++;
				goto unlock;
			}

			switch (qp->ibqp.qp_type) {
			case IB_QPT_RC:
				ruc_res =
					qib_ruc_check_hdr(
						ibp, hdr,
						lnh == QIB_LRH_GRH,
						qp,
						be32_to_cpu(ohdr->bth[0]));
				if (ruc_res)
					goto unlock;

				/* Only deal with RDMA Writes for now */
				if (opcode <
				    IB_OPCODE_RC_RDMA_READ_RESPONSE_FIRST) {
					diff = qib_cmp24(psn, qp->r_psn);
					if (!qp->r_nak_state && diff >= 0) {
						ibp->n_rc_seqnak++;
						qp->r_nak_state =
							IB_NAK_PSN_ERROR;
						/* Use the expected PSN. */
						qp->r_ack_psn = qp->r_psn;
						/*
						 * Wait to send the sequence
						 * NAK until all packets
						 * in the receive queue have
						 * been processed.
						 * Otherwise, we end up
						 * propagating congestion.
						 */
						if (list_empty(&qp->rspwait)) {
							qp->r_flags |=
								QIB_R_RSP_NAK;
							atomic_inc(
								&qp->refcount);
							list_add_tail(
							 &qp->rspwait,
							 &rcd->qp_wait_list);
						}
					} /* Out of sequence NAK */
				} /* QP Request NAKs */
				break;
			case IB_QPT_SMI:
			case IB_QPT_GSI:
			case IB_QPT_UD:
			case IB_QPT_UC:
			default:
				/* For now don't handle any other QP types */
				break;
			}

unlock:
			spin_unlock(&qp->r_lock);
			/*
			 * Notify qib_destroy_qp() if it is waiting
			 * for us to finish.
			 */
			if (atomic_dec_and_test(&qp->refcount))
				wake_up(&qp->wait);
		} /* Unicast QP */
	} /* Valid packet with TIDErr */

drop:
	return ret;
}

/*
 * qib_kreceive - receive a packet
 * @rcd: the qlogic_ib context
 * @llic: gets count of good packets needed to clear lli,
 *          (used with chips that need need to track crcs for lli)
 *
 * called from interrupt handler for errors or receive interrupt
 * Returns number of CRC error packets, needed by some chips for
 * local link integrity tracking.   crcs are adjusted down by following
 * good packets, if any, and count of good packets is also tracked.
 */
u32 qib_kreceive(struct qib_ctxtdata *rcd, u32 *llic, u32 *npkts)
{
	struct qib_devdata *dd = rcd->dd;
	struct qib_pportdata *ppd = rcd->ppd;
	__le32 *rhf_addr;
	void *ebuf;
	const u32 rsize = dd->rcvhdrentsize;        /* words */
	const u32 maxcnt = dd->rcvhdrcnt * rsize;   /* words */
	u32 etail = -1, l, hdrqtail;
	struct qib_message_header *hdr;
	u32 eflags, etype, tlen, i = 0, updegr = 0, crcs = 0;
	int last;
	u64 lval;
	struct qib_qp *qp, *nqp;

	l = rcd->head;
	rhf_addr = (__le32 *) rcd->rcvhdrq + l + dd->rhf_offset;
	if (dd->flags & QIB_NODMA_RTAIL) {
		u32 seq = qib_hdrget_seq(rhf_addr);

		if (seq != rcd->seq_cnt)
			goto bail;
		hdrqtail = 0;
	} else {
		hdrqtail = qib_get_rcvhdrtail(rcd);
		if (l == hdrqtail)
			goto bail;
		smp_rmb();  /* prevent speculative reads of dma'ed hdrq */
	}

	for (last = 0, i = 1; !last; i += !last) {
		hdr = dd->f_get_msgheader(dd, rhf_addr);
		eflags = qib_hdrget_err_flags(rhf_addr);
		etype = qib_hdrget_rcv_type(rhf_addr);
		/* total length */
		tlen = qib_hdrget_length_in_bytes(rhf_addr);
		ebuf = NULL;
		if ((dd->flags & QIB_NODMA_RTAIL) ?
		    qib_hdrget_use_egr_buf(rhf_addr) :
		    (etype != RCVHQ_RCV_TYPE_EXPECTED)) {
			etail = qib_hdrget_index(rhf_addr);
			updegr = 1;
			if (tlen > sizeof(*hdr) ||
			    etype >= RCVHQ_RCV_TYPE_NON_KD) {
				ebuf = qib_get_egrbuf(rcd, etail);
				prefetch_range(ebuf, tlen - sizeof(*hdr));
			}
		}
		if (!eflags) {
			u16 lrh_len = be16_to_cpu(hdr->lrh[2]) << 2;

			if (lrh_len != tlen) {
				qib_stats.sps_lenerrs++;
				goto move_along;
			}
		}
		if (etype == RCVHQ_RCV_TYPE_NON_KD && !eflags &&
		    ebuf == NULL &&
		    tlen > (dd->rcvhdrentsize - 2 + 1 -
				qib_hdrget_offset(rhf_addr)) << 2) {
			goto move_along;
		}

		/*
		 * Both tiderr and qibhdrerr are set for all plain IB
		 * packets; only qibhdrerr should be set.
		 */
		if (unlikely(eflags))
			crcs += qib_rcv_hdrerr(rcd, ppd, rcd->ctxt, eflags, l,
					       etail, rhf_addr, hdr);
		else if (etype == RCVHQ_RCV_TYPE_NON_KD) {
			qib_ib_rcv(rcd, hdr, ebuf, tlen);
			if (crcs)
				crcs--;
			else if (llic && *llic)
				--*llic;
		}
move_along:
		l += rsize;
		if (l >= maxcnt)
			l = 0;
		if (i == QIB_MAX_PKT_RECV)
			last = 1;

		rhf_addr = (__le32 *) rcd->rcvhdrq + l + dd->rhf_offset;
		if (dd->flags & QIB_NODMA_RTAIL) {
			u32 seq = qib_hdrget_seq(rhf_addr);

			if (++rcd->seq_cnt > 13)
				rcd->seq_cnt = 1;
			if (seq != rcd->seq_cnt)
				last = 1;
		} else if (l == hdrqtail)
			last = 1;
		/*
		 * Update head regs etc., every 16 packets, if not last pkt,
		 * to help prevent rcvhdrq overflows, when many packets
		 * are processed and queue is nearly full.
		 * Don't request an interrupt for intermediate updates.
		 */
		lval = l;
		if (!last && !(i & 0xf)) {
			dd->f_update_usrhead(rcd, lval, updegr, etail, i);
			updegr = 0;
		}
	}
	/*
	 * Notify qib_destroy_qp() if it is waiting
	 * for lookaside_qp to finish.
	 */
	if (rcd->lookaside_qp) {
		if (atomic_dec_and_test(&rcd->lookaside_qp->refcount))
			wake_up(&rcd->lookaside_qp->wait);
		rcd->lookaside_qp = NULL;
	}

	rcd->head = l;

	/*
	 * Iterate over all QPs waiting to respond.
	 * The list won't change since the IRQ is only run on one CPU.
	 */
	list_for_each_entry_safe(qp, nqp, &rcd->qp_wait_list, rspwait) {
		list_del_init(&qp->rspwait);
		if (qp->r_flags & QIB_R_RSP_NAK) {
			qp->r_flags &= ~QIB_R_RSP_NAK;
			qib_send_rc_ack(qp);
		}
		if (qp->r_flags & QIB_R_RSP_SEND) {
			unsigned long flags;

			qp->r_flags &= ~QIB_R_RSP_SEND;
			spin_lock_irqsave(&qp->s_lock, flags);
			if (ib_qib_state_ops[qp->state] &
					QIB_PROCESS_OR_FLUSH_SEND)
				qib_schedule_send(qp);
			spin_unlock_irqrestore(&qp->s_lock, flags);
		}
		if (atomic_dec_and_test(&qp->refcount))
			wake_up(&qp->wait);
	}

bail:
	/* Report number of packets consumed */
	if (npkts)
		*npkts = i;

	/*
	 * Always write head at end, and setup rcv interrupt, even
	 * if no packets were processed.
	 */
	lval = (u64)rcd->head | dd->rhdrhead_intr_off;
	dd->f_update_usrhead(rcd, lval, updegr, etail, i);
	return crcs;
}

/**
 * qib_set_mtu - set the MTU
 * @ppd: the perport data
 * @arg: the new MTU
 *
 * We can handle "any" incoming size, the issue here is whether we
 * need to restrict our outgoing size.   For now, we don't do any
 * sanity checking on this, and we don't deal with what happens to
 * programs that are already running when the size changes.
 * NOTE: changing the MTU will usually cause the IBC to go back to
 * link INIT state...
 */
int qib_set_mtu(struct qib_pportdata *ppd, u16 arg)
{
	u32 piosize;
	int ret, chk;

	if (arg != 256 && arg != 512 && arg != 1024 && arg != 2048 &&
	    arg != 4096) {
		ret = -EINVAL;
		goto bail;
	}
	chk = ib_mtu_enum_to_int(qib_ibmtu);
	if (chk > 0 && arg > chk) {
		ret = -EINVAL;
		goto bail;
	}

	piosize = ppd->ibmaxlen;
	ppd->ibmtu = arg;

	if (arg >= (piosize - QIB_PIO_MAXIBHDR)) {
		/* Only if it's not the initial value (or reset to it) */
		if (piosize != ppd->init_ibmaxlen) {
			if (arg > piosize && arg <= ppd->init_ibmaxlen)
				piosize = ppd->init_ibmaxlen - 2 * sizeof(u32);
			ppd->ibmaxlen = piosize;
		}
	} else if ((arg + QIB_PIO_MAXIBHDR) != ppd->ibmaxlen) {
		piosize = arg + QIB_PIO_MAXIBHDR - 2 * sizeof(u32);
		ppd->ibmaxlen = piosize;
	}

	ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_MTU, 0);

	ret = 0;

bail:
	return ret;
}

int qib_set_lid(struct qib_pportdata *ppd, u32 lid, u8 lmc)
{
	struct qib_devdata *dd = ppd->dd;

	ppd->lid = lid;
	ppd->lmc = lmc;

	dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LIDLMC,
			 lid | (~((1U << lmc) - 1)) << 16);

	qib_devinfo(dd->pcidev, "IB%u:%u got a lid: 0x%x\n",
		    dd->unit, ppd->port, lid);

	return 0;
}

/*
 * Following deal with the "obviously simple" task of overriding the state
 * of the LEDS, which normally indicate link physical and logical status.
 * The complications arise in dealing with different hardware mappings
 * and the board-dependent routine being called from interrupts.
 * and then there's the requirement to _flash_ them.
 */
#define LED_OVER_FREQ_SHIFT 8
#define LED_OVER_FREQ_MASK (0xFF<<LED_OVER_FREQ_SHIFT)
/* Below is "non-zero" to force override, but both actual LEDs are off */
#define LED_OVER_BOTH_OFF (8)

static void qib_run_led_override(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)opaque;
	struct qib_devdata *dd = ppd->dd;
	int timeoff;
	int ph_idx;

	if (!(dd->flags & QIB_INITTED))
		return;

	ph_idx = ppd->led_override_phase++ & 1;
	ppd->led_override = ppd->led_override_vals[ph_idx];
	timeoff = ppd->led_override_timeoff;

	dd->f_setextled(ppd, 1);
	/*
	 * don't re-fire the timer if user asked for it to be off; we let
	 * it fire one more time after they turn it off to simplify
	 */
	if (ppd->led_override_vals[0] || ppd->led_override_vals[1])
		mod_timer(&ppd->led_override_timer, jiffies + timeoff);
}

void qib_set_led_override(struct qib_pportdata *ppd, unsigned int val)
{
	struct qib_devdata *dd = ppd->dd;
	int timeoff, freq;

	if (!(dd->flags & QIB_INITTED))
		return;

	/* First check if we are blinking. If not, use 1HZ polling */
	timeoff = HZ;
	freq = (val & LED_OVER_FREQ_MASK) >> LED_OVER_FREQ_SHIFT;

	if (freq) {
		/* For blink, set each phase from one nybble of val */
		ppd->led_override_vals[0] = val & 0xF;
		ppd->led_override_vals[1] = (val >> 4) & 0xF;
		timeoff = (HZ << 4)/freq;
	} else {
		/* Non-blink set both