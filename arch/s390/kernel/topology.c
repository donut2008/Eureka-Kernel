/*
 * Linker script for gate DSO.  The gate pages are an ELF shared object
 * prelinked to its virtual address, with only one read-only segment and
 * one execute-only segment (both fit in one page).  This script controls
 * its layout.
 */

#include <asm/page.h>

SECTIONS
{
	. = GATE_ADDR + SIZEOF_HEADERS;

	.hash			: { *(.hash) }		:readable
	.gnu.hash		: { *(.gnu.hash) }
	.dynsym			: { *(.dynsym) }
	.dynstr			: { *(.dynstr) }
	.gnu.version		: { *(.gnu.version) }
	.gnu.version_d		: { *(.gnu.version_d) }
	.gnu.version_r		: { *(.gnu.version_r) }

	.note			: { *(.note*) }		:readable	:note

	.dynamic		: { *(.dynamic) }	:readable	:dynamic

	/*
	 * This linker script is used both with -r and with -shared.  For
	 * the layouts to match, we need to skip more than enough space for
	 * the dynamic symbol table et al.  If this amount is insufficient,
	 * ld -shared will barf.  Just increase it here.
	 */
	. = GATE_ADDR + 0x600;

	.data..patch		: {
		__start_gate_mckinley_e9_patchlist = .;
		*(.data..patch.mckinley_e9)
		__end_gate_mckinley_e9_patchlist = .;

		__start_gate_vtop_patchlist = .;
		*(.data..patch.vtop)
		__end_gate_vtop_patchlist = .;

		__start_gate_fsyscall_patchlist = .;
		*(.data..patch.fsyscall_table)
		__end_gate_fsyscall_patchlist = .;

		__start_gate_brl_fsys_bubble_down_patchlist = .;
		*(.data..patch.brl_fsys_bubble_down)
		__end_gate_brl_fsys_bubble_down_patchlist = .;
	}						:readable

	.IA_64.unwind_info	: { *(.IA_64.unwind_info*) }
	.IA_64.unwind		: { *(.IA_64.unwind*) }	:readable	:unwind
#ifdef HAVE_BUGGY_SEGREL
	.text (GATE_ADDR + PAGE_SIZE) : { *(.text) *(.text.*) }	:readable
#else
	. = ALIGN(PERCPU_PAGE_SIZE) + (. & (PERCPU_PAGE_SIZE - 1));
	.text			: { *(.text) *(.text.*) }	:epc
#endif

	/DISCARD/		: {
		*(.got.plt) *(.got)
		*(.data .data.* .gnu.linkonce.d.*)
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(__ex_table)
		*(__mca_table)
	}
}

/*
 * ld does not recognize this name token; use the constant.
 */
#define	PT_IA_64_UNWIND	0x70000001

/*
 * We must supply the ELF program headers explicitly to get just one
 * PT_LOAD segment, and set the flags explicitly to make segments read-only.
 */
PHDRS
{
	readable	PT_LOAD	FILEHDR	PHDRS	FLAGS(4);	/* PF_R */
#ifndef HAVE_BUGGY_SEGREL
	epc		PT_LOAD	FILEHDR PHDRS	FLAGS(1);	/* PF_X */
#endif
	dynamic		PT_DYNAMIC		FLAGS(4);	/* PF_R */
	note		PT_NOTE			FLAGS(4);	/* PF_R */
	unwind		PT_IA_64_UNWIND;
}

/*
 * This controls what symbols we export from the DSO.
 */
VERSION
{
	LINUX_2.5 {
	global:
		__kernel_syscall_via_break;
		__kernel_syscall_via_epc;
		__kernel_sigtramp;

	local: *;
	};
}

/* The ELF entry point can be used to set the AT_SYSINFO value.  */
ENTRY(__kernel_syscall_via_epc)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
 * Here is where the ball gets rolling as far as the kernel is concerned.
 * When control is transferred to _start, the bootload has already
 * loaded us to the correct address.  All that's left to do here is
 * to set up the kernel's global pointer and jump to the kernel
 * entry point.
 *
 * Copyright (C) 1998-2001, 2003, 2005 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 * Copyright (C) 1999 Intel Corp.
 * Copyright (C) 1999 Asit Mallick <Asit.K.Mallick@intel.com>
 * Copyright (C) 1999 Don Dugger <Don.Dugger@intel.com>
 * Copyright (C) 2002 Fenghua Yu <fenghua.yu@intel.com>
 *   -Optimize __ia64_save_fpu() and __ia64_load_fpu() for Itanium 2.
 * Copyright (C) 2004 Ashok Raj <ashok.raj@intel.com>
 *   Support for CPU Hotplug
 */


#include <asm/asmmacro.h>
#include <asm/fpu.h>
#include <asm/kregs.h>
#include <asm/mmu_context.h>
#include <asm/asm-offsets.h>
#include <asm/pal.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/mca_asm.h>
#include <linux/init.h>
#include <linux/linkage.h>

