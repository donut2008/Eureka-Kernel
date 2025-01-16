ne ATOM_CV_EXT_ENCODER1_INDEX                        0x0000000B
#define ATOM_DFP_INT_ENCODER3_INDEX                       0x0000000C
#define ATOM_DFP_INT_ENCODER4_INDEX                       0x0000000D

// define ASIC internal encoder id ( bit vector ), used for CRTC_SourceSelTable
#define ASIC_INT_DAC1_ENCODER_ID                                     0x00
#define ASIC_INT_TV_ENCODER_ID                                       0x02
#define ASIC_INT_DIG1_ENCODER_ID                                     0x03
#define ASIC_INT_DAC2_ENCODER_ID                                     0x04
#define ASIC_EXT_TV_ENCODER_ID                                       0x06
#define ASIC_INT_DVO_ENCODER_ID                                      0x07
#define ASIC_INT_DIG2_ENCODER_ID                                     0x09
#define ASIC_EXT_DIG_ENCODER_ID                                      0x05
#define ASIC_EXT_DIG2_ENCODER_ID                                     0x08
#define ASIC_INT_DIG3_ENCODER_ID                                     0x0a
#define ASIC_INT_DIG4_ENCODER_ID                                     0x0b
#define ASIC_INT_DIG5_ENCODER_ID                                     0x0c
#define ASIC_INT_DIG6_ENCODER_ID                                     0x0d
#define ASIC_INT_DIG7_ENCODER_ID                                     0x0e

//define Encoder attribute
#define ATOM_ANALOG_ENCODER                                                0
#define ATOM_DIGITAL_ENCODER                                             1
#define ATOM_DP_ENCODER                                                   2

#define ATOM_ENCODER_ENUM_MASK                            0x70
#define ATOM_ENCODER_ENUM_ID1                             0x00
#define ATOM_ENCODER_ENUM_ID2                             0x10
#define ATOM_ENCODER_ENUM_ID3                             0x20
#define ATOM_ENCODER_ENUM_ID4                             0x30
#define ATOM_ENCODER_ENUM_ID5                             0x40
#define ATOM_ENCODER_ENUM_ID6                             0x50

#define ATOM_DEVICE_CRT1_INDEX                            0x00000000
#define ATOM_DEVICE_LCD1_INDEX                            0x00000001
#define ATOM_DEVICE_TV1_INDEX                             0x00000002
#define ATOM_DEVICE_DFP1_INDEX                            0x00000003
#define ATOM_DEVICE_CRT2_INDEX                            0x00000004
#define ATOM_DEVICE_LCD2_INDEX                            0x00000005
#define ATOM_DEVICE_DFP6_INDEX                            0x00000006
#define ATOM_DEVICE_DFP2_INDEX                            0x00000007
#define ATOM_DEVICE_CV_INDEX                              0x00000008
#define ATOM_DEVICE_DFP3_INDEX                            0x00000009
#define ATOM_DEVICE_DFP4_INDEX                            0x0000000A
#define ATOM_DEVICE_DFP5_INDEX                            0x0000000B

#define ATOM_DEVICE_RESERVEDC_INDEX                       0x0000000C
#define ATOM_DEVICE_RESERVEDD_INDEX                       0x0000000D
#define ATOM_DEVICE_RESERVEDE_INDEX                       0x0000000E
#define ATOM_DEVICE_RESERVEDF_INDEX                       0x0000000F
#define ATOM_MAX_SUPPORTED_DEVICE_INFO                    (ATOM_DEVICE_DFP3_INDEX+1)
#define ATOM_MAX_SUPPORTED_DEVICE_INFO_2                  ATOM_MAX_SUPPORTED_DEVICE_INFO
#define ATOM_MAX_SUPPORTED_DEVICE_INFO_3                  (ATOM_DEVICE_DFP5_INDEX + 1 )

#define ATOM_MAX_SUPPORTED_DEVICE                         (ATOM_DEVICE_RESERVEDF_INDEX+1)

