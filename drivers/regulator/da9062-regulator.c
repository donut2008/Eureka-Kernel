, "\n");

	if (error_one & 0x0202) {
		error_2b = ded_add;

		/* convert to 4k address */
		block_page = error_2b >> (PAGE_SHIFT - 4);

		row = pvt->mc_symmetric ?
		/* chip select are bits 14 & 13 */
			((block_page >> 1) & 3) :
			edac_mc_find_csrow_by_page(mci, block_page);

		/* e752x mc reads 34:6 of the DRAM linear address */
		edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1,
					block_page,
					offset_in_page(error_2b << 4), 0,
					 row, -1, -1,
					"e752x UE from Read", "");

	}
	if (error_one & 0x0404) {
		error_2b = scrb_add;

		/* convert to 4k address */
		block_page = error_2b >> (PAGE_SHIFT - 4);

		row = pvt->mc_symmetric ?
		/* chip select are bits 14 & 13 */
			((block_page >> 1) & 3) :
			edac_mc_find_csrow_by_page(mci, block_page);

		/* e752x mc reads 34:6 of the DRAM linear address */
		edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1,
					block_page,
					offset_in_page(error_2b << 4), 0,
					row, -1, -1,
					"e752x UE from Scruber", "");
	}
}

static inline void process_ue(struct mem_ctl_info *mci, u16 error_one,
			u32 ded_add, u32 scrb_add, int *error_found,
			int handle_error)
{
	*error_found = 1;

	if (handle_error)
		do_process_ue(mci, error_one, ded_add, scrb_add);
}

static inline void process_ue_no_info_wr(struct mem_ctl_info *mci,
					 int *error_found, int handle_error)
{
	*error_found = 1;

	if (!handle_error)
		return;

	edac_dbg(3, "\n");
	edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1, 0, 0, 0,
			     -1, -1, -1,
			     "e752x UE log memory write", "");
}

static void do_process_ded_retry(struct mem_ctl_info *mci, u16 error,
				 u32 retry_add)
{
	u32 error_1b, page;
	int row;
	struct e752x_pvt *pvt = (struct e752x_pvt *)mci->pvt_info;

	error_1b = retry_add;
	page = error_1b >> (PAGE_SHIFT - 4);  /* convert the addr to 4k page */

	/* chip select are bits 14 & 13 */
	row = pvt->mc_symmetric ? ((page >> 1) & 3) :
		edac_mc_find_csrow_by_page(mci, page);

	e752x_mc_printk(mci, KERN_WARNING,
			"CE page 0x%lx, row %d : Memory read retry\n",
			(long unsigned int)page, row);
}

static inline void process_ded_retry(struct mem_ctl_info *mci, u16 error,
				u32 retry_add, int *error_found,
				int handle_error)
{
	*error_found = 1;

	if (handle_error)
		do_process_ded_retry(mci, error, retry_add);
}

static inline void process_threshold_ce(struct mem_ctl_info *mci, u16 error,
					int *error_found, int handle_error)
{
	*error_found = 1;

	if (handle_error)
		e752x_mc_printk(mci, KERN_WARNING, "Memory threshold CE\n");
}

static char *global_message[11] = {
	"PCI Express C1",
	"PCI Express C",
	"PCI Express B1",
	"PCI Express B",
	"PCI Express A1",
	"PCI Express A",
	"DMA Controller",
	"HUB or NS Interface",
	"System Bus",
	"DRAM Controller",  /* 9th entry */
	"Internal Buffer"
};

#define DRAM_ENTRY	9

static char *fatal_message[2] = { "Non-Fatal ", "Fatal " };

static void do_global_error(int fatal, u32 errors)
{
	int i;

	for (i = 0; i < 11; i++) {
		if (errors & (1 << i)) {
			/* If the error is from DRAM Controller OR
			 * we are to report ALL errors, then
			 * report the error
			 */
			if ((i == DRAM_ENTRY) || report_non_memory_errors)
				e752x_printk(KERN_WARNING, "%sError %s\n",
					fatal_message[fatal],
					global_message[i]);
		}
	}
}

static inline void global_error(int fatal, u32 errors, int *error_found,
				int handle_error)
{
	*error_found = 1;

	if (handle_error)
		do_global_error(fatal, errors);
}

