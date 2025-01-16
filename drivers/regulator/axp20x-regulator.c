DIMM_COLS(mtr) == 1 ? "2,048 - 11 columns" :
		 MTR_DIMM_COLS(mtr) == 2 ? "4,096 - 12 columns" :
		 "reserved");
	edac_dbg(2, "\t\tSIZE: %d MB\n", dinfo->megabytes);

	/*
	 * The type of error detection actually depends of the
	 * mode of operation. When it is just one single memory chip, at
	 * socket 0, channel 0, it uses 8-byte-over-32-byte SECDED+ code.
	 * In normal or mirrored mode, it uses Lockstep mode,
	 * with the possibility of using an extended algorithm for x8 memories
	 * See datasheet Sections 7.3.6 to 7.3.8
	 */

	dimm->nr_pages = MiB_TO_PAGES(dinfo->megabytes);
	dimm->grain = 8;
	dimm->mtype = MEM_FB_DDR2;
	if (IS_SINGLE_MODE(pvt->mc_settings_a)) {
		dimm->edac_mode = EDAC_SECDED;
		edac_dbg(2, "\t\tECC code is 8-byte-over-32-byte SECDED+ code\n");
	} else {
		edac_dbg(2, "\t\tECC code is on Lockstep mode\n");
		if (MTR_DRAM_WIDTH(mtr) == 8)
			dimm->edac_mode = EDAC_S8ECD8ED;
		else
			dimm->edac_mode = EDAC_S4ECD4ED;
	}

	/* ask what device type on this row */
	if (MTR_DRAM_WIDTH(mtr) == 8) {
		edac_dbg(2, "\t\tScrub algorithm for x8 is on %s mode\n",
			 IS_SCRBALGO_ENHANCED(pvt->mc_settings) ?
			 "enhanced" : "normal");

		dimm->dtype = DEV_X8;
	} else
		dimm->dtype = DEV_X4;

	return mtr;
}

/**
 * print_dimm_size() - Prints dump of the memory organization
 * @pvt: pointer to the private data struct used by i7300 driver
 *
 * Useful for debug. If debug is disabled, this routine do nothing
 */
static void print_dimm_size(struct i7300_pvt *pvt)
{
#ifdef CONFIG_EDAC_DEBUG
	struct i7300_dimm_info *dinfo;
	char *p;
	int space, n;
	int channel, slot;

	space = PAGE_SIZE;
	p = pvt->tmp_prt_buffer;

	n = snprintf(p, space, "              ");
	p += n;
	space -= n;
	for (channel = 0; channel < MAX_CHANNELS; channel++) {
		n = snprintf(p, space, "channel %d | ", channel);
		p += n;
		space -= n;
	}
	edac_dbg(2, "%s\n", pvt->tmp_prt_buffer);
	p = pvt->tmp_prt_buffer;
	space = PAGE_SIZE;
	n = snprintf(p, space, "-------------------------------"
			       "------------------------------");
	p += n;
	space -= n;
	edac_dbg(2, "%s\n", pvt->tmp_prt_buffer);
	p = pvt->tmp_prt_buffer;
	space = PAGE_SIZE;

	for (slot = 0; slot < MAX_SLOTS; slot++) {
		n = snprintf(p, space, "csrow/SLOT %d  ", slot);
		p += n;
		space -= n;

		for (channel = 0; channel < MAX_CHANNELS; channel++) {
			dinfo = &pvt->dimm_info[slot][channel];
			n = snprintf(p, space, "%4d MB   | ", dinfo->megabytes);
			p += n;
			space -= n;
		}

		edac_dbg(2, "%s\n", pvt->tmp_prt_buffer);
		p = pvt->tmp_prt_buffer;
		space = PAGE_SIZE;
	}

	n = snprintf(p, space, "-------------------------------"
			       "------------------------------");
	p += n;
	space -= n;
	edac_dbg(2, "%s\n", pvt->tmp_prt_buffer);
	p = pvt->tmp_prt_buffer;
	space = PAGE_SIZE;
#endif
}

