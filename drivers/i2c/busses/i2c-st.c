) {
		qib_dev_err(dd,
		 "failed to allocate congestion setting list for port %d!\n",
		 port);
		goto bail_1;
	}

	size = sizeof(struct cc_table_shadow);
	ppd->ccti_entries_shadow = kzalloc(size, GFP_KERNEL);
	if (!ppd->ccti_entries_shadow) {
		qib_dev_err(dd,
		 "failed to allocate shadow ccti list for port %d!\n",
		 port);
		goto bail_2;
	}

	size = sizeof(struct ib_cc_congestion_setting_attr);
	ppd->congestion_entries_shadow = kzalloc(size, GFP_KERNEL);
	if (!ppd->congestion_entries_shadow) {
		qib_dev_err(dd,
		 "failed to allocate shadow congestion setting list for port %d!\n",
		 port);
		goto bail_3;
	}

	return 0;

bail_3:
	kfree(ppd->ccti_entries_shadow);
	ppd->ccti_entries_shadow = NULL;
bail_2:
	kfree(ppd->congestion_entries);
	ppd->congestion_entries = NULL;
bail_1:
	kfree(ppd->ccti_entries);
	ppd->ccti_entries = NULL;
bail:
	/* User is intentionally disabling the congestion control agent */
	if (!qib_cc_table_size)
		return 0;

	if (qib_cc_table_size < IB_CCT_MIN_ENTRIES) {
		qib_cc_table_size = 0;
		qib_dev_err(dd,
		 "Congestion Control table size %d less than minimum %d for port %d\n",
		 qib_cc_table_size, IB_CCT_MIN_ENTRIES, port);
	}

	qib_dev_err(dd, "Congestion Control Agent disabled for port %d\n",
		port);
	return 0;
}

static int init_pioavailregs(struct qib_devdata *dd)
{
	int ret, pidx;
	u64 *status_page;

	dd->pioavailregs_dma = dma_alloc_coherent(
		&dd->pcidev->dev, PAGE_SIZE, &dd->pioavailregs_phys,
		GFP_KERNEL);
	if (!dd->pioavailregs_dma) {
		qib_dev_err(dd,
			"failed to allocate PIOavail reg area in memory\n");
		ret = -ENOMEM;
		goto done;
	}

	/*
	 * We really want L2 cache aligned, but for current CPUs of
	 * interest, they are the same.
	 */
	status_page = (u64 *)
		((char *) dd->pioavailregs_dma +
		 ((2 * L1_CACHE_BYTES +
		   dd->pioavregs * sizeof(u64)) & ~L1_CACHE_BYTES));
	/* device status comes first, for backwards compatibility */
	dd->devstatusp = status_page;
	*status_page++ = 0;
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		dd->pport[pidx].statusp = status_page;
		*status_page++ = 0;
	}

	/*
	 * Setup buffer to hold freeze and other messages, accessible to
	 * apps, following statusp.  This is per-unit, not per port.
	 */
	dd->freezemsg = (char *) status_page;
	*dd->freezemsg = 0;
	/* length of msg buffer is "whatever is left" */
	ret = (char *) status_page - (char *) dd->pioavailregs_dma;
	dd->freezelen = PAGE_SIZE - ret;

	ret = 0;

done:
	return ret;
}

/**
 * init_shadow_tids - allocate the shadow TID array
 * @dd: the qlogic_ib device
 *
 * allocate the shadow TID array, so we can qib_munlock previous
 * entries.  It may make more sense to move the pageshadow to the
 * ctxt data structure, so we only allocate memory for ctxts actually
 * in use, since we at 8k per ctxt, now.
 * We don't want failures here to prevent use of the driver/chip,
 * so no return value.
 */