static char *hub_message[7] = {
	"HI Address or Command Parity", "HI Illegal Access",
	"HI Internal Parity", "Out of Range Access",
	"HI Data Parity", "Enhanced Config Access",
	"Hub Interface Target Abort"
};

static void do_hub_error(int fatal, u8 errors)
{
	int i;

	for (i = 0; i < 7; i++) {
		if (errors & (1 << i))
			e752x_printk(KERN_WARNING, "%sError %s\n",
				fatal_message[fatal], hub_message[i]);
	}
}

static inline void hub_error(int fatal, u8 errors, int *error_found,
			int handle_error)
{
	*error_found = 1;

	if (handle_error)
		do_hub_error(fatal, errors);
}

#define NSI_FATAL_MASK		0x0c080081
#define NSI_NON_FATAL_MASK	0x23a0ba64
#define NSI_ERR_MASK		(NSI_FATAL_MASK | NSI_NON_FATAL_MASK)

static char *nsi_message[30] = {
	"NSI Link Down",	/* NSI_FERR/NSI_NERR bit 0, fatal error */
	"",						/* reserved */
	"NSI Parity Error",				/* bit 2, non-fatal */
	"",						/* reserved */
	"",						/* reserved */
	"Correctable Error Message",			/* bit 5, non-fatal */
	"Non-Fatal Error Message",			/* bit 6, non-fatal */
	"Fatal Error Message",				/* bit 7, fatal */
	"",						/* reserved */
	"Receiver Error",				/* bit 9, non-fatal */
	"",						/* reserved */
	"Bad TLP",					/* bit 11, non-fatal */
	"Bad DLLP",					/* bit 12, non-fatal */
	"REPLAY_NUM Rollover",				/* bit 13, non-fatal */
	"",						/* reserved */
	"Replay Timer Timeout",				/* bit 15, non-fatal */
	"",						/* reserved */
	"",						/* reserved */
	"",						/* reserved */
	"Data Link Protocol Error",			/* bit 19, fatal */
	"",						/* reserved */
	"Poisoned TLP",					/* bit 21, non-fatal */
	"",						/* reserved */
	"Completion Timeout",				/* bit 23, non-fatal */
	"Completer Abort",				/* bit 24, non-fatal */
	"Unexpected Completion",			/* bit 25, non-fatal */
	"Receiver Overflow",				/* bit 26, fatal */
	"Malformed TLP",				/* bit 27, fatal */
	"",						/* reserved */
	"Unsupported Request"				/* bit 29, non-fatal */
};

static void do_nsi_error(int fatal, u32 errors)
{
	int i;

	for (i = 0; i < 30; i++) {
		if (errors & (1 << i))
			printk(KERN_WARNING "%sError %s\n",
			       fatal_message[fatal], nsi_message[i]);
	}
}

static inline void nsi_error(int fatal, u32 errors, int *error_found,
		int handle_error)
{
	*error_found = 1;

	if (handle_error)
		do_nsi_error(fatal, errors);
}

static char *membuf_message[4] = {
	"Internal PMWB to DRAM parity",
	"Internal PMWB to System Bus Parity",
	"Internal System Bus or IO to PMWB Parity",
	"Internal DRAM to PMWB Parity"
};

static void do_membuf_error(u8 errors)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (errors & (1 << i))
			e752x_printk(KERN_WARNING, "Non-Fatal Error %s\n",
				membuf_message[i]);
	}
}

static inline void membuf_error(u8 errors, int *error_found, int handle_error)
{
	*error_found = 1;

	if (handle_error)
		do_membuf_error(errors);
}

static char *sysbus_message[10] = {
	"Addr or Request Parity",
	"Data Strobe Glitch",
	"Addr Strobe Glitch",
	"Data Parity",
	"Addr Above TOM",
	"Non DRAM Lock Error",
	"MCERR", "BINIT",
	"Memory Parity",
	"IO Subsystem Parity"
};

static void do_sysbus_error(int fatal, u32 errors)
{
	int i;

	for (i = 0; i < 10; i++) {
		if (errors & (1 << i))
			e752x_printk(KERN_WARNING, "%sError System Bus %s\n",
				fatal_message[fatal], sysbus_message[i]);
	}
}

