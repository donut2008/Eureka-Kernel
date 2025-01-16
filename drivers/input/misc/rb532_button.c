 == SRPT_STATE_DONE);
	WARN_ON(new == SRPT_STATE_NEW);

	spin_lock_irqsave(&ioctx->spinlock, flags);
	previous = ioctx->state;
	if (previous == old)
		ioctx->state = new;
	spin_unlock_irqrestore(&ioctx->spinlock, flags);
	return previous == old;
}

/**
 * srpt_post_recv() - Post an IB receive request.
 */
static int srpt_post_recv(struct srpt_device *sdev,
			  struct srpt_recv_ioctx *ioctx)
{
	struct ib_sge list;
	struct ib_recv_wr wr, *bad_wr;

	BUG_ON(!sdev);
	wr.wr_id = encode_wr_id(SRPT_RECV, ioctx->ioctx.index);

	list.addr = ioctx->ioctx.dma;
	list.length = srp_max_req_size;
	list.lkey = sdev->pd->local_dma_lkey;

	wr.next = NULL;
	wr.sg_list = &list;
	wr.num_sge = 1;

	return ib_post_srq_recv(sdev->srq, &wr, &bad_wr);
}

/**
 * srpt_post_send() - Post an IB send request.
 *
 * Returns zero upon success and a non-zero value upon failure.
 */
static int srpt_post_send(struct srpt_rdma_ch *ch,
			  struct srpt_send_ioctx *ioctx, int len)
{
	struct ib_sge list;
	struct ib_send_wr wr, *bad_wr;
	struct srpt_device *sdev = ch->sport->sdev;
	int ret;

	atomic_inc(&ch->req_lim);

	ret = -ENOMEM;
	if (unlikely(atomic_dec_return(&ch->sq_wr_avail) < 0)) {
		pr_warn("IB send queue full (needed 1)\n");
		goto out;
	}

	ib_dma_sync_single_for_device(sdev->device, ioctx->ioctx.dma, len,
				      DMA_TO_DEVICE);

	list.addr = ioctx->ioctx.dma;
	list.length = len;
	list.lkey = sdev->pd->local_dma_lkey;

	wr.next = NULL;
	wr.wr_id = encode_wr_id(SRPT_SEND, ioctx->ioctx.index);
	wr.sg_list = &list;
	wr.num_sge = 1;
	wr.opcode = IB_WR_SEND;
	wr.send_flags = IB_SEND_SIGNALED;

	ret = ib_post_send(ch->qp, &wr, &bad_wr);

out:
	if (ret < 0) {
		atomic_inc(&ch->sq_wr_avail);
		atomic_dec(&ch->req_lim);
	}
	return ret;
}

/**
 * srpt_get_desc_tbl() - Parse the data descriptors of an SRP_CMD request.
 * @ioctx: Pointer to the I/O context associated with the request.
 * @srp_cmd: Pointer to the SRP_CMD request data.
 * @dir: Pointer to the variable to which the transfer direction will be
 *   written.
 * @data_len: Pointer to the variable to which the total data length of all
 *   descriptors in the SRP_CMD request will be written.
 *
 * This function initializes ioctx->nrbuf and ioctx->r_bufs.
 *
 * Returns -EINVAL when the SRP_CMD request contains inconsistent descriptors;
 * -ENOMEM when memory allocation fails and zero upon success.
 */
static int srpt_get_desc_tbl(struct srpt_send_ioctx *ioctx,
			     struct srp_cmd *srp_cmd,
			     enum dma_data_direction *dir, u64 *data_len)
{
	struct srp_indirect_buf *idb;
	struct srp_direct_buf *db;
	unsigned add_cdb_offset;
	int ret;

	/*
	 * The pointer computations below will on