#ifdef CONFIG_HOTPLUG_CPU
#define SAL_PSR_BITS_TO_SET				\
	(IA64_PSR_AC | IA64_PSR_BN | IA64_PSR_MFH | IA64_PSR_MFL)

#define SAVE_FROM_REG(src, ptr, dest)	\
	mov dest=src;;						\
	st8 [ptr]=dest,0x08

#define RESTORE_REG(reg, ptr, _tmp)		\
	ld8 _tmp=[ptr],0x08;;				\
	mov reg=_tmp

#define SAVE_BREAK_REGS(ptr, _idx, _breg, _dest)\
	mov ar.lc=IA64_NUM_DBG_REGS-1;; 			\
	mov _idx=0;; 								\
1: 												\
	SAVE_FROM_REG(_breg[_idx], ptr, _dest);;	\
	add _idx=1,_idx;;							\
	br.cloop.sptk.many 1b

#define RESTORE_BREAK_REGS(ptr, _idx, _breg, _tmp, _lbl)\
	mov ar.lc=IA64_NUM_DBG_REGS-1;;			\
	mov _idx=0;;							\
_lbl:  RESTORE_REG(_breg[_idx], ptr, _tmp);;	\
	add _idx=1, _idx;;						\
	br.cloop.sptk.many _lbl

#define SAVE_ONE_RR(num, _reg, _tmp) \
	movl _tmp=(num<<61);;	\
	mov _reg=rr[_tmp]

#define SAVE_REGION_REGS(_tmp, _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7) \
	SAVE_ONE_RR(0,_r0, _tmp);; \
	SAVE_ONE_RR(1,_r1, _tmp);; \
	SAVE_ONE_RR(2,_r2, _tmp);; \
	SAVE_ONE_RR(3,_r3, _tmp);; \
	SAVE_ONE_RR(4,_r4, _tmp);; \
	SAVE_ONE_RR(5,_r5, _tmp);; \
	SAVE_ONE_RR(6,_r6, _tmp);; \
	SAVE_ONE_RR(7,_r7, _tmp);;

#define STORE_REGION_REGS(ptr, _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7) \
	st8 [ptr]=_r0, 8;; \
	st8 [ptr]=_r1, 8;; \
	st8 [ptr]=_r2, 8;; \
	st8 [ptr]=_r3, 8;; \
	st8 [ptr]=_r4, 8;; \
	st8 [ptr]=_r5, 8;; \
	st8 [ptr]=_r6, 8;; \
	st8 [ptr]=_r7, 8;;

#define RESTORE_REGION_REGS(ptr, _idx1, _idx2, _tmp) \
	mov		ar.lc=0x08-1;;						\
	movl	_idx1=0x00;;						\
RestRR:											\
	dep.z	_idx2=_idx1,61,3;;					\
	ld8		_tmp=[ptr],8;;						\
	mov		rr[_idx2]=_tmp;;					\
	srlz.d;;									\
	add		_idx1=1,_idx1;;						\
	br.cloop.sptk.few	RestRR

#define SET_AREA_FOR_BOOTING_CPU(reg1, reg2) \
	movl reg1=sal_state_for_booting_cpu;;	\
	ld8 reg2=[reg1];;

/*
 * Adjust region registers saved before starting to save
 * break regs and rest of the states that need to be preserved.
 */
#define SAL_TO_OS_BOOT_HANDOFF_STATE_SAVE(_reg1,_reg2,_pred)  \
	SAVE_FROM_REG(b0,_reg1,_reg2);;						\
	SAVE_FROM_REG(b1,_reg1,_reg2);;						\
	SAVE_FROM_REG(b2,_reg1,_reg2);;						\
	SAVE_FROM_REG(b3,_reg1,_reg2);;						\
	SAVE_FROM_REG(b4,_reg1,_reg2);;						\
	SAVE_FROM_REG(b5,_reg1,_reg2);;						\
	st8 [_reg1]=r1,0x08;;								\
	st8 [_reg1]=r12,0x08;;								\
	st8 [_reg1]=r13,0x08;;								\
	SAVE_FROM_REG(ar.fpsr,_reg1,_reg2);;				\
	SAVE_FROM_REG(ar.pfs,_reg1,_reg2);;					\
	SAVE_FROM_REG(ar.rnat,_reg1,_reg2);;				\
	SAVE_FROM_REG(ar.unat,_reg1,_reg2);;				\
	SAVE_FROM_REG(ar.bspstore,_reg1,_reg2);;			\
	SAVE_FROM_REG(cr.dcr,_reg1,_reg2);;					\
	SAVE_FROM_REG(cr.iva,_reg1,_reg2);;					\
	SAVE_FROM_REG(cr.pta,_reg1,_reg2);;					\
	SAVE_FROM_REG(cr.itv,_reg1,_reg2);;					\
	SAVE_FROM_REG(cr.pmv,_reg1,_reg2);;					\
	SAVE_FROM_REG(cr.cmcv,_reg1,_reg2);;				\
	SAVE_FROM_REG(cr.lrr0,_reg1,_reg2);;				\
	SAVE_FROM_REG(cr.lrr1,_reg1,_reg2);;				\
	st8 [_reg1]=r4,0x08;;								\
	st8 [_reg1]=r5,0x08;;								\
	st8 [_reg1]=r6,0x08;;								\
	st8 [_reg1]=r7,0x08;;								\
	st8 [_reg1]=_pred,0x08;;							\
	SAVE_FROM_REG(ar.lc, _reg1, _reg2);;				\
	stf.spill.nta [_reg1]=f2,16;;						\
	stf.spill.nta [_reg1]=f3,16;;						\
	stf.spill.nta [_reg1]=f4,16;;						\
	stf.spill.nta [_reg1]=f5,16;;						\
	stf.spill.nta [_reg1]=f16,16;;						\
	stf.spill.nta [_reg1]=f17,16;;						\
	stf.spill.nta [_reg1]=f18,16;;						\
	stf.spill.nta [_reg1]=f19,16;;						\
	stf.spill.nta [_reg1]=f20,16;;						\
	stf.spill.nta [_reg1]=f21,16;;						\
	stf.spill.nta [_reg1]=f22,16;;						\
	stf.spill.nta [_reg1]=f23,16;;						\
	stf.spill.nta [_reg1]=f24,16;;						\
	stf.spill.nta [_reg1]=f25,16;;						\
	stf.spill.nta [_reg1]=f26,16;;						\
	stf.spill.nta [_reg1]=f27,16;;						\
	stf.spill.nta [_reg1]=f28,16;;						\
	stf.spill.nta [_reg1]=f29,16;;						\
	stf.spill.nta [_reg1]=f30,16;;						\
	stf.spill.nta [_reg1]=f31,16;;

