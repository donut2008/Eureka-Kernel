tus = efi_high_alloc(sys_table_arg, options_bytes, 0,
				&cmdline_addr, MAX_CMDLINE_ADDRESS);
	if (status != EFI_SUCCESS)
		return NULL;

	s1 = (u8 *)cmdline_addr;
	s2 = (const u16 *)options;

	s1 = efi_utf16_to_utf8(s1, s2, options_chars);
	*s1 = '\0';

	*cmd_line_len = options_bytes;
	return (char *)cmdline_addr;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
#ifndef _DRIVERS_FIRMWARE_EFI_EFISTUB_H
#define _DRIVERS_FIRMWARE_EFI_EFISTUB_H

/* error code which can't be mistaken for valid address */
#define EFI_ERROR	(~0UL)

extern int __pure nokaslr(void);

void efi_char16_printk(efi_system_table_t *, efi_char16_t *);

bool efi__get___nokaslr(void);

efi_status_t efi_open_volume(efi_system_table_t *sys_table_arg, void *__image,
			     void **__fh);

efi_status_t efi_file_size(efi_system_table_t *sys_table_arg, void *__fh,
			   efi_char16_t *filename_16, void **handle,
			   u64 *file_sz);

efi_status_t efi_file_read(void *handle, unsigned long *size, void *addr);

efi_status_t efi_file_close(void *handle);

unsigned long get_dram_base(efi_system_table_t *sys_table_arg);

efi_status_t update_fdt(efi_system_table_t *sys_table, void *orig_fdt,
			unsigned long orig_fdt_size,
			void *fdt, int new_fdt_size, char *cmdline_ptr,
			u64 initrd_addr, u64 initrd_size,
			efi_memory_desc_t *memory_map,
			unsigned long map_size, unsigned long desc_size,
			u32 desc_ver);

efi_status_t allocate_new_fdt_and_exit_boot(efi_system_table_t *sys_table,
					    void *handle,
					    unsigned long *new_fdt_addr,
					    unsigned long max_addr,
					    u64 initrd_addr, u64 initrd_size,
					    char *cmdline_ptr,
					    unsigned long fdt_addr,
					    unsigned long fdt_size);

void *get_fdt(efi_system_table_t *sys_table, unsigned long *fdt_size);

void efi_get_virtmap(efi_memory_desc_t *memory_map, unsigned long map_size,
		     unsigned long desc_size, efi_memory_desc_t *runtime_map,
		     int *count);

efi_status_t efi_get_random_bytes(efi_system_table_t *sys_table,
				  unsigned long size, u8 *out);

efi_status_t efi_random_alloc(efi_system_table_t *sys_table_arg,
			      unsigned long size, unsigned long align,
			      unsigned long *addr, unsigned long random_seed);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * FDT related Helper functions used by the EFI stub on multiple
 * architectures. This should be #included by the EFI stub
 * implementation files.
 *
 * Copyright 2013 Linaro Limited; author Roy Franz
 *
 * This file is part of the Linux kernel, and is made available
 * under the terms of the GNU General Public License version 2.
 *
 */

#include <linux/efi.h>
#include <linux/libfdt.h>
#include <asm/efi.h>

#include "efistub.h"

