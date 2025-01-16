ruct ib_mad_notice_attr data;

	if (trap_num == IB_NOTICE_TRAP_BAD_PKEY)
		ibp->pkey_violations++;
	else
		ibp->qkey_violations++;
	ibp->n_pkt_drops++;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = trap_num;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof(data.details));
	data.details.ntc_257_258.lid1 = lid1;
	data.details.ntc_257_258.lid2 = lid2;
	data.details.ntc_257_258.key = cpu_to_be32(key);
	data.details.ntc_257_258.sl_qp1 = cpu_to_be32((sl << 28) | qp1);
	data.details.ntc_257_258.qp2 = cpu_to_be32(qp2);

	qib_send_trap(ibp, &data, sizeof(data));
}

/*
 * Send a bad M_Key trap (ch. 14.3.9).
 */
static void qib_bad_mkey(struct qib_ibport *ibp, struct ib_smp *smp)
{
	struct ib_mad_notice_attr data;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_BAD_MKEY;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof(data.details));
	data.details.ntc_256.lid = data.issuer_lid;
	data.details.ntc_256.method = smp->method;
	data.details.ntc_256.attr_id = smp->attr_id;
	data.details.ntc_256.attr_mod = smp->attr_mod;
	data.details.ntc_256.mkey = smp->mkey;
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {
		u8 hop_cnt;

		data.details.ntc_256.dr_slid = smp->dr_slid;
		data.details.ntc_256.dr_trunc_hop = IB_NOTICE_TRAP_DR_NOTICE;
		hop_cnt = smp->hop_cnt;
		if (hop_cnt > ARRAY_SIZE(data.details.ntc_256.dr_rtn_path)) {
			data.details.ntc_256.dr_trunc_hop |=
				IB_NOTICE_TRAP_DR_TRUNC;
			hop_cnt = ARRAY_SIZE(data.details.ntc_256.dr_rtn_path);
		}
		data.details.ntc_256.dr_trunc_hop |= hop_cnt;
		memcpy(data.details.ntc_256.dr_rtn_path, smp->return_path,
		       hop_cnt);
	}

	qib_send_trap(ibp, &data, sizeof(data));
}

/*
 * Send a Port Capability Mask Changed trap (ch. 14.3.11).
 */
void qib_cap_mask_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_CAP_MASK_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof(data.details));
	data.details.ntc_144.lid = data.issuer_lid;
	data.details.ntc_144.new_cap_mask = cpu_to_be32(ibp->port_cap_flags);

	qib_send_trap(ibp, &data, sizeof(data));
}

/*
 * Send a System Image GUID Changed trap (ch. 14.3.12).
 */
void qib_sys_guid_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_SYS_GUID_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof(data.details));
	data.details.ntc_145.lid = data.issuer_lid;
	data.details.ntc_145.new_sys_guid = ib_qib_sys_image_guid;

	qib_send_trap(ibp, &data, sizeof(data));
}

/*
 * Send a Node Description Changed trap (ch. 14.3.13).
 */
void qib_node_desc_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_CAP_MASK_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof(data.details));
	data.details.ntc_144.lid = data.issuer_lid;
	data.details.ntc_144.local_changes = 1;
	data.details.ntc_144.change_flags = IB_NOTICE_TRAP_NODE_DESC_CHG;

	qib_send_trap(ibp, &data, sizeof(data));
}

static int subn_get_nodedescription(struct ib_smp *smp,
				    struct ib_device *ibdev)
{
	if (smp->attr_mod)
		smp->status |= IB_SMP_INVALID_FIELD;

	memcpy(smp->data, ibdev->node_desc, sizeof(smp->data));

	return reply(smp);
}

static int subn_get_nodeinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct ib_node_info *nip = (struct ib_node_info *)&smp->data;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 vendor, majrev, minrev;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* GUID 0 is illegal */
	if (smp->attr_mod || pidx >= dd->num_pports ||
	    dd->pport[pidx].guid == 0)
		smp->status |= IB_SMP_INVALID_FIELD;
	else
		nip->port_guid = dd->pport[pidx].guid;

	nip->base_version = 1;
	nip->class_version = 1;
	nip->node_type = 1;     /* channel adapter */
	nip->num_ports = ibdev->phys_port_cnt;
	/* This is already in network order */
	nip->sys_guid = ib_qib_sys_image_guid;
	nip->node_guid = dd->pport->guid; /* Use first-port GUID as node */
	nip->partition_cap = cpu_to_be16(qib_get_npkeys(dd));
	nip->device_id = cpu_to_be16(dd->deviceid);
	majrev = dd->majrev;
	minrev = dd->minrev;
	nip->revision = cpu_to_be32((majrev << 16) | minrev);
	nip->local_port_num = port;
	vendor = dd->vendorid;
	nip->vendor_id[0] = QIB_SRC_OUI_1;
	nip->vendor_id[1] = QIB_SRC_OUI_2;
	nip->vendor_id[2] = QIB_SRC_OUI_3;

	return reply(smp);
}

