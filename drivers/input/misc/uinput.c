/*******************************************************************************
 * This file contains iSCSI extentions for RDMA (iSER) Verbs
 *
 * (c) Copyright 2013 Datera, Inc.
 *
 * Nicholas A. Bellinger <nab@linux-iscsi.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 ****************************************************************************/

#include <linux/string.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>
#include <target/target_core_base.h>
#include <target/target_core_fabric.h>
#include <target/iscsi/iscsi_transport.h>
#include <linux/semaphore.h>

#include "isert_proto.h"
#include "ib_isert.h"

#define	ISERT_MAX_CONN		8
#define ISER_MAX_RX_CQ_LEN	(ISERT_QP_MAX_RECV_DTOS * ISERT_MAX_CONN)
#define ISER_MAX_TX_CQ_LEN	(ISERT_QP_MAX_REQ_DTOS  * ISERT_MAX_CONN)
#define ISER_MAX_CQ_LEN		(ISER_MAX_RX_CQ_LEN + ISER_MAX_TX_CQ_LEN + \
				 ISERT_MAX_CONN)

static int isert_debug_level;
module_param_named(debug_level, isert_debug_level, int, 0644);
MODULE_PARM_DESC(debug_level, "Enable debug tracing if > 0 (default:0)");

static DEFINE_MUTEX(device_list_mutex);
static LIST_HEAD(device_list);
static struct workqueue_struct *isert_comp_wq;
static struct workqueue_struct *isert_release_wq;

static void
isert_unmap_cmd(struct isert_cmd *isert_cmd, struct isert_conn *isert_conn);
static int
isert_map_rdma(struct iscsi_conn *conn, struct iscsi_cmd *cmd,
	       struct isert_rdma_wr *wr);
static void
isert_unreg_rdma(struct isert_cmd *isert_cmd, struct isert_conn *isert_conn);
static int
isert_reg_rdma(struct iscsi_conn *conn, struct iscsi_cmd *cmd,
	       struct isert_rdma_wr *wr);
static int
isert_put_response(struct iscsi_conn *conn, struct iscsi_cmd *cmd);
static int
isert_rdma_post_recvl(struct isert_conn *isert_conn);
static int
isert_rdma_accept(struct isert_conn *isert_conn);
struct rdma_cm_id *isert_setup_id(struct isert_np *isert_np);

static void isert_release_work(struct work_struct *work);
static void isert_wait4flush(struct isert_conn *isert_conn);

static inline bool
isert_prot_cmd(struct isert_conn *conn, struct se_cmd *cmd)
{
	return (conn->pi_support &&
		cmd->prot_op != TARGET_PROT_NORMAL);
}


static void
isert_qp_event_callback(struct ib_event *e, void *context)
{
	struct isert_conn *isert_conn = context;

	isert_err("%s (%d): conn %p\n",
		  ib_event_msg(e->event), e->event, isert_conn);

	switch (e->event) {
	case IB_EVENT_COMM_EST:
		rdma_notify(isert_conn->cm_id, IB_EVENT_COMM_EST);
		break;
	case IB_EVENT_QP_LAST_WQE_REACHED:
		isert_warn("Reached TX IB_EVENT_QP_LAST_WQE_REACHED\n");
		break;
	default:
		break;
	}
}

static int
isert_query_device(struct ib_device *ib_dev, struct ib_device_attr *devattr)
{
	int ret;

	ret = ib_query_device(ib_dev, devattr);
	if (ret) {
		isert_err("ib_query_device() failed: %d\n", ret);
		return ret;
	}
	isert_dbg("devattr->max_sge: %d\n", devattr->max_sge);
	isert_dbg("devattr->max_sge_rd: %d\n", devattr->max_sge_rd);

	return 0;
}

static struct isert_comp *
isert_comp_get(struct isert_conn *isert_conn)
{
	struct isert_device *device = isert_conn->device;
	struct isert_comp *comp;
	int i, min = 0;

	mutex_lock(&device_list_mutex);
	for (i = 0; i < device->comps_used; i++)
		if (device->comps[i].active_qps <
		    device->comps[min].active_qps)
			min = i;
	comp = &device->comps[min];
	comp->active_qps++;
	mutex_unlock(&device_list_mutex);

	isert_info("conn %p, using comp %p min_index: %d\n",
		   isert_conn, comp, min);

	return comp;
}

