g *reserve_size,
				 unsigned long dram_base,
				 efi_loaded_image_t *image);
/*
 * EFI entry point for the arm/arm64 EFI stubs.  This is the entrypoint
 * that is described in the PE/COFF header.  Most of the code is the same
 * for both archictectures, with the arch-specific code provided in the
 * handle_kernel_image() function.
 */
unsigned long efi_entry(void *handle, efi_system_table_t *sys_table,
			       unsigned long *image_addr)
{
	efi_loaded_image_t *image;
	efi_status_t status;
	unsigned long image_size = 0;
	unsigned long dram_base;
	/* addr/point and size pairs for memory management*/
	unsigned long initrd_addr;
	u64 initrd_size = 0;
	unsigned long fdt_addr = 0;  /* Original DTB */
	unsigned long fdt_size = 0;
	char *cmdline_ptr = NULL;
	int cmdline_size = 0;
	unsigned long new_fdt_addr;
	efi_guid_t loaded_image_proto = LOADED_IMAGE_PROTOCOL_GUID;
	unsigned long reserve_addr = 0;
	unsigned long reserve_size = 0;

	/* Check if we were booted by the EFI firmware */
	if (sys_table->hdr.signature != EFI_SYSTEM_TABLE_SIGNATURE)
		goto fail;

	pr_efi(sys_table, "Booting Linux Kernel...\n");

	/*
	 * Get a handle to the loaded image protocol.  This is used to get
	 * information about the running image, such as size and the command
	 * line.
	 */
	status = sys_table->boottime->handle_protocol(handle,
					&loaded_image_proto, (void *)&image);
	if (status != EFI_SUCCESS) {
		pr_efi_err(sys_table, "Failed to get loaded image protocol\n");
		goto fail;
	}

	dram_base = get_dram_base(sys_table);
	if (dram_base == EFI_ERROR) {
		pr_efi_err(sys_table, "Failed to find DRAM base\n");
		goto fail;
	}

	/*
	 * Get the command line from EFI, using the LOADED_IMAGE
	 * protocol. We are going to copy the command line into the
	 * device tree, so this can be allocated anywhere.
	 */
	cmdline_ptr = efi_convert_cmdline(sys_table, image, &cmdline_size);
	if (!cmdline_ptr) {
		pr_efi_err(sys_table, "getting command line via LOADED_IMAGE_PROTOCOL\n");
		goto fail;
	}

	status = handle_kernel_image(sys_table, image_addr, &image_size,
				     &reserve_addr,
				     &reserve_size,
				     dram_base, image);
	if (status != EFI_SUCCESS) {
		pr_efi_err(sys_table, "Failed to relocate kernel\n");
		goto fail_free_cmdline;
	}

	if (IS_ENABLED(CONFIG_CMDLINE_EXTEND) ||
	    IS_ENABLED(CONFIG_CMDLINE_FORCE) ||
	    cmdline_size == 0)
		efi_parse_options(CONFIG_CMDLINE);

	if (!IS_ENABLED(CONFIG_CMDLINE_FORCE) && cmdline_size > 0)
		efi_parse_options(cmdline_ptr);

	/*
	 * Unauthenticated device tree data is a security hazard, so
	 * ignore 'dtb=' unless UEFI Secure Boot is disabled.
	 */
	if (efi_secureboot_enabled(sys_table)) {
		pr_efi(sys_table, "UEFI Secure Boot is enabled.\n");
	} else {
		status = handle_cmdline_files(sys_table, image, cmdline_ptr,
					      "dtb=",
					      ~0UL, &fdt_addr, &fdt_size);

		if (status != EFI_SUCCESS) {
			pr_efi_err(sys_table, "Failed to load device tree!\n");
			goto fail_free_image;
		}
	}

	if (fdt_addr) {
		pr_efi(sys_table, "Using DTB from command line\n");
	} else {
		/* Look for a device tree configuration table entry. */
		fdt_addr = (uintptr_t)get_fdt(sys_table, &fdt_size);
		if (fdt_addr)
			pr_efi(sys_table, "Using DTB from configuration table\n");
	}

	if (!fdt_addr)
		pr_efi(sys_table, "Generating empty DTB\n");

	status = handle_cmdline_files(sys_table, image, cmdline_ptr,
				      "initrd=", dram_base + SZ_512M,
				      (unsigned long *)&initrd_addr,
				      (unsigned long *)&initrd_size);
	if (status != EFI_SUCCESS)
		pr_efi_err(sys_table, "Failed initrd from command line!\n");

	new_fdt_addr = fdt_addr;
	status = allocate_new_fdt_and_exit_boot(sys_table, handle,
				&new_fdt_addr, dram_base + MAX_FDT_OFFSET,
				initrd_addr, initrd_size, cmdline_ptr,
				fdt_addr, fdt_size);

	/*
	 * If all went well, we need to return the FDT address to the
	 * calling function so it can be passed to kernel as part of
	 * the kernel boot protocol.
	 */
	if (status == EFI_SUCCESS)
		return new_fdt_addr;

	pr_efi_err(sys_table, "Failed to update FDT and exit boot services\n");

	efi_free(sys_table, initrd_size, initrd_addr);
	efi_free(sys_table, fdt_size, fdt_addr);

fail_free_image:
	efi_free(sys_table, image_size, *image_addr);
	efi_free(sys_table, reserve_size, reserve_addr);
fail_free_cmdline:
	efi_free(sys_table, cmdline_size, (unsigned long)cmdline_ptr);
fail:
	return EFI_ERROR;
}

