e_addr) ||
					(regs->b6 == bundle_addr + 0x10)) {
					regs->b6 = (regs->b6 - bundle_addr) +
						resume_addr;
				}
				break;
			case 7:
				if ((regs->b7 == bundle_addr) ||
					(regs->b7 == bundle_addr + 0x10)) {
					regs->b7 = (regs->b7 - bundle_addr) +
						resume_addr;
				}
				break;
			} /* end switch */
		}
		goto turn_ss_off;
	}

	if (slot == 2) {
		if (regs->cr_iip == bundle_addr + 0x10) {
			regs->cr_iip = resume_addr + 0x10;
		}
	} else {
		if (regs->cr_iip == bundle_addr) {
			regs->cr_iip = resume_addr;
		}
	}

turn_ss_off:
	/* Turn off Single Step bit */
	ia64_psr(regs)->ss = 0;
}

static void __kprobes prepare_ss(struct kprobe *p, struct pt_regs *regs)
{
	unsigned long bundle_addr = (unsigned long) &p->ainsn.insn->bundle;
	unsigned long slot = (unsigned long)p->addr & 0xf;

	/* single step inline if break instruction */
	if (p->ainsn.inst_flag == INST_FLAG_BREAK_INST)
		regs->cr_iip = (unsigned long)p->addr & ~0xFULL;
	else
		regs->cr_iip = bundle_addr & ~0xFULL;

	if (slot > 2)
		slot = 0;

	ia64_psr(regs)->ri = slot;

	/* turn on single stepping */
	ia64_psr(regs)->ss = 1;
}

static int __kprobes is_ia64_break_inst(struct pt_regs *regs)
{
	unsigned int slot = ia64_psr(regs)->ri;
	unsigned long *kprobe_addr = (unsigned long *)regs->cr_iip;
	bundle_t bundle;

	memcpy(&bundle, kprobe_addr, sizeof(bundle_t));

	return __is_ia64_break_inst(&bundle, slot);
}

static int __kprobes pre_kprobes_handler(struct die_args *args)
{
	struct kprobe *p;
	int ret = 0;
	struct pt_regs *regs = args->regs;
	kprobe_opcode_t *addr = (kprobe_opcode_t *)instruction_pointer(regs);
	struct kprobe_ctlblk *kcb;

	/*
	 * We don't want to be preempted for the entire
	 * duration of kprobe processing
	 */
	preempt_disable();
	kcb = get_kprobe_ctlblk();

	/* Handle recursion cases */
	if (kprobe_running()) {
		p = get_kprobe(addr);
		if (p) {
			if ((kcb->kprobe_status == KPROBE_HIT_SS) &&
	 		     (p->ainsn.inst_flag == INST_FLAG_BREAK_INST)) {
				ia64_psr(regs)->ss = 0;
				goto no_kprobe;
			}
			/* We have reentered the pre_kprobe_handler(), since
			 * another probe was hit while within the handler.
			 * We here save the original kprobes variables and
			 * just single step on the instruction of the new probe
			 * without calling any user handlers.
			 */
			save_previous_kprobe(kcb);
			set_current_kprobe(p, kcb);
			kprobes_inc_nmissed_count(p);
			prepare_ss(p, regs);
			kcb->kprobe_status = KPROBE_REENTER;
			return 1;
		} else if (args->err == __IA64_BREAK_JPROBE) {
			/*
			 * jprobe instrumented function just completed
			 */
			p = __this_cpu_read(current_kprobe);
			if (p->break_handler && p->break_handler(p, regs)) {
				goto ss_probe;
			}
		} else if (!is_ia64_break_inst(regs)) {
			/* The breakpoint instruction was removed by
			 * another cpu right after we hit, no further
			 * handling of this interrupt is appropriate
			 */
			ret = 1;
			goto no_kprobe;
		} else {
			/* Not our break */
			goto no_kprobe;
		}
	}

	p = get_kprobe(addr);
	if (!p) {
		if (!is_ia64_break_inst(regs)) {
			/*
			 * The breakpoint instruction was removed right
			 * after we hit it.  Another cpu has removed
			 * either a probepoint or a debugger breakpoint
			 * at this address.  In either case, no further
			 * handling of this interrupt is appropriate.
			 */
			ret = 1;

		}

		/* Not one of our break, let kernel handle it */
		goto no_kprobe;
	}

	set_current_kprobe(p, kcb);
	kcb->kprobe_status = KPROBE_HIT_ACTIVE;

	if (p->pre_handler && p->pre_handler(p, regs))
		/*
		 * Our pre-handler is specifically requesting that we just
		 * do a return.  This is used for both the jprobe pre-handler
		 * and the kretprobe trampoline
		 */
		return 1;

ss_probe:
#if !defined(CONFIG_PREEMPT)
	if (p->ainsn.inst_flag == INST_FLAG_BOOSTABLE && !p->post_handler) {
		/* Boost up -- we can execute copied instructions directly */
		ia64_psr(regs)->ri = p->ainsn.slot;
		regs->cr_iip = (unsigned long)&p->ainsn.insn->bundle & ~0xFULL;
		/* turn single stepping off */
		ia64_psr(regs)->ss = 0;

		reset_current_kprobe();
		preempt_enable_no_resched();
		return 1;
	}
#endif
	prepare_ss(p, regs);
	kcb->kprobe_status = KPROBE_HIT_SS;
	return 1;

no_kprobe:
	preempt_enable_no_resched();
	return ret;
}

