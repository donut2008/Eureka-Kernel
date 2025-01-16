	return mc >> 5 & 1;
}

static inline u32 i5100_mc_scrbdone(u32 mc)
{
	return mc >> 4 & 1;
}

static inline u16 i5100_spddata_rdo(u16 a)
{
	return a >> 15 & 1;
}

static inline u16 i5100_spddata_sbe(u16 a)
{
	return a >> 13 & 1;
}

static inline u16 i5100_spddata_busy(u16 a)
{
	return a >> 12 & 1;
}

static inline u16 i5100_spddata_data(u16 a)
{
	return a & ((1 << 8) - 1);
}

static inline u32 i5100_spdcmd_create(u32 dti, u32 ckovrd, u32 sa, u32 ba,
				      u32 data, u32 cmd)
{
	return	((dti & ((1 << 4) - 1))  << 28) |
		((ckovrd & 1)            << 27) |
		((sa & ((1 << 3) - 1))   << 24) |
		((ba & ((1 << 8) - 1))   << 16) |
		((data & ((1 << 8) - 1)) <<  8) |
		(cmd & 1);
}

static inline u16 i5100_tolm_tolm(u16 a)
{
	return a >> 12 & ((1 << 4) - 1);
}

static inline u16 i5100_mir_limit(u16 a)
{
	return a >> 4 & ((1 << 12) - 1);
}

static inline u16 i5100_mir_way1(u16 a)
{
	return a >> 1 & 1;
}

static inline u16 i5100_mir_way0(u16 a)
{
	return a & 1;
}

static inline u32 i5100_ferr_nf_mem_chan_indx(u32 a)
{
	return a >> 28 & 1;
}

static inline u32 i5100_ferr_nf_mem_any(u32 a)
{
	return a & I5100_FERR_NF_MEM_ANY_MASK;
}

static inline u32 i5100_nerr_nf_mem_any(u32 a)
{
	return i5100_ferr_nf_mem_any(a);
}

static inline u32 i5100_dmir_limit(u32 a)
{
	return a >> 16 & ((1 << 11) - 1);
}

static inline u32 i5100_dmir_rank(u32 a, u32 i)
{
	return a >> (4 * i) & ((1 << 2) - 1);
}

static inline u16 i5100_mtr_present(u16 a)
{
	return a >> 10 & 1;
}

static inline u16 i5100_mtr_ethrottle(u16 a)
{
	return a >> 9 & 1;
}

static inline u16 i5100_mtr_width(u16 a)
{
	return a >> 8 & 1;
}

static inline u16 i5100_mtr_numbank(u16 a)
{
	return a >> 6 & 1;
}

static inline u16 i5100_mtr_numrow(u16 a)
{
	return a >> 2 & ((1 << 2) - 1);
}

static inline u16 i5100_mtr_numcol(u16 a)
{
	return a & ((1 << 2) - 1);
}


static inline u32 i5100_validlog_redmemvalid(u32 a)
{
	return a >> 2 & 1;
}

static inline u32 i5100_validlog_recmemvalid(u32 a)
{
	return a >> 1 & 1;
}

static inline u32 i5100_validlog_nrecmemvalid(u32 a)
{
	return a & 1;
}

static inline u32 i5100_nrecmema_merr(u32 a)
{
	return a >> 15 & ((1 << 5) - 1);
}

static inline u32 i5100_nrecmema_bank(u32 a)
{
	return a >> 12 & ((1 << 3) - 1);
}

static inline u32 i5100_nrecmema_rank(u32 a)
{
	return a >>  8 & ((1 << 3) - 1);
}

static inline u32 i5100_nrecmema_dm_buf_id(u32 a)
{
	return a & ((1 << 8) - 1);
}

static inline u32 i5100_nrecmemb_cas(u32 a)
{
	return a >> 16 & ((1 << 13) - 1);
}

static inline u32 i5100_nrecmemb_ras(u32 a)
{
	return a & ((1 << 16) - 1);
}

static inline u32 i5100_redmemb_ecc_locator(u32 a)
{
	return a & ((1 << 18) - 1);
}

static inline u32 i5100_recmema_merr(u32 a)
{
	return i5100_nrecmema_merr(a);
}

