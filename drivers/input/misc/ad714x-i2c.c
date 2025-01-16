zalloc:
	filp_close(fp, current->files);
fail_sdcard_open:
	set_fs(old_fs);
	return ret;
}

static int abov_tk_check_busy(struct abov_tk_info *info)
{
	int ret, count = 0;
	unsigned char val = 0x00;

	do {
		ret = i2c_master_recv(info->client, &val, sizeof(val));

		if (val)
			count++;
		else
			break;

		if (count > 1000)
			break;
	} while (1);

	if (count > 1000)
		input_err(true, &info->client->dev, "%s: busy %d\n", __func__, count);
	return ret;
}

static int abov_tk_i2c_read_checksum(struct abov_tk_info *info)
{
	unsigned char data[6] = {0xAC, 0x9E, 0x10, 0x00, 0x3F, 0xFF};
	unsigned char data2[1] = {0x00};
	unsigned char checksum[6] = {0, };
	int ret;

	i2c_master_send(info->client, data, 6);

	usleep_range(5 * 1000, 5 * 1000);
	/* abov_tk_check_busy(info); */

	i2c_master_send(info->client, data2, 1);
	usleep_range(5 * 1000, 5 * 1000);

	ret = abov_tk_i2c_read_data(info->client, checksum, 6);


	input_info(true, &info->client->dev, "%s: ret:%d [%X][%X][%X][%X][%X][%X]\n",
			__func__, ret, checksum[0], checksum[1], checksum[2]
			, checksum[3], checksum[4], checksum[5]);
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
		input_err(true, &info->client->dev, "%s: write fail[%x%x], %d\n", __func__, *addrH, *addrL, ret);
		return ret;
	}

	usleep_range(2 * 1000, 2 * 1000);

	abov_tk_check_busy(info);

	return 0;
}

static int abov_tk_fw_mode_enter(struct abov_tk_info *info)
{
	unsigned char data[2] = {0xAC, 0x5B};
	u8 ic_ver = 0;
	int ret = 0;

	ret = i2c_master_send(info->client, data, 2);
	if (ret != 2) {
		pr_err("%s: write fail\n", __func__);
		return -1;
	}

	ret = i2c_master_recv(info->client, &ic_ver, 1);
	input_info(true, &info->client->dev, "%s: %2x, %2x\n", __func__, info->dtdata->firmup_cmd, ic_ver);
	if(info->dtdata->firmup_cmd != ic_ver){
		input_err(true, &info->client->dev, "%s: ic not matched, firmup fail\n", __func__);
		return -2;
	}

	return 0;

}

static int abov_tk_fw_mode_check(struct abov_tk_info *info)
{
	unsigned char buf[1] = {0};
	int ret;

	ret = abov_tk_i2c_read_data(info->client, buf, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: write fail\n", __func__);
		return 0;
	}

	input_info(true, &info->client->dev, "%s: ret:%02X\n", __func__, buf[0]);

	if (buf[0] == 