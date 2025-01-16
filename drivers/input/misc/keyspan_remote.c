_dbg("sge: addr: 0x%llx  length: %u lkey: %x\n",
		  sge->addr, sge->length, sge->lkey);

	return ret;
}

static inline void
isert_set_dif_domain(struct se_cmd *se_cmd, struct ib_sig_attrs *sig_attrs,
		     struct ib_sig_domain *domain)
{
	domain->sig_type = IB_SIG_TYPE_T10_DIF;
	domain->sig.dif.bg_type = IB_T10DIF_CRC;
	domain->sig.dif.pi_interval = se_cmd->se_dev->dev_attrib.block_size;
	domain->sig.dif.ref_tag = se_cmd->reftag_seed;
	/*
	 * At the moment we hard code those, but if in the future
	 * the target core would like to use it, we will take it
	 * from se_cmd.
	 */
	domain->sig.dif.apptag_check_mask = 0xffff;
	domain->sig.dif.app_escape = true;
	domain->sig.dif.ref_escape = true;
	if (se_cmd->prot_type == TARGET_DIF_TYPE1_PROT ||
	    se_cmd->prot_type == TARGET_DIF_TYPE2_PROT)
		domain->sig.dif.ref_remap = true;
};

static int
isert_set_sig_attrs(struct se_cmd *se_cmd, struct ib_sig_attrs *sig_attrs)
{
	switch (se_cmd->prot_op) {
	case TARGET_PROT_DIN_INSERT:
	case TARGET_PROT_DOUT_STRIP:
		sig_attrs->mem.sig_type = IB_SIG_TYPE_NONE;
		isert_set_dif_domain(se_cmd, sig_attrs, &sig_attrs->wire);
		break;
	case TARGET_PROT_DOUT_INSERT:
	case TARGET_PROT_DIN_STRIP:
		sig_attrs->wire.sig_type = IB_SIG_TYPE_NONE;
		isert_set_dif_domain(se_cmd, sig_attrs, &sig_attrs->mem);
		break;
	case TARGET_PROT_DIN_PASS:
	case TARGET_PROT_DOUT_PASS:
		isert_set_dif_domain(se_cmd, sig_attrs, &sig_attrs->wire);
		isert_set_dif_domain(se_cmd, sig_attrs, &sig_attrs->mem);
		break;
	default:
		isert_err("Unsupported PI operation %d\n", se_cmd->prot_op);
		return -EINVAL;
	}

	return 0;
}

static inline u8
isert_set_prot_checks(u8 prot_checks)
{
	return (prot_checks & TARGET_DIF_CHECK_GUARD  ? 0xc0 : 0) |
	       (prot_checks & TARGET_DIF_CHECK_REFTAG ? 0x30 : 0) |
	       (prot_checks & TARGET_DIF_CHECK_REFTAG ? 0x0f : 0);
}

static int
isert_reg_sig_mr(struct isert_conn *isert_conn,
		 struct se_cmd *se_cmd,
		 struct isert_rdma_wr *rdma_wr,
		 struct fast_reg_descriptor *fr_desc)
{
	struct ib_sig_handover_wr sig_wr;
	struct ib_send_wr inv_wr, *bad_wr, *wr = NULL;
	struct pi_context *pi_ctx = fr_desc->pi_ctx;
	struct ib_sig_attrs sig_attrs;
	int ret;

	memset(&sig_attrs, 0, sizeof(sig_attrs));
	ret = isert_set_sig_attrs(se_cmd, &sig_attrs);
	if (ret)
		goto err;

	sig_attrs.check_mask = isert_set_prot_checks(se_cmd->prot_checks);

	if (!(fr_desc->ind & ISERT_SIG_KEY_VALID)) {
		isert_inv_rkey(&inv_wr, pi_ctx->sig_mr);
		wr = &inv_wr;
	}

	memset(&sig_wr, 0, sizeof(sig_wr));
	sig_wr.wr.opcode = IB_WR_REG_SIG_MR;
	sig_wr.wr.wr_id = ISER_FASTREG_LI_WRID;
	sig_wr.wr.sg_list = &rdma_wr->ib_sg[DATA];
	sig_wr.wr.num_sge = 1;
	sig_wr.access_flags = IB_ACCESS_LOCAL_WRITE;
	sig_wr.sig_attrs = &sig_attrs;
	sig_wr.sig_mr = pi_ctx->sig_mr;
	if (se_cmd->t_prot_sg)
		sig_wr.prot = &rdma_wr->ib_sg[PROT];

