/*
 * (C) 2005, 2006 Linux Networx (http://lnxi.com)
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * Written Doug Thompson <norsk5@xmission.com>
 *
 */
#include <linux/module.h>
#include <linux/edac.h>
#include <linux/slab.h>
#include <linux/ctype.h>

#include "edac_core.h"
#include "edac_module.h"

#define EDAC_PCI_SYMLINK	"device"

/* data variables exported via sysfs */
static int check_pci_errors;		/* default NO check PCI parity */
static int edac_pci_panic_on_pe;	/* default NO panic on PCI Parity */
static int edac_pci_log_pe = 1;		/* log PCI parity errors */
static int edac_pci_log_npe = 1;	/* log PCI non-parity error errors */
static int edac_pci_poll_msec = 1000;	/* one second workq period */

static atomic_t pci_parity_count = ATOMIC_INIT(0);
static atomic_t pci_nonparity_count = ATOMIC_INIT(0);

static struct kobject *edac_pci_top_main_kobj;
static atomic_t edac_pci_sysfs_refcount = ATOMIC_INIT(0);

/* getter functions for the data variables */
int edac_pci_get_check_errors(void)
{
	return check_pci_errors;
}

static int edac_pci_get_log_pe(void)
{
	return edac_pci_log_pe;
}

static int edac_pci_get_log_npe(void)
{
	return edac_pci_log_npe;
}

static int edac_pci_get_panic_on_pe(void)
{
	return edac_pci_panic_on_pe;
}

int edac_pci_get_poll_msec(void)
{
	return edac_pci_poll_msec;
}

/**************************** EDAC PCI sysfs instance *******************/
static ssize_t instance_pe_count_show(struct edac_pci_ctl_info *pci, char *data)
{
	return sprintf(data, "%u\n", atomic_read(&pci->counters.pe_count));
}

static ssize_t instance_npe_count_show(struct edac_pci_ctl_info *pci,
				char *data)
{
	return sprintf(data, "%u\n", atomic_read(&pci->counters.npe_count));
}

#define to_instance(k) container_of(k, struct edac_pci_ctl_info, kobj)
#define to_instance_attr(a) container_of(a, struct instance_attribute, attr)

/* DEVICE instance kobject release() function */
static void edac_pci_instance_release(struct kobject *kobj)
{
	struct edac_pci_ctl_info *pci;

	edac_dbg(0, "\n");

	/* Form pointer to containing struct, the pci control struct */
	pci = to_instance(kobj);

	/* decrement reference count on top main kobj */
	kobject_put(edac_pci_top_main_kobj);

	kfree(pci);	/* Free the control struct */
}

/* instance specific attribute structure */
struct instance_attribute {
	struct attribute attr;
	ssize_t(*show) (struct edac_pci_ctl_info *, char *);
	ssize_t(*store) (struct edac_pci_ctl_info *, const char *, size_t);
};

/* Function to 'show' fields from the edac_pci 'instance' structure */
static ssize_t edac_pci_instance_show(struct kobject *kobj,
				struct attribute *attr, char *buffer)
{
	struct edac_pci_ctl_info *pci = to_instance(kobj);
	struct instance_attribute *instance_attr = to_instance_attr(attr);

	if (instance_attr->show)
		return instance_attr->show(pci, buffer);
	return -EIO;
}

/* Function to 'store' fields into the edac_pci 'instance' structure */
static ssize_t edac_pci_instance_store(struct kobject *kobj,
				struct attribute *attr,
				const char *buffer, size_t count)
{
	struct edac_pci_ctl_info *pci = to_instance(kobj);
	struct instance_attribute *instance_attr = to_instance_attr(attr);

	if (instance_attr->store)
		return instance_attr->store(pci, buffer, count);
	return -EIO;
}

/* fs_ops table */
static const struct sysfs_ops pci_instance_ops = {
	.show = edac_pci_instance_show,
	.store = edac_pci_instance_store
};

#define INSTANCE_ATTR(_name, _mode, _show, _store)	\
static struct instance_attribute attr_instance_##_name = {	\
	.attr	= {.name = __stringify(_name), .mode = _mode },	\
	.show	= _show,					\
	.store	= _store,					\
};

INSTANCE_ATTR(pe_count, S_IRUGO, instance_pe_count_show, NULL);
INSTANCE_ATTR(npe_count, S_IRUGO, instance_npe_count_show, NULL);

/* pci instance attributes */
static struct instance_attribute *pci_instance_attr[] = {
	&attr_instance_pe_count,
	&attr_instance_npe_count,
	NULL
};

