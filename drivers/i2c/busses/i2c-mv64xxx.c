_ibport *ibp = &dd->pport[n].ibport_data;

		if (!qib_mcast_tree_empty(ibp))
			qp_inuse++;
		rcu_read_lock();
		if (rcu_dereference(ibp->qp0))
			qp_inuse++;
		if (rcu_dereference(ibp->qp1))
			qp_inuse++;
		rcu_read_unlock();
	}

	spin_lock_irqsave(&dev->qpt_lock, flags);
	for (n = 0; n < dev->qp_table_size; n++) {
		qp = rcu_dereference_protected(dev->qp_table[n],
			lockdep_is_held(&dev->qpt_lock));
		RCU_INIT_POINTER(dev->qp_table[n], NULL);

		for (; qp; qp = rcu_dereference_protected(qp->next,
					lockdep_is_held(&dev->qpt_lock)))
			qp_inuse++;
	}
	spin_unlock_irqrestore(&dev->qpt_lock, flags);
	synchronize_rcu();

	return qp_inuse;
}

/**
 * qib_lookup_qpn - return the QP with the given QPN
 * @qpt: the QP table
 * @qpn: the QP number to look up
 *
 * The caller is responsible for decrementing the QP reference count
 * when done.
 */
struct qib_qp *qib_lookup_qpn(struct qib_ibport *ibp, u32 qpn)
{
	struct qib_qp *qp = NULL;

	rcu_read_lock();
	if (unlikely(qpn <= 1)) {
		if (qpn == 0)
			qp = rcu_dereference(ibp->qp0);
		else
			qp = rcu_dereference(ibp->qp1);
		if (qp)
			atomic_inc(&qp->refcount);
	} else {
		struct qib_ibdev *dev = &ppd_from_ibp(ibp)->dd->verbs_dev;
		unsigned n = qpn_hash(dev, qpn);

		for (qp = rcu_dereference(dev->qp_table[n]); qp;
			qp = rcu_dereference(qp->next))
			if (qp->ibqp.qp_num == qpn) {
				atomic_inc(&qp->refcount);
				break;
			}
	}
	rcu_read_unlock();
	return qp;
}

/**
 * qib_reset_qp - initialize the QP state to the reset state
 * @qp: the QP to reset
 * @type: the QP type
 */
static void qib_reset_qp(struct qib_qp *qp, enum ib_qp_type type)
{
	qp->remote_qpn = 0;
	qp->qkey = 0;
	qp->qp_access_flags = 0;
	atomic_set(&qp->s_dma_busy, 0);
	qp->s_flags &= QIB_S_SIGNAL_REQ_WR;
	qp->s_hdrwords = 0;
	qp->s_wqe = NULL;
	qp->s_draining = 0;
	qp->s_next_psn = 0;
	qp->s_last_psn = 0;
	qp->s_sending_psn = 0;
	qp->s_sending_hpsn = 0;
	qp->s_psn = 0;
	qp->r_psn = 0;
	qp->r_msn = 0;
	if (type == IB_QPT_RC) {
		qp->s_state = IB_OPCODE_RC_SEND_LAST;
		qp->r_state = IB_OPCODE_RC_SEND_LAST;
	} else {
		qp->s_state = IB_OPCODE_UC_SEND_LAST;
		qp->r_state = IB_OPCODE_UC_SEND_LAST;
	}
	qp->s_ack_state = IB_OPCODE_RC_ACKNOWLEDGE;
	qp->r_nak_state = 0;
	qp->r_aflags = 0;
	qp->r_flags = 0;
	qp->s_head = 0;
	qp->s_tail = 0;
	qp->s_cur = 0;
	qp->s_acked = 0;
	qp->s_last = 0;
	qp->s_ssn = 1;
	qp->s_lsn = 0;
	qp->s_mig_state = IB_MIG_MIGRATED;
	memset(qp->s_ack_queue, 0, sizeof(qp->s_ack_queue));
	qp->r_head_ack_queue = 0;
	qp->s_tail_ack_queue = 0;
	qp->s_num_rd_atomic = 0;
	if (qp->r_rq.wq) {
		qp->r_rq.wq->head = 0;
		qp->r_rq.wq->tail = 0;
	}
	qp->r_sge.num_sge = 0;
}

