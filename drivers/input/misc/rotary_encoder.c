 = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int scan_buffer;
	int ret;
	u8 cmd;

	ret = sscanf(buf, "%d", &scan_buffer);
	input_info(true, &client->dev, "%s : %d\n", __func__, scan_buffer);


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
		cmd = CMD_GLOVE_ON;
	} else {
		cmd = CMD_GLOVE_OFF;
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

	return sprintf(buf, "%d\n", info->glovemode);
}

static ssize_t keyboard_cover_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int keyboard_mode_on;
	int ret;
	u8 cmd;

	input_info(true, &client->dev, "%s : Mobile KBD sysfs node called\n",__func__);

	sscanf(buf, "%d", &keyboard_mode_on);
	input_info(true, &client->dev, "%s : %d\n",
		__func__, keyboard_mode_on);

	if (!(keyboard_mode_on == 0 || keyboard_mode_on == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, keyboard_mode_on);
		return count;
	}

	if (!info->enabled)
		goto out;

	if (info->keyboard_mode == keyboard_mode_on) {
		input_info(true, &client->dev, "%s same command(%d)\n",
			__func__, keyboard_mode_on);
		goto out;
	}

	if (keyboard_mode_on == 1) {
		cmd = CMD_MOBILE_KBD_ON;
	} else {
		cmd = CMD_MOBILE_KBD_OFF;
	}

	/* mobile keyboard use same register with glove mode */
	ret = abov_mode_enable(client, ABOV_KEYBOARD, cmd);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		goto out;
	}

out:
	info->keyboard_mode = keyboard_mode_on;
	return count;
}

static ssize_t flip_cover_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	int flip_mode_on;
	int ret;
	u8 cmd;

	sscanf(buf, "%d\n", &flip_mode_on);
	input_info(true, &client->dev, "%s : %d\n", __func__, flip_mode_on);

	if (!info->enabled)
		goto out;

#ifdef CONFIG_TOUCHKEY_GRIP
	if (flip_mode_on) {
		cmd = 0x10;
		ret = abov_tk_i2c_write(info->client, ABOV_SW_RESET, &cmd, 1);
		if (ret < 0) {
			input_err(true, &client->dev, "%s sw_reset fail(%d)\n", __func__, ret);
		}
		abov_sar_only_mode(info, 1);
	} else {
		abov_sar_only_mode(info, 0);
	}
#else
	/* glove mode control */
	if (flip_mode_on) {
		cmd = CMD_FLIP_ON;
	} else {
		if (info->glovemode)
			cmd = CMD_GLOVE_ON;
		cmd = CMD_FLIP_OFF;
	}

	if (info->glovemode){
		ret = abov_mode_enable(client, ABOV_GLOVE, cmd);
		if (ret < 0) {
			input_err(true, &client->dev, "%s glove mode fail(%d)\n", __func__, ret);
			goto out;
		}
	} else{
		ret = abov_mode_enable(client, ABOV_FLIP, cmd);
		if (ret < 0) {
			input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
			goto out;
		}
	}
#endif

out:
	info->flip_mode = flip_mode_on;
	return count;
}

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
static ssize_t touchkey_light_version_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int count;
	int crc_cal, crc_efs;

	if (efs_read_light_table_version(info) < 0) {
		count = sprintf(buf, "NG");
		goto out;
	} else {
		if (info->light_version_efs == info->pdata->dt_light_version) {
			if (!check_light_table_crc(info)) {
				count = sprintf(buf, "NG_CS");
				goto out;
			}
		} else {
			crc_cal = efs_calculate_crc(info);
			crc_efs = efs_read_crc(info);
			input_info(true, &info->client->dev,
					"CRC compare: efs[%d], bin[%d]\n",
					crc_cal, crc_efs);
			if (crc_cal != crc_efs) {
				count = sprintf(buf, "NG_CS");
				goto out;
			}
		}
	}

	count = sprintf(buf, "%s,%s",
			info->light_version_full_efs,
			info->light_version_full_bin);
out:
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buf);
	return count;
}

static ssize_t touchkey_light_update(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int ret;
	int led_reg;
	int window_type = read_window_type();

	ret = efs_write(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: fail %d\n", __func__, ret);
		return -EIO;
	}

	led_reg = efs_read_light_table_with_default(info, window_type);
	if ((led_reg >= LIGHT_REG_MIN_VAL) && (led_reg <= LIGHT_REG_MAX_VAL)) {
		change_touch_key_led_brightness(&info->client->dev, led_reg);
		input_info(true, &info->client->dev,
				"%s: read done for %d\n", __func__, window_type);
	} else {
		input_err(true, &info->client->dev,
				"%s: fail. key led brightness reg is %d\n", __func__, led_reg);
	}

	return size;
}

