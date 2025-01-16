e files
 *
 */
DEVICE_CHANNEL(ch0_dimm_label, S_IRUGO | S_IWUSR,
	channel_dimm_label_show, channel_dimm_label_store, 0);
DEVICE_CHANNEL(ch1_dimm_label, S_IRUGO | S_IWUSR,
	channel_dimm_label_show, channel_dimm_label_store, 1);
DEVICE_CHANNEL(ch2_dimm_label, S_IRUGO | S_IWUSR,
	channel_dimm_label_show, channel_dimm_label_store, 2);
DEVICE_CHANNEL(ch3_dimm_label, S_IRUGO | S_IWUSR,
	channel_dimm_label_show, channel_dimm_label_store, 3);
DEVICE_CHANNEL(ch4_dimm_label, S_IRUGO | S_IWUSR,
	channel_dimm_label_show, channel_dimm_label_store, 4);
DEVICE_CHANNEL(ch5_dimm_label, S_IRUGO | S_IWUSR,
	channel_dimm_label_show, channel_dimm_label_store, 5);
DEVICE_CHANNEL(ch6_dimm_label, S_IRUGO | S_IWUSR,
	channel_dimm_label_show, channel_dimm_label_store, 6);
DEVICE_CHANNEL(ch7_dimm_label, S_IRUGO | S_IWUSR,
	channel_dimm_label_show, channel_dimm_label_store, 7);

/* Total possible dynamic DIMM Label attribute file table */
static struct attribute *dynamic_csrow_dimm_attr[] = {
	&dev_attr_legacy_ch0_dimm_label.attr.attr,
	&dev_attr_legacy_ch1_dimm_label.attr.attr,
	&dev_attr_legacy_ch2_dimm_label.attr.attr,
	&dev_attr_legacy_ch3_dimm_label.attr.attr,
	&dev_attr_legacy_ch4_dimm_label.attr.attr,
	&dev_attr_legacy_ch5_dimm_label.attr.attr,
	&dev_attr_legacy_ch6_dimm_label.attr.attr,
	&dev_attr_legacy_ch7_dimm_label.attr.attr,
	NULL
};

/* possible dynamic channel ce_count attribute files */
DEVICE_CHANNEL(ch0_ce_count, S_IRUGO,
		   channel_ce_count_show, NULL, 0);
DEVICE_CHANNEL(ch1_ce_count, S_IRUGO,
		   channel_ce_count_show, NULL, 1);
DEVICE_CHANNEL(ch2_ce_count, S_IRUGO,
		   channel_ce_count_show, NULL, 2);
DEVICE_CHANNEL(ch3_ce_count, S_IRUGO,
		   channel_ce_count_show, NULL, 3);
DEVICE_CHANNEL(ch4_ce_count, S_IRUGO,
		   channel_ce_count_show, NULL, 4);
DEVICE_CHANNEL(ch5_ce_count, S_IRUGO,
		   channel_ce_count_show, NULL, 5);
DEVICE_CHANNEL(ch6_ce_count, S_IRUGO,
		   channel_ce_count_show, NULL, 6);
DEVICE_CHANNEL(ch7_ce_count, S_IRUGO,
		   channel_ce_count_show, NULL, 7);

/* Total possible dynamic ce_count attribute file table */
static struct attribute *dynamic_csrow_ce_count_attr[] = {
	&dev_attr_legacy_ch0_ce_count.attr.attr,
	&dev_attr_legacy_ch1_ce_count.attr.attr,
	&dev_attr_legacy_ch2_ce_count.attr.attr,
	&dev_attr_legacy_ch3_ce_count.attr.attr,
	&dev_attr_legacy_ch4_ce_count.attr.attr,
	&dev_attr_legacy_ch5_ce_count.attr.attr,
	&dev_attr_legacy_ch6_ce_count.attr.attr,
	&dev_attr_legacy_ch7_ce_count.attr.attr,
	NULL
};