static void clear_mr_refs(struct qib_qp *qp, int clr_sends)
{
	unsigned n;

	if (test_and_clear_bit(QIB_R_REWIND_SGE, &qp->r_aflags))
		qib_put_ss(&qp->s_rdma_read_sge);

	qib_put_ss(&qp->r_sge);

	if (clr_sends) {
		while (qp->s_last != qp->s_head) {
			struct qib_swqe *wqe = get_swqe_ptr(qp, qp->s_last);
			unsigned i;

			for (i = 0; i < wqe->wr.num_sge; i++) {
				struct qib_sge *sge = &wqe->sg_list[i];

				qib_put_mr(sge->mr);
			}
			if (qp->ibqp.qp_type == IB_QPT_UD ||
			    qp->ibqp.qp_type == IB_QPT_SMI ||
			    qp->ibqp.qp_type == IB_QPT_GSI)
				atomic_dec(&to_iah(wqe->ud_wr.ah)->refcount);
			if (++qp->s_last >= qp->s_size)
				qp->s_last = 0;
		}
		if (qp->s_rdma_mr) {
			qib_put_mr(qp->s_rdma_mr);
			qp->s_rdma_mr = NULL;
		}
	}

	if (qp->ibqp.qp_type != IB_QPT_RC)
		return;

	for (n = 0; n < ARRAY_SIZE(qp->s_ack_queue); n++) {
		struct qib_ack_entry *e = &qp->s_ack_queue[n];

		if (e->opcode == IB_OPCODE_RC_RDMA_READ_REQUEST &&
		    e->rdma_sge.mr) {
			qib_put_mr(e->rdma_sge.mr);
			e->rdma_sge.mr = NULL;
		}
	}
}

/**
 * qib_error_qp - put a QP into the error state
 * @qp: the QP to put into the error state
 * @err: the receive completion error to signal if a RWQE is active
 *
 * Flushes both send and receive work queues.
 * Returns true if last WQE event should be generated.
 * The QP r_lock and s_lock should be held and interrupts disabled.
 * If we are already in error state, just return.
 */
int qib_error_qp(struct qib_qp *qp, enum ib_wc_status err)
{
	struct qib_ibdev *dev = to_idev(qp->ibqp.device);
	struct ib_wc wc;
	int ret = 0;

	if (qp->state == IB_QPS_ERR || qp->state == IB_QPS_RESET)
		goto bail;

	qp->state = IB_QPS_ERR;

	if (qp->s_flags & (QIB_S_TIMER | QIB_S_WAIT_RNR)) {
		qp->s_flags &= ~(QIB_S_TIMER | QIB_S_WAIT_RNR);
		del_timer(&qp->s_timer);
	}

	if (qp->s_flags & QIB_S_ANY_WAIT_SEND)
		qp->s_flags &= ~QIB_S_ANY_WAIT_SEND;

	spin_lock(&dev->pending_lock);
	if (!list_empty(&qp->iowait) && !(qp->s_flags & QIB_S_BUSY)) {
		qp->s_flags &= ~QIB_S_ANY_WAIT_IO;
		list_del_init(&qp->iowait);
	}
	spin_unlock(&dev->pending_lock);

	if (!(qp->s_flags & QIB_S_BUSY)) {
		qp->s_hdrwords = 0;
		if (qp->s_rdma_mr) {
			qib_put_mr(qp->s_rdma_mr);
			qp->s_rdma_mr = NULL;
		}
		if (qp->s_tx) {
			qib_put_txreq(qp->s_tx);
			qp->s_tx = NULL;
		}
	}

	/* Schedule the sending tasklet to drain the send work queue. */
	if (qp->s_last != qp->s_head)
		qib_schedule_send(qp);

	clear_mr_refs(qp, 0);

	memset(&wc, 0, sizeof(wc));
	wc.qp = &qp->ibqp;
	wc.opcode = IB_WC_RECV;

	if (test_and_clear_bit(QIB_R_WRID_VALID, &qp->r_aflags)) {
		wc.wr_id = qp->r_wr_id;
		wc.status = err;
		qib_cq_enter(to_icq(qp->ibqp.recv_cq), &wc, 1);
	}
	wc.status = IB_WC_WR_FLUSH_ERR;

	if (qp->r_rq.wq) {
		struct qib_rwq *wq;
		u32 head;
		u32 tail;

		spin_lock(&qp->r_rq.lock);

		/* sanity check pointers before trusting them */
		wq = qp->r_rq.wq;
		head = wq->head;
		if (head >= qp->r_rq.size)
			head = 0;
		tail = wq->tail;
		if (tail >= qp->r_rq.size)
			tail = 0;
		while (tail != head) {
			wc.wr_id = get_rwqe_ptr(&qp->r_rq, tail)->wr_id;
			if (++tail >= qp->r_rq.size)
				tail = 0;
			qib_cq_enter(to_icq(qp->ibqp.recv_cq), &wc, 1);
		}
		wq->tail = tail;

		spin_unlock(&qp->r_rq.lock);
	} else if (qp->ibqp.event_handler)
		ret = 1;

bail:
	return ret;
}

/**
 * qib_modify_qp - modify the attributes of a queue pair
 * @ibqp: the queue pair who's attributes we're modifying
 * @attr: the new attributes
 * @attr_mask: the mask of attributes to modify
 * @udata: user data for libibverbs.so
 *
 * Returns 0 on success, otherwise returns an errno.
 */
