{
			const char *unit;
			unsigned long size;
			char buf[64];

			md = p;
			size = md->num_pages << EFI_PAGE_SHIFT;

			if ((size >> 40) > 0) {
				size >>= 40;
				unit = "TB";
			} else if ((size >> 30) > 0) {
				size >>= 30;
				unit = "GB";
			} else if ((size >> 20) > 0) {
				size >>= 20;
				unit = "MB";
			} else {
				size >>= 10;
				unit = "KB";
			}

			printk("mem%02d: %s "
			       "range=[0x%016lx-0x%016lx) (%4lu%s)\n",
			       i, efi_md_typeattr_format(buf, sizeof(buf), md),
			       md->phys_addr,
			       md->phys_addr + efi_md_size(md), size, unit);
		}
	}
#endif

	efi_map_pal_code();
	efi_enter_virtual_mode();
}

void
efi_enter_virtual_mode (void)
{
	void *efi_map_start, *efi_map_end, *p;
	efi_memory_desc_t *md;
	efi_status_t status;
	u64 efi_desc_size;

	efi_map_start = __va(ia64_boot_param->efi_memmap);
	efi_map_end   = efi_map_start + ia64_boot_param->efi_memmap_size;
	efi_desc_size = ia64_boot_param->efi_memdesc_size;

	for (p = efi_map_start; p < efi_map_end; p += efi_desc_size) {
		md = p;
		if (md->attribute & EFI_MEMORY_RUNTIME) {
			/*
			 * Some descriptors have multiple bits set, so the
			 * order of the tests is relevant.
			 */
			if (md->attribute & EFI_MEMORY_WB) {
				md->virt_addr = (u64) __va(md->phys_addr);
			} else if (md->attribute & EFI_MEMORY_UC) {
				md->virt_addr = (u64) ioremap(md->phys_addr, 0);
			} else if (md->attribute & EFI_MEMORY_WC) {
#if 0
				md->virt_addr = ia64_remap(md->phys_addr,
							   (_PAGE_A |
							    _PAGE_P |
							    _PAGE_D |
							    _PAGE_MA_WC |
							    _PAGE_PL_0 |
							    _PAGE_AR_RW));
#else
				printk(KERN_INFO "EFI_MEMORY_WC mapping\n");
				md->virt_addr = (u64) ioremap(md->phys_addr, 0);
#endif
			} else if (md->attribute & EFI_MEMORY_WT) {
#if 0
				md->virt_addr = ia64_remap(md->phys_addr,
							   (_PAGE_A |
							    _PAGE_P |
							    _PAGE_D |
							    _PAGE_MA_WT |
							    _PAGE_PL_0 |
							    _PAGE_AR_RW));
#else
				printk(KERN_INFO "EFI_MEMORY_WT mapping\n");
				md->virt_addr = (u64) ioremap(md->phys_addr, 0);
#endif
			}
		}
	}

	status = efi_call_phys(__va(runtime->set_virtual_address_map),
			       ia64_boot_param->efi_memmap_size,
			       efi_desc_size,
			       ia64_boot_param->efi_memdesc_version,
			       ia64_boot_param->efi_memmap);
	if (status != EFI_SUCCESS) {
		printk(KERN_WARNING "warning: unable to switch EFI into "
		       "virtual mode (status=%lu)\n", status);
		return;
	}

	set_bit(EFI_RUNTIME_SERVICES, &efi.flags);

	/*
	 * Now that EFI is in virtual mode, we call the EFI functions more
	 * efficiently:
	 */
	efi.get_time = virt_get_time;
	efi.set_time = virt_set_time;
	efi.get_wakeup_time = virt_get_wakeup_time;
	efi.set_wakeup_time = virt_set_wakeup_time;
	efi.get_variable = virt_get_variable;
	efi.get_next_variable = virt_get_next_variable;
	efi.set_variable = virt_set_variable;
	efi.get_next_high_mono_count = virt_get_next_high_mono_count;
	efi.reset_system = virt_reset_system;
}

/*
 * Walk the EFI memory map looking for the I/O port range.  There can only be
 * one entry of this type, other I/O port ranges should be described via ACPI.
 */
