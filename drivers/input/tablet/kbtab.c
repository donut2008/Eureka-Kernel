NSMR_RSP_NUM_PBL_MASK	= 0xFFFF0000
};
struct ocrdma_reg_nsmr_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;

	u32 lrkey;
	u32 num_pbl;
};

enum {
	OCRDMA_REG_NSMR_CONT_RSP_LRKEY_INDEX_SHIFT	= 0,
	OCRDMA_REG_NSMR_CONT_RSP_LRKEY_INDEX_MASK	= 0xFFFFFF,
	OCRDMA_REG_NSMR_CONT_RSP_LRKEY_SHIFT		= 24,
	OCRDMA_REG_NSMR_CONT_RSP_LRKEY_MASK		= 0xFF <<
					OCRDMA_REG_NSMR_CONT_RSP_LRKEY_SHIFT,

	OCRDMA_REG_NSMR_CONT_RSP_NUM_PBL_SHIFT		= 16,
	OCRDMA_REG_NSMR_CONT_RSP_NUM_PBL_MASK		= 0xFFFF <<
					OCRDMA_REG_NSMR_CONT_RSP_NUM_PBL_SHIFT
};

struct ocrdma_reg_nsmr_cont_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;

	u32 lrkey_key_index;
	u32 num_pbl;
};

enum {
	OCRDMA_ALLOC_MW_PD_ID_SHIFT	= 0,
	OCRDMA_ALLOC_MW_PD_ID_MASK	= 0xFFFF
};

struct ocrdma_alloc_mw {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_hdr req;

	u32 pdid;
};

enum {
	OCRDMA_ALLOC_MW_RSP_LRKEY_INDEX_SHIFT	= 0,
	OCRDMA_ALLOC_MW_RSP_LRKEY_INDEX_MASK	= 0xFFFFFF
};

struct ocrdma_alloc_mw_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;

	u32 lrkey_index;
};

struct ocrdma_attach_mcast {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_hdr req;
	u32 qp_id;
	u8 mgid[16];
	u32 mac_b0_to_b3;
	u32 vlan_mac_b4_to_b5;
};

struct ocrdma_attach_mcast_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;
};

struct ocrdma_detach_mcast {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_hdr req;
	u32 qp_id;
	u8 mgid[16];
	u32 mac_b0_to_b3;
	u32 vlan_mac_b4_to_b5;
};

struct ocrdma_detach_mcast_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;
};

enum {
	OCRDMA_CREATE_AH_NUM_PAGES_SHIFT	= 19,
	OCRDMA_CREATE_AH_NUM_PAGES_MASK		= 0xF <<
					OCRDMA_CREATE_AH_NUM_PAGES_SHIFT,

	OCRDMA_CREATE_AH_PAGE_SIZE_SHIFT	= 16,
	OCRDMA_CREATE_AH_PAGE_SIZE_MASK		= 0x7 <<
					OCRDMA_CREATE_AH_PAGE_SIZE_SHIFT,

	OCRDMA_CREATE_AH_ENTRY_SIZE_SHIFT	= 23,
	OCRDMA_CREATE_AH_ENTRY_SIZE_MASK	= 0x1FF <<
					OCRDMA_CREATE_AH_ENTRY_SIZE_SHIFT,
};

#define OCRDMA_AH_TBL_PAGES 8

struct ocrdma_create_ah_tbl {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_hdr req;

	u32 ah_conf;
	struct ocrdma_pa tbl_addr[8];
};

struct ocrdma_create_ah_tbl_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;
	u32 ahid;
};

struct ocrdma_delete_ah_tbl {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_hdr req;
	u32 ahid;
};

struct ocrdma_delete_ah_tbl_rsp {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_mbx_rsp rsp;
};

enum {
	OCRDMA_EQE_VALID_SHIFT		= 0,
	OCRDMA_EQE_VALID_MASK		= BIT(0),
	OCRDMA_EQE_MAJOR_CODE_MASK      = 0x0E,
	OCRDMA_EQE_MAJOR_CODE_SHIFT     = 0x01,
	OCRDMA_EQE_FOR_CQE_MASK		= 0xFFFE,
	OCRDMA_EQE_RESOURCE_ID_SHIFT	= 16,
	OCRDMA_EQE_RESOURCE_ID_MASK	= 0xFFFF <<
				OCRDMA_EQE_RESOURCE_ID_SHIFT,
};

enum major_code {
	OCRDMA_MAJOR_CODE_COMPLETION    = 0x00,
	OCRDMA_MAJOR_CODE_SENTINAL      = 0x01
};

struct ocrdma_eqe {
	u32 id_valid;
};

