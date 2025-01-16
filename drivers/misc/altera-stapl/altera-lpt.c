/*
 * BIF_4_1 Register documentation
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

#ifndef BIF_4_1_SH_MASK_H
#define BIF_4_1_SH_MASK_H

#define MM_INDEX__MM_OFFSET_MASK 0x7fffffff
#define MM_INDEX__MM_OFFSET__SHIFT 0x0
#define MM_INDEX__MM_APER_MASK 0x80000000
#define MM_INDEX__MM_APER__SHIFT 0x1f
#define MM_INDEX_HI__MM_OFFSET_HI_MASK 0xffffffff
#define MM_INDEX_HI__MM_OFFSET_HI__SHIFT 0x0
#define MM_DATA__MM_DATA_MASK 0xffffffff
#define MM_DATA__MM_DATA__SHIFT 0x0
#define BUS_CNTL__BIOS_ROM_WRT_EN_MASK 0x1
#define BUS_CNTL__BIOS_ROM_WRT_EN__SHIFT 0x0
#define BUS_CNTL__BIOS_ROM_DIS_MASK 0x2
#define BUS_CNTL__BIOS_ROM_DIS__SHIFT 0x1
#define BUS_CNTL__PMI_IO_DIS