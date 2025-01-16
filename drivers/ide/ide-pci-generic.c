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
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/dmaengine.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>
#include <linux/prefetch.h>
#include <linux/dca.h>
#include <linux/aer.h>
#include "dma.h"
#include "registers.h"
#include "hw.h"

#include "../dmaengine.h"

MODULE_VERSION(IOAT_DMA_VERSION);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");

static struct pci_device_id ioat_pci_tbl[] = {
	/* I/OAT v3 platforms */
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_TBG0) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_TBG1) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_TBG2) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_TBG3) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_TBG4) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_TBG5) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_TBG6) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_TBG7) },

	/* I/OAT v3.2 platforms */
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF0) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF1) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF2) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF3) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF4) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF5) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF6) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF7) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF8) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_JSF9) },

	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB0) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB1) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB2) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB3) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB4) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB5) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB6) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB7) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB8) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SNB9) },

	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB0) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB1) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB2) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB3) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB4) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB5) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB6) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB7) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB8) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_IVB9) },

	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW0) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW1) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW2) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW3) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW4) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW5) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW6) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW7) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW8) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_HSW9) },

	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX0) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX1) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX2) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX3) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX4) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX5) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX6) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX7) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX8) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDX9) },

	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_SKX) },

	/* I/OAT v3.3 platforms */
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BWD0) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BWD1) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BWD2) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BWD3) },

	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDXDE0) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDXDE1) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDXDE2) },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_INTEL_IOAT_BDXDE3) },

	{ 0, }
};
MODULE_DEVICE_TABLE(pci, ioat_pci_tbl);

static int ioat_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id);
static void ioat_remove(struct pci_dev *pdev);
static void
ioat_init_channel(struct ioatdma_device *ioat_dma,
		  struct ioatdma_chan *ioat_chan, int idx);
static void ioat_intr_quirk(struct ioatdma_device *ioat_dma);
static void ioat_enumerate_channels(struct ioatdma_device *ioat_dma);
static int ioat3_dma_self_test(struct ioatdma_device *ioat_dma);

static int ioat_dca_enabled = 1;
module_param(ioat_dca_enabled, int, 0644);
MODULE_PARM_DESC(ioat_dca_enabled, "control support of dca service (default: 1)");
int ioat_pending_level = 4;
module_param(ioat_pending_level, int, 0644);
MODULE_PARM_DESC(ioat_pending_level,
		 "high-water mark for pushing ioat descriptors (default: 4)");
int ioat_ring_alloc_order = 8;
module_param(ioat_ring_alloc_order, int, 0644);
MODULE_PARM_DESC(ioat_ring_alloc_order,
		 "ioat+: allocate 2^n descriptors per channel (default: 8 max: 16)");
int ioat_ring_max_alloc_order = IOAT_MAX_ORDER;
module_param(ioat_ring_max_alloc_order, int, 0644);
MODULE_