enu_s;
	u16 back_s;
	u16 menu_raw;
	u16 back_raw;
#ifdef CONFIG_TOUCHKEY_GRIP
	struct wake_lock touckey_wake_lock;
	u16 grip_p_thd;
	u16 grip_r_thd;
	u16 grip_n_thd;
	u16 grip_s1;
	u16 grip_s2;
	u16 grip_baseline;
	u16 grip_raw1;
	u16 grip_raw2;
	u16 grip_event;
	bool sar_mode;
	bool sar_enable;
	bool sar_enable_off;
	bool sar_sensing;
#endif
	int (*power) (bool on);
	void (*input_event)(void *data);
	int touchkey_count;
	u8 fw_update_state;
	u8 fw_ver;
	u8 fw_ver_bin;
	u8 fw_model_number;
	u8 checksum_h;
	u8 checksum_h_bin;
	u8 checksum_l;
	u8 checksum_l_bin;
	bool enabled;
	bool glovemode;
	bool keyboard_mode;
	bool flip_mode;
#ifdef LED_TWINKLE_BOOTING
	struct delayed_work led_twinkle_work;
	bool led_twinkle_check;
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	struct notifier_block vbus_nb;
#endif
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	struct delayed_work efs_open_work;
	int light_version_efs;
	char light_version_full_efs[LIGHT_VERSION_LEN];
	char light_version_full_bin[LIGHT_VERSION_LEN];
	int light_table_crc;
	u8 light_reg;
#endif
};


#ifdef CONFIG_HAS_EARLYSUSPEND
static void abov_tk_early_suspend(struct early_suspend *h);
static void abov_tk_late_resume(struct early_suspend *h);
#endif

#if 1//def CONFIG_INPUT_ENABLED
static int abov_tk_input_open(struct input_dev *dev);
static void abov_tk_input_close(struct input_dev *dev);
#endif

static int abov_tk_i2c_read_checksum(struct abov_tk_info *info);
static void abov_tk_reset(struct abov_tk_info *info);
#ifdef CONFIG_VBUS_NOTIFIER
static void abov_set_ta_status(struct abov_tk_info *info);
#endif
static int abov_touchkey_led_status;
static int abov_touchled_cmd_reserved;

void abov_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}

static int abov_mode_enable(struct i2c_client *client,u8 cmd_reg, u8 cmd)
{
	return i2c_smbus_write_byte_data(client, cmd_reg, cmd);
}

#if ABOV_ISP_FIRMUP_ROUTINE
static void abov_config_gpio_i2c(struct abov_tk_info *info, int onoff)
{
	struct device *i2c_dev = info->client->dev.parent->parent;
	struct pinctrl *pinctrl_i2c;

	if (onoff) {
		pinctrl_i2c = devm_pinctrl_get_select(i2c_dev, "on_i2c");
		if (IS_ERR(pinctrl_i2c))
			input_err(true, &info->client->dev, "%s: Failed to configure i2c pin\n", __func__);
	} else {
		pinctrl_i2c = devm_pinctrl_get_select(i2c_dev, "off_i2c");
		if (IS_ERR(pinctrl_i2c))
			input_err(true, &info->client->dev, "%s: Failed to configure i2c pin\n", __func__);
	}
}
#endif

static int abov_tk_i2c_read(struct i2c_client *client,
		u8 reg, u8 *val, unsigned int len)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);
	struct i2c_msg msg;
	int ret;
	int retry = 3;

	mutex_lock(&info->lock);
	msg.addr = client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 1;
	msg.buf = &reg;
	while (retry--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret >= 0)
			break;

		input_err(true, &client->dev, "%s fail(address set)(%d)\n",
			__func__, retry);
		abov_delay(10);
	}
	if (ret < 0) {
		mutex_unlock(&info->lock);
		return ret;
	}
	retry = 3;
	msg.flags = 1;/*I2C_M_RD*/
	msg.len = len;
	msg.buf = val;
	while (retry--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret >= 0) {
			mutex_unlock(&info->lock);
			return 0;
		}
		input_err(true, &client->dev, "%s fail(data read)(%d)\n",
			__func__, retry);
		abov_delay(10);
	}
	mutex_unlock(&info->lock);
	return ret;
}

static int abov_tk_i2c_read_data(struct i2c_client *client, u8 *val, unsigned int len)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);
	struct i2c_msg msg;
	int ret;
	int retry = 3;

	mutex_lock(&info->lock);
	msg.addr = client->addr;
	msg.flags = 1;/*I2C_M_RD*/
	msg.len = len;
	msg.buf = val;
	while (retry--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret >= 0) {
			mutex_unlock(&info->lock);
			return 0;
		}
		input_err(true, &client->dev, "%s fail(data read)(%d)\n",
			__func__, retry);
		abov_delay(10);
	}
	mutex_unlock(&info->lock);
	return ret;
}

