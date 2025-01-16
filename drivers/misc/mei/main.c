um ||
		    !strlen(data->temp_label[src])) {
			dev_info(dev,
				 "Invalid temperature source %d at index %d, source register 0x%x, temp register 0x%x\n",
				 src, i, data->REG_TEMP_SOURCE[i], reg_temp[i]);
			continue;
		}

		mask |= 1 << src;

		/* Use fixed index for SYSTIN(1), CPUTIN(2), AUXTIN(3) */
		if (src <= data->temp_fixed_num) {
			data->have_temp |= 1 << (src - 1);
			data->have_temp_fixed |= 1 << (src - 1);
			data->reg_temp[0][src - 1] = reg_temp[i];
			data->reg_temp[1][src - 1] = reg_temp_over[i];
			data->reg_temp[2][src - 1] = reg_temp_hyst[i];
			if (reg_temp_crit_h && reg_temp_crit_h[i])
				data->reg_temp[3][src - 1] = reg_temp_crit_h[i];
			else if (reg_temp_crit[src - 1])
				data->reg_temp[3][src - 1]
				  = reg_temp_crit[src - 1];
			if (reg_temp_crit_l && reg_temp_crit_l[i])
				data->reg_temp[4][src - 1] = reg_temp_crit_l[i];
			data->reg_temp_config[src - 1] = reg_temp_config[i];
			data->temp_src[src - 1] = src;
			continue;
		}

		if (s >= NUM_TEMP)
			continue;

		/* Use dynamic index for other sources */
		data->have_temp |= 1 << s;
		data->reg_temp[0][s] = reg_temp[i];
		data->reg_temp[1][s] = reg_temp_over[i];
		data->reg_temp[2][s] = reg_temp_hyst[i];
		data->reg_temp_config[s] = reg_temp_config[i];
		if (reg_temp_crit_h && reg_temp_crit_h[i])
			data->reg_temp[3][s] = reg_temp_crit_h[i];
		else if (reg_temp_crit[src - 1])
			data->reg_temp[3][s] = reg_temp_crit[src - 1];
		if (reg_temp_crit_l && reg_temp_crit_l[i])
			data->reg_temp[4][s] = reg_temp_crit_l[i];

		data->temp_src[s] = src;
		s++;
	}

	/*
	 * Repeat with temperatures used for fan control.
	 * This set of registers does not support limits.
	 */
	for (i = 0; i < num_reg_temp_mon; i++) {
		if (reg_temp_mon[i] == 0)
			continue;

		src = nct6775_read_value(data, data->REG_TEMP_SEL[i]) & 0x1f;
		if (!src || (mask & (1 << src)))
			continue;

		if (src >= data->temp_label_num ||
		    !strlen(data->temp_label[src])) {
			dev_info(dev,
				 "Invalid temperature source %d at index %d, source register 0x%x, temp register 0x%x\n",
				 src, i, data->REG_TEMP_SEL[i],
				 reg_temp_mon[i]);
			continue;
		}

		mask |= 1 << src;

		/* Use fixed index for SYSTIN(1), CPUTIN(2), AUXTIN(3) */
		if (src <= data->temp_fixed_num) {
			if (data->have_temp & (1 << (src - 1)))
				continue;
			data->have_temp |= 1 << (src - 1);
			data->have_temp_fixed |= 1 << (src - 1);
			data->reg_temp[0][src - 1] = reg_temp_mon[i];
			data->temp_src[src - 1] = src;
			continue;
		}

		if (s >= NUM_TEMP)
			continue;

		/* Use dynamic index for other sources */
		data->have_temp |= 1 << s;
		data->reg_temp[0][s] = reg_temp_mon[i];
		data->temp_src[s] = src;
		s++;
	}

#ifdef USE_ALTERNATE
	/*
	 * Go through the list of alternate temp registers and enable
	 * if possible.
	 * The temperature is already monitored if the respective bit in <mask>
	 * is set.
	 */
	for (i = 0; i < data->temp_label_num - 1; i++) {
		if (!reg_temp_alternate[i])
			continue;
		if (mask & (1 << (i + 1)))
			continue;
		if (i < data->temp_fixed_num) {
			if (data->have_temp & (1 << i))
				continue;
			data->have_temp |= 1 << i;
			data->have_temp_fixed |= 1 << i;
			data->reg_temp[0][i] = reg_temp_alternate[i];
			if (i < num_reg_temp) {
				data->reg_temp[1][i] = reg_temp_over[i];
				data->reg_temp[2][i] = reg_temp_hyst[i];
			}
			data->temp_src[i] = i + 1;
			continue;
		}

		if (s >= NUM_TEMP)	/* Abort if no more space */
			break;

		data->have_temp |= 1 << s;
		data->reg_temp[0][s] = reg_temp_alternate[i];
		data->temp_src[s] = i + 1;
		s++;
	}
