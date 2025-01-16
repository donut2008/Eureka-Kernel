		"IB%u: Forced to single port mode by module parameter\n",
			dd->unit);
		features &= PORT_SPD_CAP;
	}

	return features;
}

/*
 * This routine sleeps, so it can only be called from user context, not
 * from interrupt context.
 */
static int qib_do_7322_reset(struct qib_devdata *dd)
{
	u64 val;
	u64 *msix_vecsave;
	int i, msix_entries, ret = 1;
	u16 cmdval;
	u8 int_line, clinesz;
	unsigned long flags;

	/* Use dev_err so it shows up in logs, etc. */
	qib_dev_err(dd, "Resetting InfiniPath unit %u\n", dd->unit);

	qib_pcie_getcmd(dd, &cmdval, &int_line, &clinesz);

	msix_entries = dd->cspec->num_msix_entries;

	/* no interrupts till re-initted */
	qib_7322_set_intr_state(dd, 0);

	if (msix_entries) {
		qib_7322_nomsix(dd);
		/* can be up to 512 bytes, too big for stack */
		msix_vecsave = kmalloc(2 * dd->cspec->num_msix_entries *
			sizeof(u64), GFP_KERNEL);
		if (!msix_vecsave)
			qib_dev_err(dd, "No mem to save MSIx data\n");
	} else
		msix_vecsave = NULL;

	/*
	 * Core PCI (as of 2.6.18) doesn't save or rewrite the full vector
	 * info that is set up by the BIOS, so we have to save and restore
	 * it ourselves.   There is some risk something could change it,
	 * after we save it, but since we have disabled the MSIx, it
	 * shouldn't be touched...
	 */
	for (i = 0; i < msix_entries; i++) {
		u64 vecaddr, vecdata;

		vecaddr = qib_read_kreg64(dd, 2 * i +
				  (QIB_7322_MsixTable_OFFS / sizeof(u64)));
		vecdata = qib_read_kreg64(dd, 1 + 2 * i +
				  (QIB_7322_MsixTable_OFFS / sizeof(u64)));
		if (msix_vecsave) {
			msix_vecsave[2 * i] = vecaddr;
			/* save it without the masked bit set */
			msix_vecsave[1 + 2 * i] = vecdata & ~0x100000000ULL;
		}
	}

	dd->pport->cpspec->ibdeltainprog = 0;
	dd->pport->cpspec->ibsymdelta = 0;
	dd->pport->cpspec->iblnkerrdelta = 0;
	dd->pport->cpspec->ibmalfdelta = 0;
	/* so we check interrupts work again */
	dd->z_int_counter = qib_int_counter(dd);

	/*
	 * Keep chip from being accessed until we are ready.  Use
	 * writeq() directly, to allow the write even though QIB_PRESENT
	 * isn't set.
	 */
	dd->flags &= ~(QIB_INITTED | QIB_PRESENT | QIB_BADINTR);
	dd->flags |= QIB_DOING_RESET;
	val = dd->control | QLOGIC_IB_C_RESET;
	writeq(val, &dd->kregbase[kr_control]);

	for (i = 1; i <= 5; i++) {
		/*
		 * Allow MBIST, etc. to complete; longer on each retry.
		 * We sometimes get machine checks from bus timeout if no
		 * response, so for now, make it *really* long.
		 */
		msleep(1000 + (1 + i) * 3000);

		qib_pcie_reenable(dd, cmdval, int_line, clinesz);

		/*
		 * Use readq directly, so we don't need to mark it as PRESENT
		 * until we get a successful indication that all is well.
		 */
		val = readq(&dd->kregbase[kr_revision]);
		if (val == dd->revision)
			break;
		if (i == 5) {
			qib_dev_err(dd,
				"Failed to initialize after reset, unusable\n");
			ret = 0;
			goto  bail;
		}
	}

	dd->flags |= QIB_PRESENT; /* it's back */

	if (msix_entries) {
		/* restore the MSIx vector address and data if saved above */
		for (i = 0; i < msix_entries; i++) {
			dd->cspec->msix_entries[i].msix.entry = i;
			if (!msix_vecsave || !msix_vecsave[2 * i])
				continue;
			qib_write_kreg(dd, 2 * i +
				(QIB_7322_MsixTable_OFFS / sizeof(u64)),
				msix_vecsave[2 * i]);
			qib_write_kreg(dd, 1 + 2 * i +
				(QIB_7322_MsixTable_OFFS / sizeof(u64)),
				msix_vecsave[1 + 2 * i]);
		}
	}

	/* initialize the remaining registers.  */
	for (i = 0; i < dd->num_pports; ++i)
		write_7322_init_portregs(&dd->pport[i]);
	write_7322_initregs(dd);

	if (qib_pcie_params(dd, dd->lbus_width,
			    &dd->cspec->num_msix_entries,
			    dd->cspec->msix_entries))
		qib_dev_err(dd,
			"Reset failed to setup PCIe or interrupts; continuing anyway\n");

	qib_setup_7322_interrupt(dd, 1);

	for (i = 0; i < dd->num_pports; ++i) {
		struct qib_pportdata *ppd = &dd->pport[i];

		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags |= QIBL_IB_FORCE_NOTIFY;
		ppd->lflags &= ~QIBL_IB_AUTONEG_FAILED;
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
	}

bail:
	dd->flags &= ~QIB_DOING_RESET; /* OK or not, no longer resetting */
	kfree(msix_vecsave);
	return ret;
}

