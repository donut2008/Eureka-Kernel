#
# Makefile for linux kernel
#

#
# ARM64 maps efi runtime services in userspace addresses
# which don't have KASAN shadow. So dereference of these addresses
# in efi_call_virt() will cause crash if this code instrumented.
#
KASAN_SANITIZE_runtime-wrappers.o	:= n

obj-$(CONFIG_EFI)			+= efi.o vars.o reboot.o
obj-$(CONFIG_EFI_VARS)			+= efivars.o
obj-$(CONFIG_EFI_ESRT)			+= esrt.o
obj-$(CONFIG_EFI_VARS_PSTORE)		+= efi-pstore.o
obj-$(CONFIG_UEFI_CPER)			+= cper.o
obj-$(CONFIG_EFI_RUNTIME_MAP)		+= runtime-map.o
obj-$(CONFIG_EFI_RUNTIME_WRAPPERS)	+= runtime-wrappers.o
obj-$(CONFIG_EFI_STUB)			+= libstub/
obj-$(CONFIG_EFI_FAKE_MEMMAP)		+= fake_mem.o

arm-obj-$(CONFIG_EFI)			:= arm-init.o arm-runtime.o
obj-$(CONFIG_ARM64)			+= $(arm-obj-y)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * Extensible Firmware Interface
 *
 * Based on Extensible Firmware Interface Specification version 2.4
 *
 * Copyright (C) 2013 - 2015 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/efi.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/mm_types.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#include <asm/efi.h>

struct efi_memory_map memmap;

u64 efi_system_table;

static int __init is_normal_ram(efi_memory_desc_t *md)
{
	if (md->attribute & EFI_MEMORY_WB)
		return 1;
	return 0;
}

/*
 * Translate a EFI virtual address into a physical address: this is necessary,
 * as some data members of the EFI system table are virtually remapped after
 * SetVirtualAddressMap() has been called.
 */
static phys_addr_t efi_to_phys(unsigned long addr)
{
	efi_memory_desc_t *md;

	for_each_efi_memory_desc(&memmap, md) {
		if (!(md->attribute & EFI_MEMORY_RUNTIME))
			continue;
		if (md->virt_addr == 0)
			/* no virtual mapping has been installed by the stub */
			break;
		if (md->virt_addr <= addr &&
		    (addr - md->virt_addr) < (md->num_pages << EFI_PAGE_SHIFT))
			return md->phys_addr + addr - md->virt_addr;
	}
	return addr;
}

static int __init uefi_init(void)
{
	efi_char16_t *c16;
	void *config_tables;
	size_t table_size;
	char vendor[100] = "unknown";
	int i, retval;

	efi.systab = early_memremap(efi_system_table,
				    sizeof(efi_system_table_t));
	if (efi.systab == NULL) {
		pr_warn("Unable to map EFI system table.\n");
		return -ENOMEM;
	}

	set_bit(EFI_BOOT, &efi.flags);
	if (IS_ENABLED(CONFIG_64BIT))
		set_bit(EFI_64BIT, &efi.flags);

	/*
	 * Verify the EFI Table
	 */
	if (efi.systab->hdr.signature != EFI_SYSTEM_TABLE_SIGNATURE) {
		pr_err("System table signature incorrect\n");
		retval = -EINVAL;
		goto out;
	}
	if ((efi.systab->hdr.revision >> 16) < 2)
		pr_warn("Warning: EFI system table version %d.%02d, expected 2.00 or greater\n",
			efi.systab->hdr.revision >> 16,
			efi.systab->hdr.revision & 0xffff);

	/* Show what we know for posterity */
	c16 = early_memremap(efi_to_phys(efi.systab->fw_vendor),
			     sizeof(vendor) * sizeof(efi_char16_t));
	if (c16) {
		for (i = 0; i < (int) sizeof(vendor) - 1 && *c16; ++i)
			vendor[i] = c16[i];
		vendor[i] = '\0';
		early_memunmap(c16, sizeof(vendor) * sizeof(efi_char16_t));
	}

	pr_info("EFI v%u.%.02u by %s\n",
		efi.systab->hdr.revision >> 16,
		efi.systab->hdr.revision & 0xffff, vendor);

	table_size = sizeof(efi_config_table_64_t) * efi.systab->nr_tables;
	config_tables = early_memremap(efi_to_phys(efi.systab->tables),
				       table_size);
	if (config_tables == NULL) {
		pr_warn("Unable to map EFI config table array.\n");
		retval = -ENOMEM;
		goto out;
	}
	retval = efi_config_parse_tables(config_tables, efi.systab->nr_tables,
					 sizeof(efi_config_table_t), NULL);

	early_memunmap(config_tables, table_size);
out:
	early_memunmap(efi.systab,  sizeof(efi_system_table_t));
	return retval;
}

