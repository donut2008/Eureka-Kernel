it_waitqueue_head(&cqp_request->waitq);
		cqp_request->waiting = 0;
		cqp_request->request_done = 0;
		cqp_request->callback = 0;
		init_waitqueue_head(&cqp_request->waitq);
		nes_debug(NES_DBG_CQP, "Got cqp request %p from the available list \n",
				cqp_request);
	} else
		printk(KERN_ERR PFX "%s: Could not allocated a CQP request.\n",
			   __func__);

	return cqp_request;
}

void nes_free_cqp_request(struct nes_device *nesdev,
			  struct nes_cqp_request *cqp_request)
{
	unsigned long flags;

	nes_debug(NES_DBG_CQP, "CQP request %p (opcode 0x%02X) freed.\n",
		  cqp_request,
		  le32_to_cpu(cqp_request->cqp_wqe.wqe_words[NES_CQP_WQE_OPCODE_IDX]) & 0x3f);

	if (cqp_request->dynamic) {
		kfree(cqp_request);
	} else {
		spin_lock_irqsave(&nesdev->cqp.lock, flags);
		list_add_tail(&cqp_request->list, &nesdev->cqp_avail_reqs);
		spin_unlock_irqrestore(&nesdev->cqp.lock, flags);
	}
}

void nes_put_cqp_request(struct nes_device *nesdev,
			 struct nes_cqp_request *cqp_request)
{
	if (atomic_dec_and_test(&cqp_request->refcount))
		nes_free_cqp_request(nesdev, cqp_request);
}


/**
 * nes_post_cqp_request
 */
void nes_post_cqp_request(struct nes_device *nesdev,
			  struct nes_cqp_request *cqp_request)
{
	struct nes_hw_cqp_wqe *cqp_wqe;
	unsigned long flags;
	u32 cqp_head;
	u64 u64temp;
	u32 opcode;
	int ctx_index = NES_CQP_WQE_COMP_CTX_LOW_IDX;

	spin_lock_irqsave(&nesdev->cqp.lock, flags);

	if (((((nesdev->cqp.sq_tail+(nesdev->cqp.sq_size*2))-nesdev->cqp.sq_head) &
			(nesdev->cqp.sq_size - 1)) != 1)
			&& (list_empty(&nesdev->cqp_pending_reqs))) {
		cqp_head = nesdev->cqp.sq_head++;
		nesdev->cqp.sq_head &= nesdev->cqp.sq_size-1;
		cqp_wqe = &nesdev->cqp.sq_vbase[cqp_head];
		memcpy(cqp_wqe, &cqp_request->cqp_wqe, sizeof(*cqp_wqe));
		opcode = le32_to_cpu(cqp_wqe->wqe_words[NES_CQP_WQE_OPCODE_IDX]);
		if ((opcode & NES_CQP_OPCODE_MASK) == NES_CQP_DOWNLOAD_SEGMENT)
			ctx_index = NES_CQP_WQE_DL_COMP_CTX_LOW_IDX;
		barrier();
		u64temp = (unsigned long)cqp_request;
		set_wqe_64bit_value(cqp_wqe->wqe_words, ctx_index, u64temp);
		nes_debug(NES_DBG_CQP, "CQP request (opcode 0x%02X), line 1 = 0x%08X put on CQPs SQ,"
			" request = %p, cqp_head = %u, cqp_tail = %u, cqp_size = %u,"
			" waiting = %d, refcount = %d.\n",
			opcode & NES_CQP_OPCODE_MASK,
			le32_to_cpu(cqp_wqe->wqe_words[NES_CQP_WQE_ID_IDX]), cqp_request,
			nesdev->cqp.sq_head, nesdev->cqp.sq_tail, nesdev->cqp.sq_size,
			cqp_request->waiting, atomic_read(&cqp_request->refcount));

		barrier();

		/* Ring doorbell (1 WQEs) */
		nes_write32(nesdev->regs+NES_WQE_ALLOC, 0x01800000 | nesdev->cqp.qp_id);

		barrier();
	} else {
		nes_debug(NES_DBG_CQP, "CQP request %p (opcode 0x%02X), line 1 = 0x%08X"
				" put on the pending queue.\n",
				cqp_request,
				le32_to_cpu(cqp_request->cqp_wqe.wqe_words[NES_CQP_WQE_OPCODE_IDX])&0x3f,
				le32_to_cpu(cqp_request->cqp_wqe.wqe_words[NES_CQP_WQE_ID_IDX]));
		list_add_tail(&cqp_request->list, &nesdev->cqp_pending_reqs);
	}

