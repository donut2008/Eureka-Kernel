 SCLV_VERT_FILTER_INIT_BOT__SCL_V_INIT_INT_BOT__SHIFT 0x18
#define SCLV_VERT_FILTER_SCALE_RATIO_C__SCL_V_SCALE_RATIO_C_MASK 0x3ffffff
#define SCLV_VERT_FILTER_SCALE_RATIO_C__SCL_V_SCALE_RATIO_C__SHIFT 0x0
#define SCLV_VERT_FILTER_INIT_C__SCL_V_INIT_FRAC_C_MASK 0xffffff
#define SCLV_VERT_FILTER_INIT_C__SCL_V_INIT_FRAC_C__SHIFT 0x0
#define SCLV_VERT_FILTER_INIT_C__SCL_V_INIT_INT_C_MASK 0x7000000
#define SCLV_VERT_FILTER_INIT_C__SCL_V_INIT_INT_C__SHIFT 0x18
#define SCLV_VERT_FILTER_INIT_BOT_C__SCL_V_INIT_FRAC_BOT_C_MASK 0xffffff
#define SCLV_VERT_FILTER_INIT_BOT_C__SCL_V_INIT_FRAC_BOT_C__SHIFT 0x0
#define SCLV_VERT_FILTER_INIT_BOT_C__SCL_V_INIT_INT_BOT_C_MASK 0x7000000
#define SCLV_VERT_FILTER_INIT_BOT_C__SCL_V_INIT_INT_BOT_C__SHIFT 0x18
#define SCLV_ROUND_OFFSET__SCL_ROUND_OFFSET_RGB_Y_MASK 0xffff
#define SCLV_ROUND_OFFSET__SCL_ROUND_OFFSET_RGB_Y__SHIFT 0x0
#define SCLV_ROUND_OFFSET__SCL_ROUND_OFFSET_CBCR_MASK 0xffff0000
#define SCLV_ROUND_OFFSET__SCL_ROUND_OFFSET_CBCR__SHIFT 0x10
#define SCLV_UPDATE__SCL_UPDATE_PENDING_MASK 0x1
#define SCLV_UPDATE__SCL_UPDATE_PENDING__SHIFT 0x0
#define SCLV_UPDATE__SCL_UPDATE_TAKEN_MASK 0x100
#define SCLV_UPDATE__SCL_UPDATE_TAKEN__SHIFT 0x8
#define SCLV_UPDATE__SCL_UPDATE_LOCK_MASK 0x10000
#define SCLV_UPDATE__SCL_UPDATE_LOCK__SHIFT 0x10
#define SCLV_UPDATE__SCL_COEF_UPDATE_COMPLETE_MASK 0x1000000
#define SCLV_UPDATE__SCL_COEF_UPDATE_COMPLETE__SHIFT 0x18
#define SCLV_ALU_CONTROL__SCL_ALU_DISABLE_MASK 0x1
#define SCLV_ALU_CONTROL__SCL_ALU_DISABLE__SHIFT 0x0
#define SCLV_VIEWPORT_START__VIEWPORT_Y_START_MASK 0x3fff
#define SCLV_VIEWPORT_START__VIEWPORT_Y_START__SHIFT 0x0
#define SCLV_VIEWPORT_START__VIEWPORT_X_START_MASK 0x3fff0000
#define SCLV_VIEWPORT_START__VIEWPORT_X_START__SHIFT 0x10
#define SCLV_VIEWPORT_START_SECONDARY__VIEWPORT_Y_START_SECONDARY_MASK 0x3fff
#define SCLV_VIEWPORT_START_SECONDARY__VIEWPORT_Y_START_SECONDARY__SHIFT 0x0
#define SCLV_VIEWPORT_START_SECONDARY__VIEWPORT_X_START_SECONDARY_MASK 0x3fff0000
#define SCLV_VIEWPORT_START_SECONDARY__VIEWPORT_X_START_SECONDARY__SHIFT 0x10
#define SCLV_VIEWPORT_SIZE__VIEWPORT_HEIGHT_MASK 0x1fff
#define SCLV_VIEWPORT_SIZE__VIEWPORT_HEIGHT__SHIFT 0x0
#define SCLV_VIEWPORT_SIZE__VIEWPORT_WIDTH_MASK 0x1fff0000
#define SCLV_VIEWPORT_SIZE__VIEWPORT_WIDTH__SHIFT 0x10
#define SCLV_VIEWPORT_START_C__VIEWPORT_Y_START_C_MASK 0x3fff
#define SCLV_VIEWPORT_START_C__VIEWPORT_Y_START_C__SHIFT 0x0
#define SCLV_VIEWPORT_START_C__VIEWPORT_X_START_C_MASK 0x3fff0000
#define SCLV_VIEWPORT_START_C__VIEWPORT_X_START_C__SHIFT 0x10
#define SCLV_VIEWPORT_START_SECONDARY_C__VIEWPORT_Y_START_SECONDARY_C_MASK 0x3fff
#define SCLV_VIEWPORT_START_SECONDARY_C__VIEWPORT_Y_START_SECONDARY_C__SHIFT 0x0
#define SCLV_VIEWPORT_START_SECONDARY_C__VIEWPORT_X_START_SECONDARY_C_MASK 0x3fff0000
#define SCLV_VIEWPORT_START_SECONDARY_C__VIEWPORT_X_START_SECONDARY_C__SHIFT 0x10
#define SCLV_VIEWPORT_SIZE_C__VIEWPORT_HEIGHT_C_MASK 0x1fff
#define SCLV_VIEWPORT_SIZE_C__VIEWPORT_HEIGHT_C__SHIFT 0x0
#define SCLV_VIEWPORT_SIZE_C__VIEWPORT_WIDTH_C_MASK 0x1fff0000
#define SCLV_VIEWPORT_SIZE_C__VIEWPORT_WIDTH_C__SHIFT 0x10
#define SCLV_EXT_OVERSCAN_LEFT_RIGHT__EXT_OVERSCAN_RIGHT_MASK 0x1fff
#define SCLV_EXT_OVERSCAN_LEFT_RIGHT__EXT_OVERSCAN_RIGHT__SHIFT 0x0
#define SCLV_EXT_OVERSCAN_LEFT_RIGHT__EXT_OVERSCAN_LEFT_MASK 0x1fff0000
#define SCLV_EXT_OVERSCAN_LEFT_RIGHT__EXT_OVERSCAN_LEFT__SHIFT 0x10
#define SCLV_EXT_OVERSCAN_TOP_BOTTOM__EXT_OVERSCAN_BOTTOM_MASK 0x1fff
#define SCLV_EXT_OVERSCAN_TOP_BOTTOM__EXT_OVERSCAN_BOTTOM__SHIFT 0x0
#define SCLV_EXT_OVERSCAN_TOP_BOTTOM__EXT_OVERSCAN_TOP_MASK 0x1fff0000
#define SCLV_EXT_OVERSCAN_TOP_BOTTOM__EXT_OVERSCAN_TOP__SHIFT 0x10
#define SCLV_MODE_CHANGE_DET1__SCL_MODE_CHANGE_MASK 0x1
#define SCLV_MODE_CHANGE_DET1__SCL_MODE_CHANGE__SHIFT 0x0
#define SCLV_MODE_CHANGE_DET1__SCL_MODE_CHANGE_ACK_MASK 0x10
#define SCLV_MODE_CHANGE_DET1__SCL_MODE_CHANGE_ACK__SHIFT 0x4
#define SCLV_MODE_CHANGE_DET1__SCL_ALU_H_SCALE_RATIO_MASK 0xfffff80
#define SCLV_MODE_CHANGE_DET1__SCL_ALU_H_SCALE_RATIO__SHIFT 0x7
#define SCLV_MODE_CHANGE_DET2__SCL_ALU_V_SCALE_RATIO_MASK 0x1fffff
#define SCLV_MODE_CHANGE_DET2__SCL_ALU_V_SCALE_RATIO__SHIFT 0x0
#define SCLV_MODE_CHANGE_DET3__SCL_ALU_SOURCE_HEIGHT_MASK 0x3fff
#define SCLV_MODE_CHANGE_DET3__SCL_ALU_SOURCE_HEIGHT__SHIFT 0x0
#define SCLV_MODE_CHANGE_DET3__SCL_ALU_SOURCE_WIDTH_MASK 0x3fff0000
#define SCLV_MODE_CHANGE_DET3__SCL_ALU_SOURCE_WIDTH__SHIFT 0x10
#define SCLV_MODE_CHANGE_MASK__SCL_MODE_CHANGE_MASK_MASK 0x1
#define SCLV_MODE_CHANGE_MASK__SCL_MODE_CHANGE_MASK__SHIFT 0x0
#define SCLV_HORZ_FILTER_INIT_BOT__SCL_H_INIT_FRAC_BOT_MASK 0xffffff
#define SCLV_HORZ_FILTER_INIT_BOT__SCL_H_INIT_FRAC_BOT__SHIFT 0x0
#define SCLV_HORZ_FILTER_INIT_BOT__SCL_H_INIT_INT_BOT_MASK 0xf000000
#define SCLV_HORZ_FILTER_INIT_BOT__SCL_H_INIT_INT_BOT__SHIFT 0x18
#define SCLV_HORZ_FILTER_INIT_BOT_C__SCL_H_INIT_FRAC_BOT_C_MASK 0xffffff
#define SCLV_HORZ_FILTER_INIT_BOT_C__SCL_H_INIT_FRAC_BOT_C__SHIFT 0x0
#define SCLV_HORZ_FILTER_INIT_BOT_C__SCL_H_INIT_INT_BOT_C_MASK 0xf000000
#define SCLV_HORZ_FILTER_INIT_BOT_C__SCL_H_INIT_INT_BOT_C__SHIFT 0x18
#define SCLV_DEBUG2__SCL_DEBUG_REQ_MODE_MASK 0x1
#define SCLV_DEBUG2__SCL_DEBUG_REQ_MODE__SHIFT 0x0
#define SCLV_DEBUG2__SCL_DEBUG_EOF_MODE_MASK 0x6
#define SCLV_DEBUG2__SCL_DEBUG_EOF_MODE__SHIFT 0x1
#define SCLV_DEBUG2__SCL_DEBUG2_MASK 0xfffffff8
#define SCLV_DEBUG2__SCL_DEBUG2__SHIFT 0x3
#define SCLV_DEBUG__SCL_DEBUG_MASK 0xffffffff
#define SCLV_DEBUG__SCL_DEBUG__SHIFT 0x0
#define SCLV_TEST_DEBUG_INDEX__SCL_TEST_DEBUG_INDEX_MASK 0xff
#define SCLV_TEST_DEBUG_INDEX__SCL_TEST_DEBUG_INDEX__SHIFT 0x0
#define SCLV_TEST_DEBUG_INDEX__SCL_TEST_DEBUG_WRITE_EN_MASK 0x100
#define SCLV_TEST_DEBUG_INDEX__SCL_TEST_DEBUG_WRITE_EN__SHIFT 0x8
#define SCLV_TEST_DEBUG_DATA__SCL_TEST_DEBUG_DATA_MASK 0xffffffff
#define SCLV_TEST_DEBUG_DATA__SCL_TEST_DEBUG_DATA__SHIFT 0x0
#define COL_MAN_UPDATE__COL_MAN_UPDATE_PENDING_MASK 0x1
#define COL_MAN_UPDATE__COL_MAN_UPDATE_PENDING__SHIFT 0x0
#define COL_MAN_UPDATE__COL_MAN_UPDATE_TAKEN_MASK 0x2
#define COL_MAN_UPDATE__COL_MAN_UPDATE_TAKEN__SHIFT 0x1
#define COL_MAN_UPDATE__COL_MAN_UPDATE_LOCK_MASK 0x10000
#define COL_MAN_UPDATE__COL_MAN_UPDATE_LOCK__SHIFT 0x10
#define COL_MAN_UPDATE__COL_MAN_DISABLE_MULTIPLE_UPDATE_MASK 0x1000000
#define COL_MAN_UPDATE__COL_MAN_DISABLE_MULTIPLE_UPDATE__SHIFT 0x18
#define COL_MAN_INPUT_CSC_CONTROL__INPUT_CSC_MODE_MASK 0x3
#define COL_MAN_INPUT_CSC_CONTROL__INPUT_CSC_MODE__SHIFT 0x0
#define COL_MAN_INPUT_CSC_CONTROL__INPUT_CSC_INPUT_TYPE_MASK 0x300
#define COL_MAN_INPUT_CSC_CONTROL__INPUT_CSC_INPUT_TYPE__SHIFT 0x8
#define COL_MAN_INPUT_CSC_CONTROL__INPUT_CSC_CONVERSION_MODE_MASK 0x10000
#define COL_MAN_INPUT_CSC_CONTROL__INPUT_CSC_CONVERSION_MODE__SHIFT 0x10
#define INPUT_CSC_C11_C12_A__INPUT_CSC_C11_A_MASK 0xffff
#define INPUT_CSC_C11_C12_A__INPUT_CSC_C11_A__SHIFT 0x0
#define INPUT_CSC_C11_C12_A__INPUT_CSC_C12_A_MASK 0xffff0000
#define INPUT_CSC_C11_C12_A__INPUT_CSC_C12_A__SHIFT 0x10
#define INPUT_CSC_C13_C14_A__INPUT_CSC_C13_A_MASK 0xffff
#define INPUT_CSC_C13_C14_A__INPUT_CSC_C13_A__SHIFT 0x0
#define INPUT_CSC_C13_C14_A__INPUT_CSC_C14_A_MASK 0xffff0000
#define INPUT_CSC_C13_C14_A__INPUT_CSC_C14_A__SHIFT 0x10
#define INPUT_CSC_C21_C22_A__INPUT_CSC_C21_A_MASK 0xffff
#define INPUT_CSC_C21_C22_A__INPUT_CSC_C21_A__SHIFT 0x0
#define INPUT_CSC_C21_C22_A__INPUT_CSC_C22_A_MASK 0xffff0000
#define INPUT_CSC_C21_C22_A__INPUT_CSC_C22_A__SHIFT 0x10
#define INPUT_CSC_C23_C24_A__INPUT_CSC_C23_A_MASK 0xffff
#define INPUT_CSC_C23_C24_A__INPUT_CSC_C23_A__SHIFT 0x0
#define INPUT_CSC_C23_C24_A__INPUT_CSC_C24_A_MASK 0xffff0000
#define INPUT_CSC_C23_C24_A__INPUT_CSC_C24_A__SHIFT 0x10
#define INPUT_CSC_C31_C32_A__INPUT_CSC_C31_A_MASK 0xffff
#define INPUT_CSC_C31_C32_A__INPUT_CSC_C31_A__SHIFT 0x0
#define INPUT_CSC_C31_C32_A__INPUT_CSC_C32_A_MASK 0xffff0000
#define INPUT_CSC_C31_C32_A__INPUT_CSC_C32_A__SHIFT 0x10
#define INPUT_CSC_C33_C34_A__INPUT_CSC_C33_A_MASK 0xffff
#define INPUT_CSC_C33_C34_A__INPUT_CSC_C33_A__SHIFT 0x0
#define INPUT_CSC_C33_C34_A__INPUT_CSC_C34_A_MASK 0xffff0000
#define INPUT_CSC_C33_C34_A__INPUT_CSC_C34_A__SHIFT 0x10
#define INPUT_CSC_C11_C12_B__INPUT_CSC_C11_B_MASK 0xffff
#define INPUT_CSC_C11_C12_B__INPUT_CSC_C11_B__SHIFT 0x0
#define INPUT_CSC_C11_C12_B__INPUT_CSC_C12_B_MASK 0xffff0000
#define INPUT_CSC_C11_C12_B__INPUT_CSC_C12_B__SHIFT 0x10
#define INPUT_CSC_C13_C14_B__INPUT_CSC_C13_B_MASK 0xffff
#define INPUT_CSC_C13_C14_B__INPUT_CSC_C13_B__SHIFT 0x0
#define INPUT_CSC_C13_C14_B__INPUT_CSC_C14_B_MASK 0xffff0000
#define INPUT_CSC_C13_C14_B__INPUT_CSC_C14_B__SHIFT 0x10
#define INPUT_CSC_C21_C22_B__INPUT_CSC_C21_B_MASK 0xffff
#define INPUT_CSC_C21_C22_B__INPUT_CSC_C21_B__SHIFT 0x0
#define INPUT_CSC_C21_C22_B__INPUT_CSC_C22_B_MASK 0xffff0000
#define INPUT_CSC_C21_C22_B__INPUT_CSC_C22_B__SHIFT 0x10
#define INPUT_CSC_C23_C24_B__INPUT_CSC_C23_B_MASK 0xffff
#define INPUT_CSC_C23_C24_B__INPUT_CSC_C23_B__SHIFT 0x0
#define INPUT_CSC_C23_C24_B__INPUT_CSC_C24_B_MASK 0xffff0000
#define INPUT_CSC_C23_C24_B__INPUT_CSC_C24_B__SHIFT 0x10
#define INPUT_CSC_C31_C32_B__INPUT_CSC_C31_B_MASK 0xffff
#define INPUT_CSC_C31_C32_B__INPUT_CSC_C31_B__SHIFT 0x0
#define INPUT_CSC_C31_C32_B__INPUT_CSC_C32_B_MASK 0xffff0000
#define INPUT_CSC_C31_C32_B__INPUT_CSC_C32_B__SHIFT 0x10
#define INPUT_CSC_C33_C34_B__INPUT_CSC_C33_B_MASK 0xffff
#define INPUT_CSC_C33_C34_B__INPUT_CSC_C33_B__SHIFT 0x0
#define INPUT_CSC_C33_C34_B__INPUT_CSC_C34_B_MASK 0xffff0000
#define INPUT_CSC_C33_C34_B__INPUT_CSC_C34_B__SHIFT 0x10
#define PRESCALE_CONTROL__PRESCALE_MODE_MASK 0x3
#define PRESCALE_CONTROL__PRESCALE_MODE__SHIFT 0x0
#define PRESCALE_VALUES_R__PRESCALE_BIAS_R_MASK 0xffff
#define PRESCALE_VALUES_R__PRESCALE_BIAS_R__SHIFT 0x0
#define PRESCALE_VALUES_R__PRESCALE_SCALE_R_MASK 0xffff0000
#define PRESCALE_VALUES_R__PRESCALE_SCALE_R__SHIFT 0x10
#define PRESCALE_VALUES_G__PRESCALE_BIAS_G_MASK 0xffff
#define PRESCALE_VALUES_G__PRESCALE_BIAS_G__SHIFT 0x0
#define PRESCALE_VALUES_G__PRESCALE_SCALE_G_MASK 0xffff0000
#define PRESCALE_VALUES_G__PRESCALE_SCALE_G__SHIFT 0x10
#define PRESCALE_VALUES_B__PRESCALE_BIAS_B_MASK 0xffff
#define PRESCALE_VALUES_B__PRESCALE_BIAS_B__SHIFT 0x0
#define PRESCALE_VALUES_B__PRESCALE_SCALE_B_MASK 0xffff0000
#define PRESCALE_VALUES_B__PRESCALE_SCALE_B__SHIFT 0x10
#define COL_MAN_OUTPUT_CSC_CONTROL__OUTPUT_CSC_MODE_MASK 0x7
#define COL_MAN_OUTPUT_CSC_CONTROL__OUTPUT_CSC_MODE__SHIFT 0x0
#define OUTPUT_CSC_C11_C12_A__OUTPUT_CSC_C11_A_MASK 0xffff
#define OUTPUT_CSC_C11_C12_A__OUTPUT_CSC_C11_A__SHIFT 0x0
#define OUTPUT_CSC_C11_C12_A__OUTPUT_CSC_C12_A_MASK 0xffff0000
#define OUTPUT_CSC_C11_C12_A__OUTPUT_CSC_C12_A__SHIFT 0x10
#define OUTPUT_CSC_C13_C14_A__OUTPUT_CSC_C13_A_MASK 0xffff
#define OUTPUT_CSC_C13_C14_A__OUTPUT_CSC_C13_A__SHIFT 0x0
#define OUTPUT_CSC_C13_C14_A__OUTPUT_CSC_C14_A_MASK 0xffff0000
#define OUTPUT_CSC_C13_C14_A__OUTPUT_CSC_C14_A__SHIFT 0x10
#define OUTPUT_CSC_C21_C22_A__OUTPUT_CSC_C21_A_MASK 0xffff
#define OUTPUT_CSC_C21_C22_A__OUTPUT_CSC_C21_A__SHIFT 0x0
#define OUTPUT_CSC_C21_C22_A__OUTPUT_CSC_C22_A_MASK 0xffff0000
#define OUTPUT_CSC_C21_C22_A__OUTPUT_CSC_C22_A__SHIFT 0x10
#define OUTPUT_CSC_C23_C24_A__OUTPUT_CSC_C23_A_MASK 0xffff
#define OUTPUT_CSC_C23_C24_A__OUTPUT_CSC_C23_A__SHIFT 0x0
#define OUTPUT_CSC_C23_C24_A__OUTPUT_CSC_C24_A_MASK 0xffff0000
#define OUTPUT_CSC_C23_C24_A__OUTPUT_CSC_C24_A__SHIFT 0x10
#define OUTPUT_CSC_C31_C32_A__OUTPUT_CSC_C31_A_MASK 0xffff
#define OUTPUT_CSC_C31_C32_A__OUTPUT_CSC_C31_A__SHIFT 0x0
#define OUTPUT_CSC_C31_C32_A__OUTPUT_CSC_C32_A_MASK 0xffff0000
#define OUTPUT_CSC_C31_C32_A__OUTPUT_CSC_C32_A__SHIFT 0x10
#define OUTPUT_CSC_C33_C34_A__OUTPUT_CSC_C33_A_MASK 0xffff
#define OUTPUT_CSC_C33_C34_A__OUTPUT_CSC_C33_A__SHIFT 0x0
#define OUTPUT_CSC_C33_C34_A__OUTPUT_CSC_C34_A_MASK 0xffff0000
#define OUTPUT_CSC_C33_C34_A__OUTPUT_CSC_C34_A__SHIFT 0x10
#define OUTPUT_CSC_C11_C12_B__OUTPUT_CSC_C11_B_MASK 0xffff
#define OUTPUT_CSC_C11_C12_B__OUTPUT_CSC_C11_B__SHIFT 0x0
#define OUTPUT_CSC_C11_C12_B__OUTPUT_CSC_C12_B_MASK 0xffff0000
#define OUTPUT_CSC_C11_C12_B__OUTPUT_CSC_C12_B__SHIFT 0x10
#define OUTPUT_CSC_C13_C14_B__OUTPUT_CSC_C13_B_MASK 0xffff
#define OUTPUT_CSC_C13_C14_B__OUTPUT_CSC_C13_B__SHIFT 0x0
#define OUTPUT_CSC_C13_C14_B__OUTPUT_CSC_C14_B_MASK 0xffff0000
#define OUTPUT_CSC_C13_C14_B__OUTPUT_CSC_C14_B__SHIFT 0x10
#define OUTPUT_CSC_C21_C22_B__OUTPUT_CSC_C21_B_MASK 0xffff
#define OUTPUT_CSC_C21_C22_B__OUTPUT_CSC_C21_B__SHIFT 0x0
#define OUTPUT_CSC_C21_C22_B__OUTPUT_CSC_C22_B_MASK 0xffff0000
#define OUTPUT_CSC_C21_C22_B__OUTPUT_CSC_C22_B__SHIFT 0x10
#define OUTPUT_CSC_C23_C24_B__OUTPUT_CSC_C23_B_MASK 0xffff
#define OUTPUT_CSC_C23_C24_B__OUTPUT_CSC_C23_B__SHIFT 0x0
#define OUTPUT_CSC_C23_C24_B__OUTPUT_CSC_C24_B_MASK 0xffff0000
#define OUTPUT_CSC_C23_C24_B__OUTPUT_CSC_C24_B__SHIFT 0x10
#define OUTPUT_CSC_C31_C32_B__OUTPUT_CSC_C31_B_MASK 0xffff
#define OUTPUT_CSC_C31_C32_B__OUTPUT_CSC_C31_B__SHIFT 0x0
#define OUTPUT_CSC_C31_C32_B__OUTPUT_CSC_C32_B_MASK 0xffff0000
#define OUTPUT_CSC_C31_C32_B__OUTPUT_CSC_C32_B__SHIFT 0x10
#define OUTPUT_CSC_C33_C34_B__OUTPUT_CSC_C33_B_MASK 0xffff
#define OUTPUT_CSC_C33_C34_B__OUTPUT_CSC_C33_B__SHIFT 0x0
#define OUTPUT_CSC_C33_C34_B__OUTPUT_CSC_C34_B_MASK 0xffff0000
#define OUTPUT_CSC_C33_C34_B__OUTPUT_CSC_C34_B__SHIFT 0x10
#define DENORM_CLAMP_CONTROL__DENORM_MODE_MASK 0x3
#define DENORM_CLAMP_CONTROL__DENORM_MODE__SHIFT 0x0
#define DENORM_CLAMP_CONTROL__DENORM_10BIT_OUT_MASK 0x100
#define DENORM_CLAMP_CONTROL__DENORM_10BIT_OUT__SHIFT 0x8
#define DENORM_CLAMP_RANGE_R_CR__RANGE_CLAMP_MAX_R_CR_MASK 0xfff
#define DENORM_CLAMP_RANGE_R_CR__RANGE_CLAMP_MAX_R_CR__SHIFT 0x0
#define DENORM_CLAMP_RANGE_R_CR__RANGE_CLAMP_MIN_R_CR_MASK 0xfff000
#define DENORM_CLAMP_RANGE_R_CR__RANGE_CLAMP_MIN_R_CR__SHIFT 0xc
#define DENORM_CLAMP_RANGE_G_Y__RANGE_CLAMP_MAX_G_Y_MASK 0xfff
#define DENORM_CLAMP_RANGE_G_Y__RANGE_CLAMP_MAX_G_Y__SHIFT 0x0
#define DENORM_CLAMP_RANGE_G_Y__RANGE_CLAMP_MIN_G_Y_MASK 0xfff000
#define DENORM_CLAMP_RANGE_G_Y__RANGE_CLAMP_MIN_G_Y__SHIFT 0xc
#define DENORM_CLAMP_RANGE_B_CB__RANGE_CLAMP_MAX_B_CB_MASK 0xfff
#define DENORM_CLAMP_RANGE_B_CB__RANGE_CLAMP_MAX_B_CB__SHIFT 0x0
#define DENORM_CLAMP_RANGE_B_CB__RANGE_CLAMP_MIN_B_CB_MASK 0xfff000
#define DENORM_CLAMP_RANGE_B_CB__RANGE_CLAMP_MIN_B_CB__SHIFT 0xc
#define COL_MAN_FP_CONVERTED_FIELD__COL_MAN_FP_CONVERTED_FIELD_DATA_MASK 0x3ffff
#define COL_MAN_FP_CONVERTED_FIELD__COL_MAN_FP_CONVERTED_FIELD_DATA__SHIFT 0x0
#define COL_MAN_FP_CONVERTED_FIELD__COL_MAN_FP_CONVERTED_FIELD_INDEX_MASK 0x3f00000
#define COL_MAN_FP_CONVERTED_FIELD__COL_MAN_FP_CONVERTED_FIELD_INDEX__SHIFT 0x14
#define GAMMA_CORR_CONTROL__GAMMA_CORR_MODE_MASK 0x3
#define GAMMA_CORR_CONTROL__GAMMA_CORR_MODE__SHIFT 0x0
#define GAMMA_CORR_LUT_INDEX__GAMMA_CORR_LUT_INDEX_MASK 0xff
#define GAMMA_CORR_LUT_INDEX__GAMMA_CORR_LUT_INDEX__SHIFT 0x0
#define GAMMA_CORR_LUT_DATA__GAMMA_CORR_LUT_DATA_MASK 0x7ffff
#define GAMMA_CORR_LUT_DATA__GAMMA_CORR_LUT_DATA__SHIFT 0x0
#define GAMMA_CORR_LUT_WRITE_EN_MASK__GAMMA_CORR_LUT_WRITE_EN_MASK_MASK 0x7
#define GAMMA_CORR_LUT_WRITE_EN_MASK__GAMMA_CORR_LUT_WRITE_EN_MASK__SHIFT 0x0
#define GAMMA_CORR_CNTLA_START_CNTL__GAMMA_CORR_CNTLA_EXP_REGION_START_MASK 0x3ffff
#define GAMMA_CORR_CNTLA_START_CNTL__GAMMA_CORR_CNTLA_EXP_REGION_START__SHIFT 0x0
#define GAMMA_CORR_CNTLA_START_CNTL__GAMMA_CORR_CNTLA_EXP_REGION_START_SEGMENT_MASK 0x7f00000
#define GAMMA_CORR_CNTLA_START_CNTL__GAMMA_CORR_CNTLA_EXP_REGION_START_SEGMENT__SHIFT 0x14
#define GAMMA_CORR_CNTLA_SLOPE_CNTL__GAMMA_CORR_CNTLA_EXP_REGION_LINEAR_SLOPE_MASK 0x3ffff
#define GAMMA_CORR_CNTLA_SLOPE_CNTL__GAMMA_CORR_CNTLA_EXP_REGION_LINEAR_SLOPE__SHIFT 0x0
#define GAMMA_CORR_CNTLA_END_CNTL1__GAMMA_CORR_CNTLA_EXP_REGION_END_MASK 0xffff
#define GAMMA_CORR_CNTLA_END_CNTL1__GAMMA_CORR_CNTLA_EXP_REGION_END__SHIFT 0x0
#define GAMMA_CORR_CNTLA_END_CNTL2__GAMMA_CORR_CNT