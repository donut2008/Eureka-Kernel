q_payload.num_slots = 0;
		}

		if (mgr->payloads[i].start_slot != req_payload.start_slot) {
			mgr->payloads[i].start_slot = req_payload.start_slot;
		}
		/* work out what is required to happen with this payload */
		if (mgr->payloads[i].num_slots != req_payload.num_slots) {

			/* need to push an update for this payload */
			if (req_payload.num_slots) {
				drm_dp_create_payload_step1(mgr, mgr->proposed_vcpis[i]->vcpi, &req_payload);
				mgr->payloads[i].num_slots = req_payload.num_slots;
				mgr->payloads[i].vcpi = req_payload.vcpi;
			} else if (mgr->payloads[i].num_slots) {
				mgr->payloads[i].num_slots = 0;
				drm_dp_destroy_payload_step1(mgr, port, mgr->payloads[i].vcpi, &mgr->payloads[i]);
				req_payload.payload_state = mgr->payloads[i].payload_state;
				mgr->payloads[i].start_slot = 0;
			}
			mgr->payloads[i].payload_state = req_payload.payload_state;
		}
		cur_slots += req_payload.num_slots;

		if (port)
			drm_dp_put_port(port);
	}

	for (i = 0; i < mgr->max_payloads; i++) {
		if (mgr->payloads[i].payload_state == DP_PAYLOAD_DELETE_LOCAL) {
			DRM_DEBUG_KMS("removing payload %d\n", i);
			for (j = i; j < mgr->max_payloads - 1; j++) {
				memcpy(&mgr->payloads[j], &mgr->payloads[j + 1], sizeof(struct drm_dp_payload));
				mgr->proposed_vcpis[j] = mgr->proposed_vcpis[j + 1];
				if (mgr->proposed_vcpis[j] && mgr->proposed_vcpis[j]->num_slots) {
					set_bit(j + 1, &mgr->payload_mask);
				} else {
					clear_bit(j + 1, &mgr->payload_mask);
				}
			}
			memset(&mgr->payloads[mgr->max_payloads - 1], 0, sizeof(struct drm_dp_payload));
			mgr->proposed_vcpis[mgr->max_payloads - 1] = NULL;
			clear_bit(mgr->max_payloads, &mgr->payload_mask);

		}
	}
	mutex_unlock(&mgr->payload_lock);

	return 0;
}
EXPORT_SYMBOL(drm_dp_update_payload_part1);

/**
 * drm_dp_update_payload_part2() - Execute payload update part 2
 * @mgr: manager to use.
 *
 * This iterates over all proposed virtual channels, and tries to
 * allocate space in the link for them. For 0->slots transitions,
 * this step writes the remote VC payload commands. For slots->0
 * this just resets some internal state.
 */
int drm_dp_update_payload_part2(struct drm_dp_mst_topology_mgr *mgr)
{
	struct drm_dp_mst_port *port;
	int i;
	int ret = 0;
	mutex_lock(&mgr->payload_lock);
	for (i = 0; i < mgr->max_payloads; i++) {

		if (!mgr->proposed_vcpis[i])
			continue;

		port = container_of(mgr->proposed_vcpis[i], struct drm_dp_mst_port, vcpi);

		DRM_DEBUG_KMS("payload %d %d\n", i, mgr->payloads[i].payload_state);
		if (mgr->payloads[i].payload_state == DP_PAYLOAD_LOCAL) {
			ret = drm_dp_create_payload_step2(mgr, port, mgr->proposed_vcpis[i]->vcpi, &mgr->payloads[i]);
		} else if (mgr->payloads[i].payload_state == DP_PAYLOAD_DELETE_LOCAL) {
			ret = drm_dp_destroy_payload_step2(mgr, mgr->proposed_vcpis[i]->vcpi, &mgr->payloads[i]);
		}
		if (ret) {
			mutex_unlock(&mgr->payload_lock);
			return ret;
		}
	}
	mutex_unlock(&mgr->payload_lock);
	return 0;
}
EXPORT_SYMBOL(drm_dp_update_payload_part2);

#if 0 /* unused as of yet */
static int drm_dp_send_dpcd_read(struct drm_dp_mst_topology_mgr *mgr,
				 struct drm_dp_mst_port *port,
				 int offset, int size)
{
	int len;
	struct drm_dp_sideband_msg_tx *txmsg;

	txmsg = kzalloc(sizeof(*txmsg), GFP_KERNEL);
	if (!txmsg)
		return -ENOMEM;

	len = build_dpcd_read(txmsg, port->port_num, 0, 8);
	txmsg->dst = port->parent;

	drm_dp_queue_down_tx(mgr, txmsg);

	return 0;
}
#endif

static int drm_dp_send_dpcd_write(struct drm_dp_mst_topology_mgr *mgr,
				  struct drm_dp_mst_port *port,
				  int offset, int size, u8 *bytes)
{
	int len;
	int ret;
	struct drm_dp_sideband_msg_tx *txmsg;
	struct drm_dp_mst_branch *mstb;

	mstb = drm_dp_get_validated_mstb_ref(mgr, port->parent);
	if (!mstb)
		return -EINVAL;

	txmsg = kzalloc(sizeof(*txmsg), GFP_KERNEL);
	if (!txmsg) {
		ret = -ENOMEM;
		goto fail_put;
	}

	len = build_dpcd_write(txmsg, port->port_num, offset, size, bytes);
	txmsg->dst = mstb;

	drm_dp_queue_down_tx(mgr, txmsg);

	ret = drm_dp_mst_wait_tx_reply(mstb, txmsg);
	if (ret > 0) {
		if (txmsg->reply.reply_type == 1) {
			ret = -EINVAL;
		} else
			ret = 0;
	}
	kfree(txmsg);
fail_put:
	drm_dp_put_mst_branch_device(mstb);
	return ret;
}

static int drm_dp_encode_up_ack_reply(struct drm_dp_sideband_msg_tx *msg, u8 req_type)
{
	struct drm_dp_sideband_msg_reply_body reply;

	reply.reply_type = 1;
	reply.req_type = req_type;
	drm_dp_encode_sideband_reply(&reply, msg);
	return 0;
}

