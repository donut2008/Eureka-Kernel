pped(mq->queue) && !ctx_info->active_reqs)
		complete(&mq->cmdq_shutdown_complete);

	return err ? 1 : 0;
}

static int mmc_blk_issue_discard_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	unsigned int from, nr, arg;
	int err = 0, type = MMC_BLK_DISCARD;

	if (!mmc_can_erase(card)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (mmc_can_discard(card))
		arg = MMC_DISCARD_ARG;
	else if (mmc_can_trim(card))
		arg = MMC_TRIM_ARG;
	else
		arg = MMC_ERASE_ARG;
retry:
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_TRIM_ARG ?
				 INAND_CMD38_ARG_TRIM :
				 INAND_CMD38_ARG_ERASE,
				 card->ext_csd.generic_cmd6_time);
		if (err)
			goto out;
	}
	err = mmc_erase(card, from, nr, arg);
out:
	if (err == -EIO && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);
	blk_end_request(req, err, blk_rq_bytes(req));

	return err ? 0 : 1;
}

static int mmc_blk_cmdq_issue_secdiscard_rq(struct mmc_queue *mq,
				       struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	struct mmc_cmdq_req *cmdq_req = NULL;
	unsigned int from, nr, arg;
	struct mmc_host *host = card->host;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;
	int err = 0;

	if (!(mmc_can_secure_erase_trim(card))) {
		err = -EOPNOTSUPP;
		goto out;
	}

#ifdef CONFIG_MMC_CMDQ_DEBUG
	/* cq debug */
	exynos_ss_printk("[CQ] I_SD: tag = %d, flag = 0x%lx, active = 0x%lx\n",
				req->tag, ctx_info->curr_state,
				ctx_info->active_reqs);
#endif
	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (mmc_can_trim(card) && !mmc_erase_group_aligned(card, from, nr))
		arg = MMC_SECURE_TRIM1_ARG;
	else
		arg = MMC_SECURE_ERASE_ARG;

	cmdq_req = mmc_blk_cmdq_prep_discard_req(mq, req);
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		__mmc_switch_cmdq_mode(cmdq_req->mrq.cmd,
				EXT_CSD_CMD_SET_NORMAL,
				INAND_CMD38_ARG_EXT_CSD,
				arg == MMC_SECURE_TRIM1_ARG ?
				INAND_CMD38_ARG_SECTRIM1 :
				INAND_CMD38_ARG_SECERASE,
				0, true, false);
		err = mmc_cmdq_wait_for_dcmd(card->host, cmdq_req);
		if (err)
			goto clear_dcmd;
	}

	err = mmc_cmdq_erase(cmdq_req, card, from, nr, arg);
	if (err)
		goto clear_dcmd;

	if (arg == MMC_SECURE_TRIM1_ARG) {
		if (card->quirks & MMC_QUIRK_INAND_CMD38) {
			__mmc_switch_cmdq_mode(cmdq_req->mrq.cmd,
					EXT_CSD_CMD_SET_NORMAL,
					INAND_CMD38_ARG_EXT_CSD,
					INAND_CMD38_ARG_SECTRIM2,
					0, true, false);
			err = mmc_cmdq_wait_for_dcmd(card->host, cmdq_req);
			if (err)
				goto clear_dcmd;
		}

		err = mmc_cmdq_erase(cmdq_req, card, from, nr,
				MMC_SECURE_TRIM2_ARG);
	}
clear_dcmd:
	/* clear pending request */
	if (cmdq_req) {
		BUG_ON(!test_and_clear_bit(cmdq_req->tag,
					   &ctx_info->active_reqs));
		clear_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx_info->curr_state);
	}
out:
	blk_end_request(req, err, blk_rq_bytes(req));
	wake_up(&ctx_info->wait);
	mmc_put_card(card);
	return err ? 1 : 0;
}

static int mmc_blk_issue_secdiscard_rq(struct mmc_queue *mq,
				       struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	unsigned int from, nr, arg;
	int err = 0, type = MMC_BLK_SECDISCARD;

	if (!(mmc_can_secure_erase_trim(card))) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (mmc_can_trim(card) && !mmc_erase_group_aligned(card, from, nr))
		arg = MMC_SECURE_TRIM1_ARG;
	else
		arg = MMC_SECURE_ERASE_ARG;

retry:
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_SECURE_TRIM1_ARG ?
				 INAND_CMD38_ARG_SECTRIM1 :
				 INAND_CMD38_ARG_SECERASE,
				 card->ext_csd.generic_cmd6_time);
		if (err)
			goto out_retry;
	}

	err = mmc_erase(card, from, nr, arg);
	if (err == -EIO)
		goto out_retry;
	if (err)
		goto out;

	if (arg == MMC_SECURE_TRIM1_ARG) {
		if (card->quirks & MMC_QUIRK_INAND_CMD38) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 INAND_CMD38_ARG_EXT_CSD,
					 INAND_CMD38_ARG_SECTRIM2,
					 card->ext_csd.generic_cmd6_time);
			if (err)
				goto out_retry;
		}

		err = mmc_erase(card, from, nr, MMC_SECURE_TRIM2_ARG);
		if (err == -EIO)
			goto out_retry;
		if (err)
			goto out;
	}

out_retry:
	if (err && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);
out:
	blk_end_request(req, err, blk_rq_bytes(req));

	return err ? 0 : 1;
}

static int mmc_blk_issue_flush(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	int ret = 0;

	if (!req)
		return 0;

	if (req->cmd_flags & REQ_BARRIER) {
		/*
		 * If eMMC cache flush policy is set to 1, then the device
		 * shall flush the requests in First-In-First-Out (FIFO) order.
		 * In this case, as per spec, the host must not send any cache
		 * barrier requests as they are redundant and add unnecessary
		 * overhead to both device and host.
		 */
		if (card->ext_csd.cache_flush_policy & 1)
			goto end_req;

		/*
		 * In case barrier is not supported or enabled in the device,
		 * use flush as a fallback option.
		 */
		ret = mmc_cache_barrier(card);
		if (ret)
			ret = mmc_flush_cache(card);
	 } else if (req->cmd_flags & REQ_FLUSH) {
		ret = mmc_flush_cache(card);
	 }
	if (ret == -ENODEV) {
		pr_err("%s: %s: restart mmc card",
				req->rq_disk->disk_name, __func__);
		if (mmc_blk_reset(md, card->host, MMC_BLK_FLUSH))
			pr_err("%s: %s: fail to restart mmc",
				req->rq_disk->disk_name, __func__);
		else
			mmc_blk_reset_success(md, MMC_BLK_FLUSH);
	}

	if (ret) {
		pr_err("%s: %s: notify flush error to upper layers",
				req->rq_disk->disk_name, __func__);
		ret = -EIO;
	}

#ifdef CONFIG_MMC_SIMULATE_MAX_SPEED
	else if (atomic_read(&mq->cache_size)) {
		long used = mmc_blk_cache_used(mq, jiffies);

		if (used) {
			int speed = atomic_read(&mq->max_write_speed);

			if (speed_valid(speed)) {
				unsigned long msecs = jiffies_to_msecs(
					size_and_speed_to_jiffies(
						used, speed));
				if (msecs)
					msleep(msecs);
			}
		}
	}
#endif
end_req:
	blk_end_request_all(req, ret);

	return ret ? 0 : 1;
}

/*
 * Reformat current write as a reliable write, supporting
 * both legacy and the enhanced reliable write MMC cards.
 * In each transfer we'll handle only as much as a single
 * reliable write can handle, thus finish the request in
 * partial completions.
 */
static inline void mmc_apply_rel_rw(struct mmc_blk_request *brq,
				    struct mmc_card *card,
				    struct request *req)
{
	if (!(card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN)) {
		/* Legacy mode imposes restrictions on transfers. */
		if (!IS_ALIGNED(brq->cmd.arg, card->ext_csd.rel_sectors))
			brq->data.blocks = 1;

		if (brq->data.blocks > card->ext_csd.rel_sectors)
			brq->data.blocks = card->ext_csd.rel_sectors;
		else if (brq->data.blocks < card->ext_csd.rel_sectors)
			brq->data.blocks = 1;
	}
}

static int mmc_blk_err_check(struct mmc_card *card,
			     struct mmc_async_req *areq)
{
	struct mmc_queue_req *mq_mrq = container_of(areq, struct mmc_queue_req,
						    mmc_active);
	struct mmc_blk_request *brq = &mq_mrq->brq;
	struct request *req = mq_mrq->req;
	int need_retune = card->host->need_retune;
	int ecc_err = 0, gen_err = 0;

	/*
	 * sbc.error indicates a problem with the set block count
	 * command.  No data will have been transferred.
	 *
	 * cmd.error indicates a problem with the r/w command.  No
	 * data will have been transferred.
	 *
	 * stop.error indicates a problem with the stop command.  Data
	 * may have been transferred, or may still be transferring.
	 */
	if (brq->sbc.error || brq->cmd.error || brq->stop.error ||
	    brq->data.error) {
		switch (mmc_blk_cmd_recovery(card, req, brq, &ecc_err, &gen_err)) {
		case ERR_RETRY:
			return MMC_BLK_RETRY;
		case ERR_ABORT:
			return MMC_BLK_ABORT;
		case ERR_NOMEDIUM:
			return MMC_BLK_NOMEDIUM;
		case ERR_CONTINUE:
			break;
		}
	}

