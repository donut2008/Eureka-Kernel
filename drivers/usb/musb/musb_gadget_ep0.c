       // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR  ucBlanking;                  // ATOM_BLANKING or ATOM_BLANKINGOFF
  USHORT usBlackColorRCr;
  USHORT usBlackColorGY;
  USHORT usBlackColorBCb;
}BLANK_CRTC_PARAMETERS;
#define BLANK_CRTC_PS_ALLOCATION    BLANK_CRTC_PARAMETERS

/****************************************************************************/
// Structures used by EnableCRTCTable
//                    EnableCRTCMemReqTable
//                    UpdateCRTC_DoubleBufferRegistersTable
/****************************************************************************/
typedef struct _ENABLE_CRTC_PARAMETERS
{
  UCHAR ucCRTC;                         // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR ucEnable;                     // ATOM_ENABLE or ATOM_DISABLE
  UCHAR ucPadding[2];
}ENABLE_CRTC_PARAMETERS;
#define ENABLE_CRTC_PS_ALLOCATION   ENABLE_CRTC_PARAMETERS

/****************************************************************************/
// Structures used by SetCRTC_OverScanTable
/****************************************************************************/
typedef struct _SET_CRTC_OVERSCAN_PARAMETERS
{
  USHORT usOverscanRight;             // right
  USHORT usOverscanLeft;              // left
  USHORT usOverscanBottom;            // bottom
  USHORT usOverscanTop;               // top
  UCHAR  ucCRTC;                      // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR  ucPadding[3];
}SET_CRTC_OVERSCAN_PARAMETERS;
#define SET_CRTC_OVERSCAN_PS_ALLOCATION  SET_CRTC_OVERSCAN_PARAMETERS

/****************************************************************************/
// Structures used by SetCRTC_ReplicationTable
/****************************************************************************/
typedef struct _SET_CRTC_REPLICATION_PARAMETERS
{
  UCHAR ucH_Replication;              // horizontal replication
  UCHAR ucV_Replication;              // vertical replication
  UCHAR usCRTC;                       // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR ucPadding;
}SET_CRTC_REPLICATION_PARAMETERS;
#define SET_CRTC_REPLICATION_PS_ALLOCATION  SET_CRTC_REPLICATION_PARAMETERS

/****************************************************************************/
// Structures used by SelectCRTC_SourceTable
/****************************************************************************/
typedef struct _SELECT_CRTC_SOURCE_PARAMETERS
{
  UCHAR ucCRTC;                         // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR ucDevice;                     // ATOM_DEVICE_CRT1|ATOM_DEVICE_CRT2|....
  UCHAR ucPadding[2];
}SELECT_CRTC_SOURCE_PARAMETERS;
#define SELECT_CRTC_SOURCE_PS_ALLOCATION  SELECT_CRTC_SOURCE_PARAMETERS

typedef struct _SELECT_CRTC_SOURCE_PARAMETERS_V2
{
  UCHAR ucCRTC;                         // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR ucEncoderID;                  // DAC1/DAC2/TVOUT/DIG1/DIG2/DVO
  UCHAR ucEncodeMode;                           // Encoding mode, only valid when using DIG1/DIG2/DVO
  UCHAR ucPadding;
}SELECT_CRTC_SOURCE_PARAMETERS_V2;

//ucEncoderID
//#define ASIC_INT_DAC1_ENCODER_ID                      0x00
//#define ASIC_INT_TV_ENCODER_ID                           0x02
//#define ASIC_INT_DIG1_ENCODER_ID                        0x03
//#define ASIC_INT_DAC2_ENCODER_ID                        0x04
//#define ASIC_EXT_TV_ENCODER_ID                           0x06
//#define ASIC_INT_DVO_ENCODER_ID                           0x07
//#define ASIC_INT_DIG2_ENCODER_ID                        0x09
//#define ASIC_EXT_DIG_ENCODER_ID                           0x05

//ucEncodeMode
//#define ATOM_ENCODER_MODE_DP                              0
//#define ATOM_ENCODER_MODE_LVDS                           1
//#define ATOM_ENCODER_MODE_DVI                              2
//#define ATOM_ENCODER_MODE_HDMI                           3
//#define ATOM_ENCODER_MODE_SDVO                           4
//#define ATOM_ENCODER_MODE_TV                              13
//#define ATOM_ENCODER_MODE_CV                              14
//#define ATOM_ENCODER_MODE_CRT                              15


