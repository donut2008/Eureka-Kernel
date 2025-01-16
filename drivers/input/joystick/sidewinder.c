/
		};

	dcnt = ARRAY_SIZE(madpayload_start);
	hcnt = ARRAY_SIZE(hdr);
	if (!swapped) {
		/* for maintainability, do it at runtime */
		for (i = 0; i < hcnt; i++) {
			dw = (__force u32) cpu_to_be32(hdr[i]);
			hdr[i] = dw;
		}
		for (i = 0; i < dcnt; i++) {
			dw = (__force u32) cpu_to_be32(madpayload_start[i]);
			madpayload_start[i] = dw;
			dw = (__force u32) cpu_to_be32(madpayload_done[i]);
			madpayload_done[i] = dw;
		}
		swapped = 1;
	}

	data = which ? madpayload_done : madpayload_start;

	autoneg_7322_sendpkt(ppd, hdr, dcnt, data);
	qib_read_kreg64(dd, kr_scratch);
	udelay(2);
	autoneg_7322_sendpkt(ppd, hdr, dcnt, data);
	qib_read_kreg64(dd, kr_scratch);
	udelay(2);
}

/*
 * Do the absolute minimum to cause an IB speed change, and make it
 * ready, but don't actually trigger the change.   The caller will
 * do that when ready (if link is in Polling training state, it will
 * happen immediately, otherwise when link next goes down)
 *
 * This routine should only be used as part of the DDR autonegotation
 * code for devices that are not compliant with IB 1.2 (or code that
 * fixes things up for same).
 *
 * When link has gone down, and autoneg enabled, or autoneg has
 * failed and we give up until next time we set both speeds, and
 * then we want IBTA enabled as well as "use max enabled speed.
 */
static void set_7322_ibspeed_fast(struct qib_pportdata *ppd, u32 speed)
{
	u64 newctrlb;

	newctrlb = ppd->cpspec->ibcctrl_b & ~(IBA7322_IBC_SPEED_MASK |
				    IBA7322_IBC_IBTA_1_2_MASK |
				    IBA7322_IBC_MAX_SPEED_MASK);

	if (speed & (speed - 1)) /* multiple speeds */
		newctrlb |= (speed << IBA7322_IBC_SPEED_LSB) |
				    IBA7322_IBC_IBTA_1_2_MASK |
				    IBA7322_IBC_MAX_SPEED_MASK;
	else
		newctrlb |= speed == QIB_IB_QDR ?
			IBA7322_IBC_SPEED_QDR | IBA7322_IBC_IBTA_1_2_MASK :
			((speed == QIB_IB_DDR ?
			  IBA7322_IBC_SPEED_DDR : IBA7322_IBC_SPEED_SDR));

	if (newctrlb == ppd->cpspec->ibcctrl_b)
		return;

	ppd->cpspec->ibcctrl_b = newctrlb;
	qib_write_kreg_port(ppd, krp_ibcctrl_b, ppd->cpspec->ibcctrl_b);
	qib_write_kreg(ppd->dd, kr_scratch, 0);
}

/*
 * This routine is only used when we are not talking to another
 * IB 1.2-compliant device that we think can do DDR.
 * (This includes all existing switch chips as of Oct 2007.)
 * 1.2-compliant devices go directly to DDR prior to reaching INIT
 */
static void try_7322_autoneg(struct qib_pportdata *ppd)
{
	unsigned long flags;

	spin_lock_irqsave(&ppd->lflags_lock, flags);
	ppd->lflags |= QIBL_IB_AUTONEG_INPROG;
	spin_unlock_irqrestore(&ppd->lflags_lock, flags);
	qib_autoneg_7322_send(ppd, 0);
	set_7322_ibspeed_fast(ppd, QIB_IB_DDR);
	qib_7322_mini_pcs_reset(ppd);
	/* 2 msec is minimum length of a poll cycle */
	queue_delayed_work(ib_wq, &ppd->cpspec->autoneg_work,
			   msecs_to_jiffies(2));
}

/*
 * Handle the empirically determined mechanism for auto-negotiation
 * of DDR speed with switches.
 */
