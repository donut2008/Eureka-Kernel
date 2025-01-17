ID_BRIDGE_CONTROL_STATUS)
		printk(
	"%s""bridge: secondary_status: 0x%04x, control: 0x%04x\n",
	pfx, pcie->bridge.secondary_status, pcie->bridge.control);

	/* Fatal errors call __ghes_panic() before AER handler prints this */
	if ((pcie->validation_bits & CPER_PCIE_VALID_AER_INFO) &&
	    (gdata->error_severity & CPER_SEV_FATAL)) {
		struct aer_capability_regs *aer;

		aer = (struct aer_capability_regs *)pcie->aer_info;
		printk("%saer_uncor_status: 0x%08x, aer_uncor_mask: 0x%08x\n",
		       pfx, aer->uncor_status, aer->uncor_mask);
		printk("%saer_uncor_severity: 0x%08x\n",
		       pfx, aer->uncor_severity);
		printk("%sTLP Header: %08x %08x %08x %08x\n", pfx,
		       aer->header_log.dw0, aer->header_log.dw1,
		       aer->header_log.dw2, aer->header_log.dw3);
	}
}

static void cper_estatus_print_section(
	const char *pfx, const struct acpi_hest_generic_data *gdata, int sec_no)
{
	uuid_le *sec_type = (uuid_le *)gdata->section_type;
	__u16 severity;
	char newpfx[64];

	severity = gdata->error_severity;
	printk("%s""Error %d, type: %s\n", pfx, sec_no,
	       cper_severity_str(severity));
	if (gdata->validation_bits & CPER_SEC_VALID_FRU_ID)
		printk("%s""fru_id: %pUl\n", pfx, (uuid_le *)gdata->fru_id);
	if (gdata->validation_bits & CPER_SEC_VALID_FRU_TEXT)
		printk("%s""fru_text: %.20s\n", pfx, gdata->fru_text);

	snprintf(newpfx, sizeof(newpfx), "%s%s", pfx, INDENT_SP);
	if (!uuid_le_cmp(*sec_type, CPER_SEC_PROC_GENERIC)) {
		struct cper_sec_proc_generic *proc_err = (void *)(gdata + 1);
		printk("%s""section_type: general processor error\n", newpfx);
		if (gdata->error_data_length >= sizeof(*proc_err))
			cper_print_proc_generic(newpfx, proc_err);
		else
			goto err_section_too_small;
	} else if (!uuid_le_cmp(*sec_type, CPER_SEC_PLATFORM_MEM)) {
		struct cper_sec_mem_err *mem_err = (void *)(gdata + 1);
		printk("%s""section_type: memory error\n", newpfx);
		if (gdata->error_data_length >=
		    sizeof(struct cper_sec_mem_err_old))
			cper_print_mem(newpfx, mem_err,
				       gdata->error_data_length);
		else
			goto err_section_too_small;
	} else if (!uuid_le_cmp(*sec_type, CPER_SEC_PCIE)) {
		struct cper_sec_pcie *pcie = (void *)(gdata + 1);
		printk("%s""section_type: PCIe error\n", newpfx);
		if (gdata->error_data_length >= sizeof(*pcie))
			cper_print_pcie(newpfx, pcie, gdata);
		else
			goto err_section_too_small;
	} else
		printk("%s""section type: unknown, %pUl\n", newpfx, sec_type);

	return;

err_section_too_small:
	pr_err(FW_WARN "error section length is too small\n");
}

void cper_estatus_print(const char *pfx,
			const struct acpi_hest_generic_status *estatus)
{
	struct acpi_hest_generic_data *gdata;
	unsigned int data_len, gedata_len;
	int sec_no = 0;
	char newpfx[64];
	__u16 severity;

	severity = estatus->error_severity;
	if (severity == CPER_SEV_CORRECTED)
		printk("%s%s\n", pfx,
		       "It has been corrected by h/w "
		       "and requires no further action");
	printk("%s""event severity: %s\n", pfx, cper_severity_str(severity));
	data_len = estatus->data_length;
	gdata = (struct acpi_hest_generic_data *)(estatus + 1);
	snprintf(newpfx, sizeof(newpfx), "%s%s", pfx, INDENT_SP);
	while (data_len >= sizeof(*gdata)) {
		gedata_len = gdata->error_data_length;
		cper_estatus_print_section(newpfx, gdata, sec_no);
		data_len -= gedata_len + sizeof(*gdata);
		gdata = (void *)(gdata + 1) + gedata_len;
		sec_no++;
	}
}
EXPORT_SYMBOL_GPL(cper_estatus_print);