static void init_shadow_tids(struct qib_devdata *dd)
{
	struct page **pages;
	dma_addr_t *addrs;

	pages = vzalloc(dd->cfgctxts * dd->rcvtidcnt * sizeof(struct page *));
	if (!pages) {
		qib_dev_err(dd,
			"failed to allocate shadow page * array, no expected sends!\n");
		goto bail;
	}

	addrs = vzalloc(dd->cfgctxts * dd->rcvtidcnt * sizeof(dma_addr_t));
	if (!addrs) {
		qib_dev_err(dd,
			"failed to allocate shadow dma handle array, no expected sends!\n");
		goto bail_free;
	}

	dd->pageshadow = pages;
	dd->physshadow = addrs;
	return;

bail_free:
	vfree(pages);
bail:
	dd->pageshadow = NULL;
}

/*
 * Do initialization for device that is only needed on
 * first detect, not on resets.
 */
static int loadtime_init(struct qib_devdata *dd)
{
	int ret = 0;

	if (((dd->revision >> QLOGIC_IB_R_SOFTWARE_SHIFT) &
	     QLOGIC_IB_R_SOFTWARE_MASK) != QIB_CHIP_SWVERSION) {
		qib_dev_err(dd,
			"Driver only handles version %d, chip swversion is %d (%llx), failng\n",
			QIB_CHIP_SWVERSION,
			(int)(dd->revision >>
				QLOGIC_IB_R_SOFTWARE_SHIFT) &
				QLOGIC_IB_R_SOFTWARE_MASK,
			(unsigned long long) dd->revision);
		ret = -ENOSYS;
		goto done;
	}

	if (dd->revision & QLOGIC_IB_R_EMULATOR_MASK)
		qib_devinfo(dd->pcidev, "%s", dd->boardversion);

	spin_lock_init(&dd->pioavail_lock);
	spin_lock_init(&dd->sendctrl_lock);
	spin_lock_init(&dd->uctxt_lock);
	spin_lock_init(&dd->qib_diag_trans_lock);
	spin_lock_init(&dd->eep_st_lock);
	mutex_init(&dd->eep_lock);

	if (qib_mini_init)
		goto done;

	ret = init_pioavailregs(dd);
	init_shadow_tids(dd);

	qib_get_eeprom_info(dd);

	/* setup time (don't start yet) to verify we got interrupt */
	init_timer(&dd->intrchk_timer);
	dd->intrchk_timer.function = verify_interrupt;
	dd->intrchk_timer.data = (unsigned long) dd;

	ret = qib_cq_init(dd);
done:
	return ret;
}

/**
 * init_after_reset - re-initialize after a reset
 * @dd: the qlogic_ib device
 *
 * sanity check at least some of the values after reset, and
 * ensure no receive or transmit (explicitly, in case reset
 * failed
 */
static int init_after_reset(struct qib_devdata *dd)
{
	int i;

	/*
	 * Ensure chip does no sends or receives, tail updates, or
	 * pioavail updates while we re-initialize.  This is mostly
	 * for the driver data structures, not chip registers.
	 */
	for (i = 0; i < dd->num_pports; ++i) {
		/*
		 * ctxt == -1 means "all contexts". Only really safe for
		 * _dis_abling things, as here.
		 */
		dd->f_rcvctrl(dd->pport + i, QIB_RCVCTRL_CTXT_DIS |
				  QIB_RCVCTRL_INTRAVAIL_DIS |
				  QIB_RCVCTRL_TAILUPD_DIS, -1);
		/* Redundant across ports for some, but no big deal.  */
		dd->f_sendctrl(dd->pport + i, QIB_SENDCTRL_SEND_DIS |
			QIB_SENDCTRL_AVAIL_DIS);
	}

	return 0;
}

static void enable_chip(struct qib_devdata *dd)
{
	u64 rcvmask;
	int i;

	/*
	 * Enable PIO send, and update of PIOavail regs to memory.
	 */
	for (i = 0; i < dd->num_pports; ++i)
		dd->f_sendctrl(dd->pport + i, QIB_SENDCTRL_SEND_ENB |
			QIB_SENDCTRL_AVAIL_ENB);
	/*
	 * Enable kernel ctxts' receive and receive interrupt.
	 * Other ctxts done as user opens and inits them.
	 */
	rcvmask = QIB_RCVCTRL_CTXT_ENB | QIB_RCVCTRL_INTRAVAIL_ENB;
	rcvmask |= (dd->flags & QIB_NODMA_RTAIL) ?
		  QIB_RCVCTRL_TAILUPD_DIS : QIB_RCVCTRL_TAILUPD_ENB;
	for (i = 0; dd->rcd && i < dd->first_user_ctxt; ++i) {
		struct qib_ctxtdata *rcd = dd->rcd[i];

		if (rcd)
			dd->f_rcvctrl(rcd->ppd, rcvmask, i);
	}
}

