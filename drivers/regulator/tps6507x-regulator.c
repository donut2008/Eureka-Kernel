Branch 1:\n");
		for (slot_row = 0; slot_row < DIMMS_PER_CHANNEL; slot_row++)
			decode_mtr(slot_row, pvt->b1_mtr[slot_row]);

		pci_read_config_word(pvt->branch_1, AMBPRESENT_0,
				&pvt->b1_ambpresent0);
		edac_dbg(2, "\t\tAMB-Branch 1-present0 0x%x:\n",
			 pvt->b1_ambpresent0);
		pci_read_config_word(pvt->branch_1, AMBPRESENT_1,
				&pvt->b1_ambpresent1);
		edac_dbg(2, "\t\tAMB-Branch 1-present1 0x%x:\n",
			 pvt->b1_ambpresent1);
	}

	/* Go and determine the size of each DIMM and place in an
	 * orderly matrix */
	calculate_dimm_size(pvt);
}

/*
 *	i5400_init_dimms	Initialize the 'dimms' table within
 *				the mci control	structure with the
 *				addressing of memory.
 *
 *	return:
 *		0	success
 *		1	no actual memory found on this MC
 */
static int i5400_init_dimms(struct mem_ctl_info *mci)
{
	struct i5400_pvt *pvt;
	struct dimm_info *dimm;
	int ndimms, channel_count;
	int max_dimms;
	int mtr;
	int size_mb;
	int  channel, slot;

	pvt = mci->pvt_info;

	channel_count = pvt->maxch;
	max_dimms = pvt->maxdimmperch;

	ndimms = 0;

	/*
	 * FIXME: remove  pvt->dimm_info[slot][channel] and use the 3
	 * layers here.
	 */
	for (channel = 0; channel < mci->layers[0].size * mci->layers[1].size;
	     channel++) {
		for (slot = 0; slot < mci->layers[2].size; slot++) {
			mtr = determine_mtr(pvt, slot, channel);

			/* if no DIMMS on this slot, continue */
			if (!MTR_DIMMS_PRESENT(mtr))
				continue;

			dimm = EDAC_DIMM_PTR(mci->layers, mci->dimms, mci->n_layers,
				       channel / 2, channel % 2, slot);

			size_mb =  pvt->dimm_info[slot][channel].megabytes;

			edac_dbg(2, "dimm (branch %d channel %d slot %d): %d.%03d GB\n",
				 channel / 2, channel % 2, slot,
				 size_mb / 1000, size_mb % 1000);

			dimm->nr_pages = size_mb << 8;
			dimm->grain = 8;
			dimm->dtype = MTR_DRAM_WIDTH(mtr) == 8 ?
				      DEV_X8 : DEV_X4;
			dimm->mtype = MEM_FB_DDR2;
			/*
			 * The eccc mechanism is SDDC (aka SECC), with
			 * is similar to Chipkill.
			 */
			dimm->edac_mode = MTR_DRAM_WIDTH(mtr) == 8 ?
					  EDAC_S8ECD8ED : EDAC_S4ECD4ED;
			ndimms++;
		}
	}

	/*
	 * When just one memory is provided, it should be at location (0,0,0).
	 * With such single-DIMM mode, the SDCC algorithm degrades to SECDEC+.
	 */
	if (ndimms == 1)
		mci->dimms[0]->edac_mode = EDAC_SECDED;

	return (ndimms == 0);
}

/*
 *	i5400_enable_error_reporting
 *			Turn on the memory reporting features of the hardware
 */
static void i5400_enable_error_reporting(struct mem_ctl_info *mci)
{
	struct i5400_pvt *pvt;
	u32 fbd_error_mask;

	pvt = mci->pvt_info;

	/* Read the FBD Error Mask Register */
	pci_read_config_dword(pvt->branchmap_werrors, EMASK_FBD,
			&fbd_error_mask);

	/* Enable with a '0' */
	fbd_error_mask &= ~(ENABLE_EMASK_ALL);

	pci_write_config_dword(pvt->branchmap_werrors, EMASK_FBD,
			fbd_error_mask);
}

