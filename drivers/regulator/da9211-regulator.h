mware mapping entry when we do
 * memory hotplug.
 * @start: Start of the memory range.
 * @end:   End of the memory range (exclusive)
 * @type:  Type of the memory range.
 *
 * Adds a firmware mapping entry. This function is for memory hotplug, it is
 * similar to function firmware_map_add_early(). The only difference is that
 * it will create the syfs entry dynamically.
 *
 * Return: 0 on success, or -ENOMEM if no memory could be allocated.
 */
int __meminit firmware_map_add_hotplug(u64 start, u64 end, const char *type)
{
	struct firmware_map_entry *entry;

	entry = firmware_map_find_entry(start, end - 1, type);
	if (entry)
		return 0;

	entry = firmware_map_find_entry_bootmem(start, end - 1, type);
	if (!entry) {
		entry = kzalloc(sizeof(struct firmware_map_entry), GFP_ATOMIC);
		if (!entry)
			return -ENOMEM;
	} else {
		/* Reuse storage allocated by bootmem. */
		spin_lock(&map_entries_bootmem_lock);
		list_del(&entry->list);
		spin_unlock(&map_entries_bootmem_lock);

		memset(entry, 0, sizeof(*entry));
	}

	firmware_map_add_entry(start, end, type, entry);
	/* create the memmap entry */
	add_sysfs_fw_map_entry(entry);

	return 0;
}

/**
 * firmware_map_add_early() - Adds a firmware mapping entry.
 * @start: Start of the memory range.
 * @end:   End of the memory range.
 * @type:  Type of the memory range.
 *
 * Adds a firmware mapping entry. This function uses the bootmem allocator
 * for memory allocation.
 *
 * That function must be called before late_initcall.
 *
 * Return: 0 on success, or -ENOMEM if no memory could be allocated.
 */
int __init firmware_map_add_early(u64 start, u64 end, const char *type)
{
	struct firmware_map_entry *entry;

	entry = memblock_virt_alloc(sizeof(struct firmware_map_entry), 0);
	if (WARN_ON(!entry))
		return -ENOMEM;

	return firmware_map_add_entry(start, end, type, entry);
}

/**
 * firmware_map_remove() - remove a firmware mapping entry
 * @start: Start of the memory range.
 * @end:   End of the memory range.
 * @type:  Type of the memory range.
 *
 * removes a firmware mapping entry.
 *
 * Return: 0 on success, or -EINVAL if no entry.
 */
int __meminit firmware_map_remove(u64 start, u64 end, const char *type)
{
	struct firmware_map_entry *entry;

	spin_lock(&map_entries_lock);
	entry = firmware_map_find_entry(start, end - 1, type);
	if (!entry) {
		spin_unlock(&map_entries_lock);
		return -EINVAL;
	}

	firmware_map_remove_entry(entry);
	spin_unlock(&map_entries_lock);

	/* remove the memmap entry */
	remove_sysfs_fw_map_entry(entry);

	return 0;
}

/*
 * Sysfs functions -------------------------------------------------------------
 */

static ssize_t start_show(struct firmware_map_entry *entry, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%llx\n",
		(unsigned long long)entry->start);
}

static ssize_t end_show(struct firmware_map_entry *entry, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%llx\n",
		(unsigned long long)entry->end);
}

static ssize_t type_show(struct firmware_map_entry *entry, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", entry->type);
}

static inline struct memmap_attribute *to_memmap_attr(struct attribute *attr)
{
	return container_of(attr, struct memmap_attribute, attr);
}

static ssize_t memmap_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct firmware_map_entry *entry = to_memmap_entry(kobj);
	struct memmap_attribute *memmap_attr = to_memmap_attr(attr);

	return memmap_attr->show(entry, buf);
}

/*
 * Initialises stuff and adds the entries in the map_entries list to
 * sysfs. Important is that firmware_map_add() and firmware_map_add_early()
 * must be called before late_initcall. That's just because that function
 * is called as late_initcall() function, which means that if you call
 * firmware_map_add() or firmware_map_add_early() afterwards, the entries
 * are not added to sysfs.
 */
