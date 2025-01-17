2_CNTL__STEP_MASK 0x2
#define SDMA0_F32_CNTL__STEP__SHIFT 0x1
#define SDMA0_F32_CNTL__DBG_SELECT_BITS_MASK 0xfc
#define SDMA0_F32_CNTL__DBG_SELECT_BITS__SHIFT 0x2
#define SDMA0_FREEZE__FREEZE_MASK 0x10
#define SDMA0_FREEZE__FREEZE__SHIFT 0x4
#define SDMA0_FREEZE__FROZEN_MASK 0x20
#define SDMA0_FREEZE__FROZEN__SHIFT 0x5
#define SDMA0_FREEZE__F32_FREEZE_MASK 0x40
#define SDMA0_FREEZE__F32_FREEZE__SHIFT 0x6
#define SDMA0_PHASE0_QUANTUM__UNIT_MASK 0xf
#define SDMA0_PHASE0_QUANTUM__UNIT__SHIFT 0x0
#define SDMA0_PHASE0_QUANTUM__VALUE_MASK 0xffff00
#define SDMA0_PHASE0_QUANTUM__VALUE__SHIFT 0x8
#define SDMA0_PHASE0_QUANTUM__PREFER_MASK 0x40000000
#define SDMA0_PHASE0_QUANTUM__PREFER__SHIFT 0x1e
#define SDMA0_PHASE1_QUANTUM__UNIT_MASK 0xf
#define SDMA0_PHASE1_QUANTUM__UNIT__SHIFT 0x0
#define SDMA0_PHASE1_QUANTUM__VALUE_MASK 0xffff00
#define SDMA0_PHASE1_QUANTUM__VALUE__SHIFT 0x8
#define SDMA0_PHASE1_QUANTUM__PREFER_MASK 0x40000000
#define SDMA0_PHASE1_QUANTUM__PREFER__SHIFT 0x1e
#define SDMA_POWER_GATING__PG_CNTL_ENABLE_MASK 0x1
#define SDMA_POWER_GATING__PG_CNTL_ENABLE__SHIFT 0x0
#define SDMA_POWER_GATING__AUTOMATIC_STATUS_ENABLE_MASK 0x2
#define SDMA_POWER_GATING__AUTOMATIC_STATUS_ENABLE__SHIFT 0x1
#define SDMA_POWER_GATING__PG_STATE_VALID_MASK 0x4
#define SDMA_POWER_GATING__PG_STATE_VALID__SHIFT 0x2
#define SDMA_POWER_GATING__PG_CNTL_STATUS_MASK 0x30
#define SDMA_POWER_GATING__PG_CNTL_STATUS__SHIFT 0x4
#define SDMA_POWER_GATING__SDMA0_ON_CONDITION_MASK 0x40
#define SDMA_POWER_GATING__SDMA0_ON_CONDITION__SHIFT 0x6
#define SDMA_POWER_GATING__SDMA1_ON_CONDITION_MASK 0x80
#define SDMA_POWER_GATING__SDMA1_ON_CONDITION__SHIFT 0x7
#define SDMA_POWER_GATING__POWER_OFF_DELAY_MASK 0xfff00
#define SDMA_POWER_GATING__POWER_OFF_DELAY__SHIFT 0x8
#define SDMA_POWER_GATING__POWER_ON_DELAY_MASK 0xfff00000
#define SDMA_POWER_GATING__POWER_ON_DELAY__SHIFT 0x14
#define SDMA_PGFSM_CONFIG__FSM_ADDR_MASK 0xff
#define SDMA_PGFSM_CONFIG__FSM_ADDR__SHIFT 0x0
#define SDMA_PGFSM_CONFIG__POWER_DOWN_MASK 0x100
#define SDMA_PGFSM_CONFIG__POWER_DOWN__SHIFT 0x8
#define SDMA_PGFSM_CONFIG__POWER_UP_MASK 0x200
#define SDMA_PGFSM_CONFIG__POWER_UP__SHIFT 0x9
#define SDMA_PGFSM_CONFIG__P1_SELECT_MASK 0x400
#define SDMA_PGFSM_CONFIG__P1_SELECT__SHIFT 0xa
#define SDMA_PGFSM_CONFIG__P2_SELECT_MASK 0x800
#define SDMA_PGFSM_CONFIG__P2_SELECT__SHIFT 0xb
#define SDMA_PGFSM_CONFIG__WRITE_MASK 0x1000
#define SDMA_PGFSM_CONFIG__WRITE__SHIFT 0xc
#define SDMA_PGFSM_CONFIG__READ_MASK 0x2000
#define SDMA_PGFSM_CONFIG__READ__SHIFT 0xd
#define SDMA_PGFSM_CONFIG__SRBM_OVERRIDE_MASK 0x8000000
#define SDMA_PGFSM_CONFIG__SRBM_OVERRIDE__SHIFT 0x1b
#define SDMA_PGFSM_CONFIG__REG_ADDR_MASK 0xf0000000
#define SDMA_PGFSM_CONFIG__REG_ADDR__SHIFT 0x1c
#define SDMA_PGFSM_WRITE__VALUE_MASK 0xffffffff
#define SDMA_PGFSM_WRITE__VALUE__SHIFT 0x0
#define SDMA_PGFSM_READ__VALUE_MASK 0xffffff
#define SDMA_PGFSM_READ__VALUE__SHIFT 0x0
#define SDMA0_EDC_CONFIG__DIS_EDC_MASK 0x2
#define SDMA0_EDC_CONFIG__DIS_EDC__SHIFT 0x1
#define SDMA0_EDC_CONFIG__ECC_INT_ENABLE_MASK 0x4
#define SDMA0_EDC_CONFIG__ECC_INT_ENABLE__SHIFT 0x2
#define SDMA0_VM_CNTL__CMD_MASK 0xf
#define SDMA0_VM_CNTL__CMD__SHIFT 0x0
#define SDMA0_VM_CTX_LO__ADDR_MASK 0xfffffffc
#define SDMA0_VM_CTX_LO__ADDR__SHIFT 0x2
#define SDMA0_VM_CTX_HI__ADDR_MASK 0xffffffff
#define SDMA0_VM_CTX_HI__ADDR__SHIFT 0x0
#define SDMA0_STATUS2_REG__ID_MASK 0x3
#define SDMA0_STATUS2_REG__ID__SHIFT 0x0
#define SDMA0_STATUS2_REG__F32_INSTR_PTR_MASK 0xffc
#define SDMA0_STATUS2_REG__F32_INSTR_PTR__SHIFT 0x2
#define SDMA0_STATUS2_REG__CURRENT_FCN_IDLE_MASK 0xc000
#define SDMA0_STATUS2_REG__CURRENT_FCN_IDLE__SHIFT 0xe
#define SDMA0_STATUS2_REG__CMD_OP_MASK 0xffff0000
#define SDMA0_STATUS2_REG__CMD_OP__SHIFT 0x10
#define SDMA0_ACTIVE_FCN_ID__VFID_MASK 0xf
#define SDMA0_ACTIVE_FCN_ID__VFID__SHIFT 0x0
#define SDMA0_ACTIVE_FCN_ID__VF_MASK 0x80000000
#define SDMA0_ACTIVE_FCN_ID__VF__SHIFT 0x1f
#define SDMA0_VM_CTX_CNTL__PRIV_MASK 0x1
#define SDMA0_VM_CTX_CNTL__PRIV__SHIFT 0x0
#define SDMA0_VM_CTX_CNTL__VMID_MASK 0xf0
#define SDMA0_VM_CTX_CNTL__VMID__SHIFT 0x4
#define SDMA0_VIRT_RESET_REQ__VF_MASK 0xffff
#define SDMA0_VIRT_RESET_REQ__VF__SHIFT 0x0
#define SDMA0_VIRT_RESET_REQ__PF_MASK 0x80000000
#define SDMA0_VIRT_RESET_REQ__PF__SHIFT 0x1f
#define SDMA0_VF_ENABLE__VF_ENABLE_MASK 0x1
#define SDMA0_VF_ENABLE__VF_ENABLE__SHIFT 0x0
#define SDMA0_BA_THRESHOLD__READ_THRES_MASK 0x3ff
#define SDMA0_BA_THRESHOLD__READ_THRES__SHIFT 0x0
#define SDMA0_BA_THRESHOLD__WRITE_THRES_MASK 0x3ff0000
#define SDMA0_BA_THRESHOLD__WRITE_THRES__SHIFT 0x10
#define SDMA0_ID__DEVICE_ID_MASK 0xff
#define SDMA0_ID__DEVICE_ID__SHIFT 0x0
#define SDMA0_VERSION__VALUE_MASK 0xffff
#define SDMA0_VERSION__VALUE__SHIFT 0x0
#define SDMA0_ATOMIC_CNTL__LOOP_TIMER_MASK 0x7fffffff
#define SDMA0_ATOMIC_CNTL__LOOP_TIMER__SHIFT 0x0
#define SDMA0_ATOMIC_CNTL__ATOMIC_RTN_INT_ENABLE_MASK 0x80000000
#define SDMA0_ATOMIC_CNTL__ATOMIC_RTN_INT_ENABLE__SHIFT 0x1f
#define SDMA0_ATOMIC_PREOP_LO__DATA_MASK 0xffffffff
#define SDMA0_ATOMIC_PREOP_LO__DATA__SHIFT 0x0
#define SDMA0_ATOMIC_PREOP_HI__DATA_MASK 0xffffffff
#define SDMA0_ATOMIC_PREOP_HI__DATA__SHIFT 0x0
#define SDMA0_POWER_CNTL_IDLE__D