	if (!wr)
		wr = &sig_wr.wr;
	else
		wr->next = &sig_wr.wr;

	ret = ib_post_send(isert_conn->qp, wr, &bad_wr);
	if (ret) {
		isert_err("fast registration failed, ret:%d\n", ret);
		goto err;
	}
	fr_desc->ind &= ~ISERT_SIG_KEY_VALID;

	rdma_wr->ib_sg[SIG].lkey = pi_ctx->sig_mr->lkey;
	rdma_wr->ib_sg[SIG].addr = 0;
	rdma_wr->ib_sg[SIG].length = se_cmd->data_length;
	if (se_cmd->prot_op != TARGET_PROT_DIN_STRIP &&
	    se_cmd->prot_op != TARGET_PROT_DOUT_INSERT)
		/*
		 * We have protection guards on the wire
		 * so we need to set a larget transfer
		 */
		rdma_wr->ib_sg[SIG].length += se_cmd->prot_length;

	isert_dbg("sig_sge: addr: 0x%llx  length: %u lkey: %x\n",
		  rdma_wr->ib_sg[SIG].addr, rdma_wr->ib_sg[SIG].length,
		  rdma_wr->ib_sg[SIG].lkey);
err:
	return ret;
}

static int
isert_handle_prot_cmd(struct isert_conn *isert_conn,
		      struct isert_cmd *isert_cmd,
		      struct isert_rdma_wr *wr)
{
	struct isert_device *device = isert_conn->device;
	struct se_cmd *se_cmd = &isert_cmd->iscsi_cmd->se_cmd;
	int ret;

	if (!wr->fr_desc->pi_ctx) {
		ret = isert_create_pi_ctx(wr->fr_desc,
					  device->ib_device,
					  device->pd);
		if (ret) {
			isert_err("conn %p failed to allocate pi_ctx\n",
				  isert_conn);
			return ret;
		}
	}

	if (se_cmd->t_prot_sg) {
		ret = isert_map_data_buf(isert_conn, isert_cmd,
					 se_cmd->t_prot_sg,
					 se_cmd->t_prot_nents,
					 se_cmd->prot_length,
					 0, wr->iser_ib_op, &wr->prot);
		if (ret) {
			isert_err("conn %p failed to map protection buffer\n",
				  isert_conn);
			return ret;
		}

		memset(&wr->ib_sg[PROT], 0, sizeof(wr->ib_sg[PROT]));
		ret = isert_fast_reg_mr(isert_conn, wr->fr_desc, &wr->prot,
					ISERT_PROT_KEY_VALID, &wr->ib_sg[PROT]);
		if (ret) {
			isert_err("conn %p failed to fast reg mr\n",
				  isert_conn);
			goto unmap_prot_cmd;
		}
	}

	ret = isert_reg_sig_mr(isert_conn, se_cmd, wr, wr->fr_desc);
	if (ret) {
		isert_err("conn %p failed to fast reg mr\n",
			  isert_conn);
		goto unmap_prot_cmd;
	}
	wr->fr_desc->ind |= ISERT_PROTECTED;

	return 0;

unmap_prot_cmd:
	if (se_cmd->t_prot_sg)
		isert_unmap_data_buf(isert_conn, &wr->prot);

	return ret;
}

