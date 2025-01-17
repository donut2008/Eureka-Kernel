{
		ch = &target->ch[i];
		srp_free_ch_ib(target, ch);
		srp_free_req_data(target, ch);
	}

	kfree(target->ch);
	goto out;
}

static DEVICE_ATTR(add_target, S_IWUSR, NULL, srp_create_target);

static ssize_t show_ibdev(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct srp_host *host = container_of(dev, struct srp_host, dev);

	return sprintf(buf, "%s\n", host->srp_dev->dev->name);
}

static DEVICE_ATTR(ibdev, S_IRUGO, show_ibdev, NULL);

static ssize_t show_port(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct srp_host *host = container_of(dev, struct srp_host, dev);

	return sprintf(buf, "%d\n", host->port);
}

static DEVICE_ATTR(port, S_IRUGO, show_port, NULL);

static struct srp_host *srp_add_port(struct srp_device *device, u8 port)
{
	struct srp_host *host;

	host = kzalloc(sizeof *host, GFP_KERNEL);
	if (!host)
		return NULL;

	INIT_LIST_HEAD(&host->target_list);
	spin_lock_init(&host->target_lock);
	init_completion(&host->released);
	mutex_init(&host->add_target_mutex);
	host->srp_dev = device;
	host->port = port;

	host->dev.class = &srp_class;
	host->dev.parent = device->dev->dma_device;
	dev_set_name(&host->dev, "srp-%s-%d", device->dev->name, port);

	if (device_register(&host->dev))
		goto free_host;
	if (device_create_file(&host->dev, &dev_attr_add_target))
		goto err_class;
	if (device_create_file(&host->dev, &dev_attr_ibdev))
		goto err_class;
	if (device_create_file(&host->dev, &dev_attr_port))
		goto err_class;

	return host;

err_class:
	device_unregister(&host->dev);

free_host:
	kfree(host);

	return NULL;
}

static void srp_add_one(struct ib_device *device)
{
	struct srp_device *srp_dev;
	struct ib_device_attr *dev_attr;
	struct srp_host *host;
	int mr_page_shift, p;
	u64 max_pages_per_mr;

	dev_attr = kmalloc(sizeof *dev_attr, GFP_KERNEL);
	if (!dev_attr)
		return;

	if (ib_query_device(device, dev_attr)) {
		pr_warn("Query device failed for %s\n", device->name);
		goto free_attr;
	}

	srp_dev = kmalloc(sizeof *srp_dev, GFP_KERNEL);
	if (!srp_dev)
		goto free_attr;

	srp_dev->has_fmr = (device->alloc_fmr && device->dealloc_fmr &&
			    device->map_phys_fmr && device->unmap_fmr);
	srp_dev->has_fr = (dev_attr->device_cap_flags &
			   IB_DEVICE_MEM_MGT_EXTENSIONS);
	if (!srp_dev->has_fmr && !srp_dev->has_fr)
		dev_warn(&device->dev, "neither FMR nor FR is supported\n");

	srp_dev->use_fast_reg = (srp_dev->has_fr &&
				 (!srp_dev->has_fmr || prefer_fr));
	srp_dev->use_fmr = !srp_dev->use_fast_reg && srp_dev->has_fmr;

	/*
	 * Use the smallest page size supported by the HCA, down to a
	 * minimum of 4096 bytes. We're unlikely to build large sglists
	 * out of smaller entries.
	 */
	mr_page_shift		= max(12, ffs(dev_attr->page_size_cap) - 1);
	srp_dev->mr_page_size	= 1 << mr_page_shift;
	srp_dev->mr_page_mask	= ~((u64) srp_dev->mr_page_size - 1);
	max_pages_per_mr	= dev_attr->max_mr_size;
	do_div(max_pages_per_mr, srp_dev->mr_page_size);
	srp_dev->max_pages_per_mr = min_t(u64, SRP_MAX_PAGES_PER_MR,
					  max_pages_per_mr);
	if (srp_dev->use_fast_reg) {
		srp_dev->max_pages_per_mr =
			min_t(u32, srp_dev->max_pages_per_mr,
			      dev_attr->max_fast_reg_page_list_len);
	}
	srp_dev->mr_max_size	= srp_dev->mr_page_size *
				   srp_dev->max_pages_per_mr;
	pr_debug("%s: mr_page_shift = %d, dev_attr->max_mr_size = %#llx, dev_attr->max_fast_reg_page_list_len = %u, max_pages_per_mr = %d, mr_max_size = %#x\n",
		 device->name, mr_page_shift, dev_attr->max_mr_size,
		 dev_attr->max_fast_reg_page_list_len,
		 srp_dev->max_pages_per_mr, srp_dev->mr_max_size);

	INIT_LIST_HEAD(&srp_dev->dev_list);

	srp_dev->dev = device;
	srp_dev->pd  = ib_alloc_pd(device);
	if (IS_ERR(srp_dev->pd))
		goto free_dev;

	if (!register_always || (!srp_dev->has_fmr && !srp_dev->has_fr)) {
		srp_dev->global_mr = ib_get_dma_mr(srp_dev->pd,
						   IB_ACCESS_LOCAL_WRITE |
						   IB_ACCESS_REMOTE_READ |
						   IB_ACCESS_REMOTE_WRITE);
		if (IS_ERR(srp_dev->global_mr))
			goto err_pd;
	} else {
		srp_dev->global_mr = NULL;
	}

	for (p = rdma_start_port(device); p <= rdma_end_port(device); ++p) {
		host = srp_add_port(srp_dev, p);
		if (host)
			list_add_tail(&host->list, &srp_dev->dev_list);
	}

	ib_set_client_data(device, &srp_client, srp_dev);

	goto free_attr;

err_pd:
	ib_dealloc_pd(srp_dev->pd);

free_dev:
	kfree(srp_dev);

free_attr:
	kfree(dev_attr);
}