static void verify_interrupt(unsigned long opaque)
{
	struct qib_devdata *dd = (struct qib_devdata *) opaque;
	u64 int_counter;

	if (!dd)
		return; /* being torn down */

	/*
	 * If we don't have a lid or any interrupts, let the user know and
	 * don't bother checking again.
	 */
	int_counter = qib_int_counter(dd) - dd->z_int_counter;
	if (int_counter == 0) {
		if (!dd->f_intr_fallback(dd))
			dev_err(&dd->pcidev->dev,
				"No interrupts detected, not usable.\n");
		else /* re-arm the timer to see if fallback works */
			mod_timer(&dd->intrchk_timer, jiffies + HZ/2);
	}
}

static void init_piobuf_state(struct qib_devdata *dd)
{
	int i, pidx;
	u32 uctxts;

	/*
	 * Ensure all buffers are free, and fifos empty.  Buffers
	 * are common, so only do once for port 0.
	 *
	 * After enable and qib_chg_pioavailkernel so we can safely
	 * enable pioavail updates and PIOENABLE.  After this, packets
	 * are ready and able to go out.
	 */
	dd->f_sendctrl(dd->pport, QIB_SENDCTRL_DISARM_ALL);
	for (pidx = 0; pidx < dd->num_pports; ++pidx)
		dd->f_sendctrl(dd->pport + pidx, QIB_SENDCTRL_FLUSH);

	/*
	 * If not all sendbufs are used, add the one to each of the lower
	 * numbered contexts.  pbufsctxt and lastctxt_piobuf are
	 * calculated in chip-specific code because it may cause some
	 * chip-specific adjustments to be made.
	 */
	uctxts = dd->cfgctxts - dd->first_user_ctxt;
	dd->ctxts_extrabuf = dd->pbufsctxt ?
		dd->lastctxt_piobuf - (dd->pbufsctxt * uctxts) : 0;

	/*
	 * Set up the shadow copies of the piobufavail registers,
	 * which we compare against the chip registers for now, and
	 * the in memory DMA'ed copies of the registers.
	 * By now pioavail updates to memory should have occurred, so
	 * copy them into our working/shadow registers; this is in
	 * case something went wrong with abort, but mostly to get the
	 * initial values of the generation bit correct.
	 */
	for (i = 0; i < dd->pioavregs; i++) {
		__le64 tmp;

		tmp = dd->pioavailregs_dma[i];
		/*
		 * Don't need to worry about pioavailkernel here
		 * because we will call qib_chg_pioavailkernel() later
		 * in initialization, to busy out buffers as needed.
		 */
		dd->pioavailshadow[i] = le64_to_cpu(tmp);
	}
	while (i < ARRAY_SIZE(dd->pioavailshadow))
		dd->pioavailshadow[i++] = 0; /* for debugging sanity */

	/* after pioavailshadow is setup */
	qib_chg_pioavailkernel(dd, 0, dd->piobcnt2k + dd->piobcnt4k,
			       TXCHK_CHG_TYPE_KERN, NULL);
	dd->f_initvl15_bufs(dd);
}

/**
 * qib_create_workqueues - create per port workqueues
 * @dd: the qlogic_ib device
 */
static int qib_create_workqueues(struct qib_devdata *dd)
{
	int pidx;
	struct qib_pportdata *ppd;

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (!ppd->qib_wq) {
			char wq_name[8]; /* 3 + 2 + 1 + 1 + 1 */

			snprintf(wq_name, sizeof(wq_name), "qib%d_%d",
				dd->unit, pidx);
			ppd->qib_wq =
				create_singlethread_workqueue(wq_name);
			if (!ppd->qib_wq)
				goto wq_error;
		}
	}
	return 0;
