))
			return -EINVAL;

		attributes = new_var->Attributes;
		name = new_var->VariableName;
		size = new_var->DataSize;
		data = new_var->Data;
	}

	if ((attributes & ~EFI_VARIABLE_MASK) != 0 ||
	    efivar_validate(new_var->VendorGuid, name, data,
			    size) == false) {
		printk(KERN_ERR "efivars: Malformed variable content\n");
		return -EINVAL;
	}

	new_entry = kzalloc(sizeof(*new_entry), GFP_KERNEL);
	if (!new_entry)
		return -ENOMEM;

	if (need_compat)
		copy_out_compat(&new_entry->var, compat);
	else
		memcpy(&new_entry->var, new_var, sizeof(*new_var));

	err = efivar_entry_set(new_entry, attributes, size,
			       data, &efivar_sysfs_list);
	if (err) {
		if (err == -EEXIST)
			err = -EINVAL;
		goto out;
	}

	if (efivar_create_sysfs_entry(new_entry)) {
		printk(KERN_WARNING "efivars: failed to create sysfs entry.\n");
		kfree(new_entry);
	}
	return count;

out:
	kfree(new_entry);
	return err;
}

static ssize_t efivar_delete(struct file *filp, struct kobject *kobj,
			     struct bin_attribute *bin_attr,
			     char *buf, loff_t pos, size_t count)
{
	struct efi_variable *del_var = (struct efi_variable *)buf;
	struct compat_efi_variable *compat;
	struct efivar_entry *entry;
	efi_char16_t *name;
	efi_guid_t vendor;
	int err = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	if (is_compat()) {
		if (count != sizeof(*compat))
			return -EINVAL;

		compat = (struct compat_efi_variable *)buf;
		name = compat->VariableName;
		vendor = compat->VendorGuid;
	} else {
		if (count != sizeof(*del_var))
			return -EINVAL;

		name = del_var->VariableName;
		vendor = del_var->VendorGuid;
	}

	efivar_entry_iter_begin();
	entry = efivar_entry_find(name, vendor, &efivar_sysfs_list, true);
	if (!entry)
		err = -EINVAL;
	else if (__efivar_entry_delete(entry))
		err = -EIO;

	if (err) {
		efivar_entry_iter_end();
		return err;
	}

	if (!entry->scanning) {
		efivar_entry_iter_end();
		efivar_unregister(entry);
	} else
		efivar_entry_iter_end();

	/* It's dead Jim.... */
	return count;
}

/**
 * efivar_create_sysfs_entry - create a new entry in sysfs
 * @new_var: efivar entry to create
 *
 * Returns 0 on success, negative error code on failure
 */
static int
efivar_create_sysfs_entry(struct efivar_entry *new_var)
{
	int short_name_size;
	char *short_name;
	unsigned long utf8_name_size;
	efi_char16_t *variable_name = new_var->var.VariableName;
	int ret;

	/*
	 * Length of the variable bytes in UTF8, plus the '-' separator,
	 * plus the GUID, plus trailing NUL
	 */
	utf8_name_size = ucs2_utf8size(variable_name);
	short_name_size = utf8_name_size + 1 + EFI_VARIABLE_GUID_LEN + 1;

	short_name = kmalloc(short_name_size, GFP_KERNEL);
	if (!short_name)
		return -ENOMEM;

	ucs2_as_utf8(short_name, variable_name, short_name_size);

	/* This is ugly, but necessary to separate one vendor's
	   private variables from another's.         */
	short_name[utf8_name_size] = '-';
	efi_guid_to_str(&new_var->var.VendorGuid,
			 short_name + utf8_name_size + 1);

	new_var->kobj.kset = efivars_kset;

	ret = kobject_init_and_add(&new_var->kobj, &efivar_ktype,
				   NULL, "%s", short_name);
	kfree(short_name);
	if (ret) {
		kobject_put(&new_var->kobj);
		return ret;
	}

	kobject_uevent(&new_var->kobj, KOBJ_ADD);
	efivar_entry_add(new_var, &efivar_sysfs_list);

	return 0;
}

