) {
		mmc_queue_resume(&part_md->queue);
	}
out:
	return rc;
}

static int mmc_blk_resume(struct device *dev)
{
	struct mmc_blk_data *part_md;
	struct mmc_blk_data *md = dev_get_drvdata(dev);

	if (md) {
		/*
		 * Resume involves the card going into idle state,
		 * so current partition is always the main one.
		 */
		md->part_curr = md->part_type;
		mmc_queue_resume(&md->queue);
		list_for_each_entry(part_md, &md->part, part) {
			mmc_queue_resume(&part_md->queue);
		}
	}
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mmc_blk_pm_ops, mmc_blk_suspend, mmc_blk_resume);

static struct mmc_driver mmc_driver = {
	.drv		= {
		.name	= "mmcblk",
		.pm	= &mmc_blk_pm_ops,
	},
	.probe		= mmc_blk_probe,
	.remove		= mmc_blk_remove,
	.shutdown	= mmc_blk_shutdown,
};

static int __init mmc_blk_init(void)
{
	int res;

	if (perdev_minors != CONFIG_MMC_BLOCK_MINORS)
		pr_info("mmcblk: using %d minors per device\n", perdev_minors);

	max_devices = min(MAX_DEVICES, (1 << MINORBITS) / perdev_minors);

	res = register_blkdev(MMC_BLOCK_MAJOR, "mmc");
	if (res)
		goto out;

	res = mmc_register_driver(&mmc_driver);
	if (res)
		got