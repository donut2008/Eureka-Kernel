 read_tc350k_register_data(data, TC305K_2GRIP, TC305K_GRIP_RAW_DATA);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s fail(%d)\n", __func__, ret);
		data->grip_raw1 = 0;
		data->grip_raw2 = 0;
		return sprintf(buf, "%d\n", 0);
	}
	data->grip_raw1 = ret;
	data->grip_raw2 = 0;

	return sprintf(buf, "%d,%d\n", data->grip_raw1, data->grip_raw2);
}

static ssize_t touchkey_grip_gain_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d,%d,%d,%d\n", 0, 0, 0, 0);
}

static ssize_t touchkey_grip_check_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);

	input_err(true, &data->client->dev, "%s event:%d\n", __func__, data->grip_event);

	return sprintf(buf, "%d\n", data->grip_event);
}

static ssize_t touchkey_grip_sw_reset(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int buff;
	int ret;

	ret = sscanf(buf, "%d", &buff);
	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(buff == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, buff);
		return count;
	}

	data->grip_event = 0;

	input_info(true, &client->dev, "%s data(%d)\n", __func__, buff);

	tc300k_grip_cal_reset(data);

	return count;
}

static ssize_t touchkey_sensing_change(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret, buff;

	ret = sscanf(buf, "%d", &buff);
	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(buff == 0 || buff == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
				__func__, buff);
		return count;
	}

	touchkey_sar_sensing(data, buff);

	input_info(true, &client->dev, "%s earjack (%d)\n", __func__, buff);

	return count;
}

static ssize_t touchkey_mode_change(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret, buff;

	ret = sscanf(buf, "%d", &buff);
	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(buff == 0 || buff == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n", __func__, buff);
		return count;
	}

	input_info(true, &client->dev, "%s data(%d)\n", __func__, buff);

	tc300k_stop_mode(data, buff);

	return count;
}

static ssize_t touchkey_grip_irq_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);

	int result = 0;

	if (data->irq_count)
		result = -1;

	input_info(true, &data->client->dev, "%s - called\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		result, data->irq_count, data->max_diff);
}

static ssize_t touchkey_grip_irq_count_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);

	u8 onoff;
	int ret;

	ret = kstrtou8(buf, 10, &onoff);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s - kstrtou8 failed.(%d)\n", __func__, ret);
		return count;
	}

	mutex_lock(&data->lock_fac);

	if (onoff == 0) {
		data->abnormal_mode = 0;
		tc300k_set_debug_work(data, 0, 0);
	} else if (onoff == 1) {
		data->abnormal_mode = 1;
		data->irq_count = 0;
		data->max_diff = 0;
		tc300k_set_debug_work(data, 1, 2000);
	} else {
		input_err(true, &data->client->dev, "%s - unknown value %d\n", __func__, onoff);
	}

	mutex_unlock(&data->lock_fac);

	input_info(true, &data->client->dev, "%s - %d\n", __func__, onoff);
	
	return count;
}
#endif

static ssize_t tc300k_modecheck_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 mode, glove, run, sar, ta;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	ret = tc300k_mode_check(client);
	if (ret < 0)
		return ret;
	else
		mode = ret;

	glove = !!(mode & TC300K_MODE_GLOVE);
	run = !!(mode & TC300K_MODE_RUN);
	sar = !!(mode & TC300K_MODE_SAR);
	ta = !!(mode & TC300K_MODE_TA_CONNECTED);

	input_info(true, &client->dev, "%s: bit:%x, glove:%d, run:%d, sar:%d, ta:%d\n",
			__func__, mode, glove, run, sar, ta);
	return sprintf(buf, "bit:%x, glove:%d, run:%d, sar:%d, ta:%d\n",
			mode, glove, run, sar, ta);
}

static ssize_t touchkey_chip_name(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

	input_info(true, &client->dev, "%s\n", __func__);

	return sprintf(buf, "TC305K\n");
}

static ssize_t touchkey_crc_check_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	int ret;

	ret = tc300k_crc_check(data);

	return sprintf(buf, (ret == 0) ? "OK,%x\n" : "NG,%x\n", data->checksum);
}

