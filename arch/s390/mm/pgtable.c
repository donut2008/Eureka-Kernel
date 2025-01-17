/* bootstd.h:  Bootloader system call interface
 *
 * (c) 1999, Rt-Control, Inc.
 */

#ifndef __BOOTSTD_H__
#define __BOOTSTD_H__

#define NR_BSC 21            /* last used bootloader system call */

#define __BN_reset        0  /* reset and start the bootloader */
#define __BN_test         1  /* tests the system call interface */
#define __BN_exec         2  /* executes a bootloader image */
#define __BN_exit         3  /* terminates a bootloader image */
#define __BN_program      4  /* program FLASH from a chain */
#define __BN_erase        5  /* erase sector(s) of FLASH */
#define __BN_open         6
#define __BN_write        7
#define __BN_read         8
#define __BN_close        9
#define __BN_mmap         10 /* map a file descriptor into memory */
#define __BN_munmap       11 /* remove a file to memory mapping */
#define __BN_gethwaddr    12 /* get the hardware address of my interfaces */
#define __BN_getserialnum 13 /* get the serial number of this board */
#define __BN_getbenv      14 /* get a bootloader envvar */
#define __BN_setbenv      15 /* get a bootloader envvar */
#define __BN_setpmask     16 /* set the protection mask */
#define __BN_readenv      17 /* read environment variables */
#define __BN_flash_chattr_range		18
#define __BN_flash_erase_range		19
#define __BN_flash_write_range		20

/* Calling conventions compatible to (uC)linux/68k
 * We use similar macros to call into the bootloader as for uClinux
 */

#define __bsc_return(type, res) \
do { \
   if ((unsigned long)(res) >= (unsigned long)(-64)) { \
      /* let errno be a function, preserve res in %d0 */ \
      int __err = -(res); \
      errno = __err; \
      res = -1; \
   } \
   return (type)(res); \
} while (0)

#define _bsc0(type,name) \
type name(void) \
{ \
   register long __res __asm__ ("%d0") = __BN_##name; \
   __asm__ __volatile__ ("trap #2" \
                         : "=g" (__res) \
                         : "0" (__res) \
                         ); \
   __bsc_return(type,__res); \
}

#define _bsc1(type,name,atype,a) \
type name(atype a) \
{ \
   register long __res __asm__ ("%d0") = __BN_##name; \
   register long __a __asm__ ("%d1") = (long)a; \
   __asm__ __volatile__ ("trap #2" \
                         : "=g" (__res) \
                         : "0" (__res), "d" (__a) \
                         ); \
   __bsc_return(type,__res); \
}

#define _bsc2(type,name,atype,a,btype,b) \
type name(atype a, btype b) \
{ \
   register long __res __asm__ ("%d0") = __BN_##name; \
   register long __a __asm__ ("%d1") = (long)a; \
   register long __b __asm__ ("%d2") = (long)b; \
   __asm__ __volatile__ ("trap #2" \
                         : "=g" (__res) \
                         : "0" (__res), "d" (__a), "d" (__b) \
                         ); \
   __bsc_return(type,__res); \
}

#define _bsc3(type,name,atype,a,btype,b,ctype,c) \
type name(atype a, btype b, ctype c) \
{ \
   register long __res __asm__ ("%d0") = __BN_##name; \
   register long __a __asm__ ("%d1") = (long)a; \
   register long __b __asm__ ("%d2") = (long)b; \
   register long __c __asm__ ("%d3") = (long)c; \
   __asm__ __volatile__ ("trap #2" \
                         : "=g" (__res) \
                         : "0" (__res), "d" (__a), "d" (__b), \
                           "d" (__c) \
                         ); \
   __bsc_return(type,__res); \
}

#define _bsc4(type,name,atype,a,btype,b,ctype,c,dtype,d) \
type name(atype a, btype b, ctype c, dtype d) \
{ \
   register long __res __asm__ ("%d0") = __BN_##name; \
   register long __a __asm__ ("%d1") = (long)a; \
   register long __b __asm__ ("%d2") = (long)b; \
   register long __c __asm__ ("%d3") = (long)c; \
   register long __d __asm__ ("%d4") = (long)d; \
   __asm__ __volatile__ ("trap #2" \
                         : "=g" (__res) \
                         : "0" (__res), "d" (__a), "d" (__b), \
                           "d" (__c), "d" (__d) \
                         ); \
   __bsc_return(type,__res); \
}