/*
 * This is the base address at which to start allocating virtual memory ranges
 * for UEFI Runtime Services. This is in the low TTBR0 range so that we can use
 * any allocation we choose, and eliminate the risk of a conflict after kexec.
 * The value chosen is the largest non-zero power of 2 suitable for this purpose
 * both on 32-bit and 64-bit ARM CPUs, to maximize the likelihood that it can
 * be mapped efficiently.
 */
#define EFI_RT_VIRTUAL_BASE	0x40000000

static int cmp_mem_desc(const void *l, const void *r)
{
	const efi_memory_desc_t *left = l, *right = r;

	return (left->phys_addr > right->phys_addr) ? 1 : -1;
}

/*
 * Returns whether region @left ends exactly where region @right starts,
 * or false if either argument is NULL.
 */
static bool regions_are_adjacent(efi_memory_desc_t *left,
				 efi_memory_desc_t *right)
{
	u64 left_end;

	if (left == NULL || right == NULL)
		return false;

	left_end = left->phys_addr + left->num_pages * EFI_PAGE_SIZE;

	return left_end == right->phys_addr;
}

/*
 * Returns whether region @left and region @right have compatible memory type
 * mapping attributes, and are both EFI_MEMORY_RUNTIME regions.
 */
static bool regions_have_compatible_memory_type_attrs(efi_memory_desc_t *left,
						      efi_memory_desc_t *right)
{
	static const u64 mem_type_mask = EFI_MEMORY_WB | EFI_MEMORY_WT |
					 EFI_MEMORY_WC | EFI_MEMORY_UC |
					 EFI_MEMORY_RUNTIME;

	return ((left->attribute ^ right->attribute) & mem_type_mask) == 0;
}

/*
 * efi_get_virtmap() - create a virtual mapping for the EFI memory map
 *
 * This function populates the virt_addr fields of all memory region descriptors
 * in @memory_map whose EFI_MEMORY_RUNTIME attribute is set. Those descriptors
 * are also copied to @runtime_map, and their total count is returned in @count.
 */
void efi_get_virtmap(efi_memory_desc_t *memory_map, unsigned long map_size,
		     unsigned long desc_size, efi_memory_desc_t *runtime_map,
		     int *count)
{
	u64 efi_virt_base = EFI_RT_VIRTUAL_BASE;
	efi_memory_desc_t *in, *prev = NULL, *out = runtime_map;
	int l;

	/*
	 * To work around potential issues with the Properties Table feature
	 * introduced in UEFI 2.5, which may split PE/COFF executable images
	 * in memory into several RuntimeServicesCode and RuntimeServicesData
	 * regions, we need to preserve the relative offsets between adjacent
	 * EFI_MEMORY_RUNTIME regions with the same memory type attributes.
	 * The easiest way to find adjacent regions is to sort the memory map
	 * before traversing it.
	 */
	sort(memory_map, map_size / desc_size, desc_size, cmp_mem_desc, NULL);

	for (l = 0; l < map_size; l += desc_size, prev = in) {
		u64 paddr, size;

		in = (void *)memory_map + l;
		if (!(in->attribute & EFI_MEMORY_RUNTIME))
			continue;

		paddr = in->phys_addr;
		size = in->num_pages * EFI_PAGE_SIZE;

		/*
		 * Make the mapping compatible with 64k pages: this allows
		 * a 4k page size kernel to kexec a 64k page size kernel and
		 * vice versa.
		 */
		if (!regions_are_adjacent(prev, in) ||
		    !regions_have_compatible_memory_type_attrs(prev, in)) {

			paddr = round_down(in->phys_addr, SZ_64K);
			size += in->phys_addr - paddr;

			/*
			 * Avoid wasting memory on PTEs by choosing a virtual
			 * base that is compatible with section mappings if this
			 * region has the appropriate size and physical
			 * alignment. (Sections are 2 MB on 4k granule kernels)
			 */
			if (IS_ALIGNED(in->phys_addr, SZ_2M) && size >= SZ_2M)
				efi_virt_base = round_up(efi_virt_base, SZ_2M);
			else
				efi_virt_base = round_up(efi_virt_base, SZ_64K);
		}

		in->virt_addr = efi_virt_base + in->phys_addr - paddr;
		efi_virt_base += size;

		memcpy(out, in, desc_size);
		out = (void *)out + desc_size;
		++*count;
	}
}
                                   /*
 * Copyright (C) 2013, 2014 Linaro Ltd;  <roy.franz@linaro.org>
 *
 * This file implements the EFI boot stub for the arm64 kernel.
 * Adapted from ARM version by Mark Salter <msalter@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*
 * To prevent the compiler from emitting GOT-indirected (and thus absolute)
 * references to the section markers, override their visibility as 'hidden'
 */
