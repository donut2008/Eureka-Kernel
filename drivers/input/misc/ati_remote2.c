entifier.\n");
		goto reject;
	}

	ch = kzalloc(sizeof *ch, GFP_KERNEL);
	if (!ch) {
		rej->reason = cpu_to_be32(
			      SRP_LOGIN_REJ_INSUFFICIENT_RESOURCES);
		pr_err("rejected SRP_LOGIN_REQ because no memory.\n");
		ret = -ENOMEM;
		goto reject;
	}

	INIT_WORK(&ch->release_work, srpt_release_channel_work);
	memcpy(ch->i_port_id, req->initiator_port_id, 16);
	memcpy(ch->t_port_id, req->target_port_id, 16);
	ch->sport = &sdev->port[param->port - 1];
	ch->cm_id = cm_id;
	/*
	 * Avoid QUEUE_FULL conditions by limiting the number of buffers used
	 * for the SRP protocol to the command queue size.
	 */
	ch->rq_size = SRPT_RQ_SIZE;
	spin_lock_init(&ch->spinlock);
	ch->state = CH_CONNECTING;
	INIT_LIST_HEAD(&ch->cmd_wait_list);
	ch->rsp_size = ch->sport->port_attrib.srp_max_rsp_size;

	ch->ioctx_ring = (struct srpt_send_ioctx **)
		srpt_alloc_ioctx_ring(ch->sport->sdev, ch->rq_size,
				      sizeof(*ch->ioctx_ring[0]),
				      ch->rsp_size, DMA_TO_DEVICE);
	if (!ch->ioctx_ring)
		goto free_ch;

	INIT_LIST_HEAD(&ch->free_list);
	for (i = 0; i < ch->rq_size; i++) {
		ch->ioctx_ring[i]->ch = ch;
		list_add_tail(&ch->ioctx_ring[i]->free_list, &ch->free_list);
	}

	ret = srpt_create_ch_ib(ch);
	if (ret) {
		rej->reason = cpu_to_be32(
			      SRP_LOGIN_REJ_INSUFFICIENT_RESOURCES);
		pr_err("rejected SRP_LOGIN_REQ because creating"
		       " a new RDMA channel failed.\n");
		goto free_ring;
	}

	ret = srpt_ch_qp_rtr(ch, ch->qp);
	if (ret) {
		rej->reason = cpu_to_be32(SRP_LOGIN_REJ_INSUFFICIENT_RESOURCES);
		pr_err("rejected SRP_LOGIN_REQ because enabling"
		       " RTR failed (error code = %d)\n", ret);
		goto destroy_ib;
	}
	/*
	 * Use the initator port identifier as the session name.
	 */
	snprintf(ch->sess_name, sizeof(ch->sess_name), "0x%016llx%016llx",
			be64_to_cpu(*(__be64 *)ch->i_port_id),
			be64_to_cpu(*(__be64 *)(ch->i_port_id + 8)));

	pr_debug("registering session %s\n", ch->sess_name);

	nacl = srpt_lookup_acl(sport, ch->i_port_id);
	if (!nacl) {
		pr_info("Rejected login because no ACL has been"
			" configured yet for initiator %s.\n", ch->sess_name);
		rej->reason = cpu_to_be32(
			      SRP_LOGIN_REJ_CHANNEL_LIMIT_REACHED);
		goto destroy_ib;
	}

	ch->sess = transport_init_session(TARGET_PROT_NORMAL);
	if (IS_ERR(ch->sess)) {
		rej->reason = cpu_to_be32(
			      SRP_LOGIN_REJ_INSUFFICIENT_RESOURCES);
		pr_debug("Failed to create session\n");
		goto deregister_session;
	}
	ch->sess->se_node_acl = &nacl->nacl;
	transport_register_session(&sport->port_tpg_1, &nacl->nacl, ch->sess, ch);

	pr_debug("Establish connection sess=%p name=%s cm_id=%p\n", ch->sess,
		 ch->sess_name, ch->cm_id);

	/* create srp_login_response */
	rsp->opcode = SRP_LOGIN_RSP;
	rsp->tag = req->tag;
	rsp->max_it_iu_len = req->req_it_iu_len;
	rsp->max_ti_iu_len = req->req_it_iu_len;
	ch->max_ti_iu_len = it_iu_len;
	rsp->buf_fmt = cpu_to_be16(SRP_BUF_FORMAT_DIRECT
				   | SRP_BUF_FORMAT_INDIRECT);
	rsp->req_lim_delta = cpu_to_be32(ch->rq_size);
	atomic_set(&ch->req_lim, ch->rq_size);
	atomic_set(&ch->req_lim_delta, 0);

	/* create cm reply */
	rep_param->qp_num = ch->qp->qp_num;
	rep_param->private_data = (void *)rsp;
	rep_param->private_data_len = sizeof *rsp;
	rep_param->rnr_retry_count = 7;
	rep_param->flow_control = 1;
	rep_param->failover_accepted = 0;
	rep_param->srq = 1;
	rep_param->responder_resources = 4;
	rep_param->initiator_depth = 4;

	ret = ib_send_cm_rep(cm_id, rep_param);
	if (ret) {
		pr_err("sending SRP_LOGIN_REQ response failed"
		       " (error code = %d)\n", ret);
		goto release_channel;
	}

	spin_lock_irq(&sdev->spinlock);
	list_add_tail(&ch->list, &sdev->rch_list);
	spin_unlock_irq(&sdev->spinlock);

	goto out;

