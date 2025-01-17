/*
 * Copyright (c) 2006, 2007, 2008, 2009, 2010 QLogic Corporation.
 * All rights reserved.
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

#include <linux/pci.h>
#include <linux/delay.h>

#include "qib.h"
#include "qib_common.h"

/**
 * qib_format_hwmsg - format a single hwerror message
 * @msg message buffer
 * @msgl length of message buffer
 * @hwmsg message to add to message buffer
 */
static void qib_format_hwmsg(char *msg, size_t msgl, const char *hwmsg)
{
	strlcat(msg, "[", msgl);
	strlcat(msg, hwmsg, msgl);
	strlcat(msg, "]", msgl);
}

/**
 * qib_format_hwerrors - format hardware error messages for display
 * @hwerrs hardware errors bit vector
 * @hwerrmsgs hardware error descriptions
 * @nhwerrmsgs number of hwerrmsgs
 * @msg message buffer
 * @msgl message buffer length
 */
void qib_format_hwerrors(u64 hwerrs, const struct qib_hwerror_msgs *hwerrmsgs,
			 size_t nhwerrmsgs, char *msg, size_t msgl)
{
	int i;

	for (i = 0; i < nhwerrmsgs; i++)
		if (hwerrs & hwerrmsgs[i].mask)
			qib_format_hwmsg(msg, msgl, hwerrmsgs[i].msg);
}

static void signal_ib_event(struct qib_pportdata *ppd, enum ib_event_type ev)
{
	struct ib_event event;
	struct qib_devdata *dd = ppd->dd;

	event.device = &dd->verbs_dev.ibdev;
	event.element.port_num = ppd->port;
	event.event = ev;
	ib_dispatch_event(&event);
}

void qib_handle_e_ibstatuschanged(struct qib_pportdata *ppd, u64 ibcs)
{
	struct qib_devdata *dd = ppd->dd;
	unsigned long flags;
	u32 lstate;
	u8 ltstate;
	enum ib_event_type ev = 0;

	lstate = dd->f_iblink_state(ibcs); /* linkstate */
	ltstate = dd->f_ibphys_portstate(ibcs);

	/*
	 * If linkstate transitions into INIT from any of the various down
	 * states, or if it transitions from any of the up (INIT or better)
	 * states into any of the down states (except link recovery), then
	 * call the chip-specific code to take appropriate actions.
	 *
	 * ppd->lflags could be 0 if this is the first time the interrupt
	 * handlers has been called but the link is already up.
	 */
	if (lstate >= IB_PORT_INIT &&
	    (!ppd->lflags || (ppd->lflags & QIBL_LINKDOWN)) &&
	    ltstate == IB_PHYSPORTSTATE_LINKUP) {
		/* transitioned to UP */
		if (dd->f_ib_updown(ppd, 1, ibcs))
			goto skip_ibchange; /* chip-code handled */
	} else if (ppd->lflags & (QIBL_LINKINIT | QIBL_LINKARMED |
		   QIBL_LINKACTIVE | QIBL_IB_FORCE_NOTIFY)) {
		if (ltstate != IB_PHYSPORTSTATE_LINKUP &&
		    ltstate <= IB_PHYSPORTSTATE_CFG_TRAIN &&
		    dd->f_ib_updown(ppd, 0, ibcs))
			goto skip_ibchange; /* chip-code handled */
		qib_set_uevent_bits(ppd, _QIB_EVENT_LINKDOWN_BIT);
	}

	if (lstate != IB_PORT_DOWN) {
		/* lstate is INIT, ARMED, or ACTIVE */
		if (lstate != IB_PORT_ACTIVE) {
			*ppd->statusp &= ~QIB_STATUS_IB_READY;
			if (ppd->lflags & QIBL_LINKACTIVE)
				ev = IB_EVENT_PORT_ERR;
			spin_lock_irqsave(&ppd->lflags_lock, flags);
			if (lstate == IB_PORT_ARMED) {
				ppd->lflags |= QIBL_LINKARMED | QIBL_LINKV;
				ppd->lflags &= ~(QIBL_LINKINIT |
					QIBL_LINKDOWN | QIBL_LINKACTIVE);
			} else {
				ppd->lflags |= QIBL_LINKINIT | QIBL_LINKV;
				ppd->lflags &= ~(QIBL_LINKARMED |
					QIBL_LINKDOWN | QIBL_LINKACTIVE);
			}
			spin_unlock_irqrestore(&ppd->lflags_lock, flags);
			/* start a 75msec timer to clear symbol errors */
			mod_timer(&ppd->symerr_clear_timer,
				  msecs_to_jiffies(75));
		} else if (ltstate == IB_PHYSPORTSTATE_LINKUP &&
			   !(ppd->lflags & QIBL_LINKACTIVE)) {
			/* active, but not active defered */
			qib_hol_up(ppd); /* useful only for 6120 now */
			*ppd->statusp |=
				QIB_STATUS_IB_READY | QIB_STATUS_IB_CONF;
			qib_clear_symerror_on_linkup((unsigned long)ppd);
			spin_lock_irqsave(&ppd->lflags_lock, flags);
			ppd->lflags |= QIBL_LINKACTIVE | QIBL_LINKV;
			ppd->lflags &= ~(QIBL_LINKINIT |
				QIBL_LINKDOWN | QIBL_LINKARMED);
			spin_unlock_irqrestore(&ppd->lflags_lock, flags);
			if (dd->flags & QIB_HAS_SEND_DMA)
				qib_sdma_process_event(ppd,
					qib_sdma_event_e30_go_running);
			ev = IB_EVENT_PORT_ACTIVE;
			dd->f_setextled(ppd, 1);
		}
	} else { /* down */
		if (ppd->lflags & QIBL_LINKACTIVE)
			ev = IB_EVENT_PORT_ERR;
		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags |= QIBL_LINKDOWN | QIBL_LINKV;
		ppd->lflags &= ~(QIBL_LINKINIT |
				 QIBL_LINKACTIVE | QIBL_LINKARMED);
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
		*ppd->statusp &= ~QIB_STATUS_IB_READY;
	}

skip_ibchange:
	ppd->lastibcstat = ibcs;
	if (ev)
		signal_ib_event(ppd, ev);
}