static void srp_remove_one(struct ib_device *device, void *client_data)
{
	struct srp_device *srp_dev;
	struct srp_host *host, *tmp_host;
	struct srp_target_port *target;

	srp_dev = client_data;
	if (!srp_dev)
		return;

	list_for_each_entry_safe(host, tmp_host, &srp_dev->dev_list, list) {
		device_unregister(&host->dev);
		/*
		 * Wait for the sysfs entry to go away, so that no new
		 * target ports can be created.
		 */
		wait_for_completion(&host->released);

		/*
		 * Remove all target ports.
		 */
		spin_lock(&host->target_lock);
		list_for_each_entry(target, &host->target_list, list)
			srp_queue_remove_work(target);
		spin_unlock(&host->target_lock);

		/*
		 * srp_queue_remove_work() queues a call to
		 * srp_remove_target(). The latter function cancels
		 * target->tl_err_work so waiting for the remove works to
		 * finish is sufficient.
		 */
		flush_workqueue(srp_remove_wq);

		kfree(host);
	}

	if (srp_dev->global_mr)
		ib_dereg_mr(srp_dev->global_mr);
	ib_dealloc_pd(srp_dev->pd);

	kfree(srp_dev);
}

static struct srp_function_template ib_srp_transport_functions = {
	.has_rport_state	 = true,
	.reset_timer_if_blocked	 = true,
	.reconnect_delay	 = &srp_reconnect_delay,
	.fast_io_fail_tmo	 = &srp_fast_io_fail_tmo,
	.dev_loss_tmo		 = &srp_dev_loss_tmo,
	.reconnect		 = srp_rport_reconnect,
	.rport_delete		 = srp_rport_delete,
	.terminate_rport_io	 = srp_terminate_io,
};

static int __init srp_init_module(void)
{
	int ret;

	BUILD_BUG_ON(FIELD_SIZEOF(struct ib_wc, wr_id) < sizeof(void *));

	if (srp_sg_tablesize) {
		pr_warn("srp_sg_tablesize is deprecated, please use cmd_sg_entries\n");
		if (!cmd_sg_entries)
			cmd_sg_entries = srp_sg_tablesize;
	}

	if (!cmd_sg_entries)
		cmd_sg_entries = SRP_DEF_SG_TABLESIZE;

	if (cmd_sg_entries > 255) {
		pr_warn("Clamping cmd_sg_entries to 255\n");
		cmd_sg_entries = 255;
	}

	if (!indirect_sg_entries)
		indirect_sg_entries = cmd_sg_entries;
	else if (indirect_sg_entries < cmd_sg_entries) {
		pr_warn("Bumping up indirect_sg_entries to match cmd_sg_entries (%u)\n",
			cmd_sg_entries);
		indirect_sg_entries = cmd_sg_entries;
	}

	srp_remove_wq = create_workqueue("srp_remove");
	if (!srp_remove_wq) {
		ret = -ENOMEM;
		goto out;
	}

	ret = -ENOMEM;
	ib_srp_transport_template =
		srp_attach_transport(&ib_srp_transport_functions);
	if (!ib_srp_transport_template)
		goto destroy_wq;

	ret = class_register(&srp_class);
	if (ret) {
		pr_err("couldn't register class infiniband_srp\n");
		goto release_tr;
	}

	ib_sa_register_client(&srp_sa_client);

	ret = ib_register_client(&srp_client);
	if (ret) {
		pr_err("couldn't register IB client\n");
		goto unreg_sa;
	}

out:
	return ret;

unreg_sa:
	ib_sa_unregister_client(&srp_sa_client);
	class_unregister(&srp_class);

release_tr:
	srp_release_transport(ib_srp_transport_template);

destroy_wq:
	destroy_workqueue(srp_remove_wq);
	goto out;
}

