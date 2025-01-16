,		   NULL);
static DEVICE_ATTR(sgid,	    S_IRUGO, show_sgid,		   NULL);
static DEVICE_ATTR(dgid,	    S_IRUGO, show_dgid,		   NULL);
static DEVICE_ATTR(orig_dgid,	    S_IRUGO, show_orig_dgid,	   NULL);
static DEVICE_ATTR(req_lim,         S_IRUGO, show_req_lim,         NULL);
static DEVICE_ATTR(zero_req_lim,    S_IRUGO, show_zero_req_lim,	   NULL);
static DEVICE_ATTR(local_ib_port,   S_IRUGO, show_local_ib_port,   NULL);
static DEVICE_ATTR(local_ib_device, S_IRUGO, show_local_ib_device, NULL);
static DEVICE_ATTR(ch_count,        S_IRUGO, show_ch_count,        NULL);
static DEVICE_ATTR(comp_vector,     S_IRUGO, show_comp_vector,     NULL);
static DEVICE_ATTR(tl_retry_count,  S_IRUGO, show_tl_retry_count,  NULL);
static DEVICE_ATTR(cmd_sg_entries,  S_IRUGO, show_cmd_sg_entries,  NULL);
static DEVICE_ATTR(allow_ext_sg,    S_IRUGO, show_allow_ext_sg,    NULL);

static struct device_attribute *srp_host_attrs[] = {
	&dev_attr_id_ext,
	&dev_attr_ioc_guid,
	&dev_attr_service_id,
	&dev_attr_pkey,
	&dev_attr_sgid,
	&dev_attr_dgid,
	&dev_attr_orig_dgid,
	&dev_attr_req_lim,
	&dev_attr_zero_req_lim,
	&dev_attr_local_ib_port,
	&dev_attr_local_ib_device,
	&dev_attr_ch_count,
	&dev_attr_comp_vector,
	&dev_attr_tl_retry_count,
	&dev_attr_cmd_sg_entries,
	&dev_attr_allow_ext_sg,
	NULL
};

static struct scsi_host_template srp_template = {
	.module				= THIS_MODULE,
	.name				= "InfiniBand SRP initiator",
	.proc_name			= DRV_NAME,
	.slave_configure		= srp_slave_configure,
	.info				= srp_target_info,
	.queuecommand			= srp_queuecommand,
	.change_queue_depth             = srp_change_queue_depth,
	.eh_abort_handler		= srp_abort,
	.eh_device_reset_handler	= srp_reset_device,
	.eh_host_reset_handler		= srp_reset_host,
	.skip_settle_delay		= true,
	.sg_tablesize			= SRP_DEF_SG_TABLESIZE,
	.can_queue			= SRP_DEFAULT_CMD_SQ_SIZE,
	.this_id			= -1,
	.cmd_per_lun			= SRP_DEFAULT_CMD_SQ_SIZE,
	.use_clustering			= ENABLE_CLUSTERING,
	.shost_attrs			= srp_host_attrs,
	.track_queue_depth		= 1,
};

static int srp_sdev_count(struct Scsi_Host *host)
{
	struct scsi_device *sdev;
	int c = 0;

	shost_for_each_device(sdev, host)
		c++;

	return c;
}

/*
 * Return values:
 * < 0 upon failure. Caller is responsible for SRP target port cleanup.
 * 0 and target->state == SRP_TARGET_REMOVED if asynchronous target port
 *    removal has been scheduled.
 * 0 and target->state != SRP_TARGET_REMOVED upon success.
 */
static int srp_add_target(struct srp_host *host, struct srp_target_port *target)
{
	struct srp_rport_identifiers ids;
	struct srp_rport *rport;

	target->state = SRP_TARGET_SCANNING;
	sprintf(target->target_name, "SRP.T10:%016llX",
		be64_to_cpu(target->id_ext));

	if (scsi_add_host(target->scsi_host, host->srp_dev->dev->dma_device))
		return -ENODEV;

	memcpy(ids.port_id, &target->id_ext, 8);
	memcpy(ids.port_id + 8, &target->ioc_guid, 8);
	ids.roles = SRP_RPORT_ROLE_TARGET;
	rport = srp_rport_add(target->scsi_host, &ids);
	if (IS_ERR(rport)) {
		scsi_remove_host(target->scsi_host);
		return PTR_ERR(rport);
	}

	rport->lld_data = target;
	target->rport = rport;

	spin_lock(&host->target_lock);
	list_add_tail(&target->list, &host->target_list);
	spin_unlock(&host->target_lock);

	scsi_scan_target(&target->scsi_host->shost_gendev,
			 0, target->scsi_id, SCAN_WILD_CARD, 0);

	if (srp_connected_ch(target) < target->ch_count ||
	    target->qp_in_error) {
		shost_printk(KERN_INFO, target->scsi_host,
			     PFX "SCSI scan failed - removing SCSI host\n");
		srp_queue_remove_work(target);
		goto out;
	}

	pr_debug(PFX "%s: SCSI scan succeeded - detected %d LUNs\n",
		 dev_name(&target->scsi_host->shost_gendev),
		 srp_sdev_count(target->scsi_host));

	spin_lock_irq(&target->lock);
	if (target->state == SRP_TARGET_SCANNING)
		target->state = SRP_TARGET_LIVE;
	spin_unlock_irq(&target->lock);

out:
	return 0;
}

static voi