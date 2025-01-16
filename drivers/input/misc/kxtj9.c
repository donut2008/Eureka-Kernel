response_timeout  = 20;
	req->param.retry_count                = target->tl_retry_count;
	req->param.rnr_retry_count 	      = 7;
	req->param.max_cm_retries 	      = 15;

	req->priv.opcode     	= SRP_LOGIN_REQ;
	req->priv.tag        	= 0;
	req->priv.req_it_iu_len = cpu_to_be32(target->max_iu_len);
	req->priv.req_buf_fmt 	= cpu_to_be16(SRP_BUF_FORMAT_DIRECT |
					      SRP_BUF_FORMAT_INDIRECT);
	req->priv.req_flags	= (multich ? SRP_MULTICHAN_MULTI :
				   SRP_MULTICHAN_SINGLE);
	/*
	 * In the published SRP specification (draft rev. 16a), the
	 * port identifier format is 8 bytes of ID extension followed
	 * by 8 bytes of GUID.  Older drafts put the two halves in the
	 * opposite order, so that the GUID comes first.
	 *
	 * Targets conforming to these obsolete drafts can be
	 * recognized by the I/O Class they report.
	 */
	if (target->io_class == SRP_REV10_IB_IO_CLASS) {
		memcpy(req->priv.initiator_port_id,
		       &target->sgid.global.interface_id, 8);
		memcpy(req->priv.initiator_port_id + 8,
		       &target->initiator_ext, 8);
		memcpy(req->priv.target_port_id,     &target->ioc_guid, 8);
		memcpy(req->priv.target_port_id + 8, &target->id_ext, 8);
	} else {
		memcpy(req->priv.initiator_port_id,
		       &target->initiator_ext, 8);
		memcpy(req->priv.initiator_port_id + 8,
		       &target->sgid.global.interface_id, 8);
		memcpy(req->priv.target_port_id,     &target->id_ext, 8);
		memcpy(req->priv.target_port_id + 8, &target->ioc_guid, 8);
	}

	/*
	 * Topspin/Cisco SRP targets will reject our login unless we
	 * zero out the first 8 bytes of our initiator port ID and set
	 * the second 8 bytes to the local node GUID.
	 */
	if (srp_target_is_topspin(target)) {
		shost_printk(KERN_DEBUG, target->scsi_host,
			     PFX "Topspin/Cisco initiator port ID workaround "
			     "activated for target GUID %016llx\n",
			     be64_to_cpu(target->ioc_guid));
		memset(req->priv.initiator_port_id, 0, 8);
		memcpy(req->priv.initiator_port_id + 8,
		       &target->srp_host->srp_dev->dev->node_guid, 8);
	}

	status = ib_send_cm_req(ch->cm_id, &req->param);

	kfree(req);

	return status;
}

static bool srp_queue_remove_work(struct srp_target_port *target)
{
	bool changed = false;

	spin_lock_irq(&target->lock);
	if (target->state != SRP_TARGET_REMOVED) {
		target->state = SRP_TARGET_REMOVED;
		changed = true;
	}
	spin_unlock_irq(&target->lock);

	if (changed)
		queue_work(srp_remove_wq, &target->remove_work);

	return changed;
}

static void srp_disconnect_target(struct srp_target_port *target)
{
	struct srp_rdma_ch *ch;
	int i;

	/* XXX should send SRP_I_LOGOUT request */

	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];
		ch->connected = false;
		if (ch->cm_id && ib_send_cm_dreq(ch->cm_id, NULL, 0)) {
			shost_printk(KERN_DEBUG, target->scsi_host,
				     PFX "Sending CM DREQ failed\n");
		}
	}
}

static void srp_free_req_data(struct srp_target_port *target,
			      struct srp_rdma_ch *ch)
{
	struct srp_device *dev = target->srp_host->srp_dev;
	struct ib_device *ibdev = dev->dev;
	struct srp_request *req;
	int i;

