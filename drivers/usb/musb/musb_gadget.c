YTE_HW_I2C_DATA_PS_ALLOCATION   sReserved;     //Caller doesn't need to init this portion
}ENABLE_EXTERNAL_TMDS_ENCODER_PS_ALLOCATION;

#define ENABLE_EXTERNAL_TMDS_ENCODER_PARAMETERS_V2  LVDS_ENCODER_CONTROL_PARAMETERS_V2
typedef struct _ENABLE_EXTERNAL_TMDS_ENCODER_PS_ALLOCATION_V2
{
  ENABLE_EXTERNAL_TMDS_ENCODER_PARAMETERS_V2    sXTmdsEncoder;
  WRITE_ONE_BYTE_HW_I2C_DATA_PS_ALLOCATION      sReserved;     //Caller doesn't need to init this portion
}ENABLE_EXTERNAL_TMDS_ENCODER_PS_ALLOCATION_V2;

typedef struct _EXTERNAL_ENCODER_CONTROL_PS_ALLOCATION
{
  DIG_ENCODER_CONTROL_PARAMETERS            sDigEncoder;
  WRITE_ONE_BYTE_HW_I2C_DATA_PS_ALLOCATION sReserved;
}EXTERNAL_ENCODER_CONTROL_PS_ALLOCATION;

/****************************************************************************/
// Structures used by DVOEncoderControlTable
/****************************************************************************/
//ucTableFormatRevision=1,ucTableContentRevision=3
//ucDVOConfig:
#define DVO_ENCODER_CONFIG_RATE_SEL                     0x01
#define DVO_ENCODER_CONFIG_DDR_SPEED                  0x00
#define DVO_ENCODER_CONFIG_SDR_SPEED                  0x01
#define DVO_ENCODER_CONFIG_OUTPUT_SEL                  0x0c
#define DVO_ENCODER_CONFIG_LOW12BIT                     0x00
#define DVO_ENCODER_CONFIG_UPPER12BIT                  0x04
#define DVO_ENCODER_CONFIG_24BIT                        0x08

typedef struct _DVO_ENCODER_CONTROL_PARAMETERS_V3
{
  USHORT usPixelClock;
  UCHAR  ucDVOConfig;
  UCHAR  ucAction;                                          //ATOM_ENABLE/ATOM_DISABLE/ATOM_HPD_INIT
  UCHAR  ucReseved[4];
}DVO_ENCODER_CONTROL_PARAMETERS_V3;
#define DVO_ENCODER_CONTROL_PS_ALLOCATION_V3   DVO_ENCODER_CONTROL_PARAMETERS_V3

typedef struct _DVO_ENCODER_CONTROL_PARAMETERS_V1_4
{
  USHORT usPixelClock;
  UCHAR  ucDVOConfig;
  UCHAR  ucAction;                                          //ATOM_ENABLE/ATOM_DISABLE/ATOM_HPD_INIT
  UCHAR  ucBitPerColor;                       //please refer to definition of PANEL_xBIT_PER_COLOR
  UCHAR  ucReseved[3];
}DVO_ENCODER_CONTROL_PARAMETERS_V1_4;
#define DVO_ENCODER_CONTROL_PS_ALLOCATION_V1_4   DVO_ENCODER_CONTROL_PARAMETERS_V1_4


//ucTableFormatRevision=1
//ucTableContentRevision=3 structure is not changed but usMisc add bit 1 as another input for
// bit1=0: non-coherent mode
//     =1: coherent mode

//==========================================================================================
//Only change is here next time when changing encoder parameter definitions again!
#define LVDS_ENCODER_CONTROL_PARAMETERS_LAST     LVDS_ENCODER_CONTROL_PARAMETERS_V3
#define LVDS_ENCODER_CONTROL_PS_ALLOCATION_LAST  LVDS_ENCODER_CONTROL_PARAMETERS_LAST

#define TMDS1_ENCODER_CONTROL_PARAMETERS_LAST    LVDS_ENCODER_CONTROL_PARAMETERS_V3
#define TMDS1_ENCODER_CONTROL_PS_ALLOCATION_LAST TMDS1_ENCODER_CONTROL_PARAMETERS_LAST

#define TMDS2_ENCODER_CONTROL_PARAMETERS_LAST    LVDS_ENCODER_CONTROL_PARAMETERS_V3
#define TMDS2_ENCODER_CONTROL_PS_ALLOCATION_LAST TMDS2_ENCODER_CONTROL_PARAMETERS_LAST

#define DVO_ENCODER_CONTROL_PARAMETERS_LAST      DVO_ENCODER_CONTROL_PARAMETERS
#define DVO_ENCODER_CONTROL_PS_ALLOCATION_LAST   DVO_ENCODER_CONTROL_PS_ALLOCATION

//==========================================================================================
#define PANEL_ENCODER_MISC_DUAL                0x01
#define PANEL_ENCODER_MISC_COHERENT            0x02
#define   PANEL_ENCODER_MISC_TMDS_LINKB                0x04
#define   PANEL_ENCODER_MISC_HDMI_TYPE                0x08

#define PANEL_ENCODER_ACTION_DISABLE           ATOM_DISABLE
#define PANEL_ENCODER_ACTION_ENABLE            ATOM_ENABLE
#define PANEL_ENCODER_ACTION_COHERENTSEQ       (ATOM_ENABLE+1)

#define PANEL_ENCODER_TRUNCATE_EN              0x01
#define PANEL_ENCODER_TRUNCATE_DEPTH           0x10
#define PANEL_ENCODER_SPATIAL_DITHER_EN        0x01
#define PANEL_ENCODER_SPATIAL_DITHER_DEPTH     0x10
#define PANEL_ENCODER_TEMPORAL_DITHER_EN       0x01
#define PANEL_ENCODER_TEMPORAL_DITHER_DEPTH    0x10
#define PANEL_ENCODER_TEMPORAL_LEVEL_4         0x20
#define PANEL_ENCODER_25FRC_MASK               0x10
#define PANEL_ENCODER_25FRC_E                  0x00
#define PANEL_ENCODER_25FRC_F                  0x10
#define PANEL_ENCODER_50FRC_MASK               0x60
#define PANEL_ENCODER_50FRC_A                  0x00
#define PANEL_ENCODER_50FRC_B                  0x20
#define PANEL_ENCODER_50FRC_C                  0x40
#define PANEL_ENCODER_50FRC_D                  0x60
#define PANEL_ENCODER_75FRC_MASK               0x80
#define PANEL_ENCODER_75FRC_E                  0x00
#define PANEL_ENCODER_75FRC_F                  0x80

/****************************************************************************/
// Structures used by SetVoltageTable
/****************************************************************************/
#define SET_VOLTAGE_TYPE_ASIC_VDDC             1
#define SET_VOLTAGE_TYPE_ASIC_MVDDC            2
#define SET_VOLTAGE_TYPE_ASIC_MVDDQ            3
#define SET_VOLTAGE_TYPE_ASIC_VDDCI            4
#define SET_VOLTAGE_INIT_MODE                  5
#define SET_VOLTAGE_GET_MAX_VOLTAGE            6               //Gets the Max. voltage for the soldered Asic

#define SET_ASIC_VOLTAGE_MODE_ALL_SOURCE       0x1
#define SET_ASIC_VOLTAGE_MODE_SOURCE_A         0x2
#define SET_ASIC_VOLTAGE_MODE_SOURCE_B         0x4

#define   SET_ASIC_VOLTAGE_MODE_SET_VOLTAGE      0x0
#define   SET_ASIC_VOLTAGE_MODE_GET_GPIOVAL      0x1
#define   SET_ASIC_VOLTAGE_MODE_GET_GPIOMASK     0x2