static DEVICE_ATTR(touchkey_threshold, S_IRUGO, tc300k_threshold_show, NULL);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		tc300k_led_control);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, tc300k_update_store);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO,
		tc300k_firm_status_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO,
		tc300k_firm_version_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO,
		tc300k_firm_version_read_show, NULL);
static DEVICE_ATTR(touchkey_md_version_phone, S_IRUGO,
		tc300k_md_version_show, NULL);
static DEVICE_ATTR(touchkey_md_version_panel, S_IRUGO,
		tc300k_md_version_read_show, NULL);
static DEVICE_ATTR(touchkey_recent, S_IRUGO, recent_key_show, NULL);
static DEVICE_ATTR(touchkey_recent_ref, S_IRUGO, recent_key_ref_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, back_key_show, NULL);
static DEVICE_ATTR(touchkey_back_ref, S_IRUGO, back_key_ref_show, NULL);
static DEVICE_ATTR(touchkey_d_menu, S_IRUGO, dummy_recent_show, NULL);
static DEVICE_ATTR(touchkey_d_back, S_IRUGO, dummy_back_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw, S_IRUGO, recent_key_raw, NULL);
static DEVICE_ATTR(touchkey_recent_raw_ref, S_IRUGO, recent_key_raw_ref, NULL);
static DEVICE_ATTR(touchkey_back_raw, S_IRUGO, back_key_raw, NULL);
static DEVICE_ATTR(touchkey_back_raw_ref, S_IRUGO, back_key_raw_ref, NULL);
static DEVICE_ATTR(touchkey_d_menu_raw, S_IRUGO, dummy_recent_raw, NULL);
static DEVICE_ATTR(touchkey_d_back_raw, S_IRUGO, dummy_back_raw, NULL);

/* for tc350k */
static DEVICE_ATTR(touchkey_back_raw_inner, S_IRUGO, back_raw_inner, NULL);
static DEVICE_ATTR(touchkey_back_raw_outer, S_IRUGO, back_raw_outer, NULL);
static DEVICE_ATTR(touchkey_recent_raw_inner, S_IRUGO, recent_raw_inner, NULL);
static DEVICE_ATTR(touchkey_recent_raw_outer, S_IRUGO, recent_raw_outer, NULL);

static DEVICE_ATTR(touchkey_back_idac_inner, S_IRUGO, back_idac_inner, NULL);
static DEVICE_ATTR(touchkey_back_idac_outer, S_IRUGO, back_idac_outer, NULL);
static DEVICE_ATTR(touchkey_recent_idac_inner, S_IRUGO, recent_idac_inner, NULL);
static DEVICE_ATTR(touchkey_recent_idac_outer, S_IRUGO, recent_idac_outer, NULL);

static DEVICE_ATTR(touchkey_back_idac, S_IRUGO, back_idac_inner, NULL);
static DEVICE_ATTR(touchkey_recent_idac, S_IRUGO, recent_idac_inner, NULL);

static DEVICE_ATTR(touchkey_back_inner, S_IRUGO, back_inner, NULL);
static DEVICE_ATTR(touchkey_back_outer, S_IRUGO, back_outer, NULL);
static DEVICE_ATTR(touchkey_recent_inner, S_IRUGO, recent_inner, NULL);
static DEVICE_ATTR(touchkey_recent_outer, S_IRUGO, recent_outer, NULL);

static DEVICE_ATTR(touchkey_recent_threshold_inner, S_IRUGO, recent_threshold_inner, NULL);
static DEVICE_ATTR(touchkey_back_threshold_inner, S_IRUGO, back_threshold_inner, NULL);
static DEVICE_ATTR(touchkey_recent_threshold_outer, S_IRUGO, recent_threshold_outer, NULL);
static DEVICE_ATTR(touchkey_back_threshold_outer, S_IRUGO, back_threshold_outer, NULL);
/* end 350k */

static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP,
		tc300k_glove_mode_show, tc300k_glove_mode);
static DEVICE_ATTR(flip_mode, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, tc300k_flip_mode);
static DEVICE_ATTR(modecheck, S_IRUGO, tc300k_modecheck_show, NULL);


