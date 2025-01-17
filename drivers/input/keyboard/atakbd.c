_MASK 0x1
#define DIDT_DBR_CTRL0__DIDT_CTRL_EN__SHIFT 0x0
#define DIDT_DBR_CTRL0__USE_REF_CLOCK_MASK 0x2
#define DIDT_DBR_CTRL0__USE_REF_CLOCK__SHIFT 0x1
#define DIDT_DBR_CTRL0__PHASE_OFFSET_MASK 0xc
#define DIDT_DBR_CTRL0__PHASE_OFFSET__SHIFT 0x2
#define DIDT_DBR_CTRL0__DIDT_CTRL_RST_MASK 0x10
#define DIDT_DBR_CTRL0__DIDT_CTRL_RST__SHIFT 0x4
#define DIDT_DBR_CTRL0__DIDT_CLK_EN_OVERRIDE_MASK 0x20
#define DIDT_DBR_CTRL0__DIDT_CLK_EN_OVERRIDE__SHIFT 0x5
#define DIDT_DBR_CTRL0__UNUSED_0_MASK 0xffffffc0
#define DIDT_DBR_CTRL0__UNUSED_0__SHIFT 0x6
#define DIDT_DBR_CTRL1__MIN_POWER_MASK 0xffff
#define DIDT_DBR_CTRL1__MIN_POWER__SHIFT 0x0
#define DIDT_DBR_CTRL1__MAX_POWER_MASK 0xffff0000
#define DIDT_DBR_CTRL1__MAX_POWER__SHIFT 0x10
#define DIDT_DBR_CTRL2__MAX_POWER_DELTA_MASK 0x3fff
#define DIDT_DBR_CTRL2__MAX_POWER_DELTA__SHIFT 0x0
#define DIDT_DBR_CTRL2__UNUSED_0_MASK 0xc000
#define DIDT_DBR_CTRL2__UNUSED_0__SHIFT 0xe
#define DIDT_DBR_CTRL2__SHORT_TERM_INTERVAL_SIZE_MASK 0x3ff0000
#define DIDT_DBR_CTRL2__SHORT_TERM_INTERVAL_SIZE__SHIFT 0x10
#define DIDT_DBR_CTRL2__UNUSED_1_MASK 0x4000000
#define DIDT_DBR_CTRL2__UNUSED_1__SHIFT 0x1a
#define DIDT_DBR_CTRL2__LONG_TERM_INTERVAL_RATIO_MASK 0x78000000
#define DIDT_DBR_CTRL2__LONG_TERM_INTERVAL_RATIO__SHIFT 0x1b
#define DIDT_DBR_CTRL2__UNUSED_2_MASK 0x80000000
#define DIDT_DBR_CTRL2__UNUSED_2__SHIFT 0x1f
#define DIDT_DBR_CTRL_OCP__UNUSED_0_MASK 0xffff
#define DIDT_DBR_CTRL_OCP__UNUSED_0__SHIFT 0x0
#define DIDT_DBR_CTRL_OCP__OCP_MAX_POWER_MASK 0xffff0000
#define DIDT_DBR_CTRL_OCP__OCP_MAX_POWER__SHIFT 0x10
#define DIDT_DBR_WEIGHT0_3__WEIGHT0_MASK 0xff
#define DIDT_DBR_WEIGHT0_3__WEIGHT0__SHIFT 0x0
#define DIDT_DBR_WEIGHT0_3__WEIGHT1_MASK 0xff00
#define DIDT_DBR_WEIGHT0_3__WEIGHT1__SHIFT 0x8
#define DIDT_DBR_WEIGHT0_3__WEIGHT2_MASK 0xff0000
#define DIDT_DBR_WEIGHT0_3__WEIGHT2__SHIFT 0x10
#define DIDT_DBR_WEIGHT0_3__WEIGHT3_MASK 0xff000000
#define DIDT_DBR_WEIGHT0_3__WEIGHT3__SHIFT 0x18
#define DIDT_DBR_WEIGHT4_7__WEIGHT4_MASK 0xff
#define DIDT_DBR_WEIGHT4_7__WEIGHT4__SHIFT 0x0
#define DIDT_DBR_WEIGHT4_7__WEIGHT5_MASK 0xff00
#define DIDT_DBR_WEIGHT4_7__WEIGHT5__SHIFT 0x8
#define DIDT_DBR_WEIGHT4_7__WEIGHT6_MASK 0xff0000
#define DIDT_DBR_WEIGHT4_7__WEIGHT6__SHIFT 0x10
#define DIDT_DBR_WEIGHT4_7__WEIGHT7_MASK 0xff000000
#define DIDT_DBR_WEIGHT4_7__WEIGHT7__SHIFT 0x18
#define DIDT_DBR_WEIGHT8_11__WEIGHT8_MASK 0xff
#define DIDT_DBR_WEIGHT8_11__WEIGHT8__SHIFT 0x0
#define DIDT_DBR_WEIGHT8_11__WEIGHT9_MASK 0xff00
#define DIDT_DBR_WEIGHT8_11__WEIGHT9__SHIFT 0x8
#define DIDT_DBR_WEIGHT8_11__WEIGHT10_MASK 0xff0000
#define DIDT_DBR_WEIGHT8_11__WEIGHT10__SHIFT 0x10
#define DIDT_DBR_WEIGHT8_11__WEIGHT11_MASK 0xff000000
#define DIDT_DBR_WEIGHT8_11__WEIGHT11__SHIFT 0x18

#endif /* GFX_8_1_SH_MASK_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * GMC_7_0 Register documentation
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

#ifndef GMC_7_0_SH_MASK_H
#define GMC_7_0_SH_MASK_H

#define MC_CONFIG__MCDW_WR_ENABLE_MASK 0x1
#define MC_CONFIG__MCDW_WR_ENABLE__SHIFT 0x0
#define MC_CONFIG__MCDX_WR_ENABLE_MASK 0x2
#define MC_CONFIG__MCDX_WR_ENABLE__SHIFT 0x1
#define MC_CONFIG__MCDY_WR_ENABLE_MASK 0x4
#define MC_CONFIG__MCDY_WR_ENABLE__SHIFT 0x2
#define MC_CONFIG__MCDZ_WR_ENABLE_MASK 0x8
#define MC_CONFIG__MCDZ_WR_ENABLE__SHIFT 0x3
#define MC_CONFIG__MC_RD_ENABLE_MASK 0x30
#define MC_CONFIG__MC_RD_ENABLE