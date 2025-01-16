  IB_ACCESS_REMOTE_WRITE)))
			goto acc_err;
		qp->r_sge.sg_list = NULL;
		qp->r_sge.num_sge = 1;
		qp->r_sge.total_len = wqe->length;
		break;

	case IB_WR_RDMA_READ:
		if (unlikely(!(qp->qp_access_flags & IB_ACCESS_REMOTE_READ)))
			goto inv_err;
		if (unlikely(!qib_rkey_ok(qp, &sqp->s_sge.sge, wqe->length,
					  wqe->rdma_wr.remote_addr,
					  wqe->rdma_wr.rkey,
					  IB_ACCESS_REMOTE_READ)))
			goto acc_err;
		release = 0;
		sqp->s_sge.sg_list = NULL;
		sqp->s_sge.num_sge = 1;
		qp->r_sge.sge = wqe->sg_list[0];
		qp->r_sge.sg_list = wqe->sg_list + 1;
		qp->r_sge.num_sge = wqe->wr.num_sge;
		qp->r_sge.total_len = wqe->length;
		break;

	case IB_WR_ATOMIC_CMP_AND_SWP:
	case IB_WR_ATOMIC_FETCH_AND_ADD:
		if (unlikely(!(qp->qp_access_flags & IB_ACCESS_REMOTE_ATOMIC)))
			goto inv_err;
		if (unlikely(!qib_rkey_ok(qp, &qp->r_sge.sge, sizeof(u64),
					  wqe->atomic_wr.remote_addr,
					  wqe->atomic_wr.rkey,
					  IB_ACCESS_REMOTE_ATOMIC)))
			goto acc_err;
		/* Perform atomic OP and save result. */
		maddr = (atomic64_t *) qp->r_sge.sge.vaddr;
		sdata = wqe->atomic_wr.compare_add;
		*(u64 *) sqp->s_sge.sge.vaddr =
			(wqe->atomic_wr.wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD) ?
			(u64) atomic64_add_return(sdata, maddr) - sdata :
			(u64) cmpxchg((u64 *) qp->r_sge.sge.vaddr,
				      sdata, wqe->atomic_wr.swap);
		qib_put_mr(qp->r_sge.sge.mr);
		qp->r_sge.num_sge = 0;
		goto send_comp;

	default:
		send_status = IB_WC_LOC_QP_OP_ERR;
		goto serr;
	}

	sge = &sqp->s_sge.sge;
	while (sqp->s_len) {
		u32 len = sqp->s_len;

		if (len > sge->length)
			len = sge->length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		qib_copy_sge(&qp->r_sge, sge->vaddr, len, release);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (!release)
				qib_put_mr(sge->mr);
			if (--sqp->s_sge.num_sge)
				*sge = *sqp->s_sge.sg_list++;
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
		sqp->s_len -= len;
	}
	if (release)
		qib_put_ss(&qp->r_sge);

	if (!test_and_clear_bit(QIB_R_WRID_VALID, &qp->r_aflags))
		goto send_comp;

	if (wqe->wr.opcode == IB_WR_RDMA_WRITE_WITH_IMM)
		wc.opcode = IB_WC_RECV_RDMA_WITH_IMM;
	else
		wc.opcode = IB_WC_RECV;
	wc.wr_id = qp->r_wr_id;
	wc.status = IB_WC_SUCCESS;
	wc.byte_len = wqe->length;
	wc.qp = &qp->ibqp;
	wc.src_qp = qp->remote_qpn;
	wc.slid = qp->remote_ah_attr.dlid;
	wc.sl = qp->remote_ah_attr.sl;
	wc.port_num = 1;
	/* Signal completion event if the solicited bit is set. */
	qib_cq_enter(to_icq(qp->ibqp.recv_cq), &wc,
		       wqe->wr.send_flags & IB_SEND_SOLICITED);

send_comp:
	spin_lock_irqsave(&sqp->s_lock, flags);
	ibp->n_loop_pkts++;