/* the ktype for a pci instance */
static struct kobj_type ktype_pci_instance = {
	.release = edac_pci_instance_release,
	.sysfs_ops = &pci_instance_ops,
	.default_attrs = (struct attribute **)pci_instance_attr,
};

/*
 * edac_pci_create_instance_kobj
 *
 *	construct one EDAC PCI instance's kobject for use
 */
static int edac_pci_create_instance_kobj(struct edac_pci_ctl_info *pci, int idx)
{
	struct kobject *main_kobj;
	int err;

	edac_dbg(0, "\n");

	/* First bump the ref count on the top main kobj, which will
	 * track the number of PCI instances we have, and thus nest
	 * properly on keeping the module loaded
	 */
	main_kobj = kobject_get(edac_pci_top_main_kobj);
	if (!main_kobj) {
		err = -ENODEV;
		goto error_out;
	}

	/* And now register this new kobject under the main kobj */
	err = kobject_init_and_add(&pci->kobj, &ktype_pci_instance,
				   edac_pci_top_main_kobj, "pci%d", idx);
	if (err != 0) {
		edac_dbg(2, "failed to register instance pci%d\n", idx);
		kobject_put(edac_pci_top_main_kobj);
		goto error_out;
	}

	kobject_uevent(&pci->kobj, KOBJ_ADD);
	edac_dbg(1, "Register instance 'pci%d' kobject\n", idx);

	return 0;

	/* Error unwind statck */
error_out:
	return err;
}

/*
 * edac_pci_unregister_sysfs_instance_kobj
 *
 *	unregister the kobj for the EDAC PCI instance
 */
static void edac_pci_unregister_sysfs_instance_kobj(
			struct edac_pci_ctl_info *pci)
{
	edac_dbg(0, "\n");

	/* Unregister the instance kobject and allow its release
	 * function release the main reference count and then
	 * kfree the memory
	 */
	kobject_put(&pci->kobj);
}

/***************************** EDAC PCI sysfs root **********************/
#define to_edacpci(k) container_of(k, struct edac_pci_ctl_info, kobj)
#define to_edacpci_attr(a) container_of(a, struct edac_pci_attr, attr)

/* simple show/store functions for attributes */
static ssize_t edac_pci_int_show(void *ptr, char *buffer)
{
	int *value = ptr;
	return sprintf(buffer, "%d\n", *value);
}

static ssize_t edac_pci_int_store(void *ptr, const char *buffer, size_t count)
{
	int *value = ptr;

	if (isdigit(*buffer))
		*value = simple_strtoul(buffer, NULL, 0);

	return count;
}

struct edac_pci_dev_attribute {
	struct attribute attr;
	void *value;
	 ssize_t(*show) (void *, char *);
	 ssize_t(*store) (void *, const char *, size_t);
};

/* Set of show/store abstract level functions for PCI Parity object */
static ssize_t edac_pci_dev_show(struct kobject *kobj, struct attribute *attr,
				 char *buffer)
{
	struct edac_pci_dev_attribute *edac_pci_dev;
	edac_pci_dev = (struct edac_pci_dev_attribute *)attr;

	if (edac_pci_dev->show)
		return edac_pci_dev->show(edac_pci_dev->value, buffer);
	return -EIO;
}

static ssize_t edac_pci_dev_store(struct kobject *kobj,
				struct attribute *attr, const char *buffer,
				size_t count)
{
	struct edac_pci_dev_attribute *edac_pci_dev;
	edac_pci_dev = (struct edac_pci_dev_attribute *)attr;

	if (edac_pci_dev->store)
		return edac_pci_dev->store(edac_pci_dev->value, buffer, count);
	return -EIO;
}

static const struct sysfs_ops edac_pci_sysfs_ops = {
	.show = edac_pci_dev_show,
	.store = edac_pci_dev_store
};

#define EDAC_PCI_ATTR(_name,_mode,_show,_store)			\
static struct edac_pci_dev_attribute edac_pci_attr_##_name = {		\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.value  = &_name,					\
	.show   = _show,					\
	.store  = _store,					\
};

#define EDAC_PCI_STRING_ATTR(_name,_data,_mode,_show,_store)	\
static struct edac_pci_dev_attribute edac_pci_attr_##_name = {		\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.value  = _data,					\
	.show   = _show,					\
	.store  = _store,					\
};

/* PCI Parity control files */
EDAC_PCI_ATTR(check_pci_errors, S_IRUGO | S_IWUSR, edac_pci_int_show,
	edac_pci_int_store);
