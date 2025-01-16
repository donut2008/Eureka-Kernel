ce SMI command reg (8b) */
#define E752X_SYSBUS_FERR	0x60	/* System buss first error reg (16b) */
#define E752X_SYSBUS_NERR	0x62	/* System buss next error reg (16b) */
#define E752X_SYSBUS_ERRMASK	0x64	/* System buss error mask reg (16b) */
#define E752X_SYSBUS_SMICMD	0x6A	/* System buss SMI command reg (16b) */
#define E752X_BUF_FERR		0x70	/* Memory buffer first error reg (8b) */
#define E752X_BUF_NERR		0x72	/* Memory buffer next error reg (8b) */
#define E752X_BUF_ERRMASK	0x74	/* Memory buffer error mask reg (8b) */
#define E752X_BUF_SMICMD	0x7A	/* Memory buffer SMI cmd reg (8b) */
#define E752X_DRAM_FERR		0x80	/* DRAM first error register (16b) */
#define E752X_DRAM_NERR		0x82	/* DRAM next error register (16b) */
#define E752X_DRAM_ERRMASK	0x84	/* DRAM error mask register (8b) */
#define E752X_DRAM_SMICMD	0x8A	/* DRAM SMI command register (8b) */
#define E752X_DRAM_RETR_ADD	0xAC	/* DRAM Retry address register (32b) */
#define E752X_DRAM_SEC1_ADD	0xA0	/* DRAM first correctable memory */
					/*     error address register (32b) */
					/*
					 * 31    Reserved
					 * 30:2  CE address (64 byte block 34:6
					 * 1     Reserved
					 * 0     HiLoCS
					 */
#define E752X_DRAM_SEC2_ADD	0xC8	/* DRAM first correctable memory */
					/*     error address register (32b) */
					/*
					 * 31    Reserved
					 * 30:2  CE address (64 byte block 34:6)
					 * 1     Reserved
					 * 0     HiLoCS
					 */
#define E752X_DRAM_DED_ADD	0xA4	/* DRAM first uncorrectable memory */
					/*     error address register (32b) */
					/*
					 * 31    Reserved
					 * 30:2  CE address (64 byte block 34:6)
					 * 1     Reserved
					 * 0     HiLoCS
					 */
#define E752X_DRAM_SCRB_ADD	0xA8	/* DRAM 1st uncorrectable scrub mem */
					/*     error address register (32b) */
					/*
					 * 31    Reserved
					 * 30:2  CE address (64 byte block 34:6
					 * 1     Reserved
					 * 0     HiLoCS
					 */
#define E752X_DRAM_SEC1_SYNDROME 0xC4	/* DRAM first correctable memory */
					/*     error syndrome register (16b) */
#define E752X_DRAM_SEC2_SYNDROME 0xC6	/* DRAM second correctable memory */
					/*     error syndrome register (16b) */
#define E752X_DEVPRES1		0xF4	/* Device Present 1 register (8b) */

/* 3100 IMCH specific register addresses - device 0 function 1 */
#define I3100_NSI_FERR		0x48	/* NSI first error reg (32b) */
#define I3100_NSI_NERR		0x4C	/* NSI next error reg (32b) */
#define I3100_NSI_SMICMD	0x54	/* NSI SMI command register (32b) */
#define I3100_NSI_EMASK		0x90	/* NSI error mask register (32b) */

/* ICH5R register addresses - device 30 function 0 */
#define ICH5R_PCI_STAT		0x06	/* PCI status register (16b) */
#define ICH5R_PCI_2ND_STAT	0x1E	/* PCI status secondary reg (16b) */
#define ICH5R_PCI_BRIDGE_CTL	0x3E	/* PCI bridge control register (16b) */

enum e752x_chips {
	E7520 = 0,
	E7525 = 1,
	E7320 = 2,
	I3100 = 3
};

/*
 * Those chips Support single-rank and dual-rank memories only.
 *
 * On e752x chips, the odd rows are present only on dual-rank memories.
 * Dividing the rank by two will provide the dimm#
 *
 * i3100 MC has a different mapping: it supports only 4 ranks.
 *
 * The mapping is (from 1 to n):
 *	slot	   single-ranked	double-ranked
 *	dimm #1 -> rank #4		NA
 *	dimm #2 -> rank #3		NA
 *	dimm #3 -> rank #2		Ranks 2 and 3
 *	dimm #4 -> rank $1		Ranks 1 and 4
 *
 * FIXME: The current mapping for i3100 considers that it supports up to 8
 *	  ranks/chanel, but datasheet says that the MC supports only 4 ranks.
 */

struct e752x_pvt {
	struct pci_dev *dev_d0f0;
	struct pci_dev *dev_d0f1;
	u32 tolm;
	u32 remapbase;
	u32 remaplimit;
	int mc_symmetric;
	u8 map[8];
	int map_type;
	const struct e752x_dev_info *dev_info;
};

struct e752x_dev_info {
	u16 err_dev;
	u16 ctl_dev;
	const char *ctl_name;
};

