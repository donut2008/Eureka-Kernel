_prepare_handle_unaligned)
	.prologue
	/*
	 * r16 = fake ar.pfs, we simply need to make sure privilege is still 0
	 */
	mov r16=r0
	DO_SAVE_SWITCH_STACK
	br.call.sptk.many rp=ia64_handle_unaligned	// stack frame setup in ivt
.ret21:	.body
	DO_LOAD_SWITCH_STACK
	br.cond.sptk.many rp				// goes to ia64_leave_kernel
END(ia64_prepare_handle_unaligned)

	//
	// unw_init_running(void (*callback)(info, arg), void *arg)
	//
#	define EXTRA_FRAME_SIZE	((UNW_FRAME_INFO_SIZE+15)&~15)

GLOBAL_ENTRY(unw_init_running)
	.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(2)
	alloc loc1=ar.pfs,2,3,3,0
	;;
	ld8 loc2=[in0],8
	mov loc0=rp
	mov r16=loc1
	DO_SAVE_SWITCH_STACK
	.body

	.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(2)
	.fframe IA64_SWITCH_STACK_SIZE+EXTRA_FRAME_SIZE
	SWITCH_STACK_SAVES(EXTRA_FRAME_SIZE)
	adds sp=-EXTRA_FRAME_SIZE,sp
	.body
	;;
	adds out0=16,sp				// &info
	mov out1=r13				// current
	adds out2=16+EXTRA_FRAME_SIZE,sp	// &switch_stack
	br.call.sptk.many rp=unw_init_frame_info
1:	adds out0=16,sp				// &info
	mov b6=loc2
	mov loc2=gp				// save gp across indirect function call
	;;
	ld8 gp=[in0]
	mov out1=in1				// arg
	br.call.sptk.many rp=b6			// invoke the callback function
1:	mov gp=loc2				// restore gp

	// For now, we don't allow changing registers from within
	// unw_init_running; if we ever want to allow that, we'd
	// have to do a load_switch_stack here:
	.restore sp
	adds sp=IA64_SWITCH_STACK_SIZE+EXTRA_FRAME_SIZE,sp

	mov ar.pfs=loc1
	mov rp=loc0
	br.ret.sptk.many rp
END(unw_init_running)

#ifdef CONFIG_FUNCTION_TRACER
#ifdef CONFIG_DYNAMIC_FTRACE
GLOBAL_ENTRY(_mcount)
	br ftrace_stub
END(_mcount)

.here:
	br.ret.sptk.many b0

GLOBAL_ENTRY(ftrace_caller)
	alloc out0 = ar.pfs, 8, 0, 4, 0
	mov out3 = r0
	;;
	mov out2 = b0
	add r3 = 0x20, r3
	mov out1 = r1;
	br.call.sptk.many b0 = ftrace_patch_gp
	//this might be called from module, so we must patch gp
ftrace_patch_gp:
	movl gp=__gp
	mov b0 = r3
	;;
.global ftrace_call;
ftrace_call:
{
	.mlx
	nop.m 0x0
	movl r3 = .here;;
}
	alloc loc0 = ar.pfs, 4, 4, 2, 0
	;;
	mov loc1 = b0
	mov out0 = b0
	mov loc2 = r8
	mov loc3 = r15
	;;
	adds out0 = -MCOUNT_INSN_SIZE, out0
	mov out1 = in2
	mov b6 = r3

	br.call.sptk.many b0 = b6
	;;
	mov ar.pfs = loc0
	mov b0 = loc1
	mov r8 = loc2
	mov r15 = loc3
	br ftrace_stub
	;;
END(ftrace_caller)

#else
GLOBAL_ENTRY(_mcount)
	movl r2 = ftrace_stub
	movl r3 = ftrace_trace_function;;
	ld8 r3 = [r3];;
	ld8 r3 = [r3];;
	cmp.eq p7,p0 = r2, r3
(p7)	br.sptk.many ftrace_stub
	;;

	alloc loc0 = ar.pfs, 4, 4, 2, 0
	;;
	mov loc1 = b0
	mov out0 = b0
	mov loc2 = r8
	mov loc3 = r15
	;;
	adds out0 = -MCOUNT_INSN_SIZE, out0
	mov out1 = in2
	mov b6 = r3

	br.call.sptk.many b0 = b6
	;;
	mov ar.pfs = loc0
	mov b0 = loc1
	mov r8 = loc2
	mov r15 = loc3
	br ftrace_stub
	;;
END(_mcount)
#endif

