32 write_bytes_hi;
	u32 read_req_bytes_lo;
	u32 read_req_bytes_hi;
	u32 read_rsp_bytes_lo;
	u32 read_rsp_bytes_hi;
	u32 ack_timeouts;
	u32 rsvd[5];
};


struct ocrdma_tx_qp_err_stats {
	u32 local_length_errors;
	u32 local_protection_errors;
	u32 local_qp_operation_errors;
	u32 retry_count_exceeded_errors;
	u32 rnr_retry_count_exceeded_errors;
	u32 rsvd[3];
};

struct ocrdma_rx_stats {
	u32 roce_frame_bytes_lo;
	u32 roce_frame_bytes_hi;
	u32 roce_frame_icrc_drops;
	u32 roce_frame_payload_len_drops;
	u32 ud_drops;
	u32 qp1_drops;
	u32 psn_error_request_packets;
	u32 psn_error_resp_packets;
	u32 rnr_nak_timeouts;
	u32 rnr_nak_receives;
	u32 roce_frame_rxmt_drops;
	u32 nak_count_psn_sequence_errors;
	u32 rc_drop_count_lookup_errors;
	u32 rq_rnr_naks;
	u32 srq_rnr_naks;
	u32 roce_frames_lo;
	u32 roce_frames_hi;
	u32 rsvd;
};

struct ocrdma_rx_qp_err_stats {
	u32 nak_invalid_requst_errors;
	u32 nak_remote_operation_errors;
	u32 nak_count_remote_access_errors;
	u32 local_length_errors;
	u32 local_protection_errors;
	u32 local_qp_operation_errors;
	u32 rsvd[2];
};

struct ocrdma_tx_dbg_stats {
	u32 data[100];
};

struct ocrdma_rx_dbg_stats {
	u32 data[200];
};

struct ocrdma_rdma_stats_req {
	struct ocrdma_mbx_hdr hdr;
	u8 reset_stats;
	u8 rsvd[3];
} __packed;

struct ocrdma_rdma_stats_resp {
	struct ocrdma_mbx_hdr hdr;
	struct ocrdma_rsrc_stats act_rsrc_stats;
	struct ocrdma_rsrc_stats th_rsrc_stats;
	struct ocrdma_db_err_stats	db_err_stats;
	struct ocrdma_wqe_stats		wqe_stats;
	struct ocrdma_tx_stats		tx_stats;
	struct ocrdma_tx_qp_err_stats	tx_qp_err_stats;
	struct ocrdma_rx_stats		rx_stats;
	struct ocrdma_rx_qp_err_stats	rx_qp_err_stats;
	struct ocrdma_tx_dbg_stats	tx_dbg_stats;
	struct ocrdma_rx_dbg_stats	rx_dbg_stats;
} __packed;

