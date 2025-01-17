DIV_MASK 0xff
#define PLL_SS_CNTL__PLL_SS_AMOUNT_FBDIV__SHIFT 0x0
#define PLL_SS_CNTL__PLL_SS_AMOUNT_NFRAC_SLIP_MASK 0xf00
#define PLL_SS_CNTL__PLL_SS_AMOUNT_NFRAC_SLIP__SHIFT 0x8
#define PLL_SS_CNTL__PLL_SS_EN_MASK 0x1000
#define PLL_SS_CNTL__PLL_SS_EN__SHIFT 0xc
#define PLL_SS_CNTL__PLL_SS_MODE_MASK 0x2000
#define PLL_SS_CNTL__PLL_SS_MODE__SHIFT 0xd
#define PLL_SS_CNTL__PLL_SS_STEP_SIZE_DSFRAC_MASK 0xffff0000
#define PLL_SS_CNTL__PLL_SS_STEP_SIZE_DSFRAC__SHIFT 0x10
#define PLL_DS_CNTL__PLL_DS_FRAC_MASK 0xffff
#define PLL_DS_CNTL__PLL_DS_FRAC__SHIFT 0x0
#define PLL_DS_CNTL__PLL_DS_ORDER_MASK 0x30000
#define PLL_DS_CNTL__PLL_DS_ORDER__SHIFT 0x10
#define PLL_DS_CNTL__PLL_DS_MODE_MASK 0x40000
#define PLL_DS_CNTL__PLL_DS_MODE__SHIFT 0x12
#define PLL_DS_CNTL__PLL_DS_PRBS_EN_MASK 0x80000
#define PLL_DS_CNTL__PLL_DS_PRBS_EN__SHIFT 0x13
#define PLL_IDCLK_CNTL__PLL_LTDP_IDCLK_EN_MASK 0x1
#define PLL_IDCLK_CNTL__PLL_LTDP_IDCLK_EN__SHIFT 0x0
#define PLL_IDCLK_CNTL__PLL_LTDP_IDCLK_DIFF_EN_MASK 0x2
#define PLL_IDCLK_CNTL__PLL_LTDP_IDCLK_DIFF_EN__SHIFT 0x1
#define PLL_IDCLK_CNTL__PLL_TMDP_IDCLK_EN_MASK 0x4
#define PLL_IDCLK_CNTL__PLL_TMDP_IDCLK_EN__SHIFT 0x2
#define PLL_IDCLK_CNTL__PLL_TMDP_IDCLK_DIFF_EN_MASK 0x8
#define PLL_IDCLK_CNTL__PLL_TMDP_IDCLK_DIFF_EN__SHIFT 0x3
#define PLL_IDCLK_CNTL__PLL_IDCLK_EN_MASK 0x10
#define PLL_IDCLK_CNTL__PLL_IDCLK_EN__SHIFT 0x4
#define PLL_IDCLK_CNTL__PLL_DIFF_POST_DIV_RESET_MASK 0x100
#define PLL_IDCLK_CNTL__PLL_DIFF_POST_DIV_RESET__SHIFT 0x8
#define PLL_IDCLK_CNTL__PLL_DIFF_POST_DIV_SELECT_MASK 0x1000
#define PLL_IDCLK_CNTL__PLL_DIFF_POST_DIV_SELECT__SHIFT 0xc
#define PLL_IDCLK_CNTL__PLL_DIFF_POST_DIV_MASK 0xf0000
#define PLL_IDCLK_CNTL__PLL_DIFF_POST_DIV__SHIFT 0x10
#define PLL_IDCLK_CNTL__PLL_CUR_LTDP_MASK 0x300000
#define PLL_IDCLK_CNTL__PLL_CUR_LTDP__SHIFT 0x14
#define PLL_IDCLK_CNTL__PLL_CUR_PREDRV_MASK 0xc00000
#define PLL_IDCLK_CNTL__PLL_CUR_PREDRV__SHIFT 0x16
#define PLL_IDCLK_CNTL__PLL_CUR_TMDP_MASK 0x3000000
#define PLL_IDCLK_CNTL__PLL_CUR_TMDP__SHIFT 0x18
#define PLL_IDCLK_CNTL__PLL_CML_A_DRVSTR_MASK 0xc000000
#define PLL_IDCLK_CNTL__PLL_CML_A_DRVSTR__SHIFT 0x1a
#define PLL_IDCLK_CNTL__PLL_CML_B_DRVSTR_MASK 0x30000000
#define PLL_IDCLK_CNTL__PLL_CML_B_DRVSTR__SHIFT 0x1c
#define PLL_CNTL__PLL_RESET_MASK 0x1
#define PLL_CNTL__PLL_RESET__SHIFT 0x0
#define PLL_CNTL__PLL_POWER_DOWN_MASK 0x2
#define PLL_CNTL__PLL_POWER_DOWN__SHIFT 0x1
#define PLL_CNTL__PLL_BYPASS_CAL_MASK 0x4
#define PLL_CNTL__PLL_BYPASS_CAL__SHIFT 0x2
#define PLL_CNTL__PLL_POST_DIV_SRC_MASK 0x8
#define PLL_CNTL__PLL_POST_DIV_SRC__SHIFT 0x3
#define PLL_CNTL__PLL_VCOREF_MASK 0x30
#define PLL_CNTL__PLL_VCOREF__SHIFT 0x4
#define PLL_CNTL__PLL_PCIE_REFCLK_SEL_MASK 0x40
#define PLL_CNTL__PLL_PCIE_REFCLK_SEL__SHIFT 0x6
#define PLL_CNTL__PLL_ANTIGLITCH_RESETB_MASK 0x80
#define PLL_CNTL__PLL_ANTIGLITCH_RESETB__SHIFT 0x7
#define PLL_CNTL__PLL_CALREF_MASK 0x300
#define PLL_CNTL__PLL_CALREF__SHIFT 0x8
#define PLL_CNTL__PLL_CAL_BYPASS_REFDIV_MASK 0x400
#define PLL_CNTL__PLL_CAL_BYPASS_REFDIV__SHIFT 0xa
#define PLL_CNTL__PLL_REFCLK_SEL_MASK 0x1800
#define PLL_CNTL__PLL_REFCLK_SEL__SHIFT 0xb
#define PLL_CNTL__PLL_ANTI_GLITCH_RESET_MASK 0x2000
#define PLL_CNTL__PLL_ANTI_GLITCH_RESET__SHIFT 0xd
#define PLL_CNTL__PLL_XOCLK_DRV_R_EN_MASK 0x4000
#define PLL_CNTL__PLL_XOCLK_DRV_R_EN__SHIFT 0xe
#define PLL_CNTL__PLL_REF_DIV_SRC_MASK 0x70000
#define PLL_CNTL__PLL_REF_DIV_SRC__SHIFT 0x10
#define PLL_CNTL__PLL_LOCK_FREQ_SEL_MASK 0x80000
#define PLL_CNTL__PLL_LOCK_FREQ_SEL__SHIFT 0x13
#define PLL_CNTL__PLL_CALIB_DONE_MASK 0x100000
#define PLL_CNTL__PLL_CALIB_DONE__SHIFT 0x14
#define PLL_CNTL__PLL_LOCKED_MASK 0x200000
#define PLL_CNTL__PLL_LOCKED__SHIFT 0x15
#define PLL_CNTL__PLL_REFCLK_RECV_EN_MASK 0x400000
#define PLL_CNTL__PLL_REFCLK_RECV_EN__SHIFT 0x16
#define PLL_CNTL__PLL_REFCLK_RECV_SEL_MASK 0x800000
#define PLL_CNTL__PLL_REFCLK_RECV_SEL__SHIFT 0x17
#define PLL_CNTL__PLL_TIMING_MODE_STATUS_MASK 0x3000000
#define PLL_CNTL__PLL_TIMING_MODE_STATUS__SHIFT 0x18
#define PLL_CNTL__PLL_DIG_SPARE_MASK 0xfc000000
#define PLL_CNTL__PLL_DIG_SPARE__SHIFT 0x1a
#define PLL_ANALOG__PLL_CAL_MODE_MASK 0x1f
#define PLL_ANALOG__PLL_CAL_MODE__SHIFT 0x0
#define PLL_ANALOG__PLL_PFD_PULSE_SEL_MASK 0x60
#define PLL_ANALOG__PLL_PFD_PULSE_SEL__SHIFT 0x5
#define PLL_ANALOG__PLL_CP_MASK 0xf00
#define PLL_ANALOG__PLL_CP__SHIFT 0x8
#define PLL_ANALOG__PLL_LF_MODE_MASK 0x1ff000
#define PLL_ANALOG__PLL_LF_MODE__SHIFT 0xc
#define PLL_ANALOG__PLL_VREG_FB_TRIM_MASK 0xe00000
#define PLL_ANALOG__PLL_VREG_FB_TRIM__SHIFT 0x15
#define PLL_ANALOG__PLL_IBIAS_MASK 0xff000000
#define PLL_ANALOG__PLL_IBIAS__SHIFT 0x18
#define PLL_VREG_CNTL__PLL_VREG_CNTL_MASK 0xfffff
#define PLL_VREG_CNTL__PLL_VREG_CNTL__SHIFT 0x0
#define PLL_VREG_CNTL__PLL_BG_VREG_BIAS_MASK 0x300000
#define PLL_VREG_CNTL__PLL_BG_VREG_BIAS__SHIFT 0x14
#define PLL_VREG_CNTL__PLL_VREF_SEL_MASK 0x4000000
#define PLL_VREG_CNTL__PLL_VREF_SEL__SHIFT 0x1a
#define PLL_VREG_CNTL__PLL_VREG_BIAS_MASK 0xf0000000
#define PLL_VREG_CNTL__PLL_VREG_BIAS__SHIFT 0x1c
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCK_DETECT_ENABLE_MASK 0x1
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCK_DETECT_ENABLE__SHIFT 0x0
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCK_DET_RES100_SELECT_MASK 0x2
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCK_DET_RES100_SELECT__SHIFT 0x1
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCK_STICKY_STATUS_MASK 0x4
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCK_STICKY_STATUS__SHIFT 0x2
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCK_DET_COUNT_MASK 0x70
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCK_DET_COUNT__SHIFT 0x4
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCKED_STICKY_RST_TEST_MASK 0x80
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCKED_STICKY_RST_TEST__SHIFT 0x7
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCKED_STICKY_TEST_READBACK_MASK 0x100
#define PLL_UNLOCK_DETECT_CNTL__PLL_UNLOCKED_STICKY_TEST_READBACK__SHIFT 0x8
#define PLL_DEBUG_CNTL__PLL_DEBUG_SIGNALS_ENABLE_MASK 0x1
#define PLL_DEBUG_CNTL__PLL_DEBUG_SIGNALS_ENABLE__SHIFT 0x0
#define PLL_DEBUG_CNTL__PLL_DEBUG_MUXOUT_SEL_MASK 0xf0
#define PLL_DEBUG_CNTL__PLL_DEBUG_MUXOUT_SEL__SHIFT 0x4
#define PLL_DEBUG_CNTL__PLL_DEBUG_CLK_SEL_MASK 0x1f00
#define PLL_DEBUG_CNTL__PLL_DEBUG_CLK_SEL__SHIFT 0x8
#define PLL_DEBUG_CNTL__PLL_DEBUG_ADC_CNTL_MASK 0xff0000
#define PLL_DEBUG_CNTL__PLL_DEBUG_ADC_CNTL__SHIFT 0x10
#define PLL_DEBUG_CNTL__PLL_DEBUG_ADC_READBACK_MASK 0x7000000
#define PLL_DEBUG_CNTL__PLL_DEBUG_ADC_READBACK__SHIFT 0x18
#define PLL_DEBUG_CNTL__PLL_DEBUG_ADC_EN_MASK 0x8000000
#define PLL_DEBUG_CNTL__PLL_DEBUG_ADC_EN__SHIFT 0x1b
#define PLL_UPDATE_LOCK__PLL_UPDATE_LOCK_MASK 0x1
#define PLL_UPDATE_LOCK__PLL_UPDATE_LOCK__SHIFT 0x0
#define PLL_UPDATE_CNTL__PLL_UPDATE_PENDING_MASK 0x1
#define PLL_UPDATE_CNTL__PLL_UPDATE_PENDING__SHIFT 0x0
#define PLL_UPDATE_CNTL__PLL_UPDATE_POINT_MASK 0x100
#define PLL_UPDATE_CNTL__PLL_UPDATE_POINT__SHIFT 0x8
#define PLL_UPDATE_CNTL__PLL_AUTO_RESET_DISABLE_MASK 0x10000
#define PLL_UPDATE_CNTL__PLL_AUTO_RESET_DISABLE__SHIFT 0x10
#define PLL_XOR_LOCK__PLL_XOR_LOCK_MASK 0x1
#define PLL_XOR_LOCK__PLL_XOR_LOCK__SHIFT 0x0
#define PLL_XOR_LOCK__PLL_XOR_LOCK_READBACK_MASK 0x2
#define PLL_XOR_LOCK__PLL_XOR_LOCK_READBACK__SHIFT 0x1
#define PLL_XOR_LOCK__PLL_SPARE_MASK 0x3f00
#define PLL_XOR_LOCK__PLL_SPARE__SHIFT 0x8
#define PLL_XOR_LOCK__PLL_LOCK_COUNT_SEL_MASK 0xf0000
#define PLL_XOR_LOCK__PLL_LOCK_COUNT_SEL__SHIFT 0x10
#define PLL_XOR_LOCK__PLL_LOCK_DETECTOR_RESOLUTION_FREF_MASK 0x700000
#define PLL_XOR_LOCK__PLL_LOCK_DETECTOR_RESOLUTION_FREF__SHIFT 0x14
#define PLL_XOR_LOCK__PLL_LOCK_DETECTOR_RESOLUTION_FFB_MASK 0x3800000
#define PLL_XOR_LOCK__PLL_LOCK_DETECTOR_RESOLUTION_FFB__SHIFT 0x17
#define PLL_XOR_LOCK__PLL_LOCK_DETECTOR_OPAMP_BIAS_MASK 0xc000000
#define PLL_XOR_LOCK__PLL_LOCK_DETECTOR_OPAMP_BIAS__SHIFT 0x1a
#define PLL_XOR_LOCK__PLL_FAST_LOCK_MODE_EN_MASK 0x10000000
#define PLL_XOR_LOCK__PLL_FAST_LOCK_MODE_EN__SHIFT 0x1c
#define PLL_ANALOG_CNTL__PLL_ANALOG_TEST_EN_MASK 0x1
#define PLL_ANALOG_CNTL__PLL_ANALOG_TEST_EN__SHIFT 0x0
#define PLL_ANALOG_CNTL__PLL_ANALOG_MUX_CNTL_MASK 0x1e
#define PLL_ANALOG_CNTL__PLL_ANALOG_MUX_CNTL__SHIFT 0x1
#define PLL_ANALOG_CNTL__PLL_ANALOGOUT_MUX_CNTL_MASK 0x1e0
#define PLL_ANALOG_CNTL__PLL_ANALOGOUT_MUX_CNTL__SHIFT 0x5
#define PLL_ANALOG_CNTL__PLL_REGREF_TRIM_MASK 0x3e00
#define PLL_ANALOG_CNTL__PLL_REGREF_TRIM__SHIFT 0x9
#define PLL_ANALOG_CNTL__PLL_CALIB_FBDIV_MASK 0x1c000
#define PLL_ANALOG_CNTL__PLL_CALIB_FBDIV__SHIFT 0xe
#define PLL_ANALOG_CNTL__PLL_CALIB_FASTCAL_MASK 0x20000
#define PLL_ANALOG_CNTL__PLL_CALIB_FASTCAL__SHIFT 0x11
#define PLL_ANALOG_CNTL__PLL_TEST_SSAMP_EN_MASK 0x40000
#define PLL_ANALOG_CNTL__PLL_TEST_SSAMP_EN__SHIFT 0x12
#define VGA25_PPLL_REF_DIV__VGA25_PPLL_REF_DIV_MASK 0x3ff
#define VGA25_PPLL_REF_DIV__VGA25_PPLL_REF_DIV__SHIFT 0x0
#define VGA28_PPLL_REF_DIV__VGA28_PPLL_REF_DIV_MASK 0x3ff
#define VGA28_PPLL_REF_DIV__VGA28_PPLL_REF_DIV__SHIFT 0x0
#define VGA41_PPLL_REF_DIV__VGA41_PPLL_REF_DIV_MASK 0x3ff
#define VGA41_PPLL_REF_DIV__VGA41_PPLL_REF_DIV__SHIFT 0x0
#define VGA25_PPLL_FB_DIV__VGA25_PPLL_FB_DIV_FRACTION_MASK 0xf
#define VGA25_PPLL_FB_DIV__VGA25_PPLL_FB_DIV_FRACTION__SHIFT 0x0
#define VGA25_PPLL_FB_DIV__VGA25_PPLL_FB_DIV_FRACTION_CNTL_MASK 0x30
#define VGA25_PPLL_FB_DIV__VGA25_PPLL_FB_DIV_FRACTION_CNTL__SHIFT 0x4
#define VGA25_PPLL_FB_DIV__VGA25_PPLL_FB_DIV_MASK 0x7ff0000
#define VGA25_PPLL_FB_DIV__VGA25_PPLL_FB_DIV__SHIFT 0x10
#define VGA28_PPLL_FB_DIV__VGA28_PPLL_FB_DIV_FRACTION_MASK 0xf
#define VGA28_PPLL_FB_DIV__VGA28_PPLL_FB_DIV_FRACTION__SHIFT 0x0
#define VGA28_PPLL_FB_DIV__VGA28_PPLL_FB_DIV_FRACTION_CNTL_MASK 0x30
#define VGA28_PPLL_FB_DIV__VGA28_PPLL_FB_DIV_FRACTION_CNTL__SHIFT 0x4
#define VGA28_PPLL_FB_DIV__VGA28_PPLL_FB_DIV_MASK 0x7ff0000
#define VGA28_PPLL_FB_DIV__VGA28_PPLL_FB_DIV__SHIFT 0x10
#define VGA41_PPLL_FB_DIV__VGA41_PPLL_FB_DIV_FRACTION_MASK 0xf
#define VGA41_PPLL_FB_DIV__VGA41_PPLL_FB_DIV_FRACTION__SHIFT 0x0
#define VGA41_PPLL_FB_DIV__VGA41_PPLL_FB_DIV_FRACTION_CNTL_MASK 0x30
#define VGA41_PPLL_FB_DIV__VGA41_PPLL_FB_DIV_FRACTION_CNTL__SHIFT 0x4
#define VGA41_PPLL_FB_DIV__VGA41_PPLL_FB_DIV_MASK 0x7ff0000
#define VGA41_PPLL_FB_DIV__VGA41_PPLL_FB_DIV__SHIFT 0x10
#define VGA25_PPLL_POST_DIV__VGA25_PPLL_POST_DIV_PIXCLK_MASK 0x7f
#define VGA25_PPLL_POST_DIV__VGA25_PPLL_POST_DIV_PIXCLK__SHIFT 0x0
#define VGA25_PPLL_POST_DIV__VGA25_PPLL_POST_DIV_DVOCLK_MASK 0x7f00
#define VGA25_PPLL_POST_DIV__VGA25_PPLL_POST_DIV_DVOCLK__SHIFT 0x8
#define VGA25_PPLL_POST_DIV__VGA25_PPLL_POST_DIV_IDCLK_MASK 0x7f0000
#define VGA25_PPLL_POST_DIV__VGA25_PPLL_POST_DIV_IDCLK__SHIFT 0x10
#define VGA28_PPLL_POST_DIV__VGA28_PPLL_POST_DIV_PIXCLK_MASK 0x7f
#define VGA28_PPLL_POST_DIV__VGA28_PPLL_POST_DIV_PIXCLK__SHIFT 0x0
#define VGA28_PPLL_POST_DIV__VGA28_PPLL_POST_DIV_DVOCLK_MASK 0x7f00
#define VGA28_PPLL_POST_DIV__VGA28_PPLL_POST_DIV_DVOCLK__SHIFT 0x8
#define VGA28_PPLL_POST_DIV__VGA28_PPLL_POST_DIV_IDCLK_MASK 0x7f0000
#define VGA28_PPLL_POST_DIV__VGA28_PPLL_POST_DIV_IDCLK__SHIFT 0x10
#define VGA41_PPLL_POST_DIV__VGA41_PPLL_POST_DIV_PIXCLK_MASK 0x7f
#define VGA41_PPLL_POST_DIV__VGA41_PPLL_POST_DIV_PIXCLK__SHIFT 0x0
#define VGA41_PPLL_POST_DIV__VGA41_PPLL_POST_DIV_DVOCLK_MASK 0x7f00
#define VGA41_PPLL_POST_DIV__VGA41_PPLL_POST_DIV_DVOCLK__SHIFT 0x8
#define VGA41_PPLL_POST_DIV__VGA41_PPLL_POST_DIV_IDCLK_MASK 0x7f0000
#define VGA41_PPLL_POST_DIV__VGA41_PPLL_POST_DIV_IDCLK__SHIFT 0x10
#define VGA25_PPLL_ANALOG__VGA25_CAL_MODE_MASK 0x1f
#define VGA25_PPLL_ANALOG__VGA25_CAL_MODE__SHIFT 0x0
#define VGA25_PPLL_ANALOG__VGA25_PPLL_PFD_PULSE_SEL_MASK 0x60
#define VGA25_PPLL_ANALOG__VGA25_PPLL_PFD_PULSE_SEL__SHIFT 0x5
#define VGA25_PPLL_ANALOG__VGA25_PPLL_CP_MASK 0xf00
#define VGA25_PPLL_ANALOG__VGA25_PPLL_CP__SHIFT 0x8
#define VGA25_PPLL_ANALOG__VGA25_PPLL_LF_MODE_MASK 0x1ff000
#define VGA25_PPLL_ANALOG__VGA25_PPLL_LF_MODE__SHIFT 0xc
#define VGA25_PPLL_ANALOG__VGA25_PPLL_IBIAS_MASK 0xff000000
#define VGA25_PPLL_ANALOG__VGA25_PPLL_IBIAS__SHIFT 0x18
#define VGA28_PPLL_ANALOG__VGA28_CAL_MODE_MASK 0x1f
#define VGA28_PPLL_ANALOG__VGA28_CAL_MODE__SHIFT 0x0
#define VGA28_PPLL_ANALOG__VGA28_PPLL_PFD_PULSE_SEL_MASK 0x60
#define VGA28_PPLL_ANALOG__VGA28_PPLL_PFD_PULSE_SEL__SHIFT 0x5
#define VGA28_PPLL_ANALOG__VGA28_PPLL_CP_MASK 0xf00
#define VGA28_PPLL_ANALOG__VGA28_PPLL_CP__SHIFT 0x8
#define VGA28_PPLL_ANALOG__VGA28_PPLL_LF_MODE_MASK 0x1ff000
#define VGA28_PPLL_ANALOG__VGA28_PPLL_LF_MODE__SHIFT 0xc
#define VGA28_PPLL_ANALOG__VGA28_PPLL_IBIAS_MASK 0xff000000
#define VGA28_PPLL_ANALOG__VGA28_PPLL_IBIAS__SHIFT 0x18
#define VGA41_PPLL_ANALOG__VGA41_CAL_MODE_MASK 0x1f
#define VGA41_PPLL_ANALOG__VGA41_CAL_MODE__SHIFT 0x0
#define VGA41_PPLL_ANALOG__VGA41_PPLL_PFD_PULSE_SEL_MASK 0x60
#define VGA41_PPLL_ANALOG__VGA41_PPLL_PFD_PULSE_SEL__SHIFT 0x5
#define VGA41_PPLL_ANALOG__VGA41_PPLL_CP_MASK 0xf00
#define VGA41_PPLL_ANALOG__VGA41_PPLL_CP__SHIFT 0x8
#define VGA41_PPLL_ANALOG__VGA41_PPLL_LF_MODE_MASK 0x1ff000
#define VGA41_PPLL_ANALOG__VGA41_PPLL_LF_MODE__SHIFT 0xc
#define VGA41_PPLL_ANALOG__VGA41_PPLL_IBIAS_MASK 0xff000000
#define VGA41_PPLL_ANALOG__VGA41_PPLL_IBIAS__SHIFT 0x18
#define DISPPLL_BG_CNTL__DISPPLL_BG_PDN_MASK 0x1
#define DISPPLL_BG_CNTL__DISPPLL_BG_PDN__SHIFT 0x0
#define DISPPLL_BG_CNTL__DISPPLL_BG_ADJ_MASK 0xf0
#define DISPPLL_BG_CNTL__DISPPLL_BG_ADJ__SHIFT 0x4
#define PPLL_DIV_UPDATE_DEBUG__PLL_REF_DIV_CHANGED_MASK 0x1
#define PPLL_DIV_UPDATE_DEBUG__PLL_REF_DIV_CHANGED__SHIFT 0x0
#define PPLL_DIV_UPDATE_DEBUG__PLL_FB_DIV_CHANGED_MASK 0x2
#define PPLL_DIV_UPDATE_DEBUG__PLL_FB_DIV_CHANGED__SHIFT 0x1
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_PENDING_MASK 0x4
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_PENDING__SHIFT 0x2
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_CURRENT_STATE_MASK 0x18
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_CURRENT_STATE__SHIFT 0x3
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_ENABLE_MASK 0x20
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_ENABLE__SHIFT 0x5
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_REQ_MASK 0x40
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_REQ__SHIFT 0x6
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_ACK_MASK 0x80
#define PPLL_DIV_UPDATE_DEBUG__PLL_UPDATE_ACK__SHIFT 0x7
#define PPLL_STATUS_DEBUG__PLL_DEBUG_BUS_MASK 0xffff
#define PPLL_STATUS_DEBUG__PLL_DEBUG_BUS__SHIFT 0x0
#define PPLL_STATUS_DEBUG__PLL_UNLOCK_MASK 0x10000
#define PPLL_STATUS_DEBUG__PLL_UNLOCK__SHIFT 0x10
#define PPLL_STATUS_DEBUG__PLL_CAL_RESULT_MASK 0x1e0000
#define PPLL_STATUS_DEBUG__PLL_CAL_RESULT__SHIFT 0x11
#define PPLL_STATUS_DEBUG__PLL_POWERGOOD_ISO_ENB_MASK 0x1000000
#define PPLL_STATUS_DEBUG__PLL_POWERGOOD_ISO_ENB__SHIFT 0x18
#define PPLL_STATUS_DEBUG__PLL_POWERGOOD_S_MASK 0x2000000
#define PPLL_STATUS_DEBUG__PLL_POWERGOOD_S__SHIFT 0x19
#define PPLL_STATUS_DEBUG__PLL_POWERGOOD_V_MASK 0x4000000
#define PPLL_STATUS_DEBUG__PLL_POWERGOOD_V__SHIFT 0x1a
#define PPLL_DEBUG_MUX_CNTL__DEBUG_BUS_MUX_SEL_MASK 0x1f
#define PPLL_DEBUG_MUX_CNTL__DEBUG_BUS_MUX_SEL__SHIFT 0x0
#define PPLL_SPARE0__PLL_SPARE0_MASK 0xffffffff
#define PPLL_SPARE0__PLL_SPARE0__SHIFT 0x0
#define PPLL_SPARE1__PLL_SPARE1_MASK 0xffffffff
#define PPLL_SPARE1__PLL_SPARE1__SHIFT 0x0
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR0_MASK 0x7
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR0__SHIFT 0x0
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR1_MASK 0x70
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR1__SHIFT 0x4
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR2_MASK 0x700
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR2__SHIFT 0x8
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR3_MASK 0x7000
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR3__SHIFT 0xc
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR4_MASK 0x70000
#define UNIPHY_TX_CONTROL1__UNIPHY_PREMPH_STR4__SHIFT 0x10
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS0_MASK 0x300000
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS0__SHIFT 0x14
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS1_MASK 0xc00000
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS1__SHIFT 0x16
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS2_MASK 0x3000000
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS2__SHIFT 0x18
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS3_MASK 0xc000000
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS3__SHIFT 0x1a
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS4_MASK 0x30000000
#define UNIPHY_TX_CONTROL1__UNIPHY_TX_VS4__SHIFT 0x1c
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH0_PC_MASK 0x3
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH0_PC__SHIFT 0x0
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH1_PC_MASK 0x30
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH1_PC__SHIFT 0x4
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH2_PC_MASK 0x300
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH2_PC__SHIFT 0x8
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH3_PC_MASK 0x3000
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH3_PC__SHIFT 0xc
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH4_PC_MASK 0x30000
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH4_PC__SHIFT 0x10
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH_SEL_MASK 0x100000
#define UNIPHY_TX_CONTROL2__UNIPHY_PREMPH_SEL__SHIFT 0x14
#define UNIPHY_TX_CONTROL2__UNIPHY_RT0_CPSEL_MASK 0x600000
#define UNIPHY_TX_CONTROL2__UNIPHY_RT0_CPSEL__SHIFT 0x15
#define UNIPHY_TX_CONTROL2__UNIPHY_RT1_CPSEL_MASK 0x1800000
#define UNIPHY_TX_CONTROL2__UNIPHY_RT1_CPSEL__SHIFT 0x17
#define UNIPHY_TX_CONTROL2__UNIPHY_RT2_CPSEL_MASK 0x6000000
#define UNIPHY_TX_CONTROL2__UNIPHY_RT2_CPSEL__SHIFT 0x19
#define UNIPHY_TX_CONTROL2__UNIPHY_RT3_CPSEL_MASK 0x18000000
#define UNIPHY_TX_CONTROL2__UNIPHY_RT3_CPSEL__SHIFT 0x1b
#define UNIPHY_TX_CONTROL2__UNIPHY_RT4_CPSEL_MASK 0x60000000
#define UNIPHY_TX_CONTROL2__UNIPHY_RT4_CPSEL__SHIFT 0x1d
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_PW_CLK_MASK 0x3
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_PW_CLK__SHIFT 0x0
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_PW_DAT_MASK 0xc
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_PW_DAT__SHIFT 0x2
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_CS_CLK_MASK 0xf0
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_CS_CLK__SHIFT 0x4
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_CS_DAT_MASK 0xf00
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_CS_DAT__SHIFT 0x8
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_STR_CLK_MASK 0xf000
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_STR_CLK__SHIFT 0xc
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_STR_DAT_MASK 0xf0000
#define UNIPHY_TX_CONTROL3__UNIPHY_PREMPH_STR_DAT__SHIFT 0x10
#define UNIPHY_TX_CONTROL3__UNIPHY_PESEL0_MASK 0x100000
#define UNIPHY_TX_CONTROL3__UNIPHY_PESEL0__SHIFT 0x14
#define UNIPHY_TX_CONTROL3__UNIPHY_PESEL1_MASK 0x200000
#define UNIPHY_TX_CONTROL3__UNIPHY_PESEL1__SHIFT 0x15
#define UNIPHY_TX_CONTROL3__UNIPHY_PESEL2_MASK 0x400000
#define UNIPHY_TX_CONTROL3__UNIPHY_PESEL2__SHIFT 0x16
#define UNIPHY_TX_CONTROL3__UNIPHY_PESEL3_MASK 0x800000
#define UNIPHY_TX_CONTROL3__UNIPHY_PESEL3__SHIFT 0x17
#define UNIPHY_TX_CONTROL3__UNIPHY_TX_VS_ADJ_MASK 0x1f000000
#define UNIPHY_TX_CONTROL3__UNIPHY_TX_VS_ADJ__SHIFT 0x18
#define UNIPHY_TX_CONTROL3__UNIPHY_LVDS_PULLDWN_MASK 0x80000000
#define UNIPHY_TX_CONTROL3__UNIPHY_LVDS_PULLDWN__SHIFT 0x1f
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_NVS_CLK_MASK 0x1f
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_NVS_CLK__SHIFT 0x0
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_NVS_DAT_MASK 0x3e0
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_NVS_DAT__SHIFT 0x5
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_PVS_CLK_MASK 0x1f000
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_PVS_CLK__SHIFT 0xc
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_PVS_DAT_MASK 0x3e0000
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_PVS_DAT__SHIFT 0x11
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_OP_CLK_MASK 0x7000000
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_OP_CLK__SHIFT 0x18
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_OP_DAT_MASK 0x70000000
#define UNIPHY_TX_CONTROL4__UNIPHY_TX_OP_DAT__SHIFT 0x1c
#define UNIPHY_POWER_CONTROL__UNIPHY_BGPDN_MASK 0x1
#define UNIPHY_POWER_CONTROL__UNIPHY_BGPDN__SHIFT 0x0
#define UNIPHY_POWER_CONTROL__UNIPHY_RST_LOGIC_MASK 0x2
#define UNIPHY_POWER_CONTROL__UNIPHY_RST_LOGIC__SHIFT 0x1
#define UNIPHY_POWER_CONTROL__UNIPHY_BIASREF_SEL_MASK 0xc
#define UNIPHY_POWER_CONTROL__UNIPHY_BIASREF_SEL__SHIFT 0x2
#define UNIPHY_POWER_CONTROL__UNIPHY_BGADJ1P00_MASK 0xf00
#define UNIPHY_POWER_CONTROL__UNIPHY_BGADJ1P00__SHIFT 0x8
#define UNIPHY_POWER_CONTROL__UNIPHY_BGADJ1P25_MASK 0xf000
#define UNIPHY_POWER_CONTROL__UNIPHY_BGADJ1P25__SHIFT 0xc
#define UNIPHY_POWER_CONTROL__UNIPHY_BGADJ0P45_MASK 0xf0000
#define UNIPHY_POWER_CONTROL__UNIPHY_BGADJ0P45__SHIFT 0x10
#define UNIPHY_PLL_FBDIV__UNIPHY_PLL_FBDIV_FRACTION_MASK 0xfffc
#define UNIPHY_PLL_FBDIV__UNIPHY_PLL_FBDIV_FRACTION__SHIFT 0x2
#define UNIPHY_PLL_FBDIV__UNIPHY_PLL_FBDIV_MASK 0xfff0000
#define UNIPHY_PLL_FBDIV__UNIPHY_PLL_FBDIV__SHIFT 0x10
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_ENABLE_MASK 0x1
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_ENABLE__SHIFT 0x0
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_RESET_MASK 0x2
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_RESET__SHIFT 0x1
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_EXT_RESET_EN_MASK 0x4
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_EXT_RESET_EN__SHIFT 0x2
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_CLK_EN_MASK 0x8
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_CLK_EN__SHIFT 0x3
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_CLKPH_EN_MASK 0xf0
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_CLKPH_EN__SHIFT 0x4
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_LF_CNTL_MASK 0x7f00
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_LF_CNTL__SHIFT 0x8
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_BW_CNTL_MASK 0xff0000
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_BW_CNTL__SHIFT 0x10
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_TEST_BYPCLK_SRC_MASK 0x1000000
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_TEST_BYPCLK_SRC__SHIFT 0x18
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_TEST_BYPCLK_EN_MASK 0x2000000
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_TEST_BYPCLK_EN__SHIFT 0x19
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_TEST_VCTL_ADC_EN_MASK 0x4000000
#define UNIPHY_PLL_CONTROL1__UNIPHY_PLL_TEST_VCTL_ADC_EN__SHIFT 0x1a
#define UNIPHY_PLL_CONTROL1__UNIPHY_VCO_MODE_MASK 0x30000000
#define UNIPHY_PLL_CONTROL1__UNIPHY_VCO_MODE__SHIFT 0x1c
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_DISPCLK_MODE_MASK 0x3
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_DISPCLK_MODE__SHIFT 0x0
#define UNIPHY_PLL_CONTROL2__UNIPHY_DPLLSEL_MASK 0xc
#define UNIPHY_PLL_CONTROL2__UNIPHY_DPLLSEL__SHIFT 0x2
#define UNIPHY_PLL_CONTROL2__UNIPHY_IDCLK_SEL_MASK 0x10
#define UNIPHY_PLL_CONTROL2__UNIPHY_IDCLK_SEL__SHIFT 0x4
#define UNIPHY_PLL_CONTROL2__UNIPHY_IPCIE_REFCLK_SEL_MASK 0x20
#define UNIPHY_PLL_CONTROL2__UNIPHY_IPCIE_REFCLK_SEL__SHIFT 0x5
#define UNIPHY_PLL_CONTROL2__UNIPHY_IXTALIN_SEL_MASK 0x40
#define UNIPHY_PLL_CONTROL2__UNIPHY_IXTALIN_SEL__SHIFT 0x6
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_REFCLK_SRC_MASK 0x700
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_REFCLK_SRC__SHIFT 0x8
#define UNIPHY_PLL_CONTROL2__UNIPHY_PCIEREF_CLK_EN_MASK 0x800
#define UNIPHY_PLL_CONTROL2__UNIPHY_PCIEREF_CLK_EN__SHIFT 0xb
#define UNIPHY_PLL_CONTROL2__UNIPHY_IDCLK_EN_MASK 0x1000
#define UNIPHY_PLL_CONTROL2__UNIPHY_IDCLK_EN__SHIFT 0xc
#define UNIPHY_PLL_CONTROL2__UNIPHY_CLKINV_MASK 0x2000
#define UNIPHY_PLL_CONTROL2__UNIPHY_CLKINV__SHIFT 0xd
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_VTOI_BIAS_CNTL_MASK 0x10000
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_VTOI_BIAS_CNTL__SHIFT 0x10
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_TEST_FBDIV_FRAC_BYPASS_MASK 0x80000
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_TEST_FBDIV_FRAC_BYPASS__SHIFT 0x13
#define UNIPHY_PLL_CONTROL2__UNIPHY_PDIVFRAC_SEL_MASK 0x100000
#define UNIPHY_PLL_CONTROL2__UNIPHY_PDIVFRAC_SEL__SHIFT 0x14
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_REFDIV_MASK 0x1f000000
#define UNIPHY_PLL_CONTROL2__UNIPHY_PLL_REFDIV__SHIFT 0x18
#define UNIPHY_PLL_CONTROL2__UNIPHY_PDIV_SEL_MASK 0xe0000000
#define UNIPHY_PLL_CONTROL2__UNIPHY_PDIV_SEL__SHIFT 0x1d
#define UNIPHY_PLL_SS_STEP_SIZE__UNIPHY_PLL_SS_STEP_SIZE_MASK 0x3ffff