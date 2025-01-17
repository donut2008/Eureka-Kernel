#ifndef _ASM_IA64_PCI_H
#define _ASM_IA64_PCI_H

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/scatterlist.h>

#include <asm/io.h>
#include <asm/hw_irq.h>

struct pci_vector_struct {
	__u16 segment;	/* PCI Segment number */
	__u16 bus;	/* PCI Bus number */
	__u32 pci_id;	/* ACPI split 16 bits device, 16 bits function (see section 6.1.1) */
	__u8 pin;	/* PCI PIN (0 = A, 1 = B, 2 = C, 3 = D) */
	__u32 irq;	/* IRQ assigned */
};

/*
 * Can be used to override the logic in pci_scan_bus for skipping already-configured bus
 * numbers - to be used for buggy BIOSes or architectures with incomplete PCI setup by the
 * loader.
 */
#define pcibios_assign_all_busses()     0

#define PCIBIOS_MIN_IO		0x1000
#define PCIBIOS_MIN_MEM		0x10000000

void pcibios_config_init(void);

struct pci_dev;

/*
 * PCI_DMA_BUS_IS_PHYS should be set to 1 if there is _necessarily_ a direct
 * correspondence between device bus addresses and CPU physical addresses.
 * Platforms with a hardware I/O MMU _must_ turn this off to suppress the
 * bounce buffer handling code in the block and network device layers.
 * Platforms with separate bus address spaces _must_ turn this off and provide
 * a device DMA mapping implementation that takes care of the necessary
 * address translation.
 *
 * For now, the ia64 platforms which may have separate/multiple bus address
 * spaces all have I/O MMUs which support the merging of physically
 * discontiguous buffers, so we can use that as the sole factor to determine
 * the setting of PCI_DMA_BUS_IS_PHYS.
 */
extern unsigned long ia64_max_iommu_merge_mask;
#define PCI_DMA_BUS_IS_PHYS	(ia64_max_iommu_merge_mask == ~0UL)

#include <asm-generic/pci-dma-compat.h>

#define HAVE_PCI_MMAP
extern int pci_mmap_page_range (struct pci_dev *dev, struct vm_area_struct *vma,
				enum pci_mmap_state mmap_state, int write_combine);
#define HAVE_PCI_LEGACY
extern int pci_mmap_legacy_page_range(struct pci_bus *bus,
				      struct vm_area_struct *vma,
				      enum pci_mmap_state mmap_state);

#define pci_get_legacy_mem platform_pci_get_legacy_mem
#define pci_legacy_read platform_pci_legacy_read
#define pci_legacy_write platform_pci_legacy_write

struct pci_controller {
	struct acpi_device *companion;
	void *iommu;
	int segment;
	int node;		/* nearest node with memory or NUMA_NO_NODE for global allocation */

	void *platform_data;
};


#define PCI_CONTROLLER(busdev) ((struct pci_controller *) busdev->sysdata)
#define pci_domain_nr(busdev)    (PCI_CONTROLLER(busdev)->segment)

extern struct pci_ops pci_root_ops;

static inline int pci_proc_domain(struct pci_bus *bus)
{
	return (pci_domain_nr(bus) != 0);
}

#define HAVE_ARCH_PCI_GET_LEGACY_IDE_IRQ
static inline int pci_get_legacy_ide_irq(struct pci_dev *dev, int channel)
{
	return channel ? isa_irq_to_vector(15) : isa_irq_to_vector(14);
}

#ifdef CONFIG_INTEL_IOMMU
extern void pci_iommu_alloc(void);
#endif
#endif /* _ASM_IA64_PCI_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            #ifndef _ASM_IA64_PERCPU_H
#define _ASM_IA64_PERCPU_H

/*
 * Copyright (C) 2002-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */

#define PERCPU_ENOUGH_ROOM PERCPU_PAGE_SIZE

#ifdef __ASSEMBLY__
# define THIS_CPU(var)	(var)  /* use this to mark accesses to per-CPU variables... */
#else /* !__ASSEMBLY__ */


#include <linux/threads.h>

#ifdef CONFIG_SMP

#ifdef HAVE_MODEL_SMALL_ATTRIBUTE
# define PER_CPU_ATTRIBUTES	__attribute__((__model__ (__small__)))
#endif

#define __my_cpu_offset	__ia64_per_cpu_var(local_per_cpu_offset)

extern void *per_cpu_init(void);

#else /* ! SMP */

#define per_cpu_init()				(__phys_per_cpu_start)

#endif	/* SMP */

#define PER_CPU_BASE_SECTION ".data..percpu"