	spin_unlock_irqrestore(&nesdev->cqp.lock, flags);

	return;
}

/**
 * nes_arp_table
 */
int nes_arp_table(struct nes_device *nesdev, u32 ip_addr, u8 *mac_addr, u32 action)
{
	struct nes_adapter *nesadapter = nesdev->nesadapter;
	int arp_index;
	int err = 0;
	__be32 tmp_addr;

	for (arp_index = 0; (u32) arp_index < nesadapter->arp_table_size; arp_index++) {
		if (nesadapter->arp_table[arp_index].ip_addr == ip_addr)
			break;
	}

	if (action == NES_ARP_ADD) {
		if (arp_index != nesadapter->arp_table_size) {
			return -1;
		}

		arp_index = 0;
		err = nes_alloc_resource(nesadapter, nesadapter->allocated_arps,
				nesadapter->arp_table_size, (u32 *)&arp_index, &nesadapter->next_arp_index, NES_RESOURCE_ARP);
		if (err) {
			nes_debug(NES_DBG_NETDEV, "nes_alloc_resource returned error = %u\n", err);
			return err;
		}
		nes_debug(NES_DBG_NETDEV, "ADD, arp_index=%d\n", arp_index);

		nesadapter->arp_table[arp_index].ip_addr = ip_addr;
		memcpy(nesadapter->arp_table[arp_index].mac_addr, mac_addr, ETH_ALEN);
		return arp_index;
	}

	/* DELETE or RESOLVE */
	if (arp_index == nesadapter->arp_table_size) {
		tmp_addr = cpu_to_be32(ip_addr);
		nes_debug(NES_DBG_NETDEV, "MAC for %pI4 not in ARP table - cannot %s\n",
			  &tmp_addr, action == NES_ARP_RESOLVE ? "resolve" : "delete");
		return -1;
	}

	if (action == NES_ARP_RESOLVE) {
		nes_debug(NES_DBG_NETDEV, "RESOLVE, arp_index=%d\n", arp_index);
		return arp_index;
	}

	if (action == NES_ARP_DELETE) {
		nes_debug(NES_DBG_NETDEV, "DELETE, arp_index=%d\n", arp_index);
		nesadapter->arp_table[arp_index].ip_addr = 0;
		memset(nesadapter->arp_table[arp_index].mac_addr, 0x00, ETH_ALEN);
		nes_free_resource(nesadapter, nesadapter->allocated_arps, arp_index);
		return arp_index;
	}

	return -1;
}


/**
 * nes_mh_fix
 */