#define _bsc5(type,name,atype,a,btype,b,ctype,c,dtype,d,etype,e) \
type name(atype a, btype b, ctype c, dtype d, etype e) \
{ \
   register long __res __asm__ ("%d0") = __BN_##name; \
   register long __a __asm__ ("%d1") = (long)a; \
   register long __b __asm__ ("%d2") = (long)b; \
   register long __c __asm__ ("%d3") = (long)c; \
   register long __d __asm__ ("%d4") = (long)d; \
   register long __e __asm__ ("%d5") = (long)e; \
   __asm__ __volatile__ ("trap #2" \
                         : "=g" (__res) \
                         : "0" (__res), "d" (__a), "d" (__b), \
                           "d" (__c), "d" (__d), "d" (__e) \
                         ); \
   __bsc_return(type,__res); \
}

#endif /* __BOOTSTD_H__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            #ifndef _M68K_BUG_H
#define _M68K_BUG_H

#ifdef CONFIG_MMU
#ifdef CONFIG_BUG
#ifdef CONFIG_DEBUG_BUGVERBOSE
#ifndef CONFIG_SUN3
#define BUG() do { \
	printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
	barrier_before_unreachable(); \
	__builtin_trap(); \
} while (0)
#else
#define BUG() do { \
	printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
	barrier_before_unreachable(); \
	panic("BUG!"); \
} while (0)
#endif
#else
#define BUG() do { \
	barrier_before_unreachable(); \
	__builtin_trap(); \
} while (0)
#endif

#define HAVE_ARCH_BUG
#endif
#endif /* CONFIG_MMU */

#include <asm-generic/bug.h>

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      #ifndef _M68K_BVME6000HW_H_
#define _M68K_BVME6000HW_H_

#include <asm/irq.h>

/*
 * PIT structure
 */

#define BVME_PIT_BASE	0xffa00000

typedef struct {
	unsigned char
	pad_a[3], pgcr,
	pad_b[3], psrr,
	pad_c[3], paddr,
	pad_d[3], pbddr,
	pad_e[3], pcddr,
	pad_f[3], pivr,
	pad_g[3], pacr,
	pad_h[3], pbcr,
	pad_i[3], padr,
	pad_j[3], pbdr,
	pad_k[3], paar,
	pad_l[3], pbar,
	pad_m[3], pcdr,
	pad_n[3], psr,
	pad_o[3], res1,
	pad_p[3], res2,
	pad_q[3], tcr,
	pad_r[3], tivr,
	pad_s[3], res3,
	pad_t[3], cprh,
	pad_u[3], cprm,
	pad_v[3], cprl,
	pad_w[3], res4,
	pad_x[3], crh,
	pad_y[3], crm,
	pad_z[3], crl,
	pad_A[3], tsr,
	pad_B[3], res5;
} PitRegs_t, *PitRegsPtr;

#define bvmepit   ((*(volatile PitRegsPtr)(BVME_PIT_BASE)))

#define BVME_RTC_BASE	0xff900000

typedef struct {
	unsigned char
	pad_a[3], msr,
	pad_b[3], t0cr_rtmr,
	pad_c[3], t1cr_omr,
	pad_d[3], pfr_icr0,
	pad_e[3], irr_icr1,
	pad_f[3], bcd_tenms,
	pad_g[3], bcd_sec,
	pad_h[3], bcd_min,
	pad_i[3], bcd_hr,
	pad_j[3], bcd_dom,
	pad_k[3], bcd_mth,
	pad_l[3], bcd_year,
	pad_m[3], bcd_ujcc,
	pad_n[3], bcd_hjcc,
	pad_o[3], bcd_dow,
	pad_p[3], t0lsb,
	pad_q[3], t0msb,
	pad_r[3], t1lsb,
	pad_s[3], t1msb,
	pad_t[3], cmp_sec,
	pad_u[3], cmp_min,
	pad_v[3], cmp_hr,
	pad_w[3], cmp_dom,
	pad_x[3], cmp_mth,
	pad_y[3], cmp_dow,
	pad_z[3], sav_sec,
	pad_A[3], sav_min,
	pad_B[3], sav_hr,
	pad_C[3], sav_dom,
	pad_D[3], sav_mth,
	pad_E[3], ram,
	pad_F[3], test;
} RtcRegs_t, *RtcPtr_t;


