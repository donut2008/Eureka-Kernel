/*
 * Intel I/OAT DMA Linux driver
 * Copyright(c) 2004 - 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/dmaengine.h>
#include <linux/pci.h>
#include "dma.h"
#include "registers.h"
#include "hw.h"

#include "../dmaengine.h"

static ssize_t cap_show(struct dma_chan *c, char *page)
{
	struct dma_device *dma = c->device;

	return sprintf(page, "copy%s%s%s%s%s\n",
		       dma_has_cap(DMA_PQ, dma->cap_mask) ? " pq" : "",
		       dma_has_cap(DMA_PQ_VAL, dma->cap_mask) ? " pq_val" : "",
		       dma_has_cap(DMA_XOR, dma->cap_mask) ? " xor" : "",
		       dma_has_cap(DMA_XOR_VAL, dma->cap_mask) ? " xor_val" : "",
		       dma_has_cap(DMA_INTERRUPT, dma->cap_mask) ? " intr" : "");

}
struct ioat_sysfs_entry ioat_cap_attr = __ATTR_RO(cap);

static ssize_t version_show(struct dma_chan *c, char *page)
{
	struct dma_device *dma = c->device;
	struct ioatdma_device *ioat_dma = to_ioatdma_device(dma);

	return sprintf(page, "%d.%d\n",
		       ioat_dma->version >> 4, ioat_dma->version & 0xf);
}
struct ioat_sysfs_entry ioat_version_attr = __ATTR_RO(version);

static ssize_t
ioat_attr_show(struct kobject *kobj, struct attribute *attr, char *page)
{
	struct ioat_sysfs_entry *entry;
	struct ioatdma_chan *ioat_chan;

	entry = container_of(attr, struct ioat_sysfs_entry, attr);
	ioat_chan = container_of(kobj, struct ioatdma_chan, kobj);

	if (!entry->show)
		return -EIO;
	return entry->show(&ioat_chan->dma_chan, page);
}

const struct sysfs_ops ioat_sysfs_ops = {
	.show	= ioat_attr_show,
};

void ioat_kobject_add(struct ioatdma_device *ioat_dma, struct kobj_type *type)
{
	struct dma_device *dma = &ioat_dma->dma_dev;
	struct dma_chan *c;

	list_for_each_entry(c, &dma->channels, device_node) {
		struct ioatdma_chan *ioat_chan = to_ioat_chan(c);
		struct kobject *parent = &c->dev->device.kobj;
		int err;

		err = kobject_init_and_add(&ioat_chan->kobj, type,
					   parent, "quickdata");
		if (err) {
			dev_warn(to_dev(ioat_chan),
				 "sysfs init error (%d), continuing...\n", err);
			kobject_put(&ioat_chan->kobj);
			set_bit(IOAT_KOBJ_INIT_FAIL, &ioat_chan->state);
		}
	}
}

void ioat_kobject_del(struct ioatdma_device *ioat_dma)
{
	struct dma_device *dma = &ioat_dma->dma_dev;
	struct dma_chan *c;

	list_for_each_entry(c, &dma->channels, device_node) {
		struct ioatdma_chan *ioat_chan = to_ioat_chan(c);

		if (!test_bit(IOAT_KOBJ_INIT_FAIL, &ioat_chan->state)) {
			kobject_del(&ioat_chan->kobj);
			kobject_put(&ioat_chan->kobj);
		}
	}
}

static ssize_t ring_size_show(struct dma_chan *c, char *page)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);

	return sprintf(page, "%d\n", (1 << ioat_chan->alloc_order) & ~1);
}
static struct ioat_sysfs_entry ring_size_attr = __ATTR_RO(ring_size);

static ssize_t ring_active_show(struct dma_chan *c, char *page)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);

	/* ...taken outside the lock, no need to be precise */
	return sprintf(page, "%d\n", ioat_ring_active(ioat_chan));
}
static struct ioat_sysfs_entry ring_active_attr = __ATTR_RO(ring_active);

static struct attribute *ioat_attrs[] = {
	&ring_size_attr.attr,
	&ring_active_attr.attr,
	&ioat_cap_attr.attr,
	&ioat_version_attr.attr,
	NULL,
};

