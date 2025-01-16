+ size;
		if (phys_addr >= md->phys_addr && phys_addr < end) {
			memcpy(out_md, md, sizeof(*out_md));
			early_memunmap(md, sizeof (*md));
			return 0;
		}

		early_memunmap(md, sizeof (*md));
	}
	return -ENOENT;
}

/*
 * Calculate the highest address of an efi memory descriptor.
 */
u64 __init efi_mem_desc_end(efi_memory_desc_t *md)
{
	u64 size = md->num_pages << EFI_PAGE_SHIFT;
	u64 end = md->phys_addr + size;
	return end;
}

static __initdata efi_config_table_type_t common_tables[] = {
	{ACPI_20_TABLE_GUID, "ACPI 2.0", &efi.acpi20},
	{ACPI_TABLE_GUID, "ACPI", &efi.acpi},
	{HCDP_TABLE_GUID, "HCDP", &efi.hcdp},
	{MPS_TABLE_GUID, "MPS", &efi.mps},
	{SAL_SYSTEM_TABLE_GUID, "SALsystab", &efi.sal_systab},
	{SMBIOS_TABLE_GUID, "SMBIOS", &efi.smbios},
	{SMBIOS3_TABLE_GUID, "SMBIOS 3.0", &efi.smbios3},
	{UGA_IO_PROTOCOL_GUID, "UGA", &efi.uga},
	{EFI_SYSTEM_RESOURCE_TABLE_GUID, "ESRT", &efi.esrt},
	{EFI_PROPERTIES_TABLE_GUID, "PROP", &efi.properties_table},
	{NULL_GUID, NULL, NULL},
};

static __init int match_config_table(efi_guid_t *guid,
				     unsigned long table,
				     efi_config_table_type_t *table_types)
{
	int i;

	if (table_types) {
		for (i = 0; efi_guidcmp(table_types[i].guid, NULL_GUID); i++) {
			if (!efi_guidcmp(*guid, table_types[i].guid)) {
				*(table_types[i].ptr) = table;
				pr_cont(" %s=0x%lx ",
					table_types[i].name, table);
				return 1;
			}
		}
	}

	return 0;
}

int __init efi_config_parse_tables(void *config_tables, int count, int sz,
				   efi_config_table_type_t *arch_tables)
{
	void *tablep;
	int i;

	tablep = config_tables;
	pr_info("");
	for (i = 0; i < count; i++) {
		efi_guid_t guid;
		unsigned long table;

		if (efi_enabled(EFI_64BIT)) {
			u64 table64;
			guid = ((efi_config_table_64_t *)tablep)->guid;
			table64 = ((efi_config_table_64_t *)tablep)->table;
			table = table64;
#ifndef CONFIG_64BIT
			if (table64 >> 32) {
				pr_cont("\n");
				pr_err("Table located above 4GB, disabling EFI.\n");
				return -EINVAL;
			}
#endif
		} else {
			guid = ((efi_config_table_32_t *)tablep)->guid;
			table = ((efi_config_table_32_t *)tablep)->table;
		}

		if (!match_config_table(&guid, table, common_tables))
			match_config_table(&guid, table, arch_tables);

		tablep += sz;
	}
	pr_cont("\n");
	set_bit(EFI_CONFIG_TABLES, &efi.flags);

	/* Parse the EFI Properties table if it exists */
	if (efi.properties_table != EFI_INVALID_TABLE_ADDR) {
		efi_properties_table_t *tbl;

		tbl = early_memremap(efi.properties_table, sizeof(*tbl));
		if (tbl == NULL) {
			pr_err("Could not map Properties table!\n");
			return -ENOMEM;
		}

		if (tbl->memory_protection_attribute &
		    EFI_PROPERTIES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA)
			set_bit(EFI_NX_PE_DATA, &efi.flags);

		early_memunmap(tbl, sizeof(*tbl));
	}

	return 0;
}

