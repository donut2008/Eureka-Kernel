/*
 * Copyright (C) 1998-2004 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 * Copyright (C) 2003 Intel Co
 *	Suresh Siddha <suresh.b.siddha@intel.com>
 *	Fenghua Yu <fenghua.yu@intel.com>
 *	Arun Sharma <arun.sharma@intel.com>
 *
 * 12/07/98	S. Eranian	added pt_regs & switch_stack
 * 12/21/98	D. Mosberger	updated to match latest code
 *  6/17/99	D. Mosberger	added second unat member to "struct switch_stack"
 *
 */
#ifndef _UAPI_ASM_IA64_PTRACE_H
#define _UAPI_ASM_IA64_PTRACE_H

/*
 * When a user process is blocked, its state looks as follows:
 *
 *            +----------------------+	-------	IA64_STK_OFFSET
 *     	      |			     |	 ^
 *            | struct pt_regs       |	 |
 *	      |			     |	 |
 *            +----------------------+	 |
 *	      |			     |	 |
 *     	      |	   memory stack	     |	 |
 *	      |	(growing downwards)  |	 |
 *	      //.....................//	 |
 *					 |
 *	      //.....................//	 |
 *	      |			     |	 |
 *            +----------------------+	 |
 *            | struct switch_stack  |	 |
 *	      |			     |	 |
 *	      +----------------------+	 |
 *	      |			     |	 |
 *	      //.....................//	 |
 *					 |
 *	      //.....................//	 |
 *	      |			     |	 |
 *	      |	 register stack	     |	 |
 *	      |	(growing upwards)    |	 |
 *            |			     |	 |
 *	      +----------------------+	 |  ---	IA64_RBS_OFFSET
 *            |  struct thread_info  |	 |  ^
 *	      +----------------------+	 |  |
 *	      |			     |	 |  |
 *            |  struct task_struct  |	 |  |
 * current -> |			     |   |  |
 *	      +----------------------+ -------
 *
 * Note that ar.ec is not saved explicitly in pt_reg or switch_stack.
 * This is because ar.ec is saved as part of ar.pfs.
 */


#include <asm/fpu.h>


#ifndef __ASSEMBLY__

/*
 * This struct defines the way the registers are saved on system
 * calls.
 *
 * We don't save all floating point register because the kernel
 * is compiled to use only a very small subset, so the other are
 * untouched.
 *
 * THIS STRUCTURE MUST BE A MULTIPLE 16-BYTE IN SIZE
 * (because the memory stack pointer MUST ALWAYS be aligned this way)
 *
 */
struct pt_regs {
	/* The following registers are saved by SAVE_MIN: */
	unsigned long b6;		/* scratch */
	unsigned long b7;		/* scratch */

	unsigned long ar_csd;           /* used by cmp8xchg16 (scratch) */
	unsigned long ar_ssd;           /* reserved for future use (scratch) */

	unsigned long r8;		/* scratch (return value register 0) */
	unsigned long r9;		/* scratch (return value register 1) */
	unsigned long r10;		/* scratch (return value register 2) */
	unsigned long r11;		/* scratch (return value register 3) */

	unsigned long cr_ipsr;		/* interrupted task's psr */
	unsigned long cr_iip;		/* interrupted task's instruction pointer */
	/*
	 * interrupted task's function state; if bit 63 is cleared, it
	 * contains syscall's ar.pfs.pfm:
	 */
	unsigned long cr_ifs;

	unsigned long ar_unat;		/* interrupted task's NaT register (preserved) */
	unsigned long ar_pfs;		/* prev function state  */
	unsigned long ar_rsc;		/* RSE configuration */
	/* The following two are valid only if cr_ipsr.cpl > 0 || ti->flags & _TIF_MCA_INIT */
	unsigned long ar_rnat;		/* RSE NaT */
	unsigned long ar_bspstore;	/* RSE bspstore */

	unsigned long pr;		/* 64 predicate registers (1 bit each) */
	unsigned long b0;		/* return pointer (bp) */
	unsigned long loadrs;		/* size of dirty partition << 16 */