#define ATOM_DEVICE_CRT1_SUPPORT                          (0x1L << ATOM_DEVICE_CRT1_INDEX )
#define ATOM_DEVICE_LCD1_SUPPORT                          (0x1L << ATOM_DEVICE_LCD1_INDEX )
#define ATOM_DEVICE_TV1_SUPPORT                           (0x1L << ATOM_DEVICE_TV1_INDEX  )
#define ATOM_DEVICE_DFP1_SUPPORT                          (0x1L << ATOM_DEVICE_DFP1_INDEX )
#define ATOM_DEVICE_CRT2_SUPPORT                          (0x1L << ATOM_DEVICE_CRT2_INDEX )
#define ATOM_DEVICE_LCD2_SUPPORT                          (0x1L << ATOM_DEVICE_LCD2_INDEX )
#define ATOM_DEVICE_DFP6_SUPPORT                          (0x1L << ATOM_DEVICE_DFP6_INDEX )
#define ATOM_DEVICE_DFP2_SUPPORT                          (0x1L << ATOM_DEVICE_DFP2_INDEX )
#define ATOM_DEVICE_CV_SUPPORT                            (0x1L << ATOM_DEVICE_CV_INDEX   )
#define ATOM_DEVICE_DFP3_SUPPORT                          (0x1L << ATOM_DEVICE_DFP3_INDEX )
#define ATOM_DEVICE_DFP4_SUPPORT                          (0x1L << ATOM_DEVICE_DFP4_INDEX )
#define ATOM_DEVICE_DFP5_SUPPORT                          (0x1L << ATOM_DEVICE_DFP5_INDEX )


#define ATOM_DEVICE_CRT_SUPPORT                           (ATOM_DEVICE_CRT1_SUPPORT | ATOM_DEVICE_CRT2_SUPPORT)
#define ATOM_DEVICE_DFP_SUPPORT                           (ATOM_DEVICE_DFP1_SUPPORT | ATOM_DEVICE_DFP2_SUPPORT |  ATOM_DEVICE_DFP3_SUPPORT | ATOM_DEVICE_DFP4_SUPPORT | ATOM_DEVICE_DFP5_SUPPORT | ATOM_DEVICE_DFP6_SUPPORT)
#define ATOM_DEVICE_TV_SUPPORT                            ATOM_DEVICE_TV1_SUPPORT
#define ATOM_DEVICE_LCD_SUPPORT                           (ATOM_DEVICE_LCD1_SUPPORT | ATOM_DEVICE_LCD2_SUPPORT)

#define ATOM_DEVICE_CONNECTOR_TYPE_MASK                   0x000000F0
#define ATOM_DEVICE_CONNECTOR_TYPE_SHIFT                  0x00000004
#define ATOM_DEVICE_CONNECTOR_VGA                         0x00000001
#define ATOM_DEVICE_CONNECTOR_DVI_I                       0x00000002
#define ATOM_DEVICE_CONNECTOR_DVI_D                       0x00000003
#define ATOM_DEVICE_CONNECTOR_DVI_A                       0x00000004
#define ATOM_DEVICE_CONNECTOR_SVIDEO                      0x00000005
#define ATOM_DEVICE_CONNECTOR_COMPOSITE                   0x00000006
#define ATOM_DEVICE_CONNECTOR_LVDS                        0x00000007
#define ATOM_DEVICE_CONNECTOR_DIGI_LINK                   0x00000008
#define ATOM_DEVICE_CONNECTOR_SCART                       0x00000009
#define ATOM_DEVICE_CONNECTOR_HDMI_TYPE_A                 0x0000000A
#define ATOM_DEVICE_CONNECTOR_HDMI_TYPE_B                 0x0000000B
#define ATOM_DEVICE_CONNECTOR_CASE_1                      0x0000000E
#define ATOM_DEVICE_CONNECTOR_DISPLAYPORT                 0x0000000F