release_channel:
	srpt_set_ch_state(ch, CH_RELEASING);
	transport_deregister_session_configfs(ch->sess);

deregister_session:
	transport_deregister_session(ch->sess);
	ch->sess = NULL;

destroy_ib:
	srpt_destroy_ch_ib(ch);

free_ring:
	srpt_free_ioctx_ring((struct srpt_ioctx **)ch->ioctx_ring,
			     ch->sport->sdev, ch->rq_size,
			     ch->rsp_size, DMA_TO_DEVICE);
free_ch:
	kfree(ch);

reject:
	rej->opcode = SRP_LOGIN_REJ;
	rej->tag = req->tag;
	rej->buf_fmt = cpu_to_be16(SRP_BUF_FORMAT_DIRECT
				   | SRP_BUF_FORMAT_INDIRECT);

	ib_send_cm_rej(cm_id, IB_CM_REJ_CONSUMER_DEFINED, NULL, 0,
			     (void *)rej, sizeof *rej);

out:
	kfree(rep_param);
	kfree(rsp);
	kfree(rej);

	return ret;
}

static void srpt_cm_rej_recv(struct ib_cm_id *cm_id)
{
	pr_info("Received IB REJ for cm_id %p.\n", cm_id);
	srpt_drain_channel(cm_id);
}

/**
 * srpt_cm_rtu_recv() - Process an IB_CM_RTU_RECEIVED or USER_ESTABLISHED event.
 *
 * An IB_CM_RTU_RECEIVED message indicates that the connection is established
 * and that the recipient may begin transmitting (RTU = ready to use).
 */
static void srpt_cm_rtu_recv(struct ib_cm_id *cm_id)
{
	struct srpt_rdma_ch *ch;
	int ret;

	ch = srpt_find_channel(cm_id->context, cm_id);
	BUG_ON(!ch);

	if (srpt_test_and_set_ch_state(ch, CH_CONNECTING, CH_LIVE)) {
		struct srpt_recv_ioctx *ioctx, *ioctx_tmp;

		ret = srpt_ch_qp_rts(ch, ch->qp);

		list_for_each_entry_safe(ioctx, ioctx_tmp, &ch->cmd_wait_list,
					 wait_list) {
			list_del(&ioctx->wait_list);
			srpt_handle_new_iu(ch, ioctx, NULL);
		}
		if (ret)
			srpt_close_ch(ch);
	}
}

static void srpt_cm_timewait_exit(struct ib_cm_id *cm_id)
{
	pr_info("Received IB TimeWait exit for cm_id %p.\n", cm_id);
	srpt_drain_channel(cm_id);
}

static void srpt_cm_rep_error(struct ib_cm_id *cm_id)
{
	pr_info("Received IB REP error for cm_id %p.\n", cm_id);
	srpt_drain_channel(cm_id);
}

/**
 * srpt_cm_dreq_recv() - Process reception of a DREQ message.
 */
