UNT_MASK 0x3fff
#define SDMA1_RLC0_SKIP_CNTL__SKIP_COUNT__SHIFT 0x0
#define SDMA1_RLC0_CONTEXT_STATUS__SELECTED_MASK 0x1
#define SDMA1_RLC0_CONTEXT_STATUS__SELECTED__SHIFT 0x0
#define SDMA1_RLC0_CONTEXT_STATUS__IDLE_MASK 0x4
#define SDMA1_RLC0_CONTEXT_STATUS__IDLE__SHIFT 0x2
#define SDMA1_RLC0_CONTEXT_STATUS__EXPIRED_MASK 0x8
#define SDMA1_RLC0_CONTEXT_STATUS__EXPIRED__SHIFT 0x3
#define SDMA1_RLC0_CONTEXT_STATUS__EXCEPTION_MASK 0x70
#define SDMA1_RLC0_CONTEXT_STATUS__EXCEPTION__SHIFT 0x4
#define SDMA1_RLC0_CONTEXT_STATUS__CTXSW_ABLE_MASK 0x80
#define SDMA1_RLC0_CONTEXT_STATUS__CTXSW_ABLE__SHIFT 0x7
#define SDMA1_RLC0_CONTEXT_STATUS__CTXSW_READY_MASK 0x100
#define SDMA1_RLC0_CONTEXT_STATUS__CTXSW_READY__SHIFT 0x8
#define SDMA1_RLC0_CONTEXT_STATUS__PREEMPTED_MASK 0x200
#define SDMA1_RLC0_CONTEXT_STATUS__PREEMPTED__SHIFT 0x9
#define SDMA1_RLC0_CONTEXT_STATUS__PREEMPT_DISABLE_MASK 0x400
#define SDMA1_RLC0_CONTEXT_STATUS__PREEMPT_DISABLE__SHIFT 0xa
#define SDMA1_RLC0_DOORBELL__OFFSET_MASK 0x1fffff
#define SDMA1_RLC0_DOORBELL__OFFSET__SHIFT 0x0
#define SDMA1_RLC0_DOORBELL__ENABLE_MASK 0x10000000
#define SDMA1_RLC0_DOORBELL__ENABLE__SHIFT 0x1c
#define SDMA1_RLC0_DOORBELL__CAPTURED_MASK 0x40000000
#define SDMA1_RLC0_DOORBELL__CAPTURED__SHIFT 0x1e
#define SDMA1_RLC0_VIRTUAL_ADDR__ATC_MASK 0x1
#define SDMA1_RLC0_VIRTUAL_ADDR__ATC__SHIFT 0x0
#define SDMA1_RLC0_VIRTUAL_ADDR__INVAL_MASK 0x2
#define SDMA1_RLC0_VIRTUAL_ADDR__INVAL__SHIFT 0x1
#define SDMA1_RLC0_VIRTUAL_ADDR__PTR32_MASK 0x10
#define SDMA1_RLC0_VIRTUAL_ADDR__PTR32__SHIFT 0x4
#define SDMA1_RLC0_VIRTUAL_ADDR__SHARED_BASE_MASK 0x700
#define SDMA1_RLC0_VIRTUAL_ADDR__SHARED_BASE__SHIFT 0x8
#define SDMA1_RLC0_VIRTUAL_ADDR__VM_HOLE_MASK 0x40000000
#define SDMA1_RLC0_VIRTUAL_ADDR__VM_HOLE__SHIFT 0x1e
#define SDMA1_RLC0_APE1_CNTL__BASE_MASK 0xffff
#define SDMA1_RLC0_APE1_CNTL__BASE__SHIFT 0x0
#define SDMA1_RLC0_APE1_CNTL__LIMIT_MASK 0xffff0000
#define SDMA1_RLC0_APE1_CNTL__LIMIT__SHIFT 0x10
#define SDMA1_RLC0_DOORBELL_LOG__BE_ERROR_MASK 0x1
#define SDMA1_RLC0_DOORBELL_LOG__BE_ERROR__SHIFT 0x0
#define SDMA1_RLC0_DOORBELL_LOG__DATA_MASK 0xfffffffc
#define SDMA1_RLC0_DOORBELL_LOG__DATA__SHIFT 0x2
#define SDMA1_RLC0_WATERMARK__RD_OUTSTANDING_MASK 0xfff
#define SDMA1_RLC0_WATERMARK__RD_OUTSTANDING__SHIFT 0x0
#define SDMA1_RLC0_WATERMARK__WR_OUTSTANDING_MASK 0x1ff0000
#define SDMA1_RLC0_WATERMARK__WR_OUTSTANDING__SHIFT 0x10
#define SDMA1_RLC0_CSA_ADDR_LO__ADDR_MASK 0xfffffffc
#define SDMA1_RLC0_CSA_ADDR_LO__ADDR__SHIFT 0x2
#define SDMA1_RLC0_CSA_ADDR_HI__ADDR_MASK 0xffffffff
#define SDMA1_RLC0_CSA_ADDR_HI__ADDR__SHIFT 0x0
#define SDMA1_RLC0_IB_SUB_REMAIN__SIZE_MASK 0x3fff
#define SDMA1_RLC0_IB_SUB_REMAIN__SIZE__SHIFT 0x0
#define SDMA1_RLC0_PREEMPT__IB_PREEMPT_MASK 0x1
#define SDMA1_RLC0_PREEMPT__IB_PREEMPT__SHIFT 0x0
#define SDMA1_RLC0_DUMMY_REG__DUMMY_MASK 0xffffffff
#define SDMA1_RLC0_DUMMY_REG__DUMMY__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA0__DATA0_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA0__DATA0__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA1__DATA1_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA1__DATA1__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA2__DATA2_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA2__DATA2__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA3__DATA3_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA3__DATA3__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA4__DATA4_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA4__DATA4__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA5__DATA5_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA5__DATA5__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA6__DATA6_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA6__DATA6__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA7__DATA7_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA7__DATA7__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_DATA8__DATA8_MASK 0xffffffff
#define SDMA1_RLC0_MIDCMD_DATA8__DATA8__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_CNTL__DATA_VALID_MASK 0x1
#define SDMA1_RLC0_MIDCMD_CNTL__DATA_VALID__SHIFT 0x0
#define SDMA1_RLC0_MIDCMD_CNTL__COPY_MODE_MASK 0x2
#define SDMA1_RLC0_MIDCMD_CNTL__COPY_MODE__SHIFT 0x1
#define SDMA1_RLC0_MIDCMD_CNTL__SPLIT_STATE_MASK 0xf0
#define SDMA1_RLC0_MIDCMD_CNTL__SPLIT_STATE__SHIFT 0x4
#define SDMA1_RLC0_MIDCMD_CNTL__ALLOW_PREEMPT_MASK 0x100
#define SDMA1_RLC0_MIDCMD_CNTL__ALLOW_PREEMPT__SHIFT 0x8
#define SDMA1_RLC1_RB_CNTL__RB_ENABLE_MASK 0x1
#define SDMA1_RLC1_RB_CNTL__RB_ENABLE__SHIFT 0x0
#define SDMA1_RLC1_RB_CNTL__RB_SIZE_MASK 0x3e
#define SDMA1_RLC1_RB_CNTL__RB_SIZE__SHIFT 0x1
#define SDMA1_RLC1_RB_CNTL__RB_SWAP_ENABLE_MASK 0x200
#define SDMA1_RLC1_RB_CNTL__RB_SWAP_ENABLE__SHIFT 0x9
#define SDMA1_RLC1_RB_CNTL__RPTR_WRITEBACK_ENABLE_MASK 0x1000
#define SDMA1_RLC1_RB_CNTL__RPTR_WRITEBACK_ENABLE__SHIFT 0xc
#define SDMA1_RLC1_RB_CNTL__RPTR_WRITEBACK_SWAP_ENABLE_MASK 0x2000
#define SDMA1_RLC1_RB_CNTL__RPTR_WRITEBACK_SWAP_ENABLE__SHIFT 0xd
#define SDMA1_RLC1_RB_CNTL__RPTR_WRITEBACK_TIMER_MASK 0x1f0000
#define SDMA1_RLC1_RB_CNTL__RPTR_WRITEBACK_TIMER__SHIFT 0x10
#define SDMA1_RLC1_RB_CNTL__RB_PRIV_MASK 0x800000
#define SDMA1_RLC1_RB_CNTL__RB_PRIV__SHIFT 0x17
#define SDMA1_RLC1_RB_CNTL__RB_VMID_MASK 0xf000000
#define SDMA1_RLC1_RB_CNTL__RB_VMID__SHIFT 0x18
#define SDMA1_RLC1_RB_BASE__ADDR_MASK 0xffffffff
#define SDMA1_RLC1_RB_BASE__ADDR__SHIFT 0x0
#define SDMA1_RLC1_RB_BASE_HI__ADDR_MASK 0xffffff
#define SDMA1_RLC1_RB_BASE_HI__ADDR__SHIFT 0x0
#define SDMA1_RLC1_RB_RPTR__OFFSET_MASK 0xfffffffc
#define SDMA1_RLC1_RB_RPTR__OFFSET__SHIFT 0x2
#define SDMA1_RLC1_RB_WPTR__OFFSET_MASK 0xfffffffc
#define SDMA1_RLC1_RB_WPTR__OFFSET__SHIFT 0x2
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__ENABLE_MASK 0x1
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__ENABLE__SHIFT 0x0
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__SWAP_ENABLE_MASK 0x2
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__SWAP_ENABLE__SHIFT 0x1
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__F32_POLL_ENABLE_MASK 0x4
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__F32_POLL_ENABLE__SHIFT 0x2
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__FREQUENCY_MASK 0xfff0
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__FREQUENCY__SHIFT 0x4
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__IDLE_POLL_COUNT_MASK 0xffff0000
#define SDMA1_RLC1_RB_WPTR_POLL_CNTL__IDLE_POLL_COUNT__SHIFT 0x10
#define SDMA1_RLC1_RB_WPTR_POLL_ADDR_HI__ADDR_MASK 0xffffffff
#define SDMA1_RLC1_RB_WPTR_POLL_ADDR_HI__ADDR__SHIFT 0x0
#define 