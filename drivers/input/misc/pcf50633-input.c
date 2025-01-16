OUCH_STATUS,
			&touch_status);
	if (ret) {
		input_err(true, &data->client->dev, "Touch status read fail!\n");
		return;
	}

	tsp_verb("Touch Info: 0x%08x\n", touch_status);

	/* Check valid scan count */
	if ((touch_status & TOUCH_STATUS_MASK) != TOUCH_STATUS_MAGIC) {
		input_err(true, &data->client->dev, "Touch status is not corrected! (0x%08x)\n",
			  touch_status);
		return;
	}

	/* Status of IC is idle */
	if (GET_FINGER_ENABLE(touch_status) == 0)
		clear_input_data(data);
}
#endif

void timer_handler(unsigned long timer_data)
{
	struct ist40xx_data *data = (struct ist40xx_data *)timer_data;
	struct ist40xx_status *status = &data->status;

	if (data->irq_working || !data->initialized || data->rec_mode ||
			!status->event_mode)
		goto restart_timer;

	if ((status->sys_mode == STATE_POWER_ON) && (status->update != 1) &&
			(status->calib < 1) && (status->miscalib < 1)) {
		data->timer_ms = (u32) get_milli_second(data);

		if (status->noise_mode) {
			/* 100ms after last interrupt */
			if (data->timer_ms - data->event_ms > 100) {
#ifdef IST40XX_NOISE_MODE
				schedule_delayed_work(&data->work_noise_protect, 0);
#else
				schedule_delayed_work(&data->work_force_release, 0);
#endif
			}
		}
	}

restart_timer:
	mod_timer(&data->event_timer, get_jiffies_64() + EVENT_TIMER_INTERVAL);
}

#ifdef CONFIG_OF
static int ist40xx_parse_dt(struct device *dev, struct ist40xx_data *data)
{
	struct device_node *np = dev->of_node;
	u32 px_zone[3];
	u32 cm_spec[3];

	data->dt_data->irq_gpio = of_get_named_gpio(np, "imagis,irq-gpio", 0);

	data->dt_data->is_power_by_gpio = of_property_read_bool(np,
			"imagis,power-gpioen");
	if (data->dt_data->is_power_by_gpio) {
		data->dt_data->power_gpio = of_get_named_gpio(np, "imagis,power-gpio", 0);
	} else {
		data->dt_data->power_gpio = -1;
		if (of_property_read_string(np, "imagis,regulator_avdd",
					&data->dt_data->regulator_avdd)) {
			input_err(true, dev, "%s Failed to get regulator_avdd name property\n",
				  __func__);
		}
	}

	if (of_property_read_u32(np, "imagis,fw-bin", &data->dt_data->fw_bin) >= 0) {
		input_info(true, dev, "%s: fw-bin: %d\n", __func__, data->dt_data->fw_bin);
	}

	if (of_property_read_string(np, "imagis,ic-version",
				&data->dt_data->ic_version) >= 0) {
		input_info(true, dev, "%s: ic_version: %s\n", __func__,
			   data->dt_data->ic_version);
	}

	if (of_property_read_string(np, "imagis,project-name",
				&data->dt_data->project_name) >= 0) {
		input_info(true, dev, "%s: project_name: %s\n", __func__,
			   data->dt_data->project_name);
	}

	if (data->dt_data->ic_version && data->dt_data->project_name) {
		snprintf(data->dt_data->fw_path, FIRMWARE_PATH_LENGTH, "%s%s_%s.bin",
			 FIRMWARE_PATH, data->dt_data->ic_version,
			 data->dt_data->project_name);
		input_info(true, dev, "%s: firm path: %s\n", __func__,
			   data->dt_data->fw_path);

		snprintf(data->dt_data->cmcs_path, FIRMWARE_PATH_LENGTH,
			 "%s%s_%s_cmcs.bin", FIRMWARE_PATH,
			 data->dt_data->ic_version, data->dt_data->project_name);
		input_info(true, dev, "%s: