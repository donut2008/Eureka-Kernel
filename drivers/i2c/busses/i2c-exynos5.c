chnl) EPB_LOC(chnl, 6, 0xF)

static int qib_sd_dactrim(struct qib_devdata *dd)
{
	int ret;

	ret = ibsd_mod_allchnls(dd, VCDL_DAC2(0) | EPB_GLOBAL_WR, 0x2D, 0xFF);
	if (ret < 0)
		goto bail;

	/* more fine-tuning of what will be default */
	ret = ibsd_mod_allchnls(dd, VCDL_CTRL2(0), 3, 0xF);
	if (ret < 0)
		goto bail;

	ret = ibsd_mod_allchnls(dd, BACTRL(0) | EPB_GLOBAL_WR, 0x40, 0xFF);
	if (ret < 0)
		goto bail;

	ret = ibsd_mod_allchnls(dd, LDOUTCTRL1(0) | EPB_GLOBAL_WR, 0x04, 0xFF);
	if (ret < 0)
		goto bail;

	ret = ibsd_mod_allchnls(dd, RXHSSTATUS(0) | EPB_GLOBAL_WR, 0x04, 0xFF);
	if (ret < 0)
		goto bail;

	/*
	 * Delay for max possible number of steps, with slop.
	 * Each step is about 4usec.
	 */
	udelay(415);

	ret = ibsd_mod_allchnls(dd, LDOUTCTRL1(0) | EPB_GLOBAL_WR, 0x00, 0xFF);

bail:
	return ret;
}

#define RELOCK_FIRST_MS 3
#define RXLSPPM(chan) EPB_LOC(chan, 0, 2)
void toggle_7220_rclkrls(struct qib_devdata *dd)
{
	int loc = RXLSPPM(0) | EPB_GLOBAL_WR;
	int ret;

	ret = ibsd_mod_allchnls(dd, loc, 0, 0x80);
	if (ret < 0)
		qib_dev_err(dd, "RCLKRLS failed to clear D7\n");
	else {
		udelay(1);
		ibsd_mod_allchnls(dd, loc, 0x80, 0x80);
	}
	/* And again for good measure */
	udelay(1);
	ret = ibsd_mod_allchnls(dd, loc, 0, 0x80);
	if (ret < 0)
		qib_dev_err(dd, "RCLKRLS failed to clear D7\n");
	else {
		udelay(1);
		ibsd_mod_allchnls(dd, loc, 0x80, 0x80);
	}
	/* Now reset xgxs and IBC to complete the recovery */
	dd->f_xgxs_reset(dd->pport);
}

/*
 * Shut down the timer that polls for relock occasions, if needed
 * this is "hooked" from qib_7220_quiet_serdes(), which is called
 * just before qib_shutdown_device() in qib_driver.c shuts down all
 * the other timers
 */
void shutdown_7220_relock_poll(struct qib_devdata *dd)
{
	if (dd->cspec->relock_timer_active)
		del_timer_sync(&dd->cspec->relock_timer);
}

static unsigned qib_relock_by_timer = 1;
module_param_named(relock_by_timer, qib_relock_by_timer, uint,
		   S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(relock_by_timer, "Allow relock attempt if link not up");

static void qib_run_relock(unsigned long opaque)
{
	struct qib_devdata *dd = (struct qib_devdata *)opaque;
	struct qib_pportdata *ppd = dd->pport;
	struct qib_chip_specific *cs = dd->cspec;
	int timeoff;

	/*
	 * Check link-training state for "stuck" state, when down.
	 * if found, try relock and schedule another try at
	 * exponentially growing delay, maxed at one second.
	 * if not stuck, our work is done.
	 */
	if ((dd->flags & QIB_INITTED) && !(ppd->lflags &
	    (QIBL_IB_AUTONEG_INPROG | QIBL_LINKINIT | QIBL_LINKARMED |
	     QIBL_LINKACTIVE))) {
		if (qib_relock_by_timer) {
			if (!(ppd->lflags & QIBL_IB_LINK_DISABLED))
				toggle_7220_rclkrls(dd);
		}
		/* re-set timer for next check */
		timeoff = cs->relock_interval << 1;
		if (timeoff > HZ)
			timeoff = HZ;
		cs->relock_interval = timeoff;
	} else
		timeoff = HZ;
	mod_timer(&cs->relock_timer, jiffies + timeoff);
}

void set_7220_relock_poll(struct qib_devdata *dd, int ibup)
{
	struct qib_chip_specific *cs = dd->cspec;

	if (ibup) {
		/* We are now up, relax timer to 1 second interval */
		if (cs->relock_timer_active) {
			cs->relock_interval = HZ;
			mod_timer(&cs->relock_timer, jiffies + HZ);
		}
	} else {
		/* Transition to down, (re-)set timer to short interval. */
		unsigned int timeout;

		timeout = msecs_to_jiffies(RELOCK_FIRST_MS);
		if (timeout == 0)
			timeout = 1;
		/* If timer has not yet been started, do so. */
		if (!cs->relock_timer_active) {
			cs->relock_timer_active = 1;
			init_timer(&cs->relock_timer);
			cs->relock_timer.function = qib_run_relock;
			cs->relock_timer.data = (unsigned long) dd;
			cs->relock_interval = timeout;
			cs->relock_timer.expires = jiffies + timeout;
			add_timer(&cs->relock_timer);
		} else {
			cs->relock_interval = timeout;
			mod_timer(&cs->relock_timer, jiffies + timeout);
		}
	}
}
                                                                                                                                                                                                                                /*
 * Copyright (c) 2012 Intel Corporation. All rights reserved.
 * Copyright (c) 2007 - 2012 QLogic Corporation. All rights reserved.
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
#include <linux/netdevice.h>
#include <linux/moduleparam.h>

#include "qib.h"
#include "qib_common.h"

/* default pio off, sdma on */
static ushort sdma_descq_cnt = 256;
module_param_named(sdma_descq_cnt, sdma_descq_cnt, ushort, S_IRUGO);
MODULE_PARM_DESC(sdma_descq_cnt, "Number of SDMA descq entries");

/*
 * Bits defined in the send DMA descriptor.
 */
#define SDMA_DESC_LAST          (1ULL << 11)
#define SDMA_DESC_FIRST         (1ULL << 12)
#define SDMA_DESC_DMA_HEAD      (1ULL << 13)
#define SDMA_DESC_USE_LARGE_BUF (1ULL << 14)
#define SDMA_DESC_INTR          (1ULL << 15)
#define SDMA_DESC_COUNT_LSB     16
#define SDMA_DESC_GEN_LSB       30

char *qib_sdma_state_names[] = {
	[qib_sdma_state_s00_hw_down]          = "s00_HwDown",
	[qib_sdma_state_s10_hw_start_up_wait] = "s10_HwStartUpWait",
	[qib_sdma_state_s20_idle]             = "s20_Idle",
	[qib_sdma_state_s30_sw_clean_up_wait] = "s30_SwCleanUpWait",
	[qib_sdma_state_s40_hw_clean_up_wait] = "s40_HwCleanUpWait",
	[qib_sdma_state_s50_hw_halt_wait]     = "s50_HwHaltWait",
	[qib_sdma_state_s99_running]          = "s99_Running",
};

char *qib_sdma_event_names[] = {
	[qib_sdma_event_e00_go_hw_down]   = "e00_GoHwDown",
	[qib_sdma_event_e10_go_hw_start]  = "e10_GoHwStart",
	[qib_sdma_event_e20_hw_started]   = "e20_HwStarted",
	[qib_sdma_event_e30_go_running]   = "e30_GoRunning",
	[qib_sdma_event_e40_sw_cleaned]   = "e40_SwCleaned",
	[qib_sdma_event_e50_hw_cleaned]   = "e50_HwCleaned",
	[qib_sdma_event_e60_hw_halted]    = "e60_HwHalted",
	[qib_sdma_event_e70_go_idle]      = "e70_GoIdle",
	[qib_sdma_event_e7220_err_halted] = "e7220_ErrHalted",
	[qib_sdma_event_e7322_err_halted] = "e7322_ErrHalted",
	[qib_sdma_event_e90_timer_tick]   = "e90_TimerTick",
};

/* declare all statics here rather than keep sorting */
static int alloc_sdma(struct qib_pportdata *);
static void sdma_complete(struct kref *);
static void sdma_finalput(struct qib_sdma_state *);
static void sdma_get(struct qib_sdma_state *);
static void sdma_put(struct qib_sdma_state *);
static void sdma_set_state(struct qib_pportdata *, enum qib_sdma_states);
static void sdma_start_sw_clean_up(struct qib_pportdata *);
static void sdma_sw_clean_up_task(unsigned long);
static void unmap_desc(struct qib_pportdata *, unsigned);

static void sdma_get(struct qib_sdma_state *ss)
{
	kref_get(&ss->kref);
}

static void sdma_complete(struct kref *kref)
{
	struct qib_sdma_state *ss =
		container_of(kref, struct qib_sdma_state, kref);

	complete(&ss->comp);
}

static void sdma_put(struct qib_sdma_state *ss)
{
	kref_put(&ss->kref, sdma_complete);
}

static void sdma_finalput(struct qib_sdma_state *ss)
{
	sdma_put(ss);
	wait_for_completion(&ss->comp);
}

/*
 * Complete all the sdma requests on the active list, in the correct
 * order, and with appropriate processing.   Called when cleaning up
 * after sdma shutdown, and when new sdma requests are submitted for
 * a link that is down.   This matches what is done for requests
 * that complete normally, it's just the full list.
 *
 * Must be called with sdma_lock held
 */
static void clear_sdma_activelist(struct qib_pportdata *ppd)
{
	struct qib_sdma_txreq *txp, *txp_next;

	list_for_each_entry_safe(txp, txp_next, &ppd->sdma_activelist, list) {
		list_del_init(&txp->list);
		if (txp->flags & QIB_SDMA_TXREQ_F_FREEDESC) {
			unsigned idx;

			idx = txp->start_idx;
			while (idx != txp->next_descq_idx) {
				unmap_desc(ppd, idx);
				if (++idx == ppd->sdma_descq_cnt)
					idx = 0;
			}
		}
		if (txp->callback)
			(*txp->callback)(txp, QIB_SDMA_TXREQ_S_ABORTED);
	}
}

static void sdma_sw_clean_up_task(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *) opaque;
	unsigned long flags;

	spin_lock_irqsave(&ppd->sdma_lock, flags);

	/*
	 * At this point, the following should always be true:
	 * - We are halted, so no more descriptors are getting retired.
	 * - We are not running, so no one is submitting new work.
	 * - Only we can send the e40_sw_cleaned, so we can't start
	 *   running again until we say so.  So, the active list and
	 *   descq are ours to play with.
	 */

	/* Process all retired requests. */
	qib_sdma_make_progress(ppd);

	clear_sdma_activelist(ppd);

	/*
	 * Resync count of added and removed.  It is VERY important that
	 * sdma_descq_removed NEVER decrement - user_sdma depends on it.
	 */
	ppd->sdma_descq_removed = ppd->sdma_descq_added;

	/*
	 * Reset our notion of head and tail.
	 * Note that the HW registers will be reset when switching states
	 * due to calling __qib_sdma_process_event() below.
	 */
	ppd->sdma_descq_tail = 0;
	ppd->sdma_descq_head = 0;
	ppd->sdma_head_dma[0] = 0;
	ppd->sdma_generation = 0;

	__qib_sdma_process_event(ppd, qib_sdma_event_e40_sw_cleaned);

	spin_unlock_irqrestore(&ppd->sdma_lock, flags);
}

