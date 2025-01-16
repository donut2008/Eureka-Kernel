 must be met while calling mpc_dma_execute():
 * 	a) mchan->lock is acquired,
 * 	b) mchan->active list is empty,
 * 	c) mchan->queued list contains at least one entry.
 */
static void mpc_dma_execute(struct mpc_dma_chan *mchan)
{
	struct mpc_dma *mdma = dma_chan_to_mpc_dma(&mchan->chan);
	struct mpc_dma_desc *first = NULL;
	struct mpc_dma_desc *prev = NULL;
	struct mpc_dma_desc *mdesc;
	int cid = mchan->chan.chan_id;

	while (!list_empty(&mchan->queued)) {
		mdesc = list_first_entry(&mchan->queued,
						struct mpc_dma_desc, node);
		/*
		 * Grab either several mem-to-mem transfer descriptors
		 * or one peripheral transfer descriptor,
		 * don't mix mem-to-mem and peripheral transfer descriptors
		 * within the same 'active' list.
		 */
		if (mdesc->will_access_peripheral) {
			if (list_empty(&mchan->active))
				list_move_tail(&mdesc->node, &mchan->active);
			break;
		} else {
			list_move_tail(&mdesc->node, &mchan->active);
		}
	}

	/* Chain descriptors into one transaction */
	list_for_each_entry(mdesc, &mchan->active, node) {
		if (!first)
			first = mdesc;

		if (!prev) {
			prev = mdesc;
			continue;
		}

		prev->tcd->dlast_sga = mdesc->tcd_paddr;
		prev->tcd->e_sg = 1;
		mdesc->tcd->start = 1;

		prev = mdesc;
	}

	prev->tcd->int_maj = 1;

	/* Send first descriptor in chain into hardware */
	memcpy_toio(&mdma->tcd[cid], first->tcd, sizeof(struct mpc_dma_tcd));

	if (first != prev)
		mdma->tcd[cid].e_sg = 1;

	if (mdma->is_mpc8308) {
		/* MPC8308, no request lines, software initiated start */
		out_8(&mdma->regs->dmassrt, cid);
	} else if (first->will_access_peripheral) {
		/* Peripherals involved, start by external request signal */
		out_8(&mdma->regs->dmaserq, cid);
	} else {
		/* Memory to memory transfer, software initiated start */
		out_8(&mdma->regs->dmassrt, cid);
	}
}

/* Handle interrupt on one half of DMA controller (32 channels) */
static void mpc_dma_irq_process(struct mpc_dma *mdma, u32 is, u32 es, int off)
{
	struct mpc_dma_chan *mchan;
	struct mpc_dma_desc *mdesc;
	u32 status = is | es;
	int ch;

	while ((ch = fls(status) - 1) >= 0) {
		status &= ~(1 << ch);
		mchan = &mdma->channels[ch + off];

		spin_lock(&mchan->lock);

		out_8(&mdma->regs->dmacint, ch + off);
		out_8(&mdma->regs->dmacerr, ch + off);

		/* Check error status */
		if (es & (1 << ch))
			list_for_each_entry(mdesc, &mchan->active, node)
				mdesc->error = -EIO;

		/* Execute queued descriptors */
		list_splice_tail_init(&mchan->active, &mchan->completed);
		if (!list_empty(&mchan->queued))
			mpc_dma_execute(mchan);

		spin_unlock(&mchan->lock);
	}
}

/* Interrupt handler */
static irqreturn_t mpc_dma_irq(int irq, void *data)
{
	struct mpc_dma *mdma = data;
	uint es;

	/* Save error status register */
	es = in_be32(&mdma->regs->dmaes);
	spin_lock(&mdma->error_status_lock);
	if ((es & MPC_DMA_DMAES_VLD) && mdma->error_status == 0)
		mdma->error_status = es;
	spin_unlock(&mdma->error_status_lock);

	/* Handle interrupt on each channel */
	if (mdma->dma.chancnt > 32) {
		mpc_dma_irq_process(mdma, in_be32(&mdma->regs->dmainth),
					in_be32(&mdma->regs->dmaerrh), 32);
	}
	mpc_dma_irq_process(mdma, in_be32(&mdma->regs->dmaintl),
					in_be32(&mdma->regs->dmaerrl), 0);

	/* Schedule tasklet */
	tasklet_schedule(&mdma->tasklet);

	return IRQ_HANDLED;
}