flush_send:
	sqp->s_rnr_retry = sqp->s_rnr_retry_cnt;
	qib_send_complete(sqp, wqe, send_status);
	goto again;

rnr_nak:
	/* Handle RNR NAK */
	if (qp->ibqp.qp_type == IB_QPT_UC)
		goto send_comp;
	ibp->n_rnr_naks++;
	/*
	 * Note: we don't need the s_lock held since the BUSY flag
	 * makes this single threaded.
	 */
	if (sqp->s_rnr_retry == 0) {
		send_status = IB_WC_RNR_RETRY_EXC_ERR;
		goto serr;
	}
	if (sqp->s_rnr_retry_cnt < 7)
		sqp->s_rnr_retry--;
	spin_lock_irqsave(&sqp->s_lock, flags);
	if (!(ib_qib_state_ops[sqp->state] & QIB_PROCESS_RECV_OK))
		goto clr_busy;
	sqp->s_flags |= QIB_S_WAIT_RNR;
	sqp->s_timer.function = qib_rc_rnr_retry;
	sqp->s_timer.expires = jiffies +
		usecs_to_jiffies(ib_qib_rnr_table[qp->r_min_rnr_timer]);
	add_timer(&sqp->s_timer);
	goto clr_busy;

op_err:
	send_status = IB_WC_REM_OP_ERR;
	wc.status = IB_WC_LOC_QP_OP_ERR;
	goto err;

inv_err:
	send_status = IB_WC_REM_INV_REQ_ERR;
	wc.status = IB_WC_LOC_QP_OP_ERR;
	goto err;

acc_err:
	send_status = IB_WC_REM_ACCESS_ERR;
	wc.status = IB_WC_LOC_PROT_ERR;
err:
	/* responder goes to error state */
	qib_rc_error(qp, wc.status);

serr:
	spin_lock_irqsave(&sqp->s_lock, flags);
	qib_send_complete(sqp, wqe, send_status);
	if (sqp->ibqp.qp_type == IB_QPT_RC) {
		int lastwqe = qib_error_qp(sqp, IB_WC_WR_FLUSH_ERR);

		sqp->s_flags &= ~QIB_S_BUSY;
		spin_unlock_irqrestore(&sqp->s_lock, flags);
		if (lastwqe) {
			struct ib_event ev;

			ev.device = sqp->ibqp.device;
			ev.element.qp = &sqp->ibqp;
			ev.event = IB_EVENT_QP_LAST_WQE_REACHED;
			sqp->ibqp.event_handler(&ev, sqp->ibqp.qp_context);
		}
		goto done;
	}
clr_busy:
	sqp->s_flags &= ~QIB_S_BUSY;
unlock:
	spin_unlock_irqrestore(&sqp->s_lock, flags);
done:
	if (qp && atomic_dec_and_test(&qp->refcount))
		wake_up(&qp->wait);
}

/**
 * qib_make_grh - construct a GRH header
 * @ibp: a pointer to the IB port
 * @hdr: a pointer to the GRH header being constructed
 * @grh: the global route address to send to
 * @hwords: the number of 32 bit words of header being sent
 * @nwords: the number of 32 bit words of data being sent
 *
 * Return the size of the header in 32 bit words.
 */
u32 qib_make_grh(struct qib_ibport *ibp, struct ib_grh *hdr,
		 struct ib_global_route *grh, u32 hwords, u32 nwords)
{
	hdr->version_tclass_flow =
		cpu_to_be32((IB_GRH_VERSION << IB_GRH_VERSION_SHIFT) |
			    (grh->traffic_class << IB_GRH_TCLASS_SHIFT) |
			    (grh->flow_label << IB_GRH_FLOW_SHIFT));
	hdr->paylen = cpu_to_be16((hwords - 2 + nwords + SIZE_OF_CRC) << 2);
	/* next_hdr is defined by C8-7 in ch. 8.4.1 */
	hdr->next_hdr = IB_GRH_NEXT_HDR;
	hdr->hop_limit = grh->hop_limit;
	/* The SGID is 32-bit aligned. */
	hdr->sgid.global.subnet_prefix = ibp->gid_prefix;
	hdr->sgid.global.interface_id = grh->sgid_index ?
		ibp->guids[grh->sgid_index - 1] : ppd_from_ibp(ibp)->guid;
	hdr->dgid = grh->dgid;