efi_status_t update_fdt(efi_system_table_t *sys_table, void *orig_fdt,
			unsigned long orig_fdt_size,
			void *fdt, int new_fdt_size, char *cmdline_ptr,
			u64 initrd_addr, u64 initrd_size,
			efi_memory_desc_t *memory_map,
			unsigned long map_size, unsigned long desc_size,
			u32 desc_ver)
{
	int node, prev, num_rsv;
	int status;
	u32 fdt_val32;
	u64 fdt_val64;

	/* Do some checks on provided FDT, if it exists*/
	if (orig_fdt) {
		if (fdt_check_header(orig_fdt)) {
			pr_efi_err(sys_table, "Device Tree header not valid!\n");
			return EFI_LOAD_ERROR;
		}
		/*
		 * We don't get the size of the FDT if we get if from a
		 * configuration table.
		 */
		if (orig_fdt_size && fdt_totalsize(orig_fdt) > orig_fdt_size) {
			pr_efi_err(sys_table, "Truncated device tree! foo!\n");
			return EFI_LOAD_ERROR;
		}
	}

	if (orig_fdt)
		status = fdt_open_into(orig_fdt, fdt, new_fdt_size);
	else
		status = fdt_create_empty_tree(fdt, new_fdt_size);

	if (status != 0)
		goto fdt_set_fail;

	/*
	 * Delete any memory nodes present. We must delete nodes which
	 * early_init_dt_scan_memory may try to use.
	 */
	prev = 0;
	for (;;) {
		const char *type;
		int len;

		node = fdt_next_node(fdt, prev, NULL);
		if (node < 0)
			break;

		type = fdt_getprop(fdt, node, "device_type", &len);
		if (type && strncmp(type, "memory", len) == 0) {
			fdt_del_node(fdt, node);
			continue;
		}

		prev = node;
	}

	/*
	 * Delete all memory reserve map entries. When booting via UEFI,
	 * kernel will use the UEFI memory map to find reserved regions.
	 */
	num_rsv = fdt_num_mem_rsv(fdt);
	while (num_rsv-- > 0)
		fdt_del_mem_rsv(fdt, num_rsv);

	node = fdt_subnode_offset(fdt, 0, "chosen");
	if (node < 0) {
		node = fdt_add_subnode(fdt, 0, "chosen");
		if (node < 0) {
			status = node; /* node is error code when negative */
			goto fdt_set_fail;
		}
	}

	if ((cmdline_ptr != NULL) && (strlen(cmdline_ptr) > 0)) {
		status = fdt_setprop(fdt, node, "bootargs", cmdline_ptr,
				     strlen(cmdline_ptr) + 1);
		if (status)
			goto fdt_set_fail;
	}

	/* Set initrd address/end in device tree, if present */
	if (initrd_size != 0) {
		u64 initrd_image_end;
		u64 initrd_image_start = cpu_to_fdt64(initrd_addr);

		status = fdt_setprop(fdt, node, "linux,initrd-start",
				     &initrd_image_start, sizeof(u64));
		if (status)
			goto fdt_set_fail;
		initrd_image_end = cpu_to_fdt64(initrd_addr + initrd_size);
		status = fdt_setprop(fdt, node, "linux,initrd-end",
				     &initrd_image_end, sizeof(u64));
		if (status)
			goto fdt_set_fail;
	}

	/* Add FDT entries for EFI runtime services in chosen node. */
	node = fdt_subnode_offset(fdt, 0, "chosen");
	fdt_val64 = cpu_to_fdt64((u64)(unsigned long)sys_table);
	status = fdt_setprop(fdt, node, "linux,uefi-system-table",
			     &fdt_val64, sizeof(fdt_val64));
	if (status)
		goto fdt_set_fail;

	fdt_val64 = cpu_to_fdt64((u64)(unsigned long)memory_map);
	status = fdt_setprop(fdt, node, "linux,uefi-mmap-start",
			     &fdt_val64,  sizeof(fdt_val64));
	if (status)
		goto fdt_set_fail;

	fdt_val32 = cpu_to_fdt32(map_size);
	status = fdt_setprop(fdt, node, "linux,uefi-mmap-size",
			     &fdt_val32,  sizeof(fdt_val32));
	if (status)
		goto fdt_set_fail;

	fdt_val32 = cpu_to_fdt32(desc_size);
	status = fdt_setprop(fdt, node, "linux,uefi-mmap-desc-size",
			     &fdt_val32, sizeof(fdt_val32));
	if (status)
		goto fdt_set_fail;

	fdt_val32 = cpu_to_fdt32(desc_ver);
	status = fdt_setprop(fdt, node, "linux,uefi-mmap-desc-ver",
			     &fdt_val32, sizeof(fdt_val32));
	if (status)
		goto fdt_set_fail;

	if (IS_ENABLED(CONFIG_RANDOMIZE_BASE)) {
		efi_status_t efi_status;

		efi_status = efi_get_random_bytes(sys_table, sizeof(fdt_val64),
						  (u8 *)&fdt_val64);
		if (efi_status == EFI_SUCCESS) {
			status = fdt_setprop(fdt, node, "kaslr-seed",
					     &fdt_val64, sizeof(fdt_val64));
			if (status)
				goto fdt_set_fail;
		} else if (efi_status != EFI_NOT_FOUND) {
			return efi_status;
		}
	}
	return EFI_SUCCESS;

fdt_set_fail:
	if (status == -FDT_ERR_NOSPACE)
		return EFI_BUFFER_TOO_SMALL;

	return EFI_LOAD_ERROR;
}

