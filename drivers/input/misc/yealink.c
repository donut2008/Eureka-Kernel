 &client->dev, "%s : detach\n",__func__);
		// if sar_mode is on => must send wake-up command
		if (tkey_data->sar_mode)
			ret = tc300k_wake_up(client, TC300K_CMD_WAKE_UP);

		ret = tc300k_mode_enable(client, TC300K_CMD_TA_OFF);
		if (ret < 0)
			input_err(true, &client->dev, "%s TA mode OFF fail(%d)\n", __func__, ret);
		break;
	default:
		break;
	}

	return 0;
}

#endif

static int tc300k_connecter_check(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;

	if (data->pdata->gpio_sub_det == 0) {
		input_err(true, &client->dev, "%s: Not use sub_det pin\n", __func__);
		return SUB_DET_DISABLE;

	} else {
		if (gpio_get_value(data->pdata->gpio_sub_det)) {
			return SUB_DET_ENABLE_CON_OFF;
		} else {
			return SUB_DET_ENABLE_CON_ON;
		}

	}

}
static int tc300k_fw_check(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int ret;
	int tsk_connecter_status;

	tsk_connecter_status = tc300k_connecter_check(data);

	if (tsk_connecter_status == SUB_DET_ENABLE_CON_OFF) {
		input_err(true, &client->dev, "%s : TSK IC is disconnected! skip probe(%d)\n",
						__func__, gpio_get_value(data->pdata->gpio_sub_det));
		return -1;
	}

	ret = tc300k_get_fw_version(data, true);

	if (ret < 0) {
		if ((tsk_connecter_status == SUB_DET_ENABLE_CON_ON)||(tsk_connecter_status == SUB_DET_DISABLE)) {
			input_err(true, &client->dev, 
				"%s: i2c fail, But TSK IC is connected!\n", __func__);
			data->fw_ver = 0xFF;
		} else {
			input_err(true, &client->dev,
				"%s: i2c fail...[%d], addr[%d]\n",
				__func__, ret, data->client->addr);
			input_err(true, &client->dev, 
				"%s: touchkey driver unload\n", __func__);
			return ret;
		}
	}


	if (data->fw_ver == 0xFF) {
		input_info(true, &client->dev,
			"fw version 0xFF, Excute firmware update!\n");
		ret = tc300k_fw_update(data, FW_INKERNEL, true);
		if (ret)
			return -1;
	} else {
		ret = tc300k_fw_update(data, FW_INKERNEL, false);
		if (ret)
			return -1;
	}

	return 0;
}

static int tc300_pinctrl_init(struct tc300k_data *data)
{
	struct device *dev = &data->client->dev;
	int i;
	input_info(true, &data->client->dev, "%s\n",__func__);
	// IRQ
	data->pinctrl_irq = devm_pinctrl_get(dev);
	if (IS_ERR(data->pinctrl_irq)) {
		input_dbg(true, &data->client->dev, "%s: Failed to get irq pinctrl\n", __func__);
		data->pinctrl_irq = NULL;
		goto i2c_pinctrl_get;
	}
	for (i = 0; i < 2; ++i) {
		data->pin_state[i] = pinctrl_lookup_state(data->pinctrl_irq, str_states[i]);
		if (IS_ERR(data->pin_state[i])) {
			input_dbg(true, &data->client->dev, "%s: Failed to get irq pinctrl state\n", __func__);
			devm_pinctrl_put(data->pinctrl_irq);
			data->pinctrl_irq = NULL;
			goto i2c_pinctrl_get;
		}
	}

i2c_pinctrl_get:
	/* for h/w i2c */
	dev = data->client->dev.parent->parent;
	input_info(true, &data->client->dev, "%s: use dev's parent\n", __func__);

	// I2C
	data->pinctrl_i2c = devm_pinctrl_get(dev);
	if (IS_ERR(data->pinctrl_i2c)) {
		input_err(true, &data->client->dev, "%s: Failed to get i2c pinctrl\n", __func__);
		goto err_pinctrl_get_i2c;
	}
	for (i = 2; i < 4; ++i) {
		data->pin_state[i] = pinctrl_lookup_state(data->pinctrl_i2c, str_states[i]);
		if (IS_ERR(data->pin_state[i])) {
			input_err(true, &data->client->dev, "%s: Failed to get i2c pinctrl state\n", __func__);
			goto err_pinctrl_get_state_i2c;
		}
	}
	return 0;

err_pinctrl_get_state_i2c:
	devm_pinctrl_put(data->pinctrl_i2c);
err_pinctrl_get_i2c:
	return -ENODEV;
}

