river_ops[] = {
	{ .read = driver_stats_read, .llseek = generic_file_llseek, },
	{ .read = driver_names_read, .llseek = generic_file_llseek, },
};

/* read the per-device counters */
static ssize_t dev_counters_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct qib_devdata *dd = private2dd(file);

	avail = dd->f_read_cntrs(dd, *ppos, NULL, &counters);
	return simple_read_from_buffer(buf, count, ppos, counters, avail);
}

/* read the per-device counters */
static ssize_t dev_names_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	char *names;
	size_t avail;
	struct qib_devdata *dd = private2dd(file);

	avail = dd->f_read_cntrs(dd, *ppos, &names, NULL);
	return simple_read_from_buffer(buf, count, ppos, names, avail);
}

static const struct file_operations cntr_ops[] = {
	{ .read = dev_counters_read, .llseek = generic_file_llseek, },
	{ .read = dev_names_read, .llseek = generic_file_llseek, },
};

/*
 * Could use file_inode(file)->i_ino to figure out which file,
 * instead of separate routine for each, but for now, this works...
 */

/* read the per-port names (same for each port) */
static ssize_t portnames_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	char *names;
	size_t avail;
	struct qib_devdata *dd = private2dd(file);

	avail = dd->f_read_portcntrs(dd, *ppos, 0, &names, NULL);
	return simple_read_from_buffer(buf, count, ppos, names, avail);
}

/* read the per-port counters for port 1 (pidx 0) */
static ssize_t portcntrs_1_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct qib_devdata *dd = private2dd(file);

	avail = dd->f_read_portcntrs(dd, *ppos, 0, NULL, &counters);
	return simple_read_from_buffer(buf, count, ppos, counters, avail);
}

/* read the per-port counters for port 2 (pidx 1) */
static ssize_t portcntrs_2_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct qib_devdata *dd = private2dd(file);

	avail = dd->f_read_portcntrs(dd, *ppos, 1, NULL, &counters);
	return simple_read_from_buffer(buf, count, ppos, counters, avail);
}

static const struct file_operations portcntr_ops[] = {
	{ .read = portnames_read, .llseek = generic_file_llseek, },
	{ .read = portcntrs_1_read, .llseek = generic_file_llseek, },
	{ .read = portcntrs_2_read, .llseek = generic_file_llseek, },
};

/*
 * read the per-port QSFP data for port 1 (pidx 0)
 */
static ssize_t qsfp_1_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct qib_devdata *dd = private2dd(file);
	char *tmp;
	int ret;

	tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = qib_qsfp_dump(dd->pport, tmp, PAGE_SIZE);
	if (ret > 0)
		ret = simple_read_from_buffer(buf, count, ppos, tmp, ret);
	kfree(tmp);
	return ret;
}

/*
 * read the per-port QSFP data for port 2 (pidx 1)
 */
static ssize_t qsfp_2_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct qib_devdata *dd = private2dd(file);
	char *tmp;
	int ret;

	if (dd->num_pports < 2)
		return -ENODEV;

	tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = qib_qsfp_dump(dd->pport + 1, tmp, PAGE_SIZE);
	if (ret > 0)
		ret = simple_read_from_buffer(buf, count, ppos, tmp, ret);
	kfree(tmp);
	return ret;
}

static const struct file_operations qsfp_ops[] = {
	{ .read = qsfp_1_read, .llseek = generic_file_llseek, },
	{ .read = qsfp_2_read, .llseek = generic_file_llseek, },
};

static ssize_t flash_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	struct qib_devdata *dd;
	ssize_t ret;
	loff_t pos;
	char *tmp;

	pos = *ppos;

	if (pos < 0) {
		ret = -EINVAL;
		goto bail;
	}

	if (pos >= sizeof(struct qib_flash)) {
		ret = 0;
		goto bail;
	}

	if (count > sizeof(struct qib_flash) - pos)
		count = sizeof(struct qib_flash) - pos;

	tmp = kmalloc(count, GFP_KERNEL);
	if (!tmp) {
		ret = -ENOMEM;
		goto bail;
	}

	dd = private2dd(file);
	if (qib_eeprom_read(dd, pos, tmp, count)) {
		qib_dev_err(dd, "failed to read from flash\n");
		ret = -ENXIO;
		goto bail_tmp;
	}

	if (copy_to_user(buf, tmp, count)) {
		ret = -EFAULT;
		goto bail_tmp;
	}

	*ppos = pos + count;
	ret = count;