static void autoneg_7322_work(struct work_struct *work)
{
	struct qib_pportdata *ppd;
	struct qib_devdata *dd;
	u64 startms;
	u32 i;
	unsigned long flags;

	ppd = container_of(work, struct qib_chippport_specific,
			    autoneg_work.work)->ppd;
	dd = ppd->dd;

	startms = jiffies_to_msecs(jiffies);

	/*
	 * Busy wait for this first part, it should be at most a
	 * few hundred usec, since we scheduled ourselves for 2msec.
	 */
	for (i = 0; i < 25; i++) {
		if (SYM_FIELD(ppd->lastibcstat, IBCStatusA_0, LinkState)
		     == IB_7322_LT_STATE_POLLQUIET) {
			qib_set_linkstate(ppd, QIB_IB_LINKDOWN_DISABLE);
			break;
		}
		udelay(100);
	}

	if (!(ppd->lflags & QIBL_IB_AUTONEG_INPROG))
		goto done; /* we got there early or told to stop */

	/* we expect this to timeout */
	if (wait_event_timeout(ppd->cpspec->autoneg_wait,
			       !(ppd->lflags & QIBL_IB_AUTONEG_INPROG),
			       msecs_to_jiffies(90)))
		goto done;
	qib_7322_mini_pcs_reset(ppd);

	/* we expect this to timeout */
	if (wait_event_timeout(ppd->cpspec->autoneg_wait,
			       !(ppd->lflags & QIBL_IB_AUTONEG_INPROG),
			       msecs_to_jiffies(1700)))
		goto done;
	qib_7322_mini_pcs_reset(ppd);

	set_7322_ibspeed_fast(ppd, QIB_IB_SDR);

	/*
	 * Wait up to 250 msec for link to train and get to INIT at DDR;
	 * this should terminate early.
	 */
	wait_event_timeout(ppd->cpspec->autoneg_wait,
		!(ppd->lflags & QIBL_IB_AUTONEG_INPROG),
		msecs_to_jiffies(250));
done:
	if (ppd->lflags & QIBL_IB_AUTONEG_INPROG) {
		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags &= ~QIBL_IB_AUTONEG_INPROG;
		if (ppd->cpspec->autoneg_tries == AUTONEG_TRIES) {
			ppd->lflags |= QIBL_IB_AUTONEG_FAILED;
			ppd->cpspec->autoneg_tries = 0;
		}
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
		set_7322_ibspeed_fast(ppd, ppd->link_speed_enabled);
	}
}

/*
 * This routine is used to request IPG set in the QLogic switch.
 * Only called if r1.
 */
static void try_7322_ipg(struct qib_pportdata *ppd)
{
	struct qib_ibport *ibp = &ppd->ibport_data;
	struct ib_mad_send_buf *send_buf;
	struct ib_mad_agent *agent;
	struct ib_smp *smp;
	unsigned delay;
	int ret;

	agent = ibp->send_agent;
	if (!agent)
		goto retry;

	send_buf = ib_create_send_mad(agent, 0, 0, 0, IB_MGMT_MAD_HDR,
				      IB_MGMT_MAD_DATA, GFP_ATOMIC,
				      IB_MGMT_BASE_VERSION);
	if (IS_ERR(send_buf))
		goto retry;

	if (!ibp->smi_ah) {
		struct ib_ah *ah;

		ah = qib_create_qp0_ah(ibp, be16_to_cpu(IB_LID_PERMISSIVE));
		if (IS_ERR(ah))
			ret = PTR_ERR(ah);
		else {
			send_buf->ah = ah;
			ibp->smi_ah = to_iah(ah);
			ret = 0;
		}
	} else {
		send_buf->ah = &ibp->smi_ah->ibah;
		ret = 0;
	}

	smp = send_buf->mad;
	smp->base_version = IB_MGMT_BASE_VERSION;
	smp->mgmt_class = IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE;
	smp->class_version = 1;
	smp->method = IB_MGMT_METHOD_SEND;
	smp->hop_cnt = 1;
	smp->attr_id = QIB_VENDOR_IPG;
	smp->attr_mod = 0;

	if (!ret)
		ret = ib_post_send_mad(send_buf, NULL);
	if (ret)
		ib_free_send_mad(send_buf);
retry:
	delay = 2 << ppd->cpspec->ipg_tries;
	queue_delayed_work(ib_wq, &ppd->cpspec->ipg_work,
			   msecs_to_jiffies(delay));
}

/*
 * Timeout handler for setting IPG.
 * Only called if r1.
 */
