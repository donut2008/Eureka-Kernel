 per channel are
	 * supported on this memory controller
	 */
	pci_read_config_byte(pdev, MAXDIMMPERCH, &value);
	*num_dimms_per_channel = (int)value;

	pci_read_config_byte(pdev, MAXCH, &value);
	*num_channels = (int)value;
}

/*
 *	i5000_probe1	Probe for ONE instance of device to see if it is
 *			present.
 *	return:
 *		0 for FOUND a device
 *		< 0 for error code
 */
static int i5000_probe1(struct pci_dev *pdev, int dev_idx)
{
	struct mem_ctl_info *mci;
	struct edac_mc_layer layers[3];
	struct i5000_pvt *pvt;
	int num_channels;
	int num_dimms_per_channel;

	edac_dbg(0, "MC: pdev bus %u dev=0x%x fn=0x%x\n",
		 pdev->bus->number,
		 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));

	/* We only are looking for func 0 of the set */
	if (PCI_FUNC(pdev->devfn) != 0)
		return -ENODEV;

	/* Ask the devices for the number of CSROWS and CHANNELS so
	 * that we can calculate the memory resources, etc
	 *
	 * The Chipset will report what it can handle which will be greater
	 * or equal to what the motherboard manufacturer will implement.
	 *
	 * As we don't have a motherboard identification routine to determine
	 * actual number of slots/dimms per channel, we thus utilize the
	 * resource as specified by the chipset. Thus, we might have
	 * have more DIMMs per channel than actually on the mobo, but this
	 * allows the driver to support up to the chipset max, without
	 * some fancy mobo determination.
	 */
	i5000_get_dimm_and_channel_counts(pdev, &num_dimms_per_channel,
					&num_channels);

	edac_dbg(0, "MC: Number of Branches=2 Channels= %d  DIMMS= %d\n",
		 num_channels, num_dimms_per_channel);

	/* allocate a new MC control structure */

	layers[0].type = EDAC_MC_LAYER_BRANCH;
	layers[0].size = MAX_BRANCHES;
	layers[0].is_virt_csrow = false;
	layers[1].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = num_channels / MAX_BRANCHES;
	layers[1].is_virt_csrow = false;
	layers[2].type = EDAC_MC_LAYER_SLOT;
	layers[2].size = num_dimms_per_channel;
	layers[2].is_virt_csrow = true;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers, sizeof(*pvt));
	if (mci == NULL)
		return -ENOMEM;

	edac_dbg(0, "MC: mci = %p\n", mci);

	mci->pdev = &pdev->dev;	/* record ptr  to the generic device */

	pvt = mci->pvt_info;
	pvt->system_address = pdev;	/* Record this device in our private */
	pvt->maxch = num_channels;
	pvt->maxdimmperch = num_dimms_per_channel;

	/* 'get' the pci devices we want to reserve for our use */
	if (i5000_get_devices(mci, dev_idx))
		goto fail0;

	/* Time to get serious */
	i5000_get_mc_regs(mci);	/* retrieve the hardware registers */

	mci->mc_idx = 0;
	mci->mtype_cap = MEM_FLAG_FB_DDR2;
	mci->edac_ctl_cap = EDAC_FLAG_NONE;
	mci->edac_cap = EDAC_FLAG_NONE;
	mci->mod_name = "i5000_edac.c";
	mci->mod_ver = I5000_REVISION;
	mci->ctl_name = i5000_devs[dev_idx].ctl_name;
	mci->dev_name = pci_name(pdev);
	mci->ctl_page_to_phys = NULL;

	/* Set the function pointer to an actual operation function */
	mci->edac_check = i5000_check_error;

	/* initialize the MC control structure 'csrows' table
	 * with the mapping and control information */
	if (i5000_init_csrows(mci)) {
		edac_dbg(0, "MC: Setting mci->edac_cap to EDAC_FLAG_NONE because i5000_init_csrows() returned nonzero value\n");
		mci->edac_cap = EDAC_FLAG_NONE;	/* no csrows found */
	} else {
		edac_dbg(1, "MC: Enable error reporting now\n");
		i5000_enable_error_reporting(mci);
	}

	/* add this new MC control structure to EDAC's list of MCs */
	if (edac_mc_add_mc(mci)) {
		edac_dbg(0, "MC: failed edac_mc_add_mc()\n");
		/* FIXME: perhaps some code should go here that disables error
		 * reporting if we just enabled it
		 */
		goto fail1;
	}

	i5000_clear_error(mci);

	/* allocating generic PCI control info */
	i5000_pci = edac_pci_create_generic_ctl(&pdev->dev, EDAC_MOD_STR);
	if (!i5000_pci) {
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

	i5000_put_devices(mci);

fail0:
	edac_mc_free(mci);
	return -ENODEV;
}

