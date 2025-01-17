 */
#define QSFP_VEND_OFFS 148
#define QSFP_VEND_LEN 16
/* Byte 164 is IB Extended tranceiver codes Bits D0..3 are SDR,DDR,QDR,EDR */
#define QSFP_IBXCV_OFFS 164
/* Bytes 165..167 are Vendor OUI number */
#define QSFP_VOUI_OFFS 165
#define QSFP_VOUI_LEN 3
/* Bytes 168..183 are Vendor Part Number, string */
#define QSFP_PN_OFFS 168
#define QSFP_PN_LEN 16
/* Bytes 184,185 are Vendor Rev. Left Justified, Blank-filled */
#define QSFP_REV_OFFS 184
#define QSFP_REV_LEN 2
/*
 * Bytes 186,187 are Wavelength, if Optical. Not Qlogic req'd
 *  If copper, they are attenuation in dB:
 * Byte 186 is at 2.5Gb/sec (SDR), Byte 187 at 5.0Gb/sec (DDR)
 */
#define QSFP_ATTEN_OFFS 186
#define QSFP_ATTEN_LEN 2
/* Bytes 188,189 are Wavelength tolerance, not QLogic req'd */
/* Byte 190 is Max Case Temp. Not QLogic req'd */
/* Byte 191 is LSB of sum of bytes 128..190. Not QLogic req'd */
#define QSFP_CC_OFFS 191
/* Bytes 192..195 are Options implemented in qsfp. Not Qlogic req'd */
/* Bytes 196..211 are Serial Number, String */
#define QSFP_SN_OFFS 196
#define QSFP_SN_LEN 16
/* Bytes 212..219 are date-code YYMMDD (MM==1 for Jan) */
#define QSFP_DATE_OFFS 212
#define QSFP_DATE_LEN 6
/* Bytes 218,219 are optional lot-code, string */
#define QSFP_LOT_OFFS 218
#define QSFP_LOT_LEN 2
/* Bytes 220, 221 indicate monitoring options, Not QLogic req'd */
/* Byte 223 is LSB of sum of bytes 192..222 */
#define QSFP_CC_EXT_OFFS 223

/*
 * struct qib_qsfp_data encapsulates state of QSFP device for one port.
 * it will be part of port-chip-specific data if a board supports QSFP.
 *
 * Since multiple board-types use QSFP, and their pport_data structs
 * differ (in the chip-specific section), we need a pointer to its head.
 *
 * Avoiding premature optimization, we will have one work_struct per port,
 * and let the (increasingly inaccurately named) eep_lock arbitrate
 * access to common resources.
 *
 */

/*
 * Hold the parts of the onboard EEPROM that we care about, so we aren't
 * coonstantly bit-boffing
 */
struct qib_qsfp_cache {
	u8 id;	/* must be 0x0C or 0x0D; 0 indicates invalid EEPROM read */
	u8 pwr; /* in D6,7 */
	u8 len;	/* in meters, Cu only */
	u8 tech;
	char vendor[QSFP_VEND_LEN];
	u8 xt_xcv; /* Ext. tranceiver codes, 4 lsbs are IB speed supported */
	u8 oui[QSFP_VOUI_LEN];
	u8 partnum[QSFP_PN_LEN];
	u8 rev[QSFP_REV_LEN];
	u8 atten[QSFP_ATTEN_LEN];
	u8 cks1;	/* Checksum of bytes 128..190 */
	u8 serial[QSFP_SN_LEN];
	u8 date[QSFP_DATE_LEN];
	u8 lot[QSFP_LOT_LEN];
	u8 cks2;	/* Checsum of bytes 192..222 */
};

#define QSFP_PWR(pbyte) (((pbyte) >> 6) & 3)
#define QSFP_ATTEN_SDR(attenarray) (attenarray[0])
#define QSFP_ATTEN_DDR(attenarray) (attenarray[1])

struct qib_qsfp_data {
	/* Helps to find our way */
	struct qib_pportdata *ppd;
	struct work_struct work;
	struct qib_qsfp_cache cache;
	unsigned long t_insert;
	u8 modpresent;
};

extern int qib_refresh_qsfp_cache(struct qib_pportdata *ppd,
				  struct qib_qsfp_cache *cp);
extern int qib_qsfp_mod_present(struct qib_pportdata *ppd);
extern void qib_qsfp_init(struct qib_qsfp_data *qd,
			  void (*fevent)(struct work_struct *));
extern void qib_qsfp_deinit(struct qib_qsfp_data *qd);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
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

#include <linux/io.h>

#include "qib.h"

/* cut down ridiculously long IB macro names */
#define OP(x) IB_OPCODE_RC_##x

static void rc_timeout(unsigned long arg);

