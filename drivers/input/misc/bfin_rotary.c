	ret = kstrtoul(page, 0, &val);
	if (ret < 0) {
		pr_err("kstrtoul() failed with ret: %d\n", ret);
		return -EINVAL;
	}
	if (val > MAX_SRPT_SRQ_SIZE) {
		pr_err("val: %lu exceeds MAX_SRPT_SRQ_SIZE: %d\n", val,
			MAX_SRPT_SRQ_SIZE);
		return -EINVAL;
	}
	if (val < MIN_SRPT_SRQ_SIZE) {
		pr_err("val: %lu smaller than MIN_SRPT_SRQ_SIZE: %d\n", val,
			MIN_SRPT_SRQ_SIZE);
		return -EINVAL;
	}
	sport->port_attrib.srp_sq_size = val;

	return count;
}

CONFIGFS_ATTR(srpt_tpg_attrib_,  srp_max_rdma_size);
CONFIGFS_ATTR(srpt_tpg_attrib_,  srp_max_rsp_size);
CONFIGFS_ATTR(srpt_tpg_attrib_,  srp_sq_size);

static struct configfs_attribute *srpt_tpg_attrib_attrs[] = {
	&srpt_tpg_attrib_attr_srp_max_rdma_size,
	&srpt_tpg_attrib_attr_srp_max_rsp_size,
	&srpt_tpg_attrib_attr_srp_sq_size,
	NULL,
};

static ssize_t srpt_tpg_enable_show(struct config_item *item, char *page)
{
	struct se_portal_group *se_tpg = to_tpg(item);
	struct srpt_port *sport = container_of(se_tpg, struct srpt_port, port_tpg_1);

	return snprintf(page, PAGE_SIZE, "%d\n", (sport->enabled) ? 1: 0);
}

static ssize_t srpt_tpg_enable_store(struct config_item *item,
		const char *page, size_t count)
{
	struct se_portal_group *se_tpg = to_tpg(item);
	struct srpt_port *sport = container_of(se_tpg, struct srpt_port, port_tpg_1);
	unsigned long tmp;
        int ret;

	ret = kstrtoul(page, 0, &tmp);
	if (ret < 0) {
		pr_err("Unable to extract srpt_tpg_store_enable\n");
		return -EINVAL;
	}

	if ((tmp != 0) && (tmp != 1)) {
		pr_err("Illegal value for srpt_tpg_store_enable: %lu\n", tmp);
		return -EINVAL;
	}
	if (tmp == 1)
		sport->enabled = true;
	else
		sport->enabled = false;

	return count;
}

CONFIGFS_ATTR(srpt_tpg_, enable);

static struct configfs_attribute *srpt_tpg_attrs[] = {
	&srpt_tpg_attr_enable,
	NULL,
};

/**
 * configfs callback invoked for
 * mkdir /sys/kernel/config/target/$driver/$port/$tpg
 */
static struct se_portal_group *srpt_make_tpg(struct se_wwn *wwn,
					     struct config_group *group,
					     const char *name)
{
	struct srpt_port *sport = container_of(wwn, struct srpt_port, port_wwn);
	int res;

	/* Initialize sport->port_wwn and sport->port_tpg_1 */
	res = core_tpg_register(&sport->port_wwn, &sport->port_tpg_1, SCSI_PROTOCOL_SRP);
	if (res)
		return ERR_PTR(res);

	return &sport->port_tpg_1;
}

/**
 * configfs callback invoked for
 * rmdir /sys/kernel/config/target/$driver/$port/$tpg
 */
static void srpt_drop_tpg(struct se_portal_group *tpg)
{
	struct srpt_port *sport = container_of(tpg,
				struct srpt_port, port_tpg_1);

	sport->enabled = false;
	core_tpg_deregister(&sport->port_tpg_1);
}

/**
 * configfs callback invoked for
 * mkdir /sys/kernel/config/target/$driver/$port
 */
static struct se_wwn *srpt_make_tport(struct target_fabric_configfs *tf,
				      struct config_group *group,
				      const char *name)
{
	struct srpt_port *sport;
	int ret;

	sport = srpt_lookup_port(name);
	pr_debug("make_tport(%s)\n", name);
	ret = -EINVAL;
	if (!sport)
		goto err;

	return &sport->port_wwn;

err:
	return ERR_PTR(ret);
}

/**
 * configfs callback invoked for
 * rmdir /sys/kernel/config/target/$driver/$port
 */