static void srpt_cm_dreq_recv(struct ib_cm_id *cm_id)
{
	struct srpt_rdma_ch *ch;
	unsigned long flags;
	bool send_drep = false;

	ch = srpt_find_channel(cm_id->context, cm_id);
	BUG_ON(!ch);

	pr_debug("cm_id= %p ch->state= %d\n", cm_id, srpt_get_ch_state(ch));

	spin_lock_irqsave(&ch->spinlock, flags);
	switch (ch->state) {
	case CH_CONNECTING:
	case CH_LIVE:
		send_drep = true;
		ch->state = CH_DISCONNECTING;
		break;
	case CH_DISCONNECTING:
	case CH_DRAINING:
	case CH_RELEASING:
		WARN(true, "unexpected channel state %d\n", ch->state);
		break;
	}
	spin_unlock_irqrestore(&ch->spinlock, flags);

	if (send_drep) {
		if (ib_send_cm_drep(ch->cm_id, NULL, 0) < 0)
			pr_err("Sending IB DREP failed.\n");
		pr_info("Received DREQ and sent DREP for session %s.\n",
			ch->sess_name);
	}
}

/**
 * srpt_cm_drep_recv() - Process reception of a DREP message.
 */
static void srpt_cm_drep_recv(struct ib_cm_id *cm_id)
{
	pr_info("Received InfiniBand DREP message for cm_id %p.\n", cm_id);
	srpt_drain_channel(cm_id);
}

/**
 * srpt_cm_handler() - IB connection manager callback function.
 *
 * A non-zero return value will cause the caller destroy the CM ID.
 *
 * Note: srpt_cm_handler() must only return a non-zero value when transferring
 * ownership of the cm_id to a channel by srpt_cm_req_recv() failed. Returning
 * a non-zero value in any other case will trigger a race with the
 * ib_destroy_cm_id() call in srpt_release_channel().
 */
static int srpt_cm_handler(struct ib_cm_id *cm_id, struct ib_cm_event *event)
{
	int ret;

	ret = 0;
	switch (event->event) {
	case IB_CM_REQ_RECEIVED:
		ret = srpt_cm_req_recv(cm_id, &event->param.req_rcvd,
				       event->private_data);
		break;
	case IB_CM_REJ_RECEIVED:
		srpt_cm_rej_recv(cm_id);
		break;
	case IB_CM_RTU_RECEIVED:
	case IB_CM_USER_ESTABLISHED:
		srpt_cm_rtu_recv(cm_id);
		break;
	case IB_CM_DREQ_RECEIVED:
		srpt_cm_dreq_recv(cm_id);
		break;
	case IB_CM_DREP_RECEIVED:
		srpt_cm_drep_recv(cm_id);
		break;
	case IB_CM_TIMEWAIT_EXIT:
		srpt_cm_timewait_exit(cm_id);
		break;
	case IB_CM_REP_ERROR:
		srpt_cm_rep_error(cm_id);
		break;
	case IB_CM_DREQ_ERROR:
		pr_info("Received IB DREQ ERROR event.\n");
		break;
	case IB_CM_MRA_RECEIVED:
		pr_info("Received IB MRA event\n");
		break;
	default:
		pr_err("received unrecognized IB CM event %d\n", event->event);
		break;
	}

	return ret;
}

/**
 * srpt_perform_rdmas() - Perform IB RDMA.
 *
 * Returns zero upon success or a negative number upon failure.
 */
static int srpt_perform_rdmas(struct srpt_rdma_ch *ch,
			      struct srpt_send_ioctx *ioctx)
{
	struct ib_rdma_wr wr;
	struct ib_send_wr *bad_wr;
	struct rdma_iu *riu;
	int i;
	int ret;
	int sq_wr_avail;
	enum dma_data_direction dir;
	const int n_rdma = ioctx->n_rdma;

	dir = ioctx->cmd.data_direction;
	if (dir == DMA_TO_DEVICE) {
		/* write */
		ret = -ENOMEM;
		sq_wr_avail = atomic_sub_return(n_rdma, &ch->sq_wr_avail);
		if (sq_wr_avail < 0) {
			pr_warn("IB send queue full (needed %d)\n",
				n_rdma);
			goto out;
		}
	}

	ioctx->rdma_aborted = false;
	ret = 0;
	riu = ioctx->rdma_ius;
	memset(&wr, 0, sizeof wr);

