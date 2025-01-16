.limit = (u64) i5100_mir_limit(w) << 28;
	priv->mir[1].way[1] = i5100_mir_way1(w);
	priv->mir[1].way[0] = i5100_mir_way0(w);

	pci_read_config_word(pdev, I5100_AMIR_0, &w);
	priv->amir[0] = w;
	pci_read_config_word(pdev, I5100_AMIR_1, &w);
	priv->amir[1] = w;

	for (i = 0; i < I5100_CHANNELS; i++) {
		int j;

		for (j = 0; j < 5; j++) {
			int k;

			pci_read_config_dword(mms[i], I5100_DMIR + j * 4, &dw);

			priv->dmir[i][j].limit =
				(u64) i5100_dmir_limit(dw) << 28;
			for (k = 0; k < I5100_MAX_RANKS_PER_DIMM; k++)
				priv->dmir[i][j].rank[k] =
					i5100_dmir_rank(dw, k);
		}
	}

	i5100_init_mtr(mci);
}

static void i5100_init_csrows(struct mem_ctl_info *mci)
{
	int i;
	struct i5100_priv *priv = mci->pvt_info;

	for (i = 0; i < mci->tot_dimms; i++) {
		struct dimm_info *dimm;
		const unsigned long npages = i5100_npages(mci, i);
		const unsigned chan = i5100_csrow_to_chan(mci, i);
		const unsigned rank = i5100_csrow_to_rank(mci, i);

		if (!npages)
			continue;

		dimm = EDAC_DIMM_PTR(mci->layers, mci->dimms, mci->n_layers,
			       chan, rank, 0);

		dimm->nr_pages = npages;
		dimm->grain = 32;
		dimm->dtype = (priv->mtr[chan][rank].width == 4) ?
				DEV_X4 : DEV_X8;
		dimm->mtype = MEM_RDDR2;
		dimm->edac_mode = EDAC_SECDED;
		snprintf(dimm->label, sizeof(dimm->label), "DIMM%u",
			 i5100_rank_to_slot(mci, chan, rank));

		edac_dbg(2, "dimm channel %d, rank %d, size %ld\n",
			 chan, rank, (long)PAGES_TO_MiB(npages));
	}
}

/****************************************************************************
 *                       Error injection routines
 ****************************************************************************/

static void i5100_do_inject(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	u32 mask0;
	u16 mask1;

	/* MEM[1:0]EINJMSK0
	 * 31    - ADDRMATCHEN
	 * 29:28 - HLINESEL
	 *         00 Reserved
	 *         01 Lower half of cache line
	 *         10 Upper half of cache line
	 *         11 Both upper and lower parts of cache line
	 * 27    - EINJEN
	 * 25:19 - XORMASK1 for deviceptr1
	 * 9:5   - SEC2RAM for deviceptr2
	 * 4:0   - FIR2RAM for deviceptr1
	 */
	mask0 = ((priv->inject_hlinesel & 0x3) << 28) |
		I5100_MEMXEINJMSK0_EINJEN |
		((priv->inject_eccmask1 & 0xffff) << 10) |
		((priv->inject_deviceptr2 & 0x1f) << 5) |
		(priv->inject_deviceptr1 & 0x1f);

	/* MEM[1:0]EINJMSK1
	 * 15:0  - XORMASK2 for deviceptr2
	 */
	mask1 = priv->inject_eccmask2;

	if (priv->inject_channel == 0) {
		pci_write_config_dword(priv->mc, I5100_MEM0EINJMSK0, mask0);
		pci_write_config_word(priv->mc, I5100_MEM0EINJMSK1, mask1);
	} else {
		pci_write_config_dword(priv->mc, I5100_MEM1EINJMSK0, mask0);
		pci_write_config_word(priv->mc, I5100_MEM1EINJMSK1, mask1);
	}

	/* Error Injection Response Function
	 * Intel 5100 Memory Controller Hub Chipset (318378) datasheet
	 * hints about this register but carry no data about them. All
	 * data regarding device 19 is based on experimentation and the
	 * Intel 7300 Chipset Memory Controller Hub (318082) datasheet
	 * which appears to be accurate for the i5100 in this area.
	 *
	 * The injection code don't work without setting this register.
	 * The register needs to be flipped off then on else the hardware
	 * will only preform the first injection.
	 *
	 * Stop condition bits 7:4
	 * 1010 - Stop after one injection
	 * 1011 - Never stop injecting faults
	 *
	 * Start condition bits 3:0
	 * 1010 - Never start
	 * 1011 - Start immediately
	 */
	pci_write_config_byte(priv->einj, I5100_DINJ0, 0xaa);
	pci_write_config_byte(priv->einj, I5100_DINJ0, 0xab);
}

#define to_mci(k) container_of(k, struct mem_ctl_info, dev)
static ssize_t inject_enable_write(struct file *file, const char __user *data,
		size_t count, loff_t *ppos)
{
	struct device *dev = file->private_data;
	struct mem_ctl_info *mci = to_mci(dev);

	i5100_do_inject(mci);

	return count;
}

static const struct file_operations i5100_inject_enable_fops = {
	.open = simple_open,
	.write = inject_enable_write,
	.llseek = generic_file_llseek,
};

static int i5100_setup_debugfs(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;

	if (!i5100_debugfs)
		return -ENODEV;

	priv->debugfs = edac_debugfs_create_dir_at(mci->bus->name, i5100_debugfs);

	if (!priv->debugfs)
		return -ENOMEM;

	edac_debugfs_create_x8("inject_channel", S_IRUGO | S_IWUSR, priv->debugfs,
				&priv->inject_channel);
	edac_debugfs_create_x8("inject_hlinesel", S_IRUGO | S_IWUSR, priv->debugfs,
				&priv->inject_hlinesel);
	edac_debugfs_create_x8("inject_deviceptr1", S_IRUGO | S_IWUSR, priv->debugfs,
				&priv->inject_deviceptr1);
	edac_debugfs_create_x8("inject_deviceptr2", S_IRUGO | S_IWUSR, priv->debugfs,
				&priv->inject_deviceptr2);
	edac_debugfs_create_x16("inject_eccmask1", S_IRUGO | S_IWUSR, priv->debugfs,
				&priv->inject_eccmask1);
	edac_debugfs_create_x16("inject_eccmask2", S_IRUGO | S_IWUSR, priv->debugfs,
				&priv->inject_eccmask2);
	edac_debugfs_create_file("inject_enable", S_IWUSR, priv->debugfs,
				&mci->dev, &i5100_inject_enable_fops);

	return 0;

}

