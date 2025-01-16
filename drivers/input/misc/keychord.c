et = ch->target;
	struct srp_device *dev = target->srp_host->srp_dev;
	struct ib_device *ibdev = dev->dev;
	dma_addr_t dma_addr = ib_sg_dma_address(ibdev, sg);
	unsigned int dma_len = ib_sg_dma_len(ibdev, sg);
	unsigned int len = 0;
	int ret;

	WARN_ON_ONCE(!dma_len);

	while (dma_len) {
		unsigned offset = dma_addr & ~dev->mr_page_mask;
		if (state->npages == dev->max_pages_per_mr || offset != 0) {
			ret = srp_map_finish_fmr(state, ch);
			if (ret)
				return ret;
		}

		len = min_t(unsigned int, dma_len, dev->mr_page_size - offset);

		if (!state->npages)
			state->base_dma_addr = dma_addr;
		state->pages[state->npages++] = dma_addr & dev->mr_page_mask;
		state->dma_len += len;
		dma_addr += len;
		dma_len -= len;
	}

	/*
	 * If the last entry of the MR wasn't a full page, then we need to
	 * close it out and start a new one -- we can only merge at page
	 * boundries.
	 */
	ret = 0;
	if (len != dev->mr_page_size)
		ret = srp_map_finish_fmr(state, ch);
	return ret;
}

static int srp_map_sg_fmr(struct srp_map_state *state, struct srp_rdma_ch *ch,
			  struct srp_request *req, struct scatterlist *scat,
			  int count)
{
	struct scatterlist *sg;
	int i, ret;

	state->desc = req->indirect_desc;
	state->pages = req->map_page;
	state->fmr.next = req->fmr_list;
	state->fmr.end = req->fmr_list + ch->target->cmd_sg_cnt;

	for_each_sg(scat, sg, count, i) {
		ret = srp_map_sg_entry(state, ch, sg, i);
		if (ret)
			return ret;
	}

	ret = srp_map_finish_fmr(state, ch);
	if (ret)
		return ret;

	req->nmdesc = state->nmdesc;

	return 0;
}

static int srp_map_sg_fr(struct srp_map_state *state, struct srp_rdma_ch *ch,
			 struct srp_request *req, struct scatterlist *scat,
			 int count)
{
	state->desc = req->indirect_desc;
	state->fr.next = req->fr_list;
	state->fr.end = req->fr_list + ch->target->cmd_sg_cnt;
	state->sg = scat;

	while (count) {
		int i, n;

		n = srp_map_finish_fr(state, ch, count);
		if (unlikely(n < 0))
			return n;

		count -= n;
		for (i = 0; i < n; i++)
			state->sg = sg_next(state->sg);
	}

	req->nmdesc = state->nmdesc;

	return 0;
}

static int srp_map_sg_dma(struct srp_map_state *state, struct srp_rdma_ch *ch,
			  struct srp_request *req, struct scatterlist *scat,
			  int count)
{
	struct srp_target_port *target = ch->target;
	struct srp_device *dev = target->srp_host->srp_dev;
	struct scatterlist *sg;
	int i;

	state->desc = req->indirect_desc;
	for_each_sg(scat, sg, count, i) {
		srp_map_desc(state, ib_sg_dma_address(dev->dev, sg),
			     ib_sg_dma_len(dev->dev, sg),
			     target->global_mr->rkey);
	}

	req->nmdesc = state->nmdesc;

	return 0;
}

/*
 * Register the indirect data buffer descriptor with the HCA.
 *
 * Note: since the indirect data buffer descriptor has been allocated with
 * kmalloc() it is guaranteed that this buffer is a physically contiguous
 * memory buffer.
 */
