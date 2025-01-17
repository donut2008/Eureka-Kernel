/*
 * IA-64 Linux syscall numbers and inline-functions.
 *
 * Copyright (C) 1998-2005 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */
#ifndef _ASM_IA64_UNISTD_H
#define _ASM_IA64_UNISTD_H

#include <uapi/asm/unistd.h>



#define NR_syscalls			323 /* length of syscall table */

/*
 * The following defines stop scripts/checksyscalls.sh from complaining about
 * unimplemented system calls.  Glibc provides for each of these by using
 * more modern equivalent system calls.
 */
#define __IGNORE_fork		/* clone() */
#define __IGNORE_time		/* gettimeofday() */
#define __IGNORE_alarm		/* setitimer(ITIMER_REAL, ... */
#define __IGNORE_pause		/* rt_sigprocmask(), rt_sigsuspend() */
#define __IGNORE_utime		/* utimes() */
#define __IGNORE_getpgrp	/* getpgid() */
#define __IGNORE_vfork		/* clone() */
#define __IGNORE_umount2	/* umount() */

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLER)

#include <linux/types.h>
#include <linux/linkage.h>
#include <linux/compiler.h>

extern long __ia64_syscall (long a0, long a1, long a2, long a3, long a4, long nr);

asmlinkage unsigned long sys_mmap(
				unsigned long addr, unsigned long len,
				int prot, int flags,
				int fd, long off);
asmlinkage unsigned long sys_mmap2(
				unsigned long addr, unsigned long len,
				int prot, int flags,
				int fd, long pgoff);
struct pt_regs;
asmlinkage long sys_ia64_pipe(void);

#endif /* !__ASSEMBLY__ */
#endif /* _ASM_IA64_UNISTD_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #ifndef _ASM_IA64_UNWIND_H
#define _ASM_IA64_UNWIND_H

/*
 * Copyright (C) 1999-2000, 2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *
 * A simple API for unwinding kernel stacks.  This is used for
 * debugging and error reporting purposes.  The kernel doesn't need
 * full-blown stack unwinding with all the bells and whitles, so there
 * is not much point in implementing the full IA-64 unwind API (though
 * it would of course be possible to implement the kernel API on top
 * of it).
 */

struct task_struct;	/* forward declaration */
struct switch_stack;	/* forward declaration */

enum unw_application_register {
	UNW_AR_BSP,
	UNW_AR_BSPSTORE,
	UNW_AR_PFS,
	UNW_AR_RNAT,
	UNW_AR_UNAT,
	UNW_AR_LC,
	UNW_AR_EC,
	UNW_AR_FPSR,
	UNW_AR_RSC,
	UNW_AR_CCV,
	UNW_AR_CSD,
	UNW_AR_SSD
};

/*
 * The following declarations are private to the unwind
 * implementation:
 */

struct unw_stack {
	unsigned long limit;
	unsigned long top;
};

#define UNW_FLAG_INTERRUPT_FRAME	(1UL << 0)

/*
 * No user of this module should every access this structure directly
 * as it is subject to change.  It is declared here solely so we can
 * use automatic variables.
 */
struct unw_frame_info {
	struct unw_stack regstk;
	struct unw_stack memstk;
	unsigned int flags;
	short hint;
	short prev_script;

	/* current frame info: */
	unsigned long bsp;		/* backing store pointer value */
	unsigned long sp;		/* stack pointer value */
	unsigned long psp;		/* previous sp value */
	unsigned long ip;		/* instruction pointer value */
	unsigned long pr;		/* current predicate values */
	unsigned long *cfm_loc;		/* cfm save location (or NULL) */
	unsigned long pt;		/* struct pt_regs location */

	struct task_struct *task;
	struct switch_stack *sw;

	/* preserved state: */
	unsigned long *bsp_loc;		/* previous bsp save location */
	unsigned long *bspstore_loc;
	unsigned long *pfs_loc;
	unsigned long *rnat_loc;
	unsigned long *rp_loc;
	unsigned long *pri_unat_loc;
	unsigned long *unat_loc;
	unsigned long *pr_loc;
	unsigned long *lc_loc;
	unsigned long *fpsr_loc;
	struct unw_ireg {
		unsigned long *loc;
		struct unw_ireg_nat {
			unsigned long type : 3;		/* enum unw_nat_type */
			signed long off : 61;		/* NaT word is at loc+nat.off */
		} nat;
	} r4, r5, r6, r7;
	unsigned long *b1_loc, *b2_loc, *b3_loc, *b4_loc, *b5_loc;
	struct ia64_fpreg *f2_loc, *f3_loc, *f4_loc, *f5_loc, *fr_loc[16];
};

