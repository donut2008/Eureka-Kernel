unt % 8) || (*off % 8);
		ret = -1;
		spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
		/*
		 * Check for observer on this address range.
		 * we only support a single 32 or 64-bit read
		 * via observer, currently.
		 */
		op = diag_get_observer(dd, *off);
		if (op) {
			u32 offset = *off;

			ret = op->hook(dd, op, offset, &data64, 0, use_32);
		}
		/*
		 * We need to release lock before any copy_to_user(),
		 * whether implicit in qib_read_umem* or explicit below.
		 */
		spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);
		if (!op) {
			if (use_32)
				/*
				 * Address or length is not 64-bit aligned;
				 * do 32-bit rd
				 */
				ret = qib_read_umem32(dd, data, (u32) *off,
						      count);
			else
				ret = qib_read_umem64(dd, data, (u32) *off,
						      count);
		} else if (ret == count) {
			/* Below finishes case where observer existed */
			ret = copy_to_user(data, &data64, use_32 ?
					   sizeof(u32) : sizeof(u64));
			if (ret)
				ret = -EFAULT;
		}
	}

	if (ret >= 0) {
		*off += count;
		ret = count;
		if (dc->state == OPENED)
			dc->state = INIT;
	}
bail:
	return ret;
}

static ssize_t qib_diag_write(struct file *fp, const char __user *data,
			      size_t count, loff_t *off)
{
	struct qib_diag_client *dc = fp->private_data;
	struct qib_devdata *dd = dc->dd;
	void __iomem *kreg_base;
	ssize_t ret;

	if (dc->pid != current->pid) {
		ret = -EPERM;
		goto bail;
	}

	kreg_base = dd->kregbase;

	if (count == 0)
		ret = 0;
	else if ((count % 4) || (*off % 4))
		/* address or length is not 32-bit aligned, hence invalid */
		ret = -EINVAL;
	else if (dc->state < READY &&
		((*off || count != 8) || dc->state != INIT))
		/* No writes except second-step of init seq */
		ret = -EINVAL;  /* before any other write allowed */
	else {
		unsigned long flags;
		const struct diag_observer *op = NULL;
		int use_32 =  (count % 8) || (*off % 8);

		/*
		 * Check for observer on this address range.
		 * We only support a single 32 or 64-bit write
		 * via observer, currently. This helps, because
		 * we would otherwise have to jump through hoops
		 * to make "diag transaction" meaningful when we
		 * cannot do a copy_from_user while holding the lock.
		 */
		if (count == 4 || count == 8) {
			u64 data64;
			u32 offset = *off;

			ret = copy_from_user(&data64, data, count);
			if (ret) {
				ret = -EFAULT;
				goto bail;
			}
			spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
			op = diag_get_observer(dd, *off);
			if (op)
				ret = op->hook(dd, op, offset, &data64, ~0Ull,
					       use_32);
			spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);
		}

		if (!op) {
			if (use_32)
				/*
				 * Address or length is not 64-bit aligned;
				 * do 32-bit write
				 */
				ret = qib_write_umem32(dd, (u32) *off, data,
						       count);
			else
				ret = qib_write_umem64(dd, (u32) *off, data,
						       count);
		}
	}

	if (ret >= 0) {
		*off += count;
		ret = count;
		if (dc->state == INIT)
			dc->state = READY; /* all read/write OK now */
	}
bail:
	return ret;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (c) 2006, 2009, 2010 QLogic, Corporation. All rights reserved.
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
#include <linux/types.h>
#include <linux/scatterlist.h>

#include "qib_verbs.h"

#define BAD_DMA_ADDRESS ((u64) 0)

/*
 * The following functions implement driver specific replacements
 * for the ib_dma_*() functions.
 *
 * These functions return kernel virtual addresses instead of
 * device bus addresses since the driver uses the CPU to copy
 * data instead of using hardware DMA.
 */

static int qib_mapping_error(struct ib_device *dev, u64 dma_addr)
{
	return dma_addr == BAD_DMA_ADDRESS;
}

static u64 qib_dma_map_single(struct ib_device *dev, void *cpu_addr,
			      size_t size, enum dma_data_direction direction)
{
	BUG_ON(!valid_dma_direction(direction));
	return (u64) cpu_addr;
}

static void qib_dma_unmap_single(struct ib_device *dev, u64 addr, size_t size,
				 enum dma_data_direction direction)
{
	BUG_ON(!valid_dma_direction(direction));
}

static u64 qib_dma_map_page(struct ib_device *dev, struct page *page,
			    unsigned long offset, size_t size,
			    enum dma_data_direction direction)
{
	u64 addr;

	BUG_ON(!valid_dma_direction(direction));

	if (offset + size > PAGE_SIZE) {
		addr = BAD_DMA_ADDRESS;
		goto done;
	}

	addr = (u64) page_address(page);
	if (addr)
		addr += offset;
	/* TODO: handle highmem pages */

done:
	return addr;
}

static void qib_dma_unmap_page(struct ib_device *dev, u64 addr, size_t size,
			       enum dma_data_direction direction)
{
	BUG_ON(!valid_dma_direction(direction));
}

static int qib_map_sg(struct ib_device *dev, struct scatterlist *sgl,
		      int nents, enum dma_data_direction direction)
{
	struct scatterlist *sg;
	u64 addr;
	int i;
	int ret = nents;

	BUG_ON(!valid_dma_direction(direction));

	for_each_sg(sgl, sg, ne