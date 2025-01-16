(mmc_dev(card->host), "%s: data error %d\n",
					__func__, data.error);
			err = data.error;
			goto curr_data_free;
		}

		if (!srpmb) {
			if (copy_to_user(&(ic_ptr->cmds[i].response), cmd.resp,
						sizeof(cmd.resp))) {
				err = -EFAULT;
				goto curr_data_free;
			}

			if (!curr_cmd->write_flag) {
				if (copy_to_user((void __user *)(unsigned long)
							curr_cmd->data_ptr,
							curr_data->buf,
							curr_data->buf_bytes)) {
					err = -EFAULT;
					goto curr_data_free;
				}
			}
		} else {
			memcpy(icmd->response, cmd.resp, sizeof(cmd.resp));

			if (!curr_cmd->write_flag)
				memcpy((void *)icmd->data_ptr,
					curr_data->buf,
					curr_data->buf_bytes);
		}

		/*
		 * Ensure RPMB command has completed by polling CMD13
		 * "Send Status".
		 */
		err = ioctl_rpmb_card_status_poll(card, &status, 5);
		if (err)
			dev_err(mmc_dev(card->host),
					"%s: Card Status=0x%08X, error %d\n",
					__func__, status, err);
curr_data_free:
		if (srpmb) {
			kfree(curr_data->buf);
			kfree(curr_data);
			break;
		}

		if (err)
			break;
	}

cmd_rel_host:
	mmc_put_card(card);

idata_free:
	if (!srpmb) {
		for (i = 0; i < MMC_IOC_MAX_RPMB_CMD; i++) {
			kfree(idata->data[i]->buf);
			kfree(idata->data[i]);
		}
		kfree(idata);
	}

cmd_done:
	mmc_blk_put(md);
	if (card->cmdq_init)
		wake_up(&card->host->cmdq_ctx.wait);
	return err;
}

static int mmc_blk_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case MMC_IOC_CMD:
		return mmc_blk_ioctl_cmd(bdev,
				(struct mmc_ioc_cmd __user *)arg);
	case MMC_IOC_MULTI_CMD:
		return mmc_blk_ioctl_multi_cmd(bdev,
				(struct mmc_ioc_multi_cmd __user *)arg);
	case MMC_IOC_RPMB_CMD:
		return mmc_blk_ioctl_rpmb_cmd(bdev,
				(struct mmc_ioc_rpmb __user *)arg, NULL);
	default:
		return -EINVAL;
	}
}

#ifdef CONFIG_MMC_SRPMB
static int mmc_blk_srpmb_access(struct block_device *bdev, struct mmc_ioc_cmd *icmd)
{
	int ret = -EINVAL;
	ret = mmc_blk_ioctl_rpmb_cmd(bdev, NULL, icmd);
	return ret;
}

static void mmc_blk_get_card(struct block_device *bdev, bool get)
{
	struct gendisk *disk;
	struct mmc_blk_data *md;
	struct mmc_card *card;

	disk = bdev->bd_disk;
	md = mmc_blk_get(disk);
	if(!md)
	{
		pr_err("%s: mmc block data is invalid\n", __func__);
		return;
	}
	card = md->queue.card;

	if (get)
		mmc_get_card(card);
	else
		mmc_put_card(card);
}

#endif

