R_PIPE2_XDMA_MSTR_PERFMEAS_CNTL                               0x42f
#define mmXDMA_MSTR_PIPE3_XDMA_MSTR_PERFMEAS_CNTL                               0x43f
#define mmXDMA_MSTR_PIPE4_XDMA_MSTR_PERFMEAS_CNTL                               0x44f
#define mmXDMA_MSTR_PIPE5_XDMA_MSTR_PERFMEAS_CNTL                               0x45f
#define mmXDMA_SLV_CNTL                                                         0x460
#define mmXDMA_SLV_MEM_CLIENT_CONFIG                                            0x461
#define mmXDMA_SLV_SLS_PITCH                                                    0x462
#define mmXDMA_SLV_READ_URGENT_CNTL                                             0x463
#define mmXDMA_SLV_WRITE_URGENT_CNTL                                            0x464
#define mmXDMA_SLV_WB_RATE_CNTL                                                 0x465
#define mmXDMA_SLV_READ_LATENCY_MINMAX                                          0x466
#define mmXDMA_SLV_READ_LATENCY_AVE                                             0x467
#define mmXDMA_SLV_PCIE_NACK_STATUS                                             0x468
#define mmXDMA_SLV_MEM_NACK_STATUS                                              0x469
#define mmXDMA_SLV_RDRET_BUF_STATUS                                             0x46a
#define mmXDMA_SLV_READ_LATENCY_TIMER                                           0x46b
#define mmXDMA_SLV_FLIP_PENDING                                                 0x46c
#define mmXDMA_SLV_CHANNEL_CNTL                                                 0x470
#define mmXDMA_SLV_CHANNEL0_XDMA_SLV_CHANNEL_CNTL                               0x470
#define mmXDMA_SLV_CHANNEL1_XDMA_SLV_CHANNEL_CNTL                               0x478
#define mmXDMA_SLV_CHANNEL2_XDMA_SLV_CHANNEL_CNTL                               0x480
#define mmXDMA_SLV_CHANNEL3_XDMA_SLV_CHANNEL_CNTL                               0x488
#define mmXDMA_SLV_CHANNEL4_XDMA_SLV_CHANNEL_CNTL                               0x490
#define mmXDMA_SLV_CHANNEL5_XDMA_SLV_CHANNEL_CNTL                               0x498
#define mmXDMA_SLV_REMOTE_GPU_ADDRESS                                           0x471
#define mmXDMA_SLV_CHANNEL0_XDMA_SLV_REMOTE_GPU_ADDRESS                         0x471
#define mmXDMA_SLV_CHANNEL1_XDMA_SLV_REMOTE_GPU_ADDRESS                         0x479
#define mmXDMA_SLV_CHANNEL2_XDMA_SLV_REMOTE_GPU_ADDRESS                         0x481
#define mmXDMA_SLV_CHANNEL3_XDMA_SLV_REMOTE_GPU_ADDRESS                         0x489
#define mmXDMA_SLV_CHANNEL4_XDMA_SLV_REMOTE_GPU_ADDRESS                         0x491
#define mmXDMA_SLV_CHANNEL5_XDMA_SLV_REMOTE_GPU_ADDRESS                         0x499
#define mmXDMA_SLV_REMOTE_GPU_ADDRESS_HIGH                                      0x472
#define mmXDMA_SLV_CHANNEL0_XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH                    0x472
#define mmXDMA_SLV_CHANNEL1_XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH                    0x47a
#define mmXDMA_SLV_CHANNEL2_XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH                    0x482
#define mmXDMA_SLV_CHANNEL3_XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH                    0x48a
#define mmXDMA_SLV_CHANNEL4_XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH                    0x492
#define mmXDMA_SLV_CHANNEL5_XDMA_SLV_REMOTE_GPU_ADDRESS_HIGH                    0x49a

#endif /* DCE_11_0_D_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
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

#ifndef DCE_11_0_ENUM_H
#define DCE_11_0_ENUM_H

