t_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);

out:
	data->flip_mode = flip_mode_on;
	return count;
}

#ifdef FEATURE_GRIP_FOR_SAR
static ssize_t touchkey_sar_enable(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int buff;
	int ret;
	bool on;
	int cmd;

	ret = sscanf(buf, "%d", &buff);
	if (ret != 1) {
		input_err(true, &client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	input_info(true, &client->dev, "%s (%d)\n", __func__, buff);
//return count;	//temp

	if (!(buff >= 0 && buff <= 3)) {
		input_err(true, &client->dev, "%s: wrong command(%d)\n",
				__func__, buff);
		return count;
	}

	/*	sar enable param
	  *	0	off
	  *	1	on
	  *	2	force off
	  *	3	force off -> on
	  */

	if (buff == 3) {
		data->sar_enable_off = 0;
		input_info(true, &client->dev, 
				"%s : Power back off _ force off -> on (%d)\n",
				__func__, data->sar_enable);
		if (data->sar_enable)
			buff = 1;
		else
			return count;
	}

	if (data->sar_enable_off) {
		if (buff == 1)
			data->sar_enable = true;
		else
			data->sar_enable = false;
		input_info(true, &client->dev, 
				"%s skip, Power back off _ force off mode (%d)\n",
				__func__, data->sar_enable);
		return count;
	}

	if (buff == 1) {
		on = true;
		cmd = TC300K_CMD_SAR_ENABLE;
	} else if (buff == 2) {
		on = false;
		data->sar_enable_off = 1;
		cmd = TC300K_CMD_SAR_DISABLE;
	} else {
		on = false;
		cmd = TC300K_CMD_SAR_DISABLE;
	}

	// if sar_mode is on => must send wake-up command
	if (data->sar_mode) {
		ret = tc300k_wake_up(data->client, TC300K_CMD_WAKE_UP);
	}

	ret = tc300k_mode_enable(data->client, cmd);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
		return count;
	}


	if (buff == 1) {
		data->sar_enable = true;
	} else {
		input_report_key(data->input_dev, KEY_CP_GRIP, TSK_RELEASE);
		data->grip_event = 0;
		data->sar_enable = false;
	}

	input_info(true, &client->dev, "%s data:%d on:%d\n",__func__, buff, on);
	return count;
}

static ssize_t touchkey_grip1_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	int ret;

	ret = read_tc350k_register_data(data, TC305K_1GRIP, TC305K_GRIP_THD_PRESS);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s fail to read press thd(%d)\n", __func__, ret);
		data->grip_p_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}
	data->grip_p_thd = ret;

	ret = read_tc350k_register_data(data, TC305K_1GRIP, TC305K_GRIP_THD_RELEASE);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s fail to read release thd(%d)\n", __func__, ret);
		data->grip_r_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}

	data->grip_r_thd = ret;

	ret = read_tc350k_register_data(data, TC305K_1GRIP, TC305K_GRIP_THD_NOISE);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s fail to read noise thd(%d)\n", __func__, ret);
		data->grip_n_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}
	data->grip_n_thd = ret;

	return sprintf(buf, "%d,%d,%d\n",
			data->grip_p_thd, data->grip_r_thd, data->grip_n_thd );
}

static ssize_t touchkey_grip2_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	int ret;

	ret = read_tc350k_register_data(data, TC305K_2GRIP, TC305K_GRIP_THD_PRESS);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s fail to read press thd(%d)\n", __func__, ret);
		data->grip_p_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}
	data->grip_p_thd = ret;

	ret = read_tc350k_register_data(data, TC305K_2GRIP, TC305K_GRIP_THD_RELEASE);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s fail to read release thd(%d)\n", __func__, ret);
		data->grip_r_thd = 0;
		return sprintf(buf, "%d\n", 0);
	}

	data->grip_r_thd = ret;

	ret = read_tc350k_register_data(data, TC305K_2GRIP, TC305K_GRIP_THD_NOISE);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s fail to read noise thd(%d)\n", __func__, ret);
		data->grip_n_thd = 0;