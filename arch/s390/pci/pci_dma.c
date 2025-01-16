state *sos,
		unsigned long *nat)
{
	const pal_min_state_area_t *ms = sos->pal_min_state;
	const u64 *bank;

	/* If ipsr.ic then use pmsa_{iip,ipsr,ifs}, else use
	 * pmsa_{xip,xpsr,xfs}
	 */
	if (ia64_psr(regs)->ic) {
		regs->cr_iip = ms->pmsa_iip;
		regs->cr_ipsr = ms->pmsa_ipsr;
		regs->cr_ifs = ms->pmsa_ifs;
	} else {
		regs->cr_iip = ms->pmsa_xip;
		regs->cr_ipsr = ms->pmsa_xpsr;
		regs->cr_ifs = ms->pmsa_xfs;

		sos->iip = ms->pmsa_iip;
		sos->ipsr = ms->pmsa_ipsr;
		sos->ifs = ms->pmsa_ifs;
	}
	regs->pr = ms->pmsa_pr;
	regs->b0 = ms->pmsa_br0;
	regs->ar_rsc = ms->pmsa_rsc;
	copy_reg(&ms->pmsa_gr[1-1], ms->pmsa_nat_bits, &regs->r1, nat);
	copy_reg(&ms->pmsa_gr[2-1], ms->pmsa_nat_bits, &regs->r2, nat);
	copy_reg(&ms->pmsa_gr[3-1], ms->pmsa_nat_bits, &regs->r3, nat);
	copy_reg(&ms->pmsa_gr[8-1], ms->pmsa_nat_bits, &regs->r8, nat);
	copy_reg(&ms->pmsa_gr[9-1], ms->pmsa_nat_bits, &regs->r9, nat);
	copy_reg(&ms->pmsa_gr[10-1], ms->pmsa_nat_bits, &regs->r10, nat);
	copy_reg(&ms->pmsa_gr[11-1], ms->pmsa_nat_bits, &regs->r11, nat);
	copy_reg(&ms->pmsa_gr[12-1], ms->pmsa_nat_bits, &regs->r12, nat);
	copy_reg(&ms->pmsa_gr[13-1], ms->pmsa_nat_bits, &regs->r13, nat);
	copy_reg(&ms->pmsa_gr[14-1], ms->pmsa_nat_bits, &regs->r14, nat);
	copy_reg(&ms->pmsa_gr[15-1], ms->pmsa_nat_bits, &regs->r15, nat);
	if (ia64_psr(regs)->bn)
		bank = ms->pmsa_bank1_gr;
	else
		bank = ms->pmsa_bank0_gr;
	copy_reg(&bank[16-16], ms->pmsa_nat_bits, &regs->r16, nat);
	copy_reg(&bank[17-16], ms->pmsa_nat_bits, &regs->r17, nat);
	copy_reg(&bank[18-16], ms->pmsa_nat_bits, &regs->r18, nat);
	copy_reg(&bank[19-16], ms->pmsa_nat_bits, &regs->r19, nat);
	copy_reg(&bank[20-16], ms->pmsa_nat_bits, &regs->r20, nat);
	copy_reg(&bank[21-16], ms->pmsa_nat_bits, &regs->r21, nat);
	copy_reg(&bank[22-16], ms->pmsa_nat_bits, &regs->r22, nat);
	copy_reg(&bank[23-16], ms->pmsa_nat_bits, &regs->r23, nat);
	copy_reg(&bank[24-16], ms->pmsa_nat_bits, &regs->r24, nat);
	copy_reg(&bank[25-16], ms->pmsa_nat_bits, &regs->r25, nat);
	copy_reg(&bank[26-16], ms->pmsa_nat_bits, &regs->r26, nat);
	copy_reg(&bank[27-16], ms->pmsa_nat_bits, &regs->r27, nat);
	copy_reg(&bank[28-16], ms->pmsa_nat_bits, &regs->r28, nat);
	copy_reg(&bank[29-16], ms->pmsa_nat_bits, &regs->r29, nat);
	copy_reg(&bank[30-16], ms->pmsa_nat_bits, &regs->r30, nat);
	copy_reg(&bank[31-16], ms->pmsa_nat_bits, &regs->r31, nat);
}

/* On entry to this routine, we are running on the per cpu stack, see
 * mca_asm.h.  The original stack has not been touched by this event.  Some of
 * the original stack's registers will be in the RBS on this stack.  This stack
 * also contains a partial pt_regs and switch_stack, the rest of the data is in
 * PAL minstate.
 *
 * The first thing to do is modify the original stack to look like a blocked
 * task so we can run backtrace on the original task.  Also mark the per cpu
 * stack as current to ensure that we use the correct task state, it also means
 * that we can do backtrace on the MCA/INIT handler code itself.
 */

