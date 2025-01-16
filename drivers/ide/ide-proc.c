_err(dev, "Self-test  2nd zero setup failed\n");
		err = -ENODEV;
		goto dma_unmap;
	}
	dma->device_issue_pending(dma_chan);

	tmo = wait_for_completion_timeout(&cmp, msecs_to_jiffies(3000));

	if (tmo == 0 ||
	    dma->device_tx_status(dma_chan, cookie, NULL) != DMA_COMPLETE) {
		dev_err(dev, "Self-test 2nd validate timed out\n");
		err = -ENODEV;
		goto dma_unmap;
	}

	if (xor_val_result != SUM_CHECK_P_RESULT) {
		dev_err(dev, "Self-test validate failed compare\n");
		err = -ENODEV;
		goto dma_unmap;
	}

	for (i = 0; i < IOAT_NUM_SRC_TEST + 1; i++)
		dma_unmap_page(dev, dma_srcs[i], PAGE_SIZE, DMA_TO_DEVICE);

	goto free_resources;
dma_unmap:
	if (op == IOAT_OP_XOR) {
		if (dest_dma != DMA_ERROR_CODE)
			dma_unmap_page(dev, dest_dma, PAGE_SIZE,
				       DMA_FROM_DEVICE);
		for (i = 0; i < IOAT_NUM_SRC_TEST; i++)
			if (dma_srcs[i] != DMA_ERROR_CODE)
				dma_unmap_page(dev, dma_srcs[i], PAGE_SIZE,
					       DMA_TO_DEVICE);
	} else if (op == IOAT_OP_XOR_VAL) {
		for (i = 0; i < IOAT_NUM_SRC_TEST + 1; i++)
			if (dma_srcs[i] != DMA_ERROR_CODE)
				dma_unmap_page(dev, dma_srcs[i], PAGE_SIZE,
					       DMA_TO_DEVICE);
	}
free_resources:
	dma->device_free_chan_resources(dma_chan);
out:
	src_idx = IOAT_NUM_SRC_TEST;
	while (src_idx--)
		__free_page(xor_srcs[src_idx]);
	__free_page(dest);
	return err;
}

static int ioat3_dma_self_test(struct ioatdma_device *ioat_dma)
{
	int rc;

	rc = ioat_dma_self_test(ioat_dma);
	if (rc)
		return rc;

	rc = ioat_xor_val_self_test(ioat_dma);

	return rc;
}

static void ioat_intr_quirk(struct ioatdma_device *ioat_dma)
{
	struct dma_device *dma;
	struct dma_chan *c;
	struct ioatdma_chan *ioat_chan;
	u32 errmask;

	dma = &ioat_dma->dma_dev;

	/*
	 * if we have descriptor write back error status, we mask the
	 * error interrupts
	 */
	if (ioat_dma->cap & IOAT_CAP_DWBES) {
		list_for_each_entry(c, &dma->channels, device_node) {
			ioat_chan = to_ioat_chan(c);
			errmask = readl(ioat_chan->reg_base +
					IOAT_CHANERR_MASK_OFFSET);
			errmask |= IOAT_CHANERR_XOR_P_OR_CRC_ERR |
				   IOAT_CHANERR_XOR_Q_ERR;
			writel(errmask, ioat_chan->reg_base +
					IOAT_CHANERR_MASK_OFFSET);
		}
	}
}

