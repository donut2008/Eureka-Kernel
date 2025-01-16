apic_lock;
	}

	desc = irq_to_desc(irq);
	raw_spin_lock(&desc->lock);
	dest = get_target_cpu(gsi, irq);
	dmode = choose_dmode();
	err = register_intr(gsi, irq, dmode, polarity, trigger);
	if (err < 0) {
		raw_spin_unlock(&desc->lock);
		irq = err;
		goto unlock_iosapic_lock;
	}

	/*
	 * If the vector is shared and already unmasked for other
	 * interrupt sources, don't mask it.
	 */
	low32 = iosapic_intr_info[irq].low32;
	if (irq_is_shared(irq) && !(low32 & IOSAPIC_MASK))
		mask = 0;
	set_rte(gsi, irq, dest, mask);

	printk(KERN_INFO "GSI %u (%s, %s) -> CPU %d (0x%04x) vector %d\n",
	       gsi, (trigger == IOSAPIC_EDGE ? "edge" : "level"),
	       (polarity == IOSAPIC_POL_HIGH ? "high" : "low"),
	       cpu_logical_id(dest), dest, irq_to_vector(irq));

	raw_spin_unlock(&desc->lock);
 unlock_iosapic_lock:
	spin_unlock_irqrestore(&iosapic_lock, flags);
	return irq;
}

void
iosapic_unregister_intr (unsigned int gsi)
{
	unsigned long flags;
	int irq, index;
	u32 low32;
	unsigned long trigger, polarity;
	unsigned int dest;
	struct iosapic_rte_info *rte;

	/*
	 * If the irq associated with the gsi is not found,
	 * iosapic_unregister_intr() is unbalanced. We need to check
	 * this again after getting locks.
	 */
	irq = gsi_to_irq(gsi);
	if (irq < 0) {
		printk(KERN_ERR "iosapic_unregister_intr(%u) unbalanced\n",
		       gsi);
		WARN_ON(1);
		return;
	}

	spin_lock_irqsave(&iosapic_lock, flags);
	if ((rte = find_rte(irq, gsi)) == NULL) {
		printk(KERN_ERR "iosapic_unregister_intr(%u) unbalanced\n",
		       gsi);
		WARN_ON(1);
		goto out;
	}

	if (--rte->refcnt > 0)
		goto out;

	rte->refcnt = NO_REF_RTE;

	/* Mask the interrupt */
	low32 = iosapic_intr_info[irq].low32 | IOSAPIC_MASK;
	iosapic_write(rte->iosapic, IOSAPIC_RTE_LOW(rte->rte_index), low32);

	iosapic_intr_info[irq].count--;
	index = find_iosapic(gsi);
	iosapic_lists[index].rtes_inuse--;
	WARN_ON(iosapic_lists[index].rtes_inuse < 0);

	trigger  = iosapic_intr_info[irq].trigger;
	polarity = iosapic_intr_info[irq].polarity;
	dest     = iosapic_intr_info[irq].dest;
	printk(KERN_INFO
	       "GSI %u (%s, %s) -> CPU %d (0x%04x) vector %d unregistered\n",
	       gsi, (trigger == IOSAPIC_EDGE ? "edge" : "level"),
	       (polarity == IOSAPIC_POL_HIGH ? "high" : "low"),
	       cpu_logical_id(dest), dest, irq_to_vector(irq));

	if (iosapic_intr_info[irq].count == 0) {
#ifdef CONFIG_SMP
		/* Clear affinity */
		cpumask_setall(irq_get_affinity_mask(irq));
#endif
		/* Clear the interrupt information */
		iosapic_intr_info[irq].dest = 0;
		iosapic_intr_info[irq].dmode = 0;
		iosapic_intr_info[irq].polarity = 0;
		iosapic_intr_info[irq].trigger = 0;
		iosapic_intr_info[irq].low32 |= IOSAPIC_MASK;

		/* Destroy and reserve IRQ */
		destroy_and_reserve_irq(irq);
	}
 out:
	spin_unlock_irqrestore(&iosapic_lock, flags);
}

/*
 * ACPI calls this when it finds an entry for a platform interrupt.
 */
