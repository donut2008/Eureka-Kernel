/*
 * amd8111_edac.h, EDAC defs for AMD8111 hypertransport chip
 *
 * Copyright (c) 2008 Wind River Systems, Inc.
 *
 * Authors:	Cao Qingtao <qingtao.cao@windriver.com>
 * 		Benjamin Walsh <benjamin.walsh@windriver.com>
 * 		Hu Yongqi <yongqi.hu@windriver.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _AMD8111_EDAC_H_
#define _AMD8111_EDAC_H_

/************************************************************
 *	PCI Bridge Status and Command Register, DevA:0x04
 ************************************************************/
#define REG_PCI_STSCMD	0x04
enum pci_stscmd_bits {
	PCI_STSCMD_SSE		= BIT(30),
	PCI_STSCMD_RMA		= BIT(29),
	PCI_STSCMD_RTA		= BIT(28),
	PCI_STSCMD_SERREN	= BIT(8),
	PCI_STSCMD_CLEAR_MASK	= (PCI_STSCMD_SSE |
				   PCI_STSCMD_RMA |
				   PCI_STSCMD_RTA)
};

/************************************************************
 *	PCI Bridge Memory Base-Limit Register, DevA:0x1c
 ************************************************************/
#define REG_MEM_LIM     0x1c
enum mem_limit_bits {
	MEM_LIMIT_DPE   = BIT(31),
	MEM_LIMIT_RSE   = BIT(30),
	MEM_LIMIT_RMA   = BIT(29),
	MEM_LIMIT_RTA   = BIT(28),
	MEM_LIMIT_STA   = BIT(27),
	MEM_LIMIT_MDPE  = BIT(24),
	MEM_LIMIT_CLEAR_MASK  = (MEM_LIMIT_DPE |
				 MEM_LIMIT_RSE |
				 MEM_LIMIT_RMA |
				 MEM_LIMIT_RTA |
				 MEM_LIMIT_STA |
				 MEM_LIMIT_MDPE)
};

/************************************************************
 *	HyperTransport Link Control Register, DevA:0xc4
 ************************************************************/
#define REG_HT_LINK	0xc4
enum ht_link_bits {
	HT_LINK_LKFAIL	= BIT(4),
	HT_LINK_CRCFEN	= BIT(1),
	HT_LINK_CLEAR_MASK = (HT_LINK_LKFAIL)
};

/************************************************************
 *	PCI Bridge Interrupt and Bridge Control, DevA:0x3c
 ************************************************************/
#define REG_PCI_INTBRG_CTRL	0x3c
enum pci_intbrg_ctrl_bits {
	PCI_INTBRG_CTRL_DTSERREN	= BIT(27),
	PCI_INTBRG_CTRL_DTSTAT		= BIT(26),
	PCI_INTBRG_CTRL_MARSP		= BIT(21),
	PCI_INTBRG_CTRL_SERREN		= BIT(17),
	PCI_INTBRG_CTRL_PEREN		= BIT(16),
	PCI_INTBRG_CTRL_CLEAR_MASK	= (PCI_INTBRG_CTRL_DTSTAT),
	PCI_INTBRG_CTRL_POLL_MASK	= (PCI_INTBRG_CTRL_DTSERREN |
					   PCI_INTBRG_CTRL_MARSP |
					   PCI_INTBRG_CTRL_SERREN)
};

/************************************************************
 *		I/O Control 1 Register, DevB:0x40
 ************************************************************/
#define REG_IO_CTRL_1 0x40
enum io_ctrl_1_bits {
	IO_CTRL_1_NMIONERR	= BIT(7),
	IO_CTRL_1_LPC_ERR	= BIT(6),
	IO_CTRL_1_PW2LPC	= BIT(1),
	IO_CTRL_1_CLEAR_MASK	= (IO_CTRL_1_LPC_ERR | IO_CTRL_1_PW2LPC)
};