static int ioat3_dma_probe(struct ioatdma_device *ioat_dma, int dca)
{
	struct pci_dev *pdev = ioat_dma->pdev;
	int dca_en = system_has_dca_enabled(pdev);
	struct dma_device *dma;
	struct dma_chan *c;
	struct ioatdma_chan *ioat_chan;
	bool is_raid_device = false;
	int err;

	dma = &ioat_dma->dma_dev;
	dma->device_prep_dma_memcpy = ioat_dma_prep_memcpy_lock;
	dma->device_issue_pending = ioat_issue_pending;
	dma->device_alloc_chan_resources = ioat_alloc_chan_resources;
	dma->device_free_chan_resources = ioat_free_chan_resources;

	dma_cap_set(DMA_INTERRUPT, dma->cap_mask);
	dma->device_prep_dma_interrupt = ioat_prep_interrupt_lock;

	ioat_dma->cap = readl(ioat_dma->reg_base + IOAT_DMA_CAP_OFFSET);

	if (is_xeon_cb32(pdev) || is_bwd_noraid(pdev))
		ioat_dma->cap &=
			~(IOAT_CAP_XOR | IOAT_CAP_PQ | IOAT_CAP_RAID16SS);

	/* dca is incompatible with raid operations */
	if (dca_en && (ioat_dma->cap & (IOAT_CAP_XOR|IOAT_CAP_PQ)))
		ioat_dma->cap &= ~(IOAT_CAP_XOR|IOAT_CAP_PQ);

	if (ioat_dma->cap & IOAT_CAP_XOR) {
		is_raid_device = true;
		dma->max_xor = 8;

		dma_cap_set(DMA_XOR, dma->cap_mask);
		dma->device_prep_dma_xor = ioat_prep_xor;

		dma_cap_set(DMA_XOR_VAL, dma->cap_mask);
		dma->device_prep_dma_xor_val = ioat_prep_xor_val;
	}

	if (ioat_dma->cap & IOAT_CAP_PQ) {
		is_raid_device = true;

		dma->device_prep_dma_pq = ioat_prep_pq;
		dma->device_prep_dma_pq_val = ioat_prep_pq_val;
		dma_cap_set(DMA_PQ, dma->cap_mask);
		dma_cap_set(DMA_PQ_VAL, dma->cap_mask);

		if (ioat_dma->cap & IOAT_CAP_RAID16SS)
			dma_set_maxpq(dma, 16, 0);
		else
			dma_set_maxpq(dma, 8, 0);

		if (!(ioat_dma->cap & IOAT_CAP_XOR)) {
			dma->device_prep_dma_xor = ioat_prep_pqxor;
			dma->device_prep_dma_xor_val = ioat_prep_pqxor_val;
			dma_cap_set(DMA_XOR, dma->cap_mask);
			dma_cap_set(DMA_XOR_VAL, dma->cap_mask);

			if (ioat_dma->cap & IOAT_CAP_RAID16SS)
				dma->max_xor = 16;
			else
				dma->max_xor = 8;
		}
	}

	dma->device_tx_status = ioat_tx_status;

	/* starting with CB3.3 super extended descriptors are supported */
	if (ioat_dma->cap & IOAT_CAP_RAID16SS) {
		char pool_name[14];
		int i;

		for (i = 0; i < MAX_SED_POOLS; i++) {
			snprintf(pool_name, 14, "ioat_hw%d_sed", i);

			/* allocate SED DMA pool */
			ioat_dma->sed_hw_pool[i] = dmam_pool_create(pool_name,
					&pdev->dev,
					SED_SIZE * (i + 1), 64, 0);
			if (!ioat_dma->sed_hw_pool[i])
				return -ENOMEM;

		}
	}

	if (!(ioat_dma->cap & (IOAT_CAP_XOR | IOAT_CAP_PQ)))
		dma_cap_set(DMA_PRIVATE, dma->cap_mask);

	err = ioat_probe(ioat_dma);
	if (err)
		return err;

	list_for_each_entry(c, &dma->channels, device_node) {
		ioat_chan = to_ioat_chan(c);
		writel(IOAT_DMA_DCA_ANY_CPU,
		       ioat_chan->reg_base + IOAT_DCACTRL_OFFSET);
	}

	err = ioat_register(ioat_dma);
	if (err)
		return err;

	ioat_kobject_add(ioat_dma, &ioat_ktype);

	if (dca)
		ioat_dma->dca = ioat_dca_init(pdev, ioat_dma->reg_base);

	return 0;
}

