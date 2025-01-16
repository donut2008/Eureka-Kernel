errupt
		 * during a chip reset.
		 */
		return IRQ_HANDLED;

	this_cpu_inc(*dd->int_counter);

	/* Clear the interrupt bit we expect to be set. */
	qib_write_kreg(dd, kr_intclear, ((1ULL << QIB_I_RCVAVAIL_LSB) |
		       (1ULL << QIB_I_RCVURG_LSB)) << rcd->ctxt);

	qib_kreceive(rcd, NULL, &npkts);

	return IRQ_HANDLED;
}

/*
 * Dedicated Send buffer available interrupt handler.
 */
static irqreturn_t qib_7322bufavail(int irq, void *data)
{
	struct qib_devdata *dd = data;

	if ((dd->flags & (QIB_PRESENT | QIB_BADINTR)) != QIB_PRESENT)
		/*
		 * This return value is not great, but we do not want the
		 * interrupt core code to remove our interrupt handler
		 * because we don't appear to be handling an interrupt
		 * during a chip reset.
		 */
		return IRQ_HANDLED;

	this_cpu_inc(*dd->int_counter);

	/* Clear the interrupt bit we expect to be set. */
	qib_write_kreg(dd, kr_intclear, QIB_I_SPIOBUFAVAIL);

	/* qib_ib_piobufavail() will clear the want PIO interrupt if needed */
	if (dd->flags & QIB_INITTED)
		qib_ib_piobufavail(dd);
	else
		qib_wantpiobuf_7322_intr(dd, 0);

	return IRQ_HANDLED;
}

/*
 * Dedicated Send DMA interrupt handler.
 */
static irqreturn_t sdma_intr(int irq, void *data)
{
	struct qib_pportdata *ppd = data;
	struct qib_devdata *dd = ppd->dd;

	if ((dd->flags & (QIB_PRESENT | QIB_BADINTR)) != QIB_PRESENT)
		/*
		 * This return value is not great, but we do not want the
		 * interrupt core code to remove our interrupt handler
		 * because we don't appear to be handling an interrupt
		 * during a chip reset.
		 */
		return IRQ_HANDLED;

	this_cpu_inc(*dd->int_counter);

	/* Clear the interrupt bit we expect to be set. */
	qib_write_kreg(dd, kr_intclear, ppd->hw_pidx ?
		       INT_MASK_P(SDma, 1) : INT_MASK_P(SDma, 0));
	qib_sdma_intr(ppd);

	return IRQ_HANDLED;
}

/*
 * Dedicated Send DMA idle interrupt handler.
 */
static irqreturn_t sdma_idle_intr(int irq, void *data)
{
	struct qib_pportdata *ppd = data;
	struct qib_devdata *dd = ppd->dd;

	if ((dd->flags & (QIB_PRESENT | QIB_BADINTR)) != QIB_PRESENT)
		/*
		 * This return value is not great, but we do not want the
		 * interrupt core code to remove our interrupt handler
		 * because we don't appear to be handling an interrupt
		 * during a chip reset.
		 */
		return IRQ_HANDLED;

	this_cpu_inc(*dd->int_counter);

	/* Clear the interrupt bit we expect to be set. */
	qib_write_kreg(dd, kr_intclear, ppd->hw_pidx ?
		       INT_MASK_P(SDmaIdle, 1) : INT_MASK_P(SDmaIdle, 0));
	qib_sdma_intr(ppd);

	return IRQ_HANDLED;
}

/*
 * Dedicated Send DMA progress interrupt handler.
 */
static irqreturn_t sdma_progress_intr(int irq, void *data)
{
	struct qib_pportdata *ppd = data;
	struct qib_devdata *dd = ppd->dd;

	if ((dd->flags & (QIB_PRESENT | QIB_BADINTR)) != QIB_PRESENT)
		/*
		 * This return value is not great, but we do not want the
		 * interrupt core code to remove our interrupt handler
		 * because we don't appear to be handling an interrupt
		 * during a chip reset.
		 */
		return IRQ_HANDLED;

	this_cpu_inc(*dd->int_counter);

	/* Clear the interrupt bit we expect to be set. */
	qib_write_kreg(dd, kr_intclear, ppd->hw_pidx ?
		       INT_MASK_P(SDmaProgress, 1) :
		       INT_MASK_P(SDmaProgress, 0));
	qib_sdma_intr(ppd);

	return IRQ_HANDLED;
}

/*
 * Dedicated Send DMA cleanup interrupt handler.
 */
static irqreturn_t sdma_cleanup_intr(int irq, void *data)
{
	struct qib_pportdata *ppd = data;
	struct qib_devdata *dd = ppd->dd;

	if ((dd->flags & (QIB_PRESENT | QIB_BADINTR)) != QIB_PRESENT)
		/*
		 * This return value is not great, but we do not want the
		 * interrupt core code to remove our interrupt handler
		 * because we don't appear to be handling an interrupt
		 * during a chip reset.
		 */
		return IRQ_HANDLED;

	this_cpu_inc(*dd->int_counter);

	/* Clear the interrupt bit we expect to be set. */
	qib_write_kreg(dd, kr_intclear, ppd->hw_pidx ?
		       INT_MASK_PM(SDmaCleanupDone, 1) :
		       INT_MASK_PM(SDmaCleanupDone, 0));
	qib_sdma_process_event(ppd, qib_sdma_event_e20_hw_started);

	return IRQ_HANDLED;
}

#ifdef CONFIG_INFINIBAND_QIB_DCA

