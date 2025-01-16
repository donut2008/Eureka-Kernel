srp_conn_unique() - check whether the connection to a target is unique
 * @host:   SRP host.
 * @target: SRP target port.
 */
static bool srp_conn_unique(struct srp_host *host,
			    struct srp_target_port *target)
{
	struct srp_target_port *t;
	bool ret = false;

	if (target->state == SRP_TARGET_REMOVED)
		goto out;

	ret = true;

	spin_lock(&host->target_lock);
	list_for_each_entry(t, &host->target_list, list) {
		if (t != target &&
		    target->id_ext == t->id_ext &&
		    target->ioc_guid == t->ioc_guid &&
		    target->initiator_ext == t->initiator_ext) {
			ret = false;
			break;
		}
	}
	spin_unlock(&host->target_lock);

out:
	return ret;
}

/*
 * Target ports are added by writing
 *
 *     id_ext=<SRP ID ext>,ioc_guid=<SRP IOC GUID>,dgid=<dest GID>,
 *     pkey=<P_Key