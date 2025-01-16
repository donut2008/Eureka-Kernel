h,
			   struct srpt_recv_ioctx *recv_ioctx,
			   struct srpt_send_ioctx *send_ioctx)
{
	struct se_cmd *cmd;
	struct srp_cmd *srp_cmd;
	uint64_t unpacked_lun;
	u64 data_len;
	enum dma_data_direction dir;
	sense_reason_t ret;
	int rc;

	BUG_ON(!send_ioctx);

	srp_cmd = recv_ioctx->ioctx.buf;
	cmd = &send_ioctx->cmd;
	cmd->tag = srp_cmd->tag;

	switch (srp_cmd->task_attr) {
	case SRP_CMD_SIMPLE_Q:
		cmd->sam_task_attr = TCM_SIMPLE_TAG;
		break;
	case SRP_CMD_ORDERED_Q:
	default:
		cmd->sam_task_attr = TCM_ORDERED_TAG;
		break;
	case SRP_CMD_HEAD_OF_Q:
		cmd->sam_task_attr = TCM_HEAD_TAG;
		break;
	case SRP_CMD_ACA:
		cmd->sam_task_attr = TCM_ACA_TAG;
		break;
	}

	if (srpt_get_desc_tbl(send_ioctx, srp_cmd, &dir, &data_len)) {
		pr_err("0x%llx: parsing SRP descriptor table failed.\n",
		       srp_cmd->tag);
		ret = TCM_INVALID_CDB_FIELD;
		goto send_sense;
	}

	unpacked_lun = srpt_unpack_lun((uint8_t *)&srp_cmd->lun,
				       sizeof(srp_cmd->lun));
	rc = target_submit_cmd(cmd, ch->sess, srp_cmd->cdb,
			&send_ioctx->sense_data[0], unpacked_lun, data_len,
			TCM_SIMPLE_TAG, dir, TARGET_SCF_ACK_KREF);
	if (rc != 0) {
		ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		goto send_sense;
	}
	return 0;

send_sense:
	transport_send_check_condition_and_sense(cmd, ret, 0);
	return -1;
}

static int srp_tmr_to_tcm(int fn)
{
	switch (fn) {
	case SRP_TSK_ABORT_TASK:
		return TMR_ABORT_TASK;
	case SRP_TSK_ABORT_TASK_SET:
		return TMR_ABORT_TASK_SET;
	case SRP_TSK_CLEAR_TASK_SET:
		return TMR_CLEAR_TASK_SET;
	case SRP_TSK_LUN_RESET:
		return TMR_LUN_RESET;
	case SRP_TSK_CLEAR_ACA:
		return TMR_CLEAR_ACA;
	default:
		return -1;
	}
}

/**
 * srpt_handle_tsk_mgmt() - Process an SRP_TSK_MGMT information unit.
 *
 * Returns 0 if and only if the request will be processed by the target core.
 *
 * For more information about SRP_TSK_MGMT information units, see also section
 * 6.7 in the SRP r16a document.
 */
static void srpt_handle_tsk_mgmt(struct srpt_rdma_ch *ch,
				 struct srpt_recv_ioctx *recv_ioctx,
				 struct srpt_send_ioctx *send_ioctx)
{
	struct srp_tsk_mgmt *srp_tsk;
	struct se_cmd *cmd;
	struct se_session *sess = ch->sess;
	uint64_t unpacked_lun;
	int tcm_tmr;
	int rc;

	BUG_ON(!send_ioctx);

	srp_tsk = recv_ioctx->ioctx.buf;
	cmd = &send_ioctx->cmd;

	pr_debug("recv tsk_mgmt fn %d for task_tag %lld and cmd tag %lld"
		 " cm_id %p sess %p\n", srp_tsk->tsk_mgmt_func,
		 srp_tsk->task_tag, srp_tsk->tag, ch->cm_id, ch->sess);

