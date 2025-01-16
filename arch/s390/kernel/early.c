nux/bootmem.h>

#include <asm/delay.h>
#include <asm/hw_irq.h>
#include <asm/io.h>
#include <asm/iosapic.h>
#include <asm/machvec.h>
#include <asm/processor.h>
#include <asm/ptrace.h>

#undef DEBUG_INTERRUPT_ROUTING

#ifdef DEBUG_INTERRUPT_ROUTING
#define DBG(fmt...)	printk(fmt)
#else
#define DBG(fmt...)
#endif

static DEFINE_SPINLOCK(iosapic_lock);

/*
 * These tables map IA-64 vectors to the IOSAPIC pin that generates this
 * vector.
 */

#define NO_REF_RTE	0

static struct iosapic {
	char __iomem	*addr;		/* base address of IOSAPIC */
	unsigned int	gsi_base;	/* GSI base */
	unsigned short	num_rte;	/* # of RTEs on this IOSAPIC */
	int		rtes_inuse;	/* # of RTEs in use on this IOSAPIC */
#ifdef CONFIG_NUMA
	unsigned short	node;		/* numa node association via pxm */
#endif
	spinlock_t	lock;		/* lock for indirect reg access */
} iosapic_lists[NR_IOSAPICS];

struct iosapic_rte_info {
	struct list_head rte_list;	/* RTEs sharing the same vector */
	char		rte_index;	/* IOSAPIC RTE index */
	int		refcnt;		/* reference counter */
	struct iosapic	*iosapic;
} ____cacheline_aligned;

static struct iosapic_intr_info {
	struct list_head rtes;		/* RTEs using this vector (empty =>
					 * not an IOSAPIC interrupt) */
	int		count;		/* # of registered RTEs */
	u32		low32;		/* current value of low word of
					 * Redirection table entry */
	unsigned int	dest;		/* destination CPU physical ID */
	unsigned char	dmode	: 3;	/* delivery mode (see iosapic.h) */
	unsigned char 	polarity: 1;	/* interrupt polarity
					 * (see iosapic.h) */
	unsigned char	trigger	: 1;	/* trigger mode (see iosapic.h) */
} iosapic_intr_info[NR_IRQS];

static unsigned char pcat_compat;	/* 8259 compatibility flag */

static inline void
iosapic_write(struct iosapic *iosapic, unsigned int reg, u32 val)
{
	unsigned long flags;

	spin_lock_irqsave(&iosapic->lock, flags);
	__iosapic_write(iosapic->addr, reg, val);
	spin_unlock_irqrestore(&iosapic->lock, flags);
}

/*
 * Find an IOSAPIC associated with a GSI
 */
static inline int
find_iosapic (unsigned int gsi)
{
	int i;

	for (i = 0; i < NR_IOSAPICS; i++) {
		if ((unsigned) (gsi - iosapic_lists[i].gsi_base) <
		    iosapic_lists[i].num_rte)
			return i;
	}

	return -1;
}

static inline int __gsi_to_irq(unsigned int gsi)
{
	int irq;
	struct iosapic_intr_info *info;
	struct iosapic_rte_info *rte;

	for (irq = 0; irq < NR_IRQS; irq++) {
		info = &iosapic_intr_info[irq];
		list_for_each_entry(rte, &info->rtes, rte_list)
			if (rte->iosapic->gsi_base + rte->rte_index == gsi)
				return irq;
	}
	return -1;
}

int
gsi_to_irq (unsigned int gsi)
{
	unsigned long flags;
	int irq;

	spin_lock_irqsave(&iosapic_lock, flags);
	irq = __gsi_to_irq(gsi);
	spin_unlock_irqrestore(&iosapic_lock, flags);
	return irq;
}

static struct iosapic_rte_info *find_rte(unsigned int irq, unsigned int gsi)
{
	struct iosapic_rte_info *rte;

	list_for_each_entry(rte, &iosapic_intr_info[irq].rtes, rte_list)
		if (rte->iosapic->gsi_base + rte->rte_index == gsi)
			return rte;
	return NULL;
}