/*
 * This is called when changing to state qib_sdma_state_s10_hw_start_up_wait
 * as a result of send buffer errors or send DMA descriptor errors.
 * We want to disarm the buffers in these cases.
 */
static void sdma_hw_start_up(struct qib_pportdata *ppd)
{
	struct qib_sdma_state *ss = &ppd->sdma_state;
	unsigned bufno;

	for (bufno = ss->first_sendbuf; bufno < ss->last_sendbuf; ++bufno)
		ppd->dd->f_sendctrl(ppd, QIB_SENDCTRL_DISARM_BUF(bufno));

	ppd->dd->f_sdma_hw_start_up(ppd);
}

static void sdma_sw_tear_down(struct qib_pportdata *ppd)
{
	struct qib_sdma_state *ss = &ppd->sdma_state;

	/* Releasing this reference means the state machine has stopped. */
	sdma_put(ss);
}

static void sdma_start_sw_clean_up(struct qib_pportdata *ppd)
{
	tasklet_hi_schedule(&ppd->sdma_sw_clean_up_task);
}

static void sdma_set_state(struct qib_pportdata *ppd,
	enum qib_sdma_states next_state)
{
	struct qib_sdma_state *ss = &ppd->sdma_state;
	struct sdma_set_state_action *action = ss->set_state_action;
	unsigned op = 0;

	/* debugging bookkeeping */
	ss->previous_state = ss->current_state;
	ss->previous_op = ss->current_op;

	ss->current_state = next_state;

	if (action[next_state].op_enable)
		op |= QIB_SDMA_SENDCTRL_OP_ENABLE;

	if (action[next_state].op_intenable)
		op |= QIB_SDMA_SENDCTRL_OP_INTENABLE;

	if (action[next_state].op_halt)
		op |= QIB_SDMA_SENDCTRL_OP_HALT;

	if (action[next_state].op_drain)
		op |= QIB_SDMA_SENDCTRL_OP_DRAIN;

	if (action[next_state].go_s99_running_tofalse)
		ss->go_s99_running = 0;

	if (action[next_state].go_s99_running_totrue)
		ss->go_s99_running = 1;

	ss->current_op = op;

	ppd->dd->f_sdma_sendctrl(ppd, ss->current_op);
}

static void unmap_desc(struct qib_pportdata *ppd, unsigned head)
{
	__le64 *descqp = &ppd->sdma_descq[head].qw[0];
	u64 desc[2];
	dma_addr_t addr;
	size_t len;

	desc[0] = le64_to_cpu(descqp[0]);
	desc[1] = le64_to_cpu(descqp[1]);

	addr = (desc[1] << 32) | (desc[0] >> 32);
	len = (desc[0] >> 14) & (0x7ffULL << 2);
	dma_unmap_single(&ppd->dd->pcidev->dev, addr, len, DMA_TO_DEVICE);
}

static int alloc_sdma(struct qib_pportdata *ppd)
{
	ppd->sdma_descq_cnt = sdma_descq_cnt;
	if (!ppd->sdma_descq_cnt)
		ppd->sdma_descq_cnt = 256;

	/* Allocate memory for SendDMA descriptor FIFO */
	ppd->sdma_descq = dma_alloc_coherent(&ppd->dd->pcidev->dev,
		ppd->sdma_descq_cnt * sizeof(u64[2]), &ppd->sdma_descq_phys,
		GFP_KERNEL);

	if (!ppd->sdma_descq) {
		qib_dev_err(ppd->dd,
			"failed to allocate SendDMA descriptor FIFO memory\n");
		goto bail;
	}

	/* Allocate memory for DMA of head register to memory */
	ppd->sdma_head_dma = dma_alloc_coherent(&ppd->dd->pcidev->dev,
		PAGE_SIZE, &ppd->sdma_head_phys, GFP_KERNEL);
	if (!ppd->sdma_head_dma) {
		qib_dev_err(ppd->dd,
			"failed to allocate SendDMA head memory\n");
		goto cleanup_descq;
	}
	ppd->sdma_head_dma[0] = 0;
	return 0;

cleanup_descq:
	dma_free_coherent(&ppd->dd->pcidev->dev,
		ppd->sdma_descq_cnt * sizeof(u64[2]), (void *)ppd->sdma_descq,
		ppd->sdma_descq_phys);
	ppd->sdma_descq = NULL;
	ppd->sdma_descq_phys = 0;
bail:
	ppd->sdma_descq_cnt = 0;
	return -ENOMEM;
}