	for (i = 0; i < n_rdma; ++i, ++riu) {
		if (dir == DMA_FROM_DEVICE) {
			wr.wr.opcode = IB_WR_RDMA_WRITE;
			wr.wr.wr_id = encode_wr_id(i == n_rdma - 1 ?
						SRPT_RDMA_WRITE_LAST :
						SRPT_RDMA_MID,
						ioctx->ioctx.index);
		} else {
			wr.wr.opcode = IB_WR_RDMA_READ;
			wr.wr.wr_id = encode_wr_id(i == n_rdma - 1 ?
						SRPT_RDMA_READ_LAST :
						SRPT_RDMA_MID,
						ioctx->ioctx.index);
		}
		wr.wr.next = NULL;
		wr.remote_addr = riu->raddr;
		wr.rkey = riu->rkey;
		wr.wr.num_sge = riu->sge_cnt;
		wr.wr.sg_list = riu->sge;

		/* only get completion event for the last rdma write */
		if (i == (n_rdma - 1) && dir == DMA_TO_DEVICE)
			wr.wr.send_flags = IB_SEND_SIGNALED;

		ret = ib_post_send(ch->qp, &wr.wr, &bad_wr);
		if (ret)
			break;
	}

	if (ret)
		pr_err("%s[%d]: ib_post_send() returned %d for %d/%d\n",
				 __func__, __LINE__, ret, i, n_rdma);
	if (ret && i > 0) {
		wr.wr.num_sge = 0;
		wr.wr.wr_id = encode_wr_id(SRPT_RDMA_ABORT, ioctx->ioctx.index);
		wr.wr.send_flags = IB_SEND_SIGNALED;
		while (ch->state == CH_LIVE &&
			ib_post_send(ch->qp, &wr.wr, &bad_wr) != 0) {
			pr_info("Trying to abort failed RDMA transfer [%d]\n",
				ioctx->ioctx.index);
			msleep(1000);
		}
		while (ch->state != CH_RELEASING && !ioctx->rdma_aborted) {
			pr_info("Waiting until RDMA abort finished [%d]\n",
				ioctx->ioctx.index);
			msleep(1000);
		}
	}
out:
	if (unlikely(dir == DMA_TO_DEVICE && ret < 0))
		atomic_add(n_rdma, &ch->sq_wr_avail);
	return ret;
}

/**
 * srpt_xfer_data() - Start data transfer from initiator to target.
 */
static int srpt_xfer_data(struct srpt_rdma_ch *ch,
			  struct srpt_send_ioctx *ioctx)
{
	int ret;

	ret = srpt_map_sg_to_ib_sge(ch, ioctx);
	if (ret) {
		pr_err("%s[%d] ret=%d\n", __func__, __LINE__, ret);
		goto out;
	}

	ret = srpt_perform_rdmas(ch, ioctx);
	if (ret) {
		if (ret == -EAGAIN || ret == -ENOMEM)
			pr_info("%s[%d] queue full -- ret=%d\n",
				__func__, __LINE__, ret);
		else
			pr_err("%s[%d] fatal error -- ret=%d\n",
			       __func__, __LINE__, ret);
		goto out_unmap;
	}

out:
	return ret;
out_unmap:
	srpt_unmap_sg_to_ib_sge(ch, ioctx);
	goto out;
}

static int srpt_write_pending_status(struct se_cmd *se_cmd)
{
	struct srpt_send_ioctx *ioctx;

	ioctx = container_of(se_cmd, struct srpt_send_ioctx, cmd);
	return srpt_get_cmd_state(ioctx) == SRPT_STATE_NEED_DATA;
}

/*
 * srpt_write_pending() - Start data transfer from initiator to target (write).
 */
static int srpt_write_pending(struct se_cmd *se_cmd)
{
	struct srpt_rdma_ch *ch;
	struct srpt_send_ioctx *ioctx;
	enum srpt_command_state new_state;
	enum rdma_ch_state ch_state;
	int ret;

	ioctx = container_of(se_cmd, struct srpt_send_ioctx, cmd);

	new_state = srpt_set_cmd_state(ioctx, SRPT_STATE_NEED_DATA);
	WARN_ON(new_state == SRPT_STATE_DONE);

	ch = ioctx->ch;
	BUG_ON(!ch);

	ch_state = srpt_get_ch_state(ch);
	switch (ch_state) {
	case CH_CONNECTING:
		WARN(true, "unexpected channel state %d\n", ch_state);
		ret = -EINVAL;
		goto out;
	case CH_LIVE:
		break;
	case CH_DISCONNECTING:
	case CH_DRAINING:
	case CH_RELEASING:
		pr_debug("cmd with tag %lld: channel disconnecting\n",
			 ioctx->cmd.tag);
		srpt_set_cmd_state(ioctx, SRPT_STATE_DATA_IN);
		ret = -EINVAL;
		goto out;
	}
	ret = srpt_xfer_data(ch, ioctx);

out:
	return ret;
}

