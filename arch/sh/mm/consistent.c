s address.
 */
static inline __u64
ia64_unat_pos (void *spill_addr)
{
	return ((__u64) spill_addr >> 3) & 0x3f;
}

/*
 * Set the NaT bit of an integer register which was spilled at address
 * SPILL_ADDR.  UNAT is the mask to be updated.
 */
static inline void
ia64_set_unat (__u64 *unat, void *spill_addr, unsigned long nat)
{
	__u64 bit = ia64_unat_pos(spill_addr);
	__u64 mask = 1UL << bit;

	*unat = (*unat & ~mask) | (nat << bit);
}

/*
 * Return saved PC of a blocked thread.
 * Note that the only way T can block is through a call to schedule() -> switch_to().
 */
static inline unsigned long
thread_saved_pc (struct task_struct *t)
{
	struct unw_frame_info info;
	unsigned long ip;

	unw_init_from_blocked_task(&info, t);
	if (unw_unwind(&info) < 0)
		return 0;
	unw_get_ip(&info, &ip);
	return ip;
}

/*
 * Get the current instruction/program counter value.
 */
#define current_text_addr() \
	({ void *_pc; _pc = (void *)ia64_getreg(_IA64_REG_IP); _pc; })

static inline __u64
ia64_get_ivr (void)
{
	__u64 r;
	ia64_srlz_d();
	r = ia64_getreg(_IA64_REG_CR_IVR);
	ia64_srlz_d();
	return r;
}

static inline void
ia64_set_dbr (__u64 regnum, __u64 value)
{
	__ia64_set_dbr(regnum, value);
#ifdef CONFIG_ITANIUM
	ia64_srlz_d();
#endif
}

static inline __u64
ia64_get_dbr (__u64 regnum)
{
	__u64 retval;

	retval = __ia64_get_dbr(regnum);
#ifdef CONFIG_ITANIUM
	ia64_srlz_d();
#endif
	return retval;
}

static inline __u64
ia64_rotr (__u64 w, __u64 n)
{
	return (w >> n) | (w << (64 - n));
}

#define ia64_rotl(w,n)	ia64_rotr((w), (64) - (n))

/*
 * Take a mapped kernel address and return the equivalent address
 * in the region 7 identity mapped virtual area.
 */
static inline void *
ia64_imva (void *addr)
{
	void *result;
	result = (void *) ia64_tpa(addr);
	return __va(result);
}

#define ARCH_HAS_PREFETCH
#define ARCH_HAS_PREFETCHW
#define ARCH_HAS_SPINLOCK_PREFETCH
#define PREFETCH_STRIDE			L1_CACHE_BYTES

static inline void
prefetch (const void *x)
{
	 ia64_lfetch(ia64_lfhint_none, x);
}

static inline void
prefetchw (const void *x)
{
	ia64_lfetch_excl(ia64_lfhint_none, x);
}

#define spin_lock_prefetch(x)	prefetchw(x)

extern unsigned long boot_option_idle_override;

enum idle_boot_override {IDLE_NO_OVERRIDE=0, IDLE_HALT, IDLE_FORCE_MWAIT,
			 IDLE_NOMWAIT, IDLE_POLL};

void default_idle(void);

#define ia64_platform_is(x) (strcmp(x, ia64_platform_name) == 0)

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_IA64_PROCESSOR_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     