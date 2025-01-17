OX__SIDEPORT_EXTRA_MASK 0xff0000
#define SEM_MAILBOX__SIDEPORT_EXTRA__SHIFT 0x10
#define SEM_MAILBOX__HOSTPORT_EXTRA_MASK 0xff000000
#define SEM_MAILBOX__HOSTPORT_EXTRA__SHIFT 0x18
#define SEM_MAILBOX_CONTROL__SIDEPORT_ENABLE_MASK 0xff
#define SEM_MAILBOX_CONTROL__SIDEPORT_ENABLE__SHIFT 0x0
#define SEM_MAILBOX_CONTROL__HOSTPORT_ENABLE_MASK 0xff00
#define SEM_MAILBOX_CONTROL__HOSTPORT_ENABLE__SHIFT 0x8
#define SEM_MAILBOX_CONTROL__SIDEPORT_ENABLE_EXTRA_MASK 0xff0000
#define SEM_MAILBOX_CONTROL__SIDEPORT_ENABLE_EXTRA__SHIFT 0x10
#define SEM_MAILBOX_CONTROL__HOSTPORT_ENABLE_EXTRA_MASK 0xff000000
#define SEM_MAILBOX_CONTROL__HOSTPORT_ENABLE_EXTRA__SHIFT 0x18
#define SEM_CHICKEN_BITS__VMID_PIPELINE_EN_MASK 0x1
#define SEM_CHICKEN_BITS__VMID_PIPELINE_EN__SHIFT 0x0
#define SEM_CHICKEN_BITS__ENTRY_PIPELINE_EN_MASK 0x2
#define SEM_CHICKEN_BITS__ENTRY_PIPELINE_EN__SHIFT 0x1
#define SEM_CHICKEN_BITS__CHECK_COUNTER_EN_MASK 0x4
#define SEM_CHICKEN_BITS__CHECK_COUNTER_EN__SHIFT 0x2
#define SEM_CHICKEN_BITS__ECC_BEHAVIOR_MASK 0x18
#define SEM_CHICKEN_BITS__ECC_BEHAVIOR__SHIFT 0x3
#define SEM_CHICKEN_BITS__SIGNAL_FAIL_MASK 0x20
#define SEM_CHICKEN_BITS__SIGNAL_FAIL__SHIFT 0x5
#define SEM_CHICKEN_BITS__PHY_TRAN_EN_MASK 0x40
#define SEM_CHICKEN_BITS__PHY_TRAN_EN__SHIFT 0x6
#define SEM_CHICKEN_BITS__ADDR_CMP_UNTRAN_EN_MASK 0x80
#define SEM_CHICKEN_BITS__ADDR_CMP_UNTRAN_EN__SHIFT 0x7
#define SEM_CHICKEN_BITS__IDLE_COUNTER_INDEX_MASK 0xf00
#define SEM_CHICKEN_BITS__IDLE_COUNTER_INDEX__SHIFT 0x8
#define SEM_CHICKEN_BITS__ATCL2_BUS_ID_MASK 0x3000
#define SEM_CHICKEN_BITS__ATCL2_BUS_ID__SHIFT 0xc
#define SEM_MAILBOX_CLIENTCONFIG_EXTRA__VCE1_CLIENT0_MASK 0x1f
#define SEM_MAILBOX_CLIENTCONFIG_EXTRA__VCE1_CLIENT0__SHIFT 0x0
#define SRBM_CNTL__PWR_REQUEST_HALT_MASK 0x10000
#define SRBM_CNTL__PWR_REQUEST_HALT__SHIFT 0x10
#define SRBM_CNTL__COMBINE_SYSTEM_MC_MASK 0x20000
#define SRBM_CNTL__COMBINE_SYSTEM_MC__SHIFT 0x11
#define SRBM_CNTL__REPORT_LAST_RDERR_MASK 0x40000
#define SRBM_CNTL__REPORT_LAST_RDERR__SHIFT 0x12
#define SRBM_CNTL__PWR_GFX3D_REQUEST_HALT_MASK 0x80000
#define SRBM_CNTL__PWR_GFX3D_REQUEST_HALT__SHIFT 0x13
#define SRBM_GFX_CNTL__PIPEID_MASK 0x3
#define SRBM_GFX_CNTL__PIPEID__SHIFT 0x0
#define SRBM_GFX_CNTL__MEID_MASK 0xc
#define SRBM_GFX_CNTL__MEID__SHIFT 0x2
#define SRBM_GFX_CNTL__VMID_MASK 0xf0
#define SRBM_GFX_CNTL__VMID__SHIFT 0x4
#define SRBM_GFX_CNTL__QUEUEID_MASK 0x700
#define SRBM_GFX_CNTL__QUEUEID__SHIFT 0x8
#define SRBM_READ_CNTL__READ_TIMEOUT_MASK 0xffffff
#define SRBM_READ_CNTL__READ_TIMEOUT__SHIFT 0x0
#define SRBM_STATUS2__SDMA_RQ_PENDING_MASK 0x1
#define SRBM_STATUS2__SDMA_RQ_PENDING__SHIFT 0x0
#define SRBM_STATUS2__TST_RQ_PENDING_MASK 0x2
#define SRBM_STATUS2__TST_RQ_PENDING__SHIFT 0x1
#define SRBM_STATUS2__SDMA1_RQ_PENDING_MASK 0x4
#define SRBM_STATUS2__SDMA1_RQ_PENDING__SHIFT 0x2
#define SRBM_STATUS2__VCE0_RQ_PENDING_MASK 0x8
#define SRBM_STATUS2__VCE0_RQ_PENDING__SHIFT 0x3
#define SRBM_STATUS2__VP8_BUSY_MASK 0x10
#define SRBM_STATUS2__VP8_BUSY__SHIFT 0x4
#define SRBM_STATUS2__SDMA_BUSY_MASK 0x20
#define SRBM_STATUS2__SDMA_BUSY__SHIFT 0x5
#define SRBM_STATUS2__SDMA1_BUSY_MASK 0x40
#define SRBM_STATUS2__SDMA1_BUSY__SHIFT 0x6
#define SRBM_STATUS2__VCE0_BUSY_MASK 0x80
#define SRBM_STATUS2__VCE0_BUSY__SHIFT 0x7
#define SRBM_STATUS2__XDMA_BUSY_MASK 0x100
#define SRBM_STATUS2__XDMA_BUSY__SHIFT 0x8
#define SRBM_STATUS2__CHUB_BUSY_MASK 0x200
#define SRBM_STATUS2__CHUB_BUSY__SHIFT 0x9
#define SRBM_STATUS2__SDMA2_BUSY_MASK 0x400
#define SRBM_STATUS2__SDMA2_BUSY__SHIFT 0xa
#define SRBM_STATUS2__SDMA3_BUSY_MASK 0x800
#define SRBM_STATUS2__SDMA3_BUSY__SHIFT 0xb
#define SRBM_STATUS2__ISP_BUSY_MASK 0x2000
#define SRBM_STATUS2__ISP_BUSY__SHIFT 0xd
#define SRBM_STATUS2__VCE1_BUSY_MASK 0x4000
#define SRBM_STATUS2__VCE1_BUSY__SHIFT 0xe
#define SRBM_STATUS2__ODE_BUSY_MASK 0x8000
#define SRBM_STATUS2__ODE_BUSY__SHIFT 0xf
#define SRBM_STATUS2__SDMA2_RQ_PENDING_MASK 0x10000
#define SRBM_STATUS2__SDMA2_RQ_PENDING__SHIFT 0x10
#define SRBM_STATUS2__SDMA3_RQ_PENDING_MASK 0x20000
#define SRBM_STATUS2__SDMA3_RQ_PENDING__SHIFT 0x11
#define SRBM_STATUS2__VP8_RQ_PENDING_MASK 0x40000
#define SRBM_STATUS2__VP8_RQ_PENDING__SHIFT 0x12
#define SRBM_STATUS2__ISP_RQ_PENDING_MASK 0x80000
#define SRBM_STATUS2__ISP_RQ_PENDING__SHIFT 0x13
#define SRBM_STATUS2__VCE1_RQ_PENDING_MASK 0x100000
#define SRBM_STATUS2__VCE1_RQ_PENDING__SHIFT 0x14
#define SRBM_STATUS__UVD_RQ_PENDING_MASK 0x2
#define SRBM_STATUS__UVD_RQ_PENDING__SHIFT 0x1
#define SRBM_STATUS__SAMMSP_RQ_PENDING_MASK 0x4
#define SRBM_STATUS__SAMMSP_RQ_PENDING__SHIFT 0x2
#define SRBM_STATUS__ACP_RQ_PENDING_MASK 0x8
#define SRBM_STATUS__ACP_RQ_PENDING__SHIFT 0x3
#define SRBM_STATUS__SMU_RQ_PENDING_MASK 0x10
#define SRBM_STATUS__SMU_RQ_PENDING__SHIFT 0x4
#define SRBM_STATUS__GRBM_RQ_PENDING_MASK 0x20
#define SRBM_STATUS__GRBM_RQ_PENDING__SHIFT 0x5
#define SRBM_STATUS__HI_RQ_PENDING_MASK 0x40
#define SRBM_STATUS__HI_RQ_PENDING__SHIFT 0x6
#define SRBM_STATUS__VMC_BUSY_MASK 0x100
#define SRBM_STATUS__VMC_BUSY__SHIFT 0x8
#define SRBM_STATUS__MCB_BUSY_MASK 0x200
#define SRBM_STATUS__MCB_BUSY__SHIFT 0x9
#define SRBM_STATUS__MCB_NON_DISPLAY_BUSY_MASK 0x400
#define SRBM_STATUS__MCB_NON_DISPLAY_BUSY__SHIFT 0xa
#define SRBM_STATUS__MCC_BUSY_MASK 0x800
#define SRBM_STATUS__MCC_BUSY__SHIFT 0xb
#define SRBM_STATUS__MCD_BUSY_MASK 0x1000
#define SRBM_STATUS__MCD_BUSY__SHIFT 0xc
#define SRBM_STATUS__VMC1_BUSY_MASK 0x2000
#define SRBM_STATUS__VMC1_BUSY__SHIFT 0xd
#define SRBM_STATUS__SEM_BUSY_MASK 0x4000
#define SRBM_STATUS__SEM_BUSY__SHIFT 0xe
#define SRBM_STATUS__ACP_BUSY_MASK 0x10000
#define SRBM_STATUS__ACP_BUSY__SHIFT 0x10
#define SRBM_STATUS__IH_BUSY_MASK 0x20000
#define SRBM_STATUS__IH_BUSY__SHIFT 0x11
#define SRBM_STATUS__UVD_BUSY_MASK 0x80000
#define SRBM_STATUS__UVD_BUSY__SHIFT 0x13
#define SRBM_STATUS__SAMMSP_BUSY_MASK 0x100000
#define SRBM_STATUS__SAMMSP_BUSY__SHIFT 0x14
#define SRBM_STATUS__GCATCL2_BUSY_MASK 0x200000
#define SRBM_STATUS__GCATCL2_BUSY__SHIFT 0x15
#define SRBM_STATUS__OSATCL2_BUSY_MASK 0x400000
#define SRBM_STATUS__OSATCL2_BUSY__SHIFT 0x16
#define SRBM_STATUS__BIF_BUSY_MASK 0x20000000
#define SRBM_STATUS__BIF_BUSY__SHIFT 0x1d
#define SRBM_STATUS3__MCC0_BUSY_MASK 0x1
#define SRBM_STATUS3__MCC0_BUSY__SHIFT 0x0
#define SRBM_STATUS3__MCC1_BUSY_MASK 0x2
#define SRBM_STATUS3__MCC1_BUSY__SHIFT 0x1
#define SRBM_STATUS3__MCC2_BUSY_MASK 0x4
#define SRBM_STATUS3__MCC2_BUSY__SHIFT 0x2
#define SRBM_STATUS3__MCC3_BUSY_MASK 0x8
#define SRBM_STATUS3__MCC3_BUSY__SHIFT 0x3
#define SRBM_STATUS3__MCC4_BUSY_MASK 0x10
#define SRBM_STATUS3__MCC4_BUSY__SHIFT 0x4
#define SRBM_STATUS3__MCC5_BUSY_MASK 0x20
#define SRBM_STATUS3__MCC5_BUSY__SHIFT 0x5
#define SRBM_STATUS3__MCC6_BUSY_MASK 0x40
#define SRBM_STATUS3__MCC6_BUSY__SHIFT 0x6
#define SRBM_STATUS3__MCC7_BUSY_MASK 0x80
#define SRBM_STATUS3__MCC7_BUSY__SHIFT 0x7
#define SRBM_STATUS3__MCD0_BUSY_MASK 0x100
#define SRBM_STATUS3__MCD0_BUSY__SHIFT 0x8
#define SRBM_STATUS3__MCD1_BUSY_MASK 0x200
#define SRBM_STATUS3__MCD1_BUSY__SHIFT 0x9
#define SRBM_STATUS3__MCD2_BUSY_MASK 0x400
#define SRBM_STATUS3__MCD2_BUSY__SHIFT 0xa
#define SRBM_STATUS3__MCD3_BUSY_MASK 0x800
#define SRBM_STATUS3__MCD3_BUSY__SHIFT 0xb
#define SRBM_STATUS3__MCD4_BUSY_MASK 0x1000
#define SRBM_STATUS3__MCD4_BUSY__SHIFT 0xc
#define SRBM_STATUS3__MCD5_BUSY_MASK 0x2000
#define SRBM_STATUS3__MCD5_BUSY__SHIFT 0xd
#define SRBM_STATUS3__MCD6_BUSY_MASK 0x4000
#define SRBM_STATUS3__MCD6_BUSY__SHIFT 0xe
#define SRBM_STATUS3__MCD7_BUSY_MASK 0x8000
#define SRBM_STATUS3__MCD7_BUSY__SHIFT 0xf
#define SRBM_SOFT_RESET__SOFT_RESET_ATCL2_MASK 0x1
#define SRBM_SOFT_RESET__SOFT_RESET_ATCL2__SHIFT 0x0
#define SRBM_SOFT_RESET__SOFT_RESET_BIF_MASK 0x2
#define SRBM_SOFT_RESET__SOFT_RESET_BIF__SHIFT 0x1
#define SRBM_SOFT_RESET__SOFT_RESET_SDMA3_MASK 0x4
#define SRBM_SOFT_RESET__SOFT_RESET_SDMA3__SHIFT 0x2
#define SRBM_SOFT_RESET__SOFT_RESET_SDMA2_MASK 0x8
#define SRBM_SOFT_RESET__SOFT_RESET_SDMA2__SHIFT 0x3
#define SRBM_SOFT_RESET__SOFT_RESET_GIONB_MASK 0x10
#define SRBM_SOFT_RESET__SOFT_RESET_GIONB__SHIFT 0x4
#define SRBM_SOFT_RESET__SOFT_RESET_DC_MASK 0x20
#define SRBM_SOFT_RESET__SOFT_RESET_DC__SHIFT 0x5
#define SRBM_SOFT_RESET__SOFT_RESET_SDMA1_MASK 0x40
#define SRBM_SOFT_RESET__SOFT_RESET_SDMA1__SHIFT 0x6
#define SRBM_SOFT_RESET__SOFT_RESET_GRBM_MASK 0x100
#define SRBM_SOFT_RESET__SOFT_RESET_GRBM__SHIFT 0x8
#define SRBM_SOFT_RESET__SOFT_RESET_HDP_MASK 0x200
#define SRBM_SOFT_RESET__SOFT_RESET_HDP__SHIFT 0x9
#define SRBM_SOFT_RESET__SOFT_RESET_IH_MASK 0x400
#define SRBM_SOFT_RESET__SOFT_RESET_IH__SHIFT 0xa
#define SRBM_SOFT_RESET__SOFT_RESET_MC_MASK 0x800
#define SRBM_SOFT_RESET__SOFT_RESET_MC__SHIFT 0xb
#define SRBM_SOFT_RESET__SOFT_RESET_CHUB_MASK 0x1000
#define SRBM_SOFT_RESET__SOFT_RESET_CHUB__SHIFT 0xc
#define SRBM_SOFT_RESET__SOFT_RESET_ESRAM_MASK 0x2000
#define SRBM_SOFT_RESET__SOFT_RESET_ESRAM__SHIFT 0xd
#define SRBM_SOFT_RESET__SOFT_RESET_ROM_MASK 0x4000
#define SRBM_SOFT_RESET__SOFT_RESET_ROM__SHIFT 0xe
#define SRBM_SOFT_RESET__SOFT_RESET_SEM_MASK 0x8000
#define SRBM_SOFT_RESET__SOFT_RESET_SEM__SHIFT 0xf
#define SRBM_SOFT_RESET__SOFT_RESET_SMU_MASK 0x10000
#define SRBM_SOFT_RESET__SOFT_RESET_SMU__SHIFT 0x10
#define SRBM_SOFT_RESET__SOFT_RESET_VMC_MASK 0x20000
#define SRBM_SOFT_RESET__SOFT_RESET_VMC__SHIFT 0x11
#define SRBM_SOFT_RESET__SOFT_RESET_UVD_MASK 0x40000
#define SRBM_SOFT_RESET__SOFT_RESET_UVD__SHIFT 0x12
#define SRBM_SOFT_RESET__SOFT_RESET_VP8_MASK 0x80000
#define SRBM_SOFT_RESET__SOFT_RESET_VP8__SHIFT 0x13
#define SRBM_SOFT_RESET__SOFT_RESET_SDMA_MASK 0x100000
#define SRBM_SOFT_RESET__SOFT_RESET_SDMA__SHIFT 0x14
#define SRBM_SOFT_RESET__SOFT_RESET_TST_MASK 0x200000
#define SRBM_SOFT_RESET__SOFT_RESET_TST__SHIFT 0x15
#define SRBM_SOFT_RESET__SOFT_RESET_REGBB_MASK 0x400000
#define SRBM_SOFT_RESET__SOFT_RESET_REGBB__SHIFT 0x16
#define SRBM_SOFT_RESET__SOFT_RESET_ODE_MASK 0x800000
#define SRBM_SOFT_RESET__SOFT_RESET_ODE__SHIFT 0x17
#define SRBM_SOFT_RESET__SOFT_RESET_VCE0_MASK 0x1000000
#define SRBM_SOFT_RESET__SOFT_RESET_VCE0__SHIFT 0x18
#define SRBM_SOFT_RESET__SOFT_RESET_XDMA_MASK 0x2000000
#define SRBM_SOFT_RESET__SOFT_RESET_XDMA__SHIFT 0x19
#define SRBM_SOFT_RESET__SOFT_RESET_ACP_MASK 0x4000000
#define SRBM_SOFT_RESET__SOFT_RESET_ACP__SHIFT 0x1a
#define SRBM_SOFT_RESET__SOFT_RESET_SAMMSP_MASK 0x8000000
#define SRBM_SOFT_RESET__SOFT_RESET_SAMMSP__SHIFT 0x1b
#define SRBM_SOFT_RESET__SOFT_RESET_GRN_MASK 0x20000000
#define SRBM_SOFT_RESET__SOFT_RESET_GRN__SHIFT 0x1d
#define SRBM_SOFT_RESET__SOFT_RESET_ISP_MASK 0x40000000
#define SRBM_SOFT_RESET__SOFT_RESET_ISP__SHIFT 0x1e
#define SRBM_SOFT_RESET__SOFT_RESET_VCE1_MASK 0x80000000
#define SRBM_SOFT_RESET__SOFT_RESET_VCE1__SHIFT 0x1f
#define SRBM_DEBUG_CNTL__SRBM_DEBUG_INDEX_MASK 0x3f
#define SRBM_DEBUG_CNTL__SRBM_DEBUG_INDEX__SHIFT 0x0
#define SRBM_DEBUG_DATA__DATA_MASK 0xffffffff
#define SRBM_DEBUG_DATA__DATA__SHIFT 0x0
#define SRBM_CHIP_REVISION__CHIP_REVISION_MASK 0xff
#define SRBM_CHIP_REVISION__CHIP_REVISION__SHIFT 0x0
#define CC_SYS_RB_REDUNDANCY__FAILED_RB0_MASK 0xf00
#define CC_SYS_RB_REDUNDANCY__FAILED_RB0__SHIFT 0x8
#define CC_SYS_RB_REDUNDANCY__EN_REDUNDANCY0_MASK 0x1000
#define CC_SYS_RB_REDUNDANCY__EN_REDUNDANCY0__SHIFT 0xc
#define CC_SYS_RB_REDUNDANCY__FAILED_RB1_MASK 0xf0000
#define CC_SYS_RB_REDUNDANCY__FAILED_RB1__SHIFT 0x10
#define CC_SYS_RB_REDUNDANCY__EN_REDUNDANCY1_MASK 0x100000
#define CC_SYS_RB_REDUNDANCY__EN_REDUNDANCY1__SHIFT 0x14
#define CC_SYS_RB_BACKEND_DISABLE__BACKEND_DISABLE_MASK 0xff0000
#define CC_SYS_RB_BACKEND_DISABLE__BACKEND_DISABLE__SHIFT 0x10
#define GC_USER_SYS_RB_BACKEND_DISABLE__BACKEND_DISABLE_MASK 0xff0000
#define GC_USER_SYS_RB_BACKEND_DISABLE__BACKEND_DISABLE__SHIFT 0x10
#define SRBM_MC_CLKEN_CNTL__PREFIX_DELAY_CNT_MASK 0xf
#define SRBM_MC_CLKEN_CNTL__PREFIX_DELAY_CNT__SHIFT 0x0
#define SRBM_MC_CLKEN_CNTL__POST_DELAY_CNT_MASK 0x1f00
#define SRBM_MC_CLKEN_CNTL__POST_DELAY_CNT__SHIFT 0x8
#define SRBM_SYS_CLKEN_CNTL__PREFIX_DELAY_CNT_MASK 0xf
#define SRBM_SYS_CLKEN_CNTL__PREFIX_DELAY_CNT__SHIFT 0x0
#define SRBM_SYS_CLKEN_CNTL__POST_DELAY_CNT_MASK 0x1f00
#define SRBM_SYS_CLKEN_CNTL__POST_DELAY_CNT__SHIFT 0x8
#define SRBM_VCE_CLKEN_CNTL__PREFIX_DELAY_CNT_MASK 0xf
#define SRBM_VCE_CLKEN_CNTL__PREFIX_DELAY_CNT__SHIFT 0x0
#define SRBM_VCE_CLKEN_CNTL__POST_DELAY_CNT_MASK 0x1f00
#define SRBM_VCE_CLKEN_CNTL__POST_DELAY_CNT__SHIFT 0x8
#define SRBM_UVD_CLKEN_CNTL__PREFIX_DELAY_CNT_MASK 0xf
#define SRBM_UVD_CLKEN_CNTL__PREFIX_DELAY_CNT__SHIFT 0x0
#define SRBM_UVD_CLKEN_CNTL__POST_DELAY_CNT_MASK 0x1f00
#define SRBM_UVD_CLKEN_CNTL__POST_DELAY_CNT__SHIFT 0x8
#define SRBM_SDMA_CLKEN_CNTL__PREFIX_DELAY_CNT_MASK 0xf
#define SRBM_SDMA_CLKEN_CNTL__PREFIX_DELAY_CNT__SHIFT 0x0
#define SRBM_SDMA_CLKEN_CNTL__POST_DELAY_CNT_MASK 0x1f00
#define SRBM_SDMA_CLKEN_CNTL__POST_DELAY_CNT__SHIFT 0x8
#define SRBM_SAM_CLKEN_CNTL__PREFIX_DELAY_CNT_MASK 0xf
#define SRBM_SAM_CLKEN_CNTL__PREFIX_DELAY_CNT__SHIFT 0x0
#define SRBM_SAM_CLKEN_CNTL__POST_DELAY_CNT_MASK 0x1f00
#define SRBM_SAM_CLKEN_CNTL__POST_DELAY_CNT__SHIFT 0x8
#define SRBM_ISP_CLKEN_CNTL__PREFIX_DELAY_CNT_MASK 0xf
#define SRBM_ISP_CLKEN_CNTL__PREFIX_DELAY_CNT__SHIFT 0x0
#define SRBM_ISP_CLKEN_CNTL__POST_DELAY_CNT_MASK 0x1f00
#define SRBM_ISP_CLKEN_CNTL__POST_DELAY_CNT__SHIFT 0x8
#define SRBM_VP8_CLKEN_CNTL__PREFIX_DELAY_CNT_MASK 0xf
#define SRBM_VP8_CLKEN_CNTL__PREFIX_DELAY_CNT__SHIFT 0x0
#define SRBM_VP8_CLKEN_CNTL__POST_DELAY_CNT_MASK 0x1f00
#define SRBM_VP8_CLKEN_CNTL__POST_DELAY_CNT__SHIFT 0x8
#define SRBM_DEBUG__IGNORE_RDY_MASK 0x1
#define SRBM_DEBUG__IGNORE_RDY__SHIFT 0x0
#define SRBM_DEBUG__DISABLE_READ_TIMEOUT_MASK 0x2
#define SRBM_DEBUG__DISABLE_READ_TIMEOUT__SHIFT 0x1
#define SRBM_DEBUG__SNAPSHOT_FREE_CNTRS_MASK 0x4
#define SRBM_DEBUG__SNAPSHOT_FREE_CNTRS__SHIFT 0x2
#define SRBM_DEBUG__SYS_CLOCK_DOMAIN_OVERRIDE_MASK 0x10
#define SRBM_DEBUG__SYS_CLOCK_DOMAIN_OVERRIDE__SHIFT 0x4
#define SRBM_DEBUG__VCE_CLOCK_DOMAIN_OVERRIDE_MASK 0x20
#define SRBM_DEBUG__VCE_CLOCK_DOMAIN_OVERRIDE__SHIFT 0x5
#define SRBM_DEBUG__UVD_CLOCK_DOMAIN_OVERRIDE_MASK 0x40
#define SRBM_DEBUG__UVD_CLOCK_DOMAIN_OVERRIDE__SHIFT 0x6
#define SRBM_DEBUG__SDMA_CLOCK_DOMAIN_OVERRIDE_MASK 0x80
#define SRBM_DEBUG__SDMA_CLOCK_DOMAIN_OVERRIDE__SHIFT 0x7
#define SRBM_DEBUG__MC_CLOCK_DOMAIN_OVERRIDE_MASK 0x100
#define SRBM_DEBUG__MC_CLOCK_DOMAIN_OVERRIDE__SHIFT 0x8
#define SRBM_DEBUG__SAM_CLOCK_DOMAIN_OVERRIDE_MASK 0x200
#define SRBM_DEBUG__SAM_CLOCK_DOMAIN_OVERRIDE__SHIFT 0x9
#define SRBM_DEBUG__ISP_CLOCK_DOMAIN_OVERRIDE_MASK 0x400
#define SRBM_DEBUG__ISP_CLOCK_DOMAIN_OVERRIDE__SHIFT 0xa
#define SRBM_DEBUG__VP8_CLOCK_DOMAIN_OVERRIDE_MASK 0x800
#define SRBM_DEBUG__VP8_CLOCK_DOMAIN_OVERRIDE__SHIFT 0xb
#define SRBM_DEBUG_SNAPSHOT__MCB_RDY_MASK 0x1
#define SRBM_DEBUG_SNAPSHOT__MCB_RDY__SHIFT 0x0
#define SRBM_DEBUG_SNAPSHOT__GIONB_RDY_MASK 0x2
#define SRBM_DEBUG_SNAPSHOT__GIONB_RDY__SHIFT 0x1
#define SRBM_DEBUG_SNAPSHOT__SMU_RDY_MASK 0x4
#define SRBM_DEBUG_SNAPSHOT__SMU_RDY__SHIFT 0x2
#define SRBM_DEBUG_SNAPSHOT__SAMMSP_RDY_MASK 0x8
#define SRBM_DEBUG_SNAPSHOT__SAMMSP_RDY__SHIFT 0x3
#define SRBM_DEBUG_SNAPSHOT__ACP_RDY_MASK 0x10
#define SRBM_DEBUG_SNAPSHOT__ACP_RDY__SHIFT 0x4
#define SRBM_DEBUG_SNAPSHOT__GRBM_RDY_MASK 0x20
#define SRBM_DEBUG_SNAPSHOT__GRBM_RDY__SHIFT 0x5
#define SRBM_DEBUG_SNAPSHOT__DC_RDY_MASK 0x40
#define SRBM_DEBUG_SNAPSHOT__DC_RDY__SHIFT 0x6
#define SRBM_DEBUG_SNAPSHOT__BIF_RDY_MASK 0x80
#define SRBM_DEBUG_SNAPSHOT__BIF_RDY__SHIFT 0x7
#define SRBM_DEBUG_SNAPSHOT__XDMA_RDY_MASK 0x100
#define SRBM_DEBUG_SNAPSHOT__XDMA_RDY__SHIFT 0x8
#define SRBM_DEBUG_SNAPSHOT__UVD_RDY_MASK 0x200
#define SRBM_DEBUG_SNAPSHOT__UVD_RDY__SHIFT 0x9
#define SRBM_DEBUG_SNAPSHOT__VP8_RDY_MASK 0x400
#define SRBM_DEBUG_SNAPSHOT__VP8_RDY__SHIFT 0xa
#define SRBM_DEBUG_SNAPSHOT__REGBB_RDY_MASK 0x800
#define SRBM_DEBUG_SNAPSHOT__REGBB_RDY__SHIFT 0xb
#define SRBM_DEBUG_SNAPSHOT__ODE_RDY_MASK 0x1000
#define SRBM_DEBUG_SNAPSHOT__ODE_RDY__SHIFT 0xc
#define SRBM_DEBUG_SNAPSHOT__MCD7_RDY_MASK 0x2000
#define SRBM_DEBUG_SNAPSHOT__MCD7_RDY__SHIFT 0xd
#define SRBM_DEBUG_SNAPSHOT__MCD6_RDY_MASK 0x4000
#define SRBM_DEBUG_SNAPSHOT__MCD6_RDY__SHIFT 0xe
#define SRBM_DEBUG_SNAPSHOT__MCD5_RDY_MASK 0x8000
#define SRBM_DEBUG_SNAPSHOT__MCD5_RDY__SHIFT 0xf
#define SRBM_DEBUG_SNAPSHOT__MCD4_RDY_MASK 0x10000
#define SRBM_DEBUG_SNAPSHOT__MCD4_RDY__SHIFT 0x10
#define SRBM_DEBUG_SNAPSHOT__MCD3_RDY_MASK 0x20000
#define SRBM_DEBUG_SNAPSHOT__MCD3_RDY__SHIFT 0x11
#define SRBM_DEBUG_SNAPSHOT__MCD2_RDY_MASK 0x40000
#define SRBM_DEBUG_SNAPSHOT__MCD2_RDY__SHIFT 0x12
#define SRBM_DEBUG_SNAPSHOT__MCD1_RDY_MASK 0x80000
#define SRBM_DEBUG_SNAPSHOT__MCD1_RDY__SHIFT 0x13
#define SRBM_DEBUG_SNAPSHOT__MCD0_RDY_MASK 0x100000
#define SRBM_DEBUG_SNAPSHOT__MCD0_RDY__SHIFT 0x14
#define SRBM_DEBUG_SNAPSHOT__MCC7_RDY_MASK 0x200000
#define SRBM_DEBUG_SNAPSHOT__MCC7_RDY__SHIFT 0x15
#define SRBM_DEBUG_SNAPSHOT__MCC6_RDY_MASK 0x400000
#define SRBM_DEBUG_SNAPSHOT__MCC6_RDY__SHIFT 0x16
#define SRBM_DEBUG_SNAPSHOT__MCC5_RDY_MASK 0x800000
#define SRBM_DEBUG_SNAPSHOT__MCC5_RDY__SHIFT 0x17
#define SRBM_DEBUG_SNAPSHOT__MCC4_RDY_MASK 0x1000000
#define SRBM_DEBUG_SNAPSHOT__MCC4_RDY__SHIFT 0x18
#define SRBM_DEBUG_SNAPSHOT__MCC3_RDY_MASK 0x2000000
#define SRBM_DEBUG_SNAPSHOT__MCC3_RDY__SHIFT 0x19
#define SRBM_DEBUG_SNAPSHOT__MCC2_RDY_MASK 0x4000000
#define SRBM_DEBUG_SNAPSHOT__MCC2_RDY__SHIFT 0x1a
#define SRBM_DEBUG_SNAPSHOT__MCC1_RDY_MASK 0x8000000
#define SRBM_DEBUG_SNAPSHOT__MCC1_RDY__SHIFT 0x1b
#define SRBM_DEBUG_SNAPSHOT__MCC0_RDY_MASK 0x10000000
#define SRBM_DEBUG_SNAPSHOT__MCC0_RDY__SHIFT 0x1c
#define SRBM_DEBUG_SNAPSHOT__VCE0_RDY_MASK 0x20000000
#define SRBM_DEBUG_SNAPSHOT__VCE0_RDY__SHIFT 0x1d
#define SRBM_DEBUG_SNAPSHOT__RESERVED_MASK 0x40000000
#define SRBM_DEBUG_SNAPSHOT__RESERVED__SHIFT 0x1e
#define SRBM_DEBUG_SNAPSHOT__ISP_RDY_MASK 0x80000000
#define SRBM_DEBUG_SNAPSHOT__ISP_RDY__SHIFT 0x1f
#defi