/* process completed descriptors */
static void mpc_dma_process_completed(struct mpc_dma *mdma)
{
	dma_cookie_t last_cookie = 0;
	struct mpc_dma_chan *mchan;
	struct mpc_dma_desc *mdesc;
	struct dma_async_tx_descriptor *desc;
	unsigned long flags;
	LIST_HEAD(list);
	int i;

	for (i = 0; i < mdma->dma.chancnt; i++) {
		mchan = &mdma->channels[i];

		/* Get all completed descriptors */
		spin_lock_irqsave(&mchan->lock, flags);
		if (!list_empty(&mchan->completed))
			list_splice_tail_init(&mchan->completed, &list);
		spin_unlock_irqrestore(&mchan->lock, flags);

		if (list_empty(&list))
			continue;

		/* Execute callbacks and run dependencies */
		list_for_each_entry(mdesc, &list, node) {
			desc = &mdesc->desc;

			if (desc->callback)
				desc->callback(desc->callback_param);

			last_cookie = desc->cookie;
			dma_run_dependencies(desc);
		}

		/* Free descriptors */
		spin_lock_irqsave(&mchan->lock, flags);
		list_splice_tail_init(&list, &mchan->free);
		mchan->chan.completed_cookie = last_cookie;
		spin_unlock_irqrestore(&mchan->lock, flags);
	}
}

/* DMA Tasklet */
static void mpc_dma_tasklet(unsigned long data)
{
	struct mpc_dma *mdma = (void *)data;
	unsigned long flags;
	uint es;

	spin_lock_irqsave(&mdma->error_status_lock, flags);
	es = mdma->error_status;
	mdma->error_status = 0;
	spin_unlock_irqrestore(&mdma->error_status_lock, flags);

	/* Print nice error report */
	if (es) {
		dev_err(mdma->dma.dev,
			"Hardware reported following error(s) on channel %u:\n",
						      MPC_DMA_DMAES_ERRCHN(es));

		if (es & MPC_DMA_DMAES_GPE)
			dev_err(mdma->dma.dev, "- Group Priority Error\n");
		if (es & MPC_DMA_DMAES_CPE)
			dev_err(mdma->dma.dev, "- Channel Priority Error\n");
		if (es & MPC_DMA_DMAES_SAE)
			dev_err(mdma->dma.dev, "- Source Address Error\n");
		if (es & MPC_DMA_DMAES_SOE)
			dev_err(mdma->dma.dev, "- Source Offset"
						" Configuration Error\n");
		if (es & MPC_DMA_DMAES_DAE)
			dev_err(mdma->dma.dev, "- Destination Address"
								" Error\n");
		if (es & MPC_DMA_DMAES_DOE)
			dev_err(mdma->dma.dev, "- Destination Offset"
						" Configuration Error\n");
		if (es & MPC_DMA_DMAES_NCE)
			dev_err(mdma->dma.dev, "- NBytes/Citter"
						" Configuration Error\n");
		if (es & MPC_DMA_DMAES_SGE)
			dev_err(mdma->dma.dev, "- Scatter/Gather"
						" Configuration Error\n");
		if (es & MPC_DMA_DMAES_SBE)
			dev_err(mdma->dma.dev, "- Source Bus Error\n");
		if (es & MPC_DMA_DMAES_DBE)
			dev_err(mdma->dma.dev, "- Destination Bus Error\n");
	}

	mpc_dma_process_completed(mdma);
}

/* Submit descriptor to hardware */
static dma_cookie_t mpc_dma_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct mpc_dma_chan *mchan = dma_chan_to_mpc_dma_chan(txd->chan);
	struct mpc_dma_desc *mdesc;
	unsigned long flags;
	dma_cookie_t cookie;

	mdesc = container_of(txd, struct mpc_dma_desc, desc);

	spin_lock_irqsave(&mchan->lock, flags);

	/* Move descriptor to queue */
	list_move_tail(&mdesc->node, &mchan->queued);

	/* If channel is idle, execute all queued descriptors */
	if (list_empty(&mchan->active))
		mpc_dma_execute(mchan);

	/* Update cookie */
	cookie = dma_cookie_assign(txd);
	spin_unlock_irqrestore(&mchan->lock, flags);

	return cookie;
}

