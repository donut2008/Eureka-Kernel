
#define MC_ARB_RAMCFG__CHANSIZE__SHIFT 0x8
#define MC_ARB_RAMCFG__RSV_1_MASK 0x200
#define MC_ARB_RAMCFG__RSV_1__SHIFT 0x9
#define MC_ARB_RAMCFG__RSV_2_MASK 0x400
#define MC_ARB_RAMCFG__RSV_2__SHIFT 0xa
#define MC_ARB_RAMCFG__RSV_3_MASK 0x800
#define MC_ARB_RAMCFG__RSV_3__SHIFT 0xb
#define MC_ARB_RAMCFG__NOOFGROUPS_MASK 0x1000
#define MC_ARB_RAMCFG__NOOFGROUPS__SHIFT 0xc
#define MC_ARB_RAMCFG__RSV_4_MASK 0x3e000
#define MC_ARB_RAMCFG__RSV_4__SHIFT 0xd
#define MC_ARB_POP__ENABLE_ARB_MASK 0x1
#define MC_ARB_POP__ENABLE_ARB__SHIFT 0x0
#define MC_ARB_POP__SPEC_OPEN_MASK 0x2
#define MC_ARB_POP__SPEC_OPEN__SHIFT 0x1
#define MC_ARB_POP__POP_DEPTH_MASK 0x3c
#define MC_ARB_POP__POP_DEPTH__SHIFT 0x2
#define MC_ARB_POP__WRDATAINDEX_DEPTH_MASK 0xfc0
#define MC_ARB_POP__WRDATAINDEX_DEPTH__SHIFT 0x6
#define MC_ARB_POP__SKID_DEPTH_MASK 0x7000
#define MC_ARB_POP__SKID_DEPTH__SHIFT 0xc
#define MC_ARB_POP__WAIT_AFTER_RFSH_MASK 0x18000
#define MC_ARB_POP__WAIT_AFTER_RFSH__SHIFT 0xf
#define MC_ARB_POP__QUICK_STOP_MASK 0x20000
#define MC_ARB_POP__QUICK_STOP__SHIFT 0x11
#define MC_ARB_POP__ENABLE_TWO_PAGE_MASK 0x40000
#define MC_ARB_POP__ENABLE_TWO_PAGE__SHIFT 0x12
#define MC_ARB_POP__ALLOW_EOB_BY_WRRET_STALL_MASK 0x80000
#define MC_ARB_POP__ALLOW_EOB_BY_WRRET_STALL__SHIFT 0x13
#define MC_ARB_MINCLKS__READ_CLKS_MASK 0xff
#define MC_ARB_MINCLKS__READ_CLKS__SHIFT 0x0
#define MC_ARB_MINCLKS__WRITE_CLKS_MASK 0xff00
#define MC_ARB_MINCLKS__WRITE_CLKS__SHIFT 0x8
#define MC_ARB_MINCLKS__ARB_RW_SWITCH_MASK 0x10000
#define MC_ARB_MINCLKS__ARB_RW_SWITCH__SHIFT 0x10
#define MC_ARB_MINCLKS__RW_SWITCH_HARSH_MASK 0x60000
#define MC_ARB_MINCLKS__RW_SWITCH_HARSH__SHIFT 0x11
#define MC_ARB_SQM_CNTL__MIN_PENAL_MASK 0xff
#define MC_ARB_SQM_CNTL__MIN_PENAL__SHIFT 0x0
#define MC_ARB_SQM_CNTL__DYN_SQM_ENABLE_MASK 0x100
#define MC_ARB_SQM_CNTL__DYN_SQM_ENABLE__SHIFT 0x8
#define MC_ARB_SQM_CNTL__SQM_RDY16_MASK 0x200
#define MC_ARB_SQM_CNTL__SQM_RDY16__SHIFT 0x9
#define MC_ARB_SQM_CNTL__SQM_RESERVE_MASK 0xfc00
#define MC_ARB_SQM_CNTL__SQM_RESERVE__SHIFT 0xa
#define MC_ARB_SQM_CNTL__RATIO_MASK 0xff0000
#define MC_ARB_SQM_CNTL__RATIO__SHIFT 0x10
#define MC_ARB_SQM_CNTL__RATIO_DEBUG_MASK 0xff000000
#define MC_ARB_SQM_CNTL__RATIO_DEBUG__SHIFT 0x18
#define MC_ARB_ADDR_HASH__BANK_XOR_ENABLE_MASK 0xf
#define MC_ARB_ADDR_HASH__BANK_XOR_ENABLE__SHIFT 0x0
#define MC_ARB_ADDR_HASH__COL_XOR_MASK 0xff0
#define MC_ARB_ADDR_HASH__COL_XOR__SHIFT 0x4
#define MC_ARB_ADDR_HASH__ROW_XOR_MASK 0xffff000
#define MC_ARB_ADDR_HASH__ROW_XOR__SHIFT 0xc
#define MC_ARB_DRAM_TIMING__ACTRD_MASK 0xff
#define MC_ARB_DRAM_TIMING__ACTRD__SHIFT 0x0
#define MC_ARB_DRAM_TIMING__ACTWR_MASK 0xff00
#define MC_ARB_DRAM_TIMING__ACTWR__SHIFT 0x8
#define MC_ARB_DRAM_TIMING__RASMACTRD_MASK 0xff0000
#define MC_ARB_DRAM_TIMING__RASMACTRD__SHIFT 0x10
#define MC_ARB_DRAM_TIMING__RASMACTWR_MASK 0xff000000
#define MC_ARB_DRAM_TIMING__RASMACTWR__SHIFT 0x18
#define MC_ARB_DRAM_TIMING2__RAS2RAS_MASK 0xff
#define MC_ARB_DRAM_TIMING2__RAS2RAS__SHIFT 0x0
#define MC_ARB_DRAM_TIMING2__RP_MASK 0xff00
#define MC_ARB_DRAM_TIMING2__RP__SHIFT 0x8
#define MC_ARB_DRAM_TIMING2__WRPLUSRP_MASK 0xff0000
#define MC_ARB_DRAM_TIMING2__WRPLUSRP__SHIFT 0x10
#define MC_ARB_DRAM_TIMING2__BUS_TURN_MASK 0x1f000000
#define MC_ARB_DRAM_TIMING2__BUS_TURN__SHIFT 0x18
#define MC_ARB_WTM_CNTL_RD__WTMODE_MASK 0x3
#define MC_ARB_WTM_CNTL_RD__WTMODE__SHIFT 0x0
#define MC_ARB_WTM_CNTL_RD__HARSH_PRI_MASK 0x4
#define MC_ARB_WTM_CNTL_RD__HARSH_PRI__SHIFT 0x2
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP0_MASK 0x8
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP0__SHIFT 0x3
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP1_MASK 0x10
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP1__SHIFT 0x4
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP2_MASK 0x20
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP2__SHIFT 0x5
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP3_MASK 0x40
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP3__SHIFT 0x6
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP4_MASK 0x80
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP4__SHIFT 0x7
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP5_MASK 0x100
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP5__SHIFT 0x8
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP6_MASK 0x200
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP6__SHIFT 0x9
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP7_MASK 0x400
#define MC_ARB_WTM_CNTL_RD__ALLOW_STUTTER_GRP7__SHIFT 0xa
#define MC_ARB_WTM_CNTL_RD__ACP_HARSH_PRI_MASK 0x800
#define MC_ARB_WTM_CNTL_RD__ACP_HARSH_PRI__SHIFT 0xb
#define MC_ARB_WTM_CNTL_RD__ACP_OVER_DISP_MASK 0x1000
#define MC_ARB_WTM_CNTL_RD__ACP_OVER_DISP__SHIFT 0xc
#define MC_ARB_WTM_CNTL_RD__FORCE_ACP_URG_MASK 0x2000
#define MC_ARB_WTM_CNTL_RD__FORCE_ACP_URG__SHIFT 0xd
#define MC_ARB_WTM_CNTL_WR__WTMODE_MASK 0x3
#define MC_ARB_WTM_CNTL_WR__WTMODE__SHIFT 0x0
#define MC_ARB_WTM_CNTL_WR__HARSH_PRI_MASK 0x4
#define MC_ARB_WTM_CNTL_WR__HARSH_PRI__SHIFT 0x2
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP0_MASK 0x8
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP0__SHIFT 0x3
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP1_MASK 0x10
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP1__SHIFT 0x4
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP2_MASK 0x20
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP2__SHIFT 0x5
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP3_MASK 0x40
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP3__SHIFT 0x6
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP4_MASK 0x80
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP4__SHIFT 0x7
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP5_MASK 0x100
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP5__SHIFT 0x8
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP6_MASK 0x200
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP6__SHIFT 0x9
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP7_MASK 0x400
#define MC_ARB_WTM_CNTL_WR__ALLOW_STUTTER_GRP7__SHIFT 0xa
#define MC_ARB_WTM_CNTL_WR__ACP_HARSH_PRI_MASK 0x800
#define MC_ARB_WTM_CNTL_WR__ACP_HARSH_PRI__SHIFT 0xb
#define MC_ARB_WTM_CNTL_WR__ACP_OVER_DISP_MASK 0x1000
#define MC_ARB_WTM_CNTL_WR__ACP_OVER_DISP__SHIFT 0xc
#define MC_ARB_WTM_CNTL_WR__FORCE_ACP_URG_MASK 0x2000
#define MC_ARB_WTM_CNTL_WR__FORCE_ACP_URG__SHIFT 0xd
#define MC_ARB_WTM_GRPWT_RD__GRP0_MASK 0x3
#define MC_ARB_WTM_GRPWT_RD__GRP0__SHIFT 0x0
#define MC_ARB_WTM_GRPWT_RD__GRP1_MASK 0xc
#define MC_ARB_WTM_GRPWT_RD__GRP1__SHIFT 0x2
#define MC_ARB_WTM_GRPWT_RD__GRP2_MASK 0x30
#define MC_ARB_WTM_GRPWT_RD__GRP2__SHIFT 0x4
#define MC_ARB_WTM_GRPWT_RD__GRP3_MASK 0xc0
#define MC_ARB_WTM_GRPWT_RD__GRP3__SHIFT 0x6
#define MC_ARB_WTM_GRPWT_RD__GRP4_MASK 0x300
#define MC_ARB_WTM_GRPWT_RD__GRP4__SHIFT 0x8
#define MC_ARB_WTM_GRPWT_RD__GRP5_MASK 0xc00
#define MC_ARB_WTM_GRPWT_RD__GRP5__SHIFT 0xa
#define MC_ARB_WTM_GRPWT_RD__GRP6_MASK 0x3000
#define MC_ARB_WTM_GRPWT_RD__GRP6__SHIFT 0xc
#define MC_ARB_WTM_GRPWT_RD__GRP7_MASK 0xc000
#define MC_ARB_WTM_GRPWT_RD__GRP7__SHIFT 0xe
#define MC_ARB_WTM_GRPWT_RD__GRP_EXT_MASK 0xff0000
#define MC_ARB_WTM_GRPWT_RD__GRP_EXT__SHIFT 0x10
#define MC_ARB_WTM_GRPWT_WR__GRP0_MASK 0x3
#define MC_ARB_WTM_GRPWT_WR__GRP0__SHIFT 0x0
#define MC_ARB_WTM_GRPWT_WR__GRP1_MASK 0xc
#define MC_ARB_WTM_GRPWT_WR__GRP1__SHIFT 0x2
#define MC_ARB_WTM_GRPWT_WR__GRP2_MASK 0x30
#define MC_ARB_WTM_GRPWT_WR__GRP2__SHIFT 0x4
#define MC_ARB_WTM_GRPWT_WR__GRP3_MASK 0xc0
#define MC_ARB_WTM_GRPWT_WR__GRP3__SHIFT 0x6
#define MC_ARB_WTM_GRPWT_WR__GRP4_MASK 0x300
#define MC_ARB_WTM_GRPWT_WR__GRP4__SHIFT 0x8
#define MC_ARB_WTM_GRPWT_WR__GRP5_MASK 0xc00
#define MC_ARB_WTM_GRPWT_WR__GRP5__SHIFT 0xa
#define MC_ARB_WTM_GRPWT_WR__GRP6_MASK 0x3000
#define MC_ARB_WTM_GRPWT_WR__GRP6__SHIFT 0xc
#define MC_ARB_WTM_GRPWT_WR__GRP7_MASK 0xc000
#define MC_ARB_WTM_GRPWT_WR__GRP7__SHIFT 0xe
#define MC_ARB_WTM_GRPWT_WR__GRP_EXT_MASK 0xff0000
#define MC_ARB_WTM_GRPWT_WR__GRP_EXT__SHIFT 0x10
#define MC_ARB_TM_CNTL_RD__GROUPBY_RANK_MASK 0x1
#define MC_ARB_TM_CNTL_RD__GROUPBY_RANK__SHIFT 0x0
#define MC_ARB_TM_CNTL_RD__BANK_SELECT_MASK 0x6
#define MC_ARB_TM_CNTL_RD__BANK_SELECT__SHIFT 0x1
#define MC_ARB_TM_CNTL_RD__MATCH_RANK_MASK 0x8
#define MC_ARB_TM_CNTL_RD__MATCH_RANK__SHIFT 0x3
#define MC_ARB_TM_CNTL_RD__MATCH_BANK_MASK 0x10
#define MC_ARB_TM_CNTL_RD__MATCH_BANK__SHIFT 0x4
#define MC_ARB_TM_CNTL_WR__GROUPBY_RANK_MASK 0x1
#define MC_ARB_TM_CNTL_WR__GROUPBY_RANK__SHIFT 0x0
#define MC_ARB_TM_CNTL_WR__BANK_SELECT_MASK 0x6
#define MC_ARB_TM_CNTL_WR__BANK_SELECT__SHIFT 0x1
#define MC_ARB_TM_CNTL_WR__MATCH_RANK_MASK 0x8
#define MC_ARB_TM_CNTL_WR__MATCH_RANK__SHIFT 0x3
#define MC_ARB_TM_CNTL_WR__MATCH_BANK_MASK 0x10
#d