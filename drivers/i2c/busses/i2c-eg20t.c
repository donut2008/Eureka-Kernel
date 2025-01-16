WB_BUFMGR_ENABLE__SHIFT 0x0
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUF_DUALSIZE_REQ_MASK 0x2
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUF_DUALSIZE_REQ__SHIFT 0x1
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUFMGR_SW_INT_EN_MASK 0x10
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUFMGR_SW_INT_EN__SHIFT 0x4
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUFMGR_SW_INT_ACK_MASK 0x20
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUFMGR_SW_INT_ACK__SHIFT 0x5
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUFMGR_SW_SLICE_INT_EN_MASK 0x40
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUFMGR_SW_SLICE_INT_EN__SHIFT 0x6
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUFMGR_SW_LOCK_MASK 0xf00
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_BUFMGR_SW_LOCK__SHIFT 0x8
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_P_VMID_MASK 0xf0000
#define MCIF_WB_BUFMGR_SW_CONTROL__MCIF_WB_P_VMID__SHIFT 0x10
#define MCIF_WB_BUFMGR_CUR_LINE_R__MCIF_WB_BUFMGR_CUR_LINE_R_MASK 0x1fff
#define MCIF_WB_BUFMGR_CUR_LINE_R__MCIF_WB_BUFMGR_CUR_LINE_R__SHIFT 0x0
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_VCE_INT_STATUS_MASK 0x1
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_VCE_INT_STATUS__SHIFT 0x0
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_SW_INT_STATUS_MASK 0x2
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_SW_INT_STATUS__SHIFT 0x1
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_CUR_BUF_MASK 0x70
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_CUR_BUF__SHIFT 0x4
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUF_DUALSIZE_STATUS_MASK 0x80
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUF_DUALSIZE_STATUS__SHIFT 0x7
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_BUFTAG_MASK 0xf00
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_BUFTAG__SHIFT 0x8
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_CUR_LINE_L_MASK 0x1fff000
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_CUR_LINE_L__SHIFT 0xc
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_NEXT_BUF_MASK 0x70000000
#define MCIF_WB_BUFMGR_STATUS__MCIF_WB_BUFMGR_NEXT_BUF__SHIFT 0x1c
#define MCIF_WB_BUF_PITCH__MCIF_WB_BUF_LUMA_PITCH_MASK 0xff00
#define MCIF_WB_BUF_PITCH__MCIF_WB_BUF_LUMA_PITCH__SHIFT 0x8
#define MCIF_WB_BUF_PITCH__MCIF_WB_BUF_CHROMA_PITCH_MASK 0xff000000
#define MCIF_WB_BUF_PITCH__MCIF_WB_BUF_CHROMA_PITCH__SHIFT 0x18
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_ACTIVE_MASK 0x1
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_ACTIVE__SHIFT 0x0
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_SW_LOCKED_MASK 0x2
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_SW_LOCKED__SHIFT 0x1
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_VCE_LOCKED_MASK 0x4
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_VCE_LOCKED__SHIFT 0x2
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_OVERFLOW_MASK 0x8
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_OVERFLOW__SHIFT 0x3
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_DISABLE_MASK 0x10
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_DISABLE__SHIFT 0x4
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_MODE_MASK 0xe0
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_MODE__SHIFT 0x5
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_BUFTAG_MASK 0xf00
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_BUFTAG__SHIFT 0x8
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_NXT_BUF_MASK 0x7000
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_NXT_BUF__SHIFT 0xc
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_FIELD_MASK 0x8000
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_FIELD__SHIFT 0xf
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_CUR_LINE_L_MASK 0x1fff0000
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_CUR_LINE_L__SHIFT 0x10
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_LONG_LINE_ERROR_MASK 0x20000000
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_LONG_LINE_ERROR__SHIFT 0x1d
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_SHORT_LINE_ERROR_MASK 0x40000000
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_SHORT_LINE_ERROR__SHIFT 0x1e
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_FRAME_LENGTH_ERROR_MASK 0x80000000
#define MCIF_WB_BUF_1_STATUS__MCIF_WB_BUF_1_FRAME_LENGTH_ERROR__SHIFT 0x1f
#define MCIF_WB_BUF_1_STATUS2__MCIF_WB_BUF_1_CUR_LINE_R_MASK 0x1fff
#define MCIF_WB_BUF_1_STATUS2__MCIF_WB_BUF_1_CUR_LINE_R__SHIFT 0x0
#define MCIF_WB_BUF_1_STATUS2__MCIF_WB_BUF_1_NEW_CONTENT_MASK 0x2000
#define MCIF_WB_BUF_1_STATUS2__MCIF_WB_BUF_1_NEW_CONTENT__SHIFT 0xd
#define MCIF_WB_BUF_1_STATUS2__MCIF_WB_BUF_1_COLOR_DEPTH_MASK 0x4000
#define MCIF_WB_BUF_1_STATUS2__MCIF_WB_BUF_1_COLOR_DEPTH__SHIFT 0xe
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_ACTIVE_MASK 0x1
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_ACTIVE__SHIFT 0x0
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_SW_LOCKED_MASK 0x2
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_SW_LOCKED__SHIFT 0x1
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_VCE_LOCKED_MASK 0x4
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_VCE_LOCKED__SHIFT 0x2
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_OVERFLOW_MASK 0x8
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_OVERFLOW__SHIFT 0x3
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_DISABLE_MASK 0x10
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_DISABLE__SHIFT 0x4
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_MODE_MASK 0xe0
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_MODE__SHIFT 0x5
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_BUFTAG_MASK 0xf00
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_BUFTAG__SHIFT 0x8
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_NXT_BUF_MASK 0x7000
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_NXT_BUF__SHIFT 0xc
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_FIELD_MASK 0x8000
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_FIELD__SHIFT 0xf
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_CUR_LINE_L_MASK 0x1fff0000
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_CUR_LINE_L__SHIFT 0x10
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_LONG_LINE_ERROR_MASK 0x20000000
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_LONG_LINE_ERROR__SHIFT 0x1d
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_SHORT_LINE_ERROR_MASK 0x40000000
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_SHORT_LINE_ERROR__SHIFT 0x1e
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_FRAME_LENGTH_ERROR_MASK 0x80000000
#define MCIF_WB_BUF_2_STATUS__MCIF_WB_BUF_2_FRAME_LENGTH_ERROR__SHIFT 0x1f
#define MCIF_WB_BUF_2_STATUS2__MCIF_WB_BUF_2_CUR_LINE_R_MASK 0x1fff
#define MCIF_WB_BUF_2_STATUS2__MCIF_WB_BUF_2_CUR_LINE_R__SHIFT 0x0
#define MCIF_WB_BUF_2_STATUS2__MCIF_WB_BUF_2_NEW_CONTENT_MASK 0x2000
#define MCIF_WB_BUF_2_STATUS2__MCIF_WB_BUF_2_NEW_CONTENT__SHIFT 0xd
#define MCIF_WB_BUF_2_STATUS2__MCIF_WB_BUF_2_COLOR_DEPTH_MASK 0x4000
#define MCIF_WB_BUF_2_STATUS2__MCIF_WB_BUF_2_COLOR_DEPTH__SHIFT 0xe
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_ACTIVE_MASK 0x1
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_ACTIVE__SHIFT 0x0
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_SW_LOCKED_MASK 0x2
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_SW_LOCKED__SHIFT 0x1
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_VCE_LOCKED_MASK 0x4
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_VCE_LOCKED__SHIFT 0x2
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_OVERFLOW_MASK 0x8
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_OVERFLOW__SHIFT 0x3
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_DISABLE_MASK 0x10
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_DISABLE__SHIFT 0x4
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_MODE_MASK 0xe0
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_MODE__SHIFT 0x5
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_BUFTAG_MASK 0xf00
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_BUFTAG__SHIFT 0x8
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_NXT_BUF_MASK 0x7000
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_NXT_BUF__SHIFT 0xc
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_FIELD_MASK 0x8000
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_FIELD__SHIFT 0xf
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_CUR_LINE_L_MASK 0x1fff0000
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_CUR_LINE_L__SHIFT 0x10
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_LONG_LINE_ERROR_MASK 0x20000000
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_LONG_LINE_ERROR__SHIFT 0x1d
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_SHORT_LINE_ERROR_MASK 0x40000000
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_SHORT_LINE_ERROR__SHIFT 0x1e
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_FRAME_LENGTH_ERROR_MASK 0x80000000
#define MCIF_WB_BUF_3_STATUS__MCIF_WB_BUF_3_FRAME_LENGTH_ERROR__SHIFT 0x1f
#define MCIF_WB_BUF_3_STATUS2__MCIF_WB_BUF_3_CUR_LINE_R_MASK 0x1fff
#define MCIF_WB_BUF_3_STATUS2__MCIF_WB_BUF_3_CUR_LINE_R__SHIFT 0x0
#define MCIF_WB_BUF_3_STATUS2__MCIF_WB_BUF_3_NEW_CONTENT_MASK 0x2000
#define MCIF_WB_BUF_3_STATUS2__MCIF_WB_BUF_3_NEW_CONTENT__SHIFT 0xd
#define MCIF_WB_BUF_3_STATUS2__MCIF_WB_BUF_3_COLOR_DEPTH_MASK 0x4000
#define MCIF_WB_BUF_3_STATUS2__MCIF_WB_BUF_3_COLOR_DEPTH__SHIFT 0xe
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_ACTIVE_MASK 0x1
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_ACTIVE__SHIFT 0x0
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_SW_LOCKED_MASK 0x2
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_SW_LOCKED__SHIFT 0x1
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_VCE_LOCKED_MASK 0x4
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_VCE_LOCKED__SHIFT 0x2
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_OVERFLOW_MASK 0x8
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_OVERFLOW__SHIFT 0x3
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_DISABLE_MASK 0x10
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_DISABLE__SHIFT 0x4
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_MODE_MASK 0xe0
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_MODE__SHIFT 0x5
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_BUFTAG_MASK 0xf00
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_BUFTAG__SHIFT 0x8
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_NXT_BUF_MASK 0x7000
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_NXT_BUF__SHIFT 0xc
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_FIELD_MASK 0x8000
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_FIELD__SHIFT 0xf
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_CUR_LINE_L_MASK 0x1fff0000
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_CUR_LINE_L__SHIFT 0x10
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_LONG_LINE_ERROR_MASK 0x20000000
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_LONG_LINE_ERROR__SHIFT 0x1d
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_SHORT_LINE_ERROR_MASK 0x40000000
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_SHORT_LINE_ERROR__SHIFT 0x1e
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_FRAME_LENGTH_ERROR_MASK 0x80000000
#define MCIF_WB_BUF_4_STATUS__MCIF_WB_BUF_4_FRAME_LENGTH_ERROR__SHIFT 0x1f
#define MCIF_WB_BUF_4_STATUS2__MCIF_WB_BUF_4_CUR_LINE_R_MASK 0x1fff
#define MCIF_WB_BUF_4_STATUS2__MCIF_WB_BUF_4_CUR_LINE_R__SHIFT 0x0
#define MCIF_WB_BUF_4_STATUS2__MCIF_WB_BUF_4_NEW_CONTENT_MASK 0x2000
#define MCIF_WB_BUF_4_STATUS2__MCIF_WB_BUF_4_NEW_CONTENT__SHIFT 0xd
#define MCIF_WB_BUF_4_STATUS2__MCIF_WB_BUF_4_COLOR_DEPTH_MASK 0x4000
#define MCIF_WB_BUF_4_STATUS2__MCIF_WB_BUF_4_COLOR_DEPTH__SHIFT 0xe
#define MCIF_WB_ARBITRATION_CONTROL__MCIF_WB_CLIENT_ARBITRATION_SLICE_MASK 0x3
#define MCIF_WB_ARBITRATION_CONTROL__MCIF_WB_CLIENT_ARBITRATION_SLICE__SHIFT 0x0
#define MCIF_WB_ARBITRATION_CONTROL__MCIF_WB_TIME_PER_PIXEL_MASK 0xfc000000
#define MCIF_WB_ARBITRATION_CONTROL__MCIF_WB_TIME_PER_PIXEL__SHIFT 0x1a
#define MCIF_WB_URGENCY_WATERMARK__MCIF_WB_CLIENT0_URGENCY_WATERMARK_MASK 0xffff
#define MCIF_WB_URGENCY_WATERMARK__MCIF_WB_CLIENT0_URGENCY_WATERMARK__SHIFT 0x0
#define MCIF_WB_URGENCY_WATERMARK__MCIF_WB_CLIENT1_URGENCY_WATERMARK_MASK 0xffff0000
#define MCIF_WB_URGENCY_WATERMARK__MCIF_WB_CLIENT1_URGENCY_WATERMARK__SHIFT 0x10
#define MCIF_WB_TEST_DEBUG_INDEX__MCIF_WB_TEST_DEBUG_INDEX_MASK 0xff
#define MCIF_WB_TEST_DEBUG_INDEX__MCIF_WB_TEST_DEBUG_INDEX__SHIFT 0x0
#define MCIF_WB_TEST_DEBUG_INDEX__MCIF_WB_TEST_DEBUG_WRITE_EN_MASK 0x100
#define MCIF_WB_TEST_DEBUG_INDEX__MCIF_WB_TEST_DEBUG_WRITE_EN__SHIFT 0x8
#define MCIF_WB_TEST_DEBUG_DATA__MCIF_WB_TEST_DEBUG_DATA_MASK 0xffffffff
#define MCIF_WB_TEST_DEBUG_DATA__MCIF_WB_TEST_DEBUG_DATA__SHIFT 0x0
#define MCIF_WB_BUF_1_ADDR_Y__MCIF_WB_BUF_1_ADDR_Y_MASK 0xffffffff
#define MCIF_WB_BUF_1_ADDR_Y__MCIF_WB_BUF_1_ADDR_Y__SHIFT 0x0
#define MCIF_WB_BUF_1_ADDR_Y_OFFSET__MCIF_WB_BUF_1_ADDR_Y_OFFSET_MASK 0x3ffff
#define MCIF_WB_BUF_1_ADDR_Y_OFFSET__MCIF_WB_BUF_1_ADDR_Y_OFFSET__SHIFT 0x0
#define MCIF_WB_BUF_1_ADDR_C__MCIF_WB_BUF_1_ADDR_C_MASK 0xffffffff
#define MCIF_WB_BUF_1_ADDR_C__MCIF_WB_BUF_1_ADDR_C__SHIFT 0x0
#define MCIF_WB_BUF_1_ADDR_C_OFFSET__MCIF_WB_BUF_1_ADDR_C_OFFSET_MASK 0x3ffff
#define MCIF_WB_BUF_1_ADDR_C_OFFSET__MCIF_WB_BUF_1_ADDR_C_OFFSET__SHIFT 0x0
#define MCIF_WB_BUF_2_ADDR_Y__MCIF_WB_BUF_2_ADDR_Y_MASK 0xffffffff
#define MCIF_WB_BUF_2_ADDR_Y__MCIF_WB_BUF_2_ADDR_Y__SHIFT 0x0
#define MCIF_WB_BUF_2_ADDR_Y_OFFSET__MCIF_WB_BUF_2_ADDR_Y_OFFSET_MASK 0x3ffff
#define MCIF_WB_BUF_2_ADDR_Y_OFFSET__MCIF_WB_BUF_2_ADDR_Y_OFFSET__SHIFT 0x0
#define MCIF_WB_BUF_2_ADDR_C__MCIF_WB_BUF_2_ADDR_C_MASK 0xffffffff
#define MCIF_WB_BUF_2_ADDR_C__MCIF_WB_BUF_2_ADDR_C__SHIFT 0x0
#define MCIF_WB_BUF_2_ADDR_C_OFFSET__MCIF_WB_BUF_2_ADDR_C_OFFSET_MASK 0x3ffff
#define MCIF_WB_BUF_2_ADDR_C_OFFSET__MCIF_WB_BUF_2_ADDR_C_OFFSET__SHIFT 0x0
#define MCIF_WB_BUF_3_ADDR_Y__MCIF_WB_BUF_3_ADDR_Y_MASK 0xffffffff
#define MCIF_WB_BUF_3_ADDR_Y__MCIF_WB_BUF_3_ADDR_Y__SHIFT 0x0
#define MCIF_WB_BUF_3_ADDR_Y_OFFSET__MCIF_WB_BUF_3_ADDR_Y_OFFSET_MASK 0x3ffff
#define MCIF_WB_BUF_3_ADDR_Y_OFFSET__MCIF_WB_BUF_3_ADDR_Y_OFFSET__SHIFT 0x0
#define MCIF_WB_BUF_3_ADDR_C__MCIF_WB_BUF_3_ADDR_C_MASK 0xffffffff
#define MCIF_WB_BUF_3_ADDR_C__MCIF_WB_BUF_3_ADDR_C__SHIFT 0x0
#define MCIF_WB_BUF_3_ADDR_C_OFFSET__MCIF_WB_BUF_3_ADDR_C_OFFSET_MASK 0x3ffff
#define MCIF_WB_BUF_3_ADDR_C_OFFSET__MCIF_WB_BUF_3_ADDR_C_OFFSET__SHIFT 0x0
#define MCIF_WB_BUF_4_ADDR_Y__MCIF_WB_BUF_4_ADDR_Y_MASK 0xffffffff
#define MCIF_WB_BUF_4_ADDR_Y__MCIF_WB_BUF_4_ADDR_Y__SHIFT 0x0
#define MCIF_WB_BUF_4_ADDR_Y_OFFSET__MCIF_WB_BUF_4_ADDR_Y_OFFSET_MASK 0x3ffff
#define MCIF_WB_BUF_4_ADDR_Y_OFFSET__MCIF_WB_BUF_4_ADDR_Y_OFFSET__SHIFT 0x0
#define MCIF_WB_BUF_4_ADDR_C__MCIF_WB_BUF_4_ADDR_C_MASK 0xffffffff
#define MCIF_WB_BUF_4_ADDR_C__MCIF_WB_BUF_4_ADDR_C__SHIFT 0x0
#define MCIF_WB_BUF_4_ADDR_C_OFFSET__MCIF_WB_BUF_4_ADDR_C_OFFSET_MASK 0x3ffff
#define MCIF_WB_BUF_4_ADDR_C_OFFSET__MCIF_WB_BUF_4_ADDR_C_OFFSET__SHIFT 0x0
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_LOCK_IGNORE_MASK 0x1
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_LOCK_IGNORE__SHIFT 0x0
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_INT_EN_MASK 0x10
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_INT_EN__SHIFT 0x4
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_INT_ACK_MASK 0x20
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_INT_ACK__SHIFT 0x5
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_SLICE_INT_EN_MASK 0x40
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_SLICE_INT_EN__SHIFT 0x6
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_LOCK_MASK 0xf00
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_VCE_LOCK__SHIFT 0x8
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_SLICE_SIZE_MASK 0x1fff0000
#define MCIF_WB_BUFMGR_VCE_CONTROL__MCIF_WB_BUFMGR_SLICE_SIZE__SHIFT 0x10
#define MCIF_WB_HVVMID_CONTROL__MCIF_WB_DEFAULT_VMID_MASK 0xf00
#define MCIF_WB_HVVMID_CONTROL__MCIF_WB_DEFAULT_VMID__SHIFT 0x8
#define MCIF_WB_HVVMID_CONTROL__MCIF_WB_ALLOWED_VMID_MASK_MASK 0xffff0000
#define MCIF_WB_HVVMID_CONTROL__MCIF_WB_ALLOWED_VMID_MASK__SHIFT 0x10

