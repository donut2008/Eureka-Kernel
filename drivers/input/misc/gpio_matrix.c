t i2c_client *client = data->client;
	struct regulator *regulator;
	int ret=0;

	if (tc300k_keyled_enabled == on)
		return 0;

	if (!regulator_led)
		return 0;

	input_info(true, &client->dev, "%s %s\n",
		__func__, on ? "on" : "off");

	regulator = regulator_get(NULL, regulator_led);
	if (IS_ERR(regulator)){
		input_err(true, &client->dev, "%s: regulator_led get failed\n", __func__);
		return -EIO;
	}
	if (on) {
		ret = regulator_enable(regulator);
		if (ret) {
			input_err(true, &client->dev, "%s: regulator_led enable failed\n", __func__);
			return ret;
		}
	} else {
		if (regulator_is_enabled(regulator))
			ret = regulator_disable(regulator);
			if (ret) {
				input_err(true, &client->dev, "%s: regulator_led disable failed\n", __func__);
				return ret;
			}
			else
				regulator_force_disable(regulator);
		}
	regulator_put(regulator);

	tc300k_keyled_enabled = on;

	return 0;
}

static irqreturn_t tc300k_interrupt(int irq, void *dev_id)
{
	struct tc300k_data *data = dev_id;
	struct i2c_client *client = data->client;
	int ret, retry;
	u8 key_val;
	int i = 0;
	bool key_handle_flag;
#ifdef FEATURE_GRIP_FOR_SAR
	u8 grip_val;
	bool grip_handle_flag;

	wake_lock(&data->touchkey_wake_lock);
#endif

	input_dbg(true, &client->dev, "%s\n",__func__);

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
#ifdef FEATURE_GRIP_FOR_SAR
		wake_unlock(&data->touchkey_wake_lock);
#endif
		return IRQ_HANDLED;
	}

#ifdef FEATURE_GRIP_FOR_SAR
	// if sar_mode is on => must send wake-up command
	if (data->sar_mode) {
		ret = tc300k_wake_up(client, TC300K_CMD_WAKE_UP);
	}
	ret = i2c_smbus_read_byte_data(client, TC305K_GRIPCODE);
	if (ret < 0) {
		retry = 3;
		while (retry--) {
			input_err(true, &client->dev, "%s read fail ret=%d(retry:%d)\n",
				__func__, ret, retry);
			msleep(10);
			ret = i2c_smbus_read_byte_data(client, TC305K_GRIPCODE);
			if (ret > 0)
				break;
		}
		if (retry <= 0) {
			tc300k_reset(data);
			wake_unlock(&data->touchkey_wake_lock);
			return IRQ_HANDLED;
		}
	}
	grip_val = (u8)ret;
#endif

	ret = i2c_smbus_read_byte_data(client, TC300K_KEYCODE);
	if (ret < 0) {
		retry = 3;
		while (retry--) {
			input_err(true, &client->dev, "%s read fail ret=%d(retry:%d)\n",
				__func__, ret, retry);
			msleep(10);
			ret = i2c_smbus_read_byte_data(client, TC300K_KEYCODE);
			if (ret > 0)
				break;
		}
		if (retry <= 0) {
			tc300k_reset(data);
#ifdef FEATURE_GRIP_FOR_SAR
			wake_unlock(&data->touchkey_wake_lock);
#endif
			return IRQ_HANDLED;
		}
	}
	key_val = (u8)ret;

	for (i = 0 ; i < data->key_num * 2 ; i++){
		if (data->pdata->use_bitmap)
			key_handle_flag = (key_val & data->tsk_ev_val[i].tsk_bitmap);
		else
			key_handle_flag = (key_val == data->tsk_ev_val[i].tsk_bitmap);

		if (key_handle_flag){
			input_report_key(data->input_dev,
				data->tsk_ev_val[i].tsk_keycode, data->tsk_ev_val[i].tsk_status);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev, "key %s ver0x%02x\n",
				data->tsk_ev_val[i].tsk_status? "P" : "R", data->fw_ver);
#else
			input_info(true, &client->dev,
				"key %s : %s(0x%02X) ver0x%02x\n",
				data->tsk_ev_val[i].tsk_status? "P" : "R",
				data->tsk_ev_val[i].tsk_keyname, key_val,
				data->fw_ver);
#endif
#ifdef CONFIG_INPUT_BOOSTER
			input_booster_send_event(data->tsk_ev_val[i].tsk_keycode,
				data->tsk_ev_val[i].tsk_status ? BOOSTER_MODE_ON : BOOSTER_MODE_OFF);
#endif
		}
	}
#ifdef FEATURE_GRIP_FOR_SAR
	for (i = 0 ; i < data->grip_num * 2 ; i++){
		if (data->pdata->use_bitmap)
			grip_handle_flag = (grip_val & data->grip_ev_val[i].grip_bitmap);
		else
			grip_handle_flag = (grip_val == data->grip_ev_val[i].grip_bitmap);

		if (grip_handle_flag){
			//need to check when using 2 grip channel
			data->grip_event = data->tsk_ev_val[i].tsk_status;
			input_report_key(data->input_dev,
				data->grip_ev_val[i].grip_code, data->grip_ev_val[i].grip_status);
			input_info(true, &client->dev,
				"grip %s : %s(0x%02X) ver0x%02x\n",
				data->grip_ev_val[i].grip_status? "P" : "R",
				data->grip_ev_val[i].grip_name, grip_val,
				data->fw_ver);

#ifdef CONFIG_SEC_FACTORY
			data->diff = read_tc350k_register_data(data, TC305K_1GRIP, TC305K_GRIP_DIFF_DATA); 
			if (data->abnormal_mode) { 
				if (data->grip_event) { 
					if (data->max_diff < data->diff) 
						data->max_diff = data->diff; 
					data->irq_count++; 
				} 
			} 
#endif
		}
	}