wq_error:
	pr_err("create_singlethread_workqueue failed for port %d\n",
		pidx + 1);
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (ppd->qib_wq) {
			destroy_workqueue(ppd->qib_wq);
			ppd->qib_wq = NULL;
		}
	}
	return -ENOMEM;
}

static void qib_free_pportdata(struct qib_pportdata *ppd)
{
	free_percpu(ppd->ibport_data.pmastats);
	ppd->ibport_data.pmastats = NULL;
}

/**
 * qib_init - do the actual initialization sequence on the chip
 * @dd: the qlogic_ib device
 * @reinit: reinitializing, so don't allocate new memory
 *
 * Do the actual initialization sequence on the chip.  This is done
 * both from the init routine called from the PCI infrastructure, and
 * when we reset the chip, or detect that it was reset internally,
 * or it's administratively re-enabled.
 *
 * Memory allocation here and in called routines is only done in
 * the first case (reinit == 0).  We have to be careful, because even
 * without memory allocation, we need to re-write all the chip registers
 * TIDs, etc. after the reset or enable has completed.
 */
int qib_init(struct qib_devdata *dd, int reinit)
{
	int ret = 0, pidx, lastfail = 0;
	u32 portok = 0;
	unsigned i;
	struct qib_ctxtdata *rcd;
	struct qib_pportdata *ppd;
	unsigned long flags;

	/* Set linkstate to unknown, so we can watch for a transition. */
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags &= ~(QIBL_LINKACTIVE | QIBL_LINKARMED |
				 QIBL_LINKDOWN | QIBL_LINKINIT |
				 QIBL_LINKV);
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
	}

	if (reinit)
		ret = init_after_reset(dd);
	else
		ret = loadtime_init(dd);
	if (ret)
		goto done;

	/* Bypass most chip-init, to get to device creation */
	if (qib_mini_init)
		return 0;

	ret = dd->f_late_initreg(dd);
	if (ret)
		goto done;

	/* dd->rcd can be NULL if early init failed */
	for (i = 0; dd->rcd && i < dd->first_user_ctxt; ++i) {
		/*
		 * Set up the (kernel) rcvhdr queue and egr TIDs.  If doing
		 * re-init, the simplest way to handle this is to free
		 * existing, and re-allocate.
		 * Need to re-create rest of ctxt 0 ctxtdata as well.
		 */
		rcd = dd->rcd[i];
		if (!rcd)
			continue;

		lastfail = qib_create_rcvhdrq(dd, rcd);
		if (!lastfail)
			lastfail = qib_setup_eagerbufs(rcd);
		if (lastfail) {
			qib_dev_err(dd,
				"failed to allocate kernel ctxt's rcvhdrq and/or egr bufs\n");
			continue;
		}
	}

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		int mtu;

		if (lastfail)
			ret = lastfail;
		ppd = dd->pport + pidx;
		mtu = ib_mtu_enum_to_int(qib_ibmtu);
		if (mtu == -1) {
			mtu = QIB_DEFAULT_MTU;
			qib_ibmtu = 0; /* don't leave invalid value */
		}
		/* set max we can ever have for this driver load */
		ppd->init_ibmaxlen = min(mtu > 2048 ?
					 dd->piosize4k : dd->piosize2k,
					 dd->rcvegrbufsize +
					 (dd->rcvhdrentsize << 2));
		/*
		 * Have to initialize ibmaxlen, but this will normally
		 * change immediately in qib_set_mtu().
		 */
		ppd->ibmaxlen = ppd->init_ibmaxlen;
		qib_set_mtu(ppd, mtu);

		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags |= QIBL_IB_LINK_DISABLED;
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);

		lastfail = dd->f_bringup_serdes(ppd);
		if (lastfail) {
			qib_devinfo(dd->pcidev,
				 "Failed to bringup IB port %u\n", ppd->port);
			lastfail = -ENETDOWN;
			continue;
		}

		portok++;
	}

	if (!portok) {
		/* none of the ports initialized */
		if (!ret && lastfail)
			ret = lastfail;
		else if (!ret)
			ret = -ENETDOWN;
		/* but continue on, so we can debug cause */
	}

	enable_chip(dd);

	init_piobuf_state(dd);