#endif /* USE_ALTERNATE */

	/* Initialize the chip */
	nct6775_init_device(data);

	err = superio_enter(sio_data->sioreg);
	if (err)
		return err;

	cr2a = superio_inb(sio_data->sioreg, 0x2a);
	switch (data->kind) {
	case nct6775:
		data->have_vid = (cr2a & 0x40);
		break;
	case nct6776:
		data->have_vid = (cr2a & 0x60) == 0x40;
		break;
	case nct6106:
	case nct6779:
	case nct6791:
	case nct6792:
	case nct6793:
		break;
	}

	/*
	 * Read VID value
	 * We can get the VID input values directly at logical device D 0xe3.
	 */
	if (data->have_vid) {
		superio_select(sio_data->sioreg, NCT6775_LD_VID);
		data->vid = superio_inb(sio_data->sioreg, 0xe3);
		data->vrm = vid_which_vrm();
	}

	if (fan_debounce) {
		u8 tmp;

		superio_select(sio_data->sioreg, NCT6775_LD_HWM);
		tmp = superio_inb(sio_data->sioreg,
				  NCT6775_REG_CR_FAN_DEBOUNCE);
		switch (data->kind) {
		case nct6106:
			tmp |= 0xe0;
			break;
		case nct6775:
			tmp |= 0x1e;
			break;
		case nct6776:
		case nct6779:
			tmp |= 0x3e;
			break;
		case nct6791:
		case nct6792:
		case nct6793:
			tmp |= 0x7e;
			break;
		}
		superio_outb(sio_data->sioreg, NCT6775_REG_CR_FAN_DEBOUNCE,
			     tmp);
		dev_info(&pdev->dev, "Enabled fan debounce for chip %s\n",
			 data->name);
	}

	nct6775_check_fan_inputs(data);

	superio_exit(sio_data->sioreg);

	/* Read fan clock dividers immediately */
	nct6775_init_fan_common(dev, data);

	/* Register sysfs hooks */
	group = nct6775_create_attr_group(dev, &nct6775_pwm_template_group,
					  data->pwm_num);
	if (IS_ERR(group))
		return PTR_ERR(group);

	data->groups[num_attr_groups++] = group;

	group = nct6775_create_attr_group(dev, &nct6775_in_template_group,
					  fls(data->have_in));
	if (IS_ERR(group))
		return PTR_ERR(group);

	data->groups[num_attr_groups++] = group;

	group = nct6775_create_attr_group(dev, &nct6775_fan_template_group,
					  fls(data->has_fan));
	if (IS_ERR(group))
		return PTR_ERR(group);

	data->groups[num_attr_groups++] = group;

	group = nct6775_create_attr_group(dev, &nct6775_temp_template_group,
					  fls(data->have_temp));
	if (IS_ERR(group))
		return PTR_ERR(group);

	data->groups[num_attr_groups++] = group;
	data->groups[num_attr_groups++] = &nct6775_group_other;

	hwmon_dev = devm_hwmon_device_register_with_groups(dev, data->name,
							   data, data->groups);
	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static void nct6791_enable_io_mapping(int sioaddr)
{
	int val;

	val = superio_inb(sioaddr, NCT6791_REG_HM_IO_SPACE_LOCK_ENABLE);
	if (val & 0x10) {
		pr_info("Enabling hardware monitor logical device mappings.\n");
		superio_outb(sioaddr, NCT6791_REG_HM_IO_SPACE_LOCK_ENABLE,
			     val & ~0x10);
	}
}

static int __maybe_unused nct6775_suspend(struct device *dev)
{
	struct nct6775_data *data = nct6775_update_device(dev);

	mutex_lock(&data->update_lock);
	data->vbat = nct6775_read_value(data, data->REG_VBAT);
	if (data->kind == nct6775) {
		data->fandiv1 = nct6775_read_value(data, NCT6775_REG_FANDIV1);
		data->fandiv2 = nct6775_read_value(data, NCT6775_REG_FANDIV2);
	}
	mutex_unlock(&data->update_lock);

	return 0;
}

