&req, msg);
	msg->path_msg = true;
	return 0;
}

static int drm_dp_mst_assign_payload_id(struct drm_dp_mst_topology_mgr *mgr,
					struct drm_dp_vcpi *vcpi)
{
	int ret, vcpi_ret;

	mutex_lock(&mgr->payload_lock);
	ret = find_first_zero_bit(&mgr->payload_mask, mgr->max_payloads + 1);
	if (ret > mgr->max_payloads) {
		ret = -EINVAL;
		DRM_DEBUG_KMS("out of payload ids %d\n", ret);
		goto out_unlock;
	}

	vcpi_ret = find_first_zero_bit(&mgr->vcpi_mask, mgr->max_payloads + 1);
	if (vcpi_ret > mgr->max_payloads) {
		ret = -EINVAL;
		DRM_DEBUG_KMS("out of vcpi ids %d\n", ret);
		goto out_unlock;
	}

	set_bit(ret, &mgr->payload_mask);
	set_bit(vcpi_ret, &mgr->vcpi_mask);
	vcpi->vcpi = vcpi_ret + 1;
	mgr->proposed_vcpis[ret - 1] = vcpi;
out_unlock:
	mutex_unlock(&mgr->payload_lock);
	return ret;
}

static void drm_dp_mst_put_payload_id(struct drm_dp_mst_topology_mgr *mgr,
				      int vcpi)
{
	int i;
	if (vcpi == 0)
		return;

	mutex_lock(&mgr->payload_lock);
	DRM_DEBUG_KMS("putting payload %d\n", vcpi);
	clear_bit(vcpi - 1, &mgr->vcpi_mask);

	for (i = 0; i < mgr->max_payloads; i++) {
		if (mgr->proposed_vcpis[i])
			if (mgr->proposed_vcpis[i]->vcpi == vcpi) {
				mgr->proposed_vcpis[i] = NULL;
				clear_bit(i + 1, &mgr->payload_mask);
			}
	}
	mutex_unlock(&mgr->payload_lock);
}

static bool check_txmsg_state(struct drm_dp_mst_topology_mgr *mgr,
			      struct drm_dp_sideband_msg_tx *txmsg)
{
	bool ret;

	/*
	 * All updates to txmsg->state are protected by mgr->qlock, and the two
	 * cases we check here are terminal states. For those the barriers
	 * provided by the wake_up/wait_event pair are enough.
	 */
	ret = (txmsg->state == DRM_DP_SIDEBAND_TX_RX ||
	       txmsg->state == DRM_DP_SIDEBAND_TX_TIMEOUT);
	return ret;
}

static int drm_dp_mst_wait_tx_reply(struct drm_dp_mst_branch *mstb,
				    struct drm_dp_sideband_msg_tx *txmsg)
{
	struct drm_dp_mst_topology_mgr *mgr = mstb->mgr;
	int ret;

	ret = wait_event_timeout(mgr->tx_waitq,
				 check_txmsg_state(mgr, txmsg),
				 (4 * HZ));
	mutex_lock(&mstb->mgr->qlock);
	if (ret > 0) {
		if (txmsg->state == DRM_DP_SIDEBAND_TX_TIMEOUT) {
			ret = -EIO;
			goto out;
		}
	} else {
		DRM_DEBUG_KMS("timedout msg send %p %d %d\n", txmsg, txmsg->state, txmsg->seqno);

		/* dump some state */
		ret = -EIO;

		/* remove from q */
		if (txmsg->state == DRM_DP_SIDEBAND_TX_QUEUED ||
		    txmsg->state == DRM_DP_SIDEBAND_TX_START_SEND) {
			list_del(&txmsg->next);
		}

		if (txmsg->state == DRM_DP_SIDEBAND_TX_START_SEND ||
		    txmsg->state == DRM_DP_SIDEBAND_TX_SENT) {
			mstb->tx_slots[txmsg->seqno] = NULL;
		}
	}
out:
	mutex_unlock(&mgr->qlock);

