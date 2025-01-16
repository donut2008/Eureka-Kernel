cquired before calling this routine
 */
static void dump_sdma_7322_state(struct qib_pportdata *ppd)
{
	u64 reg, reg1, reg2;

	reg = qib_read_kreg_port(ppd, krp_senddmastatus);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmastatus: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_sendctrl);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA sendctrl: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmabase);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmabase: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmabufmask0);
	reg1 = qib_read_kreg_port(ppd, krp_senddmabufmask1);
	reg2 = qib_read_kreg_port(ppd, krp_senddmabufmask2);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmabufmask 0:%llx  1:%llx  2:%llx\n",
		 reg, reg1, reg2);

	/* get bufuse bits, clear them, and print them again if non-zero */
	reg = qib_read_kreg_port(ppd, krp_senddmabuf_use0);
	qib_write_kreg_port(ppd, krp_senddmabuf_use0, reg);
	reg1 = qib_read_kreg_port(ppd, krp_senddmabuf_use1);
	qib_write_kreg_port(ppd, krp_senddmabuf_use0, reg1);
	reg2 = qib_read_kreg_port(ppd, krp_senddmabuf_use2);
	qib_write_kreg_port(ppd, krp_senddmabuf_use0, reg2);
	/* 0 and 1 should always be zero, so print as short form */
	qib_dev_porterr(ppd->dd, ppd->port,
		 "SDMA current senddmabuf_use 0:%llx  1:%llx  2:%llx\n",
		 reg, reg1, reg2);
	reg = qib_read_kreg_port(ppd, krp_senddmabuf_use0);
	reg1 = qib_read_kreg_port(ppd, krp_senddmabuf_use1);
	reg2 = qib_read_kreg_port(ppd, krp_senddmabuf_use2);
	/* 0 and 1 should always be zero, so print as short form */
	qib_dev_porterr(ppd->dd, ppd->port,
		 "SDMA cleared senddmabuf_use 0:%llx  1:%llx  2:%llx\n",
		 reg, reg1, reg2);

	reg = qib_read_kreg_port(ppd, krp_senddmatail);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmatail: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmahead);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmahead: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmaheadaddr);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmaheadaddr: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmalengen);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmalengen: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmadesccnt);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmadesccnt: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmaidlecnt);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmaidlecnt: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmaprioritythld);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmapriorityhld: 0x%016llx\n", reg);

	reg = qib_read_kreg_port(ppd, krp_senddmareloadcnt);
	qib_dev_porterr(ppd->dd, ppd->port,
		"SDMA senddmareloadcnt: 0x%016llx\n", reg);

	dump_sdma_state(ppd);
}

static struct sdma_set_state_action sdma_7322_action_table[] = {
	[qib_sdma_state_s00_hw_down] = {
		.go_s99_running_tofalse = 1,
		.op_enable = 0,
		.op_intenable = 0,
		.op_halt = 0,
		.op_drain = 0,
	},
	[qib_sdma_state_s10_hw_start_up_wait] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 1,
		.op_drain = 0,
	},
	[qib_sdma_state_s20_idle] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 1,
		.op_drain = 0,
	},
	[qib_sdma_state_s30_sw_clean_up_wait] = {
		.op_enable = 0,
		.op_intenable = 1,
		.op_halt = 1,
		.op_drain = 0,
	},
	[qib_sdma_state_s40_hw_clean_up_wait] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 1,
		.op_drain = 0,
	},
	[qib_sdma_state_s50_hw_halt_wait] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 1,
		.op_drain = 1,
	},
	[qib_sdma_state_s99_running] = {
		.op_enable = 1,
		.op_intenable = 1,
		.op_halt = 0,
		.op_drain = 0,
		.go_s99_running_totrue = 1,
	},
};

static void qib_7322_sdma_init_early(struct qib_pportdata *ppd)
{
	ppd->sdma_state.set_state_action = sdma_7322_action_table;
}

static int init_sdma_7322_regs(struct qib_pportdata *ppd)
{
	struct qib_devdata *dd = ppd->dd;
	unsigned lastbuf, erstbuf;
	u64 senddmabufmask[3] = { 0 };
	int n, ret = 0;

	qib_write_kreg_port(ppd, krp_senddmabase, ppd->sdma_descq_phys);
	qib_sdma_7322_setlengen(ppd);
	qib_sdma_update_7322_tail(ppd, 0); /* Set SendDmaTail */
	qib_write_kreg_port(ppd, krp_senddmareloadcnt, sdma_idle_cnt);
	qib_write_kreg_port(ppd, krp_senddmadesccnt, 0);
	qib_write_kreg_port(ppd, krp_senddmaheadaddr, ppd->sdma_head_phys);

	if (dd->num_pports)
		n = dd->cspec->sdmabufcnt / dd->num_pports; /* no remainder */
	else
		n = dd->cspec->sdmabufcnt; /* failsafe for init */
	erstbuf = (dd->piobcnt2k + dd->piobcnt4k) -
		((dd->num_pports == 1 || ppd->port == 2) ? n :
		dd->cspec->sdmabufcnt);
	lastbuf = erstbuf + n;

	ppd->sdma_state.first_sendbuf = erstbuf;
	ppd->sdma_state.last_sendbuf = lastbuf;
	for (; erstbuf < lastbuf; ++erstbuf) {
		unsigned word = erstbuf / BITS_PER_LONG;
		unsigned bit = erstbuf & (BITS_PER_LONG - 1);

		BUG_ON(word >= 3);
		senddmabufmask[word] |= 1ULL << bit;
	}
	qib_write_kreg_port(ppd, krp_senddmabufmask0, senddmabufmask[0]);
	qib_write_kreg_port(ppd, krp_senddmabufmask1, senddmabufmask[1]);
	qib_write_kreg_port(ppd, krp_senddmabufmask2, senddmabufmask[2]);
	return ret;
}

