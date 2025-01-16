ats, pcur, "send_bytes",
				convert_to_64bit(tx_stats->send_bytes_lo,
						 tx_stats->send_bytes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "write_bytes",
				convert_to_64bit(tx_stats->write_bytes_lo,
						 tx_stats->write_bytes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "read_req_bytes",
				convert_to_64bit(tx_stats->read_req_bytes_lo,
						 tx_stats->read_req_bytes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "read_rsp_bytes",
				convert_to_64bit(tx_stats->read_rsp_bytes_lo,
						 tx_stats->read_rsp_bytes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "ack_timeouts",
				(u64)tx_stats->ack_timeouts);

	return stats;
}

static u64 ocrdma_sysfs_xmit_pkts(struct ocrdma_dev *dev)
{
	struct ocrdma_rdma_stats_resp *rdma_stats =
		(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_tx_stats *tx_stats = &rdma_stats->tx_stats;

	return (convert_to_64bit(tx_stats->send_pkts_lo,
				 tx_stats->send_pkts_hi) +
	convert_to_64bit(tx_stats->write_pkts_lo, tx_stats->write_pkts_hi) +
	convert_to_64bit(tx_stats->read_pkts_lo, tx_stats->read_pkts_hi) +
	convert_to_64bit(tx_stats->read_rsp_pkts_lo,
			 tx_stats->read_rsp_pkts_hi) +
	convert_to_64bit(tx_stats->ack_pkts_lo, tx_stats->ack_pkts_hi));
}

static u64 ocrdma_sysfs_xmit_data(struct ocrdma_dev *dev)
{
	struct ocrdma_rdma_stats_resp *rdma_stats =
		(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_tx_stats *tx_stats = &rdma_stats->tx_stats;

	return (convert_to_64bit(tx_stats->send_bytes_lo,
				 tx_stats->send_bytes_hi) +
		convert_to_64bit(tx_stats->write_bytes_lo,
				 tx_stats->write_bytes_hi) +
		convert_to_64bit(tx_stats->read_req_bytes_lo,
				 tx_stats->read_req_bytes_hi) +
		convert_to_64bit(tx_stats->read_rsp_bytes_lo,
				 tx_stats->read_rsp_bytes_hi))/4;
}

static char *ocrdma_wqe_stats(struct ocrdma_dev *dev)
{
	char *stats = dev->stats_mem.debugfs_mem, *pcur;
	struct ocrdma_rdma_stats_resp *rdma_stats =
		(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_wqe_stats	*wqe_stats = &rdma_stats->wqe_stats;

	memset(stats, 0, (OCRDMA_MAX_DBGFS_MEM));

	pcur = stats;
	pcur += ocrdma_add_stat(stats, pcur, "large_send_rc_wqes",
		convert_to_64bit(wqe_stats->large_send_rc_wqes_lo,
				 wqe_stats->large_send_rc_wqes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "large_write_rc_wqes",
		convert_to_64bit(wqe_stats->large_write_rc_wqes_lo,
				 wqe_stats->large_write_rc_wqes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "read_wqes",
				convert_to_64bit(wqe_stats->read_wqes_lo,
						 wqe_stats->read_wqes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "frmr_wqes",
				convert_to_64bit(wqe_stats->frmr_wqes_lo,
						 wqe_stats->frmr_wqes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "mw_bind_wqes",
				convert_to_64bit(wqe_stats->mw_bind_wqes_lo,
						 wqe_stats->mw_bind_wqes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "invalidate_wqes",
		convert_to_64bit(wqe_stats->invalidate_wqes_lo,
				 wqe_stats->invalidate_wqes_hi));
	pcur += ocrdma_add_stat(stats, pcur, "dpp_wqe_drops",
				(u64)wqe_stats->dpp_wqe_drops);
	return stats;
}

static char *ocrdma_db_errstats(struct ocrdma_dev *dev)
{
	char *stats = dev->stats_mem.debugfs_mem, *pcur;
	struct ocrdma_rdma_stats_resp *rdma_stats =
		(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_db_err_stats *db_err_stats = &rdma_stats->db_err_stats;

	memset(stats, 0, (OCRDMA_MAX_DBGFS_MEM));

	pcur = stats;
	pcur += ocrdma_add_stat(stats, pcur, "sq_doorbell_errors",
				(u64)db_err_stats->sq_doorbell_errors);
	pcur += ocrdma_add_stat(stats, pcur, "cq_doorbell_errors",
				(u64)db_err_stats->cq_doorbell_errors);
	pcur += ocrdma_add_stat(stats, pcur, "rq_srq_doorbell_errors",
				(u64)db_err_stats->rq_srq_doorbell_errors);
	pcur += ocrdma_add_stat(stats, pcur, "cq_overflow_errors",
				(u64)db_err_stats->cq_overflow_errors);
	return stats;
}

static char *ocrdma_rxqp_errstats(struct ocrdma_dev *dev)
{
	char *stats = dev->stats_mem.debugfs_mem, *pcur;
	struct ocrdma_rdma_stats_resp *rdma_stats =
		(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_rx_qp_err_stats *rx_qp_err_stats =
		 &rdma_stats->rx_qp_err_stats;

	memset(stats, 0, (OCRDMA_MAX_DBGFS_MEM));

	pcur = stats;
	pcur += ocrdma_add_stat(stats, pcur, "nak_invalid_requst_errors",
			(u64)rx_qp_err_stats->nak_invalid_requst_errors);
	pcur += ocrdma_add_stat(stats, pcur, "nak_remote_operation_errors",
			(u64)rx_qp_err_stats->nak_remote_operation_errors);
	pcur += ocrdma_add_stat(stats, pcur, "nak_count_remote_access_errors",
			(u64)rx_qp_err_stats->nak_count_remote_access_errors);
	pcur += ocrdma_add_stat(stats, pcur, "local_length_errors",
			(u64)rx_qp_err_stats->local_length_errors);
	pcur += ocrdma_add_stat(stats, pcur, "local_protection_errors",
			(u64)rx_qp_err_stats->local_protection_errors);
	pcur += ocrdma_add_stat(stats, pcur, "local_qp_operation_errors",
			(u64)rx_qp_err_stats->local_qp_operation_errors);
	return stats;
}

static char *ocrdma_txqp_errstats(struct ocrdma_dev *dev)
{
	char *stats = dev->stats_mem.debugfs_mem, *pcur;
	struct ocrdma_rdma_stats_resp *rdma_stats =
		(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_tx_qp_err_stats *tx_qp_err_stats =
		&rdma_stats->tx_qp_err_stats;

	memset(stats, 0, (OCRDMA_MAX_DBGFS_MEM));

	pcur = stats;
	pcur += ocrdma_add_stat(stats, pcur, "local_length_errors",
			(u64)tx_qp_err_stats->local_length_errors);
	pcur += ocrdma_add_stat(stats, pcur, "local_protection_errors",
			(u64)tx_qp_err_stats->local_protection_errors);
	pcur += ocrdma_add_stat(stats, pcur, "local_qp_operation_errors",
			(u64)tx_qp_err_stats->local_qp_operation_errors);
	pcur += ocrdma_add_stat(stats, pcur, "retry_count_exceeded_errors",
			(u64)tx_qp_err_stats->retry_count_exceeded_errors);
	pcur += ocrdma_add_stat(stats, pcur, "rnr_retry_count_exceeded_errors",
			(u64)tx_qp_err_stats->rnr_retry_count_exceeded_errors);
	return stats;
}

static char *ocrdma_tx_dbg_stats(struct ocrdma_dev *dev)
{
	int i;
	char *pstats = dev->stats_mem.debugfs_mem;
	struct ocrdma_rdma_stats_resp *rdma_stats =
		(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_tx_dbg_stats *tx_dbg_stats =
		&rdma_stats->tx_dbg_stats;

	memset(pstats, 0, (OCRDMA_MAX_DBGFS_MEM));

	for (i = 0; i < 100; i++)
		pstats += snprintf(pstats, 80, "DW[%d] = 0x%x\n", i,
				 tx_dbg_stats->data[i]);

	return dev->stats_mem.debugfs_mem;
}

static char *ocrdma_rx_dbg_stats(struct ocrdma_dev *dev)
{
	int i;
	char *pstats = dev->stats_mem.debugfs_mem;
	struct ocrdma_rdma_stats_resp *rdma_stats =
		(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_rx_dbg_stats *rx_dbg_stats =
		&rdma_stats->rx_dbg_stats;

	memset(pstats, 0, (OCRDMA_MAX_DBGFS_MEM));

	for (i = 0; i < 200; i++)
		pstats += snprintf(pstats, 80, "DW[%d] = 0x%x\n", i,
				 rx_dbg_stats->data[i]);

	return dev->stats_mem.debugfs_mem;
}

static char *ocrdma_driver_dbg_stats(struct ocrdma_dev *dev)
{
	char *stats = dev->stats_mem.debugfs_mem, *pcur;


	memset(stats, 0, (OCRDMA_MAX_DBGFS_MEM));

	pcur = stats;
	pcur += ocrdma_add_stat(stats, pcur, "async_cq_err",
				(u64)(dev->async_err_stats
				[OCRDMA_CQ_ERROR].counter));
	pcur += ocrdma_add_stat(stats, pcur, "async_cq_overrun_err",
				(u64)dev->async_err_stats
				[OCRDMA_CQ_OVERRUN_ERROR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "async_cq_qpcat_err",
				(u64)dev->async_err_stats
				[OCRDMA_CQ_QPCAT_ERROR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "async_qp_access_err",
				(u64)dev->async_err_stats
				[OCRDMA_QP_ACCESS_ERROR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "async_qp_commm_est_evt",
	