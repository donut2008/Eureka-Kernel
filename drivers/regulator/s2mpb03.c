].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = drc_chan + 1;
	layers[1].is_virt_csrow = false;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers, sizeof(*pvt));
	if (mci == NULL)
		return -ENOMEM;

	edac_dbg(3, "init mci\n");
	mci->mtype_cap = MEM_FLAG_RDDR;
	mci->edac_ctl_cap = EDAC_FLAG_NONE | EDAC_FLAG_SECDED |
		EDAC_FLAG_S4ECD4ED;
	/* FIXME - what if different memory types are in different csrows? */
	mci->mod_name = EDAC_MOD_STR;
	mci->mod_ver = E7XXX_REVISION;
	mci->pdev = &pdev->dev;
	edac_dbg(3, "init pvt\n");
	pvt = (struct e7xxx_pvt *)mci->pvt_info;
	pvt->dev_info = &e7xxx_devs[dev_idx];
	pvt->bridge_ck = pci_get_device(PCI_VENDOR_ID_INTEL,
					pvt->dev_info->err_dev, pvt->bridge_ck);

	if (!pvt->bridge_ck) {
		e7xxx_printk(KERN_ERR, "error reporting device not found:"
			"vendor %x device 0x%x (broken BIOS?)\n",
			PCI_VENDOR_ID_INTEL, e7xxx_devs[dev_idx].err_dev);
		goto fail0;
	}

	edac_dbg(3, "more mci init\n");
	mci->ctl_name = pvt->dev_info->ctl_name;
	mci->dev_name = pci_name(pdev);
	mci->edac_check = e7xxx_check;
	mci->ctl_page_to_phys = ctl_page_to_phys;
	e7xxx_init_csrows(mci, pdev, dev_idx, drc);
	mci->edac_cap |= EDAC_FLAG_NONE;
	edac_dbg(3, "tolm, remapbase, remaplimit\n");
	/* load the top of low memory, remap base, and remap limit vars */
	pci_read_config_word(pdev, E7XXX_TOLM, &pci_data);
	pvt->tolm = ((u32) pci_data) << 4;
	pci_read_config_word(pdev, E7XXX_REMAPBASE, &pci_data);
	pvt->remapbase = ((u32) pci_data) << 14;
	pci_read_config_word(pdev, E7XXX_REMAPLIMIT, &pci_data);
	pvt->remaplimit = ((u32) pci_data) << 14;
	e7xxx_printk(KERN_INFO,
		"tolm = %x, remapbase = %x, remaplimit = %x\n", pvt->tolm,
		pvt->remapbase, pvt->remaplimit);

	/* clear any pending errors, or initial state bits */
	e7xxx_get_error_info(mci, &discard);

	/* Here we assume that we will never see multiple instances of this
	 * type of memory controller.  The ID is therefore hardcoded to 0.
	 */
	if (edac_mc_add_mc(mci)) {
		edac_dbg(3, "failed edac_mc_add_mc()\n");
		goto fail1;
	}

	/* allocating generic PCI control info */
	e7xxx_pci = edac_pci_create_generic_ctl(&pdev->dev, EDAC_MOD_STR);
	if (!e7xxx_pci) {
		printk(KERN_WARNING
			"%s(): Unable to create PCI control\n",
			__func__);
		printk(KERN_WARNING
			"%s(): PCI error report via EDAC not setup\n",
			__func__);
	}

	/* get this far and it's successful */
	edac_dbg(3, "success\n");
	return 0;

fail1:
	pci_dev_put(pvt->bridge_ck);

fail0:
	edac_mc_free(mci);

	return -ENODEV;
}

/* returns count (>= 0), or negative on error */
static int e7xxx_init_one(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	edac_dbg(0, "\n");

	/* wake up and enable device */
	return pci_enable_device(pdev) ?
		-EIO : e7xxx_probe1(pdev, ent->driver_data);
}

static void e7xxx_remove_one(struct pci_dev *pdev)
{
	struct mem_ctl_info *mci;
	struct e7xxx_pvt *pvt;

	edac_dbg(0, "\n");

	if (e7xxx_pci)
		edac_pci_release_generic_ctl(e7xxx_pci);

	if ((mci = edac_mc_del_mc(&pdev->dev)) == NULL)
		return;

	pvt = (struct e7xxx_pvt *)mci->pvt_info;
	pci_dev_put(pvt->bridge_ck);
	edac_mc_free(mci);
}

