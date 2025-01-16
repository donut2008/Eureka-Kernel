 skip to next, if any */
				;
			continue;
		}
		str = ++nxt;
		port = simple_strtoul(str, &nxt, 0);
		if (nxt == str || *nxt != '=') {
			while (*nxt && *nxt++ != ' ') /* skip to next, if any */
				;
			continue;
		}
		str = ++nxt;
		val = simple_strtoul(str, &nxt, 0);
		if (nxt == str) {
			while (*nxt && *nxt++ != ' ') /* skip to next, if any */
				;
			continue;
		}
		if (val >= txdds_size)
			continue;
		seth1 = 0;
		h1 = 0; /* gcc thinks it might be used uninitted */
		if (*nxt == ',' && nxt[1]) {
			str = ++nxt;
			h1 = (u32)simple_strtoul(str, &nxt, 0);
			if (nxt == str)
				while (*nxt && *nxt++ != ' ') /* skip */
					;
			else
				seth1 = 1;
		}
		for (pidx = 0; dd->unit == unit && pidx < dd->num_pports;
		     ++pidx) {
			struct qib_pportdata *ppd = &dd->pport[pidx];

			if (ppd->port != port || !ppd->link_speed_supported)
				continue;
			ppd->cpspec->no_eep = val;
			if (seth1)
				ppd->cpspec->h1_val = h1;
			/* now change the IBC and serdes, overriding generic */
			init_txdds_table(ppd, 1);
			/* Re-enable the physical state machine on mezz boards
			 * now that the correct settings have been set.
			 * QSFP boards are handles by the QSFP event handler */
			if (IS_QMH(dd) || IS_QME(dd))
				qib_set_ib_7322_lstate(ppd, 0,
					    QLOGIC_IB_IBCC_LINKINITCMD_SLEEP);
			any++;
		}
		if (*nxt == '\n')
			break; /* done */
	}
	if (change && !any) {
		/* no specific setting, use the default.
		 * Change the IBC and serdes, but since it's
		 * general, don't override specific settings.
		 */
		for (pidx = 0; pidx < dd->num_pports; ++pidx)
			if (dd->pport[pidx].link_speed_supported)
				init_txdds_table(&dd->pport[pidx], 0);
	}
}

/* handle the txselect parameter changing */
static int setup_txselect(const char *str, struct kernel_param *kp)
{
	struct qib_devdata *dd;
	unsigned long val;
	char *n;

	if (strlen(str) >= MAX_ATTEN_LEN) {
		pr_info("txselect_values string too long\n");
		return -ENOSPC;
	}
	val = simple_strtoul(str, &n, 0);
	if (n == str || val >= (TXDDS_TABLE_SZ + TXDDS_EXTRA_SZ +
				TXDDS_MFG_SZ)) {
		pr_info("txselect_values must start with a number < %d\n",
			TXDDS_TABLE_SZ + TXDDS_EXTRA_SZ + TXDDS_MFG_SZ);
		return -EINVAL;
	}
	strcpy(txselect_list, str);

	list_for_each_entry(dd, &qib_dev_list, list)
		if (dd->deviceid == PCI_DEVICE_ID_QLOGIC_IB_7322)
			set_no_qsfp_atten(dd, 1);
	return 0;
}

/*
 * Write the final few registers that depend on some of the
 * init setup.  Done late in init, just before bringing up
 * the serdes.
 */
