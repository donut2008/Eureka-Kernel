_put_tx_iu(ch, iu, SRP_IU_CMD);

	/*
	 * Avoid that the loops that iterate over the request ring can
	 * encounter a dangling SCSI command pointer.
	 */
	req->scmnd = NULL;

err:
	if (scmnd->result) {
		scmnd->scsi_done(scmnd);
		ret = 0;
	} else {
		ret = SCSI_MLQUEUE_HOST_BUSY;
	}

	goto unlock_rport;
}

/*
 * Note: the resources allocated in this function are freed in
 * srp_free_ch_ib().
 */
static int srp_alloc_iu_bufs(struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;
	int i;

	ch->rx_ring = kcalloc(target->queue_size, sizeof(*ch->rx_ring),
			      GFP_KERNEL);
	if (!ch->rx_ring)
		goto err_no_ring;
	ch->tx_ring = kcalloc(target->queue_size, sizeof(*ch->tx_ring),
			      GFP_KERNEL);
	if (!ch->tx_ring)
		goto err_no_ring;

	for (i = 0; i < target->queue_size; ++i) {
		ch->rx_ring[i] = srp_alloc_iu(target->srp_host,
					      ch->max_ti_iu_len,
					      GFP_KERNEL, DMA_FROM_DEVICE);
		if (!ch->rx_ring[i])
			goto err;
	}

	for (i = 0; i < target->queue_size; ++i) {
		ch->tx_ring[i] = srp_alloc_iu(target->srp_host,
					      target->max_iu_len,
					      GFP_KERNEL, DMA_TO_DEVICE);
		if (!ch->tx_ring[i])
			goto err;

		list_add(&ch->tx_ring[i]->list, &ch->free_tx);
	}

	return 0;

err:
	for (i = 0; i < target->queue_size; ++i) {
		srp_free_iu(target->srp_host, ch->rx_ring[i]);
		srp_free_iu(target->srp_host, ch->tx_ring[i]);
	}


err_no_ring:
	kfree(ch->tx_ring);
	ch->tx_ring = NULL;
	kfree(ch->rx_ring);
	ch->rx_ring = NULL;

	return -ENOMEM;
}

static uint32_t srp_compute_rq_tmo(struct ib_qp_attr *qp_attr, int attr_mask)
{
	uint64_t T_tr_ns, max_compl_time_ms;
	uint32_t rq_tmo_jiffies;

	/*
	 * According to section 11.2.4.2 in the IBTA spec (Modify Queue Pair,
	 * table 91), both the QP timeout and the retry count have to be set
	 * for RC QP's during the RTR to RTS transition.
	 */
	WARN_ON_ONCE((attr_mask & (IB_QP_TIMEOUT | IB_QP_RETRY_CNT)) !=
		     (IB_QP_TIMEOUT | IB_QP_RETRY_CNT));

	/*
	 * Set target->rq_tmo_jiffies to one second more than the largest time
	 * it can take before an error completion is generated. See also
	 * C9-140..142 in the IBTA spec for more information about how to
	 * convert the QP Local ACK Timeout value to nanoseconds.
	 */
	T_tr_ns = 4096 * (1ULL << qp_attr->timeout);
	max_compl_time_ms = qp_attr->retry_cnt * 4 * T_tr_ns;
	do_div(max_compl_time_ms, NSEC_PER_MSEC);
	rq_tmo_jiffies = msecs_to_jiffies(max_compl_time_ms + 1000);

	return rq_tmo_jiffies;
}