#endif /* GMC_8_2_SH_MASK_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * OSS_3_0_1 Register documentation
 *
 * Copyright (C) 2014  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef OSS_3_0_1_SH_MASK_H
#define OSS_3_0_1_SH_MASK_H

#define IH_VMID_0_LUT__PASID_MASK 0xffff
#define IH_VMID_0_LUT__PASID__SHIFT 0x0
#define IH_VMID_1_LUT__PASID_MASK 0xffff
#define IH_VMID_1_LUT__PASID__SHIFT 0x0
#define IH_VMID_2_LUT__PASID_MASK 0xffff
#define IH_VMID_2_LUT__PASID__SHIFT 0x0
#define IH_VMID_3_LUT__PASID_MASK 0xffff
#define IH_VMID_3_LUT__PASID__SHIFT 0x0
#define IH_VMID_4_LUT__PASID_MASK 0xffff
#define IH_VMID_4_LUT__PASID__SHIFT 0x0
#define IH_VMID_5_LUT__PASID_MASK 0xffff
#define IH_VMID_5_LUT__PASID__SHIFT 0x0
#define IH_VMID_6_LUT__PASID_MASK 0xffff
#define IH_VMID_6_LUT__PASID__SHIFT 0x0
#define IH_VMID_7_LUT__PASID_MASK 0xffff
#define IH_VMID_7_LUT__PASID__SHIFT 0x0
#define IH_VMID_8_LUT__PASID_MASK 0xffff
#define IH_VMID_8_LUT__PASID__SHIFT 0x0
#define IH_VMID_9_LUT__PASID_MASK 0xffff
#define IH_VMID_9_LUT__PASID__SHIFT 0x0
#define IH_VMID_10_LUT__PASID_MASK 0xffff
#define IH_VMID_10_LUT__PASID__SHIFT 0x0
#define IH_VMID_11_LUT__PASID_MASK 0xffff
#define IH_VMID_11_LUT__PASID__SHIFT 0x0
#define IH_VMID_12_LUT__PASID_MASK 0xffff
#define IH_VMID_12_LUT__PASID__SHIFT 0x0
#define IH_VMID_13_LUT__PASID_MASK 0xffff
#define IH_VMID_13_LUT__PASID__SHIFT 0x0
#define IH_VMID_14_LUT__PASID_MASK 0xffff
#define IH_VMID_14_LUT__PASID__SHIFT 0x0
#define IH_VMID_15_LUT__PASID_MASK 0xffff
#define IH_VMID_15_LUT__PASID__SHIFT 0x0
#define IH_RB_CNTL__RB_ENABLE_MASK 0x1
#define IH_RB_CNTL__RB_ENABLE__SHIFT 0x0
#define IH_RB_CNTL__RB_SIZE_MASK 0x3e
#define IH_RB_CNTL__RB_SIZE__SHIFT 0x1
#define IH_RB_CNTL__RB_FULL_DRAIN_ENABLE_MASK 0x40
#define IH_RB_CNTL__RB_FULL_DRAIN_ENABLE__SHIFT 0x6
#define IH_RB_CNTL__RB_GPU_TS_ENABLE_MASK 0x80
#define IH_RB_CNTL__RB_GPU_TS_ENABLE__SHIFT 0x7
#define IH_RB_CNTL__WPTR_WRITEBACK_ENABLE_MASK 0x100
#define IH_RB_CNTL__WPTR_WRITEBACK_ENABLE__SHIFT 0x8
#define IH_RB_CNTL__WPTR_WRITEBACK_TIMER_MASK 0x3e00
#define IH_RB_CNTL__WPTR_WRITEBACK_TIMER__SHIFT 0x9
#define IH_RB_CNTL__WPTR_OVERFLOW_ENABLE_MASK 0x10000
#define IH_RB_CNTL__WPTR_OVERFLOW_ENABLE__SHIFT 0x10
#define IH_RB_CNTL__WPTR_OVERFLOW_CLEAR_MASK 0x80000000
#define IH_RB_CNTL__WPTR_OVERFLOW_CLEAR__SHIFT 0x1f
#define IH_RB_BASE__ADDR_MASK 0xffffffff
#define IH_RB_BASE__ADDR__SHIFT 0x0
#define IH_RB_RPTR__OFFSET_MASK 0x3fffc
#define IH_RB_RPTR__OFFSET__SHIFT 0x2
#define IH_RB_WPTR__RB_OVERFLOW_MASK 0x1
#define IH_RB_WPTR__RB_OVERFLOW__SHIFT 0x0
#define IH_RB_WPTR__OFFSET_MASK 0x3fffc
#define IH_RB_WPTR__OFFSET__SHIFT 0x2
#define IH_RB_WPTR__RB_LEFT_NONE_MASK 0x40000
#define IH_RB_WPTR__RB_LEFT_NONE__SHIFT 0x12
#define IH_RB_WPTR__RB_MAY_OVERFLOW_MASK 0x80000
#define IH_RB_WPTR__RB_MAY_OVERFLOW__SHIFT 0x13
#define IH_RB_WPTR_ADDR_HI__ADDR_MASK 0xff
#define IH_RB_WPTR_ADDR_HI__ADDR__SHIFT 0x0
#define IH_RB_WPTR_ADDR_LO__ADDR_MASK 0xfffffffc
#define IH_RB_WPTR_ADDR_LO__ADDR__SHIFT 0x2
#define IH_CNTL__ENABLE_INTR_MASK 0x1
#define IH_CNTL__ENABLE_INTR__SHIFT 0x0
#define IH_CNTL__MC_SWAP_MASK 0x6
#define IH_CNTL__MC_SWAP__SHIFT 0x1
#define IH_CNTL__RPTR_REARM_MASK 0x10
#define IH_CNTL__RPTR_REARM__SHIFT 0x4
#define IH_CNTL__CLIENT_FIFO_HIGHWATER_MASK 0x300
#define IH_CNTL__CLIENT_FIFO_HIGHWATER__SHIFT 0x8
#define IH_CNTL__MC_FIFO_HIGHWATER_MASK 0x7c00
#define IH_CNTL__MC_FIFO_HIGHWATER__SHIFT 0xa
#define IH_CNTL__MC_WRREQ_CREDIT_MASK 0xf8000
#define IH_CNTL__MC_WRREQ_CREDIT__SHIFT 0xf
#define IH_CNTL__MC_WR_CLEAN_CNT_MASK 0x1f00000
#define IH_CNTL__MC_WR_CLEAN_CNT__SHIFT 0x14
#define IH_CNTL__MC_VMID_MASK 0x1e000000
#define IH_CNTL__MC_VMID__SHIFT 0x19
#define IH_LEVEL_STATUS__DC_STATUS_MASK 0x1
#define IH_LEVEL_STATUS__DC_STATUS__SHIFT 0x0
#define IH_LEVEL_STATUS__ROM_STATUS_MASK 0x4
#define IH_LEVEL_STATUS__ROM_STATUS__SHIFT 0x2
#define IH_LEVEL_STATUS__SRBM_STATUS_MASK 0x8
#define IH_LEVEL_STATUS__SRBM_STATUS__SHIFT 0x3
#define IH_LEVEL_STATUS__BIF_STATUS_MASK 0x10
#define IH_LEVEL_STATUS__BIF_STATUS__SHIFT 0x4
#define IH_LEVEL_STATUS__XDMA_STATUS_MASK 0x20
#define IH_LEVEL_STATUS__XDMA_STATUS__SHIFT 0x5
#define IH_STATUS__IDLE_MASK 0x1
#define IH_STATUS__IDLE__SHIFT 0x0
#define IH_STATUS__INPUT_IDLE_MASK 0x2
#define IH_STATUS__INPUT_IDLE__SHIFT 0x1
#define IH_STATUS__RB_IDLE_MASK 0x4
#define IH_STATUS__RB_IDLE__SHIFT 0x2
#define IH_STATUS__RB_FULL_MASK 0x8
#define IH_STATUS__RB_FULL__SHIFT 0x3
#define IH_STATUS__RB_FULL_DRAIN_MASK 0x10
#define IH_STATUS__RB_FULL_DRAIN__SHIFT 0x4
#define IH_STATUS__RB_OVERFLOW_MASK 0x20
#define IH_STATUS__RB_OVERFLOW__SHIFT 0x5
#define IH_STATUS__MC_WR_IDLE_MASK 0x40
#define IH_STATUS__MC_WR_IDLE__SHIFT 0x6
#define IH_STATUS__MC_WR_STALL_MASK 0x80
#define IH_STATUS__MC_WR_STALL__SHIFT 0x7
#define IH_STATUS__MC_WR_CLEAN_PENDING_MASK 0x100
#define IH_STATUS__MC_WR_CLEAN_PENDING__SHIFT 0x8
#define IH_STATUS__MC_WR_CLEAN_STALL_MASK 0x200
#define IH_STATUS__MC_WR_CLEAN_STALL__SHIFT 0x9
#define IH_STATUS__BIF_INTERRUPT_LINE_MASK 0x400
#define IH_STATUS__BIF_INTERRUPT_LINE__SHIFT 0xa
#define IH_PERFMON_CNTL__ENABLE0_MASK 0x1
#define IH_PERFMON_CNTL__ENABLE0__SHIFT 0x0
#define IH_PERFMON_CNTL__CLEAR0_MASK 0x2
#define IH_PERFMON_CNTL__CLEAR0__SHIFT 0x1
#define IH_PERFMON_CNTL__PERF_SEL0_MASK 0xfc
#define IH_PERFMON_CNTL__PERF_SEL0__SHIFT 0x2
#define IH_PERFMON_CNTL__ENABLE1_MASK 0x100
#define IH_PERFMON_CNTL__ENABLE1__SHIFT 0x8
#define IH_PERFMON_CNTL__CLEAR1_MASK 0x200
#define IH_PERFMON_CNTL__CLEAR1__SHIFT 0x9
#define IH_PERFMON_CNTL__PERF_SEL1_MASK 0xfc00
#define IH_PERFMON_CNTL__PERF_SEL1__SHIFT 0xa
#define IH_PERFCOUNTER0_RESULT__PERF_COUNT_MASK 0xffffffff
#define IH_PERFCOUNTER0_RESULT__PERF_COUNT__SHIFT 0x0
#define IH_PERFCOUNTER1_RESULT__PERF_COUNT_MASK 0xffffffff
#define IH_PERFCOUNTER1_RESULT__PERF_COUNT__SHIFT 0x0
#define IH_DSM_MATCH_VALUE_BIT_31_0__VALUE_MASK 0xffffffff
#define IH_DSM_MATCH_VALUE_BIT_31_0__VALUE__SHIFT 0x0
#define IH_DSM_MATCH_VALUE_BIT_63_32__VALUE_MASK 0xffffffff
#define IH_DSM_MATCH_VALUE_BIT_63_32__VALUE__SHIFT 0x0
#define IH_DSM_MATCH_VALUE_BIT_95_64__VALUE_MASK 0xffffffff
#define IH_DSM_MATCH_VALUE_BIT_95_64__VALUE__SHIFT 0x0
#define IH_DSM_MATCH_FIELD_CONTROL__SRC_EN_MASK 0x1
#define IH_DSM_MATCH_FIELD_CONTROL__SRC_EN__SHIFT 0x0
#define IH_DSM_MATCH_FIELD_CONTROL__TIMESTAMP_EN_MASK 0x4
#define IH_DSM_MATCH_FIELD_CONTROL__TIMESTAMP_EN__SHIFT 0x2
#define IH_DSM_MATCH_FIELD_CONTROL__RINGID_EN_MASK 0x8
#define IH_DSM_MATCH_FIELD_CONTROL__RINGID_EN__SHIFT 0x3
#define IH_DSM_MATCH_FIELD_CONTROL__VMID_EN_MASK 0x10
#define IH_DSM_MATCH_FIELD_CONTROL__VMID_EN__SHIFT 0x4
#define IH_DSM_MATCH_FIELD_CONTROL__PASID_EN_MASK 0x20
#define IH_DSM_MATCH_FIELD_CONTROL__PASID_EN__SHIFT 0x5
#define IH_DSM_MATCH_DATA_CONTROL__VALUE_MASK 0xfffffff
#define IH_DSM_MATCH_DATA_CONTROL__VALUE__SHIFT 0x0
#define IH_VERSION__VALUE_MASK 0xfff
#define IH_VERSION__VALUE__SHIFT 0x0
#define SEM_MCIF_CONFIG__MC_REQ_SWAP_MASK 0x3
#define SEM_MCIF_CONFIG__MC_REQ_SWAP__SHIFT 0x0
#define SEM_MCIF_CONFIG__MC_WRREQ_CREDIT_MASK 0xfc
#define SEM_MCIF_CONFIG__MC_WRREQ_CREDIT__SHIFT 0x2
#define SEM_MCIF_CONFIG__MC_RDREQ_CREDIT_MASK 0x3f00
#define SEM_MCIF_CONFIG__MC_RDREQ_CREDIT__SHIFT 0x8
#define SEM_PERFMON_CNTL__PERF_ENABLE0_MASK 0x1
#define SEM_PERFMON_CNTL__PERF_ENABLE0__SHIFT 0x0
#define SEM_PERFMON_CNTL__PERF_CLEAR0_MASK 0x2
#define SEM_PERFMON_CNTL__PERF_CLEAR0__SHIFT 0x1
#define SEM_PERFMON_CNTL__PERF_SEL0_MASK 0x3fc
#define SEM_PERFMON_CNTL__PERF_SEL0__SHIFT 0x2
#define SEM_PERFMON_CNTL__PERF_ENABLE1_MASK 0x400
#define SEM_PERFMON_CNTL__PERF_ENABLE1__SHIFT 0xa
#define SEM_PERFMON_CNTL__PERF_CLEAR1_MASK 0x800
#define SEM_PERFMON_CNTL__PERF_CLEAR1__SHIFT 0xb
#define SEM_PERFMON_CNTL__PERF_SEL1_MASK 0xff000
#define SEM_PERFMON_CNTL__PERF_SEL1__SHIFT 0xc
#define SEM_PERFCOUNTER0_RESULT__P