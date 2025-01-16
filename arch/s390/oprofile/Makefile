 *)addr);
		if (other_kp)
			other_kp->ainsn.inst_flag &= ~INST_FLAG_BOOSTABLE;
	}
}

int __kprobes arch_prepare_kprobe(struct kprobe *p)
{
	unsigned long addr = (unsigned long) p->addr;
	unsigned long *kprobe_addr = (unsigned long *)(addr & ~0xFULL);
	unsigned long kprobe_inst=0;
	unsigned int