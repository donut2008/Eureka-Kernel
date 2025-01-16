E_LEVELS == 4
#define PGDIR_SHIFT		(PUD_SHIFT + (PTRS_PER_PTD_SHIFT))
#else
#define PGDIR_SHIFT		(PMD_SHIFT + (PTRS_PER_PTD_SHIFT))
#endif
#define PGDIR_SIZE		(__IA64_UL(1) << PGDIR_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE-1))
#define PTRS_PER_PGD_SHIFT	PTRS_PER_PTD_SHIFT
#define PTRS_PER_PGD		(1UL << PTRS_PER_PGD_SHIFT)
#define USER_PTRS_PER_PGD	(5*PTRS_PER_PGD/8)	/* regions 0-4 are user regions */
#define FIRST_USER_ADDRESS	0UL

/*
 * All the normal masks have the "page accessed" bits on, as any time
 * they are used, the page is accessed. They are cleared only by the
 * page-out routines.
 */
#define PAGE_NONE	__pgprot(_PAGE_PROTNONE | _PAGE_A)
#define PAGE_SHARED	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_RW)
#define PAGE_READONLY	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_R)
#define PAGE_COPY	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_R)
#define PAGE_COPY_EXEC	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_RX)
#define PAGE_GATE	__pgprot(__ACCESS_BITS | _PAGE_PL_0 | _PAGE_AR_X_RX)
#define PAGE_KERNEL	__pgprot(__DIRTY_BITS  | _PAGE_PL_0 | _PAGE_AR_RWX)
#define PAGE_KERNELRX	__pgprot(__ACCESS_BITS | _PAGE_PL_0 | _PAGE_AR_RX)
#define PAGE_KERNEL_UC	__pgprot(__DIRTY_BITS  | _PAGE_PL_0 | _PAGE_AR_RWX | \
				 _PAGE_MA_UC)

# ifndef __ASSEMBLY__

#include <linux/sched.h>	/* for mm_struct */
#include <linux/bitops.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>

/*
 * Next come the mappings that determine how mmap() protection bits
 * (PROT_EXEC, PROT_READ, PROT_WRITE, PROT_NONE) get implemented.  The
 * _P version gets used for a private shared memory segment, the _S
 * version gets used for a shared memory segment with MAP_SHARED on.
 * In a private shared memory segment, we do a copy-on-write if a task
 * attempts to write to the page.
 */
	/* xwr */
#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_READONLY	/* write to priv pg -> copy & make writable */
#define __P011	PAGE_READONLY	/* ditto */
#define __P100	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_X_RX)
#define __P101	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_RX)
#define __P110	PAGE_COPY_EXEC
#define __P111	PAGE_COPY_EXEC

#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY
#define __S010	PAGE_SHARED	/* we don't have (and don't need) write-only */
#define __S011	PAGE_SHARED
#define __S100	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_X_RX)
#define __S101	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_RX)
#define __S110	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_RWX)
#define __S111	__pgprot(__ACCESS_BITS | _PAGE_PL_3 | _PAGE_AR_RWX)

#define pgd_ERROR(e)	printk("%s:%d: bad pgd %016lx.\n", __FILE__, __LINE__, pgd_val(e))
#if CONFIG_PGTABLE_LEVELS == 4
#define pud_ERROR(e)	printk("%s:%d: bad pud %016lx.\n", __FILE__, __LINE__, pud_val(e))
#endif
#define pmd_ERROR(e)	printk("%s:%d: bad pmd %016lx.\n", __FILE__, __LINE__, pmd_val(e))
#define pte_ERROR(e)	printk("%s:%d: bad pte %016lx.\n", __FILE__, __LINE__, pte_val(e))


/*
 * Some definitions to translate between mem_map, PTEs, and page addresses:
 */


/* Quick test to s