/**
 * i7300_init_csrows() - Initialize the 'csrows' table within
 *			 the mci control structure with the
 *			 addressing of memory.
 * @mci: struct mem_ctl_info pointer
 */
static int i7300_init_csrows(struct mem_ctl_info *mci)
{
	struct i7300_pvt *pvt;
	struct i7300_dimm_info *dinfo;
	int rc = -ENODEV;
	int mtr;
	int ch, branch, slot, channel, max_channel, max_branch;
	struct dimm_info *dimm;

	pvt = mci->pvt_info;

	edac_dbg(2, "Memory Technology Registers:\n");

	if (IS_SINGLE_MODE(pvt->mc_settings_a)) {
		max_branch = 1;
		max_channel = 1;
	} else {
		max_branch = MAX_BRANCHES;
		max_channel = MAX_CH_PER_BRANCH;
	}

	/* Get the AMB present registers for the four channels */
	for (branch = 0; branch < max_branch; branch++) {
		/* Read and dump branch 0's MTRs */
		channel = to_channel(0, branch);
		pci_read_config_word(pvt->pci_dev_2x_0_fbd_branch[branch],
				     AMBPRESENT_0,
				&pvt->ambpresent[channel]);
		edac_dbg(2, "\t\tAMB-present CH%d = 0x%x:\n",
			 channel, pvt->ambpresent[channel]);

		if (max_channel == 1)
			continue;

		channel = to_channel(1, branch);
		pci_read_config_word(pvt->pci_dev_2x_0_fbd_branch[branch],
				     AMBPRESENT_1,
				&pvt->ambpresent[channel]);
		edac_dbg(2, "\t\tAMB-present CH%d = 0x%x:\n",
			 channel, pvt->ambpresent[channel]);
	}

	/* Get the set of MTR[0-7] regs by each branch */
	for (slot = 0; slot < MAX_SLOTS; slot++) {
		int where = mtr_regs[slot];
		for (branch = 0; branch < max_branch; branch++) {
			pci_read_config_word(pvt->pci_dev_2x_0_fbd_branch[branch],
					where,
					&pvt->mtr[slot][branch]);
			for (ch = 0; ch < max_channel; ch++) {
				int channel = to_channel(ch, branch);

				dimm = EDAC_DIMM_PTR(mci->layers, mci->dimms,
					       mci->n_layers, branch, ch, slot);

				dinfo = &pvt->dimm_info[slot][channel];

				mtr = decode_mtr(pvt, slot, ch, branch,
						 dinfo, dimm);

				/* if no DIMMS on this row, continue */
				if (!MTR_DIMMS_PRESENT(mtr))
					continue;

				rc = 0;

			}
		}
	}

	return rc;
}

/**
 * decode_mir() - Decodes Memory Interleave Register (MIR) info
 * @int mir_no: number of the MIR register to decode
 * @mir: array with the MIR data cached on the driver
 */
static void decode_mir(int mir_no, u16 mir[MAX_MIR])
{
	if (mir[mir_no] & 3)
		edac_dbg(2, "MIR%d: limit= 0x%x Branch(es) that participate: %s %s\n",
			 mir_no,
			 (mir[mir_no] >> 4) & 0xfff,
			 (mir[mir_no] & 1) ? "B0" : "",
			 (mir[mir_no] & 2) ? "B1" : "");
}

/**
 * i7300_get_mc_regs() - Get the contents of the MC enumeration registers
 * @mci: struct mem_ctl_info pointer
 *
 * Data read is cached internally for its usage when needed
 */