static void free_sdma(struct qib_pportdata *ppd)
{
	struct qib_devdata *dd = ppd->dd;

	if (ppd->sdma_head_dma) {
		dma_free_coherent(&dd->pcidev->dev, PAGE_SIZE,
				  (void *)ppd->sdma_head_dma,
				  ppd->sdma_head_phys);
		ppd->sdma_head_dma = NULL;
		ppd->sdma_head_phys = 0;
	}

	if (ppd->sdma_descq) {
		dma_free_coherent(&dd->pcidev->dev,
				  ppd->sdma_descq_cnt * sizeof(u64[2]),
				  ppd->sdma_descq, ppd->sdma_descq_phys);
		ppd->sdma_descq = NULL;
		ppd->sdma_descq_phys = 0;
	}
}

static inline void make_sdma_desc(struct qib_pportdata *ppd,
				  u64 *sdmadesc, u64 addr, u64 dwlen,
				  u64 dwoffset)
{

	WARN_ON(addr & 3);
	/* SDmaPhyAddr[47:32] */
	sdmadesc[1] = addr >> 32;
	/* SDmaPhyAddr[31:0] */
	sdmadesc[0] = (addr & 0xfffffffcULL) << 32;
	/* SDmaGeneration[1:0] */
	sdmadesc[0] |= (ppd->sdma_generation & 3ULL) <<
		SDMA_DESC_GEN_LSB;
	/* SDmaDwordCount[10:0] */
	sdmadesc[0] |= (dwlen & 0x7ffULL) << SDMA_DESC_COUNT_LSB;
	/* SDmaBufOffset[12:2] */
	sdmadesc[0] |= dwoffset & 0x7ffULL;
}

/* sdma_lock must be held */
int qib_sdma_make_progress(struct qib_pportdata *ppd)
{
	struct list_head *lp = NULL;
	struct qib_sdma_txreq *txp = NULL;
	struct qib_devdata *dd = ppd->dd;
	int progress = 0;
	u16 hwhead;
	u16 idx = 0;

	hwhead = dd->f_sdma_gethead(ppd);

	/* The reason for some of the complexity of this code is that
	 * not all descriptors have corresponding txps.  So, we have to
	 * be able to skip over descs until we wander into the range of
	 * the next txp on the list.
	 */

	if (!list_empty(&ppd->sdma_activelist)) {
		lp = ppd->sdma_activelist.next;
		txp = list_entry(lp, struct qib_sdma_txreq, list);
		idx = txp->start_idx;
	}

	while (ppd->sdma_descq_head != hwhead) {
		/* if desc is part of this txp, unmap if needed */
		if (txp && (txp->flags & QIB_SDMA_TXREQ_F_FREEDESC) &&
		    (idx == ppd->sdma_descq_head)) {
			unmap_desc(ppd, ppd->sdma_descq_head);
			if (++idx == ppd->sdma_descq_cnt)
				idx = 0;
		}

		/* increment dequed desc count */
		ppd->sdma_descq_removed++;

		/* advance head, wrap if needed */
		if (++ppd->sdma_descq_head == ppd->sdma_descq_cnt)
			ppd->sdma_descq_head = 0;

		/* if now past this txp's descs, do the callback */
		if (txp && txp->next_descq_idx == ppd->sdma_descq_head) {
			/* remove from active list */
			list_del_init(&txp->list);
			if (txp->callback)
				(*txp->callback)(txp, QIB_SDMA_TXREQ_S_OK);
			/* see if there is another txp */
			if (list_empty(&ppd->sdma_activelist))
				txp = NULL;
			else {
				lp = ppd->sdma_activelist.next;
				txp = list_entry(lp, struct qib_sdma_txreq,
					list);
				idx = txp->start_idx;
			}
		}
		progress = 1;
	}
	if (progress)
		qib_verbs_sdma_desc_avail(ppd, qib_sdma_descq_freecnt(ppd));
	return progress;
}

/*
 * This is called from interrupt context.
 */
void qib_sdma_intr(struct qib_pportdata *ppd)
{
	unsigned long flags;

	spin_lock_irqsave(&ppd->sdma_lock, flags);

	__qib_sdma_intr(ppd);

	spin_unlock_irqrestore(&ppd->sdma_lock, flags);
}

void __qib_sdma_intr(struct qib_pportdata *ppd)
{
	if (__qib_sdma_running(ppd)) {
		qib_sdma_make_progress(ppd);
		if (!list_empty(&ppd->sdma_userpending))
			qib_user_sdma_send_desc(ppd, &ppd->sdma_userpending);
	}
}

int qib_setup_sdma(struct qib_pportdata *ppd)
{
	struct qib_devdata *dd = ppd->dd;
	unsigned long flags;
	int ret = 0;

	ret = alloc_sdma(ppd);
	if (ret)
		goto bail;

	/* set consistent sdma state */
	ppd->dd->f_sdma_init_early(ppd);
	spin_lock_irqsave(&ppd->sdma_lock, flags);
	sdma_set_state(ppd, qib_sdma_state_s00_hw_down);
	spin_unlock_irqrestore(&ppd->sdma_lock, flags);

	/* set up reference counting */
	kref_init(&ppd->sdma_state.kref);
	init_completion(&ppd->sdma_state.comp);

	ppd->sdma_generation = 0;
	ppd->sdma_descq_head = 0;
	ppd->sdma_descq_removed = 0;
	ppd->sdma_descq_added = 0;

	ppd->sdma_intrequest = 0;
	INIT_LIST_HEAD(&ppd->sdma_userpending);

	INIT_LIST_HEAD(&ppd->sdma_activelist);

	tasklet_init(&ppd->sdma_sw_clean_up_task, sdma_sw_clean_up_task,
		(unsigned long)ppd);

	ret = dd->f_init_sdma_regs(ppd);
	if (ret)
		goto bail_alloc;

	qib_sdma_process_event(ppd, qib_sdma_event_e10_go_hw_start);

	return 0;

bail_alloc:
	qib_teardown_sdma(ppd);
bail:
	return ret;
}

void qib_teardown_sdma(struct qib_pportdata *ppd)
{
	qib_sdma_process_event(ppd, qib_sdma_event_e00_go_hw_down);

	/*
	 * This waits for the state machine to exit so it is not
	 * necessary to kill the sdma_sw_clean_up_task to make sure
	 * it is not running.
	 */
	sdma_finalput(&ppd->sdma_state);

	free_sdma(ppd);
}

int qib_sdma_running(struct qib_pportdata *ppd)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&ppd->sdma_lock, flags);
	ret = __qib_sdma_running(ppd);
	spin_unlock_irqrestore(&ppd->sdma_lock, flags);

	return ret;
}

/*
 * Complete a request when sdma not running; likely only request
 * but to simplify the code, always queue it, then process the full
 * activelist.  We process the entire list to ensure that this particular
 * request does get it's callback, but in the correct order.
 * Must be called with sdma_lock held
 */
static void complete_sdma_err_req(struct qib_pportdata *ppd,
				  struct qib_verbs_txreq *tx)
{
	atomic_inc(&tx->qp->s_dma_busy);
	/* no sdma descriptors, so no unmap_desc */
	tx->txreq.start_idx = 0;
	tx->txreq.next_descq_idx = 0;
	list_add_tail(&tx->txreq.list, &ppd->sdma_activelist);
	clear_sdma_activelist(ppd);
}

/*
 * This function queues one IB packet onto the send DMA queue per call.
 * The caller is responsible for checking:
 * 1) The number of send DMA descriptor entries is less than the size of
 *    the descriptor queue.
 * 2) The IB SGE addresses and lengths are 32-bit aligned
 *    (except possibly the last SGE's length)
 * 3) The SGE addresses are suitable for passing to dma_map_single().
 */
