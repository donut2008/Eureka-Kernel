_TPREL:
	      case RV_LTREL_TPREL:
	      case RV_DTPMOD:
	      case RV_LTREL_DTPMOD:
	      case RV_DTPREL:
	      case RV_LTREL_DTPREL:
		printk(KERN_ERR "%s: %s reloc not supported\n",
		       mod->name, reloc_name[r_type] ? reloc_name[r_type] : "?");
		return -ENOEXEC;

	      default:
		printk(KERN_ERR "%s: unknown reloc %x\n", mod->name, r_type);
		return -ENOEXEC;
	}

	if (!ok)
		return -ENOEXEC;

	DEBUGP("%s: [%p]<-%016lx = %s(%lx)\n", __func__, location, val,
	       reloc_name[r_type] ? reloc_name[r_type] : "?", sym->st_value + addend);

	switch (format) {
	      case RF_INSN21B:	ok = apply_imm21b(mod, location, (int64_t) val / 16); break;
	      case RF_INSN22:	ok = apply_imm22(mod, location, val); break;
	      case RF_INSN64:	ok = apply_imm64(mod, location, val); break;
	      case RF_INSN60:	ok = apply_imm60(mod, location, (int64_t) val / 16); break;
	      case RF_32LSB:	put_unaligned(val, (uint32_t *) location); break;
	      case RF_64LSB:	put_unaligned(val, (uint64_t *) location); break;
	      case RF_32MSB:	/* ia64 Linux is little-endian... */
	      case RF_64MSB:	/* ia64 Linux is little-endian... */
	      case RF_INSN14:	/* must be within-module, i.e., resolved by "ld -r" */
	      case RF_INSN21M:	/* must be within-module, i.e., resolved by "ld -r" */
	      case RF_INSN21F:	/* must be within-module, i.e., resolved by "ld -r" */
		printk(KERN_ERR "%s: format %u needed by %s reloc is not supported\n",
		       mod->name, format, reloc_name[r_type] ? reloc_name[r_type] : "?");
		return -ENOEXEC;

	      default:
		printk(KERN_ERR "%s: relocation %s resulted in unknown format %u\n",
		       mod->name, reloc_name[r_type] ? reloc_name[r_type] : "?", format);
		return -ENOEXEC;
	}
	return ok ? 0 : -ENOEXEC;
}

int
apply_relocate_add (Elf64_Shdr *sechdrs, const char *strtab, unsigned int symindex,
		    unsigned int relsec, struct module *mod)
{
	unsigned int i, n = sechdrs[relsec].sh_size / sizeof(Elf64_Rela);
	Elf64_Rela *rela = (void *) sechdrs[relsec].sh_addr;
	Elf64_Shdr *target_sec;
	int ret;

	DEBUGP("%s: applying section %u (%u relocs) to %u\n", __func__,
	       relsec, n, sechdrs[relsec].sh_info);

	target_sec = sechdrs + sechdrs[relsec].sh_info;

	if (target_sec->sh_entsize == ~0UL)
		/*
		 * If target section wasn't allocated, we don't need to relocate it.
		 * Happens, e.g., for debug sections.
		 */
		return 0;

	if (!mod->arch.gp) {
		/*
		 * XXX Should have an arch-hook for running this after final section
		 *     addresses have been selected...
		 */
		uint64_t gp;
		if (mod->core_size > MAX_LTOFF)
			/*
			 * This takes advantage of fact that SHF_ARCH_SMALL gets allocated
			 * at the end of the module.
			 */
			gp = mod->core_size - MAX_LTOFF / 2;
		else
			gp = mod->core_size / 2;
		gp = (uint64_t) mod->module_core + ((gp + 7) & -8);
		mod->arch.gp = gp;
		DEBUGP("%s: placing gp at 0x%lx\n", __func__, gp);
	}

	for (i = 0; i < n; i++) {
		ret = do_reloc(mod, ELF64_R_TYPE(rela[i].r_info),
			       ((Elf64_Sym *) sechdrs[symindex].sh_addr
				+ ELF64_R_SYM(rela[i].r_info)),
			       rela[i].r_addend, target_sec,
			       (void *) target_sec->sh_addr + rela[i].r_offset);
		if (ret < 0)
			return ret;
	}
	return 0;
}

/*
 * Modules contain a single unwind table which covers both the core and the init text
 * sections but since the two are not contiguous, we need to split this table up such that
 * we can register (and unregister) each "segment" separately.  Fortunately, this sounds
 * more complicated than it really is.
 */
