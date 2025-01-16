/*
 * extcon-rt8973a.c - Richtek RT8973A extcon driver to support USB switches
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 * Author: Chanwoo Choi <cw00.choi@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/extcon.h>

#include "extcon-rt8973a.h"

#define	DELAY_MS_DEFAULT		20000	/* unit: millisecond */

struct muic_irq {
	unsigned int irq;
	const char *name;
	unsigned int virq;
};

struct reg_data {
	u8 reg;
	u8 mask;
	u8 val;
	bool invert;
};

struct rt8973a_muic_info {
	struct device *dev;
	struct extcon_dev *edev;

	struct i2c_client *i2c;
	struct regmap *regmap;

	struct regmap_irq_chip_data *irq_data;
	struct muic_irq *muic_irqs;
	unsigned int num_muic_irqs;
	int irq;
	bool irq_attach;
	bool irq_detach;
	bool irq_ovp;
	bool irq_otp;
	struct work_struct irq_work;

	struct reg_data *reg_data;
	unsigned int num_reg_data;
	bool auto_config;

	struct mutex mutex;

	/*
	 * Use delayed workqueue to detect cable state and then
	 * notify cable state to notifiee/platform through uevent.
	 * After completing the booting of platform, the extcon provider
	 * driver should notify cable state to upper layer.
	 */
	struct delayed_work wq_detcable;
};

/* Default value of RT8973A register to bring up MUIC device. */
static struct reg_data rt8973a_reg_data[] = {
	{
		.reg = RT8973A_REG_CONTROL1,
		.mask = RT8973A_REG_CONTROL1_ADC_EN_MASK
			| RT8973A_REG_CONTROL1_USB_CHD_EN_MASK
			| RT8973A_REG_CONTROL1_CHGTYP_MASK
			| RT8973A_REG_CONTROL1_SWITCH_OPEN_MASK
			| RT8973A_REG_CONTROL1_AUTO_CONFIG_MASK
			| RT8973A_REG_CONTROL1_INTM_MASK,
		.val = RT8973A_REG_CONTROL1_ADC_EN_MASK
			| RT8973A_REG_CONTROL1_USB_CHD_EN_MASK
			| RT8973A_REG_CONTROL1_CHGTYP_MASK,
		.invert = false,
	},
	{ /* sentinel */ }
};

/* List of detectable cables */
static const unsigned int rt8973a_extcon_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_CHG_USB_DCP,
	EXTCON_JIG,
	EXTCON_NONE,
};

/* Define OVP (Over Voltage Protection), OTP (Over Temperature Protection) */
enum rt8973a_event_type {
	RT8973A_EVENT_ATTACH = 1,
	RT8973A_EVENT_DETACH,
	RT8973A_EVENT_OVP,
	RT8973A_EVENT_OTP,
};

/* Define supported accessory type */
enum rt8973a_muic_acc_type {
	RT8973A_MUIC_ADC_OTG = 0x0,
	RT8973A_MUIC_ADC_AUDIO_SEND_END_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S1_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S2_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S3_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S4_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S5_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S6_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S7_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S8_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S9_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S10_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S11_BUTTON,
	RT8973A_MUIC_ADC_AUDIO_REMOTE_S12_BUTTON,
	RT8973A_MUIC_ADC_RESERVED_ACC_1,
	RT8973A_MUIC_ADC_RESERVED_ACC_2,
	RT8973A_MUIC_ADC_RESERVED_ACC_3,
	RT8973A_MUIC_ADC_RESERVED_ACC_4,
	RT8973A_MUIC_ADC_RESERVED_ACC_5,
	RT8973A_MUIC_ADC_AUDIO_TYPE2,
	RT8973A_MUIC_ADC_PHONE_POWERED_DEV,
	RT8973A_MUIC_ADC_UNKNOWN_ACC_1,
	RT8973A_MUIC_ADC_UNKNOWN_ACC_2,
	RT8973A_MUIC_ADC_TA,
	RT8973A_MUIC_ADC_FACTORY_MODE_BOOT_OFF_USB,
	RT8973A_MUIC_ADC_FACTORY_MODE_BOOT_ON_USB,
	RT8973A_MUIC_ADC_UNKNOWN_ACC_3,
	RT8973A_MUIC_ADC_UNKNOWN_ACC_4,
	RT8973A_MUIC_ADC_FACTORY_MODE_BOOT_OFF_UART,
	RT8973A_MUIC_ADC_FACTORY_MODE_BOOT_ON_UART,
	RT8973A_MUIC_ADC_UNKNOWN_ACC_5,
	RT8973A_MUIC_ADC_OPEN = 0x1f,

	/* The below accessories has same ADC value (0x1f).
	   So, Device type1 is used to separate specific accessory. */
					/* |---------|--ADC| */
					/* |    [7:5]|[4:0]| */
	RT8973A_MUIC_ADC_USB = 0x3f,	/* |      001|11111| */
};

/* List of supported interrupt for RT8973A */
static struct muic_irq rt8973a_muic_irqs[] = {
	{ RT8973A_INT1_ATTACH,		"muic-attach" },
	{ RT8973A_INT1_DETACH,		"muic-detach" },
	{ RT8973A_INT1_CHGDET,		"muic-chgdet" },
	{ RT8973A_INT1_DCD_T,		"muic-dcd-t" },
	{ RT8973A_INT1_OVP,		"muic-ovp" },
	{ RT8973A_INT1_CONNECT,		"muic-connect" },
	{ RT8973A_INT1_ADC_CHG,		"muic-adc-chg" },
	{ RT8973A_INT1_OTP,		"muic-otp" },
	{ RT8973A_INT2_UVLO,		"muic-uvlo" },
	{ RT8973A_INT2_POR,		"muic-por" },
	{ RT8973A_INT2_OTP_FET,		"muic-otp-fet" },
	{ RT8973A_INT2_OVP_FET,		"muic-ovp-fet" },
	{ RT8973A_INT2_OCP_LATCH,	"muic-ocp-latch" },
	{ RT8973A_INT2_OCP,		"muic-ocp" },
	{ RT8973A_INT2_OVP_OCP,		"muic-ovp-ocp" },
};

/* Define interrupt list of RT8973A to register regmap_irq */
static const struct regmap_irq rt8973a_irqs[] = {
	/* INT1 interrupts */
	{ .reg_offset = 0, .mask = RT8973A_INT1_ATTACH_MASK, },
	{ .reg_offset = 0, .mask = RT8973A_INT1_DETACH_MASK, },
	{ .reg_offset = 0, .mask = RT8973A_INT1_CHGDET_MASK, },
	{ .reg_offset = 0, .mask = RT8973A_INT1_DCD_T_MASK, },
	{ .reg_offset = 0, .mask = RT8973A_INT1_OVP_MASK, },
	{ .reg_offset = 0, .mask = RT8973A_INT1_CONNECT_MASK, },
	{ .reg_offset = 0, .mask = RT8973A_INT1_ADC_CHG_MASK, },
	{ .reg_offset = 0, .mask = RT8973A_INT1_OTP_MASK, },

	/* INT2 interrupts */
	{ .reg_offset = 1, .mask = RT8973A_INT2_UVLOT_MASK,},
	{ .reg_offset = 1, .mask = RT8973A_INT2_POR_MASK, },
	{ .reg_offset = 1, .mask = RT8973A_INT2_OTP_FET_MASK, },
	{ .reg_offset = 1, .mask = RT8973A_INT2_OVP_FET_MASK, },
	{ .reg_offset = 1, .mask = RT8973A_INT2_OCP_LATCH_MASK, },
	{ .reg_offset = 1, .mask = RT8973A_INT2_OCP_MASK, },
	{ .reg_offset = 1, .mask = RT8973A_INT2_OVP_OCP_MASK, },
};

static const struct regmap_irq_chip rt8973a_muic_irq_chip = {
	.name			= "rt8973a",
	.status_base		= RT8973A_REG_INT1,
	.mask_base		= RT8973A_REG_INTM1,
	.mask_invert		= false,
	.num_regs		= 2,
	.irqs			= rt8973a_irqs,
	.num_irqs		= ARRAY_SIZE(rt8973a_irqs),
};

/* Define regmap configuration of RT8973A for I2C communication  */
static bool rt8973a_muic_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case RT8973A_REG_INTM1:
	case RT8973A_REG_INTM2:
		return true;
	default:
		break;
	}
	return false;
}

static const struct regmap_config rt8973a_muic_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.volatile_reg	= rt8973a_muic_volatile_reg,
	.max_register	= RT8973A_REG_END,
};

/* Change DM_CON/DP_CON/VBUSIN switch according to cable type */
static int rt8973a_muic_set_path(struct rt8973a_muic_info *info,
				unsigned int con_sw, bool attached)
{
	int ret;

	/*
	 * Don't need to set h/w path according to cable type
	 * if Auto-configuration mode of CONTROL1 register is true.
	 */
	if (info->auto_config)
		return 0;

	if (!attached)
		con_sw	= DM_DP_SWITCH_UART;

	switch (con_sw) {
	case DM_DP_SWITCH_OPEN:
	case DM_DP_SWITCH_USB:
	case DM_DP_SWITCH_UART:
		ret = regmap_update_bits(info->regmap, RT8973A_REG_MANUAL_SW1,
					RT8973A_REG_MANUAL_SW1_DP_MASK |
					RT8973A_REG_MANUAL_SW1_DM_MASK,
					con_sw);
		if (ret < 0) {
			dev_err(info->dev,
				"cannot update DM_CON/DP_CON switch\n");
			return ret;
		}
		break;
	default:
		dev_err(info->dev, "Unknown DM_CON/DP_CON switch type (%d)\n",
				con_sw);
		return -EINVAL;
	}

	return 0;
}

static int rt8973a_muic_get_cable_type(struct rt8973a_muic_info *info)
{
	unsigned int adc, dev1;
	int ret, cable_type;

	/* Read ADC value according to external cable or button */
	ret = regmap_read(info->regmap, RT8973A_REG_ADC, &adc);
	if (ret) {
		dev_err(info->dev, "failed to read ADC register\n");
		return ret;
	}
	cable_type = adc & RT8973A_REG_ADC_MASK;

	/* Read Device 1 reigster to identify correct cable type */
	ret = regmap_read(info->regmap, RT8973A_REG_DEV1, &dev1);
	if (ret) {
		dev_err(info->dev, "failed to read DEV1 register\n");
		return ret;
	}

	switch (adc) {
	case RT8973A_MUIC_ADC_OPEN:
		if (dev1 & RT8973A_REG_DEV1_USB_MASK)
			cable_type = RT8973A_MUIC_ADC_USB;
		else if (dev1 & RT8973A_REG_DEV1_DCPORT_MASK)
			cable_type = RT8973A_MUIC_ADC_TA;
		else
			cable_type = RT8973A_MUIC_ADC_OPEN;
		break;
	default:
		break;
	}

	return cable_type;
}

static int rt8973a_muic_cable_handler(struct rt8973a_muic_info *info,
					enum rt8973a_event_type event)
{
	static unsigned int prev_cable_type;
	unsigned int con_sw = DM_DP_SWITCH_UART;
	int ret, cable_type;
	unsigned int id;
	bool attached = false;

	switch (event) {
	case RT8973A_EVENT_ATTACH:
		cable_type = rt8973a_muic_get_cable_type(info);
		attached = true;
		break;
	case RT8973A_EVENT_DETACH:
		cable_type = prev_cable_type;
		attached = false;
		break;
	case RT8973A_EVENT_OVP:
	case RT8973A_EVENT_OTP:
		dev_warn(info->dev,
			"happen Over %s issue. Need to disconnect all cables\n",
			event == RT8973A_EVENT_OVP ? "Voltage" : "Temperature");
		cable_type = prev_cable_type;
		attached = false;
		break;
	default:
		dev_err(info->dev,
			"Cannot handle this event (event:%d)\n", event);
		return -EINVAL;
	}
	prev_cable_type = cable_type;

