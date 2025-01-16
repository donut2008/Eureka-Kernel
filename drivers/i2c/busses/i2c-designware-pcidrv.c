IMM;
		wc.ex.imm_data = swqe->wr.ex.imm_data;
	}

	spin_lock_irqsave(&qp->r_lock, flags);

	/*
	 * Get the next work request entry to find where to put the data.
	 */
	if (qp->r_flags & QIB_R_REUSE_SGE)
		qp->r_flags &= ~QIB_R_REUSE_SGE;
	else {
		int ret;

		ret = qib_get_rwqe(qp, 0);
		if (ret < 0) {
			qib_rc_error(qp, IB_WC_LOC_QP_OP_ERR);
			goto bail_unlock;
		}
		if (!ret) {
			if (qp->ibqp.qp_num == 0)
				ibp->n_vl15_dropped++;
			goto bail_unlock;
		}
	}
	/* Silently drop packets which are too big. */
	if (unlikely(wc.byte_len > qp->r_len)) {
		qp->r_flags |= QIB_R_REUSE_SGE;
		ibp->n_pkt_drops++;
		goto bail_unlock;
	}

	if (ah_attr->ah_flags & IB_AH_GRH) {
		qib_copy_sge(&qp->r_sge, &ah_attr->grh,
			     sizeof(struct ib_grh), 1);
		wc.wc_flags |= IB_WC_GRH;
	} else
		qib_skip_sge(&qp->r_sge, sizeof(struct ib_grh), 1);
	ssge.sg_list = swqe->sg_list + 1;
	ssge.sge = *swqe->sg_list;
	ssge.num_sge = swqe->wr.num_sge;
	sge = &ssge.sge;
	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		qib_copy_sge(&qp->r_sge, sge->vaddr, len, 1);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (--ssge.num_sge)
				*sge = *ssge.sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		length -= len;
	}
	qib_put_ss(&qp->r_sge);
	if (!test_and_clear_bit(QIB_R_WRID_VALID, &qp->r_aflags))
		goto bail_unlock;
	wc.wr_id = qp->r_wr_id;
	wc.status = IB_WC_SUCCESS;
	wc.opcode = IB_WC_RECV;
	wc.qp = &qp->ibqp;
	wc.src_qp = sqp->ibqp.qp_num;
	wc.pkey_index = qp->ibqp.qp_type == IB_QPT_GSI ?
		swqe->ud_wr.pkey_index : 0;
	wc.slid = ppd->lid | (ah_attr->src_path_bits & ((1 << ppd->lmc) - 1));
	wc.sl = ah_attr->sl;
	wc.dlid_path_bits = ah_attr->dlid & ((1 << ppd->lmc) - 1);
	wc.port_num = qp->port_num;
	/* Signal completion event if the solicited bit is set. */
	qib_cq_enter(to_icq(qp->ibqp.recv_cq), &wc,
		     swqe->wr.send_flags & IB_SEND_SOLICITED);
	ibp->n_loop_pkts++;
bail_unlock:
	spin_unlock_irqrestore(&qp->r_lock, flags);
drop:
	if (atomic_dec_and_test(&qp->refcount))
		wake_up(&qp->wait);
}

/**
 * qib_make_ud_req - construct a UD request packet
 * @qp: the QP
 *
 * Return 1 if constructed; otherwise, return 0.
 */
