st_rcv
		- ibp->z_multicast_rcv);

bail:
	return reply((struct ib_smp *) pmp);
}

static int pma_set_portcounters(struct ib_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;

	/*
	 * Since the HW doesn't support clearing counters, we save the
	 * current count and subtract it from future responses.
	 */
	qib_get_counters(ppd, &cntrs);

	if (p->counter_select & IB_PMA_SEL_SYMBOL_ERROR)
		ibp->z_symbol_error_counter = cntrs.symbol_error_counter;

	if (p->counter_select & IB_PMA_SEL_LINK_ERROR_RECOVERY)
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;

	if (p->counter_select & IB_PMA_SEL_LINK_DOWNED)
		ibp->z_link_downed_counter = cntrs.link_downed_counter;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_ERRORS)
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_REMPHYS_ERRORS)
		ibp->z_port_rcv_remphys_errors =
			cntrs.port_rcv_remphys_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_DISCARDS)
		ibp->z_port_xmit_discards = cntrs.port_xmit_discards;

	if (p->counter_select & IB_PMA_SEL_LOCAL_LINK_INTEGRITY_ERRORS)
		ibp->z_local_link_integrity_errors =
			cntrs.local_link_integrity_errors;

	if (p->counter_select & IB_PMA_SEL_EXCESSIVE_BUFFER_OVERRUNS)
		ibp->z_excessive_buffer_overrun_errors =
			cntrs.excessive_buffer_overrun_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_VL15_DROPPED) {
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_DATA)
		ibp->z_port_xmit_data = cntrs.port_xmit_data;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_DATA)
		ibp->z_port_rcv_data = cntrs.port_rcv_data;

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_PACKETS)
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_PACKETS)
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;

	return pma_get_portcounters(pmp, ibdev, port);
}