#else
#define SET_AREA_FOR_BOOTING_CPU(a1, a2)
#define SAL_TO_OS_BOOT_HANDOFF_STATE_SAVE(a1,a2, a3)
#define SAVE_REGION_REGS(_tmp, _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7)
#define STORE_REGION_REGS(ptr, _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7)
#endif

#define SET_ONE_RR(num, pgsize, _tmp1, _tmp2, vhpt) \
	movl _tmp1=(num << 61);;	\
	mov _tmp2=((ia64_rid(IA64_REGION_ID_KERNEL, (num<<61)) << 8) | (pgsize << 2) | vhpt);; \
	mov rr[_tmp1]=_tmp2

	__PAGE_ALIGNED_DATA

	.global empty_zero_page
empty_zero_page:
	.skip PAGE_SIZE

	.global swapper_pg_dir
swapper_pg_dir:
	.skip PAGE_SIZE

	.rodata
halt_msg:
	stringz "Halting kernel\n"

	__REF

	.global start_ap

	/*
	 * Start the kernel.  When the bootloader passes control to _start(), r28
	 * points to the address of the boot parameter area.  Execution reaches
	 * here in physical mode.
	 */
GLOBAL_ENTRY(_start)
start_ap:
	.prologue
	.save rp, r0		// terminate unwind chain with a NULL rp
	.body

	rsm psr.i | psr.ic
	;;
	srlz.i
	;;
 {
	flushrs				// must be first insn in group
	srlz.i
 }
	;;
	/*
	 * Save the region registers, predicate before they get clobbered
	 */
	SAVE_REGION_REGS(r2, r8,r9,r10,r11,r12,r13,r14,r15);
	mov r25=pr;;

	/*
	 * Initialize kernel region registers:
	 *	rr[0]: VHPT enabled, page size = PAGE_SHIFT
	 *	rr[1]: VHPT enabled, page size = PAGE_SHIFT
	 *	rr[2]: VHPT enabled, page size = PAGE_SHIFT
	 *	rr[3]: VHPT enabled, page size = PAGE_SHIFT
	 *	rr[4]: VHPT enabled, page size = PAGE_SHIFT
	 *	rr[5]: VHPT enabled, page size = PAGE_SHIFT
	 *	rr[6]: VHPT disabled, page size = IA64_GRANULE_SHIFT
	 *	rr[7]: VHPT disabled, page size = IA64_GRANULE_SHIFT
	 * We initialize all of them to prevent inadvertently assuming
	 * something about the state of address translation early in boot.
	 */
	SET_ONE_RR(0, PAGE_SHIFT, r2, r16, 1);;
	SET_ONE_RR(1, PAGE_SHIFT, r2, r16, 1);;
	SET_ONE_RR(2, PAGE_SHIFT, r2, r16, 1);;
	SET_ONE_RR(3, PAGE_SHIFT, r2, r16, 1);;
	SET_ONE_RR(4, PAGE_SHIFT, r2, r16, 1);;
	SET_ONE_RR(5, PAGE_SHIFT, r2, r16, 1);;
	SET_ONE_RR(6, IA64_GRANULE_SHIFT, r2, r16, 0);;
	SET_ONE_RR(7, IA64_GRANULE_SHIFT, r2, r16, 0);;
	/*
	 * Now pin mappings into the TLB for kernel text and data
	 */
	mov r18=KERNEL_TR_PA