int __init efi_config_init(efi_config_table_type_t *arch_tables)
{
	void *config_tables;
	int sz, ret;

	if (efi_enabled(EFI_64BIT))
		sz = sizeof(efi_config_table_64_t);
	else
		sz = sizeof(efi_config_table_32_t);

	/*
	 * Let's see what config tables the firmware passed to us.
	 */
	config_tables = early_memremap(efi.systab->tables,
				       efi.systab->nr_tables * sz);
	if (config_tables == NULL) {
		pr_err("Could not map Configuration table!\n");
		return -ENOMEM;
	}

	ret = efi_config_parse_tables(config_tables, efi.systab->nr_tables, sz,
				      arch_tables);

	early_memunmap(config_tables, efi.systab->nr_tables * sz);
	return ret;
}

#ifdef CONFIG_EFI_VARS_MODULE
static int __init efi_load_efivars(void)
{
	struct platform_device *pdev;

	if (!efi_enabled(EFI_RUNTIME_SERVICES))
		return 0;

	pdev = platform_device_register_simple("efivars", 0, NULL, 0);
	return IS_ERR(pdev) ? PTR_ERR(pdev) : 0;
}
device_initcall(efi_load_efivars);
#endif

#ifdef CONFIG_EFI_PARAMS_FROM_FDT

#define UEFI_PARAM(name, prop, field)			   \
	{						   \
		{ name },				   \
		{ prop },				   \
		offsetof(struct efi_fdt_params, field),    \
		FIELD_SIZEOF(struct efi_fdt_params, field) \
	}

static __initdata struct {
	const char name[32];
	const char propname[32];
	int offset;
	int size;
} dt_params[] = {
	UEFI_PARAM("System Table", "linux,uefi-system-table", system_table),
	UEFI_PARAM("MemMap Address", "linux,uefi-mmap-start", mmap),
	UEFI_PARAM("MemMap Size", "linux,uefi-mmap-size", mmap_size),
	UEFI_PARAM("MemMap Desc. Size", "linux,uefi-mmap-desc-size", desc_size),
	UEFI_PARAM("MemMap Desc. Version", "linux,uefi-mmap-desc-ver", desc_ver)
};

struct param_info {
	int found;
	void *params;
};

static int __init fdt_find_uefi_params(unsigned long node, const char *uname,
				       int depth, void *data)
{
	struct param_info *info = data;
	const void *prop;
	void *dest;
	u64 val;
	int i, len;

	if (depth != 1 || strcmp(uname, "chosen") != 0)
		return 0;

	for (i = 0; i < ARRAY_SIZE(dt_params); i++) {
		prop = of_get_flat_dt_prop(node, dt_params[i].propname, &len);
		if (!prop)
			return 0;
		dest = info->params + dt_params[i].offset;
		info->found++;

		val = of_read_number(prop, len / sizeof(u32));

		if (dt_params[i].size == sizeof(u32))
			*(u32 *)dest = val;
		else
			*(u64 *)dest = val;

		if (efi_enabled(EFI_DBG))
			pr_info("  %s: 0x%0*llx\n", dt_params[i].name,
				dt_params[i].size * 2, val);
	}
	return 1;
}

int __init efi_get_fdt_params(struct efi_fdt_params *params)
{
	struct param_info info;
	int ret;

	pr_info("Getting EFI parameters from FDT:\n");

	info.found = 0;
	info.params = params;

	ret = of_scan_flat_dt(fdt_find_uefi_params, &info);
	if (!info.found)
		pr_info("UEFI not found.\n");
	else if (!ret)
		pr_err("Can't find '%s' in device tree!\n",
		       dt_params[info.found].name);

	return ret;
}
#endif /* CONFIG_EFI_PARAMS_FROM_FDT */