static u8 tcm_to_srp_tsk_mgmt_status(const int tcm_mgmt_status)
{
	switch (tcm_mgmt_status) {
	case TMR_FUNCTION_COMPLETE:
		return SRP_TSK_MGMT_SUCCESS;
	case TMR_FUNCTION_REJECTED:
		return SRP_TSK_MGMT_FUNC_NOT_SUPP;
	}
	return SRP_TSK_MGMT_FAILED;
}

/**
 * srpt_queue_response() - Transmits the response to a SCSI command.
 *
 * Callback function called by the TCM core. Must not block since it can be
 * invoked on the context of the IB completion handler.
 */
static void srpt_queue_response(struct se_cmd *cmd)
{
	struct srpt_rdma_ch *ch;
	struct srpt_send_ioctx *ioctx;
	enum srpt_command_state state;
	unsigned long flags;
	int ret;
	enum dma_data_direction dir;
	int resp_len;
	u8 srp_tm_status;

	ioctx = container_of(cmd, struct srpt_send_ioctx, cmd);
	ch = ioctx->ch;
	BUG_ON(!ch);

	spin_lock_irqsave(&ioctx->spinlock, flags);
	state = ioctx->state;
	switch (state) {
	case SRPT_STATE_NEW:
	case SRPT_STATE_DATA_IN:
		ioctx->state = SRPT_STATE_CMD_RSP_SENT;
		break;
	case SRPT_STATE_MGMT:
		ioctx->state = SRPT_STATE_MGMT_RSP_SENT;
		break;
	default:
		WARN(true, "ch %p; cmd %d: unexpected command state %d\n",
			ch, ioctx->ioctx.index, ioctx->state);
		break;
	}
	spin_unlock_irqrestore(&ioctx->spinlock, flags);

	if (unlikely(WARN_ON_ONCE(state == SRPT_STATE_CMD_RSP_SENT)))
		return;

	dir = ioctx->cmd.data_direction;

	/* For read commands, transfer the data to the initiator. */
	if (dir == DMA_FROM_DEVICE && ioctx->cmd.data_length &&
	    !ioctx->queue_status_only) {
		ret = srpt_xfer_data(ch, ioctx);
		if (ret) {
			pr_err("xfer_data failed for tag %llu\n",
			       ioctx->cmd.tag);
			return;
		}
	}

	if (state != SRPT_STATE_MGMT)
		resp_len = srpt_build_cmd_rsp(ch, ioctx, ioctx->cmd.tag,
					      cmd->scsi_status);
	else {
		srp_tm_status
			= tcm_to_srp_tsk_mgmt_status(cmd->se_tmr_req->response);
		resp_len = srpt_build_tskmgmt_rsp(ch, ioctx, srp_tm_status,
						 ioctx->cmd.tag);
	}
	ret = srpt_post_send(ch, ioctx, resp_len);
	if (ret) {
		pr_err("sending cmd response failed for tag %llu\n",
		       ioctx->cmd.tag);
		srpt_unmap_sg_to_ib_sge(ch, ioctx);
		srpt_set_cmd_state(ioctx, SRPT_STATE_DONE);
		target_put_sess_cmd(&ioctx->cmd);
	}
}

static int srpt_queue_data_in(struct se_cmd *cmd)
{
	srpt_queue_response(cmd);
	return 0;
}

static void srpt_queue_tm_rsp(struct se_cmd *cmd)
{
	srpt_queue_response(cmd);
}

static void srpt_aborted_task(struct se_cmd *cmd)
{
	struct srpt_send_ioctx *ioctx = container_of(cmd,
				struct srpt_send_ioctx, cmd);

	srpt_unmap_sg_to_ib_sge(ioctx->ch, ioctx);
}

static int srpt_queue_status(struct se_cmd *cmd)
{
	struct srpt_send_ioctx *ioctx;

	ioctx = container_of(cmd, struct srpt_send_ioctx, cmd);
	BUG_ON(ioctx->sense_data != cmd->sense_buffer);
	if (cmd->se_cmd_flags &
	    (SCF_TRANSPORT_TASK_SENSE | SCF_EMULATED_TASK_SENSE))
		WARN_ON(cmd->scsi_status != SAM_STAT_CHECK_CONDITION);
	ioctx->queue_status_only = true;
	srpt_queue_response(cmd);
	return 0;
}

static void srpt_refresh_port_work(struct work_struct *work)
{
	struct srpt_port *sport = container_of(work, struct srpt_port, work);

	srpt_refresh_port(sport);
}

