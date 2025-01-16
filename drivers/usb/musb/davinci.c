ATOR INIT sequece through I2C -> ATOM_I2C_VOLTAGE_OBJECT_V3
#define VOLTAGE_OBJ_PHASE_LUT                4        //Set Vregulator Phase lookup table ->ATOM_GPIO_VOLTAGE_OBJECT_V3
#define VOLTAGE_OBJ_SVID2                    7        //Indicate voltage control by SVID2 ->ATOM_SVID2_VOLTAGE_OBJECT_V3
#define VOLTAGE_OBJ_EVV                      8
#define VOLTAGE_OBJ_PWRBOOST_LEAKAGE_LUT     0x10     //Powerboost Voltage and LeakageId lookup table->ATOM_LEAKAGE_VOLTAGE_OBJECT_V3
#define VOLTAGE_OBJ_HIGH_STATE_LEAKAGE_LUT   0x11     //High voltage state Voltage and LeakageId lookup table->ATOM_LEAKAGE_VOLTAGE_OBJECT_V3
#define VOLTAGE_OBJ_HIGH1_STATE_LEAKAGE_LUT  0x12     //High1 voltage state Voltage and LeakageId lookup table->ATOM_LEAKAGE_VOLTAGE_OBJECT_V3

typedef struct  _VOLTAGE_LUT_ENTRY_V2
{
  ULONG   ulVoltageId;                       // The Voltage ID which is used to program GPIO register
  USHORT  usVoltageValue;                    // The corresponding Voltage Value, in mV
}VOLTAGE_LUT_ENTRY_V2;

typedef struct  _LEAKAGE_VOLTAGE_LUT_ENTRY_V2
{
  USHORT  usVoltageLevel;                    // The Voltage ID which is used to program GPIO register
  USHORT  usVoltageId;
  USHORT  usLeakageId;                       // The corresponding Voltage Value, in mV
}LEAKAGE_VOLTAGE_LUT_ENTRY_V2;


typedef struct  _ATOM_I2C_VOLTAGE_OBJECT_V3
{
   ATOM_VOLTAGE_OBJECT_HEADER_V3 sHeader;    // voltage mode = VOLTAGE_OBJ_VR_I2C_INIT_SEQ
   UCHAR  ucVoltageRegulatorId;              //Indicate Voltage Regulator Id
   UCHAR  ucVoltageControlI2cLine;
   UCHAR  ucVoltageControlAddress;
   UCHAR  ucVoltageControlOffset;
   UCHAR  ucVoltageControlFlag;              // Bit0: 0 - One byte data; 1 - Two byte data
   UCHAR  ulReserved[3];
   VOLTAGE_LUT_ENTRY asVolI2cLut[1];         // end with 0xff
}ATOM_I2C_VOLTAGE_OBJECT_V3;

// ATOM_I2C_VOLTAGE_OBJECT_V3.ucVoltageControlFlag
#define VOLTAGE_DATA_ONE_BYTE                0
#define VOLTAGE_DATA_TWO_BYTE                1

typedef struct  _ATOM_GPIO_VOLTAGE_OBJECT_V3
{
   ATOM_VOLTAGE_OBJECT_HEADER_V3 sHeader;    // voltage mode = VOLTAGE_OBJ_GPIO_LUT or VOLTAGE_OBJ_PHASE_LUT
   UCHAR  ucVoltageGpioCntlId;               // default is 0 which indicate control through CG VID mode
   UCHAR  ucGpioEntryNum;                    // indiate the entry numbers of Votlage/Gpio value Look up table
   UCHAR  ucPhaseDelay;                      // phase delay in unit of micro second
   UCHAR  ucReserved;
   ULONG  ulGpioMaskVal;                     // GPIO Mask value
   VOLTAGE_LUT_ENTRY_V2 asVolGpioLut[1];
}ATOM_GPIO_VOLTAGE_OBJECT_V3;

