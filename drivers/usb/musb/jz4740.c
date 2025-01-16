FP6_SUPPORT                  0x0040
                                  ATOM_DEVICE_DFP2_SUPPORT                  0x0080
                                  ATOM_DEVICE_DFP3_SUPPORT                  0x0200
                                  ATOM_DEVICE_DFP4_SUPPORT                  0x0400
                                  ATOM_DEVICE_DFP5_SUPPORT                  0x0800
                                  ATOM_DEVICE_LCD1_SUPPORT                  0x0002

ulVBIOSMisc:                       Miscellenous flags for VBIOS requirement and interface
                                  bit[0]=0: INT15 callback function Get LCD EDID ( ax=4e08, bl=1b ) is not supported by SBIOS.
                                        =1: INT15 callback function Get LCD EDID ( ax=4e08, bl=1b ) is supported by SBIOS.
                                  bit[1]=0: INT15 callback function Get boot display( ax=4e08, bl=01h) is not supported by SBIOS
                                        =1: INT15 callback function Get boot display( ax=4e08, bl=01h) is supported by SBIOS
                                  bit[2]=0: INT15 callback function Get panel Expansion ( ax=4e08, bl=02h) is not supported by SBIOS
                                        =1: INT15 callback function Get panel Expansion ( ax=4e08, bl=02h) is supported by SBIOS
                                  bit[3]=0: VBIOS fast boot is disable
                                        =1: VBIOS fast boot is enable. ( VBIOS skip display device detection in every set mode if LCD panel is connect and LID is open)

ulGPUCapInfo:                     bit[0~2]= Reserved
                                  bit[3]=0: Enable AUX HW mode detection logic
                                        =1: Disable AUX HW mode detection logic
                                  bit[4]=0: Disable DFS bypass feature
                                        =1: Enable DFS bypass feature

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

ucHtcTmpLmt:                      Refer to D18F3x64 bit[22:16], HtcTmpLmt. Threshold on value to enter HTC_active state.
ucHtcHystLmt:                     Refer to D18F3x64 bit[27:24], HtcHystLmt.
                                  To calculate threshold off value to exit HTC_active state, which is Threshold on vlaue minus ucHtcHystLmt.

ulSystemConfig:                   Bit[0]=0: PCIE Power Gating Disabled
                                        =1: PCIE Power Gating Enabled
                                  Bit[1]=0: DDR-DLL shut-down feature disabled.
                                         1: DDR-DLL shut-down feature enabled.
                                  Bit[2]=0: DDR-PLL Power down feature disabled.
                                         1: DDR-PLL Power down feature enabled.
                                  Bit[3]=0: GNB DPM is disabled
                                        =1: GNB DPM is enabled
ulCPUCapInfo:                     TBD

usExtDispConnInfoOffset:          Offset to sExtDispConnInfo inside the structure
usPanelRefreshRateRange:          Bit vector for LCD supported refresh rate range. If DRR is requestd by the platform, at least two bits need to be set
                                  to indicate a range.
                                  SUPPORTED_LCD_REFRESHRATE_30Hz          0x0004
                                  SUPPORTED_LCD_REFRESHRATE_40Hz          0x0008
                                  SUPPORTED_LCD_REFRESHRATE_50Hz          0x0010
                                  SUPPORTED_LCD_REFRESHRATE_60Hz          0x0020

ucMemoryType:                     [3:0]=1:DDR1;=2:DDR2;=3:DDR3;=5:GDDR5; [7:4] is reserved.
ucUMAChannelNumber:                 System memory channel numbers.

strVBIOSMsg[40]:                  VBIOS boot up customized message string

sAvail_SCLK[5]:     