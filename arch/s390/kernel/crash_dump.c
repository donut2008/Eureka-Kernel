lone)
	PT_REGS_UNWIND_INFO(0)
{	/*
	 * Some versions of gas generate bad unwind info if the first instruction of a
	 * procedure doesn't go into the first slot of a bundle.  This is a workaround.
	 */
	nop.m 0
	nop.i 0
	/*
	 * We need to call schedule_tail() to complete the scheduling process.
	 * Called by ia64_switch_to() after do_fork()->copy_thread().  r8 contains the
	 * address of the previously executing task.
	 */
	br.call.sptk.many rp=ia64_invoke_schedule_tail
}
.ret8:
(pKStk)	br.call.sptk.many rp=call_payload
	adds r2=TI_FLAGS+IA64_TASK_SIZE,r13
	;;
	ld4 r2=[r2]
	;;
	mov r8=0
	and r2=_TIF_SYSCALL_TRACEAUDIT,r2
	;;
	cmp.ne p6,p0=r2,r0
(p6)	br.cond.spnt .strace_check_retval
	;;					// added stop bits to prevent r8 dependency
END(ia64_ret_from_clone)
	// fall through
GLOBAL_ENTRY(ia64_ret_from_syscall)
	PT_REGS_UNWIND_INFO(0)
	cmp.ge p6,p7=r8,r0			// syscall executed successfully?
	adds r2=PT(R8)+16,sp			// r2 = &pt_regs.r8
	mov r10=r0				// clear error indication in r10
(p7)	br.cond.spnt handle_syscall_error	// handle potential syscall failure
END(ia64_ret_from_syscall)
	// fall through

/*
 * ia64_leave_syscall(): Same as ia64_leave_kernel, except that it doesn't
 *	need to switch to bank 0 and doesn't restore the scratch registers.
 *	To avoid leaking kernel bits, the scratch registers are set to
 *	the following known-to-be-safe values:
 *
 *		  r1: restored (global pointer)
 *		  r2: cleared
 *		  r3: 1 (when returning to user-level)
 *	      r8-r11: restored (syscall return value(s))
 *		 r12: restored (user-level stack pointer)
 *		 r13: restored (user-level thread pointer)
 *		 r14: set to __kernel_syscall_via_epc
 *		 r15: restored (syscall #)
 *	     r16-r17: cleared
 *		 r18: user-level b6
 *		 r19: cleared
 *		 r20: user-level ar.fpsr
 *		 r21: user-level b0
 *		 r22: cleared
 *		 r23: user-level ar.bspstore
 *		 r24: user-level ar.rnat
 *		 r25: user-level ar.unat
 *		 r26: user-level ar.pfs
 *		 r27: user-level ar.rsc
 *		 r28: user-level ip
 *		 r29: user-level psr
 *		 r30: user-level cfm
 *		 r31: user-level pr
 *	      f6-f11: cleared
 *		  pr: restored (user-level pr)
 *		  b0: restored (user-level rp)
 *	          b6: restored
 *		  b7: set to __kernel_syscall_via_epc
 *	     ar.unat: restored (user-level ar.unat)
 *	      ar.pfs: restored (user-level ar.pfs)
 *	      ar.rsc: restored (user-level ar.rsc)
 *	     ar.rnat: restored (user-level ar.rnat)
 *	 ar.bspstore: restored (user-level ar.bspstore)
 *	     ar.fpsr: restored (user-level ar.fpsr)
 *	      ar.ccv: cleared
 *	      ar.csd: cleared
 *	      ar.ssd: cleared
 */
GLOBAL_ENTRY(ia64_leave_syscall)
	PT_REGS_UNWIND_INFO(0)
	/*
	 * work.need_resched etc. mustn't get changed by this CPU before it returns to
	 * user- or fsys-mode, hence we disable interrupts early on.
	 *
	 * p6 controls whether current_thread_info()->flags needs to be check for
	 * extra work.  We always check for extra work when returning to user-level.
	 * With CONFIG_PREEMPT, we also check for extra work when the preempt_count
	 * is 0.  After extra work processing has been completed, execution
	 * resumes at ia64_work_processed_syscall with p6 set to 1 if the extra-work-check
	 * needs to be redone.
	 */