/************************************************************
 *		Legacy I/O Space Registers
 ************************************************************/
#define REG_AT_COMPAT 0x61
enum at_compat_bits {
	AT_COMPAT_SERR		= BIT(7),
	AT_COMPAT_IOCHK		= BIT(6),
	AT_COMPAT_CLRIOCHK	= BIT(3),
	AT_COMPAT_CLRSERR	= BIT(2),
};

struct amd8111_dev_info {
	u16 err_dev;	/* PCI Device ID */
	struct pci_dev *dev;
	int edac_idx;	/* device index */
	char *ctl_name;
	struct edac_device_ctl_info *edac_dev;
	void (*init)(struct amd8111_dev_info *dev_info);
	void (*exit)(struct amd8111_dev_info *dev_info);
	void (*check)(struct edac_device_ctl_info *edac_dev);
};

struct amd8111_pci_info {
	u16 err_dev;	/* PCI Device ID */
	struct pci_dev *dev;
	int edac_idx;	/* pci index */
	const char *ctl_name;
	struct edac_pci_ctl_info *edac_dev;
	void (*init)(struct amd8111_pci_info *dev_info);
	void (*exit)(struct amd8111_pci_info *dev_info);
	void (*check)(struct edac_pci_ctl_info *edac_dev);
};

#endif /* _AMD8111_EDAC_H_ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * amd8131_edac.c, AMD8131 hypertransport chip EDAC kernel module
 *
 * Copyright (c) 2008 Wind River Systems, Inc.
 *
 * Authors:	Cao Qingtao <qingtao.cao@windriver.com>
 * 		Benjamin Walsh <benjamin.walsh@windriver.com>
 * 		Hu Yongqi <yongqi.hu@windriver.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/edac.h>
#include <linux/pci_ids.h>

#include "edac_core.h"
#include "edac_module.h"
#include "amd8131_edac.h"

#define AMD8131_EDAC_REVISION	" Ver: 1.0.0"
#define AMD8131_EDAC_MOD_STR	"amd8131_edac"

/* Wrapper functions for accessing PCI configuration space */
static void edac_pci_read_dword(struct pci_dev *dev, int reg, u32 *val32)
{
	int ret;

	ret = pci_read_config_dword(dev, reg, val32);
	if (ret != 0)
		printk(KERN_ERR AMD8131_EDAC_MOD_STR
			" PCI Access Read Error at 0x%x\n", reg);
}

static void edac_pci_write_dword(struct pci_dev *dev, int reg, u32 val32)
{
	int ret;

	ret = pci_write_config_dword(dev, reg, val32);
	if (ret != 0)
		printk(KERN_ERR AMD8131_EDAC_MOD_STR
			" PCI Access Write Error at 0x%x\n", reg);
}

static char * const bridge_str[] = {
	[NORTH_A] = "NORTH A",
	[NORTH_B] = "NORTH B",
	[SOUTH_A] = "SOUTH A",
	[SOUTH_B] = "SOUTH B",
	[NO_BRIDGE] = "NO BRIDGE",
};

/* Support up to two AMD8131 chipsets on a platform */
static struct amd8131_dev_info amd8131_devices[] = {
	{
	.inst = NORTH_A,
	.devfn = DEVFN_PCIX_BRIDGE_NORTH_A,
	.ctl_name = "AMD8131_PCIX_NORTH_A",
	},
	{
	.inst = NORTH_B,
	.devfn = DEVFN_PCIX_BRIDGE_NORTH_B,
	.ctl_name = "AMD8131_PCIX_NORTH_B",
	},
	{
	.inst = SOUTH_A,
	.devfn = DEVFN_PCIX_BRIDGE_SOUTH_A,
	.ctl_name = "AMD8131_PCIX_SOUTH_A",
	},
	{
	.inst = SOUTH_B,
	.devfn = DEVFN_PCIX_BRIDGE_SOUTH_B,
	.ctl_name = "AMD8131_PCIX_SOUTH_B",
	},
	{.inst = NO_BRIDGE,},
};

