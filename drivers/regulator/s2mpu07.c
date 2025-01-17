/*
 * Copyright 2011-2012 Calxeda, Inc.
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
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/edac.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>

#include "edac_core.h"
#include "edac_module.h"

/* DDR Ctrlr Error Registers */

#define HB_DDR_ECC_ERR_BASE		0x128
#define MW_DDR_ECC_ERR_BASE		0x1b4

#define HB_DDR_ECC_OPT			0x00
#define HB_DDR_ECC_U_ERR_ADDR		0x08
#define HB_DDR_ECC_U_ERR_STAT		0x0c
#define HB_DDR_ECC_U_ERR_DATAL		0x10
#define HB_DDR_ECC_U_ERR_DATAH		0x14
#define HB_DDR_ECC_C_ERR_ADDR		0x18
#define HB_DDR_ECC_C_ERR_STAT		0x1c
#define HB_DDR_ECC_C_ERR_DATAL		0x20
#define HB_DDR_ECC_C_ERR_DATAH		0x24

#define HB_DDR_ECC_OPT_MODE_MASK	0x3
#define HB_DDR_ECC_OPT_FWC		0x100
#define HB_DDR_ECC_OPT_XOR_SHIFT	16

/* DDR Ctrlr Interrupt Registers */

#define HB_DDR_ECC_INT_BASE		0x180
#define MW_DDR_ECC_INT_BASE		0x218

#define HB_DDR_ECC_INT_STATUS		0x00
#define HB_DDR_ECC_INT_ACK		0x04

#define HB_DDR_ECC_INT_STAT_CE		0x8
#define HB_DDR_ECC_INT_STAT_DOUBLE_CE	0x10
#define HB_DDR_ECC_INT_STAT_UE		0x20
#define HB_DDR_ECC_INT_STAT_DOUBLE_UE	0x40

struct hb_mc_drvdata {
	void __iomem *mc_err_base;
	void __iomem *mc_int_base;
};

static irqreturn_t highbank_mc_err_handler(int irq, void *dev_id)
{
	struct mem_ctl_info *mci = dev_id;
	struct hb_mc_drvdata *drvdata = mci->pvt_info;
	u32 status, err_addr;

	/* Read the interrupt status register */
	status = readl(drvdata->mc_int_base + HB_DDR_ECC_INT_STATUS);

	if (status & HB_DDR_ECC_INT_STAT_UE) {
		err_addr = readl(drvdata->mc_err_base + HB_DDR_ECC_U_ERR_ADDR);
		edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1,
				     err_addr >> PAGE_SHIFT,
				     err_addr & ~PAGE_MASK, 0,
				     0, 0, -1,
				     mci->ctl_name, "");
	}
	if (status & HB_DDR_ECC_INT_STAT_CE) {
		u32 syndrome = readl(drvdata->mc_err_base + HB_DDR_ECC_C_ERR_STAT);
		syndrome = (syndrome >> 8) & 0xff;
		err_addr = readl(drvdata->mc_err_base + HB_DDR_ECC_C_ERR_ADDR);
		edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1,
				     err_addr >> PAGE_SHIFT,
				     err_addr & ~PAGE_MASK, syndrome,
				     0, 0, -1,
				     mci->ctl_name, "");
	}

	/* clear the error, clears the interrupt */
	writel(status, drvdata->mc_int_base + HB_DDR_ECC_INT_ACK);
	return IRQ_HANDLED;
}

static void highbank_mc_err_inject(struct mem_ctl_info *mci, u8 synd)
{
	struct hb_mc_drvdata *pdata = mci->pvt_info;
	u32 reg;

	reg = readl(pdata->mc_err_base + HB_DDR_ECC_OPT);
	reg &= HB_DDR_ECC_OPT_MODE_MASK;
	reg |= (synd << HB_DDR_ECC_OPT_XOR_SHIFT) | HB_DDR_ECC_OPT_FWC;
	writel(reg, pdata->mc_err_base + HB_DDR_ECC_OPT);
}

#define to_mci(k) container_of(k, struct mem_ctl_info, dev)

static ssize_t highbank_mc_inject_ctrl(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mem_ctl_info *mci = to_mci(dev);
	u8 synd;

	if (kstrtou8(buf, 16, &synd))
		return -EINVAL;

	highbank_mc_err_inject(mci, synd);

	return count;
}

static DEVICE_ATTR(inject_ctrl, S_IWUSR, NULL, highbank_mc_inject_ctrl);

