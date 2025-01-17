evdata *dd = ppd->dd;
	u64 fmask;

	if (dd->cspec->recovery_ports_initted != 1)
		return; /* rest doesn't apply to dualport */
	qib_write_kreg(dd, kr_control, dd->control |
		       SYM_MASK(Control, FreezeMode));
	(void)qib_read_kreg64(dd, kr_scratch);
	udelay(3); /* ibcreset asserted 400ns, be sure that's over */
	fmask = qib_read_kreg64(dd, kr_act_fmask);
	if (!fmask) {
		/*
		 * require a powercycle before we'll work again, and make
		 * sure we get no more interrupts, and don't turn off
		 * freeze.
		 */
		ppd->dd->cspec->stay_in_freeze = 1;
		qib_7322_set_intr_state(ppd->dd, 0);
		qib_write_kreg(dd, kr_fmask, 0ULL);
		qib_dev_err(dd, "HCA unusable until powercycled\n");
		return; /* eventually reset */
	}

	qib_write_kreg(ppd->dd, kr_hwerrclear,
	    SYM_MASK(HwErrClear, IBSerdesPClkNotDetectClear_1));

	/* don't do the full clear_freeze(), not needed for this */
	qib_write_kreg(dd, kr_control, dd->control);
	qib_read_kreg32(dd, kr_scratch);
	/* take IBC out of reset */
	if (ppd->link_speed_supported) {
		ppd->cpspec->ibcctrl_a &=
			~SYM_MASK(IBCCtrlA_0, IBStatIntReductionEn);
		qib_write_kreg_port(ppd, krp_ibcctrl_a,
				    ppd->cpspec->ibcctrl_a);
		qib_read_kreg32(dd, kr_scratch);
		if (ppd->lflags & QIBL_IB_LINK_DISABLED)
			qib_set_ib_7322_lstate(ppd, 0,
				QLOGIC_IB_IBCC_LINKINITCMD_DISABLE);
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Copyright (c) 2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2005, 2006 PathScale, Inc. All rights reserved.
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

#include <rdma/ib_smi.h>

#include "qib.h"
#include "qib_mad.h"

static int reply(struct ib_smp *smp)
{
	/*
	 * The verbs framework will handle the directed/LID route
	 * packet changes.
	 */
	smp->method = IB_MGMT_METHOD_GET_RESP;
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
		smp->status |= IB_SMP_DIRECTION;
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
}

static int reply_failure(struct ib_smp *smp)
{
	/*
	 * The verbs framework will handle the directed