enum {
	OCRDMA_HBA_ATTRB_EPROM_VER_LO_MASK	= 0xFF,
	OCRDMA_HBA_ATTRB_EPROM_VER_HI_MASK	= 0xFF00,
	OCRDMA_HBA_ATTRB_EPROM_VER_HI_SHIFT	= 0x08,
	OCRDMA_HBA_ATTRB_CDBLEN_MASK		= 0xFFFF,
	OCRDMA_HBA_ATTRB_ASIC_REV_MASK		= 0xFF0000,
	OCRDMA_HBA_ATTRB_ASIC_REV_SHIFT		= 0x10,
	OCRDMA_HBA_ATTRB_GUID0_MASK		= 0xFF000000,
	OCRDMA_HBA_ATTRB_GUID0_SHIFT		= 0x18,
	OCRDMA_HBA_ATTRB_GUID13_MASK		= 0xFF,
	OCRDMA_HBA_ATTRB_GUID14_MASK		= 0xFF00,
	OCRDMA_HBA_ATTRB_GUID14_SHIFT		= 0x08,
	OCRDMA_HBA_ATTRB_GUID15_MASK		= 0xFF0000,
	OCRDMA_HBA_ATTRB_GUID15_SHIFT		= 0x10,
	OCRDMA_HBA_ATTRB_PCNT_MASK		= 0xFF000000,
	OCRDMA_HBA_ATTRB_PCNT_SHIFT		= 0x18,
	OCRDMA_HBA_ATTRB_LDTOUT_MASK		= 0xFFFF,
	OCRDMA_HBA_ATTRB_ISCSI_VER_MASK		= 0xFF0000,
	OCRDMA_HBA_ATTRB_ISCSI_VER_SHIFT	= 0x10,
	OCRDMA_HBA_ATTRB_MFUNC_DEV_MASK		= 0xFF000000,
	OCRDMA_HBA_ATTRB_MFUNC_DEV_SHIFT	= 0x18,
	OCRDMA_HBA_ATTRB_CV_MASK		= 0xFF,
	OCRDMA_HBA_ATTRB_HBA_ST_MASK		= 0xFF00,
	OCRDMA_HBA_ATTRB_HBA_ST_SHIFT		= 0x08,
	OCRDMA_HBA_ATTRB_MAX_DOMS_MASK		= 0xFF0000,
	OCRDMA_HBA_ATTRB_MAX_DOMS_SHIFT		= 0x10,
	OCRDMA_HBA_ATTRB_PTNUM_MASK		= 0x3F000000,
	OCRDMA_HBA_ATTRB_PTNUM_SHIFT		= 0x18,
	OCRDMA_HBA_ATTRB_PT_MASK		= 0xC0000000,
	OCRDMA_HBA_ATTRB_PT_SHIFT		= 0x1E,
	OCRDMA_HBA_ATTRB_ISCSI_FET_MASK		= 0xFF,
	OCRDMA_HBA_ATTRB_ASIC_GEN_MASK		= 0xFF00,
	OCRDMA_HBA_ATTRB_ASIC_GEN_SHIFT		= 0x08,
	OCRDMA_HBA_ATTRB_PCI_VID_MASK		= 0xFFFF,
	OCRDMA_HBA_ATTRB_PCI_DID_MASK		= 0xFFFF0000,
	OCRDMA_HBA_ATTRB_PCI_DID_SHIFT		= 0x10,
	OCRDMA_HBA_ATTRB_PCI_SVID_MASK		= 0xFFFF,
	OCRDMA_HBA_ATTRB_PCI_SSID_MASK		= 0xFFFF0000,
	OCRDMA_HBA_ATTRB_PCI_SSID_SHIFT		= 0x10,
	OCRDMA_HBA_ATTRB_PCI_BUSNUM_MASK	= 0xFF,
	OCRDMA_HBA_ATTRB_PCI_DEVNUM_MASK	= 0xFF00,
	OCRDMA_HBA_ATTRB_PCI_DEVNUM_SHIFT	= 0x08,
	OCRDMA_HBA_ATTRB_PCI_FUNCNUM_MASK	= 0xFF0000,
	OCRDMA_HBA_ATTRB_PCI_FUNCNUM_SHIFT	= 0x10,
	OCRDMA_HBA_ATTRB_IF_TYPE_MASK		= 0xFF000000,
	OCRDMA_HBA_ATTRB_IF_TYPE_SHIFT		= 0x18,
	OCRDMA_HBA_ATTRB_NETFIL_MASK		=0xFF
};