static umode_t csrow_dev_is_visible(struct kobject *kobj,
				    struct attribute *attr, int idx)
{
	struct device *dev = kobj_to_dev(kobj);
	struct csrow_info *csrow = container_of(dev, struct csrow_info, dev);

	if (idx >= csrow->nr_channels)
		return 0;

	if (idx >= ARRAY_SIZE(dynamic_csrow_ce_count_attr) - 1) {
		WARN_ONCE(1, "idx: %d\n", idx);
		return 0;
	}

	/* Only expose populated DIMMs */
	if (!csrow->channels[idx]->dimm->nr_pages)
		return 0;

	return attr->mode;
}


static const struct attribute_group csrow_dev_dimm_group = {
	.attrs = dynamic_csrow_dimm_attr,
	.is_visible = csrow_dev_is_visible,
};

static const struct attribute_group csrow_dev_ce_count_group = {
	.attrs = dynamic_csrow_ce_count_attr,
	.is_visible = csrow_dev_is_visible,
};

static const struct attribute_group *csrow_dev_groups[] = {
	&csrow_dev_dimm_group,
	&csrow_dev_ce_count_group,
	NULL
};

static inline int nr_pages_per_csrow(struct csrow_info *csrow)
{
	int chan, nr_pages = 0;

	for (chan = 0; chan < csrow->nr_channels; chan++)
		nr_pages += csrow->channels[chan]->dimm->nr_pages;

	return nr_pages;
}

/* Create a CSROW object under specifed edac_mc_device */
static int edac_create_csrow_object(struct mem_ctl_info *mci,
				    struct csrow_info *csrow, int index)
{
	int err;

	csrow->dev.type = &csrow_attr_type;
	csrow->dev.bus = mci->bus;
	csrow->dev.groups = csrow_dev_groups;
	device_initialize(&csrow->dev);
	csrow->dev.parent = &mci->dev;
	csrow->mci = mci;
	dev_set_name(&csrow->dev, "csrow%d", index);
	dev_set_drvdata(&csrow->dev, csrow);

	edac_dbg(0, "creating (virtual) csrow node %s\n",
		 dev_name(&csrow->dev));

	err = device_add(&csrow->dev);
	if (err)
		put_device(&csrow->dev);

	return err;
}

/* Create a CSROW object under specifed edac_mc_device */
static int edac_create_csrow_objects(struct mem_ctl_info *mci)
{
	int err, i;
	struct csrow_info *csrow;

	for (i = 0; i < mci->nr_csrows; i++) {
		csrow = mci->csrows[i];
		if (!nr_pages_per_csrow(csrow))
			continue;
		err = edac_create_csrow_object(mci, mci->csrows[i], i);
		if (err < 0) {
			edac_dbg(1,
				 "failure: create csrow objects for csrow %d\n",
				 i);
			goto error;
		}
	}
	return 0;

error:
	for (--i; i >= 0; i--) {
		csrow = mci->csrows[i];
		if (!nr_pages_per_csrow(csrow))
			continue;
		put_device(&mci->csrows[i]->dev);
	}

	return err;
}

static void edac_delete_csrow_objects(struct mem_ctl_info *mci)
{
	int i;
	struct csrow_info *csrow;

	for (i = mci->nr_csrows - 1; i >= 0; i--) {
		csrow = mci->csrows[i];
		if (!nr_pages_per_csrow(csrow))
			continue;
		device_unregister(&mci->csrows[i]->dev);
	}
}
#endif

/*
 * Per-dimm (or per-rank) devices
 */

#define to_dimm(k) container_of(k, struct dimm_info, dev)

/* show/store functions for DIMM Label attributes */
static ssize_t dimmdev_location_show(struct device *dev,
				     struct device_attribute *mattr, char *data)
{
	struct dimm_info *dimm = to_dimm(dev);

	return edac_dimm_info_location(dimm, data, PAGE_SIZE);
}

static ssize_t dimmdev_label_show(struct device *dev,
				  struct device_attribute *mattr, char *data)
{
	struct dimm_info *dimm = to_dimm(dev);