	switch (cable_type) {
	case RT8973A_MUIC_ADC_OTG:
		id = EXTCON_USB_HOST;
		con_sw = DM_DP_SWITCH_USB;
		break;
	case RT8973A_MUIC_ADC_TA:
		id = EXTCON_CHG_USB_DCP;
		con_sw = DM_DP_SWITCH_OPEN;
		break;
	case RT8973A_MUIC_ADC_FACTORY_MODE_BOOT_OFF_USB:
	case RT8973A_MUIC_ADC_FACTORY_MODE_BOOT_ON_USB:
		id = EXTCON_JIG;
		con_sw = DM_DP_SWITCH_USB;
		break;
	case RT8973A_MUIC_ADC_FACTORY_MODE_BOOT_OFF_UART:
	case RT8973A_MUIC_ADC_FACTORY_MODE_BOOT_ON_UART:
		id = EXTCON_JIG;
		con_sw = DM_DP_SWITCH_UART;
		break;
	case RT8973A_MUIC_ADC_USB:
		id = EXTCON_USB;
		con_sw = DM_DP_SWITCH_USB;
		break;
	case RT8973A_MUIC_ADC_OPEN:
		return 0;
	case RT8973A_MUIC_ADC_UNKNOWN_ACC_1:
	case RT8973A_MUIC_ADC_UNKNOWN_ACC_2:
	case RT8973A_MUIC_ADC_UNKNOWN_ACC_3:
	case RT8973A_MUIC_ADC_UNKNOWN_ACC_4:
	case RT8973A_MUIC_ADC_UNKNOWN_ACC_5:
		dev_warn(info->dev,
			"Unknown accessory type (adc:0x%x)\n", cable_type);
		return 0;
	case RT8973A_MUIC_ADC_AUDIO_SEND_END_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S1_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S2_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S3_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S4_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S5_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S6_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S7_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S8_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S9_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S10_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S11_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_REMOTE_S12_BUTTON:
	case RT8973A_MUIC_ADC_AUDIO_TYPE2:
		dev_warn(info->dev,
			"Audio device/button type (adc:0x%x)\n", cable_type);
		return 0;
	case RT8973A_MUIC_ADC_RESERVED_ACC_1:
	case RT8973A_MUIC_ADC_RESERVED_ACC_2:
	case RT8973A_MUIC_ADC_RESERVED_ACC_3:
	case RT8973A_MUIC_ADC_RESERVED_ACC_4:
	case RT8973A_MUIC_ADC_RESERVED_ACC_5:
	case RT8973A_MUIC_ADC_PHONE_POWERED_DEV:
		return 0;
	default:
		dev_err(info->dev,
			"Cannot handle this cable_type (adc:0x%x)\n",
			cable_type);
		return -EINVAL;
	}

	/* Change internal hardware path(DM_CON/DP_CON) */
	ret = rt8973a_muic_set_path(info, con_sw, attached);
	if (ret < 0)
		return ret;

	/* Change the state of external accessory */
	extcon_set_cable_state_(info->edev, id, attached);

	return 0;
}

static void rt8973a_muic_irq_work(struct work_struct *work)
{
	struct rt8973a_muic_info *info = container_of(work,
			struct rt8973a_muic_info, irq_work);
	int ret = 0;

	if (!info->edev)
		return;

	mutex_lock(&info->mutex);

	/* Detect attached or detached cables */
	if (info->irq_attach) {
		ret = rt8973a_muic_cable_handler(info, RT8973A_EVENT_ATTACH);
		info->irq_attach = false;
	}

	if (info->irq_detach) {
		ret = rt8973a_muic_cable_handler(info, RT8973A_EVENT_DETACH);
		info->irq_detach = false;
	}

	if (info->irq_ovp) {
		ret = rt8973a_muic_cable_handler(info, RT8973A_EVENT_OVP);
		info->irq_ovp = false;
	}

	if (info->irq_otp) {
		ret = rt8973a_muic_cable_handler(info, RT8973A_EVENT_OTP);
		info->irq_otp = false;
	}

	if (ret < 0)
		dev_err(info->dev, "failed to handle MUIC interrupt\n");

	mutex_unlock(&info->mutex);
}

static irqreturn_t rt8973a_muic_irq_handler(int irq, void *data)
{
	struct rt8973a_muic_info *info = data;
	int i, irq_type = -1;

	for (i = 0; i < info->num_muic_irqs; i++)
		if (irq == info->muic_irqs[i].virq)
			irq_type = info->muic_irqs[i].irq;

	switch (irq_type) {
	case RT8973A_INT1_ATTACH:
		info->irq_attach = true;
		break;
	case RT8973A_INT1_DETACH:
		info->irq_detach = true;
		break;
	case RT8973A_INT1_OVP:
		info->irq_ovp = true;
		break;
	case RT8973A_INT1_OTP:
		info->irq_otp = true;
		break;
	case RT8973A_INT1_CHGDET:
	case RT8973A_INT1_DCD_T:
	case RT8973A_INT1_CONNECT:
	case RT8973A_INT1_ADC_CHG:
	case RT8973A_INT2_UVLO:
	case RT8973A_INT2_POR:
	case RT8973A_INT2_OTP_FET:
	case RT8973A_INT2_OVP_FET:
	case RT8973A_INT2_OCP_LATCH:
	case RT8973A_INT2_OCP:
	case RT8973A_INT2_OVP_OCP:
	default:
		dev_dbg(info->dev,
			"Cannot handle this interrupt (%d)\n", irq_type);
		break;
	}

	schedule_work(&info->irq_work);

	return IRQ_HANDLED;
}

static void rt8973a_muic_detect_cable_wq(struct work_struct *work)
{
	struct rt8973a_muic_info *info = container_of(to_delayed_work(work),
				struct rt8973a_muic_info, wq_detcable);
	int ret;

	/* Notify the state of connector cable or not  */
	ret = rt8973a_muic_cable_handler(info, RT8973A_EVENT_ATTACH);
	if (ret < 0)
		dev_warn(info->dev, "failed to detect cable state\n");
}

static void rt8973a_init_dev_type(struct rt8973a_muic_info *info)
{
	unsigned int data, vendor_id, version_id;
	int i, ret;

	/* To test I2C, Print version_id and vendor_id of RT8973A */
	ret = regmap_read(info->regmap, RT8973A_REG_DEVICE_ID, &data);
	if (ret) {
		dev_err(info->dev,
			"failed to read DEVICE_ID register: %d\n", ret);
		return;
	}

	vendor_id = ((data & RT8973A_REG_DEVICE_ID_VENDOR_MASK) >>
				RT8973A_REG_DEVICE_ID_VENDOR_SHIFT);
	version_id = ((data & RT8973A_REG_DEVICE_ID_VERSION_MASK) >>
				RT8973A_REG_DEVICE_ID_VERSION_SHIFT);

	dev_info(info->dev, "Device type: version: 0x%x, vendor: 0x%x\n",
			    version_id, vendor_id);

	/* Initiazle the register of RT8973A device to bring-up */
	for (i = 0; i < info->num_reg_data; i++) {
		u8 reg = info->reg_data[i].reg;
		u8 mask = info->reg_data[i].mask;
		u8 val = 0;

		if (info->reg_data[i].invert)
			val = ~info->reg_data[i].val;
		else
			val = info->reg_data[i].val;

		regmap_update_bits(info->regmap, reg, mask, val);
	}

	/* Check whether RT8973A is auto swithcing mode or not */
	ret = regmap_read(info->regmap, RT8973A_REG_CONTROL1, &data);
	if (ret) {
		dev_err(info->dev,
			"failed to read CONTROL1 register: %d\n", ret);
		return;
	}

	data &= RT8973A_REG_CONTROL1_AUTO_CONFIG_MASK;
	if (data) {
		info->auto_config = true;
		dev_info(info->dev,
			"Enable Auto-configuration for internal path\n");
	}
}

static int rt8973a_muic_i2c_probe(struct i2c_client *i2c,
				 const struct i2c_device_id *id)
{
	struct device_node *np = i2c->dev.of_node;
	struct rt8973a_muic_info *info;
	int i, ret, irq_flags;

	if (!np)
		return -EINVAL;

	info = devm_kzalloc(&i2c->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	i2c_set_clientdata(i2c, info);

	info->dev = &i2c->dev;
	info->i2c = i2c;
	info->irq = i2c->irq;
	info->muic_irqs = rt8973a_muic_irqs;
	info->num_muic_irqs = ARRAY_SIZE(rt8973a_muic_irqs);
	info->reg_data = rt8973a_reg_data;
	info->num_reg_data = ARRAY_SIZE(rt8973a_reg_data);

	mutex_init(&info->mutex);

	INIT_WORK(&info->irq_work, rt8973a_muic_irq_work);

	info->regmap = devm_regmap_init_i2c(i2c, &rt8973a_muic_regmap_config);
	if (IS_ERR(info->regmap)) {
		ret = PTR_ERR(info->regmap);
		dev_err(info->dev, "failed to allocate register map: %d\n",
				   ret);
		return ret;
	}

	/* Support irq domain for RT8973A MUIC device */
	irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_SHARED;
	ret = regmap_add_irq_chip(info->regmap, info->irq, irq_flags, 0,
				  &rt8973a_muic_irq_chip, &info->irq_data);
	if (ret != 0) {
		dev_err(info->dev, "failed to add irq_chip (irq:%d, err:%d)\n",
				    info->irq, ret);
		return ret;
	}

	for (i = 0; i < info->num_muic_irqs; i++) {
		struct muic_irq *muic_irq = &info->muic_irqs[i];
		int virq = 0;

		virq = regmap_irq_get_virq(info->irq_data, muic_irq->irq);
		if (virq <= 0)
			return -EINVAL;
		muic_irq->virq = virq;

		ret = devm_request_threaded_irq(info->dev, virq, NULL,
						rt8973a_muic_irq_handler,
						IRQF_NO_SUSPEND,
						muic_irq->name, info);
		if (ret) {
			dev_err(info->dev,
				"failed: irq request (IRQ: %d, error :%d)\n",
				muic_irq->irq, ret);
			return ret;
		}
	}

	/* Allocate extcon device */
	info->edev = devm_extcon_dev_allocate(info->dev, rt8973a_extcon_cable);
	if (IS_ERR(info->edev)) {
		dev_err(info->dev, "failed to allocate memory for extcon\n");
		return -ENOMEM;
	}

	/* Register extcon device */
	ret = devm_extcon_dev_register(info->dev, info->edev);
	if (ret) {
		dev_err(info->dev, "failed to register extcon device\n");
		return ret;
	}

	/*
	 * Detect accessory after completing the initialization of platform
	 *
	 * - Use delayed workqueue to detect cable state and then
	 * notify cable state to notifiee/platform through uevent.
	 * After completing the booting of platform, the extcon provider
	 * driver should notify cable state to upper layer.
	 */
	INIT_DELAYED_WORK(&info->wq_detcable, rt8973a_muic_detect_cable_wq);
	queue_delayed_work(system_power_efficient_wq, &info->wq_detcable,
			msecs_to_jiffies(DELAY_MS_DEFAULT));

	/* Initialize RT8973A device and print vendor id and version id */
	rt8973a_init_dev_type(info);

	return 0;
}

static int rt8973a_muic_i2c_remove(struct i2c_client *i2c)
{
	struct rt8973a_muic_info *info = i2c_get_clientdata(i2c);

	regmap_del_irq_chip(info->irq, info->irq_data);

	return 0;
}

static const struct of_device_id rt8973a_dt_match[] = {
	{ .compatible = "richtek,rt8973a-muic" },
	{ },
};
MODULE_DEVICE_TABLE(of, rt8973a_dt_match);

#ifdef CONFIG_PM_SLEEP
static int rt8973a_muic_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct rt8973a_muic_info *info = i2c_get_clientdata(i2c);

	enable_irq_wake(info->irq);

	return 0;
}

static int rt8973a_muic_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct rt8973a_muic_info *info = i2c_get_clientdata(i2c);

	disable_irq_wake(info->irq);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(rt8973a_muic_pm_ops,
			 rt8973a_muic_suspend, rt8973a_muic_resume);

static const struct i2c_device_id rt8973a_i2c_id[] = {
	{ "rt8973a", TYPE_RT8973A },
	{ }
};
MODULE_DEVICE_TABLE(i2c, rt8973a_i2c_id);

static struct i2c_driver rt8973a_muic_i2c_driver = {
	.driver		= {
		.name	= "rt8973a",
		.pm	= &rt8973a_muic_pm_ops,
		.of_match_table = rt8973a_dt_match,
	},
	.probe	= rt8973a_muic_i2c_probe,
	.remove	= rt8973a_muic_i2c_remove,
	.id_table = rt8973a_i2c_id,
};