#ifndef EFI_FDT_ALIGN
#define EFI_FDT_ALIGN EFI_PAGE_SIZE
#endif

/*
 * Allocate memory for a new FDT, then add EFI, commandline, and
 * initrd related fields to the FDT.  This routine increases the
 * FDT allocation size until the allocated memory is large
 * enough.  EFI allocations are in EFI_PAGE_SIZE granules,
 * which are fixed at 4K bytes, so in most cases the first
 * allocation should succeed.
 * EFI boot services are exited at the end of this function.
 * There must be no allocations between the get_memory_map()
 * call and the exit_boot_services() call, so the exiting of
 * boot services is very tightly tied to the creation of the FDT
 * with the final memory map in it.
 */

efi_status_t allocate_new_fdt_and_exit_boot(efi_system_table_t *sys_table,
					    void *handle,
					    unsigned long *new_fdt_addr,
					    unsigned long max_addr,
					    u64 initrd_addr, u64 initrd_size,
					    char *cmdline_ptr,
					    unsigned long fdt_addr,
					    unsigned long fdt_size)
{
	unsigned long map_size, desc_size;
	u32 desc_ver;
	unsigned long mmap_key;
	efi_memory_desc_t *memory_map, *runtime_map;
	unsigned long new_fdt_size;
	efi_status_t status;
	int runtime_entry_count = 0;

	/*
	 * Get a copy of the current memory map that we will use to prepare
	 * the input for SetVirtualAddressMap(). We don't have to worry about
	 * subsequent allocations adding entries, since they could not affect
	 * the number of EFI_MEMORY_RUNTIME regions.
	 */
	status = efi_get_memory_map(sys_table, &runtime_map, &map_size,
				    &desc_size, &desc_ver, &mmap_key);
	if (status != EFI_SUCCESS) {
		pr_efi_err(sys_table, "Unable to retrieve UEFI memory map.\n");
		return status;
	}

	pr_efi(sys_table,
	       "Exiting boot services and installing virtual address map...\n");

	/*
	 * Estimate size of new FDT, and allocate memory for it. We
	 * will allocate a bigger buffer if this ends up being too
	 * small, so a rough guess is OK here.
	 */
	new_fdt_size = fdt_size + EFI_PAGE_SIZE;
	while (1) {
		status = efi_high_alloc(sys_table, new_fdt_size, EFI_FDT_ALIGN,
					new_fdt_addr, max_addr);
		if (status != EFI_SUCCESS) {
			pr_efi_err(sys_table, "Unable to allocate memory for new device tree.\n");
			goto fail;
		}

		/*
		 * Now that we have done our final memory allocation (and free)
		 * we can get the memory map key  needed for
		 * exit_boot_services().
		 */
		status = efi_get_memory_map(sys_table, &memory_map, &map_size,
					    &desc_size, &desc_ver, &mmap_key);
		if (status != EFI_SUCCESS)
			goto fail_free_new_fdt;

		status = update_fdt(sys_table,
				    (void *)fdt_addr, fdt_size,
				    (void *)*new_fdt_addr, new_fdt_size,
				    cmdline_ptr, initrd_addr, initrd_size,
				    memory_map, map_size, desc_size, desc_ver);

		/* Succeeding the first time is the expected case. */
		if (status == EFI_SUCCESS)
			break;

		if (status == EFI_BUFFER_TOO_SMALL) {
			/*
			 * We need to allocate more space for the new
			 * device tree, so free existing buffer that is
			 * too small.  Also free memory map, as we will need
			 * to get new one that reflects the free/alloc we do
			 * on the device tree buffer.
			 */
			efi_free(sys_table, new_fdt_size, *new_fdt_addr);
			sys_table->boottime->free_pool(memory_map);
			new_fdt_size += EFI_PAGE_SIZE;
		} else {
			pr_efi_err(sys_table, "Unable to constuct new device tree.\n");
			goto fail_free_mmap;
		}
	}

	/*
	 * Update the memory map with virtual addresses. The function will also
	 * populate @runtime_map with copies of just the EFI_MEMORY_RUNTIME
	 * entries so that we can pass it straight into SetVirtualAddressMap()
	 */
	efi_get_virtmap(memory_map, map_size, desc_size, runtime_map,
			&runtime_entry_count);

	/* Now we are ready to exit_boot_services.*/
	status = sys_table->boottime->exit_boot_services(handle, mmap_key);

	if (status == EFI_SUCCESS) {
		efi_set_virtual_address_map_t *svam;

		/* Install the new virtual address map */
		svam = sys_table->runtime->set_virtual_address_map;
		status = svam(runtime_entry_count * desc_size, desc_size,
			      desc_ver, runtime_map);

		/*
		 * We are beyond the point of no return here, so if the call to
		 * SetVirtualAddressMap() failed, we need to signal that to the
		 * incoming kernel but proceed normally otherwise.
		 */
		if (status != EFI_SUCCESS) {
			int l;

			/*
			 * Set the virtual address field of all
			 * EFI_MEMORY_RUNTIME entries to 0. This will signal
			 * the incoming kernel that no virtual translation has
			 * been installed.
			 */
			for (l = 0; l < map_size; l += desc_size) {
				efi_memory_desc_t *p = (void *)memory_map + l;

				if (p->attribute & EFI_MEMORY_RUNTIME)
					p->virt_addr = 0;
			}
		}
		return EFI_SUCCESS;
	}

	pr_efi_err(sys_table, "Exit boot services failed.\n");

fail_free_mmap:
	sys_table->boottime->free_pool(memory_map);

fail_free_new_fdt:
	efi_free(sys_table, new_fdt_size, *new_fdt_addr);

fail:
	sys_table->boottime->free_pool(runtime_map);
	return EFI_LOAD_ERROR;
}

