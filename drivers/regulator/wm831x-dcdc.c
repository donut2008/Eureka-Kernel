

	do {
		variable_name_size = 1024;

		status = ops->get_next_variable(&variable_name_size,
						variable_name,
						&vendor_guid);
		switch (status) {
		case EFI_SUCCESS:
			if (!atomic)
				spin_unlock_irq(&__efivars->lock);

			variable_name_size = var_name_strnsize(variable_name,
							       variable_name_size);

			/*
			 * Some firmware implementations return the
			 * same variable name on multiple calls to
			 * get_next_variable(). Terminate the loop
			 * immediately as there is no guarantee that
			 * we'll ever see a different variable name,
			 * and may end up looping here forever.
			 */
			if (duplicates &&
			    variable_is_present(variable_name, &vendor_guid, head)) {
				dup_variable_bug(variable_name, &vendor_guid,
						 variable_name_size);
				if (!atomic)
					spin_lock_irq(&__efivars->lock);

				status = EFI_NOT_FOUND;
				break;
			}

			err = func(variable_name, vendor_guid, variable_name_size, data);
			if (err)
				status = EFI_NOT_FOUND;

			if (!atomic)
				spin_lock_irq(&__efivars->lock);

			break;
		case EFI_NOT_FOUND:
			break;
		default:
			printk(KERN_WARNING "efivars: get_next_variable: status=%lx\n",
				status);
			status = EFI_NOT_FOUND;
			break;
		}

	} while (status != EFI_NOT_FOUND);

	spin_unlock_irq(&__efivars->lock);

	kfree(variable_name);

	return err;
}
EXPORT_SYMBOL_GPL(efivar_init);

/**
 * efivar_entry_add - add entry to variable list
 * @entry: entry to add to list
 * @head: list head
 */
void efivar_entry_add(struct efivar_entry *entry, struct list_head *head)
{
	spin_lock_irq(&__efivars->lock);
	list_add(&entry->list, head);
	spin_unlock_irq(&__efivars->lock);
}
EXPORT_SYMBOL_GPL(efivar_entry_add);

/**
 * efivar_entry_remove - remove entry from variable list
 * @entry: entry to remove from list
 */
void efivar_entry_remove(struct efivar_entry *entry)
{
	spin_lock_irq(&__efivars->lock);
	list_del(&entry->list);
	spin_unlock_irq(&__efivars->lock);
}
EXPORT_SYMBOL_GPL(efivar_entry_remove);

/*
 * efivar_entry_list_del_unlock - remove entry from variable list
 * @entry: entry to remove
 *
 * Remove @entry from the variable list and release the list lock.
 *
 * NOTE: slightly weird locking semantics here - we expect to be
 * called with the efivars lock already held, and we release it before
 * returning. This is because this function is usually called after
 * set_variable() while the lock is still held.
 */
static void efivar_entry_list_del_unlock(struct efivar_entry *entry)
{
	lockdep_assert_held(&__efivars->lock);

	list_del(&entry->list);
	spin_unlock_irq(&__efivars->lock);
}

/**
 * __efivar_entry_delete - delete an EFI variable
 * @entry: entry containing EFI variable to delete
 *
 * Delete the variable from the firmware but leave @entry on the
 * variable list.
 *
 * This function differs from efivar_entry_delete() because it does
 * not remove @entry from the variable list. Also, it is safe to be
 * called from within a efivar_entry_iter_begin() and
 * efivar_entry_iter_end() region, unlike efivar_entry_delete().
 *
 * Returns 0 on success, or a converted EFI status code if
 * set_variable() fails.
 */
int __efivar_entry_delete(struct efivar_entry *entry)
{
	const struct efivar_operations *ops = __efivars->ops;
	efi_status_t status;

	lockdep_assert_held(&__efivars->lock);

	status = ops->set_variable(entry->var.VariableName,
				   &entry->var.VendorGuid,
				   0, 0, NULL);

	return efi_status_to_err(status);
}
EXPORT_SYMBOL_GPL(__efivar_entry_delete);

/**
 * efivar_entry_delete - delete variable and remove entry from list
 * @entry: entry containing variable to delete
 *
 * Delete the variable from the firmware and remove @entry from the
 * variable list. It is the caller's responsibility to free @entry
 * once we return.
 *
 * Returns 0 on success, or a converted EFI status code if
 * set_variable() fails.
 */
