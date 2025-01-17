0                                                         0xa4
#define mmSMC_MSG_ARG_1                                                         0xa5
#define mmSMC_MSG_ARG_2                                                         0xa6
#define mmSMC_MSG_ARG_3                                                         0xa7
#define mmSMC_MSG_ARG_4                                                         0xa8
#define mmSMC_MSG_ARG_5                                                         0xa9
#define mmSMC_MSG_ARG_6                                                         0xaa
#define mmSMC_MSG_ARG_7                                                         0xab
#define mmSMC_MESSAGE_8                                                         0xb5
#define mmSMC_RESP_8                                                            0xb6
#define mmSMC_MESSAGE_9                                                         0xb7
#define mmSMC_RESP_9                                                            0xb8
#define mmSMC_MESSAGE_10                                                        0xb9
#define mmSMC_RESP_10                                                           0xba
#define mmSMC_MESSAGE_11                                                        0xbb
#define mmSMC_RESP_11                                                           0xbc
#define mmSMC_MSG_ARG_8                                                         0xbd
#define mmSMC_MSG_ARG_9                                                         0xbe
#define mmSMC_MSG_ARG_10                                                        0xbf
#define mmSMC_MSG_ARG_11                                                        0x91
#define ixSMC_SYSCON_RESET_CNTL                                                 0x80000000
#define ixSMC_SYSCON_CLOCK_CNTL_0                                               0x80000004
#define ixSMC_SYSCON_CLOCK_CNTL_1                                               0x80000008
#define ixSMC_SYSCON_CLOCK_CNTL_2                                               0x8000000c
#define ixSMC_SYSCON_MISC_CNTL                                                  0x80000010
#define ixSMC_SYSCON_MSG_ARG_0                                                  0x80000068
#define ixSMC_PC_C                                                              0x80000370
#define ixSMC_SCRATCH9                                                          0x80000424
#define mmGPIOPAD_SW_INT_STAT                                                   0x180
#define mmGPIOPAD_STRENGTH                                                      0x181
#define mmGPIOPAD_MASK                                                          0x182
#define mmGPIOPAD_A                                                             0x183
#define mmGPIOPAD_EN                                                            0x184
#define mmGPIOPAD_Y                                                             0x185
#define mmGPIOPAD_PINSTRAPS                                                     0x186
#define mmGPIOPAD_INT_STAT_EN                                                   0x187
#define mmGPIOPAD_INT_STAT                                                      0x188
#define mmGPIOPAD_INT_STAT_AK                                                   0x189
#define mmGPIOPAD_INT_EN                                                        0x18a
#define mmGPIOPAD_INT_TYPE                                                      0x18b
#define mmGPIOPAD_INT_POLARITY                                                  0x18c
#define mmGPIOPAD_EXTERN_TRIG_CNTL                                              0x18d
#define mmGPIOPAD_RCVR_SEL                                                      0x191
#define mmGPIOPAD_PU_EN                                                         0x192
#define mmGPIOPAD_PD_EN                                                         0x193
#define mmCG_FPS_CNT                                                            0x1a4
#define mmSMU_SMC_IND_INDEX                                                     0x80
#define mmSMU0_SMU_SMC_IND_INDEX                                                0x80
#define mmSMU1_SMU_SMC_IND_INDEX                                                0x82
#define mmSMU2_SMU_SMC_IND_INDEX                                                0x84
#define mmSMU3_SMU_SMC_IND_INDEX                                                0x86
#define mmSMU_SMC_IND_DATA                                                      0x81
#define mmSMU0_SMU_SMC_IND_DATA                                                 0x81
#define mmSMU1_SMU_SMC_IND_DATA                                                 0x83
#define mmSMU2_SMU_SMC_IND_DATA                                                 0x85
#define mmSMU3_SMU_SMC_IND_DATA                                                 0x87
#define ixRCU_UC_EVENTS                                                         0xc0000004
#define ixRCU_MISC_CTRL                                                         0xc0000010
#define ixCC_RCU_FUSES                                                          0xc00c0000
#define ixCC_SMU_MISC_FUSES                                                     0xc00c0004
#define ixCC_SCLK_VID_FUSES                                                     0xc00c0008
#define ixCC_GIO_IOCCFG_FUSES                                                   0xc00c000c
#define ixCC_GIO_IOC_FUSES                                                      0xc00c0010
#define ixCC_SMU_TST_EFUSE1_MISC                                                0xc00c001c
#define ixCC_TST_ID_STRAPS                                                      0xc00c0020
#define ixCC_FCTRL_FUSES                                                        0xc00c0024
#define ixSMU_MAIN_PLL_OP_FREQ                                                  0xe0003020
#define ixSMU_STATUS                                                            0xe0003088
#define ixSMU_FIRMWARE                                                          0xe00030a4
#define ixSMU_INPUT_DATA                                                        0xe00030b8
#define ixSMU_EFUSE_0                                                           0xc0100000
#define ixDPM_TABLE_1                                                           0x3f000
#define ixDPM_TABLE_2                                                           0x3f004
#define ixDPM_TABLE_3                                                           0x3f008
#define ixDPM_TABLE_4                                                           0x3f00c
#define ixDPM_TABLE_5                                                           0x3f010
#define ixDPM_TABLE_6                                                           0x3f014
#define ixDPM_TABLE_7                                                           0x3f018
#define ixDPM_TABLE_8                                                           0x3f01c
#define ixDPM_TABLE_9                                                           0x3f020
#define ixDPM_TABLE_10                                                          0x3f024
#define ixDPM_TABLE_11                                                          0x3f028
#define ixDPM_TABLE_12                                                          0x3f02c
#define ixDPM_TABLE_13                                                          0x3f030
#define ixDPM_TABLE_14                                                          0x3f034
#define ixDPM_TABLE_15                                                          0x3f038
#define ixDPM_TABLE_16                                                          0x3f03c
#define ixDPM_TABLE_17                                                          0x3f040
#define ixDPM_TABLE_18                                                          0x3f044
#define ixDPM_TABLE_19                                                          0x3f048
#define ixDPM_TABLE_20                                                          0x3f04c
#define ixDPM_TABLE_21                                                          0x3f050
#define ixDPM_TABLE_22                                                          0x3f054
#define ixDPM_TABLE_23                                                          0x3f058
#define ixDPM_TABLE_24                                                          0x3f05c
#define ixDPM_TABLE_25                                                          0x3f060
#define ixDPM_TABLE_26                                                          0x3f064
#define ixDPM_TABLE_27                                                          0x3f068
#define ixDPM_TABLE_28                                                          0x3f06c
#define ixDPM_TABLE_29                                                          0x3f070
#define ixDPM_TABLE_30                                                          0x3f074
#define ixDPM_TABLE_31                                                          0x3f078
#define ixDPM_TABLE_32                                                          0x3f07c
#define ixDPM_TABLE_33                                                          0x3f080
#define ixDPM_TABLE_34                                                          0x3f084
#define ixDPM_TABLE_35                                                          0x3f088
#define ixDPM_TABLE_36                                                          0x3f08c
#define ixDPM_TABLE_37                                                          0x3f090
#define ixDPM_TABLE_38                                                          0x3f094
#define ixDPM_TABLE_39                                                          0x3f098
#define ixDPM_TABLE_40                                                          0x3f09c
#define ixDPM_TABLE_41                                                          0x3f0a0
#define ixDPM_TABLE_42                                                          0x3f0a4
#define ixDPM_TABLE_43                                                          0x3f0a8
#define ixDPM_TABLE_44                                                          0x3f0ac
#define ixDPM_TABLE_45                                                          0x3f0b0
#define ixDPM_TABLE_46                                                          0x3f0b4
#define ixDPM_TABLE_47                                                          0x3f0b8
#define ixDPM_TABLE_48                                                          0x3f0bc
#define ixDPM_TABLE_49                                                          0x3f0c0
#define ixDPM_TABLE_50                                                          0x3f0c4
#define ixDPM_TABLE_51                                                          0x3f0c8
#def