static int __kprobes post_kprobes_handler(struct pt_regs *regs)
{
	struct kprobe *cur = kprobe_running();
	struct kprobe_ctlblk *kcb = get_kprobe_ctlblk();

	if (!cur)
		return 0;

	if ((kcb->kprobe_status != KPROBE_REENTER) && cur->post_handler) {
		kcb->kprobe_status = KPROBE_HIT_SSDONE;
		cur->post_handler(cur, regs, 0);
	}

	resume_execution(cur, regs);

	/*Restore back the original saved kprobes variables and continue. */
	if (kcb->kprobe_status == KPROBE_REENTER) {
		restore_previous_kprobe(kcb);
		goto out;
	}
	reset_current_kprobe();

out:
	preempt_enable_no_resched();
	return 1;
}

int __kprobes kprobe_fault_handler(struct pt_regs *regs, int trapnr)
{
	struct kprobe *cur = kprobe_running();
	struct kprobe_ctlblk *kcb = get_kprobe_ctlblk();


	switch(kcb->kprobe_status) {
	case KPROBE_HIT_SS:
	case KPROBE_REENTER:
		/*
		 * We are here because the instruction being single
		 * stepped caused a page fault. We reset the current
		 * kprobe and the instruction pointer points back to
		 * the probe address and allow the page fault handler
		 * to continue as a normal page fault.
		 */
		regs->cr_iip = ((unsigned long)cur->addr) & ~0xFULL;
		ia64_psr(regs)->ri = ((unsigned long)cur->addr) & 0xf;
		if (kcb->kprobe_status == KPROBE_REENTER)
			restore_previous_kprobe(kcb);
		else
			reset_current_kprobe();
		preempt_enable_no_resched();
		break;
	case KPROBE_HIT_ACTIVE:
	case KPROBE_HIT_SSDONE:
		/*
		 * We increment the nmissed count for accounting,
		 * we can also use npre/npostfault count for accounting
		 * these specific fault cases.
		 */
		kprobes_inc_nmissed_count(cur);

		/*
		 * We come here because instructions in the pre/post
		 * handler caused the page_fault, this could happen
		 * if handler tries to access user space by
		 * copy_from_user(), get_user() etc. Let the
		 * user-specified handler try to fix it first.
		 */
		if (cur->fault_handler && cur->fault_handler(cur, regs, trapnr))
			return 1;
		/*
		 * In case the user-specified fault handler returned
		 * zero, try to fix up.
		 */
		if (ia64_done_with_exception(regs))
			return 1;

		/*
		 * Let ia64_do_page_fault() fix it.
		 */
		break;
	default:
		break;
	}

	return 0;
}

