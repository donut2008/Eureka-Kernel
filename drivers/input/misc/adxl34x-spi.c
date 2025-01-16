mapped_sg_count = 0;
	}
}

/**
 * srpt_map_sg_to_ib_sge() - Map an SG list to an IB SGE list.
 */
static int srpt_map_sg_to_ib_sge(struct srpt_rdma_ch *ch,
				 struct srpt_send_ioctx *ioctx)
{
	struct ib_device *dev = ch->sport->sdev->device;
	struct se_cmd *cmd;
	struct scatterlist *sg, *sg_orig;
	int sg_cnt;
	enum dma_data_direction dir;
	struct rdma_iu *riu;
	struct srp_direct_buf *db;
	dma_addr_t dma_addr;
	struct ib_sge *sge;
	u64 raddr;
	u32 rsize;
	u32 tsize;
	u32 dma_len;
	int count, nrdma;
	int i, j, k;

	BUG_ON(!ch);
	BUG_ON(!ioctx);
	cmd = &ioctx->cmd;
	dir = cmd->data_direction;
	BUG_ON(dir == DMA_NONE);

	ioctx->sg = sg = sg_orig = cmd->t_data_sg;
	ioctx->sg_cnt = sg_cnt = cmd->t_data_nents;

	count = ib_dma_map_sg(ch->sport->sdev->device, sg, sg_cnt,
			      opposite_dma_dir(dir));
	if (unlikely(!count))
		return -EAGAIN;

	ioctx->mapped_sg_count = count;

	if (ioctx->rdma_ius && ioctx->n_rdma_ius)
		nrdma = ioctx->n_rdma_ius;
	else {
		nrdma = (count + SRPT_DEF_SG_PER_WQE - 1) / SRPT_DEF_SG_PER_WQE
			+ ioctx->n_rbuf;

		ioctx->rdma_ius = kzalloc(nrdma * sizeof *riu, GFP_KERNEL);
		if (!ioctx->rdma_ius)
			goto free_mem;

		ioctx->n_rdma_ius = nrdma;
	}

	db = ioctx->rbufs;
	tsize = cmd->data_length;
	dma_len = ib_sg_dma_len(dev, &sg[0]);
	riu = ioctx->rdma_ius;

	/*
	 * For each remote desc - calculate the #ib_sge.
	 * If #ib_sge < SRPT_DEF_SG_PER_WQE per rdma operation then
	 *      each remote desc rdma_iu is required a rdma wr;
	 * else
	 *      we need to allocate extra rdma_iu to carry extra #ib_sge in
	 *      another rdma wr
	 */
	for (i = 0, j = 0;
	     j < count && i < ioctx->n_rbuf && tsize > 0; ++i, ++riu, ++db) {
		rsize = be32_to_cpu(db->len);
		raddr = be64_to_cpu(db->va);
		riu->raddr = raddr;
		riu->rkey = be32_to_cpu(db->key);
		riu->sge_cnt = 0;

		/* calculate how many sge required for this remote_buf */
		while (rsize > 0 && tsize > 0) {

			if (rsize >= dma_len) {
				tsize -= dma_len;
				rsize -= dma_len;
				raddr += dma_len;

				if (tsize > 0) {
					++j;
					if (j < count) {
						sg = sg_next(sg);
						dma_len = ib_sg_dma_len(
								dev, sg);
					}
				}
			} else {
				tsize -= rsize;
				dma_len -= rsize;
				rsize = 0;
			}

			++riu->sge_cnt;

			if (rsize > 0 && riu->sge_cnt == SRPT_DEF_SG_PER_WQE) {
				++ioctx->n_rdma;
				riu->sge =
				    kmalloc(riu->sge_cnt * sizeof *riu->sge,
					    GFP_KERNEL);
				if (!riu->sge)
					goto free_mem;

				++riu;
				riu->sge_cnt = 0;
				riu->raddr = raddr;
				riu->rkey = be32_to_cpu(db->key);
			}
		}

		++ioctx->n_rdma;
		riu->sge = kmalloc(riu->sge_cnt * sizeof *riu->sge,
				   GFP_KERNEL);
		if (!riu->sge)
			goto free_mem;
	}

	db = ioctx->rbufs;
	tsize = cmd->data_length;
	riu = ioctx->rdma_ius;
	sg = sg_orig;
	dma_len = ib_sg_dma_len(dev, &sg[0]);
	dma_addr = ib_sg_dma_address(dev, &sg[0]);

	/* this second loop is really mapped sg_addres to rdma_iu->ib_sge */
	for (i = 0, j = 0;
	     j < count && i < ioctx->n_rbuf && tsize > 0; ++i, ++riu, ++db) {
		rsize = be32_to_cpu(db->len);
		sge = riu->sge;
		k = 0;

		while (rsize > 0 && tsize > 0) {
			sge->addr =