	return ret;
}

static struct drm_dp_mst_branch *drm_dp_add_mst_branch_device(u8 lct, u8 *rad)
{
	struct drm_dp_mst_branch *mstb;

	mstb = kzalloc(sizeof(*mstb), GFP_KERNEL);
	if (!mstb)
		return NULL;

	mstb->lct = lct;
	if (lct > 1)
		memcpy(mstb->rad, rad, lct / 2);
	INIT_LIST_HEAD(&mstb->ports);
	kref_init(&mstb->kref);
	return mstb;
}

static void drm_dp_free_mst_port(struct kref *kref);

static void drm_dp_free_mst_branch_device(struct kref *kref)
{
	struct drm_dp_mst_branch *mstb = container_of(kref, struct drm_dp_mst_branch, kref);
	if (mstb->port_parent) {
		if (list_empty(&mstb->port_parent->next))
			kref_put(&mstb->port_parent->kref, drm_dp_free_mst_port);
	}
	kfree(mstb);
}

static void drm_dp_destroy_mst_branch_device(struct kref *kref)
{
	struct drm_dp_mst_branch *mstb = container_of(kref, struct drm_dp_mst_branch, kref);
	struct drm_dp_mst_port *port, *tmp;
	bool wake_tx = false;

	/*
	 * init kref again to be used by ports to remove mst branch when it is
	 * not needed anymore
	 */
	kref_init(kref);

	if (mstb->port_parent && list_empty(&mstb->port_parent->next))
		kref_get(&mstb->port_parent->kref);

	/*
	 * destroy all ports - don't need lock
	 * as there are no more references to the mst branch
	 * device at this point.
	 */
	list_for_each_entry_safe(port, tmp, &mstb->ports, next) {
		list_del(&port->next);
		drm_dp_put_port(port);
	}

	/* drop any tx slots msg */
	mutex_lock(&mstb->mgr->qlock);
	if (mstb->tx_slots[0]) {
		mstb->tx_slots[0]->state = DRM_DP_SIDEBAND_TX_TIMEOUT;
		mstb->tx_slots[0] = NULL;
		wake_tx = true;
	}
	if (mstb->tx_slots[1]) {
		mstb->tx_slots[1]->state = DRM_DP_SIDEBAND_TX_TIMEOUT;
		mstb->tx_slots[1] = NULL;
		wake_tx = true;
	}
	mutex_unlock(&mstb->mgr->qlock);

	if (wake_tx)
		wake_up(&mstb->mgr->tx_waitq);

	kref_put(kref, drm_dp_free_mst_branch_device);
}

static void drm_dp_put_mst_branch_device(struct drm_dp_mst_branch *mstb)
{
	kref_put(&mstb->kref, drm_dp_destroy_mst_branch_device);
}


static void drm_dp_port_teardown_pdt(struct drm_dp_mst_port *port, int old_pdt)
{
	struct drm_dp_mst_branch *mstb;

	switch (old_pdt) {
	case DP_PEER_DEVICE_DP_LEGACY_CONV:
	case DP_PEER_DEVICE_SST_SINK:
		/* remove i2c over sideband */
		drm_dp_mst_unregister_i2c_bus(&port->aux);
		break;
	case DP_PEER_DEVICE_MST_BRANCHING:
		mstb = port->mstb;
		port->mstb = NULL;
		drm_dp_put_mst_branch_device(mstb);
		break;
	}
}