#endif
	input_sync(data->input_dev);
#ifdef FEATURE_GRIP_FOR_SAR
	wake_unlock(&data->touchkey_wake_lock);
#endif
	return IRQ_HANDLED;
}

static ssize_t keycode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	
	ret = i2c_smbus_read_byte_data(client, TC300K_KEYCODE);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: failed to read threshold_h (%d)\n",
			__func__, ret);
		return ret;
	}
	
	return sprintf(buf, "%d\n", ret);
}

static ssize_t third_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	if (data->pdata->tsk_ic_num == TC350K_TSK_IC) {
		value = read_tc350k_register_data(data, TC350K_3KEY, TC350K_CH_PER_DATA_OFFSET);
		return sprintf(buf, "%d\n", value);
	} 
	else {
		value = 0;
		return sprintf(buf, "%d\n", value);
	}
		
}

static ssize_t third_raw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	if (data->pdata->tsk_ic_num == TC350K_TSK_IC) {
		value = read_tc350k_register_data(data, TC350K_3KEY, TC350K_CH_RAW_DATA_OFFSET);
		return sprintf(buf, "%d\n", value);
	} 
	else {
		value = 0;
		return sprintf(buf, "%d\n", value);
	}
}

static ssize_t fourth_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	if (data->pdata->tsk_ic_num == TC350K_TSK_IC) {
		value = read_tc350k_register_data(data, TC350K_4KEY, TC350K_CH_PER_DATA_OFFSET);
		return sprintf(buf, "%d\n", value);
	} 
	else {
		value = 0;
		return sprintf(buf, "%d\n", value);
	}
}

static ssize_t fourth_raw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	if (data->pdata->tsk_ic_num == TC350K_TSK_IC) {
		value = read_tc350k_register_data(data, TC350K_4KEY, TC350K_CH_RAW_DATA_OFFSET);
		return sprintf(buf, "%d\n", value);
	} 
	else {
		value = 0;
		return sprintf(buf, "%d\n", value);
	}
}

static ssize_t debug_c0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	
	ret = i2c_smbus_read_byte_data(client, 0xC0);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: failed to read 0xC0 Register (%d)\n",
			__func__, ret);
		return ret;
	}
	
	return sprintf(buf, "%d\n", ret);
}

static ssize_t debug_c1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	
	ret = i2c_smbus_read_byte_data(client, 0xC1);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: failed to read 0xC1 Register (%d)\n",
			__func__, ret);
		return ret;
	}
	
	return sprintf(buf, "%d\n", ret);
}


static ssize_t debug_c2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	
	ret = i2c_smbus_read_byte_data(client, 0xC2);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: failed to read 0xC2 Register (%d)\n",
			__func__, ret);
		return ret;
	}
	
	return sprintf(buf, "%d\n", ret);
}

static ssize_t debug_c3_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	
	ret = i2c_smbus_read_byte_data(client, 0xC3);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: failed to read 0xC3 Register (%d)\n",
			__func__, ret);
		return ret;
	}
	
	return sprintf(buf, "%d\n", ret);
}
static ssize_t tc300k_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	int value;
	u8 threshold_h, threshold_l;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	if (data->pdata->tsk_ic_num == TC350K_TSK_IC) {
		value = read_tc350k_register_data(data, TC350K_1KEY, TC350K_THRES_DATA_OFFSET);
		return sprintf(buf, "%d\n", value);
	} else {
		ret = i2c_smbus_read_byte_data(client, TC300K_THRES_H);
		if (ret < 0) {
			input_err(true, &client->dev, "%s: failed to read threshold_h (%d)\n",
				__func__, ret);
			return ret;
		}
		threshold_h = ret;

		ret = i2c_smbus_read_byte_data(client, TC300K_THRES_L);
		if (ret < 0) {
			input_err(true, &client->dev, "%s: failed to read threshold_l (%d)\n",
				__func__, ret);
			return ret;
		}
		threshold_l = ret;

		data->threhold = (threshold_h << 8) | threshold_l;
		return sprintf(buf, "%d\n", data->threhold);
	}
}

static ssize_t tc300k_led_control(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int scan_buffer;
	int ret;
	u8 cmd;

	if (!regulator_led)
		return count;

	ret = sscanf(buf, "%d", &scan_buffer);
	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if(data->pdata->touchkey_led != true){
		input_err(true, &client->dev, "%s: touchkey_led false\n", __func__);
		return count;
		}

	if (!(scan_buffer == 0 || scan_buffer == 1)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		if (scan_buffer == 1)
			data->led_on = true;
		return count;
	}

	if (scan_buffer == 1) {
		input_info(true, &client->dev, "led on\n");
		cmd = TC300K_CMD_LED_ON;
	} else {
		input_info(true, &client->dev, "led off\n");
		cmd = TC300K_CMD_LED_OFF;
	}
	ret = i2c_smbus_write_byte_data(client, TC300K_CMD_ADDR, cmd);
	if (ret < 0)
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);

	msleep(TC300K_CMD_DELAY);

	return count;
}

static int load_fw_in_kernel(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int ret;

	ret = request_firmware(&data->fw, data->pdata->fw_name, &client->dev);
	if (ret) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		return -1;
	}
	data->fw_img = (struct fw_image *)data->fw->data;

	input_info(true, &client->dev, "0x%x 0x%x firm (size=%d)\n",
		data->fw_img->first_fw_ver, data->fw_img->second_fw_ver, data->fw_img->fw_len);
	input_info(true, &client->dev, "%s done\n", __func__);

	return 0;
}

static int load_fw_sdcard(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	struct file *fp;
	mm_segment_t old_fs;
	long fsi