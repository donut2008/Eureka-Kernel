m_buf->start, &param, sizeof(param));

	rc = gsmi_exec(GSMI_CALLBACK, GSMI_CMD_SET_NVRAM_VAR);
	if (rc < 0) {
		printk(KERN_ERR "gsmi: Set Variable failed\n");
		ret = EFI_INVALID_PARAMETER;
	}

	spin_unlock_irqrestore(&gsmi_dev.lock, flags);

	return ret;
}

static const struct efivar_operations efivar_ops = {
	.get_variable = gsmi_get_variable,
	.set_variable = gsmi_set_variable,
	.get_next_variable = gsmi_get_next_variable,
};

static ssize_t eventlog_write(struct file *filp, struct kobject *kobj,
			       struct bin_attribute *bin_attr,
			       char *buf, loff_t pos, size_t count)
{
	struct gsmi_set_eventlog_param param = {
		.data_ptr = gsmi_dev.data_buf->address,
	};
	int rc = 0;
	unsigned long flags;

	/* Pull the type out */
	if (count < sizeof(u32))
		return -EINVAL;
	param.type = *(u32 *)buf;
	buf += sizeof(u32);

	/* The remaining buffer is the data payload */
	if ((count - sizeof(u32)) > gsmi_dev.data_buf->length)
		return -EINVAL;
	param.data_len = count - sizeof(u32);

	spin_lock_irqsave(&gsmi_dev.lock, flags);

	/* data pointer */
	memset(gsmi_dev.data_buf->start, 0, gsmi_dev.data_buf->length);
	memcpy(gsmi_dev.data_buf->start, buf, param.data_len);

	/* parameter buffer */
	memset(gsmi_dev.param_buf->start, 0, gsmi_dev.param_buf->length);
	memcpy(gsmi_dev.param_buf->start, &param, sizeof(param));

	rc = gsmi_exec(GSMI_CALLBACK, GSMI_CMD_SET_EVENT_LOG);
	if (rc < 0)
		printk(KERN_ERR "gsmi: Set Event Log failed\n");

	spin_unlock_irqrestore(&gsmi_dev.lock, flags);

	return (rc == 0) ? count : rc;

}

static struct bin_attribute eventlog_bin_attr = {
	.attr = {.name = "append_to_eventlog", .mode = 0200},
	.write = eventlog_write,
};

static ssize_t gsmi_clear_eventlog_store(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 const char *buf, size_t count)
{
	int rc;
	unsigned long flags;
	unsigned long val;
	struct {
		u32 percentage;
		u32 data_type;
	} param;

	rc = kstrtoul(buf, 0, &val);
	if (rc)
		return rc;

	/*
	 * Value entered is a percentage, 0 through 100, anything else
	 * is invalid.
	 */
	if (val > 100)
		return -EINVAL;

	/* data_type here selects the smbios event log. */
	param.percentage = val;
	param.data_type = 0;

	spin_lock_irqsave(&gsmi_dev.lock, flags);

	/* parameter buffer */
	memset(gsmi_dev.param_buf->start, 0, gsmi_dev.param_buf->length);
	memcpy(gsmi_dev.param_buf->start, &param, sizeof(param));

	rc = gsmi_exec(GSMI_CALLBACK, GSMI_CMD_CLEAR_EVENT_LOG);

	spin_unlock_irqrestore(&gsmi_dev.lock, flags);

	if (rc)
		return rc;
	return count;
}

static struct kobj_attribute gsmi_clear_eventlog_attr = {
	.attr = {.name = "clear_eventlog", .mode = 0200},
	.store = gsmi_clear_eventlog_store,
};

static ssize_t gsmi_clear_config_store(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       const char *buf, size_t count)
{
	int rc;
	unsigned long flags;

	spin_lock_irqsave(&gsmi_dev.lock, flags);

	/* clear parameter buffer */
	memset(gsmi_dev.param_buf->start, 0, gsmi_dev.param_buf->length);

	rc = gsmi_exec(GSMI_CALLBACK, GSMI_CMD_CLEAR_CONFIG);

	spin_unlock_irqrestore(&gsmi_dev.lock, flags);

	if (rc)
		return rc;
	return count;
}

static struct kobj_attribute gsmi_clear_config_attr = {
	.attr = {.name = "clear_config", .mode = 0200},
	.store = gsmi_clear_config_store,
};

static const struct attribute *gsmi_attrs[] = {
	&gsmi_clear_config_attr.attr,
	&gsmi_clear_eventlog_attr.attr,
	NULL,
};

