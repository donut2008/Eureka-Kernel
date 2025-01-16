 of GNB voltage configured by SBIOS, which is suffcient to support VBIOS DISPCLK requirement.
usExtDispConnInfoOffset:          Offset to sExtDispConnInfo inside the structure
usPanelRefreshRateRange:          Bit vector for LCD supported refresh rate range. If DRR is requestd by the platform, at least two bits need to be set
                                  to indicate a range.
                                  SUPPORTED_LCD_REFRESHRATE_30Hz          0x0004
                                  SUPPORTED_LCD_REFRESHRATE_40Hz          0x0008
                                  SUPPORTED_LCD_REFRESHRATE_50Hz          0x0010
                                  SUPPORTED_LCD_REFRESHRATE_60Hz          0x0020
ucMemoryType:                     [3:0]=1:DDR1;=2:DDR2;=3:DDR3.[7:4] is reserved.
ucUMAChannelNumber:                 System memory channel numbers.
ulCSR_M3_ARB_CNTL_DEFAULT[10]:    Arrays with values for CSR M3 arbiter for default
ulCSR_M3_ARB_CNTL_UVD[10]:        Arrays with values for CSR M3 arbiter for UVD playback.
ulCSR_M3_ARB_CNTL_FS3D[10]:       Arrays with values for CSR M3 arbiter for Full Screen 3D applications.
sAvail_SCLK[5]:                   Arrays to provide availabe list of SLCK and corresponding voltage, order from low to high
ulGMCRestoreResetTime:            GMC power restore and GMC reset time to calculate data reconnection latency. Unit in ns.
ulMinimumNClk:                    Minimum NCLK speed among all NB-Pstates to calcualte data reconnection latency. Unit in 10kHz.
ulIdleNClk:                       NCLK speed while memory runs in self-refresh state. Unit in 10kHz.
ulDDR_DLL_PowerUpTime:            DDR PHY DLL power up time. Unit in ns.
ulDDR_PLL_PowerUpTime:            DDR PHY PLL power up time. Unit in ns.
usPCIEClkSSPercentage:            PCIE Clock Spred Spectrum Percentage in unit 0.01%; 100 mean 1%.
usPCIEClkSSType:                  PCIE Clock Spred Spectrum Type. 0 for Down spread(default); 1 for Center spread.
usLvdsSSPercentage:               LVDS panel ( not include eDP ) Spread Spectrum Percentage in unit of 0.01%, =0, use VBIOS default setting.
usLvdsSSpreadRateIn10Hz:          LVDS panel ( not include eDP ) Spread Spectrum frequency in unit of 10Hz, =0, use VBIOS default setting.
usHDMISSPercentage:               HDMI Spread Spectrum Percentage in unit 0.01%; 100 mean 1%,  =0, use VBIOS default setting.
usHDMISSpreadRateIn10Hz:          HDMI Spread Spectrum frequency in unit of 10Hz,  =0, use VBIOS default setting.
usDVISSPercentage:                DVI Spread Spectrum Percentage in unit 0.01%; 100 mean 1%,  =0, use VBIOS default setting.
usDVISSpreadRateIn10Hz:           DVI Spread Spectrum frequency in unit of 10Hz,  =0, use VBIOS default setting.
usMaxLVDSPclkFreqInSingleLink:    Max pixel clock LVDS panel single link, if=0 means VBIOS use default threhold, right now it is 85Mhz
ucLVDSMisc:                       [bit0] LVDS 888bit panel mode =0: LVDS 888 panel in LDI mode, =1: LVDS 888 panel in FPDI mode
                                  [bit1] LVDS panel lower and upper link mapping =0: lower link and upper link not swap, =1: lower link and upper link are swapped
                                  [bit2] LVDS 888bit per color mode  =0: 666 bit per color =1:888 bit per color
                                  [bit3] LVDS parameter override enable  =0: ucLvdsMisc parameter are not used =1: ucLvdsMisc parameter should be used
                                  [bit4] Polarity of signal sent to digital BLON output pin. =0: not inverted(active high) =1: inverted ( active low )
