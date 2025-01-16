d, &raw->msg[1], 16);
		msg->u.nak.reason = raw->msg[17];
		msg->u.nak.nak_data = raw->msg[18];
		return false;
	}

	switch (msg->req_type) {
	case DP_LINK_ADDRESS:
		return drm_dp_sideband_parse_link_address(raw, msg);
	case DP_QUERY_PAYLOAD:
		return drm_dp_sideband_parse_query_payload_ack(raw, msg);
	case DP_REMOTE_DPCD_READ:
		return drm_dp_sideband_parse_remote_dpcd_read(raw, msg);
	case DP_REMOTE_DPCD_WRITE:
		return drm_dp_sideband_parse_remote_dpcd_write(raw, msg);
	case DP_REMOTE_I2C_READ:
		return drm_dp_sideband_parse_remote_i2c_read_ack(raw, msg);
	case DP_ENUM_PATH_RESOURCES:
		return drm_dp_sideband_parse_enum_path_resources_ack(raw, msg);
	case DP_ALLOCATE_PAYLOAD:
		return drm_dp_sideband_parse_allocate_payload_ack(raw, msg);
	default:
		DRM_ERROR("Got unknown reply 0x%02x\n", msg->req_type);
		return false;
	}
}

static bool drm_dp_sideband_parse_connection_status_notify(struct drm_dp_sideband_msg_rx *raw,
							   struct drm_dp_sideband_msg_req_body *msg)
{
	int idx = 1;

	msg->u.conn_stat.port_number = (raw->msg[idx] & 0xf0) >> 4;
	idx++;
	if (idx > raw->curlen)
		goto fail_len;

	memcpy(msg->u.conn_stat.guid, &raw->msg[idx], 16);
	idx += 16;
	if (idx > raw->curlen)
		goto fail_len;

	msg->u.conn_stat.legacy_device_plug_status = (raw->msg[idx] >> 6) & 0x1;
	msg->u.conn_stat.displayport_device_plug_status = (raw->msg[idx] >> 5) & 0x1;
	msg->u.conn_stat.message_capability_status = (raw->msg[idx] >> 4) & 0x1;
	msg->u.conn_stat.input_port = (raw->msg[idx] >> 3) & 0x1;
	msg->u.conn_stat.peer_device_type = (raw->msg[idx] & 0x7);
	idx++;
	return true;
fail_len:
	DRM_DEBUG_KMS("connection status reply parse length fail %d %d\n", idx, raw->curlen);
	return false;
}

static bool drm_dp_sideband_parse_resource_status_notify(struct drm_dp_sideband_msg_rx *raw,
							   struct drm_dp_sideband_msg_req_body *msg)
{
	int idx = 1;

	msg->u.resource_stat.port_number = (raw->msg[idx] & 0xf0) >> 4;
	idx++;
	if (idx > raw->curlen)
		goto fail_len;

	memcpy(msg->u.resource_stat.guid, &raw->msg[idx], 16);
	idx += 16;
	if (idx > raw->curlen)
		goto fail_len;

	msg->u.resource_stat.available_pbn = (raw->msg[idx] << 8) | (raw->msg[idx + 1]);
	idx++;
	return true;
fail_len:
	DRM_DEBUG_KMS("resource status reply parse length fail %d %d\n", idx, raw->curlen);
	return false;
}

static bool drm_dp_sideband_parse_req(struct drm_dp_sideband_msg_rx *raw,
				      struct drm_dp_sideband_msg_req_body *msg)
{
	memset(msg, 0, sizeof(*msg));
	msg->req_type = (raw->msg[0] & 0x7f);

	switch (msg->req_type) {
	case DP_CONNECTION_STATUS_NOTIFY:
		return drm_dp_sideband_parse_connection_status_notify(raw, msg);
	case DP_RESOURCE_STATUS_NOTIFY:
		return drm_dp_sideband_parse_resource_status_notify(raw, msg);
	default:
		DRM_ERROR("Got unknown request 0x%02x\n", msg->req_type);
		return false;
	}
}

static int build_dpcd_write(struct drm_dp_sideband_msg_tx *msg, u8 port_num, u32 offset, u8 num_bytes, u8 *bytes)
{
	struct drm_dp_sideband_msg_req_body req;

	req.req_type = DP_REMOTE_DPCD_WRITE;
	req.u.dpcd_write.port_number = port_num;
	req.u.dpcd_write.dpcd_address = offset;
	req.u.dpcd_write.num_bytes = num_bytes;
	req.u.dpcd_write.bytes = bytes;
	drm_dp_encode_sideband_req(&req, msg);

	return 0;
}

static int build_link_address(struct drm_dp_sideband_msg_tx *m