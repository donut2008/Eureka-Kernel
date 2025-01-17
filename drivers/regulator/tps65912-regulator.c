ADD);

	return 0;

	/* Error unwind stack */
err_on_attrib:
	kobject_put(&block->kobj);

err_out:
	return err;
}

/*
 * edac_device_delete_block(edac_dev,block);
 */
static void edac_device_delete_block(struct edac_device_ctl_info *edac_dev,
				struct edac_device_block *block)
{
	struct edac_dev_sysfs_block_attribute *sysfs_attrib;
	int i;

	/* if this block has 'attributes' then we need to iterate over the list
	 * and 'remove' the attributes on this block
	 */
	sysfs_attrib = block->block_attributes;
	if (sysfs_attrib && block->nr_attribs) {
		for (i = 0; i < block->nr_attribs; i++, sysfs_attrib++) {

			/* remove each block_attrib file */
			sysfs_remove_file(&block->kobj,
				(struct attribute *) sysfs_attrib);
		}
	}

	/* unregister this block's kobject, SEE:
	 *	edac_device_ctrl_block_release() callback operation
	 */
	kobject_put(&block->kobj);
}

/* instance ctor/dtor code */

/*
 * edac_device_create_instance
 *	create just one instance of an edac_device 'instance'
 */
static int edac_device_create_instance(struct edac_device_ctl_info *edac_dev,
				int idx)
{
	int i, j;
	int err;
	struct edac_device_instance *instance;
	struct kobject *main_kobj;

	instance = &edac_dev->instances[idx];

	/* Init the instance's kobject */
	memset(&instance->kobj, 0, sizeof(struct kobject));

	instance->ctl = edac_dev;

	/* bump the main kobject's reference count for this controller
	 * and this instance is dependent on the main
	 */
	main_kobj = kobject_get(&edac_dev->kobj);
	if (!main_kobj) {
		err = -ENODEV;
		goto err_out;
	}

	/* Formally register this instance's kobject under the edac_device */
	err = kobject_init_and_add(&instance->kobj, &ktype_instance_ctrl,
				   &edac_dev->kobj, "%s", instance->name);
	if (err != 0) {
		edac_dbg(2, "Failed to register instance '%s'\n",
			 instance->name);
		kobject_put(main_kobj);
		goto err_out;
	}

	edac_dbg(4, "now register '%d' blocks for instance %d\n",
		 instance->nr_blocks, idx);

	/* register all blocks of this instance */
	for (i = 0; i < instance->nr_blocks; i++) {
		err = edac_device_create_block(edac_dev, instance,
						&instance->blocks[i]);
		if (err) {
			/* If any fail, remove all previous ones */
			for (j = 0; j < i; j++)
				edac_device_delete_block(edac_dev,
							&instance->blocks[j]);
			goto err_release_instance_kobj;
		}
	}
	kobject_uevent(&instance->kobj, KOBJ_ADD);

	edac_dbg(4, "Registered instance %d '%s' kobject\n",
		 idx, instance->name);

	return 0;

	/* error unwind stack */
err_release_instance_kobj:
	kobject_put(&instance->kobj);

err_out:
	return err;
}

/*
 * edac_device_remove_instance
 *	remove an edac_device instance
 */
static void edac_device_delete_instance(struct edac_device_ctl_info *edac_dev,
					int idx)
{
	struct edac_device_instance *instance;
	int i;

	instance = &edac_dev->instances[idx];

	/* unregister all blocks in this instance */
	for (i = 0; i < instance->nr_blocks; i++)
		edac_device_delete_block(edac_dev, &instance->blocks[i]);

	/* unregister this instance's kobject, SEE:
	 *	edac_device_ctrl_instance_release() for callback operation
	 */
	kobject_put(&instance->kobj);
}

/*
 * edac_device_create_instances
 *	create the first level of 'instances' for this device
 *	(ie  'cache' might have 'cache0', 'cache1', 'cache2', etc
 */
static int edac_device_create_instances(struct edac_device_ctl_info *edac_dev)
{
	int i, j;
	int err;

	edac_dbg(0, "\n");

	/* iterate over creation of the instances */
	for (i = 0; i < edac_dev->nr_instances; i++) {
		err = edac_device_create_instance(edac_dev, i);
		if (err) {
			/* unwind previous instances on error */
			for (j = 0; j < i; j++)
				edac_device_delete_instance(edac_dev, j);
			return err;
		}
	}

	return 0;
}

/*
 * edac_device_delete_instances(edac_dev);
 *	unregister all the kobjects of the instances
 */
static void edac_device_delete_instances(struct edac_device_ctl_info *edac_dev)
{
	int i;

	/* iterate over creation of the instances */
	for (i = 0; i < edac_dev->nr_instances; i++)
		edac_device_delete_instance(edac_dev, i);
}

/* edac_dev sysfs ctor/dtor  code */

/*
 * edac_device_add_main_sysfs_attributes
 *	add some attributes to this instance's main kobject
 */
