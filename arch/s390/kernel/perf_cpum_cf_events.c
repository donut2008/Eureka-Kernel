/*
 * This file contains the light-weight system call handlers (fsyscall-handlers).
 *
 * Copyright (C) 2003 Hewlett-Packard Co
 * 	David Mosberger-Tang <davidm@hpl.hp.com>
 *
 * 25-Sep-03 davidm	Implement fsys_rt_sigprocmask().
 * 18-Feb-03 louisk	Implement fsys_gettimeofday().
 * 28-Feb-03 davidm	Fixed several bugs in fsys_gettimeofday().  Tuned it some more,
 *			probably broke it along the way... ;-)
 * 13-Jul-04 clameter   Implement fsys_clock_gettime and revise fsys_gettimeofday to make
 *                      it capable of using memory based clocks without falling back to C code.
 * 08-Feb-07 Fenghua Yu Implement fsys_getcpu.
 *
 */

#include <asm/asmmacro.h>
#include <asm/errno.h>
#include <asm/asm-offsets.h>
#include <asm/percpu.h>
#include <asm/thread_info.h>
#include <asm/sal.h>
#include <asm/signal.h>
#include <asm/unistd.h>

#include "entry.h"
#include <asm/native/inst.h>

/*
 * See Documentation/ia64/fsys.txt for details on fsyscalls.
 *
 * On entry to an fsyscall handler:
 *   r10	= 0 (i.e., defaults to "successful syscall return")
 *   r11	= saved ar.pfs (a user-level value)
 *   r15	= system call number
 *   r16	= "current" task pointer (in normal kernel-mode, this is in r13)
 *   r32-r39	= system call arguments
 *   b6		= return address (a user-level value)
 *   ar.pfs	= previous frame-state (a user-level value)
 *   PSR.be	= cleared to zero (i.e., little-endian byte order is in effect)
 *   all other registers may contain values passed in from user-mode
 *
 * On return from an fsyscall handler:
 *   r11	= saved ar.pfs (as passed into the fsyscall handler)
 *   r15	= system call number (as passed into the fsyscall handler)
 *   r32-r39	= system call arguments (as passed into the fsyscall handler)
 *   b6		= return address (as passed into the fsyscall handler)
 *   ar.pfs	= previous frame-state (as passed into the fsyscall handler)
 */

ENTRY(fsys_ni_syscall)
	.prologue
	.altrp b6
	.body
	mov r8=ENOSYS
	mov r10=-1
	FSYS_RETURN
END(fsys_ni_syscall)

ENTRY(fsys_getpid)
	.prologue
	.altrp b6
	.body
	add r17=IA64_TASK_GROUP_LEADER_OFFSET,r16
	;;
	ld8 r17=[r17]				// r17 = current->group_leader
	add r9=TI_FLAGS+IA64_TASK_SIZE,r16
	;;
	ld4 r9=[r9]
	add r17=IA64_TASK_TGIDLINK_OFFSET,r17
	;;
	and r9=TIF_ALLWORK_MASK,r9
	ld8 r17=[r17]				// r17 = current->group_leader->pids[PIDTYPE_PID].pid
	;;
	add r8=IA64_PID_LEVEL_OFFSET,r17
	;;
	ld4 r8=[r8]				// r8 = pid->level
	add r17=IA64_PID_UPID_OFFSET,r17	// r17 = &pid->numbers[0]
	;;
	shl r8=r8,IA64_UPID_SHIFT
	;;
	add r17=r17,r8				// r17 = &pid->numbers[pid->level]
	;;
	ld4 r8=[r17]				// r8 = pid->numbers[pid->level].nr
	;;
	mov r17=0
	;;
	cmp.ne p8,p0=0,r9
(p8)	br.spnt.many fsys_fallback_syscall
	FSYS_RETURN
END(fsys_getpid)

ENTRY(fsys_set_tid_address)
	.prologue
	.altrp b6
	.body
	add r9=TI_FLAGS+IA64_TASK_SIZE,r16
	add r17=IA64_TASK_TGIDLINK_OFFSET,r16
	;;
	ld4 r9=[r9]
	tnat.z p6,p7=r32		// check argument register for being NaT
	ld8 r17=[r17]				// r17 = current->pids[PIDTYPE_PID].pid
	;;
	and r9=TIF_ALLWORK_MASK,r9
	add r8=IA64_PID_LEVEL_OFFSET,r17
	add r18=IA64_TASK_CLEAR_CHILD_TID_OFFSET,r16
	;;
	ld4 r8=[r8]				// r8 = pid->level
	add r17=IA64_PID_UPID_OFFSET,r17	// r17 = &pid->numbers[0]
	;;
	shl r8=r8,IA64_UPID_SHIFT
	;;
	add r17=r17,r8				// r17 = &pid->numbers[pid->level]
	;;
	ld4 r8=[r17]				// r8 = pid->numbers[pid->level].nr
	;;
	cmp.ne p8,p0=0,r9
	mov r17=-1
	;;