static int tc300_pinctrl(struct tc300k_data *data, int state)
{
	struct pinctrl *pinctrl_i2c = data->pinctrl_i2c;
	struct pinctrl *pinctrl_irq = data->pinctrl_irq;
	int ret=0;

	switch (state) {
		case I_STATE_ON_IRQ:
		case I_STATE_OFF_IRQ:
			if (pinctrl_irq)
				ret = pinctrl_select_state(pinctrl_irq, data->pin_state[state]);
			break;
		case I_STATE_ON_I2C:
		case I_STATE_OFF_I2C:
			if (pinctrl_i2c)
				ret = pinctrl_select_state(pinctrl_i2c, data->pin_state[state]);
			break;
	}
	if (ret < 0) {
		input_err(true, &data->client->dev, 
		"%s: Failed to configure tc300_pinctrl state[%d]\n", __func__, state);
		return ret;
	}

	return 0;
}

static void tc300_config_gpio_i2c(struct tc300k_data *data, int onoff)
{
	input_err(true, &data->client->dev, "%s\n",__func__);

	tc300_pinctrl(data, onoff ? I_STATE_ON_I2C : I_STATE_OFF_I2C);
	mdelay(100);
}

static int tc300k_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct tc300k_platform_data *pdata;
	struct input_dev *input_dev;
	struct tc300k_data *data;
	int ret=0;
	int i=0;
	int err=0;
	input_dbg(true, &client->dev, "%s\n",__func__);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		input_err(true, &client->dev, "%s : Do not load driver due to : lpm %d\n",
			 __func__, lpcharge);
		return -ENODEV;
	}