int qib_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		  int attr_mask, struct ib_udata *udata)
{
	struct qib_ibdev *dev = to_idev(ibqp->device);
	struct qib_qp *qp = to_iqp(ibqp);
	enum ib_qp_state cur_state, new_state;
	struct ib_event ev;
	int lastwqe = 0;
	int mig = 0;
	int ret;
	u32 pmtu = 0; /* for gcc warning only */

	spin_lock_irq(&qp->r_lock);
	spin_lock(&qp->s_lock);

	cur_state = attr_mask & IB_QP_CUR_STATE ?
		attr->cur_qp_state : qp->state;
	new_state = attr_mask & IB_QP_STATE ? attr->qp_state : cur_state;

	if (!ib_modify_qp_is_ok(cur_state, new_state, ibqp->qp_type,
				attr_mask, IB_LINK_LAYER_UNSPECIFIED))
		goto inval;

	if (attr_mask & IB_QP_AV) {
		if (attr->ah_attr.dlid >= QIB_MULTICAST_LID_BASE)
			goto inval;
		if (qib_check_ah(qp->ibqp.device, &attr->ah_attr))
			goto inval;
	}

	if (attr_mask & IB_QP_ALT_PATH) {
		if (attr->alt_ah_attr.dlid >= QIB_MULTICAST_LID_BASE)
			goto inval;
		if (qib_check_ah(qp->ibqp.device, &attr->alt_ah_attr))
			goto inval;
		if (attr->alt_pkey_index >= qib_get_npkeys(dd_from_dev(dev)))
			goto inval;
	}

	if (attr_mask & IB_QP_PKEY_INDEX)
		if (attr->pkey_index >= qib_get_npkeys(dd_from_dev(dev)))
			goto inval;

	if (attr_mask & IB_QP_MIN_RNR_TIMER)
		if (attr->min_rnr_timer > 31)
			goto inval;

	if (attr_mask & IB_QP_PORT)
		if (qp->ibqp.qp_type == IB_QPT_SMI ||
		    qp->ibqp.qp_type == IB_QPT_GSI ||
		    attr->port_num == 0 ||
		    attr->port_num > ibqp->device->phys_port_cnt)
			goto inval;

	if (attr_mask & IB_QP_DEST_QPN)
		if (attr->dest_qp_num > QIB_QPN_MASK)
			goto inval;

	if (attr_mask & IB_QP_RETRY_CNT)
		if (attr->retry_cnt > 7)
			goto inval;

	if (attr_mask & IB_QP_RNR_RETRY)
		if (attr->rnr_retry > 7)
			goto inval;

	/*
	 * Don't allow invalid path_mtu values.  OK to set greater
	 * than the active mtu (or even the max_cap, if we have tuned
	 * that to a small mtu.  We'll set qp->path_mtu
	 * to the lesser of requested attribute mtu and active,
	 * for packetizing messages.
	 * Note that the QP port has to be set in INIT and MTU in RTR.
	 */
	if (attr_mask & IB_QP_PATH_MTU) {
		struct qib_devdata *dd = dd_from_dev(dev);
		int mtu, pidx = qp->port_num - 1;

		mtu = ib_mtu_enum_to_int(attr->path_mtu);
		if (mtu == -1)
			goto inval;
		if (mtu > dd->pport[pidx].ibmtu) {
			switch (dd->pport[pidx].ibmtu) {
			case 4096:
				pmtu = IB_MTU_4096;
				break;
			case 2048:
				pmtu = IB_MTU_2048;
				break;
			case 1024:
				pmtu = IB_MTU_1024;
				break;
			case 512:
				pmtu = IB_MTU_512;
				break;
			case 256:
				pmtu = IB_MTU_256;
				break;
			default:
				pmtu = IB_MTU_2048;
			}
		} else
			pmtu = attr->path_mtu;
	}

	if (attr_mask & IB_QP_PATH_MIG_STATE) {
		if (attr->path_mig_state == IB_MIG_REARM) {
			if (qp->s_mig_state == IB_MIG_ARMED)
				goto inval;
			if (new_state != IB_QPS_RTS)
				goto inval;
		} else if (attr->path_mig_state == IB_MIG_MIGRATED) {
			if (qp->s_mig_state == IB_MIG_REARM)
				goto inval;
			if (new_state != IB_QPS_RTS && new_state != IB_QPS_SQD)
				goto inval;
			if (qp->s_mig_state == IB_MIG_ARMED)
				mig = 1;
		} else
			goto inval;
	}

	if (attr_mask & IB_QP_MAX_DEST_RD_ATOMIC)
		if (attr->max_dest_rd_atomic > QIB_MAX_RDMA_ATOMIC)
			goto inval;

	switch (new_state) {
	case IB_QPS_RESET:
		if (qp->state != IB_QPS_RESET) {
			qp->state = IB_QPS_RESET;
			spin_lock(&dev->pending_lock);
			if (!list_empty(&qp->iowait))
				list_del_init(&qp->iowait);
			spin_unlock(&dev->pending_lock);
			qp->s_flags &= ~(QIB_S_TIMER | QIB_S_ANY_WAIT);
			spin_unlock(&qp->s_lock);
			spin_unlock_irq(&qp->r_lock);
			/* Stop the sending work queue and retry timer */
			cancel_work_sync(&qp->s_work);
			del_timer_sync(&qp->s_timer);
			wait_event(qp->wait_dma, !atomic_read(&qp->s_dma_busy));
			if (qp->s_tx) {
				qib_put_txreq(qp->s_tx);
				qp->s_tx = NULL;
			}
			remove_qp(dev, qp);
			wait_event(qp->wait, !atomic_read(&qp->refcount));
			spin_lock_irq(&qp->r_lock);
			spin_lock(&qp->s_lock);
			clear_mr_refs(qp, 1);
			qib_reset_qp(qp, ibqp->qp_type);
		}
		break;

	case IB_QPS_RTR:
		/* Allow event to retrigger if QP set to RTR more than once */
		qp->r_flags &= ~QIB_R_COMM_EST;
		qp->state = new_state;
		break;

	case IB_QPS_SQD:
		qp->s_draining = qp->s_last != qp->s_cur;
		qp->state = new_state;
		break;

	case IB_QPS_SQE:
		if (qp->ibqp.qp_type == IB_QPT_RC)
			goto inval;
		qp->state = new_state;
		break;

	case IB_QPS_ERR:
		lastwqe = qib_error_qp(qp, IB_WC_WR_FLUSH_ERR);
		break;

	default:
		qp->state = new_state;
		break;
	}

	if (attr_mask & IB_QP_PKEY_INDEX)
		qp->s_pkey_index = attr->pkey_index;

	if (attr_mask & IB_QP_PORT)
		qp->port_num = attr->port_num;

	if (attr_mask & IB_QP_DEST_QPN)
		qp->remote_qpn = attr->dest_qp_num;

	if (attr_mask & IB_QP_SQ_PSN) {
		qp->s_next_psn = attr->sq_psn & QIB_PSN_MASK;
		qp->s_psn = qp->s_next_psn;
		qp->s_sending_psn = qp->s_next_psn;
		qp->s_last_psn = qp->s_next_psn - 1;
		qp->s_sending_hpsn = qp->s_last_psn;
	}

	if (attr_mask & IB_QP_RQ_PSN)
		qp->r_psn = attr->rq_psn & QIB_PSN_MASK;

	if (attr_mask & IB_QP_ACCESS_FLAGS)
		qp->qp_access_flags = attr->qp_access_flags;

	if (attr_mask & IB_QP_AV) {
		qp->remote_ah_attr = attr->ah_attr;
		qp->s_srate = attr->ah_attr.static_rate;
	}

	if (attr_mask & IB_QP_ALT_PATH) {
		qp->alt_ah_attr = attr->alt_ah_attr;
		qp->s_alt_pkey_index = attr->alt_pkey_index;
	}

	if (attr_mask & IB_QP_PATH_MIG_STATE) {
		qp->s_mig_state = attr->path_mig_state;
		if (mig) {
			qp->remote_ah_attr = qp->alt_ah_attr;
			qp->port_num = qp->alt_ah_attr.port_num;
			qp->s_pkey_index = qp->s_alt_pkey_index;
		}
	}

	if (attr_mask & IB_QP_PATH_MTU) {
		qp->path_mtu = pmtu;
		qp->pmtu = ib_mtu_enum_to_int(pmtu);
	}

	if (attr_mask & IB_QP_RETRY_CNT) {
		qp->s_retry_cnt = attr->retry_cnt;
		qp->s_retry = attr->retry_cnt;
	}

	if (attr_mask & IB_QP_RNR_RETRY) {
		qp->s_rnr_retry_cnt = attr->rnr_retry;
		qp->s_rnr_retry = attr->rnr_retry;
	}

	if (attr_mask & IB_QP_MIN_RNR_TIMER)
		qp->r_min_rnr_timer = attr->min_rnr_timer;

	if (attr_mask & IB_QP_TIMEOUT) {
		qp->timeout = attr->timeout;
		qp->timeout_jiffies =
			usecs_to_jiffies((4096UL * (1UL << qp->timeout)) /
				1000UL);
	}

	if (attr_mask & IB_QP_QKEY)
		qp->qkey = attr->qkey;

	if (attr_mask & IB_QP_MAX_DEST_RD_ATOMIC)
		qp->r_max_rd_atomic = attr->max_dest_rd_atomic;

	if (attr_mask & IB_QP_MAX_QP_RD_ATOMIC)
		qp->s_max_rd_atomic = attr->max_rd_atomic;

	spin_unlock(&qp->s_lock);
	spin_unlock_irq(&qp->r_lock);

	if (cur_state == IB_QPS_RESET && new_state == IB_QPS_INIT)
		insert_qp(dev, qp);

	if (lastwqe) {
		ev.device = qp->ibqp.device;
		ev.element.qp = &qp->ibqp;
		ev.event = IB_EVENT_QP_LAST_WQE_REACHED;
		qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
	}
	if (mig) {
		ev.device = qp->ibqp.device;
		ev.element.qp = &qp->ibqp;
		ev.event = IB_EVENT_PATH_MIG;
		qp->ibqp.event_handler(&ev, qp->ibqp.qp_context);
	}
	ret = 0;
	goto bail;

inval:
	spin_unlock(&qp->s_lock);
	spin_unlock_irq(&qp->r_lock);
	ret = -EINVAL;

bail:
	return ret;
}

