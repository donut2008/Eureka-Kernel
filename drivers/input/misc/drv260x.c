w version read success (%d)\n", ret);
		}
	}
	ret = abov_flash_fw(info, false, cmd);
#ifdef GLOVE_MODE
	if (info->glovemode)
		abov_mode_enable(client, ABOV_GLOVE, CMD_ON);
#endif
	info->enabled = true;
	enable_irq(info->irq);
	if (ret) {
		input_err(true, &client->dev, "%s fail\n", __func__);
		/* info->fw_update_state = 2; */
		info->fw_update_state = 0;

	} else {
		input_info(true, &client->dev, "%s success\n", __func__);
		info->fw_update_state = 0;
	}

touchkey_fw_update_out:
	input_info(true, &client->dev, "%s : %d\n", __func__, info->fw_update_state);

	return count;
}

static ssize_t touchkey_fw_update_status(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int count = 0;

	input_info(true, &client->dev, "%s : %d\n", __func__, info->fw_update_state);

	if (info->fw_update_state == 0)
		count = snprintf(buf, PAGE_SIZE, "PASS\n");
	else if (info->fw_update_state == 1)
		count = snprintf(buf, PAGE_SIZE, "Downloading\n");
	else if (info->fw_update_state == 2)
		count = snprintf(buf, PAGE_SIZE, "Fail\n");

	return count;
}

static ssize_t touchkey_crc_check(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int ret;
	unsigned char data[3] = {0x1B, 0x00, 0x10};
	unsigned char checksum[2] = {0, };

	i2c_master_send(info->client, data, 3);
	usleep_range(50 * 1000, 50 * 1000);

	ret = abov_tk_i2c_read(client, 0x1B, checksum, 2);

	if (ret < 0) {
		input_err(true, &client->dev, "%s: i2c read fail\n", __func__);
		return snprintf(buf, PAGE_SIZE, "NG,0000\n");
	}

	input_info(true, &client->dev, "%s : CRC:%02x%02x, BIN:%02x%02x\n", __func__, \
		checksum[0], checksum[1], \
		info->checksum_h_bin, info->checksum_l_bin);

	if ((checksum[0] != info->checksum_h_bin) ||
		(checksum[1] != info->checksum_l_bin)) {
		return snprintf(buf, PAGE_SIZE, "NG,%02x%02x\n",\
			checksum[0], checksum[1]);

	} else {
		return snprintf(buf, PAGE_SIZE, "OK,%02x%02x\n",\
			checksum[0], checksum[1]);
	}
}


#ifdef GLOVE_MODE
static ssize_t abov_glove_mode(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int scan_buffer;
	int ret;
	u8 cmd;

	ret = sscanf(buf, "%d", &scan_buffer);
	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(scan_buffer == 0 || scan_buffer == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if (!info->enabled)
		return count;

	if (info->glovemode == scan_buffer) {
		input_info(true, &client->dev, "%s same command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if (scan_buffer == 1) {
		input_info(true, &client->dev, "%s glove mode\n", __func__);
		cmd = CMD_ON;
	} else {
		input_info(true, &client->dev, "%s normal mode\n", __func__);
		cmd = CMD_OFF;
	}

	ret = abov_mode_enable(client, ABOV_GLOVE, cmd);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		return count;
	}

	info->glovemode = scan_buffer;

	return count;
}

static ssize_t abov_glove_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", info->glovemode);
}
#endif

/* Concept for flip mode is not requested yet
  * So Just enter sar only mode & do SW_RESET(grip baseline re-cal)
  * when flip cover cmd is recieved
  */
static ssize_t flip_cover_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int flip_mode_on;

	sscanf(buf, "%d\n", &flip_mode_on);
	input_info(true, &client->dev, "%s : %d\n", __func__, flip_mode_on);

	if (!info->enabled)
		goto out;

#ifdef CONFIG_TOUCHKEY_GRIP
	if (flip_mode_on) {
		abov_grip_sw_reset(info);
		abov_sar_only_mode(info, 1);
	} else {
		abov_sar_only_mode(info, 0);
	}