static __initdata char memory_type_name[][20] = {
	"Reserved",
	"Loader Code",
	"Loader Data",
	"Boot Code",
	"Boot Data",
	"Runtime Code",
	"Runtime Data",
	"Conventional Memory",
	"Unusable Memory",
	"ACPI Reclaim Memory",
	"ACPI Memory NVS",
	"Memory Mapped I/O",
	"MMIO Port Space",
	"PAL Code"
};

char * __init efi_md_typeattr_format(char *buf, size_t size,
				     const efi_memory_desc_t *md)
{
	char *pos;
	int type_len;
	u64 attr;

	pos = buf;
	if (md->type >= ARRAY_SIZE(memory_type_name))
		type_len = snprintf(pos, size, "[type=%u", md->type);
	else
		type_len = snprintf(pos, size, "[%-*s",
				    (int)(sizeof(memory_type_name[0]) - 1),
				    memory_type_name[md->type]);
	if (type_len >= size)
		return buf;

	pos += type_len;
	size -= type_len;

	attr = md->attribute;
	if (attr & ~(EFI_MEMORY_UC | EFI_MEMORY_WC | EFI_MEMORY_WT |
		     EFI_MEMORY_WB | EFI_MEMORY_UCE | EFI_MEMORY_RO |
		     EFI_MEMORY_WP | EFI_MEMORY_RP | EFI_MEMORY_XP |
		     EFI_MEMORY_RUNTIME | EFI_MEMORY_MORE_RELIABLE))
		snprintf(pos, size, "|attr=0x%016llx]",
			 (unsigned long long)attr);
	else
		snprintf(pos, size, "|%3s|%2s|%2s|%2s|%2s|%2s|%3s|%2s|%2s|%2s|%2s]",
			 attr & EFI_MEMORY_RUNTIME ? "RUN" : "",
			 attr & EFI_MEMORY_MORE_RELIABLE ? "MR" : "",
			 attr & EFI_MEMORY_XP      ? "XP"  : "",
			 attr & EFI_MEMORY_RP      ? "RP"  : "",
			 attr & EFI_MEMORY_WP      ? "WP"  : "",
			 attr & EFI_MEMORY_RO      ? "RO"  : "",
			 attr & EFI_MEMORY_UCE     ? "UCE" : "",
			 attr & EFI_MEMORY_WB      ? "WB"  : "",
			 attr & EFI_MEMORY_WT      ? "WT"  : "",
			 attr & EFI_MEMORY_WC      ? "WC"  : "",
			 attr & EFI_MEMORY_UC      ? "UC"  : "");
	return buf;
}

/*
 * efi_mem_attributes - lookup memmap attributes for physical address
 * @phys_addr: the physical address to lookup
 *
 * Search in the EFI memory map for the region covering
 * @phys_addr. Returns the EFI memory attributes if the region
 * was found in the memory map, 0 otherwise.
 *
 * Despite being marked __weak, most architectures should *not*
 * override this function. It is __weak solely for the benefit
 * of ia64 which has a funky EFI memory map that doesn't work
 * the same way as other architectures.
 */