static int i5100_init_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rc;
	struct mem_ctl_info *mci;
	struct edac_mc_layer layers[2];
	struct i5100_priv *priv;
	struct pci_dev *ch0mm, *ch1mm, *einj;
	int ret = 0;
	u32 dw;
	int ranksperch;

	if (PCI_FUNC(pdev->devfn) != 1)
		return -ENODEV;

	rc = pci_enable_device(pdev);
	if (rc < 0) {
		ret = rc;
		goto bail;
	}

	/* ECC enabled? */
	pci_read_config_dword(pdev, I5100_MC, &dw);
	if (!i5100_mc_errdeten(dw)) {
		printk(KERN_INFO "i5100_edac: ECC not enabled.\n");
		ret = -ENODEV;
		goto bail_pdev;
	}

	/* figure out how many ranks, from strapped state of 48GB_Mode input */
	pci_read_config_dword(pdev, I5100_MS, &dw);
	ranksperch = !!(dw & (1 << 8)) * 2 + 4;

	/* enable error reporting... */
	pci_read_config_dword(pdev, I5100_EMASK_MEM, &dw);
	dw &= ~I5100_FERR_NF_MEM_ANY_MASK;
	pci_write_config_dword(pdev, I5100_EMASK_MEM, dw);

	/* device 21, func 0, Channel 0 Memory Map, Error Flag/Mask, etc... */
	ch0mm = pci_get_device_func(PCI_VENDOR_ID_INTEL,
				    PCI_DEVICE_ID_INTEL_5100_21, 0);
	if (!ch0mm) {
		ret = -ENODEV;
		goto bail_pdev;
	}

	rc = pci_enable_device(ch0mm);
	if (rc < 0) {
		ret = rc;
		goto bail_ch0;
	}

	/* device 22, func 0, Channel 1 Memory Map, Error Flag/Mask, etc... */
	ch1mm = pci_get_device_func(PCI_VENDOR_ID_INTEL,
				    PCI_DEVICE_ID_INTEL_5100_22, 0);
	if (!ch1mm) {
		ret = -ENODEV;
		goto bail_disable_ch0;
	}

	rc = pci_enable_device(ch1mm);
	if (rc < 0) {
		ret = rc;
		goto bail_ch1;
	}

	layers[0].type = EDAC_MC_LAYER_CHANNEL;
	layers[0].size = 2;
	layers[0].is_virt_csrow = false;
	layers[1].type = EDAC_MC_LAYER_SLOT;
	layers[1].size = ranksperch;
	layers[1].is_virt_csrow = true;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers,
			    sizeof(*priv));
	if (!mci) {
		ret = -ENOMEM;
		goto bail_disable_ch1;
	}


	/* device 19, func 0, Error injection */
	einj = pci_get_device_func(PCI_VENDOR_ID_INTEL,
				    PCI_DEVICE_ID_INTEL_5100_19, 0);
	if (!einj) {
		ret = -ENODEV;
		goto bail_mc_free;
	}

	rc = pci_enable_device(einj);
	if (rc < 0) {
		ret = rc;
		goto bail_einj;
	}

	mci->pdev = &pdev->dev;

	priv = mci->pvt_info;
	priv->ranksperchan = ranksperch;
	priv->mc = pdev;
	priv->ch0mm = ch0mm;
	priv->ch1mm = ch1mm;
	priv->einj = einj;

	INIT_DELAYED_WORK(&(priv->i5100_scrubbing), i5100_refresh_scrubbing);

	/* If scrubbing was already enabled by the bios, start maintaining it */
	pci_read_config_dword(pdev, I5100_MC, &dw);
	if (i5100_mc_scrben(dw)) {
		priv->scrub_enable = 1;
		schedule_delayed_work(&(priv->i5100_scrubbing),
				      I5100_SCRUB_REFRESH_RATE);
	}

	i5100_init_dimm_layout(pdev, mci);
	i5100_init_interleaving(pdev, mci);

	mci->mtype_cap = MEM_FLAG_FB_DDR2;
	mci->edac_ctl_cap = EDAC_FLAG_SECDED;
	mci->edac_cap = EDAC_FLAG_SECDED;
	mci->mod_name = "i5100_edac.c";
	mci->mod_ver = "not versioned";
	mci->ctl_name = "i5100";
	mci->dev_name = pci_name(pdev);
	mci->ctl_page_to_phys = NULL;

	mci->edac_check = i5100_check_error;
	mci->set_sdram_scrub_rate = i5100_set_scrub_rate;
	mci->get_sdram_scrub_rate = i5100_get_scrub_rate;

	priv->inject_channel = 0;
	priv->inject_hlinesel = 0;
	priv->inject_deviceptr1 = 0;
	priv->inject_deviceptr2 = 0;
	priv->inject_eccmask1 = 0;
	priv->inject_eccmask2 = 0;

	i5100_init_csrows(mci);

	/* this strange construction seems to be in every driver, dunno why */
	switch (edac_op_state) {
	case EDAC_OPSTATE_POLL:
	case EDAC_OPSTATE_NMI:
		break;
	default:
		edac_op_state = EDAC_OPSTATE_POLL;
		break;
	}

	if (edac_mc_add_mc(mci)) {
		ret = -ENODEV;
		goto bail_scrub;
	}

	i5100_setup_debugfs(mci);

	return ret;

bail_scrub:
	priv->scrub_enable = 0;
	cancel_delayed_work_sync(&(priv->i5100_scrubbing));
	pci_disable_device(einj);

bail_einj:
	pci_dev_put(einj);

bail_mc_free:
	edac_mc_free(mci);

bail_disable_ch1:
	pci_disable_device(ch1mm);

bail_ch1:
	pci_dev_put(ch1mm);

bail_disable_ch0:
	pci_disable_device(ch0mm);

bail_ch0:
	pci_dev_put(ch0mm);

bail_pdev:
	pci_disable_device(pdev);

bail:
	return ret;
}

static void i5100_remove_one(struct pci_dev *pdev)
{
	struct mem_ctl_info *mci;
	struct i5100_priv *priv;

	mci = edac_mc_del_mc(&pdev->dev);

	if (!mci)
		return;

	priv = mci->pvt_info;

	edac_debugfs_remove_recursive(priv->debugfs);

	priv->scrub_enable = 0;
	cancel_delayed_work_sync(&(priv->i5100_scrubbing));

	pci_disable_device(pdev);
	pci_disable_device(priv->ch0mm);
	pci_disable_device(priv->ch1mm);
	pci_disable_device(priv->einj);
	pci_dev_put(priv->ch0mm);
	pci_dev_put(priv->ch1mm);
	pci_dev_put(priv->einj);

	edac_mc_free(mci);
}

static const struct pci_device_id i5100_pci_tbl[] = {
	/* Device 16, Function 0, Channel 0 Memory Map, Error Flag/Mask, ... */
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_5100_16) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, i5100_pci_tbl);

static struct pci_driver i5100_driver = {
	.name = KBUILD_BASENAME,
	.probe = i5100_init_one,
	.remove = i5100_remove_one,
	.id_table = i5100_pci_tbl,
};

static int __init i5100_init(void)
{
	int pci_rc;

	i5100_debugfs = edac_debugfs_create_dir_at("i5100_edac", NULL);

	pci_rc = pci_register_driver(&i5100_driver);
	return (pci_rc < 0) ? pci_rc : 0;
}

static void __exit i5100_exit(void)
{
	edac_debugfs_remove(i5100_debugfs);

	pci_unregister_driver(&i5100_driver);
}