static int srp_map_idb(struct srp_rdma_ch *ch, struct srp_request *req,
		       void **next_mr, void **end_mr, u32 idb_len,
		       __be32 *idb_rkey)
{
	struct srp_target_port *target = ch->target;
	struct srp_device *dev = target->srp_host->srp_dev;
	struct srp_map_state state;
	struct srp_direct_buf idb_desc;
	u64 idb_pages[1];
	struct scatterlist idb_sg[1];
	int ret;

	memset(&state, 0, sizeof(state));
	memset(&idb_desc, 0, sizeof(idb_desc));
	state.gen.next = next_mr;
	state.gen.end = end_mr;
	state.desc = &idb_desc;
	state.base_dma_addr = req->indirect_dma_addr;
	state.dma_len = idb_len;

	if (dev->use_fast_reg) {
		state.sg = idb_sg;
		sg_init_one(idb_sg, req->indirect_desc, idb_len);
		idb_sg->dma_address = req->indirect_dma_addr; /* hack! */
#ifdef CONFIG_NEED_SG_DMA_LENGTH
		idb_sg->dma_length = idb_sg->length;	      /* hack^2 */
#endif
		ret = srp_map_finish_fr(&state, ch, 1);
		if (ret < 0)
			return ret;
	} else if (dev->use_fmr) {
		state.pages = idb_pages;
		state.pages[0] = (req->indirect_dma_addr &
				  dev->mr_page_mask);
		state.npages = 1;
		ret = srp_map_finish_fmr(&state, ch);
		if (ret < 0)
			return ret;
	} else {
		return -EINVAL;
	}

	*idb_rkey = idb_desc.key;

	return 0;
}

static int srp_map_data(struct scsi_cmnd *scmnd, struct srp_rdma_ch *ch,
			struct srp_request *req)
{
	struct srp_target_port *target = ch->target;
	struct scatterlist *scat;
	struct srp_cmd *cmd = req->cmd->buf;
	int len, nents, count, ret;
	struct srp_device *dev;
	struct ib_device *ibdev;
	struct srp_map_state state;
	struct srp_indirect_buf *indirect_hdr;
	u32 idb_len, table_len;
	__be32 idb_rkey;
	u8 fmt;

	if (!scsi_sglist(scmnd) || scmnd->sc_data_direction == DMA_NONE)
		return sizeof (struct srp_cmd);

	if (scmnd->sc_data_direction != DMA_FROM_DEVICE &&
	    scmnd->sc_data_direction != DMA_TO_DEVICE) {
		shost_printk(KERN_WARNING, target->scsi_host,
			     PFX "Unhandled data direction %d\n",
			     scmnd->sc_data_direction);
		return -EINVAL;
	}

	nents = scsi_sg_count(scmnd);
	scat  = scsi_sglist(scmnd);

	dev = target->srp_host->srp_dev;
	ibdev = dev->dev;

	count = ib_dma_map_sg(ibdev, scat, nents, scmnd->sc_data_direction);
	if (unlikely(count == 0))
		return -EIO;

	fmt = SRP_DATA_DESC_DIRECT;
	len = sizeof (struct srp_cmd) +	sizeof (struct srp_direct_buf);

	if (count == 1 && target->global_mr) {
		/*
		 * The midlayer only generated a single gather/scatter
		 * entry, or DMA mapping coalesced everything to a
		 * single entry.  So a direct descriptor along with
		 * the DMA MR suffices.
		 */
		struct srp_direct_buf *buf = (void *) cmd->add_data;

		buf->va  = cpu_to_be64(ib_sg_dma_address(ibdev, scat));
		buf->key = cpu_to_be32(target->global_mr->rkey);
		buf->len = cpu_to_be32(ib_sg_dma_len(ibdev, scat));

		req->nmdesc = 0;
		goto map_complete;
	}

	/*
	 * We have more than one scatter/gather entry, so build our indirect
	 * descriptor table, trying to merge as many entries as we can.
	 */
	indirect_hdr = (void *) cmd->add_data;

