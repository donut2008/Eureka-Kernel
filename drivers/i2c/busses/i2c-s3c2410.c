kmalloc(sizeof(*ip), GFP_KERNEL);
	if (!ip)
		goto bail;

	size = PAGE_ALIGN(size);

	spin_lock_irq(&dev->mmap_offset_lock);
	if (dev->mmap_offset == 0)
		dev->mmap_offset = PAGE_SIZE;
	ip->offset = dev->mmap_offset;
	dev->mmap_offset += size;
	spin_unlock_irq(&dev->mmap_offset_lock);

	INIT_LIST_HEAD(&ip->pending_mmaps);
	ip->size = size;
	ip->context = context;
	ip->obj = obj;
	kref_init(&ip->ref);

bail:
	return ip;
}

void qib_update_mmap_info(struct qib_ibdev *dev, struct qib_mmap_info *ip,
			  u32 size, void *obj)
{
	size = PAGE_ALIGN(size);

	spin_lock_irq(&dev->mmap_offset_lock);
	if (dev->mmap_offset == 0)
		dev->mmap_offset = PAGE_SIZE;
	ip->offset = dev->mmap_offset;
	dev->mmap_offset += size;
	spin_unlock_irq(&dev->mmap_offset_lock);

	ip->size = size;
	ip->obj = obj;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
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

#include <rdma/ib_umem.h>
#include <rdma/ib_smi.h>

#include "qib.h"

/* Fast memory region */
struct qib_fmr {
	struct ib_fmr ibfmr;
	struct qib_mregion mr;        /* must be last */
};

static inline struct qib_fmr *to_ifmr(struct ib_fmr *ibfmr)
{
	return container_of(ibfmr, struct qib_fmr, ibfmr);
}

static int init_qib_mregion(struct qib_mregion *mr, struct ib_pd *pd,
	int count)
{
	int m, i = 0;
	int rval = 0;

	m = (count + QIB_SEGSZ - 1) / QIB_SEGSZ;
	for (; i < m; i++) {
		mr->map[i] = kzalloc(sizeof(*mr->map[0]), GFP_KERNEL);
		if (!mr->map[i])
			goto bail;
	}
	mr->mapsz = m;
	init_completion(&mr->comp);
	/* count returning the ptr to user */
	atomic_set(&mr->refcount, 1);
	mr->pd = pd;
	mr->max_segs = count;
out:
	return rval;
bail:
	while (i)
		kfree(mr->map[--i]);
	rval = -ENOMEM;
	goto out;
}

static void deinit_qib_mregion(struct qib_mregion *mr)
{
	int i = mr->mapsz;

	mr->mapsz = 0;
	while (i)
		kfree(mr->map[--i]);
}


/**
 * qib_get_dma_mr - get a DMA memory region
 * @pd: protection domain for this memory region
 * @acc: access flags
 *
 * Returns the memory region on success, otherwise returns an errno.
 * Note that all DMA addresses should be created via the
 * struct ib_dma_mapping_ops functions (see qib_dma.c).
 */
struct ib_mr *qib_get_dma_mr(struct ib_pd *pd, int acc)
{
	struct qib_mr *mr = NULL;
	struct ib_mr *ret;
	int rval;

	if (to_ipd(pd)->user) {
		ret = ERR_PTR(-EPERM);
		goto bail;
	}

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr) {
		ret = ERR_PTR(-ENOMEM);
		goto bail;
	}

	rval = init_qib_mregion(&mr->mr, pd, 0);
	if (rval) {
		ret = ERR_PTR(rval);
		goto bail;
	}


	rval = qib_alloc_lkey(&mr->mr, 1);
	if (rval) {
		ret = ERR_PTR(rval);
		goto bail_mregion;
	}

	mr->mr.access_flags = acc;
	ret = &mr->ibmr;
done:
	return ret;

bail_mregion:
	deinit_qib_mregion(&mr->mr);
bail:
	kfree(mr);
	goto done;
}

static struct qib_mr *alloc_mr(int count, struct ib_pd *pd)
{
	struct qib_mr *mr;
	int rval = -ENOMEM;
	int m;

	/* Allocate struct plus pointers to first level page tables. */
	m = (count + QIB_SEGSZ - 1) / QIB_SEGSZ;
	mr = kzalloc(sizeof(*mr) + m * sizeof(mr->mr.map[0]), GFP_KERNEL);
	if (!mr)
		goto bail;

	rval = init_qib_mregion(&mr->mr, pd, count);
	if (rval)
		goto bail;
	/*
	 * ib_reg_phys_mr() will initialize mr->ibmr except for
	 * lkey and rkey.
	 */
	rval = qib_alloc_lkey(&mr->mr, 0);
	if (rval)
		goto bail_mregion;
	mr->ibmr.lkey = mr->mr.lkey;
	mr->ibmr.rkey = mr->mr.lkey;
done:
	return mr;

bail_mregion:
	deinit_qib_mregion(&mr->mr);
bail:
	kfree(mr);
	mr = ERR_PTR(rval);
	goto done;
}