#ifdef CONFIG_PREEMPT
	RSM_PSR_I(p0, r2, r18)			// disable interrupts
	cmp.eq pLvSys,p0=r0,r0			// pLvSys=1: leave from syscall
(pKStk) adds r20=TI_PRE_COUNT+IA64_TASK_SIZE,r13
	;;
	.pred.rel.mutex pUStk,pKStk
(pKStk) ld4 r21=[r20]			// r21 <- preempt_count
(pUStk)	mov r21=0			// r21 <- 0
	;;
	cmp.eq p6,p0=r21,r0		// p6 <- pUStk || (preempt_count == 0)
#else /* !CONFIG_PREEMPT */
	RSM_PSR_I(pUStk, r2, r18)
	cmp.eq pLvSys,p0=r0,r0		// pLvSys=1: leave from syscall
(pUStk)	cmp.eq.unc p6,p0=r0,r0		// p6 <- pUStk
#endif
.global ia64_work_processed_syscall;
ia64_work_processed_syscall:
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
	adds r2=PT(LOADRS)+16,r12
	MOV_FROM_ITC(pUStk, p9, r22, r19)	// fetch time at leave
	adds r18=TI_FLAGS+IA64_TASK_SIZE,r13
	;;
(p6)	ld4 r31=[r18]				// load current_thread_info()->flags
	ld8 r19=[r2],PT(B6)-PT(LOADRS)		// load ar.rsc value for "loadrs"
	adds r3=PT(AR_BSPSTORE)+16,r12		// deferred
	;;
#else
	adds r2=PT(LOADRS)+16,r12
	adds r3=PT(AR_BSPSTORE)+16,r12
	adds r18=TI_FLAGS+IA64_TASK_SIZE,r13
	;;
(p6)	ld4 r31=[r18]				// load current_thread_info()->flags
	ld8 r19=[r2],PT(B6)-PT(LOADRS)		// load ar.rsc value for "loadrs"
	nop.i 0
	;;
#endif
	mov r16=ar.bsp				// M2  get existing backing store pointer
	ld8 r18=[r2],PT(R9)-PT(B6)		// load b6
(p6)	and r15=TIF_WORK_MASK,r31		// any work other than TIF_SYSCALL_TRACE?
	;;
	ld8 r23=[r3],PT(R11)-PT(AR_BSPSTORE)	// load ar.bspstore (may be garbage)
(p6)	cmp4.ne.unc p6,p0=r15, r0		// any special work pending?
(p6)	br.cond.spnt .work_pending_syscall
	;;
	// start restoring the state saved on the kernel stack (struct pt_regs):
	ld8 r9=[r2],PT(CR_IPSR)-PT(R9)
	ld8 r11=[r3],PT(CR_IIP)-PT(R11)
(pNonSys) break 0		//      bug check: we shouldn't be here if pNonSys is TRUE!
	;;
	invala			// M0|1 invalidate ALAT
	RSM_PSR_I_IC(r28, r29, r30)	// M2   turn off interrupts and interruption collection
	cmp.eq p9,p0=r0,r0	// A    set p9 to indicate that we should restore cr.ifs

	ld8 r29=[r2],16		// M0|1 load cr.ipsr
	ld8 r28=[r3],16		// M0|1 load cr.iip
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
(pUStk) add r14=TI_AC_LEAVE+IA64_TASK_SIZE,r13
	;;
	ld8 r30=[r2],16		// M0|1 load cr.ifs
	ld8 r25=[r3],16		// M0|1 load ar.unat
(pUStk) add r15=IA64_TASK_THREAD_ON_USTACK_OFFSET,r13
	;;
#else
	mov r22=r0		// A    clear r22
	;;
	ld8 r30=[r2],16		// M0|1 load cr.ifs
	ld8 r25=[r3],16		// M0|1 load ar.unat