typedef struct _SELECT_CRTC_SOURCE_PARAMETERS_V3
{
  UCHAR ucCRTC;                         // ATOM_CRTC1 or ATOM_CRTC2
  UCHAR ucEncoderID;                    // DAC1/DAC2/TVOUT/DIG1/DIG2/DVO
  UCHAR ucEncodeMode;                   // Encoding mode, only valid when using DIG1/DIG2/DVO
  UCHAR ucDstBpc;                       // PANEL_6/8/10/12BIT_PER_COLOR
}SELECT_CRTC_SOURCE_PARAMETERS_V3;


/****************************************************************************/
// Structures used by SetPixelClockTable
//                    GetPixelClockTable
/****************************************************************************/
//Major revision=1., Minor revision=1
typedef struct _PIXEL_CLOCK_PARAMETERS
{
  USHORT usPixelClock;                // in 10kHz unit; for bios convenient = (RefClk*FB_Div)/(Ref_Div*Post_Div)
                                      // 0 means disable PPLL
  USHORT usRefDiv;                    // Reference divider
  USHORT usFbDiv;                     // feedback divider
  UCHAR  ucPostDiv;                   // post divider
  UCHAR  ucFracFbDiv;                 // fractional feedback divider
  UCHAR  ucPpll;                      // ATOM_PPLL1 or ATOM_PPL2
  UCHAR  ucRefDivSrc;                 // ATOM_PJITTER or ATO_NONPJITTER
  UCHAR  ucCRTC;                      // Which CRTC uses this Ppll
  UCHAR  ucPadding;
}PIXEL_CLOCK_PARAMETERS;

//Major revision=1., Minor revision=2, add ucMiscIfno
//ucMiscInfo:
#define MISC_FORCE_REPROG_PIXEL_CLOCK 0x1
#define MISC_DEVICE_INDEX_MASK        0xF0
#define MISC_DEVICE_INDEX_SHIFT       4

typedef struct _PIXEL_CLOCK_PARAMETERS_V2
{
  USHORT usPixelClock;                // in 10kHz unit; for bios convenient = (RefClk*FB_Div)/(Ref_Div*Post_Div)
                                      // 0 means disable PPLL
  USHORT usRefDiv;                    // Reference divider
  USHORT usFbDiv;                     // feedback divider
  UCHAR  ucPostDiv;                   // post divider
  UCHAR  ucFracFbDiv;                 // fractional feedback divider
  UCHAR  ucPpll;                      // ATOM_PPLL1 or ATOM_PPL2
  UCHAR  ucRefDivSrc;                 // ATOM_PJITTER or ATO_NONPJITTER
  UCHAR  ucCRTC;                      // Which CRTC uses this Ppll
  UCHAR  ucMiscInfo;                  // Different bits for different purpose, bit [7:4] as device index, bit[0]=Force prog
}PIXEL_CLOCK_PARAMETERS_V2;

//Major revision=1., Minor revision=3, structure/definition change
//ucEncoderMode:
//ATOM_ENCODER_MODE_DP
//ATOM_ENOCDER_MODE_LVDS
//ATOM_ENOCDER_MODE_DVI
//ATOM_ENOCDER_MODE_HDMI
//ATOM_ENOCDER_MODE_SDVO
//ATOM_ENCODER_MODE_TV                                          13
//ATOM_ENCODER_MODE_CV                                          14
//ATOM_ENCODER_MODE_CRT                                          15

//ucDVOConfig
//#define DVO_ENCODER_CONFIG_RATE_SEL                     0x01
//#define DVO_ENCODER_CONFIG_DDR_SPEED                  0x00
//#define DVO_ENCODER_CONFIG_SDR_SPEED                  0x01
//#define DVO_ENCODER_CONFIG_OUTPUT_SEL                  0x0c
//#define DVO_ENCODER_CONFIG_LOW12BIT                     0x00
//#define DVO_ENCODER_CONFIG_UPPER12BIT                  0x04
//#define DVO_ENCODER_CONFIG_24BIT                        0x08