static int __init rt8973a_muic_i2c_init(void)
{
	return i2c_add_driver(&rt8973a_muic_i2c_driver);
}
subsys_initcall(rt8973a_muic_i2c_init);

MODULE_DESCRIPTION("Richtek RT8973A Extcon driver");
MODULE_AUTHOR("Chanwoo Choi <cw00.choi@samsung.com>");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * rt8973a.h
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __LINUX_EXTCON_RT8973A_H
#define __LINUX_EXTCON_RT8973A_H

enum rt8973a_types {
	TYPE_RT8973A,
};

/* RT8973A registers */
enum rt8973A_reg {
	RT8973A_REG_DEVICE_ID = 0x1,
	RT8973A_REG_CONTROL1,
	RT8973A_REG_INT1,
	RT8973A_REG_INT2,
	RT8973A_REG_INTM1,
	RT8973A_REG_INTM2,
	RT8973A_REG_ADC,
	RT8973A_REG_RSVD_1,
	RT8973A_REG_RSVD_2,
	RT8973A_REG_DEV1,
	RT8973A_REG_DEV2,
	RT8973A_REG_RSVD_3,
	RT8973A_REG_RSVD_4,
	RT8973A_REG_RSVD_5,
	RT8973A_REG_RSVD_6,
	RT8973A_REG_RSVD_7,
	RT8973A_REG_RSVD_8,
	RT8973A_REG_RSVD_9,
	RT8973A_REG_MANUAL_SW1,
	RT8973A_REG_MANUAL_SW2,
	RT8973A_REG_RSVD_10,
	RT8973A_REG_RSVD_11,
	RT8973A_REG_RSVD_12,
	RT8973A_REG_RSVD_13,
	RT8973A_REG_RSVD_14,
	RT8973A_REG_RSVD_15,
	RT8973A_REG_RESET,

	RT8973A_REG_END,
};

/* Define RT8973A MASK/SHIFT constant */
#define RT8973A_REG_DEVICE_ID_VENDOR_SHIFT	0
#define RT8973A_REG_DEVICE_ID_VERSION_SHIFT	3
#define RT8973A_REG_DEVICE_ID_VENDOR_MASK	(0x7 << RT8973A_REG_DEVICE_ID_VENDOR_SHIFT)
#define RT8973A_REG_DEVICE_ID_VERSION_MASK	(0x1f << RT8973A_REG_DEVICE_ID_VERSION_SHIFT)

#define RT8973A_REG_CONTROL1_INTM_SHIFT	0
#define RT8973A_REG_CONTROL1_AUTO_CONFIG_SHIFT	2
#define RT8973A_REG_CONTROL1_I2C_RST_EN_SHIFT	3
#define RT8973A_REG_CONTROL1_SWITCH_OPEN_SHIFT	4
#define RT8973A_REG_CONTROL1_CHGTYP_SHIFT	5
#define RT8973A_REG_CONTROL1_USB_CHD_EN_SHIFT	6
#define RT8973A_REG_CONTROL1_ADC_EN_SHIFT	7
#define RT8973A_REG_CONTROL1_INTM_MASK		(0x1 << RT8973A_REG_CONTROL1_INTM_SHIFT)
#define RT8973A_REG_CONTROL1_AUTO_CONFIG_MASK	(0x1 << RT8973A_REG_CONTROL1_AUTO_CONFIG_SHIFT)
#define RT8973A_REG_CONTROL1_I2C_RST_EN_MASK	(0x1 << RT8973A_REG_CONTROL1_I2C_RST_EN_SHIFT)
#define RT8973A_REG_CONTROL1_SWITCH_OPEN_MASK	(0x1 << RT8973A_REG_CONTROL1_SWITCH_OPEN_SHIFT)
#define RT8973A_REG_CONTROL1_CHGTYP_MASK	(0x1 << RT8973A_REG_CONTROL1_CHGTYP_SHIFT)
#define RT8973A_REG_CONTROL1_USB_CHD_EN_MASK	(0x1 << RT8973A_REG_CONTROL1_USB_CHD_EN_SHIFT)
#define RT8973A_REG_CONTROL1_ADC_EN_MASK	(0x1 << RT8973A_REG_CONTROL1_ADC_EN_SHIFT)

#define RT9873A_REG_INTM1_ATTACH_SHIFT		0
#define RT9873A_REG_INTM1_DETACH_SHIFT		1
#define RT9873A_REG_INTM1_CHGDET_SHIFT		2
#define RT9873A_REG_INTM1_DCD_T_SHIFT		3
#define RT9873A_REG_INTM1_OVP_SHIFT		4
#define RT9873A_REG_INTM1_CONNECT_SHIFT		5
#define RT9873A_REG_INTM1_ADC_CHG_SHIFT		6
#define RT9873A_REG_INTM1_OTP_SHIFT		7
#define RT9873A_REG_INTM1_ATTACH_MASK		(0x1 << RT9873A_REG_INTM1_ATTACH_SHIFT)
#define RT9873A_REG_INTM1_DETACH_MASK		(0x1 <<  RT9873A_REG_INTM1_DETACH_SHIFT)
#define RT9873A_REG_INTM1_CHGDET_MASK		(0x1 <<  RT9873A_REG_INTM1_CHGDET_SHIFT)
#define RT9873A_REG_INTM1_DCD_T_MASK		(0x1 <<  RT9873A_REG_INTM1_DCD_T_SHIFT)
#define RT9873A_REG_INTM1_OVP_MASK		(0x1 <<  RT9873A_REG_INTM1_OVP_SHIFT)
#define RT9873A_REG_INTM1_CONNECT_MASK		(0x1 <<  RT9873A_REG_INTM1_CONNECT_SHIFT)
#define RT9873A_REG_INTM1_ADC_CHG_MASK		(0x1 <<  RT9873A_REG_INTM1_ADC_CHG_SHIFT)
#define RT9873A_REG_INTM1_OTP_MASK		(0x1 <<  RT9873A_REG_INTM1_OTP_SHIFT)

#define RT9873A_REG_INTM2_UVLO_SHIFT		1
#define RT9873A_REG_INTM2_POR_SHIFT		2
#define RT9873A_REG_INTM2_OTP_FET_SHIFT		3
#define RT9873A_REG_INTM2_OVP_FET_SHIFT		4
#define RT9873A_REG_INTM2_OCP_LATCH_SHIFT	5
#define RT9873A_REG_INTM2_OCP_SHIFT		6
#define RT9873A_REG_INTM2_OVP_OCP_SHIFT		7
#define RT9873A_REG_INTM2_UVLO_MASK		(0x1 << RT9873A_REG_INTM2_UVLO_SHIFT)
#define RT9873A_REG_INTM2_POR_MASK		(0x1 <<  RT9873A_REG_INTM2_POR_SHIFT)
#define RT9873A_REG_INTM2_OTP_FET_MASK		(0x1 <<  RT9873A_REG_INTM2_OTP_FET_SHIFT)
#define RT9873A_REG_INTM2_OVP_FET_MASK		(0x1 <<  RT9873A_REG_INTM2_OVP_FET_SHIFT)
#define RT9873A_REG_INTM2_OCP_LATCH_MASK	(0x1 <<  RT9873A_REG_INTM2_OCP_LATCH_SHIFT)
#define RT9873A_REG_INTM2_OCP_MASK		(0x1 <<  RT9873A_REG_INTM2_OCP_SHIFT)
#define RT9873A_REG_INTM2_OVP_OCP_MASK		(0x1 <<  RT9873A_REG_INTM2_OVP_OCP_SHIFT)

#define RT8973A_REG_ADC_SHIFT			0
#define RT8973A_REG_ADC_MASK			(0x1f << RT8973A_REG_ADC_SHIFT)

#define RT8973A_REG_DEV1_OTG_SHIFT		0
#define RT8973A_REG_DEV1_SDP_SHIFT		2
#define RT8973A_REG_DEV1_UART_SHIFT		3
#define RT8973A_REG_DEV1_CAR_KIT_TYPE1_SHIFT	4
#define RT8973A_REG_DEV1_CDPORT_SHIFT		5
#define RT8973A_REG_DEV1_DCPORT_SHIFT		6
#define RT8973A_REG_DEV1_OTG_MASK		(0x1 << RT8973A_REG_DEV1_OTG_SHIFT)
#define RT8973A_REG_DEV1_SDP_MASK		(0x1 << RT8973A_REG_DEV1_SDP_SHIFT)
#define RT8973A_REG_DEV1_UART_MASK		(0x1 << RT8973A_REG_DEV1_UART_SHIFT)
#define RT8973A_REG_DEV1_CAR_KIT_TYPE1_MASK	(0x1 << RT8973A_REG_DEV1_CAR_KIT_TYPE1_SHIFT)
#define RT8973A_REG_DEV1_CDPORT_MASK		(0x1 << RT8973A_REG_DEV1_CDPORT_SHIFT)
#define RT8973A_REG_DEV1_DCPORT_MASK		(0x1 << RT8973A_REG_DEV1_DCPORT_SHIFT)
#define RT8973A_REG_DEV1_USB_MASK		(RT8973A_REG_DEV1_SDP_MASK \
						| RT8973A_REG_DEV1_CDPORT_MASK)

#define RT8973A_REG_DEV2_JIG_USB_ON_SHIFT	0
#define RT8973A_REG_DEV2_JIG_USB_OFF_SHIFT	1
#define RT8973A_REG_DEV2_JIG_UART_ON_SHIFT	2
#define RT8973A_REG_DEV2_JIG_UART_OFF_SHIFT	3
#define RT8973A_REG_DEV2_JIG_USB_ON_MASK	(0x1 << RT8973A_REG_DEV2_JIG_USB_ON_SHIFT)
#define RT8973A_REG_DEV2_JIG_USB_OFF_MASK	(0x1 << RT8973A_REG_DEV2_JIG_USB_OFF_SHIFT)
#define RT8973A_REG_DEV2_JIG_UART_ON_MASK	(0x1 << RT8973A_REG_DEV2_JIG_UART_ON_SHIFT)
#define RT8973A_REG_DEV2_JIG_UART_OFF_MASK	(0x1 << RT8973A_REG_DEV2_JIG_UART_OFF_SHIFT)

#define RT8973A_REG_MANUAL_SW1_DP_SHIFT		2
#define RT8973A_REG_MANUAL_SW1_DM_SHIFT		5
#define RT8973A_REG_MANUAL_SW1_DP_MASK		(0x7 << RT8973A_REG_MANUAL_SW1_DP_SHIFT)
#define RT8973A_REG_MANUAL_SW1_DM_MASK		(0x7 << RT8973A_REG_MANUAL_SW1_DM_SHIFT)
#define DM_DP_CON_SWITCH_OPEN			0x0
#define DM_DP_CON_SWITCH_USB			0x1
#define DM_DP_CON_SWITCH_UART			0x3
#define DM_DP_SWITCH_OPEN			((DM_DP_CON_SWITCH_OPEN << RT8973A_REG_MANUAL_SW1_DP_SHIFT) \
						| (DM_DP_CON_SWITCH_OPEN << RT8973A_REG_MANUAL_SW1_DM_SHIFT))
#define DM_DP_SWITCH_USB			((DM_DP_CON_SWITCH_USB << RT8973A_REG_MANUAL_SW1_DP_SHIFT) \
						| (DM_DP_CON_SWITCH_USB << RT8973A_REG_MANUAL_SW1_DM_SHIFT))
#define DM_DP_SWITCH_UART			((DM_DP_CON_SWITCH_UART << RT8973A_REG_MANUAL_SW1_DP_SHIFT) \
						| (DM_DP_CON_SWITCH_UART << RT8973A_REG_MANUAL_SW1_DM_SHIFT))

#define RT8973A_REG_MANUAL_SW2_FET_ON_SHIFT	0
#define RT8973A_REG_MANUAL_SW2_JIG_ON_SHIFT	2
#define RT8973A_REG_MANUAL_SW2_BOOT_SW_SHIFT	3
#define RT8973A_REG_MANUAL_SW2_FET_ON_MASK	(0x1 << RT8973A_REG_MANUAL_SW2_FET_ON_SHIFT)
#define RT8973A_REG_MANUAL_SW2_JIG_ON_MASK	(0x1 << RT8973A_REG_MANUAL_SW2_JIG_ON_SHIFT)
#define RT8973A_REG_MANUAL_SW2_BOOT_SW_MASK	(0x1 << RT8973A_REG_MANUAL_SW2_BOOT_SW_SHIFT)
#define RT8973A_REG_MANUAL_SW2_FET_ON		0
#define RT8973A_REG_MANUAL_SW2_FET_OFF		0x1
#define RT8973A_REG_MANUAL_SW2_JIG_OFF		0
#define RT8973A_REG_MANUAL_SW2_JIG_ON		0x1
#define RT8973A_REG_MANUAL_SW2_BOOT_SW_ON	0
#define RT8973A_REG_MANUAL_SW2_BOOT_SW_OFF	0x1

