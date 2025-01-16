    md->phys_addr + efi_md_size(md),
                       vaddr & mask, (vaddr & mask) + IA64_GRANULE_SIZE);
#endif
		return __va(md->phys_addr);
	}
	printk(KERN_WARNING "%s: no PAL-code memory-descriptor found\n",
	       __func__);
	return NULL;
}


static u8 __init palo_checksum(u8 *buffer, u32 length)
{
	u8 sum = 0;
	u8 *end = buffer + length;

	while (buffer < end)
		sum = (u8) (sum + *(buffer++));

	return sum;
}

/*
 * Parse and handle PALO table which is published at:
 * http://www.dig64.org/home/DIG64_PALO_R1_0.pdf
 */
static void __init handle_palo(unsigned long phys_addr)
{
	struct palo_table *palo = __va(phys_addr);
	u8  checksum;

	if (strncmp(palo->signature, PALO_SIG, sizeof(PALO_SIG) - 1)) {
		printk(KERN_INFO "PALO signature incorrect.\n");
		return;
	}

	checksum = palo_checksum((u8 *)palo, palo->length);
	if (checksum) {
		printk(KERN_INFO "PALO checksum incorrect.\n");
		return;
	}

	setup_ptcg_sem(palo->max_tlb_purges, NPTCG_FROM_PALO);
}

void
efi_map_pal_code (void)
{
	void *pal_vaddr = efi_get_pal_addr ();
	u64 psr;

	if (!pal_vaddr)
		return;

	/*
	 * Cannot write to CRx with PSR.ic=1
	 */
	psr = ia64_clear_ic();
	ia64_itr(0x1, IA64_TR_PALCODE,
		 GRANULEROUNDDOWN((unsigned long) pal_vaddr),
		 pte_val(pfn_pte(__pa(pal_vaddr) >> PAGE_SHIFT, PAGE_KERNEL)),
		 IA64_GRANULE_SHIFT);
	ia64_set_psr(psr);		/* restore psr */
}

void __init
efi_init (void)
{
	void *efi_map_start, *efi_map_end;
	efi_char16_t *c16;
	u64 efi_desc_size;
	char *cp, vendor[100] = "unknown";
	int i;

	set_bit(EFI_BOOT, &efi.flags);
	set_bit(EFI_64BIT, &efi.flags);

	/*
	 * It's too early to be able to use the standard kernel command line
	 * support...
	 */
	for (cp = boot_command_line; *cp; ) {
		if (memcmp(cp, "mem=", 4) == 0) {
			mem_limit = memparse(cp + 4, &cp);
		} else if (memcmp(cp, "max_addr=", 9) == 0) {
			max_addr = GRANULEROUNDDOWN(memparse(cp + 9, &cp));
		} else if (memcmp(cp, "min_addr=", 9) == 0) {
			min_addr = GRANULEROUNDDOWN(memparse(cp + 9, &cp));
		} else {
			while (*cp != ' ' && *cp)
				++cp;
			while (*cp == ' ')
				++cp;
		}
	}
	if (min_addr != 0UL)
		printk(KERN_INFO "Ignoring memory below %lluMB\n",
		       min_addr >> 20);
	if (max_addr != ~0UL)
		printk(KERN_INFO "Ignoring memory above %lluMB\n",
		       max_addr >> 20);

	efi.systab = __va(ia64_boot_param->efi_systab);

	/*
	 * Verify the EFI Table
	 */
	if (efi.systab == NULL)
		panic("Whoa! Can't find EFI system table.\n");
	if (efi.systab->hdr.signature != EFI_SYSTEM_TABLE_SIGNATURE)
		panic("Whoa! EFI system table signature incorrect\n");
	if ((efi.systab->hdr.revision >> 16) == 0)
		printk(KERN_WARNING "Warning: EFI system table version "
		       "%d.%02d, expected 1.00 or greater\n",
		       efi.systab->hdr.revision >> 16,
		       efi.systab->hdr.revision & 0xffff);

	/* Show what we know for posterity */
	c16 = __va(efi.systab->fw_vendor);
	if (c16) {
		for (i = 0;i < (