              ucNumberOfObjects;
  UCHAR               ucPadding[3];
  ATOM_OBJECT         asObjects[1];
}ATOM_OBJECT_TABLE;

typedef struct _ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT         //usSrcDstTableOffset pointing to this structure
{
  UCHAR               ucNumberOfSrc;
  USHORT              usSrcObjectID[1];
  UCHAR               ucNumberOfDst;
  USHORT              usDstObjectID[1];
}ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT;


//Two definitions below are for OPM on MXM module designs

#define EXT_HPDPIN_LUTINDEX_0                   0
#define EXT_HPDPIN_LUTINDEX_1                   1
#define EXT_HPDPIN_LUTINDEX_2                   2
#define EXT_HPDPIN_LUTINDEX_3                   3
#define EXT_HPDPIN_LUTINDEX_4                   4
#define EXT_HPDPIN_LUTINDEX_5                   5
#define EXT_HPDPIN_LUTINDEX_6                   6
#define EXT_HPDPIN_LUTINDEX_7                   7
#define MAX_NUMBER_OF_EXT_HPDPIN_LUT_ENTRIES   (EXT_HPDPIN_LUTINDEX_7+1)

#define EXT_AUXDDC_LUTINDEX_0                   0
#define EXT_AUXDDC_LUTINDEX_1                   1
#define EXT_AUXDDC_LUTINDEX_2                   2
#define EXT_AUXDDC_LUTINDEX_3                   3
#define EXT_AUXDDC_LUTINDEX_4                   4
#define EXT_AUXDDC_LUTINDEX_5                   5
#define EXT_AUXDDC_LUTINDEX_6                   6
#define EXT_AUXDDC_LUTINDEX_7                   7
#define MAX_NUMBER_OF_EXT_AUXDDC_LUT_ENTRIES   (EXT_AUXDDC_LUTINDEX_7+1)

//ucChannelMapping are defined as following
//for DP connector, eDP, DP to VGA/LVDS
//Bit[1:0]: Define which pin connect to DP connector DP_Lane0, =0: source from GPU pin TX0, =1: from GPU pin TX1, =2: from GPU pin TX2, =3 from GPU pin TX3
//Bit[3:2]: Define which pin connect to DP connector DP_Lane1, =0: source from GPU pin TX0, =1: from GPU pin TX1, =2: from GPU pin TX2, =3 from GPU pin TX3
//Bit[5:4]: Define which pin connect to DP connector DP_Lane2, =0: source from GPU pin TX0, =1: from GPU pin TX1, =2: from GPU pin TX2, =3 from GPU pin TX3
//Bit[7:6]: Define which pin connect to DP connector DP_Lane3, =0: source from GPU pin TX0, =1: from GPU pin TX1, =2: from GPU pin TX2, =3 from GPU pin TX3
typedef struct _ATOM_DP_CONN_CHANNEL_MAPPING
{
#if ATOM_BIG_ENDIAN
  UCHAR ucDP_Lane3_Source:2;
  UCHAR ucDP_Lane2_Source:2;
  UCHAR ucDP_Lane1_Source:2;
  UCHAR ucDP_Lane0_Source:2;
#else
  UCHAR ucDP_Lane0_Source:2;
  UCHAR ucDP_Lane1_Source:2;
  UCHAR ucDP_Lane2_Source:2;
  UCHAR ucDP_Lane3_Source:2;
#endif
}ATOM_DP_CONN_CHANNEL_MAPPING;

