address(sg));

	ipu_select_buffer(chan_id, buf_idx);
	dev_dbg(dev, "Updated sg %p on channel 0x%x buffer %d\n",
		sg, chan_id, buf_idx);

	return 0;
}

/* Called under spin_lock_irqsave(&ichan->lock) */
static int ipu_submit_channel_buffers(struct idmac_channel *ichan,
				      struct idmac_tx_desc *desc)
{
	struct scatterlist *sg;
	int i, ret = 0;

	for (i = 0, sg = desc->sg; i < 2 && sg; i++) {
		if (!ichan->sg[i]) {
			ichan->sg[i] = sg;

			ret = ipu_submit_buffer(ichan, desc, sg, i);
			if (ret < 0)
				return ret;

			sg = sg_next(sg);
		}
	}

	return ret;
}

static dma_cookie_t idmac_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct idmac_tx_desc *desc = to_tx_desc(tx);
	struct idmac_channel *ichan = to_idmac_chan(tx->chan);
	struct idmac *idmac = to_idmac(tx->chan->device);
	struct ipu *ipu = to_ipu(idmac);
	struct device *dev = &ichan->dma_chan.dev->device;
	dma_cookie_t cookie;
	unsigned long flags;
	int ret;

	/* Sanity check */
	if (!list_empty(&desc->list)) {
		/* The descriptor doesn't belong to client */
		dev_err(dev, "Descriptor %p not prepared!\n", tx);
		return -EBUSY;
	}

	mutex_lock(&ichan->chan_mutex);

	async_tx_clear_ack(tx);

	if (ichan->status < IPU_CHANNEL_READY) {
		struct idmac_video_param *video = &ichan->params.video;
		/*
		 * Initial buffer assignment - the first two sg-entries from
		 * the descriptor will end up in the IDMAC buffers
		 */
		dma_addr_t dma_1 = sg_is_last(desc->sg) ? 0 :
			sg_dma_address(&desc->sg[1]);

		WARN_ON(ichan->sg[0] || ichan->sg[1]);

		cookie = ipu_init_channel_buffer(ichan,
						 video->out_pixel_fmt,
						 video->out_width,
						 video->out_height,
						 video->out_stride,
						 IPU_ROTATE_NONE,
						 sg_dma_address(&desc->sg[0]),
						 dma_1);
		if (cookie < 0)
			goto out;
	}

	dev_dbg(dev, "Submitting sg %p\n", &desc->sg[0]);

	cookie = dma_cookie_assign(tx);

	/* ipu->lock can be taken under ichan->lock, but not v.v. */
	spin_lock_irqsave(&ichan->lock, flags);

	list_add_tail(&desc->list, &ichan->queue);
	/* submit_buffers() atomically verifies and fills empty sg slots */
	ret = ipu_submit_channel_buffers(ichan, desc);

	spin_unlock_irqrestore(&ichan->lock, flags);

	if (ret < 0) {
		cookie = ret;
		goto dequeue;
	}

	if (ichan->status < IPU_CHANNEL_ENABLED) {
		ret = ipu_enable_channel(idmac, ichan);
		if (ret < 0) {
			cookie = ret;
			goto dequeue;
		}
	}

	dump_idmac_reg(ipu);

dequeue:
	if (cookie < 0) {
		spin_lock_irqsave(&ichan->lock, flags);
		list_del_init(&desc->list);
		spin_unlock_irqrestore(&ichan->lock, flags);
		tx->cookie = cookie;
		ichan->dma_chan.cookie = cookie;
	}

out:
	mutex_unlock(&ichan->chan_mutex);

	return cookie;
}

/* Called with ichan->chan_mutex held */
static int idmac_desc_alloc(struct idmac_channel *ichan, int n)
{
	struct idmac_tx_desc *desc = vmalloc(n * sizeof(struct idmac_tx_desc));
	struct idmac *idmac = to_idmac(ichan->dma_chan.device);

	if (!desc)
		return -ENOMEM;

	/* No interrupts, just disable the tasklet for a moment */
	tasklet_disable(&to_ipu(idmac)->tasklet);

	ichan->n_tx_desc = n;
	ichan->desc = desc;
	INIT_LIST_HEAD(&ichan->queue);
	INIT_LIST_HEAD(&ichan->free_list);

	while (n--) {
		struct dma_async_tx_descriptor *txd = &desc->txd;

		memset(txd, 0, sizeof(*txd));
		dma_async_tx_descriptor_init(txd, &ichan->dma_chan);
		txd->tx_submit		= idmac_tx_submit;

		list_add(&desc->list, &ichan->free_list);

		desc++;
	}

	tasklet_enable(&to_ipu(idmac)->tasklet);

	return 0;
}

/**
 * ipu_init_channel() - initialize an IPU channel.
 * @idmac:	IPU DMAC context.
 * @ichan:	pointer to the channel object.
 * @return      0 on success or negative error code on failure.
 */