bail_tmp:
	kfree(tmp);

bail:
	return ret;
}

static ssize_t flash_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct qib_devdata *dd;
	ssize_t ret;
	loff_t pos;
	char *tmp;

	pos = *ppos;

	if (pos != 0) {
		ret = -EINVAL;
		goto bail;
	}

	if (count != sizeof(struct qib_flash)) {
		ret = -EINVAL;
		goto bail;
	}

	tmp = kmalloc(count, GFP_KERNEL);
	if (!tmp) {
		ret = -ENOMEM;
		goto bail;
	}

	if (copy_from_user(tmp, buf, count)) {
		ret = -EFAULT;
		goto bail_tmp;
	}

	dd = private2dd(file);
	if (qib_eeprom_write(dd, pos, tmp, count)) {
		ret = -ENXIO;
		qib_dev_err(dd, "failed to write to flash\n");
		goto bail_tmp;
	}

	*ppos = pos + count;
	ret = count;

bail_tmp:
	kfree(tmp);

bail:
	return ret;
}

static const struct file_operations flash_ops = {
	.read = flash_read,
	.write = flash_write,
	.llseek = default_llseek,
};

static int add_cntr_files(struct super_block *sb, struct qib_devdata *dd)
{
	struct dentry *dir, *tmp;
	char unit[10];
	int ret, i;

	/* create the per-unit directory */
	snprintf(unit, sizeof(unit), "%u", dd->unit);
	ret = create_file(unit, S_IFDIR|S_IRUGO|S_IXUGO, sb->s_root, &dir,
			  &simple_dir_operations, dd);
	if (ret) {
		pr_err("create_file(%s) failed: %d\n", unit, ret);
		goto bail;
	}

	/* create the files in the new directory */
	ret = create_file("counters", S_IFREG|S_IRUGO, dir, &tmp,
			  &cntr_ops[0], dd);
	if (ret) {
		pr_err("create_file(%s/counters) failed: %d\n",
		       unit, ret);
		goto bail;
	}
	ret = create_file("counter_names", S_IFREG|S_IRUGO, dir, &tmp,
			  &cntr_ops[1], dd);
	if (ret) {
		pr_err("create_file(%s/counter_names) failed: %d\n",
		       unit, ret);
		goto bail;
	}
	ret = create_file("portcounter_names", S_IFREG|S_IRUGO, dir, &tmp,
			  &portcntr_ops[0], dd);
	if (ret) {
		pr_err("create_file(%s/%s) failed: %d\n",
		       unit, "portcounter_names", ret);
		goto bail;
	}
	for (i = 1; i <= dd->num_pports; i++) {
		char fname[24];

		sprintf(fname, "port%dcounters", i);
		/* create the files in the new directory */
		ret = create_file(fname, S_IFREG|S_IRUGO, dir, &tmp,
				  &portcntr_ops[i], dd);
		if (ret) {
			pr_err("create_file(%s/%s) failed: %d\n",
				unit, fname, ret);
			goto bail;
		}
		if (!(dd->flags & QIB_HAS_QSFP))
			continue;
		sprintf(fname, "qsfp%d", i);
		ret = create_file(fname, S_IFREG|S_IRUGO, dir, &tmp,
				  &qsfp_ops[i - 1], dd);
		if (ret) {
			pr_err("create_file(%s/%s) failed: %d\n",
				unit, fname, ret);
			goto bail;
		}
	}

	ret = create_file("flash", S_IFREG|S_IWUSR|S_IRUGO, dir, &tmp,
			  &flash_ops, dd);
	if (ret)
		pr_err("create_file(%s/flash) failed: %d\n",
			unit, ret);
bail:
	return ret;
}

static int remove_file(struct dentry *parent, char *name)
{
	struct dentry *tmp;
	int ret;

	tmp = lookup_one_len(name, parent, strlen(name));

	if (IS_ERR(tmp)) {
		ret = PTR_ERR(tmp);
		goto bail;
	}

	spin_lock(&tmp->d_lock);
	if (simple_positive(tmp)) {
		__d_drop(tmp);
		spin_unlock(&tmp->d_lock);
		simple_