(p6)	st8 [r18]=r32
(p7)	st8 [r18]=r17
(p8)	br.spnt.many fsys_fallback_syscall
	;;
	mov r17=0			// i must not leak kernel bits...
	mov r18=0			// i must not leak kernel bits...
	FSYS_RETURN
END(fsys_set_tid_address)

#if IA64_GTOD_SEQ_OFFSET !=0
#error fsys_gettimeofday incompatible with changes to struct fsyscall_gtod_data_t
#endif
#if IA64_ITC_JITTER_OFFSET !=0
#error fsys_gettimeofday incompatible with changes to struct itc_jitter_data_t
#endif
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_DIVIDE_BY_1000 0x4000
#define CLOCK_ADD_MONOTONIC 0x8000

ENTRY(fsys_gettimeofday)
	.prologue
	.altrp b6
	.body
	mov r31 = r32
	tnat.nz p6,p0 = r33		// guard against NaT argument
(p6)    br.cond.spnt.few .fail_einval
	mov r30 = CLOCK_DIVIDE_BY_1000
	;;
.gettime:
	// Register map
	// Incoming r31 = pointer to address where to place result
	//          r30 = flags determining how time is processed
	// r2,r3 = temp r4-r7 preserved
	// r8 = result nanoseconds
	// r9 = result seconds
	// r10 = temporary storage for clock difference
	// r11 = preserved: saved ar.pfs
	// r12 = preserved: memory stack
	// r13 = preserved: thread pointer
	// r14 = address of mask / mask value
	// r15 = preserved: system call number
	// r16 = preserved: current task pointer
	// r17 = (not used)
	// r18 = (not used)
	// r19 = address of itc_lastcycle
	// r20 = struct fsyscall_gtod_data (= address of gtod_lock.sequence)
	// r21 = address of mmio_ptr
	// r22 = address of wall_time or monotonic_time
	// r23 = address of shift / value
	// r24 = address mult factor / cycle_last value
	// r25 = itc_lastcycle value
	// r26 = address clocksource cycle_last
	// r27 = (not used)
	// r28 = sequence number at the beginning of critcal section
	// r29 = address of itc_jitter
	// r30 = time processing flags / memory address
	// r31 = pointer to result
	// Predicates
	// p6,p7 short term use
	// p8 = timesource ar.itc
	// p9 = timesource mmio64
	// p10 = timesource mmio32 - not used
	// p11 = timesource not to be handled by asm code
	// p12 = memory time source ( = p9 | p10) - not used
	// p13 = do cmpxchg with itc_lastcycle
	// p14 = Divide by 1000
	// p15 = Add monotonic
	//
	// Note that instructions are optimized for McKinley. McKinley can
	// process two bundles simultaneously and therefore we continuously
	// try to feed the CPU two bundles and then a stop.

	add r2 = TI_FLAGS+IA64_TASK_SIZE,r16
	tnat.nz p6,p0 = r31		// guard against Nat argument
(p6)	br.cond.spnt.few .fail_einval
	movl r20 = fsyscall_gtod_data // load fsyscall gettimeofday data address
	;;
	ld4 r2 = [r2]			// process work pending flags
	movl r29 = itc_jitter_data	// itc_jitter
	add r22 = IA64_GTOD_WALL_TIME_OFFSET,r20	// wall_time
	add r21 = IA64_CLKSRC_MMIO_OFFSET,r20
	mov pr = r30,0xc000	// Set predicates according to function
	;;
	and r2 = TIF_ALLWORK_MASK,r2
	add r19 = IA64_ITC_LASTCYCLE_OFFSET,r29
(p15)	add r22 = IA64_GTOD_MONO_TIME_OFFSET,r20	// monotonic_time
	;;
	add r26 = IA64_CLKSRC_CYCLE_LAST_OFFSET,r20	// clksrc_cycle_last
	cmp.ne p6, p0 = 0, r2	// Fallback if work is scheduled
(p6)	br.cond.spnt.many fsys_fallback_syscall
	;;
	// Begin critical section
