dma_chan(struct dma_chan *c)
{
	return container_of(c, struct moxart_chan, vc.chan);
}

static inline struct moxart_desc *to_moxart_dma_desc(
	struct dma_async_tx_descriptor *t)
{
	return container_of(t, struct moxart_desc, vd.tx);
}

static void moxart_dma_desc_free(struct virt_dma_desc *vd)
{
	kfree(container_of(vd, struct moxart_desc, vd));
}

static int moxart_terminate_all(struct dma_chan *chan)
{
	struct moxart_chan *ch = to_moxart_dma_chan(chan);
	unsigned long flags;
	LIST_HEAD(head);
	u32 ctrl;

	dev_dbg(chan2dev(chan), "%s: ch=%p\n", __func__, ch);

	spin_lock_irqsave(&ch->vc.lock, flags);

	if (ch->desc) {
		moxart_dma_desc_free(&ch->desc->vd);
		ch->desc = NULL;
	}

	ctrl = readl(ch->base + REG_OFF_CTRL);
	ctrl &= ~(APB_DMA_ENABLE | APB_DMA_FIN_INT_EN | APB_DMA_ERR_INT_EN);
	writel(ctrl, ch->base + REG_OFF_CTRL);

	vchan_get_all_descriptors(&ch->vc, &head);
	spin_unlock_irqrestore(&ch->vc.lock, flags);
	vchan_dma_desc_free_list(&ch->vc, &head);

	return 0;
}

static int moxart_slave_config(struct dma_chan *chan,
			       struct dma_slave_config *cfg)
{
	struct moxart_chan *ch = to_moxart_dma_chan(chan);
	u32 ctrl;

	ch->cfg = *cfg;

	ctrl = readl(ch->base + REG_OFF_CTRL);
	ctrl |= APB_DMA_BURST_MODE;
	ctrl &= ~(APB_DMA_DEST_MASK | APB_DMA_SOURCE_MASK);
	ctrl &= ~(APB_DMA_DEST_REQ_NO_MASK | APB_DMA_SOURCE_REQ_NO_MASK);

	switch (ch->cfg.src_addr_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		ctrl |= APB_DMA_DATA_WIDTH_1;
		if (ch->cfg.direction != DMA_MEM_TO_DEV)
			ctrl |= APB_DMA_DEST_INC_1_4;
		else
			ctrl |= APB_DMA_SOURCE_INC_1_4;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		ctrl |= APB_DMA_DATA_WIDTH_2;
		if (ch->cfg.direction != DMA_MEM_TO_DEV)
			ctrl |= APB_DMA_DEST_INC_2_8;
		else
			ctrl |= APB_DMA_SOURCE_INC_2_8;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		ctrl &= ~APB_DMA_DATA_WIDTH;
		if (ch->cfg.direction != DMA_MEM_TO_DEV)
			ctrl |= APB_DMA_DEST_INC_4_16;
		else
			ctrl |= APB_DMA_SOURCE_INC_4_16;
		break;
	default:
		return -EINVAL;
	}

	if (ch->cfg.direction == DMA_MEM_TO_DEV) {
		ctrl &= ~APB_DMA_DEST_SELECT;
		ctrl |= APB_DMA_SOURCE_SELECT;
		ctrl |= (ch->line_reqno << 16 &
			 APB_DMA_DEST_REQ_NO_MASK);
	} else {
		ctrl |= APB_DMA_DEST_SELECT;
		ctrl &= ~APB_DMA_SOURCE_SELECT;
		ctrl |= (ch->line_reqno << 24 &
			 APB_DMA_SOURCE_REQ_NO_MASK);
	}

	writel(ctrl, ch->base + REG_OFF_CTRL);

	return 0;
}

static struct dma_async_tx_descriptor *moxart_prep_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl,
	unsigned int sg_len, enum dma_transfer_direction dir,
	unsigned long tx_flags, void *context)
{
	struct moxart_chan *ch = to_moxart_dma_chan(chan);
	struct moxart_desc *d;
	enum dma_slave_buswidth dev_width;
	dma_addr_t dev_addr;
	struct scatterlist *sgent;
	unsigned int es;
	unsigned int i;

	if (!is_slave_direction(dir)) {
		dev_err(chan2dev(chan), "%s: invalid DMA direction\n",
			__func__);
		return NULL;
	}

	if (dir == DMA_DEV_TO_MEM) {
		dev_addr = ch->cfg.src_addr;
		dev_width = ch->cfg.src_addr_width;
	} else {
		dev_addr = ch->cfg.dst_addr;
		dev_width = ch->cfg.dst_addr_width;
	}

	switch (dev_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		es = MOXART_DMA_DATA_TYPE_S8;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		es = MOXART_DMA_DATA_TYPE_S16;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		es = MOXART_DMA_DATA_TYPE_S32;
		break;
	default:
		dev_err(chan2dev(chan), "%s: unsupported data width (%u)\n",
			__func__, dev_width);
		return NULL;
	}

	d = kzalloc(sizeof(*d) + sg_len * sizeof(d->sg[0]), GFP_ATOMIC);
	if (!d)
		return NULL;

	d->dma_dir = dir;
	d->dev_addr = dev_addr;
	d->es = es;

	for_each_sg(sgl, sgent, sg_len, i) {
		d->sg[i].addr = sg_dma_address(sgent);
		d->sg[i].len = sg_dma_len(sgent);
	}

	d->sglen = sg_len;

	ch->error = 0;

	return vchan_tx_prep(&ch->vc, &d->vd, tx_flags);
}

