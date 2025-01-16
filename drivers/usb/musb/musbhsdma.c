3


// ucInputFlag
#define ATOM_PLL_INPUT_FLAG_PLL_STROBE_MODE_EN  1   // 1-StrobeMode, 0-PerformanceMode

// use for ComputeMemoryClockParamTable
typedef struct _COMPUTE_MEMORY_CLOCK_PARAM_PARAMETERS_V2_1
{
  union
  {
    ULONG  ulClock;
    ATOM_S_MPLL_FB_DIVIDER   ulFbDiv;         //Output:UPPER_WORD=FB_DIV_INTEGER,  LOWER_WORD=FB_DIV_FRAC shl (16-FB_FRACTION_BITS)
  };
  UCHAR   ucDllSpeed;                         //Output
  UCHAR   ucPostDiv;                          //Output
  union{
    UCHAR   ucInputFlag;                      //Input : ATOM_PLL_INPUT_FLAG_PLL_STROBE_MODE_EN: 1-StrobeMode, 0-PerformanceMode
    UCHAR   ucPllCntlFlag;                    //Output:
  };
  UCHAR   ucBWCntl;
}COMPUTE_MEMORY_CLOCK_PARAM_PARAMETERS_V2_1;

// definition of ucInputFlag
#define MPLL_INPUT_FLAG_STROBE_MODE_EN          0x01
// definition of ucPllCntlFlag
#define MPLL_CNTL_FLAG_VCO_MODE_MASK            0x03
#define MPLL_CNTL_FLAG_BYPASS_DQ_PLL            0x04
#define MPLL_CNTL_FLAG_QDR_ENABLE               0x08
#define MPLL_CNTL_FLAG_AD_HALF_RATE             0x10

//MPLL_CNTL_FLAG_BYPASS_AD_PLL has a wrong name, should be BYPASS_DQ_PLL
#define MPLL_CNTL_FLAG_BYPASS_AD_PLL            0x04

typedef struct _DYNAMICE_MEMORY_SETTINGS_PARAMETER
{
  ATOM_COMPUTE_CLOCK_FREQ ulClock;
  ULONG ulReserved[2];
}DYNAMICE_MEMORY_SETTINGS_PARAMETER;

typedef struct _DYNAMICE_ENGINE_SETTINGS_PARAMETER
{
  ATOM_COMPUTE_CLOCK_FREQ ulClock;
  ULONG ulMemoryClock;
  ULONG ulReserved;
}DYNAMICE_ENGINE_SETTINGS_PARAMETER;

/****************************************************************************/
// Structures used by SetEngineClockTable
/****************************************************************************/
typedef struct _SET_ENGINE_CLOCK_PARAMETERS
{
  ULONG ulTargetEngineClock;          //In 10Khz unit
}SET_ENGINE_CLOCK_PARAMETERS;

typedef struct _SET_ENGINE_CLOCK_PS_ALLOCATION
{
  ULONG ulTargetEngineClock;          //In 10Khz unit
  COMPUTE_MEMORY_ENGINE_PLL_PARAMETERS_PS_ALLOCATION sReserved;
}SET_ENGINE_CLOCK_PS_ALLOCATION;

/****************************************************************************/
// Structures used by SetMemoryClockTable
/****************************************************************************/
typedef struct _SET_MEMORY_CLOCK_PARAMETERS
{
  ULONG ulTargetMemoryClock;          //In 10Khz unit
}SET_MEMORY_CLOCK_PARAMETERS;

typedef struct _SET_MEMORY_CLOCK_PS_ALLOCATION
{
  ULONG ulTargetMemoryClock;          //In 10Khz unit
  COMPUTE_MEMORY_ENGINE_PLL_PARAMETERS_PS_ALLOCATION sReserved;
}SET_MEMORY_CLOCK_PS_ALLOCATION;

