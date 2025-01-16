a_len, data_left, rdma_write_max, va_offset = 0;
	int ret = 0, i, ib_sge_cnt;

	isert_cmd->tx_desc.isert_cmd = isert_cmd;

	offset = wr->iser_ib_op == ISER_IB_RDMA_READ ? cmd->write_data_done : 0;
	ret = isert_map_data_buf(isert_conn, isert_cmd, se_cmd->t_data_sg,
				 se_cmd->t_data_nents, se_cmd->data_length,
				 offset, wr->iser_ib_op, &wr->data);
	if (ret)
		return ret;

	data_left = data->len;
	offset = data->offset;

	ib_sge = kzalloc(sizeof(struct ib_sge) * data->nents, GFP_KERNEL);
	if (!ib_sge) {
		isert_warn("Unable to allocate ib_sge\n");
		ret = -ENOMEM;
		goto unmap_cmd;
	}
	wr->ib_sge = ib_sge;

	wr->rdma_wr_num = DIV_ROUND_UP(data->nents, isert_conn->max_sge);
	wr->rdma_wr = kzalloc(sizeof(struct ib_rdma_wr) * wr->rdma_wr_num,
				GFP_KERNEL);
	if (!wr->rdma_wr) {
		isert_dbg("Unable to allocate wr->rdma_wr\n");
		ret = -ENOMEM;
		goto unmap_cmd;
	}

	wr->isert_cmd = isert_cmd;
	rdma_write_max = isert_conn->max_sge * PAGE_SIZE;

	for (i = 0; i < wr->rdma_wr_num; i++) {
		rdma_wr = &isert_cmd->rdma_wr.rdma_wr[i];
		data_len = min(data_left, rdma_write_max);

		rdma_wr->wr.send_flags = 0;
		if (wr->iser_ib_op == ISER_IB_RDMA_WRITE) {
			rdma_wr->wr.opcode = IB_WR_RDMA_WRITE;
			rdma_wr->remote_addr = isert_cmd->read_va + offset;
			rdma_wr->rkey = isert_cmd->read_stag;
			if (i + 1 == wr->rdma_wr_num)
				rdma_wr->wr.next = &isert_cmd->tx_desc.send_wr;
			else
				rdma_wr->wr.next = &wr->rdma_wr[i + 1].wr;
		} else {
			rdma_wr->wr.opcode = IB_WR_RDMA_READ;
			rdma_wr->remote_addr = isert_cmd->write_va + va_offset;
			rdma_wr->rkey = isert_cmd->write_stag;
			if (i + 1 == wr->rdma_wr_num)
				rdma_wr->wr.send_flags = IB_SEND_SIGNALED;
			else
				rdma_wr->wr.next = &wr->rdma_wr[i + 1].wr;
		}

		ib_sge_cnt = isert_build_rdma_wr(isert_conn, isert_cmd, ib_sge,
					rdma_wr, data_len, offset);
		ib_sge += ib_sge_cnt;

		offset += data_len;
		va_offset += data_len;
		data_left -= data_len;
	}

	return 0;
unmap_cmd:
	isert_unmap_data_buf(isert_conn, data);

	return ret;
}

static inline void
isert_inv_rkey(struct ib_send_wr *inv_wr, struct ib_mr *mr)
{
	u32 rkey;

	memset(inv_wr, 0, sizeof(*inv_wr));
	inv_wr->wr_id = ISER_FASTREG_LI_WRID;
	inv_wr->opcode = IB_WR_LOCAL_INV;
	inv_wr->ex.invalidate_rkey = mr->rkey;

	/* Bump the key */
	rkey = ib_inc_rkey(mr->rkey);
	ib_update_fast_reg_key(mr, rkey);
}

static int
isert_fast_reg_mr(struct isert_conn *isert_conn,
		  struct fast_reg_descriptor *fr_desc,
		  struct isert_data_buf *mem,
		  enum isert_indicator ind,
		  struct ib_sge *sge)
{
	struct isert_device *device = isert_conn->device;
	struct ib_device *ib_dev = device->ib_device;
	struct ib_mr *mr;
	struct ib_reg_wr reg_wr;
	struct ib_send_wr inv_wr, *bad_wr, *wr = NULL;
	int ret, n;

	if (mem->dma_nents == 1) {
		sge->lkey = device->pd->local_dma_lkey;
		sge->addr = ib_sg_dma_address(ib_dev, &mem->sg[0]