#else
#ifdef GLOVE_MODE
	if (info->glovemode) {
		int ret;
		u8 cmd = CMD_ON;
		ret = abov_mode_enable(client, ABOV_GLOVE, cmd);
		if (ret < 0) {
			input_err(true, &client->dev, "%s glove mode fail(%d)\n", __func__, ret);
			goto out;
		}
	}
#endif
	/* Do nothing yet */
#endif

out:
	info->flip_mode = flip_mode_on;
	return count;
}

static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			touchkey_led_control);
#ifdef CONFIG_TOUCHKEY_GRIP
static DEVICE_ATTR(touchkey_grip_threshold, S_IRUGO, touchkey_grip_threshold_show, NULL);
static DEVICE_ATTR(touchkey_total_cap, S_IRUGO, touchkey_total_cap_show, NULL);
static DEVICE_ATTR(sar_enable, S_IRUGO | S_IWUSR | S_IWGRP, NULL, touchkey_sar_enable);
static DEVICE_ATTR(sw_reset, S_IRUGO | S_IWUSR | S_IWGRP, NULL, touchkey_grip_sw_reset);
static DEVICE_ATTR(touchkey_earjack, S_IRUGO | S_IWUSR | S_IWGRP, NULL, touchkey_sensing_change);
static DEVICE_ATTR(touchkey_grip, S_IRUGO, touchkey_grip_show, NULL);
static DEVICE_ATTR(touchkey_grip_baseline, S_IRUGO, touchkey_grip_baseline_show, NULL);
static DEVICE_ATTR(touchkey_grip_raw, S_IRUGO, touchkey_grip_raw_show, NULL);
static DEVICE_ATTR(touchkey_grip_gain, S_IRUGO, touchkey_grip_gain_show, NULL);
static DEVICE_ATTR(touchkey_grip_check, S_IRUGO, touchkey_grip_check_show, NULL);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
static DEVICE_ATTR(touchkey_sar_only_mode,  S_IRUGO | S_IWUSR | S_IWGRP,
			NULL, touchkey_mode_change);
static DEVICE_ATTR(touchkey_sar_press_threshold,  S_IRUGO | S_IWUSR | S_IWGRP,
			NULL, touchkey_sar_press_threshold_store);
static DEVICE_ATTR(touchkey_sar_release_threshold,  S_IRUGO | S_IWUSR | S_IWGRP,
			NULL, touchkey_sar_release_threshold_store);
#endif
static DEVICE_ATTR(grip_irq_count, S_IRUGO | S_IWUSR | S_IWGRP, touchkey_grip_irq_count_show, touchkey_grip_irq_count_store);
#endif
static DEVICE_ATTR(touchkey_recent, S_IRUGO, touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw, S_IRUGO, touchkey_menu_raw_show, NULL);
static DEVICE_ATTR(touchkey_back_raw, S_IRUGO, touchkey_back_raw_show, NULL);
static DEVICE_ATTR(touchkey_chip_name, S_IRUGO, touchkey_chip_name, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO, bin_fw_ver, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO, read_fw_ver, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			touchkey_fw_update);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
			touchkey_fw_update_status, NULL);
static DEVICE_ATTR(touchkey_crc_check, S_IRUGO | S_IWUSR | S_IWGRP,
			touchkey_crc_check, NULL);

#ifdef GLOVE_MODE
static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP,
			abov_glove_mode_show, abov_glove_mode);
#endif
static DEVICE_ATTR(flip_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   flip_cover_mode_enable);


static struct attribute *sec_touchkey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_brightness.attr,
#ifdef CONFIG_TOUCHKEY_GRIP
	&dev_attr_touchkey_grip_threshold.attr,
	&dev_attr_touchkey_total_cap.attr,
	&dev_attr_sar_enable.attr,
	&dev_attr_sw_reset.attr,
	&dev_attr_touchkey_earjack.attr,
	&dev_attr_touchkey_grip.attr,
	&dev_attr_touchkey_grip_baseline.attr,
	&dev_attr_touchkey_grip_raw.attr,
	&dev_attr_touchkey_grip_gain.attr,
	&dev_attr_touchkey_grip_check.attr,
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	&dev_attr_touchkey_sar_only_mode.attr,
	&dev_attr_touchkey_sar_press_threshold.attr,
	&dev_attr_touchkey_sar_release_threshold.attr,