static void drm_dp_destroy_port(struct kref *kref)
{
	struct drm_dp_mst_port *port = container_of(kref, struct drm_dp_mst_port, kref);
	struct drm_dp_mst_topology_mgr *mgr = port->mgr;

	if (!port->input) {
		port->vcpi.num_slots = 0;

		kfree(port->cached_edid);

		/*
		 * The only time we don't have a connector
		 * on an output port is if the connector init
		 * fails.
		 */
		if (port->connector) {
			/* we can't destroy the connector here, as
			 * we might be holding the mode_config.mutex
			 * from an EDID retrieval */

			mutex_lock(&mgr->destroy_connector_lock);
			kref_get(&port->parent->kref);
			list_add(&port->next, &mgr->destroy_connector_list);
			mutex_unlock(&mgr->destroy_connector_lock);
			schedule_work(&mgr->destroy_connector_work);
			return;
		}
		/* no need to clean up vcpi
		 * as if we have no connector we never setup a vcpi */
		drm_dp_port_teardown_pdt(port, port->pdt);
		port->pdt = DP_PEER_DEVICE_NONE;
	}
	kfree(port);
}

static void drm_dp_put_port(struct drm_dp_mst_port *port)
{
	kref_put(&port->kref, drm_dp_destroy_port);
}

static struct drm_dp_mst_branch *drm_dp_mst_get_validated_mstb_ref_locked(struct drm_dp_mst_branch *mstb, struct drm_dp_mst_branch *to_find)
{
	struct drm_dp_mst_port *port;
	struct drm_dp_mst_branch *rmstb;
	if (to_find == mstb) {
		kref_get(&mstb->kref);
		return mstb;
	}
	list_for_each_entry(port, &mstb->ports, next) {
		if (port->mstb) {
			rmstb = drm_dp_mst_get_validated_mstb_ref_locked(port->mstb, to_find);
			if (rmstb)
				return rmstb;
		}
	}
	return NULL;
}

static struct drm_dp_mst_branch *drm_dp_get_validated_mstb_ref(struct drm_dp_mst_topology_mgr *mgr, struct drm_dp_mst_branch *mstb)
{
	struct drm_dp_mst_branch *rmstb = NULL;
	mutex_lock(&mgr->lock);
	if (mgr->mst_primary)
		rmstb = drm_dp_mst_get_validated_mstb_ref_locked(mgr->mst_primary, mstb);
	mutex_unlock(&mgr->lock);
	return rmstb;
}

static struct drm_dp_mst_port *drm_dp_mst_get_port_ref_locked(struct drm_dp_mst_branch *mstb, struct drm_dp_mst_port *to_find)
{
	struct drm_dp_mst_port *port, *mport;

	list_for_each_entry(port, &mstb->ports, next) {
		if (port == to_find) {
			kref_get(&port->kref);
			return port;
		}
		if (port->mstb) {
			mport = drm_dp_mst_get_port_ref_locked(port->mstb, to_find);
			if (mport)
				return mport;
		}
	}
	return NULL;
}

static struct drm_dp_mst_port *drm_dp_get_validated_port_ref(struct drm_dp_mst_topology_mgr *mgr, struct drm_dp_mst_port *port)
{
	struct drm_dp_mst_port *rport = NULL;
	mutex_lock(&mgr->lock);
	if (mgr->mst_primary)
		rport = drm_dp_mst_get_port_ref_locked(mgr->mst_primary, port);
	mutex_unlock(&mgr->lock);
	return rport;
}

static struct drm_dp_mst_port *drm_dp_get_port(struct drm_dp_mst_branch *mstb, u8 port_num)
{
	struct drm_dp_mst_port *port;

	list_for_each_entry(port, &mstb->ports, next) {
		if (port->port_num == port_num) {
			kref_get(&port->kref);
			return port;
		}
	}

	return NULL;
}

/*
 * calculate a new RAD for this MST branch device
 * if parent has an LCT of 2 then it has 1 nibble of RAD,
 * if parent has an LCT of 3 then it has 2 nibbles of RAD,
 */