//for DVI/HDMI, in dual link case, both links have to have same mapping.
//Bit[1:0]: Define which pin connect to DVI connector data Lane2, =0: source from GPU pin TX0, =1: from GPU pin TX1, =2: from GPU pin TX2, =3 from GPU pin TX3
//Bit[3:2]: Define which pin connect to DVI connector data Lane1, =0: source from GPU pin TX0, =1: from GPU pin TX1, =2: from GPU pin TX2, =3 from GPU pin TX3
//Bit[5:4]: Define which pin connect to DVI connector data Lane0, =0: source from GPU pin TX0, =1: from GPU pin TX1, =2: from GPU pin TX2, =3 from GPU pin TX3
//Bit[7:6]: Define which pin connect to DVI connector clock lane, =0: source from GPU pin TX0, =1: from GPU pin TX1, =2: from GPU pin TX2, =3 from GPU pin TX3
typedef struct _ATOM_DVI_CONN_CHANNEL_MAPPING
{
#if ATOM_BIG_ENDIAN
  UCHAR ucDVI_CLK_Source:2;
  UCHAR ucDVI_DATA0_Source:2;
  UCHAR ucDVI_DATA1_Source:2;
  UCHAR ucDVI_DATA2_Source:2;
#else
  UCHAR ucDVI_DATA2_Source:2;
  UCHAR ucDVI_DATA1_Source:2;
  UCHAR ucDVI_DATA0_Source:2;
  UCHAR ucDVI_CLK_Source:2;
#endif
}ATOM_DVI_CONN_CHANNEL_MAPPING;

typedef struct _EXT_DISPLAY_PATH
{
  USHORT  usDeviceTag;                    //A bit vector to show what devices are supported
  USHORT  usDeviceACPIEnum;               //16bit device ACPI id.
  USHORT  usDeviceConnector;              //A physical connector for displays to plug in, using object connector definitions
  UCHAR   ucExtAUXDDCLutIndex;            //An index into external AUX/DDC channel LUT
  UCHAR   ucExtHPDPINLutIndex;            //An index into external HPD pin LUT
  USHORT  usExtEncoderObjId;              //external encoder object id
  union{
    UCHAR   ucChannelMapping;                  // if ucChannelMapping=0, using default one to one mapping
    ATOM_DP_CONN_CHANNEL_MAPPING asDPMapping;
    ATOM_DVI_CONN_CHANNEL_MAPPING asDVIMapping;
  };
  UCHAR   ucChPNInvert;                   // bit vector for up to 8 lanes, =0: P and N is not invert, =1 P and N is inverted
  USHORT  usCaps;
  USHORT  usReserved;
}EXT_DISPLAY_PATH;

#define NUMBER_OF_UCHAR_FOR_GUID          16
#define MAX_NUMBER_OF_EXT_DISPLAY_PATH    7

//usCaps
#define  EXT_DISPLAY_PATH_CAPS__HBR2_DISABLE               0x01
#define  EXT_DISPLAY_PATH_CAPS__DP_FIXED_VS_EN             0x02
#define  EXT_DISPLAY_PATH_CAPS__HDMI20_PI3EQX1204          0x04
#define  EXT_DISPLAY_PATH_CAPS__HDMI20_TISN65DP159RSBT     0x08

typedef  struct _ATOM_EXTERNAL_DISPLAY_CONNECTION_INFO
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  UCHAR                    ucGuid [NUMBER_OF_UCHAR_FOR_GUID];     // a GUID is a 16 byte long string
  EXT_DISPLAY_PATH         sPath[MAX_NUMBER_OF_EXT_DISPLAY_PATH]; // total of fixed 7 entries.
  UCHAR                    ucChecksum;                            // a simple Checksum of the sum of whole structure equal to 0x0.
  UCHAR                    uc3DStereoPinId;                       // use for eDP panel
  UCHAR                    ucRemoteDisplayConfig;
  UCHAR                    uceDPToLVDSRxId;
  UCHAR                    ucFixDPVoltageSwing;                   // usCaps[1]=1, this indicate DP_LANE_SET value
  UCHAR                    Reserved[3];                           // for potential expansion
}ATOM_EXTERNAL_DISPLAY_CONNECTION_INFO;

//Related definitions, all records are differnt but they have a commond header
typedef struct _ATOM_COMMON_RECORD_HEADER
{
  UCHAR               ucRecordType;                      //An emun to indicate the record type
  UCHAR               ucRecordSize;                      //The size of the whole record in byte
}ATOM_COMMON_RECORD_HEADER;


