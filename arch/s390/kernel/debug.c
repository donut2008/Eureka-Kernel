/*
 * Copyright (C) 2002-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */
#ifndef _ASM_IA64_THREAD_INFO_H
#define _ASM_IA64_THREAD_INFO_H

#ifndef ASM_OFFSETS_C
#include <asm/asm-offsets.h>
#endif
#include <asm/processor.h>
#include <asm/ptrace.h>

#ifndef __ASSEMBLY__

/*
 * On IA-64, we want to keep the task structure and kernel stack together, so they can be
 * mapped by a single TLB entry and so they can be addressed by the "current" pointer
 * without having to do pointer masking.
 */
struct thread_info {
	struct task_struct *task;	/* XXX not really needed, except for dup_task_struct() */
	__u32 flags;			/* thread_info flags (see TIF_*) */
	__u32 cpu;			/* current CPU */
	__u32 last_cpu;			/* Last CPU thread ran on */
	__u32 status;			/* Thread synchronous flags */
	mm_segment_t addr_limit;	/* user-level address space limit */
	int preempt_count;		/* 0=premptable, <0=BUG; will also serve as bh-counter */
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
	__u64 ac_stamp;
	__u64 ac_leave;
	__u64 ac_stime;
	__u64 ac_utime;
#endif
};

#define THREAD_SIZE			KERNEL_STACK_SIZE

#define INIT_THREAD_INFO(tsk)			\
{						\
	.task		= &tsk,			\
	.flags		= 0,			\
	.cpu		= 0,			\
	.addr_limit	= KERNEL_DS,		\
	.preempt_count	= INIT_PREEMPT_COUNT,	\
}

#ifndef ASM_OFFSETS_C
/* how to get the thread information struct from C */
#define current_thread_info()	((struct thread_info *) ((char *) current + IA64_TASK_SIZE))
#define alloc_thread_stack_node(tsk, node)	\
		((unsigned long *) ((char *) (tsk) + IA64_TASK_SIZE))
#define task_thread_info(tsk)	((struct thread_info *) ((char *) (tsk) + IA64_TASK_SIZE))
#else
#define current_thread_info()	((struct thread_info *) 0)
#define alloc_thread_stack_node(tsk, node)	((unsigned long *) 0)
#define task_thread_info(tsk)	((struct thread_info *) 0)
#endif
#define free_thread_stack(ti)	/* nothing */
#define task_stack_page(tsk)	((void *)(tsk))

#define __HAVE_THREAD_FUNCTIONS
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
#define setup_thread_stack(p, org)			\
	*task_thread_info(p) = *task_thread_info(org);	\
	task_thread_info(p)->ac_stime = 0;		\
	task_thread_info(p)->ac_utime = 0;		\
	task_thread_info(p)->task = (p);
#else
#define setup_thread_stack(p, org) \
	*task_thread_info(p) = *task_thread_info(org); \
	task_thread_info(p)->task = (p);
#endif
#define end_of_stack(p) (unsigned long *)((void *)(p) + IA64_RBS_OFFSET)

#define alloc_task_struct_node(node)						\
({										\
	struct page *page = alloc_pages_node(node, GFP_KERNEL | __GFP_COMP,	\
					     KERNEL_STACK_SIZE_ORDER);		\
	struct task_struct *ret = page ? page_address(page) : NULL;		\
										\
	ret;									\
})
#define free_task_struct(tsk)	free_pages((unsigned long) (tsk), KERNEL_STACK_SIZE_ORDER)

#endif /* !__ASSEMBLY */

/*
 * thread information flags
 * - these are process state flags that various assembly files may need to access
 * - pending work-to-be-done flags are in least-significant 16 bits, other flags
 *   in top 16 bits
 */