typedef struct   _SET_VOLTAGE_PARAMETERS
{
  UCHAR    ucVoltageType;               // To tell which voltage to set up, VDDC/MVDDC/MVDDQ
  UCHAR    ucVoltageMode;               // To set all, to set source A or source B or ...
  UCHAR    ucVoltageIndex;              // An index to tell which voltage level
  UCHAR    ucReserved;
}SET_VOLTAGE_PARAMETERS;

typedef struct   _SET_VOLTAGE_PARAMETERS_V2
{
  UCHAR    ucVoltageType;               // To tell which voltage to set up, VDDC/MVDDC/MVDDQ
  UCHAR    ucVoltageMode;               // Not used, maybe use for state machine for differen power mode
  USHORT   usVoltageLevel;              // real voltage level
}SET_VOLTAGE_PARAMETERS_V2;

// used by both SetVoltageTable v1.3 and v1.4
typedef struct   _SET_VOLTAGE_PARAMETERS_V1_3
{
  UCHAR    ucVoltageType;               // To tell which voltage to set up, VDDC/MVDDC/MVDDQ/VDDCI
  UCHAR    ucVoltageMode;               // Indicate action: Set voltage level
  USHORT   usVoltageLevel;              // real voltage level in unit of mv or Voltage Phase (0, 1, 2, .. )
}SET_VOLTAGE_PARAMETERS_V1_3;

//ucVoltageType
#define VOLTAGE_TYPE_VDDC                    1
#define VOLTAGE_TYPE_MVDDC                   2
#define VOLTAGE_TYPE_MVDDQ                   3
#define VOLTAGE_TYPE_VDDCI                   4
#define VOLTAGE_TYPE_VDDGFX                  5
#define VOLTAGE_TYPE_PCC                     6

#define VOLTAGE_TYPE_GENERIC_I2C_1           0x11
#define VOLTAGE_TYPE_GENERIC_I2C_2           0x12
#define VOLTAGE_TYPE_GENERIC_I2C_3           0x13
#define VOLTAGE_TYPE_GENERIC_I2C_4           0x14
#define VOLTAGE_TYPE_GENERIC_I2C_5           0x15
#define VOLTAGE_TYPE_GENERIC_I2C_6           0x16
#define VOLTAGE_TYPE_GENERIC_I2C_7           0x17
#define VOLTAGE_TYPE_GENERIC_I2C_8           0x18
#define VOLTAGE_TYPE_GENERIC_I2C_9           0x19
#define VOLTAGE_TYPE_GENERIC_I2C_10          0x1A

//SET_VOLTAGE_PARAMETERS_V3.ucVoltageMode
#define ATOM_SET_VOLTAGE                     0        //Set voltage Level
#define ATOM_INIT_VOLTAGE_REGULATOR          3        //Init Regulator
#define ATOM_SET_VOLTAGE_PHASE               4        //Set Vregulator Phase, only for SVID/PVID regulator
#define ATOM_GET_MAX_VOLTAGE                 6        //Get Max Voltage, not used from SetVoltageTable v1.3
#define ATOM_GET_VOLTAGE_LEVEL               6        //Get Voltage level from vitual voltage ID, not used for SetVoltage v1.4
#define ATOM_GET_LEAKAGE_ID                  8        //Get Leakage Voltage Id ( starting from SMU7x IP ), SetVoltage v1.4

// define vitual voltage id in usVoltageLevel
#define ATOM_VIRTUAL_VOLTAGE_ID0             0xff01
#define ATOM_VIRTUAL_VOLTAGE_ID1             0xff02
#define ATOM_VIRTUAL_VOLTAGE_ID2             0xff03
#define ATOM_VIRTUAL_VOLTAGE_ID3             0xff04
#define ATOM_VIRTUAL_VOLTAGE_ID4             0xff05
#define ATOM_VIRTUAL_VOLTAGE_ID5             0xff06
#define ATOM_VIRTUAL_VOLTAGE_ID6             0xff07
#define ATOM_VIRTUAL_VOLTAGE_ID7             0xff08

typedef struct _SET_VOLTAGE_PS_ALLOCATION
{
  SET_VOLTAGE_PARAMETERS sASICSetVoltage;
  WRITE_ONE_BYTE_HW_I2C_DATA_PS_ALLOCATION sReserved;
}SET_VOLTAGE_PS_ALLOCATION;

// New Added from SI for GetVoltageInfoTable, input parameter structure
typedef struct  _GET_VOLTAGE_INFO_INPUT_PARAMETER_V1_1
{
  UCHAR    ucVoltageType;               // Input: To tell which voltage to set up, VDDC/MVDDC/MVDDQ/VDDCI
  UCHAR    ucVoltageMode;               // Input: Indicate action: Get voltage info
  USHORT   usVoltageLevel;              // Input: real voltage level in unit of mv or Voltage Phase (0, 1, 2, .. ) or Leakage Id
  ULONG    ulReserved;
}GET_VOLTAGE_INFO_INPUT_PARAMETER_V1_1;

// New Added from SI for GetVoltageInfoTable, output parameter structure when ucVotlageMode == ATOM_GET_VOLTAGE_VID
typedef struct  _GET_VOLTAGE_INFO_OUTPUT_PARAMETER_V1_1
{
  ULONG    ulVotlageGpioState;
  ULONG    ulVoltageGPioMask;
}GET_VOLTAGE_INFO_OUTPUT_PARAMETER_V1_1;

// New Added from SI for GetVoltageInfoTable, output parameter structure when ucVotlageMode == ATOM_GET_VOLTAGE_STATEx_LEAKAGE_VID
typedef struct  _GET_LEAKAGE_VOLTAGE_INFO_OUTPUT_PARAMETER_V1_1
{
  USHORT   usVoltageLevel;
  USHORT   usVoltageId;                                  // Voltage Id programmed in Voltage Regulator
  ULONG    ulReseved;
}GET_LEAKAGE_VOLTAGE_INFO_OUTPUT_PARAMETER_V1_1;

// GetVoltageInfo v1.1 ucVoltageMode
#define ATOM_GET_VOLTAGE_VID                0x00
#define ATOM_GET_VOTLAGE_INIT_SEQ           0x03
#define ATOM_GET_VOLTTAGE_PHASE_PHASE_VID   0x04
#define ATOM_GET_VOLTAGE_SVID2              0x07        //Get SVI2 Regulator Info

// for SI, this state map to 0xff02 voltage state in Power Play table, which is power boost state
#define   ATOM_GET_VOLTAGE_STATE0_LEAKAGE_VID 0x10
// for SI, this state map to 0xff01 voltage state in Power Play table, which is performance state
#define   ATOM_GET_VOLTAGE_STATE1_LEAKAGE_VID 0x11

#define   ATOM_GET_VOLTAGE_STATE2_LEAKAGE_VID 0x12
#define   ATOM_GET_VOLTAGE_STATE3_LEAKAGE_VID 0x13


// New Added from CI Hawaii for GetVoltageInfoTable, input parameter structure
typedef struct  _GET_VOLTAGE_INFO_INPUT_PARAMETER_V1_2
{
  UCHAR    ucVoltageType;               // Input: To tell which voltage to set up, VDDC/MVDDC/MVDDQ/VDDCI
  UCHAR    ucVoltageMode;               // Input: Indicate action: Get voltage info
  USHORT   usVoltageLevel;              // Input: real voltage level in unit of mv or Voltage Phase (0, 1, 2, .. ) or Leakage Id
  ULONG    ulSCLKFreq;                  // Input: when ucVoltageMode= ATOM_GET_VOLTAGE_EVV_VOLTAGE, DPM state SCLK frequency, Define in PPTable SCLK/Voltage dependence table
}GET_VOLTAGE_INFO_INPUT_PARAMETER_V1_2;

