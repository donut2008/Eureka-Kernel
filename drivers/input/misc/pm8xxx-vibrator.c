= dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u8 r_buf;
	int ret;

	ret = abov_tk_i2c_read(client, ABOV_THRESHOLD, &r_buf, 1);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		r_buf = 0;
	}
	return sprintf(buf, "%d\n", r_buf);
}

static void get_diff_data(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(client, ABOV_DIFFDATA, r_buf, 4);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		info->menu_s = 0;
		info->back_s = 0;
		return;
	}

	info->menu_s = (r_buf[0] << 8) | r_buf[1];
	info->back_s = (r_buf[2] << 8) | r_buf[3];
}

static ssize_t touchkey_menu_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_diff_data(info);

	return sprintf(buf, "%d\n", info->menu_s);
}

static ssize_t touchkey_back_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_diff_data(info);

	return sprintf(buf, "%d\n", info->back_s);
}

static void get_raw_data(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(client, ABOV_RAWDATA, r_buf, 4);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		info->menu_raw = 0;
		info->back_raw = 0;
		return;
	}

	info->menu_raw = (r_buf[0] << 8) | r_buf[1];
	info->back_raw = (r_buf[2] << 8) | r_buf[3];
}

static ssize_t touchkey_menu_raw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_raw_data(info);

	return sprintf(buf, "%d\n", info->menu_raw);
}

static ssize_t touchkey_back_raw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_raw_data(info);

	return sprintf(buf, "%d\n", info->back_raw);
}

#ifdef CONFIG_TOUCHKEY_GRIP
static ssize_t touchkey_grip_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(info->client, CMD_SAR_THRESHOLD, r_buf, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_p_thd = 0;
		info->grip_r_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_p_thd = (r_buf[0] << 8) | r_buf[1];
	info->grip_r_thd = (r_buf[2] << 8) | r_buf[3];

	ret = abov_tk_i2c_read(info->client, CMD_SAR_NOISE_THRESHOLD, r_buf, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_n_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_n_thd = (r_buf[0] << 8) | r_buf[1];

	return sprintf(buf, "%d,%d,%d\n", info->grip_p_thd, info->grip_r_thd, info->grip_n_thd );
}
static ssize_t touchkey_total_cap_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	u8 r_buf[2];
	u8 cmd;
	int ret;
	int value;

	cmd = 0x20;
	ret = abov_tk_i2c_write(info->client, CMD_SAR_TOTALCAP, &cmd, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s write fail(%d)\n", __func__, ret);
	}

	usleep_range(10, 10);

	ret = abov_tk_i2c_read(info->client, CMD_SAR_TOTALCAP_READ, r_buf, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		return sprintf(buf, "%d\n", 0);
	}
	value = (r_buf[0] << 8) | r_buf[1];

	return sprintf(buf, "%d\n", value/100);
}
static ssize_t touchkey_grip_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(info->client, CMD_SAR_DIFFDATA, r_buf, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_s1 = 0;
		info->grip_s2 = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_s1 = (r_buf[0] << 8) | r_buf[1];
	info->grip_s2 = (r_buf[2] << 8) | r_buf[3];


	return sprintf(buf, "%d,%d\n", info->grip_s1, info->grip_s2);
}

static ssize_t touchkey_grip_baseline_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 r_buf[2];
	int ret;

	ret = abov_tk_i2c_read(info->client, CMD_SAR_BASELINE, r_buf, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_baseline = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_baseline = (r_buf[0] << 8) | r_buf[1];

	return sprintf(buf, "%d\n", info->grip_baseline);
}

static ssize_t touchkey_grip_raw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	u8 r_buf[4];
	int ret;

	ret = abov_tk_i2c_read(info->client, CMD_SAR_RAWDATA, r_buf, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		info->grip_raw1 = 0;
		info->grip_raw2 = 0;
		return sprintf(buf, "%d\n", 0);
	}
	info->grip_raw1 = (r_buf[0] << 8) | r_buf[1];
	info->grip_raw2 = 0;	//(r_buf[2] << 8) | r_buf[3]; NA

	return sprintf(buf, "%d,%d\n", info->grip_raw1, info->grip_raw2);
}

static ssize_t touchkey_grip_gain_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d,%d,%d,%d\n", 0, 0, 0, 0);
}

static ssize_t touchkey_grip_check_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", info->grip_event);
}

static ssize_t touchkey_grip_sw_reset(struct device *dev,
		 struct device_attribute *attr, const char *bu