#define TIF_SIGPENDING		0	/* signal pending */
#define TIF_NEED_RESCHED	1	/* rescheduling necessary */
#define TIF_SYSCALL_TRACE	2	/* syscall trace active */
#define TIF_SYSCALL_AUDIT	3	/* syscall auditing active */
#define TIF_SINGLESTEP		4	/* restore singlestep on return to user mode */
#define TIF_NOTIFY_RESUME	6	/* resumption notification requested */
#define TIF_MEMDIE		17	/* is terminating due to OOM killer */
#define TIF_MCA_INIT		18	/* this task is processing MCA or INIT */
#define TIF_DB_DISABLED		19	/* debug trap disabled for fsyscall */
#define TIF_RESTORE_RSE		21	/* user RBS is newer than kernel RBS */
#define TIF_POLLING_NRFLAG	22	/* idle is polling for TIF_NEED_RESCHED */

#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_SYSCALL_AUDIT	(1 << TIF_SYSCALL_AUDIT)
#define _TIF_SINGLESTEP		(1 << TIF_SINGLESTEP)
#define _TIF_SYSCALL_TRACEAUDIT	(_TIF_SYSCALL_TRACE|_TIF_SYSCALL_AUDIT|_TIF_SINGLESTEP)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
#define _TIF_MCA_INIT		(1 << TIF_MCA_INIT)
#define _TIF_DB_DISABLED	(1 << TIF_DB_DISABLED)
#define _TIF_RESTORE_RSE	(1 << TIF_RESTORE_RSE)
#define _TIF_POLLING_NRFLAG	(1 << TIF_POLLING_NRFLAG)

/* "work to do on user-return" bits */
#define TIF_ALLWORK_MASK	(_TIF_SIGPENDING|_TIF_NOTIFY_RESUME|_TIF_SYSCALL_AUDIT|\
				 _TIF_NEED_RESCHED|_TIF_SYSCALL_TRACE)
/* like TIF_ALLWORK_BITS but sans TIF_SYSCALL_TRACE or TIF_SYSCALL_AUDIT */
#define TIF_WORK_MASK		(TIF_ALLWORK_MASK&~(_TIF_SYSCALL_TRACE|_TIF_SYSCALL_AUDIT))

#define TS_RESTORE_SIGMASK	2	/* restore signal mask in do_signal() */

#ifndef __ASSEMBLY__
#define HAVE_SET_RESTORE_SIGMASK	1
static inline void set_restore_sigmask(void)
{
	struct thread_info *ti = current_thread_info();
	ti->status |= TS_RESTORE_SIGMASK;
	WARN_ON(!test_bit(TIF_SIGPENDING, &ti->flags));
}
static inline void clear_restore_sigmask(void)
{
	current_thread_info()->status &= ~TS_RESTORE_SIGMASK;
}
static inline bool test_restore_sigmask(void)
{
	return current_thread_info()->status & TS_RESTORE_SIGMASK;
}
static inline bool test_and_clear_restore_sigmask(void)
{
	struct thread_info *ti = current_thread_info();
	if (!(ti->status & TS_RESTORE_SIGMASK))
		return false;
	ti->status &= ~TS_RESTORE_SIGMASK;
	return true;
}
#endif	/* !__ASSEMBLY__ */

#endif /* _ASM_IA64_THREAD_INFO_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      #ifndef _ASM_IA64_TIMEX_H
#define _ASM_IA64_TIMEX_H

/*
 * Copyright (C) 1998-2001, 2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */
/*
 * 2001/01/18 davidm	Removed CLOCK_TICK_RATE.  It makes no sense on IA-64.
 *			Also removed cacheflush_time as it's entirely unused.
 */

#include <asm/intrinsics.h>
#include <asm/processor.h>

typedef unsigned long cycles_t;

extern void (*ia64_udelay)(unsigned long usecs);

/*
 * For performance reasons, we don't want to define CLOCK_TICK_TRATE as
 * local_cpu_data->itc_rate.  Fortunately, we don't have to, either: according to George
 * Anzinger, 1/CLOCK_TICK_RATE is taken as the resolution of the timer clock.  The time
 * calculation assumes that you will use enough of these so that your tick size <= 1/HZ.
 * If the calculation shows that your CLOCK_TICK_RATE can not supply exactly 1/HZ ticks,
 * the actual value is calculated and used to update the wall clock each jiffie.  Setting
 * the CLOCK_TICK_RATE to x*HZ insures that the calculation will find no errors.  Hence we
 * pick a multiple of HZ which gives us a (totally virtual) CLOCK_TICK_RATE of about
 * 100MHz.
 */
