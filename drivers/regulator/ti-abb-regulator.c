cmd = acpi_gbl_FADT.smi_command;

	/* register device */
	gsmi_dev.pdev = platform_device_register_full(&gsmi_dev_info);
	if (IS_ERR(gsmi_dev.pdev)) {
		printk(KERN_ERR "gsmi: unable to register platform device\n");
		return PTR_ERR(gsmi_dev.pdev);
	}

	/* SMI access needs to be serialized */
	spin_lock_init(&gsmi_dev.lock);

	ret = -ENOMEM;
	gsmi_dev.dma_pool = dma_pool_create("gsmi", &gsmi_dev.pdev->dev,
					     GSMI_BUF_SIZE, GSMI_BUF_ALIGN, 0);
	if (!gsmi_dev.dma_pool)
		goto out_err;

	/*
	 * pre-allocate buffers because sometimes we are called when
	 * this is not feasible: oops, panic, die, mce, etc
	 */
	gsmi_dev.name_buf = gsmi_buf_alloc();
	if (!gsmi_dev.name_buf) {
		printk(KERN_ERR "gsmi: failed to allocate name buffer\n");
		goto out_err;
	}

	gsmi_dev.data_buf = gsmi_buf_alloc();
	if (!gsmi_dev.data_buf) {
		printk(KERN_ERR "gsmi: failed to allocate data buffer\n");
		goto out_err;
	}

	gsmi_dev.param_buf = gsmi_buf_alloc();
	if (!gsmi_dev.param_buf) {
		printk(KERN_ERR "gsmi: failed to allocate param buffer\n");
		goto out_err;
	}

	/*
	 * Determine type of handshake used to serialize the SMI
	 * entry. See also gsmi_exec().
	 *
	 * There's a "behavior" present on some chipsets where writing the
	 * SMI trigger register in the southbridge doesn't result in an
	 * immediate SMI. Rather, the processor can execute "a few" more
	 * instructions before the SMI takes effect. To ensure synchronous
	 * behavior, implement a handshake between the kernel driver and the
	 * firmware handler to spin until released. This ioctl determines
	 * the type of handshake.
	 *
	 * NONE: The firmware handler does not implement any
	 * handshake. Either it doesn't need to, or it's legacy firmware
	 * that doesn't know it needs to and never will.
	 *
	 * CF: The firmware handler will clear the CF in the saved
	 * state before returning. The driver may set the CF and test for
	 * it to clear before proceeding.
	 *
	 * SPIN: The firmware handler does not implement any handshake
	 * but the driver should spin for a hundred or so microseconds
	 * to ensure the SMI has triggered.
	 *
	 * Finally, the handler will return -ENOSYS if
	 * GSMI_CMD_HANDSHAKE_TYPE is unimplemented, which implies
	 * HANDSHAKE_NONE.
	 */
	spin_lock_irqsave(&gsmi_dev.lock, flags);
	gsmi_dev.handshake_type = GSMI_HANDSHAKE_SPIN;
	gsmi_dev.handshake_type =
	    gsmi_exec(GSMI_CALLBACK, GSMI_CMD_HANDSHAKE_TYPE);
	if (gsmi_dev.handshake_type == -ENOSYS)
		gsmi_dev.handshake_type = GSMI_HANDSHAKE_NONE;
	spin_unlock_irqrestore(&gsmi_dev.lock, flags);

	/* Remove and clean up gsmi if the handshake could not complete. */
	if (gsmi_dev.handshake_type == -ENXIO) {
		printk(KERN_INFO "gsmi version " DRIVER_VERSION
		       " failed to load\n");
		ret = -ENODEV;
		goto out_err;
	}

	/* Register in the firmware directory */
	ret = -ENOMEM;
	gsmi_kobj = kobject_create_and_add("gsmi", firmware_kobj);
	if (!gsmi_kobj) {
		printk(KERN_INFO "gsmi: Failed to create firmware kobj\n");
		goto out_err;
	}

	/* Setup eventlog access */
	ret = sysfs_create_bin_file(gsmi_kobj, &eventlog_bin_attr);
	if (ret) {
		printk(KERN_INFO "gsmi: Failed to setup eventlog");
		goto out_err;
	}

	/* Other attributes */
	ret = sysfs_create_files(gsmi_kobj, gsmi_attrs);
	if (ret) {
		printk(KERN_INFO "gsmi: Failed to add attrs");
		goto out_remove_bin_file;
	}

	ret = efivars_register(&efivars, &efivar_ops, gsmi_kobj);
	if (ret) {
		printk(KERN_INFO "gsmi: Failed to register efivars\n");
		goto out_remove_sysfs_files;
	}

	register_reboot_notifier(&gsmi_reboot_notifier);
	register_die_notifier(&gsmi_die_notifier);
	atomic_notifier_chain_register(&panic_notifier_list,
				       &gsmi_panic_notifier);

	printk(KERN_INFO "gsmi version " DRIVER_VERSION " loaded\n");

	return 0;

