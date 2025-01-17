l(&(d->chans[request].vc.chan));
}

static int k3_dma_probe(struct platform_device *op)
{
	struct k3_dma_dev *d;
	const struct of_device_id *of_id;
	struct resource *iores;
	int i, ret, irq = 0;

	iores = platform_get_resource(op, IORESOURCE_MEM, 0);
	if (!iores)
		return -EINVAL;

	d = devm_kzalloc(&op->dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	d->base = devm_ioremap_resource(&op->dev, iores);
	if (IS_ERR(d->base))
		return PTR_ERR(d->base);

	of_id = of_match_device(k3_pdma_dt_ids, &op->dev);
	if (of_id) {
		of_property_read_u32((&op->dev)->of_node,
				"dma-channels", &d->dma_channels);
		of_property_read_u32((&op->dev)->of_node,
				"dma-requests", &d->dma_requests);
	}

	d->clk = devm_clk_get(&op->dev, NULL);
	if (IS_ERR(d->clk)) {
		dev_err(&op->dev, "no dma clk\n");
		return PTR_ERR(d->clk);
	}

	irq = platform_get_irq(op, 0);
	ret = devm_request_irq(&op->dev, irq,
			k3_dma_int_handler, 0, DRIVER_NAME, d);
	if (ret)
		return ret;

	/* init phy channel */
	d->phy = devm_kzalloc(&op->dev,
		d->dma_channels * sizeof(struct k3_dma_phy), GFP_KERNEL);
	if (d->phy == NULL)
		return -ENOMEM;

	for (i = 0; i < d->dma_channels; i++) {
		struct k3_dma_phy *p = &d->phy[i];

		p->idx = i;
		p->base = d->base + i * 0x40;
	}

	INIT_LIST_HEAD(&d->slave.channels);
	dma_cap_set(DMA_SLAVE, d->slave.cap_mask);
	dma_cap_set(DMA_MEMCPY, d->slave.cap_mask);
	d->slave.dev = &op->dev;
	d->slave.device_free_chan_resources = k3_dma_free_chan_resources;
	d->slave.device_tx_status = k3_dma_tx_status;
	d->slave.device_prep_dma_memcpy = k3_dma_prep_memcpy;
	d->slave.device_prep_slave_sg = k3_dma_prep_slave_sg;
	d->slave.device_issue_pending = k3_dma_issue_pending;
	d->slave.device_config = k3_dma_config;
	d->slave.device_pause = k3_dma_transfer_pause;
	d->slave.device_resume = k3_dma_transfer_resume;
	d->slave.device_terminate_all = k3_dma_terminate_all;
	d->slave.copy_align = DMAENGINE_ALIGN_8_BYTES;

	/* init virtual channel */
	d->chans = devm_kzalloc(&op->dev,
		d->dma_requests * sizeof(struct k3_dma_chan), GFP_KERNEL);
	if (d->chans == NULL)
		return -ENOMEM;

	for (i = 0; i < d->dma_requests; i++) {
		struct k3_dma_chan *c = &d->chans[i];

		c->status = DMA_IN_PROGRESS;
		INIT_LIST_HEAD(&c->node);
		c->vc.desc_free = k3_dma_free_desc;
		vchan_init(&c->vc, &d->slave);
	}

	/* Enable clock before accessing registers */
	ret = clk_prepare_enable(d->clk);
	if (ret < 0) {
		dev_err(&op->dev, "clk_prepare_enable failed: %d\n", ret);
		return ret;
	}

	k3_dma_enable_dma(d, true);

	ret = dma_async_device_register(&d->slave);
	if (ret)
		return ret;

	ret = of_dma_controller_register((&op->dev)->of_node,
					k3_of_dma_simple_xlate, d);
	if (ret)
		goto of_dma_register_fail;

	spin_lock_init(&d->lock);
	INIT_LIST_HEAD(&d->chan_pending);
	tasklet_init(&d->task, k3_dma_tasklet, (unsigned long)d);
	platform_set_drvdata(op, d);
	dev_info(&op->dev, "initialized\n");

	return 0;

of_dma_register_fail:
	dma_async_device_unregister(&d->slave);
	return ret;
}

static int k3_dma_remove(struct platform_device *op)
{
	struct k3_dma_chan *c, *cn;
	struct k3_dma_dev *d = platform_get_drvdata(op);

	dma_async_device_unregister(&d->slave);
	of_dma_controller_free((&op->dev)->of_node);

	list_for_each_entry_safe(c, cn, &d->slave.channels, vc.chan.device_node) {
		list_del(&c->vc.chan.device_node);
		tasklet_kill(&c->vc.task);
	}
	tasklet_kill(&d->task);
	clk_disable_unprepare(d->clk);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int k3_dma_suspend_dev(struct device *dev)
{
	struct k3_dma_dev *d = dev_get_drvdata(dev);
	u32 stat = 0;

	stat = k3_dma_get_chan_stat(d);
	if (stat) {
		dev_warn(d->slave.dev,
			"chan %d is running fail to suspend\n", stat);
		return -1;
	}
	k3_dma_enable_dma(d, false);
	clk_disable_unprepare(d->clk);
	return 0;
}

static int k3_dma_resume_dev(struct device *dev)
{
	struct k3_dma_dev *d = dev_get_drvdata(dev);
	int ret = 0;

	ret = clk_prepare_enable(d->clk);
	if (ret < 0) {
		dev_err(d->slave.dev, "clk_prepare_enable failed: %d\n", ret);
		return ret;
	}
	k3_dma_enable_dma(d, true);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(k3_dma_pmops, k3_dma_suspend_dev, k3_dma_resume_dev);

static struct platform_driver k3_pdma_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.pm	= &k3_dma_pmops,
		.of_match_table = k3_pdma_dt_ids,
	},
	.probe		= k3_dma_probe,
	.remove		= k3_dma_remove,
};

module_platform_driver(k3_pdma_driver);

MODULE_DESCRIPTION("Hisilicon k3 DMA Driver");
MODULE_ALIAS("platform:k3dma");
MODULE_LICENSE("GPL v2");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 