struct e752x_error_info {
	u32 ferr_global;
	u32 nerr_global;
	u32 nsi_ferr;	/* 3100 only */
	u32 nsi_nerr;	/* 3100 only */
	u8 hi_ferr;	/* all but 3100 */
	u8 hi_nerr;	/* all but 3100 */
	u16 sysbus_ferr;
	u16 sysbus_nerr;
	u8 buf_ferr;
	u8 buf_nerr;
	u16 dram_ferr;
	u16 dram_nerr;
	u32 dram_sec1_add;
	u32 dram_sec2_add;
	u16 dram_sec1_syndrome;
	u16 dram_sec2_syndrome;
	u32 dram_ded_add;
	u32 dram_scrb_add;
	u32 dram_retr_add;
};

static const struct e752x_dev_info e752x_devs[] = {
	[E7520] = {
		.err_dev = PCI_DEVICE_ID_INTEL_7520_1_ERR,
		.ctl_dev = PCI_DEVICE_ID_INTEL_7520_0,
		.ctl_name = "E7520"},
	[E7525] = {
		.err_dev = PCI_DEVICE_ID_INTEL_7525_1_ERR,
		.ctl_dev = PCI_DEVICE_ID_INTEL_7525_0,
		.ctl_name = "E7525"},
	[E7320] = {
		.err_dev = PCI_DEVICE_ID_INTEL_7320_1_ERR,
		.ctl_dev = PCI_DEVICE_ID_INTEL_7320_0,
		.ctl_name = "E7320"},
	[I3100] = {
		.err_dev = PCI_DEVICE_ID_INTEL_3100_1_ERR,
		.ctl_dev = PCI_DEVICE_ID_INTEL_3100_0,
		.ctl_name = "3100"},
};

/* Valid scrub rates for the e752x/3100 hardware memory scrubber. We
 * map the scrubbing bandwidth to a hardware register value. The 'set'
 * operation finds the 'matching or higher value'.  Note that scrubbing
 * on the e752x can only be enabled/disabled.  The 3100 supports
 * a normal and fast mode.
 */

#define SDRATE_EOT 0xFFFFFFFF

struct scrubrate {
	u32 bandwidth;	/* bandwidth consumed by scrubbing in bytes/sec */
	u16 scrubval;	/* register value for scrub rate */
};

/* Rate below assumes same performance as i3100 using PC3200 DDR2 in
 * normal mode.  e752x bridges don't support choosing normal or fast mode,
 * so the scrubbing bandwidth value isn't all that important - scrubbing is
 * either on or off.
 */
static const struct scrubrate scrubrates_e752x[] = {
	{0,		0x00},	/* Scrubbing Off */
	{500000,	0x02},	/* Scrubbing On */
	{SDRATE_EOT,	0x00}	/* End of Table */
};

/* Fast mode: 2 GByte PC3200 DDR2 scrubbed in 33s = 63161283 bytes/s
 * Normal mode: 125 (32000 / 256) times slower than fast mode.
 */
static const struct scrubrate scrubrates_i3100[] = {
	{0,		0x00},	/* Scrubbing Off */
	{500000,	0x0a},	/* Normal mode - 32k clocks */
	{62500000,	0x06},	/* Fast mode - 256 clocks */
	{SDRATE_EOT,	0x00}	/* End of Table */
};

static unsigned long ctl_page_to_phys(struct mem_ctl_info *mci,
				unsigned long page)
{
	u32 remap;
	struct e752x_pvt *pvt = (struct e752x_pvt *)mci->pvt_info;

	edac_dbg(3, "\n");

	if (page < pvt->tolm)
		return page;

	if ((page >= 0x100000) && (page < pvt->remapbase))
		return page;

	remap = (page - pvt->tolm) + pvt->remapbase;

	if (remap < pvt->remaplimit)
		return remap;

	e752x_printk(KERN_ERR, "Invalid page %lx - out of range\n", page);
	return pvt->tolm - 1;
}

static void do_process_ce(struct mem_ctl_info *mci, u16 error_one,
			u32 sec1_add, u16 sec1_syndrome)
{
	u32 page;
	int row;
	int channel;
	int i;
	struct e752x_pvt *pvt = (struct e752x_pvt *)mci->pvt_info;

	edac_dbg(3, "\n");

	/* convert the addr to 4k page */
	page = sec1_add >> (PAGE_SHIFT - 4);

	/* FIXME - check for -1 */
	if (pvt->mc_symmetric) {
		/* chip select are bits 14 & 13 */
		row = ((page >> 1) & 3);
		e752x_printk(KERN_WARNING,
			"Test row %d Table %d %d %d %d %d %d %d %d\n", row,
			pvt->map[0], pvt->map[1], pvt->map[2], pvt->map[3],
			pvt->map[4], pvt->map[5], pvt->map[6],
			pvt->map[7]);

		/* test for channel remapping */
		for (i = 0; i < 8; i++) {
			if (pvt->map[i] == row)
				break;
		}

		e752x_printk(KERN_WARNING, "Test computed row %d\n", i);

		if (i < 8)
			row = i;
		else
			e752x_mc_printk(mci, KERN_WARNING,
					"row %d not found in remap table\n",
					row);
	} else
		row = edac_mc_find_csrow_by_page(mci, page);

	/* 0 = channel A, 1 = channel B */
	channel = !(error_one & 1);

	/* e752x mc reads 34:6 of the DRAM li