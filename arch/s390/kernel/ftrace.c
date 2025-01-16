 INST_FLAG_FIX_RELATIVE_IP_ADDR;
			p->ainsn.inst_flag |= INST_FLAG_FIX_BRANCH_REG;
			p->ainsn.target_br_reg = ((kprobe_inst >> 6) & 0x7);
			break;
		}
	} else if (bundle_encoding[template][slot] == X) {
		switch (major_opcode) {
		  case LONG_CALL_OPCODE:
			p->ainsn.inst_flag |= INST_FLAG_FIX_BRANCH_REG;
			p->ainsn.target_br_reg = ((kprobe_inst >> 6) & 0x7);
		  break;
		}
	}
	return;
}

/*
 * In this function we check to see if the instruction
 * (qp) cmpx.crel.ctype p1,p2=r2,r3
 * on which we are inserting kprobe is cmp instruction
 * with ctype as unc.
 */
static uint __kprobes is_cmp_ctype_unc_inst(uint template, uint slot,
					    uint major_opcode,
					    unsigned long kprobe_inst)
{
	cmp_inst_t cmp_inst;
	uint ctype_unc = 0;

	if (!((bundle_encoding[template][slot] == I) ||
		(bundle_encoding[template][slot] == M)))
		goto out;

	if (!((major_opcode == 0xC) || (major_opcode == 0xD) ||
		(major_opcode == 0xE)))
		goto out;

	cmp_inst.l = kprobe_inst;
	if ((cmp_inst.f.x2 == 0) || (cmp_inst.f.x2 == 1)) {
		/* Integer compare - Register Register (A6 type)*/
		if ((cmp_inst.f.tb == 0) && (cmp_inst.f.ta == 0)
				&&(cmp_inst.f.c == 1))
			ctype_unc = 1;
	} else if ((cmp_inst.f.x2 == 2)||(cmp_inst.f.x2 == 3)) {
		/* Integer compare - Immediate Register (A8 type)*/
		if ((cmp_inst.f.ta == 0) &&(cmp_inst.f.c == 1))
			ctype_unc = 1;
	}
out:
	return ctype_unc;
}

/*
 * In this function we check to see if the instruction
 * on which we are inserting kprobe is supported.
 * Returns qp value if supported
 * Returns -EINVAL if unsupported
 */
static int __kprobes unsupported_inst(uint template, uint  slot,
				      uint major_opcode,
				      unsigned long kprobe_inst,
				      unsigned long addr)
{
	int qp;

	qp = kprobe_inst & 0x3f;
	if (is_cmp_ctype_unc_inst(template, slot, major_opcode, kprobe_inst)) {
		if (slot == 1 && qp)  {
			printk(KERN_WARNING "Kprobes on cmp unc "
					"instruction on slot 1 at <0x%lx> "
					"is not supported\n", addr);
			return -EINVAL;

		}
		qp = 0;
	}
	else if (bundle_encoding[template][slot] == I) {
		if (major_opcode == 0) {
			/*
			 * Check for Integer speculation instruction
			 * - Bit 33-35 to be equal to 0x1
			 */
			if (((kprobe_inst >> 33) & 0x7) == 1) {
				printk(KERN_WARNING
					"Kprobes on speculation inst at <0x%lx> not supported\n",
						addr);
				return -EINVAL;
			}
			/*
			 * IP relative mov instruction
			 *  - Bit 27-35 to be equal to 0x30
			 */
			if (((kprobe_inst >> 27) & 0x1FF) == 0x30) {
				printk(KERN_WARNING
					"Kprobes on \"mov r1=ip\" at <0x%lx> not supported\n",
						addr);
				return -EINVAL;

			}
		}
		else if ((major_opcode == 5) &&	!(kprobe_inst & (0xFUl << 33)) &&
				(kprobe_inst & (0x1UL << 12))) {
			/* test bit instructions, tbit,tnat,tf
			 * bit 33-36 to be equal to 0
			 * bit 12 to be equal to 1
			 */
			if (slot == 1 && qp) {
				printk(KERN_WARNING "Kprobes on test bit "
						"instruction on slot at <0x%lx> "
						"is not supported\n", addr);
				return -EINVAL;
			}
			qp = 0;
		}
	}
	else if (bundle_encoding[template][slot] == B) {
		if (major_opcode == 7) {
			/* IP-Relative Predict major code is 7 */
			printk(KERN_WARNING "Kprobes on IP-Relative"
					"Predict is not supported\n");
			return -EINVAL;
		}
		else if (major_opcode == 2) {
			/* Indirect Predict, major code is 2
			 * bit 27-32 to be equal to 10 or 11
			 */
			int x6=(kprobe_inst >> 27) & 0x3F;
			if ((x6 == 0x10) || (x6 == 0x11)) {
				printk(KERN_WARNING "Kprobes on "
					"Indirect Predict is not supported\n");
				return -EINVAL;
			}
		}
	}
	/* kernel does not use float instruction, here for safety kprobe
	 * will judge whether it is fcmp/flass/float approximation instruction
	 */
	else if (unlikely(bundle_encoding[template][slot] == F)) {
		if ((major_opcode == 4 || major_opcode == 5) &&
				(kprobe_inst  & (0x1 << 12))) {
			/* fcmp/fclass unc instruction */
			if (slot == 1 && qp) {
				printk(KERN_WARNING "Kprobes on fcmp/fclass "
					"instruction on slot at <0x%lx> "
					"is not supported\n", addr);
				return -EINVAL;

			}
			qp = 0;
		}
		if ((major_opcode == 0 || major_opcode == 1) &&
			(kprobe_inst & (0x1UL << 33))) {
			/* float Approximation instruction */
			if (slot == 1 && qp) {
				printk(KERN_WARNING "Kprobes on float Approx "
					"instr at <0x%lx> is not supported\n",
						addr);
				return -EINVAL;
			}
			qp = 0;
		}
	}
	return qp;
}

