/* abov_touchkey.c -- Linux driver for abov chip as touchkey
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
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/i2c/abov_touchkey_ft1804.h>
#include <linux/io.h>
#include <asm/unaligned.h>
#include <linux/regulator/consumer.h>
#include <linux/sec_sysfs.h>
#include <linux/wakelock.h>

#include <linux/pinctrl/consumer.h>

#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_BATTERY_SAMSUNG
#include <linux/sec_batt.h>
#endif

#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/vbus_notifier.h>
#endif

/* registers */
#define ABOV_BTNSTATUS		0x00
#define ABOV_FW_VER			0x01
//#define ABOV_PCB_VER		0x02
//#define ABOV_COMMAND		0x03
#define ABOV_THRESHOLD		0x02
//#define ABOV_SENS			0x05
//#define ABOV_SETIDAC		0x06
#define ABOV_BTNSTATUS_NEW	0x07
#define ABOV_LED_RECENT		0x08		//LED Dimming (0x01~0x1F)
#define ABOV_LED_BACK		0x09		//LED Dimming (0x01~0x1F)
#define ABOV_DIFFDATA		0x0A
#define ABOV_RAWDATA		0x0E
#define ABOV_VENDORID		0x12
#define ABOV_TSPTA			0x13
#define ABOV_GLOVE			0x13
#define ABOV_KEYBOARD		0x13
#define ABOV_MODEL_NUMBER	0x14		//Model No.
#define ABOV_FLIP			0x15
#define ABOV_SW_RESET		0x1A

/* command */
#define CMD_LED_ON			0x10
#define CMD_LED_OFF			0x20

#define CMD_SAR_TOTALCAP	0x16
#define CMD_SAR_MODE		0x17
#define CMD_SAR_TOTALCAP_READ		0x18
#define CMD_SAR_ENABLE		0x24
#define CMD_SAR_SENSING		0x25
#define CMD_SAR_NOISE_THRESHOLD	0x26
#define CMD_SAR_BASELINE	0x28
#define CMD_SAR_DIFFDATA	0x2A
#define CMD_SAR_RAWDATA		0x2E
#define CMD_SAR_THRESHOLD	0x32

#define CMD_DATA_UPDATE		0x40
#define CMD_MODE_CHECK		0x41
#define CMD_LED_CTRL_ON		0x60
#define CMD_LED_CTRL_OFF	0x70
#define CMD_STOP_MODE		0x80
#define CMD_GLOVE_OFF		0x10
#define CMD_GLOVE_ON		0x20
#define CMD_MOBILE_KBD_OFF	0x10
#define CMD_MOBILE_KBD_ON	0x20
#define CMD_FLIP_OFF		0x10
#define CMD_FLIP_ON			0x20
#define CMD_OFF			0x10
#define CMD_ON			0x20

#define ABOV_BOOT_DELAY		45
#define ABOV_RESET_DELAY	150

#ifdef CONFIG_KEYBOARD_ABOV_TOUCH_T316
#define ABOV_FLASH_MODE		0x31
#else
#define ABOV_FLASH_MODE		0x18
#endif

//static struct device *sec_touchkey;

#ifdef LED_TWINKLE_BOOTING
static void led_twinkle_work(struct work_struct *work);
#endif

#define TK_FW_PATH_BIN "abov/abov_noble.fw"
#define TK_FW_PATH_SDCARD "/sdcard/abov_fw.bin"

#define I2C_M_WR 0		/* for i2c */

enum {
	BUILT_IN = 0,
	SDCARD,
};

#define ABOV_ISP_FIRMUP_ROUTINE	0

extern unsigned int system_rev;
extern struct class *sec_class;
#ifdef CONFIG_VBUS_NOTIFIER
static bool g_ta_connected =0;
#endif
static int touchkey_keycode[] = { 0,
	KEY_RECENT, KEY_BACK
#ifdef CONFIG_TOUCHKEY_GRIP
	, KEY_CP_GRIP
#endif
};

struct abov_tk_info {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct device *dev;
	struct abov_touchkey_platform_data *pdata;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct mutex l