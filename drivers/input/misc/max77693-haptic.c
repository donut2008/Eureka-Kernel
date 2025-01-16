 IB_SMP_INVALID_FIELD;
	}

	if (clientrereg) {
		event.event = IB_EVENT_CLIENT_REREGISTER;
		ib_dispatch_event(&event);
	}

	ret = subn_get_portinfo(smp, ibdev, port);

	/* restore re-reg bit per o14-12.2.1 */
	pip->clientrereg_resv_subnetto |= clientrereg;

	goto get_only;

err:
	smp->status |= IB_SMP_INVALID_FIELD;
get_only:
	ret = subn_get_portinfo(smp, ibdev, port);
done:
	return ret;
}

/**
 * rm_pkey - decrecment the reference count for the given PKEY
 * @dd: the qlogic_ib device
 * @key: the PKEY index
 *
 * Return true if this was the last reference and the hardware table entry
 * needs to be changed.
 */
static int rm_pkey(struct qib_pportdata *ppd, u16 key)
{
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (ppd->pkeys[i] != key)
			continue;
		if (atomic_dec_and_test(&ppd->pkeyrefs[i])) {
			ppd->pkeys[i] = 0;
			ret = 1;
			goto bail;
		}
		break;
	}

	ret = 0;

bail:
	return ret;
}

/**
 * add_pkey - add the given PKEY to the hardware table
 * @dd: the qlogic_ib device
 * @key: the PKEY
 *
 * Return an error code if unable to add the entry, zero if no change,
 * or 1 if the hardware PKEY register needs to be updated.
 */
static int add_pkey(struct qib_pportdata *ppd, u16 key)
{
	int i;
	u16 lkey = key & 0x7FFF;
	int any = 0;
	int ret;

	if (lkey == 0x7FFF) {
		ret = 0;
		goto bail;
	}

	/* Look for an empty slot or a matching PKEY. */
	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (!ppd->pkeys[i]) {
			any++;
			continue;
		}
		/* If it matches exactly, try to increment the ref count */
		if (ppd->pkeys[i] == key) {
			if (atomic_inc_return(&ppd->pkeyrefs[i]) > 1) {
				ret = 0;
				goto bail;
			}
			/* Lost the race. Look for an empty slot below. */
			atomic_dec(&ppd->pkeyrefs[i]);
			any++;
		}
		/*
		 * It makes no sense to have both the limited and unlimited
		 * PKEY set at the same time since the unlimited one will
		 * disable the limited one.
		 */
		if ((ppd->pkeys[i] & 0x7FFF) == lkey) {
			ret = -EEXIST;
			goto bail;
		}
	}
	if (!any) {
		ret = -EBUSY;
		goto bail;
	}
	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (!ppd->pkeys[i] &&
		    atomic_inc_return(&ppd->pkeyrefs[i]) == 1) {
			/* for qibstats, etc. */
			ppd->pkeys[i] = key;
			ret = 1;
			goto bail;
		}
	}
	ret = -EBUSY;

bail:
	return ret;
}

/**
 * set_pkeys - set the PKEY table for ctxt 0
 * @dd: the qlogic_ib device
 * @port: the IB port number
 * @pkeys: the PKEY table
 */
static int set_pkeys(struct qib_devdata *dd, u8 port, u16 *pkeys)
{
	struct qib_pportdata *ppd;
	struct qib_ctxtdata *rcd;
	int i;
	int changed = 0;

	/*
	 * IB port one/two always maps to context zero/one,
	 * always a kernel context, no locking needed
	 * If we get here with ppd setup, no need to check
	 * that rcd is valid.
	 */
	ppd = dd->pport + (port - 1);
	rcd = dd->rcd[ppd->hw_pidx];

	for (i = 0; i < ARRAY_SIZE(rcd->pkeys); i++) {
		u16 key = pkeys[i];
		u16 okey = rcd->pkeys[i];

		if (key == okey)
			continue;
		/*
		 * The value of this PKEY table entry is changing.
		 * Remove the old entry in the hardware's array of PKEYs.
		 */
		if (okey & 0x7FFF)
			changed |= rm_pkey(ppd, okey);
		if (key & 0x7FFF) {
			int ret = add_pkey(ppd, key);

			if (ret < 0)
				key = 0;
			else
				changed |= ret;
		}
		rcd->pkeys[i] = key;
	}
	if (changed) {
		struct ib_event event;

		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PKEYS, 0);

		event.event = IB_EVENT_PKEY_CHANGE;
		event.device = &dd->verbs_dev.ibdev;
		event.element.port_num = port;
		ib_dispatch_event(&event);
	}
	return 0;
}