/**
 * qib_reg_phys_mr - register a physical memory region
 * @pd: protection domain for this memory region
 * @buffer_list: pointer to the list of physical buffers to register
 * @num_phys_buf: the number of physical buffers to register
 * @iova_start: the starting address passed over IB which maps to this MR
 *
 * Returns the memory region on success, otherwise returns an errno.
 */
struct ib_mr *qib_reg_phys_mr(struct ib_pd *pd,
			      struct ib_phys_buf *buffer_list,
			      int num_phys_buf, int acc, u64 *iova_start)
{
	struct qib_mr *mr;
	int n, m, i;
	struct ib_mr *ret;

	mr = alloc_mr(num_phys_buf, pd);
	if (IS_ERR(mr)) {
		ret = (struct ib_mr *)mr;
		goto bail;
	}

	mr->mr.user_base = *iova_start;
	mr->mr.iova = *iova_start;
	mr->mr.access_flags = acc;

	m = 0;
	n = 0;
	for (i = 0; i < num_phys_buf; i++) {
		mr->mr.map[m]->segs[n].vaddr = (void *) buffer_list[i].addr;
		mr->mr.map[m]->segs[n].length = buffer_list[i].size;
		mr->mr.length += buffer_list[i].size;
		n++;
		if (n == QIB_SEGSZ) {
			m++;
			n = 0;
		}
	}

	ret = &mr->ibmr;

bail:
	return ret;
}

/**
 * qib_reg_user_mr - register a userspace memory region
 * @pd: protection domain for this memory region
 * @start: starting userspace address
 * @length: length of region to register
 * @mr_access_flags: access flags for this memory region
 * @udata: unused by the QLogic_IB driver
 *
 * Returns the memory region on success, otherwise returns an errno.
 */
struct ib_mr *qib_reg_user_mr(struct ib_pd *pd, u64 start, u64 length,
			      u64 virt_addr, int mr_access_flags,
			      struct ib_udata *udata)
{
	struct qib_mr *mr;
	struct ib_umem *umem;
	struct scatterlist *sg;
	int n, m, entry;
	struct ib_mr *ret;

	if (length == 0) {
		ret = ERR_PTR(-EINVAL);
		goto bail;
	}

	umem = ib_umem_get(pd->uobject->context, start, length,
			   mr_access_flags, 0);
	if (IS_ERR(umem))
		return (void *) umem;

	n = umem->nmap;

	mr = alloc_mr(n, pd);
	if (IS_ERR(mr)) {
		ret = (struct ib_mr *)mr;
		ib_umem_release(umem);
		goto bail;
	}

	mr->mr.user_base = start;
	mr->mr.iova = virt_addr;
	mr->mr.length = length;
	mr->mr.offset = ib_umem_offset(umem);
	mr->mr.access_flags = mr_access_flags;
	mr->umem = umem;

	if (is_power_of_2(umem->page_size))
		mr->mr.page_shift = ilog2(umem->page_size);
	m = 0;
	n = 0;
	for_each_sg(umem->sg_head.sgl, sg, umem->nmap, entry) {
			void *vaddr;

			vaddr = page_address(sg_page(sg));
			if (!vaddr) {
				ret = ERR_PTR(-EINVAL);
				goto bail;
			}
			mr->mr.map[m]->segs[n].vaddr = vaddr;
			mr->mr.map[m]->segs[n].length = umem->page_size;
			n++;
			if (n == QIB_SEGSZ) {
				m++;
				n = 0;
			}
	}
	ret = &mr->ibmr;

bail:
	return ret;
}

/**
 * qib_dereg_mr - unregister and free a memory region
 * @ibmr: the memory region to free
 *
 * Returns 0 on success.
 *
 * Note that this is called to free MRs created by qib_get_dma_mr()
 * or qib_reg_user_mr().
 */
int qib_dereg_mr(struct ib_mr *ibmr)
{
	struct qib_mr *mr = to_imr(ibmr);
	int ret = 0;
	unsigned long timeout;

	kfree(mr->pages);
	qib_free_lkey(&mr->mr);

	qib_put_mr(&mr->mr); /* will set completion if last */
	timeout = wait_for_completion_timeout(&mr->mr.comp,
		5 * HZ);
	if (!timeout) {
		qib_get_mr(&mr->mr);
		ret = -EBUSY;
		goto out;
	}
	deinit_qib_mregion(&mr->mr);
	if (mr->umem)
		ib_umem_release(mr->umem);
	kfree(mr);
out:
	return ret;
}

/*
 * Allocate a memory region usable with the
 * IB_WR_REG_MR send work request.
 *
 * Return the memory region on success, otherwise return an errno.
 */
struct ib_mr *qib_alloc_mr(struct ib_pd *pd,
			   enum ib_mr_type mr_type,
			   u32 max_num_sg)
{
	struct qib_mr *mr;

	if (mr_type != IB_MR_TYPE_MEM_REG)
		return ERR_PTR(-EINVAL);

	mr = alloc_mr(max_num_sg, pd);
	if (IS_ERR(mr))
		return (struct ib_mr *)mr;

	mr->pages = kcalloc(max_num_sg, sizeof(u64), GFP_KERNEL);
	if (!mr->pages)
		goto err;

	return &mr->ibmr;

err:
	qib_dereg_mr(&mr->ibmr);
	return ERR_PTR(-ENOMEM);
}