(pUStk) add r14=IA64_TASK_THREAD_ON_USTACK_OFFSET,r13
	;;
#endif
	ld8 r26=[r2],PT(B0)-PT(AR_PFS)	// M0|1 load ar.pfs
	MOV_FROM_PSR(pKStk, r22, r21)	// M2   read PSR now that interrupts are disabled
	nop 0
	;;
	ld8 r21=[r2],PT(AR_RNAT)-PT(B0) // M0|1 load b0
	ld8 r27=[r3],PT(PR)-PT(AR_RSC)	// M0|1 load ar.rsc
	mov f6=f0			// F    clear f6
	;;
	ld8 r24=[r2],PT(AR_FPSR)-PT(AR_RNAT)	// M0|1 load ar.rnat (may be garbage)
	ld8 r31=[r3],PT(R1)-PT(PR)		// M0|1 load predicates
	mov f7=f0				// F    clear f7
	;;
	ld8 r20=[r2],PT(R12)-PT(AR_FPSR)	// M0|1 load ar.fpsr
	ld8.fill r1=[r3],16			// M0|1 load r1
(pUStk) mov r17=1				// A
	;;
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
(pUStk) st1 [r15]=r17				// M2|3
#else
(pUStk) st1 [r14]=r17				// M2|3
#endif
	ld8.fill r13=[r3],16			// M0|1
	mov f8=f0				// F    clear f8
	;;
	ld8.fill r12=[r2]			// M0|1 restore r12 (sp)
	ld8.fill r15=[r3]			// M0|1 restore r15
	mov b6=r18				// I0   restore b6

	LOAD_PHYS_STACK_REG_SIZE(r17)
	mov f9=f0					// F    clear f9
(pKStk) br.cond.dpnt.many skip_rbs_switch		// B

	srlz.d				// M0   ensure interruption collection is off (for cover)
	shr.u r18=r19,16		// I0|1 get byte size of existing "dirty" partition
	COVER				// B    add current frame into dirty partition & set cr.ifs
	;;
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
	mov r19=ar.bsp			// M2   get new backing store pointer
	st8 [r14]=r22			// M	save time at leave
	mov f10=f0			// F    clear f10

	mov r22=r0			// A	clear r22
	movl r14=__kernel_syscall_via_epc // X
	;;
#else
	mov r19=ar.bsp			// M2   get new backing store pointer
	mov f10=f0			// F    clear f10

	nop.m 0
	movl r14=__kernel_syscall_via_epc // X
	;;
#endif
	mov.m ar.csd=r0			// M2   clear ar.csd
	mov.m ar.ccv=r0			// M2   clear ar.ccv
	mov b7=r14			// I0   clear b7 (hint with __kernel_syscall_via_epc)

	mov.m ar.ssd=r0			// M2   clear ar.ssd
	mov f11=f0			// F    clear f11
	br.cond.sptk.many rbs_switch	// B
END(ia64_leave_syscall)

GLOBAL_ENTRY(ia64_leave_kernel)
	PT_REGS_UNWIND_INFO(0)
	/*
	 * work.need_resched etc. mustn't get changed by this CPU before it returns to
	 * user- or fsys-mode, hence we disable interrupts early on.
	 *
	 * p6 controls whether current_thread_info()->flags needs to be check for
	 * extra work.  We always check for extra work when returning to user-level.
	 * With CONFIG_PREEMPT, we also check for extra work when the preempt_count
	 * is 0.  After extra work processing has been completed, execution
	 * resumes at .work_processed_syscall with p6 set to 1 if the extra-work-check
	 * needs to be redone.
	 */
#ifdef CONFIG_PREEMPT
	RSM_PSR_I(p0, r17, r31)			// disable interrupts
	cmp.eq p0,pLvSys=r0,r0			// pLvSys=0: leave from kernel