/*
 * The official API follows below:
 */

struct unw_table_entry {
	u64 start_offset;
	u64 end_offset;
	u64 info_offset;
};

/*
 * Initialize unwind support.
 */
extern void unw_init (void);

extern void *unw_add_unwind_table (const char *name, unsigned long segment_base, unsigned long gp,
				   const void *table_start, const void *table_end);

extern void unw_remove_unwind_table (void *handle);

/*
 * Prepare to unwind blocked task t.
 */
extern void unw_init_from_blocked_task (struct unw_frame_info *info, struct task_struct *t);

extern void unw_init_frame_info (struct unw_frame_info *info, struct task_struct *t,
				 struct switch_stack *sw);

/*
 * Prepare to unwind the currently running thread.
 */
extern void unw_init_running (void (*callback)(struct unw_frame_info *info, void *arg), void *arg);

/*
 * Unwind to previous to frame.  Returns 0 if successful, negative
 * number in case of an error.
 */
extern int unw_unwind (struct unw_frame_info *info);

/*
 * Unwind until the return pointer is in user-land (or until an error
 * occurs).  Returns 0 if successful, negative number in case of
 * error.
 */
extern int unw_unwind_to_user (struct unw_frame_info *info);

#define unw_is_intr_frame(info)	(((info)->flags & UNW_FLAG_INTERRUPT_FRAME) != 0)

static inline int
unw_get_ip (struct unw_frame_info *info, unsigned long *valp)
{
	*valp = (info)->ip;
	return 0;
}

static inline int
unw_get_sp (struct unw_frame_info *info, unsigned long *valp)
{
	*valp = (info)->sp;
	return 0;
}

static inline int
unw_get_psp (struct unw_frame_info *info, unsigned long *valp)
{
	*valp = (info)->psp;
	return 0;
}

static inline int
unw_get_bsp (struct unw_frame_info *info, unsigned long *valp)
{
	*valp = (info)->bsp;
	return 0;
}

static inline int
unw_get_cfm (struct unw_frame_info *info, unsigned long *valp)
{
	*valp = *(info)->cfm_loc;
	return 0;
}

static inline int
unw_set_cfm (struct unw_frame_info *info, unsigned long val)
{
	*(info)->cfm_loc = val;
	return 0;
}

static inline int
unw_get_rp (struct unw_frame_info *info, unsigned long *val)
{
	if (!info->rp_loc)
		return -1;
	*val = *info->rp_loc;
	return 0;
}

extern int unw_access_gr (struct unw_frame_info *, int, unsigned long *, char *, int);
extern int unw_access_br (struct unw_frame_info *, int, unsigned long *, int);
extern int unw_access_fr (struct unw_frame_info *, int, struct ia64_fpreg *, int);
extern int unw_access_ar (struct unw_frame_info *, int, unsigned long *, int);
extern int unw_access_pr (struct unw_frame_info *, unsigned long *, int);

static inline int
unw_set_gr (struct unw_frame_info *i, int n, unsigned long v, char nat)
{
	return unw_access_gr(i, n, &v, &nat, 1);
}

static inline int
unw_set_br (struct unw_frame_info *i, int n, unsigned long v)
{
	return unw_access_br(i, n, &v, 1);
}

static inline int
unw_set_fr (struct unw_frame_info *i, int n, struct ia64_fpreg v)
{
	return unw_access_fr(i, n, &v, 1);
}

static inline int
unw_set_ar (struct unw_frame_info *i, int n, unsigned long v)
{
	return unw_access_ar(i, n, &v, 1);
}

static inline int
unw_set_pr (struct unw_frame_info *i, unsigned long v)
{
	return unw_access_pr(i, &v, 1);
}

#define unw_get_gr(i,n,v,nat)	unw_access_gr(i,n,v,nat,0)
#define unw_get_br(i,n,v)	unw_access_br(i,n,v,0)
#define unw_get_fr(i,n,v)	unw_access_fr(i,n,v,0)
#define unw_get_ar(i,n,v)	unw_access_ar(i,n,v,0)
#define unw_get_pr(i,v)		unw_access_pr(i,v,0)

#endif /* _ASM_UNWIND_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      #ifndef _ASM_IA64_USER_H
#define _ASM_IA64_USER_H