static int qib_set_page(struct ib_mr *ibmr, u64 addr)
{
	struct qib_mr *mr = to_imr(ibmr);

	if (unlikely(mr->npages == mr->mr.max_segs))
		return -ENOMEM;

	mr->pages[mr->npages++] = addr;

	return 0;
}

int qib_map_mr_sg(struct ib_mr *ibmr,
		  struct scatterlist *sg,
		  int sg_nents)
{
	struct qib_mr *mr = to_imr(ibmr);

	mr->npages = 0;

	return ib_sg_to_pages(ibmr, sg, sg_nents, qib_set_page);
}

/**
 * qib_alloc_fmr - allocate a fast memory region
 * @pd: the protection domain for this memory region
 * @mr_access_flags: access flags for this memory region
 * @fmr_attr: fast memory region attributes
 *
 * Returns the memory region on success, otherwise returns an errno.
 */
struct ib_fmr *qib_alloc_fmr(struct ib_pd *pd, int mr_access_flags,
			     struct ib_fmr_attr *fmr_attr)
{
	struct qib_fmr *fmr;
	int m;
	struct ib_fmr *ret;
	int rval = -ENOMEM;

	/* Allocate struct plus pointers to first level page tables. */
	m = (fmr_attr->max_pages + QIB_SEGSZ - 1) / QIB_SEGSZ;
	fmr = kzalloc(sizeof(*fmr) + m * sizeof(fmr->mr.map[0]), GFP_KERNEL);
	if (!fmr)
		goto bail;

	rval = init_qib_mregion(&fmr->mr, pd, fmr_attr->max_pages);
	if (rval)
		goto bail;

	/*
	 * ib_alloc_fmr() will initialize fmr->ibfmr except for lkey &
	 * rkey.
	 */
	rval = qib_alloc_lkey(&fmr->mr, 0);
	if (rval)
		goto bail_mregion;
	fmr->ibfmr.rkey = fmr->mr.lkey;
	fmr->ibfmr.lkey = fmr->mr.lkey;
	/*
	 * Resources are allocated but no valid mapping (RKEY can't be
	 * used).
	 */
	fmr->mr.access_flags = mr_access_flags;
	fmr->mr.max_segs = fmr_attr->max_pages;
	fmr->mr.page_shift = fmr_attr->page_shift;

	ret = &fmr->ibfmr;
done:
	return ret;

bail_mregion:
	deinit_qib_mregion(&fmr->mr);
bail:
	kfree(fmr);
	ret = ERR_PTR(rval);
	goto done;
}

/**
 * qib_map_phys_fmr - set up a fast memory region
 * @ibmfr: the fast memory region to set up
 * @page_list: the list of pages to associate with the fast memory region
 * @list_len: the number of pages to associate with the fast memory region
 * @iova: the virtual address of the start of the fast memory region
 *
 * This may be called from interrupt context.
 */

int qib_map_phys_fmr(struct ib_fmr *ibfmr, u64 *page_list,
		     int list_len, u64 iova)
{
	struct qib_fmr *fmr = to_ifmr(ibfmr);
	struct qib_lkey_table *rkt;
	unsigned long flags;
	int m, n, i;
	u32 ps;
	int ret;

	i = atomic_read(&fmr->mr.refcount);
	if (i > 2)
		return -EBUSY;

	if (list_len > fmr->mr.max_segs) {
		ret = -EINVAL;
		goto bail;
	}
	rkt = &to_idev(ibfmr->device)->lk_table;
	spin_lock_irqsave(&rkt->lock, flags);
	fmr->mr.user_base = iova;
	fmr->mr.iova = iova;
	ps = 1 << fmr->mr.page_shift;
	fmr->mr.length = list_len * ps;
	m = 0;
	n = 0;
	for (i = 0; i < list_len; i++) {
		fmr->mr.map[m]->segs[n].vaddr = (void *) page_list[i];
		fmr->mr.map[m]->segs[n].length = ps;
		if (++n == QIB_SEGSZ) {
			m++;
			n = 0;
		}
	}
	spin_unlock_irqrestore(&rkt->lock, flags);
	ret = 0;

bail:
	return ret;
}

/**
 * qib_unmap_fmr - unmap fast memory regions
 * @fmr_list: the list of fast memory regions to unmap
 *
 * Returns 0 on success.
 */
int qib_unmap_fmr(struct list_head *fmr_list)
{
	struct qib_fmr *fmr;
	struct qib_lkey_table *rkt;
	unsigned long flags;

	list_for_each_entry(fmr, fmr_list, ibfmr.list) {
		rkt = &to_idev(fmr->ibfmr.device)->lk_table;
		spin_lock_irqsave(&rkt->lock, flags);
		fmr->mr.user_base = 0;
		fmr->mr.iova = 0;
		fmr->mr.length = 0;
		spin_unlock_irqrestore(&rkt->lock, flags);
	}
	return 0;
}