static void amd8131_pcix_init(struct amd8131_dev_info *dev_info)
{
	u32 val32;
	struct pci_dev *dev = dev_info->dev;

	/* First clear error detection flags */
	edac_pci_read_dword(dev, REG_MEM_LIM, &val32);
	if (val32 & MEM_LIMIT_MASK)
		edac_pci_write_dword(dev, REG_MEM_LIM, val32);

	/* Clear Discard Timer Timedout flag */
	edac_pci_read_dword(dev, REG_INT_CTLR, &val32);
	if (val32 & INT_CTLR_DTS)
		edac_pci_write_dword(dev, REG_INT_CTLR, val32);

	/* Clear CRC Error flag on link side A */
	edac_pci_read_dword(dev, REG_LNK_CTRL_A, &val32);
	if (val32 & LNK_CTRL_CRCERR_A)
		edac_pci_write_dword(dev, REG_LNK_CTRL_A, val32);

	/* Clear CRC Error flag on link side B */
	edac_pci_read_dword(dev, REG_LNK_CTRL_B, &val32);
	if (val32 & LNK_CTRL_CRCERR_B)
		edac_pci_write_dword(dev, REG_LNK_CTRL_B, val32);

	/*
	 * Then enable all error detections.
	 *
	 * Setup Discard Timer Sync Flood Enable,
	 * System Error Enable and Parity Error Enable.
	 */
	edac_pci_read_dword(dev, REG_INT_CTLR, &val32);
	val32 |= INT_CTLR_PERR | INT_CTLR_SERR | INT_CTLR_DTSE;
	edac_pci_write_dword(dev, REG_INT_CTLR, val32);

	/* Enable overall SERR Error detection */
	edac_pci_read_dword(dev, REG_STS_CMD, &val32);
	val32 |= STS_CMD_SERREN;
	edac_pci_write_dword(dev, REG_STS_CMD, val32);

	/* Setup CRC Flood Enable for link side A */
	edac_pci_read_dword(dev, REG_LNK_CTRL_A, &val32);
	val32 |= LNK_CTRL_CRCFEN;
	edac_pci_write_dword(dev, REG_LNK_CTRL_A, val32);

	/* Setup CRC Flood Enable for link side B */
	edac_pci_read_dword(dev, REG_LNK_CTRL_B, &val32);
	val32 |= LNK_CTRL_CRCFEN;
	edac_pci_write_dword(dev, REG_LNK_CTRL_B, val32);
}

static void amd8131_pcix_exit(struct amd8131_dev_info *dev_info)
{
	u32 val32;
	struct pci_dev *dev = dev_info->dev;

	/* Disable SERR, PERR and DTSE Error detection */
	edac_pci_read_dword(dev, REG_INT_CTLR, &val32);
	val32 &= ~(INT_CTLR_PERR | INT_CTLR_SERR | INT_CTLR_DTSE);
	edac_pci_write_dword(dev, REG_INT_CTLR, val32);

	/* Disable overall System Error detection */
	edac_pci_read_dword(dev, REG_STS_CMD, &val32);
	val32 &= ~STS_CMD_SERREN;
	edac_pci_write_dword(dev, REG_STS_CMD, val32);

	/* Disable CRC Sync Flood on link side A */
	edac_pci_read_dword(dev, REG_LNK_CTRL_A, &val32);
	val32 &= ~LNK_CTRL_CRCFEN;
	edac_pci_write_dword(dev, REG_LNK_CTRL_A, val32);

	/* Disable CRC Sync Flood on link side B */
	edac_pci_read_dword(dev, REG_LNK_CTRL_B, &val32);
	val32 &= ~LNK_CTRL_CRCFEN;
	edac_pci_write_dword(dev, REG_LNK_CTRL_B, val32);
}