static int qib_late_7322_initreg(struct qib_devdata *dd)
{
	int ret = 0, n;
	u64 val;

	qib_write_kreg(dd, kr_rcvhdrentsize, dd->rcvhdrentsize);
	qib_write_kreg(dd, kr_rcvhdrsize, dd->rcvhdrsize);
	qib_write_kreg(dd, kr_rcvhdrcnt, dd->rcvhdrcnt);
	qib_write_kreg(dd, kr_sendpioavailaddr, dd->pioavailregs_phys);
	val = qib_read_kreg64(dd, kr_sendpioavailaddr);
	if (val != dd->pioavailregs_phys) {
		qib_dev_err(dd,
			"Catastrophic software error, SendPIOAvailAddr written as %lx, read back as %llx\n",
			(unsigned long) dd->pioavailregs_phys,
			(unsigned long long) val);
		ret = -EINVAL;
	}

	n = dd->piobcnt2k + dd->piobcnt4k + NUM_VL15_BUFS;
	qib_7322_txchk_change(dd, 0, n, TXCHK_CHG_TYPE_KERN, NULL);
	/* driver sends get pkey, lid, etc. checking also, to catch bugs */
	qib_7322_txchk_change(dd, 0, n, TXCHK_CHG_TYPE_ENAB1, NULL);

	qib_register_observer(dd, &sendctrl_0_observer);
	qib_register_observer(dd, &sendctrl_1_observer);

	dd->control &= ~QLOGIC_IB_C_SDMAFETCHPRIOEN;
	qib_write_kreg(dd, kr_control, dd->control);
	/*
	 * Set SendDmaFetchPriority and init Tx params, including
	 * QSFP handler on boards that have QSFP.
	 * First set our default attenuation entry for cables that
	 * don't have valid attenuation.
	 */
	set_no_qsfp_atten(dd, 0);
	for (n = 0; n < dd->num_pports; ++n) {
		struct qib_pportdata *ppd = dd->pport + n;

		qib_write_kreg_port(ppd, krp_senddmaprioritythld,
				    sdma_fetch_prio & 0xf);
		/* Initialize qsfp if present on board. */
		if (dd->flags & QIB_HAS_QSFP)
			qib_init_7322_qsfp(ppd);
	}
	dd->control |= QLOGIC_IB_C_SDMAFETCHPRIOEN;
	qib_write_kreg(dd, kr_control, dd->control);

	return ret;
}

/* per IB port errors.  */
#define SENDCTRL_PIBP (MASK_ACROSS(0, 1) | MASK_ACROSS(3, 3) | \
	MASK_ACROSS(8, 15))
#define RCVCTRL_PIBP (MASK_ACROSS(0, 17) | MASK_ACROSS(39, 41))
#define ERRS_PIBP (MASK_ACROSS(57, 58) | MASK_ACROSS(54, 54) | \
	MASK_ACROSS(36, 49) | MASK_ACROSS(29, 34) | MASK_ACROSS(14, 17) | \
	MASK_ACROSS(0, 11))

/*
 * Write the initialization per-port registers that need to be done at
 * driver load and after reset completes (i.e., that aren't done as part
 * of other init procedures called from qib_init.c).
 * Some of these should be redundant on reset, but play safe.
 */
static void write_7322_init_portregs(struct qib_pportdata *ppd)
{
	u64 val;
	int i;

	if (!ppd->link_speed_supported) {
		/* no buffer credits for this port */
		for (i = 1; i < 8; i++)
			qib_write_kreg_port(ppd, krp_rxcreditvl0 + i, 0);
		qib_write_kreg_port(ppd, krp_ibcctrl_b, 0);
		qib_write_kreg(ppd->dd, kr_scratch, 0);
		return;
	}

	/*
	 * Set the number of supported virtual lanes in IBC,
	 * for flow control packet handling on unsupported VLs
	 */
	val = qib_read_kreg_port(ppd, krp_ibsdtestiftx);
	val &= ~SYM_MASK(IB_SDTEST_IF_TX_0, VL_CAP);
	val |= (u64)(ppd->vls_supported - 1) <<
		SYM_LSB(IB_SDTEST_IF_TX_0, VL_CAP);
	qib_write_kreg_port(ppd, krp_ibsdtestiftx, val);

	qib_write_kreg_port(ppd, krp_rcvbthqp, QIB_KD_QP);

	/* enable tx header checking */
	qib_write_kreg_port(ppd, krp_sendcheckcontrol, IBA7322_SENDCHK_PKEY |
			    IBA7322_SENDCHK_BTHQP | IBA7322_SENDCHK_SLID |
			    IBA7322_SENDCHK_RAW_IPV6 | IBA7322_SENDCHK_MINSZ);

	qib_write_kreg_port(ppd, krp_ncmodectrl,
		SYM_MASK(IBNCModeCtrl_0, ScrambleCapLocal));

	/*
	 * Unconditionally clear the bufmask bits.  If SDMA is
	 * enabled, we'll set them appropriately later.
	 */
	qib_write_kreg_port(ppd, krp_senddmabufmask0, 0);
	qib_write_kreg_port(ppd, krp_senddmabufmask1, 0);
	qib_write_kreg_port(ppd, krp_senddmabufmask2, 0);
	if (ppd->dd->cspec->r1)
		ppd->p_sendctrl |= SYM_MASK(SendCtrl_0, ForceCreditUpToDate);
}