#define BVME_I596_BASE	0xff100000

#define BVME_ETHIRQ_REG	0xff20000b

#define BVME_LOCAL_IRQ_STAT  0xff20000f

#define BVME_ETHERR          0x02
#define BVME_ABORT_STATUS    0x08

#define BVME_NCR53C710_BASE	0xff000000

#define BVME_SCC_A_ADDR	0xffb0000b
#define BVME_SCC_B_ADDR	0xffb00003
#define BVME_SCC_RTxC	7372800

#define BVME_CONFIG_REG	0xff500003

#define config_reg_ptr	(volatile unsigned char *)BVME_CONFIG_REG

#define BVME_CONFIG_SW1	0x08
#define BVME_CONFIG_SW2	0x04
#define BVME_CONFIG_SW3	0x02
#define BVME_CONFIG_SW4	0x01


#define BVME_IRQ_TYPE_PRIO	0

#define BVME_IRQ_PRN		(IRQ_USER+20)
#define BVME_IRQ_TIMER		(IRQ_USER+25)
#define BVME_IRQ_I596		IRQ_AUTO_2
#define BVME_IRQ_SCSI		IRQ_AUTO_3
#define BVME_IRQ_RTC		IRQ_AUTO_6
#define BVME_IRQ_ABORT		IRQ_AUTO_7

/* SCC interrupts */
#define BVME_IRQ_SCC_BASE		IRQ_USER
#define BVME_IRQ_SCCB_TX		IRQ_USER
#define BVME_IRQ_SCCB_STAT		(IRQ_USER+2)
#define BVME_IRQ_SCCB_RX		(IRQ_USER+4)
#define BVME_IRQ_SCCB_SPCOND		(IRQ_USER+6)
#define BVME_IRQ_SCCA_TX		(IRQ_USER+8)
#define BVME_IRQ_SCCA_STAT		(IRQ_USER+10)
#define BVME_IRQ_SCCA_RX		(IRQ_USER+12)
#define BVME_IRQ_SCCA_SPCOND		(IRQ_USER+14)

/* Address control registers */

#define BVME_ACR_A32VBA		0xff400003
#define BVME_ACR_A32MSK		0xff410003
#define BVME_ACR_A24VBA		0xff420003
#define BVME_ACR_A24MSK		0xff430003
#define BVME_ACR_A16VBA		0xff440003
#define BVME_ACR_A32LBA		0xff450003
#define BVME_ACR_A24LBA		0xff460003
#define BVME_ACR_ADDRCTL	0xff470003

#define bvme_acr_a32vba		*(volatile unsigned char *)BVME_ACR_A32VBA
#define bvme_acr_a32msk		*(volatile unsigned char *)BVME_ACR_A32MSK
#define bvme_acr_a24vba		*(volatile unsigned char *)BVME_ACR_A24VBA
#define bvme_acr_a24msk		*(volatile unsigned char *)BVME_ACR_A24MSK
#define bvme_acr_a16vba		*(volatile unsigned char *)BVME_ACR_A16VBA
#define bvme_acr_a32lba		*(volatile unsigned char *)BVME_ACR_A32LBA
#define bvme_acr_a24lba		*(volatile unsigned char *)BVME_ACR_A24LBA
#define bvme_acr_addrctl	*(volatile unsigned char *)BVME_ACR_ADDRCTL

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * include/asm-m68k/cache.h
 */
#ifndef __ARCH_M68K_CACHE_H
#define __ARCH_M68K_CACHE_H