static void amd8131_pcix_check(struct edac_pci_ctl_info *edac_dev)
{
	struct amd8131_dev_info *dev_info = edac_dev->pvt_info;
	struct pci_dev *dev = dev_info->dev;
	u32 val32;

	/* Check PCI-X Bridge Memory Base-Limit Register for errors */
	edac_pci_read_dword(dev, REG_MEM_LIM, &val32);
	if (val32 & MEM_LIMIT_MASK) {
		printk(KERN_INFO "Error(s) in mem limit register "
			"on %s bridge\n", dev_info->ctl_name);
		printk(KERN_INFO "DPE: %d, RSE: %d, RMA: %d\n"
			"RTA: %d, STA: %d, MDPE: %d\n",
			val32 & MEM_LIMIT_DPE,
			val32 & MEM_LIMIT_RSE,
			val32 & MEM_LIMIT_RMA,
			val32 & MEM_LIMIT_RTA,
			val32 & MEM_LIMIT_STA,
			val32 & MEM_LIMIT_MDPE);

		val32 |= MEM_LIMIT_MASK;
		edac_pci_write_dword(dev, REG_MEM_LIM, val32);

		edac_pci_handle_npe(edac_dev, edac_dev->ctl_name);
	}

	/* Check if Discard Timer timed out */
	edac_pci_read_dword(dev, REG_INT_CTLR, &val32);
	if (val32 & INT_CTLR_DTS) {
		printk(KERN_INFO "Error(s) in interrupt and control register "
			"on %s bridge\n", dev_info->ctl_name);
		printk(KERN_INFO "DTS: %d\n", val32 & INT_CTLR_DTS);

		val32 |= INT_CTLR_DTS;
		edac_pci_write_dword(dev, REG_INT_CTLR, val32);

		edac_pci_handle_npe(edac_dev, edac_dev->ctl_name);
	}

	/* Check if CRC error happens on link side A */
	edac_pci_read_dword(dev, REG_LNK_CTRL_A, &val32);
	if (val32 & LNK_CTRL_CRCERR_A) {
		printk(KERN_INFO "Error(s) in link conf and control register "
			"on %s bridge\n", dev_info->ctl_name);
		printk(KERN_INFO "CRCERR: %d\n", val32 & LNK_CTRL_CRCERR_A);

		val32 |= LNK_CTRL_CRCERR_A;
		edac_pci_write_dword(dev, REG_LNK_CTRL_A, val32);

		edac_pci_handle_npe(edac_dev, edac_dev->ctl_name);
	}

	/* Check if CRC error happens on link side B */
	edac_pci_read_dword(dev, REG_LNK_CTRL_B, &val32);
	if (val32 & LNK_CTRL_CRCERR_B) {
		printk(KERN_INFO "Error(s) in link conf and control register "
			"on %s bridge\n", dev_info->ctl_name);
		printk(KERN_INFO "CRCERR: %d\n", val32 & LNK_CTRL_CRCERR_B);

		val32 |= LNK_CTRL_CRCERR_B;
		edac_pci_write_dword(dev, REG_LNK_CTRL_B, val32);

		edac_pci_handle_npe(edac_dev, edac_dev->ctl_name);
	}
}

static struct amd8131_info amd8131_chipset = {
	.err_dev = PCI_DEVICE_ID_AMD_8131_APIC,
	.devices = amd8131_devices,
	.init = amd8131_pcix_init,
	.exit = amd8131_pcix_exit,
	.check = amd8131_pcix_check,
};

/*
 * There are 4 PCIX Bridges on ATCA-6101 that share the same PCI Device ID,
 * so amd8131_probe() would be called by kernel 4 times, with different
 * address of pci_dev for each of them each time.
 */
