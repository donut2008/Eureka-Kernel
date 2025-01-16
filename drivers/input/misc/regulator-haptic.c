tring[data->tdata->nvdata.cal_position].f_name,
						   (data->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ");
#else
					input_info(true, &data->client->dev,
						   "%s%d loc:%s dX,dY:%d,%d mc:%d (0x%02x) \n",
						   TOUCH_UP_MESSAGE, id, location,
						   data->r_x[id] - data->p_x[id],
						   data->r_y[id] - data->p_y[id], data->move_count[id],
						   data->fw.cur.fw_ver);
#endif
					data->tsp_touched[id] = false;
				}

				data->move_count[id] = 0;
			}

			idx++;
#ifdef IST40XX_FORCE_RELEASE
		} else {
			if (data->tsp_touched[i] == true) {
				input_mt_slot(data->input_dev, i);
				input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,
						false);

				data->touch_pressed_num--;

				location_detect(data, location, data->r_x[id], data->r_y[id]);
#ifdef TCLM_CONCEPT
				input_info(true, &data->client->dev,
					   "%s%d loc:%s dX,dY:%d,%d mc:%d (0x%02x) (test_result data :%x) (C%02XT%04X.%4s%s) by force\n",
					   TOUCH_UP_MESSAGE, i, location,
					   data->r_x[i] - data->p_x[i],
					   data->r_y[i] - data->p_y[i],
					   data->move_count[i], data->fw.cur.fw_ver,
					   data->test_result.data[0],
					   data->tdata->nvdata.cal_count,
					   data->tdata->nvdata.tune_fix_ver,
					   data->tdata->tclm_string[data->tdata->nvdata.cal_position].f_name,
					   (data->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ");
#else
				input_info(true, &data->client->dev,
					   "%s%d loc:%s dX,dY:%d,%d mc:%d (0x%02x) by force\n",
					   TOUCH_UP_MESSAGE, i, location,
					   data->r_x[i] - data->p_x[i],
					   data->r_y[i] - data->p_y[i], data->move_count[i],
					   data->fw.cur.fw_ver);
#endif
				data->tsp_touched[i] = false;
				data->move_count[i] = 0;
			}
#endif
		}
	}
	if (data->touch_pressed_num >= 1){
		input_report_key(data->input_dev, BTN_TOUCH, 1);
		input_report_key(data->input_dev, BTN_TOOL_FINGER, 1);
	}
	else if (data->touch_pressed_num == 0){
		input_report_key(data->input_dev, BTN_TOUCH, 0);
		input_report_key(data->input_dev, BTN_TOOL_FINGER, 0);
	}
		
	if ((data->touch_pressed_num >= 2) && (data->check_multi == 0)) {
		data->check_multi = 1;
		data->multi_count++;
	}

	if (data->touch_pressed_num < 2)
		data->check_multi = 0;

	data->irq_err_cnt = 0;
	data->scan_retry = 0;

	input_sync(data->input_dev);
}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
void recording_data(struct ist40xx_data *data, bool idle)
{
	int ret;
	u32 scancnt = 0;
	u32 addr = 0;
	u32 *buf32;
	TSP_INFO *tsp = &data->tsp_info;

	if (idle && (data->rec_mode > 1))
		goto state_idle;

	ist40xx_delay(data->rec_delay);

	ret = ist40xx_read_reg(data->client, IST40XX_HIB_TOUCH_STATUS,
				   &scancnt);
	if (ret) {
		input_err(true, &data->client->dev, "%s failed to read scancnt\n",
			  __func__);
		goto state_idle;
	}

	if (data->recording_scancnt == scancnt) {
		input_err(true, &data->client->dev, "%s same recording scancnt\n",
			  __func__);
		goto state_idle;
	}

	data->recording_scancnt = scancnt;

	buf32 = kzalloc(data->rec_size +
			((tsp->node.self_len*2) + tsp->node.len) * sizeof(u32),
			GFP_KERNEL);
	if (!buf32) {
		input_err(true, &data->client->dev, "failed to allocate %s %d\n",
			  __func__, __LINE__);
		goto state_idle;
	}

	if (data->rec_size > 0) {
		addr = IST40XX_DA_ADDR(data->rec_addr);
		ret = ist40xx_burst_read(data->client, addr, buf32,
					 data->rec_size / IST40XX_DATA_LEN,
					 true);
		if (ret)
			goto err_rec_fail;
	}

	if ((data->rec_type == 0) || (data->rec_type == 1)) {
		addr = IST40XX_DA_ADDR(data->self_cdc_addr);
		ret = ist40xx_burst_read(data->client, addr,
					buf32 + (data->rec_size / IST40XX_DATA_LEN),
					tsp->node.self_len, true);
		if (ret)
			goto err_rec_fail;
	}

	if ((data->rec_type == 0) || (data->rec_type == 2)) {
		addr = IST40XX_DA_ADDR(data->cdc_addr);
		ret = ist40xx_burst_read(data->client, addr,
					buf32 + (data->rec_size / IST40XX_DATA_LEN) +
					tsp->node.self_len, tsp->node.len, true);
		if (ret)
			goto err_rec_fail;
	}

	if (data->rec_type == 3) {
		addr = IST40XX_DA_ADDR(data->prox_cdc_addr);
		ret = ist40xx_burst_read(data->client, addr,
					buf32 + (data->rec_size / IST40XX_DATA_LEN) +
					tsp->node.self_len + tsp->node.len, tsp->node.self_len, true);
		if (ret)
			goto err_rec_fail;
	}

	ist40xx_recording_put_frame(data, buf32,
					(data->rec_size / IST40XX_DATA_LEN) +
					(tsp->node.self_len*2) + tsp->node.len);

err_rec_fail:
	kfree(buf32);

state_idle:
	data->ignore_delay = true;
	ist40xx_write_cmd(data, IST40XX_HIB_CMD,
			  (eHCOM_SET_REC_MODE << 16) | (IST40XX_START_SCAN &
							0xFFFF));
	data->ignore_delay = false;
}
#endif

