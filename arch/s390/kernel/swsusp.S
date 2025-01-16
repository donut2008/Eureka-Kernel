16,sp
#endif
	;;
	st8.spill [r15]=r5,SW(R7)-SW(R5)	// spill r5
	mov.m ar.rsc=0			// put RSE in mode: enforced lazy, little endian, pl 0
	add r2=SW(F2)+16,sp		// r2 = &sw->f2
	;;
	st8.spill [r14]=r6,SW(B0)-SW(R6)	// spill r6
	mov.m r18=ar.fpsr		// preserve fpsr
	add r3=SW(F3)+16,sp		// r3 = &sw->f3
	;;
	stf.spill [r2]=f2,32
	mov.m r19=ar.rnat
	mov r21=b0

	stf.spill [r3]=f3,32
	st8.spill [r15]=r7,SW(B2)-SW(R7)	// spill r7
	mov r22=b1
	;;
	// since we're done with the spills, read and save ar.unat:
	mov.m r29=ar.unat
	mov.m r20=ar.bspstore
	mov r23=b2
	stf.spill [r2]=f4,32
	stf.spill [r3]=f5,32
	mov r24=b3
	;;
	st8 [r14]=r21,SW(B1)-SW(B0)		// save b0
	st8 [r15]=r23,SW(B3)-SW(B2)		// save b2
	mov r25=b4
	mov r26=b5
	;;
	st8 [r14]=r22,SW(B4)-SW(B1)		// save b1
	st8 [r15]=r24,SW(AR_PFS)-SW(B3)		// save b3
	mov r21=ar.lc		// I-unit
	stf.spill [r2]=f12,32
	stf.spill [r3]=f13,32
	;;
	st8 [r14]=r25,SW(B5)-SW(B4)		// save b4
	st8 [r15]=r16,SW(AR_LC)-SW(AR_PFS)	// save ar.pfs
	stf.spill [r2]=f14,32
	stf.spill [r3]=f15,32
	;;
	st8 [r14]=r26				// save b5
	st8 [r15]=r21				// save ar.lc
	stf.spill [r2]=f16,32
	stf.spill [r3]=f17,32
	;;
	stf.spill [r2]=f18,32
	stf.spill [r3]=f19,32
	;;
	stf.spill [r2]=f20,32
	stf.spill [r3]=f21,32
	;;
	stf.spill [r2]=f22,32
	stf.spill [r3]=f23,32
	;;
	stf.spill [r2]=f24,32
	stf.spill [r3]=f25,32
	;;
	stf.spill [r2]=f26,32
	stf.spill [r3]=f27,32
	;;
	stf.spill [r2]=f28,32
	stf.spill [r3]=f29,32
	;;
	stf.spill [r2]=f30,SW(AR_UNAT)-SW(F30)
	stf.spill [r3]=f31,SW(PR)-SW(F31)
	add r14=SW(CALLER_UNAT)+16,sp
	;;
	st8 [r2]=r29,SW(AR_RNAT)-SW(AR_UNAT)	// save ar.unat
	st8 [r14]=r17,SW(AR_FPSR)-SW(CALLER_UNAT) // save caller_unat
	mov r21=pr
	;;
	st8 [r2]=r19,SW(AR_BSPSTORE)-SW(AR_RNAT) // save ar.rnat
	st8 [r3]=r21				// save predicate registers
	;;
	st8 [r2]=r20				// save ar.bspstore
	st8 [r14]=r18				// save fpsr
	mov ar.rsc=3		// put RSE back into eager mode, pl 0
	br.cond.sptk.many b7
END(save_switch_stack)

/*
 * load_switch_stack:
 *	- "invala" MUST be done at call site (normally in DO_LOAD_SWITCH_STACK)
 *	- b7 holds address to return to
 *	- must not touch r8-r11
 */
GLOBAL_ENTRY(load_switch_stack)
	.prologue
	.altrp b7

	.body
	lfetch.fault.nt1 [sp]
	adds r2=SW(AR_BSPSTORE)+16,sp
	adds r3=SW(AR_UNAT)+16,sp
	mov ar.rsc=0						// put RSE into enforced lazy mode
	adds r14=SW(CALLER_UNAT)+16,sp
	adds r15=SW(AR_FPSR)+16,sp
	;;
	ld8 r27=[r2],(SW(B0)-SW(AR_BSPSTORE))	// bspstore
	ld8 r29=[r3],(SW(B1)-SW(AR_UNAT))	// unat
	;;
	ld8 r21=[r2],16		// restore b0
	ld8 r22=[r3],16		// restore b1
	;;
	ld8 r23=[r2],16		// restore b2
	ld8 r24=[r3],16		// restore b3
	;;
	ld8 r25=[r2],16		// restore b4
	ld8 r26=[r3],16		// restore b5
	;;
	ld8 r16=[r2],(SW(PR)-SW(AR_PFS))	// ar.pfs
	ld8 r17=[r3],(SW(AR_RNAT)-SW(AR_LC))	// ar.lc
	;;
	ld8 r28=[r2]		// restore pr
	ld8 r30=[r3]		// restore rnat
	;;
	ld8 r18=[r14],16	// restore caller's unat
	ld8 r19=[r15],24	// restore fpsr
	;;
	ldf.fill f2=[r14],32
	ldf.fill f3=[r15],32
	;;
	ldf.fill f4=[r14],32
	ldf.fill f5=[r15],32
	;;
	ldf.fill f12=[r14],32
	ldf.fill f13=[r15],32
	;;
	ldf.fill f14=[r14],32
	ldf.fill f15=[r15],32
	;;
	ldf.fill f16=[r14],32
	ldf.fill f17=[r15],32
	;;
	ldf.fill f18=[r14],32
	ldf.fill f19=[r15],32
	mov b0=r21
	;;
	ldf.fill f20=[r14],32
	ldf.fill f21=[r15],32
	mov b1=r22
	;;
	ldf.fill f22=[r14],32
	ldf.fill f23=[r15],32
	mov b2=r23
	;;
	mov ar.bspstore=r27
	mov ar.unat=r29		// establish unat holding the NaT bits for r4-r7
	mov b3=r24
	;;
	ldf.fill f24=[r14],32
	ldf.fill f25=[r15],32
	mov b4=r25
	;;
	ldf.fill f26=[r14],32
	ldf.fill f27=[r15],32
	mov b5=r26
	;;
	ldf.fill f28=[r14],32
	ldf.fill f29=[r15],32
	mov ar.pfs=r16
	;;
	ldf.fill f30=[r14],32
	ldf.fill f31=[r15],24
	mov ar.lc=r17
	;;
	ld8.fill r4=[r14],16
	ld8.fill r5=[r15],16
	mov pr=r28,-1
	;;
	ld8.fill r6=[r14],16
	ld8.fill r7=[r15],16

	mov ar.unat=r18				// restore caller's unat
	mov ar.rnat=r30				// must restore after bspstore but before rsc!
	mov ar.fpsr=r19				// restore fpsr
	mov ar.rsc=3				// put RSE back into eager mode, pl 0
	br.cond.sptk.many b7
