
	unsigned long flags;

	BUG_ON(!ch);

	ioctx = NULL;
	spin_lock_irqsave(&ch->spinlock, flags);
	if (!list_empty(&ch->free_list)) {
		ioctx = list_first_entry(&ch->free_list,
					 struct srpt_send_ioctx, free_list);
		list_del(&ioctx->free_list);
	}
	spin_unlock_irqrestore(&ch->spinlock, flags);

	if (!ioctx)
		return ioctx;

	BUG_ON(ioctx->ch != ch);
	spin_lock_init(&ioctx->spinlock);
	ioctx->state = SRPT_STATE_NEW;
	ioctx->n_rbuf = 0;
	ioctx->rbufs = NULL;
	ioctx->n_rdma = 0;
	ioctx->n_rdma_ius = 0;
	ioctx->rdma_ius = NULL;
	ioctx->mapped_sg_count = 0;
	init_completion(&ioctx->tx_done);
	ioctx->queue_status_only = false;
	/*
	 * transport_init_se_cmd() does not initialize all fields, so do it
	 * here.
	 */
	memset(&ioctx->cmd, 0, sizeof(ioctx->cmd));
	memset(&ioctx->sense_data, 0, sizeof(ioctx->sense_data));

	return ioctx;
}

/**
 * srpt_abort_cmd() - Abort a SCSI command.
 * @ioctx:   I/O context associated with the SCSI command.
 * @context: Preferred execution context.
 */
static int srpt_abort_cmd(struct srpt_send_ioctx *ioctx)
{
	enum srpt_command_state state;
	unsigned long flags;

	BUG_ON(!ioctx);

	/*
	 * If the command is in a state where the target core is waiting for
	 * the ib_srpt driv