static int subn_set_pkeytable(struct ib_smp *smp, struct ib_device *ibdev,
			      u8 port)
{
	u32 startpx = 32 * (be32_to_cpu(smp->attr_mod) & 0xffff);
	__be16 *p = (__be16 *) smp->data;
	u16 *q = (u16 *) smp->data;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	unsigned i, n = qib_get_npkeys(dd);

	for (i = 0; i < n; i++)
		q[i] = be16_to_cpu(p[i]);

	if (startpx != 0 || set_pkeys(dd, port, q) != 0)
		smp->status |= IB_SMP_INVALID_FIELD;

	return subn_get_pkeytable(smp, ibdev, port);
}

static int subn_get_sl_to_vl(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *) smp->data;
	unsigned i;

	memset(smp->data, 0, sizeof(smp->data));

	if (!(ibp->port_cap_flags & IB_PORT_SL_MAP_SUP))
		smp->status |= IB_SMP_UNSUP_METHOD;
	else
		for (i = 0; i < ARRAY_SIZE(ibp->sl_to_vl); i += 2)
			*p++ = (ibp->sl_to_vl[i] << 4) | ibp->sl_to_vl[i + 1];

	return reply(smp);
}

static int subn_set_sl_to_vl(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *) smp->data;
	unsigned i;

	if (!(ibp->port_cap_flags & IB_PORT_SL_MAP_SUP)) {
		smp->status |= IB_SMP_UNSUP_METHOD;
		return reply(smp);
	}

	for (i = 0; i < ARRAY_SIZE(ibp->sl_to_vl); i += 2, p++) {
		ibp->sl_to_vl[i] = *p >> 4;
		ibp->sl_to_vl[i + 1] = *p & 0xF;
	}
	qib_set_uevent_bits(ppd_from_ibp(to_iport(ibdev, port)),
			    _QIB_EVENT_SL2VL_CHANGE_BIT);

	return subn_get_sl_to_vl(smp, ibdev, port);
}

static int subn_get_vl_arb(struct ib_smp *smp, struct ib_device *ibdev,
			   u8 port)
{
	unsigned which = be32_to_cpu(smp->attr_mod) >> 16;
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));

	memset(smp->data, 0, sizeof(smp->data));

	if (ppd->vls_supported == IB_VL_VL0)
		smp->status |= IB_SMP_UNSUP_METHOD;
	else if (which == IB_VLARB_LOWPRI_0_31)
		(void) ppd->dd->f_get_ib_table(ppd, QIB_IB_TBL_VL_LOW_ARB,
						   smp->data);
	else if (which == IB_VLARB_HIGHPRI_0_31)
		(void) ppd->dd->f_get_ib_table(ppd, QIB_IB_TBL_VL_HIGH_ARB,
						   smp->data);
	else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}

static int subn_set_vl_arb(struct ib_smp *smp, struct ib_device *ibdev,
			   u8 port)
{
	unsigned which = be32_to_cpu(smp->attr_mod) >> 16;
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));

	if (ppd->vls_supported == IB_VL_VL0)
		smp->status |= IB_SMP_UNSUP_METHOD;
	else if (which == IB_VLARB_LOWPRI_0_31)
		(void) ppd->dd->f_set_ib_table(ppd, QIB_IB_TBL_VL_LOW_ARB,
						   smp->data);
	else if (which == IB_VLARB_HIGHPRI_0_31)
		(void) ppd->dd->f_set_ib_table(ppd, QIB_IB_TBL_VL_HIGH_ARB,
						   smp->data);
	else
		smp->status |= IB_SMP_INVALID_FIELD;

	return subn_get_vl_arb(smp, ibdev, port);
}