void nes_mh_fix(unsigned long parm)
{
	unsigned long flags;
	struct nes_device *nesdev = (struct nes_device *)parm;
	struct nes_adapter *nesadapter = nesdev->nesadapter;
	struct nes_vnic *nesvnic;
	u32 used_chunks_tx;
	u32 temp_used_chunks_tx;
	u32 temp_last_used_chunks_tx;
	u32 used_chunks_mask;
	u32 mac_tx_frames_low;
	u32 mac_tx_frames_high;
	u32 mac_tx_pauses;
	u32 serdes_status;
	u32 reset_value;
	u32 tx_control;
	u32 tx_config;
	u32 tx_pause_quanta;
	u32 rx_control;
	u32 rx_config;
	u32 mac_exact_match;
	u32 mpp_debug;
	u32 i=0;
	u32 chunks_tx_progress = 0;

	spin_lock_irqsave(&nesadapter->phy_lock, flags);
	if ((nesadapter->mac_sw_state[0] != NES_MAC_SW_IDLE) || (nesadapter->mac_link_down[0])) {
		spin_unlock_irqrestore(&nesadapter->phy_lock, flags);
		goto no_mh_work;
	}
	nesadapter->mac_sw_state[0] = NES_MAC_SW_MH;
	spin_unlock_irqrestore(&nesadapter->phy_lock, flags);
	do {
		mac_tx_frames_low = nes_read_indexed(nesdev, NES_IDX_MAC_TX_FRAMES_LOW);
		mac_tx_frames_high = nes_read_indexed(nesdev, NES_IDX_MAC_TX_FRAMES_HIGH);
		mac_tx_pauses = nes_read_indexed(nesdev, NES_IDX_MAC_TX_PAUSE_FRAMES);
		used_chunks_tx = nes_read_indexed(nesdev, NES_IDX_USED_CHUNKS_TX);
		nesdev->mac_pause_frames_sent += mac_tx_pauses;
		used_chunks_mask = 0;
		temp_used_chunks_tx = used_chunks_tx;
		temp_last_used_chunks_tx = nesdev->last_used_chunks_tx;

		if (nesdev->netdev[0]) {
			nesvnic = netdev_priv(nesdev->netdev[0]);
		} else {
			break;
		}

		for (i=0; i<4; i++) {
			used_chunks_mask <<= 8;
			if (nesvnic->qp_nic_index[i] != 0xff) {
				used_chunks_mask |= 0xff;
				if ((temp_used_chunks_tx&0xff)<(temp_last_used_chunks_tx&0xff)) {
					chunks_tx_progress = 1;
				}
			}
			temp_used_chunks_tx >>= 8;
			temp_last_used_chunks_tx >>= 8;
		}
		if ((mac_tx_frames_low) || (mac_tx_frames_high) ||
			(!(used_chunks_tx&used_chunks_mask)) ||
			(!(nesdev->last_used_chunks_tx&used_chunks_mask)) ||
			(chunks_tx_progress) ) {
			nesdev->last_used_chunks_tx = used_chunks_tx;
			break;
		}
		nesdev->last_used_chunks_tx = used_chunks_tx;
		barrier();

		nes_write_indexed(nesdev, NES_IDX_MAC_TX_CONTROL, 0x00000005);
		mh_pauses_sent++;
		mac_tx_pauses = nes_read_indexed(nesdev, NES_IDX_MAC_TX_PAUSE_FRAMES);
		if (mac_tx_pauses) {
			nesdev->mac_pause_frames_sent += mac_tx_pauses;
			break;
		}

		tx_control = nes_read_indexed(nesdev, NES_IDX_MAC_TX_CONTROL);
		tx_config = nes_read_indexed(nesdev, NES_IDX_MAC_TX_CONFIG);
		tx_pause_quanta = nes_read_indexed(nesdev, NES_IDX_MAC_TX_PAUSE_QUANTA);
		rx_control = nes_read_indexed(nesdev, NES_IDX_MAC_RX_CONTROL);
		rx_config = nes_read_indexed(nesdev, NES_IDX_MAC_RX_CONFIG);
		mac_exact_match = nes_read_indexed(nesdev, NES_IDX_MAC_EXACT_MATCH_BOTTOM);
		mpp_debug = nes_read_indexed(nesdev, NES_IDX_MPP_DEBUG);

		/* one last ditch effort to avoid a false positive */
		mac_tx_pauses = nes_read_indexed(nesdev, NES_IDX_MAC_TX_PAUSE_FRAMES);
		if (mac_tx_pauses) {
			nesdev->last_mac_tx_pauses = nesdev->mac_pause_frames_sent;
			nes_debug(NES_DBG_HW, "failsafe caught slow outbound pause\n");
			break;
		}
		mh_detected++;

		nes_write_indexed(nesdev, NES_IDX_MAC_TX_CONTROL, 0x00000000);
		nes_write_indexed(nesdev, NES_IDX_MAC_TX_CONFIG, 0x00000000);
		reset_value = nes_read32(nesdev->regs+NES_SOFTWARE_RESET);

		nes_write32(nesdev->regs+NES_SOFTWARE_RESET, reset_value | 0x0000001d);

		while (((nes_read32(nesdev->regs+NES_SOFTWARE_RESET)
				& 0x00000040) != 0x00000040) && (i++ < 5000)) {
			/* mdelay(1); */
		}

		nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_COMMON_CONTROL0, 0x00000008);
		serdes_status = nes_read_indexed(nesdev, NES_IDX_ETH_SERDES_COMMON_STATUS0);

		nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_TX_EMP0, 0x000bdef7);
		nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_TX_DRIVE0, 0x9ce73000);
		nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_RX_MODE0, 0x0ff00000);
		nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_RX_SIGDET0, 0x00000000);
		nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_BYPASS0, 0x00000000);
		nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_LOOPBACK_CONTROL0, 0x00000000);
		if (nesadapter->OneG_Mode) {
			nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_RX_EQ_CONTROL0, 0xf0182222);
		} else {
			nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_RX_EQ_CONTROL0, 0xf0042222);
		}
		serdes_status = nes_read_indexed(nesdev, NES_IDX_ETH_SERDES_RX_EQ_STATUS0);
		nes_write_indexed(nesdev, NES_IDX_ETH_SERDES_CDR_CONTROL0, 0x000000ff);

		nes_write_indexed(nesdev, NES_IDX_MAC_TX_CONTROL, tx_control);
		nes_write_indexed(nesdev, NES_IDX_MAC_TX_CONFIG, tx_config);
		nes_write_indexed(nesdev, NES_IDX_MAC_TX_PAUSE_QUANTA, tx_pause_quanta);
		nes_write_indexed(nesdev, NES_IDX_MAC_RX_CONTROL, rx_control);
		nes_write_indexed(nesdev, NES_IDX_MAC_RX_CONFIG, rx_config);
		nes_write_indexed(nesdev, NES_IDX_MAC_EXACT_MATCH_BOTTOM, mac_exact_match);
		nes_write_indexed(nesdev, NES_IDX_MPP_DEBUG, mpp_debug);

	} while (0);

	nesadapter->mac_sw_state[0] = NES_MAC_SW_IDLE;