/* Alloc channel resources */
static int mpc_dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct mpc_dma *mdma = dma_chan_to_mpc_dma(chan);
	struct mpc_dma_chan *mchan = dma_chan_to_mpc_dma_chan(chan);
	struct mpc_dma_desc *mdesc;
	struct mpc_dma_tcd *tcd;
	dma_addr_t tcd_paddr;
	unsigned long flags;
	LIST_HEAD(descs);
	int i;

	/* Alloc DMA memory for Transfer Control Descriptors */
	tcd = dma_alloc_coherent(mdma->dma.dev,
			MPC_DMA_DESCRIPTORS * sizeof(struct mpc_dma_tcd),
							&tcd_paddr, GFP_KERNEL);
	if (!tcd)
		return -ENOMEM;

	/* Alloc descriptors for this channel */
	for (i = 0; i < MPC_DMA_DESCRIPTORS; i++) {
		mdesc = kzalloc(sizeof(struct mpc_dma_desc), GFP_KERNEL);
		if (!mdesc) {
			dev_notice(mdma->dma.dev, "Memory allocation error. "
					"Allocated only %u descriptors\n", i);
			break;
		}

		dma_async_tx_descriptor_init(&mdesc->desc, chan);
		mdesc->desc.flags = DMA_CTRL_ACK;
		mdesc->desc.tx_submit = mpc_dma_tx_submit;

		mdesc->tcd = &tcd[i];
		mdesc->tcd_paddr = tcd_paddr + (i * sizeof(struct mpc_dma_tcd));

		list_add_tail(&mdesc->node, &descs);
	}

	/* Return error only if no descriptors were allocated */
	if (i == 0) {
		dma_free_coherent(mdma->dma.dev,
			MPC_DMA_DESCRIPTORS * sizeof(struct mpc_dma_tcd),
								tcd, tcd_paddr);
		return -ENOMEM;
	}

	spin_lock_irqsave(&mchan->lock, flags);
	mchan->tcd = tcd;
	mchan->tcd_paddr = tcd_paddr;
	list_splice_tail_init(&descs, &mchan->free);
	spin_unlock_irqrestore(&mchan->lock, flags);

	/* Enable Error Interrupt */
	out_8(&mdma->regs->dmaseei, chan->chan_id);

	return 0;
}

/* Free channel resources */
static void mpc_dma_free_chan_resources(struct dma_chan *chan)
{
	struct mpc_dma *mdma = dma_chan_to_mpc_dma(chan);
	struct mpc_dma_chan *mchan = dma_chan_to_mpc_dma_chan(chan);
	struct mpc_dma_desc *mdesc, *tmp;
	struct mpc_dma_tcd *tcd;
	dma_addr_t tcd_paddr;
	unsigned long flags;
	LIST_HEAD(descs);

	spin_lock_irqsave(&mchan->lock, flags);

	/* Channel must be idle */
	BUG_ON(!list_empty(&mchan->prepared));
	BUG_ON(!list_empty(&mchan->queued));
	BUG_ON(!list_empty(&mchan->active));
	BUG_ON(!list_empty(&mchan->completed));

	/* Move data */
	list_splice_tail_init(&mchan->free, &descs);
	tcd = mchan->tcd;
	tcd_paddr = mchan->tcd_paddr;

	spin_unlock_irqrestore(&mchan->lock, flags);

	/* Free DMA memory used by descriptors */
	dma_free_coherent(mdma->dma.dev,
			MPC_DMA_DESCRIPTORS * sizeof(struct mpc_dma_tcd),
								tcd, tcd_paddr);

	/* Free descriptors */
	list_for_each_entry_safe(mdesc, tmp, &descs, node)
		kfree(mdesc);

	/* Disable Error Interrupt */
	out_8(&mdma->regs->dmaceei, chan->chan_id);
}

/* Send all pending descriptor to hardware */
static void mpc_dma_issue_pending(struct dma_chan *chan)
{
	/*
	 * We are posting descriptors to the hardware as soon as
	 * they are ready, so this function does nothing.
	 */
}

