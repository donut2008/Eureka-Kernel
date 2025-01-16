de that has preemption disabled.
 */
#define ia64_set_local_fpu_owner(t) do {						\
	struct task_struct *__ia64_slfo_task = (t);					\
	__ia64_slfo_task->thread.last_fph_cpu = smp_processor_id();			\
	ia64_set_kr(IA64_KR_FPU_OWNER, (unsigned long) __ia64_slfo_task);		\
} while (0)

/* Mark the fph partition of task T as being invalid on all CPUs.  */
#define ia64_drop_fpu(t)	((t)->thread.last_fph_cpu = -1)

extern void __ia64_init_fpu (void);
extern void __ia64_save_fpu (struct ia64_fpreg *fph);
extern void __ia64_load_fpu (struct ia64_fpreg *fph);
extern void ia64_save_debug_regs (unsigned long *save_area);
extern void ia64_load_debug_regs (unsigned long *save_area);

#define ia64_fph_enable()	do { ia64_rsm(IA64_PSR_DFH); ia64_srlz_d(); } while (0)
#define ia64_fph_disable()	do { ia64_ssm(IA64_PSR_DFH); ia64_srlz_d(); } while (0)

/* load fp 0.0 into fph */
static inline void
ia64_init_fpu (void) {
	ia64_fph_enable();
	__ia64_init_fpu();
	ia64_fph_disable();
}

/* save f32-f127 at FPH */
static inline void
ia64_save_fpu (struct ia64_fpreg *fph) {
	ia64_fph_enable();
	__ia64_save_fpu(fph);
	ia64_fph_disable();
}

/* load f32-f127 from FPH */
static inline void
ia64_load_fpu (struct ia64_fpreg *fph) {
	ia64_fph_enable();
	__ia64_load_fpu(fph);
	ia64_fph_disable();
}

static inline __u64
ia64_clear_ic (void)
{
	__u64 psr;
	psr = ia64_getreg(_IA64_REG_PSR);
	ia64_stop();
	ia64_rsm(IA64_PSR_I | IA64_PSR_IC);
	ia64_srlz_i();
	return psr;
}

/*
 * Restore the psr.
 */
static inline void
ia64_set_psr (__u64 psr)
{
	ia64_stop();
	ia64_setreg(_IA64_REG_PSR_L, psr);
	ia64_srlz_i();
}

/*
 * Insert a translation into an instruction and/or data translation
 * register.
 */
static inline void
ia64_itr (__u64 target_mask, __u64 tr_num,
	  __u64 vmaddr, __u64 pte,
	  __u64 log_page_size)
{
	ia64_setreg(_IA64_REG_CR_ITIR, (log_page_size << 2));
	ia64_setreg(_IA64_REG_CR_IFA, vmaddr);
	ia64_stop();
	if (target_mask & 0x1)
		ia64_itri(tr_num, pte);
	if (target_mask & 0x2)
		ia64_itrd(tr_num, pte);
}

/*
 * Insert a translation into the instruction and/or data translation
 * cache.
 */
static inline void
ia64_itc (__u64 target_mask, __u64 vmaddr, __u64 pte,
	  __u