#endif

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev,
			"i2c_check_functionality fail\n");
		return -EIO;
	}

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct tc300k_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			input_err(true, &client->dev, "Failed to allocate memory\n");
			goto err_alloc_data;
		}

		err = tc300k_parse_dt(&client->dev, pdata);
		if (err)
			goto err_alloc_data;
	}else
		pdata = client->dev.platform_data;

	data = kzalloc(sizeof(struct tc300k_data), GFP_KERNEL);
	if (!data) {
		input_err(true, &client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc_data;
	}

	data->pdata = pdata;

	input_dev = input_allocate_device();
	if (!input_dev) {
		input_err(true, &client->dev,
			"Failed to allocate memory for input device\n");
		ret = -ENOMEM;
		goto err_alloc_input;
	}

	data->client = client;
	data->input_dev = input_dev;

	if (data->pdata == NULL) {
		input_err(true, &client->dev, "failed to get platform data\n");
		ret = -EINVAL;
		goto err_platform_data;
	}
	data->irq = -1;
	mutex_init(&data->lock);
	mutex_init(&data->lock_fac);

#ifdef FEATURE_GRIP_FOR_SAR
	wake_lock_init(&data->touchkey_wake_lock, WAKE_LOCK_SUSPEND, "touchkey wake lock");
	INIT_DELAYED_WORK(&data->debug_work, tc300k_debug_work_func);
#endif

	pdata->power = tc300k_touchkey_power;
	pdata->keyled = tc300k_touchkey_led_control;

	i2c_set_clientdata(client, data);
	tc300k_gpio_request(data);

	ret = tc300_pinctrl_init(data);
	if (ret < 0) {
		input_err(true, &client->dev,
			"%s: Failed to init pinctrl: %d\n", __func__, ret);
		goto err_platform_data;
	}

	if(pdata->boot_on_ldo){
		data->pdata->power(data, true);
		data->pdata->keyled(data, true);
	} else {
		data->pdata->power(data, true);
		data->pdata->keyled(data, true);
		msleep(200);
	}
	data->enabled = true;

	client->irq=gpio_to_irq(pdata->gpio_int);

	ret = tc300k_fw_check(data);
	if (ret) {
		input_err(true, &client->dev,
			"failed to firmware check(%d)\n", ret);
		goto err_fw_check;
	}

	snprintf(data->phys, sizeof(data->phys),
		"%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchkey";
	input_dev->phys = data->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->open = tc300k_input_open;
	input_dev->close = tc300k_input_close;

	if (pdata->use_bitmap) {
		data->tsk_ev_val = tsk_ev;
		data->key_num = ARRAY_SIZE(tsk_ev)/2;
	} else {
		data->tsk_ev_val = tsk_ev_old;
		data->key_num = ARRAY_SIZE(tsk_ev_old)/2;
	}
	input_info(true, &client->dev, "number of keys = %d\n", data->key_num);
#ifdef FEATURE_GRIP_FOR_SAR
	data->grip_ev_val = grip_ev;
	data->grip_num = ARRAY_SIZE(grip_ev)/2;
	input_info(true, &client->dev, "number of grips = %d\n", data->grip_num);
#endif

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	for (i = 0; i < data->key_num; i++) {
		set_bit(data->tsk_ev_val[i].tsk_keycode, input_dev->keybit);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		input_info(true, &client->dev, "keycode[%d]= %3d\n",
						i, data->tsk_ev_val[i].tsk_keycode);
#endif
	}
#ifdef FEATURE_GRIP_FOR_SAR
	for (i = 0; i < data->grip_num; i++) {
		set_bit(data->grip_ev_val[i].grip_code, input_dev->keybit);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		input_info(true, &client->dev, "gripcode[%d]= %3d\n",
						i, data->grip_ev_val[i].grip_code);
#endif
	}
#endif
	input_set_drvdata(input_dev, data);

	ret = input_register_device(input_dev);
	if (ret) {
		input_err(true, &client->dev, "fail to register input_dev (%d)\n",
			ret);
		goto err_register_input_dev;
	}

	ret = request_threaded_irq(client->irq, NULL, tc300k_interrupt,
				IRQF_DISABLED | IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT, TC300K_NAME, data);
	if (ret < 0) {
		input_err(true, &client->dev, "fail to request irq (%d).\n",
			pdata->gpio_int);
		goto err_request_irq;
	}
	data->irq = pdata->gpio_int;

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = tc300k_early_suspend;
	data->early_suspend.resume = tc300k_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	data->sec_touchkey = sec_device_create(client, "sec_touchkey");
	if (IS_ERR(data->sec_touchkey))
		input_err(true, &client->dev,
			"Failed to create device for the touchkey sysfs\n");

	if (data->pdata->tsk_ic_num == TC350K_TSK_IC) {
		ret = sysfs_create_group(&data->sec_touchkey->kobj,
			&sec_touchkey_attr_group_350k);
	} else {
		ret = sysfs_create_group(&data->sec_touchkey->kobj,
			&sec_touchkey_attr_group);
	}

	if (ret)
		input_err(true, &client->dev, "Failed to create sysfs group\n");


	ret = sysfs_create_link(&data->sec_touchkey->kobj, &input_dev->dev.kobj, "input");
		if (ret < 0) {
			input_err(true, &client->dev,
					"%s: Failed to create input symbolic link\n",
					__func__);
		}

	dev_set_drvdata(data->sec_touchkey, data);

#if defined (CONFIG_VBUS_NOTIFIER) && defined(FEATURE_GRIP_FOR_SAR)
	vbus_notifier_register(&data->vbus_nb, tkey_vbus_notification,
			       VBUS_NOTIFY_DEV_CHARGER);
#endif

#ifdef FEATURE_GRIP_FOR_SAR
	ret = tc300k_mode_check(client);
	if (ret >= 0) {
		data->sar_enable = !!(ret & TC300K_MODE_SAR);
		input_info(true, &client->dev, "%s: mode %d, sar %d\n",
				__func__, ret, data->sar_enable);
	}
	device_init_wakeup(&client->dev, true);
#endif
	input_info(true, &client->dev, "%s done\n", __func__);
	return 0;

err_request_irq:
	input_unregister_device(input_dev);
	input_dev = NULL;
err_register_input_dev:
err_fw_check:
	mutex_destroy(&data->lock);
	mutex_destroy(&data->lock_fac);
	data->pdata->keyled(data, false);
	data->pdata->power(data, false);
err_platform_data:
#ifdef FEATURE_GRIP_FOR_SAR
	wake_lock_destroy(&data->touchkey_wake_lock);
#endif
	input_free_device(input_dev);
err_alloc_input:
	kfree(data);
err_alloc_data:
	return ret;
}

static int tc300k_remove(struct i2c_client *client)
{
	struct tc300k_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
#ifdef FEATURE_GRIP_FOR_SAR
	device_init_wakeup(&client->dev, false);
	wake_lock_destroy(&data->touchkey_wake_lock);
	cancel_delayed_work_sync(&data->debug_work);
	flush_delayed_work(&data->debug_work);
#endif
	free_irq(client->irq, data);
	input_unregister_device(data->input_dev);
	mutex_destroy(&data->lock);
	mutex_destroy(&data->lock_fac);
	data->pdata->keyled(data, false);
	data->pdata->power(data, false);
	gpio_free(data->pdata->gpio_int);
	gpio_free(data->pdata->gpio_sda);
	gpio_free(data->pdata->gpio_scl);
	kfree(data);

	return 0;
}

static void tc300k_shutdown(struct i2c_client *client)
{
	struct tc300k_data *data = i2c_get_clientdata(client);

	input_info(true, &client->dev, "%s\n", __func__);

#ifdef FEATURE_GRIP_FOR_SAR
	device_init_wakeup(&client->dev, false);
	wake_lock_destroy(&data->touchkey_wake_lock);
	cancel_delayed_work_sync(&data->debug_work);
	flush_delayed_work(&data->debug_work);
#endif
	disable_irq(client->irq);
	data->pdata->keyled(data, false);
	data->pdata->power(data, false);
}

#if defined(CONFIG_PM)
#ifndef FEATURE_GRIP_FOR_SAR
static int tc300k_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc300k_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);

	if (!data->enabled) {
		mutex_unlock(&data->lock);
		return 0;
	}

	input_info(true, &client->dev, "%s: users=%d\n",
		__func__, data->input_dev->users);

	disable_irq(client->irq);
	data->enabled = false;
	tc300k_release_all_fingers(data);
	data->pdata->keyled(data, false);
	data->pdata->power(data, false);
	data->led_on = false;

	mutex_unlock(&data->lock);

	return 0;
}