module_init(i5100_init);
module_exit(i5100_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR
    ("Arthur Jones <ajones@riverbed.com>");
MODULE_DESCRIPTION("MC Driver for Intel I5100 memory controllers");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * Intel 5400 class Memory Controllers kernel module (Seaburg)
 *
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * Copyright (c) 2008 by:
 *	 Ben Woodard <woodard@redhat.com>
 *	 Mauro Carvalho Chehab
 *
 * Red Hat Inc. http://www.redhat.com
 *
 * Forked and adapted from the i5000_edac driver which was
 * written by Douglas Thompson Linux Networx <norsk5@xmission.com>
 *
 * This module is based on the following document:
 *
 * Intel 5400 Chipset Memory Controller Hub (MCH) - Datasheet
 * 	http://developer.intel.com/design/chipsets/datashts/313070.htm
 *
 * This Memory Controller manages DDR2 FB-DIMMs. It has 2 branches, each with
 * 2 channels operating in lockstep no-mirror mode. Each channel can have up to
 * 4 dimm's, each with up to 8GB.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/slab.h>
#include <linux/edac.h>
#include <linux/mmzone.h>

#include "edac_core.h"

/*
 * Alter this version for the I5400 module when modifications are made
 */
#define I5400_REVISION    " Ver: 1.0.0"

#define EDAC_MOD_STR      "i5400_edac"

#define i5400_printk(level, fmt, arg...) \
	edac_printk(level, "i5400", fmt, ##arg)

#define i5400_mc_printk(mci, level, fmt, arg...) \
	edac_mc_chipset_printk(mci, level, "i5400", fmt, ##arg)

/* Limits for i5400 */
#define MAX_BRANCHES		2
#define CHANNELS_PER_BRANCH	2
#define DIMMS_PER_CHANNEL	4
#define	MAX_CHANNELS		(MAX_BRANCHES * CHANNELS_PER_BRANCH)

/* Device 16,
 * Function 0: System Address
 * Function 1: Memory Branch Map, Control, Errors Register
 * Function 2: FSB Error Registers
 *
 * All 3 functions of Device 16 (0,1,2) share the SAME DID and
 * uses PCI_DEVICE_ID_INTEL_5400_ERR for device 16 (0,1,2),
 * PCI_DEVICE_ID_INTEL_5400_FBD0 and PCI_DEVICE_ID_INTEL_5400_FBD1
 * for device 21 (0,1).
 */

	/* OFFSETS for Function 0 */
#define		AMBASE			0x48 /* AMB Mem Mapped Reg Region Base */
#define		MAXCH			0x56 /* Max Channel Number */
#define		MAXDIMMPERCH		0x57 /* Max DIMM PER Channel Number */

	/* OFFSETS for Function 1 */
#define		TOLM			0x6C
#define		REDMEMB			0x7C
#define			REC_ECC_LOCATOR_ODD(x)	((x) & 0x3fe00) /* bits [17:9] indicate ODD, [8:0]  indicate EVEN */
#define		MIR0			0x80
#define		MIR1			0x84
#define		AMIR0			0x8c
#define		AMIR1			0x90

	/* Fatal error registers */
#define		FERR_FAT_FBD		0x98	/* also called as FERR_FAT_FB_DIMM at datasheet */
#define			FERR_FAT_FBDCHAN (3<<28)	/* channel index where the highest-order error occurred */

#define		NERR_FAT_FBD		0x9c
#define		FERR_NF_FBD		0xa0	/* also called as FERR_NFAT_FB_DIMM at datasheet */

	/* Non-fatal error register */
#define		NERR_NF_FBD		0xa4

	/* Enable error mask */
#define		EMASK_FBD		0xa8

#define		ERR0_FBD		0xac
#define		ERR1_FBD		0xb0
#define		ERR2_FBD		0xb4
#define		MCERR_FBD		0xb8

	/* No OFFSETS for Device 16 Function 2 */

/*
 * Device 21,
 * Function 0: Memory Map Branch 0
 *
 * Device 22,
 * Function 0: Memory Map Branch 1
 */

	/* OFFSETS for Function 0 */
#define AMBPRESENT_0	0x64
#define AMBPRESENT_1	0x66
#define MTR0		0x80
#define MTR1		0x82
#define MTR2		0x84
#define MTR3		0x86

	/* OFFSETS for Function 1 */
#define NRECFGLOG		0x74
#define RECFGLOG		0x78
#define NRECMEMA		0xbe
#define NRECMEMB		0xc0
#define NRECFB_DIMMA		0xc4
#define NRECFB_DIMMB		0xc8
#define NRECFB_DIMMC		0xcc
#define NRECFB_DIMMD		0xd0
#define NRECFB_DIMME		0xd4
#define NRECFB_DIMMF		0xd8
#define REDMEMA			0xdC
#define RECMEMA			0xf0
#define RECMEMB			0xf4
#define RECFB_DIMMA		0xf8
#define RECFB_DIMMB		0xec
#define RECFB_DIMMC		0xf0
#define RECFB_DIMMD		0xf4
#define RECFB_DIMME		0xf8
#define RECFB_DIMMF		0xfC

/*
 * Error indicator bits and masks
 * Error masks are according with Table 5-17 of i5400 datasheet
 */

enum error_mask {
	EMASK_M1  = 1<<0,  /* Memory Write error on non-redundant retry */
	EMASK_M2  = 1<<1,  /* Memory or FB-DIMM configuration CRC read error */
	EMASK_M3  = 1<<2,  /* Reserved */
	EMASK_M4  = 1<<3,  /* Uncorrectable Data ECC on Replay */
	EMASK_M5  = 1<<4,  /* Aliased Uncorrectable Non-Mirrored Demand Data ECC */
	EMASK_M6  = 1<<5,  /* Unsupported on i5400 */
	EMASK_M7  = 1<<6,  /* Aliased Uncorrectable Resilver- or Spare-Copy Data ECC */
	EMASK_M8  = 1<<7,  /* Aliased Uncorrectable Patrol Data ECC */
	EMASK_M9  = 1<<8,  /* Non-Aliased Uncorrectable Non-Mirrored Demand Data ECC */
	EMASK_M10 = 1<<9,  /* Unsupported on i5400 */
	EMASK_M11 = 1<<10, /* Non-Aliased Uncorrectable Resilver- or Spare-Copy Data ECC  */
	EMASK_M12 = 1<<11, /* Non-Aliased Uncorrectable Patrol Data ECC */
	EMASK_M13 = 1<<12, /* Memory Write error on first attempt */
	EMASK_M14 = 1<<13, /* FB-DIMM Configuration Write error on first attempt */
	EMASK_M15 = 1<<14, /* Memory or FB-DIMM configuration CRC read error */
	EMASK_M16 = 1<<15, /* Channel Failed-Over Occurred */
	EMASK_M17 = 1<<16, /* Correctable Non-Mirrored Demand Data ECC */
	EMASK_M18 = 1<<17, /* Unsupported on i5400 */
	EMASK_M19 = 1<<18, /* Correctable Resilver- or Spare-Copy Data ECC */
	EMASK_M20 = 1<<19, /* Correctable Patrol Data ECC */
	EMASK_M21 = 1<<20, /* FB-DIMM Northbound parity error on FB-DIMM Sync Status */
	EMASK_M22 = 1<<21, /* SPD protocol Error */
	EMASK_M23 = 1<<22, /* Non-Redundant Fast Reset Timeout */
	EMASK_M24 = 1<<23, /* Refresh error */
	EMASK_M25 = 1<<24, /* Memory Write error on redundant retry */
	EMASK_M26 = 1<<25, /* Redundant Fast Reset Timeout */
	EMASK_M27 = 1<<26, /* Correctable Counter Threshold Exceeded */
	EMASK_M28 = 1<<27, /* DIMM-Spare Copy Completed */
	EMASK_M29 = 1<<28, /* DIMM-Isolation Completed */
};

/*
 * Names to translate bit error into something useful
 */
static const char *error_name[] = {
	[0]  = "Memory Write error on non-redundant retry",
	[1]  = "Memory or FB-DIMM configuration CRC read error",
	/* Reserved */
	[3]  = "Uncorrectable Data ECC on Replay",
	[4]  = "Aliased Uncorrectable Non-Mirrored Demand Data ECC",
	/* M6 Unsupported on i5400 */
	[6]  = "Aliased Uncorrectable Resilver- or Spare-Copy Data ECC",
	[7]  = "Aliased Uncorrectable Patrol Data ECC",
	[8]  = "Non-Aliased Uncorrectable Non-Mirrored Demand Data ECC",
	/* M10 Unsupported on i5400 */
	[10] = "Non-Aliased Uncorrectable Resilver- or Spare-Copy Data ECC",
	[11] = "Non-Aliased Uncorrectable Patrol Data ECC",
	[12] = "Memory Write error on first attempt",
	[13] = "FB-DIMM Configuration Write error on first attempt",
	[14] = "Memory or FB-DIMM configuration CRC read error",
	[15] = "Channel Failed-Over Occurred",
	[16] = "Correctable Non-Mirrored Demand Data ECC",
	/* M18 Unsupported on i5400 */
	[18] = "Correctable Resilver- or Spare-Copy Data ECC",
	[19] = "Correctable Patrol Data ECC",
	[20] = "FB-DIMM Northbound parity error on FB-DIMM Sync Status",
	[21] = "SPD protocol Error",
	[22] = "Non-Redundant Fast Reset Timeout",
	[23] = "Refresh error",
	[24] = "Memory Write error on redundant retry",
	[25] = "Redundant Fast Reset Timeout",
	[26] = "Correctable Counter Threshold Exceeded",
	[27] = "DIMM-Spare Copy Completed",
	[28] = "DIMM-Isolation Completed",
};

/* Fatal errors */
#define ERROR_FAT_MASK		(EMASK_M1 | \
				 EMASK_M2 | \
				 EMASK_M23)

/* Correctable errors */
#define ERROR_NF_CORRECTABLE	(EMASK_M27 | \
				 EMASK_M20 | \
				 EMASK_M19 | \
				 EMASK_M18 | \
				 EMASK_M17 | \
				 EMASK_M16)
#define ERROR_NF_DIMM_SPARE	(EMASK_M29 | \
				 EMASK_M28)