static void
set_rte (unsigned int gsi, unsigned int irq, unsigned int dest, int mask)
{
	unsigned long pol, trigger, dmode;
	u32 low32, high32;
	int rte_index;
	char redir;
	struct iosapic_rte_info *rte;
	ia64_vector vector = irq_to_vector(irq);

	DBG(KERN_DEBUG"IOSAPIC: routing vector %d to 0x%x\n", vector, dest);

	rte = find_rte(irq, gsi);
	if (!rte)
		return;		/* not an IOSAPIC interrupt */

	rte_index = rte->rte_index;
	pol     = iosapic_intr_info[irq].polarity;
	trigger = iosapic_intr_info[irq].trigger;
	dmode   = iosapic_intr_info[irq].dmode;

	redir = (dmode == IOSAPIC_LOWEST_PRIORITY) ? 1 : 0;

#ifdef CONFIG_SMP
	set_irq_affinity_info(irq, (int)(dest & 0xffff), redir);
#endif

	low32 = ((pol << IOSAPIC_POLARITY_SHIFT) |
		 (trigger << IOSAPIC_TRIGGER_SHIFT) |
		 (dmode << IOSAPIC_DELIVERY_SHIFT) |
		 ((mask ? 1 : 0) << IOSAPIC_MASK_SHIFT) |
		 vector);

	/* dest contains both id and eid */
	high32 = (dest << IOSAPIC_DEST_SHIFT);

	iosapic_write(rte->iosapic, IOSAPIC_RTE_HIGH(rte_index), high32);
	iosapic_write(rte->iosapic, IOSAPIC_RTE_LOW(rte_index), low32);
	iosapic_intr_info[irq].low32 = low32;
	iosapic_intr_info[irq].dest = dest;
}

static void
nop (struct irq_data *data)
{
	/* do nothing... */
}


#ifdef CONFIG_KEXEC
void
kexec_disable_iosapic(void)
{
	struct iosapic_intr_info *info;
	struct iosapic_rte_info *rte;
	ia64_vector vec;
	int irq;

	for (irq = 0; irq < NR_IRQS; irq++) {
		info = &iosapic_intr_info[irq];
		vec = irq_to_vector(irq);
		list_for_each_entry(rte, &info->rtes,
				rte_list) {
			iosapic_write(rte->iosapic,
					IOSAPIC_RTE_LOW(rte->rte_index),
					IOSAPIC_MASK|vec);
			iosapic_eoi(rte->iosapic->addr, vec);
		}
	}
}
#endif

static void
mask_irq (struct irq_data *data)
{
	unsigned int irq = data->irq;
	u32 low32;
	int rte_index;
	struct iosapic_rte_info *rte;

	if (!iosapic_intr_info[irq].count)
		return;			/* not an IOSAPIC interrupt! */

	/* set only the mask bit */
	low32 = iosapic_intr_info[irq].low32 |= IOSAPIC_MASK;
	list_for_each_entry(rte, &iosapic_intr_info[irq].rtes, rte_list) {
		rte_index = rte->rte_index;
		iosapic_write(rte->iosapic, IOSAPIC_RTE_LOW(rte_index), low32);
	}
}

static void
unmask_irq (struct irq_data *data)
{
	unsigned int irq = data->irq;
	u32 low32;
	int rte_index;
	struct iosapic_rte_info *rte;

	if (!iosapic_intr_info[irq].count)
		return;			/* not an IOSAPIC interrupt! */

	low32 = iosapic_intr_info[irq].low32 &= ~IOSAPIC_MASK;
	list_for_each_entry(rte, &iosapic_intr_info[irq].rtes, rte_list) {
		rte_index = rte->rte_index;
		iosapic_write(rte->iosapic, IOSAPIC_RTE_LOW(rte_index), low32);
	}
}