static int tc300k_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc300k_data *data = i2c_get_clientdata(client);
	int ret;
	u8 cmd;

	mutex_lock(&data->lock);
	if (data->enabled) {
		mutex_unlock(&data->lock);
		return 0;
	}
	input_info(true, &client->dev, "%s: users=%d\n", __func__, data->input_dev->users);

	data->pdata->power(data, true);
	data->pdata->keyled(data, true);
	msleep(200);
	enable_irq(client->irq);

	data->enabled = true;
	if (regulator_led){
		if (data->led_on == true) {
			data->led_on = false;
			input_info(true, &client->dev, "led on(resume)\n");
			cmd = TC300K_CMD_LED_ON;
			ret = i2c_smbus_write_byte_data(client, TC300K_CMD_ADDR, cmd);
			if (ret < 0)
				input_err(true, &client->dev, "%s led on fail(%d)\n", __func__, ret);
			else
				msleep(TC300K_CMD_DELAY);
		}
	}

	if (data->glove_mode) {
		ret = tc300k_mode_enable(client, TC300K_CMD_GLOVE_ON);
		if (ret < 0)
			input_err(true, &client->dev, "%s glovemode fail(%d)\n", __func__, ret);
	}

	if (data->flip_mode) {
		ret = tc300k_mode_enable(client, TC300K_CMD_FLIP_ON);
		if (ret < 0)
			input_err(true, &client->dev, "%s flipmode fail(%d)\n", __func__, ret);
	}

	mutex_unlock(&data->lock);

	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tc300k_early_suspend(struct early_suspend *h)
{
	struct tc300k_data *data;
	data = container_of(h, struct tc300k_data, early_suspend);
	tc300k_suspend(&data->client->dev);
}

