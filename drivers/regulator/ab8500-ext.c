kip adding this kobject, but exit with non-fatal error. */
		rc = 0;
		goto free_ibft_obj;
	}

	if (hdr->id == id_nic) {
		/*
		* We don't search for the device in other domains than
		* zero. This is because on x86 platforms the BIOS
		* executes only devices which are in domain 0. Furthermore, the
		* iBFT spec doesn't have a domain id field :-(
		*/
		pci_dev = pci_get_bus_and_slot((nic->pci_bdf & 0xff00) >> 8,
					       (nic->pci_bdf & 0xff));
		if (pci_dev) {
			rc = sysfs_create_link(&boot_kobj->kobj,
					       &pci_dev->dev.kobj, "device");
			pci_dev_put(pci_dev);
		}
	}
	return 0;

free_ibft_obj:
	kfree(ibft_kobj);
	return rc;
}

/*
 * Scan the IBFT table structure for the NIC and Target fields. When
 * found add them on the passed-in list. We do not support the other
 * fields at this point, so they are skipped.
 */
static int __init ibft_register_kobjects(struct acpi_table_ibft *header)
{
	struct ibft_control *control = NULL;
	void *ptr, *end;
	int rc = 0;
	u16 offset;
	u16 eot_offset;

	control = (void *)header + sizeof(*header);
	end = (void *)control + control->hdr.length;
	eot_offset = (void *)header + header->header.length - (void *)control;
	rc = ibft_verify_hdr("control", (struct ibft_hdr *)control, id_control,
			     sizeof(*control));

	/* iBFT table safety checking */
	rc |= ((control->hdr.index) ? -ENODEV : 0);
	if (rc) {
		printk(KERN_ERR "iBFT error: Control header is invalid!\n");
		return rc;
	}
	for (ptr = &control->initiator_off; ptr < end; ptr += sizeof(u16)) {
		offset = *(u16 *)ptr;
		if (offset && offset < header->header.length &&
						offset < eot_offset) {
			rc = ibft_create_kobject(header,
						 (void *)header + offset);
			if (rc)
				break;
		}
	}

	return rc;
}

static void ibft_unregister(void)
{
	struct iscsi_boot_kobj *boot_kobj, *tmp_kobj;
	struct ibft_kobject *ibft_kobj;

	list_for_each_entry_safe(boot_kobj, tmp_kobj,
				 &boot_kset->kobj_list, list) {
		ibft_kobj = boot_kobj->data;
		if (ibft_kobj->hdr->id == id_nic)
			sysfs_remove_link(&boot_kobj->kobj, "device");
	};
}

static void ibft_cleanup(void)
{
	if (boot_kset) {
		ibft_unregister();
		iscsi_boot_destroy_kset(boot_kset);
	}
}

static void __exit ibft_exit(void)
{
	ibft_cleanup();
}

#ifdef CONFIG_ACPI
static const struct {
	char *sign;
} ibft_signs[] = {
	/*
	 * One spec says "IBFT", the other says "iBFT". We have to check
	 * for both.
	 */
	{ ACPI_SIG_IBFT },
	{ "iBFT" },
	{ "BIFT" },	/* Broadcom iSCSI Offload */
};

static void __init acpi_find_ibft_region(void)
{
	int i;
	struct acpi_table_header *table = NULL;

	if (acpi_disabled)
		return;

	for (i = 0; i < ARRAY_SIZE(ibft_signs) && !ibft_addr; i++) {
		acpi_get_table(ibft_signs[i].sign, 0, &table);
		ibft_addr = (struct acpi_table_ibft *)table;
	}
}
#else
static void __init acpi_find_ibft_region(void)
{
}
#endif

/*
 * ibft_init() - creates sysfs tree entries for the iBFT data.
 */
static int __init ibft_init(void)
{
	int rc = 0;

	/*
	   As on UEFI systems the setup_arch()/find_ibft_region()
	   is called before ACPI tables are parsed and it only does
	   legacy finding.
	*/
	if (!ibft_addr)
		acpi_find_ibft_region();

	if (ibft_addr) {
		pr_info("iBFT detected.\n");

		rc = ibft_check_device();
		if (rc)
			return rc;

		boot_kset = iscsi_boot_create_kset("ibft");
		if (!boot_kset)
			return -ENOMEM;

		/* Scan the IBFT for data and register the kobjects. */
		rc = ibft_register_kobjects(ibft_addr);
		if (rc)
			goto out_free;
	} else
		printk(KERN_INFO "No iBFT detected.\n");

	return 0;

out_free:
	ibft_cleanup();
	return rc;
}