#endif
	&dev_attr_grip_irq_count.attr,
#endif
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_chip_name.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_crc_check.attr,
#ifdef GLOVE_MODE
	&dev_attr_glove_mode.attr,
#endif
	&dev_attr_flip_mode.attr,
	NULL,
};

static struct attribute_group sec_touchkey_attr_group = {
	.attrs = sec_touchkey_attributes,
};

static int abov_tk_fw_check(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	int ret;
	bool force = false;

	if (info->dtdata->bringup) {
		input_info(true, &client->dev, "%s: firmware update skip, bring up\n", __func__);
		return 0;
	}

	ret = get_tk_fw_version(info, true);
	if (ret) {
		input_err(true, &client->dev,
			"%s: i2c fail...[%d], addr[%d]\n",
			__func__, ret, info->client->addr);
#ifdef LED_TWINKLE_BOOTING
		/* regard I2C fail & LCD attached status as no TKEY device */
		if (!lcdtype) {
			input_err(true, &client->dev,
				"%s : LCD is not attached\n", __func__);
				return ret;
		}
#endif
	}
	ret = abov_load_fw_kernel(info);
	if (ret) {
		input_err(true, &client->dev,
			"failed to abov_load_fw_kernel (%d)\n", ret);
	} else {
		input_err(true, &client->dev,
			"fw version read success (%d)\n", ret);
	}

	if (info->md_ver != info->md_ver_bin) {
		input_err(true, &client->dev,
			"MD version is different.(IC %x, BN %x). Do force FW update\n",
			info->md_ver, info->md_ver_bin);
		force = true;
	}

	if (info->fw_ver < info->fw_ver_bin || info->fw_ver > 0xf0 || force == true) {
		input_err(true, &client->dev, "excute tk firmware update (0x%x -> 0x%x)\n",
			info->fw_ver, info->fw_ver_bin);
		ret = abov_flash_fw(info, true, BUILT_IN);
		if (ret) {
			input_err(true, &client->dev,
				"failed to abov_flash_fw (%d)\n", ret);
		} else {
			input_err(true, &client->dev,
				"fw update success\n");
		}
	}

	return ret;
}

int abov_power(struct abov_tk_info *info, bool on)
{
	struct abov_touchkey_devicetree_data *dtdata = info->dtdata;
	int ret = 0;

	if (gpio_is_valid(dtdata->gpio_en))
		gpio_direction_output(dtdata->gpio_en, on);

	/* 1.8V on,off control. */
	if (!dtdata->vdd_io_alwayson) {
		if (!IS_ERR_OR_NULL(dtdata->vdd_io_vreg)) {
			if (on)
				ret = regulator_enable(dtdata->vdd_io_vreg);
			else
				ret = regulator_disable(dtdata->vdd_io_vreg);
			input_err(true, &info->client->dev, "[TKEY] %s: iovdd reg %s %s\n",
					__func__, on ? "enable" : "disable",
					ret ? "NG" : "OK");
		}
	}

	input_info(true, &info->client->dev, "[TKEY] %s: %s ", __func__, on ? "on" : "off");
	if (gpio_is_valid(dtdata->gpio_en))
		input_info(true, &info->client->dev, "vdd_en:%d ", gpio_get_value(dtdata->gpio_en));
	if (!IS_ERR_OR_NULL(dtdata->vdd_io_vreg))
		input_info(true, &info->client->dev, "vio_reg:%d%s ",
				regulator_is_enabled(dtdata->vdd_io_vreg),
				dtdata->vdd_io_alwayson ? "(always on)" : "");
	if (!dtdata->vdd_led) {
		input_err(true, &info->client->dev, "%s [TKEY] %s VDD get regulator\n", __func__, SECLOG);
		dtdata->vdd_led = devm_regulator_get(&info->client->dev, "abov,lvdd");
		if (IS_ERR(dtdata->vdd_led)) {
			input_err(true, &info->client->dev, "%s [TKEY] %s annot get vdd\n", __func__, SECLOG);
			dtdata->vdd_led = NULL;
			return -ENOMEM;
		}

		if (!regulator_get_voltage(dtdata->vdd_led))
			regulator_set_voltage(dtdata->vdd_led, 3300000, 3300000);
	}

	
	if (on) {
		if (regulator_is_enabled(dtdata->vdd_led)) {
			input_err(true, &info->client->dev, "%s [TKEY] %s Regulator already enabled\n", __func__, SECLOG);
			return 0;
		}
		ret = regulator_enable(dtdata->vdd_led);
		if (ret)
			input_err(true, &info->client->dev, "%s [TKEY] %s failed to enable\n", __func__, SECLOG);
		usleep_range(10000, 11000);
	} else {
		ret = regulator_disable(dtdata->vdd_led);
		if (ret)
			input_err(true, &info->client->dev, "%s [TKEY] %s failed to disabl\n", __func__, SECLOG);
	}

	return ret;
}