out_remove_sysfs_files:
	sysfs_remove_files(gsmi_kobj, gsmi_attrs);
out_remove_bin_file:
	sysfs_remove_bin_file(gsmi_kobj, &eventlog_bin_attr);
out_err:
	kobject_put(gsmi_kobj);
	gsmi_buf_free(gsmi_dev.param_buf);
	gsmi_buf_free(gsmi_dev.data_buf);
	gsmi_buf_free(gsmi_dev.name_buf);
	if (gsmi_dev.dma_pool)
		dma_pool_destroy(gsmi_dev.dma_pool);
	platform_device_unregister(gsmi_dev.pdev);
	pr_info("gsmi: failed to load: %d\n", ret);
	return ret;
}

static void __exit gsmi_exit(void)
{
	unregister_reboot_notifier(&gsmi_reboot_notifier);
	unregister_die_notifier(&gsmi_die_notifier);
	atomic_notifier_chain_unregister(&panic_notifier_list,
					 &gsmi_panic_notifier);
	efivars_unregister(&efivars);

	sysfs_remove_files(gsmi_kobj, gsmi_attrs);
	sysfs_remove_bin_file(gsmi_kobj, &eventlog_bin_attr);
	kobject_put(gsmi_kobj);
	gsmi_buf_free(gsmi_dev.param_buf);
	gsmi_buf_free(gsmi_dev.data_buf);
	gsmi_buf_free(gsmi_dev.name_buf);
	dma_pool_destroy(gsmi_dev.dma_pool);
	platform_device_unregister(gsmi_dev.pdev);
}

module_init(gsmi_init);
module_exit(gsmi_exit);

MODULE_AUTHOR("Google, Inc.");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * memconsole.c
 *
 * Infrastructure for importing the BIOS memory based console
 * into the kernel log ringbuffer.
 *
 * Copyright 2010 Google Inc. All rights reserved.
 */

#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/io.h>
#include <asm/bios_ebda.h>

#define BIOS_MEMCONSOLE_V1_MAGIC	0xDEADBABE
#define BIOS_MEMCONSOLE_V2_MAGIC	(('M')|('C'<<8)|('O'<<16)|('N'<<24))

struct biosmemcon_ebda {
	u32 signature;
	union {
		struct {
			u8  enabled;
			u32 buffer_addr;
			u16 start;
			u16 end;
			u16 num_chars;
			u8  wrapped;
		} __packed v1;
		struct {
			u32 buffer_addr;
			/* Misdocumented as number of pages! */
			u16 num_bytes;
			u16 start;
			u16 end;
		} __packed v2;
	};
} __packed;

static u32 memconsole_baseaddr;
static size_t memconsole_length;

