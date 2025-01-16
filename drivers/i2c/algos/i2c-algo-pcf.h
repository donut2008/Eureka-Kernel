/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006, 2007, 2008, 2010 QLogic Corporation. All rights reserved.
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
#include <linux/kthread.h>

#include "qib_verbs.h"
#include "qib.h"

/**
 * qib_cq_enter - add a new entry to the completion queue
 * @cq: completion queue
 * @entry: work completion entry to add
 * @sig: true if @entry is a solicitated entry
 *
 * This may be called with qp->s_lock held.
 */
void qib_cq_enter(struct qib_cq *cq, struct ib_wc *entry, int solicited)
{
	struct qib_cq_wc *wc;
	unsigned long flags;
	u32 head;
	u32 next;

	spin_lock_irqsave(&cq->lock, flags);

	/*
	 * Note that the head pointer might be writable by user processes.
	 * Take care to verify it is a sane value.
	 */
	wc = cq->queue;
	head = wc->head;
	if (head >= (unsigned) cq->ibcq.cqe) {
		head = cq->ibcq.cqe;
		next = 0;
	} else
		next = head + 1;
	if (unlikely(next == wc->tail)) {
		spin_unlock_irqrestore(&cq->lock, flags);
		if (cq->ibcq.event_handler) {
			struct ib_event ev;

			ev.device = cq->ibcq.device;
			ev.element.cq = &cq->ibcq;
			ev.event = IB_EVENT_CQ_ERR;
			cq->ibcq.event_handler(&ev, cq->ibcq.cq_context)