GLOBAL_ENTRY(ftrace_stub)
	mov r3 = b0
	movl r2 = _mcount_ret_helper
	;;
	mov b6 = r2
	mov b7 = r3
	br.ret.sptk.many b6

_mcount_ret_helper:
	mov b0 = r42
	mov r1 = r41
	mov ar.pfs = r40
	br b7
END(ftrace_stub)

#endif /* CONFIG_FUNCTION_TRACER */

	.rodata
	.align 8
	.globl sys_call_table
sys_call_table:
	data8 sys_ni_syscall		//  This must be sys_ni_syscall!  See ivt.S.
	data8 sys_exit				// 1025
	data8 sys_read
	data8 sys_write
	data8 sys_open
	data8 sys_close
	data8 sys_creat				// 1030
	data8 sys_link
	data8 sys_unlink
	data8 ia64_execve
	data8 sys_chdir
	data8 sys_fchdir			// 1035
	data8 sys_utimes
	data8 sys_mknod
	data8 sys_chmod
	data8 sys_chown
	data8 sys_lseek				// 1040
	data8 sys_getpid
	data8 sys_getppid
	data8 sys_mount
	data8 sys_umount
	data8 sys_setuid			// 1045
	data8 sys_getuid
	data8 sys_geteuid
	data8 sys_ptrace
	data8 sys_access
	data8 sys_sync				// 1050
	data8 sys_fsync
	data8 sys_fdatasync
	data8 sys_kill
	data8 sys_rename
	data8 sys_mkdir				// 1055
	data8 sys_rmdir
	data8 sys_dup
	data8 sys_ia64_pipe
	data8 sys_times
	data8 ia64_brk				// 1060
	data8 sys_setgid
	data8 sys_getgid
	data8 sys_getegid
	data8 sys_acct
	data8 sys_ioctl				// 1065
	data8 sys_fcntl
	data8 sys_umask
	data8 sys_chroot
	data8 sys_ustat
	data8 sys_dup2				// 1070
	data8 sys_setreuid
	data8 sys_setregid
	data8 sys_getresuid
	data8 sys_setresuid
	data8 sys_getresgid			// 1075
	data8 sys_setresgid
	data8 sys_getgroups
	data8 sys_setgroups
	data8 sys_getpgid
	data8 sys_setpgid			// 1080
	data8 sys_setsid
	data8 sys_getsid
	data8 sys_sethostname
	data8 sys_setrlimit
	data8 sys_getrlimit			// 1085
	data8 sys_getrusage
	data8 sys_gettimeofday
	data8 sys_settimeofday
	data8 sys_select
	data8 sys_poll				// 1090
	data8 sys_symlink
	data8 sys_readlink
	data8 sys_uselib
	data8 sys_swapon
	data8 sys_swapoff			// 1095
	data8 sys_reboot
	data8 sys_truncate
	data8 sys_ftruncate
	data8 sys_fchmod
	data8 sys_fchown			// 1100
	data8 ia64_getpriority
	data8 sys_setpriority
	data8 sys_statfs
	data8 sys_fstatfs
	data8 sys_gettid			// 1105
	data8 sys_semget
	data8 sys_semop
	data8 sys_semctl
	data8 sys_msgget
	data8 sys_msgsnd			// 1110
	data8 sys_msgrcv
	data8 sys_msgctl
	data8 sys_shmget
	data8 sys_shmat
	data8 sys_shmdt				// 1115
	data8 sys_shmctl
	data8 sys_syslog
	data8 sys_setitimer
	data8 sys_getitimer
	data8 sys_ni_syscall			// 1120		/* was: ia64_oldstat */
	data8 sys_ni_syscall					/* was: ia64_oldlstat */
	data8 sys_ni_syscall					/* was: ia64_oldfstat */
	data8 sys_vhangup
	data8 sys_lchown
	data8 sys_remap_file_pages		// 1125
	data8 sys_wait4
	data8 sys_sysinfo
	data8 sys_clone
	data8 sys_setdomainname
	data8 sys_newuname			// 1130
	data8 sys_adjtimex
	data8 sys_ni_syscall					/* was: ia64_create_module */
	data8 sys_init_module
	data8 sys_delete_module
	data8 sys_ni_syscall			// 1135		/* was: sys_get_kernel_syms */
	data8 sys_ni_syscall					/* was: sys_query_module */
	data8 sys_quotactl
	data8 sys_bdflush
	data8 sys_sysfs
	data8 sys_personality			// 1140
	data8 sys_ni_syscall		// sys_afs_syscall
	data8 sys_setfsuid
	data8 sys_setfsgid
	data8 sys_getdents
	data8 sys_flock				// 1145
	data8 sys_readv
	data8 sys_writev
	data8 sys_pread64
	data8 sys_pwrite64
	data8 sys_sysctl			// 1150
	data8 sys_mmap
	data8 sys_munmap
	data8 sys_mlock
	data8 sys_mlockall
	data8 sys_mprotect			// 1155
	data8 ia64_mremap
	data8 sys_msync
	data8 sys_munlock
	data8 sys_munlockall
	data8 sys_sched_getparam		// 1160
	data8 sys_sched_setparam
	data8 sys_sched_getscheduler
	data8 sys_sched_setscheduler
	data8 sys_sched_yield
	data8 sys_sched_get_priority_max	// 1165
	data8 sys_sched_get_priority_min
	data8 sys_sched_rr_get_interval
	data8 sys_nanosleep
	data8 sys_ni_syscall			// old nfsservctl
	data8 sys_prctl				// 1170
	data8 sys_getpagesize
	data8 sys_mmap2
	data8 sys_pciconfig_read
	data8 sys_pciconfig_write
	data8 sys_perfmonctl			// 1175
	data8 sys_sigaltstack
	data8 sys_rt_sigaction
	data8 sys_rt_sigpending
	data8 sys_rt_sigprocmask
	data8 sys_rt_sigqueueinfo		// 1180
	data8 sys_rt_sigreturn
	data8 sys_rt_sigsuspend
	data8 sys_rt_sigtimedwait
	data8 sys_getcwd
	data8 sys_capget			// 1185
	data8 sys_capset
	data8 sys_sendfile64
	data8 sys_ni_syscall		// sys_getpmsg (STREAMS)
	data8 sys_ni_syscall		// sys_putpmsg (STREAMS)
	data8 sys_socket			// 1190
	data8 sys_bind
	data8 sys_connect
	data8 sys_listen
	data8 sys_accept
	data8 sys_getsockname			// 1195
	data8 sys_getpeername
	data8 sys_socketpair
	data8 sys_send
	data8 sys_sendto
	data8 sys_recv				// 1200
	data8 sys_recvfrom
	data8 sys_shutdown
	data8 sys_setsockopt
	data8 sys_getsockopt
	data8 sys_sendmsg			// 1205
	data8 sys_recvmsg
	data8 sys_pivot_root
	data8 sys_mincore
	data8 sys_madvise
	data8 sys_newstat			// 1210
	data8 sys_newlstat
	data8 sys_newfstat
	data8 sys_clone2
	data8 sys_getdents64
	data8 sys_getunwind			// 1215
	data8 sys_readahead
	data8 sys_setxattr
	data8 sys_lsetxattr
	data8 sys_fsetxattr
	data8 sys_getxattr			// 1220
	data8 sys_lgetxattr
	data8 sys_fgetxattr
	data8 sys_listxattr
	data8 sys_llistxattr
	data8 sys_flistxattr			// 1225
	data8 sys_removexattr
	data8 sys_lremovexattr
	data8 sys_fremovexattr
	data8 sys_tkill
	data8 sys_futex				// 1230
	data8 sys_sched_setaffinity
	data8 sys_sched_getaffinity
	data8 sys_set_tid_address
	data8 sys_fadvise64_64
	data8 sys_tgkill 			// 1235
	data8 sys_exit_group
	data8 sys_lookup_dcookie
	data8 sys_io_setup
	data8 sys_io_destroy
	data8 sys_io_getevents			// 1240
	data8 sys_io_submit
	data8 sys_io_cancel
	data8 sys_epoll_create
	data8 sys_epoll_ctl
	data8 sys_epoll_wait			// 1245
	data8 sys_restart_syscall
	data8 sys_semtimedop
	data8 sys_timer_create
	data8 sys_timer_settime
	data8 sys_timer_gettime			// 1250
	data8 sys_timer_getoverrun
	data8 sys_timer_delete
	data8 sys_clock_settime
	data8 sys_clock_gettime
	data8 sys_clock_getres			// 1255
	data8 sys_clock_nanosleep
	data8 sys_fstatfs64
	data8 sys_statfs64
	data8 sys_mbind
	data8 sys_get_mempolicy			// 1260
	data8 sys_set_mempolicy
	data8 sys_mq_open
	data8 sys_mq_unlink
	data8 sys_mq_timedsend
	data8 sys_mq_timedreceive		// 1265
	data8 sys_mq_notify
	data8 sys_mq_getsetattr
	data8 sys_kexec_load
	data8 sys_ni_syscall			// reserved for vserver
	data8 sys_waitid			// 1270
	data8 sys_add_key
	data8 sys_request_key
	data8 sys_keyctl
	data8 sys_ioprio_set
	data8 sys_ioprio_get			// 1275
	data8 sys_move_pages
	data8 sys_inotify_init
	data8 sys_inotify_add_watch
	data8 sys_inotify_rm_watch
	data8 sys_migrate_pages			// 1280
	data8 sys_openat
	data8 sys_mkdirat
	data8 sys_mknodat
	data8 sys_fchownat
	data8 sys_futimesat			// 1285
	data8 sys_newfstatat
	data8 sys_unlinkat
	data8 sys_renameat
	data8 sys_linkat
	data8 sys_symlinkat			// 1290
	data8 sys_readlinkat
	data8 sys_fchmodat
	data8 sys_faccessat
	data8 sys_pselect6
	data8 sys_ppoll				// 1295
	data8 sys_unshare
	data8 sys_splice
	data8 sys_set_robust_list
	data8 sys_get_robust_list
	data8 sys_sync_file_range		// 1300
	data8 sys_tee
	data8 sys_vmsplice
	data8 sys_fallocate
	data8 sys_getcpu
	data8 sys_epoll_pwait			// 1305
	data8 sys_utimensat
	data8 sys_signalfd
	data8 sys_ni_syscall
	data8 sys_eventfd
	data8 sys_timerfd_create		// 1310
	data8 sys_timerfd_settime
	data8 sys_timerfd_gettime
	data8 sys_signalfd4
	data8 sys_eventfd2
	data8 sys_epoll_create1			// 1315
	data8 sys_dup3
	data8 sys_pipe2
	data8 sys_inotify_init1
	data8 sys_preadv
	data8 sys_pwritev			// 1320
	data8 sys_rt_tgsigqueueinfo
	data8 sys_recvmmsg
	data8 sys_fanotify_init
	data8 sys_fanotify_mark
	data8 sys_prlimit64			// 1325
	data8 sys_name_to_handle_at
	data8 sys_open_by_handle_at
	data8 sys_clock_adjtime
	data8 sys_syncfs
	data8 sys_setns				// 1330
	data8 sys_sendmmsg
	data8 sys_process_vm_readv
	data8 sys_process_vm_writev
	data8 sys_accept4
	data8 sys_finit_module			// 1335
	data8 sys_sched_setattr
	data8 sys_sched_getattr
	data8 sys_renameat2
	data8 sys_getrandom
	data8 sys_memfd_create			// 1340
	data8 sys_bpf
	data8 sys_execveat
	data8 sys_userfaultfd
	data8 sys_membarrier
	data8 sys_kcmp				// 1345
	data8 sys_mlock2

	.org sys_call_table + 8*NR_syscalls	// guard against failures to increase NR_syscalls
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
/*
 * Preserved registers that are shared between code in ivt.S and
 * entry.S.  Be careful not to step on these!
 */
