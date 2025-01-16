izeof *attr, GFP_KERNEL);
	if (!attr)
		return -ENOMEM;

	ret = ib_find_cached_pkey(target->srp_host->srp_dev->dev,
				  target->srp_host->port,
				  be16_to_cpu(target->pkey),
				  &attr->pkey_index);
	if (ret)
		goto out;

	attr->qp_state        = IB_QPS_INIT;
	attr->qp_access_flags = (IB_ACCESS_REMOTE_READ |
				    IB_ACCESS_REMOTE_WRITE);
	attr->port_num        = target->srp_host->port;

	ret = ib_modify_qp(qp, attr,
			   IB_QP_STATE		|
			   IB_QP_PKEY_INDEX	|
			   IB_QP_ACCESS_FLAGS	|
			   IB_QP_PORT);

out:
	kfree(attr);
	return ret;
}

static int srp_new_cm_id(struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;
	struct ib_cm_id *new_cm_id;

	new_cm_id = ib_create_cm_id(target->srp_host->srp_dev->dev,
				    srp_cm_handler, ch);
	if (IS_ERR(new_cm_id))
		return PTR_ERR(new_cm_id);

	if (ch->cm_id)
		ib_destroy_cm_id(ch->cm_id);
	ch->cm_id = new_cm_id;
	ch->path.sgid = target->sgid;
	ch->path.dgid = target->orig_dgid;
	ch->path.pkey = target->pkey;
	ch->path.service_id = target->service_id;

	return 0;
}

static struct ib_fmr_pool *srp_alloc_fmr_pool(struct srp_target_port *target)
{
	struct srp_device *dev = target->srp_host->srp_dev;
	struct ib_fmr_pool_param fmr_param;

	memset(&fmr_param, 0, sizeof(fmr_param));
	fmr_param.pool_size	    = target->scsi_host->can_queue;
	fmr_param.dirty_watermark   = fmr_param.pool_size / 4;
	fmr_param.cache		    = 1;
	fmr_param.max_pages_per_fmr = dev->max_pages_per_mr;
	fmr_param.page_shift	    = ilog2(dev->mr_page_size);
	fmr_param.access	    = (IB_ACCESS_LOCAL_WRITE |
				       IB_ACCESS_REMOTE_WRITE |
				       IB_ACCESS_REMOTE_READ);

	return ib_create_fmr_pool(dev->pd, &fmr_param);
}

/**
 * srp_destroy_fr_pool() - free the resources owned by a pool
 * @pool: Fast registration pool to be destroyed.
 */
static void srp_destroy_fr_pool(struct srp_fr_pool *pool)
{
	int i;
	struct srp_fr_desc *d;

	if (!pool)
		return;

	for (i = 0, d = &pool->desc[0]; i < pool->size; i++, d++) {
		if (d->mr)
			ib_dereg_mr(d->mr);
	}
	kfree(pool);
}

/**
 * srp_create_fr_pool() - allocate and initialize a pool for fast registration
 * @device:            IB device to allocate fast registration descriptors for.
 * @pd:                Protection domain associated with the FR descriptors.
 * @pool_size:         Number of descriptors to allocate.
 * @max_page_list_len: Maximum fast registration work request page list length.
 */
static struct srp_fr_pool *srp_create_fr_pool(struct ib_device *device,
					      struct ib_pd *pd, int pool_size,
					      int max_page_list_len)
{
	struct srp_fr_pool *pool;
	struct srp_fr_desc *d;
	struct ib_mr *mr;
	int i, ret = -EINVAL;

	if (pool_size <= 0)
		goto err;
	ret = -ENOMEM;
	pool = kzalloc(sizeof(struct srp_fr_pool) +
		       pool_size * sizeof(struct srp_fr_desc), GFP_KERNEL);
	if (!pool)
		goto err;
	pool->size = pool_size;
	pool->max_page_list_len = max_page_list_len;
	spin_lock_init(&pool->lock);
	INIT_LIST_HEAD(&pool->free_list);

	for (i = 0, d = &pool->desc[0]; i < pool->size; i++, d++) {
		mr = ib_alloc_mr(pd, IB_MR_TYPE_MEM_REG,
				 max_page_list_len);
		if (IS_ERR(mr)) {
			ret = PTR_ERR(mr);
			goto destroy_pool;
		}
		d->mr = mr;
		list_add_tail(&d->entry, &pool->free_list);
	}

out:
	return pool;

destroy_pool:
	srp_destroy_fr_pool(pool);

err:
	pool = ERR_PTR(ret);
	goto out;
}

/**
 * srp_fr_pool_get() - obtain a descriptor suitable for fast registration
 * @pool: Pool to obtain descriptor from.
 */
static struct srp_fr_desc *srp_fr_pool_get(struct srp_fr_pool *pool)
{
	struct srp_fr_desc *d = NULL;
	unsigned long flags;