static int
iosapic_set_affinity(struct irq_data *data, const struct cpumask *mask,
		     bool force)
{
#ifdef CONFIG_SMP
	unsigned int irq = data->irq;
	u32 high32, low32;
	int cpu, dest, rte_index;
	int redir = (irq & IA64_IRQ_REDIRECTED) ? 1 : 0;
	struct iosapic_rte_info *rte;
	struct iosapic *iosapic;

	irq &= (~IA64_IRQ_REDIRECTED);

	cpu = cpumask_first_and(cpu_online_mask, mask);
	if (cpu >= nr_cpu_ids)
		return -1;

	if (irq_prepare_move(irq, cpu))
		return -1;

	dest = cpu_physical_id(cpu);

	if (!iosapic_intr_info[irq].count)
		return -1;			/* not an IOSAPIC interrupt */

	set_irq_affinity_info(irq, dest, redir);

	/* dest contains both id and eid */
	high32 = dest << IOSAPIC_DEST_SHIFT;

	low32 = iosapic_intr_info[irq].low32 & ~(7 << IOSAPIC_DELIVERY_SHIFT);
	if (redir)
		/* change delivery mode to lowest priority */
		low32 |= (IOSAPIC_LOWEST_PRIORITY << IOSAPIC_DELIVERY_SHIFT);
	else
		/* change delivery mode to fixed */
		low32 |= (IOSAPIC_FIXED << IOSAPIC_DELIVERY_SHIFT);
	low32 &= IOSAPIC_VECTOR_MASK;
	low32 |= irq_to_vector(irq);

	iosapic_intr_info[irq].low32 = low32;
	iosapic_intr_info[irq].dest = dest;
	list_for_each_entry(rte, &iosapic_intr_info[irq].rtes, rte_list) {
		iosapic = rte->iosapic;
		rte_index = rte->rte_index;
		iosapic_write(iosapic, IOSAPIC_RTE_HIGH(rte_index), high32);
		iosapic_write(iosapic, IOSAPIC_RTE_LOW(rte_index), low32);
	}

#endif
	return 0;
}

/*
 * Handlers for level-triggered interrupts.
 */

static unsigned int
iosapic_startup_level_irq (struct irq_data *data)
{
	unmask_irq(data);
	return 0;
}

static void
iosapic_unmask_level_irq (struct irq_data *data)
{
	unsigned int irq = data->irq;
	ia64_vector vec = irq_to_vector(irq);
	struct iosapic_rte_info *rte;
	int do_unmask_irq = 0;

	irq_complete_move(irq);
	if (unlikely(irqd_is_setaffinity_pending(data))) {
		do_unmask_irq = 1;
		mask_irq(data);
	} else
		unmask_irq(data);

	list_for_each_entry(rte, &iosapic_intr_info[irq].rtes, rte_list)
		iosapic_eoi(rte->iosapic->addr, vec);

	if (unlikely(do_unmask_irq)) {
		irq_move_masked_irq(data);
		unmask_irq(data);
	}
}

#define iosapic_shutdown_level_irq	mask_irq
#define iosapic_enable_level_irq	unmask_irq
#define iosapic_disable_level_irq	mask_irq
#define iosapic_ack_level_irq		nop

static struct irq_chip irq_type_iosapic_level = {
	.name =			"IO-SAPIC-level",
	.irq_startup =		iosapic_startup_level_irq,
	.irq_shutdown =		iosapic_shutdown_level_irq,
	.irq_enable =		iosapic_enable_level_irq,
	.irq_disable =		iosapic_disable_level_irq,
	.irq_ack =		iosapic_ack_level_irq,
	.irq_mask =		mask_irq,
	.irq_unmask =		iosapic_unmask_level_irq,
	.irq_set_affinity =	iosapic_set_affinity
};

/*
 * Handlers for edge-triggered interrupts.
 */

static unsigned int
iosapic_startup_edge_irq (struct irq_data *data)
{
	unmask_irq(data);
	/*
	 * IOSAPIC simply drops interrupts pended while the
	 * corresponding pin was masked, so we can't know if an
	 * interrupt is pending already.  Let's hope not...
	 */
	return 0;
}

static void
iosapic_ack_edge_irq (struct irq_data *data)
{
	irq_complete_move(data->irq);
	irq_move_irq(data);
}

#define iosapic_enable_edge_irq		unmask_irq
#define iosapic_disable_edge_irq	nop

