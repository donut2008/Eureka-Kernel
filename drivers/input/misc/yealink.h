bov_tk_stop(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct abov_tk_info *info = i2c_get_clientdata(client);

	if (!info->enabled)
		return 0;

	input_info(true, &client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);

	disable_irq(info->irq);
	info->enabled = false;
	release_all_fingers(info);

	if (info->dtdata->power)
		info->dtdata->power(info, false);

	return 0;
}

static int abov_tk_start(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct abov_tk_info *info = i2c_get_clientdata(client);
	u8 led_data;

	if (!info->probe_done)
		return 0;

	if (info->enabled)
		return 0;

	input_info(true, &client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);

	if (info->dtdata->power) {
		info->dtdata->power(info, true);
		msleep(ABOV_RESET_DELAY);
	}

	info->enabled = true;

	if (abov_touchled_cmd_reserved && abov_touchkey_led_status == CMD_LED_ON) {
		abov_touchled_cmd_reserved = 0;
		led_data = abov_touchkey_led_status;

		abov_tk_i2c_write(client, ABOV_LED_CONTROL, &led_data, 1);

		input_info(true, &client->dev, "%s: LED reserved %s\n",
			__func__, (led_data == CMD_LED_ON) ? "on" : "off");
	}
	enable_irq(info->irq);

	return 0;
}
#endif

#ifdef USE_OPEN_CLOSE
static int abov_tk_input_open(struct input_dev *dev)
{
	struct abov_tk_info *info = input_get_drvdata(dev);
	if (!info->probe_done)
		return 0;
#ifdef CONFIG_TOUCHKEY_GRIP
	input_info(true, &info->client->dev,
			"%s: sar_enable(%d)\n", __func__, info->sar_enable);
	if (info->flip_mode)
		abov_sar_only_mode(info, 1);
	else
		abov_sar_only_mode(info, 0);

	if (device_may_wakeup(&info->client->dev))
		disable_irq_wake(info->irq);
#else
	gpio_direction_input(info->dtdata->gpio_scl);
	gpio_direction_input(info->dtdata->gpio_sda);
	abov_tk_start(&info->client->dev);
	if (info->dtdata->ta_notifier && ta_connected) {
		abov_set_ta_status(info);
	}

#ifdef GLOVE_MODE
	if (info->glovemode)
		abov_mode_enable(client, ABOV_GLOVE, CMD_ON);
#endif
#endif
	return 0;
}
static void abov_tk_input_close(struct input_dev *dev)
{
	struct abov_tk_info *info = input_get_drvdata(dev);
	if (!info->probe_done)
		return;
#ifdef LED_TWINKLE_BOOTING
	info->led_twinkle_check = 0;
#endif
#ifdef CONFIG_TOUCHKEY_GRIP
	input_info(true, &info->client->dev,
			"%s: sar_enable(%d)\n", __func__, info->sar_enable);
	abov_sar_only_mode(info, 1);

	if (device_may_wakeup(&info->client->dev))
		enable_irq_wake(info->irq);
#else
	abov_tk_stop(&info->client->dev);
#endif
}
#endif

#ifdef CONFIG_PM
static int abov_pm_suspend(struct device *dev)
{
	return 0;
}

static int abov_pm_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct i2c_device_id abov_tk_id[] = {
	{ABOV_TK_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, abov_tk_id);

#ifdef CONFIG_OF
static struct of_device_id abov_match_table[] = {
	{ .compatible = "abov,a96t3x6",},
	{ },
};
#else
#define abov_match_table NULL
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops abov_pm_ops = {
	.suspend = abov_pm_suspend,
	.resume = abov_pm_resume,
};
#endif

static struct i2c_driver abov_tk_driver = {
	.probe = abov_tk_probe,
	.remove = abov_tk_remove,
	.shutdown = abov_tk_shutdown,
	.driver = {
		.name = ABOV_TK_NAME,
		.of_match_table = abov_match_table,
#if (!defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
		.pm = &abov_pm_ops,
#endif

	},
	.id_table = abov_tk_id,
};

#if defined(CONFIG_BATTERY_SAMSUNG) && !defined(CONFIG_TOUCHKEY_GRIP)
extern unsigned int lpcharge;
#endif
static int __init touchkey_init(void)
{
	pr_err("%s %s: abov,a96t3x6\n", SECLOG, __func__);
#if defined(CONFIG_BATTERY_SAMSUNG) && !defined(CONFIG_TOUCHKEY_GRIP)
	if (lpcharge == 1) {
		pr_notice("%s %s : LPM Charging Mode!!\n", SECLOG, __func__);
		return 0;
	}
#endif

	return i2c_add_driver(&abov_tk_driver);
}

static void __exit touchkey_exit(void)
{
	i2c_del_driver(&abov_tk_driver);
}

module_init(touchkey_init);
module_exit(touchkey_exit);

/* Module information */
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Touchkey driver for Abov A96T3X6 chip");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 