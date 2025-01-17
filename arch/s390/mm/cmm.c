#ifndef _M68K_CHECKSUM_H
#define _M68K_CHECKSUM_H

#include <linux/in6.h>

#ifdef CONFIG_GENERIC_CSUM
#include <asm-generic/checksum.h>
#else

/*
 * computes the checksum of a memory block at buff, length len,
 * and adds in "sum" (32-bit)
 *
 * returns a 32-bit number suitable for feeding into itself
 * or csum_tcpudp_magic
 *
 * this function must be called with even lengths, except
 * for the last fragment, which may be odd
 *
 * it's best to have buff aligned on a 32-bit boundary
 */
__wsum csum_partial(const void *buff, int len, __wsum sum);

/*
 * the same as csum_partial, but copies from src while it
 * checksums
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */

extern __wsum csum_partial_copy_from_user(const void __user *src,
						void *dst,
						int len, __wsum sum,
						int *csum_err);

extern __wsum csum_partial_copy_nocheck(const void *src,
					      void *dst, int len,
					      __wsum sum);

/*
 *	This is a version of ip_fast_csum() optimized for IP headers,
 *	which always checksum on 4 octet boundaries.
 */
static inline __sum16 ip_fast_csum(const void *iph, unsigned int ihl)
{
	unsigned int sum = 0;
	unsigned long tmp;

	__asm__ ("subqw #1,%2\n"
		 "1:\t"
		 "movel %1@+,%3\n\t"
		 "addxl %3,%0\n\t"
		 "dbra  %2,1b\n\t"
		 "movel %0,%3\n\t"
		 "swap  %3\n\t"
		 "addxw %3,%0\n\t"
		 "clrw  %3\n\t"
		 "addxw %3,%0\n\t"
		 : "=d" (sum), "=&a" (iph), "=&d" (ihl), "=&d" (tmp)
		 : "0" (sum), "1" (iph), "2" (ihl)
		 : "memory");
	return (__force __sum16)~sum;
}

static inline __sum16 csum_fold(__wsum sum)
{
	unsigned int tmp = (__force u32)sum;

	__asm__("swap %1\n\t"
		"addw %1, %0\n\t"
		"clrw %1\n\t"
		"addxw %1, %0"
		: "=&d" (sum), "=&d" (tmp)
		: "0" (sum), "1" (tmp));

	return (__force __sum16)~sum;
}

static inline __wsum
csum_tcpudp_nofold(__be32 saddr, __be32 daddr, unsigned short len,
		  unsigned short proto, __wsum sum)
{
	__asm__ ("addl  %2,%0\n\t"
		 "addxl %3,%0\n\t"
		 "addxl %4,%0\n\t"
		 "clrl %1\n\t"
		 "addxl %1,%0"
		 : "=&d" (sum), "=d" (saddr)
		 : "g" (daddr), "1" (saddr), "d" (len + proto),
		   "0" (sum));
	return sum;
}


/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */
static inline __sum16
csum_tcpudp_magic(__be32 saddr, __be32 daddr, unsigned short len,
		  unsigned short proto, __wsum sum)
{
	return csum_fold(csum_tcpudp_nofold(saddr,daddr,len,proto,sum));
}

/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */

static inline __sum16 ip_compute_csum(const void *buff, int len)
{
	return csum_fold (csum_partial(buff, len, 0));
}

#define _HAVE_ARCH_IPV6_CSUM
static __inline__ __sum16
csum_ipv6_magic(const struct in6_addr *saddr, const struct in6_addr *daddr,
		__u32 len, unsigned short proto, __wsum sum)
{
	register unsigned long tmp;
	__asm__("addl %2@,%0\n\t"
		"movel %2@(4),%1\n\t"
		"addxl %1,%0\n\t"
		"movel %2@(8),%1\n\t"
		"addxl %1,%0\n\t"
		"movel %2@(12),%1\n\t"
		"addxl %1,%0\n\t"
		"movel %3@,%1\n\t"
		"addxl %1,%0\n\t"
		"movel %3@(4),%1\n\t"
		"addxl %1,%0\n\t"
		"movel %3@(8),%1\n\t"
		"addxl %1,%0\n\t"
		"movel %3@(12),%1\n\t"
		"addxl %1,%0\n\t"
		"addxl %4,%0\n\t"
		"clrl %1\n\t"
		"addxl %1,%0"
		: "=&d" (sum), "=&d" (tmp)
		: "a" (saddr), "a" (daddr), "d" (len + proto),
		  "0" (sum));

	return csum_fold(sum);
}

#endif /* CONFIG_GENERIC_CSUM */
#endif /* _M68K_CHECKSUM_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #ifndef __ARCH_M68K_CMPXCHG__
#define __ARCH_M68K_CMPXCHG__

#include <linux/irqflags.h>

struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((volatile struct __xchg_dummy *)(x))

extern unsigned long __invalid_xchg_size(unsigned long, volatile void *, int);

