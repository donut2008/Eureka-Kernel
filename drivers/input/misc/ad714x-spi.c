_err("%s: %d\n", __func__, ++log);

	if (abov_tk_fw_mode_check(info) != 1) {
		input_err(true, &info->client->dev, "%s: err, flash mode is not: %d\n", __func__, ret);
		/*return 0;*/
	}

	input_err(true, &info->client->dev, "%s: %d\n", __func__, ++log);

	ret = abov_tk_flash_erase(info);
	msleep(1400);

	input_err(true, &info->client->dev, "%s: %d\n", __func__, ++log);

	for (ii = 1; ii <= count; ii++) {	/* start form 1,  for header info */

		addrH = (unsigned char)((address >> 8) & 0xFF);
		addrL = (unsigned char)(address & 0xFF);
		if (cmd == BUILT_IN)
			memcpy(data, &info->firm_data_bin->data[ii * 32], 32);
		else if (cmd == SDCARD)
			memcpy(data, &info->firm_data_ums[ii * 32], 32);

		ret = abov_tk_fw_write(info, &addrH, &addrL, data);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: err, no device : %d\n", __func__, ret);
			return ret;
		}
		usleep_range(3 * 1000, 3 * 1000);

		abov_tk_check_busy(info);

		address += 0x20;

		memset(data, 0, 32);
	}

	input_err(true, &info->client->dev, "%s: %d\n", __func__, ++log);

	ret = abov_tk_i2c_read_checksum(info);

	input_err(true, &info->client->dev, "%s: %d\n", __func__, ++log);

	ret = abov_tk_fw_mode_exit(info);

	input_err(true, &info->client->dev, "%s: %d\n", __func__, ++log);


	gpio_direction_output(info->dtdata->gpio_en, 0);
	msleep(30);
	gpio_direction_output(info->dtdata->gpio_en, 1);
	msleep(100);

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

	ret = get_tk_fw_version(info, probe);
	if (ret)
		info->fw_ver = 0;

	ret = abov_load_fw(info, cmd);
	if (ret) {
		input_err(true, &client->dev,
			"%s fw load fail\n", __func__);
		return ret;
	}

	switch (cmd) {
	case BUILT_IN:
		fw_data = info->firm_data_bin->data;
		break;

	case SDCARD:
		fw_data = info->firm_data_ums;
		break;

	default:
		return -1;
		/* break; */
	}

	block_count = (int)(info->firm_size / 32);

	while (retry--) {
		ret = abov_tk_fw_update(info, cmd);
		if (ret < 0)
			break;

		if (cmd == BUILT_IN) {
			if ((info->checksum_h != info->checksum_h_bin) ||
				(info->checksum_l != info->checksum_l_bin)) {
				input_err(true, &client->dev,
					"%s checksum fail.(0x%x,0x%x),(0x%x,0x%x) retry:%d\n",
					__func__, info->checksum_h, info->checksum_l,
					info->checksum_h_bin, info->checksum_l_bin, retry);
				ret = -1;
				continue;
			}
		}
		abov_tk_reset_for_bootmode(info);
		msleep(ABOV_RESET_DELAY);
		ret = get_tk_fw_version(info, t