/****************************************************************************/
// Structures used by ASIC_Init.ctb
/****************************************************************************/
typedef struct _ASIC_INIT_PARAMETERS
{
  ULONG ulDefaultEngineClock;         //In 10Khz unit
  ULONG ulDefaultMemoryClock;         //In 10Khz unit
}ASIC_INIT_PARAMETERS;

typedef struct _ASIC_INIT_PS_ALLOCATION
{
  ASIC_INIT_PARAMETERS sASICInitClocks;
  SET_ENGINE_CLOCK_PS_ALLOCATION sReserved; //Caller doesn't need to init this structure
}ASIC_INIT_PS_ALLOCATION;

typedef struct _ASIC_INIT_CLOCK_PARAMETERS
{
  ULONG ulClkFreqIn10Khz:24;
  ULONG ucClkFlag:8;
}ASIC_INIT_CLOCK_PARAMETERS;

typedef struct _ASIC_INIT_PARAMETERS_V1_2
{
  ASIC_INIT_CLOCK_PARAMETERS asSclkClock;         //In 10Khz unit
  ASIC_INIT_CLOCK_PARAMETERS asMemClock;          //In 10Khz unit
}ASIC_INIT_PARAMETERS_V1_2;

typedef struct _ASIC_INIT_PS_ALLOCATION_V1_2
{
  ASIC_INIT_PARAMETERS_V1_2 sASICInitClocks;
  ULONG ulReserved[8];
}ASIC_INIT_PS_ALLOCATION_V1_2;

/****************************************************************************/
// Structure used by DynamicClockGatingTable.ctb
/****************************************************************************/
typedef struct _DYNAMIC_CLOCK_GATING_PARAMETERS
{
  UCHAR ucEnable;                     // ATOM_ENABLE or ATOM_DISABLE
  UCHAR ucPadding[3];
}DYNAMIC_CLOCK_GATING_PARAMETERS;
#define  DYNAMIC_CLOCK_GATING_PS_ALLOCATION  DYNAMIC_CLOCK_GATING_PARAMETERS

/****************************************************************************/
// Structure used by EnableDispPowerGatingTable.ctb
/****************************************************************************/
typedef struct _ENABLE_DISP_POWER_GATING_PARAMETERS_V2_1
{
  UCHAR ucDispPipeId;                 // ATOM_CRTC1, ATOM_CRTC2, ...
  UCHAR ucEnable;                     // ATOM_ENABLE or ATOM_DISABLE
  UCHAR ucPadding[2];
}ENABLE_DISP_POWER_GATING_PARAMETERS_V2_1;

typedef struct _ENABLE_DISP_POWER_GATING_PS_ALLOCATION
{
  UCHAR ucDispPipeId;                 // ATOM_CRTC1, ATOM_CRTC2, ...
  UCHAR ucEnable;                     // ATOM_ENABLE/ATOM_DISABLE/ATOM_INIT
  UCHAR ucPadding[2];
  ULONG ulReserved[4];
}ENABLE_DISP_POWER_GATING_PS_ALLOCATION;

/****************************************************************************/
// Structure used by EnableASIC_StaticPwrMgtTable.ctb
/****************************************************************************/
typedef struct _ENABLE_ASIC_STATIC_PWR_MGT_PARAMETERS
{
  UCHAR ucEnable;                     // ATOM_ENABLE or ATOM_DISABLE
  UCHAR ucPadding[3];
}ENABLE_ASIC_STATIC_PWR_MGT_PARAMETERS;
#define ENABLE_ASIC_STATIC_PWR_MGT_PS_ALLOCATION  ENABLE_ASIC_STATIC_PWR_MGT_PARAMETERS