static int __maybe_unused nct6775_resume(struct device *dev)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	int sioreg = data->sioreg;
	int i, j, err = 0;
	u8 reg;

	mutex_lock(&data->update_lock);
	data->bank = 0xff;		/* Force initial bank selection */

	err = superio_enter(sioreg);
	if (err)
		goto abort;

	superio_select(sioreg, NCT6775_LD_HWM);
	reg = superio_inb(sioreg, SIO_REG_ENABLE);
	if (reg != data->sio_reg_enable)
		superio_outb(sioreg, SIO_REG_ENABLE, data->sio_reg_enable);

	if (data->kind == nct6791 || data->kind == nct6792 ||
	    data->kind == nct6793)
		nct6791_enable_io_mapping(sioreg);

	superio_exit(sioreg);

	/* Restore limits */
	for (i = 0; i < data->in_num; i++) {
		if (!(data->have_in & (1 << i)))
			continue;

		nct6775_write_value(data, data->REG_IN_MINMAX[0][i],
				    data->in[i][1]);
		nct6775_write_value(data, data->REG_IN_MINMAX[1][i],
				    data->in[i][2]);
	}

	for (i = 0; i < ARRAY_SIZE(data->fan_min); i++) {
		if (!(data->has_fan_min & (1 << i)))
			continue;

		nct6775_write_value(data, data->REG_FAN_MIN[i],
				    data->fan_min[i]);
	}

	for (i = 0; i < NUM_TEMP; i++) {
		if (!(data->have_temp & (1 << i)))
			continue;

		for (j = 1; j < ARRAY_SIZE(data->reg_temp); j++)
			if (data->reg_temp[j][i])
				nct6775_write_temp(data, data->reg_temp[j][i],
						   data->temp[j][i]);
	}

	/* Restore other settings */
	nct6775_write_value(data, data->REG_VBAT, data->vbat);
	if (data->kind == nct6775) {
		nct6775_write_value(data, NCT6775_REG_FANDIV1, data->fandiv1);
		nct6775_write_value(data, NCT6775_REG_FANDIV2, data->fandiv2);
	}

abort:
	/* Force re-reading all values */
	data->valid = false;
	mutex_unlock(&data->update_lock);

	return err;
}

static SIMPLE_DEV_PM_OPS(nct6775_dev_pm_ops, nct6775_suspend, nct6775_resume);

static struct platform_driver nct6775_driver = {
	.driver = {
		.name	= DRVNAME,
		.pm	= &nct6775_dev_pm_ops,
	},
	.probe		= nct6775_probe,
};

/* nct6775_find() looks for a '627 in the Super-I/O config space */
static int __init nct6775_find(int sioaddr, struct nct6775_sio_data *sio_data)
{
	u16 val;
	int err;
	int addr;

	err = superio_enter(sioaddr);
	if (err)
		return err;

	if (force_id)
		val = force_id;
	else
		val = (superio_inb(sioaddr, SIO_REG_DEVID) << 8)
		    | superio_inb(sioaddr, SIO_REG_DEVID + 1);
	switch (val & SIO_ID_MASK) {
	case SIO_NCT6106_ID:
		sio_data->kind = nct6106;
		break;
	case SIO_NCT6775_ID:
		sio_data->kind = nct6775;
		break;
	case SIO_NCT6776_ID:
		sio_data->kind = nct6776;
		break;
	case SIO_NCT6779_ID:
		sio_data->kind = nct6779;
		break;
	case SIO_NCT6791_ID:
		sio_data->kind = nct6791;
		break;
	case SIO_NCT6792_ID:
		sio_data->kind = nct6792;
		break;
	case SIO_NCT6793_ID:
		sio_data->kind = nct6793;
		break;
	default:
		if (val != 0xffff)
			pr_debug("unsupported chip ID: 0x%04x\n", val);
		superio_exit(sioaddr);
		return -ENODEV;
	}

	/* We have a known chip, find the HWM I/O address */
	superio_select(sioaddr, NCT6775_LD_HWM);
	val = (superio_inb(sioaddr, SIO_REG_ADDR) << 8)
	    | superio_inb(sioaddr, SIO_REG_ADDR + 1);
	addr = val & IOREGION_ALIGNMENT;
	if (addr == 0) {
		pr_err("Refusing to enable a Super-I/O device with a base I/O port 0\n");
		superio_exit(sioaddr);
		return -ENODEV;
	}

	/* Activate logical device if needed */
	val = superio_inb(sioaddr, SIO_REG_ENABLE);
	if (!(val & 0x01)) {
		pr_warn("Forcibly enabling Super-I/O. Sensor is probably unusable.\n");
		superio_outb(sioaddr, SIO_REG_ENABLE, val | 0x01);
	}

	if (sio_data->kind == nct6791 || sio_data->kind == nct6792 ||
	    sio_data->kind == nct6793)
		nct6791_enable_io_mapping(sioaddr);

	superio_exit(sioaddr);
	pr_info("Found %s or compatible chip at %#x:%#x\n",
		nct6775_sio_names[sio_data->kind], sioaddr, addr);
	sio_data->sioreg = sioaddr;

	return addr;
}

/*
 * when Super-I/O functions move to a separate file, the Super-I/O
 * bus will manage the lifetime of the device and this module will only keep
 * track of the nct6775 driver. But since we use platform_device_alloc(), we
 * must keep track of the device
 */
static struct platform_device *pdev[2];