u64
efi_get_iobase (void)
{
	void *efi_map_start, *efi_map_end, *p;
	efi_memory_desc_t *md;
	u64 efi_desc_size;

	efi_map_start = __va(ia64_boot_param->efi_memmap);
	efi_map_end   = efi_map_start + ia64_boot_param->efi_memmap_size;
	efi_desc_size = ia64_boot_param->efi_memdesc_size;

	for (p = efi_map_start; p < efi_map_end; p += efi_desc_size) {
		md = p;
		if (md->type == EFI_MEMORY_MAPPED_IO_PORT_SPACE) {
			if (md->attribute & EFI_MEMORY_UC)
				return md->phys_addr;
		}
	}
	return 0;
}

static struct kern_memdesc *
kern_memory_descriptor (unsigned long phys_addr)
{
	struct kern_memdesc *md;

	for (md = kern_memmap; md->start != ~0UL; md++) {
		if (phys_addr - md->start < (md->num_pages << EFI_PAGE_SHIFT))
			 return md;
	}
	return NULL;
}

static efi_memory_desc_t *
efi_memory_descriptor (unsigned long phys_addr)
{
	void *efi_map_start, *efi_map_end, *p;
	efi_memory_desc_t *md;
	u64 efi_desc_size;

	efi_map_start = __va(ia64_boot_param->efi_memmap);
	efi_map_end   = efi_map_start + ia64_boot_param->efi_memmap_size;
	efi_desc_size = ia64_boot_param->efi_memdesc_size;

	for (p = efi_map_start; p < efi_map_end; p += efi_desc_size) {
		md = p;

		if (phys_addr - md->phys_addr < efi_md_size(md))
			 return md;
	}
	return NULL;
}

static int
efi_memmap_intersects (unsigned long phys_addr, unsigned long size)
{
	void *efi_map_start, *efi_map_end, *p;
	efi_memory_desc_t *md;
	u64 efi_desc_size;
	unsigned long end;

	efi_map_start = __va(ia64_boot_param->efi_memmap);
	efi_map_end   = efi_map_start + ia64_boot_param->efi_memmap_size;
	efi_desc_size = ia64_boot_param->efi_memdesc_size;

	end = phys_addr + size;

	for (p = efi_map_start; p < efi_map_end; p += efi_desc_size) {
		md = p;
		if (md->phys_addr < end && efi_md_end(md) > phys_addr)
			return 1;
	}
	return 0;
}

u32
efi_mem_type (unsigned long phys_addr)
{
	efi_memory_desc_t *md = efi_memory_descriptor(phys_addr);

	if (md)
		return md->type;
	return 0;
}

u64
efi_mem_attributes (unsigned long phys_addr)
{
	efi_memory_desc_t *md = efi_memory_descriptor(phys_addr);

	if (md)
		return md->attribute;
	return 0;
}
EXPORT_SYMBOL(efi_mem_attributes);

u64
efi_mem_attribute (unsigned long phys_addr, unsigned long size)
{
	unsigned long end = phys_addr + size;
	efi_memory_desc_t *md = efi_memory_descriptor(phys_addr);
	u64 attr;

	if (!md)
		return 0;

	/*
	 * EFI_MEMORY_RUNTIME is not a memory attribute; it just tells
	 * the kernel that firmware needs this region mapped.
	 */
	attr = md->attribute & ~EFI_MEMORY_RUNTIME;
	do {
		unsigned long md_end = efi_md_end(md);

		if (end <= md_end)
			return attr;

		md = efi_memory_descriptor(md_end);
		if (!md || (md->attribute & ~EFI_MEMORY_RUNTIME) != attr)
			return 0;
	} while (md);
	return 0;	/* never reached */
}

u64
kern_mem_attribute (unsigned long phys_addr, unsigned long size)
{
	unsigned long end = phys_addr + size;
	struct kern_memdesc *md;
	u64 attr;

	/*
	 * This is a hack for ioremap calls before we set up kern_memmap.
	 * Maybe we should do efi_memmap_init() earlier instead.
	 */
	if (!kern_memmap) {
		attr = efi_mem_attribute(phys_addr, size);
		if (attr & EFI_MEMORY_WB)
			return EFI_MEMORY_WB;
		return 0;
	}

	md = kern_memory_descriptor(phys_addr);
	if (!md)
		return 0;

	attr = md->attribute;
	do {
		unsigned long md_end = kmd_end(md);

		if (end <= md_end)
			return attr;

		md = kern_memory_descriptor(md_end);
		if (!md || md->attribute != attr)
			return 0;
	} while (md);
	return 0;	/* never reached */
}
EXPORT_SYMBOL(kern_mem_attribute);