static int drm_dp_send_up_ack_reply(struct drm_dp_mst_topology_mgr *mgr,
				    struct drm_dp_mst_branch *mstb,
				    int req_type, int seqno, bool broadcast)
{
	struct drm_dp_sideband_msg_tx *txmsg;

	txmsg = kzalloc(sizeof(*txmsg), GFP_KERNEL);
	if (!txmsg)
		return -ENOMEM;

	txmsg->dst = mstb;
	txmsg->seqno = seqno;
	drm_dp_encode_up_ack_reply(txmsg, req_type);

	mutex_lock(&mgr->qlock);

	process_single_up_tx_qlock(mgr, txmsg);

	mutex_unlock(&mgr->qlock);

	kfree(txmsg);
	return 0;
}

static bool drm_dp_get_vc_payload_bw(int dp_link_bw,
				     int dp_link_count,
				     int *out)
{
	switch (dp_link_bw) {
	default:
		DRM_DEBUG_KMS("invalid link bandwidth in DPCD: %x (link count: %d)\n",
			      dp_link_bw, dp_link_count);
		return false;

	case DP_LINK_BW_1_62:
		*out = 3 * dp_link_count;
		break;
	case DP_LINK_BW_2_7:
		*out = 5 * dp_link_count;
		break;
	case DP_LINK_BW_5_4:
		*out = 10 * dp_link_count;
		break;
	}
	return true;
}

/**
 * drm_dp_mst_topology_mgr_set_mst() - Set the MST state for a topology manager
 * @mgr: manager to set state for
 * @mst_state: true to enable MST on this connector - false to disable.
 *
 * This is called by the driver when it detects an MST capable device plugged
 * into a DP MST capable port, or when a DP MST capable device is unplugged.
 */
int drm_dp_mst_topology_mgr_set_mst(struct drm_dp_mst_topology_mgr *mgr, bool mst_state)
{
	int ret = 0;
	struct drm_dp_mst_branch *mstb = NULL;

	mutex_lock(&mgr->payload_lock);
	mutex_lock(&mgr->lock);
	if (mst_state == mgr->mst_state)
		goto out_unlock;

	mgr->mst_state = mst_state;
	/* set the device into MST mode */
	if (mst_state) {
		WARN_ON(mgr->mst_primary);

		/* get dpcd info */
		ret = drm_dp_dpcd_read(mgr->aux, DP_DPCD_REV, mgr->dpcd, DP_RECEIVER_CAP_SIZE);
		if (ret != DP_RECEIVER_CAP_SIZE) {
			DRM_DEBUG_KMS("failed to read DPCD\n");
			goto out_unlock;
		}

		if (!drm_dp_get_vc_payload_bw(mgr->dpcd[1],
					      mgr->dpcd[2] & DP_MAX_LANE_COUNT_MASK,
					      &mgr->pbn_div)) {
			ret = -EINVAL;
			goto out_unlock;
		}

		mgr->total_pbn = 2560;
		mgr->total_slots = DIV_ROUND_UP(mgr->total_pbn, mgr->pbn_div);
		mgr->avail_slots = mgr->total_slots;

		/* add initial branch device at LCT 1 */
		mstb = drm_dp_add_mst_branch_device(1, NULL);
		if (mstb == NULL) {
			ret = -ENOMEM;
			goto out_unlock;
		}
		mstb->mgr = mgr;

		/* give this the main reference */
		mgr->mst_primary = mstb;
		kref_get(&mgr->mst_primary->kref);

		ret = drm_dp_dpcd_writeb(mgr->aux, DP_MSTM_CTRL,
							 DP_MST_EN | DP_UP_REQ_EN | DP_UPSTREAM_IS_SRC);
		if (ret < 0) {
			goto out_unlock;
		}

		{
			struct drm_dp_payload reset_pay;
			reset_pay.start_slot = 0;
			reset_pay.num_slots = 0x3f;
			drm_dp_dpcd_write_payload(mgr, 0, &reset_pay);
		}

		queue_work(system_long_wq, &mgr->work);

		ret = 0;
	} else {
		/* disable MST on the device */
		mstb = mgr->mst_primary;
		mgr->mst_primary = NULL;
		/* this can fail if the device is gone */
		drm_dp_dpcd_writeb(mgr->aux, DP_MSTM_CTRL, 0);
		ret = 0;
		memset(mgr->payloads, 0,
		       mgr->max_payloads * sizeof(mgr->payloads[0]));
		memset(mgr->proposed_vcpis, 0,
		       mgr->max_payloads * sizeof(mgr->proposed_vcpis[0]));
		mgr->payload_mask = 0;
		set_bit(0, &mgr->payload_mask);
		mgr->vcpi_mask = 0;
	}

out_unlock:
	mutex_unlock(&mgr->lock);
	mutex_unlock(&mgr->payload_lock);
	if (mstb)
		drm_dp_put_mst_branch_device(mstb);
	return ret;

}
EXPORT_SYMBOL(drm_dp_mst_topology_mgr_set_mst);

/**
 * drm_dp_mst_topology_mgr_suspend() - suspend the MST manager
 * @mgr: manager to suspend
 *
 * This function tells the MST device that we can't handle UP messages
 * anymore. This should stop it from sending any since we are suspended.
 */
void drm_dp_mst_topology_mgr_suspend(struct drm_dp_mst_topology_mgr *mgr)
{
	mutex_lock(&mgr->lock);
	drm_dp_dpcd_writeb(mgr->aux, DP_MSTM_CTRL,
			   DP_MST_EN | DP_UPSTREAM_IS_SRC);
	mutex_unlock(&mgr->lock);
	flush_work(&mgr->work);
	flush_work(&mgr->destroy_connector_work);
}
EXPORT_SYMBOL(drm_dp_mst_topology_mgr_suspend);

/**
 * drm_dp_mst_topology_mgr_resume() - resume the MST manager
 * @mgr: manager to resume
 *
 * This will fetch DPCD and see if the device is still there,
 * if it is, it will rewrite the MSTM control bits, and return.
 *
 * if the device fails this returns -1, and the driver should do
 * a full MST reprobe, in case we were undocked.
 */