static int subn_trap_repress(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	/*
	 * For now, we only send the trap once so no need to process this.
	 * o13-6, o13-7,
	 * o14-3.a4 The SMA shall not send any message in response to a valid
	 * SubnTrapRepress() message.
	 */
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
}

static int pma_get_classportinfo(struct ib_pma_mad *pmp,
				 struct ib_device *ibdev)
{
	struct ib_class_port_info *p =
		(struct ib_class_port_info *)pmp->data;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);

	memset(pmp->data, 0, sizeof(pmp->data));

	if (pmp->mad_hdr.attr_mod != 0)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	/* Note that AllPortSelect is not valid */
	p->base_version = 1;
	p->class_version = 1;
	p->capability_mask = IB_PMA_CLASS_CAP_EXT_WIDTH;
	/*
	 * Set the most significant bit of CM2 to indicate support for
	 * congestion statistics
	 */
	p->reserved[0] = dd->psxmitwait_supported << 7;
	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->resp_time_value = 18;

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portsamplescontrol(struct ib_pma_mad *pmp,
				      struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplescontrol *p =
		(struct ib_pma_portsamplescontrol *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct qib_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 port_select = p->port_select;

	memset(pmp->data, 0, sizeof(pmp->data));

	p->port_select = port_select;
	if (pmp->mad_hdr.attr_mod != 0 || port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		goto bail;
	}
	spin_lock_irqsave(&ibp->lock, flags);
	p->tick = dd->f_get_ib_cfg(ppd, QIB_IB_CFG_PMA_TICKS);
	p->sample_status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
	p->counter_width = 4;   /* 32 bit counters */
	p->counter_mask0_9 = COUNTER_MASK0_9;
	p->sample_start = cpu_to_be32(ibp->pma_sample_start);
	p->sample_interval = cpu_to_be32(ibp->pma_sample_interval);
	p->tag = cpu_to_be16(ibp->pma_tag);
	p->counter_select[0] = ibp->pma_counter_select[0];
	p->counter_select[1] = ibp->pma_counter_select[1];
	p->counter_select[2] = ibp->pma_counter_select[2];
	p->counter_select[3] = ibp->pma_counter_select[3];
	p->counter_select[4] = ibp->pma_counter_select[4];
	spin_unlock_irqrestore(&ibp->lock, flags);

bail:
	return reply((struct ib_smp *) pmp);
}

static int pma_set_portsamplescontrol(struct ib_pma_mad *pmp,
				      struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplescontrol *p =
		(struct ib_pma_portsamplescontrol *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct qib_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 status, xmit_flags;
	int ret;

	if (pmp->mad_hdr.attr_mod != 0 || p->port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		ret = reply((struct ib_smp *) pmp);
		goto bail;
	}

	spin_lock_irqsave(&ibp->lock, flags);

	/* Port Sampling code owns the PS* HW counters */
	xmit_flags = ppd->cong_stats.flags;
	ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_SAMPLE;
	status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
	if (status == IB_PMA_SAMPLE_STATUS_DONE ||
	    (status == IB_PMA_SAMPLE_STATUS_RUNNING &&
	     xmit_flags == IB_PMA_CONG_HW_CONTROL_TIMER)) {
		ibp->pma_sample_start = be32_to_cpu(p->sample_start);
		ibp->pma_sample_interval = be32_to_cpu(p->sample_interval);
		ibp->pma_tag = be16_to_cpu(p->tag);
		ibp->pma_counter_select[0] = p->counter_select[0];
		ibp->pma_counter_select[1] = p->counter_select[1];
		ibp->pma_counter_select[2] = p->counter_select[2];
		ibp->pma_counter_select[3] = p->counter_select[3];
		ibp->pma_counter_select[4] = p->counter_select[4];
		dd->f_set_cntr_sample(ppd, ibp->pma_sample_interval,
				      ibp->pma_sample_start);
	}
	spin_unlock_irqrestore(&ibp->loc