	srpt_set_cmd_state(send_ioctx, SRPT_STATE_MGMT);
	send_ioctx->cmd.tag = srp_tsk->tag;
	tcm_tmr = srp_tmr_to_tcm(srp_tsk->tsk_mgmt_func);
	unpacked_lun = srpt_unpack_lun((uint8_t *)&srp_tsk->lun,
				       sizeof(srp_tsk->lun));
	rc = target_submit_tmr(&send_ioctx->cmd, sess, NULL, unpacked_lun,
				srp_tsk, tcm_tmr, GFP_KERNEL, srp_tsk->task_tag,
				TARGET_SCF_ACK_KREF);
	if (rc != 0) {
		send_ioctx->cmd.se_tmr_req->response = TMR_FUNCTION_REJECTED;
		goto fail;
	}
	return;
fail:
	transport_send_check_condition_and_sense(cmd, 0, 0); // XXX:
}

/**
 * srpt_handle_new_iu() - Process a newly received information unit.
 * @ch:    RDMA channel through which the information unit has been received.
 * @ioctx: SRPT I/O context associated with the information unit.
 */
static void srpt_handle_new_iu(struct srpt_rdma_ch *ch,
			       struct srpt_recv_ioctx *recv_ioctx,
			       struct srpt_send_ioctx *send_ioctx)
{
	struct srp_cmd *srp_cmd;
	enum rdma_ch_state ch_state;

	BUG_ON(!ch);
	BUG_ON(!recv_ioctx);

	ib_dma_sync_single_for_cpu(ch->sport->sdev->device,
				   recv_ioctx->ioctx.dma, srp_max_req_size,
				   DMA_FROM_DEVICE);

	ch_state = srpt_get_ch_state(ch);
	if (unlikely(ch_state == CH_CONNECTING)) {
		list_add_tail(&recv_ioctx->wait_list, &ch->cmd_wait_list);
		goto out;
	}

	if (unlikely(ch_state != CH_LIVE))
		goto out;

	srp_cmd = recv_ioctx->ioctx.buf;
	if (srp_cmd->opcode == SRP_CMD || srp_cmd->opcode == SRP_TSK_MGMT) {
		if (!send_ioctx)
			send_ioctx = srpt_get_send_ioctx(ch);
		if (unlikely(!send_ioctx)) {
			list_add_tail(&recv_ioctx->wait_list,
				      &ch->cmd_wait_list);
			goto out;
		}
	}

	switch (srp_cmd->opcode) {
	case SRP_CMD:
		srpt_handle_cmd(ch, recv_ioctx, send_ioctx);
		break;
	case SRP_TSK_MGMT:
		srpt_handle_tsk_mgmt(ch, recv_ioctx, send_ioctx);
		break;
	case SRP_I_LOGOUT:
		pr_err("Not yet implemented: SRP_I_LOGOUT\n");
		break;
	case SRP_CRED_RSP:
		pr_debug("received SRP_CRED_RSP\n");
		break;
	case SRP_AER_RSP:
		pr_debug("received SRP_AER_RSP\n");
		break;
	case SRP_RSP:
		pr_err("Received SRP_RSP\n");
		break;
	default:
		pr_err("received IU with unknown opcode 0x%x\n",
		       srp_cmd->opcode);
		break;
	}

	srpt_post_recv(ch->sport->sdev, recv_ioctx);
out:
	return;
}

static void srpt_process_rcv_completion(struct ib_cq *cq,
					struct srpt_rdma_ch *ch,
					struct ib_wc *wc)
{
	struct srpt_device *sdev = ch->sport->sdev;
	struct srpt_recv_ioctx *ioctx;
	u32 index;

	index = idx_from_wr_id(wc->wr_id);
	if (wc->status == IB_WC_SUCCESS) {
		int req_lim;

		req_lim = atomic_dec_return(&ch->req_lim);
		if (unlikely(req_lim < 0))
			pr_err("req_lim = %d < 0\n", req_lim);
		ioctx = sdev->ioctx_ring[index];
		srpt_handle_new_iu(ch, ioctx, NULL);
	} else {
		pr_info("receiving failed for idx %u with status %d\n",
			index, wc->status);
	}
}

