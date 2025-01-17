e)
{
	const unsigned long *p = vaddr;
	int res = 32;
	unsigned int words;
	unsigned long num;

	if (!size)
		return 0;

	words = (size + 31) >> 5;
	while (!(num = ~*p++)) {
		if (!--words)
			goto out;
	}

	__asm__ __volatile__ ("bfffo %1{#0,#0},%0"
			      : "=d" (res) : "d" (num & -num));
	res ^= 31;
out:
	res += ((long)p - (long)vaddr - 4) * 8;
	return res < size ? res : size;
}
#define find_first_zero_bit find_first_zero_bit

static inline int find_next_zero_bit(const unsigned long *vaddr, int size,
				     int offset)
{
	const unsigned long *p = vaddr + (offset >> 5);
	int bit = offset & 31UL, res;

	if (offset >= size)
		return size;

	if (bit) {
		unsigned long num = ~*p++ & (~0UL << bit);
		offset -= bit;

		/* Look for zero in first longword */
		__asm__ __volatile__ ("bfffo %1{#0,#0},%0"
				      : "=d" (res) : "d" (num & -num));
		if (res < 32) {
			offset += res ^ 31;
			return offset < size ? offset : size;
		}
		offset += 32;

		if (offset >= size)
			return size;
	}
	/* No zero yet, search remaining full bytes for a zero */
	return offset + find_first_zero_bit(p, size - offset);
}
#define find_next_zero_bit find_next_zero_bit

static inline int find_first_bit(const unsigned long *vaddr, unsigned size)
{
	const unsigned long *p = vaddr;
	int res = 32;
	unsigned int words;
	unsigned long num;

	if (!size)
		return 0;

	words = (size + 31) >> 5;
	while (!(num = *p++)) {
		if (!--words)
			goto out;
	}

	__asm__ __volatile__ ("bfffo %1{#0,#0},%0"
			      : "=d" (res) : "d" (num & -num));
	res ^= 31;
out:
	res += ((long)p - (long)vaddr - 4) * 8;
	return res < size ? res : size;
}
#define find_first_bit find_first_bit

static inline int find_next_bit(const unsigned long *vaddr, int size,
				int offset)
{
	const unsigned long *p = vaddr + (offset >> 5);
	int bit = offset & 31UL, res;

	if (offset >= size)
		return size;

	if (bit) {
		unsigned long num = *p++ & (~0UL << bit);
		offset -= bit;

		/* Look for one in first longword */
		__asm__ __volatile__ ("bfffo %1{#0,#0},%0"
				      : "=d" (res) : "d" (num & -num));
		if (res < 32) {
			offset += res ^ 31;
			return offset < size ? offset : size;
		}
		offset += 32;

		if (offset >= size)
			return size;
	}
	/* No one yet, search remaining full bytes for a one */
	return offset + find_first_bit(p, size - offset);
}
#define find_next_bit find_next_bit

/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 * so code should check against ~0UL first..
 */
static inline unsigned long ffz(unsigned long word)
{
	int res;

	__asm__ __volatile__ ("bfffo %1{#0,#0},%0"
			      : "=d" (res) : "d" (~word & -~word));
	return res ^ 31;
}

#endif

#ifdef __KERNEL__

#if defined(CONFIG_CPU_HAS_NO_BITFIELDS)

/*
 *	The newer ColdFire family members support a "bitrev" instruction
 *	and we can use that to implement a fast ffs. Older Coldfire parts,
 *	and normal 68000 parts don't have anything special, so we use the
 *	generic functions for those.
 */
#if (defined(__mcfisaaplus__) || defined(__mcfisac__)) && \
	!defined(CONFIG_M68000) && !defined(CONFIG_MCPU32)
static inline int __ffs(int x)
{
	__asm__ __volatile__ ("bitrev %0; ff1 %0"
		: "=d" (x)
		: "0" (x));
	return x;
}

static inline int ffs(int x)
{
	if (!x)
		return 0;
	return __ffs(x) + 1;
}

#else
#include <asm-generic/bitops/ffs.h>
#include <asm-generic/bitops/__ffs.h>
#endif

#include <asm-generic/bitops/fls.h>
#include <asm-generic/bitops/__fls.h>

#else

/*
 *	ffs: find first bit set. This is defined the same way as
 *	the libc and compiler builtin ffs routines, therefore
 *	differs in spirit from the above ffz (man ffs).
 */
static inline int ffs(int x)
{
	int cnt;

	__asm__ ("bfffo %1{#0:#0},%0"
		: "=d" (cnt)
		: "dm" (x & -x));
	return 32 - cnt;
}
#define __ffs(x) (ffs(x) - 1)

/*
 *	fls: find last bit set.
 */
static inline int fls(int x)
{
	int cnt;

	__asm__ ("bfffo %1{#0,#0},%0"
		: "=d" (cnt)
		: "dm" (x));
	return 32 - cnt;
}

static inline int __fls(int x)
{
	return fls(x) - 1;
}

#endif

#include <asm-generic/bitops/ext2-atomic.h>
#include <asm-generic/bitops/le.h>
#include <asm-generic/bitops/fls64.h>
#include <asm-generic/bitops/sched.h>
#include <asm-generic/bitops/hweight.h>
#include <asm-generic/bitops/lock.h>
#endif /* __KERNEL__ */

#endif /* _M68K_BITOPS_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          