/*
 * arch/ia64/kernel/entry.S
 *
 * Kernel entry points.
 *
 * Copyright (C) 1998-2003, 2005 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999, 2002-2003
 *	Asit Mallick <Asit.K.Mallick@intel.com>
 * 	Don Dugger <Don.Dugger@intel.com>
 *	Suresh Siddha <suresh.b.siddha@intel.com>
 *	Fenghua Yu <fenghua.yu@intel.com>
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 */
/*
 * ia64_switch_to now places correct virtual mapping in in TR2 for
 * kernel stack. This allows us to handle interrupts without changing
 * to physical mode.
 *
 * Jonathan Nicklin	<nicklin@missioncriticallinux.com>
 * Patrick O'Rourke	<orourke@missioncriticallinux.com>
 * 11/07/2000
 */
/*
 * Copyright (c) 2008 Isaku Yamahata <yamahata at valinux co jp>
 *                    VA Linux Systems Japan K.K.
 *                    pv_ops.
 */
/*
 * Global (preserved) predicate usage on syscall entry/exit path:
 *
 *	pKStk:		See entry.h.
 *	pUStk:		See entry.h.
 *	pSys:		See entry.h.
 *	pNonSys:	!pSys
 */


#include <asm/asmmacro.h>
#include <asm/cache.h>
#include <asm/errno.h>
#include <asm/kregs.h>
#include <asm/asm-offsets.h>
#include <asm/pgtable.h>
#include <asm/percpu.h>
#include <asm/processor.h>
#include <asm/thread_info.h>
#include <asm/unistd.h>
#include <asm/ftrace.h>

#include "minstate.h"

	/*
	 * execve() is special because in case of success, we need to
	 * setup a null register window frame.
	 */
ENTRY(ia64_execve)
	/*
	 * Allocate 8 input registers since ptrace() may clobber them
	 */
	.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(8)
	alloc loc1=ar.pfs,8,2,3,0
	mov loc0=rp
	.body
	mov out0=in0			// filename
	;;				// stop bit between alloc and call
	mov out1=in1			// argv
	mov out2=in2			// envp
	br.call.sptk.many rp=sys_execve
.ret0:
	cmp4.ge p6,p7=r8,r0
	mov ar.pfs=loc1			// restore ar.pfs
	sxt4 r8=r8			// return 64-bit result
	;;
	stf.spill [sp]=f0
	mov rp=loc0
(p6)	mov ar.pfs=r0			// clear ar.pfs on success
(p7)	br.ret.sptk.many rp

	/*
	 * In theory, we'd have to zap this state only to prevent leaking of
	 * security sensitive state (e.g., if current->mm->dumpable is zero).  However,
	 * this executes in less than 20 cycles even on Itanium, so it's not worth
	 * optimizing for...).
	 */
	mov ar.unat=0; 		mov ar.lc=0
	mov r4=0;		mov f2=f0;		mov b1=r0
	mov r5=0;		mov f3=f0;		mov b2=r0
	mov r6=0;		mov f4=f0;		mov b3=r0
	mov r7=0;		mov f5=f0;		mov b4=r0
	ldf.fill f12=[sp];	mov f13=f0;		mov b5=r0
	ldf.fill f14=[sp];	ldf.fill f15=[sp];	mov f16=f0
	ldf.fill f17=[sp];	ldf.fill f18=[sp];	mov f19=f0
	ldf.fill f20=[sp];	ldf.fill f21=[sp];	mov f22=f0
	ldf.fill f23=[sp];	ldf.fill f24=[sp];	mov f25=f0
	ldf.fill f26=[sp];	ldf.fill f27=[sp];	mov f28=f0
	ldf.fill f29=[sp];	ldf.fill f30=[sp];	mov f31=f0
	br.ret.sptk.many rp
END(ia64_execve)

/*
 * sys_clone2(u64 flags, u64 ustack_base, u64 ustack_size, u64 parent_tidptr, u64 child_tidptr,
 *	      u64 tls)
 */
GLOBAL_ENTRY(sys_clone2)
	/*
	 * Allocate 8 input registers since ptrace() may clobber them
	 */
	.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(8)
	alloc r16=ar.pfs,8,2,6,0
	DO_SAVE_SWITCH_STACK
	adds r2=PT(R16)+IA64_SWITCH_STACK_SIZE+16,sp
	mov loc0=rp
	mov loc1=r16				// save ar.pfs across do_fork
	.body
	mov out1=in1
	mov out2=in2
	tbit.nz p6,p0=in0,CLONE_SETTLS_BIT
	mov out3=in3	// parent_tidptr: valid only w/CLONE_PARENT_SETTID
	;;
(p6)	st8 [r2]=in5				// store TLS in r16 for copy_thread()
	mov out4=in4	// child_tidptr:  valid only w/CLONE_CHILD_SETTID or CLONE_CHILD_CLEARTID
	mov out0=in0				// out0 = clone_flags
	br.call.sptk.many rp=do_fork
.ret1:	.restore sp
	adds sp=IA64_SWITCH_STACK_SIZE,sp	// pop the switch stack
	mov ar.pfs=loc1
	mov rp=loc0
	br.ret.sptk.many rp
END(sys_clone2)

/*
 * sys_clone(u64 flags, u64 ustack_base, u64 parent_tidptr, u64 child_tidptr, u64 tls)
 *	Deprecated.  Use sys_clone2() instead.
 */
GLOBAL_ENTRY(sys_clone)
	/*
	 * Allocate 8 input registers since ptrace() may clobber them
	 */
	.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(8)
	alloc r16=ar.pfs,8,2,6,0
	DO_SAVE_SWITCH_STACK
	adds r2=PT(R16)+IA64_SWITCH_STACK_SIZE+16,sp
	mov loc0=rp
	mov loc1=r16				// save ar.pfs across do_fork
	.body
	mov out1=in1
	mov out2=16				// stacksize (compensates for 16-byte scratch a