static void ipg_7322_work(struct work_struct *work)
{
	struct qib_pportdata *ppd;

	ppd = container_of(work, struct qib_chippport_specific,
			   ipg_work.work)->ppd;
	if ((ppd->lflags & (QIBL_LINKINIT | QIBL_LINKARMED | QIBL_LINKACTIVE))
	    && ++ppd->cpspec->ipg_tries <= 10)
		try_7322_ipg(ppd);
}

static u32 qib_7322_iblink_state(u64 ibcs)
{
	u32 state = (u32)SYM_FIELD(ibcs, IBCStatusA_0, LinkState);

	switch (state) {
	case IB_7322_L_STATE_INIT:
		state = IB_PORT_INIT;
		break;
	case IB_7322_L_STATE_ARM:
		state = IB_PORT_ARMED;
		break;
	case IB_7322_L_STATE_ACTIVE:
		/* fall through */
	case IB_7322_L_STATE_ACT_DEFER:
		state = IB_PORT_ACTIVE;
		break;
	default: /* fall through */
	case IB_7322_L_STATE_DOWN:
		state = IB_PORT_DOWN;
		break;
	}
	return state;
}

/* returns the IBTA port state, rather than the IBC link training state */
static u8 qib_7322_phys_portstate(u64 ibcs)
{
	u8 state = (u8)SYM_FIELD(ibcs, IBCStatusA_0, LinkTrainingState);
	return qib_7322_physportstate[state];
}

