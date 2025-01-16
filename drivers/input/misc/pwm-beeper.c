
}

static void tc300k_grip_cal_reset(struct tc300k_data *data)
{
	/* calibrate grip sensor chn */
	struct i2c_client *client = data->client;

	input_info(true, &client->dev, "%s\n", __func__);
	i2c_smbus_write_byte_data(client, TC300K_CMD_ADDR, TC300K_CMD_GRIP_BASELINE_CAL);
	msleep(TC300K_CMD_DELAY);
}

#endif



static void tc300k_release_all_fingers(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int i;

	input_dbg(true, &client->dev, "%s\n", __func__);

	for (i = 0; i < data->key_num ; i++) {
		input_report_key(data->input_dev,
			data->tsk_ev_val[i].tsk_keycode, 0);
#ifdef CONFIG_INPUT_BOOSTER
		input_booster_send_event(data->tsk_ev_val[i].tsk_keycode,
			BOOSTER_MODE_FORCE_OFF);
#endif

	}
	input_sync(data->input_dev);
}

static void tc300k_reset(struct tc300k_data *data)
{
	input_info(true, &data->client->dev, "%s\n",__func__);

	disable_irq_nosync(data->client->irq);

	tc300k_release_all_fingers(data);

	data->pdata->keyled(data, false);
	data->pdata->power(data, false);

	msleep(50);

	data->pdata->power(data, true);
	data->pdata->keyled(data, true);
	msleep(200);

	if (data->flip_mode)
		tc300k_mode_enable(data->client, TC300K_CMD_FLIP_ON);

	if (data->glove_mode)
		tc300k_mode_enable(data->client, TC300K_CMD_GLOVE_ON);
#ifdef FEATURE_GRIP_FOR_SAR
	if (data->sar_enable)
		tc300k_mode_enable(data->client, TC300K_CMD_SAR_ENABLE);
#endif

	enable_irq(data->client->irq);
}

static void tc300k_reset_probe(struct tc300k_data *data)
{
	data->pdata->keyled(data, false);
	data->pdata->power(data, false);

	msleep(50);

	data->pdata->power(data, true);
	data->pdata->keyled(data, true);
	msleep(200);
}

int tc300k_get_fw_version(struct tc300k_data *data, bool probe)
{
	struct i2c_client *client = data->client;
#ifdef CONFIG_SEC_FACTORY
	int retry = 1;
#else
	int retry = 3;
#endif
	int buf;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	buf = i2c_smbus_read_byte_data(client, TC300K_FWVER);
	if (buf < 0) {
		while (retry--) {
			input_err(true, &client->dev, "%s read fail(%d)\n",
				__func__, retry);
			if (probe)
				tc300k_reset_probe(data);
			else
				tc300k_reset(data);
			buf = i2c_smbus_read_byte_data(client, TC300K_FWVER);
			if (buf > 0)
				break;
		}
		if (retry <= 0) {
			input_err(true, &client->dev, "%s read fail\n", __func__);
			data->fw_ver = 0;
			return -1;
		}
	}
	data->fw_ver = (u8)buf;
	input_info(true, &client->dev, "fw_ver : 0x%x\n", data->fw_ver);

	return 0;
}

int tc300k_get_md_version(struct tc300k_data *data, bool probe)
{
	struct i2c_client *client = data->client;
#ifdef CONFIG_SEC_FACTORY
	int retry = 1;
#else
	int retry = 3;
#endif
	int buf;

	if ((!data->enabled) || data->fw_downloding) {
		input_err(true, &client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	buf = i2c_smbus_read_byte_data(client, TC300K_MDVER);
	if (buf < 0) {
		while (retry--) {
			input_err(true, &client->dev, "%s read fail(%d)\n",
				__func__, retry);
			if (probe)
				tc300k_reset_probe(data);
			else
				tc300k_reset(data);
			buf = i2c_smbus_read_byte_data(client, TC300K_MDVER);
			if (buf > 0)
				break;
		}
		if (retry <= 0) {
			input_err(true, &client->dev, "%s read fail\n", __func__);
			data->md_ver = 0;
			return -1;
		}
	}
	data->md_ver = (u8)buf;
	input_info(true, &client->dev, "md_ver : 0x%x\n", data->md_ver);

	return 0;
}
static void tc300k_gpio_request(struct tc300k_data *data)
{
	int ret = 0;
	input_info(true, &data->client->dev, "%s: enter \n",__func__);

	if (!data->pdata->i2c_gpio) {
		ret = gpio_request(data->pdata->gpio_scl, "touchkey_scl");
		if (ret) {
			input_err(true, &data->client->dev, "%s: unable to request touchkey_scl [%d]\n",
					__func__, data->pdata->gpio_scl);
		}

		ret = gpio_request(data->pdata->gpio_sda, "touchkey_sda");
		if (ret) {
			input_err(true, &data->client->dev, "%s: unable to request touchkey_sda [%d]\n",
					__func__, data->pdata->gpio_sda);
		}
	}

	ret = gpio_request(data->pdata->gpio_int, "touchkey_irq");
	if (ret) {
		input_err(true, &data->client->dev, "%s: unable to request touchkey_irq [%d]\n",
				__func__, data->pdata->gpio_int);
	}
}

#ifdef CONFIG_OF
static int tc300k_parse_dt(struct device *dev,
			struct tc300k_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	of_property_read_u32(np, "coreriver,use_bitmap", &pdata->use_bitmap);

	input_info(true, dev, "%s : %s protocol.\n",
				__func__, pdata->use_bitmap ? "Use Bit-map" : "Use OLD");

	pdata->gpio_scl = of_get_named_gpio_flags(np, "coreriver,scl-gpio", 0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "coreriver,sda-gpio", 0, &pdata->sda_gpio_flags);
	pdata->gpio_int = of_get_named_gpio_flags(np, "coreriver,irq-gpio", 0, &pdata->irq_gpio_flags);

	pdata->boot_on_ldo = of_property_read_bool(np, "coreriver,boot-on-ldo");

	pdata->touchkey_led = of_property_read_bool(np, "coreriver,touchkey-led");

//	if (pdata->use_touchkey_