/* sdma_lock must be held */
static u16 qib_sdma_7322_gethead(struct qib_pportdata *ppd)
{
	struct qib_devdata *dd = ppd->dd;
	int sane;
	int use_dmahead;
	u16 swhead;
	u16 swtail;
	u16 cnt;
	u16 hwhead;

	use_dmahead = __qib_sdma_running(ppd) &&
		(dd->flags & QIB_HAS_SDMA_TIMEOUT);
retry:
	hwhead = use_dmahead ?
		(u16) le64_to_cpu(*ppd->sdma_head_dma) :
		(u16) qib_read_kreg_port(ppd, krp_senddmahead);

	swhead = ppd->sdma_descq_head;
	swtail = ppd->sdma_descq_tail;
	cnt = ppd->sdma_descq_cnt;

	if (swhead < swtail)
		/* not wrapped */
		sane = (hwhead >= swhead) & (hwhead <= swtail);
	else if (swhead > swtail)
		/* wrapped around */
		sane = ((hwhead >= swhead) && (hwhead < cnt)) ||
			(hwhead <= swtail);
	else
		/* empty */
		sane = (hwhead == swhead);

	if (unlikely(!sane)) {
		if (use_dmahead) {
			/* try one more time, directly from the register */
			use_dmahead = 0;
			goto retry;
		}
		/* proceed as if no progress */
		hwhead = swhead;
	}

	return hwhead;
}

static int qib_sdma_7322_busy(struct qib_pportdata *ppd)
{
	u64 hwstatus = qib_read_kreg_port(ppd, krp_senddmastatus);

	return (hwstatus & SYM_MASK(SendDmaStatus_0, ScoreBoardDrainInProg)) ||
	       (hwstatus & SYM_MASK(SendDmaStatus_0, HaltInProg)) ||
	       !(hwstatus & SYM_MASK(SendDmaStatus_0, InternalSDmaHalt)) ||
	       !(hwstatus & SYM_MASK(SendDmaStatus_0, ScbEmpty));
}

/*
 * Compute the amount of delay before sending the next packet if the
 * port's send rate differs from the static rate set for the QP.
 * The delay affects the next packet and the amount of the delay is
 * based on the length of the this packet.
 */
static u32 qib_7322_setpbc_control(struct qib_pportdata *ppd, u32 plen,
				   u8 srate, u8 vl)
{
	u8 snd_mult = ppd->delay_mult;
	u8 rcv_mult = ib_rate_to_delay[srate];
	u32 ret;

	ret = rcv_mult > snd_mult ? ((plen + 1) >> 1) * snd_mult : 0;

	/* Indicate VL15, else set the VL in the control word */
	if (vl == 15)
		ret |= PBC_7322_VL15_SEND_CTRL;
	else
		ret |= vl << PBC_VL_NUM_LSB;
	ret |= ((u32)(ppd->hw_pidx)) << PBC_PORT_SEL_LSB;

	return ret;
}

/*
 * Enable the per-port VL15 send buffers for use.
 * They follow the rest of the buffers, without a config parameter.
 * This was in initregs, but that is done before the shadow
 * is set up, and this has to be done after the shadow is
 * set up.
 */
static void qib_7322_initvl15_bufs(struct qib_devdata *dd)
{
	unsigned vl15bufs;

	vl15bufs = dd->piobcnt2k + dd->piobcnt4k;
	qib_chg_pioavailkernel(dd, vl15bufs, NUM_VL15_BUFS,
			       TXCHK_CHG_TYPE_KERN, NULL);
}

static void qib_7322_init_ctxt(struct qib_ctxtdata *rcd)
{
	if (rcd->ctxt < NUM_IB_PORTS) {
		if (rcd->dd->num_pports > 1) {
			rcd->rcvegrcnt = KCTXT0_EGRCNT / 2;
			rcd->rcvegr_tid_base = rcd->ctxt ? rcd->rcvegrcnt : 0;
		} else {
			rcd->rcvegrcnt = KCTXT0_EGRCNT;
			rcd->rcvegr_tid_base = 0;
		}
	} else {
		rcd->rcvegrcnt = rcd->dd->cspec->rcvegrcnt;
		rcd->rcvegr_tid_base = KCTXT0_EGRCNT +
			(rcd->ctxt - NUM_IB_PORTS) * rcd->rcvegrcnt;
	}
}