/**
 * qib_7322_put_tid - write a TID to the chip
 * @dd: the qlogic_ib device
 * @tidptr: pointer to the expected TID (in chip) to update
 * @tidtype: 0 for eager, 1 for expected
 * @pa: physical address of in memory buffer; tidinvalid if freeing
 */
static void qib_7322_put_tid(struct qib_devdata *dd, u64 __iomem *tidptr,
			     u32 type, unsigned long pa)
{
	if (!(dd->flags & QIB_PRESENT))
		return;
	if (pa != dd->tidinvalid) {
		u64 chippa = pa >> IBA7322_TID_PA_SHIFT;

		/* paranoia checks */
		if (pa != (chippa << IBA7322_TID_PA_SHIFT)) {
			qib_dev_err(dd, "Physaddr %lx not 2KB aligned!\n",
				    pa);
			return;
		}
		if (chippa >= (1UL << IBA7322_TID_SZ_SHIFT)) {
			qib_dev_err(dd,
				"Physical page address 0x%lx larger than supported\n",
				pa);
			return;
		}

		if (type == RCVHQ_RCV_TYPE_EAGER)
			chippa |= dd->tidtemplate;
		else /* for now, always full 4KB page */
			chippa |= IBA7322_TID_SZ_4K;
		pa = chippa;
	}
	writeq(pa, tidptr);
	mmiowb();
}

/**
 * qib_7322_clear_tids - clear all TID entries for a ctxt, expected and eager
 * @dd: the qlogic_ib device
 * @ctxt: the ctxt
 *
 * clear all TID entries for a ctxt, expected and eager.
 * Used from qib_close().
 */
static void qib_7322_clear_tids(struct qib_devdata *dd,
				struct qib_ctxtdata *rcd)
{
	u64 __iomem *tidbase;
	unsigned long tidinv;
	u32 ctxt;
	int i;

	if (!dd->kregbase || !rcd)
		return;

	ctxt = rcd->ctxt;

	tidinv = dd->tidinvalid;
	tidbase = (u64 __iomem *)
		((char __iomem *) dd->kregbase +
		 dd->rcvtidbase +
		 ctxt * dd->rcvtidcnt * sizeof(*tidbase));

	for (i = 0; i < dd->rcvtidcnt; i++)
		qib_7322_put_tid(dd, &tidbase[i], RCVHQ_RCV_TYPE_EXPECTED,
				 tidinv);

	tidbase = (u64 __iomem *)
		((char __iomem *) dd->kregbase +
		 dd->rcvegrbase +
		 rcd->rcvegr_tid_base * sizeof(*tidbase));

