#ifndef _ASM_IA64_PROCESSOR_H
#define _ASM_IA64_PROCESSOR_H

/*
 * Copyright (C) 1998-2004 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 1999 Don Dugger <don.dugger@intel.com>
 *
 * 11/24/98	S.Eranian	added ia64_set_iva()
 * 12/03/99	D. Mosberger	implement thread_saved_pc() via kernel unwind API
 * 06/16/00	A. Mallick	added csd/ssd/tssd for ia32 support
 */


#include <asm/intrinsics.h>
#include <asm/kregs.h>
#include <asm/ptrace.h>
#include <asm/ustack.h>

#define ARCH_HAS_PREFETCH_SWITCH_STACK

#define IA64_NUM_PHYS_STACK_REG	96
#define IA64_NUM_DBG_REGS	8

#define DEFAULT_MAP_BASE	__IA64_UL_CONST(0x2000000000000000)
#define DEFAULT_TASK_SIZE	__IA64_UL_CONST(0xa000000000000000)

/*
 * TASK_SIZE really is a mis-named.  It really is the maximum user
 * space address (plus one).  On IA-64, there are five regions of 2TB
 * each (assuming 8KB page size), for a total of 8TB of user virtual
 * address space.
 */
#define TASK_SIZE       	DEFAULT_TASK_SIZE

/*
 * This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	(current->thread.map_base)

#define IA64_THREAD_FPH_VALID	(__IA64_UL(1) << 0)	/* floating-point high state valid? */
#define IA64_THREAD_DBG_VALID	(__IA64_UL(1) << 1)	/* debug registers valid? */
#define IA64_THREAD_PM_VALID	(__IA64_UL(1) << 2)	/* performance registers valid? */
#define IA64_THREAD_UAC_NOPRINT	(__IA64_UL(1) << 3)	/* don't log unaligned accesses */
#define IA64_THREAD_UAC_SIGBUS	(__IA64_UL(1) << 4)	/* generate SIGBUS on unaligned acc. */
#define IA64_THREAD_MIGRATION	(__IA64_UL(1) << 5)	/* require migration
							   sync at ctx sw */
#define IA64_THREAD_FPEMU_NOPRINT (__IA64_UL(1) << 6)	/* don't log any fpswa faults */
#define IA64_THREAD_FPEMU_SIGFPE  (__IA64_UL(1) << 7)	/* send a SIGFPE for fpswa faults */

#define IA64_THREAD_UAC_SHIFT	3
#define IA64_THREAD_UAC_MASK	(IA64_THREAD_UAC_NOPRINT | IA64_THREAD_UAC_SIGBUS)
#define IA64_THREAD_FPEMU_SHIFT	6
#define IA64_THREAD_FPEMU_MASK	(IA64_THREAD_FPEMU_NOPRINT | IA64_THREAD_FPEMU_SIGFPE)


/*
 * This shift should be large enough to be able to represent 1000000000/itc_freq with good
 * accuracy while being small enough to fit 10*1000000000<<IA64_NSEC_PER_CYC_SHIFT in 64 bits
 * (this will give enough slack to represent 10 seconds worth of time as a scaled number).
 */
#define IA64_NSEC_PER_CYC_SHIFT	30

#ifndef __ASSEMBLY__

#include <linux/cache.h>
#include <linux/compiler.h>
#include <linux/t