/* bytes per L1 cache line */
#define        L1_CACHE_SHIFT  4
#define        L1_CACHE_BYTES  (1<< L1_CACHE_SHIFT)

#define ARCH_DMA_MINALIGN	L1_CACHE_BYTES

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #ifdef __uClinux__
#include <asm/cacheflush_no.h>
#else
#include <asm/cacheflush_mm.h>
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  #ifndef _M68K_CACHEFLUSH_H
#define _M68K_CACHEFLUSH_H

#include <linux/mm.h>
#ifdef CONFIG_COLDFIRE
#include <asm/mcfsim.h>
#endif

/* cache code */
#define FLUSH_I_AND_D	(0x00000808)
#define FLUSH_I		(0x00000008)

#ifndef ICACHE_MAX_ADDR
#define ICACHE_MAX_ADDR	0
#define ICACHE_SET_MASK	0
#define DCACHE_MAX_ADDR	0
#define DCACHE_SETMASK	0
#endif
#ifndef CACHE_MODE
#define	CACHE_MODE	0
#define	CACR_ICINVA	0
#define	CACR_DCINVA	0
#define	CACR_BCINVA	0
#endif

/*
 * ColdFire architecture has no way to clear individual cache lines, so we
 * are stuck invalidating all the cache entries when we want a clear operation.
 */
static inline void clear_cf_icache(unsigned long start, unsigned long end)
{
	__asm__ __volatile__ (
		"movec	%0,%%cacr\n\t"
		"nop"
		:
		: "r" (CACHE_MODE | CACR_ICINVA | CACR_BCINVA));
}

static inline void clear_cf_dcache(unsigned long start, unsigned long end)
{
	__asm__ __volatile__ (
		"movec	%0,%%cacr\n\t"
		"nop"
		:
		: "r" (CACHE_MODE | CACR_DCINVA));
}

static inline void clear_cf_bcache(unsigned long start, unsigned long end)
{
	__asm__ __volatile__ (
		"movec	%0,%%cacr\n\t"
		"nop"
		:
		: "r" (CACHE_MODE | CACR_ICINVA | CACR_BCINVA | CACR_DCINVA));
}

/*
 * Use the ColdFire cpushl instruction to push (and invalidate) cache lines.
 * The start and end addresses are cache line numbers not memory addresses.
 */
static inline void flush_cf_icache(unsigned long start, unsigned long end)
{
	unsigned long set;

	for (set = start; set <= end; set += (0x10 - 3)) {
		__asm__ __volatile__ (
			"cpushl %%ic,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%ic,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%ic,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%ic,(%0)"
			: "=a" (set)
			: "a" (set));
	}
}

static inline void flush_cf_dcache(unsigned long start, unsigned long end)
{
	unsigned long set;

	for (set = start; set <= end; set += (0x10 - 3)) {
		__asm__ __volatile__ (
			"cpushl %%dc,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%dc,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%dc,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%dc,(%0)"
			: "=a" (set)
			: "a" (set));
	}
}

static inline void flush_cf_bcache(unsigned long start, unsigned long end)
{
	unsigned long set;

	for (set = start; set <= end; set += (0x10 - 3)) {
		__asm__ __volatile__ (
			"cpushl %%bc,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%bc,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%bc,(%0)\n\t"
			"addq%.l #1,%0\n\t"
			"cpushl %%bc,(%0)"
			: "=a" (set)
			: "a" (set));
	}
}

/*
 * Cache handling functions
 */

static inline void flush_icache(void)
{
	if (CPU_IS_COLDFIRE) {
		flush_cf_icache(0, ICACHE_MAX_ADDR);
	} else if (CPU_IS_040_OR_060) {
		asm volatile (	"nop\n"
			"	.chip	68040\n"
			"	cpusha	%bc\n"
			"	.chip	68k");
	} else {
		unsigned long tmp;
		asm volatile (	"movec	%%cacr,%0\n"
			"	or.w	%1,%0\n"
			"	movec	%0,%%cacr"
			: "=&d" (tmp)
			: "id" (FLUSH_I));
	}
}

/*
 * invalidate the cache for the specified memory range.
 * It starts at the physical address specified for
 * the given number of bytes.
 */