/*
 * Core file format: The core file is written in such a way that gdb
 * can understand it and provide useful information to the user (under
 * linux we use the `trad-core' bfd).  The file contents are as
 * follows:
 *
 *  upage: 1 page consisting of a user struct that tells gdb
 *	what is present in the file.  Directly after this is a
 *	copy of the task_struct, which is currently not used by gdb,
 *	but it may come in handy at some point.  All of the registers
 *	are stored as part of the upage.  The upage should always be
 *	only one page long.
 *  data: The data segment follows next.  We use current->end_text to
 *	current->brk to pick up all of the user variables, plus any memory
 *	that may have been sbrk'ed.  No attempt is made to determine if a
 *	page is demand-zero or if a page is totally unused, we just cover
 *	the entire range.  All of the addresses are rounded in such a way
 *	that an integral number of pages is written.
 *  stack: We need the stack information in order to get a meaningful
 *	backtrace.  We need to write the data from usp to
 *	current->start_stack, so we round each of these in order to be able
 *	to write an integer number of pages.
 *
 * Modified 1998, 1999, 2001
 *	David Mosberger-Tang <davidm@hpl.hp.com>, Hewlett-Packard Co
 */

#include <linux/ptrace.h>
#include <linux/types.h>

#include <asm/page.h>

#define EF_SIZE		3072	/* XXX fix me */

struct user {
	unsigned long	regs[EF_SIZE/8+32];	/* integer and fp regs */
	size_t		u_tsize;		/* text size (pages) */
	size_t		u_dsize;		/* data size (pages) */
	size_t		u_ssize;		/* stack size (pages) */
	unsigned long	start_code;		/* text starting address */
	unsigned long	start_data;		/* data starting address */
	unsigned long	start_stack;		/* stack starting address */
	long int	signal;			/* signal causing core dump */
	unsigned long	u_ar0;			/* help gdb find registers */
	unsigned long	magic;			/* identifies a core file */
	char		u_comm[32];		/* user command name */
};

#define NBPG			PAGE_SIZE
#define UPAGES			1
#define HOST_TEXT_START_ADDR	(u.start_code)
#define HOST_DATA_START_ADDR	(u.start_data)
#define HOST_STACK_END_ADDR	(u.start_stack + u.u_ssize * NBPG)

#endif /* _ASM_IA64_USER_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            #ifndef _ASM_IA64_USTACK_H
#define _ASM_IA64_USTACK_H

#include <asm/page.h>
#include <uapi/asm/ustack.h>

/* The absolute hard limit for stack size is 1/2 of the mappable space in the region */
#define MAX_USER_STACK_SIZE	(RGN_MAP_LIMIT/2)
#define STACK_TOP		(0x6000000000000000UL + RGN_MAP_LIMIT)
#define STACK_TOP_MAX		STACK_TOP
#endif /* _ASM_IA64_USTACK_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #ifndef _ASM_IA64_UV_UV_H
#define _ASM_IA64_UV_UV_H

#include <asm/sn/simulator.h>

static inline int is_uv_system(void)
{
	/* temporary support for running on hardware simulator */
	return IS_MEDUSA() || ia64_platform_is("uv");
}

#endif	/* _ASM_IA64_UV_UV_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * SGI UV architectural definitions
 *
 * Copyright (C) 2008 Silicon Graphics, Inc. All rights reserved.
 */

#ifndef __ASM_IA64_UV_HUB_H__
#define __ASM_IA64_UV_HUB_H__

#include <linux/numa.h>
#include <linux/percpu.h>
#include <asm/types.h>
#include <asm/percpu.h>


/*
 * Addressing Terminology
 *
 *	M       - The low M bits of a physical address represent the offset
 *		  into the blade local memory. RAM memory on a blade is physically
 *		  contiguous (although various IO spaces may punch holes in
 *		  it)..
 *
 * 	N	- Number of bits in the node portion of a socket physical
 * 		  address.
 *
 * 	NASID   - network ID of a router, Mbrick or Cbrick. Nasid values of
 * 	 	  routers always have low bit of 1, C/MBricks have low bit
 * 		  equal to 0. Most addressing macros that target UV hub chips
 * 		  right shift the NASID by 1 to exclude the always-zero bit.
 * 		  NASIDs contain up to 15 bits.
 *
 *	GNODE   - NASID right shifted by 1 bit. Most mmrs contain gnodes instead
 *		  of nasids.
 *
 * 	PNODE   - the low N bits of the GNODE. The PNODE is the most useful variant
 * 		  of the nasid for socket usage.
 *
 *
 *  NumaLink Global Physical Address Format:
 *  +--------------------------------+---------------------+
 *  |00..000|      GNODE             |      NodeOffset     |
 *  +--------------------------------+---------------------+
 *          |<-------53 - M bits --->|<--------M bits ----->
 *
 *	M - number of node offset bits (35 .. 40)
 *
 *
 *  Memory/UV-HUB Processor Socket Address Format:
 *  +----------------+---------------+---------------------+
 *  |00..000000000000|   PNODE       |      NodeOffset     |
 *  +----------------+---------------+---------------------+
 *                   <--- N bits --->|<--------M bits ----->
 *
 *	M - number of node offset bits (35 .. 40)
 *	N - number of PNODE bits (0 .. 10)
 *
 *		Note: M + N cannot currently exceed 44 (x86_64) or 46 (IA64).
 *		The actual values are configuration dependent and are set at
 *		boot time. M & N values are set by the hardware/BIOS at boot.
 */


