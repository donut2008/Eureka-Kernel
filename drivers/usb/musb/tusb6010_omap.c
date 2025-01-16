EM_INFO_V2
{
  ATOM_INTEGRATED_SYSTEM_INFO_V1_8    sIntegratedSysInfo;       // refer to ATOM_INTEGRATED_SYSTEM_INFO_V1_8 definition
  ULONG                               ulPowerplayTable[128];    // Update comments here to link new powerplay table definition structure
}ATOM_FUSION_SYSTEM_INFO_V2;


typedef struct _ATOM_I2C_REG_INFO
{
  UCHAR ucI2cRegIndex;
  UCHAR ucI2cRegVal;
}ATOM_I2C_REG_INFO;

// this IntegrateSystemInfoTable is used for Carrizo
typedef struct _ATOM_INTEGRATED_SYSTEM_INFO_V1_9
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ULONG  ulBootUpEngineClock;
  ULONG  ulDentistVCOFreq;
  ULONG  ulBootUpUMAClock;
  ATOM_CLK_VOLT_CAPABILITY   sDISPCLK_Voltage[4];       // no longer used, keep it as is to avoid driver compiling error
  ULONG  ulBootUpReqDisplayVector;
  ULONG  ulVBIOSMisc;
  ULONG  ulGPUCapInfo;
  ULONG  ulDISP_CLK2Freq;
  USHORT usRequestedPWMFreqInHz;
  UCHAR  ucHtcTmpLmt;
  UCHAR  ucHtcHystLmt;
  ULONG  ulReserved2;
  ULONG  ulSystemConfig;
  ULONG  ulCPUCapInfo;
  ULONG  ulReserved3;
  USHORT usGPUReservedSysMemSize;
  USHORT usExtDispConnInfoOffset;
  USHORT usPanelRefreshRateRange;
  UCHAR  ucMemoryType;
  UCHAR  ucUMAChannelNumber;
  UCHAR  strVBIOSMsg[40];
  ATOM_TDP_CONFIG  asTdpConfig;
  UCHAR  ucExtHDMIReDrvSlvAddr;
  UCHAR  ucExtHDMIReDrvRegNum;
  ATOM_I2C_REG_INFO asExtHDMIRegSetting[9];
  ULONG  ulReserved[2];
  ATOM_CLK_VOLT_CAPABILITY_V2   sDispClkVoltageMapping[8];
  ATOM_AVAILABLE_SCLK_LIST   sAvail_SCLK[5];            // no longer used, keep it as is to avoid driver compiling error
  ULONG  ulGMCRestoreResetTime;
  ULONG  ulReserved4;
  ULONG  ulIdleNClk;
  ULONG  ulDDR_DLL_PowerUpTime;
  ULONG  ulDDR_PLL_PowerUpTime;
  USHORT usPCIEClkSSPercentage;
  USHORT usPCIEClkSSType;
  USHORT usLvdsSSPercentage;
  USHORT usLvdsSSpreadRateIn10Hz;
  USHORT usHDMISSPercentage;
  USHORT usHDMISSpreadRateIn10Hz;
  USHORT usDVISSPercentage;
  USHORT usDVISSpreadRateIn10Hz;
  ULONG  ulGPUReservedSysMemBaseAddrLo;
  ULONG  ulGPUReservedSysMemBaseAddrHi;
  ULONG  ulReserved5[3];
  USHORT usMaxLVDSPclkFreqInSingleLink;
  UCHAR  ucLvdsMisc;
  UCHAR  ucTravisLVDSVolAdjust;
  UCHAR  ucLVDSPwrOnSeqDIGONtoDE_in4Ms;
  UCHAR  ucLVDSPwrOnSeqDEtoVARY_BL_in4Ms;
  UCHAR  ucLVDSPwrOffSeqVARY_BLtoDE_in4Ms;
  UCHAR  ucLVDSPwrOffSeqDEtoDIGON_in4Ms;
  UCHAR  ucLVDSOffToOnDelay_in4Ms;
  UCHAR  ucLVDSPwrOnSeqVARY_BLtoBLON_in4Ms;
  UCHAR  ucLVDSPwrOffSeqBLONtoVARY_BL_in4Ms;
  UCHAR  ucMinAllowedBL_Level;
  ULONG  ulLCDBitDepthControlVal;
  ULONG  ulNbpStateMemclkFreq[4];          // only 2 level is changed.
  ULONG  ulPSPVersion;
  ULONG  ulNbpStateNClkFreq[4];
  USHORT usNBPStateVoltage[4];
  USHORT usBootUpNBVoltage;
  UCHAR  ucEDPv1_4VSMode;
  UCHAR  ucReserved2;
  ATOM_EXTERNAL_DISPLAY_CONNECTION_INFO sExtDispConnInfo;
}ATOM_INTEGRATED_SYSTEM_INFO_V1_9;