/*
 * Be extremely careful when taking the address of this variable!  Due to virtual
 * remapping, it is different from the canonical address returned by this_cpu_ptr(&var)!
 * On the positive side, using __ia64_per_cpu_var() instead of this_cpu_ptr() is slightly
 * more efficient.
 */
#define __ia64_per_cpu_var(var) (*({					\
	__verify_pcpu_ptr(&(var));					\
	((typeof(var) __kernel __force *)&(var));			\
}))

#include <asm-generic/percpu.h>

/* Equal to __per_cpu_offset[smp_processor_id()], but faster to access: */
DECLARE_PER_CPU(unsigned long, local_per_cpu_offset);

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_IA64_PERCPU_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * Copyright (C) 2001-2003 Hewlett-Packard Co
 *               Stephane Eranian <eranian@hpl.hp.com>
 */
#ifndef _ASM_IA64_PERFMON_H
#define _ASM_IA64_PERFMON_H

#include <uapi/asm/perfmon.h>


extern long perfmonctl(int fd, int cmd, void *arg, int narg);

typedef struct {
	void (*handler)(int irq, void *arg, struct pt_regs *regs);
} pfm_intr_handler_desc_t;

extern void pfm_save_regs (struct task_struct *);
extern void pfm_load_regs (struct task_struct *);

extern void pfm_exit_thread(struct task_struct *);
extern int  pfm_use_debug_registers(struct task_struct *);
extern int  pfm_release_debug_registers(struct task_struct *);
extern void pfm_syst_wide_update_task(struct task_struct *, unsigned long info, int is_ctxswin);
extern void pfm_inherit(struct task_struct *task, struct pt_regs *regs);
extern void pfm_init_percpu(void);
extern void pfm_handle_work(void);
extern int  pfm_install_alt_pmu_interrupt(pfm_intr_handler_desc_t *h);
extern int  pfm_remove_alt_pmu_interrupt(pfm_intr_handler_desc_t *h);



/*
 * Reset PMD register flags
 */
#define PFM_PMD_SHORT_RESET	0
#define PFM_PMD_LONG_RESET	1

typedef union {
	unsigned int val;
	struct {
		unsigned int notify_user:1;	/* notify user program of overflow */
		unsigned int reset_ovfl_pmds:1;	/* reset overflowed PMDs */
		unsigned int block_task:1;	/* block monitored task on kernel exit */
		unsigned int mask_monitoring:1; /* mask monitors via PMCx.plm */
		unsigned int reserved:28;	/* for future use */
	} bits;
} pfm_ovfl_ctrl_t;

typedef struct {
	unsigned char	ovfl_pmd;			/* index of overflowed PMD  */
	unsigned char   ovfl_notify;			/* =1 if monitor requested overflow notification */
	unsigned short  active_set;			/* event set active at the time of the overflow */
	pfm_ovfl_ctrl_t ovfl_ctrl;			/* return: perfmon controls to set by handler */

	unsigned long   pmd_last_reset;			/* last reset value of of the PMD */
	unsigned long	smpl_pmds[4];			/* bitmask of other PMD of interest on overflow */
	unsigned long   smpl_pmds_values[PMU_MAX_PMDS]; /* values for the other PMDs of interest */
	unsigned long   pmd_value;			/* current 64-bit value of the PMD */
	unsigned long	pmd_eventid;			/* eventid associated with PMD */
} pfm_ovfl_arg_t;


typedef struct {
	char		*fmt_name;
	pfm_uuid_t	fmt_uuid;
	size_t		fmt_arg_size;
	unsigned long	fmt_flags;

	int		(*fmt_validate)(struct task_struct *task, unsigned int flags, int cpu, void *arg);
	int		(*fmt_getsize)(struct task_struct *task, unsigned int flags, int cpu, void *arg, unsigned long *size);
	int 		(*fmt_init)(struct task_struct *task, void *buf, unsigned int flags, int cpu, void *arg);
	int		(*fmt_handler)(struct task_struct *task, void *buf, pfm_ovfl_arg_t *arg, struct pt_regs *regs, unsigned long stamp);
	int		(*fmt_restart)(struct task_struct *task, pfm_ovfl_ctrl_t *ctrl, void *buf, struct pt_regs *regs);
	int		(*fmt_restart_active)(struct task_struct *task, pfm_ovfl_ctrl_t *ctrl, void *buf, struct pt_regs *regs);
	int		(*fmt_exit)(struct task_struct *task, void *buf, struct pt_regs *regs);

	struct list_head fmt_list;
} pfm_buffer_fmt_t;

