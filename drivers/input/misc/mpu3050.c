xx_dt_data), GFP_KERNEL);
		if (!data->dt_data) {
			input_err(true, &client->dev, "failed to allocate dt data\n");
			goto err_alloc_dev;
		}

		ret = ist40xx_parse_dt(&client->dev, data);
		if (ret)
			goto err_alloc_dev;

		tdata = kzalloc(sizeof(struct sec_tclm_data), GFP_KERNEL);
		if (!tdata)
			goto error_alloc_tdata;

		sec_tclm_parse_dt(client, tdata);
	} else {
		data->dt_data = NULL;
		input_err(true, &client->dev, "don't exist of_node\n");
		goto err_alloc_dev;
	}

	ist40xx_request_gpio(client, data);

	data->client = client;
#ifdef IST40XX_PINCTRL
	/* Get pinctrl if target uses pinctrl */
	data->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(data->pinctrl)) {
		if (PTR_ERR(data->pinctrl) == -EPROBE_DEFER)
			goto err_pinctrl;

		input_err(true, &client->dev, "%s Target does not use pinctrl\n",
			  __func__);
		data->pinctrl = NULL;
	}

	if (data->pinctrl) {
		ret = ist40xx_pinctrl_configure(data, true);
		if (ret) {
			input_err(true, &client->dev,
				  "%s cannot set pinctrl state\n", __func__);
		}
	}
#endif
#endif

	input_dev = input_allocate_device();
	if (!input_dev) {
		input_err(true, &client->dev, "input_allocate_device failed\n");
		goto err_alloc_dt;
	}

	data->input_dev = input_dev;
	i2c_set_clientdata(client, data);

	data->tdata = tdata;
	if (!data->tdata)
		goto err_null_data;

	data->input_dev_proximity = input_allocate_device();
	if (!data->input_dev_proximity) {
		input_err(true, &client->dev, "%s: allocate input_dev_proximity err!\n", __func__);
		ret = -ENOMEM;
		goto err_allocate_input_dev_proximity;
	}

	data->input_dev_proximity->name = "sec_touchproximity";
	ist_set_input_prop_proximity(data, data->input_dev_proximity);

#ifdef TCLM_CONCEPT
	sec_tclm_initialize(data->tdata);
	data->tdata->client = data->client;
	data->tdata->tclm_read = ist40xx_tclm_data_read;
	data->tdata->tclm_write = ist40xx_tclm_data_write;
	data->tdata->tclm_execute_force_calibration = ist40xx_execute_force_calibration;
	data->tdata->external_factory = false;
	data->tdata->tclm_parse_dt = sec_tclm_parse_dt;
#endif
	INIT_DELAYED_WORK(&data->work_read_info, ist40xx_read_info_work);