#define QTXSLEEPS 5000
static void qib_7322_txchk_change(struct qib_devdata *dd, u32 start,
				  u32 len, u32 which, struct qib_ctxtdata *rcd)
{
	int i;
	const int last = start + len - 1;
	const int lastr = last / BITS_PER_LONG;
	u32 sleeps = 0;
	int wait = rcd != NULL;
	unsigned long flags;

	while (wait) {
		unsigned long shadow = 0;
		int cstart, previ = -1;

		/*
		 * when flipping from kernel to user, we can't change
		 * the checking type if the buffer is allocated to the
		 * driver.   It's OK the other direction, because it's
		 * from close, and we have just disarm'ed all the
		 * buffers.  All the kernel to kernel changes are also
		 * OK.
		 */
		for (cstart = start; cstart <= last; cstart++) {
			i = ((2 * cstart) + QLOGIC_IB_SENDPIOAVAIL_BUSY_SHIFT)
				/ BITS_PER_LONG;
			if (i != previ) {
				shadow = (unsigned long)
					le64_to_cpu(dd->pioavailregs_dma[i]);
				previ = i;
			}
			if (test_bit(((2 * cstart) +
				      QLOGIC_IB_SENDPIOAVAIL_BUSY_SHIFT)
				     % BITS_PER_LONG, &shadow))
				break;
		}

		if (cstart > last)
			break;

		if (sleeps == QTXSLEEPS)
			break;
		/* make sure we see an updated copy next time around */
		sendctrl_7322_mod(dd->pport, QIB_SENDCTRL_AVAIL_BLIP);
		sleeps++;
		msleep(20);
	}

	switch (which) {
	case TXCHK_CHG_TYPE_DIS1:
		/*
		 * disable checking on a range; used by diags; just
		 * one buffer, but still written generically
		 */
		for (i = start; i <= last; i++)
			clear_bit(i, dd->cspec->sendchkenable);
		break;

	case TXCHK_CHG_TYPE_ENAB1:
		/*
		 * (re)enable checking on a range; used by diags; just
		 * one buffer, but still written generically; read
		 * scratch to be sure buffer actually triggered, not
		 * just flushed from processor.
		 */
		qib_read_kreg32(dd, kr_scratch);
		for (i = start; i <= last; i++)
			set_bit(i, dd->cspec->sendchkenable);
		break;

	case TXCHK_CHG_TYPE_KERN:
		/* usable by kernel */
		for (i = start; i <= last; i++) {
			set_bit(i, dd->cspec->sendibchk);
			clear_bit(i, dd->cspec->sendgrhchk);
		}
		spin_lock_irqsave(&dd->uctxt_lock, flags);
		/* see if we need to raise avail update threshold */
		for (i = dd->first_user_ctxt;
		     dd->cspec->updthresh != dd->cspec->updthresh_dflt
		     && i < dd->cfgctxts; i++)
			if (dd->rcd[i] && dd->rcd[i]->subctxt_cnt &&
			   ((dd->rcd[i]->piocnt / dd->rcd[i]->subctxt_cnt) - 1)
			   < dd->cspec->updthresh_dflt)
				break;
		spin_unlock_irqrestore(&dd->uctxt_lock, flags);
		if (i == dd->cfgctxts) {
			spin_lock_irqsave(&dd->sendctrl_lock, flags);
			dd->cspec->updthresh = dd->cspec->updthresh_dflt;
			dd->sendctrl &= ~SYM_MASK(SendCtrl, AvailUpdThld);
			dd->sendctrl |= (dd->cspec->updthresh &
					 SYM_RMASK(SendCtrl, AvailUpdThld)) <<
					   SYM_LSB(SendCtrl, AvailUpdThld);
			spin_unlock_irqrestore(&dd->sendctrl_lock, flags);
			sendctrl_7322_mod(dd->pport, QIB_SENDCTRL_AVAIL_BLIP);
		}
		break;

	case TXCHK_CHG_TYPE_USER:
		/* for user process */
		for (i = start; i <= last; i++) {
			clear_bit(i, dd->cspec->sendibchk);
			set_bit(i, dd->cspec->sendgrhchk);
		}
		spin_lock_irqsave(&dd->sendctrl_lock, flags);
		if (rcd && rcd->subctxt_cnt && ((rcd->piocnt
			/ rcd->subctxt_cnt) - 1) < dd->cspec->updthresh) {
			dd->cspec->updthresh = (rcd->piocnt /
						rcd->subctxt_cnt) - 1;
			dd->sendctrl &= ~SYM_MASK(SendCtrl, AvailUpdThld);
			dd->sendctrl |= (dd->cspec->updthresh &
					SYM_RMASK(SendCtrl, AvailUpdThld))
					<< SYM_LSB(SendCtrl, AvailUpdThld);
			spin_unlock_irqrestore(&dd->sendctrl_lock, flags);
			sendctrl_7322_mod(dd->pport, QIB_SENDCTRL_AVAIL_BLIP);
		} else
			spin_unlock_irqrestore(&dd->sendctrl_lock, flags);
		break;

	default:
		break;
	}

	for (i = start / BITS_PER_LONG; which >= 2 && i <= lastr; ++i)
		qib_write_kreg(dd, kr_sendcheckmask + i,
			       dd->cspec->sendchkenable[i]);

	for (i = start / BITS_PER_LONG; which < 2 && i <= lastr; ++i) {
		qib_write_kreg(dd, kr_sendgrhcheckmask + i,
			       dd->cspec->sendgrhchk[i]);
		qib_write_kreg(dd, kr_sendibpktmask + i,
			       dd->cspec->sendibchk[i]);
	}

	/*
	 * Be sure whatever we did was seen by the chip and acted upon,
	 * before we return.  Mostly important for which >= 2.
	 */
	qib_read_kreg32(dd, kr_scratch);
}


/* useful for trigger analyzers, etc. */
static void writescratch(struct qib_devdata *dd, u32 val)
{
	qib_write_kreg(dd, kr_scratch, val);
}

/* Dummy for now, use chip regs soon */
static int qib_7322_tempsense_rd(struct qib_devdata *dd, int regnum)
{
	return -ENXIO;
}

/**
 * qib_init_iba7322_funcs - set up the chip-specific function pointers
 * @dev: the pci_dev for qlogic_ib device
 * @ent: pci_device_id struct for this dev
 *
 * Also allocates, inits, and returns the devdata struct for this
 * device instance
 *
 * This is global, and is called directly at init to set up the
 * chip-specific function pointers for later use.
 */
