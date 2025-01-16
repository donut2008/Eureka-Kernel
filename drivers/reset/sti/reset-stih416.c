= 0x37,
	DBG_BLOCK_ID_LDS10_BY4                           = 0x38,
	DBG_BLOCK_ID_LDS14_BY4                           = 0x39,
	DBG_BLOCK_ID_LDS18_BY4                           = 0x3a,
	DBG_BLOCK_ID_UNUSED40_BY4                        = 0x3b,
} DebugBlockId_BY4;
typedef enum DebugBlockId_BY8 {
	DBG_BLOCK_ID_RESERVED_BY8                        = 0x0,
	DBG_BLOCK_ID_CSC_BY8                             = 0x1,
	DBG_BLOCK_ID_SDMA0_BY8                           = 0x2,
	DBG_BLOCK_ID_CP0_BY8                             = 0x3,
	DBG_BLOCK_ID_SXM0_BY8                            = 0x4,
	DBG_BLOCK_ID_TCA_BY8                             = 0x5,
	DBG_BLOCK_ID_MCD_BY8                             = 0x6,
	DBG_BLOCK_ID_SQA_BY8                             = 0x7,
	DBG_BLOCK_ID_SQB_BY8                             = 0x8,
	DBG_BLOCK_ID_CB_BY8                              = 0x9,
	DBG_BLOCK_ID_SXS_BY8                             = 0xa,
	DBG_BLOCK_ID_DB_BY8                              = 0xb,
	DBG_BLOCK_ID_TCP_BY8                             = 0xc,
	DBG_BLOCK_ID_TCP8_BY8                            = 0xd,
	DBG_BLOCK_ID_TCP16_BY8                           = 0xe,
	DBG_BLOCK_ID_TCP_RESERVED0_BY8                   = 0xf,
	DBG_BLOCK_ID_TCC_BY8                             = 0x10,
	DBG_BLOCK_ID_SPS_BY8                             = 0x11,
	DBG_BLOCK_ID_TA_BY8                              = 0x12,
	DBG_BLOCK_ID_TA08_BY8                            = 0x13,
	DBG_BLOCK_ID_TA10_BY8                            = 0x14,
	DBG_BLOCK_ID_TA18_BY8                            = 0x15,
	DBG_BLOCK_ID_TD_BY8                              = 0x16,
	DBG_BLOCK_ID_TD08_BY8                            = 0x17,
	DBG_BLOCK_ID_TD10_BY8                            = 0x18,
	DBG_BLOCK_ID_TD18_BY8                            = 0x19,
	DBG_BLOCK_ID_LDS_BY8                             = 0x1a,
	DBG_BLOCK_ID_LDS08_BY8                           = 0x1b,
	DBG_BLOCK_ID_LDS10_BY8                           = 0x1c,
	DBG_BLOCK_ID_LDS18_BY8                           = 0x1d,
} DebugBlockId_BY8;
typedef enum DebugBlockId_BY16 {
	DBG_BLOCK_ID_RESERVED_BY16                       = 0x0,
	DBG_BLOCK_ID_SDMA0_BY16                          = 0x1,
	DBG_BLOCK_ID_SXM_BY16                            = 0x2,
	DBG_BLOCK_ID_MCD_BY16                            = 0x3,
	DBG_BLOCK_ID_SQB_BY16                            = 0x4,
	DBG_BLOCK_ID_SXS_BY16                            = 0x5,
	DBG_BLOCK_ID_TCP_BY16                            = 0x6,
	DBG_BLOCK_ID_TCP16_BY16                          = 0x7,
	DBG_BLOCK_ID_TCC_BY16                            = 0x8,
	DBG_BLOCK_ID_TA_BY16                             = 0x9,
	DBG_BLOCK_ID_TA10_BY16                           = 0xa,
	DBG_BLOCK_ID_TD_BY16                             = 0xb,
	DBG_BLOCK_ID_TD10_BY16                           = 0xc,
	DBG_BLOCK_ID_LDS_BY16                            = 0xd,
	DBG_BLOCK_ID_LDS10_BY16                          = 0xe,
} DebugBlockId_BY16;
typedef enum SurfaceEndian {
	ENDIAN_NONE                                      = 0x0,
	ENDIAN_8IN16                                     = 0x1,
	ENDIAN_8IN32                                     = 0x2,
	ENDIAN_8IN64                                     = 0x3,
} SurfaceEndian;
typedef enum ArrayMode {
	ARRAY_LINEAR_GENERAL                             = 0x0,
	ARRAY_LINEAR_ALIGNED                             = 0x1,
	ARRAY_1D_TILED_THIN1                             = 0x2,
	ARRAY_1D_TILED_THICK                             = 0x3,
	ARRAY_2D_TILED_THIN1                             = 0x4,
	ARRAY_PRT_TILED_THIN1                            = 0x5,
	ARRAY_PRT_2D_TILED_THIN1                         = 0x6,
	ARRAY_2D_TILED_THICK                             = 0x7,
	ARRAY_2D_TILED_XTHICK                            = 0x8,
	ARRAY_PRT_TILED_THICK                            = 0x9,
	ARRAY_PRT_2D_TILED_THICK                         = 0xa,
	ARRAY_PRT_3D_TILED_THIN1                         = 0xb,
	ARRAY_3D_TILED_THIN1                             = 0xc,
	ARRAY_3D_TILED_THICK                             = 0xd,
	ARRAY_3D_TILED_XTHICK                            = 0xe,
	ARRAY_PRT_3D_TILED_THICK                         = 0xf,
} ArrayMode;
typedef enum PipeTiling {
	CONFIG_1_PIPE                                    = 0x0,
	CONFIG_2_PIPE                                    = 0x1,
	CONFIG_4_PIPE                                    = 0x2,
	CONFIG_8_PIPE                                    = 0x3,
} PipeTiling;
typedef enum BankTiling {
	CONFIG_4_BANK                                    = 0x0,
	CONFIG_8_BANK                                    = 0x1,
} BankTiling;
typedef enum GroupInterleave {
	CONFIG_256B_GROUP                                = 0x0,
	CONFIG_512B_GROUP                                = 0x1,
} GroupInterleave;
typedef enum RowTiling {
	CONFIG_1KB_ROW                                   = 0x0,
	CONFIG_2KB_ROW                                   = 0x1,
	CONFIG_4KB_ROW                                   = 0x2,
	CONFIG_8KB_ROW                                   = 0x3,
	CONFIG_1KB_ROW_OPT                               = 0x4,
	CONFIG_2KB_ROW_OPT                               = 0x5,
	CONFIG_4KB_ROW_OPT                               = 0x6,
	CONFIG_8KB_ROW_OPT                               = 0x7,
} RowTiling;
typedef enum BankSwapBytes {
	CONFIG_128B_SWAPS                                = 0x0,
	CONFIG_256B_SWAPS                                = 0x1,
	CONFIG_512B_SWAPS                                = 0x2,
	CONFIG_1KB_SWAPS                                 = 0x3,
} BankSwapBytes;
typedef enum SampleSplitBytes {
	CONFIG_1KB_SPLIT                                 = 0x0,
	CONFIG_2KB_SPLIT                                 = 0x1,
	CONFIG_4KB_SPLIT                                 = 0x2,
	CONFIG_8KB_SPLIT                                 = 0x3,
} SampleSplitBytes;
typedef enum NumPipes {
	ADDR