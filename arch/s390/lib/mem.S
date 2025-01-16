SIGNALED	= (1<<1),
	IB_SEND_SOLICITED	= (1<<2),
	IB_SEND_INLINE		= (1<<3),
	IB_SEND_IP_CSUM		= (1<<4),

	/* reserve bits 26-31 for low level drivers' internal use */
	IB_SEND_RESERVED_START	= (1 << 26),
	IB_SEND_RESERVED_END	= (1 << 31),
};

struct ib_sge {
	u64	addr;
	u32	length;
	u32	lkey;
};

/**
 * struct ib_mw_bind_info - Parameters for a memory window bind operation.
 * @mr: A memory region to bind the memory window to.
 * @addr: The address where the memory window should begin.
 * @length: The length of the memory window, in bytes.
 * @mw_access_flags: Access flags from enum ib_access_flags for the window.
 *
 * This struct contains the shared parameters for type 1 and type 2
 * memory window bind operations.
 */
struct ib_mw_bind_info {
	struct ib_mr   *mr;
	u64		addr;
	u64		length;
	int		mw_access_flags;
};

struct ib_send_wr {
	struct ib_send_wr      *next;
	u64			wr_id;
	struct ib_sge	       *sg_list;
	int			num_sge;
	enum ib_wr_opcode	opcode;
	int			send_flags;
	union {
		__be32		imm_data;
		u32		invalidate_rkey;
	} ex;
};

struct ib_rdma_wr {
	struct ib_send_wr	wr;
	u64			remote_addr;
	u32			rkey;
};

static inline struct ib_rdma_wr *rdma_wr(struct ib_send_wr *wr)
{
	return container_of(wr, struct ib_rdma_wr, wr);
}

struct ib_atomic_wr {
	struct ib_send_wr	wr;
	u64			remote_addr;
	u64			compare_add;
	u64			swap;
	u64			compare_add_mask;
	u64			swap_mask;
	u32			rkey;
};

static inline struct ib_atomic_wr *atomic_wr(struct ib_send_wr *wr)
{
	return container_of(wr, struct ib_atomic_wr, wr);
}

struct ib_ud_wr {
	struct ib_send_wr	wr;
	struct ib_ah		*ah;
	void			*header;
	int			hlen;
	int			mss;
	u32			remote_qpn;
	u32			remote_qkey;
	u16			pkey_index; /* valid for GSI only */
	u8			port_num;   /* valid for DR SMPs on switch