 event->event);
		break;
	case RDMA_CM_EVENT_REJECTED:       /* FALLTHRU */
	case RDMA_CM_EVENT_UNREACHABLE:    /* FALLTHRU */
	case RDMA_CM_EVENT_CONNECT_ERROR:
		ret = isert_connect_error(cma_id);
		break;
	default:
		isert_err("Unhandled RDMA CMA event: %d\n", event->event);
		break;
	}

	return ret;
}

static int
isert_post_recvm(struct isert_conn *isert_conn, u32 count)
{
	struct ib_recv_wr *rx_wr, *rx_wr_failed;
	int i, ret;
	struct iser_rx_desc *rx_desc;

	for (rx_wr = isert_conn->rx_wr, i = 0; i < count; i++, rx_wr++) {
		rx_desc = &isert_conn->rx_descs[i];
		rx_wr->wr_id = (uintptr_t)rx_desc;
		rx_wr->sg_list = &rx_desc->rx_sg;
		rx_wr->num_sge = 1;
		rx_wr->next = rx_wr + 1;
	}
	rx_wr--;
	rx_wr->next = NULL; /* mark end of work requests list */

	ret = ib_post_recv(isert_conn->qp, isert_conn->rx_wr,
			   &rx_wr_failed);
	if (ret)
		isert_err("ib_post_recv() failed with ret: %d\n", ret);

	return ret;
}

static int
isert_post_recv(struct isert_conn *isert_conn, struct iser_rx_desc *rx_desc)
{
	struct ib_recv_wr *rx_wr_failed, rx_wr;
	int ret;

	rx_wr.wr_id = (uintptr_t)rx_desc;
	rx_wr.sg_list = &rx_desc->rx_sg;
	rx_wr.num_sge = 1;
	rx_wr.next = NULL;

	ret = ib_post_recv(isert_conn->qp, &rx_wr, &rx_wr_failed);
	if (ret)
		isert_err("ib_post_recv() failed with ret: %d\n", ret);

	return ret;
}

static int
isert_post_send(struct isert_conn *isert_conn, struct iser_tx_desc *tx_desc)
{
	struct ib_device *ib_dev = isert_conn->cm_id->device;
	struct ib_send_wr send_wr, *send_wr_failed;
	int ret;

	ib_dma_sync_single_for_device(ib_dev, tx_desc->dma_addr,
				      ISER_HEADERS_LEN, DMA_TO_DEVICE);

	send_wr.next	= NULL;
	send_wr.wr_id	= (uintptr_t)tx_desc;
	send_wr.sg_list	= tx_desc->tx_sg;
	send_wr.num_sge	= tx_desc->num_sge;
	send_wr.opcode	= IB_WR_SEND;
	send_wr.send_flags = IB_SEND_SIGNALED;

	ret = ib_post_send(isert_conn->qp, &send_wr, &send_wr_failed);
	if (ret)
		isert_err("ib_post_send() failed, ret: %d\n", ret);

	return ret;
}

static void
isert_create_send_desc(struct isert_conn *isert_conn,
		       struct isert_cmd *isert_cmd,
		       struct iser_tx_desc *tx_desc)
{
	struct isert_device *device = isert_conn->device;
	struct ib_device *ib_dev = device->ib_device;

	ib_dma_sync_single_for_cpu(ib_dev, tx_desc->dma_addr,
				   ISER_HEADERS_LEN, DMA_TO_DEVICE);

	memset(&tx_desc->iser_header, 0, sizeof(struct iser_hdr));
	tx_desc->iser_header.flags = ISER_VER;

	tx_desc->num_sge = 1;
	tx_desc->isert_cmd = isert_cmd;

	if (tx_desc->tx_sg[0].lkey != device->pd->local_dma_lkey) {
		tx_desc->tx_sg[0].lkey = device->pd->local_dma_lkey;
		isert_dbg("tx_desc %p lkey mismatch, fixing\n", tx_desc);
	}
}

static int
isert_init_tx_hdrs(struct isert_conn *isert_conn,
		   struct iser_tx_desc *tx_desc)
{
	struct isert_device *device = isert_conn->device;
	struct ib_device *ib_dev = device->ib_device;
	u64 dma_addr;

	dma_addr = ib_dma_map_single(ib_dev, (void *)tx_desc,
			ISER_HEADERS_LEN, DMA_TO_DEVICE);
	if (ib_dma_mapping_error(ib_dev, dma_addr)) {
		isert_err("ib_dma_mapping_error() failed\n");
		return -ENOMEM;
	}

	tx_desc->dma_addr = dma_addr;
	tx_desc->tx_sg[0].addr	= tx_desc->dma_addr;
	tx_desc->tx_sg[0].length = ISER_HEADERS_LEN;
	tx_desc->tx_sg[0].lkey = device->pd->local_dma_lkey;

	isert_dbg("Setup tx_sg[0].addr: 0x%llx length: %u lkey: 0x%x\n",
		  tx_desc->tx_sg[0].addr, tx_desc->tx_sg[0].length,
		  tx_desc->tx_sg[0].lkey);

	return 0;
}

static void
isert_init_send_wr(struct isert_conn *isert_conn, struct isert_cmd *isert_cmd,
		   struct ib_send_wr *send_wr)
{
	struct iser_tx_desc *tx_desc = &isert_cmd->tx_desc;

	isert_cmd->rdma_wr.iser_ib_op = ISER_IB_SEND;
	send_wr->wr_id = (uintptr_t)&isert_cmd->tx_desc;
	send_wr->opcode = IB_WR_SEND;
	send_wr->sg_list = &tx_desc->tx_sg[0];
	send_wr->num_sge = isert_cmd->tx_desc.num_sge;
	send_wr->send_flags = IB_SEND_SIGNALED;
}

static int
isert_rdma_post_recvl(struct isert_conn *isert_conn)
{
	struct ib_recv_wr rx_wr, *rx_wr_fail;
	struct ib_sge sge;
	int ret;

	memset(&sge, 0, sizeof(struct ib_sge));
	sge.addr = isert_conn->login_req_dma;
	sge.length = ISER_RX_LOGIN_SIZE;
	sge.lkey = isert_conn->device->pd->local_dma_lkey;

	isert_dbg("Setup sge: addr: %llx length: %d 0x%08x\n",
		sge.addr, sge.length, sge.lkey);

	memset(&rx_wr, 0, sizeof(struct ib_recv_wr));
	rx_wr.wr_id = (uintptr_t)isert_conn->login_req_buf;
	rx_wr.sg_list = &sge;
	rx_wr.num_sge = 1;

	ret = ib_post_recv(isert_conn->qp, &rx_wr, &rx_wr_fail);
	if (ret)
		isert_err("ib_post_recv() failed: %d\n", ret);

	return ret;
}