static int pma_set_portcounters_cong(struct ib_pma_mad *pmp,
				     struct ib_device *ibdev, u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_devdata *dd = dd_from_ppd(ppd);
	struct qib_verbs_counters cntrs;
	u32 counter_select = (be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24) & 0xFF;
	int ret = 0;
	unsigned long flags;

	qib_get_counters(ppd, &cntrs);
	/* Get counter values before we save them */
	ret = pma_get_portcounters_cong(pmp, ibdev, port);

	if (counter_select & IB_PMA_SEL_CONG_XMIT) {
		spin_lock_irqsave(&ppd->ibport_data.lock, flags);
		ppd->cong_stats.counter = 0;
		dd->f_set_cntr_sample(ppd, QIB_CONG_TIMER_PSINTERVAL,
				      0x0);
		spin_unlock_irqrestore(&ppd->ibport_data.lock, flags);
	}
	if (counter_select & IB_PMA_SEL_CONG_PORT_DATA) {
		ibp->z_port_xmit_data = cntrs.port_xmit_data;
		ibp->z_port_rcv_data = cntrs.port_rcv_data;
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;
	}
	if (counter_select & IB_PMA_SEL_CONG_ALL) {
		ibp->z_symbol_error_counter =
			cntrs.symbol_error_counter;
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;
		ibp->z_link_downed_counter =
			cntrs.link_downed_counter;
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;
		ibp->z_port_rcv_remphys_errors =
			cntrs.port_rcv_remphys_errors;
		ibp->z_port_xmit_discards =
			cntrs.port_xmit_discards;
		ibp->z_local_link_integrity_errors =
			cntrs.local_link_integrity_errors;
		ibp->z_excessive_buffer_overrun_errors =
			cntrs.excessive_buffer_overrun_errors;
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	return ret;
}

static int pma_set_portcounters_ext(struct ib_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u64 swords, rwords, spkts, rpkts, xwait;
	struct qib_pma_counters pma;

	qib_snapshot_counters(ppd, &swords, &rwords, &spkts, &rpkts, &xwait);

	if (p->counter_select & IB_PMA_SELX_PORT_XMIT_DATA)
		ibp->z_port_xmit_data = swords;

	if (p->counter_select & IB_PMA_SELX_PORT_RCV_DATA)
		ibp->z_port_rcv_data = rwords;

	if (p->counter_select & IB_PMA_SELX_PORT_XMIT_PACKETS)
		ibp->z_port_xmit_packets = spkts;

	if (p->counter_select & IB_PMA_SELX_PORT_RCV_PACKETS)
		ibp->z_port_rcv_packets = rpkts;

	qib_snapshot_pmacounters(ibp, &pma);

	if (p->counter_select & IB_PMA_SELX_PORT_UNI_XMIT_PACKETS)
		ibp->z_unicast_xmit = pma.n_unicast_xmit;

	if (p->counter_select & IB_PMA_SELX_PORT_UNI_RCV_PACKETS)
		ibp->z_unicast_rcv = pma.n_unicast_rcv;

	if (p->counter_select & IB_PMA_SELX_PORT_MULTI_XMIT_PACKETS)
		ibp->z_multicast_xmit = pma.n_multicast_xmit;

	if (p->counter_select & IB_PMA_SELX_PORT_MULTI_RCV_PACKETS)
		ibp->z_multicast_rcv = pma.n_multicast_rcv;

	return pma_get_portcounters_ext(pmp, ibdev, port);
}

static int process_subn(struct ib_device *ibdev, int mad_flags,
			u8 port, const struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_smp *smp = (struct ib_smp *)out_mad;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int ret;

	*out_mad = *in_mad;
	if (smp->class_version != 1) {
		smp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply(smp);
		goto bail;
	}

	ret = check_mkey(ibp, smp, mad_flags);
	if (ret) {
		u32 port_num = be32_to_cpu(smp->attr_mod);

		/*
		 * If this is a get/set portinfo, we already check the
		 * M_Key if the MAD is for another port and the M_Key
		 * is OK on the receiving port. This check is needed
		 * to increment the error counters when the M_Key
		 * fails to match on *both* ports.
		 */
		if (in_mad->mad_hdr.attr_id == IB_SMP_ATTR_PORT_INFO &&
		    (smp->method == IB_MGMT_METHOD_GET ||
		     smp->method == IB_MGMT_METHOD_SET) &&
		    port_num && port_num <= ibdev->phys_port_cnt &&
		    port != port_num)
			(void) check_mkey(to_iport(ibdev, port_num), smp, 0);
		ret = IB_MAD_RESULT_FAILURE;
		goto bail;
	}

	switch (smp->method) {
	case IB_MGMT_METHOD_GET:
		switch (smp->attr_id) {
		case IB_SMP_ATTR_NODE_DESC:
			ret = subn_get_nodedescription(smp, ibdev);
			goto bail;
		case IB_SMP_ATTR_NODE_INFO:
			ret = subn_get_nodeinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_GUID_INFO:
			ret = subn_get_guidinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PORT_INFO:
			ret = subn_get_portinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PKEY_TABLE:
			ret = subn_get_pkeytable(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SL_TO_VL_TABLE:
			ret = subn_get_sl_to_vl(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_VL_ARB_TABLE:
			ret = subn_get_vl_arb(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SM_INFO:
			if (ibp->port_cap_flags & IB_PORT_SM_DISABLED) {
				ret = IB_MAD_RESULT_SUCCESS |
					IB_MAD_RESULT_CONSUMED;
				goto bail;
			}
			if (ibp->port_cap_flags & IB_PORT_SM) {
				ret = IB_MAD_RESULT_SUCCESS;
				goto bail;
			}
			/* FALLTHROUGH */
		default:
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (smp->attr_id) {
		case IB_SMP_ATTR_GUID_INFO:
			ret = subn_set_guidinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PORT_INFO:
			ret = subn_set_portinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PKEY_TABLE:
			ret = subn_set_pkeytable(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SL_TO_VL_TABLE:
			ret = subn_set_sl_to_vl(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_VL_ARB_TABLE:
			ret = subn_set_vl_arb(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SM_INFO:
			if (ibp->port_cap_flags & IB_PORT_SM_DISABLED) {
				ret = IB_MAD_RESULT_SUCCESS |
					IB_MAD_RESULT_CONSUMED;
				goto bail;
			}
			if (ibp->port_cap_flags & IB_PORT_SM) {
				ret = IB_MAD_RESULT_SUCCESS;
				goto bail;
			}
			/* FALLTHROUGH */
		default:
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
			goto bail;
		}

	case IB_MGMT_METHOD_TRAP_REPRESS:
		if (smp->attr_id == IB_SMP_ATTR_NOTICE)
			ret = subn_trap_repress(smp, ibdev, port);
		else {
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
		}
		goto bail;

	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_REPORT:
	case IB_MGMT_METHOD_REPORT_RESP:
	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	case IB_MGMT_METHOD_SEND:
		if (ib_get_smp_direction(smp) &&
		    smp->attr_id == QIB_VENDOR_IPG) {
			ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PORT,
					      smp->data[0]);
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		} else
			ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	default:
		smp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply(smp);
	}

bail:
	return ret;
}

static int process_perf(struct ib_device *ibdev, u8 port,
			const struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_pma_mad *pmp = (struct ib_pma_mad *)out_mad;
	int ret;

	*out_mad = *in_mad;
	if (pmp->mad_hdr.class_version != 1) {
		pmp->mad_hdr.status |= IB_SMP_UNSUP_VERSION;
		ret = reply((struct ib_smp *) pmp);
		goto bail;
	}

	switch (pmp->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
		switch (pmp->mad_hdr.attr_id) {
		case IB_PMA_CLASS_PORT_INFO:
			ret = pma_get_classportinfo(pmp, ibdev);
			goto bail;
		case IB_PMA_PORT_SAMPLES_CONTROL:
			ret = pma_get_portsamplescontrol(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_SAMPLES_RESULT:
			ret = pma_get_portsamplesresult(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_SAMPLES_RESULT_EXT:
			ret = pma_get_portsamplesresult_ext(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS:
			ret = pma_get_portcounters(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_EXT:
			ret = pma_get_portcounters_ext(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_CONG:
			ret = pma_get_portcounters_cong(pmp, ibdev, port);
			goto bail;
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_