static int i7300_get_mc_regs(struct mem_ctl_info *mci)
{
	struct i7300_pvt *pvt;
	u32 actual_tolm;
	int i, rc;

	pvt = mci->pvt_info;

	pci_read_config_dword(pvt->pci_dev_16_0_fsb_ctlr, AMBASE,
			(u32 *) &pvt->ambase);

	edac_dbg(2, "AMBASE= 0x%lx\n", (long unsigned int)pvt->ambase);

	/* Get the Branch Map regs */
	pci_read_config_word(pvt->pci_dev_16_1_fsb_addr_map, TOLM, &pvt->tolm);
	pvt->tolm >>= 12;
	edac_dbg(2, "TOLM (number of 256M regions) =%u (0x%x)\n",
		 pvt->tolm, pvt->tolm);

	actual_tolm = (u32) ((1000l * pvt->tolm) >> (30 - 28));
	edac_dbg(2, "Actual TOLM byte addr=%u.%03u GB (0x%x)\n",
		 actual_tolm/1000, actual_tolm % 1000, pvt->tolm << 28);

	/* Get memory controller settings */
	pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map, MC_SETTINGS,
			     &pvt->mc_settings);
	pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map, MC_SETTINGS_A,
			     &pvt->mc_settings_a);

	if (IS_SINGLE_MODE(pvt->mc_settings_a))
		edac_dbg(0, "Memory controller operating on single mode\n");
	else
		edac_dbg(0, "Memory controller operating on %smirrored mode\n",
			 IS_MIRRORED(pvt->mc_settings) ? "" : "non-");

	edac_dbg(0, "Error detection is %s\n",
		 IS_ECC_ENABLED(pvt->mc_settings) ? "enabled" : "disabled");
	edac_dbg(0, "Retry is %s\n",
		 IS_RETRY_ENABLED(pvt->mc_settings) ? "enabled" : "disabled");

	/* Get Memory Interleave Range registers */
	pci_read_config_word(pvt->pci_dev_16_1_fsb_addr_map, MIR0,
			     &pvt->mir[0]);
	pci_read_config_word(pvt->pci_dev_16_1_fsb_addr_map, MIR1,
			     &pvt->mir[1]);
	pci_read_config_word(pvt->pci_dev_16_1_fsb_addr_map, MIR2,
			     &pvt->mir[2]);

	/* Decode the MIR regs */
	for (i = 0; i < MAX_MIR; i++)
		decode_mir(i, pvt->mir);

	rc = i7300_init_csrows(mci);
	if (rc < 0)
		return rc;

	/* Go and determine the size of each DIMM and place in an
	 * orderly matrix */
	print_dimm_size(pvt);

	return 0;
}

/*************************************************
 * i7300 Functions related to device probe/release
 *************************************************/

/**
 * i7300_put_devices() - Release the PCI devices
 * @mci: struct mem_ctl_info pointer
 */
static void i7300_put_devices(struct mem_ctl_info *mci)
{
	struct i7300_pvt *pvt;
	int branch;

	pvt = mci->pvt_info;

	/* Decrement usage count for devices */
	for (branch = 0; branch < MAX_CH_PER_BRANCH; branch++)
		pci_dev_put(pvt->pci_dev_2x_0_fbd_branch[branch]);
	pci_dev_put(pvt->pci_dev_16_2_fsb_err_regs);
	pci_dev_put(pvt->pci_dev_16_1_fsb_addr_map);
}

/**
 * i7300_get_devices() - Find and perform 'get' operation on the MCH's
 *			 device/functions we want to reference for this driver
 * @mci: struct mem_ctl_info pointer
 *
 * Access and prepare the several devices for usage:
 * I7300 devices used by this driver:
 *    Device 16, functions 0,1 and 2:	PCI_DEVICE_ID_INTEL_I7300_MCH_ERR
 *    Device 21 function 0:		PCI_DEVICE_ID_INTEL_I7300_MCH_FB0
 *    Device 22 function 0:		PCI_DEVICE_ID_INTEL_I7300_MCH_FB1
 */
