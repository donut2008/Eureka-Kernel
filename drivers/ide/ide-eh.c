 to_idmac_chan(chan);
	struct idmac *idmac = to_idmac(chan->device);
	struct ipu *ipu = to_ipu(idmac);
	struct list_head *list, *tmp;
	unsigned long flags;

	mutex_lock(&ichan->chan_mutex);

	spin_lock_irqsave(&ipu->lock, flags);
	ipu_ic_disable_task(ipu, chan->chan_id);

	/* Return all descriptors into "prepared" state */
	list_for_each_safe(list, tmp, &ichan->queue)
		list_del_init(list);

	ichan->sg[0] = NULL;
	ichan->sg[1] = NULL;

	spin_unlock_irqrestore(&ipu->lock, flags);

	ichan->status = IPU_CHANNEL_INITIALIZED;

	mutex_unlock(&ichan->chan_mutex);

	return 0;
}

static int __idmac_terminate_all(struct dma_chan *chan)
{
	struct idmac_channel *ichan = to_idmac_chan(chan);
	struct idmac *idmac = to_idmac(chan->device);
	struct ipu *ipu = to_ipu(idmac);
	unsigned long flags;
	int i;

	ipu_disable_channel(idmac, ichan,
			    ichan->status >= IPU_CHANNEL_ENABLED);

	tasklet_disable(&ipu->tasklet);

	/* ichan->queue is modified in ISR, have to spinlock */
	spin_lock_irqsave(&ichan->lock, flags);
	list_splice_init(&ichan->queue, &ichan->free_list);

	if (ichan->desc)
		for (i = 0; i < ichan->n_tx_desc; i++) {
			struct idmac_tx_desc *desc = ichan->desc + i;
			if (list_empty(&desc->list))
				/* Descriptor was prepared, but not submitted */
				list_add(&desc->list, &ichan->free_list);

			async_tx_clear_ack(&desc->txd);
		}

	ichan->sg[0] = NULL;
	ichan->sg[1] = NULL;
	spin_unlock_irqrestore(&ichan->lock, flags);

	tasklet_enable(&ipu->tasklet);

	ichan->status = IPU_CHANNEL_INITIALIZED;

	return 0;
}

static int idmac_terminate_all(struct dma_chan *chan)
{
	struct idmac_channel *ichan = to_idmac_chan(chan);
	int ret;

	mutex_lock(&ichan->chan_mutex);

	ret = __idmac_terminate_all(chan);

	mutex_unlock(&ichan->chan_mutex);

	return ret;
}

#ifdef DEBUG
static irqreturn_t ic_sof_irq(int irq, void *dev_id)
{
	struct idmac_channel *ichan = dev_id;
	printk(KERN_DEBUG "Got SOF IRQ %d on Channel %d\n",
	       irq, ichan->dma_chan.chan_id);
	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}

static irqreturn_t ic_eof_irq(int irq, void *dev_id)
{
	struct idmac_channel *ichan = dev_id;
	printk(KERN_DEBUG "Got EOF IRQ %d on Channel %d\n",
	       irq, ichan->dma_chan.chan_id);
	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}

static int ic_sof = -EINVAL, ic_eof = -EINVAL;
#endif

static int idmac_alloc_chan_resources(struct dma_chan *chan)
{
	struct idmac_channel *ichan = to_idmac_chan(chan);
	struct idmac *idmac = to_idmac(chan->device);
	int ret;

	/* dmaengine.c now guarantees to only offer free channels */
	BUG_ON(chan->client_count > 1);
	WARN_ON(ichan->status != IPU_CHANNEL_FREE);

	dma_cookie_init(chan);

	ret = ipu_irq_map(chan->chan_id);
	if (ret < 0)
		goto eimap;

	ichan->eof_irq = ret;

	/*
	 * Important to first disable the channel, because maybe someone
	 * used it before us, e.g., the bootloader
	 */
	ipu_disable_channel(idmac, ichan, true);

	ret = ipu_init_channel(idmac, ichan);
	if (ret < 0)
		goto eichan;

	ret = request_irq(ichan->eof_irq, idmac_interrupt, 0,
			  ichan->eof_name, ichan);
	if (ret < 0)
		goto erirq;

#ifdef DEBUG
	if (chan->chan_id == IDMAC_IC_7) {
		ic_sof = ipu_irq_map(69);
		if (ic_sof > 0) {
			ret = request_irq(ic_sof, ic_sof_irq, 0, "IC SOF", ichan);
			if (ret)
				dev_err(&chan->dev->device, "request irq failed for IC SOF");
		}
		ic_eof = ipu_irq_map(70);
		if (ic_eof > 0) {
			ret = request_irq(ic_eof, ic_eof_irq, 0, "IC EOF", ichan);
			if (ret)
				dev_err(&chan->dev->device, "request irq failed for IC EOF");
		}
	}
#endif

	ichan->status = IPU_CHANNEL_INITIALIZED;

	dev_dbg(&chan->dev->device, "Found channel 0x%x, irq %d\n",
		chan->chan_id, ichan->eof_irq);

	return ret;

erirq:
	ipu_uninit_channel(idmac, ichan);
eichan:
	ipu_irq_unmap(chan->chan_id);
eimap:
	return ret;
}

