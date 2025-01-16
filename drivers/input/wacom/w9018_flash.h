_MAX_SRQ_RQE_OFFSET		= 16,
	OCRDMA_MBX_QUERY_CFG_MAX_SRQ_RQE_MASK		= 0xFFFF <<
				OCRDMA_MBX_QUERY_CFG_MAX_SRQ_RQE_OFFSET,
	OCRDMA_MBX_QUERY_CFG_MAX_SRQ_SGE_OFFSET		= 0,
	OCRDMA_MBX_QUERY_CFG_MAX_SRQ_SGE_MASK		= 0xFFFF <<
				OCRDMA_MBX_QUERY_CFG_MAX_SRQ_SGE_OFFSET,
};

struct ocrdma_mbx_query_config {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;
	u32 qp_srq_cq_ird_ord;
	u32 max_pd_ca_ack_delay;
	u32 max_write_send_sge;
	u32 max_ird_ord_per_qp;
	u32 max_shared_ird_ord;
	u32 max_mr;
	u32 max_mr_size_hi;
	u32 max_mr_size_lo;
	u32 max_num_mr_pbl;
	u32 max_mw;
	u32 max_fmr;
	u32 max_pages_per_frmr;
	u32 max_mcast_group;
	u32 max_mcast_qp_attach;
	u32 max_total_mcast_qp_attach;
	u32 wqe_rqe_stride_max_dpp_cqs;
	u32 max_srq_rpir_qps;
	u32 max_dpp_pds_credits;
	u32 max_dpp_credits_pds_per_pd;
	u32 max_wqes_rqes_per_q;
	u32 max_cq_cqes_per_cq;
	u32 max_srq_rqe_sge;
};

struct ocrdma_fw_ver_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;

	u8 running_ver[32];
};

struct ocrdma_fw_conf_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;

	u32 config_num;
	u32 asic_revision;
	u32 phy_port;
	u32 fn_mode;
	struct {
		u32 mode;
		u32 nic_wqid_base;
		u32 nic_wq_tot;
		u32 prot_wqid_base;
		u32 prot_wq_tot;
		u32 prot_rqid_base;
		u32 prot_rqid_tot;
		u32 rsvd[6];
	} ulp[2];
	u32 fn_capabilities;
	u32 rsvd1;
	u32 rsvd2;
	u32 base_eqid;
	u32 max_eq;

};

enum {
	OCRDMA_FN_MODE_RDMA	= 0x4
};

enum {
	OCRDMA_IF_TYPE_MASK		= 0xFFFF0000,
	OCRDMA_IF_TYPE_SHIFT		= 0x10,
	OCRDMA_PHY_TYPE_MASK		= 0x0000FFFF,
	OCRDMA_FUTURE_DETAILS_MASK	= 0xFFFF0000,
	OCRDMA_FUTURE_DETAILS_SHIFT	= 0x10,
	OCRDMA_EX_PHY_DETAILS_MASK	= 0x0000FFFF,
	OCRDMA_FSPEED_SUPP_MASK		= 0xFFFF0000,
	OCRDMA_FSPEED_SUPP_SHIFT	= 0x10,
	OCRDMA_ASPEED_SUPP_MASK		= 0x0000FFFF
};

struct ocrdma_get_phy_info_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;

	u32 ityp_ptyp;
	u32 misc_params;
	u32 ftrdtl_exphydtl;
	u32 fspeed_aspeed;
	u32 future_use[2];
};

enum {
	OCRDMA_PHY_SPEED_ZERO = 0x0,
	OCRDMA_PHY_SPEED_10MBPS = 0x1,
	OCRDMA_PHY_SPEED_100MBPS = 0x2,
	OCRDMA_PHY_SPEED_1GBPS = 0x4,
	OCRDMA_PHY_SPEED_10GBPS = 0x8,
	OCRDMA_PHY_SPEED_40GBPS = 0x20
};

enum {
	OCRDMA_PORT_NUM_MASK	= 0x3F,
	OCRDMA_PT_MASK		= 0xC0,
	OCRDMA_PT_SHIFT		= 0x6,
	OCRDMA_LINK_DUP_MASK	= 0x0000FF00,
	OCRDMA_LINK_DUP_SHIFT	= 0x8,
	OCRDMA_PHY_PS_MASK	= 0x00FF0000,
	OCRDMA_PHY_PS_SHIFT	= 0x10,
	OCRDMA_PHY_PFLT_MASK	= 0xFF000000,
	OCRDMA_PHY_PFLT_SHIFT	= 0x18,
	OCRDMA_QOS_LNKSP_MASK	= 0xFFFF0000,
	OCRDMA_QOS_LNKSP_SHIFT	= 0x10,
	OCRDMA_LINK_ST_MASK	= 0x01,
	OCRDMA_PLFC_MASK	= 0x00000400,
	OCRDMA_PLFC_SHIFT	= 0x8,
	OCRDMA_PLRFC_MASK	= 0x00000200,
	OCRDMA_PLRFC_SHIFT	= 0x8,
	OCRDMA_PLTFC_MASK	= 0x00000100,
	OCRDMA_PLTFC_SHIFT	= 0x8
};

struct ocrdma_get_link_speed_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;

	u32 pflt_pps_ld_pnum;
	u32 qos_lsp;
	u32 res_lnk_st;
};

enum {
	OCRDMA_PHYS_LINK_SPEED_ZERO = 0x0,
	OCRDMA_PHYS_LINK_SPEED_10MBPS = 0x1,
	OCRDMA_PHYS_LINK_SPEED_100MBPS = 0x2,
	OCRDMA_PHYS_LINK_SPEED_1GBPS = 0x3,
	OCRDMA_PHYS_LINK_SPEED_10GBPS = 0x4,
	OCRDMA_PHYS_LINK_SPEED_20GBPS = 0x5,
	OCRDMA_PHYS_LINK_SPEED_25GBPS = 0x6,
	OCRDMA_PHYS_LINK_SPEED_40GBPS = 0x7,
	OCRDMA_PHYS_LINK_SPEED_100GBPS = 0x8
};

enum {
	OCRDMA_CREATE_CQ_VER2			= 2,
	OCRDMA_CREATE_CQ_VER3			= 3,

	OCRDMA_CREATE_CQ_PAGE_CNT_MASK		= 0xFFFF,
	OCRDMA_CREATE_CQ_PAGE_SIZE_SHIFT	= 16,
	OCRDMA_CREATE_CQ_PAGE_SIZE_MASK		= 0xFF,

	OCRDMA_CREATE_CQ_COALESCWM_SHIFT	= 1