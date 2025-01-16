iled to read thresold, buf is %s\n",
				__func__,buf);
		return count;
	}

	if (threshold > 0xff) {
		cmd[0] = (threshold >> 8) & 0xff;
		cmd[1] = 0xff & threshold;
	} else if(threshold < 0) {
		cmd[0] = 0x0;
		cmd[1] = 0x0;
	} else {
		cmd[0] = 0x0;
		cmd[1] = (u8)threshold;
	}

	input_info(true, &info->client->dev, "%s buf : %d, threshold : %d\n",
			__func__, threshold,(cmd[0]<<8 )| cmd[1]);

	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD, &cmd[0], 1);
	if (ret) {
		input_err(true, &info->client->dev,
				"%s failed to write press_threhold data1", __func__);
		goto press_threshold_out;
	}

	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD + 0x01, &cmd[1], 1);
	if (ret) {
		input_err(true, &info->client->dev,
				"%s failed to write press_threhold data2", __func__);
		goto press_threshold_out;
	}

press_threshold_out:
	return count;
}

static ssize_t touchkey_sar_release_threshold_store(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	int ret;
	int threshold;
	u8 cmd[2];

	ret = sscanf(buf, "%d", &threshold);
	if (ret != 1) {
		input_err(true, &info->client->dev,
				"%s: failed to read thresold, buf is %s\n",
				__func__, buf);
		return count;
	}

	if (threshold > 0xff) {
		cmd[0] = (threshold >> 8) & 0xff;
		cmd[1] = 0xff & threshold;
	} else if (threshold < 0) {
		cmd[0] = 0x0;
		cmd[1] = 0x0;
	} else {
		cmd[0] = 0x0;
		cmd[1] = (u8)threshold;
	}

	input_info(true, &info->client->dev, "%s buf : %d, threshold : %d\n",
			__func__, threshold,(cmd[0] << 8) | cmd[1]);

	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD + 0x02, &cmd[0], 1);
	input_info(true, &info->client->dev, "%s ret : %d\n", __func__, ret);
	if (ret) {
		input_err(true, &info->client->dev,
				"%s failed to write release_threshold_data1", __func__);
		goto release_threshold_out;
	}

	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD + 0x03, &cmd[1], 1);
	input_info(true, &info->client->dev, "%s ret : %d\n", __func__, ret);
	if (ret) {
		input_err(true, &info->client->dev,
				"%s failed to write release_threshold_data2", __func__);
		goto release_threshold_out;
	}

release_threshold_out:
	return count;
}

static ssize_t touchkey_mode_change(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int ret, data;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 0 || data == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n", __func__, data);
		return count;
	}

	input_info(true, &info->client->dev, "%s data(%d)\n", __func__, data);

	abov_sar_only_mode(info, data);

	return count;
}
#endif

static ssize_t touchkey_grip_irq_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	int result = 0;

	if (info->irq_count)
		result = -1;

	input_info(true, &info->client->dev, "%s - called\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		result, info->irq_count, info->max_diff);
}

static ssize_t touchkey_grip_irq_count_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 onoff;
	int ret;

	ret = kstrtou8(buf, 10, &onoff);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s - kstrtou8 failed.(%d)\n", __func__, ret);
		return count;
	}

	mutex_lock(&info->lock);

	if (onoff == 0) {
		info->abnormal_mode = 0;
	} else if (onoff == 1) {
		info->abnormal_mode = 1;
		info->irq_count = 0;
		info->max_diff = 0;
	} else {
		input_err(true, &info->client->dev, "%s - unknown value %d\n", __func__, onoff);
	}

	mutex_unlock(&info->lock);

	input_info(true, &info->client->dev, "%s - %d\n", __func__, onoff);
	
	return count;
}
#endif

static ssize_t touchkey_chip_name(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	input_info(true, &client->dev, "%s\n", __func__);

	return sprintf(buf, "A96T3X6\n");
}

static ssize_t bin_fw_ver(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	input_info(true, &client->dev, "fw version bin : 0x%x\n", info->fw_ver_bin);

	return snprintf(buf, PAGE_SIZE, "0x%02x\n", info->fw_ver_bin);
}

