/
	pci_read_config_dword(pvt->pci_dev_16_2_fsb_err_regs,
			      FERR_GLOBAL_HI, &error_reg);
	if (unlikely(error_reg)) {
		errors = error_reg;
		errnum = find_first_bit(&errors,
					ARRAY_SIZE(ferr_global_hi_name));
		specific = GET_ERR_FROM_TABLE(ferr_global_hi_name, errnum);
		is_fatal = ferr_global_hi_is_fatal(errnum);

		/* Clear the error bit */
		pci_write_config_dword(pvt->pci_dev_16_2_fsb_err_regs,
				       FERR_GLOBAL_HI, error_reg);

		goto error_global;
	}

	pci_read_config_dword(pvt->pci_dev_16_2_fsb_err_regs,
			      FERR_GLOBAL_LO, &error_reg);
	if (unlikely(error_reg)) {
		errors = error_reg;
		errnum = find_first_bit(&errors,
					ARRAY_SIZE(ferr_global_lo_name));
		specific = GET_ERR_FROM_TABLE(ferr_global_lo_name, errnum);
		is_fatal = ferr_global_lo_is_fatal(errnum);

		/* Clear the error bit */
		pci_write_config_dword(pvt->pci_dev_16_2_fsb_err_regs,
				       FERR_GLOBAL_LO, error_reg);

		goto error_global;
	}
	return;

error_global:
	i7300_mc_printk(mci, KERN_EMERG, "%s misc error: %s\n",
			is_fatal ? "Fatal" : "NOT fatal", specific);
}

/**
 * i7300_process_fbd_error() - Retrieve the hardware error information from
 *			       the FBD error registers and sends it via
 *			       EDAC error API calls
 * @mci: struct mem_ctl_info pointer
 */
static void i7300_process_fbd_error(struct mem_ctl_info *mci)
{
	struct i7300_pvt *pvt;
	u32 errnum, value, error_reg;
	u16 val16;
	unsigned branch, channel, bank, rank, cas, ras;
	u32 syndrome;

	unsigned long errors;
	const char *specific;
	bool is_wr;

	pvt = mci->pvt_info;

	/* read in the 1st FATAL error register */
	pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			      FERR_FAT_FBD, &error_reg);
	if (unlikely(error_reg & FERR_FAT_FBD_ERR_MASK)) {
		errors = error_reg & FERR_FAT_FBD_ERR_MASK ;
		errnum = find_first_bit(&errors,
					ARRAY_SIZE(ferr_fat_fbd_name));
		specific = GET_ERR_FROM_TABLE(ferr_fat_fbd_name, errnum);
		branch = (GET_FBD_FAT_IDX(error_reg) == 2) ? 1 : 0;

		pci_read_config_word(pvt->pci_dev_16_1_fsb_addr_map,
				     NRECMEMA, &val16);
		bank = NRECMEMA_BANK(val16);
		rank = NRECMEMA_RANK(val16);

		pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
				NRECMEMB, &value);
		is_wr = NRECMEMB_IS_WR(value);
		cas = NRECMEMB_CAS(value);
		ras = NRECMEMB_RAS(value);

		/* Clean the error register */
		pci_write_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
				FERR_FAT_FBD, error_reg);

		snprintf(pvt->tmp_prt_buffer, PAGE_SIZE,
			 "Bank=%d RAS=%d CAS=%d Err=0x%lx (%s))",
			 bank, ras, cas, errors, specific);

		edac_mc_handle_error(HW_EVENT_ERR_FATAL, mci, 1, 0, 0, 0,
				     branch, -1, rank,
				     is_wr ? "Write error" : "Read error",
				     pvt->tmp_prt_buffer);

	}

	/* read in the 1st NON-FATAL error register */
	pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			      FERR_NF_FBD, &error_reg);
	if (unlikely(error_reg & FERR_NF_FBD_ERR_MASK)) {
		errors = error_reg & FERR_NF_FBD_ERR_MASK;
		errnum = find_first_bit(&errors,
					ARRAY_SIZE(ferr_nf_fbd_name));
		specific = GET_ERR_FROM_TABLE(ferr_nf_fbd_name, errnum);
		branch = (GET_FBD_NF_IDX(error_reg) == 2) ? 1 : 0;

		pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			REDMEMA, &syndrome);

		pci_read_config_word(pvt->pci_dev_16_1_fsb_addr_map,
				     RECMEMA, &val16);
		bank = RECMEMA_BANK(val16);
		rank = RECMEMA_RANK(val16);

		pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
				RECMEMB, &value);
		is_wr = RECMEMB_IS_WR(value);
		cas = RECMEMB_CAS(value);
		ras = RECMEMB_RAS(value);

		pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
				     REDMEMB, &value);
		channel = (branch << 1);
		if (IS_SECOND_CH(value))
			channel++;

		/* Clear the error bit */
		pci_write_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
				FERR_NF_FBD, error_reg);

		/* Form out message */
		snprintf(pvt->tmp_prt_buffer, PAGE_SIZE,
			 "DRAM-Bank=%d RAS=%d CAS=%d, Err=0x%lx (%s))",
			 bank, ras, cas, errors, specific);

		edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1, 0, 0,
				     syndrome,
				     branch >> 1, channel % 2, rank,
				     is_wr ? "Write error" : "Read error",
				     pvt->tmp_prt_buffer);
	}
	return;
}