static int srpt_ch_list_empty(struct srpt_device *sdev)
{
	int res;

	spin_lock_irq(&sdev->spinlock);
	res = list_empty(&sdev->rch_list);
	spin_unlock_irq(&sdev->spinlock);

	return res;
}

/**
 * srpt_release_sdev() - Free the channel resources associated with a target.
 */
static int srpt_release_sdev(struct srpt_device *sdev)
{
	struct srpt_rdma_ch *ch, *tmp_ch;
	int res;

	WARN_ON_ONCE(irqs_disabled());

	BUG_ON(!sdev);

	spin_lock_irq(&sdev->spinlock);
	list_for_each_entry_safe(ch, tmp_ch, &sdev->rch_list, list)
		__srpt_close_ch(ch);
	spin_unlock_irq(&sdev->spinlock);

	res = wait_event_interruptible(sdev->ch_releaseQ,
				       srpt_ch_list_empty(sdev));
	if (res)
		pr_err("%s: interrupted.\n", __func__);

	return 0;
}

static struct srpt_port *__srpt_lookup_port(const char *name)
{
	struct ib_device *dev;
	struct srpt_device *sdev;
	struct srpt_port *sport;
	int i;

	list_for_each_entry(sdev, &srpt_dev_list, list) {
		dev = sdev->device;
		if (!dev)
			continue;

		for (i = 0; i < dev->phys_port_cnt; i++) {
			sport = &sdev->port[i];

			if (!strcmp(sport->port_guid, name))
				return sport;
		}
	}

	return NULL;
}

static struct srpt_port *srpt_lookup_port(const char *name)
{
	struct srpt_port *sport;

	spin_lock(&srpt_dev_lock);
	sport = __srpt_lookup_port(name);
	spin_unlock(&srpt_dev_lock);

	return sport;
}

/**
 * srpt_add_one() - Infiniband device addition callback function.
 */