**********************************************************************************************************************/

// this Table is used for Liano/Ontario APU
typedef struct _ATOM_FUSION_SYSTEM_INFO_V1
{
  ATOM_INTEGRATED_SYSTEM_INFO_V6    sIntegratedSysInfo;
  ULONG  ulPowerplayTable[128];
}ATOM_FUSION_SYSTEM_INFO_V1;


typedef struct _ATOM_TDP_CONFIG_BITS
{
#if ATOM_BIG_ENDIAN
  ULONG   uReserved:2;
  ULONG   uTDP_Value:14;  // Original TDP value in tens of milli watts
  ULONG   uCTDP_Value:14; // Override value in tens of milli watts
  ULONG   uCTDP_Enable:2; // = (uCTDP_Value > uTDP_Value? 2: (uCTDP_Value < uTDP_Value))
#else
  ULONG   uCTDP_Enable:2; // = (uCTDP_Value > uTDP_Value? 2: (uCTDP_Value < uTDP_Value))
  ULONG   uCTDP_Value:14; // Override value in tens of milli watts
  ULONG   uTDP_Value:14;  // Original TDP value in tens of milli watts
  ULONG   uReserved:2;
#endif
}ATOM_TDP_CONFIG_BITS;

typedef union _ATOM_TDP_CONFIG
{
  ATOM_TDP_CONFIG_BITS TDP_config;
  ULONG            TDP_config_all;
}ATOM_TDP_CONFIG;

/**********************************************************************************************************************
  ATOM_FUSION_SYSTEM_INFO_V1 Description
sIntegratedSysInfo:               refer to ATOM_INTEGRATED_SYSTEM_INFO_V6 definition.
ulPowerplayTable[128]:            This 512 bytes memory is used to save ATOM_PPLIB_POWERPLAYTABLE3, starting form ulPowerplayTable[0]
**********************************************************************************************************************/

// this IntegrateSystemInfoTable is used for Trinity APU
typedef struct _ATOM_INTEGRATED_SYSTEM_INFO_V1_7
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
  UCHAR  strVBIOSMsg[40];
  ATOM_TDP_CONFIG  asTdpConfig;
  ULONG  ulReserved[19];
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
  USHORT usNBP2Voltage;
  USHORT usNBP3Voltage;
  ULONG  ulNbpStateNClkFreq[4];
  UCHAR  ucNBDPMEnable;
  UCHAR  ucReserved[3];
  UCHAR  ucDPMState0VclkFid;
  UCHAR  ucDPMState0DclkFid;
  UCHAR  ucDPMState1VclkFid;
  UCHAR  ucDPMState1DclkFid;
  UCHAR  ucDPMState2VclkFid;
  UCHAR  ucDPMState2DclkFid;
  UCHAR  ucDPMState3VclkFid;
  UCHAR  ucDPMState3DclkFid;
  ATOM_EXTERNAL_DISPLAY_CONNECTION_INFO sExtDispConnInfo;
}ATOM_INTEGRATED_SYSTEM_INFO_V1_7;

// ulOtherDisplayMisc
#define INTEGRATED_SYSTEM_INFO__GET_EDID_CALLBACK_FUNC_SUPPORT            0x01
#define INTEGRATED_SYSTEM_INFO__GET_BOOTUP_DISPLAY_CALLBACK_FUNC_SUPPORT  0x02
#define INTEGRATED_SYSTEM_INFO__GET_EXPANSION_CALLBACK_FUNC_SUPPORT       0x04
#define INTEGRATED_SYSTEM_INFO__FAST_BOOT_SUPPORT                         0x08

// ulGPUCapInfo
#define SYS_INFO_GPUCAPS__TMDSHDMI_COHERENT_SINGLEPLL_MODE                0x01
#define SYS_INFO_GPUCAPS__DP_SINGLEPLL_MODE                               0x02
#define SYS_INFO_GPUCAPS__DISABLE_AUX_MODE_DETECT                         0x08
#define SYS_INFO_GPUCAPS__ENABEL_DFS_BYPASS                               0x10
//ulGPUCapInfo[16]=1 indicate SMC firmware is able to support GNB fast resume function, so that driver can call SMC to program most of GNB register during resuming, from ML
#define SYS_INFO_GPUCAPS__GNB_FAST_RESUME_CAPABLE                         0x00010000

//ulGPUCapInfo[17]=1 indicate battery boost feature is enable, from ML
#define SYS_INFO_GPUCAPS__BATTERY_BOOST_ENABLE                            0x00020000

/**********************************************************************************************************************
  ATOM_INTEGRATED_SYSTEM_INFO_V1_7 Description
ulBootUpEngineClock:              VBIOS bootup Engine clock frequency, in 10kHz unit. if it is equal 0, then VBIOS use pre-defined bootup engine clock
ulDentistVCOFreq:                 Dentist VCO clock in 10kHz unit.
ulBootUpUMAClock:                 System memory boot up clock frequency in 10Khz unit.
sDISPCLK_Voltage:                 Report Display clock voltage requirement.

ulBootUpReqDisplayVector:         VBIOS boot up display IDs, following are supported devices in Trinity projects:
                                  ATOM_DEVICE_CRT1_SUPPORT                  0x0001
                                  ATOM_DEVICE_DFP1_SUPPORT                  0x0008
                                  ATOM_DEVICE_DFP6_SUPPORT                  0x0040
                                  ATOM_DEVICE_DFP2_SUPPORT                  0x0080
                                  ATOM_DEVICE_DFP3_SUPPORT                  0x0200
                                  ATOM_DEVICE_DFP4_SUPPORT                  0x0400
                                  ATOM_DEVICE_DFP5_SUPPORT                  0x0800
                                  ATOM_DEVICE_LCD1_SUPPORT                  0x0002
ulOtherDisplayMisc:                 bit[0]=0: INT15 callback function Get LCD EDID ( ax=4e08, bl=1b ) is not supported by SBIOS.
                                        =1: INT15 callback function Get LCD EDID ( ax=4e08, bl=1b ) is supported by SBIOS.
                                  bit[1]=0: INT15 callback function Get boot display( ax=4e08, bl=01h) is not supported by SBIOS
                                        =1: INT15 callback function Get boot display( ax=4e08, bl=01h) is supported by SBIOS
                                  bit[2]=0: INT15 callback function Get panel Expansion ( ax=4e08, bl=02h) is not supported by SBIOS
                                        =1: INT15 callback function Get panel Expansion ( ax=4e08, bl=02h) is supported by SBIOS
                                  bit[3]=0: VBIOS fast boot is disable
                                        =1: VBIOS fast boot is enable. ( VBIOS skip display device detection in every set mode if LCD panel is connect and LID is open)
ulGPUCapInfo:                     bit[0]=0: TMDS/HDMI Coherent Mode use cascade PLL mode.
                                        =1: TMDS/HDMI Coherent Mode use signel PLL mode.
                                  bit[1]=0: DP mode use cascade PLL mode ( New for Trinity )
                                        =1: DP mode use single PLL mode
                                  bit[3]=0: Enable AUX HW mode detection logic
                                        =1: Disable AUX HW mode detection logic

ulSB_MMIO_Base_Addr:              Physical Base address to SB MMIO space. Driver needs to initialize it for SMU usage.

usRequestedPWMFreqInHz:           When it's set to 0x0 by SBIOS: the LCD BackLight is not controlled by GPU(SW).
                                  Any attempt to change BL using VBIOS function or enable VariBri from PP table is not effective since ATOM_BIOS_INFO_BL_CONTROLLED_BY_GPU==0;

                                  When it's set to a non-zero frequency, the BackLight is controlled by GPU (SW) in one of two ways below:
                                  1. SW uses the GPU BL PWM output to control the BL, in chis case, this non-zero frequency determines what freq GPU should use;
                                  VBIOS will set up proper PWM frequency and ATOM_BIOS_INFO_BL_CONTROLLED_BY_GPU==1,as the result,
                                  Changing BL using VBIOS function is functional in both driver and non-driver present environment;
                                  and enabling VariBri under the driver environment from PP table is optional.

                                  2. SW uses other means to control BL (like DPCD),this non-zero frequency serves as a flag only indicating
                                  that BL control from GPU is expected.
                                  VBIOS will NOT set up PWM frequency but make ATOM_BIOS_INFO_BL_CONTROLLED_BY_GPU==1
                                  Changing BL using VBIOS function could be functional in both driver and non-driver present environment,but
                                  it's per platform
                                  and enabling VariBri under the driver environment from PP table is optional.

ucHtcTmpLmt:                      Refer to D18F3x64 bit[22:16], HtcTmpLmt.
                                  Threshold on value to enter HTC_active state.
ucHtcHystLmt:                     Refer to D18F3x64 bit[27:24], HtcHystLmt.
                                  To calculate threshold off value to exit HTC_active state, which is Threshold on vlaue minus ucHtcHystLmt.
ulMinEngineClock:                 Minimum SCLK allowed in 10kHz unit. This is calculated based on WRCK Fuse settings.
ulSystemConfig:                   Bit[0]=0: PCIE Power Gating Disabled
                                        =1: PCIE Power Gating Enabled
                                  Bit[1]=0: DDR-DLL shut-down feature disabled.
                                         1: DDR-DLL shut-down feature enabled.
                                  Bit[2]=0: DDR-PLL Power down feature disabled.
                                         1: DDR-PLL Power down feature enabled.
ulCPUCapInfo:                     TBD
usNBP0Voltage:                    VID for voltage on NB P0 State
usNBP1Voltage:                    VID for voltage on NB P1 State
usNBP2Voltage:                    VID for voltage on NB P2 State
usNBP3Voltage:                    VID for voltage on NB P3 State
usBootUpNBVoltage:                Voltage Index of GNB voltage configured by SBIOS, which is suffcient to support VBIOS DISPCLK requirement.
usExtDispConnInfoOffset:          Offset to sExtDispConnInfo inside the structure
usPanelRefreshRateRange:          Bit vector for LCD supported refresh rate range. If DRR is requestd by the platform, at least two bits need to be set
                                  to indicate a range.
                                  SUPPORTED_LCD_REFRESHRATE_30Hz          0x0004
                                  SUPPORTED_LCD_REFRESHRATE_40Hz          0x0008
                                  SUPPORTED_LCD_REFRESHRATE_50Hz          0x0010
                                  SUPPORTED_LCD_REFRESHRATE_60Hz          0x0020
ucMemoryType:                     [3:0]=1:DDR1;=2:DDR2;=3:DDR3.[7:4] is reserved.
ucUMAChannelNumber:                 System memory channel numbers.
ulCSR_M3_ARB_CNTL_DEFAULT[10]:    Arrays with values for CSR M3 arbiter for default
ulCSR_M3_ARB_CNTL_UVD[10]:        Arrays with values for CSR M3 arbiter for UVD playback.
ulCSR_M3_ARB_CNTL_FS3D[10]:       Arrays with values for CSR M3 arbiter for Full Screen 3D applications.
sAvail_SCLK[5]:                   Arrays to provide availabe list of SLCK and corresponding voltage, order from low to high
ulGMCRestoreResetTime:            GMC power restore and GMC reset time to calculate data reconnection latency. Unit in ns.
ulMinimumNClk:                    Minimum NCLK speed among all NB-Pstates to calcualte data reconnection latency. Unit in 10kHz.
ulIdleNClk:                       NCLK speed while memory runs in self-refresh state. Unit in 10kHz.
ulDDR_DLL_PowerUpTime:            DDR PHY DLL power up time. Unit in ns.
ulDDR_PLL_PowerUpTime:            DDR PHY PLL power up time. Unit in ns.
usPCIEClkSSPercentage:            PCIE Clock Spread Spectrum Percentage in unit 0.01%; 100 mean 1%.
usPCIEClkSSType:                  PCIE Clock Spread Spectrum Type. 0 for Down spread(default); 1 for Center spread.
usLvdsSSPercentage:               LVDS panel ( not include eDP ) Spread Spectrum Percentage in unit of 0.01%, =0, use VBIOS default setting.
usLvdsSSpreadRateIn10Hz:          LVDS panel ( not include eDP ) Spread Spectrum frequency in unit of 10Hz, =0, use VBIOS default setting.
usHDMISSPercentage:               HDMI Spread Spectrum Percentage in unit 0.01%; 100 mean 1%,  =0, use VBIOS default setting.
usHDMISSpreadRateIn10Hz:          HDMI Spread Spectrum frequency in unit of 10Hz,  =0, use VBIOS default setting.
usDVISSPercentage:                DVI Spread Spectrum Percentage in unit 0.01%; 100 mean 1%,  =0, use VBIOS default setting.
usDVISSpreadRateIn10Hz:           DVI Spread Spectrum frequency in unit of 10Hz,  =0, use VBIOS default setting.
usMaxLVDSPclkFreqInSingleLink:    Max pixel clock LVDS panel single link, if=0 means VBIOS use default threhold, right now it is 85Mhz
ucLVDSMisc:                       [bit0] LVDS 888bit panel mode =0: LVDS 888 panel in LDI mode, =1: LVDS 888 panel in FPDI mode
                                  [bit1] LVDS panel lower and upper link mapping =0: lower link and upper link not swap, =1: lower link and upper link are swapped
                                  [bit2] LVDS 888bit per color mode  =0: 666 bit per color =1:888 bit per color
                                  [bit3] LVDS parameter override enable  =0: ucLvdsMisc parameter are not used =1: ucLvdsMisc parameter should be used
                                  [bit4] Polarity of signal sent to digital BLON output pin. =0: not inverted(active high) =1: inverted ( active low )
                                  [bit5] Travid LVDS output voltage override enable, when =1, use ucTravisLVDSVolAdjust value to overwrite Traivs register LVDS_CTRL_4
ucTravisLVDSVolAdjust             When ucLVDSMisc[5]=1,it means platform SBIOS want to overwrite TravisLVDSVoltage. Then VBIOS will use ucTravisLVDSVolAdjust
                                  value to program Travis register LVDS_CTRL_4
ucLVDSPwrOnSeqDIGONtoDE_in4Ms:    LVDS power up sequence time in unit of 4ms, time delay from DIGON signal active to data enable signal active( DE ).
                                  =0 mean use VBIOS default which is 8 ( 32ms ). The LVDS power up sequence is as following: DIGON->DE->VARY_BL->BLON.
                                  This parameter is used by VBIOS only. VBIOS will patch LVDS_InfoTable.
ucLVDSPwrOnDEtoVARY_BL_in4Ms:     LVDS power up sequence time in unit of 4ms., time delay from DE( data enable ) active to Vary Brightness enable signal active( VARY_BL ).
                                  =0 mean use VBIOS default which is 90 ( 360ms ). The LVDS power up sequence is as following: DIGON->DE->VARY_BL->BLON.
                                  This parameter is used by VBIOS only. VBIOS will patch LVDS_InfoTable.

ucLVDSPwrOffVARY_BLtoDE_in4Ms:    LVDS power down sequence time in unit of 4ms, time delay from data enable ( DE ) signal off to LCDVCC (DIGON) off.
                                  =0 mean use VBIOS default delay which is 8 ( 32ms ). The LVDS power down sequence is as following: BLON->VARY_BL->DE->DIGON
                                  This parameter is used by VBIOS only. VBIOS will patch LVDS_InfoTable.

ucLVDSPwrOffDEtoDIGON_in4Ms:      LVDS power down sequence time in unit of 4ms, time delay from vary brightness enable signal( VARY_BL) off to data enable ( DE ) signal off.
                                  =0 mean use VBIOS default which is 90 ( 360ms ). The LVDS power down sequence is as following: BLON->VARY_BL->