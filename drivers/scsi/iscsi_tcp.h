nnel to dma engine */
	list_add_tail(&chan->chan.device_node, &pdev->device.channels);

	return 0;
}

static const struct of_device_id mmp_pdma_dt_ids[] = {
	{ .compatible = "marvell,pdma-1.0", },
	{}
};
MODULE_DEVICE_TABLE(of, mmp_pdma_dt_ids);

static struct dma_chan *mmp_pdma_dma_xlate(struct of_phandle_args *dma_spec,
					   struct of_dma *ofdma)
{
	struct mmp_pdma_device *d = ofdma->of_dma_data;
	struct dma_chan *chan;

	chan = dma_get_any_slave_channel(&d->device);
	if (!chan)
		return NULL;

	to_mmp_pdma_chan(chan)->drcmr = dma_spec->args[0];

	return chan;
}

static int mmp_pdma_probe(struct platform_device *op)
{
	struct mmp_pdma_device *pdev;
	const struct of_device_id *of_id;
	struct mmp_dma_platdata *pdata = dev_get_platdata(&op->dev);
	struct resource *iores;
	int i, ret, irq = 0;
	int dma_channels = 0, irq_num = 0;
	const enum dma_slave_buswidth widths =
		DMA_SLAVE_BUSWIDTH_1_BYTE   | DMA_SLAVE_BUSWIDTH_2_BYTES |
		DMA_SLAVE_BUSWIDTH_4_BYTES;

	pdev = devm_kzalloc(&op->dev, sizeof(*pdev), GFP_KERNEL);
	if (!pdev)
		return -ENOMEM;

	pdev->dev = &op->dev;

	spin_lock_init(&pdev->phy_lock);

	iores = platform_get_resource(op, IORESOURCE_MEM, 0);
	pdev->base = devm_ioremap_resource(pdev->dev, iores);
	if (IS_ERR(pdev->base))
		return PTR_ERR(pdev->base);

	of_id = of_match_device(mmp_pdma_dt_ids, pdev->dev);
	if (of_id)
		of_property_read_u32(pdev->dev->of_node, "#dma-channels",
				     &dma_channels);
	else if (pdata && pdata->dma_channels)
		dma_channels = pdata->dma_channels;
	else
		dma_channels = 32;	/* default 32 channel */
	pdev->dma_channels = dma_channels;

	for (i = 0; i < dma_channels; i++) {
		if (platform_get_irq(op, i) > 0)
			irq_num++;
	}

	pdev->phy = devm_kcalloc(pdev->dev, dma_channels, sizeof(*pdev->phy),
				 GFP_KERNEL);
	if (pdev->phy == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&pdev->device.channels);

	if (irq_num != dma_channels) {
		/* all chan share one irq, 