static void srpt_add_one(struct ib_device *device)
{
	struct srpt_device *sdev;
	struct srpt_port *sport;
	struct ib_srq_init_attr srq_attr;
	int i;

	pr_debug("device = %p, device->dma_ops = %p\n", device,
		 device->dma_ops);

	sdev = kzalloc(sizeof *sdev, GFP_KERNEL);
	if (!sdev)
		goto err;

	sdev->device = device;
	INIT_LIST_HEAD(&sdev->rch_list);
	init_waitqueue_head(&sdev->ch_releaseQ);
	spin_lock_init(&sdev->spinlock);

	if (ib_query_device(device, &sdev->dev_attr))
		goto free_dev;

	sdev->pd = ib_alloc_pd(device);
	if (IS_ERR(sdev->pd))
		goto free_dev;

	sdev->srq_size = min(srpt_srq_size, sdev->dev_attr.max_srq_wr);

	srq_attr.event_handler = srpt_srq_event;
	srq_attr.srq_context = (void *)sdev;
	srq_attr.attr.max_wr = sdev->srq_size;
	srq_attr.attr.max_sge = 1;
	srq_attr.attr.srq_limit = 0;
	srq_attr.srq_type = IB_SRQT_BASIC;

	sdev->srq = ib_create_srq(sdev->pd, &srq_attr);
	if (IS_ERR(sdev->srq))
		goto err_pd;

	pr_debug("%s: create SRQ #wr= %d max_allow=%d dev= %s\n",
		 __func__, sdev->srq_size, sdev->dev_attr.max_srq_wr,
		 device->name);

	if (!srpt_service_guid)
		srpt_service_guid = be64_to_cpu(device->node_guid);

	sdev->cm_id = ib_create_cm_id(device, srpt_cm_handler, sdev);
	if (IS_ERR(sdev->cm_id))
		goto err_srq;

	/* print out target login information */
	pr_debug("Target login info: id_ext=%016llx,ioc_guid=%016llx,"
		 "pkey=ffff,service_id=%016llx\n", srpt_service_guid,
		 srpt_service_guid, srpt_service_guid);

	/*
	 * We do not have a consistent service_id (ie. also id_ext of target_id)
	 * to identify this target. We currently use the guid of the first HCA
	 * in the system as service_id; therefore, the target_id will change
	 * if this HCA is gone bad and replaced by different HCA
	 */
	if (ib_cm_listen(sdev->cm_id, cpu_to_be64(srpt_service_guid), 0))
		goto err_cm;

	INIT_IB_EVENT_HANDLER(&sdev->event_handler, sdev->device,
			      srpt_event_handler);
	if (ib_register_event_handler(&sdev->event_handler))
		goto err_cm;

	sdev->ioctx_ring = (struct srpt_recv_ioctx **)
		srpt_alloc_ioctx_ring(sdev, sdev->srq_size,
				      sizeof(*sdev->ioctx_ring[0]),
				      srp_max_req_size, DMA_FROM_DEVICE);
	if (!sdev->ioctx_ring)
		goto err_event;

	for (i = 0; i < sdev->srq_size; ++i)
		srpt_post_recv(sdev, sdev->ioctx_ring[i]);

	WARN_ON(sdev->device->phys_port_cnt > ARRAY_SIZE(sdev->port));

	for (i = 1; i <= sdev->device->phys_port_cnt; i++) {
		sport = &sdev->port[i - 1];
		sport->sdev = sdev;
		sport->port = i;
		sport->port_attrib.srp_max_rdma_size = DEFAULT_MAX_RDMA_SIZE;
		sport->port_attrib.srp_max_rsp_size = DEFAULT_MAX_RSP_SIZE;
		sport->port_attrib.srp_sq_size = DEF_SRPT_SQ_SIZE;
		INIT_WORK(&sport->work, srpt_refresh_port_work);
		INIT_LIST_HEAD(&sport->port_acl_list);
		spin_lock_init(&sport->port_acl_lock);

		if (srpt_refresh_port(sport)) {
			pr_err("MAD registration failed for %s-%d.\n",
			       srpt_sdev_name(sdev), i);
			goto err_ring;
		}
		snprintf(sport->port_guid, sizeof(sport->port_guid),
			"0x%016llx%016llx",
			be64_to_cpu(sport->gid.global.subnet_prefix),
			be64_to_cpu(sport->gid.global.interface_id));
	}

	spin_lock(&srpt_dev_lock);
	list_add_tail(&sdev->list, &srpt_dev_list);
	spin_unlock(&srpt_dev_lock);

out:
	ib_set_client_data(device, &srpt_client, sdev);
	pr_debug("added %s.\n", device->name);
	return;

err_ring:
	srpt_free_ioctx_ring((struct srpt_ioctx **)sdev->ioctx_ring, sdev,
			     sdev->srq_size, srp_max_req_size,
			     DMA_FROM_DEVICE);
err_event:
	ib_unregister_event_handler(&sdev->event_handler);
err_cm:
	ib_destroy_cm_id(sdev->cm_id);
err_srq:
	ib_destroy_srq(sdev->srq);
err_pd:
	ib_dealloc_pd(sdev->pd);
free_dev:
	kfree(sdev);
err:
	sdev = NULL;
	pr_info("%s(%s) failed.\n", __func__, device->name);
	goto out;
}

/**
 * srpt_remove_one() - InfiniBand device removal callback function.
 */
static void srpt_remove_one(struct ib_device *device, void *client_data)
{
	struct srpt_device *sdev = client_data;
	int i;

	if (!sdev) {
		pr_info("%s(%s): nothing to do.\n", __func__, device->name);
		return;
	}

	srpt_unregister_mad_agent(sdev);

	ib_unregister_event_handler(&sdev->event_handler);

	/* Cancel any work queued by the just unregistered IB event handler. */
	for (i = 0; i < sdev->device->phys_port_cnt; i++)
		cancel_work_sync(&sdev->port[i].work);

	ib_destroy_cm_id(sdev->cm_id);

	/*
	 * Unregistering a target must happen after destroying sdev->cm_id
	 * such that no new SRP_LOGIN_REQ information units can arrive while
	 * destroying the target.
	 */
	spin_lock(&srpt_dev_lock);
	list_del(&sdev->list);
	spin_unlock(&srpt_dev_lock);
	srpt_release_sdev(sdev);

	ib_destroy_srq(sdev->srq);
	ib_dealloc_pd(sdev->pd);

	srpt_free_ioctx_ring((struct srpt_ioctx **)sdev->ioctx_ring, sdev,
			     sdev->srq_size, srp_max_req_size, DMA_FROM_DEVICE);
	sdev->ioctx_ring = NULL;
	kfree(sdev);
}

static struct ib_client srpt_client = {
	.name = DRV_NAME,
	.add = srpt_add_one,
	.remove = srpt_remove_one
};