static struct attribute *highbank_dev_attrs[] = {
	&dev_attr_inject_ctrl.attr,
	NULL
};

ATTRIBUTE_GROUPS(highbank_dev);

struct hb_mc_settings {
	int	err_offset;
	int	int_offset;
};

static struct hb_mc_settings hb_settings = {
	.err_offset = HB_DDR_ECC_ERR_BASE,
	.int_offset = HB_DDR_ECC_INT_BASE,
};

static struct hb_mc_settings mw_settings = {
	.err_offset = MW_DDR_ECC_ERR_BASE,
	.int_offset = MW_DDR_ECC_INT_BASE,
};

static const struct of_device_id hb_ddr_ctrl_of_match[] = {
	{ .compatible = "calxeda,hb-ddr-ctrl",		.data = &hb_settings },
	{ .compatible = "calxeda,ecx-2000-ddr-ctrl",	.data = &mw_settings },
	{},
};
MODULE_DEVICE_TABLE(of, hb_ddr_ctrl_of_match);

static int highbank_mc_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	const struct hb_mc_settings *settings;
	struct edac_mc_layer layers[2];
	struct mem_ctl_info *mci;
	struct hb_mc_drvdata *drvdata;
	struct dimm_info *dimm;
	struct resource *r;
	void __iomem *base;
	u32 control;
	int irq;
	int res = 0;

	id = of_match_device(hb_ddr_ctrl_of_match, &pdev->dev);
	if (!id)
		return -ENODEV;

	layers[0].type = EDAC_MC_LAYER_CHIP_SELECT;
	layers[0].size = 1;
	layers[0].is_virt_csrow = true;
	layers[1].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = 1;
	layers[1].is_virt_csrow = false;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers,
			    sizeof(struct hb_mc_drvdata));
	if (!mci)
		return -ENOMEM;

	mci->pdev = &pdev->dev;
	drvdata = mci->pvt_info;
	platform_set_drvdata(pdev, mci);

	if (!devres_open_group(&pdev->dev, NULL, GFP_KERNEL)) {
		res = -ENOMEM;
		goto free;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(&pdev->dev, "Unable to get mem resource\n");
		res = -ENODEV;
		goto err;
	}

	if (!devm_request_mem_region(&pdev->dev, r->start,
				     resource_size(r), dev_name(&pdev->dev))) {
		dev_err(&pdev->dev, "Error while requesting mem region\n");
		res = -EBUSY;
		goto err;
	}

	base = devm_ioremap(&pdev->dev, r->start, resource_size(r));
	if (!base) {
		dev_err(&pdev->dev, "Unable to map regs\n");
		res = -ENOMEM;
		goto err;
	}

	settings = id->data;
	drvdata->mc_err_base = base + settings->err_offset;
	drvdata->mc_int_base = base + settings->int_offset;

	control = readl(drvdata->mc_err_base + HB_DDR_ECC_OPT) & 0x3;
	if (!control || (control == 0x2)) {
		dev_err(&pdev->dev, "No ECC present, or ECC disabled\n");
		res = -ENODEV;
		goto err;
	}

	mci->mtype_cap = MEM_FLAG_DDR3;
	mci->edac_ctl_cap = EDAC_FLAG_NONE | EDAC_FLAG_SECDED;
	mci->edac_cap = EDAC_FLAG_SECDED;
	mci->mod_name = pdev->dev.driver->name;
	mci->mod_ver = "1";
	mci->ctl_name = id->compatible;
	mci->dev_name = dev_name(&pdev->dev);
	mci->scrub_mode = SCRUB_SW_SRC;

	/* Only a single 4GB DIMM is supported */
	dimm = *mci->dimms;
	dimm->nr_pages = (~0UL >> PAGE_SHIFT) + 1;
	dimm->grain = 8;
	dimm->dtype = DEV_X8;
	dimm->mtype = MEM_DDR3;
	dimm->edac_mode = EDAC_SECDED;

	res = edac_mc_add_mc_with_groups(mci, highbank_dev_groups);
	if (res < 0)
		goto err;

	irq = platform_get_irq(pdev, 0);
	res = devm_request_irq(&pdev->dev, irq, highbank_mc_err_handler,
			       0, dev_name(&pdev->dev), mci);
	if (res < 0) {
		dev_err(&pdev->dev, "Unable to request irq %d\n", irq);
		goto err2;
	}

	devres_close_group(&pdev->dev, NULL);
	return 0;
err2:
	edac_mc_del_mc(&pdev->dev);