#ifdef CONFIG_COMPAT
static int mmc_blk_compat_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	return mmc_blk_ioctl(bdev, mode, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static const struct block_device_operations mmc_bdops = {
	.open			= mmc_blk_open,
	.release		= mmc_blk_release,
	.getgeo			= mmc_blk_getgeo,
	.owner			= THIS_MODULE,
	.ioctl			= mmc_blk_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= mmc_blk_compat_ioctl,
#endif
#ifdef CONFIG_MMC_SRPMB
	.srpmb_access		= mmc_blk_srpmb_access,
	.get_card		= mmc_blk_get_card,
#endif
};

static int mmc_blk_cmdq_switch(struct mmc_card *card,
			       struct mmc_blk_data *md, bool enable)
{
	int ret = 0;
	bool cmdq_mode = !!mmc_card_cmdq(card);
	struct mmc_host *host = card->host;
	struct mmc_cmdq_context_info *ctx = &host->cmdq_ctx;

	if (!(card->host->caps2 & MMC_CAP2_CMD_QUEUE) ||
	    !card->ext_csd.cmdq_support ||
	    (enable && !(md->flags & MMC_BLK_CMD_QUEUE)) ||
	    (cmdq_mode == enable))
		return 0;

	if (enable) {
		ret = mmc_set_blocklen(card, MMC_CARD_CMDQ_BLK_SIZE);
		if (ret) {
			pr_err("%s: failed (%d) to set block-size to %d\n",
			       __func__, ret, MMC_CARD_CMDQ_BLK_SIZE);
			goto out;
		}

	} else {
		if (!test_bit(CMDQ_STATE_HALT, &ctx->curr_state)) {
			ret = mmc_cmdq_halt(host, true);
			if (ret) {
				pr_err("%s: halt: failed: %d\n",
					mmc_hostname(host), ret);
				goto out;
			}
		}
	}

	ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			 EXT_CSD_CMDQ, enable,
			 card->ext_csd.generic_cmd6_time);
	if (ret) {
		pr_err("%s: cmdq mode %sable failed %d\n",
		       md->disk->disk_name, enable ? "en" : "dis", ret);
		goto out;
	}

	if (enable) {
		mmc_card_set_cmdq(card);
		host->cmdq_ops->enable(host);
	} else {
		mmc_card_clr_cmdq(card);
		host->cmdq_ops->disable(host, true);
	}
out:
	return ret;
}

static inline int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md)
{
	int ret;
	struct mmc_blk_data *main_md = dev_get_drvdata(&card->dev);
	u8* ext_csd;
	int status;

	if ((main_md->part_curr == md->part_type) &&
	    (card->part_curr == md->part_type))
		return 0;

	if (mmc_card_mmc(card)) {
		u8 part_config = card->ext_csd.part_config;

		if (md->part_type) {
			/* disable CQ mode for non-user data partitions */
			ret = mmc_blk_cmdq_switch(card, md, false);
			if (ret){
				if(md->part_type == EXT_CSD_PART_CONFIG_ACC_RPMB) 
					mmc_cmdq_error_logging(card, NULL, RPMB_SWITCH_ERR);
			
				return ret;
			}
		}

		part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		part_config |= md->part_type;

		ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_PART_CONFIG, part_config,
				 card->ext_csd.part_time);
		if (ret) {
			if(md->part_type == EXT_CSD_PART_CONFIG_ACC_RPMB) 
				mmc_cmdq_error_logging(card, NULL, RPMB_SWITCH_ERR);

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
	}

	main_md->part_curr = md->part_type;
	return 0;
}

static u32 mmc_sd_num_wr_blocks(struct mmc_card *card)
{
	int err;
	u32 result;
	__be32 *blocks;

	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};

	struct scatterlist sg;

	cmd.opcode = MMC_APP_CMD;
	cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		return (u32)-1;
	if (!mmc_host_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
		return (u32)-1;

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = SD_APP_SEND_NUM_WR_BLKS;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = 4;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;
	mmc_set_data_timeout(&data, card);

	mrq.cmd = &cmd;
	mrq.data = &data;

	blocks = kmalloc(4, GFP_KERNEL);
	if (!blocks)
		return (u32)-1;

	sg_init_one(&sg, blocks, 4);

	mmc_wait_for_req(card->host, &mrq);

	result = ntohl(*blocks);
	kfree(blocks);

	if (cmd.error || data.error)
		result = (u32)-1;

	return result;
}

static int get_card_status(struct mmc_card *card, u32 *status, int retries)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_SEND_STATUS;
	if (!mmc_host_is_spi(card->host))
		cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, retries);
	if (err == 0)
		*status = cmd.resp[0];
	return err;
}

#define CMD_ERRORS							\
	(R1_OUT_OF_RANGE |	/* Command argument out of range */	\
	 R1_ADDRESS_ERROR |	/* Misaligned address */		\
	 R1_BLOCK_LEN_ERROR |	/* Transferred block length incorrect */\
	 R1_WP_VIOLATION |	/* Tried to write to protected block */	\
	 R1_CC_ERROR |		/* Card controller error */		\
	 R1_ERROR)		/* General/unknown error */

