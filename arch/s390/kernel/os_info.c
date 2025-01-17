name = "System RAM";
				break;

			case EFI_ACPI_MEMORY_NVS:
				name = "ACPI Non-volatile Storage";
				break;

			case EFI_UNUSABLE_MEMORY:
				name = "reserved";
				flags |= IORESOURCE_DISABLED;
				break;

			case EFI_PERSISTENT_MEMORY:
				name = "Persistent Memory";
				break;

			case EFI_RESERVED_TYPE:
			case EFI_RUNTIME_SERVICES_CODE:
			case EFI_RUNTIME_SERVICES_DATA:
			case EFI_ACPI_RECLAIM_MEMORY:
			default:
				name = "reserved";
				break;
		}

		if ((res = kzalloc(sizeof(struct resource),
				   GFP_KERNEL)) == NULL) {
			printk(KERN_ERR
			       "failed to allocate resource for iomem\n");
			return;
		}

		res->name = name;
		res->start = md->phys_addr;
		res->end = md->phys_addr + efi_md_size(md) - 1;
		res->flags = flags;

		if (insert_resource(&iomem_resource, res) < 0)
			kfree(res);
		else {
			/*
			 * We don't know which region contains
			 * kernel data so we try it repeatedly and
			 * let the resource manager test it.
			 */
			insert_resource(res, code_resource);
			insert_resource(res, data_resource);
			insert_resource(res, bss_resource);
#ifdef CONFIG_KEXEC
                        insert_resource(res, &efi_memmap_res);
                        insert_resource(res, &boot_param_res);
			if (crashk_res.end > crashk_res.start)
				insert_resource(res, &crashk_res);
#endif
		}
	}
}

#ifdef CONFIG_KEXEC
/* find a block of memory aligned to 64M exclude reserved regions
   rsvd_regions are sorted
 */
unsigned long __init
kdump_find_rsvd_region (unsigned long size, struct rsvd_region *r, int n)
{
	int i;
	u64 start, end;
	u64 alignment = 1UL << _PAGE_SIZE_64M;
	void *efi_map_start, *efi_map_end, *p;
	efi_memory_desc_t *md;
	u64 efi_desc_size;

	efi_map_start = __va(ia64_boot_param->efi_memmap);
	efi_map_end   = efi_map_start + ia64_boot_param->efi_memmap_size;
	efi_desc_size = ia64_boot_param->efi_memdesc_size;

	for (p = efi_map_start; p < efi_map_end; p += efi_desc_size) {
		md = p;
		if (!efi_wb(md))
			continue;
		start = ALIGN(md->phys_addr, alignment);
		end = efi_md_end(md);
		for (i = 0; i < n; i++) {
			if (__pa(r[i].start) >= start && __pa(r[i].end) < end) {
				if (__pa(r[i].start) > start + size)
					return start;
				start = ALIGN(__pa(r[i].end), alignment);
				if (i < n-1 &&
				    __pa(r[i+1].start) < start + size)
					continue;
				else
					break;
			}
		}
		if (end > start + size)
			return start;
	}

	printk(KERN_WARNING
	       "Cannot reserve 0x%lx byte of memory for crashdump\n", size);
	return ~0UL;
}
#endif

#ifdef CONFIG_CRASH_DUMP
/* locate the size find a the descriptor at a certain address */
unsigned long __init
vmcore_find_descriptor_size (unsigned long address)
{
	void *efi_map_start, *efi_map_end, *p;
	efi_memory_desc_t *md;
	u64 efi_desc_size;
	unsigned long ret = 0;

	efi_map_start = __va(ia64_boot_param->efi_memmap);
	efi_map_end   = efi_map_start + ia64_boot_param->efi_memmap_size;
	efi_desc_size = ia64_boot_param->efi_memdesc_size;

	for (p = efi_map_start; p < efi_map_end; p += efi_desc_size) {
		md = p;
		if (efi_wb(md) && md->type == EFI_LOADER_DATA
		    && md->phys_addr == address) {
			ret = efi_md_size(md);
			break;
		}
	}

	if (ret == 0)
		printk(KERN_WARNING "Cannot locate EFI vmcore descriptor\n");

	return ret;
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             