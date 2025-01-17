TCH_IMPCTL_CAL_DONE_DIS_MASK 0x80
#define BIF_FEATURES_CONTROL_MISC__PLL_SWITCH_IMPCTL_CAL_DONE_DIS__SHIFT 0x7
#define BIF_FEATURES_CONTROL_MISC__IGNORE_BE_CHECK_GASKET_COMB_DIS_MASK 0x100
#define BIF_FEATURES_CONTROL_MISC__IGNORE_BE_CHECK_GASKET_COMB_DIS__SHIFT 0x8
#define BIF_FEATURES_CONTROL_MISC__MC_BIF_REQ_ID_ROUTING_DIS_MASK 0x200
#define BIF_FEATURES_CONTROL_MISC__MC_BIF_REQ_ID_ROUTING_DIS__SHIFT 0x9
#define BIF_FEATURES_CONTROL_MISC__AZ_BIF_REQ_ID_ROUTING_DIS_MASK 0x400
#define BIF_FEATURES_CONTROL_MISC__AZ_BIF_REQ_ID_ROUTING_DIS__SHIFT 0xa
#define BIF_FEATURES_CONTROL_MISC__ATC_PRG_RESP_PASID_UR_EN_MASK 0x800
#define BIF_FEATURES_CONTROL_MISC__ATC_PRG_RESP_PASID_UR_EN__SHIFT 0xb
#define BIF_DOORBELL_CNTL__SELF_RING_DIS_MASK 0x1
#define BIF_DOORBELL_CNTL__SELF_RING_DIS__SHIFT 0x0
#define BIF_DOORBELL_CNTL__TRANS_CHECK_DIS_MASK 0x2
#define BIF_DOORBELL_CNTL__TRANS_CHECK_DIS__SHIFT 0x1
#define BIF_DOORBELL_CNTL__UNTRANS_LBACK_EN_MASK 0x4
#define BIF_DOORBELL_CNTL__UNTRANS_LBACK_EN__SHIFT 0x2
#define BIF_DOORBELL_CNTL__NON_CONSECUTIVE_BE_ZERO_DIS_MASK 0x8
#define BIF_DOORBELL_CNTL__NON_CONSECUTIVE_BE_ZERO_DIS__SHIFT 0x3
#define BIF_SLVARB_MODE__SLVARB_MODE_MASK 0x3
#define BIF_SLVARB_MODE__SLVARB_MODE__SHIFT 0x0
#define BIF_FB_EN__FB_READ_EN_MASK 0x1
#define BIF_FB_EN__FB_READ_EN__SHIFT 0x0
#define BIF_FB_EN__FB_WRITE_EN_MASK 0x2
#define BIF_FB_EN__FB_WRITE_EN__SHIFT 0x1
#define BIF_BUSNUM_CNTL1__ID_MASK_MASK 0xff
#define BIF_BUSNUM_CNTL1__ID_MASK__SHIFT 0x0
#define BIF_BUSNUM_LIST0__ID0_MASK 0xff
#define BIF_BUSNUM_LIST0__ID0__SHIFT 0x0
#define BIF_BUSNUM_LIST0__ID1_MASK 0xff00
#define BIF_BUSNUM_LIST0__ID1__SHIFT 0x8
#define BIF_BUSNUM_LIST0__ID2_MASK 0xff0000
#define BIF_BUSNUM_LIST0__ID2__SHIFT 0x10
#define BIF_BUSNUM_LIST0__ID3_MASK 0xff000000
#define BIF_BUSNUM_LIST0__ID3__SHIFT 0x18
#define BIF_BUSNUM_LIST1__ID4_MASK 0xff
#define BIF_BUSNUM_LIST1__ID4__SHIFT 0x0
#define BIF_BUSNUM_LIST1__ID5_MASK 0xff00
#define BIF_BUSNUM_LIST1__ID5__SHIFT 0x8
#define BIF_BUSNUM_LIST1__ID6_MASK 0xff0000
#define BIF_BUSNUM_LIST1__ID6__SHIFT 0x10
#define BIF_BUSNUM_LIST1__ID7_MASK 0xff000000
#define BIF_BUSNUM_LIST1__ID7__SHIFT 0x18
#define BIF_BUSNUM_CNTL2__AUTOUPDATE_SEL_MASK 0xff
#define BIF_BUSNUM_CNTL2__AUTOUPDATE_SEL__SHIFT 0x0
#define BIF_BUSNUM_CNTL2__AUTOUPDATE_EN_MASK 0x100
#define BIF_BUSNUM_CNTL2__AUTOUPDATE_EN__SHIFT 0x8
#define BIF_BUSNUM_CNTL2__HDPREG_CNTL_MASK 0x10000
#define BIF_BUSNUM_CNTL2__HDPREG_CNTL__SHIFT 0x10
#define BIF_BUSNUM_CNTL2__ERROR_MULTIPLE_ID_MATCH_MASK 0x20000
#define BIF_BUSNUM_CNTL2__ERROR_MULTIPLE_ID_MATCH__SHIFT 0x11
#define BIF_BUSY_DELAY_CNTR__DELAY_CNT_MASK 0x3f
#define BIF_BUSY_DELAY_CNTR__DELAY_CNT__SHIFT 0x0
#define BIF_PERFMON_CNTL__PERFCOUNTER_EN_MASK 0x1
#define BIF_PERFMON_CNTL__PERFCOUNTER_EN__SHIFT 0x0
#define BIF_PERFMON_CNTL__PERFCOUNTER_RESET0_MASK 0x2
#define BIF_PERFMON_CNTL__PERFCOUNTER_RESET0__SHIFT 0x1
#define BIF_PERFMON_CNTL__PERFCOUNTER_RESET1_MASK 0x4
#define BIF_PERFMON_CNTL__PERFCOUNTER_RESET1__SHIFT 0x2
#define BIF_PERFMON_CNTL__PERF_SEL0_MASK 0x1f00
#define BIF_PERFMON_CNTL__PERF_SEL0__SHIFT 0x8
#define BIF_PERFMON_CNTL__PERF_SEL1_MASK 0x3e000
#define BIF_PERFMON_CNTL__PERF_SEL1__SHIFT 0xd
#define BIF_PERFCOUNTER0_RESULT__PERFCOUNTER_RESULT_MASK 0xffffffff
#define BIF_PERFCOUNTER0_RESULT__PERFCOUNTER_RESULT__SHIFT 0x0
#define BIF_PERFCOUNTER1_RESULT__PERFCOUNTER_RESULT_MASK 0xffffffff
#define BIF_PERFCOUNTER1_RESULT__PERFCOUNTER_RESULT__SHIFT 0x0
#define SLAVE_HANG_PROTECTION_CNTL__HANG_PROTECTION_TIMER_SEL_MASK 0xe
#define SLAVE_HANG_PROTECTION_CNTL__HANG_PROTECTION_TIMER_SEL__SHIFT 0x1
#define GPU_HDP_FLUSH_REQ__CP0_MASK 0x1
#define GPU_HDP_FLUSH_REQ__CP0__SHIFT 0x0
#define GPU_HDP_FLUSH_REQ__CP1_MASK 0x2
#define GPU_HDP_FLUSH_REQ__CP1__SHIFT 0x1
#define GPU_HDP_FLUSH_REQ__CP2_MASK 0x4
#define GPU_HDP_FLUSH_REQ__CP2__SHIFT 0x2
#define GPU_HDP_FLUSH_REQ__CP3_MASK 0x8
#define GPU_HDP_FLUSH_REQ__CP3__SHIFT 0x3
#define GPU_HDP_FLUSH_REQ__CP4_MASK 0x10
#define GPU_HDP_FLUSH_REQ__CP4__SHIFT 0x4
#define GPU_HDP_FLUSH_REQ__CP5_MASK 0x20
#define GPU_HDP_FLUSH_REQ__CP5__SHIFT 0x5
#define GPU_HDP_FLUSH_REQ__CP6_MASK 0x40
#define GPU_HDP_FLUSH_REQ__CP6__SHIFT 0x6
#define GPU_HDP_FLUSH_REQ__CP7_MASK 0x80
#define GPU_HDP_FLUSH_REQ__CP7__SHIFT 0x7
#define GPU_HDP_FLUSH_REQ__CP8_MASK 0x100
#define GPU_HDP_FLUSH_REQ__CP8__SHIFT 0x8
#define GPU_HDP_FLUSH_REQ__CP9_MASK 0x200
#define GPU_HDP_FLUSH_REQ__CP9__SHIFT 0x9
#define GPU_HDP_FLUSH_REQ__SDMA0_MASK 0x400
#define GPU_HDP_FLUSH_REQ__SDMA0__SHIFT 0xa
#define GPU_HDP_FLUSH_REQ__SDMA1_MASK 0x800
#define GPU_HDP_FLUSH_REQ__SDMA1__SHIFT 0xb
#define GPU_HDP_FLUSH_DONE__CP0_MASK 0x1
#define GPU_HDP_FLUSH_DONE__CP0__SHIFT 0x0
#define GPU_HDP_FLUSH_DONE__CP1_MASK 0x2
#define GPU_HDP_FLUSH_DONE__CP1__SHIFT 0x1
#define GPU_HDP_FLUSH_DONE__CP2_MASK 0x4
#define GPU_HDP_FLUSH_DONE__CP2__SHIFT 0x2
#define GPU_HDP_FLUSH_DONE__CP3_MASK 0x8
#define GPU_HDP_FLUSH_DONE__CP3__SHIFT 0x3
#define GPU_HDP_FLUSH_DONE__CP4_MASK 0x10
#define GPU_HDP_FLUSH_DONE__CP4__SHIFT 0x4
#define GPU_HDP_FLUSH_DONE__CP5_MASK 0x20
#define GPU_HDP_FLUSH_DONE__CP5__SHIFT 0x5
#define GPU_HDP_FLUSH_DONE__CP6_MASK 0x40
#define GPU_HDP_FLUSH_DONE__CP6__SHIFT 0x6
#define GPU_HDP_FLUSH_DONE__CP7_MASK 0x80
#define GPU_HDP_FLUSH_DONE__CP7__SHIFT 0x7
#define GPU_HDP_FLUSH_DONE__CP8_MASK 0x100
#define GPU_HDP_FLUSH_DONE__CP8__SHIFT 0x8
#define GPU_HDP_FLUSH_DONE__CP9_MASK 0x200
#define GPU_HDP_FLUSH_DONE__CP9__SHIFT 0x9
#define GPU_HDP_FLUSH_DONE__SDMA0_MASK 0x400
#define GPU_HDP_FLUSH_DONE__SDMA0__SHIFT 0xa
#define GPU_HDP_FLUSH_DONE__SDMA1_MASK 0x800
#define GPU_HDP_FLUSH_DONE__SDMA1__SHIFT 0xb
#define SLAVE_HANG_ERROR__SRBM_HANG_ERROR_MASK 0x1
#define SLAVE_HANG_ERROR__SRBM_HANG_ERROR__SHIFT 0x0
#define SLAVE_HANG_ERROR__HDP_HANG_ERROR_MASK 0x2
#define SLAVE_HANG_ERROR__HDP_HANG_ERROR__SHIFT 0x1
#define SLAVE_HANG_ERROR__VGA_HANG_ERROR_MASK 0x4
#define SLAVE_HANG_ERROR__VGA_HANG_ERROR__SHIFT 0x2
#define SLAVE_HANG_ERROR__ROM_HANG_ERROR_MASK 0x8
#define SLAVE_HANG_ERROR__ROM_HANG_ERROR__SHIFT 0x3
#define SLAVE_HANG_ERROR__AUDIO_HANG_ERROR_MASK 0x10
#define SLAVE_HANG_ERROR__AUDIO_HANG_ERROR__SHIFT 0x4
#define SLAVE_HANG_ERROR__CEC_HANG_ERROR_MASK 0x20
#define SLAVE_HANG_ERROR__CEC_HANG_ERROR__SHIFT 0x5
#define SLAVE_HANG_ERROR__XDMA_HANG_ERROR_MASK 0x80
#define SLAVE_HANG_ERROR__XDMA_HANG_ERROR__SHIFT 0x7
#define SLAVE_HANG_ERROR__DOORBELL_HANG_ERROR_MASK 0x100
#define SLAVE_HANG_ERROR__DOORBELL_HANG_ERROR__SHIFT 0x8
#define SLAVE_HANG_ERROR__GARLIC_HANG_ERROR_MASK 0x200
#define SLAVE_HANG_ERROR__GARLIC_HANG_ERROR__SHIFT 0x9
#define CAPTURE_HOST_BUSNUM__CHECK_EN_MASK 0x1
#define CAPTURE_HOST_BUSNUM__CHECK_EN__SHIFT 0x0
#define HOST_BUSNUM__HOST_ID_MASK 0xffff
#define HOST_BUSNUM__HOST_ID__SHIFT 0x0
#define PEER_REG_RANGE0__START_ADDR_MASK 0xffff
#define PEER_REG_RANGE0__START_ADDR__SHIFT 0x0
#define PEER_REG_RANGE0__END_ADDR_MASK 0xffff0000
#define PEER_REG_RANGE0__END_ADDR__SHIFT 0x10
#define PEER_REG_RANGE1__START_ADDR_MASK 0xffff
#define PEER_REG_RANGE1__START_ADDR__SHIFT 0x0
#define PEER_REG_RANGE1__END_ADDR_MASK 0xffff0000
#define PEER_REG_RANGE1__END_ADDR__SHIFT 0x10
#define PEER0_FB_OFFSET_HI__PEER0_FB_OFFSET_HI_MASK 0xfffff
#define PEER0_FB_OFFSET_HI__PEER0_FB_OFFSET_HI__SHIFT 0x0
#define PEER0_FB_OFFSET_LO__PEER0_FB_OFFSET_LO_MASK 0xfffff
#define PEER0_FB_OFFSET_LO__PEER0_FB_OFFSET_LO__SHIFT 0x0
#define PEER0_FB_OFFSET_LO__PEER0_FB_EN_MASK 0x80000000
#define PEER0_FB_OFFSET_LO__PEER0_FB_EN__SHIFT 0x1f
#define PEER1_FB_OFFSET_HI__PEER1_FB_OFFSET_HI_MASK 0xfffff
#define PEER1_FB_OFFSET_HI__PEER1_FB_OFFSET_HI__SHIFT 0x0
#define PEER1_FB_OFFSET_LO__PEER1_FB_OFFSET_LO_MASK 0xfffff
#define PEER1_FB_OFFSET_LO__PEER1_FB_OFFSET_LO__SHIFT 0x0
#define PEER1_FB_OFFSET_LO__PEER1_FB_EN_MASK 0x80000000
#define PEER1_FB_OFFSET_LO__PEER1_FB_EN__SHIFT 0x1f
#define PEER2_FB_OFFSET_HI__PEER2_FB_OFFSET_HI_MASK 0xfffff
#define PEER2_FB_OFFSET_HI__PEER2_FB_OFFSET_HI__SHIFT 0x0
#define PEER2_FB_OFFSET_LO__PEER2_FB_OFFSET_LO_MASK 0xfffff
#define PEER2_FB_OFFSET_LO__PEER2_FB_OFFSET_LO__SHIFT 0x0
#define PEER2_FB_OFFSET_LO__PEER2_FB_EN_MASK 0x80000000
#define PEER2_FB_OFFSET_LO__PEER2_FB_EN__SHIFT 0x1f
#define PEER3_FB_OFFSET_HI__PEER3_FB_OFFSET_HI_MASK 0xfffff
#define PEER3_FB_OFFSET_HI__PEER3_FB_OFFSET_HI__SHIFT 0x0
#define PEER3_FB_OFFSET_LO__PEER3_FB_OFFSET_LO_MASK 0xfffff
#define PEER3_FB_OFFSET_LO__PEER3_FB_OFFSET_LO__SHIFT 0x0
#define PEER3_FB_OFFSET_LO__PEER3_FB_EN_MASK 0x80000000
#define PEER3_FB_OFFSET_LO__PEER3_FB_EN__SHIFT 0x1f
#define DBG_BYPASS_SRBM_ACCESS__DBG_BYPASS_SRBM_ACCESS_EN_MASK 0x1
#define DBG_BYPASS_SRBM_ACCESS__DBG_BYPASS_SRBM_ACCESS_EN__SHIFT 0x0
#define DBG_BYPASS_SRBM_ACCESS__DBG_APER_AD_MASK 0x1e
#define DBG_BYPASS_SRBM_ACCESS__DBG_APER_AD__SHIFT 0x1
#define SMBUS_BACO_DUMMY__SMBUS_BACO_DUMMY_DATA_MASK 0xffffffff
#define SMBUS_BACO_DUMMY__SMBUS_BACO_DUMMY_DATA__SHIFT 0x0
#define BIF_DEVFUNCNUM_LIST0__DEVFUNC_ID0_MASK 0xff
#define BIF_DEVFUNCNUM_LIST0__DEVFUNC_ID0__SHIFT 0x0
#define BIF_DEVFUNCNUM_LIST0__DEVFUNC_ID1_MASK 0xff00
#define BIF_DEVFUNCNUM_LIST0__DEVFUNC_ID1__SHIFT 0x8
#define BIF_DEVFUNCNUM_LIST0__DEVFUNC_ID2_MASK 0xff0000
#define BIF_DEVFUNCNUM_LIST0__DEVFUNC_ID2__SHIFT 0x10
#define BIF_DEVFUNCNUM_LIST0__DEVFUNC_ID3_MASK 0xff000000
#define BIF_DEVFUNCNUM_LIST0__DEVFUNC_ID3__SHIFT 0x18
#define BIF_DEVFUNCNUM_LIST1__DEVFUNC_ID4_MASK 0xff
#define BIF_DEVFUNCNUM_LIST1__DEVFUNC_ID4__SHIFT 0x0
#define BIF_DEVFUNCNUM_LIST1__DEVFUNC_ID5_MASK 0xff00
#define BIF_DEVFUNCNUM_LIST1__DEVFUNC_ID5__SHIFT 0x8
#define BIF_DEVFUNCNUM_LIST1__DEVFUNC_ID6_MASK 0xff0000
#define BIF_DEVFUNCNUM_LIST1__DEVFUNC_ID6__SHIFT 0x10
#define BIF_DEVFUNCNUM_LIST1__DEVFUNC_ID7_MASK 0xff000000
#define BIF_DEVFUNCNUM_LIST1__DEVFUNC_ID7__SHIFT 0x18
#define BACO_CNTL__BACO_EN_MASK 0x1
#define BACO_CNTL__BACO_EN__SHIFT 0x0
#define BACO_CNTL__BACO_BCLK_OFF_MASK 0x2
#define BACO_CNTL__BACO_BCLK_OFF__SHIFT 0x1
#define BACO_CNTL__BACO_ISO_DIS_MASK 0x4
#define BACO_CNTL__BACO_ISO_DIS__SHIFT 0x2
#define BACO_CNTL__BACO_POWER_OFF_MASK 0x8
#define BACO_CNTL__BACO_POWER_OFF__SHIFT 0x3
#define BACO_CNTL__BACO_RESET_EN_MASK 0x10
#define BACO_CNTL__BACO_RESET_EN__SHIFT 0x4
#define BACO_CNTL__BACO_HANG_PROTECTION_EN_MASK 0x20
#define BACO_CNTL__BACO_HANG_PROTECTION_EN__SHIFT 0x5
#define BACO_CNTL__BACO_MODE_MASK 0x40
#define BACO_CNTL__BACO_MODE__SHIFT 0x6
#define BACO_CNTL__BACO_ANA_ISO_DIS_MASK 0x80
#define BACO_CNTL__BACO_ANA_ISO_DIS__SHIFT 0x7
#define BACO_CNTL__RCU_BIF_CONFIG_DONE_MASK 0x100
#define BACO_CNTL__RCU_BIF_CONFIG_DONE__SHIFT 0x8
#define BACO_CNTL__PWRGOOD_BF_MASK 0x200
#define BACO_CNTL__PWRGOOD_BF__SHIFT 0x9
#define BACO_CNTL__PWRGOOD_GPIO_MASK 0x400
#define BACO_CNTL__PWRGOOD_GPIO__SHIFT 0xa
#define BACO_CNTL__PWRGOOD_MEM_MASK 0x800
#define BACO_CNTL__PWRGOOD_MEM__SHIFT 0xb
#define BACO_CNTL__PWRGOOD_DVO_MASK 0x1000
#define BACO_CNTL__PWRGOOD_DVO__SHIFT 0xc
#define BACO_CNTL__PWRGOOD_IDSC_MASK 0x2000
#define BACO_CNTL__PWRGOOD_IDSC__SHIFT 0xd
#define BACO_CNTL__BACO_POWER_OFF_DRAM_MASK 0x10000
#define BACO_CNTL__BACO_POWER_OFF_DRAM__SHIFT 0x10
#define BACO_CNTL__BACO_BF_MEM_PHY_ISO_CNTRL_MASK 0x20000
#define BACO_CNTL__BACO_BF_MEM_PHY_ISO_CNTRL__SHIFT 0x11
#define BF_ANA_ISO_CNTL__BF_ANA_ISO_DIS_MASK_MASK 0x1
#define BF_ANA_ISO_CNTL__BF_ANA_ISO_DIS_MASK__SHIFT 0x0
#define BF_ANA_ISO_CNTL__BF_VDDC_ISO_DIS_MASK_MASK 0x2
#define BF_ANA_ISO_CNTL__BF_VDDC_ISO_DIS_MASK__SHIFT 0x1
#define MEM_TYPE_CNTL__BF_MEM_PHY_G5_G3_MASK 0x1
#define MEM_TYPE_CNTL__BF_MEM_PHY_G5_G3__SHIFT 0x0
#define BIF_BACO_DEBUG__BIF_BACO_SCANDUMP_FLG_MASK 0x1
#define BIF_BACO_DEBUG__BIF_BACO_SCANDUMP_FLG__SHIFT 0x0
#define BIF_BACO_DEBUG_LATCH__BIF_BACO_LATCH_FLG_MASK 0x1
#define BIF_BACO_DEBUG_LATCH__BIF_BACO_LATCH_FLG__SHIFT 0x0
#define BACO_CNTL_MISC__BIF_ROM_REQ_DIS_MASK 0x1
#define BACO_CNTL_MISC__BIF_ROM_REQ_DIS__SHIFT 0x0
#define BACO_CNTL_MISC__BIF_AZ_REQ_DIS_MASK 0x2
#define BACO_CNTL_MISC__BIF_AZ_REQ_DIS__SHIFT 0x1
#define BACO_CNTL_MISC__BACO_LINK_RST_WIDTH_SEL_MASK 0xc
#define BACO_CNTL_MISC__BACO_LINK_RST_WIDTH_SEL__SHIFT 0x2
#define BIF_SSA_PWR_STATUS__SSA_GFX_PWR_STATUS_MASK 0x1
#define BIF_SSA_PWR_STATUS__SSA_GFX_PWR_STATUS__SHIFT 0x0
#define BIF_SSA_PWR_STATUS__SSA_DISP_PWR_STATUS_MASK 0x2
#define BIF_SSA_PWR_STATUS__SSA_DISP_PWR_STATUS__SHIFT 0x1
#define BIF_SSA_PWR_STATUS__SSA_MC_PWR_STATUS_MASK 0x4
#define BIF_SSA_PWR_STATUS__SSA_MC_PWR_STATUS__SHIFT 0x2
#define BIF_SSA_GFX0_LOWER__SSA_GFX0_LOWER_MASK 0x3fffc
#define BIF_SSA_GFX0_LOWER__SSA_GFX0_LOWER__SHIFT 0x2
#define BIF_SSA_GFX0_LOWER__SSA_GFX0_REG_CMP_EN_MASK 0x40000000
#define BIF_SSA_GFX0_LOWER__SSA_GFX0_REG_CMP_EN__SHIFT 0x1e
#define BIF_SSA_GFX0_LOWER__SSA_GFX0_REG_STALL_EN_MASK 0x80000000
#define BIF_SSA_GFX0_LOWER__SSA_GFX0_REG_STALL_EN__SHIFT 0x1f
#define BIF_SSA_GFX0_UPPER__SSA_GFX0_UPPER_MASK 0x3fffc
#define BIF_SSA_GFX0_UPPER__SSA_GFX0_UPPER__SHIFT 0x2
#define BIF_SSA_GFX1_LOWER__SSA_GFX1_LOWER_MASK 0x3fffc
#define BIF_SSA_GFX1_LOWER__SSA_GFX1_LOWER__SHIFT 0x2
#define BIF_SSA_GFX1_LOWER__SSA_GFX1_REG_CMP_EN_MASK 0x40000000
#define BIF_SSA_GFX1_LOWER__SSA_GFX1_REG_CMP_EN__SHIFT 0x1e
#define BIF_SSA_GFX1_LOWER__SSA_GFX1_REG_STALL_EN_MASK 0x80000000
#define BIF_SSA_GFX1_LOWER__SSA_GFX1_REG_STALL_EN__SHIFT 0x1f
#define BIF_SSA_GFX1_UPPER__SSA_GFX1_UPPER_MASK 0x3fffc
#define BIF_SSA_GFX1_UPPER__SSA_GFX1_UPPER__SHIFT 0x2
#define BIF_SSA_GFX2_LOWER__SSA_GFX2_LOWER_MASK 0x3fffc
#define BIF_SSA_GFX2_LOWER__SSA_GFX2_LOWER__SHIFT 0x2
#define BIF_SSA_GFX2_LOWER__SSA_GFX2_REG_CMP_EN_MASK 0x40000000
#define BIF_SSA_GFX2_LOWER__SSA_GFX2_REG_CMP_EN__SHIFT 0x1e
#define BIF_SSA_GFX2_LOWER__SSA_GFX2_REG_STALL_EN_MASK 0x80000000
#define BIF_SSA_GFX2_LOWER__SSA_GFX2_REG_STALL_EN__SHIFT 0x1f
#define BIF_SSA_GFX2_UPPER__SSA_GFX2_UPPER_MASK 0x3fffc
#define BIF_SSA_GFX2_UPPER__SSA_GFX2_UPPER__SHIFT 0x2
#define BIF_SSA_GFX3_LOWER__SSA_GFX3_LOWER_MASK 0x3fffc
#define BIF_SSA_GFX3_LOWER__SSA_GFX3_LOWER__SHIFT 0x2
#define BIF_SSA_GFX3_LOWER__SSA_GFX3_REG_CMP_EN_MASK 0x40000000
#define BIF_SSA_GFX3_LOWER__SSA_GFX3_REG_CMP_EN__SHIFT 0x1e
#define BIF_SSA_GFX3_LOWER__SSA_GFX3_REG_STALL_EN_MASK 0x80000000
#define BIF_SSA_GFX3_LOWER__SSA_GFX3_REG_STALL_EN__SHIFT 0x1f
#define BIF_SSA_GFX3_UPPER__SSA_GFX3_UPPER_MASK 0x3fffc
#define BIF_SSA_GFX3_UPPER__SSA_GFX3_UPPER__SHIFT 0x2
#define BIF_SSA_DISP_LOWER__SSA_DISP_LOWER_MASK 0x3fffc
#define BIF_SSA_DISP_LOWER__SSA_DISP_LOWER__SHIFT 0x2
#define BIF_SSA_DISP_LOWER__SSA_DISP_REG_CMP_EN_MASK 0x40000000
#define BIF_SSA_DISP_LOWER__SSA_DISP_REG_CMP_EN__SHIFT 0x1e
#define BIF_SSA_DISP_LOWER__SSA_DISP_REG_STALL_EN_MASK 0x80000000
#define BIF_SSA_DISP_LOWER__SSA_DISP_REG_STALL_EN__SHIFT 0x1f
#define BIF_SSA_DISP_UPPER__SSA_DISP_UPPER_MASK 0x3fffc
#define BIF_SSA_DISP_UPPER__SSA_DISP_UPPER__SHIFT 0x2
#define BIF_SSA_MC_LOWER__SSA_MC_LOWER_MASK 0x3fffc
#define BIF_SSA_MC_LOWER__SSA_MC_LOWER__SHIFT 0x2
#define BIF_SSA_MC_LOWER__SSA_MC_FB_STALL_EN_MASK 0x20000000
#define BIF_SSA_MC_LOWER__SSA_MC_FB_STALL_EN__SHIFT 0x1d
#define BIF_SSA_MC_LOWER__SSA_MC_REG_CMP_EN_MASK 0x40000000
#define BIF_SSA_MC_LOWER__SSA_MC_REG_CMP_EN__SHIFT 0x1e
#define BIF_SSA_MC_LOWER__SSA_MC_REG_STALL_EN_MASK 0x80000000
#define BIF_SSA_MC_LOWER__SSA_MC_REG_STALL_EN__SHIFT 0x1f
#define BIF_SSA_MC_UPPER__SSA_MC_UPPER_MASK 0x3fffc
#define BIF_SSA_MC_UPPER__SSA_MC_UPPER__SHIFT 0x2
#define IMPCTL_RESET__IMP_SW_RESET_MASK 0x1
#define IMPCTL_RESET__IMP_SW_RESET__SHIFT 0x0
#define GARLIC_FLUSH_CNTL__CP_RB0_WPTR_MASK 0x1
#define GARLIC_FLUSH_CNTL__CP_RB0_WPTR__SHIFT 0x0
#define GARLIC_FLUSH_CNTL__CP_RB1_WPTR_MASK 0x2
#define GARLIC_FLUSH_CNTL__CP_RB1_WPTR__SHIFT 0x1
#define GARLIC_FLUSH_CNTL__CP_RB2_WPTR_MASK 0x4
#define GARLIC_FLUSH_CNTL__CP_RB2_WPTR__SHIFT 0x2
#define GARLIC_FLUSH_CNTL__UVD_RBC_RB_WPTR_MASK 0x8
#define GARLIC_FLUSH_CNTL__UVD_RBC_RB_WPTR__SHIFT 0x3
#define GARLIC_FLUSH_CNTL__SDMA0_GFX_RB_WPTR_MASK 0x10
#define GARLIC_FLUSH_CNTL__SDMA0_GFX_RB_WPTR__SHIFT 0x4
#define GARLIC_FLUSH_CNTL__SDMA1_GFX_RB_WPTR_MASK 0x20
#define GARLIC_FLUSH_CNTL__SDMA1_GFX_RB_WPTR__SHIFT 0x5
#define GARLIC_FLUSH_CNTL__CP_DMA_ME_COMMAND_MASK 0x40
#define GARLIC_FLUSH_CNTL__CP_DMA_ME_COMMAND__SHIFT 0x6
#define GARLIC_FLUSH_CNTL__CP_DMA_PFP_COMMAND_MASK 0x80
#define GARLIC_FLUSH_CNTL__CP_DMA_PFP_COMMAND__SHIFT 0x7
#define GARLIC_FLUSH_CNTL__SAM_SA