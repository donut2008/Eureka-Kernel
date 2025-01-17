#include "edac_module.h"

static struct dentry *edac_debugfs;

static ssize_t edac_fake_inject_write(struct file *file,
				      const char __user *data,
				      size_t count, loff_t *ppos)
{
	struct device *dev = file->private_data;
	struct mem_ctl_info *mci = to_mci(dev);
	static enum hw_event_mc_err_type type;
	u16 errcount = mci->fake_inject_count;

	if (!errcount)
		errcount = 1;

	type = mci->fake_inject_ue ? HW_EVENT_ERR_UNCORRECTED
				   : HW_EVENT_ERR_CORRECTED;

	printk(KERN_DEBUG
	       "Generating %d %s fake error%s to %d.%d.%d to test core handling. NOTE: this won't test the driver-specific decoding logic.\n",
		errcount,
		(type == HW_EVENT_ERR_UNCORRECTED) ? "UE" : "CE",
		errcount > 1 ? "s" : "",
		mci->fake_inject_layer[0],
		mci->fake_inject_layer[1],
		mci->fake_inject_layer[2]
	       );
	edac_mc_handle_error(type, mci, errcount, 0, 0, 0,
			     mci->fake_inject_layer[0],
			     mci->fake_inject_layer[1],
			     mci->fake_inject_layer[2],
			     "FAKE ERROR", "for EDAC testing only");

	return count;
}

static const struct file_operations debug_fake_inject_fops = {
	.open = simple_open,
	.write = edac_fake_inject_write,
	.llseek = generic_file_llseek,
};

int __init edac_debugfs_init(void)
{
	edac_debugfs = debugfs_create_dir("edac", NULL);
	if (IS_ERR(edac_debugfs)) {
		edac_debugfs = NULL;
		return -ENOMEM;
	}
	return 0;
}

void edac_debugfs_exit(void)
{
	debugfs_remove(edac_debugfs);
}

int edac_create_debugfs_nodes(struct mem_ctl_info *mci)
{
	struct dentry *d, *parent;
	char name[80];
	int i;

	if (!edac_debugfs)
		return -ENODEV;

	d = debugfs_create_dir(mci->dev.kobj.name, edac_debugfs);
	if (!d)
		return -ENOMEM;
	parent = d;

	for (i = 0; i < mci->n_layers; i++) {
		sprintf(name, "fake_inject_%s",
			     edac_layer_name[mci->layers[i].type]);
		d = debugfs_create_u8(name, S_IRUGO | S_IWUSR, parent,
				      &mci->fake_inject_layer[i]);
		if (!d)
			goto nomem;
	}

	d = debugfs_create_bool("fake_inject_ue", S_IRUGO | S_IWUSR, parent,
				&mci->fake_inject_ue);
	if (!d)
		goto nomem;

	d = debugfs_create_u16("fake_inject_count", S_IRUGO | S_IWUSR, parent,
				&mci->fake_inject_count);
	if (!d)
		goto nomem;

	d = debugfs_create_file("fake_inject", S_IWUSR, parent,
				&mci->dev,
				&debug_fake_inject_fops);
	if (!d)
		goto nomem;

	mci->debugfs = parent;
	return 0;
nomem:
	edac_debugfs_remove_recursive(mci->debugfs);
	return -ENOMEM;
}

/* Create a toplevel dir under EDAC's debugfs hierarchy */
struct dentry *edac_debugfs_create_dir(const char *dirname)
{
	if (!edac_debugfs)
		return NULL;

	return debugfs_create_dir(dirname, edac_debugfs);
}
EXPORT_SYMBOL_GPL(edac_debugfs_create_dir);

/* Create a toplevel dir under EDAC's debugfs hierarchy with parent @parent */
struct dentry *
edac_debugfs_create_dir_at(const char *dirname, struct dentry *parent)
{
	return debugfs_create_dir(dirname, parent);
}
EXPORT_SYMBOL_GPL(edac_debugfs_create_dir_at);

/*
 * Create a file under EDAC's hierarchy or a sub-hierarchy:
 *
 * @name: file name
 * @mode: file permissions
 * @parent: parent dentry. If NULL, it becomes the toplevel EDAC dir
 * @data: private data of caller
 * @fops: file operations of this file
 */