struct qib_devdata *qib_init_iba7322_funcs(struct pci_dev *pdev,
					   const struct pci_device_id *ent)
{
	struct qib_devdata *dd;
	int ret, i;
	u32 tabsize, actual_cnt = 0;

	dd = qib_alloc_devdata(pdev,
		NUM_IB_PORTS * sizeof(struct qib_pportdata) +
		sizeof(struct qib_chip_specific) +
		NUM_IB_PORTS * sizeof(struct qib_chippport_specific));
	if (IS_ERR(dd))
		goto bail;

	dd->f_bringup_serdes    = qib_7322_bringup_serdes;
	dd->f_cleanup           = qib_setup_7322_cleanup;
	dd->f_clear_tids        = qib_7322_clear_tids;
	dd->f_free_irq          = qib_7322_free_irq;
	dd->f_get_base_info     = qib_7322_get_base_info;
	dd->f_get_msgheader     = qib_7322_get_msgheader;
	dd->f_getsendbuf        = qib_7322_getsendbuf;
	dd->f_gpio_mod          = gpio_7322_mod;
	dd->f_eeprom_wen        = qib_7322_eeprom_wen;
	dd->f_hdrqempty         = qib_7322_hdrqempty;
	dd->f_ib_updown         = qib_7322_ib_updown;
	dd->f_init_ctxt         = qib_7322_init_ctxt;
	dd->f_initvl15_bufs     = qib_7322_initvl15_bufs;
	dd->f_intr_fallback     = qib_7322_intr_fallback;
	dd->f_late_initreg      = qib_late_7322_initreg;
	dd->f_setpbc_control    = qib_7322_setpbc_control;
	dd->f_portcntr          = qib_portcntr_7322;
	dd->f_put_tid           = qib_7322_put_tid;
	dd->f_quiet_serdes      = qib_7322_mini_quiet_serdes;
	dd->f_rcvctrl           = rcvctrl_7322_mod;
	dd->f_read_cntrs        = qib_read_7322cntrs;
	dd->f_read_portcntrs    = qib_read_7322portcntrs;
	dd->f_reset             = qib_do_7322_reset;
	dd->f_init_sdma_regs    = init_sdma_7322_regs;
	dd->f_sdma_busy         = qib_sdma_7322_busy;
	dd->f_sdma_gethead      = qib_sdma_7322_gethead;
	dd->f_sdma_sendctrl     = qib_7322_sdma_sendctrl;
	dd->f_sdma_set_desc_cnt = qib_sdma_set_7322_desc_cnt;
	dd->f_sdma_update_tail  = qib_sdma_update_7322_tail;
	dd->f_sendctrl          = sendctrl_7322_mod;
	dd->f_set_armlaunch     = qib_set_7322_armlaunch;
	dd->f_set_cntr_sample   = qib_set_cntr_7322_sample;
	dd->f_iblink_state      = qib_7322_iblink_state;
	dd->f_ibphys_portstate  = qib_7322_phys_portstate;
	dd->f_get_ib_cfg        = qib_7322_get_ib_cfg;
	dd->f_set_ib_cfg        = qib_7322_set_ib_cfg;
	dd->f_set_ib_loopback   = qib_7322_set_loopback;
	dd->f_get_ib_table      = qib_7322_get_ib_table;
	dd->f_set_ib_table      = qib_7322_set_ib_table;
	dd->f_set_intr_state    = qib_7322_set_intr_state;
	dd->f_setextled         = qib_setup_7322_setextled;
	dd->f_txchk_change      = qib_7322_txchk_change;
	dd->f_update_usrhead    = qib_update_7322_usrhead;
	dd->f_wantpiobuf_intr   = qib_wantpiobuf_7322_intr;
	dd->f_xgxs_reset        = qib_7322_mini_pcs_reset;
	dd->f_sdma_hw_clean_up  = qib_7322_sdma_hw_clean_up;
	dd->f_sdma_hw_start_up  = qib_7322_sdma_hw_start_up;
	dd->f_sdma_init_early   = qib_7322_sdma_init_early;
	dd->f_writescratch      = writescratch;
	dd->f_tempsense_rd	= qib_7322_tempsense_rd;
#ifdef CONFIG_INFINIBAND_QIB_DCA
	dd->f_notify_dca	= qib_7322_notify_dca;
#endif
	/*
	 * Do remaining PCIe setup and save PCIe values in dd.
	 * Any error printing is already done by the init code.
	 * On return, we have the chip mapped, but chip registers
	 * are not set up until start of qib_init_7322_variables.
	 */
	ret = qib_pcie_ddinit(dd, pdev, ent);
	if (ret < 0)
		goto bail_free;

	/* initialize chip-specific variables */
	ret = qib_init_7322_variables(dd);
	if (ret)
		goto bail_cleanup;

	if (qib_mini_init || !dd->num_pports)
		goto bail;

	/*
	 * Determine number of vectors we want; depends on port count
	 * and number of configured kernel receive queues actually used.
	 * Should also depend on whether sdma is enabled or not, but
	 * that's such a rare testing case it's not worth worrying about.
	 */
	tabsize = dd->first_user_ctxt + ARRAY_SIZE(irq_table);
	for (i = 0; i < tabsize; i++)
		if ((i < ARRAY_SIZE(irq_table) &&
		     irq_table[i].port <= dd->num_pports) ||
		    (i >= ARRAY_SIZE(irq_table) &&
		     dd->rcd[i - ARRAY_SIZE(irq_table)]))
			actual_cnt++;
	/* reduce by ctxt's < 2 */
	if (qib_krcvq01_no_msi)
		actual_cnt -= dd->num_pports;

	tabsize = actual_cnt;
	dd->cspec->msix_entries = kzalloc(tabsize *
			sizeof(struct qib_msix_entry), GFP_KERNEL);
	if (!dd->cspec->msix_entries) {
		qib_dev_err(dd, "No memory for MSIx table\n");
		tabsize = 0;
	}
	for (i = 0; i < tabsize; i++)
		dd->cspec->msix_entries[i].msix.entry = i;

	if (qib_pcie_params(dd, 8, &tabsize, dd->cspec->msix_entries))
		qib_dev_err(dd,
			"Failed to setup PCIe or interrupts; continuing anyway\n");
	/* may be less than we wanted, if not enough available */
	dd->cspec->num_msix_entries = tabsize;

	/* setup interrupt handler */
	qib_setup_7322_interrupt(dd, 1);

	/* clear diagctrl register, in case diags were running and crashed */
	qib_write_kreg(dd, kr_hwdiagctrl, 0);
#ifdef CONFIG_INFINIBAND_QIB_DCA
	if (!dca_add_requester(&pdev->dev)) {
		qib_devinfo(dd->pcidev, "DCA enabled\n");
		dd->flags |= QIB_DCA_ENABLED;
		qib_setup_dca(dd);
	}
#endif
	goto bail;

bail_cleanup:
	qib_pcie_ddcleanup(dd);
bail_free:
	qib_free_devdata(dd);
	dd = ERR_PTR(ret);
bail:
	return dd;
}

/*
 * Set the table entry at the specified index from the table specifed.
 * There are 3 * TXDDS_TABLE_SZ entries in all per port, with the first
 * TXDDS_TABLE_SZ for SDR, the next for DDR, and the last for QDR.
 * 'idx' below addresses the correct entry, while its 4 LSBs select the
 * corresponding entry (one of TXDDS_TABLE_SZ) from the selected table.
 */
