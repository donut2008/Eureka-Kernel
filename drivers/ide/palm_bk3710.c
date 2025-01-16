_has_cap(DMA_XOR_VAL, dma_chan->device->cap_mask))
		goto free_resources;

	/* zero sum the sources with the destintation page */
	for (i = 0; i < IOP_ADMA_NUM_SRC_TEST; i++)
		zero_sum_srcs[i] = xor_srcs[i];
	zero_sum_srcs[i] = dest;

	zero_sum_result = 1;

	for (i = 0; i < IOP_ADMA_NUM_SRC_TEST + 1; i++)
		dma_srcs[i] = dma_map_page(dma_chan->device->dev,
					   zero_sum_srcs[i], 0, PAGE_SIZE,
					   DMA_TO_DEVICE);
	tx = iop_adma_prep_dma_xor_val(dma_chan, dma_srcs,
				       IOP_ADMA_NUM_SRC_TEST + 1, PAGE_SIZE,
				       &zero_sum_result,
				       DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

	cookie = iop_adma_tx_submit(tx);
	iop_adma_issue_pending(dma_chan);
	msleep(8);

	if (iop_adma_status(dma_chan, cookie, NULL) != DMA_COMPLETE) {
		dev_err(dma_chan->device->dev,
			"Self-test zero sum timed out, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	if (zero_sum_result != 0) {
		dev_err(dma_chan->device->dev,
			"Self-test zero sum failed compare, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	/* test for non-zero parity sum */
	zero_sum_result = 0;
	for (i = 0; i < IOP_ADMA_NUM_SRC_TEST + 1; i++)
		dma_srcs[i] = dma_map_page(dma_chan->device->dev,
					   zero_sum_srcs[i], 0, PAGE_SIZE,
					   DMA_TO_DEVICE);
	tx = iop_adma_prep_dma_xor_val(dma_chan, dma_srcs,
				       IOP_ADMA_NUM_SRC_TEST + 1, PAGE_SIZE,
				       &zero_sum_result,
				       DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

	cookie = iop_adma_tx_submit(tx);
	iop_adma_issue_pending(dma_chan);
	msleep(8);

	if (iop_adma_status(dma_chan, cookie, NULL) != DMA_COMPLETE) {
		dev_err(dma_chan->device->dev,
			"Self-test non-zero sum timed out, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	if (zero_sum_result != 1) {
		dev_err(dma_chan->device->dev,
			"Self-test non-zero sum failed compare, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

free_resources:
	iop_adma_free_chan_resources(dma_chan);
out:
	src_idx = IOP_ADMA_NUM_SRC_TEST;
	while (src_idx--)
		__free_page(xor_srcs[src_idx]);
	__free_page(dest);
	return err;
}

#ifdef CONFIG_RAID6_PQ
static int
iop_adma_pq_zero_sum_self_test(struct iop_adma_device *device)
{
	/* combined sources, software pq results, and extra hw pq results */
	struct page *pq[IOP_ADMA_NUM_SRC_TEST+2+2];
	/* ptr to the extra hw pq buffers defined above */
	struct page **pq_hw = &pq[IOP_ADMA_NUM_SRC_TEST+2];
	/* address conversion buffers (dma_map / page_address) */
	void *pq_sw[IOP_ADMA_NUM_SRC_TEST+2];
	dma_addr_t pq_src[IOP_ADMA_NUM_SRC_TEST+2];
	dma_addr_t *pq_dest = &pq_src[IOP_ADMA_NUM_SRC_TEST];

	int i;
	struct dma_async_tx_descriptor *tx;
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;
	u32 zero_sum_result;
	int err = 0;
	struct device *dev;

	dev_dbg(device->common.dev, "%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(pq); i++) {
		pq[i] = alloc_page(GFP_KERNEL);
		if (!pq[i]) {
			while (i--)
				__free_page(pq[i]);
			return -ENOMEM;
		}
	}

	/* Fill in src buffers */
	for (i = 0; i < IOP_ADMA_NUM_SRC_TEST; i++) {
		pq_sw[i] = page_address(pq[i]);
		memset(pq_sw[i], 0x11111111 * (1<<i), PAGE_SIZE);
	}
	pq_sw[i] = page_address(pq[i]);
	pq_sw[i+1] = page_address(pq[i+1]);

	dma_chan = container_of(device->common.channels.next,
				struct dma_chan,
				device_node);
	if (iop_adma_alloc_chan_resources(dma_chan) < 1) {
		err = -ENODEV;
		goto out;
	}

	dev = dma_chan->device->dev;

	/* initialize the dests */
	memset(page_address(pq_hw[0]), 0 , PAGE_SIZE);
	memset(page_address(pq_hw[1]), 0 , PAGE_SIZE);

	/* test pq */
	pq_dest[0] = dma_map_page(dev, pq_hw[0], 0, PAGE_SIZE, DMA_FROM_DEVICE);
	pq_dest[1] = dma_map_page(dev, pq_hw[1], 0, PAGE_SIZE, DMA_FROM_DEVICE);
	for (i = 0; i < IOP_ADMA_NUM_SRC_TEST; i++)
		pq_src[i] = dma_map_page(dev, pq[i], 0, PAGE_SIZE,
					 DMA_TO_DEVICE);

	tx = iop_adma_prep_dma_pq(dma_chan, pq_dest, pq_src,
				  IOP_ADMA_NUM_SRC_TEST, (u8 *)raid6_gfexp,
				  PAGE_SIZE,
				  DMA_PREP_INTERRUPT |
				  DMA_CTRL_ACK);

	cookie = iop_adma_tx_submit(tx);
	iop_adma_issue_pending(dma_chan);
	msleep(8);

	if (iop_adma_status(dma_chan, cookie, NULL) !=
		DMA_COMPLETE) {
		dev_err(dev, "Self-test pq timed out, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	raid6_call.gen_syndrome(IOP_ADMA_NUM_SRC_TEST+2, PAGE_SIZE, pq_sw);

	if (memcmp(pq_sw[IOP_ADMA_NUM_SRC_TEST],
		   page_address(pq_hw[0]), PAGE_SIZE) != 0) {
		dev_err(dev, "Self-test p failed compare, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}
	if (memcmp(pq_sw[IOP_ADMA_NUM_SRC_TEST+1],
		   page_address(pq_hw[1]), PAGE_SIZE) != 0) {
		dev_err(dev, "Self-test q failed compare, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	/* test correct zero sum using the software generated pq values */
	for (i = 0; i < IOP_ADMA_NUM_SRC_TEST + 2; i++)
		pq_src[i] = dma_map_page(dev, pq[i], 0, PAGE_SIZE,
					 DMA_TO_DEVICE);

	zero_sum_result = ~0;
	tx = iop_adma_prep_dma_pq_val(dma_chan, &pq_src[IOP_ADMA_NUM_SRC_TEST],
				      pq_src, IOP_ADMA_NUM_SRC_TEST,
				      raid6_gfexp, PAGE_SIZE, &zero_sum_result,
				      DMA_PREP_INTERRUPT|DMA_CTRL_ACK);

	cookie = iop_adma_tx_submit(tx);
	iop_adma_issue_pending(dma_chan);
	msleep(8);

	if (iop_adma_status(dma_chan, cookie, NULL) !=
		DMA_COMPLETE) {
		dev_err(dev, "Self-test pq-zero-sum timed out, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	if (zero_sum_result != 0) {
		dev_err(dev, "Self-test pq-zero-sum failed to validate: %x\n",
			zero_sum_result);
		err = -ENODEV;
		goto free_resources;
	}

	/* test incorrect zero sum */
	i = IOP_ADMA_NUM_SRC_TEST;
	memset(pq_sw[i] + 100, 0, 100);
	memset(pq_sw[i+1] + 200, 0, 200);
	for (i = 0; i < IOP_ADMA_NUM_SRC_TEST + 2; i++)
		pq_src[i] = dma_map_page(dev, pq[i], 0, PAGE_SIZE,
					 DMA_TO_DEVICE);

	zero_sum_result = 0;
	tx = iop_adma_prep_dma_pq_val(dma_chan, &pq_src[IOP_ADMA_NUM_SRC_TEST],
				      pq_src, IOP_ADMA_NUM_SRC_TEST,
				      raid6_gfexp, PAGE_SIZE, &zero_sum_result,
				      DMA_PREP_INTERRUPT|DMA_CTRL_ACK);

	cookie = iop_adma_tx_submit(tx);
	iop_adma_issue_pending(dma_chan);
	msleep(8);

	if (iop_adma_status(dma_chan, cookie, NULL) !=
		DMA_COMPLETE) {
		dev_err(dev, "Self-test !pq-zero-sum timed out, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	if (zero_sum_result != (SUM_CHECK_P_RESULT | SUM_CHECK_Q_RESULT)) {
		dev_err(dev, "Self-test !pq-zero-sum failed to validate: %x\n",
			zero_sum_result);
		err = -ENODEV;
		goto free_resources;
	}

free_resources:
	iop_adma_free_chan_resources(dma_chan);
out:
	i = ARRAY_SIZE(pq);
	while (i--)
		__free_page(pq[i]);
	return err;
}
#endif

static int iop_adma_remove(struct platform_device *dev)
{
	struct iop_adma_device *device = platform_get_drvdata(dev);
	struct dma_chan *chan, *_chan;
	struct iop_adma_chan *iop_chan;
	struct iop_adma_platform_data *plat_data = dev_get_platdata(&dev->dev);

	dma_async_device_unregister(&device->common);

	dma_free_coherent(&dev->dev, plat_data->pool_size,
			device->dma_desc_pool_virt, device->dma_desc_pool);

	list_for_each_entry_safe(chan, _chan, &device->common.channels,
				device_node) {
		iop_chan = to_iop_adma_chan(chan);
		list_del(&chan->device_node);
		kfree(iop_chan);
	}
	kfree(device);

	return 0;
}

static int iop_adma_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret = 0, i;
	struct iop_adma_device *adev;
	struct iop_adma_chan *iop_chan;
	struct dma_device *dma_dev;
	struct iop_adma_platform_data *plat_data = dev_get_platdata(&pdev->dev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	if (!devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res), pdev->name))
		return -EBUSY;

	adev = kzalloc(sizeof(*adev), GFP_KERNEL);
	if (!adev)
		return -ENOMEM;
	dma_dev = &adev->common;

	/* allocate coherent memory for hardware descriptors
	 * note: writecombine gives slightly better performance, but
	 * requires that we explicitly flush the writes
	 */
	adev->dma_desc_pool_virt = dma_alloc_writecombine(&pdev->dev,
							  plat_data->pool_size,
							  &adev->dma_desc_pool,
							  GFP_KERNEL);
	if (!adev->dma_desc_pool_virt) {
		ret = -ENOMEM;
		goto err_free_adev;
	}

	dev_dbg(&pdev->dev, "%s: allocated descriptor pool virt %p phys %p\n",
		__func__, adev->dma_desc_pool_virt,
		(void *) adev->dma_desc_pool);

	adev->id = plat_data->hw_id;

	/* discover transaction capabilites from the platform data */
	dma_dev->cap_mask = plat_data->cap_mask;

	adev->pdev = pdev;
	platform_set_drvdata(pdev, adev);

	INIT_LIST_HEAD(&dma_dev->channels);

	/* set base routines */
	dma_dev->device_alloc_chan_resources = iop_adma_alloc_chan_resources;
	dma_dev->device_free_chan_resources = iop_adma_free_chan_resources;
	dma_dev->device_tx_status = iop_adma_status;
	dma_dev->device_issue_pending = iop_adma_issue_pending;
	dma_dev->dev = &pdev->dev;

	/* set prep routines based on capability */
	if (dma_has_cap(DMA_MEMCPY, dma_dev->cap_mask))
		dma_dev->device_prep_dma_memcpy = iop_adma_prep_dma_memcpy;
	if (dma_has_cap(DMA_XOR, dma_dev->cap_mask)) {
		dma_dev->max_xor = iop_adma_get_max_xor();
		dma_dev->device_prep_dma_xor = iop_adma_prep_dma_xor;
	}
	if (dma_has_cap(DMA_XOR_VAL, dma_dev->cap_mask))
		dma_dev->device_prep_dma_xor_val =
			iop_adma_prep_dma_xor_val;
	if (dma_has_cap(DMA_PQ, dma_dev->cap_mask)) {
		dma_set_maxpq(dma_dev, iop_adma_get_max_pq(), 0);
		dma_dev->device_prep_dma_pq = iop_adma_prep_dma_pq;
	}
	if (dma_has_cap(DMA_PQ_VAL, dma_dev->cap_mask))
		dma_dev->device_prep_dma_pq_val =
			iop_adma_prep_dma_pq_val;
	if (dma_has_cap(DMA_INTERRUPT, dma_dev->cap_mask))
		dma_dev->device_prep_dma_interrupt =
			iop_adma_prep_dma_interrupt;

	iop_chan = kzalloc(sizeof(*iop_chan), GFP_KERNEL);
	if (!iop_chan) {
		ret = -ENOMEM;
		goto err_free_dma;
	}
	iop_chan->device = adev;

	iop_chan->mmr_base = devm_ioremap(&pdev->dev, res->start,
					resource_size(res));
	if (!iop_chan->mmr_base) {
		ret = -ENOMEM;
		goto err_free_iop_chan;
	}
	tasklet_init(&iop_chan->irq_tasklet, iop_adma_tasklet, (unsigned long)
		iop_chan);

	/* clear errors before enabling interrupts */
	iop_adma_device_clear_err_status(iop_chan);

	for (i = 0; i < 3; i++) {
		irq_handler_t handler[] = { iop_adma_eot_handler,
					iop_adma_eoc_handler,
					iop_adma_err_handler };
		int irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			ret = -ENXIO;
			goto err_free_iop_chan;
		} else {
			ret = devm_request_irq(&pdev->dev, irq,
					handler[i], 0, pdev->name, iop_chan);
			if (ret)
				goto err_free_iop_chan;
		}
	}

	spin_lock_init(&iop_chan->lock);
	INIT_LIST_HEAD(&iop_chan->chain);
	INIT_LIST_HEAD(&iop_chan->all_slots);
	iop_chan->common.device = dma_dev;
	dma_cookie_init(&iop_chan->common);
	list_add_tail(&iop_chan->common.device_node, &dma_dev->channels);

	if (dma_has_cap(DMA_MEMCPY, dma_dev->cap_mask)) {
		ret = iop_adma_memcpy_self_test(adev);
		dev_dbg(&pdev->dev, "m