/**
 * qib_dealloc_fmr - deallocate a fast memory region
 * @ibfmr: the fast memory region to deallocate
 *
 * Returns 0 on success.
 */
int qib_dealloc_fmr(struct ib_fmr *ibfmr)
{
	struct qib_fmr *fmr = to_ifmr(ibfmr);
	int ret = 0;
	unsigned long timeout;

	qib_free_lkey(&fmr->mr);
	qib_put_mr(&fmr->mr); /* will set completion if last */
	timeout = wait_for_completion_timeout(&fmr->mr.comp,
		5 * HZ);
	if (!timeout) {
		qib_get_mr(&fmr->mr);
		ret = -EBUSY;
		goto out;
	}
	deinit_qib_mregion(&fmr->mr);
	kfree(fmr);
out:
	return ret;
}

void mr_rcu_callback(struct rcu_head *list)
{
	struct qib_mregion *mr = container_of(list, struct qib_mregion, list);

	complete(&mr->comp);
}
                                                                                                       /*
 * Copyright (c) 2008, 2009 QLogic Corporation. All rights reserved.
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
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/aer.h>
#include <linux/module.h>

#include "qib.h"

/*
 * This file contains PCIe utility routines that are common to the
 * various QLogic InfiniPath adapters
 */

/*
 * Code to adjust PCIe capabilities.
 * To minimize the change footprint, we call it
 * from qib_pcie_params, which every chip-specific
 * file calls, even though this violates some
 * expectations of harmlessness.
 */
static void qib_tune_pcie_caps(struct qib_devdata *);
static void qib_tune_pcie_coalesce(struct qib_devdata *);

/*
 * Do all the common PCIe setup and initialization.
 * devdata is not yet allocated, and is not allocated until after this
 * routine returns success.  Therefore qib_dev_err() can't be used for error
 * printing.
 */
int qib_pcie_init(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret;

	ret = pci_enable_device(pdev);
	if (ret) {
		/*
		 * This can happen (in theory) iff:
		 * We did a chip reset, and then failed to reprogram the
		 * BAR, or the chip reset due to an internal error.  We then
		 * unloaded the driver and reloaded it.
		 *
		 * Both reset cases set the BAR back to initial state.  For
		 * the latter case, the AER sticky error bit at offset 0x718
		 * should be set, but the Linux kernel doesn't yet know
		 * about that, it appears.  If the original BAR was retained
		 * in the kernel data structures, this may be OK.
		 */
		qib_early_err(&pdev->dev, "pci enable failed: error %d\n",
			      -ret);
		goto done;
	}

	ret = pci_request_regions(pdev, QIB_DRV_NAME);
	if (ret) {
		qib_devinfo(pdev, "pci_request_regions fails: err %d\n", -ret);
		goto bail;
	}

	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (ret) {
		/*
		 * If the 64 bit setup fails, try 32 bit.  Some systems
		 * do not setup 64 bit maps on systems with 2GB or less
		 * memory installed.
		 */
		ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (ret) {
			qib_devinfo(pdev, "Unable to set DMA mask: %d\n", ret);
			goto bail;
		}
		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	} else
		ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	if (ret) {
		qib_early_err(&pdev->dev,
			      "Unable to set DMA consistent mask: %d\n", ret);
		goto bail;
	}

	pci_set_master(pdev);
	ret = pci_enable_pcie_error_reporting(pdev);
	if (ret) {
		qib_early_err(&pdev->dev,
			      "Unable to enable pcie error reporting: %d\n",
			      ret);
		ret = 0;
	}
	goto done;

bail:
	pci_disable_device(pdev);
	pci_release_regions(pdev);
done:
	return ret;
}

/*
 * Do remaining PCIe setup, once dd is allocated, and save away
 * fields required to re-initialize after a chip reset, or for
 * various other purposes
 */
int qib_pcie_ddinit(struct qib_devdata *dd, struct pci_dev *pdev,
		    const struct pci_device_id *ent)
{
	unsigned long len;
	resource_size_t addr;

	dd->pcidev = pdev;
	pci_set_drvdata(pdev, dd);

	addr = pci_resource_start(pdev, 0);
	len = pci_resource_len(pdev, 0);

#if defined(__powerpc__)
	/* There isn't a generic way to specify writethrough mappings */
	dd->kregbase = __ioremap(addr, len, _PAGE_NO_CACHE | _PAGE_WRITETHRU);
#else
	dd->kregbase = ioremap_nocache(addr, len);
#endif

	if (!dd->kregbase)
		return -ENOMEM;

	dd->kregend = (u64 __iomem *)((void __iomem *) dd->kregbase + len);
	dd->physaddr = addr;        /* used for io_remap, etc. */

	/*
	 * Save BARs to rewrite after device reset.  Save all 64 bits of
	 * BAR, just in case.
	 */
	dd->pcibar0 = addr;
	dd->pcibar1 = addr >> 32;
	dd->deviceid = ent->device; /* save for later use */
	dd->vendorid = ent->vendor;

	return 0;
}

/*
 * Do PCIe cleanup, after chip-specific cleanup, etc.  Just prior
 * to releasing the dd memory.
 * void because none of the core pcie cleanup returns are void
 */
