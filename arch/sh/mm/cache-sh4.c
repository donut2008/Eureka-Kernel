he kernel
 *  flushes icache explicitly if necessary.
 */
#define pte_present_exec_user(pte)\
	((pte_val(pte) & (_PAGE_P | _PAGE_PL_MASK | _PAGE_AR_RX)) == \
		(_PAGE_P | _PAGE_PL_3 | _PAGE_AR_RX))

extern void __ia64_sync_icache_dcache(pte_t pteval);
static inline void set_pte(pte_t *ptep, pte_t pteval)
{
	/* page is present && page is user  && page is executable
	 * && (page swapin or new page or page migraton
	 *	|| copy_on_write with page copying.)
	 */
	if (pte_present_exec_user(pteval) &&
	    (!pte_present(*ptep) ||
		pte_pfn(*ptep) != pte_pfn(pteval)))
		/* load_module() calles flush_icache_range() explicitly*/
		__ia64_sync_icache_dcache(pteval);
	*ptep = pteval;
}

#define set_pte_at(mm,addr,ptep,pteval) set_pte(ptep,pteval)

/*
 * Make page protection values cacheable, uncacheable, or write-
 * combining.  Note that "protection" is really a misnomer here as the
 * protection value contains the memory attribute bits, dirty bits, and
 * various other bits as well.
 */
#define pgprot_cacheable(prot)		__pgprot((pgprot_val(prot) & ~_PAGE_MA_MASK) | _PAGE_MA_WB)
#define pgprot_noncached(prot)		__pgprot((pgprot_val(prot) & ~_PAGE_MA_MASK) | _PAGE_MA_UC)
#define pgprot_writecombine(prot)	__pgprot((pgprot_val(prot) & ~_PAGE_MA_MASK) | _PAGE_MA_WC)

struct file;
extern pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
				     unsigned long size, pgprot_t vma_prot);
#define __HAVE_PHYS_MEM_ACCESS_PROT

static inline unsigned long
pgd_index (unsigned long address)
{
	unsigned long region = address >> 61;
	unsigned long l1index = (address >> PGDIR_SHIFT) & ((PTRS_PER_PGD >> 3) - 1);

	return (region << (PAGE_SHIFT - 6)) | l1index;
}

/* The offset in the 1-level directory is given by the 3 region bits
   (61..63) and the level-1 bits.  */
static inline pgd_t*
pgd_offset (const struct mm_struct *mm, unsigned long address)
{
	return mm->pgd + pgd_index(address);
}

/* In the kernel's mapped region we completely ignore the region number
   (since we know it's in region number 5). */
