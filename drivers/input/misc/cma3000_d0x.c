__);

#ifdef TCLM_CONCEPT
	sec_tclm_debug_info(data->tdata);
#endif
#if defined(CONFIG_INPUT_SEC_SECURE_TOUCH)
	ist40xx_secure_touch_stop(data, 1);
#endif

	ist40xx_suspend(&data->client->dev);
}

static int ist40xx_ts_open(struct input_dev *dev)
{
	struct ist40xx_data *data = input_get_drvdata(dev);

	if (!data->info_work_done) {
		input_err(true, &data->client->dev, "%s: not finished info work\n",
			  __func__);
		return 0;
	}

	input_info(true, &data->client->dev, "%s:\n", __func__);

	return ist40xx_resume(&data->client->dev);
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ist40xx_early_suspend(struct early_suspend *h)
{
	struct ist40xx_data *data = container_of(h, struct ist40xx_data,
			early_suspend);

	ist40xx_suspend(&data->client->dev);
}

static void ist40xx_late_resume(struct early_suspend *h)
{
	struct ist40xx_data *data = container_of(h, struct ist40xx_data,
			early_suspend);

	ist40xx_resume(&data->client->dev);
}
#endif

void ist40xx_set_ta_mode(bool mode)
{
	struct ist40xx_data *data = ts_data;

	if (mode == ((data->noise_mode >> NOISE_MODE_TA) & 1))
		return;

	input_info(true, &data->client->dev, "%s: mode = %d\n", __func__, mode);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_TA);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_TA);

	if (data->initialized && (data->status.sys_mode != STATE_POWER_OFF))
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist40xx_set_ta_mode);

void ist40xx_set_edge_mode(int mode)
{
	struct ist40xx_data *data = ts_data;

	input_info(true, &data->client->dev, "%s: mode = %d\n", __func__, mode);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_EDGE);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_EDGE);

	if (data->status.sys_mode != STATE_POWER_OFF)
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist40xx_set_edge_mode);

void ist40xx_set_call_mode(int mode)
{
	struct ist40xx_data *data = ts_data;

	if (mode == ((data->noise_mode >> NOISE_MODE_CALL) & 1))
		return;

	input_info(true, &data->client->dev, "%s: mode = %d\n", __func__, mode);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_CALL);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_CALL);

	if (data->initialized && (data->status.sys_mode != STATE_POWER_OFF))
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist40xx_set_call_mode);

void ist40xx_set_halfaod_mode(int mode)
{
	struct ist40xx_data *data = ts_data;

	if (mode == ((data->noise_mode >> NOISE_MODE_HALFAOD) & 1))
		return;

	input_info(true, &data->client->dev, "%s: mode = %d\n", __func__, mode);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_HALFAOD);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_HALFAOD);

	if (data->initialized && (data->status.sys_mode != STATE_POWER_OFF))
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist40xx_set_halfaod_mode);

void ist40xx_set_cover_mode(int mode)
{
	struct ist40xx_data *data = ts_data;

	if (mode == ((data->noise_mode >> NOISE_MODE_COVER) & 1))
		return;

	input_info(true, &data->client->dev, "%s: mode = %d\n", __func__, mode);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_COVER);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_COVER);

	if (data->initialized && (data->status.sys_mode != STATE_POWER_OFF))
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist40xx_set_cover_mode);

void ist40xx_set_glove_mode(int mode)
{
	struct ist40xx_data *data = ts_data;

	if (mode == ((data->noise_mode >> NOISE_MODE_GLOVE) & 1))
		return;

	input_info(true, &data->client->dev, "%s: mode = %d\n", __func__, mode);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_GLOVE);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_GLOVE);

	if (data->initialized && (data->status.sys_mode != STATE_POWER_OFF))
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist40xx_set_glove_mode);

int ist40xx_set_rejectzone_mode(int mode, u16 top, u16 bottom)
{
	struct ist40xx_data *data = ts_data;
	int ret;

	input_info(true, &data->client->dev, "%s: mode = %d (T:%d,B:%d)\n",
			__func__, mode, top, bottom);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_REJECTZONE);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_REJECTZONE);

	data->rejectzone_t = top;
	data->rejectzone_b = bottom;

	if (data->initialized && (data->status.sys_mode != STATE_POWER_OFF)) {
		ret = ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
		if (ret)
			return ret;
		ret = ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_REJECTZONE_TOP << 16) | data->rejectzone_t));
		if (ret)
			return ret;
		ret = ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_REJECTZONE_BOTTOM << 16) | data->rejectzone_b));
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL(ist40xx_set_rejectzone_mode);

