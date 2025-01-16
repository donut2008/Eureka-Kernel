
	return ret;
}

#ifdef CONFIG_OF
static int abov_parse_dt(struct device *dev,
			struct abov_touchkey_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int ret;
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	int i;
	u32 tmp[LIGHT_TABLE_MAX] = {0, };
#endif

	pdata->gpio_int = of_get_named_gpio(np, "abov,irq-gpio", 0);
	if(pdata->gpio_int < 0){
		input_err(true, dev, "unable to get gpio_int\n");
		return pdata->gpio_int;
	}

	pdata->gpio_scl = of_get_named_gpio(np, "abov,scl-gpio", 0);
	if(pdata->gpio_scl < 0){
		input_err(true, dev, "unable to get gpio_scl\n");
		return pdata->gpio_scl;
	}

	pdata->gpio_sda = of_get_named_gpio(np, "abov,sda-gpio", 0);
	if(pdata->gpio_sda < 0){
		input_err(true, dev, "unable to get gpio_sda\n");
		return pdata->gpio_sda;
	}

	pdata->sub_det = of_get_named_gpio(np, "abov,sub-det",0);
	if(pdata->sub_det < 0){
		input_info(true, dev, "unable to get sub_det\n");
	}else{
		input_info(true, dev, "%s: sub_det:%d\n",__func__,pdata->sub_det);
	}

	if (of_property_read_bool(np, "abov,use_gpio_ldo")) {
		input_info(true, dev, "%s: use use_gpio_ldo\n", __func__);

		pdata->gpio_ldo_en = of_get_named_gpio(np, "abov,gpio_ldo_en", 0);
		if (gpio_is_valid(pdata->gpio_ldo_en)) {
			gpio_request_one(pdata->gpio_ldo_en, GPIOF_OUT_INIT_LOW, "TOUCHKEY_GPIO_OUTPUT_LOW");
		} else {
			input_err(true, dev, "failed to get tsp_ldo_en\n");
			return -EINVAL;
		}
	}

	ret = of_property_read_string(np, "abov,fw_path", (const char **)&pdata->fw_path);
	if (ret) {
		input_err(true, dev, "touchkey:failed to read fw_path %d\n", ret);
		pdata->fw_path = TK_FW_PATH_BIN;
	}
	input_info(true, dev, "%s: fw path %s\n", __func__, pdata->fw_path);

	pdata->boot_on_ldo = of_property_read_bool(np, "abov,boot-on-ldo");
	pdata->bringup = of_property_read_bool(np, "abov,bringup");
	pdata->ta_notifier = of_property_read_bool(np, "abov,ta-notifier");

	input_info(true, dev, "%s: gpio_int:%d, gpio_scl:%d, gpio_sda:%d\n",
			__func__, pdata->gpio_int, pdata->gpio_scl,
			pdata->gpio_sda);

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	ret = of_property_read_u32_array(np, "abov,light_version", tmp, 2);
	if (ret) {
		input_err(true, dev, "touchkey:failed to read light_version %d\n", ret);
	}
	pdata->dt_light_version = tmp[0];
	pdata->dt_light_table = tmp[1];

	input_info(true, dev, "%s: light_version:%d, light_table:%d\n",
			__func__, pdata->dt_light_version, pdata->dt_light_table);

	if(pdata->dt_light_table > 0){
		ret = of_property_read_u32_array(np, "abov,octa_id", tmp, pdata->dt_light_table);
		if (ret) {
			input_err(true, dev, "touchkey:failed to read light_version %d\n", ret);
		}
		for(i = 0 ; i < pdata->dt_light_table ; i++){
			tkey_light_reg_table[i].octa_id = tmp[i];
		}

		ret = of_property_read_u32_array(np, "abov,light_reg", tmp, pdata->dt_light_table);
		if (ret) {
			input_err(true, dev, "touchkey:failed to read light_version %d\n", ret);
		}
		for(i = 0 ; i < pdata->dt_light_table ; i++){
			tkey_light_reg_table[i].led_reg = tmp[i];
		}

		for(i = 0 ; i < pdata->dt_light_table ; i++){
			input_info(true, dev, "%s: tkey_light_reg_table: %d 0x%02x\n",
				__func__, tkey_light_reg_table[i].octa_id, tkey_light_reg_table[i].led_reg);
		}
	}
#endif
	return 0;
}
#else
static int abov_parse_dt(struct device *dev,
			struct abov_touchkey_platform_data *pdata)
{
	return -ENODEV;
}
#endif