typedef struct  _ATOM_LEAKAGE_VOLTAGE_OBJECT_V3
{
   ATOM_VOLTAGE_OBJECT_HEADER_V3 sHeader;    // voltage mode = 0x10/0x11/0x12
   UCHAR    ucLeakageCntlId;                 // default is 0
   UCHAR    ucLeakageEntryNum;               // indicate the entry number of LeakageId/Voltage Lut table
   UCHAR    ucReserved[2];
   ULONG    ulMaxVoltageLevel;
   LEAKAGE_VOLTAGE_LUT_ENTRY_V2 asLeakageIdLut[1];
}ATOM_LEAKAGE_VOLTAGE_OBJECT_V3;


typedef struct  _ATOM_SVID2_VOLTAGE_OBJECT_V3
{
   ATOM_VOLTAGE_OBJECT_HEADER_V3 sHeader;    // voltage mode = VOLTAGE_OBJ_SVID2
// 14:7 � PSI0_VID
// 6 � PSI0_EN
// 5 � PSI1
// 4:2 � load line slope trim.
// 1:0 � offset trim,
   USHORT   usLoadLine_PSI;
// GPU GPIO pin Id to SVID2 regulator VRHot pin. possible value 0~31. 0 means GPIO0, 31 means GPIO31
   UCHAR    ucSVDGpioId;     //0~31 indicate GPIO0~31
   UCHAR    ucSVCGpioId;     //0~31 indicate GPIO0~31
   ULONG    ulReserved;
}ATOM_SVID2_VOLTAGE_OBJECT_V3;

typedef union _ATOM_VOLTAGE_OBJECT_V3{
  ATOM_GPIO_VOLTAGE_OBJECT_V3 asGpioVoltageObj;
  ATOM_I2C_VOLTAGE_OBJECT_V3 asI2cVoltageObj;
  ATOM_LEAKAGE_VOLTAGE_OBJECT_V3 asLeakageObj;
  ATOM_SVID2_VOLTAGE_OBJECT_V3 asSVID2Obj;
}ATOM_VOLTAGE_OBJECT_V3;

typedef struct  _ATOM_VOLTAGE_OBJECT_INFO_V3_1
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ATOM_VOLTAGE_OBJECT_V3     asVoltageObj[3];   //Info for Voltage control
}ATOM_VOLTAGE_OBJECT_INFO_V3_1;


typedef struct  _ATOM_ASIC_PROFILE_VOLTAGE
{
   UCHAR    ucProfileId;
   UCHAR    ucReserved;
   USHORT   usSize;
   USHORT   usEfuseSpareStartAddr;
   USHORT   usFuseIndex[8];                                    //from LSB to MSB, Max 8bit,end of 0xffff if less than 8 efuse id,
   ATOM_LEAKID_VOLTAGE               asLeakVol[2];         //Leakid and relatd voltage
}ATOM_ASIC_PROFILE_VOLTAGE;

//ucProfileId
#define   ATOM_ASIC_PROFILE_ID_EFUSE_VOLTAGE                     1
#define   ATOM_ASIC_PROFILE_ID_EFUSE_PERFORMANCE_VOLTAGE         1
#define   ATOM_ASIC_PROFILE_ID_EFUSE_THERMAL_VOLTAGE             2

typedef struct  _ATOM_ASIC_PROFILING_INFO
{
  ATOM_COMMON_TABLE_HEADER         asHeader;
  ATOM_ASIC_PROFILE_VOLTAGE        asVoltage;
}ATOM_ASIC_PROFILING_INFO;

typedef struct  _ATOM_ASIC_PROFILING_INFO_V2_1
{
  ATOM_COMMON_TABLE_HEADER         asHeader;
  UCHAR  ucLeakageBinNum;                // indicate the entry number of LeakageId/Voltage Lut table
  USHORT usLeakageBinArrayOffset;        // offset of USHORT Leakage Bin list array ( from lower LeakageId to higher)

  UCHAR  ucElbVDDC_Num;
  USHORT usElbVDDC_IdArrayOffset;        // offset of USHORT virtual VDDC voltage id ( 0xff01~0xff08 )
  USHORT usElbVDDC_LevelArrayOffset;     // offset of 2 dimension voltage level USHORT array

  UCHAR  ucElbVDDCI_Num;
  USHORT usElbVDDCI_IdArrayOffset;       // offset of USHORT virtual VDDCI voltage id ( 0xff01~0xff08 )
  USHORT usElbVDDCI_LevelArrayOffset;    // offset of 2 dimension voltage level USHORT array
}ATOM_ASIC_PROFILING_INFO_V2_1;