/*
 * Return true for RAM regions we want to permanently reserve.
 */
static __init int is_reserve_region(efi_memory_desc_t *md)
{
	switch (md->type) {
	case EFI_LOADER_CODE:
	case EFI_LOADER_DATA:
	case EFI_BOOT_SERVICES_CODE:
	case EFI_BOOT_SERVICES_DATA:
	case EFI_CONVENTIONAL_MEMORY:
	case EFI_PERSISTENT_MEMORY:
		return 0;
	default:
		break;
	}
	return is_normal_ram(md);
}

static __init void reserve_regions(void)
{
	efi_memory_desc_t *md;
	u64 paddr, npages, size;

	if (efi_enabled(EFI_DBG))
		pr_info("Processing EFI memory map:\n");

	for_each_efi_memory_desc(&memmap, md) {
		paddr = md->phys_addr;
		npages = md->num_pages;

		if (efi_enabled(EFI_DBG)) {
			char buf[64];

			pr_info("  0x%012llx-0x%012llx %s",
				paddr, paddr + (npages << EFI_PAGE_SHIFT) - 1,
				efi_md_typeattr_format(buf, sizeof(buf), md));
		}

		memrange_efi_to_native(&paddr, &npages);
		size = npages << PAGE_SHIFT;

		if (is_normal_ram(md))
			early_init_dt_add_memory_arch(paddr, size);

		if (is_reserve_region(md)) {
			memblock_mark_nomap(paddr, size);
			if (efi_enabled(EFI_DBG))
				pr_cont("*");
		}

		if (efi_enabled(EFI_DBG))
			pr_cont("\n");
	}

	set_bit(EFI_MEMMAP, &efi.flags);
}