	/* if field has not been initialized, there is nothing to send */
	if (!dimm->label[0])
		return 0;

	return snprintf(data, sizeof(dimm->label) + 1, "%s\n", dimm->label);
}

static ssize_t dimmdev_label_store(struct device *dev,
				   struct device_attribute *mattr,
				   const char *data,
				   size_t count)
{
	struct dimm_info *dimm = to_dimm(dev);
	size_t copy_count = count;

	if (count == 0)
		return -EINVAL;

	if (data[count - 1] == '\0' || data[count - 1] == '\n')
		copy_count -= 1;

	if (copy_count == 0 || copy_count >= sizeof(dimm->label))
		return -EINVAL;

	strncpy(dimm->label, data, copy_count);
	dimm->label[copy_count] = '\0';

	return count;
}

static ssize_t dimmdev_size_show(struct device *dev,
				 struct device_attribute *mattr, char *data)
{
	struct dimm_info *dimm = to_dimm(dev);

	return sprintf(data, "%u\n", PAGES_TO_MiB(dimm->nr_pages));
}

static ssize_t dimmdev_mem_type_show(struct device *dev,
				     struct device_attribute *mattr, char *data)
{
	struct dimm_info *dimm = to_dimm(dev);

	return sprintf(data, "%s\n", mem_types[dimm->mtype]);
}

static ssize_t dimmdev_dev_type_show(struct device *dev,
				     struct device_attribute *mattr, char *data)
{
	struct dimm_info *dimm = to_dimm(dev);

	return sprintf(data, "%s\n", dev_types[dimm->dtype]);
}

static ssize_t dimmdev_edac_mode_show(struct device *dev,
				      struct device_attribute *mattr,
				      char *data)
{
	struct dimm_info *dimm = to_dimm(dev);

	return sprintf(data, "%s\n", edac_caps[dimm->edac_mode]);
}

/* dimm/rank attribute files */
static DEVICE_ATTR(dimm_label, S_IRUGO | S_IWUSR,
		   dimmdev_label_show, dimmdev_label_store);
static DEVICE_ATTR(dimm_location, S_IRUGO, dimmdev_location_show, NULL);
static DEVICE_ATTR(size, S_IRUGO, dimmdev_size_show, NULL);
static DEVICE_ATTR(dimm_mem_type, S_IRUGO, dimmdev_mem_type_show, NULL);
static DEVICE_ATTR(dimm_dev_type, S_IRUGO, dimmdev_dev_type_show, NULL);
static DEVICE_ATTR(dimm_edac_mode, S_IRUGO, dimmdev_edac_mode_show, NULL);

/* attributes of the dimm<id>/rank<id> object */
static struct attribute *dimm_attrs[] = {
	&dev_attr_dimm_label.attr,
	&dev_attr_dimm_location.attr,
	&dev_attr_size.attr,
	&dev_attr_dimm_mem_type.attr,
	&dev_attr_dimm_dev_type.attr,
	&dev_attr_dimm_edac_mode.attr,
	NULL,
};

static struct attribute_group dimm_attr_grp = {
	.attrs	= dimm_attrs,
};

static const struct attribute_group *dimm_attr_groups[] = {
	&dimm_attr_grp,
	NULL
};

static void dimm_attr_release(struct device *dev)
{
	struct dimm_info *dimm = container_of(dev, struct dimm_info, dev);

	edac_dbg(1, "Releasing dimm device %s\n", dev_name(dev));
	kfree(dimm);
}

static struct device_type dimm_attr_type = {
	.groups		= dimm_attr_groups,
	.release	= dimm_attr_release,
};