static int abov_tk_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct abov_tk_info *info;
	struct input_dev *input_dev;
	int ret = 0;
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	int i;
	char tmp[2] = {0, };
#endif

#ifdef LED_TWINKLE_BOOTING
	if (get_samsung_lcd_attached() == 0) {
                input_err(true, &client->dev, "%s : get_samsung_lcd_attached()=0 \n", __func__);
                return -EIO;
        }
#endif
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev,
			"i2c_check_functionality fail\n");
		return -EIO;
	}

	info = kzalloc(sizeof(struct abov_tk_info), GFP_KERNEL);
	if (!info) {
		input_err(true, &client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		input_err(true, &client->dev,
			"Failed to allocate memory for input device\n");
		ret = -ENOMEM;
		goto err_input_alloc;
	}

	info->client = client;
	info->input_dev = input_dev;

#ifdef CONFIG_TOUCHKEY_GRIP
	wake_lock_init(&info->touckey_wake_lock, WAKE_LOCK_SUSPEND, "touchkey wake lock");
#endif

	if (client->dev.of_node) {
		struct abov_touchkey_platform_data *pdata;
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct abov_touchkey_platform_data), GFP_KERNEL);
		if (!pdata) {
			input_err(true, &client->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_config;
		}

		ret = abov_parse_dt(&client->dev, pdata);
		if (ret){
			input_err(true, &client->dev, "failed to abov_parse_dt\n");
			ret = -ENOMEM;
			goto err_config;
		}

		info->pdata = pdata;
	} else
		info->pdata = client->dev.platform_data;

	if (info->pdata == NULL) {
		input_err(true, &client->dev, "failed to get platform data\n");
		goto err_config;
	}
#if 1
	/* Get pinctrl if target uses pinctrl */
		info->pinctrl = devm_pinctrl_get(&client->dev);
		if (IS_ERR(info->pinctrl)) {
			if (PTR_ERR(info->pinctrl) == -EPROBE_DEFER)
				goto err_config;

			input_err(true, &client->dev, "%s: Target does not use pinctrl\n", __func__);
			info->pinctrl = NULL;
		}

		if (info->pinctrl) {
			ret = abov_pinctrl_configure(info, true);
			if (ret)
				input_err(true, &client->dev,
					"%s: cannot set ts pinctrl active state\n", __func__);
		}

		/* sub-det pinctrl */
		if (gpio_is_valid(info->pdata->sub_det)) {
			info->pinctrl_det = devm_pinctrl_get(&client->dev);
			if (IS_ERR(info->pinctrl_det)) {
				input_err(true, &client->dev, "%s: Failed to get pinctrl\n", __func__);
				goto err_config;
			}

			info->pins_default = pinctrl_lookup_state(info->pinctrl_det, "sub_det");
			if (IS_ERR(info->pins_default)) {
				input_err(true, &client->dev, "%s: Failed to get pinctrl state\n", __func__);
				devm_pinctrl_put(info->pinctrl_det);
				goto err_config;
			}

			ret = pinctrl_select_state(info->pinctrl_det, info->pins_default);
			if (ret < 0)
				input_err(true, &client->dev, "%s: Failed to configure sub_det pin\n", __func__);
		}