int cper_estatus_check_header(const struct acpi_hest_generic_status *estatus)
{
	if (estatus->data_length &&
	    estatus->data_length < sizeof(struct acpi_hest_generic_data))
		return -EINVAL;
	if (estatus->raw_data_length &&
	    estatus->raw_data_offset < sizeof(*estatus) + estatus->data_length)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL_GPL(cper_estatus_check_header);

int cper_estatus_check(const struct acpi_hest_generic_status *estatus)
{
	struct acpi_hest_generic_data *gdata;
	unsigned int data_len, gedata_len;
	int rc;

	rc = cper_estatus_check_header(estatus);
	if (rc)
		return rc;
	data_len = estatus->data_length;
	gdata = (struct acpi_hest_generic_data *)(estatus + 1);
	while (data_len >= sizeof(*gdata)) {
		gedata_len = gdata->error_data_length;
		if (gedata_len > data_len - sizeof(*gdata))
			return -EINVAL;
		data_len -= gedata_len + sizeof(*gdata);
		gdata = (void *)(gdata + 1) + gedata_len;
	}
	if (data_len)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL_GPL(cper_estatus_check);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 #include <linux/efi.h>
#include <linux/module.h>
#include <linux/pstore.h>
#include <linux/slab.h>
#include <linux/ucs2_string.h>

#define DUMP_NAME_LEN 52

static bool efivars_pstore_disable =
	IS_ENABLED(CONFIG_EFI_VARS_PSTORE_DEFAULT_DISABLE);

module_param_named(pstore_disable, efivars_pstore_disable, bool, 0644);

#define PSTORE_EFI_ATTRIBUTES \
	(EFI_VARIABLE_NON_VOLATILE | \
	 EFI_VARIABLE_BOOTSERVICE_ACCESS | \
	 EFI_VARIABLE_RUNTIME_ACCESS)

static int efi_pstore_open(struct pstore_info *psi)
{
	psi->data = NULL;
	return 0;
}

static int efi_pstore_close(struct pstore_info *psi)
{
	psi->data = NULL;
	return 0;
}

struct pstore_read_data {
	u64 *id;
	enum pstore_type_id *type;
	int *count;
	struct timespec *timespec;
	bool *compressed;
	char **buf;
};

static inline u64 generic_id(unsigned long timestamp,
			     unsigned int part, int count)
{
	return ((u64) timestamp * 100 + part) * 1000 + count;
}

static int efi_pstore_read_func(struct efivar_entry *entry, void *data)
{
	efi_guid_t vendor = LINUX_EFI_CRASH_GUID;
	struct pstore_read_data *cb_data = data;
	char name[DUMP_NAME_LEN], data_type;
	int i;
	int cnt;
	unsigned int part;
	unsigned long time, size;

	if (efi_guidcmp(entry->var.VendorGuid, vendor))
		return 0;

	for (i = 0; i < DUMP_NAME_LEN; i++)
		name[i] = entry->var.VariableName[i];

	if (sscanf(name, "dump-type%u-%u-%d-%lu-%c",
		   cb_data->type, &part, &cnt, &time, &data_type) == 5) {
		*cb_data->id = generic_id(time, part, cnt);
		*cb_data->count = cnt;
		cb_data->timespec->tv_sec = time;
		cb_data->timespec->tv_nsec = 0;
		if (data_type == 'C')
			*cb_data->compressed = true;
		else
			*cb_data->compressed = false;
	} else if (sscanf(name, "dump-type%u-%u-%d-%lu",
		   cb_data->type, &part, &cnt, &time) == 4) {
		*cb_data->id = generic_id(time, part, cnt);
		*cb_data->count = cnt;
		cb_data->timespec->tv_sec = time;
		cb_data->timespec->tv_nsec = 0;
		*cb_data->compressed = false;
	} else if (sscanf(name, "dump-type%u-%u-%lu",
			  cb_data->type, &part, &time) == 3) {
		/*
		 * Check if an old format,
		 * which doesn't support holding
		 * multiple logs, remains.
		 */
		*cb_data->id = generic_id(time, part, 0);
		*cb_data->count = 0;
		cb_data->timespec->tv_sec = time;
		cb_data->timespec->tv_nsec = 0;
		*cb_data->compressed = false;
	} else
		return 0;

	entry->var.DataSize = 1024;
	__efivar_entry_get(entry, &entry->var.Attributes,
			   &entry->var.DataSize, entry->var.Data);
	size = entry->var.DataSize;
	memcpy(*cb_data->buf, entry->var.Data,
	       (size_t)min_t(unsigned long, EFIVARS_DATA_SIZE_MAX, size));

	return size;
}

/**
 * efi_pstore_scan_sysfs_enter
 * @pos: scanning entry
 * @next: next entry
 * @head: list head
 */
static void efi_pstore_scan_sysfs_enter(struct efivar_entry *pos,
					struct efivar_entry *next,
					struct list_head *head)
{
	pos->scanning = true;
	if (&next->list != head)
		next->scanning = true;
}

/**
 * __efi_pstore_scan_sysfs_exit
 * @entry: deleting entry
 * @turn_off_scanning: Check if a scanning flag should be turned off
 */
static inline void __efi_pstore_scan_sysfs_exit(struct efivar_entry *entry,
						bool turn_off_scanning)
{
	if (entry->deleting) {
		list_del(&entry->list);
		efivar_entry_iter_end();
		efivar_unregister(entry);
		efivar_entry_iter_begin();
	} else if (turn_off_scanning)
		entry->scanning = false;
}

/**
 * efi_pstore_scan_sysfs_exit
 * @pos: scanning entry
 * @next: next entry
 * @head: list head
 * @stop: a flag checking if scanning will stop
 */
static void efi_pstore_scan_sysfs_exit(struct efivar_entry *pos,
				       struct efivar_entry *next,
				       struct list_head *head, bool stop)
{
	__efi_pstore_scan_sysfs_exit(pos, true);
	if (stop)
		__efi_pstore_scan_sysfs_exit(next, &next->list != head);
}

/**
 * efi_pstore_sysfs_entry_iter
 *
 * @data: function-specific data to pass to callback
 * @pos: entry to begin iterating from
 *
 * You MUST call efivar_enter_iter_begin() before this function, and
 * efivar_entry_iter_end() afterwards.
 *
 * It is possible to begin iteration from an arbitrary entry within
 * the list by passing @pos. @pos is updated on return to point to
 * the next entry of the last one passed to efi_pstore_read_func().
 * To begin iterating from the beginning of the list @pos must be %NULL.
 */
static int efi_pstore_sysfs_entry_iter(void *data, struct efivar_entry **pos)
{
	struct efivar_entry *entry, *n;
	struct list_head *head = &efivar_sysfs_list;
	int size = 0;

	if (!*pos) {
		list_for_each_entry_safe(entry, n, head, list) {
			efi_pstore_scan_sysfs_enter(entry, n, head);

			size = efi_pstore_read_func(entry, data);
			efi_pstore_scan_sysfs_exit(entry, n, head, size < 0);
			if (size)
				break;
		}
		*pos = n;
		return size;
	}

	list_for_each_entry_safe_from((*pos), n, head, list) {
		efi_pstore_scan_sysfs_enter((*pos), n, head);

		size = efi_pstore_read_func((*pos), data);
		efi_pstore_scan_sysfs_exit((*pos), n, head, size < 0);
		if (size)
			break;
	}
	*pos = n;
	return size;
}

/**
 * efi_pstore_read
 *
 * This function returns a size of NVRAM entry logged via efi_pstore_write().
 * The meaning and behavior of efi_pstore/pstore are as below.
 *
 * size > 0: Got data of an entry logged via efi_pstore_write() successfully,
 *           and pstore filesystem will continue reading subsequent entries.
 * size == 0: Entry was not logged via efi_pstore_write(),
 *            and efi_pstore driver will continue reading subsequent entries.
 * size < 0: Failed to get data of entry logging via efi_pstore_write(),
 *           and pstore will stop reading entry.
 */
static ssize_t efi_pstore_read(u64 *id, enum pstore_type_id *type,
			       int *count, struct timespec *timespec,
			       char **buf, bool *compressed,
			       struct pstore_info *psi)
{
	struct pstore_read_data data;
	ssize_t size;

	data.id = id;
	data.type = type;
	data.count = count;
	data.timespec = timespec;
	data.compressed = compressed;
	data.buf = buf;

	*data.buf = kzalloc(EFIVARS_DATA_SIZE_MAX, GFP_KERNEL);
	if (!*data.buf)
		return -ENOMEM;

	efivar_entry_iter_begin();
	size = efi_pstore_sysfs_entry_iter(&data,
					   (struct efivar_entry **)&psi->data);
	efivar_entry_iter_end();
	if (size <= 0)
		kfree(*data.buf);
	return size;
}

static int efi_pstore_write(enum pstore_type_id type,
		enum kmsg_dump_reason reason, u64 *id,
		unsigned int part, int count, bool compressed, size_t size,
		struct pstore_info *psi)
{
	char name[DUMP_NAME_LEN];
	efi_char16_t efi_name[DUMP_NAME_LEN];
	efi_guid_t vendor = LINUX_EFI_CRASH_GUID;
	int i, ret = 0;

	sprintf(name, "dump-type%u-%u-%d-%lu-%c", type, part, count,
		get_seconds(), compressed ? 'C' : 'D');

	for (i = 0; i < DUMP_NAME_LEN; i++)
		efi_name[i] = name[i];

	efivar_entry_set_safe(efi_name, vendor, PSTORE_EFI_ATTRIBUTES,
			      !pstore_cannot_block_path(reason),
			      size, psi->buf);

	if (reason == KMSG_DUMP_OOPS)
		efivar_run_worker();

	*id = part;
	return ret;
};

struct pstore_erase_data {
	u64 id;
	enum pstore_type_id type;
	int count;
	struct timespec time;
	efi_char16_t *name;
};

/*
 * Clean up an entry with the same name
 */
static int efi_pstore_erase_func(struct efivar_entry *entry, void *data)
{
	struct pstore_erase_data *ed = data;
	efi_guid_t vendor = LINUX_EFI_CRASH_GUID;
	efi_char16_t efi_name_old[DUMP_NAME_LEN];
	efi_char16_t *efi_name = ed->name;
	unsigned long ucs2_len = ucs2_strlen(ed->name);
	char name_old[DUMP_NAME_LEN];
	int i;

	if (efi_guidcmp(entry->var.VendorGuid, vendor))
		return 0;

	if (ucs2_strncmp(entry->var.VariableName,
			  efi_name, (size_t)ucs2_len)) {
		/*
		 * Check if an old format, which doesn't support
		 * holding multiple logs, remains.
		 */
		sprintf(name_old, "dump-type%u-%u-%lu", ed->type,
			(unsigned int)ed->id, ed->time.tv_sec);

		for (i = 0; i < DUMP_NAME_LEN; i++)
			efi_name_old[i] = name_old[i];

		if (ucs2_strncmp(entry->var.VariableName, efi_name_old,
				  ucs2_strlen(efi_name_old)))
			return 0;
	}

	if (entry->scanning) {
		/*
		 * Skip deletion because this entry will be deleted
		 * after scanning is completed.
		 */
		entry->deleting = true;
	} else
		list_del(&entry->list);

	/* found */
	__efivar_entry_delete(entry);

	return 1;
}

static int efi_pstore_erase(enum pstore_type_id type, u64 id, int count,
			    struct timespec time, struct pstore_info *psi)
{
	struct pstore_erase_data edata;
	struct efivar_entry *entry = NULL;
	char name[DUMP_NAME_LEN];
	efi_char16_t efi_name[DUMP_NAME_LEN];
	int found, i;
	unsigned int part;

	do_div(id, 1000);
	part = do_div(id, 100);
	sprintf(name, "dump-type%u-%u-%d-%lu", type, part, count, time.tv_sec);

	for (i = 0; i < DUMP_NAME_LEN; i++)
		efi_name[i] = name[i];

	edata.id = part;
	edata.type = type;
	edata.count = count;
	edata.time = time;
	edata.name = efi_name;

	efivar_entry_iter_begin();
	found = __efivar_entry_iter(efi_pstore_erase_func, &efivar_sysfs_list, &edata, &entry);

	if (found && !entry->scanning) {
		efivar_entry_iter_end();
		efivar_unregister(entry);
	} else
		efivar_entry_iter_end();

	return 0;
}

static struct pstore_info efi_pstore_info = {
	.owner		= THIS_MODULE,
	.name		= "efi",
	.flags		= PSTORE_FLAGS_FRAGILE,
	.open		= efi_pstore_open,
	.close		= efi_pstore_close,
	.read		= efi_pstore_read,
	.write		= efi_pstore_write,
	.erase		= efi_pstore_erase,
};

static __init int efivars_pstore_init(void)
{
	if (!efi_enabled(EFI_RUNTIME_SERVICES))
		return 0;

	if (!efivars_kobject())
		return 0;

	if (efivars_pstore_disable)
		return 0;

	efi_pstore_info.buf = kmalloc(4096, GFP_KERNEL);
	if (!efi_pstore_info.buf)
		return -ENOMEM;

	efi_pstore_info.bufsize = 1024;
	spin_lock_init(&efi_pstore_info.buf_lock);

	if (pstore_register(&efi_pstore_info)) {
		kfree(efi_pstore_info.buf);
		efi_pstore_info.buf = NULL;
		efi_pstore_info.bufsize = 0;
	}

	return 0;
}

static __exit void efivars_pstore_exit(void)
{
}

module_init(efivars_pstore_init);
module_exit(efivars_pstore_exit);

MODULE_DESCRIPTION("EFI variable backend for pstore");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:efivars");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * efi.c - EFI subsystem
 *
 * Copyright (C) 2001,2003,2004 Dell <Matt_Domsch@dell.com>
 * Copyright (C) 2004 Intel Corporation <matthew.e.tolentino@intel.com>
 * Copyright (C) 2013 Tom Gundersen <teg@jklm.no>
 *
 * This code registers /sys/firmware/efi{,/efivars} when EFI is supported,
 * allowing the efivarfs to be mounted or the efivars module to be loaded.
 * The existance of /sys/firmware/efi may also be used by userspace to
 * determine that the system supports EFI.
 *
 * This file is released under the GPLv2.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/early_ioremap.h>

struct efi __read_mostly efi = {
	.mps			= EFI_INVALID_TABLE_ADDR,
	.acpi			= EFI_INVALID_TABLE_ADDR,
	.acpi20			= EFI_INVALID_TABLE_ADDR,
	.smbios			= EFI_INVALID_TABLE_ADDR,
	.smbios3		= EFI_INVALID_TABLE_ADDR,
	.sal_systab		= EFI_INVALID_TABLE_ADDR,
	.boot_info		= EFI_INVALID_TABLE_ADDR,
	.hcdp			= EFI_INVALID_TABLE_ADDR,
	.uga			= EFI_INVALID_TABLE_ADDR,
	.uv_systab		= EFI_INVALID_TABLE_ADDR,
	.fw_vendor		= EFI_INVALID_TABLE_ADDR,
	.runtime		= EFI_INVALID_TABLE_ADDR,
	.config_table		= EFI_INVALID_TABLE_ADDR,
	.esrt			= EFI_INVALID_TABLE_ADDR,
	.properties_table	= EFI_INVALID_TABLE_ADDR,
};
EXPORT_SYMBOL(efi);

static bool disable_runtime;
static int __init setup_noefi(char *arg)
{
	disable_runtime = true;
	return 0;
}
early_param("noefi", setup_noefi);

bool efi_runtime_disabled(void)
{
	return disable_runtime;
}

static int __init parse_efi_cmdline(char *str)
{
	if (!str) {
		pr_warn("need at least one option\n");
		return -EINVAL;
	}

	if (parse_option_str(str, "debug"))
		set_bit(EFI_DBG, &efi.flags);

	if (parse_option_str(str, "noruntime"))
		disable_runtime = true;

	return 0;
}
early_param("efi", parse_efi_cmdline);

struct kobject *efi_kobj;

/*
 * Let's not leave out systab information that snuck into
 * the efivars driver
 */
static ssize_t systab_show(struct kobject *kobj,
			   struct kobj_attribute *attr, char *buf)
{
	char *str = buf;

	if (!kobj || !buf)
		return -EINVAL;

	if (efi.mps != EFI_INVALID_TABLE_ADDR)
		str += sprintf(str, "MPS=0x%lx\n", efi.mps);
	if (efi.acpi20 != EFI_INVALID_TABLE_ADDR)
		str += sprintf(str, "ACPI20=0x%lx\n", efi.acpi20);
	if (efi.acpi != EFI_INVALID_TABLE_ADDR)
		str += sprintf(str, "ACPI=0x%lx\n", efi.acpi);
	/*
	 * If both SMBIOS and SMBIOS3 entry points are implemented, the
	 * SMBIOS3 entry point shall be preferred, so we list it first to
	 * let applications stop parsing after the first match.
	 */
	if (efi.smbios3 != EFI_INVALID_TABLE_ADDR)
		str += sprintf(str, "SMBIOS3=0x%lx\n", efi.smbios3);
	if (efi.smbios != EFI_INVALID_TABLE_ADDR)
		str += sprintf(str, "SMBIOS=0x%lx\n", efi.smbios);
	if (efi.hcdp != EFI_INVALID_TABLE_ADDR)
		str += sprintf(str, "HCDP=0x%lx\n", efi.hcdp);
	if (efi.boot_info != EFI_INVALID_TABLE_ADDR)
		str += sprintf(str, "BOOTINFO=0x%lx\n", efi.boot_info);
	if (efi.uga != EFI_INVALID_TABLE_ADDR)
		str += sprintf(str, "UGA=0x%lx\n", efi.uga);

	return str - buf;
}

static struct kobj_attribute efi_attr_systab = __ATTR_RO_MODE(systab, 0400);

#define EFI_FIELD(var) efi.var

#define EFI_ATTR_SHOW(name) \
static ssize_t name##_show(struct kobject *kobj, \
				struct kobj_attribute *attr, char *buf) \
{ \
	return sprintf(buf, "0x%lx\n", EFI_FIELD(name)); \
}

EFI_ATTR_SHOW(fw_vendor);
EFI_ATTR_SHOW(runtime);
EFI_ATTR_SHOW(config_table);

static ssize_t fw_platform_size_show(struct kobject *kobj,
				     struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", efi_enabled(EFI_64BIT) ? 64 : 32);
}

static struct kobj_attribute efi_attr_fw_vendor = __ATTR_RO(fw_vendor);
static struct kobj_attribute efi_attr_runtime = __ATTR_RO(runtime);
static struct kobj_attribute efi_attr_config_table = __ATTR_RO(config_table);
static struct kobj_attribute efi_attr_fw_platform_size =
	__ATTR_RO(fw_platform_size);

static struct attribute *efi_subsys_attrs[] = {
	&efi_attr_systab.attr,
	&efi_attr_fw_vendor.attr,
	&efi_attr_runtime.attr,
	&efi_attr_config_table.attr,
	&efi_attr_fw_platform_size.attr,
	NULL,
};

static umode_t efi_attr_is_visible(struct kobject *kobj,
				   struct attribute *attr, int n)
{
	if (attr == &efi_attr_fw_vendor.attr) {
		if (efi_enabled(EFI_PARAVIRT) ||
				efi.fw_vendor == EFI_INVALID_TABLE_ADDR)
			return 0;
	} else if (attr == &efi_attr_runtime.attr) {
		if (efi.runtime == EFI_INVALID_TABLE_ADDR)
			return 0;
	} else if (attr == &efi_attr_config_table.attr) {
		if (efi.config_table == EFI_INVALID_TABLE_ADDR)
			return 0;
	}

	return attr->mode;
}

static struct attribute_group efi_subsys_attr_group = {
	.attrs = efi_subsys_attrs,
	.is_visible = efi_attr_is_visible,
};

static struct efivars generic_efivars;
static struct efivar_operations generic_ops;

static int generic_ops_register(void)
{
	generic_ops.get_variable = efi.get_variable;
	generic_ops.set_variable = efi.set_variable;
	generic_ops.set_variable_nonblocking = efi.set_variable_nonblocking;
	generic_ops.get_next_variable = efi.get_next_variable;
	generic_ops.query_variable_store = efi_query_variable_store;

	return efivars_register(&generic_efivars, &generic_ops, efi_kobj);
}

static void generic_ops_unregister(void)
{
	efivars_unregister(&generic_efivars);
}

/*
 * We register the efi subsystem with the firmware subsystem and the
 * efivars subsystem with the efi subsystem, if the system was booted with
 * EFI.
 */
static int __init efisubsys_init(void)
{
	int error;

	if (!efi_enabled(EFI_BOOT))
		return 0;

	/* We register the efi directory at /sys/firmware/efi */
	efi_kobj = kobject_create_and_add("efi", firmware_kobj);
	if (!efi_kobj) {
		pr_err("efi: Firmware registration failed.\n");
		return -ENOMEM;
	}

	error = generic_ops_register();
	if (error)
		goto err_put;

	error = sysfs_create_group(efi_kobj, &efi_subsys_attr_group);
	if (error) {
		pr_err("efi: Sysfs attribute export failed with error %d.\n",
		       error);
		goto err_unregister;
	}

	error = efi_runtime_map_init(efi_k