/* Create a DIMM object under specifed memory controller device */
static int edac_create_dimm_object(struct mem_ctl_info *mci,
				   struct dimm_info *dimm,
				   int index)
{
	int err;
	dimm->mci = mci;

	dimm->dev.type = &dimm_attr_type;
	dimm->dev.bus = mci->bus;
	device_initialize(&dimm->dev);

	dimm->dev.parent = &mci->dev;
	if (mci->csbased)
		dev_set_name(&dimm->dev, "rank%d", index);
	else
		dev_set_name(&dimm->dev, "dimm%d", index);
	dev_set_drvdata(&dimm->dev, dimm);
	pm_runtime_forbid(&mci->dev);

	err =  device_add(&dimm->dev);

	edac_dbg(0, "creating rank/dimm device %s\n", dev_name(&dimm->dev));

	return err;
}

/*
 * Memory controller device
 */

#define to_mci(k) container_of(k, struct mem_ctl_info, dev)

static ssize_t mci_reset_counters_store(struct device *dev,
					struct device_attribute *mattr,
					const char *data, size_t count)
{
	struct mem_ctl_info *mci = to_mci(dev);
	int cnt, row, chan, i;
	mci->ue_mc = 0;
	mci->ce_mc = 0;
	mci->ue_noinfo_count = 0;
	mci->ce_noinfo_count = 0;

	for (row = 0; row < mci->nr_csrows; row++) {
		struct csrow_info *ri = mci->csrows[row];

		ri->ue_count = 0;
		ri->ce_count = 0;

		for (chan = 0; chan < ri->nr_channels; chan++)
			ri->channels[chan]->ce_count = 0;
	}

	cnt = 1;
	for (i = 0; i < mci->n_layers; i++) {
		cnt *= mci->layers[i].size;
		memset(mci->ce_per_layer[i], 0, cnt * sizeof(u32));
		memset(mci->ue_per_layer[i], 0, cnt * sizeof(u32));
	}

	mci->start_time = jiffies;
	return count;
}

/* Memory scrubbing interface:
 *
 * A MC driver can limit the scrubbing bandwidth based on the CPU type.
 * Therefore, ->set_sdram_scrub_rate should be made to return the actual
 * bandwidth that is accepted or 0 when scrubbing is to be disabled.
 *
 * Negative value still means that an error has occurred while setting
 * the scrub rate.
 */
static ssize_t mci_sdram_scrub_rate_store(struct device *dev,
					  struct device_attribute *mattr,
					  const char *data, size_t count)
{
	struct mem_ctl_info *mci = to_mci(dev);
	unsigned long bandwidth = 0;
	int new_bw = 0;

	if (kstrtoul(data, 10, &bandwidth) < 0)
		return -EINVAL;

	new_bw = mci->set_sdram_scrub_rate(mci, bandwidth);
	if (new_bw < 0) {
		edac_printk(KERN_WARNING, EDAC_MC,
			    "Error setting scrub rate to: %lu\n", bandwidth);
		return -EINVAL;
	}

	return count;
}

/*
 * ->get_sdram_scrub_rate() return value semantics same as above.
 */
static ssize_t mci_sdram_scrub_rate_show(struct device *dev,
					 struct device_attribute *mattr,
					 char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);
	int bandwidth = 0;

	bandwidth = mci->get_sdram_scrub_rate(mci);
	if (bandwidth < 0) {
		edac_printk(KERN_DEBUG, EDAC_MC, "Error reading scrub rate\n");
		return bandwidth;
	}

	return sprintf(data, "%d\n", bandwidth);
}

/* default attribute files for the MCI object */
static ssize_t mci_ue_count_show(struct device *dev,
				 struct device_attribute *mattr,
				 char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);

	return sprintf(data, "%d\n", mci->ue_mc);
}

static ssize_t mci_ce_count_show(struct device *dev,
				 struct device_attribute *mattr,
				 char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);

	return sprintf(data, "%d\n", mci->ce_mc);
}

static ssize_t mci_ce_noinfo_show(struct device *dev,
				  struct device_attribute *mattr,
				  char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);

	return sprintf(data, "%d\n", mci->ce_noinfo_count);
}

static ssize_t mci_ue_noinfo_show(struct device *dev,
				  struct device_attribute *mattr,
				  char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);

	return sprintf(data, "%d\n", mci->ue_noinfo_count);
}