int get_tk_fw_version(struct abov_tk_info *info, bool bootmode)
{
	struct i2c_client *client = info->client;
	u8 buf;
	int ret;
	int retry = 3;

	ret = abov_tk_i2c_read(client, ABOV_FW_VER, &buf, 1);
	if (ret < 0) {
		while (retry--) {
			input_err(true, &client->dev, "%s read fail(%d)\n",
				__func__, retry);
			if (!bootmode)
				abov_tk_reset(info);
			else
				return -1;
			ret = abov_tk_i2c_read(client, ABOV_FW_VER, &buf, 1);
			if (ret == 0)
				break;
		}
		if (retry == 0)
			return -1;
	}

	info->fw_ver = buf;

	retry = 3;
	ret = abov_tk_i2c_read(client, ABOV_MODEL_NO, &buf, 1);
	if (ret < 0) {
		while (retry--) {
			input_err(true, &client->dev, "%s read fail(%d)\n",
				__func__, retry);
			if (!bootmode)
				abov_tk_reset(info);
			else
				return -1;
			ret = abov_tk_i2c_read(client, ABOV_MODEL_NO, &buf, 1);
			if (ret == 0)
				break;
		}
		if (retry == 0)
			return -1;
	}

	info->md_ver = buf;
	input_info(true, &client->dev, "%s : fw = 0x%x, md = 0x%x\n",
		__func__, info->fw_ver, info->md_ver);
	return 0;
}

static ssize_t read_fw_ver(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int ret;

	ret = get_tk_fw_version(info, false);
	if (ret < 0) {
		input_err(true, &client->dev, "%s read fail\n", __func__);
		info->fw_ver = 0;
	}

	return snprintf(buf, PAGE_SIZE, "0x%02x\n", info->fw_ver);
}

static int abov_load_fw_kernel(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	ret = request_firmware(&info->firm_data_bin,
		info->dtdata->fw_name, &client->dev);
	if (ret) {
		input_err(true, &client->dev,
			"%s request_firmware fail.\n", __func__);
		return ret;
	}
	info->firm_size = info->firm_data_bin->size;
	info->fw_ver_bin = info->firm_data_bin->data[5];
	info->md_ver_bin = info->firm_data_bin->data[1];
	input_info(true, &client->dev, "%s : fw = 0x%x, md = 0x%x\n",
		__func__, info->fw_ver_bin, info->md_ver_bin);

	info->checksum_h_bin = info->firm_data_bin->data[8];
	info->checksum_l_bin = info->firm_data_bin->data[9];

	input_info(true, &client->dev, "%s : crc 0x%x 0x%x\n",
		__func__, info->checksum_h_bin, info->checksum_l_bin);

	return ret;
}

static int abov_load_fw(struct abov_tk_info *info, u8 cmd)
{
	struct i2c_client *client = info->client;
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;

	switch (cmd) {
	case BUILT_IN:
		break;

	case SDCARD:
		old_fs = get_fs();
		set_fs(get_ds());
		fp = filp_open(TK_FW_PATH_SDCARD, O_RDONLY, S_IRUSR);
		if (IS_ERR(fp)) {
			input_err(true, &client->dev,
				"%s %s open error\n", __func__, TK_FW_PATH_SDCARD);
			ret = -ENOENT;
			goto fail_sdcard_open;
		}

		fsize = fp->f_path.dentry->d_inode->i_size;
		info->firm_data_ums = kzalloc((size_t)fsize, GFP_KERNEL);
		if (!info->firm_data_ums) {
			input_err(true, &client->dev,
				"%s fail to kzalloc for fw\n", __func__);
			ret = -ENOMEM;
			goto fail_sdcard_kzalloc;
		}

		nread = vfs_read(fp,
			(char __user *)info->firm_data_ums, fsize, &fp->f_pos);
		if (nread != fsize) {
			input_err(true, &client->dev,
				"%s fail to vfs_read file\n", __func__);
			ret = -EINVAL;
			goto fail_sdcard_size;
		}
		filp_close(fp, current->files);
		set_fs(old_fs);
		info->firm_size = nread;
		break;

	default:
		ret = -1;
		break;
	}
	input_info(true, &client->dev, "fw_size : %lu\n", info->firm_size);
	input_info(true, &client->dev, "%s success\n", __func__);
	return ret;

fail_sdcard_size:
	kfree(