static int abov_tk_i2c_write(struct i2c_client *client,
		u8 reg, u8 *val, unsigned int len)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);
	struct i2c_msg msg[1];
	unsigned char data[2];
	int ret;
	int retry = 3;

	mutex_lock(&info->lock);
	data[0] = reg;
	data[1] = *val;
	msg->addr = client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 2;
	msg->buf = data;

	while (retry--) {
		ret = i2c_transfer(client->adapter, msg, 1);
		if (ret >= 0) {
			mutex_unlock(&info->lock);
			return 0;
		}
		input_err(true, &client->dev, "%s fail(%d)\n",
			__func__, retry);
		abov_delay(10);
	}
	mutex_unlock(&info->lock);
	return ret;
}

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
static int efs_read_light_table_version(struct abov_tk_info *info);

static void change_touch_key_led_brightness(struct device *dev, int led_reg)
{
	struct abov_tk_info *info = dev_get_drvdata(dev);
	int ret;
	
	input_info(true, dev, "%s: 0x%02x\n", __func__, led_reg);
	info->light_reg = led_reg;

	/*led dimming */
	ret = abov_tk_i2c_write(info->client, ABOV_LED_BACK, &info->light_reg, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s led dimming back key write fail(%d)\n", __func__, ret);
	}

	ret = abov_tk_i2c_write(info->client, ABOV_LED_RECENT, &info->light_reg, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s led dimming recent key write fail(%d)\n", __func__, ret);
	}
}

static int read_window_type(void)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	char window_type[2] = {0, };

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open("/sys/class/lcd/panel/window_type", O_RDONLY, 0440);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		return ret;
	}

	ret = type_filp->f_op->read(type_filp, window_type,
			sizeof(window_type), &type_filp->f_pos);
	if (ret != 2 * sizeof(char)) {
		pr_err("%s touchkey %s: fd read fail\n", SECLOG, __func__);
		ret = -EIO;
		return ret;
	}

	filp_close(type_filp, current->files);
	set_fs(old_fs);

	if (window_type[1] < '0' || window_type[1] >= 'f')
		return -EAGAIN;

	ret = (window_type[1] - '0') & 0x0f;
	pr_info("%s touchkey %s: %d\n", SECLOG, __func__, ret);
	return ret;
}

static int efs_calculate_crc (struct abov_tk_info *info)
{
	struct file *temp_file = NULL;
	int crc = info->light_version_efs;
	mm_segment_t old_fs;
	char predefine_value_path[LIGHT_TABLE_PATH_LEN];
	int ret = 0, i;
	char temp_vol[LIGHT_CRC_SIZE] = {0, };
	int table_size;

	efs_read_light_table_version(info);
	table_size = (int)strlen(info->light_version_full_efs) - 8;

	for (i = 0; i < table_size; i++) {
		char octa_temp = info->light_version_full_efs[8 + i];
		int octa_temp_i;

		if (octa_temp >= 'A')
			octa_temp_i = octa_temp - 'A' + 0x0A;
		else
			octa_temp_i = octa_temp - '0';
		
		input_info(true, &info->client->dev, "%s: octa %d\n", __func__, octa_temp_i);

		snprintf(predefine_value_path, LIGHT_TABLE_PATH_LEN, "%s%d",
				LIGHT_TABLE_PATH, octa_temp_i);
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		temp_file = filp_open(predefine_value_path, O_RDONLY, 0440);
		if (!IS_ERR(temp_file)) {
			temp_file->f_op->read(temp_file, temp_vol,
					sizeof(temp_vol), &temp_file->f_pos);
			filp_close(temp_file, current->files);
			if (kstrtoint(temp_vol, 0, &ret) < 0) {
				ret = -EIO;
			} else {
				crc += octa_temp_i;
				crc += ret;
				ret = 0;
			}
		}
		set_fs(old_fs);
	}

	if (!ret)
		ret = crc;

	return ret;
}

static int efs_read_crc(struct abov_tk_info *info)
{
	struct file *temp_file = NULL;
	char crc[LIGHT_CRC_SIZE] = {0, };
	mm_segment_t old_fs;
	int ret = 0;
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	temp_file = filp_open(LIGHT_CRC_PATH, O_RDONLY, 0440);
	if (IS_ERR(temp_file)) {
		ret = PTR_ERR(temp_file);
		input_info(true, &info->client->dev,
				"%s: failed to open efs file %d\n", __func__, ret);
	} else {
		temp_file->f_op->read(temp_file, crc, sizeof(crc), &temp_file->f_pos);
		filp_close(temp_file, current->files);
		if (kstrtoint(crc, 0, &ret) < 0)
			ret = -EIO;
	}
	set_fs(old_fs);

	return ret;
}

static bool check_light_table_crc(struct abov_tk_info *info)
{
	int crc_efs = efs_read_crc(info);

	if (info->light_version_efs == info->pdata->dt_li