#define RT8973A_REG_RESET_SHIFT			0
#define RT8973A_REG_RESET_MASK			(0x1 << RT8973A_REG_RESET_SHIFT)
#define RT8973A_REG_RESET			0x1

/* RT8973A Interrupts */
enum rt8973a_irq {
	/* Interrupt1*/
	RT8973A_INT1_ATTACH,
	RT8973A_INT1_DETACH,
	RT8973A_INT1_CHGDET,
	RT8973A_INT1_DCD_T,
	RT8973A_INT1_OVP,
	RT8973A_INT1_CONNECT,
	RT8973A_INT1_ADC_CHG,
	RT8973A_INT1_OTP,

	/* Interrupt2*/
	RT8973A_INT2_UVLO,
	RT8973A_INT2_POR,
	RT8973A_INT2_OTP_FET,
	RT8973A_INT2_OVP_FET,
	RT8973A_INT2_OCP_LATCH,
	RT8973A_INT2_OCP,
	RT8973A_INT2_OVP_OCP,

	RT8973A_NUM,
};

#define RT8973A_INT1_ATTACH_MASK		BIT(0)
#define RT8973A_INT1_DETACH_MASK		BIT(1)
#define RT8973A_INT1_CHGDET_MASK		BIT(2)
#define RT8973A_INT1_DCD_T_MASK			BIT(3)
#define RT8973A_INT1_OVP_MASK			BIT(4)
#define RT8973A_INT1_CONNECT_MASK		BIT(5)
#define RT8973A_INT1_ADC_CHG_MASK		BIT(6)
#define RT8973A_INT1_OTP_MASK			BIT(7)
#define RT8973A_INT2_UVLOT_MASK			BIT(0)
#define RT8973A_INT2_POR_MASK			BIT(1)
#define RT8973A_INT2_OTP_FET_MASK		BIT(2)
#define RT8973A_INT2_OVP_FET_MASK		BIT(3)
#define RT8973A_INT2_OCP_LATCH_MASK		BIT(4)
#define RT8973A_INT2_OCP_MASK			BIT(5)
#define RT8973A_INT2_OVP_OCP_MASK		BIT(6)

#endif /*  __LINUX_EXTCON_RT8973A_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * extcon-sm5502.c - Silicon Mitus SM5502 extcon drvier to support USB switches
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 * Author: Chanwoo Choi <cw00.choi@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/extcon.h>

#include "extcon-sm5502.h"

#define	DELAY_MS_DEFAULT		17000	/* unit: millisecond */

struct muic_irq {
	unsigned int irq;
	const char *name;
	unsigned int virq;
};

struct reg_data {
	u8 reg;
	unsigned int val;
	bool invert;
};

struct sm5502_muic_info {
	struct device *dev;
	struct extcon_dev *edev;

	struct i2c_client *i2c;
	struct regmap *regmap;

	struct regmap_irq_chip_data *irq_data;
	struct muic_irq *muic_irqs;
	unsigned int num_muic_irqs;
	int irq;
	bool irq_attach;
	bool irq_detach;
	struct work_struct irq_work;

	struct reg_data *reg_data;
	unsigned int num_reg_data;

	struct mutex mutex;

	/*
	 * Use delayed workqueue to detect cable state and then
	 * notify cable state to notifiee/platform through uevent.
	 * After completing the booting of platform, the extcon provider
	 * driver should notify cable state to upper layer.
	 */
	struct delayed_work wq_detcable;
};

/* Default value of SM5502 register to bring up MUIC device. */
static struct reg_data sm5502_reg_data[] = {
	{
		.reg = SM5502_REG_RESET,
		.val = SM5502_REG_RESET_MASK,
		.invert = true,
	}, {
		.reg = SM5502_REG_CONTROL,
		.val = SM5502_REG_CONTROL_MASK_INT_MASK,
		.invert = false,
	}, {
		.reg = SM5502_REG_INTMASK1,
		.val = SM5502_REG_INTM1_KP_MASK
			| SM5502_REG_INTM1_LKP_MASK
			| SM5502_REG_INTM1_LKR_MASK,
		.invert = true,
	}, {
		.reg = SM5502_REG_INTMASK2,
		.val = SM5502_REG_INTM2_VBUS_DET_MASK
			| SM5502_REG_INTM2_REV_ACCE_MASK
			| SM5502_REG_INTM2_ADC_CHG_MASK
			| SM5502_REG_INTM2_STUCK_KEY_MASK
			| SM5502_REG_INTM2_STUCK_KEY_RCV_MASK
			| SM5502_REG_INTM2_MHL_MASK,
		.invert = true,
	},
};

/* List of detectable cables */
static const unsigned int sm5502_extcon_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_CHG_USB_DCP,
	EXTCON_NONE,
};

/* Define supported accessory type */
enum sm5502_muic_acc_type {
	SM5502_MUIC_ADC_GROUND = 0x0,
	SM5502_MUIC_ADC_SEND_END_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S1_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S2_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S3_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S4_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S5_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S6_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S7_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S8_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S9_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S10_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S11_BUTTON,
	SM5502_MUIC_ADC_REMOTE_S12_BUTTON,
	SM5502_MUIC_ADC_RESERVED_ACC_1,
	SM5502_MUIC_ADC_RESERVED_ACC_2,
	SM5502_MUIC_ADC_RESERVED_ACC_3,
	SM5502_MUIC_ADC_RESERVED_ACC_4,
	SM5502_MUIC_ADC_RESERVED_ACC_5,
	SM5502_MUIC_ADC_AUDIO_TYPE2,
	SM5502_MUIC_ADC_PHONE_POWERED_DEV,
	SM5502_MUIC_ADC_TTY_CONVERTER,
	SM5502_MUIC_ADC_UART_CABLE,
	SM5502_MUIC_ADC_TYPE1_CHARGER,
	SM5502_MUIC_ADC_FACTORY_MODE_BOOT_OFF_USB,
	SM5502_MUIC_ADC_FACTORY_MODE_BOOT_ON_USB,
	SM5502_MUIC_ADC_AUDIO_VIDEO_CABLE,
	SM5502_MUIC_ADC_TYPE2_CHARGER,
	SM5502_MUIC_ADC_FACTORY_MODE_BOOT_OFF_UART,
	SM5502_MUIC_ADC_FACTORY_MODE_BOOT_ON_UART,
	SM5502_MUIC_ADC_AUDIO_TYPE1,
	SM5502_MUIC_ADC_OPEN = 0x1f,

	/* The below accessories have same ADC value (0x1f or 0x1e).
	   So, Device type1 is used to separate specific accessory. */
							/* |---------|--ADC| */
							/* |    [7:5]|[4:0]| */
	SM5502_MUIC_ADC_AUDIO_TYPE1_FULL_REMOTE = 0x3e,	/* |      001|11110| */
	SM5502_MUIC_ADC_AUDIO_TYPE1_SEND_END = 0x5e,	/* |      010|11110| */
							/* |Dev Type1|--ADC| */
	SM5502_MUIC_ADC_OPEN_USB = 0x5f,		/* |      010|11111| */
	SM5502_MUIC_ADC_OPEN_TA = 0xdf,			/* |      110|11111| */
	SM5502_MUIC_ADC_OPEN_USB_OTG = 0xff,		/* |      111|11111| */
};

/* List of supported interrupt for SM5502 */
static struct muic_irq sm5502_muic_irqs[] = {
	{ SM5502_IRQ_INT1_ATTACH,	"muic-attach" },
	{ SM5502_IRQ_INT1_DETACH,	"muic-detach" },
	{ SM5502_IRQ_INT1_KP,		"muic-kp" },
	{ SM5502_IRQ_INT1_LKP,		"muic-lkp" },
	{ SM5502_IRQ_INT1_LKR,		"muic-lkr" },
	{ SM5502_IRQ_INT1_OVP_EVENT,	"muic-ovp-event" },
	{ SM5502_IRQ_INT1_OCP_EVENT,	"muic-ocp-event" },
	{ SM5502_IRQ_INT1_OVP_OCP_DIS,	"muic-ovp-ocp-dis" },
	{ SM5502_IRQ_INT2_VBUS_DET,	"muic-vbus-det" },
	{ SM5502_IRQ_INT2_REV_ACCE,	"muic-rev-acce" },
	{ SM5502_IRQ_INT2_ADC_CHG,	"muic-adc-chg" },
	{ SM5502_IRQ_INT2_STUCK_KEY,	"muic-stuck-key" },
	{ SM5502_IRQ_INT2_STUCK_KEY_RCV, "muic-stuck-key-rcv" },
	{ SM5502_IRQ_INT2_MHL,		"muic-mhl" },
};

/* Define interrupt list of SM5502 to register regmap_irq */
static const struct regmap_irq sm5502_irqs[] = {
	/* INT1 interrupts */
	{ .reg_offset = 0, .mask = SM5502_IRQ_INT1_ATTACH_MASK, },
	{ .reg_offset = 0, .mask = SM5502_IRQ_INT1_DETACH_MASK, },
	{ .reg_offset = 0, .mask = SM5502_IRQ_INT1_KP_MASK, },
	{ .reg_offset = 0, .mask = SM5502_IRQ_INT1_LKP_MASK, },
	{ .reg_offset = 0, .mask = SM5502_IRQ_INT1_LKR_MASK, },
	{ .reg_offset = 0, .mask = SM5502_IRQ_INT1_OVP_EVENT_MASK, },
	{ .reg_offset = 0, .mask = SM5502_IRQ_INT1_OCP_EVENT_MASK, },
	{ .reg_offset = 0, .mask = SM5502_IRQ_INT1_OVP_OCP_DIS_MASK, },

	/* INT2 interrupts */
	{ .reg_offset = 1, .mask = SM5502_IRQ_INT2_VBUS_DET_MASK,},
	{ .reg_offset = 1, .mask = SM5502_IRQ_INT2_REV_ACCE_MASK, },
	{ .reg_offset = 1, .mask = SM5502_IRQ_INT2_ADC_CHG_MASK, },
	{ .reg_offset = 1, .mask = SM5502_IRQ_INT2_STUCK_KEY_MASK, },
	{ .reg_offset = 1, .mask = SM5502_IRQ_INT2_STUCK_KEY_RCV_MASK, },
	{ .reg_offset = 1, .mask = SM5502_IRQ_INT2_MHL_MASK, },
};

static const struct regmap_irq_chip sm5502_muic_irq_chip = {
	.name			= "sm5502",
	.status_base		= SM5502_REG_INT1,
	.mask_base		= SM5502_REG_INTMASK1,
	.mask_invert		= false,
	.num_regs		= 2,
	.irqs			= sm5502_irqs,
	.num_irqs		= ARRAY_SIZE(sm5502_irqs),
};

/* Define regmap configuration of SM5502 for I2C communication  */
static bool sm5502_muic_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case SM5502_REG_INTMASK1:
	case SM5502_REG_INTMASK2:
		return true;
	default:
		break;
	}
	return false;
}

static const struct regmap_config sm5502_muic_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.volatile_reg	= sm5502_muic_volatile_reg,
	.max_register	= SM5502_REG_END,
};