#define PRED_LEAVE_SYSCALL	1 /* TRUE iff leave from syscall */
#define PRED_KERNEL_STACK	2 /* returning to kernel-stacks? */
#define PRED_USER_STACK		3 /* returning to user-stacks? */
#define PRED_SYSCALL		4 /* inside a system call? */
#define PRED_NON_SYSCALL	5 /* complement of PRED_SYSCALL */

#ifdef __ASSEMBLY__
# define PASTE2(x,y)	x##y
# define PASTE(x,y)	PASTE2(x,y)

# define pLvSys		PASTE(p,PRED_LEAVE_SYSCALL)
# define pKStk		PASTE(p,PRED_KERNEL_STACK)
# define pUStk		PASTE(p,PRED_USER_STACK)
# define pSys		PASTE(p,PRED_SYSCALL)
# define pNonSys	PASTE(p,PRED_NON_SYSCALL)
#endif

#define PT(f)		(IA64_PT_REGS_##f##_OFFSET)
#define SW(f)		(IA64_SWITCH_STACK_##f##_OFFSET)
#define SOS(f)		(IA64_SAL_OS_STATE_##f##_OFFSET)

#define PT_REGS_SAVES(off)			\
	.unwabi 3, 'i';				\
	.fframe IA64_PT_REGS_SIZE+16+(off);	\
	.spillsp rp, PT(CR_IIP)+16+(off);	\
	.spillsp ar.pfs, PT(CR_IFS)+16+(off);	\
	.spillsp ar.unat, PT(AR_UNAT)+16+(off);	\
	.spillsp ar.fpsr, PT(AR_FPSR)+16+(off);	\
	.spillsp pr, PT(PR)+16+(off);

