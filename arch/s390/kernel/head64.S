ut_user((x) >> 16, ((u16 __user *)(ptr) + 1))); break;		\
		case 8: __ret = (__put_user((x), (u32 __user *)(ptr)))				\
			| (__put_user((x) >> 32, ((u32 __user *)(ptr) + 1))); break;		\
		default: __ret = __put_user_unaligned_unknown();				\
	}											\
	__ret;											\
})

extern long __get_user_unaligned_unknown (void);

#define __get_user_unaligned(x, ptr)								\
({												\
	long __ret;										\
	switch (sizeof(*(ptr))) {								\
		case 1: __ret = __get_user((x), (ptr)); break;					\
		case 2: __ret = (__get_user((x), (u8 __user *)(ptr)))				\
			| (__get_user((x) >> 8, ((u8 __user *)(ptr) + 1))); break;		\
		case 4: __ret = (__get_user((x), (u16 __user *)(ptr)))				\
			| (__get_user((x) >> 16, ((u16 __user *)(ptr) + 1))); break;		\
		case 8: __ret = (__get_user((x), (u32 __user *)(ptr)))				\
			| (__get_user((x) >> 32, ((u32 __user *)(ptr) + 1))); break;		\
		default: __ret = __get_user_unaligned_unknown();				\
	}											\
	__ret;											\
})

#ifdef ASM_SUPPORTED
  struct __large_struct { unsigned long buf[100]; };
# define __m(x) (*(struct __large_struct __user *)(x))

/* We need to declare the __ex_table section before we can use it in .xdata.  */
asm (".section \"__ex_table\", \"a\"\n\t.previous");

# define __get_user_size(val, addr, n, err)							\
do {												\
	register long __gu_r8 asm ("r8") = 0;							\
	register long __gu_r9 asm ("r9");							\
	asm ("\n[1:]\tld"#n" %0=%2%P2\t// %0 and %1 get overwritten by exception handler\n"	\
	     "\t.xdata4 \"__ex_table\", 1b-., 1f-.+4\n"						\
	     "[1:]"										\
	     : "=r"(__gu_r9), "=r"(__gu_r8) : "m"(__m(addr)), "1"(__gu_r8));			\
	(err) = __gu_r8;									\
	(val) = __gu_r9;									\
} while (0)

/*
 * The "__put_user_size()" macro tells gcc it reads from memory instead of writing it.  This
 * is because they do not write to any memory gcc knows about, so there are no aliasing
 * issues.
 */
# define __put_user_size(val, addr, n, err)							\
do {												\
	register long __pu_r8 asm ("r8") = 0;							\
	asm volatile ("\n[1:]\tst"#n" %1=%r2%P1\t// %0 gets overwritten by exception handler\n"	\
		      "\t.xdata4 \"__ex_table\", 1b-., 1f-.\n"					\
		      "[1:]"									\
		      : "=r"(__pu_r8) : "m"(__m(addr)), "rO"(val), "0"(__pu_r8));		\
	(err) = __pu_r8;									\
} while (0)

#else /* !ASM_SUPPORTED */
# define RELOC_TYPE	2	/* ip-rel */
# define __get_user_size(val, addr, n, err)				\
do {									\
	__ld_user("__ex_table", (unsigned long) addr, n, RELOC_TYPE);	\
	(err) = ia64_getreg(_IA64_REG_R8);				\
	(val) = ia64_getreg(_IA64_REG_R9);				\
} while (0)
# define __put_user_size(val, addr, n, err)				\
do {									\
	__st_user("__ex_table", (unsigned long) addr, n, RELOC_TYPE,	\
		  (__force unsigned long) (val));			\
	(err) = ia64_getreg(_IA64_REG_R8);				\
} while (0)
#endif /* !ASM_SUPPORTED */

extern void __get_user_unknown (void);

/*
 * Evaluating arguments X, PTR, SIZE, and SEGMENT may involve subroutine-calls, which
 * could clobber r8 and r9 (among others).  Thus, be careful not to evaluate it while
 * using r8/r9.
 */
#define __do_get_user(check, x, ptr, size, segment)					\
({											\
	const __typeof__(*(ptr)) __user *__gu_ptr = (ptr);				\
	__typeof__ (size) __gu_size = (size);						\
	long __gu_err = -EFAULT;							\
	unsigned long __gu_val = 0;							\
	if (!check || __access_ok(_