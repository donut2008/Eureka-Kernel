data, 6);
	abov_delay(5);

	i2c_master_send(info->client, data2, 1);
	abov_delay(5);

	ret = abov_tk_i2c_read_data(info->client, checksum, 6);

	input_info(true, &info->client->dev, "%s: ret:%d [%X][%X][%X][%X][%X]\n",
			__func__, ret, checksum[0], checksum[1], checksum[2]
			, checksum[4], checksum[5]);
	info->checksum_h = checksum[4];
	info->checksum_l = checksum[5];
	return 0;
}

static int abov_tk_fw_write(struct abov_tk_info *info, unsigned char *addrH,
						unsigned char *addrL, unsigned char *val)
{
	int length = 36, ret = 0;
	unsigned char data[36];

	data[0] = 0xAC;
	data[1] = 0x7A;
	memcpy(&data[2], addrH, 1);
	memcpy(&data[3], addrL, 1);
	memcpy(&data[4], val, 32);

	ret = i2c_master_send(info->client, data, length);
	if (ret != length) {
		input_err(true, &info->client->dev,
			"%s: write fail[%x%x], %d\n", __func__, *addrH, *addrL, ret);
		return ret;
	}

	abov_delay(3);

	abov_tk_check_busy(info);

	return 0;
}

static int abov_tk_fw_mode_enter(struct abov_tk_info *info)
{
	unsigned char data[2] = {0xAC, 0x5B};
	int ret = 0;

	ret = i2c_master_send(info->client, data, 2);
	if (ret != 2) {
		input_err(true, &info->client->dev, "%s: write fail\n", __func__);
		return -1;
	}

	return 0;

}


static int abov_tk_fw_mode_check(struct abov_tk_info *info)
{
	unsigned char buf[1] = {0};
	int ret;

	ret = abov_tk_i2c_read_data(info->client, buf, 1);
	if (ret < 0){
		input_err(true, &info->client->dev, "%s: write fail\n", __func__);
		return 0;
	}

	input_info(true, &info->client->dev, "%s: ret:%02X\n",__func__, buf[0]);

#ifdef CONFIG_KEYBOARD_ABOV_TOUCH_T316
	if ((buf[0] == ABOV_FLASH_MODE) || (buf[0] == 0x32)) /* support T316, T326 */
		return 1;
#else
	if (buf[0] == ABOV_FLASH_MODE)
		return 1;
#endif

	input_err(true, &info->client->dev, "%s: flash mode does not match,  %X, %X\n",
			__func__, ABOV_FLASH_MODE, buf[0]);

	return 0;
}

static int abov_tk_flash_erase(struct abov_tk_info *info)
{
	unsigned char data[2] = {0xAC, 0x2D};
	int ret = 0;

	ret = i2c_master_send(info->client, data, 2);
	if (ret != 2) {
		input_err(true, &info->client->dev, "%s: write fail\n", __func__);
		return -1;
	}

	return 0;

}

static int abov_tk_fw_mode_exit(struct abov_tk_info *info)
{
	unsigned char data[2] = {0xAC, 0xE1};
	int ret = 0;

	ret = i2c_master_send(info->client, data, 2);
	if (ret != 2) {
		input_err(true, &info->client->dev, "%s: write fail\n", __func__);
		return -1;
	}

	return 0;

}

static int abov_tk_fw_update(struct abov_tk_info *info, u8 cmd)
{
	int ret, ii = 0;
	int count;
	unsigned short address;
	unsigned char addrH, addrL;
	unsigned char data[32] = {0, };


	input_info(true, &info->client->dev, "%s start\n", __func__);

	count = info->firm_size / 32;
	address = 0x800;

	input_info(true, &info->client->dev, "%s reset\n", __func__);
	abov_tk_reset_for_bootmode(info);
	abov_delay(ABOV_BOOT_DELAY);
	ret = abov_tk_fw_mode_enter(info);
	if(ret<0){
		input_err(true, &info->client->dev,
			"%s:abov_tk_fw_mode_enter fail\n", __func__);
		return ret;
	}
	abov_delay(5);
	input_info(true, &info->client->dev, "%s fw_mode_cmd sended\n", __func__);

	if (abov_tk_fw_mode_check(info) != 1) {
		input_err(true, &info->client->dev, "%s: err, flash mode is not: %d\n", __func__, ret);
		return 0;
	}

	ret = abov_tk_flash_erase(info);
	abov_delay(1400);
	input_info(true, &info->client->dev, "%s fw_write start\n", __func__);

	for (ii = 1; ii < count; ii++) {
		/* first 32byte is header */
		addrH = (unsigned char)((address >> 8) & 0xFF);
		addrL = (unsigned char)(address & 0xFF);
		if (cmd == BUILT_IN)
			memcpy(data, &info->firm_data_bin->data[ii * 32], 32);
		else if (cmd == SDCARD)
			memcpy(data, &info->firm_data_ums[ii * 32], 32);

		ret = abov_tk_fw_write(info, &addrH, &addrL, data);
		if (ret < 0) {
			input_err(true, &info->client->dev,
				"%s: err, no device : %d\n", __func__, ret);
			return ret;
		}

		address += 0x20;

		memset(data, 0, 32);
	}
	ret = abov_tk_i2c_read_checksum(info);
	input_info(true, &info->client->dev, "%s checksum readed\n", __func__);

	ret = abov_tk_fw_mode_exit(info);
	input_info(true, &info->client->dev, "%s fw_write end\n", __func__);

	return ret;
}