static int edac_device_add_main_sysfs_attributes(
			struct edac_device_ctl_info *edac_dev)
{
	struct edac_dev_sysfs_attribute *sysfs_attrib;
	int err = 0;

	sysfs_attrib = edac_dev->sysfs_attributes;
	if (sysfs_attrib) {
		/* iterate over the array and create an attribute for each
		 * entry in the list
		 */
		while (sysfs_attrib->attr.name != NULL) {
			err = sysfs_create_file(&edac_dev->kobj,
				(struct attribute*) sysfs_attrib);
			if (err)
				goto err_out;

			sysfs_attrib++;
		}
	}

err_out:
	return err;
}

/*
 * edac_device_remove_main_sysfs_attributes
 *	remove any attributes to this instance's main kobject
 */
static void edac_device_remove_main_sysfs_attributes(
			struct edac_device_ctl_info *edac_dev)
{
	struct edac_dev_sysfs_attribute *sysfs_attrib;

	/* if there are main attributes, defined, remove them. First,
	 * point to the start of the array and iterate over it
	 * removing each attribute listed from this device's instance's kobject
	 */
	sysfs_attrib = edac_dev->sysfs_attributes;
	if (sysfs_attrib) {
		while (sysfs_attrib->attr.name != NULL) {
			sysfs_remove_file(&edac_dev->kobj,
					(struct attribute *) sysfs_attrib);
			sysfs_attrib++;
		}
	}
}

/*
 * edac_device_create_sysfs() Constructor
 *
 * accept a created edac_device control structure
 * and 'export' it to sysfs. The 'main' kobj should already have been
 * created. 'instance' and 'block' kobjects should be registered
 * along with any 'block' attributes from the low driver. In addition,
 * the main attributes (if any) are connected to the main kobject of
 * the control structure.
 *
 * Return:
 *	0	Success
 *	!0	Failure
 */
int edac_device_create_sysfs(struct edac_device_ctl_info *edac_dev)
{
	int err;
	struct kobject *edac_kobj = &edac_dev->kobj;

	edac_dbg(0, "idx=%d\n", edac_dev->dev_idx);

	/*  go create any main attributes callers wants */
	err = edac_device_add_main_sysfs_attributes(edac_dev);
	if (err) {
		edac_dbg(0, "failed to add sysfs attribs\n");
		goto err_out;
	}

	/* create a symlink from the edac device
	 * to the platform 'device' being used for this
	 */
	err = sysfs_create_link(edac_kobj,
				&edac_dev->dev->kobj, EDAC_DEVICE_SYMLINK);
	if (err) {
		edac_dbg(0, "sysfs_create_link() returned err= %d\n", err);
		goto err_remove_main_attribs;
	}

	/* Create the first level instance directories
	 * In turn, the nested blocks beneath the instances will
	 * be registered as well
	 */
	err = edac_device_create_instances(edac_dev);
	if (err) {
		edac_dbg(0, "edac_device_create_instances() returned err= %d\n",
			 err);
		goto err_remove_link;
	}


	edac_dbg(4, "create-instances done, idx=%d\n", edac_dev->dev_idx);

	return 0;

	/* Error unwind stack */
err_remove_link:
	/* remove the sym link */
	sysfs_remove_link(&edac_dev->kobj, EDAC_DEVICE_SYMLINK);

err_remove_main_attribs:
	edac_device_remove_main_sysfs_attributes(edac_dev);

err_out:
	return err;
}

/*
 * edac_device_remove_sysfs() destructor
 *
 * given an edac_device struct, tear down the kobject resources
 */
