_fmr_pool(ch->fmr_pool);
		ch->fmr_pool = fmr_pool;
	}

	kfree(init_attr);
	return 0;

err_qp:
	ib_destroy_qp(qp);

err_send_cq:
	ib_destroy_cq(send_cq);

err_recv_cq:
	ib_destroy_cq(recv_cq);

err:
	kfree(init_attr);
	return ret;
}

/*
 * Note: this function may be called without srp_alloc_iu_bufs() having been
 * invoked. Hence the ch->[rt]x_ring checks.
 */
static void srp_free_ch_ib(struct srp_target_port *target,
			   struct srp_rdma_ch *ch)
{
	struct srp_device *dev = target->srp_host->srp_dev;
	int i;

	if (!ch->target)
		return;

	if (ch->cm_id) {
		ib_destroy_cm_id(ch->cm_id);
		ch->cm_id = NULL;
	}

	/* If srp_new_cm_id() succeeded but srp_create_ch_ib() not, return. */
	if (!ch->qp)
		return;

	if (dev->use_fast_reg) {
		if (ch->fr_pool)
			srp_destroy_fr_pool(ch->fr_pool);
	} else if (dev->use_fmr) {
		if (ch->fmr_pool)
			ib_destroy_fmr_pool(ch->fmr_pool);
	}
	srp_destroy_qp(ch);
	ib_destroy_cq(ch->send_cq);
	ib_destroy_cq(ch->recv_cq);

	/*
	 * Avoid that the SCSI error handler tries to use this channel after
	 * it has been freed. The SCSI error handler can namely continue
	 * trying to perform recovery actions after scsi_remove_host()
	 * returned.
	 */
	ch->target = NULL;

	ch->qp = NULL;
	ch->send_cq = ch->recv_cq = NULL;

	if (ch->rx_ring) {
		for (i = 0; i < target->queue_size; ++i)
			srp_free_iu(target->srp_host, ch->rx_ring[i]);
		kfree(ch->rx_ring);
		ch->rx_ring = NULL;
	}
	if (ch->tx_ring) {
		for (i = 0; i < target->queue_size; ++i)
			srp_free_iu(target->srp_host, ch->tx_ring[i]);
		kfree(ch->tx_ring);
		ch->tx_ring = NULL;
	}
}

static void srp_path_rec_completion(int status,
				    struct ib_sa_path_rec *pathrec,
				    void *ch_ptr)
{
	struct srp_rdma_ch *ch = ch_ptr;
	struct srp_target_port *target = ch->target;

	ch->status = status;
	if (status)
		shost_printk(KERN_ERR, target->scsi_host,
			     PFX "Got failed path rec status %d\n", status);
	else
		ch->path = *pathrec;
	complete(&ch->done);
}

static int srp_lookup_path(struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;
	int ret = -ENODEV;

	ch->path.numb_path = 1;

	init_completion(&ch->done);

	/*
	 * Avoid that the SCSI host can be removed by srp_remove_target()
	 * before srp_path_rec_completion() is called.
	 */
	if (!scsi_host_get(target->scsi_host))
		goto out;

	ch->path_query_id = ib_sa_path_rec_get(&srp_sa_client,
					       target->srp_host->srp_dev->dev,
					       target->srp_host->port,
					       &ch->path,
					       IB_SA_PATH_REC_SERVICE_ID |
					       IB_SA_PATH_REC_DGID	 |
					       IB_SA_PATH_REC_SGID	 |
					       IB_SA_PATH_REC_NUMB_PATH	 |
					       IB_SA_PATH_REC_PKEY,
					       SRP_PATH_REC_TIMEOUT_MS,
					       GFP_KERNEL,
					       srp_path_rec_completion,
					       ch, &ch->path_query);
	ret = ch->path_query_id;
	if (ret < 0)
		goto put;

	ret = wait_for_completion_interruptible(&ch->done);
	if (ret < 0)
		goto put;

	ret = ch->status;
	if (ret < 0)
		shost_printk(KERN_WARNING, target->scsi_host,
			     PFX "Path record query failed\n");

put:
	scsi_host_put(target->scsi_host);

out:
	return ret;
}

static int srp_send_req(struct srp_rdma_ch *ch, bool multich)
{
	struct srp_target_port *target = ch->target;
	struct {
		struct ib_cm_req_param param;
		struct srp_login_req   priv;
	} *req = NULL;
	int status;

	req = kzalloc(sizeof *req, GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	req->param.primary_path		      = &ch->path;
	req->