static int
isert_reg_rdma(struct iscsi_conn *conn, struct iscsi_cmd *cmd,
	       struct isert_rdma_wr *wr)
{
	struct se_cmd *se_cmd = &cmd->se_cmd;
	struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);
	struct isert_conn *isert_conn = conn->context;
	struct fast_reg_descriptor *fr_desc = NULL;
	struct ib_rdma_wr *rdma_wr;
	struct ib_sge *ib_sg;
	u32 offset;
	int ret = 0;
	unsigned long flags;

	isert_cmd->tx_desc.isert_cmd = isert_cmd;

	offset = wr->iser_ib_op == ISER_IB_RDMA_READ ? cmd->write_data_done : 0;
	ret = isert_map_data_buf(isert_conn, isert_cmd, se_cmd->t_data_sg,
				 se_cmd->t_data_nents, se_cmd->data_length,
				 offset, wr->iser_ib_op, &wr->data);
	if (ret)
		return ret;

	if (wr->data.dma_nents != 1 || isert_prot_cmd(isert_conn, se_cmd)) {
		spin_lock_irqsave(&isert_conn->pool_lock, flags);
		fr_desc = list_first_entry(&isert_conn->fr_pool,
					   struct fast_reg_descriptor, list);
		list_del(&fr_desc->list);
		spin_unlock_irqrestore(&isert_conn->pool_lock, flags);
		wr->fr_desc = fr_desc;
	}

	ret = isert_fast_reg_mr(isert_conn, fr_desc, &wr->data,
				ISERT_DATA_KEY_VALID, &wr->ib_sg[DATA]);
	if (ret)
		goto unmap_cmd;

	if (isert_prot_cmd(isert_conn, se_cmd)) {
		ret = isert_handle_prot_cmd(isert_conn, isert_cmd, wr);
		if (ret)
			goto unmap_cmd;

		ib_sg = &wr->ib_sg[SIG];
	} else {
		ib_sg = &wr->ib_sg[DATA];
	}

	memcpy(&wr->s_ib_sge, ib_sg, sizeof(*ib_sg));
	wr->ib_sge = &wr->s_ib_sge;
	wr->rdma_wr_num = 1;
	memset(&wr->s_rdma_wr, 0, sizeof(wr->s_rdma_wr));
	wr->rdma_wr = &wr->s_rdma_wr;
	wr->isert_cmd = isert_cmd;

	rdma_wr = &isert_cmd->rdma_wr.s_rdma_wr;
	rdma_wr->wr.sg_list = &wr->s_ib_sge;
	rdma_wr->wr.num_sge = 1;
	rdma_wr->wr.wr_id = (uintptr_t)&isert_cmd->tx_desc;
	if (wr->iser_ib_op == ISER_IB_RDMA_WRITE) {
		rdma_wr->wr.opcode = IB_WR_RDMA_WRITE;
		rdma_wr->remote_addr = isert_cmd->read_va;
		rdma_wr->rkey = isert_cmd->read_stag;
		rdma_wr->wr.send_flags = !isert_prot_cmd(isert_conn, se_cmd) ?
				      0 : IB_SEND_SIGNALED;
	} else {
		rdma_wr->wr.opcode = IB_WR_RDMA_READ;
		rdma_wr->remote_addr = isert_cmd->write_va;
		rdma_wr->rkey = isert_cmd->write_stag;
		rdma_wr->wr.send_flags = IB_SEND_SIGNALED;
	}

	return 0;

unmap_cmd:
	if (fr_desc) {
		spin_lock_irqsave(&isert_conn->pool_lock, flags);
		list_add_tail(&fr_desc->list, &isert_conn->fr_pool);
		spin_unlock_irqrestore(&isert_conn->pool_lock, flags);
	}
	isert_unmap_data_buf(isert_conn, &wr->data);

	return ret;
}

static int
isert_put_datain(struct iscsi_conn *conn, struct iscsi_cmd *cmd)
{
	struct se_cmd *se_cmd = &cmd->se_cmd;
	struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);
	struct isert_rdma_wr *wr = &isert_cmd->rdma_wr;
	struct isert_conn *isert_conn = conn->context;
	struct isert_device *device = isert_conn->device;
	struct ib_send_wr *wr_failed;
	int rc;

	isert_dbg("Cmd: %p RDMA_WRITE data_length: %u\n",
		 isert_cmd, se_cmd->data_length);

	wr->iser_ib_op = ISER_IB_RDMA_WRITE;
	rc = device->reg_rdma_mem(conn, cmd, wr);
	if (rc) {
		isert_err("Cmd: %p failed to prepare RDMA res\n", isert_cmd);
		return rc;
	}

	if (!isert_prot_cmd(isert_conn, se_cmd)) {
		/*
		 * Build isert_conn->tx_desc for iSCSI response PDU and attach
		 */
		isert_create_send_desc(isert_conn, isert_cmd,
				       &isert_cmd->tx_desc);
		iscsit_build_rsp_pdu(cmd, conn, true, (struct iscsi_scsi_rsp *)
				     &isert_cmd->tx_desc.iscsi_header);
		isert_init_tx_hdrs(isert_conn, &isert_cmd->tx_desc);
		isert_init_send_wr(isert_conn, isert_cmd,
				   &isert_cmd->tx_desc.send_wr);
		isert_cmd->rdma_wr.s_rdma_wr.wr.next = &isert_cmd->tx_desc.send_wr;
		wr->rdma_wr_num += 1;

		rc = isert_post_recv(isert_conn, isert_cmd->rx_desc);
		if (rc) {
			isert_err("ib_post_recv failed with %d\n", rc);
			return rc;
		}
	}

	rc = ib_post_send(isert_conn->qp, &wr->rdma_wr->wr, &wr_failed);
	if (rc)
		isert_warn("ib_post_send() failed for IB_WR_RDMA_WRITE\n");

	if (!isert_prot_cmd(isert_conn, se_cmd))
		isert_dbg("Cmd: %p posted RDMA_WRITE + Response for iSER Data "
			 "READ\n", isert_cmd);
	else
		isert_dbg("Cmd: %p posted RDMA_WRITE for iSER Data READ\n",
			 isert_cmd);

	return 1;
}