static int i7300_get_devices(struct mem_ctl_info *mci)
{
	struct i7300_pvt *pvt;
	struct pci_dev *pdev;

	pvt = mci->pvt_info;

	/* Attempt to 'get' the MCH register we want */
	pdev = NULL;
	while ((pdev = pci_get_device(PCI_VENDOR_ID_INTEL,
				      PCI_DEVICE_ID_INTEL_I7300_MCH_ERR,
				      pdev))) {
		/* Store device 16 funcs 1 and 2 */
		switch (PCI_FUNC(pdev->devfn)) {
		case 1:
			if (!pvt->pci_dev_16_1_fsb_addr_map)
				pvt->pci_dev_16_1_fsb_addr_map =
							pci_dev_get(pdev);
			break;
		case 2:
			if (!pvt->pci_dev_16_2_fsb_err_regs)
				pvt->pci_dev_16_2_fsb_err_regs =
							pci_dev_get(pdev);
			break;
		}
	}

	if (!pvt->pci_dev_16_1_fsb_addr_map ||
	    !pvt->pci_dev_16_2_fsb_err_regs) {
		/* At least one device was not found */
		i7300_printk(KERN_ERR,
			"'system address,Process Bus' device not found:"
			"vendor 0x%x device 0x%x ERR funcs (broken BIOS?)\n",
			PCI_VENDOR_ID_INTEL,
			PCI_DEVICE_ID_INTEL_I7300_MCH_ERR);
		goto error;
	}

	edac_dbg(1, "System Address, processor bus- PCI Bus ID: %s  %x:%x\n",
		 pci_name(pvt->pci_dev_16_0_fsb_ctlr),
		 pvt->pci_dev_16_0_fsb_ctlr->vendor,
		 pvt->pci_dev_16_0_fsb_ctlr->device);
	edac_dbg(1, "Branchmap, control and errors - PCI Bus ID: %s  %x:%x\n",
		 pci_name(pvt->pci_dev_16_1_fsb_addr_map),
		 pvt->pci_dev_16_1_fsb_addr_map->vendor,
		 pvt->pci_dev_16_1_fsb_addr_map->device);
	edac_dbg(1, "FSB Error Regs - PCI Bus ID: %s  %x:%x\n",
		 pci_name(pvt->pci_dev_16_2_fsb_err_regs),
		 pvt->pci_dev_16_2_fsb_err_regs->vendor,
		 pvt->pci_dev_16_2_fsb_err_regs->device);

	pvt->pci_dev_2x_0_fbd_branch[0] = pci_get_device(PCI_VENDOR_ID_INTEL,
					    PCI_DEVICE_ID_INTEL_I7300_MCH_FB0,
					    NULL);
	if (!pvt->pci_dev_2x_0_fbd_branch[0]) {
		i7300_printk(KERN_ERR,
			"MC: 'BRANCH 0' device not found:"
			"vendor 0x%x device 0x%x Func 0 (broken BIOS?)\n",
			PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_I7300_MCH_FB0);
		goto error;
	}

	pvt->pci_dev_2x_0_fbd_branch[1] = pci_get_device(PCI_VENDOR_ID_INTEL,
					    PCI_DEVICE_ID_INTEL_I7300_MCH_FB1,
					    NULL);
	if (!pvt->pci_dev_2x_0_fbd_branch[1]) {
		i7300_printk(KERN_ERR,
			"MC: 'BRANCH 1' device not found:"
			"vendor 0x%x device 0x%x Func 0 "
			"(broken BIOS?)\n",
			PCI_VENDOR_ID_INTEL,
			PCI_DEVICE_ID_INTEL_I7300_MCH_FB1);
		goto error;
	}

	return 0;

error:
	i7300_put_devices(mci);
	return -ENODEV;
}

/**
 * i7300_init_one() - Probe for one instance of the device
 * @pdev: struct pci_dev pointer
 * @id: struct pci_device_id pointer - currently unused
 */