static ssize_t mci_seconds_show(struct device *dev,
				struct device_attribute *mattr,
				char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);

	return sprintf(data, "%ld\n", (jiffies - mci->start_time) / HZ);
}

static ssize_t mci_ctl_name_show(struct device *dev,
				 struct device_attribute *mattr,
				 char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);

	return sprintf(data, "%s\n", mci->ctl_name);
}

static ssize_t mci_size_mb_show(struct device *dev,
				struct device_attribute *mattr,
				char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);
	int total_pages = 0, csrow_idx, j;

	for (csrow_idx = 0; csrow_idx < mci->nr_csrows; csrow_idx++) {
		struct csrow_info *csrow = mci->csrows[csrow_idx];

		for (j = 0; j < csrow->nr_channels; j++) {
			struct dimm_info *dimm = csrow->channels[j]->dimm;

			total_pages += dimm->nr_pages;
		}
	}

	return sprintf(data, "%u\n", PAGES_TO_MiB(total_pages));
}

static ssize_t mci_max_location_show(struct device *dev,
				     struct device_attribute *mattr,
				     char *data)
{
	struct mem_ctl_info *mci = to_mci(dev);
	int i;
	char *p = data;

	for (i = 0; i < mci->n_layers; i++) {
		p += sprintf(p, "%s %d ",
			     edac_layer_name[mci->layers[i].type],
			     mci->layers[i].size - 1);
	}

	return p - data;
}

/* default Control file */
static DEVICE_ATTR(reset_counters, S_IWUSR, NULL, mci_reset_counters_store);

/* default Attribute files */
static DEVICE_ATTR(mc_name, S_IRUGO, mci_ctl_name_show, NULL);
static DEVICE_ATTR(size_mb, S_IRUGO, mci_size_mb_show, NULL);
static DEVICE_ATTR(seconds_since_reset, S_IRUGO, mci_seconds_show, NULL);
static DEVICE_ATTR(ue_noinfo_count, S_IRUGO, mci_ue_noinfo_show, NULL);
static DEVICE_ATTR(ce_noinfo_count, S_IRUGO, mci_ce_noinfo_show, NULL);
static DEVICE_ATTR(ue_count, S_IRUGO, mci_ue_count_show, NULL);
static DEVICE_ATTR(ce_count, S_IRUGO, mci_ce_count_show, NULL);
static DEVICE_ATTR(max_location, S_IRUGO, mci_max_location_show, NULL);

/* memory scrubber attribute file */
DEVICE_ATTR(sdram_scrub_rate, 0, mci_sdram_scrub_rate_show,
	    mci_sdram_scrub_rate_store); /* umode set later in is_visible */

static struct attribute *mci_attrs[] = {
	&dev_attr_reset_counters.attr,
	&dev_attr_mc_name.attr,
	&dev_attr_size_mb.attr,
	&dev_attr_seconds_since_reset.attr,
	&dev_attr_ue_noinfo_count.attr,
	&dev_attr_ce_noinfo_count.attr,
	&dev_attr_ue_count.attr,
	&dev_attr_ce_count.attr,
	&dev_attr_max_location.attr,
	&dev_attr_sdram_scrub_rate.attr,
	NULL
};

static umode_t mci_attr_is_visible(struct kobject *kobj,
				   struct attribute *attr, int idx)
{
	struct device *dev = kobj_to_dev(kobj);
	struct mem_ctl_info *mci = to_mci(dev);
	umode_t mode = 0;

	if (attr != &dev_attr_sdram_scrub_rate.attr)
		return attr->mode;
	if (mci->get_sdram_scrub_rate)
		mode |= S_IRUGO;
	if (mci->set_sdram_scrub_rate)
		mode |= S_IWUSR;
	return mode;
}

static struct attribute_group mci_attr_grp = {
	.attrs	= mci_attrs,
	.is_visible = mci_attr_is_visible,
};