void ist40xx_set_sensitivity_mode(int mode)
{
	struct ist40xx_data *data = ts_data;

	if (mode == ((data->noise_mode >> NOISE_MODE_SENSITIVITY) & 1))
		return;

	input_info(true, &data->client->dev, "%s: mode = %d\n", __func__, mode);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_SENSITIVITY);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_SENSITIVITY);

	if (data->initialized && (data->status.sys_mode != STATE_POWER_OFF))
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
			((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist40xx_set_sensitivity_mode);

void ist40xx_set_touchable_mode(int mode)
{
	struct ist40xx_data *data = ts_data;

	if (mode == ((data->noise_mode >> NOISE_MODE_TOUCHABLE) & 1))
		return;

	input_info(true, &data->client->dev, "%s: mode = %d\n", __func__, mode);

	if (mode)
		data->noise_mode |= (1 << NOISE_MODE_TOUCHABLE);
	else
		data->noise_mode &= ~(1 << NOISE_MODE_TOUCHABLE);

	if (data->initialized && (data->status.sys_mode != STATE_POWER_OFF))
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
			((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist40xx_set_touchable_mode);

#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER) || defined(CONFIG_MUIC_NOTIFIER)
static int otg_flag = 0;
#endif

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
static int tsp_ccic_notification(struct notifier_block *nb,
	   unsigned long action, void *data)
{
	CC_NOTI_USB_STATUS_TYPEDEF usb_status =
	    *(CC_NOTI_USB_STATUS_TYPEDEF *) data;

	switch (usb_status.drp) {
	case USB_STATUS_NOTIFY_ATTACH_DFP:
		otg_flag = 1;
		tsp_info("%s : otg_flag 1\n", __func__);
		break;
	case USB_STATUS_NOTIFY_DETACH:
		otg_flag = 0;
		break;
	default:
		break;
	}

	return 0;
}
#else
#ifdef CONFIG_MUIC_NOTIFIER
static int tsp_muic_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;

	switch (action) {
	case MUIC_NOTIFY_CMD_DETACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
		otg_flag = 0;
		break;
	case MUIC_NOTIFY_CMD_ATTACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
		if (attached_dev == ATTACHED_DEV_OTG_MUIC) {
			otg_flag = 1;
			tsp_info("%s : otg_flag 1\n", __func__);
		}
		break;
	default:
		break;
	}

	return 0;
}
#endif
#endif

#ifdef CONFIG_VBUS_NOTIFIER
static int tsp_vbus_notification(struct notifier_block *nb,
		unsigned long cmd, void *data)
{
	vbus_status_t vbus_type = *(vbus_status_t *) data;

	tsp_info("%s cmd=%lu, vbus_type=%d\n", __func__, cmd, vbus_type);

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
		tsp_info("%s : attach\n", __func__);
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER) || defined(CONFIG_MUIC_NOTIFIER)
		if (!otg_flag)
#endif
			ist40xx_set_ta_mode(true);
		break;
	case STATUS_VBUS_LOW:
		tsp_info("%s : detach\n", __func__);
		ist40xx_set_ta_mode(false);
		break;
	default:
		break;
	}
	return 0;
}
#endif

static void reset_work_func(struct work_struct *work)
{
	struct delayed_work *delayed_work = to_delayed_work(work);
	struct ist40xx_data *data = container_of(delayed_work, struct ist40xx_data,
			work_reset_check);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &data->client->dev, "%s: return, TUI is enabled!\n",
			  __func__);
		return;
	}
#endif
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&data->st_enabled) == SECURE_TOUCH_ENABLED) {
		input_err(true, &data->client->dev,
			  "%s: TSP no accessible from Linux, TUI is enabled!\n",
			  __func__);
		return;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return;
#endif

	if ((data == NULL) || (data->client == NULL))
		return;

	input_info(true, &data->client->dev, "Request reset function\n");

	if ((data->initialized == 1) &&
	    (data->status.sys_mode != STATE_POWER_OFF) &&
	    (data->status.update != 1) && (data->status.calib < 1) &&
	    (data->status.miscalib < 1)) {
		mutex_lock(&data->lock);
		ist40xx_disable_irq(data);
		ist40xx_reset(data, false);
		clear_input_data(data);
		ist40xx_enable_irq(data);
		ist40xx_start(data);
		mutex_unlock(&data->lock);
	}
}

#ifdef IST40XX_NOISE_MODE
static void noise_work_func(struct work_struct *work)
{
	int ret;
	u32 touch_status = 0;
	u32 scan_count = 0;
	struct delayed_work *