static ssize_t memconsole_read(struct file *filp, struct kobject *kobp,
			       struct bin_attribute *bin_attr, char *buf,
			       loff_t pos, size_t count)
{
	char *memconsole;
	ssize_t ret;

	memconsole = ioremap_cache(memconsole_baseaddr, memconsole_length);
	if (!memconsole) {
		pr_err("memconsole: ioremap_cache failed\n");
		return -ENOMEM;
	}
	ret = memory_read_from_buffer(buf, count, &pos, memconsole,
				      memconsole_length);
	iounmap(memconsole);
	return ret;
}

static struct bin_attribute memconsole_bin_attr = {
	.attr = {.name = "log", .mode = 0444},
	.read = memconsole_read,
};


static void __init found_v1_header(struct biosmemcon_ebda *hdr)
{
	pr_info("BIOS console v1 EBDA structure found at %p\n", hdr);
	pr_info("BIOS console buffer at 0x%.8x, "
	       "start = %d, end = %d, num = %d\n",
	       hdr->v1.buffer_addr, hdr->v1.start,
	       hdr->v1.end, hdr->v1.num_chars);

	memconsole_length = hdr->v1.num_chars;
	memconsole_baseaddr = hdr->v1.buffer_addr;
}

static void __init found_v2_header(struct biosmemcon_ebda *hdr)
{
	pr_info("BIOS console v2 EBDA structure found at %p\n", hdr);
	pr_info("BIOS console buffer at 0x%.8x, "
	       "start = %d, end = %d, num_bytes = %d\n",
	       hdr->v2.buffer_addr, hdr->v2.start,
	       hdr->v2.end, hdr->v2.num_bytes);

	memconsole_length = hdr->v2.end - hdr->v2.start;
	memconsole_baseaddr = hdr->v2.buffer_addr + hdr->v2.start;
}

/*
 * Search through the EBDA for the BIOS Memory Console, and
 * set the global variables to point to it.  Return true if found.
 */
static bool __init found_memconsole(void)
{
	unsigned int address;
	size_t length, cur;

	address = get_bios_ebda();
	if (!address) {
		pr_info("BIOS EBDA non-existent.\n");
		return false;
	}

	/* EBDA length is byte 0 of EBDA (in KB) */
	length = *(u8 *)phys_to_virt(address);
	length <<= 10; /* convert to bytes */

	/*
	 * Search through EBDA for BIOS memory console structure
	 * note: signature is not necessarily dword-aligned
	 */
	for (cur = 0; cur < length; cur++) {
		struct biosmemcon_ebda *hdr = phys_to_virt(address + cur);

		/* memconsole v1 */
		if (hdr->signature == BIOS_MEMCONSOLE_V1_MAGIC) {
			found_v1_header(hdr);
			return true;
		}

		/* memconsole v2 */
		if (hdr->signature == BIOS_MEMCONSOLE_V2_MAGIC) {
			found_v2_header(hdr);
			return true;
		}
	}

	pr_info("BIOS console EBDA structure not found!\n");
	return false;
}

static struct dmi_system_id memconsole_dmi_table[] __initdata = {
	{
		.ident = "Google Board",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Google, Inc."),
		},
	},
	{}
};
MODULE_DEVICE_TABLE(dmi, memconsole_dmi_table);

static int __init memconsole_init(void)
{
	if (!dmi_check_system(memconsole_dmi_table))
		return -ENODEV;

	if (!found_memconsole())
		return -ENODEV;

	memconsole_bin_attr.size = memconsole_length;
	return sysfs_create_bin_file(firmware_kobj, &memconsole_bin_attr);
}

static void __exit memconsole_exit(void)
{
	sysfs_remove_bin_file(firmware_kobj, &memconsole_bin_attr);
}

module_init(memconsole_init);
module_exit(memconsole_exit);