/* Change DM_CON/DP_CON/VBUSIN switch according to cable type */
static int sm5502_muic_set_path(struct sm5502_muic_info *info,
				unsigned int con_sw, unsigned int vbus_sw,
				bool attached)
{
	int ret;

	if (!attached) {
		con_sw	= DM_DP_SWITCH_OPEN;
		vbus_sw	= VBUSIN_SWITCH_OPEN;
	}

	switch (con_sw) {
	case DM_DP_SWITCH_OPEN:
	case DM_DP_SWITCH_USB:
	case DM_DP_SWITCH_AUDIO:
	case DM_DP_SWITCH_UART:
		ret = regmap_update_bits(info->regmap, SM5502_REG_MANUAL_SW1,
					 SM5502_REG_MANUAL_SW1_DP_MASK |
					 SM5502_REG_MANUAL_SW1_DM_MASK,
					 con_sw);
		if (ret < 0) {
			dev_err(info->dev,
				"cannot update DM_CON/DP_CON switch\n");
			return ret;
		}
		break;
	default:
		dev_err(info->dev, "Unknown DM_CON/DP_CON switch type (%d)\n",
				con_sw);
		return -EINVAL;
	};

	switch (vbus_sw) {
	case VBUSIN_SWITCH_OPEN:
	case VBUSIN_SWITCH_VBUSOUT:
	case VBUSIN_SWITCH_MIC:
	case VBUSIN_SWITCH_VBUSOUT_WITH_USB:
		ret = regmap_update_bits(info->regmap, SM5502_REG_MANUAL_SW1,
					 SM5502_REG_MANUAL_SW1_VBUSIN_MASK,
					 vbus_sw);
		if (ret < 0) {
			dev_err(info->dev,
				"cannot update VBUSIN switch\n");
			return ret;
		}
		break;
	default:
		dev_err(info->dev, "Unknown VBUS switch type (%d)\n", vbus_sw);
		return -EINVAL;
	};

	return 0;
}

/* Return cable type of attached or detached accessories */
static unsigned int sm5502_muic_get_cable_type(struct sm5502_muic_info *info)
{
	unsigned int cable_type = -1, adc, dev_type1;
	int ret;

	/* Read ADC value according to external cable or button */
	ret = regmap_read(info->regmap, SM5502_REG_ADC, &adc);
	if (ret) {
		dev_err(info->dev, "failed to read ADC register\n");
		return ret;
	}

	/*
	 * If ADC is SM5502_MUIC_ADC_GROUND(0x0), external cable hasn't
	 * connected with to MUIC device.
	 */
	cable_type = adc & SM5502_REG_ADC_MASK;
	if (cable_type == SM5502_MUIC_ADC_GROUND)
		return SM5502_MUIC_ADC_GROUND;

	switch (cable_type) {
	case SM5502_MUIC_ADC_GROUND:
	case SM5502_MUIC_ADC_SEND_END_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S1_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S2_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S3_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S4_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S5_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S6_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S7_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S8_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S9_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S10_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S11_BUTTON:
	case SM5502_MUIC_ADC_REMOTE_S12_BUTTON:
	case SM5502_MUIC_ADC_RESERVED_ACC_1:
	case SM5502_MUIC_ADC_RESERVED_ACC_2:
	case SM5502_MUIC_ADC_RESERVED_ACC_3:
	case SM5502_MUIC_ADC_RESERVED_ACC_4:
	case SM5502_MUIC_ADC_RESERVED_ACC_5:
	case SM5502_MUIC_ADC_AUDIO_TYPE2:
	case SM5502_MUIC_ADC_PHONE_POWERED_DEV:
	case SM5502_MUIC_ADC_TTY_CONVERTER:
	case SM5502_MUIC_ADC_UART_CABLE:
	case SM5502_MUIC_ADC_TYPE1_CHARGER:
	case SM5502_MUIC_ADC_FACTORY_MODE_BOOT_OFF_USB:
	case SM5502_MUIC_ADC_FACTORY_MODE_BOOT_ON_USB:
	case SM5502_MUIC_ADC_AUDIO_VIDEO_CABLE:
	case SM5502_MUIC_ADC_TYPE2_CHARGER:
	case SM5502_MUIC_ADC_FACTORY_MODE_BOOT_OFF_UART:
	case SM5502_MUIC_ADC_FACTORY_MODE_BOOT_ON_UART:
		break;
	case SM5502_MUIC_ADC_AUDIO_TYPE1:
		/*
		 * Check whether cable type is
		 * SM5502_MUIC_ADC_AUDIO_TYPE1_FULL_REMOTE
		 * or SM5502_MUIC_ADC_AUDIO_TYPE1_SEND_END
		 * by using Button event.
		 */
		break;
	case SM5502_MUIC_ADC_OPEN:
		ret = regmap_read(info->regmap, SM5502_REG_DEV_TYPE1,
				  &dev_type1);
		if (ret) {
			dev_err(info->dev, "failed to read DEV_TYPE1 reg\n");
			return ret;
		}

		switch (dev_type1) {
		case SM5502_REG_DEV_TYPE1_USB_SDP_MASK:
			cable_type = SM5502_MUIC_ADC_OPEN_USB;
			break;
		case SM5502_REG_DEV_TYPE1_DEDICATED_CHG_MASK:
			cable_type = SM5502_MUIC_ADC_OPEN_TA;
			break;
		case SM5502_REG_DEV_TYPE1_USB_OTG_MASK:
			cable_type = SM5502_MUIC_ADC_OPEN_USB_OTG;
			break;
		default:
			dev_dbg(info->dev,
				"cannot identify the cable type: adc(0x%x)\n",
				adc);
			return -EINVAL;
		};
		break;
	default:
		dev_err(info->dev,
			"failed to identify the cable type: adc(0x%x)\n", adc);
		return -EINVAL;
	};

	return cable_type;
}

static int sm5502_muic_cable_handler(struct sm5502_muic_info *info,
				     bool attached)
{
	static unsigned int prev_cable_type = SM5502_MUIC_ADC_GROUND;
	unsigned int cable_type = SM5502_MUIC_ADC_GROUND;
	unsigned int con_sw = DM_DP_SWITCH_OPEN;
	unsigned int vbus_sw = VBUSIN_SWITCH_OPEN;
	unsigned int id;
	int ret;

	/* Get the type of attached or detached cable */
	if (attached)
		cable_type = sm5502_muic_get_cable_type(info);
	else
		cable_type = prev_cable_type;
	prev_cable_type = cable_type;

	switch (cable_type) {
	case SM5502_MUIC_ADC_OPEN_USB:
		id	= EXTCON_USB;
		con_sw	= DM_DP_SWITCH_USB;
		vbus_sw	= VBUSIN_SWITCH_VBUSOUT_WITH_USB;
		break;
	case SM5502_MUIC_ADC_OPEN_TA:
		id	= EXTCON_CHG_USB_DCP;
		con_sw	= DM_DP_SWITCH_OPEN;
		vbus_sw	= VBUSIN_SWITCH_VBUSOUT;
		break;
	case SM5502_MUIC_ADC_OPEN_USB_OTG:
		id	= EXTCON_USB_HOST;
		con_sw	= DM_DP_SWITCH_USB;
		vbus_sw	= VBUSIN_SWITCH_OPEN;
		break;
	default:
		dev_dbg(info->dev,
			"cannot handle this cable_type (0x%x)\n", cable_type);
		return 0;
	};

	/* Change internal hardware path(DM_CON/DP_CON, VBUSIN) */
	ret = sm5502_muic_set_path(info, con_sw, vbus_sw, attached);
	if (ret < 0)
		return ret;

	/* Change the state of external accessory */
	extcon_set_cable_state_(info->edev, id, attached);

	return 0;
}

static void sm5502_muic_irq_work(struct work_struct *work)
{
	struct sm5502_muic_info *info = container_of(work,
			struct sm5502_muic_info, irq_work);
	int ret = 0;

	if (!info->edev)
		return;

	mutex_lock(&info->mutex);

	/* Detect attached or detached cables */
	if (info->irq_attach) {
		ret = sm5502_muic_cable_handler(info, true);
		info->irq_attach = false;
	}
	if (info->irq_detach) {
		ret = sm5502_muic_cable_handler(info, false);
		info->irq_detach = false;
	}

	if (ret < 0)
		dev_err(info->dev, "failed to handle MUIC interrupt\n");

	mutex_unlock(&info->mutex);
}

/*
 * Sets irq_attach or irq_detach in sm5502_muic_info and returns 0.
 * Returns -ESRCH if irq_type does not match registered IRQ for this dev type.
 */
static int sm5502_parse_irq(struct sm5502_muic_info *info, int irq_type)
{
	switch (irq_type) {
	case SM5502_IRQ_INT1_ATTACH:
		info->irq_attach = true;
		break;
	case SM5502_IRQ_INT1_DETACH:
		info->irq_detach = true;
		break;
	case SM5502_IRQ_INT1_KP:
	case SM5502_IRQ_INT1_LKP:
	case SM5502_IRQ_INT1_LKR:
	case SM5502_IRQ_INT1_OVP_EVENT:
	case SM5502_IRQ_INT1_OCP_EVENT:
	case SM5502_IRQ_INT1_OVP_OCP_DIS:
	case SM5502_IRQ_INT2_VBUS_DET:
	case SM5502_IRQ_INT2_REV_ACCE:
	case SM5502_IRQ_INT2_ADC_CHG:
	case SM5502_IRQ_INT2_STUCK_KEY:
	case SM5502_IRQ_INT2_STUCK_KEY_RCV:
	case SM5502_IRQ_INT2_MHL:
	default:
		break;
	}

	return 0;
}

static irqreturn_t sm5502_muic_irq_handler(int irq, void *data)
{
	struct sm5502_muic_info *info = data;
	int i, irq_type = -1, ret;

	for (i = 0; i < info->num_muic_irqs; i++)
		if (irq == info->muic_irqs[i].virq)
			irq_type = info->muic_irqs[i].irq;

	ret = sm5502_parse_irq(info, irq_type);
	if (ret < 0) {
		dev_warn(info->dev, "cannot handle is interrupt:%d\n",
				    irq_type);
		return IRQ_HANDLED;
	}
	schedule_work(&info->irq_work);

	return IRQ_HANDLED;
}

static void sm5502_muic_detect_cable_wq(struct work_struct *work)
{
	struct sm5502_muic_info *info = container_of(to_delayed_work(work),
				struct sm5502_muic_info, wq_detcable);
	int ret;

	/* Notify the state of connector cable or not  */
	ret = sm5502_muic_cable_handler(info, true);
	if (ret < 0)
		dev_warn(info->dev, "failed to detect cable state\n");
}

static void sm5502_init_dev_type(struct sm5502_muic_info *info)
{
	unsigned int reg_data, vendor_id, version_id;
	int i, ret;

	/* To test I2C, Print version_id and vendor_id of SM5502 */
	ret = regmap_read(info->regmap, SM5502_REG_DEVICE_ID, &reg_data);
	if (ret) {
		dev_err(info->dev,
			"failed to read DEVICE_ID register: %d\n", ret);
		return;
	}

	vendor_id = ((reg_data & SM5502_REG_DEVICE_ID_VENDOR_MASK) >>
				SM5502_REG_DEVICE_ID_VENDOR_SHIFT);
	version_id = ((reg_data & SM5502_REG_DEVICE_ID_VERSION_MASK) >>
				SM5502_REG_DEVICE_ID_VERSION_SHIFT);

	dev_info(info->dev, "Device type: version: 0x%x, vendor: 0x%x\n",
			    version_id, vendor_id);

	/* Initiazle the register of SM5502 device to bring-up */
	for (i = 0; i < info->num_reg_data; i++) {
		unsigned int val = 0;

		if (!info->reg_data[i].invert)
			val |= ~info->reg_data[i].val;
		else
			val = info->reg_data[i].val;
		regmap_write(info->regmap, info->reg_data[i].reg, val);
	}
}