static u8 drm_dp_calculate_rad(struct drm_dp_mst_port *port,
				 u8 *rad)
{
	int parent_lct = port->parent->lct;
	int shift = 4;
	int idx = (parent_lct - 1) / 2;
	if (parent_lct > 1) {
		memcpy(rad, port->parent->rad, idx + 1);
		shift = (parent_lct % 2) ? 4 : 0;
	} else
		rad[0] = 0;

	rad[idx] |= port->port_num << shift;
	return parent_lct + 1;
}

/*
 * return sends link address for new mstb
 */
static bool drm_dp_port_setup_pdt(struct drm_dp_mst_port *port)
{
	int ret;
	u8 rad[6], lct;
	bool send_link = false;
	switch (port->pdt) {
	case DP_PEER_DEVICE_DP_LEGACY_CONV:
	case DP_PEER_DEVICE_SST_SINK:
		/* add i2c over sideband */
		ret = drm_dp_mst_register_i2c_bus(&port->aux);
		break;
	case DP_PEER_DEVICE_MST_BRANCHING:
		lct = drm_dp_calculate_rad(port, rad);

		port->mstb = drm_dp_add_mst_branch_device(lct, rad);
		if (port->mstb) {
			port->mstb->mgr = port->mgr;
			port->mstb->port_parent = port;

			send_link = true;
		}
		break;
	}
	return send_link;
}

static void drm_dp_check_mstb_guid(struct drm_dp_mst_branch *mstb, u8 *guid)
{
	int ret;

	memcpy(mstb->guid, guid, 16);

	if (!drm_dp_validate_guid(mstb->mgr, mstb->guid)) {
		if (mstb->port_parent) {
			ret = drm_dp_send_dpcd_write(
					mstb->mgr,
					mstb->port_parent,
					DP_GUID,
					16,
					mstb->guid);
		} else {

			ret = drm_dp_dpcd_write(
					mstb->mgr->aux,
					DP_GUID,
					mstb->guid,
					16);
		}
	}
}

static void build_mst_prop_path(const struct drm_dp_mst_branch *mstb,
				int pnum,
				char *proppath,
				size_t proppath_size)
{
	int i;
	char temp[8];
	snprintf(proppath, proppath_size, "mst:%d", mstb->mgr->conn_base_id);
	for (i = 0; i < (mstb->lct - 1); i++) {
		int shift = (i % 2) ? 0 : 4;
		int port_num = (mstb->rad[i / 2] >> shift) & 0xf;
		snprintf(temp, sizeof(temp), "-%d", port_num);
		strlcat(proppath, temp, proppath_size);
	}
	snprintf(temp, sizeof(temp), "-%d", pnum);
	strlcat(proppath, temp, proppath_size);
}

static void drm_dp_add_port(struct drm_dp_mst_branch *mstb,
			    struct device *dev,
			    struct drm_dp_link_addr_reply_port *port_msg)
{
	struct drm_dp_mst_port *port;
	bool ret;
	bool created = false;
	int old_pdt = 0;
	int old_ddps = 0;
	port = drm_dp_get_port(mstb, port_msg->port_number);
	if (!port) {
		port = kzalloc(sizeof(*port), GFP_KERNEL);
		if (!port)
			return;
		kref_init(&port->kref);
		port->parent = mstb;
		port->port_num = port_msg->port_number;
		port->mgr = mstb->mgr;
		port->aux.name = "DPMST";
		port->aux.dev = dev;
		created = true;
	} else {
		old_pdt = port->pdt;
		old_ddps = port->ddps;
	}

	port->pdt = port_msg->peer_device_type;
	port->input = port_msg->input_port;
	port->mcs = port_msg->mcs;
	port->ddps = port_msg->ddps;
	port->ldps = port_msg->legacy_device_plug_status;
	port->dpcd_rev = port_msg->dpcd_revision;
	port->num_sdp_streams = port_msg->num_sdp_streams;
	port->num_sdp_stream_sinks = port_msg->num_sdp_stream_sinks;