static u32 restart_sge(struct qib_sge_state *ss, struct qib_swqe *wqe,
		       u32 psn, u32 pmtu)
{
	u32 len;

	len = ((psn - wqe->psn) & QIB_PSN_MASK) * pmtu;
	ss->sge = wqe->sg_list[0];
	ss->sg_list = wqe->sg_list + 1;
	ss->num_sge = wqe->wr.num_sge;
	ss->total_len = wqe->length;
	qib_skip_sge(ss, len, 0);
	return wqe->length - len;
}

static void start_timer(struct qib_qp *qp)
{
	qp->s_flags |= QIB_S_TIMER;
	qp->s_timer.function = rc_timeout;
	/* 4.096 usec. * (1 << qp->timeout) */
	qp->s_timer.expires = jiffies + qp->timeout_jiffies;
	add_timer(&qp->s_timer);
}

/**
 * qib_make_rc_ack - construct a response packet (ACK, NAK, or RDMA read)
 * @dev: the device for this QP
 * @qp: a pointer to the QP
 * @ohdr: a pointer to the IB header being constructed
 * @pmtu: the path MTU
 *
 * Return 1 if constructed; otherwise, return 0.
 * Note that we are in the responder's side of the QP context.
 * Note the QP s_lock must be held.
 */
static int qib_make_rc_ack(struct qib_ibdev *dev, struct qib_qp *qp,
			   struct qib_other_headers *ohdr, u32 pmtu)
{
	struct qib_ack_entry *e;
	u32 hwords;
	u32 len;
	u32 bth0;
	u32 bth2;

	/* Don't send an ACK if we aren't supposed to. */
	if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK))
		goto bail;

	/* header size in 32-bit words LRH+BTH = (8+12)/4. */
	hwords = 5;

	switch (qp->s_ack_state) {
	case OP(RDMA_READ_RESPONSE_LAST):
	case OP(RDMA_READ_RESPONSE_ONLY):
		e = &qp->s_ack_queue[qp->s_tail_ack_queue];
		if (e->rdma_sge.mr) {
			qib_put_mr(e->rdma_sge.mr);
			e->rdma_sge.mr = NULL;
		}
		/* FALLTHROUGH */
	case OP(ATOMIC_ACKNOWLEDGE):
		/*
		 * We can increment the tail pointer now that the last
		 * response has been sent instead of only being
		 * constructed.
		 */
		if (++qp->s_tail_ack_queue > QIB_MAX_RDMA_ATOMIC)
			qp->s_tail_ack_queue = 0;
		/* FALLTHROUGH */
	case OP(SEND_ONLY):
	case OP(ACKNOWLEDGE):
		/* Check for no next entry in the queue. */
		if (qp->r_head_ack_queue == qp->s_tail_ack_queue) {
			if (qp->s_flags & QIB_S_ACK_PENDING)
				goto normal;
			goto bail;
		}

		e = &qp->s_ack_queue[qp->s_tail_ack_queue];
		if (e->opcode == OP(RDMA_READ_REQUEST)) {
			/*
			 * If a RDMA read response is being resent and
			 * we haven't seen the duplicate request yet,
			 * then stop sending the remaining responses the
			 * responder has seen until the requester resends it.
			 */
			len = e->rdma_sge.sge_length;
			if (len && !e->rdma_sge.mr) {
				qp->s_tail_ack_queue = qp->r_head_ack_queue;
				goto bail;
			}
			/* Copy SGE state in case we need to resend */
			qp->s_rdma_mr = e->rdma_sge.mr;
			if (qp->s_rdma_mr)
				qib_get_mr(qp->s_rdma_mr);
			qp->s_ack_rdma_sge.sge = e->rdma_sge;
			qp->s_ack_rdma_sge.num_sge = 1;
			qp->s_cur_sge = &qp->s_ack_rdma_sge;
			if (len > pmtu) {
				len = pmtu;
				qp->s_ack_state = OP(RDMA_READ_RESPONSE_FIRST);
			} else {
				qp->s_ack_state = OP(RDMA_READ_RESPONSE_ONLY);
				e->sent = 1;
			}
			ohdr->u.aeth = qib_compute_aeth(qp);
			hwords++;
			qp->s_ack_rdma_psn = e->psn;
			bth2 = qp->s_ack_rdma_psn++ & QIB_PSN_MASK;
		} else {
			/* COMPARE_SWAP or FETCH_ADD */
			qp->s_cur_sge = NULL;
			len = 0;
			qp->s_ack_state = OP(ATOMIC_ACKNOWLEDGE);
			ohdr->u.at.aeth = qib_compute_aeth(qp);
			ohdr->u.at.atomic_ack_eth[0] =
				cpu_to_be32(e->atomic_data >> 32);
			ohdr->u.at.atomic_ack_eth[1] =
				cpu_to_be32(e->atomic_data);
			hwords += sizeof(ohdr->u.at) / sizeof(u32);
			bth2 = e->psn & QIB_PSN_MASK;
			e->sent = 1;
		}
		bth0 = qp->s_ack_state << 24;
		break;

	case OP(RDMA_READ_RESPONSE_FIRST):
		qp->s_ack_state = OP(RDMA_READ_RESPONSE_MIDDLE);
		/* FALLTHROUGH */
	case OP(RDMA_READ_RESPONSE_MIDDLE):
		qp->s_cur_sge = &qp->s_ack_rdma_sge;
		qp->s_rdma_mr = qp->s_ack_rdma_sge.sge.mr;
		if (qp->s_rdma_mr)
			qib_get_mr(qp->s_rdma_mr);
		len = qp->s_ack_rdma_sge.sge.sge_length;
		if (len > pmtu)
			len = pmtu;
		else {
			ohdr->u.aeth = qib_compute_aeth(qp);
			hwords++;
			qp->s_ack_state = OP(RDMA_READ_RESPONSE_LAST);
			e = &qp->s_ack_queue[qp->s_tail_ack_queue];
			e->sent = 1;
		}
		bth0 = qp->s_ack_state << 24;
		bth2 = qp->s_ack_rdma_psn++ & QIB_PSN_MASK;
		break;

	default:
normal:
		/*
		 * Send a regular ACK.
		 * Set the s_ack_state so we wait until after sending
		 * the ACK before setting s_ack_state to ACKNOWLEDGE
		 * (see above).
		 */
		qp->s_ack_state = OP(SEND_ONLY);
		qp->s_flags &= ~QIB_S_ACK_PENDING;
		qp->s_cur_sge = NULL;
		if (qp->s_nak_state)
			ohdr->u.aeth =
				cpu_to_be32((qp->r_msn & QIB_MSN_MASK) |
					    (qp->s_nak_state <<
					     QIB_AETH_CREDIT_SHIFT));
		else
			ohdr->u.aeth = qib_compute_aeth(qp);
		hwords++;
		len = 0;
		bth0 = OP(ACKNOWLEDGE) << 24;
		bth2 = qp->s_ack_psn & QIB_PSN_MASK;
	}
	qp->s_rdma_ack_cnt++;
	qp->s_hdrwords = hwords;
	qp->s_cur_size = len;
	qib_make_ruc_header(qp, ohdr, bth0, bth2);
	return 1;