void edac_device_remove_sysfs(struct edac_device_ctl_info *edac_dev)
{
	edac_dbg(0, "\n");

	/* remove any main attributes for this device */
	edac_device_remove_main_sysfs_attributes(edac_dev);

	/* remove the device sym link */
	sysfs_remove_link(&edac_dev->kobj, EDAC_DEVICE_SYMLINK);

	/* walk the instance/block kobject tree, deconstructing it */
	edac_device_delete_instances(edac_dev);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * edac_mc kernel module
 * (C) 2005, 2006 Linux Networx (http://lnxi.com)
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * Written by Thayne Harbaugh
 * Based on work by Dan Hollis <goemon at anime dot net> and others.
 *	http://www.anime.net/~goemon/linux-ecc/
 *
 * Modified by Dave Peterson and Doug Thompson
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/sysctl.h>
#include <linux/highmem.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/edac.h>
#include <linux/bitops.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include "edac_core.h"
#include "edac_module.h"
#include <ras/ras_event.h>

#ifdef CONFIG_EDAC_ATOMIC_SCRUB
#include <asm/edac.h>
#else
#define edac_atomic_scrub(va, size) do { } while (0)
#endif

/* lock to memory controller's control array */
static DEFINE_MUTEX(mem_ctls_mutex);
static LIST_HEAD(mc_devices);

/*
 * Used to lock EDAC MC to just one module, avoiding two drivers e. g.
 *	apei/ghes and i7core_edac to be used at the same time.
 */
static void const *edac_mc_owner;

static struct bus_type mc_bus[EDAC_MAX_MCS];

unsigned edac_dimm_info_location(struct dimm_info *dimm, char *buf,
			         unsigned len)
{
	struct mem_ctl_info *mci = dimm->mci;
	int i, n, count = 0;
	char *p = buf;

	for (i = 0; i < mci->n_layers; i++) {
		n = snprintf(p, len, "%s %d ",
			      edac_layer_name[mci->layers[i].type],
			      dimm->location[i]);
		p += n;
		len -= n;
		count += n;
		if (!len)
			break;
	}

	return count;
}

#ifdef CONFIG_EDAC_DEBUG

static void edac_mc_dump_channel(struct rank_info *chan)
{
	edac_dbg(4, "  channel->chan_idx = %d\n", chan->chan_idx);
	edac_dbg(4, "    channel = %p\n", chan);
	edac_dbg(4, "    channel->csrow = %p\n", chan->csrow);
	edac_dbg(4, "    channel->dimm = %p\n", chan->dimm);
}

static void edac_mc_dump_dimm(struct dimm_info *dimm, int number)
{
	char location[80];

	edac_dimm_info_location(dimm, location, sizeof(location));

	edac_dbg(4, "%s%i: %smapped as virtual row %d, chan %d\n",
		 dimm->mci->csbased ? "rank" : "dimm",
		 number, location, dimm->csrow, dimm->cschannel);
	edac_dbg(4, "  dimm = %p\n", dimm);
	edac_dbg(4, "  dimm->label = '%s'\n", dimm->label);
	edac_dbg(4, "  dimm->nr_pages = 0x%x\n", dimm->nr_pages);
	edac_dbg(4, "  dimm->grain = %d\n", dimm->grain);
	edac_dbg(4, "  dimm->nr_pages = 0x%x\n", dimm->nr_pages);
}

static void edac_mc_dump_csrow(struct csrow_info *csrow)
{
	edac_dbg(4, "csrow->csrow_idx = %d\n", csrow->csrow_idx);
	edac_dbg(4, "  csrow = %p\n", csrow);
	edac_dbg(4, "  csrow->first_page = 0x%lx\n", csrow->first_page);
	edac_dbg(4, "  csrow->last_page = 0x%lx\n", csrow->last_page);
	edac_dbg(4, "  csrow->page_mask = 0x%lx\n", csrow->page_mask);
	edac_dbg(4, "  csrow->nr_channels = %d\n", csrow->nr_channels);
	edac_dbg(4, "  csrow->channels = %p\n", csrow->channels);
	edac_dbg(4, "  csrow->mci = %p\n", csrow->mci);
}

static void edac_mc_dump_mci(struct mem_ctl_info *mci)
{
	edac_dbg(3, "\tmci = %p\n", mci);
	edac_dbg(3, "\tmci->mtype_cap = %lx\n", mci->mtype_cap);
	edac_dbg(3, "\tmci->edac_ctl_cap = %lx\n", mci->edac_ctl_cap);
	edac_dbg(3, "\tmci->edac_cap = %lx\n", mci->edac_cap);
	edac_dbg(4, "\tmci->edac_check = %p\n", mci->edac_check);
	edac_dbg(3, "\tmci->nr_csrows = %d, csrows = %p\n",
		 mci->nr_csrows, mci->csrows);
	edac_dbg(3, "\tmci->nr_dimms = %d, dimms = %p\n",
		 mci->tot_dimms, mci->dimms);
	edac_dbg(3, "\tdev = %p\n", mci->pdev);
	edac_dbg(3, "\tmod_name:ctl_name = %s:%s\n",
		 mci->mod_name, mci->ctl_name);
	edac_dbg(3, "\tpvt_info = %p\n\n", mci->pvt_info);
}

#endif				/* CONFIG_EDAC_DEBUG */

const char * const edac_mem_types[] = {
	[MEM_EMPTY]	= "Empty csrow",
	[MEM_RESERVED]	= "Reserved csrow type",
	[MEM_UNKNOWN]	= "Unknown csrow type",
	[MEM_FPM]	= "Fast page mode RAM",
	[MEM_EDO]	= "Extended data out RAM",
	[MEM_BEDO]	= "Burst Extended data out RAM",
	[MEM_SDR]	= "Single data rate SDRAM",
	[MEM_RDR]	= "Registered single data rate SDRAM",
	[MEM_DDR]	= "Double data rate SDRAM",
	[MEM_RDDR]	= "Registered Double data rate SDRAM",
	[MEM_RMBS]	= "Rambus DRAM",
	[MEM_DDR2]	= "Unbuffered DDR2 RAM",
	[MEM_FB_DDR2]	= "Fully buffered DDR2",
	[MEM_RDDR2]