static const struct pci_device_id e7xxx_pci_tbl[] = {
	{
	 PCI_VEND_DEV(INTEL, 7205_0), PCI_ANY_ID, PCI_ANY_ID, 0, 0,
	 E7205},
	{
	 PCI_VEND_DEV(INTEL, 7500_0), PCI_ANY_ID, PCI_ANY_ID, 0, 0,
	 E7500},
	{
	 PCI_VEND_DEV(INTEL, 7501_0), PCI_ANY_ID, PCI_ANY_ID, 0, 0,
	 E7501},
	{
	 PCI_VEND_DEV(INTEL, 7505_0), PCI_ANY_ID, PCI_ANY_ID, 0, 0,
	 E7505},
	{
	 0,
	 }			/* 0 terminated list. */
};

MODULE_DEVICE_TABLE(pci, e7xxx_pci_tbl);

static struct pci_driver e7xxx_driver = {
	.name = EDAC_MOD_STR,
	.probe = e7xxx_init_one,
	.remove = e7xxx_remove_one,
	.id_table = e7xxx_pci_tbl,
};

static int __init e7xxx_init(void)
{
       /* Ensure that the OPSTATE is set correctly for POLL or NMI */
       opstate_init();

	return pci_register_driver(&e7xxx_driver);
}

static void __exit e7xxx_exit(void)
{
	pci_unregister_driver(&e7xxx_driver);
}