done:
	if (!ret) {
		/* chip is OK for user apps; mark it as initialized */
		for (pidx = 0; pidx < dd->num_pports; ++pidx) {
			ppd = dd->pport + pidx;
			/*
			 * Set status even if port serdes is not initialized
			 * so that diags will work.
			 */
			*ppd->statusp |= QIB_STATUS_CHIP_PRESENT |
				QIB_STATUS_INITTED;
			if (!ppd->link_speed_enabled)
				continue;
			if (dd->flags & QIB_HAS_SEND_DMA)
				ret = qib_setup_sdma(ppd);
			init_timer(&ppd->hol_timer);
			ppd->hol_timer.function = qib_hol_event;
			ppd->hol_timer.data = (unsigned long)ppd;
			ppd->hol_state = QIB_HOL_UP;
		}

		/* now we can enable all interrupts from the chip */
		dd->f_set_intr_state(dd, 1);

		/*
		 * Setup to verify we get an interrupt, and fallback
		 * to an alternate if necessary and possible.
		 */
		mod_timer(&dd->intrchk_timer, jiffies + HZ/2);
		/* start stats retrieval timer */
		mod_timer(&dd->stats_timer, jiffies + HZ * ACTIVITY_TIMER);
	}

	/* if ret is non-zero, we probably should do some cleanup here... */
	return ret;
}

/*
 * These next two routines are placeholders in case we don't have per-arch
 * code for controlling write combining.  If explicit control of write
 * combining is not available, performance will probably be awful.
 */

int __attribute__((weak)) qib_enable_wc(struct qib_devdata *dd)
{
	return -EOPNOTSUPP;
}

void __attribute__((weak)) qib_disable_wc(struct qib_devdata *dd)
{
}

static inline struct qib_devdata *__qib_lookup(int unit)
{
	return idr_find(&qib_unit_table, unit);
}

struct qib_devdata *qib_lookup(int unit)
{
	struct qib_devdata *dd;
	unsigned long flags;

	spin_lock_irqsave(&qib_devs_lock, flags);
	dd = __qib_lookup(unit);
	spin_unlock_irqrestore(&qib_devs_lock, flags);

	return dd;
}

/*
 * Stop the timers during unit shutdown, or after an error late
 * in initialization.
 */
static void qib_stop_timers(struct qib_devdata *dd)
{
	struct qib_pportdata *ppd;
	int pidx;

	if (dd->stats_timer.data) {
		del_timer_sync(&dd->stats_timer);
		dd->stats_timer.data = 0;
	}
	if (dd->intrchk_timer.data) {
		del_timer_sync(&dd->intrchk_timer);
		dd->intrchk_timer.data = 0;
	}
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		if (ppd->hol_timer.data)
			del_timer_sync(&ppd->hol_timer);
		if (ppd->led_override_timer.data) {
			del_timer_sync(&ppd->led_override_timer);
			atomic_set(&ppd->led_override_timer_active, 0);
		}
		if (ppd->symerr_clear_timer.data)
			del_timer_sync(&ppd->symerr_clear_timer);
	}
}

/**
 * qib_shutdown_device - shut down a device
 * @dd: the qlogic_ib device
 *
 * This is called to make the device quiet when we are about to
 * unload the driver, and also when the device is administratively
 * disabled.   It does not free any data structures.
 * Everything it does has to be setup again by qib_init(dd, 1)
 */