static DEVICE_ATTR(touchkey_keycode, S_IRUGO, keycode_show, NULL);
static DEVICE_ATTR(touchkey_3rd, S_IRUGO, third_show, NULL);
static DEVICE_ATTR(touchkey_3rd_raw, S_IRUGO, third_raw_show, NULL);
static DEVICE_ATTR(touchkey_4th, S_IRUGO, fourth_show, NULL);
static DEVICE_ATTR(touchkey_4th_raw, S_IRUGO, fourth_raw_show, NULL);
static DEVICE_ATTR(touchkey_debug0, S_IRUGO, debug_c0_show, NULL);
static DEVICE_ATTR(touchkey_debug1, S_IRUGO, debug_c1_show, NULL);
static DEVICE_ATTR(touchkey_debug2, S_IRUGO, debug_c2_show, NULL);
static DEVICE_ATTR(touchkey_debug3, S_IRUGO, debug_c3_show, NULL);

#ifdef FEATURE_GRIP_FOR_SAR
static DEVICE_ATTR(touchkey_grip_threshold, S_IRUGO, touchkey_grip1_threshold_show, NULL);
static DEVICE_ATTR(touchkey_grip2ch_threshold, S_IRUGO, touchkey_grip2_threshold_show, NULL);
static DEVICE_ATTR(touchkey_total_cap, S_IRUGO, touchkey_total_cap1_show, NULL);
static DEVICE_ATTR(touchkey_total_cap2ch, S_IRUGO, touchkey_total_cap2_show, NULL);
static DEVICE_ATTR(sar_enable, S_IWUSR | S_IWGRP, NULL, touchkey_sar_enable);
static DEVICE_ATTR(sw_reset, S_IWUSR | S_IWGRP, NULL, touchkey_grip_sw_reset);
static DEVICE_ATTR(touchkey_earjack, S_IWUSR | S_IWGRP, NULL, touchkey_sensing_change);
static DEVICE_ATTR(touchkey_grip, S_IRUGO, touchkey_grip1_show, NULL);
static DEVICE_ATTR(touchkey_grip2ch, S_IRUGO, touchkey_grip2_show, NULL);
static DEVICE_ATTR(touchkey_grip_baseline, S_IRUGO, touchkey_grip1_baseline_show, NULL);
static DEVICE_ATTR(touchkey_grip2ch_baseline, S_IRUGO, touchkey_grip2_baseline_show, NULL);
static DEVICE_ATTR(touchkey_grip_raw, S_IRUGO, touchkey_grip1_raw_show, NULL);
static DEVICE_ATTR(touchkey_grip2ch_raw, S_IRUGO, touchkey_grip2_raw_show, NULL);
static DEVICE_ATTR(touchkey_grip_gain, S_IRUGO, touchkey_grip_gain_show, NULL);
static DEVICE_ATTR(touchkey_grip_check, S_IRUGO, touchkey_grip_check_show, NULL);
static DEVICE_ATTR(touchkey_sar_only_mode,  S_IWUSR | S_IWGRP, NULL, touchkey_mode_change);
static DEVICE_ATTR(grip_irq_count, S_IRUGO | S_IWUSR | S_IWGRP, touchkey_grip_irq_count_show, touchkey_grip_irq_count_store);
static DEVICE_ATTR(touchkey_ref_cap, S_IRUGO, touchkey_ref_cap_show, NULL);
#endif
static DEVICE_ATTR(touchkey_chip_name, S_IRUGO, touchkey_chip_name, NULL);
static DEVICE_ATTR(touchkey_crc_check, S_IRUGO, touchkey_crc_check_show, NULL);

static struct attribute *sec_touchkey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_brightness.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_md_version_phone.attr,
	&dev_attr_touchkey_md_version_panel.attr,
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_recent_ref.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_back_ref.attr,
	&dev_attr_touchkey_d_menu.attr,
	&dev_attr_touchkey_d_back.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_recent_raw_ref.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_back_raw_ref.attr,
	&dev_attr_touchkey_d_menu_raw.attr,
	&dev_attr_touchkey_d_back_raw.attr,
	&dev_attr_glove_mode.attr,
	&dev_attr_flip_mode.attr,
	&dev_attr_modecheck.attr,
	
	&dev_attr_touchkey_keycode.attr,
	&dev_attr_touchkey_3rd.attr,
	&dev_attr_touchkey_3rd_raw.attr,
	&dev_attr_touchkey_4th.attr,
	&dev_attr_touchkey_4th_raw.attr,
	&dev_attr_touchkey_debug0.attr,
	&dev_attr_touchkey_debug1.attr,
	&dev_attr_touchkey_debug2.attr,
	&dev_attr_touchkey_debug3.attr,
	