/* Check request completion status */
static enum dma_status
mpc_dma_tx_status(struct dma_chan *chan, dma_cookie_t cookie,
	       struct dma_tx_state *txstate)
{
	return dma_cookie_status(chan, cookie, txstate);
}

/* Prepare descriptor for memory to memory copy */
static struct dma_async_tx_descriptor *
mpc_dma_prep_memcpy(struct dma_chan *chan, dma_addr_t dst, dma_addr_t src,
					size_t len, unsigned long flags)
{
	struct mpc_dma *mdma = dma_chan_to_mpc_dma(chan);
	struct mpc_dma_chan *mchan = dma_chan_to_mpc_dma_chan(chan);
	struct mpc_dma_desc *mdesc = NULL;
	struct mpc_dma_tcd *tcd;
	unsigned long iflags;

	/* Get free descriptor */
	spin_lock_irqsave(&mchan->lock, iflags);
	if (!list_empty(&mchan->free)) {
		mdesc = list_first_entry(&mchan->free, struct mpc_dma_desc,
									node);
		list_del(&mdesc->node);
	}
	spin_unlock_irqrestore(&mchan->lock, iflags);

	if (!mdesc) {
		/* try to free completed descriptors */
		mpc_dma_process_completed(mdma);
		return NULL;
	}

	mdesc->error = 0;
	mdesc->will_access_peripheral = 0;
	tcd = mdesc->tcd;

	/* Prepare Transfer Control Descriptor for this transaction */
	memset(tcd, 0, sizeof(struct mpc_dma_tcd));

	if (IS_ALIGNED(src | dst | len, 32)) {
		tcd->ssize = MPC_DMA_TSIZE_32;
		tcd->dsize = MPC_DMA_TSIZE_32;
		tcd->soff = 32;
		tcd->doff = 32;
	} else if (!mdma->is_mpc8308 && IS_ALIGNED(src | dst | len, 16)) {
		/* MPC8308 doesn't support 16 byte transfers */
		tcd->ssize = MPC_DMA_TSIZE_16;
		tcd->dsize = MPC_DMA_TSIZE_16;
		tcd->soff = 16;
		tcd->doff = 16;
	} else if (IS_ALIGNED(src | dst | len, 4)) {
		tcd->ssize = MPC_DMA_TSIZE_4;
		tcd->dsize = MPC_DMA_TSIZE_4;
		tcd->soff = 4;
		tcd->doff = 4;
	} else if (IS_ALIGNED(src | dst | len, 2)) {
		tcd->ssize = MPC_DMA_TSIZE_2;
		tcd->dsize = MPC_DMA_TSIZE_2;
		tcd->soff = 2;
		tcd->doff = 2;
	} else {
		tcd->ssize = MPC_DMA_TSIZE_1;
		tcd->dsize = MPC_DMA_TSIZE_1;
		tcd->soff = 1;
		tcd->doff = 1;
	}

	tcd->saddr = src;
	tcd->daddr = dst;
	tcd->nbytes = len;
	tcd->biter = 1;
	tcd->citer = 1;

	/* Place descriptor in prepared list */
	spin_lock_irqsave(&mchan->lock, iflags);
	list_add_tail(&mdesc->node, &mchan->prepared);
	spin_unlock_irqrestore(&mchan->lock, iflags);

	return &mdesc->desc;
}