MODULE_AUTHOR("Google, Inc.");
MODULE_LICENSE("GPL");
                                      /*
 *  Copyright 2007-2010 Red Hat, Inc.
 *  by Peter Jones <pjones@redhat.com>
 *  Copyright 2008 IBM, Inc.
 *  by Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *  Copyright 2008
 *  by Konrad Rzeszutek <ketuzsezr@darnok.org>
 *
 * This code exposes the iSCSI Boot Format Table to userland via sysfs.
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
 * Changelog:
 *
 *  06 Jan 2010 - Peter Jones <pjones@redhat.com>
 *    New changelog entries are in the git log from now on.  Not here.
 *
 *  14 Mar 2008 - Konrad Rzeszutek <ketuzsezr@darnok.org>
 *    Updated comments and copyrights. (v0.4.9)
 *
 *  11 Feb 2008 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *    Converted to using ibft_addr. (v0.4.8)
 *
 *   8 Feb 2008 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *    Combined two functions in one: reserve_ibft_region. (v0.4.7)
 *
 *  30 Jan 2008 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Added logic to handle IPv6 addresses. (v0.4.6)
 *
 *  25 Jan 2008 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Added logic to handle badly not-to-spec iBFT. (v0.4.5)
 *
 *   4 Jan 2008 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Added __init to function declarations. (v0.4.4)
 *
 *  21 Dec 2007 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Updated kobject registration, combined unregister functions in one
 *   and code and style cleanup. (v0.4.3)
 *
 *   5 Dec 2007 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Added end-markers to enums and re-organized kobject registration. (v0.4.2)
 *
 *   4 Dec 2007 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Created 'device' sysfs link to the NIC and style cleanup. (v0.4.1)
 *
 *  28 Nov 2007 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Added sysfs-ibft documentation, moved 'find_ibft' function to
 *   in its own file and added text attributes for every struct field.  (v0.4)
 *
 *  21 Nov 2007 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Added text attributes emulating OpenFirmware /proc/device-tree naming.
 *   Removed binary /sysfs interface (v0.3)
 *
 *  29 Aug 2007 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   Added functionality in setup.c to reserve iBFT region. (v0.2)
 *
 *  27 Aug 2007 - Konrad Rzeszutek <konradr@linux.vnet.ibm.com>
 *   First version exposing iBFT data via a binary /sysfs. (v0.1)
 *
 */


#include <linux/blkdev.h>
#include <linux/capability.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/iscsi_ibft.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/acpi.h>
#include <linux/iscsi_boot_sysfs.h>

#define IBFT_ISCSI_VERSION "0.5.0"
#define IBFT_ISCSI_DATE "2010-Feb-25"

MODULE_AUTHOR("Peter Jones <pjones@redhat.com> and "
	      "Konrad Rzeszutek <ketuzsezr@darnok.org>");
MODULE_DESCRIPTION("sysfs interface to BIOS iBFT information");
MODULE_LICENSE("GPL");
MODULE_VERSION(IBFT_ISCSI_VERSION);

#ifndef CONFIG_ISCSI_IBFT_FIND
struct acpi_table_ibft *ibft_addr;
#endif

struct ibft_hdr {
	u8 id;
	u8 version;
	u16 length;
	u8 index;
	u8 flags;
} __attribute__((__packed__));

struct ibft_control {
	struct ibft_hdr hdr;
	u16 extensions;
	u16 initiator_off;
	u16 nic0_off;
	u16 tgt0_off;
	u16 nic1_off;
	u16 tgt1_off;
} __attribute__((__packed__));

struct ibft_initiator {
	struct ibft_hdr hdr;
	char isns_server[16];
	char slp_server[16];
	char pri_radius_server[16];
	char sec_radius_server[16];
	u16 initiator_name_len;
	u16 initiator_name_off;
} __attribute__((__packed__));

struct ibft_nic {
	struct ibft_hdr hdr;
	char ip_addr[16];
	u8 subnet_mask_prefix;
	u8 origin;
	char gateway[16];
	char primary_dns[16];
	char secondary_dns[16];
	char dhcp[16];
	u16 vlan;
	char mac[6];
	u16 pci_bdf;
	u16 hostname_len;
	u16 hostname_off;
} __attribute__((__packed__));