#ifndef CONFIG_RMW_INSNS
static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	unsigned long flags, tmp;

	local_irq_save(flags);

	switch (size) {
	case 1:
		tmp = *(u8 *)ptr;
		*(u8 *)ptr = x;
		x = tmp;
		break;
	case 2:
		tmp = *(u16 *)ptr;
		*(u16 *)ptr = x;
		x = tmp;
		break;
	case 4:
		tmp = *(u32 *)ptr;
		*(u32 *)ptr = x;
		x = tmp;
		break;
	default:
		tmp = __invalid_xchg_size(x, ptr, size);
		break;
	}

	local_irq_restore(flags);
	return x;
}
#else
static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
	case 1:
		__asm__ __volatile__
			("moveb %2,%0\n\t"
			 "1:\n\t"
			 "casb %0,%1,%2\n\t"
			 "jne 1b"
			 : "=&d" (x) : "d" (x), "m" (*__xg(ptr)) : "memory");
		break;
	case 2:
		__asm__ __volatile__
			("movew %2,%0\n\t"
			 "1:\n\t"
			 "casw %0,%1,%2\n\t"
			 "jne 1b"
			 : "=&d" (x) : "d" (x), "m" (*__xg(ptr)) : "memory");
		break;
	case 4:
		__asm__ __volatile__
			("movel %2,%0\n\t"
			 "1:\n\t"
			 "casl %0,%1,%2\n\t"
			 "jne 1b"
			 : "=&d" (x) : "d" (x), "m" (*__xg(ptr)) : "memory");
		break;
	default:
		x = __invalid_xchg_size(x, ptr, size);
		break;
	}
	return x;
}
#endif

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))

#include <asm-generic/cmpxchg-local.h>

#define cmpxchg64_local(ptr, o, n) __cmpxchg64_local_generic((ptr), (o), (n))

extern unsigned long __invalid_cmpxchg_size(volatile void *,
					    unsigned long, unsigned long, int);

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
#ifdef CONFIG_RMW_INSNS

static inline unsigned long __cmpxchg(volatile void *p, unsigned long old,
				      unsigned long new, int size)
{
	switch (size) {
	case 1:
		__asm__ __volatile__ ("casb %0,%2,%1"
				      : "=d" (old), "=m" (*(char *)p)
				      : "d" (new), "0" (old), "m" (*(char *)p));
		break;
	case 2:
		__asm__ __volatile__ ("casw %0,%2,%1"
				      : "=d" (old), "=m" (*(short *)p)
				      : "d" (new), "0" (old), "m" (*(short *)p));
		break;
	case 4:
		__asm__ __volatile__ ("casl %0,%2,%1"
				      : "=d" (old), "=m" (*(int *)p)
				      : "d" (new), "0" (old), "m" (*(int *)p));
		break;
	default:
		old = __invalid_cmpxchg_size(p, old, new, size);
		break;
	}
	return old;
}

#define cmpxchg(ptr, o, n)						    \
	((__typeof__(*(ptr)))__cmpxchg((ptr), (unsigned long)(o),	    \
			(unsigned long)(n), sizeof(*(ptr))))
#define cmpxchg_local(ptr, o, n)					    \
	((__typeof__(*(ptr)))__cmpxchg((ptr), (unsigned long)(o),	    \
			(unsigned long)(n), sizeof(*(ptr))))

#define cmpxchg64(ptr, o, n)	cmpxchg64_local((ptr), (o), (n))

#else

/*
 * cmpxchg_local and cmpxchg64_local are atomic wrt current CPU. Always make
 * them available.
 */
#define cmpxchg_local(ptr, o, n)				  	       \
	((__typeof__(*(ptr)))__cmpxchg_local_generic((ptr), (unsigned long)(o),\
			(unsigned long)(n), sizeof(*(ptr))))

#include <asm-generic/cmpxchg.h>

#endif

#endif /* __ARCH_M68K_CMPXCHG__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /****************************************************************************/

/*
 *	coldfire.h -- Motorola ColdFire CPU sepecific defines
 *
 *	(C) Copyright 1999-2006, Greg Ungerer (gerg@snapgear.com)
 *	(C) Copyright 2000, Lineo (www.lineo.com)
 */

/****************************************************************************/
#ifndef	coldfire_h
#define	coldfire_h
/****************************************************************************/


/*
 *	Define master clock frequency. This is done at config time now.
 *	No point enumerating dozens of possible clock options here. And
 *	in any case new boards come along from time to time that have yet
 *	another different clocking frequency.
 */
#ifdef CONFIG_CLOCK_FREQ
#define	MCF_CLK		CONFIG_CLOCK_FREQ
#else
#error "Don't know what your ColdFire CPU clock frequency is??"
#endif

/*
 *	Define the processor internal peripherals base address.
 *
 *	The majority of ColdFire parts use an MBAR register to set
 *	the base address. Some have an IPSBAR register instead, and it
 *	has slightly different rules on its size and alignment. Some
 *	parts have fixed addresses and the internal peripherals cannot
 *	be relocated in the CPU address space.
 *
 *	The value of MBAR or IPSBAR is config time selectable, we no
 *	longer hard define it here. No MBAR or IPSBAR will be defined if
 *	this part has a fixed peripheral address map.
 */
#ifdef CONFIG_MBAR
#define	MCF_MBAR	CONFIG_MBAR
#endif
#ifdef CONFIG_IPSBAR
#define	MCF_IPSBAR	CONFIG_IPSBAR
#endif

/****************************************************************************/
#endif	/* coldfire_h */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    