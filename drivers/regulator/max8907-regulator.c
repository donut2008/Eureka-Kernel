= NULL;
	char *p;
	u8 grain_bits;

	list_for_each_entry(pvt, &ghes_reglist, list) {
		if (ghes == pvt->ghes)
			break;
	}
	if (!pvt) {
		pr_err("Internal error: Can't find EDAC structure\n");
		return;
	}
	mci = pvt->mci;
	e = &mci->error_desc;

	/* Cleans the error report buffer */
	memset(e, 0, sizeof (*e));
	e->error_count = 1;
	e->grain = 1;
	strcpy(e->label, "unknown label");
	e->msg = pvt->msg;
	e->other_detail = pvt->other_detail;
	e->top_layer = -1;
	e->mid_layer = -1;
	e->low_layer = -1;
	*pvt->other_detail = '\0';
	*pvt->msg = '\0';

	switch (sev) {
	case GHES_SEV_CORRECTED:
		type = HW_EVENT_ERR_CORRECTED;
		break;
	case GHES_SEV_RECOVERABLE:
		type = HW_EVENT_ERR_UNCORRECTED;
		break;
	case GHES_SEV_PANIC:
		type = HW_EVENT_ERR_FATAL;
		break;
	default:
	case GHES_SEV_NO:
		type = HW_EVENT_ERR_INFO;
	}

	edac_dbg(1, "error validation_bits: 0x%08llx\n",
		 (long long)mem_err->validation_bits);

	/* Error type, mapped on e->msg */
	if (mem_err->validation_bits & CPER_MEM_VALID_ERROR_TYPE) {
		p = pvt->msg;
		switch (mem_err->error_type) {
		case 0:
			p += sprintf(p, "Unknown");
			break;
		case 1:
			p += sprintf(p, "No error");
			break;
		case 2:
			p += sprintf(p, "Single-bit ECC");
			break;
		case 3:
			p += sprintf(p, "Multi-bit ECC");
			break;
		case 4:
			p += sprintf(p, "Single-symbol ChipKill ECC");
			break;
		case 5:
			p += sprintf(p, "Multi-symbol ChipKill ECC");
			break;
		case 6:
			p += sprintf(p, "Master abort");
			break;
		case 7:
			p += sprintf(p, "Target abort");
			break;
		case 8:
			p += sprintf(p, "Parity Error");
			break;
		case 9:
			p += sprintf(p, "Watchdog timeout");
			break;
		case 10:
			p += sprintf(p, "Invalid address");
			break;
		case 11:
			p += sprintf(p, "Mirror Broken");
			break;
		case 12:
			p += sprintf(p, "Memory Sparing");
			break;
		case 13:
			p += sprintf(p, "Scrub corrected error");
			break;
		case 14:
			p += sprintf(p, "Scrub uncorrected error");
			break;
		case 15:
			p += sprintf(p, "Physical Memory Map-out event");
			break;
		default:
			p += sprintf(p, "reserved error (%d)",
				     mem_err->error_type);
		}
	} else {
		strcpy(pvt->msg, "unknown error");
	}

	/* Error address */
	if (mem_err->validation_bits & CPER_MEM_VALID_PA) {
		e->page_frame_number = mem_err->physical_addr >> PAGE_SHIFT;
		e->offset_in_page = mem_err->physical_addr & ~PAGE_MASK;
	}

	/* Error grain */
	if (mem_err->validation_bits & CPER_MEM_VALID_PA_MASK)
		e->grain = ~mem_err->physical_addr_mask + 1;

	/* Memory error location, mapped on e->location */
	p = e->location;
	if (mem_err->validation_bits & CPER_MEM_VALID_NODE)
		p += sprintf(p, "node:%d ", mem_err->node);
	if (mem_err->validation_bits & CPER_MEM_VALID_CARD)
		p += sprintf(p, "card:%d ", mem_err->card);
	if (mem_err->validation_bits & CPER_MEM_VALID_MODULE)
		p += sprintf(p, "module:%d ", mem_err->module);
	if (mem_err->validation_bits & CPER_MEM_VALID_RANK_NUMBER)
		p += sprintf(p, "rank:%d ", mem_err->rank);
	if (mem_err->validation_bits & CPER_MEM_VALID_BANK)
		p += sprintf(p, "bank:%d ", mem_err->bank);
	if (mem_err->validation_bits & CPER_MEM_VALID_ROW)
		p += sprintf(p, "row:%d ", mem_err->row);
	if (mem_err->validation_bits & CPER_MEM_VALID_COLUMN)
		p += sprintf(p, "col:%d ", mem_err->column);
	if (mem_err->validation_bits & CPER_MEM_VALID_BIT_POSITION)
		p += sprintf(p, "bit_pos:%d ", mem_err->bit_pos);
	if (mem_err->validation_bits & CPER_MEM_VALID_MODULE_HANDLE) {
		const char *bank = NULL, *device = NULL;
		dmi_memdev_name(mem_err->mem_dev_handle, &bank, &device);
		if (bank != NULL && device != NULL)
			p += sprintf(p, "DIMM location:%s %s ", bank, device);
		else
			p += sprintf(p, "DIMM DMI handle: 0x%.4x ",
				     mem_err->mem_dev_handle);
	}
	if (p > e->location)
		*(p - 1) = '\0';

	/* All other fields are mapped on e->other_detail */
	p = pvt->other_detail;
	if (mem_err->validation_bits & CPER_MEM_VALID_ERROR_STATUS) {
		u64 status = mem_err->error_status;

		p += sprintf(p, "status(0x%016llx): ", (long long)status);
		switch ((status >> 8) & 0xff) {
		case 1:
			p += sprintf(p, "Error detected internal to the component ");
			break;
		case 16:
			p += sprintf(p, "Error detected in the bus ");
			break;
		case 4:
			p += sprintf(p, "Storage error in DRAM memory ");
			break;
		case 5:
			p += sprintf(p, "Storage error in TLB ");
			break;
		case 6:
			p += sprintf(p, "Storage error in cache ");
			break;
		case 7:
			p += sprintf(p, "Error in one or more functional units ");
			break;
		case 8:
			p += sprintf(p, "component failed self test ");
			break;
		case 9:
			p += sprintf(p, "Overflow or undervalue of internal queue ");
			break;
		case 17:
			p += sprintf(p, "Virtual address not found on IO-TLB or IO-PDIR ");
			break;
		case 18:
			p += sprintf(p, "Improper access error ");
			break;
		case 19:
			p += sprintf(p, "Access to a memory address which is not mapped to any component ");
			break;
		case 20:
			p += sprintf(p, "Loss of Lockstep ");
			break;
		case 21:
			p += sprintf(p, "Response not associated with a request ");
			break;
		case 22:
			p += sprintf(p, "Bus parity error - must also set the A, C, or D Bits ");
			break;
		case 23:
			p += sprintf(p, "Detection of a PATH_ERROR ");
			break;
		case 25:
			p += sprintf(p, "Bus operation timeout ");
			break;
		case 26:
			p += sprintf(p, "A read was issued to data that has been poisoned ");
			break;
		default:
			p += sprintf(p, "reserved ");
			break;
		}
	}
	if (mem_err->validation_bits & CPER_MEM_VALID_REQUESTOR_ID)
		p += sprintf(p, "requestorID: 0x%016llx ",
			     (long long)mem_err->requestor_id);
	if (mem_err->validation_bits & CPER_MEM_VALID_RESPONDER_ID)
		p += sprintf(p, "responderID: 0x%016llx ",
			     (long long)mem_err->responder_id);
	if (mem_err->validation_bits & CPER_MEM_VALID_TARGET_ID)
		p += sprintf(p, "targetID: 0x%016llx ",
			     (long long)mem_err->responder_id);
	if (p > pvt->other_detail)
		*(p - 1) = '\0';

	/* Sanity-check driver-supplied grain value. */
	if (WARN_ON_ONCE(!e->grain))
		e->grain = 1;

	grain_bits = fls_long(e->grain - 1);

	/* Generate the trace event */
	snprintf(pvt->detail_location, sizeof(pvt->detail_location),
		 "APEI location: %s %s", e->location, e->other_detail);
	trace_mc_event(type, e->msg, e->label, e->error_count,
		       mci->mc_idx, e->top_layer, e->mid_layer, e->low_layer,
		       (e->page_frame_number << PAGE_SHIFT) | e->offset_in_page,
		       grain_bits, e->syndrome, pvt->detail_location);

	/* Report the error via EDAC API */
	edac_raw_mc_handle_error(type, mci, e);
}
EXPORT_SYMBOL_GPL(ghes_edac_report_mem_error);