EDAC_PCI_ATTR(edac_pci_log_pe, S_IRUGO | S_IWUSR, edac_pci_int_show,
	edac_pci_int_store);
EDAC_PCI_ATTR(edac_pci_log_npe, S_IRUGO | S_IWUSR, edac_pci_int_show,
	edac_pci_int_store);
EDAC_PCI_ATTR(edac_pci_panic_on_pe, S_IRUGO | S_IWUSR, edac_pci_int_show,
	edac_pci_int_store);
EDAC_PCI_ATTR(pci_parity_count, S_IRUGO, edac_pci_int_show, NULL);
EDAC_PCI_ATTR(pci_nonparity_count, S_IRUGO, edac_pci_int_show, NULL);

/* Base Attributes of the memory ECC object */
static struct edac_pci_dev_attribute *edac_pci_attr[] = {
	&edac_pci_attr_check_pci_errors,
	&edac_pci_attr_edac_pci_log_pe,
	&edac_pci_attr_edac_pci_log_npe,
	&edac_pci_attr_edac_pci_panic_on_pe,
	&edac_pci_attr_pci_parity_count,
	&edac_pci_attr_pci_nonparity_count,
	NULL,
};

/*
 * edac_pci_release_main_kobj
 *
 *	This release function is called when the reference count to the
 *	passed kobj goes to zero.
 *
 *	This kobj is the 'main' kobject that EDAC PCI instances
 *	link to, and thus provide for proper nesting counts
 */
static void edac_pci_release_main_kobj(struct kobject *kobj)
{
	edac_dbg(0, "here to module_put(THIS_MODULE)\n");

	kfree(kobj);

	/* last reference to top EDAC PCI kobject has been removed,
	 * NOW release our ref count on the core module
	 */
	module_put(THIS_MODULE);
}

/* ktype struct for the EDAC PCI main kobj */
static struct kobj_type ktype_edac_pci_main_kobj = {
	.release = edac_pci_release_main_kobj,
	.sysfs_ops = &edac_pci_sysfs_ops,
	.default_attrs = (struct attribute **)edac_pci_attr,
};

/**
 * edac_pci_main_kobj_setup()
 *
 *	setup the sysfs for EDAC PCI attributes
 *	assumes edac_subsys has already been initialized
 */
static int edac_pci_main_kobj_setup(void)
{
	int err;
	struct bus_type *edac_subsys;

	edac_dbg(0, "\n");

	/* check and count if we have already created the main kobject */
	if (atomic_inc_return(&edac_pci_sysfs_refcount) != 1)
		return 0;

	/* First time, so create the main kobject and its
	 * controls and attributes
	 */
	edac_subsys = edac_get_sysfs_subsys();
	if (edac_subsys == NULL) {
		edac_dbg(1, "no edac_subsys\n");
		err = -ENODEV;
		goto decrement_count_fail;
	}

	/* Bump the reference count on this module to ensure the
	 * modules isn't unloaded until we deconstruct the top
	 * level main kobj for EDAC PCI
	 */
	if (!try_module_get(THIS_MODULE)) {
		edac_dbg(1, "try_module_get() failed\n");
		err = -ENODEV;
		goto mod_get_fail;
	}

	edac_pci_top_main_kobj = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (!edac_pci_top_main_kobj) {
		edac_dbg(1, "Failed to allocate\n");
		err = -ENOMEM;
		goto kzalloc_fail;
	}

	/* Instanstiate the pci object */
	err = kobject_init_and_add(edac_pci_top_main_kobj,
				   &ktype_edac_pci_main_kobj,
				   &edac_subsys->dev_root->kobj, "pci");
	if (err) {
		edac_dbg(1, "Failed to register '.../edac/pci'\n");
		goto kobject_init_and_add_fail;
	}

	/* At this point, to 'release' the top level kobject
	 * for EDAC PCI, then edac_pci_main_kobj_teardown()
	 * must be used, for resources to be cleaned up properly
	 */
	kobject_uevent(edac_pci_top_main_kobj, KOBJ_ADD);
	edac_dbg(1, "Registered '.../edac/pci' kobject\n");

	return 0;

	/* Error unwind statck */
kobject_init_and_add_fail:
	kobject_put(edac_pci_top_main_kobj);

kzalloc_fail:
	module_put(THIS_MODULE);

mod_get_fail:
	edac_put_sysfs_subsys();

decrement_count_fail:
	/* if are on this error exit, nothing to tear down */
	atomic_dec(&edac_pci_sysfs_refcount);

	return err;
}

/*
 * 