int drm_dp_mst_topology_mgr_resume(struct drm_dp_mst_topology_mgr *mgr)
{
	int ret = 0;

	mutex_lock(&mgr->lock);

	if (mgr->mst_primary) {
		int sret;
		u8 guid[16];

		sret = drm_dp_dpcd_read(mgr->aux, DP_DPCD_REV, mgr->dpcd, DP_RECEIVER_CAP_SIZE);
		if (sret != DP_RECEIVER_CAP_SIZE) {
			DRM_DEBUG_KMS("dpcd read failed - undocked during suspend?\n");
			ret = -1;
			goto out_unlock;
		}

		ret = drm_dp_dpcd_writeb(mgr->aux, DP_MSTM_CTRL,
					 DP_MST_EN | DP_UP_REQ_EN | DP_UPSTREAM_IS_SRC);
		if (ret < 0) {
			DRM_DEBUG_KMS("mst write failed - undocked during suspend?\n");
			ret = -1;
			goto out_unlock;
		}

		/* Some hubs forget their guids after they resume */
		sret = drm_dp_dpcd_read(mgr->aux, DP_GUID, guid, 16);
		if (sret != 16) {
			DRM_DEBUG_KMS("dpcd read failed - undocked during suspend?\n");
			ret = -1;
			goto out_unlock;
		}
		drm_dp_check_mstb_guid(mgr->mst_primary, guid);

		ret = 0;
	} else
		ret = -1;

out_unlock:
	mutex_unlock(&mgr->lock);
	return ret;
}
EXPORT_SYMBOL(drm_dp_mst_topology_mgr_resume);

static bool drm_dp_get_one_sb_msg(struct drm_dp_mst_topology_mgr *mgr, bool up)
{
	int len;
	u8 replyblock[32];
	int replylen, origlen, curreply;
	int ret;
	struct drm_dp_sideband_msg_rx *msg;
	int basereg = up ? DP_SIDEBAND_MSG_UP_REQ_BASE : DP_SIDEBAND_MSG_DOWN_REP_BASE;
	msg = up ? &mgr->up_req_recv : &mgr->down_rep_recv;

	len = min(mgr->max_dpcd_transaction_bytes, 16);
	ret = drm_dp_dpcd_read(mgr->aux, basereg,
			       replyblock, len);
	if (ret != len) {
		DRM_DEBUG_KMS("failed to read DPCD down rep %d %d\n", len, ret);
		return false;
	}
	ret = drm_dp_sideband_msg_build(msg, replyblock, len, true);
	if (!ret) {
		DRM_DEBUG_KMS("sideband msg build failed %d\n", replyblock[0]);
		return false;
	}
	replylen = msg->curchunk_len + msg->curchunk_hdrlen;

	origlen = replylen;
	replylen -= len;
	curreply = len;
	while (replylen > 0) {
		len = min3(replylen, mgr->max_dpcd_transaction_bytes, 16);
		ret = drm_dp_dpcd_read(mgr->aux, basereg + curreply,
				    replyblock, len);
		if (ret != len) {
			DRM_DEBUG_KMS("failed to read a chunk (len %d, ret %d)\n",
				      len, ret);
			return false;
		}

		ret = drm_dp_sideband_msg_build(msg, replyblock, len, false);
		if (!ret) {
			DRM_DEBUG_KMS("failed to build sideband msg\n");
			return false;
		}

		curreply += len;
		replylen -= len;
	}
	return true;
}

static int drm_dp_mst_handle_down_rep(struct drm_dp_mst_topology_mgr *mgr)
{
	int ret = 0;

	if (!drm_dp_get_one_sb_msg(mgr, false)) {
		memset(&mgr->down_rep_recv, 0,
		       sizeof(struct drm_dp_sideband_msg_rx));
		return 0;
	}

	if (mgr->down_rep_recv.have_eomt) {
		struct drm_dp_sideband_msg_tx *txmsg;
		struct drm_dp_mst_branch *mstb;
		int slot = -1;
		mstb = drm_dp_get_mst_branch_device(mgr,
						    mgr->down_rep_recv.initial_hdr.lct,
						    mgr->down_rep_recv.initial_hdr.rad);

		if (!mstb) {
			DRM_DEBUG_KMS("Got MST reply from unknown device %d\n", mgr->down_rep_recv.initial_hdr.lct);
			memset(&mgr->down_rep_recv, 0, sizeof(struct drm_dp_sideband_msg_rx));
			return 0;
		}

		/* find the message */
		slot = mgr->down_rep_recv.initial_hdr.seqno;
		mutex_lock(&mgr->qlock);
		txmsg = mstb->tx_slots[slot];
		/* remove from slots */
		mutex_unlock(&mgr->qlock);

		if (!txmsg) {
			DRM_DEBUG_KMS("Got MST reply with no msg %p %d %d %02x %02x\n",
			       mstb,
			       mgr->down_rep_recv.initial_hdr.seqno,
			       mgr->down_rep_recv.initial_hdr.lct,
				      mgr->down_rep_recv.initial_hdr.rad[0],
				      mgr->down_rep_recv.msg[0]);
			drm_dp_put_mst_branch_device(mstb);
			memset(&mgr->down_rep_recv, 0, sizeof(struct drm_dp_sideband_msg_rx));
			return 0;
		}

		drm_dp_sideband_parse_reply(&mgr->down_rep_recv, &txmsg->reply);
		if (txmsg->reply.reply_type == 1) {
			DRM_DEBUG_KMS("Got NAK reply: req 0x%02x, reason 0x%02x, nak data 0x%02x\n", txmsg->reply.req_type, txmsg->reply.u.nak.reason, txmsg->reply.u.nak.nak_data);
		}

		memset(&mgr->down_rep_recv, 0, sizeof(struct drm_dp_sideband_msg_rx));
		drm_dp_put_mst_branch_device(mstb);

		mutex_lock(&mgr->qlock);
		txmsg->state = DRM_DP_SIDEBAND_TX_RX;
		mstb->tx_slots[slot] = NULL;
		mutex_unlock(&mgr->qlock);

		wake_up(&mgr->tx_waitq);
	}
	return ret;
}