//ucMiscInfo: also changed, see below
#define PIXEL_CLOCK_MISC_FORCE_PROG_PPLL                  0x01
#define PIXEL_CLOCK_MISC_VGA_MODE                              0x02
#define PIXEL_CLOCK_MISC_CRTC_SEL_MASK                     0x04
#define PIXEL_CLOCK_MISC_CRTC_SEL_CRTC1                     0x00
#define PIXEL_CLOCK_MISC_CRTC_SEL_CRTC2                     0x04
#define PIXEL_CLOCK_MISC_USE_ENGINE_FOR_DISPCLK         0x08
#define PIXEL_CLOCK_MISC_REF_DIV_SRC                    0x10
// V1.4 for RoadRunner
#define PIXEL_CLOCK_V4_MISC_SS_ENABLE               0x10
#define PIXEL_CLOCK_V4_MISC_COHERENT_MODE           0x20


typedef struct _PIXEL_CLOCK_PARAMETERS_V3
{
  USHORT usPixelClock;                // in 10kHz unit; for bios convenient = (RefClk*FB_Div)/(Ref_Div*Post_Div)
                                      // 0 means disable PPLL. For VGA PPLL,make sure this value is not 0.
  USHORT usRefDiv;                    // Reference divider
  USHORT usFbDiv;                     // feedback divider
  UCHAR  ucPostDiv;                   // post divider
  UCHAR  ucFracFbDiv;                 // fractional feedback divider
  UCHAR  ucPpll;                      // ATOM_PPLL1 or ATOM_PPL2
  UCHAR  ucTransmitterId;             // graphic encoder id defined in objectId.h
   union
   {
  UCHAR  ucEncoderMode;               // encoder type defined as ATOM_ENCODER_MODE_DP/DVI/HDMI/
   UCHAR  ucDVOConfig;                           // when use DVO, need to know SDR/DDR, 12bit or 24bit
   };
  UCHAR  ucMiscInfo;                  // bit[0]=Force program, bit[1]= set pclk for VGA, b[2]= CRTC sel
                                      // bit[3]=0:use PPLL for dispclk source, =1: use engine clock for dispclock source
                                      // bit[4]=0:use XTALIN as the source of reference divider,=1 use the pre-defined clock as the source of reference divider
}PIXEL_CLOCK_PARAMETERS_V3;

#define PIXEL_CLOCK_PARAMETERS_LAST                     PIXEL_CLOCK_PARAMETERS_V2
#define GET_PIXEL_CLOCK_PS_ALLOCATION                  PIXEL_CLOCK_PARAMETERS_LAST


typedef struct _PIXEL_CLOCK_PARAMETERS_V5
{
  UCHAR  ucCRTC;             // ATOM_CRTC1~6, indicate the CRTC controller to
                             // drive the pixel clock. not used for DCPLL case.
  union{
  UCHAR  ucReserved;
  UCHAR  ucFracFbDiv;        // [gphan] temporary to prevent build problem.  remove it after driver code is changed.
  };
  USHORT usPixelClock;       // target the pixel clock to drive the CRTC timing
                             // 0 means disable PPLL/DCPLL.
  USHORT usFbDiv;            // feedback divider integer part.
  UCHAR  ucPostDiv;          // post divider.
  UCHAR  ucRefDiv;           // Reference divider
  UCHAR  ucPpll;             // ATOM_PPLL1/ATOM_PPLL2/ATOM_DCPLL
  UCHAR  ucTransmitterID;    // ASIC encoder id defined in objectId.h,
                             // indicate which graphic encoder will be used.
  UCHAR  ucEncoderMode;      // Encoder mode:
  UCHAR  ucMiscInfo;         // bit[0]= Force program PPLL
                             // bit[1]= when VGA timing is used.
                             // bit[3:2]= HDMI panel bit depth: =0: 24bpp =1:30bpp, =2:32bpp
                             // bit[4]= RefClock source for PPLL.
                             // =0: XTLAIN( default mode )
                              // =1: other external clock source, which is pre-defined
                             //     by VBIOS depend on the feature required.
                             // bit[7:5]: reserved.
  ULONG  ulFbDivDecFrac;     // 20 bit feedback divider decimal fraction part, range from 1~999999 ( 0.000001 to 0.999999 )

}PIXEL_CLOCK_PARAMETERS_V5;

#define PIXEL_CLOCK_V5_MISC_FORCE_PROG_PPLL               0x01
#define PIXEL_CLOCK_V5_MISC_VGA_MODE                        0x02
#define PIXEL_CLOCK_V5_MISC_HDMI_BPP_MASK           0x0c
#define PIXEL_CLOCK_V5_MISC_HDMI_24BPP              0x00
#define PIXEL_CLOCK_V5_MISC_HDMI_30BPP              0x04
#define PIXEL_CLOCK_V5_MISC_HDMI_32BPP              0x08
#define PIXEL_CLOCK_V5_MISC_REF_DIV_SRC             0x10

