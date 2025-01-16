%s write fail(%d)\n", __func__, ret);
	}

	usleep_range(10, 10);

	ret = abov_tk_i2c_read(info->client, CMD_SAR_TOTALCAP_READ, r_buf, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		return sprintf(buf, "%d\n", 0);
	}
	value = (r_buf[0] << 8) | r_buf[1];

	return sprintf(buf, "%d\n", value / 100);
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
	info->grip_raw2 = 0;

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

	input_info(true, &info->client->dev, "%s event:%d\n", __func__, info->grip_event);

	return sprintf(buf, "%d\n", info->grip_event);
}

static ssize_t touchkey_grip_sw_reset(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int data;
	int ret;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	info->grip_event = 0;

	input_info(true, &info->client->dev, "%s data(%d)\n",__func__,data);

	abov_grip_sw_reset(info);

	return count;
}

static ssize_t touchkey_sensing_change(struct device *dev,
		 struc