static void mmc_error_count_log(struct mmc_card *card, int index, int error, u32 status)
{
	struct mmc_card_error_log *err_log;
	int i = 0;
	int cpu = raw_smp_processor_id();

	err_log = card->err_log;

	for (i = 0; i < 2; i++)
	{
		if (err_log[index + i].err_type == error) {
			index += i;
			break;
		}
	}

	if (i >= 2)
		return;

	if (!err_log[index].status)
		err_log[index].status = status;
	if (!err_log[index].first_issue_time)
		err_log[index].first_issue_time = cpu_clock(cpu);
	err_log[index].last_issue_time = cpu_clock(cpu);
	err_log[index].count++;
}

static void mmc_card_error_logging(struct mmc_card *card, struct mmc_blk_request *brq, u32 status)
{
	struct mmc_card_error_log *err_log;
	int index = 0;
	int ret = 0;
	bool noti = false;

	err_log = card->err_log;

	if (!brq)
		return;

	if (status & STATUS_MASK || brq->stop.resp[0] & STATUS_MASK || brq->cmd.resp[0] & STATUS_MASK)
	{
		if(status & R1_ERROR || brq->stop.resp[0] & R1_ERROR || brq->cmd.resp[0] & R1_ERROR) {
			err_log[index].ge_cnt++;
			if (!(err_log[index].ge_cnt % 1000))
				noti = true;
		}
		if(status & R1_CC_ERROR || brq->stop.resp[0] & R1_CC_ERROR || brq->cmd.resp[0] & R1_CC_ERROR)
			err_log[index].cc_cnt++;
		if(status & R1_CARD_ECC_FAILED || brq->stop.resp[0] & R1_CARD_ECC_FAILED || brq->cmd.resp[0] & R1_CARD_ECC_FAILED) {
			err_log[index].ecc_cnt++;
			if (!(err_log[index].ecc_cnt % 1000))
				noti = true;
		}
		if(status & R1_WP_VIOLATION || brq->stop.resp[0] & R1_WP_VIOLATION || brq->cmd.resp[0] & R1_WP_VIOLATION) {
			err_log[index].wp_cnt++;
			if (!(err_log[index].wp_cnt % 100))
				noti = true;
		}
		if(status & R1_OUT_OF_RANGE || brq->stop.resp[0] & R1_OUT_OF_RANGE || brq->cmd.resp[0] & R1_OUT_OF_RANGE) {
			err_log[index].oor_cnt++;
			if (!(err_log[index].oor_cnt % 100))
				noti = true;
		}
	}
	
	/*
	 * Make Notification about SD Card Errors
	 *
	 * Condition :
	 *   GE, ECC : Every 1000 errors
	 *   WP, OOR : Every  100 errors
	 */
 	if (noti && card->type == MMC_TYPE_SD && card->host->sdcard_uevent) {
		ret = card->host->sdcard_uevent(card);
		if (ret)
			pr_err("%s: Failed to Send Uevent with err %d\n",
					mmc_hostname(card->host), ret);
		else
			card->err_log[0].noti_cnt++;
	}

	if (brq->sbc.error)
		mmc_error_count_log(card, index, brq->sbc.error, status);
	if (brq->cmd.error) {
		index = 2;
		mmc_error_count_log(card, index, brq->cmd.error, status);
	}
	if (brq->data.error) {
		index = 4;
		mmc_error_count_log(card, index, brq->data.error, status);
	}
	if (brq->stop.error) {
		index = 6;
		mmc_error_count_log(card, index, brq->stop.error, status);
	}
	
	/* card stuck in prg state */
	if (!(status & R1_READY_FOR_DATA) ||
		(R1_CURRENT_STATE(status) == R1_STATE_PRG)) {
		index = 8;
		mmc_error_count_log(card, index, -ETIMEDOUT, status);
	}

	return;
}

void mmc_cmdq_error_logging(struct mmc_card *card,
		struct mmc_cmdq_req *cqrq, u32 status)
{
	int index = 0;
	struct mmc_card_error_log *err_log = card->err_log;

	if (status & STATUS_MASK  ||
			status & CQERR_MASK)
	{
		if (status & R1_ERROR)
			err_log[index].ge_cnt++;
		if (status & R1_CC_ERROR)
			err_log[index].cc_cnt++;
		if (status & R1_CARD_ECC_FAILED)
			err_log[index].ecc_cnt++;
		if (status & R1_WP_VIOLATION)
			err_log[index].wp_cnt++;
		if (status & R1_OUT_OF_RANGE)
			err_log[index].oor_cnt++;
		if (status & HALT_UNHALT_ERR)
			err_log[index].halt_cnt++;
		if (status & CQ_EN_DIS_ERR)
			err_log[index].cq_cnt++;
		if (status & RPMB_SWITCH_ERR)
			err_log[index].rpmb_cnt++;
		if (status & CQ_HW_RST) {
			err_log[index].hw_rst_cnt++;
#if defined(CONFIG_SEC_ABC) 
			if ((err_log[index].hw_rst_cnt % 20) == 0)
				sec_abc_send_event("MODULE=storage@ERROR=mmc_hwreset_err");
#endif
		}
	}

