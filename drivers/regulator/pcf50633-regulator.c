-------------------------------
 */

/**
 * firmware_map_add_entry() - Does the real work to add a firmware memmap entry.
 * @start: Start of the memory range.
 * @end:   End of the memory range (exclusive).
 * @type:  Type of the memory range.
 * @entry: Pre-allocated (either kmalloc() or bootmem allocator), uninitialised
 *         entry.
 *
 * Common implementation of firmware_map_add() and firmware_map_add_early()
 * which expects a pre-allocated struct firmware_map_entry.
 *
 * Return: 0 always
 */
static int firmware_map_add_entry(u64 start, u64 end,
				  const char *type,
				  struct firmware_map_entry *entry)
{
	BUG_ON(start > end);

	entry->start = start;
	entry->end = end - 1;
	entry->type = type;
	INIT_LIST_HEAD(&entry->list);
	kobject_init(&entry->kobj, &memmap_ktype);

	spin_lock(&map_entries_lock);
	list_add_tail(&entry->list, &map_entries);
	spin_unlock(&map_entries_lock);

	return 0;
}

/**
 * firmware_map_remove_entry() - Does the real work to remove a firmware
 * memmap entry.
 * @entry: removed entry.
 *
 * The caller must hold map_entries_lock, and release it properly.
 */
static inline void firmware_map_remove_entry(struct firmware_map_entry *entry)
{
	list_del(&entry->list);
}

/*
 * Add memmap entry on sysfs
 */
static int add_sysfs_fw_map_entry(struct firmware_map_entry *entry)
{
	static int map_entries_nr;
	static struct kset *mmap_kset;

	if (entry->kobj.state_in_sysfs)
		return -EEXIST;

	if (!mmap_kset) {
		mmap_kset = kset_create_and_add("memmap", NULL, firmware_kobj);
		if (!mmap_kset)
			return -ENOMEM;
	}

	entry->kobj.kset = mmap_kset;
	if (kobject_add(&entry->kobj, NULL, "%d", map_entries_nr++))
		kobject_put(&entry->kobj);

	return 0;
}

/*
 * Remove memmap entry on sysfs
 */
static inline void remove_sysfs_fw_map_entry(struct firmware_map_entry *entry)
{
	kobject_put(&entry->kobj);
}

/**
 * firmware_map_find_entry_in_list() - Search memmap entry in a given list.
 * @start: Start of the memory range.
 * @end:   End of the memory range (exclusive).
 * @type:  Type of the memory range.
 * @list:  In which to find the entry.
 *
 * This function is to find the memmap entey of a given memory range in a
 * given list. The caller must hold map_entries_lock, and must not release
 * the lock until the processing of the returned entry has completed.
 *
 * Return: Pointer to the entry to be found on success, or NULL on failure.
 */
static struct firmware_map_entry * __meminit
firmware_map_find_entry_in_list(u64 start, u64 end, const char *type,
				struct list_head *list)
{
	struct firmware_map_entry *entry;

	list_for_each_entry(entry, list, list)
		if ((entry->start == start) && (entry->end == end) &&
		    (!strcmp(entry->type, type))) {
			return entry;
		}

	return NULL;
}

/**
 * firmware_map_find_entry() - Search memmap entry in map_entries.
 * @start: Start of the memory range.
 * @end:   End of the memory range (exclusive).
 * @type:  Type of the memory range.
 *
 * This function is to find the memmap entey of a given memory range.
 * The caller must hold map_entries_lock, and must not release the lock
 * until the processing of the returned entry has completed.
 *
 * Return: Pointer to the entry to be found on success, or NULL on failure.
 */
static struct firmware_map_entry * __meminit
firmware_map_find_entry(u64 start, u64 end, const char *type)
{
	return firmware_map_find_entry_in_list(start, end, type, &map_entries);
}

/**
 * firmware_map_find_entry_bootmem() - Search memmap entry in map_entries_bootmem.
 * @start: Start of the memory range.
 * @end:   End of the memory range (exclusive).
 * @type:  Type of the memory range.
 *
 * This function is similar to firmware_map_find_entry except that it find the
 * given entry in map_entries_bootmem.
 *
 * Return: Pointer to the entry to be found on success, or NULL on failure.
 */
static struct firmware_map_entry * __meminit
firmware_map_find_entry_bootmem(u64 start, u64 end, const char *type)
{
	return firmware_m