static struct dma_async_tx_descriptor *
mpc_dma_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct mpc_dma *mdma = dma_chan_to_mpc_dma(chan);
	struct mpc_dma_chan *mchan = dma_chan_to_mpc_dma_chan(chan);
	struct mpc_dma_desc *mdesc = NULL;
	dma_addr_t per_paddr;
	u32 tcd_nunits;
	struct mpc_dma_tcd *tcd;
	unsigned long iflags;
	struct scatterlist *sg;
	size_t len;
	int iter, i;

	/* Currently there is no proper support for scatter/gather */
	if (sg_len != 1)
		return NULL;

	if (!is_slave_direction(direction))
		return NULL;

	for_each_sg(sgl, sg, sg_len, i) {
		spin_lock_irqsave(&mchan->lock, iflags);

		mdesc = list_first_entry(&mchan->free,
						struct mpc_dma_desc, node);
		if (!mdesc) {
			spin_unlock_irqrestore(&mchan->lock, iflags);
			/* Try to free completed descriptors */
			mpc_dma_process_completed(mdma);
			return NULL;
		}

		list_del(&mdesc->node);

		if (direction == DMA_DEV_TO_MEM) {
			per_paddr = mchan->src_per_paddr;
			tcd_nunits = mchan->src_tcd_nunits;
		} else {
			per_paddr = mchan->dst_per_paddr;
			tcd_nunits = mchan->dst_tcd_nunits;
		}

		spin_unlock_irqrestore(&mchan->lock, iflags);

		if (per_paddr == 0 || tcd_nunits == 0)
			goto err_prep;

		mdesc->error = 0;
		mdesc->will_access_peripheral = 1;

		/* Prepare Transfer Control Descriptor for this transaction */
		tcd = mdesc->tcd;

		memset(tcd, 0, sizeof(struct mpc_dma_tcd));

		if (!IS_ALIGNED(sg_dma_address(sg), 4))
			goto err_prep;

		if (direction == DMA_DEV_TO_MEM) {
			tcd->saddr = per_paddr;
			tcd->daddr = sg_dma_address(sg);
			tcd->soff = 0;
			tcd->doff = 4;
		} else {
			tcd->saddr = sg_dma_address(sg);
			tcd->daddr = per_paddr;
			tcd->soff = 4;
			tcd->doff = 0;
		}

		tcd->ssize = MPC_DMA_TSIZE_4;
		tcd->dsize = MPC_DMA_TSIZE_4;

		len = sg_dma_len(sg);
		tcd->nbytes = tcd_nunits * 4;
		if (!IS_ALIGNED(len, tcd->nbytes))
			goto err_prep;

		iter = len / tcd->nbytes;
		if (iter >= 1 << 15) {
			/* len is too big */
			goto err_prep;
		}
		/* citer_linkch contains the high bits of iter */
		tcd->biter = iter & 0x1ff;
		tcd->biter_linkch = iter >> 9;
		tcd->citer = tcd->biter;
		tcd->citer_linkch = tcd->biter_linkch;

		tcd->e_sg = 0;
		tcd->d_req = 1;

		/* Place descriptor in prepared list */
		spin_lock_irqsave(&mchan->lock, iflags);
		list_add_tail(&mdesc->node, &mchan->prepared);
		spin_unlock_irqrestore(&mchan->lock, iflags);
	}

	return &mdesc->desc;

err_prep:
	/* Put the descriptor back */
	spin_lock_irqsave(&mchan->lock, iflags);
	list_add_tail(&mdesc->node, &mchan->free);
	spin_unlock_irqrestore(&mchan->lock, iflags);

	return NULL;
}

static int mpc_dma_device_config(struct dma_chan *chan,
				 struct dma_slave_config *cfg)
{
	struct mpc_dma_chan *mchan = dma_chan_to_mpc_dma_chan(chan);
	unsigned long flags;

	/*
	 * Software constraints:
	 *  - only transfers between a peripheral device and
	 *     memory are supported;
	 *  - only peripheral devices with 4-byte FIFO access register
	 *     are supported;
	 *  - minimal transfer chunk is 4 bytes and consequently
	 *     source and destination addresses must be 4-byte aligned
	 *     and transfer size must be aligned on (4 * maxburst)
	 *     boundary;
	 *  - during the transfer RAM address is being incremented by
	 *     the size of minimal transfer chunk;
	 *  - peripheral port's address is constant during the transfer.
	 */

	if (cfg->src_addr_width != DMA_SLAVE_BUSWIDTH_4_BYTES ||
	    cfg->dst_addr_width != DMA_SLAVE_BUSWIDTH_4_BYTES ||
	    !IS_ALIGNED(cfg->src_addr, 4) ||
	    !IS_ALIGNED(cfg->dst_addr, 4)) {
		return -EINVAL;
	}

	spin_lock_irqsave(&mchan->lock, flags);

	mchan->src_per_paddr = cfg->src_addr;
	mchan->src_tcd_nunits = cfg->src_maxburst;
	mchan->dst_per_paddr = cfg->dst_addr;
	mchan->dst_tcd_nunits = cfg->dst_maxburst;

