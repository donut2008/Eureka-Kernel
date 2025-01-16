POINT2_MASK 0x400
#define CC_RCU_FUSES__RCU_BREAK_POINT2__SHIFT 0xa
#define CC_RCU_FUSES__SMU_IOC_MST_DISABLE_MASK 0x4000
#define CC_RCU_FUSES__SMU_IOC_MST_DISABLE__SHIFT 0xe
#define CC_RCU_FUSES__FCH_LOCKOUT_ENABLE_MASK 0x8000
#define CC_RCU_FUSES__FCH_LOCKOUT_ENABLE__SHIFT 0xf
#define CC_RCU_FUSES__FCH_XFIRE_FILTER_ENABLE_MASK 0x10000
#define CC_RCU_FUSES__FCH_XFIRE_FILTER_ENABLE__SHIFT 0x10
#define CC_RCU_FUSES__XFIRE_DISABLE_MASK 0x20000
#define CC_RCU_FUSES__XFIRE_DISABLE__SHIFT 0x11
#define CC_RCU_FUSES__SAMU_FUSE_DISABLE_MASK 0x40000
#define CC_RCU_FUSES__SAMU_FUSE_DISABLE__SHIFT 0x12
#define CC_RCU_FUSES__BIF_RST_POLLING_DISABLE_MASK 0x80000
#define CC_RCU_FUSES__BIF_RST_POLLING_DISABLE__SHIFT 0x13
#define CC_RCU_FUSES__MEM_HARDREP_EN_MASK 0x200000
#define CC_RCU_FUSES__MEM_HARDREP_EN__SHIFT 0x15
#define CC_RCU_FUSES__PCIE_INIT_DISABLE_MASK 0x400000
#define CC_RCU_FUSES__PCIE_INIT_DISABLE__SHIFT 0x16
#define CC_RCU_FUSES__DSMU_DISABLE_MASK 0x800000
#define CC_RCU_FUSES__DSMU_DISABLE__SHIFT 0x17
#define CC_RCU_FUSES__WRP_FUSE_VALID_MASK 0x1000000
#define CC_RCU_FUSES__WRP_FUSE_VALID__SHIFT 0x18
#define CC_RCU_FUSES__PHY_FUSE_VALID_MASK 0x2000000
#define CC_RCU_FUSES__PHY_FUSE_VALID__SHIFT 0x19
#define CC_RCU_FUSES__RCU_SPARE_MASK 0xfc000000
#define CC_RCU_FUSES__RCU_SPARE__SHIFT 0x1a
#define CC_SMU_MISC_FUSES__IOMMU_V2_DISABLE_MASK 0x2
#define CC_SMU_MISC_FUSES__IOMMU_V2_DISABLE__SHIFT 0x1
#define CC_SMU_MISC_FUSES__MinSClkDid_MASK 0x1fc
#define CC_SMU_MISC_FUSES__MinSClkDid__SHIFT 0x2
#define CC_SMU_MISC_FUSES__MISC_SPARE_MASK 0x600
#define CC_SMU_MISC_FUSES__MISC_SPARE__SHIFT 0x9
#define CC_SMU_MISC_FUSES__PostResetGnbClkDid_MASK 0x3f800
#define CC_SMU_MISC_FUSES__PostResetGnbClkDid__SHIFT 0xb
#define CC_SMU_MISC_FUSES__L2IMU_tn2_dtc_half_MASK 0x40000
#define CC_SMU_MISC_FUSES__L2IMU_tn2_dtc_half__SHIFT 0x12
#define CC_SMU_MISC_FUSES__L2IMU_tn2_ptc_half_MASK 0x80000
#define CC_SMU_MISC_FUSES__L2IMU_tn2_ptc_half__SHIFT 0x13
#define CC_SMU_MISC_FUSES__L2IMU_tn2_itc_half_MASK 0x100000
#define CC_SMU_MISC_FUSES__L2IMU_tn2_itc_half__SHIFT 0x14
#define CC_SMU_MISC_FUSES__L2IMU_tn2_pdc_half_MASK 0x200000
#define CC_SMU_MISC_FUSES__L2IMU_tn2_pdc_half__SHIFT 0x15
#define CC_SMU_MISC_FUSES__L2IMU_tn2_ptc_dis_MASK 0x400000
#define CC_SMU_MISC_FUSES__L2IMU_tn2_ptc_dis__SHIFT 0x16
#define CC_SMU_MISC_FUSES__L2IMU_tn2_itc_dis_MASK 0x800000
#define CC_SMU_MISC_FUSES__L2IMU_tn2_itc_dis__SHIFT 0x17
#define CC_SMU_MISC_FUSES__VCE_DISABLE_MASK 0x8000000
#define CC_SMU_MISC_FUSES__VCE_DISABLE__SHIFT 0x1b
#define CC_SMU_MISC_FUSES__IOC_IOMMU_DISABLE_MASK 0x10000000
#define CC_SMU_MISC_FUSES__IOC_IOMMU_DISABLE__SHIFT 0x1c
#define CC_SMU_MISC_FUSES__GNB_SPARE_MASK 0x60000000
#define CC_SMU_MISC_FUSES__GNB_SPARE__SHIFT 0x1d
#define CC_SCLK_VID_FUSES__SClkVid0_MASK 0xff
#define CC_SCLK_VID_FUSES__SClkVid0__SHIFT 0x0
#define CC_SCLK_VID_FUSES__SClkVid1_MASK 0xff00
#define CC_SCLK_VID_FUSES__SClkVid1__SHIFT 0x8
#define CC_SCLK_VID_FUSES__SClkVid2_MASK 0xff0000
#define CC_SCLK_VID_FUSES__SClkVid2__SHIFT 0x10
#define CC_SCLK_VID_FUSES__SClkVid3_MASK 0xff000000
#define CC_SCLK_VID_FUSES__SClkVid3__SHIFT 0x18
#define CC_GIO_IOCCFG_FUSES__NB_REV_ID_MASK 0x7fe
#define CC_GIO_IOCCFG_FUSES__NB_REV_ID__SHIFT 0x1
#define CC_GIO_IOC_FUSES__IOC_FUSES_MASK 0x3e
#define CC_GIO_IOC_FUSES__IOC_FUSES__SHIFT 0x1
#define CC_SMU_TST_EFUSE1_MISC__RF_RM_6_2_MASK 0x3e
#define CC_SMU_TST_EFUSE1_MISC__RF_RM_6_2__SHIFT 0x1
#define CC_SMU_TST_EFUSE1_MISC__RME_MASK 0x40
#define CC_SMU_TST_EFUSE1_MISC__RME__SHIFT 0x6
#define CC_SMU_TST_EFUSE1_MISC__MBIST_DISABLE_MASK 0x80
#define CC_SMU_TST_EFUSE1_MISC__MBIST_DISABLE__SHIFT 0x7
#define CC_SMU_TST_EFUSE1_MISC__HARD_REPAIR_DISABLE_MASK 0x100
#define CC_SMU_TST_EFUSE1_MISC__HARD_REPAIR_DISABLE__SHIFT 0x8
#define CC_SMU_TST_EFUSE1_MISC__SOFT_REPAIR_DISABLE_MASK 0x200
#define CC_SMU_TST_EFUSE1_MISC__SOFT_REPAIR_DISABLE__SHIFT 0x9
#define CC_SMU_TST_EFUSE1_MISC__GPU_DIS_MASK 0x400
#define CC_SMU_TST_EFUSE1_MISC__GPU_DIS__SHIFT 0xa
#define CC_SMU_TST_EFUSE1_MISC__SMS_PWRDWN_DISABLE_MASK 0x800
#define CC_SMU_TST_EFUSE1_MISC__SMS_PWRDWN_DISABLE__SHIFT 0xb
#define CC_SMU_TST_EFUSE1_MISC__CRBBMP1500_DISA_MASK 0x1000
#define CC_SMU_TST_EFUSE1_MISC__CRBBMP1500_DISA__SHIFT 0xc
#define CC_SMU_TST_EFUSE1_MISC__CRBBMP1500_DISB_MASK 0x2000
#define CC_SMU_TST_EFUSE1_MISC__CRBBMP1500_DISB__SHIFT 0xd
#define CC_SMU_TST_EFUSE1_MISC__RM_RF8_MASK 0x4000
#define CC_SMU_TST_EFUSE1_MISC__RM_RF8__SHIFT 0xe
#define CC_SMU_TST_EFUSE1_MISC__DFT_SPARE1_MASK 0x400000
#define CC_SMU_TST_EFUSE1_MISC__DFT_SPARE1__SHIFT 0x16
#define CC_SMU_TST_EFUSE1_MISC__DFT_SPARE2_MASK 0x800000
#define CC_SMU_TST_EFUSE1_MISC__DFT_SPARE2__SHIFT 0x17
#define CC_SMU_TST_EFUSE1_MISC__DFT_SPARE3_MASK 0x1000000
#define CC_SMU_TST_EFUSE1_MISC__DFT_SPARE3__SHIFT 0x18
#define CC_SMU_TST_EFUSE1_MISC__VCE_DISABLE_MASK 0x2000000
#define CC_SMU_TST_EFUSE1_MISC__VCE_DISABLE__SHIFT 0x19
#define CC_SMU_TST_EFUSE1_MISC__DCE_SCAN_DISABLE_MASK 0x4000000
#define CC_SMU_TST_EFUSE1_MISC__DCE_SCAN_DISABLE__SHIFT 0x1a
#define CC_TST_ID_STRAPS__DEVICE_ID_MASK 0xffff0
#define CC_TST_ID_STRAPS__DEVICE_ID__SHIFT 0x4
#define CC_TST_ID_STRAPS__MAJOR_REV_ID_MASK 0xf00000
#define CC_TST_ID_STRAPS__MAJOR_REV_ID__SHIFT 0x14
#define CC_TST_ID_STRAPS__MINOR_REV_ID_MASK 0xf000000
#define CC_TST_ID_STRAPS__MINOR_REV_ID__SHIFT 0x18
#define CC_TST_ID_STRAPS__ATI_REV_ID_MASK 0xf0000000
#define CC_TST_ID_STRAPS__ATI_REV_ID__SHIFT 0x1c
#define CC_FCTRL_FUSES__EXT_EFUSE_MACRO_PRESENT_MASK 0x2
#define CC_FCTRL_FUSES__EXT_EFUSE_MACRO_PRESENT__SHIFT 0x1
#define CC_HARVEST_FUSES__VCE_DISABLE_MASK 0x6
#define CC_HARVEST_FUSES__VCE_DISABLE__SHIFT 0x1
#define CC_HARVEST_FUSES__UVD_DISABLE_MASK 0x10
#define CC_HARVEST_FUSES__UVD_DISABLE__SHIFT 0x4
#define CC_HARVEST_FUSES__ACP_DISABLE_MASK 0x40
#define CC_HARVEST_FUSES__ACP_DISABLE__SHIFT 0x6
#define CC_HARVEST_FUSES__DC_DISABLE_MASK 0x3f00
#define CC_HARVEST_FUSES__DC_DISABLE__SHIFT 0x8
#define SMU_MAIN_PLL_OP_FREQ__PLL_OP_FREQ_MASK 0xffffffff
#define SMU_MAIN_PLL_OP_FREQ__PLL_OP_FREQ__SHIFT 0x0
#define SMU_STATUS__SMU_DONE_MASK 0x1
#define SMU_STATUS__SMU_DONE__SHIFT 0x0
#define SMU_STATUS__SMU_PASS_MASK 0x2
#define SMU_STATUS__SMU_PASS__SHIFT 0x1
#define SMU_FIRMWARE__SMU_IN_PROG_MASK 0x1
#define SMU_FIRMWARE__SMU_IN_PROG__SHIFT 0x0
#define SMU_FIRMWARE__SMU_RD_DONE_MASK 0x6
#define SMU_FIRMWARE__SMU_RD_DONE__SHIFT 0x1
#define SMU_FIRMWARE__SMU_SRAM_RD_BLOCK_EN_MASK 0x8
#define SMU_FIRMWARE__SMU_SRAM_RD_BLOCK_EN__SHIFT 0x3
#define SMU_FIRMWARE__SMU_SRAM_WR_BLOCK_EN_MASK 0x10
#define SMU_FIRMWARE__SMU_SRAM_WR_BLOCK_EN__SHIFT 0x4
#define SMU_FIRMWARE__SMU_counter_MASK 0xf00
#define SMU_FIRMWARE__SMU_counter__SHIFT 0x8
#define SMU_FIRMWARE__SMU_MODE_MASK 0x10000
#define SMU_FIRMWARE__SMU_MODE__SHIFT 0x10
#define SMU_FIRMWARE__SMU_SEL_MASK 0x20000
#define SMU_FIRMWARE__SMU_SEL__SHIFT 0x11
#define SMU_INPUT_DATA__START_ADDR_MASK 0x7fffffff
#define SMU_INPUT_DATA__START_ADDR__SHIFT 0x0
#define SMU_INPUT_DATA__AUTO_START_MASK 0x80000000
#define SMU_INPUT_DATA__AUTO_START__SHIFT 0x1f
#define SMU_EFUSE_0__EFUSE_DATA_MASK 0xffffffff
#define SMU_EFUSE_0__EFUSE_DATA__SHIFT 0x0
#define FIRMWARE_FLAGS__INTERRUPTS_ENABLED_MASK 0x1
#define FIRMWARE_FLAGS__INTERRUPTS_ENABLED__SHIFT 0x0
#define FIRMWARE_FLAGS__RESERVED_MASK 0xfffffe
#define FIRMWARE_FLAGS__RESERVED__SHIFT 0x1
#define FIRMWARE_FLAGS__TEST_COUNT_MASK 0xff000000
#define FIRMWARE_FLAGS__TEST_COUNT__SHIFT 0x18
#define TDC_STATUS__VDD_Boost_MASK 0xff
#define TDC_STATUS__VDD_Boost__SHIFT 0x0
#define TDC_STATUS__VDD_Throttle_MASK 0xff00
#define TDC_STATUS__VDD_Throttle__SHIFT 0x8
#define TDC_STATUS__VDDC_Boost_MASK 0xff0000
#define TDC_STATUS__VDDC_Boost__SHIFT 0x10
#define TDC_STATUS__VDDC_Throttle_MASK 0xff000000
#define TDC_STATUS__VDDC_Throttle__SHIFT 0x18
#define TDC_MV_AVERAGE__IDD_MASK 0xffff
#define TDC_MV_AVERAGE__IDD__SHIFT 0x0
#define TDC_MV_AVERAGE__IDDC_MASK 0xffff0000
#define TDC_MV_AVERAGE__IDDC__SHIFT 0x10
#define TDC_VRM_LIMIT__IDD_MASK 0xffff
#define TDC_VRM_LIMIT__IDD__SHIFT 0x0
#define TDC_VRM_LIMIT__IDDC_MASK 0xffff0000
#define TDC_VRM_LIMIT__IDDC__SHIFT 0x10
#define FEATURE_STATUS__SCLK_DPM_ON_MASK 0x1
#define FEATURE_STATUS__SCLK_DPM_ON__SHIFT 0x0
#define FEATURE_STATUS__MCLK_DPM_ON_MASK 0x2
#define FEATURE_STATUS__MCLK_DPM_ON__SHIFT 0x1
#define FEATURE_STATUS__LCLK_DPM_ON_MASK 0x4
#define FEATURE_STATUS__LCLK_DPM_ON__SHIFT 0x2
#define FEATURE_STATUS__UVD_DPM_ON_MASK 0x8
#define FEATURE_STATUS__UVD_DPM_ON__SHIFT 0x3
#define FEATURE_STATUS__VCE_DPM_ON_MASK 0x10
#define FEATURE_STATUS__VCE_DPM_ON__SHIFT 0x4
#define FEATURE_STATUS__SAMU_DPM_ON_MASK 0x20
#define FEATURE_STATUS__SAMU_DPM_ON__SHIFT 0x5
#define FEATURE_STATUS__ACP_DPM_ON_MASK 0x40
#define FEATURE_STATUS__ACP_DPM_ON__SHIFT 0x6
#define FEATURE_STATUS__PCIE_DPM_ON_MASK 0x80
#define FEATURE_STATUS__PCIE_DPM_ON__SHIFT 0x7
#define FEATURE_STATUS__BAPM_ON_MASK 0x100
#define FEATURE_STATUS__BAPM_ON__SHIFT 0x8
#define FEATURE_STATUS__LPMX_ON_MASK 0x200
#define FEATURE_STATUS__LPMX_ON__SHIFT 0x9
#define FEATURE_STATUS__NBDPM_ON_MASK 0x400
#define FEATURE_STATUS__NBDPM_ON__SHIFT 0xa
#define FEATURE_STATUS__LHTC_ON_MASK 0x800
#define FEATURE_STATUS__LHTC_ON__SHIFT 0xb
#define FEATURE_STATUS__VPC_ON_MASK 0x1000
#define FEATURE_STATUS__VPC_ON__SHIFT 0xc
#define FEATURE_STATUS__VOLTAGE_CONTROLLER_ON_MASK 0x2000
#define FEATURE_STATUS__VOLTAGE_CONTROLLER_ON__SHIFT 0xd
#define FEATURE_STATUS__TDC_LIMIT_ON_MASK 0x4000
#define FEATURE_STATUS__TDC_LIMIT_ON__SHIFT 0xe
#define FEATURE_STATUS__GPU_CAC_ON_MASK 0x8000
#define FEATURE_STATUS__GPU_CAC_ON__SHIFT 0xf
#define FEATURE_STATUS__AVS_ON_MASK 0x10000
#define FEATURE_STATUS__AVS_ON__SHIFT 0x10
#define FEATURE_STATUS__SPMI_ON_MASK 0x20000
#define FEATURE_STATUS__SPMI_ON__SHIFT 0x11
#define FEATURE_STATUS__SCLK_DPM_FORCED_MASK 0x40000
#define FEATURE_STATUS__SCLK_DPM_FORCED__SHIFT 0x12
#define FEATURE_STATUS__MCLK_DPM_FORCED_MASK 0x80000
#define FEATURE_STATUS__MCLK_DPM_FORCED__SHIFT 0x13
#define FEATURE_STATUS__LCLK_DPM_FORCED_MASK 0x100000
#define FEATURE_STATUS__LCLK_DPM_FORCED__SHIFT 0x14
#define FEATURE_STATUS__PCIE_DPM_FORCED_MASK 0x200000
#define FEATURE_STATUS__PCIE_DPM_FORCED__SHIFT 0x15
#define FEATURE_STATUS__RESERVED_MASK 0xffc00000
#define FEATURE_STATUS__RESERVED__SHIFT 0x16
#define ENTITY_TEMPERATURES_1__GPU_MASK 0xffffffff
#define ENTITY_TEMPERATURES_1__GPU__SHIFT 0x0
#define MCARB_DRAM_TIMING_TABLE_1__entries_0_0_McArbDramTiming_MASK 0xffffffff
#define MCARB_DRAM_TIMING_TABLE_1__entries_0_0_McArbDramTiming__SHIFT 0x0
#define MCARB_DRAM_TIMING_TABLE_2__entries_0_0_McArbDramTiming2_MASK 0xffffffff
#define MCARB_DRAM_TIMING_TABLE_2__entries_0_0_McArbDramTiming2__SHIFT 0x0
#define MCARB_DRAM_TIMING_TABLE_3__entries_0_0_padding_2_MASK 0xff
#define MCARB_DRAM_TIMING_TABLE_3__entries_0_0_padding_2__SHIFT 0x0
#define MCARB_DRAM_TIMING_TABLE_3__entries_0_0_padding_1_MASK 0xff00
#define MCARB_DRAM_TIMING_TABLE_3__entries_0_0_padding_1__SHIFT 0x8
#define MCARB_DRAM_TIMING_TABLE_3__entries_0_0_padding_0_MASK 0xff0000
#define MCARB_DRAM_TIMING_TABLE_3__entries_0_0_padding_0__SHIFT 0x10
#define MCARB_DRAM_TIMING_TABLE_3__entries_0_0_McArbBurstTime