	unsigned long r1;		/* the gp pointer */
	unsigned long r12;		/* interrupted task's memory stack pointer */
	unsigned long r13;		/* thread pointer */

	unsigned long ar_fpsr;		/* floating point status (preserved) */
	unsigned long r15;		/* scratch */

	/* The remaining registers are NOT saved for system calls.  */

	unsigned long r14;		/* scratch */
	unsigned long r2;		/* scratch */
	unsigned long r3;		/* scratch */

	/* The following registers are saved by SAVE_REST: */
	unsigned long r16;		/* scratch */
	unsigned long r17;		/* scratch */
	unsigned long r18;		/* scratch */
	unsigned long r19;		/* scratch */
	unsigned long r20;		/* scratch */
	unsigned long r21;		/* scratch */
	unsigned long r22;		/* scratch */
	unsigned long r23;		/* scratch */
	unsigned long r24;		/* scratch */
	unsigned long r25;		/* scratch */
	unsigned long r26;		/* scratch */
	unsigned long r27;		/* scratch */
	unsigned long r28;		/* scratch */
	unsigned long r29;		/* scratch */
	unsigned long r30;		/* scratch */
	unsigned long r31;		/* scratch */

	unsigned long ar_ccv;		/* compare/exchange value (scratch) */

	/*
	 * Floating point registers that the kernel considers scratch:
	 */
	struct ia64_fpreg f6;		/* scratch */
	struct ia64_fpreg f7;		/* scratch */
	struct ia64_fpreg f8;		/* scratch */
	struct ia64_fpreg f9;		/* scratch */
	struct ia64_fpreg f10;		/* scratch */
	struct ia64_fpreg f11;		/* scratch */
};

/*
 * This structure contains the addition registers that need to
 * preserved across a context switch.  This generally consists of
 * "preserved" registers.
 */
struct switch_stack {
	unsigned long caller_unat;	/* user NaT collection register (preserved) */
	unsigned long ar_fpsr;		/* floating-point status register */

	struct ia64_fpreg f2;		/* preserved */
	struct ia64_fpreg f3;		/* preserved */
	struct ia64_fpreg f4;		/* preserved */
	struct ia64_fpreg f5;		/* preserved */

	struct ia64_fpreg f12;		/* scratch, but untouched by kernel */
	struct ia64_fpreg f13;		/* scratch, but untouched by kernel */
	struct ia64_fpreg f14;		/* scratch, but untouched by kernel */
	struct ia64_fpreg f15;		/* scratch, but untouched by kernel */
	struct ia64_fpreg f16;		/* preserved */
	struct ia64_fpreg f17;		/* preserved */
	struct ia64_fpreg f18;		/* preserved */
	struct ia64_fpreg f19;		/* preserved */
	struct ia64_fpreg f20;		/* preserved */
	struct ia64_fpreg f21;		/* preserved */
	struct ia64_fpreg f22;		/* preserved */
	struct ia64_fpreg f23;		/* preserved */
	struct ia64_fpreg f24;		/* preserved */
	struct ia64_fpreg f25;		/* preserved */
	struct ia64_fpreg f26;		/* preserved */
	struct ia64_fpreg f27;		/* preserved */
	struct ia64_fpreg f28;		/* preserved */
	struct ia64_fpreg f29;		/* preserved */
	struct ia64_fpreg f30;		/* preserved */
	struct ia64_fpreg f31;		/* preserved */

	unsigned long r4;		/* preserved */
	unsigned long r5;		/* preserved */
	unsigned long r6;		/* preserved */
	unsigned long r7;		/* preserved */

	unsigned long b0;		/* so we can force a direct return in copy_thread */
	unsigned long b1;
	unsigned long b2;
	unsigned long b3;
	unsigned long b4;
	unsigned long b5;