static int
isert_put_login_tx(struct iscsi_conn *conn, struct iscsi_login *login,
		   u32 length)
{
	struct isert_conn *isert_conn = conn->context;
	struct isert_device *device = isert_conn->device;
	struct ib_device *ib_dev = device->ib_device;
	struct iser_tx_desc *tx_desc = &isert_conn->login_tx_desc;
	int ret;

	isert_create_send_desc(isert_conn, NULL, tx_desc);

	memcpy(&tx_desc->iscsi_header, &login->rsp[0],
	       sizeof(struct iscsi_hdr));

	isert_init_tx_hdrs(isert_conn, tx_desc);

	if (length > 0) {
		struct ib_sge *tx_dsg = &tx_desc->tx_sg[1];

		ib_dma_sync_single_for_cpu(ib_dev, isert_conn->login_rsp_dma,
					   length, DMA_TO_DEVICE);

		memcpy(isert_conn->login_rsp_buf, login->rsp_buf, length);

		ib_dma_sync_single_for_device(ib_dev, isert_conn->login_rsp_dma,
					      length, DMA_TO_DEVICE);

		tx_dsg->addr	= isert_conn->login_rsp_dma;
		tx_dsg->length	= length;
		tx_dsg->lkey	= isert_conn->device->pd->local_dma_lkey;
		tx_desc->num_sge = 2;
	}
	if (!login->login_failed) {
		if (login->login_complete) {
			if (!conn->sess->sess_ops->SessionType &&
			    isert_conn->device->use_fastreg) {
				ret = isert_conn_create_fastreg_pool(isert_conn);
				if (ret) {
					isert_err("Conn: %p failed to create"
					       " fastreg pool\n", isert_conn);
					return ret;
				}
			}

			ret = isert_alloc_rx_descriptors(isert_conn);
			if (ret)
				return ret;

			ret = isert_post_recvm(isert_conn,
					       ISERT_QP_MAX_RECV_DTOS);
			if (ret)
				return ret;

			/* Now we are in FULL_FEATURE phase */
			mutex_lock(&isert_conn->mutex);
			isert_conn->state = ISER_CONN_FULL_FEATURE;
			mutex_unlock(&isert_conn->mutex);
			goto post_send;
		}

		ret = isert_rdma_post_recvl(isert_conn);
		if (ret)
			return ret;
	}
post_send:
	ret = isert_post_send(isert_conn, tx_desc);
	if (ret)
		return ret;

	return 0;
}

static void
isert_rx_login_req(struct isert_conn *isert_conn)
{
	struct iser_rx_desc *rx_desc = (void *)isert_conn->login_req_buf;
	int rx_buflen = isert_conn->login_req_len;
	struct iscsi_conn *conn = isert_conn->conn;
	struct iscsi_login *login = conn->conn_login;
	int size;

	isert_info("conn %p\n", isert_conn);

	WARN_ON_ONCE(!login);

	if (login->first_request) {
		struct iscsi_login_req *login_req =
			(struct iscsi_login_req *)&rx_desc->iscsi_header;
		/*
		 * Setup the initial iscsi_login values from the leading
		 * login request PDU.
		 */
		login->leading_connection = (!login_req->tsih) ? 1 : 0;
		login->current_stage =
			(login_req->flags & ISCSI_FLAG_LOGIN_CURRENT_STAGE_MASK)
			 >> 2;
		login->version_min	= login_req->min_version;
		login->version_max	= login_req->max_version;
		memcpy(login->isid, login_req->isid, 6);
		login->cmd_sn		= be32_to_cpu(login_req->cmdsn);
		login->init_task_tag	= login_req->itt;
		login->initial_exp_statsn = be32_to_cpu(login_req->exp_statsn);
		login->cid		= be16_to_cpu(login_req->cid);
		login->tsih		= be16_to_cpu(login_req->tsih);
	}

	memcpy(&login->req[0], (void *)&rx_desc->iscsi_header, ISCSI_HDR_LEN);

	size = min(rx_buflen, MAX_KEY_VALUE_PAIRS);
	isert_dbg("Using login payload size: %d, rx_buflen: %d "
		  "MAX_KEY_VALUE_PAIRS: %d\n", size, rx_buflen,
		  MAX_KEY_VALUE_PAIRS);
	memcpy(login->req_buf, &rx_desc->data[0], size);

	if (login->first_request) {
		complete(&isert_conn->login_comp);
		return;
	}
	schedule_delayed_work(&conn->login_work, 0);
}

static struct iscsi_cmd
*isert_allocate_cmd(struct iscsi_conn *conn, struct iser_rx_desc *rx_desc)
{
	struct isert_conn *isert_conn = conn->context;
	struct isert_cmd *isert_cmd;
	struct iscsi_cmd *cmd;

	cmd = iscsit_allocate_cmd(conn, TASK_INTERRUPTIBLE);
	if (!cmd) {
		isert_err("Unable to allocate iscsi_cmd + isert_cmd\n");
		return NULL;
	}
	isert_cmd = iscsit_priv_cmd(cmd);
	isert_cmd->conn = isert_conn;
	isert_cmd->iscsi_cmd = cmd;
	isert_cmd->rx_desc = rx_desc;

	return cmd;
}

static int
isert_handle_scsi_cmd(struct isert_conn *isert_conn,
		      struct isert_cmd *isert_cmd, struct iscsi_cmd *cmd,
		      struct iser_rx_desc *rx_desc, unsigned char *buf)
{
	struct iscsi_conn *conn = isert_conn->conn;
	struct iscsi_scsi_req *hdr = (struct iscsi_scsi_req *)buf;
	int imm_data, imm_data_len, unsol_data, sg_nents, rc;
	bool dump_payload = false;
	unsigned int data_len;

	rc = iscsit_setup_scsi_cmd(conn, cmd, buf);
	if (rc < 0)
		return rc;