/**
 * srpt_process_send_completion() - Process an IB send completion.
 *
 * Note: Although this has not yet been observed during tests, at least in
 * theory it is possible that the srpt_get_send_ioctx() call invoked by
 * srpt_handle_new_iu() fails. This is possible because the req_lim_delta
 * value in each response is set to one, and it is possible that this response
 * makes the initiator send a new request before the send completion for that
 * response has been processed. This could e.g. happen if the call to
 * srpt_put_send_iotcx() is delayed because of a higher priority interrupt or
 * if IB retransmission causes generation of the send completion to be
 * delayed. Incoming information units for which srpt_get_send_ioctx() fails
 * are queued on cmd_wait_list. The code below processes these delayed
 * requests one at a time.
 */
static void srpt_process_send_completion(struct ib_cq *cq,
					 struct srpt_rdma_ch *ch,
					 struct ib_wc *wc)
{
	struct srpt_send_ioctx *send_ioctx;
	uint32_t index;
	enum srpt_opcode opcode;

	index = idx_from_wr_id(wc->wr_id);
	opcode = opcode_from_wr_id(wc->wr_id);
	send_ioctx = ch->ioctx_ring[index];
	if (wc->status == IB_WC_SUCCESS) {
		if (opcode == SRPT_SEND)
			srpt_handle_send_comp(ch, send_ioctx);
		else {
			WARN_ON(opcode != SRPT_RDMA_ABORT &&
				wc->opcode != IB_WC_RDMA_READ);
			srpt_handle_rdma_comp(ch, send_ioctx, opcode);
		}
	} else {
		if (opcode == SRPT_SEND) {
			pr_info("sending response for idx %u failed"
				" with status %d\n", index, wc->status);
			srpt_handle_send_err_comp(ch, wc->wr_id);
		} else if (opcode != SRPT_RDMA_MID) {
			pr_info("RDMA t %d for idx %u failed with"
				" status %d\n", opcode, index, wc->status);
			srpt_handle_rdma_err_comp(ch, send_ioctx, opcode);
		}
	}

	while (unlikely(opcode == SRPT_SEND
			&& !list_empty(&ch->cmd_wait_list)
			&& srpt_get_ch_state(ch) == CH_LIVE
			&& (send_ioctx = srpt_get_send_ioctx(ch)) != NULL)) {
		struct srpt_recv_ioctx *recv_ioctx;

		recv_ioctx = list_first_entry(&ch->cmd_wait_list,
					      struct srpt_recv_ioctx,
					      wait_list);
		list_del(&recv_ioctx->wait_list);
		srpt_handle_new_iu(ch, recv_ioctx, send_ioctx);
	}
}

static void srpt_process_completion(struct ib_cq *cq, struct srpt_rdma_ch *ch)
{
	struct ib_wc *const wc = ch->wc;
	int i, n;

	WARN_ON(cq != ch->cq);

	ib_req_notify_cq(cq, IB_CQ_NEXT_COMP);
	while ((n = ib_poll_cq(cq, ARRAY_SIZE(ch->wc), wc)) > 0) {
		for (i = 0; i < n; i++) {
			if (opcode_from_wr_id(wc[i].wr_id) == SRPT_RECV)
				srpt_process_rcv_completion(cq, ch, &wc[i]);
			else
				srpt_process_send_completion(cq, ch, &wc[i]);
		}
	}
}

/**
 * srpt_completion() - IB completion queue callback function.
 *
 * Notes:
 * - It is guaranteed that a completion handler will never be invoked
 *   concurrently on two different CPUs for the same completion queue. See also
 *   Documentation/infiniband/core_locking.txt and the implementation of
 *   handle_edge_irq() in kernel/irq/chip.c.
 * - When threaded IRQs are enabled, completion handlers are invoked in thread
 *   context instead of interrupt context.
 */
static void srpt_completion(struct ib_cq *cq, void *ctx)
{
	struct srpt_rdma_ch *ch = ctx;

	wake_up_interruptible(&ch->wait_queue);
}