	unsigned long ar_pfs;		/* previous function state */
	unsigned long ar_lc;		/* loop counter (preserved) */
	unsigned long ar_unat;		/* NaT bits for r4-r7 */
	unsigned long ar_rnat;		/* RSE NaT collection register */
	unsigned long ar_bspstore;	/* RSE dirty base (preserved) */
	unsigned long pr;		/* 64 predicate registers (1 bit each) */
};


/* pt_all_user_regs is used for PTRACE_GETREGS PTRACE_SETREGS */
struct pt_all_user_regs {
	unsigned long nat;
	unsigned long cr_iip;
	unsigned long cfm;
	unsigned long cr_ipsr;
	unsigned long pr;

	unsigned long gr[32];
	unsigned long br[8];
	unsigned long ar[128];
	struct ia64_fpreg fr[128];
};

#endif /* !__ASSEMBLY__ */

/* indices to application-registers array in pt_all_user_regs */
#define PT_AUR_RSC	16
#define PT_AUR_BSP	17
#define PT_AUR_BSPSTORE	18
#define PT_AUR_RNAT	19
#define PT_AUR_CCV	32
#define PT_AUR_UNAT	36
#define PT_AUR_FPSR	40
#define PT_AUR_PFS	64
#define PT_AUR_LC	65
#define PT_AUR_EC	66

/*
 * The numbers chosen here are somewhat arbitrary but absolutely MUST
 * not overlap with any of the number assigned in <linux/ptrace.h>.
 */
#define PTRACE_SINGLEBLOCK	12	/* resume execution until next branch */
#define PTRACE_OLD_GETSIGINFO	13	/* (replaced by PTRACE_GETSIGINFO in <linux/ptrace.h>)  */
#define PTRACE_OLD_SETSIGINFO	14	/* (replaced by PTRACE_SETSIGINFO in <linux/ptrace.h>)  */
#define PTRACE_GETREGS		18	/* get all registers (pt_all_user_regs) in one shot */
#define PTRACE_SETREGS		19	/* set all registers (pt_all_user_regs) in one shot */

#define PTRACE_OLDSETOPTIONS	21

#endif /* _UAPI_ASM_IA64_PTRACE_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     #ifndef _ASM_IA64_PTRACE_OFFSETS_H
#define _ASM_IA64_PTRACE_OFFSETS_H

/*
 * Copyright (C) 1999, 2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */
/*
 * The "uarea" that can be accessed via PEEKUSER and POKEUSER is a
 * virtual structure that would have the following definition:
 *
 *	struct uarea {
 *		struct ia64_fpreg fph[96];		// f32-f127
 *		unsigned long nat_bits;
 *		unsigned long empty1;
 *		struct ia64_fpreg f2;			// f2-f5
 *			:
 *		struct ia64_fpreg f5;
 *		struct ia64_fpreg f10;			// f10-f31
 *			:
 *		struct ia64_fpreg f31;
 *		unsigned long r4;			// r4-r7
 *			:
 *		unsigned long r7;
 *		unsigned long b1;			// b1-b5
 *			:
 *		unsigned long b5;
 *		unsigned long ar_ec;
 *		unsigned long ar_lc;
 *		unsigned long empty2[5];
 *		unsigned long cr_ipsr;
 *		unsigned long cr_iip;
 *		unsigned long cfm;
 *		unsigned long ar_unat;
 *		unsigned long ar_pfs;
 *		unsigned long ar_rsc;
 *		unsigned long ar_rnat;
 *		unsigned long ar_bspstore;
 *		unsigned long pr;
 *		unsigned long b6;
 *		unsigned long ar_bsp;
 *		unsigned long r1;
 *		unsigned long r2;
 *		unsigned long r3;
 *		unsigned long r12;
 *		unsigned long r13;
 *		unsigned long r14;
 *		unsigned long r15;
 *		unsigned long r8;
 *		unsigned long r9;
 *		unsigned long r10;
 *		unsigned long r11;
 *		unsigned long r16;
 *			:
 *		unsigned long r31;
 *		unsigned long ar_ccv;
 *		unsigned long ar_fpsr;
 *		unsigned long b0;
 *		unsigned long b7;
 *		unsigned long f6;
 *		unsigned long f7;
 *		unsigned long f8;
 *		unsigned long f9;
 *		unsigned long ar_csd;
 *		unsigned long ar_ssd;
 *		unsigned long rsvd1[710];
 *		unsigned long dbr[8];
 *		unsigned long rsvd2[504];
 *		unsigned long ibr[8];
 *		unsigned long rsvd3[504];
 *		unsigned long pmd[4];
 *	}
 */