/*
 *	i5400_probe1	Probe for ONE instance of device to see if it is
 *			present.
 *	return:
 *		0 for FOUND a device
 *		< 0 for error code
 */
static int i5400_probe1(struct pci_dev *pdev, int dev_idx)
{
	struct mem_ctl_info *mci;
	struct i5400_pvt *pvt;
	struct edac_mc_layer layers[3];

	if (dev_idx >= ARRAY_SIZE(i5400_devs))
		return -EINVAL;

	edac_dbg(0, "MC: pdev bus %u dev=0x%x fn=0x%x\n",
		 pdev->bus->number,
		 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));

	/* We only are looking for func 0 of the set */
	if (PCI_FUNC(pdev->devfn) != 0)
		return -ENODEV;

	/*
	 * allocate a new MC control structure
	 *
	 * This drivers uses the DIMM slot as "csrow" and the rest as "channel".
	 */
	layers[0].type = EDAC_MC_LAYER_BRANCH;
	layers[0].size = MAX_BRANCHES;
	layers[0].is_virt_csrow = false;
	layers[1].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = CHANNELS_PER_BRANCH;
	layers[1].is_virt_csrow = false;
	layers[2].type = EDAC_MC_LAYER_SLOT;
	layers[2].size = DIMMS_PER_CHANNEL;
	layers[2].is_virt_csrow = true;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers, sizeof(*pvt));
	if (mci == NULL)
		return -ENOMEM;

	edac_dbg(0, "MC: mci = %p\n", mci);

	mci->pdev = &pdev->dev;	/* record ptr  to the generic device */

	pvt = mci->pvt_info;
	pvt->system_address = pdev;	/* Record this device in our private */
	pvt->maxch = MAX_CHANNELS;
	pvt->maxdimmperch = DIMMS_PER_CHANNEL;

	/* 'get' the pci devices we want to reserve for our use */
	if (i5400_get_devices(mci, dev_idx))
		goto fail0;

	/* Time to get serious */
	i5400_get_mc_regs(mci);	/* retrieve the hardware registers */

	mci->mc_idx = 0;
	mci->mtype_cap = MEM_FLAG_FB_DDR2;
	mci->edac_ctl_cap = EDAC_FLAG_NONE;
	mci->edac_cap = EDAC_FLAG_NONE;
	mci->mod_name = "i5400_edac.c";
	mci->mod_ver = I5400_REVISION;
	mci->ctl_name = i5400_devs[dev_idx].ctl_name;
	mci->dev_name = pci_name(pdev);
	mci->ctl_page_to_phys = NULL;

	/* Set the function pointer to an actual operation function */
	mci->edac_check = i5400_check_error;

	/* initialize the MC control structure 'dimms' table
	 * with the mapping and control information */
	if (i5400_init_dimms(mci)) {
		edac_dbg(0, "MC: Setting mci->edac_cap to EDAC_FLAG_NONE because i5400_init_dimms() returned nonzero value\n");
		mci->edac_cap = EDAC_FLAG_NONE;	/* no dimms found */
	} else {
		edac_dbg(1, "MC: Enable error reporting now\n");
		i5400_enable_error_reporting(mci);
	}

	/* add this new MC control structure to EDAC's list of MCs */
	if (edac_mc_add_mc(mci)) {
		edac_dbg(0, "MC: failed edac_mc_add_mc()\n");
		/* FIXME: perhaps some code should go here that disables error
		 * reporting if we just enabled it
		 */
		goto fail1;
	}

	i5400_clear_error(mci);

	/* allocating generic PCI control info */
	i5400_pci = edac_pci_create_generic_ctl(&pdev->dev, EDAC_MOD_STR);
	if (!i5400_pci) {
		printk(KERN_WARNING
			"%s(): Unable to create PCI control\n",
			__func__);
		printk(KERN_WARNING
			"%s(): PCI error report via EDAC not setup\n",
			__func__);
	}

	return 0;

	/* Error exit unwinding stack */