/*
 * Write the initialization per-device registers that need to be done at
 * driver load and after reset completes (i.e., that aren't done as part
 * of other init procedures called from qib_init.c).  Also write per-port
 * registers that are affected by overall device config, such as QP mapping
 * Some of these should be redundant on reset, but play safe.
 */
static void write_7322_initregs(struct qib_devdata *dd)
{
	struct qib_pportdata *ppd;
	int i, pidx;
	u64 val;

	/* Set Multicast QPs received by port 2 to map to context one. */
	qib_write_kreg(dd, KREG_IDX(RcvQPMulticastContext_1), 1);

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		unsigned n, regno;
		unsigned long flags;

		if (dd->n_krcv_queues < 2 ||
			!dd->pport[pidx].link_speed_supported)
			continue;

		ppd = &dd->pport[pidx];

		/* be paranoid against later code motion, etc. */
		spin_lock_irqsave(&dd->cspec->rcvmod_lock, flags);
		ppd->p_rcvctrl |= SYM_MASK(RcvCtrl_0, RcvQPMapEnable);
		spin_unlock_irqrestore(&dd->cspec->rcvmod_lock, flags);

		/* Initialize QP to context mapping */
		regno = krp_rcvqpmaptable;
		val = 0;
		if (dd->num_pports > 1)
			n = dd->first_user_ctxt / dd->num_pports;
		else
			n = dd->first_user_ctxt - 1;
		for (i = 0; i < 32; ) {
			unsigned ctxt;

			if (dd->num_pports > 1)
				ctxt = (i % n) * dd->num_pports + pidx;
			else if (i % n)
				ctxt = (i % n) + 1;
			else
				ctxt = ppd->hw_pidx;
			val |= ctxt << (5 * (i % 6));
			i++;
			if (i % 6 == 0) {
				qib_write_kreg_port(ppd, regno, val);
				val = 0;
				regno++;
			}
		}
		qib_write_kreg_port(ppd, regno, val);
	}

	/*
	 * Setup up interrupt mitigation for kernel contexts, but
	 * not user contexts (user contexts use interrupts when
	 * stalled waiting for any packet, so want those interrupts
	 * right away).
	 */
	for (i = 0; i < dd->first_user_ctxt; i++) {
		dd->cspec->rcvavail_timeout[i] = rcv_int_timeout;
		qib_write_kreg(dd, kr_rcvavailtimeout + i, rcv_int_timeout);
	}

	/*
	 * Initialize  as (disabled) rcvflow tables.  Application code
	 * will setup each flow as it uses the flow.
	 * Doesn't clear any of the error bits that might be set.
	 */
	val = TIDFLOW_ERRBITS; /* these are W1C */
	for (i = 0; i < dd->cfgctxts; i++) {
		int flow;

		for (flow = 0; flow < NUM_TIDFLOWS_CTXT; flow++)
			qib_write_ureg(dd, ur_rcvflowtable+flow, val, i);
	}

	/*
	 * dual cards init to dual port recovery, single port cards to
	 * the one port.  Dual port cards may later adjust to 1 port,
	 * and then back to dual port if both ports are connected
	 * */
	if (dd->num_pports)
		setup_7322_link_recovery(dd->pport, dd->num_pports > 1);
}

