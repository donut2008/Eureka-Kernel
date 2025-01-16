gestion_setting(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	int i;
	struct ib_cc_congestion_setting_attr *p =
		(struct ib_cc_congestion_setting_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct ib_cc_congestion_entry_shadow *entries;

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	spin_lock(&ppd->cc_shadow_lock);

	entries = ppd->congestion_entries_shadow->entries;
	p->port_control = cpu_to_be16(
		ppd->congestion_entries_shadow->port_control);
	p->control_map = cpu_to_be16(
		ppd->congestion_entries_shadow->control_map);
	for (i = 0; i < IB_CC_CCS_ENTRIES; i++) {
		p->entries[i].ccti_increase = entries[i].ccti_increase;
		p->entries[i].ccti_timer = cpu_to_be16(entries[i].ccti_timer);
		p->entries[i].trigger_threshold = entries[i].trigger_threshold;
		p->entries[i].ccti_min = entries[i].ccti_min;
	}

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);
}

static int cc_get_congestion_control_table(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_table_attr *p =
		(struct ib_cc_table_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 cct_block_index = be32_to_cpu(ccp->attr_mod);
	u32 max_cct_block;
	u32 cct_entry;
	struct ib_cc_table_entry_shadow *entries;
	int i;

	/* Is the table index more than what is supported? */
	if (cct_block_index > IB_CC_TABLE_CAP_DEFAULT - 1)
		goto bail;

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	spin_lock(&ppd->cc_shadow_lock);

	max_cct_block =
		(ppd->ccti_entries_shadow->ccti_last_entry + 1)/IB_CCT_ENTRIES;
	max_cct_block = max_cct_block ? max_cct_block - 1 : 0;

	if (cct_block_index > max_cct_block) {
		spin_unlock(&ppd->cc_shadow_lock);
		goto bail;
	}

	ccp->attr_mod = cpu_to_be32(cct_block_index);

	cct_entry = IB_CCT_ENTRIES * (cct_block_index + 1);

	cct_entry--;

	p->ccti_limit = cpu_to_be16(cct_entry);

	entries = &ppd->ccti_entries_shadow->
			entries[IB_CCT_ENTRIES * cct_block_index];
	cct_entry %= IB_CCT_ENTRIES;

	for (i = 0; i <= cct_entry; i++)
		p->ccti_entries[i].entry = cpu_to_be16(entries[i].entry);

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);

bail:
	return reply_failure((struct ib_smp *) ccp);
}

