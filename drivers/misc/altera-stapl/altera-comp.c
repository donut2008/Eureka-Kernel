ta conrruption occasionally due to a firmware bug.
	 */
	MMC_FIXUP("V10008", CID_MANFID_KINGSTON, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_TRIM_BROKEN),
	MMC_FIXUP("V10016", CID_MANFID_KINGSTON, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_TRIM_BROKEN),

	END_FIXUP
};

static int mmc_blk_probe(struct mmc_card *card)
{
	struct mmc_blk_data *md, *part_md;
	char cap_str[10];

	/*
	 * Check that the card supports the command class(es) we need.
	 */
	if (!(card->csd.cmdclass & CCC_BLOCK_READ))
		return -ENODEV;

	mmc_fixup_device(card, blk_fixups);

	md = mmc_blk_alloc(card);
	if (IS_ERR(md))
		return PTR_ERR(md);

	string_get_size((u64)get_capacity(md->disk), 512, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
	pr_info("%s: %s %s %s %s\n",
		md->disk->disk_name, mmc_card_id(card), mmc_card_name(card),
		cap_str, md->read_only ? "(ro)" : "");
	ST_LOG("%s: %s %s %s %s\n",
		md->disk->disk_name, mmc_card_id(card), mmc_card_name(card),
		cap_str, md->read_only ? "(ro)" : "");

	if (mmc_blk_alloc_parts(card, md))
		goto out;

	dev_set_drvdata(&card->dev, md);

#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
	if (card && mmc_card_sd(card))
		mmc_set_bus_resume_policy(card->host, 1);
#endif

	if (mmc_add_disk(md))
		goto out;

	list_for_each_entry(part_md, &md->part, part) {
		if (mmc_add_disk(part_md))
			goto out;
	}

	pm_runtime_set_autosuspend_delay(&card->dev, 3000);
	pm_runtime_use_autosuspend(&card->dev);

	/*
	 * Don't enable runtime PM for SD-combo cards here. Leave that
	 * decision to be taken during the SDIO init sequence instead.
	 */
	if (card->type != MMC_TYPE_SD_COMBO) {
		pm_runtime_set_active(&card->dev);
		pm_runtime_enable(&card->dev);
	}

	if (card)
		mmc_card_debug_log_sysfs_init(card);

#if defined(CONFIG_MMC_CQ_HCI) && defined(CONFIG_MMC_DATA_LOG)
	if (mmc_card_mmc(card)) {
		struct hd_struct *part;
		int i;
		md->mmc_system_start = 0;
		md->mmc_system_end = 0;
		md->mmc_sys_log_en = false;

		for (i = 1; i < 30 ; i++) {
			if (!md->disk->part_tbl)
				break;
			part = md->disk->part_tbl->part[i];
			if (!part)
				break;
			if (!strncmp(part->info->volname, "SYSTEM", 6) ||
					!strncmp(part->info->volname, "system", 6) ||
					!strncmp(part->info->volname, "SUPER", 5) ||
					!strncmp(part->info->volname, "super", 5)) {
				md->mmc_system_start = part->start_sect;
				md->mmc_system_end = part->start_sect + part->nr_sects;
				md->mmc_sys_log_en = true;
				printk("MMC data logging enabled\n");
				printk("MMC %s partition, from : %lu, to %lu\n",
					part->info->volname, md->mmc_system_start, md->mmc_system_end);
				break;
			}
		}
	}
#endif
	return 0;

 out:
	mmc_blk_remove_parts(card, md);
	mmc_blk_remove_req(md);
	return 0;
}

static void mmc_blk_remove(struct mmc_card *card)
{
	struct mmc_blk_data *md = dev_get_drvdata(&card->dev);

	mmc_blk_remove_parts(card, md);
	pm_runtime_get_sync(&card->dev);
	mmc_claim_host(card->host);
	mmc_blk_part_switch(card, md);
	mmc_release_host(card->host);
	if (card->type != MMC_TYPE_SD_COMBO)
		pm_runtime_disable(&card->dev);
	pm_runtime_put_noidle(&card->dev);
	mmc_blk_remove_req(md);
	dev_set_drvdata(&card->dev, NULL);
}

static void mmc_blk_shutdown(struct mmc_card *card)
{
	struct mmc_blk_data *part_md;
	struct mmc_blk_data *md = dev_get_drvdata(&card->dev);
	int rc;

	if (md) {
		rc = mmc_queue_suspend(&md->queue, 1);
		if (rc)
			got