static inline void sysbus_error(int fatal, u32 errors, int *error_found,
				int handle_error)
{
	*error_found = 1;

	if (handle_error)
		do_sysbus_error(fatal, errors);
}

static void e752x_check_hub_interface(struct e752x_error_info *info,
				int *error_found, int handle_error)
{
	u8 stat8;

	//pci_read_config_byte(dev,E752X_HI_FERR,&stat8);

	stat8 = info->hi_ferr;

	if (stat8 & 0x7f) {	/* Error, so process */
		stat8 &= 0x7f;

		if (stat8 & 0x2b)
			hub_error(1, stat8 & 0x2b, error_found, handle_error);

		if (stat8 & 0x54)
			hub_error(0, stat8 & 0x54, error_found, handle_error);
	}
	//pci_read_config_byte(dev,E752X_HI_NERR,&stat8);

	stat8 = info->hi_nerr;

	if (stat8 & 0x7f) {	/* Error, so process */
		stat8 &= 0x7f;

		if (stat8 & 0x2b)
			hub_error(1, stat8 & 0x2b, error_found, handle_error);

		if (stat8 & 0x54)
			hub_error(0, stat8 & 0x54, error_found, handle_error);
	}
}

static void e752x_check_ns_interface(struct e752x_error_info *info,
				int *error_found, int handle_error)
{
	u32 stat32;

	stat32 = info->nsi_ferr;
	if (stat32 & NSI_ERR_MASK) { /* Error, so process */
		if (stat32 & NSI_FATAL_MASK)	/* check for fatal errors */
			nsi_error(1, stat32 & NSI_FATAL_MASK, error_found,
				  handle_error);
		if (stat32 & NSI_NON_FATAL_MASK) /* check for non-fatal ones */
			nsi_error(0, stat32 & NSI_NON_FATAL_MASK, error_found,
				  handle_error);
	}
	stat32 = info->nsi_nerr;
	if (stat32 & NSI_ERR_MASK) {
		if (stat32 & NSI_FATAL_MASK)
			nsi_error(1, stat32 & NSI_FATAL_MASK, error_found,
				  handle_error);
		if (stat32 & NSI_NON_FATAL_MASK)
			nsi_error(0, stat32 & NSI_NON_FATAL_MASK, error_found,
				  handle_error);
	}
}

static void e752x_check_sysbus(struct e752x_error_info *info,
			int *error_found, int handle_error)
{
	u32 stat32, error32;

	//pci_read_config_dword(dev,E752X_SYSBUS_FERR,&stat32);
	stat32 = info->sysbus_ferr + (info->sysbus_nerr << 16);

	if (stat32 == 0)
		return;		/* no errors */

	error32 = (stat32 >> 16) & 0x3ff;
	stat32 = stat32 & 0x3ff;

	if (stat32 & 0x087)
		sysbus_error(1, stat32 & 0x087, error_found, handle_error);

	if (stat32 & 0x378)
		sysbus_error(0, stat32 & 0x378, error_found, handle_error);

	if (error32 & 0x087)
		sysbus_error(1, error32 & 0x087, error_found, handle_error);

	if (error32 & 0x378)
		sysbus_error(0, error32 & 0x378, error_found, handle_error);
}

static void e752x_check_membuf(struct e752x_error_info *info,
			int *error_found, int handle_error)
{
	u8 stat8;

	stat8 = info->buf_ferr;

	if (stat8 & 0x0f) {	/* Error, so process */
		stat8 &= 0x0f;
		membuf_error(stat8, error_found, handle_error);
	}

	stat8 = info->buf_nerr;

	if (stat8 & 0x0f) {	/* Error, so process */
		stat8 &= 0x0f;
		membuf_error(stat8, error_found, handle_error);
	}
}