static void reset_dca_notifier(struct qib_devdata *dd, struct qib_msix_entry *m)
{
	if (!m->dca)
		return;
	qib_devinfo(dd->pcidev,
		"Disabling notifier on HCA %d irq %d\n",
		dd->unit,
		m->msix.vector);
	irq_set_affinity_notifier(
		m->msix.vector,
		NULL);
	m->notifier = NULL;
}

static void setup_dca_notifier(struct qib_devdata *dd, struct qib_msix_entry *m)
{
	struct qib_irq_notify *n;

	if (!m->dca)
		return;
	n = kzalloc(sizeof(*n), GFP_KERNEL);
	if (n) {
		int ret;

		m->notifier = n;
		n->notify.irq = m->msix.vector;
		n->notify.notify = qib_irq_notifier_notify;
		n->notify.release = qib_irq_notifier_release;
		n->arg = m->arg;
		n->rcv = m->rcv;
		qib_devinfo(dd->pcidev,
			"set notifier irq %d rcv %d notify %p\n",
			n->notify.irq, n->rcv, &n->notify);
		ret = irq_set_affinity_notifier(
				n->notify.irq,
				&n->notify);
		if (ret) {
			m->notifier = NULL;
			kfree(n);
		}
	}
}

#endif

/*
 * Set up our chip-specific interrupt handler.
 * The interrupt type has already been setup, so
 * we just need to do the registration and error checking.
 * If we are using MSIx interrupts, we may fall back to
 * INTx later, if the interrupt handler doesn't get called
 * within 1/2 second (see verify_interrupt()).
 */
static void qib_setup_7322_interrupt(struct qib_devdata *dd, int clearpend)
{
	int ret, i, msixnum;
	u64 redirect[6];
	u64 mask;
	const struct cpumask *local_mask;
	int firstcpu, secondcpu = 0, currrcvcpu = 0;

	if (!dd->num_pports)
		return;

	if (clearpend) {
		/*
		 * if not switching interrupt types, be sure interrupts are
		 * disabled, and then clear anything pending at this point,
		 * because we are starting clean.
		 */
		qib_7322_set_intr_state(dd, 0);

		/* clear the reset error, init error/hwerror mask */
		qib_7322_init_hwerrors(dd);

		/* clear any interrupt bits that might be set */
		qib_write_kreg(dd, kr_intclear, ~0ULL);

		/* make sure no pending MSIx intr, and clear diag reg */
		qib_write_kreg(dd, kr_intgranted, ~0ULL);
		qib_write_kreg(dd, kr_vecclr_wo_int, ~0ULL);
	}

	if (!dd->cspec->num_msix_entries) {
		/* Try to get INTx interrupt */
try_intx:
		if (!dd->pcidev->irq) {
			qib_dev_err(dd,
				"irq is 0, BIOS error?  Interrupts won't work\n");
			goto bail;
		}
		ret = request_irq(dd->pcidev->irq, qib_7322intr,
				  IRQF_SHARED, QIB_DRV_NAME, dd);
		if (ret) {
			qib_dev_err(dd,
				"Couldn't setup INTx interrupt (irq=%d): %d\n",
				dd->pcidev->irq, ret);
			goto bail;
		}
		dd->cspec->irq = dd->pcidev->irq;
		dd->cspec->main_int_mask = ~0ULL;
		goto bail;
	}

	/* Try to get MSIx interrupts */
	memset(redirect, 0, sizeof(redirect));
	mask = ~0ULL;
	msixnum = 0;
	local_mask = cpumask_of_pcibus(dd->pcidev->bus);
	firstcpu = cpumask_first(local_mask);
	if (firstcpu >= nr_cpu_ids ||
			cpumask_weight(local_mask) == num_online_cpus()) {
		local_mask = topology_core_cpumask(0);
		firstcpu = cpumask_first(local_mask);
	}
	if (firstcpu < nr_cpu_ids) {
		secondcpu = cpumask_next(firstcpu, local_mask);
		if (secondcpu >= nr_cpu_ids)
			secondcpu = firstcpu;
		currrcvcpu = secondcpu;
	}
	for (i = 0; msixnum < dd->cspec->num_msix_entries; i++) {
		irq_handler_t handler;
		void *arg;
		u64 val;
		int lsb, reg, sh;
#ifdef CONFIG_INFINIBAND_QIB_DCA
		int dca = 0;
#endif

		dd->cspec->msix_entries[msixnum].
			name[sizeof(dd->cspec->msix_entries[msixnum].name) - 1]
			= '\0';
		if (i < ARRAY_SIZE(irq_table)) {
			if (irq_table[i].port) {
				/* skip if for a non-configured port */
				if (irq_table[i].port > dd->num_pports)
					continue;
				arg = dd->pport + irq_table[i].port - 1;
			} else
				arg = dd;
#ifdef CONFIG_INFINIBAND_QIB_DCA
			dca = irq_table[i].dca;
#endif
			lsb = irq_table[i].lsb;
			handler = irq_table[i].handler;
			snprintf(dd->cspec->msix_entries[msixnum].name,
				sizeof(dd->cspec->msix_entries[msixnum].name)
				 - 1,
				QIB_DRV_NAME "%d%s", dd->unit,
				irq_table[i].name);
		} else {
			unsigned ctxt;

			ctxt = i - ARRAY_SIZE(irq_table);
			/* per krcvq context receive interrupt */
			arg = dd->rcd[ctxt];
			if (!arg)
				continue;
			if (qib_krcvq01_no_msi && ctxt < 2)
				continue;
#ifdef CONFIG_INFINIBAND_QIB_DCA
			dca = 1;
#endif
			lsb = QIB_I_RCVAVAIL_LSB + ct