struct e7xxx_dev_info *dev_info;
};

struct e7xxx_dev_info {
	u16 err_dev;
	const char *ctl_name;
};

struct e7xxx_error_info {
	u8 dram_ferr;
	u8 dram_nerr;
	u32 dram_celog_add;
	u16 dram_celog_syndrome;
	u32 dram_uelog_add;
};

static struct edac_pci_ctl_info *e7xxx_pci;

static const struct e7xxx_dev_info e7xxx_devs[] = {
	[E7500] = {
		.err_dev = PCI_DEVICE_ID_INTEL_7500_1_ERR,
		.ctl_name = "E7500"},
	[E7501] = {
		.err_dev = PCI_DEVICE_ID_INTEL_7501_1_ERR,
		.ctl_name = "E7501"},
	[E7505] = {
		.err_dev = PCI_DEVICE_ID_INTEL_7505_1_ERR,
		.ctl_name = "E7505"},
	[E7205] = {
		.err_dev = PCI_DEVICE_ID_INTEL_7205_1_ERR,
		.ctl_name = "E7205"},
};

/* FIXME - is this valid for both SECDED and S4ECD4ED? */
static inline int e7xxx_find_channel(u16 syndrome)
{
	edac_dbg(3, "\n");

	if ((syndrome & 0xff00) == 0)
		return 0;

	if ((syndrome & 0x00ff) == 0)
		return 1;

	if ((syndrome & 0xf000) == 0 || (syndrome & 0x0f00) == 0)
		return 0;

	return 1;
}

static unsigned long ctl_page_to_phys(struct mem_ctl_info *mci,
				unsigned long page)
{
	u32 remap;
	struct e7xxx_pvt *pvt = (struct e7xxx_pvt *)mci->pvt_info;

	edac_dbg(3, "\n");

	if ((page < pvt->tolm) ||
		((page >= 0x100000) && (page < pvt->remapbase)))
		return page;

	remap = (page - pvt->tolm) + pvt->remapbase;

	if (remap < pvt->remaplimit)
		return remap;

	e7xxx_printk(KERN_ERR, "Invalid page %lx - out of range\n", page);
	return pvt->tolm - 1;
}

static void process_ce(struct mem_ctl_info *mci, struct e7xxx_error_info *info)
{
	u32 error_1b, page;
	u16 syndrome;
	int row;
	int channel;

	edac_dbg(3, "\n");
	/* read the error address */
	error_1b = info->dram_celog_add;
	/* FIXME - should use PAGE_SHIFT */
	page = error_1b >> 6;	/* convert the address to 4k page */
	/* read the syndrome */
	syndrome = info->dram_celog_syndrome;
	/* FIXME - check for -1 */
	row = edac_mc_find_csrow_by_page(mci, page);
	/* convert syndrome to channel */
	channel = e7xxx_find_channel(syndrome);
	edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1, page, 0, syndrome,
			     row, channel, -1, "e7xxx CE", "");
}

static void process_ce_no_info(struct mem_ctl_info *mci)
{
	edac_dbg(3, "\n");
	edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1, 0, 0, 0, -1, -1, -1,
			     "e7xxx CE log register overflow", "");
}

static void process_ue(struct mem_ctl_info *mci, struct e7xxx_error_info *info)
{
	u32 error_2b, block_page;
	int row;

	edac_dbg(3, "\n");
	/* read the error address */
	error_2b = info->dram_uelog_add;
	/* FIXME - should use PAGE_SHIFT */
	block_page = error_2b >> 6;	/* convert to 4k address */
	row = edac_mc_find_csrow_by_page(mci, block_page);

	edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1, block_page, 0, 0,
			     row, -1, -1, "e7xxx UE", "");
}

static void process_ue_no_info(struct mem_ctl_info *mci)
{
	edac_dbg(3, "\n");

	edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1, 0, 0, 0, -1, -1, -1,
			     "e7xxx UE log register overflow", "");
}

static void e7xxx_get_error_info(struct mem_ctl_info *mci,
				 struct e7xxx_error_info *info)
{
	struct e7xxx_pvt *pvt;

	pvt = (struct e7xxx_pvt *)mci->pvt_info;
	pci_read_config_byte(pvt->bridge_ck, E7XXX_DRAM_FERR, &info->dram_ferr);
	pci_read_config_byte(pvt->bridge_ck, E7XXX_DRAM_NERR, &info->dram_nerr);

	if ((info->dram_ferr & 1) || (info->dram_nerr & 1)) {
		pci_read_config_dword(pvt->bridge_ck, E7XXX_DRAM_CELOG_ADD,
				&info->dram_celog_add);
		pci_read_config_word(pvt->bridge_ck,
				E7XXX_DRAM_CELOG_SYNDROME,
				&info->dram_celog_syndrome);
	}

	if ((info->dram_ferr & 2) || (info->dram_nerr & 2))
		pci_read_config_dword(pvt->bridge_ck, E7XXX_DRAM_UELOG_ADD,
				&info->dram_uelog_add);

	if (info->dram_ferr & 3)
		pci_write_bits8(pvt->bridge_ck, E7XXX_DRAM_FERR, 0x03, 0x03);

	if (info->dram_nerr & 3)
		pci_write_bits8(pvt->bridge_ck, E7XXX_DRAM_NERR, 0x03, 0x03);
}

static int e7xxx_process_error_info(struct mem_ctl_info *mci,
				struct e7xxx_error_info *info,
				int handle_errors)
{
	int error_found;

	error_found = 0;

	/* decode and report errors */
	if (info->dram_ferr & 1) {	/* check first error correctable */
		error_found = 1;

		if (handle_errors)
			process_ce(mci, info);
	}

	if (info->dram_ferr & 2) {	/* check first error uncorrectable */
		error_found = 1;

		if (handle_errors)
			process_ue(mci, info);
	}

	if (info->dram_nerr & 1) {	/* check next error correctable */
		error_found = 1;

		if (handle_errors) {
			if (info->dram_ferr & 1)
				process_ce_no_info(mci);
			else
				process_ce(mci, info);
		}
	}

	if (info->dram_nerr & 2) {	/* check next error uncorrectable */
		error_found = 1;

		if (handle_errors) {
			if (info->dram_ferr & 2)
				process_ue_no_info(mci);
			else
				process_ue(mci, info);
		}
	}

	return error_found;
}

static void e7xxx_check(struct mem_ctl_info *mci)
{
	struct 