static struct task_struct *
ia64_mca_modify_original_stack(struct pt_regs *regs,
		const struct switch_stack *sw,
		struct ia64_sal_os_state *sos,
		const char *type)
{
	char *p;
	ia64_va va;
	extern char ia64_leave_kernel[];	/* Need asm address, not function descriptor */
	const pal_min_state_area_t *ms = sos->pal_min_state;
	struct task_struct *previous_current;
	struct pt_regs *old_regs;
	struct switch_stack *old_sw;
	unsigned size = sizeof(struct pt_regs) +
			sizeof(struct switch_stack) + 16;
	unsigned long *old_bspstore, *old_bsp;
	unsigned long *new_bspstore, *new_bsp;
	unsigned long old_unat, old_rnat, new_rnat, nat;
	u64 slots, loadrs = regs->loadrs;
	u64 r12 = ms->pmsa_gr[12-1], r13 = ms->pmsa_gr[13-1];
	u64 ar_bspstore = regs->ar_bspstore;
	u64 ar_bsp = regs->ar_bspstore + (loadrs >> 16);
	const char *msg;
	int cpu = smp_processor_id();

	previous_current = curr_task(cpu);
	set_curr_task(cpu, current);
	if ((p = strchr(current->comm, ' ')))
		*p = '\0';

	/* Best effort attempt to cope with MCA/INIT delivered while in
	 * physical mode.
	 */
	regs->cr_ipsr = ms->pmsa_ipsr;
	if (ia64_psr(regs)->dt == 0) {
		va.l = r12;
		if (va.f.reg == 0) {
			va.f.reg = 7;
			r12 = va.l;
		}
		va.l = r13;
		if (va.f.reg == 0) {
			va.f.reg = 7;
			r13 = va.l;
		}
	}
	if (ia64_psr(regs)->rt == 0) {
		va.l = ar_bspstore;
		if (va.f.reg == 0) {
			va.f.reg = 7;
			ar_bspstore = va.l;
		}
		va.l = ar_bsp;
		if (va.f.reg == 0) {
			va.f.reg = 7;
			ar_bsp = va.l;
		}
	}

	/* mca_asm.S ia64_old_stack() cannot assume that the dirty registers
	 * have been copied to the old stack, the old stack may fail the
	 * validation tests below.  So ia64_old_stack() must restore the dirty
	 * registers from the new stack.  The old and new bspstore probably
	 * have different alignments, so loadrs calculated on the old bsp
	 * cannot be used to restore from the new bsp.  Calculate a suitable
	 * loadrs for the new stack and save it in the new pt_regs, where
	 * ia64_old_stack() can get it.
	 */
	old_bspstore = (unsigned long *)ar_bspstore;
	old_bsp = (unsigned long *)ar_bsp;
	slots = ia64_rse_num_regs(old_bspstore, old_bsp);
	new_bspstore = (unsigned long *)((u64)current + IA64_RBS_OFFSET);
	new_bsp = ia64_rse_skip_regs(new_bspstore, slots);
	regs->loadrs = (new_bsp - new_bspstore) * 8 << 16;

	/* Verify the previous stack state before we change it */
	if (user_mode(regs)) {
		msg = "occurred in user space";
		/* previous_current is guaranteed to be valid when the task was
		 * in user space, so ...
		 */
		ia64_mca_modify_comm(previous_current);
		goto no_mod;
	}

	if (r13 != sos->prev_IA64_KR_CURRENT) {
		msg = "inconsistent previous current and r13";
		goto no_mod;
	}

	if (!mca_recover_range(ms->pmsa_iip)) {
		if ((r12 - r13) >= KERNEL_STACK_SIZE) {
			msg = "inconsistent r12 and r13";
			goto no_mod;
		}
		if ((ar_bspstore - r13) >= KERNEL_STACK_SIZE) {
			msg = "inconsistent ar.bspstore and r13";
			goto no_mod;
		}
		va.p = old_bspstore;
		if (va.f.reg < 5) {
			msg = "old_bspstore is in the wrong region";
			goto no_mod;
		}
		if ((ar_bsp - r13) >= KERNEL_STACK_SIZE) {
			msg = "inconsistent ar.bsp and r13";
			goto no_mod;
		}
		size += (ia64_rse_skip_regs(old_bspstore, slots) - old_bspstore) * 8;
		if (ar_bspstore + size > r12) {
			msg = "no room for blocked state";
			goto no_mod;
		}
	}

	ia64_mca_modify_comm(previous_current);

	/* Make the original task look blocked.  First stack a struct pt_regs,
	 * describing the state at the time of interrupt.  mca_asm.S built a
	 * partial pt_regs, copy it and fill in the blanks using minstate.
	 */
	p = (char *)r12 - sizeof(*regs);
	old_regs = (struct pt_regs *)p;
	memcpy(old_regs, regs, sizeof(*regs));
	old_regs->loadrs = loadrs;
	old_unat = old_regs->ar_unat;
	finish_pt_regs(old_regs, sos, &old_unat);

	/* Next stack a struct switch_stack.  mca_asm.S built a partial
	 * switch_stack, copy it and fill in the blanks using pt_regs and
	 * minstate.
	 *
	 * In the synthesized switch_stack, b0 points to ia64_leave_kernel,
	 * ar.pfs is set to 0.
	 *
	 * unwind.c::unw_unwind() does special processing for interrupt frames.
	 * It checks if the PRED_NON_SYSCALL predicate is set, if the predicate
	 * is clear then unw_unwind() does _not_ adjust bsp over pt_regs.  Not
	 * that this is documented, of course.  Set PRED_NON_SYSCALL in the
	 * switch_stack on the original stack so it will unwind correctly when
	 * unwind.c reads pt_regs.
	 *
	 * thread.ksp is updated to point to the synthesized switch_stack.
	 */
	p -= sizeof(struct switch_stack);
	old_sw = (struct switch_stack *)p;
	memcpy(old_sw, sw, sizeof(*sw));
	old_sw->caller_unat = old_unat;
	old_sw->ar_fpsr = old_regs->ar_fpsr;
	copy_reg(&ms->pmsa_gr[4-1], ms->pmsa_nat_bits, &old_sw->r4, &old_unat);
	copy_reg(&ms->pmsa_gr[5-1], ms->pmsa_nat_bits, &old_sw->r5, &old_unat);
	copy_reg(&ms->pmsa_gr[6-1], ms->pmsa_nat_bits, &old_sw->r6, &old_unat);
	copy_reg(&ms->pmsa_gr[7-1], ms->pmsa_nat_bits, &old_sw->r7, &old_unat);
	old_sw->b0 = (u64)ia64_leave_kernel;
	old_sw->b1 = ms->pmsa_br1;
	old_sw->ar_pfs = 0;
	old_sw->ar_unat = old_unat;
	old_sw->pr = old_regs->pr | (1UL << PRED_NON_SYSCALL);
	previous_current->thread.ksp = (u64)p - 16;

	/* Finally copy the original stack's registers back to its RBS.
	 * Registers from ar.bspstore through ar.bsp at the time of the event
	 * are in the current RBS, copy them back to the original stack.  The
	 * copy must be done register by register because the original bspstore
	 * and the current one have different alignments, so the saved RNAT
	 * data occurs at different places.
	 *
	 * mca_asm does cover, so the old_bsp already includes all registers at
	 * the time of MCA/INIT.  It also does flushrs, so all registers before
	 * this function have been written to backing store on the MCA/INIT
	 * stack.
	 */
	new_rnat = ia64_get_rnat(ia64_rse_rnat_addr(new_bspstore));
	old_rnat = regs->ar_rnat;
	while (slots--) {
		if (ia64_rse_is_rnat_slot(new_bspstore)) {
			new_rnat = ia64_get_rnat(new_bspstore++);
		}
		if (ia64_rse_is_rnat_slot(old_bspstore)) {
			*old_bspstore++ = old_rnat;
			old_rnat = 0;
		}
		nat = (new_rnat >> ia64_rse_slot_num(new_bspstore)) & 1UL;
		old_rnat &= ~(1UL << ia64_rse_slot_num(old_bspstore));
		old_rnat |= (nat << ia64_rse_slot_num(old_bspstore));
		*old_bspstore++ = *new_bspstore++;
	}
	old_sw->ar_bspstore = (unsigned long)old_bspstore;
	old_sw->ar_rnat = old_rnat;

	sos->prev_task = previous_current;
	return previous_current;

no_mod:
	mprintk(KERN_INFO "cpu %d, %s %s, original stack not modified\n",
			smp_processor_id(), type, msg);
	old_unat = regs->ar_unat;
	finish_pt_regs(regs, sos, &old_unat);
	return previous_current;
}

