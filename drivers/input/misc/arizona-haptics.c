IG_TOUCHKEY_GRIP
	grip_data = (buf >> 4) & 0x03;
	grip_press = !(grip_data % 2);
#endif

	press = (menu_data ? menu_press : 0) || (back_data ? back_press : 0);

	if (menu_data)
		input_report_key(info->input_dev,
			touchkey_keycode[1], menu_press);
	if (back_data)
		input_report_key(info->input_dev,
			touchkey_keycode[2], back_press);
#ifdef CONFIG_TOUCHKEY_GRIP
	if (grip_data) {
		input_report_key(info->input_dev,
				touchkey_keycode[3], grip_press);
		info->grip_event =  grip_press;
	}
#ifdef CONFIG_SEC_FACTORY
	ret = abov_tk_i2c_read(info->client, CMD_SAR_DIFFDATA, r_buf, 2);
	if (ret < 0) {
		retry = 3;
		while (retry--) {
			input_err(true, &client->dev, "%s read fail(%d)\n",
				__func__, retry);
			ret = abov_tk_i2c_read(info->client, CMD_SAR_DIFFDATA, r_buf, 2);
			if (ret == 0)
				break;

			usleep_range(10 * 1000, 10 * 1000);
		}
	}
	
	info->diff = (r_buf[0] << 8) | r_buf[1];
	if (info->abnormal_mode) {
		if (info->grip_event) {
			if (info->max_diff < info->diff)
				info->max_diff = info->diff;
			info->irq_count++;
		}
	}
#endif
#endif

	input_sync(info->input_dev);

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
	input_info(true, &client->dev,
		"key %s%s ver0x%02x\n",
		menu_data ? (menu_press ? "P" : "R") : "",
		back_data ? (back_press ? "P" : "R") : "",
		info->fw_ver);
#else
	input_info(true, &client->dev,
		"%s%s%x ver0x%02x\n",
		menu_data ? (menu_press ? "menu P " : "menu R ") : "",
		back_data ? (back_press ? "back P " : "back R ") : "",
		buf, info->fw_ver);
#endif

#ifdef CONFIG_TOUCHKEY_GRIP
	if (grip_data) {
		input_info(true, &client->dev, "grip %s %x, TA %d\n",
			grip_press ? "P " : "R ", buf, ta_connected);
	}
	wake_unlock(&info->touchkey_wake_lock);
#endif

	return IRQ_HANDLED;
}

static int touchkey_led_set(struct abov_tk_info *info, int data)
{
	u8 cmd;
	int ret;

	if (data == 1)
		cmd = CMD_LED_ON;
	else
		cmd = CMD_LED_OFF;

	if (!info->enabled)
		goto out;

	ret = abov_tk_i2c_write(info->client, ABOV_LED_CONTROL, &cmd, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
		goto out;
	}

	return 0;
out:
	abov_touchled_cmd_reserved = 1;
	abov_touchkey_led_status = cmd;
	return 1;
}

static ssize_t touchkey_led_control(struct device *dev,
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

	if (!(data == 0 || data == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

#ifdef LED_TWINKLE_BOOTING
	if (info->led_twinkle_check == 1) {
		info->led_twinkle_check = 0;
		cancel_delayed_work(&info->led_twinkle_work);
	}
#endif

	if (touchkey_led_set(info, data))
		return count;

	msleep(20);

	abov_touchled_cmd_reserved = 0;
	input_info(true, &client->dev, "%s data(%d)\n", __func__,data);

	return count;
}

static ssize_t touchkey_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u8 r_buf;
	int ret;

	ret = abov_tk_i2c_read(client, ABOV_THRESHOLD, &r_buf, 1);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		r_buf = 0;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", r_buf);
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

	return snprintf(buf, PAGE_SIZE, "%d\n", info->menu_raw);
}

static ssize_t touchkey_back_raw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);

	get_raw_data(info);

	return snprintf(buf, PAGE_SIZE, "%d\n", info->back_raw);
}

#ifdef CONFIG_TOUCHKEY_GRIP
static ssize_t touchkey_sar_enable(struct device *dev,
		 struct device_attribute *attr, const c