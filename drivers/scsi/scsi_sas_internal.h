ice_alloc_chan_resources	= moxart_alloc_chan_resources;
	dma->device_free_chan_resources		= moxart_free_chan_resources;
	dma->device_issue_pending		= moxart_issue_pending;
	dma->device_tx_status			= moxart_tx_status;
	dma->device_config			= moxart_slave_config;
	dma->device_terminate_all		= moxart_terminate_all;
	dma->dev				= dev;

	INIT_LIST_HEAD(&dma->channels);
}

static irqreturn_t moxart_dma_interrupt(int irq, void *devid)
{
	struct moxart_dmadev *mc = devid;
	struct moxart_chan *ch = &mc->slave_chans[0];
	unsigned int i;
	unsigned long flags;
	u32 ctrl;

	dev_dbg(chan2dev(&ch->vc.chan), "%s\n", __func__);

	for (i = 0; i < APB_DMA_MAX_CHANNEL; i++, ch++) {
		if (!ch->allocated)
			continue;

		ctrl = readl(ch->base + REG_OFF_CTRL);

		dev_dbg(chan2dev(&ch->vc.chan), "%s: ch=%p ch->base=%p ctrl=%x\n",
			__func__, ch, ch->base, ctrl);

		if (ctrl & APB_DMA_FIN_INT_STS) {
			ctrl &= ~APB_DMA_FIN_INT_STS;
			if (ch->desc) {
				spin_lock_irqsave(&ch->vc.lock, flags);
				if (++ch->sgidx < ch->desc->sglen) {
					moxart_dma_start_sg(ch, ch->sgidx);
				} else {
					vchan_cookie_complete(&ch->desc->vd);
					moxart_dma_start_desc(&ch->vc.chan);
				}
				spin_unlock_irqrestore(&ch->vc.lock, flags);
			}
		}

		if (ctrl & APB_DMA_ERR_INT_STS) {
			ctrl &= ~APB_DMA_ERR_INT_STS;
			ch->error = 1;
		}

		writel(ctrl, ch->base + REG_OFF_CTRL);
	}

	return IRQ_HANDLED;
}

static int moxart_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node