static void
isert_comp_put(struct isert_comp *comp)
{
	mutex_lock(&device_list_mutex);
	comp->active_qps--;
	mutex_unlock(&device_list_mutex);
}

static struct ib_qp *
isert_create_qp(struct isert_conn *isert_conn,
		struct isert_comp *comp,
		struct rdma_cm_id *cma_id)
{
	struct isert_device *device = isert_conn->device;
	struct ib_qp_init_attr attr;
	int ret;

	memset(&attr, 0, sizeof(struct ib_qp_init_attr));
	attr.event_handler = isert_qp_event_callback;
	attr.qp_context = isert_conn;
	attr.send_cq = comp->cq;
	attr.recv_cq = comp->cq;
	attr.cap.max_send_wr = ISERT_QP_MAX_REQ_DTOS;
	attr.cap.max_recv_wr = ISERT_QP_MAX_RECV_DTOS + 1;
	attr.cap.max_send_sge = device->dev_attr.max_sge;
	isert_conn->max_sge = min(device->dev_attr.max_sge,
				  device->dev_attr.max_sge_rd);
	attr.cap.max_recv_sge = 1;
	attr.sq_sig_type = IB_SIGNAL_REQ_WR;
	attr.qp_type = IB_QPT_RC;
	if (device->pi_capable)
		attr.create_flags |= IB_QP_CREATE_SIGNATURE_EN;

	ret = rdma_create_qp(cma_id, device->pd, &attr);
	if (ret) {
		isert_err("rdma_create_qp failed for cma_id %d\n", ret);
		return ERR_PTR(ret);
	}

	return cma_id->qp;
}

static int
isert_conn_setup_qp(struct isert_conn *isert_conn, struct rdma_cm_id *cma_id)
{
	struct isert_comp *comp;
	int ret;

	comp = isert_comp_get(isert_conn);
	isert_conn->qp = isert_create_qp(isert_conn, comp, cma_id);
	if (IS_ERR(isert_conn->qp)) {
		ret = PTR_ERR(isert_conn->qp);
		goto err;
	}

	return 0;
err:
	isert_comp_put(comp);
	return ret;
}

static void
isert_cq_event_callback(struct ib_event *e, void *context)
{
	isert_dbg("event: %d\n", e->event);
}

static int
isert_alloc_rx_descriptors(struct isert_conn *isert_conn)
{
	struct isert_device *device = isert_conn->device;
	struct ib_device *ib_dev = device->ib_device;
	struct iser_rx_desc *rx_desc;
	struct ib_sge *rx_sg;
	u64 dma_addr;
	int i, j;

	isert_conn->rx_descs = kzalloc(ISERT_QP_MAX_RECV_DTOS *
				sizeof(struct iser_rx_desc), GFP_KERNEL);
	if (!isert_conn->rx_descs)
		goto fail;

	rx_desc = isert_conn->rx_descs;

	for (i = 0; i < ISERT_QP_MAX_RECV_DTOS; i++, rx_desc++)  {
		dma_addr = ib_dma_map_single(ib_dev, (void *)rx_desc,
					ISER_RX_PAYLOAD_SIZE, DMA_FROM_DEVICE);
		if (ib_dma_mapping_error(ib_dev, dma_addr))
			goto dma_map_fail;

		rx_desc->dma_addr = dma_addr;

		rx_sg = &rx_desc->rx_sg;
		rx_sg->addr = rx_desc->dma_addr;
		rx_sg->length = ISER_RX_PAYLOAD_SIZE;
		rx_sg->lkey = device->pd->local_dma_lkey;
	}

	return 0;

dma_map_fail:
	rx_desc = isert_conn->rx_descs;
	for (j = 0; j < i; j++, rx_desc++) {
		ib_dma_unmap_single(ib_dev, rx_desc->dma_addr,
				    ISER_RX_PAYLOAD_SIZE, DMA_FROM_DEVICE);
	}
	kfree(isert_conn->rx_descs);
	isert_conn->rx_descs = NULL;
fail:
	isert_err("conn %p failed to allocate rx descriptors\n", isert_conn);

	return -ENOMEM;
}

