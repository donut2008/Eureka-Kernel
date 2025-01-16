OnSeqVARY_BLtoBLON_in4Ms:
                                  LVDS power up sequence time in unit of 4ms. Time delay from VARY_BL signal on to DLON signal active.
                                  =0 means to use VBIOS default delay which is 0 ( 0ms ).
                                  This parameter is used by VBIOS only. VBIOS will patch LVDS_InfoTable.

ucLVDSPwrOffSeqBLONtoVARY_BL_in4Ms:
                                  LVDS power down sequence time in unit of 4ms. Time delay from BLON signal off to VARY_BL signal off.
                                  =0 means to use VBIOS default delay which is 0 ( 0ms ).
                                  This parameter is used by VBIOS only. VBIOS will patch LVDS_InfoTable.

ucMinAllowedBL_Level:             Lowest LCD backlight PWM level. This is customer platform specific parameters. By default it is 0.

ulNbpStateMemclkFreq[4]:          system memory clock frequncey in unit of 10Khz in different NB pstate.

**********************************************************************************************************************/

// this IntegrateSystemInfoTable is used for Kaveri & Kabini APU
typedef struct _ATOM_INTEGRATED_SYSTEM_INFO_V1_8
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ULONG  ulBootUpEngineClock;
  ULONG  ulDentistVCOFreq;
  ULONG  ulBootUpUMAClock;
  ATOM_CLK_VOLT_CAPABILITY   sDISPCLK_Voltage[4];
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
  ULONG  ulReserved[19];
  ATOM_AVAILABLE_SCLK_LIST   sAvail_SCLK[5];
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
  ATOM_CLK_VOLT_CAPABILITY   s5thDISPCLK_Voltage;
  ULONG  ulReserved5;
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
  ULONG  ulNbpStateMemclkFreq[4];
  ULONG  ulPSPVersion;
  ULONG  ulNbpStateNClkFreq[4];
  USHORT usNBPStateVoltage[4];
  USHORT usBootUpNBVoltage;
  USHORT usReserved2;
  ATOM_EXTERNAL_DISPLAY_CONNECTION_INFO sExtDispConnInfo;
}ATOM_INTEGRATED_SYSTEM_INFO_V1_8;

/*************************************************