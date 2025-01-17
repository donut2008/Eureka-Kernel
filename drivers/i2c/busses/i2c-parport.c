s", i + 1);
		remove_file(dir, fname);
		if (dd->flags & QIB_HAS_QSFP) {
			sprintf(fname, "qsfp%d", i + 1);
			remove_file(dir, fname);
		}
	}
	remove_file(dir, "flash");
	mutex_unlock(&d_inode(dir)->i_mutex);
	ret = simple_rmdir(d_inode(root), dir);
	d_delete(dir);
	dput(dir);

bail:
	mutex_unlock(&d_inode(root)->i_mutex);
	dput(root);
	return ret;
}

/*
 * This fills everything in when the fs is mounted, to handle umount/mount
 * after device init.  The direct add_cntr_files() call handles adding
 * them from the init code, when the fs is already mounted.
 */
static int qibfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct qib_devdata *dd, *tmp;
	unsigned long flags;
	int ret;

	static struct tree_descr files[] = {
		[2] = {"driver_stats", &driver_ops[0], S_IRUGO},
		[3] = {"driver_stats_names", &driver_ops[1], S_IRUGO},
		{""},
	};

	ret = simple_fill_super(sb, QIBFS_MAGIC, files);
	if (ret) {
		pr_err("simple_fill_super failed: %d\n", ret);
		goto bail;
	}

	spin_lock_irqsave(&qib_devs_lock, flags);

	list_for_each_entry_safe(dd, tmp, &qib_dev_list, list) {
		spin_unlock_irqrestore(&qib_devs_lock, flags);
		ret = add_cntr_files(sb, dd);
		if (ret)
			goto bail;
		spin_lock_irqsave(&qib_devs_lock, flags);
	}

	spin_unlock_irqrestore(&qib_devs_lock, flags);

bail:
	return ret;
}

static struct dentry *qibfs_mount(struct file_system_type *fs_type, int flags,
			const char *dev_name, void *data)
{
	struct dentry *ret;

	ret = mount_single(fs_type, flags, data, qibfs_fill_super);
	if (!IS_ERR(ret))
		qib_super = ret->d_sb;
	return ret;
}

static void qibfs_kill_super(struct super_block *s)
{
	kill_litter_super(s);
	qib_super = NULL;
}

int qibfs_add(struct qib_devdata *dd)
{
	int ret;

	/*
	 * On first unit initialized, qib_super will not yet exist
	 * because nobody has yet tried to mount the filesystem, so
	 * we can't consider that to be an error; if an error occurs
	 * during the mount, that will get a complaint, so this is OK.
	 * add_cntr_files() for all units is done at mount from
	 * qibfs_fill_super(), so one way or another, everything works.
	 */
	if (qib_super == NULL)
		ret = 0;
	else
		ret = add_cntr_files(qib_super, dd);
	return ret;
}

int qibfs_remove(struct qib_devdata *dd)
{
	int ret = 0;

	if (qib_super)
		ret = remove_device_files(qib_super, dd);

	return ret;
}

static struct file_system_type qibfs_fs_type = {
	.owner =        THIS_MODULE,
	.name =         "ipathfs",
	.mount =        qibfs_mount,
	.kill_sb =      qibfs_kill_super,
};
MODULE_ALIAS_FS("ipathfs");

int __init qib_init_qibfs(void)
{
	return register_filesystem(&qibfs_fs_type);
}

