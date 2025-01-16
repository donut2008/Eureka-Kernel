P_FLAG_DOUNDER))
			scsi_set_resid(scmnd, be32_to_cpu(rsp->data_out_res_cnt));
		else if (unlikely(rsp->flags & SRP_RSP_FLAG_DOOVER))
			scsi_set_resid(scmnd, -be32_to_cpu(rsp->data_out_res_cnt));

		srp_free_req(ch, req, scmnd,
			     be32_to_cpu(rsp->req_lim_delta));

		scmnd->host_scribble = NULL;
		scmnd->scsi_done(scmnd);
	}
}

static int srp_response_common(struct srp_rdma_ch *ch, s32 req_delta,
			       void *rsp, int len)
{
	struct srp_target_port *target = ch->target;
	struct ib_device *dev = target->srp_host->srp_dev->dev;
	unsigned long flags;
	struct srp_iu *iu;
	int err;

	spin_lock_irqsave(&ch->lock, flags);
	ch->req_lim += req_delta;
	iu = __srp_get_tx_iu(ch, SRP_IU_RSP);
	spin_unlock_irqrestore(&ch->lock, flags);

	if (!iu) {
		shost_printk(KERN_ERR, target->scsi_host, PFX
			     "no IU available to send response\n");
		return 1;
	}

	ib_dma_sync_single_for_cpu(dev, iu->dma, len, DMA_TO_DEVICE);
	memcpy(iu->buf, rsp, len);
	ib_dma_sync_single_for_device(dev, iu->dma, len, DMA_TO_DEVICE);

	err = srp_post_send(ch, iu, len);
	if (err) {
		shost_printk(KERN_ERR, target->scsi_host, PFX
			     "unable to post response: %d\n", err);
		srp_put_tx_iu(ch, iu, SRP_IU_RSP);
	}

	return err;
}

static void srp_process_cred_req(struct srp_rdma_ch *ch,
				 struct srp_cred_req *req)
{
	struct srp_cred_rsp rsp = {
		.opcode = SRP_CRED_RSP,
		.tag = req->tag,
	};
	s32 delta = be32_to_cpu(req->req_lim_delta);

	if (srp_response_common(ch, delta, &rsp, sizeof(rsp)))
		shost_printk(KERN_ERR, ch->target->scsi_host, PFX
			     "problems processing SRP_CRED_REQ\n");
}

static void srp_process_aer_req(struct srp_rdma_ch *ch,
				struct srp_aer_req *req)
{
	struct srp_target_port *target = ch->target;
	struct srp_aer_rsp rsp = {
		.opcode = SRP_AER_RSP,
		.tag = req->tag,
	};
	s32 delta = be32_to_cpu(req->req_lim_delta);

	shost_printk(KERN_ERR, target->scsi_host, PFX
		     "ignoring AER for LUN %llu\n", scsilun_to_int(&req->lun));

	if (srp_response_common(ch, delta, &rsp, sizeof(rsp)))
		shost_printk(KERN_ERR, target->scsi_host, PFX
			     "problems processing SRP_AER_REQ\n");
}

static void srp_handle_recv(struct srp_rdma_ch *ch, struct ib_wc *wc)
{
	struct srp_target_port *target = ch->target;
	struct ib_device *dev = target->srp_host->srp_dev->dev;
	struct srp_iu *iu = (struct srp_iu *) (uintptr_t) wc->wr_id;
	int res;
	u8 opcode;

	ib_dma_sync_single_for_cpu(dev, iu->dma, ch->max_ti_iu_len,
				   DMA_FROM_DEVICE);

	opcode = *(u8 *) iu->buf;

	if (0) {
		shost_printk(KERN_ERR, target->scsi_host,
			     PFX "recv completion, opcode 0x%02x\n", opcode);
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 8, 1,
			       iu->buf, wc->byte_len, true);
	}

	switch (opcode) {
	case SRP_RSP:
		srp_process_rsp(ch, iu->buf);
		break;

	case SRP_CRED_REQ:
		srp_process_cred_req(ch, iu->buf);
		break;

	case SRP_AER_REQ:
		srp_process_aer_req(ch, iu->buf);
		break;

	case SRP_T_LOGOUT:
		/* XXX Handle target logout */
		shost_printk(KERN_WARNING, target->scsi_host,
			     PFX "Got target logout request\n");
		break;

	default:
		shost_printk(KERN_WARNING, target->scsi_host,
			     PFX "Unhandled SRP opcode 0x%02x\n", opcode);
		break;
	}

	ib_dma_sync_single_for_device(dev, iu->dma, ch->max_ti_iu_len,
				      DMA_FROM_DEVICE);

	res = srp_post_recv(ch, iu);
	if (res != 0)
		shost_printk(KERN_ERR, target->scsi_host,
			     PFX "Recv failed with error code %d\n", res);
}

/**
 * srp_tl_err_work() - handle a transport layer error
 * @work: Work structure embedded in an SRP target port.
 *
 * Note: This function may get invoked before the rport has been created,
 * hence the target->rport test.
 */
static void srp_tl_err_work(struct work_struct *work)
{
	struct srp_target_port *target;

	target = container_of(work, struct srp_target_port, tl_err_work);
	if (target->rport)
		srp_start_tl_fail_timers(target->rport);
}

static void srp_handle_qp_err(u64 wr_id, enum ib_wc_status wc_status,
			      bool send_err, struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;

	if (wr_id == SRP_LAST_WR_ID) {
		complete(&ch->done);
		return;
	}

	if (ch->connected && !target->qp_in_error) {
		if (wr_id & LOCAL_INV_WR_ID_MASK) {
			shost_printk(KERN_ERR, target->scsi_host, PFX
				     "LOCAL_INV failed with