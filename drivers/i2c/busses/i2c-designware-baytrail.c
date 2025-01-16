/*
 * GMC_8_2 Register documentation
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

#ifndef GMC_8_2_SH_MASK_H
#define GMC_8_2_SH_MASK_H

#define MC_CONFIG__MCDW_WR_ENABLE_MASK 0x1
#define MC_CONFIG__MCDW_WR_ENABLE__SHIFT 0x0
#define MC_CONFIG__MCDX_WR_ENABLE_MASK 0x2
#define MC_CONFIG__MCDX_WR_ENABLE__SHIFT 0x1
#define MC_CONFIG__MCDY_WR_ENABLE_MASK 0x4
#define MC_CONFIG__MCDY_WR_ENABLE__SHIFT 0x2
#define MC_CONFIG__MCDZ_WR_ENABLE_MASK 0x8
#define MC_CONFIG__MCDZ_WR_ENABLE__SHIFT 0x3
#define MC_CONFIG__MCDS_WR_ENABLE_MASK 0x10
#define MC_CONFIG__MCDS_WR_ENABLE__SHIFT 0x4
#define MC_CONFIG__MCDT_WR_ENABLE_MASK 0x20
#define MC_CONFIG__MCDT_WR_ENABLE__SHIFT 0x5
#define MC_CONFIG__MCDU_WR_ENABLE_MASK 0x40
#define MC_CONFIG__MCDU_WR_ENABLE__SHIFT 0x6
#define MC_CONFIG__MCDV_WR_ENABLE_MASK 0x80
#define MC_CONFIG__MCDV_WR_ENABLE__SHIFT 0x7
#define MC_CONFIG__MC_RD_ENABLE_MASK 0x700
#define MC_CONFIG__MC_RD_ENABLE__SHIFT 0x8
#define MC_CONFIG__MCC_INDEX_MODE_ENABLE_MASK 0x80000000
#define MC_CONFIG__MCC_INDEX_MODE_ENABLE__SHIFT 0x1f
#define MC_ARB_ATOMIC__TC_GRP_MASK 0x7
#define MC_ARB_ATOMIC__TC_GRP__SHIFT 0x0
#define MC_ARB_ATOMIC__TC_GRP_EN_MASK 0x8
#define MC_ARB_ATOMIC__TC_GRP_EN__SHIFT 0x3
#define MC_ARB_ATOMIC__SDMA_GRP_MASK 0x70
#define MC_ARB_ATOMIC__SDMA_GRP__SHIFT 0x4
#define MC_ARB_ATOMIC__SDMA_GRP_EN_MASK 0x80
#define MC_ARB_ATOMIC__SDMA_GRP_EN__SHIFT 0x7
#define MC_ARB_ATOMIC__OUTSTANDING_MASK 0xff00
#define MC_ARB_ATOMIC__OUTSTANDING__SHIFT 0x8
#define MC_ARB_ATOMIC__ATOMIC_RTN_GRP_MASK 0xff0000
#define MC_ARB_ATOMIC__ATOMIC_RTN_GRP__SHIFT 0x10
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP0_MASK 0x1
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP0__SHIFT 0x0
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP1_MASK 0x2
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP1__SHIFT 0x1
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP2_MASK 0x4
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP2__SHIFT 0x2
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP3_MASK 0x8
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP3__SHIFT 0x3
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP4_MASK 0x10
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP4__SHIFT 0x4
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP5_MASK 0x20
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP5__SHIFT 0x5
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP6_MASK 0x40
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP6__SHIFT 0x6
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP7_MASK 0x80
#define MC_ARB_AGE_CNTL__RESET_RD_GROUP7__SHIFT 0x7
#define MC_ARB_AGE_CNTL__RESET_WR_GROUP0_MASK 0x100
#define MC_ARB_AGE_CNTL__RESET_WR_GROUP0__SHIFT 0x8
#define MC_ARB_AGE_CNTL__RESET_WR_GROUP1_MASK 0x200
#define MC_ARB_AGE_CNTL__RESET_WR_GROUP1__SHIFT 0x9
#define MC_ARB_AGE_CNTL__RESET_WR_GROUP2_MASK 0x400
#define MC_ARB_AGE_CNTL__RESET_WR_GROUP2__SHIFT 0xa
#define MC_ARB_AGE_CNTL__RESET_WR_GROUP3_MASK 0x800
#define MC_ARB_AGE_CNTL