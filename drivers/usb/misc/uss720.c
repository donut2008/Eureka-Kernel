SK 0xf000
#define XDMA_SLV_READ_URGENT_CNTL__XDMA_SLV_READ_STALL_DELAY__SHIFT 0xc
#define XDMA_SLV_READ_URGENT_CNTL__XDMA_SLV_READ_URGENT_TIMER_MASK 0xffff0000
#define XDMA_SLV_READ_URGENT_CNTL__XDMA_SLV_READ_URGENT_TIMER__SHIFT 0x10
#define XDMA_SLV_WRITE_URGENT_CNTL__XDMA_SLV_WRITE_STALL_MASK 0x1
#define XDMA_SLV_WRITE_URGENT_CNTL__XDMA_SLV_WRITE_STALL__SHIFT 0x0
#define XDMA_SLV_WRITE_URGENT_CNTL__XDMA_SLV_WRITE_URGENT_LEVEL_MASK 0xf00
#define XDMA_SLV_WRITE_URGENT_CNTL__XDMA_SLV_WRITE_URGENT_LEVEL__SHIFT 0x8
#define XDMA_SLV_WRITE_URGENT_CNTL__XDMA_SLV_WRITE_STALL_DELAY_MASK 0xf000
#define XDMA_SLV_WRITE_URGENT_CNTL__XDMA_SLV_WRITE_STALL_DELAY__SHIFT 0xc
#define XDMA_SLV_WB_RATE_CNTL__XDMA_SLV_WB_BURST_SIZE_MASK 0x1ff
#define XDMA_SLV_WB_RATE_CNTL__XDMA_SLV_WB_BURST_SIZE__SHIFT 0x0
#define XDMA_SLV_WB_RATE_CNTL__XDMA_SLV_WB_BURST_PERIOD_MASK 0xffff0000
#define XDMA_SLV_WB_RATE_CNTL__XDMA_SLV_WB_BURST_PERIOD__SHIFT 0x10
#define XDMA_SLV_READ_LATENCY_MINMAX__XDMA_SLV_READ_LATENCY_MIN_MASK 0xffff
#define XDMA_SLV_READ_LATENCY_MINMAX__XDMA_SLV_READ_LATENCY_MIN__SHIFT 0x0
#define XDMA_SLV_READ_LATENCY_MINMAX__XDMA_SLV_READ_LATENCY_MAX_MASK 0xffff0000
#define XDMA_SLV_READ_LATENCY_MINMAX__XDMA_SLV_READ_LATENCY_MAX__SHIFT 0x10
#define XDMA_SLV_READ_LATENCY_AVE__XDMA_SLV_READ_LATENCY_ACC_MASK 0xfffff
#define XDMA_SLV_READ_LATENCY_AVE__XDMA_SLV_READ_LATENCY_ACC__SHIFT 0x0
#define XDMA_SLV_READ_LATENCY_AVE__XDMA_SLV_READ_LATENCY_COUNT_MASK 0xfff00000
#define XDMA_SLV_READ_LATENCY_AVE__XDMA_SLV_READ_LATENCY_COUNT__SHIFT 0x14
#define XDMA_SLV_PCIE_NACK_STATUS__XDMA_SLV_PCIE_NACK_TAG_MASK 0x3ff
#define XDMA_SLV_PCIE_NACK_STATUS__XDMA_SLV_PCIE_NACK_TAG__SHIFT 0x0
#define XDMA_SLV_PCIE_NACK_STATUS__XDMA_SLV_PCIE_NACK_MASK 0x3000
#define XDMA_SLV_PCIE_NACK_STATUS__XDMA_SLV_PCIE_NACK__SHIFT 0xc
#define XDMA_SLV_PCIE_NACK_STATUS__XDMA_SLV_PCIE_NACK_CLR_MASK 0x10000
#define XDMA_SLV_PCIE_NACK_STATUS__XDMA_SLV_PCIE_NACK_CLR__SHIFT 0x10
#define XDMA_SLV_MEM_NACK_STATUS__XDMA_SLV_MEM_NACK_TAG_MASK 0xffff
#define XDMA_SLV_MEM_NACK_STATUS__XDMA_SLV_MEM_NACK_TAG__SHIFT 0x0
#define XDMA_SLV_MEM_NACK_STATUS__XDMA_SLV_MEM_NACK_MASK 0x30000
#define XDMA_SLV_MEM_NACK_STATUS__XDMA_SLV_MEM_NACK__SHIFT 0x10
#define XDMA_SLV_MEM_NACK_STATUS__XDMA_SLV_MEM_NACK_CLR_MASK 0x80000000
#define XDMA_SLV_MEM_NACK_STATUS__XDMA_SLV_MEM_NACK_CLR__SHIFT 0x1f
#define XDMA_SLV_RDRET_BUF_STATUS__XDMA_SLV_RDRET_FREE_ENTRIES_MASK 0x3ff
#define XDMA_SLV_RDRET_BUF_STATUS__XDMA_SLV_RDRET_FREE_ENTRIES__SHIFT 0x0
#define XDMA_SLV_RDRET_BUF_STATUS__XDMA_SLV_RDRET_BUF_SIZE_MASK 0x3ff000
#define XDMA_SLV_RDRET_BUF_STATUS__XDMA_SLV_RDRET_BUF_SIZE__SHIFT 0xc
#define XDMA_SLV_RDRET_BUF_STATUS__XDMA_SLV_RDRET_PG_STATE_MASK 0xc00000
#define XDMA_SLV_RDRET_BUF_STATUS__XDMA_SLV_RDRET_PG_STATE__SHIFT 0x16
#define XDMA_SLV_RDRET_BUF_STATUS__XDMA_SLV_RDRET_PG_TRANS_MASK 0x1000000
#define XDMA_SLV_RDRET_BUF_STATUS__XDMA_SLV_RDRET_PG_TRANS__SHIFT 0x18
#define XDMA_SLV_READ_LATENCY_TIMER__XDMA_SLV_READ_LATENCY_TIMER_MASK 0xffff
#define XDMA_SLV_READ_LATENCY_TIMER__XDMA_SLV_READ_LATENCY_TIMER__SHIFT 0x0
#define XDMA_SLV_FLIP_PENDING__XDMA_SLV_FLIP_PENDING_MASK 0x1
#define XDMA_SLV_FLIP_PENDING__XDMA_SLV_FLIP_PENDING__SHIFT 0x0
#define XDMA_SLV_CHANNEL_CNTL__XDMA_SLV_CHANNEL_WEIGHT_MASK 0x1ff
#define XDMA_SLV_CHANNEL_CNTL__XDMA_SLV_CHANNEL_WEIGHT__SHIFT 0x0
#define XDMA_SLV_CHANNEL_CNTL__XDMA_SLV_STOP_TRANSFER_MASK 0x10000
#define XDMA_SLV_CHANNEL_CNTL__XDMA_SLV_STOP_TRANSFER__SHIFT 0x10
#define XDMA_SLV_CHANNEL_CNTL__XDMA_SLV_CHANNEL_SOFT_RESET_MASK 0x20000
#define XDMA_SLV_CHANNEL_CNTL__XDMA_SLV_CHANNEL_SOFT_RESET__SHIFT 0x11
#define XDMA_SLV_CHANNEL_CNTL__XDMA_SLV_CHANNEL_ACTIVE_MASK 0x1000000
#define XDMA_SLV_CHANNEL_CNTL__XDMA_SLV_CHANNEL_ACTIVE__SHIFT 0x18
#define XDMA_SLV_REMOTE_GPU_ADDRESS__XDMA_SLV_REMOTE_GPU_ADDRESS_MASK 0xffffffff
#define XDMA_SLV_REMOTE_GPU_ADDRESS__XDMA_SLV_REMOTE_GPU_ADDRESS__SHIFT 0x0
#define XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH__XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH_MASK 0xff
#define XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH__XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH__SHIFT 0x0

