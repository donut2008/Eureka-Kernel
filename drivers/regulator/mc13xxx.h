/*
 * Intel e7xxx Memory Controller kernel module
 * (C) 2003 Linux Networx (http://lnxi.com)
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * See "enum e7xxx_chips" below for supported chipsets
 *
 * Written by Thayne Harbaugh
 * Based on work by Dan Hollis <goemon at anime dot net> and others.
 *	http://www.anime.net/~goemon/linux-ecc/
 *
 * Datasheet:
 *	http://www.intel.com/content/www/us/en/chipsets/e7501-chipset-memory-controller-hub-datasheet.html
 *
 * Contributors:
 *	Eric Biederman (Linux Networx)
 *	Tom Zimmerman (Linux Networx)
 *	Jim Garlick (Lawrence Livermore National Labs)
 *	Dave Peterson (Lawrence Livermore National Labs)
 *	That One Guy (Some other place)
 *	Wang Zhenyu (intel.com)
 *
 * $Id: edac_e7xxx.c,v 1.5.2.9 2005/10/05 00:43:44 dsp_llnl Exp $
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/edac.h>
#include "edac_core.h"

#define	E7XXX_REVISION " Ver: 2.0.2"
#define	EDAC_MOD_STR	"e7xxx_edac"

#define e7xxx_printk(level, fmt, arg...) \
	edac_printk(level, "e7xxx", fmt, ##arg)

#define e7xxx_mc_printk(mci, level, fmt, arg...) \
	edac_mc_chipset_printk(mci, level, "e7xxx", fmt, ##arg)

#ifndef PCI_DEVICE_ID_INTEL_7205_0
#define PCI_DEVICE_ID_INTEL_7205_0	0x255d
#endif				/* PCI_DEVICE_ID_INTEL_7205_0 */

#ifndef PCI_DEVICE_ID_INTEL_7205_1_ERR
#define PCI_DEVICE_ID_INTEL_7205_1_ERR	0x2551
#endif				/* PCI_DEVICE_ID_INTEL_7205_1_ERR */

#ifndef PCI_DEVICE_ID_INTEL_7500_0
#define PCI_DEVICE_ID_INTEL_7500_0	0x2540
#endif				/* PCI_DEVICE_ID_INTEL_7500_0 */

#ifndef PCI_DEVICE_ID_INTEL_7500_1_ERR
#define PCI_DEVICE_ID_INTEL_7500_1_ERR	0x2541
#endif				/* PCI_DEVICE_ID_INTEL_7500_1_ERR */

#ifndef PCI_DEVICE_ID_INTEL_7501_0
#define PCI_DEVICE_ID_INTEL_7501_0	0x254c
#endif				/* PCI_DEVICE_ID_INTEL_7501_0 */

#ifndef PCI_DEVICE_ID_INTEL_7501_1_ERR
#define PCI_DEVICE_ID_INTEL_7501_1_ERR	0x2541
#endif				/* PCI_DEVICE_ID_INTEL_7501_1_ERR */

#ifndef PCI_DEVICE_ID_INTEL_7505_0
#define PCI_DEVICE_ID_INTEL_7505_0	0x2550
#endif				/* PCI_DEVICE_ID_INTEL_7505_0 */

#ifndef PCI_DEVICE_ID_INTEL_7505_1_ERR
#define PCI_DEVICE_ID_INTEL_7505_1_ERR	0x2551
#endif				/* PCI_DEVICE_ID_INTEL_7505_1_ERR */

#define E7XXX_NR_CSROWS		8	/* number of csrows */
#define E7XXX_NR_DIMMS		8	/* 2 channels, 4 dimms/channel */

/* E7XXX register addresses - device 0 function 0 */
#define E7XXX_DRB		0x60	/* DRAM row boundary register (8b) */
#define E7XXX_DRA		0x70	/* DRAM row attribute register (8b) */
					/*
					 * 31   Device width row 7 0=x8 1=x4
					 * 27   Device width row 6
					 * 23   Device width row 5
					 * 19   Device width row 4
					 * 15   Device width row 3
					 * 11   Device width row 2
					 *  7   Device width row 1
					 *  3   Device width row 0
					 */
#define E7XXX_DRC		0x7C	/* DRAM controller mode reg (32b) */
					/*
					 * 22    Number channels 0=1,1=2
					 * 19:18 DRB Granularity 32/64MB
					 */
#define E7XXX_TOLM		0xC4	/* DRAM top of low memory reg (16b) */
#define E7XXX_REMAPBASE		0xC6	/* DRAM remap base address reg (16b) */
#define E7XXX_REMAPLIMIT	0xC8	/* DRAM remap limit address reg (16b) */

/* E7XXX register addresses - device 0 function 1 */
#define E7XXX_DRAM_FERR		0x80	/* DRAM first error register (8b) */
#define E7XXX_DRA