extern void cache_clear(unsigned long paddr, int len);
/*
 * push any dirty cache in the specified memory range.
 * It starts at the physical address specified for
 * the given number of bytes.
 */
extern void cache_push(unsigned long paddr, int len);

/*
 * push and invalidate pages in the specified user virtual
 * memory range.
 */
extern void cache_push_v(unsigned long vaddr, int len);

/* This is needed whenever the virtual mapping of the current
   process changes.  */
#define __flush_cache_all()					\
({								\
	if (CPU_IS_COLDFIRE) {					\
		flush_cf_dcache(0, DCACHE_MAX_ADDR);		\
	} else if (CPU_IS_040_OR_060) {				\
		__asm__ __volatile__("nop\n\t"			\
				     ".chip 68040\n\t"		\
				     "cpusha %dc\n\t"		\
				     ".chip 68k");		\
	} else {						\
		unsigned long _tmp;				\
		__asm__ __volatile__("movec %%cacr,%0\n\t"	\
				     "orw %1,%0\n\t"		\
				     "movec %0,%%cacr"		\
				     : "=&d" (_tmp)		\
				     : "di" (FLUSH_I_AND_D));	\
	}							\
})

#define __flush_cache_030()					\
({								\
	if (CPU_IS_020_OR_030) {				\
		unsigned long _tmp;				\
		__asm__ __volatile__("movec %%cacr,%0\n\t"	\
				     "orw %1,%0\n\t"		\
				     "movec %0,%%cacr"		\
				     : "=&d" (_tmp)		\
				     : "di" (FLUSH_I_AND_D));	\
	}							\
})

#define flush_cache_all() __flush_cache_all()

#define flush_cache_vmap(start, end)		flush_cache_all()
#define flush_cache_vunmap(start, end)		flush_cache_all()

static inline void flush_cache_mm(struct mm_struct *mm)
{
	if (mm == current->mm)
		__flush_cache_030();
}

#define flush_cache_dup_mm(mm)			flush_cache_mm(mm)

/* flush_cache_range/flush_cache_page must be macros to avoid
   a dependency on linux/mm.h, which includes this file... */
static inline void flush_cache_range(struct vm_area_struct *vma,
				     unsigned long start,
				     unsigned long end)
{
	if (vma->vm_mm == current->mm)
	        __flush_cache_030();
}

static inline void flush_cache_page(struct vm_area_struct *vma, unsigned long vmaddr, unsigned long pfn)
{
	if (vma->vm_mm == current->mm)
	        __flush_cache_030();
}


/* Push the page at kernel virtual address and clear the icache */
/* RZ: use cpush %bc instead of cpush %dc, cinv %ic */
static inline void __flush_page_to_ram(void *vaddr)
{
	if (CPU_IS_COLDFIRE) {
		unsigned long addr, start, end;
		addr = ((unsigned long) vaddr) & ~(PAGE_SIZE - 1);
		start = addr & ICACHE_SET_MASK;
		end = (addr + PAGE_SIZE - 1) & ICACHE_SET_MASK;
		if (start > end) {
			flush_cf_bcache(0, end);
			end = ICACHE_MAX_ADDR;
		}
		flush_cf_bcache(start, end);
	} else if (CPU_IS_040_OR_060) {
		__asm__ __volatile__("nop\n\t"
				     ".chip 68040\n\t"
				     "cpushp %%bc,(%0)\n\t"
				     ".chip 68k"
				     : : "a" (__pa(vaddr)));
	} else {
		unsigned long _tmp;
		__asm__ __volatile__("movec %%cacr,%0\n\t"
				     "orw %1,%0\n\t"
				     "movec %0,%%cacr"
				     : "=&d" (_tmp)
				     : "di" (FLUSH_I));
	}
}

#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1
#define flush_dcache_page(page)		__flush_page_to_ram(page_address(page))
#define flush_dcache_mmap_lock(mapping)		do { } while (0)
#define flush_dcache_mmap_unlock(mapping)	do { } while (0)
#define flush_icache_page(vma, page)	__flush_page_to_ram(page_address(page))

