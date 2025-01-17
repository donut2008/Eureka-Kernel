OUT_EN_MASK 0x10
#define DIG_TEST_PATTERN__DIG_RANDOM_PATTERN_OUT_EN__SHIFT 0x4
#define DIG_TEST_PATTERN__DIG_RANDOM_PATTERN_RESET_MASK 0x20
#define DIG_TEST_PATTERN__DIG_RANDOM_PATTERN_RESET__SHIFT 0x5
#define DIG_TEST_PATTERN__DIG_TEST_PATTERN_EXTERNAL_RESET_EN_MASK 0x40
#define DIG_TEST_PATTERN__DIG_TEST_PATTERN_EXTERNAL_RESET_EN__SHIFT 0x6
#define DIG_TEST_PATTERN__LVDS_EYE_PATTERN_MASK 0x100
#define DIG_TEST_PATTERN__LVDS_EYE_PATTERN__SHIFT 0x8
#define DIG_TEST_PATTERN__DIG_STATIC_TEST_PATTERN_MASK 0x3ff0000
#define DIG_TEST_PATTERN__DIG_STATIC_TEST_PATTERN__SHIFT 0x10
#define DIG_RANDOM_PATTERN_SEED__DIG_RANDOM_PATTERN_SEED_MASK 0xffffff
#define DIG_RANDOM_PATTERN_SEED__DIG_RANDOM_PATTERN_SEED__SHIFT 0x0
#define DIG_RANDOM_PATTERN_SEED__DIG_RAN_PAT_DURING_DE_ONLY_MASK 0x1000000
#define DIG_RANDOM_PATTERN_SEED__DIG_RAN_PAT_DURING_DE_ONLY__SHIFT 0x18
#define DIG_FIFO_STATUS__DIG_FIFO_LEVEL_ERROR_MASK 0x1
#define DIG_FIFO_STATUS__DIG_FIFO_LEVEL_ERROR__SHIFT 0x0
#define DIG_FIFO_STATUS__DIG_FIFO_USE_OVERWRITE_LEVEL_MASK 0x2
#define DIG_FIFO_STATUS__DIG_FIFO_USE_OVERWRITE_LEVEL__SHIFT 0x1
#define DIG_FIFO_STATUS__DIG_FIFO_OVERWRITE_LEVEL_MASK 0xfc
#define DIG_FIFO_STATUS__DIG_FIFO_OVERWRITE_LEVEL__SHIFT 0x2
#define DIG_FIFO_STATUS__DIG_FIFO_ERROR_ACK_MASK 0x100
#define DIG_FIFO_STATUS__DIG_FIFO_ERROR_ACK__SHIFT 0x8
#define DIG_FIFO_STATUS__DIG_FIFO_CAL_AVERAGE_LEVEL_MASK 0xfc00
#define DIG_FIFO_STATUS__DIG_FIFO_CAL_AVERAGE_LEVEL__SHIFT 0xa
#define DIG_FIFO_STATUS__DIG_FIFO_MAXIMUM_LEVEL_MASK 0x1f0000
#define DIG_FIFO_STATUS__DIG_FIFO_MAXIMUM_LEVEL__SHIFT 0x10
#define DIG_FIFO_STATUS__DIG_FIFO_MINIMUM_LEVEL_MASK 0x3c00000
#define DIG_FIFO_STATUS__DIG_FIFO_MINIMUM_LEVEL__SHIFT 0x16
#define DIG_FIFO_STATUS__DIG_FIFO_CALIBRATED_MASK 0x20000000
#define DIG_FIFO_STATUS__DIG_FIFO_CALIBRATED__SHIFT 0x1d
#define DIG_FIFO_STATUS__DIG_FIFO_FORCE_RECAL_AVERAGE_MASK 0x40000000
#define DIG_FIFO_STATUS__DIG_FIFO_FORCE_RECAL_AVERAGE__SHIFT 0x1e
#define DIG_FIFO_STATUS__DIG_FIFO_FORCE_RECOMP_MINMAX_MASK 0x80000000
#define DIG_FIFO_STATUS__DIG_FIFO_FORCE_RECOMP_MINMAX__SHIFT 0x1f
#define DIG_DISPCLK_SWITCH_CNTL__DIG_DISPCLK_SWITCH_POINT_MASK 0x1
#define DIG_DISPCLK_SWITCH_CNTL__DIG_DISPCLK_SWITCH_POINT__SHIFT 0x0
#define DIG_DISPCLK_SWITCH_STATUS__DIG_DISPCLK_SWITCH_ALLOWED_MASK 0x1
#define DIG_DISPCLK_SWITCH_STATUS__DIG_DISPCLK_SWITCH_ALLOWED__SHIFT 0x0
#define DIG_DISPCLK_SWITCH_STATUS__DIG_DISPCLK_SWITCH_ALLOWED_INT_MASK 0x10
#define DIG_DISPCLK_SWITCH_STATUS__DIG_DISPCLK_SWITCH_ALLOWED_INT__SHIFT 0x4
#define DIG_DISPCLK_SWITCH_STATUS__DIG_DISPCLK_SWITCH_ALLOWED_INT_ACK_MASK 0x100
#define DIG_DISPCLK_SWITCH_STATUS__DIG_DISPCLK_SWITCH_ALLOWED_INT_ACK__SHIFT 0x8
#define DIG_DISPCLK_SWITCH_STATUS__DIG_DISPCLK_SWITCH_ALLOWED_INT_MASK_MASK 0x1000
#define DIG_DISPCLK_SWITCH_STATUS__DIG_DISPCLK_SWITCH_ALLOWED_INT_MASK__SHIFT 0xc
#define HDMI_CONTROL__HDMI_KEEPOUT_MODE_MASK 0x1
#define HDMI_CONTROL__HDMI_KEEPOUT_MODE__SHIFT 0x0
#define HDMI_CONTROL__HDMI_CLOCK_CHANNEL_RATE_MASK 0x4
#define HDMI_CONTROL__HDMI_CLOCK_CHANNEL_RATE__SHIFT 0x2
#define HDMI_CONTROL__HDMI_NO_EXTRA_NULL_PACKET_FILLED_MASK 0x8
#define HDMI_CONTROL__HDMI_NO_EXTRA_NULL_PACKET_FILLED__SHIFT 0x3
#define HDMI_CONTROL__HDMI_PACKET_GEN_VERSION_MASK 0x10
#define HDMI_CONTROL__HDMI_PACKET_GEN_VERSION__SHIFT 0x4
#define HDMI_CONTROL__HDMI_ERROR_ACK_MASK 0x100
#define HDMI_CONTROL__HDMI_ERROR_ACK__SHIFT 0x8
#define HDMI_CONTROL__HDMI_ERROR_MASK_MASK 0x200
#define HDMI_CONTROL__HDMI_ERROR_MASK__SHIFT 0x9
#define HDMI_CONTROL__HDMI_DEEP_COLOR_ENABLE_MASK 0x1000000
#define HDMI_CONTROL__HDMI_DEEP_COLOR_ENABLE__SHIFT 0x18
#define HDMI_CONTROL__HDMI_DEEP_COLOR_DEPTH_MASK 0x30000000
#define HDMI_CONTROL__HDMI_DEEP_COLOR_DEPTH__SHIFT 0x1c
#define HDMI_STATUS__HDMI_ACTIVE_AVMUTE_MASK 0x1
#define HDMI_STATUS__HDMI_ACTIVE_AVMUTE__SHIFT 0x0
#define HDMI_STATUS__HDMI_AUDIO_PACKET_ERROR_MASK 0x10000
#define HDMI_STATUS__HDMI_AUDIO_PACKET_ERROR__SHIFT 0x10
#define HDMI_STATUS__HDMI_VBI_PACKET_ERROR_MASK 0x100000
#define HDMI_STATUS__HDMI_VBI_PACKET_ERROR__SHIFT 0x14
#define HDMI_STATUS__HDMI_ERROR_INT_MASK 0x8000000
#define HDMI_STATUS__HDMI_ERROR_INT__SHIFT 0x1b
#define HDMI_AUDIO_PACKET_CONTROL__HDMI_AUDIO_DELAY_EN_MASK 0x30
#define HDMI_AUDIO_PACKET_CONTROL__HDMI_AUDIO_DELAY_EN__SHIFT 0x4
#define HDMI_AUDIO_PACKET_CONTROL__HDMI_AUDIO_SEND_MAX_PACKETS_MASK 0x100
#define HDMI_AUDIO_PACKET_CONTROL__HDMI_AUDIO_SEND_MAX_PACKETS__SHIFT 0x8
#define HDMI_AUDIO_PACKET_CONTROL__HDMI_AUDIO_PACKETS_PER_LINE_MASK 0x1f0000
#define HDMI_AUDIO_PACKET_CONTROL__HDMI_AUDIO_PACKETS_PER_LINE__SHIFT 0x10
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_SEND_MASK 0x1
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_SEND__SHIFT 0x0
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_CONT_MASK 0x2
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_CONT__SHIFT 0x1
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_SELECT_MASK 0x30
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_SELECT__SHIFT 0x4
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_SOURCE_MASK 0x100
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_SOURCE__SHIFT 0x8
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_AUTO_SEND_MASK 0x1000
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_AUTO_SEND__SHIFT 0xc
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_N_MULTIPLE_MASK 0x70000
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_N_MULTIPLE__SHIFT 0x10
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_AUDIO_PRIORITY_MASK 0x80000000
#define HDMI_ACR_PACKET_CONTROL__HDMI_ACR_AUDIO_PRIORITY__SHIFT 0x1f
#define HDMI_VBI_PACKET_CONTROL__HDMI_NULL_SEND_MASK 0x1
#define HDMI_VBI_PACKET_CONTROL__HDMI_NULL_SEND__SHIFT 0x0
#define HDMI_VBI_PACKET_CONTROL__HDMI_GC_SEND_MASK 0x10
#define HDMI_VBI_PACKET_CONTROL__HDMI_GC_SEND__SHIFT 0x4
#define HDMI_VBI_PACKET_CONTROL__HDMI_GC_CONT_MASK 0x20
#define HDMI_VBI_PACKET_CONTROL__HDMI_GC_CONT__SHIFT 0x5
#define HDMI_VBI_PACKET_CONTROL__HDMI_ISRC_SEND_MASK 0x100
#define HDMI_VBI_PACKET_CONTROL__HDMI_ISRC_SEND__SHIFT 0x8
#define HDMI_VBI_PACKET_CONTROL__HDMI_ISRC_CONT_MASK 0x200
#define HDMI_VBI_PACKET_CONTROL__HDMI_ISRC_CONT__SHIFT 0x9
#define HDMI_VBI_PACKET_CONTROL__HDMI_ISRC_LINE_MASK 0x3f0000
#define HDMI_VBI_PACKET_CONTROL__HDMI_ISRC_LINE__SHIFT 0x10
#define HDMI_INFOFRAME_CONTROL0__HDMI_AVI_INFO_SEND_MASK 0x1
#define HDMI_INFOFRAME_CONTROL0__HDMI_AVI_INFO_SEND__SHIFT 0x0
#define HDMI_INFOFRAME_CONTROL0__HDMI_AVI_INFO_CONT_MASK 0x2
#define HDMI_INFOFRAME_CONTROL0__HDMI_AVI_INFO_CONT__SHIFT 0x1
#define HDMI_INFOFRAME_CONTROL0__HDMI_AUDIO_INFO_SEND_MASK 0x10
#define HDMI_INFOFRAME_CONTROL0__HDMI_AUDIO_INFO_SEND__SHIFT 0x4
#define HDMI_INFOFRAME_CONTROL0__HDMI_AUDIO_INFO_CONT_MASK 0x20
#define HDMI_INFOFRAME_CONTROL0__HDMI_AUDIO_INFO_CONT__SHIFT 0x5
#define HDMI_INFOFRAME_CONTROL0__HDMI_MPEG_INFO_SEND_MASK 0x100
#define HDMI_INFOFRAME_CONTROL0__HDMI_MPEG_INFO_SEND__SHIFT 0x8
#define HDMI_INFOFRAME_CONTROL0__HDMI_MPEG_INFO_CONT_MASK 0x200
#define HDMI_INFOFRAME_CONTROL0__HDMI_MPEG_INFO_CONT__SHIFT 0x9
#define HDMI_INFOFRAME_CONTROL1__HDMI_AVI_INFO_LINE_MASK 0x3f
#define HDMI_INFOFRAME_CONTROL1__HDMI_AVI_INFO_LINE__SHIFT 0x0
#define HDMI_INFOFRAME_CONTROL1__HDMI_AUDIO_INFO_LINE_MASK 0x3f00
#define HDMI_INFOFRAME_CONTROL1__HDMI_AUDIO_INFO_LINE__SHIFT 0x8
#define HDMI_INFOFRAME_CONTROL1__HDMI_MPEG_INFO_LINE_MASK 0x3f0000
#define HDMI_INFOFRAME_CONTROL1__HDMI_MPEG_INFO_LINE__SHIFT 0x10
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC0_SEND_MASK 0x1
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC0_SEND__SHIFT 0x0
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC0_CONT_MASK 0x2
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC0_CONT__SHIFT 0x1
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC1_SEND_MASK 0x10
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC1_SEND__SHIFT 0x4
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC1_CONT_MASK 0x20
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC1_CONT__SHIFT 0x5
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC0_LINE_MASK 0x3f0000
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC0_LINE__SHIFT 0x10
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC1_LINE_MASK 0x3f000000
#define HDMI_GENERIC_PACKET_CONTROL0__HDMI_GENERIC1_LINE__SHIFT 0x18
#define HDMI_GC__HDMI_GC_AVMUTE_MASK 0x1
#define HDMI_GC__HDMI_GC_AVMUTE__SHIFT 0x0
#define HDMI_GC__HDMI_GC_AVMUTE_CONT_MASK 0x4
#define HDMI_GC__HDMI_GC_AVMUTE_CONT__SHIFT 0x2
#define HDMI_GC__HDMI_DEFAULT_PHASE_MASK 0x10
#define HDMI_GC__HDMI_DEFAULT_PHASE__SHIFT 0x4
#define HDMI_GC__HDMI_PACKING_PHASE_MASK 0xf00
#define HDMI_GC__HDMI_PACKING_PHASE__SHIFT 0x8
#define HDMI_GC__HDMI_PACKING_PHASE_OVERRIDE_MASK 0x1000
#define HDMI_GC__HDMI_PACKING_PHASE_OVERRIDE__SHIFT 0xc
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_AUDIO_LAYOUT_OVRD_MASK 0x1
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_AUDIO_LAYOUT_OVRD__SHIFT 0x0
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_AUDIO_LAYOUT_SELECT_MASK 0x2
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_AUDIO_LAYOUT_SELECT__SHIFT 0x1
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_AUDIO_CHANNEL_ENABLE_MASK 0xff00
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_AUDIO_CHANNEL_ENABLE__SHIFT 0x8
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_DP_AUDIO_STREAM_ID_MASK 0xff0000
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_DP_AUDIO_STREAM_ID__SHIFT 0x10
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_HBR_ENABLE_OVRD_MASK 0x1000000
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_HBR_ENABLE_OVRD__SHIFT 0x18
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_60958_OSF_OVRD_MASK 0x10000000
#define AFMT_AUDIO_PACKET_CONTROL2__AFMT_60958_OSF_OVRD__SHIFT 0x1c
#define AFMT_ISRC1_0__AFMT_ISRC_STATUS_MASK 0x7
#define AFMT_ISRC1_0__AFMT_ISRC_STATUS__SHIFT 0x0
#define AFMT_ISRC1_0__AFMT_ISRC_CONTINUE_MASK 0x40
#define AFMT_ISRC1_0__AFMT_ISRC_CONTINUE__SHIFT 0x6
#define AFMT_ISRC1_0__AFMT_ISRC_VALID_MASK 0x80
#define AFMT_ISRC1_0__AFMT_ISRC_VALID__SHIFT 0x7
#define AFMT_ISRC1_1__AFMT_UPC_EAN_ISRC0_MASK 0xff
#define AFMT_ISRC1_1__AFMT_UPC_EAN_ISRC0__SHIFT 0x0
#define AFMT_ISRC1_1__AFMT_UPC_EAN_ISRC1_MASK 0xff00
#define AFMT_ISRC1_1__AFMT_UPC_EAN_ISRC1__SHIFT 0x8
#define AFMT_ISRC1_1__AFMT_UPC_EAN_ISRC2_MASK 0xff0000
#define AFMT_ISRC1_1__AFMT_UPC_EAN_ISRC2__SHIFT 0x10
#define AFMT_ISRC1_1__AFMT_UPC_EAN_ISRC3_MASK 0xff000000
#define AFMT_ISRC1_1__AFMT_UPC_EAN_ISRC3__SHIFT 0x18
#define AFMT_ISRC1_2__AFMT_UPC_EAN_ISRC4_MASK 0xff
#define AFMT_ISRC1_2__AFMT_UPC_EAN_ISRC4__SHIFT 0x0
#define AFMT_ISRC1_2__AFMT_UPC_EAN_ISRC5_MASK 0xff00
#define AFMT_ISRC1_2__AFMT_UPC_EAN_ISRC5__SHIFT 0x8
#define AFMT_ISRC1_2__AFMT_UPC_EAN_ISRC6_MASK 0xff0000
#define AFMT_ISRC1_2__AFMT_UPC_EAN_ISRC6__SHIFT 0x10
#define AFMT_ISRC1_2__AFMT_UPC_EAN_ISRC7_MASK 0xff000000
#define AFMT_ISRC1_2__AFMT_UPC_EAN_ISRC7__SHIFT 0x18
#define AFMT_ISRC1_3__AFMT_UPC_EAN_ISRC8_MASK 0xff
#define AFMT_ISRC1_3__AFMT_UPC_EAN_ISRC8__SHIFT 0x0
#define AFMT_ISRC1_3__AFMT_UPC_EAN_ISRC9_MASK 0xff00
#define AFMT_ISRC1_3__AFMT_UPC_EAN_ISRC9__SHIFT 0x8
#define AFMT_ISRC1_3__AFMT_UPC_EAN_ISRC10_MASK 0xff0000
#define AFMT_ISRC1_3__AFMT_UPC_EAN_ISRC10__SHIFT 0x10
#define AFMT_ISRC1_3__AFMT_UPC_EAN_ISRC11_MASK 0xff000000
#define AFMT_ISRC1_3__AFMT_UPC_EAN_ISRC11__SHIFT 0x18
#define AFMT_ISRC1_4__AFMT_UPC_EAN_ISRC12_MASK 0xff
#define AFMT_ISRC1_4__AFMT_UPC_EAN_ISRC12__SHIFT 0x0
#define AFMT_ISRC1_4__AFMT_UPC_EAN_ISRC13_MASK 0xff00
#define AFMT_ISRC1_4__AFMT_UPC_EAN_ISRC13__SHIFT 0x8
#define AFMT_ISRC1_4__AFMT_UPC_EAN_ISRC14_MASK 0xff0000
#define AFMT_ISRC1_4__AFMT_UPC_EAN_ISRC14__SHIFT 0x10
#define AFMT_ISRC1_4__AFMT_UPC_EAN_ISRC15_MASK 0xff000000
#define AFMT_ISRC1_4__AFMT_UPC_EAN_ISRC15__SHIFT 0x18
#define AFMT_ISRC2_0__AFMT_UPC_EAN_ISRC16_MASK 0xff
#define AFMT_ISRC2_0__AFMT_UPC_EAN_ISRC16__SHIFT 0x0
#define AFMT_ISRC2_0__AFMT_UPC_EAN_ISRC17_MASK 0xff00
#define AFMT_ISRC2_0__AFMT_UPC_EAN_ISRC17__SHIFT 0x8
#define AFMT_ISRC2_0__AFMT_UPC_EAN_ISRC18_MASK 0xff0000
#define AFMT_ISRC2_0__AFMT_UPC_EAN_ISRC18__SHIFT 0x10
#define AFMT_ISRC2_0__AFMT_UPC_EAN_ISRC19_MASK 0xff000000
#define AFMT_ISRC2_0__AFMT_UPC_EAN_ISRC19__SHIFT 0x18
#define AFMT_ISRC2_1__AFMT_UPC_EAN_ISRC20_MASK 0xff
#define AFMT_ISRC2_1__AFMT_UPC_EAN_ISRC20__SHIFT 0x0
#define AFMT_ISRC2_1__AFMT_UPC_EAN_ISRC21_MASK 0xff00
#define AFMT_ISRC2_1__AFMT_UPC_EAN_ISRC21__SHIFT 0x8
#define AFMT_ISRC2_1__AFMT_UPC_EAN_ISRC22_MASK 0xff0000
#define AFMT_ISRC2_1__AFMT_UPC_EAN_ISRC22__SHIFT 0x10
#define AFMT_ISRC2_1__AFMT_UPC_EAN_ISRC23_MASK 0xff000000
#define AFMT_ISRC2_1__AFMT_UPC_EAN_ISRC23__SHIFT 0x18
#define AFMT_ISRC2_2__AFMT_UPC_EAN_ISRC24_MASK 0xff
#define AFMT_ISRC2_2__AFMT_UPC_EAN_ISRC24__SHIFT 0x0
#define AFMT_ISRC2_2__AFMT_UPC_EAN_ISRC25_MASK 0xff00
#define AFMT_ISRC2_2__AFMT_UPC_EAN_ISRC25__SHIFT 0x8
#define AFMT_ISRC2_2__AFMT_UPC_EAN_ISRC26_MASK 0xff0000
#define AFMT_ISRC2_2__AFMT_UPC_EAN_ISRC26__SHIFT 0x10
#define AFMT_ISRC2_2__AFMT_UPC_EAN_ISRC27_MASK 0xff000000
#define AFMT_ISRC2_2__AFMT_UPC_EAN_ISRC27__SHIFT 0x18
#define AFMT_ISRC2_3__AFMT_UPC_EAN_ISRC28_MASK 0xff
#define AFMT_ISRC2_3__AFMT_UPC_EAN_ISRC28__SHIFT 0x0
#define AFMT_ISRC2_3__AFMT_UPC_EAN_ISRC29_MASK 0xff00
#define AFMT_ISRC2_3__AFMT_UPC_EAN_ISRC29__SHIFT 0x8
#define AFMT_ISRC2_3__AFMT_UPC_EAN_ISRC30_MASK 0xff0000
#define AFMT_ISRC2_3__AFMT_UPC_EAN_ISRC30__SHIFT 0x10
#define AFMT_ISRC2_3__AFMT_UPC_EAN_ISRC31_MASK 0xff000000
#define AFMT_ISRC2_3__AFMT_UPC_EAN_ISRC31__SHIFT 0x18
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_CHECKSUM_MASK 0xff
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_CHECKSUM__SHIFT 0x0
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_S_MASK 0x300
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_S__SHIFT 0x8
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_B_MASK 0xc00
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_B__SHIFT 0xa
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_A_MASK 0x1000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_A__SHIFT 0xc
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_Y_MASK 0x6000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_Y__SHIFT 0xd
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_PB1_RSVD_MASK 0x8000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_PB1_RSVD__SHIFT 0xf
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_R_MASK 0xf0000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_R__SHIFT 0x10
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_M_MASK 0x300000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_M__SHIFT 0x14
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_C_MASK 0xc00000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_C__SHIFT 0x16
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_SC_MASK 0x3000000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_SC__SHIFT 0x18
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_Q_MASK 0xc000000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_Q__SHIFT 0x1a
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_EC_MASK 0x70000000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_EC__SHIFT 0x1c
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_ITC_MASK 0x80000000
#define AFMT_AVI_INFO0__AFMT_AVI_INFO_ITC__SHIFT 0x1f
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_VIC_MASK 0x7f
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_VIC__SHIFT 0x0
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_PB4_RSVD_MASK 0x80
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_PB4_RSVD__SHIFT 0x7
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_PR_MASK 0xf00
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_PR__SHIFT 0x8
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_CN_MASK 0x3000
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_CN__SHIFT 0xc
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_YQ_MASK 0xc000
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_YQ__SHIFT 0xe
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_TOP_MASK 0xffff0000
#define AFMT_AVI_INFO1__AFMT_AVI_INFO_TOP__SHIFT 0x10
#define AFMT_AVI_INFO2__AFMT_AVI_INFO_BOTTOM_MASK 0xffff
#define AFMT_AVI_INFO2__AFMT_AVI_INFO_BOTTOM__SHIFT 0x0
#define AFMT_AVI_INFO2__AFMT_AVI_INFO_LEFT_MASK 0xffff0000
#define AFMT_AVI_INFO2__AFMT_AVI_INFO_LEFT__SHIFT 0x10
#define AFMT_AVI_INFO3__AFMT_AVI_INFO_RIGHT_MASK 0xffff
#define AFMT_AVI_INFO3__AFMT_AVI_INFO_RIGHT__SHIFT 0x0
#define AFMT_AVI_INFO3__AFMT_AVI_INFO_VERSION_MASK 0xff000000
#define AFMT_AVI_INFO3__AFMT_AVI_INFO_VERSION__SHIFT 0x18
#define AFMT_MPEG_INFO0__AFMT_MPEG_INFO_CHECKSUM_MASK 0xff
#define AFMT_MPEG_INFO0__AFMT_MPEG_INFO_CHECKSUM__SHIFT 0x0
#define AFMT_MPEG_INFO0__AFMT_MPEG_INFO_MB0_MASK 0xff00
#define AFMT_MPEG_INFO0__AFMT_MPEG_INFO_MB0__SHIFT 0x8
#define AFMT_MPEG_INFO0__AFMT_MPEG_INFO_MB1_MASK 0xff0000
#define AFMT_MPEG_INFO0__AFMT_MPEG_INFO_MB1__SHIFT 0x10
#define AFMT_MPEG_INFO0__AFMT_MPEG_INFO_MB2_MASK 0xff000000
#define AFMT_MPEG_INFO0__AFMT_MPEG_INFO_MB2__SHIFT 0x18
#define AFMT_MPEG_INFO1__AFMT_MPEG_INFO_MB3_MASK 0xff
#define AFMT_MPEG_INFO1__AFMT_MPEG_INFO_MB3__SHIFT 0x0
#define AFMT_MPEG_INFO1__AFMT_MPEG_INFO_MF_MASK 0x300
#define AFMT_MPEG_INFO1__AFMT_MPEG_INFO_MF__SHIFT 0x8
#define AFMT_MPEG_INFO1__AFMT_MPEG_INFO_FR_MASK 0x1000
#define AFMT_MPEG_INFO1__AFMT_MPEG_INFO_FR__SHIFT 0xc
#define AFMT_GENERIC_HDR__AFMT_GENERIC_HB0_MASK 0xff
#define AFMT_GENERIC_HDR__AFMT_GENERIC_HB0__SHIFT 0x0
#define AFMT_GENERIC_HDR__AFMT_GENERIC_HB1_MASK 0xff00
#define AFMT_GENERIC_HDR__AFMT_GENERIC_HB1__SHIFT 0x8
#define AFMT_GENERIC_HDR__AFMT_GENERIC_HB2_MASK 0xff0000
#define AFMT_GENERIC_HDR__AFMT_GENERIC_HB2__SHIFT 0x10
#define AFMT_GENERIC_HDR__AFMT_GENERIC_HB3_MASK 0xff000000
#define AFMT_GENERIC_HDR__AFMT_GENERIC_HB3__SHIFT 0x18
#define AFMT_GENERIC_0__AFMT_GENERIC_BYTE0_MASK 0xff
#define AFMT_GENERIC_0__AFMT_GENERIC_BYTE0__SHIFT 0x0
#define AFMT_GENERIC_0__AFMT_GENERIC_BYTE1_MASK 0xff00
#define AFMT_GENERIC_0__AFMT_GENERIC_BYTE1__SHIFT 0x8
#define AFMT_GENERIC_0__AFMT_GENERIC_BYTE2_MASK 0xff0000
#define AFMT_GENERIC_0__AFMT_GENERIC_BYTE2__SHIFT 0x10
#define AFMT_GENERIC_0__AFMT_GENERIC_BYTE3_MASK 0xff000000
#define AFMT_GENERIC_0__AFMT_GENERIC_BYTE3__SHIFT 0x18
#define AFMT_GENERIC_1__AFMT_GENERIC_BYTE4_MASK 0xff
#define AFMT_GENERIC_1__AFMT_GENERIC_BYTE4__SHIFT 0x0
#define AFMT_GENERIC_1__AFMT_GENERIC_BYTE5_MASK 0xff00
#define AFMT_GENERIC_1__AFMT_GENERIC_BYTE5__SHIFT 0x8
#define AFMT_GENERIC_1__AFMT_GENERIC_BYTE6_MASK 0xff0000
#define AFMT_GENERIC_1__AFMT_GENERIC_BYTE6__SHIFT 0x10
#define AFMT_GENERIC_1__AFMT_GENERIC_BYTE7_MASK 0xff000000
#define AFMT_GENERIC_1__AFMT_GENERIC_BYTE7__SHIFT 0x18
#define AFMT_GENERIC_2__AFMT_GENERIC_BYTE8_MASK 0xff
#define AFMT_GENERIC_2__AFMT_GENERIC_BYTE8__SHIFT 0x0
#define AFMT_GENERIC_2__AFMT_GENERIC_BYTE9_MASK 0xff00
#define AFMT_GENERIC_2__AFMT_GENERIC_BYTE9__SHIFT 0x8
#define AFMT_GENERIC_2__AFMT_GENERIC_BYTE10_MASK 0xff0000
#define AFMT_GENERIC_2__AFMT_GENERIC_BYTE10__SHIFT 0x10
#define AFMT_GENERIC_2__AFMT_GENERIC_BYTE11_MASK 0xff000000
#define AFMT_GENERIC_2__AFMT_GENERIC_BYTE11__SHIFT 0x18
#define AFMT_GENERIC_3__AFMT_GENERIC_BYTE12_MASK 0xff
#define AFMT_GENERIC_3__AFMT_GENERIC_BYTE12__SHIFT 0x0
#define AFMT_GENERIC_3__AFMT_GENERIC_BYTE13_MASK 0xff00
#define AFMT_GENERIC_3__AFMT_GENERIC_BYTE13__SHIFT 0x8
#define AFMT_GENERIC_3__AFMT_GENERIC_BYTE14_MASK 0xff0000
#define AFMT_GENERIC_3__AFMT_GENERIC_BYTE14__SHIFT 0x10
#define AFMT_GENERIC_3__AFMT_GENERIC_BYTE15_MASK 0xff000000
#define AFMT_GENERIC_3__AFMT_GENERIC_BYTE15__SHIFT 0x18
#define AFMT_GENERIC_4__AFMT_GENERIC_BYTE16_MASK 0xff
#define AFMT_GENERIC_4__AFMT_GENERIC_BYTE16__SHIFT 0x0
#define AFMT_GENERIC_4__AFMT_GENERIC_BYTE17_MASK 0xff00
#define AFMT_GENERIC_4__AFMT_GENERIC_BYTE17__SHIFT 0x8
#define AFMT_GENERIC_4__AFMT_GENERIC_BYTE18_MASK 0xff0000
#define AFMT_GENERIC_4__AFMT_GENERIC_BYTE18__SHIFT 0x10
#define AFMT_GENERIC_4__AFMT_GENERIC_BYTE19_MASK 0xff000000
#define AFMT_GENERIC_4__AFMT_GENERIC_BYTE19__SHIFT 0x18
#define AFMT_GENERIC_5__AFMT_GENERIC_BYTE20_MASK 0xff
#define AFMT_GENERIC_5__AFMT_GENERIC_BYTE20__SHIFT 0x0
#define AFMT_GENERIC_5__AFMT_GENERIC_BYTE21_MASK 0xff00
#define AFMT_GENERIC_5__AFMT_GENERIC_BYTE21__SHIFT 0x8
#define AFMT_GENERIC_5__AFMT_GENERIC_BYTE22_MASK 0xff0000
#define AFMT_GENERIC_5__AFMT_GENERIC_BYTE22__SHIFT 0x10
#define AFMT_GENERIC_5__AFMT_GENERIC_BYTE23_MASK 0xff000000
#define AFMT_GENERIC_5__AFMT_GENERIC_BYTE23__SHIFT 0x18
#define AFMT_GENERIC_6__AFMT_GENERIC_BYTE24_MASK 0xff
#define AFMT_GENERIC_6__AFMT_GENERIC_BYTE24__SHIFT 0x0
#define AFMT_GENERIC_6__AFMT_GENERIC_BYTE25_MASK 0xff00
#define AFMT_GENERIC_6__AFMT_GENERIC_BYTE25__SHIFT 0x8
#define AFMT_GENERIC_6__AFMT_GENERIC_BYTE26_MASK 0xff0000
#define AFMT_GENERIC_6__AFMT_GENERIC_BYTE26__SHIFT 0x10
#define AFMT_GENERIC_6__AFMT_GENERIC_BYTE27_MASK 0xff000000
#define AFMT_GENERIC_6__AFMT_GENERIC_BYTE27__SHIFT 0x18
#define AFMT_GENERIC_7__AFMT_GENERIC_BYTE28_MASK 0xff
#define AFMT_GENERIC_7__AFMT_GENERIC_BYTE28__SHIFT 0x0
#define AFMT_GENERIC_7__AFMT_GENERIC_BYTE29_MASK 0xff00
#define AFMT_GENERIC_7__AFMT_GENERIC_BYTE29__SHIFT 0x8
#define AFMT_GENERIC_7__AFMT_GENERIC_BYTE30_MASK 0xff0000
#define AFMT_GENERIC_7__AFMT_GENERIC_BYTE30__SHIFT 0x10
#define AFMT_GENERIC_7__AFMT_GENERIC_BYTE31_MASK 0xff000000
#define AFMT_GENERIC