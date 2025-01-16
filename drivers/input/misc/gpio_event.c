/*
 * Copyright (c) 2005 Cisco Systems.  All rights reserved.
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/parser.h>
#include <linux/random.h>
#include <linux/jiffies.h>
#include <rdma/ib_cache.h>

#include <linux/atomic.h>

#include <scsi/scsi.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_tcq.h>
#include <scsi/srp.h>
#include <scsi/scsi_transport_srp.h>

#include "ib_srp.h"

#define DRV_NAME	"ib_srp"
#define PFX		DRV_NAME ": "
#define DRV_VERSION	"2.0"
#define DRV_RELDATE	"July 26, 2015"

MODULE_AUTHOR("Roland Dreier");
MODULE_DESCRIPTION("InfiniBand SCSI RDMA Protocol initiator");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_INFO(release_date, DRV_RELDATE);

static unsigned int srp_sg_tablesize;
static unsigned int cmd_sg_entries;
static unsigned int indirect_sg_entries;
static bool allow_ext_sg;
static bool prefer_fr = true;
static bool register_always = true;
static int topspin_workarounds = 1;

module_param(srp_sg_tablesize, uint, 0444);
MODULE_PARM_DESC(srp_sg_tablesize, "Deprecated name for cmd_sg_entries");

module_param(cmd_sg_entries, uint, 0444);
MODULE_PARM_DESC(cmd_sg_entries,
		 "Default number of gather/scatter entries in the SRP command (default is 12, max 255)");

module_param(indirect_sg_entries, uint, 0444);
MODULE_PARM_DESC(indirect_sg_entries,
		 "Default max number of gather/scatter entries (default is 12, max is " __stringify(SCSI_MAX_SG_CHAIN_SEGMENTS) ")");

module_param(allow_ext_sg, bool, 0444);
MODULE_PARM_DESC(allow_ext_sg,
		  "Default behavior when there are more than cmd_sg_entries S/G entries after mapping; fails the request when false (default false)");

module_param(topspin_workarounds, int, 0444);
MODULE_PARM_DESC(topspin_workarounds,
		 "Enable workarounds for Topspin/Cisco SRP target bugs if != 0");

module_param(prefer_fr, bool, 0444);
MODULE_PARM_DESC(prefer_fr,
"Whether to use fast registration if both FMR and fast registration are supported");

module_param(register_always, bool, 0444);
MODULE_PARM_DESC(register_always,
		 "Use memory registration even for contiguous memory regions");

static const struct kernel_param_ops srp_tmo_ops;

static int srp_reconnect_delay = 10;
module_param_cb(reconnect_delay, &srp_tmo_ops, &srp_reconnect_delay,
		S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(reconnect_delay, "Time between successive reconnect attempts");

static int srp_fast_io_fail_tmo = 15;
module_param_cb(fast_io_fail_tmo, &srp_tmo_ops, &srp_fast_io_fail_tmo,
		S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fast_io_fail_tmo,
		 "Number of seconds between the observation of a transport"
		 " layer error and failing all I/O. \"off\" means that this"
		 " functionality is disabled.");

static int srp_dev_loss_tmo = 600;
module_param_cb(dev_loss_tmo, &srp_tmo_ops, &srp_dev_loss_tmo,
		S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_loss_tmo,
		 "Maximum number of seconds that the SRP transport should"
		 " insulate transport layer errors. After this time has been"
		 " exceeded the SCSI host is removed. Should be"
		 " between 1 and " __stringify(SCSI_DEVICE_BLOCK_MAX_TIMEOUT)
		 " if fast_io_fail_tmo has not been set. \"off\" means that"
		 " this functionality is disabled.");

static unsigned ch_count;
module_param(ch_count, uint, 0444);
MODULE_PARM_DESC(ch_count,
		 "Number of RDMA channels to use for communication with an SRP target. Using more than one channel improves performance if the HCA supports multiple completion vectors. The default value is the minimum of four times the number of online CPU sockets and the number of completion vectors supported by the HCA.");

static void srp_add_one(struct ib_device *device);
static void srp_remove_one(struct ib_device *device, void *client_data);
static void srp_recv_completion(struct ib_cq *cq, void *ch_ptr);
static void srp_send_completion(struct ib_cq *cq, void *ch_ptr);
static int srp_cm_handler(struct ib_cm_id *cm_id, struct ib_cm_event *event);

static struct scsi_transport_template *ib_srp_transport_template;
static struct workqueue_struct *srp_remove_wq;

static struct ib_client srp_client = {
	.name   = "srp",
	.add    = srp_add_one,
	.remove = srp_remove_one
};

static struct ib_sa_client srp_sa_client;

static int srp_tmo_get(char *buffer, const struct kernel_param *kp)
{
	int tmo = *(int *)kp->arg;

	if (tmo >= 0)
		return sprintf(buffer, "%d", tmo);
	else
		return spr