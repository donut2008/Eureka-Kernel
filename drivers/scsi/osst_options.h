->desc, chan);
	mxs_chan->desc.tx_submit = mxs_dma_tx_submit;

	/* the descriptor is ready */
	async_tx_ack(&mxs_chan->desc);

	return 0;

err_clk:
	free_irq(mxs_chan->chan_irq, mxs_dma);
err_irq:
	dma_free_coherent(mxs_dma->dma_device.dev, CCW_BLOCK_SIZE,
			mxs_chan->ccw, mxs_chan->ccw_phys);
err_alloc:
	return ret;
}

static void mxs_dma_free_chan_resources(struct dma_chan *chan)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;

	mxs_dma_disable_chan(chan);

	free_irq(mxs_chan->chan_irq, mxs_dma);

	dma_free_coherent(mxs_dma->dma_device.dev, CCW_BLOCK_SIZE,
			mxs_chan->ccw, mxs_chan->ccw_phys);

	clk_disable_unprepare(mxs_dma->clk);
}

/*
 * How to use the flags for ->device_prep_slave_sg() :
 *    [1] If there is only one DMA command in the DMA chain, the code should be:
 *            ......
 *            ->device_prep_slave_sg(DMA_CTRL_ACK);
 *            ......
 *    [2] If there are two DMA commands in the DMA chain, the code should be
 *            ......
 *            ->device_prep_slave_sg(0);
 *            ......
 *            ->device_prep_slave_sg(DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
 *            ......
 *    [3] If there are more than two DMA commands in the DMA chain, the code
 *        should be:
 *            ......
 *            ->device_prep_slave_sg(0);                                // First
 *            ......
 *            ->device_prep_slave_sg(DMA_PREP_INTERRUPT [| DMA_CTRL_ACK]);
 *            ......
 *            ->device_prep_slave_sg(DMA_PREP_INTERRUPT | DMA_CTRL_ACK); // Last
 *            ......
 */
static struct dma_async_tx_descriptor *mxs_dma_prep_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	struct mxs_dma_ccw *ccw;
	struct scatterlist *sg;
	u32 i, j;
	u32 *pio;
	bool append = flags & DMA_PREP_INTERRUPT;
	int idx = append ? mxs_chan->desc_count : 0;

	if (mxs_chan->status == DMA_IN_PROGRESS && !append)
		return NULL;

	if (sg_len + (append ? idx : 0) > NUM_CCW) {
		dev_err(mxs_dma->dma_device.dev,
				"maximum number of sg exceeded: %d > %d\n",
				sg_len, NUM_CCW);
		goto err_out;
	}

	mxs_chan->status = DMA_IN_PROGRESS;
	mxs_chan->flags = 0;

	/*
	 * If the sg is prepared with append flag set, the sg
	 * will be appended to the last prepared sg.
	 */
	if (append) {
		BUG_ON(idx < 1);
		ccw = &mxs_chan->ccw[idx - 1];
		ccw->next = mxs_chan->ccw_phys + sizeof(*ccw) * idx;
		ccw->bits |= CCW_CHAIN;
		ccw->bits &= ~CCW_IRQ;
		ccw->bits &= ~CCW_DEC_SEM;
	} else {
		idx = 0;
	}

	if (direction == DMA_TRANS_NONE) {
		ccw = &mxs_chan->ccw[idx++];
		pio = (u32 *) sgl;

		for (j = 0; j < sg_len;)
			ccw->pio_words[j++] = *pio++;

		ccw->bits = 0;
		ccw->bits |= CCW_IRQ;
		ccw->bits |= CCW_DEC_SEM;
		if (flags & DMA_CTRL_ACK)
			ccw->bits |= CCW_WAIT4END;
		ccw->bits |= CCW_HALT_ON_TERM;
		ccw->bits |= CCW_TERM_FLUSH;
		ccw->bits |= BF_CCW(sg_len, PIO_NUM);
		ccw->bits |= BF_CCW(MXS_DMA_CMD_NO_XFER, COMMAND);
	} else {
		for_each_sg(sgl, sg, sg_len, i) {
			if (sg_dma_len(sg) > MAX_XFER_BYTES) {
				dev_err(mxs_dma->dma_device.dev, "maximum bytes for sg entry exceeded: %d > %d\n",
						sg_dma_len(sg), MAX_XFER_BYTES);
				goto err_out;
			}

			ccw = &mxs_chan->ccw[idx++];

			ccw->next = mxs_chan->ccw_phys + sizeof(*ccw) * idx;
			ccw->bufaddr = sg->dma_address;
			ccw->xfer_bytes = sg_dma_len(sg);

			ccw->bits = 0;
			ccw->bits |= CCW_CHAIN;
			ccw->bits |= CCW_HALT_ON_TERM;
			ccw->bits |= CCW_TERM_FLUSH;
			ccw->bits |= BF_CCW(direction == DMA_DEV_TO_MEM ?
					MXS_DMA_CMD_WRITE : MXS_DMA_CMD_READ,
					COMMAND);

			if (i + 1 == sg_len) {
				ccw->bits &= ~CCW_CHAIN;
				ccw->bits |= CCW_IRQ;
				ccw->bits |= CCW_DEC_SEM;
				if (flags & DMA_CTRL_ACK)
					ccw->bits |= CCW_WAIT4END;
			}
		}
	}
	mxs_chan->desc_count = idx;

	return &mxs_chan->desc;

err_out:
	mxs_chan->status = DMA_ERROR;
	return NULL;
}

static struct dma_async_tx_descriptor *mxs_dma_prep_dma_cyc