typedef struct _CRTC_PIXEL_CLOCK_FREQ
{
#if ATOM_BIG_ENDIAN
  ULONG  ucCRTC:8;            // ATOM_CRTC1~6, indicate the CRTC controller to
                              // drive the pixel clock. not used for DCPLL case.
  ULONG  ulPixelClock:24;     // target the pixel clock to drive the CRTC timing.
                              // 0 means disable PPLL/DCPLL. Expanded to 24 bits comparing to previous version.
#else
  ULONG  ulPixelClock:24;     // target the pixel clock to drive the CRTC timing.
                              // 0 means disable PPLL/DCPLL. Expanded to 24 bits comparing to previous version.
  ULONG  ucCRTC:8;            // ATOM_CRTC1~6, indicate the CRTC controller to
                              // drive the pixel clock. not used for DCPLL case.
#endif
}CRTC_PIXEL_CLOCK_FREQ;

typedef struct _PIXEL_CLOCK_PARAMETERS_V6
{
  union{
    CRTC_PIXEL_CLOCK_FREQ ulCrtcPclkFreq;    // pixel clock and CRTC id frequency
    ULONG ulDispEngClkFreq;                  // dispclk frequency
  };
  USHORT usFbDiv;            // feedback divider integer part.
  UCHAR  ucPostDiv;          // post divider.
  UCHAR  ucRefDiv;           // Reference divider
  UCHAR  ucPpll;             // ATOM_PPLL1/ATOM_PPLL2/ATOM_DCPLL
  UCHAR  ucTransmitterID;    // ASIC encoder id defined in objectId.h,
                             // indicate which graphic encoder will be used.
  UCHAR  ucEncoderMode;      // Encoder mode:
  UCHAR  ucMiscInfo;         // bit[0]= Force program PPLL
                             // bit[1]= when VGA timing is used.
                             // bit[3:2]= HDMI panel bit depth: =0: 24bpp =1:30bpp, =2:32bpp
                             // bit[4]= RefClock source for PPLL.
                             // =0: XTLAIN( default mode )
                              // =1: other external clock source, which is pre-defined
                             //     by VBIOS depend on the feature required.
                             // bit[7:5]: reserved.
  ULONG  ulFbDivDecFrac;     // 20 bit feedback divider decimal fraction part, range from 1~999999 ( 0.000001 to 0.999999 )

}PIXEL_CLOCK_PARAMETERS_V6;

#define PIXEL_CLOCK_V6_MISC_FORCE_PROG_PPLL               0x01
#define PIXEL_CLOCK_V6_MISC_VGA_MODE                        0x02
#define PIXEL_CLOCK_V6_MISC_HDMI_BPP_MASK           0x0c
#define PIXEL_CLOCK_V6_MISC_HDMI_24BPP              0x00
#define PIXEL_CLOCK_V6_MISC_HDMI_36BPP              0x04
#define PIXEL_CLOCK_V6_MISC_HDMI_36BPP_V6           0x08    //for V6, the correct defintion for 36bpp should be 2 for 36bpp(2:1)
#define PIXEL_CLOCK_V6_MISC_HDMI_30BPP              0x08
#define PIXEL_CLOCK_V6_MISC_HDMI_30BPP_V6           0x04    //for V6, the correct defintion for 30bpp should be 1 for 36bpp(5:4)
#define PIXEL_CLOCK_V6_MISC_HDMI_48BPP              0x0c
#define PIXEL_CLOCK_V6_MISC_REF_DIV_SRC             0x10
#define PIXEL_CLOCK_V6_MISC_GEN_DPREFCLK            0x40
#define PIXEL_CLOCK_V6_MISC_DPREFCLK_BYPASS         0x40

typedef struct _GET_DISP_PLL_STATUS_INPUT_PARAMETERS_V2
{
  PIXEL_CLOCK_PARAMETERS_V3 sDispClkInput;
}GET_DISP_PLL_STATUS_INPUT_PARAMETERS_V2;