void qib_pcie_ddcleanup(struct qib_devdata *dd)
{
	u64 __iomem *base = (void __iomem *) dd->kregbase;

	dd->kregbase = NULL;
	iounmap(base);
	if (dd->piobase)
		iounmap(dd->piobase);
	if (dd->userbase)
		iounmap(dd->userbase);
	if (dd->piovl15base)
		iounmap(dd->piovl15base);

	pci_disable_device(dd->pcidev);
	pci_release_regions(dd->pcidev);

	pci_set_drvdata(dd->pcidev, NULL);
}

static void qib_msix_setup(struct qib_devdata *dd, int pos, u32 *msixcnt,
			   struct qib_msix_entry *qib_msix_entry)
{
	int ret;
	int nvec = *msixcnt;
	struct msix_entry *msix_entry;
	int i;

	ret = pci_msix_vec_count(dd->pcidev);
	if (ret < 0)
		goto do_intx;

	nvec = min(nvec, ret);

	/* We can't pass qib_msix_entry array to qib_msix_setup
	 * so use a dummy msix_entry array and copy the allocated
	 * irq back to the qib_msix_entry array. */
	msix_entry = kcalloc(nvec, sizeof(*msix_entry), GFP_KERNEL);
	if (!msix_entry)
		goto do_intx;

	for (i = 0; i < nvec; i++)
		msix_entry[i] = qib_msix_entry[i].msix;

	ret = pci_enable_msix_range(dd->pcidev, msix_entry, 1, nvec);
	if (ret < 0)
		goto free_msix_entry;
	else
		nvec = ret;

	for (i = 0; i < nvec; i++)
		qib_msix_entry[i].msix = msix_entry[i];

	kfree(msix_entry);
	*msixcnt = nvec;
	return;

free_msix_entry:
	kfree(msix_entry);

do_intx:
	qib_dev_err(
		dd,
		"pci_enable_msix_range %d vectors failed: %d, falling back to INTx\n",
		nvec, ret);
	*msixcnt = 0;
	qib_enable_intx(dd->pcidev);
}

/**
 * We save the msi lo and hi values, so we can restore them after
 * chip reset (the kernel PCI infrastructure doesn't yet handle that
 * correctly.
 */
static int qib_msi_setup(struct qib_devdata *dd, int pos)
{
	struct pci_dev *pdev = dd->pcidev;
	u16 control;
	int ret;

	ret = pci_enable_msi(pdev);
	if (ret)
		qib_dev_err(dd,
			"pci_enable_msi failed: %d, interrupts may not work\n",
			ret);
	/* continue even if it fails, we may still be OK... */

	pci_read_config_dword(pdev, pos + PCI_MSI_ADDRESS_LO,
			      &dd->msi_lo);
	pci_read_config_dword(pdev, pos + PCI_MSI_ADDRESS_HI,
			      &dd->msi_hi);
	pci_read_config_word(pdev, pos + PCI_MSI_FLAGS, &control);
	/* now save the data (vector) info */
	pci_read_config_word(pdev, pos + ((control & PCI_MSI_FLAGS_64BIT)
				    ? 12 : 8),
			     &dd->msi_data);
	return ret;
}

int qib_pcie_params(struct qib_devdata *dd, u32 minw, u32 *nent,
		    struct qib_msix_entry *entry)
{
	u16 linkstat, speed;
	int pos = 0, ret = 1;

	if (!pci_is_pcie(dd->pcidev)) {
		qib_dev_err(dd, "Can't find PCI Express capability!\n");
		/* set up something... */
		dd->lbus_width = 1;
		dd->lbus_speed = 2500; /* Gen1, 2.5GHz */
		goto bail;
	}

	pos = dd->pcidev->msix_cap;
	if (nent && *nent && pos) {
		qib_msix_setup(dd, pos, nent, entry);
		ret = 0; /* did it, either MSIx or INTx */
	} else {
		pos = dd->pcidev->msi_cap;
		if (pos)
			ret = qib_msi_setup(dd, pos);
		else
			qib_dev_err(dd, "No PCI MSI or MSIx capability!\n");
	}
	if (!pos)
		qib_enable_intx(dd->pcidev);

	pcie_capability_read_word(dd->pcidev, PCI_EXP_LNKSTA, &linkstat);
	/*
	 * speed is bits 0-3, linkwidth is bits 4-8
	 * no defines for them in headers
	 */
	speed = linkstat & 0xf;
	linkstat >>= 4;
	linkstat &= 0x1f;
	dd->lbus_width = linkstat;

	switch (speed) {
	case 1:
		dd->lbus_speed = 2500; /* Gen1, 2.5GHz */
		break;
	case 2:
		dd->lbus_speed = 5000; /* Gen1, 5GHz */
		break;
	default: /* not defined, assume gen1 */
		dd->lbus_speed = 2500;
		break;
	}

	/*
	 * Check against expected pcie width and complain if "wrong"
	 * on first initialization, not afterwards (i.e., reset).
	 */
	if (minw && linkstat < minw)
		qib_dev_err(dd,
			    "PCIe width %u (x%u HCA), performance reduced\n",
			    linkstat, minw);