no_mh_work:
	nesdev->nesadapter->mh_timer.expires = jiffies + (HZ/5);
	add_timer(&nesdev->nesadapter->mh_timer);
}

/**
 * nes_clc
 */
void nes_clc(unsigned long parm)
{
	unsigned long flags;
	struct nes_device *nesdev = (struct nes_device *)parm;
	struct nes_adapter *nesadapter = nesdev->nesadapter;

	spin_lock_irqsave(&nesadapter->phy_lock, flags);
    nesadapter->link_interrupt_count[0] = 0;
    nesadapter->link_interrupt_count[1] = 0;
    nesadapter->link_interrupt_count[2] = 0;
    nesadapter->link_interrupt_count[3] = 0;
	spin_unlock_irqrestore(&nesadapter->phy_lock, flags);

	nesadapter->lc_timer.expires = jiffies + 3600 * HZ;  /* 1 hour */
	add_timer(&nesadapter->lc_timer);
}


/**
 * nes_dump_mem
 */
void nes_dump_mem(unsigned int dump_debug_level, void *addr, int length)
{
	char  xlate[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'a', 'b', 'c', 'd', 'e', 'f'};
	char  *ptr;
	char  hex_buf[80];
	char  ascii_buf[20];
	int   num_char;
	int   num_ascii;
	int   num_hex;

	if (!(nes_debug_level & dump_debug_level)) {
		return;
	}

	ptr = addr;
	if (length > 0x100) {
		nes_debug(dump_debug_level, "Length truncated from %x to %x\n", length, 0x100);
		length = 0x100;
	}
	nes_debug(dump_debug_level, "Address=0x%p, length=0x%x (%d)\n", ptr, length, length);

	memset(ascii_buf, 0, 20);
	memset(hex_buf, 0, 80);

	num_ascii = 0;
	num_hex = 0;
	for (num_char = 0; num_char < length; num_char++) {
		if (num_ascii == 8) {
			ascii_buf[num_ascii++] = ' ';
			hex_buf[num_hex++] = '-';
			hex_buf[num_hex++] = ' ';
		}

		if (*ptr < 0x20 || *ptr > 0x7e)
			ascii_buf[num_ascii++] = '.';
		else
			ascii_buf[num_ascii++] = *ptr;
		hex_buf[num_hex++] = xlate[((*ptr & 0xf0) >> 4)];
		hex_buf[num_hex++] = xlate[*ptr & 0x0f];
		hex_buf[num_hex++] = ' ';
		ptr++;

		if (num_ascii >= 17) {
			/* output line and reset */
			nes_debug(dump_debug_level, "   %s |  %s\n", hex_buf, ascii_buf);
			memset(ascii_buf, 0, 20);
			memset(hex_buf, 0, 80);
			num_ascii = 0;
			num_hex = 0;
		}
	}

	/* output the rest */
	if (num_ascii) {
		while (num_ascii < 17) {
			if (num_ascii == 8) {
				hex_buf[num_hex++] = ' ';
				hex_buf[num_hex++] = ' ';
			}
			hex_buf[num_hex++] = ' ';
			hex_buf[num_hex++] = ' ';
			hex_buf[num_hex++] = ' ';
			num_ascii++;
		}

		nes_debug(dump_debug_level, "   %s |  %s\n", hex_buf, ascii_buf);
	}
}
                                                                                                                                                                                                                         /* This file is part of the Emulex RoCE Device Driver for
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

#ifndef __OCRDMA_H__
#define __OCRDMA_H__

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/pci.h>

#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_addr.h>

#include <be_roce.h>
#include "ocrdma_sli.h"

#define OCRDMA_ROCE_DRV_VERSION "11.0.0.0"

#define OCRDMA_ROCE_DRV_DESC "Emulex OneConnect RoCE Driver"
#define OCRDMA_NODE_DESC "Emulex OneConnect RoCE HCA"

#define OC_NAME_SH	OCRDMA_NODE_DESC "(Skyhawk)"
#define OC_NAME_UNKNOWN OCRDMA_NODE_DESC "(Unknown)"

#define OC_SKH_DEVICE_PF 0x720
#define OC_SKH_DEVICE_VF 0x728
#define OCRDMA_MAX_AH 512

#define OCRDMA_UVERBS(CMD_NAME) (1ull << IB_USER_VERBS_CMD_##CMD_NAME)

#define convert_to_64bit(lo, hi) ((u64)hi << 32 | (u64)lo)
#define EQ_INTR_PER_SEC_THRSH_HI 150000
#define EQ_INTR_PER_SEC_THRSH_LOW 100000
#define EQ_AIC_MAX_EQD 20
#define EQ_AIC_MIN_EQD 0

void ocrdma_eqd_set_task(struct work_struct *work);

struct ocrdma_dev_attr {
	u8 fw_ver[32];
	u32 vendor_id;
	u32 device_id;
	u16 max_pd;
	u16 max_dpp_pds;
	u16 max_cq;
	u16 max_cqe;
	u16 max_qp;
	u16 max_wqe;
	u16 max_rqe;
	u16 max_srq;
	u32 max_inline_data;
	int max_send_sge;
	int max_recv_sge;
	int max_srq_sge;
	int max_rdma_sge;
	int max_mr;
	u64 max_mr_size;
	u32 max_num_mr_pbl;
	int max_mw;
	int max_fmr;
	int max_map_per_fmr;
	int max_pages_per_frmr;
	u16 max_ord_per_qp;
	u16 max_ird_per_qp;

	int device_cap_flags;
	u8 cq_overflow_detect;
	u8 srq_supported;

	u32 wqe_size;
	u32 rqe_size;
	u32 ird_page_size;
	u8 local_ca_ack_delay;
	u8 ird;
	u8 num_ird_pages;
};

struct ocrdma_dma_mem {
	void *va;
	dma_addr_t pa;
	u32 size;
};

struct ocrdma_pbl {
	void *va;
	dma_addr_t pa;
};

struct ocrdma_queue_info {
	void *va;
	dma_addr_t dma;
	u32 size;
	u16 len;
	u16 entry_size;		/* Size of an element in the queue */
	u16 id;			/* qid, where to ring the doorbell. */
	u16 head, tail;
	bool created;
};