static const struct attribute_group *mci_attr_groups[] = {
	&mci_attr_grp,
	NULL
};

static void mci_attr_release(struct device *dev)
{
	struct mem_ctl_info *mci = container_of(dev, struct mem_ctl_info, dev);

	edac_dbg(1, "Releasing csrow device %s\n", dev_name(dev));
	kfree(mci);
}

static struct device_type mci_attr_type = {
	.groups		= mci_attr_groups,
	.release	= mci_attr_release,
};

/*
 * Create a new Memory Controller kobject instance,
 *	mc<id> under the 'mc' directory
 *
 * Return:
 *	0	Success
 *	!0	Failure
 */
int edac_create_sysfs_mci_device(struct mem_ctl_info *mci,
				 const struct attribute_group **groups)
{
	char *name;
	int i, err;

	/*
	 * The memory controller needs its own bus, in order to avoid
	 * namespace conflicts at /sys/bus/edac.
	 */
	name = kasprintf(GFP_KERNEL, "mc%d", mci->mc_idx);
	if (!name)
		return -ENOMEM;

	mci->bus->name = name;

	edac_dbg(0, "creating bus %s\n", mci->bus->name);

	err = bus_register(mci->bus);
	if (err < 0) {
		kfree(name);
		return err;
	}

	/* get the /sys/devices/system/edac subsys reference */
	mci->dev.type = &mci_attr_type;
	device_initialize(&mci->dev);

	mci->dev.parent = mci_pdev;
	mci->dev.bus = mci->bus;
	mci->dev.groups = groups;
	dev_set_name(&mci->dev, "mc%d", mci->mc_idx);
	dev_set_drvdata(&mci->dev, mci);
	pm_runtime_forbid(&mci->dev);

	edac_dbg(0, "creating device %s\n", dev_name(&mci->dev));
	err = device_add(&mci->dev);
	if (err < 0) {
		edac_dbg(1, "failure: create device %s\n", dev_name(&mci->dev));
		goto fail_unregister_bus;
	}

	/*
	 * Create the dimm/rank devices
	 */
	for (i = 0; i < mci->tot_dimms; i++) {
		struct dimm_info *dimm = mci->dimms[i];
		/* Only expose populated DIMMs */
		if (!dimm->nr_pages)
			continue;

#ifdef CONFIG_EDAC_DEBUG
		edac_dbg(1, "creating dimm%d, located at ", i);
		if (edac_debug_level >= 1) {
			int lay;
			for (lay = 0; lay < mci->n_layers; lay++)
				printk(KERN_CONT "%s %d ",
					edac_layer_name[mci->layers[lay].type],
					dimm->location[lay]);
			printk(KERN_CONT "\n");
		}
#endif
		err = edac_create_dimm_object(mci, dimm, i);
		if (err) {
			edac_dbg(1, "failure: create dimm %d obj\n", i);
			goto fail_unregister_dimm;
		}
	}

#ifdef CONFIG_EDAC_LEGACY_SYSFS
	err = edac_create_csrow_objects(mci);
	if (err < 0)
		goto fail_unregister_dimm;
#endif

	edac_create_debugfs_nodes(mci);
	return 0;

fail_unregister_dimm:
	for (i--; i >= 0; i--) {
		struct dimm_info *dimm = mci->dimms[i];
		if (!dimm->nr_pages)
			continue;

		device_unregister(&dimm->dev);
	}
	device_unregister(&mci->dev);
fail_unregister_bus:
	bus_unregister(mci->bus);
	kfree(name);

	return err;
}

/*
 * remove a Memory Controller instance
 */
void edac_remove_sysfs_mci_device(struct mem_ctl_info *mci)
{
	int i;

	edac_dbg(0, "\n");

#ifdef CONFIG_EDAC_DEBUG
	edac_debugfs_remove_recursive(mci->debugfs);
#endif
#ifdef CONFIG_EDAC_LEGACY_SYSFS
	edac_delete_csrow_objects(mci);
#endif

	for (i = 0; i < mci->tot_dimms; i++) {
		struct dimm_info *dimm = mci->dimms[i];
		if (dimm->nr_pages == 0)
			continue;
		edac_dbg(0, "removing device %s\n", dev_name(&dimm->dev));
		device_unregister(&dimm->dev);
	}
}