	if (!ch->req_ring)
		return;

	for (i = 0; i < target->req_ring_size; ++i) {
		req = &ch->req_ring[i];
		if (dev->use_fast_reg) {
			kfree(req->fr_list);
		} else {
			kfree(req->fmr_list);
			kfree(req->map_page);
		}
		if (req->indirect_dma_addr) {
			ib_dma_unmap_single(ibdev, req->indirect_dma_addr,
					    target->indirect_size,
					    DMA_TO_DEVICE);
		}
		kfree(req->indirect_desc);
	}

	kfree(ch->req_ring);
	ch->req_ring = NULL;
}

static int srp_alloc_req_data(struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;
	struct srp_device *srp_dev = target->srp_host->srp_dev;
	struct ib_device *ibdev = srp_dev->dev;
	struct srp_request *req;
	void *mr_list;
	dma_addr_t dma_addr;
	int i, ret = -ENOMEM;

	ch->req_ring = kcalloc(target->req_ring_size, sizeof(*ch->req_ring),
			       GFP_KERNEL);
	if (!ch->req_ring)
		goto out;

	for (i = 0; i < target->req_ring_size; ++i) {
		req = &ch->req_ring[i];
		mr_list = kmalloc(target->cmd_sg_cnt * sizeof(void *),
				  GFP_KERNEL);
		if (!mr_list)
			goto out;
		if (srp_dev->use_fast_reg) {
			req->fr_list = mr_list;
		} else {
			req->fmr_list = mr_list;
			req->map_page = kmalloc(srp_dev->max_pages_per_mr *
						sizeof(void *), GFP_KERNEL);
			if (!req->map_page)
				goto out;
		}
		req->indirect_desc = kmalloc(target->indirect_size, GFP_KERNEL);
		if (!req->indirect_desc)
			goto out;

		dma_addr = ib_dma_map_single(ibdev, req->indirect_desc,
					     target->indirect_size,
					     DMA_TO_DEVICE);
		if (ib_dma_mapping_error(ibdev, dma_addr))
			goto out;

		req->indirect_dma_addr = dma_addr;
	}
	ret = 0;

out:
	return ret;
}

/**
 * srp_del_scsi_host_attr() - Remove attributes defined in the host template.
 * @shost: SCSI host whose attributes to remove from sysfs.
 *
 * Note: Any attributes defined in the host template and that did not exist
 * before invocation of this function will be ignored.
 */
static void srp_del_scsi_host_attr(struct Scsi_Host *shost)
{
	struct device_attribute **attr;

	for (attr = shost->hostt->shost_attrs; attr && *attr; ++attr)
		device_remove_file(&shost->shost_dev, *attr);
}

static void srp_remove_target(struct srp_target_port *target)
{
	struct srp_rdma_ch *ch;
	int i;

	WARN_ON_ONCE(target->state != SRP_TARGET_REMOVED);

	srp_del_scsi_host_attr(target->scsi_host);
	srp_rport_get(target->rport);
	srp_remove_host(target->scsi_host);
	scsi_remove_host(target->scsi_host);
	srp_stop_rport_timers(target->rport);
	srp_disconnect_target(target);
	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];
		srp_free_ch_ib(target, ch);
	}
	cancel_work_sync(&target->tl_err_work);
	srp_rport_put(target->rport);
	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];
		srp_free_req_data(target, ch);
	}
	kfree(target->ch);
	target->ch = NULL;

	spin_lock(&target->srp_host->target_lock);
	list_del(&target->list);
	spin_unlock(&target->srp_host->target_lock);

	scsi_host_put(target->scsi_host);
}

static void srp_remove_work(struct work_struct *work)
{
	struct srp_target_port *target =
		container_of(work, struct srp_target_port, remove_work);

	WARN_ON_ONCE(target->state != SRP_TARGET_REMOVED);

	srp_remove_target(target);
}