static int gsmi_shutdown_reason(int reason)
{
	struct gsmi_log_entry_type_1 entry = {
		.type     = GSMI_LOG_ENTRY_TYPE_KERNEL,
		.instance = reason,
	};
	struct gsmi_set_eventlog_param param = {
		.data_len = sizeof(entry),
		.type     = 1,
	};
	static int saved_reason;
	int rc = 0;
	unsigned long flags;

	/* avoid duplicate entries in the log */
	if (saved_reason & (1 << reason))
		return 0;

	spin_lock_irqsave(&gsmi_dev.lock, flags);

	saved_reason |= (1 << reason);

	/* data pointer */
	memset(gsmi_dev.data_buf->start, 0, gsmi_dev.data_buf->length);
	memcpy(gsmi_dev.data_buf->start, &entry, sizeof(entry));

	/* parameter buffer */
	param.data_ptr = gsmi_dev.data_buf->address;
	memset(gsmi_dev.param_buf->start, 0, gsmi_dev.param_buf->length);
	memcpy(gsmi_dev.param_buf->start, &param, sizeof(param));

	rc = gsmi_exec(GSMI_CALLBACK, GSMI_CMD_SET_EVENT_LOG);

	spin_unlock_irqrestore(&gsmi_dev.lock, flags);

	if (rc < 0)
		printk(KERN_ERR "gsmi: Log Shutdown Reason failed\n");
	else
		printk(KERN_EMERG "gsmi: Log Shutdown Reason 0x%02x\n",
		       reason);

	return rc;
}

static int gsmi_reboot_callback(struct notifier_block *nb,
				unsigned long reason, void *arg)
{
	gsmi_shutdown_reason(GSMI_SHUTDOWN_CLEAN);
	return NOTIFY_DONE;
}

static struct notifier_block gsmi_reboot_notifier = {
	.notifier_call = gsmi_reboot_callback
};

static int gsmi_die_callback(struct notifier_block *nb,
			     unsigned long reason, void *arg)
{
	if (reason == DIE_OOPS)
		gsmi_shutdown_reason(GSMI_SHUTDOWN_OOPS);
	return NOTIFY_DONE;
}

static struct notifier_block gsmi_die_notifier = {
	.notifier_call = gsmi_die_callback
};

static int gsmi_panic_callback(struct notifier_block *nb,
			       unsigned long reason, void *arg)
{

	/*
	 * Panic callbacks are executed with all other CPUs stopped,
	 * so we must not attempt to spin waiting for gsmi_dev.lock
	 * to be released.
	 */
	if (spin_is_locked(&gsmi_dev.lock))
		return NOTIFY_DONE;

	gsmi_shutdown_reason(GSMI_SHUTDOWN_PANIC);
	return NOTIFY_DONE;
}

static struct notifier_block gsmi_panic_notifier = {
	.notifier_call = gsmi_panic_callback,
};

/*
 * This hash function was blatantly copied from include/linux/hash.h.
 * It is used by this driver to obfuscate a board name that requires a
 * quirk within this driver.
 *
 * Please do not remove this copy of the function as any changes to the
 * global utility hash_64() function would break this driver's ability
 * to identify a board and provide the appropriate quirk -- mikew@google.com
 */
static u64 __init local_hash_64(u64 val, unsigned bits)
{
	u64 hash = val;

	/*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
	u64 n = hash;
	n <<= 18;
	hash -= n;
	n <<= 33;
	hash -= n;
	n <<= 3;
	hash += n;
	n <<= 3;
	hash -= n;
	n <<= 4;
	hash += n;
	n <<= 2;
	hash += n;

	/* High bits are more random, so use them. */
	return hash >> (64 - bits);
}

static u32 __init hash_oem_table_id(char s[8])
{
	u64 input;
	memcpy(&input, s, 8);
	return local_hash_64(input, 32);
}

static struct dmi_system_id gsmi_dmi_table[] __initdata = {
	{
		.ident = "Google Board",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Google, Inc."),
		},
	},
	{}
};
MODULE_DEVICE_TABLE(dmi, gsmi_dmi_table);

static __init int gsmi_system_valid(void)
{
	u32 hash;

	if (!dmi_check_system(gsmi_dmi_table))
		return -ENODEV;

	/*
	 * Only newer firmware supports the gsmi interface.  All older
	 * firmware that didn't support this interface used to plug the
	 * table name in the first four bytes of the oem_table_id field.
	 * Newer firmware doesn't do that though, so use that as the
	 * discriminant factor.  We have to do this in order to
	 * whitewash our board names out of the public driver.
	 */
	if (!strncmp(acpi_gbl_FADT.header.oem_table_id, "FACP", 4)) {
		printk(KERN_INFO "gsmi: Board is too old\n");
		return -ENODEV;
	}

	/* Disable on board with 1.0 BIOS due to Google bug 2602657 */
	hash = hash_oem_table_id(acpi_gbl_FADT.header.oem_table_id);
	if (hash == QUIRKY_BOARD_HASH) {
		const char *bios_ver = dmi_get_system_info(DMI_BIOS_VERSION);
		if (strncmp(bios_ver, "1.0", 3) == 0) {
			pr_info("gsmi: disabled on this board's BIOS %s\n",
				bios_ver);
			return -ENODEV;
		}
	}

	/* check for valid SMI command port in ACPI FADT */
	if (acpi_gbl_FADT.smi_command == 0) {
		pr_info("gsmi: missing smi_command\n");
		return -ENODEV;
	}

	/* Found */
	return 0;
}

static struct kobject *gsmi_kobj;
static struct efivars efivars;

static const struct platform_device_info gsmi_dev_info = {
	.name		= "gsmi",
	.id		= -1,
	/* SMI callbacks require 32bit addresses */
	.dma_mask	= DMA_BIT_MASK(32),
};

static __init int gsmi_init(void)
{
	unsigned long flags;
	int ret;

	ret