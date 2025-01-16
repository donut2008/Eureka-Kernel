;
		} else {
			poll_time = MIN_CPE_POLL_INTERVAL;

			printk(KERN_WARNING "Returning to interrupt driven CPE handler\n");
			enable_irq(local_vector_to_irq(IA64_CPE_VECTOR));
			cpe_poll_enabled = 0;
		}

		if (cpe_poll_enabled)
			mod_timer(&cpe_poll_timer, jiffies + poll_time);
		start_count = -1;
	}

	return IRQ_HANDLED;
}

/*
 *  ia64_mca_cpe_poll
 *
 *	Poll for Corrected Platform Errors (CPEs), trigger interrupt
 *	on first cpu, from there it will trickle through all the cpus.
 *
 * Inputs   :   dummy(unused)
 * Outputs  :   None
 *
 */
static void
ia64_mca_cpe_poll (unsigned long dummy)
{
	/* Trigger a CPE interrupt cascade  */
	platform_send_ipi(cpumask_first(cpu_online_mask), IA64_CPEP_VECTOR,
							IA64_IPI_DM_INT, 0);
}

#endif /* CONFIG_ACPI */

static int
default_monarch_init_process(struct notifier_block *self, unsigned long val, void *data)
{
	int c;
	struct task_struct *g, *t;
	if (val != DIE_INIT_MONARCH_PROCESS)
		return NOTIFY_DONE;
#ifdef CONFIG_KEXEC
	if (atomic_read(&kdump_in_progress))
		return NOTIFY_DONE;
#endif

	/*
	 * FIXME: mlogbuf will brim over with INIT stack dumps.
	 * To enable show_stack from INIT, we use oops_in_progress which should
	 * be used in real oops. This would cause something wrong after INIT.
	 */
	BREAK_LOGLEVEL(console_loglevel);
	ia64_mlogbuf_dump_from_init();

	printk(KERN_ERR "Processes interrupted by INIT -");
	for_each_online_cpu(c) {
		struct ia64_sal_os_state *s;
		t = __va(__per_cpu_mca[c] + IA64_MCA_CPU_INIT_STACK_OFFSET);
		s = (struct ia64_sal_os_state *)((char *)t + MCA_SOS_OFFSET);
		g = s->prev_task;
		if (g) {
			if (g->pid)
				printk(" %d", g->pid);
			else
				printk(" %d (cpu %d task 0x%p)", g->pid, task_cpu(g), g);
		}
	}
	printk("\n\n");
	if (read_trylock(&tasklist_lock)) {
		do_each_thread (g, t) {
			printk("\nBacktrace of pid %d (%s)\n", t->pid, t->comm);
			show_stack(t, NULL);
		} while_each_thread (g, t);
		read_unlock(&tasklist_lock);
	}
	/* FIXME: This will not restore zapped printk locks. */
	RESTORE_LOGLEVEL(console_loglevel);
	return NOTIFY_DONE;
}

/*
 * C portion of the OS INIT handler
 *
 * Called from ia64_os_init_dispatch
 *
 * Inputs: pointer to pt_regs where processor info was saved.  SAL/OS state for
 * this event.  This code is used for both monarch and slave INIT events, see
 * sos->monarch.
 *
 * All INIT events switch to the INIT stack and change the previous process to
 * blocked status.  If one of the INIT events is the monarch then we are
 * probably processing the nmi button/command.  Use the monarch cpu to dump all
 * the processes.  The slave INIT events all spin until the monarch cpu
 * returns.  We can also get INIT slave events for MCA, in which case the MCA
 * process is the monarch.
 */

void
ia64_init_handler(struct pt_regs *regs, struct switch_stack *sw,
		  s