// New in GetVoltageInfo v1.2 ucVoltageMode
#define ATOM_GET_VOLTAGE_EVV_VOLTAGE        0x09

// New Added from CI Hawaii for EVV feature
typedef struct  _GET_EVV_VOLTAGE_INFO_OUTPUT_PARAMETER_V1_2
{
  USHORT   usVoltageLevel;                               // real voltage level in unit of mv
  USHORT   usVoltageId;                                  // Voltage Id programmed in Voltage Regulator
  USHORT   usTDP_Current;                                // TDP_Current in unit of  0.01A
  USHORT   usTDP_Power;                                  // TDP_Current in unit  of 0.1W
}GET_EVV_VOLTAGE_INFO_OUTPUT_PARAMETER_V1_2;

/****************************************************************************/
// Structures used by TVEncoderControlTable
/****************************************************************************/
typedef struct _TV_ENCODER_CONTROL_PARAMETERS
{
  USHORT usPixelClock;                // in 10KHz; for bios convenient
  UCHAR  ucTvStandard;                // See definition "ATOM_TV_NTSC ..."
  UCHAR  ucAction;                    // 0: turn off encoder
                                      // 1: setup and turn on encoder
}TV_ENCODER_CONTROL_PARAMETERS;

typedef struct _TV_ENCODER_CONTROL_PS_ALLOCATION
{
  TV_ENCODER_CONTROL_PARAMETERS sTVEncoder;
  WRITE_ONE_BYTE_HW_I2C_DATA_PS_ALLOCATION    sReserved; // Don't set this one
}TV_ENCODER_CONTROL_PS_ALLOCATION;

//==============================Data Table Portion====================================


/****************************************************************************/
// Structure used in Data.mtb
/****************************************************************************/
typedef struct _ATOM_MASTER_LIST_OF_DATA_TABLES
{
  USHORT        UtilityPipeLine;          // Offest for the utility to get parser info,Don't change this position!
  USHORT        MultimediaCapabilityInfo; // Only used by MM Lib,latest version 1.1, not configuable from Bios, need to include the table to build Bios
  USHORT        MultimediaConfigInfo;     // Only used by MM Lib,latest version 2.1, not configuable from Bios, need to include the table to build Bios
  USHORT        StandardVESA_Timing;      // Only used by Bios
  USHORT        FirmwareInfo;             // Shared by various SW components,latest version 1.4
  USHORT        PaletteData;              // Only used by BIOS
  USHORT        LCD_Info;                 // Shared by various SW components,latest version 1.3, was called LVDS_Info
  USHORT        DIGTransmitterInfo;       // Internal used by VBIOS only version 3.1
  USHORT        AnalogTV_Info;            // Shared by various SW components,latest version 1.1
  USHORT        SupportedDevicesInfo;     // Will be obsolete from R600
  USHORT        GPIO_I2C_Info;            // Shared by various SW components,latest version 1.2 will be used from R600
  USHORT        VRAM_UsageByFirmware;     // Shared by various SW components,latest version 1.3 will be used from R600
  USHORT        GPIO_Pin_LUT;             // Shared by various SW components,latest version 1.1
  USHORT        VESA_ToInternalModeLUT;   // Only used by Bios
  USHORT        ComponentVideoInfo;       // Shared by various SW components,latest version 2.1 will be used from R600
  USHORT        PowerPlayInfo;            // Shared by various SW components,latest version 2.1,new design from R600
  USHORT        GPUVirtualizationInfo;    // Will be obsolete from R600
  USHORT        SaveRestoreInfo;          // Only used by Bios
  USHORT        PPLL_SS_Info;             // Shared by various SW components,latest version 1.2, used to call SS_Info, change to new name because of int ASIC SS info
  USHORT        OemInfo;                  // Defined and used by external SW, should be obsolete soon
  USHORT        XTMDS_Info;               // Will be obsolete from R600
  USHORT        MclkSS_Info;              // Shared by various SW components,latest version 1.1, only enabled when ext SS chip is used
  USHORT        Object_Header;            // Shared by various SW components,latest version 1.1
  USHORT        IndirectIOAccess;         // Only used by Bios,this table position can't change at all!!
  USHORT        MC_InitParameter;         // Only used by command table
  USHORT        ASIC_VDDC_Info;           // Will be obsolete from R600
  USHORT        ASIC_InternalSS_Info;     // New tabel name from R600, used to be called "ASIC_MVDDC_Info"
  USHORT        TV_VideoMode;             // Only used by command table
  USHORT        VRAM_Info;                // Only used by command table, latest version 1.3
  USHORT        MemoryTrainingInfo;       // Used for VBIOS and Diag utility for memory training purpose since R600. the new table rev start from 2.1
  USHORT        IntegratedSystemInfo;     // Shared by various SW components
  USHORT        ASIC_ProfilingInfo;       // New table name from R600, used to be called "ASIC_VDDCI_Info" for pre-R600
  USHORT        VoltageObjectInfo;        // Shared by various SW components, latest version 1.1
  USHORT        PowerSourceInfo;          // Shared by various SW components, latest versoin 1.1
  USHORT	      ServiceInfo;
}ATOM_MASTER_LIST_OF_DATA_TABLES;

typedef struct _ATOM_MASTER_DATA_TABLE
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  ATOM_MASTER_LIST_OF_DATA_TABLES   ListOfDataTables;
}ATOM_MASTER_DATA_TABLE;

// For backward compatible
#define LVDS_Info                LCD_Info
#define DAC_Info                 PaletteData
#define TMDS_Info                DIGTransmitterInfo
#define CompassionateData        GPUVirtualizationInfo

/****************************************************************************/
// Structure used in MultimediaCapabilityInfoTable
/****************************************************************************/
typedef struct _ATOM_MULTIMEDIA_CAPABILITY_INFO
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  ULONG                    ulSignature;      // HW info table signature string "$ATI"
  UCHAR                    ucI2C_Type;       // I2C type (normal GP_IO, ImpactTV GP_IO, Dedicated I2C pin, etc)
  UCHAR                    ucTV_OutInfo;     // Type of TV out supported (3:0) and video out crystal frequency (6:4) and TV data port (7)
  UCHAR                    ucVideoPortInfo;  // Provides the video port capabilities
  UCHAR                    ucHostPortInfo;   // Provides host port configuration information
}ATOM_MULTIMEDIA_CAPABILITY_INFO;


/****************************************************************************/
// Structure used in MultimediaConfigInfoTable
/****************************************************************************/
typedef struct _ATOM_MULTIMEDIA_CONFIG_INFO
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  ULONG                    ulSignature;      // MM info table signature sting "$MMT"
  UCHAR                    ucTunerInfo;      // Type of tuner installed on the adapter (4:0) and video input for tuner (7:5)
  UCHAR                    ucAudioChipInfo;  // List the audio chip type (3:0) product type (4) and OEM revision (7:5)
  UCHAR                    ucProductID;      // Defines as OEM ID or ATI board ID dependent on product type setting
  UCHAR                    ucMiscInfo1;      // Tuner voltage (1:0) HW teletext support (3:2) FM audio decoder (5:4) reserved (6) audio scrambling (7)
  UCHAR                    ucMiscInfo2;      // I2S input config (0) I2S output config (1) I2S Audio Chip (4:2) SPDIF Output Config (5) reserved (7:6)
  UCHAR                    ucMiscInfo3;      // Video Decoder Type (3:0) Video In Standard/Crystal (7:4)
  UCHAR                    ucMiscInfo4;      // Video Decoder Host Config (2:0) reserved (7:3)
  UCHAR                    ucVideoInput0Info;// Video Input 0 Type (1:0) F/B setting (2) physical connector ID (5:3) reserved (7:6)
  UCHAR                    ucVideoInput1Info;// Video Input 1 Type (1:0) F/B setting (2) physical connector ID (5:3) reserved (7:6)
  UCHAR                    ucVideoInput2Info;// Video Input 2 Type (1:0) F/B setting (2) physical connector ID (5:3) reserved (7:6)
  UCHAR                    ucVideoInput3Info;// Video Input 3 Type (1:0) F/B setting (2) physical connector ID (5:3) reserved (7:6)
  UCHAR                    ucVideoInput4Info;// Video Input 4 Type (1:0) F/B setting (2) physical connector ID (5:3) reserved (7:6)
}ATOM_MULTIMEDIA_CONFIG_INFO;