/****************************************************************************/
// Structures used by DAC_LoadDetectionTable.ctb
/****************************************************************************/
typedef struct _DAC_LOAD_DETECTION_PARAMETERS
{
  USHORT usDeviceID;                  //{ATOM_DEVICE_CRTx_SUPPORT,ATOM_DEVICE_TVx_SUPPORT,ATOM_DEVICE_CVx_SUPPORT}
  UCHAR  ucDacType;                   //{ATOM_DAC_A,ATOM_DAC_B, ATOM_EXT_DAC}
  UCHAR  ucMisc;                                 //Valid only when table revision =1.3 and above
}DAC_LOAD_DETECTION_PARAMETERS;

// DAC_LOAD_DETECTION_PARAMETERS.ucMisc
#define DAC_LOAD_MISC_YPrPb                  0x01

typedef struct _DAC_LOAD_DETECTION_PS_ALLOCATION
{
  DAC_LOAD_DETECTION_PARAMETERS            sDacload;
  ULONG                                    Reserved[2];// Don't set this one, allocation for EXT DAC
}DAC_LOAD_DETECTION_PS_ALLOCATION;

/****************************************************************************/
// Structures used by DAC1EncoderControlTable.ctb and DAC2EncoderControlTable.ctb
/****************************************************************************/
typedef struct _DAC_ENCODER_CONTROL_PARAMETERS
{
  USHORT usPixelClock;                // in 10KHz; for bios convenient
  UCHAR  ucDacStandard;               // See definition of ATOM_DACx_xxx, For DEC3.0, bit 7 used as internal flag to indicate DAC2 (==1) or DAC1 (==0)
  UCHAR  ucAction;                    // 0: turn off encoder
                                      // 1: setup and turn on encoder
                                      // 7: ATOM_ENCODER_INIT Initialize DAC
}DAC_ENCODER_CONTROL_PARAMETERS;

#define DAC_ENCODER_CONTROL_PS_ALLOCATION  DAC_ENCODER_CONTROL_PARAMETERS

/****************************************************************************/
// Structures used by DIG1EncoderControlTable
//                    DIG2EncoderControlTable
//                    ExternalEncoderControlTable
/****************************************************************************/
typedef struct _DIG_ENCODER_CONTROL_PARAMETERS
{
  USHORT usPixelClock;      // in 10KHz; for bios convenient
  UCHAR  ucConfig;
                            // [2] Link Select:
                            // =0: PHY linkA if bfLane<3
                            // =1: PHY linkB if bfLanes<3
                            // =0: PHY linkA+B if bfLanes=3
                            // [3] Transmitter Sel
                            // =0: UNIPHY or PCIEPHY
                            // =1: LVTMA
  UCHAR ucAction;           // =0: turn off encoder
                            // =1: turn on encoder
  UCHAR ucEncoderMode;
                            // =0: DP   encoder
                            // =1: LVDS encoder
                            // =2: DVI  encoder
                            // =3: HDMI encoder
                            // =4: SDVO encoder
  UCHAR ucLaneNum;          // how many lanes to enable
  UCHAR ucReserved[2];
}DIG_ENCODER_CONTROL_PARAMETERS;
#define DIG_ENCODER_CONTROL_PS_ALLOCATION             DIG_ENCODER_CONTROL_PARAMETERS
#define EXTERNAL_ENCODER_CONTROL_PARAMETER            DIG_ENCODER_CONTROL_PARAMETERS