struct ocrdma_aic_obj {         /* Adaptive interrupt coalescing (AIC) info */
	u32 prev_eqd;
	u64 eq_intr_cnt;
	u64 prev_eq_intr_cnt;
};

struct ocrdma_eq {
	struct ocrdma_queue_info q;
	u32 vector;
	int cq_cnt;
	struct ocrdma_dev *dev;
	char irq_name[32];
	struct ocrdma_aic_obj aic_obj;
};

struct ocrdma_mq {
	struct ocrdma_queue_info sq;
	struct ocrdma_queue_info cq;
	bool rearm_cq;
};

struct mqe_ctx {
	struct mutex lock; /* for serializing mailbox commands on MQ */
	wait_queue_head_t cmd_wait;
	u32 tag;
	u16 cqe_status;
	u16 ext_status;
	bool cmd_done;
	bool fw_error_state;
};

struct ocrdma_hw_mr {
	u32 lkey;
	u8 fr_mr;
	u8 remote_atomic;
	u8 remote_rd;
	u8 remote_wr;
	u8 local_rd;
	u8 local_wr;
	u8 mw_bind;
	u8 rsvd;
	u64 len;
	struct ocrdma_pbl *pbl_table;
	u32 num_pbls;
	u32 num_pbes;
	u32 pbl_size;
	u32 pbe_size;
	u64 fbo;
	u64 va;
};