int efivar_entry_delete(struct efivar_entry *entry)
{
	const struct efivar_operations *ops = __efivars->ops;
	efi_status_t status;

	spin_lock_irq(&__efivars->lock);
	status = ops->set_variable(entry->var.VariableName,
				   &entry->var.VendorGuid,
				   0, 0, NULL);
	if (!(status == EFI_SUCCESS || status == EFI_NOT_FOUND)) {
		spin_unlock_irq(&__efivars->lock);
		return efi_status_to_err(status);
	}

	efivar_entry_list_del_unlock(entry);
	return 0;
}
EXPORT_SYMBOL_GPL(efivar_entry_delete);

/**
 * efivar_entry_set - call set_variable()
 * @entry: entry containing the EFI variable to write
 * @attributes: variable attributes
 * @size: size of @data buffer
 * @data: buffer containing variable data
 * @head: head of variable list
 *
 * Calls set_variable() for an EFI variable. If creating a new EFI
 * variable, this function is usually followed by efivar_entry_add().
 *
 * Before writing the variable, the remaining EFI variable storage
 * space is checked to ensure there is enough room available.
 *
 * If @head is not NULL a lookup is performed to determine whether
 * the entry is already on the list.
 *
 * Returns 0 on success, -EEXIST if a lookup is performed and the entry
 * already exists on the list, or a converted EFI status code if
 * set_variable() fails.
 */
int efivar_entry_set(struct efivar_entry *entry, u32 attributes,
		     unsigned long size, void *data, struct list_head *head)
{
	const struct efivar_operations *ops = __efivars->ops;
	efi_status_t status;
	efi_char16_t *name = entry->var.VariableName;
	efi_guid_t vendor = entry->var.VendorGuid;

	spin_lock_irq(&__efivars->lock);

	if (head && efivar_entry_find(name, vendor, head, false)) {
		spin_unlock_irq(&__efivars->lock);
		return -EEXIST;
	}

	status = check_var_size(attributes, size + ucs2_strsize(name, 1024));
	if (status == EFI_SUCCESS || status == EFI_UNSUPPORTED)
		status = ops->set_variable(name, &vendor,
					   attributes, size, data);

	spin_unlock_irq(&__efivars->lock);

	return efi_status_to_err(status);

}
EXPORT_SYMBOL_GPL(efivar_entry_set);

/*
 * efivar_entry_set_nonblocking - call set_variable_nonblocking()
 *
 * This function is guaranteed to not block and is suitable for calling
 * from crash/panic handlers.
 *
 * Crucially, this function will not block if it cannot acquire
 * __efivars->lock. Instead, it returns -EBUSY.
 */
static int
efivar_entry_set_nonblocking(efi_char16_t *name, efi_guid_t vendor,
			     u32 attributes, unsigned long size, void *data)
{
	const struct efivar_operations *ops = __efivars->ops;
	unsigned long flags;
	efi_status_t status;

	if (!spin_trylock_irqsave(&__efivars->lock, flags))
		return -EBUSY;

	status = check_var_size(attributes, size + ucs2_strsize(name, 1024));
	if (status != EFI_SUCCESS) {
		spin_unlock_irqrestore(&__efivars->lock, flags);
		return -ENOSPC;
	}

	status = ops->set_variable_nonblocking(name, &vendor, attributes,
					       size, data);

	spin_unlock_irqrestore(&__efivars->lock, flags);
	return efi_status_to_err(status);
}

/**
 * efivar_entry_set_safe - call set_variable() if enough space in firmware
 * @name: buffer containing the variable name
 * @vendor: variable vendor guid
 * @attributes: variable attributes
 * @block: can we block in this context?
 * @size: size of @data buffer
 * @data: buffer containing variable data
 *
 * Ensures there is enough free storage in the firmware for this variable, and
 * if so, calls set_variable(). If creating a new EFI variable, this function
 * is usually followed by efivar_entry_add().
 *
 * Returns 0 on success, -ENOSPC if the firmware does not have enough
 * space for set_variable() to succeed, or a converted EFI status code
 * if set_variable() fails.
 */