	/*
	 * Check for errors relating to the execution of the
	 * initial command - such as address errors.  No data
	 * has been transferred.
	 */
	if (brq->cmd.resp[0] & CMD_ERRORS) {
		pr_err("%s: r/w command failed, status = %#x\n",
		       req->rq_disk->disk_name, brq->cmd.resp[0]);
		mmc_card_error_logging(card, brq, brq->cmd.resp[0]);
		return MMC_BLK_ABORT;
	}

	/*
	 * Everything else is either success, or a data error of some
	 * kind.  If it was a write, we may have transitioned to
	 * program mode, which we have to wait for it to complete.
	 */
	if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ) {
		int err;

		/* Check stop command response */
		if (brq->stop.resp[0] & R1_ERROR) {
			pr_err("%s: %s: general error sending stop command, stop cmd response %#x\n",
			       req->rq_disk->disk_name, __func__,
			       brq->stop.resp[0]);
			gen_err = 1;
		}

		err = card_busy_detect(card, MMC_BLK_TIMEOUT_MS, false, req,
					&gen_err);
		if (err)
			return MMC_BLK_CMD_ERR;
	}

	if (brq->data.error) {
		if (need_retune && !brq->retune_retry_done) {
			pr_info("%s: retrying because a re-tune was needed\n",
				req->rq_disk->disk_name);
			brq->retune_retry_done = 1;
			return MMC_BLK_RETRY;
		}
		pr_err("%s: error %d transferring data, sector %u, nr %u, cmd response %#x, card status %#x\n",
		       req->rq_disk->disk_name, brq->data.error,
		       (unsigned)blk_rq_pos(req),
		       (unsigned)blk_rq_sectors(req),
		       brq->cmd.resp[0], brq->stop.resp[0]);

		if (rq_data_dir(req) == READ) {
			if (ecc_err)
				return MMC_BLK_ABORT;
			if (mmc_card_sd(card))
				return MMC_BLK_ABORT;
			return MMC_BLK_DATA_ERR;
		} else {
			return MMC_BLK_CMD_ERR;
		}
	}

	if (!brq->data.bytes_xfered)
		return MMC_BLK_RETRY;

	if (mmc_packed_cmd(mq_mrq->cmd_type)) {
		if (unlikely(brq->data.blocks << 9 != brq->data.bytes_xfered))
			return MMC_BLK_PARTIAL;
		else
			return MMC_BLK_SUCCESS;
	}

	if (blk_rq_bytes(req) != brq->data.bytes_xfered)
		return MMC_BLK_PARTIAL;

	return MMC_BLK_SUCCESS;
}

static int mmc_blk_packed_err_check(struct mmc_card *card,
				    struct mmc_async_req *areq)
{
	struct mmc_queue_req *mq_rq = container_of(areq, struct mmc_queue_req,
			mmc_active);
	struct request *req = mq_rq->req;
	struct mmc_packed *packed = mq_rq->packed;
	int err, check, status;
	u8 *ext_csd;

	BUG_ON(!packed);

	packed->retries--;
	check = mmc_blk_err_check(card, areq);
	err = get_card_status(card, &status, 0);
	if (err) {
		pr_err("%s: error %d sending status command\n",
		       req->rq_disk->disk_name, err);
		return MMC_BLK_ABORT;
	}

	if (status & R1_EXCEPTION_EVENT) {
		err = mmc_get_ext_csd(card, &ext_csd);
		if (err) {
			pr_err("%s: error %d sending ext_csd\n",
			       req->rq_disk->disk_name, err);
			return MMC_BLK_ABORT;
		}

		if ((ext_csd[EXT_CSD_EXP_EVENTS_STATUS] &
		     EXT_CSD_PACKED_FAILURE) &&
		    (ext_csd[EXT_CSD_PACKED_CMD_STATUS] &
		     EXT_CSD_PACKED_GENERIC_ERROR)) {
			if (ext_csd[EXT_CSD_PACKED_CMD_STATUS] &
			    EXT_CSD_PACKED_INDEXED_ERROR) {
				packed->idx_failure =
				  ext_csd[EXT_CSD_PACKED_FAILURE_INDEX] - 1;
				check = MMC_BLK_PARTIAL;
			}
			pr_err("%s: packed cmd failed, nr %u, sectors %u, "
			       "failure index: %d\n",
			       req->rq_disk->disk_name, packed->nr_entries,
			       packed->blocks, packed->idx_failure);
		}
		kfree(ext_csd);
	}

	return check;
}

static void mmc_blk_rw_rq_prep(struct mmc_queue_req *mqrq,
			       struct mmc_card *card,
			       int disable_multi,
			       struct mmc_queue *mq)
{
	u32 readcmd, writecmd;
	struct mmc_blk_request *brq = &mqrq->brq;
	struct request *req = mqrq->req;
	struct mmc_blk_data *md = mq->data;
	bool do_data_tag;

	/*
	 * Reliable writes are used to implement Forced Unit Access and
	 * are supported only on MMCs.
	 */
	bool do_rel_wr = (req->cmd_flags & REQ_FUA) &&
		(rq_data_dir(req) == WRITE) &&
		(md->flags & MMC_BLK_REL_WR);

	memset(brq, 0, sizeof(struct mmc_blk_request));
	brq->mrq.cmd = &brq->cmd;
	brq->mrq.data = &brq->data;

	brq->cmd.arg = blk_rq_pos(req);
	if (!mmc_card_blockaddr(card))
		brq->cmd.arg <<= 9;
	brq->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	brq->data.blksz = 512;
	brq->stop.opcode = MMC_STOP_TRANSMISSION;
	brq->stop.arg = 0;
	brq->data.blocks = blk_rq_sectors(req);

	/*
	 * The block layer doesn't support all sector count
	 * restrictions, so we need to be prepared for too big
	 * requests.
	 */
	if (brq->data.blocks > card->host->max_blk_count)
		brq->data.blocks = card->host->max_blk_count;

	if (brq->data.blocks > 1) {
		/*
		 * After a read error, we redo the request one sector
		 * at a time in order to accurately determine which
		 * sectors can be read successfully.
		 */
		if (disable_multi)
			brq->data.blocks = 1;

		/*
		 * Some controllers have HW issues while operating
		 * in multiple I/O mode
		 */
		if (card->host->ops->multi_io_quirk)
			brq->data.blocks = card->host->ops->multi_io_quirk(card,
						(rq_data_dir(req) == READ) ?
						MMC_DATA_READ : MMC_DATA_WRITE,
						brq->data.blocks);
	}

	if (brq->data.blocks > 1 || do_rel_wr) {
		/* SPI multiblock writes terminate using a special
		 * token, not a STOP_TRANSMISSION request.
		 */
		if (!mmc_host_is_spi(card->host) ||
		    rq_data_dir(req) == READ)
			brq->mrq.stop = &brq->stop;
		readcmd = MMC_READ_MULTIPLE_BLOCK;
		writecmd = MMC_WRITE_MULTIPLE_BLOCK;
	} else {
		brq->mrq.stop = NULL;
		readcmd = MMC_READ_SINGLE_BLOCK;
		writecmd = MMC_WRITE_BLOCK;
	}
	if (rq_data_dir(req) == READ) {
		brq->cmd.opcode = readcmd;
		brq->data.flags |= MMC_DATA_READ;
		if (brq->mrq.stop)
			brq->stop.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 |
					MMC_CMD_AC;
	} else {
		brq->cmd.opcode = writecmd;
		brq->data.flags |= MMC_DATA_WRITE;
		if (brq->mrq.stop)
			brq->stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B |
					MMC_CMD_AC;
	}

	if (do_rel_wr)
		mmc_apply_rel_rw(brq, card, req);

	/*
	 * Data tag is used only during writing meta data to speed
	 * up write and any subsequent read of this meta data
	 */
	do_data_tag = (card->ext_csd.data_tag_unit_size) &&
		(req->cmd_flags & REQ_META) &&
		(rq_data_dir(req) == WRITE) &&
		((brq->data.blocks * brq->data.blksz) >=
		 card->ext_csd.data_tag_unit_size);

	/*
	 * Pre-defined multi-block transfers are preferable to
	 * open ended-ones (and necessary for reliable writes).
	 * However, it is not sufficient to just send CMD23,
	 * and avoid the final CMD12, as on an error condition
	 * CMD12 (stop) needs to be sent anyway. This, coupled
	 * with Auto-CMD23 enhancements provided by some
	 * hosts, means that the complexity of dealing
	 * with this is best left to the host. If CMD23 is
	 * supported by card and host, we'll fill sbc in and let
	 * the host deal with handling it correctly. This means
	 * that for hosts that don't expose MMC_CAP_CMD23, no
	 * change of behavior will be observed.
	 *
	 * N.B: Some MMC cards experience perf degradation.
	 * We'll avoid using CMD23-bounded multiblock writes for
	 * these, while retaining features like reliable writes.
	 */
	if ((md->flags & MMC_BLK_CMD23) && mmc_op_multi(brq->cmd.opcode) &&
	    (do_rel_wr || !(card->quirks & MMC_QUIRK_BLK_NO_CMD23) ||
	     do_data_tag)) {
		brq->sbc.opcode = MMC_SET_BLOCK_COUNT;
		brq->sbc.arg = brq->data.blocks |
			(do_rel_wr ? (1 << 31) : 0) |
			(do_data_tag ? (1 << 29) : 0);
		brq->sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;
		brq->mrq.sbc = &brq->sbc;
	}