int __init
iosapic_register_platform_intr (u32 int_type, unsigned int gsi,
				int iosapic_vector, u16 eid, u16 id,
				unsigned long polarity, unsigned long trigger)
{
	static const char * const name[] = {"unknown", "PMI", "INIT", "CPEI"};
	unsigned char delivery;
	int irq, vector, mask = 0;
	unsigned int dest = ((id << 8) | eid) & 0xffff;

	switch (int_type) {
	      case ACPI_INTERRUPT_PMI:
		irq = vector = iosapic_vector;
		bind_irq_vector(irq, vector, CPU_MASK_ALL);
		/*
		 * since PMI vector is alloc'd by FW(ACPI) not by kernel,
		 * we need to make sure the vector is available
		 */
		iosapic_reassign_vector(irq);
		delivery = IOSAPIC_PMI;
		break;
	      case ACPI_INTERRUPT_INIT:
		irq = create_irq();
		if (irq < 0)
			panic("%s: out of interrupt vectors!\n", __func__);
		vector = irq_to_vector(irq);
		delivery = IOSAPIC_INIT;
		break;
	      case ACPI_INTERRUPT_CPEI:
		irq = vector = IA64_CPE_VECTOR;
		BUG_ON(bind_irq_vector(irq, vector, CPU_MASK_ALL));
		delivery = IOSAPIC_FIXED;
		mask = 1;
		break;
	      default:
		printk(KERN_ERR "%s: invalid int type 0x%x\n", __func__,
		       int_type);
		return -1;
	}

	register_intr(gsi, irq, delivery, polarity, trigger);

	printk(KERN_INFO
	       "PLATFORM int %s (0x%x): GSI %u (%s, %s) -> CPU %d (0x%04x)"
	       " vector %d\n",
	       int_type < ARRAY_SIZE(name) ? name[int_type] : "unknown",
	       int_type, gsi, (trigger == IOSAPIC_EDGE ? "edge" : "level"),
	       (polarity == IOSAPIC_POL_HIGH ? "high" : "low"),
	       cpu_logical_id(dest), dest, vector);

	set_rte(gsi, irq, dest, mask);
	return vector;
}

/*
 * ACPI calls this when it finds an entry for a legacy ISA IRQ override.
 */
void iosapic_override_isa_irq(unsigned int isa_irq, unsigned int gsi,
			      unsigned long polarity, unsigned long trigger)
{
	int vector, irq;
	unsigned int dest = cpu_physical_id(smp_processor_id());
	unsigned char dmode;

	irq = vector = isa_irq_to_vector(isa_irq);
	BUG_ON(bind_irq_vector(irq, vector, CPU_MASK_ALL));
	dmode = choose_dmode();
	register_intr(gsi, irq, dmode, polarity, trigger);

	DBG("ISA: IRQ %u -> GSI %u (%s,%s) -> CPU %d (0x%04x) vector %d\n",
	    isa_irq, gsi, trigger == IOSAPIC_EDGE ? "edge" : "level",
	    polarity == IOSAPIC_POL_HIGH ? "high" : "low",
	    cpu_logical_id(dest), dest, vector);

	set_rte(gsi, irq, dest, 1);
}

void __init
ia64_native_iosapic_pcat_compat_init(void)
{
	if (pcat_compat) {
		/*
		 * Disable the compatibility mode interrupts (8259 style),
		 * needs IN/OUT support enabled.
		 */
		printk(KERN_INFO
		       "%s: Disabling PC-AT compatible 8259 interrupts\n",
		       __func__);
		outb(0xff, 0xA1);
		outb(0xff, 0x21);
	}
}

void __init
iosapic_system_init (int system_pcat_compat)
{
	int irq;

	for (irq = 0; irq < NR_IRQS; ++irq) {
		iosapic_intr_info[irq].low32 = IOSAPIC_MASK;
		/* mark as unused */
		INIT_LIST_HEAD(&iosapic_intr_info[irq].rtes);

		iosapic_intr_info[irq].count = 0;
	}

	pcat_compat = system_pcat_compat;
	if (pcat_compat)
		iosapic_pcat_compat_init();
}

static inline int
iosapic_alloc (void)
{
	int index;

	for (index = 0; index < NR_IOSAPICS; index++)
		if (!iosapic_lists[index].addr)
			return index;

	printk(KERN_WARNING "%s: failed to allocate iosapic\n", __func__);
	return -1;
}

static inline void
iosapic_free (int index)
{
	memset(&iosapic_lists[index], 0, sizeof(iosapic_lists[0]));
}

static inline int
iosapic_check_gsi_range (unsigned int gsi_base, unsigned int ver)
{
	int index;
	unsigned int gsi_end, base, end;

	/* check gsi range */
	gsi_end = gsi_base + ((ver >> 16) & 0xff);
	for (index = 0; index < NR_IOSAPICS; index++) {
		if (!iosapic_lists[index].addr)
			continue;

		base = iosapic_lists[index].gsi_base;
		end  = base + iosapic_lists[index].num_rte - 1;

		if (gsi_end < base || end < gsi_base)
			continue; /* OK */

		return -EBUSY;
	}
	return 0;
}

