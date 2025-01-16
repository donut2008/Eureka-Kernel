nly happen if relaxed ordering is
		 * used and requests are sent after an RDMA read or atomic
		 * is sent but before the response is received.
		 */
		if ((wqe->wr.opcode == IB_WR_RDMA_READ &&
		     (opcode != OP(RDMA_READ_RESPONSE_LAST) || diff != 0)) ||
		    ((wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		      wqe->wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD) &&
		     (opcode != OP(ATOMIC_ACKNOWLEDGE) || diff != 0))) {
			/* Retry this request. */
			if (!(qp->r_flags & QIB_R_RDMAR_SEQ)) {
				qp->r_flags |= QIB_R_RDMAR_SEQ;
				qib_restart_rc(qp, qp->s_last_psn + 1, 0);
				if (list_empty(&qp->rspwait)) {
					qp->r_flags |= QIB_R_RSP_SEND;
					atomic_inc(&qp->refcount);
					list_add_tail(&qp->rspwait,
						      &rcd->qp_wait_list);
				}
			}
			/*
			 * No need to process the ACK/NAK since we are
			 * restarting an earlier request.
			 */
			goto bail;
		}
		if (wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		    wqe->wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD) {
			u64 *vaddr = wqe->sg_list[0].vaddr;
			*vaddr = val;
		}
		if (qp->s_num_rd_atomic &&
		    (wqe->wr.opcode == IB_WR_RDMA_READ ||
		     wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		     wqe->wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD)) {
			qp->s_num_rd_atomic--;
			/* Restart sending task if fence is complete */
			if ((qp->s_flags & QIB_S_WAIT_FENCE) &&
			    !qp->s_num_rd_atomic) {
				qp->s_flags &= ~(QIB_S_WAIT_FENCE |
						 QIB_S_WAIT_ACK);
				qib_schedule_send(qp);
			} else if (qp->s_flags & QIB_S_WAIT_RDMAR) {
				qp->s_flags &= ~(QIB_S_WAIT_RDMAR |
						 QIB_S_WAIT_ACK);
				qib_schedule_send(qp);
			}
		}
		wqe = do_rc_completion(qp, wqe, ibp);
		if (qp->s_acked == qp->s_tail)
			break;
	}

	switch (aeth >> 29) {
	case 0:         /* ACK */
		ibp->n_rc_acks++;
		if (qp->s_acked != qp->s_tail) {
			/*
			 * We are expecting more ACKs so
			 * reset the retransmit timer.
			 */
			start_timer(qp);
			/*
			 * We can stop resending the earlier packets and
			 * continue with the next packet the receiver wants.
			 */
			if (qib_cmp24(qp->s_psn, psn) <= 0)
				reset_psn(qp, psn + 1);
		} else if (qib_cmp24(qp->s_psn, psn) <= 0) {
			qp->s_state = OP(SEND_LAST);
			qp->s_psn = psn + 1;
		}
		if (qp->s_flags & QIB_S_WAIT_ACK) {
			qp->s_flags &= ~QIB_S_WAIT_ACK;
			qib_schedule_send(qp);
		}
		qib_get_credit(qp, aeth);
		qp->s_rnr_retry = qp->s_rnr_retry_cnt;
		qp->s_retry = qp->s_retry_cnt;
		update_last_psn(qp, psn);
		ret = 1;
		goto bail;

	case 1:         /* RNR NAK */
		ibp->n_rnr_naks++;
		if (qp->s_acked == qp->s_tail)
			goto bail;
		if (qp->s_flags & QIB_S_WAIT_RNR)
			goto bail;
		if (qp->s_rnr_retry == 0) {
			status = IB_WC_RNR_RETRY_EXC_ERR;
			goto class_b;
		}
		if (qp->s_rnr_retry_cnt < 7)
			qp->s_rnr_retry--;

		/* The last valid PSN is the previous PSN. */
		update_last_psn(qp, psn - 1);

		ibp->n_rc_resends += (qp->s_psn - psn) & QIB_PSN_MASK;

		reset_psn(qp, psn);

		qp->s_flags &= ~(QIB_S_WAIT_SSN_CREDIT | QIB_S_WAIT_ACK);
		qp->s_flags |= QIB_S_WAIT_RNR;
		qp->s_timer.function = qib_rc_rnr_retry;
		qp->s_timer.expires = jiffies + usecs_to_jiffies(
			ib_qib_rnr_table[(aeth >> QIB_AETH_CREDIT_SHIFT) &
					   QIB_AETH_CREDIT_MASK]);
		add_timer(&qp->s_timer);
		goto bail;

	case 3:         /* NAK */
		if (qp->s_acked == qp->s_tail)
			goto bail;
		/* The last valid PSN is the previous PSN. */
		update_last_psn(qp, psn - 1);
		switch ((aeth >> QIB_AETH_CREDIT_SHIFT) &
			QIB_AETH_CREDIT_MASK) {
		case 0: /* PSN sequence error */
			ibp->n_seq_naks++;
			/*
			 * Back up to the responder's expected PSN.
			 * Note that we might get a NAK in the middle of an
			 * RDMA READ response which terminates the RDMA
			 * READ.
			 */
			qib_restart_rc(qp, psn, 0);
			qib_schedule_send(qp);
			break;

		case 1: /* Invalid Request */
			status = IB_WC_REM_INV_REQ_ERR;
			ibp->n_other_naks++;
			goto class_b;

		case 2: /* Remote Access Error */
			status = IB_WC_REM_ACCESS_ERR;
			ibp->n_other_naks++;
			goto class_b;

		case 3: /* Remote Operation Error */
			status = IB_WC_REM_OP_ERR;
			ibp->n_other_naks++;
class_b:
			if (qp->s_last == qp->s_acked) {
				qib_send_complete(qp, wqe, status);
				qib_error_qp(qp, IB_WC_WR_FLUSH_ERR);
			}
			break;

		default:
			/* Ignore other reserved NAK error codes */
			goto reserved;
		}
		qp->s_retry = qp->s_retry_cnt;
		qp->s_rnr_retry = qp->s_rnr_retry_cnt;
		goto bail;

	default:                /* 2: reserved */
reserved:
		/* Ignore reserved NAK codes. */
		goto bail;
	}

