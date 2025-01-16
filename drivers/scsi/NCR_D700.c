sc_node) {
		i++;
		if (async_tx_test_ack(&desc->txd)) {
			list_del(&desc->desc_node);
			ret = desc;
			break;
		}
		dev_dbg(chan2dev(&pd_chan->chan), "desc %p not ACKed\n", desc);
	}
	spin_unlock(&pd_chan->lock);
	dev_dbg(chan2dev(&pd_chan->chan), "scanned %d descriptors\n", i);

	if (!ret) {
		ret = pdc_alloc_desc(&pd_chan->chan, GFP_ATOMIC);
		if (ret) {
			spin_lock(&pd_chan->lock);
			pd_chan->descs_allocated++;
			spin_unlock(&pd_chan->lock);
		} else {
			dev_err(chan2dev(&pd_chan->chan),
				"failed to alloc desc\n");
		}
	}

	return ret;
}

static void pdc_desc_put(struct pch_dma_chan *pd_chan,
			 struct pch_dma_desc *desc)
{
	if (desc) {
		spin_lock(&pd_chan->lock);
		list_splice_init(&desc->tx_list, &pd_chan->free_list);
		list_add(&desc->desc_node, &pd_chan->free_list);
		spin_unlock(&pd_chan->lock);
	}
}

static int pd_alloc_chan_resources(struct dma_chan *chan)
{
	struct pch_dma_chan *pd_chan = to_pd_chan(chan);
	struct pch_dma_desc *desc;
	LIST_HEAD(tmp_list);
	int i;

	if (!pdc_is_idle(pd_chan)) {
		dev_dbg(chan2dev(chan), "DMA channel not idle ?\n");
		return -EIO;
	}

	if (!list_empty(&pd_chan->free_list))
		return pd_chan->descs_allocated;

	for (i = 0; i < init_nr_desc_per_channel; i++) {
		desc = pdc_alloc_desc(chan, GFP_KERNEL);

		if (!desc) {
			dev_warn(chan2dev(chan),
				"Only allocated %d initial descriptors\n", i);
			break;
		}

		list_add_tail(&desc->desc_node, &tmp_list);
	}

	spin_lock_irq(&pd_chan->lock);
	list_splice(&tmp_list, &pd_chan->free_list);
	pd_chan->descs_allocated = i;
	dma_cookie_init(chan);
	spin_unlock_irq(&pd_chan->lock);

	pdc_enable_irq(chan, 1);

	return pd_chan->descs_allocated;
}

static void pd_free_chan_resources(struct dma_chan *chan)
{
	struct pch_dma_chan *pd_chan = to_pd_chan(chan);
	struct pch_dma *pd = to_pd(chan->device);
	struct pch_dma_desc *desc, *_d;
	LIST_HEAD(tmp_list);

	BUG_ON(!pdc_is_idle(pd_chan));
	BUG_ON(!list_empty(&pd_chan->active_list));
	BUG_ON(!list_empty(&pd_chan->queue));

	spin_lock_irq(&pd_chan->lock);
	list_splice_init(&pd_chan->free_list, &tmp_list);
	pd_chan->descs_allocated = 0;
	spin_unlock_irq(&pd_chan->lock);

	list_for_each_entry_safe(desc, _d, &tmp_list, desc_node)
		pci_pool_free(pd->pool, desc, desc->txd.phys);

	pdc_enable_irq(chan, 0);
}

static enum dma_status pd_tx_status(struct dma_chan *chan, dma_cookie_t cookie,
				    struct dma_tx_state *txstate)
{
	return dma_cookie_status(chan, cookie, txstate);
}

static void pd_issue_pending(struct dma_chan *chan)
{
	struct pch_dma_chan *pd_chan = to_pd_chan(chan);

	if (pdc_is_idle(pd_chan)) {
		spin_lock(&pd_chan->lock);
		pdc_advance_work(pd_chan);
		spin_unlock(&pd_chan->lock);
	}
}