static int
iosapic_delete_rte(unsigned int irq, unsigned int gsi)
{
	struct iosapic_rte_info *rte, *temp;

	list_for_each_entry_safe(rte, temp, &iosapic_intr_info[irq].rtes,
								rte_list) {
		if (rte->iosapic->gsi_base + rte->rte_index == gsi) {
			if (rte->refcnt)
				return -EBUSY;

			list_del(&rte->rte_list);
			kfree(rte);
			return 0;
		}
	}

	return -EINVAL;
}

int iosapic_init(unsigned long phys_addr, unsigned int gsi_base)
{
	int num_rte, err, index;
	unsigned int isa_irq, ver;
	char __iomem *addr;
	unsigned long flags;

	spin_lock_irqsave(&iosapic_lock, flags);
	index = find_iosapic(gsi_base);
	if (index >= 0) {
		spin_unlock_irqrestore(&iosapic_lock, flags);
		return -EBUSY;
	}

	addr = ioremap(phys_addr, 0);
	if (addr == NULL) {
		spin_unlock_irqrestore(&iosapic_lock, flags);
		return -ENOMEM;
	}
	ver = iosapic_version(addr);
	if ((err = iosapic_check_gsi_range(gsi_base, ver))) {
		iounmap(addr);
		spin_unlock_irqrestore(&iosapic_lock, flags);
		return err;
	}

	/*
	 * The MAX_REDIR register holds the highest input pin number
	 * (starting from 0).  We add 1 so that we can use it for
	 * number of pins (= RTEs)
	 */
	num_rte = ((ver >> 16) & 0xff) + 1;

	index = iosapic_alloc();
	iosapic_lists[index].addr = addr;
	iosapic_lists[index].gsi_base = gsi_base;
	iosapic_lists[index].num_rte = num_rte;
#ifdef CONFIG_NUMA
	iosapic_lists[index].node = MAX_NUMNODES;
#endif
	spin_lock_init(&iosapic_lists[index].lock);
	spin_unlock_irqrestore(&iosapic_lock, flags);

	if ((gsi_base == 0) && pcat_compat) {
		/*
		 * Map the legacy ISA devices into the IOSAPIC data.  Some of
		 * these may get reprogrammed later on with data from the ACPI
		 * Interrupt Source Override table.
		 */
		for (isa_irq = 0; isa_irq < 16; ++isa_irq)
			iosapic_override_isa_irq(isa_irq, isa_irq,
						 IOSAPIC_POL_HIGH,
						 IOSAPIC_EDGE);
	}
	return 0;
}

int iosapic_remove(unsigned int gsi_base)
{
	int i, irq, index, err = 0;
	unsigned long flags;

	spin_lock_irqsave(&iosapic_lock, flags);
	index = find_iosapic(gsi_base);
	if (index < 0) {
		printk(KERN_WARNING "%s: No IOSAPIC for GSI base %u\n",
		       __func__, gsi_base);
		goto out;
	}

	if (iosapic_lists[index].rtes_inuse) {
		err = -EBUSY;
		printk(KERN_WARNING "%s: IOSAPIC for GSI base %u is busy\n",
		       __func__, gsi_base);
		goto out;
	}

	for (i = gsi_base; i < gsi_base + iosapic_lists[index].num_rte; i++) {
		irq = __gsi_to_irq(i);
		if (irq < 0)
			continue;

		err = iosapic_delete_rte(irq, i);
		if (err)
			goto out;
	}

	iounmap(iosapic_lists[index].addr);
	iosapic_free(index);
 out:
	spin_unlock_irqrestore(&iosapic_lock, flags);
	return err;
}

#ifdef CONFIG_NUMA
void map_iosapic_to_node(unsigned int gsi_base, int node)
{
	int index;

	index = find_iosapic(gsi_base);
	if (index < 0) {
		printk(KERN_WARNING "%s: No IOSAPIC for GSI %u\n",
		       __func__, gsi_base);
		return;
	}
	iosapic_lists[index].node = node;
	return;
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 *	linux/arch/ia64/kernel/irq.c
 *
 *	Copyright (C) 1992, 1998 Linus Torvalds, Ingo Molnar
 *
 * This file contains the code used by various IRQ handling routines:
 * asking for different IRQs should be done through these routines
 * instead of just grabbing them. Thus setups with different IRQ numbers
 * shouldn't result in any weird surprises, and installing new handlers
 * should be easier.
 *
 * Copyright (C) Ashok Raj<ashok.raj@intel.com>, Intel Corporation 2004
 *
 * 4/14/2004: Added code to handle cpu migration and do safe irq
 *			migration without losing interrupts for iosapic
 *			architecture.
 */

#include <asm/delay.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>

#include <asm/mca.h>

/*
 * 'what should we do if we get a hw irq event on an illegal vector'.
 * each architecture has to answer this themselves.
 */
void ack_bad_irq(unsigned int i