typedef struct _GET_DISP_PLL_STATUS_OUTPUT_PARAMETERS_V2
{
  UCHAR  ucStatus;
  UCHAR  ucRefDivSrc;                 // =1: reference clock source from XTALIN, =0: source from PCIE ref clock
  UCHAR  ucReserved[2];
}GET_DISP_PLL_STATUS_OUTPUT_PARAMETERS_V2;

typedef struct _GET_DISP_PLL_STATUS_INPUT_PARAMETERS_V3
{
  PIXEL_CLOCK_PARAMETERS_V5 sDispClkInput;
}GET_DISP_PLL_STATUS_INPUT_PARAMETERS_V3;


/****************************************************************************/
// Structures used by AdjustDisplayPllTable
/****************************************************************************/
typedef struct _ADJUST_DISPLAY_PLL_PARAMETERS
{
   USHORT usPixelClock;
   UCHAR ucTransmitterID;
   UCHAR ucEncodeMode;
   union
   {
      UCHAR ucDVOConfig;                           //if DVO, need passing link rate and output 12bitlow or 24bit
      UCHAR ucConfig;                                 //if none DVO, not defined yet
   };
   UCHAR ucReserved[3];
}ADJUST_DISPLAY_PLL_PARAMETERS;

#define ADJUST_DISPLAY_CONFIG_SS_ENABLE            0x10
#define ADJUST_DISPLAY_PLL_PS_ALLOCATION              ADJUST_DISPLAY_PLL_PARAMETERS

typedef struct _ADJUST_DISPLAY_PLL_INPUT_PARAMETERS_V3
{
   USHORT usPixelClock;                    // target pixel clock
   UCHAR ucTransmitterID;                  // GPU transmitter id defined in objectid.h
   UCHAR ucEncodeMode;                     // encoder mode: CRT, LVDS, DP, TMDS or HDMI
  UCHAR ucDispPllConfig;                 // display pll configure parameter defined as following DISPPLL_CONFIG_XXXX
  UCHAR ucExtTransmitterID;               // external encoder id.
   UCHAR ucReserved[2];
}ADJUST_DISPLAY_PLL_INPUT_PARAMETERS_V3;

// usDispPllConfig v1.2 for RoadRunner
#define DISPPLL_CONFIG_DVO_RATE_SEL                0x0001     // need only when ucTransmitterID = DVO
#define DISPPLL_CONFIG_DVO_DDR_SPEED               0x0000     // need only when ucTransmitterID = DVO
#define DISPPLL_CONFIG_DVO_SDR_SPEED               0x0001     // need only when ucTransmitterID = DVO
#define DISPPLL_CONFIG_DVO_OUTPUT_SEL              0x000c     // need only when ucTransmitterID = DVO
#define DISPPLL_CONFIG_DVO_LOW12BIT                0x0000     // need only when ucTransmitterID = DVO
#define DISPPLL_CONFIG_DVO_UPPER12BIT              0x0004     // need only when ucTransmitterID = DVO
#define DISPPLL_CONFIG_DVO_24BIT                   0x0008     // need only when ucTransmitterID = DVO
#define DISPPLL_CONFIG_SS_ENABLE                   0x0010     // Only used when ucEncoderMode = DP or LVDS
#define DISPPLL_CONFIG_COHERENT_MODE               0x0020     // Only used when ucEncoderMode = TMDS or HDMI
#define DISPPLL_CONFIG_DUAL_LINK                   0x0040     // Only used when ucEncoderMode = TMDS or LVDS


typedef struct _ADJUST_DISPLAY_PLL_OUTPUT_PARAMETERS_V3
{
  ULONG ulDispPllFreq;                 // return display PPLL freq which is used to generate the pixclock, and related idclk, symclk etc
  UCHAR ucRefDiv;                      // if it is none-zero, it is used to be calculated the other ppll parameter fb_divider and post_div ( if it is not given )
  UCHAR ucPostDiv;                     // if it is none-zero, it is used to be calculated the other ppll parameter fb_divider
  UCHAR ucReserved[2];
}ADJUST_DISPLAY_PLL_OUTPUT_PARAMETERS_V3;

typedef struct _ADJUST_DISPLAY_PLL_PS_ALLOCATION_V3
{
  union
  {
    ADJUST_DISPLAY_PLL_INPUT_PARAMETERS_V3  sInput;
    ADJUST_DISPLAY_PLL_OUTPUT_PARAMETERS_V3 sOutput;
  };
} ADJUST_DISPLAY_PLL_PS_ALLOCATION_V3;