u64 __weak efi_mem_attributes(unsigned long phys_addr)
{
	struct efi_memory_map *map;
	efi_memory_desc_t *md;
	void *p;

	if (!efi_enabled(EFI_MEMMAP))
		return 0;

	map = efi.memmap;
	for (p = map->map; p < map->map_end; p += map->desc_size) {
		md = p;
		if ((md->phys_addr <= phys_addr) &&
		    (phys_addr < (md->phys_addr +
		    (md->num_pages << EFI_PAGE_SHIFT))))
			return md->attribute;
	}
	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Originally from efivars.c,
 *
 * Copyright (C) 2001,2003,2004 Dell <Matt_Domsch@dell.com>
 * Copyright (C) 2004 Intel Corporation <matthew.e.tolentino@intel.com>
 *
 * This code takes all variables accessible from EFI runtime and
 *  exports them via sysfs
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Changelog:
 *
 *  17 May 2004 - Matt Domsch <Matt_Domsch@dell.com>
 *   remove check for efi_enabled in exit
 *   add MODULE_VERSION
 *
 *  26 Apr 2004 - Matt Domsch <Matt_Domsch@dell.com>
 *   minor bug fixes
 *
 *  21 Apr 2004 - Matt Tolentino <matthew.e.tolentino@intel.com)
 *   converted driver to export variable information via sysfs
 *   and moved to drivers/firmware directory
 *   bumped revision number to v0.07 to reflect conversion & move
 *
 *  10 Dec 2002 - Matt Domsch <Matt_Domsch@dell.com>
 *   fix locking per Peter Chubb's findings
 *
 *  25 Mar 2002 - Matt Domsch <Matt_Domsch@dell.com>
 *   move uuid_unparse() to include/asm-ia64/efi.h:efi_guid_to_str()
 *
 *  12 Feb 2002 - Matt Domsch <Matt_Domsch@dell.com>
 *   use list_for_each_safe when deleting vars.
 *   remove ifdef CONFIG_SMP around include <linux/smp.h>
 *   v0.04 release to linux-ia64@linuxia64.org
 *
 *  20 April 2001 - Matt Domsch <Matt_Domsch@dell.com>
 *   Moved vars from /proc/efi to /proc/efi/vars, and made
 *   efi.c own the /proc/efi directory.
 *   v0.03 release to linux-ia64@linuxia64.org
 *
 *  26 March 2001 - Matt Domsch <Matt_Domsch@dell.com>
 *   At the request of Stephane, moved ownership of /proc/efi
 *   to efi.c, and now efivars lives under /proc/efi/vars.
 *
 *  12 March 2001 - Matt Domsch <Matt_Domsch@dell.com>
 *   Feedback received from Stephane Eranian incorporated.
 *   efivar_write() checks copy_from_user() return value.
 *   efivar_read/write() returns proper errno.
 *   v0.02 release to linux-ia64@linuxia64.org
 *
 *  26 February 2001 - Matt Domsch <Matt_Domsch@dell.com>
 *   v0.01 release to linux-ia64@linuxia64.org
 */

#include <linux/efi.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/ucs2_string.h>
#include <linux/compat.h>

#define EFIVARS_VERSION "0.08"
#define EFIVARS_DATE "2004-May-17"

MODULE_AUTHOR("Matt Domsch <Matt_Domsch@Dell.com>");
MODULE_DESCRIPTION("sysfs interface to EFI Variables");
MODULE_LICENSE("GPL");
MODULE_VERSION(EFIVARS_VERSION);
MODULE_ALIAS("platform:efivars");

LIST_HEAD(efivar_sysfs_list);
EXPORT_SYMBOL_GPL(efivar_sysfs_list);

static struct kset *efivars_kset;

static struct bin_attribute *efivars_new_var;
static struct bin_attribute *efivars_del_var;

struct compat_efi_variable {
	efi_char16_t  VariableName[EFI_VAR_NAME_LEN/sizeof(efi_char16_t)];
	efi_guid_t    VendorGuid;
	__u32         DataSize;
	__u8          Data[1024];
	__u32         Status;
	__u32         Attributes;
} __packed;

struct efivar_attribute {
	struct attribute attr;
	ssize_t (*show) (struct efivar_entry *entry, char *buf);
	ssize_t (*store)(struct efivar_entry *entry, const char *buf, size_t count);
};

#define EFIVAR_ATTR(_name, _mode, _show, _store) \
struct efivar_attribute efivar_attr_##_name = { \
	.attr = {.name = __stringify(_name), .mode = _mode}, \
	.show = _show, \
	.store = _store, \
};

#define to_efivar_attr(_attr) container_of(_attr, struct efivar_attribute, attr)
#define to_efivar_entry(obj)  container_of(obj, struct efivar_entry, kobj)