static inline u32 i5100_recmema_bank(u32 a)
{
	return i5100_nrecmema_bank(a);
}

static inline u32 i5100_recmema_rank(u32 a)
{
	return i5100_nrecmema_rank(a);
}

static inline u32 i5100_recmemb_cas(u32 a)
{
	return i5100_nrecmemb_cas(a);
}

static inline u32 i5100_recmemb_ras(u32 a)
{
	return i5100_nrecmemb_ras(a);
}

/* some generic limits */
#define I5100_MAX_RANKS_PER_CHAN	6
#define I5100_CHANNELS			    2
#define I5100_MAX_RANKS_PER_DIMM	4
#define I5100_DIMM_ADDR_LINES		(6 - 3)	/* 64 bits / 8 bits per byte */
#define I5100_MAX_DIMM_SLOTS_PER_CHAN	4
#define I5100_MAX_RANK_INTERLEAVE	4
#define I5100_MAX_DMIRS			5
#define I5100_SCRUB_REFRESH_RATE	(5 * 60 * HZ)

struct i5100_priv {
	/* ranks on each dimm -- 0 maps to not present -- obtained via SPD */
	int dimm_numrank[I5100_CHANNELS][I5100_MAX_DIMM_SLOTS_PER_CHAN];

	/*
	 * mainboard chip select map -- maps i5100 chip selects to
	 * DIMM slot chip selects.  In the case of only 4 ranks per
	 * channel, the mapping is fairly obvious but not unique.
	 * we map -1 -> NC and assume both channels use the same
	 * map...
	 *
	 */
	int dimm_csmap[I5100_MAX_DIMM_SLOTS_PER_CHAN][I5100_MAX_RANKS_PER_DIMM];

	/* memory interleave range */
	struct {
		u64	 limit;
		unsigned way[2];
	} mir[I5100_CHANNELS];

	/* adjusted memory interleave range register */
	unsigned amir[I5100_CHANNELS];

	/* dimm interleave range */
	struct {
		unsigned rank[I5100_MAX_RANK_INTERLEAVE];
		u64	 limit;
	} dmir[I5100_CHANNELS][I5100_MAX_DMIRS];

	/* memory technology registers... */
	struct {
		unsigned present;	/* 0 or 1 */
		unsigned ethrottle;	/* 0 or 1 */
		unsigned width;		/* 4 or 8 bits  */
		unsigned numbank;	/* 2 or 3 lines */
		unsigned numrow;	/* 13 .. 16 lines */
		unsigned numcol;	/* 11 .. 12 lines */
	} mtr[I5100_CHANNELS][I5100_MAX_RANKS_PER_CHAN];

	u64 tolm;		/* top of low memory in bytes */
	unsigned ranksperchan;	/* number of ranks per channel */

	struct pci_dev *mc;	/* device 16 func 1 */
	struct pci_dev *einj;	/* device 19 func 0 */
	struct pci_dev *ch0mm;	/* device 21 func 0 */
	struct pci_dev *ch1mm;	/* device 22 func 0 */

	struct delayed_work i5100_scrubbing;
	int scrub_enable;

	/* Error injection */
	u8 inject_channel;
	u8 inject_hlinesel;
	u8 inject_deviceptr1;
	u8 inject_deviceptr2;
	u16 inject_eccmask1;
	u16 inject_eccmask2;

	struct dentry *debugfs;
};

static struct dentry *i5100_debugfs;

/* map a rank/chan to a slot number on the mainboard */
static int i5100_rank_to_slot(const struct mem_ctl_info *mci,
			      int chan, int rank)
{
	const struct i5100_priv *priv = mci->pvt_info;
	int i;

	for (i = 0; i < I5100_MAX_DIMM_SLOTS_PER_CHAN; i++) {
		int j;
		const int numrank = priv->dimm_numrank[chan][i];

		for (j = 0; j < numrank; j++)
			if (priv->dimm_csmap[i][j] == rank)
				return i * 2 + chan;
	}

	return -1;
}

