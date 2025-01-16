atagram mode.
	 */
	IB_DEVICE_UD_IP_CSUM		= (1<<18),
	IB_DEVICE_UD_TSO		= (1<<19),
	IB_DEVICE_XRC			= (1<<20),
	IB_DEVICE_MEM_MGT_EXTENSIONS	= (1<<21),
	IB_DEVICE_BLOCK_MULTICAST_LOOPBACK = (1<<22),
	IB_DEVICE_MEM_WINDOW_TYPE_2A	= (1<<23),
	IB_DEVICE_MEM_WINDOW_TYPE_2B	= (1<<24),
	IB_DEVICE_RC_IP_CSUM		= (1<<25),
	IB_DEVICE_RAW_IP_CSUM		= (1<<26),
	IB_DEVICE_MANAGED_FLOW_STEERING = (1<<29),
	IB_DEVICE_SIGNATURE_HANDOVER	= (1<<30),
	IB_DEVICE_ON_DEMAND_PAGING	= (1<<31),
};

enum ib_signature_prot_cap {
	IB_PROT_T10DIF_TYPE_1 = 1,
	IB_PROT_T10DIF_TYPE_2 = 1 << 1,
	IB_PROT_T10DIF_TYPE_3 = 1 << 2,
};

enum ib_signature_guard_cap {
	IB_GUARD_T10DIF_CRC	= 1,
	IB_GUARD_T10DIF_CSUM	= 1 << 1,
};

enum ib_atomic_cap {
	IB_ATOMIC_NONE,
	IB_ATOMIC_HCA,
	IB_ATOMIC_GLOB
};

enum ib_odp_general_cap_bits {
	IB_ODP_SUPPORT = 1 << 0,
};

enum ib_odp_transport_cap_bits {
	IB_ODP_SUPPORT_SEND	= 1 << 0,
	IB_ODP_SUPPORT_RECV	= 1 << 1,
	IB_ODP_SUPPORT_WRITE	= 1 << 2,
	IB_ODP_SUPPORT_READ	= 1 << 3,
	IB_ODP_SUPPORT_ATOMIC	= 1 << 4,
};

struct ib_odp_caps {
	uint64_t general_caps;
	str