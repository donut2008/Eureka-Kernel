lease = qib_port_release,
	.sysfs_ops = &qib_diagc_ops,
	.default_attrs = diagc_default_attributes
};

/* End diag_counters */

/* end of per-port file structures and support code */

/*
 * Start of per-unit (or driver, in some cases, but replicated
 * per unit) functions (these get a device *)
 */
static ssize_t show_rev(struct device *device, struct device_attribute *attr,
			char *buf)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);

	return sprintf(buf, "%x\n", dd_from_dev(dev)->minrev);
}

static ssize_t show_hca(struct device *device, struct device_attribute *attr,
			char *buf)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);
	struct qib_devdata *dd = dd_from_dev(dev);
	int ret;

	if (!dd->boardname)
		ret = -EINVAL;
	else
		ret = scnprintf(buf, PAGE_SIZE, "%s\n", dd->boardname);
	return ret;
}

static ssize_t show_version(struct device *device,
			    struct device_attribute *attr, char *buf)
{
	/* The string printed here is already newline-terminated. */
	return scnprintf(buf, PAGE_SIZE, "%s", (char *)ib_qib_version);
}

static ssize_t show_boardversion(struct device *device,
				 struct device_attribute *attr, char *buf)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);
	struct qib_devdata *dd = dd_from_dev(dev);

	/* The string printed here is already newline-terminated. */
	return scnprintf(buf, PAGE_SIZE, "%s", dd->boardversion);
}


static ssize_t show_localbus_info(struct device *device,
				  struct device_attribute *attr, char *buf)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);
	struct qib_devdata *dd = dd_from_dev(dev);

	/* The string printed here is already newline-terminated. */
	return scnprintf(buf, PAGE_SIZE, "%s", dd->lbus_info);
}


static ssize_t show_nctxts(struct device *device,
			   struct device_attribute *attr, char *buf)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);
	struct qib_devdata *dd = dd_from_dev(dev);

	/* Return the number of user ports (contexts) available. */
	/* The calculation below deals with a special case where
	 * cfgctxts is set to 1 on a single-port board. */
	return scnprintf(buf, PAGE_SIZE, "%u\n",
			(dd->first_user_ctxt > dd->cfgctxts) ? 0 :
			(dd->cfgctxts - dd->first_user_ctxt));
}

static ssize_t show_nfreectxts(struct device *device,
			   struct device_attribute *attr, char *buf)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);
	struct qib_devdata *dd = dd_from_dev(dev);

	/* Return the number of free user ports (contexts) available. */
	return scnprintf(buf, PAGE_SIZE, "%u\n", dd->freectxts);
}

static ssize_t show_serial(struct device *device,
			   struct device_attribute *attr, char *buf)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);
	struct qib_devdata *dd = dd_from_dev(dev);

	buf[sizeof(dd->serial)] = '\0';
	memcpy(buf, dd->serial, sizeof(dd->serial));
	strcat(buf, "\n");
	return strlen(buf);
}

static ssize_t store_chip_reset(struct device *device,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);
	struct qib_devdata *dd = dd_from_dev(dev);
	int ret;

	if (count < 5 || memcmp(buf, "reset", 5) || !dd->diag_client) {
		ret = -EINVAL;
		goto bail;
	}

	ret = qib_reset_device(dd->unit);
bail:
	return ret < 0 ? ret : count;
}

/*
 * Dump tempsense regs. in decimal, to ease shell-scripts.
 */
static ssize_t show_tempsense(struct device *device,
			      struct device_attribute *attr, char *buf)
{
	struct qib_ibdev *dev =
		container_of(device, struct qib_ibdev, ibdev.dev);
	struct qib_devdata *dd = dd_from_dev(dev);
	int ret;
	int idx;
	u8 regvals[8];

	ret = -ENXIO;
	for (idx = 0; idx < 8; ++idx) {
		if (idx == 6)
			continue;
		ret = dd->f_tempsense_rd(dd, idx);
		if (ret < 0)
			break;
		regvals[idx] = ret;
	}
	if (idx == 8)
		ret = scnprintf(buf, PAGE_SIZE, "%d %d %02X %02X %d %d\n",
				*(signed char *)(regvals),
				*(signed char *)(regvals + 1),
				regvals[2], regvals[3],
				*(signed char *)(regvals + 5),
				*(signed char *)(regvals + 7));
	return ret;
}