static int sm5022_muic_i2c_probe(struct i2c_client *i2c,
				 const struct i2c_device_id *id)
{
	struct device_node *np = i2c->dev.of_node;
	struct sm5502_muic_info *info;
	int i, ret, irq_flags;

	if (!np)
		return -EINVAL;

	info = devm_kzalloc(&i2c->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	i2c_set_clientdata(i2c, info);

	info->dev = &i2c->dev;
	info->i2c = i2c;
	info->irq = i2c->irq;
	info->muic_irqs = sm5502_muic_irqs;
	info->num_muic_irqs = ARRAY_SIZE(sm5502_muic_irqs);
	info->reg_data = sm5502_reg_data;
	info->num_reg_data = ARRAY_SIZE(sm5502_reg_data);

	mutex_init(&info->mutex);

	INIT_WORK(&info->irq_work, sm5502_muic_irq_work);

	info->regmap = devm_regmap_init_i2c(i2c, &sm5502_muic_regmap_config);
	if (IS_ERR(info->regmap)) {
		ret = PTR_ERR(info->regmap);
		dev_err(info->dev, "failed to allocate register map: %d\n",
				   ret);
		return ret;
	}

	/* Support irq domain for SM5502 MUIC device */
	irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_SHARED;
	ret = regmap_add_irq_chip(info->regmap, info->irq, irq_flags, 0,
				  &sm5502_muic_irq_chip, &info->irq_data);
	if (ret != 0) {
		dev_err(info->dev, "failed to request IRQ %d: %d\n",
				    info->irq, ret);
		return ret;
	}

	for (i = 0; i < info->num_muic_irqs; i++) {
		struct muic_irq *muic_irq = &info->muic_irqs[i];
		int virq = 0;

		virq = regmap_irq_get_virq(info->irq_data, muic_irq->irq);
		if (virq <= 0)
			return -EINVAL;
		muic_irq->virq = virq;

		ret = devm_request_threaded_irq(info->dev, virq, NULL,
						sm5502_muic_irq_handler,
						IRQF_NO_SUSPEND,
						muic_irq->name, info);
		if (ret) {
			dev_err(info->dev,
				"failed: irq request (IRQ: %d, error :%d)\n",
				muic_irq->irq, ret);
			return ret;
		}
	}

	/* Allocate extcon device */
	info->edev = devm_extcon_dev_allocate(info->dev, sm5502_extcon_cable);
	if (IS_ERR(info->edev)) {
		dev_err(info->dev, "failed to allocate memory for extcon\n");
		return -ENOMEM;
	}

	/* Register extcon device */
	ret = devm_extcon_dev_register(info->dev, info->edev);
	if (ret) {
		dev_err(info->dev, "failed to register extcon device\n");
		return ret;
	}

	/*
	 * Detect accessory after completing the initialization of platform
	 *
	 * - Use delayed workqueue to detect cable state and then
	 * notify cable state to notifiee/platform through uevent.
	 * After completing the booting of platform, the extcon provider
	 * driver should notify cable state to upper layer.
	 */
	INIT_DELAYED_WORK(&info->wq_detcable, sm5502_muic_detect_cable_wq);
	queue_delayed_work(system_power_efficient_wq, &info->wq_detcable,
			msecs_to_jiffies(DELAY_MS_DEFAULT));

	/* Initialize SM5502 device and print vendor id and version id */
	sm5502_init_dev_type(info);

	return 0;
}

static int sm5502_muic_i2c_remove(struct i2c_client *i2c)
{
	struct sm5502_muic_info *info = i2c_get_clientdata(i2c);

	regmap_del_irq_chip(info->irq, info->irq_data);

	return 0;
}

static const struct of_device_id sm5502_dt_match[] = {
	{ .compatible = "siliconmitus,sm5502-muic" },
	{ },
};
MODULE_DEVICE_TABLE(of, sm5502_dt_match);

#ifdef CONFIG_PM_SLEEP
static int sm5502_muic_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5502_muic_info *info = i2c_get_clientdata(i2c);

	enable_irq_wake(info->irq);

	return 0;
}

static int sm5502_muic_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5502_muic_info *info = i2c_get_clientdata(i2c);

	disable_irq_wake(info->irq);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(sm5502_muic_pm_ops,
			 sm5502_muic_suspend, sm5502_muic_resume);

static const struct i2c_device_id sm5502_i2c_id[] = {
	{ "sm5502", TYPE_SM5502 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sm5502_i2c_id);

static struct i2c_driver sm5502_muic_i2c_driver = {
	.driver		= {
		.name	= "sm5502",
		.pm	= &sm5502_muic_pm_ops,
		.of_match_table = sm5502_dt_match,
	},
	.probe	= sm5022_muic_i2c_probe,
	.remove	= sm5502_muic_i2c_remove,
	.id_table = sm5502_i2c_id,
};

static int __init sm5502_muic_i2c_init(void)
{
	return i2c_add_driver(&sm5502_muic_i2c_driver);
}
subsys_initcall(sm5502_muic_i2c_init);

MODULE_DESCRIPTION("Silicon Mitus SM5502 Extcon driver");
MODULE_AUTHOR("Chanwoo Choi <cw00.choi@samsung.com>");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * sm5502.h
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __LINUX_EXTCON_SM5502_H
#define __LINUX_EXTCON_SM5502_H

enum sm5502_types {
	TYPE_SM5502,
};

/* SM5502 registers */
enum sm5502_reg {
	SM5502_REG_DEVICE_ID = 0x01,
	SM5502_REG_CONTROL,
	SM5502_REG_INT1,
	SM5502_REG_INT2,
	SM5502_REG_INTMASK1,
	SM5502_REG_INTMASK2,
	SM5502_REG_ADC,
	SM5502_REG_TIMING_SET1,
	SM5502_REG_TIMING_SET2,
	SM5502_REG_DEV_TYPE1,
	SM5502_REG_DEV_TYPE2,
	SM5502_REG_BUTTON1,
	SM5502_REG_BUTTON2,
	SM5502_REG_CAR_KIT_STATUS,
	SM5502_REG_RSVD1,
	SM5502_REG_RSVD2,
	SM5502_REG_RSVD3,
	SM5502_REG_RSVD4,
	SM5502_REG_MANUAL_SW1,
	SM5502_REG_MANUAL_SW2,
	SM5502_REG_DEV_TYPE3,
	SM5502_REG_RSVD5,
	SM5502_REG_RSVD6,
	SM5502_REG_RSVD7,
	SM5502_REG_RSVD8,
	SM5502_REG_RSVD9,
	SM5502_REG_RESET,
	SM5502_REG_RSVD10,
	SM5502_REG_RESERVED_ID1,
	SM5502_REG_RSVD11,
	SM5502_REG_RSVD12,
	SM5502_REG_RESERVED_ID2,
	SM5502_REG_RSVD13,
	SM5502_REG_OCP,
	SM5502_REG_RSVD14,
	SM5502_REG_RSVD15,
	SM5502_REG_RSVD16,
	SM5502_REG_RSVD17,
	SM5502_REG_RSVD18,
	SM5502_REG_RSVD19,
	SM5502_REG_RSVD20,
	SM5502_REG_RSVD21,
	SM5502_REG_RSVD22,
	SM5502_REG_RSVD23,
	SM5502_REG_RSVD24,
	SM5502_REG_RSVD25,
	SM5502_REG_RSVD26,
	SM5502_REG_RSVD27,
	SM5502_REG_RSVD28,
	SM5502_REG_RSVD29,
	SM5502_REG_RSVD30,
	SM5502_REG_RSVD31,
	SM5502_REG_RSVD32,
	SM5502_REG_RSVD33,
	SM5502_REG_RSVD34,
	SM5502_REG_RSVD35,
	SM5502_REG_RSVD36,
	SM5502_REG_RESERVED_ID3,

	SM5502_REG_END,
};

/* Define SM5502 MASK/SHIFT constant */
#define SM5502_REG_DEVICE_ID_VENDOR_SHIFT	0
#define SM5502_REG_DEVICE_ID_VERSION_SHIFT	3
#define SM5502_REG_DEVICE_ID_VENDOR_MASK	(0x3 << SM5502_REG_DEVICE_ID_VENDOR_SHIFT)
#define SM5502_REG_DEVICE_ID_VERSION_MASK	(0x1f << SM5502_REG_DEVICE_ID_VERSION_SHIFT)

#define SM5502_REG_CONTROL_MASK_INT_SHIFT	0
#define SM5502_REG_CONTROL_WAIT_SHIFT		1
#define SM5502_REG_CONTROL_MANUAL_SW_SHIFT	2
#define SM5502_REG_CONTROL_RAW_DATA_SHIFT	3
#define SM5502_REG_CONTROL_SW_OPEN_SHIFT	4
#define SM5502_REG_CONTROL_MASK_INT_MASK	(0x1 << SM5502_REG_CONTROL_MASK_INT_SHIFT)
#define SM5502_REG_CONTROL_WAIT_MASK		(0x1 << SM5502_REG_CONTROL_WAIT_SHIFT)
#define SM5502_REG_CONTROL_MANUAL_SW_MASK	(0x1 << SM5502_REG_CONTROL_MANUAL_SW_SHIFT)
#define SM5502_REG_CONTROL_RAW_DATA_MASK	(0x1 << SM5502_REG_CONTROL_RAW_DATA_SHIFT)
#define SM5502_REG_CONTROL_SW_OPEN_MASK		(0x1 << SM5502_REG_CONTROL_SW_OPEN_SHIFT)

#define SM5502_REG_INTM1_ATTACH_SHIFT		0
#define SM5502_REG_INTM1_DETACH_SHIFT		1
#define SM5502_REG_INTM1_KP_SHIFT		2
#define SM5502_REG_INTM1_LKP_SHIFT		3
#define SM5502_REG_INTM1_LKR_SHIFT		4
#define SM5502_REG_INTM1_OVP_EVENT_SHIFT	5
#define SM5502_REG_INTM1_OCP_EVENT_SHIFT	6
#define SM5502_REG_INTM1_OVP_OCP_DIS_SHIFT	7
#define SM5502_REG_INTM1_ATTACH_MASK		(0x1 << SM5502_REG_INTM1_ATTACH_SHIFT)
#define SM5502_REG_INTM1_DETACH_MASK		(0x1 << SM5502_REG_INTM1_DETACH_SHIFT)
#define SM5502_REG_INTM1_KP_MASK		(0x1 << SM5502_REG_INTM1_KP_SHIFT)
#define SM5502_REG_INTM1_LKP_MASK		(0x1 << SM5502_REG_INTM1_LKP_SHIFT)
#define SM5502_REG_INTM1_LKR_MASK		(0x1 << SM5502_REG_INTM1_LKR_SHIFT)
#define SM5502_REG_INTM1_OVP_EVENT_MASK		(0x1 << SM5502_REG_INTM1_OVP_EVENT_SHIFT)
#define SM5502_REG_INTM1_OCP_EVENT_MASK		(0x1 << SM5502_REG_INTM1_OCP_EVENT_SHIFT)
#define SM5502_REG_INTM1_OVP_OCP_DIS_MASK	(0x1 << SM5502_REG_INTM1_OVP_OCP_DIS_SHIFT)

#define SM5502_REG_INTM2_VBUS_DET_SHIFT		0
#define SM5502_REG_INTM2_REV_ACCE_SHIFT		1
#define SM5502_REG_INTM2_ADC_CHG_SHIFT		2
#define SM5502_REG_INTM2_STUCK_KEY_SHIFT	3
#define SM5502_REG_INTM2_STUCK_KEY_RCV_SHIFT	4
#define SM5502_REG_INTM2_MHL_SHIFT		5
#define SM5502_REG_INTM2_VBUS_DET_MASK		(0x1 << SM5502_REG_INTM2_VBUS_DET_SHIFT)
#define SM5502_REG_INTM2_REV_ACCE_MASK		(0x1 << SM5502_REG_INTM2_REV_ACCE_SHIFT)
#define SM5502_REG_INTM2_ADC_CHG_MASK		(0x1 << SM5502_REG_INTM2_ADC_CHG_SHIFT)
#define SM5502_REG_INTM2_STUCK_KEY_MASK		(0x1 << SM5502_REG_INTM2_STUCK_KEY_SHIFT)
#define SM5502_REG_INTM2_STUCK_KEY_RCV_MASK	(0x1 << SM5502_REG_INTM2_STUCK_KEY_RCV_SHIFT)
#define SM5502_REG_INTM2_MHL_MASK		(0x1 << SM5502_REG_INTM2_MHL_SHIFT)

#define SM5502_REG_ADC_SHIFT			0
#define SM5502_REG_ADC_MASK			(0x1f << SM5502_REG_ADC_SHIFT)

#define SM5502_REG_TIMING_SET1_KEY_PRESS_SHIFT	4
#define SM5502_REG_TIMING_SET1_KEY_PRESS_MASK	(0xf << SM5502_REG_TIMING_SET1_KEY_PRESS_SHIFT)
#define TIMING_KEY_PRESS_100MS			0x0
#define TIMING_KEY_PRESS_200MS			0x1
#define TIMING_KEY_PRESS_300MS			0x2
#define TIMING_KEY_PRESS_400MS			0x3
#define TIMING_KEY_PRESS_500MS			0x4
#define TIMING_KEY_PRESS_600MS			0x5
#define TIMING_KEY_PRESS_700MS			0x6
#define TIMING_KEY_PRESS_800MS			0x7
#define TIMING_KEY_PRESS_900MS			0x8
#define TIMING_KEY_PRESS_1000MS			0x9
#define SM5502_REG_TIMING_SET1_ADC_DET_SHIFT	0
#define SM5502_REG_TIMING_SET1_ADC_DET_MASK	(0xf << SM5502_REG_TIMING_SET1_ADC_DET_SHIFT)
#define TIMING_ADC_DET_50MS			0x0
#define TIMING_ADC_DET_100MS			0x1
#define TIMING_ADC_DET_150MS			0x2
#define TIMING_ADC_DET_200MS			0x3
#define TIMING_ADC_DET_300MS			0x4
#define TIMING_ADC_DET_400MS			0x5
#define TIMING_ADC_DET_500MS			0x6
#define TIMING_ADC_DET_600MS			0x7
#define TIMING_ADC_DET_700MS			0x8
#define TIMING_ADC_DET_800MS			0x9
#define TIMING_ADC_DET_900MS			0xA
#define TIMING_ADC_DET_1000MS			0xB

#define SM5502_REG_TIMING_SET2_SW_WAIT_SHIFT	4
#define SM5502_REG_TIMING_SET2_SW_WAIT_MASK	(0xf << SM5502_REG_TIMING_SET2_SW_WAIT_SHIFT)
#define TIMING_SW_WAIT_10MS			0x0
#define TIMING_SW_WAIT_30MS			0x1
#define TIMING_SW_WAIT_50MS			0x2
#define TIMING_SW_WAIT_70MS			0x3
#define TIMING_SW_WAIT_90MS			0x4
#define TIMING_SW_WAIT_110MS			0x5
#define TIMING_SW_WAIT_130MS			0x6
#define TIMING_SW_WAIT_150MS			0x7
#define TIMING_SW_WAIT_170MS			0x8
#define TIMING_SW_WAIT_190MS			0x9
#define TIMING_SW_WAIT_210MS			0xA
#define SM5502_REG_TIMING_SET2_LONG_KEY_SHIFT	0
#define SM5502_REG_TIMING_SET2_LONG_KEY_MASK	(0xf << SM5502_REG_TIMING_SET2_LONG_KEY_SHIFT)
#define TIMING_LONG_KEY_300MS			0x0
#define TIMING_LONG_KEY_400MS			0x1
#define TIMING_LONG_KEY_500MS			0x2
#define TIMING_LONG_KEY_600MS			0x3
#define TIMING_LONG_KEY_700MS			0x4
#define TIMING_LONG_KEY_800MS			0x5
#define TIMING_LONG_KEY_900MS			0x6
#define TIMING_LONG_KEY_1000MS			0x7
#define TIMING_LONG_KEY_1100MS			0x8
#define TIMING_LONG_KEY_1200MS			0x9
#define TIMING_LONG_KEY_1300MS			0xA
#define TIMING_LONG_KEY_1400MS			0xB
#define TIMING_LONG_KEY_1500MS			0xC

#define SM5502_REG_DEV_TYPE1_AUDIO_TYPE1_SHIFT		0
#define SM5502_REG_DEV_TYPE1_AUDIO_TYPE2_SHIFT		1
#define SM5502_REG_DEV_TYPE1_USB_SDP_SHIFT		2
#define SM5502_REG_DEV_TYPE1_UART_SHIFT			3
#define SM5502_REG_DEV_TYPE1_CAR_KIT_CHARGER_SHIFT	4
#define SM5502_REG_DEV_TYPE1_USB_CHG_SHIFT		5
#define SM5502_REG_DEV_TYPE1_DEDICATED_CHG_SHIFT	6
#define SM5502_REG_DEV_TYPE1_USB_OTG_SHIFT		7
#define SM5502_REG_DEV_TYPE1_AUDIO_TYPE1_MASK		(0x1 << SM5502_REG_DEV_TYPE1_AUDIO_TYPE1_SHIFT)
#define SM5502_REG_DEV_TYPE1_AUDIO_TYPE1__MASK		(0x1 << SM5502_REG_DEV_TYPE1_AUDIO_TYPE2_SHIFT)
#define SM5502_REG_DEV_TYPE1_USB_SDP_MASK		(0x1 << SM5502_REG_DEV_TYPE1_USB_SDP_SHIFT)
#define SM5502_REG_DEV_TYPE1_UART_MASK			(0x1 << SM5502_REG_DEV_TYPE1_UART_SHIFT)
#define SM5502_REG_DEV_TYPE1_CAR_KIT_CHARGER_MASK	(0x1 << SM5502_REG_DEV_TYPE1_CAR_KIT_CHARGER_SHIFT)
#define SM5502_REG_DEV_TYPE1_USB_CHG_MASK		(0x1 << SM5502_REG_DEV_TYPE1_USB_CHG_SHIFT)
#define SM5502_REG_DEV_TYPE1_DEDICATED_CHG_MASK		(0x1 << SM5502_REG_DEV_TYPE1_DEDICATED_CHG_SHIFT)
#define SM5502_REG_DEV_TYPE1_USB_OTG_MASK		(0x1 << SM5502_REG_DEV_TYPE1_USB_OTG_SHIFT)

#define SM5502_REG_DEV_TYPE2_JIG_USB_ON_SHIFT		0
#define SM5502_REG_DEV_TYPE2_JIG_USB_OFF_SHIFT		1
#define SM5502_REG_DEV_TYPE2_JIG_UART_ON_SHIFT		2
#define SM5502_REG_DEV_TYPE2_JIG_UART_OFF_SHIFT		3
#define SM5502_REG_DEV_TYPE2_PPD_SHIFT			4
#define SM5502_REG_DEV_TYPE2_TTY_SHIFT			5
#define SM5502_REG_DEV_TYPE2_AV_CABLE_SHIFT		6
#define SM5502_REG_DEV_TYPE2_JIG_USB_ON_MASK		(0x1 << SM5502_REG_DEV_TYPE2_JIG_USB_ON_SHIFT)
#define SM5502_REG_DEV_TYPE2_JIG_USB_OFF_MASK		(0x1 << SM5502_REG_DEV_TYPE2_JIG_USB_OFF_SHIFT)
#define SM5502_REG_DEV_TYPE2_JIG_UART_ON_MASK		(0x1 << SM5502_REG_DEV_TYPE2_JIG_UART_ON_SHIFT)
#define SM5502_REG_DEV_TYPE2_JIG_UART_OFF_MASK		(0x1 << SM5502_REG_DEV_TYPE2_JIG_UART_OFF_SHIFT)
#define SM5502_REG_DEV_TYPE2_PPD_MASK			(0x1 << SM5502_REG_DEV_TYPE2_PPD_SHIFT)
#define SM5502_REG_DEV_TYPE2_TTY_MASK			(0x1 << SM5502_REG_DEV_TYPE2_TTY_SHIFT)
#define SM5502_REG_DEV_TYPE2_AV_CABLE_MASK		(0x1 << SM5502_REG_DEV_TYPE2_AV_CABLE_SHIFT)

#define SM5502_REG_MANUAL_SW1_VBUSIN_SHIFT	0
#define SM5502_REG_MANUAL_SW1_DP_SHIFT		2
#define SM5502_REG_MANUAL_SW1_DM_SHIFT		5
#define SM5502_REG_MANUAL_SW1_VBUSIN_MASK	(0x3 << SM5502_REG_MANUAL_SW1_VBUSIN_SHIFT)
#define SM5502_REG_MANUAL_SW1_DP_MASK		(0x7 << SM5502_REG_MANUAL_SW1_DP_SHIFT)
#define SM5502_REG_MANUAL_SW1_DM_MASK		(0x7 << SM5502_REG_MANUAL_SW1_DM_SHIFT)
#define VBUSIN_SWITCH_OPEN			0x0
#define VBUSIN_SWITCH_VBUSOUT			0x1
#define VBUSIN_SWITCH_MIC			0x2
#define VBUSIN_SWITCH_VBUSOUT_WITH_USB		0x3
#define DM_DP_CON_SWITCH_OPEN			0x0
#define DM_DP_CON_SWITCH_USB			0x1
#define DM_DP_CON_SWITCH_AUDIO			0x2
#define DM_DP_CON_SWITCH_UART			0x3
#define DM_DP_SWITCH_OPEN			((DM_DP_CON_SWITCH_OPEN <<SM5502_REG_MANUAL_SW1_DP_SHIFT) \
						| (DM_DP_CON_SWITCH_OPEN <<SM5502_REG_MANUAL_SW1_DM_SHIFT))