/* fph: */
#define PT_F32			0x0000
#define PT_F33			0x0010
#define PT_F34			0x0020
#define PT_F35			0x0030
#define PT_F36			0x0040
#define PT_F37			0x0050
#define PT_F38			0x0060
#define PT_F39			0x0070
#define PT_F40			0x0080
#define PT_F41			0x0090
#define PT_F42			0x00a0
#define PT_F43			0x00b0
#define PT_F44			0x00c0
#define PT_F45			0x00d0
#define PT_F46			0x00e0
#define PT_F47			0x00f0
#define PT_F48			0x0100
#define PT_F49			0x0110
#define PT_F50			0x0120
#define PT_F51			0x0130
#define PT_F52			0x0140
#define PT_F53			0x0150
#define PT_F54			0x0160
#define PT_F55			0x0170
#define PT_F56			0x0180
#define PT_F57			0x0190
#define PT_F58			0x01a0
#define PT_F59			0x01b0
#define PT_F60			0x01c0
#define PT_F61			0x01d0
#define PT_F62			0x01e0
#define PT_F63			0x01f0
#define PT_F64			0x0200
#define PT_F65			0x0210
#define PT_F66			0x0220
#define PT_F67			0x0230
#define PT_F68			0x0240
#define PT_F69			0x0250
#define PT_F70			0x0260
#define PT_F71			0x0270
#define PT_F72			0x0280
#define PT_F73			0x0290
#define PT_F74			0x02a0
#define PT_F75			0x02b0
#define PT_F76			0x02c0
#define PT_F77			0x02d0
#define PT_F78			0x02e0
#define PT_F79			0x02f0
#define PT_F80			0x0300
#define PT_F81			0x0310
#define PT_F82			0x0320
#define PT_F83			0x0330
#define PT_F84			0x0340
#define PT_F85			0x0350
#define PT_F86			0x0360
#define PT_F87			0x0370
#define PT_F88			0x0380
#define PT_F89			0x0390
#define PT_F90			0x03a0
#define PT_F91			0x03b0
#define PT_F92			0x03c0
#define PT_F93			0x03d0
#define PT_F94			0x03e0
#define PT_F95			0x03f0
#define PT_F96			0x0400
#define PT_F97			0x0410
#define PT_F98			0x0420
#define PT_F99			0x0430
#define PT_F100			0x0440
#define PT_F101			0x0450
#define PT_F102			0x0460
#define PT_F103			0x0470
#define PT_F104			0x0480
#define PT_F105			0x0490
#define PT_F106			0x04a0
#define PT_F107			0x04b0
#define PT_F108			0x04c0
#define PT_F109			0x04d0
#define PT_F110			0x04e0
#define PT_F111			0x04f0
#define PT_F112			0x0500
#define PT_F113			0x0510
#define PT_F114			0x0520
#define PT_F115			0x0530
#define PT_F116			0x0540
#define PT_F117			0x0550
#define PT_F118			0x0560
#define PT_F119			0x0570
#define PT_F120			0x0580
#define PT_F121			0x0590
#define PT_F122			0x05a0
#define PT_F123			0x05b0
#define PT_F124			0x05c0
#define PT_F125			0x05d0
#define PT_F126			0x05e0
#define PT_F127			0x05f0

#define PT_NAT_BITS		0x0600

