CALIB_MASK) == IST40XX_MISCALIB_MSG) {
		input_info(true, &data->client->dev, "Mis calibration End\n");
		data->status.miscalib_msg = *msg;
		data->status.miscalib_result = IST40XX_MISCALIB_VAL(*msg);
		goto irq_event;
	}

	if ((*msg & CALIB_MSG_MASK) == CALIB_MSG_VALID) {
		ret = ist40xx_burst_read(data->client, IST40XX_HIB_INTR_MSG,
				data->status.calib_msg, IST40XX_MAX_CALIB_SIZE, true);
		input_info(true, &data->client->dev, "Auto calibration\n");
		input_info(true, &data->client->dev, "SLF calib status:0x%08X\n",
			   data->status.calib_msg[0]);
		input_info(true, &data->client->dev, "MTL calib status:0x%08X\n",
			   data->status.calib_msg[1]);
		input_info(true, &data->client->dev, "MAX CH status   :0x%08X\n",
			   data->status.calib_msg[2]);
		goto irq_event;
	}

	if ((CMCS_MSG(*msg) == CM_MSG_VALID) || (CMCS_MSG(*msg) == CS_MSG_VALID) ||
		(CMCS_MSG(*msg) == CMJIT_MSG_VALID) ||
		(CMCS_MSG(*msg) == CRJIT_MSG_VALID) ||
		(CMCS_MSG(*msg) == CRJIT2_MSG_VALID)) {
		data->status.cmcs = *msg;
		data->status.cmcs_result = CMCS_RESULT(*msg);
		input_info(true, &data->client->dev, "CMCS notify: 0x%08X\n", *msg);
		goto irq_event;
	}

	ret = PARSE_SPECIAL_MESSAGE(*msg);
	if (ret >= 0) {
		tsp_debug("special cmd: %d (0x%08X)\n", ret, *msg);
		ist40xx_special_cmd(data, ret);

		goto irq_event;
	}

	memset(data->fingers, 0, sizeof(data->fingers));

	if ((!CHECK_INTR_STATUS(*msg)))
		goto irq_err;

	read_cnt = PARSE_TOUCH_CNT(*msg);
	if (read_cnt <= 0 && !PARSE_HOVER_NOTI(*msg)) {
		input_err(true, &data->client->dev, "report touch is none\n");
		   goto irq_err;
	}

	if (PARSE_HOVER_NOTI(*msg)) {
		if (data->hover != PARSE_HOVER_VAL(*msg))
			data->hover = PARSE_HOVER_VAL(*msg);
		input_report_abs(data->input_dev_proximity, ABS_MT_CUSTOM, data->hover);
		input_sync(data->input_dev_proximity);
		input_info(true, &data->client->dev, "Hover Level %d\n", data->hover);

		if (read_cnt <= 0)
			goto irq_event;
	}

	ret = ist40xx_burst_read(data->client, IST40XX_HIB_COORD, &msg[offset],
			read_cnt * IST40XX_TOUCH_FRAME_CNT, true);
	if (ret)
		goto irq_err;

	tsp_debug("Read Cnt:%d\n", read_cnt);
	for (i = 0; i < read_cnt; i++) {
		tsp_verb("%2d:0x%08X\n", i, msg[i * 2 + offset]);
		tsp_verb("%2s 0x%08X\n", "  ", msg[i * 2 + 1 + offset]);
	}

	data->t_status = *msg;
	memcpy(data->fingers, &msg[offset], sizeof(finger_info) * read_cnt);

	report_input_data(data);

	if (data->intr_debug3_size > 0) {
		buf32 = kzalloc(data->intr_debug3_size * sizeof(u32), GFP_KERNEL);
		if (!buf32) {
			input_err(true, &data->client->dev, "failed to allocate %s %d\n",
				  __func__, __LINE__);
			goto irq_err;
		}
		tsp_debug("Intr_debug3 (addr: 0x%08x)\n", data->intr_debug3_addr);
		ist40xx_burst_read(data->client,
				IST40XX_DA_ADDR(data->intr_debug3_addr), buf32,
				data->intr_debug3_size, true);

		for (i = 0; i < data->intr_debug3_size; i++)
			tsp_debug(" %08x\n", buf32[i]);
		kfree(buf32);
	}

	goto irq_end;

irq_err:
	input_err(true, &data->client->dev, "intr msg: 0x%08x, ret: %d\n",
		  msg[0], ret);
	ist40xx_request_reset(data);
	goto irq_event;
irq_end:
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	if (data->rec_mode)
		recording_data(data, idle);
#endif
irq_event:
irq_ignore:
	data->irq_working = false;
	data->event_ms = (u32) get_milli_second(data);
	if (data->initialized)
		mod_timer(&data->event_timer,
			  get_jiffies_64() + EVENT_TIMER_INTERVAL);
	return IRQ_HANDLED;

irq_ic_err:
	ist40xx_scheduled_reset(data);
	data->irq_working = false;
	data->event_ms = (u32) get_milli_second(data);
	if (data->initialized)
		mod_timer(&data->event_timer,
			  get_jiffies_64() + EVENT_TIMER_INTERVAL);
	return IRQ_HANDLED;
}

#ifdef IST40XX_PINCTRL
static int ist40xx_pinctrl_configure(struct ist40xx_data *data, bool active)
{
	struct pinctrl_state *set_state;

	int retval;

	input_info(true, &data->client->dev, "%s %s\n", __func__,
		   active ? "ACTIVE" : "SUSPEND");

	set_state = pinctrl_lookup_state(data->pinctrl,
					 active ? "on_state" : "off_state");
	if (IS_ERR(set_state)) {
		input_err(true, &data->client->dev, "%s cannot get active state\n",
			  __func__);
		return -EINVAL;
	}

	retval = pinctrl_select_state(data->pinctrl, set_state);
	if (retval) {
		input_err(true, &data->client->dev, "%s cannot set pinctrl %s state\n",
			__func__, active ? "active" : "suspend");
		return -EINVAL;
	}

	return 0;
}
#endif

static int ist40xx_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ist40xx_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
		input_err(true, &data->client->dev, "%s TUI cancel event call!\n",
			  __func__);
		msleep(100);
		tui_force_close(1);
		msleep(200);
		if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
			input_err(true, &data->client->dev, "%s TUI flag force clear!\n",
				  __func__);
			trustedui_clear_mask(
					TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
			trustedui_set_mode(TRUSTEDUI_MODE_OFF);
		}
	}
#endif

	del_timer(&data->event_timer);
	cancel_delayed_work_sync(&data->work_reset_check);
#ifdef IST40XX_NOISE_MODE
	cancel_delayed_work_sync(&data->work_noise_protect);
#else
	cancel_delayed_work_sync(&data->work_force_release);
#endif

	mutex_lock(&data->lock);
	if (data->lpm_mode || data->fod_lp_mode) {
		ist40xx_disable_irq(data);
		ist40xx_cmd_gesture(data, IST40XX_ENABLE);

		if (device_may_wakeup(&data->client->dev))
			enable_irq_wake(data->client->irq);

		data->status.sys_mode = STATE_LPM;
		ist40xx_enable_irq(data);
	} else {
		ist40xx_power_