//ucConfig
#define ATOM_ENCODER_CONFIG_DPLINKRATE_MASK           0x01
#define ATOM_ENCODER_CONFIG_DPLINKRATE_1_62GHZ        0x00
#define ATOM_ENCODER_CONFIG_DPLINKRATE_2_70GHZ        0x01
#define ATOM_ENCODER_CONFIG_DPLINKRATE_5_40GHZ        0x02
#define ATOM_ENCODER_CONFIG_LINK_SEL_MASK             0x04
#define ATOM_ENCODER_CONFIG_LINKA                     0x00
#define ATOM_ENCODER_CONFIG_LINKB                     0x04
#define ATOM_ENCODER_CONFIG_LINKA_B                   ATOM_TRANSMITTER_CONFIG_LINKA
#define ATOM_ENCODER_CONFIG_LINKB_A                   ATOM_ENCODER_CONFIG_LINKB
#define ATOM_ENCODER_CONFIG_TRANSMITTER_SEL_MASK      0x08
#define ATOM_ENCODER_CONFIG_UNIPHY                    0x00
#define ATOM_ENCODER_CONFIG_LVTMA                     0x08
#define ATOM_ENCODER_CONFIG_TRANSMITTER1              0x00
#define ATOM_ENCODER_CONFIG_TRANSMITTER2              0x08
#define ATOM_ENCODER_CONFIG_DIGB                      0x80         // VBIOS Internal use, outside SW should set this bit=0
// ucAction
// ATOM_ENABLE:  Enable Encoder
// ATOM_DISABLE: Disable Encoder

//ucEncoderMode
#define ATOM_ENCODER_MODE_DP                          0
#define ATOM_ENCODER_MODE_LVDS                        1
#define ATOM_ENCODER_MODE_DVI                         2
#define ATOM_ENCODER_MODE_HDMI                        3
#define ATOM_ENCODER_MODE_SDVO                        4
#define ATOM_ENCODER_MODE_DP_AUDIO                    5
#define ATOM_ENCODER_MODE_TV                          13
#define ATOM_ENCODER_MODE_CV                          14
#define ATOM_ENCODER_MODE_CRT                         15
#define ATOM_ENCODER_MODE_DVO                         16
#define ATOM_ENCODER_MODE_DP_SST                      ATOM_ENCODER_MODE_DP    // For DP1.2
#define ATOM_ENCODER_MODE_DP_MST                      5                       // For DP1.2


typedef struct _ATOM_DIG_ENCODER_CONFIG_V2
{
#if ATOM_BIG_ENDIAN
    UCHAR ucReserved1:2;
    UCHAR ucTransmitterSel:2;     // =0: UniphyAB, =1: UniphyCD  =2: UniphyEF
    UCHAR ucLinkSel:1;            // =0: linkA/C/E =1: linkB/D/F
    UCHAR ucReserved:1;
    UCHAR ucDPLinkRate:1;         // =0: 1.62Ghz, =1: 2.7Ghz
#else
    UCHAR ucDPLinkRate:1;         // =0: 1.62Ghz, =1: 2.7Ghz
    UCHAR ucReserved:1;
    UCHAR ucLinkSel:1;            // =0: linkA/C/E =1: linkB/D/F
    UCHAR ucTransmitterSel:2;     // =0: UniphyAB, =1: UniphyCD  =2: UniphyEF
    UCHAR ucReserved1:2;
#endif
}ATOM_DIG_ENCODER_CONFIG_V2;


typedef struct _DIG_ENCODER_CONTROL_PARAMETERS_V2
{
  USHORT usPixelClock;      // in 10KHz; for bios convenient
  ATOM_DIG_ENCODER_CONFIG_V2 acConfig;
  UCHAR ucAction;
  UCHAR ucEncoderMode;
                            // =0: DP   encoder
                            // =1: LVDS encoder
                            // =2: DVI  encoder
                            // =3: HDMI encoder
                            // =4: SDVO encoder
  UCHAR ucLaneNum;          // how many lanes to enable
  UCHAR ucStatus;           // = DP_LINK_TRAINING_COMPLETE or DP_LINK_TRAINING_INCOMPLETE, only used by VBIOS with command ATOM_ENCODER_CMD_QUERY_DP_LINK_TRAINING_STATUS
  UCHAR ucReserved;
}DIG_ENCODER_CONTROL_PARAMETERS_V2;

//ucConfig
#define ATOM_ENCODER_CONFIG_V2_DPLINKRATE_MASK            0x01
#define ATOM_ENCODER_CONFIG_V2_DPLINKRATE_1_62GHZ        0x00
#define ATOM_ENCODER_CONFIG_V2_DPLINKRATE_2