static int __init sensors_nct6775_init(void)
{
	int i, err;
	bool found = false;
	int address;
	struct resource res;
	struct nct6775_sio_data sio_data;
	int sioaddr[2] = { 0x2e, 0x4e };

	err = platform_driver_register(&nct6775_driver);
	if (err)
		return err;

	/*
	 * initialize sio_data->kind and sio_data->sioreg.
	 *
	 * when Super-I/O functions move to a separate file, the Super-I/O
	 * driver will probe 0x2e and 0x4e and auto-detect the presence of a
	 * nct6775 hardware monitor, and call probe()
	 */
	for (i = 0; i < ARRAY_SIZE(pdev); i++) {
		address = nct6775_find(sioaddr[i], &sio_data);
		if (address <= 0)
			continue;

		found = true;

		pdev[i] = platform_device_alloc(DRVNAME, address);
		if (!pdev[i]) {
			err = -ENOMEM;
			goto exit_device_unregister;
		}

		err = platform_device_add_data(pdev[i], &sio_data,
					       sizeof(struct nct6775_sio_data));
		if (err)
			goto exit_device_put;

		memset(&res, 0, sizeof(res));
		res.name = DRVNAME;
		res.start = address + IOREGION_OFFSET;
		res.end = address + IOREGION_OFFSET + IOREGION_LENGTH - 1;
		res.flags = IORESOURCE_IO;

		err = acpi_check_resource_conflict(&res);
		if (err) {
			platform_device_put(pdev[i]);
			pdev[i] = NULL;
			continue;
		}

		err = platform_device_add_resources(pdev[i], &res, 1);
		if (err)
			goto exit_device_put;

		/* platform_device_add calls probe() */
		err = platform_device_add(pdev[i]);
		if (err)
			goto exit_device_put;
	}
	if (!found) {
		err = -ENODEV;
		goto exit_unregister;
	}

	return 0;

exit_device_put:
	platform_device_put(pdev[i]);
exit_device_unregister:
	while (--i >= 0) {
		if (pdev[i])
			platform_device_unregister(pdev[i]);
	}
exit_unregister:
	platform_driver_unregister(&nct6775_driver);
	return err;
}

static void __exit sensors_nct6775_exit(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pdev); i++) {
		if (pdev[i])
			platform_device_unregister(pdev[i]);
	}
	platform_driver_unregister(&nct6775_driver);
}

MODULE_AUTHOR("Guenter Roeck <linux@roeck-us.net>");
MODULE_DESCRIPTION("Driver for NCT6775F and compatible chips");
MODULE_LICENSE("GPL");

module_init(sensors_nct6775_init);
module_exit(sensors_nct6775_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
    MaxLinear MXL5005S VSB/QAM/DVBT tuner driver

    Copyright (C) 2008 MaxLinear
    Copyright (C) 2006 Steven Toth <stoth@linuxtv.org>
      Functions:
	mxl5005s_reset()
	mxl5005s_writereg()
	mxl5005s_writeregs()
	mxl5005s_init()
	mxl5005s_reconfigure()
	mxl5005s_AssignTunerMode()
	mxl5005s_set_params()
	mxl5005s_get_frequency()
	mxl5005s_get_bandwidth()
	mxl5005s_release()
	mxl5005s_attach()

    Copyright (C) 2008 Realtek
    Copyright (C) 2008 Jan Hoogenraad
      Functions:
	mxl5005s_SetRfFreqHz()

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/*
    History of this driver (Steven Toth):
      I was given a public release of a linux driver that included
      support for the MaxLinear MXL5005S silicon tuner. Analysis of
      the tuner driver showed clearly three things.

      1. The tuner driver didn't support the LinuxTV tuner API
	 so the code Realtek added had to be removed.

      2. A significant amount of the driver is reference driver code
	 from MaxLinear, I felt it was important to identify and
	 preserve this.

      3. New code has to be added to interface correctly with the
	 LinuxTV API, as a regular kernel module.

      Other than the reference driver enum's, I've clearly marked
      sections of the code and retained the copyright of the
      respective owners.
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "dvb_frontend.h"
#include "mxl5005s.h"

static int debug;

#define dprintk(level, arg...) do {    \
	if (level <= debug)            \
		printk(arg);    \
	} while (0)

#define TUNER_REGS_NUM          104
#define INITCTRL_NUM            40

#ifdef _MXL_PRODUCTION
#define CHCTRL_NUM              39
#else
#define CHCTRL_NUM              36
#endif

#define MXLCTRL_NUM             189
#define MASTER_CONTROL_ADDR     9

/* Enumeration of Master Control Register State */
enum master_control_state {
	MC_LOAD_START = 1,
	MC_POWER_DOWN,
	MC_SYNTH_RESET,
	MC_SEQ_OFF
};

/* Enumeration of MXL5005 Tuner Modulation Type */
enum {
	MXL_DEFAULT_MODULATION = 0,
	MXL_DVBT,
	MXL_ATSC