static int srpt_compl_thread(void *arg)
{
	struct srpt_rdma_ch *ch;

	/* Hibernation / freezing of the SRPT kernel thread is not supported. */
	current->flags |= PF_NOFREEZE;

	ch = arg;
	BUG_ON(!ch);
	pr_info("Session %s: kernel thread %s (PID %d) started\n",
		ch->sess_name, ch->thread->comm, current->pid);
	while (!kthread_should_stop()) {
		wait_event_interruptible(ch->wait_queue,
			(srpt_process_completion(ch->cq, ch),
			 kthread_should_stop()));
	}
	pr_info("Session %s: kernel thread %s (PID %d) stopped\n",
		ch->sess_name, ch->thread->comm, current->pid);
	return 0;
}

/**
 * srpt_create_ch_ib() - Create receive and send completion queues.
 */
static int srpt_create_ch_ib(struct srpt_rdma_ch *ch)
{
	struct ib_qp_init_attr *qp_init;
	struct srpt_port *sport = ch->sport;
	struct srpt_device *sdev = sport->sdev;
	u32 srp_sq_size = sport->port_attrib.srp_sq_size;
	struct ib_cq_init_attr cq_attr = {};
	int ret;

	WARN_ON(ch->rq_size < 1);

	ret = -ENOMEM;
	qp_init = kzalloc(sizeof *qp_init, GFP_KERNEL);
	if (!qp_init)
		goto out;

retry:
	cq_attr.cqe = ch->rq_size + srp_sq_size;
	ch->cq = ib_create_cq(sdev->device, srpt_completion, NULL, ch,
			      &cq_attr);
	if (IS_ERR(ch->cq)) {
		ret = PTR_ERR(ch->cq);
		pr_err("failed to create CQ cqe= %d ret= %d\n",
		       ch->rq_size + srp_sq_size, ret);
		goto out;
	}

	qp_init->qp_context = (void *)ch;
	qp_init->event_handler
		= (void(*)(struct ib_event *, void*))srpt_qp_event;
	qp_init->send_cq = ch->cq;
	qp_init->recv_cq = ch->cq;
	qp_init->srq = sdev->srq;
	qp_init->sq_sig_type = IB_SIGNAL_REQ_WR;
	qp_init->qp_type = IB_QPT_RC;
	qp_init->cap.max_send_wr = srp_sq_size;
	qp_init->cap.max_send_sge = SRPT_DEF_SG_PER_WQE;

	ch->qp = ib_create_qp(sdev->pd, qp_init);
	if (IS_ERR(ch->qp)) {
		ret = PTR_ERR(ch->qp);
		if (ret == -ENOMEM) {
			srp_sq_size /= 2;
			if (srp_sq_size >= MIN_SRPT_SQ_SIZE) {
				ib_destroy_cq(ch->cq);
				goto retry;
			}
		}
		pr_err("failed to create_qp ret= %d\n", ret);
		goto err_destroy_cq;
	}

	atomic_set(&ch->sq_wr_avail, qp_init->cap.max_send_wr);

	pr_debug("%s: max_cqe= %d max_sge= %d sq_size = %d cm_id= %p\n",
		 __func__, ch->cq->cqe, qp_init->cap.max_send_sge,
		 qp_init->cap.max_send_wr, ch->cm_id);

	ret = srpt_init_ch_qp(ch, ch->qp);
	if (ret)
		goto err_destroy_qp;

	init_waitqueue_head(&ch->wait_queue);

	pr_debug("creating thread for session %s\n", ch->sess_name);

	ch->thread = kthread_run(srpt_compl_thread, ch, "ib_srpt_compl");
	if (IS_ERR(ch->thread)) {
		pr_err("failed to create kernel thread %ld\n",
		       PTR_ERR(ch->thread));
		ch->thread = NULL;
		goto err_destroy_qp;
	}

out:
	kfree(qp_init);
	return ret;

err_destroy_qp:
	ib_destroy_qp(ch->qp);
err_destroy_cq:
	ib_destroy_cq(ch->cq);
	goto out;
}

static void srpt_destroy_ch_ib(struct srpt_rdma_ch *ch)
{
	if (ch->thread)
		kthread_stop(ch->thread);

	ib_destroy_qp(ch->qp);
	ib_destroy_cq(ch->cq);
}