#ifdef USE_OPEN_CLOSE
	input_dev->open = ist40xx_ts_open;
	input_dev->close = ist40xx_ts_close;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = ist40xx_early_suspend;
	data->early_suspend.resume = ist40xx_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	data->irq_enabled = false;
	data->status.event_mode = false;
	mutex_init(&data->lock);
	mutex_init(&data->aod_lock);
	mutex_init(&data->i2c_lock);
	mutex_init(&data->i2c_wc_lock);

	ts_data = data;

	/* initialize data variable */
	data->ignore_delay = false;
	data->irq_working = false;
	data->max_scan_retry = 2;
	data->max_irq_err_cnt = IST40XX_MAX_ERR_CNT;
	data->report_rate = -1;
	data->idle_rate = -1;
	data->timer_period_ms = 5000;
	data->status.sys_mode = STATE_POWER_OFF;
	data->rec_mode = 0;
	data->rec_type = 0;
	data->rec_file_name = kzalloc(IST40XX_REC_FILENAME_SIZE, GFP_KERNEL);
	data->debug_mode = 0;
	data->checksum_result = 0;
	data->multi_count = 0;
	data->all_finger_count = 0;
	data->all_spay_count = 0;
	data->all_aod_tsp_count = 0;
	data->all_singletap_count = 0;
	data->lpm_mode = 0;
	data->info_work_done = false;
	data->rejectzone_t = 0;
	data->rejectzone_b = 0;
	data->hover = 0;
	data->fod_property = 0;

	for (i = 0; i < IST40XX_MAX_FINGER_ID; i++)
		data->tsp_touched[i] = false;

	for (i = 0; i < 4; i++)
		data->rect_data[i] = 0;

	/* init irq thread */
	input_info(true, &client->dev, "client->irq : %d\n", client->irq);
	ret = request_threaded_irq(client->irq, NULL, ist40xx_irq_thread,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "ist40xx_ts",
				   data);
	if (ret)
		goto err_init_drv;

	/* system power init */
	ret = ist40xx_init_system(data);
	if (ret) {
		input_err(true, &client->dev, "chip initialization failed\n");
		goto err_init_drv;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	trustedui_set_tsp_irq(client->irq);
	input_info(true, &client->dev, "%s[%d] called!\n", __func__, client->irq);
#endif

	ret = ist40xx_auto_bin_update(data);
	if (ret == 0)
		goto err_irq;

	ret = ist40xx_get_info(data);
	input_info(true, &client->dev, "Get info: %s\n", (ret == 0 ? "success" : "fail"));
	if (ret)
		goto err_read_info;

	ret = ist40xx_init_update_sysfs(data);
	if (ret)
		goto err_sysfs;

	ret = ist40xx_init_misc_sysfs(data);
	if (ret)
		goto err_sysfs;

	ret = ist40xx_init_cmcs_sysfs(data);
	if (ret)
		goto err_sysfs;

#ifdef SEC_FACTORY_MODE
	ret = sec_touch_sysfs(data);
	if (ret)
		goto err_sec_sysfs;
#endif

	input_info(true, &client->dev, "Create sysfs!!\n");

	INIT_DELAYED_WORK(&data->work_reset_check, reset_work_func);
#ifdef IST40XX_NOISE_MODE
	INIT_DELAYED_WORK(&data->work_noise_protect, noise_work_func);
#else
	INIT_DELAYED_WORK(&data->work_force_release, release_work_func);
#endif

	init_timer(&data->event_timer);
	data->event_timer.data = (unsigned long)data;
	data->event_timer.function = timer_handler;
	data->event_timer.expires = jiffies_64 + EVENT_TIMER_INTERVAL;
	mod_timer(&data->event_timer, get_jiffies_64() + EVENT_TIMER_INTERVAL);

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	manager_notifier_register(&data->ccic_nb, tsp_ccic_notification,
			MANAGER_NOTIFY_CCIC_USB);
#else
#ifdef CONFIG_MUIC_NOTIFIER
	muic_notifier_register(&data->muic_nb, tsp_muic_notification,
			MUIC_NOTIFY_DEV_CHARGER);
#endif
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	vbus_notifier_register(&data->vbus_nb, tsp_vbus_notification,
			VBUS_NOTIFY_DEV_CHARGER);
#endif

	device_init_wakeup(&client->dev, true);

	ist40xx_start(data);
	data->initialized = true;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tui_tsp_info = data;
#endif

#ifdef CONFIG_SAMSUNG_TUI
	stui_tsp_info = data;
#endif

	schedule_delayed_work(&data->work_read_info, msecs_to_jiffies(500));

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	dump_callbacks.inform_dump = dump_tsp_log;
	INIT_DELAYED_WORK(&data->ghost_check, ist40xx_check_rawdata);
	p_ghost_check = &data->ghost_check;
#endif

#ifdef CONFIG_PM
	init_completion(&data->resume_done);
	complete_all(&data->resume_done);
#endif

	input_info(true, &client->dev, "### IMAGIS probe success ###\n");

	return 0;

#ifdef SEC_FACTORY_MODE
err_sec_sysfs:
	sec_touch_sysfs_remove(data);
	sec_cmd_exit(&data->sec, SEC_CLASS_DEVT_TSP);
#endif
err_sysfs:
	class_destroy(ist40xx_class);
err_read_info:
err_irq:
	ist40xx_disable_irq(data);
	free_irq(client->irq, data);
err_init_drv:
	input_free_device(input_dev);
	data->status.event_mode = false;
	ist40xx_power_off(data);
#if (defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
	unregister_early_suspend(&data->early_suspend);
#endif
err_allocate_input_dev_proximity:
err_null_data:
err_alloc_dt:
#ifdef IST40XX_PINCTRL
err_pinctrl:
#endif
	if (tdata)
		kfree(tdata);
	if (data->dt_data)
		ist40xx_free_gpio(data);
error_alloc_tdata:
	if (data->dt_data) {
		input_err(true, &client->dev, "Error, ist40xx mem free\n");
		kfree(data->dt_data);
	}
err_alloc_dev:
#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif
	kfree(data);
	input_err(true, &client->dev, "Error, ist40xx init driver\n");

	return -ENODEV;
}