extern int pfm_register_buffer_fmt(pfm_buffer_fmt_t *fmt);
extern int pfm_unregister_buffer_fmt(pfm_uuid_t uuid);

/*
 * perfmon interface exported to modules
 */
extern int pfm_mod_read_pmds(struct task_struct *, void *req, unsigned int nreq, struct pt_regs *regs);
extern int pfm_mod_write_pmcs(struct task_struct *, void *req, unsigned int nreq, struct pt_regs *regs);
extern int pfm_mod_write_ibrs(struct task_struct *task, void *req, unsigned int nreq, struct pt_regs *regs);
extern int pfm_mod_write_dbrs(struct task_struct *task, void *req, unsigned int nreq, struct pt_regs *regs);

/*
 * describe the content of the local_cpu_date->pfm_syst_info field
 */
#define PFM_CPUINFO_SYST_WIDE	0x1	/* if set a system wide session exists */
#define PFM_CPUINFO_DCR_PP	0x2	/* if set the system wide session has started */
#define PFM_CPUINFO_EXCL_IDLE	0x4	/* the system wide session excludes the idle task */

/*
 * sysctl control structure. visible to sampling formats
 */
typedef struct {
	int	debug;		/* turn on/off debugging via syslog */
	int	debug_ovfl;	/* turn on/off debug printk in overflow handler */
	int	fastctxsw;	/* turn on/off fast (unsecure) ctxsw */
	int	expert_mode;	/* turn on/off value checking */
} pfm_sysctl_t;
extern pfm_sysctl_t pfm_sysctl;


#endif /* _ASM_IA64_PERFMON_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   #ifndef _ASM_IA64_PGALLOC_H
#define _ASM_IA64_PGALLOC_H

/*
 * This file contains the functions and defines necessary to allocate
 * page tables.
 *
 * This hopefully works with any (fixed) ia-64 page-size, as defined
 * in <asm/page.h> (currently 8192).
 *
 * Copyright (C) 1998-2001 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 2000, Goutham Rao <goutham.rao@intel.com>
 */


#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/page-flags.h>
#include <linux/threads.h>
#include <linux/quicklist.h>

#include <asm/mmu_context.h>

static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	return quicklist_alloc(0, GFP_KERNEL, NULL);
}

static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
	quicklist_free(0, NULL, pgd);
}

#if CONFIG_PGTABLE_LEVELS == 4
static inline void
pgd_populate(struct mm_struct *mm, pgd_t * pgd_entry, pud_t * pud)
{
	pgd_val(*pgd_entry) = __pa(pud);
}

static inline pud_t *pud_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	return quicklist_alloc(0, GFP_KERNEL, NULL);
}

static inline void pud_free(struct mm_struct *mm, pud_t *pud)
{
	quicklist_free(0, NULL, pud);
}
#define __pud_free_tlb(tlb, pud, address)	pud_free((tlb)->mm, pud)
#endif /* CONFIG_PGTABLE_LEVELS == 4 */

static inline void
pud_populate(struct mm_struct *mm, pud_t * pud_entry, pmd_t * pmd)
{
	pud_val(*pud_entry) = __pa(pmd);
}

static inline pmd_t *pmd_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	return quicklist_alloc(0, GFP_KERNEL, NULL);
}

static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd)
{
	quicklist_free(0, NULL, pmd);
}

#define __pmd_free_tlb(tlb, pmd, address)	pmd_free((tlb)->mm, pmd)

static inline void
pmd_populate(struct mm_struct *mm, pmd_t * pmd_entry, pgtable_t pte)
{
	pmd_val(*pmd_entry) = page_to_phys(pte);
}
#define pmd_pgtable(pmd) pmd_page(pmd)

static inline void
pmd_populate_kernel(struct mm_struct *mm, pmd_t * pmd_entry, pte_t * pte)
{
	pmd_val(*pmd_entry) = __pa(pte);
}

static inline pgtable_t pte_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	struct page *page;
	void *pg;

	pg = quicklist_alloc(0, GFP_KERNEL, NULL);
	if (!pg)
		return NULL;
	page = virt_to_page(pg);
	if (!pgtable_page_ctor(page)) {
		quicklist_free(0, NULL, pg);
		return NULL;
	}
	return page;
}

static inline pte_t *pte_alloc_one_kernel(struct mm_struct *mm,
					  unsigned long addr)
{
	return quicklist_alloc(0, GFP_KERNEL, NULL);
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
	pgtable_page_dtor(pte);
	quicklist_free_page(0, NULL, pte);
}

static inline void pte_free_k