#endif
	ret = abov_gpio_reg_init(&client->dev, info->pdata);
	if(ret){
		input_err(true, &client->dev, "failed to init reg\n");
		goto pwr_config;
	}
	if (info->pdata->power)
		info->pdata->power(info, true);

	if(!info->pdata->boot_on_ldo)
		abov_delay(ABOV_RESET_DELAY);

	if (gpio_is_valid(info->pdata->sub_det)) {
		ret = gpio_get_value(info->pdata->sub_det);
		if (ret) {
			input_err(true, &client->dev, "Device wasn't connected to board \n");
			ret = -ENODEV;
			goto err_i2c_check;
		}
	}

	info->enabled = true;
	info->irq = -1;
	client->irq = gpio_to_irq(info->pdata->gpio_int);

	mutex_init(&info->lock);

	info->input_event = info->pdata->input_event;
	info->touchkey_count = sizeof(touchkey_keycode) / sizeof(int);
	i2c_set_clientdata(client, info);

	ret = abov_tk_fw_check(info);
	if (ret) {
		input_err(true, &client->dev,
			"failed to firmware check (%d)\n", ret);
		goto err_reg_input_dev;
	}

	ret = get_tk_fw_version(info, false);
	if (ret < 0) {
		input_err(true, &client->dev, "%s read fail\n", __func__);
		goto err_reg_input_dev;
	}

	snprintf(info->phys, sizeof(info->phys),
		 "%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchkey";
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &client->dev;
#if 1 //def CONFIG_INPUT_ENABLED
	input_dev->open = abov_tk_input_open;
	input_dev->close = abov_tk_input_close;
#endif
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(KEY_RECENT, input_dev->keybit);
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_CP_GRIP, input_dev->keybit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	input_set_drvdata(input_dev, info);

	ret = input_register_device(input_dev);
	if (ret) {
		input_err(true, &client->dev, "failed to register input dev (%d)\n",
			ret);
		goto err_reg_input_dev;
	}

	if (!info->pdata->irq_flag) {
		input_err(true, &client->dev, "no irq_flag\n");
		ret = request_threaded_irq(client->irq, NULL, abov_tk_interrupt,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT, ABOV_TK_NAME, info);
	} else {
		ret = request_threaded_irq(client->irq, NULL, abov_tk_interrupt,
			info->pdata->irq_flag, ABOV_TK_NAME, info);
	}
	if (ret < 0) {
		input_err(true, &client->dev, "Failed to register interrupt\n");
		goto err_req_irq;
	}
	info->irq = client->irq;

#ifdef CONFIG_HAS_EARLYSUSPEND
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING;
	info->early_suspend.suspend = abov_tk_early_suspend;
	info->early_suspend.resume = abov_tk_late_resume;
	register_early_suspend(&info->early_suspend);
#endif

	info->dev = sec_device_create(info, "sec_touchkey");
	if (IS_ERR(info->dev))
		input_err(true, &client->dev,
		"Failed to create device for the touchkey sysfs\n");

	ret = sysfs_create_group(&info->dev ->kobj,
		&sec_touchkey_attr_group);
	if (ret)
		input_err(true, &client->dev, "Failed to create sysfs group\n");

	ret = sysfs_create_link(&info->dev ->kobj,
		&info->input_dev->dev.kobj, "input");
	if (ret < 0) {
		input_err(true, &client->dev,
			"%s: Failed to create input symbolic link\n",
			__func__);
	}


#ifdef LED_TWINKLE_BOOTING
	if (get_samsung_lcd_attached() == 0) {
		input_err(true, &client->dev,
			"%s : get_samsung_lcd_attached()=0, so start LED twinkle \n", __func__);

		INIT_DELAYED_WORK(&info->led_twinkle_work, led_twinkle_work);
		info->led_twinkle_check =  1;

		schedule_delayed_work(&info->led_twinkle_work, msecs_to_jiffies(400));
	}
#endif
#ifdef CONFIG_TOUCHKEY_GRIP
	info->sar_sensing = 1;
	device_init_wakeup(&client->dev, true);
#endif

	input_err(true, &client->dev, "%s done\n", __func__);

#ifdef CONFIG_TOUCHKEY_GRIP
	if (lpcharge == 1) {
		disable_irq(info->irq);
		input_err(true, &client->dev, "%s disable_irq\n", __func__);
		abov_sar_only_mode(info, 1);
	}
#endif

#ifdef CONFIG_VBUS_NOTIFIER
	if (info->pdata->ta_notifier) {
		vbus_notifier_register(&info->vbus_nb, abov_touchkey_vbus_notification,
					VBUS_NOTIFY_DEV_CHARGER);
	}
#endif

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	INIT_DELAYED_WORK(&info->efs_open_work, touchkey_efs_open_work);

	info->light_table_crc = info->pdata->dt_light_version;
	sprintf(info->light_version_full_bin, "T%d.", info->pdata->dt_light_version);
	for (i = 0; i < info->pdata->dt_light_table; i++) {
		info->light_table_crc += tkey_light_reg_table[i].octa_id;
		info->light_table_crc += tkey_light_reg_table[i].led_reg;
		snprintf(tmp, 2, "%X", tkey_light_reg_table[i].octa_id);
		strncat(info->light_version_full_bin, tmp, 1);
	}
	input_info(true, &client->dev, "%s: light version of kernel : %s\n",
			__func__, info->light_version_full_bin);

	schedule_delayed_work(&info->efs_open_work, msecs_to_jiffies(2000));
#endif

	return 0;

err_req_irq:
	input_unregister_device(input_dev);
err_reg_input_dev:
	mutex_destroy(&info->lock);
	gpio_free(info->pdata->gpio_int);
err_i2c_check:
	if (info->pdata->power)
		info->pdata->power(info, false);
pwr_config:
err_config:
#ifdef CONFIG_TOUCHKEY_GRIP
	wake_lock_destroy(&info->touckey_wake_lock);
#endif
	input_free_device(input_dev);
err_input_alloc:
	kfree(info);
err_alloc:
	input_err(true, &client->dev, "%s fail\n",__func__);
	return ret;

}