//Here is parameter to convert Efuse value to Measure value
//Measured = LN((2^Bitsize-1)/EFUSE-1)*(Range)/(-alpha)+(Max+Min)/2
typedef struct _EFUSE_LOGISTIC_FUNC_PARAM
{
  USHORT usEfuseIndex;                  // Efuse Index in DWORD address, for example Index 911, usEuseIndex=112
  UCHAR  ucEfuseBitLSB;                 // Efuse bit LSB in DWORD address, for example Index 911, usEfuseBitLSB= 911-112*8=15
  UCHAR  ucEfuseLength;                 // Efuse bits length,
  ULONG  ulEfuseEncodeRange;            // Range = Max - Min, bit31 indicate the efuse is negative number
  ULONG  ulEfuseEncodeAverage;          // Average = ( Max + Min )/2
}EFUSE_LOGISTIC_FUNC_PARAM;

//Linear Function: Measured = Round ( Efuse * ( Max-Min )/(2^BitSize -1 ) + Min )
typedef struct _EFUSE_LINEAR_FUNC_PARAM
{
  USHORT usEfuseIndex;                  // Efuse Index in DWORD address, for example Index 911, usEuseIndex=112
  UCHAR  ucEfuseBitLSB;                 // Efuse bit LSB in DWORD address, for example Index 911, usEfuseBitLSB= 911-112*8=15
  UCHAR  ucEfuseLength;                 // Efuse bits length,
  ULONG  ulEfuseEncodeRange;            // Range = Max - Min, bit31 indicate the efuse is negative number
  ULONG  ulEfuseMin;                    // Min
}EFUSE_LINEAR_FUNC_PARAM;


typedef struct  _ATOM_ASIC_PROFILING_INFO_V3_1
{
  ATOM_COMMON_TABLE_HEADER         asHeader;
  ULONG  ulEvvDerateTdp;
  ULONG  ulEvvDerateTdc;
  ULONG  ulBoardCoreTemp;
  ULONG  ulMaxVddc;
  ULONG  ulMinVddc;
  ULONG  ulLoadLineSlop;
  ULONG  ulLeakageTemp;
  ULONG  ulLeakageVoltage;
  EFUSE_LINEAR_FUNC_PARAM sCACm;
  EFUSE_LINEAR_FUNC_PARAM sCACb;
  EFUSE_LOGISTIC_FUNC_PARAM sKt_b;
  EFUSE_LOGISTIC_FUNC_PARAM sKv_m;
  EFUSE_LOGISTIC_FUNC_PARAM sKv_b;
  USHORT usLkgEuseIndex;
  UCHAR  ucLkgEfuseBitLSB;
  UCHAR  ucLkgEfuseLength;
  ULONG  ulLkgEncodeLn_MaxDivMin;
  ULONG  ulLkgEncodeMax;
  ULONG  ulLkgEncodeMin;
  ULONG  ulEfuseLogisticAlpha;
  USHORT usPowerDpm0;
  USHORT usCurrentDpm0;
  USHORT usPowerDpm1;
  USHORT usCurrentDpm1;
  USHORT usPowerDpm2;
  USHORT usCurrentDpm2;
  USHORT usPowerDpm3;
  USHORT usCurrentDpm3;
  USHORT usPowerDpm4;
  USHORT usCurrentDpm4;
  USHORT usPowerDpm5;
  USHORT usCurrentDpm5;
  USHORT usPowerDpm6;
  USHORT usCurrentDpm6;
  USHORT usPowerDpm7;
  USHORT usCurrentDpm7;
}ATOM_ASIC_PROFILING_INFO_V3_1;