void qib_clear_symerror_on_linkup(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)opaque;

	if (ppd->lflags & QIBL_LINKACTIVE)
		return;

	ppd->ibport_data.z_symbol_error_counter =
		ppd->dd->f_portcntr(ppd, QIBPORTCNTR_IBSYMBOLERR);
}

/*
 * Handle receive interrupts for user ctxts; this means a user
 * process was waiting for a packet to arrive, and didn't want
 * to poll.
 */
void qib_handle_urcv(struct qib_devdata *dd, u64 ctxtr)
{
	struct qib_ctxtdata *rcd;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&dd->uctxt_lock, flags);
	for (i = dd->first_user_ctxt; dd->rcd && i < dd->cfgctxts; i++) {
		if (!(ctxtr & (1ULL << i)))
			continue;
		rcd = dd->rcd[i];
		if (!rcd || !rcd->cnt)
			continue;

		if (test_and_clear_bit(QIB_CTXT_WAITING_RCV, &rcd->flag)) {
			wake_up_interruptible(&rcd->wait);
			dd->f_rcvctrl(rcd->ppd, QIB_RCVCTRL_INTRAVAIL_DIS,
				      rcd->ctxt);
		} else if (test_and_clear_bit(QIB_CTXT_WAITING_URG,
					      &rcd->flag)) {
			rcd->urgent++;
			wake_up_interruptible(&rcd->wait);
		}
	}
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);
}