#define DM_DP_SWITCH_USB			((DM_DP_CON_SWITCH_USB <<SM5502_REG_MANUAL_SW1_DP_SHIFT) \
						| (DM_DP_CON_SWITCH_USB <<SM5502_REG_MANUAL_SW1_DM_SHIFT))
#define DM_DP_SWITCH_AUDIO			((DM_DP_CON_SWITCH_AUDIO <<SM5502_REG_MANUAL_SW1_DP_SHIFT) \
						| (DM_DP_CON_SWITCH_AUDIO <<SM5502_REG_MANUAL_SW1_DM_SHIFT))
#define DM_DP_SWITCH_UART			((DM_DP_CON_SWITCH_UART <<SM5502_REG_MANUAL_SW1_DP_SHIFT) \
						| (DM_DP_CON_SWITCH_UART <<SM5502_REG_MANUAL_SW1_DM_SHIFT))

#define SM5502_REG_RESET_MASK			(0x1)

/* SM5502 Interrupts */
enum sm5502_irq {
	/* INT1 */
	SM5502_IRQ_INT1_ATTACH,
	SM5502_IRQ_INT1_DETACH,
	SM5502_IRQ_INT1_KP,
	SM5502_IRQ_INT1_LKP,
	SM5502_IRQ_INT1_LKR,
	SM5502_IRQ_INT1_OVP_EVENT,
	SM5502_IRQ_INT1_OCP_EVENT,
	SM5502_IRQ_INT1_OVP_OCP_DIS,

	/* INT2 */
	SM5502_IRQ_INT2_VBUS_DET,
	SM5502_IRQ_INT2_REV_ACCE,
	SM5502_IRQ_INT2_ADC_CHG,
	SM5502_IRQ_INT2_STUCK_KEY,
	SM5502_IRQ_INT2_STUCK_KEY_RCV,
	SM5502_IRQ_INT2_MHL,

	SM5502_IRQ_NUM,
};

#define SM5502_IRQ_INT1_ATTACH_MASK		BIT(0)
#define SM5502_IRQ_INT1_DETACH_MASK		BIT(1)
#define SM5502_IRQ_INT1_KP_MASK			BIT(2)
#define SM5502_IRQ_INT1_LKP_MASK		BIT(3)
#define SM5502_IRQ_INT1_LKR_MASK		BIT(4)
#define SM5502_IRQ_INT1_OVP_EVENT_MASK		BIT(5)
#define SM5502_IRQ_INT1_OCP_EVENT_MASK		BIT(6)
#define SM5502_IRQ_INT1_OVP_OCP_DIS_MASK	BIT(7)
#define SM5502_IRQ_INT2_VBUS_DET_MASK		BIT(0)
#define SM5502_IRQ_INT2_REV_ACCE_MASK		BIT(1)
#define SM5502_IRQ_INT2_ADC_CHG_MASK		BIT(2)
#define SM5502_IRQ_INT2_STUCK_KEY_MASK		BIT(3)
#define SM5502_IRQ_INT2_STUCK_KEY_RCV_MASK	BIT(4)
#define SM5502_IRQ_INT2_MHL_MASK		BIT(5)

#endif /*  __LINUX_EXTCON_SM5502_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /**
 * drivers/extcon/extcon-usb-gpio.c - USB GPIO extcon driver
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com
 * Author: Roger Quadros <rogerq@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/extcon.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define USB_GPIO_DEBOUNCE_MS	20	/* ms */

struct usb_extcon_info {
	struct device *dev;
	struct extcon_dev *edev;

	struct gpio_desc *id_gpiod;
	int id_irq;

	unsigned long debounce_jiffies;
	struct delayed_work wq_detcable;
};

static const unsigned int usb_extcon_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_NONE,
};