typedef struct  _ATOM_ASIC_PROFILING_INFO_V3_2
{
  ATOM_COMMON_TABLE_HEADER         asHeader;
  ULONG  ulEvvLkgFactor;
  ULONG  ulBoardCoreTemp;
  ULONG  ulMaxVddc;
  ULONG  ulMinVddc;
  ULONG  ulLoadLineSlop;
  ULONG  ulLeakageTemp;
  ULONG  ulLeakageVoltage;
  EFUSE_LINEAR_FUNC_PARAM sCACm;
  EFUSE_LINEAR_FUNC_PARAM sCACb;
  EFUSE_LOGISTIC_FUNC_PARAM sKt_b;
  EFUSE_LOGISTIC_FUNC_PARAM sKv_m;
  EFUSE_LOGISTIC_FUNC_PARAM sKv_b;
  USHORT usLkgEuseIndex;
  UCHAR  ucLkgEfuseBitLSB;
  UCHAR  ucLkgEfuseLength;
  ULONG  ulLkgEncodeLn_MaxDivMin;
  ULONG  ulLkgEncodeMax;
  ULONG  ulLkgEncodeMin;
  ULONG  ulEfuseLogisticAlpha;
  USHORT usPowerDpm0;
  USHORT usPowerDpm1;
  USHORT usPowerDpm2;
  USHORT usPowerDpm3;
  USHORT usPowerDpm4;
  USHORT usPowerDpm5;
  USHORT usPowerDpm6;
  USHORT usPowerDpm7;
  ULONG  ulTdpDerateDPM0;
  ULONG  ulTdpDerateDPM1;
  ULONG  ulTdpDerateDPM2;
  ULONG  ulTdpDerateDPM3;
  ULONG  ulTdpDerateDPM4;
  ULONG  ulTdpDerateDPM5;
  ULONG  ulTdpDerateDPM6;
  ULONG  ulTdpDerateDPM7;
}ATOM_ASIC_PROFILING_INFO_V3_2;


// for Tonga/Fiji speed EVV algorithm
typedef struct  _ATOM_ASIC_PROFILING_INFO_V3_3
{
  ATOM_COMMON_TABLE_HEADER         asHeader;
  ULONG  ulEvvLkgFactor;
  ULONG  ulBoardCoreTemp;
  ULONG  ulMaxVddc;
  ULONG  ulMinVddc;
  ULONG  ulLoadLineSlop;
  ULONG  ulLeakageTemp;
  ULONG  ulLeakageVoltage;
  EFUSE_LINEAR_FUNC_PARAM sCACm;
  EFUSE_LINEAR_FUNC_PARAM sCACb;
  EFUSE_LOGISTIC_FUNC_PARAM sKt_b;
  EFUSE_LOGISTIC_FUNC_PARAM sKv_m;
  EFUSE_LOGISTIC_FUNC_PARAM sKv_b;
  USHORT usLkgEuseIndex;
  UCHAR  ucLkgEfuseBitLSB;
  UCHAR  ucLkgEfuseLength;
  ULONG  ulLkgEncodeLn_MaxDivMin;
  ULONG  ulLkgEncodeMax;
  ULONG  ulLkgEncodeMin;
  ULONG  ulEfuseLogisticAlpha;
  USHORT usPowerDpm0;
  USHORT usPowerDpm1;
  USHORT usPowerDpm2;
  USHORT usPowerDpm3;
  USHORT usPowerDpm4;
  USHORT usPowerDpm5;
  USHORT usPowerDpm6;
  USHORT usPowerDpm7;
  ULONG  ulTdpDerateDPM0;
  ULONG  ulTdpDerateDPM1;
  ULONG  ulTdpDerateDPM2;
  ULONG  ulTdpDerateDPM3;
  ULONG  ulTdpDerateDPM4;
  ULONG  ulTdpDerateDPM5;
  ULONG  ulTdpDerateDPM6;
  ULONG  ulTdpDerateDPM7;
  EFUSE_LINEAR_FUNC_PARAM sRoFuse;
  ULONG  ulRoAlpha;
  ULONG  ulRoBeta;
  ULONG  ulRoGamma;
  ULONG  ulRoEpsilon;
  ULONG  ulATermRo;
  ULONG  ulBTermRo;
  ULONG  ulCTermRo;
  ULONG  ulSclkMargin;
  ULONG  ulFmaxPercent;
  ULONG  ulCRPercent;
  ULONG  ulSFmaxPercent;
  ULONG  ulSCRPercent;
  ULONG  ulSDCMargine;
}ATOM_ASIC_PROFILING_INFO_V3_3;

