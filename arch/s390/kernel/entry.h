r16]=r30,TI_AC_STIME-TI_AC_STAMP	// update stamp
	sub r18=r30,r19				// elapsed time in user mode
	;;
	add r20=r20,r22				// sum stime
	add r21=r21,r18				// sum utime
	;;
	st8 [r16]=r20				// update stime
	st8 [r17]=r21				// update utime
	;;
#endif
	mov ar.rsc=0x3				// M2   set eager mode, pl 0, LE, loadrs=0
	mov rp=r14				// I0   set the real return addr
	and r3=_TIF_SYSCALL_TRACEAUDIT,r3	// A
	;;
	SSM_PSR_I(p0, p6, r22)			// M2   we're on kernel stacks now, reenable irqs
	cmp.eq p8,p0=r3,r0			// A
(p10)	br.cond.spnt.many ia64_ret_from_syscall	// B    return if bad call-frame or r15 is a NaT

	nop.m 0
(p8)	br.call.sptk.many b6=b6			// B    (ignore return address)
	br.cond.spnt ia64_trace_syscall		// B
END(fsys_bubble_down)

	.rodata
	.align 8
	.globl fsyscall_table

	data8 fsys_bubble_down
fsyscall_table:
	data8 fsys_ni_syscall
	data8 0				// exit			// 1025
	data8 0				// read
	data8 0				// write
	data8 0				// open
	data8 0				// close
	data8 0				// creat		// 1030
	data8 0				// link
	data8 0				// unlink
	data8 0				// execve
	data8 0				// chdir
	data8 0				// fchdir		// 1035
	data8 0				// utimes
	data8 0				// mknod
	data8 0				// chmod
	data8 0				// chown
	data8 0				// lseek		// 1040
	data8 fsys_getpid		// getpid
	data8 0				// getppid
	data8 0				// mount
	data8 0				// umount
	data8 0				// setuid		// 1045
	data8 0				// getuid
	data8 0				// geteuid
	data8 0				// ptrace
	data8 0				// access
	data8 0				// sync			// 1050
	data8 0				// fsync
	data8 0				// fdatasync
	data8 0				// kill
	data8 0				// rename
	data8 0				// mkdir		// 1055
	data8 0				// rmdir
	data8 0				// dup
	data8 0				// pipe
	data8 0				// times
	data8 0				// brk			// 1060
	data8 0				// setgid
	data8 0				// getgid
	data8 0				// getegid
	data8 0				// acct
	data8 0				// ioctl		// 1065
	data8 0				// fcntl
	data8 0				// umask
	data8 0				// chroot
	data8 0				// ustat
	data8 0				// dup2			// 1070
	data8 0				// setreuid
	data8 0				// setregid
	data8 0				// getresuid
	data8 0				// setresuid
	data8 0				// getresgid		// 1075
	data8 0				// setresgid
	data8 0				// getgroups
	data8 0				// setgroups
	data8 0				// getpgid
	data8 0				// setpgid		// 1080
	data8 0				// setsid
	data8 0				// getsid
	data8 0				// sethostname
	data8 0				// setrlimit
	data8 0				// getrlimit		// 1085
	data8 0				// getrusage
	data8 fsys_gettimeofday		// gettimeofday
	data8 0				// settimeofday
	data8 0				// select
	data8 0				// poll			// 1090
	data8 0				// symlink
	data8 0				// readlink
	data8 0				// uselib
	data8 0				// swapon
	data8 0				// swapoff		// 1095
	data8 0				// reboot
	data8 0				// truncate
	data8 0				// ftruncate
	data8 0				// fchmod
	data8 0				// fchown		// 1100
	data8 0				// getpriority
	data8 0				// setpriority
	data8 0				// statfs
	data8 0	