	imm_data = cmd->immediate_data;
	imm_data_len = cmd->first_burst_len;
	unsol_data = cmd->unsolicited_data;
	data_len = cmd->se_cmd.data_length;

	if (imm_data && imm_data_len == data_len)
		cmd->se_cmd.se_cmd_flags |= SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC;
	rc = iscsit_process_scsi_cmd(conn, cmd, hdr);
	if (rc < 0) {
		return 0;
	} else if (rc > 0) {
		dump_payload = true;
		goto sequence_cmd;
	}

	if (!imm_data)
		return 0;

	if (imm_data_len != data_len) {
		sg_nents = max(1UL, DIV_ROUND_UP(imm_data_len, PAGE_SIZE));
		sg_copy_from_buffer(cmd->se_cmd.t_data_sg, sg_nents,
				    &rx_desc->data[0], imm_data_len);
		isert_dbg("Copy Immediate sg_nents: %u imm_data_len: %d\n",
			  sg_nents, imm_data_len);
	} else {
		sg_init_table(&isert_cmd->sg, 1);
		cmd->se_cmd.t_data_sg = &isert_cmd->sg;
		cmd->se_cmd.t_data_nents = 1;
		sg_set_buf(&isert_cmd->sg, &rx_desc->data[0], imm_data_len);
		isert_dbg("Transfer Immediate imm_data_len: %d\n",
			  imm_data_len);
	}

	cmd->write_data_done += imm_data_len;

	if (cmd->write_data_done == cmd->se_cmd.data_length) {
		spin_lock_bh(&cmd->istate_lock);
		cmd->cmd_flags |= ICF_GOT_LAST_DATAOUT;
		cmd->i_state = ISTATE_RECEIVED_LAST_DATAOUT;
		spin_unlock_bh(&cmd->istate_lock);
	}

sequence_cmd:
	rc = iscsit_sequence_cmd(conn, cmd, buf, hdr->cmdsn);

	if (!rc && dump_payload == false && unsol_data)
		iscsit_set_unsoliticed_dataout(cmd);
	else if (dump_payload && imm_data)
		target_put_sess_cmd(&cmd->se_cmd);

	return 0;
}

static int
isert_handle_iscsi_dataout(struct isert_conn *isert_conn,
			   struct iser_rx_desc *rx_desc, unsigned char *buf)
{
	struct scatterlist *sg_start;
	struct iscsi_conn *conn = isert_conn->conn;
	struct iscsi_cmd *cmd = NULL;
	struct iscsi_data *hdr = (struct iscsi_data *)buf;
	u32 unsol_data_len = ntoh24(hdr->dlength);
	int rc, sg_nents, sg_off, page_off;

	rc = iscsit_check_dataout_hdr(conn, buf, &cmd);
	if (rc < 0)
		return rc;
	else if (!cmd)
		return 0;
	/*
	 * FIXME: Unexpected unsolicited_data out
	 */
	if (!cmd->unsolicited_data) {
		isert_err("Received unexpected solicited data payload\n");
		dump_stack();
		return -1;
	}

	isert_dbg("Unsolicited DataOut unsol_data_len: %u, "
		  "write_data_done: %u, data_length: %u\n",
		  unsol_data_len,  cmd->write_data_done,
		  cmd->se_cmd.data_length);

	sg_off = cmd->write_data_done / PAGE_SIZE;
	sg_start = &cmd->se_cmd.t_data_sg[sg_off];
	sg_nents = max(1UL, DIV_ROUND_UP(unsol_data_len, PAGE_SIZE));
	page_off = cmd->write_data_done % PAGE_SIZE;
	/*
	 * FIXME: Non page-aligned unsolicited_data out
	 */
	if (page_off) {
		isert_err("unexpected non-page aligned data payload\n");
		dump_stack();
		return -1;
	}
	isert_dbg("Copying DataOut: sg_start: %p, sg_off: %u "
		  "sg_nents: %u from %p %u\n", sg_start, sg_off,
		  sg_nents, &rx_desc->data[0], unsol_data_len);

	sg_copy_from_buffer(sg_start, sg_nents, &rx_desc->data[0],
			    unsol_data_len);

	rc = iscsit_check_dataout_payload(cmd, hdr, false);
	if (rc < 0)
		return rc;

	/*
	 * multiple data-outs on the same command can arrive -
	 * so post the buffer before hand
	 */
	rc = isert_post_recv(isert_conn, rx_desc);
	if (rc) {
		isert_err("ib_post_recv failed with %d\n", rc);
		return rc;
	}
	return 0;
}

static int
isert_handle_nop_out(struct isert_conn *isert_conn, struct isert_cmd *isert_cmd,
		     struct iscsi_cmd *cmd, struct iser_rx_desc *rx_desc,
		     unsigned char *buf)
{
	struct iscsi_conn *conn = isert_conn->conn;
	struct iscsi_nopout *hdr = (struct iscsi_nopout *)buf;
	int rc;

	rc = iscsit_setup_nop_out(conn, cmd, hdr);
	if (rc < 0)
		return rc;
	/*
	 * FIXME: Add support for NOPOUT payload using unsolicited RDMA payload
	 */

	return iscsit_process_nop_out(conn, cmd, hdr);
}

static int
isert_handle_text_cmd(struct isert_conn *isert_conn, struct isert_cmd *isert_cmd,
		      struct iscsi_cmd *cmd, struct iser_rx_desc *rx_desc,
		      struct iscsi_text *hdr)
{
	struct iscsi_conn *conn = isert_conn->conn;
	u32 payload_length = ntoh24(hdr->dlength);
	int rc;
	unsigned char *text_in = NULL;

	rc = iscsit_setup_text_cmd(conn, cmd, hdr);
	if (rc < 0)
		return rc;

	if (payload_length) {
		text_in = kzalloc(payload_length, GFP_KERNEL);
		if (!text_in) {
			isert_err("Unable to allocate text_in of payload_length: %u\n",
				  payload_length);
			return -ENOMEM;
		}
	}
	cmd->text_in_ptr = text_in;

	memcpy(cmd->text_in_ptr, &rx_desc->data[0], payload_length);

	return iscsit_process_text_cmd(conn, cmd, hdr);
}