static void
isert_free_rx_descriptors(struct isert_conn *isert_conn)
{
	struct ib_device *ib_dev = isert_conn->device->ib_device;
	struct iser_rx_desc *rx_desc;
	int i;

	if (!isert_conn->rx_descs)
		return;

	rx_desc = isert_conn->rx_descs;
	for (i = 0; i < ISERT_QP_MAX_RECV_DTOS; i++, rx_desc++)  {
		ib_dma_unmap_single(ib_dev, rx_desc->dma_addr,
				    ISER_RX_PAYLOAD_SIZE, DMA_FROM_DEVICE);
	}

	kfree(isert_conn->rx_descs);
	isert_conn->rx_descs = NULL;
}

static void isert_cq_work(struct work_struct *);
static void isert_cq_callback(struct ib_cq *, void *);

static void
isert_free_comps(struct isert_device *device)
{
	int i;

	for (i = 0; i < device->comps_used; i++) {
		struct isert_comp *comp = &device->comps[i];

		if (comp->cq) {
			cancel_work_sync(&comp->work);
			ib_destroy_cq(comp->cq);
		}
	}
	kfree(device->comps);
}

static int
isert_alloc_comps(struct isert_device *device,
		  struct ib_device_attr *attr)
{
	int i, max_cqe, ret = 0;

	device->comps_used = min(ISERT_MAX_CQ, min_t(int, num_online_cpus(),
				 device->ib_device->num_comp_vectors));

	isert_info("Using %d CQs, %s supports %d vectors support "
		   "Fast registration %d pi_capable %d\n",
		   device->comps_used, device->ib_device->name,
		   device->ib_device->num_comp_vectors, device->use_fastreg,
		   device->pi_capable);

	device->comps = kcalloc(device->comps_used, sizeof(struct isert_comp),
				GFP_KERNEL);
	if (!device->comps) {
		isert_err("Unable to allocate completion contexts\n");
		return -ENOMEM;
	}

	max_cqe = min(ISER_MAX_CQ_LEN, attr->max_cqe);

	for (i = 0; i < device->comps_used; i++) {
		struct ib_cq_init_attr cq_attr = {};
		struct isert_comp *comp = &device->comps[i];

		comp->device = device;
		INIT_WORK(&comp->work, isert_cq_work);
		cq_attr.cqe = max_cqe;
		cq_attr.comp_vector = i;
		comp->cq = ib_create_cq(device->ib_device,
					isert_cq_callback,
					isert_cq_event_callback,
					(void *)comp,
					&cq_attr);
		if (IS_ERR(comp->cq)) {
			isert_err("Unable to allocate cq\n");
			ret = PTR_ERR(comp->cq);
			comp->cq = NULL;
			goto out_cq;
		}

		ret = ib_req_notify_cq(comp->cq, IB_CQ_NEXT_COMP);
		if (ret)
			goto out_cq;
	}

	return 0;
out_cq:
	isert_free_comps(device);
	return ret;
}

static int
isert_create_device_ib_res(struct isert_device *device)
{
	struct ib_device_attr *dev_attr;
	int ret;

	dev_attr = &device->dev_attr;
	ret = isert_query_device(device->ib_device, dev_attr);
	if (ret)
		return ret;

	/* asign function handlers */
	if (dev_attr->device_cap_flags & IB_DEVICE_MEM_MGT_EXTENSIONS &&
	    dev_attr->device_cap_flags & IB_DEVICE_SIGNATURE_HANDOVER) {
		device->use_fastreg = 1;
		device->reg_rdma_mem = isert_reg_rdma;
		device->unreg_rdma_mem = isert_unreg_rdma;
	} else {
		device->use_fastreg = 0;
		device->reg_rdma_mem = isert_map_rdma;
		device->unreg_rdma_mem = isert_unmap_cmd;
	}

	ret = isert_alloc_comps(device, dev_attr);
	if (ret)
		return ret;

	device->pd = ib_alloc_pd(device->ib_device);
	if (IS_ERR(device->pd)) {
		ret = PTR_ERR(device->pd);
		isert_err("failed to allocate pd, device %p, ret=%d\n",
			  device, ret);
		goto out_cq;
	}

	/* Check signature cap */
	device->pi_capable = dev_attr->device_cap_flags &
			     IB_DEVICE_SIGNATURE_HANDOVER ? true : false;

	return 0;

out_cq:
	isert_free_comps(device);
	return ret;
}

static void
isert_free_device_ib_res(struct isert_device *device)
{
	isert_info("device %p\n", device);

	ib_dealloc_pd(device->pd);
	isert_free_comps(device);
}