static int
create_efivars_bin_attributes(void)
{
	struct bin_attribute *attr;
	int error;

	/* new_var */
	attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (!attr)
		return -ENOMEM;

	attr->attr.name = "new_var";
	attr->attr.mode = 0200;
	attr->write = efivar_create;
	efivars_new_var = attr;

	/* del_var */
	attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (!attr) {
		error = -ENOMEM;
		goto out_free;
	}
	attr->attr.name = "del_var";
	attr->attr.mode = 0200;
	attr->write = efivar_delete;
	efivars_del_var = attr;

	sysfs_bin_attr_init(efivars_new_var);
	sysfs_bin_attr_init(efivars_del_var);

	/* Register */
	error = sysfs_create_bin_file(&efivars_kset->kobj, efivars_new_var);
	if (error) {
		printk(KERN_ERR "efivars: unable to create new_var sysfs file"
			" due to error %d\n", error);
		goto out_free;
	}

	error = sysfs_create_bin_file(&efivars_kset->kobj, efivars_del_var);
	if (error) {
		printk(KERN_ERR "efivars: unable to create del_var sysfs file"
			" due to error %d\n", error);
		sysfs_remove_bin_file(&efivars_kset->kobj, efivars_new_var);
		goto out_free;
	}

	return 0;
out_free:
	kfree(efivars_del_var);
	efivars_del_var = NULL;
	kfree(efivars_new_var);
	efivars_new_var = NULL;
	return error;
}

static int efivar_update_sysfs_entry(efi_char16_t *name, efi_guid_t vendor,
				     unsigned long name_size, void *data)
{
	struct efivar_entry *entry = data;

	if (efivar_entry_find(name, vendor, &efivar_sysfs_list, false))
		return 0;

	memcpy(entry->var.VariableName, name, name_size);
	memcpy(&(entry->var.VendorGuid), &vendor, sizeof(efi_guid_t));

	return 1;
}

static void efivar_update_sysfs_entries(struct work_struct *work)
{
	struct efivar_entry *entry;
	int err;

	/* Add new sysfs entries */
	while (1) {
		entry = kzalloc(sizeof(*entry), GFP_KERNEL);
		if (!entry)
			return;

		err = efivar_init(efivar_update_sysfs_entry, entry,
				  true, false, &efivar_sysfs_list);
		if (!err)
			break;

		efivar_create_sysfs_entry(entry);
	}

	kfree(entry);
}

static int efivars_sysfs_callback(efi_char16_t *name, efi_guid_t vendor,
				  unsigned long name_size, void *data)
{
	struct efivar_entry *entry;

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	memcpy(entry->var.VariableName, name, name_size);
	memcpy(&(entry->var.VendorGuid), &vendor, sizeof(efi_guid_t));

	efivar_create_sysfs_entry(entry);

	return 0;
}

static int efivar_sysfs_destroy(struct efivar_entry *entry, void *data)
{
	efivar_entry_remove(entry);
	efivar_unregister(entry);
	return 0;
}

static void efivars_sysfs_exit(void)
{
	/* Remove all entries and destroy */
	__efivar_entry_iter(efivar_sysfs_destroy, &efivar_sysfs_list, NULL, NULL);

	if (efivars_new_var)
		sysfs_remove_bin_file(&efivars_kset->kobj, efivars_new_var);
	if (efivars_del_var)
		sysfs_remove_bin_file(&efivars_kset->kobj, efivars_del_var);
	kfree(efivars_new_var);
	kfree(efivars_del_var);
	kset_unregister(efivars_kset);
}

int efivars_sysfs_init(void)
{
	struct kobject *parent_kobj = efivars_kobject();
	int error = 0;

	if (!efi_enabled(EFI_RUNTIME_SERVICES))
		return -ENODEV;

	/* No efivars has been registered yet */
	if (!parent_kobj)
		return 0;

	printk(KERN_INFO "EFI Variables Facility v%s %s\n", EFIVARS_VERSION,
	       EFIVARS_DATE);

	efivars_kset = kset_create_and_add("vars", NULL, parent_kobj);
	if (!efivars_kset) {
		printk(KERN_ERR "efivars: Subsystem registration failed.\n");
		return -ENOMEM;
	}

	efivar_init(efivars_sysfs_callback, NULL, false,
		    true, &efivar_sysfs_list);

	error = create_efivars_bin_attributes();
	if (error) {
		efivars_sysfs_exit();
		return error;
	}

	INIT_WORK(&efivar_work, efivar_update_sysfs_entries);

	return 0;
}
EXPORT_SYMBOL_GPL(efivars_sysfs_init);