#define DDS_ENT_AMP_LSB 14
#define DDS_ENT_MAIN_LSB 9
#define DDS_ENT_POST_LSB 5
#define DDS_ENT_PRE_XTRA_LSB 3
#define DDS_ENT_PRE_LSB 0

/*
 * Set one entry in the TxDDS table for spec'd port
 * ridx picks one of the entries, while tp points
 * to the appropriate table entry.
 */
static void set_txdds(struct qib_pportdata *ppd, int ridx,
		      const struct txdds_ent *tp)
{
	struct qib_devdata *dd = ppd->dd;
	u32 pack_ent;
	int regidx;

	/* Get correct offset in chip-space, and in source table */
	regidx = KREG_IBPORT_IDX(IBSD_DDS_MAP_TABLE) + ridx;
	/*
	 * We do not use qib_write_kreg_port() because it was intended
	 * only for registers in the lower "port specific" pages.
	 * So do index calculation  by hand.
	 */
	if (ppd->hw_pidx)
		regidx += (dd->palign / sizeof(u64));

	pack_ent = tp->amp << DDS_ENT_AMP_LSB;
	pack_ent |= tp->main << DDS_ENT_MAIN_LSB;
	pack_ent |= tp->pre << DDS_ENT_PRE_LSB;
	pack_ent |= tp->post << DDS_ENT_POST_LSB;
	qib_write_kreg(dd, regidx, pack_ent);
	/* Prevent back-to-back writes by hitting scratch */
	qib_write_kreg(ppd->dd, kr_scratch, 0);
}

static const struct vendor_txdds_ent vendor_txdds[] = {
	{ /* Amphenol 1m 30awg NoEq */
		{ 0x41, 0x50, 0x48 }, "584470002       ",
		{ 10,  0,  0,  5 }, { 10,  0,  0,  9 }, {  7,  1,  0, 13 },
	},
	{ /* Amphenol 3m 28awg NoEq */
		{ 0x41, 0x50, 0x48 }, "584470004       ",
		{  0,  0,  0,  8 }, {  0,  0,  0, 11 }, {  0,  1,  7, 15 },
	},
	{ /* Finisar 3m OM2 Optical */
		{ 0x00, 0x90, 0x65 }, "FCBG410QB1C03-QL",
		{  0,  0,  0,  3 }, {  0,  0,  0,  4 }, {  0,  0,  0, 13 },
	},
	{ /* Finisar 30m OM2 Optical */
		{ 0x00, 0x90, 0x65 }, "FCBG410QB1C30-QL",
		{  0,  0,  0,  1 }, {  0,  0,  0,  5 }, {  0,  0,  0, 11 },
	},
	{ /* Finisar Default OM2 Optical */
		{ 0x00, 0x90, 0x65 }, NULL,
		{  0,  0,  0,  2 }, {  0,  0,  0,  5 }, {  0,  0,  0, 12 },
	},
	{ /* Gore 1m 30awg NoEq */
		{ 0x00, 0x21, 0x77 }, "QSN3300-1       ",
		{  0,  0,  0,  6 }, {  0,  0,  0,  9 }, {  0,  1,  0, 15 },
	},
	{ /* Gore 2m 30awg NoEq */
		{ 0x00, 0x21, 0x77 }, "QSN3300-2       ",
		{  0,  0,  0,  8 }, {  0,  0,  0, 10 }, {  0,  1,  7, 15 },
	},
	{ /* Gore 1m 28awg NoEq */
		{ 0x00, 0x21, 0x77 }, "QSN3800-1       ",
		{  0,  0,  0,  6 }, {  0,  0,  0,  8 }, {  0,  1,  0, 15 },
	},
	{ /* Gore 3m 28awg NoEq */
		{ 0x00, 0x21, 0x77 }, "QSN3800-3       ",
		{  0,  0,  0,  9 }, {  0,  0,  0, 13 }, {  0,  1,  7, 15 },
	},
	{ /* Gore 5m 24awg Eq */
		{ 0x00, 0x21, 0x77 }, "QSN7000-5       ",
		{  0,  0,  0,  7 }, {  0,  0,  0,  9 }, {  0,  1,  3, 15 },
	},
	{ /* Gore 7m 24awg Eq */
		{ 0x00, 0x21, 0x77 }, "QSN7000-7       ",
		{  0,  0,  0,  9 }, {  0,  0,  0, 11 }, {  0,  2,  6, 15 },
	},
	{ /* Gore 5m 26awg Eq */
		{ 0x00, 0x21, 0x77 }, "QSN7600-5       ",
		{  0,  0,  0,  8 }, {  0,  0,  0, 11 }, {  0,  1,  9, 13 },
	},
	{ /* Gore 7m 26awg Eq */
		{ 0x00, 0x21, 0x77 }, "QSN7600-7       ",
		{  0,  0,  0,  8 }, {  0,  0,  0, 11 }, {  10,  1,  8, 15 },
	},
	{ /* Intersil 12m 24awg Active */
		{ 0x00, 0x30, 0xB4 }, "QLX4000CQSFP1224",
		{  0,  0,  0,  2 }, {  0,  0,  0,  5 }, {  0,  3,  0,  9 },
	},
	{ /* Intersil 10m 28awg Active */
		{ 0x00, 0x30, 0xB4 }, "QLX4000CQSFP1028",
		{  0,  0,  0,  6 }, {  0,  0,  0,  4 }, {  0,  2,  0,  2 },
	},
	{ /* Intersil 7m 30awg Active */
		{ 0x00, 0x30, 0xB4 }, "QLX4000CQSFP0730",
		{  0,  0,  0,  6 }, {  0,  0,  0,  4 }, {  0,  1,  0,  3 },
	},
	{ /* Intersil 5m 32awg Active */
		{ 0x00, 0x30, 0xB4 }, "QLX4000CQSFP0532",
		{  0,  0,  0,  6 }, {  0,  0,  0,  6 }, {  0,  2,  0,  8 },
	},
	{ /* Intersil Default Active */
		{ 0x00, 0x30, 0xB4 }, NULL,
		{  0,  0,  0,  6 }, {  0,  0,  0,  5 }, {  0,  2,  0,  5 },
	},
	{ /* Luxtera 20m Active Optical */
		{ 0x00, 0x25, 0x63 }, NULL,
		{  0,  0,  0,  5 }, {  0,  0,  0,  8 }, {  0,  2,  0,  12 },
	},
	{ /* Molex 1M Cu loopback */
		{ 0x00, 0x09, 0x3A }, "74763-0025      ",
		{  2,  2,  6, 15 }, {  2,  2,  6, 15 }, {  2,  2,  6, 15 },
	},
	{ /* Molex 2m 28awg NoEq */
		{ 0x00, 0x09, 0x3A }, "74757-2201      ",
		{  0,  0,  0,  6 }, {  0,  0,  0,  9 }, {  0,  1,  1, 15 },
	},
};