static void e752x_check_dram(struct mem_ctl_info *mci,
			struct e752x_error_info *info, int *error_found,
			int handle_error)
{
	u16 error_one, error_next;

	error_one = info->dram_ferr;
	error_next = info->dram_nerr;

	/* decode and report errors */
	if (error_one & 0x0101)	/* check first error correctable */
		process_ce(mci, error_one, info->dram_sec1_add,
			info->dram_sec1_syndrome, error_found, handle_error);

	if (error_next & 0x0101)	/* check next error correctable */
		process_ce(mci, error_next, info->dram_sec2_add,
			info->dram_sec2_syndrome, error_found, handle_error);

	if (error_one & 0x4040)
		process_ue_no_info_wr(mci, error_found, handle_error);

	if (error_next & 0x4040)
		process_ue_no_info_wr(mci, error_found, handle_error);

	if (error_one & 0x2020)
		process_ded_retry(mci, error_one, info->dram_retr_add,
				error_found, handle_error);

	if (error_next & 0x2020)
		process_ded_retry(mci, error_next, info->dram_retr_add,
				error_found, handle_error);

	if (error_one & 0x0808)
		process_threshold_ce(mci, error_one, error_found, handle_error);

	if (error_next & 0x0808)
		process_threshold_ce(mci, error_next, error_found,
				handle_error);

	if (error_one & 0x0606)
		process_ue(mci, error_one, info->dram_ded_add,
			info->dram_scrb_add, error_found, handle_error);

	if (error_next & 0x0606)
		process_ue(mci, error_next, info->dram_ded_add,
			info->dram_scrb_add, error_found, handle_error);
}

static void e752x_get_error_info(struct mem_ctl_info *mci,
				 struct e752x_error_info *info)
{
	struct pci_dev *dev;
	struct e752x_pvt *pvt;

	memset(info, 0, sizeof(*info));
	pvt = (struct e752x_pvt *)mci->pvt_info;
	dev = pvt->dev_d0f1;
	pci_read_config_dword(dev, E752X_FERR_GLOBAL, &info->ferr_global);

	if (info->ferr_global) {
		if (pvt->dev_info->err_dev == PCI_DEVICE_ID_INTEL_3100_1_ERR) {
			pci_read_config_dword(dev, I3100_NSI_FERR,
					     &info->nsi_ferr);
			info->hi_ferr = 0;
		} else {
			pci_read_config_byte(dev, E752X_HI_FERR,
					     &info->hi_ferr);
			info->nsi_ferr = 0;
		}
		pci_read_config_word(dev, E752X_SYSBUS_FERR,
				&info->sysbus_ferr);
		pci_read_config_byte(dev, E752X_BUF_FERR, &info->buf_ferr);
		pci_read_config_word(dev, E752X_DRAM_FERR, &info->dram_ferr);
		pci_read_config_dword(dev, E752X_DRAM_SEC1_ADD,
				&info->dram_sec1_add);
		pci_read_config_word(dev, E752X_DRAM_SEC1_SYNDROME,
				&info->dram_sec1_syndrome);
		pci_read_config_dword(dev, E752X_DRAM_DED_ADD,
				&info->dram_ded_add);
		pci_read_config_dword(dev, E752X_DRAM_SCRB_ADD,
				&info->dram_scrb_add);
		pci_read_config_dword(dev, E752X_DRAM_RETR_ADD,
				&info->dram_retr_add);

		/* ignore the reserved bits just in case */
		if (info->hi_ferr & 0x7f)
			pci_write_config_byte(dev, E752X_HI_FERR,
					info->hi_ferr);

		if (info->nsi_ferr & NSI_ERR_MASK)
			pci_write_config_dword(dev, I3100_NSI_FERR,
					info->nsi_ferr);

		if (info->sysbus_ferr)
			pci_write_config_word(dev, E752X_SYSBUS_FERR,
					info->sysbus_ferr);

		if (info->buf_ferr & 0x0f)
			pci_write_config_byte(dev, E752X_BUF_FERR,
					info->buf_ferr);

		if (info->dram_ferr)
			pci_write_bits16(pvt->dev_d0f1, E752X_DRAM_FERR,
					 info->dram_ferr, info->dram_ferr);

		pci_write_config_dword(dev, E752X_FERR_GLOBAL,
				info->ferr_global);
	}

	pci_read_config_dword(dev, E752X_NERR_GLOBAL, &info->nerr_global);