#define ERROR_NF_SPD_PROTOCOL	(EMASK_M22)
#define ERROR_NF_NORTH_CRC	(EMASK_M21)

/* Recoverable errors */
#define ERROR_NF_RECOVERABLE	(EMASK_M26 | \
				 EMASK_M25 | \
				 EMASK_M24 | \
				 EMASK_M15 | \
				 EMASK_M14 | \
				 EMASK_M13 | \
				 EMASK_M12 | \
				 EMASK_M11 | \
				 EMASK_M9  | \
				 EMASK_M8  | \
				 EMASK_M7  | \
				 EMASK_M5)

/* uncorrectable errors */
#define ERROR_NF_UNCORRECTABLE	(EMASK_M4)

/* mask to all non-fatal errors */
#define ERROR_NF_MASK		(ERROR_NF_CORRECTABLE   | \
				 ERROR_NF_UNCORRECTABLE | \
				 ERROR_NF_RECOVERABLE   | \
				 ERROR_NF_DIMM_SPARE    | \
				 ERROR_NF_SPD_PROTOCOL  | \
				 ERROR_NF_NORTH_CRC)

/*
 * Define error masks for the several registers
 */

/* Enable all fatal and non fatal errors */
#define ENABLE_EMASK_ALL	(ERROR_FAT_MASK | ERROR_NF_MASK)

/* mask for fatal error registers */
#define FERR_FAT_MASK ERROR_FAT_MASK

/* masks for non-fatal error register */
static inline int to_nf_mask(unsigned int mask)
{
	return (mask & EMASK_M29) | (mask >> 3);
};

static inline int from_nf_ferr(unsigned int mask)
{
	return (mask & EMASK_M29) |		/* Bit 28 */
	       (mask & ((1 << 28) - 1) << 3);	/* Bits 0 to 27 */
};

#define FERR_NF_MASK		to_nf_mask(ERROR_NF_MASK)
#define FERR_NF_CORRECTABLE	to_nf_mask(ERROR_NF_CORRECTABLE)
#define FERR_NF_DIMM_SPARE	to_nf_mask(ERROR_NF_DIMM_SPARE)
#define FERR_NF_SPD_PROTOCOL	to_nf_mask(ERROR_NF_SPD_PROTOCOL)
#define FERR_NF_NORTH_CRC	to_nf_mask(ERROR_NF_NORTH_CRC)
#define FERR_NF_RECOVERABLE	to_nf_mask(ERROR_NF_RECOVERABLE)
#define FERR_NF_UNCORRECTABLE	to_nf_mask(ERROR_NF_UNCORRECTABLE)

/* Defines to extract the vaious fields from the
 *	MTRx - Memory Technology Registers
 */
#define MTR_DIMMS_PRESENT(mtr)		((mtr) & (1 << 10))
#define MTR_DIMMS_ETHROTTLE(mtr)	((mtr) & (1 << 9))
#define MTR_DRAM_WIDTH(mtr)		(((mtr) & (1 << 8)) ? 8 : 4)
#define MTR_DRAM_BANKS(mtr)		(((mtr) & (1 << 6)) ? 8 : 4)
#define MTR_DRAM_BANKS_ADDR_BITS(mtr)	((MTR_DRAM_BANKS(mtr) == 8) ? 3 : 2)
#define MTR_DIMM_RANK(mtr)		(((mtr) >> 5) & 0x1)
#define MTR_DIMM_RANK_ADDR_BITS(mtr)	(MTR_DIMM_RANK(mtr) ? 2 : 1)
#define MTR_DIMM_ROWS(mtr)		(((mtr) >> 2) & 0x3)
#define MTR_DIMM_ROWS_ADDR_BITS(mtr)	(MTR_DIMM_ROWS(mtr) + 13)
#define MTR_DIMM_COLS(mtr)		((mtr) & 0x3)
#define MTR_DIMM_COLS_ADDR_BITS(mtr)	(MTR_DIMM_COLS(mtr) + 10)

/* This applies to FERR_NF_FB-DIMM as well as FERR_FAT_FB-DIMM */
static inline int extract_fbdchan_indx(u32 x)
{
	return (x>>28) & 0x3;
}

/* Device name and register DID (Device ID) */
struct i5400_dev_info {
	const char *ctl_name;	/* name for this device */
	u16 fsb_mapping_errors;	/* DID for the branchmap,control */
};

/* Table of devices attributes supported by this driver */
static const struct i5400_dev_info i5400_devs[] = {
	{
		.ctl_name = "I5400",
		.fsb_mapping_errors = PCI_DEVICE_ID_INTEL_5400_ERR,
	},
};

struct i5400_dimm_info {
	int megabytes;		/* size, 0 means not present  */
};