#pragma GCC visibility push(hidden)
#include <asm/sections.h>
#pragma GCC visibility pop

#include <linux/efi.h>
#include <asm/efi.h>

#include "efistub.h"

efi_status_t __init handle_kernel_image(efi_system_table_t *sys_table_arg,
					unsigned long *image_addr,
					unsigned long *image_size,
					unsigned long *reserve_addr,
					unsigned long *reserve_size,
					unsigned long dram_base,
					efi_loaded_image_t *image)
{
	efi_status_t status;
	unsigned long kernel_size, kernel_memsize = 0;
	void *old_image_addr = (void *)*image_addr;
	unsigned long preferred_offset;
	u64 phys_seed = 0;

	if (IS_ENABLED(CONFIG_RANDOMIZE_BASE)) {
		if (!efi__get___nokaslr()) {
			status = efi_get_random_bytes(sys_table_arg,
						      sizeof(phys_seed),
						      (u8 *)&phys_seed);
			if (status == EFI_NOT_FOUND) {
				pr_efi(sys_table_arg, "EFI_RNG_PROTOCOL unavailable, no randomness supplied\n");
			} else if (status != EFI_SUCCESS) {
				pr_efi_err(sys_table_arg, "efi_get_random_bytes() failed\n");
				return status;
			}
		} else {
			pr_efi(sys_table_arg, "KASLR disabled on kernel command line\n");
		}
	}

	/*
	 * The preferred offset of the kernel Image is TEXT_OFFSET bytes beyond
	 * a 2 MB aligned base, which itself may be lower than dram_base, as
	 * long as the resulting offset equals or exceeds it.
	 */
	preferred_offset = round_down(dram_base, MIN_KIMG_ALIGN) + TEXT_OFFSET;
	if (preferred_offset < dram_base)
		preferred_offset += MIN_KIMG_ALIGN;

	kernel_size = _edata - _text;
	kernel_memsize = kernel_size + (_end - _edata);

	if (IS_ENABLED(CONFIG_RANDOMIZE_BASE) && phys_seed != 0) {
		/*
		 * If KASLR is enabled, and we have some randomness available,
		 * locate the kernel at a randomized offset in physical memory.
		 */
		*reserve_size = kernel_memsize + TEXT_OFFSET;
		status = efi_random_alloc(sys_table_arg, *reserve_size,
					  MIN_KIMG_ALIGN, reserve_addr,
					  phys_seed);

		*image_addr = *reserve_addr + TEXT_OFFSET;
	} else {
		/*
		 * Else, try a straight allocation at the preferred offset.
		 * This will work around the issue where, if dram_base == 0x0,
		 * efi_low_alloc() refuses to allocate at 0x0 (to prevent the
		 * address of the allocation to be mistaken for a FAIL return
		 * value or a NULL pointer). It will also ensure that, on
		 * platforms where the [dram_base, dram_base + TEXT_OFFSET)
		 * interval is partially occupied by the firmware (like on APM
		 * Mustang), we can still place the kernel at the address
		 * 'dram_base + TEXT_OFFSET'.
		 */
		if (*image_addr == preferred_offset)
			return EFI_SUCCESS;

		*image_addr = *reserve_addr = preferred_offset;
		*reserve_size = round_up(kernel_memsize, EFI_ALLOC_ALIGN);

		status = efi_call_early(allocate_pages, EFI_ALLOCATE_ADDRESS,
					EFI_LOADER_DATA,
					*reserve_size / EFI_PAGE_SIZE,
					(efi_physical_addr_t *)reserve_addr);
	}

	if (status != EFI_SUCCESS) {
		*reserve_size = kernel_memsize + TEXT_OFFSET;
		status = efi_low_alloc(sys_table_arg, *reserve_size,
				       MIN_KIMG_ALIGN, reserve_addr);

		if (status != EFI_SUCCESS) {
			pr_efi_err(sys_table_arg, "Failed to relocate kernel\n");
			*reserve_size = 0;
			return status;
		}
		*image_addr = *reserve_addr + TEXT_OFFSET;
	}
	memcpy((void *)*image_addr, old_image_addr, kernel_size);

	return EFI_SUCCESS;
}
                                                                                                                                                                                                                                              /*
 * Helper functions used by the EFI stub on multiple
 * architectures. This should be #included by the EFI stub
 * implementation files.
 *
 * Copyright 2011 Intel Corporation; author Matt Fleming
 *
 * This file is part of the Linux kernel, and is made available
 * under the terms of the GNU General Public License version 2.
 *
 */