#ifdef LED_TWINKLE_BOOTING
static void led_twinkle_work(struct work_struct *work)
{
	struct abov_tk_info *info = container_of(work, struct abov_tk_info,
						led_twinkle_work.work);
	static bool led_on = 1;
	static int count = 0;
	input_info(true, &info->client->dev, "%s, on=%d, c=%d\n",__func__, led_on, count++ );

	if(info->led_twinkle_check == 1){

		touchkey_led_set(info,led_on);
		if(led_on)	led_on = 0;
		else		led_on = 1;

		schedule_delayed_work(&info->led_twinkle_work, msecs_to_jiffies(400));
	}else{

		if(led_on == 0)
			touchkey_led_set(info, 0);
	}

}
#endif

static int abov_tk_remove(struct i2c_client *client)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);

/*	if (info->enabled)
		info->pdata->power(0);
*/
	info->enabled = false;
#ifdef CONFIG_TOUCHKEY_GRIP
	device_init_wakeup(&client->dev, false);
	wake_lock_destroy(&info->touckey_wake_lock);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&info->early_suspend);
#endif
	if (info->irq >= 0)
		free_irq(info->irq, info);
	input_unregister_device(info->input_dev);
	input_free_device(info->input_dev);
	kfree(info);

	return 0;
}

static void abov_tk_shutdown(struct i2c_client *client)
{
	struct abov_tk_info *info = i2c_get_clientdata(client);
	u8 cmd = CMD_LED_OFF;
	input_info(true, &client->dev, "Inside abov_tk_shutdown \n");

	if (info->enabled){
		disable_irq(info->irq);
	abov_tk_i2c_write(client, ABOV_BTNSTATUS, &cmd, 1);
		info->pdata->power(info, false);
	}
	info->enabled = false;
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	cancel_delayed_work(&info->efs_open_work);
#endif
// just power off.
//	if (info->irq >= 0)
//		free_irq(info->irq, info);
//	kfree(info);
}

#if defined(CONFIG_PM) && !defined(CONFIG_TOUCHKEY_GRIP)
static int abov_tk_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct abov_tk_info *info = i2c_get_clientdata(client);

	if (!info->enabled) {
		input_info(true, &client->dev, "%s: already power off\n", __func__);
		return 0;
	}

	input_info(true, &client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);

	disable_irq(info->irq);
	info->enabled = false;
	release_all_fingers(info);

	if (info->pdata->power)
		info->pdata->power(info, false);
	return 0;
}