/* driver private data structure */
struct i5400_pvt {
	struct pci_dev *system_address;		/* 16.0 */
	struct pci_dev *branchmap_werrors;	/* 16.1 */
	struct pci_dev *fsb_error_regs;		/* 16.2 */
	struct pci_dev *branch_0;		/* 21.0 */
	struct pci_dev *branch_1;		/* 22.0 */

	u16 tolm;				/* top of low memory */
	union {
		u64 ambase;				/* AMB BAR */
		struct {
			u32 ambase_bottom;
			u32 ambase_top;
		} u __packed;
	};

	u16 mir0, mir1;

	u16 b0_mtr[DIMMS_PER_CHANNEL];	/* Memory Technlogy Reg */
	u16 b0_ambpresent0;			/* Branch 0, Channel 0 */
	u16 b0_ambpresent1;			/* Brnach 0, Channel 1 */

	u16 b1_mtr[DIMMS_PER_CHANNEL];	/* Memory Technlogy Reg */
	u16 b1_ambpresent0;			/* Branch 1, Channel 8 */
	u16 b1_ambpresent1;			/* Branch 1, Channel 1 */

	/* DIMM information matrix, allocating architecture maximums */
	struct i5400_dimm_info dimm_info[DIMMS_PER_CHANNEL][MAX_CHANNELS];

	/* Actual values for this controller */
	int maxch;				/* Max channels */
	int maxdimmperch;			/* Max DIMMs per channel */
};

/* I5400 MCH error information retrieved from Hardware */
struct i5400_error_info {
	/* These registers are always read from the MC */
	u32 ferr_fat_fbd;	/* First Errors Fatal */
	u32 nerr_fat_fbd;	/* Next Errors Fatal */
	u32 ferr_nf_fbd;	/* First Errors Non-Fatal */
	u32 nerr_nf_fbd;	/* Next Errors Non-Fatal */

	/* These registers are input ONLY if there was a Recoverable Error */
	u32 redmemb;		/* Recoverable Mem Data Error log B */
	u16 recmema;		/* Recoverable Mem Error log A */
	u32 recmemb;		/* Recoverable Mem Error log B */

	/* These registers are input ONLY if there was a Non-Rec Error */
	u16 nrecmema;		/* Non-Recoverable Mem log A */
	u32 nrecmemb;		/* Non-Recoverable Mem log B */

};

/* note that nrec_rdwr changed from NRECMEMA to NRECMEMB between the 5000 and
   5400 better to use an inline function than a macro in this case */
static inline int nrec_bank(struct i5400_error_info *info)
{
	return ((info->nrecmema) >> 12) & 0x7;
}
static inline int nrec_rank(struct i5400_error_info *info)
{
	return ((info->nrecmema) >> 8) & 0xf;
}
static inline int nrec_buf_id(struct i5400_error_info *info)
{
	return ((info->nrecmema)) & 0xff;
}
static inline int nrec_rdwr(struct i5400_error_info *info)
{
	return (info->nrecmemb) >> 31;
}
/* This applies to both NREC and REC string so it can be used with nrec_rdwr
   and rec_rdwr */
static inline const char *rdwr_str(int rdwr)
{
	return rdwr ? "Write" : "Read";
}
static inline int nrec_cas(struct i5400_error_info *info)
{
	return ((info->nrecmemb) >> 16) & 0x1fff;
}
static inline int nrec_ras(struct i5400_error_info *info)
{
	return (info->nrecmemb) & 0xffff;
}
static inline int rec_bank(struct i5400_error_info *info)
{
	return ((info->recmema) >> 12) & 0x7;
}
static inline int rec_rank(struct i5400_error_info *info)
{
	return ((info->recmema) >> 8) & 0xf;
}
static inline int rec_rdwr(struct i5400_error_info *info)
{
	return (info->recmemb) >> 31;
}
static inline int rec_cas(struct i5400_error_info *info)
{
	return ((info->recmemb) >> 16) & 0x1fff;
}
static inline int rec_ras(struct i5400_error_info *info)
{
	return (info->recmemb) & 0xffff;
}

static struct edac_pci_ctl_info *i5400_pci;

/*
 *	i5400_get_error_info	Retrieve the hardware error information from
 *				the hardware and cache it in the 'info'
 *				structure
 */
static void i5400_get_error_info(struct mem_ctl_info *mci,
				 struct i5400_error_info *info)
{
	struct i5400_pvt *pvt;
	u32 value;

	pvt = mci->pvt_info;

	/* read in the 1st FATAL error register */
	pci_read_config_dword(pvt->branchmap_werrors, FERR_FAT_FBD, &value);

	/* Mask only the bits that the doc says are valid
	 */
	value &= (FERR_FAT_FBDCHAN | FERR_FAT_MASK);

	/* If there is an error, then read in the
	   NEXT FATAL error register and the Memory Error Log Register A
	 */
	if (value & FERR_FAT_MASK) {
		info->ferr_fat_fbd = value;

		/* harvest the various error data we need */
		pci_read_config_dword(pvt->branchmap_werrors,
				NERR_FAT_FBD, &info->nerr_fat_fbd);
		pci_read_config_word(pvt->branchmap_werrors,
				NRECMEMA, &info->nrecmema);
		pci_read_config_dword(pvt->branchmap_werrors,
				NRECMEMB, &info->nrecmemb);

		/* Clear the error bits, by writing them back */
		pci_write_config_dword(pvt->branchmap_werrors,
				FERR_FAT_FBD, value);
	} else {
		info->ferr_fat_fbd = 0;
		info->nerr_fat_fbd = 0;
		info->nrecmema = 0;
		info->nrecmemb = 0;
	}

	/* read in the 1st NON-FATAL error register */
	pci_read_config_dword(pvt->branchmap_werrors, FERR_NF_FBD, &value);

	/* If there is an error, then read in the 1st NON-FATAL error
	 * register as well */
	if (value & FERR_NF_MASK) {
		info->ferr_nf_fbd = value;

		/* harvest the various error data we need */
		pci_read_config_dword(pvt->branchmap_werrors,
				NERR_NF_FBD, &info->nerr_nf_fbd);
		pci_read_config_word(pvt->branchmap_werrors,
				RECMEMA, &info->recmema);
		pci_read_config_dword(pvt->branchmap_werrors,
				RECMEMB, &info->recmemb);
		pci_read_config_dword(pvt->branchmap_werrors,
				REDMEMB, &info->redmemb);

		/* Clear the error bits, by writing them back */
		pci_write_config_dword(pvt->branchmap_werrors,
				FERR_NF_FBD, value);
	} else {
		info->ferr_nf_fbd = 0;
		info->nerr_nf_fbd = 0;
		info->recmema = 0;
		info->recmemb = 0;
		info->redmemb = 0;
	}
}

/*
 * i5400_proccess_non_recoverable_info(struct mem_ctl_info *mci,
 * 					struct i5400_error_info *info,
 * 					int handle_errors);
 *
 *	handle the Intel FATAL and unrecoverable errors, if any
 */
