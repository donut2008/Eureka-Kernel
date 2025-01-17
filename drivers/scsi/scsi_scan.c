VICE_ID_ML7213_DMA4_12CH	0x8032
#define PCI_DEVICE_ID_ML7223_DMA1_4CH	0x800B
#define PCI_DEVICE_ID_ML7223_DMA2_4CH	0x800E
#define PCI_DEVICE_ID_ML7223_DMA3_4CH	0x8017
#define PCI_DEVICE_ID_ML7223_DMA4_4CH	0x803B
#define PCI_DEVICE_ID_ML7831_DMA1_8CH	0x8810
#define PCI_DEVICE_ID_ML7831_DMA2_4CH	0x8815

static const struct pci_device_id pch_dma_id_table[] = {
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_EG20T_PCH_DMA_8CH), 8 },
	{ PCI_VDEVICE(INTEL, PCI_DEVICE_ID_EG20T_PCH_DMA_4CH), 4 },
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7213_DMA1_8CH), 8}, /* UART Video */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7213_DMA2_8CH), 8}, /* PCMIF SPI */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7213_DMA3_4CH), 4}, /* FPGA */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7213_DMA4_12CH), 12}, /* I2S */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7223_DMA1_4CH), 4}, /* UART */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7223_DMA2_4CH), 4}, /* Video SPI */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7223_DMA3_4CH), 4}, /* Security */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7223_DMA4_4CH), 4}, /* FPGA */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7831_DMA1_8CH), 8}, /* UART */
	{ PCI_VDEVICE(ROHM, PCI_DEVICE_ID_ML7831_DMA2_4CH), 4}, /* SPI */
	{ 0, },
};

static struct pci_driver pch_dma_driver = {
	.name		= DRV_NAME,
	.id_table	= pch_dma_id_table,
	.probe		= pch_dma_probe,
	.remove		= pch_dma_remove,
#ifdef CONFIG_PM
	.suspend	= pch_dma_suspend,
	.resume		= pch_dma_resume,
#endif
};

module_pci_driver(pch_dma_driver);

MODULE_DESCRIPTION("Intel EG20T PCH / LAPIS Semicon ML7213/ML7223/ML7831 IOH "
		   "DMA controller driver");