#define PT_REGS_UNWIND_INFO(off)		\
	.prologue;				\
	PT_REGS_SAVES(off);			\
	.body

#define SWITCH_STACK_SAVES(off)							\
	.savesp ar.unat,SW(CALLER_UNAT)+16+(off);				\
	.savesp ar.fpsr,SW(AR_FPSR)+16+(off);					\
	.spillsp f2,SW(F2)+16+(off); .spillsp f3,SW(F3)+16+(off);		\
	.spillsp f4,SW(F4)+16+(off); .spillsp f5,SW(F5)+16+(off);		\
	.spillsp f16,SW(F16)+16+(off); .spillsp f17,SW(F17)+16+(off);		\
	.spillsp f18,SW(F18)+16+(off); .spillsp f19,SW(F19)+16+(off);		\
	.spillsp f20,SW(F20)+16+(off); .spillsp f21,SW(F21)+16+(off);		\
	.spillsp f22,SW(F22)+16+(off); .spillsp f23,SW(F23)+16+(off);		\
	.spillsp f24,SW(F24)+16+(off); .spillsp f25,SW(F25)+16+(off);		\
	.spillsp f26,SW(F26)+16+(off); .spillsp f27,SW(F27)+16+(off);		\
	.spillsp f28,SW(F28)+16+(off); .spillsp f29,SW(F29)+16+(off);		\
	.spillsp f30,SW(F30)+16+(off); .spillsp f31,SW(F31)+16+(off);		\
	.spillsp r4,SW(R4)+16+(off); .spillsp r5,SW(R5)+16+(off);		\
	.spillsp r6,SW(R6)+16+(off); .spillsp r7,SW(R7)+16+(off);		\
	.spillsp b0,SW(B0)+16+(off); .spillsp b1,SW(B1)+16+(off);		\
	.spillsp b2,SW(B2)+16+(off); .spillsp b3,SW(B3)+16+(off);		\
	.spillsp b4,SW(B4)+16+(off); .spillsp b5,SW(B5)+16+(off);		\
	.spillsp ar.pfs,SW(AR_PFS)+16+(off); .spillsp ar.lc,SW(AR_LC)+16+(off);	\
	.spillsp @priunat,SW(AR_UNAT)+16+(off);					\
	.spillsp ar.rnat,SW(AR_RNAT)+16+(off);					\
	.spillsp ar.bspstore,SW(AR_BSPSTORE)+16+(off);				\
	.spillsp pr,SW(PR)+16+(off)

