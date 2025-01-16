BUG_ON(cfg->vector == IRQ_VECTOR_UNASSIGNED);
	vector = cfg->vector;
	domain = cfg->domain;
	for_each_cpu_and(cpu, &cfg->domain, cpu_online_mask)
		per_cpu(vector_irq, cpu)[vector] = -1;
	cfg->vector = IRQ_VECTOR_UNASSIGNED;
	cfg->domain = CPU_MASK_NONE;
	irq_status[irq] = IRQ_UNUSED;
	cpumask_andnot(&vector_table[vector], &vector_table[vector], &domain);
}

static void clear_irq_vector(int irq)
{
	unsigned long flags;

	spin_lock_irqsave(&vector_lock, flags);
	__clear_irq_vector(irq);
	spin_unlock_irqrestore(&vector_lock, flags);
}

int
ia64_native_assign_irq_vector (int irq)
{
	unsigned long flags;
	int vector, cpu;
	cpumask_t domain = CPU_MASK_NONE;

	vector = -ENOSPC;

	spin_lock_irqsave(&vector_lock, flags);
	for_each_online_cpu(cpu) {
		domain = vector_allocation_domain(cpu);
		vector = find_unassigned_vector(domain);
		if (vector >= 0)
			break;
	}
	if (vector < 0)
		goto out;
	if (irq == AUTO_ASSIGN)
		irq = vector;
	BUG_ON(__bind_irq_vector(irq, vector, domain));
 out:
	spin_unlock_irqrestore(&vector_lock, flags);
	return vector;
}

void
ia64_native_free_irq_vector (int vector)
{
	if (vector < IA64_FIRST_DEVICE_VECTOR ||
	    vector > IA64_LAST_DEVICE_VECTOR)
		return;
	clear_irq_vector(vector);
}

int
reserve_irq_vector (int vector)
{
	if (vector < IA64_FIRST_DEVICE_VECTOR ||
	    vector > IA64_LAST_DEVICE_VECTOR)
		return -EINVAL;
	return !!bind_irq_vector(vector, vector, CPU_MASK_ALL);
}

/*
 * Initialize vector_irq on a new cpu. This function must be called
 * with vector_lock held.
 */
void __setup_vector_irq(int cpu)
{
	int irq, vector;

	/* Clear vector_irq */
	for (vector = 0; vector < IA64_NUM_VECTORS; ++vector)
		per_cpu(vector_irq, cpu)[vector] = -1;
	/* Mark the inuse vectors */
	for (irq = 0; irq < NR_IRQS; ++irq) {
		if (!cpumask_test_cpu(cpu, &irq_cfg[irq].domain))
			continue;
		vector = irq_to_vector(irq);
		per_cpu(vector_irq, cpu)[vector] = irq;
	}
}

#if defined(CONFIG_SMP) && (defined(CONFIG_IA64_GENERIC) || defined(CONFIG_IA64_DIG))

static enum vector_domain_type {
	VECTOR_DOMAIN_NONE,
	VECTOR_DOMAIN_PERCPU
} vector_domain_type = VECTOR_DOMAIN_NONE;

static cpumask_t vector_allocation_domain(int cpu)
{
	if (vector_domain_type == VECTOR_DOMAIN_PERCPU)
		return *cpumask_of(cpu);
	return CPU_MASK_ALL;
}

static int __irq_prepare_move(int irq, int cpu)
{
	struct irq_cfg *cfg = &irq_cfg[irq];
	int vector;
	cpumask_t domain;

	if (cfg->move_in_progress || cfg->move_cleanup_count)
		return -EBUSY;
	if (cfg->vector == IRQ_VECTOR_UNASSIGNED || !cpu_online(cpu))
		return -EINVAL;
	if (cpumask_test_cpu(cpu, &cfg->domain))
		return 0;
	domain = vector_allocation_domain(cpu);
	vector = find_unassigned_vector(domain);
	if (vector < 0)
		return -ENOSPC;
	cfg->move_in_progress = 1;
	cfg->old_domain = cfg->domain;
	cfg->vector = IRQ_VECTOR_UNASSIGNED;
	cfg->domain = CPU_MASK_NONE;
	BUG_ON(__bind_irq_vector(irq, vector, domain));
	return 0;
}

