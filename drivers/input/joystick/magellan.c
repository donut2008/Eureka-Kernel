",
			 ppd->dd->unit, ppd->port);
	} else if (!strncmp(what, "off", 3)) {
		ppd->cpspec->ibcctrl_a &= ~SYM_MASK(IBCCtrlA_0,
							Loopback);
		/* enable heart beat again */
		val = IBA7322_IBC_HRTBT_RMASK << IBA7322_IBC_HRTBT_LSB;
		qib_devinfo(ppd->dd->pcidev,
			"Disabling IB%u:%u IBC loopback (normal)\n",
			ppd->dd->unit, ppd->port);
	} else
		ret = -EINVAL;
	if (!ret) {
		qib_write_kreg_port(ppd, krp_ibcctrl_a,
				    ppd->cpspec->ibcctrl_a);
		ctrlb = ppd->cpspec->ibcctrl_b & ~(IBA7322_IBC_HRTBT_MASK
					     << IBA7322_IBC_HRTBT_LSB);
		ppd->cpspec->ibcctrl_b = ctrlb | val;
		qib_write_kreg_port(ppd, krp_ibcctrl_b,
				    ppd->cpspec->ibcctrl_b);
		qib_write_kreg(ppd->dd, kr_scratch, 0);
	}
	return ret;
}

static void get_vl_weights(struct qib_pportdata *ppd, unsigned regno,
			   struct ib_vl_weight_elem *vl)
{
	unsigned i;

	for (i = 0; i < 16; i++, regno++, vl++) {
		u32 val = qib_read_kreg_port(ppd, regno);

		vl->vl = (val >> SYM_LSB(LowPriority0_0, VirtualLane)) &
			SYM_RMASK(LowPriority0_0, VirtualLane);
		vl->weight = (val >> SYM_LSB(LowPriority0_0, Weight)) &
			SYM_RMASK(LowPriority0_0, Weight);
	}
}

static void set_vl_weights(struct qib_pportdata *ppd, unsigned regno,
			   struct ib_vl_weight_elem *vl)
{
	unsigned i;

	for (i = 0; i < 16; i++, regno++, vl++) {
		u64 val;

		val = ((vl->vl & SYM_RMASK(LowPriority0_0, VirtualLane)) <<
			SYM_LSB(LowPriority0_0, VirtualLane)) |
		      ((vl->weight & SYM_RMASK(LowPriority0_0, Weight)) <<
			SYM_LSB(LowPriority0_0, Weight));
		qib_write_kreg_port(ppd, regno, val);
	}
	if (!(ppd->p_sendctrl & SYM_MASK(SendCtrl_0, IBVLArbiterEn))) {
		struct qib_devdata *dd = ppd->dd;
		unsigned long flags;

		spin_lock_irqsave(&dd->sendctrl_lock, flags);
		ppd->p_sendctrl |= SYM_MASK(SendCtrl_0, IBVLArbiterEn);
		qib_write_kreg_port(ppd, krp_sendctrl, ppd->p_sendctrl);
		qib_write_kreg(dd, kr_scratch, 0);
		spin_unlock_irqrestore(&dd->sendctrl_lock, flags);
	}
}

