*data)
{
	struct edac_device_block *block = to_block(kobj);

	return sprintf(data, "%u\n", block->counters.ue_count);
}

static ssize_t block_ce_count_show(struct kobject *kobj,
					struct attribute *attr, char *data)
{
	struct edac_device_block *block = to_block(kobj);

	return sprintf(data, "%u\n", block->counters.ce_count);
}

/* DEVICE block kobject release() function */
static void edac_device_ctrl_block_release(struct kobject *kobj)
{
	struct edac_device_block *block;

	edac_dbg(1, "\n");

	/* get the container of the kobj */
	block = to_block(kobj);

	/* map from 'block kobj' to 'block->instance->controller->main_kobj'
	 * now 'release' the block kobject
	 */
	kobject_put(&block->instance->ctl->kobj);
}


/* Function to 'show' fields from the edac_dev 'block' structure */
static ssize_t edac_dev_block_show(struct kobject *kobj,
				struct attribute *attr, char *buffer)
{
	struct edac_dev_sysfs_block_attribute *block_attr =
						to_block_attr(attr);

	if (block_attr->show)
		return block_attr->show(kobj, attr, buffer);
	return -EIO;
}

/* Function to 'store' fields into the edac_dev 'block' structure */
static ssize_t edac_dev_block_store(struct kobject *kobj,
				struct attribute *attr,
				const char *buffer, size_t count)
{
	struct edac_dev_sysfs_block_attribute *block_attr;

	block_attr = to_block_attr(attr);

	if (block_attr->store)
		return block_att