static void qib_shutdown_device(struct qib_devdata *dd)
{
	struct qib_pportdata *ppd;
	unsigned pidx;

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;

		spin_lock_irq(&ppd->lflags_lock);
		ppd->lflags &= ~(QIBL_LINKDOWN | QIBL_LINKINIT |
				 QIBL_LINKARMED | QIBL_LINKACTIVE |
				 QIBL_LINKV);
		spin_unlock_irq(&ppd->lflags_lock);
		*ppd->statusp &= ~(QIB_STATUS_IB_CONF | QIB_STATUS_IB_READY);
	}
	dd->flags &= ~QIB_INITTED;

	/* mask interrupts, but not errors */
	dd->f_set_intr_state(dd, 0);

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		dd->f_rcvctrl(ppd, QIB_RCVCTRL_TAILUPD_DIS |
				   QIB_RCVCTRL_CTXT_DIS |
				   QIB_RCVCTRL_INTRAVAIL_DIS |
				   QIB_RCVCTRL_PKEY_ENB, -1);
		/*
		 * Gracefully stop all sends allowing any in progress to
		 * trickle out first.
		 */
		dd->f_sendctrl(ppd, QIB_SENDCTRL_CLEAR);
	}

	/*
	 * Enough for anything that's going to trickle out to have actually
	 * done so.
	 */
	udelay(20);

	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		ppd = dd->pport + pidx;
		dd->f_setextled(ppd, 0); /* make sure LEDs are off */

		if (dd->flags & QIB_HAS_SEND_DMA)
			qib_teardown_sdma(ppd);

		dd->f_sendctrl(ppd, QIB_SENDCTRL_AVAIL_DIS |
				    QIB_SENDCTRL_SEND_DIS);
		/*
		 * Clear SerdesEnable.
		 * We can't count on interrupts since we are stopping.
		 */
		dd->f_quiet_serdes(ppd);

		if (ppd->qib_wq) {
			destroy_workqueue(ppd->qib_wq);
			ppd->qib_wq = NULL;
		}
		qib_free_pportdata(ppd);
	}

}

/**
 * qib_free_ctxtdata - free a context's allocated data
 * @dd: the qlogic_ib device
 * @rcd: the ctxtdata structure
 *
 * free up any allocated data for a context
 * This should not touch anything that would affect a simultaneous
 * re-allocation of context data, because it is called after qib_mutex
 * is released (and can be called from reinit as well).
 * It should never change any chip state, or global driver state.
 */
void qib_free_ctxtdata(struct qib_devdata *dd, struct qib_ctxtdata *rcd)
{
	if (!rcd)
		return;

	if (rcd->rcvhdrq) {
		dma_free_coherent(&dd->pcidev->dev, rcd->rcvhdrq_size,
				  rcd->rcvhdrq, rcd->rcvhdrq_phys);
		rcd->rcvhdrq = NULL;
		if (rcd->rcvhdrtail_kvaddr) {
			dma_free_coherent(&dd->pcidev->dev, PAGE_SIZE,
					  rcd->rcvhdrtail_kvaddr,
					  rcd->rcvhdrqtailaddr_phys);
			rcd->rcvhdrtail_kvaddr = NULL;
		}
	}
	if (rcd->rcvegrbuf) {
		unsigned e;

		for (e = 0; e < rcd->rcvegrbuf_chunks; e++) {
			void *base = rcd->rcvegrbuf[e];
			size_t size = rcd->rcvegrbuf_size;

			dma_free_coherent(&dd->pcidev->dev, size,
					  base, rcd->rcvegrbuf_phys[e]);
		}
		kfree(rcd->rcvegrbuf);
		rcd->rcvegrbuf = NULL;
		kfree(rcd->rcvegrbuf_phys);
		rcd->rcvegrbuf_phys = NULL;
		rcd->rcvegrbuf_chunks = 0;
	}

	kfree(rcd->tid_pg_list);
	vfree(rcd->user_event_mask);
	vfree(rcd->subctxt_uregbase);
	vfree(rcd->subctxt_rcvegrbuf);
	vfree(rcd->subctxt_rcvhdr_base);
#ifdef CONFIG_DEBUG_FS
	kfree(rcd->opstats);
	rcd->opstats = NULL;
#endif
	kfree(rcd);
}

/*
 * Perform a PIO buffer bandwidth write test, to verify proper system
 * configuration.  Even when all the setup calls work, occasionally
 * BIOS or other issues can prevent write combining from working, or
 * can cause other bandwidth problems to the chip.
 *
 * This test simply writes the same buffer over and over again, and
 * measures close to the peak bandwidth to the chip (not testing
 * data bandwidth to the wire).   On chips that use an address-based
 * trigger to send packets to the wire, this is easy.  On chips that
 * use a count to trigger, we want to make sure that the packet doesn't
 * go out on the wire, or trigger flow control checks.
 */