	if (!cqrq)
		return;

	if (cqrq->cmd.error) {
		index = 2;
		mmc_error_count_log(card, index, cqrq->cmd.error, status);
	}
	if (cqrq->data.error) {
		index = 4;
		mmc_error_count_log(card, index, cqrq->data.error, status);
	}
}
EXPORT_SYMBOL(mmc_cmdq_error_logging);


static ssize_t error_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gendisk *disk;
	struct mmc_blk_data *md;
	struct mmc_card *card = NULL;
	struct mmc_card_error_log *err_log;
	int total_len = 0;
	int i = 0;
	u64 total_c_cnt = 0;
	u64 total_t_cnt = 0;

	disk = dev_to_disk(dev);

	if (disk) {
		md = disk->private_data;
		if (md)
			card = md->queue.card;
	}
	if (!card) {
		total_len = snprintf(buf, PAGE_SIZE, "It's no card error..\n");
		goto out;
	}

	err_log = card->err_log;

	total_len += snprintf(buf, PAGE_SIZE,
			"type: err    status: first_issue_time:  last_issue_time:      count\n");

	for (i = 0; i < 10; i++) {
		total_len += snprintf(buf + (sizeof(char)*68*(i+1)), PAGE_SIZE,
				"%4s:%4d 0x%08x %16llu, %16llu, %10d\n",
				err_log[i].type, err_log[i].err_type,
				err_log[i].status,
				err_log[i].first_issue_time,
				err_log[i].last_issue_time,
				err_log[i].count);
	}

	for (i = 0; i < 6; i++) {
		if (err_log[i].err_type == -EILSEQ && total_c_cnt < MAX_CNT_U64)
			total_c_cnt += err_log[i].count;
		if (err_log[i].err_type == -ETIMEDOUT && total_t_cnt < MAX_CNT_U64)
			total_t_cnt += err_log[i].count;
	}

	total_len += snprintf(buf + total_len, PAGE_SIZE,
						   "GE:%d,CC:%d,ECC:%d,WP:%d,OOR:%d,CRC:%lld,TMO:%lld,HALT:%d,CQEN:%d,RPMB:%d\n",
						   err_log[0].ge_cnt, err_log[0].cc_cnt, err_log[0].ecc_cnt,
						   err_log[0].wp_cnt, err_log[0].oor_cnt, total_c_cnt, total_t_cnt,
						   err_log[0].halt_cnt, err_log[0].cq_cnt, err_log[0].rpmb_cnt);
out:
	return total_len;
}

static ssize_t error_count_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct gendisk *disk;
	struct mmc_blk_data *md;
	struct mmc_card *card = NULL;
	int value;

	disk = dev_to_disk(dev);

	if (disk) {
		md = disk->private_data;
		if (md)
			card = md->queue.card;
	}
	if (!card)
		goto out;

	if (kstrtoint(buf, 0, &value))
		goto out;

out:
	return len;
}