int qib_make_ud_req(struct qib_qp *qp)
{
	struct qib_other_headers *ohdr;
	struct ib_ah_attr *ah_attr;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	struct qib_swqe *wqe;
	unsigned long flags;
	u32 nwords;
	u32 extra_bytes;
	u32 bth0;
	u16 lrh0;
	u16 lid;
	int ret = 0;
	int next_cur;

	spin_lock_irqsave(&qp->s_lock, flags);

	if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_NEXT_SEND_OK)) {
		if (!(ib_qib_state_ops[qp->state] & QIB_FLUSH_SEND))
			goto bail;
		/* We are in the error state, flush the work request. */
		if (qp->s_last == qp->s_head)
			goto bail;
		/* If DMAs are in progress, we can't flush immediately. */
		if (atomic_read(&qp->s_dma_busy)) {
			qp->s_flags |= QIB_S_WAIT_DMA;
			goto bail;
		}
		wqe = get_swqe_ptr(qp, qp->s_last);
		qib_send_complete(qp, wqe, IB_WC_WR_FLUSH_ERR);
		goto done;
	}

	if (qp->s_cur == qp->s_head)
		goto bail;

	wqe = get_swqe_ptr(qp, qp->s_cur);
	next_cur = qp->s_cur + 1;
	if (next_cur >= qp->s_size)
		next_cur = 0;

	/* Construct the header. */
	ibp = to_iport(qp->ibqp.device, qp->port_num);
	ppd = ppd_from_ibp(ibp);
	ah_attr = &to_iah(wqe->ud_wr.ah)->attr;
	if (ah_attr->dlid >= QIB_MULTICAST_LID_BASE) {
		if (ah_attr->dlid != QIB_PERMISSIVE_LID)
			this_cpu_inc(ibp->pmastats->n_multicast_xmit);
		else
			this_cpu_inc(ibp->pmastats->n_unicast_xmit);
	} else {
		this_cpu_inc(ibp->pmastats->n_unicast_xmit);
		lid = ah_attr->dlid & ~((1 << ppd->lmc) - 1);
		if (unlikely(lid == ppd->lid)) {
			/*
			 * If DMAs are in progress, we can't generate
			 * a completion for the loopback packet since
			 * it would be out of order.
			 * XXX Instead of waiting, we could queue a
			 * zero length descriptor so we get a callback.
			 */
			if (atomic_read(&qp->s_dma_busy)) {
				qp->s_flags |= QIB_S_WAIT_DMA;
				goto bail;
			}
			qp->s_cur = next_cur;
			spin_unlock_irqrestore(&qp->s_lock, flags);
			qib_ud_loopback(qp, wqe);
			spin_lock_irqsave(&qp->s_lock, flags);
			qib_send_complete(qp, wqe, IB_WC_SUCCESS);
			goto done;
		}
	}

	qp->s_cur = next_cur;
	extra_bytes = -wqe->length & 3;
	nwords = (wqe->length + extra_bytes) >> 2;

	/* header size in 32-bit words LRH+BTH+DETH = (8+12+8)/4. */
	qp->s_hdrwords = 7;
	qp->s_cur_size = wqe->length;
	qp->s_cur_sge = &qp->s_sge;
	qp->s_srate = ah_attr->static_rate;
	qp->s_wqe = wqe;
	qp->s_sge.sge = wqe->sg_list[0];
	qp->s_sge.sg_list = wqe->sg_list + 1;
	qp->s_sge.num_sge = wqe->wr.num_sge;
	qp->s_sge.total_len = wqe->length;

	if (ah_attr->ah_flags & IB_AH_GRH) {
		/* Header size in 32-bit words. */
		qp->s_hdrwords += qib_make_grh(ibp, &qp->s_hdr->u.l.grh,
					       &ah_attr->grh,
					       qp->s_hdrwords, nwords);
		lrh0 = QIB_LRH_GRH;
		ohdr = &qp->s_hdr->u.l.oth;
		/*
		 * Don't worry about sending to locally attached multicast
		 * QPs.  It is unspecified by the spec. what happens.
		 */
	} else {
		/* Header size in 32-bit words. */
		lrh0 = QIB_LRH_BTH;
		ohdr = &qp->s_hdr->u.oth;
	}
	if (wqe->wr.opcode == IB_WR_SEND_WITH_IMM) {
		qp->s_hdrwords++;
		ohdr->u.ud.imm_data = wqe->wr.ex.imm_data;
		bth0 = IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE << 24;
	} else
		bth0 = IB_OPCODE_UD_SEND_ONLY << 24;
	lrh0 |= ah_attr->sl << 4;
	if (qp->ibqp.qp_type == IB_QPT_SMI)
		lrh0 |= 0xF000; /* Set VL (see ch. 13.5.3.1) */
	else
		lrh0 |= ibp->sl_to_vl[ah_attr->sl] << 12;
	qp->s_hdr->lrh[0] = cpu_to_be16(lrh0);
	qp->s_hdr->lrh[1] = cpu_to_be16(ah_attr->dlid);  /* DEST LID */
	qp->s_hdr->lrh[2] = cpu_to_be16(qp->s_hdrwords + nwords + SIZE_OF_CRC);
	lid = ppd->lid;
	if (lid) {
		lid |= ah_attr->src_path_bits & ((1 << ppd->lmc) - 1);
		qp->s_hdr->lrh[3] = cpu_to_be16(lid);
	} else
		qp->s_hdr->lrh[3] = IB_LID_PERMISSIVE;
	if (wqe->wr.send_flags & IB_SEND_SOLICITED)
		bth0 |= IB_BTH_SOLICITED;
	bth0 |= extra_bytes << 20;
	bth0 |= qp->ibqp.qp_type == IB_QPT_SMI ? QIB_DEFAULT_P_KEY :
		qib_get_pkey(ibp, qp->ibqp.qp_type == IB_QPT_GSI ?
			     wqe->ud_wr.pkey_index : qp->s_pkey_index);
	ohdr->bth[0] = cpu_to_be32(bth0);
	/*
	 * Use the multicast QP if the destination LID is a multicast LID.
	 */
	ohdr->bth[1] = ah_attr->dlid >= QIB_MULTICAST_LID_BASE &&
		ah_attr->dlid != QIB_PERMISSIVE_LID ?
		cpu_to_be32(QIB_MULTICAST_QPN) :
		cpu_to_be32(wqe->ud_wr.remote_qpn);
	ohdr->bth[2] = cpu_to_be32(qp->s_next_psn++ & QIB_PSN_MASK);
	/*
	 * Qkeys with the high order bit set mean use the
	 * qkey from the QP context instead of the WR (see 10.2.5).
	 */
	ohdr->u.ud.deth[0] = cpu_to_be32((int)wqe->ud_wr.remote_qkey < 0 ?
					 qp->qkey : wqe->ud_wr.remote_qkey);
	ohdr->u.ud.deth[1] = cpu_to_be32(qp->ibqp.qp_num);

done:
	ret = 1;
	goto unlock;

bail:
	qp->s_flags &= ~QIB_S_BUSY;
unlock:
	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

static unsigned qib_lookup_pkey(struct qib_ibport *ibp, u16 pkey)
{
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_devdata *dd = ppd->dd;
	unsigned ctxt = ppd->hw_pidx;
	unsigned i;

	pkey &= 0x7fff;	/* remove limited/full membership bit */

	for (i = 0; i < ARRAY_SIZE(dd->rcd[ctxt]->pkeys); ++i)
		if ((dd->rcd[ctxt]->pkeys[i] & 0x7fff) == pkey)
			return i;

	/*
	 * Should not get here, this means hardware failed to validate pkeys.
	 * Punt and return index 0.
	 */
	return 0;
}

/**
 * qib_ud_rcv - receive an incoming UD packet
 * @ibp: the port the packet came in on
 * @hdr: the packet header
 * @has_grh: true if the packet has a GRH
 * @data: the packet data
 * @tlen: the packet length
 * @qp: the QP the packet came on
 *
 * This is called from qib_qp_rcv() to process an incoming UD packet
 * for the given QP.
 * Called at interrupt level.
 */
void qib_ud_rcv(struct qib_ibport *ibp, struct qib_ib_header *hdr