int efivar_entry_set_safe(efi_char16_t *name, efi_guid_t vendor, u32 attributes,
			  bool block, unsigned long size, void *data)
{
	const struct efivar_operations *ops = __efivars->ops;
	unsigned long flags;
	efi_status_t status;

	if (!ops->query_variable_store)
		return -ENOSYS;

	/*
	 * If the EFI variable backend provides a non-blocking
	 * ->set_variable() operation and we're in a context where we
	 * cannot block, then we need to use it to avoid live-locks,
	 * since the implication is that the regular ->set_variable()
	 * will block.
	 *
	 * If no ->set_variable_nonblocking() is provided then
	 * ->set_variable() is assumed to be non-blocking.
	 */
	if (!block && ops->set_variable_nonblocking)
		return efivar_entry_set_nonblocking(name, vendor, attributes,
						    size, data);

	if (!block) {
		if (!spin_trylock_irqsave(&__efivars->lock, flags))
			return -EBUSY;
	} else {
		spin_lock_irqsave(&__efivars->lock, flags);
	}

	status = check_var_size(attributes, size + ucs2_strsize(name, 1024));
	if (status != EFI_SUCCESS) {
		spin_unlock_irqrestore(&__efivars->lock, flags);
		return -ENOSPC;
	}

	status = ops->set_variable(name, &vendor, attributes, size, data);

	spin_unlock_irqrestore(&__efivars->lock, flags);

	return efi_status_to_err(status);
}
EXPORT_SYMBOL_GPL(efivar_entry_set_safe);

/**
 * efivar_entry_find - search for an entry
 * @name: the EFI variable name
 * @guid: the EFI variable vendor's guid
 * @head: head of the variable list
 * @remove: should we remove the entry from the list?
 *
 * Search for an entry on the variable list that has the EFI variable
 * name @name and vendor guid @guid. If an entry is found on the list
 * and @remove is true, the entry is removed from the list.
 *
 * The caller MUST call efivar_entry_iter_begin() and
 * efivar_entry_iter_end() before and after the invocation of this
 * function, respectively.
 *
 * Returns the entry if found on the list, %NULL otherwise.
 */
struct efivar_entry *efivar_entry_find(efi_char16_t *name, efi_guid_t guid,
				       struct list_head *head, bool remove)
{
	struct efivar_entry *entry, *n;
	int strsize1, strsize2;
	bool found = false;

	lockdep_assert_held(&__efivars->lock);

	list_for_each_entry_safe(entry, n, head, list) {
		strsize1 = ucs2_strsize(name, 1024);
		strsize2 = ucs2_strsize(entry->var.VariableName, 1024);
		if (strsize1 == strsize2 &&
		    !memcmp(name, &(entry->var.VariableName), strsize1) &&
		    !efi_guidcmp(guid, entry->var.VendorGuid)) {
			found = true;
			break;
		}
	}

	if (!found)
		return NULL;

	if (remove) {
		if (entry->scanning) {
			/*
			 * The entry will be deleted
			 * after scanning is completed.
			 */
			entry->deleting = true;
		} else
			list_del(&entry->list);
	}

	return entry;
}
EXPORT_SYMBOL_GPL(efivar_entry_find);

/**
 * efivar_entry_size - obtain the size of a variable
 * @entry: entry for this variable
 * @size: location to store the variable's size
 */
int efivar_entry_size(struct efivar_entry *entry, unsigned long *size)
{
	const struct efivar_operations *ops = __efivars->ops;
	efi_status_t status;

	*size = 0;

	spin_lock_irq(&__efivars->lock);
	status = ops->get_variable(entry->var.VariableName,
				   &entry->var.VendorGuid, NULL, size, NULL);
	spin_unlock_irq(&__efivars->lock);

	if (status != EFI_BUFFER_TOO_SMALL)
		return efi_status_to_err(status);

	return 0;
}
EXPORT_SYMBOL_GPL(efivar_entry_size);

/**
 * __efivar_entry_get - call get_variable()
 * @entry: read data for this variable
 * @attributes: variable attributes
 * @size: size of @data buffer
 * @data: buffer to store variable data
 *
 * The caller MUST call efivar_entry_iter_begin() and
 * efivar_entry_iter_end() before and after the invocation of this
 * function, respectively.
 */
int __efivar_entry_get(struct efivar_entry *entry, u32 *attributes,
		       unsigned long *size, void *data)
{
	const struct efivar_operations *ops = __efivars->ops;
	efi_status_t status;

	lockdep_assert_held(&__efivars->lock);

	status = ops->get_variable(entry->var.VariableName,
				   &entry->var.VendorGuid,
				   attributes, size, data);

	return efi_status_to_err(status);
}
EXPORT_SYMBOL_GPL(__efivar_entry_get);

/**
 * efivar_entry_get - call get_variable()
 * @entry: read data for this variable
 * @attributes: variable attributes
 * @size: size of @data buffer
 * @data: buffer to store variable data
 */
