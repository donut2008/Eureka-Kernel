n))
			goto drop;
		qib_copy_sge(&qp->r_sge, data, pmtu, 1);
		break;

	case OP(RDMA_WRITE_LAST_WITH_IMMEDIATE):
		wc.ex.imm_data = ohdr->u.imm_data;
rdma_last_imm:
		hdrsize += 4;
		wc.wc_flags = IB_WC_WITH_IMM;

		/* Get the number of bytes the message was padded by. */
		pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
		/* Check for invalid length. */
		/* XXX LAST len should be >= 1 */
		if (unlikely(tlen < (hdrsize + pad + 4)))
			goto drop;
		/* Don't count the CRC. */
		tlen -= (hdrsize + pad + 4);
		if (unlikely(tlen + qp->r_rcv_len != qp->r_len))
			goto drop;
		if (test_and_clear_bit(QIB_R_REWIND_SGE, &qp->r_aflags))
			qib_put_ss(&qp->s_rdma_read_sge);
		else {
			ret = qib_get_rwqe(qp, 1);
			if (ret < 0)
				goto op_err;
			if (!ret)
				goto drop;
		}
		wc.byte_len = qp->r_len;
		wc.opcode = IB_WC_RECV_RDMA_WITH_IMM;
		qib_copy_sge(&qp->r_sge, data, tlen, 1);
		qib_put_ss(&qp->r_sge);
		goto last_imm;

	case OP(RDMA_WRITE_LAST):
rdma_last:
		/* Get the number of bytes the message was padded by. */
		pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
		/* Check for invalid length. */
		/* XXX LAST len should be >= 1 */
		if (unlikely(tlen < (hdrsize + pad + 4)))
			goto drop;
		/* Don't count the CRC. */
		tlen -= (hdrsize + pad + 4);
		if (unlikely(tlen + qp->r_rcv_len != qp->r_len))
			goto drop;
		qib_copy_sge(&qp->r_sge, data, tlen, 1);
		qib_put_ss(&qp->r_sge);
		break;

	default:
		/* Drop packet for unknown opcodes. */
		goto drop;
	}
	qp->r_psn++;
	qp->r_state = opcode;
	return;

rewind:
	set_bit(QIB_R_REWIND_SGE, &qp->r_aflags);
	qp->r_sge.num_sge = 0;
drop:
	ibp->n_pkt_drops++;
	return;

op_err:
	qib_rc_error(qp, IB_WC_LOC_QP_OP_ERR);
	return;

}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * Copyright (c) 2006, 2007, 2008, 2009 QLogic Corporation. All rights reserved.
 * Copyright (c) 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions 