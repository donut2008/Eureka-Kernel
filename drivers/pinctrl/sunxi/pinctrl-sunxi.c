RCE_SEL_OUTPUT_PIX                    = 0x0,
	DCP_CRC_SOURCE_SEL_INPUT_L32                     = 0x1,
	DCP_CRC_SOURCE_SEL_INPUT_H32                     = 0x2,
	DCP_CRC_SOURCE_SEL_OUTPUT_CNTL                   = 0x4,
} DCP_CRC_SOURCE_SEL;
typedef enum DCP_CRC_LINE_SEL {
	DCP_CRC_LINE_SEL_RESERVED                        = 0x0,
	DCP_CRC_LINE_SEL_EVEN                            = 0x1,
	DCP_CRC_LINE_SEL_ODD                             = 0x2,
	DCP_CRC_LINE_SEL_BOTH                            = 0x3,
} DCP_CRC_LINE_SEL;
typedef enum DCP_GRPH_FLIP_RATE {
	DCP_GRPH_FLIP_RATE_1FRAME                        = 0x0,
	DCP_GRPH_FLIP_RATE_2FRAME                        = 0x1,
	DCP_GRPH_FLIP_RATE_3FRAME                        = 0x2,
	DCP_GRPH_FLIP_RATE_4FRAME                        = 0x3,
	DCP_GRPH_FLIP_RATE_5FRAME                        = 0x4,
	DCP_GRPH_FLIP_RATE_6FRAME                        = 0x5,
	DCP_GRPH_FLIP_RATE_7FRAME                        = 0x6,
	DCP_GRPH_FLIP_RATE_8FRAME                        = 0x7,
} DCP_GRPH_FLIP_RATE;
typedef enum DCP_GRPH_FLIP_RATE_ENABLE {
	DCP_GRPH_FLIP_RATE_ENABLE_FALSE                  = 0x0,
	DCP_GRPH_FLIP_RATE_ENABLE_TRUE                   = 0x1,
} DCP_GRPH_FLIP_RATE_ENABLE;
typedef enum DCP_GSL0_EN {
	DCP_GSL0_EN_FALSE                                = 0x0,
	DCP_GSL0_EN_TRUE                                 = 0x1,
} DCP_GSL0_EN;
typedef enum DCP_GSL1_EN {
	DCP_GSL1_EN_FALSE                                = 0x0,
	DCP_GSL1_EN_TRUE                                 = 0x1,
} DCP_GSL1_EN;
typedef enum DCP_GSL2_EN {
	DCP_GSL2_EN_FALSE                                = 0x0,
	DCP_GSL2_EN_TRUE                                 = 0x1,
} DCP_GSL2_EN;
typedef enum DCP_GSL_MASTER_EN {
	DCP_GSL_MASTER_EN_FALSE                          = 0x0,
	DCP_GSL_MASTER_EN_TRUE                           = 0x1,
} DCP_GSL_MASTER_EN;
typedef enum DCP_GSL_XDMA_GROUP {
	DCP_GSL_XDMA_GROUP_VSYNC                         = 0x0,
	DCP_GSL_XDMA_GROUP_HSYNC0                        = 0x1,
	DCP_GSL_XDMA_GROUP_HSYNC1                        = 0x2,
	DCP_GSL_XDMA_GROUP_HSYNC2                        = 0x3,
} DCP_GSL_XDMA_GROUP;
typedef enum DCP_GSL_XDMA_GROUP_UNDERFLOW_EN {
	DCP_GSL_XDMA_GROUP_UNDERFLOW_EN_FALSE            = 0x0,
	DCP_GSL_XDMA_GROUP_UNDERFLOW_EN_TRUE             = 0x1,
} DCP_GSL_XDMA_GROUP_UNDERFLOW_EN;
typedef enum DCP_GSL_SYNC_SOURCE {
	DCP_GSL_SYNC_SOURCE_FLIP                         = 0x0,
	DCP_GSL_SYNC_SOURCE_PHASE0                       = 0x1,
	DCP_GSL_SYNC_SOURCE_RESET                        = 0x2,
	DCP_GSL_SYNC_SOURCE_PHASE1                       = 0x3,
} DCP_GSL_SYNC_SOURCE;
typedef enum DCP_GSL_DELAY_SURFACE_UPDATE_PENDING {
	DCP_GSL_DELAY_SURFACE_UPDATE_PENDING_FALSE       = 0x0,
	DCP_GSL_DELAY_SURFACE_UPDATE_PENDING_TRUE        = 0x1,
} DCP_GSL_DELAY_SURFACE_UPDATE_PENDING;
typedef enum DCP_TEST_DEBUG_WRITE_EN {
	DCP_TEST_DEBUG_WRITE_EN_FALSE                    = 0x0,
	DCP_TEST_DEBUG_WRITE_EN_TRUE                     = 0x1,
} DCP_TEST_DEBUG_WRITE_EN;
typedef enum DCP_GRPH_STEREOSYNC_FLIP_EN {
	DCP_GRPH_STEREOSYNC_FLIP_EN_FALSE                = 0x0,
	DCP_GRPH_STEREOSYNC_FLIP_EN_TRUE                 = 0x1,
} DCP_GRPH_STEREOSYNC_FLIP_EN;
typedef enum DCP_GRPH_STEREOSYNC_FLIP_MODE {
	DCP_GRPH_STEREOSYNC_FLIP_MODE_FLIP               = 0x0,
	DCP_GRPH_STEREOSYNC_FLIP_MODE_PHASE0             = 0x1,
	DCP_GRPH_STEREOSYNC_FLIP_MODE_RESET              = 0x2,
	DCP_GRPH_STEREOSYNC_FLIP_MODE_PHASE1             = 0x3,
} DCP_GRPH_STEREOSYNC_FLIP_MODE;
typedef enum DCP_GRPH_STEREOSYNC_SELECT_DISABLE {
	DCP_GRPH_STEREOSYNC_SELECT_DISABLE_FALSE         = 0x0,
	DCP_GRPH_STEREOSYNC_SELECT_DISABLE_TRUE          = 0x1,
} DCP_GRPH_STEREOSYNC_SELECT_DISABLE;
typedef enum DCP_GRPH_ROTATION_ANGLE {
	DCP_GRPH_ROTATION_ANGLE_0                        = 0x0,
	DCP_GRPH_ROTATION_ANGLE_90                       = 0x1,
	DCP_GRPH_ROTATION_ANGLE_180                      = 0x2,
	DCP_GRPH_ROTATION_ANGLE_270                      = 0x3,
} DCP_GRPH_ROTATION_ANGLE;
typedef enum DCP_GRPH_XDMA_CACHE_UNDERFLOW_CNT_EN {
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_CNT_EN_FALSE       = 0x0,
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_CNT_EN_TRUE        = 0x1,
} DCP_GRPH_XDMA_CACHE_UNDERFLOW_CNT_EN;
typedef enum DCP_GRPH_XDMA_CACHE_UNDERFLOW_CNT_MODE {
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_CNT_MODE_RELY_NUM  = 0x0,
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_CNT_MODE_RELY_ENABLE= 0x1,
} DCP_GRPH_XDMA_CACHE_UNDERFLOW_CNT_MODE;
typedef enum DCP_GRPH_REGAMMA_MODE {
	DCP_GRPH_REGAMMA_MODE_BYPASS                     = 0x0,
	DCP_GRPH_REGAMMA_MODE_SRGB                       = 0x1,
	DCP_GRPH_REGAMMA_MODE_XVYCC                      = 0x2,
	DCP_GRPH_REGAMMA_MODE_PROGA                      = 0x3,
	DCP_GRPH_REGAMMA_MODE_PROGB                      = 0x4,
} DCP_GRPH_REGAMMA_MODE;
typedef enum DCP_ALPHA_ROUND_TRUNC_MODE {
	DCP_ALPHA_ROUND_TRUNC_MODE_ROUND                 = 0x0,
	DCP_ALPHA_ROUND_TRUNC_MODE_TRUNC                 = 0x1,
} DCP_ALPHA_ROUND_TRUNC_MODE;
typedef enum DCP_CURSOR_ALPHA_BLND_ENA {
	DCP_CURSOR_ALPHA_BLND_ENA_FALSE                  = 0x0,
	DCP_CURSOR_ALPHA_BLND_ENA_TRUE                   = 0x1,
} DCP_CURSOR_ALPHA_BLND_ENA;
typedef enum DCP_GRPH_XDMA_CACHE_UNDERFLOW_FRAME_MASK {
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_FRAME_MASK_FALSE   = 0x0,
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_FRAME_MASK_TRUE    = 0x1,
} DCP_GRPH_XDMA_CACHE_UNDERFLOW_FRAME_MASK;
typedef enum DCP_GRPH_XDMA_CACHE_UNDERFLOW_FRAME_ACK {
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_FRAME_ACK_FALSE    = 0x0,
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_FRAME_ACK_TRUE     = 0x1,
} DCP_GRPH_XDMA_CACHE_UNDERFLOW_FRAME_ACK;
typedef enum DCP_GRPH_XDMA_CACHE_UNDERFLOW_INT_MASK {
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_INT_MASK_FALSE     = 0x0,
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_INT_MASK_TRUE      = 0x1,
} DCP_GRPH_XDMA_CACHE_UNDERFLOW_INT_MASK;
typedef enum DCP_GRPH_XDMA_CACHE_UNDERFLOW_INT_ACK {
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_INT_ACK_FALSE      = 0x0,
	DCP_GRPH_XDMA_CACHE_UNDERFLOW_INT_ACK_TRUE       = 0x1,
} DCP_GRPH_XDMA_CACHE_UNDERFLOW_INT_ACK;
typedef enum DCP_GRPH_SURFACE_COUNTER_EN {
	DCP_GRPH_SURFACE_COUNTER_EN_DISABLE              = 0x0,
	DCP_GRPH_SURFACE_COUNTER_EN_ENABLE               = 0x1,
} DCP_GRPH_SURFACE_COUNTER_EN;
typedef enum DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT {
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_0          = 0x0,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_1          = 0x1,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_2          = 0x2,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_3          = 0x3,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_4          = 0x4,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_5          = 0x5,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_6          = 0x6,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_7          = 0x7,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_8          = 0x8,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_9          = 0x9,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_10         = 0xa,
	DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT_11         = 0xb,
} DCP_GRPH_SURFACE_COUNTER_EVENT_SELECT;
typedef enum DCP_GRPH_SURFACE_COUNTER_ERR_WRAP_OCCURED {
	DCP_GRPH_SURFACE_COUNTER_ERR_WRAP_OCCURED_NO     = 0x0,
	DCP_GRPH_SURFACE_COUNTER_ERR_WRAP_OCCURED_YES    = 0x1,
} DCP_GRPH_SURFACE_COUNTER_ERR_WRAP_OCCURED;
typedef enum HDMI_KEEPOUT_MODE {
	HDMI_KEEPOUT_0_650PIX_AFTER_VSYNC                = 0x0,
	HDMI_KEEPOUT_509_650PIX_AFTER_VSYNC              = 0x1,
} HDMI_KEEPOUT_MODE;
typedef enum HDMI_CLOCK_CHANNEL_RATE {
	HDMI_CLOCK_CHANNEL_FREQ_EQUAL_TO_CHAR_RATE       = 0x0,
	HDMI_CLOCK_CHANNEL_FREQ_QUARTER_TO_CHAR_RATE     = 0x1,
} HDMI_CLOCK_CHANNEL_RATE;
typedef enum HDMI_NO_EXTRA_NULL_PACKET_FILLED {
	HDMI_EXTRA_NULL_PACKET_FILLED_ENABLE             = 0x0,
	HDMI_EXTRA_NULL_PACKET_FILLED_DISABLE            = 0x1,
} HDMI_NO_EXTRA_NULL_PACKET_FILLED;
typedef enum HDMI_PACKET_GEN_VERSION {
	HDMI_PACKET_GEN_VERSION_OLD                      = 0x0,
	HDMI_PACKET_GEN_VERSION_NEW                      = 0x1,
} HDMI_PACKET_GEN_VERSION;
typedef enum HDMI_ERROR_ACK {
	HDMI_ERROR_ACK_INT                               = 0x0,
	HDMI_ERROR_NOT_ACK                               = 0x1,
} HDMI_ERROR_ACK;
typedef enum HDMI_ERROR_MASK {
	HDMI_ERROR_MASK_INT                              = 0x0,
	HDMI_ERROR_NOT_MASK                              = 0x1,
} HDMI_ERROR_MASK;
typedef enum HDMI_DEEP_COLOR_DEPTH {
	HDMI_DEEP_COLOR_DEPTH_24BPP                      = 0x0,
	HDMI_DEEP_COLOR_DEPTH_30BPP                      = 0x1,
	HDMI_DEEP_COLOR_DEPTH_36BPP                      = 0x2,
	HDMI_DEEP_COLOR_DEPTH_RESERVED                   = 0x3,
} HDMI_DEEP_COLOR_DEPTH;
typedef enum HDMI_AUDIO_DELAY_EN {
	HDMI_AUDIO_DELAY_DISABLE                         = 0x0,
	HDMI_AUDIO_DELAY_58CLK                           = 0x1,
	HDMI_AUDIO_DELAY_56CLK                           = 0x2,
	HDMI_AUDIO_DELAY_RESERVED                        = 0x3,
} HDMI_AUDIO_DELAY_EN;
typedef enum HDMI_AUDIO_SEND_MAX_PACKETS {
	HDMI_NOT_SEND_MAX_AUDIO_PACKETS                  = 0x0,
	HDMI_SEND_MAX_AUDIO_PACKETS                      = 0x1,
} HDMI_AUDIO_SEND_MAX_PACKETS;
typedef enum HDMI_ACR_SEND {
	HDMI_ACR_NOT_SEND                                = 0x0,
	HDMI_ACR_PKT_SEND                                = 0x1,
} HDMI_ACR_SEND;
typedef enum HDMI_ACR_CONT {
	HDMI_ACR_CONT_DISABLE                            = 0x0,
	HDMI_ACR_CONT_ENABLE                             = 0x1,
} HDMI_ACR_CONT;
typedef enum HDMI_ACR_SELECT {
	HDMI_ACR_SELECT_HW                               = 0x0,
	HDMI_ACR_SELECT_32K                              = 0x1,
	HDMI_ACR_SELECT_44K                              = 0x2,
	HDMI_ACR_SELECT_48K                              = 0x3,
} HDMI_ACR_SELECT;
typedef enum HDMI_ACR_SOURCE {
	HDMI_ACR_SOURCE_HW                               = 0x0,
	HDMI_ACR_SOURCE_SW                               = 0x1,
} HDMI_ACR_SOURCE;
typedef enum HDMI_ACR_N_MULTIPLE {
	HDMI_ACR_0_MULTIPLE_RESERVED                     = 0x0,
	HDMI_ACR_1_MULTIPLE                              = 0x1,
	HDMI_ACR_2_MULTIPLE                              = 0x2,
	HDMI_ACR_3_MULTIPLE_RESERVED                     = 0x3,
	HDMI_ACR_4_MULTIPLE                              = 0x4,
	HDMI_ACR_5_MULTIPLE_RESERVED                     = 0x5,
	HDMI_ACR_6_MULTIPLE_RESERVED                     = 0x6,
	HDMI_ACR_7_MULTIPLE_RESERVED                     = 0x7,
} HDMI_ACR_N_MULTIPLE;
typedef enum HDMI_ACR_AUDIO_PRIORITY {
	HDMI_ACR_PKT_HIGH_PRIORITY_THAN_AUDIO_SAMPLE     = 0x0,
	HDMI_AUDIO_SAMPLE_HIGH_PRIORITY_THAN_ACR_PKT     = 0x1,
} HDMI_ACR_AUDIO_PRIORITY;
typedef enum HDMI_NULL_SEND {
	HDMI_NULL_NOT_SEND                               = 0x0,
	HDMI_NULL_PKT_SEND                               = 0x1,
} HDMI_NULL_SEND;
typedef enum HDMI_GC_SEND {
	HDMI_GC_NOT_SEND                                 = 0x0,
	HDMI_GC_PKT_SEND                                 = 0x1,
} HDMI_GC_SEND;
typedef enum HDMI_GC_CONT {
	HDMI_GC_CONT_DISABLE                             = 0x0,
	HDMI_GC_CONT_ENABLE                              = 0x1,
} HDMI_GC_CONT;
typedef enum HDMI_ISRC_SEND {
	HDMI_ISRC_NOT_SEND                               = 0x0,
	HDMI_ISRC_PKT_SEND                               = 0x1,
} HDMI_ISRC_SEND;
typedef enum HDMI_ISRC_CONT {
	HDMI_ISRC_CONT_DISABLE                           = 0x0,
	HDMI_ISRC_CONT_ENABLE                            = 0x1,
} HDMI_ISRC_CONT;
typedef enum HDMI_AVI_INFO_SEND {
	HDMI_AVI_INFO_NOT_SEND                           = 0x0,
	HDMI_AVI_INFO_PKT_SEND                           = 0x1,
} HDMI_AVI_INFO_SEND;
typedef enum HDMI_AVI_INFO_CONT {
	HDMI_AVI_INFO_CONT_DISABLE                       = 0x0,
	HDMI_AVI_INFO_CONT_ENABLE                        = 0x1,
} HDMI_AVI_INFO_CONT;
typedef enum HDMI_AUDIO_INFO_SEND {
	HDMI_AUDIO_INFO_NOT_SEND                         = 0x0,
	HDMI_AUDIO_INFO_PKT_SEND                         = 0x1,
} HDMI_AUDIO_INFO_SEND;
typedef enum HDMI_AUDIO_INFO_CONT {
	HDMI_AUDIO_INFO_CONT_DISABLE                     = 0x0,
	HDMI_AUDIO_INFO_CONT_ENABLE                      = 0x1,
} HDMI_AUDIO_INFO_CONT;
typedef enum HDMI_MPEG_INFO_SEND {
	HDMI_MPEG_INFO_NOT_SEND                          = 0x0,
	HDMI_MPEG_INFO_PKT_SEND                          = 0x1,
} HDMI_MPEG_INFO_SEND;
typedef enum HDMI_MPEG_INFO_CONT {
	HDMI_MPEG_INFO_CONT_DISABLE                      = 0x0,
	HDMI_MPEG_INFO_CONT_ENABLE                       = 0x1,
} HDMI_MPEG_INFO_CONT;
typedef enum HDMI_GENERIC0_SEND {
	HDMI_GENERIC0_NOT_SEND                           = 0x0,
	HDMI_GENERIC0_PKT_SEND                           = 0x1,
} HDMI_GENERIC0_SEND;
typedef enum HDMI_GENERIC0_CONT {
	HDMI_GENERIC0_CONT_DISABLE                       = 0x0,
	HDMI_GENERIC0_CONT_ENABLE                        = 0x1,
} HDMI_GENERIC0_CONT;
typedef enum HDMI_GENERIC1_SEND {
	HDMI_GENERIC1_NOT_SEND                           = 0x0,
	HDMI_GENERIC1_PKT_SEND                           = 0x1,
} HDMI_GENERIC1_SEND;
typedef enum HDMI_GENERIC1_CONT {
	HDMI_GENERIC1_CONT_DISABLE                       = 0x0,
	HDMI_GENERIC1_CONT_ENABLE                        = 0x1,
} HDMI_GENERIC1_CONT;
typedef enum HDMI_GC_AVMUTE_CONT {
	HDMI_GC_AVMUTE_CONT_DISABLE                      = 0x0,
	HDMI_GC_AVMUTE_CONT_ENABLE                       = 0x1,
} HDMI_GC_AVMUTE_CONT;
typedef enum HDMI_PACKING_PHASE_OVERRIDE {
	HDMI_PACKING_PHASE_SET_BY_HW                     = 0x0,
	HDMI_PACKING_PHASE_SET_BY_SW                     = 0x1,
} HDMI_PACKING_PHASE_OVERRIDE;
typedef enum HDMI_GENERIC2_SEND {
	HDMI_GENERIC2_NOT_SEND                           = 0x0,
	HDMI_GENERIC2_PKT_SEND                           = 0x1,
} HDMI_GENERIC2_SEND;
typedef enum HDMI_GENERIC2_CONT {
	HDMI_GENERIC2_CONT_DISABLE                       = 0x0,
	HDMI_GENERIC2_CONT_ENABLE                        = 0x1,
} HDMI_GENERIC2_CONT;
typedef enum HDMI_GENERIC3_SEND {
	HDMI_GENERIC3_NOT_SEND                           = 0x0,
	HDMI_GENERIC3_PKT_SEND                           = 0x1,
} HDMI_GENERIC3_SEND;
typedef enum HDMI_GENERIC3_CONT {
	HDMI_GENERIC3_CONT_DISABLE                       = 0x0,
	HDMI_GENERIC3_CONT_ENABLE                        = 0x1,
} HDMI_GENERIC3_CONT;
typedef enum TMDS_PIXEL_ENCODING {
	TMDS_PIXEL_ENCODING_444                          = 0x0,
	TMDS_PIXEL_ENCODING_422                          = 0x1,
} TMDS_PIXEL_ENCODING;
typedef enum TMDS_COLOR_FORMAT {
	TMDS_COLOR_FORMAT__24BPP__TWIN30BPP_MSB__DUAL48BPP= 0x0,
	TMDS_COLOR_FORMAT_TWIN30BPP_LSB                  = 0x1,
	TMDS_COLOR_FORMAT_DUAL30BPP                      = 0x2,
	TMDS_COLOR_FORMAT_RESERVED                       = 0x3,
} TMDS_COLOR_FORMAT;
typedef enum TMDS_STEREOSYNC_CTL_SEL_REG {
	TMDS_STEREOSYNC_CTL0                             = 0x0,
	TMDS_STEREOSYNC_CTL1                             = 0x1,
	TMDS_STEREOSYNC_CTL2                             = 0x2,
	TMDS_STEREOSYNC_CTL3                             = 0x3,
} TMDS_STEREOSYNC_CTL_SEL_REG;
typedef enum TMDS_CTL0_DATA_SEL {
	TMDS_CTL0_DATA_SEL0_RESERVED                     = 0x0,
	TMDS_CTL0_DATA_SEL1_DISPLAY_ENABLE               = 0x1,
	TMDS_CTL0_DATA_SEL2_VSYNC                        = 0x2,
	TMDS_CTL0_DATA_SEL3_RESERVED                     = 0x3,
	TMDS_CTL0_DATA_SEL4_HSYNC                        = 0x4,
	TMDS_CTL0_DATA_SEL5_SEL7_RESERVED                = 0x5,
	TMDS_CTL0_DATA_SEL8_RANDOM_DATA                  = 0x6,
	TMDS_CTL0_DATA_SEL9_SEL15_RANDOM_DATA            = 0x7,
} TMDS_CTL0_DATA_SEL;
typedef enum TMDS_CTL0_DATA_DELAY {
	TMDS_CTL0_DATA_DELAY_0PIX                        = 0x0,
	TMDS_CTL0_DATA_DELAY_1PIX                        = 0x1,
	TMDS_CTL0_DATA_DELAY_2PIX                        = 0x2,
	TMDS_CTL0_DATA_DELAY_3PIX                        = 0x3,
	TMDS_CTL0_DATA_DELAY_4PIX                        = 0x4,
	TMDS_CTL0_DATA_DELAY_5PIX                        = 0x5,
	TMDS_CTL0_DATA_DELAY_6PIX                        = 0x6,
	TMDS_CTL0_DATA_DELAY_7PIX                        = 0x7,
} TMDS_CTL0_DATA_DELAY;
typedef enum TMDS_CTL0_DATA_INVERT {
	TMDS_CTL0_DATA_NORMAL                            = 0x0,
	TMDS_CTL0_DATA_INVERT_EN                         = 0x1,
} TMDS_CTL0_DATA_INVERT;
typedef enum TMDS_CTL0_DATA_MODULATION {
	TMDS_CTL0_DATA_MODULATION_DISABLE                = 0x0,
	TMDS_CTL0_DATA_MODULATION_BIT0                   = 0x1,
	TMDS_CTL0_DATA_MODULATION_BIT1                   = 0x2,
	TMDS_CTL0_DATA_MODULATION_BIT2                   = 0x3,
} TMDS_CTL0_DATA_MODULATION;
typedef enum TMDS_CTL0_PATTERN_OUT_EN {
	TMDS_CTL0_PATTERN_OUT_DISABLE                    = 0x0,
	TMDS_CTL0_PATTERN_OUT_ENABLE                     = 0x1,
} TMDS_CTL0_PATTERN_OUT_EN;
typedef enum TMDS_CTL1_DATA_SEL {
	TMDS_CTL1_DATA_SEL0_RESERVED                     = 0x0,
	TMDS_CTL1_DATA_SEL1_DISPLAY_ENABLE               = 0x1,
	TMDS_CTL1_DATA_SEL2_VSYNC                        = 0x2,
	TMDS_CTL1_DATA_SEL3_RESERVED                     = 0x3,
	TMDS_CTL1_DATA_SEL4_HSYNC                        = 0x4,
	TMDS_CTL1_DATA_SEL5_SEL7_RESERVED                = 0x5,
	TMDS_CTL1_DATA_SEL8_BLANK_TIME                   = 0x6,
	TMDS_CTL1_DATA_SEL9_SEL15_RESERVED               = 0x7,
} TMDS_CTL1_DATA_SEL;
typedef enum TMDS_CTL1_DATA_DELAY {
	TMDS_CTL1_DATA_DELAY_0PIX                        = 0x0,
	TMDS_CTL1_DATA_DELAY_1PIX                        = 0x1,
	TMDS_CTL1_DATA_DELAY_2PIX                        = 0x2,
	TMDS_CTL1_DATA_DELAY_3PIX                        = 0x3,
	TMDS_CTL1_DATA_DELAY_4PIX                        = 0x4,
	TMDS_CTL1_DATA_DELAY_5PIX                        = 0x5,
	TMDS_CTL1_DATA_DELAY_6PIX                        = 0x6,
	TMDS_CTL1_DATA_DELAY_7PIX                        = 0x7,
} TMDS_CTL1_DATA_DELAY;
typedef enum TMDS_CTL1_DATA_INVERT {
	TMDS_CTL1_DATA_NORMAL                            = 0x0,
	TMDS_CTL1_DATA_INVERT_EN                         = 0x1,
} TMDS_CTL1_DATA_INVERT;
typedef enum TMDS_CTL1_DATA_MODULATION {
	TMDS_CTL1_DATA_MODULATION_DISABLE                = 0x0,
	TMDS_CTL1_DATA_MODULATION_BIT0                   = 0x1,
	TMDS_CTL1_DATA_MODULATION_BIT1                   = 0x2,
	TMDS_CTL1_DATA_MODULATION_BIT2                   = 0x3,
} TMDS_CTL1_DATA_MODULATION;
typedef enum TMDS_CTL1_PATTERN_OUT_EN {
	TMDS_CTL1_PATTERN_OUT_DISABLE                    = 0x0,
	TMDS_CTL1_PATTERN_OUT_ENABLE                     = 0x1,
} TMDS_CTL1_PATTERN_OUT_EN;
typedef enum TMDS_CTL2_DATA_SEL {
	TMDS_CTL2_DATA_SEL0_RESERVED                     = 0x0,
	TMDS_CTL2_DATA_SEL1_DISPLAY_ENABLE               = 0x1,
	TMDS_CTL2_DATA_SEL2_VSYNC                        = 0x2,
	TMDS_CTL2_DATA_SEL3_RESERVED                     = 0x3,
	TMDS_CTL2_DATA_SEL4_HSYNC                        = 0x4,
	TMDS_CTL2_DATA_SEL5_SEL7_RESERVED                = 0x5,
	TMDS_CTL2_DATA_SEL8_BLANK_TIME                   = 0x6,
	TMDS_CTL2_DATA_SEL9_SEL15_RESERVED               = 0x7,
} TMDS_CTL2_DATA_SEL;
typedef enum TMDS_CTL2_DATA_DELAY {
	TMDS_CTL2_DATA_DELAY_0PIX                        = 0x0,
	TMDS_CTL2_DATA_DELAY_1PIX                        = 0x1,
	TMDS_CTL2_DATA_DELAY_2PIX                        = 0x2,
	TMDS_CTL2_DATA_DELAY_3PIX                        = 0x3,
	TMDS_CTL2_DATA_DELAY_4PIX                        = 0x4,
	TMDS_CTL2_DATA_DELAY_5PIX                        = 0x5,
	TMDS_CTL2_DATA_DELAY_6PIX                        = 0x6,
	TMDS_CTL2_DATA_DELAY_7PIX                        = 0x7,
} TMDS_CTL2_DATA_DELAY;
typedef enum TMDS_CTL2_DATA_INVERT {
	TMDS_CTL2_DATA_NORMAL                            = 0x0,
	TMDS_CTL2_DATA_INVERT_EN                         = 0x1,
} TMDS_CTL2_DATA_INVERT;
typedef enum TMDS_CTL2_DATA_MODULATION {
	TMDS_CTL2_DATA_MODULATION_DISABLE                = 0x0,
	TMDS_CTL2_DATA_MODULATION_BIT0                   = 0x1,
	TMDS_CTL2_DATA_MODULATION_BIT1                   = 0x2,
	TMDS_CTL2_DATA_MODULATION_BIT2                   = 0x3,
} TMDS_CTL2_DATA_MODULATION;
typedef enum TMDS_CTL2_PATTERN_OUT_EN {
	TMDS_CTL2_PATTERN_OUT_DISABLE                    = 0x0,
	TMDS_CTL2_PATTERN_OUT_ENABLE                     = 0x1,
} TMDS_CTL2_PATTERN_OUT_EN;
typedef enum TMDS_CTL3_DATA_DELAY {
	TMDS_CTL3_DATA_DELAY_0PIX                        = 0x0,
	TMDS_CTL3_DATA_DELAY_1PIX                        = 0x1,
	TMDS_CTL3_DATA_DELAY_2PIX                        = 0x2,
	TMDS_CTL3_DATA_DELAY_3PIX                        = 0x3,
	TMDS_CTL3_DATA_DELAY_4PIX                        = 0x4,
	TMDS_CTL3_DATA_DELAY_5PIX                        = 0x5,
	TMDS_CTL3_DATA_DELAY_6PIX                        = 0x6,
	TMDS_CTL3_DATA_DELAY_7PIX                        = 0x7,
} TMDS_CTL3_DATA_DELAY;
typedef enum TMDS_CTL3_DATA_INVERT {
	TMDS_CTL3_DATA_NORMAL                            = 0x0,
	TMDS_CTL3_DATA_INVERT_EN                         = 0x1,
} TMDS_CTL3_DATA_INVERT;
typedef enum TMDS_CTL3_DATA_MODULATION {
	TMDS_CTL3_DATA_MODULATION_DISABLE                = 0x0,
	TMDS_CTL3_DATA_MODULATION_BIT0                   = 0x1,
	TMDS_CTL3_DATA_MODULATION_BIT1                   = 0x2,
	TMDS_CTL3_DATA_MODULATION_BIT2                   = 0x3,
} TMDS_CTL3_DATA_MODULATION;
typedef enum TMDS_CTL3_PATTERN_OUT_EN {
	TMDS_CTL3_PATTERN_OUT_DISABLE                    = 0x0,
	TMDS_CTL3_PATTERN_OUT_ENABLE                     = 0x1,
} TMDS_CTL3_PATTERN_OUT_EN;
typedef enum TMDS_CTL3_DATA_SEL {
	TMDS_CTL3_DATA_SEL0_RESERVED                     = 0x0,
	TMDS_CTL3_DATA_SEL1_DISPLAY_ENABLE               = 0x1,
	TMDS_CTL3_DATA_SEL2_VSYNC                        = 0x2,
	TMDS_CTL3_DATA_SEL3_RESERVED                     = 0x3,
	TMDS_CTL3_DATA_SEL4_HSYNC                        = 0x4,
	TMDS_CTL3_DATA_SEL5_SEL7_RESERVED                = 0x5,
	TMDS_CTL3_DATA_SEL8_BLANK_TIME                   = 0x6,
	TMDS_CTL3_DATA_SEL9_SEL15_RESERVED               = 0x7,
} TMDS_CTL3_DATA_SEL;
typedef enum DIG_FE_CNTL_SOURCE_SELECT {
	DIG_FE_SOURCE_FROM_FMT0                          = 0x0,
	DIG_FE_SOURCE_FROM_FMT1                          = 0x1,
	DIG_FE_SOURCE_FROM_FMT2                          = 0x2,
	DIG_FE_SOURCE_FROM_FMT3                          = 0x3,
} DIG_FE_CNTL_SOURCE_SELECT;
typedef enum DIG_FE_CNTL_STEREOSYNC_SELECT {
	DIG_FE_STEREOSYNC_FROM_FMT0                      = 0x0,
	DIG_FE_STEREOSYNC_FROM_FMT1                      = 0x1,
	DIG_FE_STEREOSYNC_FROM_FMT2                      = 0x2,
	DIG_FE_STEREOSYNC_FROM_FMT3                      = 0x3,
} DIG_FE_CNTL_STEREOSYNC_SELECT;
typedef enum DIG_FIFO_READ_CLOCK_SRC {
	DIG_FIFO_READ_CLOCK_SRC_FROM_DCCG                = 0x0,
	DIG_FIFO_READ_CLOCK_SRC_FROM_DISPLAY_PIPE        = 0x1,
} DIG_FIFO_READ_CLOCK_SRC;
typedef enum DIG_OUTPUT_CRC_CNTL_LINK_SEL {
	DIG_OUTPUT_CRC_ON_LINK0                          = 0x0,
	DIG_OUTPUT_CRC_ON_LINK1                          = 0x1,
} DIG_OUTPUT_CRC_CNTL_LINK_SEL;
typedef enum DIG_OUTPUT_CRC_DATA_SEL {
	DIG_OUTPUT_CRC_FOR_FULLFRAME                     = 0x0,
	DIG_OUTPUT_CRC_FOR_ACTIVEONLY                    = 0x1,
	DIG_OUTPUT_CRC_FOR_VBI                           = 0x2,
	DIG_OUTPUT_CRC_FOR_AUDIO                         = 0x3,
} DIG_OUTPUT_CRC_DATA_SEL;
typedef enum DIG_TEST_PATTERN_TEST_PATTERN_OUT_EN {
	DIG_IN_NORMAL_OPERATION                          = 0x0,
	DIG_IN_DEBUG_MODE                                = 0x1,
} DIG_TEST_PATTERN_TEST_PATTERN_OUT_EN;
typedef enum DIG_TEST_PATTERN_HALF_CLOCK_PATTERN_SEL {
	DIG_10BIT_TEST_PATTERN                           = 0x0,
	DIG_ALTERNATING_TEST_PATTERN                     = 0x1,
} DIG_TEST_PATTERN_HALF_CLOCK_PATTERN_SEL;
typedef enum DIG_TEST_PATTERN_RANDOM_PATTERN_OUT_EN {
	DIG_TEST_PATTERN_NORMAL                          = 0x0,
	DIG_TEST_PATTERN_RANDOM                          = 0x1,
} DIG_TEST_PATTERN_RANDOM_PATTERN_OUT_EN;
typedef enum DIG_TEST_PATTERN_RANDOM_PATTERN_RESET {
	DIG_RANDOM_PATTERN_ENABLED                       = 0x0,
	DIG_RANDOM_PATTERN_RESETED                       = 0x1,
} DIG_TEST_PATTERN_RANDOM_PATTERN_RESET;
typedef enum DIG_TEST_PATTERN_EXTERNAL_RESET_EN {
	DIG_TEST_PATTERN_EXTERNAL_RESET_ENABLE           = 0x0,
	DIG_TEST_PATTERN_EXTERNAL_RESET_BY_EXT_SIG       = 0x1,
} DIG_TEST_PATTERN_EXTERNAL_RESET_EN;
typedef enum DIG_RANDOM_PATTERN_SEED_RAN_PAT {
	DIG_RANDOM_PATTERN_SEED_RAN_PAT_ALL_PIXELS       = 0x0,
	DIG_RANDOM_PATTERN_SEED_RAN_PAT_DE_HIGH          = 0x1,
} DIG_RANDOM_PATTERN_SEED_RAN_PAT;
typedef enum DIG_FIFO_STATUS_USE_OVERWRITE_LEVEL {
	DIG_FIFO_USE_OVERWRITE_LEVEL                     = 0x0,
	DIG_FIFO_USE_CAL_AVERAGE_LEVEL                   = 0x1,
} DIG_FIFO_STATUS_USE_OVERWRITE_LEVEL;
typedef enum DIG_FIFO_ERROR_ACK {
	DIG_FIFO_ERROR_ACK_INT                           = 0x0,
	DIG_FIFO_ERROR_NOT_ACK                           = 0x1,
} DIG_FIFO_ERROR_ACK;
typedef enum DIG_FIFO_STATUS_FORCE_RECAL_AVERAGE {
	DIG_FIFO_NOT_FORCE_RECAL_AVERAGE                 = 0x0,
	DIG_FIFO_FORCE_RECAL_AVERAGE_LEVEL               = 0x1,
} DIG_FIFO_STATUS_FORCE_RECAL_AVERAGE;
typedef enum DIG_FIFO_STATUS_FORCE_RECOMP_MINMAX {
	DIG_FIFO_NOT_FORCE_RECOMP_MINMAX                 = 0x0,
	DIG_FIFO_FORCE_RECOMP_MINMAX                     = 0x1,
} DIG_FIFO_STATUS_FORCE_RECOMP_MINMAX;
typedef enum DIG_DISPCLK_SWITCH_CNTL_SWITCH_POINT {
	DIG_DISPCLK_SWITCH_AT_EARLY_VBLANK               = 0x0,
	DIG_DISPCLK_SWITCH_AT_FIRST_HSYNC                = 0x1,
} DIG_DISPCLK_SWITCH_CNTL_SWITCH_POINT;
typedef enum DIG_DISPCLK_SWITCH_ALLOWED_INT_ACK {
	DIG_DISPCLK_SWITCH_ALLOWED_ACK_INT               = 0x0,
	DIG_DISPCLK_SWITCH_ALLOWED_INT_NOT_ACK           = 0x1,
} DIG_DISPCLK_SWITCH_ALLOWED_INT_ACK;
typedef enum DIG_DISPCLK_SWITCH_ALLOWED_INT_MASK {
	DIG_DISPCLK_SWITCH_ALLOWED_MASK_INT              = 0x0,
	DIG_DISPCLK_SWITCH_ALLOWED_INT_UNMASK            = 0x1,
} DIG_DISPCLK_SWITCH_ALLOWED_INT_MASK;
typedef enum AFMT_INTERRUPT_STATUS_CHG_MASK {
	AFMT_INTERRUPT_DISABLE                           = 0x0,
	AFMT_INTERRUPT_ENABLE                            = 0x1,
} AFMT_INTERRUPT_STATUS_CHG_MASK;
typedef enum HDMI_GC_AVMUTE {
	HDMI_GC_AVMUTE_SET                               = 0x0,
	HDMI_GC_AVMUTE_UNSET                             = 0x1,
} HDMI_GC_AVMUTE;
typedef enum HDMI_DEFAULT_PAHSE {
	HDMI_DEFAULT_PHASE_IS_0                          = 0x0,
	HDMI_DEFAULT_PHASE_IS_1                          = 0x1,
} HDMI_DEFAULT_PAHSE;
typedef enum AFMT_AUDIO_PACKET_CONTROL2_AUDIO_LAYOUT_OVRD {
	AFMT_AUDIO_LAYOUT_DETERMINED_BY_AZ_AUDIO_CHANNEL_STATUS= 0x0,
	AFMT_AUDIO_LAYOUT_OVRD_BY_REGISTER               = 0x1,
} AFMT_AUDIO_PACKET_CONTROL2_AUDIO_LAYOUT_OVRD;
typedef enum AUDIO_LAYOUT_SELECT {
	AUDIO_LAYOUT_0                                   = 0x0,
	AUDIO_LAYOUT_1                                   = 0x1,
} AUDIO_LAYOUT_SELECT;
typedef enum AFMT_AUDIO_CRC_CONTROL_CONT {
	AFMT_AUDIO_CRC_ONESHOT                           = 0x0,
	AFMT_AUDIO_CRC_AUTO_RESTART                      = 0x1,
} AFMT_AUDIO_CRC_CONTROL_CONT;
typedef enum AFMT_AUDIO_CRC_CONTROL_SOURCE {
	AFMT_AUDIO_CRC_SOURCE_FROM_FIFO_INPUT            = 0x0,
	AFMT_AUDIO_CRC_SOURCE_