int efivar_entry_get(struct efivar_entry *entry, u32 *attributes,
		     unsigned long *size, void *data)
{
	const struct efivar_operations *ops = __efivars->ops;
	efi_status_t status;

	spin_lock_irq(&__efivars->lock);
	status = ops->get_variable(entry->var.VariableName,
				   &entry->var.VendorGuid,
				   attributes, size, data);
	spin_unlock_irq(&__efivars->lock);

	return efi_status_to_err(status);
}
EXPORT_SYMBOL_GPL(efivar_entry_get);

/**
 * efivar_entry_set_get_size - call set_variable() and get new size (atomic)
 * @entry: entry containing variable to set and get
 * @attributes: attributes of variable to be written
 * @size: size of data buffer
 * @data: buffer containing data to write
 * @set: did the set_variable() call succeed?
 *
 * This is a pretty special (complex) function. See efivarfs_file_write().
 *
 * Atomically call set_variable() for @entry and if the call is
 * successful, return the new size of the variable from get_variable()
 * in @size. The success of set_variable() is indicated by @set.
 *
 * Returns 0 on success, -EINVAL if the variable data is invalid,
 * -ENOSPC if the firmware does not have enough available space, or a
 * converted EFI status code if either of set_variable() or
 * get_variable() fail.
 *
 * If the EFI variable does not exist when calling set_variable()
 * (EFI_NOT_FOUND), @entry is removed from the variable list.
 */
int efivar_entry_set_get_size(struct efivar_entry *entry, u32 attributes,
			      unsigned long *size, void *data, bool *set)
{
	const struct efivar_operations *ops = __efivars->ops;
	efi_char16_t *name = entry->var.VariableName;
	efi_guid_t *vendor = &entry->var.VendorGuid;
	efi_status_t status;
	int err;

	*set = false;

	if (efivar_validate(*vendor, name, data, *size) == false)
		return -EINVAL;

	/*
	 * The lock here protects the get_variable call, the conditional
	 * set_variable call, and removal of the variable from the efivars
	 * list (in the case of an authenticated delete).
	 */
	spin_lock_irq(&__efivars->lock);

	/*
	 * Ensure that the available space hasn't shrunk below the safe level
	 */
	status = check_var_size(attributes, *size + ucs2_strsize(name, 1024));
	if (status != EFI_SUCCESS) {
		if (status != EFI_UNSUPPORTED) {
			err = efi_status_to_err(status);
			goto out;
		}

		if (*size > 65536) {
			err = -ENOSPC;
			goto out;
		}
	}

	status = ops->set_variable(name, vendor, attributes, *size, data);
	if (status != EFI_SUCCESS) {
		err = efi_status_to_err(status);
		goto out;
	}

	*set = true;

	/*
	 * Writing to the variable may have caused a change in size (which
	 * could either be an append or an overwrite), or the variable to be
	 * deleted. Perform a GetVariable() so we can tell what actually
	 * happened.
	 */
	*size = 0;
	status = ops->get_variable(entry->var.VariableName,
				   &entry->var.VendorGuid,
				   NULL, size, NULL);

	if (status == EFI_NOT_FOUND)
		efivar_entry_list_del_unlock(entry);
	else
		spin_unlock_irq(&__efivars->lock);

	if (status && status != EFI_BUFFER_TOO_SMALL)
		return efi_status_to_err(status);

	return 0;

out:
	spin_unlock_irq(&__efivars->lock);
	return err;

}
EXPORT_SYMBOL_GPL(efivar_entry_set_get_size);

/**
 * efivar_entry_iter_begin - begin iterating the variable list
 *
 * Lock the variable list to prevent entry insertion and removal until
 * efivar_entry_iter_end() is called. This function is usually used in
 * conjunction with __efivar_entry_iter() or efivar_entry_iter().
 */
void efivar_entry_iter_begin(void)
{
	spin_lock_irq(&__efivars->lock);
}
EXPORT_SYMBOL_GPL(efivar_entry_iter_begin);

/**
 * efivar_entry_iter_end - finish iterating the variable list
 *
 * Unlock the variable list and allow modifications to the list again.
 */
void efivar_entry_iter_end(void)
{
	spin_unlock_irq(&__efivars->lock);
}
EXPORT_SYMBOL_GPL(efivar_entry_iter_end);

/**
 * __efivar_entry_iter - iterate over variable list
 * @func: callback function
 * @head: head of the variable list
 * @data: function-specific data to pass to callback
 * @prev: entry to begin iterating from
 *
 * Iterate over the list of EFI variables and call @func with every
 * entry on the list. It is safe for @func to remove entries in the
 * list via efivar_entry_delete().
 *
 * You MUST call efivar_enter_iter_begin() before this function, and
 * efivar_entry_iter_end() afterwards.
 *
 * It is possible to begin iteration from an arbitrary entry within
 * the list by passing @prev. @prev is updated on return to point to
 * the last entry passed to @func. To begin iterating from the
 * beginning of the list @prev must be %NULL.
 *
 * The restrictions for @func are the same as documented for
 * efivar_entry_iter().
 */