(pKStk)	adds r20=TI_PRE_COUNT+IA64_TASK_SIZE,r13
	;;
	.pred.rel.mutex pUStk,pKStk
(pKStk)	ld4 r21=[r20]			// r21 <- preempt_count
(pUStk)	mov r21=0			// r21 <- 0
	;;
	cmp.eq p6,p0=r21,r0		// p6 <- pUStk || (preempt_count == 0)
#else
	RSM_PSR_I(pUStk, r17, r31)
	cmp.eq p0,pLvSys=r0,r0		// pLvSys=0: leave from kernel
(pUStk)	cmp.eq.unc p6,p0=r0,r0		// p6 <- pUStk
#endif
.work_processed_kernel:
	adds r17=TI_FLAGS+IA64_TASK_SIZE,r13
	;;
(p6)	ld4 r31=[r17]				// load current_thread_info()->flags
	adds r21=PT(PR)+16,r12
	;;

	lfetch [r21],PT(CR_IPSR)-PT(PR)
	adds r2=PT(B6)+16,r12
	adds r3=PT(R16)+16,r12
	;;
	lfetch [r21]
	ld8 r28=[r2],8		// load b6
	adds r29=PT(R24)+16,r12

	ld8.fill r16=[r3],PT(AR_CSD)-PT(R16)
	adds r30=PT(AR_CCV)+16,r12
(p6)	and r19=TIF_WORK_MASK,r31		// any work other than TIF_SYSCALL_TRACE?
	;;
	ld8.fill r24=[r29]
	ld8 r15=[r30]		// load ar.ccv
(p6)	cmp4.ne.unc p6,p0=r19, r0		// any special work pending?
	;;
	ld8 r29=[r2],16		// load b7
	ld8 r30=[r3],16		// load ar.csd
(p6)	br.cond.spnt .work_pending
	;;
	ld8 r31=[r2],16		// load ar.ssd
	ld8.fill r8=[r3],16
	;;
	ld8.fill r9=[r2],16
	ld8.fill r10=[r3],PT(R17)-PT(R10)
	;;
	ld8.fill r11=[r2],PT(R18)-PT(R11)
	ld8.fill r17=[r3],16
	;;
	ld8.fill r18=[r2],16
	ld8.fill r19=[r3],16
	;;
	ld8.fill r20=[r2],16
	ld8.fill r21=[r3],16
	mov ar.csd=r30
	mov ar.ssd=r31
	;;
	RSM_PSR_I_IC(r23, r22, r25)	// initiate turning off of interrupt and interruption collection
	invala			// invalidate ALAT
	;;
	ld8.fill r22=[r2],24
	ld8.fill r23=[r3],24
	mov b6=r28
	;;
	ld8.fill r25=[r2],16
	ld8.fill r26=[r3],16
	mov b7=r29
	;;
	ld8.fill r27=[r2],16
	ld8.fill r28=[r3],16
	;;
	ld8.fill r29=[r2],16
	ld8.fill r30=[r3],24
	;;
	ld8.fill r31=[r2],PT(F9)-PT(R31)
	adds r3=PT(F10)-PT(F6),r3
	;;
	ldf.fill f9=[r2],PT(F6)-PT(F9)
	ldf.fill f10=[r3],PT(F8)-PT(F10)
	;;
	ldf.fill f6=[r2],PT(F7)-PT(F6)
	;;
	ldf.fill f7=[r2],PT(F11)-PT(F7)
	ldf.fill f8=[r3],32
	;;
	srlz.d	// ensure that inter. collection is off (VHPT is don't care, since text is pinned)
	mov ar.ccv=r15
	;;
	ldf.fill f11=[r2]
	BSW_0(r2, r3, r15)	// switch back to bank 0 (no stop bit required beforehand...)
	;;
(pUStk)	mov r18=IA64_KR(CURRENT)// M2 (12 cycle read latency)
	adds r16=PT(CR_IPSR)+16,r12
	adds r17=PT(CR_IIP)+16,r12

