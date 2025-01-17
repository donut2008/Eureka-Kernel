/*
 * Access to user system call parameters and results
 *
 * Copyright (C) 2008 Intel Corp.  Shaohua Li <shaohua.li@intel.com>
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * See asm-generic/syscall.h for descriptions of what we must do here.
 */

#ifndef _ASM_SYSCALL_H
#define _ASM_SYSCALL_H	1

#include <uapi/linux/audit.h>
#include <linux/sched.h>
#include <linux/err.h>

static inline long syscall_get_nr(struct task_struct *task,
				  struct pt_regs *regs)
{
	if ((long)regs->cr_ifs < 0) /* Not a syscall */
		return -1;

	return regs->r15;
}

static inline void syscall_rollback(struct task_struct *task,
				    struct pt_regs *regs)
{
	/* do nothing */
}

static inline long syscall_get_error(struct task_struct *task,
				     struct pt_regs *regs)
{
	return regs->r10 == -1 ? -regs->r8:0;
}

static inline long syscall_get_return_value(struct task_struct *task,
					    struct pt_regs *regs)
{
	return regs->r8;
}

static inline void syscall_set_return_value(struct task_struct *task,
					    struct pt_regs *regs,
					    int error, long val)
{
	if (error) {
		/* error < 0, but ia64 uses > 0 return value */
		regs->r8 = -error;
		regs->r10 = -1;
	} else {
		regs->r8 = val;
		regs->r10 = 0;
	}
}

extern void ia64_syscall_get_set_arguments(struct task_struct *task,
	struct pt_regs *regs, unsigned int i, unsigned int n,
	unsigned long *args, int rw);
static inline void syscall_get_arguments(struct task_struct *task,
					 struct pt_regs *regs,
					 unsigned int i, unsigned int n,
					 unsigned long *args)
{
	BUG_ON(i + n > 6);

	ia64_syscall_get_set_arguments(task, regs, i, n, args, 0);
}

static inline void syscall_set_arguments(struct task_struct *task,
					 struct pt_regs *regs,
					 unsigned int i, unsigned int n,
					 unsigned long *args)
{
	BUG_ON(i + n > 6);

	ia64_syscall_get_set_arguments(task, regs, i, n, args, 1);
}

static inline int syscall_get_arch(void)
{
	return AUDIT_ARCH_IA64;
}
#endif	/* _ASM_SYSCALL_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
 * Modified 1999
 *	David Mosberger-Tang <davidm@hpl.hp.com>, Hewlett-Packard Co
 *
 * 99/01/28	Added N_IRDA and N_SMSBLOCK
 */
#ifndef _ASM_IA64_TERMIOS_H
#define _ASM_IA64_TERMIOS_H

#include <uapi/asm/termios.h>


/*	intr=^C		quit=^\		erase=del	kill=^U
	eof=^D		vtime=\0	vmin=\1		sxtc=\0
	start=^Q	stop=^S		susp=^Z		eol=\0
	reprint=^R	discard=^U	werase=^W	lnext=^V
	eol2=\0
*/
#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"

/*
 * Translate a "termio" structure into a "termios". Ugh.
 */
#define SET_LOW_TERMIOS_BITS(termios, termio, x) {	\
	unsigned short __tmp;				\
	get_user(__tmp,&(termio)->x);			\
	*(unsigned short *) &(termios)->x = __tmp;	\
}

#define user_termio_to_kernel_termios(termios, termio)		\
({								\
	SET_LOW_TERMIOS_BITS(termios, termio, c_iflag);		\
	SET_LOW_TERMIOS_BITS(termios, termio, c_oflag);		\
	SET_LOW_TERMIOS_BITS(termios, termio, c_cflag);		\
	SET_LOW_TERMIOS_BITS(termios, termio, c_lflag);		\
	copy_from_user((termios)->c_cc, (termio)->c_cc, NCC);	\
})

/*
 * Translate a "termios" structure into a "termio". Ugh.
 */
#define kernel_termios_to_user_termio(termio, termios)		\
({								\
	put_user((termios)->c_iflag, &(termio)->c_iflag);	\
	put_user((termios)->c_oflag, &(termio)->c_oflag);	\
	put_user((termios)->c_cflag, &(termio)->c_cflag);	\
	put_user((termios)->c_lflag, &(termio)->c_lflag);	\
	put_user((termios)->c_line,  &(termio)->c_line);	\
	copy_to_user((termio)->c_cc, (termios)->c_cc, NCC);	\
})

#define user_termios_to_kernel_termios(k, u) copy_from_user(k, u, sizeof(struct termios2))
#define kernel_termios_to_user_termios(u, k) copy_to_user(u, k, sizeof(struct termios2))
#define user_termios_to_kernel_termios_1(k, u) copy_from_user(k, u, sizeof(struct termios))
#define kernel_termios_to_user_termios_1(u, k) copy_to_user(u, k, sizeof(struct termios))

#endif /*