/*
 * CMD : CMD_GET_COORD
 *
 *   1st  [31:24]   [23:21]   [20:16]   [15:12]   [11]  [10]   [9:0]
 *		Checksum  KeyCnt	KeyStatus FingerCnt Rsvd. Palm   FingerStatus
 *   2nd  [31:28]   [27:24]   [23:12]   [11:0]
 *		Major	 Minor	 X		 Y
 */
irqreturn_t ist40xx_irq_thread(int irq, void *ptr)
{
	int i, ret = 0;
	int read_cnt;
	int offset = 1;
	u32 msg[IST40XX_MAX_FINGER_ID * IST40XX_TOUCH_FRAME_CNT + offset];
	u32 *buf32;
	u32 ms;
	bool idle = false;
	struct ist40xx_data *data = (struct ist40xx_data *)ptr;

	data->irq_working = true;
	memset(msg, 0, sizeof(msg));

	if (!data->irq_enabled)
		goto irq_ignore;

#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	if (ist40xx_filter_interrupt(data) == IRQ_HANDLED) {
		ret = wait_for_completion_interruptible_timeout((&data->st_interrupt),
				msecs_to_jiffies(10 * MSEC_PER_SEC));
		return IRQ_HANDLED;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return IRQ_HANDLED;
#endif

	if (data->status.sys_mode == STATE_LPM) {
		pm_wakeup_event(data->input_dev->dev.parent, 2000);
#ifdef CONFIG_PM
		ret = wait_for_completion_interruptible_timeout(&data->resume_done, msecs_to_jiffies(500));
		if (ret == 0) {
			input_err(true, &data->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return IRQ_HANDLED;
		} else if (ret < 0) {
			input_err(true, &data->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return IRQ_HANDLED;
		}
#endif
	} else if (data->status.sys_mode == STATE_POWER_OFF)
		goto irq_ignore;

	ms = get_milli_second(data);

	if (data->intr_debug1_size > 0) {
		buf32 = kzalloc(data->intr_debug1_size * sizeof(u32), GFP_KERNEL);
		if (!buf32) {
			input_err(true, &data->client->dev, "failed to allocate %s %d\n",
				  __func__, __LINE__);
			goto irq_err;
		}

		tsp_debug("Intr_debug1 (addr: 0x%08x)\n", data->intr_debug1_addr);
		ist40xx_burst_read(data->client,
				IST40XX_DA_ADDR(data->intr_debug1_addr), buf32,
				data->intr_debug1_size, true);

		for (i = 