	mmc_set_data_timeout(&brq->data, card);

	brq->data.sg = mqrq->sg;
	brq->data.sg_len = mmc_queue_map_sg(mq, mqrq);

	/*
	 * Adjust the sg list so it is the same size as the
	 * request.
	 */
	if (brq->data.blocks != blk_rq_sectors(req)) {
		int i, data_size = brq->data.blocks << 9;
		struct scatterlist *sg;

		for_each_sg(brq->data.sg, sg, brq->data.sg_len, i) {
			data_size -= sg->length;
			if (data_size <= 0) {
				sg->length += data_size;
				i++;
				break;
			}
		}
		brq->data.sg_len = i;
	}

	mqrq->mmc_active.mrq = &brq->mrq;
	mqrq->mmc_active.err_check = mmc_blk_err_check;

	mmc_queue_bounce_pre(mqrq);
}

static inline u8 mmc_calc_packed_hdr_segs(struct request_queue *q,
					  struct mmc_card *card)
{
	unsigned int hdr_sz = mmc_large_sector(card) ? 4096 : 512;
	unsigned int max_seg_sz = queue_max_segment_size(q);
	unsigned int len, nr_segs = 0;

	do {
		len = min(hdr_sz, max_seg_sz);
		hdr_sz -= len;
		nr_segs++;
	} while (hdr_sz);

	return nr_segs;
}

static u8 mmc_blk_prep_packed_list(struct mmc_queue *mq, struct request *req)
{
	struct request_queue *q = mq->queue;
	struct mmc_card *card = mq->card;
	struct request *cur = req, *next = NULL;
	struct mmc_blk_data *md = mq->data;
	struct mmc_queue_req *mqrq = mq->mqrq_cur;
	bool en_rel_wr = card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN;
	unsigned int req_sectors = 0, phys_segments = 0;
	unsigned int max_blk_count, max_phys_segs;
	bool put_back = true;
	u8 max_packed_rw = 0;
	u8 reqs = 0;

	if (!(md->flags & MMC_BLK_PACKED_CMD))
		goto no_packed;

	if ((rq_data_dir(cur) == WRITE) &&
	    mmc_host_packed_wr(card->host))
		max_packed_rw = card->ext_csd.max_packed_writes;

	if (max_packed_rw == 0)
		goto no_packed;

	if (mmc_req_rel_wr(cur) &&
	    (md->flags & MMC_BLK_REL_WR) && !en_rel_wr)
		goto no_packed;

	if (mmc_large_sector(card) &&
	    !IS_ALIGNED(blk_rq_sectors(cur), 8))
		goto no_packed;

	mmc_blk_clear_packed(mqrq);

	max_blk_count = min(card->host->max_blk_count,
			    card->host->max_req_size >> 9);
	if (unlikely(max_blk_count > 0xffff))
		max_blk_count = 0xffff;

	max_phys_segs = queue_max_segments(q);
	req_sectors += blk_rq_sectors(cur);
	phys_segments += cur->nr_phys_segments;

	if (rq_data_dir(cur) == WRITE) {
		req_sectors += mmc_large_sector(card) ? 8 : 1;
		phys_segments += mmc_calc_packed_hdr_segs(q, card);
	}

	do {
		if (reqs >= max_packed_rw - 1) {
			put_back = false;
			break;
		}

		spin_lock_irq(q->queue_lock);
		next = blk_fetch_request(q);
		spin_unlock_irq(q->queue_lock);
		if (!next) {
			put_back = false;
			break;
		}

		if (mmc_large_sector(card) &&
		    !IS_ALIGNED(blk_rq_sectors(next), 8))
			break;

		if (next->cmd_flags & REQ_DISCARD ||
		    next->cmd_flags & REQ_FLUSH)
			break;

		if (rq_data_dir(cur) != rq_data_dir(next))
			break;

		if (mmc_req_rel_wr(next) &&
		    (md->flags & MMC_BLK_REL_WR) && !en_rel_wr)
			break;

		req_sectors += blk_rq_sectors(next);
		if (req_sectors > max_blk_count)
			break;

		phys_segments +=  next->nr_phys_segments;
		if (phys_segments > max_phys_segs)
			break;

		list_add_tail(&next->queuelist, &mqrq->packed->list);
		cur = next;
		reqs++;
	} while (1);

	if (put_back) {
		spin_lock_irq(q->queue_lock);
		blk_requeue_request(q, next);
		spin_unlock_irq(q->queue_lock);
	}

	if (reqs > 0) {
		list_add(&req->queuelist, &mqrq->packed->list);
		mqrq->packed->nr_entries = ++reqs;
		mqrq->packed->retries = reqs;
		return reqs;
	}

no_packed:
	mqrq->cmd_type = MMC_PACKED_NONE;
	return 0;
}

static void mmc_blk_packed_hdr_wrq_prep(struct mmc_queue_req *mqrq,
					struct mmc_card *card,
					struct mmc_queue *mq)
{
	struct mmc_blk_request *brq = &mqrq->brq;
	struct request *req = mqrq->req;
	struct request *prq;
	struct mmc_blk_data *md = mq->data;
	struct mmc_packed *packed = mqrq->packed;
	bool do_rel_wr, do_data_tag;
	__le32 *packed_cmd_hdr;
	u8 hdr_blocks;
	u8 i = 1;

	BUG_ON(!packed);

	mqrq->cmd_type = MMC_PACKED_WRITE;
	packed->blocks = 0;
	packed->idx_failure = MMC_PACKED_NR_IDX;

	packed_cmd_hdr = packed->cmd_hdr;
	memset(packed_cmd_hdr, 0, sizeof(packed->cmd_hdr));
	packed_cmd_hdr[0] = cpu_to_le32((packed->nr_entries << 16) |
		(PACKED_CMD_WR << 8) | PACKED_CMD_VER);
	hdr_blocks = mmc_large_sector(card) ? 8 : 1;

	/*
	 * Argument for each entry of packed group
	 */
	list_for_each_entry(prq, &packed->list, queuelist) {
		do_rel_wr = mmc_req_rel_wr(prq) && (md->flags & MMC_BLK_REL_WR);
		do_data_tag = (card->ext_csd.data_tag_unit_size) &&
			(prq->cmd_flags & REQ_META) &&
			(rq_data_dir(prq) == WRITE) &&
			blk_rq_bytes(prq) >= card->ext_csd.data_tag_unit_size;
		/* Argument of CMD23 */
		packed_cmd_hdr[(i * 2)] = cpu_to_le32(
			(do_rel_wr ? MMC_CMD23_ARG_REL_WR : 0) |
			(do_data_tag ? MMC_CMD23_ARG_TAG_REQ : 0) |
			blk_rq_sectors(prq));
		/* Argument of CMD18 or CMD25 */
		packed_cmd_hdr[((i * 2)) + 1] = cpu_to_le32(
			mmc_card_blockaddr(card) ?
			blk_rq_pos(prq) : blk_rq_pos(prq) << 9);
		packed->blocks += blk_rq_sectors(prq);
		i++;
	}

	memset(brq, 0, sizeof(struct mmc_blk_request));
	brq->mrq.cmd = &brq->cmd;
	brq->mrq.data = &brq->data;
	brq->mrq.sbc = &brq->sbc;
	brq->mrq.stop = &brq->stop;

	brq->sbc.opcode = MMC_SET_BLOCK_COUNT;
	brq->sbc.arg = MMC_CMD23_ARG_PACKED | (packed->blocks + hdr_blocks);
	brq->sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;

	brq->cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
	brq->cmd.arg = blk_rq_pos(req);
	if (!mmc_card_blockaddr(card))
		brq->cmd.arg <<= 9;
	brq->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	brq->data.blksz = 512;
	brq->data.blocks = packed->blocks + hdr_blocks;
	brq->data.flags |= MMC_DATA_WRITE;

	brq->stop.opcode = MMC_STOP_TRANSMISSION;
	brq->stop.arg = 0;
	brq->stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	mmc_set_data_timeout(&brq->data, card);

	brq->data.sg = mqrq->sg;
	brq->data.sg_len = mmc_queue_map_sg(mq, mqrq);

	mqrq->mmc_active.mrq = &brq->mrq;
	mqrq->mmc_active.err_check = mmc_blk_packed_err_check;

	mmc_queue_bounce_pre(mqrq);
}

static int mmc_blk_cmd_err(struct mmc_blk_data *md, struct mmc_card *card,
			   struct mmc_blk_request *brq, struct request *req,
			   int ret)
{
	struct mmc_queue_req *mq_rq;
	mq_rq = container_of(brq, struct mmc_queue_req, brq);

	/*
	 * If this is an SD card and we're writing, we can first
	 * mark the known good sectors as ok.
	 *
	 * If the card is not SD, we can still ok written sectors
	 * as reported by the controller (which might be less than
	 * the real number of written sectors, but never more).
	 */
	if (mmc_card_sd(card)) {
		u32 blocks;

		blocks = mmc_sd_num_wr_blocks(card);
		if (blocks != (u32)-1) {
			ret = blk_end_request(req, 0, blocks << 9);
		}
	} else {
		if (!mmc_packed_cmd(mq_rq->cmd_type))
			ret = blk_end_request(req, 0, brq->data.bytes_xfered);
	}
	return ret;
}