err:
	devres_release_group(&pdev->dev, NULL);
free:
	edac_mc_free(mci);
	return res;
}

static int highbank_mc_remove(struct platform_device *pdev)
{
	struct mem_ctl_info *mci = platform_get_drvdata(pdev);

	edac_mc_del_mc(&pdev->dev);
	edac_mc_free(mci);
	return 0;
}

static struct platform_driver highbank_mc_edac_driver = {
	.probe = highbank_mc_probe,
	.remove = highbank_mc_remove,
	.driver = {
		.name = "hb_mc_edac",
		.of_match_table = hb_ddr_ctrl_of_match,
	},
};

module_platform_driver(highbank_mc_edac_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Calxeda, Inc.");
MODULE_DESCRIPTION("EDAC Driver for Calxeda Highbank");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * Intel 3000/3010 Memory Controller kernel module
 * Copyright (C) 2007 Akamai Technologies, Inc.
 * Shamelessly copied from:
 * 	Intel D82875P Memory Controller kernel module
 * 	(C) 2003 Linux Networx (http://lnxi.com)
 *
 * This file may be distributed under the terms of the
 * GNU General Public License.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/edac.h>
#include "edac_core.h"

#define I3000_REVISION		"1.1"

#define EDAC_MOD_STR		"i3000_edac"

#define I3000_RANKS		8
#define I3000_RANKS_PER_CHANNEL	4
#define I3000_CHANNELS		2

/* Intel 3000 register addresses - device 0 function 0 - DRAM Controller */

#define I3000_MCHBAR		0x44	/* MCH Memory Mapped Register BAR */
#define I3000_MCHBAR_MASK	0xffffc000
#define I3000_MMR_WINDOW_SIZE	16384

#define I3000_EDEAP	0x70	/* Extended DRAM Error Address Pointer (8b)
				 *
				 * 7:1   reserved
				 * 0     bit 32 of address
				 */
#define I3000_DEAP	0x58	/* DRAM Error Address Pointer (32b)
				 *
				 * 31:7  address
				 * 6:1   reserved
				 * 0     Error channel 0/1
				 */
#define I3000_DEAP_GRAIN 		(1 << 7)

/*
 * Helper functions to decode the DEAP/EDEAP hardware registers.
 *
 * The type promotion here is deliberate; we're deriving an
 * unsigned long pfn and offset from hardware regs which are u8/u32.
 */

static inline unsigned long deap_pfn(u8 edeap, u32 deap)
{
	deap >>= PAGE_SHIFT;
	deap |= (edeap & 1) << (32 - PAGE_SHIFT);
	return deap;
}

static inline unsigned long deap_offset(u32 deap)
{
	return deap & ~(I3000_DEAP_GRAIN - 1) & ~PAGE_MASK;
}

static inline int deap_channel(u32 deap)
{
	return deap & 1;
}

#define I3000_DERRSYN	0x5c	/* DRAM Error Syndrome (8b)
				 *
				 *  7:0  DRAM ECC Syndrome
				 */

#define I3000_ERRSTS	0xc8	/* Error Status Register (16b)
				 *
				 * 15:12 reserved
				 * 11    MCH Thermal Sensor Event
				 *         for SMI/SCI/SERR
				 * 10    reserved
				 *  9    LOCK to non-DRAM Memory Flag (LCKF)
				 *  8    Received Refresh Timeout Flag (RRTOF)
				 *  7:2  reserved
				 *  1    Multi-bit DRAM ECC Error Flag (DMERR)
				 *  0    Single-bit DRAM ECC Error Flag (DSERR)
				 */
#define I3000_ERRSTS_BITS	0x0b03	/* bits which indicate errors */
#define I3000_ERRSTS_UE		0x0002
#define I3000_ERRSTS_CE		0x0001

#define I3000_ERRCMD	0xca	/* Error Command (16b)
				 *
				 * 15:12 reserved
				 * 11    SERR on MCH Thermal Sensor Event
				 *         (TSESERR)
				 * 10    reserved
				 *  9    SERR on LOCK to non-DRAM Memory
				 *         (LCKERR)
				 *  8    SERR on DRAM Refresh Timeout
				 *         (DRTOERR)
				 *  7:2  reserved
				 *  1    SERR Multi-Bit DRAM ECC Error
				 *         (DMERR)
				 *  0    SERR on Single-Bit ECC Error
				 *         (DSERR)
				 */

/* Intel  MMIO register space - device 0 function 0 - MMR space */

#define I3000_DRB_SHIFT 25	/* 32MiB grain */

#define I3000_C0DRB	0x100	/* Channel 0 DRAM Rank Boundary (8b x 4)
				 *
				 * 7:0   Channel 0 DRAM Rank Boundary Address
				 */
#define I3000_C1DRB	0x180	/* Channel 1 DRAM Rank Boundary (8b x 4)
				 *
				 * 7:0   Channel 1 DRAM Rank Boundary Address
				 */

#define I3000_C0DRA	0x108	/* Channel 0 DRAM Rank Attribute (8b x 2)
				 *
				 * 7     reserved
				 * 6:4   DRAM odd Rank Attribute
				 * 3     reserved
				 * 2:0   DRAM even Rank Attribute
				 *
				 * Each attribute defines the page
				 * size of the corresponding rank:
				 *     000: unpopulated
				 *     001: reserved
				 *     010: 4 KB
				 *     011: 8 KB
				 *     100: 16 KB
				 *     Others: reserved
				 */
#define I3000_C1DRA	0x188	/* Channel 1 DRAM Rank Attribute (8b x 2) */

static inline unsigned char odd_rank_attrib(unsigned char dra)
{
	return (dra & 0x70) >> 4;
}

static inline unsigned char even_rank_attrib(unsigned char dra)
{
	return dra & 0x07;
}

#define I3000_C0DRC0	0x120	/* DRAM Controller Mode 0 (32b)
				 *
				 * 31:30 reserved
				 * 29    Initialization Complete (IC)
				 * 28:11 reserved
				 * 10:8  Refresh Mode Select (RMS)
				 * 7     reserved
				 * 6:4   Mode Select (SMS)
				 * 3:2   reserved
				 * 1:0   DRAM Type (DT)
				 */

#define I3000_C0DRC1	0x124	/* DRAM Controller Mode 1 (32b)
				 *
				 * 31    Enhanced Addressing Enable (ENHADE)
				 * 30:0  reserved
				 */

enum i3000p_chips {
	I3000 = 0,
};

struct i3000_dev_info {
	const char *ctl_name;
};

struct i3000_error_info {
	u16 errsts;
	u8 derrsyn;
	u8 edeap;
	u32 deap;
	u16 errsts2;
};

static const struct i3000_dev_info i3000_devs[] = {
	[I3000] = {
		.ctl_name = "i3000"},
};

static struct pci_dev *mci_pdev;
static int i3000_registered = 1;
static struct edac_pci_ctl_info *i3000_pci;

static void i3000_get_error_info(struct mem_ctl_info *mci,
				 struct i3000_error_info *info)
{
	struct pci_dev *pdev;

	pdev = to_pci_dev(mci->pdev);

	/*
	 * This is a mess because there is no atomic way to read all the
	 * registers at once and the registers can transition from CE being
	 * overwritten by UE.
	 */
	pci_read_config_word(pdev, I3000_ERRSTS, &info->errsts);
	if (!(info->errsts & I3000_ERRSTS_BITS))
		return;
	pci_read_config_byte(pdev, I3000_EDEAP, &info->edeap);
	pci_read_config_dword(pdev, I3000_DEAP, &info->deap);
	pci_read_config_byte(pdev, I3000_DERRSYN, &info->derrsyn);
	pci_read_config_word(pdev, I3000_ERRSTS, &info->errsts2);

	/*
	 * If the error is the same for both reads then the first set
	 * of reads is valid.  If there is a change then there is a CE
	 * with no info and the second set of reads is valid and
	 * should be UE info.
	 */
	if ((info->errsts ^ info->errsts2) & I3000_ERRSTS_BITS) {
		pci_read_config_byte(pdev, I3000_EDEAP, &info->edeap);
		pci_read_config_dword(pdev, I3000_DEAP, &info->deap);
		pci_read_config_byte(pdev, I3000_DERRSYN, &info->derrsyn);
	}

	/*
	 * Clear any error bits.
	 * (Yes, we really clear bits by writing 1 to them.)
	 */
	pci_write_bits16(pdev, I3000_ERRSTS, I3000_ERRSTS_BITS,
			 I3000_ERRSTS_BITS);
}

static int i3000_process_error_info(struct mem_ctl_info *mci,
				struct i3000_error_info *info,
				int handle_errors)
{
	int row, multi_chan, channel;
	unsigned long pfn, offset;

	multi_chan = mci->csrows[0]->nr_channels - 1;

	if (!(info->errsts & I3000_ERRSTS_BITS))
		return 0;

	if (!handle_errors)
		return 1;

	if ((info->errsts ^ info->errsts2) & I3000_ERRSTS_BITS) {
		edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1, 0, 0, 0,
				     -1, -1, -1,
				     "UE overwrote CE", "");
		info->errsts = info->errsts2;
	}

	pfn = deap_pfn(info->edeap, info->deap);
	offset = deap_offset(info->deap);
	channel = deap_channel(info->deap);

	row = edac_mc_find_csrow_by_page(mci, pfn);

	if (info->errsts & I3000_ERRSTS_UE)
		edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1,
				     pfn, offset, 0,
				     row, -1, -1,
				     "i3000 UE", "");
	else
		edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1,
				     pfn, offset, info->derrsyn,
				     row, multi_chan ? channel : 0, -1,
				     "i3000 CE", "");

	return 1;
}