static void ioat_shutdown(struct pci_dev *pdev)
{
	struct ioatdma_device *ioat_dma = pci_get_drvdata(pdev);
	struct ioatdma_chan *ioat_chan;
	int i;

	if (!ioat_dma)
		return;

	for (i = 0; i < IOAT_MAX_CHANS; i++) {
		ioat_chan = ioat_dma->idx[i];
		if (!ioat_chan)
			continue;

		spin_lock_bh(&ioat_chan->prep_lock);
		set_bit(IOAT_CHAN_DOWN, &ioat_chan->state);
		spin_unlock_bh(&ioat_chan->prep_lock);
		/*
		 * Synchronization rule for del_timer_sync():
		 *  - The caller must not hold locks which would prevent
		 *    completion of the timer's handler.
		 * So prep_lock cannot be held before calling it.
		 */
		del_timer_sync(&ioat_chan->timer);

		/* this should quiesce then reset */
		ioat_reset_hw(ioat_chan);
	}

	ioat_disable_interrupts(ioat_dma);
}

void ioat_resume(struct ioatdma_device *ioat_dma)
{
	struct ioatdma_chan *ioat_chan;
	u32 chanerr;
	int i;

	for (i = 0; i < IOAT_MAX_CHANS; i++) {
		ioat_chan = ioat_dma->idx[i];
		if (!ioat_chan)
			continue;

		spin_lock_bh(&ioat_chan->prep_lock);
		clear_bit(IOAT_CHAN_DOWN, &ioat_chan->state);
		spin_unlock_bh(&ioat_chan->prep_lock);

		chanerr = readl(ioat_chan->reg_base + IOAT_CHANERR_OFFSET);
		writel(chanerr, ioat_chan->reg_base + IOAT_CHANERR_OFFSET);

		/* no need to reset as shutdown already did that */
	}
}

#define DRV_NAME "ioatdma"

static pci_ers_result_t ioat_pcie_error_detected(struct pci_dev *pdev,
						 enum pci_channel_state error)
{
	dev_dbg(&pdev->dev, "%s: PCIe AER error %d\n", DRV_NAME, error);

	/* quiesce and block I/O */
	ioat_shutdown(pdev);

	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t ioat_pcie_error_slot_reset(struct pci_dev *pdev)
{
	pci_ers_result_t result = PCI_ERS_RESULT_RECOVERED;
	int err;

	dev_dbg(&pdev->dev, "%s post reset handling\n", DRV_NAME);

	if (pci_enable_device_mem(pdev) < 0) {
		dev_err(&pdev->dev,
			"Failed to enable PCIe device after reset.\n");
		result = PCI_ERS_RESULT_DISCONNECT;
	} else {
		pci_set_master(pdev);
		pci_restore_state(pdev);
		pci_save_state(pdev);
		pci_wake_from_d3(pdev, false);
	}

	err = pci_cleanup_aer_uncorrect_error_status(pdev);
	if (err) {
		dev_err(&pdev->dev,
			"AER uncorrect error status clear failed: %#x\n", err);
	}

	return result;
}

static void ioat_pcie_error_resume(struct pci_dev *pdev)
{
	struct ioatdma_device *ioat_dma = pci_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "%s: AER handling resuming\n", DRV_NAME);

	/* initialize and bring everything back */
	ioat_resume(ioat_dma);
}

static const struct pci_error_handlers ioat_err_handler = {
	.error_detected = ioat_pcie_error_detected,
	.slot_reset = ioat_pcie_error_slot_reset,
	.resume = ioat_pcie_error_resume,
};

static struct pci_driver ioat_pci_driver = {
	.name		= DRV_NAME,
	.id_table	= ioat_pci_tbl,
	.probe		= ioat_pci_probe,
	.remove		= ioat_remove,
	.shutdown	= ioat_shutdown,
	.err_handler	= &ioat_err_handler,
};

static struct ioatdma_device *
alloc_ioatdma(struct pci_dev *pdev, void __iomem *iobase)
{
	struct device *dev = &pdev->dev;
	struct ioatdma_device *d = devm_kzalloc(dev, sizeof(*d), GFP_KERNEL);

	if (!d)
		return NULL;
	d->pdev = pdev;
	d->reg_base = iobase;
	return d;
}