static void
isert_device_put(struct isert_device *device)
{
	mutex_lock(&device_list_mutex);
	device->refcount--;
	isert_info("device %p refcount %d\n", device, device->refcount);
	if (!device->refcount) {
		isert_free_device_ib_res(device);
		list_del(&device->dev_node);
		kfree(device);
	}
	mutex_unlock(&device_list_mutex);
}

static struct isert_device *
isert_device_get(struct rdma_cm_id *cma_id)
{
	struct isert_device *device;
	int ret;

	mutex_lock(&device_list_mutex);
	list_for_each_entry(device, &device_list, dev_node) {
		if (device->ib_device->node_guid == cma_id->device->node_guid) {
			device->refcount++;
			isert_info("Found iser device %p refcount %d\n",
				   device, device->refcount);
			mutex_unlock(&device_list_mutex);
			return device;
		}
	}

	device = kzalloc(sizeof(struct isert_device), GFP_KERNEL);
	if (!device) {
		mutex_unlock(&device_list_mutex);
		return ERR_PTR(-ENOMEM);
	}

	INIT_LIST_HEAD(&device->dev_node);

	device->ib_device = cma_id->device;
	ret = isert_create_device_ib_res(device);
	if (ret) {
		kfree(device);
		mutex_unlock(&device_list_mutex);
		return ERR_PTR(ret);
	}

	device->refcount++;
	list_add_tail(&device->dev_node, &device_list);
	isert_info("Created a new iser device %p refcount %d\n",
		   device, device->refcount);
	mutex_unlock(&device_list_mutex);

	return device;
}

static void
isert_conn_free_fastreg_pool(struct isert_conn *isert_conn)
{
	struct fast_reg_descriptor *fr_desc, *tmp;
	int i = 0;

	if (list_empty(&isert_conn->fr_pool))
		return;

	isert_info("Freeing conn %p fastreg pool", isert_conn);

	list_for_each_entry_safe(fr_desc, tmp,
				 &isert_conn->fr_pool, list) {
		list_del(&fr_desc->list);
		ib_dereg_mr(fr_desc->data_mr);
		if (fr_desc->pi_ctx) {
			ib_dereg_mr(fr_desc->pi_ctx->prot_mr);
			ib_dereg_mr(fr_desc->pi_ctx->sig_mr);
			kfree(fr_desc->pi_ctx);
		}
		kfree(fr_desc);
		++i;
	}

	if (i < isert_conn->fr_pool_size)
		isert_warn("Pool still has %d regions registered\n",
			isert_conn->fr_pool_size - i);
}

static int
isert_create_pi_ctx(struct fast_reg_descriptor *desc,
		    struct ib_device *device,
		    struct ib_pd *pd)
{
	struct pi_context *pi_ctx;
	int ret;

	pi_ctx = kzalloc(sizeof(*desc->pi_ctx), GFP_KERNEL);
	if (!pi_ctx) {
		isert_err("Failed to allocate pi context\n");
		return -ENOMEM;
	}

	pi_ctx->prot_mr = ib_alloc_mr(pd, IB_MR_TYPE_MEM_REG,
				      ISCSI_ISER_SG_TABLESIZE);
	if (IS_ERR(pi_ctx->prot_mr)) {
		isert_err("Failed to allocate prot frmr err=%ld\n",
			  PTR_ERR(pi_ctx->prot_mr));
		ret = PTR_ERR(pi_ctx->prot_mr);
		goto err_pi_ctx;
	}
	desc->ind |= ISERT_PROT_KEY_VALID;

	pi_ctx->sig_mr = ib_alloc_mr(pd, IB_MR_TYPE_SIGNATURE, 2);
	if (IS_ERR(pi_ctx->sig_mr)) {
		isert_err("Failed to allocate signature enabled mr err=%ld\n",
			  PTR_ERR(pi_ctx->sig_mr));
		ret = PTR_ERR(pi_ctx->sig_mr);
		goto err_prot_mr;
	}

	desc->pi_ctx = pi_ctx;
	desc->ind |= ISERT_SIG_KEY_VALID;
	desc->ind &= ~ISERT_PROTECTED;

	return 0;

err_prot_mr:
	ib_dereg_mr(pi_ctx->prot_mr);
err_pi_ctx:
	kfree(pi_ctx);

	return ret;
}

