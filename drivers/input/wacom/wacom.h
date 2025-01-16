
	OCRDMA_DB_RQ_SHIFT		= 24
};

#define OCRDMA_ROUDP_FLAGS_SHIFT	0x03

#define OCRDMA_DB_CQ_RING_ID_MASK       0x3FF	/* bits 0 - 9 */
#define OCRDMA_DB_CQ_RING_ID_EXT_MASK  0x0C00	/* bits 10-11 of qid at 12-11 */
/* qid #2 msbits at 12-11 */
#define OCRDMA_DB_CQ_RING_ID_EXT_MASK_SHIFT  0x1
#define OCRDMA_DB_CQ_NUM_POPPED_SHIFT	16	/* bits 16 - 28 */
/* Rearm bit */
#define OCRDMA_DB_CQ_REARM_SHIFT	29	/* bit 29 */
/* solicited bit */
#define OCRDMA_DB_CQ_SOLICIT_SHIFT	31	/* bit 31 */

#define OCRDMA_EQ_ID_MASK		0x1FF	/* bits 0 - 8 */
#define OCRDMA_EQ_ID_EXT_MASK		0x3e00	/* bits 9-13 */
#define OCRDMA_EQ_ID_EXT_MASK_SHIFT	2	/* qid bits 9-13 at 11-15 */

/* Clear the interrupt for this eq */
#define OCRDMA_EQ_CLR_SHIFT		9	/* bit 9 */
/* Must be 1 */
#define OCRDMA_EQ_TYPE_SHIFT		10	/* bit 10 */
/* Number of event entries processed */
#define OCRDMA_NUM_EQE_SHIFT		16	/* bits 16 - 28 */
/* Rearm bit */
#define OCRDMA_REARM_SHIFT		29	/* bit 29 */

#define OCRDMA_MQ_ID_MASK		0x7FF	/* bits 0 - 10 */
/* Number of entries posted */
#define OCRDMA_MQ_NUM_MQE_SHIFT	16	/* bits 16 - 29 */

#define OCRDMA_MIN_HPAGE_SIZE	4096

#define OCRDMA_MIN_Q_PAGE_SIZE	4096
#define OCRDMA_MAX_Q_PAGES	8

#define OCRDMA_SLI_ASIC_ID_OFFSET	0x9C
#define OCRDMA_SLI_ASIC_REV_MASK	0x000000FF
#define OCRDMA_SLI_ASIC_GEN_NUM_MASK	0x0000FF00
#define OCRDMA_SLI_ASIC_GEN_NUM_SHIFT	0x08
/*
# 0: 4K Bytes
# 1: 8K Bytes
# 2: 16K Bytes
# 3: 32K Bytes
# 4: 64K Bytes
# 5: 128K Bytes
# 6: 256K Bytes
# 7: 512K Bytes
*/
#define OCRDMA_MAX_Q_PAGE_SIZE_CNT	8
#define OCRDMA_Q_PAGE_BASE_SIZE (OCRDMA_MIN_Q_PAGE_SIZE * OCRDMA_MAX_Q_PAGES)

#define MAX_OCRDMA_QP_PAGES		8
#define OCRDMA_MAX_WQE_MEM_SIZE (MAX_OCRDMA_QP_PAGES * OCRDMA_MIN_HQ_PAGE_SIZE)

#define OCRDMA_CREATE_CQ_MAX_PAGES	4
#define OCRDMA_DPP_CQE_SIZE		4

#define OCRDMA_GEN2_MAX_CQE 1024
#define OCRDMA_GEN2_CQ_PAGE_SIZE 4096
#define OCRDMA_GEN2_WQE_SIZE 256
#define OCRDMA_MAX_CQE  4095
#define OCRDMA_CQ_PAGE_SIZE 16384
#define OCRDMA_WQE_SIZE 128
#define OCRDMA_WQE_STRIDE 8
#define OCRDMA_WQE_ALIGN_BYTES 16

#define MAX_OCRDMA_SRQ_PAGES MAX_OCRDMA_QP_PAGES

enum {
	OCRDMA_MCH_OPCODE_SHIFT	= 0,
	OCRDMA_MCH_OPCODE_MASK	= 0xFF,
	OCRDMA_MCH_SUBSYS_SHIFT	= 8,
	OCRDMA_MCH_SUBSYS_MASK	= 0xFF00
};

/* mailbox cmd header */
struct ocrdma_mbx_hdr {
	u32 subsys_op;
	u32 timeout;		/* in seconds */
	u32 cmd_len;
	u32 rsvd_version;
};

