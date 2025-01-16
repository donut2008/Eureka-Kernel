 -> bit 16 */
		| ((b1 & 0x0000100000000000) >> 23)		/* ic -> bit 21 */
		| ((b0 >> 46) << 22) | ((b1 & 0x7fffff) << 40)	/* imm41 -> bit 22 */
		| ((b1 & 0x0800000000000000) <<  4));		/* i -> bit 63 */
}

#endif /* !USE_BRL */

void
module_arch_freeing_init (struct module *mod)
{
	if (mod->arch.init_unw_table) {
		unw_remove_unwind_table(mod->arch.init_unw_table);
		mod->arch.init_unw_table = NULL;
	}
}

/* Have we already seen one of these relocations? */
/* FIXME: we could look in other sections, too --RR */
static int
duplicate_reloc (const Elf64_Rela *rela, unsigned int num)
{
	unsigned int i;

	for (i = 0; i < num; i++) {
		if (rela[i].r_info == rela[num].r_info && rela[i].r_addend == rela[num].r_addend)
			return 1;
	}
	return 0;
}

/* Count how many GOT entries we may need */
static unsigned int
count_gots (const Elf64_Rela *rela, unsigned int num)
{
	unsigned int i, ret = 0;

	/* Sure, this is order(n^2), but it's usually short, and not
           time critical */
	for (i = 0; i < num; i++) {
		switch (ELF64_R_TYPE(rela[i].r_info)) {
		      case R_IA64_LTOFF22:
		      case R_IA64_LTOFF22X:
		      case R_IA64_LTOFF64I:
		      case R_IA64_LTOFF_FPTR22:
		      case R_IA64_LTOFF_FPTR64I:
		      case R_IA64_LTOFF_FPTR32MSB:
		      case R_IA64_LTOFF_FPTR32LSB:
		      case R_IA64_LTOFF_FPTR64MSB:
		      case R_IA64_LTOFF_FPTR64LSB:
			if (!duplicate_reloc(rela, i))
				ret++;
			break;
		}
	}
	return ret;
}

/* Count how many PLT entries we may need */
static unsigned int
count_plts (const Elf64_Rela *rela, unsigned int num)
{
	unsigned int i, ret = 0;

	/* Sure, this is order(n^2), but it's usually short, and not
           time critical */
	for (i = 0; i < num; i++) {
		switch (ELF64_R_TYPE(rela[i].r_info)) {
		      case R_IA64_PCREL21B:
		      case R_IA64_PLTOFF22:
		      case R_IA64_PLTOFF64I:
		      case R_IA64_PLTOFF64MSB:
		      case R_IA64_PLTOFF64LSB:
		      case R_IA64_IPLTMSB:
		      case R_IA64_IPLTLSB:
			if (!duplicate_reloc(rela, i))
				ret++;
			break;
		}
	}
	return ret;
}

/* We need to create an function-descriptors for any internal function
   which is referenced. */
static unsigned int
count_fdescs (const Elf64_Rela *rela, unsigned int num)
{
	unsigned int i, ret = 0;

	/* Sure, this is order(n^2), but it's usually short, and not time critical.  */
	for (i = 0; i < num; i++) {
		switch (ELF64_R_TYPE(rela[i].r_info)) {
		      case R_IA64_FPTR64I:
		      case R_IA64_FPTR32LSB:
		      case R_IA64_FPTR32MSB:
		      case R_IA64_FPTR64LSB:
		      case R_IA64_FPTR64MSB:
		      case R_IA64_LTOFF_FPTR22:
		      case R_IA64_LTOFF_FPTR32LSB:
		      case R_IA64_LTOFF_FPTR32MSB:
		      case R_IA64_LTOFF_FPTR64I:
		      case R_IA64_LTOFF_FPTR64LSB:
		      case R_IA64_LTOFF_FPTR64MSB:
		      case R_IA64_IPLTMSB:
		      case R_IA64_IPLTLSB:
			/*
			 * Jumps to static functions sometimes go straight to their
			 * offset.  Of course, that may not be possible if the jump is
			 * from init -> core or vice. versa, so we need to generate an
			 * FDESC (and PLT etc) for that.
			 */
		      case R_IA64_PCREL21B:
			if (!duplicate_reloc(rela, i))
				ret++;
			break;
		}
	}
	return ret;
}