struct kobj_type ioat_ktype = {
	.sysfs_ops = &ioat_sysfs_ops,
	.default_attrs = ioat_attrs,
};
                                                                                                                                                                                                                  /*
 * offload engine driver for the Intel Xscale series of i/o processors
 * Copyright © 2006, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

/*
 * This driver supports the asynchrounous DMA copy and RAID engines available
 * on the Intel Xscale(R) family of I/O Processors (IOP 32x, 33x, 134x)
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/memory.h>
#include <linux/ioport.h>
#include <linux/raid/pq.h>
#include <linux/slab.h>

#include <mach/adma.h>

#include "dmaengine.h"

#define to_iop_adma_chan(chan) container_of(chan, struct iop_adma_chan, common)
#define to_iop_adma_device(dev) \
	container_of(dev, struct iop_adma_device, common)
#define tx_to_iop_adma_slot(tx) \
	container_of(tx, struct iop_adma_desc_slot, async_tx)

/**
 * iop_adma_free_slots - flags descriptor slots for reuse
 * @slot: Slot to free
 * Caller must hold &iop_chan->lock while calling this function
 */
static void iop_adma_free_slots(struct iop_adma_desc_slot *slot)
{
	int stride = slot->slots_per_op;

	while (stride--) {
		slot->slots_per_op = 0;
		slot = list_entry(slot->slot_node.next,
				struct iop_adma_desc_slot,
				slot_node);
	}
}

static dma_cookie_t
iop_adma_run_tx_complete_actions(struct iop_adma_desc_slot *desc,
	struct iop_adma_chan *iop_chan, dma_cookie_t cookie)
{
	struct dma_async_tx_descriptor *tx = &desc->async_tx;

	BUG_ON(tx->cookie < 0);
	if (tx->cookie > 0) {
		cookie = tx->cookie;
		tx->cookie = 0;

		/* call the callback (must not sleep or submit new
		 * operations to this channel)
		 */
		if (tx->callback)
			tx->callback(tx->callback_param);

		dma_descriptor_unmap(tx);
		if (desc->group_head)
			desc->group_head = NULL;
	}

	/* run dependent operations */
	dma_run_dependencies(tx);

	return cookie;
}

static int
iop_adma_clean_slot(struct iop_adma_desc_slot *desc,
	struct iop_adma_chan *iop_chan)
{
	/* the client is allowed to attach dependent operations
	 * until 'ack' is set
	 */
	if (!async_tx_test_ack(&desc->async_tx))
		return 0;

	/* leave the last descriptor in the chain
	 * so we can append to it
	 */
	if (desc->chain_node.next == &iop_chan->chain)
		return 1;

	dev_dbg(iop_chan->device->common.dev,
		"\tfree slot: %d slots_per_op: %d\n",
		desc->idx, desc->slots_per_op);

	list_del(&desc->chain_node);
	iop_adma_free_slots(desc);

	return 0;
}

