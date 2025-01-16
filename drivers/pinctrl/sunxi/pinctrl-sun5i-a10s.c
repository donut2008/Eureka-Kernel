ENABLE                        = 0xf,
} DCIOCHIP_MASK_4BIT;
typedef enum DCIOCHIP_ENABLE_4BIT {
	DCIOCHIP_4BIT_DISABLE                            = 0x0,
	DCIOCHIP_4BIT_ENABLE                             = 0xf,
} DCIOCHIP_ENABLE_4BIT;
typedef enum DCIOCHIP_MASK_5BIT {
	DCIOCHIP_MASIK_5BIT_DISABLE                      = 0x0,
	DCIOCHIP_MASIK_5BIT_ENABLE                       = 0x1f,
} DCIOCHIP_MASK_5BIT;
typedef enum DCIOCHIP_ENABLE_5BIT {
	DCIOCHIP_5BIT_DISABLE                            = 0x0,
	DCIOCHIP_5BIT_ENABLE                             = 0x1f,
} DCIOCHIP_ENABLE_5BIT;
typedef enum DCIOCHIP_MASK_2BIT {
	DCIOCHIP_MASK_2BIT_DISABLE                       = 0x0,
	DCIOCHIP_MASK_2BIT_ENABLE                        = 0x3,
} DCIOCHIP_MASK_2BIT;
typedef enum DCIOCHIP_ENABLE_2BIT {
	DCIOCHIP_2BIT_DISABLE                            = 0x0,
	DCIOCHIP_2BIT_ENABLE                             = 0x3,
} DCIOCHIP_ENABLE_2BIT;
typedef enum DCIOCHIP_REF_27_SRC_SEL {
	DCIOCHIP_REF_27_SRC_SEL_XTAL_DIVIDER             = 0x0,
	DCIOCHIP_REF_27_SRC_SEL_DISP_CLKIN2_DIVIDER      = 0x1,
	DCIOCHIP_REF_27_SRC_SEL_XTAL_BYPASS              = 0x2,
	DCIOCHIP_REF_27_SRC_SEL_DISP_CLKIN2_BYPASS       = 0x3,
} DCIOCHIP_REF_27_SRC_SEL;
typedef enum DCIOCHIP_DVO_VREFPON {
	DCIOCHIP_DVO_VREFPON_DISABLE                     = 0x0,
	DCIOCHIP_DVO_VREFPON_ENABLE                      = 0x1,
} DCIOCHIP_DVO_VREFPON;
typedef enum DCIOCHIP_DVO_VREFSEL {
	DCIOCHIP_DVO_VREFSEL_ONCHIP                      = 0x0,
	DCIOCHIP_DVO_VREFSEL_EXTERNAL                    = 0x1,
} DCIOCHIP_DVO_VREFSEL;
typedef enum DCP_GRPH_ENABLE {
	DCP_GRPH_ENABLE_FALSE                            = 0x0,
	DCP_GRPH_ENABLE_TRUE                             = 0x1,
} DCP_GRPH_ENABLE;
typedef enum DCP_GRPH_KEYER_ALPHA_SEL {
	DCP_GRPH_KEYER_ALPHA_SEL_FALSE                   = 0x0,
	DCP_GRPH_KEYER_ALPHA_SEL_TRUE                    = 0x1,
} DCP_GRPH_KEYER_ALPHA_SEL;
typedef enum DCP_GRPH_DEPTH {
	DCP_GRPH_DEPTH_8BPP                              = 0x0,
	DCP_GRPH_DEPTH_16BPP                             = 0x1,
	DCP_GRPH_DEPTH_32BPP                             = 0x2,
	DCP_GRPH_DEPTH_64BPP                             = 0x3,
} DCP_GRPH_DEPTH;
typedef enum DCP_GRPH_NUM_BANKS {
	DCP_GRPH_NUM_BANKS_2BANK                         = 0x0,
	DCP_GRPH_NUM_BANKS_4BANK                         = 0x1,
	DCP_GRPH_NUM_BANKS_8BANK                         = 0x2,
	DCP_GRPH_NUM_BANKS_16BANK                        = 0x3,
} DCP_GRPH_NUM_BANKS;
typedef enum DCP_GRPH_BANK_WIDTH {
	DCP_GRPH_BANK_WIDTH_1                            = 0x0,
	DCP_GRPH_BANK_WIDTH_2                            = 0x1,
	DCP_GRPH_BANK_WIDTH_4                            = 0x2,
	DCP_GRPH_BANK_WIDTH_8                            = 0x3,
} DCP_GRPH_BANK_WIDTH;
typedef enum DCP_GRPH_FORMAT {
	DCP_GRPH_FORMAT_8BPP                             = 0x0,
	DCP_GRPH_FORMAT_16BPP                            = 0x1,
	DCP_GRPH_FORMAT_32BPP                            = 0x2,
	DCP_GRPH_FORMAT_64BPP                            = 0x3,
} DCP_GRPH_FORMAT;
typedef enum DCP_GRPH_BANK_HEIGHT {
	DCP_GRPH_BANK_HEIGHT_1                           = 0x0,
	DCP_GRPH_BANK_HEIGHT_2                           = 0x1,
	DCP_GRPH_BANK_HEIGHT_4                           = 0x2,
	DCP_GRPH_BANK_HEIGHT_8                           = 0x3,
} DCP_GRPH_BANK_HEIGHT;
typedef enum DCP_GRPH_TILE_SPLIT {
	DCP_GRPH_TILE_SPLIT_64B                          = 0x0,
	DCP_GRPH_TILE_SPLIT_128B                         = 0x1,
	DCP_GRPH_TILE_SPLIT_256B                         = 0x2,
	DCP_GRPH_TILE_SPLIT_512B                         = 0x3,
	DCP_GRPH_TILE_SPLIT_1B                           = 0x4,
	DCP_GRPH_TILE_SPLIT_2B                           = 0x5,
	DCP_GRPH_TILE_SPLIT_4B                           = 0x6,
} DCP_GRPH_TILE_SPLIT;
typedef enum DCP_GRPH_ADDRESS_TRANSLATION_ENABLE {
	DCP_GRPH_ADDRESS_TRANSLATION_ENABLE_FALSE        = 0x0,
	DCP_GRPH_ADDRESS_TRANSLATION_ENABLE_TRUE         = 0x1,
} DCP_GRPH_ADDRESS_TRANSLATION_ENABLE;
typedef enum DCP_GRPH_PRIVILEGED_ACCESS_ENABLE {
	DCP_GRPH_PRIVILEGED_ACCESS_ENABLE_FALSE          = 0x0,
	DCP_GRPH_PRIVILEGED_ACCESS_ENABLE_TRUE           = 0x1,
} DCP_GRPH_PRIVILEGED_ACCESS_ENABLE;
typedef enum DCP_GRPH_MACRO_TILE_ASPECT {
	DCP_GRPH_MACRO_TILE_ASPECT_1                     = 0x0,
	DCP_GRPH_MACRO_TILE_ASPECT_2                     = 0x1,
	DCP_GRPH_MACRO_TILE_ASPECT_4                     = 0x2,
	DCP_GRPH_MACRO_TILE_ASPECT_8                     = 0x3,
} DCP_GRPH_MACRO_TILE_ASPECT;
typedef enum DCP_GRPH_ARRAY_MODE {
	DCP_GRPH_ARRAY_MODE_0                            = 0x0,
	DCP_GRPH_ARRAY_MODE_1                            = 0x1,
	DCP_GRPH_ARRAY_MODE_2                            = 0x2,
	DCP_GRPH_ARRAY_MODE_3                            = 0x3,
	DCP_GRPH_ARRAY_MODE_4                            = 0x4,
	DCP_GRPH_ARRAY_MODE_7                            = 0x7,
	DCP_GRPH_ARRAY_MODE_12                           = 0xc,
	DCP_GRPH_ARRAY_MODE_13                           = 0xd,
} DCP_GRPH_ARRAY_MODE;
typedef enum DCP_GRPH_MICRO_TILE_MODE {
	DCP_GRPH_MICRO_TILE_MODE_0                       = 0x0,
	DCP_GRPH_MICRO_TILE_MODE_1                       = 0x1,
	DCP_GRPH_MICRO_TILE_MODE_2                       = 0x2,
	DCP_GRPH_MICRO_TILE_MODE_3                       = 0x3,
} DCP_GRPH_MICRO_TILE_MODE;
typedef enum DCP_GRPH_COLOR_EXPANSION_MODE {
	DCP_GRPH_COLOR_EXPANSION_MODE_DEXP               = 0x0,
	DCP_GRPH_COLOR_EXPANSION_MODE_ZEXP               = 0x1,
} DCP_GRPH_COLOR_EXPANSION_MODE;
typedef enum DCP_GRPH_LUT_10BIT_BYPASS_EN {
	DCP_GRPH_LUT_10BIT_BYPASS_EN_FALSE               = 0x0,
	DCP_GRPH_LUT_10BIT_BYPASS_EN_TRUE                = 0x1,
} DCP_GRPH_LUT_10BIT_BYPASS_EN;
typedef enum DCP_GRPH_LUT_10BIT_BYPASS_DBL_BUF_EN {
	DCP_GRPH_LUT_10BIT_BYPASS_DBL_BUF_EN_FALSE       = 0x0,
	DCP_GRPH_LUT_10BIT_BYPASS_DBL_BUF_EN_TRUE        = 0x1,
} DCP_GRPH_LUT_10BIT_BYPASS_DBL_BUF_EN;
typedef enum DCP_GRPH_ENDIAN_SWAP {
	DCP_GRPH_ENDIAN_SWAP_NONE                        = 0x0,
	DCP_GRPH_ENDIAN_SWAP_8IN16                       = 0x1,
	DCP_GRPH_ENDIAN_SWAP_8IN32                       = 0x2,
	DCP_GRPH_ENDIAN_SWAP_8IN64                       = 0x3,
} DCP_GRPH_ENDIAN_SWAP;
typedef enum DCP_GRPH_RED_CROSSBAR {
	DCP_GRPH_RED_CROSSBAR_FROM_R                     = 0x0,
	DCP_GRPH_RED_CROSSBAR_FROM_G                     = 0x1,
	DCP_GRPH_RED_CROSSBAR_FROM_B                     = 0x2,
	DCP_GRPH_RED_CROSSBAR_FROM_A                     = 0x3,
} DCP_GRPH_RED_CROSSBAR;
typedef enum DCP_GRPH_GREEN_CROSSBAR {
	DCP_GRPH_GREEN_CROSSBAR_FROM_G                   = 0x0,
	DCP_GRPH_GREEN_CROSSBAR_FROM_B                   = 0x1,
	DCP_GRPH_GREEN_CROSSBAR_FROM_A                   = 0x2,
	DCP_GRPH_GREEN_CROSSBAR_FROM_R                   = 0x3,
} DCP_GRPH_GREEN_CROSSBAR;
typedef enum DCP_GRPH_BLUE_CROSSBAR {
	DCP_GRPH_BLUE_CROSSBAR_FROM_B                    = 0x0,
	DCP_GRPH_BLUE_CROSSBAR_FROM_A                    = 0x1,
	DCP_GRPH_BLUE_CROSSBAR_FROM_R                    = 0x2,
	DCP_GRPH_BLUE_CROSSBAR_FROM_G                    = 0x3,
} DCP_GRPH_BLUE_CROSSBAR;
typedef enum DCP_GRPH_ALPHA_CROSSBAR {
	DCP_GRPH_ALPHA_CROSSBAR_FROM_A                   = 0x0,
	DCP_GRPH_ALPHA_CROSSBAR_FROM_R                   = 0x1,
	DCP_GRPH_ALPHA_CROSSBAR_FROM_G                   = 0x2,
	DCP_GRPH_ALPHA_CROSSBAR_FROM_B                   = 0x3,
} DCP_GRPH_ALPHA_CROSSBAR;
typedef enum DCP_GRPH_PRIMARY_DFQ_ENABLE {
	DCP_GRPH_PRIMARY_DFQ_ENABLE_FALSE                = 0x0,
	DCP_GRPH_PRIMARY_DFQ_ENABLE_TRUE                 = 0x1,
} DCP_GRPH_PRIMARY_DFQ_ENABLE;
typedef enum DCP_GRPH_SECONDARY_DFQ_ENABLE {
	DCP_GRPH_SECONDARY_DFQ_ENABLE_FALSE              = 0x0,
	DCP_GRPH_SECONDARY_DFQ_ENABLE_TRUE               = 0x1,
} DCP_GRPH_SECONDARY_DFQ_ENABLE;
typedef enum DCP_GRPH_INPUT_GAMMA_MODE {
	DCP_GRPH_INPUT_GAMMA_MODE_LUT                    = 0x0,
	DCP_GRPH_INPUT_GAMMA_MODE_BYPASS                 = 0x1,
} DCP_GRPH_INPUT_GAMMA_MODE;
typedef enum DCP_GRPH_MODE_UPDATE_PENDING {
	DCP_GRPH_MODE_UPDATE_PENDING_FALSE               = 0x0,
	DCP_GRPH_MODE_UPDATE_PENDING_TRUE                = 0x1,
} DCP_GRPH_MODE_UPDATE_PENDING;
typedef enum DCP_GRPH_MODE_UPDATE_TAKEN {
	DCP_GRPH_MODE_UPDATE_TAKEN_FALSE                 = 0x0,
	DCP_GRPH_MODE_UPDATE_TAKEN_TRUE                  = 0x1,
} DCP_GRPH_MODE_UPDATE_TAKEN;
typedef enum DCP_GRPH_SURFACE_UPDATE_PENDING {
	DCP_GRPH_SURFACE_UPDATE_PENDING_FALSE            = 0x0,
	DCP_GRPH_SURFACE_UPDATE_PENDING_TRUE             = 0x1,
} DCP_GRPH_SURFACE_UPDATE_PENDING;
typedef enum DCP_GRPH_SURFACE_UPDATE_TAKEN {
	DCP_GRPH_SURFACE_UPDATE_TAKEN_FALSE              = 0x0,
	DCP_GRPH_SURFACE_UPDATE_TAKEN_TRUE               = 0x1,
} DCP_GRPH_SURFACE_UPDATE_TAKEN;
typedef enum DCP_GRPH_SURFACE_XDMA_PENDING_ENABLE {
	DCP_GRPH_SURFACE_XDMA_PENDING_ENABLE_FALSE       = 0x0,
	DCP_GRPH_SURFACE_XDMA_PENDING_ENABLE_TRUE        = 0x1,
} DCP_GRPH_SURFACE_XDMA_PENDING_ENABLE;
typedef enum DCP_GRPH_UPDATE_LOCK {
	DCP_GRPH_UPDATE_LOCK_FALSE                       = 0x0,
	DCP_GRPH_UPDATE_LOCK_TRUE                        = 0x1,
} DCP_GRPH_UPDATE_LOCK;
typedef enum DCP_GRPH_SURFACE_IGNORE_UPDATE_LOCK {
	DCP_GRPH_SURFACE_IGNORE_UPDATE_LOCK_FALSE        = 0x0,
	DCP_GRPH_SURFACE_IGNORE_UPDATE_LOCK_TRUE         = 0x1,
} DCP_GRPH_SURFACE_IGNORE_UPDATE_LOCK;
typedef enum DCP_GRPH_MODE_DISABLE_MULTIPLE_UPDATE {
	DCP_GRPH_MODE_DISABLE_MULTIPLE_UPDATE_FALSE      = 0x0,
	DCP_GRPH_MODE_DISABLE_MULTIPLE_UPDATE_TRUE       = 0x1,
} DCP_GRPH_MODE_DISABLE_MULTIPLE_UPDATE;
typedef enum DCP_GRPH_SURFACE_DISABLE_MULTIPLE_UPDATE {
	DCP_GRPH_SURFACE_DISABLE_MULTIPLE_UPDATE_FALSE   = 0x0,
	DCP_GRPH_SURFACE_DISABLE_MULTIPLE_UPDATE_TRUE    = 0x1,
} DCP_GRPH_SURFACE_DISABLE_MULTIPLE_UPDATE;
typedef enum DCP_GRPH_SURFACE_UPDATE_H_RETRACE_EN {
	DCP_GRPH_SURFACE_UPDATE_H_RETRACE_EN_FALSE       = 0x0,
	DCP_GRPH_SURFACE_UPDATE_H_RETRACE_EN_TRUE        = 0x1,
} DCP_GRPH_SURFACE_UPDATE_H_RETRACE_EN;
typedef enum DCP_GRPH_XDMA_SUPER_AA_EN {
	DCP_GRPH_XDMA_SUPER_AA_EN_FALSE                  = 0x0,
	DCP_GRPH_XDMA_SUPER_AA_EN_TRUE                   = 0x1,
} DCP_GRPH_XDMA_SUPER_AA_EN;
typedef enum DCP_GRPH_DFQ_RESET {
	DCP_GRPH_DFQ_RESET_FALSE                         = 0x0,
	DCP_GRPH_DFQ_RESET_TRUE                          = 0x1,
} DCP_GRPH_DFQ_RESET;
typedef enum DCP_GRPH_DFQ_SIZE {
	DCP_GRPH_DFQ_SIZE_DEEP1                          = 0x0,
	DCP_GRPH_DFQ_SIZE_DEEP2                          = 0x1,
	DCP_GRPH_DFQ_SIZE_DEEP3                          = 0x2,
	DCP_GRPH_DFQ_SIZE_DEEP4                          = 0x3,
	DCP_GRPH_DFQ_SIZE_DEEP5                          = 0x4,
	DCP_GRPH_DFQ_SIZE_DEEP6                          = 0x5,
	DCP_GRPH_DFQ_SIZE_DEEP7                          = 0x6,
	DCP_GRPH_DFQ_SIZE_DEEP8                          = 0x7,
} DCP_GRPH_DFQ_SIZE;
typedef enum DCP_GRPH_DFQ_MIN_FREE_ENTRIES {
	DCP_GRPH_DFQ_MIN_FREE_ENTRIES_1                  = 0x0,
	DCP_GRPH_DFQ_MIN_FREE_ENTRIES_2                  = 0x1,
	DCP_GRPH_DFQ_MIN_FREE_ENTRIES_3                  = 0x2,
	DCP_GRPH_DFQ_MIN_FREE_ENTRIES_4                  = 0x3,
	DCP_GRPH_DFQ_MIN_FREE_ENTRIES_5                  = 0x4,
	DCP_GRPH_DFQ_MIN_FREE_ENTRIES_6                  = 0x5,
	DCP_GRPH_DFQ_MIN_FREE_ENTRIES_7                  = 0x6,
	DCP_GRPH_DFQ_MIN_FREE_ENTRIES_8                  = 0x7,
} DCP_GRPH_DFQ_MIN_FREE_ENTRIES;
typedef enum DCP_GRPH_DFQ_RESET_ACK {
	DCP_GRPH_DFQ_RESET_ACK_FALSE                     = 0x0,
	DCP_GRPH_DFQ_RESET_ACK_TRUE                      = 0x1,
} DCP_GRPH_DFQ_RESET_ACK;
typedef enum DCP_GRPH_PFLIP_INT_CLEAR {
	DCP_GRPH_PFLIP_INT_CLEAR_FALSE                   = 0x0,
	DCP_GRPH_PFLIP_INT_CLEAR_TRUE                    = 0x1,
} DCP_GRPH_PFLIP_INT_CLEAR;
typedef enum DCP_GRPH_PFLIP_INT_MASK {
	DCP_GRPH_PFLIP_INT_MASK_FALSE                    = 0x0,
	DCP_GRPH_PFLIP_INT_MASK_TRUE                     = 0x1,
} DCP_GRPH_PFLIP_INT_MASK;
typedef enum DCP_GRPH_PFLIP_INT_TYPE {
	DCP_GRPH_PFLIP_INT_TYPE_LEGACY_LEVEL             = 0x0,
	DCP_GRPH_PFLIP_INT_TYPE_PULSE                    = 0x1,
} DCP_GRPH_PFLIP_INT_TYPE;
typedef enum DCP_GRPH_PRESCALE_SELECT {
	DCP_GRPH_PRESCALE_SELECT_FIXED                   = 0x0,
	DCP_GRPH_PRESCALE_SELECT_FLOATING                = 0x1,
} DCP_GRPH_PRESCALE_SELECT;
typedef enum DCP_GRPH_PRESCALE_R_SIGN {
	DCP_GRPH_PRESCALE_R_SIGN_UNSIGNED                = 0x0,
	DCP_GRPH_PRESCALE_R_SIGN_SIGNED                  = 0x1,
} DCP_GRPH_PRESCALE_R_SIGN;
typedef enum DCP_GRPH_PRESCALE_G_SIGN {
	DCP_GRPH_PRESCALE_G_SIGN_UNSIGNED                = 0x0,
	DCP_GRPH_PRESCALE_G_SIGN_SIGNED                  = 0x1,
} DCP_GRPH_PRESCALE_G_SIGN;
typedef enum DCP_GRPH_PRESCALE_B_SIGN {
	DCP_GRPH_PRESCALE_B_SIGN_UNSIGNED                = 0x0,
	DCP_GRPH_PRESCALE_B_SIGN_SIGNED                  = 0x1,
} DCP_GRPH_PRESCALE_B_SIGN;
typedef enum DCP_GRPH_PRESCALE_BYPASS {
	DCP_GRPH_PRESCALE_BYPASS_FALSE                   = 0x0,
	DCP_GRPH_PRESCALE_BYPASS_TRUE                    = 0x1,
} DCP_GRPH_PRESCALE_BYPASS;
typedef enum DCP_INPUT_CSC_GRPH_MODE {
	DCP_INPUT_CSC_GRPH_MODE_BYPASS                   = 0x0,
	DCP_INPUT_CSC_GRPH_MODE_INPUT_CSC_COEF           = 0x1,
	DCP_INPUT_CSC_GRPH_MODE_SHARED_COEF              = 0x2,
	DCP_INPUT_CSC_GRPH_MODE_RESERVED                 = 0x3,
} DCP_INPUT_CSC_GRPH_MODE;
typedef enum DCP_OUTPUT_CSC_GRPH_MODE {
	DCP_OUTPUT_CSC_GRPH_MODE_BYPASS                  = 0x0,
	DCP_OUTPUT_CSC_GRPH_MODE_RGB                     = 0x1,
	DCP_OUTPUT_CSC_GRPH_MODE_YCBCR601                = 0x2,
	DCP_OUTPUT_CSC_GRPH_MODE_YCBCR709                = 0x3,
	DCP_OUTPUT_CSC_GRPH_MODE_OUTPUT_CSC_COEF         = 0x4,
	DCP_OUTPUT_CSC_GRPH_MODE_SHARED_COEF             = 0x5,
	DCP_OUTPUT_CSC_GRPH_MODE_RESERVED0               = 0x6,
	DCP_OUTPUT_CSC_GRPH_MODE_RESERVED1               = 0x7,
} DCP_OUTPUT_CSC_GRPH_MODE;
typedef enum DCP_DENORM_MODE {
	DCP_DENORM_MODE_UNITY                            = 0x0,
	DCP_DENORM_MODE_6BIT                             = 0x1,
	DCP_DENORM_MODE_8BIT                             = 0x2,
	DCP_DENORM_MODE_10BIT                            = 0x3,
	DCP_DENORM_MODE_11BIT                            = 0x4,
	DCP_DENORM_MODE_12BIT                            = 0x5,
	DCP_DENORM_MODE_RESERVED0                        = 0x6,
	DCP_DENORM_MODE_RESERVED1                        = 0x7,
} DCP_DENORM_MODE;
typedef enum DCP_DENORM_14BIT_OUT {
	DCP_DENORM_14BIT_OUT_FALSE                       = 0x0,
	DCP_DENORM_14BIT_OUT_TRUE                        = 0x1,
} DCP_DENORM_14BIT_OUT;
typedef enum DCP_OUT_ROUND_TRUNC_MODE {
	DCP_OUT_ROUND_TRUNC_MODE_TRUNCATE_12             = 0x0,
	DCP_OUT_ROUND_TRUNC_MODE_TRUNCATE_11             = 0x1,
	DCP_OUT_ROUND_TRUNC_MODE_TRUNCATE_10             = 0x2,
	DCP_OUT_ROUND_TRUNC_MODE_TRUNCATE_9              = 0x3,
	DCP_OUT_ROUND_TRUNC_MODE_TRUNCATE_8              = 0x4,
	DCP_OUT_ROUND_TRUNC_MODE_TRUNCATE_RESERVED       = 0x5,
	DCP_OUT_ROUND_TRUNC_MODE_TRUNCATE_14             = 0x6,
	DCP_OUT_ROUND_TRUNC_MODE_TRUNCATE_13             = 0x7,
	DCP_OUT_ROUND_TRUNC_MODE_ROUND_12                = 0x8,
	DCP_OUT_ROUND_TRUNC_MODE_ROUND_11                = 0x9,
	DCP_OUT_ROUND_TRUNC_MODE_ROUND_10                = 0xa,
	DCP_OUT_ROUND_TRUNC_MODE_ROUND_9                 = 0xb,
	DCP_OUT_ROUND_TRUNC_MODE_ROUND_8                 = 0xc,
	DCP_OUT_ROUND_TRUNC_MODE_ROUND_RESERVED          = 0xd,
	DCP_OUT_ROUND_TRUNC_MODE_ROUND_14                = 0xe,
	DCP_OUT_ROUND_TRUNC_MODE_ROUND_13                = 0xf,
} DCP_OUT_ROUND_TRUNC_MODE;
typedef enum DCP_KEY_MODE {
	DCP_KEY_MODE_ALPHA0                              = 0x0,
	DCP_KEY_MODE_ALPHA1                              = 0x1,
	DCP_KEY_MODE_IN_RANGE_ALPHA1                     = 0x2,
	DCP_KEY_MODE_IN_RANGE_ALPHA0                     = 0x3,
} DCP_KEY_MODE;
typedef enum DCP_GRPH_DEGAMMA_MODE {
	DCP_GRPH_DEGAMMA_MODE_BYPASS                     = 0x0,
	DCP_GRPH_DEGAMMA_MODE_ROMA                       = 0x1,
	DCP_GRPH_DEGAMMA_MODE_ROMB                       = 0x2,
	DCP_GRPH_DEGAMMA_MODE_RESERVED                   = 0x3,
} DCP_GRPH_DEGAMMA_MODE;
typedef enum DCP_CURSOR2_DEGAMMA_MODE {
	DCP_CURSOR2_DEGAMMA_MODE_BYPASS                  = 0x0,
	DCP_CURSOR2_DEGAMMA_MODE_ROMA                    = 0x1,
	DCP_CURSOR2_DEGAMMA_MODE_ROMB                    = 0x2,
	DCP_CURSOR2_DEGAMMA_MODE_RESERVED                = 0x3,
} DCP_CURSOR2_DEGAMMA_MODE;
typedef enum DCP_CURSOR_DEGAMMA_MODE {
	DCP_CURSOR_DEGAMMA_MODE_BYPASS                   = 0x0,
	DCP_CURSOR_DEGAMMA_MODE_ROMA                     = 0x1,
	DCP_CURSOR_DEGAMMA_MODE_ROMB                     = 0x2,
	DCP_CURSOR_DEGAMMA_MODE_RESERVED                 = 0x3,
} DCP_CURSOR_DEGAMMA_MODE;
typedef enum DCP_GRPH_GAMUT_REMAP_MODE {
	DCP_GRPH_GAMUT_REMAP_MODE_BYPASS                 = 0x0,
	DCP_GRPH_GAMUT_REMAP_MODE_ROMA                   = 0x1,
	DCP_GRPH_GAMUT_REMAP_MODE_ROMB                   = 0x2,
	DCP_GRPH_GAMUT_REMAP_MODE_RESERVED               = 0x3,
} DCP_GRPH_GAMUT_REMAP_MODE;
typedef enum DCP_SPATIAL_DITHER_EN {
	DCP_SPATIAL_DITHER_EN_FALSE                      = 0x0,
	DCP_SPATIAL_DITHER_EN_TRUE                       = 0x1,
} DCP_SPATIAL_DITHER_EN;
typedef enum DCP_SPATIAL_DITHER_MODE {
	DCP_SPATIAL_DITHER_MODE_BYPASS                   = 0x0,
	DCP_SPATIAL_DITHER_MODE_ROMA                     = 0x1,
	DCP_SPATIAL_DITHER_MODE_ROMB                     = 0x2,
	DCP_SPATIAL_DITHER_MODE_RESERVED                 = 0x3,
} DCP_SPATIAL_DITHER_MODE;
typedef enum DCP_SPATIAL_DITHER_DEPTH {
	DCP_SPATIAL_DITHER_DEPTH_30BPP                   = 0x0,
	DCP_SPATIAL_DITHER_DEPTH_24BPP                   = 0x1,
	DCP_SPATIAL_DITHER_DEPTH_36BPP                   = 0x2,
	DCP_SPATIAL_DITHER_DEPTH_UNDEFINED               = 0x3,
} DCP_SPATIAL_DITHER_DEPTH;
typedef enum DCP_FRAME_RANDOM_ENABLE {
	DCP_FRAME_RANDOM_ENABLE_FALSE                    = 0x0,
	DCP_FRAME_RANDOM_ENABLE_TRUE                     = 0x1,
} DCP_FRAME_RANDOM_ENABLE;
typedef enum DCP_RGB_RANDOM_ENABLE {
	DCP_RGB_RANDOM_ENABLE_FALSE                      = 0x0,
	DCP_RGB_RANDOM_ENABLE_TRUE                       = 0x1,
} DCP_RGB_RANDOM_ENABLE;
typedef enum DCP_HIGHPASS_RANDOM_ENABLE {
	DCP_HIGHPASS_RANDOM_ENABLE_FALSE                 = 0x0,
	DCP_HIGHPASS_RANDOM_ENABLE_TRUE                  = 0x1,
} DCP_HIGHPASS_RANDOM_ENABLE;
typedef enum DCP_CURSOR_EN {
	DCP_CURSOR_EN_FALSE                              = 0x0,
	DCP_CURSOR_EN_TRUE                               = 0x1,
} DCP_CURSOR_EN;
typedef enum DCP_CUR_INV_TRANS_CLAMP {
	DCP_CUR_INV_TRANS_CLAMP_FALSE                    = 0x0,
	DCP_CUR_INV_TRANS_CLAMP_TRUE                     = 0x1,
} DCP_CUR_INV_TRANS_CLAMP;
typedef enum DCP_CURSOR_MODE {
	DCP_CURSOR_MODE_MONO_2BPP                        = 0x0,
	DCP_CURSOR_MODE_24BPP_1BIT                       = 0x1,
	DCP_CURSOR_MODE_24BPP_8BIT_PREMULTI              = 0x2,
	DCP_CURSOR_MODE_24BPP_8BIT_UNPREMULTI            = 0x3,
} DCP_CURSOR_MODE;
typedef enum DCP_CURSOR_2X_MAGNIFY {
	DCP_CURSOR_2X_MAGNIFY_FALSE                      = 0x0,
	DCP_CURSOR_2X_MAGNIFY_TRUE                       = 0x1,
} DCP_CURSOR_2X_MAGNIFY;
typedef enum DCP_CURSOR_FORCE_MC_ON {
	DCP_CURSOR_FORCE_MC_ON_FALSE                     = 0x0,
	DCP_CURSOR_FORCE_MC_ON_TRUE                      = 0x1,
} DCP_CURSOR_FORCE_MC_ON;
typedef enum DCP_CURSOR_URGENT_CONTROL {
	DCP_CURSOR_URGENT_CONTROL_MODE_0                 = 0x0,
	DCP_CURSOR_URGENT_CONTROL_MODE_1                 = 0x1,
	DCP_CURSOR_URGENT_CONTROL_MODE_2                 = 0x2,
	DCP_CURSOR_URGENT_CONTROL_MODE_3                 = 0x3,
	DCP_CURSOR_URGENT_CONTROL_MODE_4                 = 0x4,
} DCP_CURSOR_URGENT_CONTROL;
typedef enum DCP_CURSOR_UPDATE_PENDING {
	DCP_CURSOR_UPDATE_PENDING_FALSE                  = 0x0,
	DCP_CURSOR_UPDATE_PENDING_TRUE                   = 0x1,
} DCP_CURSOR_UPDATE_PENDING;
typedef enum DCP_CURSOR_UPDATE_TAKEN {
	DCP_CURSOR_UPDATE_TAKEN_FALSE                    = 0x0,
	DCP_CURSOR_UPDATE_TAKEN_TRUE                     = 0x1,
} DCP_CURSOR_UPDATE_TAKEN;
typedef enum DCP_CURSOR_UPDATE_LOCK {
	DCP_CURSOR_UPDATE_LOCK_FALSE                     = 0x0,
	DCP_CURSOR_UPDATE_LOCK_TRUE                      = 0x1,
} DCP_CURSOR_UPDATE_LOCK;
typedef enum DCP_CURSOR_DISABLE_MULTIPLE_UPDATE {
	DCP_CURSOR_DISABLE_MULTIPLE_UPDATE_FALSE         = 0x0,
	DCP_CURSOR_DISABLE_MULTIPLE_UPDATE_TRUE          = 0x1,
} DCP_CURSOR_DISABLE_MULTIPLE_UPDATE;
typedef enum DCP_CURSOR_UPDATE_STEREO_MODE {
	DCP_CURSOR_UPDATE_STEREO_MODE_BOTH               = 0x0,
	DCP_CURSOR_UPDATE_STEREO_MODE_SECONDARY_ONLY     = 0x1,
	DCP_CURSOR_UPDATE_STEREO_MODE_UNDEFINED          = 0x2,
	DCP_CURSOR_UPDATE_STEREO_MODE_PRIMARY_ONLY       = 0x3,
} DCP_CURSOR_UPDATE_STEREO_MODE;
typedef enum DCP_CURSOR2_EN {
	DCP_CURSOR2_EN_FALSE                             = 0x0,
	DCP_CURSOR2_EN_TRUE                              = 0x1,
} DCP_CURSOR2_EN;
typedef enum DCP_CUR2_INV_TRANS_CLAMP {
	DCP_CUR2_INV_TRANS_CLAMP_FALSE                   = 0x0,
	DCP_CUR2_INV_TRANS_CLAMP_TRUE                    = 0x1,
} DCP_CUR2_INV_TRANS_CLAMP;
typedef enum DCP_CURSOR2_MODE {
	DCP_CURSOR2_MODE_MONO_2BPP                       = 0x0,
	DCP_CURSOR2_MODE_24BPP_1BIT                      = 0x1,
	DCP_CURSOR2_MODE_24BPP_8BIT_PREMULTI             = 0x2,
	DCP_CURSOR2_MODE_24BPP_8BIT_UNPREMULTI           = 0x3,
} DCP_CURSOR2_MODE;
typedef enum DCP_CURSOR2_2X_MAGNIFY {
	DCP_CURSOR2_2X_MAGNIFY_FALSE                     = 0x0,
	DCP_CURSOR2_2X_MAGNIFY_TRUE                      = 0x1,
} DCP_CURSOR2_2X_MAGNIFY;
typedef enum DCP_CURSOR2_FORCE_MC_ON {
	DCP_CURSOR2_FORCE_MC_ON_FALSE                    = 0x0,
	DCP_CURSOR2_FORCE_MC_ON_TRUE                     = 0x1,
} DCP_CURSOR2_FORCE_MC_ON;
typedef enum DCP_CURSOR2_URGENT_CONTROL {
	DCP_CURSOR2_URGENT_CONTROL_MODE_0                = 0x0,
	DCP_CURSOR2_URGENT_CONTROL_MODE_1                = 0x1,
	DCP_CURSOR2_URGENT_CONTROL_MODE_2                = 0x2,
	DCP_CURSOR2_URGENT_CONTROL_MODE_3                = 0x3,
	DCP_CURSOR2_URGENT_CONTROL_MODE_4                = 0x4,
} DCP_CURSOR2_URGENT_CONTROL;
typedef enum DCP_CURSOR2_UPDATE_PENDING {
	DCP_CURSOR2_UPDATE_PENDING_FALSE                 = 0x0,
	DCP_CURSOR2_UPDATE_PENDING_TRUE                  = 0x1,
} DCP_CURSOR2_UPDATE_PENDING;
typedef enum DCP_CURSOR2_UPDATE_TAKEN {
	DCP_CURSOR2_UPDATE_TAKEN_FALSE                   = 0x0,
	DCP_CURSOR2_UPDATE_TAKEN_TRUE                    = 0x1,
} DCP_CURSOR2_UPDATE_TAKEN;
typedef enum DCP_CURSOR2_UPDATE_LOCK {
	DCP_CURSOR2_UPDATE_LOCK_FALSE                    = 0x0,
	DCP_CURSOR2_UPDATE_LOCK_TRUE                     = 0x1,
} DCP_CURSOR2_UPDATE_LOCK;
typedef enum DCP_CURSOR2_DISABLE_MULTIPLE_UPDATE {
	DCP_CURSOR2_DISABLE_MULTIPLE_UPDATE_FALSE        = 0x0,
	DCP_CURSOR2_DISABLE_MULTIPLE_UPDATE_TRUE         = 0x1,
} DCP_CURSOR2_DISABLE_MULTIPLE_UPDATE;
typedef enum DCP_CURSOR2_UPDATE_STEREO_MODE {
	DCP_CURSOR2_UPDATE_STEREO_MODE_BOTH              = 0x0,
	DCP_CURSOR2_UPDATE_STEREO_MODE_SECONDARY_ONLY    = 0x1,
	DCP_CURSOR2_UPDATE_STEREO_MODE_UNDEFINED         = 0x2,
	DCP_CURSOR2_UPDATE_STEREO_MODE_PRIMARY_ONLY      = 0x3,
} DCP_CURSOR2_UPDATE_STEREO_MODE;
typedef enum DCP_CUR_REQUEST_FILTER_DIS {
	DCP_CUR_REQUEST_FILTER_DIS_FALSE                 = 0x0,
	DCP_CUR_REQUEST_FILTER_DIS_TRUE                  = 0x1,
} DCP_CUR_REQUEST_FILTER_DIS;
typedef enum DCP_CURSOR_STEREO_EN {
	DCP_CURSOR_STEREO_EN_FALSE                       = 0x0,
	DCP_CURSOR_STEREO_EN_TRUE                        = 0x1,
} DCP_CURSOR_STEREO_EN;
typedef enum DCP_CURSOR_STEREO_OFFSET_YNX {
	DCP_CURSOR_STEREO_OFFSET_YNX_X_POSITION          = 0x0,
	DCP_CURSOR_STEREO_OFFSET_YNX_Y_POSITION          = 0x1,
} DCP_CURSOR_STEREO_OFFSET_YNX;
typedef enum DCP_CURSOR2_STEREO_EN {
	DCP_CURSOR2_STEREO_EN_FALSE                      = 0x0,
	DCP_CURSOR2_STEREO_EN_TRUE                       = 0x1,
} DCP_CURSOR2_STEREO_EN;
typedef enum DCP_CURSOR2_STEREO_OFFSET_YNX {
	DCP_CURSOR2_STEREO_OFFSET_YNX_X_POSITION         = 0x0,
	DCP_CURSOR2_STEREO_OFFSET_YNX_Y_POSITION         = 0x1,
} DCP_CURSOR2_STEREO_OFFSET_YNX;
typedef enum DCP_DC_LUT_RW_MODE {
	DCP_DC_LUT_RW_MODE_256_ENTRY                     = 0x0,
	DCP_DC_LUT_RW_MODE_PWL                           = 0x1,
} DCP_DC_LUT_RW_MODE;
typedef enum DCP_DC_LUT_VGA_ACCESS_ENABLE {
	DCP_DC_LUT_VGA_ACCESS_ENABLE_FALSE               = 0x0,
	DCP_DC_LUT_VGA_ACCESS_ENABLE_TRUE                = 0x1,
} DCP_DC_LUT_VGA_ACCESS_ENABLE;
typedef enum DCP_DC_LUT_AUTOFILL {
	DCP_DC_LUT_AUTOFILL_FALSE                        = 0x0,
	DCP_DC_LUT_AUTOFILL_TRUE                         = 0x1,
} DCP_DC_LUT_AUTOFILL;
typedef enum DCP_DC_LUT_AUTOFILL_DONE {
	DCP_DC_LUT_AUTOFILL_DONE_FALSE                   = 0x0,
	DCP_DC_LUT_AUTOFILL_DONE_TRUE                    = 0x1,
} DCP_DC_LUT_AUTOFILL_DONE;
typedef enum DCP_DC_LUT_INC_B {
	DCP_DC_LUT_INC_B_NA                              = 0x0,
	DCP_DC_LUT_INC_B_2                               = 0x1,
	DCP_DC_LUT_INC_B_4                               = 0x2,
	DCP_DC_LUT_INC_B_8                               = 0x3,
	DCP_DC_LUT_INC_B_16                              = 0x4,
	DCP_DC_LUT_INC_B_32                              = 0x5,
	DCP_DC_LUT_INC_B_64                              = 0x6,
	DCP_DC_LUT_INC_B_128                             = 0x7,
	DCP_DC_LUT_INC_B_256                             = 0x8,
	DCP_DC_LUT_INC_B_512                             = 0x9,
} DCP_DC_LUT_INC_B;
typedef enum DCP_DC_LUT_DATA_B_SIGNED_EN {
	DCP_DC_LUT_DATA_B_SIGNED_EN_FALSE                = 0x0,
	DCP_DC_LUT_DATA_B_SIGNED_EN_TRUE                 = 0x1,
} DCP_DC_LUT_DATA_B_SIGNED_EN;
typedef enum DCP_DC_LUT_DATA_B_FLOAT_POINT_EN {
	DCP_DC_LUT_DATA_B_FLOAT_POINT_EN_FALSE           = 0x0,
	DCP_DC_LUT_DATA_B_FLOAT_POINT_EN_TRUE            = 0x1,
} DCP_DC_LUT_DATA_B_FLOAT_POINT_EN;
typedef enum DCP_DC_LUT_DATA_B_FORMAT {
	DCP_DC_LUT_DATA_B_FORMAT_U0P10                   = 0x0,
	DCP_DC_LUT_DATA_B_FORMAT_S1P10                   = 0x1,
	DCP_DC_LUT_DATA_B_FORMAT_U1P11                   = 0x2,
	DCP_DC_LUT_DATA_B_FORMAT_U0P12                   = 0x3,
} DCP_DC_LUT_DATA_B_FORMAT;
typedef enum DCP_DC_LUT_INC_G {
	DCP_DC_LUT_INC_G_NA                              = 0x0,
	DCP_DC_LUT_INC_G_2                               = 0x1,
	DCP_DC_LUT_INC_G_4                               = 0x2,
	DCP_DC_LUT_INC_G_8                               = 0x3,
	DCP_DC_LUT_INC_G_16                              = 0x4,
	DCP_DC_LUT_INC_G_32                              = 0x5,
	DCP_DC_LUT_INC_G_64                              = 0x6,
	DCP_DC_LUT_INC_G_128                             = 0x7,
	DCP_DC_LUT_INC_G_256                             = 0x8,
	DCP_DC_LUT_INC_G_512                             = 0x9,
} DCP_DC_LUT_INC_G;
typedef enum DCP_DC_LUT_DATA_G_SIGNED_EN {
	DCP_DC_LUT_DATA_G_SIGNED_EN_FALSE                = 0x0,
	DCP_DC_LUT_DATA_G_SIGNED_EN_TRUE                 = 0x1,
} DCP_DC_LUT_DATA_G_SIGNED_EN;
typedef enum DCP_DC_LUT_DATA_G_FLOAT_POINT_EN {
	DCP_DC_LUT_DATA_G_FLOAT_POINT_EN_FALSE           = 0x0,
	DCP_DC_LUT_DATA_G_FLOAT_POINT_EN_TRUE            = 0x1,
} DCP_DC_LUT_DATA_G_FLOAT_POINT_EN;
typedef enu