static struct dma_async_tx_descriptor *pd_prep_slave_sg(struct dma_chan *chan,
			struct scatterlist *sgl, unsigned int sg_len,
			enum dma_transfer_direction direction, unsigned long flags,
			void *context)
{
	struct pch_dma_chan *pd_chan = to_pd_chan(chan);
	struct pch_dma_slave *pd_slave = chan->private;
	struct pch_dma_desc *first = NULL;
	struct pch_dma_desc *prev = NULL;
	struct pch_dma_desc *desc = NULL;
	struct scatterlist *sg;
	dma_addr_t reg;
	int i;

	if (unlikely(!sg_len)) {
		dev_info(chan2dev(chan), "prep_slave_sg: length is zero!\n");
		return NULL;
	}

	if (direction == DMA_DEV_TO_MEM)
		reg = pd_slave->rx_reg;
	else if (direction == DMA_MEM_TO_DEV)
		reg = pd_slave->tx_reg;
	else
		return NULL;

	pd_chan->dir = direction;
	pdc_set_dir(chan);

	for_each_sg(sgl, sg, sg_len, i) {
		desc = pdc_desc_get(pd_chan);

		if (!desc)
			goto err_desc_get;

		desc->regs.dev_addr = reg;
		desc->regs.mem_addr = sg_dma_address(sg);
		desc->regs.size = sg_dma_len(sg);
		desc->regs.next = DMA_DESC_FOLLOW_WITHOUT_IRQ;

		switch (pd_slave->width) {
		case PCH_DMA_WIDTH_1_BYTE:
			if (desc->regs.size > DMA_DESC_MAX_COUNT_1_BYTE)
				goto err_desc_get;
			desc->regs.size |= DMA_DESC_WIDTH_1_BYTE;
			break;
		case PCH_DMA_WIDTH_2_BYTES:
			if (desc->regs.size > DMA_DESC_MAX_COUNT_2_BYTES)
				goto err_desc_get;
			desc->regs.size |= DMA_DESC_WIDTH_2_BYTES;
			break;
		case PCH_DMA_WIDTH_4_BYTES:
			if (desc->regs.size > DMA_DESC_MAX_COUNT_4_BYTES)
				goto err_desc_get;
			desc->regs.size |= DMA_DESC_WIDTH_4_BYTES;
			break;
		default:
			goto err_desc_get;
		}

		if (!first) {
			first = desc;
		} else {
			prev->regs.next |= desc->txd.phys;
			list_add_tail(&desc->desc_node, &first->tx_list);
		}

		prev = desc;
	}

	if (flags & DMA_PREP_INTERRUPT)
		desc->regs.next = DMA_DESC_END_WITH_IRQ;
	else
		desc->regs.next = DMA_DESC_END_WITHOUT_IRQ;

	first->txd.cookie = -EBUSY;
	desc->txd.flags = flags;

	return &first->txd;

err_desc_get:
	dev_err(chan2dev(chan), "failed to get desc or wrong parameters\n");
	pdc_desc_put(pd_chan, first);
	return NULL;
}

static int pd_device_terminate_all(struct dma_chan *chan)
{
	struct pch_dma_chan *pd_chan = to_pd_chan(chan);
	struct pch_dma_desc *desc, *_d;
	LIST_HEAD(list);

	spin_lock_irq(&pd_chan->lock);

	pdc_set_mode(&pd_chan->chan, DMA_CTL0_DISABLE);

	list_splice_init(&pd_chan->active_list, &list);
	list_splice_init(&pd_chan->queue, &list);

	list_for_each_entry_safe(desc, _d, &list, desc_node)
		pdc_chain_complete(pd_chan, desc);

	spin_unlock_irq(&pd_chan->lock);

	return 0;
}

static void pdc_tasklet(unsigned long data)
{
	struct pch_dma_chan *pd_chan = (struct pch_dma_chan *)data;
	unsigned long flags;

	if (!pdc_is_idle(pd_chan)) {
		dev_err(chan2dev(&pd_chan->chan),
			"BUG: handle non-idle channel in tasklet\n");
		return;
	}

	spin_lock_irqsave(&pd_chan->lock, flags);
	if (test_and_clear_bit(0, &pd_chan->err_status))
		pdc_handle_error(pd_chan);
	else
		pdc_advance_work(pd_chan);
	spin_unlock_irqrestore(&pd_chan->lock, flags);
}

static irqreturn_t pd_irq(int irq, void *devid)
{
	struct pch_dma *pd = (struct pch_dma *)devid;
	struct pch_dma_chan *pd_chan;
	u32 sts0;
	u32 sts2;
	int i;
	int ret0 = IRQ_NONE;
	int ret2 = IRQ_NONE;

	sts0 = dma_readl(pd, STS0);
	sts2 = dma_readl(pd, STS2);

	dev_dbg(pd->dma.dev, "pd_irq sts0: %x\n", sts0);

	for (i = 0; i < pd->dma.chancnt; i++) {
		pd_chan = &pd->channels[i];

		if (i < 8) {
			if (sts0 & DMA_STATUS_IRQ(i)) {
				if (sts0 & DMA_STATUS0_ERR(i))
					set_bit(0, &pd_chan->err_status);

				tasklet_schedule(&pd_chan->tasklet);
				ret0 = IRQ_HANDLED;
			}
		} else {
			if (sts2 & DMA_STATUS_IRQ(i - 8)) {
				if (sts2 & DMA_STATUS2_ERR(i))
					set_bit(0, &pd_chan->err_status);

				tasklet_schedule(&pd_chan->tasklet);
				ret2 = IRQ_HANDLED;
			}
		}
	}

	/* clear interrupt bits in status register */
	if (ret0)
		dma_writel(pd, STS0, sts0);
	if (ret2)
		dma_writel(pd, STS2, sts2);

	return ret0 | ret2;
}