static int
isert_rx_opcode(struct isert_conn *isert_conn, struct iser_rx_desc *rx_desc,
		uint32_t read_stag, uint64_t read_va,
		uint32_t write_stag, uint64_t write_va)
{
	struct iscsi_hdr *hdr = &rx_desc->iscsi_header;
	struct iscsi_conn *conn = isert_conn->conn;
	struct iscsi_cmd *cmd;
	struct isert_cmd *isert_cmd;
	int ret = -EINVAL;
	u8 opcode = (hdr->opcode & ISCSI_OPCODE_MASK);

	if (conn->sess->sess_ops->SessionType &&
	   (!(opcode & ISCSI_OP_TEXT) || !(opcode & ISCSI_OP_LOGOUT))) {
		isert_err("Got illegal opcode: 0x%02x in SessionType=Discovery,"
			  " ignoring\n", opcode);
		return 0;
	}

	switch (opcode) {
	case ISCSI_OP_SCSI_CMD:
		cmd = isert_allocate_cmd(conn, rx_desc);
		if (!cmd)
			break;

		isert_cmd = iscsit_priv_cmd(cmd);
		isert_cmd->read_stag = read_stag;
		isert_cmd->read_va = read_va;
		isert_cmd->write_stag = write_stag;
		isert_cmd->write_va = write_va;

		ret = isert_handle_scsi_cmd(isert_conn, isert_cmd, cmd,
					rx_desc, (unsigned char *)hdr);
		break;
	case ISCSI_OP_NOOP_OUT:
		cmd = isert_allocate_cmd(conn, rx_desc);
		if (!cmd)
			break;

		isert_cmd = iscsit_priv_cmd(cmd);
		ret = isert_handle_nop_out(isert_conn, isert_cmd, cmd,
					   rx_desc, (unsigned char *)hdr);
		break;
	case ISCSI_OP_SCSI_DATA_OUT:
		ret = isert_handle_iscsi_dataout(isert_conn, rx_desc,
						(unsigned char *)hdr);
		break;
	case ISCSI_OP_SCSI_TMFUNC:
		cmd = isert_allocate_cmd(conn, rx_desc);
		if (!cmd)
			break;

		ret = iscsit_handle_task_mgt_cmd(conn, cmd,
						(unsigned char *)hdr);
		break;
	case ISCSI_OP_LOGOUT:
		cmd = isert_allocate_cmd(conn, rx_desc);
		if (!cmd)
			break;

		ret = iscsit_handle_logout_cmd(conn, cmd, (unsigned char *)hdr);
		break;
	case ISCSI_OP_TEXT:
		if (be32_to_cpu(hdr->ttt) != 0xFFFFFFFF)
			cmd = iscsit_find_cmd_from_itt(conn, hdr->itt);
		else
			cmd = isert_allocate_cmd(conn, rx_desc);

		if (!cmd)
			break;

		isert_cmd = iscsit_priv_cmd(cmd);
		ret = isert_handle_text_cmd(isert_conn, isert_cmd, cmd,
					    rx_desc, (struct iscsi_text *)hdr);
		break;
	default:
		isert_err("Got unknown iSCSI OpCode: 0x%02x\n", opcode);
		dump_stack();
		break;
	}

	return ret;
}

static void
isert_rx_do_work(struct iser_rx_desc *rx_desc, struct isert_conn *isert_conn)
{
	struct iser_hdr *iser_hdr = &rx_desc->iser_header;
	uint64_t read_va = 0, write_va = 0;
	uint32_t read_stag = 0, write_stag = 0;

	switch (iser_hdr->flags & 0xF0) {
	case ISCSI_CTRL:
		if (iser_hdr->flags & ISER_RSV) {
			read_stag = be32_to_cpu(iser_hdr->read_stag);
			read_va = be64_to_cpu(iser_hdr->read_va);
			isert_dbg("ISER_RSV: read_stag: 0x%x read_va: 0x%llx\n",
				  read_stag, (unsigned long long)read_va);
		}
		if (iser_hdr->flags & ISER_WSV) {
			write_stag = be32_to_cpu(iser_hdr->write_stag);
			write_va = be64_to_cpu(iser_hdr->write_va);
			isert_dbg("ISER_WSV: write_stag: 0x%x write_va: 0x%llx\n",
				  write_stag, (unsigned long long)write_va);
		}

		isert_dbg("ISER ISCSI_CTRL PDU\n");
		break;
	case ISER_HELLO:
		isert_err("iSER Hello message\n");
		break;
	default:
		isert_warn("Unknown iSER hdr flags: 0x%02x\n", iser_hdr->flags);
		break;
	}

	isert_rx_opcode(isert_conn, rx_desc,
			read_stag, read_va, write_stag, write_va);
}

static void
isert_rcv_completion(struct iser_rx_desc *desc,
		     struct isert_conn *isert_conn,
		     u32 xfer_len)
{
	struct ib_device *ib_dev = isert_conn->device->ib_device;
	struct iscsi_hdr *hdr;
	u64 rx_dma;
	int rx_buflen;

	if ((char *)desc == isert_conn->login_req_buf) {
		rx_dma = isert_conn->login_req_dma;
		rx_buflen = ISER_RX_LOGIN_SIZE;
		isert_dbg("login_buf: Using rx_dma: 0x%llx, rx_buflen: %d\n",
			 rx_dma, rx_buflen);
	} else {
		rx_dma = desc->dma_addr;
		rx_buflen = ISER_RX_PAYLOAD_SIZE;
		isert_dbg("req_buf: Using rx_dma: 0x%llx, rx_buflen: %d\n",
			 rx_dma, rx_buflen);
	}

	ib_dma_sync_single_for_cpu(ib_dev, rx_dma, rx_buflen, DMA_FROM_DEVICE);

	hdr = &desc->iscsi_header;
	isert_dbg("iSCSI opcode: 0x%02x, ITT: 0x%08x, flags: 0x%02x dlen: %d\n",
		 hdr->opcode, hdr->itt, hdr->flags,
		 (int)(xfer_len - ISER_HEADERS_LEN));

	if ((char *)desc == isert_conn->login_req_buf) {
		isert_conn->login_req_len = xfer_len - ISER_HEADERS_LEN;
		if (isert_conn->conn) {
			struct iscsi_login *login = isert_conn->conn->conn_login;

			if (login && !login->first_request)
				isert_rx_login_req(isert_conn);
		}
		mutex_lock(&isert_conn->mutex);
		complete(&isert_conn->login_req_comp);
		mutex_unlock(&isert_conn->mutex);
	} else {
		isert_rx_do_work(desc, isert_conn);
	}

	ib_dma_sync_single_for_device(ib_dev, rx_dma, rx_buflen,
				      DMA_FROM_DEVICE);

}

