/*
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

/*
 * This file contains support for diagnostic functions.  It is accessed by
 * opening the qib_diag device, normally minor number 129.  Diagnostic use
 * of the QLogic_IB chip may render the chip or board unusable until the
 * driver is unloaded, or in some cases, until the system is rebooted.
 *
 * Accesses to the chip through this interface are not similar to going
 * through the /sys/bus/pci resource mmap interface.
 */

#include <linux/io.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "qib.h"
#include "qib_common.h"

#undef pr_fmt
#define pr_fmt(fmt) QIB_DRV_NAME ": " fmt

/*
 * Each client that opens the diag device must read then write
 * offset 0, to prevent lossage from random cat or od. diag_state
 * sequences this "handshake".
 */
enum diag_state { UNUSED = 0, OPENED, INIT, READY };

/* State for an individual client. PID so children cannot abuse handshake */
static struct qib_diag_client {
	struct qib_diag_client *next;
	struct qib_devdata *dd;
	pid_t pid;
	enum diag_state state;
} *client_pool;

/*
 * Get a client struct. Recycled if possible, else kmalloc.
 * Must be called with qib_mutex held
 */
static struct qib_diag_client *get_client(struct qib_devdata *dd)
{
	struct qib_diag_client *dc;

	dc = client_pool;
	if (dc)
		/* got from pool remove it and use */
		client_pool = dc->next;
	else
		/* None in pool, alloc and init */
		dc = kmalloc(sizeof(*dc), GFP_KERNEL);

	if (dc) {
		dc->next = NULL;
		dc->dd = dd;
		dc->pid = current->pid;
		dc->state = OPENED;
	}
	return dc;
}

/*
 * Return to pool. Must be called with qib_mutex held
 */
static void return_client(struct qib_diag_client *dc)
{
	struct qib_devdata *dd = dc->dd;
	struct qib_diag_client *tdc, *rdc;

	rdc = NULL;
	if (dc == dd->diag_client) {
		dd->diag_client = dc->next;
		rdc = dc;
	} else {
		tdc = dc->dd->diag_client;
		while (tdc) {
			if (dc == tdc->next) {
				tdc->next = dc->next;
				rdc = dc;
				break;
			}
			tdc = tdc->next;
		}
	}
	if (rdc) {
		rdc->state = UNUSED;
		rdc->dd = NULL;
		rdc->pid = 0;
		rdc->next = client_pool;
		client_pool = rdc;
	}
}

static int qib_diag_open(struct inode *in, struct file *fp);
static int qib_diag_release(struct inode *in, struct file *fp);
static ssize_t qib_diag_read(struct file *fp, char __user *data,
			     size_t count, loff_t *off);
static ssize_t qib_diag_write(struct file *fp, const char __user *data,
			      size_t count, loff_t *off);

static const struct file_operations diag_file_ops = {
	.owner = THIS_MODULE,
	.write = qib_diag_write,
	.read = qib_diag_read,
	.open = qib_diag_open,
	.release = qib_diag_release,
	.llseek = default_llseek,
};

static atomic_t diagpkt_count = ATOMIC_INIT(0);
static struct cdev *diagpkt_cdev;
static struct device *diagpkt_device;

static ssize_t qib_diagpkt_write(struct file *fp, const char __user *data,
				 size_t count, loff_t *off);

static const struct file_operations diagpkt_file_ops = {
	.owner = THIS_MODULE,
	.write = qib_diagpkt_write,
	.llseek = noop_llseek,
};

int qib_diag_add(struct qib_devdata *dd)
{
	char name[16];
	int ret = 0;

	if (atomic_inc_return(&diagpkt_count) == 1) {
		ret = qib_cdev_init(QIB_DIAGPKT_MINOR, "ipath_diagpkt",
				    &diagpkt_file_ops, &diagpkt_cdev,
				    &diagpkt_device);
		if (ret)
			goto done;
	}

	snprintf(name, sizeof(name), "ipath_diag%d", dd->unit);
	ret = qib_cdev_init(QIB_DIAG_MINOR_BASE + dd->unit, name,
			    &diag_file_ops, &dd->diag_cdev,
			    &dd->diag_device);
done:
	return ret;
}

static void qib_unregister_observers(struct qib_devdata *dd);

void qib_diag_remove(struct qib_devdata *dd)
{
	struct qib_diag_client *dc;

	if (atomic_dec_and_test(&diagpkt_count))
		qib_cdev_cleanup(&diagpkt_cdev, &diagpkt_device);

	qib_cdev_cleanup(&dd->diag_cdev, &dd->diag_device);

	/*
	 * Return all diag_clients of this device. There should be none,
	 * as we are "guaranteed" that no clients are still open
	 */
	while (dd->diag_client)
		return_client(dd->diag_client);

	/* Now clean up all unused client structs */
	while (client_pool) {
		dc = client_pool;
		client_pool = dc->next;
		kfree(dc);
	}
	/* Clean up observer list */
	qib_unregister_observers(dd);
}