static void
register_unwind_table (struct module *mod)
{
	struct unw_table_entry *start = (void *) mod->arch.unwind->sh_addr;
	struct unw_table_entry *end = start + mod->arch.unwind->sh_size / sizeof (*start);
	struct unw_table_entry tmp, *e1, *e2, *core, *init;
	unsigned long num_init = 0, num_core = 0;

	/* First, count how many init and core unwind-table entries there are.  */
	for (e1 = start; e1 < end; ++e1)
		if (in_init(mod, e1->start_offset))
			++num_init;
		else
			++num_core;
	/*
	 * Second, sort the table such that all unwind-table entries for the init and core
	 * text sections are nicely separated.  We do this with a stupid bubble sort
	 * (unwind tables don't get ridiculously huge).
	 */
	for (e1 = start; e1 < end; ++e1) {
		for (e2 = e1 + 1; e2 < end; ++e2) {
			if (e2->start_offset < e1->start_offset) {
				tmp = *e1;
				*e1 = *e2;
				*e2 = tmp;
			}
		}
	}
	/*
	 * Third, locate the init and core segments in the unwind table:
	 */
	if (in_init(mod, start->start_offset)) {
		init = start;
		core = start + num_init;
	} else {
		core = start;
		init = start + num_core;
	}

	DEBUGP("%s: name=%s, gp=%lx, num_init=%lu, num_core=%lu\n", __func__,
	       mod->name, mod->arch.gp, num_init, num_core);

	/*
	 * Fourth, register both tables (if not empty).
	 */
	if (num_core > 0) {
		mod->arch.core_unw_table = unw_add_unwind_table(mod->name, 0, mod->arch.gp,
								core, core + num_core);
		DEBUGP("%s:  core: handle=%p [%p-%p)\n", __func__,
		       mod->arch.core_unw_table, core, core + num_core);
	}
	if (num_init > 0) {
		mod->arch.init_unw_table = unw_add_unwind_table(mod->name, 0, mod->arch.gp,
								init, init + num_init);
		DEBUGP("%s:  init: handle=%p [%p-%p)\n", __func__,
		       mod->arch.init_unw_table, init, init + num_init);
	}
}

int
module_finalize (const Elf_Ehdr *hdr, const Elf_Shdr *sechdrs, struct module *mod)
{
	DEBUGP("%s: init: entry=%p\n", __func__, mod->init);
	if (mod->arch.unwind)
		register_unwind_table(mod);
	return 0;
}