/*
 * end of per-unit (or driver, in some cases, but replicated
 * per unit) functions
 */

/* start of per-unit file structures and support code */
static DEVICE_ATTR(hw_rev, S_IRUGO, show_rev, NULL);
static DEVICE_ATTR(hca_type, S_IRUGO, show_hca, NULL);
static DEVICE_ATTR(board_id, S_IRUGO, show_hca, NULL);
static DEVICE_ATTR(version, S_IRUGO, show_version, NULL);
static DEVICE_ATTR(nctxts, S_IRUGO, show_nctxts, NULL);
static DEVICE_ATTR(nfreectxts, S_IRUGO, show_nfreectxts, NULL);
static DEVICE_ATTR(serial, S_IRUGO, show_serial, NULL);
static DEVICE_ATTR(boardversion, S_IRUGO, show_boardversion, NULL);
static DEVICE_ATTR(tempsense, S_IRUGO, show_tempsense, NULL);
static DEVICE_ATTR(localbus_info, S_IRUGO, show_localbus_info, NULL);
static DEVICE_ATTR(chip_reset, S_IWUSR, NULL, store_chip_reset);

static struct device_attribute *qib_attributes[] = {
	&dev_attr_hw_rev,
	&dev_attr_hca_type,
	&dev_attr_board_id,
	&dev_attr_version,
	&dev_attr_nctxts,
	&dev_attr_nfreectxts,
	&dev_attr_serial,
	&dev_attr_boardversion,
	&dev_attr_tempsense,
	&dev_attr_localbus_info,
	&dev_attr_chip_reset,
};

int qib_create_port_files(struct ib_device *ibdev, u8 port_num,
			  struct kobject *kobj)
{
	struct qib_pportdata *ppd;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	int ret;

	if (!port_num || port_num > dd->num_pports) {
		qib_dev_err(dd,
			"Skipping infiniband class with invalid port %u\n",
			port_num);
		ret = -ENODEV;
		goto bail;
	}
	ppd = &dd->pport[port_num - 1];

	ret = kobject_init_and_add(&ppd->pport_kobj, &qib_port_ktype, kobj,
				   "linkcontrol");
	if (ret) {
		qib_dev_err(dd,
			"Skipping linkcontrol sysfs info, (err %d) port %u\n",
			ret, port_num);
		goto bail_link;
	}
	kobject_uevent(&ppd->pport_kobj, KOBJ_ADD);

	ret = kobject_init_and_add(&ppd->sl2vl_kobj, &qib_sl2vl_ktype, kobj,
				   "sl2vl");
	if (ret) {
		qib_dev_err(dd,
			"Skipping sl2vl sysfs info, (err %d) port %u\n",
			ret, port_num);
		goto bail_sl;
	}
	kobject_uevent(&ppd->sl2vl_kobj, KOBJ_ADD);

	ret = kobject_init_and_add(&ppd->diagc_kobj, &qib_diagc_ktype, kobj,
				   "diag_counters");
	if (ret) {
		qib_dev_err(dd,
			"Skipping diag_counters sysfs info, (err %d) port %u\n",
			ret, port_num);
		goto bail_diagc;
	}
	kobject_uevent(&ppd->diagc_kobj, KOBJ_ADD);

	if (!qib_cc_table_size || !ppd->congestion_entries_shadow)
		return 0;

	ret = kobject_init_and_add(&ppd->pport_cc_kobj, &qib_port_cc_ktype,
				kobj, "CCMgtA");
	if (ret) {
		qib_dev_err(dd,
		 "Skipping Congestion Control sysfs info, (err %d) port %u\n",
		 ret, port_num);
		goto bail_cc;
	}

	kobject_uevent(&ppd->pport_cc_kobj, KOBJ_ADD);