	/* manage mstb port lists with mgr lock - take a reference
	   for this list */
	if (created) {
		mutex_lock(&mstb->mgr->lock);
		kref_get(&port->kref);
		list_add(&port->next, &mstb->ports);
		mutex_unlock(&mstb->mgr->lock);
	}

	if (old_ddps != port->ddps) {
		if (port->ddps) {
			if (!port->input)
				drm_dp_send_enum_path_resources(mstb->mgr, mstb, port);
		} else {
			port->available_pbn = 0;
			}
	}

	if (old_pdt != port->pdt && !port->input) {
		drm_dp_port_teardown_pdt(port, old_pdt);

		ret = drm_dp_port_setup_pdt(port);
		if (ret == true)
			drm_dp_send_link_address(mstb->mgr, port->mstb);
	}

	if (created && !port->input) {
		char proppath[255];

		build_mst_prop_path(mstb, port->port_num, proppath, sizeof(proppath));
		port->connector = (*mstb->mgr->cbs->add_connector)(mstb->mgr, port, proppath);
		if (!port->connector) {
			/* remove it from the port list */
			mutex_lock(&mstb->mgr->lock);
			list_del(&port->next);
			mutex_unlock(&mstb->mgr->lock);
			/* drop port list reference */
			drm_dp_put_port(port);
			goto out;
		}
		if ((port->pdt == DP_PEER_DEVICE_DP_LEGACY_CONV ||
		     port->pdt == DP_PEER_DEVICE_SST_SINK) &&
		    port->port_num >= DP_MST_LOGICAL_PORT_0) {
			port->cached_edid = drm_get_edid(port->connector, &port->aux.ddc);
			drm_mode_connector_set_tile_property(port->connector);
		}
		(*mstb->mgr->cbs->register_connector)(port->connector);
	}

out:
	/* put reference to this port */
	drm_dp_put_port(port);
}

static void drm_dp_update_port(struct drm_dp_mst_branch *mstb,
			       struct drm_dp_connection_status_notify *conn_stat)
{
	struct drm_dp_mst_port *port;
	int old_pdt;
	int old_ddps;
	bool dowork = false;
	port = drm_dp_get_port(mstb, conn_stat->port_number);
	if (!port)
		return;

	old_ddps = port->ddps;
	old_pdt = port->pdt;
	port->pdt = conn_stat->peer_device_type;
	port->mcs = conn_stat->message_capability_status;
	port->ldps = conn_stat->legacy_device_plug_status;
	port->ddps = conn_stat->displayport_device_plug_status;

	if (old_ddps != port->ddps) {
		if (port->ddps) {
			dowork = true;
		} else {
			port->available_pbn = 0;
		}
	}
	if (old_pdt != port->pdt && !port->input) {
		drm_dp_port_teardown_pdt(port, old_pdt);

		if (drm_dp_port_setup_pdt(port))
			dowork = true;
	}

	drm_dp_put_port(port);
	if (dowork)
		queue_work(system_long_wq, &mstb->mgr->work);

}

static struct drm_dp_mst_branch *drm_dp_get_mst_branch_device(struct drm_dp_mst_topology_mgr *mgr,
							       u8 lct, u8 *rad)
{
	struct drm_dp_mst_branch *mstb;
	struct drm_dp_mst_port *port;
	int i;
	/* find the port by iterating down */

	mutex_lock(&mgr->lock);
	mstb = mgr->mst_primary;

	if (!mstb)
		goto out;

	for (i = 0; i < lct - 1; i++) {
		int shift = (i % 2) ? 0 : 4;
		int port_num = (rad[i / 2] >> shift) & 0xf;

		list_for_each_entry(port, &mstb->ports, next) {
			if (port->port_num == port_num) {
				mstb = port->mstb;
				if (!mstb) {
					DRM_ERROR("failed to lookup MSTB with lct %d, rad %02x\n", lct, rad[0]);
					goto out;
				}

				break;
			}
		}
	}
	kref_get(&mstb->kref);
out:
	mutex_unlock(&mgr->lock);
	return mstb;
}