#define CLOCK_TICK_RATE		(HZ * 100000UL)

static inline cycles_t
get_cycles (void)
{
	cycles_t ret;

	ret = ia64_getreg(_IA64_REG_AR_ITC);
	return ret;
}

extern void ia64_cpu_local_tick (void);
extern unsigned long long ia64_native_sched_clock (void);

#endif /* _ASM_IA64_TIMEX_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       #ifndef _ASM_IA64_TLB_H
#define _ASM_IA64_TLB_H
/*
 * Based on <asm-generic/tlb.h>.
 *
 * Copyright (C) 2002-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */
/*
 * Removing a translation from a page table (including TLB-shootdown) is a four-step
 * procedure:
 *
 *	(1) Flush (virtual) caches --- ensures virtual memory is coherent with kernel memory
 *	    (this is a no-op on ia64).
 *	(2) Clear the relevant portions of the page-table
 *	(3) Flush the TLBs --- ensures that stale content is gone from CPU TLBs
 *	(4) Release the pages that were freed up in step (2).
 *
 * Note that the ordering of these steps is crucial to avoid races on MP machines.
 *
 * The Linux kernel defines several platform-specific hooks for TLB-shootdown.  When
 * unmapping a portion of the virtual address space, these hooks are called according to
 * the following template:
 *
 *	tlb <- tlb_gather_mmu(mm, start, end);		// start unmap for address space MM
 *	{
 *	  for each vma that needs a shootdown do {
 *	    tlb_start_vma(tlb, vma);
 *	      for each page-table-entry PTE that needs to be removed do {
 *		tlb_remove_tlb_entry(tlb, pte, address);
 *		if (pte refers to a normal page) {
 *		  tlb_remove_page(tlb, page);
 *		}
 *	      }
 *	    tlb_end_vma(tlb, vma);
 *	  }
 *	}
 *	tlb_finish_mmu(tlb, start, end);	// finish unmap for address space MM
 */
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/swap.h>

#include <asm/pgalloc.h>
#include <asm/processor.h>
#include <asm/tlbflush.h>
#include <asm/machvec.h>

/*
 * If we can't allocate a page to make a big batch of page pointers
 * to work on, then just handle a few from the on-stack structure.
 */
#define	IA64_GATHER_BUNDLE	8

struct mmu_gather {
	struct mm_struct	*mm;
	unsigned int		nr;
	unsigned int		max;
	unsigned char		fullmm;		/* non-zero means full mm flush */
	unsigned char		need_flush;	/* really unmapped some PTEs? */
	unsigned long		start, end;
	unsigned long		start_addr;
	unsigned long		end_addr;
	struct page		**pages;
	struct page		*local[IA64_GATHER_BUNDLE];
};

struct ia64_tr_entry {
	u64 ifa;
	u64 itir;
	u64 pte;
	u64 rr;
}; /*Record for tr entry!*/

extern int ia64_itr_entry(u64 target_mask, u64 va, u64 pte, u64 log_size);
extern void ia64_ptr_entry(u64 target_mask, int slot);

extern struct ia64_tr_entry *ia64_idtrs[NR_CPUS];

/*
 region register macros
*/
#define RR_TO_VE(val)   (((val) >> 0) & 0x0000000000000001)
#define RR_VE(val)	(((val) & 0x0000000000000001) << 0)
#define RR_VE_MASK	0x0000000000000001L
#define RR_VE_SHIFT	0
#define RR_TO_PS(val)	(((val) >> 2) & 0x000000000000003f)
#define RR_PS(val)	(((val) & 0x000000000000003f) << 2)
#define RR_PS_MASK	0x00000000000000fcL
#define RR_PS_SHIFT	2
#define RR_RID_MASK	0x00000000ffffff00L
#define RR_TO_RID(val) 	((val >> 8) & 0xffffff)

static inline void
ia64_tlb_flush_mmu_tlbonly(struct mmu_gather *tlb, unsigned long start, unsigned long end)
{
	tlb->need_flush = 0;

	if (tlb->fullmm) {
		/*
		 * Tearing down the entire address space.  This happens both as a result
		 * of exit() and execve().  The latter case necessitates the call to
		 * flush_tlb_mm() here.
		 */
		flush_tlb_mm(tlb->mm);
	} else if (unlikely (end - start >= 1024*1024*1024*1024UL
			     || REGION_NUMBER(start) != REGION_NUMBER(end - 1)))
	{
		/*
		 * If we flush more than a tera-byte or across regions, we're probably
		 * better off just flushing the entire TLB(s).  This should be very rare
		 * and is not worth optimizing for.
		 */
		flush_tlb_all();
	} else {
		/*
		 * XXX fix me: flush_tlb_range() should take an mm pointer instead of a
		 * vma pointer.
		 */
		struct vm_area_struct vma;

		vma.vm_mm = tlb->mm;
		/* flush the address range from the tlb: */
		flush_tlb_range(&vma, start, end);
		/* now flush the virt. page-table area mapping the address range: */
		flush_tlb_range(&vma, ia64_thash(start), ia64_thash(end));
	}

}

static inline void
ia64_tlb_flush_mmu_free(struct mmu_gather *tlb)
{
	unsigned long i;
	unsigned int nr;

	/* lastly, release the freed pages */
	nr = tlb->nr;

	tlb->nr = 0;
	tlb->start_addr = ~0UL;
	for (i = 0; i < nr; ++i)
		free_page_and_swap_cache(tlb->pages[i]);
}

/*
 * Flush the TLB for address range START to END and, if not in fast mode, release the
 * freed pages that where gathered up to this point.
 */
static inline void
ia64_tlb_flush_mmu (struct mmu_gather *tlb, unsigned long start, unsigned long end)
{
	if (!tlb->need_flush)
		return;
	ia64_tlb_flush_mmu_tlbonly(tlb, start, end);
	ia64_tlb_flush_mmu_free(tlb);
}

static inline void __tlb_alloc_page(struct mmu_gather *tlb)
{
	unsigned long addr = __get_free_pages(GFP_NOWAIT | __GFP_NOWARN, 0);

	if (addr) {
		tlb->pages = (void *)addr;
		tlb->max = PAGE_SIZE / sizeof(void *);
	}
}


static inline void
tlb_gather_mmu(struct mmu_gather *tlb, struct mm_struct *mm, unsigned long start, unsigned long end)
{
	tlb->mm = mm;
	tlb->max = ARRAY_SIZE(tlb->local);
	tlb->pages = tlb->local;
	tlb->nr = 0;
	tlb->fullmm = !(start | (end+1));
	tlb->start = start;
	tlb->end = end;
	tlb->start_addr = ~0UL;
}

/*
 * Called at the end of the shootdown operation to free up any resources that were
 * collected.
 */
static inline void
tlb_finish_mmu(struct mmu_gather *tlb, unsigned long start, unsigned long end)
{
	/*
	 * Note: tlb->nr may be 0 at this point, so we can't rely on tlb->start_addr and
	 * tlb->end_addr.
	 */
	ia64_tlb_flush_mmu(tlb, start, end);

	/* keep the page table cache within bounds */
	check_pgt_cache();

	if (tlb->pages != tlb->local)
		free_pages((unsigned long)tlb->pages, 0);
}

/*
 * Logically, this routine frees PAGE.  On MP machines, the actual freeing of the page
 * must be delayed until after the TLB has been flushed (see comments at the beginning of
 * this file).
 */
static inline int __tlb_remove_page(struct mmu_gather *tlb, struct page *page)
{
	tlb->need_flush = 1;

	if (!tlb->nr && tlb->pages == tlb->local)
		__tlb_alloc_page(tlb);

	tlb->pages[tlb->nr++] = page;
	VM_BUG_ON(tlb->nr > tlb->max);

	return tlb->max - tlb->nr;
}