static void mmc_card_debug_log_sysfs_init(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	card->error_count.show = error_count_show;
	card->error_count.store = error_count_store;
	sysfs_attr_init(&card->error_count.attr);
	card->error_count.attr.name = "err_count";

	card->error_count.attr.mode = S_IRUGO | S_IWUSR;

	if (device_create_file((disk_to_dev(md->disk)), &card->error_count)) {
		pr_err("%s: Failed to create err_count sysfs entry\n",
				mmc_hostname(card->host));
		return;
	}
	/* init. card->err_log */
	snprintf(card->err_log[0].type, sizeof(char)*5, "sbc ");
	snprintf(card->err_log[1].type, sizeof(char)*5, "sbc ");
	card->err_log[0].err_type = -EILSEQ;
	card->err_log[1].err_type = -ETIMEDOUT;

	snprintf(card->err_log[2].type, sizeof(char)*5, "cmd ");
	snprintf(card->err_log[3].type, sizeof(char)*5, "cmd ");
	card->err_log[2].err_type = -EILSEQ;
	card->err_log[3].err_type = -ETIMEDOUT;

	snprintf(card->err_log[4].type, sizeof(char)*5, "data");
	snprintf(card->err_log[5].type, sizeof(char)*5, "data");
	card->err_log[4].err_type = -EILSEQ;
	card->err_log[5].err_type = -ETIMEDOUT;

	snprintf(card->err_log[6].type, sizeof(char)*5, "stop");
	snprintf(card->err_log[7].type, sizeof(char)*5, "stop");
	card->err_log[6].err_type = -EILSEQ;
	card->err_log[7].err_type = -ETIMEDOUT;

	snprintf(card->err_log[8].type, sizeof(char)*5, "busy");
	snprintf(card->err_log[9].type, sizeof(char)*5, "busy");
	card->err_log[8].err_type = -EILSEQ;
	card->err_log[9].err_type = -ETIMEDOUT;
}

static int card_busy_detect(struct mmc_card *card, unsigned int timeout_ms,
		bool hw_busy_detect, struct request *req, int *gen_err)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(timeout_ms);
	int err = 0;
	u32 status;
	struct mmc_queue_req *mq_mrq;
	struct mmc_blk_request *brq = NULL;

	if (card->host->areq) {
		mq_mrq = container_of(card->host->areq, struct mmc_queue_req,
				mmc_active);
		brq = &mq_mrq->brq;
	}

	do {
		err = get_card_status(card, &status, 5);
		if (err) {
			pr_err("%s: error %d requesting status\n",
			       req->rq_disk->disk_name, err);
			return err;
		}

		if (status & R1_ERROR) {
			pr_err("%s: %s: error sending status cmd, status %#x\n",
				req->rq_disk->disk_name, __func__, status);
			*gen_err = 1;
		}

		if (status & CMD_ERRORS) {
			pr_err("%s: %s: command error reported, status %#x\n",
					req->rq_disk->disk_name, __func__, status);

			if (mmc_card_sd(card) && brq)
				brq->data.error = -EIO;
			else {
				if (!(status & R1_WP_VIOLATION) && brq)
					brq->data.error = -EIO;
			}

			mmc_card_error_logging(card, brq, status);

			if ((R1_CURRENT_STATE(status) == R1_STATE_RCV) ||
					(R1_CURRENT_STATE(status) == R1_STATE_DATA)) {
				struct mmc_command cmd = {0};

				cmd.opcode = MMC_STOP_TRANSMISSION;
				cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
				cmd.busy_timeout = timeout_ms;
				err = mmc_wait_for_cmd(card->host, &cmd, 5);

				if (err)
					pr_err("%s: error %d sending stop command\n",
							req->rq_disk->disk_name, err);
				else
					status = cmd.resp[0];

				/*
				 * If the stop cmd also timed out, the card is probably
				 * not present, so abort. Other errors are bad news too.
				 */
				if (err)
					return -EBUSY;
			}
		}

		/* We may rely on the host hw to handle busy detection.*/
		if ((card->host->caps & MMC_CAP_WAIT_WHILE_BUSY) &&
			hw_busy_detect)
			break;

		/*
		 * Timeout if the device never becomes ready for data and never
		 * leaves the program state.
		 */
		if (time_after(jiffies, timeout)) {
			pr_err("%s: Card stuck in programming state! %s %s\n",
				mmc_hostname(card->host),
				req->rq_disk->disk_name, __func__);
			mmc_card_error_logging(card, brq, status);
			return -ETIMEDOUT;
		}

		/*
		 * Some cards mishandle the status bits,
		 * so make sure to check both the busy
		 * indication and the card state.
		 */
	} while (!(status & R1_READY_FOR_DATA) ||
		 (R1_CURRENT_STATE(status) == R1_STATE_PRG));

	return err;
}

static int send_stop(struct mmc_card *card, unsigned int timeout_ms,
		struct request *req, int *gen_err, u32 *stop_status)
{
	struct mmc_host *host = card->host;
	struct mmc_command cmd = {0};
	int err;
	bool use_r1b_resp = rq_data_dir(req) == WRITE;

