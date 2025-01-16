/*
 * file for managing the edac_device subsystem of devices for EDAC
 *
 * (C) 2007 SoftwareBitMaker 
 *
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * Written Doug Thompson <norsk5@xmission.com>
 *
 */

#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/edac.h>

#include "edac_core.h"
#include "edac_module.h"

#define EDAC_DEVICE_SYMLINK	"device"

#define to_edacdev(k) container_of(k, struct edac_device_ctl_info, kobj)
#define to_edacdev_attr(a) container_of(a, struct edacdev_attribute, attr)


/*
 * Set of edac_device_ctl_info attribute store/show functions
 */

/* 'log_ue' */
static ssize_t edac_device_ctl_log_ue_show(struct edac_device_ctl_info
					*ctl_info, char *data)
{
	return sprintf(data, "%u\n", ctl_info->log_ue);
}

static ssize_t edac_device_ctl_log_ue_store(struct edac_device_ctl_info
					*ctl_info, const char *data,
					size_t count)
{
	/* if parameter is zero, turn off flag, if non-zero turn on flag */
	ctl_info->log_ue = (simple_strtoul(data, NULL, 0) != 0);

	return count;
}

/* 'log_ce' */
static ssize_t edac_device_ctl_log_ce_show(struct edac_device_ctl_info
					*ctl_info, char *data)
{
	return sprintf(data, "%u\n", ctl_info->log_ce);
}

static ssize_t edac_device_ctl_log_ce_store(struct edac_device_ctl_info
					*ctl_info, const char *data,
					size_t count)
{
	/* if parameter is zero, turn off flag, if non-zero turn on flag */
	ctl_info->log_ce = (simple_strtoul(data, NULL, 0) != 0);

	return count;
}

/* 'panic_on_ue' */
static ssize_t edac_device_ctl_panic_on_ue_show(struct edac_device_ctl_info
						*ctl_info, char *data)
{
	return sprintf(data, "%u\n", ctl_info->panic_on_ue);
}

static ssize_t edac_device_ctl_panic_on_ue_store(struct edac_device_ctl_info
						 *ctl_info, const char *data,
						 size_t count)
{
	/* if parameter is zero, turn off flag, if non-zero turn on flag */
	ctl_info->panic_on_ue = (simple_strtoul(data, NULL, 0) != 0);

	return count;
}

/* 'poll_msec' show and store functions*/
static ssize_t edac_device_ctl_poll_msec_show(struct edac_device_ctl_info
					*ctl_info, char *data)
{
	return sprintf(data, "%u\n", ctl_info->poll_msec);
}

static ssize_t edac_device_ctl_poll_msec_store(struct edac_device_ctl_info
					*ctl_info, const char *data,
					size_t count)
{
	unsigned long value;

	/* get the value and enforce that it is non-zero, must be at least
	 * one millisecond for the delay period, between scans
	 * Then cancel last outstanding delay for the work request
	 * and set a new one.
	 */
	value = simple_strtoul(data, NULL, 0);
	edac_device_reset_delay_period(ctl_info, value);

	return count;
}

/* edac_device_ctl_info specific attribute structure */
struct ctl_info_attribute {
	struct attribute attr;
	ssize_t(*show) (struct edac_device_ctl_info *, char *);
	ssize_t(*store) (struct edac_device_ctl_info *, const char *, size_t);
};

#define to_ctl_info(k) container_of(k, struct edac_device_ctl_info, kobj)
#define to_ctl_info_attr(a) container_of(a,struct ctl_info_attribute,attr)

/* Function to 'show' fields from the edac_dev 'ctl_info' structure */
static ssize_t edac_dev_ctl_info_show(struct kobject *kobj,
				struct attribute *attr, char *buffer)
{
	struct edac_device_ctl_info *edac_dev = to_ctl_info(kobj);
	struct ctl_info_attribute *ctl_info_attr = to_ctl_info_attr(attr);

	if (ctl_info_attr->show)
		return ctl_info_attr->show(edac_dev, buffer);
	return -EIO;
}

/* Function to 'store' fields into the edac_dev 'ctl_info' structure */
static ssize_t edac_dev_ctl_info_store(struct kobject *kobj,
				struct attribute *attr,
				const char *buffer, size_t count)
{
	struct edac_device_ctl_info *edac_dev = to_ctl_info(kobj);
	struct ctl_info_attribute *ctl_info_attr = to_ctl_info_attr(attr);

	if (ctl_info_attr->store)
		return ctl_info_attr->store(edac_dev, buffer, count);
	return -EIO;
}

/* edac_dev file operations for an 'ctl_info' */
static const struct sysfs_ops device_ctl_info_ops = {
	.show = edac_dev_ctl_info_show,
	.store = edac_dev_ctl_info_store
};

#define CTL_INFO_ATTR(_name,_mode,_show,_store)        \
static struct ctl_info_attribute attr_ctl_info_##_name = {      \
	.attr = {.name = __stringify(_name), .mode = _mode },   \
	.show   = _show,                                        \
	.store  = _store,                                       \
};

/* Declare the various ctl_info attributes here and their respective ops */
CTL_INFO_ATTR(log_ue, S_IRUGO | S_IWUSR,
	edac_device_ctl_log_ue_show, edac_device_ctl_log_ue_store);
CTL_INFO_ATTR(log_ce, S_IRUGO | S_IWUSR,
	edac_device_ctl_log_ce_show, edac_device_ctl_log_ce_store);