/**
 * __srpt_close_ch() - Close an RDMA channel by setting the QP error state.
 *
 * Reset the QP and make sure all resources associated with the channel will
 * be deallocated at an appropriate time.
 *
 * Note: The caller must hold ch->sport->sdev->spinlock.
 */
static void __srpt_close_ch(struct srpt_rdma_ch *ch)
{
	enum rdma_ch_state prev_state;
	unsigned long flags;

	spin_lock_irqsave(&ch->spinlock, flags);
	prev_state = ch->state;
	switch (prev_state) {
	case CH_CONNECTING:
	case CH_LIVE:
		ch->state = CH_DISCONNECTING;
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&ch->spinlock, flags);

	switch (prev_state) {
	case CH_CONNECTING:
		ib_send_cm_rej(ch->cm_id, IB_CM_REJ_NO_RESOURCES, NULL, 0,
			       NULL, 0);
		/* fall through */
	case CH_LIVE:
		if (ib_send_cm_dreq(ch->cm_id, NULL, 0) < 0)
			pr_err("sending CM DREQ failed.\n");
		break;
	case CH_DISCONNECTING:
		break;
	case CH_DRAINING:
	case CH_RELEASING:
		break;
	}
}

/**
 * srpt_close_ch() - Close an RDMA channel.
 */
static void srpt_close_ch(struct srpt_rdma_ch *ch)
{
	struct srpt_device *sdev;

	sdev = ch->sport->sdev;
	spin_lock_irq(&sdev->spinlock);
	__srpt_close_ch(ch);
	spin_unlock_irq(&sdev->spinlock);
}

/**
 * srpt_shutdown_session() - Whether or not a session may be shut down.
 */
static int srpt_shutdown_session(struct se_session *se_sess)
{
	struct srpt_rdma_ch *ch = se_sess->fabric_sess_ptr;
	unsigned long flags;

	spin_lock_irqsave(&ch->spinlock, flags);
	if (ch->in_shutdown) {
		spin_unlock_irqrestore(&ch->spinlock, flags);
		return true;
	}

	ch->in_shutdown = true;
	target_sess_cmd_list_set_waiting(se_sess);
	spin_unlock_irqrestore(&ch->spinlock, flags);

	return true;
}

/**
 * srpt_drain_channel() - Drain a channel by resetting the IB queue pair.
 * @cm_id: Pointer to the CM ID of the channel to be drained.
 *
 * Note: Must be called from inside srpt_cm_handler to avoid a race between
 * accessing sdev->spinlock and the call to kfree(sdev) in srpt_remove_one()
 * (the caller of srpt_cm_handler holds the cm_id spinlock; srpt_remove_one()
 * waits until all target sessions for the associated IB device have been
 * unregistered and target session registration involves a call to
 * ib_destroy_cm_id(), which locks the cm_id spinlock and hence waits until
 * this function has finished).
 */
static void srpt_drain_channel(struct ib_cm_id *cm_id)
{
	struct srpt_device *sdev;
	struct srpt_rdma_ch *ch;
	int ret;
	bool do_reset = false;

	WARN_ON_ONCE(irqs_disabled());

	sdev = cm_id->context;
	BUG_ON(!sdev);
	spin_lock_irq(&sdev->spinlock);
	list_for_each_entry(ch, &sdev->rch_list, list) {
		if (ch->cm_id == cm_id) {
			do_reset = srpt_test_and_set_ch_state(ch,
					CH_CONNECTING, CH_DRAINING) ||
				   srpt_test_and_set_ch_state(ch,
					CH_LIVE, CH_DRAINING) ||
				   srpt_test_and_set_ch_state(ch,
					CH_DISCONNECTING, CH_DRAINING);
			break;
		}
	}
	spin_unlock_irq(&sdev->spinlock);

	if (do_reset) {
		if (ch->sess)
			srpt_shutdown_session(ch->sess);

		ret = srpt_ch_qp_err(ch);
		if (ret < 0)
			pr_err("Setting queue pair in error state"
			       " failed: %d\n", ret);
	}
}