static void abov_set_ta_status(struct abov_tk_info *info)
{
	u8 cmd_data = 0x10;
	u8 cmd_ta;
	int ret = 0;

	input_info(true, &info->client->dev, "%s ta_connected %d\n", __func__, ta_connected);

	if (info->enabled == false) {
		input_info(true, &info->client->dev, "%s status of ic is off\n", __func__);
		return;
	}

	if (ta_connected) {
		cmd_ta = 0x10;
	} else {
		cmd_ta = 0x20;
	}

	ret = abov_tk_i2c_write(info->client, ABOV_TSPTA, &cmd_ta, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
	}

	ret = abov_tk_i2c_write(info->client, ABOV_SW_RESET, &cmd_data, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s sw reset fail(%d)\n", __func__, ret);
	}
}

#ifdef CONFIG_VBUS_NOTIFIER
int abov_touchkey_vbus_notification(struct notifier_block *nb,
		unsigned long cmd, void *data)
{
	struct abov_tk_info *info = container_of(nb, struct abov_tk_info, vbus_nb);
	vbus_status_t vbus_type = *(vbus_status_t *)data;

	input_info(true, &info->client->dev, "%s cmd=%lu, vbus_type=%d\n", __func__, cmd, vbus_type);

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
		input_info(true, &info->client->dev, "%s : attach\n",__func__);
		ta_connected = true;
		break;
	case STATUS_VBUS_LOW:
		input_info(true, &info->client->dev, "%s : detach\n",__func__);
		ta_connected = false;

		break;
	default:
		break;
	}

	abov_set_ta_status(info);

	return 0;
}
#endif

static int abov_pinctrl_configure(struct abov_tk_info *info, bool active)
{
	struct pinctrl_state *set_state;
	int retval;

	if (active) {
		set_state =
			pinctrl_lookup_state(info->pinctrl,
						"on_irq");
		if (IS_ERR(set_state)) {
			input_err(true, &info->client->dev, "%s: cannot get ts pinctrl active state\n", __func__);
			return PTR_ERR(set_state);
		}
	} else {
		set_state =
			pinctrl_lookup_state(info->pinctrl,
						"off_irq");
		if (IS_ERR(set_state)) {
			input_err(true, &info->client->dev, "%s: cannot get gpiokey pinctrl sleep state\n", __func__);
			return PTR_ERR(set_state);
		}
	}
	retval = pinctrl_select_state(info->pinctrl, set_state);
	if (retval) {
		input_err(true, &info->client->dev, "%s: cannot set ts pinctrl active state\n", __func__);
		return retval;
	}

	input_info(true, &info->client->dev, "%s %s\n",
			__func__, active ? "ACTIVE" : "SUSPEND");

	return 0;
}

