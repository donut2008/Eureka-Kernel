fter the BTH */
			ohdr->u.imm_data = wqe->wr.ex.imm_data;
			hwords += 1;
			if (wqe->wr.send_flags & IB_SEND_SOLICITED)
				bth0 |= IB_BTH_SOLICITED;
		}
		bth2 |= IB_BTH_REQ_ACK;
		qp->s_cur++;
		if (qp->s_cur >= qp->s_size)
			qp->s_cur = 0;
		break;

	case OP(RDMA_READ_RESPONSE_MIDDLE):
		/*
		 * qp->s_state is normally set to the opcode of the
		 * last packet constructed for new requests and therefore
		 * is never set to RDMA read response.
		 * RDMA_READ_RESPONSE_MIDDLE is used by the ACK processing
		 * thread to indicate a RDMA read needs to be restarted from
		 * an earlier PSN without interferring with the sending thread.
		 * See qib_restart_rc().
		 */
		len = ((qp->s_psn - wqe->psn) & QIB_PSN_MASK) * pmtu;
		ohdr->u.rc.reth.vaddr =
			cpu_to_be64(wqe->rdma_wr.remote_addr + len);
		ohdr->u.rc.reth.rkey =
			cpu_to_be32(wqe->rdma_wr.rkey);
		ohdr->u.rc.reth.length = cpu_to_be32(wqe->length - len);
		qp->s_state = OP(RDMA_READ_REQUEST);
		hwords += sizeof(ohdr->u.rc.reth) / sizeof(u32);
		bth2 = (qp->s_psn & QIB_PSN_MASK) | IB_BTH_REQ_ACK;
		qp->s_psn = wqe->lpsn + 1;
		ss = NULL;
		len = 0;
		qp->s_cur++;
		if (qp->s_cur == qp->s_size)
			qp->s_cur = 0;
		break;
	}
	qp->s_sending_hpsn = bth2;
	delta = (((int) bth2 - (int) wqe->psn) << 8) >> 8;
	if (delta && delta % QIB_PSN_CREDIT == 0)
		bth2 |= IB_BTH_REQ_ACK;
	if (qp->s_flags & QIB_S_SEND_ONE) {
		qp->s_flags &= ~QIB_S_SEND_ONE;
		qp->s_flags |= QIB_S_WAIT_ACK;
		bth2 |= IB_BTH_REQ_ACK;
	}
	qp->s_len -= len;
	qp->s_hdrwords = hwords;
	qp->s_cur_sge = ss;
	qp->s_cur_size = len;
	qib_make_ruc_header(qp, ohdr, bth0 | (qp->s_state << 24), bth2);
done:
	ret = 1;
	goto unlock;

bail:
	qp->s_flags &= ~QIB_S_BUSY;
unlock:
	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

/**
 * qib_send_rc_ack - Construct an ACK packet and send it
 * @qp: a pointer to the QP
 *
 * This is called from qib_rc_rcv() and qib_kreceive().
 * Note that RDMA reads and atomics are handled in the
 * send side QP state and tasklet.
 */