#define PT_F2			0x0610
#define PT_F3			0x0620
#define PT_F4			0x0630
#define PT_F5			0x0640
#define PT_F10			0x0650
#define PT_F11			0x0660
#define PT_F12			0x0670
#define PT_F13			0x0680
#define PT_F14			0x0690
#define PT_F15			0x06a0
#define PT_F16			0x06b0
#define PT_F17			0x06c0
#define PT_F18			0x06d0
#define PT_F19			0x06e0
#define PT_F20			0x06f0
#define PT_F21			0x0700
#define PT_F22			0x0710
#define PT_F23			0x0720
#define PT_F24			0x0730
#define PT_F25			0x0740
#define PT_F26			0x0750
#define PT_F27			0x0760
#define PT_F28			0x0770
#define PT_F29			0x0780
#define PT_F30			0x0790
#define PT_F31			0x07a0
#define PT_R4			0x07b0
#define PT_R5			0x07b8
#define PT_R6			0x07c0
#define PT_R7			0x07c8

#define PT_B1			0x07d8
#define PT_B2			0x07e0
#define PT_B3			0x07e8
#define PT_B4			0x07f0
#define PT_B5			0x07f8

#define PT_AR_EC		0x0800
#define PT_AR_LC		0x0808

#define PT_CR_IPSR		0x0830
#define PT_CR_IIP		0x0838
#define PT_CFM			0x0840
#define PT_AR_UNAT		0x0848
#define PT_AR_PFS		0x0850
#define PT_AR_RSC		0x0858
#define PT_AR_RNAT		0x0860
#define PT_AR_BSPSTORE		0x0868
#define PT_PR			0x0870
#define PT_B6			0x0878
#define PT_AR_BSP		0x0880	/* note: this points to the *end* of the backing store! */
#define PT_R1			0x0888
#define PT_R2			0x0890
#define PT_R3			0x0898
#define PT_R12			0x08a0
#define PT_R13			0x08a8
#define PT_R14			0x08b0
#define PT_R15			0x08b8
#define PT_R8 			0x08c0
#define PT_R9			0x08c8
#define PT_R10			0x08d0
#define PT_R11			0x08d8
#define PT_R16			0x08e0
#define PT_R17			0x08e8
#define PT_R18			0x08f0
#define PT_R19			0x08f8
#define PT_R20			0x0900
#define PT_R21			0x0908
#define PT_R22			0x0910
#define PT_R23			0x0918
#define PT_R24			0x0920
#define PT_R25			0x0928
#define PT_R26			0x0930
#define PT_R27			0x0938
#define PT_R28			0x0940
#define PT_R29			0x0948
#define PT_R30			0x0950
#define PT_R31			0x0958
#define PT_AR_CCV		0x0960
#define PT_AR_FPSR		0x0968
#define PT_B0			0x0970
#define PT_B7			0x0978
#define PT_F6			0x0980
#define PT_F7			0x0990
#define PT_F8			0x09a0
#define PT_F9			0x09b0
#define PT_AR_CSD		0x09c0
#define PT_AR_SSD		0x09c8

#define PT_DBR			0x2000	/* data breakpoint registers */
#define PT_IBR			0x3000	/* instruction breakpoint registers */
#define PT_PMD			0x4000	/* performance monitoring counters */

#endif /* _ASM_IA64_PTRACE_OFFSETS_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     #ifndef _ASM_IA64_RESOURCE_H
#define _ASM_IA64_RESOURCE_H

#include <asm/ustack.h>
#include <asm-generic/resource.h>

#endif /* _ASM_IA64_RESOURCE_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        #ifndef _ASM_IA64_RSE_H
#define _ASM_IA64_RSE_H

/*
 * Copyright (C) 1998, 1999 Hewlett-Packard Co
 * Copyright (C) 1998, 1999 David Mosberger-Tang <davidm@hpl.hp.com>
 *
 * Register stack engine related helper functions.  This file may be
 * used in applications, so be careful about the name-space and give
 * some consideration to non-GNU C compilers (though __inline__ is
 * fine).
 */

static __inline__ unsigned long
ia64_rse_slot_num (unsigned long *addr)
{
	return (((unsigned long) addr) >> 3) & 0x3f;
}

/*
 * Return TRUE if ADDR is the address of an RNAT slot.
 */