static struct drm_dp_mst_branch *get_mst_branch_device_by_guid_helper(
	struct drm_dp_mst_branch *mstb,
	uint8_t *guid)
{
	struct drm_dp_mst_branch *found_mstb;
	struct drm_dp_mst_port *port;

	if (!mstb)
		return NULL;

	if (memcmp(mstb->guid, guid, 16) == 0)
		return mstb;


	list_for_each_entry(port, &mstb->ports, next) {
		found_mstb = get_mst_branch_device_by_guid_helper(port->mstb, guid);

		if (found_mstb)
			return found_mstb;
	}

	return NULL;
}

static struct drm_dp_mst_branch *drm_dp_get_mst_branch_device_by_guid(
	struct drm_dp_mst_topology_mgr *mgr,
	uint8_t *guid)
{
	struct drm_dp_mst_branch *mstb;

	/* find the port by iterating down */
	mutex_lock(&mgr->lock);

	mstb = get_mst_branch_device_by_guid_helper(mgr->mst_primary, guid);

	if (mstb)
		kref_get(&mstb->kref);

	mutex_unlock(&mgr->lock);
	return mstb;
}

static void drm_dp_check_and_send_link_address(struct drm_dp_mst_topology_mgr *mgr,
					       struct drm_dp_mst_branch *mstb)
{
	struct drm_dp_mst_port *port;
	struct drm_dp_mst_branch *mstb_child;
	if (!mstb->link_address_sent)
		drm_dp_send_link_address(mgr, mstb);

	list_for_each_entry(port, &mstb->ports, next) {
		if (port->input)
			continue;

		if (!port->ddps)
			continue;

		if (!port->available_pbn)
			drm_dp_send_enum_path_resources(mgr, mstb, port);

		if (port->mstb) {
			mstb_child = drm_dp_get_validated_mstb_ref(mgr, port->mstb);
			if (mstb_child) {
				drm_dp_check_and_send_link_address(mgr, mstb_child);
				drm_dp_put_mst_branch_device(mstb_child);
			}
		}
	}
}

static void drm_dp_mst_link_probe_work(struct work_struct *work)
{
	struct drm_dp_mst_topology_mgr *mgr = container_of(work, struct drm_dp_mst_topology_mgr, work);
	struct drm_dp_mst_branch *mstb;

	mutex_lock(&mgr->lock);
	mstb = mgr->mst_primary;
	if (mstb) {
		kref_get(&mstb->kref);
	}
	mutex_unlock(&mgr->lock);
	if (mstb) {
		drm_dp_check_and_send_link_address(mgr, mstb);
		drm_dp_put_mst_branch_device(mstb);
	}
}

static bool drm_dp_validate_guid(struct drm_dp_mst_topology_mgr *mgr,
				 u8 *guid)
{
	static u8 zero_guid[16];

	if (!memcmp(guid, zero_guid, 16)) {
		u64 salt = get_jiffies_64();
		memcpy(&guid[0], &salt, sizeof(u64));
		memcpy(&guid[8], &salt, sizeof(u64));
		return false;
	}
	return true;
}

#if 0
static int build_dpcd_read(struct drm_dp_sideband_msg_tx *msg, u8 port_num, u32 offset, u8 num_bytes)
{
	struct drm_dp_sideband_msg_req_body req;

	req.req_type = DP_REMOTE_DPCD_READ;
	req.u.dpcd_read.port_number = port_num;
	req.u.dpcd_read.dpcd_address = offset;
	req.u.dpcd_read.num_bytes = num_bytes;
	drm_dp_encode_sideband_req(&req, msg);

	return 0;
}
#endif