static int drm_dp_mst_handle_up_req(struct drm_dp_mst_topology_mgr *mgr)
{
	int ret = 0;

	if (!drm_dp_get_one_sb_msg(mgr, true)) {
		memset(&mgr->up_req_recv, 0,
		       sizeof(struct drm_dp_sideband_msg_rx));
		return 0;
	}

	if (mgr->up_req_recv.have_eomt) {
		struct drm_dp_sideband_msg_req_body msg;
		struct drm_dp_mst_branch *mstb = NULL;
		bool seqno;

		if (!mgr->up_req_recv.initial_hdr.broadcast) {
			mstb = drm_dp_get_mst_branch_device(mgr,
							    mgr->up_req_recv.initial_hdr.lct,
							    mgr->up_req_recv.initial_hdr.rad);
			if (!mstb) {
				DRM_DEBUG_KMS("Got MST reply from unknown device %d\n", mgr->up_req_recv.initial_hdr.lct);
				memset(&mgr->up_req_recv, 0, sizeof(struct drm_dp_sideband_msg_rx));
				return 0;
			}
		}

		seqno = mgr->up_req_recv.initial_hdr.seqno;
		drm_dp_sideband_parse_req(&mgr->up_req_recv, &msg);

		if (msg.req_type == DP_CONNECTION_STATUS_NOTIFY) {
			drm_dp_send_up_ack_reply(mgr, mgr->mst_primary, msg.req_type, seqno, false);

			if (!mstb)
				mstb = drm_dp_get_mst_branch_device_by_guid(mgr, msg.u.conn_stat.guid);

			if (!mstb) {
				DRM_DEBUG_KMS("Got MST reply from unknown device %d\n", mgr->up_req_recv.initial_hdr.lct);
				memset(&mgr->up_req_recv, 0, sizeof(struct drm_dp_sideband_msg_rx));
				return 0;
			}

			drm_dp_update_port(mstb, &msg.u.conn_stat);

			DRM_DEBUG_KMS("Got CSN: pn: %d ldps:%d ddps: %d mcs: %d ip: %d pdt: %d\n", msg.u.conn_stat.port_number, msg.u.conn_stat.legacy_device_plug_status, msg.u.conn_stat.displayport_device_plug_status, msg.u.conn_stat.message_capability_status, msg.u.conn_stat.input_port, msg.u.conn_stat.peer_device_type);
			(*mgr->cbs->hotplug)(mgr);

		} else if (msg.req_type == DP_RESOURCE_STATUS_NOTIFY) {
			drm_dp_send_up_ack_reply(mgr, mgr->mst_primary, msg.req_type, seqno, false);
			if (!mstb)
				mstb = drm_dp_get_mst_branch_device_by_guid(mgr, msg.u.resource_stat.guid);

			if (!mstb) {
				DRM_DEBUG_KMS("Got MST reply from unknown device %d\n", mgr->up_req_recv.initial_hdr.lct);
				memset(&mgr->up_req_recv, 0, sizeof(struct drm_dp_sideband_msg_rx));
				return 0;
			}

			DRM_DEBUG_KMS("Got RSN: pn: %d avail_pbn %d\n", msg.u.resource_stat.port_number, msg.u.resource_stat.available_pbn);
		}

		if (mstb)
			drm_dp_put_mst_branch_device(mstb);

		memset(&mgr->up_req_recv, 0, sizeof(struct drm_dp_sideband_msg_rx));
	}
	return ret;
}

/**
 * drm_dp_mst_hpd_irq() - MST hotplug IRQ notify
 * @mgr: manager to notify irq for.
 * @esi: 4 bytes from SINK_COUNT_ESI
 * @handled: whether the hpd interrupt was consumed or not
 *
 * This should be called from the driver when it detects a short IRQ,
 * along with the value of the DEVICE_SERVICE_IRQ_VECTOR_ESI0. The
 * topology manager will process the sideband messages received as a result
 * of this.
 */
int drm_dp_mst_hpd_irq(struct drm_dp_mst_topology_mgr *mgr, u8 *esi, bool *handled)
{
	int ret = 0;
	int sc;
	*handled = false;
	sc = esi[0] & 0x3f;

	if (sc != mgr->sink_count) {
		mgr->sink_count = sc;
		*handled = true;
	}

	if (esi[1] & DP_DOWN_REP_MSG_RDY) {
		ret = drm_dp_mst_handle_down_rep(mgr);
		*handled = true;
	}

	if (esi[1] & DP_UP_REQ_MSG_RDY) {
		ret |= drm_dp_mst_handle_up_req(mgr);
		*handled = true;
	}

	drm_dp_mst_kick_tx(mgr);
	return ret;
}
EXPORT_SYMBOL(drm_dp_mst_hpd_irq);

/**
 * drm_dp_mst_detect_port() - get connection status for an MST port
 * @mgr: manager for this port
 * @port: unverified pointer to a port
 *
 * This returns the current connection state for a port. It validates the
 * port pointer still exists so the caller doesn't require a reference
 */
enum drm_connector_status drm_dp_mst_detect_port(struct drm_connector *connector,
						 struct drm_dp_mst_topology_mgr *mgr, struct drm_dp_mst_port *port)
{
	enum drm_connector_status status = connector_status_disconnected;

	/* we need to search for the port in the mgr in case its gone */
	port = drm_dp_get_validated_port_ref(mgr, port);
	if (!port)
		return connector_status_disconnected;

	if (!port->ddps)
		goto out;

	switch (port->pdt) {
	case DP_PEER_DEVICE_NONE:
	case DP_PEER_DEVICE_MST_BRANCHING:
		break;

	case DP_PEER_DEVICE_SST_SINK:
		status = connector_status_connected;
		/* for logical ports - cache the EDID */
		if (port->port_num >= 8 && !port->cached_edid) {
			port->cached_edid = drm_get_edid(connector, &port->aux.ddc);
		}
		break;
	case DP_PEER_DEVICE_DP_LEGACY_CONV:
		if (port->ldps)
			status = connector_status_connected;
		break;
	}
out:
	drm_dp_put_port(port);
	return status;
}
EXPORT_SYMBOL(drm_dp_mst_detect_port);