/**
 * srpt_find_channel() - Look up an RDMA channel.
 * @cm_id: Pointer to the CM ID of the channel to be looked up.
 *
 * Return NULL if no matching RDMA channel has been found.
 */
static struct srpt_rdma_ch *srpt_find_channel(struct srpt_device *sdev,
					      struct ib_cm_id *cm_id)
{
	struct srpt_rdma_ch *ch;
	bool found;

	WARN_ON_ONCE(irqs_disabled());
	BUG_ON(!sdev);

	found = false;
	spin_lock_irq(&sdev->spinlock);
	list_for_each_entry(ch, &sdev->rch_list, list) {
		if (ch->cm_id == cm_id) {
			found = true;
			break;
		}
	}
	spin_unlock_irq(&sdev->spinlock);

	return found ? ch : NULL;
}

/**
 * srpt_release_channel() - Release channel resources.
 *
 * Schedules the actual release because:
 * - Calling the ib_destroy_cm_id() call from inside an IB CM callback would
 *   trigger a deadlock.
 * - It is not safe to call TCM transport_* functions from interrupt context.
 */
static void srpt_release_channel(struct srpt_rdma_ch *ch)
{
	schedule_work(&ch->release_work);
}

static void srpt_release_channel_work(struct work_struct *w)
{
	struct srpt_rdma_ch *ch;
	struct srpt_device *sdev;
	struct se_session *se_sess;

	ch = container_of(w, struct srpt_rdma_ch, release_work);
	pr_debug("ch = %p; ch->sess = %p; release_done = %p\n", ch, ch->sess,
		 ch->release_done);

	sdev = ch->sport->sdev;
	BUG_ON(!sdev);

	se_sess = ch->sess;
	BUG_ON(!se_sess);

	target_wait_for_sess_cmds(se_sess);

	transport_deregister_session_configfs(se_sess);
	transport_deregister_session(se_sess);
	ch->sess = NULL;

	ib_destroy_cm_id(ch->cm_id);

	srpt_destroy_ch_ib(ch);

	srpt_free_ioctx_ring((struct srpt_ioctx **)ch->ioctx_ring,
			     ch->sport->sdev, ch->rq_size,
			     ch->rsp_size, DMA_TO_DEVICE);

	spin_lock_irq(&sdev->spinlock);
	list_del(&ch->list);
	spin_unlock_irq(&sdev->spinlock);

	if (ch->release_done)
		complete(ch->release_done);

	wake_up(&sdev->ch_releaseQ);

	kfree(ch);
}

static struct srpt_node_acl *__srpt_lookup_acl(struct srpt_port *sport,
					       u8 i_port_id[16])
{
	struct srpt_node_acl *nacl;

	list_for_each_entry(nacl, &sport->port_acl_list, list)
		if (memcmp(nacl->i_port_id, i_port_id,
			   sizeof(nacl->i_port_id)) == 0)
			return nacl;

	return NULL;
}

static struct srpt_node_acl *srpt_lookup_acl(struct srpt_port *sport,
					     u8 i_port_id[16])
{
	struct srpt_node_acl *nacl;

	spin_lock_irq(&sport->port_acl_lock);
	nacl = __srpt_lookup_acl(sport, i_port_id);
	spin_unlock_irq(&sport->port_acl_lock);

	return nacl;
}

/**
 * srpt_cm_req_recv() - Process the event IB_CM_REQ_RECEIVED.
 *
 * Ownership of the cm_id is transferred to the target session if this
 * functions returns zero. Otherwise the caller remains the owner of cm_id.
 */