static int qib_init_7322_variables(struct qib_devdata *dd)
{
	struct qib_pportdata *ppd;
	unsigned features, pidx, sbufcnt;
	int ret, mtu;
	u32 sbufs, updthresh;
	resource_size_t vl15off;

	/* pport structs are contiguous, allocated after devdata */
	ppd = (struct qib_pportdata *)(dd + 1);
	dd->pport = ppd;
	ppd[0].dd = dd;
	ppd[1].dd = dd;

	dd->cspec = (struct qib_chip_specific *)(ppd + 2);

	ppd[0].cpspec = (struct qib_chippport_specific *)(dd->cspec + 1);
	ppd[1].cpspec = &ppd[0].cpspec[1];
	ppd[0].cpspec->ppd = &ppd[0]; /* for autoneg_7322_work() */
	ppd[1].cpspec->ppd = &ppd[1]; /* for autoneg_7322_work() */

	spin_lock_init(&dd->cspec->rcvmod_lock);
	spin_lock_init(&dd->cspec->gpio_lock);

	/* we haven't yet set QIB_PRESENT, so use read directly */
	dd->revision = readq(&dd->kregbase[kr_revision]);

	if ((dd->revision & 0xffffffffU) == 0xffffffffU) {
		qib_dev_err(dd,
			"Revision register read failure, giving up initialization\n");
		ret = -ENODEV;
		goto bail;
	}
	dd->flags |= QIB_PRESENT;  /* now register routines work */

	dd->majrev = (u8) SYM_FIELD(dd->revision, Revision_R, ChipRevMajor);
	dd->minrev = (u8) SYM_FIELD(dd->revision, Revision_R, ChipRevMinor);
	dd->cspec->r1 = dd->minrev == 1;

	get_7322_chip_params(dd);
	features = qib_7322_boardname(dd);

	/* now that piobcnt2k and 4k set, we can allocate these */
	sbufcnt = dd->piobcnt2k + dd->piobcnt4k +
		NUM_VL15_BUFS + BITS_PER_LONG - 1;
	sbufcnt /= BITS_PER_LONG;
	dd->cspec->sendchkenable = kmalloc(sbufcnt *
		sizeof(*dd->cspec->sendchkenable), GFP_KERNEL);
	dd->cspec->sendgrhchk = kmalloc(sbufcnt *
		sizeof(*dd->cspec->sendgrhchk), GFP_KERNEL);
	dd->cspec->sendibchk = kmalloc(sbufcnt *
		sizeof(*dd->cspec->sendibchk), GFP_KERNEL);
	if (!dd->cspec->sendchkenable || !dd->cspec->sendgrhchk ||
		!dd->cspec->sendibchk) {
		qib_dev_err(dd, "Failed allocation for hdrchk bitmaps\n");
		ret = -ENOMEM;
		goto bail;
	}

	ppd = dd->pport;

	/*
	 * GPIO bits for TWSI data and clock,
	 * used for serial EEPROM.
	 */
	dd->gpio_sda_num = _QIB_GPIO_SDA_NUM;
	dd->gpio_scl_num = _QIB_GPIO_SCL_NUM;
	dd->twsi_eeprom_dev = QIB_TWSI_EEPROM_DEV;

	dd->flags |= QIB_HAS_INTX | QIB_HAS_LINK_LATENCY |
		QIB_NODMA_RTAIL | QIB_HAS_VLSUPP | QIB_HAS_HDRSUPP |
		QIB_HAS_THRESH_UPDATE |
		(sdma_idle_cnt ? QIB_HAS_SDMA_TIMEOUT : 0);
	dd->flags |= qib_special_trigger ?
		QIB_USE_SPCL_TRIG : QIB_HAS_SEND_DMA;

	/*
	 * Setup initial values.  These may change when PAT is enabled, but
	 * we need these to do initial chip register accesses.
	 */
	qib_7322_set_baseaddrs(dd);

	mtu = ib_mtu_enum_to_int(qib_ibmtu);
	if (mtu == -1)
		mtu = QIB_DEFAULT_MTU;

	dd->cspec->int_enable_mask = QIB_I_BITSEXTANT;
	/* all hwerrors become interrupts, unless special purposed */
	dd->cspec->hwerrmask = ~0ULL;
	/*  link_recovery setup causes these errors, so ignore them,
	 *  other than clearing them when they occur */
	dd->cspec->hwerrmask &=
		~(SYM_MASK(HwErrMask, IBSerdesPClkNotDetectMask_0) |
		  SYM_MASK(HwErrMask, IBSerdesPClkNotDetectMask_1) |
		  HWE_MASK(LATriggered));

	for (pidx = 0; pidx < NUM_IB_PORTS; ++pidx) {
		struct qib_chippport_specific *cp = ppd->cpspec;

		ppd->link_speed_supported = features & PORT_SPD_CAP;
		features >>=  PORT_SPD_CAP_SHIFT;
		if (!ppd->link_speed_supported) {
			/* single port mode (7340, or configured) */
			dd->skip_kctxt_mask |= 1 << pidx;
			if (pidx == 0) {
				/* Make sure port is disabled. */
				qib_write_kreg_port(ppd, krp_rcvctrl, 0);
				qib_write_kreg_port(ppd, krp_ibcctrl_a, 0);
				ppd[0] = ppd[1];
				dd->cspec->hwerrmask &= ~(SYM_MASK(HwErrMask,
						  IBSerdesPClkNotDetectMask_0)
						  | SYM_MASK(HwErrMask,
						  SDmaMemReadErrMask_0));
				dd->cspec->int_enable_mask &= ~(
				     SYM_MASK(IntMask, SDmaCleanupDoneMask_0) |
				     SYM_MASK(IntMask, SDmaIdleIntMask_0) |
				     SYM_MASK(IntMask, SDmaProgressIntMask_0) |
				     SYM_MASK(IntMask, SDmaIntMask_0) |
				     SYM_MASK(IntMask, ErrIntMask_0) |
				     SYM_MASK(IntMask, SendDoneIntMask_0));
			} else {
				/* Make sure port is disabled. */
				qib_write_kreg_port(ppd, krp_rcvctrl, 0);
				qib_write_kreg_port(ppd, krp_ibcctrl_a, 0);
				dd->cspec->hwerrmask &= ~(SYM_MASK(HwErrMask,
						  IBSerdesPClkNotDetectMask_1)
						  | SYM_MASK(HwErrMask,
						  SDmaMemReadErrMask_1));
				dd->cspec->int_enable_mask &= ~(
				     SYM_MASK(IntMask, SDmaCleanupDoneMask_1) |
				     SYM_MASK(IntMask, SDmaIdleIntMask_1) |
				     SYM_MASK(IntMask, SDmaProgressIntMask_1) |
				     SYM_MASK(IntMask, SDmaIntMask_1) |
				     SYM_MASK(IntMask, ErrIntMask_1) |
				     SYM_MASK(IntMask, SendDoneIntMask_1));
			}
			continue;
		}

		dd->num_pports++;
		ret = qib_init_pportdata(ppd, dd, pidx, dd->num_pports);
		if (ret) {
			dd->num_pports--;
			goto bail;
		}

		ppd->link_width_supported = IB_WIDTH_1X | IB_WIDTH_4X;
		ppd->link_width_enabled = IB_WIDTH_4X;
		ppd->link_speed_enabled = ppd->link_speed_supported;
		/*
		 * Set the initial values to reasonable default, will be set
		 * for real when link is up.
		 */
		ppd->link_width_active = IB_WIDTH_4X;
		ppd->link_speed_active = QIB_IB_SDR;
		ppd->delay_mult = ib_rate_to_delay[IB_RATE_10_GBPS];
		switch (qib_num_cfg_vls) {
		case 1:
			ppd->vls_supported = IB_VL_VL0;
			break;
		case 2:
			ppd->vls_supported = IB_VL_VL0_1;
			break;
		default:
			qib_devinfo(dd->pcidev,
				    "Invalid num_vls %u, using 4 VLs\n",
				    qib_num_cfg_vls);
			qib_num_cfg_vls = 4;
			/* fall through */
		case 4:
			ppd->vls_supported = IB_VL_VL0_3;
			break;
		case 8:
			if (mtu <= 2048)
				ppd->vls_supported = IB_VL_VL0_7;
			else {
				qib_devinfo(dd->pcidev,
					    "Invalid num_vls %u for MTU %d , using 4 VLs\n",
					    qib_num_cfg_vls, mtu);
				ppd->vls_supported = IB_VL_VL0_3;
				qib_num_cfg_vls = 4;
			}
			break;
		}
		ppd->vls_operational = ppd->vls_supported;

		init_waitqueue_head(&cp->autoneg_wait);
		INIT_DELAYED_WORK(&cp->autoneg_work,
				  autoneg_7322_work);
		if (ppd->dd->cspec->r1)
			INIT_DELAYED_WORK(&cp->ipg_work, ipg_7322_work);

		/*
		 * For Mez and similar cards, no qsfp info, so do
		 * the "cable info" setup here.  Can be overridden
		 * in adapter-specific routines.
		 */
		if (!(dd->flags & QIB_HAS_QSFP)) {
			if (!IS_QMH(dd) && !IS_QME(dd))
				qib_devinfo(dd->pcidev,
					"IB%u:%u: Unknown mezzanine card type\n",
					dd->unit, ppd->port);
			cp->h1_val = IS_QMH(dd) ? H1_FORCE_QMH : H1_FORCE_QME;
			/*
			 * Choose center value as default tx serdes setting
			 * until changed through module parameter.
			 */
			ppd->cpspec->no_eep = IS_QMH(dd) ?
				TXDDS_TABLE_SZ + 2 : TXDDS_TABLE_SZ + 4;
		} else
			cp->h1_val = H1_FORCE_VAL;

		/* Avoid writes to chip for mini_init */
		if (!qib_mini_init)
			write_7322_init_portregs(ppd);

		init_timer(&cp->chase_timer);
		cp->chase_timer.function = reenable_chase;
		cp->chase_timer.data = (unsigned long)ppd;

		ppd++;
	}

	dd->rcvhdrentsize = qib_rcvhdrentsize ?
		qib_rcvhdrentsize : QIB_RCVHDR_ENTSIZE;
	dd->rcvhdrsize = qib_rcvhdrsize ?
		qib_rcvhdrsize : QIB_DFLT_RCVHDRSIZE;
	dd->rhf_offset = dd->rcvhdrentsize - sizeof(u64) / sizeof(u32);

	/* we always allocate at least 2048 bytes for eager buffers */
	dd->rcvegrbufsize = max(mtu, 2048);
	BUG_ON(!is_power_of_2(dd->rcvegrbufsize));
	dd->rcvegrbufsize_shift = ilog2(dd->rcvegrbufsize);

	qib_7322_tidtemplate(dd);

	/*
	 * We can request a receive interrupt for 1 or
	 * more packets from current offset.
	 */
	dd->rhdrhead_intr_off =
		(u64) rcv_int_count << IBA7322_HDRHEAD_PKTINT_SHIFT;

	/* setup the stats timer; the add_timer is done at end of init */
	init_timer(&dd->stats_timer);
	dd->stats_timer.function = qib_get_7322_faststats;
	dd->stats_timer.data = (unsigned long) dd;

	dd->ureg_align = 0x10000;  /* 64KB alignment */

	dd->piosize2kmax_dwords = dd->piosize2k >> 2;

	qib_7322_config_ctxts(dd);
	qib_set_ctxtcnt(dd);

	/*
	 * We do not set WC on the VL15 buffers to avoid
	 * a rare problem with unaligned writes from
	 * interrupt-flushed store buffers, so we need
	 * to map those separately here.  We can't solve
	 * this for the rarely used mtrr case.
	 */
	ret = init_chip_wc_pat(dd, 0);
	if (ret)
		goto bail;

	/* vl15 buffers start just after the 4k buffers */
	vl15off = dd-