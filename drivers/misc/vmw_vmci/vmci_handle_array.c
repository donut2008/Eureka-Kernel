ITY__SHIFT 0x15
#define PCIE_UNCORR_ERR_SEVERITY__UNCORR_INT_ERR_SEVERITY_MASK 0x400000
#define PCIE_UNCORR_ERR_SEVERITY__UNCORR_INT_ERR_SEVERITY__SHIFT 0x16
#define PCIE_UNCORR_ERR_SEVERITY__MC_BLOCKED_TLP_SEVERITY_MASK 0x800000
#define PCIE_UNCORR_ERR_SEVERITY__MC_BLOCKED_TLP_SEVERITY__SHIFT 0x17
#define PCIE_UNCORR_ERR_SEVERITY__ATOMICOP_EGRESS_BLOCKED_SEVERITY_MASK 0x1000000
#define PCIE_UNCORR_ERR_SEVERITY__ATOMICOP_EGRESS_BLOCKED_SEVERITY__SHIFT 0x18
#define PCIE_UNCORR_ERR_SEVERITY__TLP_PREFIX_BLOCKED_ERR_SEVERITY_MASK 0x2000000
#define PCIE_UNCORR_ERR_SEVERITY__TLP_PREFIX_BLOCKED_ERR_SEVERITY__SHIFT 0x19
#define PCIE_CORR_ERR_STATUS__RCV_ERR_STATUS_MASK 0x1
#define PCIE_CORR_ERR_STATUS__RCV_ERR_STATUS__SHIFT 0x0
#define PCIE_CORR_ERR_STATUS__BAD_TLP_STATUS_MASK 0x40
#define PCIE_CORR_ERR_STATUS__BAD_TLP_STATUS__SHIFT 0x6
#define PCIE_CORR_ERR_STATUS__BAD_DLLP_STATUS_MASK 0x80
#define PCIE_CORR_ERR_STATUS__BAD_DLLP_STATUS__SHIFT 0x7
#define PCIE_CORR_ERR_STATUS__REPLAY_NUM_ROLLOVER_STATUS_MASK 0x100
#define PCIE_CORR_ERR_STATUS__REPLAY_NUM_ROLLOVER_STATUS__SHIFT 0x8
#define PCIE_CORR_ERR_STATUS__REPLAY_TIMER_TIMEOUT_STATUS_MASK 0x1000
#define PCIE_CORR_ERR_STATUS__REPLAY_TIMER_TIMEOUT_STATUS__SHIFT 0xc
#define PCIE_CORR_ERR_STATUS__ADVISORY_NONFATAL_ERR_STATUS_MASK 0x2000
#define PCIE_CORR_ERR_STATUS__ADVISORY_NONFATAL_ERR_STATUS__SHIFT 0xd
#define PCIE_CORR_ERR_STATUS__CORR_INT_ERR_STATUS_MASK 0x4000
#define PCIE_CORR_ERR_STATUS__CORR_INT_ERR_STATUS__SHIFT 0xe
#define PCIE_CORR_ERR_STATUS__HDR_LOG_OVFL_STATUS_MASK 0x8000
#define PCIE_CORR_ERR_STATUS__HDR_LOG_OVFL_STATUS__SHIFT 0xf
#define PCIE_CORR_ERR_MASK__RCV_ERR_MASK_MASK 0x1
#define PCIE_CORR_ERR_MASK__RCV_ERR_MASK__SHIFT 0x0
#define PCIE_CORR_ERR_MASK__BAD_TLP_MASK_MASK 0x40
#define PCIE_CORR_ERR_MASK__BAD_TLP_MASK__SHIFT 0x6
#define PCIE_CORR_ERR_MASK__BAD_DLLP_MASK_MASK 0x80
#define PCIE_CORR_ERR_MASK__BAD_DLLP_MASK__SHIFT 0x7
#define PCIE_CORR_ERR_MASK__REPLAY_NUM_ROLLOVER_MASK_MASK 0x100
#define PCIE_CORR_ERR_MASK__REPLAY_NUM_ROLLOVER_MASK__SHIFT 0x8
#define PCIE_CORR_ERR_MASK__REPLAY_TIMER_TIMEOUT_MASK_MASK 0x1000
#define PCIE_CORR_ERR_MASK__REPLAY_TIMER_TIMEOUT_MASK__SHIFT 0xc
#define PCIE_CORR_ERR_MASK__ADVISORY_NONFATAL_ERR_MASK_MASK 0x2000
#define PCIE_CORR_ERR_MASK__ADVISORY_NONFATAL_ERR_MASK__SHIFT 0xd
#define PCIE_CORR_ERR_MASK__CORR_INT_ERR_MASK_MASK 0x4000
#define PCIE_CORR_ERR_MASK__CORR_INT_ERR_MASK__SHIFT 0xe
#define PCIE_CORR_ERR_MASK__HDR_LOG_OVFL_MASK_MASK 0x8000
#define PCIE_CORR_ERR_MASK__HDR_LOG_OVFL_MASK__SHIFT 0xf
#define PCIE_ADV_ERR_CAP_CNTL__FIRST_ERR_PTR_MASK 0x1f
#define PCIE_ADV_ERR_CAP_CNTL__FIRST_ERR_PTR__SHIFT 0x0
#define PCIE_ADV_ERR_CAP_CNTL__ECRC_GEN_CAP_MASK 0x20
#define PCIE_ADV_ERR_CAP_CNTL__ECRC_GEN_CAP__SHIFT 0x5
#define PCIE_ADV_ERR_CAP_CNTL__ECRC_GEN_EN_MASK 0x40
#define PCIE_ADV_ERR_CAP_CNTL__ECRC_GEN_EN__SHIFT 0x6
#define PCIE_ADV_ERR_CAP_CNTL__ECRC_CHECK_CAP_MASK 0x80
#define PCIE_ADV_ERR_CAP_CNTL__ECRC_CHECK_CAP__SHIFT 0x7
#define PCIE_ADV_ERR_CAP_CNTL__ECRC_CHECK_EN_MASK 0x100
#define PCIE_ADV_ERR_CAP_CNTL__ECRC_CHECK_EN__SHIFT 0x8
#define PCIE_ADV_ERR_CAP_CNTL__MULTI_HDR_RECD_CAP_MASK 0x200
#define PCIE_ADV_ERR_CAP_CNTL__MULTI_HDR_RECD_CAP__SHIFT 0x9
#define PCIE_ADV_ERR_CAP_CNTL__MULTI_HDR_RECD_EN_MASK 0x400
#define PCIE_ADV_ERR_CAP_CNTL__MULTI_HDR_RECD_EN__SHIFT 0xa
#define PCIE_ADV_ERR_CAP_CNTL__TLP_PREFIX_LOG_PRESENT_MASK 0x800
#define PCIE_ADV_ERR_CAP_CNTL__TLP_PREFIX_LOG_PRESENT__SHIFT 0xb
#define PCIE_HDR_LOG0__TLP_HDR_MASK 0xffffffff
#define PCIE_HDR_LOG0__TLP_HDR__SHIFT 0x0
#define PCIE_HDR_LOG1__TLP_HDR_MASK 0xffffffff
#define PCIE_HDR_LOG1__TLP_HDR__SHIFT 0x0
#define PCIE_HDR_LOG2__TLP_HDR_MASK 0xffffffff
#define PCIE_HDR_LOG2__TLP_HDR__SHIFT 0x0
#defin