#define DO_SAVE_SWITCH_STACK			\
	movl r28=1f;				\
	;;					\
	.fframe IA64_SWITCH_STACK_SIZE;		\
	adds sp=-IA64_SWITCH_STACK_SIZE,sp;	\
	mov.ret.sptk b7=r28,1f;			\
	SWITCH_STACK_SAVES(0);			\
	br.cond.sptk.many save_switch_stack;	\
1:

#define DO_LOAD_SWITCH_STACK			\
	movl r28=1f;				\
	;;					\
	invala;					\
	mov.ret.sptk b7=r28,1f;			\
	br.cond.sptk.many load_switch_stack;	\
1:	.restore sp;				\
	adds sp=IA64_SWITCH_STACK_SIZE,sp
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * err_inject.c -
 *	1.) Inject errors to a processor.
 *	2.) Query error injection capabilities.
 * This driver along with user space code can be acting as an error
 * injection tool.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Written by: Fenghua Yu <fenghua.yu@intel.com>, Intel Corporation
 * Copyright (C) 2006, Intel Corp.  All rights reserved.
 *
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/module.h>

#define ERR_INJ_DEBUG

#define ERR_DATA_BUFFER_SIZE 3 		// Three 8-byte;

#define define_one_ro(name) 						\
static DEVICE_ATTR(name, 0444, show_##name, NULL)

#define define_one_rw(name) 						\
static DEVICE_ATTR(name, 0644, show_##name, store_##name)

static u64 call_start[NR_CPUS];
static u64 phys_addr[NR_CPUS];
static u64 err_type_info[NR_CPUS];
static u64 err_struct_info[NR_CPUS];
static struct {
	u64 data1;
	u64 data2;
	u64 data3;
} __attribute__((__aligned__(16))) err_data_buffer[NR_CPUS];
static s64 status[NR_CPUS];
static u64 capabilities[NR_CPUS];
static u64 resources[NR_CPUS];

#define show(name) 							\
static ssize_t 								\
show_##name(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	u32 cpu=dev->id;						\
	return sprintf(buf, "%lx\n", name[cpu]);			\
}

#define store(name)							\
static ssize_t 								\
store_##name(struct device *dev, struct device_attribute *attr,	\
					const char *buf, size_t size)	\
{									\
	unsigned int cpu=dev->id;					\
	name[cpu] = simple_strtoull(buf, NULL, 16);			\
	return size;							\
}

