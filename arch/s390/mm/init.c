#ifndef _M68K_BITOPS_H
#define _M68K_BITOPS_H
/*
 * Copyright 1992, Linus Torvalds.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#ifndef _LINUX_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

#include <linux/compiler.h>
#include <asm/barrier.h>

/*
 *	Bit access functions vary across the ColdFire and 68k families.
 *	So we will break them out here, and then macro in the ones we want.
 *
 *	ColdFire - supports standard bset/bclr/bchg with register operand only
 *	68000    - supports standard bset/bclr/bchg with memory operand
 *	>= 68020 - also supports the bfset/bfclr/bfchg instructions
 *
 *	Although it is possible to use only the bset/bclr/bchg with register
 *	operands on all platforms you end up with larger generated code.
 *	So we use the best form possible on a given platform.
 */

static inline void bset_reg_set_bit(int nr, volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;

	__asm__ __volatile__ ("bset %1,(%0)"
		:
		: "a" (p), "di" (nr & 7)
		: "memory");
}

static inline void bset_mem_set_bit(int nr, volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;

	__asm__ __volatile__ ("bset %1,%0"
		: "+m" (*p)
		: "di" (nr & 7));
}

static inline void bfset_mem_set_bit(int nr, volatile unsigned long *vaddr)
{
	__asm__ __volatile__ ("bfset %1{%0:#1}"
		:
		: "d" (nr ^ 31), "o" (*vaddr)
		: "memory");
}

#if defined(CONFIG_COLDFIRE)
#define	set_bit(nr, vaddr)	bset_reg_set_bit(nr, vaddr)
#elif defined(CONFIG_CPU_HAS_NO_BITFIELDS)
#define	set_bit(nr, vaddr)	bset_mem_set_bit(nr, vaddr)
#else
#define set_bit(nr, vaddr)	(__builtin_constant_p(nr) ? \
				bset_mem_set_bit(nr, vaddr) : \
				bfset_mem_set_bit(nr, vaddr))
#endif

#define __set_bit(nr, vaddr)	set_bit(nr, vaddr)


static inline void bclr_reg_clear_bit(int nr, volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;

	__asm__ __volatile__ ("bclr %1,(%0)"
		:
		: "a" (p), "di" (nr & 7)
		: "memory");
}

static inline void bclr_mem_clear_bit(int nr, volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;

	__asm__ __volatile__ ("bclr %1,%0"
		: "+m" (*p)
		: "di" (nr & 7));
}

static inline void bfclr_mem_clear_bit(int nr, volatile unsigned long *vaddr)
{
	__asm__ __volatile__ ("bfclr %1{%0:#1}"
		:
		: "d" (nr ^ 31), "o" (*vaddr)
		: "memory");
}

#if defined(CONFIG_COLDFIRE)
#define	clear_bit(nr, vaddr)	bclr_reg_clear_bit(nr, vaddr)
#elif defined(CONFIG_CPU_HAS_NO_BITFIELDS)
#define	clear_bit(nr, vaddr)	bclr_mem_clear_bit(nr, vaddr)
#else
#define clear_bit(nr, vaddr)	(__builtin_constant_p(nr) ? \
				bclr_mem_clear_bit(nr, vaddr) : \
				bfclr_mem_clear_bit(nr, vaddr))
#endif

#define __clear_bit(nr, vaddr)	clear_bit(nr, vaddr)


static inline void bchg_reg_change_bit(int nr, volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;

	__asm__ __volatile__ ("bchg %1,(%0)"
		:
		: "a" (p), "di" (nr & 7)
		: "memory");
}

static inline void bchg_mem_change_bit(int nr, volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;

	__asm__ __volatile__ ("bchg %1,%0"
		: "+m" (*p)
		: "di" (nr & 7));
}

static inline void bfchg_mem_change_bit(int nr, volatile unsigned long *vaddr)
{
	__asm__ __volatile__ ("bfchg %1{%0:#1}"
		:
		: "d" (nr ^ 31), "o" (*vaddr)
		: "memory");
}

#if defined(CONFIG_COLDFIRE)
#define	change_bit(nr, vaddr)	bchg_reg_change_bit(nr, vaddr)
#elif defined(CONFIG_CPU_HAS_NO_BITFIELDS)
#define	change_bit(nr, vaddr)	bchg_mem_change_bit(nr, vaddr)
#else
#define change_bit(nr, vaddr)	(__builtin_constant_p(nr) ? \
				bchg_mem_change_bit(nr, vaddr) : \
				bfchg_mem_change_bit(nr, vaddr))
#endif

#define __change_bit(nr, vaddr)	change_bit(nr, vaddr)


static inline int test_bit(int nr, const unsigned long *vaddr)
{
	return (vaddr[nr >> 5] & (1UL << (nr & 31))) != 0;
}


static inline int bset_reg_test_and_set_bit(int nr,
					    volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;
	char retval;

	__asm__ __volatile__ ("bset %2,(%1); sne %0"
		: "=d" (retval)
		: "a" (p), "di" (nr & 7)
		: "memory");
	return retval;
}

static inline int bset_mem_test_and_set_bit(int nr,
					    volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;
	char retval;

	__asm__ __volatile__ ("bset %2,%1; sne %0"
		: "=d" (retval), "+m" (*p)
		: "di" (nr & 7));
	return retval;
}

static inline int bfset_mem_test_and_set_bit(int nr,
					     volatile unsigned long *vaddr)
{
	char retval;

	__asm__ __volatile__ ("bfset %2{%1:#1}; sne %0"
		: "=d" (retval)
		: "d" (nr ^ 31), "o" (*vaddr)
		: "memory");
	return retval;
}

#if defined(CONFIG_COLDFIRE)
#define	test_and_set_bit(nr, vaddr)	bset_reg_test_and_set_bit(nr, vaddr)
#elif defined(CONFIG_CPU_HAS_NO_BITFIELDS)
#define	test_and_set_bit(nr, vaddr)	bset_mem_test_and_set_bit(nr, vaddr)
#else
#define test_and_set_bit(nr, vaddr)	(__builtin_constant_p(nr) ? \
					bset_mem_test_and_set_bit(nr, vaddr) : \
					bfset_mem_test_and_set_bit(nr, vaddr))
#endif

#define __test_and_set_bit(nr, vaddr)	test_and_set_bit(nr, vaddr)


static inline int bclr_reg_test_and_clear_bit(int nr,
					      volatile unsigned long *vaddr)
{
	char *p = (char *)vaddr + (nr ^ 31) / 8;
	char retval;

	__asm__ __volatile__ ("bclr %2,(%1); sne %0"
		: "=d" (retval)
		: "a" (p), "di" (nr & 7)
		: "memory");
	return r