#define ATOM_DEVICE_DAC_INFO_MASK                         0x0000000F
#define ATOM_DEVICE_DAC_INFO_SHIFT                        0x00000000
#define ATOM_DEVICE_DAC_INFO_NODAC                        0x00000000
#define ATOM_DEVICE_DAC_INFO_DACA                         0x00000001
#define ATOM_DEVICE_DAC_INFO_DACB                         0x00000002
#define ATOM_DEVICE_DAC_INFO_EXDAC                        0x00000003

#define ATOM_DEVICE_I2C_ID_NOI2C                          0x00000000

#define ATOM_DEVICE_I2C_LINEMUX_MASK                      0x0000000F
#define ATOM_DEVICE_I2C_LINEMUX_SHIFT                     0x00000000

#define ATOM_DEVICE_I2C_ID_MASK                           0x00000070
#define ATOM_DEVICE_I2C_ID_SHIFT                          0x00000004
#define ATOM_DEVICE_I2C_ID_IS_FOR_NON_MM_USE              0x00000001
#define ATOM_DEVICE_I2C_ID_IS_FOR_MM_USE                  0x00000002
#define ATOM_DEVICE_I2C_ID_IS_FOR_SDVO_USE                0x00000003    //For IGP RS600
#define ATOM_DEVICE_I2C_ID_IS_FOR_DAC_SCL                 0x00000004    //For IGP RS690

#define ATOM_DEVICE_I2C_HARDWARE_CAP_MASK                 0x00000080
#define ATOM_DEVICE_I2C_HARDWARE_CAP_SHIFT                0x00000007
#define ATOM_DEVICE_USES_SOFTWARE_ASSISTED_I2C            0x00000000
#define ATOM_DEVICE_USES_HARDWARE_ASSISTED_I2C            0x00000001

//  usDeviceSupport:
//  Bits0   = 0 - no CRT1 support= 1- CRT1 is supported
//  Bit 1   = 0 - no LCD1 support= 1- LCD1 is supported
//  Bit 2   = 0 - no TV1  support= 1- TV1  is supported
//  Bit 3   = 0 - no DFP1 support= 1- DFP1 is supported
//  Bit 4   = 0 - no CRT2 support= 1- CRT2 is supported
//  Bit 5   = 0 - no LCD2 support= 1- LCD2 is supported
//  Bit 6   = 0 - no DFP6 support= 1- DFP6 is supported
//  Bit 7   = 0 - no DFP2 support= 1- DFP2 is supported
//  Bit 8   = 0 - no CV   support= 1- CV   is supported
//  Bit 9   = 0 - no DFP3 support= 1- DFP3 is supported
//  Bit 10= 0 - no DFP4 support= 1- DFP4 is supported
//  Bit 11= 0 - no DFP5 support= 1- DFP5 is supported
//
//

/****************************************************************************/
// Structure used in MclkSS_InfoTable
/****************************************************************************/
//      ucI2C_ConfigID
//    [7:0] - I2C LINE Associate ID
//          = 0   - no I2C
//    [7]      -   HW_Cap        =   1,  [6:0]=HW assisted I2C ID(HW line selection)
//                          =   0,  [6:0]=SW assisted I2C ID
//    [6-4]   - HW_ENGINE_ID  =   1,  HW engine for NON multimedia use
//                          =   2,   HW engine for Multimedia use
//                          =   3-7   Reserved for future I2C engines
//      [3-0] - I2C_LINE_MUX  = A Mux number when it's HW assisted I2C or GPIO ID when it's SW I2C

typedef struct _ATOM_I2C_ID_CONFIG
{
#if ATOM_BIG_ENDIAN
  UCHAR   bfHW_Capable:1;
  UCHAR   bfHW_EngineID:3;
  UCHAR   bfI2C_LineMux:4;
#else
  UCHAR   bfI2C_LineMux:4;
  UCHAR   bfHW_EngineID:3;
  UCHAR   bfHW_Capable:1;
#endif
}ATOM_I2C_ID_CONFIG;