int qib_sdma_verbs_send(struct qib_pportdata *ppd,
			struct qib_sge_state *ss, u32 dwords,
			struct qib_verbs_txreq *tx)
{
	unsigned long flags;
	struct qib_sge *sge;
	struct qib_qp *qp;
	int ret = 0;
	u16 tail;
	__le64 *descqp;
	u64 sdmadesc[2];
	u32 dwoffset;
	dma_addr_t addr;

	spin_lock_irqsave(&ppd->sdma_lock, flags);

retry:
	if (unlikely(!__qib_sdma_running(ppd))) {
		complete_sdma_err_req(ppd, tx);
		goto unlock;
	}

	if (tx->txreq.sg_count > qib_sdma_descq_freecnt(ppd)) {
		if (qib_sdma_make_progress(ppd))
			goto retry;
		if (ppd->dd->flags & QIB_HAS_SDMA_TIMEOUT)
			ppd->dd->f_sdma_set_desc_cnt(ppd,
					ppd->sdma_descq_cnt / 2);
		goto busy;
	}

	dwoffset = tx->hdr_dwords;
	make_sdma_desc(ppd, sdmadesc, (u64) tx->txreq.addr, dwoffset, 0);

	sdmadesc[0] |= SDMA_DESC_FIRST;
	if (tx->txreq.flags & QIB_SDMA_TXREQ_F_USELARGEBUF)
		sdmadesc[0] |= SDMA_DESC_USE_LARGE_BUF;

	/* write to the descq */
	tail = ppd->sdma_descq_tail;
	descqp = &ppd->sdma_descq[tail].qw[0];
	*descqp++ = cpu_to_le64(sdmadesc[0]);
	*descqp++ = cpu_to_le64(sdmadesc[1]);

	/* increment the tail */
	if (++tail == ppd->sdma_descq_cnt) {
		tail = 0;
		descqp = &ppd->sdma_descq[0].qw[0];
		++ppd->sdma_generation;
	}

	tx->txreq.start_idx = tail;

	sge = &ss->sge;
	while (dwords) {
		u32 dw;
		u32 len;

		len = dwords << 2;
		if (len > sge->length)
			len = sge->length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		dw = (len + 3) >> 2;
		addr = dma_map_single(&ppd->dd->pcidev->dev, sge->vaddr,
				      dw << 2, DMA_TO_DEVICE);
		if (dma_mapping_error(&ppd->dd->pcidev->dev, addr)) {
			ret = -ENOMEM;
			goto unmap;
		}
		sdmadesc[0] = 0;
		make_sdma_desc(ppd, sdmadesc, (u64) addr, dw, dwoffset);
		/* SDmaUseLargeBuf has to be set in every descriptor */
		if (tx->txreq.flags & QIB_SDMA_TXREQ_F_USELARGEBUF)
			sdmadesc[0] |= SDMA_DESC_USE_LARGE_BUF;
		/* write to the descq */
		*descqp++ = cpu_to_le64(sdmadesc[0]);
		*descqp++ = cpu_to_le64(sdmadesc[1]);

		/* increment the tail */
		if (++tail == ppd->sdma_descq_cnt) {
			tail = 0;
			descqp = &ppd->sdma_descq[0].qw[0];
			++ppd->sdma_generation;
		}
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}

		dwoffset += dw;
		dwords -= dw;
	}

	if (!tail)
		descqp = &ppd->sdma_descq[ppd->sdma_descq_cnt].qw[0];
	descqp -= 2;
	descqp[0] |= cpu_to_le64(SDMA_DESC_LAST);
	if (tx->txreq.flags & QIB_SDMA_TXREQ_F_HEADTOHOST)
		descqp[0] |= cpu_to_le64(SDMA_DESC_DMA_HEAD);
	if (tx->txreq.flags & QIB_SDMA_TXREQ_F_INTREQ)
		descqp[0] |= cpu_to_le64(SDMA_DESC_INTR);

	atomic_inc(&tx->qp->s_dma_busy);
	tx->txreq.next_descq_idx = tail;
	ppd->dd->f_sdma_update_tail(ppd, tail);
	ppd->sdma_descq_added += tx->txreq.sg_count;
	list_add_tail(&tx->txreq.list, &ppd->sdma_activelist);
	goto unlock;

unmap:
	for (;;) {
		if (!tail)
			tail = ppd->sdma_descq_cnt - 1;
		else
			tail--;
		if (tail == ppd->sdma_descq_tail)
			break;
		unmap_desc(ppd, tail);
	}
	qp = tx->qp;
	qib_put_txreq(tx);
	spin_lock(&qp->r_lock);
	spin_lock(&qp->s_lock);
	if (qp->ibqp.qp_type == IB_QPT_RC) {
		/* XXX what about error sending RDMA read responses? */
		if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK)
			qib_error_qp(qp, IB_WC_GENERAL_ERR);
	} else if (qp->s_wqe)
		qib_send_complete(qp, qp->s_wqe, IB_WC_GENERAL_ERR);
	spin_unlock(&qp->s_lock);
	spin_unlock(&qp->r_lock);
	/* return zero to process the next send work request */
	goto unlock;

busy:
	qp = tx->qp;
	spin_lock(&qp->s_lock);
	if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK) {
		struct qib_ibdev *dev;

		/*
		 * If we couldn't queue the DMA request, save the info
		 * and try again later rather than destroying the
		 * buffer and undoing the side effects of the copy.
		 */
		tx->ss = ss;
		tx->dwords = dwords;
		qp->s_tx = tx;
		dev = &ppd->dd->verbs_dev;
		spin_lock(&dev->pending_lock);
		if (list_empty(&qp->iowait)) {
			struct qib_ibport *ibp;

			ibp = &ppd->ibport_data;
			ibp->n_dmawait++;
			qp->s_flags |= QIB_S_WAIT_DMA_DESC;
			list_add_tail(&qp->iowait, &dev->dmawait);
		}
		spin_unlock(&dev->pending_lock);
		qp->s_flags &= ~QIB_S_BUSY;
		spin_unlock(&qp->s_lock);
		ret = -EBUSY;
	} else {
		spin_unlock(&qp->s_lock);
		qib_put_txreq(tx);
	}
unlock:
	spin_unlock_irqrestore(&ppd->sdma_lock, flags);
	return ret;
}

/*
 * sdma_lock should be acquired before calling this routine
 */
void dump_sdma_state(struct qib_pportdata *ppd)
{
	struct qib_sdma_desc *descq;
	struct qib_sdma_txreq *txp, *txpnext;
	__le64 *descqp;
	u64 desc[2];
	u64 addr;
	u16 gen, dwlen, dwoffset;
	u16 head, tail, cnt;

	head = ppd->sdma_descq_head;
	tail = ppd->sdma_descq_tail;
	cnt = qib_sdma_descq_freecnt(ppd);
	descq = ppd->sdma_descq;

	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA ppd->sdma_descq_head: %u\n", head);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA ppd->sdma_descq_tail: %u\n", tail);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA sdma_descq_freecnt: %u\n", cnt);

	/* print info for each entry in the descriptor queue */
	while (head != tail) {
		char flags[6] = { 'x', 'x', 'x', 'x', 'x', 0 };

		descqp = &descq[head].qw[0];
		desc[0] = le64_to_cpu(descqp[0]);
		desc[1] = le64_to_cpu(descqp[1]);
		flags[0] = (desc[0] & 1<<15) ? 'I' : '-';
		flags[1] = (desc[0] & 1<<14) ? 'L' : 'S';
		flags[2] = (desc[0] & 1<<13) ? 'H' : '-';
		flags[3] = (desc[0] & 1<<12) ? 'F' : '-';
		flags[4] = (desc[0] & 1<<11) ? 'L' : '-';
		addr = (desc[1] << 32) | ((desc[0] >> 32) & 0xfffffffcULL);
		gen = (desc[0] >> 30) & 3ULL;
		dwlen = (desc[0] >> 14) & (0x7ffULL << 2);
		dwoffset = (desc[0] & 0x7ffULL) << 2;
		qib_dev_porterr(ppd->dd, ppd->port,
			"SDMA sdmadesc[%u]: flags:%s addr:0x%016llx gen:%u len:%u bytes offset:%u bytes\n",
			 head, flags, addr, gen, dwlen, dwoffset);
		if (++head == ppd->sdma_descq_cnt)
			head = 0;
	}

	/* print dma descriptor indices from the TX requests */
	list_for_each_entry_safe(txp, txpnext, &ppd->sdma_activelist,
				 list)
		qib_dev_porterr(ppd->dd, ppd->port,
			"SDMA txp->start_idx: %u txp->next_descq_idx: %u\n",
			txp->start_idx, txp->next_descq_idx);
}