/**
 * drm_dp_mst_get_edid() - get EDID for an MST port
 * @connector: toplevel connector to get EDID for
 * @mgr: manager for this port
 * @port: unverified pointer to a port.
 *
 * This returns an EDID for the port connected to a connector,
 * It validates the pointer still exists so the caller doesn't require a
 * reference.
 */
struct edid *drm_dp_mst_get_edid(struct drm_connector *connector, struct drm_dp_mst_topology_mgr *mgr, struct drm_dp_mst_port *port)
{
	struct edid *edid = NULL;

	/* we need to search for the port in the mgr in case its gone */
	port = drm_dp_get_validated_port_ref(mgr, port);
	if (!port)
		return NULL;

	if (port->cached_edid)
		edid = drm_edid_duplicate(port->cached_edid);
	else {
		edid = drm_get_edid(connector, &port->aux.ddc);
		drm_mode_connector_set_tile_property(connector);
	}
	drm_dp_put_port(port);
	return edid;
}
EXPORT_SYMBOL(drm_dp_mst_get_edid);

/**
 * drm_dp_find_vcpi_slots() - find slots for this PBN value
 * @mgr: manager to use
 * @pbn: payload bandwidth to convert into slots.
 */
int drm_dp_find_vcpi_slots(struct drm_dp_mst_topology_mgr *mgr,
			   int pbn)
{
	int num_slots;

	num_slots = DIV_ROUND_UP(pbn, mgr->pbn_div);

	if (num_slots > mgr->avail_slots)
		return -ENOSPC;
	return num_slots;
}
EXPORT_SYMBOL(drm_dp_find_vcpi_slots);

static int drm_dp_init_vcpi(struct drm_dp_mst_topology_mgr *mgr,
			    struct drm_dp_vcpi *vcpi, int pbn)
{
	int num_slots;
	int ret;

	num_slots = DIV_ROUND_UP(pbn, mgr->pbn_div);

	if (num_slots > mgr->avail_slots)
		return -ENOSPC;

	vcpi->pbn = pbn;
	vcpi->aligned_pbn = num_slots * mgr->pbn_div;
	vcpi->num_slots = num_slots;

	ret = drm_dp_mst_assign_payload_id(mgr, vcpi);
	if (ret < 0)
		return ret;
	return 0;
}

/**
 * drm_dp_mst_allocate_vcpi() - Allocate a virtual channel
 * @mgr: manager for this port
 * @port: port to allocate a virtual channel for.
 * @pbn: payload bandwidth number to request
 * @slots: returned number of slots for this PBN.
 */
bool drm_dp_mst_allocate_vcpi(struct drm_dp_mst_topology_mgr *mgr, struct drm_dp_mst_port *port, int pbn, int *slots)
{
	int ret;

	port = drm_dp_get_validated_port_ref(mgr, port);
	if (!port)
		return false;

	if (port->vcpi.vcpi > 0) {
		DRM_DEBUG_KMS("payload: vcpi %d already allocated for pbn %d - requested pbn %d\n", port->vcpi.vcpi, port->vcpi.pbn, pbn);
		if (pbn == port->vcpi.pbn) {
			*slots = port->vcpi.num_slots;
			drm_dp_put_port(port);
			return true;
		}
	}

	ret = drm_dp_init_vcpi(mgr, &port->vcpi, pbn);
	if (ret) {
		DRM_DEBUG_KMS("failed to init vcpi %d %d %d\n", DIV_ROUND_UP(pbn, mgr->pbn_div), mgr->avail_slots, ret);
		goto out;
	}
	DRM_DEBUG_KMS("initing vcpi for %d %d\n", pbn, port->vcpi.num_slots);
	*slots = port->vcpi.num_slots;

	drm_dp_put_port(port);
	return true;
out:
	return false;
}
EXPORT_SYMBOL(drm_dp_mst_allocate_vcpi);

int drm_dp_mst_get_vcpi_slots(struct drm_dp_mst_topology_mgr *mgr, struct drm_dp_mst_port *port)
{
	int slots = 0;
	port = drm_dp_get_validated_port_ref(mgr, port);
	if (!port)
		return slots;

	slots = port->vcpi.num_slots;
	drm_dp_put_port(port);
	return slots;
}
EXPORT_SYMBOL(drm_dp_mst_get_vcpi_slots);

/**
 * drm_dp_mst_reset_vcpi_slots() - Reset number of slots to 0 for VCPI
 * @mgr: manager for this port
 * @port: unverified pointer to a port.
 *
 * This just resets the number of slots for the ports VCPI for later programming.
 */
void drm_dp_mst_reset_vcpi_slots(struct drm_dp_mst_topology_mgr *mgr, struct drm_dp_mst_port *port)
{
	port = drm_dp_get_validated_port_ref(mgr, port);
	if (!port)
		return;
	port->vcpi.num_slots = 0;
	drm_dp_put_port(port);
}
EXPORT_SYMBOL(drm_dp_mst_reset_vcpi_slots);

/**
 * drm_dp_mst_deallocate_vcpi() - deallocate a VCPI
 * @mgr: manager for this port
 * @port: unverified port to deallocate vcpi for
 */
void drm_dp_mst_deallocate_vcpi(struct drm_dp_mst_topology_mgr *mgr, struct drm_dp_mst_port *port)
{
	port = drm_dp_get_validated_port_ref(mgr, port);
	if (!port)
		return;

	drm_dp_mst_put_payload_id(mgr, port->vcpi.vcpi);
	port->vcpi.num_slots = 0;
	port->vcpi.pbn = 0;
	port->vcpi.aligned_pbn = 0;
	port->vcpi.vcpi = 0;
	drm_dp_put_port(port);
}
EXPORT_SYMBOL(drm_dp_mst_deallocate_vcpi);