#ifdef FEATURE_GRIP_FOR_SAR
	&dev_attr_touchkey_grip_threshold.attr,
	&dev_attr_touchkey_grip2ch_threshold.attr,
	&dev_attr_touchkey_total_cap.attr,
	&dev_attr_touchkey_total_cap2ch.attr,
	&dev_attr_sar_enable.attr,
	&dev_attr_sw_reset.attr,
	&dev_attr_touchkey_earjack.attr,
	&dev_attr_touchkey_grip.attr,
	&dev_attr_touchkey_grip2ch.attr,
	&dev_attr_touchkey_grip_baseline.attr,
	&dev_attr_touchkey_grip2ch_baseline.attr,
	&dev_attr_touchkey_grip_raw.attr,
	&dev_attr_touchkey_grip2ch_raw.attr,
	&dev_attr_touchkey_grip_gain.attr,
	&dev_attr_touchkey_grip_check.attr,
	&dev_attr_touchkey_sar_only_mode.attr,
	&dev_attr_grip_irq_count.attr,
	&dev_attr_touchkey_ref_cap.attr,
#endif
	&dev_attr_touchkey_chip_name.attr,
	&dev_attr_touchkey_crc_check.attr,
	NULL,
};

static struct attribute_group sec_touchkey_attr_group = {
	.attrs = sec_touchkey_attributes,
};

static struct attribute *sec_touchkey_attributes_350k[] = {
	&dev_attr_brightness.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_md_version_phone.attr,
	&dev_attr_touchkey_md_version_panel.attr,

	&dev_attr_touchkey_back_raw_inner.attr,
	&dev_attr_touchkey_back_raw_outer.attr,
	&dev_attr_touchkey_recent_raw_inner.attr,
	&dev_attr_touchkey_recent_raw_outer.attr,

	&dev_attr_touchkey_back_idac_inner.attr,
	&dev_attr_touchkey_back_idac_outer.attr,
	&dev_attr_touchkey_recent_idac_inner.attr,
	&dev_attr_touchkey_recent_idac_outer.attr,

	&dev_attr_touchkey_back_inner.attr,
	&dev_attr_touchkey_back_outer.attr,
	&dev_attr_touchkey_recent_inner.attr,
	&dev_attr_touchkey_recent_outer.attr,

	&dev_attr_touchkey_recent_threshold_inner.attr,
	&dev_attr_touchkey_back_threshold_inner.attr,
	&dev_attr_touchkey_recent_threshold_outer.attr,
	&dev_attr_touchkey_back_threshold_outer.attr,

	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_recent_idac.attr,
	&dev_attr_touchkey_back_idac.attr,
	&dev_attr_touchkey_threshold.attr,

	&dev_attr_flip_mode.attr,
	&dev_attr_modecheck.attr,

	&dev_attr_touchkey_keycode.attr,
	&dev_attr_touchkey_3rd.attr,
	&dev_attr_touchkey_3rd_raw.attr,
	&dev_attr_touchkey_4th.attr,
	&dev_attr_touchkey_4th_raw.attr,
	&dev_attr_touchkey_debug0.attr,
	&dev_attr_touchkey_debug1.attr,
	&dev_attr_touchkey_debug2.attr,
	&dev_attr_touchkey_debug3.attr,
	
#ifdef FEATURE_GRIP_FOR_SAR
	&dev_attr_touchkey_grip_threshold.attr,
	&dev_attr_touchkey_grip2ch_threshold.attr,
	&dev_attr_touchkey_total_cap.attr,
	&dev_attr_touchkey_total_cap2ch.attr,
	&dev_attr_sar_enable.attr,
	&dev_attr_sw_reset.attr,
	&dev_attr_touchkey_earjack.attr,
	&dev_attr_touchkey_grip.attr,
	&dev_attr_touchkey_grip2ch.attr,
	&dev_attr_t