static int srpt_cm_req_recv(struct ib_cm_id *cm_id,
			    struct ib_cm_req_event_param *param,
			    void *private_data)
{
	struct srpt_device *sdev = cm_id->context;
	struct srpt_port *sport = &sdev->port[param->port - 1];
	struct srp_login_req *req;
	struct srp_login_rsp *rsp;
	struct srp_login_rej *rej;
	struct ib_cm_rep_param *rep_param;
	struct srpt_rdma_ch *ch, *tmp_ch;
	struct srpt_node_acl *nacl;
	u32 it_iu_len;
	int i;
	int ret = 0;

	WARN_ON_ONCE(irqs_disabled());

	if (WARN_ON(!sdev || !private_data))
		return -EINVAL;

	req = (struct srp_login_req *)private_data;

	it_iu_len = be32_to_cpu(req->req_it_iu_len);

	pr_info("Received SRP_LOGIN_REQ with i_port_id 0x%llx:0x%llx,"
		" t_port_id 0x%llx:0x%llx and it_iu_len %d on port %d"
		" (guid=0x%llx:0x%llx)\n",
		be64_to_cpu(*(__be64 *)&req->initiator_port_id[0]),
		be64_to_cpu(*(__be64 *)&req->initiator_port_id[8]),
		be64_to_cpu(*(__be64 *)&req->target_port_id[0]),
		be64_to_cpu(*(__be64 *)&req->target_port_id[8]),
		it_iu_len,
		param->port,
		be64_to_cpu(*(__be64 *)&sdev->port[param->port - 1].gid.raw[0]),
		be64_to_cpu(*(__be64 *)&sdev->port[param->port - 1].gid.raw[8]));

	rsp = kzalloc(sizeof *rsp, GFP_KERNEL);
	rej = kzalloc(sizeof *rej, GFP_KERNEL);
	rep_param = kzalloc(sizeof *rep_param, GFP_KERNEL);

	if (!rsp || !rej || !rep_param) {
		ret = -ENOMEM;
		goto out;
	}

	if (it_iu_len > srp_max_req_size || it_iu_len < 64) {
		rej->reason = cpu_to_be32(
			      SRP_LOGIN_REJ_REQ_IT_IU_LENGTH_TOO_LARGE);
		ret = -EINVAL;
		pr_err("rejected SRP_LOGIN_REQ because its"
		       " length (%d bytes) is out of range (%d .. %d)\n",
		       it_iu_len, 64, srp_max_req_size);
		goto reject;
	}

	if (!sport->enabled) {
		rej->reason = cpu_to_be32(
			      SRP_LOGIN_REJ_INSUFFICIENT_RESOURCES);
		ret = -EINVAL;
		pr_err("rejected SRP_LOGIN_REQ because the target port"
		       " has not yet been enabled\n");
		goto reject;
	}

	if ((req->req_flags & SRP_MTCH_ACTION) == SRP_MULTICHAN_SINGLE) {
		rsp->rsp_flags = SRP_LOGIN_RSP_MULTICHAN_NO_CHAN;

		spin_lock_irq(&sdev->spinlock);

		list_for_each_entry_safe(ch, tmp_ch, &sdev->rch_list, list) {
			if (!memcmp(ch->i_port_id, req->initiator_port_id, 16)
			    && !memcmp(ch->t_port_id, req->target_port_id, 16)
			    && param->port == ch->sport->port
			    && param->listen_id == ch->sport->sdev->cm_id
			    && ch->cm_id) {
				enum rdma_ch_state ch_state;

				ch_state = srpt_get_ch_state(ch);
				if (ch_state != CH_CONNECTING
				    && ch_state != CH_LIVE)
					continue;

				/* found an existing channel */
				pr_debug("Found existing channel %s"
					 " cm_id= %p state= %d\n",
					 ch->sess_name, ch->cm_id, ch_state);

				__srpt_close_ch(ch);

				rsp->rsp_flags =
					SRP_LOGIN_RSP_MULTICHAN_TERMINATED;
			}
		}

		spin_unlock_irq(&sdev->spinlock);

	} else
		rsp->rsp_flags = SRP_LOGIN_RSP_MULTICHAN_MAINTAINED;

	if (*(__be64 *)req->target_port_id != cpu_to_be64(srpt_service_guid)
	    || *(__be64 *)(req->target_port_id + 8) !=
	       cpu_to_be64(srpt_service_guid)) {
		rej->reason = cpu_to_be32(
			      SRP_LOGIN_REJ_UNABLE_ASSOCIATE_CHANNEL);
		ret = -ENOMEM;
		pr_err("rejected SRP_LOG