static void qib_verify_pioperf(struct qib_devdata *dd)
{
	u32 pbnum, cnt, lcnt;
	u32 __iomem *piobuf;
	u32 *addr;
	u64 msecs, emsecs;

	piobuf = dd->f_getsendbuf(dd->pport, 0ULL, &pbnum);
	if (!piobuf) {
		qib_devinfo(dd->pcidev,
			 "No PIObufs for checking perf, skipping\n");
		return;
	}

	/*
	 * Enough to give us a reasonable test, less than piobuf size, and
	 * likely multiple of store buffer length.
	 */
	cnt = 1024;

	addr = vmalloc(cnt);
	if (!addr) {
		qib_devinfo(dd->pcidev,
			 "Couldn't get memory for checking PIO perf, skipping\n");
		goto done;
	}

	preempt_disable();  /* we want reasonably accurate elapsed time */
	msecs = 1 + jiffies_to_msecs(jiffies);
	for (lcnt = 0; lcnt < 10000U; lcnt++) {
		/* wait until we cross msec boundary */
		if (jiffies_to_msecs(jiffies) >= msecs)
			break;
		udelay(1);
	}

	dd->f_set_armlaunch(dd, 0);

	/*
	 * length 0, no dwords actually sent
	 */
	writeq(0, piobuf);
	qib_flush_wc();

	/*
	 * This is only roughly accurate, since even with preempt we
	 * still take interrupts that could take a while.   Running for
	 * >= 5 msec seems to get us "close enough" to accurate values.
	 */
	msecs = jiffies_to_msecs(jiffies);
	for (emsecs = lcnt = 0; emsecs <= 5UL; lcnt++) {
		qib_pio_copy(piobuf + 64, addr, cnt >> 2);
		emsecs = jiffies_to_msecs(jiffies) - msecs;
	}

	/* 1 GiB/sec, slightly over IB SDR line rate */
	if (lcnt < (emsecs * 1024U))
		qib_dev_err(dd,
			    "Performance problem: bandwidth to PIO buffers is only %u MiB/sec\n",
			    lcnt / (u32) emsecs);

	preempt_enable();

	vfree(addr);

done:
	/* disarm piobuf, so it's available again */
	dd->f_sendctrl(dd->pport, QIB_SENDCTRL_DISARM_BUF(pbnum));
	qib_sendbuf_done(dd, pbnum);
	dd->f_set_armlaunch(dd, 1);
}

void qib_free_devdata(struct qib_devdata *dd)
{
	unsigned long flags;

	spin_lock_irqsave(&qib_devs_lock, flags);
	idr_remove(&qib_unit_table, dd->unit);
	list_del(&dd->list);
	spin_unlock_irqrestore(&qib_devs_lock, flags);

#ifdef CONFIG_DEBUG_FS
	qib_dbg_ibdev_exit(&dd->verbs_dev);
#endif
	free_percpu(dd->int_counter);
	ib_dealloc_device(&dd->verbs_dev.ibdev);
}

u64 qib_int_counter(struct qib_devdata *dd)
{
	int cpu;
	u64 int_counter = 0;

	for_each_possible_cpu(cpu)
		int_counter += *per_cpu_ptr(dd->int_counter, cpu);
	return int_counter;
}

u64 qib_sps_ints(void)
{
	unsigned long flags;
	struct qib_devdata *dd;
	u64 sps_ints = 0;

	spin_lock_irqsave(&qib_devs_lock, flags);
	list_for_each_entry(dd, &qib_dev_list, list) {
		sps_ints += qib_int_counter(dd);
	}
	spin_unlock_irqrestore(&qib_devs_lock, flags);
	return sps_ints;
}

/*
 * Allocate our primary per-unit data structure.  Must be done via verbs
 * allocator, because the verbs cleanup process both does cleanup and
 * free of the data structure.
 * "extra" is for chip-specific data.
 *
 * 