static int cc_set_congestion_setting(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_congestion_setting_attr *p =
		(struct ib_cc_congestion_setting_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int i;

	ppd->cc_sl_control_map = be16_to_cpu(p->control_map);

	for (i = 0; i < IB_CC_CCS_ENTRIES; i++) {
		ppd->congestion_entries[i].ccti_increase =
			p->entries[i].ccti_increase;

		ppd->congestion_entries[i].ccti_timer =
			be16_to_cpu(p->entries[i].ccti_timer);

		ppd->congestion_entries[i].trigger_threshold =
			p->entries[i].trigger_threshold;

		ppd->congestion_entries[i].ccti_min =
			p->entries[i].ccti_min;
	}

	return reply((struct ib_smp *) ccp);
}

static int cc_set_congestion_control_table(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_table_attr *p =
		(struct ib_cc_table_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 cct_block_index = be32_to_cpu(ccp->attr_mod);
	u32 cct_entry;
	struct ib_cc_table_entry_shadow *entries;
	int i;

	/* Is the table index more than what is supported? */
	if (cct_block_index > IB_CC_TABLE_CAP_DEFAULT - 1)
		goto bail;

	/* If this packet is the first in the sequence then
	 * zero the total table entry count.
	 */
	if (be16_to_cpu(p->ccti_limit) < IB_CCT_ENTRIES)
		ppd->total_cct_entry = 0;

	cct_entry = (be16_to_cpu(p->ccti_limit))%IB_CCT_ENTRIES;

	/* ccti_limit is 0 to 63 */
	ppd->total_cct_entry += (cct_entry + 1);

	if (ppd->total_cct_entry > ppd->cc_supported_table_entries)
		goto bail;

	ppd->ccti_limit = be16_to_cpu(p->ccti_limit);

	entries = ppd->ccti_entries + (IB_CCT_ENTRIES * cct_block_index);

	for (i = 0; i <= cct_entry; i++)
		entries[i].entry = be16_to_cpu(p->ccti_entries[i].entry);

	spin_lock(&ppd->cc_shadow_lock);

	ppd->ccti_entries_shadow->ccti_last_entry = ppd->total_cct_entry - 1;
	memcpy(ppd->ccti_entries_shadow->entries, ppd->ccti_entries,
		(ppd->total_cct_entry * sizeof(struct ib_cc_table_entry)));

	ppd->congestion_entries_shadow->port_control = IB_CC_CCS_PC_SL_BASED;
	ppd->congestion_entries_shadow->control_map = ppd->cc_sl_control_map;
	memcpy(ppd->congestion_entries_shadow->entries, ppd->congestion_entries,
		IB_CC_CCS_ENTRIES * sizeof(struct ib_cc_congestion_entry));

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);

bail:
	return reply_failure((struct ib_smp *) ccp);
}

static int check_cc_key(struct qib_ibport *ibp,
			struct ib_cc_mad *ccp, int mad_flags)
{
	return 0;
}

static int process_cc(struct ib_device *ibdev, int mad_flags,
			u8 port, const struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_cc_mad *ccp = (struct ib_cc_mad *)out_mad;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	int ret;

	*out_mad = *in_mad;

	if (ccp->class_version != 2) {
		ccp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply((struct ib_smp *)ccp);
		goto bail;
	}

	ret = check_cc_key(ibp, ccp, mad_flags);
	if (ret)
		goto bail;

	switch (ccp->method) {
	case IB_MGMT_METHOD_GET:
		switch (ccp->attr_id) {
		case IB_CC_ATTR_CLASSPORTINFO:
			ret = cc_get_classportinfo(ccp, ibdev);
			goto bail;

		case IB_CC_ATTR_CONGESTION_INFO:
			ret = cc_get_congestion_info(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CA_CONGESTION_SETTING:
			ret = cc_get_congestion_setting(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CONGESTION_CONTROL_TABLE:
			ret = cc_get_congestion_control_table(ccp, ibdev, port);
			goto bail;

			/* FALLTHROUGH */
		default:
			ccp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) ccp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (ccp->attr_id) {
		case IB_CC_ATTR_CA_CONGESTION_SETTING:
			ret = cc_set_congestion_setting(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CONGESTION_CONTROL_TABLE:
			ret = cc_set_congestion_control_table(ccp, ibdev, port);
			goto bail;

			/* FALLTHROUGH */
		default:
			ccp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) ccp);
			goto bail;
		}

	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	case IB_MGMT_METHOD_TRAP:
	default:
		ccp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply((struct ib_smp *) ccp);
	}

bail:
	return ret;
}

/**
 * qib_process_mad - process an incoming MAD packet
 * @ibdev: the infiniband device this packet came in on
 * @mad_flags: MAD flags
 * @port: the port number this packet came in on
 * @in_wc: the work completion entry for this packet
 * @in_grh: the global route header for this packet
 * @in_mad: the incoming MAD
 * @out_mad: any outgoing MAD reply
 *
 * Returns IB_MAD_RESULT_SUCCESS if this is a MAD that we are not
 * interested in processing.
 *
 * Note that the verbs framework has already done the MAD sanity checks,
 * and hop count/pointer updating for IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE
 * MADs.
 *
 * This is called by the ib_mad module.
 */
int qib_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
		    const struct ib_wc *in_wc, const struct ib_grh *in_grh,
		    const struct ib_mad_hdr *in, size_t in_mad_size,
		    struct ib_mad_hdr *out, size_t *out_mad_size,
		    u16 *out_mad_pkey_index)
{
	int ret;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	const struct ib_mad *in_mad = (const struct ib_mad *)in;
	struct ib_mad *out_mad = (struct ib_mad *)out;

	if (WARN_ON_ONCE(in_mad_size != sizeof(*in_mad) ||
			 *out_mad_size != sizeof(*out_mad)))
		return IB_MAD_RESULT_FAILURE;

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		ret = process_subn(ibdev, mad_flags, port, in_mad, out_mad);
		goto bail;

	case IB_MGMT_CLASS_PERF_MGMT:
		ret = process_