static void i5400_proccess_non_recoverable_info(struct mem_ctl_info *mci,
				    struct i5400_error_info *info,
				    unsigned long allErrors)
{
	char msg[EDAC_MC_LABEL_LEN + 1 + 90 + 80];
	int branch;
	int channel;
	int bank;
	int buf_id;
	int rank;
	int rdwr;
	int ras, cas;
	int errnum;
	char *type = NULL;
	enum hw_event_mc_err_type tp_event = HW_EVENT_ERR_UNCORRECTED;

	if (!allErrors)
		return;		/* if no error, return now */

	if (allErrors &  ERROR_FAT_MASK) {
		type = "FATAL";
		tp_event = HW_EVENT_ERR_FATAL;
	} else if (allErrors & FERR_NF_UNCORRECTABLE)
		type = "NON-FATAL uncorrected";
	else
		type = "NON-FATAL recoverable";

	/* ONLY ONE of the possible error bits will be set, as per the docs */

	branch = extract_fbdchan_indx(info->ferr_fat_fbd);
	channel = branch;

	/* Use the NON-Recoverable macros to extract data */
	bank = nrec_bank(info);
	rank = nrec_rank(info);
	buf_id = nrec_buf_id(info);
	rdwr = nrec_rdwr(info);
	ras = nrec_ras(info);
	cas = nrec_cas(info);

	edac_dbg(0, "\t\tDIMM= %d  Channels= %d,%d  (Branch= %d DRAM Bank= %d Buffer ID = %d rdwr= %s ras= %d cas= %d)\n",
		 rank, channel, channel + 1, branch >> 1, bank,
		 buf_id, rdwr_str(rdwr), ras, cas);

	/* Only 1 bit will be on */
	errnum = find_first_bit(&allErrors, ARRAY_SIZE(error_name));

	/* Form out message */
	snprintf(msg, sizeof(msg),
		 "Bank=%d Buffer ID = %d RAS=%d CAS=%d Err=0x%lx (%s)",
		 bank, buf_id, ras, cas, allErrors, error_name[errnum]);

	edac_mc_handle_error(tp_event, mci, 1, 0, 0, 0,
			     branch >> 1, -1, rank,
			     rdwr ? "Write error" : "Read error",
			     msg);
}

/*
 * i5400_process_fatal_error_info(struct mem_ctl_info *mci,
 * 				struct i5400_error_info *info,
 * 				int handle_errors);
 *
 *	handle the Intel NON-FATAL errors, if any
 */
static void i5400_process_nonfatal_error_info(struct mem_ctl_info *mci,
					struct i5400_error_info *info)
{
	char msg[EDAC_MC_LABEL_LEN + 1 + 90 + 80];
	unsigned long allErrors;
	int branch;
	int channel;
	int bank;
	int rank;
	int rdwr;
	int ras, cas;
	int errnum;

	/* mask off the Error bits that are possible */
	allErrors = from_nf_ferr(info->ferr_nf_fbd & FERR_NF_MASK);
	if (!allErrors)
		return;		/* if no error, return now */

	/* ONLY ONE of the possible error bits will be set, as per the docs */

	if (allErrors & (ERROR_NF_UNCORRECTABLE | ERROR_NF_RECOVERABLE)) {
		i5400_proccess_non_recoverable_info(mci, info, allErrors);
		return;
	}

	/* Correctable errors */
	if (allErrors & ERROR_NF_CORRECTABLE) {
		edac_dbg(0, "\tCorrected bits= 0x%lx\n", allErrors);

		branch = extract_fbdchan_indx(info->ferr_nf_fbd);

		channel = 0;
		if (REC_ECC_LOCATOR_ODD(info->redmemb))
			channel = 1;

		/* Convert channel to be based from zero, instead of
		 * from branch base of 0 */
		channel += branch;

		bank = rec_bank(info);
		rank = rec_rank(info);
		rdwr = rec_rdwr(info);
		ras = rec_ras(info);
		cas = rec_cas(info);

		/* Only 1 bit will be on */
		errnum = find_first_bit(&allErrors, ARRAY_SIZE(error_name));

		edac_dbg(0, "\t\tDIMM= %d Channel= %d  (Branch %d DRAM Bank= %d rdwr= %s ras= %d cas= %d)\n",
			 rank, channel, branch >> 1, bank,
			 rdwr_str(rdwr), ras, cas);

		/* Form out message */
		snprintf(msg, sizeof(msg),
			 "Corrected error (Branch=%d DRAM-Bank=%d RDWR=%s "
			 "RAS=%d CAS=%d, CE Err=0x%lx (%s))",
			 branch >> 1, bank, rdwr_str(rdwr), ras, cas,
			 allErrors, error_name[errnum]);

		edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1, 0, 0, 0,
				     branch >> 1, channel % 2, rank,
				     rdwr ? "Write error" : "Read error",
				     msg);

		return;
	}

	/* Miscellaneous errors */
	errnum = find_first_bit(&allErrors, ARRAY_SIZE(error_name));

	branch = extract_fbdchan_indx(info->ferr_nf_fbd);

	i5400_mc_printk(mci, KERN_EMERG,
			"Non-Fatal misc error (Branch=%d Err=%#lx (%s))",
			branch >> 1, allErrors, error_name[errnum]);
}

/*
 *	i5400_process_error_info	Process the error info that is
 *	in the 'info' structure, previously retrieved from hardware
 */
static void i5400_process_error_info(struct mem_ctl_info *mci,
				struct i5400_error_info *info)
{	u32 allErrors;

	/* First handle any fatal errors that occurred */
	allErrors = (info->ferr_fat_fbd & FERR_FAT_MASK);
	i5400_proccess_non_recoverable_info(mci, info, allErrors);

	/* now handle any non-fatal errors that occurred */
	i5400_process_nonfatal_error_info(mci, info);
}

/*
 *	i5400_clear_error	Retrieve any error from the hardware
 *				but do NOT process that error.
 *				Used for 'clearing' out of previous errors
 *				Called by the Core module.
 */
static void i5400_clear_error(struct mem_ctl_info *mci)
{
	struct i5400_error_info info;

	i5400_get_error_info(mci, &info);
}

/*
 *	i5400_check_error	Retrieve and process errors reported by the
 *				hardware. Called by the Core module.
 */
static void i5400_check_error(struct mem_ctl_info *mci)
{
	struct i5400_error_info info;
	edac_dbg(4, "MC%d\n", mci->mc_idx);
	i5400_get_error_info(mci, &info);
	i5400_process_error_info(mci, &info);
}

/*
 *	i5400_put_devices	'put' all the devices that we have
 *				reserved via 'get'
 */
static void i5400_put_devices(struct mem_ctl_info *mci)
{
	struct i5400_pvt *pvt;

	pvt = mci->pvt_info;

	/* Decrement usage count for devices */
	pci_dev_put(pvt->branch_1);
	pci_dev_put(pvt->branch_0);
	pci_dev_put(pvt->fsb_error_regs);
	pci_dev_put(pvt->branchmap_werrors);
}

/*
 *	i5400_get_devices	Find and perform 'get' operation on the MCH's
 *			device/functions we want to reference for this driver
 *
 *			Need to 'get' device 16 func 1 and func 2
 */