static int subn_get_guidinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 startgx = 8 * be32_to_cpu(smp->attr_mod);
	__be64 *p = (__be64 *) smp->data;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* 32 blocks of 8 64-bit GUIDs per block */

	memset(smp->data, 0, sizeof(smp->data));

	if (startgx == 0 && pidx < dd->num_pports) {
		struct qib_pportdata *ppd = dd->pport + pidx;
		struct qib_ibport *ibp = &ppd->ibport_data;
		__be64 g = ppd->guid;
		unsigned i;

		/* GUID 0 is illegal */
		if (g == 0)
			smp->status |= IB_SMP_INVALID_FIELD;
		else {
			/* The first is a copy of the read-only HW GUID. */
			p[0] = g;
			for (i = 1; i < QIB_GUIDS_PER_PORT; i++)
				p[i] = ibp->guids[i - 1];
		}
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}

static void set_link_width_enabled(struct qib_pportdata *ppd, u32 w)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LWID_ENB, w);
}

static void set_link_speed_enabled(struct qib_pportdata *ppd, u32 s)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_SPD_ENB, s);
}

static int get_overrunthreshold(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_OVERRUN_THRESH);
}

/**
 * set_overrunthreshold - set the overrun threshold
 * @ppd: the physical port data
 * @n: the new threshold
 *
 * Note that this will only take effect when the link state changes.
 */
static int set_overrunthreshold(struct qib_pportdata *ppd, unsigned n)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_OVERRUN_THRESH,
					 (u32)n);
	return 0;
}

static int get_phyerrthreshold(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_PHYERR_THRESH);
}

/**
 * set_phyerrthreshold - set the physical error threshold
 * @ppd: the physical port data
 * @n: the new threshold
 *
 * Note that this will only take effect when the link state changes.
 */
static int set_phyerrthreshold(struct qib_pportdata *ppd, unsigned n)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PHYERR_THRESH,
					 (u32)n);
	return 0;
}

/**
 * get_linkdowndefaultstate - get the default linkdown state
 * @ppd: the physical port data
 *
 * Returns zero if the default is POLL, 1 if the default is SLEEP.
 */
static int get_linkdowndefaultstate(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT) ==
		IB_LINKINITCMD_SLEEP;
}

static int check_mkey(struct qib_ibport *ibp, struct ib_smp *smp, int mad_flags)
{
	int valid_mkey = 0;
	int ret = 0;

	/* Is the mkey in the process of expiring? */
	if (ibp->mkey_lease_timeout &&
	    time_after_eq(jiffies, ibp->mkey_lease_timeout)) {
		/* Clear timeout and mkey protection field. */
		ibp->mkey_lease_timeout = 0;
		ibp->mkeyprot = 0;
	}

	if ((mad_flags & IB_MAD_IGNORE_MKEY) ||  ibp->mkey == 0 ||
	    ibp->mkey == smp->mkey)
		valid_mkey = 1;

	/* Unset lease timeout on any valid Get/Set/TrapRepress */
	if (valid_mkey && ibp->mkey_lease_timeout &&
	    (smp->method == IB_MGMT_METHOD_GET ||
	     smp->method == IB_MGMT_METHOD_SET ||
	     smp->method == IB_MGMT_METHOD_TRAP_REPRESS))
		ibp->mkey_lease_timeout = 0;

	if (!valid_mkey) {
		switch (smp->method) {
		case IB_MGMT_METHOD_GET:
			/* Bad mkey not a violation below level 2 */
			if (ibp->mkeyprot < 2)
				break;
		case IB_MGMT_METHOD_SET:
		case IB_MGMT_METHOD_TRAP_REPRESS:
			if (ibp->mkey_violations != 0xFFFF)
				++ibp->mkey_violations;
			if (!ibp->mkey_lease_timeout && ibp->mkey_lease_period)
				ibp->mkey_lease_timeout = jiffies +
					ibp->mkey_lease_period * HZ;
			/* Generate a trap notice. */
			qib_bad_mkey(ibp, smp);
			ret = 1;
		}
	}

	return ret;
}

static int subn_get_portinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	struct ib_port_info *pip = (struct ib_port_info *)smp->data;
	u8 mtu;
	int ret;
	u32 state;
	u32 port_num = be32_to_cpu(smp->attr_mod);

	if (port_num == 0)
		port_num = port;
	else {
		if (port_num > ibdev->phys_port_cnt) {
			smp->status |= IB_SMP_INVALID_FIELD;
			ret = reply(smp);
			goto bail;
		}
		if (port_num != port) {
			ibp = to_iport(ibdev, port_num);
			ret = check_mkey(ibp, smp, 0);
			if (ret) {
				ret = IB_MAD_RESULT_FAILURE;
				goto bail;
			}
		}
	}

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;

	/* Clear all fields.  Only set the non-zero fields. */
	memset(smp->data, 0, sizeof(smp->data));

	/* Only return the mkey if the protection field allows it. */
	if (!(smp->method == IB_MGMT_METHOD_GET &&
	      ibp->mkey != smp->mkey &&
	      ibp->mkeyprot == 1))
		pip->mkey = ibp->mkey;
	pip->gid_prefix = ibp->gid_prefix;
	pip->lid = cpu_to_be16(ppd->lid);
	p