int qib_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		 int attr_mask, struct ib_qp_init_attr *init_attr)
{
	struct qib_qp *qp = to_iqp(ibqp);

	attr->qp_state = qp->state;
	attr->cur_qp_state = attr->qp_state;
	attr->path_mtu = qp->path_mtu;
	attr->path_mig_state = qp->s_mig_state;
	attr->qkey = qp->qkey;
	attr->rq_psn = qp->r_psn & QIB_PSN_MASK;
	attr->sq_psn = qp->s_next_psn & QIB_PSN_MASK;
	attr->dest_qp_num = qp->remote_qpn;
	attr->qp_access_flags = qp->qp_access_flags;
	attr->cap.max_send_wr = qp->s_size - 1;
	attr->cap.max_recv_wr = qp->ibqp.srq ? 0 : qp->r_rq.size - 1;
	attr->cap.max_send_sge = qp->s_max_sge;
	attr->cap.max_recv_sge = qp->r_rq.max_sge;
	attr->cap.max_inline_data = 0;
	attr->ah_attr = qp->remote_ah_attr;
	attr->alt_ah_attr = qp->alt_ah_attr;
	attr->pkey_index = qp->s_pkey_index;
	attr->alt_pkey_index = qp->s_alt_pkey_index;
	attr->en_sqd_async_notify = 0;
	attr->sq_draining = qp->s_draining;
	attr->max_rd_atomic = qp->s_max_rd_atomic;
	attr->max_dest_rd_atomic = qp->r_max_rd_atomic;
	attr->min_rnr_timer = qp->r_min_rnr_timer;
	attr->port_num = qp->port_num;
	attr->timeout = qp->timeout;
	attr->retry_cnt = qp->s_retry_cnt;
	attr->rnr_retry = qp->s_rnr_retry_cnt;
	attr->alt_port_num = qp->alt_ah_attr.port_num;
	attr->alt_timeout = qp->alt_timeout;

	init_attr->event_handler = qp->ibqp.event_handler;
	init_attr->qp_context = qp->ibqp.qp_context;
	init_attr->send_cq = qp->ibqp.send_cq;
	init_attr->recv_cq = qp->ibqp.recv_cq;
	init_attr->srq = qp->ibqp.srq;
	init_attr->cap = attr->cap;
	if (qp->s_flags & QIB_S_SIGNAL_REQ_WR)
		init_attr->sq_sig_type = IB_SIGNAL_REQ_WR;
	else
		init_attr->sq_sig_type = IB_SIGNAL_ALL_WR;
	init_attr->qp_type = qp->ibqp.qp_type;
	init_attr->port_num = qp->port_num;
	return 0;
}

