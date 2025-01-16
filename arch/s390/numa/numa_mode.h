/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000-2006 Silicon Graphics, Inc. All rights reserved.
 */
#ifndef _ASM_IA64_SN_PCI_PCIDEV_H
#define _ASM_IA64_SN_PCI_PCIDEV_H

#include <linux/pci.h>

/*
 * In ia64, pci_dev->sysdata must be a *pci_controller. To provide access to
 * the pcidev_info structs for all devices under a controller, we keep a
 * list of pcidev_info under pci_controller->platform_data.
 */
struct sn_platform_data {
	void *provider_soft;
	struct list_head pcidev_info;
};

#define SN_PLATFORM_DATA(busdev) \
	((struct sn_platform_d