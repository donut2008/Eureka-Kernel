pkt->tidsm =
					(struct qib_tid_session_member *)tidsm;
				pkt->tidsmcount = tidsmsize/
					sizeof(struct qib_tid_session_member);
				pkt->tidsmidx = 0;
				idx++;
			}

			/*
			 * pbc 'fill1' field is borrowed to pass frag size,
			 * we need to clear it after picking frag size, the
			 * hardware requires this field to be zero.
			 */
			*pbc = cpu_to_le32(le32_to_cpu(*pbc) & 0x0000FFFF);
		} else {
			pkt = kmem_cache_alloc(pq->pkt_slab, GFP_KERNEL);
			if (!pkt) {
				ret = -ENOMEM;
				goto free_pbc;
			}
			pkt->largepkt = 0;
			pkt->frag_size = bytes_togo;
			pkt->addrlimit = ARRAY_SIZE(pkt->addr);
		}
		pkt->bytes_togo = bytes_togo;
		pkt->payload_size = 0;
		pkt->counter = counter;
		pkt->tiddma = tiddma;

		/* setup the first header */
		qib_user_sdma_init_frag(pkt, 0, /* index */
			0, len,		/* offset, len */
			1, 0,		/* first last desc */
			0, 0,		/* put page, dma mapped */
			NULL, pbc,	/* struct page, virt addr */
			dma_addr, len);	/* dma addr, dma length */
		pkt->index = 0;
		pkt->naddr = 1;

		if (nfrags) {
			ret = qib_user_sdma_init_payload(dd, pq, pkt,
							 iov + idx_save + 1,
							 nfrags, npages);
			if (ret < 0)
				goto free_pkt;
		} else {
			/* since there is no payload, mark the
			 * header as the last desc. */
			pkt->addr[0].last_desc = 1;

			if (dma_addr == 0) {
				/*
				 * the header is not dma mapped yet.
				 * it should be from kmalloc.
				 */
				dma_addr = dma_map_single(&dd->pcidev->dev,
					pbc, len, DMA_TO_DEVICE);
				if (dma_mapping_error(&dd->pcidev->dev,
								dma_addr)) {
					ret = -ENOMEM;
					goto free_pkt;
				}
				pkt->addr[0].addr = dma_addr;
				pkt->addr[0].dma_mapped = 1;
			}
		}

		counter++;
		npkts++;
		pkt->pq = pq;
		pkt->index = 0; /* reset index for push on hw */
		*ndesc += pkt->naddr;

		list_add_tail(&pkt->list, list);
	}

	*maxpkts = npkts;
	ret = idx;
	goto done;

free_pkt:
	if (pkt->largepkt)
		kfree(pkt);
	else
		kmem_cache_free(pq->pkt_slab, pkt);
free_pbc:
	if (dma_addr)
		dma_pool_free(pq->header_cache, pbc, dma_addr);
	else
		kfree(pbc);
free_list:
	qib_user_sdma_free_pkt_list(&dd->pcidev->dev, pq, list);
done:
	return ret;
}

static void qib_user_sdma_set_complete_counter(struct qib_user_sdma_queue *pq,
					       u32 c)
{
	pq->sent_counter = c;
}

/* try to clean out queue -- needs pq->lock */
static int qib_user_sdma_queue_clean(struct qib_pportdata *ppd,
				     struct qib_user_sdma_queue *pq)
{
	struct qib_devdata *dd = ppd->dd;
	struct list_head free_list;
	struct qib_user_sdma_pkt *pkt;
	struct qib_user_sdma_pkt *pkt_prev;
	unsigned long flags;
	int ret = 0;

	if (!pq->num_sending)
		return 0;

	INIT_LIST_HEAD(&free_list);

	/*
	 * We need this spin lock here because interrupt handler
	 * might modify this list in qib_user_sdma_send_desc(), also
	 * we can not get interrupted, otherwise it is a deadlock.
	 */
	spin_lock_irqsave(&pq->sent_lock, flags);
	list_for_each_entry_safe(pkt, pkt_prev, &pq->sent, list) {
		s64 descd = ppd->sdma_descq_removed - pkt->added;

		if (descd < 0)
			break;

		list_move_tail(&pkt->list, &free_list);

		/* one more packet cleaned */
		ret++;
		pq->num_sending--;
	}
	spin_unlock_irqrestore(&pq->sent_lock, flags);

