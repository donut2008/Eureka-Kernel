#ifndef _ASM_IA64_PAGE_H
#define _ASM_IA64_PAGE_H
/*
 * Pagetable related stuff.
 *
 * Copyright (C) 1998, 1999, 2002 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */

#include <asm/intrinsics.h>
#include <asm/types.h>

/*
 * The top three bits of an IA64 address are its Region Number.
 * Different regions are assigned to different purposes.
 */
#define RGN_SHIFT	(61)
#define RGN_BASE(r)	(__IA64_UL_CONST(r)<<RGN_SHIFT)
#define RGN_BITS	(RGN_BASE(-1))

#define RGN_KERNEL	7	/* Identity mapped region */
#define RGN_UNCACHED    6	/* Identity mapped I/O region */
#define RGN_GATE	5	/* Gate page, Kernel text, etc */
#define RGN_HPAGE	4	/* For Huge TLB pages */

/*
 * PAGE_SHIFT determines the actual kernel page size.
 */
#if defined(CONFIG_IA64_PAGE_SIZE_4KB)
# define PAGE_SHIFT	12
#elif defined(CONFIG_IA64_PAGE_SIZE_8KB)
# define PAGE_SHIFT	13
#elif defined(CONFIG_IA64_PAGE_SIZE_16KB)
# define PAGE_SHIFT	14
#elif defined(CONFIG_IA64_PAGE_SIZE_64KB)
# define PAGE_SHIFT	16
#else
# error Unsupported page size!
#endif

#define PAGE_SIZE		(__IA64_UL_CONST(1) << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE - 1))

#define PERCPU_PAGE_SHIFT	18	/* log2() of max. size of per-CPU area */
#define PERCPU_PAGE_SIZE	(__IA64_UL_CONST(1) << PERCPU_PAGE_SHIFT)


#ifdef CONFIG_HUGETLB_PAGE
# define HPAGE_REGION_BASE	RGN_BASE(RGN_HPAGE)
# define HPAGE_SHIFT		hpage_shift
# define HPAGE_SHIFT_DEFAULT	28	/* check ia64 SDM for architecture supported size */
# define HPAGE_SIZE		(__IA64_UL_CONST(1) << HPAGE_SHIFT)
# define HPAGE_MASK		(~(HPAGE_SIZE - 1))

# define HAVE_ARCH_HUGETLB_UNMAPPED_AREA
#endif /* CONFIG_HUGETLB_PAGE */

#ifdef __ASSEMBLY__
# define __pa(x)		((x) - PAGE_OFFSET)
# define __va(x)		((x) + PAGE_OFFSET)
#else /* !__ASSEMBLY */
#  define STRICT_MM_TYPECHECKS

extern void clear_page (void *page);
extern void copy_page (void *to, void *from);

/*
 * clear_user_page() and copy_user_page() can't be inline functions because
 * flush_dcache_page() can't be defined until later...
 */
#define clear_user_page(addr, vaddr, page)	\
do {						\
	clear_page(addr);			\
	flush_dcache_page(page);		\
} while (0)

#define copy_user_page(to, from, vaddr, page)	\
do {						\
	copy_page((to), (from));		\
	flush_dcache_page(page);		\
} while (0)


#define __alloc_zeroed_user_highpage(movableflags, vma, vaddr)		\
({									\
	struct page *page = alloc_page_vma(				\
		GFP_HIGHUSER | __GFP_ZERO | movableflags, vma, vaddr);	\
	if (page)							\
 		flush_dcache_page(page);				\
	page;								\
})

#define __HAVE_ARCH_ALLOC_ZEROED_USER_HIGHPAGE

#define virt_addr_valid(kaddr)	pfn_valid(__pa(kaddr) >> PAGE_SHIFT)

#ifdef CONFIG_VIRTUAL_MEM_MAP
extern int ia64_pfn_valid (unsigned long pfn);
#else
# define ia64_pfn_valid(pfn) 1
#endif

#ifdef CONFIG_VIRTUAL_MEM_MAP
extern struct page *vmem_map;
#ifdef CONFIG_DISCONTIGMEM
# define page_to_pfn(page)	((unsigned long) (page - vmem_map))
# define pfn_to_page(pfn)	(vmem_map + (pfn))
#else
# include <asm-generic/memory_model.h>
#endif
#else
# include <asm-generic/memory_model.h>
#endif

#ifdef CONFIG_FLATMEM
# define pfn_valid(pfn)		(((pfn) < max_mapnr) && ia64_pfn_valid(pfn))
#elif defined(CONFIG_DISCONTIGMEM)
extern unsigned long min_low_pfn;
extern unsigned long max_low_pfn;
# define pfn_valid(pfn)		(((pfn) >= min_low_pfn) && ((pfn) < max_low_pfn) && ia64_pfn_valid(pfn))
#endif

#define page_to_phys(page)	(page_to_pfn(page) << PAGE_SHIFT)
#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)
#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)

typedef union ia64_va {
	struct {
		unsigned long off : 61;		/* intra-region offset */
		unsigned long reg :  3;		/* region number */
	} f;
	unsigned long l;
	void *p;
} ia64_va;

/*
 * Note: These macros depend on the fact that PAGE_OFFSET has all
 * region bits set to 1 and all other bits set to zero.  They are
 * expressed in this way to ensure they result in a single "dep"
 * instruction.
 */
#define __pa(x)		({ia64_va _v; _v.l = (long) (x); _v.f.reg