struct dentry *
edac_debugfs_create_file(const char *name, umode_t mode, struct dentry *parent,
			 void *data, const struct file_operations *fops)
{
	if (!parent)
		parent = edac_debugfs;

	return debugfs_create_file(name, mode, parent, data, fops);
}
EXPORT_SYMBOL_GPL(edac_debugfs_create_file);

/* Wrapper for debugfs_create_x8() */
struct dentry *edac_debugfs_create_x8(const char *name, umode_t mode,
				       struct dentry *parent, u8 *value)
{
	if (!parent)
		parent = edac_debugfs;

	return debugfs_create_x8(name, mode, parent, value);
}
EXPORT_SYMBOL_GPL(edac_debugfs_create_x8);

/* Wrapper for debugfs_create_x16() */
struct dentry *edac_debugfs_create_x16(const char *name, umode_t mode,
				       struct dentry *parent, u16 *value)
{
	if (!parent)
		parent = edac_debugfs;

	return debugfs_create_x16(name, mode, parent, value);
}
EXPORT_SYMBOL_GPL(edac_debugfs_create_x16);
         /*
 * Intel e752x Memory Controller kernel module
 * (C) 2004 Linux Networx (http://lnxi.com)
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * Implement support for the e7520, E7525, e7320 and i3100 memory controllers.
 *
 * Datasheets:
 *	http://www.intel.in/content/www/in/en/chipsets/e7525-memory-controller-hub-datasheet.html
 *	ftp://download.intel.com/design/intarch/datashts/31345803.pdf
 *
 * Written by Tom Zimmerman
 *
 * Contributors:
 * 	Thayne Harbaugh at realmsys.com (?)
 * 	Wang Zhenyu at intel.com
 * 	Dave Jiang at mvista.com
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/edac.h>
#include "edac_core.h"

#define E752X_REVISION	" Ver: 2.0.2"
#define EDAC_MOD_STR	"e752x_edac"

static int report_non_memory_errors;
static int force_function_unhide;
static int sysbus_parity = -1;

static struct edac_pci_ctl_info *e752x_pci;

#define e752x_printk(level, fmt, arg...) \
	edac_printk(level, "e752x", fmt, ##arg)

#define e752x_mc_printk(mci, level, fmt, arg...) \
	edac_mc_chipset_printk(mci, level, "e752x", fmt, ##arg)

#ifndef PCI_DEVICE_ID_INTEL_7520_0
#define PCI_DEVICE_ID_INTEL_7520_0      0x3590
#endif				/* PCI_DEVICE_ID_INTEL_7520_0      */

#ifndef PCI_DEVICE_ID_INTEL_7520_1_ERR
#define PCI_DEVICE_ID_INTEL_7520_1_ERR  0x3591
#endif				/* PCI_DEVICE_ID_INTEL_7520_1_ERR  */

#ifndef PCI_DEVICE_ID_INTEL_7525_0
#define PCI_DEVICE_ID_INTEL_7525_0      0x359E
#endif				/* PCI_DEVICE_ID_INTEL_7525_0      */

#ifndef PCI_DEVICE_ID_INTEL_7525_1_ERR
#define PCI_DEVICE_ID_INTEL_7525_1_ERR  0x3593
#endif				/* PCI_DEVICE_ID_INTEL_7525_1_ERR  */

#ifndef PCI_DEVICE_ID_INTEL_7320_0
#define PCI_DEVICE_ID_INTEL_7320_0	0x3592
#endif				/* PCI_DEVICE_ID_INTEL_7320_0 */

#ifndef PCI_DEVICE_ID_INTEL_7320_1_ERR
#define PCI_DEVICE_ID_INTEL_7320_1_ERR	0x3593
#endif				/* PCI_DEVICE_ID_INTEL_7320_1_ERR */

#ifndef PCI_DEVICE_ID_INTEL_3100_0
#define PCI_DEVICE_ID_INTEL_3100_0	0x35B0
#endif				/* PCI_DEVICE_ID_INTEL_3100_0 */

#ifndef PCI_DEVICE_ID_INTEL_3100_1_ERR
#define PCI_DEVICE_ID_INTEL_3100_1_ERR	0x35B1
#endif				/* PCI_DEVICE_ID_INTEL_3100_1_ERR */

#define E752X_NR_CSROWS		8	/* number of csrows */

/* E752X r