int abov_gpio_reg_init(struct device *dev,
			struct abov_touchkey_devicetree_data *dtdata)
{
	int ret = 0;

	if (dtdata->gpio_rst > 0) {
		ret = gpio_request(dtdata->gpio_rst, "tkey_gpio_rst");
		if(ret < 0){
			input_err(true, dev, "unable to request gpio_rst\n");
			return ret;
		}
	}
	ret = gpio_request(dtdata->gpio_int, "tkey_gpio_int");
	if (ret < 0) {
		input_err(true, dev, "unable to request gpio_int\n");
		return ret;
	}

	ret = gpio_request(dtdata->gpio_en, "tkey_gpio_en");
	if (ret < 0) {
		input_err(true, dev, "unable to request gpio_en\n");
		return ret;
	}

	dtdata->vdd_io_vreg = regulator_get(dev, "vddo");
	if (IS_ERR_OR_NULL(dtdata->vdd_io_vreg)) {
		regulator_put(dtdata->vdd_io_vreg);
		dtdata->vdd_io_vreg = NULL;
		input_err(true, dev, "dtdata->vdd_io_vreg get error, ignoring\n");
	} else {
		regulator_set_voltage(dtdata->vdd_io_vreg, 1800000, 1800000);

		/* 1.8V always on : for reduce probe time in bootup */
		if (dtdata->vdd_io_alwayson) {
			ret = regulator_enable(dtdata->vdd_io_vreg);
			if (ret) {
				input_err(true, dev, "[TKEY] %s: iovdd reg enable fail\n",
					__func__);
			}
		}

	}

	dtdata->power = abov_power;

	return ret;
}

#ifdef CONFIG_OF
static int abov_parse_dt(struct device *dev,
			struct abov_touchkey_devicetree_data *dtdata)
{
	struct device_node *np = dev->of_node;
	int ret;

	dtdata->gpio_rst = of_get_named_gpio(np, "abov,rst-gpio", 0);
	if (dtdata->gpio_rst < 0)
		input_err(true, dev, "unable to get gpio_rst\n");

	dtdata->gpio_en = of_get_named_gpio(np, "abov,tkey_en-gpio", 0);
	if (dtdata->gpio_en < 0) {
		input_err(true, dev, "unable to get gpio_en\n");
		return dtdata->gpio_en;
	}

	dtdata->gpio_int = of_get_named_gpio(np, "abov,irq-gpio", 0);
	if (dtdata->gpio_int < 0) {
		input_err(true, dev, "unable to get gpio_int\n");
		return dtdata->gpio_int;
	}

	dtdata->gpio_scl = of_get_named_gpio(np, "abov,scl-gpio", 0);
	if (dtdata->gpio_scl < 0) {
		input_err(true, dev, "unable to get gpio_scl\n");
		return dtdata->gpio_scl;
	}

	dtdata->gpio_sda = of_get_named_gpio(np, "abov,sda-gpio", 0);
	if (dtdata->gpio_sda < 0) {
		input_err(true, dev, "unable to get gpio_sda\n");
		return dtdata->gpio_sda;
	}

	dtdata->vdd_io_alwayson = of_property_read_bool(np, "abov,vddo_alwayson");
	if (dtdata->vdd_io_alwayson < 0) {
		input_err(true, dev, "unable to get vdd_io_alwayson\n");
		dtdata->vdd_io_alwayson = 0;
	}

	ret = of_property_read_u32(np, "abov,bringup", &dtdata->bringup);
	if (ret < 0)
		dtdata->bringup = 0;

	ret = of_property_read_u32(np, "abov,firmup_cmd", &dtdata->firmup_cmd);
	if (ret < 0)
		dtdata->firmup_cmd = 0;

	of_property_read_string(np, "abov,firmware_name", &dtdata->fw_name);
	
	dtdata->ta_notifier = of_property_read_bool(np, "abov,ta-notifier");
	dtdata->not_support_key = of_property_read_bool(np, "abov,not_support_key");