static int qib_7322_ib_updown(struct qib_pportdata *ppd, int ibup, u64 ibcs)
{
	int ret = 0, symadj = 0;
	unsigned long flags;
	int mult;

	spin_lock_irqsave(&ppd->lflags_lock, flags);
	ppd->lflags &= ~QIBL_IB_FORCE_NOTIFY;
	spin_unlock_irqrestore(&ppd->lflags_lock, flags);

	/* Update our picture of width and speed from chip */
	if (ibcs & SYM_MASK(IBCStatusA_0, LinkSpeedQDR)) {
		ppd->link_speed_active = QIB_IB_QDR;
		mult = 4;
	} else if (ibcs & SYM_MASK(IBCStatusA_0, LinkSpeedActive)) {
		ppd->link_speed_active = QIB_IB_DDR;
		mult = 2;
	} else {
		ppd->link_speed_active = QIB_IB_SDR;
		mult = 1;
	}
	if (ibcs & SYM_MASK(IBCStatusA_0, LinkWidthActive)) {
		ppd->link_width_active = IB_WIDTH_4X;
		mult *= 4;
	} else
		ppd->link_width_active = IB_WIDTH_1X;
	ppd->delay_mult = ib_rate_to_delay[mult_to_ib_rate(mult)];

	if (!ibup) {
		u64 clr;

		/* Link went down. */
		/* do IPG MAD again after linkdown, even if last time failed */
		ppd->cpspec->ipg_tries = 0;
		clr = qib_read_kreg_port(ppd, krp_ibcstatus_b) &
			(SYM_MASK(IBCStatusB_0, heartbeat_timed_out) |
			 SYM_MASK(IBCStatusB_0, heartbeat_crosstalk));
		if (clr)
			qib_write_kreg_port(ppd, krp_ibcstatus_b, clr);
		if (!(ppd->lflags & (QIBL_IB_AUTONEG_FAILED |
				     QIBL_IB_AUTONEG_INPROG)))
			set_7322_ibspeed_fast(ppd, ppd->link_speed_enabled);
		if (!(ppd->lflags & QIBL_IB_AUTONEG_INPROG)) {
			struct qib_qsfp_data *qd =
				&ppd->cpspec->qsfp_data;
			/* unlock the Tx settings, speed may change */
			qib_write_kreg_port(ppd, krp_tx_deemph_override,
				SYM_MASK(IBSD_TX_DEEMPHASIS_OVERRIDE_0,
				reset_tx_deemphasis_override));
			qib_cancel_sends(ppd);
			/* on link down, ensure sane pcs state */
			qib_7322_mini_pcs_reset(ppd);
			/* schedule the qsfp refresh which should turn the link
			   off */
			if (ppd->dd->flags & QIB_HAS_QSFP) {
				qd->t_insert = jiffies;
				queue_work(ib_wq, &qd->work);
			}
			spin_lock_irqsave(&ppd->sdma_lock, flags);
			if (__qib_sdma_running(ppd))
				__qib_sdma_process_event(ppd,
					qib_sdma_event_e70_go_idle);
			spin_unlock_irqrestore(&ppd->sdma_lock, flags);
		}
		clr = read_7322_creg32_port(ppd, crp_iblinkdown);
		if (clr == ppd->cpspec->iblnkdownsnap)
			ppd->cpspec->iblnkdowndelta++;
	} else {
		if (qib_compat_ddr_negotiate &&
		    !(ppd->lflags & (QIBL_IB_AUTONEG_FAILED |
				     QIBL_IB_AUTONEG_INPROG)) &&
		    ppd->link_speed_active == QIB_IB_SDR &&
		    (ppd->link_speed_enabled & QIB_IB_DDR)
		    && ppd->cpspec->autoneg_tries < AUTONEG_TRIES) {
			/* we are SDR, and auto-negotiation enabled */
			++ppd->cpspec->autoneg_tries;
			if (!ppd->cpspec->ibdeltainprog) {
				ppd->cpspec->ibdeltainprog = 1;
				ppd->cpspec->ibsymdelta +=
					read_7322_creg32_port(ppd,
						crp_ibsymbolerr) -
						ppd->cpspec->ibsymsnap;
				ppd->cpspec->iblnkerrdelta +=
					read_7322_creg32_port(ppd,
						crp_iblinkerrrecov) -
						ppd->cpspec->iblnkerrsnap;
			}
			try_7322_autoneg(ppd);
			ret = 1; /* no other IB status change processing */
		} else if ((ppd->lflags & QIBL_IB_AUTONEG_INPROG) &&
			   ppd->link_speed_active == QIB_IB_SDR) {
			qib_autoneg_7322_send(ppd, 1);
			set_7322_ibspeed_fast(ppd, QIB_IB_DDR);
			qib_7322_mini_pcs_reset(ppd);
			udelay(2);
			ret = 1; /* no other IB status change processing */
		} else if ((ppd->lflags & QIBL_IB_AUTONEG_INPROG) &&
			   (ppd->link_speed_active & QIB_IB_DDR)) {
			spin_lock_irqsave(&ppd->lflags_lock, flags);
			ppd->lflags &= ~(QIBL_IB_AUTONEG_INPROG |
					 QIBL_IB_AUTONEG_FAILED);
			spin_unlock_irqrestore(&ppd->lflags_lock, flags);
			ppd->cpspec->autoneg_tries = 0;
			/* re-enable SDR, for next link down */
			set_7322_ibspeed_fast(ppd, ppd->link_speed_enabled);
			wake_up(&ppd->cpspec->autoneg_wait);
			symadj = 1;
		} else if (ppd->lflags & QIBL_IB_AUTONEG_FAILED) {
			/*
			 * Clear autoneg failure flag, and do setup
			 * so we'll try next time link goes down and
			 * back to INIT (possibly connected to a
			 * different device).
			 */
			spin_lock_irqsave(&ppd->lflags_lock, flags);
			ppd->lflags &= ~QIBL_IB_AUTONEG_FAILED;
			spin_unlock_irqrestore(&ppd->lflags_lock, flags);
			ppd->cpspec->ibcctrl_b |= IBA7322_IBC_IBTA_1_2_MASK;
			symadj = 1;
		}
		if (!(ppd->lflags & QIBL_IB_AUTONEG_INPROG)) {
			symadj = 1;
			if (ppd->dd->cspec->r1 && ppd->cpspec->ipg_tries <= 10)
				try_7322_ipg(ppd);
			if (!ppd->cpspec->recovery_init)
				setup_7322_link_recovery(ppd, 0);
			ppd->cpspec->qdr_dfe_time = jiffies +
				msecs_to_jiffies(QDR_DFE_DISABLE_DELAY);
		}
		ppd->cpspec->ibmalfusesnap = 0;
		ppd->cpspec->ibmalfsnap = read_7322_creg32_port(ppd,
			crp_errlink);
	}
	if (symadj) {
		ppd->cpspec->iblnkdownsnap =
			read_7322_creg32_port(ppd, crp_iblinkdown);
		if (ppd->cpspec->ibdeltainprog) {
			ppd->cpspec->ibdeltainprog = 0;
			ppd->cpspec->ibsymdelta += read_7322_creg32_port(ppd,
				crp_ibsymbolerr) - ppd->cpspec->ibsymsnap;
			ppd->cpspec->iblnkerrdelta += read_7322_creg32_port(ppd,
				crp_iblinkerrrecov) - ppd->cpspec->iblnkerrsnap;
		}
	} else if (!ibup && qib_compat_ddr_negotiate &&
		   !ppd->cpspec->ibdeltainprog &&
			!(ppd->lflags & QIBL_IB_AUTONEG_INPROG)) {
		ppd->cpspec->ibdeltainprog = 1;
		ppd->cpspec->ibsymsnap = read_7322_creg32_port(ppd,
			crp_ibsymbolerr);
		ppd->cpspec->iblnkerrsnap = read_7322_creg32_port(ppd,
			crp_iblinkerrrecov);
	}

	if (!ret)
		qib_setup_7322_setextled(ppd, ibup);
	return ret;
}