static void srp_rport_delete(struct srp_rport *rport)
{
	struct srp_target_port *target = rport->lld_data;

	srp_queue_remove_work(target);
}

/**
 * srp_connected_ch() - number of connected channels
 * @target: SRP target port.
 */
static int srp_connected_ch(struct srp_target_port *target)
{
	int i, c = 0;

	for (i = 0; i < target->ch_count; i++)
		c += target->ch[i].connected;

	return c;
}

static int srp_connect_ch(struct srp_rdma_ch *ch, bool multich)
{
	struct srp_target_port *target = ch->target;
	int ret;

	WARN_ON_ONCE(!multich && srp_connected_ch(target) > 0);

	ret = srp_lookup_path(ch);
	if (ret)
		goto out;

	while (1) {
		init_completion(&ch->done);
		ret = srp_send_req(ch, multich);
		if (ret)
			goto out;
		ret = wait_for_completion_interruptible(&ch->done);
		if (ret < 0)
			goto out;

		/*
		 * The CM event handling code will set status to
		 * SRP_PORT_REDIRECT if we get a port redirect REJ
		 * back, or SRP_DLID_REDIRECT if we get a lid/qp
		 * redirect REJ back.
		 */
		ret = ch->status;
		switch (ret) {
		case 0:
			ch->connected = true;
			goto out;

		case SRP_PORT_REDIRECT:
			ret = srp_lookup_path(ch);
			if (ret)
				goto out;
			break;

		case SRP_DLID_REDIRECT:
			break;

		case SRP_STALE_CONN:
			shost_printk(KERN_ERR, target->scsi_host, PFX
				     "giving up on stale connection\n");
			ret = -ECONNRESET;
			goto out;

		default:
			goto out;
		}
	}

out:
	return ret <= 0 ? ret : -ENODEV;
}

static int srp_inv_rkey(struct srp_rdma_ch *ch, u32 rkey)
{
	struct ib_send_wr *bad_wr;
	struct ib_send_wr wr = {
		.opcode		    = IB_WR_LOCAL_INV,
		.wr_id		    = LOCAL_INV_WR_ID_MASK,
		.next		    = NULL,
		.num_sge	    = 0,
		.send_flags	    = 0,
		.ex.invalidate_rkey = rkey,
	};

	return ib_post_send(ch->qp, &wr, &bad_wr);
}

static void srp_unmap_data(struct scsi_cmnd *scmnd,
			   struct srp_rdma_ch *ch,
			   struct srp_request *req)
{
	struct srp_target_port *target = ch->target;
	struct srp_device *dev = target->srp_host->srp_dev;
	struct ib_device *ibdev = dev->dev;
	int i, res;

	if (!scsi_sglist(scmnd) ||
	    (scmnd->sc_data_direction != DMA_TO_DEVICE &&
	     scmnd->sc_data_direction != DMA_FROM_DEVICE))
		return;

	if (dev->use_fast_reg) {
		struct srp_fr_desc **pfr;

		for (i = req->nmdesc, pfr = req->fr_list; i > 0; i--, pfr++) {
			res = srp_inv_rkey(ch, (*pfr)->mr->rkey);
			if (res < 0) {
				shost_printk(KERN_ERR, target->scsi_host, PFX
				  "Queueing INV WR for rkey %#x failed (%d)\n",
				  (*pfr)->mr->rkey, res);
				queue_work(system_long_wq,
					   &target->tl_err_work);
			}
		}
		if (req->nmdesc)
			srp_fr_pool_put(ch->fr_pool, req->fr_list,
					req->nmdesc);
	} else if (dev->use_fmr) {
		struct ib_pool_fmr **pfmr;

		for (i = req->nmdesc, pfmr = req->fmr_list; i > 0; i--, pfmr++)
			ib_fmr_pool_unmap(*pfmr);
	}

	ib_dma_unmap_sg(ibdev, scsi_sglist(scmnd), scsi_sg_count(scmnd),
			scmnd->sc_data_direction);
}