	if (info->nerr_global) {
		if (pvt->dev_info->err_dev == PCI_DEVICE_ID_INTEL_3100_1_ERR) {
			pci_read_config_dword(dev, I3100_NSI_NERR,
					     &info->nsi_nerr);
			info->hi_nerr = 0;
		} else {
			pci_read_config_byte(dev, E752X_HI_NERR,
					     &info->hi_nerr);
			info->nsi_nerr = 0;
		}
		pci_read_config_word(dev, E752X_SYSBUS_NERR,
				&info->sysbus_nerr);
		pci_read_config_byte(dev, E752X_BUF_NERR, &info->buf_nerr);
		pci_read_config_word(dev, E752X_DRAM_NERR, &info->dram_nerr);
		pci_read_config_dword(dev, E752X_DRAM_SEC2_ADD,
				&info->dram_sec2_add);
		pci_read_config_word(dev, E752X_DRAM_SEC2_SYNDROME,
				&info->dram_sec2_syndrome);

		if (info->hi_nerr & 0x7f)
			pci_write_config_byte(dev, E752X_HI_NERR,
					info->hi_nerr);

		if (info->nsi_nerr & NSI_ERR_MASK)
			pci_write_config_dword(dev, I3100_NSI_NERR,
					info->nsi_nerr);

		if (info->sysbus_nerr)
			pci_write_config_word(dev, E752X_SYSBUS_NERR,
					info->sysbus_nerr);

		if (info->buf_nerr & 0x0f)
			pci_write_config_byte(dev, E752X_BUF_NERR,
					info->buf_nerr);

		if (info->dram_nerr)
			pci_write_bits16(pvt->dev_d0f1, E752X_DRAM_NERR,
					 info->dram_nerr, info->dram_nerr);

		pci_write_config_dword(dev, E752X_NERR_GLOBAL,
				info->nerr_global);
	}
}

static int e752x_process_error_info(struct mem_ctl_info *mci,
				struct e752x_error_info *info,
				int handle_errors)
{
	u32 error32, stat32;
	int error_found;

	error_found = 0;
	error32 = (info->ferr_global >> 18) & 0x3ff;
	stat32 = (info->ferr_global >> 4) & 0x7ff;

	if (error32)
		global_error(1, error32, &error_found, handle_errors);

	if (stat32)
		global_error(0, stat32, &error_found, handle_errors);

	error32 = (info->nerr_global >> 18) & 0x3ff;
	stat32 = (info->nerr_global >> 4) & 0x7ff;

	if (error32)
		global_error(1, error32, &error_found, handle_errors);

	if (stat32)
		global_error(0, stat32, &error_found, handle_errors);

	e752x_check_hub_interface(info, &error_found, handle_errors);
	e752x_check_ns_interface(info, &error_found, handle_errors);
	e752x_check_sysbus(info, &error_found, handle_errors);
	e752x_check_membuf(info, &error_found, handle_errors);
	e752x_check_dram(mci, info, &error_found, handle_errors);
	return error_found;
}

static void e752x_check(struct mem_ctl_info *mci)
{
	struct e752x_error_info info;

	edac_dbg(3, "\n");
	e752x_get_error_info(mci, &info);
	e752x_process_error_info(mci, &info, 1);
}

/* Program byte/sec bandwidth scrub rate to hardware */
static int set_sdram_scrub_rate(struct mem_ctl_info *mci, u32 new_bw)
{
	const struct scrubrate *scrubrates;
	struct e752x_pvt *pvt = (struct e752x_pvt *) mci->pvt_info;
	struct pci_dev *pdev = pvt->dev_d0f0;
	int i;

	if (pvt->dev_info->ctl_dev == PCI_DEVICE_ID_INTEL_3100_0)
		scrubrates = scrubrates_i3100;
	else
		scrubrates = scrubrates_e752x;

	/* Translate the desired scrub rate to a e752x/3100 register value.
	 * Search for the bandwidth that is equal or greater than the
	 * desired rate and program the cooresponding register value.
	 */
	for (i = 0; scrubrates[i].bandwidth != SDRATE_EOT; i++)
		if (scrubrates[i].bandwidth >= new_bw)
			break;

	if (scrubrates[i].bandwidth == SDRATE_EOT)
		return -1;

	pci_write_config_word(pdev, E752X_MCHSCRB, scrubrates[i].scrubval);

	return scrubrates[i].bandwidth;
}