	qib_tune_pcie_caps(dd);

	qib_tune_pcie_coalesce(dd);

bail:
	/* fill in string, even on errors */
	snprintf(dd->lbus_info, sizeof(dd->lbus_info),
		 "PCIe,%uMHz,x%u\n", dd->lbus_speed, dd->lbus_width);
	return ret;
}

/*
 * Setup pcie interrupt stuff again after a reset.  I'd like to just call
 * pci_enable_msi() again for msi, but when I do that,
 * the MSI enable bit doesn't get set in the command word, and
 * we switch to to a different interrupt vector, which is confusing,
 * so I instead just do it all inline.  Perhaps somehow can tie this
 * into the PCIe hotplug support at some point
 */
int qib_reinit_intr(struct qib_devdata *dd)
{
	int pos;
	u16 control;
	int ret = 0;

	/* If we aren't using MSI, don't restore it */
	if (!dd->msi_lo)
		goto bail;

	pos = dd->pcidev->msi_cap;
	if (!pos) {
		qib_dev_err(dd,
			"Can't find MSI capability, can't restore MSI settings\n");
		ret = 0;
		/* nothing special for MSIx, just MSI */
		goto bail;
	}
	pci_write_config_dword(dd->pcidev, pos + PCI_MSI_ADDRESS_LO,
			       dd->msi_lo);
	pci_write_config_dword(dd->pcidev, pos + PCI_MSI_ADDRESS_HI,
			       dd->msi_hi);
	pci_read_config_word(dd->pcidev, pos + PCI_MSI_FLAGS, &control);
	if (!(control & PCI_MSI_FLAGS_ENABLE)) {
		control |= PCI_MSI_FLAGS_ENABLE;
		pci_write_config_word(dd->pcidev, pos + PCI_MSI_FLAGS,
				      control);
	}
	/* now rewrite the data (vector) info */
	pci_write_config_word(dd->pcidev, pos +
			      ((control & PCI_MSI_FLAGS_64BIT) ? 12 : 8),
			      dd->msi_data);
	ret = 1;
bail:
	if (!ret && (dd->flags & QIB_HAS_INTX)) {
		qib_enable_intx(dd->pcidev);
		ret = 1;
	}

	/* and now set the pci master bit again */
	pci_set_master(dd->pcidev);

	return ret;
}

/*
 * Disable msi interrupt if enabled, and clear msi_lo.
 * This is used primarily for the fallback to INTx, but
 * is also used in reinit after reset, and during cleanup.
 */
void qib_nomsi(struct qib_devdata *dd)
{
	dd->msi_lo = 0;
	pci_disable_msi(dd->pcidev);
}

/*
 * Same as qib_nosmi, but for MSIx.
 */
void qib_nomsix(struct qib_devdata *dd)
{
	pci_disable_msix(dd->pcidev);
}

/*
 * Similar to pci_intx(pdev, 1), except that we make sure
 * msi(x) is off.
 */
void qib_enable_intx(struct pci_dev *pdev)
{
	u16 cw, new;
	int pos;

	/* first, turn on INTx */
	pci_read_config_word(pdev, PCI_COMMAND, &cw);
	new = cw & ~PCI_COMMAND_INTX_DISABLE;
	if (new != cw)
		pci_write_config_word(pdev, PCI_COMMAND, new);

	pos = pdev->msi_cap;
	if (pos) {
		/* then turn off MSI */
		pci_read_config_word(pdev, pos + PCI_MSI_FLAGS, &cw);
		new = cw & ~PCI_MSI_FLAGS_ENABLE;
		if (new != cw)
			pci_write_config_word(pdev, pos + PCI_MSI_FLAGS, new);
	}
	pos = pdev->msix_cap;
	if (pos) {
		/* then turn off MSIx */
		pci_read_config_word(pdev, pos + PCI_MSIX_FLAGS, &cw);
		new = cw & ~PCI_MSIX_FLAGS_ENABLE;
		if (new != cw)
			pci_write_config_word(pdev, pos + PCI_MSIX_FLAGS, new);
	}
}

/*
 * These two routines are helper routines for the device reset code
 * to move all the pcie code out of the chip-specific driver code.
 */
void qib_pcie_getcmd(struct qib_devdata *dd, u16 *cmd, u8 *iline, u8 *cline)
{
	pci_read_config_word(dd->pcidev, PCI_COMMAND, cmd);
	pci_read_config_byte(dd->pcidev, PCI_INTERRUPT_LINE, iline);
	pci_read_config_byte(dd->pcidev, PCI_CACHE_LINE_SIZE, cline);
}

