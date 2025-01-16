/*
 * Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation.  * All rights reserved.
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
#include <linux/vmalloc.h>
#include <linux/jhash.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/seq_file.h>
#endif

#include "qib.h"

#define RVT_BITS_PER_PAGE           (PAGE_SIZE*BITS_PER_BYTE)
#define RVT_BITS_PER_PAGE_MASK      (RVT_BITS_PER_PAGE-1)

static inline unsigned mk_qpn(struct qib_qpn_table *qpt,
			      struct qpn_map *map, unsigned off)
{
	return (map - qpt->map) * RVT_BITS_PER_PAGE + off;
}

static inline unsigned find_next_offset(struct qib_qpn_table *qpt,
					struct qpn_map *map, unsigned off,
					unsigned n)
{
	if (qpt->mask) {
		off++;
		if (((off & qpt->mask) >> 1) >= n)
			off = (off | qpt->mask) + 2;
	} else
		off = find_next_zero_bit(map->page, RVT_BITS_PER_PAGE, off);
	return off;
}

/*
 * Convert the AETH credit code into the number of credits.
 */
static u32 credit_table[31] = {
	0,                      /* 0 */
	1,                      /* 1 */
	2,                      /* 2 */
	3,                      /* 3 */
	4,                      /* 4 */
	6,                      /* 5 */
	8,                      /* 6 */
	12,                     /* 7 */
	16,                     /* 8 */
	24,                     /* 9 */
	32,                     /* A */
	48,                     /* B */
	64,                     /* C */
	96,                     /* D */
	128,                    /* E */
	192,                    /* F */
	256,                    /* 10 */
	384,                    /* 11 */
	512,                    /* 12 */
	768,                    /* 13 */
	1024,                   /* 14 */
	1536,                   /* 15 */
	2048,                   /* 16 */
	3072,                   /* 17 */
	4096,                   /* 18 */
	6144,                   /* 19 */
	8192,                   /* 1A */
	12288,                  /* 1B */
	16384,                  /* 1C */
	24576,                  /* 1D */
	32768                   /* 1E */
};

static void get_map_page(struct qib_qpn_table *qpt, struct qpn_map *map,
			 gfp_t gfp)
{
	unsigned long page = get_zeroed_page(gfp);

	/*
	 * Free the page if someone raced with us installing it.
	 */

	spin_lock(&qpt->lock);
	if (map->page)
		free_page(page);
	else
		map->page = (void *)page;
	spin_unlock(&qpt->lock);
}

/*
 * Allocate the next available QPN or
 * zero/one for QP type IB_QPT_SMI/IB_QPT_GSI.
 */
static int alloc_qpn(struct qib_devdata *dd, struct qib_qpn_table *qpt,
		     enum ib_qp_type type, u8 port, gfp_t gfp)
{
	u32 i, offset, max_scan, qpn;
	struct qpn_map *map;
	u32 ret;

	if (type == IB_QPT_SMI || type == IB_QPT_GSI) {
		unsigned n;

		ret = type == IB_QPT_GSI;
		n = 1 << (ret + 2 * (port - 1));
		spin_lock(&qpt->lock);
		if (qpt->flags & n)
			ret = -EINVAL;
		else
			qpt->flags |= n;
		spin_unlock(&qpt->lock);
		goto bail;
	}

	qpn = qpt->last + 2;
	if (qpn >= QPN_MAX)
		qpn = 2;
	if (qpt->mask && ((qpn & qpt->mask) >> 1) >= dd->n_krcv_queues)
		qpn = (qpn | qpt->mask) + 2;
	offset = qpn & RVT_BITS_PER_PAGE_MASK;
	map = &qpt->map[qpn / RVT_BITS_PER_PAGE];
	max_scan = qpt->nmaps - !offset;
	for (i = 0;;) {
		if (unlikely(!map->page)) {
			get_map_page(qpt, map, gfp);
			if (unlikely(!map->page))
				break;
		}
		do {
			if (!test_and_set_bit(offset, map->page)) {
				qpt->last = qpn;
				ret = qpn;
				goto bail;
			}
			offset = find_next_offset(qpt, map, offset,
				dd->n_krcv_queues);
			qpn = mk_qpn(qpt, map, offset);
			/*
			 * This test differs from alloc_pidmap().
			 * If find_next_offset() does find a zero
			 * bit, we don't need to check for QPN
			 * wrapping around past our starting QPN.
			 * We just need to be sure we don't loop
			 * forever.
			 */
		} while (offset < RVT_BITS_PER_PAGE && qpn < QPN_MAX);
		/*
		 * In order to keep the number of pages allocated to a
		 * minimum, we scan the all existing pages before increasing
		 * the size of the bitmap table.
		 */
		if (++i > max_scan) {
			if (qpt->nmaps == QPNMAP_ENTRIES)
				break;
			map = &qpt->map[qpt->nmaps++];
			offset = 0;
		} else if (map < &qpt->map[qpt->nmaps]) {
			++map;
			offset = 0;
		} else {
			map = &qpt->map[0];
			offset = 2;
		}
		qpn = mk_qpn(qpt, map, offset);
	}

	ret = -ENOMEM;

bail:
	return ret;
}

static void free_qpn(struct qib_qpn_table *qpt, u32 qpn)
{
	struct qpn_map *map;

	map = qpt->map + qpn / RVT_BITS_PER_PAGE;
	if (map->page)
		clear_bit(qpn & RVT_BITS_PER_PAGE_MASK, map->page);
}

static inline unsigned qpn_hash(struct qib_ibdev *dev, u32 qpn)
{
	return jhash_1word(qpn, dev->qp_rnd) &
		(dev->qp_table_size - 1);
}


/*
 * Put the QP into the hash table.
 * The hash table holds a reference to the QP.
 */
static void insert_qp(struct qib_ibdev *dev, struct qib_qp *qp)
{
	struct qib_ibport *ibp = to_iport(qp->ibqp.device, qp->port_num);
	unsigned long flags;
	unsigned n = qpn_hash(dev, qp->ibqp.qp_num);

	atomic_inc(&qp->refcount);
	spin_lock_irqsave(&dev->qpt_lock, flags);

	if (qp->ibqp.qp_num == 0)
		rcu_assign_pointer(ibp->qp0, qp);
	else if (qp->ibqp.qp_num == 1)
		rcu_assign_pointer(ibp->qp1, qp);
	else {
		qp->next = dev->qp_table[n];
		rcu_assign_pointer(dev->qp_table[n], qp);
	}

	spin_unlock_irqrestore(&dev->qpt_lock, flags);
}

/*
 * Remove the QP from the table so it can't be found asynchronously by
 * the receive interrupt routine.
 */
static void remove_qp(struct qib_ibdev *dev, struct qib_qp *qp)
{
	struct qib_ibport *ibp = to_iport(qp->ibqp.device, qp->port_num);
	unsigned n = qpn_hash(dev, qp->ibqp.qp_num);
	unsigned long flags;
	int removed = 1;

	spin_lock_irqsave(&dev->qpt_lock, flags);

	if (rcu_dereference_protected(ibp->qp0,
			lockdep_is_held(&dev->qpt_lock)) == qp) {
		RCU_INIT_POINTER(ibp->qp0, NULL);
	} else if (rcu_dereference_protected(ibp->qp1,
			lockdep_is_held(&dev->qpt_lock)) == qp) {
		RCU_INIT_POINTER(ibp->qp1, NULL);
	} else {
		struct qib_qp *q;
		struct qib_qp __rcu **qpp;

		re