	for (i = 0; i < rcd->rcvegrcnt; i++)
		qib_7322_put_tid(dd, &tidbase[i], RCVHQ_RCV_TYPE_EAGER,
				 tidinv);
}

/**
 * qib_7322_tidtemplate - setup constants for TID updates
 * @dd: the qlogic_ib device
 *
 * We setup stuff that we use a lot, to avoid calculating each time
 */
static void qib_7322_tidtemplate(struct qib_devdata *dd)
{
	/*
	 * For now, we always allocate 4KB buffers (at init) so we can
	 * receive max size packets.  We may want a module parameter to
	 * specify 2KB or 4KB and/or make it per port instead of per device
	 * for those who want to reduce memory footprint.  Note that the
	 * rcvhdrentsize size must be large enough to hold the largest
	 * IB header (currently 96 bytes) that we expect to handle (plus of
	 * course the 2 dwords of RHF).
	 */
	if (dd->rcvegrbufsize == 2048)
		dd->tidtemplate = IBA7322_TID_SZ_2K;
	else if (dd->rcvegrbufsize == 4096)
		dd->tidtemplate = IBA7322_TID_SZ_4K;
	dd->tidinvalid = 0;
}

/**
 * qib_init_7322_get_base_info - set chip-specific flags for user code
 * @rcd: the qlogic_ib ctxt
 * @kbase: qib_base_info pointer
 *
 * We set the PCIE flag because the lower bandwidth on PCIe vs
 * HyperTransport can affect some user packet algorithims.
 */

static int qib_7322_get_base_info(struct qib_ctxtdata *rcd,
				  struct qib_base_info *kinfo)
{
	kinfo->spi_runtime_flags |= QIB_RUNTIME_CTXT_MSB_IN_QP |
		QIB_RUNTIME_PCIE | QIB_RUNTIME_NODMA_RTAIL |
		QIB_RUNTIME_HDRSUPP | QIB_RUNTIME_SDMA;
	if (rcd->dd->cspec->r1)
		kinfo->spi_runtime_flags |= QIB_RUNTIME_RCHK;
	if (rcd->dd->flags & QIB_USE_SPCL_TRIG)
		kinfo->spi_runtime_flags |= QIB_RUNTIME_SPECIAL_TRIGGER;

	return 0;
}

static struct qib_message_header *
qib_7322_get_msgheader(struct qib_devdata *dd, __le32 *rhf_addr)
{
	u32 offset = qib_hdrget_offset(rhf_addr);

	return (struct qib_message_header *)
		(rhf_addr - dd->rhf_offset + offset);
}

/*
 * Configure number of contexts.
 */