static int mmc_blk_end_packed_req(struct mmc_queue_req *mq_rq)
{
	struct request *prq;
	struct mmc_packed *packed = mq_rq->packed;
	int idx = packed->idx_failure, i = 0;
	int ret = 0;

	BUG_ON(!packed);

	while (!list_empty(&packed->list)) {
		prq = list_entry_rq(packed->list.next);
		if (idx == i) {
			/* retry from error index */
			packed->nr_entries -= idx;
			mq_rq->req = prq;
			ret = 1;

			if (packed->nr_entries == MMC_PACKED_NR_SINGLE) {
				list_del_init(&prq->queuelist);
				mmc_blk_clear_packed(mq_rq);
			}
			return ret;
		}
		list_del_init(&prq->queuelist);
		blk_end_request(prq, 0, blk_rq_bytes(prq));
		i++;
	}

	mmc_blk_clear_packed(mq_rq);
	return ret;
}

static void mmc_blk_abort_packed_req(struct mmc_queue_req *mq_rq)
{
	struct request *prq;
	struct mmc_packed *packed = mq_rq->packed;

	BUG_ON(!packed);

	while (!list_empty(&packed->list)) {
		prq = list_entry_rq(packed->list.next);
		list_del_init(&prq->queuelist);
		blk_end_request(prq, -EIO, blk_rq_bytes(prq));
	}

	mmc_blk_clear_packed(mq_rq);
}

static void mmc_blk_revert_packed_req(struct mmc_queue *mq,
				      struct mmc_queue_req *mq_rq)
{
	struct request *prq;
	struct request_queue *q = mq->queue;
	struct mmc_packed *packed = mq_rq->packed;

	BUG_ON(!packed);

	while (!list_empty(&packed->list)) {
		prq = list_entry_rq(packed->list.prev);
		if (prq->queuelist.prev != &packed->list) {
			list_del_init(&prq->queuelist);
			spin_lock_irq(q->queue_lock);
			blk_requeue_request(mq->queue, prq);
			spin_unlock_irq(q->queue_lock);
		} else {
			list_del_init(&prq->queuelist);
		}
	}

	mmc_blk_clear_packed(mq_rq);
}

static int mmc_blk_cmdq_start_req(struct mmc_host *host,
				  struct mmc_cmdq_req *cmdq_req)
{
	struct mmc_request *mrq = &cmdq_req->mrq;

	mrq->done = mmc_blk_cmdq_req_done;
	return mmc_cmdq_start_req(host, cmdq_req);
}

/* prepare for non-data commands */
static struct mmc_cmdq_req *mmc_cmdq_prep_dcmd(
		struct mmc_queue_req *mqrq, struct mmc_queue *mq)
{
	struct request *req = mqrq->req;
	struct mmc_cmdq_req *cmdq_req = &mqrq->cmdq_req;

	memset(&mqrq->cmdq_req, 0, sizeof(struct mmc_cmdq_req));

	cmdq_req->mrq.data = NULL;
	cmdq_req->cmd_flags = req->cmd_flags;
	cmdq_req->mrq.req = mqrq->req;
	req->special = mqrq;
	cmdq_req->cmdq_req_flags |= DCMD;
	cmdq_req->mrq.cmdq_req = cmdq_req;
	mqrq->allowed = MAX_RETRIES;

	return &mqrq->cmdq_req;
}


#define IS_RT_CLASS_REQ(x)     \
	(IOPRIO_PRIO_CLASS(req_get_ioprio(x)) == IOPRIO_CLASS_RT)
SIO_PATCH_VERSION(eMMC_CP, 1, 0, "");
/* IOPP-emmc_cp-v1.0.4.4 */

static struct mmc_cmdq_req *mmc_blk_cmdq_rw_prep(
		struct mmc_queue_req *mqrq, struct mmc_queue *mq)
{
	struct mmc_card *card = mq->card;
	struct request *req = mqrq->req;
	struct mmc_blk_data *md = mq->data;
	bool do_rel_wr = mmc_req_rel_wr(req) && (md->flags & MMC_BLK_REL_WR);
	bool do_data_tag;
	bool read_dir = (rq_data_dir(req) == READ);
	struct mmc_cmdq_req *cmdq_rq = &mqrq->cmdq_req;

	memset(&mqrq->cmdq_req, 0, sizeof(struct mmc_cmdq_req));

	cmdq_rq->tag = req->tag;
	if (read_dir) {
		cmdq_rq->cmdq_req_flags |= DIR;
		cmdq_rq->data.flags = MMC_DATA_READ;
	} else {
		cmdq_rq->data.flags = MMC_DATA_WRITE;
	}
	if (read_dir || req->cmd_flags & REQ_SYNC)
		cmdq_rq->cmdq_req_flags |= PRIO;

	if (do_rel_wr)
		cmdq_rq->cmdq_req_flags |= REL_WR;

	cmdq_rq->data.blocks = blk_rq_sectors(req);
	cmdq_rq->blk_addr = blk_rq_pos(req);
	cmdq_rq->data.blksz = MMC_CARD_CMDQ_BLK_SIZE;

	mmc_set_data_timeout(&cmdq_rq->data, card);

	do_data_tag = (card->ext_csd.data_tag_unit_size) &&
		(req->cmd_flags & REQ_META) &&
		(rq_data_dir(req) == WRITE) &&
		((cmdq_rq->data.blocks * cmdq_rq->data.blksz) >=
		 card->ext_csd.data_tag_unit_size);
	if (do_data_tag)
		cmdq_rq->cmdq_req_flags |= DAT_TAG;
	cmdq_rq->data.sg = mqrq->sg;
	cmdq_rq->data.sg_len = mmc_queue_map_sg(mq, mqrq);

	/*
	 * Adjust the sg list so it is the same size as the
	 * request.
	 */
	if (cmdq_rq->data.blocks > card->host->max_blk_count)
		cmdq_rq->data.blocks = card->host->max_blk_count;

	if (cmdq_rq->data.blocks != blk_rq_sectors(req)) {
		int i, data_size = cmdq_rq->data.blocks << 9;
		struct scatterlist *sg;

		for_each_sg(cmdq_rq->data.sg, sg, cmdq_rq->data.sg_len, i) {
			data_size -= sg->length;
			if (data_size <= 0) {
				sg->length += data_size;
				i++;
				break;
			}
		}
		cmdq_rq->data.sg_len = i;
	}

	mqrq->cmdq_req.cmd_flags = req->cmd_flags;
	mqrq->cmdq_req.mrq.req = mqrq->req;
	mqrq->cmdq_req.mrq.cmdq_req = &mqrq->cmdq_req;
	mqrq->cmdq_req.mrq.data = &mqrq->cmdq_req.data;
	mqrq->req->special = mqrq;
	mqrq->allowed = MAX_RETRIES;

	pr_debug("%s: %s: mrq: 0x%p req: 0x%p mqrq: 0x%p bytes to xf: %d mmc_cmdq_req: 0x%p card-addr: 0x%08x dir(r-1/w-0): %d\n",
		 mmc_hostname(card->host), __func__, &mqrq->cmdq_req.mrq,
		 mqrq->req, mqrq, (cmdq_rq->data.blocks * cmdq_rq->data.blksz),
		 cmdq_rq, cmdq_rq->blk_addr,
		 (cmdq_rq->cmdq_req_flags & DIR) ? 1 : 0);

	return &mqrq->cmdq_req;
}

static int mmc_blk_cmdq_issue_rw_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_queue_req *active_mqrq;
	struct mmc_card *card = mq->card;
	struct mmc_host *host = card->host;
	struct mmc_cmdq_req *mc_rq;
#ifdef CONFIG_MMC_CMDQ_DEBUG
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;
#endif
	int ret = 0;

	BUG_ON((req->tag < 0) || (req->tag > card->ext_csd.cmdq_depth));
	BUG_ON(test_and_set_bit(req->tag, &host->cmdq_ctx.data_active_reqs));
	BUG_ON(test_and_set_bit(req->tag, &host->cmdq_ctx.active_reqs));

#ifdef CONFIG_MMC_CMDQ_DEBUG
	/* cq debug */
	exynos_ss_printk("[CQ] I_N: tag = %d, flag = 0x%lx, active = 0x%lx\n",
				req->tag, ctx_info->curr_state,
				ctx_info->active_reqs);
#endif
	active_mqrq = &mq->mqrq_cmdq[req->tag];
	active_mqrq->req = req;

	mc_rq = mmc_blk_cmdq_rw_prep(active_mqrq, mq);

#if defined(CONFIG_MMC_CQ_HCI) && defined(CONFIG_MMC_DATA_LOG)
	mmc_blk_cmdq_store_req_log(card, req, &mc_rq->data, true, 0);
#endif

	ret = mmc_blk_cmdq_start_req(card->host, mc_rq);

	if (!ret && (card->quirks & MMC_QUIRK_CMDQ_EMPTY_BEFORE_DCMD)) {
		unsigned int sectors = blk_rq_sectors(req);

		if (((sectors > 0) && (sectors < 8))
		    && (rq_data_dir(req) == READ))
			host->cmdq_ctx.active_small_sector_read_reqs++;
	}

	return ret;
}