static void tc300k_late_resume(struct early_suspend *h)
{
	struct tc300k_data *data;
	data = container_of(h, struct tc300k_data, early_suspend);
	tc300k_resume(&data->client->dev);
}
#endif

static void tc300k_input_close(struct input_dev *dev)
{
	struct tc300k_data *data = input_get_drvdata(dev);
#ifdef FEATURE_GRIP_FOR_SAR
	input_info(true, &data->client->dev, 
			"%s: sar_enable(%d)\n", __func__, data->sar_enable);
	tc300k_stop_mode(data, 1);

	if (device_may_wakeup(&data->client->dev))
		enable_irq_wake(data->client->irq);
#else
	input_info(true, &data->client->dev, "%s: users=%d\n", __func__,
		   data->input_dev->users);

	tc300k_suspend(&data->client->dev);
	tc300_pinctrl(data, I_STATE_OFF_IRQ);
#endif
}

static int tc300k_input_open(struct input_dev *dev)
{
	struct tc300k_data *data = input_get_drvdata(dev);

#ifdef FEATURE_GRIP_FOR_SAR
	input_info(true, &data->client->dev, 
			"%s: sar_enable(%d)\n", __func__, data->sar_enable);
	tc300k_stop_mode(data, 0);

	if (device_may_wakeup(&data->client->dev))
		disable_irq_wake(data->client->irq);
#else
	input_info(true, &data->client->dev, "%s: users=%d\n", __func__,
		   data->input_dev->users);

	tc300_pinctrl(data, I_STATE_ON_IRQ);
	tc300k_resume(&data->client->dev);
#endif

	return 0;
}
#endif /* CONFIG_PM */

#if 0
#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static const struct dev_pm_ops tc300k_pm_ops = {
	.suspend = tc300k_suspend,
	.resume = tc300k_resume,
};
#endif
#endif

static const struct i2c_device_id tc300k_id[] = {
	{TC300K_NAME, 0},
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id coreriver_match_table[] = {
	{ .compatible = "coreriver,tc300-keypad",},
	{ },
};
#else
#define coreriver_match_table	NULL
#endif
MODULE_DEVICE_TABLE(i2c, tc300k_id);

static struct i2c_driver tc300k_driver = {
	.probe = tc300k_probe,
	.remove = tc300k_remove,
	.shutdown = tc300k_shutdown,
	.driver = {
		.name = TC300K_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = coreriver_match_table,
#endif
#if 0
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
		.pm	= &tc300_pm_ops,
#endif
#endif
	},
	.id_table = tc300k_id,
};

static int __init tc300k_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&tc300k_driver);
	if (ret) {
		printk(KERN_ERR "[TK] coreriver touch keypad registration failed. ret= %d\n",
			ret);
	}
	printk(KERN_ERR "[TK] %s: init done %d\n", __func__, ret);

	return ret;
}

static void __exit tc300k_exit(void)
{
	i2c_del_driver(&tc300k_driver);
}