static int drm_dp_dpcd_write_payload(struct drm_dp_mst_topology_mgr *mgr,
				     int id, struct drm_dp_payload *payload)
{
	u8 payload_alloc[3], status;
	int ret;
	int retries = 0;

	drm_dp_dpcd_writeb(mgr->aux, DP_PAYLOAD_TABLE_UPDATE_STATUS,
			   DP_PAYLOAD_TABLE_UPDATED);

	payload_alloc[0] = id;
	payload_alloc[1] = payload->start_slot;
	payload_alloc[2] = payload->num_slots;

	ret = drm_dp_dpcd_write(mgr->aux, DP_PAYLOAD_ALLOCATE_SET, payload_alloc, 3);
	if (ret != 3) {
		DRM_DEBUG_KMS("failed to write payload allocation %d\n", ret);
		goto fail;
	}

retry:
	ret = drm_dp_dpcd_readb(mgr->aux, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
	if (ret < 0) {
		DRM_DEBUG_KMS("failed to read payload table status %d\n", ret);
		goto fail;
	}

	if (!(status & DP_PAYLOAD_TABLE_UPDATED)) {
		retries++;
		if (retries < 20) {
			usleep_range(10000, 20000);
			goto retry;
		}
		DRM_DEBUG_KMS("status not set after read payload table status %d\n", status);
		ret = -EINVAL;
		goto fail;
	}
	ret = 0;
fail:
	return ret;
}

static int do_get_act_status(struct drm_dp_aux *aux)
{
	int ret;
	u8 status;

	ret = drm_dp_dpcd_readb(aux, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
	if (ret < 0)
		return ret;

	return status;
}

/**
 * drm_dp_check_act_status() - Check ACT handled status.
 * @mgr: manager to use
 *
 * Check the payload status bits in the DPCD for ACT handled completion.
 */
int drm_dp_check_act_status(struct drm_dp_mst_topology_mgr *mgr)
{
	/*
	 * There doesn't seem to be any recommended retry count or timeout in
	 * the MST specification. Since some hubs have been observed to take
	 * over 1 second to update their payload allocations under certain
	 * conditions, we use a rather large timeout value.
	 */
	const int timeout_ms = 3000;
	int ret, status;

	ret = readx_poll_timeout(do_get_act_status, mgr->aux, status,
				 status & DP_PAYLOAD_ACT_HANDLED || status < 0,
				 200, timeout_ms * USEC_PER_MSEC);
	if (ret < 0 && status >= 0) {
		DRM_DEBUG_KMS("Failed to get ACT after %dms, last status: %02x\n",
			      timeout_ms, status);
		return -EINVAL;
	} else if (status < 0) {
		DRM_DEBUG_KMS("Failed to read payload table status: %d\n",
			      status);
		return status;
	}

	return 0;
}
EXPORT_SYMBOL(drm_dp_check_act_status);

/**
 * drm_dp_calc_pbn_mode() - Calculate the PBN for a mode.
 * @clock: dot clock for the mode
 * @bpp: bpp for the mode.
 *
 * This uses the formula in the spec to calculate the PBN value for a mode.
 */
int drm_dp_calc_pbn_mode(int clock, int bpp)
{
	u64 kbps;
	s64 peak_kbps;
	u32 numerator;
	u32 denominator;

	kbps = clock * bpp;

	/*
	 * margin 5300ppm + 300ppm ~ 0.6% as per spec, factor is 1.006
	 * The unit of 54/64Mbytes/sec is an arbitrary unit chosen based on
	 * common multiplier to render an integer PBN for all link rate/lane
	 * counts combinations
	 * calculate
	 * peak_kbps *= (1006/1000)
	 * peak_kbps *= (64/54)
	 * peak_kbps *= 8    convert to bytes
	 */

	numerator = 64 * 1006;
	denominator = 54 * 8 * 1000 * 1000;

	kbps *= numerator;
	peak_kbps = drm_fixp_from_fraction(kbps, denominator);

	return drm_fixp2int_ceil(peak_kbps);
}
EXPORT_SYMBOL(drm_dp_calc_pbn_mode);

static int test_calc_pbn_mode(void)
{
	int ret;
	ret = drm_dp_calc_pbn_mode(154000, 30);
	if (ret != 689) {
		DRM_ERROR("PBN calculation test failed - clock %d, bpp %d, expected PBN %d, actual PBN %d.\n",
				154000, 30, 689, ret);
		return -EINVAL;
	}
	ret = drm_dp_calc_pbn_mode(234000, 30);
	if (ret != 1047) {
		DRM_ERROR("PBN calculation test failed - clock %d, bpp %d, expected PBN %d, actual PBN %d.\n",
				234000, 30, 1047, ret);
		return -EINVAL;
	}
	ret = drm_dp_calc_pbn_mode(297000, 24);
	if (ret != 1063) {
		DRM_ERROR("PBN calculation test failed - clock %d, bpp %d, expected PBN %d, actual PBN %d.\n",
				297000, 24, 1063, ret);
		return -EINVAL;
	}
	return 0;
}

/* we want to kick the TX after we've ack the up/down IRQs. */
static void drm_dp_mst_kick_tx(struct drm_dp_mst_topology_mgr *mgr)
{
	queue_work(system_long_wq, &mgr->tx_work);
}

static void drm_dp_mst_dump_mstb(struct seq_file *m,
				 struct drm_dp_mst_branch *mstb)
{
	struct drm_dp_mst_port *port;
	int tabs = mstb->lct;
	char prefix[10];
	int i;

	for (i = 0; i < tabs; i++)
		prefix[i] = '\t';
	prefix[i] = '\0';

	seq_printf(m, "%smst: %p, %d\n", prefix, mstb, mstb->num_ports);
	list_for_each_entry(port, &mstb->ports, next) {
		seq_printf(m, "%sport: %d: ddps: %d ldps: %d, %p, conn: %p\n", prefix, port->port_num, port->ddps, port->ldps, port, port->connector);
		if (port->mstb)
			drm_dp_mst_dump_mstb(m, port->mstb);
	}
}

static bool dump_dp_payload_table(struct drm_dp_mst_topology_mgr *mgr,
				  char *buf)
{
	int ret;
	int i;
	for (i = 0; i < 4; i++) {
		ret = drm_dp_dpcd_read(mgr->aux, DP_PAYLOAD_TABLE_UPDATE_STATUS + (i * 16), &buf[i * 16], 16);
		if (ret != 16)
			break;
	}
	if (i == 4)
		return true;
	return false;
}

/**
 * drm_dp_mst_dump_topology(): dump topology to seq file.
 * @m: seq_file to dump output to
 * @mgr: manager to dump current topology for.
 *
 * helper to dump MST topology to a seq file for debugfs.
 */
void drm_dp_mst_dump_topology(struct seq_file *m,
			      struct drm_dp_mst_topology_mgr *mgr)
{
	int i;
	struct drm_dp_mst_port *port;
	mutex_lock(&mgr->lock);
	if (mgr->mst_primary)
		drm_dp_mst_dump_mstb(m, mgr->mst_primary);

	/* dump VCPIs */
	mutex_unlock(&mgr->lock);

	mutex_lock(&mgr->payload_lock);
	seq_printf(m, "vcpi: %lx %lx\n", mgr->payload_mask, mgr->vcpi_mask);

	for (i = 0; i < mgr->max_payloads; i++) {
		if (mgr->proposed_vcpis[i]) {
			port = container_of(mgr->proposed_vcpis[i], struct drm_dp_mst_port, vcpi);
			seq_printf(m, "vcpi %d: %d %d %d\n", i, port->port_num, port->vcpi.vcpi, port->vcpi.num_slots);
		} else
			seq_printf(m, "vcpi %d:unsed\n", i);
	}
	for (i = 0; i < mgr->max_payloads; i++) {
		seq_printf(m, "payload %d: %d, %d, %d\n",
			   i,
			   mgr->payloads[i].payload_state,
			   mgr->payloads[i].start_slot,
			   mgr->payloads[i].num_slots);


	}
	mutex_unlock(&mgr->payload_lock);

	mutex_lock(&mgr->lock);
	if (mgr->mst_primary) {
		u8 buf[64];
		bool bret;
		int ret;
		ret = drm_dp_dpcd_read(mgr->aux, DP_DPCD_REV, buf, DP_RECEIVER_CAP_SIZE);
		seq_printf(m, "dpcd: ");
		for (i = 0; i < DP_RECEIVER_CAP_SIZE; i++)
			seq_printf(m, "%02x ", buf[i]);
		seq_printf(m, "\n");
		ret = drm_dp_dpcd_read(mgr->aux, DP_FAUX_CAP, buf, 2);
		seq_printf(m, "faux/mst: ");
		for (i = 0; i < 2; i++)
			seq_printf(m, "%02x ", buf[i]);
		seq_printf(m, "\n");
		ret = drm_dp_dpcd_read(mgr->aux, DP_MSTM_CTRL, buf, 1);
		seq_printf(m, "mst ctrl: ");
		for (i = 0; i < 1; i++)
			seq_printf(m, "%02x ", buf[i]);
		seq_printf(m, "\n");

		/* dump the standard OUI branch header */
		ret = drm_dp_dpcd_read(mgr->aux, DP_BRANCH_OUI, buf, DP_BRANCH_OUI_HEADER_SIZE);
		seq_printf(m, "branch oui: ");
		for (i = 0; i < 0x3; i++)
			seq_printf(m, "%02x", buf[i]);
		seq_printf(m, " devid: ");
		for (i = 0x3; i < 0x8; i++)
			seq_printf(m, "%c", buf[i]);
		seq_printf(m, " revision: hw: %x.%x sw: %x.%x", buf[0x9] >> 4, buf[0x9] & 0xf, buf[0xa], buf[0xb]);
		seq_printf(m, "\n");
		bret = dump_dp_payload_table(mgr, buf);
		if (bret == true) {
			seq_printf(m, "payload table: ");
			for (i = 0; i < 63; i++)
				seq_printf(m, "%02x ", buf[i]);
			seq_printf(m, "\n");
		}

	}

	mutex_unlock(&mgr->lock);

}
EXPORT_SYMBOL(drm_dp_mst_dump_topology);

static void drm_dp_tx_work(struct work_struct *work)
{
	struct drm_dp_mst_topology_mgr *mgr = container_of(work, struct drm_dp_mst_topology_mgr, tx_work);

	mutex_lock(&mgr->qlock);
	if (mgr->tx_down_in_progress)
		process_single_down_tx_qlock(mgr);
	mutex_unlock(&mgr->qlock);
}

static void drm_dp_free_mst_port(struct kref *kref)
{
	struct drm_dp_mst_port *port = container_of(kref, struct drm_dp_mst_port, kref);
	kref_put(&port->parent->kref, drm_dp_free_mst_branch_device);
	kfree(port);
}

static void drm_dp_destroy_connector_work(struct work_struct *work)
{
	struct drm_dp_mst_topology_mgr *mgr = container_of(work, struct drm_dp_mst_topology_mgr, destroy_connector_work);
	struct drm_dp_mst_port *port;
	bool send_hotplug = false;
	/*
	 * Not a regular list traverse as we have to drop the destroy
	 * connector lock before destroying the connector, to avoid AB->BA
	 * ordering between this lock and the config mutex.
	 */
	for (;;) {
		mutex_lock(&mgr->destroy_connector_lock);
		port = list_first_entry_or_null(&mgr->destroy_connector_list, struct drm_dp_mst_port, next);
		if (!port) {
			mutex_unlock(&mgr->destroy_connector_lock);
			break;
		}
		list_del(&port->next);
		mutex_unlock(&mgr->destroy_connector_lock);

		kref_init(&port->kref);
		INIT_LIST_HEAD(&port->next);

		mgr->cbs->destroy_connector(mgr, port->connector);

		drm_dp_port_teardown_pdt(port, port->pdt);
		port->pdt = DP_PEER_DEVICE_NONE;

		if (!port->input && port->vcpi.vcpi > 0) {
			drm_dp_mst_reset_vcpi_slots(mgr, port);
			drm_dp_update_payload_part1(mgr);
			drm_dp_mst_put_payload_id(mgr, port->vcpi.vcpi);
		}

		kref_put(&port->kref, drm_dp_free_mst_port);
		send_hotplug = true;
	}
	if (send_hotplug)
		(*mgr->cbs->hotplug)(mgr);
}

/**
 * drm_dp_mst_topology_mgr_init - initialise a topology manager
 * @mgr: manager struct to initialise
 * @dev: device providing this structure - for i2c addition.
 * @aux: DP helper aux channel to talk to this device
 * @max_dpcd_transaction_bytes: hw specific DPCD transaction limit
 * @max_payloads: maximum number of payloads this GPU can source
 * @conn_base_id: the connector object ID the MST device is connected to.
 *
 * Return 0 for success, or negative error code on failure
 */
int drm_dp_mst_topology_mgr_init(struct drm_dp_mst_topology_mgr *mgr,
				 struct device *dev, struct drm_dp_aux *aux,
				 int max_dpcd_transaction_bytes,
				 int max_payloads, int conn_base_id)
{
	mutex_init(&mgr->lock);
	mutex_init(&mgr->qlock);
	mutex_init(&mgr->payload_lock);
	mutex_init(&mgr->destroy_connector_lock);
	INIT_LIST_HEAD(&mgr->tx_msg_downq);
	INIT_LIST_HEAD(&mgr->destroy_connector_list);
	INIT_WORK(&mgr->work, drm_dp_mst_link_probe_work);
	INIT_WORK(&mgr->tx_work, drm_dp_tx_work);
	INIT_WORK(&mgr->destroy_connector_work, drm_dp_destroy_connector_work);
	init_waitqueue_head(&mgr->tx_waitq);
	mgr->dev = dev;
	mgr->aux = aux;
	mgr->max_dpcd_transaction_bytes = max_dpcd_transaction_bytes;
	mgr->max_payloads = max_payloads;
	mgr->conn_base_id = conn_base_id;
	mgr->payloads = kcalloc(max_payloads, sizeof(struct drm_dp_payload), GFP_KERNEL);
	if (!mgr->payloads)
		return -ENOMEM;
	mgr->proposed_vcpis = kcalloc(max_payloads, sizeof(struct drm_dp_vcpi *), GFP_KERNEL);
	if (!mgr->proposed_vcpis)
		return -ENOMEM;
	set_bit(0, &mgr->payload_mask);
	test_calc_pbn_mode();
	return 0;
}
EXPORT_SYMBOL(drm_dp_mst_topology_mgr_init);

/**
 * drm_dp_mst_topology_mgr_destroy() - destroy topology manager.
 * @mgr: manager to destroy
 */
void drm_dp_mst_topology_mgr_destroy(struct drm_dp_mst_topology_mgr *mgr)
{
	flush_work(&mgr->work);
	flush_work(&mgr->destroy_connector_work);
	mutex_lock(&mgr->payload_lock);
	kfree(mgr->payloads);
	mgr->payloads = NULL;
	kfree(mgr->proposed_vcpis);
	mgr->proposed_vcpis = NULL;
	mutex_unlock(&mgr->payload_lock);
	mgr->dev = NULL;
	mgr->aux = NULL;
}
EXPORT_SYMBOL(drm_dp_mst_topology_mgr_destroy);

/* I2C device */
static int drm_dp_mst_i2c_xfer(struct i2c_adapter *adapter, struct i2c_msg *msgs,
			       int num)
{
	struct drm_dp_aux *aux = adapter->algo_data;
	struct drm_dp_mst_port *port = container_of(aux, struct drm_dp_mst_port, aux);
	struct drm_dp_mst_branch *mstb;
	struct drm_dp_mst_topology_mgr *mgr = port->mgr;
	unsigned int i;
	bool reading = false;
	struct drm_dp_sideband_msg_req_body msg;
	struct drm_dp_sideband_msg_tx *txmsg = NULL;
	int ret;

	mstb = drm_dp_get_validated_mstb_ref(mgr, port->parent);
	if (!mstb)
		return -EREMOTEIO;

	/* construct i2c msg */
	/* see if last msg is a read */
	if (msgs[num - 1].flags & I2C_M_RD)
		reading = true;

	if (!reading || (num - 1 > DP_REMOTE_I2C_READ_MAX_TRANSACTIONS)) {
		DRM_DEBUG_KMS("Unsupported I2C transaction for MST device\n");
		ret = -EIO;
		goto out;
	}

	memset(&msg, 0, sizeof(msg));
	msg.req_type = DP_REMOTE_I2C_READ;
	msg.u.i2c_read.num_transactions = num - 1;
	msg.u.i2c_read.port_number = port->port_num;
	for (i = 0; i < num - 1; i++) {
		msg.u.i2c_read.transactions[i].i2c_dev_id = msgs[i].addr;
		msg.u.i2c_read.transactions[i].num_bytes = msgs[i].len;
		msg.u.i2c_read.transactions[i].bytes = msgs[i].buf;
		msg.u.i2c_read.transactions[i].no_stop_bit = !(msgs[i].flags & I2C_M_STOP);
	}
	msg.u.i2c_read.read_i2c_device_id = msgs[num - 1].addr;
	msg.u.i2c_read.num_bytes_read = msgs[num - 1].len;

	txmsg = kzalloc(sizeof(*txmsg), GFP_KERNEL);
	if (!txmsg) {
		ret = -ENOMEM;
		goto out;
	}

	txmsg->dst = mstb;
	drm_dp_encode_sideband_req(&msg, txmsg);

	drm_dp_queue_down_tx(mgr, txmsg);

	ret = drm_dp_mst_wait_tx_reply(mstb, txmsg);
	if (ret > 0) {

		if (txmsg->reply.reply_type == 1) { /* got a NAK back */
			ret = -EREMOTEIO;
			goto out;
		}
		if (txmsg->reply.u.remote_i2c_read_ack.num_bytes != msgs[num - 1].len) {
			ret = -EIO;
			goto out;
		}
		memcpy(msgs[num - 1].buf, txmsg->reply.u.remote_i2c_read_ack.bytes, msgs[num - 1].len