// definition for ucEDPv1_4VSMode
#define EDP_VS_LEGACY_MODE                  0
#define EDP_VS_LOW_VDIFF_MODE               1
#define EDP_VS_HIGH_VDIFF_MODE              2
#define EDP_VS_STRETCH_MODE                 3
#define EDP_VS_SINGLE_VDIFF_MODE            4
#define EDP_VS_VARIABLE_PREM_MODE           5


// this IntegrateSystemInfoTable is used for Carrizo
typedef struct _ATOM_INTEGRATED_SYSTEM_INFO_V1_10
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ULONG  ulBootUpEngineClock;
  ULONG  ulDentistVCOFreq;
  ULONG  ulBootUpUMAClock;
  ULONG  ulReserved0[8];
  ULONG  ulBootUpReqDisplayVector;
  ULONG  ulVBIOSMisc;
  ULONG  ulGPUCapInfo;
  ULONG  ulReserved1;
  USHORT usRequestedPWMFreqInHz;
  UCHAR  ucHtcTmpLmt;
  UCHAR  ucHtcHystLmt;
  ULONG  ulReserved2;
  ULONG  ulSystemConfig;
  ULONG  ulCPUCapInfo;
  ULONG  ulReserved3;
  USHORT usGPUReservedSysMemSize;
  USHORT usExtDispConnInfoOffset;
  USHORT usPanelRefreshRateRange;
  UCHAR  ucMemoryType;
  UCHAR  ucUMAChannelNumber;
  UCHAR  strVBIOSMsg[40];
  ATOM_TDP_CONFIG  asTdpConfig;
  ULONG  ulReserved[7];
  ATOM_CLK_VOLT_CAPABILITY_V2   sDispClkVoltageMapping[8];
  ULONG  ulReserved6[10];
  ULONG  ulGMCRestoreResetTime;
  ULONG  ulReserved4;
  ULONG  ulIdleNClk;
  ULONG  ulDDR_DLL_PowerUpTime;
  ULONG  ulDDR_PLL_PowerUpTime;
  USHORT usPCIEClkSSPercentage;
  USHORT usPCIEClkSSType;
  USHORT usLvdsSSPercentage;
  USHORT usLvdsSSpreadRateIn10Hz;
  USHORT usHDMISSPercentage;
  USHORT usHDMISSpreadRateIn10Hz;
  USHORT usDVISSPercentage;
  USHORT usDVISSpreadRateIn10Hz;
  ULONG  ulGPUReservedSysMemBaseAddrLo;
  ULONG  ulGPUReservedSysMemBaseAddrHi;
  ULONG  ulReserved5[3];
  USHORT usMaxLVDSPclkFreqInSingleLink;
  UCHAR  ucLvdsMisc;
  UCHAR  ucTravisLVDSVolAdjust;
  UCHAR  ucLVDSPwrOnSeqDIGONtoDE_in4Ms;
  UCHAR  ucLVDSPwrOnSeqDEtoVARY_BL_in4Ms;
  UCHAR  ucLVDSPwrOffSeqVARY_BLtoDE_in4Ms;
  UCHAR  ucLVDSPwrOffSeqDEtoDIGON_in4Ms;
  UCHAR  ucLVDSOffToOnDelay_in4Ms;
  UCHAR  ucLVDSPwrOnSeqVARY_BLtoBLON_in4Ms;
  UCHAR  ucLVDSPwrOffSeqBLONtoVARY_BL_in4Ms;
  UCHAR  ucMinAllowedBL_Level;
  ULONG  ulLCDBitDepthControlVal;
  ULONG  ulNbpStateMemclkFreq[2];
  ULONG  ulReserved7[2];
  ULONG  ulPSPVersion;
  ULONG  ulNbpStateNClkFreq[4];
  USHORT usNBPStateVoltage[4];
  USHORT usBootUpNBVoltage;
  UCHAR  ucEDPv1_4VSMode;
  UCHAR  ucReserved2;
  ATOM_EXTERNAL_DISPLAY_CONNECTION_INFO sExtDispConnInfo;
}ATOM_INTEGRATED_SYSTEM_INFO_V1_10;

