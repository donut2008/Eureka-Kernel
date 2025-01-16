d get their customers updated.
	 */
	if (sos->monarch && atomic_add_return(1, &monarchs) > 1) {
		mprintk(KERN_WARNING "%s: Demoting cpu %d to slave.\n",
			       __func__, cpu);
		atomic_dec(&monarchs);
		sos->monarch = 0;
	}

	if (!sos->monarch) {
		ia64_mc_info.imi_rendez_checkin[cpu] = IA64_MCA_RENDEZ_CHECKIN_INIT;

#ifdef CONFIG_KEXEC
		while (monarch_cpu == -1 && !atomic_read(&kdump_in_progress))
			udelay(1000);
#else
		while (monarch_cpu == -1)
			cpu_relax();	/* spin until monarch enters */
#endif

		NOTIFY_INIT(DIE_INIT_SLAVE_ENTER, regs, (long)&nd, 1);
		NOTIFY_INIT(DIE_INIT_SLAVE_PROCESS, regs, (long)&nd, 1);

#ifdef CONFIG_KEXEC
		while (monarch_cpu != -1 && !atomic_read(&kdump_in_progress))
			udelay(1000);
#else
		while (monarch_cpu != -1)
			cpu_relax();	/* spin until monarch leaves */
#endif

		NOTIFY_INIT(DIE_INIT_SLAVE_LEAVE, regs, (long)&nd, 1);

		mprintk("Slave on cpu %d returning to normal service.\n", cpu);
		set_curr_task(cpu, previous_current);
		ia64_mc_info.imi_rendez_checkin[cpu] = IA64_MCA_RENDEZ_CHECKIN_NOTDONE;
		atomic_dec(&slaves);
		return;
	}

	monarch_cpu = cpu;
	NOTIFY_INIT(DIE_INIT_MONARCH_ENTER, regs, (long)&nd, 1);

	/*
	 * Wait for a bit.  On some machines (e.g., HP's zx2000 and zx6000, INIT can be
	 * generated via the BMC's command-line interface, but since the console is on the
	 * same serial line, the user will need some time to switch out of the BMC before
	 * the dump begins.
	 */
	mprintk("Delaying for 5 seconds...\n");
	udelay(5*1000000);
	ia64_wait_for_slaves(cpu, "INIT");
	/* If nobody intercepts DIE_INIT_MONARCH_PROCESS then we drop through
	 * to default_monarch_init_process() above and just print all the
	 * tasks.
	 */
	NOTIFY_INIT(DIE_INIT_MONARCH_PROCESS, regs, (long)&nd, 1);
	NOTIFY_INIT(DIE_INIT_MONARCH_LEAVE, regs, (long)&nd, 1);

	mprintk("\nINIT dump complete.  Monarch on cpu %d returning to normal service.\n", cpu);
	atomic_dec(&monarchs);
	set_curr_task(cpu, previous_current);
	monarch_cpu = -1;
	return;
}

static int __init
ia64_mca_disable_cpe_polling(char *str)
{
	cpe_poll_enabled = 0;
	return 1;
}

__setup("disable_cpe_poll", ia64_mca_disable_cpe_polling);

static struct irqaction cmci_irqaction = {
	.handler =	ia64_mca_cmc_int_handler,
	.name =		"cmc_hndlr"
};

static struct irqaction cmcp_irqaction = {
	.handler =	ia64_mca_cmc_int_caller,
	.name =		"cmc_poll"
};

static struct irqaction mca_rdzv_irqaction = {
	.handler =	ia64_mca_rendez_int_handler,
	.name =		"mca_rdzv"
};

static struct irqaction mca_wkup_irqaction = {
	.handler =	ia64_mca_wakeup_int_handler,
	.name =		"mca_wkup"
};

#ifdef CONFIG_ACPI
static struct irqaction mca_cpe_irqaction = {
	.handler =	ia64_mca_cpe_int_handler,
	.name =		"cpe_hndlr"
};

static struct irqaction mca_cpep_irqaction = {
	.handler =	ia64_mca_cpe_int_caller,
	.name =		"cpe_poll"
};
#endif /* CONFIG_ACPI */

/* Minimal format of the MCA/INIT stacks.  The pseudo processes that run on
 * these stacks can never sleep, they cannot return from the kernel to user
 * space, they do not appear in a normal ps listing.  So there is no need to
 * format most of the fields.
 */

static void
format_mca_init_stack(void *mca_data, unsigned long offset,
		const char *type, int cpu)
{
	struct task_struct *p = (struct task_struct *)((char *)mca_data + offset);
	struct thread_info *ti;
	memset(p, 0, KERNEL_STACK_SIZE);
	ti = task_thread_info(p);
	ti->flags = _TIF_MCA_INIT;
	ti->preempt_count = 1;
	ti->task = p;
	ti->cpu = cpu;
	p->stack = ti;
	p->state = TASK_UNINTERRUPTIBLE;
	cpumask_set_cpu(cpu, &p->cpus_allowed);
	INIT_LIST_HEAD(&p->tasks);
	p->parent = p->real_parent = p->group_leader = p;
	INIT_LIST_HEAD(&p->children);
	INIT_LIST_HEAD(&p->sibling);
	strncpy(p->comm, type, sizeof(p->comm)-1);
}

/* Caller prevents this from being called after init */
static void * __init_refok mca_bootmem(void)
{
	return __alloc_bootmem(sizeof(struct ia64_mca_cpu),
	                    KERNEL_STACK_SIZE, 0);
}

/* Do per-CPU MCA-related initialization.  */
void
ia64_mca_cpu_init(void *cpu_data)
{
	void *pal_vaddr;
	void *data;
	long sz = 