/**
 * qib_compute_aeth - compute the AETH (syndrome + MSN)
 * @qp: the queue pair to compute the AETH for
 *
 * Returns the AETH.
 */
__be32 qib_compute_aeth(struct qib_qp *qp)
{
	u32 aeth = qp->r_msn & QIB_MSN_MASK;

	if (qp->ibqp.srq) {
		/*
		 * Shared receive queues don't generate credits.
		 * Set the credit field to the invalid value.
		 */
		aeth |= QIB_AETH_CREDIT_INVAL << QIB_AETH_CREDIT_SHIFT;
	} else {
		u32 min, max, x;
		u32 credits;
		struct qib_rwq *wq = qp->r_rq.wq;
		u32 head;
		u32 tail;

		/* sanity check pointers before trusting them */
		head = wq->head;
		if (head >= qp->r_rq.size)
			head = 0;
		tail = wq->tail;
		if (tail >= qp->r_rq.size)
			tail = 0;
		/*
		 * Compute the number of credits available (RWQEs).
		 * XXX Not holding the r_rq.lock here so there is a small
		 * chance that the pair of reads are not atomic.
		 */
		credits = head - tail;
		if ((int)credits < 0)
			credits += qp->r_rq.size;
		/*
		 * Binary search the credit table to find the code to
		 * use.
		 */
		min = 0;
		max = 31;
		for (;;) {
			x = (min + max) / 2;
			if (credit_table[x] == credits)
				break;
			if (credit_table[x] > credits)
				max = x;
			else if (min == x)
				break;
			else
				min = x;
		}
		aeth |= x << QIB_AETH_CREDIT_SHIFT;
	}
	return cpu_to_be32(aeth);
}

/**
 * qib_create_qp - create a queue pair for a device
 * @ibpd: the protection domain who's device we create the queue pair for
 * @init_attr: the attributes of the queue pair
 * @udata: user data for libibverbs.so
 *
 * Returns the queue pair on success, otherwise returns an errno.
 *
 * Called by the ib_create_qp() core verbs function.
 */