static void __iop_adma_slot_cleanup(struct iop_adma_chan *iop_chan)
{
	struct iop_adma_desc_slot *iter, *_iter, *grp_start = NULL;
	dma_cookie_t cookie = 0;
	u32 current_desc = iop_chan_get_current_descriptor(iop_chan);
	int busy = iop_chan_is_busy(iop_chan);
	int seen_current = 0, slot_cnt = 0, slots_per_op = 0;

	dev_dbg(iop_chan->device->common.dev, "%s\n", __func__);
	/* free completed slots from the chain starting with
	 * the oldest descriptor
	 */
	list_for_each_entry_safe(iter, _iter, &iop_chan->chain,
					chain_node) {
		pr_debug("\tcookie: %d slot: %d busy: %d "
			"this_desc: %#x next_desc: %#llx ack: %d\n",
			iter->async_tx.cookie, iter->idx, busy,
			iter->async_tx.phys, (u64)iop_desc_get_next_desc(iter),
			async_tx_test_ack(&iter->async_tx));
		prefetch(_iter);
		prefetch(&_iter->async_tx);

		/* do not advance past the current descriptor loaded into the
		 * hardware channel, subsequent descriptors are either in
		 * process or have not been submitted
		 */
		if (seen_current)
			break;

		/* stop the search if we reach the current descriptor and the
		 * channel is busy, or if it appears that the current descriptor
		 * needs to be re-read (i.e. has been appended to)
		 */
		if (iter->async_tx.phys == current_desc) {
			BUG_ON(seen_current++);
			if (busy || iop_desc_get_next_desc(iter))
				break;
		}

		/* detect the start of a group transaction */
		if (!slot_cnt && !slots_per_op) {
			slot_cnt = iter->slot_cnt;
			slots_per_op = iter->slots_per_op;
			if (slot_cnt <= slots_per_op) {
				slot_cnt = 0;
				slots_per_op = 0;
			}
		}

		if (slot_cnt) {
			pr_debug("\tgroup++\n");
			if (!grp_start)
				grp_start = iter;
			slot_cnt -= slots_per_op;
		}

		/* all the members of a group are complete */
		if (slots_per_op != 0 && slot_cnt == 0) {
			struct iop_adma_desc_slot *grp_iter, *_grp_iter;
			int end_of_chain = 0;
			pr_debug("\tgroup end\n");

			/* collect the total results */
			if (grp_start->xor_check_result) {
				u32 zero_sum_result = 0;
				slot_cnt = grp_start->slot_cnt;
				grp_iter = grp_start;

				list_for_each_entry_from(grp_iter,
					&iop_chan->chain, chain_node) {
					zero_sum_result |=
					    iop_desc_get_zero_result(grp_iter);
					    pr_debug("\titer%d result: %d\n",
					    grp_iter->idx, zero_sum_result);
					slot_cnt -= slots_per_op;
					if (slot_cnt == 0)
						break;
				}
				pr_debug("\tgrp_start->xor_check_result: %p\n",
					grp_start->xor_check_result);
				*grp_start->xor_check_result = zero_sum_result;
			}

			/* clean up the group */
			slot_cnt = grp_start->slot_cnt;
			grp_iter = grp_start;
			list_for_each_entry_safe_from(grp_iter, _grp_iter,
				&iop_chan->chain, chain_node) {
				cookie = iop_adma_run_tx_complete_actions(
					grp_iter, iop_chan, cookie);

				slot_cnt -= slots_per_op;
				end_of_chain = iop_adma_clean_slot(grp_iter,
					iop_chan);

				if (slot_cnt == 0 || end_of_chain)
					break;
			}

			/* the group should be complete at this point */
			BUG_ON(slot_cnt);

			slots_per_op = 0;
			grp_start = NULL;
			if (end_of_chain)
				break;
			else
				continue;
		} else if (slots_per_op) /* wait for group completion */
			continue;

		/* write back zero sum results (single descriptor case) */
		if (iter->xor_check_result && iter->async_tx.cookie)
			*iter->xor_check_result =
				iop_desc_get_zero_result(iter);

		cookie = iop_adma_run_tx_complete_actions(
					iter, iop_chan, cookie);

		if (iop_adma_clean_slot(iter, iop_chan))
			break;
	}

	if (cookie > 0) {
		iop_chan->common.completed_cookie = cookie;
		pr_debug("\tcompleted cookie %d\n", cookie);
	}
}

static void
iop_adma_slot_cleanup(struct iop_adma_chan *iop_chan)
{
	spin_lock_bh(&iop_chan->lock);
	__iop_adma_slot_cleanup(iop_chan);
	spin_unlock_bh(&iop_chan->lock);
}

static void iop_adma_tasklet(unsigned long data)
{
	struct iop_adma_chan *iop_chan = (struct iop_adma_chan *) data;

	/* lockdep will flag depedency submissions as potentially
	 * recursive locking, this is not the case as a dependency
	 * submission will never recurse a channels submit routine.
	 * There are checks in async_tx.c to prevent this.
	 */
	spin_lock_nested(&iop_chan->lock, SINGLE_DEPTH_NESTING);
	__iop_adma_slot_cleanup(iop_chan);
	spin_unlock(&iop_chan->lock);
}