static int qib_7322_get_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	switch (which) {
	case QIB_IB_TBL_VL_HIGH_ARB:
		get_vl_weights(ppd, krp_highprio_0, t);
		break;

	case QIB_IB_TBL_VL_LOW_ARB:
		get_vl_weights(ppd, krp_lowprio_0, t);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static int qib_7322_set_ib_table(struct qib_pportdata *ppd, int which, void *t)
{
	switch (which) {
	case QIB_IB_TBL_VL_HIGH_ARB:
		set_vl_weights(ppd, krp_highprio_0, t);
		break;

	case QIB_IB_TBL_VL_LOW_ARB:
		set_vl_weights(ppd, krp_lowprio_0, t);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static void qib_update_7322_usrhead(struct qib_ctxtdata *rcd, u64 hd,
				    u32 updegr, u32 egrhd, u32 npkts)
{
	/*
	 * Need to write timeout register before updating rcvhdrhead to ensure
	 * that the timer is enabled on reception of a packet.
	 */
	if (hd >> IBA7322_HDRHEAD_PKTINT_SHIFT)
		adjust_rcv_timeout(rcd, npkts);
	if (updegr)
		qib_write_ureg(rcd->dd, ur_rcvegrindexhead, egrhd, rcd->ctxt);
	mmiowb();
	qib_write_ureg(rcd->dd, ur_rcvhdrhead, hd, rcd->ctxt);
	qib_write_ureg(rcd->dd, ur_rcvhdrhead, hd, rcd->ctxt);
	mmiowb();
}

static u32 qib_7322_hdrqempty(struct qib_ctxtdata *rcd)
{
	u32 head, tail;

	head = qib_read_ureg32(rcd->dd, ur_rcvhdrhead, rcd->ctxt);
	if (rcd->rcvhdrtail_kvaddr)
		tail = qib_get_rcvhdrtail(rcd);
	else
		tail = qib_read_ureg32(rcd->dd, ur_rcvhdrtail, rcd->ctxt);
	return head == tail;
}

#define RCVCTRL_COMMON_MODS (QIB_RCVCTRL_CTXT_ENB | \
	QIB_RCVCTRL_CTXT_DIS | \
	QIB_RCVCTRL_TIDFLOW_ENB | \
	QIB_RCVCTRL_TIDFLOW_DIS | \
	QIB_RCVCTRL_TAILUPD_ENB | \
	QIB_RCVCTRL_TAILUPD_DIS | \
	QIB_RCVCTRL_INTRAVAIL_ENB | \
	QIB_RCVCTRL_INTRAVAIL_DIS | \
	QIB_RCVCTRL_BP_ENB | \
	QIB_RCVCTRL_BP_DIS)

#define RCVCTRL_PORT_MODS (QIB_RCVCTRL_CTXT_ENB | \
	QIB_RCVCTRL_CTXT_DIS | \
	QIB_RCVCTRL_PKEY_DIS | \
	QIB_RCVCTRL_PKEY_ENB)

/*
 * Modify the RCVCTRL register in chip-specific way. This
 * is a function because bit positions and (future) register
 * location is chip-specifc, but the needed operations are
 * generic. <op> is a bit-mask because we often want to
 * do multiple modifications.
 */
static void rcvctrl_7322_mod(struct qib_pportdata *ppd, unsigned int op,
			     int ctxt)
{
	struct qib_devdata *dd = ppd->dd;
	struct qib_ctxtdata *rcd;
	u64 mask, val;
	unsigned long flags;

	spin_lock_irqsave(&dd->cspec->rcvmod_lock, flags);

	if (op & QIB_RCVCTRL_TIDFLOW_ENB)
		dd->rcvctrl |= SYM_MASK(RcvCtrl, TidFlowEnable);
	if (op & QIB_RCVCTRL_TIDFLOW_DIS)
		dd->rcvctrl &= ~SYM_MASK(RcvCtrl, TidFlowEnable);
	if (op & QIB_RCVCTRL_TAILUPD_ENB)
		dd->rcvctrl |= SYM_MASK(RcvCtrl, TailUpd);
	if (op & QIB_RCVCTRL_TAILUPD_DIS)
		dd->rcvctrl &= ~SYM_MASK(RcvCtrl, TailUpd);
	if (op & QIB_RCVCTRL_PKEY_ENB)
		ppd->p_rcvctrl &= ~SYM_MASK(RcvCtrl_0, RcvPartitionKeyDisable);
	if (op & QIB_RCVCTRL_PKEY_DIS)
		ppd->p_rcvctrl |= SYM_MASK(RcvCtrl_0, RcvPartitionKeyDisable);
	if (ctxt < 0) {
		mask = (1ULL << dd->ctxtcnt) - 1;
		rcd = NULL;
	} else {
		mask = (1ULL << ctxt);
		rcd = dd->rcd[ctxt];
	}
	if ((op & QIB_RCVCTRL_CTXT_ENB) && rcd) {
		ppd->p_rcvctrl |=
			(mask << SYM_LSB(RcvCtrl_0, ContextEnableKernel));
		if (!(dd->flags & QIB_NODMA_RTAIL)) {
			op |= QIB_RCVCTRL_TAILUPD_ENB; /* need reg write */
			dd->rcvctrl |= SYM_MASK(RcvCtrl, TailUpd);
		}
		/* Write these registers before the context is enabled. */
		qib_write_kreg_ctxt(dd, krc_rcvhdrtailaddr, ctxt,
				    rcd->rcvhdrqtailaddr_phys);
		qib_write_kreg_ctxt(dd, krc_rcvhdraddr, ctxt,
				    rcd->rcvhdrq_phys);
		rcd->seq_cnt = 1;
	}
	if (op & QIB_RCVCTRL_CTXT_DIS)
		ppd->p_rcvctrl &=
			~(mask << SYM_LSB(RcvCtrl_0, ContextEnableKernel));
	if (op & QIB_RCVCTRL_BP_ENB)
		dd->rcvctrl |= mask << SYM_LSB(RcvCtrl