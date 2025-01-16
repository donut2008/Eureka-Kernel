turn true;
	}

	return false;
}

struct variable_validate {
	efi_guid_t vendor;
	char *name;
	bool (*validate)(efi_char16_t *var_name, int match, u8 *data,
			 unsigned long len);
};

/*
 * This is the list of variables we need to validate, as well as the
 * whitelist for what we think is safe not to default to immutable.
 *
 * If it has a validate() method that's not NULL, it'll go into the
 * validation routine.  If not, it is assumed valid, but still used for
 * whitelisting.
 *
 * Note that it's sorted by {vendor,name}, but globbed names must come after
 * any other name with the same prefix.
 */
static const struct variable_validate variable_validate[] = {
	{ EFI_GLOBAL_VARIABLE_GUID, "BootNext", validate_uint16 },
	{ EFI_GLOBAL_VARIABLE_GUID, "BootOrder", validate_boot_order },
	{ EFI_GLOBAL_VARIABLE_GUID, "Boot*", validate_load_option },
	{ EFI_GLOBAL_VARIABLE_GUID, "DriverOrder", validate_boot_order },
	{ EFI_GLOBAL_VARIABLE_GUID, "Driver*", validate_load_option },
	{ EFI_GLOBAL_VARIABLE_GUID, "ConIn", validate_device_path },
	{ EFI_GLOBAL_VARIABLE_GUID, "ConInDev", validate_device_path },
	{ EFI_GLOBAL_VARIABLE_GUID, "ConOut", validate_device_path },
	{ EFI_GLOBAL_VARIABLE_GUID, "ConOutDev", validate_device_path },
	{ EFI_GLOBAL_VARIABLE_GUID, "ErrOut", validate_device_path },
	{ EFI_GLOBAL_VARIABLE_GUID, "ErrOutDev", validate_device_path },
	{ EFI_GLOBAL_VARIABLE_GUID, "Lang", validate_ascii_string },
	{ EFI_GLOBAL_VARIABLE_GUID, "OsIndications", NULL },
	{ EFI_GLOBAL_VARIABLE_GUID, "PlatformLang", validate_ascii_string },
	{ EFI_GLOBAL_VARIABLE_GUID, "Timeout", validate_uint16 },
	{ LINUX_EFI_CRASH_GUID, "*", NULL },
	{ NULL_GUID, "", NULL },
};

/*
 * Check if @var_name matches the pattern given in @match_name.
 *
 * @var_name: an array of @len non-NUL characters.
 * @match_name: a NUL-terminated pattern string, optionally ending in "*". A
 *              final "*" character matches any trailing characters @var_name,
 *              including the case when there are none left in @var_name.
 * @match: on output, the number of non-wildcard characters in @match_name
 *         that @var_name matches, regardless of the return value.
 * @return: whether @var_name fully matches @match_name.
 */
static bool
variable_matches(const char *var_name, size_t len, const char *match_name,
		 int *match)
{
	for (*match = 0; ; (*match)++) {
		char c = match_name[*match];

		switch (c) {
		case '*':
			/* Wildcard in @match_name means we've matched. */
			return true;

		case '\0':
			/* @match_name has ended. Has @var_name too? */
			return (*match == len);

		default:
			/*
			 * We've reached a non-wildcard char in @match_name.
			 * Continue only if there's an identical character in
			 * @var_name.
			 */
			if (*match < len && c == var_name[*match])
				continue;
			return false;
		}
	}
}

bool
efivar_validate(efi_guid_t vendor, efi_char16_t *var_name, u8 *data,
		unsigned long data_size)
{
	int i;
	unsigned long utf8_size;
	u8 *utf8_name;

	utf8_size = ucs2_utf8size(var_name);
	utf8_name = kmalloc(utf8_size + 1, GFP_KERNEL);
	if (!utf8_name)
		return false;

	ucs2_as_utf8(utf8_name, var_name, utf8_size);
	utf8_name[utf8_size] = '\0';

	for (i = 0; variable_validate[i].name[0] != '\0'; i++) {
		const char *name = variable_validate[i].name;
		int match = 0;

		if (efi_guidcmp(vendor, variable_validate[i].vendor))
			continue;

		if (variable_matches(utf8_name, utf8_size+1, name, &match)) {
			if (variable_validate[i].validate == NULL)
				break;
			kfree(utf8_name);
			return variable_validate[i].validate(var_name, match,
							     data, data_size);
		}
	}
	kfree(utf8_name);
	return true;
}
EXPORT_SYMBOL_GPL(efivar_validate);

bool
efivar_variable_is_removable(efi_guid_t vendor, const char *var_name,
			     size_t len)
{
	int i;
	bool found = false;
	int match = 0;

	/*
	 * Check if our variable is in the validated variables list
	 */
	for (i = 0; variable_validate[i].name[0] != '\0'; i++) {
		if (efi_guidcmp(variable_validate[i].vendor, vendor))
			continue;

		if (variable_matches(var_name, len,
				     variable_validate[i].name, &match)) {
			found = true;
			break;
		}
	}

	/*
	 * If it's in our list, it is removable.
	 */
	return found;
}
EXPORT_SYMBOL_GPL(efivar_variable_is_removable);

static efi_status_t
check_var_size(u32 attributes, unsigned long size)
{
	const struct efivar_operations *fops = __efivars->ops;

	if (!fops->query_variable_store)
		return EFI_UNSUPPORTED;

	return fops->query_variable_store(attributes, size);
}

static int efi_status_to_err(efi_status_t status)
{
	int err;

	switch (status) {
	case EFI_SUCCESS:
		err = 0;
		break;
	case EFI_INVALID_PARAMETER:
		err = -EINVAL;
		break;
	case EFI_OUT_OF_RESOURCES:
		err = -ENOSPC;
		break;
	case EFI_DEVICE_ERROR:
		err = -EIO;
		break;
	case EFI_WRITE_PROTECTED:
		err = -EROFS;
		break;
	case EFI_SECURITY_VIOLATION:
		err = -EACCES;
		break;
	case EFI_NOT_FOUND:
		err = -ENOENT;
		break;
	default:
		err = -EINVAL;
	}

	return err;
}

static bool variable_is_present(efi_char16_t *variable_name, efi_guid_t *vendor,
				struct list_head *head)
{
	struct efivar_entry *entry, *n;
	unsigned long strsize1, strsize2;
	bool found = false;

	strsize1 = ucs2_strsize(variable_name, 1024);
	list_for_each_entry_safe(entry, n, head, list) {
		strsiz