typedef struct _ATOM_POWER_SOURCE_OBJECT
{
   UCHAR  ucPwrSrcId;                                   // Power source
   UCHAR  ucPwrSensorType;                              // GPIO, I2C or none
   UCHAR  ucPwrSensId;                                  // if GPIO detect, it is GPIO id,  if I2C detect, it is I2C id
   UCHAR  ucPwrSensSlaveAddr;                           // Slave address if I2C detect
   UCHAR  ucPwrSensRegIndex;                            // I2C register Index if I2C detect
   UCHAR  ucPwrSensRegBitMask;                          // detect which bit is used if I2C detect
   UCHAR  ucPwrSensActiveState;                         // high active or low active
   UCHAR  ucReserve[3];                                 // reserve
   USHORT usSensPwr;                                    // in unit of watt
}ATOM_POWER_SOURCE_OBJECT;

typedef struct _ATOM_POWER_SOURCE_INFO
{
      ATOM_COMMON_TABLE_HEADER      asHeader;
      UCHAR                                    asPwrbehave[16];
      ATOM_POWER_SOURCE_OBJECT      asPwrObj[1];
}ATOM_POWER_SOURCE_INFO;


//Define ucPwrSrcId
#define POWERSOURCE_PCIE_ID1                  0x00
#define POWERSOURCE_6PIN_CONNECTOR_ID1   0x01
#define POWERSOURCE_8PIN_CONNECTOR_ID1   0x02
#define POWERSOURCE_6PIN_CONNECTOR_ID2   0x04
#define POWERSOURCE_8PIN_CONNECTOR_ID2   0x08

//define ucPwrSensorId
#define POWER_SENSOR_ALWAYS                     0x00
#define POWER_SENSOR_GPIO                        0x01
#define POWER_SENSOR_I2C                        0x02

typedef struct _ATOM_CLK_VOLT_CAPABILITY
{
  ULONG      ulVoltageIndex;                      // The Voltage Index indicated by FUSE, same voltage index shared with SCLK DPM fuse table
  ULONG      ulMaximumSupportedCLK;               // Maximum clock supported with specified voltage index, unit in 10kHz
}ATOM_CLK_VOLT_CAPABILITY;


typedef struct _ATOM_CLK_VOLT_CAPABILITY_V2
{
  USHORT     usVoltageLevel;                      // The real Voltage Level round up value in unit of mv,
  ULONG      ulMaximumSupportedCLK;               // Maximum clock supported with specified voltage index, unit in 10kHz
}ATOM_CLK_VOLT_CAPABILITY_V2;

typedef struct _ATOM_AVAILABLE_SCLK_LIST
{
  ULONG      ulSupportedSCLK;               // Maximum clock supported with specified voltage index,  unit in 10kHz
  USHORT     usVoltageIndex;                // The Voltage Index indicated by FUSE for specified SCLK
  USHORT     usVoltageID;                   // The Voltage ID indicated by FUSE for specified SCLK
}ATOM_AVAILABLE_SCLK_LIST;

// ATOM_INTEGRATED_SYSTEM_INFO_V6 ulSystemConfig cap definition
#define ATOM_IGP_INFO_V6_SYSTEM_CONFIG__PCIE_POWER_GATING_ENABLE             1       // refer to ulSystemConfig bit[0]