static int amd8131_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct amd8131_dev_info *dev_info;

	for (dev_info = amd8131_chipset.devices; dev_info->inst != NO_BRIDGE;
		dev_info++)
		if (dev_info->devfn == dev->devfn)
			break;

	if (dev_info->inst == NO_BRIDGE) /* should never happen */
		return -ENODEV;

	/*
	 * We can't call pci_get_device() as we are used to do because
	 * there are 4 of them but pci_dev_get() instead.
	 */
	dev_info->dev = pci_dev_get(dev);

	if (pci_enable_device(dev_info->dev)) {
		pci_dev_put(dev_info->dev);
		printk(KERN_ERR "failed to enable:"
			"vendor %x, device %x, devfn %x, name %s\n",
			PCI_VENDOR_ID_AMD, amd8131_chipset.err_dev,
			dev_info->devfn, dev_info->ctl_name);
		return -ENODEV;
	}

	/*
	 * we do not allocate extra private structure for
	 * edac_pci_ctl_info, but make use of existing
	 * one instead.
	 */
	dev_info->edac_idx = edac_pci_alloc_index();
	dev_info->edac_dev = edac_pci_alloc_ctl_info(0, dev_info->ctl_name);
	if (!dev_info->edac_dev)
		return -ENOMEM;

	dev_info->edac_dev->pvt_info = dev_info;
	dev_info->edac_dev->dev = &dev_info->dev->dev;
	dev_info->edac_dev->mod_name = AMD8131_EDAC_MOD_STR;
	dev_info->edac_dev->ctl_name = dev_info->ctl_name;
	dev_info->edac_dev->dev_name = dev_name(&dev_info->dev->dev);

	if (edac_op_state == EDAC_OPSTATE_POLL)
		dev_info->edac_dev->edac_check = amd8131_chipset.check;

	if (amd8131_chipset.init)
		amd8131_chipset.init(dev_info);

	if (edac_pci_add_device(dev_info->edac_dev, dev_info->edac_idx) > 0) {
		printk(KERN_ERR "failed edac_pci_add_device() for %s\n",
			dev_info->ctl_name);
		edac_pci_free_ctl_info(dev_info->edac_dev);
		return -ENODEV;
	}

	printk(KERN_INFO "added one device on AMD8131 "
		"vendor %x, device %x, devfn %x, name %s\n",
		PCI_VENDOR_ID_AMD, amd8131_chipset.err_dev,
		dev_info->devfn, dev_info->ctl_name);

	return 0;
}

static void amd8131_remove(struct pci_dev *dev)
{
	struct amd8131_dev_info *dev_info;

	for (dev_info = amd8131_chipset.devices; dev_info->inst != NO_BRIDGE;
		dev_info++)
		if (dev_info->devfn == dev->devfn)
			break;

	if (dev_info->inst == NO_BRIDGE) /* should never happen */
		return;

	if (dev_info->edac_dev) {
		edac_pci_del_device(dev_info->edac_dev->dev);
		edac_pci_free_ctl_info(dev_info->edac_dev);
	}

	if (amd8131_chipset.exit)
		amd8131_chipset.exit(dev_info);

	pci_dev_put(dev_info->dev);
}

static const struct pci_device_id amd8131_edac_pci_tbl[] = {
	{
	PCI_VEND_DEV(AMD, 8131_BRIDGE),
	.subvendor = PCI_ANY_ID,
	.subdevice = PCI_ANY_ID,
	.class = 0,
	.class_mask = 0,
	.driver_data = 0,
	},
	{
	0,
	}			/* table is NULL-terminated */
};
MODULE_DEVICE_TABLE(pci, amd8131_edac_pci_tbl);

static struct pci_driver amd8131_edac_driver = {
	.name = AMD8131_EDAC_MOD_STR,
	.probe = amd8131_probe,
	.remove = amd8131_remove,
	.id_table = amd8131_edac_pci_tbl,
};

static int __init amd8131_edac_init(void)
{
	printk(KERN_INFO "AMD8131 EDAC driver " AMD8131_EDAC_REVISION "\n");
	printk(KERN_INFO "\t(c) 2008 Wind River Systems, Inc.\n");

	/* Only POLL mode supported so far */
	edac_op_state = EDAC_OPSTATE_POLL;

	return pci_register_driver(&amd8131_edac_driver);
}