static __inline__ unsigned long
ia64_rse_is_rnat_slot (unsigned long *addr)
{
	return ia64_rse_slot_num(addr) == 0x3f;
}

/*
 * Returns the address of the RNAT slot that covers the slot at
 * address SLOT_ADDR.
 */
static __inline__ unsigned long *
ia64_rse_rnat_addr (unsigned long *slot_addr)
{
	return (unsigned long *) ((unsigned long) slot_addr | (0x3f << 3));
}

/*
 * Calculate the number of registers in the dirty partition starting at BSPSTORE and
 * ending at BSP.  This isn't simply (BSP-BSPSTORE)/8 because every 64th slot stores
 * ar.rnat.
 */
static __inline__ unsigned long
ia64_rse_num_regs (unsigned long *bspstore, unsigned long *bsp)
{
	unsigned long slots = (bsp - bspstore);

	return slots - (ia64_rse_slot_num(bspstore) + slots)/0x40;
}

/*
 * The inverse of the above: given bspstore and the number of
 * registers, calculate ar.bsp.
 */
static __inline__ unsigned long *
ia64_rse_skip_regs (unsigned long *addr, long num_regs)
{
	long delta = ia64_rse_slot_num(addr) + num_regs;

	if (num_regs < 0)
		delta -= 0x3e;
	return addr + num_regs + delta/0x3f;
}

#endif /* _ASM_IA64_RSE_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         #ifndef _ASM_IA64_SEMBUF_H
#define _ASM_IA64_SEMBUF_H

/*
 * The semid64_ds structure for IA-64 architecture.
 * Note extra padding because this structure is passed back and forth
 * between kernel and user space.
 *
 * Pad space is left for:
 * - 2 miscellaneous 64-bit values
 */

struct semid64_ds {
	struct ipc64_perm sem_perm;		/* permissions .. see ipc.h */
	__kernel_time_t	sem_otime;		/* last semop time */
	__kernel_time_t	sem_ctime;		/* last change time */
	unsigned long	sem_nsems;		/* no. of semaphores in array */
	unsigned long	__unused1;
	unsigned long	__unused2;
};

#endif /* _ASM_IA64_SEMBUF_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         #ifndef __IA64_SETUP_H
#define __IA64_SETUP_H

#define COMMAND_LINE_SIZE	2048

extern struct ia64_boot_param {
	__u64 command_line;		/* physical address of command line arguments */
	__u64 efi_systab;		/* physical address of EFI system table */
	__u64 efi_memmap;		/* physical address of EFI memory map */
	__u64 efi_memmap_size;		/* size of EFI memory map */
	__u64 efi_memdesc_size;		/* size of an EFI memory map descriptor */
	__u32 efi_memdesc_version;	/* memory descriptor version */
	struct {
		__u16 num_cols;	/* number of columns on console output device */
		__u16 num_rows;	/* number of rows on console output device */
		__u16 orig_x;	/* cursor's x position */
		__u16 orig_y;	/* cursor's y position */
	} console_info;
	__u64 fpswa;		/* physical address of the fpswa interface */
	__u64 initrd_start;
	__u64 initrd_size;
} *ia64_boot_param;

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   #ifndef _ASM_IA64_SHMBUF_H
#define _ASM_IA64_SHMBUF_H

/*
 * The shmid64_ds structure for IA-64 architecture.
 * Note extra padding because this structure is passed back and forth
 * between kernel and user space.
 *
 * Pad space is left for:
 * - 2 miscellaneous 64-bit values
 */

struct shmid64_ds {
	struct ipc64_perm	shm_perm;	/* operation perms */
	size_t			shm_segsz;	/* size of segment (bytes) */
	__kernel_time_t		shm_atime;	/* last attach time */
	__kernel_time_t		shm_dtime;	/* last detach time */
	__kernel_time_t		shm_ctime;	/* last change time */
	__kernel_pid_t		shm_cpid;	/* pid of creator */
	__kernel_pid_t		shm_lpid;	/* pid of last operator */
	unsigned long		shm_nattch;	/* no. of current attaches */
	unsigned long		__unused1;
	unsigned long		__unused2;
};

