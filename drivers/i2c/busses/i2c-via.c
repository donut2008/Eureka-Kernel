oid *data, u32 tlen, struct qib_qp *qp)
{
	struct qib_other_headers *ohdr;
	int opcode;
	u32 hdrsize;
	u32 pad;
	struct ib_wc wc;
	u32 qkey;
	u32 src_qp;
	u16 dlid;

	/* Check for GRH */
	if (!has_grh) {
		ohdr = &hdr->u.oth;
		hdrsize = 8 + 12 + 8;   /* LRH + BTH + DETH */
	} else {
		ohdr = &hdr->u.l.oth;
		hdrsize = 8 + 40 + 12 + 8; /* LRH + GRH + BTH + DETH */
	}
	qkey = be32_to_cpu(ohdr->u.ud.deth[0]);
	src_qp = be32_to_cpu(ohdr->u.ud.deth[1]) & QIB_QPN_MASK;

	/*
	 * Get the number of bytes the message was padded by
	 * and drop incomplete packets.
	 */
	pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
	if (unlikely(tlen < (hdrsize + pad + 4)))
		goto drop;

	tlen -= hdrsize + pad + 4;

	/*
	 * Check that the permissive LID is only used on QP0
	 * and the QKEY matches (see 9.6.1.4.1 and 9.6.1.5.1).
	 */
	if (qp->ibqp.qp_num) {
		if (unlikely(hdr->lrh[1] == IB_LID_PERMISSIVE ||
			     hdr->lrh[3] == IB_LID_PERMISSIVE))
			goto drop;
		if (qp->ibqp.qp_num > 1) {
			u16 pkey1, pkey2;

			pkey1 = be32_to_cpu(ohdr->bth[0]);
			pkey2 = qib_get_pkey(ibp, qp->s_pkey_index);
			if (unlikely(!qib_pkey_ok(pkey1, pkey2))) {
				qib_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_PKEY,
					      pkey1,
					      (be16_to_cpu(hdr->lrh[0]) >> 4) &
						0xF,
					      src_qp, qp->ibqp.qp_num,
					      hdr->lrh[3], hdr->lrh[1]);
				return;
			}
		}
		if (unlikely(qkey != qp->qkey)) {
			qib_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_QKEY, qkey,
				      (be16_to_cpu(hdr->lrh[0]) >> 4) & 0xF,
				      src_qp, qp->ibqp.qp_num,
				      hdr->lrh[3], hdr->lrh[1]);
			return;
		}
		/* Drop invalid MAD packets (see 13.5.3.1). */
		if (unlikely(qp->ibqp.qp_num == 1 &&
			     (tlen != 256 ||
			      (be16_to_cpu(hdr->lrh[0]) >> 12) == 15)))
			goto drop;
	} else {
		struct ib_smp *smp;

		/* Drop invalid MAD packets (see 13.5.3.1). */
		if (tlen != 256 || (be16_to_cpu(hdr->lrh[0]) >> 12) != 15)
			goto drop;
		smp = (struct ib_smp *) data;
		if ((hdr->lrh[1] == IB_LID_PERMISSIVE ||
		     hdr->lrh[3] == IB_LID_PERMISSIVE) &&
		    smp->mgmt_class != IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
			goto drop;
	}

	/*
	 * The opcode is in the low byte when its in network order
	 * (top byte when in host order).
	 */
	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	if (qp->ibqp.qp_num > 1 &&
	    opcode == IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE) {
		wc.ex.imm_data = ohdr->u.ud.imm_data;
		wc.wc_flags = IB_WC_WITH_IMM;
	} else if (opcode == IB_OPCODE_UD_SEND_ONLY) {
		wc.ex.imm_data = 0;
		wc.wc_flags = 0;
	} else
		goto drop;

	/*
	 * A GRH is expected to precede the data even if not
	 * present on the wire.
	 */
	wc.byte_len = tlen + sizeof(struct ib_grh);

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
			return;
		}
		if (!ret) {
			if (qp->ibqp.qp_num == 0)
				ibp->n_vl15_dropped++;
			return;
		}
	}
	/* Silently drop packets which are too big. */
	if (unlikely(wc.byte_len > qp->r_len)) {
		qp->r_flags |= QIB_R_REUSE_SGE;
		goto drop;
	}
	if (has_grh) {
		qib_copy_sge(&qp->r_sge, &hdr->u.l.grh,
			     sizeof(struct ib_grh), 1);
		wc.wc_flags |= IB_WC_GRH;
	} else
		qib_skip_sge(&qp->r_sge, sizeof(struct ib_grh), 1);
	qib_copy_sge(&qp->r_sge, data, wc.byte_len - sizeof(struct ib_grh), 1);
	qib_put_ss(&qp->r_sge);
	if (!test_and_clear_bit(QIB_R_WRID_VALID, &qp->r_aflags))
		return;
	wc.wr_id = qp->r_wr_id;
	wc.status = IB_WC_SUCCESS;
	wc.opcode = IB_WC_RECV;
	wc.vendor_err = 0;
	wc.qp = &qp->ibqp;
	wc.src_qp = src_qp;
	wc.pkey_index = qp->ibqp.qp_type == IB_QPT_GSI ?
		qib_lookup_pkey(ibp, be32_to_cpu(ohdr->bth[0])) : 0;
	wc.slid = be16_to_cpu(hdr->lrh[3]);
	wc.sl = (be16_to_cpu(hdr->lrh[0]) >> 4) & 0xF;
	dlid = be16_to_cpu(hdr->lrh[1]);
	/*
	 * Save the LMC lower bits if the destination LID is a unicast LID.
	 */
	wc.dlid_path_bits = dlid >= QIB_MULTICAST_LID_BASE ? 0 :
		dlid & ((1 << ppd_from