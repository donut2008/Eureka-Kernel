 CONFIG_VIRTUAL_MEM_MAP
# define VMALLOC_END_INIT	(RGN_BASE(RGN_GATE) + (1UL << (4*PAGE_SHIFT - 9)))
extern unsigned long VMALLOC_END;
#else
#if defined(CONFIG_SPARSEMEM) && defined(CONFIG_SPARSEMEM_VMEMMAP)
/* SPARSEMEM_VMEMMAP uses half of vmalloc... */
# define VMALLOC_END		(RGN_BASE(RGN_GATE) + (1UL << (4*PAGE_SHIFT - 10)))
# define vmemmap		((struct page *)VMALLOC_END)
#else
# define VMALLOC_END		(RGN_BASE(RGN_GATE) + (1UL << (4*PAGE_SHIFT - 9)))
#endif
#endif

/* fs/proc/kcore.c */
#define	kc_vaddr_to_offset(v) ((v) - RGN_BASE(RGN_GATE))
#define	kc_offset_to_vaddr(o) ((o) + RGN_BASE(RGN_GATE))

#define RGN_MAP_SHIFT (PGDIR_SHIFT + PTRS_PER_PGD_SHIFT - 3)
#define RGN_MAP_LIMIT	((1UL << RGN_MAP_SHIFT) - PAGE_SIZE)	/* per region addr limit */

/*
 * Conversion functions: convert page frame number (pfn) and a protection value to a page
 * table entry (pte).
 */
#define pfn_pte(pfn, pgprot) \
({ pte_t __pte; pte_val(__pte) = ((pfn) << PAGE_SHIFT) | pgprot_val(pgprot); __pte; })

/* Extract pfn from pte.  */
#define pte_pfn(_pte)		((pte_val(_pte) & _PFN_MASK) >> PAGE_SHIFT)

#define mk_pte(page, pgprot)	pfn_pte(page_to_pfn(page), (pgprot))

/* This takes a physical page address that is used by the remapping functions */
#define mk_pte_phys(physpage, pgprot) \
({ pte_t __pte; pte_val(__pte) = physpage + pgprot_val(pgprot); __pte; })

#define pte_modify(_pte, newprot) \
	(__pte((pte_val(_pte) & ~_PAGE_CHG_MASK) | (pgprot_val(newprot) & _PAGE_CHG_MASK)))

#define pte_none(pte) 			(!pte_val(pte))
#define pte_present(pte)		(pte_val(pte) & (_PAGE_P | _PAGE_PROTNONE))
#define pte_clear(mm,addr,pte)		(pte_val(*(pte)) = 0UL)
/* pte_page() returns the "struct page *" corresponding to the PTE: */
#define pte_page(pte)			virt_to_page(((pte_val(pte) & _PFN_MASK) + PAGE_OFFSET))

#define pmd_none(pmd)			(!pmd_val(pmd))
#define pmd_bad(pmd)			(!ia64_phys_addr_valid(pmd_val(pmd)))
#define pmd_present(pmd)		(pmd_val(pmd) != 0UL)
#define pmd_clear(pmdp)			(pmd_val(*(pmdp)) = 0UL)
#define pmd_page_vaddr(pmd)		((unsigned long) __va(pmd_val(pmd) & _PFN