int __kprobes kprobe_exceptions_notify(struct notifier_block *self,
				       unsigned long val, void *data)
{
	struct die_args *args = (struct die_args *)data;
	int ret = NOTIFY_DONE;

	if (args->regs && user_mode(args->regs))
		return ret;

	switch(val) {
	case DIE_BREAK:
		/* err is break number from ia64_bad_break() */
		if ((args->err >> 12) == (__IA64_BREAK_KPROBE >> 12)
			|| args->err == __IA64_BREAK_JPROBE
			|| args->err == 0)
			if (pre_kprobes_handler(args))
				ret = NOTIFY_STOP;
		break;
	case DIE_FAULT:
		/* err is vector number from ia64_fault() */
		if (args->err == 36)
			if (post_kprobes_handler(args->regs))
				ret = NOTIFY_STOP;
		break;
	default:
		break;
	}
	return ret;
}

struct param_bsp_cfm {
	unsigned long ip;
	unsigned long *bsp;
	unsigned long cfm;
};

static void ia64_get_bsp_cfm(struct unw_frame_info *info, void *arg)
{
	unsigned long ip;
	struct param_bsp_cfm *lp = arg;

	do {
		unw_get_ip(info, &ip);
		if (ip == 0)
			break;
		if (ip == lp->ip) {
			unw_get_bsp(info, (unsigned long*)&lp->bsp);
			unw_get_cfm(info, (unsigned long*)&lp->cfm);
			return;
		}
	} while (unw_unwind(info) >= 0);
	lp->bsp = NULL;
	lp->cfm = 0;
	return;
}

unsigned long arch_deref_entry_point(void *entry)
{
	return ((struct fnptr *)entry)->ip;
}

int __kprobes setjmp_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct jprobe *jp = container_of(p, struct jprobe, kp);
	unsigned long addr = arch_deref_entry_point(jp->entry);
	struct kprobe_ctlblk *kcb = get_kprobe_ctlblk();
	struct param_bsp_cfm pa;
	int bytes;

	/*
	 * Callee owns the argument space and could overwrite it, eg
	 * tail call optimization. So to be absolutely safe
	 * we save the argument space before transferring the control
	 * to instrumented jprobe function which runs in
	 * the process context
	 */
	pa.ip = regs->cr_iip;
	unw_init_running(ia64_get_bsp_cfm, &pa);
	bytes = (char *)ia64_rse_skip_regs(pa.bsp, pa.cfm & 0x3f)
				- (char *)pa.bsp;
	memcpy( kcb->jprobes_saved_stacked_regs,
		pa.bsp,
		bytes );
	kcb->bsp = pa.bsp;
	kcb->cfm = pa.cfm;

	/* save architectural state */
	kcb->jprobe_saved_regs = *regs;

	/* after rfi, execute the jprobe instrumented function */
	regs->cr_iip = addr & ~0xFULL;
	ia64_psr(regs)->ri = addr & 0xf;
	regs->r1 = ((struct fnptr *)(jp->entry))->gp;

	/*
	 * fix the return address to our jprobe_inst_return() function
	 * in the jprobes.S file
	 */
	regs->b0 = ((struct fnptr *)(jprobe_inst_return))->ip;

	return 1;
}

/* ia64 does not need this */
void __kprobes jprobe_return(void)
{
}

int __kprobes longjmp_break_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct kprobe_ctlblk *kcb = get_kprobe_ctlblk();
	int bytes;

	/* restoring architectural state */
	*regs = kcb->jprobe_saved_regs;

	/* restoring the original argument space */
	flush_register_stack();
	bytes = (char *)ia64_rse_skip_regs(kcb->bsp, kcb->cfm & 0x3f)
				- (char *)kcb->bsp;
	memcpy( kcb->bsp,
		kcb->jprobes_saved_stacked_regs,
		bytes );
	invalidate_stacked_regs();

	preempt_enable_no_resched();
	return 1;
}

static struct kprobe trampoline_p = {
	.pre_handler = trampoline_probe_handler
};

int __init arch_init_kprobes(void)
{
	trampoline_p.addr =
		(kprobe_opcode_t *)((struct fnptr *)kretprobe_trampoline)->ip;
	return register_kprobe(&trampoline_p);
}