enum {
	OCRDMA_MBX_RSP_OPCODE_SHIFT	= 0,
	OCRDMA_MBX_RSP_OPCODE_MASK	= 0xFF,
	OCRDMA_MBX_RSP_SUBSYS_SHIFT	= 8,
	OCRDMA_MBX_RSP_SUBSYS_MASK	= 0xFF << OCRDMA_MBX_RSP_SUBSYS_SHIFT,

	OCRDMA_MBX_RSP_STATUS_SHIFT	= 0,
	OCRDMA_MBX_RSP_STATUS_MASK	= 0xFF,
	OCRDMA_MBX_RSP_ASTATUS_SHIFT	= 8,
	OCRDMA_MBX_RSP_ASTATUS_MASK	= 0xFF << OCRDMA_MBX_RSP_ASTATUS_SHIFT
};

/* mailbox cmd response */
struct ocrdma_mbx_rsp {
	u32 subsys_op;
	u32 status;
	u32 rsp_len;
	u32 add_rsp_len;
};

enum {
	OCRDMA_MQE_EMBEDDED	= 1,
	OCRDMA_MQE_NONEMBEDDED	= 0
};

struct ocrdma_mqe_sge {
	u32 pa_lo;
	u32 pa_hi;
	u32 len;
};

enum {
	OCRDMA_MQE_HDR_EMB_SHIFT	= 0,
	OCRDMA_MQE_HDR_EMB_MASK		= BIT(0),
	OCRDMA_MQE_HDR_SGE_CNT_SHIFT	= 3,
	OCRDMA_MQE_HDR_SGE_CNT_MASK	= 0x1F << OCRDMA_MQE_HDR_SGE_CNT_SHIFT,
	OCRDMA_MQE_HDR_SPECIAL_SHIFT	= 24,
	OCRDMA_MQE_HDR_SPECIAL_MASK	= 0xFF << OCRDMA_MQE_HDR_SPECIAL_SHIFT
};

struct ocrdma_mqe_hdr {
	u32 spcl_sge_cnt_emb;
	u32 pyld_len;
	u32 tag_lo;
	u32 tag_hi;
	u32 rsvd3;
};

struct ocrdma_mqe_emb_cmd {
	struct ocrdma_mbx_hdr mch;
	u8 pyld[220];
};

struct ocrdma_mqe {
	struct ocrdma_mqe_hdr hdr;
	union {
		struct ocrdma_mqe_emb_cmd emb_req;
		struct {
			struct ocrdma_mqe_sge sge[19];
		} nonemb_req;
		u8 cmd[236];
		struct ocrdma_mbx_rsp rsp;
	} u;
};

#define OCRDMA_EQ_LEN       4096
#define OCRDMA_MQ_CQ_LEN    256
#define OCRDMA_MQ_LEN       128

#define PAGE_SHIFT_4K		12
#define PAGE_SIZE_4K		(1 << PAGE_SHIFT_4K)

/* Returns number of pages spanned by the data starting at the given addr */
#define PAGES_4K_SPANNED(_address, size) \
	((u32)((((size_t)(_address) & (PAGE_SIZE_4K - 1)) +	\
			(size) + (PAGE_SIZE_4K - 1)) >> PAGE_SHIFT_4K))

struct ocrdma_delete_q_req {
	struct ocrdma_mbx_hdr req;
	u32 id;
};

struct ocrdma_pa {
	u32 lo;
	u32 hi;
};

#define MAX_OCRDMA_EQ_PAGES	8
struct ocrdma_create_eq_req {
	struct ocrdma_mbx_hdr req;
	u32 num_pages;
	u32 valid;
	u32 cnt;
	u32 delay;
	u32 rsvd;
	struct ocrdma_pa pa[MAX_OCRDMA_EQ_PAGES];
};

enum {
	OCRDMA_CREATE_EQ_VALID	= BIT(29),
	OCRDMA_CREATE_EQ_CNT_SHIFT	= 26,
	OCRDMA_CREATE_CQ_DELAY_SHIFT	= 13,
};

struct ocrdma_create_eq_rsp {
	struct ocrdma_mbx_rsp rsp;
	u32 vector_eqid;
};

#define OCRDMA_EQ_MINOR_OTHER	0x1

struct ocrmda_set_eqd {
	u32 eq_id;
	u32 phase;
	u32 delay_multiplier;
};

struct ocrdma_modify_eqd_cmd {
	struct ocrdma_mbx_hdr req;
	u32 num_eq;
	struct ocrmda_set_eqd set_eqd[8];
} __packed;