/**
 * srp_claim_req - Take ownership of the scmnd associated with a request.
 * @ch: SRP RDMA channel.
 * @req: SRP request.
 * @sdev: If not NULL, only take ownership for this SCSI device.
 * @scmnd: If NULL, take ownership of @req->scmnd. If not NULL, only take
 *         ownership of @req->scmnd if it equals @scmnd.
 *
 * Return value:
 * Either NULL or a pointer to the SCSI command the caller became owner of.
 */
static struct scsi_cmnd *srp_claim_req(struct srp_rdma_ch *ch,
				       struct srp_request *req,
				       struct scsi_device *sdev,
				       struct scsi_cmnd *scmnd)
{
	unsigned long flags;

	spin_lock_irqsave(&ch->lock, flags);
	if (req->scmnd &&
	    (!sdev || req->scmnd->device == sdev) &&
	    (!scmnd || req->scmnd == scmnd)) {
		scmnd = req->scmnd;
		req->scmnd = NULL;
	} else {
		scmnd = NULL;
	}
	spin_unlock_irqrestore(&ch->lock, flags);

	return scmnd;
}

/**
 * srp_free_req() - Unmap data and add request to the free request list.
 * @ch:     SRP RDMA channel.
 * @req:    Request to be freed.
 * @scmnd:  SCSI command associated with @req.
 * @req_lim_delta: Amount to be added to @target->req_lim.
 */
static void srp_free_req(struct srp_rdma_ch *ch, struct srp_request *req,
			 struct scsi_cmnd *scmnd, s32 req_lim_delta)
{
	unsigned long flags;

	srp_unmap_data(scmnd, ch, req);

	spin_lock_irqsave(&ch->lock, flags);
	ch->req_lim += req_lim_delta;
	spin_unlock_irqrestore(&ch->lock, flags);
}

static void srp_finish_req(struct srp_rdma_ch *ch, struct srp_request *req,
			   struct scsi_device *sdev, int result)
{
	struct scsi_cmnd *scmnd = srp_claim_req(ch, req, sdev, NULL);

	if (scmnd) {
		srp_free_req(ch, req, scmnd, 0);
		scmnd->result = result;
		scmnd->scsi_done(scmnd);
	}
}

static void srp_terminate_io(struct srp_rport *rport)
{
	struct srp_target_port *target = rport->lld_data;
	struct srp_rdma_ch *ch;
	struct Scsi_Host *shost = target->scsi_host;
	struct scsi_device *sdev;
	int i, j;

	/*
	 * Invoking srp_terminate_io() while srp_queuecommand() is running
	 * is not safe. Hence the warning statement below.
	 */
	shost_for_each_device(sdev, shost)
		WARN_ON_ONCE(sdev->request_queue->request_fn_active);

	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];

		for (j = 0; j < target->req_ring_size; ++j) {
			struct srp_request *req = &ch->req_ring[j];

			srp_finish_req(ch, req, NULL,
				       DID_TRANSPORT_FAILFAST << 16);
		}
	}
}

/*
 * It is up to the caller to ensure that srp_rport_reconnect() calls are
 * serialized and that no concurrent srp_queuecommand(), srp_abort(),
 * srp_reset_device() or srp_reset_host() calls will occur while this function
 * is in progress. One way to realize that is not to call this function
 * directly but to call srp_reconnect_rport() instead since that last function
 * serializes calls of this function via rport->mutex and also blocks
 * srp_queuecommand() calls before invoking this function.
 */
