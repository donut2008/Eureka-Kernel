e,
				   enum ipu_rotate_mode rot_mode,
				   dma_addr_t phyaddr_0, dma_addr_t phyaddr_1)
{
	enum ipu_channel channel = ichan->dma_chan.chan_id;
	struct idmac *idmac = to_idmac(ichan->dma_chan.device);
	struct ipu *ipu = to_ipu(idmac);
	union chan_param_mem params = {};
	unsigned long flags;
	uint32_t reg;
	uint32_t stride_bytes;

	stride_bytes = stride * bytes_per_pixel(pixel_fmt);

	if (stride_bytes % 4) {
		dev_err(ipu->dev,
			"Stride length must be 32-bit aligned, stride = %d, bytes = %d\n",
			stride, stride_bytes);
		return -EINVAL;
	}

	/* IC channel's stride must be a multiple of 8 pixels */
	if ((channel <= IDMAC_IC_13) && (stride % 8)) {
		dev_err(ipu->dev, "Stride must be 8 pixel multiple\n");
		return -EINVAL;
	}

	/* Build parameter memory data for DMA channel */
	ipu_ch_param_set_size(&params, pixel_fmt, width, height, stride_bytes);
	ipu_ch_param_set_buffer(&params, phyaddr_0, phyaddr_1);
	ipu_ch_param_set_rotation(&params, rot_mode);

	spin_lock_irqsave(&ipu->lock, flags);

	ipu_write_param_mem(dma_param_addr(channel), (uint32_t *)&params, 10);

	reg = idmac_read_ipureg(ipu, IPU_CHA_DB_MODE_SEL);

	if (phyaddr_1)
		reg |= 1UL << channel;
	else
		reg &= ~(1UL << channel);

	idmac_write_ipureg(ipu, reg, IPU_CHA_DB_MODE_SEL);

	ichan->status = IPU_CHANNEL_READY;

	spin_unlock_irqrestore(&ipu->lock, flags);

	return 0;
}

/**
 * ipu_select_buffer() - mark a channel's buffer as ready.
 * @channel:	channel ID.
 * @buffer_n:	buffer number to mark ready.
 */
static void ipu_select_buffer(enum ipu_channel channel, int buffer_n)
{
	/* No locking - this is a write-one-to-set register, cleared by IPU */
	if (buffer_n == 0)
		/* Mark buffer 0 as ready. */
		idmac_write_ipureg(&ipu_data, 1UL << channel, IPU_CHA_BUF0_RDY);
	else
		/* Mark buffer 1 as ready. */
		idmac_write_ipureg(&ipu_data, 1UL << channel, IPU_CHA_BUF1_RDY);
}

/**
 * ipu_update_channel_buffer() - update physical address of a channel buffer.
 * @ichan:	IDMAC channel.
 * @buffer_n:	buffer number to update.
 *		0 or 1 are the only valid values.
 * @phyaddr:	buffer physical address.
 */
/* Called under spin_lock(_irqsave)(&ichan->lock) */
static void ipu_update_channel_buffer(struct idmac_channel *ichan,
				      int buffer_n, dma_addr_t phyaddr)
{
	enum ipu_channel channel = ichan->dma_chan.chan_id;
	uint32_t reg;
	unsigned long flags;

	spin_lock_irqsave(&ipu_data.lock, flags);

	if (buffer_n == 0) {
		reg = idmac_read_ipureg(&ipu_data, IPU_CHA_BUF0_RDY);
		if (reg & (1UL << channel)) {
			ipu_ic_disable_task(&ipu_data, channel);
			ichan->status = IPU_CHANNEL_READY;
		}

		/* 44.3.3.1.9 - Row Number 1 (WORD1, offset 0) */
		idmac_write_ipureg(&ipu_data, dma_param_addr(channel) +
				   0x0008UL, IPU_IMA_ADDR);
		idmac_write_ipureg(&ipu_data, phyaddr, IPU_IMA_DATA);
	} else {
		reg = idmac_read_ipureg(&ipu_data, IPU_CHA_BUF1_RDY);
		if (reg & (1UL << channel)) {
			ipu_ic_disable_task(&ipu_data, channel);
			ichan->status = IPU_CHANNEL_READY;
		}

		/* Check if double-buffering is already enabled */
		reg = idmac_read_ipureg(&ipu_data, IPU_CHA_DB_MODE_SEL);

		if (!(reg & (1UL << channel)))
			idmac_write_ipureg(&ipu_data, reg | (1UL << channel),
					   IPU_CHA_DB_MODE_SEL);

		/* 44.3.3.1.9 - Row Number 1 (WORD1, offset 1) */
		idmac_write_ipureg(&ipu_data, dma_param_addr(channel) +
				   0x0009UL, IPU_IMA_ADDR);
		idmac_write_ipureg(&ipu_data, phyaddr, IPU_IMA_DATA);
	}

	spin_unlock_irqrestore(&ipu_data.lock, flags);
}

/* Called under spin_lock_irqsave(&ichan->lock) */
static int ipu_submit_buffer(struct idmac_channel *ichan,
	struct idmac_tx_desc *desc, struct scatterlist *sg, int buf_idx)
{
	unsigned int chan_id = ichan->dma_chan.chan_id;
	struct device *dev = &ichan->dma_chan.dev->device;

	if (async_tx_test_ack(&desc->txd))
		return -EINTR;

	/*
	 * On first invocation this shouldn't be necessary, the call to
	 * ipu_init_channel_buffer() above wi