#ifdef	CONFIG_PM
static void pch_dma_save_regs(struct pch_dma *pd)
{
	struct pch_dma_chan *pd_chan;
	struct dma_chan *chan, *_c;
	int i = 0;

	pd->regs.dma_ctl0 = dma_readl(pd, CTL0);
	pd->regs.dma_ctl1 = dma_readl(pd, CTL1);
	pd->regs.dma_ctl2 = dma_readl(pd, CTL2);
	pd->regs.dma_ctl3 = dma_readl(pd, CTL3);

	list_for_each_entry_safe(chan, _c, &pd->dma.channels, device_node) {
		pd_chan = to_pd_chan(chan);

		pd->ch_regs[i].dev_addr = channel_readl(pd_chan, DEV_ADDR);
		pd->ch_regs[i].mem_addr = channel_readl(pd_chan, MEM_ADDR);
		pd->ch_regs[i].size = channel_readl(pd_chan, SIZE);
		pd->ch_regs[i].next = channel_readl(pd_chan, NEXT);

		i++;
	}
}

static void pch_dma_restore_regs(struct pch_dma *pd)
{
	struct pch_dma_chan *pd_chan;
	struct dma_chan *chan, *_c;
	int i = 0;

	dma_writel(pd, CTL0, pd->regs.dma_ctl0);
	dma_writel(pd, CTL1, pd->regs.dma_ctl1);
	dma_writel(pd, CTL2, pd->regs.dma_ctl2);
	dma_writel(pd, CTL3, pd->regs.dma_ctl3);

	list_for_each_entry_safe(chan, _c, &pd->dma.channels, device_node) {
		pd_chan = to_pd_chan(chan);

		channel_writel(pd_chan, DEV_ADDR, pd->ch_regs[i].dev_addr);
		channel_writel(pd_chan, MEM_ADDR, pd->ch_regs[i].mem_addr);
		channel_writel(pd_chan, SIZE, pd->ch_regs[i].size);
		channel_writel(pd_chan, NEXT, pd->ch_regs[i].next);

		i++;
	}
}

static int pch_dma_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct pch_dma *pd = pci_get_drvdata(pdev);

	if (pd)
		pch_dma_save_regs(pd);

	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));

	return 0;
}

static int pch_dma_resume(struct pci_dev *pdev)
{
	struct pch_dma *pd = pci_get_drvdata(pdev);
	int err;

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);

	err = pci_enable_device(pdev);
	if (err) {
		dev_dbg(&pdev->dev, "failed to enable device\n");
		return err;
	}

	if (pd)
		pch_dma_restore_regs(pd);

	return 0;
}
#endif

static int pch_dma_probe(struct pci_dev *pdev,
				   const struct pci_device_id *id)
{
	struct pch_dma *pd;
	struct pch_dma_regs *regs;
	unsigned int nr_channels;
	int err;
	int i;

	nr_channels = id->driver_data;
	pd = kzalloc(sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return -ENOMEM;

	pci_set_drvdata(pdev, pd);

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "Cannot enable PCI device\n");
		goto err_free_mem;
	}

	if (!(pci_resource_flags(pdev, 1) & IORESOURCE_MEM)) {
		dev_err(&pdev->dev, "Cannot find proper base address\n");
		err = -ENODEV;
		goto err_disable_pdev;
	}

	err = pci_request_regions(pdev, DRV_NAME);
	if (err) {
		dev_err(&pdev->dev, "Cannot obtain PCI resources\n");
		goto err_disable_pdev;
	}

	err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
	if (err) {
		dev_err(&pdev->dev, "Cannot set proper DMA config\n");
		goto err_free_res;
	}

	regs = pd->membase = pci_iomap(pdev, 1, 0);
	if (!pd->membase) {
		dev_err(&pdev->dev, "Cannot map MMIO registers\n");
		err = -ENOMEM;
		goto err_free_res;
	}

	pci_set_master(pdev);
	pd->dma.dev = &pdev->dev;

	err = request_irq(pdev->irq, pd_irq, IRQF_SHARED, DRV_NAME, pd);
	if (err) {
		dev_err(&pdev->dev, "Failed to request IRQ\n");
		goto err_iounmap;
	}

	pd->pool = pci_pool_create("pch_dma_desc_pool", pdev,
				   sizeof(struct pch_dma_desc), 4, 0);
	if (!pd->pool) {
		dev_err(&pdev->dev, "Failed to alloc DMA descriptors\n");
		err = -ENOMEM;
		goto err_free_irq;
	}


	INIT_LIST_HEAD(&pd->dma.channels);

	for (i = 0; i < nr_channels; i++) {
		struct pch_dma_chan *pd_chan = &pd->channels[i];

		pd_chan->chan.device = &pd->dma;
		dma_cookie_init(&pd_chan->chan);

		pd_chan->membase = &regs->desc[i];

		spin_lock_init(&pd_chan->lock);

		INIT_LIST_HEAD(&pd_chan->active_list);
		INIT_LIST_HEAD(&pd_chan->queue);
		INIT_LIST_HEAD(&pd_chan->free_list);

		tasklet_init(&pd_chan->tasklet, pdc_tasklet,
			     (unsigned long)pd_chan);
		list_add_tail(&pd_chan->chan.device_node, &pd->dma.channels);
	}

	dma_cap_zero(pd-