/**************************************************************************/
// This portion is only used when ext thermal chip or engine/memory clock SS chip is populated on a design
//Memory SS Info Table
//Define Memory Clock SS chip ID
#define ICS91719  1
#define ICS91720  2

//Define one structure to inform SW a "block of data" writing to external SS chip via I2C protocol
typedef struct _ATOM_I2C_DATA_RECORD
{
  UCHAR         ucNunberOfBytes;                                              //Indicates how many bytes SW needs to write to the external ASIC for one block, besides to "Start" and "Stop"
  UCHAR         ucI2CData[1];                                                 //I2C data in bytes, should be less than 16 bytes usually
}ATOM_I2C_DATA_RECORD;


//Define one structure to inform SW how many blocks of data writing to external SS chip via I2C protocol, in addition to other information
typedef struct _ATOM_I2C_DEVICE_SETUP_INFO
{
  ATOM_I2C_ID_CONFIG_ACCESS       sucI2cId;               //I2C line and HW/SW assisted cap.
  UCHAR                              ucSSChipID;             //SS chip being used
  UCHAR                              ucSSChipSlaveAddr;      //Slave Address to set up this SS chip
  UCHAR                           ucNumOfI2CDataRecords;  //number of data block
  ATOM_I2C_DATA_RECORD            asI2CData[1];
}ATOM_I2C_DEVICE_SETUP_INFO;

//==========================================================================================
typedef struct  _ATOM_ASIC_MVDD_INFO
{
  ATOM_COMMON_TABLE_HEADER         sHeader;
  ATOM_I2C_DEVICE_SETUP_INFO      asI2CSetup[1];
}ATOM_ASIC_MVDD_INFO;

//==========================================================================================
#define ATOM_MCLK_SS_INFO         ATOM_ASIC_MVDD_INFO

//==========================================================================================
/**************************************************************************/

typedef struct _ATOM_ASIC_SS_ASSIGNMENT
{
   ULONG                        ulTargetClockRange;                  //Clock Out frequence (VCO ), in unit of 10Khz
  USHORT              usSpreadSpectrumPercentage;      //in unit of 0.01%
   USHORT                     usSpreadRateInKhz;                  //in unit of kHz, modulation freq
  UCHAR               ucClockIndication;                 //Indicate which clock source needs SS
   UCHAR                        ucSpreadSpectrumMode;               //Bit1=0 Down Spread,=1 Center Spread.
   UCHAR                        ucReserved[2];
}ATOM_ASIC_SS_ASSIGNMENT;

