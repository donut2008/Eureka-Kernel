MASK 0x100
#define SDMA1_RLC1_CONTEXT_STATUS__CTXSW_READY__SHIFT 0x8
#define SDMA1_RLC1_CONTEXT_STATUS__PREEMPTED_MASK 0x200
#define SDMA1_RLC1_CONTEXT_STATUS__PREEMPTED__SHIFT 0x9
#define SDMA1_RLC1_CONTEXT_STATUS__PREEMPT_DISABLE_MASK 0x400
#define SDMA1_RLC1_CONTEXT_STATUS__PREEMPT_DISABLE__SHIFT 0xa
#define SDMA1_RLC1_DOORBELL__OFFSET_MASK 0x1fffff
#define SDMA1_RLC1_DOORBELL__OFFSET__SHIFT 0x0
#define SDMA1_RLC1_DOORBELL__ENABLE_MASK 0x10000000
#define SDMA1_RLC1_DOORBELL__ENABLE__SHIFT 0x1c
#define SDMA1_RLC1_DOORBELL__CAPTURED_MASK 0x40000000
#define SDMA1_RLC1_DOORBELL__CAPTURED__SHIFT 0x1e
#define SDMA1_RLC1_VIRTUAL_ADDR__ATC_MASK 0x1
#define SDMA1_RLC1_VIRTUAL_ADDR__ATC__SHIFT 0x0
#define SDMA1_RLC1_VIRTUAL_ADDR__INVAL_MASK 0x2
#define SDMA1_RLC1_VIRTUAL_ADDR__INVAL__SHIFT 0x1
#define SDMA1_RLC1_VIRTUAL_ADDR__PTR32_MASK 0x10
#define SDMA1_RLC1_VIRTUAL_ADDR__PTR32__SHIFT 0x4
#define SDMA1_RLC1_VIRTUAL_ADDR__SHARED_BASE_MASK 0x700
#define SDMA1_RLC1_VIRTUAL_ADDR__SHARED_BASE__SHIFT 0x8
#define SDMA1_RLC1_VIRTUAL_ADDR__VM_HOLE_MASK 0x40000000
#define SDMA1_RLC1_VIRTUAL_ADDR__VM_HOLE__SHIFT 0x1e
#define SDMA1_RLC1_APE1_CNTL__BASE_MASK 0xffff
#define SDMA1_RLC1_APE1_CNTL__BASE__SHIFT 0x0
#define SDMA1_RLC1_APE1_CNTL__LIMIT_MASK 0xffff0000
#define SDMA1_RLC1_APE1_CNTL__LIMIT__SHIFT 0x10
#define SDMA1_RLC1_DOORBELL_LOG__BE_ERROR_MASK 0x1
#define SDMA1_RLC1_DOORBELL_LOG__BE_ERROR__SHIFT 0x0
#define SDMA1_RLC1_DOORBELL_LOG__DATA_MASK 0xfffffffc
#define SDMA1_RLC1_DOORBELL_LOG__DATA__SHIFT 0x2
#define SDMA1_RLC1_WATERMARK__RD_OUTSTANDING_MASK 0xfff
#define SDMA1_RLC1_WATERMARK__RD_OUTSTANDING__SHIFT 0x0
#define SDMA1_RLC1_WATERMARK__WR_OUTSTANDING_MASK 0x1ff0000
#define SDMA1_RLC1_WATERMARK__WR_OUTSTANDING__SHIFT 0x10
#define SDMA1_RLC1_CSA_ADDR_LO__ADDR_MASK 0xfffffffc
#define SDMA1_RLC1_CSA_ADDR_LO__ADDR__SHIFT 0x2
#define SDMA1_RLC1_CSA_ADDR_HI__ADDR_MASK 0xffffffff
#define SDMA1_RLC1_CSA_ADDR_HI__ADDR__SHIFT 0x0
#define SDMA1_RLC1_IB_SUB_REMAIN__SIZE_MASK 0x3fff
#define SDMA1_RLC1_IB_SUB_REMAIN__SIZE__SHIFT 0x0
#define SDMA1_RLC1_PREEMPT__IB_PREEMPT_MASK 0x1
#define SDMA1_RLC1_PREEMPT__IB_PREEMPT__SHIFT 0x0
#define SDMA1_RLC1_DUMMY_REG__DUMMY_MASK 0xffffffff
#define SDMA1_RLC1_DUMMY_REG__DUMMY__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA0__DATA0_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA0__DATA0__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA1__DATA1_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA1__DATA1__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA2__DATA2_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA2__DATA2__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA3__DATA3_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA3__DATA3__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA4__DATA4_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA4__DATA4__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA5__DATA5_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA5__DATA5__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA6__DATA6_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA6__DATA6__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA7__DATA7_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA7__DATA7__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_DATA8__DATA8_MASK 0xffffffff
#define SDMA1_RLC1_MIDCMD_DATA8__DATA8__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_CNTL__DATA_VALID_MASK 0x1
#define SDMA1_RLC1_MIDCMD_CNTL__DATA_VALID__SHIFT 0x0
#define SDMA1_RLC1_MIDCMD_CNTL__COPY_MODE_MASK 0x2
#define SDMA1_RLC1_MIDCMD_CNTL__COPY_MODE__SHIFT 0x1
#define SDMA1_RLC1_MIDCMD_CNTL__SPLIT_STATE_MASK 0xf0
#define SDMA1_RLC1_MIDCMD_CNTL__SPLIT_STATE__SHIFT 0x4
#define SDMA1_RLC1_MIDCMD_CNTL__ALLOW_PREEMPT_MASK 0x100
#define SDMA1_RLC1_MIDCMD_CNTL__ALLOW_PREEMPT__SHIFT 0x8
#define HDP_HOST_PATH_CNTL__BIF_RDRET_CREDIT_MASK 0x7
#define HDP_HOST_PATH_CNTL__BIF_RDRET_CREDIT__SHIFT 0x0
#define HDP_HOST_PATH_CNTL__MC_WRREQ_CREDIT_MASK 0x1f8
#define HDP_HOST_PATH_CNTL__MC_WRREQ_CREDIT__SHIFT 0x3
#define HDP_HOST_PATH_CNTL__WR_STALL_TIMER_MASK 0x600
#define HDP_HOST_PATH_CNTL__WR_STALL_TIMER__SHIFT 0x9
#define HDP_HOST_PATH_CNTL__RD_STALL_TIMER_MASK 0x1800
#define HDP_HOST_PATH_CNTL__RD_STALL_TIMER__SHIFT 0xb
#define HDP_HOST_PATH_CNTL__WRITE_COMBINE_TIMER_MASK 0x180000
#define HDP_HOST_PATH_CNTL__WRITE_COMBINE_TIMER__SHIFT 0x13
#define HDP_HOST_PATH_CNTL__WRITE_COMBINE_EN_MASK 0x200000
#define HDP_HOST_PATH_CNTL__WRITE_COMBINE_EN__SHIFT 0x15
#define HDP_HOST_PATH_CNTL__CACHE_INVALIDATE_MASK 0x400000
#define HDP_HOST_PATH_CNTL__CACHE_INVALIDATE__SHIFT 0x16
#define HDP_HOST_PATH_CNTL__CLOCK_GATING_DIS_MASK 0x800000
#define HDP_HOST_PATH_CNTL__CLOCK_GATING_DIS__SHIFT 0x17
#define HDP_HOST_PATH_CNTL__REG_CLK_ENABLE_COUNT_MASK 0xf000000
#define HDP_HOST_PATH_CNTL__REG_CLK_ENABLE_COUNT__SHIFT 0x18
#define HDP_HOST_PATH_CNTL__ALL_SURFACES_DIS_MASK 0x20000000
#define HDP_HOST_PATH_CNTL__ALL_SURFACES_DIS__SHIFT 0x1d
#define HDP_HOST_PATH_CNTL__WRITE_THROUGH_CACHE_DIS_MASK 0x40000000
#define HDP_HOST_PATH_CNTL__WRITE_THROUGH_CACHE_DIS__SHIFT 0x1e
#define HDP_HOST_PATH_CNTL__LIN_RD_CACHE_DIS_MASK 0x80000000
#define HDP_HOST_PATH_CNTL__LIN_RD_CACHE_DIS__SHIFT 0x1f
#define HDP_NONSURFACE_BASE__NONSURF_BASE_MASK 0xffffffff
#define HDP_NONSURFACE_BASE__NONSURF_BASE__SHIFT 0x0
#define HDP_NONSURFACE_INFO__NONSURF_ADDR_TYPE_MASK 0x1
#define HDP_NONSURFACE_INFO__NONSURF_ADDR_TYPE__SHIFT 0x0
#define HDP_NONSURFACE_INFO__NONSURF_ARRAY_MODE_MASK 0x1e
#define HDP_NONSURFACE_INFO__NONSURF_ARRAY_MODE__SHIFT 0x1
#define HDP_NONSURFACE_INFO__NONSURF_ENDIAN_MASK 0x60
#define HDP_NONSURFACE_INFO__NONSURF_ENDIAN__SHIFT 0x5
#define HDP_NONSURFACE_INFO__NONSURF_PIXEL_SIZE_MASK 0x380
#define HDP_NONSURFACE_INFO__NONSURF_PIXEL_SIZE__SHIFT 0x7
#define HDP_NONSURFACE_INFO__NONSURF_SAMPLE_NUM_MASK 0x1c00
#define HDP_NONSURFACE_INFO__NONSURF_SAMPLE_NUM__SHIFT 0xa
#define HDP_NONSURFACE_INFO__NONSURF_SAMPLE_SIZE_MASK 0x6000
#define HDP_NONSURFACE_INFO__NONSURF_SAMPLE_SIZE__SHIFT 0xd
#define HDP_NONSURFACE_INFO__NONSURF_PRIV_MASK 0x8000
#define HDP_NONSURFACE_INFO__NONSURF_PRIV__SHIFT 0xf
#define HDP_NONSURFACE_INFO__NONSURF_TILE_COMPACT_MASK 0x10000
#define HDP_NONSURFACE_INFO__NONSURF_TILE_COMPACT__SHIFT 0x10
#define HDP_NONSURFACE_INFO__NONSURF_TILE_SPLIT_MASK 0xe0000
#define HDP_NONSURFACE_INFO__NONSURF_TILE_SPLIT__SHIFT 0x11
#define HDP_NONSURFACE_INFO__NONSURF_NUM_BANKS_MASK 0x300000
#define HDP_NONSURFACE_INFO__NONSURF_NUM_BANKS__SHIFT 0x14
#define HDP_NONSURFACE_INFO__NONSURF_BANK_WIDTH_MASK 0xc00000
#define HDP_NONSURFACE_INFO__NONSURF_BANK_WIDTH__SHIFT 0x16
#define HDP_NONSURFACE_INFO__NONSURF_BANK_HEIGHT_MASK 0x3000000
#define HDP_NONSURFACE_INFO__NONSURF_BANK_HEIGHT__SHIFT 0x18
#define HDP_NONSURFACE_INFO__NONSURF_MACRO_TILE_ASPECT_MASK 0xc000000
#define HDP_NONSURFACE_INFO__NONSURF_MACRO_TILE_ASPECT__SHIFT 0x1a
#define HDP_NONSURFACE_INFO__NONSURF_MICRO_TILE_MODE_MASK 0x70000000
#define HDP_NONSURFACE_INFO__NONSURF_MICRO_TILE_MODE__SHIFT 0x1c
#define HDP_NONSURFACE_INFO__NONSURF_SLICE_TILE_MAX_MSB_MASK 0x80000000
#define HDP_NONSURFACE_INFO__NONSURF_SLICE_TILE_MAX_MSB__SHIFT 0x1f
#define HDP_NONSURFACE_SIZE__NONSURF_PITCH_TILE_MAX_MASK 0x7ff
#define HDP_NONSURFACE_SIZE__NONSURF_PITCH_TILE_MAX__SHIFT 0x0
#define HDP_NONSURFACE_SIZE__NONSURF_SLICE_TILE_MAX_MASK 0xfffff800
#define HDP_NONSURFACE_SIZE__NONSURF_SLICE_TILE_MAX__SHIFT 0xb
#define HDP_NONSURF_FLAGS__NONSURF_WRITE_FLAG_MASK 0x1
#define HDP_NONSURF_FLAGS__NONSURF_WRITE_FLAG__SHIFT 0x0
#define HDP_NONSURF_FLAGS__NONSURF_READ_FLAG_MASK 0x2
#define HDP_NONSURF_FLAGS__NONSURF_READ_FLAG__SHIFT 0x1
#define HDP_NONSURF_FLAGS_CLR__NONSURF_WRITE_FLAG_CLR_MASK 0x1
#define HDP_NONSURF_FLAGS_CLR__NONSURF_WRITE_FLAG_CLR__SHIFT 0x0
#define HDP_NONSURF_FLAGS_CLR__NONSURF_READ_FLAG_CLR_MASK 0x2
#define HDP_NONSURF_FLAGS_CLR__NONSURF_READ_FLAG_CLR__SHIFT 0x1
#define HDP_SW_SEMAPHORE__SW_SEMAPHORE_MASK 0xffffffff
#define HDP_SW_SEMAPHORE__SW_SEMAPHORE__SHIFT 0x0
#define HDP_DEBUG0__HDP_DEBUG__SHIFT 0x0
#define HDP_DEBUG1__HDP_DEBUG__SHIFT 0x0
#define HDP_LAST_SURFACE_HIT__LAST_SURFACE_HIT_MASK 0x3f
#define HDP_LAST_SURFACE_HIT__LAST_SURFACE_HIT__SHIFT 0x0
#define HDP_TILING_CONFIG__PIPE_TILING_MASK 0xe
#define HDP_TILING_CONFIG__PIPE_TILING__SHIFT 0x1
#define HDP_TILING_CONFIG__BANK_TILING_MASK 0x30
#define HDP_TILING_CONFIG__BANK_TILING__SHIFT 0x4
#define HDP_TILING_CONFIG__GROUP_SIZE_MASK 0xc0
#define HDP_TILING_CONFIG__GROUP_SIZE__SHIFT 0x6
#define HDP_TILING_CONFIG__ROW_TILING_MASK 0x700
#define HDP_TILING_CONFIG__ROW_TILING__SHIFT 0x8
#define HDP_TILING_CONFIG__BANK_SWAPS_MASK 0x3800
#define HDP_TILING_CONFIG__BANK_SWAPS__SHIFT 0xb
#define HDP_TILING_CONFIG__SAMPLE_SPLIT_MASK 0xc000
#define HDP_TILING_CONFIG__SAMPLE_SPLIT__SHIFT 0xe
#define HDP_SC_MULTI_CHIP_CNTL__LOG2_NUM_CHIPS_MASK 0x7
#define HDP_SC_MULTI_CHIP_CNTL__LOG2_NUM_CHIPS__SHIFT 0x0
#define HDP_SC_MULTI_CHIP_CNTL__MULTI_CHIP_TILE_SIZE_MASK 0x18
#define HDP_SC_MULTI_CHIP_CNTL__MULTI_CHIP_TILE_SIZE__SHIFT 0x3
#define HDP_OUTSTANDING_REQ__WRITE_REQ_MASK 0xff
#define HDP_OUTSTANDING_REQ__WRITE_REQ__SHIFT 0x0
#define HDP_OUTSTANDING_REQ__READ_REQ_MASK 0xff00
#define HDP_OUTSTANDING_REQ__READ_REQ__SHIFT 0x8
#define HDP_ADDR_CONFIG__NUM_PIPES_MASK 0x7
#define HDP_ADDR_CONFIG__NUM_PIPES__SHIFT 0x0
#define HDP_ADDR_CONFIG__PIPE_INTERLEAVE_SIZE_MASK 0x70
#define HDP_ADDR_CONFIG__PIPE_INTERLEAVE_SIZE__SHIFT 0x4
#define HDP_ADDR_CONFIG__BANK_INTERLEAVE_SIZE_MASK 0x700
#define HDP_ADDR_CONFIG__BANK_INTERLEAVE_SIZE__SHIFT 0x8
#define HDP_ADDR_CONFIG__NUM_SHADER_ENGINES_MASK 0x3000
#define HDP_ADDR_CONFIG__NUM_SHADER_ENGINES__SHIFT 0xc
#define HDP_ADDR_CONFIG__SHADER_ENGINE_TILE_SIZE_MASK 0x70000
#define HDP_ADDR_CONFIG__SHADER_ENGINE_TILE_SIZE__SHIFT 0x10
#define HDP_ADDR_CONFIG__NUM_GPUS_MASK 0x700000
#define HDP_ADDR_CONFIG__NUM_GPUS__SHIFT 0x14
#define HDP_ADDR_CONFIG__MULTI_GPU_TILE_SIZE_MASK 0x3000000
#define HDP_ADDR_CONFIG__MULTI_GPU_TILE_SIZE__SHIFT 0x18
#define HDP_ADDR_CONFIG__ROW_SIZE_MASK 0x30000000
#define HDP_ADDR_CONFIG__ROW_SIZE__SHIFT 0x1c
#define HDP_ADDR_CONFIG__NUM_LOWER_PIPES_MASK 0x40000000
#define HDP_ADDR_CONFIG__NUM_LOWER_PIPES__SHIFT 0x1e
#define HDP_MISC_CNTL__FLUSH_INVALIDATE_CACHE_MASK 0x1
#define HDP_MISC_CNTL__FLUSH_INVALIDATE_CACHE__SHIFT 0x0
#define HDP_MISC_CNTL__VM_ID_MASK 0x1e
#define HDP_MISC_CNTL__VM_ID__SHIFT 0x1
#define HDP_MISC_CNTL__OUTSTANDING_WRITE_COUNT_1024_MASK 0x20
#define HDP_MISC_CNTL__OUTSTANDING_WRITE_COUNT_1024__SHIFT 0x5
#define HDP_MISC_CNTL__MULTIPLE_READS_MASK 0x40
#define HDP_MISC_CNTL__MULTIPLE_READS__SHIFT 0x6
#define HDP_MISC_CNTL__HDP_BIF_RDRET_CREDIT_MASK 0x780
#define HDP_MISC_CNTL__HDP_BIF_RDRET_CREDIT__SHIFT 0x7
#define HDP_MISC_CNTL__SIMULTANEOUS_READS_WRITES_MASK 0x800
#define HDP_MISC_CNTL__SIMULTANEOUS_READS_WRITES__SHIFT 0xb
#define HDP_MISC_CNTL__NO_SPLIT_ARRAY_LINEAR_MASK 0x1000
#define HDP_MISC_CNTL__NO_SPLIT_ARRAY_LINEAR__SHIFT 0xc
#define HDP_MISC_CNTL__MC_RDREQ_CREDIT_MASK 0x7e000
#define HDP_MISC_CNTL__MC_RDREQ_CREDIT__SHIFT 0xd
#define HDP_MISC_CNTL__READ_CACHE_INVALIDATE_MASK 0x80000
#define HDP_MISC_CNTL__READ_CACHE_INVALIDATE__SHIFT 0x13
#define HDP_MISC_CNTL__ADDRLIB_LINEAR_BYPASS_MASK 0x100000
#define HDP_MISC_CNTL__ADDRLIB_LINEAR_BYPASS__SHIFT 0x14
#define HDP_MISC_CNTL__FED_ENABLE_MASK 0x200000
#define HDP_MISC_CNTL__FED_ENABLE__SHIFT 0x15
#define HDP_MISC_CNTL__LEGACY_TILING_ENABLE_MASK 0x400000
#define HDP_MISC_CNTL__LEGACY_TILING_ENABLE__SHIFT 0x16
#define HDP_MISC_CNTL__LEGACY_SURFACES_ENABLE_MASK 0x800000
#define HDP_MISC_CNTL__LEGACY_SURFACES_ENABLE__SHIFT 0x17
#define HDP_MEM_POWER_LS__LS_ENABLE_MASK 0x1
#define HDP_MEM_POWER_LS__LS_ENABLE__SHIFT 0x0
#define HDP_MEM_POWER_LS__LS_SETUP_MASK 0x7e
#define HDP_MEM_POWER_LS__LS_SETUP__SHIFT 0x1
#define HDP_MEM_POWER_LS__LS_HOLD_MASK 0x1f80
#define HDP_MEM_POWER_LS__LS_HOLD__SHIFT 0x7
#define HDP_NONSURFACE_PREFETCH__NONSURF_PREFETCH_PRI_MASK 0x7
#define HDP_NONSURFACE_PREFETCH__NONSURF_PREFETCH_PRI__SHIFT 0x0
#define HDP_NONSURFACE_PREFETCH__NONSURF_PREFETCH_DIR_MASK 0x38
#define HDP_NONSURFACE_PREFETCH__NONSURF_PREFETCH_DIR__SHIFT 0x3
#define HDP_NONSURFACE_PREFETCH__NONSURF_PREFETCH_NUM_MASK 0x1c0
#define HDP_NONSURFACE_PREFETCH__NONSURF_PREFETCH_NUM__SHIFT 0x6
#define HDP_NONSURFACE_PREFETCH__NONSURF_PREFETCH_MAX_Z_MASK 0xffe00
#define HDP_NONSURFACE_PREFETCH__NONSURF_PREFETCH_MAX_Z__SHIFT 0x9
#define HDP_NONSURFACE_PREFETCH__NONSURF_PIPE_CONFIG_MASK 0xf8000000
#define HDP_NONSURFACE_PREFETCH__NONSURF_PIPE_CONFIG__SHIFT 0x1b
#define HDP_MEMIO_CNTL__MEMIO_SEND_MASK 0x1
#define HDP_MEMIO_CNTL__MEMIO_SEND__SHIFT 0x0
#define HDP_MEMIO_CNTL__MEMIO_OP_MASK 0x2
#define HDP_MEMIO_CNTL__MEMIO_OP__SHIFT 0x1
#define HDP_MEMIO_CNTL__MEMIO_BE_MASK 0x3c
#define HDP_MEMIO_CNTL__MEMIO_BE__SHIFT 0x2
#define HDP_MEMIO_CNTL__MEMIO_WR_STROBE_MASK 0x40
#define HDP_MEMIO_CNTL__MEMIO_WR_STROBE__SHIFT 0x6
#define HDP_MEMIO_CNTL__MEMIO_RD_STROBE_MASK 0x80
#define HDP_MEMIO_CNTL__MEMIO_RD_STROBE__SHIFT 0x7
#define HDP_MEMIO_CNTL__MEMIO_ADDR_UPPER_MASK 0x3f00
#define HDP_MEMIO_CNTL__MEMIO_ADDR_UPPER__SHIFT 0x8
#define HDP_MEMIO_CNTL__MEMIO_CLR_WR_ERROR_MASK 0x4000
#define HDP_MEMIO_CNTL__MEMIO_CLR_WR_ERROR__SHIFT 0xe
#define HDP_MEMIO_CNTL__MEMIO_CLR_RD_ERROR_MASK 0x8000
#define HDP_MEMIO_CNTL__MEMIO_CLR_RD_ERROR__SHIFT 0xf
#define HDP_MEMIO_CNTL__MEMIO_VF_MASK 0x10000
#define HDP_MEMIO_CNTL__MEMIO_VF__SHIFT 0x10
#define HDP_MEMIO_CNTL__MEMIO_VFID_MASK 0x1e0000
#define HDP_MEMIO_CNTL__MEMIO_VFID__SHIFT 0x11
#define HDP_MEMIO_ADDR__MEMIO_ADDR_LOWER_MASK 0xffffffff
#define HDP_MEMIO_ADDR__MEMIO_ADDR_LOWER__SHIFT 0x0
#define HDP_MEMIO_STATUS__MEMIO_WR_STATUS_MASK 0x1
#define HDP_MEMIO_STATUS__MEMIO_WR_STATUS__SHIFT 0x0
#define HDP_MEMIO_STATUS__MEMIO_RD_STATUS_MASK 0x2
#define HDP_MEMIO_STATUS__MEMIO_RD_STATUS__SHIFT 0x1
#define HDP_MEMIO_STATUS__MEMIO_WR_ERROR_MASK 0x4
#define HDP_MEMIO_STATUS__MEMIO_WR_ERROR__SHIFT 0x2
#define HDP_MEMIO_STATUS__MEMIO_RD_ERROR_MASK 0x8
#define HDP_MEMIO_STATUS__MEMIO_RD_ERROR__SHIFT 0x3
#define HDP_MEMIO_WR_DATA__MEMIO_WR_DATA_MASK 0xffffffff
#define HDP_MEMIO_WR_DATA__MEMIO_WR_DATA__SHIFT 0x0
#define HDP_MEMIO_RD_DATA__MEMIO_RD_DATA_MASK 0xffffffff
#define HDP_MEMIO_RD_DATA__MEMIO_RD_DATA__SHIFT 0x0
#define HDP_VF_ENABLE__VF_EN_MASK 0x1
#define HDP_VF_ENABLE__VF_EN__SHIFT 0x0
#define HDP_VF_ENABLE__VF_NUM_MASK 0xffff0000
#define HDP_VF_ENABLE__VF_NUM__SHIFT 0x10
#define HDP_XDP_DIRECT2HDP_FIRST__RESERVED_MASK 0xffffffff
#define HDP_XDP_DIRECT2HDP_FIRST__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_FLUSH_NUM_MASK 0xf
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_FLUSH_NUM__SHIFT 0x0
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_MBX_ENC_DATA_MASK 0xf0
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_MBX_ENC_DATA__SHIFT 0x4
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_MBX_ADDR_SEL_MASK 0x700
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_MBX_ADDR_SEL__SHIFT 0x8
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_XPB_CLG_MASK 0xf800
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_XPB_CLG__SHIFT 0xb
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_SEND_HOST_MASK 0x10000
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_SEND_HOST__SHIFT 0x10
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_SEND_SIDE_MASK 0x20000
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_SEND_SIDE__SHIFT 0x11
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_ALTER_FLUSH_NUM_MASK 0x40000
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_ALTER_FLUSH_NUM__SHIFT 0x12
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_RSVD_0_MASK 0x80000
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_RSVD_0__SHIFT 0x13
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_RSVD_1_MASK 0x100000
#define HDP_XDP_D2H_FLUSH__D2H_FLUSH_RSVD_1__SHIFT 0x14
#define HDP_XDP_D2H_BAR_UPDATE__D2H_BAR_UPDATE_ADDR_MASK 0xffff
#define HDP_XDP_D2H_BAR_UPDATE__D2H_BAR_UPDATE_ADDR__SHIFT 0x0
#define HDP_XDP_D2H_BAR_UPDATE__D2H_BAR_UPDATE_FLUSH_NUM_MASK 0xf0000
#define HDP_XDP_D2H_BAR_UPDATE__D2H_BAR_UPDATE_FLUSH_NUM__SHIFT 0x10
#define HDP_XDP_D2H_BAR_UPDATE__D2H_BAR_UPDATE_BAR_NUM_MASK 0x700000
#define HDP_XDP_D2H_BAR_UPDATE__D2H_BAR_UPDATE_BAR_NUM__SHIFT 0x14
#define HDP_XDP_D2H_RSVD_3__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_3__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_4__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_4__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_5__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_5__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_6__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_6__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_7__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_7__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_8__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_8__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_9__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_9__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_10__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_10__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_11__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_11__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_12__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_12__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_13__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_13__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_14__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_14__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_15__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_15__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_16__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_16__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_17__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_17__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_18__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_18__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_19__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_19__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_20__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_20__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_21__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_21__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_22__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_22__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_23__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_23__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_24__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_24__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_25__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_25__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_26__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_26__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_27__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_27__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_28__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_28__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_29__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_29__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_30__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_30__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_31__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_31__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_32__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_32__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_33__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_33__RESERVED__SHIFT 0x0
#define HDP_XDP_D2H_RSVD_34__RESERVED_MASK 0xffffffff
#define HDP_XDP_D2H_RSVD_34__RESERVED__SHIFT 0x0
#define HDP_XDP_DIRECT2HDP_LAST__RESERVED_MASK 0xffffffff
#define HDP_XDP_DIRECT2HDP_LAST__RESERVED__SHIFT 0x0
#define HDP_XDP_P2P_BAR_CFG__P2P_BAR_CFG_ADDR_SIZE_MASK 0xf
#define HDP_XDP_P2P_BAR_CFG__P2P_BAR_CFG_ADDR_SIZE__SHIFT 0x0
#define HDP_XDP_P2P_BAR_CFG__P2P_BAR_CFG_BAR_FROM_MASK 0x30
#define HDP_XDP_P2P_BAR_CFG__P2P_BAR_CFG_BAR_FROM__SHIFT 0x4
#define HDP_XDP_P2P_MBX_OFFSET__P2P_MBX_OFFSET_MASK 0x3fff
#define HDP_XDP_P2P_MBX_OFFSET__P2P_MBX_OFFSET__SHIFT 0x0
#define HDP_XDP_P2P_MBX_ADDR0__VALID_MASK 0x1
#define HDP_XDP_P2P_MBX_ADDR0__VALID__SHIFT 0x0
#define HDP_XDP_P2P_MBX_ADDR0__ADDR_MASK 0x1ffffe
#define HDP_XDP_P2P_MBX_ADDR0__ADDR__SHIFT 0x1
#define HDP_XDP_P2P_MBX_ADDR0__ADDR_39_36_MASK 0x1e00000
#define HDP_XDP_P2P_MBX_ADDR0__ADDR_39_36__SHIFT 0x15
#define HDP_XDP_P2P_MBX_ADDR1__VALID_MASK 0x1
#define HDP_XDP_P2P_MBX_ADDR1__VALID__SHIFT 0x0
#define HDP_XDP_P2P_MBX_ADDR1__ADDR_MASK 0x1ffffe
#define HDP_XDP_P2P_MBX_ADDR1__ADDR__SHIFT 0x1
#define HDP_XDP_P2P_MBX_ADDR1__ADDR_39_36_MASK 0x1e00000
#define HDP_XDP_P2P_MBX_ADDR1__ADDR_39_36__SHIFT 0x15
#define HDP_XDP_P2P_MBX_ADDR2__VALID_MASK 0x1
#define HDP_XDP_P2P_MBX_ADDR2__VALID__SHIFT 0x0
#define HDP_XDP_P2P_MBX_ADDR2__ADDR_MASK 0x1ffffe
#define HDP_XDP_P2P_MBX_ADDR2__ADDR__SHIFT 0x1
#define HDP_XDP_P2P_MBX_ADDR2__ADDR_39_36_MASK 0x1e00000
#define HDP_XDP_P2P_MBX_ADDR2__ADDR_39_36__SHIFT 0x15
#define HDP_XDP_P2P_MBX_ADDR3__VALID_MASK 0x1
#define HDP_XDP_P2P_MBX_ADDR3__VALID__SHIFT 0x0
#define HDP_XDP_P2P_MBX_ADDR3__ADDR_MASK 0x1ffffe
#define HDP_XDP_P2P_MBX_ADDR3__ADDR__SHIFT 0x1
#define HDP_XDP_P2P_MBX_ADDR3__ADDR_39_36_MASK 0x1e00000
#define HDP_XDP_P2P_MBX_ADDR3__ADDR_39_36__SHIFT 0x15
#define HDP_XDP_P2P_MBX_ADDR4__VALID_MASK 0x1
#define HDP_XDP_P2P_MBX_ADDR4__VALID__SHIFT 0x0
#define HDP_XDP_P2P_MBX_ADDR4__ADDR_MASK 0x1ffffe
#define HDP_XDP_P2P_MBX_ADDR4__ADDR__SHIFT 0x1
#define HDP_XDP_P2P_MBX_ADDR4__ADDR_39_36_MASK 0x1e00000
#define HDP_XDP_P2P_MBX_ADDR4__ADDR_39_36__SHIFT 0x15
#define HDP_XDP_P2P_MBX_ADDR5__VALID_MASK 0x1
#define HDP_XDP_P2P_MBX_ADDR5__VALID__SHIFT 0x0
#define HDP_XDP_P2P_MBX_ADDR5__ADDR_MASK 0x1ffffe
#define HDP_XDP_P2P_MBX_ADDR5__ADDR__SHIFT 0x1
#define HDP_XDP_P2P_MBX_ADDR5__ADDR_39_36_MASK 0x1e00000
#define HDP_XDP_P2P_MBX_ADDR5__ADDR_39_36__SHIFT 0x15
#define HDP_XDP_P2P_MBX_ADDR6__VALID_MASK 0x1
#define HDP_XDP_P2P_MBX_ADDR6__VALID__SHIFT 0x0
#define HDP_XDP_P2P_MBX_ADDR6__ADDR_MASK 0x1ffffe
#define HDP_XDP_P2P_MBX_ADDR6__ADDR__SHIFT 0x1
#define HDP_XDP_P2P_MBX_ADDR6__ADDR_39_36_MASK 0x1e00000
#define HDP_XDP_P2P_MBX_ADDR6__ADDR_39_36__SHIFT 0x15
#define HDP_XDP_HDP_MBX_MC_CFG__HDP_MBX_MC_CFG_TAP_WRREQ_PRIV_MASK 0x1
#define HDP_XDP_HDP_MBX_MC_CFG__HDP_MBX_MC_CFG_TAP_WRREQ_PRIV__SHIFT 0x0
#define HDP_XDP_HDP_MBX_MC_CFG__HDP_MBX_MC_CFG_TAP_WRREQ_SWAP_MASK 0x6
#define HDP_XDP_HDP_MBX_MC_CFG__HDP_MBX_MC_CFG_TAP_WRREQ_SWAP__SHIFT 0x1
#define HDP_XDP_HDP_MBX_MC_CFG__HDP_MBX_MC_CFG_TAP_WRREQ_TRAN_MASK 0x8
#define HDP_XDP_HDP_MBX_MC_CFG__HDP_MBX_MC_CFG_TAP_WRREQ_TRAN__SHIFT 0x3
#define HDP_XDP_HDP_MBX_MC_CFG__HDP_MBX_MC_CFG_TAP_WRREQ_VMID_MASK 0xf0
#define HDP_XDP_HDP_MBX_MC_CFG__HDP_MBX_MC_CFG_TAP_WRREQ_VMID__SHIFT 0x4
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_HST_TAP_WRREQ_PRIV_MASK 0x1
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_HST_TAP_WRREQ_PRIV__SHIFT 0x0
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_HST_TAP_WRREQ_SWAP_MASK 0x6
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_HST_TAP_WRREQ_SWAP__SHIFT 0x1
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_HST_TAP_WRREQ_TRAN_MASK 0x8
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_HST_TAP_WRREQ_TRAN__SHIFT 0x3
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_SID_TAP_WRREQ_PRIV_MASK 0x10
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_SID_TAP_WRREQ_PRIV__SHIFT 0x4
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_SID_TAP_WRREQ_SWAP_MASK 0x60
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_SID_TAP_WRREQ_SWAP__SHIFT 0x5
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_SID_TAP_WRREQ_TRAN_MASK 0x80
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_SID_TAP_WRREQ_TRAN__SHIFT 0x7
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_XL8R_WRREQ_CRD_OVERRIDE_MASK 0x3f00
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_XL8R_WRREQ_CRD_OVERRIDE__SHIFT 0x8
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_XDP_HIGHER_PRI_THRESH_MASK 0xfc000
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_XDP_HIGHER_PRI_THRESH__SHIFT 0xe
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_MC_STALL_ON_BUF_FULL_MASK_MASK 0x700000
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_MC_STALL_ON_BUF_FULL_MASK__SHIFT 0x14
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_HST_TAP_WRREQ_VMID_MASK 0x7800000
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_HST_TAP_WRREQ_VMID__SHIFT 0x17
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_SID_TAP_WRREQ_VMID_MASK 0x78000000
#define HDP_XDP_HDP_MC_CFG__HDP_MC_CFG_SID_TAP_WRREQ_VMID__SHIFT 0x1b
#define HDP_XDP_HST_CFG__HST_CFG_WR_COMBINE_EN_MASK 0x1
#define HDP_XDP_HST_CFG__HST_CFG_WR_COMBINE_EN__SHIFT 0x0
#define HDP_XDP_HST_CFG__HST_CFG_WR_COMBINE_TIMER_MASK 0x6
#define HDP_XDP_HST_CFG__HST_CFG_WR_COMBINE_TIMER__SHIFT 0x1
#define HDP_XDP_SID_CFG__SID_CFG_WR_COMBINE_EN_MASK 0x1
#define HDP_XDP_SID_CFG__SID_CFG_WR_COMBINE_EN__SHIFT 0x0
#define HDP_XDP_SID_CFG__SID_CFG_WR_COMBINE_TIMER_MASK 0x6
#define HDP_XDP_SID_CFG__SID_CFG_WR_COMBINE_TIMER__SHIFT 0x1
#define HDP_XDP_SID_CFG__SID_CFG_FLNUM_MSB_SEL_MASK 0x18
#define HDP_XDP_SID_CFG__SID_CFG_FLNUM_MSB_SEL__SHIFT 0x3
#define HDP_XDP_HDP_IPH_CFG__HDP_IPH_CFG_SYS_FIFO_DEPTH_OVERRIDE_MASK 0x3f
#define HDP_XDP_HDP_IPH_CFG__HDP_IPH_CFG_SYS_FIFO_DEPTH_OVERRIDE__SHIFT 0x0
#define HDP_XDP_HDP_IPH_CFG__HDP_IPH_CFG_XDP_FIFO_DEPTH_OVERRIDE_MASK 0xfc0
#define HDP_XDP_HDP_IPH_CFG__HDP_IPH_CFG_XDP_FIFO_DEPTH_OVERRIDE__SHIFT 0x6
#define HDP_XDP_HDP_IPH_CFG__HDP_IPH_CFG_INVERSE_PEER_TAG_MATCHING_MASK 0x1000
#define HDP_XDP_HDP_IPH_CFG__HDP_IPH_CFG_INVERSE_PEER_TAG_MATCHING__SHIFT 0xc
#define HDP_XDP_HDP_IPH_CFG__HDP_IPH_CFG_P2P_RD_EN_MASK 0x2000
#define HDP_XDP_HDP_IPH_CFG__HDP_IPH_CFG_P2P_RD_EN__SHIFT 0xd
#define HDP_XDP_SRBM_CFG__SRBM_CFG_REG_CLK_ENABLE_COUNT_MASK 0x3f
#define HDP_XDP_SRBM_CFG__SRBM_CFG_REG_CLK_ENABLE_COUNT__SHIFT 0x0
#define HDP_XDP_SRBM_CFG__SRBM_CFG_REG_CLK_GATING_DIS_MASK 0x40
#define HDP_XDP_SRBM_CFG__SRBM_CFG_REG_CLK_GATING_DIS__SHIFT 0x6
#define HDP_XDP_SRBM_CFG__SRBM_CFG_WAKE_DYN_CLK_MASK 0x80
#define HDP_XDP_SRBM_CFG__SRBM_CFG_WAKE_DYN_CLK__SHIFT 0x7
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_0_ON_DELAY_MASK 0xf
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_0_ON_DELAY__SHIFT 0x0
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_1_OFF_DELAY_MASK 0xff0
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_1_OFF_DELAY__SHIFT 0x4
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_2_RSVD_MASK 0x3ffff000
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_2_RSVD__SHIFT 0xc
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_3_SOFT_CORE_OVERRIDE_MASK 0x40000000
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_3_SOFT_CORE_OVERRIDE__SHIFT 0x1e
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_4_SOFT_REG_OVERRIDE_MASK 0x80000000
#define HDP_XDP_CGTT_BLK_CTRL__CGTT_BLK_CTRL_4_SOFT_REG_OVERRIDE__SHIFT 0x1f
#define HDP_XDP_P2P_BAR0__ADDR_MASK 0xffff
#define HDP_XDP_P2P_BAR0__ADDR__SHIFT 0x0
#define HDP_XDP_P2P_BAR0__FLUSH_MASK 0xf0000
#define HDP_XDP_P2P_BAR0__FLUSH__SHIFT 0x10
#define HDP_XDP_P2P_BAR0__VALID_MASK 0x100000
#define HDP_XDP_P2P_BAR0__VALID__SHIFT 0x14
#define HDP_XDP_P2P_BAR1__ADDR_MASK 0xffff
#define HDP_XDP_P2P_BAR1__ADDR__SHIFT 0x0
#define HDP_XDP_P2P_BAR1__FLUSH_MASK 0xf0000
#define HDP_XDP_P2P_BAR1__FLUSH__SHIFT 0x10
#define HDP_XDP_P2P_BAR1__VALID_MASK 0x100000
#define HDP_XDP_P2P_BAR1__VALID__SHIFT 0x14
#define HDP_XDP_P2P_BAR2__ADDR_MASK 0xffff
#define HDP_XDP_P2P_BAR2__ADDR__SHIFT 0x0
#define HDP_XDP_P2P_BAR2__FLUSH_MASK 0xf0000
#define HDP_XDP_P2P_BAR2__FLUSH__SHIFT 0x10
#define HDP_XDP_P2P_BAR2__VALID_MASK 0x100000
#define HDP_XDP_P2P_BAR2__VALID__SHIFT 0x14
#define HDP_XDP_P2P_BAR3__ADDR_MASK 0xffff
#define HDP_XDP_P2P_BAR3__ADDR__SHIFT 0x0
#define HDP_XDP_P2P_BAR3__FLUSH_MASK 0xf0000
#define HDP_XDP_P2P_BAR3__FLUSH__SHIFT 0x10
#define HDP_XDP_P2P_BAR3__VALID_MASK 0x100000
#define HDP_XDP_P2P_BAR3__VALID__SHIFT 0x14
#define HDP_XDP_P2P_BAR4__ADDR_MASK 0xffff
#define HDP_XDP_P2P_BAR4__ADDR__SHIFT 0x0
#define HDP_XDP_P2P_BAR4__FLUSH_MASK 0xf0000
#define HDP_XDP_P2P_BAR4__FLUSH__SHIFT 0x10
#define HDP_XDP_P2P_BAR4__VALID_MASK 0x100000
#define HDP_XDP_P2P_BAR4__VALID__SHIFT 0x14
#define HDP_XDP_P2P_BAR5__ADDR_MASK 0xffff
#define HDP_XDP_P2P_BAR5__ADDR__SHIFT 0x0
#define HDP_XDP_P2P_BAR5__FLUSH_MASK 0xf0000
#define HDP_XDP_P2P_BAR5__FLUSH__SHIFT 0x10
#define HDP_XDP_P2P_BAR5__VALID_MASK 0x100000
#define HDP_XDP_P2P_BAR5__VALID__SHIFT 0x14
#define HDP_XDP_P2P_BAR6__ADDR_MASK 0xffff
#define HDP_XDP_P2P_BAR6__ADDR__SHIFT 0x0
#define HDP_XDP_P2P_BAR6__FLUSH_MASK 0xf0000
#define HDP_XDP_P2P_BAR6__FLUSH__SHIFT 0x10
#define HDP_XDP_P2P_BAR6__VALID_MASK 0x100000
#define HDP_XDP_P2P_BAR6__VALID__SHIFT 0x14
#define HDP_XDP_P2P_BAR7__ADDR_MASK 0xffff
#define HDP_XDP_P2P_BAR7__ADDR__SHIFT 0x0
#define HDP_XDP_P2P_BAR7__FLUSH_MASK 0xf0000
#define HDP_XDP_P2P_BAR7__FLUSH__SHIFT 0x10
#define HDP_XDP_P2P_BAR7__VALID_MASK 0x100000
#define HDP_XDP_P2P_BAR7__VALID__SHIFT 0x14
#define HDP_XDP_FLUSH_ARMED_STS__FLUSH_ARMED_STS_MASK 0xffffffff
#define HDP_XDP_FLUSH_ARMED_STS__FLUSH_ARMED_STS__SHIFT 0x0
#define HDP_XDP_FLUSH_CNTR0_STS__FLUSH_CNTR0_STS_MASK 0x3ffffff
#define HDP_XDP_FLUSH_CNTR0_STS__FLUSH_CNTR0_STS__SHIFT 0x0
#define HDP_XDP_BUSY_STS__BUSY_BITS_MASK 0x3ffff
#define HDP_XDP_BUSY_STS__BUSY_BITS__SHIFT 0x0
#define HDP_XDP_STICKY__STICKY_STS_MASK 0xffff
#define HDP_XDP_STICKY__STICKY_STS__SHIFT 0x0
#define HDP_XDP_STICKY__STICKY_W1C_MASK 0xffff0000
#define HDP_XDP_STICKY__STICKY_W1C__SHIFT 0x10
#define HDP_XDP_CHKN__CHKN_0_RSVD_MASK 0xff
#define HDP_XDP_CHKN__CHKN_0_RSVD__SHIFT 0x0
#define HDP_XDP_CHKN__CHKN_1_RSVD_MASK 0xff00
#define HDP_XDP_CHKN__CHKN_1_RSVD__SHIFT 0x8
#define HDP_XDP_CHKN__CHKN_2_RSVD_MASK 0xff0000
#define HDP_XDP_CHKN__CHKN_2_RSVD__SHIFT 0x10
#define HDP_XDP_CHKN__CHKN_3_R