#include <linux/efi.h>
#include <asm/efi.h>

#include "efistub.h"

/*
 * Some firmware implementations have problems reading files in one go.
 * A read chunk size of 1MB seems to work for most platforms.
 *
 * Unfortunately, reading files in chunks triggers *other* bugs on some
 * platforms, so we provide a way to disable this workaround, which can
 * be done by passing "efi=nochunk" on the EFI boot stub command line.
 *
 * If you experience issues with initrd images being corrupt it's worth
 * trying efi=nochunk, but chunking is enabled by default because there
 * are far more machines that require the workaround than those that
 * break with it enabled.
 */
#define EFI_READ_CHUNK_SIZE	(1024 * 1024)

static unsigned long __chunk_size = EFI_READ_CHUNK_SIZE;

/*
 * Allow the platform to override the allocation granularity: this allows
 * systems that have the capability to run with a larger page size to deal
 * with the allocations for initrd and fdt more efficiently.
 */
#ifndef EFI_ALLOC_ALIGN
#define EFI_ALLOC_ALIGN		EFI_PAGE_SIZE
#endif

static int __section(.data) __nokaslr;

int __pure nokaslr(void)
{
	return __nokaslr;
}

struct file_info {
	efi_file_handle_t *handle;
	u64 size;
};

void efi_printk(efi_system_table_t *sys_table_arg, char *str)
{
	char *s8;

	for (s8 = str; *s8; s8++) {
		efi_char16_t ch[2] = { 0 };

		ch[0] = *s8;
		if (*s8 == '\n') {
			efi_char16_t nl[2] = { '\r', 0 };
			efi_char16_printk(sys_table_arg, nl);
		}

		efi_char16_printk(sys_table_arg, ch);
	}
}

efi_status_t efi_get_memory_map(efi_system_table_t *sys_table_arg,
				efi_memory_desc_t **map,
				unsigned long *map_size,
				unsigned long *desc_size,
				u32 *desc_ver,
				unsigned long *key_ptr)
{
	efi_memory_desc_t *m = NULL;
	efi_status_t status;
	unsigned long key;
	u32 desc_version;

	*map_size = sizeof(*m) * 32;
again:
	/*
	 * Add an additional efi_memory_desc_t because we're doing an
	 * allocation which may be in a new descriptor region.
	 */
	*map_size += sizeof(*m);
	status = efi_call_early(allocate_pool, EFI_LOADER_DATA,
				*map_size, (void **)&m);
	if (status != EFI_SUCCESS)
		goto fail;

	*desc_size = 0;
	key = 0;
	status = efi_call_early(get_memory_map, map_size, m,
				&key, desc_size, &desc_version);
	if (status == EFI_BUFFER_TOO_SMALL) {
		efi_call_early(free_pool, m);
		goto again;
	}

	if (status != EFI_SUCCESS)
		efi_call_early(free_pool, m);

	if (key_ptr && status == EFI_SUCCESS)
		*key_ptr = key;
	if (desc_ver && status == EFI_SUCCESS)
		*desc_ver = desc_version;

fail:
	*map = m;
	return status;
}


unsigned long get_dram_b