static void srpt_drop_tport(struct se_wwn *wwn)
{
	struct srpt_port *sport = container_of(wwn, struct srpt_port, port_wwn);

	pr_debug("drop_tport(%s\n", config_item_name(&sport->port_wwn.wwn_group.cg_item));
}

static ssize_t srpt_wwn_version_show(struct config_item *item, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", DRV_VERSION);
}

CONFIGFS_ATTR_RO(srpt_wwn_, version);

static struct configfs_attribute *srpt_wwn_attrs[] = {
	&srpt_wwn_attr_version,
	NULL,
};

static const struct target_core_fabric_ops srpt_template = {
	.module				= THIS_MODULE,
	.name				= "srpt",
	.node_acl_size			= sizeof(struct srpt_node_acl),
	.get_fabric_name		= srpt_get_fabric_name,
	.tpg_get_wwn			= srpt_get_fabric_wwn,
	.tpg_get_tag			= srpt_get_tag,
	.tpg_check_demo_mode		= srpt_check_false,
	.tpg_check_demo_mode_cache	= srpt_check_true,
	.tpg_check_demo_mode_write_protect = srpt_check_true,
	.tpg_check_prod_mode_write_protect = srpt_check_false,
	.tpg_get_inst_index		= srpt_tpg_get_inst_index,
	.release_cmd			= srpt_release_cmd,
	.check_stop_free		= srpt_check_stop_free,
	.shutdown_session		= srpt_shutdown_session,
	.close_session			= srpt_close_session,
	.sess_get_index			= srpt_sess_get_index,
	.sess_get_initiator_sid		= NULL,
	.write_pending			= srpt_write_pending,
	.write_pending_status		= srpt_write_pending_status,
	.set_default_node_attributes	= srpt_set_default_node_attrs,
	.get_cmd_state			= srpt_get_tcm_cmd_state,
	.queue_data_in			= srpt_queue_data_in,
	.queue_status			= srpt_queue_status,
	.queue_tm_rsp			= srpt_queue_tm_rsp,
	.aborted_task			= srpt_aborted_task,
	/*
	 * Setup function pointers for generic logic in
	 * target_core_fabric_configfs.c
	 */
	.fabric_make_wwn		= srpt_make_tport,
	.fabric_drop_wwn		= srpt_drop_tport,
	.fabric_make_tpg		= srpt_make_tpg,
	.fabric_drop_tpg		= srpt_drop_tpg,
	.fabric_init_nodeacl		= srpt_init_nodeacl,
	.fabric_cleanup_nodeacl		= srpt_cleanup_nodeacl,

	.tfc_wwn_attrs			= srpt_wwn_attrs,
	.tfc_tpg_base_attrs		= srpt_tpg_attrs,
	.tfc_tpg_attrib_attrs		= srpt_tpg_attrib_attrs,
};

/**
 * srpt_init_module() - Kernel module initialization.
 *
 * Note: Since ib_register_client() registers callback functions, and since at
 * least one of these callback functions (srpt_add_one()) calls target core
 * functions, this driver must be registered with the target core before
 * ib_register_client() is called.
 */
static int __init srpt_init_module(void)
{
	int ret;

	ret = -EINVAL;
	if (srp_max_req_size < MIN_MAX_REQ_SIZE) {
		pr_err("invalid value %d for kernel module parameter"
		       " srp_max_req_size -- must be at least %d.\n",
		       srp_max_req_size, MIN_MAX_REQ_SIZE);
		goto out;
	}

	if (srpt_srq_size < MIN_SRPT_SRQ_SIZE
	    || srpt_srq_size > MAX_SRPT_SRQ_SIZE) {
		pr_err("invalid value %d for kernel module parameter"
		       " srpt_srq_size -- must be in the range [%d..%d].\n",
		       srpt_srq_size, MIN_SRPT_SRQ_SIZE, MAX_SRPT_SRQ_SIZE);
		goto out;
	}

	ret = target_register_template(&srpt_template);
	if (ret)
		goto out;

	ret = ib_register_client(&srpt_client);
	if (ret) {
		pr_err("couldn't register IB client\n");
		goto out_unregister_target;
	}

	return 0;

out_unregister_target:
	target_unregister_template(&srpt_template);
out:
	return ret;
}

static void __exit srpt_cleanup_module(void)
{
	ib_unregister_client(&srpt_client);
	target_unregister_template(&srpt_template);
}

module_init(srpt_init_module);
module_exit(srpt_cleanup_module);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    