static int ioat_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	void __iomem * const *iomap;
	struct device *dev = &pdev->dev;
	struct ioatdma_device *device;
	int err;

	err = pcim_enable_device(pdev);
	if (err)
		return err;

	err = pcim_iomap_regions(pdev, 1 << IOAT_MMIO_BAR, DRV_NAME);
	if (err)
		return err;
	iomap = pcim_iomap_table(pdev);
	if (!iomap)
		return -ENOMEM;

	err = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (err)
		err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
	if (err)
		return err;

	err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
	if (err)
		err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	if (err)
		return err;

	device = alloc_ioatdma(pdev, iomap[IOAT_MMIO_BAR]);
	if (!device)
		return -ENOMEM;
	pci_set_master(pdev);
	pci_set_drvdata(pdev, device);

	device->version = readb(device->reg_base + IOAT_VER_OFFSET);
	if (device->version >= IOAT_VER_3_0) {
		if (is_skx_ioat(pdev))
			device->version = IOAT_VER_3_2;
		err = ioat3_dma_probe(device, ioat_dca_enabled);

		if (device->version >= IOAT_VER_3_3)
			pci_enable_pcie_error_reporting(pdev);
	} else
		return -ENODEV;

	if (err) {
		dev_err(dev, "Intel(R) I/OAT DMA Engine init failed\n");
		pci_disable_pcie_error_reporting(pdev);
		return -ENODEV;
	}

	return 0;
}

static void ioat_remove(struct pci_dev *pdev)
{
	struct ioatdma_device *device = pci_get_drvdata(pdev);

	if (!device)
		return;

	dev_err(&pdev->dev, "Removing dma and dca services\n");
	if (device->dca) {
		unregister_dca_provider(device->dca, &pdev->dev);
		free_dca_provider(device->dca);
		device->dca = NULL;
	}

	pci_disable_pcie_error_reporting(pdev);
	ioat_dma_remove(device);
}

static int __init ioat_init_module(void)
{
	int err = -ENOMEM;

	pr_info("%s: Intel(R) QuickData Technology Driver %s\n",
		DRV_NAME, IOAT_DMA_VERSION);

	ioat_cache = kmem_cache_create("ioat", sizeof(struct ioat_ring_ent),
					0, SLAB_HWCACHE_ALIGN, NULL);
	if (!ioat_cache)
		return -ENOMEM;

	ioat_sed_cache = KMEM_CACHE(ioat_sed_ent, 0);
	if (!ioat_sed_cache)
		goto err_ioat_cache;

	err = pci_register_driver(&ioat_pci_driver);
	if (err)
		goto err_ioat3_cache;

	return 0;

 err_ioat3_cache:
	kmem_cache_destroy(ioat_sed_cache);

 err_ioat_cache:
	kmem_cache_destroy(ioat_cache);

	return err;
}
module_init(ioat_init_module);

static void __exit ioat_exit_module(void)
{
	pci_unregister_driver(&ioat_pci_driver);
	kmem_cache_destroy(ioat_cache);
}
module_exit(ioat_exit_module);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
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
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/gfp.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/prefetch.h>
#include "../dmaengine.h"
#include "registers.h"
#include "hw.h"
#include "dma.h"

#define MAX_SCF	1024

/* provide a lookup table for setting the source address in the base or
 * extended descriptor of an xor or pq descriptor
 */
static const u8 xor_idx_to_desc = 0xe0;
static const u8 xor_idx_to_field[] = { 1, 4, 5, 6, 7, 0, 1, 2 };
static const u8 pq_idx_to_desc = 0xf8;
static const u8 pq16_idx_to_desc[] = { 0, 0, 1, 1, 1, 1, 1, 1, 1,
				       2, 2, 2, 2, 2, 2, 2 };
static const u8 pq_idx_to_field[] = { 1, 4, 5, 0, 1, 2, 4, 5 };
static const u8 pq16_idx_to_field[] = { 1, 4, 1, 2, 3, 4, 5, 6, 7,
					0, 1, 2, 3, 4, 5, 6 };

