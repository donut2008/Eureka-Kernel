/* virtual address */
	u32 max_sges;
	u32 head, tail;
	u32 entry_size;
	u32 max_cnt;
	u32 max_wqe_idx;
	u16 dbid;		/* qid, where to ring the doorbell. */
	u32 len;
	dma_addr_t pa;
};

struct ocrdma_srq {
	struct ib_srq ibsrq;
	u8 __iomem *db;
	struct ocrdma_qp_hwq_info rq;
	u64 *rqe_wr_id_tbl;
	u32 *idx_bit_fields;
	u32 bit_fields_len;

	/* provide synchronization to multiple context(s) posting rqe */
	spinlock_t q_lock ____cacheline_aligned;

	struct ocrdma_pd *pd;
	u32 id;
};

struct ocrdma_qp {
	struct ib_qp ibqp;

	u8 __iomem *sq_db;
	struct ocrdma_qp_hwq_info sq;
	struct {
		uint64_t wrid;
		uint16_t dpp_wqe_idx;
		uint16_t dpp_wqe;
		uint8_t  signaled;
		uint8_t  rsvd[3];
	} *wqe_wr_id_tbl;
	u32 max_inline_data;

	/* provide synchronization to multiple context(s) posting wqe, rqe */
	spinlock_t q_lock ____cacheline_aligned;
	struct ocrdma_cq *sq_cq;
	/* list maintained per CQ to flush SQ errors */
	struct list_head sq_entry;

	u8 __iomem *rq_db;
	struct ocrdma_qp_hwq_info rq;
	u64 *rqe_wr_id_tbl;
	struct ocrdma_cq *rq_cq;
	struct ocrdma_srq *srq;
	/* list maintained per CQ to flush RQ errors */
	struct list_head rq_entry;

	enum ocrdma_qp_state state;	/*  QP state */
	int cap_flags;
	u32 max_ord, max_ird;

	u32 id;
	struct ocrdma_pd *pd;

	enum ib_qp_type qp_type;

	int sgid_idx;
	u32 qkey;
	bool dpp_enabled;
	u8 *ird_q_va;
	bool signaled;
};

struct ocrdma_ucontext {
	struct ib_ucontext ibucontext;

	struct list_head mm_head;
	struct mutex mm_list_lock; /* protects list entries of mm type */
	struct ocrdma_pd *cntxt_pd;
	int pd_in_use;

	struct {
		u32 *va;
		dma_addr_t pa;
		u32 len;
	} ah_tbl;
};

struct ocrdma_mm {
	struct {
		u64 phy_addr;
		unsigned long len;
	} key;
	struct list_head entry;
};

static inline struct ocrdma_dev *get_ocrdma_dev(struct ib_device *ibdev)
{
	return container_of(ibdev, struct ocrdma_dev, ibdev);
}

static inline struct ocrdma_ucontext *get_ocrdma_ucontext(struct ib_ucontext
							  *ibucontext)
{
	return container_of(ibucontext, struct ocrdma_ucontext, ibucontext);
}

static inline struct ocrdma_pd *get_ocrdma_pd(struct ib_pd *ibpd)
{
	return container_of(ibpd, struct ocrdma_pd, ibpd);
}

static inline struct ocrdma_cq *get_ocrdma_cq(struct ib_cq *ibcq)
{
	return container_of(ibcq, struct ocrdma_cq, ibcq);
}

static inline struct ocrdma_qp *get_ocrdma_qp(struct ib_qp *ibqp)
{
	return container_of(ibqp, struct ocrdma_qp, ibqp);
}

static inline struct ocrdma_mr *get_ocrdma_mr(struct ib_mr *ibmr)
{
	return container_of(ibmr, struct ocrdma_mr, ibmr);
}

static inline struct ocrdma_ah *get_ocrdma_ah(struct ib_ah *ibah)
{
	return container_of(ibah, struct ocrdma_ah, ibah);
}

static inline struct ocrdma_srq *get_ocrdma_srq(struct ib_srq *ibsrq)
{
	return container_of(ibsrq, struct ocrdma_srq, ibsrq);
}

