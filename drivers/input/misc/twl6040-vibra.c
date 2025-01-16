ret != 0) {
		input_info(true,&info->client->dev, "%s failed to write press_threhold data2", __func__);
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
		input_err(true, &info->client->dev, "%s: failed to read thresold, buf is %s\n", __func__,buf);
		return count;
	}

	if(threshold > 0xff) {
		cmd[0] = (threshold >> 8) & 0xff;
		cmd[1] = 0xff & threshold;
	}else if(threshold < 0) {
		cmd[0] = 0x0;
		cmd[1] = 0x0;
	}else{
		cmd[0] = 0x0;
		cmd[1] = (u8)threshold;
	}

	input_info(true,&info->client->dev, "%s buf : %d, threshold : %d\n",
		__func__, threshold,(cmd[0] << 8) | cmd[1]);

	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD+0x02, &cmd[0], 1);
	input_info(true,&info->client->dev, "%s ret : %d\n", __func__,ret);

	if (ret != 0) {
		input_info(true,&info->client->dev, "%s failed to write release_threshold_data1", __func__);
		goto release_threshold_out;
	}
	ret = abov_tk_i2c_write(info->client, CMD_SAR_THRESHOLD+0x03, &cmd[1], 1);
	input_info(true,&info->client->dev, "%s ret : %d\n", __func__,ret);
	if (ret != 0) {
		input_info(true,&info->client->dev, "%s failed to write release_threshold_data2", __func__);
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
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	input_info(true, &info->client->dev, "%s data(%d)\n",__func__,data);

	abov_sar_only_mode(info, data);

	return count;
}
#endif

#endif

static ssize_t touchkey_chip_name(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	input_info(true, &client->dev, "%s\n", __func__);
#ifdef CONFIG_KEYBOARD_ABOV_TOUCH_T316
	return sprintf(buf, "A96T326\n");
#else
	return sprintf(buf, "FT1804\n");
#endif
}

static ssize_t bin_fw_ver(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	input_info(true, &client->dev, "fw version bin : 0x%x\n", info->fw_ver_bin);

	return sprintf(buf, "0x%02x\n", info->fw_ver_bin);
}

static int get_tk_fw_version(struct abov_tk_info *info, bool bootmode)
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
		if (retry <= 0)
			return -1;
	}

	info->fw_ver = buf;
	input_info(true, &client->dev, "%s : 0x%x\n", __func__, buf);
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

	return sprintf(buf, "0x%02x\n", info->fw_ver);
}

static int abov_load_fw(struct abov_tk_info *info, u8 cmd)
{
	struct i2c_client *client = info->client;
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;

	switch(cmd) {
	case BUILT_IN:
		ret = request_firmware(&info->firm_data_bin,
			info->pdata->fw_path, &client->dev);
		if (ret) {
			input_err(true, &client->dev,
				"%s request_firmware fail(%d)\n", __func__, cmd);
			return ret;
		}
		/* Header info
		* 0x00 0x91 : model info,
		* 0x00 0x00 : module info (Rev 0.0),
		* 0x00 0xF3 : F/W
		* 0x00 0x00 0x17 0x10 : checksum
		* ~ 22byte 0x00 */
		info->fw_model_number = info->firm_data_bin->data[1];
		info->fw_ver_bin = info->firm_data_bin->data[5];
		info->checksum_h_bin = info->firm_data_bin->data[8];
		info->checksum_l_bin = info->firm_data_bin->data[9];
		info->firm_size = info->firm_data_bin->size;

		input_info(true, &client->dev, "%s, bin version:%2X,%2X,%2X   crc:%2X,%2X\n", __func__, \
			info->firm_data_bin->data[1], info->firm_data_bin->data[3], info->fw_ver_bin, \
			info->checksum_h_bin, info->checksum_l_bin);
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
		info->checksum_h_bin = info->firm_data_ums[8];
		info->checksum_l_bin = info->firm_data_ums[9];

		input_info(true, &client->dev,"%s, bin version:%2X,%2X,%2X   crc:%2X,%2X\n", __func__, \
			info->firm_data_ums[1], info->firm_data_ums[3], info->firm_data_ums[5], \
			info->checksum_h_bin, info->checksum_l_bin);
		break;

	default:
		ret = -1;
		break;
	}
	input_info(true, &client->dev, "fw_size : %lu\n", info->firm_size);
	input_info(true, &client->dev, "%s success\n", __func__);
	return ret;

fail_sdcard_size:
	kfree(&info->firm_data_ums);
fail_sdcard_kzalloc:
	filp_close(fp, current->files);
fail_sdcard_open:
	set_fs(old_fs);
	return ret;
}

#if ABOV_ISP_FIRMUP_ROUTINE
void abov_i2c_start(int scl, int sda)
{
	gpio_direction_output(sda, 1);
	gpio_direction_output(scl, 1);
	usleep_range(15, 17);
	gpio_direction_output(sda, 0);
	usleep_range(10, 12);
	gpio_direction_output(scl, 0);
	usleep_range(10, 12);
}

void abov_i2c_stop(int scl, int sda)
{
	gpio_direction_output(scl, 0);
	usleep_range(10, 12);
	gpio_direction_output(sda, 0);
	usleep_range(10, 12);
	gpio_direction_output(scl, 1);
	usleep_range(10, 12);
	gpio_direction_output(sda, 1);
}