/* The monarch/slave interaction is based on monarch_cpu and requires that all
 * slaves have entered rendezvous before the monarch leaves.  If any cpu has
 * not entered rendezvous yet then wait a bit.  The assumption is that any
 * slave that has not rendezvoused after a reasonable time is never going to do
 * so.  In this context, slave includes cpus that respond to the MCA rendezvous
 * interrupt, as well as cpus that receive the INIT slave event.
 */

static void
ia64_wait_for_slaves(int monarch, const char *type)
{
	int c, i , wait;

	/*
	 * wait 5 seconds total for slaves (arbitrary)
	 */
	for (i = 0; i < 5000; i++) {
		wait = 0;
		for_each_online_cpu(c) {
			if (c == monarch)
				continue;
			if (ia64_mc_info.imi_rendez_checkin[c]
					== IA64_MCA_RENDEZ_CHECKIN_NOTDONE) {
				udelay(1000);		/* short wait */
				wait = 1;
				break;
			}
		}
		if (!wait)
			goto all_in;
	}

	/*
	 * Maybe slave(s) dead. Print buffered messages immediately.
	 */
	ia64_mlogbuf_finish(0);
	mprintk(KERN_INFO "OS %s slave did not rendezvous on cpu", type);
	for_each_online_cpu(c) {
		if (c == monarch)
			continue;
		if (ia64_mc_info.imi_rendez_checkin[c] == IA64_MCA_RENDEZ_CHECKIN_NOTDONE)
			mprintk(" %d", c);
	}
	mprintk("\n");
	return;

all_in:
	mprintk(KERN_INFO "All OS %s slaves have reached rendezvous\n", type);
	return;
}