/*
 * Issues a flush (dcmd) request
 */
int mmc_blk_cmdq_issue_flush_rq(struct mmc_queue *mq, struct request *req)
{
	int err;
	struct mmc_queue_req *active_mqrq;
	struct mmc_card *card = mq->card;
	struct mmc_cmdq_req *cmdq_req;
	struct mmc_host *host = card->host;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;

	BUG_ON(!card);
	host = card->host;
	BUG_ON(!host);
	BUG_ON(req->tag > card->ext_csd.cmdq_depth);
	BUG_ON(test_and_set_bit(req->tag, &host->cmdq_ctx.active_reqs));

#ifdef CONFIG_MMC_CMDQ_DEBUG
	/* cq debug */
	exynos_ss_printk("[CQ] I_F: tag = %d, flag = 0x%lx, active = 0x%lx\n",
				req->tag, ctx_info->curr_state,
				ctx_info->active_reqs);
#endif
	ctx_info = &host->cmdq_ctx;

	set_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx_info->curr_state);

	active_mqrq = &mq->mqrq_cmdq[req->tag];
	active_mqrq->req = req;

	cmdq_req = mmc_cmdq_prep_dcmd(active_mqrq, mq);
	cmdq_req->cmdq_req_flags |= QBR;
	cmdq_req->mrq.cmd = &cmdq_req->cmd;
	cmdq_req->tag = req->tag;

	err = mmc_cmdq_prepare_flush(cmdq_req->mrq.cmd);
	if (err) {
		pr_err("%s: failed (%d) preparing flush req\n",
		       mmc_hostname(host), err);
		return err;
	}

	err = mmc_blk_cmdq_start_req(card->host, cmdq_req);
	return err;
}
EXPORT_SYMBOL(mmc_blk_cmdq_issue_flush_rq);

static void mmc_blk_cmdq_reset(struct mmc_host *host, bool clear_all)
{
	if (!host->cmdq_ops->reset)
		return;

	if (!test_bit(CMDQ_STATE_HALT, &host->cmdq_ctx.curr_state)) {
		if (mmc_cmdq_halt(host, true)) {
			pr_err("%s: halt failed\n", mmc_hostname(host));
			goto reset;
		}
	}

	if (clear_all)
		mmc_cmdq_discard_queue(host, 0);
reset:
	mmc_hw_reset(host);
	host->cmdq_ops->reset(host, true);
	clear_bit(CMDQ_STATE_HALT, &host->cmdq_ctx.curr_state);
	mmc_cmdq_error_logging(host->card, NULL, CQ_HW_RST);
}

static void mmc_blk_cmdq_shutdown(struct mmc_queue *mq)
{
	int err;
	struct mmc_card *card = mq->card;
	struct mmc_host *host = card->host;

	mmc_get_card(card);
	err = mmc_cmdq_halt(host, true);
	if (err) {
		pr_err("%s: halt: failed: %d\n", __func__, err);
		return;
	}

	/* disable CQ mode in card */
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			 EXT_CSD_CMDQ, 0,
			 card->ext_csd.generic_cmd6_time);
	if (err) {
		pr_err("%s: failed to switch card to legacy mode: %d\n",
		       __func__, err);
		goto out;
	} else {
		mmc_card_clr_cmdq(card);
		host->cmdq_ops->disable(host, false);
		host->card->cmdq_init = false;
	}
out:
	mmc_put_card(card);
}

static enum blk_eh_timer_return mmc_blk_cmdq_req_timed_out(struct request *req)
{
	struct mmc_queue *mq = req->q->queuedata;
	struct mmc_host *host = mq->card->host;
	struct mmc_queue_req *mq_rq = req->special;
	struct mmc_request *mrq;
	struct mmc_cmdq_req *cmdq_req;

	BUG_ON(!host);
	/*
	 * The mmc_queue_req will be present only if the request
	 * is issued to the LLD. The request could be fetched from
	 * block layer queue but could be waiting to be issued
	 * (for e.g. clock scaling is waiting for an empty cmdq queue)
	 * Reset the timer in such cases to give LLD more time
	 */
	if (!mq_rq) {
		pr_warn("%s: restart timer for tag: %d\n", __func__, req->tag);
		return BLK_EH_RESET_TIMER;
	}

	mrq = &mq_rq->cmdq_req.mrq;
	cmdq_req = &mq_rq->cmdq_req;

	if (host->err_mrq == NULL)
		host->err_mrq = mrq;

	BUG_ON(!mrq || !cmdq_req);

	if (cmdq_req->cmdq_req_flags & DCMD)
		mrq->cmd->error = -ETIMEDOUT;
	else
		mrq->data->error = -ETIMEDOUT;

	if (cmdq_req->cmdq_req_flags & DCMD &&
			!(mrq->req->cmd_flags & REQ_FLUSH)) {
		mrq->done(mrq);
		return BLK_EH_NOT_HANDLED;
	} else {
		return BLK_EH_HANDLED;
	}
}

static void mmc_blk_cmdq_err(struct mmc_queue *mq)
{
	int err;
	int retry = 0;
	int gen_err;
	u32 status;
	unsigned long timeout;
	struct mmc_request *mrq_t;
	struct mmc_request *mrq_n;
	struct mmc_queue_req *mqrq_t;

	struct mmc_host *host = mq->card->host;
	struct mmc_request *mrq = host->err_mrq;
	struct mmc_card *card = mq->card;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;
	unsigned long flags;

	mmc_get_card(card);
	printk("\n\n=============== CQ RECOVERY START ======================\n\n");

	err = mmc_cmdq_halt(host, true);
	if (err) {
		pr_err("halt: failed: %d\n", err);
		goto reset;
	}

	if (mrq->data && mrq->data->error) {
		for (; retry < MAX_RETRIES; retry++) {
			err = get_card_status(card, &status, 0);
			if (!err)
				break;
		}

		mmc_cmdq_error_logging(host->card, mrq->cmdq_req, status);

		if (err) {
			pr_err("%s: No response from card !!! err:%d\n",
			       mmc_hostname(host), err);
			goto reset;
		}

		if (R1_CURRENT_STATE(status) == R1_STATE_DATA ||
		    R1_CURRENT_STATE(status) == R1_STATE_RCV) {
			err =  send_stop(card, MMC_CMDQ_STOP_TIMEOUT_MS,
					 mrq->req, &gen_err, &status);

			mmc_cmdq_error_logging(host->card, mrq->cmdq_req, status);

			if (err) {
				pr_err("%s: error %d sending stop (%d) command\n",
					mrq->req->rq_disk->disk_name,
					err, status);
				goto reset;
			}
		}

		/*
		 * Exynos supports only Discard entire queue mode
		 * when it comes to Task Management
		 * So you should go to cq reset routine and requeue some tasks.
		 */

	} else if (mrq->cmd && mrq->cmd->error) {
		/* DCMD commands */

		/*
		 * Notify completion for non flush commands like discard
		 * that wait for DCMD finish.
		 */
		if (!(mrq->req->cmd_flags & REQ_FLUSH)) {
			complete(&mrq->completion);
			goto reset;
		}
	} else {
		/* Unexpected cases */
		WARN_ON(1);
	}

reset:
	/* Collect all the pending tasks to cancel */
	timeout = jiffies + msecs_to_jiffies(120000);
	while (time_before(jiffies, timeout)) {
		if (!ctx_info->active_reqs)
			break;
	}

	spin_lock_irqsave(&mq->eh_lock, flags);
	list_for_each_entry_safe_reverse(mrq_t, mrq_n, &mq->eh_mrq, eh_entry) {
		list_del(&mrq_t->eh_entry);
		BUG_ON(!mrq_t->req);
		mqrq_t = mrq_t->req->special;
		/* Increase retry count */
		mrq_t->req->retries++;
		if (mrq_t->req->retries <= mqrq_t->allowed) {
			pr_err("[CQ] %s:----- REQUEUE: tag %d sector %u, nr %u, retries %d\n",
				mmc_hostname(mrq_t->host), mrq_t->req->tag,
				(unsigned)blk_rq_pos(mrq_t->req),
				(unsigned)blk_rq_sectors(mrq_t->req),
				mrq_t->req->retries);
			spin_lock_irq(mq->queue->queue_lock);
			blk_requeue_request(mrq_t->req->q, mrq_t->req);
			spin_unlock_irq(mq->queue->queue_lock);
		} else {
			pr_err("[CQ] %s:----- Finish request: tag %d, sector %u, nr %u, retries %d\n",
				mmc_hostname(mrq_t->host), mrq_t->req->tag,
				(unsigned)blk_rq_pos(mrq_t->req),
				(unsigned)blk_rq_sectors(mrq_t->req),
				mrq_t->req->retries);
			blk_end_request_all(mrq_t->req, err);
		}
	}
	spin_unlock_irqrestore(&mq->eh_lock, flags);
	mmc_blk_cmdq_reset(host, true);

	host->err_mrq = NULL;
	pm_runtime_mark_last_busy(&card->dev);
	clear_bit(CMDQ_STATE_ERR, &ctx_info->curr_state);
	clear_bit(CMDQ_STATE_ERR_HOST, &ctx_info->curr_state);
	set_bit(CMDQ_STATE_ERR_RCV_DONE, &ctx_info->curr_state);
	clear_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx_info->curr_state);
	clear_bit(CMDQ_STATE_DO_RECOVERY, &ctx_info->curr_state);
	ctx_info->dump_state = CMDQ_DUMP_NONE_ERR;
	printk("\n\n=============== CQ RECOVERY END ======================\n\n");
	wake_up(&ctx_info->wait);
	mmc_put_card(card);
}

