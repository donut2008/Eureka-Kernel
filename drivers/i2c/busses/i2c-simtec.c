/*
 * Copyright (c) 2006, 2007, 2008, 2009 QLogic Corporation. All rights reserved.
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
#include "qib_qsfp.h"

/*
 * QSFP support for ib_qib driver, using "Two Wire Serial Interface" driver
 * in qib_twsi.c
 */
#define QSFP_MAX_RETRY 4

static int qsfp_read(struct qib_pportdata *ppd, int addr, void *bp, int len)
{
	struct qib_devdata *dd = ppd->dd;
	u32 out, mask;
	int ret, cnt, pass = 0;
	int stuck = 0;
	u8 *buff = bp;

	ret = mutex_lock_interruptible(&dd->eep_lock);
	if (ret)
		goto no_unlock;

	if (dd->twsi_eeprom_dev == QIB_TWSI_NO_DEV) {
		ret = -ENXIO;
		goto bail;
	}

	/*
	 * We presume, if we are called at all, that this board has
	 * QSFP. This is on the same i2c chain as the legacy parts,
	 * but only responds if the module is selected via GPIO pins.
	 * Further, there are very long setup and hold requirements
	 * on MODSEL.
	 */
	mask = QSFP_GPIO_MOD_SEL_N | QSFP_GPIO_MOD_RST_N | QSFP_GPIO_LP_MODE;
	out = QSFP_GPIO_MOD_RST_N | QSFP_GPIO_LP_MODE;
	if (ppd->hw_pidx) {
		mask <<= QSFP_GPIO_PORT2_SHIFT;
		out <<= QSFP_GPIO_PORT2_SHIFT;
	}

	dd->f_gpio_mod(dd, out, mask, mask);

	/*
	 * Module could take up to 2 Msec to respond to MOD_SEL, and there
	 * is no way to tell if it is ready, so we must wait.
	 */
	msleep(20);

	/* Make sure TWSI bus is in sane state. */
	ret = qib_twsi_reset(dd);
	if (ret) {
		qib_dev_porterr(dd, ppd->port,
				"QSFP interface Reset for read failed\n");
		ret = -EIO;
		stuck = 1;
		goto deselect;
	}

	/* All QSFP modules are at A0 */

	cnt = 0;
	while (cnt < len) {
		unsigned in_page;
		int wlen = len - cnt;

		in_page = addr % QSFP_PAGESIZE;
		if ((in_page + wlen) > QSFP_PAGESIZE)
			wlen = QSFP_PAGESIZE - in_page;
		ret = qib_twsi_blk_rd(dd, QSFP_DEV, addr, buff + cnt, wlen);
		/* Some QSFP's fail first try. Retry as experiment */
		if (ret && cnt == 0 && ++pass < QSFP_MAX_RETRY)
			continue;
		if (ret) {
			/* qib_twsi_blk_rd() 1 for error, else 0 */
			ret = -EIO;
			goto deselect;
		}
		addr += wlen;
		cnt += wlen;
	}
	ret = cnt;

deselect:
	/*
	 * Module could take up to 10 uSec after transfer before
	 * read