CTL_INFO_ATTR(panic_on_ue, S_IRUGO | S_IWUSR,
	edac_device_ctl_panic_on_ue_show,
	edac_device_ctl_panic_on_ue_store);
CTL_INFO_ATTR(poll_msec, S_IRUGO | S_IWUSR,
	edac_device_ctl_poll_msec_show, edac_device_ctl_poll_msec_store);

/* Base Attributes of the EDAC_DEVICE ECC object */
static struct ctl_info_attribute *device_ctrl_attr[] = {
	&attr_ctl_info_panic_on_ue,
	&attr_ctl_info_log_ue,
	&attr_ctl_info_log_ce,
	&attr_ctl_info_poll_msec,
	NULL,
};

/*
 * edac_device_ctrl_master_release
 *
 *	called when the reference count for the 'main' kobj
 *	for a edac_device control struct reaches zero
 *
 *	Reference count model:
 *		One 'main' kobject for each control structure allocated.
 *		That main kobj is initially set to one AND
 *		the reference count for the EDAC 'core' module is
 *		bumped by one, thus added 'keep in memory' dependency.
 *
 *		Each new internal kobj (in instances and blocks) then
 *		bumps the 'main' kobject.
 *
 *		When they are released their release functions decrement
 *		the 'main' kobj.
 *
 *		When the main kobj reaches zero (0) then THIS function
 *		is called which then decrements the EDAC 'core' module.
 *		When the module reference count reaches zero then the
 *		module no longer has dependency on keeping the release
 *		function code in memory and module can be unloaded.
 *
 *		This will support several control objects as well, each
 *		with its own 'main' kobj.
 */
static void edac_device_ctrl_master_release(struct kobject *kobj)
{
	struct edac_device_ctl_info *edac_dev = to_edacdev(kobj);

	edac_dbg(4, "control index=%d\n", edac_dev->dev_idx);

	/* decrement the EDAC CORE module ref count */
	module_put(edac_dev->owner);

	/* free the control struct containing the 'main' kobj
	 * passed in to this routine
	 */
	kfree(edac_dev);
}

/* ktype for the main (master) kobject */
static struct kobj_type ktype_device_ctrl = {
	.release = edac_device_ctrl_master_release,
	.sysfs_ops = &device_ctl_info_ops,
	.default_attrs = (struct attribute **)device_ctrl_attr,
};

/*
 * edac_device_register_sysfs_main_kobj
 *
 *	perform the high level setup for the new edac_device instance
 *
 * Return:  0 SUCCESS
 *         !0 FAILURE
 */
int edac_device_register_sysfs_main_kobj(struct edac_device_ctl_info *edac_dev)
{
	struct bus_type *edac_subsys;
	int err;

	edac_dbg(1, "\n");

	/* get the /sys/devices/system/edac reference */
	edac_subsys = edac_get_sysfs_subsys();
	if (edac_subsys == NULL) {
		edac_dbg(1, "no edac_subsys error\n");
		err = -ENODEV;
		goto err_out;
	}

	/* Point to the 'edac_subsys' this instance 'reports' to */
	edac_dev->edac_subsys = edac_subsys;

	/* Init the devices's kobject */
	memset(&edac_dev->kobj, 0, sizeof(struct kobject));

	/* Record which module 'owns' this control structure
	 * and bump the ref count of the module
	 */
	edac_dev->owner = THIS_MODULE;

	if (!try_module_get(edac_dev->owner)) {
		err = -ENODEV;
		goto err_mod_get;
	}

	/* register */
	err = kobject_init_and_add(&edac_dev->kobj, &ktype_device_ctrl,
				   &edac_subsys->dev_root->kobj,
				   "%s", edac_dev->name);
	if (err) {
		edac_dbg(1, "Failed to register '.../edac/%s'\n",
			 edac_dev->name);
		goto err_kobj_reg;
	}
	kobject_uevent(&edac_dev->kobj, KOBJ_ADD);

	/* At this point, to 'free' the control struct,
	 * edac_device_unregister_sysfs_main_kobj() must be used
	 */

	edac_dbg(4, "Registered '.../edac/%s' kobject\n", edac_dev->name);

	return 0;

	/* Error exit stack */
err_kobj_reg:
	kobject_put(&edac_dev->kobj);
	module_put(edac_dev->owner);

err_mod_get:
	edac_put_sysfs_subsys();

err_out:
	return err;
}

/*
 * edac_device_unregister_sysfs_main_kobj:
 *	the '..../edac/<name>' kobject
 */
void edac_device_unregister_sysfs_main_kobj(struct edac_device_ctl_info *dev)
{
	edac_dbg(0, "\n");
	edac_dbg(4, "name of kobject is: %s\n", kobject_name(&dev->kobj));

	/*
	 * Unregister the edac device's kobject and
	 * allow for reference count to reach 0 at which point
	 * the callback will be called to:
	 *   a) module_put() this module
	 *   b) 'kfree' the memory
	 */
	kobject_put(&dev->kobj);
	edac_put_sysfs_subsys();
}

/* edac_dev -> instance information */

/*
 * Set of low-level instance attribute show functions
 */
static ssize_t instance_ue_count_show(struct edac_device_instance *instance,
				char *data)
{
	return sprintf(data, "%u\n", instance->counters.ue_count);
}

static ssize_t instance_ce_count_show(struct edac_device_instance *instance,
				char *data)
{
	return sprintf(data, "%u\n", instance->counters.ce_count);
}

#define to_instance(k) container_of(k, struct edac_device_instance, kobj)
#define to_instance_attr(a) container_of(a,struct instance_attribute,attr)

/* DEVICE instance 