/*
 * Prototype for sysfs creation function
 */
static int
efivar_create_sysfs_entry(struct efivar_entry *new_var);

static ssize_t
efivar_guid_read(struct efivar_entry *entry, char *buf)
{
	struct efi_variable *var = &entry->var;
	char *str = buf;

	if (!entry || !buf)
		return 0;

	efi_guid_to_str(&var->VendorGuid, str);
	str += strlen(str);
	str += sprintf(str, "\n");

	return str - buf;
}

static ssize_t
efivar_attr_read(struct efivar_entry *entry, char *buf)
{
	struct efi_variable *var = &entry->var;
	unsigned long size = sizeof(var->Data);
	char *str = buf;
	int ret;

	if (!entry || !buf)
		return -EINVAL;

	ret = efivar_entry_get(entry, &var->Attributes, &size, var->Data);
	var->DataSize = size;
	if (ret)
		return -EIO;

	if (var->Attributes & EFI_VARIABLE_NON_VOLATILE)
		str += sprintf(str, "EFI_VARIABLE_NON_VOLATILE\n");
	if (var->Attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS)
		str += sprintf(str, "EFI_VARIABLE_BOOTSERVICE_ACCESS\n");
	if (var->Attributes & EFI_VARIABLE_RUNTIME_ACCESS)
		str += sprintf(str, "EFI_VARIABLE_RUNTIME_ACCESS\n");
	if (var->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
		str += sprintf(str, "EFI_VARIABLE_HARDWARE_ERROR_RECORD\n");
	if (var->Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
		str += sprintf(str,
			"EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS\n");
	if (var->Attributes &
			EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
		str += sprintf(str,
			"EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS\n");
	if (var->Attributes & EFI_VARIABLE_APPEND_WRITE)
		str += sprintf(str, "EFI_VARIABLE_APPEND_WRITE\n");
	return str - buf;
}

static ssize_t
efivar_size_read(struct efivar_entry *entry, char *buf)
{
	struct efi_variable *var = &entry->var;
	unsigned long size = sizeof(var->Data);
	char *str = buf;
	int ret;

	if (!entry || !buf)
		return -EINVAL;

	ret = efivar_entry_get(entry, &var->Attributes, &size, var->Data);
	var->DataSize = size;
	if (ret)
		return -EIO;

	str += sprintf(str, "0x%lx\n", var->DataSize);
	return str - buf;
}

static ssize_t
efivar_data_read(struct efivar_entry *entry, char *buf)
{
	struct efi_variable *var = &entry->var;
	unsigned long size = sizeof(var->Data);
	int ret;

	if (!entry || !buf)
		return -EINVAL;

	ret = efivar_entry_get(entry, &var->Attributes, &size, var->Data);
	var->DataSize = size;
	if (ret)
		return -EIO;

	memcpy(buf, var->Data, var->DataSize);
	return var->DataSize;
}

static inline int
sanity_check(struct efi_variable *var, efi_char16_t *name, efi_guid_t vendor,
	     unsigned long size, u32 attributes, u8 *data)
{
	/*
	 * If only updating the variable data, then the name
	 * and guid should remain the same
	 */
	if (memcmp(name, var->VariableName, sizeof(var->VariableName)) ||
		efi_guidcmp(vendor, var->VendorGuid)) {
		printk(KERN_ERR "efivars: Cannot edit the wrong variable!\n");
		return -EINVAL;
	}

	if ((size <= 0) || (attributes == 0)){
		printk(KERN_ERR "efivars: DataSize & Attributes must be valid!\n");
		return -EINVAL;
	}

	if ((attributes & ~EFI_VARIABLE_MASK) != 0 ||
	    efivar_validate(vendor, name, data, size) == false) {
		printk(KERN_ERR "efivars: Malformed variable content\n");
		return -EINVAL;
	}

	return 0;
}

static inline bool is_compat(void)
{
	if (IS_ENABLED(CONFIG_COMPAT) && is_compat_task())
		return true;

	return false;
}

static void
copy_out_compat(struct efi_variable *dst, struct compat_efi_variable *src)
{
	memcpy(dst->VariableName, src->VariableName, EFI_VAR_NAME_LEN);
	memcpy(dst->Data, src->Data, sizeof(src->Data));

	dst->VendorGuid = src->VendorGuid;
	dst->DataSize = src->DataSize;
	dst->Attributes = src->Attributes;
}

/*
 * We allow each variable to be edited via rewriting the
 * entire efi variable structure.
 */
static ssize_t
efivar_store_raw(struct efivar_entry *entry, const char *buf, size_t count)
{
	struct efi_variable *new_var, *var = &entry->var;
	efi_char16_t *name;
	unsigned long size;
	efi_guid_t vendor;
	u32 attributes;
	u8 *data;
	int err;

	if (!entry || !buf)
		return -EINVAL;

	if (is_compat()) {
		struct compat_efi_variable *compat;

		if (count != sizeof(*compat))
			return -EINVAL;

		compat = (struct compat_efi_variable *)buf;
		attributes = compat->Attributes;
		vendor = compat->VendorGuid;
		name = compat->VariableName;
		size = compat->DataSize;
		data = compat->Data;

		err = sanity_check(var, name, vendor, size, attributes, data);
		if (err)
			return err;

		copy_out_compat(&entry->var, compat);
	} else {
		if (count != sizeof(struct efi_variable))
			return -EINVAL;

		new_var = (struct efi_variable *)buf;

		attributes = new_var->Attributes;
		vendor = new_var->VendorGuid;
		name = new_var->VariableName;
		size = new_var->DataSize;
		data = new_var->Data;

		err = sanity_check(var, name, vendor, size, attributes, data);
		if (err)
			return err;

		memcpy(&entry->var, new_var, count);
	}

	err = efivar_entry_set(entry, attributes, size, data, NULL);
	if (err) {
		printk(KERN_WARNING "efivars: set_variable() failed: status=%d\n", err);
		return -EIO;
	}

	return count;
}

static ssize_t
efivar_show_raw(struct efivar_entry *entry, char *buf)
{
	struct efi_variable *var = &entry->var;
	struct compat_efi_variable *compat;
	unsigned long datasize = sizeof(var->Data);
	size_t size;
	int ret;

	if (!entry || !buf)
		return 0;

	ret = efivar_entry_get(entry, &var->Attributes, &datasize, var->Data);
	var->DataSize = datasize;
	if (ret)
		return -EIO;

	if (is_compat()) {
		compat = (struct compat_efi_variable *)buf;

		size = sizeof(*compat);
		memcpy(compat->VariableName, var->VariableName,
			EFI_VAR_NAME_LEN);
		memcpy(compat->Data, var->Data, sizeof(compat->Data));

		compat->VendorGuid = var->VendorGuid;
		compat->DataSize = var->DataSize;
		compat->Attributes = var->Attributes;
	} else {
		size = sizeof(*var);
		memcpy(buf, var, size);
	}

	return size;
}

/*
 * Generic read/write functions that call the specific functions of
 * the attributes...
 */
static ssize_t efivar_attr_show(struct kobject *kobj, struct attribute *attr,
				char *buf)
{
	struct efivar_entry *var = to_efivar_entry(kobj);
	struct efivar_attribute *efivar_attr = to_efivar_attr(attr);
	ssize_t ret = -EIO;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	if (efivar_attr->show) {
		ret = efivar_attr->show(var, buf);
	}
	return ret;
}

static ssize_t efivar_attr_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	struct efivar_entry *var = to_efivar_entry(kobj);
	struct efivar_attribute *efivar_attr = to_efivar_attr(attr);
	ssize_t ret = -EIO;

	if (!capable(CAP_SYS_ADMIN))
		return -E