struct ibft_tgt {
	struct ibft_hdr hdr;
	char ip_addr[16];
	u16 port;
	char lun[8];
	u8 chap_type;
	u8 nic_assoc;
	u16 tgt_name_len;
	u16 tgt_name_off;
	u16 chap_name_len;
	u16 chap_name_off;
	u16 chap_secret_len;
	u16 chap_secret_off;
	u16 rev_chap_name_len;
	u16 rev_chap_name_off;
	u16 rev_chap_secret_len;
	u16 rev_chap_secret_off;
} __attribute__((__packed__));

/*
 * The kobject different types and its names.
 *
*/
enum ibft_id {
	id_reserved = 0, /* We don't support. */
	id_control = 1, /* Should show up only once and is not exported. */
	id_initiator = 2,
	id_nic = 3,
	id_target = 4,
	id_extensions = 5, /* We don't support. */
	id_end_marker,
};

/*
 * The kobject and attribute structures.
 */

struct ibft_kobject {
	struct acpi_table_ibft *header;
	union {
		struct ibft_initiator *initiator;
		struct ibft_nic *nic;
		struct ibft_tgt *tgt;
		struct ibft_hdr *hdr;
	};
};

static struct iscsi_boot_kset *boot_kset;

/* fully null address */
static const char nulls[16];

/* IPv4-mapped IPv6 ::ffff:0.0.0.0 */
static const char mapped_nulls[16] = { 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0xff, 0xff,
                                       0x00, 0x00, 0x00, 0x00 };

static int address_not_null(u8 *ip)
{
	return (memcmp(ip, nulls, 16) && memcmp(ip, mapped_nulls, 16));
}

/*
 * Helper functions to parse data properly.
 */
static ssize_t sprintf_ipaddr(char *buf, u8 *ip)
{
	char *str = buf;

	if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0 &&
	    ip[4] == 0 && ip[5] == 0 && ip[6] == 0 && ip[7] == 0 &&
	    ip[8] == 0 && ip[9] == 0 && ip[10] == 0xff && ip[11] == 0xff) {
		/*
		 * IPV4
		 */
		str += sprintf(buf, "%pI4", ip + 12);
	} else {
		/*
		 * IPv6
		 */
		str += sprintf(str, "%pI6", ip);
	}
	str += sprintf(str, "\n");
	return str - buf;
}

static ssize_t sprintf_string(char *str, int len, char *buf)
{
	return sprintf(str, "%.*s\n", len, buf);
}

/*
 * Helper function to verify the IBFT header.
 */
static int ibft_verify_hdr(char *t, struct ibft_hdr *hdr, int id, int length)
{
	if (hdr->id != id) {
		printk(KERN_ERR "iBFT error: We expected the %s " \
				"field header.id to have %d but " \
				"found %d instead!\n", t, id, hdr->id);
		return -ENODEV;
	}
	if (hdr->length != length) {
		printk(KERN_ERR "iBFT error: We expected the %s " \
				"field header.length to have %d but " \
				"found %d instead!\n", t, length, hdr->length);
		return -ENODEV;
	}

	return 0;
}

/*
 *  Routines for parsing the iBFT data to be human readable.
 */
