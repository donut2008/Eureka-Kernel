counter);
	pcur += ocrdma_add_stat(stats, pcur, "async_qp_last_wqe_evt",
				(u64)dev->async_err_stats
				[OCRDMA_QP_LAST_WQE_EVENT].counter);

	pcur += ocrdma_add_stat(stats, pcur, "cqe_loc_len_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_LOC_LEN_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_loc_qp_op_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_LOC_QP_OP_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_loc_eec_op_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_LOC_EEC_OP_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_loc_prot_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_LOC_PROT_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_wr_flush_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_WR_FLUSH_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_mw_bind_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_MW_BIND_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_bad_resp_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_BAD_RESP_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_loc_access_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_LOC_ACCESS_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_rem_inv_req_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_REM_INV_REQ_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_rem_access_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_REM_ACCESS_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_rem_op_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_REM_OP_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_retry_exc_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_RETRY_EXC_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_rnr_retry_exc_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_RNR_RETRY_EXC_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_loc_rdd_viol_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_LOC_RDD_VIOL_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_rem_inv_rd_req_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_REM_INV_RD_REQ_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_rem_abort_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_REM_ABORT_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_inv_eecn_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_INV_EECN_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_inv_eec_state_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_INV_EEC_STATE_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_fatal_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_FATAL_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_resp_timeout_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_RESP_TIMEOUT_ERR].counter);
	pcur += ocrdma_add_stat(stats, pcur, "cqe_general_err",
				(u64)dev->cqe_err_stats
				[OCRDMA_CQE_GENERAL_ERR].counter);
	return stats;
}

static void ocrdma_update_stats(struct ocrdma_dev *dev)
{
	ulong now = jiffies, secs;
	int status = 0;
	struct ocrdma_rdma_stats_resp *rdma_stats =
		      (struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_rsrc_stats *rsrc_stats = &rdma_stats->act_rsrc_stats;

	secs = jiffies_to_msecs(now - dev->last_stats_time) / 1000U;
	if (secs) {
		/* update */
		status = ocrdma_mbx_rdma_stats(dev, false);
		if (status)
			pr_err("%s: stats mbox failed with status = %d\n",
			       __func__, status);
		/* Update PD counters from PD resource manager */
		if (dev->pd_mgr->pd_prealloc_valid) {
			rsrc_stats->dpp_pds = dev->pd_mgr->pd_dpp_count;
			rsrc_stats->non_dpp_pds = dev->pd_mgr->pd_norm_count;
			/* Threshold stata*/
			rsrc_stats = &rdma_stats->th_rsrc_stats;
			rsrc_stats->dpp_pds = dev->pd_mgr->pd_dpp_thrsh;
			rsrc_stats->non_dpp_pds = dev->pd_mgr->pd_norm_thrsh;
		}
		dev->last_stats_time = jiffies;
	}
}

static ssize_t ocrdma_dbgfs_ops_write(struct file *filp,
					const char __user *buffer,
					size_t count, loff_t *ppos)
{
	char tmp_str[32];
	long reset;
	int status = 0;
	struct ocrdma_stats *pstats = filp->private_data;
	struct ocrdma_dev *dev = pstats->dev;

	if (*ppos != 0 || count == 0 || count > sizeof(tmp_str))
		goto err;

	if (copy_from_user(tmp_str, buffer, count))
		goto err;

	tmp_str[count-1] = '\0';
	if (kstrtol(tmp_str, 10, &reset))
		goto err;

	switch (pstats->type) {
	case OCRDMA_RESET_STATS:
		if (reset) {
			status = ocrdma_mbx_rdma_stats(dev, true);
			if (status) {
				pr_err("Failed to reset stats = %d", status);
				goto err;
			}
		}
		break;
	default:
		goto err;
	}

	return count;
err:
	return -EFAULT;
}

int ocrdma_pma_counters(struct ocrdma_dev *dev,
			struct ib_mad *out_mad)
{
	struct ib_pma_portcounters *pma_cnt;

	memset(out_mad->data, 0, sizeof out_mad->data);
	pma_cnt = (void *)(out_mad->data + 40);
	ocrdma_update_stats(dev);

	pma_cnt->port_xmit_data    = cpu_to_be32(ocrdma_sysfs_xmit_data(dev));
	pma_cnt->port_rcv_data     = cpu_to_be32(ocrdma_sysfs_rcv_data(dev));
	pma_cnt->port_xmit_packets = cpu_to_be32(ocrdma_sysfs_xmit_pkts(dev));
	pma_cnt->port_rcv_packets  = cpu_to_be32(ocrdma_sysfs_rcv_pkts(dev));
	return 0;
}

static ssize_t ocrdma_dbgfs_ops_read(struct file *filp, char __user *buffer,
					size_t usr_buf_len, loff_t *ppos)
{
	struct ocrdma_stats *pstats = filp->private_data;
	struct ocrdma_dev *dev = pstats->dev;
	ssize_t status = 0;
	char *data = NULL;

	/* No partial reads */
	if (*ppos != 0)
		return 0;

	mutex_lock(&dev->stats_lock);

	ocrdma_update_stats(dev);

	switch (pstats->type) {
	case OCRDMA_RSRC_STATS:
		data = ocrdma_resource_stats(dev);
		break;
	case OCRDMA_RXSTATS:
		data = ocrdma_rx_stats(dev);
		break;
	case OCRDMA_WQESTATS:
		data = ocrdma_wqe_stats(dev);
		break;
	case OCRDMA_TXSTATS:
		data = ocrdma_tx_stats(dev);
		break;
	case OCRDMA_DB_ERRSTATS:
		data = ocrdma_db_errstats(dev);
		break;
	case OCRDMA_RXQP_ERRSTATS:
		data = ocrdma_rxqp_errstats(dev);
		break;
	case OCRDMA_TXQP_ERRSTATS:
		data = ocrdma_txqp_errstats(dev);
		break;
	case OCRDMA_TX_DBG_STATS:
		data = ocrdma_tx_dbg_stats(dev);
		break;
	case OCRDMA_RX_DBG_STATS:
		data = ocrdma_rx_dbg_stats(dev);
		break;
	case OCRDMA_DRV_STATS:
		data = ocrdma_driver_dbg_stats(dev);
		break;

	default:
		status = -EFAULT;
		goto exit;
	}

	if (usr_buf_len < strlen(data)) {
		status = -ENOSPC;
		goto exit;
	}

	status = simple_read_from_buffer(buffer, usr_buf_len, ppos, data,
					 strlen(data));
exit:
	mutex_unlock(&dev->stats_lock);
	return status;
}

static const struct file_operations ocrdma_dbg_ops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ocrdma_dbgfs_ops_read,
	.write = ocrdma_dbgfs_ops_write,
};