void qib_send_rc_ack(struct qib_qp *qp)
{
	struct qib_devdata *dd = dd_from_ibdev(qp->ibqp.device);
	struct qib_ibport *ibp = to_iport(qp->ibqp.device, qp->port_num);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u64 pbc;
	u16 lrh0;
	u32 bth0;
	u32 hwords;
	u32 pbufn;
	u32 __iomem *piobuf;
	struct qib_ib_header hdr;
	struct qib_other_headers *ohdr;
	u32 control;
	unsigned long flags;

	spin_lock_irqsave(&qp->s_lock, flags);

	if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK))
		goto unlock;

	/* Don't send ACK or NAK if a RDMA read or atomic is pending. */
	if ((qp->s_flags & QIB_S_RESP_PENDING) || qp->s_rdma_ack_cnt)
		goto queue_ack;

	/* Construct the header with s_lock held so APM doesn't change it. */
	ohdr = &hdr.u.oth;
	lrh0 = QIB_LRH_BTH;
	/* header size in 32-bit words LRH+BTH+AETH = (8+12+4)/4. */
	hwords = 6;
	if (unlikely(qp->remote_ah_attr.ah_flags & IB_AH_GRH)) {
		hwords += qib_make_grh(ibp, &hdr.u.l.grh,
				       &qp->remote_ah_attr.grh, hwords, 0);
		ohdr = &hdr.u.l.oth;
		lrh0 = QIB_LRH_GRH;
	}
	/* read pkey_index w/o lock (its atomic) */
	bth0 = qib_get_pkey(ibp, qp->s_pkey_index) | (OP(ACKNOWLEDGE) << 24);
	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth0 |= IB_BTH_MIG_REQ;
	if (qp->r_nak_state)
		ohdr->u.aeth = cpu_to_be32((qp->r_msn & QIB_MSN_MASK) |
					    (qp->r_nak_state <<
					     QIB_AETH_CREDIT_SHIFT));
	else
		ohdr->u.aeth = qib_compute_aeth(qp);
	lrh0 |= ibp->sl_to_vl[qp->remote_ah_attr.sl] << 12 |
		qp->remote_ah_attr.sl << 4;
	hdr.lrh[0] = cpu_to_be16(lrh0);
	hdr.lrh[1] = cpu_to_be16(qp->remote_ah_attr.dlid);
	hdr.lrh[2] = cpu_to_be16(hwords + SIZE_OF_CRC);
	hdr.lrh[3] = cpu_to_be16(ppd->lid | qp->remote_ah_attr.src_path_bits);
	ohdr->bth[0] = cpu_to_be32(bth0);
	ohdr->bth[1] = cpu_to_be32(qp->remote_qpn);
	ohdr->bth[2] = cpu_to_be32(qp->r_ack_psn & QIB_PSN_MASK);

	spin_unlock_irqrestore(&qp->s_lock, flags);

	/* Don't try to send ACKs if the link isn't ACTIVE */
	if (!(ppd->lflags & QIBL_LINKACTIVE))
		goto done;

	control = dd->f_setpbc_control(ppd, hwords + SIZE_OF_CRC,
				       qp->s_srate, lrh0 >> 12);
	/* length is + 1 for the control dword */
	pbc = ((u64) control << 32) | (hwords + 1);

	piobuf = dd->f_getsendbuf(ppd, pbc, &pbufn);
	if (!piobuf) {
		/*
		 * We are out of PIO buffers at the moment.
		 * Pass responsibility for sending the ACK to the
		 * send tasklet so that when a PIO buffer becomes
		 * available, the ACK is sent ahead of other outgoing
		 * packets.
		 */
		spin_lock_irqsave(&qp->s_lock, flags);
		goto queue_ack;
	}

	/*
	 * Write the pbc.
	 * We have to flush after the PBC for correctness
	 * on some cpus or WC buffer can be written out of order.
	 */
	writeq(pbc, piobuf);

	if (dd->flags & QIB_PIO_FLUSH_WC) {
		u32 *hdrp = (u32 *) &hdr;

		qib_flush_wc();
		qib_pio_copy(piobuf + 2, hdrp, hwords - 1);
		qib_flush_wc();
		__raw_writel(hdrp[hwords - 1], piobuf + hwords + 1);
	} else
		qib_pio_copy(piobuf + 2, (u32 *) &hdr, hwords);

	if (dd->flags & QIB_USE_SPCL_TRIG) {
		u32 spcl_off = (pbufn >= dd->piobcnt2k) ? 2047 : 1023;

		qib_flush_wc();
		__raw_writel(0xaebecede, piobuf + spcl_off);
	}

	qib_flush_wc();
	qib_sendbuf_done(dd, pbufn);

	this_cpu_inc(ibp->pmastats->n_unicast_xmit);
	goto done;

queue_ack:
	if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK) {
		ibp->n_rc_qacks++;
		qp->s_flags |= QIB_S_ACK_PENDING | QIB_S_RESP_PENDING;
		qp->s_nak_state = qp->r_nak_state;
		qp->s_ack_psn = qp->r_ack_psn;

		/* Schedule the send tasklet. */
		qib_schedule_send(qp);
	}
unlock:
	spin_unlock_irqrestore(&qp->s_lock, flags);
done:
	return;
}

/**
 * reset_psn - reset the QP state to send starting from PSN
 * @qp: the QP
 * @psn: the packet sequence number to restart at
 *
 * This is called from qib_rc_rcv() to process an incoming RC ACK
 * for the given QP.
 * Called at interrupt level with the QP s_lock held.
 */