static int
isert_create_fr_desc(struct ib_device *ib_device, struct ib_pd *pd,
		     struct fast_reg_descriptor *fr_desc)
{
	fr_desc->data_mr = ib_alloc_mr(pd, IB_MR_TYPE_MEM_REG,
				       ISCSI_ISER_SG_TABLESIZE);
	if (IS_ERR(fr_desc->data_mr)) {
		isert_err("Failed to allocate data frmr err=%ld\n",
			  PTR_ERR(fr_desc->data_mr));
		return PTR_ERR(fr_desc->data_mr);
	}
	fr_desc->ind |= ISERT_DATA_KEY_VALID;

	isert_dbg("Created fr_desc %p\n", fr_desc);

	return 0;
}

static int
isert_conn_create_fastreg_pool(struct isert_conn *isert_conn)
{
	struct fast_reg_descriptor *fr_desc;
	struct isert_device *device = isert_conn->device;
	struct se_session *se_sess = isert_conn->conn->sess->se_sess;
	struct se_node_acl *se_nacl = se_sess->se_node_acl;
	int i, ret, tag_num;
	/*
	 * Setup the number of FRMRs based upon the number of tags
	 * available to session in iscsi_target_locate_portal().
	 */
	tag_num = max_t(u32, ISCSIT_MIN_TAGS, se_nacl->queue_depth);
	tag_num = (tag_num * 2) + ISCSIT_EXTRA_TAGS;

	isert_conn->fr_pool_size = 0;
	for (i = 0; i < tag_num; i++) {
		fr_desc = kzalloc(sizeof(*fr_desc), GFP_KERNEL);
		if (!fr_desc) {
			isert_err("Failed to allocate fast_reg descriptor\n");
			ret = -ENOMEM;
			goto err;
		}

		ret = isert_create_fr_desc(device->ib_device,
					   device->pd, fr_desc);
		if (ret) {
			isert_err("Failed to create fastreg descriptor err=%d\n",
			       ret);
			kfree(fr_desc);
			goto err;
		}

		list_add_tail(&fr_desc->list, &isert_conn->fr_pool);
		isert_conn->fr_pool_size++;
	}

	isert_dbg("Creating conn %p fastreg pool size=%d",
		 isert_conn, isert_conn->fr_pool_size);

	return 0;

err:
	isert_conn_free_fastreg_pool(isert_conn);
	return ret;
}

static void
isert_init_conn(struct isert_conn *isert_conn)
{
	isert_conn->state = ISER_CONN_INIT;
	INIT_LIST_HEAD(&isert_conn->node);
	init_completion(&isert_conn->login_comp);
	init_completion(&isert_conn->login_req_comp);
	init_completion(&isert_conn->wait);
	kref_init(&isert_conn->kref);
	mutex_init(&isert_conn->mutex);
	spin_lock_init(&isert_conn->pool_lock);
	INIT_LIST_HEAD(&isert_conn->fr_pool);
	INIT_WORK(&isert_conn->release_work, isert_release_work);
}

static void
isert_free_login_buf(struct isert_conn *isert_conn)
{
	struct ib_device *ib_dev = isert_conn->device->ib_device;

	ib_dma_unmap_single(ib_dev, isert_conn->login_rsp_dma,
			    ISER_RX_LOGIN_SIZE, DMA_TO_DEVICE);
	ib_dma_unmap_single(ib_dev, isert_conn->login_req_dma,
			    ISCSI_DEF_MAX_RECV_SEG_LEN,
			    DMA_FROM_DEVICE);
	kfree(isert_conn->login_buf);
}

static int
isert_alloc_login_buf(struct isert_conn *isert_conn,
		      struct ib_device *ib_dev)
{
	int ret;

	isert_conn->login_buf = kzalloc(ISCSI_DEF_MAX_RECV_SEG_LEN +
					ISER_RX_LOGIN_SIZE, GFP_KERNEL);
	if (!isert_conn->login_buf) {
		isert_err("Unable to allocate isert_conn->login_buf\n");
		return -ENOMEM;
	}

	isert_conn->login_req_buf = isert_conn->login_buf;
	isert_conn->login_rsp_buf = isert_conn->login_buf +
				    ISCSI_DEF_MAX_RECV_SEG_LEN;

	isert_dbg("Set login_buf: %p login_req_buf: %p login_rsp_buf: %p\n",
		 isert_conn->login_buf, isert_conn->login_req_buf,
		 isert_conn->login_rsp_buf);

