
#define GENERAL_PWRMGT__GPU_COUNTER_CLK_MASK 0x8000
#define GENERAL_PWRMGT__GPU_COUNTER_CLK__SHIFT 0xf
#define GENERAL_PWRMGT__GPU_COUNTER_OFF_MASK 0x10000
#define GENERAL_PWRMGT__GPU_COUNTER_OFF__SHIFT 0x10
#define GENERAL_PWRMGT__GPU_COUNTER_INTF_OFF_MASK 0x20000
#define GENERAL_PWRMGT__GPU_COUNTER_INTF_OFF__SHIFT 0x11
#define GENERAL_PWRMGT__SPARE18_MASK 0x40000
#define GENERAL_PWRMGT__SPARE18__SHIFT 0x12
#define GENERAL_PWRMGT__ACPI_D3_VID_MASK 0x180000
#define GENERAL_PWRMGT__ACPI_D3_VID__SHIFT 0x13
#define GENERAL_PWRMGT__DYN_SPREAD_SPECTRUM_EN_MASK 0x800000
#define GENERAL_PWRMGT__DYN_SPREAD_SPECTRUM_EN__SHIFT 0x17
#define GENERAL_PWRMGT__SPARE27_MASK 0x8000000
#define GENERAL_PWRMGT__SPARE27__SHIFT 0x1b
#define GENERAL_PWRMGT__SPARE_MASK 0xf0000000
#define GENERAL_PWRMGT__SPARE__SHIFT 0x1c
#define CNB_PWRMGT_CNTL__GNB_SLOW_MODE_MASK 0x3
#define CNB_PWRMGT_CNTL__GNB_SLOW_MODE__SHIFT 0x0
#define CNB_PWRMGT_CNTL__GNB_SLOW_MASK 0x4
#define CNB_PWRMGT_CNTL__GNB_SLOW__SHIFT 0x2
#define CNB_PWRMGT_CNTL__FORCE_NB_PS1_MASK 0x8
#define CNB_PWRMGT_CNTL__FORCE_NB_PS1__SHIFT 0x3
#define CNB_PWRMGT_CNTL__DPM_ENABLED_MASK 0x10
#define CNB_PWRMGT_CNTL__DPM_ENABLED__SHIFT 0x4
#define CNB_PWRMGT_CNTL__SPARE_MASK 0xffffffe0
#define CNB_PWRMGT_CNTL__SPARE__SHIFT 0x5
#define SCLK_PWRMGT_CNTL__SCLK_PWRMGT_OFF_MASK 0x1
#define SCLK_PWRMGT_CNTL__SCLK_PWRMGT_OFF__SHIFT 0x0
#define SCLK_PWRMGT_CNTL__SCLK_LOW_D1_MASK 0x2
#define SCLK_PWRMGT_CNTL__SCLK_LOW_D1__SHIFT 0x1
#define SCLK_PWRMGT_CNTL__DYN_PWR_DOWN_EN_MASK 0x4
#define SCLK_PWRMGT_CNTL__DYN_PWR_DOWN_EN__SHIFT 0x2
#define SCLK_PWRMGT_CNTL__RESET_BUSY_CNT_MASK 0x10
#define SCLK_PWRMGT_CNTL__RESET_BUSY_CNT__SHIFT 0x4
#define SCLK_PWRMGT_CNTL__RESET_SCLK_CNT_MASK 0x20
#define SCLK_PWRMGT_CNTL__RESET_SCLK_CNT__SHIFT 0x5
#define SCLK_PWRMGT_CNTL__RESERVED_0_MASK 0x40
#define SCLK_PWRMGT_CNTL__RESERVED_0__SHIFT 0x6
#define SCLK_PWRMGT_CNTL__DYN_GFX_CLK_OFF_EN_MASK 0x80
#define SCLK_PWRMGT_CNTL__DYN_GFX_CLK_OFF_EN__SHIFT 0x7
#define SCLK_PWRMGT_CNTL__GFX_CLK_FORCE_ON_MASK 0x100
#define SCLK_PWRMGT_CNTL__GFX_CLK_FORCE_ON__SHIFT 0x8
#define SCLK_PWRMGT_CNTL__GFX_CLK_REQUEST_OFF_MASK 0x200
#define SCLK_PWRMGT_CNTL__GFX_CLK_REQUEST_OFF__SHIFT 0x9
#define SCLK_PWRMGT_CNTL__GFX_CLK_FORCE_OFF_MASK 0x400
#define SCLK_PWRMGT_CNTL__GFX_CLK_FORCE_OFF__SHIFT 0xa
#define SCLK_PWRMGT_CNTL__GFX_CLK_OFF_ACPI_D1_MASK 0x800
#define SCLK_PWRMGT_CNTL__GFX_CLK_OFF_ACPI_D1__SHIFT 0xb
#define SCLK_PWRMGT_CNTL__GFX_CLK_OFF_ACPI_D2_MASK 0x1000
#define SCLK_PWRMGT_CNTL__GFX_CLK_OFF_ACPI_D2__SHIFT 0xc
#define SCLK_PWRMGT_CNTL__GFX_CLK_OFF_ACPI_D3_MASK 0x2000
#define SCLK_PWRMGT_CNTL__GFX_CLK_OFF_ACPI_D3__SHIFT 0xd
#define SCLK_PWRMGT_CNTL__DYN_LIGHT_SLEEP_EN_MASK 0x4000
#define SCLK_PWRMGT_CNTL__DYN_LIGHT_SLEEP_EN__SHIFT 0xe
#define SCLK_PWRMGT_CNTL__AUTO_SCLK_PULSE_SKIP_MASK 0x8000
#define SCLK_PWRMGT_CNTL__AUTO_SCLK_PULSE_SKIP__SHIFT 0xf
#define SCLK_PWRMGT_CNTL__LIGHT_SLEEP_COUNTER_MASK 0x1f0000
#define SCLK_PWRMGT_CNTL__LIGHT_SLEEP_COUNTER__SHIFT 0x10
#define SCLK_PWRMGT_CNTL__DYNAMIC_PM_EN_MASK 0x200000
#define SCLK_PWRMGT_CNTL__DYNAMIC_PM_EN__SHIFT 0x15
#define SCLK_PWRMGT_CNTL__DPM_DYN_PWR_DOWN_CNTL_MASK 0x400000
#define SCLK_PWRMGT_CNTL__DPM_DYN_PWR_DOWN_CNTL__SHIFT 0x16
#define SCLK_PWRMGT_CNTL__DPM_DYN_PWR_DOWN_EN_MASK 0x800000
#define SCLK_PWRMGT_CNTL__DPM_DYN_PWR_DOWN_EN__SHIFT 0x17
#define SCLK_PWRMGT_CNTL__RESERVED_3_MASK 0x1000000
#define SCLK_PWRMGT_CNTL__RESERVED_3__SHIFT 0x18
#define SCLK_PWRMGT_CNTL__VOLTAGE_UPDATE_EN_MASK 0x2000000
#define SCLK_PWRMGT_CNTL__VOLTAGE_UPDATE_EN__SHIFT 0x19
#define SCLK_PWRMGT_CNTL__FORCE_PM0_INTERRUPT_MASK 0x10000000
#define SCLK_PWRMGT_CNTL__FORCE_PM0_INTERRUPT__SHIFT 0x1c
#define SCLK_PWRMGT_CNTL__FORCE_PM1_INTERRUPT_MASK 0x20000000
#define SCLK_PWRMGT_CNTL__FORCE_PM1_INTERRUPT__SHIFT 0x1d
#define SCLK_PWRMGT_CNTL__GFX_VOLTAGE_CHANGE_EN_MASK 0x40000000
#define SCLK_PWRMGT_CNTL__GFX_VOLTAGE_CHANGE_EN__SHIFT 0x1e
#define SCLK_PWRMGT_CNTL__GFX_VOLTAGE_CHANGE_MODE_MASK 0x80000000
#define SCLK_PWRMGT_CNTL__GFX_VOLTAGE_CHANGE_MODE__SHIFT 0x1f
#define TARGET_AND_CURRENT_PROFILE_INDEX__TARGET_STATE_MASK 0xf
#define TARGET_AND_CURRENT_PROFILE_INDEX__TARGET_STATE__SHIFT 0x0
#define TARGET_AND_CURRENT_PROFILE_INDEX__CURRENT_STATE_MASK 0xf0
#define TARGET_AND_CURRENT_PROFILE_INDEX__CURRENT_STATE__SHIFT 0x4
#define TARGET_AND_CURRENT_PROFILE_INDEX__CURR_MCLK_INDEX_MASK 0xf00
#define TARGET_AND_CURRENT_PROFILE_INDEX__CURR_MCLK_INDEX__SHIFT 0x8
#define TARGET_AND_CURRENT_PROFILE_INDEX__TARG_MCLK_INDEX_MASK 0xf000
#define TARGET_AND_CURRENT_PROFILE_INDEX__TARG_MCLK_INDEX__SHIFT 0xc
#define TARGET_AND_CURRENT_PROFILE_INDEX__CURR_SCLK_INDEX_MASK 0x1f0000
#define TARGET_AND_CURRENT_PROFILE_INDEX__CURR_SCLK_INDEX__SHIFT 0x10
#define TARGET_AND_CURRENT_PROFILE_INDEX__TARG_SCLK_INDEX_MASK 0x3e00000
#define TARGET_AND_CURRENT_PROFILE_INDEX__TARG_SCLK_INDEX__SHIFT 0x15
#define TARGET_AND_CURRENT_PROFILE_INDEX__CURR_LCLK_INDEX_MASK 0x1c000000
#define TARGET_AND_CURRENT_PROFILE_INDEX__CURR_LCLK_INDEX__SHIFT 0x1a
#define TARGET_AND_CURRENT_PROFILE_INDEX__TARG_LCLK_INDEX_MASK 0xe0000000
#define TARGET_AND_CURRENT_PROFILE_INDEX__TARG_LCLK_INDEX__SHIFT 0x1d
#define CG_FREQ_TRAN_VOTING_0__BIF_FREQ_THROTTLING_VOTE_EN_MASK 0x1
#define CG_FREQ_TRAN_VOTING_0__BIF_FREQ_THROTTLING_VOTE_EN__SHIFT 0x0
#define CG_FREQ_TRAN_VOTING_0__HDP_FREQ_THROTTLING_VOTE_EN_MASK 0x2
#define CG_FREQ_TRAN_VOTING_0__HDP_FREQ_THROTTLING_VOTE_EN__SHIFT 0x1
#define CG_FREQ_TRAN_VOTING_0__ROM_FREQ_THROTTLING_VOTE_EN_MASK 0x4
#define