void edac_unregister_sysfs(struct mem_ctl_info *mci)
{
	const char *name = mci->bus->name;

	edac_dbg(1, "Unregistering device %s\n", dev_name(&mci->dev));
	device_unregister(&mci->dev);
	bus_unregister(mci->bus);
	kfree(name);
}

static void mc_attr_release(struct device *dev)
{
	/*
	 * There's no container structure here, as this is just the mci
	 * parent device, used to create the /sys/devices/mc sysfs node.
	 * So, there are no attributes on it.
	 */
	edac_dbg(1, "Releasing device %s\n", dev_name(dev));
	kfree(dev);
}

static struct device_type mc_attr_type = {
	.release	= mc_attr_release,
};
/*
 * Init/exit code for the module. Basically, creates/removes /sys/class/rc
 */
int __init edac_mc_sysfs_init(void)
{
	struct bus_type *edac_subsys;
	int err;

	/* get the /sys/devices/system/edac subsys reference */
	edac_subsys = edac_get_sysfs_subsys();
	if (edac_subsys == NULL) {
		edac_dbg(1, "no edac_subsys\n");
		err = -EINVAL;
		goto out;
	}

	mci_pdev = kzalloc(sizeof(*mci_pdev), GFP_KERNEL);
	if (!mci_pdev) {
		err = -ENOMEM;
		goto out_put_sysfs;
	}

	mci_pdev->bus = edac_subsys;
	mci_pdev->type = &mc_attr_type;
	device_initialize(mci_pdev);
	dev_set_name(mci_pdev, "mc");

	err = device_add(mci_pdev);
	if (err < 0)
		goto out_dev_free;

	edac_dbg(0, "device %s created\n", dev_name(mci_pdev));

	return 0;

 out_dev_free:
	kfree(mci_pdev);
 out_put_sysfs:
	edac_put_sysfs_subsys();
 out:
	return err;
}

void edac_mc_sysfs_exit(void)
{
	device_unregister(mci_pdev);
	edac_put_sysfs_subsys();
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * edac_module.c
 *
 * (C) 2007 www.softwarebitmaker.com
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * Author: Doug Thompson <dougthompson@xmission.com>
 *
 */
#include <linux/edac.h>

#include "edac_core.h"
#include "edac_module.h"

#define EDAC_VERSION "Ver: 3.0.0"

#ifdef CONFIG_EDAC_DEBUG

static int edac_set_debug_level(const char *buf, struct kernel_param *kp)
{
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 0, &val);
	if (ret)
		return ret;

	if (val > 4)
		return -EINVAL;

	return param_set_int(buf, kp);
}

/* Values of 0 to 4 will generate output */
int edac_debug_level = 2;
EXPORT_SYMBOL_GPL(edac_debug_level);

module_param_call(edac_debug_level, edac_set_debug_level, param_get_int,
		  &edac_debug_level, 0644);
MODULE_PARM_DESC(edac_debug_level, "EDAC debug level: [0-4], default: 2");
#endif

/* scope is to module level only */
struct workqueue_struct *edac_workqueue;

/*
 * edac_op_state_to_string()
 */
char *edac_op_state_to_string(int opstate)
{
	if (opstate == OP_RUNNING_POLL)
		return "POLLED";
	else if (opstate == OP_RUNNING_INTERRUPT)
		return "INTERRUPT";
	else if (opstate == OP_RUNNING_POLL_INTR)
		return "POLL-INTR";
	else if (opstate == OP_ALLOC)
		return "ALLOC";
	else if (opstate == OP_OFFLINE)
		return "OFFLINE";

	return "UNKNOWN";
}