void qib_pcie_reenable(struct qib_devdata *dd, u16 cmd, u8 iline, u8 cline)
{
	int r;

	r = pci_write_config_dword(dd->pcidev, PCI_BASE_ADDRESS_0,
				   dd->pcibar0);
	if (r)
		qib_dev_err(dd, "rewrite of BAR0 failed: %d\n", r);
	r = pci_write_config_dword(dd->pcidev, PCI_BASE_ADDRESS_1,
				   dd->pcibar1);
	if (r)
		qib_dev_err(dd, "rewrite of BAR1 failed: %d\n", r);
	/* now re-enable memory access, and restore cosmetic settings */
	pci_write_config_word(dd->pcidev, PCI_COMMAND, cmd);
	pci_write_config_byte(dd->pcidev, PCI_INTERRUPT_LINE, iline);
	pci_write_config_byte(dd->pcidev, PCI_CACHE_LINE_SIZE, cline);
	r = pci_enable_device(dd->pcidev);
	if (r)
		qib_dev_err(dd,
			"pci_enable_device failed after reset: %d\n", r);
}


static int qib_pcie_coalesce;
module_param_named(pcie_coalesce, qib_pcie_coalesce, int, S_IRUGO);
MODULE_PARM_DESC(pcie_coalesce, "tune PCIe colescing on some Intel chipsets");

/*
 * Enable PCIe completion and data coalescing, on Intel 5x00 and 7300
 * chipsets.   This is known to be unsafe for some revisions of some
 * of these chipsets, with some BIOS settings, and enabling it on those
 * systems may result in the system crashing, and/or data corruption.
 */
static void qib_tune_pcie_coalesce(struct qib_devdata *dd)
{
	int r;
	struct pci_dev *parent;
	u16 devid;
	u32 mask, bits, val;

	if (!qib_pcie_coalesce)
		return;

	/* Find out supported and configured values for parent (root) */
	parent = dd->pcidev->bus->self;
	if (parent->bus->parent) {
		qib_devinfo(dd->pcidev, "Parent not root\n");
		return;
	}
	if (!pci_is_pcie(parent))
		return;
	if (parent->vendor != 0x8086)
		return;

	/*
	 *  - bit 12: Max_rdcmp_Imt_EN: need to set to 1
	 *  - bit 11: COALESCE_FORCE: need to set to 0
	 *  - bit 10: COALESCE_EN: need to set to 1
	 *  (but limitations on some on some chipsets)
	 *
	 *  On the Intel 5000, 5100, and 7300 chipsets, there is
	 *  also: - bit 25:24: COALESCE_MODE, need to set to 0
	 */
	devid = parent->device;
	if (devid >= 0x25e2 && devid <= 0x25fa) {
		/* 5000 P/V/X/Z */
		if (parent->revision <= 0xb2)
			bits = 1U << 10;
		else
			bits = 7U << 10;
		mask = (3U << 24) | (7U << 10);
	} else if (devid >= 0x65e2 && devid <= 0x65fa) {
		/* 5100 */
		bits = 1U << 10;
		mask = (3U << 24) | (7U << 10);
	} else if (devid >= 0x4021 && devid <= 0x402e) {
		/* 5400 */
		bits = 7U << 10;
		mask = 7U << 10;
	} else if (devid >= 0x3604 && devid <= 0x360a) {
		/* 7300 */
		bits = 7U << 10;
		mask = (3U << 24) | (7U << 10);
	} else {
		/* not one of the chipsets that we know about */
		return;
	}
	pci_read_config_dword(parent, 0x48, &val);
	val &= ~mask;
	val |= bits;
	r = pci_write_config_dword(parent, 0x48, val);
}

/*
 * BIOS may not set PCIe bus-utilization parameters for best performance.
 * Check and optionally adjust them to maximize our throughput.
 */
static int qib_pcie_caps;
module_param_named(pcie_caps, qib_pcie_caps, int, S_IRUGO);
MODULE_PARM_DESC(pcie_caps, "Max PCIe tuning: Payload (0..3), ReadReq (4..7)");

static void qib_tune_pcie_caps(struct qib_devdata *dd)
{
	struct pci_dev *parent;
	u16 rc_mpss, rc_mps, ep_mpss, ep_mps;
	u16 rc_mrrs, ep_mrrs, max_mrrs;

	/* Find out supported and configured values for parent (root) */
	parent = dd->pcidev->bus->self;
	if (!pci_is_root_bus(parent->bus)) {
		qib_devinfo(dd->pcidev, "Parent not root\n");
		return;
	}

	if (!pci_is_pcie(parent) || !pci_is_pcie(dd->pcidev))
		return;

	rc_mpss = parent->pcie_mpss;
	rc_mps = ffs(pcie_get_mps(parent)) - 8;
	/* Find out supported and configured values for endpoint (us) */
	ep_mpss = dd->pcidev->pcie_mpss;
	ep_mps = ffs(pcie_get_mps(dd->pcidev)) - 8;

	/* Find max payload supported by root, endpoint */
	if (rc_mpss > ep_mpss)
		rc_mpss = ep_mpss;

	/* If Supported greater than limit in module param, limit it */
	if (rc_mpss > (qib_pcie_caps & 7))
		rc_mpss = qib_pcie_caps & 7;
	/* If less than (allowed, supported), bump root payload */
	if (rc_mpss > rc_mps) {
		rc_mps = rc_mpss;
		pcie_set_mps(parent, 128 << rc_mps);
	}
	/* If less than (allowed, supported), bump endpoint payload */
	if (rc_mpss > ep_mps) {
		ep_mps = rc_mpss;
		pcie_set_mps(dd->pcidev, 128 << ep_mps);
	}

	/*
	 * Now the Read Request size.
	 * No field for max supported, but PCIe spec limits it to 4096,
	 * which is code '5' (log2(4096) - 7)
	 */
	max_mrrs = 5;
	if (max_mrrs > ((qib_pcie_caps >> 4) & 7))
		max_mrrs = (qib_pcie_caps >> 4) & 7;

	max_mrrs = 128 << max_mrrs;
	rc_mrrs = pcie_get_readrq(parent);
	ep_mrrs = pcie_get_readrq(dd->pcidev);

	if (max_mrrs > rc_mrrs) {
		rc_mrrs = max_mrrs;
		pcie_set_readrq(parent, rc_mrrs);
	}
	if (max_mrrs > ep_mrrs) {
		ep_mrrs = max_mrrs;
		pcie_set_readrq(dd->pcidev, ep_mrrs);
	}
}
/* End of PCIe capability tuning */