/*
 * Does read/modify/write to appropriate registers to
 * set output and direction bits selected by mask.
 * these are in their canonical postions (e.g. lsb of
 * dir will end up in D48 of extctrl on existing chips).
 * returns contents of GP Inputs.
 */
static int gpio_7322_mod(struct qib_devdata *dd, u32 out, u32 dir, u32 mask)
{
	u64 read_val, new_out;
	unsigned long flags;

	if (mask) {
		/* some bits being written, lock access to GPIO */
		dir &= mask;
		out &= mask;
		spin_lock_irqsave(&dd->cspec->gpio_lock, flags);
		dd->cspec->extctrl &= ~((u64)mask << SYM_LSB(EXTCtrl, GPIOOe));
		dd->cspec->extctrl |= ((u64) dir << SYM_LSB(EXTCtrl, GPIOOe));
		new_out = (dd->cspec->gpio_out & ~mask) | out;

		qib_write_kreg(dd, kr_extctrl, dd->cspec->extctrl);
		qib_write_kreg(dd, kr_gpio_out, new_out);
		dd->cspec->gpio_out = new_out;
		spin_unlock_irqrestore(&dd->cspec->gpio_lock, flags);
	}
	/*
	 * It is unlikely that a read at this time would get valid
	 * data on a pin whose direction line was set in the same
	 * call to this function. We include the read here because
	 * that allows us to potentially combine a change on one pin with
	 * a read on another, and because the old code did something like
	 * this.
	 */
	read_val = qib_read_kreg64(dd, kr_extstatus);
	return SYM_FIELD(read_val, EXTStatus, GPIOIn);
}

/* Enable writes to config EEPROM, if possible. Returns previous state */
static int qib_7322_eeprom_wen(struct qib_devdata *dd, int wen)
{
	int prev_wen;
	u32 mask;

	mask = 1 << QIB_EEPROM_WEN_NUM;
	prev_wen = ~gpio_7322_mod(dd, 0, 0, 0) >> QIB_EEPROM_WEN_NUM;
	gpio_7322_mod(dd, wen ? 0 : mask, mask, mask);

	return prev_wen & 1;
}

/*
 * Read fundamental info we need to use the chip.  These are
 * the registers that describe chip capabilities, and are
 * saved in shadow registers.
 */
static void get_7322_chip_params(struct qib_devdata *dd)
{
	u64 val;
	u32 piobufs;
	int mtu;

	dd->palign = qib_read_kreg32(dd, kr_pagealign);

	dd->uregbase = qib_read_kreg32(dd, kr_userregbase);

	dd->rcvtidcnt = qib_read_kreg32(dd, kr_rcvtidcnt);
	dd->rcvtidbase = qib_read_kreg32(dd, kr_rcvtidbase);
	dd->rcvegrbase = qib_read_kreg32(dd, kr_rcvegrbase);
	dd->piobufbase = qib_read_kreg64(dd, kr_sendpiobufbase);
	dd->pio2k_bufbase = dd->piobufbase & 0xffffffff;

	val = qib_read_kreg64(dd, kr_sendpiobufcnt);
	dd->piobcnt2k = val & ~0U;
	dd->piobcnt4k = val >> 32;
	val = qib_read_kreg64(dd, kr_sendpiosize);
	dd->piosize2k = val & ~0U;
	dd->piosize4k = val >> 32;

	mtu = ib_mtu_enum_to_int(qib_ibmtu);
	if (mtu == -1)
		mtu = QIB_DEFAULT_MTU;
	dd->pport[0].ibmtu = (u32)mtu;
	dd->pport[1].ibmtu = (u32)mtu;

	/* these may be adjusted in init_chip_wc_pat() */
	dd->pio2kbase = (u32 __iomem *)
		((char __iomem *) dd->kregbase + dd->pio2k_bufbase);
	dd->pio4kbase = (u32 __iomem *)
		((char __iomem *) dd->kregbase +
		 (dd->piobufbase >> 32));
	/*
	 * 4K buffers take 2 pages; we use roundup just to be
	 * paranoid; we calculate it once here, rather than on
	 * ever buf allocate
	 */
	dd->align4k = ALIGN(dd->piosize4k, dd->palign);

	piobufs = dd->piobcnt4k + dd->piobcnt2k + NUM_VL15_BUFS;

	dd->pioavregs = ALIGN(piobufs, sizeof(u64) * BITS_PER_BYTE / 2) /
		(sizeof(u64) * BITS_PER_BYTE / 2);
}