static struct irq_chip irq_type_iosapic_edge = {
	.name =			"IO-SAPIC-edge",
	.irq_startup =		iosapic_startup_edge_irq,
	.irq_shutdown =		iosapic_disable_edge_irq,
	.irq_enable =		iosapic_enable_edge_irq,
	.irq_disable =		iosapic_disable_edge_irq,
	.irq_ack =		iosapic_ack_edge_irq,
	.irq_mask =		mask_irq,
	.irq_unmask =		unmask_irq,
	.irq_set_affinity =	iosapic_set_affinity
};

static unsigned int
iosapic_version (char __iomem *addr)
{
	/*
	 * IOSAPIC Version Register return 32 bit structure like:
	 * {
	 *	unsigned int version   : 8;
	 *	unsigned int reserved1 : 8;
	 *	unsigned int max_redir : 8;
	 *	unsigned int reserved2 : 8;
	 * }
	 */
	return __iosapic_read(addr, IOSAPIC_VERSION);
}

static int iosapic_find_sharable_irq(unsigned long trigger, unsigned long pol)
{
	int i, irq = -ENOSPC, min_count = -1;
	struct iosapic_intr_info *info;

	/*
	 * shared vectors for edge-triggered interrupts are not
	 * supported yet
	 */
	if (trigger == IOSAPIC_EDGE)
		return -EINVAL;

	for (i = 0; i < NR_IRQS; i++) {
		info = &iosapic_intr_info[i];
		if (info->trigger == trigger && info->polarity == pol &&
		    (info->dmode == IOSAPIC_FIXED ||
		     info->dmode == IOSAPIC_LOWEST_PRIORITY) &&
		    can_request_irq(i, IRQF_SHARED)) {
			if (min_count == -1 || info->count < min_count) {
				irq = i;
				min_count = info->count;
			}
		}
	}
	return irq;
}

/*
 * if the given vector is already owned by other,
 *  assign a new vector for the other and make the vector available
 */
static void __init
iosapic_reassign_vector (int irq)
{
	int new_irq;

	if (iosapic_intr_info[irq].count) {
		new_irq = create_irq();
		if (new_irq < 0)
			panic("%s: out of interrupt vectors!\n", __func__);
		printk(KERN_INFO "Reassigning vector %d to %d\n",
		       irq_to_vector(irq), irq_to_vector(new_irq));
		memcpy(&iosapic_intr_info[new_irq], &iosapic_intr_info[irq],
		       sizeof(struct iosapic_intr_info));
		INIT_LIST_HEAD(&iosapic_intr_info[new_irq].rtes);
		list_move(iosapic_intr_info[irq].rtes.next,
			  &iosapic_intr_info[new_irq].rtes);
		memset(&iosapic_intr_info[irq], 0,
		       sizeof(struct iosapic_intr_info));
		iosapic_intr_info[irq].low32 = IOSAPIC_MASK;
		INIT_LIST_HEAD(&iosapic_intr_info[irq].rtes);
	}
}

static inline int irq_is_shared (int irq)
{
	return (iosapic_intr_info[irq].count > 1);
}

struct irq_chip*
ia64_native_iosapic_get_irq_chip(unsigned long trigger)
{
	if (trigger == IOSAPIC_EDGE)
		return &irq_type_iosapic_edge;
	else
		return &irq_type_iosapic_level;
}

static int
register_intr (unsigned int gsi, int irq, unsigned char delivery,
	       unsigned long polarity, unsigned long trigger)
{
	struct irq_chip *chip, *irq_type;
	int index;
	struct iosapic_rte_info *rte;

	index = find_iosapic(gsi);
	if (index < 0) {
		printk(KERN_WARNING "%s: No IOSAPIC for GSI %u\n",
		       __func__, gsi);
		return -ENODEV;
	}

	rte = find_rte(irq, gsi);
	if (!rte) {
		rte = kzalloc(sizeof (*rte), GFP_ATOMIC);
		if (!rte) {
			printk(KERN_WARNING "%s: cannot allocate memory\n",
			       __func__);
			return -ENOMEM