static inline int is_cqe_valid(struct ocrdma_cq *cq, struct ocrdma_cqe *cqe)
{
	int cqe_valid;
	cqe_valid = le32_to_cpu(cqe->flags_status_srcqpn) & OCRDMA_CQE_VALID;
	return (cqe_valid == cq->phase);
}

static inline int is_cqe_for_sq(struct ocrdma_cqe *cqe)
{
	return (le32_to_cpu(cqe->flags_status_srcqpn) &
		OCRDMA_CQE_QTYPE) ? 0 : 1;
}

static inline int is_cqe_invalidated(struct ocrdma_cqe *cqe)
{
	return (le32_to_cpu(cqe->flags_status_srcqpn) &
		OCRDMA_CQE_INVALIDATE) ? 1 : 0;
}

static inline int is_cqe_imm(struct ocrdma_cqe *cqe)
{
	return (le32_to_cpu(cqe->flags_status_srcqpn) &
		OCRDMA_CQE_IMM) ? 1 : 0;
}

static inline int is_cqe_wr_imm(struct ocrdma_cqe *cqe)
{
	return (le32_to_cpu(cqe->flags_status_srcqpn) &
		OCRDMA_CQE_WRITE_IMM) ? 1 : 0;
}

static inline int ocrdma_resolve_dmac(struct ocrdma_dev *dev,
		struct ib_ah_attr *ah_attr, u8 *mac_addr)
{
	struct in6_addr in6;

	memcpy(&in6, ah_attr->grh.dgid.raw, sizeof(in6));
	if (rdma_is_multicast_addr(&in6))
		rdma_get_mcast_mac(&in6, mac_addr);
	else if (rdma_link_local_addr(&in6))
		rdma_get_ll_mac(&in6, mac_addr);
	else
		memcpy(mac_addr, ah_attr->dmac, ETH_ALEN);
	return 0;
}

static inline char *hca_name(struct ocrdma_dev *dev)
{
	switch (dev->nic_info.pdev->device) {
	case OC_SKH_DEVICE_PF:
	case OC_SKH_DEVICE_VF:
		return OC_NAME_SH;
	default:
		return OC_NAME_UNKNOWN;
	}
}

static inline int ocrdma_get_eq_table_index(struct ocrdma_dev *dev,
		int eqid)
{
	int indx;

	for (indx = 0; indx < dev->eq_cnt; indx++) {
		if (dev->eq_tbl[indx].q.id == eqid)
			return indx;
	}

	return -EINVAL;
}

static inline u8 ocrdma_get_asic_type(struct ocrdma_dev *dev)
{
	if (dev->nic_info.dev_family == 0xF && !dev->asic_id) {
		pci_read_config_dword(
			dev->nic_info.pdev,
			OCRDMA_SLI_ASIC_ID_OFFSET, &dev->asic_id);
	}

	return (dev->asic_id & OCRDMA_SLI_ASIC_GEN_NUM_MASK) >>
				OCRDMA_SLI_ASIC_GEN_NUM_SHIFT;
}

static inline u8 ocrdma_get_pfc_prio(u8 *pfc, u8 prio)
{
	return *(pfc + prio);
}

static inline u8 ocrdma_get_app_prio(u8 *app_prio, u8 prio)
{
	return *(app_prio + prio);
}

static inline u8 ocrdma_is_enabled_and_synced(u32 state)
{	/* May also be used to interpret TC-state, QCN-state
	 * Appl-state and Logical-link-state in future.
	 */
	return (state & OCRDMA_STATE_FLAG_ENABLED) &&
		(state & OCRDMA_STATE_FLAG_SYNC);
}