show(call_start)

/* It's user's responsibility to call the PAL procedure on a specific
 * processor. The cpu number in driver is only used for storing data.
 */
static ssize_t
store_call_start(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	unsigned int cpu=dev->id;
	unsigned long call_start = simple_strtoull(buf, NULL, 16);

#ifdef ERR_INJ_DEBUG
	printk(KERN_DEBUG "pal_mc_err_inject for cpu%d:\n", cpu);
	printk(KERN_DEBUG "err_type_info=%lx,\n", err_type_info[cpu]);
	printk(KERN_DEBUG "err_struct_info=%lx,\n", err_struct_info[cpu]);
	printk(KERN_DEBUG "err_data_buffer=%lx, %lx, %lx.\n",
			  err_data_buffer[cpu].data1,
			  err_data_buffer[cpu].data2,
			  err_data_buffer[cpu].data3);
#endif
	switch (call_start) {
	    case 0: /* Do nothing. */
		break;
	    case 1: /* Call pal_mc_error_inject in physical mode. */
		status[cpu]=ia64_pal_mc_error_inject_phys(err_type_info[cpu],
					err_struct_info[cpu],
					ia64_tpa(&err_data_buffer[cpu]),
					&capabilities[cpu],
			 		&resources[cpu]);
		break;
	    case 2: /* Call pal_mc_error_inject in virtual mode. */
		status[cpu]=ia64_pal_mc_error_inject_virt(err_type_info[cpu],
					err_struct_info[cpu],
					ia64_tpa(&err_data_buffer[cpu]),
					&capabilities[cpu],
			 		&resources[cpu]);
		break;
	    default:
		status[cpu] = -EINVAL;
		break;
	}

#ifdef ERR_INJ_DEBUG
	printk(KERN_DEBUG "Returns: status=%d,\n", (int)status[cpu]);
	printk(KERN_DEBUG "capapbilities=%lx,\n", capabilities[cpu]);
	printk(KERN_DEBUG "resources=%lx\n", resources[cpu]);
#endif
	return size;
}

show(err_type_info)
store(err_type_info)

static ssize_t
show_virtual_to_phys(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	unsigned int cpu=dev->id;
	return sprintf(buf, "%lx\n", phys_addr[cpu]);
}

static ssize_t
store_virtual_to_phys(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t size)
{
	unsigned int cpu=dev->id;
	u64 virt_addr=simple_strtoull(buf, NULL, 16);
	int ret;

        ret = get_user_pages(current, current->mm, virt_addr,
			     1, FOLL_WRITE, NULL, NULL);
	if (ret<=0) {
#ifdef ERR_INJ_DEBUG
		printk("Virtual address %lx is not existing.\n",virt_addr);
#endif
		return -EINVAL;
	}

	phys_addr[cpu] = ia64_tpa(virt_addr);
	return size;
}

show(err_struct_info)
store(err_struct_info)

static ssize_t
show_err_data_buffer(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	unsigned int cpu=dev->id;

	return sprintf(buf, "%lx, %lx, %lx\n",
			err_data_buffer[cpu].data1,
			err_data_buffer[cpu].data2,
			err_data_buffer[cpu].data3);
}

static ssize_t
store_err_data_buffer(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	unsigned int cpu=dev->id;
	int ret;

#ifdef ERR_INJ_DEBUG
	printk("write err_data_buffer=[%lx,%lx,%lx] on cpu%d\n",
		 err_data_buffer[cpu].data1,
		 err_data_buffer[cpu].data2,
		 err_data_buffer[cpu].data3,
		 cpu);
#endif
	ret=sscanf(buf, "%lx, %lx, %lx",
			&err_data_buffer[cpu].data1,
			&err_data_buffer[cpu].data2,
			&err_data_buffer[cpu].data3);
	if (ret!=ERR_DATA_BUFFER_SIZE)
		return -EINVAL;

	return size;
}

show(status)
show(capabilities)
show(resources)