#define pgd_offset_k(addr) \
	(init_mm.pgd + (((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1)))

/* Look up a pgd entry in the gate area.  On IA-64, the gate-area
   resides in the kernel-mapped segment, hence we use pgd_offset_k()
   here.  */
#define pgd_offset_gate(mm, addr)	pgd_offset_k(addr)

#if CONFIG_PGTABLE_LEVELS == 4
/* Find an entry in the second-level page table.. */
#define pud_offset(dir,addr) \
	((pud_t *) pgd_page_vaddr(*(dir)) + (((addr) >> PUD_SHIFT) & (PTRS_PER_PUD - 1)))
#endif

/* Find an entry in the third-level page table.. */
#define pmd_offset(dir,addr) \
	((pmd_t *) pud_page_vaddr(*(dir)) + (((addr) >> PMD_SHIFT) & (PTRS_PER_PMD - 1)))

/*
 * Find an entry in the third-level page table.  This looks more complicated than it
 * should be because some platforms place page tables in high memory.
 */
#define pte_index(addr)	 	(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
#define pte_offset_kernel(dir,addr)	((pte_t *) pmd_page_vaddr(*(dir)) + pte_index(addr))
#define pte_offset_map(dir,addr)	pte_offset_kernel(dir, addr)
#define pte_unmap(pte)			do { } while (0)

/* atomic versions of the some PTE manipulations: */

static inline int
ptep_test_and_clear_young (struct vm_area_struct *vma, unsigned long addr, pte_t *ptep)
{
#ifdef CONFIG_SMP
	if (!pte_young(*ptep))
		return 0;
	return test_and_clear_bit(_PAGE_A_BIT, ptep);
#else
	pte_t pte = *ptep;
	if (!pte_young(pte))
		return 0;
	set_pte_at(vma->vm_mm, addr, ptep, pte_mkold(pte));
	return 1;
#endif
}

static inline pte_t
ptep_get_and_clear(struct mm_struct *mm, unsigned long addr, pte_t *ptep)
{
#ifdef CONFIG_SMP
	return __pte(xchg((long *) ptep, 0));
#else
	pte_t pte = *ptep;
	pte_clear(mm, addr, ptep);
	return pte;
#endif
}

static inline void
ptep_set_wrprotect(struct mm_struct *mm, unsigned long addr, pte_t *ptep)
{
#ifdef CONFIG_SMP
	unsigned long new, old;

	do {
		old = pte_val(*ptep);
		new = pte_val(pte_wrprotect(__pte (old)));
	} while (cmpxchg((unsigned long *) ptep, old, new) != old);
#else
	pte_t old_pte = *ptep;
	set_pte_at(mm, addr, ptep, pte_wrprotect(old_pte));
#endif
}

static inline int
pte_same (pte_t a, pte_t b)
{
	return pte_val(a) == pte_val(b);
}

#define update_mmu_cache(vma, address, ptep) do { } while (0)

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];
extern void paging_init (void);

/*
 * Note: The macros below rely on the fact that MAX_SWAPFILES_SHIFT <= number of
 *	 bits in the swap-type field of the swap pte.  It would be nice to
 *	 enforce that, but we can't easily include <linux/swap.h> here.
 *	 (Of course, better still would be to define MAX_SWAPFILES_SHIFT here...).
 *
 * Format of swap pte:
 *	bit   0   : present bit (must be zero)
 *	bits  1- 7: swap-type
 *	bits  8-62: swap offset
 *	bit  63   : _PAGE_PROTNONE bit
 */
#define __swp_type(entry)		(((entry).val >> 1) & 0x7f)
#define __swp_offset(entry)		(((entry).val << 1) >> 9)
#define __swp_entry(type,offset)	((swp_entry_t) { ((type) << 1) | ((long) (offset) << 8) })
#define __pte_to_swp_entry(pte)		((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)		((pte_t) { (x).val })

/*
 * ZERO_PAGE is a global shared page that is always zero: used
 * for zero-mapped memory areas etc..
 */
extern unsigned long empty_zero_page[PAGE_SIZE/sizeof(unsigned long)];
extern struct page *zero_page_memmap_ptr;
#define ZERO_PAGE(vaddr) (zero_page_memmap_ptr)

/* We provide our own get_unmapped_area to cope with VA holes for userland */
#define HAVE_ARCH_UNMAPPED_AREA

#ifdef CONFIG_HUGETLB_PAGE
#define HUGETLB_PGDIR_SHIFT	(HPAGE_SHIFT + 2*(PAGE_SHIFT-3))
#define HUGETLB_PGDIR_SIZE	(__IA64_UL(1) << HUGETLB_PGDIR_SHIFT)
#define HUGETLB_PGDIR_MASK	(~(HUGETLB_PGDIR_SIZE-1))
#endif