	spin_lock_irqsave(&pool->lock, flags);
	if (!list_empty(&pool->free_list)) {
		d = list_first_entry(&pool->free_list, typeof(*d), entry);
		list_del(&d->entry);
	}
	spin_unlock_irqrestore(&pool->lock, flags);

	return d;
}

/**
 * srp_fr_pool_put() - put an FR descriptor back in the free list
 * @pool: Pool the descriptor was allocated from.
 * @desc: Pointer to an array of fast registration descriptor pointers.
 * @n:    Number of descriptors to put back.
 *
 * Note: The caller must already have queued an invalidation request for
 * desc->mr->rkey before calling this function.
 */
static void srp_fr_pool_put(struct srp_fr_pool *pool, struct srp_fr_desc **desc,
			    int n)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&pool->lock, flags);
	for (i = 0; i < n; i++)
		list_add(&desc[i]->entry, &pool->free_list);
	spin_unlock_irqrestore(&pool->lock, flags);
}

static struct srp_fr_pool *srp_alloc_fr_pool(struct srp_target_port *target)
{
	struct srp_device *dev = target->srp_host->srp_dev;

	return srp_create_fr_pool(dev->dev, dev->pd,
				  target->scsi_host->can_queue,
				  dev->max_pages_per_mr);
}

/**
 * srp_destroy_qp() - destroy an RDMA queue pair
 * @ch: SRP RDMA channel.
 *
 * Change a queue pair into the error state and wait until all receive
 * completions have been processed before destroying it. This avoids that
 * the receive completion handler can access the queue pair while it is
 * being destroyed.
 */
static void srp_destroy_qp(struct srp_rdma_ch *ch)
{
	static struct ib_qp_attr attr = { .qp_state = IB_QPS_ERR };
	static struct ib_recv_wr wr = { .wr_id = SRP_LAST_WR_ID };
	struct ib_recv_wr *bad_wr;
	int ret;

	/* Destroying a QP and reusing ch->done is only safe if not connected */
	WARN_ON_ONCE(ch->connected);

	ret = ib_modify_qp(ch->qp, &attr, IB_QP_STATE);
	WARN_ONCE(ret, "ib_cm_init_qp_attr() returned %d\n", ret);
	if (ret)
		goto out;

	init_completion(&ch->done);
	ret = ib_post_recv(ch->qp, &wr, &bad_wr);
	WARN_ONCE(ret, "ib_post_recv() returned %d\n", ret);
	if (ret == 0)
		wait_for_completion(&ch->done);

out:
	ib_destroy_qp(ch->qp);
}

static int srp_create_ch_ib(struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;
	struct srp_device *dev = target->srp_host->srp_dev;
	struct ib_qp_init_attr *init_attr;
	struct ib_cq *recv_cq, *send_cq;
	struct ib_qp *qp;
	struct ib_fmr_pool *fmr_pool = NULL;
	struct srp_fr_pool *fr_pool = NULL;
	const int m = dev->use_fast_reg ? 3 : 1;
	struct ib_cq_init_attr cq_attr = {};
	int ret;

	init_attr = kzalloc(sizeof *init_attr, GFP_KERNEL);
	if (!init_attr)
		return -ENOMEM;

	/* + 1 for SRP_LAST_WR_ID */
	cq_attr.cqe = target->queue_size + 1;
	cq_attr.comp_vector = ch->comp_vector;
	recv_cq = ib_create_cq(dev->dev, srp_recv_completion, NULL, ch,
			       &cq_attr);
	if (IS_ERR(recv_cq)) {
		ret = PTR_ERR(recv_cq);
		goto err;
	}

	cq_attr.cqe = m * target->queue_size;
	cq_attr.comp_vector = ch->comp_vector;
	send_cq = ib_create_cq(dev->dev, srp_send_completion, NULL, ch,
			       &cq_attr);
	if (IS_ERR(send_cq)) {
		ret = PTR_ERR(send_cq);
		goto err_recv_cq;
	}

	ib_req_notify_cq(recv_cq, IB_CQ_NEXT_COMP);

	init_attr->event_handler       = srp_qp_event;
	init_attr->cap.max_send_wr     = m * target->queue_size;
	init_attr->cap.max_recv_wr     = target->queue_size + 1;
	init_attr->cap.max_recv_sge    = 1;
	init_attr->cap.max_send_sge    = 1;
	init_attr->sq_sig_type         = IB_SIGNAL_REQ_WR;
	init_attr->qp_type             = IB_QPT_RC;
	init_attr->send_cq             = send_cq;
	init_attr->recv_cq             = recv_cq;

	qp = ib_create_qp(dev->pd, init_attr);
	if (IS_ERR(qp)) {
		ret = PTR_ERR(qp);
		goto err_send_cq;
	}

	ret = srp_init_qp(target, qp);
	if (ret)
		goto err_qp;

	if (dev->use_fast_reg) {
		fr_pool = srp_alloc_fr_pool(target);
		if (IS_ERR(fr_p