/****************************************************************************/
// Structures used by EnableYUVTable
/****************************************************************************/
typedef struct _ENABLE_YUV_PARAMETERS
{
  UCHAR ucEnable;                     // ATOM_ENABLE:Enable YUV or ATOM_DISABLE:Disable YUV (RGB)
  UCHAR ucCRTC;                       // Which CRTC needs this YUV or RGB format
  UCHAR ucPadding[2];
}ENABLE_YUV_PARAMETERS;
#define ENABLE_YUV_PS_ALLOCATION ENABLE_YUV_PARAMETERS

/****************************************************************************/
// Structures used by GetMemoryClockTable
/****************************************************************************/
typedef struct _GET_MEMORY_CLOCK_PARAMETERS
{
  ULONG ulReturnMemoryClock;          // current memory speed in 10KHz unit
} GET_MEMORY_CLOCK_PARAMETERS;
#define GET_MEMORY_CLOCK_PS_ALLOCATION  GET_MEMORY_CLOCK_PARAMETERS

/****************************************************************************/
// Structures used by GetEngineClockTable
/****************************************************************************/
typedef struct _GET_ENGINE_CLOCK_PARAMETERS
{
  ULONG ulReturnEngineClock;          // current engine speed in 10KHz unit
} GET_ENGINE_CLOCK_PARAMETERS;
#define GET_ENGINE_CLOCK_PS_ALLOCATION  GET_ENGINE_CLOCK_PARAMETERS

/****************************************************************************/
// Following Structures and constant may be obsolete
/****************************************************************************/
//Maxium 8 bytes,the data read in will be placed in the parameter space.
//Read operaion successeful when the paramter space is non-zero, otherwise read operation failed
typedef struct _READ_EDID_FROM_HW_I2C_DATA_PARAMETERS
{
  USHORT    usPrescale;         //Ratio between Engine clock and I2C clock
  USHORT    usVRAMAddress;      //Adress in Frame Buffer where to pace raw EDID
  USHORT    usStatus;           //When use output: lower byte EDID checksum, high byte hardware status
                                //WHen use input:  lower byte as 'byte to read':currently limited to 128byte or 1byte
  UCHAR     ucSlaveAddr;        //Read from which slave
  UCHAR     ucLineNumber;       //Read from which HW assisted line
}READ_EDID_FROM_HW_I2C_DATA_PARAMETERS;
#define READ_EDID_FROM_HW_I2C_DATA_PS_ALLOCATION  READ_EDID_FROM_HW_I2C_DATA_PARAMETERS


#define  ATOM_WRITE_I2C_FORMAT_PSOFFSET_PSDATABYTE                  0
#define  ATOM_WRITE_I2C_FORMAT_PSOFFSET_PSTWODATABYTES              1
#define  ATOM_WRITE_I2C_FORMAT_PSCOUNTER_PSOFFSET_IDDATABLOCK       2
#define  ATOM_WRITE_I2C_FORMAT_PSCOUNTER_IDOFFSET_PLUS_IDDATABLOCK  3
#define  ATOM_WRITE_I2C_FORMAT_IDCOUNTER_IDOFFSET_IDDATABLOCK       4

typedef struct _WRITE_ONE_BYTE_HW_I2C_DATA_PARAMETERS
{
  USHORT    usPrescale;         //Ratio between Engine clock and I2C clock
  USHORT    usByteOffset;       //Write to which byte
                                //Upper portion of usByteOffset is Format of data
                                //1bytePS+offsetPS
                                //2bytesPS+offsetPS
                                //blockID+offsetPS
                                //blockID+offsetID
                                //blockID+counterID+offsetID
  UCHAR     ucData;             //PS data1
  UCHAR     ucStatus;           //Status byte 1=success, 2=failure, Also is used as PS data2
  UCHAR     ucSlaveAddr;        //Write to which slave
  UCHAR     ucLineNumber;       //Write from which HW assisted line
}WRITE_ONE_BYTE_HW_I2C_DATA_PARAMETERS;

#define WRITE_ONE_BYTE_HW_I2C_DATA_PS_ALLOCATION  WRITE_ONE_BYTE_HW_I2C_DATA_PARAMETERS