static int abov_tk_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct abov_tk_info *info = i2c_get_clientdata(client);
	u8 led_data;

	if (info->enabled) {
		input_info(true, &client->dev, "%s: already power on\n", __func__);
		return 0;
	}

	input_info(true, &info->client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);

	if (info->pdata->power) {
		info->pdata->power(info, true);
		abov_delay(ABOV_RESET_DELAY);
	} else /* touchkey on by i2c */
		get_tk_fw_version(info, true);

	info->enabled = true;

	if (abov_touchled_cmd_reserved && \
		abov_touchkey_led_status == CMD_LED_ON) {
		abov_touchled_cmd_reserved = 0;
		led_data=abov_touchkey_led_status;

		abov_tk_i2c_write(client, ABOV_BTNSTATUS, &led_data, 1);

		input_info(true, &info->client->dev, "%s: LED reserved on\n", __func__);
	}
	enable_irq(info->irq);

	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void abov_tk_early_suspend(struct early_suspend *h)
{
	struct abov_tk_info *info;
	info = container_of(h, struct abov_tk_info, early_suspend);
	abov_tk_suspend(&info->client->dev);

}

static void abov_tk_late_resume(struct early_suspend *h)
{
	struct abov_tk_info *info;
	info = container_of(h, struct abov_tk_info, early_suspend);
	abov_tk_resume(&info->client->dev);
}
#endif

#if 1//def CONFIG_INPUT_ENABLED
static int abov_tk_input_open(struct input_dev *dev)
{
	struct abov_tk_info *info = input_get_drvdata(dev);

	input_info(true, &info->client->dev, "%s: users=%d, v:0x%02x, g(%d), f(%d), k(%d)\n", __func__,
		info->input_dev->users, info->fw_ver, info->flip_mode, info->glovemode, info->keyboard_mode);
#ifdef CONFIG_TOUCHKEY_GRIP
	if (lpcharge == 1) {
		input_info(true, &info->client->dev, "%s(lpcharge): sar_enable(%d)\n", __func__, info->sar_enable);
		return 0;
	}

	input_info(true, &info->client->dev, "%s: sar_enable(%d)\n", __func__, info->sar_enable);

	/* abov_led_power(info, true); */

	if (info->flip_mode)
		abov_sar_only_mode(info, 1);
	else
		abov_sar_only_mode(info, 0);

	if (device_may_wakeup(&info->client->dev))
		disable_irq_wake(info->irq );
#else
	abov_tk_resume(&info->client->dev);
	if (info->pinctrl)
		abov_pinctrl_configure(info, true);

#ifdef CONFIG_VBUS_NOTIFIER
	if (info->pdata->ta_notifier && g_ta_connected) {
		abov_set_ta_status(info);
	}
#endif

	if (info->flip_mode){
		abov_mode_enable(info->client, ABOV_FLIP, CMD_FLIP_ON);
	} else {
		if (info->glovemode)
			abov_mode_enable(info->client, ABOV_GLOVE, CMD_GLOVE_ON);
	}
	if (info->keyboard_mode)
		abov_mode_enable(info->client, ABOV_KEYBOARD, CMD_MOBILE_KBD_ON);
#endif
	return 0;
}
static void abov_tk_input_close(struct input_dev *dev)
{
	struct abov_tk_info *info = input_get_drvdata(dev);

	input_info(true, &info->client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);
#ifdef CONFIG_TOUCHKEY_GRIP
	input_info(true, &info->client->dev, "%s: sar_enable(%d)\n", __func__, info->sar_enable);
	abov_sar_only_mode(info, 1);

	if (device_may_wakeup(&info->client->dev))
		enable_irq_wake(info->irq );

	/* abov_led_power(info, false); */

#else
	abov_tk_suspend(&info->client->dev);
	if (info->pinctrl)
		abov_pinctrl_configure(info, false);
#endif

#ifdef LED_TWINKLE_BOOTING
	info->led_twinkle_check = 0;
#endif

}
#endif

