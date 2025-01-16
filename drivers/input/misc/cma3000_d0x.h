etting device queue depth
 * @sdev: scsi device struct
 * @qdepth: requested queue depth
 *
 * Returns queue depth.
 */
static int
srp_change_queue_depth(struct scsi_device *sdev, int qdepth)
{
	if (!sdev->tagged_supported)
		qdepth = 1;
	return scsi_change_queue_depth(sdev, qdepth);
}

static int srp_send_tsk_mgmt(struct srp_rdma_ch *ch, u64 req_tag, u64 lun,
			     u8 func, u8 *status)
{
	struct srp_target_port *target = ch->target;
	struct srp_rport *rport = target->rport;
	struct ib_device *dev = target->srp_host->srp_dev->dev;
	struct srp_iu *iu;
	struct srp_tsk_mgmt *tsk_mgmt;
	int res;

	if (!ch->connected || target->qp_in_error)
		return -1;

	/*
	 * Lock the rport mutex to avoid that srp_create_ch_ib() is
	 * invoked while a task management function is being sent.
	 */
	mutex_lock(&rport->mutex);
	spin_lock_irq(&ch->lock);
	iu = __srp_get_tx_iu(ch, SRP_IU_TSK_MGMT);
	spin_unlock_irq(&ch->lock);

	if (!iu) {
		mutex_unlock(&rport->mutex);

		return -1;
	}

	ib_dma_sync_single_for_cpu(dev, iu->dma, sizeof *tsk_mgmt,
				   DMA_TO_DEVICE);
	tsk_mgmt = iu->buf;
	memset(tsk_mgmt, 0, sizeof *tsk_mgmt);

	tsk_mgmt->opcode 	= SRP_TSK_MGMT;
	int_to_scsilun(lun, &tsk_mgmt->lun);
	tsk_mgmt->tsk_mgmt_func = func;
	tsk_mgmt->task_tag	= req_tag;

	spin_l