static int
isert_map_data_buf(struct isert_conn *isert_conn, struct isert_cmd *isert_cmd,
		   struct scatterlist *sg, u32 nents, u32 length, u32 offset,
		   enum iser_ib_op_code op, struct isert_data_buf *data)
{
	struct ib_device *ib_dev = isert_conn->cm_id->device;

	data->dma_dir = op == ISER_IB_RDMA_WRITE ?
			      DMA_TO_DEVICE : DMA_FROM_DEVICE;

	data->len = length - offset;
	data->offset = offset;
	data->sg_off = data->offset / PAGE_SIZE;

	data->sg = &sg[data->sg_off];
	data->nents = min_t(unsigned int, nents - data->sg_off,
					  ISCSI_ISER_SG_TABLESIZE);
	data->len = min_t(unsigned int, data->len, ISCSI_ISER_SG_TABLESIZE *
					PAGE_SIZE);

	data->dma_nents = ib_dma_map_sg(ib_dev, data->sg, data->nents,
					data->dma_dir);
	if (unlikely(!data->dma_nents)) {
		isert_err("Cmd: unable to dma map SGs %p\n", sg);
		return -EINVAL;
	}

	isert_dbg("Mapped cmd: %p count: %u sg: %p sg_nents: %u rdma_len %d\n",
		  isert_cmd, data->dma_nents, data->sg, data->nents, data->len);

	return 0;
}

static void
isert_unmap_data_buf(struct isert_conn *isert_conn, struct isert_data_buf *data)
{
	struct ib_device *ib_dev = isert_conn->cm_id->device;

	ib_dma_unmap_sg(ib_dev, data->sg, data->nents, data->dma_dir);
	memset(data, 0, sizeof(*data));
}



static void
isert_unmap_cmd(struct isert_cmd *isert_cmd, struct isert_conn *isert_conn)
{
	struct isert_rdma_wr *wr = &isert_cmd->rdma_wr;

	isert_dbg("Cmd %p\n", isert_cmd);

	if (wr->data.sg) {
		isert_dbg("Cmd %p unmap_sg op\n", isert_cmd);
		isert_unmap_data_buf(isert_conn, &wr->data);
	}

	if (wr->rdma_wr) {
		isert_dbg("Cmd %p free send_wr\n", isert_cmd);
		kfree(wr->rdma_wr);
		wr->rdma_wr = NULL;
	}

	if (wr->ib_sge) {
		isert_dbg("Cmd %p free ib_sge\n", isert_cmd);
		kfree(wr->ib_sge);
		wr->ib_sge = NULL;
	}
}

static void
isert_unreg_rdma(struct isert_cmd *isert_cmd, struct isert_conn *isert_conn)
{
	struct isert_rdma_wr *wr = &isert_cmd->rdma_wr;

	isert_dbg("Cmd %p\n", isert_cmd);

	if (wr->fr_desc) {
		isert_dbg("Cmd %p free fr_desc %p\n", isert_cmd, wr->fr_desc);
		if (wr->fr_desc->ind & ISERT_PROTECTED) {
			isert_unmap_data_buf(isert_conn, &wr->prot);
			wr->fr_desc->ind &= ~ISERT_PROTECTED;
		}
		spin_lock_bh(&isert_conn->pool_lock);
		list_add_tail(&wr->fr_desc->list, &isert_conn->fr_pool);
		spin_unlock_bh(&isert_conn->pool_lock);
		wr->fr_desc = NULL;
	}

	if (wr->data.sg) {
		isert_dbg("Cmd %p unmap_sg op\n", isert_cmd);
		isert_unmap_data_buf(isert_conn, &wr->data);
	}

	wr->ib_sge = NULL;
	wr->rdma_wr = NULL;
}

static void
isert_put_cmd(struct isert_cmd *isert_cmd, bool comp_err)
{
	struct iscsi_cmd *cmd = isert_cmd->iscsi_cmd;
	struct isert_conn *isert_conn = isert_cmd->conn;
	struct iscsi_conn *conn = isert_conn->conn;
	struct isert_device *device = isert_conn->device;
	struct iscsi_text_rsp *hdr;

	isert_dbg("Cmd %p\n", isert_cmd);

	switch (cmd->iscsi_opcode) {
	case ISCSI_OP_SCSI_CMD:
		spin_lock_bh(&conn->cmd_lock);
		if (!list_empty(&cmd->i_conn_node))
			list_del_init(&cmd->i_conn_node);
		spin_unlock_bh(&conn->cmd_lock);

		if (cmd->data_direction == DMA_TO_DEVICE) {
			iscsit_stop_dataout_timer(cmd);
			/*
			 * Check for special case during comp_err where
			 * WRITE_PENDING has been handed off from core,
			 * but requires an extra target_put_sess_cmd()
			 * before transport_generic_free_cmd() below.
			 */
			if (comp_err &&
			    cmd->se_cmd.t_state == TRANSPORT_WRITE_PENDING) {
				struct se_cmd *se_cmd = &cmd->se_cmd;

				target_put_sess_cmd(se_cmd);
			}
		}

		device->unreg_rdma_mem(isert_cmd, isert_conn);
		transport_generic_free_cmd(&cmd->se_cmd, 0);
		break;
	case ISCSI_OP_SCSI_TMFUNC:
		spin_lock_bh(&conn->cmd_lock);
		if (!list_empty(&cmd->i_conn_node))
			list_del_init(&cmd->i_conn_node);
		spin_unlock_bh(&conn->cmd_lock);

		transport_generic_free_cmd(&cmd->se_cmd, 0);
		break;
	case ISCSI_OP_REJECT:
	case ISCSI_OP_NOOP_OUT:
	case ISCSI_OP_TEXT:
		hdr = (struct iscsi_text_rsp *)&isert_cmd->tx_desc.iscsi_header;
		/* If the continue bit is on, keep the command alive */
		if (hdr->flags & ISCSI_FLAG_TEXT_CONTINUE)
			break;

		spin_lock_bh(&conn->cmd_lock);
		if (!list_empty(&cmd->i_conn_node))
			list_del_init(&cmd->i_conn_node);
		spin_unlock_bh(&conn->cmd_lock);

		/*
		 * Handle special case for REJECT when iscsi_add_reject*() has
		 * overwritten the original iscsi_opcode assignment, and the
		 * associated cmd->se_cmd needs to be released.
		 */
		if (cmd->se_cmd.se_tfo != NULL) {
			isert_dbg("Calling transport_generic_free_cmd for 0x%02x\n",
				 cmd->iscsi_opcode);
			transport_generic_free_cmd(&cmd->se_cmd, 0);
			break;
		}
		/*
		 * Fall-through
		 */
	default:
		iscsit_release_cmd(cmd);
		break;
	}
}