/* Convert current scrub rate value into byte/sec bandwidth */
static int get_sdram_scrub_rate(struct mem_ctl_info *mci)
{
	const struct scrubrate *scrubrates;
	struct e752x_pvt *pvt = (struct e752x_pvt *) mci->pvt_info;
	struct pci_dev *pdev = pvt->dev_d0f0;
	u16 scrubval;
	int i;

	if (pvt->dev_info->ctl_dev == PCI_DEVICE_ID_INTEL_3100_0)
		scrubrates = scrubrates_i3100;
	else
		scrubrates = scrubrates_e752x;

	/* Find the bandwidth matching the memory scrubber configuration */
	pci_read_config_word(pdev, E752X_MCHSCRB, &scrubval);
	scrubval = scrubval & 0x0f;

	for (i = 0; scrubrates[i].bandwidth != SDRATE_EOT; i++)
		if (scrubrates[i].scrubval == scrubval)
			break;

	if (scrubrates[i].bandwidth == SDRATE_EOT) {
		e752x_printk(KERN_WARNING,
			"Invalid sdram scrub control value: 0x%x\n", scrubval);
		return -1;
	}
	return scrubrates[i].bandwidth;

}

/* Return 1 if dual channel mode is active.  Else return 0. */
static inline int dual_channel_active(u16 ddrcsr)
{
	return (((ddrcsr >> 12) & 3) == 3);
}

/* Remap csrow index numbers if map_type is "reverse"
 */
static inline int remap_csrow_index(struct mem_ctl_info *mci, int index)
{
	struct e752x_pvt *pvt = mci->pvt_info;

	if (!pvt->map_type)
		return (7 - index);

	return (index);
}

static void e752x_init_csrows(struct mem_ctl_info *mci, struct pci_dev *pdev,
			u16 ddrcsr)
{
	struct csrow_info *csrow;
	enum edac_type edac_mode;
	unsigned long last_cumul_size;
	int index, mem_dev, drc_chan;
	int drc_drbg;		/* DRB granularity 0=64mb, 1=128mb */
	int drc_ddim;		/* DRAM Data Integrity Mode 0=none, 2=edac */
	u8 value;
	u32 dra, drc, cumul_size, i, nr_pages;

	dra = 0;
	for (index = 0; index < 4; index++) {
		u8 dra_reg;
		pci_read_config_byte(pdev, E752X_DRA + index, &dra_reg);
		dra |= dra_reg << (index * 8);
	}
	pci_read_config_dword(pdev, E752X_DRC, &drc);
	drc_chan = dual_channel_active(ddrcsr) ? 1 : 0;
	drc_drbg = drc_chan + 1;	/* 128 in dual mode, 64 in single */
	drc_ddim = (drc >> 20) & 0x3;

	/* The dram row boundary (DRB) reg values are boundary address for
	 * each DRAM row with a granularity of 64 or 128MB (single/dual
	 * channel operation).  DRB regs are cumulative; therefore DRB7 will
	 * contain the total memory contained in all eight rows.
	 */
	for (last_cumul_size = index = 0; index < mci->nr_csrows; index++) {
		/* mem_dev 0=x8, 1=x4 */
		mem_dev = (dra >> (index * 4 + 2)) & 0x3;
		csrow = mci->csrows[remap_csrow_index(mci, index)];

		mem_dev = (mem_dev == 2);
		pci_read_config_byte(pdev, E752X_DRB + index, &value);
		/* convert a 128 or 64 MiB DRB to a page size. */
		cumul_size = value << (25 + drc_drbg - PAGE_SHIFT);
		edac_dbg(3, "(%d) cumul_size 0x%x\n", index, cumul_size);
		if (cumul_size == last_cumul_size)
			continue;	/* not populated */

		csrow->first_page = last_cumul_size;
		csrow->last_page = cumul_size - 1;
		nr_pages = cumul_size - last_cumul_size;
		last_cumul_size = cumul_size;

		/*
		* if single channel or x8 devices then SECDED
		* if dual channel and x4 then S4ECD4ED
		*/
		if (drc_ddim) {
			if (drc_chan && mem_dev) {
				edac_mode = EDAC_S4ECD4ED;
				mci->edac_cap |= EDAC_FLAG_S4ECD4ED;
			} else {
				edac_mode = EDAC_SECDED;
				mci->edac_cap |= EDAC_FLAG_SECDED;
			}
		} else
			edac_mode = EDAC_NONE;
		for (i = 0; i < csrow->nr_channels; i++) {
			struct dimm_info *dimm = csrow->channels[i]->dimm;

			edac_dbg(3, "Initializing rank at (%i,%i)\n", index, i);
			dimm->nr_pages = nr_pages / csrow->nr_channels;
			dimm->grain = 1 << 12;	/* 4KiB - resolution of CELOG */
			dimm->mtype = MEM_RDDR;	/* only one type supported */
			dimm->dtype = mem_dev ? DEV_X4 : DEV_X8;
			dimm->edac_mode = edac_mode;
		}
	}
}