typedef union _ATOM_I2C_ID_CONFIG_ACCESS
{
  ATOM_I2C_ID_CONFIG sbfAccess;
  UCHAR              ucAccess;
}ATOM_I2C_ID_CONFIG_ACCESS;


/****************************************************************************/
// Structure used in GPIO_I2C_InfoTable
/****************************************************************************/
typedef struct _ATOM_GPIO_I2C_ASSIGMENT
{
  USHORT                    usClkMaskRegisterIndex;
  USHORT                    usClkEnRegisterIndex;
  USHORT                    usClkY_RegisterIndex;
  USHORT                    usClkA_RegisterIndex;
  USHORT                    usDataMaskRegisterIndex;
  USHORT                    usDataEnRegisterIndex;
  USHORT                    usDataY_RegisterIndex;
  USHORT                    usDataA_RegisterIndex;
  ATOM_I2C_ID_CONFIG_ACCESS sucI2cId;
  UCHAR                     ucClkMaskShift;
  UCHAR                     ucClkEnShift;
  UCHAR                     ucClkY_Shift;
  UCHAR                     ucClkA_Shift;
  UCHAR                     ucDataMaskShift;
  UCHAR                     ucDataEnShift;
  UCHAR                     ucDataY_Shift;
  UCHAR                     ucDataA_Shift;
  UCHAR                     ucReserved1;
  UCHAR                     ucReserved2;
}ATOM_GPIO_I2C_ASSIGMENT;

typedef struct _ATOM_GPIO_I2C_INFO
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ATOM_GPIO_I2C_ASSIGMENT   asGPIO_Info[ATOM_MAX_SUPPORTED_DEVICE];
}ATOM_GPIO_I2C_INFO;

/****************************************************************************/
// Common Structure used in other structures
/****************************************************************************/

#ifndef _H2INC

//Please don't add or expand this bitfield structure below, this one will retire soon.!
typedef struct _ATOM_MODE_MISC_INFO
{
#if ATOM_BIG_ENDIAN
  USHORT Reserved:6;
  USHORT RGB888:1;
  USHORT DoubleClock:1;
  USHORT Interlace:1;
  USHORT CompositeSync:1;
  USHORT V_ReplicationBy2:1;
  USHORT H_ReplicationBy2:1;
  USHORT VerticalCutOff:1;
  USHORT VSyncPolarity:1;      //0=Active High, 1=Active Low
  USHORT HSyncPolarity:1;      //0=Active High, 1=Active Low
  USHORT HorizontalCutOff:1;
#else
  USHORT HorizontalCutOff:1;
  USHORT HSyncPolarity:1;      //0=Active High, 1=Active Low
  USHORT VSyncPolarity:1;      //0=Active High, 1=Active Low
  USHORT VerticalCutOff:1;
  USHORT H_ReplicationBy2:1;
  USHORT V_ReplicationBy2:1;
  USHORT CompositeSync:1;
  USHORT Interlace:1;
  USHORT DoubleClock:1;
  USHORT RGB888:1;
  USHORT Reserved:6;
#endif
}ATOM_MODE_MISC_INFO;

typedef union _ATOM_MODE_MISC_INFO_ACCESS
{
  ATOM_MODE_MISC_INFO sbfAccess;
  USHORT              usAccess;
}ATOM_MODE_MISC_INFO_ACCESS;

#else

typedef union _ATOM_MODE_MISC_INFO_ACCESS
{
  USHORT              usAccess;
}ATOM_MODE_MISC_INFO_ACCESS;

#endif

// usModeMiscInfo-
#define ATOM_H_CUTOFF           0x01
#define ATOM_HSYNC_POLARITY     0x02             //0=Active High, 1=Active Low
#define ATOM_VSYNC_POLARITY     0x04             //0=Active High, 1=Active Low
#define ATOM_V_CUTOFF           0x08
#define ATOM_H_REPLICATIONBY2   0x10
#define ATOM_V_REPLICATIONBY2   0x20
#define ATOM_COMPOSITESYNC      0x40
#define ATOM_INTERLACE          0x80
#define ATOM_DOUBLE_CLOCK_MODE  0x100
#define ATOM_RGB888_MODE        0x200

