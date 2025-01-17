ent but didn't start iscsi login
	 * process. So work-around this by cleaning up what ever piled
	 * up in accepted and pending lists.
	 */
	mutex_lock(&isert_np->mutex);
	if (!list_empty(&isert_np->pending)) {
		isert_info("Still have isert pending connections\n");
		list_for_each_entry_safe(isert_conn, n,
					 &isert_np->pending,
					 node) {
			isert_info("cleaning isert_conn %p state (%d)\n",
				   isert_conn, isert_conn->state);
			list_move_tail(&isert_conn->node, &drop_conn_list);
		}
	}

	if (!list_empty(&isert_np->accepted)) {
		isert_info("Still have isert accepted connections\n");
		list_for_each_entry_safe(isert_conn, n,
					 &isert_np->accepted,
					 node) {
			isert_info("cleaning isert_conn %p state (%d)\n",
				   isert_conn, isert_conn->state);
			list_move_tail(&isert_conn->node, &drop_conn_list);
		}
	}
	mutex_unlock(&isert_np->mutex);

	list_for_each_entry_safe(isert_conn, n, &drop_conn_list, node) {
		list_del_init(&isert_conn->node);
		isert_connect_release(isert_conn);
	}

	np->np_context = NULL;
	kfree(isert_np);
}

static void isert_release_work(struct work_struct *work)
{
	struct isert_conn *isert_conn = container_of(work,
						     struct isert_conn,
						     release_work);

	isert_info("Starting release conn %p\n", isert_conn);

	wait_for_completion(&isert_conn->wait);

	mutex_lock(&isert_conn->mutex);
	isert_conn->state = ISER_CONN_DOWN;
	mutex_unlock(&isert_conn->mutex);

	isert_info("Destroying conn %p\n", isert_conn);
	isert_put_conn(isert_conn);
}

static void
isert_wait4logout(struct isert_conn *isert_conn)
{
	struct iscsi_conn *conn = isert_conn->conn;

	isert_info("conn %p\n", isert_conn);

	if (isert_conn->logout_posted) {
		isert_info("conn %p wait for conn_logout_comp\n", isert_conn);
		wait_for_completion_timeout(&conn->conn_logout_comp,
					    SECONDS_FOR_LOGOUT_COMP * HZ);
	}
}

static void
isert_wait4flush(struct isert_conn *isert_conn)
{
	struct ib_recv_wr *bad_wr;

	isert_info("conn %p\n", isert_conn);

	init_completion(&isert_conn->wait_comp_err);
	isert_conn->beacon.wr_id = ISER_BEACON_WRID;
	/* post an indication that all flush errors were consumed */
	if (ib_post_recv(isert_conn->qp, &isert_conn->beacon, &bad_wr)) {
		isert_err("conn %p failed to post beacon", isert_conn);
		return;
	}

	wait_for_completion(&isert_conn->wait_comp_err);
}

static void
isert_wait4cmds(struct iscsi_conn *conn)
{
	isert_info("iscsi_conn %p\n", conn);

	if (conn->sess) {
		target_sess_cmd_list_set_waiting(conn->sess->se_sess);
		target_wait_for_sess_cmds(conn->sess->se_sess);
	}
}

/**
 * isert_put_unsol_pending_cmds() - Drop commands waiting for
 *     unsolicitate dataout
 * @conn:    iscsi connection
 *
 * We might still have commands that are waiting for unsolicited
 * dataouts messages. We must put the extra reference on those
 * before blocking on the target_wait_for_session_cmds
 */
static void
isert_put_unsol_pending_cmds(struct iscsi_conn *conn)
{
	struct iscsi_cmd *cmd, *tmp;
	static LIST_HEAD(drop_cmd_list);

	spin_lock_bh(&conn->cmd_lock);
	list_for_each_entry_safe(cmd, tmp, &conn->conn_cmd_list, i_conn_node) {
		if ((cmd->cmd_flags & ICF_NON_IMMEDIATE_UNSOLICITED_DATA) &&
		    (cmd->write_data_done < conn->sess->sess_ops->FirstBurstLength) &&
		    (cmd->write_data_done < cmd->se_cmd.data_length))
			list_move_tail(&cmd->i_conn_node, &drop_cmd_list);
	}
	spin_unlock_bh(&conn->cmd_lock);

	list_for_each_entry_safe(cmd, tmp, &drop_cmd_list, i_conn_node) {
		list_del_init(&cmd->i_conn_node);
		if (cmd->i_state != ISTATE_REMOVE) {
			struct isert_cmd *isert_cmd = iscsit_priv_cmd(cmd);

			isert_info("conn %p dropping cmd %p\n", conn, cmd);
			isert_put_cmd(isert_cmd, true);
		}
	}
}