void qib_sdma_process_event(struct qib_pportdata *ppd,
	enum qib_sdma_events event)
{
	unsigned long flags;

	spin_lock_irqsave(&ppd->sdma_lock, flags);

	__qib_sdma_process_event(ppd, event);

	if (ppd->sdma_state.current_state == qib_sdma_state_s99_running)
		qib_verbs_sdma_desc_avail(ppd, qib_sdma_descq_freecnt(ppd));

	spin_unlock_irqrestore(&ppd->sdma_lock, flags);
}

void __qib_sdma_process_event(struct qib_pportdata *ppd,
	enum qib_sdma_events event)
{
	struct qib_sdma_state *ss = &ppd->sdma_state;

	switch (ss->current_state) {
	case qib_sdma_state_s00_hw_down:
		switch (event) {
		case qib_sdma_event_e00_go_hw_down:
			break;
		case qib_sdma_event_e30_go_running:
			/*
			 * If down, but running requested (usually result
			 * of link up, then we need to start up.
			 * This can happen when hw down is requested while
			 * bringing the link up with traffic active on
			 * 7220, e.g. */
			ss->go_s99_running = 1;
			/* fall through and start dma engine */
		case qib_sdma_event_e10_go_hw_start:
			/* This reference means the state machine is started */
			sdma_get(&ppd->sdma_state);
			sdma_set_state(ppd,
				       qib_sdma_state_s10_hw_start_up_wait);
			break;
		case qib_sdma_event_e20_hw_started:
			break;
		case qib_sdma_event_e40_sw_cleaned:
			sdma_sw_tear_down(ppd);
			break;
		case qib_sdma_event_e50_hw_cleaned:
			break;
		case qib_sdma_event_e60_hw_halted:
			break;
		case qib_sdma_event_e70_go_idle:
			break;
		case qib_sdma_event_e7220_err_halted:
			break;
		case qib_sdma_event_e7322_err_halted:
			break;
		case qib_sdma_event_e90_timer_tick:
			break;
		}
		break;

	case qib_sdma_state_s10_hw_start_up_wait:
		switch (event) {
		case qib_sdma_event_e00_go_hw_down:
			sdma_set_state(ppd, qib_sdma_state_s00_hw_down);
			sdma_sw_tear_down(ppd);
			break;
		case qib_sdma_event_e10_go_hw_start:
			break;
		case qib_sdma_event_e20_hw_started:
			sdma_set_state(ppd, ss->go_s99_running ?
				       qib_sdma_state_s99_running :
				       qib_sdma_state_s20_idle);
			break;
		case qib_sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case qib_sdma_event_e40_sw_cleaned:
			break;
		case qib_sdma_event_e50_hw_cleaned:
			break;
		case qib_sdma_event_e60_hw_halted:
			break;
		case qib_sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case qib_sdma_event_e7220_err_halted:
			break;
		case qib_sdma_event_e7322_err_halted:
			break;
		case qib_sdma_event_e90_timer_tick:
			break;
		}
		break;

	case qib_sdma_state_s20_idle:
		switch (event) {
		case qib_sdma_event_e00_go_hw_down:
			sdma_set_state(ppd, qib_sdma_state_s00_hw_down);
			sdma_sw_tear_down(ppd);
			break;
		case qib_sdma_event_e10_go_hw_start:
			break;
		case qib_sdma_event_e20_hw_started:
			break;
		case qib_sdma_event_e30_go_running:
			sdma_set_state(ppd, qib_sdma_state_s99_running);
			ss->go_s99_running = 1;
			break;
		case qib_sdma_event_e40_sw_cleaned:
			break;
		case qib_sdma_event_e50_hw_cleaned:
			break;
		case qib_sdma_event_e60_hw_halted:
			break;
		case qib_sdma_event_e70_go_idle:
			break;
		case qib_sdma_event_e7220_err_halted:
			break;
		case qib_sdma_event_e7322_err_halted:
			break;
		case qib_sdma_event_e90_timer_tick:
			break;
		}
		break;

	case qib_sdma_state_s30_sw_clean_up_wait:
		switch (event) {
		case qib_sdma_event_e00_go_hw_down:
			sdma_set_state(ppd, qib_sdma_state_s00_hw_down);
			break;
		case qib_sdma_event_e10_go_hw_start:
			break;
		case qib_sdma_event_e20_hw_started:
			break;
		case qib_sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case qib_sdma_event_e40_sw_cleaned:
			sdma_set_state(ppd,
				       qib_sdma_state_s10_hw_start_up_wait);
			sdma_hw_start_up(ppd);
			break;
		case qib_sdma_event_e50_hw_cleaned:
			break;
		case qib_sdma_event_e60_hw_halted:
			break;
		case qib_sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case qib_sdma_event_e7220_err_halted:
			break;
		case qib_sdma_event_e7322_err_halted:
			break;
		case qib_sdma_event_e90_timer_tick:
			break;
		}
		break;

	case qib_sdma_state_s40_hw_clean_up_wait:
		switch (event) {
		case qib_sdma_event_e00_go_hw_down:
			sdma_set_state(ppd, qib_sdma_state_s00_hw_down);
			sdma_start_sw_clean_up(ppd);
			break;
		case qib_sdma_event_e10_go_hw_start:
			break;
		case qib_sdma_event_e20_hw_started:
			break;
		case qib_sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case qib_sdma_event_e40_sw_cleaned:
			break;
		case qib_sdma_event_e50_hw_cleaned:
			sdma_set_state(ppd,
				       qib_sdma_state_s30_sw_clean_up_wait);
			sdma_start_sw_clean_up(ppd);
			break;
		case qib_sdma_event_e60_hw_halted:
			break;
		case qib_sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case qib_sdma_event_e7220_err_halted:
			break;
		case qib_sdma_event_e7322_err_halted:
			break;
		case qib_sdma_event_e90_timer_tick:
			break;
		}
		break;

	case qib_sdma_state_s50_hw_halt_wait:
		switch (event) {
		case qib_sdma_event_e00_go_hw_down:
			sdma_set_state(ppd, qib_sdma_state_s00_hw_down);
			sdma_start_sw_clean_up(ppd);
			break;
		case qib_sdma_event_e10_go_hw_start:
			break;
		case qib_sdma_event_e20_hw_started:
			break;
		case qib_sdma_event_e30_go_running:
			ss->go_s99_running = 1;
			break;
		case qib_sdma_event_e40_sw_cleaned:
			break;
		case qib_sdma_event_e50_hw_cleaned:
			break;
		case qib_sdma_event_e60_hw_halted:
			sdma_set_state(ppd,
				       qib_sdma_state_s40_hw_clean_up_wait);
			ppd->dd->f_sdma_hw_clean_up(ppd);
			break;
		case qib_sdma_event_e70_go_idle:
			ss->go_s99_running = 0;
			break;
		case qib_sdma_event_e7220_err_halted:
			break;
		case qib_sdma_event_e7322_err_halted:
			break;
		case qib_sdma_event_e90_timer_tick:
			break;
		}
		break;

	case qib_sdma_state_s99_running:
		switch (event) {
		case qib_sdma_event_e00_go_hw_down:
			sdma_set_state(ppd, qib_sdma_state_s00_hw_down);
			sdma_start_sw_clean_up(ppd);
			break;
		case qib_sdma_event_e10_go_hw_start:
			break;
		case qib_sdma_event_e20_hw_started:
			break;
		case qib_sdma_event_e30_go_running:
			break;
		case qib_sdma_event_e40_sw_cleaned:
			break;
		case qib_sdma_event_e50_hw_cleaned:
			break;
		case qib_sdma_event_e60_hw_halted:
			sdma_set_state(ppd,
				       qib_sdma_state_s30_sw_clean_up_wait);
			sdma_start_sw_clean_up(ppd);
			break;
		case qib_sdma_event_e70_go_idle:
			sdma_set_state(ppd, qib_sdma_state_s50_hw_halt_wait);
			ss->go_s99_running = 0;
			break;
		case qib_sdma_event_e7220_err_halted:
			sdma_set_state(ppd,
				       qib_sdma_state_s30_sw_clean_up_wait);
			sdma_start_sw_clean_up(ppd);
			break;
		case qib_sdma_event_e7322_err_halted:
			sdma_set_state(ppd, qib_sdma_state_s50_hw_halt_wait);
			break;
		case qib_sdma_event_e90_timer_tick:
			break;
		}
		break;
	}