#define __HAVE_ARCH_PTEP_SET_ACCESS_FLAGS
/*
 * Update PTEP with ENTRY, which is guaranteed to be a less
 * restrictive PTE.  That is, ENTRY may have the ACCESSED, DIRTY, and
 * WRITABLE bits turned on, when the value at PTEP did not.  The
 * WRITABLE bit may only be turned if SAFELY_WRITABLE is TRUE.
 *
 * SAFELY_WRITABLE is TRUE if we can update the value at PTEP without
 * having to worry about races.  On SMP machines, there are only two
 * cases where this is true:
 *
 *	(1) *PTEP has the PRESENT bit turned OFF
 *	(2) ENTRY has the DIRTY bit turned ON
 *
 * On ia64, we could implement this routine with a cmpxchg()-loop
 * which ORs in the _PAGE_A/_PAGE_D bit if they're set in ENTRY.
 * However, like on x86, we can get a more streamlined version by
 * observing that it is OK to drop ACCESSED bit updates when
 * SAFELY_WRITABLE is FALSE.  Besides being rare, all that would do is
 * result in an extra Access-bit fault, which would then turn on the
 * ACCESSED bit in the low-level fault handler (iaccess_bit or
 * daccess_bit in ivt.S).
 */
#ifdef CONFIG_SMP
# define ptep_set_access_flags(__vma, __addr, __ptep, __entry, __safely_writable) \
({									\
	int __changed = !pte_same(*(__ptep), __entry);			\
	if (__changed && __safely_writable) {				\
		set_pte(__ptep, __entry);				\
		flush_tlb_page(__vma, __addr);				\
	}								\
	__changed;							\
})
#else
# define ptep_set_access_flags(__vma, __addr, __ptep, __entry, __safely_writable) \
({									\
	int __changed = !pte_same(*(__ptep), __entry);			\
	if (__changed) {						\
		set_pte_at((__vma)->vm_mm, (__addr), __ptep, __entry);	\
		flush_tlb_page(__vma, __addr);				\
	}								\
	__changed;							\
})
#endif

#  ifdef CONFIG_VIRTUAL_MEM_MAP
  /* arch mem_map init routine is needed due to holes in a virtual mem_map */
#   define __HAVE_ARCH_MEMMAP_INIT
    extern void memmap_init (unsigned long size, int nid, unsigned long zone,
			     unsigned long start_pfn);
#  endif /* CONFIG_VIRTUAL_MEM_MAP */
# endif /* !__ASSEMBLY__ */

/*
 * Identity-mapped regions use a large page size.  We'll call such large pages
 * "granules".  If you can think of a better name that's unambiguous, let me
 * know...
 */
#if defined(CONFIG_IA64_GRANULE_64MB)
# define IA64_GRANULE_SHIFT	_PAGE_SIZE_64M
#elif defined(CONFIG_IA64_GRANULE_16MB)
# define IA64_GRANULE_SHIFT	_PAGE_SIZE_16M
#endif
#define IA64_GRANULE_SIZE	(1 << IA64_GRANULE_SHIFT)
/*
 * log2() of the page size we use to map the kernel image (IA64_TR_KERNEL):
 */
#define KERNEL_TR_PAGE_SHIFT	_PAGE_SIZE_64M
#define KERNEL_TR_PAGE_SIZE	(1 << KERNEL_TR_PAGE_SHIFT)

/*
 * No page table caches to initialise
 */
#define pgtable_cache_init()	do { } while (0)

/* These tell get_user_pages() that the first gate page is accessible from user-level.  */
#define FIXADDR_USER_START	GATE_ADDR
#ifdef HAVE_BUGGY_SEGREL
# define FIXADDR_USER_END	(GATE_ADDR + 2*PAGE_SIZE)
#else
# define FIXADDR_USER_END	(GATE_ADDR + 2*PERCPU_PAGE_SIZE)
#endif

#define __HAVE_ARCH_PTEP_TEST_AND_CLEAR_YOUNG
#define __HAVE_ARCH_PTEP_GET_AND_CLEAR
#define __HAVE_ARCH_PTEP_SET_WRPROTECT
#define __HAVE_ARCH_PTE_SAME
#define __HAVE_ARCH_PGD_OFFSET_GATE


#if CONFIG_PGTABLE_LEVELS == 3
#include <asm-generic/pgtable-nopud.h>
#endif
#include <asm-generic/pgtable.h>

#endif /* _ASM_IA64_PGTABLE_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  