/* invoked by block layer in softirq context */
void mmc_blk_cmdq_complete_rq(struct request *rq)
{
	struct mmc_queue_req *mq_rq = rq->special;
	struct mmc_request *mrq = &mq_rq->cmdq_req.mrq;
	struct mmc_host *host = mrq->host;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;
	struct mmc_cmdq_req *cmdq_req = &mq_rq->cmdq_req;
	struct mmc_queue *mq = (struct mmc_queue *)rq->q->queuedata;
	int err = 0;
	unsigned long flags;

	if (mrq->cmd && mrq->cmd->error)
		err = mrq->cmd->error;
	else if (mrq->data && mrq->data->error)
		err = mrq->data->error;
	else if (mrq->cmdq_req && mrq->cmdq_req->resp_err)
		err = mrq->cmdq_req->resp_err;

	/* clear pending request */
	BUG_ON(!test_and_clear_bit(cmdq_req->tag,
				   &ctx_info->active_reqs));

	if (!(cmdq_req->cmdq_req_flags & DCMD))
		BUG_ON(!test_and_clear_bit(cmdq_req->tag,
			 &ctx_info->data_active_reqs));

	mmc_cmdq_post_req(host, mrq, err);

#if defined(CONFIG_MMC_CQ_HCI) && defined(CONFIG_MMC_DATA_LOG)
	mmc_blk_cmdq_store_req_log(host->card, rq, &cmdq_req->data, false, err);
#endif

	/*
	 * All the host and timed-out errors would come here
	 * except for discard.
	 */
	if (err) {
		if (host->err_mrq == NULL)
			host->err_mrq = mrq;

		pr_err("%s: %s: txfr error: %d\n", mmc_hostname(mrq->host),
		       __func__, err);
		spin_lock_irqsave(&mq->eh_lock, flags);
		list_add_tail(&mrq->eh_entry, &mq->eh_mrq);
		spin_unlock_irqrestore(&mq->eh_lock, flags);
		if (test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state)) {
			pr_err("%s: CQ in error state, ending current req: %d\n",
				__func__, err);
		} else {
			set_bit(CMDQ_STATE_ERR, &ctx_info->curr_state);
			host->cmdq_ops->pclear(host);
			schedule_work(&mq->cmdq_err_work);
		}
		goto err;
	}

	if (cmdq_req->cmdq_req_flags & DCMD) {
		clear_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx_info->curr_state);
		blk_end_request_all(rq, err);
		goto out;
	}

	blk_end_request(rq, err, cmdq_req->data.bytes_xfered);

out:
	if (!test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state))
		wake_up(&ctx_info->wait);

	mmc_put_card(host->card);
	if (!ctx_info->active_reqs)
		wake_up_interruptible(&host->cmdq_ctx.queue_empty_wq);

	if (blk_queue_stopped(mq->queue) && !ctx_info->active_reqs)
		complete(&mq->cmdq_shutdown_complete);

	return;
err:
	mmc_put_card(host->card);

	return;
}

/*
 * Complete reqs from block layer softirq context
 * Invoked in irq context
 */
void mmc_blk_cmdq_req_done(struct mmc_request *mrq)
{
	struct request *req = mrq->req;

	blk_complete_request(req);
}
EXPORT_SYMBOL(mmc_blk_cmdq_req_done);

static int mmc_blk_issue_rw_rq(struct mmc_queue *mq, struct request *rqc)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	struct mmc_blk_request *brq = &mq->mqrq_cur->brq;
	int ret = 1, disable_multi = 0, retry = 0, type, retune_retry_done = 0;
	enum mmc_blk_status status;
	struct mmc_queue_req *mq_rq;
	struct request *req = rqc;
	struct mmc_async_req *areq;
	const u8 packed_nr = 2;
	u8 reqs = 0;
#ifdef CONFIG_MMC_SIMULATE_MAX_SPEED
	unsigned long waitfor = jiffies;
#endif

	if (!rqc && !mq->mqrq_prev->req)
		return 0;

	if (rqc)
		reqs = mmc_blk_prep_packed_list(mq, rqc);

	do {
		if (rqc) {
			/*
			 * When 4KB native sector is enabled, only 8 blocks
			 * multiple read or write is allowed
			 */
			if ((brq->data.blocks & 0x07) &&
			    (card->ext_csd.data_sector_size == 4096)) {
				pr_err("%s: Transfer size is not 4KB sector size aligned\n",
					req->rq_disk->disk_name);
				mq_rq = mq->mqrq_cur;
				goto cmd_abort;
			}

			if (reqs >= packed_nr)
				mmc_blk_packed_hdr_wrq_prep(mq->mqrq_cur,
							    card, mq);
			else
				mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
			areq = &mq->mqrq_cur->mmc_active;
		} else
			areq = NULL;
		areq = mmc_start_req(card->host, areq, (int *) &status);
		if (!areq) {
			if (status == MMC_BLK_NEW_REQUEST)
				set_bit(MMC_QUEUE_NEW_REQUEST, &mq->flags);
			return 0;
		}

		mq_rq = container_of(areq, struct mmc_queue_req, mmc_active);
		brq = &mq_rq->brq;
		req = mq_rq->req;
		type = rq_data_dir(req) == READ ? MMC_BLK_READ : MMC_BLK_WRITE;
		mmc_queue_bounce_post(mq_rq);

		switch (status) {
		case MMC_BLK_SUCCESS:
		case MMC_BLK_PARTIAL:
			/*
			 * A block was successfully transferred.
			 */
			mmc_blk_reset_success(md, type);

			mmc_blk_simulate_delay(mq, rqc, waitfor);

			if (mmc_packed_cmd(mq_rq->cmd_type)) {
				ret = mmc_blk_end_packed_req(mq_rq);
				break;
			} else {
				ret = blk_end_request(req, 0,
						brq->data.bytes_xfered);
			}

			/*
			 * If the blk_end_request function returns non-zero even
			 * though all data has been transferred and no errors
			 * were returned by the host controller, it's a bug.
			 */
			if (status == MMC_BLK_SUCCESS && ret) {
				pr_err("%s BUG rq_tot %d d_xfer %d\n",
				       __func__, blk_rq_bytes(req),
				       brq->data.bytes_xfered);
				rqc = NULL;
				goto cmd_abort;
			}
			break;
		case MMC_BLK_CMD_ERR:
			ret = mmc_blk_cmd_err(md, card, brq, req, ret);
			if (mmc_blk_reset(md, card->host, type))
				goto cmd_abort;
			if (!ret)
				goto start_new_req;
			break;
		case MMC_BLK_RETRY:
			retune_retry_done = brq->retune_retry_done;
			if (retry++ < 5)
				break;
			/* Fall through */
		case MMC_BLK_ABORT:
			if (!mmc_blk_reset(md, card->host, type))
				break;
			goto cmd_abort;
		case MMC_BLK_DATA_ERR: {
			int err;

			err = mmc_blk_reset(md, card->host, type);
			if (!err)
				break;
			if (err == -ENODEV ||
				mmc_packed_cmd(mq_rq->cmd_type))
				goto cmd_abort;
			/* Fall through */
		}
		case MMC_BLK_ECC_ERR:
			if (brq->data.blocks > 1) {
				/* Redo read one sector at a time */
				pr_warn("%s: retrying using single block read\n",
					req->rq_disk->disk_name);
				disable_multi = 1;
				break;
			}
			/*
			 * After an error, we redo I/O one sector at a
			 * time, so we only reach here after trying to
			 * read a single sector.
			 */
			ret = blk_end_request(req, -EIO,
						brq->data.blksz);
			if (!ret)
				goto start_new_req;
			break;
		case MMC_BLK_NOMEDIUM:
			goto cmd_abort;
		default:
			pr_err("%s: Unhandled return value (%d)",
					req->rq_disk->disk_name, status);
			goto cmd_abort;
		}

		if (ret) {
			if (mmc_packed_cmd(mq_rq->cmd_type)) {
				if (!mq_rq->packed->retries)
					goto cmd_abort;
				mmc_blk_packed_hdr_wrq_prep(mq_rq, card, mq);
				mmc_start_req(card->host,
					      &mq_rq->mmc_active, NULL);
			} else {

				/*
				 * In case of a incomplete request
				 * prepare it again and resend.
				 */
				mmc_blk_rw_rq_prep(mq_rq, card,
						disable_multi, mq);
				mmc_start_req(card->host,
						&mq_rq->mmc_active, NULL);
			}
			mq_rq->brq.retune_retry_done = retune_retry_done;
		}
	} while (ret);

	return 1;

 cmd_abort:
	if (mmc_packed_cmd(mq_rq->cmd_type)) {
		mmc_blk_abort_packed_req(mq_rq);
	} else {
		if (mmc_card_removed(card))
			req->cmd_flags |= REQ_QUIET;
		while (ret)
			ret = blk_end_request(req, -EIO,
					blk_rq_cur_bytes(req));
	}

 start_new_req:
	if (rqc) {
		if (mmc_card_removed(card)) {
			rqc->cmd_flags |= REQ_QUIET;
			blk_end_request_all(rqc, -EIO);
		} else {
			/*
			 * If current request is packed, it needs to put back.
			 */
			if (mmc_packed_cmd(mq->mqrq_cur->cmd_type))
				mmc_blk_revert_packed_req(mq, mq->mqrq_cur);

			mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
			mmc_start_req(card->host,
				      &mq->mqrq_cur->mmc_active, NULL);
		}
	}

	return 0;
}