typedef enum CRTC_CONTROL_CRTC_START_POINT_CNTL {
	CRTC_CONTROL_CRTC_START_POINT_CNTL_NORMAL        = 0x0,
	CRTC_CONTROL_CRTC_START_POINT_CNTL_DP            = 0x1,
} CRTC_CONTROL_CRTC_START_POINT_CNTL;
typedef enum CRTC_CONTROL_CRTC_FIELD_NUMBER_CNTL {
	CRTC_CONTROL_CRTC_FIELD_NUMBER_CNTL_NORMAL       = 0x0,
	CRTC_CONTROL_CRTC_FIELD_NUMBER_CNTL_DP           = 0x1,
} CRTC_CONTROL_CRTC_FIELD_NUMBER_CNTL;
typedef enum CRTC_CONTROL_CRTC_DISABLE_POINT_CNTL {
	CRTC_CONTROL_CRTC_DISABLE_POINT_CNTL_DISABLE     = 0x0,
	CRTC_CONTROL_CRTC_DISABLE_POINT_CNTL_DISABLE_CURRENT= 0x1,
	CRTC_CONTROL_CRTC_DISABLE_POINT_CNTL_RESERVED    = 0x2,
	CRTC_CONTROL_CRTC_DISABLE_POINT_CNTL_DISABLE_FIRST= 0x3,
} CRTC_CONTROL_CRTC_DISABLE_POINT_CNTL;
typedef enum CRTC_CONTROL_CRTC_FIELD_NUMBER_POLARITY {
	CRTC_CONTROL_CRTC_FIELD_NUMBER_POLARITY_FALSE    = 0x0,
	CRTC_CONTROL_CRTC_FIELD_NUMBER_POLARITY_TRUE     = 0x1,
} CRTC_CONTROL_CRTC_FIELD_NUMBER_POLARITY;
typedef enum CRTC_CONTROL_CRTC_DISP_READ_REQUEST_DISABLE {
	CRTC_CONTROL_CRTC_DISP_READ_REQUEST_DISABLE_FALSE= 0x0,
	CRTC_CONTROL_CRTC_DISP_READ_REQUEST_DISABLE_TRUE = 0x1,
} CRTC_CONTROL_CRTC_DISP_READ_REQUEST_DISABLE;
typedef enum CRTC_CONTROL_CRTC_SOF_PULL_EN {
	CRTC_CONTROL_CRTC_SOF_PULL_EN_FALSE              = 0x0,
	CRTC_CONTROL_CRTC_SOF_PULL_EN_TRUE               = 0x1,
} CRTC_CONTROL_CRTC_SOF_PULL_EN;
typedef enum CRTC_H_SYNC_B_CNTL_CRTC_H_SYNC_B_POL {
	CRTC_H_SYNC_B_CNTL_CRTC_H_SYNC_B_POL_FALSE       = 0x0,
	CRTC_H_SYNC_B_CNTL_CRTC_H_SYNC_B_POL_TRUE        = 0x1,
} CRTC_H_SYNC_B_CNTL_CRTC_H_SYNC_B_POL;
typedef enum CRTC_V_TOTAL_CONTROL_CRTC_V_TOTAL_MAX_SEL {
	CRTC_V_TOTAL_CONTROL_CRTC_V_TOTAL_MAX_SEL_FALSE  = 0x0,
	CRTC_V_TOTAL_CONTROL_CRTC_V_TOTAL_MAX_SEL_TRUE   = 0x1,
} CRTC_V_TOTAL_CONTROL_CRTC_V_TOTAL_MAX_SEL;
typedef enum CRTC_V_TOTAL_CONTROL_CRTC_V_TOTAL_MIN_SEL {
	CRTC_V_TOTAL_CONTROL_CRTC_V_TOTAL_MIN_SEL_FALSE  = 0x0,
	CRTC_V_TOTAL_CONTROL_CRTC_V_TOTAL_MIN_SEL_TRUE   = 0x1,
} CRTC_V_TOTAL_CONTROL_CRTC_V_TOTAL_MIN_SEL;
typedef enum CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_EN {
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_EN_FALSE= 0x0,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_EN_TRUE= 0x1,
} CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_EN;
typedef enum CRTC_V_TOTAL_CONTROL_CRTC_FORCE_LOCK_TO_MASTER_VSYNC {
	CRTC_V_TOTAL_CONTROL_CRTC_FORCE_LOCK_TO_MASTER_VSYNC_DISABLE= 0x0,
	CRTC_V_TOTAL_CONTROL_CRTC_FORCE_LOCK_TO_MASTER_VSYNC_ENABLE= 0x1,
} CRTC_V_TOTAL_CONTROL_CRTC_FORCE_LOCK_TO_MASTER_VSYNC;
typedef enum CRTC_V_TOTAL_CONTROL_CRTC_FORCE_LOCK_ON_EVENT {
	CRTC_V_TOTAL_CONTROL_CRTC_FORCE_LOCK_ON_EVENT_DISABLE= 0x0,
	CRTC_V_TOTAL_CONTROL_CRTC_FORCE_LOCK_ON_EVENT_ENABLE= 0x1,
} CRTC_V_TOTAL_CONTROL_CRTC_FORCE_LOCK_ON_EVENT;
typedef enum CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK {
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_FRAME_START= 0x0,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_CRTC_TRIG_A= 0x1,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_CRTC_TRIG_B= 0x2,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_CURSOR_CHANGE= 0x3,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_OTHER_CLIENT= 0x4,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_MC_DC_REGION0= 0x5,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_MC_DC_REGION1= 0x6,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_MC_DC_REGION2= 0x7,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_MC_DC_REGION3= 0x8,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_GRAPHIC_UPDATE_PENDING= 0x9,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_RESERVED2= 0xa,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_INVALID= 0xb,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_DOUBLE_BUFFER= 0xc,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_D1CRTC_VERT_COUNT_NOM= 0xd,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_D1CRTC_VERT_COUNT= 0xe,
	CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK_RESERVED= 0xf,
} CRTC_V_TOTAL_CONTROL_CRTC_SET_V_TOTAL_MIN_MASK;
typedef enum CRTC_V_TOTAL_INT_STATUS_CRTC_SET_V_TOTAL_MIN_EVENT_OCCURED_ACK {
	CRTC_V_TOTAL_INT_STATUS_CRTC_SET_V_TOTAL_MIN_EVENT_OCCURED_ACK_FALSE= 0x0,
	CRTC_V_TOTAL_INT_STATUS_CRTC_SET_V_TOTAL_MIN_EVENT_OCCURED_ACK_TRUE= 0x1,
} CRTC_V_TOTAL_INT_STATUS_CRTC_SET_V_TOTAL_MIN_EVENT_OCCURED_ACK;
typedef enum CRTC_VSYNC_NOM_INT_STATUS_CRTC_VSYNC_NOM_INT_CLEAR {
	CRTC_VSYNC_NOM_INT_STATUS_CRTC_VSYNC_NOM_INT_CLEAR_FALSE= 0x0,
	CRTC_VSYNC_NOM_INT_STATUS_CRTC_VSYNC_NOM_INT_CLEAR_TRUE= 0x1,
} CRTC_VSYNC_NOM_INT_STATUS_CRTC_VSYNC_NOM_INT_CLEAR;
typedef enum CRTC_V_SYNC_B_CNTL_CRTC_V_SYNC_B_POL {
	CRTC_V_SYNC_B_CNTL_CRTC_V_SYNC_B_POL_FALSE       = 0x0,
	CRTC_V_SYNC_B_CNTL_CRTC_V_SYNC_B_POL_TRUE        = 0x1,
} CRTC_V_SYNC_B_CNTL_CRTC_V_SYNC_B_POL;
typedef enum CRTC_DTMTEST_CNTL_CRTC_DTMTEST_CRTC_EN {
	CRTC_DTMTEST_CNTL_CRTC_DTMTEST_CRTC_EN_FALSE     = 0x0,
	CRTC_DTMTEST_CNTL_CRTC_DTMTEST_CRTC_EN_TRUE      = 0x1,
} CRTC_DTMTEST_CNTL_CRTC_DTMTEST_CRTC_EN;
typedef enum CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT {
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_VSYNCA_OTHER= 0x1,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_HSYNCA_OTHER= 0x2,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_GENERICF= 0x5,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_GENERICE= 0x6,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_VSYNCA  = 0x7,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_HSYNCA  = 0x8,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_VSYNCB  = 0x9,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_HSYNCB  = 0xa,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_HPD1    = 0xb,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_HPD2    = 0xc,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_GENERICD= 0xd,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_GENERICC= 0xe,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_VIDEO   = 0xf,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_IGSL0   = 0x10,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_IGSL1   = 0x11,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_IGSL2   = 0x12,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_IBLON   = 0x13,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_GENERICA= 0x14,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_GENERICB= 0x15,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_IGSL_ALLOW= 0x16,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT_MANUAL_FLOW= 0x17,
} CRTC_TRIGA_CNTL_CRTC_TRIGA_SOURCE_SELECT;
typedef enum CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT {
	CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT_INTERLACE= 0x1,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT_GENERICA= 0x2,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT_GENERICB= 0x3,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT_HSYNCA= 0x4,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT_HSYNCB= 0x5,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT_VIDEO = 0x6,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT_GENERICC= 0x7,
} CRTC_TRIGA_CNTL_CRTC_TRIGA_POLARITY_SELECT;
typedef enum CRTC_TRIGA_CNTL_CRTC_TRIGA_RESYNC_BYPASS_EN {
	CRTC_TRIGA_CNTL_CRTC_TRIGA_RESYNC_BYPASS_EN_FALSE= 0x0,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_RESYNC_BYPASS_EN_TRUE = 0x1,
} CRTC_TRIGA_CNTL_CRTC_TRIGA_RESYNC_BYPASS_EN;
typedef enum CRTC_TRIGA_CNTL_CRTC_TRIGA_CLEAR {
	CRTC_TRIGA_CNTL_CRTC_TRIGA_CLEAR_FALSE           = 0x0,
	CRTC_TRIGA_CNTL_CRTC_TRIGA_CLEAR_TRUE            = 0x1,
} CRTC_TRIGA_CNTL_CRTC_TRIGA_CLEAR;
typedef enum CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT {
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_VSYNCA_OTHER= 0x1,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_HSYNCA_OTHER= 0x2,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_GENERICF= 0x5,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_GENERICE= 0x6,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_VSYNCA  = 0x7,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_HSYNCA  = 0x8,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_VSYNCB  = 0x9,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_HSYNCB  = 0xa,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_HPD1    = 0xb,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_HPD2    = 0xc,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_GENERICD= 0xd,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_GENERICC= 0xe,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_VIDEO   = 0xf,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_IGSL0   = 0x10,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_IGSL1   = 0x11,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_IGSL2   = 0x12,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_IBLON   = 0x13,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_GENERICA= 0x14,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_GENERICB= 0x15,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_IGSL_ALLOW= 0x16,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT_MANUAL_FLOW= 0x17,
} CRTC_TRIGB_CNTL_CRTC_TRIGB_SOURCE_SELECT;
typedef enum CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT {
	CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT_INTERLACE= 0x1,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT_GENERICA= 0x2,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT_GENERICB= 0x3,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT_HSYNCA= 0x4,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT_HSYNCB= 0x5,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT_VIDEO = 0x6,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT_GENERICC= 0x7,
} CRTC_TRIGB_CNTL_CRTC_TRIGB_POLARITY_SELECT;
typedef enum CRTC_TRIGB_CNTL_CRTC_TRIGB_RESYNC_BYPASS_EN {
	CRTC_TRIGB_CNTL_CRTC_TRIGB_RESYNC_BYPASS_EN_FALSE= 0x0,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_RESYNC_BYPASS_EN_TRUE = 0x1,
} CRTC_TRIGB_CNTL_CRTC_TRIGB_RESYNC_BYPASS_EN;
typedef enum CRTC_TRIGB_CNTL_CRTC_TRIGB_CLEAR {
	CRTC_TRIGB_CNTL_CRTC_TRIGB_CLEAR_FALSE           = 0x0,
	CRTC_TRIGB_CNTL_CRTC_TRIGB_CLEAR_TRUE            = 0x1,
} CRTC_TRIGB_CNTL_CRTC_TRIGB_CLEAR;
typedef enum CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_MODE {
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_MODE_DISABLE= 0x0,
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_MODE_HCOUNT= 0x1,
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_MODE_HCOUNT_VCOUNT= 0x2,
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_MODE_RESERVED= 0x3,
} CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_MODE;
typedef enum CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_CHECK {
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_CHECK_FALSE= 0x0,
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_CHECK_TRUE= 0x1,
} CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_CHECK;
typedef enum CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_TRIG_SEL {
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_TRIG_SEL_FALSE= 0x0,
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_TRIG_SEL_TRUE= 0x1,
} CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_TRIG_SEL;
typedef enum CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_CLEAR {
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_CLEAR_FALSE= 0x0,
	CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_CLEAR_TRUE= 0x1,
} CRTC_FORCE_COUNT_NOW_CNTL_CRTC_FORCE_COUNT_NOW_CLEAR;
typedef enum CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT {
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_LOGIC0= 0x0,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_GENERICF= 0x1,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_GENERICE= 0x2,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_HPD1= 0x3,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_HPD2= 0x4,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_DDC1DATA= 0x5,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_DDC1CLK= 0x6,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_DDC2DATA= 0x7,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_DDC2CLK= 0x8,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_DVOCLK= 0x9,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_MANUAL= 0xa,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_LOGIC1= 0xb,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_GENERICB= 0xc,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_GENERICA= 0xd,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_GENERICD= 0xe,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_GENERICC= 0xf,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT_GPIO= 0x10,
} CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_SOURCE_SELECT;
typedef enum CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_POLARITY {
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_POLARITY_FALSE= 0x0,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_POLARITY_TRUE= 0x1,
} CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_POLARITY;
typedef enum CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_GRANULARITY {
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_GRANULARITY_FALSE= 0x0,
	CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_GRANULARITY_TRUE= 0x1,
} CRTC_FLOW_CONTROL_CRTC_FLOW_CONTROL_GRANULARITY;
typedef enum CRTC_STEREO_FORCE_NEXT_EYE_CRTC_STEREO_FORCE_NEXT_EYE {
	CRTC_STEREO_FORCE_NEXT_EYE_CRTC_STEREO_FORCE_NEXT_EYE_NO= 0x0,
	CRTC_STEREO_FORCE_NEXT_EYE_CRTC_STEREO_FORCE_NEXT_EYE_RIGHT= 0x1,
	CRTC_STEREO_FORCE_NEXT_EYE_CRTC_STEREO_FORCE_NEXT_EYE_LEFT= 0x2,
	CRTC_STEREO_FORCE_NEXT_EYE_CRTC_STEREO_FORCE_NEXT_EYE_RESERVED= 0x3,
} CRTC_STEREO_FORCE_NEXT_EYE_CRTC_STEREO_FORCE_NEXT_EYE;
typedef enum CRTC_CONTROL_CRTC_MASTER_EN {
	CRTC_CONTROL_CRTC_MASTER_EN_FALSE                = 0x0,
	CRTC_CONTROL_CRTC_MASTER_EN_TRUE                 = 0x1,
} CRTC_CONTROL_CRTC_MASTER_EN;
typedef enum CRTC_BLANK_CONTROL_CRTC_BLANK_DATA_EN {
	CRTC_BLANK_CONTROL_CRTC_BLANK_DATA_EN_FALSE      = 0x0,
	CRTC_BLANK_CONTROL_CRTC_BLANK_DATA_EN_TRUE       = 0x1,
} CRTC_BLANK_CONTROL_CRTC_BLANK_DATA_EN;
typedef enum CRTC_BLANK_CONTROL_CRTC_BLANK_DE_MODE {
	CRTC_BLANK_CONTROL_CRTC_BLANK_DE_MODE_FALSE      = 0x0,
	CRTC_BLANK_CONTROL_CRTC_BLANK_DE_MODE_TRUE       = 0x1,
} CRTC_BLANK_CONTROL_CRTC_BLANK_DE_MODE;
typedef enum CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_ENABLE {
	CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_ENABLE_FALSE= 0x0,
	CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_ENABLE_TRUE= 0x1,
} CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_ENABLE;
typedef enum CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_FORCE_NEXT_FIELD {
	CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_FORCE_NEXT_FIELD_NOT= 0x0,
	CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_FORCE_NEXT_FIELD_ODD= 0x1,
	CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_FORCE_NEXT_FIELD_EVEN= 0x2,
	CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_FORCE_NEXT_FIELD_NOT2= 0x3,
} CRTC_INTERLACE_CONTROL_CRTC_INTERLACE_FORCE_NEXT_FIELD;
typedef enum CRTC_FIELD_INDICATION_CONTROL_CRTC_FIELD_INDICATION_OUTPUT_POLARITY {
	CRTC_FIELD_INDICATION_CONTROL_CRTC_FIELD_INDICATION_OUTPUT_POLARITY_FALSE= 0x0,
	CRTC_FIELD_INDICATION_CONTROL_CRTC_FIELD_INDICATION_OUTPUT_POLARITY_TRUE= 0x1,
} CRTC_FIELD_INDICATION_CONTROL_CRTC_FIELD_INDICATION_OUTPUT_POLARITY;
typedef enum CRTC_FIELD_INDICATION_CONTROL_CRTC_FIELD_ALIGNMENT {
	CRTC_FIELD_INDICATION_CONTROL_CRTC_FIELD_ALIGNMENT_FALSE= 0x0,
	CRTC_FIELD_INDICATION_CONTROL_CRTC_FIELD_ALIGNMENT_TR