/**
 * i7300_check_error() - Calls the error checking subroutines
 * @mci: struct mem_ctl_info pointer
 */
static void i7300_check_error(struct mem_ctl_info *mci)
{
	i7300_process_error_global(mci);
	i7300_process_fbd_error(mci);
};

/**
 * i7300_clear_error() - Clears the error registers
 * @mci: struct mem_ctl_info pointer
 */
static void i7300_clear_error(struct mem_ctl_info *mci)
{
	struct i7300_pvt *pvt = mci->pvt_info;
	u32 value;
	/*
	 * All error values are RWC - we need to read and write 1 to the
	 * bit that we want to cleanup
	 */

	/* Clear global error registers */
	pci_read_config_dword(pvt->pci_dev_16_2_fsb_err_regs,
			      FERR_GLOBAL_HI, &value);
	pci_write_config_dword(pvt->pci_dev_16_2_fsb_err_regs,
			      FERR_GLOBAL_HI, value);

	pci_read_config_dword(pvt->pci_dev_16_2_fsb_err_regs,
			      FERR_GLOBAL_LO, &value);
	pci_write_config_dword(pvt->pci_dev_16_2_fsb_err_regs,
			      FERR_GLOBAL_LO, value);

	/* Clear FBD error registers */
	pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			      FERR_FAT_FBD, &value);
	pci_write_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			      FERR_FAT_FBD, value);

	pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			      FERR_NF_FBD, &value);
	pci_write_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			      FERR_NF_FBD, value);
}

/**
 * i7300_enable_error_reporting() - Enable the memory reporting logic at the
 *				    hardware
 * @mci: struct mem_ctl_info pointer
 */
static void i7300_enable_error_reporting(struct mem_ctl_info *mci)
{
	struct i7300_pvt *pvt = mci->pvt_info;
	u32 fbd_error_mask;

	/* Read the FBD Error Mask Register */
	pci_read_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			      EMASK_FBD, &fbd_error_mask);

	/* Enable with a '0' */
	fbd_error_mask &= ~(EMASK_FBD_ERR_MASK);

	pci_write_config_dword(pvt->pci_dev_16_1_fsb_addr_map,
			       EMASK_FBD, fbd_error_mask);
}

/************************************************
 * i7300 Functions related to memory enumberation
 ************************************************/

/**
 * decode_mtr() - Decodes the MTR descriptor, filling the edac structs
 * @pvt: pointer to the private data struct used by i7300 driver
 * @slot: DIMM slot (0 to 7)
 * @ch: Channel number within the branch (0 or 1)
 * @branch: Branch number (0 or 1)
 * @dinfo: Pointer to DIMM info where dimm size is stored
 * @p_csrow: Pointer to the struct csrow_info that corresponds to that element
 */
static int decode_mtr(struct i7300_pvt *pvt,
		      int slot, int ch, int branch,
		      struct i7300_dimm_info *dinfo,
		      struct dimm_info *dimm)
{
	int mtr, ans, addrBits, channel;

	channel = to_channel(ch, branch);

	mtr = pvt->mtr[slot][branch];