void ocrdma_add_port_stats(struct ocrdma_dev *dev)
{
	if (!ocrdma_dbgfs_dir)
		return;

	/* Create post stats base dir */
	dev->dir = debugfs_create_dir(dev->ibdev.name, ocrdma_dbgfs_dir);
	if (!dev->dir)
		goto err;

	dev->rsrc_stats.type = OCRDMA_RSRC_STATS;
	dev->rsrc_stats.dev = dev;
	if (!debugfs_create_file("resource_stats", S_IRUSR, dev->dir,
				 &dev->rsrc_stats, &ocrdma_dbg_ops))
		goto err;

	dev->rx_stats.type = OCRDMA_RXSTATS;
	dev->rx_stats.dev = dev;
	if (!debugfs_create_file("rx_stats", S_IRUSR, dev->dir,
				 &dev->rx_stats, &ocrdma_dbg_ops))
		goto err;

	dev->wqe_stats.type = OCRDMA_WQESTATS;
	dev->wqe_stats.dev = dev;
	if (!debugfs_create_file("wqe_stats", S_IRUSR, dev->dir,
				 &dev->wqe_stats, &ocrdma_dbg_ops))
		goto err;

	dev->tx_stats.type = OCRDMA_TXSTATS;
	dev->tx_stats.dev = dev;
	if (!debugfs_create_file("tx_stats", S_IRUSR, dev->dir,
				 &dev->tx_stats, &ocrdma_dbg_ops))
		goto err;

	dev->db_err_stats.type = OCRDMA_DB_ERRSTATS;
	dev->db_err_stats.dev = dev;
	if (!debugfs_create_file("db_err_stats", S_IRUSR, dev->dir,
				 &dev->db_err_stats, &ocrdma_dbg_ops))
		goto err;


	dev->tx_qp_err_stats.type = OCRDMA_TXQP_ERRSTATS;
	dev->tx_qp_err_stats.dev = dev;
	if (!debugfs_create_file("tx_qp_err_stats", S_IRUSR, dev->dir,
				 &dev->tx_qp_err_stats, &ocrdma_dbg_ops))
		goto err;

	dev->rx_qp_err_stats.type = OCRDMA_RXQP_ERRSTATS;
	dev->rx_qp_err_stats.dev = dev;
	if (!debugfs_create_file("rx_qp_err_stats", S_IRUSR, dev->dir,
				 &dev->rx_qp_err_stats, &ocrdma_dbg_ops))
		goto err;


	dev->tx_dbg_stats.type = OCRDMA_TX_DBG_STATS;
	dev->tx_dbg_stats.dev = dev;
	if (!debugfs_create_file("tx_dbg_stats", S_IRUSR, dev->dir,
				 &dev->tx_dbg_stats, &ocrdma_dbg_ops))
		goto err;

	dev->rx_dbg_stats.type = OCRDMA_RX_DBG_STATS;
	dev->rx_dbg_stats.dev = dev;
	if (!debugfs_create_file("rx_dbg_stats", S_IRUSR, dev->dir,
				 &dev->rx_dbg_stats, &ocrdma_dbg_ops))
		goto err;

	dev->driver_stats.type = OCRDMA_DRV_STATS;
	dev->driver_stats.dev = dev;
	if (!debugfs_create_file("driver_dbg_stats", S_IRUSR, dev->dir,
					&dev->driver_stats, &ocrdma_dbg_ops))
		goto err;

	dev->reset_stats.type = OCRDMA_RESET_STATS;
	dev->reset_stats.dev = dev;
	if (!debugfs_create_file("reset_stats", 0200, dev->dir,
				&dev->reset_stats, &ocrdma_dbg_ops))
		goto err;

	/* Now create dma_mem for stats mbx command */
	if (!ocrdma_alloc_stats_mem(dev))
		goto err;

	mutex_init(&dev->stats_lock);

	return;
err:
	ocrdma_release_stats_mem(dev);
	debugfs_remove_recursive(dev->dir);
	dev->dir = NULL;
}