	ret = sysfs_create_bin_file(&ppd->pport_cc_kobj,
				&cc_setting_bin_attr);
	if (ret) {
		qib_dev_err(dd,
		 "Skipping Congestion Control setting sysfs info, (err %d) port %u\n",
		 ret, port_num);
		goto bail_cc;
	}

	ret = sysfs_create_bin_file(&ppd->pport_cc_kobj,
				&cc_table_bin_attr);
	if (ret) {
		qib_dev_err(dd,
		 "Skipping Congestion Control table sysfs info, (err %d) port %u\n",
		 ret, port_num);
		goto bail_cc_entry_bin;
	}

	qib_devinfo(dd->pcidev,
		"IB%u: Congestion Control Agent enabled for port %d\n",
		dd->unit, port_num);

	return 0;

bail_cc_entry_bin:
	sysfs_remove_bin_file(&ppd->pport_cc_kobj, &cc_setting_bin_attr);
bail_cc:
	kobject_put(&ppd->pport_cc_kobj);
bail_diagc:
	kobject_put(&ppd->diagc_kobj);
bail_sl:
	kobject_put(&ppd->sl2vl_kobj);
bail_link:
	kobject_put(&ppd->pport_kobj);
bail:
	return ret;
}

/*
 * Register and create our files in /sys/class/infiniband.
 */
int qib_verbs_register_sysfs(struct qib_devdata *dd)
{
	struct ib_device *dev = &dd->verbs_dev.ibdev;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(qib_attributes); ++i) {
		ret = device_create_file(&dev->dev, qib_attributes[i]);
		if (ret)
			goto bail;
	}

	return 0;
bail:
	for (i = 0; i < ARRAY_SIZE(qib_attributes); ++i)
		device_remove_file(&dev->dev, qib_attributes[i]);
	return ret;
}

/*
 * Unregister and remove our files in /sys/class/infiniband.
 */