//Define ucClockIndication, SW uses the IDs below to search if the SS is requried/enabled on a clock branch/signal type.
//SS is not required or enabled if a match is not found.
#define ASIC_INTERNAL_MEMORY_SS            1
#define ASIC_INTERNAL_ENGINE_SS            2
#define ASIC_INTERNAL_UVD_SS             3
#define ASIC_INTERNAL_SS_ON_TMDS         4
#define ASIC_INTERNAL_SS_ON_HDMI         5
#define ASIC_INTERNAL_SS_ON_LVDS         6
#define ASIC_INTERNAL_SS_ON_DP           7
#define ASIC_INTERNAL_SS_ON_DCPLL        8
#define ASIC_EXTERNAL_SS_ON_DP_CLOCK     9
#define ASIC_INTERNAL_VCE_SS             10
#define ASIC_INTERNAL_GPUPLL_SS          11


typedef struct _ATOM_ASIC_SS_ASSIGNMENT_V2
{
   ULONG                        ulTargetClockRange;                  //For mem/engine/uvd, Clock Out frequence (VCO ), in unit of 10Khz
                                                    //For TMDS/HDMI/LVDS, it is pixel clock , for DP, it is link clock ( 27000 or 16200 )
  USHORT              usSpreadSpectrumPercentage;      //in unit of 0.01%
   USHORT                     usSpreadRateIn10Hz;                  //in unit of 10Hz, modulation freq
  UCHAR               ucClockIndication;                 //Indicate which clock source needs SS
   UCHAR                        ucSpreadSpectrumMode;               //Bit0=0 Down Spread,=1 Center Spread, bit1=0: internal SS bit1=1: external SS
   UCHAR                        ucReserved[2];
}ATOM_ASIC_SS_ASSIGNMENT_V2;

//ucSpreadSpectrumMode
//#define ATOM_SS_DOWN_SPREAD_MODE_MASK          0x00000000
//#define ATOM_SS_DOWN_SPREAD_MODE               0x00000000
//#define ATOM_SS_CENTRE_SPREAD_MODE_MASK        0x00000001
//#define ATOM_SS_CENTRE_SPREAD_MODE             0x00000001
//#define ATOM_INTERNAL_SS_MASK                  0x00000000
//#define ATOM_EXTERNAL_SS_MASK                  0x00000002

typedef struct _ATOM_ASIC_INTERNAL_SS_INFO
{
  ATOM_COMMON_TABLE_HEADER         sHeader;
  ATOM_ASIC_SS_ASSIGNMENT            asSpreadSpectrum[4];
}ATOM_ASIC_INTERNAL_SS_INFO;

typedef struct _ATOM_ASIC_INTERNAL_SS_INFO_V2
{
  ATOM_COMMON_TABLE_HEADER         sHeader;
  ATOM_ASIC_SS_ASSIGNMENT_V2        asSpreadSpectrum[1];      //this is point only.
}ATOM_ASIC_INTERNAL_SS_INFO_V2;

typedef struct _ATOM_ASIC_SS_ASSIGNMENT_V3
{
   ULONG                        ulTargetClockRange;                  //For mem/engine/uvd, Clock Out frequence (VCO ), in unit of 10Khz
                                                    //For TMDS/HDMI/LVDS, it is pixel clock , for DP, it is link clock ( 27000 or 16200 )
  USHORT              usSpreadSpectrumPercentage;      //in unit of 0.01% or 0.001%, decided by ucSpreadSpectrumMode bit4
   USHORT                     usSpreadRateIn10Hz;                  //in unit of 10Hz, modulation freq
  UCHAR               ucClockIndication;                 //Indicate which clock source needs SS
   UCHAR                        ucSpreadSpectrumMode;               //Bit0=0 Down Spread,=1 Center Spread, bit1=0: internal SS bit1=1: external SS
   UCHAR                        ucReserved[2];
}ATOM_ASIC_SS_ASSIGNMENT_V3;

//ATOM_ASIC_SS_ASSIGNMENT_V3.ucSpreadSpectrumMode
#define SS_MODE_V3_CENTRE_SPREAD_MASK             0x01
#define SS_MODE_V3_EXTERNAL_SS_MASK               0x02
#define SS_MODE_V3_PERCENTAGE_DIV_BY_1000_MASK    0x10