/*
 * Maximum number of bricks in all partitions and in all coherency domains.
 * This is the total number of bricks accessible in the numalink fabric. It
 * includes all C & M bricks. Routers are NOT included.
 *
 * This value is also the value of the maximum number of non-router NASIDs
 * in the numalink fabric.
 *
 * NOTE: a brick may contain 1 or 2 OS nodes. Don't get these confused.
 */
#define UV_MAX_NUMALINK_BLADES	16384

/*
 * Maximum number of C/Mbricks within a software SSI (hardware may support
 * more).
 */
#define UV_MAX_SSI_BLADES	1

/*
 * The largest possible NASID of a C or M brick (+ 2)
 */
#define UV_MAX_NASID_VALUE	(UV_MAX_NUMALINK_NODES * 2)

/*
 * The following defines attributes of the HUB chip. These attributes are
 * frequently referenced and are kept in the per-cpu data areas of each cpu.
 * They are kept together in a struct to minimize cache misses.
 */
struct uv_hub_info_s {
	unsigned long	global_mmr_base;
	unsigned long	gpa_mask;
	unsigned long	gnode_upper;
	unsigned long	lowmem_remap_top;
	unsigned long	lowmem_remap_base;
	unsigned short	pnode;
	unsigned short	pnode_mask;
	unsigned short	coherency_domain_number;
	unsigned short	numa_blade_id;
	unsigned char	blade_processor_id;
	unsigned char	m_val;
	unsigned char	n_val;
};
DECLARE_PER_CPU(struct uv_hub_info_s, __uv_hub_info);
#define uv_hub_info 		this_cpu_ptr(&__uv_hub_info)
#define uv_cpu_hub_info(cpu)	(&per_cpu(__uv_hub_info, cpu))

/*
 * Local & Global MMR space macros.
 * 	Note: macros are intended to be used ONLY by inline functions
 * 	in this file - not by other kernel code.
 * 		n -  NASID (full 15-bit global nasid)
 * 		g -  GNODE (full 15-bit global nasid, right shifted 1)
 * 		p -  PNODE (local part of nsids, right shifted 1)
 */
#define UV_NASID_TO_PNODE(n)		(((n) >> 1) & uv_hub_info->pnode_mask)
#define UV_PNODE_TO_NASID(p)		(((p) << 1) | uv_hub_info->gnode_upper)

#define UV_LOCAL_MMR_BASE		0xf4000000UL
#define UV_GLOBAL_MMR32_BASE		0xf8000000UL
#define UV_GLOBAL_MMR64_BASE		(uv_hub_info->global_mmr_base)

#define UV_GLOBAL_MMR32_PNODE_SHIFT	15
#define UV_GLOBAL_MMR64_PNODE_SHIFT	26

#define UV_GLOBAL_MMR32_PNODE_BITS(p)	((p) << (UV_GLOBAL_MMR32_PNODE_SHIFT))

#define UV_GLOBAL_MMR64_PNODE_BITS(p)					\
	((unsigned long)(p) << UV_GLOBAL_MMR64_PNODE_SHIFT)

/*
 * Macros for converting between kernel virtual addresses, socket local physical
 * addresses, and UV global physical addresses.
 * 	Note: use the standard __pa() & __va() macros for converting
 * 	      between socket virtual and socket physical addresses.
 */

/* socket phys RAM --> UV global physical address */
static inline unsigned long uv_soc_phys_ram_to_gpa(unsigned long paddr)
{
	if (paddr < uv_hub_info->lowmem_remap_top)
		paddr += uv_hub_info->lowmem_remap_base;
	return paddr | uv_hub_info->gnode_upper;
}


/* socket virtual --> UV global physical address */
static inline unsigned long uv_gpa(void *v)
{
	return __pa(v) | uv_hub_info->gnode_upper;
}