int __efivar_entry_iter(int (*func)(struct efivar_entry *, void *),
			struct list_head *head, void *data,
			struct efivar_entry **prev)
{
	struct efivar_entry *entry, *n;
	int err = 0;

	if (!prev || !*prev) {
		list_for_each_entry_safe(entry, n, head, list) {
			err = func(entry, data);
			if (err)
				break;
		}

		if (prev)
			*prev = entry;

		return err;
	}


	list_for_each_entry_safe_continue((*prev), n, head, list) {
		err = func(*prev, data);
		if (err)
			break;
	}

	return err;
}
EXPORT_SYMBOL_GPL(__efivar_entry_iter);

/**
 * efivar_entry_iter - iterate over variable list
 * @func: callback function
 * @head: head of variable list
 * @data: function-specific data to pass to callback
 *
 * Iterate over the list of EFI variables and call @func with every
 * entry on the list. It is safe for @func to remove entries in the
 * list via efivar_entry_delete() while iterating.
 *
 * Some notes for the callback function:
 *  - a non-zero return value indicates an error and terminates the loop
 *  - @func is called from atomic context
 */
int efivar_entry_iter(int (*func)(struct efivar_entry *, void *),
		      struct list_head *head, void *data)
{
	int err = 0;

	efivar_entry_iter_begin();
	err = __efivar_entry_iter(func, head, data, NULL);
	efivar_entry_iter_end();

	return err;
}
EXPORT_SYMBOL_GPL(efivar_entry_iter);

/**
 * efivars_kobject - get the kobject for the registered efivars
 *
 * If efivars_register() has not been called we return NULL,
 * otherwise return the kobject used at registration time.
 */
struct kobject *efivars_kobject(void)
{
	if (!__efivars)
		return NULL;

	return __efivars->kobject;
}
EXPORT_SYMBOL_GPL(efivars_kobject);

/**
 * efivar_run_worker - schedule the efivar worker thread
 */
void efivar_run_worker(void)
{
	if (efivar_wq_enabled)
		schedule_work(&efivar_work);
}
EXPORT_SYMBOL_GPL(efivar_run_worker);

/**
 * efivars_register - register an efivars
 * @efivars: efivars to register
 * @ops: efivars operations
 * @kobject: @efivars-specific kobject
 *
 * Only a single efivars can be registered at any time.
 */
int efivars_register(struct efivars *efivars,
		     const struct efivar_operations *ops,
		     struct kobject *kobject)
{
	spin_lock_init(&efivars->lock);
	efivars->ops = ops;
	efivars->kobject = kobject;

	__efivars = efivars;

	return 0;
}
EXPORT_SYMBOL_GPL(efivars_register);

/**
 * efivars_unregister - unregister an efivars
 * @efivars: efivars to unregister
 *
 * The caller must have already removed every entry from the list,
 * failure to do so is an error.
 */
int efivars_unregister(struct efivars *efivars)
{
	int rv;

	if (!__efivars) {
		printk(KERN_ERR "efivars not registered\n");
		rv = -EINVAL;
		goto out;
	}

	if (__efivars != efivars) {
		rv = -EINVAL;
		goto out;
	}

	__efivars = NULL;

	rv = 0;
out:
	return rv;
}
EXPORT_SYMBOL_GPL(efivars_unregister);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          config GOOGLE_FIRMWARE
	bool "Google Firmware Drivers"
	depends on X86
	default n
	help
	  These firmware drivers are used by Google's servers.  They are
	  only useful if you are working directly on one of their
	  proprietary servers.  If in doubt, say "N".

menu "Google Firmware Drivers"
	depends on GOOGLE_FIRMWARE

config GOOGLE_SMI
	tristate "SMI interface for Google platforms"
	depends on ACPI && DMI && EFI
	select EFI_VARS
	help
	  Say Y here if you want to enable SMI callbacks for Google
	  platforms.  This provides an interface for writing to and
	  clearing the EFI event log and reading and writing NVRAM
	  variables.

config GOOGLE_MEMCONSOLE
	tristate "Firmware Memory Console"
	depends on DMI
	help
	  This option enables the kernel to search for a firmware log in
	  the EBDA on Google servers.  If found, this log is exported to
	  userland in the file /sys/firmware/log.

endmenu
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          