static int drm_dp_send_sideband_msg(struct drm_dp_mst_topology_mgr *mgr,
				    bool up, u8 *msg, int len)
{
	int ret;
	int regbase = up ? DP_SIDEBAND_MSG_UP_REP_BASE : DP_SIDEBAND_MSG_DOWN_REQ_BASE;
	int tosend, total, offset;
	int retries = 0;

retry:
	total = len;
	offset = 0;
	do {
		tosend = min3(mgr->max_dpcd_transaction_bytes, 16, total);

		ret = drm_dp_dpcd_write(mgr->aux, regbase + offset,
					&msg[offset],
					tosend);
		if (ret != tosend) {
			if (ret == -EIO && retries < 5) {
				retries++;
				goto retry;
			}
			DRM_DEBUG_KMS("failed to dpcd write %d %d\n", tosend, ret);

			return -EIO;
		}
		offset += tosend;
		total -= tosend;
	} while (total > 0);
	return 0;
}

static int set_hdr_from_dst_qlock(struct drm_dp_sideband_msg_hdr *hdr,
				  struct drm_dp_sideband_msg_tx *txmsg)
{
	struct drm_dp_mst_branch *mstb = txmsg->dst;
	u8 req_type;

	/* both msg slots are full */
	if (txmsg->seqno == -1) {
		if (mstb->tx_slots[0] && mstb->tx_slots[1]) {
			DRM_DEBUG_KMS("%s: failed to find slot\n", __func__);
			return -EAGAIN;
		}
		if (mstb->tx_slots[0] == NULL && mstb->tx_slots[1] == NULL) {
			txmsg->seqno = mstb->last_seqno;
			mstb->last_seqno ^= 1;
		} else if (mstb->tx_slots[0] == NULL)
			txmsg->seqno = 0;
		else
			txmsg->seqno = 1;
		mstb->tx_slots[txmsg->seqno] = txmsg;
	}

	req_type = txmsg->msg[0] & 0x7f;
	if (req_type == DP_CONNECTION_STATUS_NOTIFY ||
		req_type == DP_RESOURCE_STATUS_NOTIFY)
		hdr->broadcast = 1;
	else
		hdr->broadcast = 0;
	hdr->path_msg = txmsg->path_msg;
	hdr->lct = mstb->lct;
	hdr->lcr = mstb->lct - 1;
	if (mstb->lct > 1)
		memcpy(hdr->rad, mstb->rad, mstb->lct / 2);
	hdr->seqno = txmsg->seqno;
	return 0;
}
/*
 * process a single block of the next message in the sideband queue
 */
static int process_single_tx_qlock(struct drm_dp_mst_topology_mgr *mgr,
				   struct drm_dp_sideband_msg_tx *txmsg,
				   bool up)
{
	u8 chunk[48];
	struct drm_dp_sideband_msg_hdr hdr;
	int len, space, idx, tosend;
	int ret;

	memset(&hdr, 0, sizeof(struct drm_dp_sideband_msg_hdr));

	if (txmsg->state == DRM_DP_SIDEBAND_TX_QUEUED) {
		txmsg->seqno = -1;
		txmsg->state = DRM_DP_SIDEBAND_TX_START_SEND;
	}

	/* make hdr from dst mst - for replies use seqno
	   otherwise assign one */
	ret = set_hdr_from_dst_qlock(&hdr, txmsg);
	if (ret < 0)
		return ret;

	/* amount left to send in this message */
	len = txmsg->cur_len - txmsg->cur_offset;

	/* 48 - sideband msg size - 1 byte for data CRC, x header bytes */
	space = 48 - 1 - drm_dp_calc_sb_hdr_size(&hdr);

	tosend = min(len, space);
	if (len == txmsg->cur_len)
		hdr.somt = 1;
	if (space >= len)
		hdr.eomt = 1;


	hdr.msg_len = tosend + 1;
	drm_dp_encode_sideband_msg_hdr(&hdr, chunk, &idx);
	memcpy(&chunk[idx], &txmsg->msg[txmsg->cur_offset], tosend);
	/* add crc at end */
	drm_dp_crc_sideband_chunk_req(&chunk[idx], tosend);
	idx += tosend + 1;

	ret = drm_dp_send_sideband_msg(mgr, up, chunk, idx);
	if (ret) {
		DRM_DEBUG_KMS("sideband msg failed to send\n");
		return ret;
	}

	txmsg->cur_offset += tosend;
	if (txmsg->cur_offset == txmsg->cur_len) {
		txmsg->state = DRM_DP_SIDEBAND_TX_SENT;
		return 1;
	}
	return 0;
}