	/* GRH header size in 32-bit words. */
	return sizeof(struct ib_grh) / sizeof(u32);
}

void qib_make_ruc_header(struct qib_qp *qp, struct qib_other_headers *ohdr,
			 u32 bth0, u32 bth2)
{
	struct qib_ibport *ibp = to_iport(qp->ibqp.device, qp->port_num);
	u16 lrh0;
	u32 nwords;
	u32 extra_bytes;

	/* Construct the header. */
	extra_bytes = -qp->s_cur_size & 3;
	nwords = (qp->s_cur_size + extra_bytes) >> 2;
	lrh0 = QIB_LRH_BTH;
	if (unlikely(qp->remote_ah_attr.ah_flags & IB_AH_GRH)) {
		qp->s_hdrwords += qib_make_grh(ibp, &qp->s_hdr->u.l.grh,
					       &qp->remote_ah_attr.grh,
					       qp->s_hdrwords, nwords);
		lrh0 = QIB_LRH_GRH;
	}
	lrh0 |= ibp->sl_to_vl[qp->remote_ah_attr.sl] << 12 |
		qp->remote_ah_attr.sl << 4;
	qp->s_hdr->lrh[0] = cpu_to_be16(lrh0);
	qp->s_hdr->lrh[1] = cpu_to_be16(qp->remote_ah_attr.dlid);
	qp->s_hdr->lrh[2] = cpu_to_be16(qp->s_hdrwords + nwords + SIZE_OF_CRC);
	qp->s_hdr->lrh[3] = cpu_to_be16(ppd_from_ibp(ibp)->lid |
				       qp->remote_ah_attr.src_path_bits);
	bth0 |= qib_get_pkey(ibp, qp->s_pkey_index);
	bth0 |= extra_bytes << 20;
	if (qp->s_mig_state == IB_MIG_MIGRATED)
		bth0 |= IB_BTH_MIG_REQ;
	ohdr->bth[0] = cpu_to_be32(bth0);
	ohdr->bth[1] = cpu_to_be32(qp->remote_qpn);
	ohdr->bth[2] = cpu_to_be32(bth2);
	this_cpu_inc(ibp->pmastats->n_unicast_xmit);
}

/**
 * qib_do_send - perform a send on a QP
 * @work: contains a pointer to the QP
 *
 * Process entries in the send work queue until credit or queue is
 * exhausted.  Only allow one CPU to send a packet per QP (tasklet).
 * Otherwise, two threads could send packets out of order.
 */
void qib_do_send(struct work_struct *work)
{
	struct qib_qp *qp = container_of(work, struct qib_qp, s_work);
	struct qib_ibport *ibp = to_iport(qp->ibqp.device, qp->port_num);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int (*make_req)(struct qib_qp *qp);
	unsigned long flags;

	if ((qp->ibqp.qp_type == IB_QPT_RC ||
	     qp->ibqp.qp_type == IB_QPT_UC) &&
	    (qp->remote_ah_attr.dlid & ~((1 << ppd->lmc) - 1)) == ppd->lid) {
		qib_ruc_loopback(qp);
		return;
	}

	if (qp->ibqp.qp_type == IB_QPT_RC)
		make_req = qib_make_rc_req;
	else if (qp->ibqp.qp_type == IB_QPT_UC)
		make_req = qib_make_uc_req;
	else
		make_req = qib_make_ud_req;

	spin_lock_irqsave(&qp->s_lock, flags);

	/* Return if we are already busy processing a work request. */
	if (!qib_send_ok(qp)) {
		spin_unlock_irqrestore(&qp->s_lock, flags);
		return;
	}

	qp->s_flags |= QIB_S_BUSY;

	spin_unlock_irqrestore(&qp->s_lock, flags);

	do {
		/* Check for a constructed packet to be sent. */
		if (qp->s_hdrwords != 0) {
			/*
			 * If the packet cannot be sent now, return and
			 * the send tasklet will be woken up later.
			 */
			if (qib_verbs_send(qp, qp->s_hdr, qp->s_hdrwords,
					   qp->s_cur_sge, qp->s_cur_size))
				break;
			/* Record that s_hdr is empty. */
			qp->s_hdrwords = 0;
		}
	} while (make_req(qp));
}