static void idmac_free_chan_resources(struct dma_chan *chan)
{
	struct idmac_channel *ichan = to_idmac_chan(chan);
	struct idmac *idmac = to_idmac(chan->device);

	mutex_lock(&ichan->chan_mutex);

	__idmac_terminate_all(chan);

	if (ichan->status > IPU_CHANNEL_FREE) {
#ifdef DEBUG
		if (chan->chan_id == IDMAC_IC_7) {
			if (ic_sof > 0) {
				free_irq(ic_sof, ichan);
				ipu_irq_unmap(69);
				ic_sof = -EINVAL;
			}
			if (ic_eof > 0) {
				free_irq(ic_eof, ichan);
				ipu_irq_unmap(70);
				ic_eof = -EINVAL;
			}
		}
#endif
		free_irq(ichan->eof_irq, ichan);
		ipu_irq_unmap(chan->chan_id);
	}

	ichan->status = IPU_CHANNEL_FREE;

	ipu_uninit_channel(idmac, ichan);

	mutex_unlock(&ichan->chan_mutex);

	tasklet_schedule(&to_ipu(idmac)->tasklet);
}

static enum dma_status idmac_tx_status(struct dma_chan *chan,
		       dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	return dma_cookie_status(chan, cookie, txstate);
}

static int __init ipu_idmac_init(struct ipu *ipu)
{
	struct idmac *idmac = &ipu->idmac;
	struct dma_device *dma = &idmac->dma;
	int i;

	dma_cap_set(DMA_SLAVE, dma->cap_mask);
	dma_cap_set(DMA_PRIVATE, dma->cap_mask);

	/* Compulsory common fields */
	dma->dev				= ipu->dev;
	dma->device_alloc_chan_resources	= idmac_alloc_chan_resources;
	dma->device_free_chan_resources		= idmac_free_chan_resources;
	dma->device_tx_status			= idmac_tx_status;
	dma->device_issue_pending		= idmac_issue_pending;

	/* Compulsory for DMA_SLAVE fields */
	dma->device_prep_slave_sg		= idmac_prep_slave_sg;
	dma->device_pause			= idmac_pause;
	dma->device_terminate_all		= idmac_terminate_all;

	INIT_LIST_HEAD(&dma->channels);
	for (i = 0; i < IPU_CHANNELS_NUM; i++) {
		struct idmac_channel *ichan = ipu->channel + i;
		struct dma_chan *dma_chan = &ichan->dma_chan;

		spin_lock_init(&ichan->lock);
		mutex_init(&ichan->chan_mutex);

		ichan->status		= IPU_CHANNEL_FREE;
		ichan->sec_chan_en	= false;
		snprintf(ichan->eof_name, sizeof(ichan->eof_name), "IDMAC EOF %d", i);

		dma_chan->device	= &idmac->dma;
		dma_cookie_init(dma_chan);
		dma_chan->chan_id	= i;
		list_add_tail(&dma_chan->device_node, &dma->channels);
	}

	idmac_write_icreg(ipu, 0x00000070, IDMAC_CONF);

	return dma_async_device_register(&idmac->dma);
}

static void ipu_idmac_exit(struct ipu *ipu)
{
	int i;
	struct idmac *idmac = &ipu->idmac;

	for (i = 0; i < IPU_CHANNELS_NUM; i++) {
		struct idmac_channel *ichan = ipu->channel + i;

		idmac_terminate_all(&ichan->dma_chan);
	}

	dma_async_device_unregister(&idmac->dma);
}

/*****************************************************************************
 * IPU common probe / remove
 */