static void xor_set_src(struct ioat_raw_descriptor *descs[2],
			dma_addr_t addr, u32 offset, int idx)
{
	struct ioat_raw_descriptor *raw = descs[xor_idx_to_desc >> idx & 1];

	raw->field[xor_idx_to_field[idx]] = addr + offset;
}

static dma_addr_t pq_get_src(struct ioat_raw_descriptor *descs[2], int idx)
{
	struct ioat_raw_descriptor *raw = descs[pq_idx_to_desc >> idx & 1];

	return raw->field[pq_idx_to_field[idx]];
}

static dma_addr_t pq16_get_src(struct ioat_raw_descriptor *desc[3], int idx)
{
	struct ioat_raw_descriptor *raw = desc[pq16_idx_to_desc[idx]];

	return raw->field[pq16_idx_to_field[idx]];
}

static void pq_set_src(struct ioat_raw_descriptor *descs[2],
		       dma_addr_t addr, u32 offset, u8 coef, int idx)
{
	struct ioat_pq_descriptor *pq = (struct ioat_pq_descriptor *) descs[0];
	struct ioat_raw_descriptor *raw = descs[pq_idx_to_desc >> idx & 1];

	raw->field[pq_idx_to_field[idx]] = addr + offset;
	pq->coef[idx] = coef;
}

static void pq16_set_src(struct ioat_raw_descriptor *desc[3],
			dma_addr_t addr, u32 offset, u8 coef, unsigned idx)
{
	struct ioat_pq_descriptor *pq = (struct ioat_pq_descriptor *)desc[0];
	struct ioat_pq16a_descriptor *pq16 =
		(struct ioat_pq16a_descriptor *)desc[1];
	struct ioat_raw_descriptor *raw = desc[pq16_idx_to_desc[idx]];

	raw->field[pq16_idx_to_field[idx]] = addr + offset;

	if (idx < 8)
		pq->coef[idx] = coef;
	else
		pq16->coef[idx - 8] = coef;
}

static struct ioat_sed_ent *
ioat3_alloc_sed(struct ioatdma_device *ioat_dma, unsigned int hw_pool)
{
	struct ioat_sed_ent *sed;
	gfp_t flags = __GFP_ZERO | GFP_ATOMIC;

	sed = kmem_cache_alloc(ioat_sed_cache, flags);
	if (!sed)
		return NULL;

	sed->hw_pool = hw_pool;
	sed->hw = dma_pool_alloc(ioat_dma->sed_hw_pool[hw_pool],
				 flags, &sed->dma);
	if (!sed->hw) {
		kmem_cache_free(ioat_sed_cache, sed);
		return NULL;
	}

	return sed;
}

struct dma_async_tx_descriptor *
ioat_dma_prep_memcpy_lock(struct dma_chan *c, dma_addr_t dma_dest,
			   dma_addr_t dma_src, size_t len, unsigned long flags)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);
	struct ioat_dma_descriptor *hw;
	struct ioat_ring_ent *desc;
	dma_addr_t dst = dma_dest;
	dma_addr_t src = dma_src;
	size_t total_len = len;
	int num_descs, idx, i;

	if (test_bit(IOAT_CHAN_DOWN, &ioat_chan->state))
		return NULL;

	num_descs = ioat_xferlen_to_descs(ioat_chan, len);
	if (likely(num_descs) &&
	    ioat_check_space_lock(ioat_chan, num_descs) == 0)
		idx = ioat_chan->head;
	else
		return NULL;
	i = 0;
	do {
		size_t copy = min_t(size_t, len, 1 << ioat_chan->xfercap_log);

		desc = ioat_get_ring_ent(ioat_chan, idx + i);
		hw = desc->hw;

		hw->size = copy;
		hw->ctl = 0;
		hw->src_addr = src;
		hw->dst_addr = dst;

		len -= copy;
		dst += copy;
		src += copy;
		dump_desc_dbg(ioat_chan, desc);
	} while (++i < num_descs);

	desc->txd.flags = flags;
	desc->len = total_len;
	hw->ctl_f.int_en = !!(flags & DMA_PREP_INTERRUPT);
	hw->ctl_f.fence = !!(flags & DMA_PREP_FENCE);
	hw->ctl_f.compl_write = 1;
	dump_desc_dbg(ioat_chan, desc);
	/* we leave the channel locked to ensure in order submission */

	return &desc->txd;
}