	ss->last_event = event;
}
                                                                                                                                                                                                                                                             /*
 * Copyright (c) 2006, 2007, 2008, 2009 QLogic Corporation. All rights reserved.
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

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "qib_verbs.h"

/**
 * qib_post_srq_receive - post a receive on a shared receive queue
 * @ibsrq: the SRQ to post the receive on
 * @wr: the list of work requests to post
 * @bad_wr: A pointer to the first WR to cause a problem is put here
 *
 * This may be called from interrupt context.
 */
int qib_post_srq_receive(struct ib_srq *ibsrq, struct ib_recv_wr *wr,
			 struct ib_recv_wr **bad_wr)
{
	struct qib_srq *srq = to_isrq(ibsrq);
	struct qib_rwq *wq;
	unsigned long flags;
	int ret;

	for (; wr; wr = wr->next) {
		struct qib_rwqe *wqe;
		u32 next;
		int i;

		if ((unsigned) wr->num_sge > srq->rq.max_sge) {
			*bad_wr = wr;
			ret = -EINVAL;
			goto bail;
		}

		spin_lock_irqsave(&srq->rq.lock, flags);
		wq = srq->rq.wq;
		next = wq->head + 1;
		if (next >= srq->rq.size)
			next = 0;
		if (next == wq->tail) {
			spin_unlock_irqrestore(&srq->rq.lock, flags);
			*bad_wr = wr;
			ret = -ENOMEM;
			goto bail;
		}

		wqe = get_rwqe_ptr(&srq->rq, wq->head);
		wqe->wr_id = wr->wr_id;
		wqe->num_sge = wr->num_sge;
		for (i = 0; i < wr->num_sge; i++)
			wqe->sg_list[i] = wr->sg_list[i];
		/* Make sure queue entry is written before the head index. */
		smp_wmb();
		wq->head = next;
		spin_unlock_irqrestore(&srq->rq.lock, flags);
	}
	ret = 0;

bail:
	return ret;
}

/**
 * qib_create_srq - create a shared receive queue
 * @ibpd: the protection domain of the SRQ to create
 * @srq_init_attr: the attributes of the SRQ
 * @udata: data from libibverbs when creating a user SRQ
 */
struct ib_srq *qib_create_srq(struct ib_pd *ibpd,
			      struct ib_srq_init_attr *srq_init_attr,
			      struct ib_udata *udata)
{
	struct qib_ibdev *dev = to_idev(ibpd->device);
	struct qib_srq *srq;
	u32 sz;
	struct ib_srq *ret;

	if (srq_init_attr->srq_type != IB_SRQT_BASIC) {
		ret = ERR_PTR(-ENOSYS);
		goto done;
	}

	if (srq_init_attr->attr.max_sge == 0 ||
	    srq_init_attr->attr.max_sge > ib_qib_max_srq_sges ||
	    srq_init_attr->attr.max_wr == 0 ||
	    srq_init_attr->attr.max_wr > ib_qib_max_srq_wrs) {
		ret = ERR_PTR(-EINVAL);
		goto done;
	}

	srq = kmalloc(sizeof(*srq), GFP_KERNEL);
	if (!srq) {
		ret = ERR_PTR(-ENOMEM);
		goto done;
	}

	/*
	 * Need to use vmalloc() if we want to support large #s of entries.
	 */
	srq->rq.size = srq_init_attr->attr.max_wr + 1;
	srq->rq.max_sge = srq_init_attr->attr.max_sge;
	sz = sizeof(struct ib_sge) * srq->rq.max_sge +
		sizeof(struct qib_rwqe);
	srq->rq.wq = vmalloc_user(sizeof(struct qib_rwq) + srq->rq.size * sz);
	if (!srq->rq.wq) {
		ret = ERR_PTR(-ENOMEM);
		goto bail_srq;
	}

	/*
	 * Return the address of the RWQ as the offset to mmap.
	 * See qib_mmap() for details.
	 */
	if (udata && udata->outlen >= sizeof(__u64)) {
		int err;
		u32 s = sizeof(struct qib_rwq) + srq->rq.size * sz;

		srq->ip =
		    qib_create_mmap_info(dev, s, ibpd->uobject->context,
					 srq->rq.wq);
		if (!srq->ip) {
			ret = ERR_PTR(-ENOMEM);
			goto bail_wq;
		}

		err = ib_copy_to_udata(udata, &srq->ip->offset,
				       sizeof(srq->ip->offset));
		if (err) {
			ret = ERR_PTR(err);
			goto bail_ip;
		}
	} else
		srq->ip = NULL;

	/*
	 * ib_create_srq() will initialize srq->ibsrq.
	 */
	spin_lock_init(&srq->rq.lock);
	srq->rq.wq->head = 0;
	srq->rq.wq->tail = 0;
	srq->limit = srq_init_attr->attr.srq_limit;

	spin_lock(&dev->n_srqs_lock);
	if (dev->n_srqs_allocated == ib_qib_max_srqs) {
		spin_unlock(&dev->n_srqs_lock);
		ret = ERR_PTR(-ENOMEM);
		goto bail_ip;
	}

	dev->n_srqs_allocated++;
	spin_unlock(&dev->n_srqs_lock);

	if (srq->ip) {
		spin_lock_irq(&dev->pending_lock);
		list_add(&srq->ip->pending_mmaps, &dev->pending_mmaps);
		spin_unlock_irq(&dev->pending_lock);
	}

	ret = &srq->ibsrq;
	goto done;

bail_ip:
	kfree(srq->ip);
bail_wq:
	vfree(srq->rq.wq);
bail_srq:
	kfree(srq);
done:
	return ret;
}

/**
 * qib_modify_srq - modify a shared receive queue
 * @ibsrq: the SRQ to modify
 * @attr: the new attributes of the SRQ
 * @attr_mask: indicates which attributes to modify
 * @udata: user data for libibverbs.so
 */
int qib_modify_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr,
		   enum ib_srq_attr_mask attr_mask,
		   struct ib_udata *udata)
{
	struct qib_srq *srq = to_isrq(ibsrq);
	struct qib_rwq *wq;
	int ret = 0;