	if (!list_empty(&free_list)) {
		u32 counter;

		pkt = list_entry(free_list.prev,
				 struct qib_user_sdma_pkt, list);
		counter = pkt->counter;

		qib_user_sdma_free_pkt_list(&dd->pcidev->dev, pq, &free_list);
		qib_user_sdma_set_complete_counter(pq, counter);
	}

	return ret;
}

void qib_user_sdma_queue_destroy(struct qib_user_sdma_queue *pq)
{
	if (!pq)
		return;

	pq->sdma_rb_node->refcount--;
	if (pq->sdma_rb_node->refcount == 0) {
		rb_erase(&pq->sdma_rb_node->node, &qib_user_sdma_rb_root);
		kfree(pq->sdma_rb_node);
	}
	dma_pool_destroy(pq->header_cache);
	kmem_cache_destroy(pq->pkt_slab);
	kfree(pq);
}

/* clean descriptor queue, returns > 0 if some elements cleaned */
static int qib_user_sdma_hwqueue_clean(struct qib_pportdata *ppd)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&ppd->sdma_lock, flags);
	ret = qib_sdma_make_progress(ppd);
	spin_unlock_irqrestore(&ppd->sdma_lock, flags);

	return ret;
}

/* we're in close, drain packets so that we can cleanup successfully... */
void qib_user_sdma_queue_drain(struct qib_pportdata *ppd,
			       struct qib_user_sdma_queue *pq)
{
	struct qib_devdata *dd = ppd->dd;
	unsigned long flags;
	int i;

	if (!pq)
		return;

	for (i = 0; i < QIB_USER_SDMA_DRAIN_TIMEOUT; i++) {
		mutex_lock(&pq->lock);
		if (!pq->num_pending && !pq->num_sending) {
			mutex_unlock(&pq->lock);
			break;
		}
		qib_user_sdma_hwqueue_clean(ppd);
		qib_user_sdma_queue_clean(ppd, pq);
		mutex_unlock(&pq->lock);
		msleep(20);
	}

	if (pq->num_pending || pq->num_sending) {
		struct qib_user_sdma_pkt *pkt;
		struct qib_user_sdma_pkt *pkt_prev;
		struct list_head free_list;

		mutex_lock(&pq->lock);
		spin_lock_irqsave(&ppd->sdma_lock, flags);
		/*
		 * Since we hold sdma_lock, it is safe without sent_lock.
		 */
		if (pq->num_pending) {
			list_for_each_entry_safe(pkt, pkt_prev,
					&ppd->sdma_userpending, list) {
				if (pkt->pq == pq) {
					list_move_tail(&pkt->list, &pq->sent);
					pq->num_pending--;
					pq->num_sending++;
				}
			}
		}
		spin_unlock_irqrestore(&ppd->sdma_lock, flags);

		qib_dev_err(dd, "user sdma lists not empty: forcing!\n");
		INIT_LIST_HEAD(&free_list);
		list_splice_init(&pq->sent, &free_list);
		pq->num_sending = 0;
		qib_user_sdma_free_pkt_list(&dd->pcidev->dev, pq, &free_list);
		mutex_unlock(&pq->lock);
	}
}

static inline __le64 qib_sdma_make_desc0(u8 gen,
					 u64 addr, u64 dwlen, u64 dwoffset)
{
	return cpu_to_le64(/* SDmaPhyAddr[31:0] */
			   ((addr & 0xfffffffcULL) << 32) |
			   /* SDmaGeneration[1:0] */
			   ((gen & 3ULL) << 30) |
			   /* SDmaDwordCount[10:0] */
			   ((dwlen & 0x7ffULL) << 16) |
			   /* SDmaBufOffset[12:2] */
			   (dwoffset & 0x7ffULL));
}

