INEAR_GENERAL                             = 0x0,
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
	ADDR_CONFIG_1_PIPE                               = 0x0,
	ADDR_CONFIG_2_PIPE                               = 0x1,
	ADDR_CONFIG_4_PIPE                               = 0x2,
	ADDR_CONFIG_8_PIPE                               = 0x3,
} NumPipes;
typedef enum PipeInterleaveSize {
	ADDR_CONFIG_PIPE_INTERLEAVE_256B                 = 0x0,
	ADDR_CONFIG_PIPE_INTERLEAVE_512B                 = 0x1,
} PipeInterleaveSize;
typedef enum BankInterleaveSize {
	ADDR_CONFIG_BANK_INTERLEAVE_1                    = 0x0,
	ADDR_CONFIG_BANK_INTERLEAVE_2                    = 0x1,
	ADDR_CONFIG_BANK_INTERLEAVE_4                    = 0x2,
	ADDR_CONFIG_BANK_INTERLEAVE_8                    = 0x3,
} BankInterleaveSize;
typedef enum NumShaderEngines {
	ADDR_CONFIG_1_SHADER_ENGINE                      = 0x0,
	ADDR_CONFIG_2_SHADER_ENGINE                      = 0x1,
} NumShaderEngines;
typedef enum ShaderEngineTileSize {
	ADDR_CONFIG_SE_TILE_16                           = 0x0,
	ADDR_CONFIG_SE_TILE_32                           = 0x1,
} ShaderEngineTileSize;
typedef enum NumGPUs {
	ADDR_CONFIG_1_GPU                                = 0x0,
	ADDR_CONFIG_2_GPU                                = 0x1,
	ADDR_CONFIG_4_GPU                                = 0x2,
} NumGPUs;
typedef enum MultiGPUTileSize {
	ADDR_CONFIG_GPU_TILE_16                          = 0x0,
	ADDR_CONFIG_GPU_TILE_32                          = 0x1,
	ADDR_CONFIG_GPU_TILE_64                          = 0x2,
	ADDR_CONFIG_GPU_TILE_128                         = 0x3,
} MultiGPUTileSize;
typedef enum RowSize {
	ADDR_CONFIG_1KB_ROW                              = 0x0,
	ADDR_CONFIG_2KB_ROW                              = 0x1,
	ADDR_CONFIG_4KB_ROW                              = 0x2,
} RowSize;
typedef enum NumLowerPipes {
	ADDR_CONFIG_1_LOWER_PIPES                        = 0x0,
	ADDR_CONFIG_2_LOWER_PIPES                        = 0x1,
} NumLowerPipes;
typedef enum DebugBlockId {
	DBG_CLIENT_BLKID_RESERVED                        = 0x0,
	DBG_CLIENT_BLKID_dbg                             = 0x1,
	DBG_CLIENT_BLKID_scf2                            = 0x2,
	DBG_CLIENT_BLKID_mcd5_0                          = 0x3,
	DBG_CLIENT_BLKID_mcd5_1                          = 0x4,
	DBG_CLIENT_BLKID_mcd6_0                          = 0x5,
	DBG_CLIENT_BLKID_mcd6_1                          = 0x6,
	DBG_CLIENT_BLKID_mcd7_0                          = 0x7,
	DBG_CLIENT_BLKID_mcd7_1                          = 0x8,
	DBG_CLIENT_BLKID_vmc                             = 0x9,
	DBG_CLIENT_BLKID_sx30                            = 0xa,
	DBG_CLIENT_BLKID_mcd2_0                          = 0xb,
	DBG_CLIENT_BLKID_mcd2_1                          = 0xc,
	DBG_CLIENT_BLKID_bci1                            = 0xd,
	DBG_CLIENT_BLKID_xdma_dbg_client_wrapper         = 0xe,
	DBG_CLIENT_BLKID_mcc0                            = 0xf,
	DBG_CLIENT_BLKID_uvdf_0                          = 0x10,
	DBG_CLIENT_BLKID_uvdf_1                          = 0x11,
	DBG_CLIENT_BLKID_uvdf_2                          = 0x12,
	DBG_CLIENT_BLKID_bci0                            = 0x13,
	DBG_CLIENT_BLKID_vcec0_0                         = 0x14,
	DBG_CLIENT_BLKID_cb100                           = 0x15,
	DBG_CLIENT_BLKID_cb001                           = 0x16,
	DBG_CLIENT_BLKID_cb002                           = 0x17,
	DBG_CLIENT_BLKID_cb003                           = 0x18,
	DBG_CLIENT_BLKID_mcd4_0                          = 0x19,
	DBG_CLIENT_BLKID_mcd4_1                          = 0x1a,
	DBG_CLIENT_BLKID_tmonw00                         = 0x1b,
	DBG_CLIENT_BLKID_cb101                           = 0x1c,
	DBG_CLIENT_BLKID_cb102                           = 0x1d,
	DBG_CLIENT_BLKID_cb103                           = 0x1e,
	DBG_CLIENT_BLKID_sx10                            = 0x1f,
	DBG_CLIENT_BLKID_cb301                           = 0x20,
	DBG_CLIENT_BLKID_cb302                           = 0x21,
	DBG_CLIENT_BLKID_cb303                           = 0x22,
	DBG_CLIENT_BLKID_tmonw01                         = 0x23,
	DBG_CLIENT_BLKID_tmonw02                         = 0x24,
	DBG_CLIENT_BLKID_vcea0_0                         = 0x25,
	DBG_CLIENT_BLKID_vcea0_1                         = 0x26,
	DBG_CLIENT_BLKID_vcea0_2                         = 0x27,
	DBG_CLIENT_BLKID_vcea0_3                         = 0x28,
	DBG_CLIENT_BLKID_scf1                            = 0x29,
	DBG_CLIENT_BLKID_sx20                            = 0x2a,
	DBG_CLIENT_BLKID_spim1                           = 0x2b,
	DBG_CLIENT_BLKID_scb1                            = 0x2c,
	DBG_CLIENT_BLKID_pa10                            = 0x2d,
	DBG_CLIENT_BLKID_pa00                            = 0x2e,
	DBG_CLIENT_BLKID_gmcon                           = 0x2f,
	DBG_CLIENT_BLKID_mcb                             = 0x30,
	DBG_CLIENT_BLKID_vgt0                            = 0x31,
	DBG_CLIENT_BLKID_pc0                             = 0x32,
	DBG_CLIENT_BLKID_bci2                            = 0x33,
	DBG_CLIENT_BLKID_uvdb_0                          = 0x34,
	DBG_CLIENT_BLKID_spim3                           = 0x35,
	DBG_CLIENT_BLKID_scb3                            = 0x36,
	DBG_CLIENT_BLKID_cpc_0                           = 0x37,
	DBG_CLIENT_BLKID_cpc_1                           = 0x38,
	DBG_CLIENT_BLKID_uvdm_0                          = 0x39,
	DBG_CLIENT_BLKID_uvdm_1                          = 0x3a,
	DBG_CLIENT_BLKID_uvdm_2                          = 0x3b,
	DBG_CLIENT_BLKID_uvdm_3                          = 0x3c,
	DBG_CLIENT_BLKID_cb000                           = 0x3d,
	DBG_CLIENT_BLKID_spim0                           = 0x3e,
	DBG_CLIENT_BLKID_scb0                            = 0x3f,
	DBG_CLIENT_BLKID_mcc2                            = 0x40,
	DBG_CLIENT_BLKID_ds0                             = 0x41,
	DBG_CLIENT_BLKID_srbm                            = 0x42,
	DBG_CLIENT_BLKID_ih                              = 0x43,
	DBG_CLIENT_BLKID_sem                             = 0x44,
	DBG_CLIENT_BLKID_sdma_0                          = 0x45,
	DBG_CLIENT_BLKID_sdma_1                          = 0x46,
	DBG_CLIENT_BLKID_hdp                             = 0x47,
	DBG_CLIENT_BLKID_acp_0                           = 0x48,
	DBG_CLIENT_BLKID_acp_1                           = 0x49,
	DBG_CLIENT_BLKID_cb200                           = 0x4a,
	DBG_CLIENT_BLKID_scf3                            = 0x4b,
	DBG_CLIENT_BLKID_bci3                            = 0x4c,
	DBG_CLIENT_BLKID_mcd0_0                          = 0x4d,
	DBG_CLIENT_BLKID_mcd0_1                          = 0x4e,
	DBG_CLIENT_BLKID_pa11                            = 0x4f,
	DBG_CLIENT_BLKID_pa01                            = 0x50,
	DBG_CLIENT_BLKID_cb201                           = 0x51,
	DBG_CLIENT_BLKID_cb202                           = 0x52,
	DBG_CLIENT_BLKID_cb203                           = 0x53,
	DBG_CLIENT_BLKID_spim2                           = 0x54,
	DBG_CLIENT_BLKID_scb2                            = 0x55,
	DBG_CLIENT_BLKID_vgt2                            = 0x56,
	DBG_CLIENT_BLKID_pc2                             = 0x57,
	DBG_CLIENT_BLKID_smu_0                           = 0x58,
	DBG_CLIENT_BLKID_smu_1                           = 0x59,
	DBG_CLIENT_BLKID_smu_2                           = 0x5a,
	DBG_CLIENT_BLKID_cb1                             = 0x5b,
	DBG_CLIENT_BLKID_ia0                             = 0x5c,
	DBG_CLIENT_BLKID_wd                              = 0x5d,
	DBG_CLIENT_BLKID_ia1                             = 0x5e,
	DBG_CLIENT_BLKID_scf0                            = 0x5f,
	DBG_CLIENT_BLKID_vgt1                            = 0x60,
	DBG_CLIENT_BLKID_pc1                             = 0x61,
	DBG_CLIENT_BLKID_cb0                             = 0x62,
	DBG_CLIENT_BLKID_gdc_one_0                       = 0x63,
	DBG_CLIENT_BLKID_gdc_one_1                       = 0x64,
	DBG_CLIENT_BLKID_gdc_one_2                       = 0x65,
	DBG_CLIENT_BLKID_gdc_one_3                       = 0x66,
	DBG_CLIENT_BLKID_gdc_one_4                       = 0x67,
	DBG_CLIENT_BLKID_gdc_one_5                       = 0x68,
	DBG_CLIENT_BLKID_gdc_one_6                       = 0x69,
	DBG_CLIENT_BLKID_gdc_one_7                       = 0x6a,
	DBG_CLIENT_BLKID_gdc_one_8                       = 0x6b,
	DBG_CLIENT_BLKID_gdc_one_9                       = 0x6c,
	DBG_CLIENT_BLKID_gdc_one_10                      = 0x6d,
	DBG_CLIENT_BLKID_gdc_one_11                      = 0x6e,
	DBG_CLIENT_BLKID_gdc_one_12                      = 0x6f,
	DBG_CLIENT_BLKID_gdc_one_13                      = 0x70,
	DBG_CLIENT_BLKID_gdc_one_14                      = 0x71,
	DBG_CLIENT_BLKID_gdc_one_15                      = 0x72,
	DBG_CLIENT_BLKID_gdc_one_16                      = 0x73,
	DBG_CLIENT_BLKID_g