.time_redo:
	ld4.acq r28 = [r20]	// gtod_lock.sequence, Must take first
	;;
	and r28 = ~1,r28	// And make sequence even to force retry if odd
	;;
	ld8 r30 = [r21]		// clocksource->mmio_ptr
	add r24 = IA64_CLKSRC_MULT_OFFSET,r20
	ld4 r2 = [r29]		// itc_jitter value
	add r23 = IA64_CLKSRC_SHIFT_OFFSET,r20
	add r14 = IA64_CLKSRC_MASK_OFFSET,r20
	;;
	ld4 r3 = [r24]		// clocksource mult value
	ld8 r14 = [r14]         // clocksource mask value
	cmp.eq p8,p9 = 0,r30	// use cpu timer if no mmio_ptr
	;;
	setf.sig f7 = r3	// Setup for mult scaling of counter
(p8)	cmp.ne p13,p0 = r2,r0	// need itc_jitter compensation, set p13
	ld4 r23 = [r23]		// clocksource shift value
	ld8 r24 = [r26]		// get clksrc_cycle_last value
(p9)	cmp.eq p13,p0 = 0,r30	// if mmio_ptr, clear p13 jitter control
	;;
	.pred.rel.mutex p8,p9
	MOV_FROM_ITC(p8, p6, r2, r10)	// CPU_TIMER. 36 clocks latency!!!
(p9)	ld8 r2 = [r30]		// MMIO_TIMER. Could also have latency issues..
(p13)	ld8 r25 = [r19]		// get itc_lastcycle value
	ld8 r9 = [r22],IA64_TIMESPEC_TV_NSEC_OFFSET	// tv_sec
	;;
	ld8 r8 = [r22],-IA64_TIMESPEC_TV_NSEC_OFFSET	// tv_nsec
(p13)	sub r3 = r25,r2		// Diff needed before comparison (thanks davidm)
	;;
(p13)	cmp.gt.unc p6,p7 = r3,r0 // check if it is less than last. p6,p7 cleared
	sub r10 = r2,r24	// current_cycle - last_cycle
	;;
(p6)	sub r10 = r25,r24	// time we got was less than last_cycle
(p7)	mov ar.ccv = r25	// more than last_cycle. Prep for cmpxchg
	;;
(p7)	cmpxchg8.rel r3 = [r19],r2,ar.ccv
	;;
(p7)	cmp.ne p7,p0 = r25,r3	// if cmpxchg not successful
	;;
(p7)	sub r10 = r3,r24	// then use new last_cycle instead
	;;
	and r10 = r10,r14	// Apply mask
	;;
	setf.sig f8 = r10
	nop.i 123
	;;
	// fault check takes 5 cycles and we have spare time
EX(.fail_efault, probe.w.fault r31, 3)
	xmpy.l f8 = f8,f7	// nsec_per_cyc*(counter-last_counter)
	;;
	getf.sig r2 = f8
	mf
	;;
	ld4 r10 = [r20]		// gtod_lock.sequence
	shr.u r2 = r2,r23	// shift by factor
	;;
	add r8 = r8,r2		// Add xtime.nsecs
	cmp4.ne p7,p0 = r28,r10
(p7)	br.cond.dpnt.few .time_redo	// sequence number changed, redo
	// End critical section.
	// Now r8=tv->tv_nsec and r9=tv->tv_sec
	mov r10 = r0
	movl r2 = 1000000000
	add r23 = IA64_TIMESPEC_TV_NSEC_OFFSET, r31
(p14)	movl r3 = 2361183241434822607	// Prep for / 1000 hack
	;;
.time_normalize:
	mov r21 = r8
	cmp.ge p6,p0 = r8,r2
(p14)	shr.u r20 = r8, 3 // We can repeat this if necessary just wasting time
	;;
(p14)	setf.sig f8 = r20
(p6)	sub r8 = r8,r2
(p6)	add r9 = 1,r9		// two nops before the branch.
(p14)	setf.sig f7 = r3	// Chances for repeats are 1 in 10000 for gettod
(p6)	br.cond.dpnt.few .time_normalize
	;;
	// Divided by 8 though shift. Now divide by 125
	// The compiler was able to do that with a multiply
	// and a shift and we do the same
EX(.fail_efault, probe.w.fault r23, 3)	// This also costs 5 cycles
(p14)	xmpy.hu f8 = f8, f7		// xmpy has 5 cycles latency so use it
	;;
(p14)	getf.sig r2 = f8
	;;
	mov r8 = r0
(p14)	shr.u r21 = r2, 4
	;;
EX(.fail_efault, st8 [r31] = r9)
EX(.fail_efault, st8 [r23] = r21)
	FSYS_RETURN
.fail_einval:
	mov r8 = EINVAL
	mov r10 = -1
	FSYS_RETURN
.fail_efault:
	mov r8 = EFAULT
	mov r10 = -1
	FSYS_RETURN
END(fsys_gettimeofday)