/****************************************************************************/
// Structures used in FirmwareInfoTable
/****************************************************************************/

// usBIOSCapability Defintion:
// Bit 0 = 0: Bios image is not Posted, =1:Bios image is Posted;
// Bit 1 = 0: Dual CRTC is not supported, =1: Dual CRTC is supported;
// Bit 2 = 0: Extended Desktop is not supported, =1: Extended Desktop is supported;
// Others: Reserved
#define ATOM_BIOS_INFO_ATOM_FIRMWARE_POSTED         0x0001
#define ATOM_BIOS_INFO_DUAL_CRTC_SUPPORT            0x0002
#define ATOM_BIOS_INFO_EXTENDED_DESKTOP_SUPPORT     0x0004
#define ATOM_BIOS_INFO_MEMORY_CLOCK_SS_SUPPORT      0x0008      // (valid from v1.1 ~v1.4):=1: memclk SS enable, =0 memclk SS disable.
#define ATOM_BIOS_INFO_ENGINE_CLOCK_SS_SUPPORT      0x0010      // (valid from v1.1 ~v1.4):=1: engclk SS enable, =0 engclk SS disable.
#define ATOM_BIOS_INFO_BL_CONTROLLED_BY_GPU         0x0020
#define ATOM_BIOS_INFO_WMI_SUPPORT                  0x0040
#define ATOM_BIOS_INFO_PPMODE_ASSIGNGED_BY_SYSTEM   0x0080
#define ATOM_BIOS_INFO_HYPERMEMORY_SUPPORT          0x0100
#define ATOM_BIOS_INFO_HYPERMEMORY_SIZE_MASK        0x1E00
#define ATOM_BIOS_INFO_VPOST_WITHOUT_FIRST_MODE_SET 0x2000
#define ATOM_BIOS_INFO_BIOS_SCRATCH6_SCL2_REDEFINE  0x4000
#define ATOM_BIOS_INFO_MEMORY_CLOCK_EXT_SS_SUPPORT  0x0008      // (valid from v2.1 ): =1: memclk ss enable with external ss chip
#define ATOM_BIOS_INFO_ENGINE_CLOCK_EXT_SS_SUPPORT  0x0010      // (valid from v2.1 ): =1: engclk ss enable with external ss chip


#ifndef _H2INC

//Please don't add or expand this bitfield structure below, this one will retire soon.!
typedef struct _ATOM_FIRMWARE_CAPABILITY
{
#if ATOM_BIG_ENDIAN
  USHORT Reserved:1;
  USHORT SCL2Redefined:1;
  USHORT PostWithoutModeSet:1;
  USHORT HyperMemory_Size:4;
  USHORT HyperMemory_Support:1;
  USHORT PPMode_Assigned:1;
  USHORT WMI_SUPPORT:1;
  USHORT GPUControlsBL:1;
  USHORT EngineClockSS_Support:1;
  USHORT MemoryClockSS_Support:1;
  USHORT ExtendedDesktopSupport:1;
  USHORT DualCRTC_Support:1;
  USHORT FirmwarePosted:1;
#else
  USHORT FirmwarePosted:1;
  USHORT DualCRTC_Support:1;
  USHORT ExtendedDesktopSupport:1;
  USHORT MemoryClockSS_Support:1;
  USHORT EngineClockSS_Support:1;
  USHORT GPUControlsBL:1;
  USHORT WMI_SUPPORT:1;
  USHORT PPMode_Assigned:1;
  USHORT HyperMemory_Support:1;
  USHORT HyperMemory_Size:4;
  USHORT PostWithoutModeSet:1;
  USHORT SCL2Redefined:1;
  USHORT Reserved:1;
#endif
}ATOM_FIRMWARE_CAPABILITY;

typedef union _ATOM_FIRMWARE_CAPABILITY_ACCESS
{
  ATOM_FIRMWARE_CAPABILITY sbfAccess;
  USHORT                   susAccess;
}ATOM_FIRMWARE_CAPABILITY_ACCESS;

#else

typedef union _ATOM_FIRMWARE_CAPABILITY_ACCESS
{
  USHORT                   susAccess;
}ATOM_FIRMWARE_CAPABILITY_ACCESS;

#endif

typedef struct _ATOM_FIRMWARE_INFO
{
  ATOM_COMMON_TABLE_HEADER        sHeader;
  ULONG                           ulFirmwareRevision;
  ULONG                           ulDefaultEngineClock;       //In 10Khz unit
  ULONG                           ulDefaultMemoryClock;       //In 10Khz unit
  ULONG                           ulDriverTargetEngineClock;  //In 10Khz unit
  ULONG                           ulDriverTargetMemoryClock;  //In 10Khz unit
  ULONG                           ulMaxEngineClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxMemoryClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxPixelClockPLL_Output;  //In 10Khz unit
  ULONG                           ulASICMaxEngineClock;       //In 10Khz unit
  ULONG                           ulASICMaxMemoryClock;       //In 10Khz unit
  UCHAR                           ucASICMaxTemperature;
  UCHAR                           ucPadding[3];               //Don't use them
  ULONG                           aulReservedForBIOS[3];      //Don't use them
  USHORT                          usMinEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Output; //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Output; //In 10Khz unit
  USHORT                          usMaxPixelClock;            //In 10Khz unit, Max.  Pclk
  USHORT                          usMinPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMaxPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMinPixelClockPLL_Output;  //In 10Khz unit, the definitions above can't change!!!
  ATOM_FIRMWARE_CAPABILITY_ACCESS usFirmwareCapability;
  USHORT                          usReferenceClock;           //In 10Khz unit
  USHORT                          usPM_RTS_Location;          //RTS PM4 starting location in ROM in 1Kb unit
  UCHAR                           ucPM_RTS_StreamSize;        //RTS PM4 packets in Kb unit
  UCHAR                           ucDesign_ID;                //Indicate what is the board design
  UCHAR                           ucMemoryModule_ID;          //Indicate what is the board design
}ATOM_FIRMWARE_INFO;

typedef struct _ATOM_FIRMWARE_INFO_V1_2
{
  ATOM_COMMON_TABLE_HEADER        sHeader;
  ULONG                           ulFirmwareRevision;
  ULONG                           ulDefaultEngineClock;       //In 10Khz unit
  ULONG                           ulDefaultMemoryClock;       //In 10Khz unit
  ULONG                           ulDriverTargetEngineClock;  //In 10Khz unit
  ULONG                           ulDriverTargetMemoryClock;  //In 10Khz unit
  ULONG                           ulMaxEngineClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxMemoryClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxPixelClockPLL_Output;  //In 10Khz unit
  ULONG                           ulASICMaxEngineClock;       //In 10Khz unit
  ULONG                           ulASICMaxMemoryClock;       //In 10Khz unit
  UCHAR                           ucASICMaxTemperature;
  UCHAR                           ucMinAllowedBL_Level;
  UCHAR                           ucPadding[2];               //Don't use them
  ULONG                           aulReservedForBIOS[2];      //Don't use them
  ULONG                           ulMinPixelClockPLL_Output;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Output; //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Output; //In 10Khz unit
  USHORT                          usMaxPixelClock;            //In 10Khz unit, Max.  Pclk
  USHORT                          usMinPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMaxPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMinPixelClockPLL_Output;  //In 10Khz unit - lower 16bit of ulMinPixelClockPLL_Output
  ATOM_FIRMWARE_CAPABILITY_ACCESS usFirmwareCapability;
  USHORT                          usReferenceClock;           //In 10Khz unit
  USHORT                          usPM_RTS_Location;          //RTS PM4 starting location in ROM in 1Kb unit
  UCHAR                           ucPM_RTS_StreamSize;        //RTS PM4 packets in Kb unit
  UCHAR                           ucDesign_ID;                //Indicate what is the board design
  UCHAR                           ucMemoryModule_ID;          //Indicate what is the board design
}ATOM_FIRMWARE_INFO_V1_2;

