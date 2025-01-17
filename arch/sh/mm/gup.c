ne REGION_NUMBER(x)	({ia64_va _v; _v.l = (long) (x); _v.f.reg;})
#define REGION_OFFSET(x)	({ia64_va _v; _v.l = (long) (x); _v.f.off;})

#ifdef CONFIG_HUGETLB_PAGE
# define htlbpage_to_page(x)	(((unsigned long) REGION_NUMBER(x) << 61)			\
				 | (REGION_OFFSET(x) >> (HPAGE_SHIFT-PAGE_SHIFT)))
# define HUGETLB_PAGE_ORDER	(HPAGE_SHIFT - PAGE_SHIFT)
extern unsigned int hpage_shift;
#endif

static __inline__ int
get_order (unsigned long size)
{
	long double d = size - 1;
	long order;

	order = ia64_getf_exp(d);
	order = order - PAGE_SHIFT - 0xffff + 1;
	if (order < 0)
		order = 0;
	return order;
}

#endif /* !__ASSEMBLY__ */

#ifdef STRICT_MM_TYPECHECKS
  /*
   * These are used to make use of C type-checking..
   */
  typedef struct { unsigned long pte; } pte_t;
  typedef struct { unsigned long pmd; } pmd_t;
#if CONFIG_PGTABLE_LEVELS == 4
  typedef struct { unsigned long pud; } pud_t;
#endif
  typedef struct { unsigned long pgd; } pgd_t;
  typedef struct { unsigned long pgprot; } pgprot_t;
  typedef struct page *pgtable_t;

# define pte_val(x)	((x).pte)
# define pmd_val(x)	((x).pmd)
#if CONFIG_PGTABLE_LEVELS == 4
# define pud_val(x)	((x).pud)
#endif
# define pgd_val(x)	((x).pgd)
# define pgprot_val(x)	((x).pgprot)

# define __pte(x)	((pte_t) { (x) } )
# define __pmd(x)	((pmd_t) { (x) } )
# define __pgprot(x)	((pgprot_t) { (x) } )

#else /* !STRICT_MM_TYPECHECKS */
  /*
   * .. while these make it easier on the compiler
   */
# ifndef __ASSEMBLY__
    typedef unsigned long pte_t;
    typedef unsigned long pmd_t;
    typedef unsigned long pgd_t;
    typedef unsigned long pgprot_t;
    typedef struct page *pgtable_t;
# endif

# define pte_val(x)	(x)
# define pmd_val(x)	(x)
# define pgd_val(x)	(x)
# define pgprot_val(x)	(x)

# define __pte(x)	(x)
# define __pgd(x)	(x)
# define __pgprot(x)	(x)
#endif /* !STRICT_MM_TYPECHECKS */

#define PAGE_OFFSET			RGN_BASE(RGN_KERNEL)

#define VM_DATA_DEFAULT_FLAGS		(VM_READ | VM_WRITE |					\
					 VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC |		\
					 (((current->personality & READ_IMPLIES_EXEC) != 0)	\
					  ? VM_EXEC : 0))

#define GATE_ADDR		RGN_BASE(RGN_GATE)

/*
 * 0xa000000000000000+2*PERCPU_PAGE_SIZE
 * - 0xa000000000000000+3*PERCPU_PAGE_SIZE remain unmapped (guard page)
 */
#define KERNEL_START		 (GATE_ADDR+__IA64_UL_CONST(0x100000000))
#define PERCPU_ADDR		(-PERCPU_PAGE_SIZE)
#define LOAD_OFFSET		(KERNEL_START - KERNEL_TR_PAGE_SIZE)

#define __HAVE_ARCH_GATE_AREA	1

#endif /* _ASM_IA64_PAGE_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        #ifndef _ASM_IA64_PAL_H
#define _ASM_IA64_PAL_H

/*
 * Processor Abstraction Layer definitions.
 *
 * This is based on Intel IA-64 Architecture Software Developer's Manual rev 1.0
 * chapter 11 IA-64 Processor Abstraction Layer
 *
 * Copyright (C) 1998-2001 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 * Copyright (C) 1999 Srinivasa Prasad Thirumalachar <sprasad@sprasad.engr.sgi.com>
 * Copyright (C) 2008 Silicon Graphics, Inc. (SGI)
 *
 * 99/10/01	davidm	Make sure we pass zero for reserved parameters.
 * 00/03/07	davidm	Updated pal_cache_flush() to be in sync with PAL v2.6.
 * 00/03/23     cfleck  Modified processor min-state save area to match updated PAL & SAL info
 * 00/05/24     eranian Updated to latest PAL spec, fix structures bugs, added
 * 00/05/25	eranian Support for stack calls, and static physical calls
 * 00/06/18	eranian Support for stacked physical calls
 * 06/10/26	rja	Support for Intel Itanium Architecture Software Developer's
 *			Manual Rev 2.2 (Jan 2006)
 */

/*
 * Note that some of these calls use a static-register only calling
 * convention which has nothing to do with the regular calling
 * convention.
 */
#define PAL_CACHE_FLUSH		1	/* flush i/d cache */
#define PAL_CACHE_INFO		2	/* get detailed i/d cache info */
#define PAL_CACHE_INIT		3	/* initialize i/d cache */
#define PAL_CACHE_SUMMARY	4	/* get summary of cache hierarchy */
#define PAL_MEM_ATTRIB		5	/* list supported memory attributes */
#define PAL_PTCE_INFO		6	/* purge TLB info */
#define PAL_VM_INFO		7	/* return supported virtual memory features */
#define PAL_VM_SUMMARY		8	/* return summary on supported vm features */
#define PAL_BUS_GET_FEATURES	9	/* return processor bus interface features settings */
#define PAL_BUS_SET_FEATURES	10	/* set processor bus features */
#define PAL_DEBUG_INFO		11	/* get number of debug registers */
#define PAL_FIXED_ADDR		12	/* get fixed component of processors's directed address */
#define PAL_FREQ_BASE		13	/* base frequency of the platform */
#define PAL_FREQ_RATIOS		14	/* ratio of processor, bus and ITC frequency */
#define PAL_PERF_MON_INFO	15	/* return performance monitor info */
#define PAL_PLATFORM_ADDR	16	/* set processor interrupt block and IO port space addr */
#define PAL_PROC_GET_FEATURES	17	/* get configurable processor features & settings */
#define PAL_PROC_SET_FEATURES	18	/* enable/disable configurable processor features */
#define PAL_RSE_INFO		19	/* return rse information */
#define PAL_VERSION		20	/* return version of PAL code */
#define PAL_MC_CLEAR_LOG	21	/* clear al