static inline __le64 qib_sdma_make_first_desc0(__le64 descq)
{
	return descq | cpu_to_le64(1ULL << 12);
}

static inline __le64 qib_sdma_make_last_desc0(__le64 descq)
{
					      /* last */  /* dma head */
	return descq | cpu_to_le64(1ULL << 11 | 1ULL << 13);
}

static inline __le64 qib_sdma_make_desc1(u64 addr)
{
	/* SDmaPhyAddr[47:32] */
	return cpu_to_le64(addr >> 32);
}

static void qib_user_sdma_send_frag(struct qib_pportdata *ppd,
				    struct qib_user_sdma_pkt *pkt, int idx,
				    unsigned ofs, u16 tail, u8 gen)
{
	const u64 addr = (u64) pkt->addr[idx].addr +
		(u64) pkt->addr[idx].offset;
	const u64 dwlen = (u64) pkt->addr[idx].length / 4;
	__le64 *descqp;
	__le64 descq0;

	descqp = &ppd->sdma_descq[tail].qw[0];

	descq0 = qib_sdma_make_desc0(gen, addr, dwlen, ofs);
	if (pkt->addr[idx].first_desc)
		descq0 = qib_sdma_make_first_desc0(descq0);
	if (pkt->addr[idx].last_desc) {
		descq0 = qib_sdma_make_last_desc0(descq0);
		if (ppd->sdma_intrequest) {
			descq0 |= cpu_to_le64(1ULL << 15);
			ppd->sdma_intrequest = 0;
		}
	}

	descqp[0] = descq0;
	descqp[1] = qib_sdma_make_desc1(addr);
}

void qib_user_sdma_send_desc(struct qib_pportdata *ppd,
				struct list_head *pktlist)
{
	struct qib_devdata *dd = ppd->dd;
	u16 nfree, nsent;
	u16 tail, tail_c;
	u8 gen, gen_c;

	nfree = qib_sdma_descq_freecnt(ppd);
	if (!nfree)
		return;

retry:
	nsent = 0;
	tail_c = tail = ppd->sdma_descq_tail;
	gen_c = gen = ppd->sdma_generation;
	while (!list_empty(pktlist)) {
		struct qib_user_sdma_pkt *pkt =
			list_entry(pktlist->next, struct qib_user_sdma_pkt,
				   list);
		int i, j, c = 0;
		unsigned ofs = 0;
		u16 dtail = tail;

		for (i = pkt->index; i < pkt->naddr && nfree; i++) {
			qib_user_sdma_send_frag(ppd, pkt, i, ofs, tail, gen);
			ofs += pkt->addr[i].length >> 2;

			if (++tail == ppd->sdma_descq_cnt) {
				tail = 0;
				++gen;
				ppd->sdma_intrequest = 1;
			} else if (tail == (ppd->sdma_descq_cnt>>1)) {
				ppd->sdma_intrequest = 1;
			}
			nfree--;
			if (pkt->addr[i].last_desc == 0)
				continue;

			/*
			 * If the packet is >= 2KB mtu equivalent, we
			 * have to use the large buffers, and have to
			 * mark each descriptor as part of a large
			 * buffer packet.
			 */
			if (ofs > dd->piosize2kmax_dwords) {
				for (j = pkt->index; j <= i; j++) {
					ppd->sdma_descq[dtail].qw[0] |=
						cpu_to_le64(1ULL << 14);
					if (++dtail == ppd->sdma_descq_cnt)
						dtail = 0;
				}
			}
			c += i + 1 - pkt->index;
			pkt->index = i + 1; /* index for next first */
			tail_c = dtail = tail;
			gen_c = gen;
			ofs = 0;  /* reset for next packet */
		}

		ppd->sdma_descq_added += c;
		nsent += c;
		if (pkt->index == pkt->naddr) {
			pkt->added = ppd->sdma_descq_added;
			pkt->pq->added = pkt->added;
			pkt->pq->num_pending--;
			spin_lock(&pkt->pq->sent_lock);
			pkt->pq->num_sending++;
			list_move_tail(&pkt->list, &pkt->pq->sent);
			spin_unlock(&pkt->pq->sent_lock);
		}
		if (!nfree || (nsent<<2) > ppd->sdma_descq_cnt)
			break;
	}

