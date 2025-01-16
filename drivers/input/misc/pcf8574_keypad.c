7) target 22 (13) (15:11) */
	le_val = (ppd->dd->cspec->r1 || IS_QME(ppd->dd)) ? 0xb6c0 : 0x6bac;
	ibsd_wr_allchans(ppd, 21, le_val, 0xfffe);
	/*       Enable VGA */
	ibsd_wr_allchans(ppd, 5, 0, BMASK(0, 0));
	msleep(20);
	/*       Set Frequency Loop Bandwidth */
	ibsd_wr_allchans(ppd, 2, (15 << 5), BMASK(8, 5));
	/*       Enable Frequency Loop */
	ibsd_wr_allchans(ppd, 2, (1 << 4), BMASK(4, 4));
	/*       Set Timing Loop Bandwidth */
	ibsd_wr_allchans(ppd, 2, 0, BMASK(11, 9));
	/*       Enable Timing Loop */
	ibsd_wr_allchans(ppd, 2, (1 << 3), BMASK(3, 3));
	msleep(50);
	/*       Enable DFE
	 *       Set receive adaptation mode.  SDR and DDR adaptation are
	 *       always on, and QDR is initially enabled; later disabled.
	 */
	qib_write_kreg_port(ppd, krp_static_adapt_dis(0), 0ULL);
	qib_write_kreg_port(ppd, krp_static_adapt_dis(1), 0ULL);
	qib_write_kreg_port(ppd, krp_static_adapt_dis(2),
			    ppd->dd->cspec->r1 ?
			    QDR_STATIC_ADAPT_DOWN_R1 : QDR_STATIC_ADAPT_DOWN);
	ppd->cpspec->qdr_dfe_on = 1;
	/*       Disable LE1  */
	ibsd_wr_allchans(ppd, 13, (0 << 5), (1 << 5));
	/*       Disable auto adapt for LE1 */
	ibsd_wr_allchans(ppd, 1, (0 << 15), BMASK(15, 15));
	msleep(20);
	/*       Enable AFE Offset Cancel */
	ibsd_wr_allchans(ppd, 12, (1 << 12), BMASK(12, 12));
	/*       Enable Baseline Wander Correction */
	ibsd_wr_allchans(ppd, 12, (1 << 13), BMASK(13, 13));
	/* Termination: rxtermctrl_r2d addr 11 bits [12:11] = 1 */
	ibsd_wr_allchans(ppd, 11, (1 << 11), BMASK(12, 11));
	/* VGA output common mode */
	ibsd_wr_allchans(ppd, 12, (3 << 2), BMASK(3, 2));

	/*
	 * Initialize the Tx DDS tables.  Also done every QSFP event,
	 * for adapters with QSFP
	 */
	init_txdds_table(ppd, 0);

	return 0;
}

/* start adjust QMH serdes parameters */

static void set_man_code(struct qib_pportdata *ppd, int chan, int code)
{
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), (chan + (chan >> 1)),
		9, code << 9, 0x3f << 9);
}

static void set_man_mode_h1(struct qib_pportdata *ppd, int chan,
	int enable, u32 tapenable)
{
	if (enable)
		ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), (chan + (chan >> 1)),
			1, 3 << 10, 0x1f << 10);
	else
		ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), (chan + (chan >> 1)),
			1, 0, 0x1f << 10);
}

/* Set clock to 1, 0, 1, 0 */
static void clock_man(struct qib_pportdata *ppd, int chan)
{
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), (chan + (chan >> 1)),
		4, 0x4000, 0x4000);
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), (chan + (chan >> 1)),
		4, 0, 0x4000);
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), (chan + (chan >> 1)),
		4, 0x4000, 0x4000);
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), (chan + (chan >> 1)),
		4, 0, 0x4000);
}

/*
 * write the current Tx serdes pre,post,main,amp settings into the serdes.
 * The caller must pass the settings appropriate for the current speed,
 * or not care if they are correct for the current speed.
 */
static void write_tx_serdes_param(struct qib_pportdata *ppd,
				  struct txdds_ent *txdds)
{
	u64 deemph;

	deemph = qib_read_kreg_port(ppd, krp_tx_deemph_override);
	/* field names for amp, main, post, pre, respectively */
	deemph &= ~(SYM_MASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0, txampcntl_d2a) |
		    SYM_MASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0, txc0_ena) |
		    SYM_MASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0, txcp1_ena) |
		    SYM_MASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0, txcn1_ena));

	deemph |= SYM_MASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
			   tx_override_deemphasis_select);
	deemph |= (txdds->amp & SYM_RMASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
		    txampcntl_d2a)) << SYM_LSB(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
				       txampcntl_d2a);
	deemph |= (txdds->main & SYM_RMASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
		     txc0_ena)) << SYM_LSB(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
				   txc0_ena);
	deemph |= (txdds->post & SYM_RMASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
		     txcp1_ena)) << SYM_LSB(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
				    txcp1_ena);
	deemph |= (txdds->pre & SYM_RMASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
		     txcn1_ena)) << SYM_LSB(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
				    txcn1_ena);
	qib_write_kreg_port(ppd, krp_tx_deemph_override, deemph);
}

/*
 * Set the parameters for mez cards on link bounce, so they are
 * always exactly what was requested.  Similar logic to init_txdds
 * but does just the serdes.
 */
static void adj_tx_serdes(struct qib_pportdata *ppd)
{
	const struct txdds_ent *sdr_dds, *ddr_dds, *qdr_dds;
	struct txdds_ent *dds;

	find_best_ent(ppd, &sdr_dds, &ddr_dds, &qdr_dds, 1);
	dds = (struct txdds_ent *)(ppd->link_speed_active == QIB_IB_QDR ?
		qdr_dds : (ppd->link_speed_active == QIB_IB_DDR ?
				ddr_dds : sdr_dds));
	write_tx_serdes_param(ppd, dds);
}

/* set QDR forced value for H1, if needed */
sta