static int srp_rport_reconnect(struct srp_rport *rport)
{
	struct srp_target_port *target = rport->lld_data;
	struct srp_rdma_ch *ch;
	int i, j, ret = 0;
	bool multich = false;

	srp_disconnect_target(target);

	if (target->state == SRP_TARGET_SCANNING)
		return -ENODEV;

	/*
	 * Now get a new local CM ID so that we avoid confusing the target in
	 * case things are really fouled up. Doing so also ensures that all CM
	 * callbacks will have finished before a new QP is allocated.
	 */
	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];
		ret += srp_new_cm_id(ch);
	}
	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];
		for (j = 0; j < target->req_ring_size; ++j) {
			struct srp_request *req = &ch->req_ring[j];

			srp_finish_req(ch, req, NULL, DID_RESET << 16);
		}
	}
	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];
		/*
		 * Whether or not creating a new CM ID succeeded, create a new
		 * QP. This guarantees that all completion callback function
		 * invocations have finished before request resetting starts.
		 */
		ret += srp_create_ch_ib(ch);

		INIT_LIST_HEAD(&ch->free_tx);
		for (j = 0; j < target->queue_size; ++j)
			list_add(&ch->tx_ring[j]->list, &ch->free_tx);
	}

	target->qp_in_error = false;

	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];
		if (ret)
			break;
		ret = srp_connect_ch(ch, multich);
		multich = true;
	}

	if (ret == 0)
		shost_printk(KERN_INFO, target->scsi_host,
			     PFX "reconnect succeeded\n");

	return ret;
}

static void srp_map_desc(struct srp_map_state *state, dma_addr_t dma_addr,
			 unsigned int dma_len, u32 rkey)
{
	struct srp_direct_buf *desc = state->desc;

	WARN_ON_ONCE(!dma_len);

	desc->va = cpu_to_be64(dma_addr);
	desc->key = cpu_to_be32(rkey);
	desc->len = cpu_to_be32(dma_len);

	state->total_len += dma_len;
	state->desc++;
	state->ndesc++;
}

static int srp_map_finish_fmr(struct srp_map_state *state,
			      struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;
	struct srp_device *dev = target->srp_host->srp_dev;
	struct ib_pool_fmr *fmr;
	u64 io_addr = 0;

	if (state->fmr.next >= state->fmr.end)
		return -ENOMEM;

	WARN_ON_ONCE(!dev->use_fmr);

	if (state->npages == 0)
		return 0;

	if (state->npages == 1 && target->global_mr) {
		srp_map_desc(state, state->base_dma_addr, state->dma_len,
			     target->global_mr->rkey);
		goto reset_state;
	}

	fmr = ib_fmr_pool_map_phys(ch->fmr_pool, state->pages,
				   state->npages, io_addr);
	if (IS_ERR(fmr))
		return PTR_ERR(fmr);

	*state->fmr.next++ = fmr;
	state->nmdesc++;

	srp_map_desc(state, state->base_dma_addr & ~dev->mr_page_mask,
		     state->dma_len, fmr->fmr->rkey);

reset_state:
	state->npages = 0;
	state->dma_len = 0;

	return 0;
}

static int srp_map_finish_fr(struct srp_map_state *state,
			     struct srp_rdma_ch *ch, int sg_nents)
{
	struct srp_target_port *target = ch->target;
	struct srp_device *dev = target->srp_host->srp_dev;
	struct ib_send_wr *bad_wr;
	struct ib_reg_wr wr;
	struct srp_fr_desc *desc;
	u32 rkey;
	int n, err;

	if (state->fr.next >= state->fr.end)
		return -ENOMEM;

	WARN_ON_ONCE(!dev->use_fast_reg);

	if (sg_nents == 0)
		return 0;

	if (sg_nents == 1 && target->global_mr) {
		srp_map_desc(state, sg_dma_address(state->sg),
			     sg_dma_len(state->sg),
			     target->global_mr->rkey);
		return 1;
	}

	desc = srp_fr_pool_get(ch->fr_pool);
	if (!desc)
		return -ENOMEM;

	rkey = ib_inc_rkey(desc->mr->rkey);
	ib_update_fast_reg_key(desc->mr, rkey);

	n = ib_map_mr_sg(desc->mr, state->sg, 