struct mgmt_hba_attribs {
	u8 flashrom_version_string[32];
	u8 manufacturer_name[32];
	u32 supported_modes;
	u32 rsvd_eprom_verhi_verlo;
	u32 mbx_ds_ver;
	u32 epfw_ds_ver;
	u8 ncsi_ver_string[12];
	u32 default_extended_timeout;
	u8 controller_model_number[32];
	u8 controller_description[64];
	u8 controller_serial_number[32];
	u8 ip_version_string[32];
	u8 firmware_version_string[32];
	u8 bios_version_string[32];
	u8 redboot_version_string[32];
	u8 driver_version_string[32];
	u8 fw_on_flash_version_string[32];
	u32 functionalities_supported;
	u32 guid0_asicrev_cdblen;
	u8 generational_guid[12];
	u32 portcnt_guid15;
	u32 mfuncdev_iscsi_ldtout;
	u32 ptpnum_maxdoms_hbast_cv;
	u32 firmware_post_status;
	u32 hba_mtu[8];
	u32 res_asicgen_iscsi_feaures;
	u32 rsvd1[3];
};

struct mgmt_controller_attrib {
	struct mgmt_hba_attribs hba_attribs;
	u32 pci_did_vid;
	u32 pci_ssid_svid;
	u32 ityp_fnum_devnum_bnum;
	u32 uid_hi;
	u32 uid_lo;
	u32 res_nnetfil;
	u32 rsvd0[4];
};

struct ocrdma_get_ctrl_attribs_rsp {
	struct ocrdma_mbx_hdr hdr;
	struct mgmt_controller_attrib ctrl_attribs;
};

#define OCRDMA_SUBSYS_DCBX 0x10

enum OCRDMA_DCBX_OPCODE {
	OCRDMA_CMD_GET_DCBX_CONFIG = 0x01
};

enum OCRDMA_DCBX_PARAM_TYPE {
	OCRDMA_PARAMETER_TYPE_ADMIN	= 0x00,
	OCRDMA_PARAMETER_TYPE_OPER	= 0x01,
	OCRDMA_PARAMETER_TYPE_PEER	= 0x02
};

enum OCRDMA_DCBX_APP_PROTO {
	OCRDMA_APP_PROTO_ROCE	= 0x8915
};

enum OCRDMA_DCBX_PROTO {
	OCRDMA_PROTO_SELECT_L2	= 0x00,
	OCRDMA_PROTO_SELECT_L4	= 0x01
};

enum OCRDMA_DCBX_APP_PARAM {
	OCRDMA_APP_PARAM_APP_PROTO_MASK = 0xFFFF,
	OCRDMA_APP_PARAM_PROTO_SEL_MASK = 0xFF,
	OCRDMA_APP_PARAM_PROTO_SEL_SHIFT = 0x10,
	OCRDMA_APP_PARAM_VALID_MASK	= 0xFF,
	OCRDMA_APP_PARAM_VALID_SHIFT	= 0x18
};

enum OCRDMA_DCBX_STATE_FLAGS {
	OCRDMA_STATE_FLAG_ENABLED	= 0x01,
	OCRDMA_STATE_FLAG_ADDVERTISED	= 0x02,
	OCRDMA_STATE_FLAG_WILLING	= 0x04,
	OCRDMA_STATE_FLAG_SYNC		= 0x08,
	OCRDMA_STATE_FLAG_UNSUPPORTED	= 0x40000000,
	OCRDMA_STATE_FLAG_NEG_FAILD	= 0x80000000
};

enum OCRDMA_TCV_AEV_OPV_ST {
	OCRDMA_DCBX_TC_SUPPORT_MASK	= 0xFF,
	OCRDMA_DCBX_TC_SUPPORT_SHIFT	= 0x18,
	OCRDMA_DCBX_APP_ENTRY_SHIFT	= 0x10,
	OCRDMA_DCBX_OP_PARAM_SHIFT	= 0x08,
	OCRDMA_DCBX_STATE_MASK		= 0xFF
};

struct ocrdma_app_parameter {
	u32 valid_proto_app;
	u32 oui;
	u32 app_prio[2];
};

struct ocrdma_dcbx_cfg {
	u32 tcv_aev_opv_st;
	u32 tc_state;
	u32 pfc_state;
	u32 qcn_state;
	u32 appl_state;
	u32 ll_state;
	u32 tc_bw[2];
	u32 tc_prio[8];
	u32 pfc_prio[2];
	struct ocrdma_app_parameter app_param[15];
};