	ib_dma_sync_single_for_cpu(ibdev, req->indirect_dma_addr,
				   target->indirect_size, DMA_TO_DEVICE);

	memset(&state, 0, sizeof(state));
	if (dev->use_fast_reg)
		srp_map_sg_fr(&state, ch, req, scat, count);
	else if (dev->use_fmr)
		srp_map_sg_fmr(&state, ch, req, scat, count);
	else
		srp_map_sg_dma(&state, ch, req, scat, count);

	/* We've mapped the request, now pull as much of the indirect
	 * descriptor table as we can into the command buffer. If this
	 * target is not using an external indirect table, we are
	 * guaranteed to fit into the command, as the SCSI layer won't
	 * give us more S/G entries than we allow.
	 */
	if (state.ndesc == 1) {
		/*
		 * Memory registration collapsed the sg-list into one entry,
		 * so use a direct descriptor.
		 */
		struct srp_direct_buf *buf = (void *) cmd->add_data;

		*buf = req->indirect_desc[0];
		goto map_complete;
	}

	if (unlikely(target->cmd_sg_cnt < state.ndesc &&
						!target->allow_ext_sg)) {
		shost_printk(KERN_ERR, target->scsi_host,
			     "Could not fit S/G list into SRP_CMD\n");
		return -EIO;
	}

	count = min(state.ndesc, target->cmd_sg_cnt);
	table_len = state.ndesc * sizeof (struct srp_direct_buf);
	idb_len = sizeof(struct srp_indirect_buf) + table_len;

	fmt = SRP_DATA_DESC_INDIRECT;
	len = sizeof(struct srp_cmd) + sizeof (struct srp_indirect_buf);
	len += count * sizeof (struct srp_direct_buf);

	memcpy(indirect_hdr->desc_list, req->indirect_desc,
	       count * sizeof (struct srp_direct_buf));

	if (!target->global_mr) {
		ret = srp_map_idb(ch, req, state.gen.next, state.gen.end,
				  idb_len, &idb_rkey);
		if (ret < 0)
			return ret;
		req->nmdesc++;
	} else {
		idb_rkey = cpu_to_be32(target->global_mr->rkey);
	}

	indirect_hdr->table_desc.va = cpu_to_be64(req->indirect_dma_addr);
	indirect_hdr->table_desc.key = idb_rkey;
	indirect_hdr->table_desc.len = cpu_to_be32(table_len);
	indirect_hdr->len = cpu_to_be32(state.total_len);

	if (scmnd->sc_data_direction == DMA_TO_DEVICE)
		cmd->data_out_desc_cnt = count;
	else
		cmd->data_in_desc_cnt = count;

	ib_dma_sync_single_for_device(ibdev, req->indirect_dma_addr, table_len,
				      DMA_TO_DEVICE);

map_complete:
	if (scmnd->sc_data_direction == DMA_TO_DEVICE)
		cmd->buf_fmt = fmt << 4;
	else
		cmd->buf_fmt = fmt;

	return len;
}

/*
 * Return an IU and possible credit to the free pool
 */
static void srp_put_tx_iu(struct srp_rdma_ch *ch, struct srp_iu *iu,
			  enum srp_iu_type iu_type)
{
	unsigned long flags;

	spin_lock_irqsave(&ch->lock, flags);
	list_add(&iu->list, &ch->free_tx);
	if (iu_type != SRP_IU_RSP)
		++ch->req_lim;
	spin_unlock_irqrestore(&ch->lock, flags);
}

/*
 * Must be called with ch->lock held to protect req_lim and free_tx.
 * If IU is not sent, it must be returned using srp_put_tx_iu().
 *
 * Note:
 * An upper limit for the number of allocated information units for each
 * request type is:
 * - SRP_IU_CMD: SRP_CMD_SQ_SIZE, since the SCSI mid-layer never queues
 *   more than Scsi_Host.can_queue requests.
 * - SRP_IU_TSK_MGMT: SRP_TSK_MGMT_SQ_SIZE.
 * - SRP_IU_RSP: 1, since a conforming SRP target never sends more than
 *   one unanswered SRP request to an initiator.
 */