	isert_conn->login_req_dma = ib_dma_map_single(ib_dev,
				(void *)isert_conn->login_req_buf,
				ISCSI_DEF_MAX_RECV_SEG_LEN, DMA_FROM_DEVICE);

	ret = ib_dma_mapping_error(ib_dev, isert_conn->login_req_dma);
	if (ret) {
		isert_err("login_req_dma mapping error: %d\n", ret);
		isert_conn->login_req_dma = 0;
		goto out_login_buf;
	}

	isert_conn->login_rsp_dma = ib_dma_map_single(ib_dev,
					(void *)isert_conn->login_rsp_buf,
					ISER_RX_LOGIN_SIZE, DMA_TO_DEVICE);

	ret = ib_dma_mapping_error(ib_dev, isert_conn->login_rsp_dma);
	if (ret) {
		isert_err("login_rsp_dma mapping error: %d\n", ret);
		isert_conn->login_rsp_dma = 0;
		goto out_req_dma_map;
	}

	return 0;

out_req_dma_map:
	ib_dma_unmap_single(ib_dev, isert_conn->login_req_dma,
			    ISCSI_DEF_MAX_RECV_SEG_LEN, DMA_FROM_DEVICE);
out_login_buf:
	kfree(isert_conn->login_buf);
	return ret;
}

static int
isert_connect_request(struct rdma_cm_id *cma_id, struct rdma_cm_event *event)
{
	struct isert_np *isert_np = cma_id->context;
	struct iscsi_np *np = isert_np->np;
	struct isert_conn *isert_conn;
	struct isert_device *device;
	int ret = 0;

	spin_lock_bh(&np->np_thread_lock);
	if (!np->enabled) {
		spin_unlock_bh(&np->np_thread_lock);
		isert_dbg("iscsi_np is not enabled, reject connect request\n");
		return rdma_reject(cma_id, NULL, 0);
	}
	spin_unlock_bh(&np->np_thread_lock);

	isert_dbg("cma_id: %p, portal: %p\n",
		 cma_id, cma_id->context);

	isert_conn = kzalloc(sizeof(struct isert_conn), GFP_KERNEL);
	if (!isert_conn)
		return -ENOMEM;

	isert_init_conn(isert_conn);
	isert_conn->cm_id = cma_id;

	ret = isert_alloc_login_buf(isert_conn, cma_id->device);
	if (ret)
		goto out;

	device = isert_device_get(cma_id);
	if (IS_ERR(device)) {
		ret = PTR_ERR(device);
		goto out_rsp_dma_map;
	}
	isert_conn->device = device;

	/* Set max inflight RDMA READ requests */
	isert_conn->initiator_depth = min_t(u8,
				event->param.conn.initiator_depth,
				device->dev_attr.max_qp_init_rd_atom);
	isert_dbg("Using initiator_depth: %u\n", isert_conn->initiator_depth);

	ret = isert_conn_setup_qp(isert_conn, cma_id);
	if (ret)
		goto out_conn_dev;

	ret = isert_rdma_post_recvl(isert_conn);
	if (ret)
		goto out_conn_dev;

	ret = isert_rdma_accept(isert_conn);
	if (ret)
		goto out_conn_dev;

	mutex_lock(&isert_np->mutex);
	list_add_tail(&isert_conn->node, &isert_np->accepted);
	mutex_unlock(&isert_np->mutex);

	return 0;

out_conn_dev:
	isert_device_put(device);
out_rsp_dma_map:
	isert_free_login_buf(isert_conn);
out:
	kfree(isert_conn);
	rdma_reject(cma_id, NULL, 0);
	return ret;
}

static void
isert_connect_release(struct isert_conn *isert_conn)
{
	struct isert_device *device = isert_conn->device;

	isert_dbg("conn %p\n", isert_conn);

	BUG_ON(!device);

	if (device->use_fastreg)
		isert_conn_free_fastreg_pool(isert_conn);

	isert_free_rx_descriptors(isert_conn);
	if (isert_conn->cm_id)
		rdma_destroy_id(isert_conn->cm_id);

	if (isert_conn->qp) {
		struct isert_comp *comp = isert_conn->qp->recv_cq->cq_context;

		isert_comp_put(comp);
		ib_destroy_qp(isert_conn->qp);
	}

	if (isert_conn->login_buf)
		isert_free_login_buf(isert_conn);

	isert_device_put(device);

	kfree(isert_conn);
}