int __kprobes arch_trampoline_kprobe(struct kprobe *p)
{
	if (p->addr ==
		(kprobe_opcode_t *)((struct fnptr *)kretprobe_trampoline)->ip)
		return 1;

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * arch/ia64/kernel/machine_kexec.c
 *
 * Handle transition of Linux booting another kernel
 * Copyright (C) 2005 Hewlett-Packard Development Comapny, L.P.
 * Copyright (C) 2005 Khalid Aziz <khalid.aziz@hp.com>
 * Copyright (C) 2006 Intel Corp, Zou Nan hai <nanhai.zou@intel.com>
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <linux/mm.h>
#include <linux/kexec.h>
#include <linux/cpu.h>
#include <linux/irq.h>
#include <linux/efi.h>
#include <linux/numa.h>
#include <linux/mmzone.h>

#include <asm/numa.h>
#include <asm/mmu_context.h>
#include <asm/setup.h>
#include <asm/delay.h>
#include <asm/meminit.h>
#include <asm/processor.h>
#include <asm/sal.h>
#include <asm/mca.h>

typedef void (*relocate_new_kernel_t)(
					unsigned long indirection_page,
					unsigned long start_address,
					struct ia64_boot_param *boot_param,
					unsigned long pal_addr) __noreturn;

struct kimage *ia64_kimage;

struct resource efi_memmap_res = {
        .name  = "EFI Memory Map",
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_BUSY | IORESOURCE_MEM
};

struct resource boot_param_res = {
        .name  = "Boot parameter",
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_BUSY | IORESOURCE_MEM
};


/*
 * Do what every setup is needed on image and the
 * reboot code buffer to allow us to avoid allocations
 * later.
 */
int machine_kexec_prepare(struct kimage *image)
{
	void *control_code_buffer;
	const unsigned long *func;

	func = (unsigned long *)&relocate_new_kernel;
	/* Pre-load control code buffer to minimize work in kexec path */
	control_code_buffer = page_address(image->control_code_page);
	memcpy((void *)control_code_buffer, (const void *)func[0],
			relocate_new_kernel_size);
	flush_icache_range((unsigned long)control_code_buffer,
			(unsigned long)control_code_buffer + relocate_new_kernel_size);
	ia64_kimage = image;

	return 0;
}

void machine_kexec_cleanup(struct kimage *image)
{
}

/*
 * Do not allocate memory (or fail in any way) in machine_kexec().
 * We are past the point of no return, committed to rebooting now.
 */
static void ia64_machine_kexec(struct unw_frame_info *info, void *arg)
{
	struct kimage *image = arg;
	relocate_new_kernel_t rnk;
	void *pal_addr = efi_get_pal_addr();
	unsigned long code_addr;
	int ii;
	u64 fp, gp;
	ia64_fptr_t *init_handler = (ia64_fptr_t *)ia64_os_init_on_kdump;

	BUG_ON(!image);
	code_addr = (unsigned long)page_address(image->control_code_page);
	if (image->type == KEXEC_TYPE_CRASH) {
		crash_save_this_cpu();
		current->thread.ksp = (__u64)info->sw - 16;

		/* Register noop init handler */
		fp = ia64_tpa(init_handler->fp);
		gp = ia64_tpa(ia64_getreg(_IA64_REG_GP));
		ia64_sal_set_vectors(SAL_VECTOR_OS_INIT, fp, gp, 0, fp, gp, 0);
	} else {
		/* Unregister init handlers of current kernel */
		ia64_sal_set_vectors(SAL_VECTOR_OS_INIT, 0, 0, 0, 0, 0, 0);
	}

	/* Unregister mca handler - No more recovery on current kernel */
	ia64_sal_set_vectors(SAL_VECTOR_OS_MCA, 0, 0, 0, 0, 0, 0);

	/* Interrupts aren't acceptable while we reboot */
	local_irq_disable();

	/* Mask CMC and Performance Monitor interrupts */
	ia64_setreg(_IA64_REG_CR_PMV, 1 << 16);
	ia64_setreg(_IA64_REG_CR_CMCV, 1 << 16);

	/* Mask ITV and Local Redirect Registers */
	ia64_set_itv(1 << 16);
	ia64_set_lrr0(1 << 16);
	ia64_set_lrr1(1 << 16);

	/* terminate possible nested in-service interrupts */
	for (ii = 0; ii < 16; ii++)
		ia64_eoi();

	/* unmask TPR and clear any pending interrupts */
	ia64_setreg(_IA64_REG_CR_TPR, 0);
	ia64_srlz_d();
	while (ia64_get_ivr() != IA64_SPURIOUS_INT_VECTOR)
		ia64_eoi();
	platform_kernel_launch_event();
	rnk = (relocate_new_kernel_t)&code_addr;
	(*rnk)(image->head, image->start, ia64_boot_param,
		     GRANULEROUNDDOWN((unsigned long) pal_addr));
	BUG();
}

void machine_kexec(struct kimage *image)
{
	BUG_ON(!image);
	unw_init_running(ia64_machine_kexec, image);
	for(;;);
}

void arch_crash_save_vmcoreinfo(void)
{
#if defined(CONFIG_DISCONTIGMEM) || defined(CONFIG_SPARSEMEM)
	VMCOREINFO_SYMBOL(pgdat_list);
	VMCOREINFO_LENGTH(pgdat_list, MAX_NUMNODES);
#endif
#ifdef CONFIG_NUMA
	VMCOREINFO_SYMBOL(node_memblk);
	VMCOREINFO_LENGTH(node_memblk, NR_NODE_MEMBLKS);
	VMCOREINFO_STRUCT_SIZE(node_memblk_s);
	VMCOREINFO_OFFSET(node_memblk_s, start_paddr);
	VMCOREINFO_OFFSET(node_memblk_s, size);
#endif
#if CONFIG_PGTABLE_LEVELS == 3
	VMCOREINFO_CONFIG(PGTABLE_3);
#elif CONFIG_PGTABLE_LEVELS == 4
	VMCOREINFO_CONFIG(PGTABLE_4);
#endif
}

unsigned long paddr_vmcoreinfo_note(void)
{
	return ia64_tpa((unsigned long)(char *)&vmcoreinfo_note);
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       #include <linux/module.h>
#include <linux/dma-mapping.h>
#include <asm/machvec.h>

#ifdef CONFIG_IA64_GENERIC

#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/page.h>

struct ia64_machine_vector ia64_mv;
EXPORT_SYMBOL(ia64_mv);

static struct ia64_machine_vector * __init
lookup_machvec (const char *name)
{
	extern struct ia64_machine_vector machvec_start[];
	extern struct ia64_machine_vector machvec_end[];
	struct ia64_machine_vector *mv;

	for (mv = machvec_start; mv < machvec_end; ++mv)
		if (strcmp (mv->name, name) == 0)
			return mv;

	return 0;
}

void __init
machvec_init (const char *name)
{
	struct ia64_machine_vector *mv;

	if (!name)
		name = acpi_get_sysname();
	mv = lookup_machvec(name);
	if (!mv)
		panic("generic kernel failed to find machine vector for"
		      " platform %s!", name);

	ia64_mv = *mv;
	printk(KERN_INFO "booting generic kernel on platform %s\n", name);
}

void __init
machvec_init_from_cmdline(const char *cmdline)
{
	char str[64];
	const char *start;
	char *end;

	if (! (start = strstr(cmdline, "machvec=")) )
		return machvec_init(NULL);

	strlcpy(str, start + strlen("machvec="), sizeof(str));
	if ( (end = strchr(str, ' ')) )
		*end = '\0';

	return machvec_init(str);
}

#endif /* CONFIG_IA64_GENERIC */

void
machvec_setup (char **arg)
{
}
EXPORT_SYMBOL(machvec_setup);

void
machvec_timer_interrupt (int irq, void *dev_id)
{
}
EXPORT_SYMBOL(machvec_timer_interrupt);

void
machvec_dma_sync_single(struct device *hwdev, dma_addr_t dma_handle, size_t size,
			enum dma_data_direction dir)
{
	mb();
}
EXPORT_SYMBOL(machvec_dma_sync_single);

void
machvec_dma_sync_sg(struct device *hwdev, struct scatterlist *sg, int n,
		    enum dma_data_direction dir)
{
	mb();
}
EXPORT_SYMBOL(machvec_dma_sync_sg);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        