int __exit qib_exit_qibfs(void)
{
	return unregister_filesystem(&qibfs_fs_type);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/printk.h>
#ifdef CONFIG_INFINIBAND_QIB_DCA
#include <linux/dca.h>
#endif

#include "qib.h"
#include "qib_common.h"
#include "qib_mad.h"
#ifdef CONFIG_DEBUG_FS
#include "qib_debugfs.h"
#include "qib_verbs.h"
#endif

#undef pr_fmt
#define pr_fmt(fmt) QIB_DRV_NAME ": " fmt

/*
 * min buffers we want to have per context, after driver
 */
#define QIB_MIN_USER_CTXT_BUFCNT 7

#define QLOGIC_IB_R_SOFTWARE_MASK 0xFF
#define QLOGIC_IB_R_SOFTWARE_SHIFT 24
#define QLOGIC_IB_R_EMULATOR_MASK (1ULL<<62)

/*
 * Number of ctxts we are configured to use (to allow for more pio
 * buffers per ctxt, etc.)  Zero means use chip value.
 */
ushort qib_cfgctxts;
module_param_named(cfgctxts, qib_cfgctxts, ushort, S_IRUGO);
MODULE_PARM_DESC(cfgctxts, "Set max number of contexts to use");

unsigned qib_numa_aware;
module_param_named(numa_aware, qib_numa_aware, uint, S_IRUGO);
MODULE_PARM_DESC(numa_aware,
	"0 -> PSM allocation close to HCA, 1 -> PSM allocation local to process");

/*
 * If set, do not write to any regs if avoidable, hack to allow
 * check for deranged default register values.
 */
ushort qib_mini_init;
module_param_named(mini_init, qib_mini_init, ushort, S_IRUGO);
MODULE_PARM_DESC(mini_init, "If set, do minimal diag init");

unsigned qib_n_krcv_queues;
module_param_named(krcvqs, qib_n_krcv_queues, uint, S_IRUGO);
MODULE_PARM_DESC(krcvqs, "number of kernel receive queues per IB port");

unsigned qib_cc_table_size;
module_param_named(cc_table_size, qib_cc_table_size, uint, S_IRUGO);
MODULE_PARM_DESC(cc_table_size, "Congestion control table entries 0 (CCA disabled - default), min = 128, max = 1984");

static void verify_interrupt(unsigned long);

static struct idr qib_unit_table;
u32 qib_cpulist_count;
unsigned long *qib_cpulist;

/* set number of contexts we'll actually use */
void qib_set_ctxtcnt(struct qib_devdata *dd)
{
	if (!qib_cfgctxts) {
		dd->cfgctxts = dd->first_user_ctxt + num_online_cpus();
		if (dd->cfgctxts > dd->ctxtcnt)
			dd->cfgctxts = dd->ctxtcnt;
	} else if (qib_cfgctxts < dd->num_pports)
		dd->cfgctxts = dd->ctxtcnt;
	else if (qib_cfgctxts <= dd->ctxtcnt)
		dd->cfgctxts = qib_cfgctxts;
	else
		dd->cfgctxts = dd->ctxtcnt;
	dd->freectxts = (dd->first_user_ctxt > dd->cfgctxts) ? 0 :
		dd->cfgctxts - dd->first_user_ctxt;
}

/*
 * Common code for creating the receive context array.
 */
int qib_create_ctxts(struct qib_devdata *dd)
{
	unsigned i;
	int local_node_id = pcibus_to_node(dd->pcidev->bus);

	if (local_node_id < 0)
		local_node_id = numa_node_id();
	dd->assigned_node_id = local_node_id;

	/*
	 * Allocate full ctxtcnt array, rather than just cfgctxts, because
	 * cleanup iterates across all possible ctxts.
	 */
	dd->rcd = kcalloc(dd->ctxtcnt, sizeof(*dd->rcd), GFP_KERNEL);
	if (!dd->rcd) {
		qib_dev_err(dd,
			"Unable to allocate ctxtdata array, failing\n");
		return -ENOMEM;
	}

	/* create (one or more) kctxt */
	for (i = 0; i < dd->first_user_ctxt; ++i) {
		struct qib_pportdata *ppd;
		struct qib_ctxtdata *rcd;

		if (dd->skip_kctxt_mask & (1 << i))
			continue;

		ppd = dd->pport + (i % dd->num_pports);

		rcd = qib_create_ctxtdata(ppd, i, dd->assigned_node_id);
		if (!rcd) {
			qib_dev_err(dd,
				"Unable to allocate ctxtdata for Kernel ctxt, failing\n");
			kfree(dd->rcd);
			dd->rcd = NULL;
			return -ENOMEM;
	