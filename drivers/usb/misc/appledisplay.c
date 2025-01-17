_DATA_MASK 0xffffffff
#define XDMA_AON_TEST_DEBUG_DATA__XDMA_AON_TEST_DEBUG_DATA__SHIFT 0x0
#define XDMA_MSTR_CNTL__XDMA_MSTR_ALPHA_POSITION_MASK 0x3000
#define XDMA_MSTR_CNTL__XDMA_MSTR_ALPHA_POSITION__SHIFT 0xc
#define XDMA_MSTR_CNTL__XDMA_MSTR_MEM_READY_MASK 0x4000
#define XDMA_MSTR_CNTL__XDMA_MSTR_MEM_READY__SHIFT 0xe
#define XDMA_MSTR_CNTL__XDMA_MSTR_ENABLE_MASK 0x10000
#define XDMA_MSTR_CNTL__XDMA_MSTR_ENABLE__SHIFT 0x10
#define XDMA_MSTR_CNTL__XDMA_MSTR_DEBUG_MODE_MASK 0x40000
#define XDMA_MSTR_CNTL__XDMA_MSTR_DEBUG_MODE__SHIFT 0x12
#define XDMA_MSTR_CNTL__XDMA_MSTR_SOFT_RESET_MASK 0x100000
#define XDMA_MSTR_CNTL__XDMA_MSTR_SOFT_RESET__SHIFT 0x14
#define XDMA_MSTR_CNTL__XDMA_MSTR_BIF_STALL_EN_MASK 0x200000
#define XDMA_MSTR_CNTL__XDMA_MSTR_BIF_STALL_EN__SHIFT 0x15
#define XDMA_MSTR_STATUS__XDMA_MSTR_VCOUNT_CURRENT_MASK 0x3fff
#define XDMA_MSTR_STATUS__XDMA_MSTR_VCOUNT_CURRENT__SHIFT 0x0
#define XDMA_MSTR_STATUS__XDMA_MSTR_WRITE_LINE_CURRENT_MASK 0xfff0000
#define XDMA_MSTR_STATUS__XDMA_MSTR_WRITE_LINE_CURRENT__SHIFT 0x10
#define XDMA_MSTR_STATUS__XDMA_MSTR_STATUS_SELECT_MASK 0x70000000
#define XDMA_MSTR_STATUS__XDMA_MSTR_STATUS_SELECT__SHIFT 0x1c
#define XDMA_MSTR_MEM_CLIENT_CONFIG__XDMA_MSTR_MEM_CLIENT_SWAP_MASK 0x300
#define XDMA_MSTR_MEM_CLIENT_CONFIG__XDMA_MSTR_MEM_CLIENT_SWAP__SHIFT 0x8
#define XDMA_MSTR_MEM_CLIENT_CONFIG__XDMA_MSTR_MEM_CLIENT_VMID_MASK 0xf000
#define XDMA_MSTR_MEM_CLIENT_CONFIG__XDMA_MSTR_MEM_CLIENT_VMID__SHIFT 0xc
#define XDMA_MSTR_MEM_CLIENT_CONFIG__XDMA_MSTR_MEM_CLIENT_PRIV_MASK 0x10000
#define XDMA_MSTR_MEM_CLIENT_CONFIG__XDMA_MSTR_MEM_CLIENT_PRIV__SHIFT 0x10
#define XDMA_MSTR_LOCAL_SURFACE_BASE_ADDR__XDMA_MSTR_LOCAL_SURFACE_BASE_ADDR_MASK 0xffffffff
#define XDMA_MSTR_LOCAL_SURFACE_BASE_ADDR__XDMA_MSTR_LOCAL_SURFACE_BASE_ADDR__SHIFT 0x0
#define XDMA_MSTR_LOCAL_SURFACE_BASE_ADDR_HIGH__XDMA_MSTR_LOCAL_SURFACE_BASE_ADDR_HIGH_MASK 0xff
#define XDMA_MSTR_LOCAL_SURFACE_BASE_ADDR_HIGH__XDMA_MSTR_LOCAL_SURFACE_BASE_ADDR_HIGH__SHIFT 0x0
#define XDMA_MSTR_LOCAL_SURFACE_PITCH__XDMA_MSTR_LOCAL_SURFACE_PITCH_MASK 0x3fff
#define XDMA_MSTR_LOCAL_SURFACE_PITCH__XDMA_MSTR_LOCAL_SURFACE_PITCH__SHIFT 0x0
#define XDMA_MSTR_CMD_URGENT_CNTL__XDMA_MSTR_CMD_CLIENT_STALL_MASK 0x1
#define XDMA_MSTR_CMD_URGENT_CNTL__XDMA_MSTR_CMD_CLIENT_STALL__SHIFT 0x0
#define XDMA_MSTR_CMD_URGENT_CNTL__XDMA_MSTR_CMD_URGENT_LEVEL_MASK 0xf00
#define XDMA_MSTR_CMD_URGENT_CNTL__XDMA_MSTR_CMD_URGENT_LEVEL__SHIFT 0x8
#define XDMA_MSTR_CMD_URGENT_CNTL__XDMA_MSTR_CMD_STALL_DELAY_MASK 0xf000
#define XDMA_MSTR_CMD_URGENT_CNTL__XDMA_MSTR_CMD_STALL_DELAY__SHIFT 0xc
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_CLIENT_STALL_MASK 0x1
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_CLIENT_STALL__SHIFT 0x0
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_URGENT_LIMIT_MASK 0xf0
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_URGENT_LIMIT__SHIFT 0x4
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_URGENT_LEVEL_MASK 0xf00
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_URGENT_LEVEL__SHIFT 0x8
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_STALL_DELAY_MASK 0xf000
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_STALL_DELAY__SHIFT 0xc
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_URGENT_TIMER_MASK 0xffff0000
#define XDMA_MSTR_MEM_URGENT_CNTL__XDMA_MSTR_MEM_URGENT_TIMER__SHIFT 0x10
#define XDMA_MSTR_PCIE_NACK_STATUS__XDMA_MSTR_PCIE_NACK_TAG_MASK 0x3ff
#define XDMA_MSTR_PCIE_NACK_STATUS__XDMA_MSTR_PCIE_NACK_TAG__SHIFT 0x0
#define XDMA_MSTR_PCIE_NACK_STATUS__XDMA_MSTR_PCIE_NACK_MASK 0x3000
#define XDMA_MSTR_PCIE_NACK_STATUS__XDMA_MSTR_PCIE_NACK__SHIFT 0xc
#define XDMA_MSTR_PCIE_NACK_STATUS__XDMA_MSTR_PCIE_NACK_CLR_MASK 0x10000
#define XDMA_MSTR_PCIE_NACK_STATUS__XDMA_MSTR_PCIE_NACK_CLR__SHIFT 0x10
#define XDMA_MSTR_MEM_NACK_STATUS__XDMA_MSTR_MEM_NACK_TAG_MASK 0x3ff
#define XDMA_MSTR_MEM_NACK_STATUS__XDMA_MSTR_MEM_NACK_TAG__SHIFT 0x0
#define XDMA_MSTR_MEM_NACK_STATUS__XDMA_MSTR_MEM_NACK_MASK 0x3000
#define XDMA_MSTR_MEM_NACK_STATUS__XDMA_MSTR_MEM_NACK__SHIFT 0xc
#define XDMA_MSTR_MEM_NACK_STATUS__XDMA_MSTR_MEM_NACK_CLR_MASK 0x10000
#define XDMA_MSTR_MEM_NACK_STATUS__XDMA_MSTR_MEM_NACK_CLR__SHIFT 0x10
#define XDMA_MSTR_VSYNC_GSL_CHECK__XDMA_MSTR_VSYNC_GSL_CHECK_SEL_MASK 0x7
#define XDMA_MSTR_VSYNC_GSL_CHECK__XDMA_MSTR_VSYNC_GSL_CHECK_SEL__SHIFT 0x0
#define XDMA_MSTR_VSYNC_GSL_CHECK__XDMA_MSTR_VSYNC_GSL_CHECK_V_COUNT_MASK 0x3fff00
#define XDMA_MSTR_VSYNC_GSL_CHECK__XDMA_MSTR_VSYNC_GSL_CHECK_V_COUNT__SHIFT 0x8
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_CACHE_LINES_MASK 0xff
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_CACHE_LINES__SHIFT 0x0
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_READ_REQUEST_MASK 0x100
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_READ_REQUEST__SHIFT 0x8
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_FRAME_MODE_MASK 0x200
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_FRAME_MODE__SHIFT 0x9
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_SOFT_RESET_MASK 0x400
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_SOFT_RESET__SHIFT 0xa
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_CACHE_INVALIDATE_MASK 0x800
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_CACHE_INVALIDATE__SHIFT 0xb
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_REQUEST_CHANNEL_ID_MASK 0x7000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_REQUEST_CHANNEL_ID__SHIFT 0xc
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_FLIP_MODE_MASK 0x8000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_FLIP_MODE__SHIFT 0xf
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_REQUEST_MIN_MASK 0xff0000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_REQUEST_MIN__SHIFT 0x10
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_ACTIVE_MASK 0x1000000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_ACTIVE__SHIFT 0x18
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_FLUSHING_MASK 0x2000000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_FLUSHING__SHIFT 0x19
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_FLIP_PENDING_MASK 0x4000000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_PIPE_FLIP_PENDING__SHIFT 0x1a
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_VSYNC_GSL_ENABLE_MASK 0x8000000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_VSYNC_GSL_ENABLE__SHIFT 0x1b
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_SUPERAA_ENABLE_MASK 0x10000000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_SUPERAA_ENABLE__SHIFT 0x1c
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_HSYNC_GSL_GROUP_MASK 0x60000000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_HSYNC_GSL_GROUP__SHIFT 0x1d
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_GSL_GROUP_MASTER_MASK 0x80000000
#define XDMA_MSTR_PIPE_CNTL__XDMA_MSTR_GSL_GROUP_MASTER__SHIFT 0x1f
#define XDMA_MSTR_READ_COMMAND__XDMA_MSTR_REQUEST_SIZE_MASK 0x3fff
#define XDMA_MSTR_READ_COMMAND__XDMA_MSTR_REQUEST_SIZE__SHIFT 0x0
#define XDMA_MSTR_READ_COMMAND__XDMA_MSTR_REQUEST_PREFETCH_MASK 0x3fff0000
#define XDMA_MSTR_READ_COMMAND__XDMA_MSTR_REQUEST_PREFETCH__SHIFT 0x10
#define XDMA_MSTR_CHANNEL_DIM__XDMA_MSTR_CHANNEL_WIDTH_MASK 0x3fff
#define XDMA_MSTR_CHANNEL_DIM__XDMA_MSTR_CHANNEL_WIDTH__SHIFT 0x0
#define XDMA_MSTR_CHANNEL_DIM__XDMA_MSTR_CHANNEL_HEIGHT_MASK 0x3fff0000
#define XDMA_MSTR_CHANNEL_DIM__XDMA_MSTR_CHANNEL_HEIGHT__SHIFT 0x10
#define XDMA_MSTR_HEIGHT__XDMA_MSTR_ACTIVE_HEIGHT_MASK 0x3fff
#define XDMA_MSTR_HEIGHT__XDMA_MSTR_ACTIVE_HEIGHT__SHIFT 0x0
#define XDMA_MSTR_HEIGHT__XDMA_MSTR_FRAME_HEIGHT_MASK 0x3fff0000
#define XDMA_MSTR_HEIGHT__XDMA_MSTR_FRAME_HEIGHT__SHIFT 0x10
#define XDMA_MSTR_REMOTE_SURFACE_BASE__XDMA_MSTR_REMOTE_SURFACE_BASE_MASK 0xffffffff
#define XDMA_MSTR_REMOTE_SURFACE_BASE__XDMA_MSTR_REMOTE_SURFACE_BASE__SHIFT 0x0
#define XDMA_MSTR_REMOTE_SURFACE_BASE_HIGH__XDMA_MSTR_REMOTE_SURFACE_BASE_HIGH_MASK 0xff
#define XDMA_MSTR_REMOTE_SURFACE_BASE_HIGH__XDMA_MSTR_REMOTE_SURFACE_BASE_HIGH__SHIFT 0x0
#define XDMA_MSTR_REMOTE_GPU_ADDRESS__XDMA_MSTR_REMOTE_GPU_ADDRESS_MASK 0xffffffff
#define XDMA_MSTR_REMOTE_GPU_ADDRESS__XDMA_MSTR_REMOTE_GPU_ADDRESS__SHIFT 0x0
#define XDMA_MSTR_REMOTE_GPU_ADDRESS_HIGH__XDMA_MSTR_REMOTE_GPU_ADDRESS_HIGH_MASK 0xff
#define XDMA_MSTR_REMOTE_GPU_ADDRESS_HIGH__XDMA_MSTR_REMOTE_GPU_ADDRESS_HIGH__SHIFT 0x0
#define XDMA_MSTR_CACHE_BASE_ADDR__XDMA_MSTR_CACHE_BASE_ADDR_MASK 0xffffffff
#define XDMA_MSTR_CACHE_BASE_ADDR__XDMA_MSTR_CACHE_BASE_ADDR__SHIFT 0x0
#define XDMA_MSTR_CACHE_BASE_ADDR_HIGH__XDMA_MSTR_CACHE_BASE_ADDR_HIGH_MASK 0xff
#define XDMA_MSTR_CACHE_BASE_ADDR_HIGH__XDMA_MSTR_CACHE_BASE_ADDR_HIGH__SHIFT 0x0
#define XDMA_MSTR_CACHE__XDMA_MSTR_CACHE_PITCH_MASK 0x3fff
#define XDMA_MSTR_CACHE__XDMA_MSTR_CACHE_PITCH__SHIFT 0x0
#define XDMA_MSTR_CACHE__XDMA_MSTR_CACHE_TLB_PG_STATE_MASK 0x60000000
#define XDMA_MSTR_CACHE__XDMA_MSTR_CACHE_TLB_PG_STATE__SHIFT 0x1d
#define XDMA_MSTR_CACHE__XDMA_MSTR_CACHE_TLB_PG_TRANS_MASK 0x80000000
#define XDMA_MSTR_CACHE__XDMA_MSTR_CACHE_TLB_PG_TRANS__SHIFT 0x1f
#define XDMA_MSTR_CHANNEL_START__XDMA_MSTR_CHANNEL_START_X_MASK 0x3fff
#define XDMA_MSTR_CHANNEL_START__XDMA_MSTR_CHANNEL_START_X__SHIFT 0x0
#define XDMA_MSTR_CHANNEL_START__XDMA_MSTR_CHANNEL_START_Y_MASK 0x3fff0000
#define XDMA_MSTR_CHANNEL_START__XDMA_MSTR_CHANNEL_START_Y__SHIFT 0x10
#define XDMA_MSTR_PERFMEAS_STATUS__XDMA_MSTR_PERFMEAS_DATA_MASK 0xffffff
#define XDMA_MSTR_PERFMEAS_STATUS__XDMA_MSTR_PERFMEAS_DATA__SHIFT 0x0
#define XDMA_MSTR_PERFMEAS_STATUS__XDMA_MSTR_PERFMEAS_INDEX_MASK 0x7000000
#define XDMA_MSTR_PERFMEAS_STATUS__XDMA_MSTR_PERFMEAS_INDEX__SHIFT 0x18
#define XDMA_MSTR_PERFMEAS_STATUS__XDMA_MSTR_PERFMEAS_INDEX_MODE_MASK 0xc0000000
#define XDMA_MSTR_PERFMEAS_STATUS__XDMA_MSTR_PERFMEAS_INDEX_MODE__SHIFT 0x1e
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_CACHE_BW_MEAS_ITER_MASK 0xfff
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_CACHE_BW_MEAS_ITER__SHIFT 0x0
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_CACHE_BW_SEGID_SEL_MASK 0x1f000
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_CACHE_BW_SEGID_SEL__SHIFT 0xc
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_CACHE_BW_COUNTER_RST_MASK 0x20000
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_CACHE_BW_COUNTER_RST__SHIFT 0x11
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_LT_MEAS_ITER_MASK 0x7ff80000
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_LT_MEAS_ITER__SHIFT 0x13
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_LT_COUNTER_RST_MASK 0x80000000
#define XDMA_MSTR_PERFMEAS_CNTL__XDMA_MSTR_LT_COUNTER_RST__SHIFT 0x1f
#define XDMA_SLV_CNTL__XDMA_SLV_READ_LINES_MASK 0x1
#define XDMA_SLV_CNTL__XDMA_SLV_READ_LINES__SHIFT 0x0
#define XDMA_SLV_CNTL__XDMA_SLV_MEM_READY_MASK 0x200
#define XDMA_SLV_CNTL__XDMA_SLV_MEM_READY__SHIFT 0x9
#define XDMA_SLV_CNTL__XDMA_SLV_ACTIVE_MASK 0x400
#define XDMA_SLV_CNTL__XDMA_SLV_ACTIVE__SHIFT 0xa
#define XDMA_SLV_CNTL__XDMA_SLV_ALPHA_POSITION_MASK 0x3000
#define XDMA_SLV_CNTL__XDMA_SLV_ALPHA_POSITION__SHIFT 0xc
#define XDMA_S