void
module_arch_cleanup (struct module *mod)
{
	if (mod->arch.init_unw_table) {
		unw_remove_unwind_table(mod->arch.init_unw_table);
		mod->arch.init_unw_table = NULL;
	}
	if (mod->arch.core_unw_table) {
		unw_remove_unwind_table(mod->arch.core_unw_table);
		mod->arch.core_unw_table = NULL;
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * MSI hooks for standard x86 apic
 */

#include <linux/pci.h>
#include <linux/irq.h>
#include <linux/msi.h>
#include <linux/dmar.h>
#include <asm/smp.h>
#include <asm/msidef.h>

static struct irq_chip	ia64_msi_chip;

#ifdef CONFIG_SMP
static int ia64_set_msi_irq_affinity(struct irq_data *idata,
				     const cpumask_t *cpu_mask, bool force)
{
	struct msi_msg msg;
	u32 addr, data;
	int cpu = cpumask_first_and(cpu_mask, cpu_online_mask);
	unsigned int irq = idata->irq;

	if (irq_prepare_move(irq, cpu))
		return -1;

	__get_cached_msi_msg(irq_data_get_msi_desc(idata), &msg);

	addr = msg.address_lo;
	addr &= MSI_ADDR_DEST_ID_MASK;
	addr |= MSI_ADDR_DEST_ID_CPU(cpu_physical_id(cpu));
	msg.address_lo = addr;

	data = msg.data;
	data &= MSI_DATA_VECTOR_MASK;
	data |= MSI_DATA_VECTOR(irq_to_vector(irq));
	msg.data = data;

	pci_write_msi_msg(irq, &msg);
	cpumask_copy(irq_data_get_affinity_mask(idata), cpumask_of(cpu));

	return 0;
}
#endif /* CONFIG_SMP */

int ia64_setup_msi_irq(struct pci_dev *pdev, struct msi_desc *desc)
{
	struct msi_msg	msg;
	unsigned long	dest_phys_id;
	int	irq, vector;

	irq = create_irq();
	if (irq < 0)
		return irq;

	irq_set_msi_desc(irq, desc);
	dest_phys_id = cpu_physical_id(cpumask_any_and(&(irq_to_domain(irq)),
						       cpu_online_mask));
	vector = irq_to_vector(irq);

	msg.address_hi = 0;
	msg.address_lo =
		MSI_ADDR_HEADER |
		MSI_ADDR_DEST_MODE_PHYS |
		MSI_ADDR_REDIRECTION_CPU |
		MSI_ADDR_DEST_ID_CPU(dest_phys_id);

	msg.data =
		MSI_DATA_TRIGGER_EDGE |
		MSI_DATA_LEVEL_ASSERT |
		MSI_DATA_DELIVERY_FIXED |
		MSI_DATA_VECTOR(vector);

	pci_write_msi_msg(irq, &msg);
	irq_set_chip_and_handler(irq, &ia64_msi_chip, handle_edge_irq);

	return 0;
}

void ia64_teardown_msi_irq(unsigned int irq)
{
	destroy_irq(irq);
}

static void ia64_ack_msi_irq(struct irq_data *data)
{
	irq_complete_move(data->irq);
	irq_move_irq(data);
	ia64_eoi();
}

static int ia64_msi_retrigger_irq(struct irq_data *data)
{
	unsigned int vector = irq_to_vector(data->irq);
	ia64_resend_irq(vector);

	return 1;
}

/*
 * Generic ops used on most IA64 platforms.
 */
static struct irq_chip ia64_msi_chip = {
	.name			= "PCI-MSI",
	.irq_mask		= pci_msi_mask_irq,
	.irq_unmask		= pci_msi_unmask_irq,
	.irq_ack		= ia64_ack_msi_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	= ia64_set_msi_irq_affinity,
#endif
	.irq_retrigger		= ia64_msi_retrigger_irq,
};


int arch_setup_msi_irq(struct pci_dev *pdev, struct msi_desc *desc)
{
	if (platform_setup_msi_irq)
		return platform_setup_msi_irq(pdev, desc);

	return ia64_setup_msi_irq(pdev, desc);
}

void arch_teardown_msi_irq(unsigned int irq)
{
	if (platform_teardown_msi_irq)
		return platform_teardown_msi_irq(irq);

	return ia64_teardown_msi_irq(irq);
}

#ifdef CONFIG_INTEL_IOMMU
#ifdef CONFIG_SMP
static int dmar_msi_set_affinity(struct irq_data *data,
				 const struct cpumask *mask, bool force)
{
	unsigned int irq = data->irq;
	struct irq_cfg *cfg = irq_cfg + irq;
	struct msi_msg msg;
	int cpu = cpumask_first_and(mask, cpu_online_mask);

	if (irq_prepare_move(irq, cpu))
		return -1;

	dmar_msi_read(irq, &msg);

	msg.data &= ~MSI_DATA_VECTOR_MASK;
	msg.data |= MSI_DATA_VECTOR(cfg->vector);
	msg.address_lo &= ~MSI_ADDR_DEST_ID_MASK;
	msg.address_lo |= MSI_ADDR_DEST_ID_CPU(cpu_physical_id(cpu));

	dmar_msi_write(irq, &msg);
	cpumask_copy(irq_data_get_affinity_mask(data), mask);

	return 0;
}
#endif /* CONFIG_SMP */

static struct irq_chip dmar_msi_type = {
	.name = "DMAR_MSI",
	.irq_unmask = dmar_msi_unmask,
	.irq_mask = dmar_msi_mask,
	.irq_ack = ia64_ack_msi_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity = dmar_msi_set_affinity,
#endif
	.irq_retrigger = ia64_msi_retrigger_irq,
};

static void
msi_compose_msg(struct pci_dev *pdev, unsigned int irq, struct msi_msg *msg)
{
	struct irq_cfg *cfg = irq_cfg + irq;
	unsigned dest;

	dest = cpu_physical_id(cpumask_first_and(&(irq_to_domain(irq)),
						 cpu_online_mask));

	msg->address_hi = 0;
	msg->address_lo =
		MSI_ADDR_HEADER |
		MSI_ADDR_DEST_MODE_PHYS |
		MSI_ADDR_REDIRECTION_CPU |
		MSI_ADDR_DEST_ID_CPU(dest);

	msg->data =
		MSI_DATA_TRIGGER_EDGE |
		MSI_DATA_LEVEL_ASSERT |
		MSI_DATA_DELIVERY_FIXED |
		MSI_DATA_VECTOR(cfg->vector);
}

int dmar_alloc_hwirq(int id, int node, void *arg)
{
	int irq;
	struct msi_msg msg;

	irq = create_irq();
	if (irq > 0) {
		irq_set_handler_data(irq, arg);
		irq_set_chip_and_handler_name(irq, &dmar_msi_type,
					      handle_edge_irq, "edge");
		msi_compose_msg(NULL, irq, &msg);
		dmar_msi_write(irq, &msg);
	}

	return irq;
}

void dmar_free_hwirq(int irq)
{
	irq_set_handler_data(ir