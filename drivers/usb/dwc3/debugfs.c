
#define TMON1_RDIR12_DATA__TEMP_Z_DATA_MASK 0xfff
#define TMON1_RDIR12_DATA__TEMP_Z_DATA__SHIFT 0x0
#define TMON1_RDIR13_DATA__TEMP_Z_DATA_MASK 0xfff
#define TMON1_RDIR13_DATA__TEMP_Z_DATA__SHIFT 0x0
#define TMON1_RDIR14_DATA__TEMP_Z_DATA_MASK 0xfff
#define TMON1_RDIR14_DATA__TEMP_Z_DATA__SHIFT 0x0
#define TMON1_RDIR15_DATA__TEMP_Z_DATA_MASK 0xfff
#define TMON1_RDIR15_DATA__TEMP_Z_DATA__SHIFT 0x0
#define TMON1_INT_DATA__TEMP_Z_DATA_MASK 0xfff
#define TMON1_INT_DATA__TEMP_Z_DATA__SHIFT 0x0
#define TMON1_RDIL_PRESENT0__RDIL_PRESENT_7_0_MASK 0xff
#define TMON1_RDIL_PRESENT0__RDIL_PRESENT_7_0__SHIFT 0x0
#define TMON1_RDIL_PRESENT1__RDIL_PRESENT_15_8_MASK 0xff
#define TMON1_RDIL_PRESENT1__RDIL_PRESENT_15_8__SHIFT 0x0
#define TMON1_RDIR_PRESENT0__RDIR_PRESENT_7_0_MASK 0xff
#define TMON1_RDIR_PRESENT0__RDIR_PRESENT_7_0__SHIFT 0x0
#define TMON1_RDIR_PRESENT1__RDIR_PRESENT_15_8_MASK 0xff
#define TMON1_RDIR_PRESENT1__RDIR_PRESENT_15_8__SHIFT 0x0
#define TMON1_CONFIG__NUM_ACQ_MASK 0x7
#define TMON1_CONFIG__NUM_ACQ__SHIFT 0x0
#define TMON1_CONFIG__FORCE_MAX_ACQ_MASK 0x8
#define TMON1_CONFIG__FORCE_MAX_ACQ__SHIFT 0x3
#define TMON1_CONFIG__RDI_INTERLEAVE_MASK 0x10
#define TMON1_CONFIG__RDI_INTERLEAVE__SHIFT 0x4
#define TMON1_CONFIG__RE_CALIB_EN_MASK 0x40
#define TMON1_CONFIG__RE_CALIB_EN__SHIFT 0x6
#define TMON1_TEMP_CALC_COEFF0__Z_MASK 0x7ff
#define TMON1_TEMP_CALC_COEFF0__Z__SHIFT 0x0
#define TMON1_TEMP_CALC_COEFF1__A_MASK 0xfff
#define TMON1_TEMP_CALC_COEFF1__A__SHIFT 0x0
#define TMON1_TEMP_CALC_COEFF2__B_MASK 0x3f
#define TMON1_TEMP_CALC_COEFF2__B__SHIFT 0x0
#define TMON1_TEMP_CALC_COEFF3__C_MASK 0x7ff
#define TMON1_TEMP_CALC_COEFF3__C__SHIFT 0x0
#define TMON1_TEMP_CALC_COEFF4__K_MASK 0x1
#define TMON1_TEMP_CALC_COEFF4__K__SHIFT 0x0
#define TMON1_DEBUG0__DEBUG_Z_MASK 0x7ff
#define TMON1_DEBUG0__DEBUG_Z__SHIFT 0x0
#define TMON1_DEBUG0__DEBUG_Z_EN_MASK 0x800
#define TMON1_DEBUG0__DEBUG_Z_EN__SHIFT 0xb
#define TMON1_DEBUG1__DEBUG_RDI_MASK 0x1f
#define TMON1_DEBUG1__DEBUG_RDI__SHIFT 0x0
#define THM_TMON0_REMOTE_START__DATA_MASK 0xffffffff
#define THM_TMON0_REMOTE_START__DATA__SHIFT 0x0
#define THM_TMON0_REMOTE_END__DATA_MASK 0xffffffff
#define THM_TMON0_REMOTE_END__DATA__SHIFT 0x0
#define THM_TMON1_REMOTE_START__DATA_MASK 0xffffffff
#define THM_TMON1_REMOTE_START__DATA__SHIFT 0x0
#define THM_TMON1_REMOTE_END__DATA_MASK 0xffffffff
#define THM_TMON1_REMOTE_END__DATA__SHIFT 0x0
#define THM_TCON_LOCAL0__HaltPolling_MASK 0x1
#define THM_TCON_LOCAL0__HaltPolling__SHIFT 0x0
#define THM_TCON_LOCAL0__TMON0_PwrDn_Dis_MASK 0x2
#define THM_TCON_LOCAL0__TMON0_PwrDn_Dis__SHIFT 0x1
#define THM_TCON_LOCAL0__TMON1_PwrDn_Dis_MASK 0x4
#define THM_TCON_LOCAL0__TMON1_PwrDn_Dis__SHIFT 0x2
#define THM_TCON_LOCAL1__PwrDn_Limit_Temp_MASK 0x7
#define THM_TCON_LOCAL1__PwrDn_Limit_Temp__SHIFT 0x0
#define THM_TCON_LOCAL1__PwrDn_DelaySlope_MASK 0x38
#define THM_TCON_LOCAL1__PwrDn_DelaySlope__SHIFT 0x3
#define THM_TCON_LOCAL1__PwrDn_MinDelay_MASK 0x1c0
#define THM_TCON_LOCAL1__PwrDn_MinDelay__SHIFT 0x6
#define THM_TCON_LOCAL2__PwrDn_MaxDlyMult_MASK 0x3
#define THM_TCON_LOCAL2__PwrDn_MaxDlyMult__SHIFT 0x0
#define THM_TCON_LOCAL2__PwrDn_NumSensors_MASK 0xc
#define THM_TCON_LOCAL2__PwrDn_NumSensors__SHIFT 0x2
#define THM_TCON_LOCAL2__start_mission_polling_MASK 0x10
#define THM_TCON_LOCAL2__start_mission_polling__SHIFT 0x4
#define THM_TCON_LOCAL2__short_stagger_count_MASK 0x20
#define THM_TCON_LOCAL2__short_stagger_count__SHIFT 0x5
#define THM_TCON_LOCAL2__sbtsi_use_corrected_MASK 0x40
#define THM_TCON_LOCAL2__sbtsi_use_corrected__SHIFT 0x6
#define THM_TCON_LOCAL2__csrslave_use_corrected_MASK 0x80
#define THM_TCON_LOCAL2__csrslave_use_corrected__SHIFT 0x7
#define THM_TCON_LOCAL2__smu_use_corrected_MASK 0x100
#define THM_TCON_LOCAL2__smu_use_corrected__SHIFT 0x8
#define THM_TCON_LOCAL2__skip_scale_correction_MASK 0x800
#define THM_TCON_LOCAL2__skip_scale_correction__SHIFT 0xb
#define THM_TCON_LOCAL3__Global_TMAX_MASK 0x7ff
#define THM_TCON_LOCAL3__Global_TMAX__SHIFT 0x0
#define THM_TCON_LOCAL4__Global_TMAX_ID_MASK 0xff
#define THM_TCON_LOCAL4__Global_TMAX_ID__SHIFT 0x0
#define THM_TCON_LOCAL5__Global_TMIN_MASK 0x7ff
#define THM_TCON_LOCAL5__Global_TMIN__SHIFT 0x0
#define THM_TCON_LOCAL6__Global_TMIN_ID_MASK 0xff
#define THM_TCON_LOCAL6__Global_TMIN_ID__SHIFT 0x0
#define THM_TCON_LOCAL7__THERMID_MASK 0xff
#define THM_TCON_LOCAL7__THERMID__SHIFT 0x0
#define THM_TCON_LOCAL8__THERMMAX_MASK 0x7ff
#define THM_TCON_LOCAL8__THERMMAX__SHIFT 0x0
#define THM_TCON_LOCAL9__Tj_Max_TMON0_MASK 0x7ff
#define THM_TCON_LOCAL9__Tj_Max_TMON0__SHIFT 0x0
#define THM_TCON_LOCAL10__TMON0_Tj_Max_RS_ID_MASK 0xf
#define THM_TCON_LOCAL10__TMON0_Tj_Max_RS_ID__SHIFT 0x0
#define THM_TCON_LOCAL11__Tj_Max_TMON1_MASK 0x7ff
#define THM_TCON_LOCAL11__Tj_Max_TMON1__SHIFT 0x0
#define THM_TCON_LOCAL12__TMON1_Tj_Max_RS_ID_MASK 0xf
#define THM_TCON_LOCAL12__TMON1_Tj_Max_RS_ID__SHIFT 0x0
#define THM_TCON_LOCAL13__PowerDownTmon0_MASK 0x1
#define THM_TCON_LOCAL13__PowerDownTmon0__SHIFT 0x0
#define THM_TCON_LOCAL13__PowerDownTmon1_MASK 0x2
#define THM_TCON_LOCAL13__PowerDownTmon1__SHIFT 0x1
#define THM_TCON_LOCAL14__boot_done_MASK 0x1
#define THM_TCON_LOCAL14__boot_done__SHIFT 0x0
#define THM_FUSE0__FUSE_TmonRsInterleave_MASK 0x1
#define THM_FUSE0__FUSE_TmonRsInterleave__SHIFT 0x0
#define THM_FUSE0__FUSE_TmonNumAcq_MASK 0xe
#define THM_FUSE0__FUSE_TmonNumAcq__SHIFT 0x1
#define THM_FUSE0__FUSE_TmonForceMaxAcq_MASK 0x10
#define THM_FUSE0__FUSE_TmonForceMaxAcq__SHIFT 0x4
#define THM_FUSE0__FUSE_TmonClkDiv_MASK 0x60
#define THM_FUSE0__FUSE_TmonClkDiv__SHIFT 0x5
#define THM_FUSE0__FUSE_TmonBGAdj1_MASK 0x7f80
#define THM_FUSE0__FUSE_TmonBGAdj1__SHIFT 0x7
#define THM_FUSE0__FUSE_TmonBGAdj0_MASK 0x7f8000
#define THM_FUSE0__FUSE_TmonBGAdj0__SHIFT 0xf
#define THM_FUSE0__FUSE_TconZtValue_MASK 0xff800000
#define THM_FUSE0__FUSE_TconZtValue__SHIFT 0x17
#define THM_FUSE1__FUSE_TconZtValue_MASK 0x3
#define THM_FUSE1__FUSE_TconZtValue__SHIFT 0x0
#define THM_FUSE1__FUSE_TconUseSecondary_MASK 0xc
#define THM_FUSE1__FUSE_TconUseSecondary__SHIFT 0x2
#define THM_FUSE1__FUSE_TconTmpAdjLoRes_MASK 0x10
#define THM_FUSE1__FUSE_TconTmpAdjLoRes__SHIFT 0x4
#define THM_FUSE1__FUSE_TconPwrUpStaggerTime_MASK 0x60
#define THM_FUSE1__FUSE_TconPwrUpStaggerTime__SHIFT 0x5
#define THM_FUSE1__FUSE_TconPwrDnTmpLmt_MASK 0x380
#define THM_FUSE1__FUSE_TconPwrDnTmpLmt__SHIFT 0x7
#define THM_FUSE1__FUSE_TconPwrDnNumSensors_MASK 0xc00
#define THM_FUSE1__FUSE_TconPwrDnNumSensors__SHIFT 0xa
#define THM_FUSE1__FUSE_TconPwrDnMinDelay_MASK 0x7000
#define THM_FUSE1__FUSE_TconPwrDnMinDelay__SHIFT 0xc
#define THM_FUSE1__FUSE_TconPwrDnMaxDelayMult_MASK 0x18000
#define THM_FUSE1__FUSE_TconPwrDnMaxDelayMult__SHIFT 0xf
#define THM_FUSE1__FUSE_TconPwrDnDelaySlope_MASK 0xe0000
#define THM_FUSE1__FUSE_TconPwrDnDelaySlope__SHIFT 0x11
#define THM_FUSE1__FUSE_TconKValue_MASK 0x100000
#define THM_FUSE1__FUSE_TconKValue__SHIFT 0x14
#define THM_FUSE1__FUSE_TconDtValue31_MASK 0x7e00000
#define THM_FUSE1__FUSE_TconDtValue31__SHIFT 0x15
#define THM_FUSE1__FUSE_TconDtValue30_MASK 0xf8000000
#define THM_FUSE1__FUSE_TconDtValue30__SHIFT 0x1b
#define THM_FUSE2__FUSE_TconDtValue30_MASK 0x1
#define THM_FUSE2__FUSE_TconDtValue30__SHIFT 0x0
#define THM_FUSE2__FUSE_TconDtValue29_MASK 0x7e
#define THM_FUSE2__FUSE_TconDtValue29__SHIFT 0x1
#define THM_FUSE2__FUSE_TconDtValue28_MASK 0x1f80
#define THM_FUSE2__FUSE_TconDtValue28__SHIFT 0x7
#define THM_FUSE2__FUSE_TconDtValue27_MASK 0x7e000
#define THM_FUSE2__FUSE_TconDtValue27__SHIFT 0xd
#define THM_FUSE2__FUSE_TconDtValue26_MASK 0x1f80000
#define THM_FUSE2__FUSE_TconDtValue26__SHIFT 0x13
#define THM_FUSE2__FUSE_TconDtValue25_MASK 0x7e000000
#define THM_FUSE2__FUSE_TconDtValue25__SHIFT 0x19
#define THM_FUSE2__FUSE_TconDtValue24_MASK 0x80000000
#define THM_FUSE2__FUSE_TconDtValue24__SHIFT 0x1f
#define THM_FUSE3__FUSE_TconDtValue24_MASK 0x1f
#define THM_FUSE3__FUSE_TconDtValue24__SHIFT 0x0
#define THM_FUSE3__FUSE_TconDtValue23_MASK 0x7e0
#define THM_FUSE3__FUSE_TconDtValue23__SHIFT 0x5
#define THM_FUSE3__FUSE_TconDtValue22_MASK 0x1f800
#define THM_FUSE3__FUSE_TconDtValue22__SHIFT 0xb
#define THM_FUSE3__FUSE_TconDtValue21_MASK 0x7e0000
#define THM_FUSE3__FUSE_TconDtValue21__SHIFT 0x11
#define THM_FUSE3__FUSE_TconDtValue20_MASK 0x1f800000
#define THM_FUSE3__FUSE_TconDtValue20__SHIFT 0x17
#define THM_FUSE3__FUSE_TconDtValue19_MASK 0xe0000000
#define THM_FUSE3__FUSE_TconDtValue19__SHIFT 0x1d
#define THM_FUSE4__FUSE_TconDtValue19_MASK 0x7
#define THM_FUSE4__FUSE_TconDtValue19__SHIFT 0x0
#define THM_FUSE4__FUSE_TconDtValue18_MASK 0x1f8
#define THM_FUSE4__FUSE_TconDtValue18__SHIFT 0x3
#define THM_FUSE4__FUSE_TconDtValue17_MASK 0x7e00
#define THM_FUSE4__FUSE_TconDtValue17__SHIFT 0x9
#define THM_FUSE4__FUSE_TconDtValue16_MASK 0x1f8000
#define THM_FUSE4__FUSE_TconDtValue16__SHIFT 0xf
#define THM_FUSE4__FUSE_TconDtValue15_MASK 0x7e00000
#define THM_FUSE4__FUSE_TconDtValue15__SHIFT 0x15
#define THM_FUSE4__FUSE_TconDtValue14_MASK 0xf8000000
#define THM_FUSE4__FUSE_TconDtValue14__SHIFT 0x1b
#define THM_FUSE5__FUSE_TconDtValue14_MASK 0x1
#define THM_FUSE5__FUSE_TconDtValue14__SHIFT 0x0
#define THM_FUSE5__FUSE_TconDtValue13_MASK 0x7e
#define THM_FUSE5__FUSE_TconDtValue13__SHIFT 0x1
#define THM_FUSE5__FUSE_TconDtValue12_MASK 0x1f80
#define THM_FUSE5__FUSE_TconDtValue12__SHIFT 0x7
#define THM_FUSE5__FUSE_TconDtValue11_MASK 0x7e000
#define THM_FUSE5__FUSE_TconDtValue11__SHIFT 0xd
#define THM_FUSE5__FUSE_TconDtValue10_MASK 0x1f80000
#define THM_FUSE5__FUSE_TconDtValue10__SHIFT 0x13
#define THM_FUSE5__FUSE_TconDtValue9_MASK 0x7e000000
#define THM_FUSE5__FUSE_TconDtValue9__SHIFT 0x19
#define THM_FUSE5__FUSE_TconDtValue8_MASK 0x80000000
#define THM_FUSE5__FUSE_TconDtValue8__SHIFT 0x1f
#define THM_FUSE6__FUSE_TconDtValue8_MASK 0x1f
#define THM_FUSE6__FUSE_TconDtValue8__SHIFT 0x0
#define THM_FUSE6__FUSE_TconDtValue7_MASK 0x7e0
#define THM_FUSE6__FUSE_TconDtValue7__SHIFT 0x5
#define THM_FUSE6__FUSE_TconDtValue6_MASK 0x1f800
#define THM_FUSE6__FUSE_TconDtValue6__SHIFT 0xb
#define THM_FUSE6__FUSE_TconDtValue5_MASK 0x7e0000
#define THM_FUSE6__FUSE_TconDtValue5__SHIFT 0x11
#define THM_FUSE6__FUSE_TconDtValue4_MASK 0x1f800000
#define THM_FUSE6__FUSE_TconDtValue4__SHIFT 0x17
#define THM_FUSE6__FUSE_TconDtValue3_MASK 0xe0000000
#define THM_FUSE6__FUSE_TconDtValue3__SHIFT 0x1d
#define THM_FUSE7__FUSE_TconDtValue3_MASK 0x7
#define THM_FUSE7__FUSE_TconDtValue3__SHIFT 0x0
#define THM_FUSE7__FUSE_TconDtValue2_MASK 0x1f8
#define THM_FUSE7__FUSE_TconDtValue2__SHIFT 0x3
#define THM_FUSE7__FUSE_TconDtValue1_MASK 0x7e00
#define THM_FUSE7__FUSE_TconDtValue1__SHIFT 0x9
#define THM_FUSE7__FUSE_TconDtValue0_MASK 0x1f8000
#define THM_FUSE7__FUSE_TconDtValue0__SHIFT 0xf
#define THM_FUSE7__FUSE_TconCtValue1_MASK 0xffe00000
#define THM_FUSE7__FUSE_TconCtValue1__SHIFT 0x15
#define THM_FUSE8__FUSE_TconCtValue0_MASK 0x7ff
#define THM_FUSE8__FUSE_TconCtValue0__SHIFT 0x0
#define THM_FUSE8__FUSE_TconBtValue_MASK 0x1f800
#define THM_FUSE8__FUSE_TconBtValue__SHIFT 0xb
#define THM_FUSE8__FUSE_TconBootDelay_MASK 0x60000
#define THM_FUSE8__FUSE_TconBootDelay__SHIFT 0x11
#define THM_FUSE8__FUSE_TconAtValue1_MASK 0x7ff80000
#define THM_FUSE8__FUSE_TconAtValue1__SHIFT 0x13
#define THM_FUSE8__FUSE_TconAtValue0_MASK 0x80000000
#define THM_FUSE8__FUSE_TconAtValue0__SHIFT 0x1f
#define THM_FUSE9__FUSE_TconAtValue0_MASK 0x7ff
#define THM_FUSE9__FUSE_TconAtValue0__SHIFT 0x0
#define THM_FUSE9__FUSE_ThermTripLimit_MASK 0x7f800
#define THM_FUSE9__FUSE_ThermTripLimit__SHIFT 0xb
#define THM_FUSE9__FUSE_ThermTripEn_MASK 0x80000
#define THM_FUSE9__FUSE_ThermTripEn__SHIFT 0x13
#define THM_FUSE9__FUSE_HtcTmpLmt_MASK 0x7f00000
#define THM_FUSE9__FUSE_HtcTmpLmt__SHIFT 0x14
#define THM_FUSE9__FUSE_HtcMsrLock_MASK 0x8000000
#define THM_FUSE9__FUSE_HtcMsrLock__SHIFT 0x1b
#define THM_FUSE9__FUSE_HtcHystLmt_MASK 0xf0000000
#define THM_FUSE9__FUSE_HtcHystLmt__SHIFT 0x1c
#define THM_FUSE10__FUSE_HtcDis_MASK 0x1
#define THM_FUSE10__FUSE_HtcDis__SHIFT 0x0
#define THM_FUSE10__FUSE_HtcClkInact_MASK 0xe
#define THM_FUSE10__FUSE_HtcClkInact__SHIFT 0x1
#define THM_FUSE10__FUSE_HtcClkAct_MASK 0x70
#define THM_FUSE10__FUSE_HtcClkAct__SHIFT 0x4
#define THM_FUSE10__FUSE_UnusedBits_MASK 0xffffff80
#define THM_FUSE10__FUSE_UnusedBits__SHIFT 0x7
#define THM_FUSE11__PA_SPARE_MASK 0xff
#define THM_FUSE11__PA_SPARE__SHIFT 0x0
#define THM_FUSE12__FusesValid_MASK 0x1
#define THM_FUSE12__FusesValid__SHIFT 0x0
#define MP0PUB_IND_INDEX__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_0__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_0__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_0__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_0__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_1__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_1__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_1__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_1__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_2__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_2__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_2__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_2__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_3__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_3__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_3__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_3__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_4__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_4__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_4__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_4__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_5__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_5__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_5__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_5__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_6__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_6__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_6__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_6__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_7__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_7__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_7__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_7__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_8__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_8__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_8__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_8__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_9__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_9__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_9__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_9__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_10__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_10__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_10__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_10__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_11__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_11__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_11__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_11__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_12__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_12__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_12__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_12__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_13__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_13__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_13__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_13__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_14__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_14__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_14__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_14__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0PUB_IND_INDEX_15__MP0PUB_IND_ADDR_MASK 0xffffffff
#define MP0PUB_IND_INDEX_15__MP0PUB_IND_ADDR__SHIFT 0x0
#define MP0PUB_IND_DATA_15__MP0PUB_IND_DATA_MASK 0xffffffff
#define MP0PUB_IND_DATA_15__MP0PUB_IND_DATA__SHIFT 0x0
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_0_MASK 0x1
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_0__SHIFT 0x0
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_1_MASK 0x2
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_1__SHIFT 0x1
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_2_MASK 0x4
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_2__SHIFT 0x2
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_3_MASK 0x8
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_3__SHIFT 0x3
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_4_MASK 0x10
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_4__SHIFT 0x4
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_5_MASK 0x20
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_5__SHIFT 0x5
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_6_MASK 0x40
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_6__SHIFT 0x6
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_7_MASK 0x80
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_7__SHIFT 0x7
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_8_MASK 0x100
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_8__SHIFT 0x8
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_9_MASK 0x200
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_9__SHIFT 0x9
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_10_MASK 0x400
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_10__SHIFT 0xa
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_11_MASK 0x800
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_11__SHIFT 0xb
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_12_MASK 0x1000
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_12__SHIFT 0xc
#define MP0_IND_ACCESS_CNTL__AUTO_INCREMENT_IND_13_MASK 0x2000
#define MP0_IND_ACCESS_C