int irq_prepare_move(int irq, int cpu)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&vector_lock, flags);
	ret = __irq_prepare_move(irq, cpu);
	spin_unlock_irqrestore(&vector_lock, flags);
	return ret;
}

void irq_complete_move(unsigned irq)
{
	struct irq_cfg *cfg = &irq_cfg[irq];
	cpumask_t cleanup_mask;
	int i;

	if (likely(!cfg->move_in_progress))
		return;

	if (unlikely(cpumask_test_cpu(smp_processor_id(), &cfg->old_domain)))
		return;

	cpumask_and(&cleanup_mask, &cfg->old_domain, cpu_online_mask);
	cfg->move_cleanup_count = cpumask_weight(&cleanup_mask);
	for_each_cpu(i, &cleanup_mask)
		platform_send_ipi(i, IA64_IRQ_MOVE_VECTOR, IA64_IPI_DM_INT, 0);
	cfg->move_in_progress = 0;
}

static irqreturn_t smp_irq_move_cleanup_interrupt(int irq, void *dev_id)
{
	int me = smp_processor_id();
	ia64_vector vector;
	unsigned long flags;

	for (vector = IA64_FIRST_DEVICE_VECTOR;
	     vector < IA64_LAST_DEVICE_VECTOR; vector++) {
		int irq;
		struct irq_desc *desc;
		struct irq_cfg *cfg;
		irq = __this_cpu_read(vector_irq[vector]);
		if (irq < 0)
			continue;

		desc = irq_to_desc(irq);
		cfg = irq_cfg + irq;
		raw_spin_lock(&desc->lock);
		if (!cfg->move_cleanup_count)
			goto unlock;

		if (!cpumask_test_cpu(me, &cfg->old_domain))
			goto unlock;

		spin_lock_irqsave(&vector_lock, flags);
		__this_cpu_write(vector_irq[vector], -1);
		cpumask_clear_cpu(me, &vector_table[vector]);
		spin_unlock_irqrestore(&vector_lock, flags);
		cfg->move_cleanup_count--;
	unlock:
		raw_spin_unlock(&desc->lock);
	}
	return IRQ_HANDLED;
}

static struct irqaction irq_move_irqaction = {
	.handler =	smp_irq_move_cleanup_interrupt,
	.name =		"irq_move"
};

static int __init parse_vector_domain(char *arg)
{
	if (!arg)
		return -EINVAL;
	if (!strcmp(arg, "percpu")) {
		vector_domain_type = VECTOR_DOMAIN_PERCPU;
		no_int_routing = 1;
	}
	return 0;
}
early_param("vector", parse_vector_domain);
#else
static cpumask_t vector_allocation_domain(int cpu)
{
	return CPU_MASK_ALL;
}
#endif


void destroy_and_reserve_irq(unsigned int irq)
{
	unsigned long flags;

	irq_init_desc(irq);
	spin_lock_irqsave(&vector_lock, flags);
	__clear_irq_vector(irq);
	irq_status[irq] = IRQ_RSVD;
	spin_unlock_irqrestore(&vector_lock, flags);
}

/*
 * Dynamic irq allocate and deallocation for MSI
 */
int create_irq(void)
{
	unsigned long flags;
	int irq, vector, cpu;
	cpumask_t domain = CPU_MASK_NONE;

	irq = vector = -ENOSPC;
	spin_lock_irqsave(&vector_lock, flags);
	for_each_online_cpu(cpu) {
		domain = vector_allocation_domain(cpu);
		vector = find_unassigned_vector(domain);
		if (vector >= 0)
			break;
	}
	if (vector < 0)
		goto out;
	irq = find_unassigned_irq();
	if (irq < 0)
		goto out;
	BUG_ON(__bind_irq_vector(irq, vector, domain));
 out:
	spin_unlock_irqrestore(&vector_lock, flags);
	if (irq >= 0)
		irq_init_desc(irq);
	return irq;
}

void destroy_irq(unsigned int irq)
{
	irq_init_desc(irq);
	clear_irq_vector(irq);
}