/*
 * edac_workqueue_setup
 *	initialize the edac work queue for polling operations
 */
static int edac_workqueue_setup(void)
{
	edac_workqueue = create_singlethread_workqueue("edac-poller");
	if (edac_workqueue == NULL)
		return -ENODEV;
	else
		return 0;
}

/*
 * edac_workqueue_teardown
 *	teardown the edac workqueue
 */
static void edac_workqueue_teardown(void)
{
	if (edac_workqueue) {
		flush_workqueue(edac_workqueue);
		destroy_workqueue(edac_workqueue);
		edac_workqueue = NULL;
	}
}

/*
 * edac_init
 *      module initialization entry point
 */
static int __init edac_init(void)
{
	int err = 0;

	edac_printk(KERN_INFO, EDAC_MC, EDAC_VERSION "\n");

	/*
	 * Harvest and clear any boot/initialization PCI parity errors
	 *
	 * FIXME: This only clears errors logged by devices present at time of
	 *      module initialization.  We should also do an initial clear
	 *      of each newly hotplugged device.
	 */
	edac_pci_clear_parity_errors();

	err = edac_mc_sysfs_init();
	if (err)
		goto err_sysfs;

	edac_debugfs_init();

	err = edac_workqueue_setup();
	if (err) {
		edac_printk(KERN_ERR, EDAC_MC, "Failure initializing workqueue\n");
		goto err_wq;
	}

	return 0;

err_wq:
	edac_debugfs_exit();
	edac_mc_sysfs_exit();

err_sysfs:
	return err;
}

/*
 * edac_exit()
 *      module exit/termination function
 */
static void __exit edac_exit(void)
{
	edac_dbg(0, "\n");

	/* tear down the various subsystems */
	edac_workqueue_teardown();
	edac_mc_sysfs_exit();
	edac_debugfs_exit();
}

/*
 * Inform the kernel of our entry and exit points
 */
subsys_initcall(edac_init);
module_exit(edac_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Doug Thompson www.softwarebitmaker.com, et al");
MODULE_DESCRIPTION("Core library routines for EDAC reporting");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
/*
 * edac_module.h
 *
 * For defining functions/data for within the EDAC_CORE module only
 *
 * written by doug thompson <norsk5@xmission.h>
 */

#ifndef	__EDAC_MODULE_H__
#define	__EDAC_MODULE_H__

#include "edac_core.h"

/*
 * INTERNAL EDAC MODULE:
 * EDAC memory controller sysfs create/remove functions
 * and setup/teardown functions
 *
 * edac_mc objects
 */
	/* on edac_mc_sysfs.c */
int edac_mc_sysfs_init(void);
void edac_mc_sysfs_exit(void);
extern int edac_create_sysfs_mci_device(struct mem_ctl_info *mci,
					const struct attribute_group **groups);
extern void edac_remove_sysfs_mci_device(struct mem_ctl_info *mci);
void edac_unregister_sysfs(struct mem_ctl_info *mci);
extern int edac_get_log_ue(void);
extern int edac_get_log_ce(void);
extern int edac_get_panic_on_ue(void);
extern int edac_mc_get_log_ue(void);
extern int edac_mc_get_log_ce(void);
extern int edac_mc_get_panic_on_ue(void);
extern int edac_get_poll_msec(void);
extern unsigned int edac_mc_get_poll_msec(void);

unsigned edac_dimm_info_location(struct dimm_info *dimm, char *buf,
				 unsigned len);

	/* on edac_device.c */
extern int edac_device_register_sysfs_main_kobj(
				struct edac_device_ctl_info *edac_dev);
extern void edac_device_unregister_sysfs_main_kobj(
				struct edac_device_ctl_info *edac_dev);
extern int edac_device_create_sysfs(struct edac_device_ctl_info *edac_dev);
extern void edac_device_remove_sysfs(struct edac_device_ctl_info *edac_dev);

/* edac core workqueue: single CPU mode */
extern struct workqueue_struct *edac_workqueue;
extern void edac_dev