static void __exit srp_cleanup_module(void)
{
	ib_unregister_client(&srp_client);
	ib_sa_unregister_client(&srp_sa_client);
	class_unregister(&srp_class);
	srp_release_transport(ib_srp_transport_template);
	destroy_workqueue(srp_remove_wq);
}

module_init(srp_init_module);
module_exit(srp_cleanup_module);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * Copyright (c) 2006 - 2009 Mellanox Technology Inc.  All rights reserved.
 * Copyright (C) 2008 - 2011 Bart Van Assche <bvanassche@acm.org>.
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
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <scsi/scsi_proto.h>
#include <scsi/scsi_tcq.h>
#include <target/target_core_base.h>
#include <target/target_core_fabric.h>
#include "ib_srpt.h"

/* Name of this kernel module. */
#define DRV_NAME		"ib_srpt"
#define DRV_VERSION		"2.0.0"
#define DRV_RELDATE		"2011-02-14"

#define SRPT_ID_STRING	"Linux SRP target"

#undef pr_fmt
#define pr_fmt(fmt) DRV_NAME " " fmt

MODULE_AUTHOR("Vu Pham and Bart Van Assche");
MODULE_DESCRIPTION("InfiniBand SCSI RDMA Protocol target "
		   "v" DRV_VERSION " (" DRV_RELDATE ")");
MODULE_LICENSE("Dual BSD/GPL");

/*
 * Global Variables
 */

static u64 srpt_service_guid;
static DEFINE_SPINLOCK(srpt_dev_lock);	/* Protects srpt_dev_list. */
static LIST_HEAD(srpt_dev_list);	/* List of srpt_device structures. */

static unsigned srp_max_req_size = DEFAULT_MAX_REQ_SIZE;
module_param(srp_max_req_size, int, 0444);
MODULE_PARM_DESC(srp_max_req_size,
		 "Maximum size of SRP request messages in bytes.");

static int srpt_srq_size = DEFAULT_SRPT_SRQ_SIZE;
module_param(srpt_srq_size, int, 0444);
MODULE_PARM_DESC(srpt_srq_size,
		 "Shared receive queue (SRQ) size.");

static int srpt_get_u64_x(char *buffer, struct kernel_param *kp)
{
	return sprintf(buffer, "0x%016llx", *(u64 *)kp->arg);
}
module_param_call(srpt_service_guid, NULL, srpt_get_u64_x, &srpt_service_guid,
		  0444);
MODULE_PARM_DESC(srpt_service_guid,
		 "Using this value for ioc_guid, id_ext, and cm_listen_id"
		 " instead of using the node_guid of the first HCA.");

static struct ib_client srpt_client;
static void srpt_release_channel(struct srpt_rdma_ch *ch);
static int srpt_queue_status(struct se_cmd *cmd);

/**
 * opposite_dma_dir() - Swap DMA_TO_DEVICE and DMA_FROM_DEVICE.
 */
static inline
enum dma_data_direction opposite_dma_dir(enum dma_data_direction dir)
{
	switch (dir) {
	case DMA_TO_DEVICE:	return DMA_FROM_DEVICE;
	case DMA_FROM_DEVICE:	return DMA_TO_DEVICE;
	default:		return dir;
	}
}

/**
 * srpt_sdev_name() - Return the name associated with the HCA.
 *
 * Examples are ib0, ib1, ...
 */
static inline const char *srpt_sdev_name(struct srpt_device *sdev)
{
	return sdev->device->name;
}

static enum rdma_ch_state srpt_get_ch_state(struct srpt_rdma_ch *ch)
{
	unsigned long flags;
	enum rdma_ch_state state;

	spin_lock_irqsave(&ch->spinlock, flags);
	state = ch->state;
	spin_unlock_irqrestore(&ch->spinlock, flags);
	return state;
}

static enum rdma_ch_state
srpt_set_ch_state(struct srpt_rdma_ch *ch, enum rdma_ch_state new_state)
{
	unsigned long flags;
	enum rdma_ch_state prev;

	spin_lock_irqsave(&ch->spinlock, flags);
	prev = ch->state;
	ch->state = new_state;
	spin_unlock_irqrestore(&ch->spinlock, flags);
	return prev;
}

/**
 * srpt_test_and_set_ch_state() - Test and set the channel state.
 *
 * Returns 