/*
 * The chip base addresses in cspec and cpspec have to be set
 * after possible init_chip_wc_pat(), rather than in
 * get_7322_chip_params(), so split out as separate function
 */
static void qib_7322_set_baseaddrs(struct qib_devdata *dd)
{
	u32 cregbase;

	cregbase = qib_read_kreg32(dd, kr_counterregbase);

	dd->cspec->cregbase = (u64 __iomem *)(cregbase +
		(char __iomem *)dd->kregbase);

	dd->egrtidbase = (u64 __iomem *)
		((char __iomem *) dd->kregbase + dd->rcvegrbase);

	/* port registers are defined as relative to base of chip */
	dd->pport[0].cpspec->kpregbase =
		(u64 __iomem *)((char __iomem *)dd->kregbase);
	dd->pport[1].cpspec->kpregbase =
		(u64 __iomem *)(dd->palign +
		(char __iomem *)dd->kregbase);
	dd->pport[0].cpspec->cpregbase =
		(u64 __iomem *)(qib_read_kreg_port(&dd->pport[0],
		kr_counterregbase) + (char __iomem *)dd->kregbase);
	dd->pport[1].cpspec->cpregbase =
		(u64 __iomem *)(qib_read_kreg_port(&dd->pport[1],
		kr_counterregbase) + (char __iomem *)dd->kregbase);
}

/*
 * This is a fairly special-purpose observer, so we only support
 * the port-specific parts of SendCtrl
 */

#define SENDCTRL_SHADOWED (SYM_MASK(SendCtrl_0, SendEnable) |		\
			   SYM_MASK(SendCtrl_0, SDmaEnable) |		\
			   SYM_MASK(SendCtrl_0, SDmaIntEnable) |	\
			   SYM_MASK(SendCtrl_0, SDmaSingleDescriptor) | \
			   SYM_MASK(SendCtrl_0, SDmaHalt) |		\
			   SYM_MASK(SendCtrl_0, IBVLArbiterEn) |	\
			   SYM_MASK(SendCtrl_0, ForceCreditUpToDate))