static const char *i5100_err_msg(unsigned err)
{
	static const char *merrs[] = {
		"unknown", /* 0 */
		"uncorrectable data ECC on replay", /* 1 */
		"unknown", /* 2 */
		"unknown", /* 3 */
		"aliased uncorrectable demand data ECC", /* 4 */
		"aliased uncorrectable spare-copy data ECC", /* 5 */
		"aliased uncorrectable patrol data ECC", /* 6 */
		"unknown", /* 7 */
		"unknown", /* 8 */
		"unknown", /* 9 */
		"non-aliased uncorrectable demand data ECC", /* 10 */
		"non-aliased uncorrectable spare-copy data ECC", /* 11 */
		"non-aliased uncorrectable patrol data ECC", /* 12 */
		"unknown", /* 13 */
		"correctable demand data ECC", /* 14 */
		"correctable spare-copy data ECC", /* 15 */
		"correctable patrol data ECC", /* 16 */
		"unknown", /* 17 */
		"SPD protocol error", /* 18 */
		"unknown", /* 19 */
		"spare copy initiated", /* 20 */
		"spare copy completed", /* 21 */
	};
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(merrs); i++)
		if (1 << i & err)
			return merrs[i];

	return "none";
}

/* convert csrow index into a rank (per channel -- 0..5) */
static int i5100_csrow_to_rank(const struct mem_ctl_info *mci, int csrow)
{
	const struct i5100_priv *priv = mci->pvt_info;

	return csrow % priv->ranksperchan;
}

/* convert csrow index into a channel (0..1) */
static int i5100_csrow_to_chan(const struct mem_ctl_info *mci, int csrow)
{
	const struct i5100_priv *priv = mci->pvt_info;

	return csrow / priv->ranksperchan;
}

static void i5100_handle_ce(struct mem_ctl_info *mci,
			    int chan,
			    unsigned bank,
			    unsigned rank,
			    unsigned long syndrome,
			    unsigned cas,
			    unsigned ras,
			    const char *msg)
{
	char detail[80];

	/* Form out message */
	snprintf(detail, sizeof(detail),
		 "bank %u, cas %u, ras %u\n",
		 bank, cas, ras);

	edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1,
			     0, 0, syndrome,
			     chan, rank, -1,
			     msg, detail);
}

static void i5100_handle_ue(struct mem_ctl_info *mci,
			    int chan,
			    unsigned bank,
			    unsigned rank,
			    unsigned long syndrome,
			    unsigned cas,
			    unsigned ras,
			    const char *msg)
{
	char detail[80];

	/* Form out message */
	snprintf(detail, sizeof(detail),
		 "bank %u, cas %u, ras %u\n",
		 bank, cas, ras);

	edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1,
			     0, 0, syndrome,
			     chan, rank, -1,
			     msg, detail);
}

static void i5100_read_log(struct mem_ctl_info *mci, int chan,
			   u32 ferr, u32 nerr)
{
	struct i5100_priv *priv = mci->pvt_info;
	struct pci_dev *pdev = (chan) ? priv->ch1mm : priv->ch0mm;
	u32 dw;
	u32 dw2;
	unsigned syndrome = 0;
	unsigned ecc_loc = 0;
	unsigned merr;
	unsigned bank;
	unsigned rank;
	unsigned cas;
	unsigned ras;

	pci_read_config_dword(pdev, I5100_VALIDLOG, &dw);

	if (i5100_validlog_redmemvalid(dw)) {
		pci_read_config_dword(pdev, I5100_REDMEMA, &dw2);
		syndrome = dw2;
		pci_read_config_dword(pdev, I5100_REDMEMB, &dw2);
		ecc_loc = i5100_redmemb_ecc_locator(dw2);
	}

	if (i5100_validlog_recmemvalid(dw)) {
		const char *msg;

		pci_read_config_dword(pdev, I5100_RECMEMA, &dw2);
		merr = i5100_recmema_merr(dw2);
		bank = i5100_recmema_bank(dw2);
		rank = i5100_recmema_rank(dw2);

		pci_read_config_dword(pdev, I5100_RECMEMB, &dw2);
		cas = i5100_recmemb_cas(dw2);
		ras = i5100_recmemb_ras(dw2);

		/* FIXME:  not really sure if this is what merr is...
		 */
		if (!merr)
			msg = i5100_err_msg(ferr);
		else
			msg = i5100_err_msg(nerr);

		i5100_handle_ce(mci, chan, bank, rank, syndrome, cas, ras, msg);
	}

	if (i5100_validlog_nrecmemvalid(dw)) {
		const char *msg;

		pci_read_config_dword(pdev, I5100_NRECMEMA, &dw2);
		merr = i5100_nrecmema_merr(dw2);
		bank = i5100_nrecmema_bank(dw2);
		rank = i5100_nrecmema_rank(dw2);

		pci_read_config_dword(pdev, I5100_NRECMEMB, &dw2);
		cas = i5100_nrecmemb_cas(dw2);
		ras = i5100_nrecmemb_ras(dw2);

		/* FIXME:  not really sure if this is what merr is...
		 */
		if (!merr)
			msg = i5100_err_msg(ferr);
		else
			msg = i5100_err_msg(nerr);

		i5100_handle_ue(mci, chan, bank, rank, syndrome, cas, ras, msg);
	}

	pci_write_config_dword(pdev, I5100_VALIDLOG, dw);
}