struct ocrdma_get_dcbx_cfg_req {
	struct ocrdma_mbx_hdr hdr;
	u32 param_type;
} __packed;

struct ocrdma_get_dcbx_cfg_rsp {
	struct ocrdma_mbx_rsp hdr;
	struct ocrdma_dcbx_cfg cfg;
} __packed;

#endif				/* __OCRDMA_SLI_H__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /* This file is part of the Emulex RoCE Device Driver for
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

#include <rdma/ib_addr.h>
#include <rdma/ib_pma.h>
#include "ocrdma_stats.h"

static struct dentry *ocrdma_dbgfs_dir;

static int ocrdma_add_stat(char *start, char *pcur,
				char *name, u64 count)
{
	char buff[128] = {0};
	int cpy_len = 0;

	snprintf(buff, 128, "%s: %llu\n", name, count);
	cpy_len = strlen(buff);

	if (pcur + cpy_len > start + OCRDMA_MAX_DBGFS_MEM) {
		pr_err("%s: No space in stats buff\n", __func__);
		return 0;
	}

	memcpy(pcur, buff, cpy_len);
	return cpy_len;
}

static bool ocrdma_alloc_stats_mem(struct ocrdma_dev *dev)
{
	struct stats_mem *mem = &dev->stats_mem;

	/* Alloc mbox command mem*/
	mem->size = max_t(u32, sizeof(struct ocrdma_rdma_stats_req),
			sizeof(struct ocrdma_rdma_stats_resp));

	mem->va   = dma_alloc_coherent(&dev->nic_info.pdev->dev, mem->size,
					 &mem->pa, GFP_KERNEL);
	if (!mem->va) {
		pr_err("%s: stats mbox allocation failed\n", __func__);
		return false;
	}

	memset(mem->va, 0, mem->size);

	/* Alloc debugfs mem */
	mem->debugfs_mem = kzalloc(OCRDMA_MAX_DBGFS_MEM, GFP_KERNEL);
	if (!mem->debugfs_mem) {
		pr_err("%s: stats debugfs mem allocation failed\n", __func__);
		return false;
	}

	return true;
}

static void ocrdma_release_stats_mem(struct ocrdma_dev *dev)
{
	struct stats_mem *mem = &dev->stats_mem;

	if (mem->va)
		dma_free_coherent(&dev->nic_info.pdev->dev, mem->size,
				  mem->va, mem->pa);
	kfree(mem->debugfs_mem);
}

static char *ocrdma_resource_stats(struct ocrdma_dev *dev)
{
	char *stats = dev->stats_mem.debugfs_mem, *pcur;
	struct ocrdma_rdma_stats_resp *rdma_stats =
			(struct ocrdma_rdma_stats_resp *)dev->stats_mem.va;
	struct ocrdma_rsrc_stats *rsrc_stats = &rdma_stats->act_rsrc_stats;

	memset(stats, 0, (OCRDMA_MAX_DBGFS_MEM));

	pcur = stats;
	pcur += ocrdma_add_stat(stats, pcur, "active_dpp_pds",
				(u64)rsrc_stats->dpp_pds);
	pcur += ocrdma_add_stat(stats, pcur, "active_non_dpp_pds",
				(u64)rsrc_stats->non_dpp_pds);
	pcur += ocrdma_add_stat(stats, pcur, "active_rc_dpp_qps",
				(u64)rsrc_stats->rc_dpp_qps);
	pcur += ocrdma_add_stat(stats, pcur, "active_uc_dpp_qps",
				(u64)rsrc_stats->uc_dpp_qps);
	pcur += ocrdma_add_stat(stats, pcur, "active_ud_dpp_qps",
				(u64)rsrc_stats->ud_dpp_qps);
	pcur