static void
isert_unmap_tx_desc(struct iser_tx_desc *tx_desc, struct ib_device *ib_dev)
{
	if (tx_desc->dma_addr != 0) {
		isert_dbg("unmap single for tx_desc->dma_addr\n");
		ib_dma_unmap_single(ib_dev, tx_desc->dma_addr,
				    ISER_HEADERS_LEN, DMA_TO_DEVICE);
		tx_desc->dma_addr = 0;
	}
}

static void
isert_completion_put(struct iser_tx_desc *tx_desc, struct isert_cmd *isert_cmd,
		     struct ib_device *ib_dev, bool comp_err)
{
	if (isert_cmd->pdu_buf_dma != 0) {
		isert_dbg("unmap single for isert_cmd->pdu_buf_dma\n");
		ib_dma_unmap_single(ib_dev, isert_cmd->pdu_buf_dma,
				    isert_cmd->pdu_buf_len, DMA_TO_DEVICE);
		isert_cmd->pdu_buf_dma = 0;
	}

	isert_unmap_tx_desc(tx_desc, ib_dev);
	isert_put_cmd(isert_cmd, comp_err);
}

static int
isert_check_pi_status(struct se_cmd *se_cmd, struct ib_mr *sig_mr)
{
	struct ib_mr_status mr_status;
	int ret;

	ret = ib_check_mr_status(sig_mr, IB_MR_CHECK_SIG_STATUS, &mr_status);
	if (ret) {
		isert_err("ib_check_mr_status failed, ret %d\n", ret);
		goto fail_mr_status;
	}

	if (mr_status.fail_status & IB_MR_CHECK_SIG_STATUS) {
		u64 sec_offset_err;
		u32 block_size = se_cmd->se_dev->dev_attrib.block_size + 8;

		switch (mr_status.sig_err.err_type) {
		case IB_SIG_BAD_GUARD:
			se_cmd->pi_err = TCM_LOGICAL_BLOCK_GUARD_CHECK_FAILED;
			break;
		case IB_SIG_BAD_REFTAG:
			se_cmd->pi_err = TCM_LOGICAL_BLOCK_REF_TAG_CHECK_FAILED;
			break;
		case IB_SIG_BAD_APPTAG:
			se_cmd->pi_err = TCM_LOGICAL_BLOCK_APP_TAG_CHECK_FAILED;
			break;
		}
		sec_offset_err = mr_status.sig_err.sig_err_offset;
		do_div(sec_offset_err, block_size);
		se_cmd->bad_sector = sec_offset_err + se_cmd->t_task_lba;

		isert_err("PI error found type %d at sector 0x%llx "
			  "expected 0x%x vs actual 0x%x\n",
			  mr_status.sig_err.err_type,
			  (unsigned long long)se_cmd->bad_sector,
			  mr_status.sig_err.expected,
			  mr_status.sig_err.actual);
		ret = 1;
	}

fail_mr_status:
	return ret;
}

static void
isert_completion_rdma_write(struct iser_tx_desc *tx_desc,
			    struct isert_cmd *isert_cmd)
{
	struct isert_rdma_wr *wr = &isert_cmd->rdma_wr;
	struct iscsi_cmd *cmd = isert_cmd->iscsi_cmd;
	struct se_cmd *se_cmd = &cmd->se_cmd;
	struct isert_conn *isert_conn = isert_cmd->conn;
	struct isert_device *device = isert_conn->device;
	int ret = 0;

	if (wr->fr_desc && wr->fr_desc->ind & ISERT_PROTECTED) {
		ret = isert_check_pi_status(se_cmd,
					    wr->fr_desc->pi_ctx->sig_mr);
		wr->fr_desc->ind &= ~ISERT_PROTECTED;
	}

	device->unreg_rdma_mem(isert_cmd, isert_conn);
	wr->rdma_wr_num = 0;
	if (ret)
		transport_send_check_condition_and_sense(se_cmd,
							 se_cmd->pi_err, 0);
	else
		isert_put_response(isert_conn->conn, cmd);
}

static void
isert_completion_rdma_read(struct iser_tx_desc *tx_desc,
			   struct isert_cmd *isert_cmd)
{
	struct isert_rdma_wr *wr = &isert_cmd->rdma_wr;
	struct iscsi_cmd *cmd = isert_cmd->iscsi_cmd;
	struct se_cmd *se_cmd = &cmd->se_cmd;
	struct isert_conn *isert_conn = isert_cmd->conn;
	struct isert_device *device = isert_conn->device;
	int ret = 0;

	if (wr->fr_desc && wr->fr_desc->ind & ISERT_PROTECTED) {
		ret = isert_check_pi_status(se_cmd,
					    wr->fr_desc->pi_ctx->sig_mr);
		wr->fr_desc->ind &= ~ISERT_PROTECTED;
	}

	iscsit_stop_dataout_timer(cmd);
	device->unreg_rdma_mem(isert_cmd, isert_conn);
	cmd->write_data_done = wr->data.len;
	wr->rdma_wr_num = 0;

	isert_dbg("Cmd: %p RDMA_READ comp calling execute_cmd\n", isert_cmd);
	spin_lock_bh(&cmd->istate_lock);
	cmd->cmd_flags |= ICF_GOT_LAST_DATAOUT;
	cmd->i_state = ISTATE_RECEIVED_LAST_DATAOUT;
	spin_unlock_bh(&cmd->istate_lock);