static void __exit amd8131_edac_exit(void)
{
	pci_unregister_driver(&amd8131_edac_driver);
}

module_init(amd8131_edac_init);
module_exit(amd8131_edac_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cao Qingtao <qingtao.cao@windriver.com>\n");
MODULE_DESCRIPTION("AMD8131 HyperTransport PCI-X Tunnel EDAC kernel module");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * amd8131_edac.h, EDAC defs for AMD8131 hypertransport chip
 *
 * Copyright (c) 2008 Wind River Systems, Inc.
 *
 * Authors:	Cao Qingtao <qingtao.cao@windriver.com>
 * 		Benjamin Walsh <benjamin.walsh@windriver.com>
 * 		Hu Yongqi <yongqi.hu@windriver.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _AMD8131_EDAC_H_
#define _AMD8131_EDAC_H_

#define DEVFN_PCIX_BRIDGE_NORTH_A	8
#define DEVFN_PCIX_BRIDGE_NORTH_B	16
#define DEVFN_PCIX_BRIDGE_SOUTH_A	24
#define DEVFN_PCIX_BRIDGE_SOUTH_B	32

/************************************************************
 *	PCI-X Bridge Status and Command Register, DevA:0x04
 ************************************************************/
#define REG_STS_CMD	0x04
enum sts_cmd_bits {
	STS_CMD_SSE	= BIT(30),
	STS_CMD_SERREN	= BIT(8)
};

/************************************************************
 *	PCI-X Bridge Interrupt and Bridge Control Register,
 ************************************************************/
#define REG_INT_CTLR	0x3c
enum int_ctlr_bits {
	INT_CTLR_DTSE	= BIT(27),
	INT_CTLR_DTS	= BIT(26),
	INT_CTLR_SERR	= BIT(17),
	INT_CTLR_PERR	= BIT(16)
};

/************************************************************
 *	PCI-X Bridge Memory Base-Limit Register, DevA:0x1C
 ************************************************************/
#define REG_MEM_LIM	0x1c
enum mem_limit_bits {
	MEM_LIMIT_DPE 	= BIT(31),
	MEM_LIMIT_RSE 	= BIT(30),
	MEM_LIMIT_RMA 	= BIT(29),
	MEM_LIMIT_RTA 	= BIT(28),
	MEM_LIMIT_STA	= BIT(27),
	MEM_LIMIT_MDPE	= BIT(24),
	MEM_LIMIT_MASK	= MEM_LIMIT_DPE|MEM_LIMIT_RSE|MEM_LIMIT_RMA|
				MEM_LIMIT_RTA|MEM_LIMIT_STA|MEM_LIMIT_MDPE
};

/************************************************************
 *	Link Configuration And Control Register, side A
 ************************************************************/
#define REG_LNK_CTRL_A	0xc4

/************************************************************
 *	Link Configuration And Control Register, side B
 ************************************************************/
#define REG_LNK_CTRL_B  0xc8

enum lnk_ctrl_bits {
	LNK_CTRL_CRCERR_A	= BIT(9),
	LNK_CTRL_CRCERR_B	= BIT(8),
	LNK_CTRL_CRCFEN		= BIT(1)
};

enum pcix_bridge_inst {
	NORTH_A = 0,
	NORTH_B = 1,
	SOUTH_A = 2,
	SOUTH_B = 3,
	NO_BRIDGE = 4
};

struct amd8131_dev_info {
	int devfn;
	enum pcix_bridge_inst inst;
	struct pci_dev *dev;
	int edac_idx;	/* pci device index */
	char *ctl_name;
	struct edac_pci_ctl_info *edac_dev;
};

/*
 * AMD8131 chipset has two pairs of PCIX Bridge and related IOAPIC
 * Controller, and ATCA-6101 has two AMD8131 chipsets, so there are
 * four PCIX Bridges on ATCA-6101 altogether.
 *
 * These PCIX Bridges share the same PCI Device ID and are all of
 * Function Zero, they could be discrimated by their pci_dev->devfn.
 * They share the same set of init/check/exit methods, and their
 * private structures are collected in the devices[] array.
 */
struct amd8131_info {
	u16 err_dev;	/* PCI Device ID for AMD8131 APIC*/
	struct amd8131_dev_info *devices;
	void (*init)(struct amd8131_dev_info *dev_info);
	void (*exit)(struct amd8131_dev_info *dev_info);
	void (*check)(struct edac_pci_ctl_info *edac_dev);
};

#endif /* _AMD8131_EDAC_H_ */

                                                                                                                                                                                                                                                                                                                                   /*
 * Cell MIC driver for ECC counting
 *
 * Copyright 2007 Benjamin Herrenschmidt, IBM Corp.
 *                <benh@kernel.crashing.org>
 *
 * This file may be distributed under the terms of the
 * GNU General Public License.
 */
#undef DEBUG

#include <linux/edac.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/stop_machine.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <asm/machdep.h>
#include <asm/cell-regs.h>

#include "edac_core.h"

struct cell_edac_priv
{
	struct cbe_mic_tm_regs __iomem	*regs;
	int				node;
	int				chanmask;
#ifdef DEBUG
	u64				prev_fir;
#endif
};

static void cell_edac_count_ce(struct mem_ctl_info *mci, int chan, u64 ar)
{
	struct cell_edac_priv		*priv = mci->pvt_info;
	struct csrow_info		*csrow = mci->csrows[0];
	unsigned long			address, pfn, offset, syndrome;

	dev_dbg(mci->pdev, "ECC CE err on node %d, channel %d, ar = 0x%016llx\n",
		priv->node, chan, ar);

	/* Address decoding is likely a bit bogus, to dbl check */
	address = (ar & 0xffffffffe0000000ul) >> 29;
	if (priv->chanmask == 0x3)
		address = (address << 1) | chan;
	pfn = address >> PAGE_SHIFT;
	offset = address & ~PAGE_MASK;
	syndrome = (ar & 0x000000001fe00000ul) >> 21;

	/* TODO: Decoding of the error address */
	edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1,
			     csrow->first_page + pfn, offset, syndrome,
			     0, chan, -1, "", "");
}