static const struct txdds_ent txdds_sdr[TXDDS_TABLE_SZ] = {
	/* amp, pre, main, post */
	{  2, 2, 15,  6 },	/* Loopback */
	{  0, 0,  0,  1 },	/*  2 dB */
	{  0, 0,  0,  2 },	/*  3 dB */
	{  0, 0,  0,  3 },	/*  4 dB */
	{  0, 0,  0,  4 },	/*  5 dB */
	{  0, 0,  0,  5 },	/*  6 dB */
	{  0, 0,  0,  6 },	/*  7 dB */
	{  0, 0,  0,  7 },	/*  8 dB */
	{  0, 0,  0,  8 },	/*  9 dB */
	{  0, 0,  0,  9 },	/* 10 dB */
	{  0, 0,  0, 10 },	/* 11 dB */
	{  0, 0,  0, 11 },	/* 12 dB */
	{  0, 0,  0, 12 },	/* 13 dB */
	{  0, 0,  0, 13 },	/* 14 dB */
	{  0, 0,  0, 14 },	/* 15 dB */
	{  0, 0,  0, 15 },	/* 16 dB */
};

static const struct txdds_ent txdds_ddr[TXDDS_TABLE_SZ] = {
	/* amp, pre, main, post */
	{  2, 2, 15,  6 },	/* Loopback */
	{  0, 0,  0,  8 },	/*  2 dB */
	{  0, 0,  0,  8 },	/*  3 dB */
	{  0, 0,  0,  9 },	/*  4 dB */
	{  0, 0,  0,  9 },	/*  5 dB */
	{  0, 0,  0, 10 },	/*  6 dB */
	{  0, 0,  0, 10 },	/*  7 dB */
	{  0, 0,  0, 11 },	/*  8 dB */
	{  0, 0,  0, 11 },	/*  9 dB */
	{  0, 0,  0, 12 },	/* 10 dB */
	{  0, 0,  0, 12 },	/* 11 dB */
	{  0, 0,  0, 13 },	/* 12 dB */
	{  0, 0,  0, 13 },	/* 13 dB */
	{  0, 0,  0, 14 },	/* 14 dB */
	{  0, 0,  0, 14 },	/* 15 dB */
	{  0, 0,  0, 15 },	/* 16 dB */
};

static const struct txdds_ent txdds_qdr[TXDDS_TABLE_SZ] = {
	/* amp, pre, main, post */
	{  2, 2, 15,  6 },	/* Loopback */
	{  0, 1,  0,  7 },	/*  2 dB (also QMH7342) */
	{  0, 1,  0,  9 },	/*  3 dB (also QMH7342) */
	{  0, 1,  0, 11 },	/*  4 dB */
	{  0, 1,  0, 13 },	/*  5 dB */
	{  0, 1,  0, 15 },	/*  6 dB */
	{  0, 1,  3, 15 },	/*  7 dB */
	{  0, 1,  7, 15 },	/*  8 dB */
	{  0, 1,  7, 15 },	/*  9 dB */
	{  0, 1,  8, 15 },	/* 10 dB */
	{  0, 1,  9, 15 },	/* 11 dB */
	{  0, 1, 10, 15 },	/* 12 dB */
	{  0, 2,  6, 15 },	/* 13 dB */
	{  0, 2,  7, 15 },	/* 14 dB */
	{  0, 2,  8, 15 },	/* 15 dB */
	{  0, 2,  9, 15 },	/* 16 dB */
};

/*
 * extra entries for use with txselect, for indices >= TXDDS_TABLE_SZ.
 * These are mostly used for mez cards going through connectors
 * and backplane traces, but can be used to add other "unusual"
 * table values as well.
 */
static const struct txdds_ent txdds_extra_sdr[TXDDS_EXTRA_SZ] = {
	/* amp, pre, main, post */
	{  0, 0, 0,  1 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  1 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  2 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  2 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  3 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  4 },	/* QMH7342 backplane settings */
	{  0, 1, 4, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 3, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0, 12 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0, 11 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0,  9 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0, 14 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 2, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0, 11 },       /* QME7342 backplane settings 1.1 */
	{  0, 1, 0,  7 },       /* QME7342 backplane settings 1.1 */
	{  0, 1, 0,  9 },       /* QME7342 backplane settings 1.1 */
	{  0, 1, 0,  6 },       /* QME7342 backplane settings 1.1 */
	{  0, 1, 0,  8 },       /* QME7342 backplane settings 1.1 */
};

static const struct txdds_ent txdds_extra_ddr[TXDDS_EXTRA_SZ] = {
	/* amp, pre, main, post */
	{  0, 0, 0,  7 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  7 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  8 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  8 },	/* QMH7342 backplane settings */
	{  0, 0, 0,  9 },	/* QMH7342 backplane settings */
	{  0, 0, 0, 10 },	/* QMH7342 backplane settings */
	{  0, 1, 4, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 3, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0, 12 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0, 11 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0,  9 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0, 14 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 2, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1, 0, 11 },       /* QME7342 backplane settings 1.1 */
	{  0, 1, 0,  7 },       /* QME7342 backplane settings 1.1 */
	{  0, 1, 0,  9 },       /* QME7342 backplane settings 1.1 */
	{  0, 1, 0,  6 },       /* QME7342 backplane settings 1.1 */
	{  0, 1, 0,  8 },       /* QME7342 backplane settings 1.1 */
};