#endif /* DCE_10_0_SH_MASK_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * DCE_11_0 Register documentation
 *
 * Copyright (C) 2014  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DCE_11_0_D_H
#define DCE_11_0_D_H

#define mmPIPE0_PG_CONFIG                                                       0x2c0
#define mmPIPE0_PG_ENABLE                                                       0x2c1
#define mmPIPE0_PG_STATUS                                                       0x2c2
#define mmPIPE1_PG_CONFIG                                                       0x2c3
#define mmPIPE1_PG_ENABLE                                                       0x2c4
#define mmPIPE1_PG_STATUS                                                       0x2c5
#define mmPIPE2_PG_CONFIG                                                       0x2c6
#define mmPIPE2_PG_ENABLE                                                       0x2c7
#define mmPIPE2_PG_STATUS                                                       0x2c8
#define mmDCFEV0_PG_CONFIG                                                      0x2db
#define mmDCFEV0_PG_ENABLE                                                      0x2dc
#define mmDCFEV0_PG_STATUS                                                      0x2dd
#define mmDCPG_INTERRUPT_STATUS                                                 0x2de
#define mmDCPG_INTERRUPT_CONTROL                                                0x2df
#define mmDC_IP_REQUEST_CNTL                                                    0x2d2
#define mmDC_PGFSM_CONFIG_REG                                                   0x2d3
#define mmDC_PGFSM_WRITE_REG                                                    0x2d4
#define mmDC_PGCNTL_STATUS_REG                                                  0x2d5
#define mmDCPG_TEST_DEBUG_INDEX                                                 0x2d6
#define mmDCPG_TEST_DEBUG_DATA                                                  0x2d7
#define mmBL1_PWM_AMBIENT_LIGHT_LEVEL                                           0x1628
#define mmBL1_PWM_USER_LEVEL                                                    0x1629
#define mmBL1_PWM_TARGET_ABM_LEVEL                                              0x162a
#define mmBL1_PWM_CURRENT_ABM_LEVEL                                             0x162b
#define mmBL1_PWM_FINAL_DUTY_CYCLE                                              0x162c
#define mmBL1_PWM_MINIMUM_DUTY_CYCLE                                            0x162d
#define mmBL1_PWM_ABM_CNTL                                                      0x162e
#define mmBL1_PWM_BL_UPDATE_SAMPLE_RATE                                         0x162f
#define mmBL1_PWM_GRP2_REG_LOCK                                                 0x1630
#define mmDC_ABM1_CNTL                                                          0x1638
#define mmDC_ABM1_IPCSC_COEFF_SEL                                               0x1639
#define mmDC_ABM1_ACE_OFFSET_SLOPE_0                                            0x163a
#define mmDC_ABM1_ACE_OFFSET_SLOPE_1                                            0x163b
#define mmDC_ABM1_ACE_OFFSET_SLOPE_2                                            0x163c
#define mmDC_ABM1_ACE_OFFSET_SLOPE_3                                            0x163d
#define mmDC_ABM1_ACE_OFFSET_SLOPE_4                                            0x163e
#define mmDC_ABM1_ACE_THRES_12                                                  0x163f
#define mmDC_ABM1_ACE_THRES_34                                                  0x1640
#define mmDC_ABM1_ACE_CNTL_MISC                                                 0x1641
#define mmDC_ABM1_DEBUG_MISC                                                    0x1649
#define mmDC_ABM1_HGLS_REG_READ_PROGRESS                                        0x164a
#define mmDC_ABM1_HG_MISC_CTRL                                                  0x164b
#define mmDC_ABM1_LS_SUM_OF_LUMA                                                0x164c
#define mmDC_ABM1_LS_MIN_MAX_LUMA                                               0x164d
#define mmDC_ABM1_LS_FILTERED_MIN_MAX_LUMA                                      0x164e
#define mmDC_ABM1_LS_PIXEL_COUNT                                                0x164f
#define mmDC_ABM1_LS_OVR_SCAN_BIN                                               0x1650
#define mmDC_ABM1_LS_MIN_MAX_PIXEL_VALUE_THRES                                  0x1651
#define mmDC_ABM1_LS_MIN_PIXEL_VALUE_COUNT                                      0x1652
#define mmDC_ABM1_LS_MAX_PIXEL_VALUE_COUNT                                      0x1653
#define mmDC_ABM1_HG_SAMPLE_RATE                                                0x1654
#define mmDC_ABM1_LS_SAMPLE_RATE                                                0x1655
#define mmDC_ABM1_HG_BIN_1_32_SHIFT_FLAG                                        0x1656
#define mmDC_ABM1_HG_BIN_1_8_SHIFT_INDEX                                        0x1657
#define mmDC_ABM1_HG_BIN_9_16_SHIFT_INDEX                                       0x1658
#define mmDC_ABM1_HG_BIN_17_24_SHIFT_INDEX                                      0x1659
#define mmDC_ABM1_HG_BIN_25_32_SHIFT_INDEX                                      0x165a
#define mmDC_ABM1_HG_RESULT_1                                                   0x165b
#define mmDC_ABM1_HG_RESULT_2                                                   0x165c
#define mmDC_ABM1_HG_RESULT_3                                                   0x165d
#define mmDC_ABM1_HG_RESULT_4                                                   0x165e
#define mmDC_ABM1_HG_RESULT_5                                                   0x165f
#define mmDC_ABM1_HG_RESULT_6                                                   0x1660
#define mmDC_ABM1_HG_RESULT_7                                                   0x1661
#define mmDC_ABM1_HG_RESULT_8                                                   0x1662
#define mmDC_ABM1_HG_RESULT_9                                                   0x1663
#define mmDC_ABM1_HG_RESULT_10                                                  0x1664
#define mmDC_ABM1_HG_RESULT_11                                                  0x1665
#define mmDC_ABM1_HG_RESULT_12                                                  0x1666
#define mmDC_ABM1_HG_RESULT_13                                                  0x1667
#define mmDC_ABM1_HG_RESULT_14                                                  0x1668
#define mmDC_ABM1_HG_RESULT_15                                                  0x1669
#define mmDC_ABM1_HG_RESULT_16                                                  0x166a
#define mmDC_ABM1_HG_RESULT_17                                                  0x166b
#define mmDC_ABM1_HG_RESULT_18                                                  0x166c
#define mmDC_ABM1_HG_RESULT_19                                                  0x166d
#define mmDC_ABM1_HG_RESULT_20                                                  0x166e
#define mmDC_ABM1_HG_RESULT_21                                                  0x166f
#define mmDC_ABM1_HG_RESULT_22                                                  0x1670
#define mmDC_ABM1_HG_RESULT_23                                                  0x1671
#define mmDC_ABM1_HG_RESULT_24                                                  0x1672
#define mmDC_ABM1_OVERSCAN_PIXEL_VALUE                                          0x169b
#define mmDC_ABM1_BL_MASTER_LOCK                                                0x169c
#define mmABM_TEST_DEBUG_INDEX                                                  0x169e
#define mmABM_TEST_DEBUG_DATA                                                   0x169f
#define mmCRTC_H_BLANK_EARLY_NUM                                                0x1b7d
#define mmCRTC0_CRTC_H_BLANK_EARLY_NUM                                          0x1b7d
#define mmCRTC1_CRTC_H_BLANK_EARLY_NUM                                          0x1d7d
#define mmCRTC2_CRTC_H_BLANK_EARLY_NUM                                          0x1f7d
#define mmCRTC3_CRTC_H_BLANK_EARLY_NUM                                          0x417d
#define mmCRTC4_CRTC_H_BLANK_EARLY_NUM                                          0x437d
#define mmCRTC5_CRTC_H_BLANK_EARLY_NUM                                          0x457d
#define mmCRTC_H_TOTAL                                                          0x1b80
#define mmCRTC0_CRTC_H_TOTAL                                                    0x1b80
#define mmCRTC1_CRTC_H_TOTAL                                                    0x1d80
#define mmCRTC2_CRTC_H_TOTAL                                                    0x1f80
#define mmCRTC3_CRTC_H_TOTAL                                                    0x4180
#define mmCRTC4_CRTC_H_TOTAL                                                    0x4380
#define mmCRTC5_CRTC_H_TOTAL                                                    0x4580
#define mmCRTC_H_BLANK_START_END                                                0x1b81
#define mmCRTC0_CRTC_H_BLANK_START_END                                          0x1b81
#define mmCRTC1_CRTC_H_BLANK_START_END                                          0x1d81
#define mmCRTC2_CRTC_H_BLANK_START_END                                          0x1f81
#define mmCRTC3_CRTC_H_BLANK_START_END                                          0x4181
#define mmCRTC4_CRTC_H_BLANK_START_END                                          0x4381
#define mmCRTC5_CRTC_H_BLANK_START_END                                          0x4581
#define mmCRTC_H_SYNC_A                                                         0x1b82
#define mmCRTC0_CRTC_H_SYNC_A                                                   0x1b82
#define mmCRTC1_CRTC_H_SYNC_A                                                   0x1d82
#define mmCRTC2_CRTC_H_SYNC_A                                                   0x1f82
#define mmCRTC3_CRTC_H_SYNC_A                                                   0x4182
#define mmCRTC4_CRTC_H_SYNC_A                                                   0x4382
#define mmCRTC5_CRTC_H_SYNC_A                                                   0x4582
#define mmCRTC_H_SYNC_A_CNTL                                                    0x1b83
#define mmCRTC0_CRTC_H_SYNC_A_CNTL                                              0x1b83
#define mmCRTC1_CRTC_H_SYNC_A_CNTL                                              0x1d83
#define mmCRTC2_CRTC_H_SYNC_A_CNTL                                              0x1f83
#define mmCRTC3_CRTC_H_SYNC_A_CNTL                                              0x4183
#define mmCRTC4_CRTC_H_SYNC_A_CNTL                                              0x4383
#define mmCRTC5_CRTC_H_SYNC_A_CNTL                                              0x4583
#define mmCRTC_H_SYNC_B                                                         0x1b84
#define mmCRTC0_CRTC_H_SYNC_B                                                   0x1b84
#define mmCRTC1_CRTC_H_SYNC_B                                                   0x1d84
#define mmCRTC2_CRTC_H_SYNC_B                                                   0x1f84
#define mmCRTC3_CRTC_H_SYNC_B                                                   0x4184
#define mmCRTC4_CRTC_H_SYNC_B                                                   0x4384
#define mmCRTC5_CRTC_H_SYNC_B                                                   0x4584
#define mmCRTC_H_SYNC_B_CNTL                                                    0x1b85
#define mmCRTC0_CRTC_H_SYNC_B_CNTL                                              0x1b85
#define mmCRTC1_CRTC_H_SYNC_B_CNTL                                              0x1d85
#define mmCRTC2_CRTC_H_SYNC_B_CNTL                                              0x1f85
#define mmCRTC3_CRTC_H_SYNC_B_CNTL                                              0x4185
#define mmCRTC4_CRTC_H_SYNC_B_CNTL                                              0x4385
#define mmCRTC5_CRTC_H_SYNC_B_CNTL                                              0x4585
#define mmCRTC_VBI_END                                                          0x1b86
#define mmCRTC0_CRTC_VBI_END                                                    0x1b86
#define mmCRTC1_CRTC_VBI_END                                                    0x1d86
#define mmCRTC2_CRTC_VBI_END                                                    0x1f86
#define mmCRTC3_CRTC_VBI_END                                                    0x4186
#define mmCRTC4_CRTC_VBI_END                                                    0x4386
#define mmCRTC5_CRTC_VBI_END                                                    0x4586
#define mmCRTC_V_TOTAL                                                          0x1b87
#define mmCRTC0_CRTC_V_TOTAL                                                    0x1b87
#define mmCRTC1_CRTC_V_TOTAL                                                    0x1d87
#define mmCRTC2_CRTC_V_TOTAL                                                    0x1f87
#define mmCRTC3_CRTC_V_TOTAL                                                    0x4187
#define mmCRTC4_CRTC_V_TOTAL                                                    0x4387
#define mmCRTC5_CRTC_V_TOTAL                                                    0x4587
#define mmCRTC_V_TOTAL_MIN                                                      0x1b88
#define mmCRTC0_CRTC_V_TOTAL_MIN                                                0x1b88
#define mmCRTC1_CRTC_V_TOTAL_MIN                                                0x1d88
#define mmCRTC2_CRTC_V_TOTAL_MIN                                                0x1f88
#define mmCRTC3_CRTC_V_TOTAL_MIN                                                0x4188
#define mmCRTC4_CRTC_V_TOTAL_MIN                                                0x4388
#define mmCRTC5_CRTC_V_TOTAL_MIN                                                0x4588
#define mmCRTC_V_TOTAL_MAX                                                      0x1b89
#define mmCRTC0_CRTC_V_TOTAL_MAX                                                0x1b89
#define mmCRTC1_CRTC_V_TOTAL_MAX