enum OCRDMA_CQE_STATUS {
	OCRDMA_CQE_SUCCESS = 0,
	OCRDMA_CQE_LOC_LEN_ERR,
	OCRDMA_CQE_LOC_QP_OP_ERR,
	OCRDMA_CQE_LOC_EEC_OP_ERR,
	OCRDMA_CQE_LOC_PROT_ERR,
	OCRDMA_CQE_WR_FLUSH_ERR,
	OCRDMA_CQE_MW_BIND_ERR,
	OCRDMA_CQE_BAD_RESP_ERR,
	OCRDMA_CQE_LOC_ACCESS_ERR,
	OCRDMA_CQE_REM_INV_REQ_ERR,
	OCRDMA_CQE_REM_ACCESS_ERR,
	OCRDMA_CQE_REM_OP_ERR,
	OCRDMA_CQE_RETRY_EXC_ERR,
	OCRDMA_CQE_RNR_RETRY_EXC_ERR,
	OCRDMA_CQE_LOC_RDD_VIOL_ERR,
	OCRDMA_CQE_REM_INV_RD_REQ_ERR,
	OCRDMA_CQE_REM_ABORT_ERR,
	OCRDMA_CQE_INV_EECN_ERR,
	OCRDMA_CQE_INV_EEC_STATE_ERR,
	OCRDMA_CQE_FATAL_ERR,
	OCRDMA_CQE_RESP_TIMEOUT_ERR,
	OCRDMA_CQE_GENERAL_ERR,

	OCRDMA_MAX_CQE_ERR
};

enum {
	/* w0 */
	OCRDMA_CQE_WQEIDX_SHIFT		= 0,
	OCRDMA_CQE_WQEIDX_MASK		= 0xFFFF,

	/* w1 */
	OCRDMA_CQE_UD_XFER_LEN_SHIFT	= 16,
	OCRDMA_CQE_PKEY_SHIFT		= 0,
	OCRDMA_CQE_PKEY_MASK		= 0xFFFF,

	/* w2 */
	OCRDMA_CQE_QPN_SHIFT		= 0,
	OCRDMA_CQE_QPN_MASK		= 0x0000FFFF,

	OCRDMA_CQE_BUFTAG_SHIFT		= 16,
	OCRDMA_CQE_BUFTAG_MASK		= 0xFFFF << OCRDMA_CQE_BUFTAG_SHIFT,

	/* w3 */
	OCRDMA_CQE_UD_STATUS_SHIFT	= 24,
	OCRDMA_CQE_UD_STATUS_MASK	= 0x7 << OCRDMA_CQE_UD_STATUS_SHIFT,
	OCRDMA_CQE_STATUS_SHIFT		= 16,
	OCRDMA_CQE_STATUS_MASK		= 0xFF << OCRDMA_CQE_STATUS_SHIFT,
	OCRDMA_CQE_VALID		= BIT(31),
	OCRDMA_CQE_INVALIDATE		= BIT(30),
	OCRDMA_CQE_QTYPE		= BIT(29),
	OCRDMA_CQE_IMM			= BIT(28),
	OCRDMA_CQE_WRITE_IMM		= BIT(27),
	OCRDMA_CQE_QTYPE_SQ		= 0,
	OCRDMA_CQE_QTYPE_RQ		= 1,
	OCRDMA_CQE_SRCQP_MASK		= 0xFFFFFF
};

struct ocrdma_cqe {
	union {
		/* w0 to w2 */
		struct {
			u32 wqeidx;
			u32 bytes_xfered;
			u32 qpn;
		} wq;
		struct {
			u32 lkey_immdt;
			u32 rxlen;
			u32 buftag_qpn;
		} rq;
		struct {
			u32 lkey_immdt;
			u32 rxlen_pkey;
			u32 buftag_qpn;
		} ud;
		struct {
			u32 word_0;
			u32 word_1;
			u32 qpn;
		} cmn;
	};
	u32 flags_status_srcqpn;	/* w3 */
};

struct ocrdma_sge {
	u32 addr_hi;
	u32 addr_lo;
	u32 lrkey;
	u32 len;
};

enum {
	OCRDMA_FLAG_SIG		= 0x1,
	OCRDMA_FLAG_INV		= 0x2,
	OCRDMA_FLAG_FENCE_L	= 0x4,
	OCRDMA_FLAG_FENCE_R	= 0x8,
	OCRDMA_FLAG_SOLICIT	= 0x10,
	OCRDMA_FLAG_IMM		= 0x20,
	OCRDMA_FLAG_AH_VLAN_PR  = 0x40,

	/* Stag flags */
	OCRDMA_LKEY_FLAG_LOCAL_WR	= 0x1,
	OCRDMA_LKEY_FLAG_REMOTE_RD	= 0x2,
	OCRDMA_LKEY_FLAG_REMOTE_WR	= 0x4,
	OCRDMA_LKEY_FLAG_VATO		= 0x8,
};

enum OCRDMA_WQE_OPCODE {
	OCRDMA_WRITE		= 0x06,
	OCRDMA_READ		= 0x0C,
	OCRDMA_RESV0		= 0x02,
	OCRDMA_SEND		= 0x00,
	OCRDMA_CMP_SWP		= 0x14,
	OCRDMA_BIND_MW		= 0x10,
	OCRDMA_FR_MR            = 0x11,
	OCR