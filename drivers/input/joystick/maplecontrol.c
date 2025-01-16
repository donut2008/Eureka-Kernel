_TSM_OCNT__TESTS__SHIFT 0x8
#define MC_SEQ_TSM_OCNT__CMP_VALUE_MASK 0xffff0000
#define MC_SEQ_TSM_OCNT__CMP_VALUE__SHIFT 0x10
#define MC_SEQ_TSM_NCNT__TRUE_ACT_MASK 0xf
#define MC_SEQ_TSM_NCNT__TRUE_ACT__SHIFT 0x0
#define MC_SEQ_TSM_NCNT__FALSE_ACT_MASK 0xf0
#define MC_SEQ_TSM_NCNT__FALSE_ACT__SHIFT 0x4
#define MC_SEQ_TSM_NCNT__TESTS_MASK 0xff00
#define MC_SEQ_TSM_NCNT__TESTS__SHIFT 0x8
#define MC_SEQ_TSM_NCNT__RANGE_LOW_MASK 0xf0000
#define MC_SEQ_TSM_NCNT__RANGE_LOW__SHIFT 0x10
#define MC_SEQ_TSM_NCNT__RANGE_HIGH_MASK 0xf00000
#define MC_SEQ_TSM_NCNT__RANGE_HIGH__SHIFT 0x14
#define MC_SEQ_TSM_NCNT__NIBBLE_SKIP_MASK 0xf000000
#define MC_SEQ_TSM_NCNT__NIBBLE_SKIP__SHIFT 0x18
#define MC_SEQ_TSM_BCNT__TRUE_ACT_MASK 0xf
#define MC_SEQ_TSM_BCNT__TRUE_ACT__SHIFT 0x0
#define MC_SEQ_TSM_BCNT__FALSE_ACT_MASK 0xf0
#define MC_SEQ_TSM_BCNT__FALSE_ACT__SHIFT 0x4
#define MC_SEQ_TSM_BCNT__BCNT_TESTS_MASK 0xff00
#define MC_SEQ_TSM_BCNT__BCNT_TESTS__SHIFT 0x8
#define MC_SEQ_TSM_BCNT__COMP_VALUE_MASK 0xff0000
#define MC_SEQ_TSM_BCNT__COMP_VALUE__SHIFT 0x10
#define MC_SEQ_TSM_BCNT__DONE_TESTS_MASK 0xff000000
#define MC_SEQ_TSM_BCNT__DONE_TESTS__SHIFT 0x18
#define MC_SEQ_TSM_FLAG__TRUE_ACT_MASK 0xf
#define MC_SEQ_TSM_FLAG__TRUE_ACT__SHIFT 0x0
#define MC_SEQ_TSM_FLAG__FALSE_ACT_MASK 0xf0
#define MC_SEQ_TSM_FLAG__FALSE_ACT__SHIFT 0x4
#define MC_SEQ_TSM_FLAG__FLAG_TESTS_MASK 0xff00
#define MC_SEQ_TSM_FLAG__FLAG_TESTS__SHIFT 0x8
#define MC_SEQ_TSM_FLAG__NBBL_MASK_MASK 0xf0000
#define MC_SEQ_TSM_FLAG__NBBL_MASK__SHIFT 0x10
#define MC_SEQ_TSM_FLAG__ERROR_TESTS_MASK 0xff000000
#define MC_SEQ_TSM_FLAG__ERROR_TESTS__SHIFT 0x18
#define MC_SEQ_TSM_UPDATE__TRUE_ACT_MASK 0xf
#define MC_SEQ_TSM_UPDATE__TRUE_ACT__SHIFT 0x0
#define MC_SEQ_TSM_UPDATE__FALSE_ACT_MASK 0xf0
#define MC_SEQ_TSM_UPDATE__FALSE_ACT__SHIFT 0x4
#define MC_SEQ_TSM_UPDATE__UPDT_TESTS_MASK 0xff00
#define MC_SEQ_TSM_UPDATE__UPDT_TESTS__SHIFT 0x8
#define MC_SEQ_TSM_UPDATE__AREF_COUNT_MASK 0xff0000
#define MC_SEQ_TSM_UPDATE__AREF_COUNT__SHIFT 0x10
#define MC_SEQ_TSM_UPDATE__CAPTR_TESTS_MASK 0xff000000
#define MC_SEQ_TSM_UPDATE__CAPTR_TESTS__SHIFT 0x18
#define MC_SEQ_TSM_EDC__EDC_MASK 0xffffffff
#define MC_SEQ_TSM_EDC__EDC__SHIFT 0x0
#define MC_SEQ_TSM_DBI__DBI_MASK 0xffffffff
#define MC_SEQ_TSM_DBI__DBI__SHIFT 0x0
#define MC_SEQ_TSM_WCDR__WCDR_MASK 0xffffffff
#define MC_SEQ_TSM_WCDR__WCDR__SHIFT 0x0
#define MC_SEQ_TSM_MISC__WCDR_PTR_MASK 0xffff
#define MC_SEQ_TSM_MISC__WCDR_PTR__SHIFT 0x0
#define MC_SEQ_TSM_MISC__WCDR_MASK_MASK 0xf0000
#define MC_SEQ_TSM_MISC__WCDR_MASK__SHIFT 0x10
#define MC_SEQ_TSM_MISC__CH1_OFFSET_MASK 0x3f00000
#define MC_SEQ_TSM_MISC__CH1_OFFSET__SHIFT 0x14
#define MC_SEQ_TSM_MISC__CH1_WCDR_OFFSET_MASK 0xfc000000
#define MC_SEQ_TSM_MISC__CH1_WCDR_OFFSET__SHIFT 0x1a
#define MC_SEQ_TIMER_WR__COUNTER_MASK 0xffffffff
#define MC_SEQ_TIMER_WR__COUNTER__SHIFT 0x0
#define MC_SEQ_TIMER_RD__COUNTER_MASK 0xffffffff
#define MC_SEQ_TIMER_RD__COUNTER__SHIFT 0x0
#define MC_SEQ_DRAM_ERROR_INSERTION__TX_MASK 0xffff
#define MC_SEQ_DRAM_ERROR_INSERTION__TX__SHIFT 0x0
#define MC_SEQ_DRAM_ERROR_INSERTION__RX_MASK 0xffff0000
#define MC_SEQ_DRAM_ERROR_INSERTION__RX__SHIFT 0x10
#define MC_PHY_TIMING_D0__RXC0_DLY_MASK 0xf
#define MC_PHY_TIMING_D0__RXC0_DLY__SHIFT 0x0
#define MC_PHY_TIMING_D0__RXC0_EXT_MASK 0xf0
#define MC_PHY_TIMING_D0__RXC0_EXT__SHIFT 0x4
#define MC_PHY_TIMING_D0__RXC1_DLY_MASK 0xf00
#define MC_PHY_TIMING_D0__RXC1_DLY__SHIFT 0x8
#define MC_PHY_TIMING_D0__RXC1_EXT_MASK 0xf000
#define MC_PHY_TIMING_D0__RXC1_EXT__SHIFT 0xc
#define MC_PHY_TIMING_D0__TXC0_DLY_MASK 0x70000
#define MC_PHY_TIMING_D0__TXC0_DLY__SHIFT 0x10
#define MC_PHY_TIMING_D0__TXC0_EXT_MASK 0xf00000
#define MC_PHY_TIMING_D0__TXC0_EXT__SHIFT 0x14
#define MC_PHY_TIMING_D0__TXC1_DLY_MASK 0x7000000
#define MC_PHY_TIMING_D0__TXC1_DLY__SHIFT 0x18
#define MC_PHY_TIMING_D0__TXC1_EXT_MASK 0xf0000000
#define MC_PHY_TIMING_D0__TXC1_EXT__SHIFT 0x1c
#define MC_PHY_TIMING_D1__RXC0_DLY_MASK 0xf
#define MC_PHY_TIMING_D1__RXC0_DLY__SHIFT 0x0
#define MC_PHY_TIMING_D1__RXC0_EXT_MASK 0xf0
#define MC_PHY_TIMING_D1__RXC0_EXT__SHIFT 0x4
#define MC_PHY_TIMING_D1__RXC1_DLY_MASK 0xf00
#define MC_PHY_TIMING_D1__RXC1_DLY__SHIFT 0x8
#define MC_PHY_TIMING_D1__RXC1_EXT_MASK 0xf000
#define MC_PHY_TIMING_D1__RXC1_EXT__SHIFT 0xc
#define MC_PHY_TIMING_D1__TXC0_DLY_MASK 0x70000
#define MC_PHY_TIMING_D1__TXC0_DLY__SHIFT 0x10
#define MC_PHY_TIMING_D1__TXC0_EXT_MASK 0xf00000
#define MC_PHY_TIMING_D1__TXC0_EXT__SHIFT 0x14
#define MC_PHY_TIMING_D1__TXC1_DLY_MASK 0x7000000
#define MC_PHY_TIMING_D1__TXC1_DLY__SHIFT 0x18
#define MC_PHY_TIMING_D1__TXC1_EXT_MASK 0xf0000000
#define MC_PHY_TIMING_D1__TXC1_EXT__SHIFT 0x1c
#define MC_PHY_TIMING_2__IND_LD_CNT_MASK 0x7f
#define MC_PHY_TIMING_2__IND_LD_CNT__SHIFT 0x0
#define MC_PHY_TIMING_2__RXC0_INV_MASK 0x100
#define MC_PHY_TIMING_2__RXC0_INV__SHIFT 0x8
#define MC_PHY_TIMING_2__RXC1_INV_MASK 0x200
#define MC_PHY_TIMING_2__RXC1_INV__SHIFT 0