static inline void tlb_flush_mmu_tlbonly(struct mmu_gather *tlb)
{
	ia64_tlb_flush_mmu_tlbonly(tlb, tlb->start_addr, tlb->end_addr);
}

static inline void tlb_flush_mmu_free(struct mmu_gather *tlb)
{
	ia64_tlb_flush_mmu_free(tlb);
}

static inline void tlb_flush_mmu(struct mmu_gather *tlb)
{
	ia64_tlb_flush_mmu(tlb, tlb->start_addr, tlb->end_addr);
}

static inline void tlb_remove_page(struct mmu_gather *tlb, struct page *page)
{
	if (!__tlb_remove_page(tlb, page))
		tlb_flush_mmu(tlb);
}

/*
 * Remove TLB entry for PTE mapped at virtual address ADDRESS.  This is called for any
 * PTE, not just those pointing to (normal) physical memory.
 */
static inline void
__tlb_remove_tlb_entry (struct mmu_gather *tlb, pte_t *ptep, unsigned long address)
{
	if (tlb->start_addr == ~0UL)
		tlb->start_addr = address;
	tlb->end_addr = address + PAGE_SIZE;
}

static inline void
tlb_flush_pmd_range(struct mmu_gather *tlb, unsigned long address,
		    unsigned long size)
{
	if (tlb->start_addr > address)
		tlb->start_addr = address;
	if (tlb->end_addr < address + size)
		tlb->end_addr = address + size;
}

#define tlb_migrate_finish(mm)	platform_tlb_migrate_finish(mm)

#define tlb_start_vma(tlb, vma)			do { } while (0)
#define tlb_end_vma(tlb, vma)			do { } while (0)

#define tlb_remove_tlb_entry(tlb, ptep, addr)		\
do {							\
	tlb->need_flush = 1;				\
	__tlb_remove_tlb_entry(tlb, ptep, addr);	\
} while (0)

#define pte_free_tlb(tlb, ptep, address)		\
do {							\
	tlb->need_flush = 1;				\
	__pte_free_tlb(tlb, ptep, address);		\
} while (0)

#define pmd_free_tlb(tlb, ptep, address)		\
do {							\
	tlb->need_flush = 1;				\
	__pmd_free_tlb(tlb, ptep, address);		\
} while (0)

#define pud_free_tlb(tlb, pudp, address)		\
do {							\
	tlb->need_flush = 1;				\
	__pud_free_tlb(tlb, pudp, address);		\
} while (0)

#endif /* _ASM_IA64_TLB_H */
                                                                                                                                                                                                                                                                                                   #ifndef _ASM_IA64_TLBFLUSH_H
#define _ASM_IA64_TLBFLUSH_H

/*
 * Copyright (C) 2002 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */


#include <linux/mm.h>

#include <asm/intrinsics.h>
#include <asm/mmu_context.h>
#include <asm/page.h>

/*
 * Now for some TLB flushing routines.  This is the kind of stuff that
 * can be very expensive, so try to avoid them whenever possible.
 */
extern void setup_ptcg_sem(int max_purges, int from_palo);

/*
 * Flush everything (kernel mapping may also have changed due to
 * vmalloc/vfree).
 */
extern void local_flush_tlb_all (void);

#ifdef CONFIG_SMP
  extern void smp_flush_tlb_all (void);
  extern void smp_flush_tlb_mm (struct mm_struct *mm);
  extern void smp_flush_tlb_cpumask (cpumask_t xcpumask);
# define flush_tlb_all()	smp_flush_tlb_all()
#else
# define flush_tlb_all()	local_flush_tlb_all()
# define smp_flush_tlb_cpumask(m) local_flush_tlb_all()
#endif

static inline void
local_finish_flush_tlb_mm (struct mm_struct *mm)
{
	if (mm == current->active_mm)
		activate_context(mm);
}

