 IS_QME(ppd->dd) ? 0 : 1;
	ibsd_wr_allchans(ppd, 13, (le_val << 5), (1 << 5));

	/* Clear cmode-override, may be set from older driver */
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 10, 0 << 14, 1 << 14);

	/* Timing Recovery: rxtapsel addr 5 bits [9:8] = 0 */
	ibsd_wr_allchans(ppd, 5, (0 << 8), BMASK(9, 8));

	/* setup LoS params; these are subsystem, so chan == 5 */
	/* LoS filter threshold_count on, ch 0-3, set to 8 */
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 5, 8 << 11, BMASK(14, 11));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 7, 8 << 4, BMASK(7, 4));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 8, 8 << 11, BMASK(14, 11));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 10, 8 << 4, BMASK(7, 4));

	/* LoS filter threshold_count off, ch 0-3, set to 4 */
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 6, 4 << 0, BMASK(3, 0));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 7, 4 << 8, BMASK(11, 8));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 9, 4 << 0, BMASK(3, 0));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 10, 4 << 8, BMASK(11, 8));

	/* LoS filter select enabled */
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 9, 1 << 15, 1 << 15);

	/* LoS target data:  SDR=4, DDR=2, QDR=1 */
	ibsd_wr_allchans(ppd, 14, (1 << 3), BMASK(5, 3)); /* QDR */
	ibsd_wr_allchans(ppd, 20, (2 << 10), BMASK(12, 10)); /* DDR */
	ibsd_wr_allchans(ppd, 20, (4 << 13), BMASK(15, 13)); /* SDR */

	serdes_7322_los_enable(ppd, 1);

	/* rxbistena; set 0 to avoid effects of it switch later */
	ibsd_wr_allchans(ppd, 9, 0 << 15, 1 << 15);

	/* Configure 4 DFE taps, and only they adapt */
	ibsd_wr_allchans(ppd, 16, 0 << 0, BMASK(1, 0));

	/* gain hi stop 32 (22) (6:1) lo stop 7 (10:7) target 22 (13) (15:11) */
	le_val = (ppd->dd->cspec->r1 || IS_QME(ppd->dd)) ? 0xb6c0 : 0x6bac;
	ibsd_wr_allchans(ppd, 21, le_val, 0xfffe);

	/*
	 * Set receive adaptation mode.  SDR and DDR adaptation are
	 * always on, and QDR is initially enabled; later disabled.
	 */
	qib_write_kreg_port(ppd, krp_static_adapt_dis(0), 0ULL);
	qib_write_kreg_port(ppd, krp_static_adapt_dis(1), 0ULL);
	qib_write_kreg_port(ppd, krp_static_adapt_dis(2),
			    ppd->dd->cspec->r1 ?
			    QDR_STATIC_ADAPT_DOWN_R1 : QDR_STATIC_ADAPT_DOWN);
	ppd->cpspec->qdr_dfe_on = 1;

	/* FLoop LOS gate: PPM filter  enabled */
	ibsd_wr_allchans(ppd, 38, 0 << 10, 1 << 10);

	/* rx offset center enabled */
	ibsd_wr_allchans(ppd, 12, 1 << 4, 1 << 4);

	if (!ppd->dd->cspec->r1) {
		ibsd_wr_allchans(ppd, 12, 1 << 12, 1 << 12);
		ibsd_wr_allchans(ppd, 12, 2 << 8, 0x0f << 8);
	}

	/* Set the frequency loop bandwidth to 15 */
	ibsd_wr_allchans(ppd, 2, 15 << 5, BMASK(8, 5));

	return 0;
}