#ifdef CONFIG_SMP
#	define IS_RESCHEDULE(vec)	(vec == IA64_IPI_RESCHEDULE)
#	define IS_LOCAL_TLB_FLUSH(vec)	(vec == IA64_IPI_LOCAL_TLB_FLUSH)
#else
#	define IS_RESCHEDULE(vec)	(0)
#	define IS_LOCAL_TLB_FLUSH(vec)	(0)
#endif
/*
 * That's where the IVT branches when we get an external
 * interrupt. This branches to the correct hardware IRQ handler via
 * function ptr.
 */
void
ia64_handle_irq (ia64_vector vector, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	unsigned long saved_tpr;

#if IRQ_DEBUG
	{
		unsigned long bsp, sp;

		/*
		 * Note: if the interrupt happened while executing in
		 * the context switch routine (ia64_switch_to), we may
		 * get a spurious stack overflow here.  This is
		 * because the register and the memory stack are not
		 * switched atomically.
		 */
		bsp = ia64_getreg(_IA64_REG_AR_BSP);
		sp = ia64_getreg(_IA64_REG_SP);

		if ((sp - bsp) < 1024) {
			static DEFINE_RATELIMIT_STATE(ratelimit, 5 * HZ, 5);

			if (__ratelimit(&ratelimit)) {
				printk("ia64_handle_irq: DANGER: less than "
				       "1KB of free stack space!!\n"
				       "(bsp=0x%lx, sp=%lx)\n", bsp, sp);
			}
		}
	}
#endif /* IRQ_DEBUG */

	/*
	 * Always set TPR to limit maximum interrupt nesting depth to
	 * 16 (without this, it would be ~240, which could easily lead
	 * to kernel stack overflows).
	 */
	irq_enter();
	saved_tpr = ia64_getreg(_IA64_REG_CR_TPR);
	ia64_srlz_d();
	while (vector != IA64_SPURIOUS_INT_VECTOR) {
		int irq = local_vector_to_irq(vector);

		if (unlikely(IS_LOCAL_TLB_FLUSH(vector))) {
			smp_local_flush_tlb();
			kstat_incr_irq_this_cpu(irq);
		} else if (unlikely(IS_RESCHEDULE(vector))) {
			scheduler_ipi();
			kstat_incr_irq_this_cpu(irq);
		} else {
			ia64_setreg(_IA64_REG_CR_TPR, vector);
			ia64_srlz_d();

			if (unlikely(irq < 0)) {
				printk(KERN_ERR "%s: Unexpected interrupt "
				       "vector %d on CPU %d is not mapped "
				       "to any IRQ!\n", __func__, vector,
				       smp_processor_id());
			} else
				generic_handle_irq(irq);

			/*
			 * Disable interrupts and send EOI:
			 */
			local_irq_disable();
			ia64_setreg(_IA64_REG_CR_TPR, saved_tpr);
		}
		ia64_eoi();
		vector = ia64_get_ivr();
	}
	/*
	 * This must be done *after* the ia64_eoi().  For example, the keyboard softirq
	 * handler needs to be able to wait for further keyboard interrupts, which can't
	 * come through until ia64_eoi() has been done.
	 */
	irq_exit();
	set_irq_regs(old_regs);
}

#ifdef CONFIG_HOTPLUG_CPU
/*
 * This function emulates a interrupt processing when a cpu is about to be
 * brought down.
 */
void ia64_process_pending_intr(void)
{
	ia64_vector vector;
	unsigned long saved_tpr;
	extern unsigned int vectors_in_migration[NR_IRQS];

	vector = ia64_get_ivr();

	irq_enter();
	saved_tpr = ia64_getreg(_IA64_REG_CR_TPR);
	ia64_srlz_d();

	 /*
	  * Perform normal interrupt style processing
	  */
	while (vector != IA64_SPURIOUS_INT_VECTOR) {
		int irq = local_vector_to_irq(vector);

		if (unlikely(IS_LOCAL_TLB_FLUSH(vector))) {
			smp_local_flush_tlb();
			kstat_incr_irq_this_cpu(irq);
		}