struct ocrdma_modify_eqd_req {
	struct ocrdma_mqe_hdr hdr;
	struct ocrdma_modify_eqd_cmd cmd;
};


struct ocrdma_modify_eq_delay_rsp {
	struct ocrdma_mbx_rsp hdr;
	u32 rsvd0;
} __packed;

enum {
	OCRDMA_MCQE_STATUS_SHIFT	= 0,
	OCRDMA_MCQE_STATUS_MASK		= 0xFFFF,
	OCRDMA_MCQE_ESTATUS_SHIFT	= 16,
	OCRDMA_MCQE_ESTATUS_MASK	= 0xFFFF << OCRDMA_MCQE_ESTATUS_SHIFT,
	OCRDMA_MCQE_CONS_SHIFT		= 27,
	OCRDMA_MCQE_CONS_MASK		= BIT(27),
	OCRDMA_MCQE_CMPL_SHIFT		= 28,
	OCRDMA_MCQE_CMPL_MASK		= BIT(28),
	OCRDMA_MCQE_AE_SHIFT		= 30,
	OCRDMA_MCQE_AE_MASK		= BIT(30),
	OCRDMA_MCQE_VALID_SHIFT		= 31,
	OCRDMA_MCQE_VALID_MASK		= BIT(31)
};

struct ocrdma_mcqe {
	u32 status;
	u32 tag_lo;
	u32 tag_hi;
	u32 valid_ae_cmpl_cons;
};

enum {
	OCRDMA_AE_MCQE_QPVALID		= BIT(31),
	OCRDMA_AE_MCQE_QPID_MASK	= 0xFFFF,

	OCRDMA_AE_MCQE_CQVALID		= BIT(31),
	OCRDMA_AE_MCQE_CQID_MASK	= 0xFFFF,
	OCRDMA_AE_MCQE_VALID		= BIT(31),
	OCRDMA_AE_MCQE_AE		= BIT(30),
	OCRDMA_AE_MCQE_EVENT_TYPE_SHIFT	= 16,
	OCRDMA_AE_MCQE_EVENT_TYPE_MASK	=
					0xFF << OCRDMA_AE_MCQE_EVENT_TYPE_SHIFT,
	OCRDMA_AE_MCQE_EVENT_CODE_SHIFT	= 8,
	OCRDMA_AE_MCQE_EVENT_CODE_MASK	=
					0xFF << OCRDMA_AE_MCQE_EVENT_CODE_SHIFT
};
struct ocrdma_ae_mcqe {
	u32 qpvalid_qpid;
	u32 cqvalid_cqid;
	u32 evt_tag;
	u32 valid_ae_event;
};

enum {
	OCRDMA_AE_PVID_MCQE_ENABLED_SHIFT = 0,
	OCRDMA_AE_PVID_MCQE_ENABLED_MASK  = 0xFF,
	OCRDMA_AE_PVID_MCQE_TAG_SHIFT = 16,
	OCRDMA_AE_PVID_MCQE_TAG_MASK = 0xFFFF << OCRDMA_AE_PVID_MCQE_TAG_SHIFT
};

struct ocrdma_ae_pvid_mcqe {
	u32 tag_enabled;
	u32 event_tag;
	u32 rsvd1;
	u32 rsvd2;
};

enum {
	OCRDMA_AE_MPA_MCQE_REQ_ID_SHIFT		= 16,
	OCRDMA_AE_MPA_MCQE_REQ_ID_MASK		= 0xFFFF <<
					OCRDMA_AE_MPA_MCQE_REQ_ID_SHIFT,

	OCRDMA_AE_MPA_MCQE_EVENT_CODE_SHIFT	= 8,
	OCRDMA_AE_MPA_MCQE_EVENT_CODE_MASK	= 0xFF <<
					OCRDMA_AE_MPA_MCQE_EVENT_CODE_SHIFT,
	OCRDMA_AE_MPA_MCQE_EVENT_TYPE_SHIFT	= 16,
	OCRDMA_AE_MPA_MCQE_EVENT_TYPE_MASK	= 0xFF <<
					OCRDMA_AE_MPA_MCQE_EVENT_TYPE_SHIFT,
	OCRDMA_AE_MPA_MCQE_EVENT_AE_SHIFT	= 30,
	OCRDMA_AE_MPA_MCQE_EVENT_AE_MASK	= BIT(30),
	OCRDMA_AE_MPA_MCQE_EVENT_VALID_SHIFT	= 31,
	OCRDMA_AE_MPA_MCQE_EVENT_VALID_MASK	= BIT(31)
};