static int ipu_init_channel(struct idmac *idmac, struct idmac_channel *ichan)
{
	union ipu_channel_param *params = &ichan->params;
	uint32_t ipu_conf;
	enum ipu_channel channel = ichan->dma_chan.chan_id;
	unsigned long flags;
	uint32_t reg;
	struct ipu *ipu = to_ipu(idmac);
	int ret = 0, n_desc = 0;

	dev_dbg(ipu->dev, "init channel = %d\n", channel);

	if (channel != IDMAC_SDC_0 && channel != IDMAC_SDC_1 &&
	    channel != IDMAC_IC_7)
		return -EINVAL;

	spin_lock_irqsave(&ipu->lock, flags);

	switch (channel) {
	case IDMAC_IC_7:
		n_desc = 16;
		reg = idmac_read_icreg(ipu, IC_CONF);
		idmac_write_icreg(ipu, reg & ~IC_CONF_CSI_MEM_WR_EN, IC_CONF);
		break;
	case IDMAC_IC_0:
		n_desc = 16;
		reg = idmac_read_ipureg(ipu, IPU_FS_PROC_FLOW);
		idmac_write_ipureg(ipu, reg & ~FS_ENC_IN_VALID, IPU_FS_PROC_FLOW);
		ret = ipu_ic_init_prpenc(ipu, params, true);
		break;
	case IDMAC_SDC_0:
	case IDMAC_SDC_1:
		n_desc = 4;
	default:
		break;
	}

	ipu->channel_init_mask |= 1L << channel;

	/* Enable IPU sub module */
	ipu_conf = idmac_read_ipureg(ipu, IPU_CONF) |
		ipu_channel_conf_mask(channel);
	idmac_write_ipureg(ipu, ipu_conf, IPU_CONF);

	spin_unlock_irqrestore(&ipu->lock, flags);

	if (n_desc && !ichan->desc)
		ret = idmac_desc_alloc(ichan, n_desc);

	dump_idmac_reg(ipu);

	return ret;
}

/**
 * ipu_uninit_channel() - uninitialize an IPU channel.
 * @idmac:	IPU DMAC context.
 * @ichan:	pointer to the channel object.
 */
static void ipu_uninit_channel(struct idmac *idmac, struct idmac_channel *ichan)
{
	enum ipu_channel channel = ichan->dma_chan.chan_id;
	unsigned long flags;
	uint32_t reg;
	unsigned long chan_mask = 1UL << channel;
	uint32_t ipu_conf;
	struct ipu *ipu = to_ipu(idmac);

	spin_lock_irqsave(&ipu->lock, flags);

	if (!(ipu->channel_init_mask & chan_mask)) {
		dev_err(ipu->dev, "Channel already uninitialized %d\n",
			channel);
		spin_unlock_irqrestore(&ipu->lock, flags);
		return;
	}

	/* Reset the double buffer */
	reg = idmac_read_ipureg(ipu, IPU_CHA_DB_MODE_SEL);
	idmac_write_ipureg(ipu, reg & ~chan_mask, IPU_CHA_DB_MODE_SEL);

	ichan->sec_chan_en = false;

	switch (channel) {
	case IDMAC_IC_7:
		reg = idmac_read_icreg(ipu, IC_CONF);
		idmac_write_icreg(ipu, reg & ~(IC_CONF_RWS_EN | IC_CONF_PRPENC_EN),
			     IC_CONF);
		break;
	case IDMAC_IC_0:
		reg = idmac_read_icreg(ipu, IC_CONF);
		idmac_write_icreg(ipu, reg & ~(IC_CONF_PRPENC_EN | IC_CONF_PRPENC_CSC1),
				  IC_CONF);
		break;
	case IDMAC_SDC_0:
	case IDMAC_SDC_1:
	default:
		break;
	}

	ipu->channel_init_mask &= ~(1L << channel);

	ipu_conf = idmac_read_ipureg(ipu, IPU_CONF) &
		~ipu_channel_conf_mask(channel);
	idmac_write_ipureg(ipu, ipu_conf, IPU_CONF);

	spin_unlock_irqrestore(&ipu->lock, flags);

	ichan->n_tx_desc = 0;
	vfree(ichan->desc);
	ichan->desc = NULL;
}

/**
 * ipu_disable_channel() - disable an IPU channel.
 * @idmac:		IPU DMAC context.
 * @ichan:		channel object pointer.
 * @wait_for_stop:	flag to set whether to wait for channel end of frame or
 *			return immediately.
 * @return:		0 on success or negative error code on failure.
 */
static int ipu_disable_channel(struct idmac *idmac, struct idmac_channel *ichan,
			       bool wait_for_stop)
{
	enum ipu_channel channel = ichan->dma_chan.chan_id;
	struct ipu *ipu = to_ipu(idmac);
	uint32_t reg;
	unsigned long flags;
	unsigned long chan_mask = 1UL << channel;
	unsigned int timeout;

	if (wait_for_stop && channel != IDMAC_SDC_1 && channel != IDMAC_SDC_0) {
		timeout = 40;
		/* This waiting always fails. Related to spurious irq problem */
		while ((idmac_read_icreg(ipu, IDMAC_CHA_BUSY) & chan_mask) ||
		       (ipu_channel_status(ipu, channel) == TASK_STAT_ACTIVE)) {
			timeout--;
			msleep(10);

			if (!timeout) {
				dev_dbg(ipu->dev,
					"Warning: timeout waiting for channel %u to "
					"stop: buf0_rdy = 0x%08X, buf1_rdy = 0x%08X, "
					"busy = 0x%08X, tstat = 0x%08X\n", channel,
					idmac_read_ipureg(ipu, IPU_CHA_BUF0_RDY),
					idmac_read_ipureg(ipu, IPU_CHA_BUF1_RDY),
					idmac_read_icreg(ipu, IDMAC_CHA_BUSY),
					idmac_read_ipureg(ipu, IPU_TASKS_STAT));
		