_MASK 0x20
#define D2F3_LINK_CNTL2__HW_AUTONOMOUS_SPEED_DISABLE__SHIFT 0x5
#define D2F3_LINK_CNTL2__SELECTABLE_DEEMPHASIS_MASK 0x40
#define D2F3_LINK_CNTL2__SELECTABLE_DEEMPHASIS__SHIFT 0x6
#define D2F3_LINK_CNTL2__XMIT_MARGIN_MASK 0x380
#define D2F3_LINK_CNTL2__XMIT_MARGIN__SHIFT 0x7
#define D2F3_LINK_CNTL2__ENTER_MOD_COMPLIANCE_MASK 0x400
#define D2F3_LINK_CNTL2__ENTER_MOD_COMPLIANCE__SHIFT 0xa
#define D2F3_LINK_CNTL2__COMPLIANCE_SOS_MASK 0x800
#define D2F3_LINK_CNTL2__COMPLIANCE_SOS__SHIFT 0xb
#define D2F3_LINK_CNTL2__COMPLIANCE_DEEMPHASIS_MASK 0xf000
#define D2F3_LINK_CNTL2__COMPLIANCE_DEEMPHASIS__SHIFT 0xc
#define D2F3_LINK_STATUS2__CUR_DEEMPHASIS_LEVEL_MASK 0x10000
#define D2F3_LINK_STATUS2__CUR_DEEMPHASIS_LEVEL__SHIFT 0x10
#define D2F3_LINK_STATUS2__EQUALIZATION_COMPLETE_MASK 0x20000
#define D2F3_LINK_STATUS2__EQUALIZATION_COMPLETE__SHIFT 0x11
#define D2F3_LINK_STATUS2__EQUALIZATION_PHASE1_SUCCESS_MASK 0x40000
#define D2F3_LINK_STATUS2__EQUALIZATION_PHASE1_SUCCESS__SHIFT 0x12
#define D2F3_LINK_STATUS2__EQUALIZATION_PHASE2_SUCCESS_MASK 0x80000
#define D2F3_LINK_STATUS2__EQUALIZATION_PHASE2_SUCCESS__SHIFT 0x13
#define D2F3_LINK_STATUS2__EQUALIZATION_PHASE3_SUCCESS_MASK 0x100000
#define D2F3_LINK_STATUS2__EQUALIZATION_PHASE3_SUCCESS__SHIFT 0x14
#define D2F3_LINK_STATUS2__LINK_EQUALIZATION_REQUEST_MASK 0x200000
#define D2F3_LINK_STATUS2__LINK_EQUALIZATION_REQUEST__SHIFT 0x15
#define D2F3_SLOT_CAP2__RESERVED_MASK 0xffffffff
#define D2F3_SLOT_CAP2__RESERVED__SHIFT 0x0
#define D2F3_SLOT_CNTL2__RESERVED_MASK 0xffff
#define D2F3_SLOT_CNTL2__RESERVED__SHIFT 0x0
#define D2F3_SLOT_STATUS2__RESERVED_MASK 0xffff0000
#define D2F3_SLOT_STATUS2__RESERVED__SHIFT 0x10
#define D2F3_MSI_CAP_LIST__CAP_ID_MASK 0xff
#define D2F3_MSI_CAP_LIST__CAP_ID__SHIFT 0x0
#define D2F3_MSI_CAP_LIST__NEXT_PTR_MASK 0xff00
#define D2F3_MSI_CAP_LIST__NEXT_PTR__SHIFT 0x8
#define D2F3_MSI_MSG_CNTL__MSI_EN_MASK 0x10000
#define D2F3_MSI_MSG_CNTL__MSI_EN__SHIFT 0x10
#define D2F3_MSI_MSG_CNTL__MSI_MULTI_CAP_MASK 0xe0000
#define D2F3_MSI_MSG_CNTL__MSI_MULTI_CAP__SHIFT 0x11
#define D2F3_MSI_MSG_CNTL__MSI_MULTI_EN_MASK 0x700000
#define D2F3_MSI_MSG_CNTL__MSI_MULTI_EN__SHIFT 0x14
#define D2F3_MSI_MSG_CNTL__MSI_64BIT_MASK 0x800000
#define D2F3_MSI_MSG_CNTL__MSI_64BIT__SHIFT 0x17
#define D2F3_MSI_MSG_CNTL__MSI_PERVECTOR_MASKING_CAP_MASK 0x1000000
#define D2F3_MSI_MSG_CNTL__MSI_PERVECTOR_MASKING_CAP__SHIFT 0x18
#define D2F3_MSI_MSG_ADDR_LO__MSI_MSG_ADDR_LO_MASK 0xfffffffc
#define D2F3_MSI_MSG_ADDR_LO__MSI_MSG_ADDR_LO__SHIFT 0x2
#define D2F3_MSI_MSG_ADDR_HI__MSI_MSG_ADDR_HI_MASK 0xffffffff
#define D2F3_MSI_MSG_ADDR_HI__MSI_MSG_ADDR_HI__SHIFT 0x0
#define D2F3_MSI_MSG_DATA_64__MSI_DATA_64_MASK 0xffff
#define D2F3_MSI_MSG_DATA_64__MSI_DATA_64__SHIFT 0x0
#define D2F3_MSI_MSG_DATA__MSI_DATA_MASK 0xffff
#define D2F3_MSI_MSG_DATA__MSI_DATA__SHIFT 0x0
#define D2F3_SSID_CAP_LIST__CAP_ID_MASK 0xff
#define D2F3_SSID_CAP_LIST__CAP_ID__SHIFT 0x0
#define D2F3_SSID_CAP_LIST__NEXT_PTR_MASK 0xff00
#define D2F3_SSID_CAP_LIST__NEXT_PTR__SHIFT 0x8
#define D2F3_SSID_CAP__SUBSYSTEM_VENDOR_ID_MASK 0xffff
#define D2F3_SSID_CAP__SUBSYSTEM_VENDOR_ID__SHIFT 0x0
#define D2F3_SSID_CAP__SUBSYSTEM_ID_MASK 0xffff0000
#define D2F3_SSID_CAP__SUBSYSTEM_ID__SHIFT 0x10
#define D2F3_MSI_MAP_CAP_LIST__CAP_ID_MASK 0xff
#define D2F3_MSI_MAP_CAP_LIST__CAP_ID__SHIFT 0x0
#define D2F3_MSI_MAP_CAP_LIST__NEXT_PTR_MASK 0xff00
#define D2F3_MSI_MAP_CAP_LIST__NEXT_PTR__SHIFT 0x8
#define D2F3_MSI_MAP_CAP__EN_MASK 0x10000
#define D2F3_MSI_MAP_CAP__EN__SHIFT 0x10
#define D2F3_MSI_MAP_CAP__FIXD_MASK 0x20000
#define D2F3_MSI_MAP_CAP__FIXD__SHIFT 0x11
#define D2F3_MSI_MAP_CAP__CAP_TYPE_MASK 0xf8000000
#define D2F3_MSI_MAP_CAP__CAP_TYPE__SHIFT 0x1b
#define D2F3_MSI_MAP_ADDR_LO__MSI_MAP_ADDR_LO_MASK 0xfff00000
#define D2F3_MSI_MAP_ADDR_LO__MSI_MAP_ADDR_LO__SHIFT 0x14
#define D2F3_MSI_MAP_ADDR_HI__MSI_MAP_ADDR_HI_MASK 0xffffffff
#define D2F3_MSI_MAP_AD