struct ocrdma_ae_mpa_mcqe {
	u32 req_id;
	u32 w1;
	u32 w2;
	u32 valid_ae_event;
};

enum {
	OCRDMA_AE_QP_MCQE_NEW_QP_STATE_SHIFT	= 0,
	OCRDMA_AE_QP_MCQE_NEW_QP_STATE_MASK	= 0xFFFF,
	OCRDMA_AE_QP_MCQE_QP_ID_SHIFT		= 16,
	OCRDMA_AE_QP_MCQE_QP_ID_MASK		= 0xFFFF <<
						OCRDMA_AE_QP_MCQE_QP_ID_SHIFT,

	OCRDMA_AE_QP_MCQE_EVENT_CODE_SHIFT	= 8,
	OCRDMA_AE_QP_MCQE_EVENT_CODE_MASK	= 0xFF <<
				OCRDMA_AE_QP_MCQE_EVENT_CODE_SHIFT,
	OCRDMA_AE_QP_MCQE_EVENT_TYPE_SHIFT	= 16,
	OCRDMA_AE_QP_MCQE_EVENT_TYPE_MASK	= 0xFF <<
				OCRDMA_AE_QP_MCQE_EVENT_TYPE_SHIFT,
	OCRDMA_AE_QP_MCQE_EVENT_AE_SHIFT	= 30,
	OCRDMA_AE_QP_MCQE_EVENT_AE_MASK		= BIT(30),
	OCRDMA_AE_QP_MCQE_EVENT_VALID_SHIFT	= 31,
	OCRDMA_AE_QP_MCQE_EVENT_VALID_MASK	= BIT(31)
};

struct ocrdma_ae_qp_mcqe {
	u32 qp_id_state;
	u32 w1;
	u32 w2;
	u32 valid_ae_event;
};

enum ocrdma_async_event_code {
	OCRDMA_ASYNC_LINK_EVE_CODE	= 0x01,
	OCRDMA_ASYNC_GRP5_EVE_CODE	= 0x05,
	OCRDMA_ASYNC_RDMA_EVE_CODE	= 0x14
};

enum ocrdma_async_grp5_events {
	OCRDMA_ASYNC_EVENT_QOS_VALUE	= 0x01,
	OCRDMA_ASYNC_EVENT_COS_VALUE	= 0x02,
	OCRDMA_ASYNC_EVENT_PVID_STATE	= 0x03
};

enum OCRDMA_ASYNC_EVENT_TYPE {
	OCRDMA_CQ_ERROR			= 0x00,
	OCRDMA_CQ_OVERRUN_ERROR		= 0x01,
	OCRDMA_CQ_QPCAT_ERROR		= 0x02,
	OCRDMA_QP_ACCESS_ERROR		= 0x03,
	OCRDMA_QP_COMM_EST_EVENT	= 0x04,
	OCRDMA_SQ_DRAINED_EVENT		= 0x05,
	OCRDMA_DEVICE_FATAL_EVENT	= 0x08,
	OCRDMA_SRQCAT_ERROR		= 0x0E,
	OCRDMA_SRQ_LIMIT_EVENT		= 0x0F,
	OCRDMA_QP_LAST_WQE_EVENT	= 0x10,

	OCRDMA_MAX_ASYNC_ERRORS
};

struct ocrdma_ae_lnkst_mcqe {
	u32 speed_state_ptn;
	u32 qos_reason_falut;
	u32 evt_tag;
	u32 valid_ae_event;
};

enum {
	OCRDMA_AE_LSC_PORT_NUM_MASK	= 0x3F,
	OCRDMA_AE_LSC_PT_SHIFT		= 0x06,
	OCRDMA_AE_LSC_PT_MASK		= (0x03 <<
			OCRDMA_AE_LSC_PT_SHIFT),
	OCRDMA_AE_LSC_LS_SHIFT		= 0x08,
	OCRDMA_AE_LSC_LS_MASK		= (0xFF <<
			OCRDMA_AE_LSC_LS_SHIFT),
	OCRDMA_AE_LSC_LD_SHIFT		= 0x10,
	OCRDMA_AE_LSC_LD_MASK		= (0xFF <<
			OCRDMA_AE_LSC_LD_SHIFT),
	OCRDMA_AE_LSC_PPS_SHIFT		= 0x18,
	OCRDMA_AE_LSC_PPS_MASK		= (0xFF <<
			OCRDMA_AE_LSC_PPS_SHIFT),
	OCRDMA_AE_LSC_PPF_MASK		= 0xFF,
	OC