struct ib_qp *qib_create_qp(struct ib_pd *ibpd,
			    struct ib_qp_init_attr *init_attr,
			    struct ib_udata *udata)
{
	struct qib_qp *qp;
	int err;
	struct qib_swqe *swq = NULL;
	struct qib_ibdev *dev;
	struct qib_devdata *dd;
	size_t sz;
	size_t sg_list_sz;
	struct ib_qp *ret;
	gfp_t gfp;


	if (init_attr->cap.max_send_sge > ib_qib_max_sges ||
	    init_attr->cap.max_send_wr > ib_qib_max_qp_wrs ||
	    init_attr->create_flags & ~(IB_QP_CREATE_USE_GFP_NOIO))
		return ERR_PTR(-EINVAL);

	/* GFP_NOIO is applicable in RC QPs only */
	if (init_attr->create_flags & IB_QP_CREATE_USE_GFP_NOIO &&
	    init_attr->qp_type != IB_QPT_RC)
		return ERR_PTR(-EINVAL);

	gfp = init_attr->create_flags & IB_QP_CREATE_USE_GFP_NOIO ?
			GFP_NOIO : GFP_KERNEL;

	/* Check receive queue parameters if no SRQ is specified. */
	if (!init_attr->srq) {
		if (init_attr->cap.max_recv_sge > ib_qib_max_sges ||
		    init_attr->cap.max_recv_wr > ib_qib_max_qp_wrs) {
			ret = ERR_PTR(-EINVAL);
			goto bail;
		}
		if (init_attr->cap.max_send_sge +
		    init_attr->cap.max_send_wr +
		    init_attr->cap.max_recv_sge +
		    init_attr->cap.max_recv_wr == 0) {
			ret = ERR_PTR(-EINVAL);
			goto bail;
		}
	}

	switch (init_attr->qp_type) {
	case IB_QPT_SMI:
	case IB_QPT_GSI:
		if (init_attr->port_num == 0 ||
		    init_attr->port_num > ibpd->device->phys_port_cnt) {
			ret = ERR_PTR(-EINVAL);
			goto bail;
		}
	case IB_QPT_UC:
	case IB_QPT_RC:
	case IB_QPT_UD:
		sz = sizeof(struct qib_sge) *
			init_attr->cap.max_send_sge +
			sizeof(struct qib_swqe);
		swq = __vmalloc((init_attr->cap.max_send_wr + 1) * sz,
				gfp, PAGE_KERNEL);
		if (swq == NULL) {
			ret = ERR_PTR(-ENOMEM);
			goto bail;
		}
		sz = sizeof(*qp);
		sg_list_sz = 0;
		if (init_attr->srq) {
			struct qib_srq *srq = to_isrq(init_attr->srq);

			if (srq->rq.max_sge > 1)
				sg_list_sz = sizeof(*qp->r_sg_list) *
					(srq->rq.max_sge - 1);
		} else if (init_attr->cap.max_recv_sge > 1)
			sg_list_sz = sizeof(*qp->r_sg_list) *
				(init_attr->cap.max_recv_sge - 1);
		qp = kzalloc(sz + sg_list_sz, gfp);
		if (!qp) {
			ret = ERR_PTR(-ENOMEM);
			goto bail_swq;
		}
		RCU_INIT_POINTER(qp->next, NULL);
		qp->s_hdr = kzalloc(sizeof(*qp->s_hdr), gfp);
		if (!qp->s_hdr) {
			ret = ERR_PTR(-ENOMEM);
			goto bail_qp;
		}
		qp->timeout_jiffies =
			usecs_to_jiffies((4096UL * (1UL << qp->timeout)) /
				1000UL);
		if (init_attr->srq)
			sz = 0;
		else {
			qp->r_rq.size = init_attr->cap.max_recv_wr + 1;
			qp->r_rq.max_sge = init_attr->cap.max_recv_sge;
			sz = (sizeof(struct ib_sge) * qp->r_rq.max_sge) +
				sizeof(struct qib_rwqe);
			if (gfp != GFP_NOIO)
				qp->r_rq.wq = vmalloc_user(
						sizeof(struct qib_rwq) +
						qp->r_rq.size * sz);
			else
				qp->r_rq.wq = __vmalloc(
						sizeof(struct qib_rwq) +
						qp->r_rq.size * sz,
						gfp, PAGE_KERNEL);

			if (!qp->r_rq.wq) {
				ret = ERR_PTR(-ENOMEM);
				goto bail_qp;
			}
		}

		/*
		 * ib_create_qp() will initialize qp->ibqp
		 * except for qp->ibqp.qp_num.
		 */
		spin_lock_init(&qp->r_lock);
		spin_lock_init(&qp->s_lock);
		spin_lock_init(&qp->r_rq.lock);
		atomic_set(&qp->refcount, 0);
		init_waitqueue_head(&qp->wait);
		init_waitqueue_head(&qp->wait_dma);
		init_timer(&qp->s_timer);
		qp->s_timer.data = (unsigned long)qp;
		INIT_WORK(&qp->s_work, qib_do_send);
		INIT_LIST_HEAD(&qp->iowait);
		INIT_LIST_HEAD(&qp->rspwait);
		qp->state = IB_QPS_RESET;
		qp->s_wq = swq;
		qp->s_size = init_attr->cap.max_send_wr + 1;
		qp->s_max_sge = init_attr->cap.max_send_sge;
		if (init_attr->sq_sig_type == IB_SIGNAL_REQ_WR)
			qp->s_flags = QIB_S_SIGNAL_REQ_WR;
		dev = to_idev(ibpd->device);
		dd = dd_from_dev(dev);
		err = alloc_qpn(dd, &dev->qpn_table, init_attr->qp_type,
				init_attr->port_num, gfp);
		if (err < 0) {
			ret = ERR_PTR(err);
			vfree(qp->r_rq.wq);
			goto bail_qp;
		}
		qp->ibqp.qp_num = err;
		qp->port_num = init_attr->port_num;
		qib_reset_qp(qp, init_attr->qp_type);
		break;

	default:
		/* Don't support raw QPs */
		ret = ERR_PTR(-ENOSYS);
		goto bail;
	}