int
valid_phys_addr_range (phys_addr_t phys_addr, unsigned long size)
{
	u64 attr;

	/*
	 * /dev/mem reads and writes use copy_to_user(), which implicitly
	 * uses a granule-sized kernel identity mapping.  It's really
	 * only safe to do this for regions in kern_memmap.  For more
	 * details, see Documentation/ia64/aliasing.txt.
	 */
	attr = kern_mem_attribute(phys_addr, size);
	if (attr & EFI_MEMORY_WB || attr & EFI_MEMORY_UC)
		return 1;
	return 0;
}

int
valid_mmap_phys_addr_range (unsigned long pfn, unsigned long size)
{
	unsigned long phys_addr = pfn << PAGE_SHIFT;
	u64 attr;

	attr = efi_mem_attribute(phys_addr, size);

	/*
	 * /dev/mem mmap uses normal user pages, so we don't need the entire
	 * granule, but the entire region we're mapping must support the same
	 * attribute.
	 */
	if (attr & EFI_MEMORY_WB || attr & EFI_MEMORY_UC)
		return 1;

	/*
	 * Intel firmware doesn't tell us about all the MMIO regions, so
	 * in general we have to allow mmap requests.  But if EFI *does*
	 * tell us about anything inside this region, we should deny it.
	 * The user can always map a smaller region to avoid the overlap.
	 */
	if (efi_memmap_intersects(phys_addr, size))
		return 0;

	return 1;
}

pgprot_t
phys_mem_access_prot(struct file *file, unsigned long pfn, unsigned long size,
		     pgprot_t vma_prot)
{
	unsigned long phys_addr = pfn << PAGE_SHIFT;
	u64 attr;

	/*
	 * For /dev/mem mmap, we use user mappings, but if the region is
	 * in kern_memmap (and hence may be covered by a kernel mapping),
	 * we must use the same attribute as the kernel mapping.
	 */
	attr = kern_mem_attribute(phys_addr, size);
	if (attr & EFI_MEMORY_WB)
		return pgprot_cacheable(vma_prot);
	else if (attr & EFI_MEMORY_UC)
		return pgprot_noncached(vma_prot);

	/*
	 * Some chipsets don't support UC access to memory.  If
	 * WB is supported, we prefer that.
	 */
	if (efi_mem_attribute(phys_addr, size) & EFI_MEMORY_WB)
		return pgprot_cacheable(vma_prot);

	return pgprot_noncached(vma_prot);
}

int __init
efi_uart_console_only(void)
{
	efi_status_t status;
	char *s, name[] = "ConOut";
	efi_guid_t guid = EFI_GLOBAL_VARIABLE_GUID;
	efi_char16_t *utf16, name_utf16[32];
	unsigned char data[1024];
	unsigned long size = sizeof(data);
	struct efi_generic_dev_path *hdr, *end_addr;
	int uart = 0;

	/* Convert to UTF-16 */
	utf16 = name_utf16;
	s = name;
	while (*s)
		*utf16++ = *s++ & 0x7f;
	*utf16 = 0;

	status = efi.get_variable(name_utf16, &guid, NULL, &size, data);
	if (status != EFI_SUCCESS) {
		printk(KERN_ERR "No EFI %s variable?\n", name);
		return 0;
	}

	hdr = (struct efi_generic_dev_path *) data;
	end_addr = (struct efi_generic_dev_path *) ((u8 *) data + size);
	while (hdr < end_addr) {
		if (hdr->type == EFI_DEV_MSG &&
		    hdr->sub_type == EFI_DEV_MSG_UART)
			uart = 1;
		else if (hdr->type == EFI_DEV_END_PATH ||
			  hdr->type == EFI_DEV_END_PATH2) {
			if (!uart)
				return 0;
			if (hdr->sub_type == EFI_DEV_END_ENTIRE)
				return 1;
			uart = 0;
		}
		hdr = (struct efi_generic_dev_path *)((u8 *) hdr + hdr->length);
	}
	printk(KERN_ERR "Malformed %s value\n", name);
	return 0;
}

/*
 * Look for the first granule aligned memory descriptor memory
 * that is big enough to hold EFI memory map. Make sure this
 * descriptor is atleast granule sized so it does not get trimmed
 */