/*
 * Flush a specified user mapping.  This is called, e.g., as a result of fork() and
 * exit().  fork() ends up here because the copy-on-write mechanism needs to write-protect
 * the PTEs of the parent task.
 */
static inline void
flush_tlb_mm (struct mm_struct *mm)
{
	if (!mm)
		return;

	set_bit(mm->context, ia64_ctx.flushmap);
	mm->context = 0;

	if (atomic_read(&mm->mm_users) == 0)
		return;		/* happens as a result of exit_mmap() */

#ifdef CONFIG_SMP
	smp_flush_tlb_mm(mm);
#else
	local_finish_flush_tlb_mm(mm);
#endif
}

extern void flush_tlb_range (struct vm_area_struct *vma, unsigned long start, unsigned long end);

/*
 * Page-granular tlb flush.
 */
static inline void
flush_tlb_page (struct vm_area_struct *vma, unsigned long addr)
{
#ifdef CONFIG_SMP
	flush_tlb_range(vma, (addr & PAGE_MASK), (addr & PAGE_MASK) + PAGE_SIZE);
#else
	if (vma->vm_mm == current->active_mm)
		ia64_ptcl(addr, (PAGE_SHIFT << 2));
	else
		vma->vm_mm->context = 0;
#endif
}

/*
 * Flush the local TLB. Invoked from another cpu using an IPI.
 */
#ifdef CONFIG_SMP
void smp_local_flush_tlb(void);
#else
#define smp_local_flush_tlb()
#endif

static inline void flush_tlb_kernel_range(unsigned long start,
					  unsigned long end)
{
	flush_tlb_all();	/* XXX fix me */
}

#endif /* _ASM_IA64_TLBFLUSH_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Copyright (C) 2002, Erich Focht, NEC
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef _ASM_IA64_TOPOLOGY_H
#define _ASM_IA64_TOPOLOGY_H

#include <asm/acpi.h>
#include <asm/numa.h>
#include <asm/smp.h>

#ifdef CONFIG_NUMA

/* Nodes w/o CPUs are preferred for memory allocations, see build_zonelists */
#define PENALTY_FOR_NODE_WITH_CPUS 255

/*
 * Nodes within this distance are eligible for reclaim by zone_reclaim() when
 * zone_reclaim_mode is enabled.
 */
#define RECLAIM_DISTANCE 15

/*
 * Returns a bitmask of CPUs on Node 'node'.
 */
#define cpumask_of_node(node) ((node) == -1 ?				\
			       cpu_all_mask :				\
			       &node_to_cpu_mask[node])

/*
 * Returns the number of the node containing Node 'nid'.
 * Not implemented here. Multi-level hierarchies detected with
 * the help of node_distance().
 */
#define parent_node(nid) (nid)

/*
 * Determines the node for a given pci bus
 */
#define pcibus_to_node(bus) PCI_CONTROLLER(bus)->node

void build_cpu_to_node_map(void);

#endif /* CONFIG_NUMA */

#ifdef CONFIG_SMP
#define topology_physical_package_id(cpu)	(cpu_data(cpu)->socket_id)
#define topology_core_id(cpu)			(cpu_data(cpu)->core_id)
#define topology_core_cpumask(cpu)		(&cpu_core_map[cpu])
#define topology_sibling_cpumask(cpu)		(&per_cpu(cpu_sibling_map, cpu))
#endif

extern void arch_fix_phys_package_id(int num, u32 slot);

#define cpumask_of_pcibus(bus)	(pcibus_to_node(bus) == -1 ?		\
				 cpu_all_mask :				\
				 cpumask_of_node(pcibus_to_node(bus)))

#include <asm-generic/topology.h>

#endif /* _ASM_IA64_TOPOLOGY_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * This file is never included by application software unless explicitly
 * requested (e.g., via linux/types.h) in which case the application is
 * Linux specific so (user-) name space pollution is not a major issue.
 * However, for interoperability, libraries still need to be careful to
 * avoid naming clashes.
 *
 * Based on <asm-alpha/types.h>.
 *
 * Modified 1998-2000, 2002
 *	David Mosberger-Tang <davidm@hpl.hp.com>, Hewlett-Packard Co
 */