void ocrdma_rem_port_stats(struct ocrdma_dev *dev)
{
	if (!dev->dir)
		return;
	debugfs_remove(dev->dir);
	mutex_destroy(&dev->stats_lock);
	ocrdma_release_stats_mem(dev);
}

void ocrdma_init_debugfs(void)
{
	/* Create base dir in debugfs root dir */
	ocrdma_dbgfs_dir = debugfs_create_dir("ocrdma", NULL);
}

void ocrdma_rem_debugfs(void)
{
	debugfs_remove_recursive(ocrdma_dbgfs_dir);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /* This file is part of the Emulex RoCE Device Driver for
 * RoCE (RDMA over Converged Ethernet) adapters.
 * Copyright (C) 2012-2015 Emulex. All rights reserved.
 * EMULEX and SLI are trademarks of Emulex.
 * www.emulex.com
 *
 * This software is available to you under a choice of one of two licenses.
 * You may choose to be licensed under the terms of the GNU General Public
 * License (GPL) Version 2, available from the file COPYING in the main
 * directory of this source tree, or the BSD license below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contact Information:
 * linux-drivers@emulex.com
 *
 * Emulex
 * 3333 Susan Street
 * Costa Mesa, CA 92626
 */

#ifndef __OCRDMA_STATS_H__
#define __OCRDMA_STATS_H__

#include <linux/debugfs.h>
#include "ocrdma.h"
#include "ocrdma_hw.h"

#define OCRDMA_MAX_DBGFS_MEM 4096

enum OCRDMA_STATS_TYPE {
	OCRDMA_RSRC_STATS,
	OCRDMA_RXSTATS,
	OCRDMA_WQESTATS,
	OCRDMA_TXSTATS,
	OCRDMA_DB_ERRSTATS,
	OCRDMA_RXQP_ERRSTATS,
	OCRDMA_TXQP_ERRSTATS,
	OCRDMA_TX_DBG_STATS,
	OCRDMA_RX_DBG_STATS,
	OCRDMA_DRV_STATS,
	OCRDMA_RESET_STATS
};

void ocrdma_rem_debugfs(void);
void ocrdma_init_debugfs(void);
void ocrdma_rem_port_stats(struct ocrdma_dev *dev);
void ocrdma_add_port_stats(struct ocrdma_dev *dev);
int ocrdma_pma_counters(struct ocrdma_dev *dev,
			struct ib_mad *out_mad);

#endif	/* __OCRDMA_STATS_H__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         