void qib_verbs_unregister_sysfs(struct qib_devdata *dd)
{
	struct qib_pportdata *ppd;
	int i;

	for (i = 0; i < dd->num_pports; i++) {
		ppd = &dd->pport[i];
		if (qib_cc_table_size &&
			ppd->congestion_entries_shadow) {
			sysfs_remove_bin_file(&ppd->pport_cc_kobj,
				&cc_setting_bin_attr);
			sysfs_remove_bin_file(&ppd->pport_cc_kobj,
				&cc_table_bin_attr);
			kobject_put(&ppd->pport_cc_kobj);
		}
		kobject_put(&ppd->diagc_kobj);
		kobject_put(&ppd->sl2vl_kobj);
		kobject_put(&ppd->pport_kobj);
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (c) 2012 Intel Corporation. All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>

#include "qib.h"

/*
 * QLogic_IB "Two Wire Serial Interface" driver.
 * Originally written for a not-quite-i2c serial eeprom, which is
 * still used on some supported boards. Later boards have added a
 * variety of other uses, most board-specific, so the bit-boffing
 * part has been split off to this file, while the other parts
 * have been moved to chip-specific files.
 *
 * We have also dropped all pretense of fully generic (e.g. pretend
 * we don't know whether '1' is the higher voltage) interface, as
 * the restrictions of the generic i2c interface (e.g. no access from
 * driver itself) make it unsuitable for this use.
 */

#define READ_CMD 1
#define WRITE_CMD 0

/**
 * i2c_wait_for_writes - wait for a write
 * @dd: the qlogic_ib device
 *
 * We use this instead of udelay directly, so we can make sure
 * that previous register writes have been flushed all the way
 * to the chip.  Since we are delaying anyway, the cost doesn't
 * hurt, and makes the bit twiddling more regular
 */
static void i2c_wait_for_writes(struct qib_devdata *dd)
{
	/*
	 * implicit read of EXTStatus is as good as explicit
	 * read of scratch, if all we want to do is flush
	 * writes.
	 */
	dd->f_gpio_mod(dd, 0, 0, 0);
	rmb(); /* inlined, so prevent compiler reordering */
}

/*
 * QSFP modules are allowed to hold SCL low for 500uSec. Allow twice that
 * for "almost compliant" modules
 */
#define SCL_WAIT_USEC 1000

/* BUF_WAIT is time bus must be free between STOP or ACK and to next START.
 * Should be 20, but some chips need more.
 */
#define TWSI_BUF_WAIT_USEC 60

static void scl_out(struct qib_devdata *dd, u8 bit)
{
	u32 mask;

	udelay(1);

	mask = 1UL << dd->gpio_scl_num;

	/* SCL is meant to be bare-drain, so never set "OUT", just DIR */
	dd->f_gpio_mod(dd, 0, bit ? 0 : mask, mask);

	/*
	 * Allow for slow slaves by simple
	 * delay for falling edge, sampling on rise.
	 */
	if (!bit)
		udelay(2);
	else {
		int rise_usec;

		for (rise_usec = SCL_WAIT_USEC; rise_usec > 0; rise_usec -= 2) {
			if (mask & dd->f_gpio_mod(dd, 0, 0, 0))
				break;
			udelay(2);
		}
		if (rise_usec <= 0)
			qib_dev_err(dd, "SCL interface stuck low > %d uSec\n",
				    SCL_WAIT_USEC);
	}
	i2c_wait_for_writes(dd);
}

static void sda_out(struct qib_devdata *dd, u8 bit)
{
	u32 mask;

	mask = 1UL << dd->gpio_sda_num;

	/* SDA is meant to be bare-drain, so never set "OUT", just DIR */
	dd->f_gpio_mod(dd, 0, bit ? 0 : mask, mask);

	i2c_wait_for_writes(dd);
	udelay(2);
}

static u8 sda_in(struct qib_devdata *dd, int wait)
{
	int bnum;
	u32 read_val, mask;

	bnum = dd->gpio_sda_num;
	mask = (1UL << bnum);
	/* SDA is meant to be bare-drain, so never set "OUT", just DIR */
	dd->f_gpio_mod(dd, 0, 0, mask);
	read_val = dd->f_gpio_mod(dd, 0, 0, 0);
	if (wait)
		i2c_wait_for_writes(dd);
	return (read_val & mask) >> bnum;
}

/**
 * i2c_ackrcv - see if ack following write is true
 * @dd: the qlogic_ib device
 */
static int i2c_ackrcv(struct qib_devdata *dd)
{
	u8 ack_received;

	/* AT ENTRY SCL = LOW */
	/* change direction, ignore data */
	ack_received = sda_in(dd, 1);
	scl_out(dd, 1);
	ack_received = sda_in(dd, 1) == 0;
	scl_out(dd, 0);
	return ack_received;
}

static void stop_cmd(struct qib_devdata *dd);

/**
 * rd_byte - read a byte, sending STOP on last, else ACK
 * @dd: the qlogic_ib device
 *
 * Returns byte shifted out of device
 */
static int rd_byte(struct qib_devdata *dd, int last)
{
	int bit_cntr, data;

	data = 0;

	for (bit_cntr = 7; bit_cntr >= 0; --bit_cntr) {
		data <<= 1;
		scl_out(dd, 1);
		data |= sda_in(dd, 0);
		scl_out(dd, 0);
	}
	if (last) {
		scl_out(dd, 1);
		stop_cmd(dd);
	} else {
		sda_out(dd, 0);
		scl_out(dd, 1);
		scl_out(dd, 0);
		sda_out(dd, 1);
	}
	return data;
}

/**
 * wr_byte - write a byte, one bit at a time
 * @dd: the qlogic_ib device
 * @data: the byte to write
 *
 * Returns 0 if we got the following ack, otherwise 1
 */
static int wr_byte(struct qib_devdata *dd, u8 data)
{
	int bit_cntr;
	u8 bit;

	for (bit_cntr = 7; bit_cntr >= 0; bit_cntr--) {
		bit = (data >> bit_cntr) & 1;
		sda_out(dd, bit);
		scl_out(dd, 1);
		scl_out(dd, 0);
	}
	return (!i2c_ackrcv(dd)) ? 1 : 0;
}

/*
 * issue TWSI start sequence:
 * (both clock/data high, clock high, data low while clock is high)
 */
static void start_seq(struct qib_devdata *dd)
{
	sda_out(dd, 1);
	scl_out(dd, 1);
	sda_out(dd, 0);
	udelay(1);
	scl_out(dd, 0);
}

/**
 * stop_seq - transmit the stop sequence
 * @dd: the qlogic_ib device
 *
 * (both clock/data low, clock high, data high while clock is high)
 */
static void stop_seq(struct qib_devdata *dd)
{
	scl_out(dd, 0);
	sda_out(dd, 0);
	scl_out(dd, 1);
	sda_out(dd, 1);
}

/**
 * stop_cmd - transmit the stop condition
 * @dd: the qlogic_ib device
 *
 * (both clock/data low, clock high, data high while clock is high)
 */
static void stop_cmd(struct qib_devdata *dd)
{
	stop_seq(dd);
	udelay(TWSI_BUF_WAIT_USEC);
}

/**
 * qib_twsi_reset - reset I2C communication
 * @dd: the qlogic_ib device
 */

int qib_twsi_reset(struct qib_devdata *dd)
{
	int clock_cycles_left = 9;
	int was_high = 0;
	u32 pins, mask;

	/* Both SCL and SDA should be high. If not, there
	 * is something wrong.
	 */
	mask = (1UL << dd->gpio_scl_num) | (1UL << dd->gpio_sda_num);

	/*
	 * Force pins to desired innocuous state.
	 * This is the default power-on state with out=0 and dir=0,
	 * So tri-stated and should be floating high (barring HW problems)
	 */
	dd->f_gpio_mod(dd, 0, 0, mask);

	/*
	 * Clock nine times to get all listeners into a sane state.
	 * If SDA does not go high at any point, we are wedged.
	 * One vendor recommends then issuing START followed by STOP.
	 * we cannot use our "normal" functions to do that, because
	 * if SCL drops between them, another vendor's part will
	 * wedge, dropping SDA and keeping it low forever, at the end of
	 * the next transaction (even if it was not the device addressed).
	 * So our START and STOP take place with SCL held high.
	 */
	while (clock_cycles_left--) {
		scl_out(dd, 0);
		scl_out(dd, 1);
		/* Note if SDA is high, but keep clocking to sync slave */
		was_high |= sda_in(dd, 0);
	}

	if (was_high) {
		/*
		 * We saw a high, which we hope means the slave is sync'd.
		 * Issue START, STOP, pause for T_BUF.
		 */

		pins = dd->f_gpio_mod(dd, 0, 0, 0);
		if ((pins & mask) != mask)
			qib_dev_err(dd, "GPIO pins not at rest: %d\n",
				    pins & mask);
		/* Drop SDA to issue START */
		udelay(1); /* Guarantee .6 uSec setup */
		sda_out(dd, 0);
		udelay(1); /* Guarantee .6 uSec hold */
		/* At this point, SCL is high, SDA low. Raise SDA for STOP */
		sda_out(dd, 1);
		udelay(TWSI_BUF_WAIT_USEC);
	}

	return !was_high;
}

#define QIB_TWSI_START 0x100
#define QIB_TWSI_STOP 0x200

/* Write byte to TWSI, optionally prefixed with START or suffixed with
 * STOP.
 * returns 0 if OK (ACK received), else != 0
 */
static int qib_twsi_wr(struct qib_devdata *dd, int data, int flags)
{
	int ret = 1;

	if (flags & QIB_TWSI_START)
		start_seq(dd);

	ret = wr_byte(dd, data); /* Leaves SCL low (from i2c_ackrcv()) */

	if (flags & QIB_TWSI_STOP)
		stop_cmd(dd);
	return ret;
}

/* Added functionality for IBA7220-based cards */
#define QIB_TEMP_DEV 0x98

/*
 * qib_twsi_blk_rd
 * Formerly called qib_eeprom_internal_read, and only used for eeprom,
 * but now the general interface for data transfer from twsi devices.
 * One vestige of its former role is that it recognizes a device
 * QIB_TWSI_NO_DEV and does the correct operation for the legacy part,
 * which responded to all TWSI device codes, interpreting them as
 * address within device. On all other devices found on board handled by
 * this driver, the device is followed by a one-byte "address" which selects
 * the "register" or "offset" within the device from which data should
 * be read.
 */
int qib_twsi_blk_rd(struct qib_devdata *dd, int dev, int addr,
		    void *buffer, int len)
{
	int ret;
	u8 *bp = buffer;

	ret = 1;

	if (dev == QIB_TWSI_NO_DEV) {
		/* legacy not-really-I2C */
		addr = (addr << 1) | READ_CMD;
		ret = qib_twsi_wr(dd, addr, QIB_TWSI_START);
	} else {
		/* Actual I2C */
		ret = qib_twsi_wr(dd, dev | WRITE_CMD, QIB_TWSI_START);
		if (ret) {
			stop_cmd(dd);
			ret = 1;
			goto bail;
		}
		/*
		 * SFF spec claims we do _not_ stop after the addr
		 * but simply issue a start with the "read" dev-addr.
		 * Since we are implicitely waiting for ACK here,
		 * we need t_buf (nominally 20uSec) before that start,
		 * and cannot rely on the delay built in to the STOP
		 */
		ret = qib_twsi_wr(dd, addr, 0);
		udelay(TWSI_BUF_WAIT_USEC);

		if (ret) {
			qib_dev_err(dd,
				"Failed to write interface read addr %02X\n",
				addr);
			ret = 1;
			goto bail;
		}
		ret = qib_twsi_wr(dd, dev | READ_CMD, QIB_TWSI_START);
	}
	if (ret) {
		stop_cmd(dd);
		ret = 1;
		goto bail;
	}

	/*
	 * block devices keeps clocking data out as long as we ack,
	 * automatically incrementing the address. Some have "pages"
	 * whose boundaries will not be crossed, but the handling
	 * of these is left to the caller, who is in a better
	 * position to know.
	 */
	while (len-- > 0) {
		/*
		 * Get and store data, sending ACK if length remaining,
		 * else STOP
		 */
		*bp++ = rd_byte(dd, !len);
	}

	ret = 0;

bail:
	return ret;
}

/*
 * qib_twsi_blk_wr
 * Formerly called qib_eeprom_internal_write, and only used for eeprom,
 * but now the general interface for data transfer to twsi devices.
 * One vestige of its former role is that it recognizes a device
 * QIB_TWSI_NO_DEV and does the correct operation for the legacy part,
 * which responded to all TWSI device codes, interpreting them as
 * address within device. On all other devices found on board handled by
 * this driver, the device is followed by a one-byte "address" which selects
 * the "register" or "offset" within the device to which data should
 * be written.
 */
int qib_twsi_blk_wr(struct qib_devdata *dd, int dev, int addr,
		    const void *buffer, int len)
{
	int sub_len;
	const u8 *bp = buffer;
	int max_wait_time, i;
	int ret = 1;

	while (len > 0) {
		if (dev == QIB_TWSI_NO_DEV) {
			if (qib_twsi_wr(dd, (addr << 1) | WRITE_CMD,
					QIB_TWSI_START)) {
				goto failed_write;
			}
		} else {
			/* Real I2C */
			if (qib_twsi_wr(dd, dev | WRITE_CMD, QIB_TWSI_START))
				goto failed_write;
			ret = qib_twsi_wr(dd, addr, 0);
			if (ret) {
				qib_dev_err(dd,
					"Failed to write interface write addr %02X\n",
					addr);
				goto failed_write;
			}
		}

		sub_len = min(len, 4);
		addr += sub_len;
		len -= sub_len;

		for (i = 0; i < sub_len; i++)
			if (qib_twsi_wr(dd, *bp++, 0))
				goto failed_write;

		stop_cmd(dd);

		/*
		 * Wait for write complete by waiting for a successful
		 * read (the chip replies with a zero after the write
		 * cmd completes, and before it writes to the eeprom.
		 * The startcmd for the read will fail the ack until
		 * the writes have completed.   We do this inline to avoid
		 * the debug prints that are in the real read routine
		 * if the startcmd fails.
		 * We also use the proper device address, so it doesn't matter
		 * whether we have real eeprom_dev. Legacy likes any address.
		 */
		max_wait_time = 100;
		while (qib_twsi_wr(dd, dev | READ_CMD, QIB_TWSI_START)) {
			stop_cmd(dd);
			if (!--max_wait_time)
				goto failed_write;
		}
		/* now read (and ignore) the resulting byte */
		rd_byte(dd, 1);
	}

	ret = 0;
	goto bail;

failed_write:
	stop_cmd(dd);
	ret = 1;

bail:
	return ret;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * Copyright (c) 2008, 2009, 2010 QLogic Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/moduleparam.h>

#include "qib.h"

static unsigned qib_hol_timeout_ms = 3000;
module_param_named(hol_timeout_ms, qib_hol_timeout_ms, uint, S_IRUGO);
MODULE_PARM_DESC(hol_timeout_ms,
		 "duration of user app suspension after link failure");

unsigned qib_sdma_fetch_arb = 1;
module_param_named(fetch_arb, qib_sdma_fetch_arb, uint, S_IRUGO);
MODULE_PARM_DESC(fetch_arb, "IBA7220: change SDMA descriptor arbitration");

/**
 * qib_disarm_piobufs - cancel a range of PIO buffers
 * @dd: the qlogic_ib device
 * @first: the first PIO buffer to cancel
 * @cnt: the number of PIO buffers to cancel
 *
 * Cancel a range of PIO buffers. Used at user process close,
 * in case it died while writing to a PIO buffer.
 */
void qib_disarm_piobufs(struct qib_devdata *dd, unsigned first, unsigned cnt)
{
	unsigned long flags;
	unsigned i;
	unsigned last;

	last = first + cnt;
	spin_lock_irqsave(&dd->pioavail_lock, flags);
	for (i = first; i < last; i++) {
		__clear_bit(i, dd->pio_need_disarm);
		dd->f_sendctrl(dd->pport, QIB_SENDCTRL_DISARM_BUF(i));
	}
	spin_unlock_irqrestore(&dd->pioavail_lock, flags);
}

/*
 * This is called by a user process when it sees the DISARM_BUFS event
 * bit is set.
 */
int qib_disarm_piobufs_ifneeded(struct qib_ctxtdata *rcd)
{
	struct qib_devdata *dd = rcd->dd;
	unsigned i;
	unsigned last;
	unsigned n = 0;

	last = rcd->pio_base + rcd->piocnt;
	/*
	 * Don't need uctxt_lock here, since user has called in to us.
	 * Clear at start in case more interrupts set bits while we
	 * are disarming
	 */
	if (rcd->user_event_mask) {
		/*
		 * subctxt_cnt is 0 if not shared, so do base
		 * separately, first, then remaining subctxt, if any
		 */
		clear_bit(_QIB_EVENT_DISARM_BUFS_BIT, &rcd->user_event_mask[0]);
		for (i = 1; i < rcd->subctxt_cnt; i++)
			clear_bit(_QIB_EVENT_DISARM_BUFS_BIT,
				  &rcd->user_event_mask[i]);
	}
	spin_lock_irq(&dd->pioavail_lock);
	for (i = rcd->pio_base; i < last; i++) {
		if (__test_and_clear_bit(i, dd->pio_need_disarm)) {
			n++;
			dd->f_sendctrl(rcd->ppd, QIB_SENDCTRL_DISARM_BUF(i));
		}
	}
	spin_unlock_irq(&dd->pioavail_lock);
	return 0;
}

static struct qib_pportdata *is_sdma_buf(struct qib_devdata *dd, unsigned i)
{
	struct qib_pportdata *ppd;
	unsigned pidx;

	for (pidx = 0; pidx < dd->num_pports; pidx++) {
		ppd = dd->pport + pidx;
		if (i >= ppd->sdma_state.first_sendbuf &&
		    i < ppd->sdma_state.last_sendbuf)
			return ppd;
	}
	return NULL;
}

/*
 * Return true if send buffer is being used by a user context.
 * Sets  _QIB_EVENT_DISARM_BUFS_BIT in user_event_mask as a side ef