static void
isert_connected_handler(struct rdma_cm_id *cma_id)
{
	struct isert_conn *isert_conn = cma_id->qp->qp_context;
	struct isert_np *isert_np = cma_id->context;

	isert_info("conn %p\n", isert_conn);

	mutex_lock(&isert_conn->mutex);
	isert_conn->state = ISER_CONN_UP;
	kref_get(&isert_conn->kref);
	mutex_unlock(&isert_conn->mutex);

	mutex_lock(&isert_np->mutex);
	list_move_tail(&isert_conn->node, &isert_np->pending);
	mutex_unlock(&isert_np->mutex);

	isert_info("np %p: Allow accept_np to continue\n", isert_np);
	up(&isert_np->sem);
}

static void
isert_release_kref(struct kref *kref)
{
	struct isert_conn *isert_conn = container_of(kref,
				struct isert_conn, kref);

	isert_info("conn %p final kref %s/%d\n", isert_conn, current->comm,
		   current->pid);

	isert_connect_release(isert_conn);
}

static void
isert_put_conn(struct isert_conn *isert_conn)
{
	kref_put(&isert_conn->kref, isert_release_kref);
}

static void
isert_handle_unbound_conn(struct isert_conn *isert_conn)
{
	struct isert_np *isert_np = isert_conn->cm_id->context;

	mutex_lock(&isert_np->mutex);
	if (!list_empty(&isert_conn->node)) {
		/*
		 * This means iscsi doesn't know this connection
		 * so schedule a cleanup ourselves
		 */
		list_del_init(&isert_conn->node);
		isert_put_conn(isert_conn);
		complete(&isert_conn->wait);
		queue_work(isert_release_wq, &isert_conn->release_work);
	}
	mutex_unlock(&isert_np->mutex);
}

/**
 * isert_conn_terminate() - Initiate connection termination
 * @isert_conn: isert connection struct
 *
 * Notes:
 * In case the connection state is BOUND, move state
 * to TEMINATING and start teardown sequence (rdma_disconnect).
 * In case the connection state is UP, complete flush as well.
 *
 * This routine must be called with mutex held. Thus it is
 * safe to call multiple times.
 */
static void
isert_conn_terminate(struct isert_conn *isert_conn)
{
	int err;

	if (isert_conn->state >= ISER_CONN_TERMINATING)
		return;

	isert_info("Terminating conn %p state %d\n",
		   isert_conn, isert_conn->state);
	isert_conn->state = ISER_CONN_TERMINATING;
	err = rdma_disconnect(isert_conn->cm_id);
	if (err)
		isert_warn("Failed rdma_disconnect isert_conn %p\n",
			   isert_conn);

	isert_info("conn %p completing wait\n", isert_conn);
	complete(&isert_conn->wait);
}

static int
isert_np_cma_handler(struct isert_np *isert_np,
		     enum rdma_cm_event_type event)
{
	isert_dbg("%s (%d): isert np %p\n",
		  rdma_event_msg(event), event, isert_np);

	switch (event) {
	case RDMA_CM_EVENT_DEVICE_REMOVAL:
		isert_np->cm_id = NULL;
		break;
	case RDMA_CM_EVENT_ADDR_CHANGE:
		isert_np->cm_id = isert_setup_id(isert_np);
		if (IS_ERR(isert_np->cm_id)) {
			isert_err("isert np %p setup id failed: %ld\n",
				  isert_np, PTR_ERR(isert_np->cm_id));
			isert_np->cm_id = NULL;
		}
		break;
	default:
		isert_err("isert np %p Unexpected event %d\n",
			  isert_np, event);
	}

	return -1;
}

static int
isert_disconnected_handler(struct rdma_cm_id *cma_id,
			   enum rdma_cm_event_type event)
{
	struct isert_conn *isert_conn = cma_id->qp->qp_context;

	mutex_lock(&isert_conn->mutex);
	switch (isert_conn->state) {
	case ISER_CONN_TERMINATING:
		break;
	case ISER_CONN_UP:
		isert_conn_terminate(isert_conn);
		isert_wait4flush(isert_conn);
		isert_handle_unbound_conn(isert_conn);
		break;
	case ISER_CONN_BOUND:
	case ISER_CONN_FULL_FEATURE: /* FALLTHRU */
		iscsit_cause_