static int i7300_init_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct mem_ctl_info *mci;
	struct edac_mc_layer layers[3];
	struct i7300_pvt *pvt;
	int rc;

	/* wake up device */
	rc = pci_enable_device(pdev);
	if (rc == -EIO)
		return rc;

	edac_dbg(0, "MC: pdev bus %u dev=0x%x fn=0x%x\n",
		 pdev->bus->number,
		 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));

	/* We only are looking for func 0 of the set */
	if (PCI_FUNC(pdev->devfn) != 0)
		return -ENODEV;

	/* allocate a new MC control structure */
	layers[0].type = EDAC_MC_LAYER_BRANCH;
	layers[0].size = MAX_BRANCHES;
	layers[0].is_virt_csrow = false;
	layers[1].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = MAX_CH_PER_BRANCH;
	layers[1].is_virt_csrow = true;
	layers[2].type = EDAC_MC_LAYER_SLOT;
	layers[2].size = MAX_SLOTS;
	layers[2].is_virt_csrow = true;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers, sizeof(*pvt));
	if (mci == NULL)
		return -ENOMEM;

	edac_dbg(0, "MC: mci = %p\n", mci);

	mci->pdev = &pdev->dev;	/* record ptr  to the generic device */

	pvt = mci->pvt_info;
	pvt->pci_dev_16_0_fsb_ctlr = pdev;	/* Record this device in our private */

	pvt->tmp_prt_buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!pvt->tmp_prt_buffer) {
		edac_mc_free(mci);
		return -ENOMEM;
	}

	/* 'get' the pci devices we want to reserve for our use */
	if (i7300_get_devices(mci))
		goto fail0;

	mci->mc_idx = 0;
	mci->mtype_cap = MEM_FLAG_FB_DDR2;
	mci->edac_ctl_cap = EDAC_FLAG_NONE;
	mci->edac_cap = EDAC_FLAG_NONE;
	mci->mod_name = "i7300_edac.c";
	mci->mod_ver = I7300_REVISION;
	mci->ctl_name = i7300_devs[0].ctl_name;
	mci->dev_name = pci_name(pdev);
	mci->ctl_page_to_phys = NULL;

	/* Set the function pointer to an actual operation function */
	mci->edac_check = i7300_check_error;

	/* initialize the MC control structure 'csrows' table
	 * with the mapping and control information */
	if (i7300_get_mc_regs(mci)) {
		edac_dbg(0, "MC: Setting mci->edac_cap to EDAC_FLAG_NONE because i7300_init_csrows() returned nonzero value\n");
		mci->edac_cap = EDAC_FLAG_NONE;	/* no csrows found */
	} else {
		edac_dbg(1, "MC: Enable error reporting now\n");
		i7300_enable_error_reporting(mci);
	}

	/* add this new MC control structure to EDAC's list of MCs */
	if (edac_mc_add_mc(mci)) {
		edac_dbg(0, "MC: failed edac_mc_add_mc()\n");
		/* FIXME: perhaps some code should go here that disables error
		 * reporting if we just enabled it
		 */
		goto fail1;
	}

	i7300_clear_error(mci);

	/* allocating generic PCI control info */
	i7300_pci = edac_pci_create_generic_ctl(&pdev->dev, EDAC_MOD_STR);
	if (!i7300_pci) {
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

	i7300_put_devices(mci);

fail0:
	kfree(pvt->tmp_prt_buffer);
	edac_mc_free(mci);
	return -ENODEV;
}

/**
 * i7300_remove_one() - Remove the driver
 * @pdev: struct pci_dev pointer
 */
static void i7300_remove_one(struct pci_dev *pdev)
{
	struct mem_ctl_info *mci;
	char *tmp;

	edac_dbg(0, "\n");

	if (i7300_pci)
		edac_pci_release_generic_ctl(i7300_pci);

	mci = edac_mc_del_mc(&pdev->dev);
	if (!mci)
		return;

	tmp = ((struct i7300_pvt *)mci->pvt_info)->tmp_prt_buffer;

	/* retrieve references to resources, and free those resources */
	i7300_put_devices(mci);

	kfree(tmp);
	edac_mc_free(mci);
}

/*
 * pci_device_id: table for which devices we are looking for
 *
 * Has only 8086:3