#define ATOM_I2C_RECORD_TYPE                           1
#define ATOM_HPD_INT_RECORD_TYPE                       2
#define ATOM_OUTPUT_PROTECTION_RECORD_TYPE             3
#define ATOM_CONNECTOR_DEVICE_TAG_RECORD_TYPE          4
#define ATOM_CONNECTOR_DVI_EXT_INPUT_RECORD_TYPE       5 //Obsolete, switch to use GPIO_CNTL_RECORD_TYPE
#define ATOM_ENCODER_FPGA_CONTROL_RECORD_TYPE          6 //Obsolete, switch to use GPIO_CNTL_RECORD_TYPE
#define ATOM_CONNECTOR_CVTV_SHARE_DIN_RECORD_TYPE      7
#define ATOM_JTAG_RECORD_TYPE                          8 //Obsolete, switch to use GPIO_CNTL_RECORD_TYPE
#define ATOM_OBJECT_GPIO_CNTL_RECORD_TYPE              9
#define ATOM_ENCODER_DVO_CF_RECORD_TYPE                10
#define ATOM_CONNECTOR_CF_RECORD_TYPE                  11
#define ATOM_CONNECTOR_HARDCODE_DTD_RECORD_TYPE        12
#define ATOM_CONNECTOR_PCIE_SUBCONNECTOR_RECORD_TYPE   13
#define ATOM_ROUTER_DDC_PATH_SELECT_RECORD_TYPE        14
#define ATOM_ROUTER_DATA_CLOCK_PATH_SELECT_RECORD_TYPE 15
#define ATOM_CONNECTOR_HPDPIN_LUT_RECORD_TYPE          16 //This is for the case when connectors are not known to object table
#define ATOM_CONNECTOR_AUXDDC_LUT_RECORD_TYPE          17 //This is for the case when connectors are not known to object table
#define ATOM_OBJECT_LINK_RECORD_TYPE                   18 //Once this record is present under one object, it indicats the oobject is linked to another obj described by the record
#define ATOM_CONNECTOR_REMOTE_CAP_RECORD_TYPE          19
#define ATOM_ENCODER_CAP_RECORD_TYPE                   20
#define ATOM_BRACKET_LAYOUT_RECORD_TYPE                21


//Must be updated when new record type is added,equal to that record definition!
#define ATOM_MAX_OBJECT_RECORD_NUMBER             ATOM_ENCODER_CAP_RECORD_TYPE

typedef struct  _ATOM_I2C_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  ATOM_I2C_ID_CONFIG          sucI2cId;
  UCHAR                       ucI2CAddr;              //The slave address, it's 0 when the record is attached to connector for DDC
}ATOM_I2C_RECORD;

typedef struct  _ATOM_HPD_INT_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  UCHAR                       ucHPDIntGPIOID;         //Corresponding block in GPIO_PIN_INFO table gives the pin info
  UCHAR                       ucPlugged_PinState;
}ATOM_HPD_INT_RECORD;


typedef struct  _ATOM_OUTPUT_PROTECTION_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  UCHAR                       ucProtectionFlag;
  UCHAR                       ucReserved;
}ATOM_OUTPUT_PROTECTION_RECORD;

typedef struct  _ATOM_CONNECTOR_DEVICE_TAG
{
  ULONG                       ulACPIDeviceEnum;       //Reserved for now
  USHORT                      usDeviceID;             //This Id is same as "ATOM_DEVICE_XXX_SUPPORT"
  USHORT                      usPadding;
}ATOM_CONNECTOR_DEVICE_TAG;

typedef struct  _ATOM_CONNECTOR_DEVICE_TAG_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  UCHAR                       ucNumberOfDevice;
  UCHAR                       ucReserved;
  ATOM_CONNECTOR_DEVICE_TAG   asDeviceTag[1];         //This Id is same as "ATOM_DEVICE_XXX_SUPPORT", 1 is only for allocation
}ATOM_CONNECTOR_DEVICE_TAG_RECORD;


typedef struct  _ATOM_CONNECTOR_DVI_EXT_INPUT_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  UCHAR                              ucConfigGPIOID;
  UCHAR                              ucConfigGPIOState;       //Set to 1 when it's active high to enable external flow in
  UCHAR                       ucFlowinGPIPID;
  UCHAR                       ucExtInGPIPID;
}ATOM_CONNECTOR_DVI_EXT_INPUT_RECORD;