/*
 * This should be called with s_lock held.
 */
void qib_send_complete(struct qib_qp *qp, struct qib_swqe *wqe,
		       enum ib_wc_status status)
{
	u32 old_last, last;
	unsigned i;

	if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_OR_FLUSH_SEND))
		return;

	for (i = 0; i < wqe->wr.num_sge; i++) {
		struct qib_sge *sge = &wqe->sg_list[i];

		qib_put_mr(sge->mr);
	}
	if (qp->ibqp.qp_type == IB_QPT_UD ||
	    qp->ibqp.qp_type == IB_QPT_SMI ||
	    qp->ibqp.qp_type == IB_QPT_GSI)
		atomic_dec(&to_iah(wqe->ud_wr.ah)->refcount);

	/* See ch. 11.2.4.1 and 10.7.3.1 */
	if (!(qp->s_flags & QIB_S_SIGNAL_REQ_WR) ||
	    (wqe->wr.send_flags & IB_SEND_SIGNALED) ||
	    status != IB_WC_SUCCESS) {
		struct ib_wc wc;

		memset(&wc, 0, sizeof(wc));
		wc.wr_id = wqe->wr.wr_id;
		wc.status = status;
		wc.opcode = ib_qib_wc_opcode[wqe->wr.opcode];
		wc.qp = &qp->ibqp;
		if (status == IB_WC_SUCCESS)
			wc.byte_len = wqe->length;
		qib_cq_enter(to_icq(qp->ibqp.send_cq), &wc,
			     status != IB_WC_SUCCESS);
	}

	last = qp->s_last;
	old_last = last;
	if (++last >= qp->s_size)
		last = 0;
	qp->s_last = last;
	if (qp->s_acked == old_last)
		qp->s_acked = last;
	if (qp->s_cur == old_last)
		qp->s_cur = last;
	if (qp->s_tail == old_last)
		qp->s_tail = last;
	if (qp->state == IB_QPS_SQD && last == qp->s_cur)
		qp->s_draining = 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
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
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * This file contains all of the code that is specific to the SerDes
 * on the QLogic_IB 7220 chip.
 */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/firmware.h>

#include "qib.h"
#include "qib_7220.h"

#define SD7220_FW_NAME "qlogic/sd7220.fw"
MODULE_FIRMWARE(SD7220_FW_NAME);

/*
 * Same as in qib_iba7220.c, but just the registers needed here.
 * Could move whole set to qib_7220.h, but decided better to keep
 * local.
 */
#define KREG_IDX(regname) (QIB_7220_##regname##_OFFS / sizeof(u64))
#define kr_hwerrclear KREG_IDX(HwErrClear)
#define kr_hwerrmask KREG_IDX(HwErrMask)
#define kr_hwerrstatus KREG_IDX(HwErrStatus)
#define kr_ibcstatus KREG_IDX(IBCStatus)
#define kr_ibserdesctrl KREG_IDX(IBSerDesCtrl)
#define kr_scratch KREG_IDX(Scratch)
#define kr_xgxs_cfg KREG_IDX(XGXSCfg)
/* these are used only here, not in qib_iba7220.c */
#define kr_ibsd_epb_access_ctrl KREG_IDX(ibsd_epb_access_ctrl)
#define kr_ibsd_epb_transaction_reg KREG_IDX(ibsd_epb_transaction_reg)
#define kr_pciesd_epb_transaction_reg KREG_IDX(pciesd_epb_transaction_reg)
#define kr_pciesd_epb_access_ctrl KREG_IDX(pciesd_epb_access_ctrl)
#define kr_serdes_ddsrxeq0 KREG_IDX(SerDes_DDSRXEQ0)