static ssize_t ibft_attr_show_initiator(void *data, int type, char *buf)
{
	struct ibft_kobject *entry = data;
	struct ibft_initiator *initiator = entry->initiator;
	void *ibft_loc = entry->header;
	char *str = buf;

	if (!initiator)
		return 0;

	switch (type) {
	case ISCSI_BOOT_INI_INDEX:
		str += sprintf(str, "%d\n", initiator->hdr.index);
		break;
	case ISCSI_BOOT_INI_FLAGS:
		str += sprintf(str, "%d\n", initiator->hdr.flags);
		break;
	case ISCSI_BOOT_INI_ISNS_SERVER:
		str += sprintf_ipaddr(str, initiator->isns_server);
		break;
	case ISCSI_BOOT_INI_SLP_SERVER:
		str += sprintf_ipaddr(str, initiator->slp_server);
		break;
	case ISCSI_BOOT_INI_PRI_RADIUS_SERVER:
		str += sprintf_ipaddr(str, initiator->pri_radius_server);
		break;
	case ISCSI_BOOT_INI_SEC_RADIUS_SERVER:
		str += sprintf_ipaddr(str, initiator->sec_radius_server);
		break;
	case ISCSI_BOOT_INI_INITIATOR_NAME:
		str += sprintf_string(str, initiator->initiator_name_len,
				      (char *)ibft_loc +
				      initiator->initiator_name_off);
		break;
	default:
		break;
	}

	return str - buf;
}

static ssize_t ibft_attr_show_nic(void *data, int type, char *buf)
{
	struct ibft_kobject *entry = data;
	struct ibft_nic *nic = entry->nic;
	void *ibft_loc = entry->header;
	char *str = buf;
	__be32 val;

	if (!nic)
		return 0;

	switch (type) {
	case ISCSI_BOOT_ETH_INDEX:
		str += sprintf(str, "%d\n", nic->hdr.index);
		break;
	case ISCSI_BOOT_ETH_FLAGS:
		str += sprintf(str, "%d\n", nic->hdr.flags);
		break;
	case ISCSI_BOOT_ETH_IP_ADDR:
		str += sprintf_ipaddr(str, nic->ip_addr);
		break;
	case ISCSI_BOOT_ETH_SUBNET_MASK:
		val = cpu_to_be32(~((1 << (32-nic->subnet_mask_prefix))-1));
		str += sprintf(str, "%pI4", &val);
		break;
	case ISCSI_BOOT_ETH_ORIGIN:
		str += sprintf(str, "%d\n", nic->origin);
		break;
	case ISCSI_BOOT_ETH_GATEWAY:
		str += sprintf_ipaddr(str, nic->gateway);
		break;
	case ISCSI_BOOT_ETH_PRIMARY_DNS:
		str += sprintf_ipaddr(str, nic->primary_dns);
		break;
	case ISCSI_BOOT_ETH_SECONDARY_DNS:
		str += sprintf_ipaddr(str, nic->secondary_dns);
		break;
	case ISCSI_BOOT_ETH_DHCP:
		str += sprintf_ipaddr(str, nic->dhcp);
		break;
	case ISCSI_BOOT_ETH_VLAN:
		str += sprintf(str, "%d\n", nic->vlan);
		break;
	case ISCSI_BOOT_ETH_MAC:
		str += sprintf(str, "%pM\n", nic->mac);
		break;
	case ISCSI_BOOT_ETH_HOSTNAME:
		str += sprintf_string(str, nic->hostname_len,
				      (char *)ibft_loc + nic->hostname_off);
		break;
	default:
		break;
	}

	return str - buf;
};