static const struct txdds_ent txdds_extra_qdr[TXDDS_EXTRA_SZ] = {
	/* amp, pre, main, post */
	{  0, 1,  0,  4 },	/* QMH7342 backplane settings */
	{  0, 1,  0,  5 },	/* QMH7342 backplane settings */
	{  0, 1,  0,  6 },	/* QMH7342 backplane settings */
	{  0, 1,  0,  8 },	/* QMH7342 backplane settings */
	{  0, 1,  0, 10 },	/* QMH7342 backplane settings */
	{  0, 1,  0, 12 },	/* QMH7342 backplane settings */
	{  0, 1,  4, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1,  3, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1,  0, 12 },	/* QME7342 backplane settings 1.0 */
	{  0, 1,  0, 11 },	/* QME7342 backplane settings 1.0 */
	{  0, 1,  0,  9 },	/* QME7342 backplane settings 1.0 */
	{  0, 1,  0, 14 },	/* QME7342 backplane settings 1.0 */
	{  0, 1,  2, 15 },	/* QME7342 backplane settings 1.0 */
	{  0, 1,  0, 11 },      /* QME7342 backplane settings 1.1 */
	{  0, 1,  0,  7 },      /* QME7342 backplane settings 1.1 */
	{  0, 1,  0,  9 },      /* QME7342 backplane settings 1.1 */
	{  0, 1,  0,  6 },      /* QME7342 backplane settings 1.1 */
	{  0, 1,  0,  8 },      /* QME7342 backplane settings 1.1 */
};

static const struct txdds_ent txdds_extra_mfg[TXDDS_MFG_SZ] = {
	/* amp, pre, main, post */
	{ 0, 0, 0, 0 },         /* QME7342 mfg settings */
	{ 0, 0, 0, 6 },         /* QME7342 P2 mfg settings */
};

static const struct txdds_ent *get_atten_table(const struct txdds_ent *txdds,
					       unsigned atten)
{
	/*
	 * The attenuation table starts at 2dB for entry 1,
	 * with entry 0 being the loopback entry.
	 */
	if (atten <= 2)
		atten = 1;
	else if (atten > TXDDS_TABLE_SZ)
		atten = TXDDS_TABLE_SZ - 1;
	else
		atten--;
	return txdds + atten;
}

/*
 * if override is set, the module parameter txselect has a value
 * for this specific port, so use it, rather than our normal mechanism.
 */
static void find_best_ent(struct qib_pportdata *ppd,
			  const struct txdds_ent **sdr_dds,
			  const struct txdds_ent **ddr_dds,
			  const struct txdds_ent **qdr_dds, int override)
{
	struct qib_qsfp_cache *qd = &ppd->cpspec->qsfp_data.cache;
	int idx;

	/* Search table of known cables */
	for (idx = 0; !override && idx < ARRAY_SIZE(vendor_txdds); ++idx) {
		const struct vendor_txdds_ent *v = vendor_txdds + idx;

		if (!memcmp(v->oui, qd->oui, QSFP_VOUI_LEN) &&
		    (!v->partnum ||
		     !memcmp(v->partnum, qd->partnum, QSFP_PN_LEN))) {
			*sdr_dds = &v->sdr;
			*ddr_dds = &v->ddr;
			*qdr_dds = &v->qdr;
			return;
		}
	}

	/* Active cables don't have attenuation so we only set SERDES
	 * settings to account for the attenuation of the board traces. */
	if (!override && QSFP_IS_ACTIVE(qd->tech)) {
		*sdr_dds = txdds_sdr + ppd->dd->board_atten;
		*ddr_dds = txdds_ddr + ppd->dd->board_atten;
		*qdr_dds = txdds_qdr + ppd->dd->board_atten;
		return;
	}

	if (!override && QSFP_HAS_ATTEN(qd->tech) && (qd->atten[0] ||
						      qd->atten[1])) {
		*sdr_dds = get_atten_table(txdds_sdr, qd->atten[0]);
		*ddr_dds = get_atten_table(txdds_ddr, qd->atten[0]);
		*qdr_dds = get_atten_table(txdds_qdr, qd->atten[1]);
		return;
	} else if (ppd->cpspec->no_eep < TXDDS_TABLE_SZ) {
		/*
		 * If we have no (or incomplete) data from the cable
		 * EEPROM, or no QSFP, or override is set, use the
		 * module parameter value to index into the attentuation
		 * table.
		 */
		idx = ppd->cpspec->no_eep;
		*sdr_dds = &txdds_sdr[idx];
		*ddr_dds = &txdds_ddr[idx];
		*qdr_dds = &txdds_qdr[idx];
	} else if (ppd->cpspec->no_eep < (TXDDS_TABLE_SZ + TXDDS_EXTRA_SZ)) {
		/* similar to above, but index into the "extra" table. */
		idx = ppd->cpspec->no_eep - TXDDS_TABLE_SZ;
		*sdr_dds = &txdds_extra_sdr[idx];
		*ddr_dds = &txdds_extra_ddr[idx];
		*qdr_dds = &txdds_extra_qdr[idx];
	} else if ((IS_QME(ppd->dd) || IS_QMH(ppd->dd)) &&
		   ppd->cpspec->no_eep < (TXDDS_TABLE_SZ + TXDDS_EXTRA_SZ +
					  TXDDS_MFG_SZ)) {
		idx = ppd->cpspec->no_eep - (TXDDS_TABLE_SZ + TXDDS_EXTRA_SZ);
		pr_info("IB%u:%u use idx %u into txdds_mfg\n",
			ppd->dd->unit, ppd->port, idx);
		*sdr_dds = &txdds_extra_mfg[idx];
		*ddr_dds = &txdds_extra_mfg[idx];
		*qdr_dds = &txdds_extra_mfg[idx];
	} else {
		/* this shouldn't happen, it's range checked */
		*sdr_dds = txdds_sdr + qib_long_atten;
		*ddr_dds = txdds_ddr + qib_long_atten;
		*qdr_dds = txdds_qdr + qib_long_atten;
	}
}