/*  mca_insert_tr
 *
 *  Switch rid when TR reload and needed!
 *  iord: 1: itr, 2: itr;
 *
*/
static void mca_insert_tr(u64 iord)
{

	int i;
	u64 old_rr;
	struct ia64_tr_entry *p;
	unsigned long psr;
	int cpu = smp_processor_id();

	if (!ia64_idtrs[cpu])
		return;

	psr = ia64_clear_ic();
	for (i = IA64_TR_ALLOC_BASE; i < IA64_TR_ALLOC_MAX; i++) {
		p = ia64_idtrs[cpu] + (iord - 1) * IA64_TR_ALLOC_MAX;
		if (p->pte & 0x1) {
			old_rr = ia64_get_rr(p->ifa);
			if (old_rr != p->rr) {
				ia64_set_rr(p->ifa, p->rr);
				ia64_srlz_d();
			}
			ia64_ptr(iord, p->ifa, p->itir >> 2);
			ia64_srlz_i();
			if (iord & 0x1) {
				ia64_itr(0x1, i, p->ifa, p->pte, p->itir >> 2);
				ia64_srlz_i();
			}
			if (iord & 0x2) {
				ia64_itr(0x2, i, p->ifa, p->pte, p->itir >> 2);
				ia64_srlz_i();
			}
			if (old_rr != p->rr) {
				ia64_set_rr(p->ifa, old_rr);
				ia64_srlz_d();
			}
		}
	}
	ia64_set_psr(psr);
}

/*
 * ia64_mca_handler
 *
 *	This is uncorrectable machine check handler called from OS_MCA
 *	dispatch code which is in turn called from SAL_CHECK().
 *	This is the place where the core of OS MCA handling is done.
 *	Right now the logs are extracted and displayed in a well-defined
 *	format. This handler code is supposed to be run only on the
 *	monarch processor. Once the monarch is done with MCA handling
 *	further MCA logging is enabled by clearing logs.
 *	Monarch also has the duty of sending wakeup-IPIs to pull the
 *	slave processors out of rendezvous spinloop.
 *
 *	If multiple processors call into OS_MCA, the first will become
 *	the monarch.  Subsequent cpus will be recorded in the mca_cpu
 *	bitmask.  After the first monarch has processed its MCA, it
 *	will wake up the next cpu in the mca_cpu bitmask and then go
 *	into the rendezvous loop.  When all processors have serviced
 *	their MCA, the last monarch frees up the rest of the processors.
 */
void
ia64_mca_handler(struct pt_regs *regs, struct switch_stack *sw,
		 struct ia64_sal_os_state *sos)
{
	int recover, cpu = smp_processor_id();
	struct task_struct *previous_current;
	struct ia64_mca_notify_die nd =
		{ .sos = sos, .monarch_cpu = &monarch_cpu, .data = &recover };
	static atomic_t mca_count;
	static cpumask_t mca_cpu;

	if (atomic_add_return(1, &mca_count) == 1) {
		monarch_cpu = cpu;
		sos->monarch = 1;
	} else {
		cpumask_set_cpu(cpu, &mca_cpu);
		sos->monarch = 0;
	}
	mprintk(KERN_INFO "Entered OS MCA handler. PSP=%lx cpu=%d "
		"monarch=%ld\n", sos->proc_state_param, cpu, sos->monarch);

	previous_current = ia64_mca_modify_original_stack(regs, sw, sos, "MCA");

	NOTIFY_MCA(DIE_MCA_MONARCH_ENTER, regs, (long)&nd, 1);

	ia64_mc_info.imi_rendez_checkin[cpu] = IA64_MCA_RENDEZ_CHECKIN_CONCURRENT_MCA;
	if (sos->monarch) {
		ia64_wait_for_slaves(cpu, "MCA");

		/* Wakeup all the processors which are spinning in the
		 * rendezvous loop.  They will leave SAL, then spin in the OS
		 * with interrupts disabled until this monarch cpu leaves the
		 * MCA handler.  That gets control back to the OS so we can
		 * backtrace the other cpus, backtrace when spinning in SAL
		 * 