static void cell_edac_count_ue(struct mem_ctl_info *mci, int chan, u64 ar)
{
	struct cell_edac_priv		*priv = mci->pvt_info;
	struct csrow_info		*csrow = mci->csrows[0];
	unsigned long			address, pfn, offset;

	dev_dbg(mci->pdev, "ECC UE err on node %d, channel %d, ar = 0x%016llx\n",
		priv->node, chan, ar);

	/* Address decoding is likely a bit bogus, to dbl check */
	address = (ar & 0xffffffffe0000000ul) >> 29;
	if (priv->chanmask == 0x3)
		address = (address << 1) | chan;
	pfn = address >> PAGE_SHIFT;
	offset = address & ~PAGE_MASK;

	/* TODO: Decoding of the error address */
	edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1,
			     csrow->first_page + pfn, offset, 0,
			     0, chan, -1, "", "");
}

static void cell_edac_check(struct mem_ctl_info *mci)
{
	struct cell_edac_priv		*priv = mci->pvt_info;
	u64				fir, addreg, clear = 0;

	fir = in_be64(&priv->regs->mic_fir);
#ifdef DEBUG
	if (fir != priv->prev_fir) {
		dev_dbg(mci->pdev, "fir change : 0x%016lx\n", fir);
		priv->prev_fir = fir;
	}
#endif
	if ((priv->chanmask & 0x1) && (fir & CBE_MIC_FIR_ECC_SINGLE_0_