static struct dma_async_tx_descriptor *
__ioat_prep_xor_lock(struct dma_chan *c, enum sum_check_flags *result,
		      dma_addr_t dest, dma_addr_t *src, unsigned int src_cnt,
		      size_t len, unsigned long flags)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);
	struct ioat_ring_ent *compl_desc;
	struct ioat_ring_ent *desc;
	struct ioat_ring_ent *ext;
	size_t total_len = len;
	struct ioat_xor_descriptor *xor;
	struct ioat_xor_ext_descriptor *xor_ex = NULL;
	struct ioat_dma_descriptor *hw;
	int num_descs, with_ext, idx, i;
	u32 offset = 0;
	u8 op = result ? IOAT_OP_XOR_VAL : IOAT_OP_XOR;

	BUG_ON(src_cnt < 2);

	num_descs = ioat_xferlen_to_descs(ioat_chan, len);
	/* we need 2x the number of descriptors to cover greater than 5
	 * sources
	 */
	if (src_cnt > 5) {
		with_ext = 1;
		num_descs *= 2;
	} else
		with_ext = 0;

	/* completion writes from the raid engine may pass completion
	 * writes from the legacy engine, so we need one extra null
	 * (legacy) descriptor to ensure all completion writes arrive in
	 * order.
	 */
	if (likely(num_descs) &&
	    ioat_check_space_lock(ioat_chan, num_descs+1) == 0)
		idx = ioat_chan->head;
	else
		return NULL;
	i = 0;
	do {
		struct ioat_raw_descriptor *descs[2];
		size_t xfer_size = min_t(size_t,
					 len, 1 << ioat_chan->xfercap_log);
		int s;

		desc = ioat_get_ring_ent(ioat_chan, idx + i);
		xor = desc->xor;

		/* save a branch by unconditionally retrieving the
		 * extended descriptor xor_set_src() knows to not write
		 * to it in the single descriptor case
		 */
		ext = ioat_get_ring_ent(ioat_chan, idx + i + 1);
		xor_ex = ext->xor_ex;

		descs[0] = (struct ioat_raw_descriptor *) xor;
		descs[1] = (struct ioat_raw_descriptor *) xor_ex;
		for (s = 0; s < src_cnt; s++)
			xor_set_src(descs, src[s], offset, s);
		xor->size = xfer_size;
		xor->dst_addr = dest + offset;
		xor->ctl = 0;
		xor->ctl_f.op = op;
		xor->ctl_f.src_cnt = src_cnt_to_hw(src_cnt);

		len -= xfer_size;
		offset += xfer_size;
		dump_desc_dbg(ioat_chan, desc);
	} while ((i += 1 + with_ext) < num_descs);

	/* last xor descriptor carries the unmap parameters and fence bit */
	desc->txd.flags = flags;
	desc->len = total_len;
	if (result)
		desc->result = result;
	xor->ctl_f.fence = !!(flags & DMA_PREP_FENCE);

	/* completion descriptor carries interrupt bit */
	compl_desc = ioat_get_ring_ent(ioat_chan, idx + i);
	compl_desc->txd.flags = flags & DMA_PREP_INTERRUPT;
	hw = compl_desc->hw;
	hw->ctl = 0;
	hw->ctl_f.null = 1;
	hw->ctl_f.int_en = !!(flags & DMA_PREP_INTERRUPT);
	hw->ctl_f.compl_write = 1;
	hw->size = NULL_DESC_BUFFER_SIZE;
	dump_desc_dbg(ioat_chan, compl_desc);

	/* we leave the channel locked to ensure in order submission */
	return &compl_desc->txd;
}

struct dma_async_tx_descriptor *
ioat_prep_xor(struct dma_chan *chan, dma_addr_t dest, dma_addr_t *src,
	       unsigned int src_cnt, size_t len, unsigned long flags)
{
	struct ioat