static void isert_wait_conn(struct iscsi_conn *conn)
{
	struct isert_conn *isert_conn = conn->context;

	isert_info("Starting conn %p\n", isert_conn);

	mutex_lock(&isert_conn->mutex);
	/*
	 * Only wait for wait_comp_err if the isert_conn made it
	 * into full feature phase..
	 */
	if (isert_conn->state == ISER_CONN_INIT) {
		mutex_unlock(&isert_conn->mutex);
		return;
	}
	isert_conn_terminate(isert_conn);
	mutex_unlock(&isert_conn->mutex);

	isert_wait4flush(isert_conn);
	isert_put_unsol_pending_cmds(conn);
	isert_wait4cmds(conn);
	isert_wait4logout(isert_conn);

	queue_work(isert_release_wq, &isert_conn->release_work);
}

static void isert_free_conn(struct iscsi_conn *conn)
{
	struct isert_conn *isert_conn = conn->context;

	isert_wait4flush(isert_conn);
	isert_put_conn(isert_conn);
}

static struct iscsit_transport iser_target_transport = {
	.name			= "IB/iSER",
	.transport_type		= ISCSI_INFINIBAND,
	.priv_size		= sizeof(struct isert_cmd),
	.owner			= THIS_MODULE,
	.iscsit_setup_np	= isert_setup_np,
	.iscsit_accept_np	= isert_accept_np,
	.iscsit_free_np		= isert_free_np,
	.iscsit_wait_conn	= isert_wait_conn,
	.iscsit_free_conn	= isert_free_conn,
	.iscsit_get_login_rx	= isert_get_login_rx,
	.iscsit_put_login_tx	= isert_put_login_tx,
	.iscsit_immediate_queue	= isert_immediate_queue,
	.iscsit_response_queue	= isert_response_queue,
	.iscsit_get_dataout	= isert_get_dataout,
	.iscsit_queue_data_in	= isert_put_datain,
	.iscsit_queue_status	= isert_put_response,
	.iscsit_aborted_task	= isert_aborted_task,
	.iscsit_get_sup_prot_ops = isert_get_sup_prot_ops,
};

static int __init isert_init(void)
{
	int ret;

	isert_comp_wq = alloc_workqueue("isert_comp_wq",
					WQ_UNBOUND | WQ_HIGHPRI, 0);
	if (!isert_comp_wq) {
		isert_err("Unable to allocate isert_comp_wq\n");
		ret = -ENOMEM;
		return -ENOMEM;
	}

	isert_release_wq = alloc_workqueue("isert_release_wq", WQ_UNBOUND,
					WQ_UNBOUND_MAX_ACTIVE);
	if (!isert_release_wq) {
		isert_err("Unable to allocate isert_release_wq\n");
		ret = -ENOMEM;
		goto destroy_comp_wq;
	}

	iscsit_register_transport(&iser_target_transport);
	isert_info("iSER_TARGET[0] - Loaded iser_target_transport\n");

	return 0;

destroy_comp_wq:
	destroy_workqueue(isert_comp_wq);

	return ret;
}

static void __exit isert_exit(void)
{
	flush_scheduled_work();
	destroy_workqueue(isert_release_wq);
	destroy_workqueue(isert_comp_wq);
	iscsit_unregister_transport(&iser_target_transport);
	isert_info("iSER_TARGET[0] - Released iser_target_transport\n");
}

MODULE_DESCRIPTION("iSER-Target for mainline target infrastructure");
MODULE_VERSION("1.0");
MODULE_AUTHOR("nab@Linux-iSCSI.org");
MODULE_LICENSE("GPL");

module_init(isert_init);
module_exit(isert_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    