ENTRY(fsys_clock_gettime)
	.prologue
	.altrp b6
	.body
	cmp4.ltu p6, p0 = CLOCK_MONOTONIC, r32
	// Fallback if this is not CLOCK_REALTIME or CLOCK_MONOTONIC
(p6)	br.spnt.few fsys_fallback_syscall
	mov r31 = r33
	shl r30 = r32,15
	br.many .gettime
END(fsys_clock_gettime)

/*
 * fsys_getcpu doesn't use the third parameter in this implementation. It reads
 * current_thread_info()->cpu and corresponding node in cpu_to_node_map.
 */
ENTRY(fsys_getcpu)
	.prologue
	.altrp b6
	.body
	;;
	add r2=TI_FLAGS+IA64_TASK_SIZE,r16
	tnat.nz p6,p0 = r32			// guard against NaT argument
	add r3=TI_CPU+IA64_TASK_SIZE,r16
	;;
	ld4 r3=[r3]				// M r3 = thread_info->cpu
	ld4 r2=[r2]				// M r2 = thread_info->flags
(p6)    br.cond.spnt.few .fail_einval		// B
	;;
	tnat.nz p7,p0 = r33			// I guard against NaT argument
(p7)    br.cond.spnt.few .fail_einval		// B
	;;
	cmp.ne p6,p0=r32,r0
	cmp.ne p7,p0=r33,r0
	;;
#ifdef CONFIG_NUMA
	movl r17=cpu_to_node_map
	;;
EX(.fail_efault, (p6) probe.w.fault r32, 3)		// M This takes 5 cycles
EX(.fail_efault, (p7) probe.w.fault r33, 3)		// M This takes 5 cycles
	shladd r18=r3,1,r17
	;;
	ld2 r20=[r18]				// r20 = cpu_to_node_map[cpu]
	and r2 = TIF_ALLWORK_MASK,r2
	;;
	cmp.ne p8,p0=0,r2
(p8)	br.spnt.many fsys_fallback_syscall
	;;
	;;
EX(.fail_efault, (p6) st4 [r32] = r3)
EX(.fail_efault, (p7) st2 [r33] = r20)
	mov r8=0
	;;
#else
EX(.fail_efault, (p6) probe.w.fault r32, 3)		// M This takes 5 cycles
EX(.fail_efault, (p7) probe.w.fault r33, 3)		// M This takes 5 cycles
	and r2 = TIF_ALLWORK_MASK,r2
	;;
	cmp.ne p8,p0=0,r2
(p8)	br.spnt.many fsys_fallback_syscall
	;;
EX(.fail_efault, (p6) st4 [r32] = r3)
EX(.fail_efault, (p7) st2 [r33] = r0)
	mov r8=0
	;;
#endif
	FSYS_RETURN
END(fsys_getcpu)

ENTRY(fsys_fallback_syscall)
	.prologue
	.altrp b6
	.body
	/*
	 * We only get here from light-weight syscall handlers.  Thus, we already
	 * know that r15 contains a valid syscall number.  No need to re-check.
	 */
	adds r17=-1024,r15
	movl r14=sys_call_table
	;;
	RSM_PSR_I(p0, r26, r27)
	shladd r18=r17,3,r14
	;;
	ld8 r18=[r18]				// load normal (heavy-weight) syscall entry-point
	MOV_FROM_PSR(p0, r29, r26)		// read psr (12 cyc load latency)
	mov r27=ar.rsc
	mov r21=ar.fpsr
	mov r26=ar.pfs
END(fsys_fallback_syscall)
	/* FALL THROUGH */
GLOBAL_ENTRY(fsys_bubble_down)
	.prologue
	.altrp b6
	.body
	/*
	 * We get here for syscalls that don't have a lightweight
	 * handler.  For those, we need to bubble down into the kernel
	 * and that requires setting up a minimal pt_regs structure,
	 * and initializing the CPU state more or less as if an
	 * interruption had occurred.  To make syscall-restarts work,
	 * we setup pt_regs such that cr_iip points to the second
	 * instruction in syscall_via_break.  Decrementing the IP
	 * hence will restart the syscall via break and not
	 * decrementing IP will return us to the caller, as usual.
	 * Note that we preserve the value of psr.pp rather than
	 * initializing it from dcr.pp.  This makes it possible to
	 * distinguish fsyscall execution from other privileged
	 * execution.
	 *
	 * On entry:
	 *	- normal fsyscall handler register usage, except
	 *	  that we also have:
	 *	- r18: address of syscall entry point
	 *	- r21: ar.fpsr
	 *	- r26: ar.pfs
	 *	- r27: ar.rsc
	 *	- r29: psr
	 *
	 * We used to clear some PSR bits here but that requires slow
	 * serialization.  Fortuntely, that isn't really necessary.
	 * 