int
module_frob_arch_sections (Elf_Ehdr *ehdr, Elf_Shdr *sechdrs, char *secstrings,
			   struct module *mod)
{
	unsigned long core_plts = 0, init_plts = 0, gots = 0, fdescs = 0;
	Elf64_Shdr *s, *sechdrs_end = sechdrs + ehdr->e_shnum;

	/*
	 * To store the PLTs and function-descriptors, we expand the .text section for
	 * core module-code and the .init.text section for initialization code.
	 */
	for (s = sechdrs; s < sechdrs_end; ++s)
		if (strcmp(".core.plt", secstrings + s->sh_name) == 0)
			mod->arch.core_plt = s;
		else if (strcmp(".init.plt", secstrings + s->sh_name) == 0)
			mod->arch.init_plt = s;
		else if (strcmp(".got", secstrings + s->sh_name) == 0)
			mod->arch.got = s;
		else if (strcmp(".opd", secstrings + s->sh_name) == 0)
			mod->arch.opd = s;
		else if (strcmp(".IA_64.unwind", secstrings + s->sh_name) == 0)
			mod->arch.unwind = s;

	if (!mod->arch.core_plt || !mod->arch.init_plt || !mod->arch.got || !mod->arch.opd) {
		printk(KERN_ERR "%s: sections missing\n", mod->name);
		return -ENOEXEC;
	}

	/* GOT and PLTs can occur in any relocated section... */
	for (s = sechdrs + 1; s < sechdrs_end; ++s) {
		const Elf64_Rela *rels = (void *)ehdr + s->sh_offset;
		unsigned long numrels = s->sh_size/sizeof(Elf64_Rela);

		if (s->sh_type != SHT_RELA)
			continue;

		gots += count_gots(rels, numrels);
		fdescs += count_fdescs(rels, numrels);
		if (strstr(secstrings + s->sh_name, ".init"))
			init_plts += count_plts(rels, numrels);
		else
			core_plts += count_plts(rels, numrels);
	}

	mod->arch.core_plt->sh_type = SHT_NOBITS;
	mod->arch.core_plt->sh_flags = SHF_EXECINSTR | SHF_ALLOC;
	mod->arch.core_plt->sh_addralign = 16;
	mod->arch.core_plt->sh_size = core_plts * sizeof(struct plt_entry);
	mod->arch.init_plt->sh_type = SHT_NOBITS;
	mod->arch.init_plt->sh_flags = SHF_EXECINSTR | SHF_ALLOC;
	mod->arch.init_plt->sh_addralign = 16;
	mod->arch.init_plt->sh_size = init_plts * sizeof(struct plt_entry);
	mod->arch.got->sh_type = SHT_NOBITS;
	mod->arch.got->sh_flags = ARCH_SHF_SMALL | SHF_ALLOC;
	mod->arch.got->sh_addralign = 8;
	mod->arch.got->sh_size = gots * sizeof(struct got_entry);
	mod->arch.opd->sh_type = SHT_NOBITS;
	mod->arch.opd->sh_flags = SHF_ALLOC;
	mod->arch.opd->sh_addralign = 8;
	mod->arch.opd->sh_size = fdescs * sizeof(struct fdesc);
	DEBUGP("%s: core.plt=%lx, init.plt=%lx, got=%lx, fdesc=%lx\n",
	       __func__, mod->arch.core_plt->sh_size, mod->arch.init_plt->sh_size,
	       mod->arch.got->sh_size, mod->arch.opd->sh_size);
	return 0;
}

static inline int
in_init (const struct module *mod, uint64_t addr)
{
	return addr - (uint64_t) mod->module_init < mod->init_size;
}

static inline int
in_core (const struct module *mod, uint64_t addr)
{
	return addr - (uint64_t) mod->module_core < mod->core_size;
}

static inline int
is_internal (const struct module *mod, uint64_t value)
{
	return in_init(mod, value) || in_core(mod, value);
}

/*
 * Get gp-relative offset for the linkage-table entry of VALUE.
 */
static uint64_t
get_ltoff (struct module *mod, uint64_t value, int *okp)
{
	struct got_entry *got, *e;

	if (!*okp)
		return 0;

	got = (void *) mod->arch.got->sh_addr;
	for (e = got; e < got + mod->arch.next_got_entry; ++e)
		if (e->val == value)
			goto found;

	/* Not enough GOT entries? */
	BUG_ON(e >= (struct got_entry *) (mod->arch.got->sh_addr + mod->arch.got->sh_size));

	e->val = value;
	++mod->arch.next_got_entry;
  found:
	return (uint64_t) e - mod->arch.gp;
}