static void i5100_check_error(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	u32 dw, dw2;

	pci_read_config_dword(priv->mc, I5100_FERR_NF_MEM, &dw);
	if (i5100_ferr_nf_mem_any(dw)) {

		pci_read_config_dword(priv->mc, I5100_NERR_NF_MEM, &dw2);

		i5100_read_log(mci, i5100_ferr_nf_mem_chan_indx(dw),
			       i5100_ferr_nf_mem_any(dw),
			       i5100_nerr_nf_mem_any(dw2));

		pci_write_config_dword(priv->mc, I5100_NERR_NF_MEM, dw2);
	}
	pci_write_config_dword(priv->mc, I5100_FERR_NF_MEM, dw);
}

/* The i5100 chipset will scrub the entire memory once, then
 * set a done bit. Continuous scrubbing is achieved by enqueing
 * delayed work to a workqueue, checking every few minutes if
 * the scrubbing has completed and if so reinitiating it.
 */

static void i5100_refresh_scrubbing(struct work_struct *work)
{
	struct delayed_work *i5100_scrubbing = container_of(work,
							    struct delayed_work,
							    work);
	struct i5100_priv *priv = container_of(i5100_scrubbing,
					       struct i5100_priv,
					       i5100_scrubbing);
	u32 dw;

	pci_read_config_dword(priv->mc, I5100_MC, &dw);

	if (priv->scrub_enable) {

		pci_read_config_dword(priv->mc, I5100_MC, &dw);

		if (i5100_mc_scrbdone(dw)) {
			dw |= I5100_MC_SCRBEN_MASK;
			pci_write_config_dword(priv->mc, I5100_MC, dw);
			pci_read_config_dword(priv->mc, I5100_MC, &dw);
		}

		schedule_delayed_work(&(priv->i5100_scrubbing),
				      I5100_SCRUB_REFRESH_RATE);
	}
}
/*
 * The bandwidth is based on experimentation, feel free to refine it.
 */
static int i5100_set_scrub_rate(struct mem_ctl_info *mci, u32 bandwidth)
{
	struct i5100_priv *priv = mci->pvt_info;
	u32 dw;

	pci_read_config_dword(priv->mc, I5100_MC, &dw);
	if (bandwidth) {
		priv->scrub_enable = 1;
		dw |= I5100_MC_SCRBEN_MASK;
		schedule_delayed_work(&(priv->i5100_scrubbing),
				      I5100_SCRUB_REFRESH_RATE);
	} else {
		priv->scrub_enable = 0;
		dw &= ~I5100_MC_SCRBEN_MASK;
		cancel_delayed_work(&(priv->i5100_scrubbing));
	}
	pci_write_config_dword(priv->mc, I5100_MC, dw);

	pci_read_config_dword(priv->mc, I5100_MC, &dw);

	bandwidth = 5900000 * i5100_mc_scrben(dw);

	return bandwidth;
}

static int i5100_get_scrub_rate(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	u32 dw;

	pci_read_config_dword(priv->mc, I5100_MC, &dw);

	return 5900000 * i5100_mc_scrben(dw);
}

