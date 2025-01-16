truct ia64_mca_cpu);
	int cpu = smp_processor_id();
	static int first_time = 1;

	/*
	 * Structure will already be allocated if cpu has been online,
	 * then offlined.
	 */
	if (__per_cpu_mca[cpu]) {
		data = __va(__per_cpu_mca[cpu]);
	} else {
		if (first_time) {
			data = mca_bootmem();
			first_time = 0;
		} else
			data = (void *)__get_free_pages(GFP_ATOMIC,
							get_order(sz));
		if (!data)
			panic("Could not allocate MCA memory for cpu %d\n",
					cpu);
	}
	format_mca_init_stack(data, offsetof(struct ia64_mca_cpu, mca_stack),
		"MCA", cpu);
	format_mca_init_stack(data, offsetof(struct ia64_mca_cpu, init_stack),
		"INIT", cpu);
	__this_cpu_write(ia64_mca_data, (__per_cpu_mca[cpu] = __pa(data)));

	/*
	 * Stash away a copy of the PTE needed to map the per-CPU page.
	 * We may need it during MCA recovery.
	 */
	__this_cpu_write(ia64_mca_per_cpu_pte,
		pte_val(mk_pte_phys(__pa(cpu_data), PAGE_KERNEL)));

	/*
	 * Also, stash away a copy of the PAL address and the PTE
	 * needed to map it.
	 */
	pal_vaddr = efi_get_pal_addr();
	if (!pal_vaddr)
		return;
	__this_cpu_write(ia64_mca_pal_base,
		GRANULEROUNDDOWN((unsigned long) pal_vaddr));
	__this_cpu_write(ia64_mca_pal_pte, pte_val(mk_pte_phys(__pa(pal_vaddr),
							      PAGE_KERNEL)));
}

static void ia64_mca_cmc_vector_adjust(void *dummy)
{
	unsigned long flags;

	local_irq_save(flags);
	if (!cmc_polling_enabled)
		ia64_mca_cmc_vector_enable(NULL);
	local_irq_restore(flags);
}

static int mca_cpu_callback(struct notifier_block *nfb,
				      unsigned long action,
				      void *hcpu)
{
	int hotcpu = (unsigned long) hcpu;

	switch (action) {
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		smp_call_function_single(hotcpu, ia64_mca_cmc_vector_adjust,
					 NULL, 0);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block mca_cpu_notifier = {
	.notifier_call = mca_cpu_callback
};

/*
 * ia64_mca_init
 *
 *  Do all the system level mca specific initialization.
 *
 *	1. Register spinloop and wakeup request interrupt vectors
 *
 *	2. Register OS_MCA handler entry point
 *
 *	3. Register OS_INIT handler entry point
 *
 *  4. Initialize MCA/CMC/INIT related log buffers maintained by the OS.
 *
 *  Note that this initialization is done very early before some kernel
 *  services are available.
 *
 *  Inputs  :   None
 *
 *  Outputs :   None
 */
void __init
ia64_mca_init(void)
{
	ia64_fptr_t *init_hldlr_ptr_monarch = (ia64_fptr_t *)ia64_os_init_dispatch_monarch;
	ia64_fptr_t *init_hldlr_ptr_slave = (ia64_fptr_t *)ia64_os_init_dispatch_slave;
	ia64_fptr_t *mca_hldlr_ptr = (ia64_fptr_t *)ia64_os_mca_dispatch;
	int i;
	long rc;
	struct ia64_sal_retval isrv;
	unsigned long timeout = IA64_MCA_RENDEZ_TIMEOUT; /* platform specific */
	static struct notifier_block default_init_monarch_nb = {
		.notifier_call = default_monarch_init_process,
		.priority = 0/* we need to notified last */
	};

	IA64_MCA_DEBUG("%s: begin\n", __func__);

	/* Clear the Rendez checkin flag for all cpus */
	for(i = 0 ; i < NR_CPUS; i++)
		ia64_mc_info.imi_rendez_checkin[i] = IA64_MCA_RENDEZ_CHECKIN_NOTDONE;

	/*
	 * Register the rendezvous spinloop and wakeup mechanism with SAL
	 */

	/* Register the rendezvous interrupt vector with SAL */
	while (1) {
		isrv = ia64_sal_mc_set_params(SAL_MC_PARAM_RENDEZ_INT,
					      SAL_MC_PARAM_MECHANISM_INT,
					      IA64_MCA_RENDEZ_VECTOR,
					      timeout,
					      SAL_MC_PARAM_RZ_ALWAYS);
		rc = isrv.status;
		if (rc == 0)
			break;
		if (rc == -2) {
			printk(KERN_INFO "Increasing MCA rendezvous timeout from "
				"%ld to %ld milliseconds\n", timeout, isrv.v0);
			timeout = isrv.v0;
			NOTIFY_MCA(DIE_MCA_NEW_TIMEOUT, NULL, timeout, 0);
			continue;
		}
		printk(KERN_ERR "Failed to register rendezvous interrupt "
		       "with SAL (status %ld)\n", rc);
		return;
	}

	/* Register the wakeup interrupt vector with SAL */
	isrv = ia64_sal_mc_set_params(SAL_MC_PARAM_RENDEZ_WAKEUP,
				      SAL_MC_PARAM_MECHANISM_INT,
				      IA64_MCA_WAKEUP_VECTOR,
				      0, 0);
	rc = isrv.status;
	if (rc) {
		printk(KERN_ERR "Failed to register wakeup interrupt with SAL "
		       "(status %ld)\n", rc);
		return;
	}

	IA64_MCA_DEBUG("%s: registered MCA rendezvous spinloop and wakeup mech.\n", __func__);

	ia64_mc_info.imi_mca_handler        = ia64_tpa(mca_hldlr_ptr->fp);
	/*
	 * XXX - disable 