typedef struct  _ATOM_ENCODER_FPGA_CONTROL_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  UCHAR                       ucCTL1GPIO_ID;
  UCHAR                       ucCTL1GPIOState;        //Set to 1 when it's active high
  UCHAR                       ucCTL2GPIO_ID;
  UCHAR                       ucCTL2GPIOState;        //Set to 1 when it's active high
  UCHAR                       ucCTL3GPIO_ID;
  UCHAR                       ucCTL3GPIOState;        //Set to 1 when it's active high
  UCHAR                       ucCTLFPGA_IN_ID;
  UCHAR                       ucPadding[3];
}ATOM_ENCODER_FPGA_CONTROL_RECORD;

typedef struct  _ATOM_CONNECTOR_CVTV_SHARE_DIN_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  UCHAR                       ucGPIOID;               //Corresponding block in GPIO_PIN_INFO table gives the pin info
  UCHAR                       ucTVActiveState;        //Indicating when the pin==0 or 1 when TV is connected
}ATOM_CONNECTOR_CVTV_SHARE_DIN_RECORD;

typedef struct  _ATOM_JTAG_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  UCHAR                       ucTMSGPIO_ID;
  UCHAR                       ucTMSGPIOState;         //Set to 1 when it's active high
  UCHAR                       ucTCKGPIO_ID;
  UCHAR                       ucTCKGPIOState;         //Set to 1 when it's active high
  UCHAR                       ucTDOGPIO_ID;
  UCHAR                       ucTDOGPIOState;         //Set to 1 when it's active high
  UCHAR                       ucTDIGPIO_ID;
  UCHAR                       ucTDIGPIOState;         //Set to 1 when it's active high
  UCHAR                       ucPadding[2];
}ATOM_JTAG_RECORD;


//The following generic object gpio pin control record type will replace JTAG_RECORD/FPGA_CONTROL_RECORD/DVI_EXT_INPUT_RECORD above gradually
typedef struct _ATOM_GPIO_PIN_CONTROL_PAIR
{
  UCHAR                       ucGPIOID;               // GPIO_ID, find the corresponding ID in GPIO_LUT table
  UCHAR                       ucGPIO_PinState;        // Pin state showing how to set-up the pin
}ATOM_GPIO_PIN_CONTROL_PAIR;

typedef struct  _ATOM_OBJECT_GPIO_CNTL_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  UCHAR                       ucFlags;                // Future expnadibility
  UCHAR                       ucNumberOfPins;         // Number of GPIO pins used to control the object
  ATOM_GPIO_PIN_CONTROL_PAIR  asGpio[1];              // the real gpio pin pair determined by number of pins ucNumberOfPins
}ATOM_OBJECT_GPIO_CNTL_RECORD;

//Definitions for GPIO pin state
#define GPIO_PIN_TYPE_INPUT             0x00
#define GPIO_PIN_TYPE_OUTPUT            0x10
#define GPIO_PIN_TYPE_HW_CONTROL        0x20

//For GPIO_PIN_TYPE_OUTPUT the following is defined
#define GPIO_PIN_OUTPUT_STATE_MASK      0x01
#define GPIO_PIN_OUTPUT_STATE_SHIFT     0
#define GPIO_PIN_STATE_ACTIVE_LOW       0x0
#define GPIO_PIN_STATE_ACTIVE_HIGH      0x1

// Indexes to GPIO array in GLSync record
// GLSync record is for Frame Lock/Gen Lock feature.
#define ATOM_GPIO_INDEX_GLSYNC_REFCLK    0
#define ATOM_GPIO_INDEX_GLSYNC_HSYNC     1
#define ATOM_GPIO_INDEX_GLSYNC_VSYNC     2
#define ATOM_GPIO_INDEX_GLSYNC_SWAP_REQ  3
#define ATOM_GPIO_INDEX_GLSYNC_SWAP_GNT  4
#define ATOM_GPIO_INDEX_GLSYNC_INTERRUPT 5
#define ATOM_GPIO_INDEX_GLSYNC_V_RESET   6
#define ATOM_GPIO_INDEX_GLSYNC_SWAP_CNTL 7
#define ATOM_GPIO_INDEX_GLSYNC_SWAP_SEL  8
#define ATOM_GPIO_INDEX_GLSYNC_MAX       9

