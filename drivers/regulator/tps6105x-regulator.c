_handle_ce(edac_dev, 0, 0, edac_dev->ctl_name);
}

static struct cpc925_dev_info cpc925_devs[] = {
	{
	.ctl_name = CPC925_CPU_ERR_DEV,
	.init = cpc925_cpu_init,
	.exit = cpc925_cpu_exit,
	.check = cpc925_cpu_check,
	},
	{
	.ctl_name = CPC925_HT_LINK_DEV,
	.init = cpc925_htlink_init,
	.exit = cpc925_htlink_exit,
	.check = cpc925_htlink_check,
	},
	{ }
};

/*
 * Add CPU Err detection and HyperTransport Link Err detection
 * as common "edac_device", they have no corresponding device
 * nodes in the Open Firmware DTB and we have to add platform
 * devices for them. Also, they will share the MMIO with that
 * of memory controller.
 */
static void cpc925_add_edac_devices(void __iomem *vbase)
{
	struct cpc925_dev_info *dev_info;

	if (!vbase) {
		cpc925_printk(KERN_ERR, "MMIO not established yet\n");
		return;
	}

	for (dev_info = &cpc925_devs[0]; dev_info->init; dev_info++) {
		dev_info->vbase = vbase;
		dev_info->pdev = platform_device_register_simple(
					dev_info->ctl_name, 0, NULL, 0);
		if (IS_ERR(dev_info->pdev)) {
			cpc925_printk(KERN_ERR,
				"Can't register platform device for %s\n",
				dev_info->ctl_name);
			continue;
		}

		/*
		 * Don't have to allocate private structure but
		 * make use of cpc925_devs[] instead.
		 */
		dev_info->edac_idx = edac_device_alloc_index();
		dev_info->edac_dev =
			edac_device_alloc_ctl_info(0, dev_info->ctl_name,
				1, NULL, 0, 0, NULL, 0, dev_info->edac_idx);
		if (!dev_info->edac_dev) {
			cpc925_printk(KERN_ERR, "No memory for edac device\n");
			goto err1;
		}

		dev_info->edac_dev->pvt_info = dev_info;
		dev_info->edac_dev->dev = &dev_info->pdev->dev;
		dev_info->edac_dev->ctl_name = dev_info->ctl_name;
		dev_info->edac_dev->mod_name = CPC925_EDAC_MOD_STR;
		dev_info->edac_dev->dev_name = dev_name(&dev_info->pdev->dev);

		if (edac_op_state == EDAC_OPSTATE_POLL)
			dev_info->edac_dev->edac_check = dev_info->check;

		if (dev_info->init)
			dev_info->init(dev_info);

		if (edac_device_add_device(dev_info->edac_dev) > 0) {
			cpc925_printk(KERN_ERR,
				"Unable to add edac device for %s\n",
				dev_info->ctl_name);
			goto err2;
		}

		edac_dbg(0, "Successfully added edac device for %s\n",
			 dev_info->ctl_name);

		continue;

err2:
		if (dev_info->exit)
			dev_info->exit(dev_info);
		edac_device_free_ctl_info(dev_info->edac_dev);
err1:
		platform_device_unregister(dev_info->pdev);
	}
}

/*
 * Delete the common "edac_device" for CPU Err Detection
 * and HyperTransport Link Err Detection
 */
static void cpc925_del_edac_devices(void)
{
	struct cpc925_dev_info *dev_info;

	for (dev_info = &cpc925_devs[0]; dev_info->init; dev_info++) {
		if (dev_info->edac_dev) {
			edac_device_del_device(dev_info->edac_dev->dev);
			edac_device_free_ctl_info(dev_info->edac_dev);
			platform_device_unregister(dev_info->pdev);
		}

		if (dev_info->exit)
			dev_info->exit(dev_info);

		edac_dbg(0, "Successfully deleted edac device for %s\n",
			 dev_info->ctl_name);
	}
}

/* Convert current back-ground scrub rate into byte/sec bandwidth */
static int cpc925_get_sdram_scrub_rate(struct mem_ctl_info *mci)
{
	struct cpc925_mc_pdata *pdata = mci->pvt_info;
	int bw;
	u32 mscr;
	u8 si;

	mscr = __raw_readl(pdata->vbase + REG_MSCR_OFFSET);
	si = (mscr & MSCR_SI_MASK) >> MSCR_SI_SHIFT;

	edac_dbg(0, "Mem Scrub Ctrl Register 0x%x\n", mscr);

	if (((mscr & MSCR_SCRUB_MOD_MASK) != MSCR_BACKGR_SCRUB) ||
	    (si == 0)) {
		cpc925_mc_printk(mci, KERN_INFO, "Scrub mode not enabled\n");
		bw = 0;
	} else
		bw = CPC925_SCRUB_BLOCK_SIZE * 0xFA67 / si;

	return bw;
}

/* Return 0 for single channel; 1 for dual channel */
static int cpc925_mc_get_channels(void __iomem *vbase)
{
	int dual = 0;
	u32 mbcr;

	mbcr = __raw_readl(vbase + REG_MBCR_OFFSET);

	/*
	 * Dual channel only when 128-bit wide physical bus
	 * and 128-bit configuration.
	 */
	if (((mbcr & MBCR_64BITCFG_MASK) == 0) &&
	    ((mbcr & MBCR_64BITBUS_MASK) == 0))
		dual = 1;

	edac_dbg(0, "%s channel\n", (dual > 0) ? "Dual" : "Single");

	return dual;
}

static int cpc925_probe(struct platform_device *pdev)
{
	static int edac_mc_idx;
	struct mem_ctl_info *mci;
	struct edac_mc_layer layers[2];
	void __iomem *vbase;
	struct cpc925_mc_pdata *pdata;
	struct resource *r;
	int res = 0, nr_channels;

	edac_dbg(0, "%s platform device found!\n", pdev->name);

	if (!devres_open_group(&pdev->dev, cpc925_probe, GFP_KERNEL)) {
		res = -ENOMEM;
		goto out;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		cpc925_printk(KERN_ERR, "Unable to get resource\n");
		res = -ENOENT;
		goto err1;
	}

	if (!devm_request_mem_region(&pdev->dev,
				     r->start,
				     resource_size(r),
				     pdev->name)) {
		cpc925_printk(KERN_ERR, "Unable to request mem region\n"