static int
isert_get_dataout(struct iscsi_conn *conn, struct iscsi_cmd *cmd, bool recovery)
{
	struct se_cmd *se_cmd = &cmd->se_cmd;
	struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);
	struct isert_rdma_wr *wr = &isert_cmd->rdma_wr;
	struct isert_conn *isert_conn = conn->context;
	struct isert_device *device = isert_conn->device;
	struct ib_send_wr *wr_failed;
	int rc;

	isert_dbg("Cmd: %p RDMA_READ data_length: %u write_data_done: %u\n",
		 isert_cmd, se_cmd->data_length, cmd->write_data_done);
	wr->iser_ib_op = ISER_IB_RDMA_READ;
	rc = device->reg_rdma_mem(conn, cmd, wr);
	if (rc) {
		isert_err("Cmd: %p failed to prepare RDMA res\n", isert_cmd);
		return rc;
	}

	rc = ib_post_send(isert_conn->qp, &wr->rdma_wr->wr, &wr_failed);
	if (rc)
		isert_warn("ib_post_send() failed for IB_WR_RDMA_READ\n");

	isert_dbg("Cmd: %p posted RDMA_READ memory for ISER Data WRITE\n",
		 isert_cmd);

	return 0;
}

static int
isert_immediate_queue(struct iscsi_conn *conn, struct iscsi_cmd *cmd, int state)
{
	struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);
	int ret = 0;

	switch (state) {
	case ISTATE_REMOVE:
		spin_lock_bh(&conn->cmd_lock);
		list_del_init(&cmd->i_conn_node);
		spin_unlock_bh(&conn->cmd_lock);
		isert_put_cmd(isert_cmd, true);
		break;
	case ISTATE_SEND_NOPIN_WANT_RESPONSE:
		ret = isert_put_nopin(cmd, conn, false);
		break;
	default:
		isert_err("Unknown immediate state: 0x%02x\n", state);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int
isert_response_queue(struct iscsi_conn *conn, struct iscsi_cmd *cmd, int state)
{
	struct isert_conn *isert_conn = conn->context;
	int ret;

	switch (state) {
	case ISTATE_SEND_LOGOUTRSP:
		ret = isert_put_logout_rsp(cmd, conn);
		if (!ret)
			isert_conn->logout_posted = true;
		break;
	case ISTATE_SEND_NOPIN:
		ret = isert_put_nopin(cmd, conn, true);
		break;
	case ISTATE_SEND_TASKMGTRSP:
		ret = isert_put_tm_rsp(cmd, conn);
		break;
	case ISTATE_SEND_REJECT:
		ret = isert_put_reject(cmd, conn);
		break;
	case ISTATE_SEND_TEXTRSP:
		ret = isert_put_text_rsp(cmd, conn);
		break;
	case ISTATE_SEND_STATUS:
		/*
		 * Special case for sending non GOOD SCSI status from TX thread
		 * context during pre se_cmd excecution failure.
		 */
		ret = isert_put_response(conn, cmd);
		break;
	default:
		isert_err("Unknown response state: 0x%02x\n", state);
		ret = -EINVAL;
		break;
	}

	return ret;
}

struct rdma_cm_id *
isert_setup_id(struct isert_np *isert_np)
{
	struct iscsi_np *np = isert_np->np;
	struct rdma_cm_id *id;
	struct sockaddr *sa;
	int ret;

	sa = (struct sockaddr *)&np->np_sockaddr;
	isert_dbg("ksockaddr: %p, sa: %p\n", &np->np_sockaddr, sa);

	id = rdma_create_id(&init_net, isert_cma_handler, isert_np,
			    RDMA_PS_TCP, IB_QPT_RC);
	if (IS_ERR(id)) {
		isert_err("rdma_create_id() failed: %ld\n", PTR_ERR(id));
		ret = PTR_ERR(id);
		goto out;
	}
	isert_dbg("id %p context %p\n", id, id->context);

	ret = rdma_bind_addr(id, sa);
	if (ret) {
		isert_err("rdma_bind_addr() failed: %d\n", ret);
		goto out_id;
	}

	ret = rdma_listen(id, 0);
	if (ret) {
		isert_err("rdma_listen() failed: %d\n", ret);
		goto out_id;
	}

	return id;
out_id:
	rdma_destroy_id(id);
out:
	return ERR_PTR(ret);
}

static int
isert_setup_np(struct iscsi_np *np,
	       struct sockaddr_storage *ksockaddr)
{
	struct isert_np *isert_np;
	struct rdma_cm_id *isert_lid;
	int ret;

	isert_np = kzalloc(sizeof(struct isert_np), GFP_KERNEL);
	if (!isert_np) {
		isert_err("Unable to allocate struct isert_np\n");
		return -ENOMEM;
	}
	sema_init(&isert_np->sem, 0);
	mutex_init(&isert_np->mutex);
	INIT_LIST_HEAD(&isert_np->accepted);
	INIT_LIST_HEAD(&isert_np->pending);
	isert_np->np = np;

	/*
	 * Setup the np->np_sockaddr from the passed sockaddr setup
	 * in iscsi_target_configfs.c code..
	 */
	memcpy(&np->np_sockaddr, ksockaddr,
	       sizeof(struct sockaddr_storage));

	isert_lid = isert_setup_id(isert_np);
	if (IS_ERR(isert_lid)) {
		ret = PTR_ERR(isert_lid);
		goto out;
	}

	isert_np->cm_id = isert_lid;
	np->np_context = isert_np;

	return 0;

out:
	kfree(isert_np);

	return ret;
}

static int
isert_rdma_accept(struct isert_conn *isert_conn)
{
	struct rdma_cm_id *cm_id = isert_conn->cm_id;
	struct rdma_conn_param cp;
	int ret;

	memset(&cp, 0, sizeof(struct rdma_conn_param));
	cp.initiator_depth = isert_conn->initiator_depth;
	cp.retry_count = 7;
	cp.rnr_retry_count = 7;

	ret = rdma_accept(cm_id, &cp);
	if (ret) {
		isert_err("rdma_accept() failed with: %d\n", ret);
		return ret;
	}

	return 0;
}

static int
isert_get_login_rx(struct iscsi_conn *conn, struct iscsi_login *login)
{
	struct isert_conn *isert_conn = conn->context;
	int ret;

	isert_info("before login_req comp conn: %p\n", isert_conn);
	ret = wait_for_completion_interruptible(&isert_conn->login_req_comp);
	if (ret) {
		isert_err("isert_conn %p interrupted before got login req\n",
			  isert_conn);
		return ret;
	}
	reinit_completion(&isert_conn->login_req_comp);

	/*
	 * For login requests after the first PDU, isert_rx_login_req() will
	 * kick schedule_delayed_work(&conn->login_work) as the packet is
	 * received, which turns this callback from iscsi_target_do_login_rx()
	 * into a NOP.
	 */
	if (!login->first_request)
		return 0;

	isert_rx_login_req(isert_conn);

	isert_info("before login_comp conn: %p\n", conn);
	ret = wait_for_completion_interruptible(&isert_conn->login_comp);
	if (ret)
		return ret;

	isert_info("processing login->req: %p\n", login->req);

	return 0;
}

static void
isert_set_conn_info(struct iscsi_np *np, struct iscsi_conn *conn,
		    struct isert_conn *isert_conn)
{
	struct rdma_cm_id *cm_id = isert_conn->cm_id;
	struct rdma_route *cm_route = &cm_id->route;

	conn->login_family = np->np_sockaddr.ss_family;

	conn->login_sockaddr = cm_route->addr.dst_addr;
	conn->local_sockaddr = cm_route->addr.src_addr;
}

static int
isert_accept_np(struct iscsi_np *np, struct iscsi_conn *conn)
{
	struct isert_np *isert_np = np->np_context;
	struct isert_conn *isert_conn;
	int ret;

accept_wait:
	ret = down_interruptible(&isert_np->sem);
	if (ret)
		return -ENODEV;

	spin_lock_bh(&np->np_thread_lock);
	if (np->np_thread_state >= ISCSI_NP_THREAD_RESET) {
		spin_unlock_bh(&np->np_thread_lock);
		isert_dbg("np_thread_state %d\n",
			 np->np_thread_state);
		/**
		 * No point in stalling here when np_thread
		 * is in state RESET/SHUTDOWN/EXIT - bail
		 **/
		return -ENODEV;
	}
	spin_unlock_bh(&np->np_thread_lock);

	mutex_lock(&isert_np->mutex);
	if (list_empty(&isert_np->pending)) {
		mutex_unlock(&isert_np->mut