/*
 * The IBSerDesMappTable is a memory that holds values to be stored in
 * various SerDes registers by IBC.
 */
#define kr_serdes_maptable KREG_IDX(IBSerDesMappTable)

/*
 * Below used for sdnum parameter, selecting one of the two sections
 * used for PCIe, or the single SerDes used for IB.
 */
#define PCIE_SERDES0 0
#define PCIE_SERDES1 1

/*
 * The EPB requires addressing in a particular form. EPB_LOC() is intended
 * to make #definitions a little more readable.
 */
#define EPB_ADDR_SHF 8
#define EPB_LOC(chn, elt, reg) \
	(((elt & 0xf) | ((chn & 7) << 4) | ((reg & 0x3f) << 9)) << \
	 EPB_ADDR_SHF)
#define EPB_IB_QUAD0_CS_SHF (25)
#define EPB_IB_QUAD0_CS (1U <<  EPB_IB_QUAD0_CS_SHF)
#define EPB_IB_UC_CS_SHF (26)
#define EPB_PCIE_UC_CS_SHF (27)
#define EPB_GLOBAL_WR (1U << (EPB_ADDR_SHF + 8))

/* Forward declarations. */
static int qib_sd7220_reg_mod(struct qib_devdata *dd, int sdnum, u32 loc,
			      u32 data, u32 mask);
static int ibsd_mod_allchnls(struct qib_devdata *dd, int loc, int val,
			     int mask);
static int qib_sd_trimdone_poll(struct qib_devdata *dd);
static void qib_sd_trimdone_monitor(struct qib_devdata *dd, const char *where);
static int qib_sd_setvals(struct qib_devdata *dd);
static int qib_sd_early(struct qib_devdata *dd);
static int qib_sd_dactrim(struct qib_devdata *dd);
static int qib_internal_presets(struct qib_devdata *dd);
/* Tweak the register (CMUCTRL5) that contains the TRIMSELF controls */
static int qib_sd_trimself(struct qib_devdata *dd, int val);
static int epb_access(struct qib_devdata *dd, int sdnum, int claim);
static int qib_sd7220_ib_load(struct qib_devdata *dd,
			      const struct firmware *fw);
static int qib_sd7220_ib_vfy(struct qib_devdata *dd,
			     const struct firmware *fw);

/*
 * Below keeps track of whether the "once per power-on" initialization has
 * been done, because uC code Version 1.32.17 or higher allows the uC to
 * be reset at will, and Automatic Equalization may require it. So the
 * state of the reset "pin", is no longer valid. Instead, we check for the
 * actual uC code having been loaded.
 */
static int qib_ibsd_ucode_loaded(struct qib_pportdata *ppd,
				 const struct firmware *fw)
{
	struct qib_devdata *dd = ppd->dd;

	if (!dd->cspec->serdes_first_init_done &&
	    qib_sd7220_ib_vfy(dd, fw) > 0)
		dd->cspec->serdes_first_init_done = 1;
	return dd->cspec->serdes_first_init_done;
}

/* repeat #define for local use. "Real" #define is in qib_iba7220.c */
#define QLOGIC_IB_HWE_IB_UC_MEMORYPARITYERR      0x0000004000000000ULL
#define IB_MPREG5 (EPB_LOC(6, 0, 0xE) | (1L << EPB_IB_UC_CS_SHF))
#define IB_MPREG6 (EPB_LOC(6, 0, 0xF) | (1U << EPB_IB_UC_CS_SHF))
#define UC_PAR_CLR_D 8
#define UC_PAR_CLR_M 0xC
#define IB_CTRL2(chn) (EPB_LOC(chn, 7, 3) | EPB_IB_QUAD0_CS)
#define START_EQ1(chan) EPB_LOC(chan, 7, 0x27)

void qib_sd7220_clr_ibpar(struct qib_devdata *dd)
{
	