//usRefreshRate-
#define ATOM_REFRESH_43         43
#define ATOM_REFRESH_47         47
#define ATOM_REFRESH_56         56
#define ATOM_REFRESH_60         60
#define ATOM_REFRESH_65         65
#define ATOM_REFRESH_70         70
#define ATOM_REFRESH_72         72
#define ATOM_REFRESH_75         75
#define ATOM_REFRESH_85         85

// ATOM_MODE_TIMING data are exactly the same as VESA timing data.
// Translation from EDID to ATOM_MODE_TIMING, use the following formula.
//
//   VESA_HTOTAL         =   VESA_ACTIVE + 2* VESA_BORDER + VESA_BLANK
//                  =   EDID_HA + EDID_HBL
//   VESA_HDISP         =   VESA_ACTIVE   =   EDID_HA
//   VESA_HSYNC_START   =   VESA_ACTIVE + VESA_BORDER + VESA_FRONT_PORCH
//                  =   EDID_HA + EDID_HSO
//   VESA_HSYNC_WIDTH   =   VESA_HSYNC_TIME   =   EDID_HSPW
//   VESA_BORDER         =   EDID_BORDER


/****************************************************************************/
// Structure used in SetCRTC_UsingDTDTimingTable
/****************************************************************************/
typedef struct _SET_CRTC_USING_DTD_TIMING_PARAMETERS
{
  USHORT  usH_Size;
  USHORT  usH_Blanking_Time;
  USHORT  usV_Size;
  USHORT  usV_Blanking_Time;
  USHORT  usH_SyncOffset;
  USHORT  usH_SyncWidth;
  USHORT  usV_SyncOffset;
  USHORT  usV_SyncWidth;
  ATOM_MODE_MISC_INFO_ACCESS  susModeMiscInfo;
  UCHAR   ucH_Border;         // From DFP EDID
  UCHAR   ucV_Border;
  UCHAR   ucCRTC;             // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR   ucPadding[3];
}SET_CRTC_USING_DTD_TIMING_PARAMETERS;

/****************************************************************************/
// Structure used in SetCRTC_TimingTable
/****************************************************************************/
typedef struct _SET_CRTC_TIMING_PARAMETERS
{
  USHORT                      usH_Total;        // horizontal total
  USHORT                      usH_Disp;         // horizontal display
  USHORT                      usH_SyncStart;    // horozontal Sync start
  USHORT                      usH_SyncWidth;    // horizontal Sync width
  USHORT                      usV_Total;        // vertical total
  USHORT                      usV_Disp;         // vertical display
  USHORT                      usV_SyncStart;    // vertical Sync start
  USHORT                      usV_SyncWidth;    // vertical Sync width
  ATOM_MODE_MISC_INFO_ACCESS  susModeMiscInfo;
  UCHAR                       ucCRTC;           // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR                       ucOverscanRight;  // right
  UCHAR                       ucOverscanLeft;   // left
  UCHAR                       ucOverscanBottom; // bottom
  UCHAR                       ucOverscanTop;    // top
  UCHAR                       ucReserved;
}SET_CRTC_TIMING_PARAMETERS;
#define SET_CRTC_TIMING_PARAMETERS_PS_ALLOCATION SET_CRTC_TIMING_PARAMETERS


/****************************************************************************/
// Structure used in StandardVESA_TimingTable
//                   AnalogTV_InfoTable
//                   ComponentVideoInfoTable
/****************************************************************************/
typedef struct _ATOM_MODE_TIMING
{
  USHORT  usCRTC_H_Total;
  USHORT  usCRTC_H_Disp;
  USHORT  usCRTC_H_SyncStart;
  USHORT  usCRTC_H_SyncWidth;
  USHORT  usCRTC_V_Total;
  USHORT  usCRTC_V_Disp;
  USHORT  usCRTC_V_SyncS