/*
 *	i5000_init_one	constructor for one instance of device
 *
 * 	returns:
 *		negative on error
 *		count (>= 0)
 */
static int i5000_init_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rc;

	edac_dbg(0, "MC:\n");

	/* wake up device */
	rc = pci_enable_device(pdev);
	if (rc)
		return rc;

	/* now probe and enable the device */
	return i5000_probe1(pdev, id->driver_data);
}

/*
 *	i5000_remove_one	destructor for one instance of device
 *
 */
static void i5000_remove_one(struct pci_dev *pdev)
{
	struct mem_ctl_info *mci;

	edac_dbg(0, "\n");

	if (i5000_pci)
		edac_pci_release_generic_ctl(i5000_pci);

	if ((mci = edac_mc_del_mc(&pdev->dev)) == NULL)
		return;

	/* retrieve references to resources, and free those resources */
	i5000_put_devices(mci);
	edac_mc_free(mci);
}

/*
 *	pci_device_id	table for which devices we are looking for
 *
 *	The "E500P" device is the first device supported.
 */
static const struct pci_device_id i5000_pci_tbl[] = {
	{PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_I5000_DEV16),
	 .driver_data = I5000P},

	{0,}			/* 0 terminated list. */
};

MODULE_DEVICE_TABLE(pci, i5000_pci_tbl);

/*
 *	i5000_driver	pci_driver structure for this module
 *
 */
static struct pci_driver i5000_driver = {
	.name = KBUILD_BASENAME,
	.probe = i5000_init_one,
	.remove = i5000_remove_one,
	.id_table = i5000_pci_tbl,
};

/*
 *	i5000_init		Module entry function
 *			Try to initialize this module for its devices
 */
static int __init i5000_init(void)
{
	int pci_rc;

	edac_dbg(2, "MC:\n");

       /* Ensure that the OPSTATE is set correctly for POLL or NMI */
       opstate_init();

	pci_rc = pci_register_driver(&i5000_driver);

	return (pci_rc < 0) ? pci_rc : 0;
}

/*
 *	i5000_exit()	Module exit function
 *			Unregister the driver
 */
static void __exit i5000_exit(void)
{
	edac_dbg(2, "MC:\n");
	pci_unregister_driver(&i5000_driver);
}

module_init(i5000_init);
module_exit(i5000_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR
    ("Linux Networx (http://lnxi.com) Doug Thompson <norsk5@xmission.com>");
MODULE_DESCRIPTION("MC Driver for Intel I5000 memory controllers - "
		I5000_REVISION);

module_param(edac_op_state, int, 0444);
MODULE_PARM_DESC(edac_op_state, "EDAC Error Reporting state: 0=Poll,1=NMI");
module_param(misc_messages, int, 0444);
MODULE_PARM_DESC(misc_messages, "Log miscellaneous non fatal messages");

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * Intel 5100 Memory Controllers kernel module
 *
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * This module is based on the following document:
 *
 * Intel 5100X Chipset Memory Controller Hub (MCH) - Datasheet
 *      http://download.intel.com/design/chipsets/datashts/318378.pdf
 *
 * The intel 5100 has two independent channels. EDAC core currently
 * can not reflect this configuration so instead the chip-select
 * rows for each respective channel are laid out one after another,
 * the first half