static int srpt_check_true(struct se_portal_group *se_tpg)
{
	return 1;
}

static int srpt_check_false(struct se_portal_group *se_tpg)
{
	return 0;
}

static char *srpt_get_fabric_name(void)
{
	return "srpt";
}

static char *srpt_get_fabric_wwn(struct se_portal_group *tpg)
{
	struct srpt_port *sport = container_of(tpg, struct srpt_port, port_tpg_1);

	return sport->port_guid;
}

static u16 srpt_get_tag(struct se_portal_group *tpg)
{
	return 1;
}

static u32 srpt_tpg_get_inst_index(struct se_portal_group *se_tpg)
{
	return 1;
}

static void srpt_release_cmd(struct se_cmd *se_cmd)
{
	struct srpt_send_ioctx *ioctx = container_of(se_cmd,
				struct srpt_send_ioctx, cmd);
	struct srpt_rdma_ch *ch = ioctx->ch;
	unsigned long flags;

	WARN_ON(ioctx->state != SRPT_STATE_DONE);
	WARN_ON(ioctx->mapped_sg_count != 0);

	if (ioctx->n_rbuf > 1) {
		kfree(ioctx->rbufs);
		ioctx->rbufs = NULL;
		ioctx->n_rbuf = 0;
	}

	spin_lock_irqsave(&ch->spinlock, flags);
	list_add(&ioctx->free_list, &ch->free_list);
	spin_unlock_irqrestore(&ch->spinlock, flags);
}

/**
 * srpt_close_session() - Forcibly close a session.
 *
 * Callback function invoked by the TCM core to clean up sessions associated
 * with a node ACL when the user invokes
 * rmdir /sys/kernel/config/target/$driver/$port/$tpg/acls/$i_port_id
 */
static void srpt_close_session(struct se_session *se_sess)
{
	DECLARE_COMPLETION_ONSTACK(release_done);
	struct srpt_rdma_ch *ch;
	struct srpt_device *sdev;
	unsigned long res;

	ch = se_sess->fabric_sess_ptr;
	WARN_ON(ch->sess != se_sess);

	pr_debug("ch %p state %d\n", ch, srpt_get_ch_state(ch));

	sdev = ch->sport->sdev;
	spin_lock_irq(&sdev->spinlock);
	BUG_ON(ch->release_done);
	ch->release_done = &release_done;
	__srpt_close_ch(ch);
	spin_unlock_irq(&sdev->spinlock);

	res = wait_for_completion_timeout(&release_done, 60 * HZ);
	WARN_ON(res == 0);
}

/**
 * srpt_sess_get_index() - Return the value of scsiAttIntrPortIndex (SCSI-MIB).
 *
 * A quote from RFC 4455 (SCSI-MIB) about this MIB object:
 * This object represents an arbitrary integer used to uniquely identify a
 * particular attached remote initiator port to a particular SCSI target port
 * within a particular SCSI target device within a particular SCSI instance.
 */
static u32 srpt_sess_get_index(struct se_session *se_sess)
{
	return 0;
}

static void srpt_set_default_node_attrs(struct se_node_acl *nacl)
{
}

/* Note: only used from inside debug printk's by the TCM core. */
static int srpt_get_tcm_cmd_state(struct se_cmd *se_cmd)
{
	struct srpt_send_ioctx *ioctx;

	ioctx = container_of(se_cmd, struct srpt_send_ioctx, cmd);
	return srpt_get_cmd_state(ioctx);
}

/**
 * srpt_parse_i_port_id() - Parse an initiator port ID.
 * @name: ASCII representation of a 128-bit initiator port ID.
 * @i_port_id: Binary 128-bit port ID.
 */
static int srpt_parse_i_port_id(u8 i_port_id[16], const char *name)
{
	const char *p;
	unsigned len, count, leading_zero_bytes;
	int ret;

	p = name;
	if (strncasecmp(p, "0x", 2) == 0)
		p += 2;
	ret = -EINVAL;
	len = strlen(p);
	if (len % 2)
		goto out;
	count = min(len / 2, 16U);
	leading_zero_bytes = 16 - count;
	memset(i_port_id, 0, leading_zero_bytes);
	ret = hex2bin(i_port_id + leading_zero_bytes, p, count);
	if (ret < 0)
		pr_debug("hex2bin failed for srpt_parse_i_port_id: %d\n", ret);
out:
	return ret;
}

/*
 * configfs callback function invoked for
 * mkdir /sys/kernel/conf