	/* advance the tail on the chip if necessary */
	if (ppd->sdma_descq_tail != tail_c) {
		ppd->sdma_generation = gen_c;
		dd->f_sdma_update_tail(ppd, tail_c);
	}

	if (nfree && !list_empty(pktlist))
		goto retry;
}

/* pq->lock must be held, get packets on the wire... */
static int qib_user_sdma_push_pkts(struct qib_pportdata *ppd,
				 struct qib_user_sdma_queue *pq,
				 struct list_head *pktlist, int count)
{
	unsigned long flags;

	if (unlikely(!(ppd->lflags & QIBL_LINKACTIVE)))
		return -ECOMM;

	/* non-blocking mode */
	if (pq->sdma_rb_node->refcount > 1) {
		spin_lock_irqsave(&ppd->sdma_lock, flags);
		if (unlikely(!__qib_sdma_running(ppd))) {
			spin_unlock_irqrestore(&ppd->sdma_lock, flags);
			return -ECOMM;
		}
		pq->num_pending += count;
		list_splice_tail_init(pktlist, &ppd->sdma_userpending);
		qib_user_sdma_send_desc(ppd, &ppd->sdma_userpending);
		spin_unlock_irqrestore(&ppd->sdma_lock, flags);
		return 0;
	}

	/* In this case, descriptors from this process are not
	 * linked to ppd pending queue, interrupt handler
	 * won't update this process, it is OK to directly
	 * modify without sdma lock.
	 */


	pq->num_pending += count;
	/*
	 * Blocking mode for single rail process, we must
	 * release/regain sdma_lock to give other process
	 * chance to make progress. This is important for
	 * performance.
	 */
	do {
		spin_lock_irqsave(&ppd->sdma_lock, flags);
		if (unlikely(!__qib_sdma_running(ppd))) {
			spin_unlock_irqrestore(&ppd->sdma_lock, flags);
			return -ECOMM;
		}
		qib_user_sdma_send_desc(ppd, pktlist);
		if (!list_empty(pktlist))
			qib_sdma_make_progress(ppd);
		spin_unlock_irqrestore(&ppd->sdma_lock, flags);
	} while (!list_empty(pktlist));

	return 0;
}

int qib_user_sdma_writev(struct qib_ctxtdata *rcd,
			 struct qib_user_sdma_queue *pq,
			 const struct iovec *iov,
			 unsigned long dim)
{
	struct qib_devdata *dd = rcd->dd;
	struct qib_pportdata *ppd = rcd->ppd;
	int ret = 0;
	struct list_head list;
	int npkts = 0;

	INIT_LIST_HEAD(&list);

	mutex_lock(&pq->lock);

	/* why not -ECOMM like qib_user_sdma_push_pkts() below? */
	if (!qib_sdma_running(ppd))
		goto done_unlock;

	/* if I have packets not complete yet */
	if (pq->added > ppd->sdma_descq_removed)
		qib_user_sdma_hwqueue_clean(ppd);
	/* if I have complete packets to be freed */
	if (pq->num_sending)
		qib_user_sdma_queue_clean(ppd, pq);

	while (dim) {
		int mxp = 1;
		int ndesc = 0;

		ret = qib_user_sdma_queue_pkts(dd, ppd, pq,
				iov, dim, &list, &mxp, &ndesc);
		if (ret < 0)
			goto done_unlock;
		else {
			dim -= ret;
			iov += ret;
		}

		/* force packets onto the sdma hw queue... */
		if (!list_empty(&list)) {
			/*
			 * Lazily clean hw queue.
			 */
			if (qib_sdma_descq_freecnt(ppd) < ndesc) {
				qib_user_sdma_hwqueue_clean(ppd);
				if (pq->num_sending)
					qib_user_sdma_queue_clean(ppd, pq);
			}

			ret = qib_user_sdma_push_pkts(ppd, pq, &list, mxp);
			if (ret < 0)
				goto done_unlock;
			else {
				npkts += mxp;
				pq->counter += mxp;
			}
		}
	}

done_unlock:
	if (!list_empty(&list))
		qib_user_sdma_free_pkt_list(&dd->pcidev->dev, pq, &list);
	mutex_unlock(&pq->lock);

	return (ret < 0) ? ret : npkts;
}