static void reset_psn(struct qib_qp *qp, u32 psn)
{
	u32 n = qp->s_acked;
	struct qib_swqe *wqe = get_swqe_ptr(qp, n);
	u32 opcode;

	qp->s_cur = n;

	/*
	 * If we are starting the request from the beginning,
	 * let the normal send code handle initialization.
	 */
	if (qib_cmp24(psn, wqe->psn) <= 0) {
		qp->s_state = OP(SEND_LAST);
		goto done;
	}

	/* Find the work request opcode corresponding to the given PSN. */
	opcode = wqe->wr.opcode;
	for (;;) {
		int diff;

		if (++n == qp->s_size)
			n = 0;
		if (n == qp->s_tail)
			break;
		wqe = get_swqe_ptr(qp, n);
		diff = qib_cmp24(psn, wqe->psn);
		if (diff < 0)
			break;
		qp->s_cur = n;
		/*
		 * If we are starting the request from the beginning,
		 * let the normal send code handle initialization.
		 */
		if (diff == 0) {
			qp->s_state = OP(SEND_LAST);
			goto done;
		}
		opcode = wqe->wr.opcode;
	}

	/*
	 * Set the state to restart in the middle of a request.
	 * Don't change the s_sge, s_cur_sge, or s_cur_size.
	 * See qib_make_rc_req().
	 */
	switch (opcode) {
	case IB_WR_SEND:
	case IB_WR_SEND_WITH_IMM:
		qp->s_state = OP(RDMA_READ_RESPONSE_FIRST);
		break;

	case IB_WR_RDMA_WRITE:
	case IB_WR_RDMA_WRITE_WITH_IMM:
		qp->s_state = OP(RDMA_READ_RESPONSE_LAST);
		break;

	case IB_WR_RDMA_READ:
		qp->s_state = OP(RDMA_READ_RESPONSE_MIDDLE);
		break;

	default:
		/*
		 * This case shouldn't happen since its only
		 * one PSN per req.
		 */
		qp->s_state = OP(SEND_LAST);
	}
done:
	qp->s_psn = psn;
	/*
	 * Set QIB_S_WAIT_PSN as qib_rc_complete() may start the timer
	 * asynchronously before the send tasklet can get scheduled.
	 * Doing it in qib_make_rc_req() is too late.
	 */
	if ((qib_cmp24(qp->s_psn, qp->s_sending_hpsn) <= 0) &&
	    (qib_cmp24(qp->s_sending_psn, qp->s_sending_hpsn) <= 0))
		qp->s_flags |= QIB_S_WAIT_PSN;
}

/*
 * Back up requester to resend the last un-ACKed request.
 * The QP r_lock and s_lock should be held and interrupts disabled.
 */
static void qib_restart_rc(struct qib_qp *qp, u32 psn, int wait)
{
	struct qib_swqe *wqe = get_swqe_ptr(qp, qp->s_acked);
	struct qib_ibport *ibp;

	if (qp->s_retry == 0) {
		if (qp->s_mig_state == IB_MIG_ARMED) {
			qib_migrate_qp(qp);
			qp->s_retry = qp->s_retry_cnt;
		} else if (qp->s_last == qp->s_acked) {
			qib_send_complete(qp, wqe, IB_WC_RETRY_EXC_ERR);
			qib_error_qp(qp, IB_WC_WR_FLUSH_ERR);
			return;
		} else /* XXX need to handle delayed completion */
			return;
	} else
		qp->s_retry--;

	ibp = to_iport(qp->ibqp.device, qp->port_num);
	if (wqe->wr.opcode == IB_WR_RDMA_READ)
		ibp->n_rc_resends++;
	else
		ibp->n_rc_resends += (qp->s_psn - psn) & QIB_PSN_MASK;

	qp->s_flags &= ~(QIB_S_WAIT_FENCE | QIB_S_WAIT_RDMAR |
			 QIB_S_WAIT_SSN_CREDIT | QIB_S_WAIT_PSN |
			 QIB_S_WAIT_ACK);
	if (wait)
		qp->s_flags |= QIB_S_SEND_ONE;
	reset_psn(qp, psn);
}

/*
 * This is called from s_timer for missing responses.
 */
static void rc_timeout(unsigned long arg)
{
	struct qib_qp *qp = (struct qib_qp *)arg;
	struct qib_ibport *ibp;
	unsigned long flags;

	spin_lock_irqsave(&qp->r_lock, flags);
	spin_lock(&qp->s_lock);
	if (qp->s_flags & QIB_S_TIMER) {
		ibp = to_iport(qp->ibqp.device, qp->port_num);
		ibp->n_rc_timeouts++;
		qp->s_flags &= ~QIB_S_TIMER;
		del_timer(&qp->s_timer);
		qib_restart_rc(qp, qp->s_last_psn + 1, 1);
		qib_schedule_send(qp);
	}
	spin_unlock(&qp->s_lock);
	spin_unlock_irqrestore(&qp->r_lock, flags);
}

/*
 * This is called from s_timer for RNR timeouts.
 */