typedef struct _SET_UP_HW_I2C_DATA_PARAMETERS
{
  USHORT    usPrescale;         //Ratio between Engine clock and I2C clock
  UCHAR     ucSlaveAddr;        //Write to which slave
  UCHAR     ucLineNumber;       //Write from which HW assisted line
}SET_UP_HW_I2C_DATA_PARAMETERS;

/**************************************************************************/
#define SPEED_FAN_CONTROL_PS_ALLOCATION   WRITE_ONE_BYTE_HW_I2C_DATA_PARAMETERS


/****************************************************************************/
// Structures used by PowerConnectorDetectionTable
/****************************************************************************/
typedef struct   _POWER_CONNECTOR_DETECTION_PARAMETERS
{
  UCHAR   ucPowerConnectorStatus;      //Used for return value 0: detected, 1:not detected
   UCHAR   ucPwrBehaviorId;
   USHORT   usPwrBudget;                         //how much power currently boot to in unit of watt
}POWER_CONNECTOR_DETECTION_PARAMETERS;

typedef struct POWER_CONNECTOR_DETECTION_PS_ALLOCATION
{
  UCHAR   ucPowerConnectorStatus;      //Used for return value 0: detected, 1:not detected
   UCHAR   ucReserved;
   USHORT   usPwrBudget;                         //how much power currently boot to in unit of watt
  WRITE_ONE_BYTE_HW_I2C_DATA_PS_ALLOCATION    sReserved;
}POWER_CONNECTOR_DETECTION_PS_ALLOCATION;


/****************************LVDS SS Command Table Definitions**********************/

/****************************************************************************/
// Structures used by EnableSpreadSpectrumOnPPLLTable
/****************************************************************************/
typedef struct   _ENABLE_LVDS_SS_PARAMETERS
{
  USHORT  usSpreadSpectrumPercentage;
  UCHAR   ucSpreadSpectrumType;           //Bit1=0 Down Spread,=1 Center Spread. Bit1=1 Ext. =0 Int. Others:TBD
  UCHAR   ucSpreadSpectrumStepSize_Delay; //bits3:2 SS_STEP_SIZE; bit 6:4 SS_DELAY
  UCHAR   ucEnable;                       //ATOM_ENABLE or ATOM_DISABLE
  UCHAR   ucPadding[3];
}ENABLE_LVDS_SS_PARAMETERS;

//ucTableFormatRevision=1,ucTableContentRevision=2
typedef struct   _ENABLE_LVDS_SS_PARAMETERS_V2
{
  USHORT  usSpreadSpectrumPercentage;
  UCHAR   ucSpreadSpectrumType;           //Bit1=0 Down Spread,=1 Center Spread. Bit1=1 Ext. =0 Int. Others:TBD
  UCHAR   ucSpreadSpectrumStep;           //
  UCHAR   ucEnable;                       //ATOM_ENABLE or ATOM_DISABLE
  UCHAR   ucSpreadSpectrumDelay;
  UCHAR   ucSpreadSpectrumRange;
  UCHAR   ucPadding;
}ENABLE_LVDS_SS_PARAMETERS_V2;

//This new structure is based on ENABLE_LVDS_SS_PARAMETERS but expands to SS on PPLL, so other devices can use SS.
typedef struct   _ENABLE_SPREAD_SPECTRUM_ON_PPLL
{
  USHORT  usSpreadSpectrumPercentage;
  UCHAR   ucSpreadSpectrumType;           // Bit1=0 Down Spread,=1 Center Spread. Bit1=1 Ext. =0 Int. Others:TBD
  UCHAR   ucSpreadSpectrumStep;           //
  UCHAR   ucEnable;                       // ATOM_ENABLE or ATOM_DISABLE
  UCHAR   ucSpreadSpectrumDelay;
  UCHAR   ucSpreadSpectrumRange;
  UCHAR   ucPpll;                                      // ATOM_PPLL1/ATOM_PPLL2
}ENABLE_SPREAD_SPECTRUM_ON_PPLL;

 typedef struct _ENABLE_SPREAD_SPECTRUM_ON_PPLL_V2
{
  USHORT  usSpreadSpectrumPercentage;
  UCHAR   ucSpreadSpectrumType;           // Bit[0]: 0-Down Spread,1-Center Spread.
                                        // Bit[1]: 1-Ext. 0-Int.
                                        // Bit[3:2]: =0 P1PLL =1 P2PLL =2 DCPLL
                                        // Bits[7:4] reserved
  UCHAR   ucEnable;                       // ATOM_ENABLE or ATOM_DISABLE
  USHORT  usSpreadSpectrumAmount;         // Includes SS_AMOUNT_FBDIV[7:0] and SS_AMOUNT_NFRAC_SLIP[11:8]
  USHORT  usSpreadSpectrumStep;           // SS_STEP_SIZE_DSFRAC
}ENABLE_SPREAD_SPECTRUM_ON_PPLL_V2;

