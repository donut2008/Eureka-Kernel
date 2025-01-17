;
}

static int octeon_lmc_edac_remove(struct platform_device *pdev)
{
	struct mem_ctl_info *mci = platform_get_drvdata(pdev);

	edac_mc_del_mc(&pdev->dev);
	edac_mc_free(mci);
	return 0;
}

static struct platform_driver octeon_lmc_edac_driver = {
	.probe = octeon_lmc_edac_probe,
	.remove = octeon_lmc_edac_remove,
	.driver = {
		   .name = "octeon_lmc_edac",
	}
};
module_platform_driver(octeon_lmc_edac_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ralf Baechle <ralf@linux-mips.org>");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              