module_init(e7xxx_init);
module_exit(e7xxx_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Networx (http://lnxi.com) Thayne Harbaugh et al\n"
		"Based on.work by Dan Hollis et al");
MODULE_DESCRIPTION("MC support for Intel e7xxx memory controllers");
module_param(edac_op_state, int, 0444);
MODULE_PARM_DESC(edac_op_state, "EDAC Error Reporting state: 0=Poll,1=NMI");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Defines, structures, APIs for edac_core module
 *
 * (C) 2007 Linux Networx (http://lnxi.com)
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * Written by Thayne Harbaugh
 * Based on work by Dan Hollis <goemon at anime dot net> and others.
 *	http://www.anime.net/~goemon/linux-ecc/
 *
 * NMI handling support added by
 *     Dave Peterson <dsp@llnl.gov> <dave_peterson@pobox.com>
 *
 * Refactored for multi-source files:
 *	Doug Thompson <norsk5@xmission.com>
 *
 */

#ifndef _EDAC_CORE_H_
#define _EDAC_CORE_H_

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/smp.h>
#include <linux/pci.h>
#include <linux/time.h>
#include <linux/nmi.h>
#include <linux/rcupdate.h>
#include <linux/completion.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/edac.h>

#define EDAC_DEVICE_NAME_LEN	31
#define EDAC_ATTRIB_VALUE_LEN	15

#if PAGE_SHIFT < 20
#define PAGES_TO_MiB(pages)	((pages) >> (20 - PAGE_SHIFT))
#define MiB_TO_PAGES(mb)	((mb) << (20 - PAGE_SHIFT))
#else				/* PAGE_SHIFT > 20 */
#define PAGES_TO_MiB(pages)	((pages) << (PAGE_SHIFT - 20))
#define MiB_TO_PAGES(mb)	((mb) >> (PAGE_SHIFT - 20))
#endif

#define edac_printk(level, prefix, fmt, arg...) \
	printk(level "EDAC " prefix ": " fmt, ##arg)

#define edac_mc_printk(mci, level, fmt, arg...) \
	printk(level "EDAC MC%d: " fmt, mci->mc_idx, ##arg)

#define edac_mc_chipset_printk(mci, level, prefix, fmt, arg...) \
	printk(level "EDAC " prefix " MC%d: " fmt, mci->mc_idx, ##arg)

#define edac_device_printk(ctl, level, fmt, arg...) \
	printk(level "EDAC DEVICE%d: " fmt, ctl->dev_idx, ##arg)

#define edac_pci_printk(ctl, level, fmt, arg...) \
	printk(level "EDAC PCI%d: " fmt, ctl->pci_idx, ##arg)

/* prefixes for edac_printk() and edac_mc_printk() */
#define EDAC_MC "MC"
#define EDAC_PCI "PCI"
#define EDAC_DEBUG "DEBUG"

extern const char * const edac_mem_types[];

#ifdef CONFIG_EDAC_DEBUG
extern int edac_debug_level;

#define edac_dbg(level, fmt, ...)					\
do {									\
	if (level <= edac_debug_level)					\
		edac_printk(KERN_DEBUG, EDAC_DEBUG,			\
			    "%s: " fmt, __func__, ##__VA_ARGS__);	\
} while (0)

#else				/* !CONFIG_EDAC_DEBUG */

#define edac_dbg(level, fmt, ...)					\
do {									\
	if (0)								\
		edac_printk(KERN_DEBUG, EDAC_DEBUG,			\
			    "%s: " fmt, __func__, ##__VA_ARGS__);	\
} while (0)

#endif				/* !CONFIG_EDAC_DEBUG */

#define PCI_VEND_DEV(vend, dev) PCI_VENDOR_ID_ ## vend, \
	PCI_DEVICE_ID_ ## vend ## _ ## dev

#define edac_dev_name(dev) (dev)->dev_name

#define to_mci(k) container_of(k, struct mem_ctl_info, dev)

/*
 * The following are the structures to provide for a generic
 * or abstract 'edac_device'. This set of structures and the
 * code that implements the APIs for the same, provide for
 * registering EDAC type devices which are NOT standard memory.
 *
 * CPU caches (L1 and L2)
 * DMA engines
 * Core CPU switches
 * Fabric switch units
 * PCIe interface controllers
 * other EDAC/ECC type devices that can be monitored for
 * errors, etc.
 *
 * It allows for a 2 level set of hierarchy. For example:
 *
 * cache could be composed of L1, L2 and L3 levels of cache.
 * Each CPU core would have its own L1 cache, while sharing
 * L2 and maybe L3 caches.
 *
 * View them arranged, via the sysfs presentation:
 * /sys/devices/system/edac/..
 *
 *	mc/		<existing memory device directory>
 *	cpu/cpu0/..	<L1 and L2 block directory>
 *		/L1-cache/ce_count
 *			 /ue_count
 *		/L2-cache/ce_count
 *			 /ue_count
 *	cpu/cpu1/..	<L1 and L2 block directory>
 *		/L1-cache/ce_count
 *			 /ue_count
 *		/L2-cache/ce_count
 *			 /ue_count
 *	...
 *
 *	the L1 and L2 directories would be "edac_device_block's"
 */

struct edac_device_counter {
	u32 ue_count;
	u32 ce_count;
};

/* forward reference */
struct edac_device_ctl_info;
struct edac_device_block;

/* edac_dev_sysfs_attribute structure
 *	used for driver sysfs attributes in mem_ctl_info
 *	for extra controls and attributes:
 *		like high level error Injection controls
 */
struct edac_dev_sysfs_attribute {
	struct attribute attr;
	ssize_t (*show)(struct edac_device_ctl_info *, char *);
	ssize_t (*store)(struct edac_device_ctl_info *, const char *, size_t);
};

/* edac_dev_sysfs_block_attribute structure
 *
 *	used in leaf 'block' nodes for adding controls/attributes
 *
 *	each block in each instance of the containing control structure
 *	can have an array of the following. The show and store functions
 *	will be filled in with the show/store function in the
 *	low level driver.
 *
 *	The 'value' field will be the actual value field used for
 *	counting
 */
struct edac_dev_sysfs_block_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kobject *, struct attribute *, char *);
	ssize_t (*store)(struct kobject *, struct attribute *,
			const char *, size_t);
	struct edac_device_block *block;

	unsigned int value;
};

/* device block control structure */
struct edac_device_block {
	struct edac_device_instance *instance;	/* Up Pointer */
	char name[EDAC_DEVICE_NAME_LEN + 1];

	struct edac_device_counter counters;	/* basic UE and CE counters */

	int nr_attribs;		/* how many attributes */

	/* this block's attributes, could be NULL */
	struct edac_dev_sysfs_block_attribute *block_attributes;

	/* edac sysfs device control */
	struct kobject kobj;
};

/* device instance control structure */
struct edac_device_instance {
	struct edac_device_ctl_info *ctl;	/* Up pointer */
	char name[EDAC_DEVICE_NAME_LEN + 4];

	struct edac_device_counter counters;	/* instance counters */

	u32 nr_blocks;		/* how many blocks */
	struct edac_device_block *blocks;	/* block array */

	/* edac sysfs device control */
	struct kobject kobj;
};


/*
 * Abstract edac_device control info structure
 *
 */
struct edac_device_ctl_info {
	/* for global list of edac_device_ctl_info structs */
	struct list_head link;

	struct module *owner;	/* Module owner of this control struct */

	int dev_idx;

	/* Per instance controls for this edac_device */
	int log_ue;		/* boolean for logging UEs */
	int log_ce;		/* boolean for logging CEs */
	int panic_on_ue;	/* boolean for panic'ing on an UE */
	unsigned poll_msec;	/* number of milliseconds to poll interval */
	unsigned long delay;	/* number of jiffies for poll_msec */

	/* Additional top controller level attributes, but specified
	 * by the low level driver.
	 *
	 * Set by the low level driver to provide attributes at the
	 * controller level, same level as 'ue_count' and 'ce_count' above.
	 * An array of structures, NULL terminated
	 *
	 * If attributes are desired, then set to array of attributes
	 * If no attributes are desired, leave NULL
	 */
	struct edac_dev_sysfs_attribute *sysfs_attributes;

	/* pointer to main 'edac' subsys in sysfs */
	struct bus_type *edac_subsys;

	/* the internal state of this controller instance */
	int op_state;
	/* work struct for this instance */
	struct delayed_work work;

	/* point