/*
 * From here through qib_pci_err_handler definition is invoked via
 * PCI error infrastructure, registered via pci
 */
static pci_ers_result_t
qib_pci_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
{
	struct qib_devdata *dd = pci_get_drvdata(pdev);
	pci_ers_result_t ret = PCI_ERS_RESULT_RECOVERED;

	switch (state) {
	case pci_channel_io_normal:
		qib_devinfo(pdev, "State Normal, ignoring\n");
		break;

	case pci_channel_io_frozen:
		qib_devinfo(pdev, "State Frozen, requesting reset\n");
		pci_disable_device(pdev);
		ret = PCI_ERS_RESULT_NEED_RESET;
		break;

	case pci_channel_io_perm_failure:
		qib_devinfo(pdev, "State Permanent Failure, disabling\n");
		if (dd) {
			/* no more register accesses! */
			dd->flags &= ~QIB_PRESENT;
			qib_disable_after_error(dd);
		}
		 /* else early, or other problem */
		ret =  PCI_ERS_RESULT_DISCONNECT;
		break;

	default: /* shouldn't happen */
		qib_devinfo(pdev, "QIB PCI errors detected (state %d)\n",
			state);
		break;
	}
	return ret;
}

static pci_ers_result_t
qib_pci_mmio_enabled(struct pci_dev *pdev)
{
	u64 words = 0U;
	struct qib_devdata *dd = pci_get_drvdata(pdev);
	pci_ers_result_t ret = PCI_ERS_RESULT_RECOVERED;

	if (dd && dd->pport) {
		words = dd->f_portcntr(dd->pport, QIBPORTCNTR_WORDRCV);
		if (words == ~0ULL)
			ret = PCI_ERS_RESULT_NEED_RESET;
	}
	qib_devinfo(pdev,
		"QIB mmio_enabled function called, read wordscntr %Lx, returning %d\n",
		words, ret);
	return  ret;
}

static pci_ers_result_t
qib_pci_slot_reset(struct pci_dev *pdev)
{
	qib_devinfo(pdev, "QIB slot_reset function called, ignored\n");
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static pci_ers_result_t
qib_pci_link_reset(struct pci_dev *pdev)
{
	qib_devinfo(pdev, "QIB link_reset function called, ignored\n");
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static void
qib_pci_resume(struct pci_dev *pdev)
{
	struct qib_devdata *dd = pci_get_drvdata(pdev);

	qib_devinfo(pdev, "QIB resume function called\n");
	pci_cleanup_aer_uncorrect_error_status(pdev);
	/*
	 * Running jobs will fail, since it's asynchronous
	 * unlike sysfs-requested reset.   Better than
	 * doing nothing.
	 */
	qib_init(dd, 1); /* same as re-init after reset */
}

const struct pci_error_handlers qib_pci_err_handler = {
	.error_detected = qib_pci_error_detected,
	.mmio_enabled = qib_pci_mmio_enabled,
	.link_reset = qib_pci_link_reset,
	.slot_reset = qib_pci_slot_reset,
	.resume = qib_pci_resume,
};
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (c) 2009 QLogic Corporation. All rights reserved.
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
 * qib_pio_copy - copy data to MMIO space, in multiples of 32-bits
 * @to: destination, in MMIO space (must be 64-bit aligned)
 * @from: source (must be 64-bit aligned)
 * @count: number of 32-bit quantities to copy
 *
 * Copy data from kernel space to MMIO space, in multiples of 32 bits at a
 * time.  Order of access is not guaranteed, nor is a memory barrier
 * performed afterwards.
 */
void qib_pio_copy(void __iomem *to, const void *from, size_t count)
{
#ifdef CONFIG_64BIT
	u64 __iomem *dst = to;
	const u64 *src = from;
	const u64 *end = src + (count >> 1);

	while (src < end)
		__raw_writeq(*src++, dst++);
	if (count & 1)
		__raw_writel(*(const u32 *)src, dst);
#else
	u32 __iomem *dst = to;
	const u32 *src = from;
	const u32 *end = src + count;

	while (src < end)
		__raw_writel(*src++, dst++);
#endif
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         