static int i5400_get_devices(struct mem_ctl_info *mci, int dev_idx)
{
	struct i5400_pvt *pvt;
	struct pci_dev *pdev;

	pvt = mci->pvt_info;
	pvt->branchmap_werrors = NULL;
	pvt->fsb_error_regs = NULL;
	pvt->branch_0 = NULL;
	pvt->branch_1 = NULL;

	/* Attempt to 'get' the MCH register we want */
	pdev = NULL;
	while (1) {
		pdev = pci_get_device(PCI_VENDOR_ID_INTEL,
				      PCI_DEVICE_ID_INTEL_5400_ERR, pdev);
		if (!pdev) {
			/* End of list, leave */
			i5400_printk(KERN_ERR,
				"'system address,Process Bus' "
				"device not found:"
				"vendor 0x%x device 0x%x ERR func 1 "
				"(broken BIOS?)\n",
				PCI_VENDOR_ID_INTEL,
				PCI_DEVICE_ID_INTEL_5400_ERR);
			return -ENODEV;
		}

		/* Store device 16 func 1 */
		if (PCI_FUNC(pdev->devfn) == 1)
			break;
	}
	pvt->branchmap_werrors = pdev;

	pdev = NULL;
	while (1) {
		pdev = pci_get_device(PCI_VENDOR_ID_INTEL,
				      PCI_DEVICE_ID_INTEL_5400_ERR, pdev);
		if (!pdev) {
			/* End of list, leave */
			i5400_printk(KERN_ERR,
				"'system address,Process Bus' "
				"device not found:"
				"vendor 0x%x device 0x%x ERR func 2 "
				"(broken BIOS?)\n",
				PCI_VENDOR_ID_INTEL,
				PCI_DEVICE_ID_INTEL_5400_ERR);

			pci_dev_put(pvt->branchmap_werrors);
			return -ENODEV;
		}

		/* Store device 16 func 2 */
		if (PCI_FUNC(pdev->devfn) == 2)
			break;
	}
	pvt->fsb_error_regs = pdev;

	edac_dbg(1, "System Address, processor bus- PCI Bus ID: %s  %x:%x\n",
		 pci_name(pvt->system_address),
		 pvt->system_address->vendor, pvt->system_address->device);
	edac_dbg(1, "Branchmap, control and errors - PCI Bus ID: %s  %x:%x\n",
		 pci_name(pvt->branchmap_werrors),
		 pvt->branchmap_werrors->vendor,
		 pvt->branchmap_werrors->device);
	edac_dbg(1, "FSB Error Regs - PCI Bus ID: %s  %x:%x\n",
		 pci_name(pvt->fsb_error_regs),
		 pvt->fsb_error_regs->vendor, pvt->fsb_error_regs->device);

	pvt->branch_0 = pci_get_device(PCI_VENDOR_ID_INTEL,
				       PCI_DEVICE_ID_INTEL_5400_FBD0, NULL);
	if (!pvt->branch_0) {
		i5400_printk(KERN_ERR,
			"MC: 'BRANCH 0' device not found:"
			"vendor 0x%x device 0x%x Func 0 (broken BIOS?)\n",
			PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_5400_FBD0);

		pci_dev_put(pvt->fsb_error_regs);
		pci_dev_put(pvt->branchmap_werrors);
		return -ENODEV;
	}

	/* If this device claims to have more than 2 channels then
	 * fetch Branch 1's information
	 */
	if (pvt->maxch < CHANNELS_PER_BRANCH)
		return 0;

	pvt->branch_1 = pci_get_device(PCI_VENDOR_ID_INTEL,
				       PCI_DEVICE_ID_INTEL_5400_FBD1, NULL);
	if (!pvt->branch_1) {
		i5400_printk(KERN_ERR,
			"MC: 'BRANCH 1' device not found:"
			"vendor 0x%x device 0x%x Func 0 "
			"(broken BIOS?)\n",
			PCI_VENDOR_ID_INTEL,
			PCI_DEVICE_ID_INTEL_5400_FBD1);

		pci_dev_put(pvt->branch_0);
		pci_dev_put(pvt->fsb_error_regs);
		pci_dev_put(pvt->branchmap_werrors);
		return -ENODEV;
	}

	return 0;
}

/*
 *	determine_amb_present
 *
 *		the information is contained in DIMMS_PER_CHANNEL different
 *		registers determining which of the DIMMS_PER_CHANNEL requires
 *              knowing which channel is in question
 *
 *	2 branches, each with 2 channels
 *		b0_ambpresent0 for channel '0'
 *		b0_ambpresent1 for channel '1'
 *		b1_ambpresent0 for channel '2'
 *		b1_ambpresent1 for channel '3'
 */
static int determine_amb_present_reg(struct i5400_pvt *pvt, int channel)
{
	int amb_present;

	if (channel < CHANNELS_PER_BRANCH) {
		if (channel & 0x1)
			amb_present = pvt->b0_ambpresent1;
		else
			amb_present = pvt->b0_ambpresent0;
	} else {
		if (channel & 0x1)
			amb_present = pvt->b1_ambpresent1;
		else
			amb_present = pvt->b1_ambpresent0;
	}

	return amb_present;
}

/*
 * determine_mtr(pvt, dimm, channel)
 *
 * return the proper MTR register as determine by the dimm and desired channel
 */
static int determine_mtr(struct i5400_pvt *pvt, int dimm, int channel)
{
	int mtr;
	int n;

	/* There is one MTR for each slot pair of FB-DIMMs,
	   Each slot pair may be at branch 0 or branch 1.
	 */
	n = dimm;

	if (n >= DIMMS_PER_CHANNEL) {
		edac_dbg(0, "ERROR: trying to access an invalid dimm: %d\n",
			 dimm);
		return 0;
	}

	if (channel < CHANNELS_PER_BRANCH)
		mtr = pvt->b0_mtr[n];
	else
		mtr = pvt->b1_mtr[n];

	return mtr;
}

/*
 */
static void decode_mtr(int slot_row, u16 mtr)
{
	int ans;

	ans = MTR_DIMMS_PRESENT(mtr);

	edac_dbg(2, "\tMTR%d=0x%x:  DIMMs are %sPresent\n",
		 slot_row, mtr, ans ? "" : "NOT ");
	if (!ans)
		return;

	edac_dbg(2, "\t\tWIDTH: x%d\n", MTR_DRAM_WIDTH(mtr));

	edac_dbg(2, "\t\tELECTRICAL THROTTLING is %s\n",
		 MTR_DIMMS_ETHROTTLE(mtr) ? "enabled" : "disabled");

	edac_dbg(2, "\t\tNUMBANK: %d bank(s)\n", MTR_DRAM_BANKS(mtr));
	edac_dbg(2, "\t\tNUMRANK: %s\n",
		 MTR_DIMM_RANK(mtr) ? "double" : "single");
	edac_dbg(2, "\t\tNUMROW: %s\n",
		 MTR_DIMM_ROWS(mtr) == 0 ? "8,192 - 13 rows" :
		 MTR_DIMM_ROWS(mtr) == 1 ? "16,384 - 14 rows" :
		 MTR_DIMM_ROWS(mtr) == 2 ? "32,768 - 15 rows" :
		 "65,536 - 16 rows");
	edac_dbg(2, "\t\tNUMCOL: %s\n",
		 MTR_DIMM_COLS(mtr) == 0 ? "1,024 - 10 columns" :
		 MTR_DIMM_COLS(mtr) == 1 ? "2,048 - 11 columns" :
		 MTR_DIMM_COLS(mtr) == 2 ? "4,096 - 12 columns" :
		 "reserved");
}