MODULE_AUTHOR("Yong Wang <yong.y.wang@intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_DEVICE_TABLE(pci, pch_dma_id_table);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #include <linux/types.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/dmi.h>
#include <linux/efi.h>
#include <linux/bootmem.h>
#include <linux/random.h>
#include <asm/dmi.h>
#include <asm/unaligned.h>

struct kobject *dmi_kobj;
EXPORT_SYMBOL_GPL(dmi_kobj);

/*
 * DMI stands for "Desktop Management Interface".  It is part
 * of and an antecedent to, SMBIOS, which stands for System
 * Management BIOS.  See further: http://www.dmtf.org/standards
 */
static const char dmi_empty_string[] = "";

static u32 dmi_ver __initdata;
static u32 dmi_len;
static u16 dmi_num;
static u8 smbios_entry_point[32];
static int smbios_entry_point_size;

/*
 * Catch too early calls to dmi_check_system():
 */
static int dmi_initialized;

/* DMI system identification string used during boot */
static char dmi_ids_string[128] __initdata;

static struct dmi_memdev_info {
	const char *device;
	const char *bank;
	u16 handle;
} *dmi_memdev;
static int dmi_memdev_nr;

static const char * __init dmi_string_nosave(const struct dmi_header *dm, u8 s)
{
	const u8 *bp = ((u8 *) dm) + dm->length;
	const u8 *nsp;

	if (s) {
		while (--s > 0 && *bp)
			bp += strlen(bp) + 1;

		/* Strings containing only spaces are considered empty */
		nsp = bp;
		while (*nsp == ' ')
			nsp++;
		if (*nsp != '\0')
			return bp;
	}

	return dmi_empty_string;
}

static const char * __init dmi_string(const struct dmi_header *dm, u8 s)
{
	const char *bp = dmi_string_nosave(dm, s);
	char *str;
	size_t len;

	if (bp == dmi_empty_string)
		return dmi_empty_string;

	len = strlen(bp) + 1;
	str = dmi_alloc(len);
	if (str != NULL)
		strcpy(str, bp);

	return str;
}

/*
 *	We have to be cautious here. We have seen BIOSes with DMI pointers
 *	pointing to completely the wrong place for example
 */
static void dmi_decode_table(u8 *buf,
			     void (*decode)(const struct dmi_header *, void *),
			     void *private_data)
{
	u8 *data = buf;
	int i = 0;

	/*
	 * Stop when we have seen all the items the table claimed to have
	 * (SMBIOS < 3.0 only) OR we reach an end-of-table marker (SMBIOS
	 * >= 3.0 only) OR we run off the end of the table (should never
	 * happen but sometimes does on bogus implementations.)
	 */
	while ((!dmi_num || i < dmi_num) &&
	       (data - buf + sizeof(struct dmi_header)) <= dmi_len) {
		const struct dmi_header *dm = (const struct dmi_header *)data;

		/*
		 *  We want to know the total length (formatted area and
		 *  strings) before decoding to make sure we won't run off the
		 *  table in dmi_decode or dmi_string
		 */
		data += dm->length;
		while ((data - buf < dmi_len - 1) && (data[0] || data[1]))
			data++;
		if (data - buf < dmi_len - 1)
			decode(dm, private_data);

		data += 2;
		i++;

		/*
		 * 7.45 End-of-Table (Type 127) [SMBIOS reference spec v3.0.0]
		 * For tables behind a 64-bit entry point, we have no item
		 * count and no exact table length, so stop on end-of-table
		 * marker. For tables behind a 32-bit entry point, we have
		 * seen OEM structures behind the end-of-table marker on
		 * some systems, so don't trust it.
		 */
		if (!dmi_num && dm->type == DMI_ENTRY_END_OF_TABLE)
			break;
	}

	/* Trim DMI table length if needed */
	if (dmi_len > data - buf)
		dmi_len = data - buf;
}

static phys_addr_t dmi_base;

static int __init dmi_walk_early(void (*decode)(const struct dmi_header *,
		void *))
{
	u8 *buf;
	u32 orig_dmi_len = dmi_len;

	buf = dmi_early_remap(dmi_base, orig_dmi_len);
	if (buf == NULL)
		return -1;

	dmi_decode_table(buf, decode, NULL);

	add_device_randomness(buf, dmi_len);

	dmi_early_unmap(buf, orig_dmi_len);
	return 0;
}

static int __init dmi_checksum(const u8 *buf, u8 len)
{
	u8 sum = 0;
	int a;

	for (a = 0; a < len; a++)
		sum += buf[a];

	return sum == 0;
}

static const char *dmi_ident[DMI_STRING_MAX];
static LIST_HEAD(dmi_devices);
int dmi_available;

/*
 *	Save a DMI string
 */
static void __init dmi_save_ident(const struct dmi_header *dm, int slot,
		int string)
{
	const char *d = (const char *) dm;
	const char *p;

	if (dmi_ident[slot])
		return;

	p = dmi_string(dm, d[string]);
	if (p == NULL)
		return;

	dmi_ident[slot] = p;
}

static void __init dmi_save_uuid(const struct dmi_header *dm, int slot,
		int index)
{
	const u8 *d = (u8 *) dm + index;
	char *s;
	int is_ff = 1, is_00 = 1, i;

	if (dmi_ident[slot])
		return;

	for (i = 0; i < 16 && (is_ff || is_00); i++) {
		if (d[i] != 0x00)
			is_00 = 0;
		if (d[i] != 0xFF)
			is_ff = 0;
	}

	if (is_ff || is_00)
		return;

	s = dmi_alloc(16*2+4+1);
	if (!s)
		return;

	/*
	 * As of version 2.6 of the SMBIOS specification, the first 3 fields of
	 * the UUID are supposed to be little-endian encoded.  The specification
	 * says that this is the defacto standard.
	 */
	if (dmi_ver >= 0x020600)
		sprintf(s, "%pUL", d);
	else
		sprintf(s, "%pUB", d);

	dmi_ident[slot] = s;
}

static void __init dmi_save_type(const struct dmi_header *dm, int slot,
		int index)
{
	const u8 *d = (u8 *) dm + index;
	char *s;

	if (dmi_ident[slot])
		return;

	s = dmi_alloc(4);
	if (!s)
		return;

	sprintf(s, "%u", *d & 0x7F);
	dmi_ident[slot] = s;
}

static void __init dmi_save_one_device(int type, const char *name)
{
	struct dmi_device *dev;

	/* No duplicate device */
	if (dmi_find_device(type, name, NULL))
		return;

	dev = dmi_alloc(sizeof(*dev) + strlen(name) + 1);
	if (!dev)
		return;

	dev->type = type;
	strcpy((char *)(dev + 1), name);
	dev->name = (char *)(dev + 1);
	dev->device_data = NULL;
	list_add(&dev->list, &dmi_devices);
}

static void __init dmi_save_devices(const struct dmi_header *dm)
{
	int i, count = (dm->length - sizeof(struct dmi_header)) / 2;

	for (i = 0; i < count; i++) {
		const char *d = (char *)(dm + 1) + (i * 2);

		/* Skip disabled device */
		if ((*d & 0x80) == 0)
			continue;

		dmi_save_one_device(*d & 0x7f, dmi_string_nosave(dm, *(d + 1)));
	}
}

static void __init dmi_save_oem_strings_devices(const struct dmi_header *dm)
{
	int i, count = *(u8 *)(dm + 1);
	struct dmi_device *dev;

	for (i = 1; i <= count; i++) {
		const char *devname = dmi_string(dm, i);

		if (devname == dmi_empty_string)
			continue;

		dev = dmi_alloc(sizeof(*dev));
		if (!dev)
			break;

		dev->type = DMI_DEV_TYPE_OEM_STRING;
		dev->name = devname;
		dev->device_data = NULL;

		list_add(&dev->list, &dmi_devices);
	}
}

static void __init dmi_save_ipmi_device(const struct dmi_header *dm)
{
	struct dmi_device *dev;
	void *data;

	data = dmi_alloc(dm->length);
	if (data == NULL)
		return;

	memcpy(data, dm, dm->length);

	dev = dmi_alloc(sizeof(*dev));
	if (!dev)
		return;

	dev->type = DMI_DEV_TYPE_IPMI;
	dev->name = "IPMI controller";
	dev->device_data = data;

	list_add_tail(&dev->list, &dmi_devices);
}

static void __init dmi_save_dev_onboard(int instance, int segment, int bus,
					int devfn, const char *name)
{
	struct dmi_dev_onboard *onboard_dev;

	onboard_dev = dmi_alloc(sizeof(*onboard_dev) + strlen(name) + 1);
	if (!onboard_dev)
		return;

	onboard_dev->instance = instance;
	onboard_dev->segment = segment;
	onboard_dev->bus = bus;
	onboard_dev->devfn = devfn;

	strcpy((char *)&onboard_dev[1], name);
	onboard_dev->dev.type = DMI_DEV_TYPE_DEV_ONBOARD;
	onboard_dev->dev.name = (char *)&onboard_dev[1];
	onboard_dev->dev.device_data = onboard_dev;

	list_add(&onboard_dev->dev.list, &dmi_devices);
}

static void __init dmi_save_extended_devices(const struct dmi_header *dm)
{
	const u8 *d = (u8 *) dm + 5;

	/* Skip disabled device */
	if ((*d & 0x80) == 0)
		return;

	dmi_save_dev_onboard(*(d+1), *(u16 *)(d+2), *(d+4), *(d+5),
			     dmi_string_nosave(dm, *(d-1)));
	dmi_save_one_device(*d & 0x7f, dmi_string_nosave(dm, *(d - 1)));
}

static void __init count_mem_devices(const struct dmi_header *dm, void *v)
{
	if (dm->type != DMI_ENTRY_MEM_DEVICE)
		return;
	dmi_memdev_nr++;
}

static void __init save_mem_devices(const struct dmi_header *dm, void *v)
{
	const char *d = (const char *)dm;
	static int nr;

	if (dm->type != DMI_ENTRY_MEM_DEVICE)
		return;
	if (nr >= dmi_memdev_nr) {
		pr_warn(FW_BUG "Too many DIMM entries in SMBIOS table\n");
		return;
	}
	dmi_memdev[nr].handle = get_unaligned(&dm->handle);
	dmi_memdev[nr].device = dmi_string(dm, d[0x10]);
	dmi_memdev[nr].bank = dmi_string(dm, d[0x11]);
	nr++;
}

void __init dmi_memdev_walk(void)
{
	if (!dmi_available)
		return;

	if (dmi_walk_early(count_mem_devices) == 0 && dmi_memdev_nr) {
		dmi_memdev = dmi_alloc(sizeof(*dmi_memdev) * dmi_memdev_nr);
		if (dmi_memdev)
			dmi_walk_early(save_mem_devices);
	}
}

/*
 *	Process a DMI table entry. Right now all we care about are the BIOS
 *	and machine entries. For 2.5 we should pull the smbus controller info
 *	out of here.
 */
static void __init dmi_decode(const struct dmi_header *dm, void *dummy)
{
	switch (dm->type) {
	case 0:		/* BIOS Information */
		dmi_save_ident(dm, DMI_BIOS_VENDOR, 4);
		dmi_save_ident(dm, DMI_BIOS_VERSION, 5);
		dmi_save_ident(dm, DMI_BIOS_DATE, 8);
		break;
	case 1:		/* System Information */
		dmi_save_ident(dm, DMI_SYS_VENDOR, 4);
		dmi_save_ident(dm, DMI_PRODUCT_NAME, 5);
		dmi_save_ident(dm, DMI_PRODUCT_VERSION, 6);
		dmi_save_ident(dm, DMI_PRODUCT_SERIAL, 7);
		dmi_save_uuid(dm, DMI_PRODUCT_UUID, 8);
		break;
	case 2:		/* Base Board Information */
		dmi_save_ident(dm, DMI_BOARD_VENDOR, 4);
		dmi_save_ident(dm, DMI_BOARD_NAME, 5);
		dmi_save_ident(dm, DMI_BOARD_VERSION, 6);
		dmi_save_ident(dm, DMI_BOARD_SERIAL, 7);
		dmi_save_ident(dm, DMI_BOARD_ASSET_TAG, 8);
		break;
	case 3:		/* Chassis Information */
		dmi_save_ident(dm, DMI_CHASSIS_VENDOR, 4);
		dmi_save_type(dm, DMI_CHASSIS_TYPE, 5);
		dmi_save_ident(dm, DMI_CHASSIS_VERSION, 6);
		dmi_save_ident(dm, DMI_CHASSIS_SERIAL, 7);
		dmi_save_ident(dm, DMI_CHASSIS_ASSET_TAG, 8);
		break;
	case 10:	/* Onboard Devices Information */
		dmi_save_devices(dm);
		break;
	case 11:	/* OEM Strings */
		dmi_save_oem_strings_devices(dm);
		break;
	case 38:	/* IPMI Device Information */
		dmi_save_ipmi_device(dm);
		break;
	case 41:	/* Onboard Devices Extended Information */
		dmi_save_extended_devices(dm);
	}
}

static int __init print_filtered(char *buf, size_t len, const char *info)
{
	int c = 0;
	const char *p;

	if (!info)
		return c;

	for (p = info; *p; p++)
		if (isprint(*p))
			c += scnprintf(buf + c, len - c, "%c", *p);
		else
			c += scnprintf(buf + c, len - c, "\\x%02x", *p & 0xff);
	return c;
}

static void __init dmi_format_ids(char *buf, size_t len)
{
	int c = 0;
	const char *board;	/* Board Name is optional */

	c += print_filtered(buf + c, len - c,
			    dmi_get_system_info(DMI_SYS_VENDOR));
	c += scnprintf(buf + c, len - c, " ");
	c += print_filtered(buf + c, len - c,
			    dmi_get_system_info(DMI_PRODUCT_NAME));

	board = dmi_get_system_info(DMI_BOARD_NAME);
	if (board) {
		c += scnprintf(buf + c, len - c, "/");
		c += print_filtered(buf + c, len - c, board);
	}
	c += scnprintf(buf + c, len - c, ", BIOS ");
	c += print_filtered(buf + c, len - c,
			    dmi_get_system_info(DMI_BIOS_VERSION));
	c += scnprintf(buf + c, len - c, " ");
	c += print_filtered(buf + c, len - c,
			    dmi_get_system_info(DMI_BIOS_DATE));
}

/*
 * Check for DMI/SMBIOS headers in the system firmware image.  Any
 * SMBIOS header must start 16 bytes before the DMI header, so take a
 * 32 byte buffer and check for DMI at offset 16 and SMBIOS at offset
 * 0.  If the DMI header is present, set dmi_ver accordingly (SMBIOS
 * takes precedence) and return 0.  Otherwise return 1.
 */
static int __init dmi_present(const u8 *buf)
{
	u32 smbios_ver;

	if (memcmp(buf, "_SM_", 4) == 0 &&
	    buf[5] < 32 && dmi_checksum(buf, buf[5])) {
		smbios_ver = get_unaligned_be16(buf + 6);
		smbios_entry_point_size = buf[5];
		memcpy(smbios_entry_point, buf, smbios_entry_point_size);

		/* Some BIOS report weird SMBIOS version, fix that up */
		switch (smbios_ver) {
		case 0x021F:
		case 0x0221:
			pr_debug("SMBIOS version fixup (2.%d->2.%d)\n",
				 smbios_ver & 0xFF, 3);
			smbios_ver = 0x0203;
			break;
		case 0x0233:
			pr_debug("SMBIOS version fixup (2.%d->2.%d)\n", 51, 6);
			smbios_ver = 0x0206;
			break;
		}
	} else {
		smbios_ver = 0;
	}

	buf += 16;

	if (memcmp(buf, "_DMI_", 5) == 0 && dmi_checksum(buf, 15)) {
		if (smbios_ver)
			dmi_ver = smbios_ver;
		else
			dmi_ver = (buf[14] & 0xF0) << 4 | (buf[14] & 0x0F);
		dmi_ver <<= 8;
		dmi_num = get_unaligned_le16(buf + 12);
		dmi_len = get_unaligned_le16(buf + 6);
		dmi_base = get_unaligned_le32(buf + 8);

		if (dmi_walk_early(dmi_decode) == 0) {
			if (smbios_ver) {
				pr_info("SMBIOS %d.%d present.\n",
					dmi_ver >> 16, (dmi_ver >> 8) & 0xFF);
			} else {
				smbios_entry_point_size = 15;
				memcpy(smbios_entry_point, buf,
				       smbios_entry_point_size);
				pr_info("Legacy DMI %d.%d present.\n",
					dmi_ver >> 16, (dmi_ver >> 8) & 0xFF);
			}
			dmi_format_ids(dmi_ids_string, sizeof(dmi_ids_string));
			printk(KERN_DEBUG "DMI: %s\n", dmi_ids_string);
			return 0;
		}
	}

	return 1;
}

/*
 * Check for the SMBIOS 3.0 64-bit entry point signature. Unlike the legacy
 * 32-bit entry point, there is no embedded DMI header (_DMI_) in here.
 */
static int __init dmi_smbios3_present(const u8 *buf)
{
	if (memcmp(buf, "_SM3_", 5) == 0 &&
	    buf[6] < 32 && dmi_checksum(buf, buf[6])) {
		dmi_ver = get_unaligned_be32(buf + 6) & 0xFFFFFF;
		dmi_num = 0;			/* No longer specified */
		dmi_len = get_unaligned_le32(buf + 12);
		dmi_base = get_unaligned_le64(buf + 16);
		smbios_entry_point_size = buf[6];
		memcpy(smbios_entry_point, buf, smbios_entry_point_size);

		if (dmi_walk_early(dmi_decode) == 0) {
			pr_info("SMBIOS %d.%d.%d present.\n",
				dmi_ver >> 16, (dmi_ver >> 8) & 0xFF,
				dmi_ver & 0xFF);
			dmi_format_ids(dmi_ids_string, sizeof(dmi_ids_string));
			pr_debug("DMI: %s\n", dmi_ids_string);
			return 0;
		}
	}
	return 1;
}

void __init dmi_scan_machine(void)
{
	char __iomem *p, *q;
	char buf[32];

	if (efi_enabled(EFI_CONFIG_TABLES)) {
		/*
		 * According to the DMTF SMBIOS reference spec v3.0.0, it is
		 * allowed to define both the 64-bit entry point (smbios3) and
		 * the 32-bit entry point (smbios), in which case they should
		 * either both point to the same SMBIOS structure table, or the
		 * table pointed to by the 64-bit entry point should contain a
		 * superset of the table contents pointed to by the 32-bit entry
		 * point (section 5.2)
		 * This implies that the 64-bit entry point should have
		 * precedence if it is defined and supported by the OS. If we
		 * have the 64-bit entry point, but fail to decode it, fall
		 * back to the legacy one (if available)
		 */
		if (efi.smbios3 != EFI_INVALID_TABLE_ADDR) {
			p = dmi_early_remap(efi.smbios3, 32);
			if (p == NULL)
				goto error;
			memcpy_fromio(buf, p, 32);
			dmi_early_unmap(p, 32);

			if (!dmi_smbios3_present(buf)) {
				dmi_available = 1;
				goto out;
			}
		}
		if (efi.smbios == EFI_INVALID_TABLE_ADDR)
			goto error;

		/* This is called as a core_initcall() because it isn't
		 * needed during early boot.  This also means we can
		 * iounmap the space when we're done with it.
		 */
		p = dmi_early_remap(efi.smbios, 32);
		if (p == NULL)
			goto error;
		memcpy_fromio(buf, p, 32);
		dmi_early_unmap(p, 32);

		if (!dmi_present(buf)) {
			dmi_available = 1;
			goto out;
		}
	} else if (IS_ENABLED(CONFIG_DMI_SCAN_MACHINE_NON_EFI_FALLBACK)) {
		p = dmi_early_remap(0xF0000, 0x10000);
		if (p == NULL)
			goto error;

		/*
		 * Iterate over all possible DMI header addresses q.
		 * Maintain the 32 bytes around q in buf.  On the
		 * first iteration, substitute zero for the
		 * out-of-range bytes so there is no chance of falsely
		 * detecting an SMBIOS header.
		 */
		memset(buf, 0, 16);
		for (q = p; q < p + 0x10000; q += 16) {
			memcpy_fromio(buf + 16, q, 16);
			if (!dmi_smbios3_present(buf) || !dmi_present(buf)) {
				dmi_available = 1;
				dmi_early_unmap(p, 0x10000);
				goto out;
			}
			memcpy(buf, buf + 16, 16);
		}
		dmi_early_unmap(p, 0x10000);
	}
 error:
	pr_info("DMI not present or invalid.\n");
 out:
	dmi_initialized = 1;
}

static ssize_t raw_table_read(struct file *file, struct kobject *kobj,
			      struct bin_attribute *attr, char *buf,
			      loff_t pos, size_t count)
{
	memcpy(buf, attr->private + pos, count);
	return count;
}

static BIN_ATTR(smbios_entry_point, S_IRUSR, raw_table_read, NULL, 0);
static BIN_ATTR(DMI, S_IRUSR, raw_table_read, NULL, 0);

static int __init dmi_init(void)
{
	struct kobject *tables_kobj;
	u8 *dmi_table;
	int ret = -ENOMEM;

	if (!dmi_available) {
		ret = -ENODATA;
		goto err;
	}

	/*
	 * Set up dmi directory at /sys/firmware/dmi. This entry should stay
	 * even after farther error, as it can be used by other modules like
	 * dmi-sysfs.
	 */
	dmi_kobj = kobject_create_and_add("dmi", firmware_kobj);
	if (!dmi_kobj)
		goto err;

	tables_kobj = kobject_create_and_add("tables", dmi_kobj);
	if (!tables_kobj)
		goto err;

	dmi_table = dmi_remap(dmi_base, dmi_len);
	if (!dmi_table)
		goto err_tables;

	bin_attr_smbios_entry_point.size = smbios_entry_point_size;
	bin_attr_smbios_entry_point.private = smbios_entry_point;
	ret = sysfs_create_bin_file(tables_kobj, &bin_attr_smbios_entry_point);
	if (ret)
		goto err_unmap;

	bin_attr_DMI.size = dmi_len;
	bin_attr_DMI.private = dmi_table;
	ret = sysfs_create_bin_file(tables_kobj, &bin_attr_DMI);
	if (!ret)
		return 0;

	sysfs_remove_bin_file(tables_kobj,
			      &bin_attr_smbios_entry_point);
 err_unmap:
	dmi_unmap(dmi_table);
 err_tables:
	kobject_del(tables_kobj);
	kobject_put(tables_kobj);
 err:
	pr_err("dmi: Firmware registration failed.\n");

	return ret;
}
subsys_initcall(dmi_init);

/**
 * dmi_set_dump_stack_arch_desc - set arch description for dump_stack()
 *
 * Invoke dump_stack_set_arch_desc() with DMI system information so that
 * DMI identifiers are printed out on task dumps.  Arch boot code should
 * call this function after dmi_scan_machine() if it wants to print out DMI
 * identifiers on task dumps.
 */
void __init dmi_set_dump_stack_arch_desc(void)
{
	dump_stack_set_arch_desc("%s", dmi_ids_string);
}

/**
 *	dmi_matches - check if dmi_system_id structure matches system DMI data
 *	@dmi: pointer to the dmi_system_id structure to check
 */
static bool dmi_matches(const struct dmi_system_id *dmi)
{
	int i;

	WARN(!dmi_initialized, KERN_ERR "dmi check: not initialized yet.\n");

	for (i = 0; i < ARRAY_SIZE(dmi->matches); i++) {
		int s = dmi->matches[i].slot;
		if (s == DMI_NONE)
			break;
		if (dmi_ident[s]) {
			if (!dmi->matches[i].exact_match &&
			    strstr(dmi_ident[s], dmi->matches[i].substr))
				continue;
			else if (dmi->matches[i].exact_match &&
				 !strcmp(dmi_ident[s], dmi->matches[i].substr))
				continue;
		}

		/* No match */
		return false;
	}
	return true;
}

/**
 *	dmi_is_end_of_table - check for end-of-table marker
 *	@dmi: pointer to the dmi_system_id structure to check
 */
static bool dmi_is_end_of_table(const struct dmi_system_id *dmi)
{
	return dmi->matches[0].slot == DMI_NONE;
}

/**
 *	dmi_check_system - check system DMI data
 *	@list: array of dmi_system_id structures to match against
 *		All non-null elements of the list must match
 *		their slot's (field index's) data (i.e., each
 *		list string must be a substring of the specified
 *		DMI slot's string data) to be considered a
 *		successful match.
 *
 *	Walk the blacklist table running matching functions until someone
 *	returns non zero or we hit the end. Callback function is called for
 *	each successful match. Returns the number of matches.
 */
int dmi_check_system(const struct dmi_system_id *list)
{
	int count = 0;
	const struct dmi_system_id *d;

	for (d = list; !dmi_is_end_of_table(d); d++)
		if (dmi_matches(d)) {
			count++;
			if (d->callback && d->callback(d))
				break;
		}

	return count;
}
EXPORT_SYMBOL(dmi_check_system);

/**
 *	dmi_first_match - find dmi_system_id structure matching system DMI data
 *	@list: array of dmi_system_id structures to match against
 *		All non-null elements of the list must match
 *		their slot's (field index's) data (i.e., each
 *		list string must be a substring of the specified
 *		DMI slot's string data) to be considered a
 *		successful match.
 *
 *	Walk the blacklist table until the first match is found.  Return the
 *	pointer to the matching entry or NULL if there's no match.
 */
const struct dmi_system_id *dmi_first_match(const struct dmi_system_id *list)
{
	const struct dmi_system_id *d;

	for (d = list; !dmi_is_end_of_table(d); d++)
		if (dmi_matches(d))
			return d;

	return NULL;
}
EXPORT_SYMBOL(dmi_first_match);

/**
 *	dmi_get_system_info - return DMI data value
 *	@field: data index (see enum dmi_field)
 *
 *	Returns one DMI data value, can be used to perform
 *	complex DMI data checks.
 */
const char *dmi_get_system_info(int field)
{
	return dmi_ident[field];
}
EXPORT_SYMBOL(dmi_get_system_info);

/**
 * dmi_name_in_serial - Check if string is in the DMI product serial information
 * @str: string to check for
 */
int dmi_name_in_serial(const char *str)
{
	int f = DMI_PRODUCT_SERIAL;
	if (dmi_ident[f] && strstr(dmi_ident[f], str))
		return 1;
	return 0;
}

/**
 *	dmi_name_in_vendors - Check if string is in the DMI system or board vendor name
 *	@str: Case sensitive Name
 */
int dmi_name_in_vendors(const char *str)
{
	static int fields[] = { DMI_SYS_VENDOR, DMI_BOARD_VENDOR, DMI_NONE };
	int i;
	for (i = 0; fields[i] != DMI_NONE; i++) {
		int f = fields[i];
		if (dmi_ident[f] && strstr(dmi_ident[f], str))
			return 1;
	}
	return 0;
}
EXPORT_SYMBOL(dmi_name_in_vendors);

/**
 *	dmi_find_device - find onboard device by type/name
 *	@type: device type or %DMI_DEV_TYPE_ANY to match all device types
 *	@name: device name string or %NULL to match all
 *	@from: previous device found in search, or %NULL for new search.
 *
 *	Iterates through the list of known onboard devices. If a device is
 *	found with a matching @vendor and @device, a pointer to its device
 *	structure is returned.  Otherwise, %NULL is returned.
 *	A new search is initiated by passing %NULL as the @from argument.
 *	If @from is not %NULL, searches continue from next device.
 */
const struct dmi_device *dmi_find_device(int type, const char *name,
				    const struct dmi_device *from)
{
	const struct list_head *head = from ? &from->list : &dmi_devices;
	struct list_head *d;

	for (d = head->next; d != &dmi_devices; d = d->next) {
		const struct dmi_device *dev =
			list_entry(d, struct dmi_device, list);

		if (((type == DMI_DEV_TYPE_ANY) || (dev->type == type)) &&
		    ((name == NULL) || (strcmp(dev->name, name) == 0)))
			return dev;
	}

	return NULL;
}
EXPORT_SYMBOL(dmi_find_device);

/**
 *	dmi_get_date - parse a DMI date
 *	@field:	data index (see enum dmi_field)
 *	@yearp: optional out parameter for the year
 *	@monthp: optional out parameter for the month
 *	@dayp: optional out parameter for the day
 *
 *	The date field is assumed to be in the form resembling
 *	[mm[/dd]]/yy[yy] and the result is stored in the out
 *	parameters any or all of which can be omitted.
 *
 *	If the field doesn't exist, all out parameters are set to zero
 *	and false is returned.  Otherwise, true is returned with any
 *	invalid part of date set to zero.
 *
 *	On return, year, month and day are guaranteed to be in the
 *	range of [0,9999], [0,12] and [0,31] respectively.
 */
bool dmi_get_date(int field, int *yearp, int *monthp, int *dayp)
{
	int year = 0, month = 0, day = 0;
	bool exists;
	const char *s, *y;
	char *e;

	s = dmi_get_system_info(field);
	exists = s;
	if (!exists)
		goto out;

	/*
	 * Determine year first.  We assume the date string resembles
	 * mm/dd/yy[yy] but the original code extracted only the year
	 * from the end.  Keep the behavior in the spirit of no
	 * surprises.
	 */
	y = strrchr(s, '/');
	if (!y)
		goto out;

	y++;
	year = simple_strtoul(y, &e, 10);
	if (y != e && year < 100) {	/* 2-digit year */
		year += 1900;
		if (year < 1996)	/* no dates < spec 1.0 */
			year += 100;
	}
	if (year > 9999)		/* year should fit in %04d */
		year = 0;

	/* parse the mm and dd */
	month = simple_strtoul(s, &e, 10);
	if (s == e || *e != '/' || !month || month > 12) {
		month = 0;
		goto out;
	}

	s = e + 1;
	day = simple_strtoul(s, &e, 10);
	if (s == y || s == e || *e != '/' || day > 31)
		day = 0;
out:
	if (yearp)
		*yearp = year;
	if (monthp)
		*monthp = month;
	if (dayp)
		*dayp = day;
	return exists;
}
EXPORT_SYMBOL(dmi_get_date);

/**
 *	dmi_walk - Walk the DMI table and get called back for every record
 *	@decode: Callback function
 *	@private_data: Private data to be passed to the callback function
 *
 *	Returns -1 when the DMI table can't be reached, 0 on success.
 */
int dmi_walk(void (*decode)(const struct dmi_header *, void *),
	     void *private_data)
{
	u8 *buf;

	if (!dmi_available)
		return -1;

	buf = dmi_remap(dmi_base, dmi_len);
	if (buf == NULL)
		return -1;

	dmi_decode_table(buf, decode, private_data);

	dmi_unmap(buf);
	return 0;
}
EXPORT_SYMBOL_GPL(dmi_walk);

/**
 * dmi_match - compare a string to the dmi field (if exists)
 * @f: DMI field identifier
 * @str: string to compare the DMI field to
 *
 * Returns true if the requested field equals to the str (including NULL).
 */
bool dmi_match(enum dmi_field f, const char *str)
{
	const char *info = dmi_get_system_info(f);

	if (info == NULL || str == NULL)
		return info == str;

	return !strcmp(info, str);
}
EXPORT_SYMBOL_GPL(dmi_match);

void dmi_memdev_name(u16 handle, const char **bank, const char **device)
{
	int n;

	if (dmi_memdev == NULL)
		return;

	for (n = 0; n < dmi_memdev_nr; n++) {
		if (handle == dmi_memdev[n].handle) {
			*bank = dmi_memdev[n].bank;
			*device = dmi_memdev[n].device;
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(dmi_memdev_name);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * linux/drivers/firmware/edd.c
 *  Copyright (C) 2002, 2003, 2004 Dell Inc.
 *  by Matt Domsch <Matt_Domsch@dell.com>
 *  disk signature by Matt Domsch, Andrew Wilks, and Sandeep K. Shandilya
 *  legacy CHS by Patrick J. LoPresti <patl@users.sourceforge.net>
 *
 * BIOS Enhanced Disk Drive Services (EDD)
 * conformant to T13 Committee www.t13.org
 *   projects 1572D, 1484D, 1386D, 1226DT
 *
 * This code takes information provided by BIOS EDD calls
 * fn41 - Check Extensions Present and
 * fn48 - Get Device Parameters with EDD extensions
 * made in setup.S, copied to safe structures in setup.c,
 * and presents it in sysfs.
 *
 * Please see http://linux.dell.com/edd/results.html for
 * the list of BIOSs which have been reported to implement EDD.
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

#include <linux/module.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/limits.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/blkdev.h>
#include <linux/edd.h>

#define EDD_VERSION "0.16"
#define EDD_DATE    "2004-Jun-25"

MODULE_AUTHOR("Matt Domsch <Matt_Domsch@Dell.com>");
MODULE_DESCRIPTION("sysfs interface to BIOS EDD information");
MODULE_LICENSE("GPL");
MODULE_VERSION(EDD_VERSION);

#define left (PAGE_SIZE - (p - buf) - 1)

struct edd_device {
	unsigned int index;
	unsigned int mbr_signature;
	struct edd_info *info;
	struct kobject kobj;
};

struct edd_attribute {
	struct attribute attr;
	ssize_t(*show) (struct edd_device * edev, char *buf);
	int (*test) (struct edd_device * edev);
};

/* forward declarations */
static int edd_dev_is_type(struct edd_device *edev, const char *type);
static struct pci_dev *edd_get_pci_dev(struct edd_device *edev);

static struct edd_device *edd_devices[EDD_MBR_SIG_MAX];

#define EDD_DEVICE_ATTR(_name,_mode,_show,_test) \
struct edd_attribute edd_attr_##_name = { 	\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.show	= _show,				\
	.test	= _test,				\
};

static int
edd_has_mbr_signature(struct edd_device *edev)
{
	return edev->index < min_t(unsigned char, edd.mbr_signature_nr, EDD_MBR_SIG_MAX);
}

static int
edd_has_edd_info(struct edd_device *edev)
{
	return edev->index < min_t(unsigned char, edd.edd_info_nr, EDDMAXNR);
}

static inline struct edd_info *
edd_dev_get_info(struct edd_device *edev)
{
	return edev->info;
}

static inline void
edd_dev_set_info(struct edd_device *edev, int i)
{
	edev->index = i;
	if (edd_has_mbr_signature(edev))
		edev->mbr_signature = edd.mbr_signature[i];
	if (edd_has_edd_info(edev))
		edev->info = &edd.edd_info[i];
}

#define to_edd_attr(_attr) container_of(_attr,struct edd_attribute,attr)
#define to_edd_device(obj) container_of(obj,struct edd_device,kobj)

static ssize_t
edd_attr_show(struct kobject * kobj, struct attribute *attr, char *buf)
{
	struct edd_device *dev = to_edd_device(kobj);
	struct edd_attribute *edd_attr = to_edd_attr(attr);
	ssize_t ret = -EIO;

	if (edd_attr->show)
		ret = edd_attr->show(dev, buf);
	return ret;
}

static const struct sysfs_ops edd_attr_ops = {
	.show = edd_attr_show,
};

static ssize_t
edd_show_host_bus(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	int i;

	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	for (i = 0; i < 4; i++) {
		if (isprint(info->params.host_bus_type[i])) {
			p += scnprintf(p, left, "%c", info->params.host_bus_type[i]);
		} else {
			p += scnprintf(p, left, " ");
		}
	}

	if (!strncmp(info->params.host_bus_type, "ISA", 3)) {
		p += scnprintf(p, left, "\tbase_address: %x\n",
			     info->params.interface_path.isa.base_address);
	} else if (!strncmp(info->params.host_bus_type, "PCIX", 4) ||
		   !strncmp(info->params.host_bus_type, "PCI", 3) ||
		   !strncmp(info->params.host_bus_type, "XPRS", 4)) {
		p += scnprintf(p, left,
			     "\t%02x:%02x.%d  channel: %u\n",
			     info->params.interface_path.pci.bus,
			     info->params.interface_path.pci.slot,
			     info->params.interface_path.pci.function,
			     info->params.interface_path.pci.channel);
	} else if (!strncmp(info->params.host_bus_type, "IBND", 4) ||
		   !strncmp(info->params.host_bus_type, "HTPT", 4)) {
		p += scnprintf(p, left,
			     "\tTBD: %llx\n",
			     info->params.interface_path.ibnd.reserved);

	} else {
		p += scnprintf(p, left, "\tunknown: %llx\n",
			     info->params.interface_path.unknown.reserved);
	}
	return (p - buf);
}

static ssize_t
edd_show_interface(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	int i;

	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	for (i = 0; i < 8; i++) {
		if (isprint(info->params.interface_type[i])) {
			p += scnprintf(p, left, "%c", info->params.interface_type[i]);
		} else {
			p += scnprintf(p, left, " ");
		}
	}
	if (!strncmp(info->params.interface_type, "ATAPI", 5)) {
		p += scnprintf(p, left, "\tdevice: %u  lun: %u\n",
			     info->params.device_path.atapi.device,
			     info->params.device_path.atapi.lun);
	} else if (!strncmp(info->params.interface_type, "ATA", 3)) {
		p += scnprintf(p, left, "\tdevice: %u\n",
			     info->params.device_path.ata.device);
	} else if (!strncmp(info->params.interface_type, "SCSI", 4)) {
		p += scnprintf(p, left, "\tid: %u  lun: %llu\n",
			     info->params.device_path.scsi.id,
			     info->params.device_path.scsi.lun);
	} else if (!strncmp(info->params.interface_type, "USB", 3)) {
		p += scnprintf(p, left, "\tserial_number: %llx\n",
			     info->params.device_path.usb.serial_number);
	} else if (!strncmp(info->params.interface_type, "1394", 4)) {
		p += scnprintf(p, left, "\teui: %llx\n",
			     info->params.device_path.i1394.eui);
	} else if (!strncmp(info->params.interface_type, "FIBRE", 5)) {
		p += scnprintf(p, left, "\twwid: %llx lun: %llx\n",
			     info->params.device_path.fibre.wwid,
			     info->params.device_path.fibre.lun);
	} else if (!strncmp(info->params.interface_type, "I2O", 3)) {
		p += scnprintf(p, left, "\tidentity_tag: %llx\n",
			     info->params.device_path.i2o.identity_tag);
	} else if (!strncmp(info->params.interface_type, "RAID", 4)) {
		p += scnprintf(p, left, "\tidentity_tag: %x\n",
			     info->params.device_path.raid.array_number);
	} else if (!strncmp(info->params.interface_type, "SATA", 4)) {
		p += scnprintf(p, left, "\tdevice: %u\n",
			     info->params.device_path.sata.device);
	} else {
		p += scnprintf(p, left, "\tunknown: %llx %llx\n",
			     info->params.device_path.unknown.reserved1,
			     info->params.device_path.unknown.reserved2);
	}

	return (p - buf);
}

/**
 * edd_show_raw_data() - copies raw data to buffer for userspace to parse
 * @edev: target edd_device
 * @buf: output buffer
 *
 * Returns: number of bytes written, or -EINVAL on failure
 */
static ssize_t
edd_show_raw_data(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	ssize_t len = sizeof (info->params);
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	if (!(info->params.key == 0xBEDD || info->params.key == 0xDDBE))
		len = info->params.length;

	/* In case of buggy BIOSs */
	if (len > (sizeof(info->params)))
		len = sizeof(info->params);

	memcpy(buf, &info->params, len);
	return len;
}

static ssize_t
edd_show_version(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	p += scnprintf(p, left, "0x%02x\n", info->version);
	return (p - buf);
}

static ssize_t
edd_show_mbr_signature(struct edd_device *edev, char *buf)
{
	char *p = buf;
	p += scnprintf(p, left, "0x%08x\n", edev->mbr_signature);
	return (p - buf);
}

static ssize_t
edd_show_extensions(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	if (info->interface_support & EDD_EXT_FIXED_DISK_ACCESS) {
		p += scnprintf(p, left, "Fixed disk access\n");
	}
	if (info->interface_support & EDD_EXT_DEVICE_LOCKING_AND_EJECTING) {
		p += scnprintf(p, left, "Device locking and ejecting\n");
	}
	if (info->interface_support & EDD_EXT_ENHANCED_DISK_DRIVE_SUPPORT) {
		p += scnprintf(p, left, "Enhanced Disk Drive support\n");
	}
	if (info->interface_support & EDD_EXT_64BIT_EXTENSIONS) {
		p += scnprintf(p, left, "64-bit extensions\n");
	}
	return (p - buf);
}

static ssize_t
edd_show_info_flags(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	if (info->params.info_flags & EDD_INFO_DMA_BOUNDARY_ERROR_TRANSPARENT)
		p += scnprintf(p, left, "DMA boundary error transparent\n");
	if (info->params.info_flags & EDD_INFO_GEOMETRY_VALID)
		p += scnprintf(p, left, "geometry valid\n");
	if (info->params.info_flags & EDD_INFO_REMOVABLE)
		p += scnprintf(p, left, "removable\n");
	if (info->params.info_flags & EDD_INFO_WRITE_VERIFY)
		p += scnprintf(p, left, "write verify\n");
	if (info->params.info_flags & EDD_INFO_MEDIA_CHANGE_NOTIFICATION)
		p += scnprintf(p, left, "media change notification\n");
	if (info->params.info_flags & EDD_INFO_LOCKABLE)
		p += scnprintf(p, left, "lockable\n");
	if (info->params.info_flags & EDD_INFO_NO_MEDIA_PRESENT)
		p += scnprintf(p, left, "no media present\n");
	if (info->params.info_flags & EDD_INFO_USE_INT13_FN50)
		p += scnprintf(p, left, "use int13 fn50\n");
	return (p - buf);
}

static ssize_t
edd_show_legacy_max_cylinder(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	p += snprintf(p, left, "%u\n", info->legacy_max_cylinder);
	return (p - buf);
}

static ssize_t
edd_show_legacy_max_head(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	p += snprintf(p, left, "%u\n", info->legacy_max_head);
	return (p - buf);
}

static ssize_t
edd_show_legacy_sectors_per_track(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	p += snprintf(p, left, "%u\n", info->legacy_sectors_per_track);
	return (p - buf);
}

static ssize_t
edd_show_default_cylinders(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	p += scnprintf(p, left, "%u\n", info->params.num_default_cylinders);
	return (p - buf);
}

static ssize_t
edd_show_default_heads(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	p += scnprintf(p, left, "%u\n", info->params.num_default_heads);
	return (p - buf);
}

static ssize_t
edd_show_default_sectors_per_track(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	p += scnprintf(p, left, "%u\n", info->params.sectors_per_track);
	return (p - buf);
}

static ssize_t
edd_show_sectors(struct edd_device *edev, char *buf)
{
	struct edd_info *info;
	char *p = buf;
	if (!edev)
		return -EINVAL;
	info = edd_dev_get_info(edev);
	if (!info || !buf)
		return -EINVAL;

	p += scnprintf(p, left, "%llu\n", info->params.number_of_sectors);
	return (p - buf);
}


/*
 * Some device instances may not have all the above attributes,
 * or the attribute values may be meaningless (i.e. if
 * the device is < EDD 3.0, it won't have host_bus and interface
 * information), so don't bother making files for them.  Likewise
 * if the default_{cylinders,heads,sectors_per_track} values
 * are zero, the BIOS doesn't provide sane values, don't bother
 * creating files for them either.
 */

static int
edd_has_legacy_max_cylinder(struct edd_device *edev)
{
	struct edd_info *info;
	if (!edev)
		return 0;
	info = edd_dev_get_info(edev);
	if (!info)
		return 0;
	return info->legacy_max_cylinder > 0;
}

static int
edd_has_legacy_max_head(struct edd_device *edev)
{
	struct edd_info *info;
	if (!edev)
		return 0;
	info = edd_dev_get_info(edev);
	if (!info)
		return 0;
	return info->legacy_max_head > 0;
}

static int
edd_has_legacy_sectors_per_track(struct edd_device *edev)
{
	struct edd_info *info;
	if (!edev)
		return 0;
	info = edd_dev_get_info(edev);
	if (!info)
		return 0;
	return info->legacy_sectors_per_track > 0;
}

static int
edd_has_default_cylinders(struct edd_device *edev)
{
	struct edd_info *info;
	if (!edev)
		return 0;
	info = edd_dev_get_info(edev);
	if (!info)
		return 0;
	return info->params.num_default_cylinders > 0;
}

static int
edd_has_default_heads(struct edd_device *edev)
{
	struct edd_info *info;
	if (!edev)
		return 0;
	info = edd_dev_get_info(edev);
	if (!info)
		return 0;
	return info->params.num_default_heads > 0;
}

static int
edd_has_default_sectors_per_track(struct edd_device *edev)
{
	struct edd_info *info;
	if (!edev)
		return 0;
	info = edd_dev_get_info(edev);
	if (!info)
		return 0;
	return info->params.sectors_per_track > 0;
}

static int
edd_has_edd30(struct edd_device *edev)
{
	struct edd_info *info;
	int i;
	u8 csum = 0;

	if (!edev)
		return 0;
	info = edd_dev_get_info(edev);
	if (!info)
		return 0;

	if (!(info->params.key == 0xBEDD || info->params.key == 0xDDBE)) {
		return 0;
	}


	/* We support only T13 spec */
	if (info->params.device_path_info_length != 44)
		return 0;

	for (i = 30; i < info->params.device_path_info_length + 30; i++)
		csum += *(((u8 *)&info->params) + i);

	if (csum)
		return 0;

	return 1;
}


static EDD_DEVICE_ATTR(raw_data, 0444, edd_show_raw_data, edd_has_edd_info);
static EDD_DEVICE_ATTR(version, 0444, edd_show_version, edd_has_edd_info);
static EDD_DEVICE_ATTR(extensions, 0444, edd_show_extensions, edd_has_edd_info);
static EDD_DEVICE_ATTR(info_flags, 0444, edd_show_info_flags, edd_has_edd_info);
static EDD_DEVICE_ATTR(sectors, 0444, edd_show_sectors, edd_has_edd_info);
static EDD_DEVICE_ATTR(legacy_max_cylinder, 0444,
                       edd_show_legacy_max_cylinder,
		       edd_has_legacy_max_cylinder);
static EDD_DEVICE_ATTR(legacy_max_head, 0444, edd_show_legacy_max_head,
		       edd_has_legacy_max_head);
static EDD_DEVICE_ATTR(legacy_sectors_per_track, 0444,
                       edd_show_legacy_sectors_per_track,
		       edd_has_legacy_sectors_per_track);
static EDD_DEVICE_ATTR(default_cylinders, 0444, edd_show_default_cylinders,
		       edd_has_default_cylinders);
static EDD_DEVICE_ATTR(default_heads, 0444, edd_show_default_heads,
		       edd_has_default_heads);
static EDD_DEVICE_ATTR(default_sectors_per_track, 0444,
		       edd_show_default_sectors_per_track,
		       edd_has_default_sectors_per_track);
static EDD_DEVICE_ATTR(interface, 0444, edd_show_interface, edd_has_edd30);
static EDD_DEVICE_ATTR(host_bus, 0444, edd_show_host_bus, edd_has_edd30);
static EDD_DEVICE_ATTR(mbr_signature, 0444, edd_show_mbr_signature, edd_has_mbr_signature);


/* These are default attributes that are added for every edd
 * device discovered.  There are none.
 */
static struct attribute * def_attrs[] = {
	NULL,
};

/* These attributes are conditional and only added for some devices. */
static struct edd_attribute * edd_attrs[] = {
	&edd_attr_raw_data,
	&edd_attr_version,
	&edd_attr_extensions,
	&edd_attr_info_flags,
	&edd_attr_sectors,
	&edd_attr_legacy_max_cylinder,
	&edd_attr_legacy_max_head,
	&edd_attr_legacy_sectors_per_track,
	&edd_attr_default_cylinders,
	&edd_attr_default_heads,
	&edd_attr_default_sectors_per_track,
	&edd_attr_interface,
	&edd_attr_host_bus,
	&edd_attr_mbr_signature,
	NULL,
};

/**
 *	edd_release - free edd structure
 *	@kobj:	kobject of edd structure
 *
 *	This is called when the refcount of the edd structure
 *	reaches 0. This should happen right after we unregister,
 *	but just in case, we use the release callback anyway.
 */

static void edd_release(struct kobject * kobj)
{
	struct edd_device * dev = to_edd_device(kobj);
	kfree(dev);
}

static struct kobj_type edd_ktype = {
	.release	= edd_release,
	.sysfs_ops	= &edd_attr_ops,
	.default_attrs	= def_attrs,
};

static struct kset *edd_kset;


/**
 * edd_dev_is_type() - is this EDD device a 'type' device?
 * @edev: target edd_device
 * @type: a host bus or interface identifier string per the EDD spec
 *
 * Returns 1 (TRUE) if it is a 'type' device, 0 otherwise.
 */
static int
edd_dev_is_type(struct edd_device *edev, const char *type)
{
	struct edd_info *info;
	if (!edev)
		return 0;
	info = edd_dev_get_info(edev);

	if (type && info) {
		if (!strncmp(info->params.host_bus_type, type, strlen(type)) ||
		    !strncmp(info->params.interface_type, type, strlen(type)))
			return 1;
	}
	return 0;
}

/**
 * edd_get_pci_dev() - finds pci_dev that matches edev
 * @edev: edd_device
 *
 * Returns pci_dev if found, or NULL
 */
static struct pci_dev *
edd_get_pci_dev(struct edd_device *edev)
{
	struct edd_info *info = edd_dev_get_info(edev);

	if (edd_dev_is_type(edev, "PCI") || edd_dev_is_type(edev, "XPRS")) {
		return pci_get_bus_and_slot(info->params.interface_path.pci.bus,
				     PCI_DEVFN(info->params.interface_path.pci.slot,
					       info->params.interface_path.pci.
					       function));
	}
	return NULL;
}

static int
edd_create_symlink_to_pcidev(struct edd_device *edev)
{

	struct pci_dev *pci_dev = edd_get_pci_dev(edev);
	int ret;
	if (!pci_dev)
		return 1;
	ret = sysfs_create_link(&edev->kobj,&pci_dev->dev.kobj,"pci_dev");
	pci_dev_put(pci_dev);
	return ret;
}

static inline void
edd_device_unregister(struct edd_device *edev)
{
	kobject_put(&edev->kobj);
}

static void edd_populate_dir(struct edd_device * edev)
{
	struct edd_attribute * attr;
	int error = 0;
	int i;

	for (i = 0; (attr = edd_attrs[i]) && !error; i++) {
		if (!attr->test ||
		    (attr->test && attr->test(edev)))
			error = sysfs_create_file(&edev->kobj,&attr->attr);
	}

	if (!error) {
		edd_create_symlink_to_pcidev(edev);
	}
}

static int
edd_device_register(struct edd_device *edev, int i)
{
	int error;

	if (!edev)
		return 1;
	edd_dev_set_info(edev, i);
	edev->kobj.kset = edd_kset;
	error = kobject_init_and_add(&edev->kobj, &edd_ktype, NULL,
				     "int13_dev%02x", 0x80 + i);
	if (!error) {
		edd_populate_dir(edev);
		kobject_uevent(&edev->kobj, KOBJ_ADD);
	}
	return error;
}

static inline int edd_num_devices(void)
{
	return max_t(unsigned char,
		     min_t(unsigned char, EDD_MBR_SIG_MAX, edd.mbr_signature_nr),
		     min_t(unsigned char, EDDMAXNR, edd.edd_info_nr));
}

/**
 * edd_init() - creates sysfs tree of EDD data
 */
static int __init
edd_init(void)
{
	int i;
	int rc=0;
	struct edd_device *edev;

	printk(KERN_INFO "BIOS EDD facility v%s %s, %d devices found\n",
	       EDD_VERSION, EDD_DATE, edd_num_devices());

	if (!edd_num_devices()) {
		printk(KERN_INFO "EDD information not available.\n");
		return -ENODEV;
	}

	edd_kset = kset_create_and_add("edd", NULL, firmware_kobj);
	if (!edd_kset)
		return -ENOMEM;

	for (i = 0; i < edd_num_devices(); i++) {
		edev = kzalloc(sizeof (*edev), GFP_KERNEL);
		if (!edev) {
			rc = -ENOMEM;
			goto out;
		}

		rc = edd_device_register(edev, i);
		if (rc) {
			kfree(edev);
			goto out;
		}
		edd_devices[i] = edev;
	}

	return 0;

out:
	while (--i >= 0)
		edd_device_unregister(edd_devices[i]);
	kset_unregister(edd_kset);
	return rc;
}

static void __exit
edd_exit(void)
{
	int i;
	struct edd_device *edev;

	for (i = 0; i < edd_num_devices(); i++) {
		if ((edev = edd_devices[i]))
			edd_device_unregister(edev);
	}
	kset_unregister(edd_kset);
}

late_initcall(edd_init);
module_exit(edd_exit);
                  menu "EFI (Extensible Firmware Interface) Support"
	depends on EFI

config EFI_VARS
	tristate "EFI Variable Support via sysfs"
	depends on EFI
	default n
	help
	  If you say Y here, you are able to get EFI (Extensible Firmware
	  Interface) variable information via sysfs.  You may read,
	  write, create, and destroy EFI variables through this interface.

	  Note that using this driver in concert with efibootmgr requires
	  at least test release version 0.5.0-test3 or later, which is
	  available from:
	  <http://linux.dell.com/efibootmgr/testing/efibootmgr-0.5.0-test3.tar.gz>

	  Subsequent efibootmgr releases may be found at:
	  <http://github.com/vathpela/efibootmgr>

config EFI_ESRT
	bool
	depends on EFI && !IA64
	default y

config EFI_VARS_PSTORE
	tristate "Register efivars backend for pstore"
	depends on EFI_VARS && PSTORE
	default y
	help
	  Say Y here to enable use efivars as a backend to pstore. This
	  will allow writing console messages, crash dumps, or anything
	  else supported by pstore to EFI variables.

config EFI_VARS_PSTORE_DEFAULT_DISABLE
	bool "Disable using efivars as a pstore backend by default"
	depends on EFI_VARS_PSTORE
	default n
	help
	  Saying Y here will disable the use of efivars as a storage
	  backend for pstore by default. This setting can be overridden
	  using the efivars module's pstore_disable parameter.

config EFI_RUNTIME_MAP
	bool "Export efi runtime maps to sysfs"
	depends on X86 && EFI && KEXEC_CORE
	default y
	help
	  Export efi runtime memory maps to /sys/firmware/efi/runtime-map.
	  That memory map is used for example by kexec to set up efi virtual
	  mapping the 2nd kernel, but can also be used for debugging purposes.

	  See also Documentation/ABI/testing/sysfs-firmware-efi-runtime-map.

config EFI_FAKE_MEMMAP
	bool "Enable EFI fake memory map"
	depends on EFI && X86
	default n
	help
	  Saying Y here will enable "efi_fake_mem" boot option.
	  By specifying this parameter, you can add arbitrary attribute
	  to specific memory range by updating original (firmware provided)
	  EF