struct ocrdma_mr {
	struct ib_mr ibmr;
	struct ib_umem *umem;
	struct ocrdma_hw_mr hwmr;
	u64 *pages;
	u32 npages;
};

struct ocrdma_stats {
	u8 type;
	struct ocrdma_dev *dev;
};

struct ocrdma_pd_resource_mgr {
	u32 pd_norm_start;
	u16 pd_norm_count;
	u16 pd_norm_thrsh;
	u16 max_normal_pd;
	u32 pd_dpp_start;
	u16 pd_dpp_count;
	u16 pd_dpp_thrsh;
	u16 max_dpp_pd;
	u16 dpp_page_index;
	unsigned long *pd_norm_bitmap;
	unsigned long *pd_dpp_bitmap;
	bool pd_prealloc_valid;
};

struct stats_mem {
	struct ocrdma_mqe mqe;
	void *va;
	dma_addr_t pa;
	u32 size;
	char *debugfs_mem;
};

struct phy_info {
	u16 auto_speeds_supported;
	u16 fixed_speeds_supported;
	u16 phy_type;
	u16 interface_type;
};

enum ocrdma_flags {
	OCRDMA_FLAGS_LINK_STATUS_INIT = 0x01
};

struct ocrdma_dev {
	struct ib_device ibdev;
	struct ocrdma_dev_attr attr;

	struct mutex dev_lock; /* provides syncronise access to device data */
	spinlock_t flush_q_lock ____cacheline_aligned;

	struct ocrdma_cq **cq_tbl;
	struct ocrdma_qp **qp_tbl;

	struct ocrdma_eq *eq_tbl;
	int eq_cnt;
	struct delayed_work eqd_work;
	u16 base_eqid;
	u16 max_eq;

	/* provided synchronization to sgid table for
	 * updating gid entries triggered by notifier.
	 */
	spinlock_t sgid_lock;

	int gsi_qp_created;
	struct ocrdma_cq *gsi_sqcq;
	struct ocrdma_cq *gsi_rqcq;

	struct {
		struct ocrdma_av *va;
		dma_addr_t pa;
		u32 size;
		u32 num_ah;
		/* provide synchronization for av
		 * entry allocations.
		 */
		spinlock_t lock;
		u32 ahid;
		struct ocrdma_pbl pbl;
	} av_tbl;

	void *mbx_cmd;
	struct ocrdma_mq mq;
	struct mqe_ctx mqe_ctx;

	struct be_dev_info nic_info;
	struct phy_info phy;
	char model_number[32];
	u32 hba_port_num;

	struct list_head entry;
	int id;
	u64 *stag_arr;
	u8 sl; /* service level */
	bool pfc_state;
	atomic_t update_sl;
	u16 pvid;
	u32 asic_id;
	u32 flags;

	ulong last_stats_time;
	struct mutex stats_lock; /* provide synch for debugfs operations */
	struct stats_mem stats_mem;
	struct ocrdma_stats rsrc_stats;
	struct ocrdma_stats rx_stats;
	struct ocrdma_stats wqe_st