/*
 * In this function we override the bundle with
 * the break instruction at the given slot.
 */
static void __kprobes prepare_break_inst(uint template, uint  slot,
					 uint major_opcode,
					 unsigned long kprobe_inst,
					 struct kprobe *p,
					 int qp)
{
	unsigned long break_inst = BREAK_INST;
	bundle_t *bundle = &p->opcode.bundle;

	/*
	 * Copy the original kprobe_inst qualifying predicate(qp)
	 * to the break instruction
	 */
	break_inst |= qp;

	switch (slot) {
	  case 0:
		bundle->quad0.slot0 = break_inst;
		break;
	  case 1:
		bundle->quad0.slot1_p0 = break_inst;
		bundle->quad1.slot1_p1 = break_inst >> (64-46);
		break;
	  case 2:
		bundle->quad1.slot2 = break_inst;
		break;
	}

	/*
	 * Update the instruction flag, so that we can
	 * emulate the instruction properly after we
	 * single step on original instruction
	 */
	update_kprobe_inst_flag(template, slot, major_opcode, kprobe_inst, p);
}

static void __kprobes get_kprobe_inst(bundle_t *bundle, uint slot,
	       	unsigned long *kprobe_inst, uint *major_opcode)
{
	unsigned long kprobe_inst_p0, kprobe_inst_p1;
	unsigned int template;

	template = bundle->quad0.template;

	switch (slot) {
	  case 0:
		*major_opcode = (bundle->quad0.slot0 >> SLOT0_OPCODE_SHIFT);
		*kprobe_inst = bundle->quad0.slot0;
		  break;
	  case 1:
		*major_opcode = (bundle->quad1.slot1_p1 >> SLOT1_p1_OPCODE_SHIFT);
		kprobe_inst_p0 = bundle->quad0.slot1_p0;
		kprobe_inst_p1 = bundle->quad1.slot1_p1;
		*kprobe_inst = kprobe_inst_p0 | (kprobe_inst_p1 << (64-46));
		break;
	  case 2:
		*major_opcode = (bundle->quad1.slot2 >> SLOT2_OPCODE_SHIFT);
		*kprobe_inst = bundle->quad1.slot2;
		break;
	}
}

/* Returns non-zero if the addr is in the Interrupt Vector Table */
static int __kprobes in_ivt_functions(unsigned long addr)
{
	return (addr >= (unsigned long)__start_ivt_text
		&& addr < (unsigned long)__end_ivt_text);
}

static int __kprobes valid_kprobe_addr(int template, int slot,
				       unsigned long addr)
{
	if ((slot > 2) || ((bundle_encoding[template][1] == L) && slot > 1)) {
		printk(KERN_WARNING "Attempting to insert unaligned kprobe "
				"at 0x%lx\n", addr);
		return -EINVAL;
	}

	if (in_ivt_functions(addr)) {
		printk(KERN_WARNING "Kprobes can't be inserted inside "
				"IVT functions at 0x%lx\n", addr);
		return -EINVAL;
	}

	return 0;
}

static void __kprobes save_previous_kprobe(struct kprobe_ctlblk *kcb)
{
	unsigned int i;
	i = atomic_add_return(1, &kcb->prev_kprobe_index);
	kcb->prev_kprobe[i-1].kp = kprobe_running();
	kcb->prev_kprobe[i-1].status = kcb->kprobe_status;
}

static void __kprobes restore_previous_kprobe(struct kprobe_ctlblk *kcb)
{
	unsigned int i;
	i = atomic_read(&kcb->prev_kprobe_index);
	__this_cpu_write(current_kprobe, kcb->prev_kprobe[i-1].k