static struct dma_chan *moxart_of_xlate(struct of_phandle_args *dma_spec,
					struct of_dma *ofdma)
{
	struct moxart_dmadev *mdc = ofdma->of_dma_data;
	struct dma_chan *chan;
	struct moxart_chan *ch;

	chan = dma_get_any_slave_channel(&mdc->dma_slave);
	if (!chan)
		return NULL;

	ch = to_moxart_dma_chan(chan);
	ch->line_reqno = dma_spec->args[0];

	return chan;
}

static int moxart_alloc_chan_resources(struct dma_chan *chan)
{
	struct moxart_chan *ch = to_moxart_dma_chan(chan);

	dev_dbg(chan2dev(chan), "%s: allocating channel #%u\n",
		__func__, ch->ch_num);
	ch->allocated = 1;

	return 0;
}

static void moxart_free_chan_resources(struct dma_chan *chan)
{
	struct moxart_chan *ch = to_moxart_dma_chan(chan);

	vchan_free_chan_resources(&ch->vc);

	dev_dbg(chan2dev(chan), "%s: freeing channel #%u\n",
		__func__, ch->ch_num);
	ch->allocated = 0;
}

static void moxart_dma_set_params(struct moxart_chan *ch, dma_addr_t src_addr,
				  dma_addr_t dst_addr)
{
	writel(src_addr, ch->base + REG_OFF_ADDRESS_SOURCE);
	writel(dst_addr, ch->base + REG_OFF_ADDRESS_DEST);
}

static void moxart_set_transfer_params(struct moxart_chan *ch, unsigned int len)
{
	struct moxart_desc *d = ch->desc;
	unsigned int sglen_div = es_bytes[d->es];

	d->dma_cycles = len >> sglen_div;

	/*
	 * There are 4 cycles on 64 bytes copied, i.e. one cycle copies 16
	 * bytes ( when width is APB_DMAB_DATA_WIDTH_4 ).
	 */
	writel(d->dma_cycles, ch->base + REG_OFF_CYCLES);

	dev_dbg(chan2dev(&ch->vc.chan), "%s: set %u DMA cycles (len=%u)\n",
		__func__, d->dma_cycles, len);
}

static void moxart_start_dma(struct moxart_chan *ch)
{
	u32 ctrl;

	ctrl = readl(ch->base + REG_OFF_CTRL);
	ctrl |= (APB_DMA_ENABLE | APB_DMA_FIN_INT_EN | APB_DMA_ERR_INT_EN);
	writel(ctrl, ch->base + REG_OFF_CTRL);
}

static void moxart_dma_start_sg(struct moxart_chan *ch, unsigned int idx)
{
	struct moxart_desc *d = ch->desc;
	struct moxart_sg *sg = ch->desc->sg + idx;

	if (ch->desc->dma_dir == DMA_MEM_TO_DEV)
		moxart_dma_set_params(ch, sg->addr, d->dev_addr);
	else if (ch->desc->dma_dir == DMA_DEV_TO_MEM)
		moxart_dma_set_params(ch, d->dev_addr, sg->addr);

	moxart_set_transfer_params(ch, sg->len);

	moxart_start_dma(ch);
}

static void moxart_dma_start_desc(struct dma_chan *chan)
{
	struct moxart_chan *ch = to_moxart_dma_chan(chan);
	struct virt_dma_desc *vd;

	vd = vchan_next_desc(&ch->vc);

	if (!vd) {
		ch->desc = NULL;
		return;
	}

	list_del(&vd->node);

	ch->desc = to_moxart_dma_desc(&vd->tx);
	ch->sgidx = 0;

	moxart_dma_start_sg(ch, 0);
}

static void moxart_issue_pending(struct dma_chan *chan)
{
	struct moxart_chan *ch = to_moxart_dma_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&ch->vc.lock, flags);
	if (vchan_issue_pending(&ch->vc) && !ch->desc)
		moxart_dma_start_desc(chan);
	spin_unlock_irqrestore(&ch->vc.lock, flags);
}

static size_t moxart_dma_desc_size(struct mo