static void abov_release_fw(struct abov_tk_info *info, u8 cmd)
{
	switch(cmd) {
	case BUILT_IN:
		release_firmware(info->firm_data_bin);
		break;

	case SDCARD:
		kfree(info->firm_data_ums);
		break;

	default:
		break;
	}
}

static int abov_flash_fw(struct abov_tk_info *info, bool probe, u8 cmd)
{
	struct i2c_client *client = info->client;
	int retry = 2;
	int ret;
	int block_count;
	const u8 *fw_data;

	switch(cmd) {
	case BUILT_IN:
		fw_data = info->firm_data_bin->data;
		break;

	case SDCARD:
		fw_data = info->firm_data_ums;
		break;

	default:
		return -1;
		break;
	}

	block_count = (int)(info->firm_size / 32);

	while (retry--) {
		ret = abov_tk_fw_update(info, cmd);
		if (ret < 0)
			break;
#if ABOV_ISP_FIRMUP_ROUTINE
		abov_tk_reset_for_bootmode(info);
		abov_fw_update(info, fw_data, block_count,
			info->pdata->gpio_scl, info->pdata->gpio_sda);
#endif

		if ((info->checksum_h != info->checksum_h_bin) ||
			(info->checksum_l != info->checksum_l_bin)) {
			input_err(true, &client->dev,
				"%s checksum fail.(0x%x,0x%x),(0x%x,0x%x) retry:%d\n",
				__func__, info->checksum_h, info->checksum_l,
				info->checksum_h_bin, info->checksum_l_bin, retry);
			ret = -1;
			continue;
		} else {
			input_info(true, &client->dev,"%s checksum successed.\n",__func__);
		}

		abov_tk_reset_for_bootmode(info);
		abov_delay(ABOV_RESET_DELAY);
		ret = get_tk_fw_version(info, true);
		if (ret) {
			input_err(true, &client->dev, "%s fw version read fail\n", __func__);
			ret = -1;
			continue;
		}

		if (info->fw_ver == 0) {
			input_err(true, &client->dev, "%s fw version fail (0x%x)\n",
				__func__, info->fw_ver);
			ret = -1;
			continue;
		}

		if ((cmd == BUILT_IN) && (info->fw_ver != info->fw_ver_bin)){
			input_err(true, &client->dev, "%s fw version fail 0x%x, 0x%x\n",
				__func__, info->fw_ver, info->fw_ver_bin);
			ret = -1;
			continue;
		}
		ret = 0;
		break;
	}

	return ret;
}

static ssize_t touchkey_fw_update(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int ret;
	u8 cmd;

	switch(*buf) {
	case 's':
	case 'S':
		cmd = BUILT_IN;
		break;
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	case 'i':
	case 'I':
		cmd = SDCARD;
		break;
#endif
	default:
		info->fw_update_state = 2;
		goto touchkey_fw_update_out;
	}

	ret = abov_load_fw(info, cmd);
	if (ret) {
		input_err(true, &client->dev,
			"%s fw load fail\n", __func__);
		info->fw_update_state = 2;
		goto touchkey_fw_update_out;
	}

	info->fw_update_state = 1;
	disable_irq(info->irq);
	info->enabled = false;
	ret = abov_flash_fw(info, false, cmd);
	if (info->flip_mode){
		abov_mode_enable(client, ABOV_FLIP, CMD_FLIP_ON);
	} else {
		if (info->glovemode)
			abov_mode_enable(client, ABOV_GLOVE, CMD_GLOVE_ON);
	}
	if (info->keyboard_mode)
		abov_mode_enable(client, ABOV_KEYBOARD, CMD_MOBILE_KBD_ON);

	info->enabled = true;
	enable_irq(info->irq);
	if (ret) {
		input_err(true, &client->dev, "%s fail\n", __func__);
//		info->fw_update_state = 2;
		info->fw_update_state = 0;

	} else {
		input_info(true, &client->dev, "%s success