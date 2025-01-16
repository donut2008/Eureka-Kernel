, QIB_IB_CFG_LINKLATENCY);
		pip->link_roundtrip_latency[0] = v >> 16;
		pip->link_roundtrip_latency[1] = v >> 8;
		pip->link_roundtrip_latency[2] = v;
	}

	ret = reply(smp);

bail:
	return ret;
}

/**
 * get_pkeys - return the PKEY table
 * @dd: the qlogic_ib device
 * @port: the IB port number
 * @pkeys: the pkey table is placed here
 */
static int get_pkeys(struct qib_devdata *dd, u8 port, u16 *pkeys)
{
	struct qib_pportdata *ppd = dd->pport + port - 1;
	/*
	 * always a kernel context, no locking needed.
	 * If we get here with ppd setup, no need to check
	 * that pd is valid.
	 */
	struct qib_ctxtdata *rcd = dd->rcd[ppd->hw_pidx];

	memcpy(pkeys, rcd->pkeys, sizeof(rcd->pkeys));

	return 0;
}

static int subn_get_pkeytable(struct ib_smp *smp, struct ib_device *ibdev,
			      u8 port)
{
	u32 startpx = 32 * (be32_to_cpu(smp->attr_mod) & 0xffff);
	u16 *p = (u16 *) smp->data;
	__be16 *q = (__be16 *) smp->data;

	/* 64 blocks of 32 16-bit P_Key entries */

	memset(smp->data, 0, sizeof(smp->data));
	if (startpx == 0) {
		struct qib_devdata *dd = dd_from_ibdev(ibdev);
		unsigned i, n = qib_get_npkeys(dd);

		get_pkeys(dd, port, p);

		for (i = 0; i < n; i++)
			q[i] = cpu_to_be16(p[i]);
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}

static int subn_set_guidinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 startgx = 8 * be32_to_cpu(smp->attr_mod);
	__be64 *p = (__be64 *) smp->data;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* 32 blocks of 8 64-bit GUIDs per block */

	if (startgx == 0 && pidx < dd->num_pports) {
		struct qib_pportdata *ppd = dd->pport + pidx;
		struct qib_ibport *ibp = &ppd->ibport_data;
		unsigned i;

		/* The first entry is read-only. */
		for (i = 1; i < QIB_GUIDS_PER_PORT; i++)
			ibp->guids[i - 1] = p[i];
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	/* The only GUID we support is the first read-only entry. */
	return subn_get_guidinfo(smp, ibdev, port);
}

/**
 * subn_set_portinfo - set port information
 * @smp: the incoming SM packet
 * @ibdev: the infiniband device
 * @port: the port on the device
 *
 * Set Portinfo (see ch. 14.2.5.6).
 */
static int subn_set_portinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct ib_port_info *pip = (struct ib_port_info *)smp->data;
	struct ib_event event;
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	u8 clientrereg = (pip->clientrereg_resv_subnetto & 0x80);
	unsigned long flags;
	u16 lid, smlid;
	u8 lwe;
	u8 lse;
	u8 state;
	u8 vls;
	u8 msl;
	u16 lstate;
	int ret, ore, mtu;
	u32 port_num = be32_to_cpu(smp->attr_mod);

	if (port_num == 0)
		port_num = port;
	else {
		if (port_num > ibdev->phys_port_cnt)
			goto err;
		/* Port attributes can only be set on the receiving port */
		if (port_num != port)
			goto get_only;
	}

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;
	event.device = ibdev;
	event.element.port_num = port;

	ibp->mkey = pip->mkey;
	ibp->gid_prefix = pip->gid_prefix;
	ibp->mkey_lease_period = be16_to_cpu(pip->mkey_lease_period);

	lid = be16_to_cpu(pip->lid);
	/* Must be a valid unicast LID address. */
	if (lid == 0 || lid >= QIB_MULTICAST_LID_BASE)
		smp->status |= IB_SMP_INVALID_FIELD;
	else if (ppd->lid != lid || ppd->lmc != (pip->mkeyprot_resv_lmc & 7)) {
		if (ppd->lid != lid)
			qib_set_uevent_bits(ppd, _QIB_EVENT_LID_CHANGE_BIT);
		if (ppd->lmc != (pip->mkeyprot_resv_lmc & 7))
			qib_set_uevent_bits(ppd, _QIB_EVENT_LMC_CHANGE_BIT);
		qib_set_lid(ppd, lid, pip->mkeyprot_resv_lmc & 7);
		event.event = IB_EVENT_LID_CHANGE;
		ib_dispatch_event(&event);
	}

	smlid = be16_to_cpu(pip->sm_lid);
	msl = pip->neighbormtu_mastersmsl & 0xF;
	/* Must be a valid unicast LID address. */
	if (smlid == 0 || smlid >= QIB_MULTICAST_LID_BASE)
		smp->status |= IB_SMP_INVALID_FIELD;
	else if (smlid != ibp->sm_lid || msl != ibp->sm_sl) {
		spin_lock_irqsave(&ibp->lock, flags);
		if (ibp->sm_ah) {
			if (smlid != ibp->sm_lid)
				ibp->sm_ah->attr.dlid = smlid;
			if (msl != ibp->sm_sl)
				ibp->sm_ah->attr.sl = msl;
		}
		spin_unlock_irqrestore(&ibp->lock, flags);
		if (smlid != ibp->sm_lid)
			ibp->sm_lid = smlid;
		if (msl != ibp->sm_sl)
			ibp->sm_sl = msl;
		event.event = IB_EVENT_SM_CHANGE;
		ib_dispatch_event(&event);
	}

	/* Allow 1x or 4x to be set (see 14.2.6.6). */
	lwe = pip->link_width_enabled;
	if (lwe) {
		if (lwe == 0xFF)
			set_link_width_enabled(ppd, ppd->link_width_supported);
		else if (lwe >= 16 || (lwe & ~ppd->link_width_supported))
			smp->status |= IB_SMP_INVALID_FIELD;
		else if (lwe != ppd->link_width_enabled)
			set_link_width_enabled(ppd, lwe);
	}

	lse = pip->linkspeedactive_enabled & 0xF;
	if (lse) {
		/*
		 * The IB 1.2 spec. only allows link speed values
		 * 1, 3, 5, 7, 15.  1.2.1 extended to allow specific
		 * speeds.
		 */
		if (lse == 15)
			set_link_speed_enabled(ppd,
					       ppd->link_speed_supported);
		else if (lse >= 8 || (lse & ~ppd->link_speed_supported))
			smp->status |= IB_SMP_INVALID_FIELD;
		else if (lse != ppd->link_speed_enabled)
			set_link_speed_enabled(ppd, lse);
	}

	/* Set link down default state. */
	switch (pip->portphysstate_linkdown & 0xF) {
	case 0: /* NOP */
		break;
	case 1: /* SLEEP */
		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_SLEEP);
		break;
	case 2: /* POLL */
		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_POLL);
		break;
	default:
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	ibp->mkeyprot = pip->mkeyprot_resv_lmc >> 6;
	ibp->vl_high_limit = pip->vl_high_limit;
	(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_LIMIT,
				    ibp->vl_high_limit);

	mtu = ib_mtu_enum_to_int((pip->neighbormtu_mastersmsl >> 4) & 0xF);
	if (mtu == -1)
		smp->status |= IB_SMP_INVALID_FIELD;
	else
		qib_set_mtu(ppd, mtu);

	/* Set operational VLs */
	vls = (pip->operationalvl_pei_peo_fpi_fpo >> 4) & 0xF;
	if (vls) {
		if (vls > ppd->vls_supported)
			smp->status |= IB_SMP_INVALID_FIELD;
		else
			(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_OP_VLS, vls);
	}

	if (pip->mkey_violations == 0)
		ibp->mkey_violations = 0;

	if (pip->pkey_violations == 0)
		ibp->pkey_violations = 0;

	if (pip->qkey_violations == 0)
		ibp->qkey_violations = 0;

	ore = pip->localphyerrors_overrunerrors;
	if (set_phyerrthreshold(ppd, (ore >> 4) & 0xF))
		smp->status |= IB_SMP_INVALID_FIELD;

	if (set_overrunthreshold(ppd, (ore & 0xF)))
		smp->status |= IB_SMP_INVALID_FIELD;

	ibp->subnet_timeout = pip->clientrereg_resv_subnetto & 0x1F;

	/*
	 * Do the port state change now that the other link parameters
	 * have been set.
	 * Changing the port physical state only makes sense if the link
	 * is down or is being set to down.
	 */
	state = pip->linkspeed_portstate & 0xF;
	lstate = (pip->portphysstate_linkdown >> 4) & 0xF;
	if (lstate && !(state == IB_PORT_DOWN || state == IB_PORT_NOP))
		smp->status |= IB_SMP_INVALID_FIELD;

	/*
	 * Only state changes of DOWN, ARM, and ACTIVE are valid
	 * and must be in the correct state to take effect (see 7.2.6).
	 */
	switch (state) {
	case IB_PORT_NOP:
		if (lstate == 0)
			break;
		/* FALLTHROUGH */
	case IB_PORT_DOWN:
		if (lstate == 0)
			lstate = QIB_IB_LINKDOWN_ONLY;
		else if (lstate == 1)
			lstate = QIB_IB_LINKDOWN_