static void init_txdds_table(struct qib_pportdata *ppd, int override)
{
	const struct txdds_ent *sdr_dds, *ddr_dds, *qdr_dds;
	struct txdds_ent *dds;
	int idx;
	int single_ent = 0;

	find_best_ent(ppd, &sdr_dds, &ddr_dds, &qdr_dds, override);

	/* for mez cards or override, use the selected value for all entries */
	if (!(ppd->dd->flags & QIB_HAS_QSFP) || override)
		single_ent = 1;

	/* Fill in the first entry with the best entry found. */
	set_txdds(ppd, 0, sdr_dds);
	set_txdds(ppd, TXDDS_TABLE_SZ, ddr_dds);
	set_txdds(ppd, 2 * TXDDS_TABLE_SZ, qdr_dds);
	if (ppd->lflags & (QIBL_LINKINIT | QIBL_LINKARMED |
		QIBL_LINKACTIVE)) {
		dds = (struct txdds_ent *)(ppd->link_speed_active ==
					   QIB_IB_QDR ?  qdr_dds :
					   (ppd->link_speed_active ==
					    QIB_IB_DDR ? ddr_dds : sdr_dds));
		write_tx_serdes_param(ppd, dds);
	}

	/* Fill in the remaining entries with the default table values. */
	for (idx = 1; idx < ARRAY_SIZE(txdds_sdr); ++idx) {
		set_txdds(ppd, idx, single_ent ? sdr_dds : txdds_sdr + idx);
		set_txdds(ppd, idx + TXDDS_TABLE_SZ,
			  single_ent ? ddr_dds : txdds_ddr + idx);
		set_txdds(ppd, idx + 2 * TXDDS_TABLE_SZ,
			  single_ent ? qdr_dds : txdds_qdr + idx);
	}
}

#define KR_AHB_ACC KREG_IDX(ahb_access_ctrl)
#define KR_AHB_TRANS KREG_IDX(ahb_transaction_reg)
#define AHB_TRANS_RDY SYM_MASK(ahb_transaction_reg, ahb_rdy)
#define AHB_ADDR_LSB SYM_LSB(ahb_transaction_reg, ahb_address)
#define AHB_DATA_LSB SYM_LSB(ahb_transaction_reg, ahb_data)
#define AHB_WR SYM_MASK(ahb_transaction_reg, write_not_read)
#define AHB_TRANS_TRIES 10

/*
 * The chan argument is 0=chan0, 1=chan1, 2=pll, 3=chan2, 4=chan4,
 * 5=subsystem which is why most calls have "chan + chan >> 1"
 * for the channel argument.
 */
static u32 ahb_mod(struct qib_devdata *dd, int quad, int chan, int addr,
		    u32 data, u32 mask)
{
	u32 rd_data, wr_data, sz_mask;
	u64 trans, acc, prev_acc;
	u32 ret = 0xBAD0BAD;
	int tries;

	prev_acc = qib_read_kreg64(dd, KR_AHB_ACC);
	/* From this point on, make sure we return access */
	acc = (quad << 1) | 1;
	qib_write_kreg(dd, KR_AHB_ACC, acc);

	for (tries = 1; tries < AHB_TRANS_TRIES; ++tries) {
		trans = qib_read_kreg64(dd, KR_AHB_TRANS);
		if (trans & AHB_TRANS_RDY)
			break;
	}
	if (tries >= AHB_TRANS_TRIES) {
		qib_dev_err(dd, "No ahb_rdy in %d tries\n", AHB_TRANS_TRIES);
		goto bail;
	}

	/* If mask is not all 1s, we need to read, but different SerDes
	 * entities have different sizes
	 */
	sz_mask = (1UL << ((quad == 1) ? 32 : 16)) - 1;
	wr_data = data & mask & sz_mask;
	if ((~mask & sz_mask) != 0) {
		trans = ((chan << 6) | addr) << (AHB_ADDR_LSB + 1);
		qib_write_kreg(dd, KR_AHB_TRANS, trans);

		for (tries = 1; tries < AHB_TRANS_TRIES; ++tries) {
			trans = qib_read_kreg64(dd, KR_AHB_TRANS);
			if (trans & AHB_TRANS_RDY)
				break;
		}
		if (tries >= AHB_TRANS_TRIES) {
			qib_dev_err(dd, "No Rd ahb_rdy in %d tries\n",
				    AHB_TRANS_TRIES);
			goto bail;
		}
		/* Re-read in case host split reads and read data first */
		trans = qib_read_kreg64(dd, KR_AHB_TRANS);
		rd_data = (uint32_t)(trans >> AHB_DATA_LSB);
		wr_data |= (rd_data & ~mask & sz_mask);
	}

	/* If mask is not zero, we need to write. */
	if (mask & sz_mask) {
		trans = ((chan << 6) | addr) << (AHB_ADDR_LSB + 1);
		trans |= ((uint64_t)wr_data << AHB_DATA_LSB);
		trans |= AHB_WR;
		qib_write_kreg(dd, KR_AHB_TRANS, trans);

		for (tries = 1; tries < AHB_TRANS_TRIES; ++tries) {
			trans = qib_read_kreg64(dd, KR_AHB_TRANS);
			if (trans & AHB_TRANS_RDY)
				break;
		}
		if (tries >= AHB_TRANS_TRIES) {
			qib_dev_err(dd, "No Wr ahb_rdy in %d tries\n",
				    AHB_TRANS_TRIES);
			goto bail;
		}
	}
	ret = wr_data;
bail:
	qib_write_kreg(dd, KR_AHB_ACC, prev_acc);
	return ret;
}

static void ibsd_wr_allchans(struct qib_pportdata *ppd, int addr, unsigned data,
			     unsi