void __init efi_init(void)
{
	struct efi_fdt_params params;

	/* Grab UEFI information placed in FDT by stub */
	if (!efi_get_fdt_params(&params))
		return;

	efi_system_table = params.system_table;

	memmap.phys_map = params.mmap;
	memmap.map = early_memremap(params.mmap, params.mmap_size);
	if (memmap.map == NULL) {
		/*
		* If we are booting via UEFI, the UEFI memory map is the only
		* description of memory we have, so there is little point in
		* proceeding if we cannot access it.
		*/
		panic("Unable to map EFI memory map.\n");
	}
	memmap.map_end = memmap.map + params.mmap_size;
	memmap.desc_size = params.desc_size;
	memmap.desc_version = params.desc_ver;

	if (uefi_init() < 0)
		return;

	reserve_regions();
	early_memunmap(memmap.map, params.mmap_size);

	if (IS_ENABLED(CONFIG_ARM)) {
		/*
		 * ARM currently does not allow ioremap_cache() to be called on
		 * memory regions that are covered by struct page. So remove the
		 * UEFI memory map from the linear mapping.
		 */
		memblock_mark_nomap(params.mmap & PAGE_MASK,
				    PAGE_ALIGN(params.mmap_size +
					       (params.mmap & ~PAGE_MASK)));
	} else {
		memblock_reserve(params.mmap & PAGE_MASK,
				 PAGE_ALIGN(params.mmap_size +
					    (params.mmap & ~PAGE_MASK)));
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Extensible Firmware Interface
 *
 * Based on Extensible Firmware Interface Specification version 2.4
 *
 * Copyright (C) 2013, 2014 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/efi.h>
#include <linux/io.h>
#include <linux/memblock.h>
#include <linux/mm_types.h>
#include <linux/preempt.h>
#include <linux/rbtree.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <asm/cacheflush.h>
#include <asm/efi.h>
#include <asm/mmu.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>

extern u64 efi_system_table;

static struct mm_struct efi_mm = {
	.mm_rb			= RB_ROOT,
	.mm_users		= ATOMIC_INIT(2),
	.mm_count		= ATOMIC_INIT(1),
	.mmap_sem		= __RWSEM_INITIALIZER(efi_mm.mmap_sem),
	.page_table_lock	= __SPIN_LOCK_UNLOCKED(efi_mm.page_table_lock),
	.mmlist			= LIST_HEAD_INIT(efi_mm.mmlist),
};

static bool __init efi_virtmap_init(void)
{
	efi_memory_desc_t *md;

	efi_mm.pgd = pgd_alloc(&efi_mm);
	init_new_context(NULL, &efi_mm);

	for_each_efi_memory_desc(&memmap, md) {
		phys_addr_t phys = md->phys_addr;
		int ret;

		if (!(md->attribute & EFI_MEMORY_RUNTIME))
			continue;
		if (md->virt_addr == 0)
			return false;

		ret = efi_create_mapping(&efi_mm, md);
		if  (!ret) {
			pr_info("  EFI remap %pa => %p\n",
				&phys, (void *)(unsigned long)md->virt_addr);
		} else {
			pr_warn("  EFI remap %pa: failed to create mapping (%d)\n",
				&phys, ret);
			return false;
		}
	}
	return true;
}

/*
 * Enable the UEFI Runtime Services if all prerequisites are in place, i.e.,
 * non-early mapping of the UEFI system table and virtual mappings for all
 * EFI_MEMORY_RUNTIME regions.
 */
static int __init arm_enable_runtime_services(void)
{
	u64 mapsize;

	if (!efi_enabled(EFI_BOOT)) {
		pr_info("EFI services will not be available.\n");
		return 0;
	}

	if (efi_runtime_disabled()) {
		pr_info("EFI runtime services will be disabled.\n");
		return 0;
	}

	pr_info("Remapping and enabling EFI services.\n");

	mapsize = memmap.map_end - memmap.map;
	memmap.map = (__force void *)ioremap_cache(memmap.phys_map,
						   mapsize);
	if (!memmap.map) {
		pr_err("Failed to remap EFI memory map\n");
		return -ENOMEM;
	}
	memmap.map_end = memmap.map + mapsize;
	efi.memmap = &memmap;

	efi.systab = (__force void *)ioremap_cache(efi_system_table,
						   sizeof(efi_system_table_t));
	if (!efi.systab) {
		pr_err("Failed to remap EFI System Table\n");
		return -ENOMEM;
	}
	set_bit(EFI_SYSTEM_TABLES, &efi.flags);

	if (!efi_virtmap_init()) {
		pr_err("No UEFI virtual mapping was installed -- runtime services will not be available\n");
		return -ENOMEM;
	}

	/* Set up runtime services function pointers */
	efi_native_runtime_setup();
	set_bit(EFI_RUNTIME_SERVICES, &efi.flags);

	efi.runtime_version = efi.systab->hdr.revision;

	return 0;
}
early_initcall(arm_enable_runtime_services);

void efi_virtmap_load(void)
{
	preempt_disable();
	efi_set_pgd(&efi_mm);
}

void efi_virtmap_unload(void)
{
	efi_set_pgd(current->active_mm);
	preempt_enable();
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * UEFI Common Platform Error Record (CPER) support
 *
 * Copyright (C) 2010, Intel Corp.
 *	Author: Huang Ying <ying.huang@intel.com>
 *
 * CPER is the format used to describe platform hardware error by
 * various tables, such as ERST, BERT and HEST etc.
 *
 * For more information about CPER, please refer to Appendix N of UEFI
 * Specification version 2.4.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/cper.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/pci.h>
#include <linux/aer.h>

#define INDENT_SP	" "

/*
 * CPER record ID need to be unique even after reboot, because record
 * ID is used as index for ERST storage, while CPER records from
 * multiple boot may co-exist in ERST.
 */
u64 cper_next_record_id(void)
{
	static atomic64_t seq;

	if (!atomic64_read(&seq))
		atomic64_set(&seq, ((u64)get_seconds()) << 32);

	return atomic64_inc_return(&seq);
}
EXPORT_SYMBOL_GPL(cper_next_record_id);

static const char * const severity_strs[] = {
	"recoverable",
	"fatal",
	"corrected",
	"info",
};

const char *cper_severity_str(unsigned int severity)
{
	return severity < ARRAY_SIZE(severity_strs) ?
		severity_strs[severity] : "unknown";
}
EXPORT_SYMBOL_GPL(cper_severity_str);

/*
 * cper_print_bits - print strings for set bits
 * @pfx: prefix for each line, including log level and prefix string
 * @bits: bit mask
 * @strs: string array, indexed by bit position
 * @strs_size: size of the string array: @strs
 *
 * For each set bit in @bits, print the corresponding string in @strs.
 * If the output length is longer than 80, multiple line will be
 * printed, with @pfx is printed at the beginning of each line.
 */
void cper_print_bits(const char *pfx, unsigned int bits,
		     const char * const strs[], unsigned int strs_size)
{
	int i, len = 0;
	const char *str;
	char buf[84];

	for (i = 0; i < strs_size; i++) {
		if (!(bits & (1U << i)))
			continue;
		str = strs[i];
		if (!str)
			continue;
		if (len && len + strlen(str) + 2 > 80) {
			printk("%s\n", buf);
			len = 0;
		}
		if (!len)
			len = snprintf(buf, sizeof(buf), "%s%s", pfx, str);
		else
			len += snprintf(buf+len, sizeof(buf)-len, ", %s", str);
	}
	if (len)
		printk("%s\n", buf);
}

static const char * const proc_type_strs[] = {
	"IA32/X64",
	"IA64",
};

static const char * const proc_isa_strs[] = {
	"IA32",
	"IA64",
	"X64",
};

static const char * const proc_error_type_strs[] = {
	"cache error",
	"TLB error",
	"bus error",
	"micro-architectural error",
};

static const char * const proc_op_strs[] = {
	"unknown or generic",
	"data read",
	"data write",
	"instruction execution",
};

static const char * const proc_flag_strs[] = {
	"restartable",
	"precise IP",
	"overflow",
	"corrected",
};

static void cper_print_proc_generic(const char *pfx,
				    const struct cper_sec_proc_generic *proc)
{
	if (proc->validation_bits & CPER_PROC_VALID_TYPE)
		printk("%s""processor_type: %d, %s\n", pfx, proc->proc_type,
		       proc->proc_type < ARRAY_SIZE(proc_type_strs) ?
		       proc_type_strs[proc->proc_type] : "unknown");
	if (proc->validation_bits & CPER_PROC_VALID_ISA)
		printk("%s""processor_isa: %d, %s\n", pfx, proc->proc_isa,
		       proc->proc_isa < ARRAY_SIZE(proc_isa_strs) ?
		       proc_isa_strs[proc->proc_isa] : "unknown");
	if (proc->validation_bits & CPER_PROC_VALID_ERROR_TYPE) {
		printk("%s""error_type: 0x%02x\n", pfx, proc->proc_error_type);
		cper_print_bits(pfx, proc->proc_error_type,
				proc_error_type_strs,
				ARRAY_SIZE(proc_error_type_strs));
	}
	if (proc->validation_bits & CPER_PROC_VALID_OPERATION)
		printk("%s""operation: %d, %s\n", pfx, proc->operation,
		       proc->operation < ARRAY_SIZE(proc_op_strs) ?
		       proc_op_strs[proc->operation] : "unknown");
	if (proc->validation_bits & CPER_PROC_VALID_FLAGS) {
		printk("%s""flags: 0x%02x\n", pfx, proc->flags);
		cper_print_bits(pfx, proc->flags, proc_flag_strs,
				ARRAY_SIZE(proc_flag_strs));
	}
	if (proc->validation_bits & CPER_PROC_VALID_LEVEL)
		printk("%s""level: %d\n", pfx, proc->level);
	if (proc->validation_bits & CPER_PROC_VALID_VERSION)
		printk("%s""version_info: 0x%016llx\n", pfx, proc->cpu_version);
	if (proc->validation_bits & CPER_PROC_VALID_ID)
		printk("%s""processor_id: 0x%016llx\n", pfx, proc->proc_id);
	if (proc->validation_bits & CPER_PROC_VALID_TARGET_ADDRESS)
		printk("%s""target_address: 0x%016llx\n",
		       pfx, proc->target_addr);
	if (proc->validation_bits & CPER_PROC_VALID_REQUESTOR_ID)
		printk("%s""requestor_id: 0x%016llx\n",
		       pfx, proc->requestor_id);
	if (proc->validation_bits & CPER_PROC_VALID_RESPONDER_ID)
		printk("%s""responder_id: 0x%016llx\n",
		       pfx, proc->responder_id);
	if (proc->validation_bits & CPER_PROC_VALID_IP)
		printk("%s""IP: 0x%016llx\n", pfx, proc->ip);
}

static const char * const mem_err_type_strs[] = {
	"unknown",
	"no error",
	"single-bit ECC",
	"multi-bit ECC",
	"single-symbol chipkill ECC",
	"multi-symbol chipkill ECC",
	"master abort",
	"target abort",
	"parity error",
	"watchdog timeout",
	"invalid address",
	"mirror Broken",
	"memory sparing",
	"scrub corrected error",
	"scrub uncorrected error",
	"physical memory map-out event",
};

const char *cper_mem_err_type_str(unsigned int etype)
{
	return etype < ARRAY_SIZE(mem_err_type_strs) ?
		mem_err_type_strs[etype] : "unknown";
}
EXPORT_SYMBOL_GPL(cper_mem_err_type_str);

static int cper_mem_err_location(struct cper_mem_err_compact *mem, char *msg)
{
	u32 len, n;

	if (!msg)
		return 0;

	n = 0;
	len = CPER_REC_LEN - 1;
	if (mem->validation_bits & CPER_MEM_VALID_NODE)
		n += scnprintf(msg + n, len - n, "node: %d ", mem->node);
	if (mem->validation_bits & CPER_MEM_VALID_CARD)
		n += scnprintf(msg + n, len - n, "card: %d ", mem->card);
	if (mem->validation_bits & CPER_MEM_VALID_MODULE)
		n += scnprintf(msg + n, len - n, "module: %d ", mem->module);
	if (mem->validation_bits & CPER_MEM_VALID_RANK_NUMBER)
		n += scnprintf(msg + n, len - n, "rank: %d ", mem->rank);
	if (mem->validation_bits & CPER_MEM_VALID_BANK)
		n += scnprintf(msg + n, len - n, "bank: %d ", mem->bank);
	if (mem->validation_bits & CPER_MEM_VALID_DEVICE)
		n += scnprintf(msg + n, len - n, "device: %d ", mem->device);
	if (mem->validation_bits & CPER_MEM_VALID_ROW)
		n += scnprintf(msg + n, len - n, "row: %d ", mem->row);
	if (mem->validation_bits & CPER_MEM_VALID_COLUMN)
		n += scnprintf(msg + n, len - n, "column: %d ", mem->column);
	if (mem->validation_bits & CPER_MEM_VALID_BIT_POSITION)
		n += scnprintf(msg + n, len - n, "bit_position: %d ",
			       mem->bit_pos);
	if (mem->validation_bits & CPER_MEM_VALID_REQUESTOR_ID)
		n += scnprintf(msg + n, len - n, "requestor_id: 0x%016llx ",
			       mem->requestor_id);
	if (mem->validation_bits & CPER_MEM_VALID_RESPONDER_ID)
		n += scnprintf(msg + n, len - n, "responder_id: 0x%016llx ",
			       mem->responder_id);
	if (mem->validation_bits & CPER_MEM_VALID_TARGET_ID)
		scnprintf(msg + n, len - n, "target_id: 0x%016llx ",
			  mem->target_id);

	msg[n] = '\0';
	return n;
}

static int cper_dimm_err_location(struct cper_mem_err_compact *mem, char *msg)
{
	u32 len, n;
	const char *bank = NULL, *device = NULL;

	if (!msg || !(mem->validation_bits & CPER_MEM_VALID_MODULE_HANDLE))
		return 0;

	len = CPER_REC_LEN;
	dmi_memdev_name(mem->mem_dev_handle, &bank, &device);
	if (bank && device)
		n = snprintf(msg, len, "DIMM location: %s %s ", bank, device);
	else
		n = snprintf(msg, len,
			     "DIMM location: not present. DMI handle: 0x%.4x ",
			     mem->mem_dev_handle);

	return n;
}

void cper_mem_err_pack(const struct cper_sec_mem_err *mem,
		       struct cper_mem_err_compact *cmem)
{
	cmem->validation_bits = mem->validation_bits;
	cmem->node = mem->node;
	cmem->card = mem->card;
	cmem->module = mem->module;
	cmem->bank = mem->bank;
	cmem->device = mem->device;
	cmem->row = mem->row;
	cmem->column = mem->column;
	cmem->bit_pos = mem->bit_pos;
	cmem->requestor_id = mem->requestor_id;
	cmem->responder_id = mem->responder_id;
	cmem->target_id = mem->target_id;
	cmem->rank = mem->rank;
	cmem->mem_array_handle = mem->mem_array_handle;
	cmem->mem_dev_handle = mem->mem_dev_handle;
}

const char *cper_mem_err_unpack(struct trace_seq *p,
				struct cper_mem_err_compact *cmem)
{
	const char *ret = trace_seq_buffer_ptr(p);
	char rcd_decode_str[CPER_REC_LEN];

	if (cper_mem_err_location(cmem, rcd_decode_str))
		trace_seq_printf(p, "%s", rcd_decode_str);
	if (cper_dimm_err_location(cmem, rcd_decode_str))
		trace_seq_printf(p, "%s", rcd_decode_str);
	trace_seq_putc(p, '\0');

	return ret;
}

static void cper_print_mem(const char *pfx, const struct cper_sec_mem_err *mem,
	int len)
{
	struct cper_mem_err_compact cmem;
	char rcd_decode_str[CPER_REC_LEN];

	/* Don't trust UEFI 2.1/2.2 structure with bad validation bits */
	if (len == sizeof(struct cper_sec_mem_err_old) &&
	    (mem->validation_bits & ~(CPER_MEM_VALID_RANK_NUMBER - 1))) {
		pr_err(FW_WARN "valid bits set for fields beyond structure\n");
		return;
	}
	if (mem->validation_bits & CPER_MEM_VALID_ERROR_STATUS)
		printk("%s""error_status: 0x%016llx\n", pfx, mem->error_st