/*
 * This file contains the code that gets mapped at the upper end of each task's text
 * region.  For now, it contains the signal trampoline code only.
 *
 * Copyright (C) 1999-2003 Hewlett-Packard Co
 * 	David Mosberger-Tang <davidm@hpl.hp.com>
 */


#include <asm/asmmacro.h>
#include <asm/errno.h>
#include <asm/asm-offsets.h>
#include <asm/sigcontext.h>
#include <asm/unistd.h>
#include <asm/kregs.h>
#include <asm/page.h>
#include <asm/native/inst.h>

/*
 * We can't easily refer to symbols inside the kernel.  To avoid full runtime relocation,
 * complications with the linker (which likes to create PLT stubs for branches
 * to targets outside the shared object) and to avoid multi-phase kernel builds, we
 * simply create minimalistic "patch lists" in special ELF sections.
 */
	.section ".data..patch.fsyscall_table", "a"
	.previous
#define LOAD_FSYSCALL_TABLE(reg)			\
[1:]	movl reg=0;					\
	.xdata4 ".data..patch.fsyscall_table", 1b-.

	.section ".data..patch.brl_fsys_bubble_down", "a"
	.previous
#define BRL_COND_FSYS_BUBBLE_DOWN(pr)			\
[1:](pr)brl.cond.sptk 0;				\
	;;						\
	.xdata4 ".data..patch.brl_fsys_bubble_down", 1b-.

GLOBAL_ENTRY(__kernel_syscall_via_break)
	.prologue
	.altrp b6
	.body
	/*
	 * Note: for (fast) syscall restart to work, the break instruction must be
	 *	 the first one in the bundle addressed by syscall_via_break.
	 */
{ .mib
	break 0x100000
	nop.i 0
	br.ret.sptk.many b6
}
END(__kernel_syscall_via_break)

#	define ARG0_OFF		(16 + IA64_SIGFRAME_ARG0_OFFSET)
#	define ARG1_OFF		(16 + IA64_SIGFRAME_ARG1_OFFSET)
#	define ARG2_OFF		(16 + IA64_SIGFRAME_ARG2_OFFSET)
#	define SIGHANDLER_OFF	(16 + IA64_SIGFRAME_HANDLER_OFFSET)
#	define SIGCONTEXT_OFF	(16 + IA64_SIGFRAME_SIGCONTEXT_OFFSET)

#	define FLAGS_OFF	IA64_SIGCONTEXT_FLAGS_OFFSET
#	define CFM_OFF		IA64_SIGCONTEXT_CFM_OFFSET
#	define FR6_OFF		IA64_SIGCONTEXT_FR6_OFFSET
#	define BSP_OFF		IA64_SIGCONTEXT_AR_BSP_OFFSET
#	define RNAT_OFF		IA64_SIGCONTEXT_AR_RNAT_OFFSET
#	define UNAT_OFF		IA64_SIGCONTEXT_AR_UNAT_OFFSET
#	define FPSR_OFF		IA64_SIGCONTEXT_AR_FPSR_OFFSET
#	define PR_OFF		IA64_SIGCONTEXT_PR_OFFSET
#	define RP_OFF		IA64_SIGCONTEXT_IP_OFFSET
#	define SP_OFF		IA64_SIGCONTEXT_R12_OFFSET
#	define RBS_BASE_OFF	IA64_SIGCONTEXT_RBS_BASE_OFFSET
#	define LOADRS_OFF	IA64_SIGCONTEXT_LOADRS_OFFSET
#	define base0		r2
#	define base1		r3
	/*
	 * When we get here, the memory stack looks like this:
	 *
	 *   +===============================+
       	 *   |				     |
       	 *   //	    struct sigframe          //
       	 *   |				     |
	 *   +-------------------------------+ <-- sp+16
	 *   |      16 byte of scratch       |
	 *   |            space              |
	 *   +-------------------------------+ <-- sp
	 *
	 * The register stack looks _exactly_ the way it looked at the time the signal
	 * occurred.  In other words, we're treading on a potential mine-field: each
	 * incoming general register may be a NaT val