static inline u8 ocrdma_get_ae_link_state(u32 ae_state)
{
	return ((ae_state & OCRDMA_AE_LSC_LS_MASK) >> OCRDMA_AE_LSC_LS_SHIFT);
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /* This file is part of the Emulex RoCE Device Driver for
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

#ifndef __OCRDMA_AH_H__
#define __OCRDMA_AH_H__

enum {
	OCRDMA_AH_ID_MASK		= 0x3FF,
	OCRDMA_AH_VLAN_VALID_MASK	= 0x01,
	OCRDMA_AH_VLAN_VALID_SHIFT	= 0x1F
};

struct ib_ah *ocrdma_create_ah(struct ib_pd *, struct ib_ah_attr *);
int ocrdma_destroy_ah(struct ib_ah *);
int ocrdma_query_ah(struct ib_ah *, struct ib_ah_attr *);
int ocrdma_modify_ah(struct ib_ah *, struct ib_ah_attr *);

int ocrdma_process_mad(struct ib_device *,
		       int process_mad_flags,
		       u8 port_num,
		       const struct ib_wc *in_wc,
		       const struct ib_grh *in_grh,
		       const struct ib_mad_hdr *in, size_t in_mad_size,
		       struct ib_mad_hdr *out, size_t *out_mad_size,
		       u16 *out_mad_pkey_index);
#endif				/* __OCRDMA_AH_H__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /* This file is part of the Emulex RoCE Device Driver for
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

#ifndef __OCRDMA_HW_H__
#define __OCRDMA_HW_H__

#include "ocrdma_sli.h"

static inline void ocrdma_cpu_to_le32(void *dst, u32 len)
{
#ifdef __BIG_ENDIAN
	int i = 0;
	u32 *src_ptr = dst;
	u32 *dst_ptr = dst;
	for (; i < (len / 4); i++)
		*(dst_ptr + i) = cpu_to_le32p(src_ptr + i);
#endif
}

static inline void ocrdma_le32_to_cpu(void *dst, u32 len)
{
#ifdef __BIG_ENDIAN
	int i = 0;
	u32 *src_ptr = dst;
	u32 *dst_ptr = dst;
	for (; i < (len / sizeof(u32)); i++)
		*(dst_ptr + i) = le32_to_cpu(*(src_ptr + i));
#endif
}

static inline void ocrdma_copy_cpu_to_le32(void *dst, void *src, u32 len)
{
#ifdef __BIG_ENDIAN
	int i = 0;
	u32 *src_ptr = src;
	u32 *dst_ptr = dst;
	for (; i < (len / sizeof(u32)); i++)
		*(dst_ptr + i) = cpu_to_le32p(src_ptr + i);
#else
	memcpy(dst, src, len);
#endif
}

static inline void ocrdma_copy_le32_to_cpu(void *dst, void *src, u32 len)
{
#ifdef __BIG_ENDIAN
	int i = 0;
	u32 *src_ptr = src;
	u32 *dst_ptr = dst;
	for (; i < len / sizeof(u32); i++)
		*(dst_ptr + i) = le32_to_cpu(*(src_ptr + i));
#else
	memcpy(dst, src, len);
#endif
}

static inline u64 ocrdma_get_db_addr(struct ocrdma_dev *dev, u32 pdid)
{
	return dev->nic_info.unmapped_db + (pdid * dev->nic_info.db_page_size);
}

int ocrdma_init_hw(struct ocrdma_dev *);
void ocrdma_cleanup_hw(struct ocrdma_dev *);

enum ib_qp_state get_ibqp_state(enum ocrdma_qp_state qps);
void ocrdma_ring_cq_db(struct ocrdma_dev *, u16 cq_id, bool armed,
		       bool solicited, u16 cqe_popped);

/* verbs specific mailbox commands */
int ocrdma_mbx_get_link_speed(struct ocrdma_dev *dev, u8 *lnk_speed,
			      u8 *lnk_st);
int ocrdma_query_config(struct ocrdma_dev *,
			struct ocrdma_mbx_query_config *config);

int ocrdma_mbx_alloc_pd(struct ocrdma_dev *, struct ocrdma_pd *);
int ocrdma_mbx_dealloc_pd(struct ocrdma_dev *, struct ocrdma_pd *);

int ocrdma_mbx_alloc_lkey(struct ocrdma_dev *, struct ocrdma_hw_mr *hwmr,
			  u32 pd_id, int addr_check);
int ocrdma_mbx_dealloc_lkey(struct ocrdma_dev *, int fmr, u32 lkey);

int ocrdma_reg_mr(struct ocrdma_dev *, struct ocrdma_hw_mr *hwmr,
			u32 pd_id, int acc);
int ocrdma_mbx_create_cq(struct ocrdma_dev *, struct ocrdma_cq *,
				int entries, int dpp_cq, u16 pd_id);
int ocrdma_mbx_destroy_cq(struct ocrdma_dev *, struct ocrdma_cq *);

int ocrdma_mbx_create_qp(struct ocrdma_qp *, struct ib_qp_init_attr *attrs,
			 u8 enable_dpp_cq, u16 dpp_cq_id, u16 *dpp_offset,
			 u16 *dpp_credit_lmt);
int ocrdma_mbx_modify_qp(struct ocrdma_dev *, struct ocrdma_qp *,
			 struct ib_qp_attr *attrs, int attr_mask);
int ocrdma_mbx_query_qp(struct ocrdma_dev *, struct ocrdma_qp *,
			struct ocrdma_qp_params *param);
int ocrdma_mbx_destroy_qp(struct ocrdma_dev *, struct ocrdma_qp *);
int ocrdma_mbx_create_srq(struct ocrdma_dev *, struct ocrdma_srq *,
			  struct ib_srq_init_attr *,
			  struct ocrdma_pd *);
int ocrdma_mbx_modify_srq(struct ocrdma_srq *, struct ib_srq_attr *);
int ocrdma_mbx_query_srq(struct ocrdma_srq *, struct ib_srq_attr *);
int ocrdma_mbx_destroy_srq(struct ocrdma_dev *, struct ocrdma_srq *);

int ocrdma_alloc_av(struct ocrdma_dev *, struct ocrdma_ah *);
int ocrdma_free_av(struct ocrdma_dev *, struct ocrdma_ah *);

int ocrdma_qp_state_change(struct ocrdma_qp *, enum ib_qp_state new_state,
			    enum ib_qp_state *old_ib_state);
bool ocrdma_is_qp_in_sq_flushlist(struct ocrdma_cq *, struct ocrdma_qp *);
bool ocrdma_is_qp_in_rq_flushlist(struct ocrdma_cq *, struct ocrdma_qp *);
void ocrdma_flush_qp(struct ocrdma_qp *);
int ocrdma_get_irq(struct ocrdma_dev *dev, struct ocrdma_eq *eq);

int ocrdma_mbx_rdma_stats(struct ocrdma_dev *, bool reset);
char *port_speed_string(struct ocrdma_dev *dev);
void ocrdma_init_service_level(struct ocrdma_dev *);
void ocrdma_alloc_pd_pool(struct ocrdma_dev *dev);
void ocrdma_free_pd_range(struct ocrdma_dev *dev);
void ocrdma_update_link_state(struct ocrdma_dev *dev, u8 lstate);

#endif				/* __OCRDMA_HW_H__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /* This file is part of the Emulex RoCE Device Driver for
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

#include <linux/module.h>
#include <linux/idr.h>
#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_mad.h>

#include <linux/netdevice.h>
#include <net/addrconf.h>

#include "ocrdma.h"
#include "ocrdma_verbs.h"
#include "ocrdma_ah.h"
#include "be_roce.h"
#include "ocrdma_hw.h"
#include "ocrdma_stats.h"
#include "ocrdma_abi.h"

MODULE_VERSION(OCRDMA_ROCE_DRV_VERSION);
MODULE_DESCRIPTION(OCRDMA_ROCE_DRV_DESC " " OCRDMA_ROCE_DRV_VERSION);
MODULE_AUTHOR("Emulex Corporation");
MODULE_LICENSE("Dual BSD/GPL");

static DEFINE_IDR(ocrdma_dev_id);

void ocrdma_get_guid(struct ocrdma_dev *dev, u8 *guid)
{
	u8 mac_addr[6];

	memcpy(&mac_addr[0], &dev->nic_info.mac_addr[0], ETH_ALEN);
	guid[0] = mac_addr[0] ^ 2;
	guid[1] = mac_addr[1];
	guid[2] = mac_addr[2];
	guid[3] = 0xff;
	guid[4] = 0xfe;
	guid[5] = mac_addr[3];
	guid[6] = mac_addr[4];
	guid[7] = mac_addr[5];
}
static enum rdma_link_layer ocrdma_link_layer(struct ib_device *device,
					      u8 port_num)
{
	return IB_LINK_LAYER_ETHERNET;
}

static int ocrdma_port_immutable(struct ib_device *ibdev, u8 port_num,
			         struct ib_port_immutable *immutable)
{
	struct ib_port_attr attr;
	int err;

	err = ocrdma_query_port(ibdev, port_num, &attr);
	if (err)
		return err;

	immutable->pkey_tbl_len = attr.pkey_tbl_len;
	immutable->gid_tbl_len = attr.gid_tbl_len;
	immutable->core_cap_flags = RDMA_CORE_PORT_IBA_ROCE;
	immutable->max_mad_size = IB_MGMT_MAD_SIZE;

	return 0;
}

static int ocrdma_register_device(struct ocrdma_dev *dev)
{
	strlcpy(dev->ibdev.name, "ocrdma%d", IB_DEVICE_NAME_MAX);
	ocrdma_get_guid(dev, (u8 *)&dev->ibdev.node_guid);
	memcpy(dev->ibdev.node_desc, OCRDMA_NODE_DESC,
	       sizeof(OCRDMA_NODE_DESC));
	dev->ibdev.owner = THIS_MODULE;
	dev->ibdev.uverbs_abi_ver = OCRDMA_ABI_VERSION;
	dev->ibdev.uverbs_cmd_mask =
	    OCRDMA_UVERBS(GET_CONTEXT) |
	    OCRDMA_UVERBS(QUERY_DEVICE) |
	    OCRDMA_UVERBS(QUERY_PORT) |
	    OCRDMA_UVERBS(ALLOC_PD) |
	    OCRDMA_UVERBS(DEALLOC_PD) |
	    OCRDMA_UVERBS(REG_MR) |
	    OCRDMA_UVERBS(DEREG_MR) |
	    OCRDMA_UVERBS(CREATE_COMP_CHANNEL) |
	    OCRDMA_UVERBS(CREATE_CQ) |
	    OCRDMA_UVERBS(RESIZE_CQ) |
	    OCRDMA_UVERBS(DESTROY_CQ) |
	    OCRDMA_UVERBS(REQ_NOTIFY_CQ) |
	    OCRDMA_UVERBS(CREATE_QP) |
	    OCRDMA_UVERBS(MODIFY_QP) |
	    OCRDMA_UVERBS(QUERY_QP) |
	    OCRDMA_UVERBS(DESTROY_QP) |
	    OCRDMA_UVERBS(POLL_CQ) |
	    OCRDMA_UVERBS(POST_SEND) |
	    OCRDMA_UVERBS(POST_RECV);

	dev->ibdev.uverbs_cmd_mask |=
	    OCRDMA_UVERBS(CREATE_AH) |
	     OCRDMA_UVERBS(MODIFY_AH) |
	     OCRDMA_UVERBS(QUERY_AH) |
	     OCRDMA_UVERBS(DESTROY_AH);

	dev->ibdev.node_type = RDMA_NODE_IB_CA;
	dev->ibdev.phys_port_cnt = 1;
	dev->ibdev.num_comp_vectors = dev->eq_cnt;

	/* mandatory verbs. */
	dev->ibdev.query_device = ocrdma_query_device;
	dev->ibdev.query_port = ocrdma_query_port;
	dev->ibdev.modify_port = ocrdma_modify_port;
	dev->ibdev.query_gid = ocrdma_query_gid;
	dev->ibdev.get_netdev = ocrdma_get_netdev;
	dev->ibdev.add_gid = ocrdma_add_gid;
	dev->ibdev.del_gid = ocrdma_del_gid;
	dev->ibdev.get_link_layer = ocrdma_link_layer;
	dev->ibdev.alloc_pd = ocrdma_alloc_pd;
	dev->ibdev.dealloc_pd = ocrdma_dealloc_pd;

	dev->ibdev.create_cq = ocrdma_create_cq;
	dev->ibdev.destroy_cq = ocrdma_destroy_cq;
	dev->ibdev.resize_cq = ocrdma_resize_cq;

	dev->ibdev.create_qp = ocrdma_create_qp;
	dev->ibdev.modify_qp = ocrdma_modify_qp;
	dev->ibdev.query_qp = ocrdma_query_qp;
	dev->ibdev.destroy_qp = ocrdma_destroy_qp;

	dev->ibdev.query_pkey = ocrdma_query_pkey;
	dev->ibdev.create_ah = ocrdma_create_ah;
	dev->ibdev.destroy_ah = ocrdma_destroy_ah;
	dev->ibdev.query_ah = ocrdma_query_ah;
	dev->ibdev.modify_ah = ocrdma_modify_ah;

	dev->ibdev.poll_cq = ocrdma_poll_cq;
	dev->ibdev.post_send = ocrdma_post_send;
	dev->ibdev.post_recv = ocrdma_post_recv;
	dev->ibdev.req_notify_cq = ocrdma_arm_cq;

	dev->ibdev.get_dma_mr = ocrdma_get_dma_mr;
	dev->ibdev.reg_phys_mr = ocrdma_reg_kernel_mr;
	dev->ibdev.dereg_mr = ocrdma_dereg_mr;
	dev->ibdev.reg_user_mr = ocrdma_reg_user_mr;

	dev->ibdev.alloc_mr = ocrdma_alloc_mr;
	dev->ibdev.map_mr_sg = ocrdma_map_mr_sg;

	/* mandatory to support user space verbs consumer. */
	dev->ibdev.alloc_ucontext = ocrdma_alloc_ucontext;
	dev->ibdev.dealloc_ucontext = ocrdma_dealloc_ucontext;
	dev->ibdev.mmap = ocrdma_mmap;
	dev->ibdev.dma_device = &dev->nic_info.pdev->dev;

	dev->ibdev.process_mad = ocrdma_process_mad;
	dev->ibdev.get_port_immutable = ocrdma_port_immutable;

	if (ocrdma_get_asic_type(dev) == OCRDMA_ASIC_GEN_SKH_R) {
		dev->ibdev.uverbs_cmd_mask |=
		     OCRDMA_UVERBS(CREATE_SRQ) |
		     OCRDMA_UVERBS(MODIFY_SRQ) |
		     OCRDMA_UVERBS(QUERY_SRQ) |
		     OCRDMA_UVERBS(DESTROY_SRQ) |
		     OCRDMA_UVERBS(POST_SRQ_RECV);

		dev->ibdev.create_srq = ocrdma_create_srq;
		dev->ibdev.modify_srq = ocrdma_modify_srq;
		dev->ibdev.query_srq = ocrdma_query_srq;
		dev->ibdev.destroy_srq = ocrdma_destroy_srq;
		dev->ibdev.post_srq_recv = ocrdma_post_srq_recv;
	}
	return ib_register_device(&dev->ibdev, NULL);
}

static int ocrdma_alloc_resources(struct ocrdma_dev *dev)
{
	mutex_init(&dev->dev_lock);
	dev->cq_tbl = kzalloc(sizeof(struct ocrdma_cq *) *
			      OCRDMA_MAX_CQ, GFP_KERNEL);
	if (!dev->cq_tbl)
		goto alloc_err;

	if (dev->attr.max_qp) {
		dev->qp_tbl = kzalloc(sizeof(struct ocrdma_qp *) *
				      OCRDMA_MAX_QP, GFP_KERNEL);
		if (!dev->qp_tbl)
			goto alloc_err;
	}

	dev->stag_arr = kzalloc(sizeof(u64) * OCRDMA_MAX_STAG, GFP_KERNEL);
	if (dev->stag_arr == NULL)
		goto alloc_err;

	ocrdma_alloc_pd_pool(dev);

	spin_lock_init(&dev->av_tbl.lock);
	spin_lock_init(&dev->flush_q_lock);
	return 0;
alloc_err:
	pr_err("%s(%d) error.\n", __func__, dev->id);
	return -ENOMEM;
}

static void ocrdma_free_resources(struct ocrdma_dev *dev)
{
	kfree(dev->stag_arr);
	kfree(dev->qp_tbl);
	kfree(dev->cq_tbl);
}

/* OCRDMA sysfs interface */
static ssize_t show_rev(struct device *device, struct device_attribute *attr,
			char *buf)
{
	struct ocrdma_dev *dev = dev_get_drvdata(device);

	return scnprintf(buf, PAGE_SIZE, "0x%x\n", dev->nic_info.pdev->vendor);
}

static ssize_t show_fw_ver(struct device *device, struct device_attribute *attr,
			char *buf)
{
	struct ocrdma_dev *dev = dev_get_drvdata(device);

	return scnprintf(buf, PAGE_SIZE, "%s\n", &dev->attr.fw_ver[0]);
}

static ssize_t show_hca_type(struct device *device,
			     struct device_attribute *attr, char *buf)
{
	struct ocrdma_dev *dev = dev_get_drvdata(device);

	return scnprintf(buf, PAGE_SIZE, "%s\n", &dev->model_number[0]);
}

static DEVICE_ATTR(hw_rev, S_IRUGO, show_rev, NULL);
static DEVICE_ATTR(fw_ver, S_IRUGO, show_fw_ver, NULL);
static DEVICE_ATTR(hca_type, S_IRUGO, show_hca_type, NULL);

static struct device_attribute *ocrdma_attributes[] = {
	&dev_attr_hw_rev,
	&dev_attr_fw_ver,
	&dev_attr_hca_type
};

static void ocrdma_remove_sysfiles(struct ocrdma_dev *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ocrdma_attributes); i++)
		device_remove_file(&dev->ibdev.dev, ocrdma_attributes[i]);
}