int qib_user_sdma_make_progress(struct qib_pportdata *ppd,
				struct qib_user_sdma_queue *pq)
{
	int ret = 0;

	mutex_lock(&pq->lock);
	qib_user_sdma_hwqueue_clean(ppd);
	ret = qib_user_sdma_queue_clean(ppd, pq);
	mutex_unlock(&pq->lock);

	return ret;
}

u32 qib_user_sdma_complete_counter(const struct qib_user_sdma_queue *pq)
{
	return pq ? pq->sent_counter : 0;
}

u32 qib_user_sdma_inflight_counter(struct qib_user_sdma_queue *pq)
{
	return pq ? pq->counter : 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * Copyright (c) 2007, 2008 QLogic Corporation. All rights reserved.
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
#include <linux/device.h>

struct qib_user_sdma_queue;

struct qib_user_sdma_queue *
qib_user_sdma_queue_create(struct device *dev, int unit, int port, int sport);
void qib_user_sdma_queue_destroy(struct qib_user_sdma_queue *pq);

int qib_user_sdma_writev(struct qib_ctxtdata *pd,
			 struct qib_user_sdma_queue *pq,
			 const struct iovec *iov,
			 unsigned long dim);

int qib_user_sdma_make_progress(struct qib_pportdata *ppd,
				struct qib_user_sdma_queue *pq);

void qib_user_sdma_queue_drain(struct qib_pportdata *ppd,
			       struct qib_user_sdma_queue *pq);

u32 qib_user_sdma_complete_counter(const struct qib_user_sdma_queue *pq);
u32 qib_user_sdma_inflight_counter(struct qib_user_sdma_queue *pq);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
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

#include <rdma/ib_mad.h>
#include <rdma/ib_user_verbs.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/utsname.h>
#include <linux/rculist.h>
#include <linux/mm.h>
#include <linux/random.h>
#include <linux/vmalloc.h>

#include "qib.h"
#include "qib_common.h"

static unsigned int ib_qib_qp_table_size = 256;
module_param_named(qp_table_size, ib_qib_qp_table_size, uint, S_IRUGO);
MODULE_PARM_DESC(qp_table_size, "QP table size");

unsigned int ib_qib_lkey_table_size = 16;
module_param_named(lkey_table_size, ib_qib_lkey_table_size, uint,
		   S_IRUGO);
MODULE_PARM_DESC(lkey_table_size,
		 "LKEY table size in bits (2^n, 1 <= n <= 23)");

static unsigned int ib_qib_max_pds = 0xFFFF;
module_param_named(max_pds, ib_qib_max_pds, uint, S_IRUGO);
MODULE_PARM_DESC(max_pds,
		 "Maximum number of protection domains to support");

static unsigned int ib_qib_max_ahs = 0xFFFF;
module_param_named(max_ahs, ib_qib_max_ahs, uint, S_IRUGO);
MODULE_PARM_DESC(max_ahs, "Maximum number of address handles to support");

unsigned int ib_qib_max_cqes = 0x2FFFF;
module_param_named(max_cqes, ib_qib_max_cqes, uint, S_IRUGO);
MODULE_PARM_DESC(max_cqes,
		 "Maximum number of completion queue entries to support");

unsigned int ib_qib_max_cqs = 0x1FFFF;
module_param_named(max_cqs, ib_qib_max_cqs, uint, S_IRUGO);
MODULE_PARM_DESC(max_cqs, "Maximum number of completion queues to support");

unsigned int ib_qib_max_qp_wrs = 0x3FFF;
module_param_named(max_qp_wrs, ib_qib_max_qp_wrs, uint, S_IRUGO);
MODULE_PARM_DESC(max_qp_wrs, "Maximum number of QP WRs to support");

unsigned int ib_qib_max_q