bail:
	qp->s_ack_state = OP(ACKNOWLEDGE);
	qp->s_flags &= ~(QIB_S_RESP_PENDING | QIB_S_ACK_PENDING);
	return 0;
}

/**
 * qib_make_rc_req - construct a request packet (SEND, RDMA r/w, ATOMIC)
 * @qp: a pointer to the QP
 *
 * Return 1 if constructed; otherwise, return 0.
 */
int qib_make_rc_req(struct qib_qp *qp)
{
	struct qib_ibdev *dev = to_idev(qp->ibqp.device);
	struct qib_other_headers *ohdr;
	struct qib_sge_state *ss;
	struct qib_swqe *wqe;
	u32 hwords;
	u32 len;
	u32 bth0;
	u32 bth2;
	u32 pmtu = qp->pmtu;
	char newreq;
	unsigned long flags;
	int ret = 0;
	int delta;

	ohdr = &qp->s_hdr->u.oth;
	if (qp->remote_ah_attr.ah_flags & IB_AH_GRH)
		ohdr = &qp->s_hdr->u.l.oth;

	/*
	 * The lock is needed to synchronize between the sending tasklet,
	 * the receive interrupt handler, and timeout resends.
	 */
	spin_lock_irqsave(&qp->s_lock, flags);

	/* Sending responses has higher priority over sending requests. */
	if ((qp->s_flags & QIB_S_RESP_PENDING) &&
	    qib_make_rc_ack(dev, qp, ohdr, pmtu))
		goto done;

	if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_SEND_OK)) {
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
		qib_send_complete(qp, wqe, qp->s_last != qp->s_acked ?
			IB_WC_SUCCESS : IB_WC_WR_FLUSH_ERR);
		/* will get called again */
		goto done;
	}

	if (qp->s_flags & (QIB_S_WAIT_RNR | QIB_S_WAIT_ACK))
		goto bail;

	if (qib_cmp24(qp->s_psn, qp->s_sending_hpsn) <= 0) {
		if (qib_cmp24(qp->s_sending_psn, qp->s_sending_hpsn) <= 0) {
			qp->s_flags |= QIB_S_WAIT_PSN;
			goto bail;
		}
		qp->s_sending_psn = qp->s_psn;
		qp->s_sending_hpsn = qp->s_psn - 1;
	}

	/* header size in 32-bit words LRH+BTH = (8+12)/4. */
	hwords = 5;
	bth0 = 0;

	/* Send a request. */
	wqe = get_swqe_ptr(qp, qp->s_cur);
	switch (qp->s_state) {
	default:
		if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_NEXT_SEND_OK))
			goto bail;
		/*
		 * Resend an old request or start a new one.
		 *
		 * We keep track of the current SWQE so that
		 * we don't