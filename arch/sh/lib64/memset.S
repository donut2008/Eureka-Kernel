IDTH_12X: return 12;
	default: 	  return -1;
	}
}

enum ib_port_speed {
	IB_SPEED_SDR	= 1,
	IB_SPEED_DDR	= 2,
	IB_SPEED_QDR	= 4,
	IB_SPEED_FDR10	= 8,
	IB_SPEED_FDR	= 16,
	IB_SPEED_EDR	= 32
};

struct ib_protocol_stats {
	/* TBD... */
};

struct iw_protocol_stats {
	u64	ipInReceives;
	u64	ipInHdrErrors;
	u64	ipInTooBigErrors;
	u64	ipInNoRoutes;
	u64	ipInAddrErrors;
	u64	ipInUnknownProtos;
	u64	ipInTruncatedPkts;
	u64	ipInDiscards;
	u64	ipInDelivers;
	u64	ipOutForwDatagrams;
	u64	ipOutRequests;
	u64	ipOutDiscards;
	u64	ipOutNoRoutes;
	u64	ipReasmTimeout;
	u64	ipReasmReqds;
	u64	ipReasmOKs;
	u64	ipReasmFails;
	u64	ipFragOKs;
	u64	ipFragFails;
	u64	ipFragCreates;
	u64	ipInMcastPkts;
	u64	ipOutMcastPkts;
	u64	ipInBcastPkts;
	u64	ipOutBcastPkts;

	u64	tcpRtoAlgorithm;
	u64	tcpRtoMin;
	u64	tcpRtoMax;
	u64	tcpMaxConn;
	u64	tcpActiveOpens;
	u64	tcpPassiveOpens;
	u64	tcpAttemptFails;
	u64	tcpEstabResets;
	u64	tcpCurrEstab;
	u64	tcpInSegs;
	u64	tcpOutSegs;
	u64	tcpRetransSegs;
	u64	tcpInErrs;
	u64	tcpOutRsts;
};

union rdma_protocol_stats {
	struct ib_protocol_stats	ib;
	struct iw_protocol_stats	iw;
};

/* Define bits for the various functionality this port needs to be supported by
 * the core.
 */
/* Management                           0x00000FFF */
#define RDMA_CORE_CAP_IB_MAD            0x00000001
#define RDMA_CORE_CAP_IB_SMI            0x00000002
#define RDMA_CORE_CAP_IB_CM             0x00000004
#define RDMA_CORE_CAP_IW_CM             0x00000008
#define RDMA_CORE_CAP_IB_SA             0x00000010
#define RDMA_CORE_CAP_OPA_MAD           0x00000020

/* Address format                       0x000FF000 */
#define RDMA_CORE_CAP_AF_IB             0x00001000
#define RDMA_CORE_CAP_ETH_AH            0x00002000

/* Protocol                             0xFFF00000 */
#define RDMA_CORE_CAP_PROT_IB           0x00100000
#define RDMA_CORE_CAP_PROT_ROCE         0x00200000
#define RDMA_CORE_CAP_PROT_IWARP        0x00400000

#define RDMA_CORE_P