static void usb_extcon_detect_cable(struct work_struct *work)
{
	int id;
	struct usb_extcon_info *info = container_of(to_delayed_work(work),
						    struct usb_extcon_info,
						    wq_detcable);

	/* check ID and update cable state */
	id = gpiod_get_value_cansleep(info->id_gpiod);
	if (id) {
		/*
		 * ID = 1 means USB HOST cable detached.
		 * As we don't have event for USB peripheral cable attached,
		 * we simulate USB peripheral attach here.
		 */
		extcon_set_cable_state_(info->edev, EXTCON_USB_HOST, false);
		extcon_set_cable_state_(info->edev, EXTCON_USB, true);
	} else {
		/*
		 * ID = 0 means USB HOST cable attached.
		 * As we don't have event for USB peripheral cable detached,
		 * we simulate USB peripheral detach here.
		 */
		extcon_set_cable_state_(info->edev, EXTCON_USB, false);
		extcon_set_cable_state_(info->edev, EXTCON_USB_HOST, true);
	}
}

static irqreturn_t usb_irq_handler(int irq, void *dev_id)
{
	struct usb_extcon_info *info = dev_id;

	queue_delayed_work(system_power_efficient_wq, &info->wq_detcable,
			   info->debounce_jiffies);

	return IRQ_HANDLED;
}

static int usb_extcon_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct usb_extcon_info *info;
	int ret;

	if (!np)
		return -EINVAL;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = dev;
	info->id_gpiod = devm_gpiod_get(&pdev->dev, "id", GPIOD_IN);
	if (IS_ERR(info->id_gpiod)) {
		dev_err(dev, "failed to get ID GPIO\n");
		return PTR_ERR(info->id_gpiod);
	}

	info->edev = devm_extcon_dev_allocate(dev, usb_extcon_cable);
	if (IS_ERR(info->edev)) {
		dev_err(dev, "failed to allocate extcon device\n");
		return -ENOMEM;
	}

	ret = devm_extcon_dev_register(dev, info->edev);
	if (ret < 0) {
		dev_err(dev, "failed to register extcon device\n");
		return ret;
	}

	ret = gpiod_set_debounce(info->id_gpiod,
				 USB_GPIO_DEBOUNCE_MS * 1000);
	if (ret < 0)
		info->debounce_jiffies = msecs_to_jiffies(USB_GPIO_DEBOUNCE_MS);

	INIT_DELAYED_WORK(&info->wq_detcable, usb_extcon_detect_cable);

	info->id_irq = gpiod_to_irq(info->id_gpiod);
	if (info->id_irq < 0) {
		dev_err(dev, "failed to get ID IRQ\n");
		return info->id_irq;
	}

	ret = devm_request_threaded_irq(dev, info->id_irq, NULL,
					usb_irq_handler,
					IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					pdev->name, info);
	if (ret < 0) {
		dev_err(dev, "failed to request handler for ID IRQ\n");
		return ret;
	}

	platform_set_drvdata(pdev, info);
	device_init_wakeup(dev, 1);

	/* Perform initial detection */
	usb_extcon_detect_cable(&info->wq_detcable.work);

	return 0;
}

static int usb_extcon_remove(struct platform_device *pdev)
{
	struct usb_extcon_info *info = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&info->wq_detcable);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int usb_extcon_suspend(struct device *dev)
{
	struct usb_extcon_info *info = dev_get_drvdata(dev);
	int ret = 0;

	if (device_may_wakeup(dev)) {
		ret = enable_irq_wake(info->id_irq);
		if (ret)
			return ret;
	}

	/*
	 * We don't want to process any IRQs after this point
	 * as GPIOs used behind I2C subsystem might not be
	 * accessible until resume completes. So disable IRQ.
	 */
	disable_irq(info->id_irq);

	return ret;
}

static int usb_extcon_resume(struct device *dev)
{
	struct usb_extcon_info *info = dev_get_drvdata(dev);
	int ret = 0;

	if (device_may_wakeup(dev)) {
		ret = disable_irq_wake(info->id_irq);
		if (ret)
			return ret;
	}

	enable_irq(info->id_irq);
	if (!device_may_wakeup(dev))
		queue_delayed_work(system_power_efficient_wq,
				   &info->wq_detcable, 0);

	return ret;
}
#endif

static SIMPLE_DEV_PM_OPS(usb_extcon_pm_ops,
			 usb_extcon_suspend, usb_extcon_resume);

static const struct of_device_id usb_extcon_dt_match[] = {
	{ .compatible = "linux,extcon-usb-gpio", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, usb_extcon_dt_match);

static struct platform_driver usb_extcon_driver = {
	.probe		= usb_extcon_probe,
	.remove		= usb_extcon_remove,
	.driver		= {
		.name	= "extcon-usb-gpio",
		.pm	= &usb_extcon_pm_ops,
		.of_match_table = usb_extcon_dt_match,
	},
};

module_platform_driver(usb_extcon_driver);

MODULE_AUTHOR("Roger Quadros <rogerq@ti.com>");
MODULE_DESCRIPTION("USB GPIO extcon driver");
MODULE_LICENSE("GPL v2");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 *  drivers/extcon/extcon.c - External Connector (extcon) framework.
 *
 *  External connector (extcon) class driver
 *
 * Copyright (C) 2015 Samsung Electronics
 * Author: Chanwoo Choi <cw00.choi@samsung.com>
 *
 * Copyright (C) 2012 Samsung Electronics
 * Author: Donggeun Kim <dg77.kim@samsung.com>
 * Author: MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * based on android/drivers/switch/switch_class.c
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/extcon.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#define SUPPORTED_CABLE_MAX	32
#define CABLE_NAME_MAX		30

static const char *extcon_name[] =  {
	[EXTCON_NONE]			= "NONE",

	/* USB external connector */
	[EXTCON_USB]			= "USB",
	[EXTCON_USB_HOST]		= "USB-HOST",

	/* Charging external connector */
	[EXTCON_CHG_USB_SDP]		= "SDP",
	[EXTCON_CHG_USB_DCP]		= "DCP",
	[EXTCON_CHG_USB_CDP]		= "CDP",
	[EXTCON_CHG_USB_ACA]		= "ACA",
	[EXTCON_CHG_USB_FAST]		= "FAST-CHARGER",
	[EXTCON_CHG_USB_SLOW]		= "SLOW-CHARGER",

	/* Jack external connector */
	[EXTCON_JACK_MICROPHONE]	= "MICROPHONE",
	[EXTCON_JACK_HEADPHONE]		= "HEADPHONE",
	[EXTCON_JACK_LINE_IN]		= "LINE-IN",
	[EXTCON_JACK_LINE_OUT]		= "LINE-OUT",
	[EXTCON_JACK_VIDEO_IN]		= "VIDEO-IN",
	[EXTCON_JACK_VIDEO_OUT]		= "VIDEO-OUT",
	[EXTCON_JACK_SPDIF_IN]		= "SPDIF-IN",
	[EXTCON_JACK_SPDIF_OUT]		= "SPDIF-OUT",

	/* Display external connector */
	[EXTCON_DISP_HDMI]		= "HDMI",
	[EXTCON_DISP_MHL]		= "MHL",
	[EXTCON_DISP_DVI]		= "DVI",
	[EXTCON_DISP_VGA]		= "VGA",

	/* Miscellaneous external connector */
	[EXTCON_DOCK]			= "DOCK",
	[EXTCON_JIG]			= "JIG",
	[EXTCON_MECHANICAL]		= "MECHANICAL",

	NULL,
};

static struct class *extcon_class;
#if defined(CONFIG_ANDROID)
static struct class_compat *switch_class;
#endif /* CONFIG_ANDROID */

static LIST_HEAD(extcon_dev_list);
static DEFINE_MUTEX(extcon_dev_list_lock);

/**
 * check_mutually_exclusive - Check if new_state violates mutually_exclusive
 *			      condition.
 * @edev:	the extcon device
 * @new_state:	new cable attach status for @edev
 *
 * Returns 0 if nothing violates. Returns the index + 1 for the first
 * violated condition.
 */
static int check_mutually_exclusive(struct extcon_dev *edev, u32 new_state)
{
	int i = 0;

	if (!edev->mutually_exclusive)
		return 0;

	for (i = 0; edev->mutually_exclusive[i]; i++) {
		int weight;
		u32 correspondants = new_state & edev->mutually_exclusive[i];

		/* calculate the total number of bits set */
		weight = hweight32(correspondants);
		if (weight > 1)
			return i + 1;
	}

	return 0;
}

static int find_cable_index_by_id(struct extcon_dev *edev, const unsigned int id)
{
	int i;

	/* Find the the index of extcon cable in edev->supported_cable */
	for (i = 0; i < edev->max_supported; i++) {
		if (edev->supported_cable[i] == id)
			return i;
	}

	return -EINVAL;
}

static int find_cable_id_by_name(struct extcon_dev *edev, const char *name)
{
	int id = -EINVAL;
	int i = 0;

	/* Find the id of extcon cable */
	while (extcon_name[i]) {
		if (!strncmp(extcon_name[i], name, CABLE_NAME_MAX)) {
			id = i;
			break;
		}
		i++;
	}

	return id;
}

static int find_cable_index_by_name(struct extcon_dev *edev, const char *name)
{
	int id;

	if (edev->max_supported == 0)
		return -EINVAL;

	/* Find the the number of extcon cable */
	id = find_cable_id_by_name(edev, name);
	if (id < 0)
		return id;

	return find_cable_index_by_id(edev, id);
}

static bool is_extcon_changed(u32 prev, u32 new, int idx, bool *attached)
{
	if (((prev >> idx) & 0x1) != ((new >> idx) & 0x1)) {
		*attached = ((new >> idx) & 0x1) ? true : false;
		return true;
	}

	return false;
}

static ssize_t state_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int i, count = 0;
	struct extcon_dev *edev = dev_get_drvdata(dev);

	if (edev->max_supported == 0)
		return sprintf(buf, "%u\n", edev->state);

	for (i = 0; i < edev->max_supported; i++) {
		count += sprintf(buf + count, "%s=%d\n",
				extcon_name[edev->supported_cable[i]],
				 !!(edev->state & (1 << i)));
	}

	return count;
}

static ssize_t state_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	u32 state;
	ssize_t ret = 0;
	struct extcon_dev *edev = dev_get_drvdata(dev);

	ret = sscanf(buf, "0x%x", &state);
	if (ret == 0)
		ret = -EINVAL;
	else
		ret = extcon_set_state(edev, state);

	if (ret < 0)
		return ret;

	return count;
}
static DEVICE_ATTR_RW(state);

static ssize_t name_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct extcon_dev *edev = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", edev->name);
}
static DEVICE_ATTR_RO(name);

static ssize_t cable_name_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct extcon_cable *cable = container_of(attr, struct extcon_cable,
						  attr_name);
	int i = cable->cable_index;

	return sprintf(buf, "%s\n",
			extcon_name[cable->edev->supported_cable[i]]);
}

static ssize_t cable_state_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct extcon_cable *cable = container_of(attr, struct extcon_cable,
						  attr_state);

	int i = cable->cable_index;

	return sprintf(buf, "%d\n",
		       extcon_get_cable_state_(cable->edev,
					       cable->edev->supported_cable[i]));
}

/**
 * extcon_update_state() - Update the cable attach states of the extcon device
 *			   only for the masked bits.
 * @edev:	the extcon device
 * @mask:	the bit mask to designate updated bits.
 * @state:	new cable attach status for @edev
 *
 * Changing the state sends uevent with environment variable containing
 * the name of extcon device (envp[0]) and the state output (envp[1]).
 * Tizen uses this format for extcon device to get events from ports.
 * Android uses this format as well.
 *
 * Note that the notifier provides which bits are changed in the state
 * variable with the val parameter (second) to the callback.
 */
int extcon_update_state(struct extcon_dev *edev, u32 mask, u32 state)
{
	char name_buf[120];
	char state_buf[120];
	char *prop_buf;
	char *envp[3];
	int env_offset = 0;
	int length;
	int index;
	unsigned long flags;
	bool attached;

	if (!edev)
		return -EINVAL;

	spin_lock_irqsave(&edev->lock, flags);

	if (edev->state != ((edev->state & ~mask) | (state & mask))) {
		u32 old_state;

		if (check_mutually_exclusive(edev, (edev->state & ~mask) |
						   (state & mask))) {
			spin_unlock_irqrestore(&edev->lock, flags);
			return -EPERM;
		}

		old_state = edev->state;
		edev->state &= ~mask;
		edev->state |= state & mask;

		for (index = 0; index < edev->max_supported; index++) {
			if (is_extcon_changed(old_state, edev->state, ind