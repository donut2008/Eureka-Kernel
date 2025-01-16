0
#define D3F5_COMMAND__FAST_B2B_EN__SHIFT 0x9
#define D3F5_COMMAND__INT_DIS_MASK 0x400
#define D3F5_COMMAND__INT_DIS__SHIFT 0xa
#define D3F5_STATUS__INT_STATUS_MASK 0x80000
#define D3F5_STATUS__INT_STATUS__SHIFT 0x13
#define D3F5_STATUS__CAP_LIST_MASK 0x100000
#define D3F5_STATUS__CAP_LIST__SHIFT 0x14
#define D3F5_STATUS__PCI_66_EN_MASK 0x200000
#define D3F5_STATUS__PCI_66_EN__SHIFT 0x15
#define D3F5_STATUS__FAST_BACK_CAPABLE_MASK 0x800000
#define D3F5_STATUS__FAST_BACK_CAPABLE__SHIFT 0x17
#define D3F5_STATUS__MASTER_DATA_PARITY_ERROR_MASK 0x1000000
#define D3F5_STATUS__MASTER_DATA_PARITY_ERROR__SHIFT 0x18
#define D3F5_STATUS__DEVSEL_TIMING_MASK 0x6000000
#define D3F5_STATUS__DEVSEL_TIMING__SHIFT 0x19
#define D3F5_STATUS__SIGNAL_TARGET_ABORT_MASK 0x8000000
#define D3F5_STATUS__SIGNAL_TARGET_ABORT__SHIFT 0x1b
#define D3F5_STATUS__RECEIVED_TARGET_ABORT_MASK 0x10000000
#define D3F5_STATUS__RECEIVED_TARGET_ABORT__SHIFT 0x1c
#define D3F5_STATUS__RECEIVED_MASTER_ABORT_MASK 0x20000000
#define D3F5_STATUS__RECEIVED_MASTER_ABORT__SHIFT 0x1d
#define D3F5_STATUS__SIGNALED_SYSTEM_ERROR_MASK 0x40000000
#define D3F5_STATUS__SIGNALED_SYSTEM_ERROR__SHIFT 0x1e
#define D3F5_STATUS__PARITY_ERROR_DETECTED_MASK 0x80000000
#define D3F5_STATUS__PARITY_ERROR_DETECTED__SHIFT 0x1f
#define D3F5_REVISION_ID__MINOR_REV_ID_MASK 0xf
#define D3F5_REVISION_ID__MINOR_REV_ID__SHIFT 0x0
#define D3F5_REVISION_ID__MAJOR_REV_ID_MASK 0xf0
#define D3F5_REVISION_ID__MAJOR_REV_ID__SHIFT 0x4
#define D3F5_PROG_INTERFACE__PROG_INTERFACE_MASK 0xff00
#define D3F5_PROG_INTERFACE__PROG_INTERFACE__SHIFT 0x8
#define D3F5_SUB_CLASS__SUB_CLASS_MASK 0xff0000
#define D3F5_SUB_CLASS__SUB_CLASS__SHIFT 0x10
#define D3F5_BASE_CLASS__BASE_CLASS_MASK 0xff000000
#define D3F5_BASE_CLASS__BASE_CLASS__SHIFT 0x18
#define D3F5_CACHE_LINE__CACHE_LINE_SIZE_MASK 0xff
#define D3F5_CACHE_LINE__CACHE_LINE_SIZE__SHIFT 0x0
#define D3F5_LATENCY__LATENCY_TIMER_MASK 0xff00
#define D3F5_LATENCY__LATENCY_TIMER__SHIFT 0x8
#define D3F5_HEADER__HEADER_TYPE_MASK 0x7f0000
#define D3F5_HEADER__HEADER_TYPE__SHIFT 0x10
#define D3F5_HEADER__DEVICE_TYPE_MASK 0x800000
#define D3F5_HEADER__DEVICE_TYPE__SHIFT 0x17
#define D3F5_BIST__BIST_COMP_MASK 0xf000000
#define D3F5_BIST__BIST_COMP__SHIFT 0x18
#define D3F5_BIST__BIST_STRT_MASK 0x40000000
#define D3F5_BIST__BIST_STRT__SHIFT 0x1e
#define D3F5_BIST__BIST_CAP_MASK 0x80000000
#define D3F5_BIST__BIST_CAP__SHIFT 0x1f
#define D3F5_SUB_BUS_NUMBER_LATENCY__PRIMARY_BUS_MASK 0xff
#define D3F5_SUB_BUS_NUMBER_LATENCY__PRIMARY_BUS__SHIFT 0x0
#define D3F5_SUB_BUS_NUMBER_LATENCY__SECONDARY_BUS_MASK 0xff00
#define D3F5_SUB_BUS_NUMBER_LATENCY__SECONDARY_BUS__SHIFT 0x8
#define D3F5_SUB_BUS_NUMBER_LATENCY__SUB_BUS_NUM_MASK 0xff0000
#define D3F5_SUB_BUS_NUMBER_LATENCY__SUB_BUS_NUM__SHIFT 0x10
#define D3F5_SUB_BUS_NUMBER_LATENCY__SECONDARY_LATENCY_TIMER_MASK 0xff000000
#define D3F5_SUB_BUS_NUMBER_LATENCY__SECONDARY_LATENCY_TIMER__SHIFT 0x18
#define D3F5_IO_BASE_LIMIT__IO_BASE_TYPE_MASK 0xf
#define D3F5_IO_BASE_LIMIT__IO_BASE_TYPE__SHIFT 0x0
#define D3F5_IO_BASE_LIMIT__IO_BASE_MASK 0xf0
#define D3F5_IO_BASE_LIMIT__IO_BASE__SHIFT 0x4
#define D3F5_IO_BASE_LIMIT__IO_LIMIT_TYPE_MASK 0xf00
#define D3F5_IO_BASE_LIMIT__IO_LIMIT_TYPE__SHIFT 0x8
#define D3F5_IO_BASE_LIMIT__IO_LIMIT_MASK 0xf000
#define D3F5_IO_BASE_LIMIT__IO_LIMIT__SHIFT 0xc
#define D3F5_SECONDARY_STATUS__CAP_LIST_MASK 0x100000
#define D3F5_SECONDARY_STATUS__CAP_LIST__SHIFT 0x14
#define D3F5_SECONDARY_STATUS__PCI_66_EN_MASK 0x200000
#define D3F5_SECONDARY_STATUS__PCI_66_EN__SHIFT 0x15
#define D3F5_SECONDARY_STATUS__FAST_BACK_CAPABLE_MASK 0x800000
#define D3F5_SECONDARY_STATUS__FAST_BACK_CAPABLE__SHIFT 0x17
#define D3F5_SECONDARY_STATUS__MASTER_DATA_PARITY_ERROR_MASK 0x1000000
#define D3F5_SECONDARY_STATUS__MASTER_DATA_PARITY_ERROR__SHIFT 0x18
#define D3F5_SECONDARY_STATUS__DEVSEL_TIMING_MASK 0x6000000
#define D3F5_SECONDARY_STATUS__DEVSEL_TIMING__SHIFT 0x19
#define D3F5_SECONDARY_STATUS__SIGNAL_TARGET_ABORT_MASK 0x8000000
#define D3F5_SECONDARY_STATUS__SIGNAL_TARGET_ABORT__SHIFT 0x1b
#define D3F5_SECONDARY_STATUS__RECEIVED_TARGET_ABORT_MASK 0x10000000
#define D3F5_SECONDARY_STATUS__RECEIVED_TARGET_ABORT__SHIFT 0x1c
#define D3F5_SECONDARY_STATUS__RECEIVED_MASTER_ABORT_MASK 0x20000000
#define D3F5_SECONDARY_STATUS__RECEIVED_MASTER_ABORT__SHIFT 0x1d
#define D3F5_SECONDARY_STATUS__RECEIVED_SYSTEM_ERROR_MASK 0x40000000
#define D3F5_SECONDARY_STATUS__RECEIVED_SYSTEM_ERROR__SHIFT 0x1e
#define D3F5_SECONDARY_STATUS__PARITY_ERROR_DETECTED_MASK 0x80000000
#define D3F5_SECONDARY_STATUS__PARITY_ERROR_DETECTED__SHIFT 0x1f
#define D3F5_MEM_BASE_LIMIT__MEM_BASE_TYPE_MASK 0xf
#define D3F5_MEM_BASE_LIMIT__MEM_BASE_TYPE__SHIFT 0x0
#define D3F5_MEM_BASE_LIMIT__MEM_BASE_31_20_MASK 0xfff0
#define D3F5_MEM_BASE_LIMIT__MEM_BASE_31_20__SHIFT 0x4
#define D3F5_MEM_BASE_LIMIT__MEM_LIMIT_TYPE_MASK 0xf0000
#define D3F5_MEM_BASE_LIMIT__MEM_LIMIT_TYPE__SHIFT 0x10
#define D3F5_MEM_BASE_LIMIT__MEM_LIMIT_31_20_MASK 0xfff00000
#define D3F5_MEM_BASE_LIMIT__MEM_LIMIT_31_20__SHIFT 0x14
#define D3F5_PREF_BASE_LIMIT__PREF_MEM_BASE_TYPE_MASK 0xf
#define D3F5_PREF_BASE_LIMIT__PREF_MEM_BASE_TYPE__SHIFT 0x0
#define D3F5_PREF_BASE_LIMIT__PREF_MEM_BASE_31_20_MASK 0xfff0
#define D3F5_PREF_BASE_LIMIT__PREF_MEM_BASE_31_20__SHIFT 0x4
#define D3F5_PREF_BASE_LIMIT__PREF_MEM_LIMIT_TYPE_MASK 0xf0000
#define D3F5_PREF_BASE_LIMIT__PREF_MEM_LIMIT_TYPE__SHIFT 0x10
#define D3F5_PREF_BASE_LIMIT__PREF_MEM_LIMIT_31_20_MASK 0xfff00000
#define D3F5_PREF_BASE_LIMIT__PREF_MEM_LIMIT_31_20__SHIFT 0x14
#define D3F5_PREF_BASE_UPPER__PREF_BASE_UPPER_MASK 0xffffffff
#define D3F5_PREF_BASE_UPPER__PREF_BASE_UPPER__SHIFT 0x0
#define D3F5_PREF_LIMIT_UPPER__PREF_LIMIT_UPPER_MASK 0xffffffff
#define D3F5_PREF_LIMIT_UPPER__PREF_LIMIT_UPPER__SHIFT 0x0
#define D3F5_IO_BASE_LIMIT_HI__IO_BASE_31_16_MASK 0xffff
#define D3F5_IO_BASE_LIMIT_HI__IO_BASE_31_16__SHIFT 0x0
#define D3F5_IO_BASE_LIMIT_HI__IO_LIMIT_31_16_MASK 0xffff0000
#define D3F5_IO_BASE_LIMIT_HI__IO_LIMIT_31_16__SHIFT 0x10
#define D3F5_IRQ_BRIDGE_CNTL__PARITY_RESPONSE_EN_MASK 0x10000
#define D3F5_IRQ_BRIDGE_CNTL__PARITY_RESPONSE_EN__SHIFT 0x10
#define D3F5_IRQ_BRIDGE_CNTL__SERR_EN_MASK 0x20000
#define D3F5_IRQ_BRIDGE_CNTL__SERR_EN__SHIFT 0x11
#define D3F5_IRQ_BRIDGE_CNTL__ISA_EN_MASK 0x40000
#define D3F5_IRQ_BRIDGE_CNTL__ISA_EN__SHIFT 0x12
#define D3F5_IRQ_BRIDGE_CNTL__VGA_EN_MASK 0x80000
#define D3F5_IRQ_BRIDGE_CNTL__VGA_EN__SHIFT 0x13
#define D3F5_IRQ_BRIDGE_CNTL__VGA_DEC_MASK 0x100000
#define D3F5_IRQ_BRIDGE_CNTL__VGA_DEC__SHIFT 0x14
#define D3F5_IRQ_BRIDGE_CNTL__MASTER_ABORT_MODE_MASK 0x200000
#define D3F5_IRQ_BRIDGE_CNTL__MASTER_ABORT_MODE__SHIFT 0x15
#define D3F5_IRQ_BRIDGE_CNTL__SECONDARY_BUS_RESET_MASK 0x400000
#define D3F5_IRQ_BRIDGE_CNTL__SECONDARY_BUS_RESET__SHIFT 0x16
#define D3F5_IRQ_BRIDGE_CNTL__FAST_B2B_EN_MASK 0x800000
#define D3F5_IRQ_BRIDGE_CNTL__FAST_B2B_EN__SHIFT 0x17
#define D3F5_CAP_PTR__CAP_PTR_MASK 0xff
#define D3F5_CAP_PTR__CAP_PTR__SHIFT 0x0
#define D3F5_INTERRUPT_LINE__INTERRUPT_LINE_MASK 0xff
#define D3F5_INTERRUPT_LINE__INTERRUPT_LINE__SHIFT 0x0
#define D3F5_INTERRUPT_PIN__INTERRUPT_PIN_MASK 0xff00
#define D3F5_INTERRUPT_PIN__INTERRUPT_PIN__SHIFT 0x8
#define D3F5_EXT_BRIDGE_CNTL__IO_PORT_80_EN_MASK 0x1
#define D3F5_EXT_BRIDGE_CNTL__IO_PORT_80_EN__SHIFT 0x0
#define D3F5_PMI_CAP_LIST__CAP_ID_MASK 0xff
#define D3F5_PMI_CAP_LIST__CAP_ID__SHIFT 0x0
#define D3F5_PMI_CAP_LIST__NEXT_PTR_MASK 0xff00
#define D3F5_PMI_CAP_LIST__NEXT_PTR__SHIFT 0x8
#define D3F5_PMI_CAP__VERSION_MASK 0x70000
#define D3F5_PMI_CAP__VERSION__SHIFT 0x10
#define D3F5_PMI_CAP__PME_CLOCK_MASK 0x80000
#define D3F5_PMI_CAP__PME_CLOCK__SHIFT 0x13
#define D3F5_PMI_CAP__DEV_SPECIFIC_INIT_MASK 0x200000
#define D3F5_PMI_CAP__DEV_SPECIFIC_INIT__SHIFT 0x15
#define D3F5_PMI_CAP__AUX_CURRENT_MASK 0x1c00000
#define D3F5_PMI_CAP__AUX_CURRENT__SHIFT 0x16
#define D3F5_PMI_CAP__D1_SUPPORT_MASK 0x2000000
#define D3F5_PMI_CAP__D1_SUPPORT__SHIFT 0x19
#define D3F5_PMI_CAP__D2_SUPPORT_MASK 0x4000000
#define D3F5_PMI_CAP__D2_SUPPORT__SHIFT 0x1a
#define D3F5_PMI_CAP__PME_SUPPORT_MASK 0xf8000000
#define D3F5_PMI_CAP__PME_SUPPORT__SHIFT 0x1b
#define D3F5_PMI_STATUS_CNTL__POWER_STATE_MASK 0x3
#define D3F5_PMI_STATUS_CNTL__POWER_STATE__SHIFT 0x0
#define D3F5_PMI_STATUS_CNTL__NO_SOFT_RESET_MASK 0x8
#define D3F5_PMI_STATUS_CNTL__NO_SOFT_RESET__SHIFT 0x3
#define D3F5_PMI_STATUS_CNTL__PME_EN_MASK 0x100
#define D3F5_PMI_STATUS_CNTL__PME_EN__SHIFT 0x8
#define D3F5_PMI_STATUS_CNTL__DATA_SELECT_MASK 0x1e00
#define D3F5_PMI_STATUS_CNTL__DATA_SELECT__SHIFT 0x9
#define D3F5_PMI_STATUS_CNTL__DATA_SCALE_MASK 0x6000
#define D3F5_PMI_STATUS_CNTL__DATA_SCALE__SHIFT 0xd
#define D3F5_PMI_STATUS_CNTL__PME_STATUS_MASK 0x8000
#define D3F5_PMI_STATUS_CNTL__PME_STATUS__SHIFT 0xf
#define D3F5_PMI_STATUS_CNTL__B2_B3_SUPPORT_MASK 0x400000
#define D3F5_PMI_STATUS_CNTL__B2_B3_SUPPORT__SHIFT 0x16
#define D3F5_PMI_STATUS_CNTL__BUS_PWR_EN_MASK 0x800000
#define D3F5_PMI_STATUS_CNTL__BUS_PWR_EN__SHIFT 0x17
#define D3F5_PMI_STATUS_CNTL__PMI_DATA_MASK 0xff000000
#define D3F5_PMI_STATUS_CNTL__PMI_DATA__SHIFT 0x18
#define D3F5_PCIE_CAP_LIST__CAP_ID_MASK 0xff
#define D3F5_PCIE_CAP_LIST__CAP_ID__SHIFT 0x0
#define D3F5_PCIE_CAP_LIST__NEXT_PTR_MASK 0xff00
#define D3F5_PCIE_CAP_LIST__NEXT_PTR__SHIFT 0x8
#define D3F5_PCIE_CAP__VERSION_MASK 0xf0000
#define D3F5_PCIE_CAP__VERSION__SHIFT 0x10
#define D3F5_PCIE_CAP__DEVICE_TYPE_MASK 0xf00000
#define D3F5_PCIE_CAP__DEVICE_TYPE__SHIFT 0x14
#define D3F5_PCIE_CAP__SLOT_IMPLEMENTED_MASK 0x1000000
#define D3F5_PCIE_CAP__SLOT_IMPLEMENTED__SHIFT 0x18
#define D3F5_PCIE_CAP__INT_MESSAGE_NUM_MASK 0x3e000000
#define D3F5_PCIE_CAP__INT_MESSAGE_NUM__SHIFT 0x19
#define D3F5_DEVICE_CAP__MAX_PAYLOAD_SUPPORT_MASK 0x7
#define D3F5_DEVICE_CAP__MAX_PAYLOAD_SUPPORT__SHIFT 0x0
#define D3F5_DEVICE_CAP__PHANTOM_FUNC_MASK 0x18
#define D3F5_DEVICE_CAP__PHANTOM_FUNC__SHIFT 0x3
#define D3F5_DEVICE_CAP__EXTENDED_TAG_MASK 0x20
#define D3F5_DEVICE_CAP__EXTENDED_TAG__SHIFT 0x5
#define D3F5_DEVICE_CAP__L0S_ACCEPTABLE_LATENCY_MASK 0x1c0
#define D3F5_DEVICE_CAP__L0S_ACCEPTABLE_LATENCY__SHIFT 0x6
#define D3F5_DEVICE_CAP__L1_ACCEPTABLE_LATENCY_MASK 0xe00
#define D3F5_DEVICE_CAP__L1_ACCEPTABLE_LATENCY__SHIFT 0x9
#define D3F5_DEVICE_CAP__ROLE_BASED_ERR_REPORTING_MASK 0x8000
#define D3F5_DEVICE_CAP__ROLE_BASED_ERR_REPORTING__SHIFT 0xf
#define D3F5_DEVICE_CAP__CAPTURED_SLOT_POWER_LIMIT_MASK 0x3fc0000
#define D3F5_DEVICE_CAP__CAPTURED_SLOT_POWER_LIMIT__SHIFT 0x12
#define D3F5_DEVICE_CAP__CAPTURED_SLOT_POWER_SCALE_MASK 0xc000000
#define D3F5_DEVICE_CAP__CAPTURED_SLOT_POWER_SCALE__SHIFT 0x1a
#define D3F5_DEVICE_CAP__FLR_CAPABLE_MASK 0x10000000
#define D3F5_DEVICE_CAP__FLR_CAPABLE__SHIFT 0x1c
#define D3F5_DEVICE_CNTL__CORR_ERR_EN_MASK 0x1
#define D3F5_DEVICE_CNTL__CORR_ERR_EN__SHIFT 0x0
#define D3F5_DEVICE_CNTL__NON_FATAL_ERR_EN_MASK 0x2
#define D3F5_DEVICE_CNTL__NON_FATAL_ERR_EN__SHIFT 0x1
#define D3F5_DEVICE_CNTL__FATAL_ERR_EN_MASK 0x4
#define D3F5_DEVICE_CNTL__FATAL_ERR_EN__SHIFT 0x2
#define D3F5_DEVICE_CNTL__USR_REPORT_EN_MASK 0x8
#define D3F5_DEVICE_CNTL__USR_REPORT_EN__SHIFT 0x3
#define D3F5_DEVICE_CNTL__RELAXED_ORD_EN_MASK 0x10
#define D3F5_DEVICE_CNTL__RELAXED_ORD_EN__SHIFT 0x4
#define D3F5_DEVICE_CNTL__MAX_PAYLOAD_SIZE_MASK 0xe0
#define D3F5_DEVICE_CNTL__MAX_PAYLOAD_SIZE__SHIFT 0x5
#define D3F5_DEVICE_CNTL__EXTENDED_TAG_EN_MASK 0x100
#define D3F5_DEVICE_CNTL__EXTENDED_TAG_EN__SHIFT 0x8
#define D3F5_DEVICE_CNTL__PHANTOM_FUNC_EN_MASK 0x200
#define D3F5_DEVICE_CNTL__PHANTOM_FUNC_EN__SHIFT 0x9
#define D3F5_DEVICE_CNTL__AUX_POWER_PM_EN_MASK 0x400
#define D3F5_DEVICE_CNTL__AUX_POWER_PM_EN__SHIFT 0xa
#define D3F5_DEVICE_CNTL__NO_SNOOP_EN_MASK 0x800
#define D3F5_DEVICE_CNTL__NO_SNOOP_EN__SHIFT 0xb
#define D3F5_DEVICE_CNTL__MAX_READ_REQUEST_SIZE_MASK 0x7000
#define D3F5_DEVICE_CNTL__MAX_READ_REQUEST_SIZE__SHIFT 0xc
#define D3F5_DEVICE_CNTL__BRIDGE_CFG_RETRY_EN_MASK 0x8000
#define D3F5_DEVICE_CNTL__BRIDGE_CFG_RETRY_EN__SHIFT 0xf
#define D3F5_DEVICE_STATUS__CORR_ERR_MASK 0x10000
#define D3F5_DEVICE_STATUS__CORR_ERR__SHIFT 0x10
#define D3F5_DEVICE_STATUS__NON_FATAL_ERR_MASK 0x20000
#define D3F5_DEVICE_STATUS__NON_FATAL_ERR__SHIFT 0x11
#define D3F5_DEVICE_STATUS__FATAL_ERR_MASK 0x40000
#define D3F5_DEVICE_STATUS__FATAL_ERR__SHIFT 0x12
#define D3F5_DEVICE_STATUS__USR_DETECTED_MASK 0x80000
#define D3F5_DEVICE_STATUS__USR_DETECTED__SHIFT 0x13
#define D3F5_DEVICE_STATUS__AUX_PWR_MASK 0x100000
#define D3F5_DEVICE_STATUS__AUX_PWR__SHIFT 0x14
#define D3F5_DEVICE_STATUS__TRANSACTIONS_PEND_MASK 0x200000
#define D3F5_DEVICE_STATUS__TRANSACTIONS_PEND__SHIFT 0x15
#define D3F5_LINK_CAP__LINK_SPEED_MASK 0xf
#define D3F5_LINK_CAP__LINK_SPEED__SHIFT 0x0
#define D3F5_LINK_CAP__LINK_WIDTH_MASK 0x3f0
#define D3F5_LINK_CAP__LINK_WIDTH__SHIFT 0x4
#define D3F5_LINK_CAP__PM_SUPPORT_MASK 0xc00
#define D3F5_LINK_CAP__PM_SUPPORT__SHIFT 0xa
#define D3F5_LINK_CAP__L0S_EXIT_LATENCY_MASK 0x7000
#define D3F5_LINK_CAP__L0S_EXIT_LATENCY__SHIFT 0xc
#define D3F5_LINK_CAP__L1_EXIT_LATENCY_MASK 0x38000
#define D3F5_LINK_CAP__L1_EXIT_LATENCY__SHIFT 0xf
#define D3F5_LINK_CAP__CLOCK_POWER_MANAGEMENT_MASK 0x40000
#define D3F5_LINK_CAP__CLOCK_POWER_MANAGEMENT__SHIFT 0x12
#define D3F5_LINK_CAP__SURPRISE_DOWN_ERR_REPORTING_MASK 0x80000
#define D3F5_LINK_CAP__SURPRISE_DOWN_ERR_REPORTING__SHIFT 0x13
#define D3F5_LINK_CAP__DL_ACTIVE_REPORTING_CAPABLE_MASK 0x100000
#define D3F5_LINK_CAP__DL_ACTIVE_REPORTING_CAPABLE__SHIFT 0x14
#define D3F5_LINK_CAP__LINK_BW_NOTIFICATION_CAP_MASK 0x200000
#define D3F5_LINK_CAP__LINK_BW_NOTIFICATION_CAP__SHIFT 0x15
#define D3F5_LINK_CAP__ASPM_OPTIONALITY_COMPLIANCE_MASK 0x400000
#define D3F5_LINK_CAP__ASPM_OPTIONALITY_COMPLIANCE__SHIFT 0x16
#define D3F5_LINK_CAP__PORT_NUMBER_MASK 0xff000000
#define D3F5_LINK_CAP__PORT_NUMBER__SHIFT 0x18
#define D3F5_LINK_CNTL__PM_CONTROL_MASK 0x3
#define D3F5_LINK_CNTL__PM_CONTROL__SHIFT 0x0
#define D3F5_LINK_CNTL__READ_CPL_BOUNDARY_MASK 0x8
#define D3F5_LINK_CNTL__READ_CPL_BOUNDARY__SHIFT 0x3
#define D3F5_LINK_CNTL__LINK_DIS_MASK 0x10
#define D3F5_LINK_CNTL__LINK_DIS__SHIFT 0x4
#define D3F5_LINK_CNTL__RETRAIN_LINK_MASK 0x20
#define D3F5_LINK_CNTL__RETRAIN_LINK__SHIFT 0x5
#define D3F5_LINK_CNTL__COMMON_CLOCK_CFG_MASK 0x40
#define D3F5_LINK_CNTL__COMMON_CLOCK_CFG__SHIFT 0x6
#define D3F5_LINK_CNTL__EXTENDED_SYNC_MASK 0x80
#define D3F5_LINK_CNTL__EXTENDED_SYNC__SHIFT 0x7
#define D3F5_LINK_CNTL__CLOCK_POWER_MANAGEMENT_EN_MASK 0x100
#define D3F5_LINK_CNTL__CLOCK_POWER_MANAGEMENT_EN__SHIFT 0x8
#define D3F5_LINK_CNTL__HW_AUTONOMOUS_WIDTH_DISABLE_MASK 0x200
#define D3F5_LINK_CNTL__HW_AUTONOMOUS_WIDTH_DISABLE__SHIFT 0x9
#define D3F5_LINK_CNTL__LINK_BW_MANAGEMENT_INT_EN_MASK 0x400
#define D3F5_LINK_CNTL__LINK_BW_MANAGEMENT_INT_EN__SHIFT 0xa
#define D3F5_LINK_CNTL__LINK_AUTONOMOUS_BW_INT_EN_MASK 0x800
#define D3F5_LINK_CNTL__LINK_AUTONOMOUS_BW_INT_EN__SHIFT 0xb
#define D3F5_LINK_STATUS__CURRENT_LINK_SPEED_MASK 0xf0000
#define D3F5_LINK_STATUS__CURRENT_LINK_SPEED__SHIFT 0x10
#define D3F5_LINK_STATUS__NEGOTIATED_LINK_WIDTH_MASK 0x3f00000
#define D3F5_LINK_STATUS__NEGOTIATED_LINK_WIDTH__SHIFT 0x14
#define D3F5_LINK_STATUS__LINK_TRAINING_MASK 0x8000000
#define D3F5_LINK_STATUS__LINK_TRAINING__SHIFT 0x1b
#define D3F5_LINK_STATUS__SLOT_CLOCK_CFG_MASK 0x10000000
#define D3F5_LINK_STATUS__SLOT_CLOCK_CFG__SHIFT 0x1c
#define D3F5_LINK_STATUS__DL_ACTIVE_MASK 0x20000000
#define D3F5_LINK_STATUS__DL_ACTIVE__SHIFT 0x1d
#define D3F5_LINK_STATUS__LINK_BW_MANAGEMENT_STATUS_MASK 0x40000000
#define D3F5_LINK_STATUS__LINK_BW_MANAGEMENT_STATUS__SHIFT 0x1e
#define D3F5_LINK_STATUS__LINK_AUTONOMOUS_BW_STATUS_MASK 0x80000000
#define D3F5_LINK_STATUS__LINK_AUTONOMOUS_BW_STATUS__SHIFT 0x1f
#define D3F5_SLOT_CAP__ATTN_BUTTON_PRESENT_MASK 0x1
#define D3F5_SLOT_CAP__ATTN_BUTTON_PRESENT__SHIFT 0x0
#define D3F5_SLOT_CAP__PWR_CONTROLLER_PRESENT_MASK 0x2
#define D3F5_SLOT_CAP__PWR_CONTROLLER_PRESENT__SHIFT 0x1
#define D3F5_SLOT_CAP__MRL_SENSOR_PRESENT_MASK 0x4
#define D3F5_SLOT_CAP__MRL_SENSOR_PRESENT__SHIFT 0x2
#define D3F5_SLOT_CAP__ATTN_INDICATOR_PRESENT_MASK 0x8
#define D3F5_SLOT_CAP__ATTN_INDICATOR_PRESENT__SHIFT 0x3
#define D3F5_SLOT_CAP__PWR_INDICATOR_PRESENT_MASK 0x10
#define D3F5_SLOT_CAP__PWR_INDICATOR_PRESENT__SHIFT 0x4
#define D3F5_SLOT_CAP__HOTPLUG_SURPRISE_MASK 0x20
#define D3F5_SLOT_CAP__HOTPLUG_SURPRISE__SHIFT 0x5
#define D3F5_SLOT_CAP__HOTPLUG_CAPABLE_MASK 0x40
#define D3F5_SLOT_CAP__HOTPLUG_CAPABLE__SHIFT 0x6
#define D3F5_SLOT_CAP__SLOT_PWR_LIMIT_VALUE_MASK 0x7f80
#define D3F5_SLOT_CAP__SLOT_PWR_LIMIT_VALUE__SHIFT 0x7
#define D3F5_SLOT_CAP__SLOT_PWR_LIMIT_SCALE_MASK 0x18000
#define D3F5_SLOT_CAP__SLOT_PWR_LIMIT_SCALE__SHIFT 0xf
#define D3F5_SLOT_CAP__ELECTROMECH_INTERLOCK_PRESENT_MASK 0x20000
#define D3F5_SLOT_CAP__ELECTROMECH_INTERLOCK_PRESENT__SHIFT 0x11
#define D3F5_SLOT_CAP__NO_COMMAND_COMPLETED_SUPPORTED_MASK 0x40000
#define D3F5_SLOT_CAP__NO_COMMAND_COMPLETED_SUPPORTED__SHIFT 0x12
#define D3F5_SLOT_CAP__PHYSICAL_SLOT_NUM_MASK 0xfff80000
#define D3F5_SLOT_CAP__PHYSICAL_SLOT_NUM__SHIFT 0x13
#define D3F5_SLOT_CNTL__ATTN_BUTTON_PRESSED_EN_MASK 0x1
#define D3F5_SLOT_CNTL__ATTN_BUTTON_PRESSED_EN__SHIFT 0x0
#define D3F5_SLOT_CNTL__PWR_FAULT_DETECTED_EN_MASK 0x2
#define D3F5_SLOT_CNTL__PWR_FAULT_DETECTED_EN__SHIFT 0x1
#define D3F5_SLOT_CNTL__MRL_SENSOR_CHANGED_EN_MASK 0x4
#define D3F5_SLOT_CNTL__MRL_SENSOR_CHANGED_EN__SHIFT 0x2
#define D3F5_SLOT_CNTL__PRESENCE_DETECT_CHANGED_EN_MASK 0x8
#define D3F5_SLOT_CNTL__PRESENCE_DETECT_CHANGED_EN__SHIFT 0x3
#define D3F5_SLOT_CNTL__COMMAND_COMPLETED_INTR_EN_MASK 0x10
#define D3F5_SLOT_CNTL__COMMAND_COMPLETED_INTR_EN__SHIFT 0x4
#define D3F5_SLOT_CNTL__HOTPLUG_INTR_EN_MASK 0x20
#define D3F5_SLOT_CNTL__HOTPLUG_INTR_EN__SHIFT 0x5
#define D3F5_SLOT_CNTL__ATTN_INDICATOR_CNTL_MASK 0xc0
#define D3F5_SLOT_CNTL__ATTN_INDICATOR_CNTL__SHIFT 0x6
#define D3F5_SLOT_CNTL__PWR_INDICATOR_CNTL_MASK 0x300
#define D3F5_SLOT_CNTL__PWR_INDICATOR_CNTL__SHIFT 0x8
#define D3F5_SLOT_CNTL__PWR_CONTROLLER_CNTL_MASK 0x400
#define D3F5_SLOT_CNTL__PWR_CONTROLLER_CNTL__SHIFT 0xa
#define D3F5_SLOT_CNTL__ELECTROMECH_INTERLOCK_CNTL_MASK 0x800
#define D3F5_SLOT_CNTL__ELECTROMECH_INTERLOCK_CNTL__SHIFT 0xb
#define D3F5_SLOT_CNTL__DL_STATE_CHANGED_EN_MASK 0x1000
#define D3F5_SLOT_CNTL__DL_STATE_CHANGED_EN__SHIFT 0xc
#define D3F5_SLOT_STATUS__ATTN_BUTTON_PRESSED_MASK 0x10000
#define D3F5_SLOT_STATUS__ATTN_BUTTON_PRESSED__SHIFT 0x10
#define D3F5_SLOT_STATUS__PWR_FAULT_DETECTED_MASK 0x20000
#define D3F5_SLOT_STATUS__PWR_FAULT_DETECTED__SHIFT 0x11
#define D3F5_SLOT_STATUS__MRL_SENSOR_CHANGED_MASK 0x40000
#define D3F5_SLOT_STATUS__MRL_SENSOR_CHANGED__SHIFT 0x12
#define D3F5_SLOT_STATUS__PRESENCE_DETECT_CHANGED_MASK 0x80000
#define D3F5_SLOT_STATUS__PRESENCE_DETECT_CHANGED__SHIFT 0x13
#define D3F5_SLOT_STATUS__COMMAND_COMPLETED_MASK 0x100000
#define D3F5_SLOT_STATUS__COMMAND_COMPLETED__SHIFT 0x14
#define D3F5_SLOT_STATUS_