static inline int
gp_addressable (struct module *mod, uint64_t value)
{
	return value - mod->arch.gp + MAX_LTOFF/2 < MAX_LTOFF;
}

/* Get PC-relative PLT entry for this value.  Returns 0 on failure. */
static uint64_t
get_plt (struct module *mod, const struct insn *insn, uint64_t value, int *okp)
{
	struct plt_entry *plt, *plt_end;
	uint64_t target_ip, target_gp;

	if (!*okp)
		return 0;

	if (in_init(mod, (uint64_t) insn)) {
		plt = (void *) mod->arch.init_plt->sh_addr;
		plt_end = (void *) plt + mod->arch.init_plt->sh_size;
	} else {
		plt = (void *) mod->arch.core_plt->sh_addr;
		plt_end = (void *) plt + mod->arch.core_plt->sh_size;
	}

	/* "value" is a pointer to a function-descriptor; fetch the target ip/gp from it: */
	target_ip = ((uint64_t *) value)[0];
	target_gp = ((uint64_t *) value)[1];

	/* Look for existing PLT entry. */
	while (plt->bundle[0][0]) {
		if (plt_target(plt) == target_ip)
			goto found;
		if (++plt >= plt_end)
			BUG();
	}
	*plt = ia64_plt_template;
	if (!patch_plt(mod, plt, target_ip, target_gp)) {
		*okp = 0;
		return 0;
	}
#if ARCH_MODULE_DEBUG
	if (plt_target(plt) != target_ip) {
		printk("%s: mistargeted PLT: wanted %lx, got %lx\n",
		       __func__, target_ip, plt_target(plt));
		*okp = 0;
		return 0;
	}
#endif
  found:
	return (uint64_t) plt;
}

/* Get function descriptor for VALUE. */
static uint64_t
get_fdesc (struct module *mod, uint64_t value, int *okp)
{
	struct fdesc *fdesc = (void *) mod->arch.opd->sh_addr;

	if (!*okp)
		return 0;

	if (!value) {
		printk(KERN_ERR "%s: fdesc for zero requested!\n", mod->name);
		return 0;
	}

	if (!is_internal(mod, value))
		/*
		 * If it's not a module-local entry-point, "value" already points to a
		 * function-descriptor.
		 */
		return value;

	/* Look for existing function descriptor. */
	while (fdesc->ip) {
		if (fdesc->ip == value)
			return (uint64_t)fdesc;
		if ((uint64_t) ++fdesc >= mod->arch.opd->sh_addr + mod->arch.opd->sh_size)
			BUG();
	}

	/* Create new one */
	fdesc->ip = value;
	fdesc->gp = mod->arch.gp;
	return (uint64_t) fdesc;
}

static inline int
do_reloc (struct module *mod, uint8_t r_type, Elf64_Sym *sym, uint64_t addend,
	  Elf64_Shdr *sec, void *location)
{
	enum reloc_target_format format = (r_type >> FORMAT_SHIFT) & FORMAT_MASK;
	enum reloc_value_formula formula = (r_type >> VALUE_SHIFT) & VALUE_MASK;
	uint64_t val;
	int ok = 1;

	val = sym->st_value + addend;

	switch (formula) {
	      case RV_SEGREL:	/* segment base is arbitrarily chosen to be 0 for kernel modules */
	      case RV_DIRECT:
		break;

	      case RV_GPREL:	  val -= mod->arch.gp; break;
	      case RV_LTREL:	  val = get_ltoff(mod, val, &ok); break;
	      case RV_PLTREL:	  val = get_plt(mod, location, val, &ok); break;
	      case RV_FPTR:	  val = get_fdesc(mod, val, &ok); break;
	      case RV_SECREL:	  val -= sec->sh_addr; break;
	      case RV_LTREL_FPTR: val = get_ltoff(mod, get_fdesc(mod, val, &ok), &ok); break;

	      case RV_PCREL:
		switch (r_type) {
		      case R_IA64_PCREL21B:
			if ((in_init(mod, val) && in_core(mod, (uint64_t)location)) ||
			    (in_core(mod, val) && in_init(mod, (uint64_t)location))) {
				/*
				 * Init section may have been allocated far away from core,
				 * if the branch won't reach, then allocate a plt for it.
				 */
				uint64_t delta = ((int64_t)val - (int64_t)location) / 16;
				if (delta + (1 << 20) >= (1 << 21)) {
					val = get_fdesc(mod, val, &ok);
					val = get_p