	init_attr->cap.max_inline_data = 0;

	/*
	 * Return the address of the RWQ as the offset to mmap.
	 * See qib_mmap() for details.
	 */
	if (udata && udata->outlen >= sizeof(__u64)) {
		if (!qp->r_rq.wq) {
			__u64 offset = 0;

			err = ib_copy_to_udata(udata, &offset,
					       sizeof(offset));
			if (err) {
				ret = ERR_PTR(err);
				goto bail_ip;
			}
		} else {
			u32 s = sizeof(struct qib_rwq) + qp->r_rq.size * sz;

			qp->ip = qib_create_mmap_info(dev, s,
						      ibpd->uobject->context,
						      qp->r_rq.wq);
			if (!qp->ip) {
				ret = ERR_PTR(-ENOMEM);
				goto bail_ip;
			}

			err = ib_copy_to_udata(udata, &(qp->ip->offset),
					       sizeof(qp->ip->offset));
			if (err) {
				ret = ERR_PTR(err);
				goto bail_ip;
			}
		}
	}

	spin_lock(&dev->n_qps_lock);
	if (dev->n_qps_allocated == ib_qib_max_qps) {
		spin_unlock(&dev->n_qps_lock);
		ret = ERR_PTR(-ENOMEM);
		goto bail_ip;
	}

	dev->n_qps_allocated++;
	spin_unlock(&dev->n_qps_lock);

	if (qp->ip) {
		spin_lock_irq(&dev->pending_lock);
		list_add(&qp->ip->pending_mmaps, &dev->pending_mmaps);
		spin_unlock_irq(&dev->pending_lock);
	}

	ret = &qp->ibqp;
	goto bail;

bail_ip:
	if (qp->ip)
		kref_put(&qp->ip->ref, qib_release_mmap_info);
	else
		vfree(qp->r_rq.wq);
	free_qpn(&dev->qpn_table, qp->ibqp.qp_num);
bail_qp:
	kfree(qp->s_hdr);
	kfree(qp);
bail_swq:
	vfree(swq);
bail:
	return ret;
}

/**
 * qib_destroy_qp - destroy a queue pair
 * @ibqp: the queue pair to destroy
 *
 * Returns 0 on success.
 *
 * Note that this can be called while the QP is actively sending or
 * receiving!
 */
int qib_destroy_qp(struct ib_qp *ibqp)
{
	struct qib_qp *qp = to_iqp(ibqp);
	struct qib_ibdev *dev = to_idev(ibqp->device);

	/* Make sure HW and driver activity is stopped. */
	spin_lock_irq(&qp->s_lock);
	if (qp->state != IB_QPS_RESET) {
		qp->state = IB_QPS_RESET;
		spin_lock(&dev->pending_lock);
		if (!list_empty(&qp->iowait))
			list_del_init(&qp->iowait);
		spin_unlock(&dev->pending_lock);
		qp->s_flags &= ~(QIB_S_TIMER | QIB_S_ANY_WAIT);
		spin_unlock_irq(&qp->s_lock);
		cancel_work_sync(&qp->s_work);
		del_timer_sync(&qp->s_timer);
		wait_event(qp->wait_dma, !atomic_read(&qp->s_dma_busy));
		if (qp->s_tx) {
			qib_put_txreq(qp->s_tx);
			qp->s_tx = NULL;
		}
		remove_qp(dev, qp);
		wait_event(qp->wait, !atomic_read(&qp->refcount));
		clear_mr_refs(qp, 1);
	} else
		spin_unlock_irq(&qp->s_lock);

	/* all user's cleaned up, mark it available */
	free_qpn(&dev->qpn_table, qp->ibqp.qp_num);
	spin_lock(&dev->n_qps_lock);
	dev->n_qps_allocated--;
	spin_unlock(&dev->n_qps_lock);

	if (qp->ip)
		kref_put(&qp->ip->ref, qib_release_mmap_info);
	else
		vfree(qp->r_rq.wq);
	vfree(qp->s_wq);
	kfree(qp->s_hdr);
	kfree(qp);
	return 0;
}