struct kern_memdesc *
find_memmap_space (void)
{
	u64	contig_low=0, contig_high=0;
	u64	as = 0, ae;
	void *efi_map_start, *efi_map_end, *p, *q;
	efi_memory_desc_t *md, *pmd = NULL, *check_md;
	u64	space_needed, efi_desc_size;
	unsigned long total_mem = 0;

	efi_map_start = __va(ia64_boot_param->efi_memmap);
	efi_map_end   = efi_map_start + ia64_boot_param->efi_memmap_size;
	efi_desc_size = ia64_boot_param->efi_memdesc_size;

	/*
	 * Worst case: we need 3 kernel descriptors for each efi descriptor
	 * (if every entry has a WB part in the middle, and UC head and tail),
	 * plus one for the end marker.
	 */
	space_needed = sizeof(kern_memdesc_t) *
		(3 * (ia64_boot_param->efi_memmap_size/efi_desc_size) + 1);

	for (p = efi_map_start; p < efi_map_end; pmd = md, p += efi_desc_size) {
		md = p;
		if (!efi_wb(md)) {
			continue;
		}
		if (pmd == NULL || !efi_wb(pmd) ||
		    efi_md_end(pmd) != md->phys_addr) {
			contig_low = GRANULEROUNDUP(md->phys_addr);
			contig_high = efi_md_end(md);
			for (q = p + efi_desc_size; q < efi_map_end;
			     q += efi_desc_size) {
				check_md = q;
				if (!efi_wb(check_md))
					break;
				if (contig_high != check_md->phys_addr)
					break;
				contig_high = efi_md_end(check_md);
			}
			contig_high = GRANULEROUNDDOWN(contig_high);
		}
		if (!is_memory_available(md) || md->type == EFI_LOADER_DATA)
			continue;

		/* Round ends inward to granule boundaries */
		as = max(contig_low, md->phys_addr);
		ae = min(contig_high, efi_md_end(md));

		/* keep within max_addr= and min_addr= command line arg */
		as = max(as, min_addr);
		ae = min(ae, max_addr);
		if (ae <= as)
			continue;

		/* avoid going over mem= command line arg */
		if (total_mem + (ae - as) > mem_limit)
			ae -= total_mem + (ae - as) - mem_limit;

		if (ae <= as)
			continue;

		if (ae - as > space_needed)
			break;
	}
	if (p >= efi_map_end)
		panic("Can't allocate space for kernel memory descriptors");

	return __va(as);
}

/*
 * Walk the EFI memory map and gather all memory available for kernel
 * to use.  We can allocate partial granules only if the unavailable
 * parts exist, and are WB.
 */
unsigned long
efi_memmap_init(u64 *s, u64 *e)
{
	struct kern_memdesc *k, *prev = NULL;
	u64	contig_low=0, contig_high=0;
	u64	as, ae, lim;
	void *efi_map_start, *efi_map_end, *p, *q;
	efi_memory_desc_t *md, *pmd = NULL, *check_md;
	u64	efi_desc_size;
	unsigned long total_mem = 0;

	k = kern_memmap = find_memmap_space();

	efi_map_start = __va(ia64_boot_param->efi_memmap);
	efi_map_end   = efi_map_start + ia64_boot_param->efi_memmap_size;
	efi_desc_size = ia64_boot_param->efi_memdesc_size;

	for (p = efi_map_start; p < efi_map_end; pmd = md, p += efi_desc_size) {
		md = p;
		if (!efi_wb(md)) {
			if (efi_uc(md) &&
			    (md->type == EFI_CONVENTIONAL_MEMORY ||
			     md->type == EFI_BOOT_SERVICES_DATA)) {
				k->attribute = EFI_MEMORY_UC;
				k->start = md->phys_addr;
				k->num_pages = md->num_pages;
				k++;
			}
			continue;
		}
		if (pmd == NULL || !efi_wb(pmd) ||
		    efi_md_end(pmd) != md->phys_addr) {
			contig_low = GRANULEROUNDUP(md->phys_addr);
			contig_high = efi_md_end(md);
			for (q = p + efi_desc_size; q < efi_map_end;
			     q += efi_desc_size) {
				check_md = q;
				if (!efi_wb(check_md))
					break;
				if (contig_high != check_md->phys_addr)
					break;
				contig_high = efi_md_end(check_md);
			}
			contig_high = GRANULEROUNDDOWN(contig_high);
		}
		if (!is_memory_available(md))
			continue;

		/*
		 * Round ends inward to granule boundaries
		 * Give trimmings to uncached allocator
		 */
		if (md->phys_addr < contig_low) {
			lim = min(efi_md_end(md), contig_low);
			if (efi_uc(md)) {
				if (k > kern_memmap &&
				    (k-1)->attribute == EFI_MEMORY_UC &&
				    kmd_end(k-1) == md->phys_addr) {
					(k-1)->num