s  :   cpuid
 *  Outputs :   None
 */
static void
ia64_mca_wakeup(int cpu)
{
	platform_send_ipi(cpu, IA64_MCA_WAKEUP_VECTOR, IA64_IPI_DM_INT, 0);
}

/*
 * ia64_mca_wakeup_all
 *
 *	Wakeup all the slave cpus which have rendez'ed previously.
 *
 *  Inputs  :   None
 *  Outputs :   None
 */
static void
ia64_mca_wakeup_all(void)
{
	int cpu;

	/* Clear the Rendez checkin flag for all cpus */
	for_each_online_cpu(cpu) {
		if (ia64_mc_info.imi_rendez_checkin[cpu] == IA64_MCA_RENDEZ_CHECKIN_DONE)
			ia64_mca_wakeup(cpu);
	}

}

/*
 * ia64_mca_rendez_interrupt_handler
 *
 *	This is handler used to put slave processors into spinloop
 *	while the monarch processor does the mca handling and later
 *	wake each slave up once the monarch is done.  The state
 *	IA64_MCA_RENDEZ_CHECKIN_DONE indicates the cpu is rendez'ed
 *	in SAL.  The state IA64_MCA_RENDEZ_CHECKIN_NOTDONE indicates
 *	the cpu has come out of OS rendezvous.
 *
 *  Inputs  :   None
 *  Outputs :   None
 */
static irqreturn_t
ia64_mca_rendez_int_handler(int rendez_irq, void *arg)
{
	unsigned long flags;
	int cpu = smp_processor_id();
	struct ia64_mca_notify_die nd =
		{ .sos = NULL, .monarch_cpu = &monarch_cpu };

	/* Mask all interrupts */
	local_irq_save(flags);

	NOTIFY_MCA(DIE_MCA_RENDZVOUS_ENTER, get_irq_regs(), (long)&nd, 1);

	ia64_mc_info.imi_rendez_checkin[cpu] = IA64_MCA_RENDEZ_CHECKIN_DONE;
	/* Register with the SAL monarch that the slave has
	 * reached SAL
	 */
	ia64_sal_mc_rendez();

	NOTIFY_MCA(DIE_MCA_RENDZVOUS_PROCESS, get_irq_regs(), (long)&nd, 1);

	/* Wait for the monarch cpu to exit. */
	while (monarch_cpu != -1)
	       cpu_relax();	/* spin until monarch leaves */

	NOTIFY_MCA(DIE_MCA_RENDZVOUS_LEAVE, get_irq_regs(), (long)&nd, 1);

	ia64_mc_info.imi_rendez_checkin[cpu] = IA64_MCA_RENDEZ_CHECKIN_NOTDONE;
	/* Enable all interrupts */
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

/*
 * ia64_mca_wakeup_int_handler
 *
 *	The interrupt handler for processing the inter-cpu interrupt to the
 *	slave cpu which was spinning in the rendez loop.
 *	Since this spinning is done by turning off the interrupts and
 *	polling on the wakeup-interrupt bit in the IRR, there is
 *	nothing useful to be done in the handler.
 *
 *  Inputs  :   wakeup_irq  (Wakeup-interrupt bit)
 *	arg		(Interrupt handler specific argument)
 *  Outputs :   None
 *
 */
static irqreturn_t
ia64_mca_wakeup_int_handler(int wakeup_irq, void *arg)
{
	return IRQ_HANDLED;
}

/* Function pointer for extra MCA recovery */
int (*ia64_mca_ucmc_extension)
	(void*,struct ia64_sal_os_state*)
	= NULL;

int
ia64_reg_MCA_extension(int (*fn)(void *, struct ia64_sal_os_state *))
{
	if (ia64_mca_ucmc_extension)
		return 1;

	ia64_mca_ucmc_extension = fn;
	return 0;
}

void
ia64_unreg_MCA_extension(void)
{
	if (ia64_mca_ucmc_extension)
		ia64_mca_ucmc_extension = NULL;
}

EXPORT_SYMBOL(ia64_reg_MCA_extension);
EXPORT_SYMBOL(ia64_unreg_MCA_extension);


static inline void
copy_reg(const u64 *fr, u64 fnat, unsigned long *tr, unsigned long *tnat)
{
	u64 fslot, tslot, nat;
	*tr = *fr;
	fslot = ((unsigned long)fr >> 3) & 63;
	tslot = ((unsigned long)tr >> 3) & 63;
	*tnat &= ~(1UL << tslot);
	nat = (fnat >> fslot) & 1;
	*tnat |= (nat << tslot);
}

/* Change the comm field on the MCA/INT task to include the pid that
 * was interrupted, it makes for easier debugging.  If that pid was 0
 * (swapper or nested MCA/INIT) then use the start of the previous comm
 * field suffixed with its cpu.
 */

static void
ia64_m