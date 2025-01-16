 We are doing page-based allocations,
	 * so we must be aligned to a page.
	 */
	if (align < EFI_ALLOC_ALIGN)
		align = EFI_ALLOC_ALIGN;

	nr_pages = round_up(size, EFI_ALLOC_ALIGN) / EFI_PAGE_SIZE;
again:
	for (i = 0; i < map_size / desc_size; i++) {
		efi_memory_desc_t *desc;
		unsigned long m = (unsigned long)map;
		u64 start, end;

		desc = (efi_memory_desc_t *)(m + (i * desc_size));
		if (desc->type != EFI_CONVENTIONAL_MEMORY)
			continue;

		if (desc->num_pages < nr_pages)
			continue;

		start = desc->phys_addr;
		end = start + desc->num_pages * (1UL << EFI_PAGE_SHIFT);

		if (end > max)
			end = max;

		if ((start + size) > end)
			continue;

		if (round_down(end - size, align) < start)
			continue;

		start = round_down(end - size, align);

		/*
		 * Don't allocate at 0x0. It will confuse code that
		 * checks pointers against NULL.
		 */
		if (start == 0x0)
			continue;

		if (start > max_addr)
			max_addr = start;
	}

	if (!max_addr)
		status = EFI_NOT_FOUND;
	else {
		status = efi_call_early(allocate_pages,
					EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA,
					nr_pages, &max_addr);
		if (status != EFI_SUCCESS) {
			max = max_addr;
			max_addr = 0;
			goto again;
		}

		*addr = max_addr;
	}

	efi_call_early(free_pool, map);
fail:
	return status;
}

/*
 * Allocate at the lowest possible address.
 */
efi_status_t efi_low_alloc(efi_system_table_t *sys_table_arg,
			   unsigned long size, unsigned long align,
			   unsigned long *addr)
{
	unsigned long map_size, desc_size;
	efi_memory_desc_t *map;
	efi_status_t status;
	unsigned long nr_pages;
	int i;

	status = efi_get_memory_map(sys_table_arg, &map, &map_size, &desc_size,
				    NULL, NULL);
	if (status != EFI_SUCCESS)
		goto fail;

	/*
	 * Enforce minimum alignment that EFI requires when requesting
	 * a specific address.  We are doing page-based allocations,
	 * so we must be aligned to a page.
	 */
	if (align < EFI_ALLOC_ALIGN)
		align = EFI_ALLOC_ALIGN;

	nr_pages = round_up(size, EFI_ALLOC_ALIGN) / EFI_PAGE_SIZE;
	for (i = 0; i < map_size / desc_size; i++) {
		efi_memory_desc_t *desc;
		unsigned long m = (unsigned long)map;
		u64 start, end;

		desc = (efi_memory_desc_t *)(m + (i * desc_size));

		if (desc->type != EFI_CONVENTIONAL_MEMORY)
			continue;

		if (desc->num_pages < nr_pages)
			continue;

		start = desc->phys_addr;
		end = start + desc->num_pages * (1UL << EFI_PAGE_SHIFT);

		/*
		 * Don't allocate at 0x0. It will confuse code that
		 * checks pointers against NULL. Skip the first 8
		 * bytes so we start at a nice even number.
		 */
		if (start == 0x0)
			start += 8;

		start = round_up(start, align);
		if ((start + size) > end)
			continue;

		status = efi_call_early(allocate_pages,
					EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA,
					nr_pages, &start);
		if (status == EFI_SUCCESS) {
			*addr = start;
			break;
		}
	}

	if (i == map_size / desc_size)
		status = EFI_NOT_FOUND;

	efi_call_early(free_pool, map);
fail:
	return status;
}

void efi_free(efi_system_table_t *sys_table_arg, unsigned long size,
	      unsigned long addr)
{
	unsigned long nr_pages;

	if (!size)
		return;

	nr_pages = round_up(size, EFI_ALLOC_ALIGN) / EFI_PAGE_SIZE;
	efi_call_early(free_pages, addr, nr_pages);
}

/*
 * Parse the ASCII string 'cmdline' for EFI options, denoted by the efi=
 * option, e.g. efi=nochunk.
 *
 * It should be noted that efi= is parsed in two very different
 * environments, first in the early boot environment of the EFI boot
 * stub, and subsequently during the kernel boot.
 */