static void process_single_down_tx_qlock(struct drm_dp_mst_topology_mgr *mgr)
{
	struct drm_dp_sideband_msg_tx *txmsg;
	int ret;

	WARN_ON(!mutex_is_locked(&mgr->qlock));

	/* construct a chunk from the first msg in the tx_msg queue */
	if (list_empty(&mgr->tx_msg_downq)) {
		mgr->tx_down_in_progress = false;
		return;
	}
	mgr->tx_down_in_progress = true;

	txmsg = list_first_entry(&mgr->tx_msg_downq, struct drm_dp_sideband_msg_tx, next);
	ret = process_single_tx_qlock(mgr, txmsg, false);
	if (ret == 1) {
		/* txmsg is sent it should be in the slots now */
		list_del(&txmsg->next);
	} else if (ret) {
		DRM_DEBUG_KMS("failed to send msg in q %d\n", ret);
		list_del(&txmsg->next);
		if (txmsg->seqno != -1)
			txmsg->dst->tx_slots[txmsg->seqno] = NULL;
		txmsg->state = DRM_DP_SIDEBAND_TX_TIMEOUT;
		wake_up(&mgr->tx_waitq);
	}
	if (list_empty(&mgr->tx_msg_downq)) {
		mgr->tx_down_in_progress = false;
		return;
	}
}

/* called holding qlock */
static void process_single_up_tx_qlock(struct drm_dp_mst_topology_mgr *mgr,
				       struct drm_dp_sideband_msg_tx *txmsg)
{
	int ret;

	/* construct a chunk from the first msg in the tx_msg queue */
	ret = process_single_tx_qlock(mgr, txmsg, true);

	if (ret != 1)
		DRM_DEBUG_KMS("failed to send msg in q %d\n", ret);

	if (txmsg->seqno != -1) {
		WARN_ON((unsigned int)txmsg->seqno >
			ARRAY_SIZE(txmsg->dst->tx_slots));
		txmsg->dst->tx_slots[txmsg->seqno] = NULL;
	}
}

static void drm_dp_queue_down_tx(struct drm_dp_mst_topology_mgr *mgr,
				 struct drm_dp_sideband_msg_tx *txmsg)
{
	mutex_lock(&mgr->qlock);
	list_add_tail(&txmsg->next, &mgr->tx_msg_downq);
	if (!mgr->tx_down_in_progress)
		process_single_down_tx_qlock(mgr);
	mutex_unlock(&mgr->qlock);
}

static void drm_dp_send_link_address(struct drm_dp_mst_topology_mgr *mgr,
				     struct drm_dp_mst_branch *mstb)
{
	int len;
	struct drm_dp_sideband_msg_tx *txmsg;
	int ret;

	txmsg = kzalloc(sizeof(*txmsg), GFP_KERNEL);
	if (!txmsg)
		return;

	txmsg->dst = mstb;
	len = build_link_address(txmsg);

	mstb->link_address_sent = true;
	drm_dp_queue_down_tx(mgr, txmsg);

	ret = drm_dp_mst_wait_tx_reply(mstb, txmsg);
	if (ret > 0) {
		int i;

		if (txmsg->reply.reply_type == 1)
			DRM_DEBUG_KMS("link address nak received\n");
		else {
			DRM_DEBUG_KMS("link address reply: %d\n", txmsg->reply.u.l