	if (ret) {
		target_put_sess_cmd(se_cmd);
		transport_send_check_condition_and_sense(se_cmd,
							 se_cmd->pi_err, 0);
	} else {
		target_execute_cmd(se_cmd);
	}
}

static void
isert_do_control_comp(struct work_struct *work)
{
	struct isert_cmd *isert_cmd = container_of(work,
			struct isert_cmd, comp_work);
	struct isert_conn *isert_conn = isert_cmd->conn;
	struct ib_device *ib_dev = isert_conn->cm_id->device;
	struct iscsi_cmd *cmd = isert_cmd->iscsi_cmd;

	isert_dbg("Cmd %p i_state %d\n", isert_cmd, cmd->i_state);

	switch (cmd->i_state) {
	case ISTATE_SEND_TASKMGTRSP:
		iscsit_tmr_post_handler(cmd, cmd->conn);
	case ISTATE_SEND_REJECT:   /* FALLTHRU */
	case ISTATE_SEND_TEXTRSP:  /* FALLTHRU */
		cmd->i_state = ISTATE_SENT_STATUS;
		isert_completion_put(&isert_cmd->tx_desc, isert_cmd,
				     ib_dev, false);
		break;
	case ISTATE_SEND_LOGOUTRSP:
		iscsit_logout_post_handler(cmd, cmd->conn);
		break;
	default:
		isert_err("Unknown i_state %d\n", cmd->i_state);
		dump_stack();
		break;
	}
}

static void
isert_response_completion(struct iser_tx_desc *tx_desc,
			  struct isert_cmd *isert_cmd,
			  struct isert_conn *isert_conn,
			  struct ib_device *ib_dev)
{
	struct iscsi_cmd *cmd = isert_cmd->iscsi_cmd;

	if (cmd->i_state == ISTATE_SEND_TASKMGTRSP ||
	    cmd->i_state == ISTATE_SEND_LOGOUTRSP ||
	    cmd->i_state == ISTATE_SEND_REJECT ||
	    cmd->i_state == ISTATE_SEND_TEXTRSP) {
		isert_unmap_tx_desc(tx_desc, ib_dev);

		INIT_WORK(&isert_cmd->comp_work, isert_do_control_comp);
		queue_work(isert_comp_wq, &isert_cmd->comp_work);
		return;
	}

	cmd->i_state = ISTATE_SENT_STATUS;
	isert_completion_put(tx_desc, isert_cmd, ib_dev, false);
}

static void
isert_snd_completion(struct iser_tx_desc *tx_desc,
		      struct isert_conn *isert_conn)
{
	struct ib_device *ib_dev = isert_conn->cm_id->device;
	struct isert_cmd *isert_cmd = tx_desc->isert_cmd;
	struct isert_rdma_wr *wr;

	if (!isert_cmd) {
		isert_unmap_tx_desc(tx_desc, ib_dev);
		return;
	}
	wr = &isert_cmd->rdma_wr;

	isert_dbg("Cmd %p iser_ib_op %d\n", isert_cmd, wr->iser_ib_op);

	switch (wr->iser_ib_op) {
	case ISER_IB_SEND:
		isert_response_completion(tx_desc, isert_cmd,
					  isert_conn, ib_dev);
		break;
	case ISER_IB_RDMA_WRITE:
		isert_completion_rdma_write(tx_desc, isert_cmd);
		break;
	case ISER_IB_RDMA_READ:
		isert_completion_rdma_read(tx_desc, isert_cmd);
		break;
	default:
		isert_err("Unknown wr->iser_ib_op: 0x%x\n", wr->iser_ib_op);
		dump_stack();
		break;
	}
}

/**
 * is_isert_tx_desc() - Indicate if the completion wr_id
 *     is a TX descriptor or not.
 * @isert_conn: iser connection
 * @wr_id: completion WR identifier
 *
 * Since we cannot rely on wc opcode in FLUSH errors
 * we must work around it by checking if the wr_id address
 * falls in the iser connection rx_descs buffer. If so
 * it is an RX descriptor, otherwize it is a TX.
 */
static inline bool
is_isert_tx_desc(struct isert_conn *isert_conn, void *wr_id)
{
	void *start = isert_conn->rx_descs;
	int len = ISERT_QP_MAX_RECV_DTOS * sizeof(*isert_conn->rx_descs);

	if ((wr_id >= start && wr_id < start + len) ||
	    (wr_id == isert_conn->login_req_buf))
		return false;

	return true;
}

static void
isert_cq_comp_err(struct isert_conn *isert_conn, struct ib_wc *wc)
{
	if (wc->wr_id == ISER_BEACON_WRID) {
		isert_info("conn %p completing wait_comp_err\n",
			   isert_conn);
		complete(&isert_conn->wait_comp_err);
	} else if (is_isert_tx_desc(isert_conn, (void *)(uintptr_t)wc->wr_id)) {
		struct ib_device *ib_dev = isert_conn->cm_id->device;
		struct isert_cmd *isert_cmd;
		struct iser_tx_desc *desc;

		desc = (struct iser_tx_desc *)(uintptr_t)wc->wr_id;
		isert_cmd = desc->isert_cmd;
		if (!isert_cmd)
			isert_unmap_tx_desc(desc, ib_dev);
		else
			isert_completion_put(desc, isert_cmd, ib_dev, true);
	}
}

static void
isert_handle_wc(struct ib_wc *wc)
{
	struct isert_conn *isert_conn;
	struct iser_tx_desc *tx_desc;
	struct iser_rx_desc *rx_desc;

	isert_conn = wc->qp->qp_context;
	if (likely(wc->status == IB_WC_SUCCESS)) {
		if (wc->opcode == IB_WC_RECV) {
			rx_desc = (struct iser_rx_desc *)(uintptr_t)wc->wr_id;
			isert_rcv_completion(rx_desc, isert_conn, wc->byte_len);
		} else {
			tx_desc = (struct iser_tx_desc *)(uintptr_t)wc->wr_id;
			isert_snd_completion(tx_desc, isert_conn);
		}
	} else {
		if (wc->status != IB_WC_WR_FLUSH_ERR)
			isert_err("%s (%d): wr id %llx vend_err %x\n",
				  ib_wc_status_msg(wc->status), wc->status,
				  wc->wr_id, wc->vendor_err);
		else
			isert_dbg("%s (%d): wr id %llx\n",
				  ib_wc_status_msg(wc->status), wc->status,
				  wc->wr_id);

		if (wc->wr_id != ISER_FASTREG_LI_WRID)
			isert_cq_comp_err(isert_conn, wc);
	}
}