/**
 * qib_init_qpn_table - initialize the QP number table for a device
 * @qpt: the QPN table
 */
void qib_init_qpn_table(struct qib_devdata *dd, struct qib_qpn_table *qpt)
{
	spin_lock_init(&qpt->lock);
	qpt->last = 1;          /* start with QPN 2 */
	qpt->nmaps = 1;
	qpt->mask = dd->qpn_mask;
}

/**
 * qib_free_qpn_table - free the QP number table for a device
 * @qpt: the QPN table
 */
void qib_free_qpn_table(struct qib_qpn_table *qpt)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(qpt->map); i++)
		if (qpt->map[i].page)
			free_page((unsigned long) qpt->map[i].page);
}

/**
 * qib_get_credit - flush the send work queue of a QP
 * @qp: the qp who's send work queue to flush
 * @aeth: the Acknowledge Extended Transport Header
 *
 * The QP s_lock should be held.
 */
void qib_get_credit(struct qib_qp *qp, u32 aeth)
{
	u32 credit = (aeth >> QIB_AETH_CREDIT_SHIFT) & QIB_AETH_CREDIT_MASK;

	/*
	 * If the credit is invalid, we can send
	 * as many packets as we like.  Otherwise, we have to
	 * honor the credit field.
	 */
	if (credit == QIB_AETH_CREDIT_INVAL) {
		if (!(qp->s_flags & QIB_S_UNLIMITED_CREDIT)) {
			qp->s_flags |= QIB_S_UNLIMITED_CREDIT;
			if (qp->s_flags & QIB_S_WAIT_SSN_CREDIT) {
				qp->s_flags &= ~QIB_S_WAIT_SSN_CREDIT;
				qib_schedule_send(qp);
			}
		}
	} else if (!(qp->s_flags & QIB_S_UNLIMITED_CREDIT)) {
		/* Compute new LSN (i.e., MSN + credit) */
		credit = (aeth + credit_table[credit]) & QIB_MSN_MASK;
		if (qib_cmp24(credit, qp->s_lsn) > 0) {
			qp->s_lsn = credit;
			if (qp->s_flags & QIB_S_WAIT_SSN_CREDIT) {
				qp->s_flags &= ~QIB_S_WAIT_SSN_CREDIT;
				qib_schedule_send(qp);
			}
		}
	}
}

#ifdef CONFIG_DEBUG_FS

struct qib_qp_iter {
	struct qib_ibdev *dev;
	struct qib_qp *qp;
	int n;
};

struct qib_qp_iter *qib_qp_iter_init(struct qib_ibdev *dev)
{
	struct qib_qp_iter *iter;

	iter = kzalloc(sizeof(*iter), GFP_KERNEL);
	if (!iter)
		return NULL;

	iter->dev = dev;
	if (qib_qp_iter_next(iter)) {
		kfree(iter);
		return NULL;
	}

	return iter;
}

int qib_qp_iter_next(struct qib_qp_iter *iter)
{
	struct qib_ibdev *dev = iter->dev;
	int n = iter->n;
	int ret = 1;
	struct qib_qp *pqp = iter->qp;
	struct qib_qp *qp;

	for (; n < dev->qp_table_size; n++) {
		if (pqp)
			qp = rcu_dereference(pqp->next);
		else
			qp = rcu_dereference(dev->qp_table[n]);
		pqp = qp;
		if (qp) {
			iter->qp = qp;
			iter->n = n;
			return 0;
		}
	}
	return ret;
}

static const char * const qp_type_str[] = {
	"SMI", "GSI", "RC", "UC", "UD",
};

void qib_qp_iter_print(struct seq_file *s, struct qib_qp_iter *iter)
{
	struct qib_swqe *wqe;
	struct qib_qp *qp = iter->qp;

	wqe = get_swqe_ptr(qp, qp->s_last);
	seq_printf(s,
		   "N %d QP%u %s %u %u %u f=%x %u %u %u %u %u PSN %x %x %x %x %x (%u %u %u %u %u %u) QP%u LID %x\n",
		   iter->n,
		   qp->ibqp.qp_num,
		   qp_type_str[qp->ibqp.qp_type],
		   qp->state,
		   wqe->wr.opcode,
		   qp->s_hdrwords,
		   qp->s_flags,
		   atomic_read(&qp->s_dma_busy),
		   !list_empty(&qp->iowait),
		   qp->timeout,
		   wqe->ssn,
		   qp->s_lsn,
		   qp->s_last_psn,
		   qp->s_psn, qp->s_next_psn,
		   qp->s_sending_psn, qp->s_sending_hpsn,
		   qp->s_last, qp->s_acked, qp->s_cur,
		   qp->s_tail, qp->s_head, qp->s_size,
		   qp->remote_qpn,
		   qp->remote_ah_attr.dlid);
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 