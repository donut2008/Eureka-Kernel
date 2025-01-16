/*===---- xopintrin.h - XOP intrinsics -------------------------------------===
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *===-----------------------------------------------------------------------===
 */

#ifndef __X86INTRIN_H
#error "Never use <xopintrin.h> directly; include <x86intrin.h> instead."
#endif

#ifndef __XOPINTRIN_H
#define __XOPINTRIN_H

#include <fma4intrin.h>

/* Define the default attributes for the functions in this file. */
#define __DEFAULT_FN_ATTRS __attribute__((__always_inline__, __nodebug__, __target__("xop")))

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_maccs_epi16(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacssww((__v8hi)__A, (__v8hi)__B, (__v8hi)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_macc_epi16(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacsww((__v8hi)__A, (__v8hi)__B, (__v8hi)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_maccsd_epi16(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacsswd((__v8hi)__A, (__v8hi)__B, (__v4si)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_maccd_epi16(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacswd((__v8hi)__A, (__v8hi)__B, (__v4si)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_maccs_epi32(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacssdd((__v4si)__A, (__v4si)__B, (__v4si)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_macc_epi32(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacsdd((__v4si)__A, (__v4si)__B, (__v4si)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_maccslo_epi32(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacssdql((__v4si)__A, (__v4si)__B, (__v2di)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_macclo_epi32(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacsdql((__v4si)__A, (__v4si)__B, (__v2di)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_maccshi_epi32(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacssdqh((__v4si)__A, (__v4si)__B, (__v2di)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_macchi_epi32(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmacsdqh((__v4si)__A, (__v4si)__B, (__v2di)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_maddsd_epi16(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmadcsswd((__v8hi)__A, (__v8hi)__B, (__v4si)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_maddd_epi16(__m128i __A, __m128i __B, __m128i __C)
{
  return (__m128i)__builtin_ia32_vpmadcswd((__v8hi)__A, (__v8hi)__B, (__v4si)__C);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_haddw_epi8(__m128i __A)
{
  return (__m128i)__builtin_ia32_vphaddbw((__v16qi)__A);
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_haddd_epi8(__m128i __A)
{
  return (__m128i)__builtin_ia32_vphaddbd((_/* i915_irq.c -- IRQ support for the I915 -*- linux-c -*-
 */
/*
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/sysrq.h>
#include <linux/slab.h>
#include <linux/circ_buf.h>
#include <drm/drmP.h>
#include <drm/i915_drm.h>
#include "i915_drv.h"
#include "i915_trace.h"
#include "intel_drv.h"

/**
 * DOC: interrupt handling
 *
 * These functions provide the basic support for enabling and disabling the
 * interrupt handling support. There's a lot more functionality in i915_irq.c
 * and related files, but that will be described in separate chapters.
 */

static const u32 hpd_ilk[HPD_NUM_PINS] = {
	[HPD_PORT_A] = DE_DP_A_HOTPLUG,
};

static const u32 hpd_ivb[HPD_NUM_PINS] = {
	[HPD_PORT_A] = DE_DP_A_HOTPLUG_IVB,
};

static const u32 hpd_bdw[HPD_NUM_PINS] = {
	[HPD_PORT_A] = GEN8_PORT_DP_A_HOTPLUG,
};

static const u32 hpd_ibx[HPD_NUM_PINS] = {
	[HPD_CRT] = SDE_CRT_HOTPLUG,
	[HPD_SDVO_B] = SDE_SDVOB_HOTPLUG,
	[HPD_PORT_B] = SDE_PORTB_HOTPLUG,
	[HPD_PORT_C] = SDE_PORTC_HOTPLUG,
	[HPD_PORT_D] = SDE_PORTD_HOTPLUG
};

static const u32 hpd_cpt[HPD_NUM_PINS] = {
	[HPD_CRT] = SDE_CRT_HOTPLUG_CPT,
	[HPD_SDVO_B] = SDE_SDVOB_HOTPLUG_CPT,
	[HPD_PORT_B] = SDE_PORTB_HOTPLUG_CPT,
	[HPD_PORT_C] = SDE_PORTC_HOTPLUG_CPT,
	[HPD_PORT_D] = SDE_PORTD_HOTPLUG_CPT
};

static const u32 hpd_spt[HPD_NUM_PINS] = {
	[HPD_PORT_A] = SDE_PORTA_HOTPLUG_SPT,
	[HPD_PORT_B] = SDE_PORTB_HOTPLUG_CPT,
	[HPD_PORT_C] = SDE_PORTC_HOTPLUG_CPT,
	[HPD_PORT_D] = SDE_PORTD_HOTPLUG_CPT,
	[HPD_PORT_E] = SDE_PORTE_HOTPLUG_SPT
};

static const u32 hpd_mask_i915[HPD_NUM_PINS] = {
	[HPD_CRT] = CRT_HOTPLUG_INT_EN,
	[HPD_SDVO_B] = SDVOB_HOTPLUG_INT_EN,
	[HPD_SDVO_C] = SDVOC_HOTPLUG_INT_EN,
	[HPD_PORT_B] = PORTB_HOTPLUG_INT_EN,
	[HPD_PORT_C] = PORTC_HOTPLUG_INT_EN,
	[HPD_PORT_D] = PORTD_HOTPLUG_INT_EN
};

static const u32 hpd_status_g4x[HPD_NUM_PINS] = {
	[HPD_CRT] = CRT_HOTPLUG_INT_STATUS,
	[HPD_SDVO_B] = SDVOB_HOTPLUG_INT_STATUS_G4X,
	[HPD_SDVO_C] = SDVOC_HOTPLUG_INT_STATUS_G4X,
	[HPD_PORT_B] = PORTB_HOTPLUG_INT_STATUS,
	[HPD_PORT_C] = PORTC_HOTPLUG_INT_STATUS,
	[HPD_PORT_D] = PORTD_HOTPLUG_INT_STATUS
};

static const u32 hpd_status_i915[HPD_NUM_PINS] = {
	[HPD_CRT] = CRT_HOTPLUG_INT_STATUS,
	[HPD_SDVO_B] = SDVOB_HOTPLUG_INT_STATUS_I915,
	[HPD_SDVO_C] = SDVOC_HOTPLUG_INT_STATUS_I915,
	[HPD_PORT_B] = PORTB_HOTPLUG_INT_STATUS,
	[HPD_PORT_C] = PORTC_HOTPLUG_INT_STATUS,
	[HPD_PORT_D] = PORTD_HOTPLUG_INT_STATUS
};

/* BXT hpd list */
static const u32 hpd_bxt[HPD_NUM_PINS] = {
	[HPD_PORT_A] = BXT_DE_PORT_HP_DDIA,
	[HPD_PORT_B] = BXT_DE_PORT_HP_DDIB,
	[HPD_PORT_C] = BXT_DE_PORT_HP_DDIC
};

/* IIR can theoretically queue up two events. Be paranoid. */
#define GEN8_IRQ_RESET_NDX(typ