n entry, reenabled depend on work, and
	 * disabled on exit.
	 */
.work_pending_syscall:
	add r2=-8,r2
	add r3=-8,r3
	;;
	st8 [r2]=r8
	st8 [r3]=r10
.work_pending:
	tbit.z p6,p0=r31,TIF_NEED_RESCHED	// is resched not needed?
(p6)	br.cond.sptk.few .notify
	br.call.spnt.many rp=preempt_schedule_irq
.ret9:	cmp.eq p6,p0=r0,r0	// p6 <- 1 (re-check)
(pLvSys)br.cond.sptk.few  ia64_work_pending_syscall_end
	br.cond.sptk.many .work_processed_kernel

.notify:
(pUStk)	br.call.spnt.many rp=notify_resume_user
.ret10:	cmp.ne p6,p0=r0,r0	// p6 <- 0 (don't re-check)
(pLvSys)br.cond.sptk.few  ia64_work_pending_syscall_end
	br.cond.sptk.many .work_processed_kernel

.global ia64_work_pending_syscall_end;
ia64_work_pending_syscall_end:
	adds r2=PT(R8)+16,r12
	adds r3=PT(R10)+16,r12
	;;
	ld8 r8=[r2]
	ld8 r10=[r3]
	br.cond.sptk.many ia64_work_processed_syscall
END(ia64_leave_kernel)

ENTRY(handle_syscall_error)
	/*
	 * Some system calls (e.g., ptrace, mmap) can return arbitrary values which could
	 * lead us to mistake a negative return value as a failed syscall.  Those syscall
	 * must deposit a non-zero value in pt_regs.r8 to indicate an error.  If
	 * pt_regs.r8 is zero, we assume that the call completed successfully.
	 */
	PT_REGS_UNWIND_INFO(0)
	ld8 r3=[r2]		// load pt_regs.r8
	;;
	cmp.eq p6,p7=r3,r0	// is pt_regs.r8==0?
	;;
(p7)	mov r10=-1
(p7)	sub r8=0,r8		// negate return value to get errno
	br.cond.sptk ia64_leave_syscall
END(handle_syscall_error)

	/*
	 * Invoke schedule_tail(task) while preserving in0-in7, which may be needed
	 * in case a system call gets restarted.
	 */
GLOBAL_ENTRY(ia64_invoke_schedule_tail)
	.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(8)
	alloc loc1=ar.pfs,8,2,1,0
	mov loc0=rp
	mov out0=r8				// Address of previous task
	;;
	br.call.sptk.many rp=schedule_tail
.ret11:	mov ar.pfs=loc1
	mov rp=loc0
	br.ret.sptk.many rp
END(ia64_invoke_schedule_tail)

	/*
	 * Setup stack and call do_notify_resume_user(), keeping interrupts
	 * disabled.
	 *
	 * Note that pSys and pNonSys need to be set up by the caller.
	 * We declare 8 input registers so the system call args get preserved,
	 * in case we need to restart a system call.
	 */
GLOBAL_ENTRY(notify_resume_user)
	.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(8)
	alloc loc1=ar.pfs,8,2,3,0 // preserve all eight input regs in case of syscall restart!
	mov r9=ar.unat
	mov loc0=rp				// save return address
	mov out0=0				// there is no "oldset"
	adds out1=8,sp				// out1=&sigscratch->ar_pfs
(pSys)	mov out2=1				// out2==1 => we're in a syscall
	;;
(pNonSys) mov out2=0				// out2==0 => not a syscall
	.fframe 16
	.spillsp ar.unat, 16
	st8 [sp]=r9,-16				// allocate space for ar.unat and save it
	st8 [out1]=loc1,-8			// save ar.pfs, out1=&sigscratch
	.body
	br.call.sptk.many rp=do_notify_resume_user
.ret15:	.restore sp
	adds sp=16,sp				// pop scratch stack space
	;;
	ld8 r9=[sp]				// load new unat from sigscratch->scratch_unat
	mov rp=loc0
	;;
	mov ar.unat=r9
	mov ar.pfs=loc1
	br.ret.sptk.many rp
END(notify_resume_user)

ENTRY(sys_rt_sigreturn)
	PT_REGS_