// this IntegrateSystemInfoTable is used for Liano/Ontario APU
typedef struct _ATOM_INTEGRATED_SYSTEM_INFO_V6
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ULONG  ulBootUpEngineClock;
  ULONG  ulDentistVCOFreq;
  ULONG  ulBootUpUMAClock;
  ATOM_CLK_VOLT_CAPABILITY   sDISPCLK_Voltage[4];
  ULONG  ulBootUpReqDisplayVector;
  ULONG  ulOtherDisplayMisc;
  ULONG  ulGPUCapInfo;
  ULONG  ulSB_MMIO_Base_Addr;
  USHORT usRequestedPWMFreqInHz;
  UCHAR  ucHtcTmpLmt;
  UCHAR  ucHtcHystLmt;
  ULONG  ulMinEngineClock;
  ULONG  ulSystemConfig;
  ULONG  ulCPUCapInfo;
  USHORT usNBP0Voltage;
  USHORT usNBP1Voltage;
  USHORT usBootUpNBVoltage;
  USHORT usExtDispConnInfoOffset;
  USHORT usPanelRefreshRateRange;
  UCHAR  ucMemoryType;
  UCHAR  ucUMAChannelNumber;
  ULONG  ulCSR_M3_ARB_CNTL_DEFAULT[10];
  ULONG  ulCSR_M3_ARB_CNTL_UVD[10];
  ULONG  ulCSR_M3_ARB_CNTL_FS3D[10];
  ATOM_AVAILABLE_SCLK_LIST   sAvail_SCLK[5];
  ULONG  ulGMCRestoreResetTime;
  ULONG  ulMinimumNClk;
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
  ULONG  SclkDpmBoostMargin;
  ULONG  SclkDpmThrottleMargin;
  USHORT SclkDpmTdpLimitPG;
  USHORT SclkDpmTdpLimitBoost;
  ULONG  ulBoostEngineCLock;
  UCHAR  ulBoostVid_2bit;
  UCHAR  EnableBoost;
  USHORT GnbTdpLimit;
  USHORT usMaxLVDSPclkFreqInSingleLink;
  UCHAR  ucLvdsMisc;
  UCHAR  ucLVDSReserved;
  ULONG  ulReserved3[15];
  ATOM_EXTERNAL_DISPLAY_CONNECTION_INFO sExtDispConnInfo;
}ATOM_INTEGRATED_SYSTEM_INFO_V6;

// ulGPUCapInfo
#define INTEGRATED_SYSTEM_INFO_V6_GPUCAPINFO__TMDSHDMI_COHERENT_SINGLEPLL_MODE       0x01
#define INTEGRATED_SYSTEM_INFO_V6_GPUCAPINFO__DISABLE_AUX_HW_MODE_DETECTION          0x08

//ucLVDSMisc:
#define SYS_INFO_LVDSMISC__888_FPDI_MODE                                             0x01
#define SYS_INFO_LVDSMISC__DL_CH_SWAP                                                0x02
#define SYS_INFO_LVDSMISC__888_BPC                                                   0x04
#define SYS_INFO_LVDSMISC__OVERRIDE_EN                                               0x08
#define SYS_INFO_LVDSMISC__BLON_ACTIVE_LOW                                           0x10
// new since Trinity
#define SYS_INFO_LVDSMISC__TRAVIS_LVDS_VOL_OVERRIDE_EN                               0x20

// not used any more
#define SYS_INFO_LVDSMISC__VSYNC_ACTIVE_LOW                                          0x04
#define SYS_INFO_LVDSMISC__HSYNC_ACTIVE_LOW                                          0x08

/**********************************************************************************************************************
  ATOM_INTEGRATED_SYSTEM_INFO_V6 Description
ulBootUpEngineClock:              VBIOS bootup Engine clock frequency, in 10kHz unit. if it is equal 0, then VBIOS use pre-defined bootup engine clock
ulDentistVCOFreq:                 Dentist VCO clock in 10kHz unit.
ulBootUpUMAClock:                 System memory boot up clock frequency in 10Khz unit.
sDISPCLK_Voltage:                 Report Display clock voltage requirement.

ulBootUpReqDisplayVector:         VBIOS boot up display IDs, following are supported devices in Liano/Ontaio projects:
                                  ATOM_DEVICE_CRT1_SUPPORT                  0x0001
                                  ATOM_DEVICE_CRT2_SUPPORT                  0x0010
                                  ATOM_DEVICE_DFP1_SUPPORT                  0x0008
                                  ATOM_DEVICE_DFP6_SUPPORT                  0x0040
                                  ATOM_DEVICE_DFP2_SUPPORT                  0x0080
                                  ATOM_DEVICE_DFP3_SUPPORT                  0x0200
                                  ATOM_DEVICE_DFP4_SUPPORT                  0x0400
                                  ATOM_DEVICE_DFP5_SUPPORT                  0x0800
                                  ATOM_DEVICE_LCD1_SUPPORT                  0x0002
ulOtherDisplayMisc:                 Other display related flags, not defined yet.
ulGPUCapInfo:                     bit[0]=0: TMDS/HDMI Coherent Mode use cascade PLL mode.
                                        =1: TMDS/HDMI Coherent Mode use signel PLL mode.
                                  bit[3]=0: Enable HW AUX mode detection logic
                                        =1: Disable HW AUX mode d