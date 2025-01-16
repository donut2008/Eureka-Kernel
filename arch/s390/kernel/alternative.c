ty)){
			printk (KERN_WARNING
				"%s: cannot override the interrupt\n",
				__func__);
			return -EINVAL;
		}
		rte->refcnt++;
		iosapic_intr_info[irq].count++;
		iosapic_lists[index].rtes_inuse++;
	}

	iosapic_intr_info[irq].polarity = polarity;
	iosapic_intr_info[irq].dmode    = delivery;
	iosapic_intr_info[irq].trigger  = trigger;

	irq_type = iosapic_get_irq_chip(trigger);

	chip = irq_get_chip(irq);
	if (irq_type != NULL && chip != irq_type) {
		if (chip != &no_irq_chip)
			printk(KERN_WARNING
			       "%s: changing vector %d from %s to %s\n",
			       __func__, irq_to_vector(irq),
			       chip->name, irq_type->name);
		chip = irq_type;
	}
	irq_set_chip_handler_name_locked(irq_get_irq_data(irq), chip,
		trigger == IOSAPIC_EDGE ? handle_edge_irq : handle_level_irq,
		NULL);
	return 0;
}

static unsigned int
get_target_cpu (unsigned int gsi, int irq)
{
#ifdef CONFIG_SMP
	static int cpu = -1;
	extern int cpe_vector;
	cpumask_t domain = irq_to_domain(irq);

	/*
	 * In case of vector shared by multiple RTEs, all RTEs that
	 * share the vector need to use the same destination CPU.
	 */
	if (iosapic_intr_info[irq].count)
		return iosapic_intr_info[irq].dest;

	/*
	 * If the platform supports redirection via XTP, let it
	 * distribute interrupts.
	 */
	if (smp_int_redirect & SMP_IRQ_REDIRECTION)
		return cpu_physical_id(smp_processor_id());

	/*
	 * Some interrupts (ACPI SCI, for instance) are registered
	 * before the BSP is marked as online.
	 */
	if (!cpu_online(smp_processor_id()))
		return cpu_physical_id(smp_processor_id());

#ifdef CONFIG_ACPI
	if (cpe_vector > 0 && irq_to_vector(irq) == IA64_CPEP_VECTOR)
		return get_cpei_target_cpu();
#endif

#ifdef CONFIG_NUMA
	{
		int num_cpus, cpu_index, iosapic_index, numa_cpu, i = 0;
		const struct cpumask *cpu_mask;

		iosapic_index = find_iosapic(gsi);
		if (iosapic_index < 0 ||
		    iosapic_lists[iosapic_index].node == MAX_NUMNODES)
			goto skip_numa_setup;

		cpu_mask = cpumask_of_node(iosapic_lists[iosapic_index].node);
		num_cpus = 0;
		for_each_cpu_and(numa_cpu, cpu_mask, &domain) {
			if (cpu_online(numa_cpu))
				num_cpus++;
		}

		if (!num_cpus)
			goto skip_numa_setup;

		/* Use irq assignment to distribute across cpus in node */
		cpu_index = irq % num_cpus;

		for_each_cpu_and(numa_cpu, cpu_mask, &domain)
			if (cpu_online(numa_cpu) && i++ >= cpu_index)
				break;

		if (numa_cpu < nr_cpu_ids)
			return cpu_physical_id(numa_cpu);
	}
skip_numa_setup:
#endif
	/*
	 * Otherwise, round-robin interrupt vectors across all the
	 * processors.  (It'd be nice if we could be smarter in 