#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
	.pred.rel.mutex pUStk,pKStk
	MOV_FROM_PSR(pKStk, r22, r29)	// M2 read PSR now that interrupts are disabled
	MOV_FROM_ITC(pUStk, p9, r22, r29)	// M  fetch time at leave
	nop.i 0
	;;
#else
	MOV_FROM_PSR(pKStk, r22, r29)	// M2 read PSR now that interrupts are disabled
	nop.i 0
	nop.i 0
	;;
#endif
	ld8 r29=[r16],16	// load cr.ipsr
	ld8 r28=[r17],16	// load cr.iip
	;;
	ld8 r30=[r16],16	// load cr.ifs
	ld8 r25=[r17],16	// load ar.unat
	;;
	ld8 r26=[r16],16	// load ar.pfs
	ld8 r27=[r17],16	// load ar.rsc
	cmp.eq p9,p0=r0,r0	// set p9 to indicate that we should restore cr.ifs
	;;
	ld8 r24=[r16],16	// load ar.rnat (may be garbage)
	ld8 r23=[r17],16	// load ar.bspstore (may be garbage)
	;;
	ld8 r31=[r16],16	// load predicates
	ld8 r21=[r17],16	// load b0
	;;
	ld8 r19=[r16],16	// load ar.rsc value for "loadrs"
	ld8.fill r1=[r17],16	// load r1
	;;
	ld8.fill r12=[r16],16
	ld8.fill r13=[r17],16
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
(pUStk)	adds r3=TI_AC_LEAVE+IA64_TASK_SIZE,r18
#else
(pUStk)	adds r18=IA64_TASK_THREAD_ON_USTACK_OFFSET,r18
#endif
	;;
	ld8 r20=[r16],16	// ar.fpsr
	ld8.fill r15=[r17],16
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
(pUStk)	adds r18=IA64_TASK_THREAD_ON_USTACK_OFFSET,r18	// deferred
#endif
	;;
	ld8.fill r14=[r16],16
	ld8.fill r2=[r17]
(pUStk)	mov r17=1
	;;
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
	//  mmi_ :  ld8 st1 shr;;         mmi_ : st8 st1 shr;;
	//  mib  :  mov add br        ->  mib  : ld8 add br
	//  bbb_ :  br  nop cover;;       mbb_ : mov br  cover;;
	//
	//  no one require bsp in r16 if (pKStk) branch is selected.
(pUStk)	st8 [r3]=r22		// save time at leave
(pUStk)	st1 [r18]=r17		// restore current->thread.on_ustack
	shr.u r18=r19,16	// get byte size of existing "dirty" partition
	;;
	ld8.fill r3=[r16]	// deferred
	LOAD_PHYS_STACK_REG_SIZE(r17)
(pKStk)	br.cond.dpnt skip_rbs_switch
	mov r16=ar.bsp		// get existing backing store pointer
#else
	ld8.fill r3=[r16]
(pUStk)	st1 [r18]=r17		// restore current->thread.on_ustack
	shr.u r18=r19,16	// get byte size of existing "dirty" partition
	;;
	mov r16=ar.bsp		// get existing backing store pointer
	LOAD_PHYS_STACK_REG_SIZE(r17)
(pKStk)	br.cond.dpnt skip_rbs_switch
#endif

	/*
	 * Restore user backing store.
	 *
	 * NOTE: alloc, loadrs, and cover can't be predicated.
	 */
(pNonSys) br.cond.dpnt dont_preserve_current_frame
	COVER				// add current frame into dirty partition and set cr.ifs
	;;
	mov r19=ar.bsp			// get new backing store pointer
rbs_switch:
	sub r16=r16,r18			// krbs = old bsp - size of dirty partition
	cmp.ne p9,p0=r0,r0		// clear p9 to skip restore of cr.ifs
	;;
	sub r19=r19,r16			// calculate total byte size of dirty partition
	add r18=64,r18			// don't force in0-in7 into memory...
	;;
	shl r19=r19,16			// shift size of dirty partition into loadrs position
	;;