module_init(efivars_sysfs_init);
module_exit(efivars_sysfs_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * esrt.c
 *
 * This module exports EFI System Resource Table (ESRT) entries into userspace
 * through the sysfs file system. The ESRT provides a read-only catalog of
 * system components for which the system accepts firmware upgrades via UEFI's
 * "Capsule Update" feature. This module allows userland utilities to evaluate
 * what firmware updates can be applied to this system, and potentially arrange
 * for those updates to occur.
 *
 * Data is currently found below /sys/firmware/efi/esrt/...
 */
#define pr_fmt(fmt) "esrt: " fmt

#include <linux/capability.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/memblock.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <asm/io.h>
#include <asm/early_ioremap.h>

struct efi_system_resource_entry_v1 {
	efi_guid_t	fw_class;
	u32		fw_type;
	u32		fw_version;
	u32		lowest_supported_fw_version;
	u32		capsule_flags;
	u32		last_attempt_version;
	u32		last_attempt_status;
};

/*
 * _count and _version are what they seem like.  _max is actually just
 * accounting info for the firmware when creating the table; it should never
 * have been exposed to us.  To wit, the spec says:
 * The maximum number of resource array entries that can be within the
 * table without reallocating the table, must not be zero.
 * Since there's no guidance about what that means in terms of memory layout,
 * it means nothing to us.
 */
struct efi_system_resource_table {
	u32	fw_resource_count;
	u32	fw_resource_count_max;
	u64	fw_resource_version;
	u8	entries[];
};

static phys_addr_t esrt_data;
static size_t esrt_data_size;

static struct efi_system_resource_table *esrt;

struct esre_entry {
	union {
		struct efi_system_resource_entry_v1 *esre1;
	} esre;

	struct kobject kobj;
	struct list_head list;
};

/* global list of esre_entry. */
static LIST_HEAD(entry_list);

/* entry attribute */
struct esre_attribute {
	struct attribute attr;
	ssize_t (*show)(struct esre_entry *entry, char *buf);
	ssize_t (*store)(struct esre_entry *entry,
			 const char *buf, size_t count);
};

static struct esre_entry *to_entry(struct kobject *kobj)
{
	return container_of(kobj, struct esre_entry, kobj);
}

static struct esre_attribute *to_attr(struct attribute *attr)
{
	return container_of(attr, struct esre_attribute, attr);
}

static ssize_t esre_attr_show(struct kobject *kobj,
			      struct attribute *_attr, char *buf)
{
	struct esre_entry *entry = to_entry(kobj);
	struct esre_attribute *attr = to_attr(_attr);

	/* Don't tell normal users what firmware versions we've got... */
	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	return attr->show(entry, buf);
}

static const struct sysfs_ops esre_attr_ops = {
	.show = esre_attr_show,
};

/* Generic ESRT Entry ("ESRE") support. */
static ssize_t fw_class_show(struct esre_entry *entry, char *buf)
{
	char *str = buf;

	efi_guid_to_str(&entry->esre.esre1->fw_class, str);
	str += strlen(str);
	str += sprintf(str, "\n");

	return str - buf;
}

static struct esre_attribute esre_fw_class = __ATTR_RO_MODE(fw_class, 0400);

#define esre_attr_decl(name, size, fmt) \
static ssize_t name##_show(struct esre_entry *entry, char *buf) \
{ \
	return sprintf(buf, fmt "\n", \
		       le##size##_to_cpu(entry->esre.esre1->name)); \
} \
\
static struct esre_attribute esre_##name = __ATTR_RO_MODE(name, 0400)

esre_attr_decl(fw_type, 32, "%u");
esre_attr_decl(fw_version, 32, "%u");
esre_attr_decl(lowest_supported_fw_version, 32, "%u");
esre_attr_decl(capsule_flags, 32, "0x%x");
esre_attr_decl(last_attempt_version, 32, "%u");
esre_attr_decl(last_attempt_status, 32, "%u");

static struct attribute *esre1_attrs[] = {
	&esre_fw_class.attr,
	&esre_fw_type.attr,
	&esre_fw_version.attr,
	&esre_lowest_supported_fw_version.attr,
	&esre_capsule_flags.attr,
	&esre_last_attempt_version.attr,
	&esre_last_attempt_status.attr,
	NULL
};
static void esre_release(struct kobject *kobj)
{
	struct esre_entry *entry = to_entry(kobj);

	list_del(&entry->list);
	kfree(entry);
}

static struct kobj_type esre1_ktype = {
	.release = esre_release,
	.sysfs_ops = &esre_attr_ops,
	.default_attrs = esre1_attrs,
};


static struct kobject *esrt_kobj;
static struct kset *esrt_kset;

static int esre_create_sysfs_entry(void *esre, int entry_num)
{
	struct esre_entry *entry;
	char name[20];

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	sprintf(name, "entry%d", entry_num);

	entry->kobj.kset = esrt_kset;

	if (esrt->fw_resource_version == 1) {
		int rc = 0;

		entry->esre.esre1 = esre;
		rc = kobject_init_and_add(&entry->kobj, &esre1_ktype, NULL,
					  "%s", name);
		if (rc) {
			kobject_put(&entry->kobj);
			return rc;
		}
	}

	list_add_tail(&entry->list, &entry_list);
	return 0;
}

/* support for displaying ESRT fields at the top level */
#define esrt_attr_decl(name, size, fmt) \
static ssize_t name##_show(struct kobject *kobj, \
				  struct kobj_attribute *attr, char *buf)\
{ \
	return sprintf(buf, fmt "\n", le##size##_to_cpu(esrt->name)); \
} \
\
static struct kobj_attribute esrt_##name = __ATTR_RO_MODE(name, 0400)

esrt_attr_decl(fw_resource_count, 32, "%u");
esrt_attr_decl(fw_resource_count_max, 32, "%u");
esrt_attr_decl(fw_resource_version, 64, "%llu");

static struct attribute *esrt_attrs[] = {
	&esrt_fw_resource_count.attr,
	&esrt_fw_resource_count_max.attr,
	&esrt_fw_resource_version.attr,
	NULL,
};

static inline int esrt_table_exists(void)
{
	if (!efi_enabled(EFI_CONFIG_TABLES))
		return 0;
	if (efi.esrt == EFI_INVALID_TABLE_ADDR)
		return 0;
	return 1;
}

static umode_t esrt_attr_is_visible(struct kobject *kobj,
				    struct attribute *attr, int n)
{
	if (!esrt_table_exists())
		return 0;
	return attr->mode;
}

static struct attribute_group esrt_attr_group = {
	.attrs = esrt_attrs,
	.is_visible = esrt_attr_is_visible,
};

/*
 * remap the table, copy it to kmalloced pages, and unmap it.
 */
void __init efi_esrt_init(void)
{
	void *va;
	struct efi_system_resource_table tmpesrt;
	struct efi_system_resource_entry_v1 *v1_entries;
	size_t size, max, entry_size, entries_size;
	efi_memory_desc_t md;
	int rc;
	phys_addr_t end;

	pr_debug("esrt-init: loading.\n");
	if (!esrt_table_exists())
		return;

	rc = efi_mem_desc_lookup(efi.esrt, &md);
	if (rc < 0) {
		pr_warn("ESRT header is not in the memory map.\n");
		return;
	}

	max = efi_mem_desc_end(&md);
	if (max < efi.esrt) {
		pr_err("EFI memory descriptor is invalid. (esrt: %p max: %p)\n",
		       (void *)efi.esrt, (void *)max);
		return;
	}

	size = sizeof(*esrt);
	max -= efi.esrt;

	if (max < size) {
		pr_err("ESRT header doen't fit on single memory map entry. (size: %zu max: %zu)\n",
		       size, max);
		return;
	}

	va = early_memremap(efi.esrt, size);
	if (!va) {
		pr_err("early_memremap(%p, %zu) failed.\n", (void *)efi.esrt,
		       size);
		return;
	}

	memcpy(&tmpesrt, va, sizeof(tmpesrt));

	if (tmpesrt.fw_resource_version == 1) {
		entry_size = sizeof (*v1_entries);
	} else {
		pr_err("Unsupported ESRT version %lld.\n",
		       tmpesrt.fw_resource_version);
		return;
	}

	if (tmpesrt.fw_resource_count > 0 && max - size < entry_size) {
		pr_err("ESRT memory map entry can only hold the header. (max: %zu size: %zu)\n",
		       max - size, entry_size);
		goto err_memunmap;
	}

	/*
	 * The format doesn't really give us any boundary to test here,
	 * so I'm making up 128 as the max number of individually updatable
	 * components we support.
	 * 128 should be pretty excessive, but there's still some chance
	 * somebody will do that someday and we'll need to raise this.
	 */
	if (tmpesrt.fw_resource_count > 128) {
		pr_err("ESRT says fw_resource_count has very large value %d.\n",
		       tmpesrt.fw_resource_count);
		goto err_memunmap;
	}

	/*
	 * We know it can't be larger than N * sizeof() here, and N is limited
	 * by the previous test to a small number, so there's no overflow.
	 */
	entries_size = tmpesrt.fw_resource_count * entry_size;
	if (max < size + entries_size) {
		pr_err("ESRT does not fit on single memory map entry (size: %zu max: %zu)\n",
		       size, max);
		goto err_memunmap;
	}

	/* remap it with our (plausible) new pages */
	early_memunmap(va, size);
	size += entries_size;
	va = early_memremap(efi.esrt, size);
	if (!va) {
		pr_err("early_memremap(%p, %zu) failed.\n", (void *)efi.esrt,
		       size);
		return;
	}

	esrt_data = (phys_addr_t)efi.esrt;
	esrt_data_size = size;

	end = esrt_data + size;
	pr_info("Reserving ESRT space from %pa to %pa.\n", &esrt_data, &end);
	memblock_reserve(esrt_data, esrt_data_size);

	pr_debug("esrt-init: loaded.\n");
err_memunmap:
	early_memunmap(va, size);
}

static int __init register_entries(void)
{
	struct efi_system_resource_entry_v1 *v1_entries = (void *)esrt->entries;
	int i, rc;

	if (!esrt_table_exists())
		return 0;

	for (i = 0; i < le32_to_cpu(esrt->fw_resource_count); i++) {
		void *esre = NULL;
		if (esrt->fw_resource_version == 1) {
			esre = &v1_entries[i];
		} else {
			pr_err("Unsupported ESRT version %lld.\n",
			       esrt->fw_resource_version);
			return -EINVAL;
		}

		rc = esre_create_sysfs_entry(esre, i);
		if (rc < 0) {
			pr_err("ESRT entry creation failed with error %d.\n",
			       rc);
			return rc;
		}
	}
	return 0;
}

static void cleanup_entry_list(void)
{
	struct esre_entry *entry, *next;

	list_for_each_entry_safe(entry, next, &entry_list, list) {
		kobject_put(&entry->kobj);
	}
}

static int __init esrt_sysfs_init(void)
{
	int error;
	struct efi_system_resource_table __iomem *ioesrt;

	pr_debug("esrt-sysfs: loading.\n");
	if (!esrt_data || !esrt_data_size)
		return -ENOSYS;

	ioesrt = ioremap(esrt_data, esrt_data_size);
	if (!ioesrt) {
		pr_err("ioremap(%pa, %zu) failed.\n", &esrt_data,
		       esrt_data_size);
		return -ENOMEM;
	}

	esrt = kmalloc(esrt_data_size, GFP_KERNEL);
	if (!esrt) {
		pr_err("kmalloc failed. (wanted %zu bytes)\n", esrt_data_size);
		iounmap(ioesrt);
		return -ENOMEM;
	}

	memcpy_fromio(esrt, ioesrt, esrt_data_size);

	esrt_kobj = kobject_create_and_add("esrt", efi_kobj);
	if (!esrt_kobj) {
		pr_err("Firmware table registration failed.\n");
		error = -ENOMEM;
		goto err;
	}

	error = sysfs_create_group(esrt_kobj, &esrt_attr_group);
	if (error) {
		pr_err("Sysfs attribute export failed with error %d.\n",
		       error);
		goto err_remove_esrt;
	}

	esrt_kset = kset_create_and_add("entries", NULL, esrt_kobj);
	if (!esrt_kset) {
		pr_err("kset creation failed.\n");
		error = -ENOMEM;
		goto err_remove_group;
	}

	error = register_entries();
	if (error)
		goto err_cleanup_list;

	memblock_remove(esrt_data, esrt_data_size);

	pr_debug("esrt-sysfs: loaded.\n");

	return 0;
err_cleanup_list:
	cleanup_entry_list();
	kset_unregister(esrt_kset);
err_remove_group:
	sysfs_remove_group(esrt_kobj, &esrt_attr_group);
err_remove_esrt:
	kobject_put(esrt_kobj);
err:
	kfree(esrt);
	esrt = NULL;
	return error;
}
device_initcall(esrt_sysfs_init);

/*
MODULE_AUTHOR("Peter Jones <pjones@redhat.com>");
MODULE_DESCRIPTION("EFI System Resource Table support");
MODULE_LICENSE("GPL");
*/
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 * fake_mem.c
 *
 * Copyright (C) 2015 FUJITSU LIMITED
 * Author: Taku Izumi <izumi.taku@jp.fujitsu.com>
 *
 * This code introduces new boot option named "efi_fake_mem"
 * By specifying this parameter, you can add arbitrary attribute to
 * specific memory range by updating original (firmware provided) EFI
 * memmap.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms and conditions of the GNU General Public License,
 *  version 2, as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *  The full GNU General Public License is included in this distribution in
 *  the file called "COPYING".
 */

#include <linux/kernel.h>
#include <linux/efi.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/types.h>
#include <linux/sort.h>
#include <asm/efi.h>

#define EFI_MAX_FAKEMEM CONFIG_EFI_MAX_FAKE_MEM

struct fake_mem {
	struct range range;
	u64 attribute;
};
static struct fake_mem fake_mems[EFI_MAX_FAKEMEM];
static int nr_fake_mem;

static int __init cmp_fake_mem(const void *x1, const void *x2)
{
	const struct fake_mem *m1 = x1;
	const struct fake_mem *m2 = x2;

	if (m1->range.start < m2->range.start)
		return -1;
	if (m1->range.start > m2->range.start)
		return 1;
	return 0;
}

void __init efi_fake_memmap(void)
{
	u64 start, end, m_start, m_end, m_attr;
	int new_nr_map = memmap.nr_map;
	efi_memory_desc_t *md;
	phys_addr_t new_memmap_phy;
	void *new_memmap;
	void *old, *new;
	int i;

	if (!nr_fake_mem || !efi_enabled(EFI_MEMMAP))
		return;

	/* count up the number of EFI memory descriptor */
	for (old = memmap.map; old < memmap.map_end; old += memmap.desc_size) {
		md = old;
		start = md->phys_addr;
		end = start + (md->num_pages << EFI_PAGE_SHIFT) - 1;

		for (i = 0; i < nr_fake_mem; i++) {
			/* modifying range */
			m_start = fake_mems[i].range.start;
			m_end = fake_mems[i].range.end;

			if (m_start <= start) {
				/* split into 2 parts */
				if (start < m_end && m_end < end)
					new_nr_map++;
			}
			if (start < m_start && m_start < end) {
				/* split into 3 parts */
				if (m_end < end)
					new_nr_map += 2;
				/* split into 2 parts */
				if (end <= m_end)
					new_nr_map++;
			}
		}
	}

	/* allocate memory for new EFI memmap */
	new_memmap_phy = memblock_alloc(memmap.desc_size * new_nr_map,
					PAGE_SIZE);
	if (!new_memmap_phy)
		return;

	/* create new EFI memmap */
	new_memmap = early_memremap(new_memmap_phy,
				    memmap.desc_size * new_nr_map);
	if (!new_memmap) {
		memblock_free(new_memmap_phy, memmap.desc_size * new_nr_map);
		return;
	}

	for (old = memmap.map, new = new_memmap;
	     old < memmap.map_end;
	     old += memmap.desc_size, new += memmap.desc_size) {

		/* copy original EFI memory descriptor */
		memcpy(new, old, memmap.desc_size);
		md = new;
		start = md->phys_addr;
		end = md->phys_addr + (md->num_pages << EFI_PAGE_SHIFT) - 1;

		for (i = 0; i < nr_fake_mem; i++) {
			/* modifying range */
			m_start = fake_mems[i].range.start;
			m_end = fake_mems[i].range.end;
			m_attr = fake_mems[i].attribute;

			if (m_start <= start && end <= m_end)
				md->attribute |= m_attr;

			if (m_start <= start &&
			    (start < m_end && m_end < end)) {
				/* first part */
				md->attribute |= m_attr;
				md->num_pages = (m_end - md->phys_addr + 1) >>
					EFI_PAGE_SHIFT;
				/* latter part */
				new += memmap.desc_size;
				memcpy(new, old, memmap.desc_size);
				md = new;
				md->phys_addr = m_end + 1;
				md->num_pages = (end - md->phys_addr + 1) >>
					EFI_PAGE_SHIFT;
			}

			if ((start < m_start && m_start < end) && m_end < end) {
				/* first part */
				md->num_pages = (m_start - md->phys_addr) >>
					EFI_PAGE_SHIFT;
				/* middle part */
				new += memmap.desc_size;
				memcpy(new, old, memmap.desc_size);
				md = new;
				md->attribute |= m_attr;
				md->phys_addr = m_start;
				md->num_pages = (m_end - m_start + 1) >>
					EFI_PAGE_SHIFT;
				/* last part */
				new += memmap.desc_size;
				memcpy(new, old, memmap.desc_size);
				md = new;
				md->phys_addr = m_end + 1;
				md->num_pages = (end - m_end) >>
					EFI_PAGE_SHIFT;
			}

			if ((start < m_start && m_start < end) &&
			    (end <= m_end)) {
				/* first part */
				md->num_pages = (m_start - md->phys_addr) >>
					EFI_PAGE_SHIFT;
				/* latter part */
				new += memmap.desc_size;
				memcpy(new, old, memmap.desc_size);
				md = new;
				md->phys_addr = m_start;
				md->num_pages = (end - md->phys_addr + 1) >>
					EFI_PAGE_SHIFT;
				md->attribute |= m_attr;
			}
		}
	}

	/* swap into new EFI memmap */
	efi_unmap_memmap();
	memmap.map = new_memmap;
	memmap.phys_map = new_memmap_phy;
	memmap.nr_map = new_nr_map;
	memmap.map_end = memmap.map + memmap.nr_map * memmap.desc_size;
	set_bit(EFI_MEMMAP, &efi.flags);

	/* print new EFI memmap */
	efi_print_memmap();
}

static int __init setup_fake_mem(char *p)
{
	u64 start = 0, mem_size = 0, attribute = 0;
	int i;

	if (!p)
		return -EINVAL;

	while (*p != '\0') {
		mem_size = memparse(p, &p);
		if (*p == '@')
			start = memparse(p+1, &p);
		else
			break;

		if (*p == ':')
			attribute = simple_strtoull(p+1, &p, 0);
		else
			break;

		if (nr_fake_mem >= EFI_MAX_FAKEMEM)
			break;

		fake_mems[nr_fake_mem].range.start = start;
		fake_mems[nr_fake_mem].range.end = start + mem_size - 1;
		fake_mems[nr_fake_mem].attribute = attribute;
		nr_fake_mem++;

		if (*p == ',')
			p++;
	}

	sort(fake_mems, nr_fake_mem, sizeof(struct fake_mem),
	     cmp_fake_mem, NULL);

	for (i = 0; i < nr_fake_mem; i++)
		pr_info("efi_fake_mem: add attr=0x%016llx to [mem 0x%016llx-0x%016llx]",
			fake_mems[i].attribute, fake_mems[i].range.start,
			fake_mems[i].range.end);

	return *p == '\0' ? 0 : -EINVAL;
}

early_param("efi_fake_mem", setup_fake_mem);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        #
# The stub may be linked into the kernel proper or into a separate boot binary,
# but in either case, it executes before the kernel does (with MMU disabled) so
# things like ftrace and stack-protector are likely to cause trouble if left
# enabled, even if doing so doesn't break the build.
#
cflags-$(CONFIG_X86_32)		:= -march=i386
cflags-$(CONFIG_X86_64)		:= -mcmodel=small
cflags-$(CONFIG_X86)		+= -m$(BITS) -D__KERNEL__ $(LINUX_INCLUDE) -O2 \
				   -fPIC -fno-strict-aliasing -mno-red-zone \
				   -mno-mmx -mno-sse

cflags-$(CONFIG_ARM64)		:= $(subst -pg,,$(KBUILD_CFLAGS)) -fpie
cflags-$(CONFIG_ARM)		:= $(subst -pg,,$(KBUILD_CFLAGS)) \
				   -fno-builtin -fpic -mno-single-pic-base

cflags-$(CONFIG_EFI_ARMSTUB)	+= -I$(srctree)/scripts/dtc/libfdt

KBUILD_CFLAGS			:= $(cflags-y) -DDISABLE_BRANCH_PROFILING \
				   $(call cc-option,-ffreestanding) \
				   $(call cc-option,-fno-stack-protector) \
				   $(DISABLE_LTO)

GCOV_PROFILE			:= n
KASAN_SANITIZE			:= n

# Prevents link failures: __sanitizer_cov_trace_pc() is not linked in.
KCOV_INSTRUMENT			:= n

lib-y				:= efi-stub-helper.o

# include the stub's generic dependencies from lib/ when building for ARM/arm64
arm-deps := fdt_rw.c fdt_ro.c fdt_wip.c fdt.c fdt_empty_tree.c fdt_sw.c sort.c

$(obj)/lib-%.o: $(srctree)/lib/%.c FORCE
	$(call if_changed_rule,cc_o_c)

lib-$(CONFIG_EFI_ARMSTUB)	+= arm-stub.o fdt.o string.o \
				   $(patsubst %.c,lib-%.o,$(arm-deps))

lib-$(CONFIG_ARM64)		+= arm64-stub.o random.o
CFLAGS_arm64-stub.o 		:= -DTEXT_OFFSET=$(TEXT_OFFSET)

#
# arm64 puts the stub in the kernel proper, which will unnecessarily retain all
# code indefinitely unless it is annotated as __init/__initdata/__initconst etc.
# So let's apply the __init annotations at the section level, by prefixing
# the section names directly. This will ensure that even all the inline string
# literals are covered.
# The fact that the stub and the kernel proper are essentially the same binary
# also means that we need to be extra careful to make sure that the stub does
# not rely on any absolute symbol references, considering that the virtual
# kernel mapping that the linker uses is not active yet when the stub is
# executing. So build all C dependencies of the EFI stub into libstub, and do
# a verification pass to see if any absolute relocations exist in any of the
# object files.
#
extra-$(CONFIG_EFI_ARMSTUB)	:= $(lib-y)
lib-$(CONFIG_EFI_ARMSTUB)	:= $(patsubst %.o,%.stub.o,$(lib-y))

STUBCOPY_FLAGS-y		:= -R .debug* -R *ksymtab* -R *kcrctab*
STUBCOPY_FLAGS-$(CONFIG_ARM64)	+= --prefix-alloc-sections=.init \
				   --prefix-symbols=__efistub_
STUBCOPY_RELOC-$(CONFIG_ARM64)	:= R_AARCH64_ABS

$(obj)/%.stub.o: $(obj)/%.o FORCE
	$(call if_changed,stubcopy)

quiet_cmd_stubcopy = STUBCPY $@
      cmd_stubcopy = if $(OBJCOPY) $(STUBCOPY_FLAGS-y) $< $@; then	\
		     $(OBJDUMP) -r $@ | grep $(STUBCOPY_RELOC-y)	\
		     && (echo >&2 "$@: absolute symbol references not allowed in the EFI stub"; \
			 rm -f $@; /bin/false); else /bin/false; fi
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * EFI stub implementation that is shared by arm and arm64 architectures.
 * This should be #included by the EFI stub implementation files.
 *
 * Copyright (C) 2013,2014 Linaro Limited
 *     Roy Franz <roy.franz@linaro.org
 * Copyright (C) 2013 Red Hat, Inc.
 *     Mark Salter <msalter@redhat.com>
 *
 * This file is part of the Linux kernel, and is made available under the
 * terms of the GNU General Public License version 2.
 *
 */

#include <linux/efi.h>
#include <linux/sort.h>
#include <asm/efi.h>

#include "efistub.h"

bool efi__get___nokaslr(void)
{
	return nokaslr();
}

static int efi_secureboot_enabled(efi_system_table_t *sys_table_arg)
{
	static efi_guid_t const var_guid = EFI_GLOBAL_VARIABLE_GUID;
	static efi_char16_t const var_name[] = {
		'S', 'e', 'c', 'u', 'r', 'e', 'B', 'o', 'o', 't', 0 };

	efi_get_variable_t *f_getvar = sys_table_arg->runtime->get_variable;
	unsigned long size = sizeof(u8);
	efi_status_t status;
	u8 val;

	status = f_getvar((efi_char16_t *)var_name, (efi_guid_t *)&var_guid,
			  NULL, &size, &val);

	switch (status) {
	case EFI_SUCCESS:
		return val;
	case EFI_NOT_FOUND:
		return 0;
	default:
		return 1;
	}
}

efi_status_t efi_open_volume(efi_system_table_t *sys_table_arg,
			     void *__image, void **__fh)
{
	efi_file_io_interface_t *io;
	efi_loaded_image_t *image = __image;
	efi_file_handle_t *fh;
	efi_guid_t fs_proto = EFI_FILE_SYSTEM_GUID;
	efi_status_t status;
	void *handle = (void *)(unsigned long)image->device_handle;

	status = sys_table_arg->boottime->handle_protocol(handle,
				 &fs_proto, (void **)&io);
	if (status != EFI_SUCCESS) {
		efi_printk(sys_table_arg, "Failed to handle fs_proto\n");
		return status;
	}

	status = io->open_volume(io, &fh);
	if (status != EFI_SUCCESS)
		efi_printk(sys_table_arg, "Failed to open volume\n");

	*__fh = fh;
	return status;
}

efi_status_t efi_file_close(void *handle)
{
	efi_file_handle_t *fh = handle;

	return fh->close(handle);
}

efi_status_t
efi_file_read(void *handle, unsigned long *size, void *addr)
{
	efi_file_handle_t *fh = handle;

	return fh->read(handle, size, addr);
}


efi_status_t
efi_file_size(efi_system_table_t *sys_table_arg, void *__fh,
	      efi_cha