define_one_rw(call_start);
define_one_rw(err_type_info);
define_one_rw(err_struct_info);
define_one_rw(err_data_buffer);
define_one_rw(virtual_to_phys);
define_one_ro(status);
define_one_ro(capabilities);
define_one_ro(resources);

static struct attribute *default_attrs[] = {
	&dev_attr_call_start.attr,
	&dev_attr_virtual_to_phys.attr,
	&dev_attr_err_type_info.attr,
	&dev_attr_err_struct_info.attr,
	&dev_attr_err_data_buffer.attr,
	&dev_attr_status.attr,
	&dev_attr_capabilities.attr,
	&dev_attr_resources.attr,
	NULL
};

static struct attribute_group err_inject_attr_group = {
	.attrs = default_attrs,
	.name = "err_inject"
};
/* Add/Remove err_inject interface for CPU device */
static int err_inject_add_dev(struct device *sys_dev)
{
	return sysfs_create_group(&sys_dev->kobj, &err_inject_attr_group);
}

static int err_inject_remove_dev(struct device *sys_dev)
{
	sysfs_remove_group(&sys_dev->kobj, &err_inject_attr_group);
	return 0;
}
static int err_inject_cpu_callback(struct notifier_block *nfb,
		unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	struct device *sys_dev;

	sys_dev = get_cpu_device(cpu);
	switch (action) {
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		err_inject_add_dev(sys_dev);
		break;
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		err_inject_remove_dev(sys_dev);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block err_inject_cpu_notifier =
{
	.notifier_call = err_inject_cpu_callback,
};

static int __init
err_inject_init(void)
{
	int i;

#ifdef ERR_INJ_DEBUG
	printk(KERN_INFO "Enter error injection driver.\n");
#endif

	cpu_notifier_register_begin();

	for_each_online_cpu(i) {
		err_inject_cpu_callback(&err_inject_cpu_notifier, CPU_ONLINE,
				(void *)(long)i);
	}

	__register_hotcpu_notifier(&err_inject_cpu_notifier);

	cpu_notifier_register_done();

	return 0;
}

static void __exit
err_inject_exit(void)
{
	int i;
	struct device *sys_dev;

#ifdef ERR_INJ_DEBUG
	printk(KERN_INFO "Exit error injection driver.\n");
#endif

	cpu_notifier_register_begin();

	for_each_online_cpu(i) {
		sys_dev = get_cpu_device(i);
		sysfs_remove_group(&sys_dev->kobj, &err_inject_attr_group);
	}

	__unregister_hotcpu_notifier(&err_inject_cpu_notifier);

	cpu_notifier_register_done();
}

module_init(err_inject_init);
module_exit(err_inject_exit);

MODULE_AUTHOR("Fenghua Yu <fenghua.yu@intel.com>");
MODULE_DESCRIPTION("MC error injection kernel sysfs interface");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                   /*
 * Extensible SAL Interface (ESI) support routines.
 *
 * Copyright (C) 2006 Hewlett-Packard Co
 * 	Alex Williamson <alex.williamson@hp.com>
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>

#include <asm/esi.h>
#include <asm/sal.h>

MODULE_AUTHOR("Alex Williamson <alex.williamson@hp.com>");
MODULE_DESCRIPTION("Extensible SAL Interface (ESI) support");
MODULE_LICENSE("GPL");

#define MODULE_NAME	"esi"

#define ESI_TABLE_GUID					\
    EFI_GUID(0x43EA58DC, 0xCF28, 0x4b06, 0xB3,		\
	     0x91, 0xB7, 0x50, 0x59, 0x34, 0x2B, 0xD4)

enum esi_systab_entry_type {
	ESI_DESC_ENTRY_POINT = 0
};

/*
 * Entry type:	Size:
 *	0	48
 */
#define ESI_DESC_SIZE(type)	"\060"[(unsigned) (type)]

typedef struct ia64_esi_desc_entry_point {
	u8 type;
	u8 reserved1[15];
	u64 esi_proc;
	u64 gp;
	efi_guid_t guid;
} ia64_esi_desc_entry_point_t;

struct pdesc {
	void *addr;
	void *gp;
};

static struct ia64_sal_systab *esi_systab;

static int __init esi_init (void)
{
	efi_config_table_t *config_tables;
	struct ia64_sal_systab *systab;
	unsigned long esi = 0;
	char *p;
	int i;

	config_tables = __va(efi.systab->tables);

	for (i = 0; i < (int) efi.systab->nr_tables; ++i) {
		if (efi_guidcmp(config_tables[i].guid, ESI_TABLE_GUID) == 0) {
			esi = config_tables[i].table;
			break;
		}
	}

	if (!esi)
		return -ENODEV;

	systab = __va(esi);

	if (strncmp(systab->signature, "ESIT", 4) != 0) {
		printk(KERN_ERR "bad signature in ESI system table!");
		return -ENODEV;
	}

	p = (char *) (systab + 1);
	for (i = 0; i < systab->entry_count; i++) {
		/*
		 * The first byte of each entry type contains the type
		 * descriptor.
		 */
		switch (*p) {
		      case ESI_DESC_ENTRY_POINT:
			break;
		      default:
			printk(KERN_WARNING "Unknown table type %d found in "
			       "ESI table, ignoring rest of table\n", *p);
			return -ENODEV;
		}

		p += ESI_DESC_SIZE(*p);
	}

	esi_systab = systab;
	return 0;
}


int ia64_esi_call (efi_guid_t guid, struct ia64_sal_retval *isrvp,
		   enum esi_proc_type proc_type, u64 func,
		   u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6,
		   u64 arg7)
{
	struct ia64_fpreg fr[6];
	unsigned long flags = 0;
	int i;
	char *p;

	if (!esi_systab)
		return -1;

	p = (char *) (esi_systab + 1);
	for (i = 0; i < esi_systab->entry_count; i++) {
		if (*p == ESI_DESC_ENTRY_POINT) {
			ia64_esi_desc_entry_point_t *esi = (void *)p;
			if (!efi_guidcmp(guid, esi->guid)) {
				ia64_sal_handler esi_proc;
				struct pdesc pdesc;

				pdesc.addr = __va(esi->esi_proc);
				pdesc.gp = __va(esi->gp);

				esi_proc = (ia64_sal_handler) &pdesc;

				ia64_save_scratch_fpregs(fr);
				if (proc_type == ESI_PROC_SERIALIZED)
					spin_lock_irqsave(&sal_lock, flags);
				else if (proc_type == ESI_PROC_MP_SAFE)
					local_irq_save(flags);
				else
					preempt_disable();
				*isrvp = (*esi_proc)(func, arg1, arg2, arg3,
						     arg4, arg5, arg6, arg7);
				if (proc_type == ESI_PROC_SERIALIZED)
					spin_unlock_irqrestore(&sal_lock,
							       flags);
				else if (proc_type == ESI_PROC_MP_SAFE)
					local_irq_restore(flags);
				else
					preempt_enable();
				ia64_load_scratch_fpregs(fr);
				return 0;
			}
		}
		p += ESI_DESC_SIZE(*p);
	}
	return -1;
}
EXPORT_SYMBOL_GPL(ia64_esi_call);

int ia64_esi_call_phys (efi_guid_t guid, struct ia64_sal_retval *isrvp,
			u64 func, u64 arg1, u64 arg2, u64 arg3, u64 arg4,
			u64 arg5, u64 arg6, u64 arg7)
{
	struct ia64_fpreg fr[6];
	unsigned long flags;
	u64 esi_params[8];
	char *p;
	int i;

	if (!esi_systab)
		return -1;

	p = (char *) (esi_systab + 1);
	for (i = 0; i < esi_systab->entry_count; i++) {
		if (*p == ESI_DESC_ENTRY_POINT) {
			ia64_esi_desc_entry_point_t *esi = (void *)p;
			if (!efi_guidcmp(guid, esi->guid)) {
				ia64_sal_handler esi_proc;
				struct pdesc pdesc;

				pdesc.addr = (void *)esi->esi_proc;
				pdesc.gp = (void *)esi->gp;

				esi_proc = (ia64_sal_handler) &pdesc;

				esi_params[0] = func;
				esi_params[1] = arg1;
				esi_params[2] = arg2;
				esi_params[3] = arg3;
				esi_params[4] = arg4;
				esi_params[5] = arg5;
				esi_params[6] = arg6;
				esi_params[7] = arg7;
				ia64_save_scratch_fpregs(fr);
				spin_lock_irqsave(&sal_lock, flags);
				*isrvp = esi_call_phys(esi_proc, esi_params);
				spin_unlock_irqrestore(&sal_lock, flags);
				ia64_load_scratch_fpregs(fr);
				return 0;
			}
		}
		p += ESI_DESC_SIZE(*p);
	}
	return -1;
}
EXPORT_SYMBOL_GPL(ia64_esi_call_phys);

static void __exit esi_exit (void)
{
}

module_init(esi_init);
module_exit(esi_exit);	/* makes module removable... */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   