typedef struct _ATOM_ASIC_INTERNAL_SS_INFO_V3
{
  ATOM_COMMON_TABLE_HEADER         sHeader;
  ATOM_ASIC_SS_ASSIGNMENT_V3        asSpreadSpectrum[1];      //this is pointer only.
}ATOM_ASIC_INTERNAL_SS_INFO_V3;


//==============================Scratch Pad Definition Portion===============================
#define ATOM_DEVICE_CONNECT_INFO_DEF  0
#define ATOM_ROM_LOCATION_DEF         1
#define ATOM_TV_STANDARD_DEF          2
#define ATOM_ACTIVE_INFO_DEF          3
#define ATOM_LCD_INFO_DEF             4
#define ATOM_DOS_REQ_INFO_DEF         5
#define ATOM_ACC_CHANGE_INFO_DEF      6
#define ATOM_DOS_MODE_INFO_DEF        7
#define ATOM_I2C_CHANNEL_STATUS_DEF   8
#define ATOM_I2C_CHANNEL_STATUS1_DEF  9
#define ATOM_INTERNAL_TIMER_DEF       10

// BIOS_0_SCRATCH Definition
#define ATOM_S0_CRT1_MONO               0x00000001L
#define ATOM_S0_CRT1_COLOR              0x00000002L
#define ATOM_S0_CRT1_MASK               (ATOM_S0_CRT1_MONO+ATOM_S0_CRT1_COLOR)

#define ATOM_S0_TV1_COMPOSITE_A         0x00000004L
#define ATOM_S0_TV1_SVIDEO_A            0x00000008L
#define ATOM_S0_TV1_MASK_A              (ATOM_S0_TV1_COMPOSITE_A+ATOM_S0_TV1_SVIDEO_A)

#define ATOM_S0_CV_A                    0x00000010L
#define ATOM_S0_CV_DIN_A                0x00000020L
#define ATOM_S0_CV_MASK_A               (ATOM_S0_CV_A+ATOM_S0_CV_DIN_A)


#define ATOM_S0_CRT2_MONO               0x00000100L
#define ATOM_S0_CRT2_COLOR              0x00000200L
#define ATOM_S0_CRT2_MASK               (ATOM_S0_CRT2_MONO+ATOM_S0_CRT2_COLOR)

#define ATOM_S0_TV1_COMPOSITE           0x00000400L
#define ATOM_S0_TV1_SVIDEO              0x00000800L
#define ATOM_S0_TV1_SCART               0x00004000L
#define ATOM_S0_TV1_MASK                (ATOM_S0_TV1_COMPOSITE+ATOM_S0_TV1_SVIDEO+ATOM_S0_TV1_SCART)

#define ATOM_S0_CV                      0x00001000L
#define ATOM_S0_CV_DIN                  0x00002000L
#define ATOM_S0_CV_MASK                 (ATOM_S0_CV+ATOM_S0_CV_DIN)

#define ATOM_S0_DFP1                    0x00010000L
#define ATOM_S0_DFP2                    0x00020000L
#define ATOM_S0_LCD1                    0x00040000L
#define ATOM_S0_LCD2                    0x00080000L
#define ATOM_S0_DFP6                    0x00100000L
#define ATOM_S0_DFP3                    0x00200000L
#define ATOM_S0_DFP4                    0x00400000L
#define ATOM_S0_DFP5                    0x00800000L


#define ATOM_S0_DFP_MASK                ATOM_S0_DFP1 | ATOM_S0_DFP2 | ATOM_S0_DFP3 | ATOM_S0_DFP4 | ATOM_S0_DFP5 | ATOM_S0_DFP6

#define ATOM_S0_FAD_REGISTER_BUG        0x02000000L // If set, indicates we are running a PCIE asic with
                                                    // the FAD/HDP reg access bug.  Bit is read by DAL, this is obsolete from RV5xx

#define ATOM_S0_THERMAL_STATE_MASK      0x1C000000L
#define ATOM_S0_THERMAL_STATE_SHIFT     26