static void i3000_check(struct mem_ctl_info *mci)
{
	struct i3000_error_info info;

	edac_dbg(1, "MC%d\n", mci->mc_idx);
	i3000_get_error_info(mci, &info);
	i3000_process_error_info(mci, &info, 1);
}

static int i3000_is_interleaved(const unsigned char *c0dra,
				const unsigned char *c1dra,
				const unsigned char *c0drb,
				const unsigned char *c1drb)
{
	int i;

	/*
	 * If the channels aren't populated identically then
	 * we're not interleaved.
	 */
	for (i = 0; i < I3000_RANKS_PER_CHANNEL / 2; i++)
		if (odd_rank_attrib(c0dra[i]) != odd_rank_attrib(c1dra[i]) ||
			even_rank_attrib(c0dra[i]) !=
						even_rank_attrib(c1dra[i]))
			return 0;

	/*
	 * If the rank boundaries for the two channels are different
	 * then we're not interleaved.
	 */
	for (i = 0; i < I3000_RANKS_PER_CHANNEL; i++)
		if (c0drb[i] != c1drb[i])
			return 0;

	return 1;
}

static int i3000_probe1(struct pci_dev *pdev, int dev_idx)
{
	int rc;
	int i, j;
	struct mem_ctl_info *mci = NULL;
	struct edac_mc_layer layers[2];
	unsigned long last_cumul_size, nr_pages;
	int interleaved, nr_channels;
	unsigned char dra[I3000_RANKS / 2], drb[I3000_RANKS];
	unsigned char *c0dra = dra, *c1dra = &dra[I3000_RANKS_PER_CHANNEL / 2];
	unsigned char *c0drb = drb, *c1drb = &drb[I3000_RANKS_PER_CHANNEL];
	unsigned long mchbar;
	void __iomem *window;

	edac_dbg(0, "MC:\n");

	pci_read_config_dword(pdev, I3000_MCHBAR, (u32 *) & mchbar);
	mchbar &= I3000_MCHBAR_MASK;
	window = ioremap_nocache(mchbar, I3000_MMR_WINDOW_SIZE);
	if (!window) {
		printk(KERN_ERR "i3000: cannot map mmio space at 0x%lx\n",
			mchbar);
		return -ENODEV;
	}

	c0dra[0] = readb(window + I3000_C0DRA + 0);	/* ranks 0,1 */
	c0dra[1] = readb(window + I3000_C0DRA + 1);	/* ranks 2,3 */
	c1dra[0] = readb(window + I3000_C1DRA + 0);	/* ranks 0,1 */
	c1dra[1] = readb(window + I3000_C1DRA + 1);	/* ranks 2,3 */

	for (i = 0; i < I3000_RANKS_PER_CHANNEL; i++) {
		c0drb[i] = readb(window + I3000_C0DRB + i);
		c1drb[i] = readb(window + I3000_C1DRB + i);
	}

	iounmap(window);

	/*
	 * Figure out how many channels we have.
	 *
	 * If we have what the datasheet calls "asymmetric channels"
	 * (essentially the same as what was called "virtual single
	 * channel mode" in the i82875) then it's a single channel as
	 * far as EDAC is concerned.
	 */
	interleaved = i3000_is_interleaved(c0dra, c1dra, c0drb, c1drb);
	nr_channels = interleaved ? 2 : 1;

	layers[0].type = EDAC_MC_LAYER_CHIP_SELECT;
	layers[0].size = I3000_RANKS / nr_channels;
	layers[0].is_virt_csrow = true;
	layers[1].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = nr_channels;
	layers[1].is_virt_csrow = false;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers, 0);
	if (!mci)
		return -ENOMEM;

	edac_dbg(3, "MC: init mci\n");

	mci->pdev = &pdev->dev;
	mci->mtype_cap = MEM_FLAG_DDR2;

	mci->edac_ctl_cap = EDAC_FLAG_SECDED;
	mci->edac_cap = EDAC_FLAG_SECDED;

	mci->mod_name = EDAC_MOD_STR;
	mci->mod_ver = I3000_REVISION;
	mci->ctl_name = i3000_devs[dev_idx].ctl_name;
	mci->dev_name = pci_name(pdev);
	mci->edac_check = i3000_check;
	mci->ctl_page_to_phys = NULL;

	/*
	 * The dram rank boundary (DRB) reg values are boundary addresses
	 * for each DRAM rank with a granularity of 32MB.  DRB regs are
	 * cumulative; the last one will contain the total memory
	 * contained in all ranks.
	 *
	 * If we're in interleaved mode then we're only walking through
	 * the ranks of controller 0, so we double all the values we see.
	 */
	for (last_cumul_size = i = 0; i < mci->nr_csrows; i++) {
		u8 value;
		u32 cumul_size;
		struct csrow_info *csrow = mci->csrows[i];

		value = drb[i];
		cumul_size = value << (I3000_DRB_SHIFT - PAGE_SHIFT);
		if (interleaved)
			cumul_size <<= 1;
		edac_dbg(3, "MC: (%d) cumul_size 0x%x\n", i, cumul_size);
		if (cumul_size == last_cumul_size)
			continue;

		csrow->first_page = last_cumul_size;
		csrow->last_page = cumul_size - 1;
		nr_pages = cumul_size - last_cumul_size;
		last_cumul_size = cumul_size;

		for (j = 0; j < nr_channels; j++) {
			struct dimm_info *dimm = csrow->channels[j]->dimm;

			dimm->nr_pages = nr_pages / nr_channels;
			dimm->grain = I3000_DEAP_GRAIN;
			dimm->mtype = MEM_DDR2;
			dimm->dtype = DEV_UNKNOWN;
			dimm->edac_mode = EDAC_UNKNOWN;
		}
	}

	/*
	 * Clear any error bits.
	 * (Yes, we really clear bits by writing 1 to them.)
	 */
	pci_write_bits16(pdev, I3000_ERRSTS, I3000_ERRSTS_BITS,
			 I3000_ERRSTS_BITS);

	rc = -ENODEV;
	if (edac_mc_add_mc(mci)) {
		edac_dbg(3, "MC: failed edac_mc_add_mc()\n");
		goto fail;
	}

	/* allocating generic PCI control info */
	i3000_pci = edac_pci_create_generic_ctl(&pdev->dev, EDAC_MOD_STR);
	if (!i3000_pci) {
		printk(KERN_WARNING
			"%s(): Unable to create PCI control\n",
			__func__);
		printk(KERN_WARNING
			"%s(): PCI error report via EDAC not setup\n",
			__func__);
	}

	/* get this far and it's successful */
	edac_dbg(3, "MC: success\n");
	return 0;