bail:
	return ret;
}

/*
 * We have seen an out of sequence RDMA read middle or last packet.
 * This ACKs SENDs and RDMA writes up to the first RDMA read or atomic SWQE.
 */
static void rdma_seq_err(struct qib_qp *qp, struct qib_ibport *ibp, u32 psn,
			 struct qib_ctxtdata *rcd)
{
	struct qib_swqe *wqe;

	/* Remove QP from retry timer */
	if (qp->s_flags & (QIB_S_TIMER | QIB_S_WAIT_RNR)) {
		qp->s_flags &= ~(QIB_S_TIMER | QIB_S_WAIT_RNR);
		del_timer(&qp->s_timer);
	}

	wqe = get_swqe_ptr(qp, qp->s_acked);

	while (qib_cmp24(psn, wqe->lpsn) > 0) {
		if (wqe->wr.opcode == IB_WR_RDMA_READ ||
		    wqe->wr.opcode == IB_WR_ATOMIC_CMP_AND_SWP ||
		    wqe->wr.opcode == IB_WR_ATOMIC_FETCH_AND_ADD)
			break;
		wqe = do_rc_completion(qp, wqe, ibp);
	}

	ibp->n_rdma_seq++;
	qp->r_flags |= QIB_R_RDMAR_SEQ;
	qib_restart_rc(qp, qp->s_last_psn + 1, 0);
	if (list_empty(&qp->rspwait)) {
		qp->r_flags |= QIB_R_RSP_SEND;
		atomic_inc(&qp->refcount);
		list_add_tail(&qp->rspwait, &rcd->qp_wait_list);
	}
}

/**
 * qib_rc_rcv_resp - process an incoming RC response packet
 * @ibp: the port this packet came in on
 * @ohdr: the other headers for this packet
 * @data: the packet data
 * @tlen: the packet length
 * @qp: the QP for this packet
 * @opcode: the opcode for this packet
 * @psn: the packet sequence number for this packet
 * @hdrsize: the header length
 * @pmtu: the path MTU
 *
 * This is called from qib_rc_rcv() to process an incoming RC response
 * packet for the given QP.
 * Called at interrupt level.
 */
static void qib_rc_rcv_resp(struct qib_ibport *ibp,
			    struct qib_other_headers *ohdr,
			    void *data, u32 tlen,
			    struct qib_qp *qp,
			    u32 opcode,
			    u32 psn, u32 hdrsize, u32 pmtu,
			    struct qib_ctxtdata *rcd)
{
	struct qib_swqe *wqe;
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	enum ib_wc_status status;
	unsigned long flags;
	int diff;
	u32 pad;
	u32 aeth;
	u64 val;

	if (opcode != OP(RDMA_READ_RESPONSE_MIDDLE)) {
		/*
		 * If ACK'd PSN on SDMA busy list try to make progress to
		 * reclaim SDMA credits.
		 */
		if ((qib_cmp24(psn, qp->s_sending_psn) >= 0) &&
		    (qib_cmp24(qp->s_sending_psn, qp->s_sending_hpsn) <= 0)) {

			/*
			 * If send tasklet not running attempt to progress
			 * SDMA queue.
			 */
			i