	/* Apply defaults */
	if (mchan->src_tcd_nunits == 0)
		mchan->src_tcd_nunits = 1;
	if (mchan->dst_tcd_nunits == 0)
		mchan->dst_tcd_nunits = 1;

	spin_unlock_irqrestore(&mchan->lock, flags);

	return 0;
}

static int mpc_dma_device_terminate_all(struct dma_chan *chan)
{
	struct mpc_dma_chan *mchan = dma_chan_to_mpc_dma_chan(chan);
	struct mpc_dma *mdma = dma_chan_to_mpc_dma(chan);
	unsigned long flags;

	/* Disable channel requests */
	spin_lock_irqsave(&mchan->lock, flags);

	out_8(&mdma->regs->dmacerq, chan->chan_id);
	list_splice_tail_init(&mchan->prepared, &mchan->free);
	list_splice_tail_init(&mchan->queued, &mchan->free);
	list_splice_tail_init(&mchan->active, &mchan->free);

	spin_unlock_irqrestore(&mchan->lock, flags);

	return 0;
}

static int mpc_dma_probe(struct platform_device *op)
{
	struct device_node *dn = op->dev.of_node;
	struct device *dev = &op->dev;
	struct dma_device *dma;
	struct mpc_dma *mdma;
	struct mpc_dma_chan *mchan;
	struct resource res;
	ulong regs_start, regs_size;
	int retval, i;
	u8 chancnt;

	mdma = devm_kzalloc(dev, sizeof(struct mpc_dma), GFP_KERNEL);
	if (!mdma) {
		dev_err(dev, "Memory exhausted!\n");
		retval = -ENOMEM;
		goto err;
	}

	mdma->irq = irq_of_parse_and_map(dn, 0);
	if (mdma->irq == NO_IRQ) {
		dev_err(dev, "Error mapping IRQ!\n");
		retval = -EINVAL;
		goto err;
	}

	if (of_device_is_compatible(dn, "fsl,mpc8308-dma")) {
		mdma->is_mpc8308 = 1;
		mdma->irq2 = irq_of_parse_and_map(dn, 1);
		if (mdma->irq2 == NO_IRQ) {
			dev_err(dev, "Error mapping IRQ!\n");
			retval = -EINVAL;
			goto err_dispose1;
		}
	}

	retval = of_address_to_resource(dn, 0, &res);
	if (retval) {
		dev_err(dev, "Error parsing memory region!\n");
		goto err_dispose2;
	}

	regs_start = res.start;
	regs_size = resource_size(&res);

	if (!devm_request_mem_region(dev, regs_start, regs_size, DRV_NAME)) {
		dev_err(dev, "Error requesting memory region!\n");
		retval = -EBUSY;
		goto err_dispose2;
	}

	mdma->regs = devm_ioremap(dev, regs_start, regs_size);
	if (!mdma->regs) {
		dev_err(dev, "Error mapping memory region!\n");
		retval = -ENOMEM;
		goto err_dispose2;
	}

	mdma->tcd = (struct mpc_dma_tcd *)((u8 *)(mdma->regs)
							+ MPC_DMA_TCD_OFFSET);

	retval = request_irq(mdma->irq, &mpc_dma_irq, 0, DRV_NAME, mdma);
	if (retval) {
		dev_err(dev, "Error requesting IRQ!\n");
		retval = -EINVAL;
		goto err_dispose2;
	}

	if (mdma->is_mpc8308) {
		retval = request_irq(mdma->irq2, &mpc_dma_irq, 0,
							DRV_NAME, mdma);
		if (retval) {
			dev_err(dev, "Error requesting IRQ2!\n");
			retval = -EINVAL;
			goto err_free1;
		}
	}

	spin_lock_init(&mdma->error_status_lock);

	dma = &mdma->dma;
	dma->dev = dev;
	dma->device_alloc_chan_resources = mpc_dma_alloc_chan_resources;
	dma->device_free_chan_resources = mpc_dma_free_chan_resources;
	dma->device_issue_pending = mpc_dma_issue_pending;
	dma->device_tx_status = mpc_dma_tx_status;
	dma->device_prep_dma_memcpy = mpc_dma_prep_memcpy;
	dma->device_prep_slave_sg = mpc_dma_p