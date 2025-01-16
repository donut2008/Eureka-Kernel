_TC1__ENABLE_MASK 0x1
#define MC_WR_TC1__ENABLE__SHIFT 0x0
#define MC_WR_TC1__PRESCALE_MASK 0x6
#define MC_WR_TC1__PRESCALE__SHIFT 0x1
#define MC_WR_TC1__BLACKOUT_EXEMPT_MASK 0x8
#define MC_WR_TC1__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_WR_TC1__STALL_MODE_MASK 0x30
#define MC_WR_TC1__STALL_MODE__SHIFT 0x4
#define MC_WR_TC1__STALL_OVERRIDE_MASK 0x40
#define MC_WR_TC1__STALL_OVERRIDE__SHIFT 0x6
#define MC_WR_TC1__MAX_BURST_MASK 0x780
#define MC_WR_TC1__MAX_BURST__SHIFT 0x7
#define MC_WR_TC1__LAZY_TIMER_MASK 0x7800
#define MC_WR_TC1__LAZY_TIMER__SHIFT 0xb
#define MC_WR_TC1__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_WR_TC1__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_CITF_INT_CREDITS_WR__CNTR_WR_HUB_MASK 0x3f
#define MC_CITF_INT_CREDITS_WR__CNTR_WR_HUB__SHIFT 0x0
#define MC_CITF_INT_CREDITS_WR__CNTR_WR_LCL_MASK 0xfc0
#define MC_CITF_INT_CREDITS_WR__CNTR_WR_LCL__SHIFT 0x6
#define MC_CITF_CREDITS_ARB_RD2__READ_MED_MASK 0xff
#define MC_CITF_CREDITS_ARB_RD2__READ_MED__SHIFT 0x0
#define MC_CITF_WTM_RD_CNTL__GROUP0_DECREMENT_MASK 0x7
#define MC_CITF_WTM_RD_CNTL__GROUP0_DECREMENT__SHIFT 0x0
#define MC_CITF_WTM_RD_CNTL__GROUP1_DECREMENT_MASK 0x38
#define MC_CITF_WTM_RD_CNTL__GROUP1_DECREMENT__SHIFT 0x3
#define MC_CITF_WTM_RD_CNTL__GROUP2_DECREMENT_MASK 0x1c0
#define MC_CITF_WTM_RD_CNTL__GROUP2_DECREMENT__SHIFT 0x6
#define MC_CITF_WTM_RD_CNTL__GROUP3_DECREMENT_MASK 0xe00
#define MC_CITF_WTM_RD_CNTL__GROUP3_DECREMENT__SHIFT 0x9
#define MC_CITF_WTM_RD_CNTL__GROUP4_DECREMENT_MASK 0x7000
#define MC_CITF_WTM_RD_CNTL__GROUP4_DECREMENT__SHIFT 0xc
#define MC_CITF_WTM_RD_CNTL__GROUP5_DECREMENT_MASK 0x38000
#define MC_CITF_WTM_RD_CNTL__GROUP5_DECREMENT__SHIFT 0xf
#define MC_CITF_WTM_RD_CNTL__GROUP6_DECREMENT_MASK 0x1c0000
#define MC_CITF_WTM_RD_CNTL__GROUP6_DECREMENT__SHIFT 0x12
#define MC_CITF_WTM_RD_CNTL__GROUP7_DECREMENT_MASK 0xe00000
#define MC_CITF_WTM_RD_CNTL__GROUP7_DECREMENT__SHIFT 0x15
#define MC_CITF_WTM_RD_CNTL__DISABLE_REMOTE_MASK 0x1000000
#define MC_CITF_WTM_RD_CNTL__DISABLE_REMOTE__SHIFT 0x18
#define MC_CITF_WTM_RD_CNTL__DISABLE_LOCAL_MASK 0x2000000
#define MC_CITF_WTM_RD_CNTL__DISABLE_LOCAL__SHIFT 0x19
#define MC_CITF_WTM_WR_CNTL__GROUP0_DECREMENT_MASK 0x7
#define MC_CITF_WTM_WR_CNTL__GROUP0_DECREMENT__SHIFT 0x0
#define MC_CITF_WTM_WR_CNTL__GROUP1_DECREMENT_MASK 0x38
#define MC_CITF_WTM_WR_CNTL__GROUP1_DECREMENT__SHIFT 0x3
#define MC_CITF_WTM_WR_CNTL__GROUP2_DECREMENT_MASK 0x1c0
#define MC_CITF_WTM_WR_CNTL__GROUP2_DECREMENT__SHIFT 0x6
#define MC_CITF_WTM_WR_CNTL__GROUP3_DECREMENT_MASK 0xe00
#define MC_CITF_WTM_WR_CNTL__GROUP3_DECREMENT__SHIFT 0x9
#define MC_CITF_WTM_WR_CNTL__GROUP4_DECREMENT_MASK 0x7000
#define MC_CITF_WTM_WR_CNTL__GROUP4_DECREMENT__SHIFT 0xc
#define MC_CITF_WTM_WR_CNTL__GROUP5_DECREMENT_MASK 0x38000
#define MC_CITF_WTM_WR_CNTL__GROUP5_DECREMENT__SHIFT 0xf
#define MC_CITF_WTM_WR_CNTL__GROUP6_DECREMENT_MASK 0x1c0000
#define MC_CITF_WTM_WR_CNTL__GROUP6_DECREMENT__SHIFT 0x12
#define MC_CITF_WTM_WR_CNTL__GROUP7_DECREMENT_MASK 0xe00000
#define MC_CITF_WTM_WR_CNTL__GROUP7_DECREMENT__SHIFT 0x15
#define MC_CITF_WTM_WR_CNTL__DISABLE_REMOTE_MASK 0x1000000
#define MC_CITF_WTM_WR_CNTL__DISABLE_REMOTE__SHIFT 0x18
#define MC_CITF_WTM_WR_CNTL__DISABLE_LOCAL_MASK 0x2000000
#define MC_CITF_WTM_WR_CNTL__DISABLE_LOCAL__SHIFT 0x19
#define MC_RD_CB__ENABLE_MASK 0x1
#define MC_RD_CB__ENABLE__SHIFT 0x0
#define MC_RD_CB__PRESCALE_MASK 0x6
#define MC_RD_CB__PRESCALE__SHIFT 0x1
#define MC_RD_CB__BLACKOUT_EXEMPT_MASK 0x8
#define MC_RD_CB__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_RD_CB__STALL_MODE_MASK 0x30
#define MC_RD_CB__STALL_MODE__SHIFT 0x4
#define MC_RD_CB__STALL_OVERRIDE_MASK 0x40
#define MC_RD_CB__STALL_OVERRIDE__SHIFT 0x6
#define MC_RD_CB__MAX_BURST_MASK 0x780
#define MC_RD_CB__MAX_BURST__SHIFT 0x7
#define MC_RD_CB__LAZY_TIMER_MASK 0x7800
#define MC_RD_CB__LAZY_TIMER__SHIFT 0xb
#define MC_RD_CB__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_RD_CB__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_RD_DB__ENABLE_MASK 0x1
#define MC_RD_DB__ENABLE__SHIFT 0x0
#define MC_RD_DB__PRESCALE_MASK 0x6
#define MC_RD_DB__PRESCALE__SHIFT 0x1
#define MC_RD_DB__BLACKOUT_EXEMPT_MASK 0x8
#define MC_RD_DB__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_RD_DB__STALL_MODE_MASK 0x30
#define MC_RD_DB__STALL_MODE__SHIFT 0x4
#define MC_RD_DB__STALL_OVERRIDE_MASK 0x40
#define MC_RD_DB__STALL_OVERRIDE__SHIFT 0x6
#define MC_RD_DB__MAX_BURST_MASK 0x780
#define MC_RD_DB__MAX_BURST__SHIFT 0x7
#define MC_RD_DB__LAZY_TIMER_MASK 0x7800
#define MC_RD_DB__LAZY_TIMER__SHIFT 0xb
#define MC_RD_DB__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_RD_DB__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_RD_TC0__ENABLE_MASK 0x1
#define MC_RD_TC0__ENABLE__SHIFT 0x0
#define MC_RD_TC0__PRESCALE_MASK 0x6
#define MC_RD_TC0__PRESCALE__SHIFT 0x1
#define MC_RD_TC0__BLACKOUT_EXEMPT_MASK 0x8
#define MC_RD_TC0__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_RD_TC0__STALL_MODE_MASK 0x30
#define MC_RD_TC0__STALL_MODE__SHIFT 0x4
#define MC_RD_TC0__STALL_OVERRIDE_MASK 0x40
#define MC_RD_TC0__STALL_OVERRIDE__SHIFT 0x6
#define MC_RD_TC0__MAX_BURST_MASK 0x780
#define MC_RD_TC0__MAX_BURST__SHIFT 0x7
#define MC_RD_TC0__LAZY_TIMER_MASK 0x7800
#define MC_RD_TC0__LAZY_TIMER__SHIFT 0xb
#define MC_RD_TC0__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_RD_TC0__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_RD_TC1__ENABLE_MASK 0x1
#define MC_RD_TC1__ENABLE__SHIFT 0x0
#define MC_RD_TC1__PRESCALE_MASK 0x6
#define MC_RD_TC1__PRESCALE__SHIFT 0x1
#define MC_RD_TC1__BLACKOUT_EXEMPT_MASK 0x8
#define MC_RD_TC1__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_RD_TC1__STALL_MODE_MASK 0x30
#define MC_RD_TC1__STALL_MODE__SHIFT 0x4
#define MC_RD_TC1__STALL_OVERRIDE_MASK 0x40
#define MC_RD_TC1__STALL_OVERRIDE__SHIFT 0x6
#define MC_RD_TC1__MAX_BURST_MASK 0x780
#define MC_RD_TC1__MAX_BURST__SHIFT 0x7
#define MC_RD_TC1__LAZY_TIMER_MASK 0x7800
#define MC_RD_TC1__LAZY_TIMER__SHIFT 0xb
#define MC_RD_TC1__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_RD_TC1__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_RD_HUB__ENABLE_MASK 0x1
#define MC_RD_HUB__ENABLE__SHIFT 0x0
#define MC_RD_HUB__PRESCALE_MASK 0x6
#define MC_RD_HUB__PRESCALE__SHIFT 0x1
#define MC_RD_HUB__BLACKOUT_EXEMPT_MASK 0x8
#define MC_RD_HUB__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_RD_HUB__STALL_MODE_MASK 0x30
#define MC_RD_HUB__STALL_MODE__SHIFT 0x4
#define MC_RD_HUB__STALL_OVERRIDE_MASK 0x40
#define MC_RD_HUB__STALL_OVERRIDE__SHIFT 0x6
#define MC_RD_HUB__MAX_BURST_MASK 0x780
#define MC_RD_HUB__MAX_BURST__SHIFT 0x7
#define MC_RD_HUB__LAZY_TIMER_MASK 0x7800
#define MC_RD_HUB__LAZY_TIMER__SHIFT 0xb
#define MC_RD_HUB__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_RD_HUB__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_WR_CB__ENABLE_MASK 0x1
#define MC_WR_CB__ENABLE__SHIFT 0x0
#define MC_WR_CB__PRESCALE_MASK 0x6
#define MC_WR_CB__PRESCALE__SHIFT 0x1
#define MC_WR_CB__BLACKOUT_EXEMPT_MASK 0x8
#define MC_WR_CB__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_WR_CB__STALL_MODE_MASK 0x30
#define MC_WR_CB__STALL_MODE__SHIFT 0x4
#define MC_WR_CB__STALL_OVERRIDE_MASK 0x40
#define MC_WR_CB__STALL_OVERRIDE__SHIFT 0x6
#define MC_WR_CB__MAX_BURST_MASK 0x780
#define MC_WR_CB__MAX_BURST__SHIFT 0x7
#define MC_WR_CB__LAZY_TIMER_MASK 0x7800
#define MC_WR_CB__LAZY_TIMER__SHIFT 0xb
#define MC_WR_CB__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_WR_CB__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_WR_DB__ENABLE_MASK 0x1
#define MC_WR_DB__ENABLE__SHIFT 0x0
#define MC_WR_DB__PRESCALE_MASK 0x6
#define MC_WR_DB__PRESCALE__SHIFT 0x1
#define MC_WR_DB__BLACKOUT_EXEMPT_MASK 0x8
#define MC_WR_DB__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_WR_DB__STALL_MODE_MASK 0x30
#define MC_WR_DB__STALL_MODE__SHIFT 0x4
#define MC_WR_DB__STALL_OVERRIDE_MASK 0x40
#define MC_WR_DB__STALL_OVERRIDE__SHIFT 0x6
#define MC_WR_DB__MAX_BURST_MASK 0x780
#define MC_WR_DB__MAX_BURST__SHIFT 0x7
#define MC_WR_DB__LAZY_TIMER_MASK 0x7800
#define MC_WR_DB__LAZY_TIMER__SHIFT 0xb
#define MC_WR_DB__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_WR_DB__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_WR_HUB__ENABLE_MASK 0x1
#define MC_WR_HUB__ENABLE__SHIFT 0x0
#define MC_WR_HUB__PRESCALE_MASK 0x6
#define MC_WR_HUB__PRESCALE__SHIFT 0x1
#define MC_WR_HUB__BLACKOUT_EXEMPT_MASK 0x8
#define MC_WR_HUB__BLACKOUT_EXEMPT__SHIFT 0x3
#define MC_WR_HUB__STALL_MODE_MASK 0x30
#define MC_WR_HUB__STALL_MODE__SHIFT 0x4
#define MC_WR_HUB__STALL_OVERRIDE_MASK 0x40
#define MC_WR_HUB__STALL_OVERRIDE__SHIFT 0x6
#define MC_WR_HUB__MAX_BURST_MASK 0x780
#define MC_WR_HUB__MAX_BURST__SHIFT 0x7
#define MC_WR_HUB__LAZY_TIMER_MASK 0x7800
#define MC_WR_HUB__LAZY_TIMER__SHIFT 0xb
#define MC_WR_HUB__STALL_OVERRIDE_WTM_MASK 0x8000
#define MC_WR_HUB__STALL_OVERRIDE_WTM__SHIFT 0xf
#define MC_CITF_CREDITS_XBAR__READ_LCL_MASK 0xff
#define MC_CITF_CREDITS_XBAR__READ_LCL__SHIFT 0x0
#define MC_CITF_CREDITS_XBAR__WRITE_LCL_MASK 0xff00
#define MC_CITF_CREDITS_XBAR__WRITE_LCL__SHIFT 0x8
#define MC_RD_GRP_LCL__CB0_MASK 0xf000
#define MC_RD_GRP_LCL__CB0__SHIFT 0xc
#define MC_RD_GRP_LCL__CBCMASK0_MASK 0xf0000
#define MC_RD_GRP_LCL__CBCMASK0__SHIFT 0x10
#define MC_RD_GRP_LCL__CBFMASK0_MASK 0xf00000
#define MC_RD_GRP_LCL__CBFMASK0__SHIFT 0x14
#define MC_RD_GRP_LCL__DB0_MASK 0xf000000
#define MC_RD_GRP_LCL__DB0__SHIFT 0x18
#define MC_RD_GRP_LCL__DBHTILE0_MASK 0xf0000000
#define MC_RD_GRP_LCL__DBHTILE0__SHIFT 0x1c
#define MC_WR_GRP_LCL__CB0_MASK 0xf
#define MC_WR_GRP_LCL__CB0__SHIFT 0x0
#define MC_WR_GRP_LCL__CBCMASK0_MASK 0xf0
#define MC_WR_GRP_LCL__CBCMASK0__SHIFT 0x4
#define MC_WR_GRP_LCL__CBFMASK0_MASK 0xf00
#define MC_WR_GRP_LCL__CBFMASK0__SHIFT 0x8
#define MC_WR_GRP_LCL__DB0_MASK 0xf000
#define MC_WR_GRP_LCL__DB0__SHIFT 0xc
#define MC_WR_GRP_LCL__DBHTILE0_MASK 0xf0000
#define MC_WR_GRP_LCL__DBHTILE0__SHIFT 0x10
#define MC_WR_GRP_LCL__SX0_MASK 0xf00000
#define MC_WR_GRP_LCL__SX0__SHIFT 0x14
#define MC_WR_GRP_LCL__CBIMMED0_MASK 0xf0000000
#define MC_WR_GRP_LCL__CBIMMED0__SHIFT 0x1c
#define MC_CITF_PERF_MON_CNTL2__CID_MASK 0xff
#define MC_CITF_PERF_MON_CNTL2__CID__SH