#define ATOM_S0_SYSTEM_POWER_STATE_MASK 0xE0000000L
#define ATOM_S0_SYSTEM_POWER_STATE_SHIFT 29

#define ATOM_S0_SYSTEM_POWER_STATE_VALUE_AC     1
#define ATOM_S0_SYSTEM_POWER_STATE_VALUE_DC     2
#define ATOM_S0_SYSTEM_POWER_STATE_VALUE_LITEAC 3
#define ATOM_S0_SYSTEM_POWER_STATE_VALUE_LIT2AC 4

//Byte aligned defintion for BIOS usage
#define ATOM_S0_CRT1_MONOb0             0x01
#define ATOM_S0_CRT1_COLORb0            0x02
#define ATOM_S0_CRT1_MASKb0             (ATOM_S0_CRT1_MONOb0+ATOM_S0_CRT1_COLORb0)

#define ATOM_S0_TV1_COMPOSITEb0         0x04
#define ATOM_S0_TV1_SVIDEOb0            0x08
#define ATOM_S0_TV1_MASKb0              (ATOM_S0_TV1_COMPOSITEb0+ATOM_S0_TV1_SVIDEOb0)

#define ATOM_S0_CVb0                    0x10
#define ATOM_S0_CV_DINb0                0x20
#define ATOM_S0_CV_MASKb0               (ATOM_S0_CVb0+ATOM_S0_CV_DINb0)

#define ATOM_S0_CRT2_MONOb1             0x01
#define ATOM_S0_CRT2_COLORb1            0x02
#define ATOM_S0_CRT2_MASKb1             (ATOM_S0_CRT2_MONOb1+ATOM_S0_CRT2_COLORb1)

#define ATOM_S0_TV1_COMPOSITEb1         0x04
#define ATOM_S0_TV1_SVIDEOb1            0x08
#define ATOM_S0_TV1_SCARTb1             0x40
#define ATOM_S0_TV1_MASKb1              (ATOM_S0_TV1_COMPOSITEb1+ATOM_S0_TV1_SVIDEOb1+ATOM_S0_TV1_SCARTb1)

#define ATOM_S0_CVb1                    0x10
#define ATOM_S0_CV_DINb1                0x20
#define ATOM_S0_CV_MASKb1               (ATOM_S0_CVb1+ATOM_S0_CV_DINb1)

#define ATOM_S0_DFP1b2                  0x01
#define ATOM_S0_DFP2b2                  0x02
#define ATOM_S0_LCD1b2                  0x04
#define ATOM_S0_LCD2b2                  0x08
#define ATOM_S0_DFP6b2                  0x10
#define ATOM_S0_DFP3b2                  0x20
#define ATOM_S0_DFP4b2                  0x40
#define ATOM_S0_DFP5b2                  0x80


#define ATOM_S0_THERMAL_STATE_MASKb3    0x1C
#define ATOM_S0_THERMAL_STATE_SHIFTb3   2

#define ATOM_S0_SYSTEM_POWER_STATE_MASKb3 0xE0
#define ATOM_S0_LCD1_SHIFT              18

// BIOS_1_SCRATCH Definition
#define ATOM_S1_ROM_LOCATION_MASK       0x0000FFFFL
#define ATOM_S1_PCI_BUS_DEV_MASK        0xFFFF0000L

//   BIOS_2_SCRATCH Definition
#define ATOM_S2_TV1_STANDARD_MASK       0x0000000FL
#define ATOM_S2_CURRENT_BL_LEVEL_MASK   0x0000FF00L
#define ATOM_S2_CURRENT_BL_LEVEL_SHIFT  8

#define ATOM_S2_FORCEDLOWPWRMODE_STATE_MASK       0x0C000000L
#define ATOM_S2_FORCEDLOWPWRMODE_STATE_MASK_SHIFT 26
#define ATOM_S2_FORCEDLOWPWRMODE_STATE_CHANGE     0x10000000L