module_init(tc300k_init);
module_exit(tc300k_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Touchkey driver for Coreriver TC300K");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 *  Copyright (C) 2010,Imagis Technology Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/input/mt.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#endif

#include "ist40xx.h"
#include "ist40xx_misc.h"
#include "ist40xx_update.h"
#include "ist40xx_cmcs.h"

#if defined(CONFIG_VBUS_NOTIFIER) || defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/vbus_notifier.h>
#endif
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/manager/usb_typec_manager_notifier.h>
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
#include <linux/t-base-tui.h>
#endif
#ifdef CONFIG_SAMSUNG_TUI
#include "stui_inf.h"
#endif

struct ist40xx_data *ts_data;

#ifdef CONFIG_DISPLAY_SAMSUNG
extern int get_lcd_attached(char *mode);
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
struct ist40xx_data *tui_tsp_info;
extern int tui_force_close(uint32_t arg);
#endif

#ifdef CONFIG_SAMSUNG_TUI
struct ist40xx_data *stui_tsp_info;
#endif

int ist40xx_log_level = IST40XX_LOG_LEVEL;
void tsp_printk(int level, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (ist40xx_log_level < level)
		return;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk("%s %pV", IST40XX_LOG_TAG, &vaf);

	va_end(args);
}

long get_milli_second(struct ist40xx_data *data)
{
	ktime_get_ts(&data->t_current);

	return data->t_current.tv_sec * 1000 +
		data->t_current.tv_nsec / 1000000;
}

void ist40xx_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}

int ist40xx_intr_wait(struct ist40xx_data *data, long ms)
{
	long start_ms = get_milli_second(data);
	long curr_ms = 0;

	while (1) {
		if (!data->irq_working)
			break;

		curr_ms = get_milli_second(data);
		if ((curr_ms < 0) || (start_ms < 0)
			|| (curr_ms - start_ms > ms)) {
			input_info(true, &data->client->dev, "%s: timeout(%dms)\n",
				   __func__, ms);
			return -EPERM;
		}

		ist40xx_delay(2);
	}
	return 0;
}

void ist40xx_disable_irq(struct ist40xx_data *data)
{
	if (data->irq_enabled) {
		disable_irq(data->client->irq);
		data->irq_enabled = false;
		data->status.event_mode = false;
	}
}

void ist40xx_enable_irq(struct ist40xx_data *data)
{
	if (!data->irq_enabled) {
		enable_irq(data->client->irq);
		ist40xx_delay(10);
		data->irq_enabled = true;
		data->status.event_mode = true;
	}
}

void ist40xx_scheduled_reset(struct ist40xx_data *data)
{
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
		input_err(true, &data->client->dev, "%s return, TUI is enabled!\n",
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

	if (data->initialized)
		schedule_delayed_work(&data->work_reset_check, 0);
}

static void ist40xx_request_reset(struct ist40xx_data *data)
{
	data->irq_err_cnt++;
	if (data->irq_err_cnt >= data->max_irq_err_cnt) {
		input_info(true, &data->client->dev, "%s\n", __func__);
		ist40xx_scheduled_reset(data);
		data->irq_err_cnt = 0;
	}
}

void ist40xx_start(struct ist40xx_data *data)
{
	if (data->initialized) {
		data->scan_count = 0;
		data->scan_retry = 0;
		mod_timer(&data->event_timer,
				get_jiffies_64() + EVENT_TIMER_INTERVAL);
	}

	data->ignore_delay = true;

	/* TA mode */
	ist40xx_write_cmd(data, IST40XX_HIB_CMD,
			((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));

	if ((data->noise_mode >> NOISE_MODE_REJECTZONE) & 1) {
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_REJECTZONE_TOP << 16) | data->rejectzone_t));
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_REJECTZONE_BOTTOM << 16) | data->rejectzone_b));
		input_info(true, &data->client->dev, "%s: rejectzone T:%d, B:%d\n",
			   __func__, data->rejectzone_t, data->rejectzone_b);
	}

	if (data->report_rate >= 0) {
		ist40xx_write_cmd(data, IST40XX_HIB_CMD,
				((eHCOM_SET_TIME_ACTIVE << 16) | (data->report_rate & 0xFFFF)));
		input_info(true, &data->client->dev, "%s: active rate : %dus\n",
			   __func__, data->report_rate);
	}

	if (data->idle_rate >= 0) {
		ist40xx_w