	/*
	 * Normally we use R1B responses for WRITE, but in cases where the host
	 * has specified a max_busy_timeout we need to validate it. A failure
	 * means we need to prevent the host from doing hw busy detection, which
	 * is done by converting to a R1 response instead.
	 */
	if (host->max_busy_timeout && (timeout_ms > host->max_busy_timeout))
		use_r1b_resp = false;

	cmd.opcode = MMC_STOP_TRANSMISSION;
	if (use_r1b_resp) {
		cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
		cmd.busy_timeout = timeout_ms;
	} else {
		cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	}

	err = mmc_wait_for_cmd(host, &cmd, 5);
	if (err)
		return err;

	*stop_status = cmd.resp[0];

	/* No need to check card status in case of READ. */
	if (rq_data_dir(req) == READ)
		return 0;

	if (!mmc_host_is_spi(host) &&
		(*stop_status & R1_ERROR)) {
		pr_err("%s: %s: general error sending stop command, resp %#x\n",
			req->rq_disk->disk_name, __func__, *stop_status);
		*gen_err = 1;
	}

	return card_busy_detect(card, timeout_ms, use_r1b_resp, req, gen_err);
}

#define ERR_NOMEDIUM	3
#define ERR_RETRY	2
#define ERR_ABORT	1
#define ERR_CONTINUE	0

static int mmc_blk_cmd_error(struct request *req, const char *name, int error,
	bool status_valid, u32 status)
{
	switch (error) {
	case -EILSEQ:
		/* response crc error, retry the r/w cmd */
		pr_err("%s: %s sending %s command, card status %#x\n",
			req->rq_disk->disk_name, "response CRC error",
			name, status);
		return ERR_RETRY;

	case -ETIMEDOUT:
		pr_err("%s: %s sending %s command, card status %#x\n",
			req->rq_disk->disk_name, "timed out", name, status);

		/* If the status cmd initially failed, retry the r/w cmd */
		if (!status_valid) {
			pr_err("%s: status not valid, retrying timeout\n", req->rq_disk->disk_name);
			return ERR_RETRY;
		}
		/*
		 * If it was a r/w cmd crc error, or illegal command
		 * (eg, issued in wrong state) then retry - we should
		 * have corrected the state problem above.
		 */
		if (status & (R1_COM_CRC_ERROR | R1_ILLEGAL_COMMAND)) {
			pr_err("%s: command error, retrying timeout\n", req->rq_disk->disk_name);
			return ERR_RETRY;
		}

		/* Otherwise abort the command */
		pr_err("%s: not retrying timeout\n", req->rq_disk->disk_name);
		return ERR_ABORT;

	default:
		/* We don't understand the error code the driver gave us */
		pr_err("%s: unknown error %d sending read/write command, card status %#x\n",
		       req->rq_disk->disk_name, error, status);
		return ERR_ABORT;
	}
}

/*
 * Initial r/w and stop cmd error recovery.
 * We don't know whether the card received the r/w cmd or not, so try to
 * restore things back to a sane state.  Essentially, we do this as follows:
 * - Obtain card status.  If the first attempt to obtain card status fails,
 *   the status word will reflect the failed status cmd, not the failed
 *   r/w cmd.  If we fail to obtain card status, it suggests we can no
 *   longer communicate with the card.
 * - Check the card state.  If the card received the cmd but there was a
 *   transient problem with the response, it might still be in a data transfer
 *   mode.  Try to send it a stop command.  If this fails, we can't recover.
 * - If the r/w cmd failed due to a response CRC error, it was probably
 *   transient, so retry the cmd.
 * - If the r/w cmd timed out, but we didn't get the r/w cmd status, retry.
 * - If the r/w cmd timed out, and the r/w cmd failed due to CRC error or
 *   illegal cmd, retry.
 * Otherwise we don't understand what happened, so abort.
 */
static int mmc_blk_cmd_recovery(struct mmc_card *card, struct request *req,
	struct mmc_blk_request *brq, int *ecc_err, int *gen_err)
{
	bool prev_cmd_status_valid = true;
	u32 status, stop_status = 0;
	int err, retry;

	if (mmc_card_removed(card))
		return ERR_NOMEDIUM;

	/*
	 * Try to get card status which indicates both the card state
	 * and why there was no response.  If the first attempt fails,
	 * we can't be sure the returned status is for the r/w command.
	 */
	for (retry = 2; retry >= 0; retry--) {
		err = get_c