	if (attr_mask & IB_SRQ_MAX_WR) {
		struct qib_rwq *owq;
		struct qib_rwqe *p;
		u32 sz, size, n, head, tail;

		/* Check that the requested sizes are below the limits. */
		if ((attr->max_wr > ib_qib_max_srq_wrs) ||
		    ((attr_mask & IB_SRQ_LIMIT) ?
		     attr->srq_limit : srq->limit) > attr->max_wr) {
			ret = -EINVAL;
			goto bail;
		}

		sz = sizeof(struct qib_rwqe) +
			srq->rq.max_sge * sizeof(struct ib_sge);
		size = attr->max_wr + 1;
		wq = vmalloc_user(sizeof(struct qib_rwq) + size * sz);
		if (!wq) {
			ret = -ENOMEM;
			goto bail;
		}

		/* Check that we can write the offset to mmap. */
		if (udata && udata->inlen >= sizeof(__u64)) {
			__u64 offset_addr;
			__u64 offset = 0;

			ret = ib_copy_from_udata(&offset_addr, udata,
						 sizeof(offset_addr));
			if (ret)
				goto bail_free;
			udata->outbuf =
				(void __user *) (unsigned long) offset_addr;
			ret = ib_copy_to_udata(udata, &offset,
					       sizeof(offset));
			if (ret)
				goto bail_free;
		}

		spin_lock_irq(&srq->rq.lock);
		/*
		 * validate head and tail pointer values and compute
		 * the number of remaining WQEs.
		 */
		owq = srq->rq.wq;
		head = owq->head;
		tail = owq->tail;
		if (head >= srq->rq.size || tail >= srq->rq.size) {
			ret = -EINVAL;
			goto bail_unlock;
		}
		n = head;
		if (n < tail)
			n += srq->rq.size - tail;
		else
			n -= tail;
		if (size <= n) {
			ret = -EINVAL;
			goto bail_unlock;
		}
		n = 0;
		p = wq->wq;
		while (tail != head) {
			struct qib_rwqe *wqe;
			int i;

			wqe = get_rwqe_ptr(&srq->rq, tail);
			p->wr_id = wqe->wr_id;
			p->num_sge = wqe->num_sge;
			for (i = 0; i < wqe->num_sge; i++)
				p->sg_list[i] = wqe->sg_list[i];
			n++;
			p = (struct qib_rwqe *)((char *) p + sz);
			if (++tail >= srq->rq.size)
				tail = 0;
		}
		srq->rq.wq = wq;
		srq->rq.size = size;
		wq->head = n;
		wq->tail = 0;
		if (attr_mask & IB_SRQ_LIMIT)
			srq->limit = attr->srq_limit;
		spin_unlock_irq(&srq->rq.lock);

		vfree(owq);

		if (srq->ip) {
			struct qib_mmap_info *ip = srq->ip;
			struct qib_ibdev *dev = to_idev(srq->ibsrq.device);
			u32 s = sizeof(struct qib_rwq) + size * sz;

			qib_update_mmap_info(dev, ip, s, wq);

			/*
			 * Return the offset to mmap.
			 * See qib_mmap() for details.
			 */
			if (udata && udata->inlen >= sizeof(__u64)) {
				ret = ib_copy_to_udata(udata, &ip->offset,
						       sizeof(ip->offset));
				if (ret)
					goto bail;
			}

			/*
			 * Put user mapping info onto the pending list
			 * unless it already is on the list.
			 */
			spin_lock_irq(&dev->pending_lock);
			if (list_empty(&ip->pending_mmaps))
				list_add(&ip->pending_mmaps,
					 &dev->pending_mmaps);
			spin_unlock_irq(&dev->pending_lock);
		}
	} else if (attr_mask & IB_SRQ_LIMIT) {
		spin_lock_irq(&srq->rq.lock);
		if (attr->srq_limit >= srq->rq.size)
			ret = -EINVAL;
		else
			srq->limit = attr->srq_limit;
		spin_unlock_irq(&srq->rq.lock);
	}
	goto bail;

bail_unlock:
	spin_unlock_irq(&srq->rq.lock);
bail_free:
	vfree(wq);
bail:
	return ret;
}

int qib_query_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr)
{
	struct qib_srq *srq = to_isrq(ibsrq);

	attr->max_wr = srq->rq.size - 1;
	attr->max_sge = srq->rq.max_sge;
	attr->srq_limit = srq->limit;
	return 0;
}

/**
 * qib_destroy_srq - destroy a shared receive queue
 * @ibsrq: the SRQ to destroy
 */