struct shminfo64 {
	unsigned long	shmmax;
	unsigned long	shmmin;
	unsigned long	shmmni;
	unsigned long	shmseg;
	unsigned long	shmall;
	unsigned long	__unused1;
	unsigned long	__unused2;
	unsigned long	__unused3;
	unsigned long	__unused4;
};

#endif /* _ASM_IA64_SHMBUF_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   #ifndef _ASM_IA64_SIGCONTEXT_H
#define _ASM_IA64_SIGCONTEXT_H

/*
 * Copyright (C) 1998, 1999, 2001 Hewlett-Packard Co
 * Copyright (C) 1998, 1999, 2001 David Mosberger-Tang <davidm@hpl.hp.com>
 */

#include <asm/fpu.h>

#define IA64_SC_FLAG_ONSTACK_BIT		0	/* is handler running on signal stack? */
#define IA64_SC_FLAG_IN_SYSCALL_BIT		1	/* did signal interrupt a syscall? */
#define IA64_SC_FLAG_FPH_VALID_BIT		2	/* is state in f[32]-f[127] valid? */

#define IA64_SC_FLAG_ONSTACK		(1 << IA64_SC_FLAG_ONSTACK_BIT)
#define IA64_SC_FLAG_IN_SYSCALL		(1 << IA64_SC_FLAG_IN_SYSCALL_BIT)
#define IA64_SC_FLAG_FPH_VALID		(1 << IA64_SC_FLAG_FPH_VALID_BIT)

# ifndef __ASSEMBLY__

/*
 * Note on handling of register backing store: sc_ar_bsp contains the address that would
 * be found in ar.bsp after executing a "cover" instruction the context in which the
 * signal was raised.  If signal delivery required switching to an alternate signal stack
 * (sc_rbs_base is not NULL), the "dirty" partition (as it would exist after executing the
 * imaginary "cover" instruction) is backed by the *alternate* signal stack, not the
 * original one.  In this case, sc_rbs_base contains the base address of the new register
 * backing store.  The number of registers in the dirty partition can be calculated as:
 *
 *   ndirty = ia64_rse_num_regs(sc_rbs_base, sc_rbs_base + (sc_loadrs >> 16))
 *
 */

struct sigcontext {
	unsigned long		sc_flags;	/* see manifest constants above */
	unsigned long		sc_nat;		/* bit i == 1 iff scratch reg gr[i] is a NaT */
	stack_t			sc_stack;	/* previously active stack */

	unsigned long		sc_ip;		/* instruction pointer */
	unsigned long		sc_cfm;		/* current frame marker */
	unsigned long		sc_um;		/* user mask bits */
	unsigned long		sc_ar_rsc;	/* register stack configuration register */
	unsigned long		sc_ar_bsp;	/* backing store pointer */
	unsigned long		sc_ar_rnat;	/* RSE NaT collection register */
	unsigned long		sc_ar_ccv;	/* compare and exchange compare value register */
	unsigned long		sc_ar_unat;	/* ar.unat of interrupted context */
	unsigned long		sc_ar_fpsr;	/* floating-point status register */
	unsigned long		sc_ar_pfs;	/* previous function state */
	unsigned long		sc_ar_lc;	/* loop count register */
	unsigned long		sc_pr;		/* predicate registers */
	unsigned long		sc_br[8];	/* branch registers */
	/* Note: sc_gr[0] is used as the "uc_link" member of ucontext_t */
	unsigned long		sc_gr[32];	/* general registers (static partition) */
	struct ia64_fpreg	sc_fr[128];	/* floating-point registers */

	unsigned long		sc_rbs_base;	/* NULL or new base of sighandler's rbs */
	unsigned long		sc_loadrs;	/* see description above */