static int ist40xx_remove(struct i2c_client *client)
{
	struct ist40xx_data *data = i2c_get_clientdata(client);

#if (defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
	unregister_early_suspend(&data->early_suspend);
#endif

	ist40xx_disable_irq(data);
	free_irq(client->irq, data);
	ist40xx_power_off(data);

#ifdef SEC_FACTORY_MODE
	sec_touch_sysfs_remove(data);
	sec_cmd_exit(&data->sec, SEC_CLASS_DEVT_TSP);
#endif

#ifdef CONFIG_OF
	if (data->dt_data) {
		ist40xx_free_gpio(data);
		kfree(data->dt_data);
	}
#endif

	input_unregister_device(data->input_dev);
	input_free_device(data->input_dev);
	input_unregister_device(data->input_dev_proximity);
	input_free_device(data->input_dev_proximity);	
	kfree(data);

	return 0;
}

static void ist40xx_shutdown(struct i2c_client *client)
{
	struct ist40xx_data *data = i2c_get_clientdata(client);

	del_timer(&data->event_timer);
	cancel_delayed_work_sync(&data->work_reset_check);
#ifdef IST40XX_NOISE_MODE
	cancel_delayed_work_sync(&data->work_noise_protect);
#else
	cancel_delayed_work_sync(&data->work_force_release);
#endif
	mutex_lock(&data->lock);
	ist40xx_disable_irq(data);
	ist40xx_power_off(data);
	clear_input_data(data);
	mutex_unlock(&data->lock);
}

static struct i2c_device_id ist40xx_idtable[] = {
	{IST40XX_DEV_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, ist40xx_idtable);

#ifdef CONFIG_OF
static struct of_device_id ist40xx_match_table[] = {
	{.compatible = "imagis,ist40xx-ts",},
	{},
};
#else
#define ist40xx_match_table NULL
#endif

#if (!defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
static const struct dev_pm_ops ist40xx_pm_ops = {
	.suspend = ist40xx_suspend,
	.resume = ist40xx_resume,
};
#else
#ifdef CONFIG_PM
static int ist40xx_pm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ist40xx_data *data = i2c_get_clientdata(client);

	reinit_completion(&data->resume_done);

	return 0;
}

static int ist40xx_pm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ist40xx_data *data = i2c_get_clientdata(client);

	complete_all(&data->resume_done);

	return 0;
}

static const struct dev_pm_ops ist40xx_pm_ops = {
	.suspend = ist40xx_pm_suspend,
	.resume = ist40xx_pm_resume,
};
#endif
#endif

static struct i2c_driver ist40xx_i2c_driver = {
	.id_table = ist40xx_idtable,
	.probe = ist40xx_probe,
	.remove = ist40xx_remove,
	.shutdown = ist40xx_shutdown,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = IST40XX_DEV_NAME,
		   .of_match_table = ist40xx_match_table,
#if (!defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
		   .pm = &ist40xx_pm_ops,
#else
#ifdef CONFIG_PM
		   .pm = &ist40xx_pm_ops,
#endif
#endif
		   },
};

static int __init ist40xx_init(void)
{
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		tsp_info("%s: Do not load driver due to : lpm %d\n", __func__,
			 lpcharge);
		return -ENODEV;
	}
#endif

	return i2c_add_driver(&ist40xx_i2c_driver);
}

static void __exit ist40xx_exit(void)
{
	i2c_del_driver(&ist40xx_i2c_driver);
}

module_init(ist40xx_init);
module_exit(ist40xx_exit);

MODULE_DESCRIPTION("Imagis IST40XX touch driver");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Copyright (c) 2006 - 2011 Intel Corporation.  All rights reserved.
 *
 * This software is avail