int qib_destroy_srq(struct ib_srq *ibsrq)
{
	struct qib_srq *srq = to_isrq(ibsrq);
	struct qib_ibdev *dev = to_idev(ibsrq->device);

	spin_lock(&dev->n_srqs_lock);
	dev->n_srqs_allocated--;
	spin_unlock(&dev->n_srqs_lock);
	if (srq->ip)
		kref_put(&srq->ip->ref, qib_release_mmap_info);
	else
		vfree(srq->rq.wq);
	kfree(srq);

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Copyright (c) 2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2006 PathScale, Inc. All rights reserved.
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
#include <linux/ctype.h>

#include "qib.h"
#include "qib_mad.h"

/* start of per-port functions */
/*
 * Get/Set heartbeat enable. OR of 1=enabled, 2=auto
 */
static ssize_t show_hrtbt_enb(struct qib_pportdata *ppd, char *buf)
{
	struct qib_devdata *dd = ppd->dd;
	int ret;

	ret = dd->f_get_ib_cfg(ppd, QIB_IB_CFG_HRTBT);
	ret = scnprintf(buf, PAGE_SIZE, "%d\n", ret);
	return ret;
}

static ssize_t store_hrtbt_enb(struct qib_pportdata *ppd, const char *buf,
			       size_t count)
{
	struct qib_devdata *dd = ppd->dd;
	int ret;
	u16 val;

	ret = kstrtou16(buf, 0, &val);
	if (ret) {
		qib_dev_err(dd, "attempt to set invalid Heartbeat enable\n");
		return ret;
	}

	/*
	 * Set the "intentional" heartbeat enable per either of
	 * "Enable" and "Auto", as these are normally set together.
	 * This bit is consulted when leaving loopback mode,
	 * because entering loopback mode overrides it and automatically
	 * disables heartbeat.
	 */
	ret = dd->f_set_ib_cfg(ppd, QIB_IB_CFG_HRTBT, val);
	return ret < 0 ? ret : count;
}

static ssize_t store_loopback(struct qib_pportdata *ppd, const char *buf,
			      size_t count)
{
	struct qib_devdata *dd = ppd->dd;
	int ret = count, r;

	r = dd->f_set_ib_loopback(ppd, buf);
	if (r < 0)
		ret = r;

	return ret;
}

static ssize_t store_led_override(struct qib_pportdata *ppd, const char *buf,
				  size_t count)
{
	struct qib_devdata *dd = ppd->dd;
	int ret;
	u16 val;

	ret = kstrtou16(buf, 0, &val);
	if (ret) {
		qib_dev_err(dd, "attempt to set invalid LED override\n");
		return ret;
	}

	qib_set_led_override(ppd, val);
	return count;
}

static ssize_t show_status(struct qib_pportdata *ppd, char *buf)
{
	ssize_t ret;

	if (!ppd->statusp)
		ret = -EINVAL;
	else
		ret = scnprintf(buf, PAGE_SIZE, "0x%llx\n",
				(unsigned long long) *(ppd->statusp));
	return ret;
}

/*
 * For userland compatibility, these offsets must remain fixed.
 * They are strings for QIB_STATUS_*
 */
static const char * const qib_status_str[] = {
	"Initted",
	"",
	"",
	"",
	"",
	"Present",
	"IB_link_up",
	"IB_configured",
	"",
	"Fatal_Hardware_Error",
	NULL,
};

static ssize_t show_status_str(struct qib_pportdata *ppd, char *buf)
{
	int i, any;
	u64 s;
	ssize_t ret;

	if (!ppd->statusp) {
		ret = -EINVAL;
		goto bail;
	}

	s = *(ppd->statusp);
	*buf = '\0';
	for (any = i = 0; s && qib_status_str[i]; i++) {
		if (s & 1) {
			/* if overflow */
			if (any && strlcat(buf, " ", PAGE_SIZE) >= PAGE_SIZE)
				break;
			if (strlcat(buf, qib_status_str[i], PAGE_SIZE) >=
					PAGE_SIZE)
				break;
			any = 1;
		}
		s >>= 1;
	}
	if (any)
		strlcat(buf, "\n", PAGE_SIZE);

	ret = strlen(buf);

bail:
	return ret;
}

/* end of per-port functions */

/*
 * Start of per-port file structures and support code
 * Because we are fitting into other infrastructure, we have to supply the
 * full set of kobject/sysfs_ops structures and routines.
 */
#define QIB_PORT_ATTR(name, mode, show, store) \
	static struct qib_port_attr qib_port_attr_##name = \
		__ATTR(name, mode, show, store)

struct qib_port_attr {
	struct attribute attr;
	ssize_t (*show)(struct qib_pportdata *, char *);
	ssize_t (*store)(struct qib_pportdata *, const char *, size_t);
};

QIB_PORT_ATTR(loopback, S_IWUSR, NULL, store_loopback);
QIB_PORT_ATTR(led_override, S_IWUSR, NULL, store_led_override);
QIB_PORT_ATTR(hrtbt_enable, S_IWUSR | S_IRUGO, show_hrtbt_enb,
	      store_hrtbt_enb);
QIB_PORT_ATTR(status, S_IRUGO, show_status, NULL);
QIB_PORT_ATTR(status_str, S_IRUGO, show_status_str, NULL);

static struct attribute *port_default_attributes[] = {
	&qib_port_attr_loopback.attr,
	&qib_port_attr_led_override.attr,
	&qib_port_attr_hrtbt_enable.attr,
	&qib_port_attr_status.attr,
	&qib_port_attr_status_str.attr,
	NULL
};

/*
 * Start of per-port congestion control structures and support code
 */

/*
 * Congestion control table size followed by table entries
 */
static ssize_t read_cc_table_bin(struct file *filp, struct kobject *kobj,
		struct bin_attribute *bin_attr,
		char *buf, loff_t pos, size_t count)
{
	int ret;
	struct qib_pportdata *ppd =
		container_of(kobj, struct qib_pportdata, pport_cc_kobj);

	if (!qib_cc_table_size || !ppd->ccti_entries_shadow)
		return -EINVAL;

	ret = ppd->total_cct_entry * sizeof(struct ib_cc_table_entry_shadow)
		 + sizeof(__be16);

	if (pos > ret)
		return -EINVAL;

	if (count > ret - pos)
		count = ret - pos;

	if (!count)
		return count;

	spin_lock(&ppd->cc_shadow_lock);
	memcpy(buf, ppd->ccti_entries_shadow, count);
	spin_unlock(&ppd->cc_shadow_lock);

	return count;
}

static void qib_port_release(struct kobject *kobj)
{
	/* nothing to do since memory is freed by qib_free_devdata() */
}

static struct kobj_type qib_port_cc_ktype = {
	.release = qib_port_release,
};

static struct bin_attribute cc_table_bin_attr = {
	.attr = {.name = "cc_table_bin", .mode = 0444},
	.read = read_cc_table_bin,
	.size = PAGE_SIZE,
};

/*
 * Congestion settings: port control, control map and an array of 16
 * entries for the congestion entries - increase, timer, event log
 * trigger threshold and the minimum injection rate delay.
 */
static ssize_t read_cc_setting_bin(struct file *filp, struct kobject *kobj,
		struct bin_attribute *bin_attr,
		char *buf, loff_t pos, size_t count)
{
	int ret;
	struct qib_pportdata *ppd =
		container_of(kobj, struct qib_pportdata, pport_cc_kobj);

	if (!qib_cc_table_size || !ppd->congestion_entries_shadow)
		return -EINVAL;

	ret = sizeof(struct ib_cc_congestion_setting_attr_shadow);

	if (pos > ret)
		return -EINVAL;
	if (count > ret - pos)
		count = ret - pos;

	if (!count)
		return count;

	spin_lock(&ppd->cc_shadow_lock);
	memcpy(buf, ppd->congestion_entries_shadow, count);
	spin_unlock(&ppd->cc_shadow_lock);

	return count;
}

static struct bin_attribute cc_setting_bin_attr = {
	.attr = {.name = "cc_settings_bin", .mode = 0444},
	.read = read_cc_setting_bin,
	.size = PAGE_SIZE,
};


static ssize_t qib_portattr_show(struct kobject *kobj,
	struct attribute *attr, char *buf)
{
	struct qib_port_attr *pattr =
		container_of(attr, struct qib_port_attr, attr);
	struct qib_pportdata *ppd =
		container_of(kobj, struct qib_pportdata, pport_kobj);

	if (!pattr->show)
		return -EIO;

	return pattr->show(ppd, buf);
}

static ssize_t qib_portattr_store(struct kobject *kobj,
	struct attribute *attr, const char *buf, size_t len)
{
	struct qib_port_attr *pattr =
		container_of(attr, struct qib_port_attr, attr);
	struct qib_pportdata *ppd =
		container_of(kobj, struct qib_pportdata, pport_kobj);

	if (!pattr->store)
		return -EIO;

	return pattr->store(ppd, buf, len);
}


static const struct sysfs_ops qib_port_ops = {
	.show = qib_portattr_show,
	.store = qib_portattr_store,
};

static struct kobj_type qib_port_ktype = {
	.release = qib_port_release,
	.sysfs_ops = &qib_port_ops,
	.default_attrs = port_default_attributes
};

/* Start sl2vl */

#define QIB_SL2VL_ATTR(N) \
	static struct qib_sl2vl_attr qib_sl2vl_attr_##N = { \
		.attr = { .name = __stringify(N), .mode = 0444 }, \
		.sl = N \
	}

struct qib_sl2vl_attr {
	struct attribute attr;
	int sl;
};

QIB_SL2VL_ATTR(0);
QIB_SL2VL_ATTR(1);
QIB_SL2VL_ATTR(2);
QIB_SL2VL_ATTR(3);
QIB_SL2VL_ATTR(4);
QIB_SL2VL_ATTR(5);
QIB_SL2VL_ATTR(6);
QIB_SL2VL_ATTR(7);
QIB_SL2VL_ATTR(8);
QIB_SL2VL_ATTR(9);
QIB_SL2VL_ATTR(10);
QIB_SL2VL_ATTR(11);
QIB_SL2VL_ATTR(12);
QIB_SL2VL_ATTR(13);
QIB_SL2VL_ATTR(14);
QIB_SL2VL_ATTR(15);

static struct attribute *sl2vl_default_attributes[] = {
	&qib_sl2vl_attr_0.attr,
	&qib_sl2vl_attr_1.attr,
	&qib_sl2vl_attr_2.attr,
	&qib_sl2vl_attr_3.attr,
	&qib_sl2vl_attr_4.attr,
	&qib_sl2vl_attr_5.attr,
	&qib_sl2vl_attr_6.attr,
	&qib_sl2vl_attr_7.attr,
	&qib_sl2vl_attr_8.attr,
	&qib_sl2vl_attr_9.attr,
	&qib_sl2vl_attr_10.attr,
	&qib_sl2vl_attr_11.attr,
	&qib_sl2vl_attr_12.attr,
	&qib_sl2vl_attr_13.attr,
	&qib_sl2vl_attr_14.attr,
	&qib_sl2vl_attr_15.attr,
	NULL
};

static ssize_t sl2vl_attr_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	struct qib_sl2vl_attr *sattr =
		container_of(attr, struct qib_sl2vl_attr, attr);
	struct qib_pportdata *ppd =
		container_of(kobj, struct qib_pportdata, sl2vl_kobj);
	struct qib_ibport *qibp = &ppd->ibport_dat