static void qib_7322_config_ctxts(struct qib_devdata *dd)
{
	unsigned long flags;
	u32 nchipctxts;

	nchipctxts = qib_read_kreg32(dd, kr_contextcnt);
	dd->cspec->numctxts = nchipctxts;
	if (qib_n_krcv_queues > 1 && dd->num_pports) {
		dd->first_user_ctxt = NUM_IB_PORTS +
			(qib_n_krcv_queues - 1) * dd->num_pports;
		if (dd->first_user_ctxt > nchipctxts)
			dd->first_user_ctxt = nchipctxts;
		dd->n_krcv_queues = dd->first_user_ctxt / dd->num_pports;
	} else {
		dd->first_user_ctxt = NUM_IB_PORTS;
		dd->n_krcv_queues = 1;
	}

	if (!qib_cfgctxts) {
		int nctxts = dd->first_user_ctxt + num_online_cpus();

		if (nctxts <= 6)
			dd->ctxtcnt = 6;
		else if (nctxts <= 10)
			dd->ctxtcnt = 10;
		else if (nctxts <= nchipctxts)
			dd->ctxtcnt = nchipctxts;
	} else if (qib_cfgctxts < dd->num_pports)
		dd->ctxtcnt = dd->num_pports;
	else if (qib_cfgctxts <= nchipctxts)
		dd->ctxtcnt = qib_cfgctxts;
	if (!dd->ctxtcnt) /* none of the above, set to max */
		dd->ctxtcnt = nchipctxts;

	/*
	 * Chip can be configured for 6, 10, or 18 ctxts, and choice
	 * affects number of eager TIDs per ctxt (1K, 2K, 4K).
	 * Lock to be paranoid about later motion, etc.
	 */
	spin_lock_irqsave(&dd->cspec->rcvmod_lock, flags);
	if (dd->ctxtcnt > 10)
		dd->rcvctrl |= 2ULL << SYM_LSB(RcvCtrl, ContextCfg);
	else if (dd->ctxtcnt > 6)
		dd->rcvctrl |= 1ULL << SYM_LSB(RcvCtrl, ContextCfg);
	/* else configure for default 6 receive ctxts */

	/* The XRC opcode is 5. */
	dd->rcvctrl |= 5ULL << SYM_LSB(RcvCtrl, XrcTypeCode);

	/*
	 * RcvCtrl *must* be written here so that the
	 * chip understands how to change rcvegrcnt below.
	 */
	qib_write_kreg(dd, kr_rcvctrl, dd->rcvctrl);
	spin_unlock_irqrestore(&dd->cspec->rcvmod_lock, flags);

	/* kr_rcvegrcnt changes based on the number of contexts enabled */
	dd->cspec->rcvegrcnt = qib_read_kreg32(dd, kr_rcvegrcnt);
	if (qib_rcvhdrcnt)
		dd->rcvhdrcnt = max(dd->cspec->rcvegrcnt, qib_rcvhdrcnt);
	else
		dd->rcvhdrcnt = 2 * max(dd->cspec->rcvegrcnt,
				    dd->num_pports > 1 ? 1024U : 2048U);
}

static int qib_7322_get_ib_cfg(struct qib_pportdata *ppd, int which)
{

	int lsb, ret = 0;
	u64 maskr; /* right-justified mask */

	switch (which) {

	case QIB_IB_CFG_LWID_ENB: /* Get allowed Link-width */
		ret = ppd->link_width_enabled;
		goto done;

	case QIB_IB_CFG_LWID: /* Get currently active Link-width */
		ret = ppd->link_width_active;
		goto done;

	case QIB_IB_CFG_SPD_ENB: /* Get allowed Link speeds */
		ret = ppd->link_speed_enabled;
		goto done;

	case QIB_IB_CFG_SPD: /* Get current Link spd */
		ret = ppd->link_speed_active;
		goto done;

	case QIB_IB_CFG_RXPOL_ENB: /* Get Auto-RX-polarity enable */
		lsb = SYM_LSB(IBCCtrlB_0, IB_POLARITY_REV_SUPP);
		maskr = SYM_RMASK(IBCCtrlB_0, IB_POLARITY_REV_SUPP);
		break;

	case QIB_IB_CFG_LREV_ENB: /* Get Auto-Lane-reversal enable */
		lsb = SYM_LSB(IBCCtrlB_0, IB_LANE_REV_SUPPORTED);
		maskr = SYM_RMASK(IBCCtrlB_0, IB_LANE_REV_SUPPORTED);
		break;

	case QIB_IB_CFG_LINKLATENCY:
		ret = qib_read_kreg_port(ppd, krp_ibcstatus_b) &
			SYM_MASK(IBCStatusB_0, LinkRoundTripLatency);
		goto done;

	case QIB_IB_CFG_OP_VLS:
		ret = ppd->vls_operational;
		goto done;

	case QIB_IB_CFG_VL_HIGH_CAP:
		ret = 16;
		goto done;

	case QIB_IB_CFG_VL_LOW_CAP:
		ret = 16;
		goto done;

	case QIB_IB_CFG_OVERRUN_THRESH: /* IB overrun threshold */
		ret = SYM_FIELD(ppd->cpspec->ibcctrl_a, IBCCtrlA_0,
				OverrunThreshold);
		goto done;

	case QIB_IB_CFG_PHYERR_THRESH: /* IB PHY error threshold */
		r