/* socket virtual --> UV global physical address */
static inline void *uv_vgpa(void *v)
{
	return (void *)uv_gpa(v);
}

/* UV global physical address --> socket virtual */
static inline void *uv_va(unsigned long gpa)
{
	return __va(gpa & uv_hub_info->gpa_mask);
}

/* pnode, offset --> socket virtual */
static inline void *uv_pnode_offset_to_vaddr(int pnode, unsigned long offset)
{
	return __va(((unsigned long)pnode << uv_hub_info->m_val) | offset);
}


/*
 * Access global MMRs using the low memory MMR32 space. This region supports
 * faster MMR access but not all MMRs are accessible in this space.
 */
static inline unsigned long *uv_global_mmr32_address(int pnode,
				unsigned long offset)
{
	return __va(UV_GLOBAL_MMR32_BASE |
		       UV_GLOBAL_MMR32_PNODE_BITS(pnode) | offset);
}

static inline void uv_write_global_mmr32(int pnode, unsigned long offset,
				 unsigned long val)
{
	*uv_global_mmr32_address(pnode, offset) = val;
}

static inline unsigned long uv_read_global_mmr32(int pnode,
						 unsigned long offset)
{
	return *uv_global_mmr32_address(pnode, offset);
}

/*
 * Access Global MMR space using the MMR space located at the top of physical
 * memory.
 */
static inline unsigned long *uv_global_mmr64_address(int pnode,
				unsigned long offset)
{
	return __va(UV_GLOBAL_MMR64_BASE |
		    UV_GLOBAL_MMR64_PNODE_BITS(pnode) | offset);
}

static inline void uv_write_global_mmr64(int pnode, unsigned long offset,
				unsigned long val)
{
	*uv_global_mmr64_address(pnode, offset) = val;
}

static inline unsigned long uv_read_global_mmr64(int pnode,
						 unsigned long offset)
{
	return *uv_global_mmr64_address(pnode, offset);
}

/*
 * Access hub local MMRs. Faster than using global space but only local MMRs
 * are accessible.
 */
static inline unsigned long *uv_local_mmr_address(unsigned long offset)
{
	return __va(UV_LOCAL_MMR_BASE | offset);
}

static inline unsigned long uv_read_local_mmr(unsigned long offset)
{
	return *uv_local_mmr_address(offset);
}

static inline void uv_write_local_mmr(unsigned long offset, unsigned long val)
{
	*uv_local_mmr_address(offset) = val;
}

/*
 * Structures and definitions for converting between cpu, node, pnode, and blade
 * numbers.
 */

/* Blade-local cpu number of current cpu. Numbered 0 .. <# cpus on the blade> */
static inline int uv_blade_processor_id(void)
{
	return smp_processor_id();
}

/* Blade number of current cpu. Numnbered 0 .. <#blades -1> */
static inline int uv_numa_blade_id(void)
{
	return 0;
}

/* Convert a cpu number to the the UV blade number */
static inline int uv_cpu_to_blade_id(int cpu)
{
	return 0;
}

/* Convert linux node number to the UV blade number */
static inline int uv_node_to_blade_id(int nid)
{
	return 0;
}

/* Convert a blade id to the PNODE of the blade */
static inline int uv_blade_to_pnode(int bid)
{
	return 0;
}

/* Determine the number of possible cpus on a blade */
static inline int uv_blade_nr_possible_cpus(int bid)
{
	return num_possible_cpus();
}

/* Determine the number of online cpus on a blade */
static inline int uv_blade_nr_online_cpus(int bid)
{
	return num_online_cpus();
}

/* Convert a cpu id to the PNODE of the blade containing the cpu */
static inline int uv_cpu_to_pnode(int cpu)
{
	return 0;
}

/* Convert a linux node number to the PNODE of the blade */
static inline int uv_node_to_pnode(int nid)
{
	return 0;
}

/* Maximum possible number of blades */
static inline int uv_num_possible_blades(void)
{
	return 1;
}

static inline void uv_hub_send_ipi(int pnode, int apicid, int vector)
{
	/* not currently needed on ia64 */
}


#endif /* __ASM_IA64_UV_HUB__ */

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * SGI UV MMR definitions
 *
 * Copyright (C) 2007-2008 Silicon Graphics, Inc. All rights reserved.
 */

#ifndef _ASM_IA64_UV_UV_MMRS_H
#define _ASM_IA64_UV_UV_MMRS_H

#define UV_MMR_ENABLE		(1UL << 63)

/* ========================================================================= */