static int serdes_7322_init_new(struct qib_pportdata *ppd)
{
	unsigned long tend;
	u32 le_val, rxcaldone;
	int chan, chan_done = (1 << SERDES_CHANS) - 1;

	/* Clear cmode-override, may be set from older driver */
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 10, 0 << 14, 1 << 14);

	/* ensure no tx overrides from earlier driver loads */
	qib_write_kreg_port(ppd, krp_tx_deemph_override,
		SYM_MASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
		reset_tx_deemphasis_override));

	/* START OF LSI SUGGESTED SERDES BRINGUP */
	/* Reset - Calibration Setup */
	/*       Stop DFE adaptaion */
	ibsd_wr_allchans(ppd, 1, 0, BMASK(9, 1));
	/*       Disable LE1 */
	ibsd_wr_allchans(ppd, 13, 0, BMASK(5, 5));
	/*       Disable autoadapt for LE1 */
	ibsd_wr_allchans(ppd, 1, 0, BMASK(15, 15));
	/*       Disable LE2 */
	ibsd_wr_allchans(ppd, 13, 0, BMASK(6, 6));
	/*       Disable VGA */
	ibsd_wr_allchans(ppd, 5, 0, BMASK(0, 0));
	/*       Disable AFE Offset Cancel */
	ibsd_wr_allchans(ppd, 12, 0, BMASK(12, 12));
	/*       Disable Timing Loop */
	ibsd_wr_allchans(ppd, 2, 0, BMASK(3, 3));
	/*       Disable Frequency Loop */
	ibsd_wr_allchans(ppd, 2, 0, BMASK(4, 4));
	/*       Disable Baseline Wander Correction */
	ibsd_wr_allchans(ppd, 13, 0, BMASK(13, 13));
	/*       Disable RX Calibration */
	ibsd_wr_allchans(ppd, 4, 0, BMASK(10, 10));
	/*       Disable RX Offset Calibration */
	ibsd_wr_allchans(ppd, 12, 0, BMASK(4, 4));
	/*       Select BB CDR */
	ibsd_wr_allchans(ppd, 2, (1 << 15), BMASK(15, 15));
	/*       CDR Step Size */
	ibsd_wr_allchans(ppd, 5, 0, BMASK(9, 8));
	/*       Enable phase Calibration */
	ibsd_wr_allchans(ppd, 12, (1 << 5), BMASK(5, 5));
	/*       DFE Bandwidth [2:14-12] */
	ibsd_wr_allchans(ppd, 2, (4 << 12), BMASK(14, 12));
	/*       DFE Config (4 taps only) */
	ibsd_wr_allchans(ppd, 16, 0, BMASK(1, 0));
	/*       Gain Loop Bandwidth */
	if (!ppd->dd->cspec->r1) {
		ibsd_wr_allchans(ppd, 12, 1 << 12, BMASK(12, 12));
		ibsd_wr_allchans(ppd, 12, 2 << 8, BMASK(11, 8));
	} else {
		ibsd_wr_allchans(ppd, 19, (3 << 11), BMASK(13, 11));
	}
	/*       Baseline Wander Correction Gain [13:4-0] (leave as default) */
	/*       Baseline Wander Correction Gain [3:7-5] (leave as default) */
	/*       Data Rate Select [5:7-6] (leave as default) */
	/*       RX Parallel Word Width [3:10-8] (leave as default) */

	/* RX REST */
	/*       Single- or Multi-channel reset */
	/*       RX Analog reset */
	/*       RX Digital reset */
	ibsd_wr_allchans(ppd, 0, 0, BMASK(15, 13));
	msleep(20);
	/*       RX Analog reset */
	ibsd_wr_allchans(ppd, 0, (1 << 14), BMASK(14, 14));
	msleep(20);
	/*       RX Digital reset */
	ibsd_wr_allchans(ppd, 0, (1 << 13), BMASK(13, 13));
	msleep(20);

	/* setup LoS params; these are subsystem, so chan == 5 */
	/* LoS filter threshold_count on, ch 0-3, set to 8 */
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 5, 8 << 11, BMASK(14, 11));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 7, 8 << 4, BMASK(7, 4));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 8, 8 << 11, BMASK(14, 11));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 10, 8 << 4, BMASK(7, 4));

	/* LoS filter threshold_count off, ch 0-3, set to 4 */
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 6, 4 << 0, BMASK(3, 0));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 7, 4 << 8, BMASK(11, 8));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 9, 4 << 0, BMASK(3, 0));
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 10, 4 << 8, BMASK(11, 8));

	/* LoS filter select enabled */
	ahb_mod(ppd->dd, IBSD(ppd->hw_pidx), 5, 9, 1 << 15, 1 << 15);

	/* LoS target data:  SDR=4, DDR=2, QDR=1 */
	ibsd_wr_allchans(ppd, 14, (1 << 3), BMASK(5, 3)); /* QDR */
	ibsd_wr_allchans(ppd, 20, (2 << 10), BMASK(12, 10)); /* DDR */
	ibsd_wr_allchans(ppd, 20, (4 << 13), BMASK(15, 13)); /* SDR */

	/* Turn on LOS on initial SERDES init */
	serdes_7322_los_enable(ppd, 1);
	/* FLoop LOS gate: PPM filter  enabled */
	ibsd_wr_allchans(ppd, 38, 0 << 10, 1 << 10);

	/* RX LATCH CALIBRATION */
	/*       Enable Eyefinder Phase Calibration latch */
	ibsd_wr_allchans(ppd, 15, 1, BMASK(0, 0));
	/*       Enable RX Offset Calibration latch */
	ibsd_wr_allchans(ppd, 12, (1 << 4), BMASK(4, 4));
	msleep(20);
	/*       Start Calibration */
	ibsd_wr_allchans(ppd, 4, (1 << 10), BMASK(10, 10));
	tend = jiffies + msecs_to_jiffies(500);
	while (chan_done && !time_is_before_jiffies(tend)) {
		msleep(20);
		for (chan = 0; chan < SERDES_CHANS; ++chan) {
			rxcaldone = ahb_mod(ppd->dd, IBSD(ppd->hw_pidx),
					    (chan + (chan >> 1)),
					    25, 0, 0);
			if ((~rxcaldone & (u32)BMASK(9, 9)) == 0 &&
			    (~chan_done & (1 << chan)) == 0)
				chan_done &= ~(1 << chan);
		}
	}
	if (chan_done) {
		pr_info("Serdes %d calibration not done after .5 sec: 0x%x\n",
			 IBSD(ppd->hw_pidx), chan_done);
	} else {
		for (chan = 0; chan < SERDES_CHANS; ++chan) {
			rxcaldone = ahb_mod(ppd->dd, IBSD(ppd->hw_pidx),
					    (chan + (chan >> 1)),
					    25, 0, 0);
			if ((~rxcaldone & (u32)BMASK(10, 10)) == 0)
				pr_info("Serdes %d chan %d calibration failed\n",
					IBSD(ppd->hw_pidx), chan);
		}
	}

	/*       Turn off Calibration */
	ibsd_wr_allchans(ppd, 4, 0, BMASK(10, 10));
	msleep(20);

	/* BRING RX UP */
	/*       Set LE2 value (May be overridden in qsfp_7322_event) */
	le_val = IS_QME(ppd->dd) ? LE2_QME : LE2_DEFAULT;
	ibsd_wr_allchans(ppd, 13, (le_val << 7), BMASK(9, 7));
	/*       Set LE2 Loop bandwidth */
	ibsd_wr_allchans(ppd, 3, (7 << 5), BMASK(7, 5));
	/*       Enable LE2 */
	ibsd_wr_allchans(ppd, 13, (1 << 6), BMASK(6, 6));
	msleep(20);
	/*       Enable H0 only */
	ibsd_wr_allchans(ppd, 1, 1