void *get_fdt(efi_system_table_t *sys_table, unsigned long *fdt_size)
{
	efi_guid_t fdt_guid = DEVICE_TREE_GUID;
	efi_config_table_t *tables;
	void *fdt;
	int i;

	tables = (efi_config_table_t *) sys_table->tables;
	fdt = NULL;

	for (i = 0; i < sys_table->nr_tables; i++)
		if (efi_guidcmp(tables[i].guid, fdt_guid) == 0) {
			fdt = (void *) tables[i].table;
			if (fdt_check_header(fdt) != 0) {
				pr_efi_err(sys_table, "Invalid header detected on UEFI supplied FDT, ignoring ...\n");
				return NULL;
			}
			*fdt_size = fdt_totalsize(fdt);
			break;
	 }

	return fdt;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright (C) 2016 Linaro Ltd;  <ard.biesheuvel@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/efi.h>
#include <asm/efi.h>

#include "efistub.h"

struct efi_rng_protocol {
	efi_status_t (*get_info)(struct efi_rng_protocol *,
				 unsigned long *, efi_guid_t *);
	efi_status_t (*get_rng)(struct efi_rng_protocol *,
				efi_guid_t *, unsigned long, u8 *out);
};

efi_status_t efi_get_random_bytes(efi_system_table_t *sys_table_arg,
				  unsigned long size, u8 *out)
{
	efi_guid_t rng_proto = EFI_RNG_PROTOCOL_GUID;
	efi_status_t status;
	struct efi_rng_protocol *rng;

	status = efi_call_early(locate_protocol, &rng_proto, NULL,
				(void **)&rng);
	if (status != EFI_SUCCESS)
		return status;

	return rng->get_rng(rng, NULL, size, out);
}

/*
 * Return the number of slots covered by this entry, i.e., the number of
 * addresses it covers that are suitably aligned and supply enough room
 * for the allocation.
 */
static unsigned long get_entry_num_slots(efi_memory_desc_t *md,
					 unsigned long size,
					 unsigned long align)
{
	u64 start, end;

	if (md->type != EFI_CONVENTIONAL_MEMORY)
		return 0;

	start = round_up(md->phys_addr, align);
	end = round_down(md->phys_addr + md->num_pages * EFI_PAGE_SIZE - size,
			 align);

	if (start > end)
		return 0;

	return (end - start + 1) / align;
}

/*
 * The UEFI memory descriptors have a virtual address field that is only used
 * when installing the virtual mapping using SetVirtualAddressMap(). Since it
 * is unused here, we can reuse it to keep track of each descriptor's slot
 * count.
 */
#define MD_NUM_SLOTS(md)	((md)->virt_addr)

efi_status_t efi_random_alloc(efi_system_table_t *sys_table_arg,
			      unsigned long size,
			      unsigned long align,
			      unsigned long *addr,
			      unsigned long random_seed)
{
	unsigned long map_size, desc_size, total_slots = 0, target_slot;
	efi_status_t status;
	efi_memory_desc_t *memory_map;
	int map_offset;

	status = efi_get_memory_map(sys_table_arg, &memory_map, &map_size,
				    &desc_size, NULL, NULL);
	if (status != EFI_SUCCESS)
		return status;

	if (align < EFI_ALLOC_ALIGN)
		align = EFI_ALLOC_ALIGN;

	/* count the suitable slots in each memory map entry */
	for (map_offset = 0; map_offset < map_size; map_offset += desc_size) {
		efi_memory_desc_t *md = (void *)memory_map + map_offset;
		unsigned long slots;

		slots = get_entry_num_slots(md, size, align);
		MD_NUM_SLOTS(md) = slots;
		total_slots += slots;
	}

	/* find a random number between 0 and total_slots */
	target_slot = (total_slots * (u16)random_seed) >> 16;

	/*
	 * target_slot is now a value in the range [0, total_slots), and so
	 * it corresponds with exactly one of the suitable slots we recorded
	 * when iterating over the memory map the first time around.
	 *
	 * So iterate over the memory map again, subtracting the number of
	 * slots of each entry at each iteration, until we have found the entry
	 * that covers our chosen slot. Use the residual value of target_slot
	 * to calculate the randomly chosen address, and allocate it directly
	 * using EFI_ALLOCATE_ADDRESS.
	 */
	for (map_offset = 0; map_offset < map_size; map_offset += desc_size) {
		efi_memory_desc_t *md = (void *)memory_map + map_offset;
		efi_physical_addr_t target;
		unsigned long pages;

		if (target_slot >= MD_NUM_SLOTS(md)) {
			target_slot -= MD_NUM_SLOTS(md);
			continue;
		}

		target = round_up(md->phys_addr, align) + target_slot * align;
		pages = round_up(size, EFI_PAGE_SIZE) / EFI_PAGE_SIZE;

		status = efi_call_early(allocate_pages, EFI_ALLOCATE_ADDRESS,
					EFI_LOADER_DATA, pages, &target);
		if (status == EFI_SUCCESS)
			*addr = target;
		break;
	}

	efi_call_early(free_pool, memory_map);

	return status;
}
                                                                                                                                                                                                        /*
 * Taken from:
 *  linux/lib/string.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/types.h>
#include <linux/string.h>

#ifndef __HAVE_ARCH_STRSTR
/**
 * strstr - Find the first sub