static struct iop_adma_desc_slot *
iop_adma_alloc_slots(struct iop_adma_chan *iop_chan, int num_slots,
			int slots_per_op)
{
	struct iop_adma_desc_slot *iter, *_iter, *alloc_start = NULL;
	LIST_HEAD(chain);
	int slots_found, retry = 0;

	/* start search from the last allocated descrtiptor
	 * if a contiguous allocation can not be found start searching
	 * from the beginning of the list
	 */
retry:
	slots_found = 0;
	if (retry == 0)
		iter = iop_chan->last_used;
	else
		iter = list_entry(&iop_chan->all_slots,
			struct iop_adma_desc_slot,
			slot_node);

	list_for_each_entry_safe_continue(
		iter, _iter, &iop_chan->all_slots, slot_node) {
		prefetch(_iter);
		prefetch(&_iter->async_tx);
		if (iter->slots_per_op) {
			/* give up after finding the first busy slot
			 * on the second pass through the list
			 */
			if (retry)
				break;

			slots_found = 0;
			continue;
		}

		/* start the allocation if the slot is correctly aligned */
		if (!slots_found++) {
			if (iop_desc_is_aligned(iter, slots_per_op))
				alloc_start = iter;
			else {
				slots_found = 0;
				continue;
			}
		}

		if (slots_found == num_slots) {
			struct iop_adma_desc_slot *alloc_tail = NULL;
			struct iop_adma_desc_slot *last_used = NULL;
			iter = alloc_start;
			while (num_slots) {
				int i;
				dev_dbg(iop_chan->device->common.dev,
					"allocated slot: %d "
					"(desc %p phys: %#llx) slots_per_op %d\n",
					iter->idx, iter->hw_desc,
					(u64)iter->async_tx.phys, slots_per_op);

				/* pre-ack all but the last descriptor */
				if (num_slots != slots_per_op)
					async_tx_ack(&iter->async_tx);

				list_add_tail(&iter->chain_node, &chain);
				alloc_tail = iter;
				iter->async_tx.cookie = 0;
				iter->slot_cnt = num_slots;
				iter->xor_check_result = NULL;
				for (i = 0; i < slots_per_op; i++) {
					iter->slots_per_op = slots_per_op - i;
					last_used = iter;
					iter = list_entry(iter->slot_node.next,
						struct iop_adma_desc_slot,
						slot_node);
				}
				num_slots -= slots_per_op;
			}
			alloc_tail->group_head = alloc_start;
			alloc_tail->async_tx.cookie = -EBUSY;
			list_splice(&chain, &alloc_tail->tx_list);
			iop_chan->last_used = last_used;
			iop_desc_clear_next_desc(alloc_start);
			iop_desc_clear_next_desc(alloc_tail);
			return alloc_tail;
		}
	}
	if (!retry++)
		goto retry;

	/* perform direct reclaim if the allocation fails */
	__iop_adma_slot_cleanup(iop_chan);

	return NULL;
}

static void iop_adma_check_threshold(struct iop_adma_chan *iop_chan)
{
	dev_dbg(iop_chan->device->common.dev, "pending: %d\n",
		iop_chan->pending);

	if (iop_chan->pending >= IOP_ADMA_THRESHOLD) {
		iop_chan->pending = 0;
		iop_chan_append(iop_chan);
	}
}

static dma_cookie_t
iop_adma_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct iop_adma_desc_slot *sw_desc = tx_to_iop_adma_slot(tx);
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(tx->chan);
	struct iop_adma_desc_slot *grp_start, *old_chain_tail;
	int slot_cnt;
	int slots_per_op;
	dma_cookie_t cookie;
	dma_addr_t next_dma;

	grp_start = sw_desc->group_head;
	slot_cnt = grp_start->slot_cnt;
	slots_per_op = grp_start->slots_per_op;

	spin_lock_bh(&iop_chan->lock);
	cookie = dma_cookie_assign(tx);

	old_chain_tail = list_entry(iop_chan->chain.prev,
		struct iop_adma_desc_slot, chain_node);
	list_splice_init(&sw_desc->tx_list,
			 &old_chain_tail->chain_node);

	/* fix up the hardware chain */
	next_dma = grp_start->async_tx.phys;
	iop_desc_set_next_desc(old_chain_tail, next_dma);
	BUG_ON(iop_desc_get_next_desc(old_chain_tail) != next_dma); /* flush */

	/* check for pre-chained descriptors */
	iop_paranoia(iop_desc_get_next_desc(sw_desc));

	/* increment the pending count by the number of slots
	 * memcpy operations have a 1:1 (slot:operation) relation
	 * other operations are heavier and will pop the threshold
	 * more often.
	 */
	iop_chan->pending += slot_cnt;
	iop_adma_check_threshold(iop_chan);
	spin_unlock_bh(&iop_chan->lock);

	dev_dbg(iop_chan->device->common.dev, "%s cookie: %d slot: %d\n",
		__func__, sw_desc->async_tx.cookie, sw_desc->idx);

	return cookie;
}