END(load_switch_stack)

GLOBAL_ENTRY(prefetch_stack)
	add r14 = -IA64_SWITCH_STACK_SIZE, sp
	add r15 = IA64_TASK_THREAD_KSP_OFFSET, in0
	;;
	ld8 r16 = [r15]				// load next's stack pointer
	lfetch.fault.excl [r14], 128
	;;
	lfetch.fault.excl [r14], 128
	lfetch.fault [r16], 128
	;;
	lfetch.fault.excl [r14], 128
	lfetch.fault [r16], 128
	;;
	lfetch.fault.excl [r14], 128
	lfetch.fault [r16], 128
	;;
	lfetch.fault.excl [r14], 128
	lfetch.fault [r16], 128
	;;
	lfetch.fault [r16], 128
	br.ret.sptk.many rp
END(prefetch_stack)

	/*
	 * Invoke a system call, but do some tracing before and after the call.
	 * We MUST preserve the current register frame throughout this routine
	 * because some system calls (such as ia64_execve) directly
	 * manipulate ar.pfs.
	 */
GLOBAL_ENTRY(ia64_trace_syscall)
	PT_REGS_UNWIND_INFO(0)
	/*
	 * We need to preserve the scratch registers f6-f11 in case the system
	 * call is sigreturn.
	 */
	adds r16=PT(F6)+16,sp
	adds r17=PT(F7)+16,sp
	;;
 	stf.spill [r16]=f6,32
 	stf.spill [r17]=f7,32
	;;
 	stf.spill [r16]=f8,32
 	stf.spill [r17]=f9,32
	;;
 	stf.spill [r16]=f10
 	stf.spill [r17]=f11
	br.call.sptk.many rp=syscall_trace_enter // give parent a chance to catch syscall args
	cmp.lt p6,p0=r8,r0			// check tracehook
	adds r2=PT(R8)+16,sp			// r2 = &pt_regs.r8
	adds r3=PT(R10)+16,sp			// r3 = &pt_regs.r10
	mov r10=0
(p6)	br.cond.sptk strace_error		// syscall failed ->
	adds r16=PT(F6)+16,sp
	adds r17=PT(F7)+16,sp
	;;
	ldf.fill f6=[r16],32
	ldf.fill f7=[r17],32
	;;
	ldf.fill f8=[r16],32
	ldf.fill f9=[r17],32
	;;
	ldf.fill f10=[r16]
	ldf.fill f11=[r17]
	// the syscall number may have changed, so re-load it and re-calculate the
	// syscall entry-point:
	adds r15=PT(R15)+16,sp			// r15 = &pt_regs.r15 (syscall #)
	;;
	ld8 r15=[r15]
	mov r3=NR_syscalls - 1
	;;
	adds r15=-1024,r15
	movl r16=sys_call_table
	;;
	shladd r20=r15,3,r16			// r20 = sys_call_table + 8*(syscall-1024)
	cmp.leu p6,p7=r15,r3
	;;
(p6)	ld8 r20=[r20]				// load address of syscall entry point
(p7)	movl r20=sys_ni_syscall
	;;
	mov b6=r20
	br.call.sptk.many rp=b6			// do the syscall
.strace_check_retval:
	cmp.lt p6,p0=r8,r0			// syscall failed?
	adds r2=PT(R8)+16,sp			// r2 = &pt_regs.r8
	adds r3=PT(R10)+16,sp			// r3 = &pt_regs.r10
	mov r10=0
(p6)	br.cond.sptk strace_error		// syscall failed ->
	;;					// avoid RAW on r10
.strace_save_retval:
.mem.offset 0,0; st8.spill [r2]=r8		// store return value in slot for r8
.mem.offset 8,0; st8.spill [r3]=r10		// clear error indication in slot for r10
	br.call.sptk.many rp=syscall_trace_leave // give parent a chance to catch return value
.ret3:
(pUStk)	cmp.eq.unc p6,p0=r0,r0			// p6 <- pUStk
(pUStk)	rsm psr.i				// disable interrupts
	br.cond.sptk ia64_work_pending_syscall_end

strace_error:
	ld8 r3=[r2]				// load pt_regs.r8
	sub r