static void e752x_init_mem_map_table(struct pci_dev *pdev,
				struct e752x_pvt *pvt)
{
	int index;
	u8 value, last, row;

	last = 0;
	row = 0;

	for (index = 0; index < 8; index += 2) {
		pci_read_config_byte(pdev, E752X_DRB + index, &value);
		/* test if there is a dimm in this slot */
		if (value == last) {
			/* no dimm in the slot, so flag it as empty */
			pvt->map[index] = 0xff;
			pvt->map[index + 1] = 0xff;
		} else {	/* there is a dimm in the slot */
			pvt->map[index] = row;
			row++;
			last = value;
			/* test the next value to see if the dimm is double
			 * sided
			 */
			pci_read_config_byte(pdev, E752X_DRB + index + 1,
					&value);

			/* the dimm is single sided, so flag as empty */
			/* this is a double sided dimm to save the next row #*/
			pvt->map[index + 1] = (value == last) ? 0xff :	row;
			row++;
			last = value;
		}
	}
}

/* Return 0 on success or 1 on failure. */
static int e752x_get_devs(struct pci_dev *pdev, int dev_idx,
			struct e752x_pvt *pvt)
{
	pvt->dev_d0f1 = pci_get_device(PCI_VENDOR_ID_INTEL,
				pvt->dev_info->err_dev, NULL);

	if (pvt->dev_d0f1 == NULL) {
		pvt->dev_d0f1 = pci_scan_single_device(pdev->bus,
							PCI_DEVFN(0, 1));
		pci_dev_get(pvt->dev_d0f1);
	}

	if (pvt->dev_d0f1 == NULL) {
		e752x_printk(KERN_ERR, "error reporting device not found:"
			"vendor %x device 0x%x (broken BIOS?)\n",
			PCI_VENDOR_ID_INTEL, e752x_devs[dev_idx].err_dev);
		return 1;
	}

	pvt->dev_d0f0 = pci_get_device(PCI_VENDOR_ID_INTEL,
				e752x_devs[dev_idx].ctl_dev,
				NULL);

	if (pvt->dev_d0f0 == NULL)
		goto fail;

	return 0;

fail:
	pci_dev_put(pvt->dev_d0f1);
	return 1;
}

/* Setup system bus parity mask register.
 * Sysbus parity supported on:
 * e7320/e7520/e7525 + Xeon
 */
static void e752x_init_sysbus_parity_mask(struct e752x_pvt *pvt)
{
	char *cpu_id = cpu_data(0).x86_model_id;
	struct pci_dev *dev = pvt->dev_d0f1;
	int enable = 1;

	/* Allow module parameter override, else see if CPU supports parity */
	if (sysbus_parity != -1) {
		enable = sysbus_parity;
	} else if (cpu_id[0] && !strstr(cpu_id, "Xeon")) {
		e752x_printk(KERN_INFO, "System Bus Parity not "
			     "supported by CPU, disabling\n");
		enable = 0;
	}

	if (enable)
		pci_write_config_word(dev, E752X_SYSBUS_ERRMASK, 0x0000);
	else
		pci_write_config_word(dev, E752X_SYSBUS_ERRMASK, 0x0309);
}

