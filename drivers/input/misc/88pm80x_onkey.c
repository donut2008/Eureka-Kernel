ata->dt_data->enable_fpcb_noise_test = of_property_read_bool(np, "enable_fpcb_noise_test");
	data->dt_data->support_open_short_test = of_property_read_bool(np, "support_open_short_test");
	data->dt_data->support_mis_calibration_test = of_property_read_bool(np, "support_mis_calibration_test");

	if (of_property_read_u32_array(np, "imagis,cm_spec", cm_spec, 3)) {
		input_err(true, dev, "%s: Failed to get zone's size\n", __func__);
		data->dt_data->cm_min_spec = CM_MIN_SPEC;
		data->dt_data->cm_max_spec = CM_MAX_SPEC;
		data->dt_data->cm_spec_gap = SPEC_GAP;
	} else {
		data->dt_data->cm_min_spec = cm_spec[0];
		data->dt_data->cm_max_spec = cm_spec[1];
		data->dt_data->cm_spec_gap = cm_spec[2];
	}

	input_info(true, dev, "%s: irq:%d, tsp_ldo: %s\n", __func__,
		   data->dt_data->irq_gpio, data->dt_data->regulator_avdd);

	input_info(true, dev, "%s: power source by gpio:%d, power_gpio: %d\n",
		   __func__, data->dt_data->is_power_by_gpio, data->dt_data->power_gpio);

	return 0;
}

void sec_tclm_parse_dt(struct i2c_client *client, struct sec_tclm_data *tdata)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "imagis,tclm_level", &tdata->tclm_level) < 0) {
		tdata->tclm_level = 0;
		input_err(true, &client->dev, "%s Failed to get tclm_level property\n",
			  __func__);
	}

	if (of_property_read_u32(np, "imagis,afe_base", &tdata->afe_base) < 0) {
		tdata->afe_base = 0;
		input_err(true, &client->dev, "%s Failed to get afe_base property\n",
			  __func__);
	}

	tdata->support_tclm_test = of_property_read_bool(np, "support_tclm_test");

	input_err(true, &client->dev, "%s tclm_level %d, afe_base %04X\n", __func__,
		  tdata->tclm_level, tdata->afe_base);
}

static void ist40xx_request_gpio(struct i2c_client *client,
		struct ist40xx_data *data)
{
	int ret;

	input_info(true, &client->dev, "%s\n", __func__);

	if (gpio_is_valid(data->dt_data->irq_gpio)) {
		ret = gpio_request(data->dt_data->irq_gpio, "imagis,irq_gpio");
		if (ret) {
			input_err(true, &client->dev,
				  "%s unable to request irq_gpio [%d]\n", __func__,
				  data->dt_data->irq_gpio);
			return;
		}

		ret = gpio_direction_input(data->dt_data->irq_gpio);
		if (ret) {
			input_err(true, &client->dev,
				  "%s unable to set direction for gpio [%d]\n",
				  __func__, data->dt_data->irq_gpio);
		}
		client->irq = gpio_to_irq(data->dt_data->irq_gpio);
	}

	if (gpio_is_valid(data->dt_data->power_gpio)) {
		ret = gpio_request(data->dt_data->power_gpio, "imagis,power_gpio");
		if (ret) {
			input_err(true, &client->dev,
				  "%s unable to request power_gpio [%d]\n", __func__,
				  data->dt_data->power_gpio);
			return;
		}

		ret = gpio_direction_output(data->dt_data->power_gpio, 1);
		if (ret) {
			input_err(true, &client->dev,
				  "%s unable to set direction for gpio [%d]\n",
				  __func__, data->dt_data->power_gpio);
		}
	}
}

static void ist40xx_free_gpio(struct ist40xx_data *data)
{
	input_info(true, &data->client->dev, "%s\n", __func__);

	if (gpio_is_valid(data->dt_data->irq_gpio))
		gpio_free(data->dt_data->irq_gpio);

	if (gpio_is_valid(data->dt_data->power_gpio))
		gpio_free(data->dt_data->power_gpio);
}
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
void trustedui_mode_ist_on(void)
{
	input_info(true, &data->client->dev, "%s release all finger..", __func__);
	clear_input_data(tui_tsp_info);

	del_timer(&tui_tsp_info->event_timer);

	cancel_delayed_work_sync(&tui_tsp_info->work_reset_check);
#ifdef IST40XX_NOISE_MODE
	cancel_delayed_work_sync(&tui_tsp_info->work_noise_protect);
#else
	cancel_delayed_work_sync(&tui_tsp_info->work_force_release);
#endif

	tui_tsp_info->status.noise_mode = false;
}
EXPORT_SYMBOL(trustedui_mode_ist_on);

void trustedui_mode_ist_off(void)
{
	input_info(true, &data->client->dev, "%s ", __func__);

	tui_tsp_info->status.noise_mode = true;

	//EVENT_TIMER_INTERVAL
	mod_timer(&tui_tsp_info->event_timer,
		  get_jiffies_64() + (HZ * tui_tsp_info->timer_period_ms / 1000));
}
EXPORT_SYMBOL(trustedui_mode_ist_off);
#endif

static void ist40xx_run_rawdata(struct ist40xx_data *data)
{
#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	data->tsp_dump_lock = 1;
#endif
	input_raw_data_clear();
	input_raw_info(true, &data->client->dev, "%s start ##\n", __func__);
	ist40xx_display_booting_dump_log(data);
	input_raw_info(true, &data->client->dev, "%s done 