dont_preserve_current_frame:
	/*
	 * To prevent leaking bits between the kernel and user-space,
	 * we must clear the stacked registers in the "invalid" partition here.
	 * Not pretty, but at least it's fast (3.34 registers/cycle on Itanium,
	 * 5 registers/cycle on McKinley).
	 */
#	define pRecurse	p6
#	define pReturn	p7
#ifdef CONFIG_ITANIUM
#	define Nregs	10
#else
#	define Nregs	14
#endif
	alloc loc0=ar.pfs,2,Nregs-2,2,0
	shr.u loc1=r18,9		// RNaTslots <= floor(dirtySize / (64*8))
	sub r17=r17,r18			// r17 = (physStackedSize + 8) - dirtySize
	;;
	mov ar.rsc=r19			// load ar.rsc to be used for "loadrs"
	shladd in0=loc1,3,r17
	mov in1=0
	;;
	TEXT_ALIGN(32)
rse_clear_invalid:
#ifdef CONFIG_ITANIUM
	// cycle 0
 { .mii
	alloc loc0=ar.pfs,2,Nregs-2,2,0
	cmp.lt pRecurse,p0=Nregs*8,in0	// if more than Nregs regs left to clear, (re)curse
	add out0=-Nregs*8,in0
}{ .mfb
	add out1=1,in1			// increment recursion count
	nop.f 0
	nop.b 0				// can't do br.call here because of alloc (WAW on CFM)
	;;
}{ .mfi	// cycle 1
	mov loc1=0
	nop.f 0
	mov loc2=0
}{ .mib
	mov loc3=0
	mov loc4=0
(pRecurse) br.call.sptk.many b0=rse_clear_invalid

}{ .mfi	// cycle 2
	mov loc5=0
	nop.f 0
	cmp.ne pReturn,p0=r0,in1	// if recursion count != 0, we need to do a br.ret
}{ .mib
	mov loc6=0
	mov loc7=0
(pReturn) br.ret.sptk.many b0
}
#else /* !CONFIG_ITANIUM */
	alloc loc0=ar.pfs,2,Nregs-2,2,0
	cmp.lt pRecurse,p0=Nregs*8,in0	// if more than Nregs regs left to clear, (re)curse
	add out0=-Nregs*8,in0
	add out1=1,in1			// increment recursion count
	mov loc1=0
	mov loc2=0
	;;
	mov loc3=0
	mov loc4=0
	mov loc5=0
	mov loc6=0
	mov loc7=0
(pRecurse) br.call.dptk.few b0=rse_clear_invalid
	;;
	mov loc8=0
	mov loc9=0
	cmp.ne pReturn,p0=r0,in1	// if recursion count != 0, we need to do a br.ret
	mov loc10=0
	mov loc11=0
(pReturn) br.ret.dptk.many b0
#endif /* !CONFIG_ITANIUM */
#	undef pRecurse
#	undef pReturn
	;;
	alloc r17=ar.pfs,0,0,0,0	// drop current register frame
	;;
	loadrs
	;;
skip_rbs_switch:
	mov ar.unat=r25		// M2
(pKStk)	extr.u r22=r22,21,1	// I0 extract current value of psr.pp from r22
(pLvSys)mov r19=r0		// A  clear r19 for leave_syscall, no-op otherwise
	;;
(pUStk)	mov ar.bspstore=r23	// M2
(pKStk)	dep r29=r22,r29,21,1	// I0 update ipsr.pp with psr.pp
(pLvSys)mov r16=r0		// A  clear r16 for leave_syscall, no-op otherwise
	;;
	MOV_TO_IPSR(p0, r29, r25)	// M2
	mov ar.pfs=r26		// I0
(pLvSys)mov r17=r0		// A  clear r17 for leave_syscall, no-op otherwise

	MOV_TO_IFS(p9, r30, r25)// M2
	mov b0=r21		// I0
(pLvSys)mov r18=r0		// A  clear r18 for leave_syscall, no-op otherwi