static int __init ipu_probe(struct platform_device *pdev)
{
	struct resource *mem_ipu, *mem_ic;
	int ret;

	spin_lock_init(&ipu_data.lock);

	mem_ipu	= platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mem_ic	= platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!mem_ipu || !mem_ic)
		return -EINVAL;

	ipu_data.dev = &pdev->dev;

	platform_set_drvdata(pdev, &ipu_data);

	ret = platform_get_irq(pdev, 0);
	if (ret < 0)
		goto err_noirq;

	ipu_data.irq_fn = ret;
	ret = platform_get_irq(pdev, 1);
	if (ret < 0)
		goto err_noirq;

	ipu_data.irq_err = ret;

	dev_dbg(&pdev->dev, "fn irq %u, err irq %u\n",
		ipu_data.irq_fn, ipu_data.irq_err);

	/* Remap IPU common registers */
	ipu_data.reg_ipu = ioremap(mem_ipu->start, resource_size(mem_ipu));
	if (!ipu_data.reg_ipu) {
		ret = -ENOMEM;
		goto err_ioremap_ipu;
	}

	/* Remap Image Converter and Image DMA Controller registers */
	ipu_data.reg_ic = ioremap(mem_ic->start, resource_size(mem_ic));
	if (!ipu_data.reg_ic) {
		ret = -ENOMEM;
		goto err_ioremap_ic;
	}

	/* Get IPU clock */
	ipu_data.ipu_clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(ipu_data.ipu_clk)) {
		ret = PTR_ERR(ipu_data.ipu_clk);
		goto err_clk_get;
	}

	/* Make sure IPU HSP clock is running */
	clk_prepare_enable(ipu_data.ipu_clk);

	/* Disable all interrupts */
	idmac_write_ipureg(&ipu_data, 0, IPU_INT_CTRL_1);
	idmac_write_ipureg(&ipu_data, 0, IPU_INT_CTRL_2);
	idmac_write_ipureg(&ipu_data, 0, IPU_INT_CTRL_3);
	idmac_write_ipureg(&ipu_data, 0, IPU_INT_CTRL_4);
	idmac_write_ipureg(&ipu_data, 0, IPU_INT_CTRL_5);

	dev_dbg(&pdev->dev, "%s @ 0x%08lx, fn irq %u, err irq %u\n", pdev->name,
		(unsigned long)mem_ipu->start, ipu_data.irq_fn, ipu_data.irq_err);

	ret = ipu_irq_attach_irq(&ipu_data, pdev);
	if (ret < 0)
		goto err_attach_irq;

	/* Initialize DMA engine */
	ret = ipu_idmac_init(&ipu_data);
	if (ret < 0)
		goto err_idmac_init;

	tasklet_init(&ipu_data.tasklet, ipu_gc_tasklet, (unsigned long)&ipu_data);

	ipu_data.dev = &pdev->dev;

	dev_dbg(ipu_data.dev, "IPU initialized\n");

	return 0;

err_idmac_init:
err_attach_irq:
	ipu_irq_detach_irq(&ipu_data, pdev);
	clk_disable_unprepare(ipu_data.ipu_clk);
	clk_put(ipu_data.ipu_clk);
err_clk_get:
	iounmap(ipu_data.reg_ic);
err_ioremap_ic:
	iounmap(ipu_data.reg_ipu);
err_ioremap_ipu:
err_noirq:
	dev_err(&pdev->dev, "Failed to probe IPU: %d\n", ret);
	return ret;
}

static int ipu_remove(struct platform_device *pdev)
{
	struct ipu *ipu = platform_get_drvdata(pdev);

	ipu_idmac_exit(ipu);
	ipu_irq_detach_irq(ipu, pdev);
	clk_disable_unprepare(ipu->ipu_clk);
	clk_put(ipu->ipu_clk);
	iounmap(ipu->reg_ic);
	iounmap(ipu->reg_ipu);
	tasklet_kill(&ipu->tasklet);

	return 0;
}

/*
 * We need two MEM resources - with IPU-common and Image Converter registers,
 * including PF_CONF and IDMAC_* registers, and two IRQs - function and error
 */
static struct platform_driver ipu_platform_driver = {
	.driver = {
		.name	= "ipu-core",
	},
	.remove		= ipu_remove,
};

static int __init ipu_init(void)
{
	return platform_driver_probe(&ipu_platform_driver, ipu_probe);
}
subsys_initcall(ipu_init);

MODULE_DESCRIPTION("IPU core driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Guennadi Liakhovetski <lg@denx.de>");
MODULE_ALIAS("platform:ipu-core");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * Copyright (C) 2008
 * Guennadi Liakhovetski, DENX Software Engineering, <lg@d