static void iop_chan_start_null_memcpy(struct iop_adma_chan *iop_chan);
static void iop_chan_start_null_xor(struct iop_adma_chan *iop_chan);

/**
 * iop_adma_alloc_chan_resources -  returns the number of allocated descriptors
 * @chan - allocate descriptor resources for this channel
 * @client - current client requesting the channel be ready for requests
 *
 * Note: We keep the slots for 1 operation on iop_chan->chain at all times.  To
 * avoid deadlock, via async_xor, num_descs_in_pool must at a minimum be
 * greater than 2x the number slots needed to satisfy a device->max_xor
 * request.
 * */
static int iop_adma_alloc_chan_resources(struct dma_chan *chan)
{
	char *hw_desc;
	int idx;
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(chan);
	struct iop_adma_desc_slot *slot = NULL;
	int init = iop_chan->slots_allocated ? 0 : 1;
	struct iop_adma_platform_data *plat_data =
		dev_get_platdata(&iop_chan->device->pdev->dev);
	int num_descs_in_pool = plat_data->pool_size/IOP_ADMA_SLOT_SIZE;

	/* Allocate descriptor slots */
	do {
		idx = iop_chan->slots_allocated;
		if (idx == num_descs_in_pool)
			break;

		slot = kzalloc(sizeof(*slot), GFP_KERNEL);
		if (!slot) {
			printk(KERN_INFO "IOP ADMA Channel only initialized"
				" %d descriptor slots", idx);
			break;
		}
		hw_desc = (char *) iop_chan->device->dma_desc_pool_virt;
		slot->hw_desc = (void *) &hw_desc[idx * IOP_ADMA_SLOT_SIZE];

		dma_async_tx_descriptor_init(&slot->async_tx, chan);
		slot->async_tx.tx_submit = iop_adma_tx_submit;
		INIT_LIST_HEAD(&slot->tx_list);
		INIT_LIST_HEAD(&slot->chain_node);
		INIT_LIST_HEAD(&slot->slot_node);
		hw_desc = (char *) iop_chan->device->dma_desc_pool;
		slot->async_tx.phys =
			(dma_addr_t) &hw_desc[idx * IOP_ADMA_SLOT_SIZE];
		slot->idx = idx;

		spin_lock_bh(&iop_chan->lock);
		iop_chan->slots_allocated++;
		list_add_tail(&slot->slot_node, &iop_chan->all_slots);
		spin_unlock_bh(&iop_chan->lock);
	} while (iop_chan->slots_allocated < num_descs_in_pool);

	if (idx && !iop_chan->last_used)
		iop_chan->last_used = list_entry(iop_chan->all_slots.next,
					struct iop_adma_desc_slot,
					slot_node);

	dev_dbg(iop_chan->device->common.dev,
		"allocated %d descriptor slots last_used: %p\n",
		iop_chan->slots_allocated, iop_chan->last_used);

	/* initialize the channel and the chain with a null operation */
	if (init) {
		if (dma_has_cap(DMA_MEMCPY,
			iop_chan->device->common.cap_mask))
			iop_chan_start_null_memcpy(iop_chan);
		else if (dma_has_cap(DMA_XOR,
			iop_chan->device->common.cap_mask))
			iop_chan_start_null_xor(iop_chan);
		else
			BUG();
	}

	return (idx > 0) ? idx : -ENOMEM;
}

static struct dma_async_tx_descriptor *
iop_adma_prep_dma_interrupt(struct dma_chan *chan, unsigned long flags)
{
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(chan);
	struct iop_adma_desc_slot *sw_desc, *grp_start;
	int slot_cnt, slots_per_op;

	dev_dbg(iop_chan->device->common.dev, "%s\n", __func__);

	spin_lock_bh(&iop_chan->lock);
	slot_cnt = iop_chan_interrupt_slot_count(&slots_per_op, iop_chan);
	sw_desc = iop_adma_alloc_slots(iop_chan, slot_cnt, slots_per_op);
	if (sw_desc) {
		grp_start = sw_desc->group_head;
		