module_init(ibft_init);
module_exit(ibft_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 *  Copyright 2007-2010 Red Hat, Inc.
 *  by Peter Jones <pjones@redhat.com>
 *  Copyright 2007 IBM, Inc.
 *  by Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *  Copyright 2008
 *  by Konrad Rzeszutek <ketuzsezr@darnok.org>
 *
 * This code finds the iSCSI Boot Format Table.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v2.0 as published by
 * the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/bootmem.h>
#include <linux/blkdev.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/acpi.h>
#include <linux/iscsi_ibft.h>

#include <asm/mmzone.h>

/*
 * Physical location of iSCSI Boot Format Table.
 */
struct acpi_table_ibft *ibft_addr;
EXPORT_SYMBOL_GPL(ibft_addr);

static const struct {
	char *sign;
} ibft_signs[] = {
	{ "iBFT" },
	{ "BIFT" },	/* Broadcom iSCSI Offload */
};

#define IBFT_SIGN_LEN 4
#define IBFT_START 0x80000 /* 512kB */
#define IBFT_END 0x100000 /* 1MB */
#define VGA_MEM 0xA0000 /* VGA buffer */
#define VGA_SIZE 0x20000 /* 128kB */

static int __init find_ibft_in_mem(void)
{
	unsigned long pos;
	unsigned int len = 0;
	void *virt;
	int i;

	for (pos = IBFT_START; pos < IBFT_END; pos += 16) {
		/* The table can't be inside the VGA BIOS reserved space,
		 * so skip that area */
		if (pos == VGA_MEM)
			pos += VGA_SIZE;
		virt = isa_bus_to_virt(pos);

		for (i = 0; i < ARRAY_SIZE(ibft_signs); i++) {
			if (memcmp(virt, ibft_signs[i].sign, IBFT_SIGN_LEN) ==
			    0) {
				unsigned long *addr =
				    (unsigned long *)isa_bus_to_virt(pos + 4);
				len = *addr;
				/* if the length of the table extends past 1M,
				 * the table cannot be valid. */
				if (pos + len <= (IBFT_END-1)) {
					ibft_addr = (struct acpi_table_ibft *)virt;
					pr_info("iBFT found at 0x%lx.\n", pos);
					goto done;
				}
			}
		}
	}
done:
	return len;
}
/*
 * Routine used to find the iSCSI Boot Format Table. The logical
 * kernel address is set in the ibft_addr global variable.
 */
unsigned long __init find_ibft_region(unsigned long *sizep)
{
	ibft_addr = NULL;

	/* iBFT 1.03 section 1.4.3.1 mandates that UEFI machines will
	 * only use ACPI for this */

	if (!efi_enabled(EFI_BOOT))
		find_ibft_in_mem();

	if (ibft_addr) {
		*sizep = PAGE_ALIGN(ibft_addr->header.length);
		return (u64)isa_virt_to_bus(ibft_addr);
	}

	*sizep = 0;
	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * linux/drivers/firmware/memmap.c
 *  Copyright (C) 2008 SUSE LINUX Products GmbH
 *  by Bernhard Walle <bernhard.walle@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v2.0 as published by
 * the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/string.h>
#include <linux/firmware-map.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/mm.h>

/*
 * Data types ------------------------------------------------------------------
 */

/*
 * Firmware map entry. Because firmware memory maps are flat and not
 * hierarchical, it's ok to organise them in a linked list. No parent
 * information is necessary as for the resource tree.
 */
struct firmware_map_entry {
	/*
	 * start and end must be u64 rather than resource_size_t, because e820
	 * resources can lie at addresses above 4G.
	 */
	u64			start;	/* start of the memory range */
	u64			end;	/* end of the memory range (incl.) */
	const char		*type;	/* type of the memory range */
	struct list_head	list;	/* entry for the linked list */
	struct kobject		kobj;   /* kobject for each entry */
};

/*
 * Forward declarations --------------------------------------------------------
 */
static ssize_t memmap_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf);
static ssize_t start_show(struct firmware_map_entry *entry, char *buf);
static ssize_t end_show(struct firmware_map_entry *entry, char *buf);
static ssize_t type_show(struct firmware_map_entry *entry, char *buf);

static struct firmware_map_entry * __meminit
firmware_map_find_entry(u64 start, u64 end, const char *type);

/*
 * Static data -----------------------------------------------------------------
 */

struct memmap_attribute {
	struct attribute attr;
	ssize_t (*show)(struct firmware_map_entry *entry, char *buf);
};

static struct memmap_attribute memmap_start_attr = __ATTR_RO(start);
static struct memmap_attribute memmap_end_attr   = __ATTR_RO(end);
static struct memmap_attribute memmap_type_attr  = __ATTR_RO(type);

/*
 * These are default attributes that are added for every memmap entry.
 */
static struct attribute *def_attrs[] = {
	&memmap_start_attr.attr,
	&memmap_end_attr.attr,
	&memmap_type_attr.attr,
	NULL
};

static const struct sysfs_ops memmap_attr_ops = {
	.show = memmap_attr_show,
};

/* Firmware memory map entries. */
static LIST_HEAD(map_entries);
static DEFINE_SPINLOCK(map_entries_lock);

/*
 * For memory hotplug, there is no way to free memory map entries allocated
 * by boot mem after the system is up. So when we hot-remove memory whose
 * map entry is allocated by bootmem, we need to remember the storage and
 * reuse it when the memory is hot-added again.
 */
static LIST_HEAD(map_entries_bootmem);
static DEFINE_SPINLOCK(map_entries_bootmem_lock);


static inline struct firmware_map_entry *
to_memmap_entry(struct kobject *kobj)
{
	return container_of(kobj, struct firmware_map_entry, kobj);
}

static void __meminit release_firmware_map_entry(struct kobject *kobj)
{
	struct firmware_map_entry *entry = to_memmap_entry(kobj);

	if (PageReserved(virt_to_page(entry))) {
		/*
		 * Remember the storage allocated by bootmem, and reuse it when
		 * the memory is hot-added again. The entry will be added to
		 * map_entries_bootmem here, and deleted from &map_entries in
		 * firmware_map_remove_entry().
		 */
		spin_lock(&map_entries_bootmem_lock);
		list_add(&entry->list, &map_entries_bootmem);
		spin_unlock(&map_entries_bootmem_lock);

		return;
	}

	kfree(entry);
}

static struct kobj_type __refdata memmap_ktype = {
	.release	= release_firmware_map_entry,
	.sysfs_ops	= &memmap_attr_ops,
	.default_attrs	= def_attrs,
};