fail1:

	i5400_put_devices(mci);

fail0:
	edac_mc_free(mci);
	return -ENODEV;
}

/*
 *	i5400_init_one	constructor for one instance of device
 *
 * 	returns:
 *		negative on error
 *		count (>= 0)
 */
static int i5400_init_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rc;

	edac_dbg(0, "MC:\n");

	/* wake up device */
	rc = pci_enable_device(pdev);
	if (rc)
		return rc;

	/* now probe and enable the device */
	return i5400_probe1(pdev, id->driver_data);
}

/*
 *	i5400_remove_one	destructor for one instance of device
 *
 */
static void i5400_remove_one(struct pci_dev *pdev)
{
	struct mem_ctl_info *mci;

	edac_dbg(0, "\n");

	if (i5400_pci)
		edac_pci_release_generic_ctl(i5400_pci);

	mci = edac_mc_del_mc(&pdev->dev);
	if (!mci)
		return;

	/* retrieve references to resources, and free those resources */
	i5400_put_devices(mci);

	pci_disable_device(pdev);

	edac_mc_free(mci);
}

/*
 *	pci_device_id	table for which devices we are looking for
 *
 *	The "E500P" device is the first device supported.
 */
static const struct pci_device_id i5400_pci_tbl[] = {
	{PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_5400_ERR)},
	{0,}			/* 0 terminated list. */
};

MODULE_DEVICE_TABLE(pci, i5400_pci_tbl);

/*
 *	i5400_driver	pci_driver structure for this module
 *
 */
static struct pci_driver i5400_driver = {
	.name = "i5400_edac",
	.probe = i5400_init_one,
	.remove = i5400_remove_one,
	.id_table = i5400_pci_tbl,
};

/*
 *	i5400_init		Module entry function
 *			Try to initialize this module for its devices
 */
static int __init i5400_init(void)
{
	int pci_rc;

	edac_dbg(2, "MC:\n");

	/* Ensure that the OPSTATE is set correctly for POLL or NMI */
	opstate_init();

	pci_rc = pci_register_driver(&i5400_driver);

	return (pci_rc < 0) ? pci_rc : 0;
}

/*
 *	i5400_exit()	Module exit function
 *			Unregister the driver
 */
static void __exit i5400_exit(void)
{
	edac_dbg(2, "MC:\n");
	pci_unregister_driver(&i5400_driver);
}

module_init(i5400_init);
module_exit(i5400_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ben Woodard <woodard@redhat.com>");
MODULE_AUTHOR("Mauro Carvalho Chehab");
MODULE_AUTHOR("Red Hat Inc. (http://www.redhat.com)");
MODULE_DESCRIPTION("MC Driver for Intel I5400 memory controllers - "
		   I5400_REVISION);

module_param(edac_op_state, int, 0444);
MODULE_PARM_DESC(edac_op_state, "EDAC Error Reporting state: 0=Poll,1=NMI");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Intel 7300 class Memory Controllers kernel module (Clarksboro)
 *
 * This file may be distributed under the terms of the
 * GNU General Public License version 2 only.
 *
 * Copyright (c) 2010 by:
 *	 Mauro Carvalho Chehab
 *
 * Red Hat Inc. http://www.redhat.com
 *
 * Intel 7300 Chipset Memory Controller Hub (MCH) - Datasheet
 *	http://www.intel.com/Assets/PDF/datasheet/318082.pdf
 *
 * TODO: The chipset allow checking for PCI Express errors also. Currently,
 *	 the driver covers only memory error errors
 *
 * This driver uses "csrows" EDAC attribute to represent DIMM slot#
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
 * Alter this version for the I7300 module when modifications are made
 */
#define I7300_REVISION    " Ver: 1.0.0"

#define EDAC_MOD_STR      "i7300_edac"

#define i7300_printk(level, fmt, arg...) \
	edac_p