static void handle_channel(struct i5400_pvt *pvt, int dimm, int channel,
			struct i5400_dimm_info *dinfo)
{
	int mtr;
	int amb_present_reg;
	int addrBits;

	mtr = determine_mtr(pvt, dimm, channel);
	if (MTR_DIMMS_PRESENT(mtr)) {
		amb_present_reg = determine_amb_present_reg(pvt, channel);

		/* Determine if there is a DIMM present in this DIMM slot */
		if (amb_present_reg & (1 << dimm)) {
			/* Start with the number of bits for a Bank
			 * on the DRAM */
			addrBits = MTR_DRAM_BANKS_ADDR_BITS(mtr);
			/* Add thenumber of ROW bits */
			addrBits += MTR_DIMM_ROWS_ADDR_BITS(mtr);
			/* add the number of COLUMN bits */
			addrBits += MTR_DIMM_COLS_ADDR_BITS(mtr);
			/* add the number of RANK bits */
			addrBits += MTR_DIMM_RANK(mtr);

			addrBits += 6;	/* add 64 bits per DIMM */
			addrBits -= 20;	/* divide by 2^^20 */
			addrBits -= 3;	/* 8 bits per bytes */

			dinfo->megabytes = 1 << addrBits;
		}
	}
}

/*
 *	calculate_dimm_size
 *
 *	also will output a DIMM matrix map, if debug is enabled, for viewing
 *	how the DIMMs are populated
 */
static void calculate_dimm_size(struct i5400_pvt *pvt)
{
	struct i5400_dimm_info *dinfo;
	int dimm, max_dimms;
	char *p, *mem_buffer;
	int space, n;
	int channel, branch;

	/* ================= Generate some debug output ================= */
	space = PAGE_SIZE;
	mem_buffer = p = kmalloc(space, GFP_KERNEL);
	if (p == NULL) {
		i5400_printk(KERN_ERR, "MC: %s:%s() kmalloc() failed\n",
			__FILE__, __func__);
		return;
	}

	/* Scan all the actual DIMMS
	 * and calculate the information for each DIMM
	 * Start with the highest dimm first, to display it first
	 * and work toward the 0th dimm
	 */
	max_dimms = pvt->maxdimmperch;
	for (dimm = max_dimms - 1; dimm >= 0; dimm--) {

		/* on an odd dimm, first output a 'boundary' marker,
		 * then reset the message buffer  */
		if (dimm & 0x1) {
			n = snprintf(p, space, "---------------------------"
					"-------------------------------");
			p += n;
			space -= n;
			edac_dbg(2, "%s\n", mem_buffer);
			p = mem_buffer;
			space = PAGE_SIZE;
		}
		n = snprintf(p, space, "dimm %2d    ", dimm);
		p += n;
		space -= n;

		for (channel = 0; channel < pvt->maxch; channel++) {
			dinfo = &pvt->dimm_info[dimm][channel];
			handle_channel(pvt, dimm, channel, dinfo);
			n = snprintf(p, space, "%4d MB   | ", dinfo->megabytes);
			p += n;
			space -= n;
		}
		edac_dbg(2, "%s\n", mem_buffer);
		p = mem_buffer;
		space = PAGE_SIZE;
	}

	/* Output the last bottom 'boundary' marker */
	n = snprintf(p, space, "---------------------------"
			"-------------------------------");
	p += n;
	space -= n;
	edac_dbg(2, "%s\n", mem_buffer);
	p = mem_buffer;
	space = PAGE_SIZE;

	/* now output the 'channel' labels */
	n = snprintf(p, space, "           ");
	p += n;
	space -= n;
	for (channel = 0; channel < pvt->maxch; channel++) {
		n = snprintf(p, space, "channel %d | ", channel);
		p += n;
		space -= n;
	}

	space -= n;
	edac_dbg(2, "%s\n", mem_buffer);
	p = mem_buffer;
	space = PAGE_SIZE;

	n = snprintf(p, space, "           ");
	p += n;
	for (branch = 0; branch < MAX_BRANCHES; branch++) {
		n = snprintf(p, space, "       branch %d       | ", branch);
		p += n;
		space -= n;
	}

	/* output the last message and free buffer */
	edac_dbg(2, "%s\n", mem_buffer);
	kfree(mem_buffer);
}

/*
 *	i5400_get_mc_regs	read in the necessary registers and
 *				cache locally
 *
 *			Fills in the private data members
 */
static void i5400_get_mc_regs(struct mem_ctl_info *mci)
{
	struct i5400_pvt *pvt;
	u32 actual_tolm;
	u16 limit;
	int slot_row;
	int maxch;
	int maxdimmperch;
	int way0, way1;

	pvt = mci->pvt_info;

	pci_read_config_dword(pvt->system_address, AMBASE,
			&pvt->u.ambase_bottom);
	pci_read_config_dword(pvt->system_address, AMBASE + sizeof(u32),
			&pvt->u.ambase_top);

	maxdimmperch = pvt->maxdimmperch;
	maxch = pvt->maxch;

	edac_dbg(2, "AMBASE= 0x%lx  MAXCH= %d  MAX-DIMM-Per-CH= %d\n",
		 (long unsigned int)pvt->ambase, pvt->maxch, pvt->maxdimmperch);

	/* Get the Branch Map regs */
	pci_read_config_word(pvt->branchmap_werrors, TOLM, &pvt->tolm);
	pvt->tolm >>= 12;
	edac_dbg(2, "\nTOLM (number of 256M regions) =%u (0x%x)\n",
		 pvt->tolm, pvt->tolm);

	actual_tolm = (u32) ((1000l * pvt->tolm) >> (30 - 28));
	edac_dbg(2, "Actual TOLM byte addr=%u.%03u GB (0x%x)\n",
		 actual_tolm/1000, actual_tolm % 1000, pvt->tolm << 28);

	pci_read_config_word(pvt->branchmap_werrors, MIR0, &pvt->mir0);
	pci_read_config_word(pvt->branchmap_werrors, MIR1, &pvt->mir1);

	/* Get the MIR[0-1] regs */
	limit = (pvt->mir0 >> 4) & 0x0fff;
	way0 = pvt->mir0 & 0x1;
	way1 = pvt->mir0 & 0x2;
	edac_dbg(2, "MIR0: limit= 0x%x  WAY1= %u  WAY0= %x\n",
		 limit, way1, way0);
	limit = (pvt->mir1 >> 4) & 0xfff;
	way0 = pvt->mir1 & 0x1;
	way1 = pvt->mir1 & 0x2;
	edac_dbg(2, "MIR1: limit= 0x%x  WAY1= %u  WAY0= %x\n",
		 limit, way1, way0);

	/* Get the set of MTR[0-3] regs by each branch */
	for (slot_row = 0; slot_row < DIMMS_PER_CHANNEL; slot_row++) {
		int where = MTR0 + (slot_row * sizeof(u16));

		/* Branch 0 set of MTR registers */
		pci_read_config_word(pvt->branch_0, where,
				&pvt->b0_mtr[slot_row]);

		edac_dbg(2, "MTR%d where=0x%x B0 value=0x%x\n",
			 slot_row, where, pvt->b0_mtr[slot_row]);

		if (pvt->maxch 