static struct srp_iu *__srp_get_tx_iu(struct srp_rdma_ch *ch,
				      enum srp_iu_type iu_type)
{
	struct srp_target_port *target = ch->target;
	s32 rsv = (iu_type == SRP_IU_TSK_MGMT) ? 0 : SRP_TSK_MGMT_SQ_SIZE;
	struct srp_iu *iu;

	srp_send_completion(ch->send_cq, ch);

	if (list_empty(&ch->free_tx))
		return NULL;

	/* Initiator responses to target requests do not consume credits */
	if (iu_type != SRP_IU_RSP) {
		if (ch->req_lim <= rsv) {
			++target->zero_req_lim;
			return NULL;
		}

		--ch->req_lim;
	}

	iu = list_first_entry(&ch->free_tx, struct srp_iu, list);
	list_del(&iu->list);
	return iu;
}

static int srp_post_send(struct srp_rdma_ch *ch, struct srp_iu *iu, int len)
{
	struct srp_target_port *target = ch->target;
	struct ib_sge list;
	struct ib_send_wr wr, *bad_wr;

	list.addr   = iu->dma;
	list.length = len;
	list.lkey   = target->lkey;

	wr.next       = NULL;
	wr.wr_id      = (uintptr_t) iu;
	wr.sg_list    = &list;
	wr.num_sge    = 1;
	wr.opcode     = IB_WR_SEND;
	wr.send_flags = IB_SEND_SIGNALED;

	return ib_post_send(ch->qp, &wr, &bad_wr);
}

static int srp_post_recv(struct srp_rdma_ch *ch, struct srp_iu *iu)
{
	struct srp_target_port *target = ch->target;
	struct ib_recv_wr wr, *bad_wr;
	struct ib_sge list;

	list.addr   = iu->dma;
	list.length = iu->size;
	list.lkey   = target->lkey;

	wr.next     = NULL;
	wr.wr_id    = (uintptr_t) iu;
	wr.sg_list  = &list;
	wr.num_sge  = 1;

	return ib_post_recv(ch->qp, &wr, &bad_wr);
}

static void srp_process_rsp(struct srp_rdma_ch *ch, struct srp_rsp *rsp)
{
	struct srp_target_port *target = ch->target;
	struct srp_request *req;
	struct scsi_cmnd *scmnd;
	unsigned long flags;

	if (unlikely(rsp->tag & SRP_TAG_TSK_MGMT)) {
		spin_lock_irqsave(&ch->lock, flags);
		ch->req_lim += be32_to_cpu(rsp->req_lim_delta);
		if (rsp->tag == ch->tsk_mgmt_tag) {
			ch->tsk_mgmt_status = -1;
			if (be32_to_cpu(rsp->resp_data_len) >= 4)
				ch->tsk_mgmt_status = rsp->data[3];
			complete(&ch->tsk_mgmt_done);
		} else {
			shost_printk(KERN_ERR, target->scsi_host,
				     "Received tsk mgmt response too late for tag %#llx\n",
				     rsp->tag);
		}
		spin_unlock_irqrestore(&ch->lock, flags);
	} else {
		scmnd = scsi_host_find_tag(target->scsi_host, rsp->tag);
		if (scmnd && scmnd->host_scribble) {
			req = (void *)scmnd->host_scribble;
			scmnd = srp_claim_req(ch, req, NULL, scmnd);
		} else {
			scmnd = NULL;
		}
		if (!scmnd) {
			shost_printk(KERN_ERR, target->scsi_host,
				     "Null scmnd for RSP w/tag %#016llx received on ch %td / QP %#x\n",
				     rsp->tag, ch - target->ch, ch->qp->qp_num);

			spin_lock_irqsave(&ch->l