#ifndef _ASM_IA64_TYPES_H
#define _ASM_IA64_TYPES_H

#include <asm-generic/int-ll64.h>
#include <uapi/asm/types.h>

#ifdef __ASSEMBLY__
#else
/*
 * These aren't exported outside the kernel to avoid name space clashes
 */

struct fnptr {
	unsigned long ip;
	unsigned long gp;
};

#endif /* !__ASSEMBLY__ */
#endif /* _ASM_IA64_TYPES_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           #ifndef _ASM_IA64_UACCESS_H
#define _ASM_IA64_UACCESS_H

/*
 * This file defines various macros to transfer memory areas across
 * the user/kernel boundary.  This needs to be done carefully because
 * this code is executed in kernel mode and uses user-specified
 * addresses.  Thus, we need to be careful not to let the user to
 * trick us into accessing kernel memory that would normally be
 * inaccessible.  This code is also fairly performance sensitive,
 * so we want to spend as little time doing safety checks as
 * possible.
 *
 * To make matters a bit more interesting, these macros sometimes also
 * called from within the kernel itself, in which case the address
 * validity check must be skipped.  The get_fs() macro tells us what
 * to do: if get_fs()==USER_DS, checking is performed, if
 * get_fs()==KERNEL_DS, checking is bypassed.
 *
 * Note that even if the memory area specified by the user is in a
 * valid address range, it is still possible that we'll get a page
 * fault while accessing it.  This is handled by filling out an
 * exception handler fixup entry for each instruction that has the
 * potential to fault.  When such a fault occurs, the page fault
 * handler checks to see whether the faulting instruction has a fixup
 * associated and, if so, sets r8 to -EFAULT and clears r9 to 0 and
 * then resumes execution at the continuation point.
 *
 * Based on <asm-alpha/uaccess.h>.
 *
 * Copyright (C) 1998, 1999, 2001-2004 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */

#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/page-flags.h>
#include <linux/mm.h>

#include <asm/intrinsics.h>
#include <asm/pgtable.h>
#include <asm/io.h>

/*
 * For historical reasons, the following macros are grossly misnamed:
 */
#define KERNEL_DS	((mm_segment_t) { ~0UL })		/* cf. access_ok() */
#define USER_DS		((mm_segment_t) { TASK_SIZE-1 })	/* cf. access_ok() */

#define VERIFY_READ	0
#define VERIFY_WRITE	1

#define get_ds()  (KERNEL_DS)
#define get_fs()  (current_thread_info()->addr_limit)
#define set_fs(x) (current_thread_info()->addr_limit = (x))

#define segment_eq(a, b)	((a).seg == (b).seg)

/*
 * When accessing user memory, we need to make sure the entire area really is in
 * user-level space.  In order to do this efficiently, we make sure that the page at
 * address TASK_SIZE is never valid.  We also need to make sure that the address doesn't
 * point inside the virtually mapped linear page table.
 */
#define __access_ok(addr, size, segment)						\
({											\
	__chk_user_ptr(addr);								\
	(likely((unsigned long) (addr) <= (segment).seg)				\
	 && ((segment).seg == KERNEL_DS.seg						\
	     || likely(REGION_OFFSET((unsigned long) (addr)) < RGN_MAP_LIMIT)));	\
})
#define access_ok(type, addr, size)	__access_ok((addr), (size), get_fs())

/*
 * These are the main single-value transfer routines.  They automatically
 * use the right size if we just have the right pointer type.
 *
 * Careful to not
 * (a) re-use the arguments for side effects (sizeof/typeof is ok)
 * (b) require any knowledge of processes at this stage
 */
#define put_user(x, ptr)	__put_user_check((__typeof__(*(ptr))) (x), (ptr), sizeof(*(ptr)), get_fs())
#define get_user(x, ptr)	__get_user_check((x), (ptr), sizeof(*(ptr)), get_fs())

/*
 * The "__xxx" versions do not do address space checking, useful when
 * doing multiple accesses t