static int __init firmware_memmap_init(void)
{
	struct firmware_map_entry *entry;

	list_for_each_entry(entry, &map_entries, list)
		add_sysfs_fw_map_entry(entry);

	return 0;
}
late_initcall(firmware_memmap_init);

              /*
 * Parse the EFI PCDP table to locate the console device.
 *
 * (c) Copyright 2002, 2003, 2004 Hewlett-Packard Development Company, L.P.
 *	Khalid Aziz <khalid.aziz@hp.com>
 *	Alex Williamson <alex.williamson@hp.com>
 *	Bjorn Helgaas <bjorn.helgaas@hp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/acpi.h>
#include <linux/console.h>
#include <linux/efi.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <asm/vga.h>
#include "pcdp.h"

static int __init
setup_serial_console(struct pcdp_uart *uart)
{
#ifdef CONFIG_SERIAL_8250_CONSOLE
	int mmio;
	static char options[64], *p = options;
	char parity;

	mmio = (uart->addr.space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY);
	p += sprintf(p, "uart8250,%s,0x%llx",
		mmio ? "mmio" : "io", uart->addr.address);
	if (uart->baud) {
		p += sprintf(p, ",%llu", uart->baud);
		if (uart->bits) {
			switch (uart->parity) {
			    case 0x2: parity = 'e'; break;
			    case 0x3: parity = 'o'; break;
			    default:  parity = 'n';
			}
			p += sprintf(p, "%c%d", parity, uart->bits);
		}
	}

	add_preferred_console("uart", 8250, &options[9]);
	return setup_earlycon(options);
#else
	return -ENODEV;
#endif
}

static int __init
setup_vga_console(struct pcdp_device *dev)
{
#if defined(CONFIG_VT) && defined(CONFIG_VGA_CONSOLE)
	u8 *if_ptr;

	if_ptr = ((u8 *)dev + sizeof(struct pcdp_device));
	if (if_ptr[0] == PCDP_IF_PCI) {
		struct pcdp_if_pci if_pci;

		/* struct copy since ifptr might not be correctly aligned */

		memcpy(&if_pci, if_ptr, sizeof(if_pci));

		if (if_pci.trans & PCDP_PCI_TRANS_IOPORT)
			vga_console_iobase = if_pci.ioport_tra;

		if (if_pci.trans & PCDP_PCI_TRANS_MMIO)
			vga_console_membase = if_pci.mmio_tra;
	}

	if (efi_mem_type(vga_console_membase + 0xA0000) == EFI_CONVENTIONAL_MEMORY) {
		printk(KERN_ERR "PCDP: VGA selected, but frame buffer is not MMIO!\n");
		return -ENODEV;
	}

	conswitchp = &vga_con;
	printk(KERN_INFO "PCDP: VGA console\n");
	return 0;
#else
	return -ENODEV;
#endif
}

int __init
efi_setup_pcdp_console(char *cmdline)
{
	struct pcdp *pcdp;
	struct pcdp_uart *uart;
	struct pcdp_device *dev, *end;
	int i, serial = 0;
	int rc = -ENODEV;

	if (efi.hcdp == EFI_INVALID_TABLE_ADDR)
		return -ENODEV;

	pcdp = early_ioremap(efi.hcdp, 4096);
	printk(KERN_INFO "PCDP: v%d at 0x%lx\n", pcdp->rev, efi.hcdp);

	if (strstr(cmdline, "console=hcdp")) {
		if (pcdp->rev < 3)
			serial = 1;
	} else if (strstr(cmdline, "console=")) {
		printk(KERN_INFO "Explicit \"console=\"; ignoring PCDP\n");
		goto out;
	}

	if (pcdp->rev < 3 && efi_uart_console_only())
		serial = 1;

	for (i = 0, uart = pcdp->uart; i < pcdp->num_uarts; i++, uart++) {
		if (uart->flags & PCDP_UART_PRIMARY_CONSOLE || serial) {
			if (uart->type == PCDP_CONSOLE_UART) {
				rc = setup_serial_console(uart);
				goto out;
			}
		}
	}

	end = (struct pcdp_device *) ((u8 *) pcdp + pcdp->length);
	for (dev = (struct pcdp_device *) (pcdp->uart + pcdp->num_uarts);
	     dev < end;
	     dev = (struct pcdp_device *) ((u8 *) dev + dev->length)) {
		if (dev->flags & PCDP_PRIMARY_CONSOLE) {
			if (dev->type == PCDP_CONSOLE_VGA) {
				rc = setup_vga_console(dev);
				goto out;
			}
		}
	}

out:
	early_iounmap(pcdp, 4096);
	return rc;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Definitions for PCDP-defined console devices
 *
 * For DIG64_HCDPv10a_01.pdf and DIG64_PCDPv20.pdf (v1.0a and v2.0 resp.),
 * please see <http://www.dig64.org/specifications/>
 *
 * (c) Copyright 2002, 2004 Hewlett-Packard Development Company, L.P.
 *	Khalid Aziz <khalid.aziz@hp.com>
 *	Bjorn Helgaas <bjorn.helgaas@hp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

