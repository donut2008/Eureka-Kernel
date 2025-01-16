han_id;

	/* freeze the channel */
	if (dma_is_apbh(mxs_dma) && apbh_is_old(mxs_dma))
		writel(1 << chan_id,
			mxs_dma->base + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);
	else
		writel(1 << chan_id,
			mxs_dma->base + HW_APBHX_CHANNEL_CTRL + STMP_OFFSET_REG_SET);

	mxs_chan->status = DMA_PAUSED;
	return 0;
}

static int mxs_dma_resume_chan(struct dma_chan *chan)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	int chan_id = mxs_chan->chan.chan_id;

	/* unfreeze the channel */
	if (dma_is_apbh(mxs_dma) && apbh_is_old(mxs_dma))
		writel(1 << chan_id,
			mxs_dma->base + HW_APBHX_CTRL0 + STMP_OFFSET_REG_CLR);
	else
		writel(1 << chan_id,
			mxs_dma->base + HW_APBHX_CHANNEL_CTRL + STMP_OFFSET_REG_CLR);

	mxs_chan->status = DMA_IN_PROGRESS;
	return 0;
}

static dma_cookie_t mxs_dma_tx_submit(struct dma_async_tx_descriptor *tx)
{
	return dma_cookie_assign(tx);
}

static void mxs_dma_tasklet(unsigned long data)
{
	struct mxs_dma_chan *mxs_chan = (struct mxs_dma_chan *) data;

	if (mxs_chan->desc.callback)
		mxs_chan->desc.callback(mxs_chan->desc.callback_param);
}

static int mxs_dma_irq_to_chan(struct mxs_dma_engine *mxs_dma, int irq)
{
	int i;

	for (i = 0; i != mxs_dma->nr_channels; ++i)
		if (mxs_dma->mxs_chans[i].chan_irq == irq)
			return i;

	return -EINVAL;
}

static irqreturn_t mxs_dma_int_handler(int irq, void *dev_id)
{
	struct mxs_dma_engine *mxs_dma = dev_id;
	struct mxs_dma_chan *mxs_chan;
	u32 completed;
	u32 err;
	int chan = mxs_dma_irq_to_chan(mxs_dma, irq);

	if (chan < 0)
		return IRQ_NONE;

	/* completion status */
	completed = readl(mxs_dma->base + HW_APBHX_CTRL1);
	completed = (completed >> chan) & 0x1;

	/* Clear interrupt */
	writel((1 << chan),
			mxs_dma->base + HW_APBHX_CTRL1 + STMP_OFFSET_REG_CLR);

	/* error status */
	err = readl(mxs_dma->base + HW_APBHX_CTRL2);
	err &= (1 << (MXS_DMA_CHANNELS + chan)) | (1 << chan);

	/*
	 * error status bit is in the upper 16 bits, error irq bit in the lower
	 * 16 bits. We transform it into a simpler error code:
	 * err: 0x00 = no error, 0x01 = TERMINATION, 0x02 = BUS_ERROR
	 */
	err = (err >> (MXS_DMA_CHANNELS + chan)) + (err >> chan);

	/* Clear error irq */
	writel((1 << chan),
			mxs_dma->base + HW_APBHX_CTRL2 + STMP_OFFSET_REG_CLR);

	/*
	 * When both completion and error of termination bits set at the
	 * same time, we do not take it as an error.  IOW, it only becomes
	 * an error we need to handle here in case of either it's a bus
	 * error or a termination error with no completion. 0x01 is termination
	 * error, so we can subtract err & completed to get the real error case.
	 */
	err -= err & completed;

	mxs_chan = &mxs_dma->mxs_chans[chan];

	if (err) {
		dev_dbg(mxs_dma->dma_device.dev,
			"%s: error in channel %d\n", __func__,
			chan);
		mxs_chan->status = DMA_ERROR;
		mxs_dma_reset_chan(&mxs_chan->chan);
	} else if (mxs_chan->status != DMA_COMPLETE) {
		if (mxs_chan->flags & MXS_DMA_SG_LOOP) {
			mxs_chan->status = DMA_IN_PROGRESS;
			if (mxs_chan->flags & MXS_DMA_USE_SEMAPHORE)
				writel(1, mxs_dma->base +
					HW_APBHX_CHn_SEMA(mxs_dma, chan));
		} else {
			mxs_chan->status = DMA_COMPLETE;
		}
	}

	if (mxs_chan->status == DMA_COMPLETE) {
		if (mxs_chan->reset)
			return IRQ_HANDLED;
		dma_cookie_complete(&mxs_chan->desc);
	}

	/* schedule tasklet on this channel */
	tasklet_schedule(&mxs_chan->tasklet);

	return IRQ_HANDLED;
}

static int mxs_dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	int