#if 0//defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) &&!defined(CONFIG_INPUT_ENABLED)
static const struct dev_pm_ops abov_tk_pm_ops = {
	.suspend = abov_tk_suspend,
	.resume = abov_tk_resume,
};
#endif

static const struct i2c_device_id abov_tk_id[] = {
	{ABOV_TK_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, abov_tk_id);

#ifdef CONFIG_OF
static struct of_device_id abov_match_table[] = {
	{ .compatible = "abov,mc96ft18xx",},
	{ },
};
#else
#define abov_match_table NULL
#endif

static struct i2c_driver abov_tk_driver = {
	.probe = abov_tk_probe,
	.remove = abov_tk_remove,
	.shutdown = abov_tk_shutdown,
	.driver = {
		   .name = ABOV_TK_NAME,
		   .of_match_table = abov_match_table,
#if 0//defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) &&!defined(CONFIG_INPUT_ENABLED)
		   .pm = &abov_tk_pm_ops,
#endif
	},
	.id_table = abov_tk_id,
};

static int __init touchkey_init(void)
{
#if defined(CONFIG_BATTERY_SAMSUNG) && !defined(CONFIG_TOUCHKEY_GRIP)
	if (lpcharge == 1) {
			pr_notice("%s : Do not load driver due to : lpm %d\n",
			 __func__, lpcharge);
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
MODULE_DESCRIPTION("Touchkey driver for Abov MF18xx chip");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /* tc300k.c -- Linux driver for coreriver chip as touchkey
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Author: Junkyeong Kim <jk0430.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/i2c/tc300k.h>
//#include <plat/gpio-cfg.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#ifdef CONFIG_INPUT_BOOSTER
#include <linux/input/input_booster.h>
#endif
#include <linux/regulator/consumer.h>
#include <linux/sec_sysfs.h>
#ifdef CONFIG_BATTERY_SAMSUNG
#include <linux/sec_batt.h>
#endif

#ifdef CONFIG_TOUCHKEY_GRIP
#define FEATURE_GRIP_FOR_SAR
#endif

#if defined (CONFIG_VBUS_NOTIFIER) && defined(FEATURE_GRIP_FOR_SAR)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/vbus_notifier.h>
#endif

/* TSK IC */
#define TC300K_TSK_IC	0x00
#define TC350K_TSK_IC	0x01

/* registers */
#define TC300K_KEYCODE		0x00
#define TC300K_FWVER		0x01
#define TC300K_MDVER		0x02
#define TC300K_MODE			0x03
#define TC300K_CHECKS_H		0x04
#define TC300K_CHECKS_L		0x05
#define TC300K_THRES_H		0x06
#define TC300K_THRES_L		0x07
#define TC300K_1KEY_DATA	0x08
#define TC300K_2KEY_DATA	0x0E
#define TC300K_3KEY_DATA	0x14
#define TC300K_4KEY_DATA	0x1A
#define TC300K_5KEY_DATA	0x20
#define TC300K_6KEY_DATA	0x26

#define TC300K_CH_PCK_H_OFFSET	0x00
#define TC300K_CH_PCK_L_OFFSET	0x01
#define TC300K_DIFF_H_OFFSET	0x02
#define TC300K_DIFF_L_OFFSET	0x03
#define TC300K_RAW_H_OFFSET		0x04
#define TC300K_RAW_L_OFFSET		0x05

/* registers for tabs2(tc350k) */
#define TC350K_1KEY		0x10	// recent inner
#define TC350K_2KEY		0x18	// back inner
#define TC350K_3KEY		0x20	// recent outer
#define TC350K_4KEY		0x28	// back outer

#ifdef FEATURE_GRIP_FOR_SAR
/* registers for grip sensor */
#define TC305K_GRIPCODE			0x0F
#define TC305K_GRIP_THD_PRESS		0x00
#define TC305K_GRIP_THD_RELEASE		0x02
#define TC305K_GRIP_THD_NOISE		0x04
#define TC305K_GRIP_CH_PERCENT		0x06
#define TC305K_GRIP_DIFF_DATA		0x08
#