static inline int mmc_blk_cmdq_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md)
{
	struct mmc_blk_data *main_md = mmc_get_drvdata(card);
	struct mmc_host *host = card->host;
	struct mmc_cmdq_context_info *ctx = &host->cmdq_ctx;
	u8 part_config = card->ext_csd.part_config;
	int status, ret;
	u8 *ext_csd;

	if ((main_md->part_curr == md->part_type) &&
	    (card->part_curr == md->part_type))
		return 0;

	WARN_ON(!((card->host->caps2 & MMC_CAP2_CMD_QUEUE) &&
		 card->ext_csd.cmdq_support &&
		 (md->flags & MMC_BLK_CMD_QUEUE)));

	if (!test_bit(CMDQ_STATE_HALT, &ctx->curr_state))
		WARN_ON(mmc_cmdq_halt(host, true));

	/* disable CQ mode in card */
	if (mmc_card_cmdq(card)) {
		WARN_ON(mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_CMDQ, 0,
				  card->ext_csd.generic_cmd6_time));
		mmc_card_clr_cmdq(card);
	}

	part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
	part_config |= md->part_type;

	ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			 EXT_CSD_PART_CONFIG, part_config,
			  card->ext_csd.part_time);

	if (ret) {
			if(ret != -ETIMEDOUT)
				return ret;

			/* re-check the partition config is changed or not */
			ret = get_card_status(card, &status, 0);
			if (ret) {
				pr_err("%s: error %d requesting status\n",
			    	   __func__, ret);
				return ret;
			}
			if((R1_CURRENT_STATE(status) == R1_STATE_TRAN)
				&& (status & R1_READY_FOR_DATA)) {
				ret = mmc_get_ext_csd(card, &ext_csd);
				if (ret) {
					pr_err("%s: error %d sending ext_csd\n",
			    		   __func__, ret);
					if(ext_csd != NULL)
						kfree(ext_csd);
					return ret;
				}
				if (ext_csd[EXT_CSD_PART_CONFIG] == part_config) {
					pr_info("%s: partition config is changed %d", __func__, part_config);
					kfree(ext_csd);
					goto done;
				}
			}
			return ret;
		}
done:
	card->ext_csd.part_config = part_config;
	card->part_curr = md->part_type;

	main_md->part_curr = md->part_type;

	WARN_ON(mmc_blk_cmdq_switch(card, md, true));
	WARN_ON(mmc_cmdq_halt(host, false));

	return 0;
}

static int mmc_blk_cmdq_issue_rq(struct mmc_queue *mq, struct request *req)
{
	int ret;
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	unsigned int cmd_flags = req ? req->cmd_flags : 0;
	struct mmc_host *host = card->host;
	struct mmc_cmdq_context_info *ctx = &host->cmdq_ctx;

	mmc_get_card(card);

	/* Orphan requests doesn't exist anymore */
	clear_bit(CMDQ_STATE_ERR_RCV_DONE, &ctx->curr_state);

	if (!card->host->cmdq_ctx.active_reqs && mmc_card_doing_bkops(card)) {
		ret = mmc_cmdq_halt(card->host, true);
		if (ret)
			goto out;
		ret = mmc_stop_bkops(card);
		if (ret) {
			pr_err("%s: %s: mmc_stop_bkops failed %d\n",
					md->disk->disk_name, __func__, ret);
			goto out;
		}
		ret = mmc_cmdq_halt(card->host, false);
		if (ret)
			goto out;
	}

	ret = mmc_blk_cmdq_part_switch(card, md);
	if (ret) {
		pr_err("%s: %s: partition switch failed %d\n",
				md->disk->disk_name, __func__, ret);
		goto out;
	}

	if (req) {
		if ((cmd_flags & (REQ_FLUSH | REQ_DISCARD)) &&
		    (card->quirks & MMC_QUIRK_CMDQ_EMPTY_BEFORE_DCMD) &&
		    ctx->active_small_sector_read_reqs) {
			ret = wait_event_interruptible(ctx->queue_empty_wq,
						      !ctx->active_reqs);
			if (ret) {
				pr_err("%s: failed while waiting for the CMDQ to be empty %s err (%d)\n",
					mmc_hostname(host),
					__func__, ret);
				BUG_ON(1);
			}
			/* clear the counter now */
			ctx->active_small_sector_read_reqs = 0;
			/*
			 * If there were small sector (less than 8 sectors) read
			 * operations in progress then we have to wait for the
			 * outstanding requests to finish and should also have
			 * atleast 6 microseconds delay before queuing the DCMD
			 * request.
			 */
			udelay(MMC_QUIRK_CMDQ_DELAY_BEFORE_DCMD);
		}

		if (cmd_flags & REQ_DISCARD) {
			if (cmd_flags & REQ_SECURE &&
			   !(card->quirks & MMC_QUIRK_SEC_ERASE_TRIM_BROKEN))
				ret = mmc_blk_cmdq_issue_secdiscard_rq(mq, req);
			else
				ret = mmc_blk_cmdq_issue_discard_rq(mq, req);
		} else if (cmd_flags & REQ_FLUSH) {
			ret = mmc_blk_cmdq_issue_flush_rq(mq, req);
		} else {
			ret = mmc_blk_cmdq_issue_rw_rq(mq, req);
		}
	}

	return ret;

out:
	if (req)
		blk_end_request_all(req, ret);
	mmc_put_card(card);

	return ret;
}

static int mmc_blk_issue_rq(struct mmc_queue *mq, struct request *req)
{
	int ret;
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	struct mmc_host *host = card->host;
	unsigned long flags;
	unsigned int cmd_flags = req ? req->cmd_flags : 0;

	if (req && !mq->mqrq_prev->req) {
		/* claim host only for the first request */
		mmc_get_card(card);
#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
		if (mmc_bus_needs_resume(card->host))
			mmc_resume_bus(card->host);
#endif
	}

	ret = mmc_blk_part_switch(card, md);
	if (ret) {
		if (req) {
			blk_end_request_all(req, -EIO);
		}
		ret = 0;
		goto out;
	}

	clear_bit(MMC_QUEUE_NEW_REQUEST, &mq->flags);
	if (cmd_flags & REQ_DISCARD) {
		/* complete ongoing async transfer before issuing discard */
		if (card->host->areq)
			mmc_blk_issue_rw_rq(mq, NULL);
		if (req->cmd_flags & REQ_SECURE)
			ret = mmc_blk_issue_secdiscard_rq(mq, req);
		else
			ret = mmc_blk_issue_discard_rq(mq, req);
	} else if (cmd_flags & (REQ_FLUSH | REQ_BARRIER)) {
		/* complete ongoing async transfer before issuing flush */
		if (card->host->areq)
			mmc_blk_issue_rw_rq(mq, NULL);
		ret = mmc_blk_issue_flush(mq, req);
	} else {
		if (!req && host->areq) {
			spin_lock_irqsave(&host->context_info.lock, flags);
			host->context_info.is_waiting_last_req = true;
			spin_unlock_irqrestore(&host->context_info.lock, flags);
		}
		ret = mmc_blk_issue_rw_rq(mq, req);
	}

out:
	if ((!req && !(test_bit(MMC_QUEUE_NEW_REQUEST, &mq->flags))) ||
	     (cmd_flags & MMC_REQ_SPECIAL_MASK))
		/*
		 * Release host when there are no more requests
		 * and after special request(discard, flush) is done.
		 * In case sepecial request, there is no reentry to
		 * the 'mmc_blk_issue_rq' with 'mqrq_prev->req'.
		 */
		mmc_put_card(card);
	return ret;
}

static inline int mmc_blk_readonly(struct mmc_card *card)
{
	return mmc_card_readonly(card) ||
	       !(card->csd.cmdclass & CCC_BLOCK_WRITE);
}

