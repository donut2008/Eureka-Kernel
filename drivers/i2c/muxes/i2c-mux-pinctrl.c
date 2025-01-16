/*
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
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

#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/prefetch.h>

#include "qib.h"

/*
 * The size has to be longer than this string, so we can append
 * board/chip information to it in the init code.
 */
const char ib_qib_version[] = QIB_DRIVER_VERSION "\n";

DEFINE_SPINLOCK(qib_devs_lock);
LIST_HEAD(qib_dev_list);
DEFINE_MUTEX(qib_mutex);	/* general driver use */

unsigned qib_ibmtu;
module_param_named(ibmtu, qib_ibmtu, uint, S_IRUGO);
MODULE_PARM_DESC(ibmtu, "Set max IB MTU (0=2KB, 1=256, 2=512, ... 5=4096");

unsigned qib_compat_ddr_negotiate = 1;
module_param_named(compat_ddr_negotiate, qib_compat_ddr_negotiate, uint,
		   S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(compat_ddr_negotiate,
		 "Attempt pre-IBTA 1.2 DDR speed negotiation");

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel <ibsupport@intel.com>");
MODULE_DESCRIPTION("Intel IB driver");
MODULE_VERSION(QIB_DRIVER_VERSION);

/*
 * QIB_PIO_MAXIBHDR is the max IB header size allowed for in our
 * PIO send buffers.  This is well beyond anything currently
 * defined in the InfiniBand spec.
 */
#define QIB_PIO_MAXIBHDR 128

/*
 * QIB_MAX_PKT_RCV is the max # if packets processed per receive interrupt.
 */
#define QIB_MAX_PKT_RECV 64

struct qlogic_ib_stats qib_stats;

const char *qib_get_unit_name(int unit)
{
	static char iname[16];

	snprintf(iname, sizeof(iname), "infinipath%u", unit);
	return iname;
}

/*
 * Return count of units with at least one port ACTIVE.
 */
int qib_count_active_units(void)
{
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	unsigned long flags;
	int pidx, nunits_active = 0;

	spin_lock_irqsave(&qib_devs_lock, flags);
	list_for_each_entry(dd, &qib_dev_list, list) {
		if (!(dd->flags & QIB_PRESENT) || !dd->kregbase)
			continue;
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;
			if (ppd->lid && (ppd->lflags & (QIBL_LINKINIT |
					 QIBL_LINKARMED | QIBL_LINKACTIVE))) {
				nunits_active++;
				break;
			}
		}
	}
	spin_unlock_irqrestore(&qib_devs_lock, flags);
	return nunits_active;
}

/*
 * Return count of all units, optionally return in arguments
 * the number of usable (present) units, and the number of
 * ports that are up.
 */
int qib_count_units(int *npresentp, int *nupp)
{
	int nunits = 0, npresent = 0, nup = 0;
	struct qib_devdata *dd;
	unsigned long flags;
	int pidx;
	struct qib_pportdata *ppd;

	spin_lock_irqsave(&qib_devs_lock, flags);

	list_for_each_entry(dd, &qib_dev_list, list) {
		nunits++;
		if ((dd->flags & QIB_PRESENT) && dd->kregbase)
			npresent++;
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;
			if (ppd->lid && (ppd->lflags & (QIBL_LINKINIT |
					 QIBL_LINKARMED | QIBL_LINKACTIVE)))
				nup++;
		}
	}

	spin_unlock_irqrestore(&qib_devs_lock, flags);

	if (npresentp)
		*npresentp = npresent;
	if (nupp)
		*nupp = nup;

	return nunits;
}

/**
 * qib_wait_linkstate - wait for an IB link state change to occur
 * @dd: the qlogic_ib device
 * @state: the state to wait for
 * @msecs: the number of milliseconds to wait
 *
 * wait up to msecs milliseconds for IB link state change to occur for
 * now, take the easy polling route.  Currently used only by
 * qib_set_linkstate.  Returns 0 if state reached, otherwise
 * -ETIMEDOUT state can have multiple states set, for any of several
 * transitions.
 */
int qib_wait_linkstate(struct qib_pportdata *ppd, u32 state, int msecs)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&ppd->lflags_lock, flags);
	if (ppd->state_wanted) {
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
		ret = -EBUSY;
		goto bail;
	}
	ppd->state_wanted = state;
	spin_unlock_irqrestore(&ppd->lflags_lock, flags);
	wait_event_interruptible_timeout(ppd->state_wait,
					 (ppd->lflags & state),
					 msecs_to_jiffies(msecs));
	spin_lock_irqsave(&ppd->lflags_lock, flags);
	ppd->state_wanted = 0;
	spin_unlock_irqrestore(&ppd->lflags_lock, flags);

	if (!(ppd->lflags & state))
		ret = -ETIMEDOUT;
	else
		ret = 0;
bail:
	return ret;
}

int qib_set_linkstate(struct qib_pportdata *ppd, u8 newstate)
{
	u32 lstate;
	int ret;
	struct qib_devdata *dd = ppd->dd;
	unsigned long flags;

	switch (newstate) {
	case QIB_IB_LINKDOWN_ONLY:
		dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LSTATE,
				 IB_LINKCMD_DOWN | IB_LINKINITCMD_NOP);
		/* don't wait */
		ret = 0;
		goto bail;

	case QIB_IB_LINKDOWN:
		dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LSTATE,
				 IB_LINKCMD_DOWN | IB_LINKINITCMD_POLL);
		/* don't wait */
		ret = 0;
		goto bail;

	case QIB_IB_LINKDOWN_SLEEP:
		dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LSTATE,
				 IB_LINKCMD_DOWN | IB_LINKINITCMD_SLEEP);
		/* don't wait */
		ret = 0;
		goto bail;

	case QIB_IB_LINKDOWN_DISABLE:
		dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LSTATE,
				 IB_LINKCMD_DOWN | IB_LINKINITCMD_DISABLE);
		/* don't wait */
		ret = 0;
		goto bail;

	case QIB_IB_LINKARM:
		if (ppd->lflags & QIBL_LINKARMED) {
			ret = 0;
			goto bail;
		}
		if (!(ppd->lflags & (QIBL_LINKINIT | QIBL_LINKACTIVE))) {
			ret = -EINVAL;
			goto bail;
		}
		/*
		 * Since the port can be ACTIVE when we ask for ARMED,
		 * clear QIBL_LINKV so we can wait for a transition.
		 * If the link isn't ARMED, then something else happened
		 * and there is no point waiting for ARMED.
		 */
		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags &= ~QIBL_LINKV;
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
		dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LSTATE,
				 IB_LINKCMD_ARMED | IB_LINKINITCMD_NOP);
		lstate = QIBL_LINKV;
		break;

	case QIB_IB_LINKACTI