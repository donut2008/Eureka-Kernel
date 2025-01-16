H_CAP_LIST__CAP_VER_MASK 0xf0000
#define PCIE_DPA_ENH_CAP_LIST__CAP_VER__SHIFT 0x10
#define PCIE_DPA_ENH_CAP_LIST__NEXT_PTR_MASK 0xfff00000
#define PCIE_DPA_ENH_CAP_LIST__NEXT_PTR__SHIFT 0x14
#define PCIE_DPA_CAP__SUBSTATE_MAX_MASK 0x1f
#define PCIE_DPA_CAP__SUBSTATE_MAX__SHIFT 0x0
#define PCIE_DPA_CAP__TRANS_LAT_UNIT_MASK 0x300
#define PCIE_DPA_CAP__TRANS_LAT_UNIT__SHIFT 0x8
#define PCIE_DPA_CAP__PWR_ALLOC_SCALE_MASK 0x3000
#define PCIE_DPA_CAP__PWR_ALLOC_SCALE__SHIFT 0xc
#define PCIE_DPA_CAP__TRANS_LAT_VAL_0_MASK 0xff0000
#define PCIE_DPA_CAP__TRANS_LAT_VAL_0__SHIFT 0x10
#define PCIE_DPA_CAP__TRANS_LAT_VAL_1_MASK 0xff000000
#define PCIE_DPA_CAP__TRANS_LAT_VAL_1__SHIFT 0x18
#define PCIE_DPA_LATENCY_INDICATOR__TRANS_LAT_INDICATOR_BITS_MASK 0xff
#define PCIE_DPA_LATENCY_INDICATOR__TRANS_LAT_INDICATOR_BITS__SHIFT 0x0
#define PCIE_DPA_STATUS__SUBSTATE_STATUS_MASK 0x1f
#define PCIE_DPA_STATUS__SUBSTATE_STATUS__SHIFT 0x0
#define PCIE_DPA_STATUS__SUBSTATE_CNTL_ENABLED_MASK 0x100
#define PCIE_DPA_STATUS__SUBSTATE_CNTL_ENABLED__SHIFT 0x8
#define PCIE_DPA_CNTL__SUBSTATE_CNTL_MASK 0x1f
#define PCIE_DPA_CNTL__SUBSTATE_CNTL__SHIFT 0x0
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_0__SUBSTATE_PWR_ALLOC_MASK 0xff
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_0__SUBSTATE_PWR_ALLOC__SHIFT 0x0
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_1__SUBSTATE_PWR_ALLOC_MASK 0xff
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_1__SUBSTATE_PWR_ALLOC__SHIFT 0x0
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_2__SUBSTATE_PWR_ALLOC_MASK 0xff
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_2__SUBSTATE_PWR_ALLOC__SHIFT 0x0
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_3__SUBSTATE_PWR_ALLOC_MASK 0xff
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_3__SUBSTATE_PWR_ALLOC__SHIFT 0x0
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_4__SUBSTATE_PWR_ALLOC_MASK 0xff
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_4__SUBSTATE_PWR_ALLOC__SHIFT 0x0
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_5__SUBSTATE_PWR_ALLOC_MASK 0xff
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_5__SUBSTATE_PWR_ALLOC__SHIFT 0x0
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_6__SUBSTATE_PWR_ALLOC_MASK 0xff
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_6__SUBSTATE_PWR_ALLOC__SHIFT 0x0
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_7__SUBSTATE_PWR_ALLOC_MASK 0xff
#define PCIE_DPA_SUBSTATE_PWR_ALLOC_7__SUBSTATE_PWR_ALLOC__SHIFT 0x0
#define PCIE_SECONDARY_ENH_CAP_LIST__CAP_ID_MASK 0xffff
#define PCIE_SECONDARY_ENH_CAP_LIST__CAP_ID__SHIFT 0x0
#define PCIE_SECONDARY_ENH_CAP_LIST__CAP_VER_MASK 0xf0000
#define PCIE_SECONDARY_ENH_CAP_LIST__CAP_VER__SHIFT 0x10
#define PCIE_SECONDARY_ENH_CAP_LIST__NEXT_PTR_MASK 0xfff00000
#define PCIE_SECONDARY_ENH_CAP_LIST__NEXT_PTR__SHIFT 0x14
#define PCIE_LINK_CNTL3__PERFORM_EQUALIZATION_MASK 0x1
#define PCIE_LINK_CNTL3__PERFORM_EQUALIZATION__SHIFT 0x0
#define PCIE_LINK_CNTL3__LINK_EQUALIZATION_REQ_INT_EN_MASK 0x2
#define PCIE_LINK_CNTL3__LINK_EQUALIZATION_REQ_INT_EN__SHIFT 0x1
#define PCIE_LINK_CNTL3__RESERVED_MASK 0xfffffffc
#define PCIE_LINK_CNTL3__RESERVED__SHIFT 0x2
#define PCIE_LANE_ERROR_STATUS__LANE_ERROR_STATUS_BITS_MASK 0xffff
#define PCIE_LANE_ERROR_STATUS__LANE_ERROR_STATUS_BITS__SHIFT 0x0
#define PCIE_LANE_ERROR_STATUS__RESERVED_MASK 0xffff0000
#define PCIE_LANE_ERROR_STATUS__RESERVED__SHIFT 0x10
#define PCIE_LANE_0_EQUALIZATION_CNTL__DOWNSTREAM_PORT_TX_PRESET_MASK 0xf
#define PCIE_LANE_0_EQUALIZATION_CNTL__DOWNSTREAM_PORT_TX_PRESET__SHIFT 0x0
#define PCIE_LANE_0_EQUALIZATION_CNTL__DOWNSTREAM_PORT_RX_PRESET_HINT_MASK 0x70
#define PCIE_LANE_0_EQUALIZATION_CNTL__DOWNSTREAM_PORT_RX_PRESET_HINT__SHIFT 0x4
#define PCIE_LANE_0_EQUALIZATION_CNTL__UPSTREAM_PORT_TX_PRESET_MASK 0xf00
#define PCIE_LANE_0_EQUALIZATION_CNTL__UPSTREAM_PORT_TX_PRESET__SHIFT 0x8
#define PCIE_LANE_0_EQUALIZATION_CNTL__UPSTREAM_PORT_RX_PRESET_HINT_MASK 0x7000
#define PCIE_LANE_0_EQUALIZATION_CNTL__UPSTREAM_PORT_RX_PRESET_HINT__SHIFT 0xc
#define PCIE_LANE_0_EQUALIZATION_CNTL__RESERVED_MASK 0x8000
#define PCIE_LANE_0_EQUALIZATION_CNTL__RESERVED__SHIFT 0xf
#define PCIE_LANE_1_EQUALIZATION_CNTL__DOWNSTREAM_PORT_TX_PRESET_MASK 0xf
#define PCIE_LANE_1_EQUALIZATION_CNTL__DOWNSTREAM_PORT_TX_PRESET__SHIFT 0x0
#define PCIE_LANE_1_EQUALIZATION_CNTL__DOWNSTREAM_PORT_RX_PRESET_HINT_MASK 0x70
#define PCIE_LANE_1_EQUALIZATION_CNTL__DOWNSTREAM_PORT_RX_PRESET_HINT__SHIFT 0x4
#define PCIE_LANE_1_EQUALIZATION_CNTL__UPSTREAM_PORT_TX_PRESET_MASK 0xf00
#define PCIE_LANE_1_EQUALIZATION_CNTL__UPSTREAM_PORT_TX_PRESET__SHIFT 0x8
#define PCIE_LANE_1_EQUALIZATION_CNTL__UPSTREAM_PORT_RX_PRESET_HINT_MASK 0x7000
#define PCIE_LANE_1_EQUALIZATION_CNTL__UPSTREAM_PORT_RX_PRESET_HINT__SHIFT 0xc
#define PCIE_LANE_1_EQUALIZATION_CNTL__RESERVED_MASK 0x8000
#define PCIE_LANE_1_EQUALIZATION_CNTL__RESERVED__SHIFT 0xf
#define PCIE_LANE_2_EQUALIZATION_CNTL__DOWNSTREAM_PORT_TX_PRESET_MASK 0xf
#define PCIE_LANE_2_EQUALIZATION_CNTL__DOWNSTREAM_PORT_TX_PRESET__SHIFT 0x0
#define PCIE_LANE_2_EQUALIZATION_CNTL__DOWNSTREAM_PORT_RX_PRESET_HINT_MASK 0x70
#define PCIE_LANE_2_EQUALIZATION_CNTL__DOWNSTREAM_PORT_RX_PRESET_HINT__SHIFT 0x4
#define PCIE_LANE_2_EQUALIZATION_CNTL__UPSTREAM_PORT_TX_PRESET_MASK 0xf00
#define PCIE_LANE_2_EQUALIZATION_CNTL__UPSTREAM_PORT_TX_PRESET__SHIFT 0x8
#define PCIE_LANE_2_EQUALIZATION_CNTL__UPSTREAM_PORT_RX_PRESET_HINT_MASK 0x7000
#define PCIE_LANE_2_EQUALIZATION_CNTL__UPSTREAM_PORT_RX_PRESET_HINT__SHIFT 0xc
#define PCIE_LANE_2_EQUALIZATION_CNTL__RESERVED_MASK 0x8000
#define PCIE_LANE_2_EQUALIZATION_CNTL__RESERVED__SHIFT 0xf
#define PCIE_LANE_3_EQUALIZATION_CNTL__DOWNSTREAM_PORT_TX_PRESET_MASK 0xf
#define PCIE_LANE_3_EQUALIZATION_CNTL__DOWNSTREAM_PORT_TX_PRESET__SHIFT 0x0
#define PCIE_LANE_3_EQUALIZATION_CNTL__DOWNS