int ghes_edac_register(struct ghes *ghes, struct device *dev)
{
	bool fake = false;
	int rc, num_dimm = 0;
	struct mem_ctl_info *mci;
	struct edac_mc_layer layers[1];
	struct ghes_edac_pvt *pvt;
	struct ghes_edac_dimm_fill dimm_fill;

	/* Get the number of DIMMs */
	dmi_walk(ghes_edac_count_dimms, &num_dimm);

	/* Check if we've got a bogus BIOS */
	if (num_dimm == 0) {
		fake = true;
		num_dimm = 1;
	}

	layers[0].type = EDAC_MC_LAYER_ALL_MEM;
	layers[0].size = num_dimm;
	layers[0].is_virt_csrow = true;

	/*
	 * We need to serialize edac_mc_alloc() and edac_mc_add_mc(),
	 * to avoid duplicated memory controller numbers
	 */
	mutex_lock(&ghes_edac_lock);
	mci = edac_mc_alloc(ghes_edac_mc_num, ARRAY_SIZE(layers), layers,
			    sizeof(*pvt));
	if (!mci) {
		pr_info("Can't allocate memory for EDAC data\n");
		mutex_unlock(&ghes_edac_lock);
		return -ENOMEM;
	}

	pvt = mci->pvt_info;
	memset(pvt, 0, sizeof(*pvt));
	list_add_tail(&pvt->list, &ghes_reglist);
	pvt->ghes = ghes;
	pvt->mci  = mci;
	mci->pdev = dev;

	mci->mtype_cap = MEM_FLAG_EMPTY;
	mci->edac_ctl_cap = EDAC_FLAG_NONE;
	mci->edac_cap = EDAC_FLAG_NONE;
	mci->mod_name = "ghes_edac.c";
	mci->mod_ver = GHES_EDAC_REVISION;
	mci->ctl_name = "ghes_edac";
	mci->dev_name = "ghes";

	if (!ghes_edac_mc_num) {
		if (!fake) {
			pr_info("This EDAC driver relies on BIOS to enumerate memory and get error reports.\n");
			pr_info("Unfortunately, not all BIOSes reflect the memory layout correctly.\n");
			pr_info("So, the end result of using this driver varies from vendor to vendor.\n");
			pr_info("If you find incorrect reports, please contact your hardware vendor\n");
			pr_info("to correct its BIOS.\n");
			pr_info("This system has %d DIMM sockets.\n",
				num_dimm);
		} else {
			pr_info("This system has a very crappy BIOS: It doesn't even list the DIMMS.\n");
			pr_info("Its SMBIOS info is wrong. It is doubtful that the error report would\n");
			pr_info("work on such system. Use this driver with caution\n");
		}
	}

	if (!fake) {
		/*
		 * Fill DIMM info from DMI for the memory controller #0
		 *
		 * Keep it in blank for the other memory controllers, as
		 * there's no reliable way to properly credit each DIMM to
		 * the memory controller, as different BIOSes fill the
		 * DMI bank location fields on different ways
		 */
		if (!ghes_edac_mc_num) {
			dimm_fill.count = 0;
			dimm_fill.mci = mci;
			dmi_walk(ghes_edac_dmidecode, &dimm_fill);
		}
	} else {
		struct dimm_info *dimm = EDAC_DIMM_PTR(mci->layers, mci->dimms,
						       mci->n_layers, 0, 0, 0);

		dimm->nr_pages = 1;
		dimm->grain = 128;
		dimm->mtype = MEM_UNKNOWN;
		dimm->dtype = DEV_UNKNOWN;
		dimm->edac_mode = EDAC_SECDED;
	}

	rc = edac_mc_add_mc(mci);
	if (rc < 0) {
		pr_info("Can't register at EDAC core\n");
		edac_mc_free(mci);
		mutex_unlock(&ghes_edac_lock);
		return -ENODEV;
	}

	ghes_edac_mc_num++;
	mutex_unlock(&ghes_edac_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(ghes_edac_register);

void ghes_edac_unregister(struct ghes *ghes)
{
	struct mem_ctl_info *mci;
	struct ghes_edac_pvt *pvt, *tmp;

	list_for_each_entry_safe(pvt, tmp, &ghes_reglist, list) {
		if (ghes == pvt->ghes) {
			mci = pvt->mci;
			edac_mc_del_mc(mci->pdev);
			edac_mc_free(mci);
			list_del(&pvt->list);
		}
	}
}
EXPORT_SYMBOL_GPL(ghes_edac_unregister);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   