static void e752x_init_error_reporting_regs(struct e752x_pvt *pvt)
{
	struct pci_dev *dev;

	dev = pvt->dev_d0f1;
	/* Turn off error disable & SMI in case the BIOS turned it on */
	if (pvt->dev_info->err_dev == PCI_DEVICE_ID_INTEL_3100_1_ERR) {
		pci_write_config_dword(dev, I3100_NSI_EMASK, 0);
		pci_write_config_dword(dev, I3100_NSI_SMICMD, 0);
	} else {
		pci_write_config_byte(dev, E752X_HI_ERRMASK, 0x00);
		pci_write_config_byte(dev, E752X_HI_SMICMD, 0x00);
	}

	e752x_init_sysbus_parity_mask(pvt);

	pci_write_config_word(dev, E752X_SYSBUS_SMICMD, 0x00);
	pci_write_config_byte(dev, E752X_BUF_ERRMASK, 0x00);
	pci_write_config_byte(dev, E752X_BUF_SMICMD, 0x00);
	pci_write_config_byte(dev, E752X_DRAM_ERRMASK, 0x00);
	pci_write_config_byte(dev, E752X_DRAM_SMICMD, 0x00);
}

static int e752x_probe1(struct pci_dev *pdev, int dev_idx)
{
	u16 pci_data;
	u8 stat8;
	struct mem_ctl_info *mci;
	struct edac_mc_layer layers[2];
	struct e752x_pvt *pvt;
	u16 ddrcsr;
	int drc_chan;		/* Number of channels 0=1chan,1=2chan */
	struct e752x_error_info discard;

	edac_dbg(0, "mci\n");
	edac_dbg(0, "Starting Probe1\n");

	/* check to see if device 0 function 1 is enabled; if it isn't, we
	 * assume the BIOS has reserved it for a reason and is expecting
	 * exclusive access, we take care not to violate that assumption and
	 * fail the probe. */
	pci_read_config_byte(pdev, E752X_DEVPRES1, &stat8);
	if (!force_function_unhide && !(stat8 & (1 << 5))) {
		printk(KERN_INFO "Contact your BIOS vendor to see if the "
			"E752x error registers can be safely un-hidden\n");
		return -ENODEV;
	}
	stat8 |= (1 << 5);
	pci_write_config_byte(pdev, E752X_DEVPRES1, stat8);

	pci_read_config_word(pdev, E752X_DDRCSR, &ddrcsr);
	/* FIXME: should check >>12 or 0xf, true for all? */
	/* Dual channel = 1, Single channel = 0 */
	drc_chan = dual_channel_active(ddrcsr);

	layers[0].type = EDAC_MC_LAYER_CHIP_SELECT;
	layers[0].size = E752X_NR_CSROWS;
	layers[0].is_virt_csrow = true;
	layers[1].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = drc_chan + 1;
	layers[1].is_virt_csrow = false;
	mci = edac_mc_alloc(0, ARRAY_SIZE(layers), layers, sizeof(*pvt));
	if (mci == NULL)
		return -ENOMEM;

	edac_dbg(3, "init mci\n");
	mci->mtype_cap = MEM_FLAG_RDDR;
	/* 3100 IMCH supports SECDEC only */
	mci->edac_ctl_cap = (dev_idx == I3100) ? EDAC_FLAG_SECDED :
		(EDAC_FLAG_NONE | EDAC_FLAG_SECDED | EDAC_FLAG_S4ECD4ED);
	/* FIXME - what if different memory types are in different csrows? */
	mci->mod_name = EDAC_MOD_STR;
	mci->mod_ver = E752X_REVISION;
	mci->pdev = &pdev->dev;

	edac_dbg(3, "init pvt\n");
	pvt = (struct e752x_pvt *)mci->pvt_info;
	pvt->dev_info = &e752x_devs[dev_idx];
	pvt->mc_symmetric = ((ddrcsr & 0x10) != 0);

	if (e752x_get_devs(pdev, dev_idx, pvt)) {
		edac_mc_free(mci);
		return -ENODEV;
	}

	edac_dbg(3, "more mci init\n");
	mci->ctl_name = pvt->dev_info->ctl_name;
	mci->dev_name = pci_name(pdev);
	mci->edac_check = e752x_check;
	mci->ctl_page_to_phys = ctl_page_to_phys;
	mci->set_sdram_scrub_rate = set_sdram_scrub_rate;
	mci->get_sdram_scrub_