typedef struct _ATOM_FIRMWARE_INFO_V1_3
{
  ATOM_COMMON_TABLE_HEADER        sHeader;
  ULONG                           ulFirmwareRevision;
  ULONG                           ulDefaultEngineClock;       //In 10Khz unit
  ULONG                           ulDefaultMemoryClock;       //In 10Khz unit
  ULONG                           ulDriverTargetEngineClock;  //In 10Khz unit
  ULONG                           ulDriverTargetMemoryClock;  //In 10Khz unit
  ULONG                           ulMaxEngineClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxMemoryClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxPixelClockPLL_Output;  //In 10Khz unit
  ULONG                           ulASICMaxEngineClock;       //In 10Khz unit
  ULONG                           ulASICMaxMemoryClock;       //In 10Khz unit
  UCHAR                           ucASICMaxTemperature;
  UCHAR                           ucMinAllowedBL_Level;
  UCHAR                           ucPadding[2];               //Don't use them
  ULONG                           aulReservedForBIOS;         //Don't use them
  ULONG                           ul3DAccelerationEngineClock;//In 10Khz unit
  ULONG                           ulMinPixelClockPLL_Output;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Output; //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Output; //In 10Khz unit
  USHORT                          usMaxPixelClock;            //In 10Khz unit, Max.  Pclk
  USHORT                          usMinPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMaxPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMinPixelClockPLL_Output;  //In 10Khz unit - lower 16bit of ulMinPixelClockPLL_Output
  ATOM_FIRMWARE_CAPABILITY_ACCESS usFirmwareCapability;
  USHORT                          usReferenceClock;           //In 10Khz unit
  USHORT                          usPM_RTS_Location;          //RTS PM4 starting location in ROM in 1Kb unit
  UCHAR                           ucPM_RTS_StreamSize;        //RTS PM4 packets in Kb unit
  UCHAR                           ucDesign_ID;                //Indicate what is the board design
  UCHAR                           ucMemoryModule_ID;          //Indicate what is the board design
}ATOM_FIRMWARE_INFO_V1_3;

typedef struct _ATOM_FIRMWARE_INFO_V1_4
{
  ATOM_COMMON_TABLE_HEADER        sHeader;
  ULONG                           ulFirmwareRevision;
  ULONG                           ulDefaultEngineClock;       //In 10Khz unit
  ULONG                           ulDefaultMemoryClock;       //In 10Khz unit
  ULONG                           ulDriverTargetEngineClock;  //In 10Khz unit
  ULONG                           ulDriverTargetMemoryClock;  //In 10Khz unit
  ULONG                           ulMaxEngineClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxMemoryClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxPixelClockPLL_Output;  //In 10Khz unit
  ULONG                           ulASICMaxEngineClock;       //In 10Khz unit
  ULONG                           ulASICMaxMemoryClock;       //In 10Khz unit
  UCHAR                           ucASICMaxTemperature;
  UCHAR                           ucMinAllowedBL_Level;
  USHORT                          usBootUpVDDCVoltage;        //In MV unit
  USHORT                          usLcdMinPixelClockPLL_Output; // In MHz unit
  USHORT                          usLcdMaxPixelClockPLL_Output; // In MHz unit
  ULONG                           ul3DAccelerationEngineClock;//In 10Khz unit
  ULONG                           ulMinPixelClockPLL_Output;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Output; //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Output; //In 10Khz unit
  USHORT                          usMaxPixelClock;            //In 10Khz unit, Max.  Pclk
  USHORT                          usMinPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMaxPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMinPixelClockPLL_Output;  //In 10Khz unit - lower 16bit of ulMinPixelClockPLL_Output
  ATOM_FIRMWARE_CAPABILITY_ACCESS usFirmwareCapability;
  USHORT                          usReferenceClock;           //In 10Khz unit
  USHORT                          usPM_RTS_Location;          //RTS PM4 starting location in ROM in 1Kb unit
  UCHAR                           ucPM_RTS_StreamSize;        //RTS PM4 packets in Kb unit
  UCHAR                           ucDesign_ID;                //Indicate what is the board design
  UCHAR                           ucMemoryModule_ID;          //Indicate what is the board design
}ATOM_FIRMWARE_INFO_V1_4;

//the structure below to be used from Cypress
typedef struct _ATOM_FIRMWARE_INFO_V2_1
{
  ATOM_COMMON_TABLE_HEADER        sHeader;
  ULONG                           ulFirmwareRevision;
  ULONG                           ulDefaultEngineClock;       //In 10Khz unit
  ULONG                           ulDefaultMemoryClock;       //In 10Khz unit
  ULONG                           ulReserved1;
  ULONG                           ulReserved2;
  ULONG                           ulMaxEngineClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxMemoryClockPLL_Output; //In 10Khz unit
  ULONG                           ulMaxPixelClockPLL_Output;  //In 10Khz unit
  ULONG                           ulBinaryAlteredInfo;        //Was ulASICMaxEngineClock
  ULONG                           ulDefaultDispEngineClkFreq; //In 10Khz unit
  UCHAR                           ucReserved1;                //Was ucASICMaxTemperature;
  UCHAR                           ucMinAllowedBL_Level;
  USHORT                          usBootUpVDDCVoltage;        //In MV unit
  USHORT                          usLcdMinPixelClockPLL_Output; // In MHz unit
  USHORT                          usLcdMaxPixelClockPLL_Output; // In MHz unit
  ULONG                           ulReserved4;                //Was ulAsicMaximumVoltage
  ULONG                           ulMinPixelClockPLL_Output;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxEngineClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinEngineClockPLL_Output; //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMaxMemoryClockPLL_Input;  //In 10Khz unit
  USHORT                          usMinMemoryClockPLL_Output; //In 10Khz unit
  USHORT                          usMaxPixelClock;            //In 10Khz unit, Max.  Pclk
  USHORT                          usMinPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMaxPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMinPixelClockPLL_Output;  //In 10Khz unit - lower 16bit of ulMinPixelClockPLL_Output
  ATOM_FIRMWARE_CAPABILITY_ACCESS usFirmwareCapability;
  USHORT                          usCoreReferenceClock;       //In 10Khz unit
  USHORT                          usMemoryReferenceClock;     //In 10Khz unit
  USHORT                          usUniphyDPModeExtClkFreq;   //In 10Khz unit, if it is 0, In DP Mode Uniphy Input clock from internal PPLL, otherwise Input clock from external Spread clock
  UCHAR                           ucMemoryModule_ID;          //Indicate what is the board design
  UCHAR                           ucReserved4[3];

}ATOM_FIRMWARE_INFO_V2_1;

//the structure below to be used from NI
//ucTableFormatRevision=2
//ucTableContentRevision=2

typedef struct _PRODUCT_BRANDING
{
    UCHAR     ucEMBEDDED_CAP:2;          // Bit[1:0] Embedded feature level
    UCHAR     ucReserved:2;              // Bit[3:2] Reserved
    UCHAR     ucBRANDING_ID:4;           // Bit[7:4] Branding ID
}PRODUCT_BRANDING;