static int sendctrl_hook(struct qib_devdata *dd,
			 const struct diag_observer *op, u32 offs,
			 u64 *data, u64 mask, int only_32)
{
	unsigned long flags;
	unsigned idx;
	unsigned pidx;
	struct qib_pportdata *ppd = NULL;
	u64 local_data, all_bits;

	/*
	 * The fixed correspondence between Physical ports and pports is
	 * severed. We need to hunt for the ppd that corresponds
	 * to the offset we got. And we have to do that without admitting
	 * we know the stride, apparently.
	 */
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		u64 __iomem *psptr;
		u32 psoffs;

		ppd = dd->pport + pidx;
		if (!ppd->cpspec->kpregbase)
			continue;

		psptr = ppd->cpspec->kpregbase + krp_sendctrl;
		psoffs = (u32) (psptr - dd->kregbase) * sizeof(*psptr);
		if (psoffs == offs)
			break;
	}

	/* If pport is not being managed by driver, just avoid shadows. */
	if (pidx >= dd->num_pports)
		ppd = NULL;

	/* In any case, "idx" is flat index in kreg space */
	idx = offs / sizeof(u64);

	all_bits = ~0ULL;
	if (only_32)
		all_bits >>= 32;

	spin_lock_irqsave(&dd->sendctrl_lock, flags);
	if (!ppd || (mask & all_bits) != all_bits) {
		/*
		 * At least some mask bits are zero, so we need
		 * to read. The judgement call is whether from
		 * reg or shadow. First-cut: read reg, and complain
		 * if any bits which should be shadowed are different
		 * from their shadowed value.
		 */
		if (only_32)
			local_data = (u64)qib_read_kreg32(dd, idx);
		else
			local_data = qib_read_kreg64(dd, idx);
		*data = (local_data & ~mask) | (*data & mask);
	}
	if (mask) {
		/*
		 * At least some mask bits are one, so we need
		 * to write, but only shadow some bits.
		 */
		u64 sval, tval; /* Shadowed, transient */

		/*
		 * New shadow val is bits we don't want to touch,
		 * ORed with bits we do, that are intended for shadow.
		 */
		if (ppd) {
			sval = ppd->p_sendctrl & ~mask;
			sval |= *data & SENDCTRL_SHADOWED & mask;
			ppd->p_sendctrl = sval;
		} else
			sval = *data & SENDCTRL_SHADOWED & mask;
		tval = sval | (*data & ~SENDCTRL_SHADOWED & mask);
		qib_write_kreg(dd, idx, tval);
		qib_write_kreg(dd, kr_scratch, 0Ull);
	}
	spin_unlock_irqrestore(&dd->sendctrl_lock, flags);
	return only_32 ? 4 : 8;
}

static const struct diag_observer sendctrl_0_observer = {
	sendctrl_hook, KREG_IDX(SendCtrl_0) * sizeof(u64),
	KREG_IDX(SendCtrl_0) * sizeof(u64)
};

static const struct diag_observer sendctrl_1_observer = {
	sendctrl_hook, KREG_IDX(SendCtrl_1) * sizeof(u64),
	KREG_IDX(SendCtrl_1) * sizeof(u64)
};

static ushort sdma_fetch_prio = 8;
module_param_named(sdma_fetch_prio, sdma_fetch_prio, ushort, S_IRUGO);
MODULE_PARM_DESC(sdma_fetch_prio, "SDMA descriptor fetch priority");

/* Besides logging QSFP events, we set appropriate TxDDS values */
static void init_txdds_table(struct qib_pportdata *ppd, int override);

static void qsfp_7322_event(struct work_struct *work)
{
	struct qib_qsfp_data *qd;
	struct qib_pportdata *ppd;
	unsigned long pwrup;
	unsigned long flags;
	int ret;
	u32 le2;

	qd = container_of(work, struct qib_qsfp_data, work);
	ppd = qd->ppd;
	pwrup = qd->t_insert +
		msecs_to_jiffies(QSFP_PWR_LAG_MSEC - QSFP_MODPRS_LAG_MSEC);

	/* Delay for 20 msecs to allow ModPrs resistor to setup */
	mdelay(QSFP_MODPRS_LAG_MSEC);

	if (!qib_qsfp_mod_present(ppd)) {
		ppd->cpspec->qsfp_data.modpresent = 0;
		/* Set the physical link to disabled */
		qib_set_ib_7322_lstate(ppd, 0,
				       QLOGIC_IB_IBCC_LINKINITCMD_DISABLE);
		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags &= ~QIBL_LINKV;
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
	} else {
		/*
		 * Some QSFP's not only do not respond until the full power-up
		 * time, but may behave badly if we try. So hold off responding
		 * to insertion.
		 */
		while (1) {
			if (time_is_before_jiffies(pwrup))
				break;
			msleep(20);
		}

		ret = qib_refresh_qsfp_cache(ppd, &qd->cache);

		/*
		 * Need to change LE2 back to defaults if we couldn't
		 * read the cable type (to handle cable swaps), so do this
		 * even on failure to read cable information.  We don't
		 * get here for QME, so IS_QME check not needed here.
		 */
		if (!ret && !ppd->dd->cspec->r1) {
			if (QSFP_IS_ACTIVE_FAR(qd->cache.tech))
				le2 = LE2_QME;
			else if (qd->cache.atten[1] >= qib_long_atten &&
				 QSFP_IS_CU(qd->cache.tech))
				le2 = LE2_5m;
			else
				le2 = LE2_DEFAULT;
		} else
			le2 = LE2_DE