static void
isert_cq_work(struct work_struct *work)
{
	enum { isert_poll_budget = 65536 };
	struct isert_comp *comp = container_of(work, struct isert_comp,
					       work);
	struct ib_wc *const wcs = comp->wcs;
	int i, n, completed = 0;

	while ((n = ib_poll_cq(comp->cq, ARRAY_SIZE(comp->wcs), wcs)) > 0) {
		for (i = 0; i < n; i++)
			isert_handle_wc(&wcs[i]);

		completed += n;
		if (completed >= isert_poll_budget)
			break;
	}

	ib_req_notify_cq(comp->cq, IB_CQ_NEXT_COMP);
}

static void
isert_cq_callback(struct ib_cq *cq, void *context)
{
	struct isert_comp *comp = context;

	queue_work(isert_comp_wq, &comp->work);
}

static int
isert_post_response(struct isert_conn *isert_conn, struct isert_cmd *isert_cmd)
{
	struct ib_send_wr *wr_failed;
	int ret;

	ret = isert_post_recv(isert_conn, isert_cmd->rx_desc);
	if (ret) {
		isert_err("ib_post_recv failed with %d\n", ret);
		return ret;
	}

	ret = ib_post_send(isert_conn->qp, &isert_cmd->tx_desc.send_wr,
			   &wr_failed);
	if (ret) {
		isert_err("ib_post_send failed with %d\n", ret);
		return ret;
	}
	return ret;
}

static int
isert_put_response(struct iscsi_conn *conn, struct iscsi_cmd *cmd)
{
	struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);
	struct isert_conn *isert_conn = conn->context;
	struct ib_send_wr *send_wr = &isert_cmd->tx_desc.send_wr;
	struct iscsi_scsi_rsp *hdr = (struct iscsi_scsi_rsp *)
				&isert_cmd->tx_desc.iscsi_header;

	isert_create_send_desc(isert_conn, isert_cmd, &isert_cmd->tx_desc);
	iscsit_build_rsp_pdu(cmd, conn, true, hdr);
	isert_init_tx_hdrs(isert_conn, &isert_cmd->tx_desc);
	/*
	 * Attach SENSE DATA payload to iSCSI Response PDU
	 */
	if (cmd->se_cmd.sense_buffer &&
	    ((cmd->se_cmd.se_cmd_flags & SCF_TRANSPORT_TASK_SENSE) ||
	    (cmd->se_cmd.se_cmd_flags & SCF_EMULATED_TASK_SENSE))) {
		struct isert_device *device = isert_conn->device;
		struct ib_device *ib_dev = device->ib_device;
		struct ib_sge *tx_dsg = &isert_cmd->tx_desc.tx_sg[1];
		u32 padding, pdu_len;

		put_unaligned_be16(cmd->se_cmd.scsi_sense_length,
				   cmd->sense_buffer);
		cmd->se_cmd.scsi_sense_length += sizeof(__be16);

		padding = -(cmd->se_cmd.scsi_sense_length) & 3;
		hton24(hdr->dlength, (u32)cmd->se_cmd.scsi_sense_length);
		pdu_len = cmd->se_cmd.scsi_sense_length + padding;

		isert_cmd->pdu_buf_dma = ib_dma_map_single(ib_dev,
				(void *)cmd->sense_buffer, pdu_len,
				DMA_TO_DEVICE);

		isert_cmd->pdu_buf_len = pdu_len;
		tx_dsg->addr	= isert_cmd->pdu_buf_dma;
		tx_dsg->length	= pdu_len;
		tx_dsg->lkey	= device->pd->local_dma_lkey;
		isert_cmd->tx_desc.num_sge = 2;
	}

	isert_init_send_wr(isert_conn, isert_cmd, send_wr);

	isert_dbg("Posting SCSI Response\n");

	return isert_post_response(isert_conn, isert_cmd);
}

static void
isert_aborted_task(struct iscsi_conn *conn, struct iscsi_cmd *cmd)
{
	struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);
	struct isert_conn *isert_conn = conn->context;
	struct isert_device *device = isert_conn->device;

	spin_lock_bh(&conn->cmd_lock);
	if (!list_empty(&cmd->i_conn_node))
		list_del_init(&cmd->i_conn_node);
	spin_unlock_bh(&conn->cmd_lock);

	if (cmd->data_direction == DMA_TO_DEVICE)
		iscsit_stop_dataout_timer(cmd);

	device->unreg_rdma_mem(isert_cmd, isert_conn);
}

static enum target_prot_op
isert_get_sup_prot_ops(struct iscsi_conn *conn)
{
	struct isert_conn *isert_conn = conn->context;
	struct isert_device *device = isert_conn->device;

	if (conn->tpg->tpg_attrib.t10_pi) {
		if (device->pi_capable) {
			isert_info("conn %p PI offload enabled\n", isert_conn);
			isert_conn->pi_support = true;
			return TARGET_PROT_ALL;
		}
	}

	isert_info("conn %p PI offload disabled\n", isert_conn);
	isert_conn->pi_support = false;

	return TARGET_PROT_NORMAL;
}

static int
isert_put_nopin(struct iscsi_cmd *cmd, struct iscsi_conn *conn,
		bool nopout_response)
{
	struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);
	struct isert_conn *isert_conn = conn->context;
	struct ib_send_wr *send_wr = &isert_cmd->tx_desc.send_wr;

	isert_create_send_desc(isert_conn, isert_cmd, &isert_cmd->tx_desc);
	iscsit_build_nopin_rsp(cmd, conn, (struct iscsi_nopin *)
			       &isert_cmd->tx_desc.iscsi_header,
			       nopout_response);
	isert_init_tx_hdrs(isert_conn, &isert_cmd->tx_desc);
	isert_init_send_wr(isert_conn, isert_cmd, send_wr);

	isert_dbg("conn %p Posting NOPIN Response\n", isert_conn);

	return isert_post_response(isert_conn, isert_cmd);
}

static int
isert_put_logout_rsp(struct iscsi_cmd *cmd, struct iscsi_conn *conn)
{
	struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);