static ssize_t ibft_attr_show_target(void *data, int type, char *buf)
{
	struct ibft_kobject *entry = data;
	struct ibft_tgt *tgt = entry->tgt;
	void *ibft_loc = entry->header;
	char *str = buf;
	int i;

	if (!tgt)
		return 0;

	switch (type) {
	case ISCSI_BOOT_TGT_INDEX:
		str += sprintf(str, "%d\n", tgt->hdr.index);
		break;
	case ISCSI_BOOT_TGT_FLAGS:
		str += sprintf(str, "%d\n", tgt->hdr.flags);
		break;
	case ISCSI_BOOT_TGT_IP_ADDR:
		str += sprintf_ipaddr(str, tgt->ip_addr);
		break;
	case ISCSI_BOOT_TGT_PORT:
		str += sprintf(str, "%d\n", tgt->port);
		break;
	case ISCSI_BOOT_TGT_LUN:
		for (i = 0; i < 8; i++)
			str += sprintf(str, "%x", (u8)tgt->lun[i]);
		str += sprintf(str, "\n");
		break;
	case ISCSI_BOOT_TGT_NIC_ASSOC:
		str += sprintf(str, "%d\n", tgt->nic_assoc);
		break;
	case ISCSI_BOOT_TGT_CHAP_TYPE:
		str += sprintf(str, "%d\n", tgt->chap_type);
		break;
	case ISCSI_BOOT_TGT_NAME:
		str += sprintf_string(str, tgt->tgt_name_len,
				      (char *)ibft_loc + tgt->tgt_name_off);
		break;
	case ISCSI_BOOT_TGT_CHAP_NAME:
		str += sprintf_string(str, tgt->chap_name_len,
				      (char *)ibft_loc + tgt->chap_name_off);
		break;
	case ISCSI_BOOT_TGT_CHAP_SECRET:
		str += sprintf_string(str, tgt->chap_secret_len,
				      (char *)ibft_loc + tgt->chap_secret_off);
		break;
	case ISCSI_BOOT_TGT_REV_CHAP_NAME:
		str += sprintf_string(str, tgt->rev_chap_name_len,
				      (char *)ibft_loc +
				      tgt->rev_chap_name_off);
		break;
	case ISCSI_BOOT_TGT_REV_CHAP_SECRET:
		str += sprintf_string(str, tgt->rev_chap_secret_len,
				      (char *)ibft_loc +
				      tgt->rev_chap_secret_off);
		break;
	default:
		break;
	}

	return str - buf;
}

static int __init ibft_check_device(void)
{
	int len;
	u8 *pos;
	u8 csum = 0;

	len = ibft_addr->header.length;

	/* Sanity checking of iBFT. */
	if (ibft_addr->header.revision != 1) {
		printk(KERN_ERR "iBFT module supports only revision 1, " \
				"while this is %d.\n",
				ibft_addr->header.revision);
		return -ENOENT;
	}
	for (pos = (u8 *)ibft_addr; pos < (u8 *)ibft_addr + len; pos++)
		csum += *pos;

	if (csum) {
		printk(KERN_ERR "iBFT has incorrect checksum (0x%x)!\n", csum);
		return -ENOENT;
	}

	return 0;
}

/*
 * Helper routiners to check to determine if the entry is valid
 * in the proper iBFT structure.
 */
static umode_t ibft_check_nic_for(void *data, int type)
{
	struct ibft_kobject *entry = data;
	struct ibft_nic *nic = entry->nic;
	umode_t rc = 0;

	switch (type) {
	case ISCSI_BOOT_ETH_INDEX:
	case ISCSI_BOOT_ETH_FLAGS:
		rc = S_IRUGO;
		break;
	case ISCSI_BOOT_ETH_IP_ADDR:
		if (address_not_null(nic->ip_addr))
			rc = S_IRUGO;
		break;
	case ISCSI_BOOT_ETH_SUBNET_MASK:
		if (nic->subnet_mask_prefix)
			rc = S_IRUGO;
		break;
	case ISCSI_BOOT_ETH_ORIGIN:
		rc = S_IRUGO;
		break;
	case ISCSI_BOOT_ETH_GATEWAY:
		if (address_not_null(nic->gateway))
			rc = S_IRUGO;
		break;
	case ISCSI_BOOT_ETH_PRIMARY_DNS:
		if (address_not_null(nic->primary_dns))
			rc = S_IRUGO;
		break;
	case ISCSI_BOOT_ETH_SECONDARY_DNS:
		if (address_not_null(nic->secondary_dns))
			rc = S_IRUGO;
		break;
	case ISCSI_BOOT_ETH_DHCP:
		if (address_not_null(nic->dhcp))
			rc = S_IRUGO;
		break;
	case ISCSI_BOOT_ETH_VLAN:
	case ISCSI_BOOT_ETH_MAC:
		rc = S_IRUGO;
		bre