typedef struct _ATOM_FIRMWARE_INFO_V2_2
{
  ATOM_COMMON_TABLE_HEADER        sHeader;
  ULONG                           ulFirmwareRevision;
  ULONG                           ulDefaultEngineClock;       //In 10Khz unit
  ULONG                           ulDefaultMemoryClock;       //In 10Khz unit
  ULONG                           ulSPLL_OutputFreq;          //In 10Khz unit
  ULONG                           ulGPUPLL_OutputFreq;        //In 10Khz unit
  ULONG                           ulReserved1;                //Was ulMaxEngineClockPLL_Output; //In 10Khz unit*
  ULONG                           ulReserved2;                //Was ulMaxMemoryClockPLL_Output; //In 10Khz unit*
  ULONG                           ulMaxPixelClockPLL_Output;  //In 10Khz unit
  ULONG                           ulBinaryAlteredInfo;        //Was ulASICMaxEngineClock  ?
  ULONG                           ulDefaultDispEngineClkFreq; //In 10Khz unit. This is the frequency before DCDTO, corresponding to usBootUpVDDCVoltage.
  UCHAR                           ucReserved3;                //Was ucASICMaxTemperature;
  UCHAR                           ucMinAllowedBL_Level;
  USHORT                          usBootUpVDDCVoltage;        //In MV unit
  USHORT                          usLcdMinPixelClockPLL_Output; // In MHz unit
  USHORT                          usLcdMaxPixelClockPLL_Output; // In MHz unit
  ULONG                           ulReserved4;                //Was ulAsicMaximumVoltage
  ULONG                           ulMinPixelClockPLL_Output;  //In 10Khz unit
  UCHAR                           ucRemoteDisplayConfig;
  UCHAR                           ucReserved5[3];             //Was usMinEngineClockPLL_Input and usMaxEngineClockPLL_Input
  ULONG                           ulReserved6;                //Was usMinEngineClockPLL_Output and usMinMemoryClockPLL_Input
  ULONG                           ulReserved7;                //Was usMaxMemoryClockPLL_Input and usMinMemoryClockPLL_Output
  USHORT                          usReserved11;               //Was usMaxPixelClock;  //In 10Khz unit, Max.  Pclk used only for DAC
  USHORT                          usMinPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usMaxPixelClockPLL_Input;   //In 10Khz unit
  USHORT                          usBootUpVDDCIVoltage;       //In unit of mv; Was usMinPixelClockPLL_Output;
  ATOM_FIRMWARE_CAPABILITY_ACCESS usFirmwareCapability;
  USHORT                          usCoreReferenceClock;       //In 10Khz unit
  USHORT                          usMemoryReferenceClock;     //In 10Khz unit
  USHORT                          usUniphyDPModeExtClkFreq;   //In 10Khz unit, if it is 0, In DP Mode Uniphy Input clock from internal PPLL, otherwise Input clock from external Spread clock
  UCHAR                           ucMemoryModule_ID;          //Indicate what is the board design
  UCHAR                           ucCoolingSolution_ID;       //0: Air cooling; 1: Liquid cooling ... [COOLING_SOLUTION]
  PRODUCT_BRANDING                ucProductBranding;          // Bit[7:4]ucBRANDING_ID: Branding ID, Bit[3:2]ucReserved: Reserved, Bit[1:0]ucEMBEDDED_CAP: Embedded feature level.
  UCHAR                           ucReserved9;
  USHORT                          usBootUpMVDDCVoltage;       //In unit of mv; Was usMinPixelClockPLL_Output;
  USHORT                          usBootUpVDDGFXVoltage;      //In unit of mv;
  ULONG                           ulReserved10[3];            // New added comparing to previous version
}ATOM_FIRMWARE_INFO_V2_2;

#define ATOM_FIRMWARE_INFO_LAST  ATOM_FIRMWARE_INFO_V2_2


// definition of ucRemoteDisplayConfig
#define REMOTE_DISPLAY_DISABLE                   0x00
#define REMOTE_DISPLAY_ENABLE                    0x01

/****************************************************************************/
// Structures used in IntegratedSystemInfoTable
/****************************************************************************/
#define IGP_CAP_FLAG_DYNAMIC_CLOCK_EN      0x2
#define IGP_CAP_FLAG_AC_CARD               0x4
#define IGP_CAP_FLAG_SDVO_CARD             0x8
#define IGP_CAP_FLAG_POSTDIV_BY_2_MODE     0x10

typedef struct _ATOM_INTEGRATED_SYSTEM_INFO
{
  ATOM_COMMON_TABLE_HEADER        sHeader;
  ULONG                           ulBootUpEngineClock;          //in 10kHz unit
  ULONG                           ulBootUpMemoryClock;          //in 10kHz unit
  ULONG                           ulMaxSystemMemoryClock;       //in 10kHz unit
  ULONG                           ulMinSystemMemoryClock;       //in 10kHz unit
  UCHAR                           ucNumberOfCyclesInPeriodHi;
  UCHAR                           ucLCDTimingSel;             //=0:not valid.!=0 sel this timing descriptor from LCD EDID.
  USHORT                          usReserved1;
  USHORT                          usInterNBVoltageLow;        //An intermidiate PMW value to set the voltage
  USHORT                          usInterNBVoltageHigh;       //Another intermidiate PMW value to set the voltage
  ULONG                           ulReserved[2];

  USHORT                          usFSBClock;                     //In MHz unit
  USHORT                          usCapabilityFlag;              //Bit0=1 indicates the fake HDMI support,Bit1=0/1 for Dynamic clocking dis/enable
                                                                              //Bit[3:2]== 0:No PCIE card, 1:AC card, 2:SDVO card
                                                              //Bit[4]==1: P/2 mode, ==0: P/1 mode
  USHORT                          usPCIENBCfgReg7;                //bit[7:0]=MUX_Sel, bit[9:8]=MUX_SEL_LEVEL2, bit[10]=Lane_Reversal
  USHORT                          usK8MemoryClock;            //in MHz unit
  USHORT                          usK8SyncStartDelay;         //in 0.01 us unit
  USHORT                          usK8DataReturnTime;         //in 0.01 us unit
  UCHAR                           ucMaxNBVoltage;
  UCHAR                           ucMinNBVoltage;
  UCHAR                           ucMemoryType;                     //[7:4]=1:DDR1;=2:DDR2;=3:DDR3.[3:0] is reserved
  UCHAR                           ucNumberOfCyclesInPeriod;      //CG.FVTHROT_PWM_CTRL_REG0.NumberOfCyclesInPeriod
  UCHAR                           ucStartingPWM_HighTime;     //CG.FVTHROT_PWM_CTRL_REG0.StartingPWM_HighTime
  UCHAR                           ucHTLinkWidth;              //16 bit vs. 8 bit
  UCHAR                           ucMaxNBVoltageHigh;
  UCHAR                           ucMinNBVoltageHigh;
}ATOM_INTEGRATED_SYSTEM_INFO;