#define ATOM_PPLL_SS_TYPE_V2_DOWN_SPREAD      0x00
#define ATOM_PPLL_SS_TYPE_V2_CENTRE_SPREAD    0x01
#define ATOM_PPLL_SS_TYPE_V2_EXT_SPREAD       0x02
#define ATOM_PPLL_SS_TYPE_V2_PPLL_SEL_MASK    0x0c
#define ATOM_PPLL_SS_TYPE_V2_P1PLL            0x00
#define ATOM_PPLL_SS_TYPE_V2_P2PLL            0x04
#define ATOM_PPLL_SS_TYPE_V2_DCPLL            0x08
#define ATOM_PPLL_SS_AMOUNT_V2_FBDIV_MASK     0x00FF
#define ATOM_PPLL_SS_AMOUNT_V2_FBDIV_SHIFT    0
#define ATOM_PPLL_SS_AMOUNT_V2_NFRAC_MASK     0x0F00
#define ATOM_PPLL_SS_AMOUNT_V2_NFRAC_SHIFT    8

// Used by DCE5.0
 typedef struct _ENABLE_SPREAD_SPECTRUM_ON_PPLL_V3
{
  USHORT  usSpreadSpectrumAmountFrac;   // SS_AMOUNT_DSFRAC New in DCE5.0
  UCHAR   ucSpreadSpectrumType;           // Bit[0]: 0-Down Spread,1-Center Spread.
                                        // Bit[1]: 1-Ext. 0-Int.
                                        // Bit[3:2]: =0 P1PLL =1 P2PLL =2 DCPLL
                                        // Bits[7:4] reserved
  UCHAR   ucEnable;                       // ATOM_ENABLE or ATOM_DISABLE
  USHORT  usSpreadSpectrumAmount;         // Includes SS_AMOUNT_FBDIV[7:0] and SS_AMOUNT_NFRAC_SLIP[11:8]
  USHORT  usSpreadSpectrumStep;           // SS_STEP_SIZE_DSFRAC
}ENABLE_SPREAD_SPECTRUM_ON_PPLL_V3;


#define ATOM_PPLL_SS_TYPE_V3_DOWN_SPREAD      0x00
#define ATOM_PPLL_SS_TYPE_V3_CENTRE_SPREAD    0x01
#define ATOM_PPLL_SS_TYPE_V3_EXT_SPREAD       0x02
#define ATOM_PPLL_SS_TYPE_V3_PPLL_SEL_MASK    0x0c
#define ATOM_PPLL_SS_TYPE_V3_P1PLL            0x00
#define ATOM_PPLL_SS_TYPE_V3_P2PLL            0x04
#define ATOM_PPLL_SS_TYPE_V3_DCPLL            0x08
#define ATOM_PPLL_SS_TYPE_V3_P0PLL            ATOM_PPLL_SS_TYPE_V3_DCPLL
#define ATOM_PPLL_SS_AMOUNT_V3_FBDIV_MASK     0x00FF
#define ATOM_PPLL_SS_AMOUNT_V3_FBDIV_SHIFT    0
#define ATOM_PPLL_SS_AMOUNT_V3_NFRAC_MASK     0x0F00
#define ATOM_PPLL_SS_AMOUNT_V3_NFRAC_SHIFT    8

#define ENABLE_SPREAD_SPECTRUM_ON_PPLL_PS_ALLOCATION  ENABLE_SPREAD_SPECTRUM_ON_PPLL

typedef struct _SET_PIXEL_CLOCK_PS_ALLOCATION
{
  PIXEL_CLOCK_PARAMETERS sPCLKInput;
  ENABLE_SPREAD_SPECTRUM_ON_PPLL sReserved;//Caller doesn't need to init this portion
}SET_PIXEL_CLOCK_PS_ALLOCATION;



#define ENABLE_VGA_RENDER_PS_ALLOCATION   SET_PIXEL_CLOCK_PS_