typedef struct  _ATOM_ENCODER_DVO_CF_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  ULONG                       ulStrengthControl;      // DVOA strength control for CF
  UCHAR                       ucPadding[2];
}ATOM_ENCODER_DVO_CF_RECORD;

// Bit maps for ATOM_ENCODER_CAP_RECORD.ucEncoderCap
#define ATOM_ENCODER_CAP_RECORD_HBR2                  0x01         // DP1.2 HBR2 is supported by HW encoder
#define ATOM_ENCODER_CAP_RECORD_HBR2_EN               0x02         // DP1.2 HBR2 setting is qualified and HBR2 can be enabled
#define ATOM_ENCODER_CAP_RECORD_HDMI6Gbps_EN          0x04         // HDMI2.0 6Gbps enable or not.

typedef struct  _ATOM_ENCODER_CAP_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  union {
    USHORT                    usEncoderCap;
    struct {
#if ATOM_BIG_ENDIAN
      USHORT                  usReserved:14;        // Bit1-15 may be defined for other capability in future
      USHORT                  usHBR2En:1;           // Bit1 is for DP1.2 HBR2 enable
      USHORT                  usHBR2Cap:1;          // Bit0 is for DP1.2 HBR2 capability.
#else
      USHORT                  usHBR2Cap:1;          // Bit0 is for DP1.2 HBR2 capability.
      USHORT                  usHBR2En:1;           // Bit1 is for DP1.2 HBR2 enable
      USHORT                  usReserved:14;        // Bit1-15 may be defined for other capability in future
#endif
    };
  };
}ATOM_ENCODER_CAP_RECORD;

// value for ATOM_CONNECTOR_CF_RECORD.ucConnectedDvoBundle
#define ATOM_CONNECTOR_CF_RECORD_CONNECTED_UPPER12BITBUNDLEA   1
#define ATOM_CONNECTOR_CF_RECORD_CONNECTED_LOWER12BITBUNDLEB   2

typedef struct  _ATOM_CONNECTOR_CF_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
  USHORT                      usMaxPixClk;
  UCHAR                       ucFlowCntlGpioId;
  UCHAR                       ucSwapCntlGpioId;
  UCHAR                       ucConnectedDvoBundle;
  UCHAR                       ucPadding;
}ATOM_CONNECTOR_CF_RECORD;

typedef struct  _ATOM_CONNECTOR_HARDCODE_DTD_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;
   ATOM_DTD_FORMAT                     asTiming;
}ATOM_CONNECTOR_HARDCODE_DTD_RECORD;

typedef struct _ATOM_CONNECTOR_PCIE_SUBCONNECTOR_RECORD
{
  ATOM_COMMON_RECORD_HEADER   sheader;                //ATOM_CONNECTOR_PCIE_SUBCONNECTOR_RECORD_TYPE
  UCHAR                       ucSubConnectorType;     //CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D|X_ID_DUAL_LINK_DVI_D|HDMI_TYPE_A
  UCHAR                       ucReserved;
}ATOM_CONNECTOR_PCIE_SUBCONNECTOR_RECORD;


typedef struct _ATOM_ROUTER_DDC_PATH_SELECT_RECORD
{
   ATOM_COMMON_RECORD_HEADER   sheader;
   UCHAR                                    ucMuxType;                     //decide the number of ucMuxState, =0, no pin state, =1: single state with complement, >1: multiple state
   UCHAR                                    ucMuxControlPin;
   UCHAR                                    ucMuxState[2];               //for alligment purpose
}ATOM_ROUTER_DDC_PATH_SELECT_RECORD;

typedef struct _ATOM_ROUTER_DATA_CLOCK_PATH_SELECT_RECORD
{
   ATOM_COMMON_RECORD_HEADER   sheader;
   UCHAR                                    ucMuxType;
   UCHAR                                    ucMuxControlPin;
   UCHAR                                    ucMuxState[2];               //for alligment purpose
}ATOM_ROUTER_DATA_CLOCK_PATH_SELECT_RECORD;

// define ucMuxType
#define ATOM_ROUTER_MUX_PIN_STATE_MASK                        0x0f
#define ATOM_ROUTER_MUX_PIN_SINGLE_STATE_COMPLEMENT      0x01

typ