static struct pci_dev *pci_get_device_func(unsigned vendor,
					   unsigned device,
					   unsigned func)
{
	struct pci_dev *ret = NULL;

	while (1) {
		ret = pci_get_device(vendor, device, ret);

		if (!ret)
			break;

		if (PCI_FUNC(ret->devfn) == func)
			break;
	}

	return ret;
}

static unsigned long i5100_npages(struct mem_ctl_info *mci, int csrow)
{
	struct i5100_priv *priv = mci->pvt_info;
	const unsigned chan_rank = i5100_csrow_to_rank(mci, csrow);
	const unsigned chan = i5100_csrow_to_chan(mci, csrow);
	unsigned addr_lines;

	/* dimm present? */
	if (!priv->mtr[chan][chan_rank].present)
		return 0ULL;

	addr_lines =
		I5100_DIMM_ADDR_LINES +
		priv->mtr[chan][chan_rank].numcol +
		priv->mtr[chan][chan_rank].numrow +
		priv->mtr[chan][chan_rank].numbank;

	return (unsigned long)
		((unsigned long long) (1ULL << addr_lines) / PAGE_SIZE);
}

static void i5100_init_mtr(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	struct pci_dev *mms[2] = { priv->ch0mm, priv->ch1mm };
	int i;

	for (i = 0; i < I5100_CHANNELS; i++) {
		int j;
		struct pci_dev *pdev = mms[i];

		for (j = 0; j < I5100_MAX_RANKS_PER_CHAN; j++) {
			const unsigned addr =
				(j < 4) ? I5100_MTR_0 + j * 2 :
					  I5100_MTR_4 + (j - 4) * 2;
			u16 w;

			pci_read_config_word(pdev, addr, &w);

			priv->mtr[i][j].present = i5100_mtr_present(w);
			priv->mtr[i][j].ethrottle = i5100_mtr_ethrottle(w);
			priv->mtr[i][j].width = 4 + 4 * i5100_mtr_width(w);
			priv->mtr[i][j].numbank = 2 + i5100_mtr_numbank(w);
			priv->mtr[i][j].numrow = 13 + i5100_mtr_numrow(w);
			priv->mtr[i][j].numcol = 10 + i5100_mtr_numcol(w);
		}
	}
}

/*
 * FIXME: make this into a real i2c adapter (so that dimm-decode
 * will work)?
 */
static int i5100_read_spd_byte(const struct mem_ctl_info *mci,
			       u8 ch, u8 slot, u8 addr, u8 *byte)
{
	struct i5100_priv *priv = mci->pvt_info;
	u16 w;
	unsigned long et;

	pci_read_config_word(priv->mc, I5100_SPDDATA, &w);
	if (i5100_spddata_busy(w))
		return -1;

	pci_write_config_dword(priv->mc, I5100_SPDCMD,
			       i5100_spdcmd_create(0xa, 1, ch * 4 + slot, addr,
						   0, 0));

	/* wait up to 100ms */
	et = jiffies + HZ / 10;
	udelay(100);
	while (1) {
		pci_read_config_word(priv->mc, I5100_SPDDATA, &w);
		if (!i5100_spddata_busy(w))
			break;
		udelay(100);
	}

	if (!i5100_spddata_rdo(w) || i5100_spddata_sbe(w))
		return -1;

	*byte = i5100_spddata_data(w);

	return 0;
}

/*
 * fill dimm chip select map
 *
 * FIXME:
 *   o not the only way to may chip selects to dimm slots
 *   o investigate if there is some way to obtain this map from the bios
 */
static void i5100_init_dimm_csmap(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	int i;

	for (i = 0; i < I5100_MAX_DIMM_SLOTS_PER_CHAN; i++) {
		int j;

		for (j = 0; j < I5100_MAX_RANKS_PER_DIMM; j++)
			priv->dimm_csmap[i][j] = -1; /* default NC */
	}

	/* only 2 chip selects per slot... */
	if (priv->ranksperchan == 4) {
		priv->dimm_csmap[0][0] = 0;
		priv->dimm_csmap[0][1] = 3;