efi_status_t efi_parse_options(char const *cmdline)
{
	char *str;

	str = strstr(cmdline, "nokaslr");
	if (str == cmdline || (str && str > cmdline && *(str - 1) == ' '))
		__nokaslr = 1;

	/*
	 * If no EFI parameters were specified on the cmdline we've got
	 * nothing to do.
	 */
	str = strstr(cmdline, "efi=");
	if (!str)
		return EFI_SUCCESS;

	/* Skip ahead to first argument */
	str += strlen("efi=");

	/*
	 * Remember, because efi= is also used by the kernel we need to
	 * skip over arguments we don't understand.
	 */
	while (*str) {
		if (!strncmp(str, "nochunk", 7)) {
			str += strlen("nochunk");
			__chunk_size = -1UL;
		}

		/* Group words together, delimited by "," */
		while (*str && *str != ',')
			str++;

		if (*str == ',')
			str++;
	}

	return EFI_SUCCESS;
}

/*
 * Check the cmdline for a LILO-style file= arguments.
 *
 * We only support loading a file from the same filesystem as
 * the kernel image.
 */
efi_status_t handle_cmdline_files(efi_system_table_t *sys_table_arg,
				  efi_loaded_image_t *image,
				  char *cmd_line, char *option_string,
				  unsigned long max_addr,
				  unsigned long *load_addr,
				  unsigned long *load_size)
{
	struct file_info *files;
	unsigned long file_addr;
	u64 file_size_total;
	efi_file_handle_t *fh = NULL;
	efi_status_t status;
	int nr_files;
	char *str;
	int i, j, k;

	file_addr = 0;
	file_size_total = 0;

	str = cmd_line;

	j = 0;			/* See close_handles */

	if (!load_addr || !load_size)
		return EFI_INVALID_PARAMETER;

	*load_addr = 0;
	*load_size = 0;

	if (!str || !*str)
		return EFI_SUCCESS;

	for (nr_files = 0; *str; nr_files++) {
		str = strstr(str, option_string);
		if (!str)
			break;

		str += strlen(option_string);

		/* Skip any leading slashes */
		while (*str == '/' || *str == '\\')
			str++;

		while (*str && *str != ' ' && *str != '\n')
			str++;
	}

	if (!nr_files)
		return EFI_SUCCESS;

	status = efi_call_early(allocate_pool, EFI_LOADER_DATA,
				nr_files * sizeof(*files), (void **)&files);
	if (status != EFI_SUCCESS) {
		pr_efi_err(sys_table_arg, "Failed to alloc mem for file handle list\n");
		goto fail;
	}

	str = cmd_line;
	for (i = 0; i < nr_files; i++) {
		struct file_info *file;
		efi_char16_t filename_16[256];
		efi_char16_t *p;

		str = strstr(str, option_string);
		if (!str)
			break;

		str += strlen(option_string);

		file = &files[i];
		p = filename_16;

		/* Skip any leading slashes */
		while (*str == '/' || *str == '\\')
			str++;

		while (*str && *str != ' ' && *str != '\n') {
			if ((u8 *)p >= (u8 *)filename_16 + sizeof(filename_16))
				break;

			if (*str == '/') {
				*p++ = '\\';
				str++;
			} else {
				*p++ = *str++;
			}
		}

		*p = '\0';

		/* Only open the volume once. */
		if (!i) {
			status = efi_open_volume(sys_table_arg, image,
						 (void **)&fh);
			if (status != EFI_SUCCESS)
				goto free_files;
		}

		status = efi_file_size(sys_table_arg, fh, filename_16,
				       (void **)&file->handle, &file->size);
		if (status != EFI_SUCCESS)
			goto close_handles;

		file_size_total += file->size;
	}

	if (file_size_total) {
		unsigned long addr;

		/*
		 * Multiple files need to be at consecutive addresses in memory,
		 * so allocate enough memory for all the files.  This is used
		 * for loading multiple files.
		 */
		status = efi_high_alloc(sys_table_arg, file_size_total, 0x1000,
				    &file_addr, max_addr);
		if (status != EFI_SUCCESS) {
			pr_efi_err(sys_table_arg, "Failed to alloc highmem for files\n");
			goto close_handles;
		}

		/* We've run out of free low memory. */
		if (file_addr > max_addr) {
			pr_efi_err(sys_table_arg, "We've run out of free low memory\n");
			status = EFI_INVALID_PARAMETER;
			goto free_file_total;
		}

		addr = file_addr;
		for (j = 0; j < nr_files; j++) {
			unsigned long size;

			size = files[j].size;
			while (size) {
				unsigned long chunksize;
				if (size > __chunk_size)
					chunksize = __chunk_size;
				else
					chunksize = size;

				status = efi_file_read(files[j].handle,
						       &chunksize,
						       (void *)addr);
				if (status != EFI_SUCCESS) {
					pr_efi_err(sys_table_arg, "Failed to read file\n");
					goto free_file_total;
				}
				addr += chunksize;
				size -= chunksize;
			}

			efi_file_close(files[j].handle);
		}

	}

	efi_call_early(free_pool, files);

	*load_addr = file_addr;
	*load_size = file_size_total;

	return status;

free_file_total:
	efi_free(sys_table_arg, file_size_total, file_addr);

close_handles:
	for (k = j; k < i; k++)
		efi_file_close(files[k].handle);
free_files:
	efi_call_early(free_pool, files);
fail:
	*load_addr = 0;
	*load_size = 0;

	return status;
}
/*
 * Relocate a kernel image, either compressed or uncompressed.
 * In the ARM64 case, all kernel images are currently
 * uncompressed, and as such when we relocate it we need to
 * allocate additional space for the BSS segment. Any low
 * memory that this function should avoid needs to be
 * unavailable in the EFI memory map, as if the preferred
 * address is not available the lowest available address will
 * be used.
 */