	unsigned long		sc_ar25;	/* cmp8xchg16 uses this */
	unsigned long		sc_ar26;	/* rsvd for scratch use */
	unsigned long		sc_rsvd[12];	/* reserved for future use */
	/*
	 * The mask must come last so we can increase _NSIG_WORDS
	 * without breaking binary compatibility.
	 */
	sigset_t		sc_mask;	/* signal mask to restore after handler returns */
};

# endif /* __ASSEMBLY__ */
#endif /* _ASM_IA64_SIGCONTEXT_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * Based on <asm-i386/siginfo.h>.
 *
 * Modified 1998-2002
 *	David Mosberger-Tang <davidm@hpl.hp.com>, Hewlett-Packard Co
 */
#ifndef _UAPI_ASM_IA64_SIGINFO_H
#define _UAPI_ASM_IA64_SIGINFO_H


#define __ARCH_SI_PREAMBLE_SIZE	(4 * sizeof(int))

#define HAVE_ARCH_SIGINFO_T
#define HAVE_ARCH_COPY_SIGINFO
#define HAVE_ARCH_COPY_SIGINFO_TO_USER

#include <asm-generic/siginfo.h>

typedef struct siginfo {
	int si_signo;
	int si_errno;
	int si_code;
	int __pad0;

	union {
		int _pad[SI_PAD_SIZE];

		/* kill() */
		struct {
			pid_t _pid;		/* sender's pid */
			uid_t _uid;		/* sender's uid */
		} _kill;

		/* POSIX.1b timers */
		struct {
			timer_t _tid;		/* timer id */
			int _overrun;		/* overrun count */
			char _pad[sizeof(__ARCH_SI_UID_T) - sizeof(int)];
			sigval_t _sigval;	/* must overlay ._rt._sigval! */
			int _sys_private;	/* not to be passed to user */
		} _timer;

		/* POSIX.1b signals */
		struct {
			pid_t _pid;		/* sender's pid */
			uid_t _uid;		/* sender's uid */
			sigval_t _sigval;
		} _rt;

		/* SIGCHLD */
		struct {
			pid_t _pid;		/* which child */
			uid_t _uid;		/* sender's uid */
			int _status;		/* exit code */
			clock_t _utime;
			clock_t _stime;
		} _sigchld;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
		struct {
			void __user *_addr;	/* faulting insn/memory ref. */
			int _imm;		/* immediate value for "break" */
			unsigned int _flags;	/* see below */
			unsigned long _isr;	/* isr */
			short _addr_lsb;	/* lsb of faulting address */
			struct {
				void __user *_lower;
				void __user *_upper;
			} _addr_bnd;
		} _sigfault;

		/* SIGPOLL */
		struct {
			long _band;	/* POLL_IN, POLL_OUT, POLL_MSG (XPG requires a "long") */
			int _fd;
		} _sigpoll;
	} _sifields;
} siginfo_t;

#define si_imm		_sifields._sigfault._imm	/* as per UNIX SysV ABI spec */
#define si_flags	_sifields._sigfault._flags
/*
 * si_isr is valid for SIGILL, SIGFPE, SIGSEGV, SIGBUS, and SIGTRAP provided that
 * si_code is non-zero and __ISR_VALID is set in si_flags.
 */
#define si_isr		_sifields._sigfault._isr

/*
 * Flag values for si_flags:
 */
#define __ISR_VALID_BIT	0
#define __ISR_VALID	(1 << __ISR_VALID_BIT)

/*
 * SIGILL si_codes
 */
#define ILL_BADIADDR	(__SI_FAULT|9)	/* unimplemented instruction address */
#define __ILL_BREAK	(__SI_FAULT|10)	/* illegal break */
#define __ILL_BNDMOD	(__SI_FAULT|11)	/* bundle-update (modification) in progress */
#undef NSIGILL
#define NSIGILL		11

/*
 * SIGFPE si_codes
 */
#define __FPE_DECOVF	(__SI_FAULT|9)	/* decimal overflow */
#define __FPE_DECDIV	(__SI_FAULT|10)	/* decimal division by zero */
#define __FPE_DECERR	(__SI_FAULT|11)	/* packed decimal error */
#define __FPE_INVASC	(__SI_FAULT