static ssize_t touchkey_light_id_compare(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int count, ret;
	int window_type = read_window_type();

	if (window_type < 0) {
		input_err(true, &info->client->dev,
				"%s: window_type:%d, NG\n", __func__, window_type);
		return sprintf(buf, "NG");
	}

	ret = efs_read_light_table(info, window_type);
	if (ret < 0) {
		count = sprintf(buf, "NG");
	} else {
		count = sprintf(buf, "OK");
	}

	input_info(true, &info->client->dev,
			"%s: window_type:%d, %s\n", __func__, window_type, buf);
	return count;
}

static char* tokenizer(char* str, char delim)
{
	static char* str_addr;
	char* token = NULL;
	
	if (str != NULL)
		str_addr = str;
	else if (str_addr == NULL)
		return 0;

	token = str_addr;
	while (true) {
		if (!(*str_addr)) {
			break;
		} else if (*str_addr == delim) {
			*str_addr = '\0';
			str_addr = str_addr + 1;
			break;
		}
		str_addr++;
	}

	return token;
}

static ssize_t touchkey_light_table_write(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	struct light_info table[16];
	int ret;
	int led_reg;
	int window_type;
	char *full_version;
	char data[150] = {0, };
	int i, crc, crc_cal;
	char *octa_id;
	int table_size = 0;

	snprintf(data, sizeof(data), buf);

	input_info(true, &info->client->dev, "%s: %s\n",
			__func__, data);

	full_version = tokenizer(data, ',');
	if (!full_version)
		return -EIO;

	table_size = (int)strlen(full_version) - 8;
	input_info(true, &info->client->dev, "%s: version:%s size:%d\n",
			__func__, full_version, table_size);
	if (table_size < 0 || table_size > 16) {
		input_err(true, &info->client->dev, "%s: table_size is unavailable\n", __func__);
		return -EIO;
	}

	if (kstrtoint(tokenizer(NULL, ','), 0, &crc))
		return -EIO;

	input_info(true, &info->client->dev, "%s: crc:%d\n",
			__func__, crc);
	if (!crc)
		return -EIO;

	for (i = 0; i < table_size; i++) {
		octa_id = tokenizer(NULL, '_');
		if (!octa_id)
			break;

		if (octa_id[0] >= 'A')
			table[i].octa_id = octa_id[0] - 'A' + 0x0A;
		else
			table[i].octa_id = octa_id[0] - '0';
		if (table[i].octa_id < 0 || table[i].octa_id > 0x0F)
			break;
		if (kstrtoint(tokenizer(NULL, ','), 0, &table[i].led_reg))
			break;
	}

	if (!table_size) {
		input_err(true, &info->client->dev, "%s: no data in table\n", __func__);
		return -ENODATA;
	}

	for (i = 0; i < table_size; i++) {
		input_info(true, &info->client->dev, "%s: [%d] %X - %x\n",
				__func__, i, table[i].octa_id, table[i].led_reg);
	}

	/* write efs */
	ret = efs_write_light_table_version(info, full_version);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to write table ver %s. %d\n",
				__func__, full_version, ret);
		return ret;
	}

	info->light_version_efs = pick_light_table_version(full_version);

	for (i = 0; i < table_size; i++) {
		ret = efs_write_light_table(info, table[i]);
		if (ret < 0)
			break;
	}
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to write table%d. %d\n",
				__func__, i, ret);
		return ret;
	}

	ret = efs_write_light_table_crc(info, crc);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to write table crc. %d\n",
				__func__, ret);
		return ret;
	}

	crc_cal = efs_calculate_crc(info);
	input_info(true, &info->client->dev,
			"%s: efs crc:%d, caldulated crc:%d\n",
			__func__, crc, crc_cal);
	if (crc_cal != crc)
		return -EIO;

	window_type = read_window_type();
	led_reg = efs_read_light_table_with_default(info, window_type);
	if ((led_reg >= LIGHT_REG_MIN_VAL) && (led_reg <= LIGHT_REG_MAX_VAL)) {
		change_touch_key_led_brightness(&info->client->dev, led_reg);
		input_info(true, &info->client->dev,
				"%s: read done for %d\n", __func__, window_type);
	} else {
		input_err(true, &info->client->dev,
				"%s: fail. key led brightness reg is %d\n", __func__, led_reg);
	}

	return size;
}
#endif

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
static DEVICE_ATTR(touchkey_sar_