#define ATOM_S2_DEVICE_DPMS_STATE       0x00010000L
#define ATOM_S2_VRI_BRIGHT_ENABLE       0x20000000L

#define ATOM_S2_DISPLAY_ROTATION_0_DEGREE     0x0
#define ATOM_S2_DISPLAY_ROTATION_90_DEGREE    0x1
#define ATOM_S2_DISPLAY_ROTATION_180_DEGREE   0x2
#define ATOM_S2_DISPLAY_ROTATION_270_DEGREE   0x3
#define ATOM_S2_DISPLAY_ROTATION_DEGREE_SHIFT 30
#define ATOM_S2_DISPLAY_ROTATION_ANGLE_MASK   0xC0000000L


//Byte aligned defintion for BIOS usage
#define ATOM_S2_TV1_STANDARD_MASKb0     0x0F
#define ATOM_S2_CURRENT_BL_LEVEL_MASKb1 0xFF
#define ATOM_S2_DEVICE_DPMS_STATEb2     0x01

#define ATOM_S2_TMDS_COHERENT_MODEb3    0x10          // used by VBIOS code only, use coherent mode for TMDS/HDMI mode
#define ATOM_S2_VRI_BRIGHT_ENABLEb3     0x20
#define ATOM_S2_ROTATION_STATE_MASKb3   0xC0


// BIOS_3_SCRATCH Definition
#define ATOM_S3_CRT1_ACTIVE             0x00000001L
#define ATOM_S3_LCD1_ACTIVE             0x00000002L
#define ATOM_S3_TV1_ACTIVE              0x00000004L
#define ATOM_S3_DFP1_ACTIVE             0x00000008L
#define ATOM_S3_CRT2_ACTIVE             0x00000010L
#define ATOM_S3_LCD2_ACTIVE             0x00000020L
#define ATOM_S3_DFP6_ACTIVE                     0x00000040L
#define ATOM_S3_DFP2_ACTIVE             0x00000080L
#define ATOM_S3_CV_ACTIVE               0x00000100L
#define ATOM_S3_DFP3_ACTIVE                     0x00000200L
#define ATOM_S3_DFP4_ACTIVE                     0x00000400L
#define ATOM_S3_DFP5_ACTIVE                     0x00000800L


#define ATOM_S3_DEVICE_ACTIVE_MASK      0x00000FFFL

#define ATOM_S3_LCD_FULLEXPANSION_ACTIVE         0x00001000L
#define ATOM_S3_LCD_EXPANSION_ASPEC_RATIO_ACTIVE 0x00002000L

#define ATOM_S3_CRT1_CRTC_ACTIVE        0x00010000L
#define ATOM_S3_LCD1_CRTC_ACTIVE        0x00020000L
#define ATOM_S3_TV1_CRTC_ACTIVE         0x00040000L
#define ATOM_S3_DFP1_CRTC_ACTIVE        0x00080000L
#define ATOM_S3_CRT2_CRTC_ACTIVE        0x00100000L
#define ATOM_S3_LCD2_CRTC_ACTIVE        0x00200000L
#define ATOM_S3_DFP6_CRTC_ACTIVE        0x00400000L
#define ATOM_S3_DFP2_CRTC_ACTIVE        0x00800000L
#define ATOM_S3_CV_CRTC_ACTIVE          0x01000000L
#define ATOM_S3_DFP3_CRTC_ACTIVE            0x02000000L
#define ATOM_S3_DFP4_CRTC_ACTIVE            0x04000000L
#define ATOM_S3_DFP5_CRTC_ACTIVE            0x08000000L


#define ATOM_S3_DEVICE_CRTC_ACTIVE_MASK 0x0FFF0000L
#define ATOM_S3_ASIC_GUI_ENGINE_HUNG    0x20000000L
//Below two definitions are not supported in pplib, but in the old powerplay in DAL
#define ATOM_S3_ALLOW_FAST_PWR_S