efi_status_t efi_relocate_kernel(efi_system_table_t *sys_table_arg,
				 unsigned long *image_addr,
				 unsigned long image_size,
				 unsigned long alloc_size,
				 unsigned long preferred_addr,
				 unsigned long alignment)
{
	unsigned long cur_image_addr;
	unsigned long new_addr = 0;
	efi_status_t status;
	unsigned long nr_pages;
	efi_physical_addr_t efi_addr = preferred_addr;

	if (!image_addr || !image_size || !alloc_size)
		return EFI_INVALID_PARAMETER;
	if (alloc_size < image_size)
		return EFI_INVALID_PARAMETER;

	cur_image_addr = *image_addr;

	/*
	 * The EFI firmware loader could have placed the kernel image
	 * anywhere in memory, but the kernel has restrictions on the
	 * max physical address it can run at.  Some architectures
	 * also have a prefered address, so first try to relocate
	 * to the preferred address.  If that fails, allocate as low
	 * as possible while respecting the required alignment.
	 */
	nr_pages = round_up(alloc_size, EFI_ALLOC_ALIGN) / EFI_PAGE_SIZE;
	status = efi_call_early(allocate_pages,
				EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA,
				nr_pages, &efi_addr);
	new_addr = efi_addr;
	/*
	 * If preferred address allocation failed allocate as low as
	 * possible.
	 */
	if (status != EFI_SUCCESS) {
		status = efi_low_alloc(sys_table_arg, alloc_size, alignment,
				       &new_addr);
	}
	if (status != EFI_SUCCESS) {
		pr_efi_err(sys_table_arg, "Failed to allocate usable memory for kernel.\n");
		return status;
	}

	/*
	 * We know source/dest won't overlap since both memory ranges
	 * have been allocated by UEFI, so we can safely use memcpy.
	 */
	memcpy((void *)new_addr, (void *)cur_image_addr, image_size);

	/* Return the new address of the relocated image. */
	*image_addr = new_addr;

	return status;
}

/*
 * Get the number of UTF-8 bytes corresponding to an UTF-16 character.
 * This overestimates for surrogates, but that is okay.
 */
static int efi_utf8_bytes(u16 c)
{
	return 1 + (c >= 0x80) + (c >= 0x800);
}

/*
 * Convert an UTF-16 string, not necessarily null terminated, to UTF-8.
 */
static u8 *efi_utf16_to_utf8(u8 *dst, const u16 *src, int n)
{
	unsigned int c;

	while (n--) {
		c = *src++;
		if (n && c >= 0xd800 && c <= 0xdbff &&
		    *src >= 0xdc00 && *src <= 0xdfff) {
			c = 0x10000 + ((c & 0x3ff) << 10) + (*src & 0x3ff);
			src++;
			n--;
		}
		if (c >= 0xd800 && c <= 0xdfff)
			c = 0xfffd; /* Unmatched surrogate */
		if (c < 0x80) {
			*dst++ = c;
			continue;
		}
		if (c < 0x800) {
			*dst++ = 0xc0 + (c >> 6);
			goto t1;
		}
		if (c < 0x10000) {
			*dst++ = 0xe0 + (c >> 12);
			goto t2;
		}
		*dst++ = 0xf0 + (c >> 18);
		*dst++ = 0x80 + ((c >> 12) & 0x3f);
	t2:
		*dst++ = 0x80 + ((c >> 6) & 0x3f);
	t1:
		*dst++ = 0x80 + (c & 0x3f);
	}

	return dst;
}

#ifndef MAX_CMDLINE_ADDRESS
#define MAX_CMDLINE_ADDRESS	ULONG_MAX
#endif

/*
 * Convert the unicode UEFI command line to ASCII to pass to kernel.
 * Size of memory allocated return in *cmd_line_len.
 * Returns NULL on error.
 */
char *efi_convert_cmdline(efi_system_table_t *sys_table_arg,
			  efi_loaded_image_t *image,
			  int *cmd_line_len)
{
	const u16 *s2;
	u8 *s1 = NULL;
	unsigned long cmdline_addr = 0;
	int load_o