fail:
	if (mci)
		edac_mc_free(mci);

	return rc;
}

/* returns count (>= 0), or negative on error */
static int i3000_init_one(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int rc;

	edac_dbg(0, "MC:\n");

	if (pci_enable_device(pdev) < 0)
		return -EIO;

	rc = i3000_probe1(pdev, ent->driver_data);
	if (!mci_pdev)
		mci_pdev = pci_dev_get(pdev);

	return rc;
}

static void i3000_remove_one(struct pci_dev *pdev)
{
	struct mem_ctl_info *mci;

	edac_dbg(0, "\n");

	if (i3000_pci)
		edac_pci_release_generic_ctl(i3000_pci);

	mci = edac_mc_del_mc(&pdev->dev);
	if (!mci)
		return;

	edac_mc_free(mci);
}

static const struct pci_device_id i3000_pci_tbl[] = {
	{
	 PCI_VEND_DEV(INTEL, 3000_HB), PCI_ANY_ID, PCI_ANY_ID, 0, 0,
	 I3000},
	{
	 0,
	 }			/* 0 terminated list. */
};

MODULE_DEVICE_TABLE(pci, i3000_pci_tbl);

static struct pci_driver i3000_driver = {
	.name = EDAC_MOD_STR,
	.probe = i3000_init_one,
	.remove = i3000_remove_one,
	.id_table = i3000_pci_tbl,
};

static int __init