void qib_bad_intrstatus(struct qib_devdata *dd)
{
	static int allbits;

	/* separate routine, for better optimization of qib_intr() */

	/*
	 * We print the message and disable interrupts, in hope of
	 * having a better chance of debugging the problem.
	 */
	qib_dev_err(dd,
		"Read of chip interrupt status failed disabling interrupts\n");
	if (allbits++) {
		/* disable interrupt delivery, something is very wrong */
		if (allbits == 2)
			dd->f_set_intr_state(dd, 0);
		if (allbits == 3) {
			qib_dev_err(dd,
				"2nd bad interrupt status, unregistering interrupts\n");
			dd->flags |= QIB_BADINTR;
			dd->flags &= ~QIB_INITTED;
			dd->f_free_irq(dd);
		}
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * Copyright (c) 2006, 2007, 2009 QLogic Corporation. All rights reserved.
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

#include "qib.h"

/**
 * qib_alloc_lkey - allocate an lkey
 * @mr: memory region that this lkey protects
 * @dma_region: 0->normal key, 1->restricted DMA key
 *
 * Returns 0 if successful, otherwise returns -errno.
 *
 * Increments mr reference count as required.
 *
 * Sets the lkey field mr for non-dma regions.
 *
 */

int qib_alloc_lkey(struct qib_mregion *mr, int dma_region)
{
	unsigned long flags;
	u32 r;
	u32 n;
	int ret = 0;
	struct qib_ibdev *dev = to_idev(mr->pd->device);
	struct qib_lkey_table *rkt = &dev->lk_table;

	spin_lock_irqsave(&rkt->lock, flags);

	/* special case for dma_mr lkey == 0 */
	if (dma_region) {
		struct qib_mregion *tmr;

		tmr = rcu_access_pointer(dev->dma_mr);
		if (!tmr) {
			qib_get_mr(mr);
			rcu_assign_pointer(dev->dma_mr, mr);
			mr->lkey_published = 1;
		}
		goto success;
	}

	/* Find the next available LKEY */
	r = rkt->next;
	n = r;
	for (;;) {
		if (rkt->table[r] == NULL)
			break;
		r = (r + 1) & (rkt->max - 1);
		if (r == n)
			goto bail;
	}
	rkt->next = (r + 1) & (rkt->max - 1);
	/*
	 * Make sure lkey is never zero which is reserved to indicate an
	 * unrestricted LKEY.
	 */
	rkt->gen++;
	/*
	 * bits are capped in qib_verbs.c to insure enough bits
	 * for generation number
	 */
	mr->lkey = (r << (32 - ib_qib_lkey_table_size)) |
		((((1 << (24 - ib_qib_lkey_table_size)) - 1) & rkt->gen)
		 << 8);
	if (mr->lkey == 0) {
		mr->lkey |= 1 << 8;
		rkt->gen++;
	}
	qib_get_mr(mr);
	rcu_assign_pointer(rkt->table[r], mr);
	mr->lkey_published = 1;
success:
	spin_unlock_irqrestore(&rkt->lock, flags);
out:
	return ret;
bail:
	spin_unlock_irqrestore(&rkt->lock, flags);
	ret = -ENOMEM;
	goto out;
}

/**
 * qib_free_lkey - free an lkey
 * @mr: mr to free from tables
 */
void qib_free_lkey(struct qib_mregion *mr)
{
	unsigned long flags;
	u32 lkey = mr->lkey;
	u32 r;
	struct qib_ibdev *dev = to_idev(mr->pd->device);
	struct qib_lkey_table *rkt = &dev->lk_table;

	spin_lock_irqsave(&rkt->lock, flags);
	if (!mr->lkey_published)
		goto out;
	if (lkey == 0)
		RCU_INIT_POINTER(dev->dma_mr, NULL);
	else {
		r = lkey >> (32 - ib_qib_lkey_table_size);
		RCU_INIT_POINTER(rkt->table[r], NULL);
	}
	qib_put_mr(mr);
	mr->lkey_published = 0;
out:
	spin_unlock_irqrestore(&rkt->lock, flags);
}

/**
 * qib_lkey_ok - check IB SGE for validity and initialize
 * @rkt: table containing lkey to check SGE against
 * @pd: protection domain
 * @isge: outgoing internal SGE
 * @sge: SGE to check
 * @acc: access flags
 *
 * Return 1 if valid and successful, otherwise returns 0.
 *
 * increments the reference count upon success
 *
 * Check the IB SGE for validity and initialize our internal version
 * of it.
 */
int qib_lkey_ok(struct qib_lkey_table *rkt, struct qib_pd *pd,
		struct qib_sge *isge, struct ib_sge *sge, int acc)
{
	struct qib_mregion *mr;
	unsigned n, m;
	size_t off;

	/*
	 * We use LKEY == zero for kernel virtual addresses
	 * (see qib_get_dma_mr and qib_dma.c).
	 */
	rcu_read_lock();
	if (sge->lkey == 0) {
		struct qib_ibdev *dev = to_idev(pd->ibpd.device);

		if (pd->user)
			goto bail;
		mr = rcu_dereference(dev->dma_mr);
		if (!mr)
			goto bail;
		if (unlikely(!atomic_inc_not_zero(&mr->refcount)))
			goto bail;
		rcu_read_unlock();

		isge->mr = mr;
		isge->vaddr = (void *) sge->addr;
		isge->length = sge->length;
		isge->sge_length = sge->length;
		isge->m = 0;
		isge->n = 0;
		goto ok;
	}
	mr = rcu_dereference(
		rkt->table[(sge->lkey >> (32 - ib_qib_lkey_table_size))]);
	if (unlikely(!mr || mr->lkey != sge->lkey || mr->pd != &pd->ibpd))
		goto bail;

	off = sge->addr - mr->user_base;
	if (unlikely(sge->addr < mr->user_base ||
		     off + sge->length > mr->length ||
		     (mr->access_flags & acc) != acc))
		goto bail;
	if (unlikely(!atomic_inc_not_zero(&mr->refcount)))
		goto bail;
	rcu_read_unlock();

	off += mr->offset;
	if (mr->page_shift) {
		/*
		page sizes are uniform power of 2 so no loop is necessary
		entries_spanned_by_off is the number of times the loop below
		would have executed.
		*/
		size_t entries_spanned_by_off;

		entries_spanned_by_off = off >> mr->page_shift;
		off -= (entries_spanned_by_off << mr->page_shift);
		m = entries_spanned_by_off/QIB_SEGSZ;
		n = entries_spanned_by_off%QIB_SEGSZ;
	} else {
		m = 0;
		n = 0;
		while (off >= mr->map[m]->segs[n].length) {
			off -= mr->map[m]->segs[n].length;
			n++;
			if (n >= QIB_SEGSZ) {
				m++;
				n = 0;
			}
		}
	}
	isge->mr = mr;
	isge->vaddr = mr->map[m]->segs[n].vaddr + off;
	isge->length = mr->map[m]->segs[n].length - off;
	isge->sge_length = sge->length;
	isge->m = m;
	isge->n = n;
ok:
	return 1;
bail:
	rcu_read_unlock();
	return 0;
}

/**
 * qib_rkey_ok - check the IB virtual address, length, and RKEY
 * @qp: qp for validation
 * @sge: SGE state
 * @len: length of data
 * @vaddr: virtual address to place data
 * @rkey: rkey to check
 * @acc: access flags
 *
 * Return 1 if successful, otherwise 0.
 *
 * increments the reference count upon success
 */
int qib_rkey_ok(struct qib_qp *qp, struct qib_sge *sge,
		u32 len, u64 vaddr, u32 rkey, int acc)
{
	struct qib_lkey_table *rkt = &to_idev(qp->ibqp.device)->lk_table;
	struct qib_mregion *mr;
	unsigned n, m;
	size_t off;

	/*
	 * We use RKEY == zero for kernel virtual addresses
	 * (see qib_get_dma_mr and qib_dma.c).
	 */
	rcu_read_lock();
	if (rkey == 0) {
		struct qib_pd *pd = to_ipd(qp->ibqp.pd);
		struct qib_ibdev *dev = to_idev(pd->ibpd.device);

		if (pd->user)
			goto bail;
		mr = rcu_dereference(dev->dma_mr);
		if (!mr)
			goto bail;
		if (unlikely(!atomic_inc_not_zero(&mr->refcount)))
			goto bail;
		rcu_read_unlock();

		sge->mr = mr;
		sge->vaddr = (void *) vaddr;
		sge->length = len;
		sge->sge_length = len;
		sge->m = 0;
		sge->n = 0;
		goto ok;
	}

	mr = rcu_dereference(
		rkt->table[(rkey >> (32 - ib_qib_lkey_table_size))]);
	if (unlikely(!mr || mr->lkey != rkey || qp->ibqp.pd != mr->pd))
		goto bail;

	off = vaddr - mr->iova;
	if (unlikely(vaddr < mr->iova || off + len > mr->length ||
		     (mr->access_flags & acc) == 0))
		goto bail;
	if (unlikely(!atomic_inc_not_zero(&mr->refcount)))
		goto bail;
	rcu_read_unlock();

	off += mr->offset;
	if (mr->page_shift) {
		/*
		page sizes are uniform power of 2 so no loop is necessary
		entries_spanned_by_off is the number of times the loop below
		would have executed.
		*/
		size_t entries_spanned_by_off;

		entries_spanned_by_off = off >> mr->page_shift;
		off -= (entries_spanned_by_off << mr->page_shift);
		m = entries_spanned_by_off/QIB_SEGSZ;
		n = entries_spanned_by_off%QIB_SEGSZ;
	} else {
		m = 0;
		n = 0;
		while (off >= mr->map[m]->segs[n].length) {
			off -= mr->map[m]->segs[n].length;
			n++;
			if (n >= QIB_SEGSZ) {
				m++;
				n = 0;
			}
		}
	}
	sge->mr = mr;
	sge->vaddr = mr->map[m]->segs[n].vaddr + off;
	sge->length = mr->map[m]->segs[n].length - off;
	sge->sge_length = len;
	sge->m = m;
	sge->n = n;
ok:
	return 1;
bail:
	rcu_read_unlock();
	return 0;
}

/*
 * Initialize the memory region specified by the work request.
 */
int qib_reg_mr(struct qib_qp *qp, struct ib_reg_wr *wr)
{
	struct qib_lkey_table *rkt = &to_idev(qp->ibqp.device)->lk_table;
	struct qib_pd *pd = to_ipd(qp->ibqp.pd);
	struct qib_mr *mr = to_imr(wr->mr);
	struct qib_mregion *mrg;
	u32 key = wr->key;
	unsigned i, n, m;
	int ret = -EINVAL;
	unsigned long flags;
	u64 *page_list;
	size_t ps;

	spin_lock_irqsave(&rkt->lock, flags);
	if (pd->user || key == 0)
		goto bail;

	mrg = rcu_dereference_protected(
		rkt->table[(key >> (32 - ib_qib_lkey_table_size))],
		lockdep_is_held(&rkt->lock));
	if (unlikely(mrg == NULL || qp->ibqp.pd != mrg->pd))
		goto bail;

	if (mr->npages > mrg->max_segs)
		goto bail;

	ps = mr->ibmr.page_size;
	if (mr->ibmr.length > ps * mr->npages)
		goto bail;

	mrg->user_base = mr->ibmr.iova;
	mrg->iova = mr->ibmr.iova;
	mrg->lkey = key;
	mrg->length = mr->ibmr.length;
	mrg->access_flags = wr->access;
	page_list = mr->pages;
	m = 0;
	n = 0;
	for (i = 0; i < mr->npages; i++) {
		mrg->map[m]->segs[n].vaddr = (void *) page_list[i];
		mrg->map[m]->segs[n].length = ps;
		if (++n == QIB_SEGSZ) {
			m++;
			n = 0;
		}
	}

	ret = 0;
bail:
	spin_unlock_irqrestore(&rkt->lock, flags);
	return ret;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
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
#ifndef _QIB_MAD_H
#define _QIB_MAD_H

#include <rdma/ib_pma.h>

#define IB_SMP_UNSUP_VERSION \
cpu_to_be16(IB_MGMT_MAD_STATUS_BAD_VERSION)

#define IB_SMP_UNSUP_METHOD \
cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD)

#define IB_SMP_UNSUP_METH_ATTR \
cpu_to_be16(IB_MGMT_MAD_STATUS_UNSUPPORTED_METHOD_ATTRIB)

#define IB_SMP_INVALID_FIELD \
cpu_to_be16(IB_MGMT_MAD_STATUS_INVALID_ATTRIB_VALUE)

#define IB_VLARB_LOWPRI_0_31    1
#define IB_VLARB_LOWPRI_32_63   2
#define IB_VLARB_HIGHPRI_0_31   3
#define IB_VLARB_HIGHPRI_32_63  4

#define IB_PMA_PORT_COUNTERS_CONG       cpu_to_be16(0xFF00)

struct ib_pma_portcounters_cong {
	u8 reserved;
	u8 reserved1;
	__be16 port_check_rate;
	__be16 symbol_error_counter;
	u8 link_error_recovery_counter;
	u8 link_downed_counter;
	__be16 port_rcv_errors;
	__be16 port_rcv_remphys_errors;
	__be16 port_rcv_switch_relay_errors;
	__be16 port_xmit_discards;
	u8 port_xmit_constraint_errors;
	u8 port_rcv_constraint_errors;
	u8 reserved2;
	u8 link_overrun_errors; /* LocalLink: 7:4, BufferOverrun: 3:0 */
	__be16 reserved3;
	__be16 vl15_dropped;
	__be64 port_xmit_data;
	__be64 port_rcv_data;
	__be64 port_xmit_packets;
	__be64 port_rcv_packets;
	__be64 port_xmit_wait;
	__be64 port_adr_events;
} __packed;

#define IB_PMA_CONG_HW_CONTROL_TIMER            0x00
#define IB_PMA_CONG_HW_CONTROL_SAMPLE           0x01

#define QIB_XMIT_RATE_UNSUPPORTED               0x0
#define QIB_XMIT_RATE_PICO                      0x7
/* number of 4nsec cycles equaling 2secs */
#defi