extern void flush_icache_user_range(struct vm_area_struct *vma, struct page *page,
				    unsigned long addr, int len);
extern void flush_icache_range(unsigned long address, unsigned long endaddr);

static inline void copy_to_user_page(struct vm_area_struct *vma,
				     struct page *page, unsigned long vaddr,
				     void *dst, void *src, int len)
{
	flush_cache_page(vma, vaddr, page_to_pfn(page));
	memcpy(dst, src, len);
	flush_icache_user_range(vma, page, vaddr, len);
}
static inline void copy_from_user_page(struct vm_area_struct *vma,
				       struct page *page, unsigned long vaddr,
				       void *dst, void *src, int len)
{
	flush_cache_page(vma, vaddr, page_to_pfn(page));
	memcpy(dst, src, len);
}

#endif /* _M68K_CACHEFLUSH_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     #ifndef _M68KNOMMU_CACHEFLUSH_H
#define _M68KNOMMU_CACHEFLUSH_H

/*
 * (C) Copyright 2000-2010, Greg Ungerer <gerg@snapgear.com>
 */
#include <linux/mm.h>
#include <asm/mcfsim.h>

#define flush_cache_all()			__flush_cache_all()
#define flush_cache_mm(mm)			do { } while (0)
#define flush_cache_dup_mm(mm)			do { } while (0)
#define flush_cache_range(vma, start, end)	do { } while (0)
#define flush_cache_page(vma, vmaddr)		do { } while (0)
#define flush_dcache_range(start, len)		__flush_dcache_all()
#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 0
#define flush_dcache_page(page)			do { } while (0)
#define flush_dcache_mmap_lock(mapping)		do { } while (0)
#define flush_dcache_mmap_unlock(mapping)	do { } while (0)
#define flush_icache_range(start, len)		__flush_icache_all()
#define flush_icache_page(vma,pg)		do { } while (0)
#define flush_icache_user_range(vma,pg,adr,len)	do { } while (0)
#define flush_cache_vmap(start, end)		do { } while (0)
#define flush_cache_vunmap(start, end)		do { } while (0)

#define copy_to_user_page(vma, page, vaddr, dst, src, len) \
	memcpy(dst, src, len)
#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
	memcpy(dst, src, len)

void mcf_cache_push(void);

static inline void __clear_cache_all(void)
{
#ifdef CACHE_INVALIDATE
	__asm__ __volatile__ (
		"movec	%0, %%CACR\n\t"
		"nop\n\t"
		: : "r" (CACHE_INVALIDATE) );
#endif
}

static inline void __flush_cache_all(void)
{
#ifdef CACHE_PUSH
	mcf_cache_push();
#endif
	__clear_cache_all();
}

/*
 * Some ColdFire parts implement separate instruction and data caches,
 * on those we should just flush the appropriate cache. If we don't need
 * to do any specific flushing then this will be optimized away.
 */
static inline void __flush_icache_all(void)
{
#ifdef CACHE_INVALIDATEI
	__asm__ __volatile__ (
		"movec	%0, %%CACR\n\t"
		"nop\n\t"
		: : "r" (CACHE_INVALIDATEI) );
#endif
}

static inline void __flush_dcache_all(void)
{
#ifdef CACHE_PUSH
	mcf_cache_push();
#endif
#ifdef CACHE_INVALIDATED
	__asm__ __volatile__ (
		"movec	%0, %%CACR\n\t"
		"nop\n\t"
		: : "r" (CACHE_INVALIDATED) );
#else
	/* Flush the write buffer */
	__asm__ __volatile__ ( "nop" );
#endif
}

/*
 * Push cache entries at supplied address. We want to write back any dirty
 * data and then invalidate the cache lines associated with this address.
 */
static inline void cache_push(unsigned long paddr, int len)
{
	__flush_cache_all();
}

/*
 * Clear cache entries at supplied address (that is don't write back any
 * dirty data).
 */
static inline void cache_clear(unsigned long paddr, int len)
{
	__clear_cache_all();
}

#endif /* _M68KNOMMU_CACHEFLUSH_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                             