/* Explanation on entries in ATOM_INTEGRATED_SYSTEM_INFO
ulBootUpMemoryClock:    For Intel IGP,it's the UMA system memory clock
                        For AMD IGP,it's 0 if no SidePort memory installed or it's the boot-up SidePort memory clock
ulMaxSystemMemoryClock: For Intel IGP,it's the Max freq from memory SPD if memory runs in ASYNC mode or otherwise (SYNC mode) it's 0
                        For AMD IGP,for now this can be 0
ulMinSystemMemoryClock: For Intel IGP,it's 133MHz if memory runs in ASYNC mode or otherwise (SYNC mode) it's 0
                        For AMD IGP,for now this can be 0

usFSBClock:             For Intel IGP,it's FSB Freq
                        For AMD IGP,it's HT Link Speed

usK8MemoryClock:        For AMD IGP only. For RevF CPU, set it to 200
usK8SyncStartDelay:     For AMD IGP only. Memory access latency in K8, required for watermark calculation
usK8DataReturnTime:     For AMD IGP only. Memory access latency in K8, required for watermark calculation

VC:Voltage Control
ucMaxNBVoltage:         Voltage regulator dependent PWM value. Low 8 bits of the value for the max voltage.Set this one to 0xFF if VC without PWM. Set this to 0x0 if no VC at all.
ucMinNBVoltage:         Voltage regulator dependent PWM value. Low 8 bits of the value for the min voltage.Set this one to 0x00 if VC without PWM or no VC at all.

ucNumberOfCyclesInPeriod:   Indicate how many cycles when PWM duty is 100%. low 8 bits of the value.
ucNumberOfCyclesInPeriodHi: Indicate how many cycles when PWM duty is 100%. high 8 bits of the value.If the PWM has an inverter,set bit [7]==1,otherwise set it 0

ucMaxNBVoltageHigh:     Voltage regulator dependent PWM value. High 8 bits of  the value for the max voltage.Set this one to 0xFF if VC without PWM. Set this to 0x0 if no VC at all.
ucMinNBVoltageHigh:     Voltage regulator dependent PWM value. High 8 bits of the value for the min voltage.Set this one to 0x00 if VC without PWM or no VC at all.


usInterNBVoltageLow:    Voltage regulator dependent PWM value. The value makes the the voltage >=Min NB voltage but <=InterNBVoltageHigh. Set this to 0x0000 if VC without PWM or no VC at all.
usInterNBVoltageHigh:   Voltage regulator dependent PWM value. The value makes the the voltage >=InterNBVoltageLow but <=Max NB voltage.Set this to 0x0000 if VC without PWM or no VC at all.
*/


/*
The following IGP table is introduced from RS780, which is supposed to be put by SBIOS in FB before IGP VBIOS starts VPOST;
Then VBIOS will copy the whole structure to its image so all GPU SW components can access this data structure to get whatever they need.
The enough reservation should allow us to never change table revisions. Whenever needed, a GPU SW component can use reserved portion for new data entries.

SW components can access the IGP system infor structure in the same way as before
*/


typedef struct _ATOM_INTEGRATED_SYSTEM_INFO_V2
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ULONG                      ulBootUpEngineClock;       //in 10kHz unit
  ULONG                      ulReserved1[2];            //must be 0x0 for the reserved
  ULONG                      ulBootUpUMAClock;          //in 10kHz unit
  ULONG                      ulBootUpSidePortClock;     //in 10kHz unit
  ULONG                      ulMinSidePortClock;        //in 10kHz unit
  ULONG                      ulReserved2[6];            //must be 0x0 for the reserved
  ULONG                      ulSystemConfig;            //see explanation below
  ULONG                      ulBootUpReqDisplayVector;
  ULONG                      ulOtherDisplayMisc;
  ULONG                      ulDDISlot1Config;
  ULONG                      ulDDISlot2Config;
  UCHAR                      ucMemoryType;              //[3:0]=1:DDR1;=2:DDR2;=3:DDR3.[7:4] is reserved
  UCHAR                      ucUMAChannelNumber;
  UCHAR                      ucDockingPinBit;
  UCHAR                      ucDockingPinPolarity;
  ULONG                      ulDockingPinCFGInfo;
  ULONG                      ulCPUCapInfo;
  USHORT                     usNumberOfCyclesInPeriod;
  USHORT                     usMaxNBVoltage;
  USHORT                     usMinNBVoltage;
  USHORT                     usBootUpNBVoltage;
  ULONG                      ulHTLinkFreq;              //in 10Khz
  USHORT                     usMinHTLinkWidth;
  USHORT                     usMaxHTLinkWidth;
  USHORT                     usUMASyncStartDelay;
  USHORT                     usUMADataReturnTime;
  USHORT                     usLinkStatusZeroTime;
  USHORT                     usDACEfuse;            //for storing badgap value (for RS880 only)
  ULONG                      ulHighVoltageHTLinkFreq;     // in 10Khz
  ULONG                      ulLowVoltageHTLinkFreq;      // in 10Khz
  USHORT                     usMaxUpStreamHTLinkWidth;
  USHORT                     usMaxDownStreamHTLinkWidth;
  USHORT                     usMinUpStreamHTLinkWidth;
  USHORT                     usMinDownStreamHTLinkWidth;
  USHORT                     usFirmwareVersion;         //0 means FW is not supported. Otherwise it's the FW version loaded by SBIOS and driver should enable FW.
  USHORT                     usFullT0Time;             // Input to calculate minimum HT link change time required by NB P-State. Unit is 0.01us.
  ULONG                      ulReserved3[96];          //must be 0x0
}ATOM_INTEGRATED_SYSTEM_INFO_V2;

