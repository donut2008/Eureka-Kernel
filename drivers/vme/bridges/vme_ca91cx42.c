UAL                                       = 0x3,
	REF_GREATER                                      = 0x4,
	REF_NOTEQUAL                                     = 0x5,
	REF_GEQUAL                                       = 0x6,
	REF_ALWAYS                                       = 0x7,
} CompareRef;
typedef enum ReadSize {
	READ_256_BITS                                    = 0x0,
	READ_512_BITS                                    = 0x1,
} ReadSize;
typedef enum DepthFormat {
	DEPTH_INVALID                                    = 0x0,
	DEPTH_16                                         = 0x1,
	DEPTH_X8_24                                      = 0x2,
	DEPTH_8_24                                       = 0x3,
	DEPTH_X8_24_FLOAT                                = 0x4,
	DEPTH_8_24_FLOAT                                 = 0x5,
	DEPTH_32_FLOAT                                   = 0x6,
	DEPTH_X24_8_32_FLOAT                             = 0x7,
} DepthFormat;
typedef enum ZFormat {
	Z_INVALID                                        = 0x0,
	Z_16                                             = 0x1,
	Z_24                                             = 0x2,
	Z_32_FLOAT                                       = 0x3,
} ZFormat;
typedef enum StencilFormat {
	STENCIL_INVALID                                  = 0x0,
	STENCIL_8                                        = 0x1,
} StencilFormat;
typedef enum CmaskMode {
	CMASK_CLEAR_NONE                                 = 0x0,
	CMASK_CLEAR_ONE                                  = 0x1,
	CMASK_CLEAR_ALL                                  = 0x2,
	CMASK_ANY_EXPANDED                               = 0x3,
	CMASK_ALPHA0_FRAG1                               = 0x4,
	CMASK_ALPHA0_FRAG2                               = 0x5,
	CMASK_ALPHA0_FRAG4                               = 0x6,
	CMASK_ALPHA0_FRAGS                               = 0x7,
	CMASK_ALPHA1_FRAG1                               = 0x8,
	CMASK_ALPHA1_FRAG2                               = 0x9,
	CMASK_ALPHA1_FRAG4                               = 0xa,
	CMASK_ALPHA1_FRAGS                               = 0xb,
	CMASK_ALPHAX_FRAG1                               = 0xc,
	CMASK_ALPHAX_FRAG2                               = 0xd,
	CMASK_ALPHAX_FRAG4                               = 0xe,
	CMASK_ALPHAX_FRAGS                               = 0xf,
} CmaskMode;
typedef enum QuadExportFormat {
	EXPORT_UNUSED                                    = 0x0,
	EXPORT_32_R                                      = 0x1,
	EXPORT_32_GR                                     = 0x2,
	EXPORT_32_AR                                     = 0x3,
	EXPORT_FP16_ABGR                                 = 0x4,
	EXPORT_UNSIGNED16_ABGR                           = 0x5,
	EXPORT_SIGNED16_ABGR                             = 0x6,
	EXPORT_32_ABGR                                   = 0x7,
} QuadExportFormat;
typedef enum QuadExportFormatOld {
	EXPORT_4P_32BPC_ABGR                             = 0x0,
	EXPORT_4P_16BPC_ABGR                             = 0x1,
	EXPORT_4P_32BPC_GR                               = 0x2,
	EXPORT_4P_32BPC_AR                               = 0x3,
	EXPORT_2P_32BPC_ABGR                             = 0x4,
	EXPORT_8P_32BPC_R                                = 0x5,
} QuadExportFormatOld;
typedef enum ColorFormat {
	COLOR_INVALID                                    = 0x0,
	COLOR_8                                          = 0x1,
	COLOR_16                                         = 0x2,
	COLOR_8_8                                        = 0x3,
	COLOR_32                                         = 0x4,
	COLOR_16_16                                      = 0x5,
	COLOR_10_11_11                                   = 0x6,
	COLOR_11_11_10                                   = 0x7,
	COLOR_10_10_10_2                                 = 0x8,
	COLOR_2_10_10_10                                 = 0x9,
	COLOR_8_8_8_8                                    = 0xa,
	COLOR_32_32                                      = 0xb,
	COLOR_16_16_16_16                                = 0xc,
	COLOR_RESERVED_13                                = 0xd,
	COLOR_32_32_32_32                                = 0xe,
	COLOR_RESERVED_15                                = 0xf,
	COLOR_5_6_5                                      = 0x10,
	COLOR_1_5_5_5                                    = 0x11,
	COLOR_5_5_5_1                                    = 0x12,
	COLOR_4_4_4_4                                    = 0x13,
	COLOR_8_24                                       = 0x14,
	COLOR_24_8                                       = 0x15,
	COLOR_X24_8_32_FLOAT                             = 0x16,
	COLOR_RESERVED_23                                = 0x17,
} ColorFormat;
typedef enum SurfaceFormat {
	FMT_INVALID                                      = 0x0,
	FMT_8                                            = 0x1,
	FMT_16                                           = 0x2,
	FMT_8_8                                          = 0x3,
	FMT_32                                           = 0x4,
	FMT_16_16                                        = 0x5,
	FMT_10_11_11                                     = 0x6,
	FMT_11_11_10                                     = 0x7,
	FMT_10_10_10_2                                   = 0x8,
	FMT_2_10_10_10                                   = 0x9,
	FMT_8_8_8_8                                      = 0xa,
	FMT_32_32                                        = 0xb,
	FMT_16_16_16_16                                  = 0xc,
	FMT_32_32_32                                     = 0xd,
	FMT_32_32_32_32                                  = 0xe,
	FMT_RESERVED_4                                   = 0xf,
	FMT_5_6_5                                        = 0x10,
	FMT_1_5_5_5                                      = 0x11,
	FMT_5_5_5_1                                      = 0x12,
	FMT_4_4_4_4                                      = 0x13,
	FMT_8_24                                         = 0x14,
	FMT_24_8                                         = 0x15,
	FMT_X24_8_32_FLOAT                               = 0x16,
	FMT_RESERVED_33                                  = 0x17,
	FMT_11_11_10_FLOAT                               = 0x18,
	FMT_16_FLOAT                                     = 0x19,
	FMT_32_FLOAT                                     = 0x1a,
	FMT_16_16_FLOAT                                  = 0x1b,
	FMT_8_24_FLOAT                                   = 0x1c,
	FMT_24_8_FLOAT                                   = 0x1d,
	FMT_32_32_FLOAT                                  = 0x1e,
	FMT_10_11_11_FLOAT                               = 0x1f,
	FMT_16_16_16_16_FLOAT                            = 0x20,
	FMT_3_3_2                                        = 0x21,
	FMT_6_5_5                                        = 0x22,
	FMT_32_32_32_32_FLOAT                            = 0x23,
	FMT_RESERVED_36                                  = 0x24,
	FMT_1                                            = 0x25,
	FMT_1_REVERSED                                   = 0x26,
	FMT_GB_GR                                        = 0x27,
	FMT_BG_RG                                        = 0x28,
	FMT_32_AS_8                                      = 0x29,
	FMT_32_AS_8_8                                    = 0x2a,
	FMT_5_9_9_9_SHAREDEXP                            = 0x2b,
	FMT_8_8_8                                        = 0x2c,
	FMT_16_16_16                                     = 0x2d,
	FMT_16_16_16_FLOAT                               = 0x2e,
	FMT_4_4                                          = 0x2f,
	FMT_32_32_32_FLOAT                               = 0x30,
	FMT_BC1                                          = 0x31,
	FMT_BC2                                          = 0x32,
	FMT_BC3                                          = 0x33,
	FMT_BC4                                          = 0x34,
	FMT_BC5                                          = 0x35,
	FMT_BC6                                          = 0x36,
	FMT_BC7                                          = 0x37,
	FMT_32_AS_32_32_32_32                            = 0x38,
	FMT_APC3                                         = 0x39,
	FMT_APC4                                         = 0x3a,
	FMT_APC5                                         = 0x3b,
	FMT_APC6                                         = 0x3c,
	FMT_APC7                                         = 0x3d,
	FMT_CTX1                                         = 0x3e,
	FMT_RESERVED_63                                  = 0x3f,
} SurfaceFormat;
typedef enum BUF_DATA_FORMAT {
	BUF_DATA_FORMAT_INVALID                          = 0x0,
	BUF_DATA_FORMAT_8                                = 0x1,
	BUF_DATA_FORMAT_16                               = 0x2,
	BUF_DATA_FORMAT_8_8                              = 0x3,
	BUF_DATA_FORMAT_32                               = 0x4,
	BUF_DATA_FORMAT_16_16                            = 0x5,
	BUF_DATA_FORMAT_10_11_11                         = 0x6,
	BUF_DATA_FORMAT_11_11_10                         = 0x7,
	BUF_DATA_FORMAT_10_10_10_2                       = 0x8,
	BUF_DATA_FORMAT_2_10_10_10                       = 0x9,
	BUF_DATA_FORMAT_8_8_8_8                          = 0xa,
	BUF_DATA_FORMAT_32_32                            = 0xb,
	BUF_DATA_FORMAT_16_16_16_16                      = 0xc,
	BUF_DATA_FORMAT_32_32_32                         = 0xd,
	BUF_DATA_FORMAT_32_32_32_32                      = 0xe,
	BUF_DATA_FORMAT_RESERVED_15                      = 0xf,
} BUF_DATA_FORMAT;
typedef enum IMG_DATA_FORMAT {
	IMG_DATA_FORMAT_INVALID                          = 0x0,
	IMG_DATA_FORMAT_8                                = 0x1,
	IMG_DATA_FORMAT_16                               = 0x2,
	IMG_DATA_FORMAT_8_8                              = 0x3,
	IMG_DATA_FORMAT_32                               = 0x4,
	IMG_DATA_FORMAT_16_16                            = 0x5,
	IMG_DATA_FORMAT_10_11_11                         = 0x6,
	IMG_DATA_FORMAT_11_11_10                         = 0x7,
	IMG_DATA_FORMAT_10_10_10_2                       = 0x8,
	IMG_DATA_FORMAT_2_10_10_10                       = 0x9,
	IMG_DATA_FORMAT_8_8_8_8                          = 0xa,
	IMG_DATA_FORMAT_32_32                            = 0xb,
	IMG_DATA_FORMAT_16_16_16_16                      = 0xc,
	IMG_DATA_FORMAT_32_32_32                         = 0xd,
	IMG_DATA_FORMAT_32_32_32_32                      = 0xe,
	IMG_DATA_FORMAT_RESERVED_15                      = 0xf,
	IMG_DATA_FORMAT_5_6_5                            = 0x10,
	IMG_DATA_FORMAT_1_5_5_5                          = 0x11,
	IMG_DATA_FORMAT_5_5_5_1                          = 0x12,
	IMG_DATA_FORMAT_4_4_4_4                          = 0x13,
	IMG_DATA_FORMAT_8_24                             = 0x14,
	IMG_DATA_FORMAT_24_8                             = 0x15,
	IMG_DATA_FORMAT_X24_8_32                         = 0x16,
	IMG_DATA_FORMAT_RESERVED_23                      = 0x17,
	IMG_DATA_FORMAT_RESERVED_24                      = 0x18,
	IMG_DATA_FORMAT_RESERVED_25                      = 0x19,
	IMG_DATA_FORMAT_RESERVED_26                      = 0x1a,
	IMG_DATA_FORMAT_RESERVED_27                      = 0x1b,
	IMG_DATA_FORMAT_RESERVED_28                      = 0x1c,
	IMG_DATA_FORMAT_RESERVED_29                      = 0x1d,
	IMG_DATA_FORMAT_RESERVED_30                      = 0x1e,
	IMG_DATA_FORMAT_RESERVED_31                      = 0x1f,
	IMG_DATA_FORMAT_GB_GR                            = 0x20,
	IMG_DATA_FORMAT_BG_RG                            = 0x21,
	IMG_DATA_FORMAT_5_9_9_9                          = 0x22,
	IMG_DATA_FORMAT_BC1                              = 0x23,
	IMG_DATA_FORMAT_BC2                              = 0x24,
	IMG_DATA_FORMAT_BC3                              = 0x25,
	IMG_DATA_FORMAT_BC4                              = 0x26,
	IMG_DATA_FORMAT_BC5                              = 0x27,
	IMG_DATA_FORMAT_BC6                              = 0x28,
	IMG_DATA_FORMAT_BC7                              = 0x29,
	IMG_DATA_FORMAT_RESERVED_42                      = 0x2a,
	IMG_DATA_FORMAT_RESERVED_43                      = 0x2b,
	IMG_DATA_FORMAT_FMASK8_S2_F1                     = 0x2c,
	IMG_DATA_FORMAT_FMASK8_S4_F1                     = 0x2d,
	IMG_DATA_FORMAT_FMASK8_S8_F1                     = 0x2e,
	IMG_DATA_FORMAT_FMASK8_S2_F2                     = 0x2f,
	IMG_DATA_FORMAT_FMASK8_S4_F2                     = 0x30,
	IMG_DATA_FORMAT_FMASK8_S4_F4                     = 0x31,
	IMG_DATA_FORMAT_FMASK16_S16_F1                   = 0x32,
	IMG_DATA_FORMAT_FMASK16_S8_F2                    = 0x33,
	IMG_DATA_FORMAT_FMASK32_S16_F2                   = 0x34,
	IMG_DATA_FORMAT_FMASK32_S8_F4                    = 0x35,
	IMG_DATA_FORMAT_FMASK32_S8_F8                    = 0x36,
	IMG_DATA_FORMAT_FMASK64_S16_F4                   = 0x37,
	IMG_DATA_FORMAT_FMASK64_S16_F8                   = 0x38,
	IMG_DATA_FORMAT_4_4                              = 0x39,
	IMG_DATA_FORMAT_6_5_5                            = 0x3a,
	IMG_DATA_FORMAT_1                                = 0x3b,
	IMG_DATA_FORMAT_1_REVERSED                       = 0x3c,
	IMG_DATA_FORMAT_32_AS_8                          = 0x3d,
	IMG_DATA_FORMAT_32_AS_8_8                        = 0x3e,
	IMG_DATA_FORMAT_32_AS_32_32_32_32                = 0x3f,
} IMG_DATA_FORMAT;
typedef enum BUF_NUM_FORMAT {
	BUF_NUM_FORMAT_UNORM                             = 0x0,
	BUF_NUM_FORMAT_SNORM                             = 0x1,
	BUF_NUM_FORMAT_USCALED                           = 0x2,
	BUF_NUM_FORMAT_SSCALED                           = 0x3,
	BUF_NUM_FORMAT_UINT                              = 0x4,
	BUF_NUM_FORMAT_SINT                              = 0x5,
	BUF_NUM_FORMAT_RESERVED_6                        = 0x6,
	BUF_NUM_FORMAT_FLOAT                             = 0x7,
} BUF_NUM_FORMAT;
typedef enum IMG_NUM_FORMAT {
	IMG_NUM_FORMAT_UNORM                             = 0x0,
	IMG_NUM_FORMAT_SNORM                             = 0x1,
	IMG_NUM_FORMAT_USCALED                           = 0x2,
	IMG_NUM_FORMAT_SSCALED                           = 0x3,
	IMG_NUM_FORMAT_UINT                              = 0x4,
	IMG_NUM_FORMAT_SINT                              = 0x5,
	IMG_NUM_FORMAT_RESERVED_6                        = 0x6,
	IMG_NUM_FORMAT_FLOAT                             = 0x7,
	IMG_NUM_FORMAT_RESERVED_8                        = 0x8,
	IMG_NUM_FORMAT_SRGB                              = 0x9,
	IMG_NUM_FORMAT_RESERVED_10                       = 0xa,
	IMG_NUM_FORMAT_RESERVED_11                       = 0xb,
	IMG_NUM_FORMAT_RESERVED_12                       = 0xc,
	IMG_NUM_FORMAT_RESERVED_13                       = 0xd,
	IMG_NUM_FORMAT_RESERVED_14                       = 0xe,
	IMG_NUM_FORMAT_RESERVED_15                       = 0xf,
} IMG_NUM_FORMAT;
typedef enum TileType {
	ARRAY_COLOR_TILE                                 = 0x0,
	ARRAY_DEPTH_TILE                                 = 0x1,
} TileType;
typedef enum NonDispTilingOrder {
	ADDR_SURF_MICRO_TILING_DISPLAY                   = 0x0,
	ADDR_SURF_MICRO_TILING_NON_DISPLAY               = 0x1,
} NonDispTilingOrder;
typedef enum MicroTileMode {
	ADDR_SURF_DISPLAY_MICRO_TILING                   = 0x0,
	ADDR_SURF_THIN_MICRO_TILING                      = 0x1,
	ADDR_SURF_DEPTH_MICRO_TILING                     = 0x2,
	ADDR_SURF_ROTATED_MICRO_TILING                   = 0x3,
	ADDR_SURF_THICK_MICRO_TILING                     = 0x4,
} MicroTileMode;
typedef enum TileSplit {
	ADDR_SURF_TILE_SPLIT_64B                         = 0x0,
	ADDR_SURF_TILE_SPLIT_128B                        = 0x1,
	ADDR_SURF_TILE_SPLIT_256B                        = 0x2,
	ADDR_SURF_TILE_SPLIT_512B                        = 0x3,
	ADDR_SURF_TILE_SPLIT_1KB                         = 0x4,
	ADDR_SURF_TILE_SPLIT_2KB                         = 0x5,
	ADDR_SURF_TILE_SPLIT_4KB                         = 0x6,
} TileSplit;
typedef enum SampleSplit {
	ADDR_SURF_SAMPLE_SPLIT_1                         = 0x0,
	ADDR_SURF_SAMPLE_SPLIT_2                         = 0x1,
	ADDR_SURF_SAMPLE_SPLIT_4                         = 0x2,
	ADDR_SURF_SAMPLE_SPLIT_8                         = 0x3,
} SampleSplit;
typedef enum PipeConfig {
	ADDR_SURF_P2                                     = 0x0,
	ADDR_SURF_P2_RESERVED0                           = 0x1,
	ADDR_SURF_P2_RESERVED1                           = 0x2,
	ADDR_SURF_P2_RESERVED2                           = 0x3,
	ADDR_SURF_P4_8x16                                = 0x4,
	ADDR_SURF_P4_16x16                               = 0x5,
	ADDR_SURF_P4_16x32                               = 0x6,
	ADDR_SURF_P4_32x32                               = 0x7,
	ADDR_SURF_P8_16x16_8x16                          = 0x8,
	ADDR_SURF_P8_16x32_8x16                          = 0x9,
	ADDR_SURF_P8_32x32_8x16                          = 0xa,
	ADDR_SURF_P8_16x32_16x16                         = 0xb,
	ADDR_SURF_P8_32x32_16x16                         = 0xc,
	ADDR_SURF_P8_32x32_16x32                         = 0xd,
	ADDR_SURF_P8_32x64_32x32                         = 0xe,
	ADDR_SURF_P8_RESERVED0                           = 0xf,
	ADDR_SURF_P16_32x32_8x16                         = 0x10,
	ADDR_SURF_P16_32x32_16x16                        = 0x11,
} PipeConfig;
typedef enum NumBanks {
	ADDR_SURF_2_BANK                                 = 0x0,
	ADDR_SURF_4_BANK                                 = 0x1,
	ADDR_SURF_8_BANK                                 = 0x2,
	ADDR_SURF_16_BANK                                = 0x3,
} NumBanks;
typedef enum BankWidth {
	ADDR_SURF_BANK_WIDTH_1                           = 0x0,
	ADDR_SURF_BANK_WIDTH_2                           = 0x1,
	ADDR_SURF_BANK_WIDTH_4                           = 0x2,
	ADDR_SURF_BANK_WIDTH_8                           = 0x3,
} BankWidth;
typedef enum BankHeight {
	ADDR_SURF_BANK_HEIGHT_1                          = 0x0,
	ADDR_SURF_BANK_HEIGHT_2                          = 0x1,
	ADDR_SURF_BANK_HEIGHT_4                          = 0x2,
	ADDR_SURF_BANK_HEIGHT_8                          = 0x3,
} BankHeight;
typedef enum BankWidthHeight {
	ADDR_SURF_BANK_WH_1                              = 0x0,
	ADDR_SURF_BANK_WH_2                              = 0x1,
	ADDR_SURF_BANK_WH_4                              = 0x2,
	ADDR_SURF_BANK_WH_8                              = 0x3,
} BankWidthHeight;
typedef enum MacroTileAspect {
	ADDR_SURF_MACRO_ASPECT_1                         = 0x0,
	ADDR_SURF_MACRO_ASPECT_2                         = 0x1,
	ADDR_SURF_MACRO_ASPECT_4                         = 0x2,
	ADDR_SURF_MACRO_ASPECT_8                         = 0x3,
} MacroTileAspect;
typedef enum GATCL1RequestType {
	GATCL1_TYPE_NORMAL                               = 0x0,
	GATCL1_TYPE_SHOOTDOWN                            = 0x1,
	GATCL1_TYPE_BYPASS                               = 0x2,
} GATCL1RequestType;
typedef enum TCC_CACHE_POLICIES {
	TCC_CACHE_POLICY_LRU                             = 0x0,
	TCC_CACHE_POLICY_STREAM                          = 0x1,
} TCC_CACHE_POLICIES;
typedef enum MTYPE {
	MTYPE_NC_NV                                      = 0x0,
	MTYPE_NC                                         = 0x1,
	MTYPE_CC                                         = 0x2,
	MTYPE_UC                                         = 0x3,
} MTYPE;
typedef enum PERFMON_COUNTER_MODE {
	PERFMON_COUNTER_MODE_ACCUM                       = 0x0,
	PERFMON_COUNTER_MODE_ACTIVE_CYCLES               = 0x1,
	PERFMON_COUNTER_MODE_MAX                         = 0x2,
	PERFMON_COUNTER_MODE_DIRTY                       = 0x3,
	PERFMON_COUNTER_MODE_SAMPLE                      = 0x4,
	PERFMON_COUNTER_MODE_CYCLES_SINCE_FIRST_EVENT    = 0x5,
	PERFMON_COUNTER_MODE_CYCLES_SINCE_LAST_EVENT     = 0x6,
	PERFMON_COUNTER_MODE_CYCLES_GE_HI                = 0x7,
	PERFMON_COUNTER_MODE_CYCLES_EQ_HI                = 0x8,
	PERFMON_COUNTER_MODE_INACTIVE_CYCLES             = 0x9,
	PERFMON_COUNTER_MODE_RESERVED                    = 0xf,
} PERFMON_COUNTER_MODE;
typedef enum PERFMON_SPM_MODE {
	PERFMON_SPM_MODE_OFF                             = 0x0,
	PERFMON_SPM_MODE_16BIT_CLAMP                     = 0x1,
	PERFMON_SPM_MODE_16BIT_NO_CLAMP                  = 0x2,
	PERFMON_SPM_MODE_32BIT_CLAMP                     = 0x3,
	PERFMON_SPM_MODE_32BIT_NO_CLAMP                  = 0x4,
	PERFMON_SPM_MODE_RESERVED_5                      = 0x5,
	PERFMON_SPM_MODE_RESERVED_6                      = 0x6,
	PERFMON_SPM_MODE_RESERVED_7                      = 0x7,
	PERFMON_SPM_MODE_TEST_MODE_0                     = 0x8,
	PERFMON_SPM_MODE_TEST_MODE_1                     = 0x9,
	PERFMON_SPM_MODE_TEST_MODE_2                     = 0xa,
} PERFMON_SPM_MODE;
typedef enum SurfaceTiling {
	ARRAY_LINEAR                                     = 0x0,
	ARRAY_TILED                                      = 0x1,
} SurfaceTiling;
typedef enum SurfaceArray {
	ARRAY_1D                                         = 0x0,
	ARRAY_2D                                         = 0x1,
	ARRAY_3D                                         = 0x2,
	ARRAY_3D_SLICE                                   = 0x3,
} SurfaceArray;
typedef enum ColorArray {
	ARRAY_2D_ALT_COLOR                               = 0x0,
	ARRAY_2D_COLOR                                   = 0x1,
	ARRAY_3D_SLICE_COLOR                             = 0x3,
} ColorArray;
typedef enum DepthArray {
	ARRAY_2D_ALT_DEPTH                               = 0x0,
	ARRAY_2D_DEPTH                                   = 0x1,
} DepthArray;
typedef enum ENUM_NUM_SIMD_PER_CU {
	NUM_SIMD_PER_CU                                  = 0x4,
} ENUM_NUM_SIMD_PER_CU;
typedef enum MEM_PWR_FORCE_CTRL {
	NO_FORCE_REQUEST                                 = 0x0,
	FORCE_LIGHT_SLEEP_REQUEST                        = 0x1,
	FORCE_DEEP_SLEEP_REQUEST                         = 0x2,
	FORCE_SHUT_DOWN_REQUEST                          = 0x3,
} MEM_PWR_FORCE_CTRL;
typedef enum MEM_PWR_FORCE_CTRL2 {
	NO_FORCE_REQ                                     = 0x0,
	FORCE_LIGHT_SLEEP_REQ                            = 0x1,
} MEM_PWR_FORCE_CTRL2;
typedef enum MEM_PWR_DIS_CTRL {
	ENABLE_MEM_PWR_CTRL                              = 0x0,
	DISABLE_MEM_PWR_CTRL                             = 0x1,
} MEM_PWR_DIS_CTRL;
typedef enum MEM_PWR_SEL_CTRL {
	DYNAMIC_SHUT_DOWN_ENABLE                         = 0x0,
	DYNAMIC_DEEP_SLEEP_ENABLE                        = 0x1,
	DYNAMIC_LIGHT_SLEEP_ENABLE                       = 0x2,
} MEM_PWR_SEL_CTRL;
typedef enum MEM_PWR_SEL_CTRL2 {
	DYNAMIC_DEEP_SLEEP_EN                            = 0x0,
	DYNAMIC_LIGHT_SLEEP_EN                           = 0x1,
} MEM_PWR_SEL_CTRL2;
typedef enum HPD_INT_CONTROL_ACK {
	HPD_INT_CONTROL_ACK_0                            = 0x0,
	HPD_INT_CONTROL_ACK_1                            = 0x1,
} HPD_INT_CONTROL_ACK;
typedef enum HPD_INT_CONTROL_POLARITY {
	HPD_INT_CONTROL_GEN_INT_ON_DISCON                = 0x0,
	HPD_INT_CONTROL_GEN_INT_ON_CON                   = 0x1,
} HPD_INT_CONTROL_POLARITY;
typedef enum HPD_INT_CONTROL_RX_INT_ACK {
	HPD_INT_CONTROL_RX_INT_ACK_0                     = 0x0,
	HPD_INT_CONTROL_RX_INT_ACK_1                     = 0x1,
} HPD_INT_CONTROL_RX_INT_ACK;
typedef enum DPDBG_EN {
	DPDBG_DISABLE                                    = 0x0,
	DPDBG_ENABLE                                     = 0x1,
} DPDBG_EN;
typedef enum DPDBG_INPUT_EN {
	DPDBG_INPUT_DISABLE                              = 0x0,
	DPDBG_INPUT_ENABLE                               = 0x1,
} DPDBG_INPUT_EN;
typedef enum DPDBG_ERROR_DETECTION_MODE {
	DPDBG_ERROR_DETECTION_MODE_CSC                   = 0x0,
	DPDBG_ERROR_DETECTION_MODE_RS_ENCODING           = 0x1,
} DPDBG_ERROR_DETECTION_MODE;
typedef enum DPDBG_FIFO_OVERFLOW_INTERRUPT_MASK {
	DPDBG_FIFO_OVERFLOW_INT_DISABLE                  = 0x0,
	DPDBG_FIFO_OVERFLOW_INT_ENABLE                   = 0x1,
} DPDBG_FIFO_OVERFLOW_INTERRUPT_MASK;
typedef enum DPDBG_FIFO_OVERFLOW_INTERRUPT_TYPE {
	DPDBG_FIFO_OVERFLOW_INT_LEVEL_BASED              = 0x0,
	DPDBG_FIFO_OVERFLOW_INT_PULSE_BASED              = 0x1,
} DPDBG_FIFO_OVERFLOW_INTERRUPT_TYPE;
typedef enum DPDBG_FIFO_OVERFLOW_INTERRUPT_ACK {
	DPDBG_FIFO_OVERFLOW_INT_NO_ACK                   = 0x0,
	DPDBG_FIFO_OVERFLOW_INT_CLEAR                    = 0x1,
} DPDBG_FIFO_OVERFLOW_INTERRUPT_ACK;
typedef enum PM_ASSERT_RESET {
	PM_ASSERT_RESET_0                                = 0x0,
	PM_ASSERT_RESET_1                                = 0x1,
} PM_ASSERT_RESET;
typedef enum DAC_MUX_SELECT {
	DAC_MUX_SELECT_DACA                              = 0x0,
	DAC_MUX_SELECT_DACB                              = 0x1,
} DAC_MUX_SELECT;
typedef enum TMDS_DVO_MUX_SELECT {
	TMDS_DVO_MUX_SELECT_B                            = 0x0,
	TMDS_DVO_MUX_SELECT_G                            = 0x1,
	TMDS_DVO_MUX_SELECT_R                            = 0x2,
	TMDS_DVO_MUX_SELECT_RESERVED                     = 0x3,
} TMDS_DVO_MUX_SELECT;
typedef enum DACA_SOFT_RESET {
	DACA_SOFT_RESET_0                                = 0x0,
	DACA_SOFT_RESET_1                                = 0x1,
} DACA_SOFT_RESET;
typedef enum I2S0_SPDIF0_SOFT_RESET {
	I2S0_SPDIF0_SOFT_RESET_0                         = 0x0,
	I2S0_SPDIF0_SOFT_RESET_1                         = 0x1,
} I2S0_SPDIF0_SOFT_RESET;
typedef enum I2S1_SOFT_RESET {
	I2S1_SOFT_RESET_0                                = 0x0,
	I2S1_SOFT_RESET_1                                = 0x1,
} I2S1_SOFT_RESET;
typedef enum SPDIF1_SOFT_RESET {
	SPDIF1_SOFT_RESET_0                              = 0x0,
	SPDIF1_SOFT_RESET_1                              = 0x1,
} SPDIF1_SOFT_RESET;
typedef enum DB_CLK_SOFT_RESET {
	DB_CLK_SOFT_RESET_0                              = 0x0,
	DB_CLK_SOFT_RESET_1                              = 0x1,
} DB_CLK_SOFT_RESET;
typedef enum FMT0_SOFT_RESET {
	FMT0_SOFT_RESET_0                                = 0x0,
	FMT0_SOFT_RESET_1                                = 0x1,
} FMT0_SOFT_RESET;
typedef enum FMT1_SOFT_RESET {
	FMT1_SOFT_RESET_0                                = 0x0,
	FMT1_SOFT_RESET_1                                = 0x1,
} FMT1_SOFT_RESET;
typedef enum FMT2_SOFT_RESET {
	FMT2_SOFT_RESET_0                                = 0x0,
	FMT2_SOFT_RESET_1                                = 0x1,
} FMT2_SOFT_RESET;
typedef enum FMT3_SOFT_RESET {
	FMT3_SOFT_RESET_0                                = 0x0,
	FMT3_SOFT_RESET_1                                = 0x1,
} FMT3_SOFT_RESET;
typedef enum FMT4_SOFT_RESET {
	FMT4_SOFT_RESET_0                                = 0x0,
	FMT4_SOFT_RESET_1                                = 0x1,
} FMT4_SOFT_RESET;
typedef enum FMT5_SOFT_RESET {
	FMT5_SOFT_RESET_0                                = 0x0,
	FMT5_SOFT_RESET_1                                = 0x1,
} FMT5_SOFT_RESET;
typedef enum MVP_SOFT_RESET {
	MVP_SOFT_RESET_0                                 = 0x0,
	MVP_SOFT_RESET_1                                 = 0x1,
} MVP_SOFT_RESET;
typedef enum ABM_SOFT_RESET {
	ABM_SOFT_RESET_0                                 = 0x0,
	ABM_SOFT_RESET_1                                 = 0x1,
} ABM_SOFT_RESET;
typedef enum DVO_SOFT_RESET {
	DVO_SOFT_RESET_0                                 = 0x0,
	DVO_SOFT_RESET_1                                 = 0x1,
} DVO_SOFT_RESET;
typedef enum DIGA_FE_SOFT_RESET {
	DIGA_FE_SOFT_RESET_0                             = 0x0,
	DIGA_FE_SOFT_RESET_1                             = 0x1,
} DIGA_FE_SOFT_RESET;
typedef enum DIGA_BE_SOFT_RESET {
	DIGA_BE_SOFT_RESET_0                             = 0x0,
	DIGA_BE_SOFT_RESET_1                             = 0x1,
} DIGA_BE_SOFT_RESET;
typedef enum DIGB_FE_SOFT_RESET {
	DIGB_FE_SOFT_RESET_0                             = 0x0,
	DIGB_FE_SOFT_RESET_1                             = 0x1,
} DIGB_FE_SOFT_RESET;
typedef enum DIGB_BE_SOFT_RESET {
	DIGB_BE_SOFT_RESET_0                             = 0x0,
	DIGB_BE_SOFT_RESET_1                             = 0x1,
} DIGB_BE_SOFT_RESET;
typedef enum DIGC_FE_SOFT_RESET {
	DIGC_FE_SOFT_RESET_0                             = 0x0,
	DIGC_FE_SOFT_RESET_1                             = 0x1,
} DIGC_FE_SOFT_RESET;
typedef enum DIGC_BE_SOFT_RESET {
	DIGC_BE_SOFT_RESET_0                             = 0x0,
	DIGC_BE_SOFT_RESET_1                             = 0x1,
} DIGC_BE_SOFT_RESET;
typedef enum DIGD_FE_SOFT_RESET {
	DIGD_FE_SOFT_RESET_0                             = 0x0,
	DIGD_FE_SOFT_RESET_1                             = 0x1,
} DIGD_FE_SOFT_RESET;
typedef enum DIGD_BE_SOFT_RESET {
	DIGD_BE_SOFT_RESET_0                             = 0x0,
	DIGD_BE_SOFT_RESET_1                             = 0x1,
} DIGD_BE_SOFT_RESET;
typedef enum DIGE_FE_SOFT_RESET {
	DIGE_FE_SOFT_RESET_0                             = 0x0,
	DIGE_FE_SOFT_RESET_1                             = 0x1,
} DIGE_FE_SOFT_RESET;
typedef enum DIGE_BE_SOFT_RESET {
	DIGE_BE_SOFT_RESET_0                             = 0x0,
	DIGE_BE_SOFT_RESET_1                             = 0x1,
} DIGE_BE_SOFT_RESET;
typedef enum DIGF_FE_SOFT_RESET {
	DIGF_FE_SOFT_RESET_0                             = 0x0,
	DIGF_FE_SOFT_RESET_1                             = 0x1,
} DIGF_FE_SOFT_RESET;
typedef enum DIGF_BE_SOFT_RESET {
	DIGF_BE_SOFT_RESET_0                             = 0x0,
	DIGF_BE_SOFT_RESET_1                             = 0x1,
} DIGF_BE_SOFT_RESET;
typedef enum DIGG_FE_SOFT_RESET {
	DIGG_FE_SOFT_RESET_0                             = 0x0,
	DIGG_FE_SOFT_RESET_1                             = 0x1,
} DIGG_FE_SOFT_RESET;
typedef enum DIGG_BE_SOFT_RESET {
	DIGG_BE_SOFT_RESET_0                             = 0x0,
	DIGG_BE_SOFT_RESET_1                             = 0x1,
} DIGG_BE_SOFT_RESET;
typedef enum DPDBG_SOFT_RESET {
	DPDBG_SOFT_RESET_0                               = 0x0,
	DPDBG_SOFT_RESET_1                               = 0x1,
} DPDBG_SOFT_RESET;
typedef enum DIGLPA_FE_SOFT_RESET {
	DIGLPA_FE_SOFT_RESET_0                           = 0x0,
	DIGLPA_FE_SOFT_RESET_1                           = 0x1,
} DIGLPA_FE_SOFT_RESET;
typedef enum DIGLPA_BE_SOFT_RESET {
	DIGLPA_BE_SOFT_RESET_0                           = 0x0,
	DIGLPA_BE_SOFT_RESET_1                           = 0x1,
} DIGLPA_BE_SOFT_RESET;
typedef enum DIGLPB_FE_SOFT_RESET {
	DIGLPB_FE_SOFT_RESET_0                           = 0x0,
	DIGLPB_FE_SOFT_RESET_1                           = 0x1,
} DIGLPB_FE_SOFT_RESET;
typedef enum DIGLPB_BE_SOFT_RESET {
	DIGLPB_BE_SOFT_RESET_0                           = 0x0,
	DIGLPB_BE_SOFT_RESET_1                           = 0x1,
} DIGLPB_BE_SOFT_RESET;
typedef enum GENERICA_STEREOSYNC_SEL {
	GENERICA_STEREOSYNC_SEL_D1                       = 0x0,
	GENERICA_STEREOSYNC_SEL_D2                       = 0x1,
	GENERICA_STEREOSYNC_SEL_D3                       = 0x2,
	GENERICA_STEREOSYNC_SEL_D4                       = 0x3,
	GENERICA_STEREOSYNC_SEL_D5                       = 0x4,
	GENERICA_STEREOSYNC_SEL_D6                       = 0x5,
	GENERICA_STEREOSYNC_SEL_RESERVED                 = 0x6,
} GENERICA_STEREOSYNC_SEL;
typedef enum GENERICB_STEREOSYNC_SEL {
	GENERICB_STEREOSYNC_SEL_D1                       = 0x0,
	GENERICB_STEREOSYNC_SEL_D2                       = 0x1,
	GENERICB_STEREOSYNC_SEL_D3                       = 0x2,
	GENERICB_STEREOSYNC_SEL_D4                       = 0x3,
	GENERICB_STEREOSYNC_SEL_D5                       = 0x4,
	GENERICB_STEREOSYNC_SEL_D6                       = 0x5,
	GENERICB_STEREOSYNC_SEL_RESERVED                 = 0x6,
} GENERICB_STEREOSYNC_SEL;
typedef enum DCO_DBG_BLOCK_SEL {
	DCO_DBG_BLOCK_SEL_DCO                            = 0x0,
	DCO_DBG_BLOCK_SEL_ABM                            = 0x1,
	DCO_DBG_BLOCK_SEL_DVO                            = 0x2,
	DCO_DBG_BLOCK_SEL_DAC                            = 0x3,
	DCO_DBG_BLOCK_SEL_MVP                            = 0x4,
	DCO_DBG_BLOCK_SEL_FMT0                           = 0x5,
	DCO_DBG_BLOCK_SEL_FMT1                           = 0x6,
	DCO_DBG_BLOCK_SEL_FMT2                           = 0x7,
	DCO_DBG_BLOCK_SEL_FMT3                           = 0x8,
	DCO_DBG_BLOCK_SEL_FMT4                           = 0x9,
	DCO_DBG_BLOCK_SEL_FMT5                           = 0xa,
	DCO_DBG_BLOCK_SEL_DIGFE_A                        = 0xb,
	DCO_DBG_BLOCK_SEL_DIGFE_B                        = 0xc,
	DCO_DBG_BLOCK_SEL_DIGFE_C                        = 0xd,
	DCO_DBG_BLOCK_SEL_DIGFE_D                        = 0xe,
	DCO_DBG_BLOCK_SEL_DIGFE_E                        = 0xf,
	DCO_DBG_BLOCK_SEL_DIGFE_F                        = 0x10,
	DCO_DBG_BLOCK_SEL_DIGFE_G                        = 0x11,
	DCO_DBG_BLOCK_SEL_DIGA                           = 0x12,
	DCO_DBG_BLOCK_SEL_DIGB                           = 0x13,
	DCO_DBG_BLOCK_SEL_DIGC                           = 0x14,
	DCO_DBG_BLOCK_SEL_DIGD                           = 0x15,
	DCO_DBG_BLOCK_SEL_DIGE                           = 0x16,
	DCO_DBG_BLOCK_SEL_DIGF                           = 0x17,
	DCO_DBG_BLOCK_SEL_DIGG                           = 0x18,
	DCO_DBG_BLOCK_SEL_DPFE_A                         = 0x19,
	DCO_DBG_BLOCK_SEL_DPFE_B                         = 0x1a,
	DCO_DBG_BLOCK_SEL_DPFE_C                         = 0x1b,
	DCO_DBG_BLOCK_SEL_DPFE_D                         = 0x1c,
	DCO_DBG_BLOCK_SEL_DPFE_E                         = 0x1d,
	DCO_DBG_BLOCK_SEL_DPFE_F                         = 0x1e,
	DCO_DBG_BLOCK_SEL_DPFE_G                         = 0x1f,
	DCO_DBG_BLOCK_SEL_DPA                            = 0x20,
	DCO_DBG_BLOCK_SEL_DPB                            = 0x21,
	DCO_DBG_BLOCK_SEL_DPC                            = 0x22,
	DCO_DBG_BLOCK_SEL_DPD                            = 0x23,
	DCO_DBG_BLOCK_SEL_DPE                            = 0x24,
	DCO_DBG_BLOCK_SEL_DPF                            = 0x25,
	DCO_DBG_BLOCK_SEL_DPG                            = 0x26,
	DCO_DBG_BLOCK_SEL_AUX0                           = 0x27,
	DCO_DBG_BLOCK_SEL_AUX1                           = 0x28,
	DCO_DBG_BLOCK_SEL_AUX2                           = 0x29,
	DCO_DBG_BLOCK_SEL_AUX3                           = 0x2a,
	DCO_DBG_BLOCK_SEL_AUX4                           = 0x2b,
	DCO_DBG_BLOCK_SEL_AUX5                           = 0x2c,
	DCO_DBG_BLOCK_SEL_PERFMON_DCO                    = 0x2d,
	DCO_DBG_BLOCK_SEL_AUDIO_OUT                      = 0x2e,
	DCO_DBG_BLOCK_SEL_DIGLPFEA                       = 0x2f,
	DCO_DBG_BLOCK_SEL_DIGLPFEB                       = 0x30,
	DCO_DBG_BLOCK_SEL_DIGLPA                         = 0x31,
	DCO_DBG_BLOCK_SEL_DIGLPB                         = 0x32,
	DCO_DBG_BLOCK_SEL_DPLPFEA                        = 0x33,
	DCO_DBG_BLOCK_SEL_DPLPFEB                        = 0x34,
	DCO_DBG_BLOCK_SEL_DPLPA                          = 0x35,
	DCO_DBG_BLOCK_SEL_DPLPB                          = 0x36,
} DCO_DBG_BLOCK_SEL;
typedef enum DCO_DBG_CLOCK_SEL {
	DCO_DBG_CLOCK_SEL_DISPCLK                        = 0x0,
	DCO_DBG_CLOCK_SEL_SCLK                           = 0x1,
	DCO_DBG_CLOCK_SEL_MVPCLK                         = 0x2,
	DCO_DBG_CLOCK_SEL_DVOCLK                         = 0x3,
	DCO_DBG_CLOCK_SEL_DACCLK                         = 0x4,
	DCO_DBG_CLOCK_SEL_REFCLK                         = 0x5,
	DCO_DBG_CLOCK_SEL_SYMCLKA                        = 0x6,
	DCO_DBG_CLOCK_SEL_SYMCLKB                        = 0x7,
	DCO_DBG_CLOCK_SEL_SYMCLKC                        = 0x8,
	DCO_DBG_CLOCK_SEL_SYMCLKD                        = 0x9,
	DCO_DBG_CLOCK_SEL_SYMCLKE                        = 0xa,
	DCO_DBG_CLOCK_SEL_SYMCLKF                        = 0xb,
	DCO_DBG_CLOCK_SEL_SYMCLKG                        = 0xc,
	DCO_DBG_CLOCK_SEL_RESERVED                       = 0xd,
	DCO_DBG_CLOCK_SEL_AM0CLK                         = 0xe,
	DCO_DBG_CLOCK_SEL_AM1CLK                         = 0xf,
	DCO_DBG_CLOCK_SEL_AM2CLK                         = 0x10,
	DCO_DBG_CLOCK_SEL_SYMCLKLPA                      = 0x11,
	DCO_DBG_CLOCK_SEL_SYMCLKLPB                      = 0x12,
} DCO_DBG_CLOCK_SEL;
typedef enum DOUT_I2C_CONTROL_GO {
	DOUT_I2C_CONTROL_STOP_TRANSFER                   = 0x0,
	DOUT_I2C_CONTROL_START_TRANSFER                  = 0x1,
} DOUT_I2C_CONTROL_GO;
typedef enum DOUT_I2C_CONTROL_SOFT_RESET {
	DOUT_I2C_CONTROL_NOT_RESET_I2C_CONTROLLER        = 0x0,
	DOUT_I2C_CONTROL_RESET_I2C_CONTROLLER            = 0x1,
} DOUT_I2C_CONTROL_SOFT_RESET;
typedef enum DOUT_I2C_CONTROL_SEND_RESET {
	DOUT_I2C_CONTROL__NOT_SEND_RESET                 = 0x0,
	DOUT_I2C_CONTROL__SEND_RESET                     = 0x1,
} DOUT_I2C_CONTROL_SEND_RESET;
typedef enum DOUT_I2C_CONTROL_SW_STATUS_RESET {
	DOUT_I2C_CONTROL_NOT_RESET_SW_STATUS             = 0x0,
	DOUT_I2C_CONTROL_RESET_SW_STATUS                 = 0x1,
} DOUT_I2C_CONTROL_SW_STATUS_RESET;
typedef enum DOUT_I2C_CONTROL_DDC_SELECT {
	DOUT_I2C_CONTROL_SELECT_DDC1                     = 0x0,
	DOUT_I2C_CONTROL_SELECT_DDC2                     = 0x1,
	DOUT_I2C_CONTROL_SELECT_DDC3                     = 0x2,
	DOUT_I2C_CONTROL_SELECT_DDC4                     = 0x3,
	DOUT_I2C_CONTROL_SELECT_DDC5                     = 0x4,
	DOUT_I2C_CONTROL_SELECT_DDC6                     = 0x5,
	DOUT_I2C_CONTROL_SELECT_DDCVGA                   = 0x6,
} DOUT_I2C_CONTROL_DDC_SELECT;
typedef enum DOUT_I2C_CONTROL_TRANSACTION_COUNT {
	DOUT_I2C_CONTROL_TRANS0                          = 0x0,
	DOUT_I2C_CONTROL_TRANS0_TRANS1                   = 0x1,
	DOUT_I2C_CONTROL_TRANS0_TRANS1_TRANS2            = 0x2,
	DOUT_I2C_CONTROL_TRANS0_TRANS1_TRANS2_TRANS3     = 0x3,
} DOUT_I2C_CONTROL_TRANSACTION_COUNT;
typedef enum DOUT_I2C_CONTROL_DBG_REF_SEL {
	DOUT_I2C_CONTROL_NORMAL_DEBUG                    = 0x0,
	DOUT_I2C_CONTROL_FAST_REFERENCE_DEBUG            = 0x1,
} DOUT_I2C_CONTROL_DBG_REF_SEL;
typedef enum DOUT_I2C_ARBITRATION_SW_PRIORITY {
	DOUT_I2C_ARBITRATION_SW_PRIORITY_NORMAL          = 0x0,
	DOUT_I2C_ARBITRATION_SW_PRIORITY_HIGH            = 0x1,
	DOUT_I2C_ARBITRATION_SW_PRIORITY_0_RESERVED      = 0x2,
	DOUT_I2C_ARBITRATION_SW_PRIORITY_1_RESERVED      = 0x3,
} DOUT_I2C_ARBITRATION_SW_PRIORITY;
typedef enum DOUT_I2C_ARBITRATION_NO_QUEUED_SW_GO {
	DOUT_I2C_ARBITRATION_SW_QUEUE_ENABLED            = 0x0,
	DOUT_I2C_ARBITRATION_SW_QUEUE_DISABLED           = 0x1,
} DOUT_I2C_ARBITRATION_NO_QUEUED_SW_GO;
typedef enum DOUT_I2C_ARBITRATION_ABORT_XFER {
	DOUT_I2C_ARBITRATION_NOT_ABORT_CURRENT_TRANSFER  = 0x0,
	DOUT_I2C_ARBITRATION_ABORT_CURRENT_TRANSFER      = 0x1,
} DOUT_I2C_ARBITRATION_ABORT_XFER;
typedef enum DOUT_I2C_ARBITRATION_USE_I2C_REG_REQ {
	DOUT_I2C_ARBITRATION__NOT_USE_I2C_REG_REQ        = 0x0,
	DOUT_I2C_ARBITRATION__USE_I2C_REG_REQ            = 0x1,
} DOUT_I2C_ARBITRATION_USE_I2C_REG_REQ;
typedef enum DOUT_I2C_ARBITRATION_DONE_USING_I2C_REG {
	DOUT_I2C_ARBITRATION_DONE__NOT_USING_I2C_REG     = 0x0,
	DOUT_I2C_ARBITRATION_DONE__USING_I2C_REG         = 0x1,
} DOUT_I2C_ARBITRATION_DONE_USING_I2C_REG;
typedef enum DOUT_I2C_ACK {
	DOUT_I2C_NO_ACK                                  = 0x0,
	DOUT_I2C_ACK_TO_CLEAN                            = 0x1,
} DOUT_I2C_ACK;
typedef enum DOUT_I2C_DDC_SPEED_THRESHOLD {
	DOUT_I2C_DDC_SPEED_THRESHOLD_BIG_THAN_ZERO       = 0x0,
	DOUT_I2C_DDC_SPEED_THRESHOLD_QUATER_OF_TOTAL_SAMPLE= 0x1,
	DOUT_I2C_DDC_SPEED_THRESHOLD_HALF_OF_TOTAL_SAMPLE= 0x2,
	DOUT_I2C_DDC_SPEED_THRESHOLD_THREE_QUATERS_OF_TOTAL_SAMPLE= 0x3,
} DOUT_I2C_DDC_SPEED_THRESHOLD;
typedef enum DOUT_I2C_DDC_SETUP_DATA_DRIVE_EN {
	DOUT_I2C_DDC_SETUP_DATA_DRIVE_BY_EXTERNAL_RESISTOR= 0x0,
	DOUT_I2C_DDC_SETUP_I2C_PAD_DRIVE_SDA             = 0x1,
} DOUT_I2C_DDC_SETUP_DATA_DRIVE_EN;
typedef enum DOUT_I2C_DDC_SETUP_DATA_DRIVE_SEL {
	DOUT_I2C_DDC_SETUP_DATA_DRIVE_FOR_10MCLKS        = 0x0,
	DOUT_I2C_DDC_SETUP_DATA_DRIVE_FOR_20MCLKS        = 0x1,
} DOUT_I2C_DDC_SETUP_DATA_DRIVE_SEL;
typedef enum DOUT_I2C_DDC_SETUP_EDID_DETECT_MODE {
	DOUT_I2C_DDC_SETUP_EDID_DETECT_CONNECT           = 0x0,
	DOUT_I2C_DDC_SETUP_EDID_DETECT_DISCONNECT        = 0x1,
} DOUT_I2C_DDC_SETUP_EDID_DETECT_MODE;
typedef enum DOUT_I2C_DDC_SETUP_CLK_DRIVE_EN {
	DOUT_I2C_DDC_SETUP_CLK_DRIVE_BY_EXTERNAL_RESISTOR= 0x0,
	DOUT_I2C_DDC_SETUP_I2C_PAD_DRIVE_SCL             = 0x1,
} DOUT_I2C_DDC_SETUP_CLK_DRIVE_EN;
typedef enum DOUT_I2C_TRANSACTION_STOP_ON_NACK {
	DOUT_I2C_TRANSACTION_STOP_CURRENT_TRANS          = 0x0,
	DOUT_I2C_TRANSACTION_STOP_ALL_TRANS              = 0x1,
} DOUT_I2C_TRANSACTION_STOP_ON_NACK;
typedef enum DOUT_I2C_DATA_INDEX_WRITE {
	DOUT_I2C_DATA__NOT_INDEX_WRITE                   = 0x0,
	DOUT_I2C_DATA__INDEX_WRITE                       = 0x1,
} DOUT_I2C_DATA_INDEX_WRITE;
typedef enum DOUT_I2C_EDID_DETECT_CTRL_SEND_RESET {
	DOUT_I2C_EDID_NOT_SEND_RESET_BEFORE_EDID_READ_TRACTION= 0x0,
	DOUT_I2C_EDID_SEND_RESET_BEFORE_EDID_READ_TRACTION= 0x1,
} DOUT_I2C_EDID_DETECT_CTRL_SEND_RESET;
typedef enum DOUT_I2C_READ_REQUEST_INTERRUPT_TYPE {
	DOUT_I2C_READ_REQUEST_INTERRUPT_TYPE__LEVEL      = 0x0,
	DOUT_I2C_READ_REQUEST_INTERRUPT_TYPE__PULSE      = 0x1,
} DOUT_I2C_READ_REQUEST_INTERRUPT_TYPE;
typedef enum BLNDV_CONTROL_BLND_MODE {
	BLNDV_CONTROL_BLND_MODE_CURRENT_PIPE_ONLY        = 0x0,
	BLNDV_CONTROL_BLND_MODE_OTHER_PIPE_ONLY          = 0x1,
	BLNDV_CONTROL_BLND_MODE_ALPHA_BLENDING_MODE      = 0x2,
	BLNDV_CONTROL_BLND_MODE_OTHER_STEREO_TYPE        = 0x3,
} BLNDV_CONTROL_BLND_MODE;
typedef enum BLNDV_CONTROL_BLND_STEREO_TYPE {
	BLNDV_CONTROL_BLND_STEREO_TYPE_NON_SINGLE_PIPE_STEREO= 0x0,
	BLNDV_CONTROL_BLND_STEREO_TYPE_SIDE_BY_SIDE_SINGLE_PIPE_STEREO= 0x1,
	BLNDV_CONTROL_BLND_STEREO_TYPE_TOP_BOTTOM_SINGLE_PIPE_STEREO= 0x2,
	BLNDV_CONTROL_BLND_STEREO_TYPE_UNUSED            = 0x3,
} BLNDV_CONTROL_BLND_STEREO_TYPE;
typedef enum BLNDV_CONTROL_BLND_STEREO_POLARITY {
	BLNDV_CONTROL_BLND_STEREO_POLARITY_LOW           = 0x0,
	BLNDV_CONTROL_BLND_STEREO_POLARITY_HIGH          = 0x1,
} BLNDV_CONTROL_BLND_STEREO_POLARITY;
typedef enum BLNDV_CONTROL_BLND_FEEDTHROUGH_EN {
	BLNDV_CONTROL_BLND_FEEDTHROUGH_EN_FALSE          = 0x0,
	BLNDV_CONTROL_BLND_FEEDTHROUGH_EN_TRUE           = 0x1,
} BLNDV_CONTROL_BLND_FEEDTHROUGH_EN;
typedef enum BLNDV_CONTROL_BLND_ALPHA_MODE {
	BLNDV_CONTROL_BLND_ALPHA_MODE_CURRENT_PIXEL_ALPHA= 0x0,
	BLNDV_CONTROL_BLND_ALPHA_MODE_PIXEL_ALPHA_COMBINED_GLOBAL_GAIN= 0x1,
	BLNDV_CONTROL_BLND_ALPHA_MODE_GLOBAL_ALPHA_ONLY  = 0x2,
	BLNDV_CONTROL_BLND_ALPHA_MODE_UNUSED             = 0x3,
} BLNDV_CONTROL_BLND_ALPHA_MODE;
typedef enum BLNDV_CONTROL_BLND_MULTIPLIED_MODE {
	BLNDV_CONTROL_BLND_MULTIPLIED_MODE_FALSE         = 0x0,
	BLNDV_CONTROL_BLND_MULTIPLIED_MODE_TRUE          = 0x1,
} BLNDV_CONTROL_BLND_MULTIPLIED_MODE;
typedef enum BLNDV_SM_CONTROL2_SM_MODE {
	BLNDV_SM_CONTROL2_SM_MODE_SINGLE_PLANE           = 0x0,
	BLNDV_SM_CONTROL2_SM_MODE_ROW_SUBSAMPLING        = 0x2,
	BLNDV_SM_CONTROL2_SM_MODE_COLUMN_SUBSAMPLING     = 0x4,
	BLNDV_SM_CONTROL2_SM_MODE_CHECKERBOARD_SUBSAMPLING= 0x6,
} BLNDV_SM_CONTROL2_SM_MODE;
typedef enum BLNDV_SM_CONTROL2_SM_FRAME_ALTERNATE {
	BLNDV_SM_CONTROL2_SM_FRAME_ALTERNATE_FALSE       = 0x0,
	BLNDV_SM_CONTROL2_SM_FRAME_ALTERNATE_TRUE        = 0x1,
} BLNDV_SM_CONTROL2_SM_FRAME_ALTERNATE;
typedef enum BLNDV_SM_CONTROL2_SM_FIELD_ALTERNATE {
	BLNDV_SM_CONTROL2_SM_FIELD_ALTERNATE_FALSE       = 0x0,
	BLNDV_SM_CONTROL2_SM_FIELD_ALTERNATE_TRUE        = 0x1,
} BLNDV_SM_CONTROL2_SM_FIELD_ALTERNATE;
typedef enum BLNDV_SM_CONTROL2_SM_FORCE_NEXT_FRAME_POL {
	BLNDV_SM_CONTROL2_SM_FORCE_NEXT_FRAME_POL_NO_FORCE= 0x0,
	BLNDV_SM_CONTROL2_SM_FORCE_NEXT_FRAME_POL_RESERVED= 0x1,
	BLNDV_SM_CONTROL2_SM_FORCE_NEXT_FRAME_POL_FORCE_LOW= 0x2,
	BLNDV_SM_CONTROL2_SM_FORCE_NEXT_FRAME_POL_FORCE_HIGH= 0x3,
} BLNDV_SM_CONTROL2_SM_FORCE_NEXT_FRAME_POL;
typedef enum BLNDV_SM_CONTROL2_SM_FORCE_NEXT_TOP_POL {
	BLNDV_SM_CONTROL2_SM_FORCE_NEXT_TOP_POL_NO_FORCE = 0x0,
	BLNDV_SM_CONTROL2_SM_FORCE_NEXT_TOP_POL_RESERVED = 0x1,
	BLNDV_SM_CONTROL2_SM_FORCE_NEXT_TOP_POL_FORCE_LOW= 0x2,
	BLNDV_SM_CONTROL2_SM_FORCE_NEXT_TOP_POL_FORCE_HIGH= 0x3,
} BLNDV_SM_CONTROL2_SM_FORCE_NEXT_TOP_POL;
typedef enum BLNDV_CONTROL2_PTI_ENABLE {
	BLNDV_CONTROL2_PTI_ENABLE_FALSE                  = 0x0,
	BLNDV_CONTROL2_PTI_ENABLE_TRUE                   = 0x1,
} BLNDV_CONTROL2_PTI_ENABLE;
typedef enum BLNDV_CONTROL2_BLND_SUPERAA_DEGAMMA_EN {
	BLNDV_CONTROL2_BLND_SUPERAA_DEGAMMA_EN_FALSE     = 0x0,
	BLNDV_CONTROL2_BLND_SUPERAA_DEGAMMA_EN_TRUE      = 0x1,
} BLNDV_CONTROL2_BLND_SUPERAA_DEGAMMA_EN;
typedef enum BLNDV_CONTROL2_BLND_SUPERAA_REGAMMA_EN {
	BLNDV_CONTROL2_BLND_SUPERAA_REGAMMA_EN_FALSE     = 0x0,
	BLNDV_CONTROL2_BLND_SUPERAA_REGAMMA_EN_TRUE      = 0x1,
} BLNDV_CONTROL2_BLND_SUPERAA_REGAMMA_EN;
typedef enum BLNDV_UNDERFLOW_INTERRUPT_BLND_UNDERFLOW_INT_ACK {
	BLNDV_UNDERFLOW_INTERRUPT_BLND_UNDERFLOW_INT_ACK_FALSE= 0x0,
	BLNDV_UNDERFLOW_INTERRUPT_BLND_UNDERFLOW_INT_ACK_TRUE= 0x1,
} BLNDV_UNDERFLOW_INTERRUPT_BLND_UNDERFLOW_INT_ACK;
typedef enum BLNDV_UNDERFLOW_INTERRUPT_BLND_UNDERFLOW_INT_MASK {
	BLNDV_UNDERFLOW_INTERRUPT_BLND_UNDERFLOW_INT_MASK_FALSE= 0x0,
	BLNDV_UNDERFLOW_INTERRUPT_BLND_UNDERFLOW_INT_MASK_TRUE= 0x1,
} BLNDV_UNDERFLOW_INTERRUPT_BLND_UNDERFLOW_INT_MASK;
typedef enum BLNDV_V_UPDATE_LOCK_BLND_DCP_GRPH_V_UPDATE_LOCK {
	BLNDV_V_UPDATE_LOCK_BLND_DCP_GRPH_V_UPDATE_LOCK_FALSE= 0x0,
	BLNDV_V_UPDATE_LOCK_BLND_DCP_GRPH_V_UPDATE_LOCK_TRUE= 0x1,
} BLNDV_V_UPDATE_LOCK_BLND_DCP_GRPH_V_UPDATE_LOCK;
typedef enum BLNDV_V_UPDATE_LOCK_BLND_DCP_GRPH_SURF_V_UPDATE_LOCK {
	BLNDV_V_UPDATE_LOCK_BLND_DCP_GRPH_SURF_V_UPDATE_LOCK_FALSE= 0x0,
	BLNDV_V_UPDATE_LOCK_BLND_DCP_GRPH_SURF_V_UPDATE_LOCK_TRUE= 0x1,
} BLNDV_V_UPDATE_LOCK_BLND_DCP_GRPH_SURF_V_UPDATE_LOCK;
typedef enum BLNDV_V_UPDATE_LOCK_BLND_DCP_CUR_V_UPDATE_LOCK {
	BLNDV_V_UPDATE_LOCK_BLND_DCP_CUR_V_UPDATE_LOCK_FALSE= 0x0,
	BLNDV_V_UPDATE_LOCK_BLND_DCP_CUR_V_UPDATE_LOCK_TRUE= 0x1,
} BLNDV_V_UPDATE_LOCK_BLND_DCP_CUR_V_UPDATE_LOCK;
typedef enum BLNDV_V_UPDATE_LOCK_BLND_DCP_CUR2_V_UPDATE_LOCK {
	BLNDV_V_UPDATE_LOCK_BLND_DCP_CUR2_V_UPDATE_LOCK_FALSE= 0x0,
	BLNDV_V_UPDATE_LOCK_BLND_DCP_CUR2_V_UPDATE_LOCK_TRUE= 0x1,
} BLNDV_V_UPDATE_LOCK_BLND_DCP_CUR2_V_UPDATE_LOCK;
typedef enum BLNDV_V_UPDATE_LOCK_BLND_SCL_V_UPDATE_LOCK {
	BLNDV_V_UPDATE_LOCK_BLND_SCL_V_UPDATE_LOCK_FALSE = 0x0,
	BLNDV_V_UPDATE_LOCK_BLND_SCL_V_UPDATE_LOCK_TRUE  = 0x1,
} BLNDV_V_UPDATE_LOCK_BLND_SCL_V_UPDATE_LOCK;
typedef enum BLNDV_V_UPDATE_LOCK_BLND_BLND_V_UPDATE_LOCK {
	BLNDV_V_UPDATE_LOCK_BLND_BLND_V_UPDATE_LOCK_FALSE= 0x0,
	BLNDV_V_UPDATE_LOCK_BLND_BLND_V_UPDATE_LOCK_TRUE = 0x1,
} BLNDV_V_UPDATE_LOCK_BLND_BLND_V_UPDATE_LOCK;
typedef enum BLNDV_V_UPDATE_LOCK_BLND_V_UPDATE_LOCK_MODE {
	BLNDV_V_UPDATE_LOCK_BLND_V_UPDATE_LOCK_MODE_FALSE= 0x0,
	BLNDV_V_UPDATE_LOCK_BLND_V_UPDATE_LOCK_MODE_TRUE = 0x1,
} BLNDV_V_UPDATE_LOCK_BLND_V_UPDATE_LOCK_MODE;
typedef enum BLNDV_DEBUG_BLND_CNV_MUX_SELECT {
	BLNDV_DEBUG_BLND_CNV_MUX_SELECT_LOW              = 0x0,
	BLNDV_DEBUG_BLND_CNV_MUX_SELECT_HIGH             = 0x1,
} BLNDV_DEBUG_BLND_CNV_MUX_SELECT;
typedef enum BLNDV_TEST_DEBUG_INDEX_BLND_TEST_DEBUG_WRITE_EN {
	BLNDV_TEST_DEBUG_INDEX_BLND_TEST_DEBUG_WRITE_EN_FALSE= 0x0,
	BLNDV_TEST_DEBUG_INDEX_BLND_TEST_DEBUG_WRITE_EN_TRUE= 0x1,
} BLNDV_TEST_DEBUG_INDEX_BLND_TEST_DEBUG_WRITE_EN;

#endif /* DCE_11_0_ENUM_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * DCE_11_0 Register documentation
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
 * THE S