static struct mmc_blk_data *mmc_blk_alloc_req(struct mmc_card *card,
					      struct device *parent,
					      sector_t size,
					      bool default_ro,
					      const char *subname,
					      int area_type)
{
	struct mmc_blk_data *md;
	int devidx, ret;

	devidx = find_first_zero_bit(dev_use, max_devices);
	if (devidx >= max_devices)
		return ERR_PTR(-ENOSPC);
	__set_bit(devidx, dev_use);

	md = kzalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);
	if (!md) {
		ret = -ENOMEM;
		goto out;
	}

	/*
	 * !subname implies we are creating main mmc_blk_data that will be
	 * associated with mmc_card with dev_set_drvdata. Due to device
	 * partitions, devidx will not coincide with a per-physical card
	 * index anymore so we keep track of a name index.
	 */
	if (!subname) {
		md->name_idx = find_first_zero_bit(name_use, max_devices);
		__set_bit(md->name_idx, name_use);
	} else
		md->name_idx = ((struct mmc_blk_data *)
				dev_to_disk(parent)->private_data)->name_idx;

	md->area_type = area_type;

	/*
	 * Set the read-only status based on the supported commands
	 * and the write protect switch.
	 */
	md->read_only = mmc_blk_readonly(card);

	md->disk = alloc_disk(perdev_minors);
	if (md->disk == NULL) {
		ret = -ENOMEM;
		goto err_kfree;
	}

	spin_lock_init(&md->lock);
	INIT_LIST_HEAD(&md->part);
	md->usage = 1;

	ret = mmc_init_queue(&md->queue, card, &md->lock, subname, area_type);
	if (ret)
		goto err_putdisk;

	md->queue.issue_fn = mmc_blk_issue_rq;
	md->queue.data = md;

	md->disk->major	= MMC_BLOCK_MAJOR;
	md->disk->first_minor = devidx * perdev_minors;
	md->disk->fops = &mmc_bdops;
	md->disk->private_data = md;
	md->disk->queue = md->queue.queue;
	md->disk->driverfs_dev = parent;
	set_disk_ro(md->disk, md->read_only || default_ro);
	md->disk->flags = GENHD_FL_EXT_DEVT;
	if (area_type & (MMC_BLK_DATA_AREA_RPMB | MMC_BLK_DATA_AREA_BOOT))
		md->disk->flags |= GENHD_FL_NO_PART_SCAN;

	/*
	 * As discussed on lkml, GENHD_FL_REMOVABLE should:
	 *
	 * - be set for removable media with permanent block devices
	 * - be unset for removable block devices with permanent media
	 *
	 * Since MMC block devices clearly fall under the second
	 * case, we do not set GENHD_FL_REMOVABLE.  Userspace
	 * should use the block device creation/destruction hotplug
	 * messages to tell when the card is present.
	 */

	snprintf(md->disk->disk_name, sizeof(md->disk->disk_name),
		 "mmcblk%u%s", md->name_idx, subname ? subname : "");

	if (mmc_card_mmc(card))
		blk_queue_logical_block_size(md->queue.queue,
					     card->ext_csd.data_sector_size);
	else
		blk_queue_logical_block_size(md->queue.queue, 512);

	set_capacity(md->disk, size);

	if (mmc_host_cmd23(card->host)) {
		if ((mmc_card_mmc(card) &&
		     card->csd.mmca_vsn >= CSD_SPEC_VER_3) ||
		    (mmc_card_sd(card) &&
		     card->scr.cmds & SD_SCR_CMD23_SUPPORT &&
			 mmc_card_uhs(card)))
			md->flags |= MMC_BLK_CMD23;
	}

	if (mmc_card_mmc(card) &&
	    md->flags & MMC_BLK_CMD23 &&
	    ((card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN) ||
	     card->ext_csd.rel_sectors)) {
		md->flags |= MMC_BLK_REL_WR;
		blk_queue_flush(md->queue.queue, REQ_FLUSH | REQ_FUA);
	}

	if (card->cmdq_init) {
		md->flags |= MMC_BLK_CMD_QUEUE;
		md->queue.cmdq_complete_fn = mmc_blk_cmdq_complete_rq;
		md->queue.cmdq_issue_fn = mmc_blk_cmdq_issue_rq;
		md->queue.cmdq_error_fn = mmc_blk_cmdq_err;
		md->queue.cmdq_req_timed_out = mmc_blk_cmdq_req_timed_out;
		md->queue.cmdq_shutdown = mmc_blk_cmdq_shutdown;
	}

	if (mmc_card_mmc(card) && !card->cmdq_init &&
	    (area_type == MMC_BLK_DATA_AREA_MAIN) &&
	    (md->flags & MMC_BLK_CMD23) &&
	    card->ext_csd.packed_event_en) {
		if (!mmc_packed_init(&md->queue, card))
			md->flags |= MMC_BLK_PACKED_CMD;
	}

	return md;

 err_putdisk:
	put_disk(md->disk);
 err_kfree:
	kfree(md);
 out:
	return ERR_PTR(ret);
}

static struct mmc_blk_data *mmc_blk_alloc(struct mmc_card *card)
{
	sector_t size;

	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		size = card->ext_csd.sectors;
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		size = (typeof(sector_t))card->csd.capacity
			<< (card->csd.read_blkbits - 9);
	}

	return mmc_blk_alloc_req(card, &card->dev, size, false, NULL,
					MMC_BLK_DATA_AREA_MAIN);
}

static int mmc_blk_alloc_part(struct mmc_card *card,
			      struct mmc_blk_data *md,
			      unsigned int part_type,
			      sector_t size,
			      bool default_ro,
			      const char *subname,
			      int area_type)
{
	char cap_str[10];
	struct mmc_blk_data *part_md;

	part_md = mmc_blk_alloc_req(card, disk_to_dev(md->disk), size, default_ro,
				    subname, area_type);
	if (IS_ERR(part_md))
		return PTR_ERR(part_md);
	part_md->part_type = part_type;
	list_add(&part_md->part, &md->part);

	string_get_size((u64)get_capacity(part_md->disk), 512, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
	pr_info("%s: %s %s partition %u %s\n",
	       part_md->disk->disk_name, mmc_card_id(card),
	       mmc_card_name(card), part_md->part_type, cap_str);
	return 0;
}

/* MMC Physical partitions consist of two boot partitions and
 * up to four general purpose partitions.
 * For each partition enabled in EXT_CSD a block device will be allocatedi
 * to provide access to the partition.
 */

static int mmc_blk_alloc_parts(struct mmc_card *card, struct mmc_blk_data *md)
{
	int idx, ret = 0;

	if (!mmc_card_mmc(card))
		return 0;

	for (idx = 0; idx < card->nr_parts; idx++) {
		if (card->part[idx].size) {
			ret = mmc_blk_alloc_part(card, md,
				card->part[idx].part_cfg,
				card->part[idx].size >> 9,
				card->part[idx].force_ro,
				card->part[idx].name,
				card->part[idx].area_type);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static void mmc_blk_remove_req(struct mmc_blk_data *md)
{
	struct mmc_card *card;

	if (md) {
		/*
		 * Flush remaining requests and free queues. It
		 * is freeing the queue that stops new requests
		 * from being accepted.
		 */
		card = md->queue.card;
		mmc_cleanup_queue(&md->queue);
		if (md->flags & MMC_BLK_PACKED_CMD)
			mmc_packed_clean(&md->queue);
		if (md->flags & MMC_BLK_CMD_QUEUE)
			mmc_cmdq_clean(&md->queue, card);
		if (md->disk->flags & GENHD_FL_UP) {
			device_remove_file(disk_to_dev(md->disk), &md->force_ro);
			if ((md->area_type & MMC_BLK_DATA_AREA_BOOT) &&
					card->ext_csd.boot_ro_lockable)
				device_remove_file(disk_to_dev(md->disk),
					&md->power_ro_lock);
#ifdef CONFIG_MMC_SIMULATE_MAX_SPEED
			device_remove_file(disk_to_dev(md->disk),
						&dev_attr_max_write_speed);
			device_remove_file(disk_to_dev(md->disk),
						&dev_attr_max_read_speed);
			device_remove_file(disk_to_dev(md->disk),
						&dev_attr_cache_size);
#endif

			del_gendisk(md->disk);
		}
		mmc_blk_put(md);
	}
}

static void mmc_blk_remove_parts(struct mmc_card *card,
				 struct mmc_blk_data *md)
{
	struct list_head *pos, *q;
	struct mmc_blk_data *part_md;

	__clear_bit(md->name_idx, name_use);
	list_for_each_safe(pos, q, &md->part) {
		part_md = list_entry(pos, struct mmc_blk_data, part);
		list_del(pos);
		mmc_blk_remove_req(part_md);
	}
}

static int mmc_add_disk(struct mmc_blk_data *md)
{
	int ret;
	struct mmc_card *card = md->queue.card;

	add_disk(md->disk);
	md->force_ro.show = force_ro_show;
	md->force_ro.store = force_ro_store;
	sysfs_attr_init(&md->force_ro.attr);
	md->force_ro.attr.name = "force_ro";
	md->force_ro.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(disk_to_dev(md->disk), &md->force_ro);
	if (ret)
		goto force_ro_fail;
#ifdef CONFIG_MMC_SIMULATE_MAX_SPEED
	atomic_set(&md->queue.max_write_speed, max_write_speed);
	ret = device_create_file(disk_to_dev(md->disk),
			&dev_attr_max_write_speed);
	if (ret)
		goto max_write_speed_fail;
	atomic_set(&md->queue.max_read_speed, max_read_speed);
	ret = device_create_file(disk_to_dev(md->disk),
			&dev_attr_max_read_speed);
	if (ret)
		goto max_read_speed_fail;
	atomic_set(&md->queue.cache_size, cache_size);
	atomic_long_set(&md->queue.cache_used, 0);
	md->queue.cache_jiffies = jiffies;
	ret = device_create_file(disk_to_dev(md->disk), &dev_attr_c