static struct ocrdma_dev *ocrdma_add(struct be_dev_info *dev_info)
{
	int status = 0, i;
	u8 lstate = 0;
	struct ocrdma_dev *dev;

	dev = (struct ocrdma_dev *)ib_alloc_device(sizeof(struct ocrdma_dev));
	if (!dev) {
		pr_err("Unable to allocate ib device\n");
		return NULL;
	}
	dev->mbx_cmd = kzalloc(sizeof(struct ocrdma_mqe_emb_cmd), GFP_KERNEL);
	if (!dev->mbx_cmd)
		goto idr_err;

	memcpy(&dev->nic_info, dev_info, sizeof(*dev_info));
	dev->id = idr_alloc(&ocrdma_dev_id, NULL, 0, 0, GFP_KERNEL);
	if (dev->id < 0)
		goto idr_err;

	status = ocrdma_init_hw(dev);
	if (status)
		goto init_err;

	status = ocrdma_alloc_resources(dev);
	if (status)
		goto alloc_err;

	ocrdma_init_service_level(dev);
	status = ocrdma_register_device(dev);
	if (status)
		goto alloc_err;

	/* Query Link state and update */
	status = ocrdma_mbx_get_link_speed(dev, NULL, &lstate);
	if (!status)
		ocrdma_update_link_state(dev, lstate);

	for (i = 0; i < ARRAY_SIZE(ocrdma_attributes); i++)
		if (device_create_file(&dev->ibdev.dev, ocrdma_attributes[i]))
			goto sysfs_err;
	/* Init stats */
	ocrdma_add_port_stats(dev);
	/* Interrupt Moderation */
	INIT_DELAYED_WORK(&dev->eqd_work, ocrdma_eqd_set_task);
	schedule_delayed_work(&dev->eqd_work, msecs_to_jiffies(1000));

	pr_info("%s %s: %s \"%s\