void abov_testdelay(void)
{
	u8 i;
	u8 delay;

	/* 120nms */
	for (i = 0; i < 15; i++)
		delay = 0;
}


void abov_byte_send(u8 data, int scl, int sda)
{
	u8 i;
	for (i = 0x80; i != 0; i >>= 1) {
		gpio_direction_output(scl, 0);
		usleep_range(1,1);

		if (data & i)
			gpio_direction_output(sda, 1);
		else
			gpio_direction_output(sda, 0);

		usleep_range(1,1);
		gpio_direction_output(scl, 1);
		usleep_range(1,1);
	}
	usleep_range(1,1);

	gpio_direction_output(scl, 0);
	gpio_direction_input(sda);
	usleep_range(1,1);

	gpio_direction_output(scl, 1);
	usleep_range(1,1);

	gpio_get_value(sda);
	abov_testdelay();

	gpio_direction_output(scl, 0);
	gpio_direction_output(sda, 0);
	usleep_range(20,20);
}

u8 abov_byte_read(bool type, int scl, int sda)
{
	u8 i;
	u8 data = 0;
	u8 index = 0x7;

	gpio_direction_output(scl, 0);
	gpio_direction_input(sda);
	usleep_range(1,1);

	for (i = 0; i < 8; i++) {
		gpio_direction_output(scl, 0);
		usleep_range(1,1);
		gpio_direction_output(scl, 1);
		usleep_range(1,1);

		data = data | (u8)(gpio_get_value(sda) << index);
		index -= 1;
	}
		usleep_range(1,1);
	gpio_direction_output(scl, 0);

	gpio_direction_output(sda, 0);
		usleep_range(1,1);

	if (type) { /*ACK */
		gpio_direction_output(sda, 0);
		usleep_range(1,1);
		gpio_direction_output(scl, 1);
		usleep_range(1,1);
		gpio_direction_output(scl, 0);
		usleep_range(1,1);
	} else { /* NAK */
		gpio_direction_output(sda, 1);
		usleep_range(1,1);
		gpio_direction_output(scl, 1);
		usleep_range(1,1);
		gpio_direction_output(scl, 0);
		usleep_range(1,1);
		gpio_direction_output(sda, 0);
		usleep_range(1,1);
	}
	usleep_range(20,20);

	return data;
}

void abov_enter_mode(int scl, int sda)
{
	abov_i2c_start(scl, sda);
	abov_testdelay();
	abov_byte_send(ABOV_ID, scl, sda);
	abov_byte_send(0xAC, scl, sda);
	abov_byte_send(0x5B, scl, sda);
	abov_byte_send(0x2D, scl, sda);
	abov_i2c_stop(scl, sda);
}

void abov_firm_write(const u8 *fw_data, int block, int scl, int sda)
{
	int i, j;
	u16 pos = 0x20;
	u8 addr[2];
	u8 data[32] = {0, };

	addr[0] = 0x10;
	addr[1] = 0x00;
	for (i = 0; i < (block - 0x20); i++) {
		if (i % 8 == 0) {
			addr[0] = 0x10 + i/8;
			addr[1] = 0;
		} else
			addr[1] = addr[1] + 0x20;
		memcpy(data, fw_data + pos, 32);
		abov_i2c_start(scl, sda);
		abov_testdelay();
		abov_byte_send(ABOV_ID, scl, sda);
		abov_byte_send(0xAC, scl, sda);
		abov_byte_send(0x7A, scl, sda);
		abov_byte_send(addr[0], scl, sda);
		abov_byte_send(addr[1], scl, sda);
		for (j = 0; j < 32; j++)
			abov_byte_send(data[j], scl, sda);
		abov_i2c_stop(scl, sda);

		pos += 0x20;

		abov_delay(3);
	}
}

void abov_read_address_set(int scl, int sda)
{
		abov_i2c_start(scl, sda);
		abov_testdelay();
		abov_byte_send(ABOV_ID, scl, sda);
		abov_byte_send(0xAC, scl, sda);
		abov_byte_send(0x9E, scl, sda);
		abov_byte_send(0x10, scl, sda); /* start addr H */
		abov_byte_send(0x00, scl, sda); /* start addr L */
		abov_byte_send(0x3F, scl, sda); /* end addr H  */
		abov_byte_send(0xFF, scl, sda); /* end addr L  */
		abov_i2c_stop(scl, sda);
}

void abov_checksum(struct abov_tk_info *info, int scl, int sda)
{
	struct i2c_client *client = info->client;

	u8 status;
	u8 bootver;
	u8 firmver;
	u8 checksumh;
	u8 checksuml;

	abov_read_address_set(scl, sda);
	abov_delay(5);

	abov_i2c_start(scl, sda);
	abov_testdelay();
	abov_byte_send(ABOV_ID, scl, sda);
	abov_byte_send(0x00, scl, sda);

	abov_i2c_start(scl, sda); /* restart */
	abov_testdelay();
	abov_byte_send(ABOV_ID + 1, scl, sda);
	status = abov_byte_read(true, scl, sda);
	bootver = abov_byte_read(true, scl, sda);
	firmver = abov_byte_read(true, scl, sda);
	checksumh = abov_byte_read(true, scl, sda);
	checksuml = abov_byte_read(false, scl, sda);
	abov_i2c_stop(