void qib_rc_rnr_retry(unsigned long arg)
{
	struct qib_qp *qp = (struct qib_qp *)arg;
	unsigned long flags;

	spin_lock_irqsave(&qp->s_lock, flags);
	if (qp->s_flags & QIB_S_WAIT_RNR) {
		qp->s_flags &= ~QIB_S_WAIT_RNR;
		del_timer(&qp->s_timer);
		qib_schedule_send(qp);
	}
	spin_unlock_irqrestore(&qp->s_lock, flags);
}

/*
 * Set qp->s_sending_psn to the next PSN after the given one.
 * This would be psn+1 except when RDMA reads are present.
 */
static void reset_sending_psn(struct qib_qp *qp, u32 psn)
{
	struct qib_swqe *wqe;
	u32 n = qp->s_last;

	/* Find the work request corresponding to the given PSN. */
	for (;;) {
		wqe = get_swqe_ptr(qp, n);
		if (qib_cmp24(psn, wqe->lpsn) <= 0) {
			if (wqe->wr.opcode == IB_WR_RDMA_READ)
				qp->s_sending_psn = wqe->lpsn + 1;
			else
				qp->s_sending_psn = psn + 1;
			break;
		}
		if (++n == qp->s_size)
			n = 0;
		if (n == qp->s_tail)
			break;
	}
}

/*
 * This should be called with the QP s_lock held and interrupts disabled.
 */
void qib_rc_send_complete(struct qib_qp *qp, struct qib_ib_header *hdr)
{
	struct qib_other_headers *ohdr;
	struct qib_swqe *wqe;
	struct ib_wc wc;
	unsigned i;
	u32 opcode;
	u32 psn;

	if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_OR_FLUSH_SEND))
		return;

	/* Find out where the BTH is */
	if ((be16_to_cpu(hdr->lrh[0]) & 3) == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else
		ohdr = &hdr->u.l.oth;

	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	if (opcode >= OP(RDMA_READ_RESPONSE_FIRST) &&
	    opcode <= OP(ATOMIC_ACKNOWLEDGE)) {
		WARN_ON(!qp->s_rdma_ack_cnt);
		qp->s_rdma_ack_cnt--;
		return;
	}

	psn = be32_to_cpu(ohdr->bth[2]);
	reset_sending_psn(qp, psn);

	/*
	 * Start timer after a packet requesting an ACK has been sent and
	 * there are still requests that haven't been acked.
	 */
	if ((psn & IB_BTH_REQ_ACK) && qp->s_acked != qp->s_tail &&
	    !(qp->s_flags & (QIB_S_TIMER | QIB_S_WAIT_RNR | QIB_S_WAIT_PSN)) &&
	    (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK))
		start_timer(qp);

	while (qp->s_last != qp->s_acked) {
		wqe = get_swqe_ptr(qp, qp->s_last);
		if (qib_cmp24(wqe->lpsn, qp->s_sending_psn) >= 0 &&
		    qib_cmp24(qp->s_sending_psn, qp->s_sending_hpsn) <= 0)
			break;
		for (i = 0; i < wqe->wr.num_sge; i++) {
			struct qib_sge *sge = &wqe->sg_list[i];

			qib_put_mr(sge->mr);
		}
		/* Post a send completion queue entry if requested. */
		if (!(qp->s_flags & QIB_S_SIGNAL_REQ_WR) ||
		    (wqe->wr.send_flags & IB_SEND_SIGNALED)) {
			memset(&wc, 0, sizeof(wc));
			wc.wr_id = wqe->wr.wr_id;
			wc.status = IB_WC_SUCCESS;
			wc.opcode = ib_qib_wc_opcode[wqe->wr.opcode];
			wc.byte_len = wqe->length;
			wc.qp = &qp->ibqp;
			qib_cq_enter(to_icq(qp->ibqp.send_cq), &wc, 0);
		}
		if (++qp->s_last >= qp->s_size)
			qp->s_last = 0;
	}
	/*
	 * If we were waiting for sends to complete before resending,
	 * and they are now complete, restart sending.
	 */
	if (qp->s_flags & QIB_S_WAIT_PSN &&
	    qib_cmp24(qp->s_sending_psn, qp->s_sending_hpsn) > 0) {
		qp->s_flags &= ~QIB_S_WAIT_PSN;
		qp->s_sending_psn = qp->s_psn;
		qp->s_sending_hpsn = qp->s_psn - 1;
		qib_schedule_send(qp);
	}
}

static inline void update_last_psn(struct qib_qp *qp, u32 psn)
{
	qp->s_last_psn = psn;
}

/*
 * Generate a SWQE completion.
 * This is similar to qib_send_complete but has to check to be sure
 * that the SGEs are not being referenced if the SWQE is being resent.
 */
static struct qib_swqe *do_rc_completion(st