/*
ulBootUpEngineClock:   Boot-up Engine Clock in 10Khz;
ulBootUpUMAClock:      Boot-up UMA Clock in 10Khz; it must be 0x0 when UMA is not present
ulBootUpSidePortClock: Boot-up SidePort Clock in 10Khz; it must be 0x0 when SidePort Memory is not present,this could be equal to or less than maximum supported Sideport memory clock

ulSystemConfig:
Bit[0]=1: PowerExpress mode =0 Non-PowerExpress mode;
Bit[1]=1: system boots up at AMD overdrived state or user customized  mode. In this case, driver will just stick to this boot-up mode. No other PowerPlay state
      =0: system boots up at driver control state. Power state depends on PowerPlay table.
Bit[2]=1: PWM method is used on NB voltage control. =0: GPIO method is used.
Bit[3]=1: Only one power state(Performance) will be supported.
      =0: Multiple power states supported from PowerPlay table.
Bit[4]=1: CLMC is supported and enabled on current system.
      =0: CLMC is not supported or enabled on current system. SBIOS need to support HT link/freq change through ATIF interface.
Bit[5]=1: Enable CDLW for all driver control power states. Max HT width is from SBIOS, while Min HT width is determined by display requirement.
      =0: CDLW is disabled. If CLMC is enabled case, Min HT width will be set equal to Max HT width. If CLMC disabled case, Max HT width will be applied.
Bit[6]=1: High Voltage requested for all power states. In this case, voltage will be forced at 1.1v and powerplay table voltage drop/throttling request will be ignored.
      =0: Voltage settings is determined by powerplay table.
Bit[7]=1: Enable CLMC as hybrid Mode. CDLD and CILR will be disabled in this case and we're using legacy C1E. This is workaround for CPU(Griffin) performance issue.
      =0: Enable CLMC as regular mode, CDLD and CILR will be enabled.
Bit[8]=1: CDLF is supported and enabled on current system.
      =0: CDLF is not supported or enabled on current system.
Bit[9]=1: DLL Shut Down feature is enabled on current system.
      =0: DLL Shut Down feature is not enabled or supported on current system.

ulBootUpReqDisplayVector: This dword is a bit vector indicates what display devices are requested during boot-up. Refer to ATOM_DEVICE_xxx_SUPPORT for the bit vector definitions.

ulOtherDisplayMisc: [15:8]- Bootup LCD Expansion selection; 0-center, 1-full panel size expansion;
                       [7:0] - BootupTV standard selection; This is a bit vector to indicate what TV standards are supported by the system. Refer to ucTVSuppportedStd definition;

ulDDISlot1Config: Describes the PCIE lane configuration on this DDI PCIE slot (ADD2 card) or connector (Mobile design).
      [3:0]  - Bit vector to indicate PCIE lane config of the DDI slot/connector on chassis (bit 0=1 lane 3:0; bit 1=1 lane 7:4; bit 2=1 lane 11:8; bit 3=1 lane 15:12)
         [7:4]  - Bit vector to indicate PCIE lane config of the same DDI slot/connector on docking station (bit 4=1 lane 3:0; bit 5=1 lane 7:4; bit 6=1 lane 11:8; bit 7=1 lane 15:12)
      When a DDI connector is not "paired" (meaming two connections mutualexclusive on chassis or docking, only one of them can be connected at one time.
      in both chassis and docking, SBIOS has to duplicate the same PCIE lane info from chassis to docking or vice versa. For example:
      one DDI connector is only populated in docking with PCIE lane 8-11, but there is no paired connection on chassis, SBIOS has to copy bit 6 to bit 2.

         [15:8] - Lane configuration attribute;
      [23:16]- Connector type, possible value:
               CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D
               CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D
               CONNECTOR_OBJECT_ID_HDMI_TYPE_A
               CONNECTOR_OBJECT_ID_DISPLAYPORT
               CONNECTOR_OBJECT_ID_eDP
         [31:24]- Reserved

ulDDISlot2Config: Same as Slot1.
ucMemoryType: SidePort memory type, set it to 0x0 when Sideport memory is not installed. Driver needs this info to change sideport memory clock. Not for display in CCC.
For IGP, Hypermemory is the only memory type showed in CCC.

ucUMAChannelNumber:  how many channels for the UMA;

ulDockingPinCFGInfo: [15:0]-Bus/Device/Function # to CFG to read this Docking Pin; [31:16]-reg offset in CFG to read this pin
ucDockingPinBit:     which bit in this register to read the pin status;
ucDockingPinPolarity:Polarity of the pin when docked;

ulCPUCapInfo:        [7:0]=1:Griffin;[7:0]=2:Greyhound;[7:0]=3:K8, [7:0]=4:Pharaoh, other bits reserved for now and must be 0x0

usNumberOfCyclesInPeriod:Indicate how many cycles when PWM duty is 100%.

usMaxNBVoltage:Max. voltage control value in either PWM or GPIO mode.
usMinNBVoltage:Min. voltage control value in either PWM or GPIO mode.
                    GPIO mode: both usMaxNBVoltage & usMinNBVoltage have a valid value ulSystemConfig.SYSTEM_CONFIG_USE_PWM_ON_VOLTAGE=0
                    PWM mode: both usMaxNBVoltage & usMinNBVoltage have a valid value ulSystemConfig.SYSTEM_CONFIG_USE_PWM_ON_VOLTAGE=1
                    GPU SW don't control mode: usMaxNBVoltage & usMinNBVoltage=0 and no care about ulSystemConfig.SYSTEM_CONFIG_USE_PWM_ON_VOLTAGE

usBootUpNBVoltage:Boot-up voltage regulator dependent PWM value.


ulHTLinkFreq:       Bootup HT link Frequency in 10Khz.
usMinHTLinkWidth:   Bootup minimum HT link width. If CDLW disabled, this is equal to usMaxHTLinkWidth.
                    If CDLW enabled, both upstream and downstream width should be the same during bootup.
usMaxHTLinkWidth:   Bootup maximum HT link width. If CDLW disabled, this is equal to usMinHTLinkWidth.
                    If CDLW enabled, both upstream and downstream width should be the same during bootup.

usUMASyncStartDelay: Memory access latency, required for watermark calculation
usUMADataReturnTime: Memory access latency, required for watermark calculation
usLinkStatusZeroTime:Memory access latency required for watermark calculation, set this to 0x0 for K8 CPU, set a proper value in 0.01 the unit of us
for Griffin or Greyhound. SBIOS needs to convert to actual time by:
                     if T0Ttime [5:4]=00b, then usLinkStatusZeroTime=T0Ttime [3:0]*0.1us (0.0 to 1.5us)
                     if T0Ttime [5:4]=01b, then usLinkStatusZeroTime=T0Ttime [3:0]*0.5us (0.0 to 7.5us)
                     if T0Ttime [5:4]=10b, then usLinkStatusZeroTime=T0Ttime [3:0]*2.0us (0.0 to 30us)
                     if T0Ttime [5:4]=11b, and T0Ttime [3:0]=0x0 to 0xa, then usLinkStatusZeroTime=T0Ttime [3:0]*20us (0.0 to 200us)

ulHighVoltageHTLinkFreq:     HT link frequency for power state with low voltage. If boot up runs in HT1, this must be 0.
                             This must be less than or equal to ulHTLinkFreq(bootup frequency).
ulLowVoltageHTLinkFreq:      HT link frequency for power state with low voltage or voltage scaling 1.0v~1.1v. If boot up runs in HT1, this must be 0.
                             This must be less than or equal to ulHighVoltageHTLinkFreq.

usMaxUpStreamHTLinkWidth:    Asymmetric link width support in the future, to replace usMaxHTLinkWidth. Not used for now.
usMaxDownStreamHTLinkWidth:  same as above.
usMinUpStreamHTLinkWidth:    Asymmetric link width support in the future, to replace usMinHTLinkWidth. Not used for now.
usMinDownStreamHTLinkWidth:  same as above.
*/

// ATOM_INTEGRATED_SYSTEM_INFO::ulCPUCapInfo  - CPU type definition
#define    INTEGRATED_SYSTEM_INFO__UNKNOWN_CPU             0
#define    INTEGRATED_SYSTEM_INFO__AMD_CPU__GRIFFIN        1
#define    INTEGRATED_SYSTEM_INFO__AMD_CPU__GREYHOUND      2
#define    INTEGRATED_SYSTEM_INFO__AMD_CPU__K8             3
#define    INTEGRATED_SYSTEM_INFO__AMD_CPU__PHARAOH        4
#define    INTEGRATED_SYSTEM_INFO__AMD_CPU__OROCHI         5

#define    INTEGRATED_SYSTEM_INFO__AMD_CPU__MAX_CODE       INTEGRATED_SYSTEM_INFO__AMD_CPU__OROCHI    // this deff reflects max defined CPU code

#define SYSTEM_CONFIG_POWEREXPRESS_ENABLE                 0x00000001
#define SYSTEM_CONFIG_RUN_AT_OVERDRIVE_ENGINE             0x00000002
#define SYSTEM_CONFIG_USE_PWM_ON_VOLTAGE                  0x00000004
#define SYSTEM_CONFIG_PERFORMANCE_POWERSTATE_ONLY         0x00000008
#define SYSTEM_CONFIG_CLMC_ENABLED                        0x00000010
#define SYSTEM_CONFIG_CDLW_ENABLED                        0x00000020
#define SYSTEM_CONFIG_HIGH_VOLTAGE_REQUESTED              0x00000040
#define SYSTEM_CONFIG_CLMC_HYBRID_MODE_ENABLED            0x00000080
#define SYSTEM_CONFIG_CDLF_ENABLED                        0x00000100
#define SYSTEM_CONFIG_DLL_SHUTDOWN_ENABLED                0x00000200

#define IGP_DDI_SLOT_LANE_CONFIG_MASK                     0x000000FF

#define b0IGP_DDI_SLOT_LANE_MAP_MASK                      0x0F
#define b0IGP_DDI_SLOT_DOCKING_LANE_MAP_MASK              0xF0
#define b0IGP_DDI_SLOT_CONFIG_LANE_0_3                    0x01
#define b0IGP_DDI_SLOT_CONFIG_LANE_4_7                    0x02
#define b0IGP_DDI_SLOT_CONFIG_LANE_8_11                   0x04
#define b0IGP_DDI_SLOT_CONFIG_LANE_12_15                  0x08

#define IGP_DDI_SLOT_ATTRIBUTE_MASK                       0x0000FF00
#define IGP_DDI_SLOT_CONFIG_REVERSED              