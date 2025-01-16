te);
#endif

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
#endif
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_chip_name.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_glove_mode.attr,
	&dev_attr_keyboard_mode.attr,
	&dev_attr_flip_mode.attr,
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	&dev_attr_touchkey_light_version.attr,
	&dev_attr_touchkey_light_update.attr,
	&dev_attr_touchkey_light_id_compare.attr,
	&dev_attr_touchkey_light_table_write.attr,
#endif
	NULL,
};

static struct attribute_group sec_touchkey_attr_group = {
	.attrs = sec_touchkey_attributes,
};

extern int get_samsung_lcd_attached(void);

static int abov_tk_fw_check(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	int ret, fw_update=0;
	u8 buf;

	if (info->pdata->bringup) {
		input_info(true, &client->dev, "%s: firmware update skip, bring up\n", __func__);
		return 0;
	}

	ret = abov_load_fw(info, BUILT_IN);
	if (ret) {
		input_err(true, &client->dev,
			"%s fw load fail\n", __func__);
		return ret;
	}

	ret = get_tk_fw_version(info, true);

#ifdef LED_TWINKLE_BOOTING
	if(ret)
		input_err(true, &client->dev,
			"%s: i2c fail...[%d], addr[%d]\n",
			__func__, ret, info->client->addr);
		input_err(true, &client->dev,
			"%s: touchkey driver unload\n", __func__);

		if (get_samsung_lcd_attached() == 0) {
			input_err(true, &client->dev, "%s : get_samsung_lcd_attached()=0 \n", __func__);
			abov_release_fw(info, BUILT_IN);
			return ret;
		}
#endif

	//Check Model No.
	ret = abov_tk_i2c_read(client, ABOV_MODEL_NUMBER, &buf, 1);
	if (ret < 0) {
		input_err(true, &client->dev, "%s fail(%d)\n", __func__, ret);
	}
	if(info->fw_model_number != buf){
		input_info(true, &client->dev, "fw model number = %x ic model number = %x \n", info->fw_model_number, buf);
		fw_update = 1;
		goto flash_fw;
	}

	if ((info->fw_ver == 0) || info->fw_ver < info->fw_ver_bin){
		input_info(true, &client->dev, "excute tk firmware update (0x%x -> 0x%x\n",
			info->fw_ver, info->fw_ver_bin);
		fw_update = 1;
	}

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
	if(info->fw_ver >= 0xd0){		//test firmware
		input_info(true, &client->dev, "excute tk firmware update (0x%x -> 0x%x\n",
			info->fw_ver, info->fw_ver_bin);
		fw_update = 1;
	}
#endif

flash_fw:
	if(fw_update){
		ret = abov_flash_fw(info, true, BUILT_IN);
		if (ret) {
			input_err(true, &client->dev,
				"failed to abov_flash_fw (%d)\n", ret);
		} else {
			input_info(true, &client->dev,
				"fw update success\n");
		}
	}

	abov_release_fw(info, BUILT_IN);

	return ret;
}

static int abov_led_power(void *data, bool on)
{
	struct abov_tk_info *info = (struct abov_tk_info *)data;
	struct i2c_client *client = info->client;

	int ret = 0;

	info->pdata->avdd_vreg = regulator_get(NULL, "vtouch_3.3v");
	if (IS_ERR(info->pdata->avdd_vreg)) {
		info->pdata->avdd_vreg = NULL;
		return ret;
	}


	if(regulator_is_enabled(info->pdata->avdd_vreg)==on){
		input_info(true, &client->dev, "%s %s skip\n", __func__, on ? "on" : "off");
		return ret;
	}


	if (on) {
		if (info->pdata->avdd_vreg) {
			ret = regulator_enable(info->pdata->avdd_vreg);
			if(ret){
				input_err(true, &client->dev, "%s : avdd reg enable fail\n", __func__);
				return ret;
			}
		}
	} else {
		if (info->pdata->avdd_vreg) {
			input_info(true, &client->dev, "%s 1\