	input_info(true, dev, "%s: [GPIO] en:%d, int:%d, scl:%d, sda:%d, vdd_io:%d\n"
			"bringup:%d, firmupcmd:%x, %s: fw_name: %s\n",
			__func__, dtdata->gpio_en, dtdata->gpio_int, dtdata->gpio_scl,
			dtdata->gpio_sda, dtdata->vdd_io_alwayson, dtdata->bringup,
			dtdata->firmup_cmd, __func__, dtdata->fw_name);

	return 0;
}
#else
static int abov_parse_dt(struct device *dev,
			struct abov_touchkey_devicetree_data *dtdata)
{
	return -ENODEV;
}
#endif

static int abov_tk_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct abov_tk_info *info;
	struct input_dev *input_dev;
	int ret = 0;

	pr_err("%s++\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev,
			"i2c_check_functionality fail\n");
		return -EIO;
	}

#if !(defined(LED_TWINKLE_BOOTING) || defined(CONFIG_TOUCHKEY_GRIP))
	if (!lcdtype) {
		input_err(true, &client->dev,
			"%s : LCD is not attached\n", __func__);
		return -ENODEV;
	}
#endif

	info = kzalloc(sizeof(struct abov_tk_info), GFP_KERNEL);
	if (!info) {
		input_err(true, &client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		input_err(true, &client->dev,
			"Failed to allocate memory for input device\n");
		ret = -ENOMEM;
		goto err_input_alloc;
	}

	info->client = client;
	info->input_dev = input_dev;
	info->probe_done = false;
#ifdef CONFIG_TOUCHKEY_GRIP
	wake_lock_init(&info->touchkey_wake_lock, WAKE_LOCK_SUSPEND, "touchkey wake lock");
#endif

	if (client->dev.of_node) {
		struct abov_touchkey_devicetree_data *dtdata;

		dtdata = devm_kzalloc(&client->dev,
				sizeof(struct abov_touchkey_devicetree_data), GFP_KERNEL);
		if (!dtdata) {
			input_err(true, &client->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_config;
		}

		ret = abov_parse_dt(&client->dev, dtdata);
		if (ret)
			goto err_config;

		info->dtdata = dtdata;
	} else
		info->dtdata = client->dev.platform_data;

	if (info->dtdata == NULL) {
		input_err(true, &client->dev,"failed to get platform data\n");
		goto err_config;
	}

	/* Get pinctrl if target uses pinctrl */
	info->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(info->pinctrl)) {
		if (PTR_ERR(info->pinctrl) == -EPROBE_DEFER)
			goto err_config;

		input_err(true, &client->dev,"%s: Target does not use pinctrl\n", __func__);
		info->pinctrl = NULL;
	}

	if (info->pinctrl) {
		ret = abov_pinctrl_configure(info, true);
		if (ret)
			input_err(true, &client->dev,"%s: cannot set ts pinctrl active state\n", __func__);
	}

	ret = abov_gpio_reg_init(&client->dev, info->dtdata);
	if (ret) {
		input_err(true, &client->dev, "failed to init reg\n");
		goto pwr_config;
	}

	if (info->dtdata->power) {
		info->dtdata->power(info, true);
		msleep(ABOV_RESET_DELAY);
	}

	info->irq = -1;
	client->irq = gpio_to_irq(info->dtdata->gpio_int);
	mutex_init(&info->lock);

	info->touchkey_count = sizeof(touchkey_keycode) / sizeof(int);
	i2c_set_clientdata(client, info);

	ret = abov_tk_fw_check(info);
	if (ret) {
		input_err(true, &client->dev,
			"failed to firmware check (%d)\n", ret);
		goto err_reg_input_dev;
	}

	snprintf(info->phys, sizeof(info->phys),
		 "%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchkey";
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &client->dev;
#ifdef USE_OPEN_CLOSE
	input_dev->open = abov_tk_input_open;
	input_dev->close = abov_tk_input_close;
#endif
	set_bit(EV_KE