static void srp_cm_rep_handler(struct ib_cm_id *cm_id,
			       const struct srp_login_rsp *lrsp,
			       struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;
	struct ib_qp_attr *qp_attr = NULL;
	int attr_mask = 0;
	int ret;
	int i;

	if (lrsp->opcode == SRP_LOGIN_RSP) {
		ch->max_ti_iu_len = be32_to_cpu(lrsp->max_ti_iu_len);
		ch->req_lim       = be32_to_cpu(lrsp->req_lim_delta);

		/*
		 * Reserve credits for task management so we don't
		 * bounce requests back to the SCSI mid-layer.
		 */
		target->scsi_host->can_queue
			= min(ch->req_lim - SRP_TSK_MGMT_SQ_SIZE,
			      target->scsi_host->can_queue);
		target->scsi_host->cmd_per_lun
			= min_t(int, target->scsi_host->can_queue,
				target->scsi_host->cmd_per_lun);
	} else {
		shost_printk(KERN_WARNING, target->scsi_host,
			     PFX "Unhandled RSP opcode %#x\n", lrsp->opcode);
		ret = -ECONNRESET;
		goto error;
	}

	if (!ch->rx_ring) {
		ret = srp_alloc_iu_bufs(ch);
		if (ret)
			goto error;
	}

	ret = -ENOMEM;
	qp_attr = kmalloc(sizeof *qp_attr, GFP_KERNEL);
	if (!qp_attr)
		goto error;

	qp_attr->qp_state = IB_QPS_RTR;
	ret = ib_cm_init_qp_attr(cm_id, qp_attr, &attr_mask);
	if (ret)
		goto error_free;

	ret = ib_modify_qp(ch->qp, qp_attr, attr_mask);
	if (ret)
		goto error_free;

	for (i = 0; i < target->queue_size; i++) {
		struct srp_iu *iu = ch->rx_ring[i];

		ret = srp_post_recv(ch, iu);
		if (ret)
			goto error_free;
	}

	qp_attr->qp_state = IB_QPS_RTS;
	ret = ib_cm_init_qp_attr(cm_id, qp_attr, &attr_mask);
	if (ret)
		goto error_free;

	target->rq_tmo_jiffies = srp_compute_rq_tmo(qp_attr, attr_mask);

	ret = ib_modify_qp(ch->qp, qp_attr, attr_mask);
	if (ret)
		goto error_free;

	ret = ib_send_cm_rtu(cm_id, NULL, 0);

error_free:
	kfree(qp_attr);

error:
	ch->status = ret;
}

static void srp_cm_rej_handler(struct ib_cm_id *cm_id,
			       struct ib_cm_event *event,
			       struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;
	struct Scsi_Host *shost = target->scsi_host;
	struct ib_class_port_info *cpi;
	int opcode;

	switch (event->param.rej_rcvd.reason) {
	case IB_CM_REJ_PORT_CM_REDIRECT:
		cpi = event->param.rej_rcvd.ari;
		ch->path.dlid = cpi->redirect_lid;
		ch->path.pkey = cpi->redirect_pkey;
		cm_id->remote_cm_qpn = be32_to_cpu(cpi->redirect_qp) & 0x00ffffff;
		memcpy(ch->path.dgid.raw, cpi->redirect_gid, 16);

		ch->status = ch->path.dlid ?
			SRP_DLID_REDIRECT : SRP_PORT_REDIRECT;
		break;

	case IB_CM_REJ_PORT_REDIRECT:
		if (srp_target_is_topspin(target)) {
			/*
			 * Topspin/Cisco SRP gateways incorrectly send
			 * reject reason code 25 when they mean 24
			 * (port redirect).
			 */
			memcpy(ch->path.dgid.raw,
			       event->param.rej_rcvd.ari, 16);

			shost_printk(KERN_DEBUG, shost,
				     PFX "Topspin/Cisco redirect to target port GID %016llx%016llx\n",
				     be64_to_cpu(ch->path.dgid.global.subnet_prefix),
				     be64_to_cpu(ch->path.dgid.global.interface_id));

			ch->status = SRP_PORT_REDIRECT;
		} else {
			shost_printk(KERN_WARNING, shost,
				     "  REJ reason: IB_CM_REJ_PORT_REDIRECT\n");
			ch->status = -ECONNRESET;
		}
		break;

	case IB_CM_REJ_DUPLICATE_LOCAL_COMM_ID:
		shost_printk(KERN_WARNING, shost,
			    "  REJ reason: IB_CM_REJ_DUPLICATE_LOCAL_COMM_ID\n");
		ch->status = -ECONNRESET;
		break;

	case IB_CM_REJ_CONSUMER_DEFINED:
		opcode = *(u8 *) event->private_data;
		if (opcode == SR