/* qib_remap_ioaddr32 - remap an offset into chip address space to __iomem *
 *
 * @dd: the qlogic_ib device
 * @offs: the offset in chip-space
 * @cntp: Pointer to max (byte) count for transfer starting at offset
 * This returns a u32 __iomem * so it can be used for both 64 and 32-bit
 * mapping. It is needed because with the use of PAT for control of
 * write-combining, the logically contiguous address-space of the chip
 * may be split into virtually non-contiguous spaces, with different
 * attributes, which are them mapped to contiguous physical space
 * based from the first BAR.
 *
 * The code below makes the same assumptions as were made in
 * init_chip_wc_pat() (qib_init.c), copied here:
 * Assumes chip address space looks like:
 *		- kregs + sregs + cregs + uregs (in any order)
 *		- piobufs (2K and 4K bufs in either order)
 *	or:
 *		- kregs + sregs + cregs (in any order)
 *		- piobufs (2K and 4K bufs in either order)
 *		- uregs
 *
 * If cntp is non-NULL, returns how many bytes from offset can be accessed
 * Returns 0 if the offset is not mapped.
 */
static u32 __iomem *qib_remap_ioaddr32(struct qib_devdata *dd, u32 offset,
				       u32 *cntp)
{
	u32 kreglen;
	u32 snd_bottom, snd_lim = 0;
	u32 __iomem *krb32 = (u32 __iomem *)dd->kregbase;
	u32 __iomem *map = NULL;
	u32 cnt = 0;
	u32 tot4k, offs4k;

	/* First, simplest case, offset is within the first map. */
	kreglen = (dd->kregend - dd->kregbase) * sizeof(u64);
	if (offset < kreglen) {
		map = krb32 + (offset / sizeof(u32));
		cnt = kreglen - offset;
		goto mapped;
	}

	/*
	 * Next check for user regs, the next most common case,
	 * and a cheap check because if they are not in the first map
	 * they are last in chip.
	 */
	if (dd->userbase) {
		/* If user regs mapped, they are after send, so set limit. */
		u32 ulim = (dd->cfgctxts * dd->ureg_align) + dd->uregbase;

		if (!dd->piovl15base)
			snd_lim = dd->uregbase;
		krb32 = (u32 __iomem *)dd->userbase;
		if (offset >= dd->uregbase && offset < ulim) {
			map = krb32 + (offset - dd->uregbase) / sizeof(u32);
			cnt = ulim - offset;
			goto mapped;
		}
	}

	/*
	 * Lastly, check for offset within Send Buffers.
	 * This is gnarly because struct devdata is deliberately vague
	 * about things like 7322 VL15 buffers, and we are not in
	 * chip-specific code here, so should not make many assumptions.
	 * The one we _do_ make is that the only chip that has more sndbufs
	 * than we admit is the 7322, and it has userregs above that, so
	 * we know the snd_lim.
	 */
	/* Assume 2K buffers are first. */
	snd_bottom = dd->pio2k_bufbase;
	if (snd_lim == 0) {
		u32 tot2k = dd->piobcnt2k * ALIGN(dd->piosize2k, dd->palign);

		snd_lim = snd_bottom + tot2k;
	}
	/* If 4k buffers exist, account for them by bumping
	 * appropriate limit.
	 */
	tot4k = dd->piobcnt4k * dd->align4k;
	offs4k = dd->piobufbase >> 32;
	if (dd->piobcnt4k) {
		if (snd_bottom > offs4k)
			snd_bottom = offs4k;
		else {
			/* 4k above 2k. Bump snd_lim, if needed*/
			if (!dd->userbase || dd->piovl15base)
				snd_lim = offs4k + tot4k;
		}
	}
	/*
	 * Judgement call: can we ignore the space between SendBuffs and
	 * UserRegs, where we would like to see vl15 buffs, but not more?
	 */
	if (offset >= snd_bottom && offset < snd_lim) {
		offset -= snd_bottom;
		map = (u32 __iomem *)dd->piobase + (offset / sizeof(u32));
		cnt = snd_lim - offset;
	}

	if (!map && offs4k && dd->piovl15base) {
		snd_lim = offs4k + tot4k + 2 * dd->align4k;
		if (offset >= (offs4k + tot4k) && offset < snd_lim) {
			map = (u32 __iomem *)dd->piovl15base +
				((offset - (offs4k + tot4k)) / sizeof(u32));
			cnt = snd_lim - offset;
		}
	}

mapped:
	if (cntp)
		*cntp = cnt;
	return map;
}

/*
 * qib_read_umem64 - read a 64-bit quantity from the chip into user space
 * @dd: the qlogic_ib device
 * @uaddr: the location to store the data in user memory
 * @regoffs: the offset from BAR0 (_NOT_ full pointer, anymore)
 * @count: number of bytes to copy (multiple of 32 bits)
 *
 * This function also localizes all chip memory accesses.
 * The copy should be written such that we read full cacheline packets
 * from the chip.  This is usually used for a single qword
 *
 * NOTE:  This assumes the chip address is 64-bit aligned.
 */
static int qib_read_umem64(struct qib_devdata *dd, void __user *uaddr,
			   u32 regoffs, size_t count)
{
	const u64 __iomem *reg_addr;
	const u64 __iomem *reg_end;
	u32