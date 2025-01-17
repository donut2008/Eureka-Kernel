er->created   = mcast->created;
			iter->queuelen  = skb_queue_len(&mcast->pkt_queue);
			iter->complete  = !!mcast->ah;
			iter->send_only = !!(mcast->flags & (1 << IPOIB_MCAST_FLAG_SENDONLY));

			ret = 0;

			break;
		}

		n = rb_next(n);
	}

	spin_unlock_irq(&priv->lock);

	return ret;
}

void ipoib_mcast_iter_read(struct ipoib_mcast_iter *iter,
			   union ib_gid *mgid,
			   unsigned long *created,
			   unsigned int *queuelen,
			   unsigned int *complete,
			   unsigned int *send_only)
{
	*mgid      = iter->mgid;
	*created   = iter->created;
	*queuelen  = iter->queuelen;
	*complete  = iter->complete;
	*send_only = iter->send_only;
}

#endif /* CONFIG_INFINIBAND_IPOIB_DEBUG */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright (c) 2012 Mellanox Technologies. -  All rights reserved.
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

#include <linux/netdevice.h>
#include <linux/if_arp.h>      /* For ARPHRD_xxx */
#include <linux/module.h>
#include <net/rtnetlink.h>
#include "ipoib.h"

static const struct nla_policy ipoib_policy[IFLA_IPOIB_MAX + 1] = {
	[IFLA_IPOIB_PKEY]	= { .type = NLA_U16 },
	[IFLA_IPOIB_MODE]	= { .type = NLA_U16 },
	[IFLA_IPOIB_UMCAST]	= { .type = NLA_U16 },
};

static unsigned int ipoib_get_max_num_queues(void)
{
	return min_t(unsigned int, num_possible_cpus(), 128);
}

static int ipoib_fill_info(struct sk_buff *skb, const struct net_device *dev)
{
	struct ipoib_dev_priv *priv = netdev_priv(dev);
	u16 val;

	if (nla_put_u16(skb, IFLA_IPOIB_PKEY, priv->pkey))
		goto nla_put_failure;

	val = test_bit(IPOIB_FLAG_ADMIN_CM, &priv->flags);
	if (nla_put_u16(skb, IFLA_IPOIB_MODE, val))
		goto nla_put_failure;

	val = test_bit(IPOIB_FLAG_UMCAST, &priv->flags);
	if (nla_put_u16(skb, IFLA_IPOIB_UMCAST, val))
		goto nla_put_failure;

	return 0;

nla_put_failure:
	return -EMSGSIZE;
}

static int ipoib_changelink(struct net_device *dev,
			    struct nlattr *tb[], struct nlattr *data[])
{
	u16 mode, umcast;
	int ret = 0;

	if (data[IFLA_IPOIB_MODE]) {
		mode  = nla_get_u16(data[IFLA_IPOIB_MODE]);
		if (mode == IPOIB_MODE_DATAGRAM)
			ret = ipoib_set_mode(dev, "datagram\n");
		else if (mode == IPOIB_MODE_CONNECTED)
			ret = ipoib_set_mode(dev, "connected\n");
		else
			ret = -EINVAL;

		if (ret < 0)
			goto out_err;
	}

	if (data[IFLA_IPOIB_UMCAST]) {
		umcast = nla_get_u16(data[IFLA_IPOIB_UMCAST]);
		ipoib_set_umcast(dev, umcast);
	}

out_err:
	return ret;
}

static int ipoib_new_child_link(struct net *src_net, struct net_device *dev,
			       struct nlattr *tb[], struct nlattr *data[])
{
	struct net_device *pdev;
	struct ipoib_dev_priv *ppriv;
	u16 child_pkey;
	int err;

	if (!tb[IFLA_LINK])
		return -EINVAL;

	pdev = __dev_get_by_index(src_net, nla_get_u32(tb[IFLA_LINK]));
	if (!pdev || pdev->type != ARPHRD_INFINIBAND)
		return -ENODEV;

	ppriv = netdev_priv(pdev);

	if (test_bit(IPOIB_FLAG_SUBINTERFACE, &ppriv->flags)) {
		ipoib_warn(ppriv, "child creation disallowed for child devices\n");
		return -EINVAL;
	}

	if (!data || !data[IFLA_IPOIB_PKEY]) {
		ipoib_dbg(ppriv, "no pkey specified, using parent pkey\n");
		child_pkey  = ppriv->pkey;
	} else
		child_pkey  = nla_get_u16(data[IFLA_IPOIB_PKEY]);

	if (child_pkey == 0 || child_pkey == 0x8000)
		return -EINVAL;

	/*
	 * Set the full membership bit, so that we join the right
	 * broadcast group, etc.
	 */
	child_pkey |= 0x8000;

	err = __ipoib_vlan_add(ppriv, netdev_priv(dev), child_pkey, IPOIB_RTNL_CHILD);

	if (!err && data)
		err = ipoib_changelink(dev, tb, data);
	return err;
}

static void ipoib_unregister_child_dev(struct net_device *dev, struct list_head *head)
{
	struct ipoib_dev_priv *priv, *ppriv;

	priv = netdev_priv(dev);
	ppriv = netdev_priv(priv->parent);

	down_write(&ppriv->vlan_rwsem);
	unregister_netdevice_queue(dev, head);
	list_del(&priv->list);
	up_write(&ppriv->vlan_rwsem);
}

static size_t ipoib_get_size(const struct net_device *dev)
{
	return nla_total_size(2) +	/* IFLA_IPOIB_PKEY   */
		nla_total_size(2) +	/* IFLA_IPOIB_MODE   */
		nla_total_size(2);	/* IFLA_IPOIB_UMCAST */
}

static struct rtnl_link_ops ipoib_link_ops __read_mostly = {
	.kind		= "ipoib",
	.maxtype	= IFLA_IPOIB_MAX,
	.policy		= ipoib_policy,
	.priv_size	= sizeof(struct ipoib_dev_priv),
	.setup		= ipoib_setup,
	.newlink	= ipoib_new_child_link,
	.changelink	= ipoib_changelink,
	.dellink	= ipoib_unregister_child_dev,
	.get_size	= ipoib_get_size,
	.fill_info	= ipoib_fill_info,
	.get_num_rx_queues = ipoib_get_max_num_queues,
	.get_num_tx_queues = ipoib_get_max_num_queues,
};

int __init ipoib_netlink_init(void)
{
	return rtnl_link_register(&ipoib_link_ops);
}

void __exit ipoib_netlink_fini(void)
{
	rtnl_link_unregister(&ipoib_link_ops);
}

MODULE_ALIAS_RTNL_LINK("ipoib");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Copyright (c) 2004, 2005 Topspin Communications.  All rights reserved.
 * Copyright (c) 2005 Mellanox Technologies. All rights reserved.
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

#include <linux/slab.h>

#include "ipoib.h"

int ipoib_mcast_attach(struct net_device *dev, u16 mlid, union ib_gid *mgid, int set_qkey)
{
	struct ipoib_dev_priv *priv = netdev_priv(dev);
	struct ib_qp_attr *qp_attr = NULL;
	int ret;
	u16 pkey_index;

	if (ib_find_pkey(priv->ca, priv->port, priv->pkey, &pkey_index)) {
		clear_bit(IPOIB_PKEY_ASSIGNED, &priv->flags);
		ret = -ENXIO;
		goto out;
	}
	set_bit(IPOIB_PKEY_ASSIGNED, &priv->flags);

	if (set_qkey) {
		ret = -ENOMEM;
		qp_attr = kmalloc(sizeof *qp_attr, GFP_KERNEL);
		if (!qp_attr)
			goto out;

		/* set correct QKey for QP */
		qp_attr->qkey = priv->qkey;
		ret = ib_modify_qp(priv->qp, qp_attr, IB_QP_QKEY);
		if (ret) {
			ipoib_warn(priv, "failed to modify QP, ret = %d\n", ret);
			goto out;
		}
	}

	/* attach QP to multicast group */
	ret = ib_attach_mcast(priv->qp, mgid, mlid);
	if (ret)
		ipoib_warn(priv, "failed to attach to multicast group, ret = %d\n", ret);

out:
	kfree(qp_attr);
	return ret;
}

int ipoib_init_qp(struct net_device *dev)
{
	struct ipoib_dev_priv *priv = netdev_priv(dev);
	int ret;
	struct ib_qp_attr qp_attr;
	int attr_mask;

	if (!test_bit(IPOIB_PKEY_ASSIGNED, &priv->flags))
		return -1;

	qp_attr.qp_state = IB_QPS_INIT;
	qp_attr.qkey = 0;
	qp_attr.port_num = priv->port;
	qp_attr.pkey_index = priv->pkey_index;
	attr_mask =
	    IB_QP_QKEY |
	    IB_QP_PORT |
	    IB_QP_PKEY_INDEX |
	    IB_QP_STATE;
	ret = ib_modify_qp(priv->qp, &qp_attr, attr_mask);
	if (ret) {
		ipoib_warn(priv, "failed to modify QP to init, ret = %d\n", ret);
		goto out_fail;
	}

	qp_attr.qp_state = IB_QPS_RTR;
	/* Can't set this in a INIT->RTR transition */
	attr_mask &= ~IB_QP_PORT;
	ret = ib_modify_qp(priv->qp, &qp_attr, attr_mask);
	if (ret) {
		ipoib_warn(priv, "failed to modify QP to RTR, ret = %d\n", ret);
		goto out_fail;
	}

	qp_attr.qp_state = IB_QPS_RTS;
	qp_attr.sq_psn = 0;
	attr_mask |= IB_QP_SQ_PSN;
	attr_mask &= ~IB_QP_PKEY_INDEX;
	ret = ib_modify_qp(priv->qp, &qp_attr, attr_mask);
	if (ret) {
		ipoib_warn(priv, "failed to modify QP to RTS, ret = %d\n", ret);
		goto out_fail;
	}

	return 0;

out_fail:
	qp_attr.qp_state = IB_QPS_RESET;
	if (ib_modify_qp(priv->qp, &qp_attr, IB_QP_STATE))
		ipoib_warn(priv, "Failed to modify QP to RESET state\n");

	return ret;
}

int ipoib_transport_dev_init(struct net_device *dev, struct ib_device *ca)
{
	struct ipoib_dev_priv *priv = netdev_priv(dev);
	struct ib_qp_init_attr init_attr = {
		.cap = {
			.max_send_wr  = ipoib_sendq_size,
			.max_recv_wr  = ipoib_recvq_size,
			.max_send_sge = 1,
			.max_recv_sge = IPOIB_UD_RX_SG
		},
		.sq_sig_type = IB_SIGNAL_ALL_WR,
		.qp_type     = IB_QPT_UD
	};
	struct ib_cq_init_attr cq_attr = {};

	int ret, size;
	int i;

	priv->pd = ib_alloc_pd(priv->ca);
	if (IS_ERR(priv->pd)) {
		printk(KERN_WARNING "%s: failed to allocate PD\n", ca->name);
		return -ENODEV;
	}

	/*
	 * the various IPoIB tasks assume they will never race against
	 * themselves, so always use a single thread workqueue
	 */
	priv->wq = create_singlethread_workqueue("ipoib_wq");
	if (!priv->wq) {
		printk(KERN_WARNING "ipoib: failed to allocate device WQ\n");
		goto out_free_pd;
	}

	size = ipoib_recvq_size + 1;
	ret = ipoib_cm_dev_init(dev);
	if (!ret) {
		size += ipoib_sendq_size;
		if (ipoib_cm_has_srq(dev))
			size += ipoib_recvq_size + 1; /* 1 extra for rx_drain_qp */
		else
			size += ipoib_recvq_size * ipoib_max_conn_qp;
	} else
		if (ret != -ENOSYS)
			goto out_free_wq;

	cq_attr.cqe = size;
	priv->recv_cq = ib_create_cq(priv->ca, ipoib_ib_completion, NULL,
				     dev, &cq_attr);
	if (IS_ERR(priv->recv_cq)) {
		printk(KERN_WARNING "%s: failed to create receive CQ\n", ca->name);
		goto out_cm_dev_cleanup;
	}

	cq_attr.cqe = ipoib_sendq_size;
	priv->send_cq = ib_create_cq(priv->ca, ipoib_send_comp_handler, NULL,
				     dev, &cq_attr);
	if (IS_ERR(priv->send_cq)) {
		printk(KERN_WARNING "%s: failed to create send CQ\n", ca->name);
		goto out_free_recv_cq;
	}

	if (ib_req_notify_cq(priv->recv_cq, IB_CQ_NEXT_COMP))
		goto out_free_send_cq;

	init_attr.send_cq = priv->send_cq;
	init_attr.recv_cq = priv->recv_cq;

	if (priv->hca_caps & IB_DEVICE_UD_TSO)
		init_attr.create_flags |= IB_QP_CREATE_IPOIB_UD_LSO;

	if (priv->hca_caps & IB_DEVICE_BLOCK_MULTICAST_LOOPBACK)
		init_attr.create_flags |= IB_QP_CREATE_BLOCK_MULTICAST_LOOPBACK;

	if (priv->hca_caps & IB_DEVICE_MANAGED_FLOW_STEERING)
		init_attr.create_flags |= IB_QP_CREATE_NETIF_QP;

	if (dev->features & NETIF_F_SG)
		init_attr.cap.max_send_sge = MAX_SKB_FRAGS + 1;

	priv->qp = ib_create_qp(priv->pd, &init_attr);
	if (IS_ERR(priv->qp)) {
		printk(KERN_WARNING "%s: failed to create QP\n", ca->name);
		goto out_free_send_cq;
	}

	priv->dev->dev_addr[1] = (priv->qp->qp_num >> 16) & 0xff;
	priv->dev->dev_addr[2] = (priv->qp->qp_num >>  8) & 0xff;
	priv->dev->dev_addr[3] = (priv->qp->qp_num      ) & 0xff;

	for (i = 0; i < MAX_SKB_FRAGS + 1; ++i)
		priv->tx_sge[i].lkey = priv->pd->local_dma_lkey;

	priv->tx_wr.wr.opcode		= IB_WR_SEND;
	priv->tx_wr.wr.sg_list		= priv->tx_sge;
	priv->tx_wr.wr.send_flags	= IB_SEND_SIGNALED;

	priv->rx_sge[0].lkey = priv->pd->local_dma_lkey;

	priv->rx_sge[0].length = IPOIB_UD_BUF_SIZE(priv->max_ib_mtu);
	priv->rx_wr.num_sge = 1;

	priv->rx_wr.next = NULL;
	priv->rx_wr.sg_list = priv->rx_sge;

	return 0;

out_free_send_cq:
	ib_destroy_cq(priv->send_cq);

out_free_recv_cq:
	ib_destroy_cq(priv->recv_cq);

out_cm_dev_cleanup:
	ipoib_cm_dev_cleanup(dev);

out_free_wq:
	destroy_workqueue(priv->wq);
	priv->wq = NULL;

out_free_pd:
	ib_dealloc_pd(priv->pd);

	return -ENODEV;
}

void ipoib_transport_dev_cleanup(struct net_device *dev)
{
	struct ipoib_dev_priv *priv = netdev_priv(dev);

	if (priv->qp) {
		if (ib_destroy_qp(priv->qp))
			ipoib_warn(priv, "ib_qp_destroy failed\n");

		priv->qp = NULL;
		clear_bit(IPOIB_PKEY_ASSIGNED, &priv->flags);
	}

	if (ib_destroy_cq(priv->send_cq))
		ipoib_warn(priv, "ib_cq_destroy (send) failed\n");

	if (ib_destroy_cq(priv->recv_cq))
		ipoib_warn(priv, "ib_cq_destroy (recv) failed\n");

	ipoib_cm_dev_cleanup(dev);

	if (priv->wq) {
		flush_workqueue(priv->wq);
		destroy_workqueue(priv->wq);
		priv->wq = NULL;
	}

	ib_dealloc_pd(priv->pd);
}

void ipoib_event(struct ib_event_handler *handler,
		 struct ib_event *record)
{
	struct ipoib_dev_priv *priv =
		container_of(handler, struct ipoib_dev_priv, event_handler);

	if (record->element.port_num != priv->port)
		return;

	ipoib_dbg(priv, "Event %d on device %s port %d\n", record->event,
		  record->device->name, record->element.port_num);

	if (record->event == IB_EVENT_SM_CHANGE ||
	    record->event == IB_EVENT_CLIENT_REREGISTER) {
		queue_work(ipoib_workqueue, &priv->flush_light);
	} else if (record->event == IB_EVENT_PORT_ERR ||
		   record->event == IB_EVENT_PORT_ACTIVE ||
		   record->event == IB_EVENT_LID_CHANGE) {
		queue_work(ipoib_workqueue, &priv->flush_normal);
	} else if (record->event == IB_EVENT_PKEY_CHANGE) {
		queue_work(ipoib_workqueue, &priv->flush_heavy);
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Copyright (c) 2004 Topspin Communications.  All rights reserved.
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

#include <linux/module.h>

#include <linux/init.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>

#include "ipoib.h"

static ssize_t show_parent(struct device *d, struct device_attribute *attr,
			   char *buf)
{
	struct net_device *dev = to_net_dev(d);
	struct ipoib_dev_priv *priv = netdev_priv(dev);

	return sprintf(buf, "%s\n", priv->parent->name);
}
static DEVICE_ATTR(parent, S_IRUGO, show_parent, NULL);

int __ipoib_vlan_add(struct ipoib_dev_priv *ppriv, struct ipoib_dev_priv *priv,
		     u16 pkey, int type)
{
	int result;

	priv->max_ib_mtu = ppriv->max_ib_mtu;
	/* MTU will be reset when mcast join happens */
	priv->dev->mtu   = IPOIB_UD_MTU(priv->max_ib_mtu);
	priv->mcast_mtu  = priv->admin_mtu = priv->dev->mtu;
	priv->parent = ppriv->dev;
	set_bit(IPOIB_FLAG_SUBINTERFACE, &priv->flags);

	result = ipoib_set_dev_features(priv, ppriv->ca);
	if (result)
		goto err;

	priv->pkey = pkey;

	memcpy(priv->dev->dev_addr, ppriv->dev->dev_addr, INFINIBAND_ALEN);
	priv->dev->broadcast[8] = pkey >> 8;
	priv->dev->broadcast[9] = pkey & 0xff;

	result = ipoib_dev_init(priv->dev, ppriv->ca, ppriv->port);
	if (result < 0) {
		ipoib_warn(ppriv, "failed to initialize subinterface: "
			   "device %s, port %d",
			   ppriv->ca->name, ppriv->port);
		goto err;
	}

	result = register_netdevice(priv->dev);
	if (result) {
		ipoib_warn(priv, "failed to initialize; error %i", result);
		goto register_failed;
	}

	/* RTNL childs don't need proprietary sysfs entries */
	if (type == IPOIB_LEGACY_CHILD) {
		if (ipoib_cm_add_mode_attr(priv->dev))
			goto sysfs_failed;
		if (ipoib_add_pkey_attr(priv->dev))
			goto sysfs_failed;
		if (ipoib_add_umcast_attr(priv->dev))
			goto sysfs_failed;

		if (device_create_file(&priv->dev->dev, &dev_attr_parent))
			goto sysfs_failed;
	}

	priv->child_type  = type;
	list_add_tail(&priv->list, &ppriv->child_intfs);

	return 0;

sysfs_failed:
	result = -ENOMEM;
	unregister_netdevice(priv->dev);

register_failed:
	ipoib_dev_cleanup(priv->dev);

err:
	return result;
}

int ipoib_vlan_add(struct net_device *pdev, unsigned short pkey)
{
	struct ipoib_dev_priv *ppriv, *priv;
	char intf_name[IFNAMSIZ];
	struct ipoib_dev_priv *tpriv;
	int result;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	ppriv = netdev_priv(pdev);

	snprintf(intf_name, sizeof intf_name, "%s.%04x",
		 ppriv->dev->name, pkey);
	priv = ipoib_intf_alloc(intf_name);
	if (!priv)
		return -ENOMEM;

	if (!rtnl_trylock())
		return restart_syscall();

	down_write(&ppriv->vlan_rwsem);

	/*
	 * First ensure this isn't a duplicate. We check the parent device and
	 * then all of the legacy child interfaces to make sure the Pkey
	 * doesn't match.
	 */
	if (ppriv->pkey == pkey) {
		result = -ENOTUNIQ;
		goto out;
	}

	list_for_each_entry(tpriv, &ppriv->child_intfs, list) {
		if (tpriv->pkey == pkey &&
		    tpriv->child_type == IPOIB_LEGACY_CHILD) {
			result = -ENOTUNIQ;
			goto out;
		}
	}

	result = __ipoib_vlan_add(ppriv, priv, pkey, IPOIB_LEGACY_CHILD);

out:
	up_write(&ppriv->vlan_rwsem);

	rtnl_unlock();

	if (result)
		free_netdev(priv->dev);

	return result;
}

int ipoib_vlan_delete(struct net_device *pdev, unsigned short pkey)
{
	struct ipoib_dev_priv *ppriv, *priv, *tpriv;
	struct net_device *dev = NULL;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	ppriv = netdev_priv(pdev);

	if (!rtnl_trylock())
		return restart_syscall();

	down_write(&ppriv->vlan_rwsem);
	list_for_each_entry_safe(priv, tpriv, &ppriv->child_intfs, list) {
		if (priv->pkey == pkey &&
		    priv->child_type == IPOIB_LEGACY_CHILD) {
			list_del(&priv->list);
			dev = priv->dev;
			break;
		}
	}
	up_write(&ppriv->vlan_rwsem);

	if (dev) {
		ipoib_dbg(ppriv, "delete child vlan %s\n", dev->name);
		unregister_netdevice(dev);
	}

	rtnl_unlock();

	if (dev) {
		free_netdev(dev);
		return 0;
	}

	return -ENODEV;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          config INFINIBAND_ISER
	tristate "iSCSI Extensions for RDMA (iSER)"
	depends on SCSI && INET && INFINIBAND_ADDR_TRANS
	select SCSI_ISCSI_ATTRS
	---help---
	  Support for the iSCSI Extensions for RDMA (iSER) Protocol
          over InfiniBand. This allows you to access storage devices
          that speak iSCSI over iSER over InfiniBand.

	  The iSER protocol is defined by IETF.
	  See <http://www.ietf.org/rfc/rfc5046.txt>
	  and <http://members.infinibandta.org/kwspub/spec/Annex_iSER.PDF>
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  obj-$(CONFIG_INFINIBAND_ISER)	+= ib_iser.o

ib_iser-y			:= iser_verbs.o iser_initiator.o iser_memory.o \
				   iscsi_iser.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
 * iSCSI Initiator over iSER Data-Path
 *
 * Copyright (C) 2004 Dmitry Yusupov
 * Copyright (C) 2004 Alex Aizman
 * Copyright (C) 2005 Mike Christie
 * Copyright (c) 2005, 2006 Voltaire, Inc. All rights reserved.
 * Copyright (c) 2013-2014 Mellanox Technologies. All rights reserved.
 * maintained by openib-general@openib.org
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
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
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
 * Credits:
 *	Christoph Hellwig
 *	FUJITA Tomonori
 *	Arne Redlich
 *	Zhenyu Wang
 * Modified by:
 *      Erez Zilber
 */

#include <linux/types.h>
#include <linux/list.h>
#include <linux/hardirq.h>
#include <linux/kfifo.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <net/sock.h>

#include <asm/uaccess.h>

#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi.h>
#include <scsi/scsi_transport_iscsi.h>

#include "iscsi_iser.h"

MODULE_DESCRIPTION("iSER (iSCSI Extensions for RDMA) Datamover");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alex Nezhinsky, Dan Bar Dov, Or Gerlitz");
MODULE_VERSION(DRV_VER);

static struct scsi_host_template iscsi_iser_sht;
static struct iscsi_transport iscsi_iser_transport;
static struct scsi_transport_template *iscsi_iser_scsi_transport;
static struct workqueue_struct *release_wq;
struct iser_global ig;

int iser_debug_level = 0;
module_param_named(debug_level, iser_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level, "Enable debug tracing if > 0 (default:disabled)");

static unsigned int iscsi_max_lun = 512;
module_param_named(max_lun, iscsi_max_lun, uint, S_IRUGO);
MODULE_PARM_DESC(max_lun, "Max LUNs to allow per session (default:512");

unsigned int iser_max_sectors = ISER_DEF_MAX_SECTORS;
module_param_named(max_sectors, iser_max_sectors, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(max_sectors, "Max number of sectors in a single scsi command (default:1024");

bool iser_always_reg = true;
module_param_named(always_register, iser_always_reg, bool, S_IRUGO);
MODULE_PARM_DESC(always_register,
		 "Always register memory, even for continuous memory regions (default:true)");

bool iser_pi_enable = false;
module_param_named(pi_enable, iser_pi_enable, bool, S_IRUGO);
MODULE_PARM_DESC(pi_enable, "Enable T10-PI offload support (default:disabled)");

int iser_pi_guard;
module_param_named(pi_guard, iser_pi_guard, int, S_IRUGO);
MODULE_PARM_DESC(pi_guard, "T10-PI guard_type [deprecated]");

/*
 * iscsi_iser_recv() - Process a successful recv completion
 * @conn:         iscsi connection
 * @hdr:          iscsi header
 * @rx_data:      buffer containing receive data payload
 * @rx_data_len:  length of rx_data
 *
 * Notes: In case of data length errors or iscsi PDU completion failures
 *        this routine will signal iscsi layer of connection failure.
 */
void
iscsi_iser_recv(struct iscsi_conn *conn, struct iscsi_hdr *hdr,
		char *rx_data, int rx_data_len)
{
	int rc = 0;
	int datalen;

	/* verify PDU length */
	datalen = ntoh24(hdr->dlength);
	if (datalen > rx_data_len || (datalen + 4) < rx_data_len) {
		iser_err("wrong datalen %d (hdr), %d (IB)\n",
			datalen, rx_data_len);
		rc = ISCSI_ERR_DATALEN;
		goto error;
	}

	if (datalen != rx_data_len)
		iser_dbg("aligned datalen (%d) hdr, %d (IB)\n",
			datalen, rx_data_len);

	rc = iscsi_complete_pdu(conn, hdr, rx_data, rx_data_len);
	if (rc && rc != ISCSI_ERR_NO_SCSI_CMD)
		goto error;

	return;
error:
	iscsi_conn_failure(conn, rc);
}

/**
 * iscsi_iser_pdu_alloc() - allocate an iscsi-iser PDU
 * @task:     iscsi task
 * @opcode:   iscsi command opcode
 *
 * Netes: This routine can't fail, just assign iscsi task
 *        hdr and max hdr size.
 */
static int
iscsi_iser_pdu_alloc(struct iscsi_task *task, uint8_t opcode)
{
	struct iscsi_iser_task *iser_task = task->dd_data;

	task->hdr = (struct iscsi_hdr *)&iser_task->desc.iscsi_header;
	task->hdr_max = sizeof(iser_task->desc.iscsi_header);

	return 0;
}

/**
 * iser_initialize_task_headers() - Initialize task headers
 * @task:       iscsi task
 * @tx_desc:    iser tx descriptor
 *
 * Notes:
 * This routine may race with iser teardown flow for scsi
 * error handling TMFs. So for TMF we should acquire the
 * state mutex to avoid dereferencing the IB device which
 * may have already been terminated.
 */
int
iser_initialize_task_headers(struct iscsi_task *task,
			     struct iser_tx_desc *tx_desc)
{
	struct iser_conn *iser_conn = task->conn->dd_data;
	struct iser_device *device = iser_conn->ib_conn.device;
	struct iscsi_iser_task *iser_task = task->dd_data;
	u64 dma_addr;
	const bool mgmt_task = !task->sc && !in_interrupt();
	int ret = 0;

	if (unlikely(mgmt_task))
		mutex_lock(&iser_conn->state_mutex);

	if (unlikely(iser_conn->state != ISER_CONN_UP)) {
		ret = -ENODEV;
		goto out;
	}

	dma_addr = ib_dma_map_single(device->ib_device, (void *)tx_desc,
				ISER_HEADERS_LEN, DMA_TO_DEVICE);
	if (ib_dma_mapping_error(device->ib_device, dma_addr)) {
		ret = -ENOMEM;
		goto out;
	}

	tx_desc->wr_idx = 0;
	tx_desc->mapped = true;
	tx_desc->dma_addr = dma_addr;
	tx_desc->tx_sg[0].addr   = tx_desc->dma_addr;
	tx_desc->tx_sg[0].length = ISER_HEADERS_LEN;
	tx_desc->tx_sg[0].lkey   = device->pd->local_dma_lkey;

	iser_task->iser_conn = iser_conn;
out:
	if (unlikely(mgmt_task))
		mutex_unlock(&iser_conn->state_mutex);

	return ret;
}

/**
 * iscsi_iser_task_init() - Initialize iscsi-iser task
 * @task: iscsi task
 *
 * Initialize the task for the scsi command or mgmt command.
 *
 * Return: Returns zero on success or -ENOMEM when failing
 *         to init task headers (dma mapping error).
 */
static int
iscsi_iser_task_init(struct iscsi_task *task)
{
	struct iscsi_iser_task *iser_task = task->dd_data;
	int ret;

	ret = iser_initialize_task_headers(task, &iser_task->desc);
	if (ret) {
		iser_err("Failed to init task %p, err = %d\n",
			 iser_task, ret);
		return ret;
	}

	/* mgmt task */
	if (!task->sc)
		return 0;

	iser_task->command_sent = 0;
	iser_task_rdma_init(iser_task);
	iser_task->sc = task->sc;

	return 0;
}

/**
 * iscsi_iser_mtask_xmit() - xmit management (immediate) task
 * @conn: iscsi connection
 * @task: task management task
 *
 * Notes:
 *	The function can return -EAGAIN in which case caller must
 *	call it again later, or recover. '0' return code means successful
 *	xmit.
 *
 **/
static int
iscsi_iser_mtask_xmit(struct iscsi_conn *conn, struct iscsi_task *task)
{
	int error = 0;

	iser_dbg("mtask xmit [cid %d itt 0x%x]\n", conn->id, task->itt);

	error = iser_send_control(conn, task);

	/* since iser xmits control with zero copy, tasks can not be recycled
	 * right after sending them.
	 * The recycling scheme is based on whether a response is expected
	 * - if yes, the task is recycled at iscsi_complete_pdu
	 * - if no,  the task is recycled at iser_snd_completion
	 */
	return error;
}

static int
iscsi_iser_task_xmit_unsol_data(struct iscsi_conn *conn,
				 struct iscsi_task *task)
{
	struct iscsi_r2t_info *r2t = &task->unsol_r2t;
	struct iscsi_data hdr;
	int error = 0;

	/* Send data-out PDUs while there's still unsolicited data to send */
	while (iscsi_task_has_unsol_data(task)) {
		iscsi_prep_data_out_pdu(task, r2t, &hdr);
		iser_dbg("Sending data-out: itt 0x%x, data count %d\n",
			   hdr.itt, r2t->data_count);

		/* the buffer description has been passed with the command */
		/* Send the command */
		error = iser_send_data_out(conn, task, &hdr);
		if (error) {
			r2t->datasn--;
			goto iscsi_iser_task_xmit_unsol_data_exit;
		}
		r2t->sent += r2t->data_count;
		iser_dbg("Need to send %d more as data-out PDUs\n",
			   r2t->data_length - r2t->sent);
	}

iscsi_iser_task_xmit_unsol_data_exit:
	return error;
}

/**
 * iscsi_iser_task_xmit() - xmit iscsi-iser task
 * @task: iscsi task
 *
 * Return: zero on success or escalates $error on failure.
 */
static int
iscsi_iser_task_xmit(struct iscsi_task *task)
{
	struct iscsi_conn *conn = task->conn;
	struct iscsi_iser_task *iser_task = task->dd_data;
	int error = 0;

	if (!task->sc)
		return iscsi_iser_mtask_xmit(conn, task);

	if (task->sc->sc_data_direction == DMA_TO_DEVICE) {
		BUG_ON(scsi_bufflen(task->sc) == 0);

		iser_dbg("cmd [itt %x total %d imm %d unsol_data %d\n",
			   task->itt, scsi_bufflen(task->sc),
			   task->imm_count, task->unsol_r2t.data_length);
	}

	iser_dbg("ctask xmit [cid %d itt 0x%x]\n",
		   conn->id, task->itt);

	/* Send the cmd PDU */
	if (!iser_task->command_sent) {
		error = iser_send_command(conn, task);
		if (error)
			goto iscsi_iser_task_xmit_exit;
		iser_task->command_sent = 1;
	}

	/* Send unsolicited data-out PDU(s) if necessary */
	if (iscsi_task_has_unsol_data(task))
		error = iscsi_iser_task_xmit_unsol_data(conn, task);

 iscsi_iser_task_xmit_exit:
	return error;
}

/**
 * iscsi_iser_cleanup_task() - cleanup an iscsi-iser task
 * @task: iscsi task
 *
 * Notes: In case the RDMA device is already NULL (might have
 *        been removed in DEVICE_REMOVAL CM event it will bail-out
 *        without doing dma unmapping.
 */
static void iscsi_iser_cleanup_task(struct iscsi_task *task)
{
	struct iscsi_iser_task *iser_task = task->dd_data;
	struct iser_tx_desc *tx_desc = &iser_task->desc;
	struct iser_conn *iser_conn = task->conn->dd_data;
	struct iser_device *device = iser_conn->ib_conn.device;

	/* DEVICE_REMOVAL event might have already released the device */
	if (!device)
		return;

	if (likely(tx_desc->mapped)) {
		ib_dma_unmap_single(device->ib_device, tx_desc->dma_addr,
				    ISER_HEADERS_LEN, DMA_TO_DEVICE);
		tx_desc->mapped = false;
	}

	/* mgmt tasks do not need special cleanup */
	if (!task->sc)
		return;

	if (iser_task->status == ISER_TASK_STATUS_STARTED) {
		iser_task->status = ISER_TASK_STATUS_COMPLETED;
		iser_task_rdma_finalize(iser_task);
	}
}

/**
 * iscsi_iser_check_protection() - check protection information status of task.
 * @task:     iscsi task
 * @sector:   error sector if exsists (output)
 *
 * Return: zero if no data-integrity errors have occured
 *         0x1: data-integrity error occured in the guard-block
 *         0x2: data-integrity error occured in the reference tag
 *         0x3: data-integrity error occured in the application tag
 *
 *         In addition the error sector is marked.
 */
static u8
iscsi_iser_check_protection(struct iscsi_task *task, sector_t *sector)
{
	struct iscsi_iser_task *iser_task = task->dd_data;

	if (iser_task->dir[ISER_DIR_IN])
		return iser_check_task_pi_status(iser_task, ISER_DIR_IN,
						 sector);
	else
		return iser_check_task_pi_status(iser_task, ISER_DIR_OUT,
						 sector);
}

/**
 * iscsi_iser_conn_create() - create a new iscsi-iser connection
 * @cls_session: iscsi class connection
 * @conn_idx:    connection index within the session (for MCS)
 *
 * Return: iscsi_cls_conn when iscsi_conn_setup succeeds or NULL
 *         otherwise.
 */
static struct iscsi_cls_conn *
iscsi_iser_conn_create(struct iscsi_cls_session *cls_session,
		       uint32_t conn_idx)
{
	struct iscsi_conn *conn;
	struct iscsi_cls_conn *cls_conn;

	cls_conn = iscsi_conn_setup(cls_session, 0, conn_idx);
	if (!cls_conn)
		return NULL;
	conn = cls_conn->dd_data;

	/*
	 * due to issues with the login code re iser sematics
	 * this not set in iscsi_conn_setup - FIXME
	 */
	conn->max_recv_dlength = ISER_RECV_DATA_SEG_LEN;

	return cls_conn;
}

/**
 * iscsi_iser_conn_bind() - bind iscsi and iser connection structures
 * @cls_session:     iscsi class session
 * @cls_conn:        iscsi class connection
 * @transport_eph:   transport end-point handle
 * @is_leading:      indicate if this is the session leading connection (MCS)
 *
 * Return: zero on success, $error if iscsi_conn_bind fails and
 *         -EINVAL in case end-point doesn't exsits anymore or iser connection
 *         state is not UP (teardown already started).
 */
static int
iscsi_iser_conn_bind(struct iscsi_cls_session *cls_session,
		     struct iscsi_cls_conn *cls_conn,
		     uint64_t transport_eph,
		     int is_leading)
{
	struct iscsi_conn *conn = cls_conn->dd_data;
	struct iser_conn *iser_conn;
	struct iscsi_endpoint *ep;
	int error;

	error = iscsi_conn_bind(cls_session, cls_conn, is_leading);
	if (error)
		return error;

	/* the transport ep handle comes from user space so it must be
	 * verified against the global ib connections list */
	ep = iscsi_lookup_endpoint(transport_eph);
	if (!ep) {
		iser_err("can't bind eph %llx\n",
			 (unsigned long long)transport_eph);
		return -EINVAL;
	}
	iser_conn = ep->dd_data;

	mutex_lock(&iser_conn->state_mutex);
	if (iser_conn->state != ISER_CONN_UP) {
		error = -EINVAL;
		iser_err("iser_conn %p state is %d, teardown started\n",
			 iser_conn, iser_conn->state);
		goto out;
	}

	error = iser_alloc_rx_descriptors(iser_conn, conn->session);
	if (error)
		goto out;

	/* binds the iSER connection retrieved from the previously
	 * connected ep_handle to the iSCSI layer connection. exchanges
	 * connection pointers */
	iser_info("binding iscsi conn %p to iser_conn %p\n", conn, iser_conn);

	conn->dd_data = iser_conn;
	iser_conn->iscsi_conn = conn;

out:
	mutex_unlock(&iser_conn->state_mutex);
	return error;
}

/**
 * iscsi_iser_conn_start() - start iscsi-iser connection
 * @cls_conn: iscsi class connection
 *
 * Notes: Here iser intialize (or re-initialize) stop_completion as
 *        from this point iscsi must call conn_stop in session/connection
 *        teardown so iser transport must wait for it.
 */
static int
iscsi_iser_conn_start(struct iscsi_cls_conn *cls_conn)
{
	struct iscsi_conn *iscsi_conn;
	struct iser_conn *iser_conn;

	iscsi_conn = cls_conn->dd_data;
	iser_conn = iscsi_conn->dd_data;
	reinit_completion(&iser_conn->stop_completion);

	return iscsi_conn_start(cls_conn);
}

/**
 * iscsi_iser_conn_stop() - stop iscsi-iser connection
 * @cls_conn:  iscsi class connection
 * @flag:      indicate if recover or terminate (passed as is)
 *
 * Notes: Calling iscsi_conn_stop might theoretically race with
 *        DEVICE_REMOVAL event and dereference a previously freed RDMA device
 *        handle, so we call it under iser the state lock to protect against
 *        this kind of race.
 */
static void
iscsi_iser_conn_stop(struct iscsi_cls_conn *cls_conn, int flag)
{
	struct iscsi_conn *conn = cls_conn->dd_data;
	struct iser_conn *iser_conn = conn->dd_data;

	iser_info("stopping iscsi_conn: %p, iser_conn: %p\n", conn, iser_conn);

	/*
	 * Userspace may have goofed up and not bound the connection or
	 * might have only partially setup the connection.
	 */
	if (iser_conn) {
		mutex_lock(&iser_conn->state_mutex);
		iser_conn_terminate(iser_conn);
		iscsi_conn_stop(cls_conn, flag);

		/* unbind */
		iser_conn->iscsi_conn = NULL;
		conn->dd_data = NULL;

		complete(&iser_conn->stop_completion);
		mutex_unlock(&iser_conn->state_mutex);
	} else {
		iscsi_conn_stop(cls_conn, flag);
	}
}

/**
 * iscsi_iser_session_destroy() - destroy iscsi-iser session
 * @cls_session: iscsi class session
 *
 * Removes and free iscsi host.
 */
static void
iscsi_iser_session_destroy(struct iscsi_cls_session *cls_session)
{
	struct Scsi_Host *shost = iscsi_session_to_shost(cls_session);

	iscsi_session_teardown(cls_session);
	iscsi_host_remove(shost);
	iscsi_host_free(shost);
}

static inline unsigned int
iser_dif_prot_caps(int prot_caps)
{
	return ((prot_caps & IB_PROT_T10DIF_TYPE_1) ?
		SHOST_DIF_TYPE1_PROTECTION | SHOST_DIX_TYPE0_PROTECTION |
		SHOST_DIX_TYPE1_PROTECTION : 0) |
	       ((prot_caps & IB_PROT_T10DIF_TYPE_2) ?
		SHOST_DIF_TYPE2_PROTECTION | SHOST_DIX_TYPE2_PROTECTION : 0) |
	       ((prot_caps & IB_PROT_T10DIF_TYPE_3) ?
		SHOST_DIF_TYPE3_PROTECTION | SHOST_DIX_TYPE3_PROTECTION : 0);
}

/**
 * iscsi_iser_session_create() - create an iscsi-iser session
 * @ep:             iscsi end-point handle
 * @cmds_max:       maximum commands in this session
 * @qdepth:         session command queue depth
 * @initial_cmdsn:  initiator command sequnce number
 *
 * Allocates and adds a scsi host, expose DIF supprot if
 * exists, and sets up an iscsi session.
 */
static struct iscsi_cls_session *
iscsi_iser_session_create(struct iscsi_endpoint *ep,
			  uint16_t cmds_max, uint16_t qdepth,
			  uint32_t initial_cmdsn)
{
	struct iscsi_cls_session *cls_session;
	struct iscsi_session *session;
	struct Scsi_Host *shost;
	struct iser_conn *iser_conn = NULL;
	struct ib_conn *ib_conn;
	u16 max_cmds;

	shost = iscsi_host_alloc(&iscsi_iser_sht, 0, 0);
	if (!shost)
		return NULL;
	shost->transportt = iscsi_iser_scsi_transport;
	shost->cmd_per_lun = qdepth;
	shost->max_lun = iscsi_max_lun;
	shost->max_id = 0;
	shost->max_channel = 0;
	shost->max_cmd_len = 16;

	/*
	 * older userspace tools (before 2.0-870) did not pass us
	 * the leading conn's ep so this will be NULL;
	 */
	if (ep) {
		iser_conn = ep->dd_data;
		max_cmds = iser_conn->max_cmds;
		shost->sg_tablesize = iser_conn->scsi_sg_tablesize;
		shost->max_sectors = iser_conn->scsi_max_sectors;

		mutex_lock(&iser_conn->state_mutex);
		if (iser_conn->state != ISER_CONN_UP) {
			iser_err("iser conn %p already started teardown\n",
				 iser_conn);
			mutex_unlock(&iser_conn->state_mutex);
			goto free_host;
		}

		ib_conn = &iser_conn->ib_conn;
		if (ib_conn->pi_support) {
			u32 sig_caps = ib_conn->device->dev_attr.sig_prot_cap;

			shost->sg_prot_tablesize = shost->sg_tablesize;
			scsi_host_set_prot(shost, iser_dif_prot_caps(sig_caps));
			scsi_host_set_guard(shost, SHOST_DIX_GUARD_IP |
						   SHOST_DIX_GUARD_CRC);
		}

		/*
		 * Limit the sg_tablesize and max_sectors based on the device
		 * max fastreg page list length.
		 */
		shost->sg_tablesize = min_t(unsigned short, shost->sg_tablesize,
			ib_conn->device->dev_attr.max_fast_reg_page_list_len);
		shost->max_sectors = min_t(unsigned int,
			1024, (shost->sg_tablesize * PAGE_SIZE) >> 9);

		if (iscsi_host_add(shost,
				   ib_conn->device->ib_device->dma_device)) {
			mutex_unlock(&iser_conn->state_mutex);
			goto free_host;
		}
		mutex_unlock(&iser_conn->state_mutex);
	} else {
		max_cmds = ISER_DEF_XMIT_CMDS_MAX;
		if (iscsi_host_add(shost, NULL))
			goto free_host;
	}

	if (cmds_max > max_cmds) {
		iser_info("cmds_max changed from %u to %u\n",
			  cmds_max, max_cmds);
		cmds_max = max_cmds;
	}

	cls_session = iscsi_session_setup(&iscsi_iser_transport, shost,
					  cmds_max, 0,
					  sizeof(struct iscsi_iser_task),
					  initial_cmdsn, 0);
	if (!cls_session)
		goto remove_host;
	session = cls_session->dd_data;

	shost->can_queue = session->scsi_cmds_max;
	return cls_session;

remove_host:
	iscsi_host_remove(shost);
free_host:
	iscsi_host_free(shost);
	return NULL;
}

static int
iscsi_iser_set_param(struct iscsi_cls_conn *cls_conn,
		     enum iscsi_param param, char *buf, int buflen)
{
	int value;

	switch (param) {
	case ISCSI_PARAM_MAX_RECV_DLENGTH:
		/* TBD */
		break;
	case ISCSI_PARAM_HDRDGST_EN:
		sscanf(buf, "%d", &value);
		if (value) {
			iser_err("DataDigest wasn't negotiated to None\n");
			return -EPROTO;
		}
		break;
	case ISCSI_PARAM_DATADGST_EN:
		sscanf(buf, "%d", &value);
		if (value) {
			iser_err("DataDigest wasn't negotiated to None\n");
			return -EPROTO;
		}
		break;
	case ISCSI_PARAM_IFMARKER_EN:
		sscanf(buf, "%d", &value);
		if (value) {
			iser_err("IFMarker wasn't negotiated to No\n");
			return -EPROTO;
		}
		break;
	case ISCSI_PARAM_OFMARKER_EN:
		sscanf(buf, "%d", &value);
		if (value) {
			iser_err("OFMarker wasn't negotiated to No\n");
			return -EPROTO;
		}
		break;
	default:
		return iscsi_set_param(cls_conn, param, buf, buflen);
	}

	return 0;
}

/**
 * iscsi_iser_set_param() - set class connection parameter
 * @cls_conn:    iscsi class connection
 * @stats:       iscsi stats to output
 *
 * Output connection statistics.
 */
static void
iscsi_iser_conn_get_stats(struct iscsi_cls_conn *cls_conn, struct iscsi_stats *stats)
{
	struct iscsi_conn *conn = cls_conn->dd_data;

	stats->txdata_octets = conn->txdata_octets;
	stats->rxdata_octets = conn->rxdata_octets;
	stats->scsicmd_pdus = conn->scsicmd_pdus_cnt;
	stats->dataout_pdus = conn->dataout_pdus_cnt;
	stats->scsirsp_pdus = conn->scsirsp_pdus_cnt;
	stats->datain_pdus = conn->datain_pdus_cnt; /* always 0 */
	stats->r2t_pdus = conn->r2t_pdus_cnt; /* always 0 */
	stats->tmfcmd_pdus = conn->tmfcmd_pdus_cnt;
	stats->tmfrsp_pdus = conn->tmfrsp_pdus_cnt;
	stats->custom_length = 0;
}

static int iscsi_iser_get_ep_param(struct iscsi_endpoint *ep,
				   enum iscsi_param param, char *buf)
{
	struct iser_conn *iser_conn = ep->dd_data;
	int len;

	switch (param) {
	case ISCSI_PARAM_CONN_PORT:
	case ISCSI_PARAM_CONN_ADDRESS:
		if (!iser_conn || !iser_conn->ib_conn.cma_id)
			return -ENOTCONN;

		return iscsi_conn_get_addr_param((struct sockaddr_storage *)
				&iser_conn->ib_conn.cma_id->route.addr.dst_addr,
				param, buf);
		break;
	default:
		return -ENOSYS;
	}

	return len;
}

/**
 * iscsi_iser_ep_connect() - Initiate iSER connection establishment
 * @shost:          scsi_host
 * @dst_addr:       destination address
 * @non-blocking:   indicate if routine can block
 *
 * Allocate an iscsi endpoint, an iser_conn structure and bind them.
 * After that start RDMA connection establishment via rdma_cm. We
 * don't allocate iser_conn embedded in iscsi_endpoint since in teardown
 * the endpoint will be destroyed at ep_disconnect while iser_conn will
 * cleanup its resources asynchronuously.
 *
 * Return: iscsi_endpoint created by iscsi layer or ERR_PTR(error)
 *         if fails.
 */
static struct iscsi_endpoint *
iscsi_iser_ep_connect(struct Scsi_Host *shost, struct sockaddr *dst_addr,
		      int non_blocking)
{
	int err;
	struct iser_conn *iser_conn;
	struct iscsi_endpoint *ep;

	ep = iscsi_create_endpoint(0);
	if (!ep)
		return ERR_PTR(-ENOMEM);

	iser_conn = kzalloc(sizeof(*iser_conn), GFP_KERNEL);
	if (!iser_conn) {
		err = -ENOMEM;
		goto failure;
	}

	ep->dd_data = iser_conn;
	iser_conn->ep = ep;
	iser_conn_init(iser_conn);

	err = iser_connect(iser_conn, NULL, dst_addr, non_blocking);
	if (err)
		goto failure;

	return ep;
failure:
	iscsi_destroy_endpoint(ep);
	return ERR_PTR(err);
}

/**
 * iscsi_iser_ep_poll() - poll for iser connection establishment to complete
 * @ep:            iscsi endpoint (created at ep_connect)
 * @timeout_ms:    polling timeout allowed in ms.
 *
 * This routine boils down to waiting for up_completion signaling
 * that cma_id got CONNECTED event.
 *
 * Return: 1 if succeeded in connection establishment, 0 if timeout expired
 *         (libiscsi will retry will kick in) or -1 if interrupted by signal
 *         or more likely iser connection state transitioned to TEMINATING or
 *         DOWN during the wait period.
 */
static int
iscsi_iser_ep_poll(struct iscsi_endpoint *ep, int timeout_ms)
{
	struct iser_conn *iser_conn = ep->dd_data;
	int rc;

	rc = wait_for_completion_interruptible_timeout(&iser_conn->up_completion,
						       msecs_to_jiffies(timeout_ms));
	/* if conn establishment failed, return error code to iscsi */
	if (rc == 0) {
		mutex_lock(&iser_conn->state_mutex);
		if (iser_conn->state == ISER_CONN_TERMINATING ||
		    iser_conn->state == ISER_CONN_DOWN)
			rc = -1;
		mutex_unlock(&iser_conn->state_mutex);
	}

	iser_info("iser conn %p rc = %d\n", iser_conn, rc);

	if (rc > 0)
		return 1; /* success, this is the equivalent of POLLOUT */
	else if (!rc)
		return 0; /* timeout */
	else
		return rc; /* signal */
}

/**
 * iscsi_iser_ep_disconnect() - Initiate connection teardown process
 * @ep:    iscsi endpoint handle
 *
 * This routine is not blocked by iser and RDMA termination process
 * completion as we queue a deffered work for iser/RDMA destruction
 * and cleanup or actually call it immediately in case we didn't pass
 * iscsi conn bind/start stage, thus it is safe.
 */
static void
iscsi_iser_ep_disconnect(struct iscsi_endpoint *ep)
{
	struct iser_conn *iser_conn = ep->dd_data;

	iser_info("ep %p iser conn %p\n", ep, iser_conn);

	mutex_lock(&iser_conn->state_mutex);
	iser_conn_terminate(iser_conn);

	/*
	 * if iser_conn and iscsi_conn are bound, we must wait for
	 * iscsi_conn_stop and flush errors completion before freeing
	 * the iser resources. Otherwise we are safe to free resources
	 * immediately.
	 */
	if (iser_conn->iscsi_conn) {
		INIT_WORK(&iser_conn->release_work, iser_release_work);
		queue_work(release_wq, &iser_conn->release_work);
		mutex_unlock(&iser_conn->state_mutex);
	} else {
		iser_conn->state = ISER_CONN_DOWN;
		mutex_unlock(&iser_conn->state_mutex);
		iser_conn_release(iser_conn);
	}

	iscsi_destroy_endpoint(ep);
}

static umode_t iser_attr_is_visible(int param_type, int param)
{
	switch (param_type) {
	case ISCSI_HOST_PARAM:
		switch (param) {
		case ISCSI_HOST_PARAM_NETDEV_NAME:
		case ISCSI_HOST_PARAM_HWADDRESS:
		case ISCSI_HOST_PARAM_INITIATOR_NAME:
			return S_IRUGO;
		default:
			return 0;
		}
	case ISCSI_PARAM:
		switch (param) {
		case ISCSI_PARAM_MAX_RECV_DLENGTH:
		case ISCSI_PARAM_MAX_XMIT_DLENGTH:
		case ISCSI_PARAM_HDRDGST_EN:
		case ISCSI_PARAM_DATADGST_EN:
		case ISCSI_PARAM_CONN_ADDRESS:
		case ISCSI_PARAM_CONN_PORT:
		case ISCSI_PARAM_EXP_STATSN:
		case ISCSI_PARAM_PERSISTENT_ADDRESS:
		case ISCSI_PARAM_PERSISTENT_PORT:
		case ISCSI_PARAM_PING_TMO:
		case ISCSI_PARAM_RECV_TMO:
		case ISCSI_PARAM_INITIAL_R2T_EN:
		case ISCSI_PARAM_MAX_R2T:
		case ISCSI_PARAM_IMM_DATA_EN:
		case ISCSI_PARAM_FIRST_BURST:
		case ISCSI_PARAM_MAX_BURST:
		case ISCSI_PARAM_PDU_INORDER_EN:
		case ISCSI_PARAM_DATASEQ_INORDER_EN:
		case ISCSI_PARAM_TARGET_NAME:
		case ISCSI_PARAM_TPGT:
		case ISCSI_PARAM_USERNAME:
		case ISCSI_PARAM_PASSWORD:
		case ISCSI_PARAM_USERNAME_IN:
		case ISCSI_PARAM_PASSWORD_IN:
		case ISCSI_PARAM_FAST_ABORT:
		case ISCSI_PARAM_ABORT_TMO:
		case ISCSI_PARAM_LU_RESET_TMO:
		case ISCSI_PARAM_TGT_RESET_TMO:
		case ISCSI_PARAM_IFACE_NAME:
		case ISCSI_PARAM_INITIATOR_NAME:
		case ISCSI_PARAM_DISCOVERY_SESS:
			return S_IRUGO;
		default:
			return 0;
		}
	}

	return 0;
}

static int iscsi_iser_slave_alloc(struct scsi_device *sdev)
{
	blk_queue_virt_boundary(sdev->request_queue, ~MASK_4K);

	return 0;
}

static struct scsi_host_template iscsi_iser_sht = {
	.module                 = THIS_MODULE,
	.name                   = "iSCSI Initiator over iSER",
	.queuecommand           = iscsi_queuecommand,
	.change_queue_depth	= scsi_change_queue_depth,
	.sg_tablesize           = ISCSI_ISER_DEF_SG_TABLESIZE,
	.max_sectors            = ISER_DEF_MAX_SECTORS,
	.cmd_per_lun            = ISER_DEF_CMD_PER_LUN,
	.eh_abort_handler       = iscsi_eh_abort,
	.eh_device_reset_handler= iscsi_eh_device_reset,
	.eh_target_reset_handler = iscsi_eh_recover_target,
	.target_alloc		= iscsi_target_alloc,
	.use_clustering         = ENABLE_CLUSTERING,
	.slave_alloc            = iscsi_iser_slave_alloc,
	.proc_name              = "iscsi_iser",
	.this_id                = -1,
	.track_queue_depth	= 1,
};

static struct iscsi_transport iscsi_iser_transport = {
	.owner                  = THIS_MODULE,
	.name                   = "iser",
	.caps                   = CAP_RECOVERY_L0 | CAP_MULTI_R2T | CAP_TEXT_NEGO,
	/* session management */
	.create_session         = iscsi_iser_session_create,
	.destroy_session        = iscsi_iser_session_destroy,
	/* connection management */
	.create_conn            = iscsi_iser_conn_create,
	.bind_conn              = iscsi_iser_conn_bind,
	.destroy_conn           = iscsi_conn_teardown,
	.attr_is_visible	= iser_attr_is_visible,
	.set_param              = iscsi_iser_set_param,
	.get_conn_param		= iscsi_conn_get_param,
	.get_ep_param		= iscsi_iser_get_ep_param,
	.get_session_param	= iscsi_session_get_param,
	.start_conn             = iscsi_iser_conn_start,
	.stop_conn              = iscsi_iser_conn_stop,
	/* iscsi host params */
	.get_host_param		= iscsi_host_get_param,
	.set_host_param		= iscsi_host_set_param,
	/* IO */
	.send_pdu		= iscsi_conn_send_pdu,
	.get_stats		= iscsi_iser_conn_get_stats,
	.init_task		= iscsi_iser_task_init,
	.xmit_task		= iscsi_iser_task_xmit,
	.cleanup_task		= iscsi_iser_cleanup_task,
	.alloc_pdu		= iscsi_iser_pdu_alloc,
	.check_protection	= iscsi_iser_check_protection,
	/* recovery */
	.session_recovery_timedout = iscsi_session_recovery_timedout,

	.ep_connect             = iscsi_iser_ep_connect,
	.ep_poll                = iscsi_iser_ep_poll,
	.ep_disconnect          = iscsi_iser_ep_disconnect
};

static int __init iser_init(void)
{
	int err;

	iser_dbg("Starting iSER datamover...\n");

	if (iscsi_max_lun < 1) {
		iser_err("Invalid max_lun value of %u\n", iscsi_max_lun);
		return -EINVAL;
	}

	memset(&ig, 0, sizeof(struct iser_global));

	ig.desc_cache = kmem_cache_create("iser_descriptors",
					  sizeof(struct iser_tx_desc),
					  0, SLAB_HWCACHE_ALIGN,
					  NULL);
	if (ig.desc_cache == NULL)
		return -ENOMEM;

	/* device init is called only after the first addr resolution */
	mutex_init(&ig.device_list_mutex);
	INIT_LIST_HEAD(&ig.device_list);
	mutex_init(&ig.connlist_mutex);
	INIT_LIST_HEAD(&ig.connlist);

	release_wq = alloc_workqueue("release workqueue", 0, 0);
	if (!release_wq) {
		iser_err("failed to allocate release workqueue\n");
		return -ENOMEM;
	}

	iscsi_iser_scsi_transport = iscsi_register_transport(
							&iscsi_iser_transport);
	if (!iscsi_iser_scsi_transport) {
		iser_err("iscsi_register_transport failed\n");
		err = -EINVAL;
		goto register_transport_failure;
	}

	return 0;

register_transport_failure:
	kmem_cache_destroy(ig.desc_cache);

	return err;
}

static void __exit iser_exit(void)
{
	struct iser_conn *iser_conn, *n;
	int connlist_empty;

	iser_dbg("Removing iSER datamover...\n");
	destroy_workqueue(release_wq);

	mutex_lock(&ig.connlist_mutex);
	connlist_empty = list_empty(&ig.connlist);
	mutex_unlock(&ig.connlist_mutex);

	if (!connlist_empty) {
		iser_err("Error cleanup stage completed but we still have iser "
			 "connections, destroying them anyway\n");
		list_for_each_entry_safe(iser_conn, n, &ig.connlist,
					 conn_list) {
			iser_conn_release(iser_conn);
		}
	}

	iscsi_unregister_transport(&iscsi_iser_transport);
	kmem_cache_destroy(ig.desc_cache);
}

module_init(iser_init);
module_exit(iser_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * iSER transport for the Open iSCSI Initiator & iSER transport internals
 *
 * Copyright (C) 2004 Dmitry Yusupov
 * Copyright (C) 2004 Alex Aizman
 * Copyright (C) 2005 Mike Christie
 * based on code maintained by open-iscsi@googlegroups.com
 *
 * Copyright (c) 2004, 2005, 2006 Voltaire, Inc. All rights reserved.
 * Copyright (c) 2005, 2006 Cisco Systems.  All rights reserved.
 * Copyright (c) 2013-2014 Mellanox Technologies. All rights reserved.
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
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
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
#ifndef __ISCSI_ISER_H__
#define __ISCSI_ISER_H__

#include <linux/types.h>
#include <linux/net.h>
#include <linux/printk.h>
#include <scsi/libiscsi.h>
#include <scsi/scsi_transport_iscsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/mempool.h>
#include <linux/uio.h>

#include <linux/socket.h>
#include <linux/in.h>
#include <linux/in6.h>

#include <rdma/ib_verbs.h>
#include <rdma/ib_fmr_pool.h>
#include <rdma/rdma_cm.h>

#define DRV_NAME	"iser"
#define PFX		DRV_NAME ": "
#define DRV_VER		"1.6"

#define iser_dbg(fmt, arg...)				 \
	do {						 \
		if (unlikely(iser_debug_level > 2))	 \
			printk(KERN_DEBUG PFX "%s: " fmt,\
				__func__ , ## arg);	 \
	} while (0)

#define iser_warn(fmt, arg...)				\
	do {						\
		if (unlikely(iser_debug_level > 0))	\
			pr_warn(PFX "%s: " fmt,		\
				__func__ , ## arg);	\
	} while (0)

#define iser_info(fmt, arg...)				\
	do {						\
		if (unlikely(iser_debug_level > 1))	\
			pr_info(PFX "%s: " fmt,		\
				__func__ , ## arg);	\
	} while (0)

#define iser_err(fmt, arg...) \
	pr_err(PFX "%s: " fmt, __func__ , ## arg)

#define SHIFT_4K	12
#define SIZE_4K	(1ULL << SHIFT_4K)
#define MASK_4K	(~(SIZE_4K-1))

/* Default support is 512KB I/O size */
#define ISER_DEF_MAX_SECTORS		1024
#define ISCSI_ISER_DEF_SG_TABLESIZE	((ISER_DEF_MAX_SECTORS * 512) >> SHIFT_4K)
/* Maximum support is 8MB I/O size */
#define ISCSI_ISER_MAX_SG_TABLESIZE	((16384 * 512) >> SHIFT_4K)

#define ISER_DEF_XMIT_CMDS_DEFAULT		512
#if ISCSI_DEF_XMIT_CMDS_MAX > ISER_DEF_XMIT_CMDS_DEFAULT
	#define ISER_DEF_XMIT_CMDS_MAX		ISCSI_DEF_XMIT_CMDS_MAX
#else
	#define ISER_DEF_XMIT_CMDS_MAX		ISER_DEF_XMIT_CMDS_DEFAULT
#endif
#define ISER_DEF_CMD_PER_LUN		ISER_DEF_XMIT_CMDS_MAX

/* QP settings */
/* Maximal bounds on received asynchronous PDUs */
#define ISER_MAX_RX_MISC_PDUS		4 /* NOOP_IN(2) , ASYNC_EVENT(2)   */

#define ISER_MAX_TX_MISC_PDUS		6 /* NOOP_OUT(2), TEXT(1),         *
					   * SCSI_TMFUNC(2), LOGOUT(1) */

#define ISER_QP_MAX_RECV_DTOS		(ISER_DEF_XMIT_CMDS_MAX)

#define ISER_MIN_POSTED_RX		(ISER_DEF_XMIT_CMDS_MAX >> 2)

/* the max TX (send) WR supported by the iSER QP is defined by                 *
 * max_send_wr = T * (1 + D) + C ; D is how many inflight dataouts we expect   *
 * to have at max for SCSI command. The tx posting & completion handling code  *
 * supports -EAGAIN scheme where tx is suspended till the QP has room for more *
 * send WR. D=8 comes from 64K/8K                                              */

#define ISER_INFLIGHT_DATAOUTS		8

#define ISER_QP_MAX_REQ_DTOS		(ISER_DEF_XMIT_CMDS_MAX *    \
					(1 + ISER_INFLIGHT_DATAOUTS) + \
					ISER_MAX_TX_MISC_PDUS        + \
					ISER_MAX_RX_MISC_PDUS)

/* Max registration work requests per command */
#define ISER_MAX_REG_WR_PER_CMD		5

/* For Signature we don't support DATAOUTs so no need to make room for them */
#define ISER_QP_SIG_MAX_REQ_DTOS	(ISER_DEF_XMIT_CMDS_MAX	*       \
					(1 + ISER_MAX_REG_WR_PER_CMD) + \
					ISER_MAX_TX_MISC_PDUS         + \
					ISER_MAX_RX_MISC_PDUS)

#define ISER_GET_MAX_XMIT_CMDS(send_wr) ((send_wr			\
					 - ISER_MAX_TX_MISC_PDUS	\
					 - ISER_MAX_RX_MISC_PDUS) /	\
					 (1 + ISER_INFLIGHT_DATAOUTS))

#define ISER_WC_BATCH_COUNT   16
#define ISER_SIGNAL_CMD_COUNT 32

#define ISER_VER			0x10
#define ISER_WSV			0x08
#define ISER_RSV			0x04

#define ISER_FASTREG_LI_WRID		0xffffffffffffffffULL
#define ISER_BEACON_WRID		0xfffffffffffffffeULL

/**
 * struct iser_hdr - iSER header
 *
 * @flags:        flags support (zbva, remote_inv)
 * @rsvd:         reserved
 * @write_stag:   write rkey
 * @write_va:     write virtual address
 * @reaf_stag:    read rkey
 * @read_va:      read virtual address
 */
struct iser_hdr {
	u8      flags;
	u8      rsvd[3];
	__be32  write_stag;
	__be64  write_va;
	__be32  read_stag;
	__be64  read_va;
} __attribute__((packed));


#define ISER_ZBVA_NOT_SUPPORTED		0x80
#define ISER_SEND_W_INV_NOT_SUPPORTED	0x40

struct iser_cm_hdr {
	u8      flags;
	u8      rsvd[3];
} __packed;

/* Constant PDU lengths calculations */
#define ISER_HEADERS_LEN  (sizeof(struct iser_hdr) + sizeof(struct iscsi_hdr))

#define ISER_RECV_DATA_SEG_LEN	128
#define ISER_RX_PAYLOAD_SIZE	(ISER_HEADERS_LEN + ISER_RECV_DATA_SEG_LEN)
#define ISER_RX_LOGIN_SIZE	(ISER_HEADERS_LEN + ISCSI_DEF_MAX_RECV_SEG_LEN)

/* Length of an object name string */
#define ISER_OBJECT_NAME_SIZE		    64

enum iser_conn_state {
	ISER_CONN_INIT,		   /* descriptor allocd, no conn          */
	ISER_CONN_PENDING,	   /* in the process of being established */
	ISER_CONN_UP,		   /* up and running                      */
	ISER_CONN_TERMINATING,	   /* in the process of being terminated  */
	ISER_CONN_DOWN,		   /* shut down                           */
	ISER_CONN_STATES_NUM
};

enum iser_task_status {
	ISER_TASK_STATUS_INIT = 0,
	ISER_TASK_STATUS_STARTED,
	ISER_TASK_STATUS_COMPLETED
};

enum iser_data_dir {
	ISER_DIR_IN = 0,	   /* to initiator */
	ISER_DIR_OUT,		   /* from initiator */
	ISER_DIRS_NUM
};

/**
 * struct iser_data_buf - iSER data buffer
 *
 * @sg:           pointer to the sg list
 * @size:         num entries of this sg
 * @data_len:     total beffer byte len
 * @dma_nents:    returned by dma_map_sg
 */
struct iser_data_buf {
	struct scatterlist *sg;
	int                size;
	unsigned long      data_len;
	unsigned int       dma_nents;
};

/* fwd declarations */
struct iser_device;
struct iscsi_iser_task;
struct iscsi_endpoint;
struct iser_reg_resources;

/**
 * struct iser_mem_reg - iSER memory registration info
 *
 * @sge:          memory region sg element
 * @rkey:         memory region remote key
 * @mem_h:        pointer to registration context (FMR/Fastreg)
 */
struct iser_mem_reg {
	struct ib_sge	 sge;
	u32		 rkey;
	void		*mem_h;
};

enum iser_desc_type {
	ISCSI_TX_CONTROL ,
	ISCSI_TX_SCSI_COMMAND,
	ISCSI_TX_DATAOUT
};

/* Maximum number of work requests per task:
 * Data memory region local invalidate + fast registration
 * Protection memory region local invalidate + fast registration
 * Signature memory region local invalidate + fast registration
 * PDU send
 */
#define ISER_MAX_WRS 7

/**
 * struct iser_tx_desc - iSER TX descriptor (for send wr_id)
 *
 * @iser_header:   iser header
 * @iscsi_header:  iscsi header
 * @type:          command/control/dataout
 * @dam_addr:      header buffer dma_address
 * @tx_sg:         sg[0] points to iser/iscsi headers
 *                 sg[1] optionally points to either of immediate data
 *                 unsolicited data-out or control
 * @num_sge:       number sges used on this TX task
 * @mapped:        Is the task header mapped
 * @wr_idx:        Current WR index
 * @wrs:           Array of WRs per task
 * @data_reg:      Data buffer registration details
 * @prot_reg:      Protection buffer registration details
 * @sig_attrs:     Signature attributes
 */
struct iser_tx_desc {
	struct iser_hdr              iser_header;
	struct iscsi_hdr             iscsi_header;
	enum   iser_desc_type        type;
	u64		             dma_addr;
	struct ib_sge		     tx_sg[2];
	int                          num_sge;
	bool			     mapped;
	u8                           wr_idx;
	union iser_wr {
		struct ib_send_wr		send;
		struct ib_reg_wr		fast_reg;
		struct ib_sig_handover_wr	sig;
	} wrs[ISER_MAX_WRS];
	struct iser_mem_reg          data_reg;
	struct iser_mem_reg          prot_reg;
	struct ib_sig_attrs          sig_attrs;
};

#define ISER_RX_PAD_SIZE	(256 - (ISER_RX_PAYLOAD_SIZE + \
					sizeof(u64) + sizeof(struct ib_sge)))
/**
 * struct iser_rx_desc - iSER RX descriptor (for recv wr_id)
 *
 * @iser_header:   iser header
 * @iscsi_header:  iscsi header
 * @data:          received data segment
 * @dma_addr:      receive buffer dma address
 * @rx_sg:         ib_sge of receive buffer
 * @pad:           for sense data TODO: Modify to maximum sense length supported
 */
struct iser_rx_desc {
	struct iser_hdr              iser_header;
	struct iscsi_hdr             iscsi_header;
	char		             data[ISER_RECV_DATA_SEG_LEN];
	u64		             dma_addr;
	struct ib_sge		     rx_sg;
	char		             pad[ISER_RX_PAD_SIZE];
} __attribute__((packed));

struct iser_conn;
struct ib_conn;
struct iscsi_iser_task;

/**
 * struct iser_comp - iSER completion context
 *
 * @device:     pointer to device handle
 * @cq:         completion queue
 * @wcs:        work completion array
 * @tasklet:    Tasklet handle
 * @active_qps: Number of active QPs attached
 *              to completion context
 */
struct iser_comp {
	struct iser_device      *device;
	struct ib_cq		*cq;
	struct ib_wc		 wcs[ISER_WC_BATCH_COUNT];
	struct tasklet_struct	 tasklet;
	int                      active_qps;
};

/**
 * struct iser_device - Memory registration operations
 *     per-device registration schemes
 *
 * @alloc_reg_res:     Allocate registration resources
 * @free_reg_res:      Free registration resources
 * @fast_reg_mem:      Register memory buffers
 * @unreg_mem:         Un-register memory buffers
 * @reg_desc_get:      Get a registration descriptor for pool
 * @reg_desc_put:      Get a registration descriptor to pool
 */
struct iser_reg_ops {
	int            (*alloc_reg_res)(struct ib_conn *ib_conn,
					unsigned cmds_max,
					unsigned int size);
	void           (*free_reg_res)(struct ib_conn *ib_conn);
	int            (*reg_mem)(struct iscsi_iser_task *iser_task,
				  struct iser_data_buf *mem,
				  struct iser_reg_resources *rsc,
				  struct iser_mem_reg *reg);
	void           (*unreg_mem)(struct iscsi_iser_task *iser_task,
				    enum iser_data_dir cmd_dir);
	struct iser_fr_desc * (*reg_desc_get)(struct ib_conn *ib_conn);
	void           (*reg_desc_put)(struct ib_conn *ib_conn,
				       struct iser_fr_desc *desc);
};

/**
 * struct iser_device - iSER device handle
 *
 * @ib_device:     RDMA device
 * @pd:            Protection Domain for this device
 * @dev_attr:      Device attributes container
 * @mr:            Global DMA memory region
 * @event_handler: IB events handle routine
 * @ig_list:	   entry in devices list
 * @refcount:      Reference counter, dominated by open iser connections
 * @comps_used:    Number of completion contexts used, Min between online
 *                 cpus and device max completion vectors
 * @comps:         Dinamically allocated array of completion handlers
 * @reg_ops:       Registration ops
 */
struct iser_device {
	struct ib_device             *ib_device;
	struct ib_pd	             *pd;
	struct ib_device_attr	     dev_attr;
	struct ib_mr	             *mr;
	struct ib_event_handler      event_handler;
	struct list_head             ig_list;
	int                          refcount;
	int			     comps_used;
	struct iser_comp	     *comps;
	struct iser_reg_ops          *reg_ops;
};

#define ISER_CHECK_GUARD	0xc0
#define ISER_CHECK_REFTAG	0x0f
#define ISER_CHECK_APPTAG	0x30

/**
 * struct iser_reg_resources - Fast registration recources
 *
 * @mr:         memory region
 * @fmr_pool:   pool of fmrs
 * @page_vec:   fast reg page list used by fmr pool
 * @mr_valid:   is mr valid indicator
 */
struct iser_reg_resources {
	union {
		struct ib_mr             *mr;
		struct ib_fmr_pool       *fmr_pool;
	};
	struct iser_page_vec             *page_vec;
	u8				  mr_valid:1;
};

/**
 * struct iser_pi_context - Protection information context
 *
 * @rsc:             protection buffer registration resources
 * @sig_mr:          signature enable memory region
 * @sig_mr_valid:    is sig_mr valid indicator
 * @sig_protected:   is region protected indicator
 */
struct iser_pi_context {
	struct iser_reg_resources	rsc;
	struct ib_mr                   *sig_mr;
	u8                              sig_mr_valid:1;
	u8                              sig_protected:1;
};

/**
 * struct iser_fr_desc - Fast registration descriptor
 *
 * @list:           entry in connection fastreg pool
 * @rsc:            data buffer registration resources
 * @pi_ctx:         protection information context
 */
struct iser_fr_desc {
	struct list_head		  list;
	struct iser_reg_resources	  rsc;
	struct iser_pi_context		 *pi_ctx;
	struct list_head                  all_list;
};

/**
 * struct iser_fr_pool: connection fast registration pool
 *
 * @list:                list of fastreg descriptors
 * @lock:                protects fmr/fastreg pool
 * @size:                size of the pool
 */
struct iser_fr_pool {
	struct list_head        list;
	spinlock_t              lock;
	int                     size;
	struct list_head        all_list;
};

/**
 * struct ib_conn - Infiniband related objects
 *
 * @cma_id:              rdma_cm connection maneger handle
 * @qp:                  Connection Queue-pair
 * @post_recv_buf_count: post receive counter
 * @sig_count:           send work request signal count
 * @rx_wr:               receive work request for batch posts
 * @device:              reference to iser device
 * @comp:                iser completion context
 * @pi_support:          Indicate device T10-PI support
 * @beacon:              beacon send wr to signal all flush errors were drained
 * @flush_comp:          completes when all connection completions consumed
 * @fr_pool:             connection fast registration poool
 */
struct ib_conn {
	struct rdma_cm_id           *cma_id;
	struct ib_qp	            *qp;
	int                          post_recv_buf_count;
	u8                           sig_count;
	struct ib_recv_wr	     rx_wr[ISER_MIN_POSTED_RX];
	struct iser_device          *device;
	struct iser_comp	    *comp;
	bool			     pi_support;
	struct ib_send_wr	     beacon;
	struct completion	     flush_comp;
	struct iser_fr_pool          fr_pool;
};

/**
 * struct iser_conn - iSER connection context
 *
 * @ib_conn:          connection RDMA resources
 * @iscsi_conn:       link to matching iscsi connection
 * @ep:               transport handle
 * @state:            connection logical state
 * @qp_max_recv_dtos: maximum number of data outs, corresponds
 *                    to max number of post recvs
 * @qp_max_recv_dtos_mask: (qp_max_recv_dtos - 1)
 * @min_posted_rx:    (qp_max_recv_dtos >> 2)
 * @max_cmds:         maximum cmds allowed for this connection
 * @name:             connection peer portal
 * @release_work:     deffered work for release job
 * @state_mutex:      protects iser onnection state
 * @stop_completion:  conn_stop completion
 * @ib_completion:    RDMA cleanup completion
 * @up_completion:    connection establishment completed
 *                    (state is ISER_CONN_UP)
 * @conn_list:        entry in ig conn list
 * @login_buf:        login data buffer (stores login parameters)
 * @login_req_buf:    login request buffer
 * @login_req_dma:    login request buffer dma address
 * @login_resp_buf:   login response buffer
 * @login_resp_dma:   login response buffer dma address
 * @rx_desc_head:     head of rx_descs cyclic buffer
 * @rx_descs:         rx buffers array (cyclic buffer)
 * @num_rx_descs:     number of rx descriptors
 * @scsi_sg_tablesize: scsi host sg_tablesize
 * @scsi_max_sectors: scsi host max sectors
 */
struct iser_conn {
	struct ib_conn		     ib_conn;
	struct iscsi_conn	     *iscsi_conn;
	struct iscsi_endpoint	     *ep;
	enum iser_conn_state	     state;
	unsigned		     qp_max_recv_dtos;
	unsigned		     qp_max_recv_dtos_mask;
	unsigned		     min_posted_rx;
	u16                          max_cmds;
	char 			     name[ISER_OBJECT_NAME_SIZE];
	struct work_struct	     release_work;
	struct mutex		     state_mutex;
	struct completion	     stop_completion;
	struct completion	     ib_completion;
	struct completion	     up_completion;
	struct list_head	     conn_list;

	char  			     *login_buf;
	char			     *login_req_buf, *login_resp_buf;
	u64			     login_req_dma, login_resp_dma;
	unsigned int 		     rx_desc_head;
	struct iser_rx_desc	     *rx_descs;
	u32                          num_rx_descs;
	unsigned short               scsi_sg_tablesize;
	unsigned int                 scsi_max_sectors;
};

/**
 * struct iscsi_iser_task - iser task context
 *
 * @desc:     TX descriptor
 * @iser_conn:        link to iser connection
 * @status:           current task status
 * @sc:               link to scsi command
 * @command_sent:     indicate if command was sent
 * @dir:              iser data direction
 * @rdma_reg:         task rdma registration desc
 * @data:             iser data buffer desc
 * @prot:             iser protection buffer desc
 */
struct iscsi_iser_task {
	struct iser_tx_desc          desc;
	struct iser_conn	     *iser_conn;
	enum iser_task_status 	     status;
	struct scsi_cmnd	     *sc;
	int                          command_sent;
	int                          dir[ISER_DIRS_NUM];
	struct iser_mem_reg          rdma_reg[ISER_DIRS_NUM];
	struct iser_data_buf         data[ISER_DIRS_NUM];
	struct iser_data_buf         prot[ISER_DIRS_NUM];
};

struct iser_page_vec {
	u64 *pages;
	int length;
	int offset;
	int data_size;
};

/**
 * struct iser_global: iSER global context
 *
 * @device_list_mutex:    protects device_list
 * @device_list:          iser devices global list
 * @connlist_mutex:       protects connlist
 * @connlist:             iser connections global list
 * @desc_cache:           kmem cache for tx dataout
 */
struct iser_global {
	struct mutex      device_list_mutex;
	struct list_head  device_list;
	struct mutex      connlist_mutex;
	struct list_head  connlist;
	struct kmem_cache *desc_cache;
};

extern struct iser_global ig;
extern int iser_debug_level;
extern bool iser_pi_enable;
extern int iser_pi_guard;
extern unsigned int iser_max_sectors;
extern bool iser_always_reg;

int iser_assign_reg_ops(struct iser_device *device);

int iser_send_control(struct iscsi_conn *conn,
		      struct iscsi_task *task);

int iser_send_command(struct iscsi_conn *conn,
		      struct iscsi_task *task);

int iser_send_data_out(struct iscsi_conn *conn,
		       struct iscsi_task *task,
		       struct iscsi_data *hdr);

void iscsi_iser_recv(struct iscsi_conn *conn,
		     struct iscsi_hdr *hdr,
		     char *rx_data,
		     int rx_data_len);

void iser_conn_init(struct iser_conn *iser_conn);

void iser_conn_release(struct iser_conn *iser_conn);

int iser_conn_terminate(struct iser_conn *iser_conn);

void iser_release_work(struct work_struct *work);

void iser_rcv_completion(struct iser_rx_desc *desc,
			 unsigned long dto_xfer_len,
			 struct ib_conn *ib_conn);

void iser_snd_completion(struct iser_tx_desc *desc,
			 struct ib_conn *ib_conn);

void iser_task_rdma_init(struct iscsi_iser_task *task);

void iser_task_rdma_finalize(struct iscsi_iser_task *task);

void iser_free_rx_descriptors(struct iser_conn *iser_conn);

void iser_finalize_rdma_unaligned_sg(struct iscsi_iser_task *iser_task,
				     struct iser_data_buf *mem,
				     enum iser_data_dir cmd_dir);

int iser_reg_rdma_mem(struct iscsi_iser_task *task,
		      enum iser_data_dir dir);
void iser_unreg_rdma_mem(struct iscsi_iser_task *task,
			 enum iser_data_dir dir);

int  iser_connect(struct iser_conn *iser_conn,
		  struct sockaddr *src_addr,
		  struct sockaddr *dst_addr,
		  int non_blocking);

void iser_unreg_mem_fmr(struct iscsi_iser_task *iser_task,
			enum iser_data_dir cmd_dir);
void iser_unreg_mem_fastreg(struct iscsi_iser_task *iser_task,
			    enum iser_data_dir cmd_dir);

int  iser_post_recvl(struct iser_conn *iser_conn);
int  iser_post_recvm(struct iser_conn *iser_conn, int count);
int  iser_post_send(struct ib_conn *ib_conn, struct iser_tx_desc *tx_desc,
		    bool signal);

int iser_dma_map_task_data(struct iscsi_iser_task *iser_task,
			   struct iser_data_buf *data,
			   enum iser_data_dir iser_dir,
			   enum dma_data_direction dma_dir);

void iser_dma_unmap_task_data(struct iscsi_iser_task *iser_task,
			      struct iser_data_buf *data,
			      enum dma_data_direction dir);

int  iser_initialize_task_headers(struct iscsi_task *task,
			struct iser_tx_desc *tx_desc);
int iser_alloc_rx_descriptors(struct iser_conn *iser_conn,
			      struct iscsi_session *session);
int iser_alloc_fmr_pool(struct ib_conn *ib_conn,
			unsigned cmds_max,
			unsigned int size);
void iser_free_fmr_pool(struct ib_conn *ib_conn);
int iser_alloc_fastreg_pool(struct ib_conn *ib_conn,
			    unsigned cmds_max,
			    unsigned int size);
void iser_free_fastreg_pool(struct ib_conn *ib_conn);
u8 iser_check_task_pi_status(struct iscsi_iser_task *iser_task,
			     enum iser_data_dir cmd_dir, sector_t *sector);
struct iser_fr_desc *
iser_reg_desc_get_fr(struct ib_conn *ib_conn);
void
iser_reg_desc_put_fr(struct ib_conn *ib_conn,
		     struct iser_fr_desc *desc);
struct iser_fr_desc *
iser_reg_desc_get_fmr(struct ib_conn *ib_conn);
void
iser_reg_desc_put_fmr(struct ib_conn *ib_conn,
		      struct iser_fr_desc *desc);

static inline struct ib_send_wr *
iser_tx_next_wr(struct iser_tx_desc *tx_desc)
{
	struct ib_send_wr *cur_wr = &tx_desc->wrs[tx_desc->wr_idx].send;
	struct ib_send_wr *last_wr;

	if (tx_desc->wr_idx) {
		last_wr = &tx_desc->wrs[tx_desc->wr_idx - 1].send;
		last_wr->next = cur_wr;
	}
	tx_desc->wr_idx++;

	return cur_wr;
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * Copyright (c) 2004, 2005, 2006 Voltaire, Inc. All rights reserved.
 * Copyright (c) 2013-2014 Mellanox Technologies. All rights reserved.
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
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
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
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/kfifo.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_host.h>

#include "iscsi_iser.h"

/* Register user buffer memory and initialize passive rdma
 *  dto descriptor. Data size is stored in
 *  task->data[ISER_DIR_IN].data_len, Protection size
 *  os stored in task->prot[ISER_DIR_IN].data_len
 */
static int iser_prepare_read_cmd(struct iscsi_task *task)

{
	struct iscsi_iser_task *iser_task = task->dd_data;
	struct iser_mem_reg *mem_reg;
	int err;
	struct iser_hdr *hdr = &iser_task->desc.iser_header;
	struct iser_data_buf *buf_in = &iser_task->data[ISER_DIR_IN];

	err = iser_dma_map_task_data(iser_task,
				     buf_in,
				     ISER_DIR_IN,
				     DMA_FROM_DEVICE);
	if (err)
		return err;

	if (scsi_prot_sg_count(iser_task->sc)) {
		struct iser_data_buf *pbuf_in = &iser_task->prot[ISER_DIR_IN];

		err = iser_dma_map_task_data(iser_task,
					     pbuf_in,
					     ISER_DIR_IN,
					     DMA_FROM_DEVICE);
		if (err)
			return err;
	}

	err = iser_reg_rdma_mem(iser_task, ISER_DIR_IN);
	if (err) {
		iser_err("Failed to set up Data-IN RDMA\n");
		return err;
	}
	mem_reg = &iser_task->rdma_reg[ISER_DIR_IN];

	hdr->flags    |= ISER_RSV;
	hdr->read_stag = cpu_to_be32(mem_reg->rkey);
	hdr->read_va   = cpu_to_be64(mem_reg->sge.addr);

	iser_dbg("Cmd itt:%d READ tags RKEY:%#.4X VA:%#llX\n",
		 task->itt, mem_reg->rkey,
		 (unsigned long long)mem_reg->sge.addr);

	return 0;
}

/* Register user buffer memory and initialize passive rdma
 *  dto descriptor. Data size is stored in
 *  task->data[ISER_DIR_OUT].data_len, Protection size
 *  is stored at task->prot[ISER_DIR_OUT].data_len
 */
static int
iser_prepare_write_cmd(struct iscsi_task *task,
		       unsigned int imm_sz,
		       unsigned int unsol_sz,
		       unsigned int edtl)
{
	struct iscsi_iser_task *iser_task = task->dd_data;
	struct iser_mem_reg *mem_reg;
	int err;
	struct iser_hdr *hdr = &iser_task->desc.iser_header;
	struct iser_data_buf *buf_out = &iser_task->data[ISER_DIR_OUT];
	struct ib_sge *tx_dsg = &iser_task->desc.tx_sg[1];

	err = iser_dma_map_task_data(iser_task,
				     buf_out,
				     ISER_DIR_OUT,
				     DMA_TO_DEVICE);
	if (err)
		return err;

	if (scsi_prot_sg_count(iser_task->sc)) {
		struct iser_data_buf *pbuf_out = &iser_task->prot[ISER_DIR_OUT];

		err = iser_dma_map_task_data(iser_task,
					     pbuf_out,
					     ISER_DIR_OUT,
					     DMA_TO_DEVICE);
		if (err)
			return err;
	}

	err = iser_reg_rdma_mem(iser_task, ISER_DIR_OUT);
	if (err != 0) {
		iser_err("Failed to register write cmd RDMA mem\n");
		return err;
	}

	mem_reg = &iser_task->rdma_reg[ISER_DIR_OUT];

	if (unsol_sz < edtl) {
		hdr->flags     |= ISER_WSV;
		hdr->write_stag = cpu_to_be32(mem_reg->rkey);
		hdr->write_va   = cpu_to_be64(mem_reg->sge.addr + unsol_sz);

		iser_dbg("Cmd itt:%d, WRITE tags, RKEY:%#.4X "
			 "VA:%#llX + unsol:%d\n",
			 task->itt, mem_reg->rkey,
			 (unsigned long long)mem_reg->sge.addr, unsol_sz);
	}

	if (imm_sz > 0) {
		iser_dbg("Cmd itt:%d, WRITE, adding imm.data sz: %d\n",
			 task->itt, imm_sz);
		tx_dsg->addr = mem_reg->sge.addr;
		tx_dsg->length = imm_sz;
		tx_dsg->lkey = mem_reg->sge.lkey;
		iser_task->desc.num_sge = 2;
	}

	return 0;
}

/* creates a new tx descriptor and adds header regd buffer */
static void iser_create_send_desc(struct iser_conn	*iser_conn,
				  struct iser_tx_desc	*tx_desc)
{
	struct iser_device *device = iser_conn->ib_conn.device;

	ib_dma_sync_single_for_cpu(device->ib_device,
		tx_desc->dma_addr, ISER_HEADERS_LEN, DMA_TO_DEVICE);

	memset(&tx_desc->iser_header, 0, sizeof(struct iser_hdr));
	tx_desc->iser_header.flags = ISER_VER;
	tx_desc->num_sge = 1;
}

static void iser_free_login_buf(struct iser_conn *iser_conn)
{
	struct iser_device *device = iser_conn->ib_conn.device;

	if (!iser_conn->login_buf)
		return;

	if (iser_conn->login_req_dma)
		ib_dma_unmap_single(device->ib_device,
				    iser_conn->login_req_dma,
				    ISCSI_DEF_MAX_RECV_SEG_LEN, DMA_TO_DEVICE);

	if (iser_conn->login_resp_dma)
		ib_dma_unmap_single(device->ib_device,
				    iser_conn->login_resp_dma,
				    ISER_RX_LOGIN_SIZE, DMA_FROM_DEVICE);

	kfree(iser_conn->login_buf);

	/* make sure we never redo any unmapping */
	iser_conn->login_req_dma = 0;
	iser_conn->login_resp_dma = 0;
	iser_conn->login_buf = NULL;
}

static int iser_alloc_login_buf(struct iser_conn *iser_conn)
{
	struct iser_device *device = iser_conn->ib_conn.device;
	int			req_err, resp_err;

	BUG_ON(device == NULL);

	iser_conn->login_buf = kmalloc(ISCSI_DEF_MAX_RECV_SEG_LEN +
				     ISER_RX_LOGIN_SIZE, GFP_KERNEL);
	if (!iser_conn->login_buf)
		goto out_err;

	iser_conn->login_req_buf  = iser_conn->login_buf;
	iser_conn->login_resp_buf = iser_conn->login_buf +
						ISCSI_DEF_MAX_RECV_SEG_LEN;

	iser_conn->login_req_dma = ib_dma_map_single(device->ib_device,
						     iser_conn->login_req_buf,
						     ISCSI_DEF_MAX_RECV_SEG_LEN,
						     DMA_TO_DEVICE);

	iser_conn->login_resp_dma = ib_dma_map_single(device->ib_device,
						      iser_conn->login_resp_buf,
						      ISER_RX_LOGIN_SIZE,
						      DMA_FROM_DEVICE);

	req_err  = ib_dma_mapping_error(device->ib_device,
					iser_conn->login_req_dma);
	resp_err = ib_dma_mapping_error(device->ib_device,
					iser_conn->login_resp_dma);

	if (req_err || resp_err) {
		if (req_err)
			iser_conn->login_req_dma = 0;
		if (resp_err)
			iser_conn->login_resp_dma = 0;
		goto free_login_buf;
	}
	return 0;

free_login_buf:
	iser_free_login_buf(iser_conn);

out_err:
	iser_err("unable to alloc or map login buf\n");
	return -ENOMEM;
}

int iser_alloc_rx_descriptors(struct iser_conn *iser_conn,
			      struct iscsi_session *session)
{
	int i, j;
	u64 dma_addr;
	struct iser_rx_desc *rx_desc;
	struct ib_sge       *rx_sg;
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	struct iser_device *device = ib_conn->device;

	iser_conn->qp_max_recv_dtos = session->cmds_max;
	iser_conn->qp_max_recv_dtos_mask = session->cmds_max - 1; /* cmds_max is 2^N */
	iser_conn->min_posted_rx = iser_conn->qp_max_recv_dtos >> 2;

	if (device->reg_ops->alloc_reg_res(ib_conn, session->scsi_cmds_max,
					   iser_conn->scsi_sg_tablesize))
		goto create_rdma_reg_res_failed;

	if (iser_alloc_login_buf(iser_conn))
		goto alloc_login_buf_fail;

	iser_conn->num_rx_descs = session->cmds_max;
	iser_conn->rx_descs = kmalloc(iser_conn->num_rx_descs *
				sizeof(struct iser_rx_desc), GFP_KERNEL);
	if (!iser_conn->rx_descs)
		goto rx_desc_alloc_fail;

	rx_desc = iser_conn->rx_descs;

	for (i = 0; i < iser_conn->qp_max_recv_dtos; i++, rx_desc++)  {
		dma_addr = ib_dma_map_single(device->ib_device, (void *)rx_desc,
					ISER_RX_PAYLOAD_SIZE, DMA_FROM_DEVICE);
		if (ib_dma_mapping_error(device->ib_device, dma_addr))
			goto rx_desc_dma_map_failed;

		rx_desc->dma_addr = dma_addr;

		rx_sg = &rx_desc->rx_sg;
		rx_sg->addr   = rx_desc->dma_addr;
		rx_sg->length = ISER_RX_PAYLOAD_SIZE;
		rx_sg->lkey   = device->pd->local_dma_lkey;
	}

	iser_conn->rx_desc_head = 0;
	return 0;

rx_desc_dma_map_failed:
	rx_desc = iser_conn->rx_descs;
	for (j = 0; j < i; j++, rx_desc++)
		ib_dma_unmap_single(device->ib_device, rx_desc->dma_addr,
				    ISER_RX_PAYLOAD_SIZE, DMA_FROM_DEVICE);
	kfree(iser_conn->rx_descs);
	iser_conn->rx_descs = NULL;
rx_desc_alloc_fail:
	iser_free_login_buf(iser_conn);
alloc_login_buf_fail:
	device->reg_ops->free_reg_res(ib_conn);
create_rdma_reg_res_failed:
	iser_err("failed allocating rx descriptors / data buffers\n");
	return -ENOMEM;
}

void iser_free_rx_descriptors(struct iser_conn *iser_conn)
{
	int i;
	struct iser_rx_desc *rx_desc;
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	struct iser_device *device = ib_conn->device;

	if (device->reg_ops->free_reg_res)
		device->reg_ops->free_reg_res(ib_conn);

	rx_desc = iser_conn->rx_descs;
	for (i = 0; i < iser_conn->qp_max_recv_dtos; i++, rx_desc++)
		ib_dma_unmap_single(device->ib_device, rx_desc->dma_addr,
				    ISER_RX_PAYLOAD_SIZE, DMA_FROM_DEVICE);
	kfree(iser_conn->rx_descs);
	/* make sure we never redo any unmapping */
	iser_conn->rx_descs = NULL;

	iser_free_login_buf(iser_conn);
}

static int iser_post_rx_bufs(struct iscsi_conn *conn, struct iscsi_hdr *req)
{
	struct iser_conn *iser_conn = conn->dd_data;
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	struct iscsi_session *session = conn->session;

	iser_dbg("req op %x flags %x\n", req->opcode, req->flags);
	/* check if this is the last login - going to full feature phase */
	if ((req->flags & ISCSI_FULL_FEATURE_PHASE) != ISCSI_FULL_FEATURE_PHASE)
		return 0;

	/*
	 * Check that there is one posted recv buffer
	 * (for the last login response).
	 */
	WARN_ON(ib_conn->post_recv_buf_count != 1);

	if (session->discovery_sess) {
		iser_info("Discovery session, re-using login RX buffer\n");
		return 0;
	} else
		iser_info("Normal session, posting batch of RX %d buffers\n",
			  iser_conn->min_posted_rx);

	/* Initial post receive buffers */
	if (iser_post_recvm(iser_conn, iser_conn->min_posted_rx))
		return -ENOMEM;

	return 0;
}

static inline bool iser_signal_comp(u8 sig_count)
{
	return ((sig_count % ISER_SIGNAL_CMD_COUNT) == 0);
}

/**
 * iser_send_command - send command PDU
 */
int iser_send_command(struct iscsi_conn *conn,
		      struct iscsi_task *task)
{
	struct iser_conn *iser_conn = conn->dd_data;
	struct iscsi_iser_task *iser_task = task->dd_data;
	unsigned long edtl;
	int err;
	struct iser_data_buf *data_buf, *prot_buf;
	struct iscsi_scsi_req *hdr = (struct iscsi_scsi_req *)task->hdr;
	struct scsi_cmnd *sc  =  task->sc;
	struct iser_tx_desc *tx_desc = &iser_task->desc;
	u8 sig_count = ++iser_conn->ib_conn.sig_count;

	edtl = ntohl(hdr->data_length);

	/* build the tx desc regd header and add it to the tx desc dto */
	tx_desc->type = ISCSI_TX_SCSI_COMMAND;
	iser_create_send_desc(iser_conn, tx_desc);

	if (hdr->flags & ISCSI_FLAG_CMD_READ) {
		data_buf = &iser_task->data[ISER_DIR_IN];
		prot_buf = &iser_task->prot[ISER_DIR_IN];
	} else {
		data_buf = &iser_task->data[ISER_DIR_OUT];
		prot_buf = &iser_task->prot[ISER_DIR_OUT];
	}

	if (scsi_sg_count(sc)) { /* using a scatter list */
		data_buf->sg = scsi_sglist(sc);
		data_buf->size = scsi_sg_count(sc);
	}
	data_buf->data_len = scsi_bufflen(sc);

	if (scsi_prot_sg_count(sc)) {
		prot_buf->sg  = scsi_prot_sglist(sc);
		prot_buf->size = scsi_prot_sg_count(sc);
		prot_buf->data_len = (data_buf->data_len >>
				     ilog2(sc->device->sector_size)) * 8;
	}

	if (hdr->flags & ISCSI_FLAG_CMD_READ) {
		err = iser_prepare_read_cmd(task);
		if (err)
			goto send_command_error;
	}
	if (hdr->flags & ISCSI_FLAG_CMD_WRITE) {
		err = iser_prepare_write_cmd(task,
					     task->imm_count,
				             task->imm_count +
					     task->unsol_r2t.data_length,
					     edtl);
		if (err)
			goto send_command_error;
	}

	iser_task->status = ISER_TASK_STATUS_STARTED;

	err = iser_post_send(&iser_conn->ib_conn, tx_desc,
			     iser_signal_comp(sig_count));
	if (!err)
		return 0;

send_command_error:
	iser_err("conn %p failed task->itt %d err %d\n",conn, task->itt, err);
	return err;
}

/**
 * iser_send_data_out - send data out PDU
 */
int iser_send_data_out(struct iscsi_conn *conn,
		       struct iscsi_task *task,
		       struct iscsi_data *hdr)
{
	struct iser_conn *iser_conn = conn->dd_data;
	struct iscsi_iser_task *iser_task = task->dd_data;
	struct iser_tx_desc *tx_desc = NULL;
	struct iser_mem_reg *mem_reg;
	unsigned long buf_offset;
	unsigned long data_seg_len;
	uint32_t itt;
	int err;
	struct ib_sge *tx_dsg;

	itt = (__force uint32_t)hdr->itt;
	data_seg_len = ntoh24(hdr->dlength);
	buf_offset   = ntohl(hdr->offset);

	iser_dbg("%s itt %d dseg_len %d offset %d\n",
		 __func__,(int)itt,(int)data_seg_len,(int)buf_offset);

	tx_desc = kmem_cache_zalloc(ig.desc_cache, GFP_ATOMIC);
	if (tx_desc == NULL) {
		iser_err("Failed to alloc desc for post dataout\n");
		return -ENOMEM;
	}

	tx_desc->type = ISCSI_TX_DATAOUT;
	tx_desc->iser_header.flags = ISER_VER;
	memcpy(&tx_desc->iscsi_header, hdr, sizeof(struct iscsi_hdr));

	/* build the tx desc */
	err = iser_initialize_task_headers(task, tx_desc);
	if (err)
		goto send_data_out_error;

	mem_reg = &iser_task->rdma_reg[ISER_DIR_OUT];
	tx_dsg = &tx_desc->tx_sg[1];
	tx_dsg->addr = mem_reg->sge.addr + buf_offset;
	tx_dsg->length = data_seg_len;
	tx_dsg->lkey = mem_reg->sge.lkey;
	tx_desc->num_sge = 2;

	if (buf_offset + data_seg_len > iser_task->data[ISER_DIR_OUT].data_len) {
		iser_err("Offset:%ld & DSL:%ld in Data-Out "
			 "inconsistent with total len:%ld, itt:%d\n",
			 buf_offset, data_seg_len,
			 iser_task->data[ISER_DIR_OUT].data_len, itt);
		err = -EINVAL;
		goto send_data_out_error;
	}
	iser_dbg("data-out itt: %d, offset: %ld, sz: %ld\n",
		 itt, buf_offset, data_seg_len);


	err = iser_post_send(&iser_conn->ib_conn, tx_desc, true);
	if (!err)
		return 0;

send_data_out_error:
	kmem_cache_free(ig.desc_cache, tx_desc);
	iser_err("conn %p failed err %d\n", conn, err);
	return err;
}

int iser_send_control(struct iscsi_conn *conn,
		      struct iscsi_task *task)
{
	struct iser_conn *iser_conn = conn->dd_data;
	struct iscsi_iser_task *iser_task = task->dd_data;
	struct iser_tx_desc *mdesc = &iser_task->desc;
	unsigned long data_seg_len;
	int err = 0;
	struct iser_device *device;

	/* build the tx desc regd header and add it to the tx desc dto */
	mdesc->type = ISCSI_TX_CONTROL;
	iser_create_send_desc(iser_conn, mdesc);

	device = iser_conn->ib_conn.device;

	data_seg_len = ntoh24(task->hdr->dlength);

	if (data_seg_len > 0) {
		struct ib_sge *tx_dsg = &mdesc->tx_sg[1];
		if (task != conn->login_task) {
			iser_err("data present on non login task!!!\n");
			goto send_control_error;
		}

		ib_dma_sync_single_for_cpu(device->ib_device,
			iser_conn->login_req_dma, task->data_count,
			DMA_TO_DEVICE);

		memcpy(iser_conn->login_req_buf, task->data, task->data_count);

		ib_dma_sync_single_for_device(device->ib_device,
			iser_conn->login_req_dma, task->data_count,
			DMA_TO_DEVICE);

		tx_dsg->addr    = iser_conn->login_req_dma;
		tx_dsg->length  = task->data_count;
		tx_dsg->lkey    = device->pd->local_dma_lkey;
		mdesc->num_sge = 2;
	}

	if (task == conn->login_task) {
		iser_dbg("op %x dsl %lx, posting login rx buffer\n",
			 task->hdr->opcode, data_seg_len);
		err = iser_post_recvl(iser_conn);
		if (err)
			goto send_control_error;
		err = iser_post_rx_bufs(conn, task->hdr);
		if (err)
			goto send_control_error;
	}

	err = iser_post_send(&iser_conn->ib_conn, mdesc, true);
	if (!err)
		return 0;

send_control_error:
	iser_err("conn %p failed err %d\n",conn, err);
	return err;
}

/**
 * iser_rcv_dto_completion - recv DTO completion
 */
void iser_rcv_completion(struct iser_rx_desc *rx_desc,
			 unsigned long rx_xfer_len,
			 struct ib_conn *ib_conn)
{
	struct iser_conn *iser_conn = container_of(ib_conn, struct iser_conn,
						   ib_conn);
	struct iscsi_hdr *hdr;
	u64 rx_dma;
	int rx_buflen, outstanding, count, err;

	/* differentiate between login to all other PDUs */
	if ((char *)rx_desc == iser_conn->login_resp_buf) {
		rx_dma = iser_conn->login_resp_dma;
		rx_buflen = ISER_RX_LOGIN_SIZE;
	} else {
		rx_dma = rx_desc->dma_addr;
		rx_buflen = ISER_RX_PAYLOAD_SIZE;
	}

	ib_dma_sync_single_for_cpu(ib_conn->device->ib_device, rx_dma,
				   rx_buflen, DMA_FROM_DEVICE);

	hdr = &rx_desc->iscsi_header;

	iser_dbg("op 0x%x itt 0x%x dlen %d\n", hdr->opcode,
			hdr->itt, (int)(rx_xfer_len - ISER_HEADERS_LEN));

	iscsi_iser_recv(iser_conn->iscsi_conn, hdr, rx_desc->data,
			rx_xfer_len - ISER_HEADERS_LEN);

	ib_dma_sync_single_for_device(ib_conn->device->ib_device, rx_dma,
				      rx_buflen, DMA_FROM_DEVICE);

	/* decrementing conn->post_recv_buf_count only --after-- freeing the   *
	 * task eliminates the need to worry on tasks which are completed in   *
	 * parallel to the execution of iser_conn_term. So the code that waits *
	 * for the posted rx bufs refcount to become zero handles everything   */
	ib_conn->post_recv_buf_count--;

	if (rx_dma == iser_conn->login_resp_dma)
		return;

	outstanding = ib_conn->post_recv_buf_count;
	if (outstanding + iser_conn->min_posted_rx <= iser_conn->qp_max_recv_dtos) {
		count = min(iser_conn->qp_max_recv_dtos - outstanding,
			    iser_conn->min_posted_rx);
		err = iser_post_recvm(iser_conn, count);
		if (err)
			iser_err("posting %d rx bufs err %d\n", count, err);
	}
}

void iser_snd_completion(struct iser_tx_desc *tx_desc,
			struct ib_conn *ib_conn)
{
	struct iscsi_task *task;
	struct iser_device *device = ib_conn->device;

	if (tx_desc->type == ISCSI_TX_DATAOUT) {
		ib_dma_unmap_single(device->ib_device, tx_desc->dma_addr,
					ISER_HEADERS_LEN, DMA_TO_DEVICE);
		kmem_cache_free(ig.desc_cache, tx_desc);
		tx_desc = NULL;
	}

	if (tx_desc && tx_desc->type == ISCSI_TX_CONTROL) {
		/* this arithmetic is legal by libiscsi dd_data allocation */
		task = (void *) ((long)(void *)tx_desc -
				  sizeof(struct iscsi_task));
		if (task->hdr->itt == RESERVED_ITT)
			iscsi_put_task(task);
	}
}

void iser_task_rdma_init(struct iscsi_iser_task *iser_task)

{
	iser_task->status = ISER_TASK_STATUS_INIT;

	iser_task->dir[ISER_DIR_IN] = 0;
	iser_task->dir[ISER_DIR_OUT] = 0;

	iser_task->data[ISER_DIR_IN].data_len  = 0;
	iser_task->data[ISER_DIR_OUT].data_len = 0;

	iser_task->prot[ISER_DIR_IN].data_len  = 0;
	iser_task->prot[ISER_DIR_OUT].data_len = 0;

	memset(&iser_task->rdma_reg[ISER_DIR_IN], 0,
	       sizeof(struct iser_mem_reg));
	memset(&iser_task->rdma_reg[ISER_DIR_OUT], 0,
	       sizeof(struct iser_mem_reg));
}

void iser_task_rdma_finalize(struct iscsi_iser_task *iser_task)
{
	int prot_count = scsi_prot_sg_count(iser_task->sc);

	if (iser_task->dir[ISER_DIR_IN]) {
		iser_unreg_rdma_mem(iser_task, ISER_DIR_IN);
		iser_dma_unmap_task_data(iser_task,
					 &iser_task->data[ISER_DIR_IN],
					 DMA_FROM_DEVICE);
		if (prot_count)
			iser_dma_unmap_task_data(iser_task,
						 &iser_task->prot[ISER_DIR_IN],
						 DMA_FROM_DEVICE);
	}

	if (iser_task->dir[ISER_DIR_OUT]) {
		iser_unreg_rdma_mem(iser_task, ISER_DIR_OUT);
		iser_dma_unmap_task_data(iser_task,
					 &iser_task->data[ISER_DIR_OUT],
					 DMA_TO_DEVICE);
		if (prot_count)
			iser_dma_unmap_task_data(iser_task,
						 &iser_task->prot[ISER_DIR_OUT],
						 DMA_TO_DEVICE);
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 * Copyright (c) 2004, 2005, 2006 Voltaire, Inc. All rights reserved.
 * Copyright (c) 2013-2014 Mellanox Technologies. All rights reserved.
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
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/scatterlist.h>

#include "iscsi_iser.h"
static
int iser_fast_reg_fmr(struct iscsi_iser_task *iser_task,
		      struct iser_data_buf *mem,
		      struct iser_reg_resources *rsc,
		      struct iser_mem_reg *mem_reg);
static
int iser_fast_reg_mr(struct iscsi_iser_task *iser_task,
		     struct iser_data_buf *mem,
		     struct iser_reg_resources *rsc,
		     struct iser_mem_reg *mem_reg);

static struct iser_reg_ops fastreg_ops = {
	.alloc_reg_res	= iser_alloc_fastreg_pool,
	.free_reg_res	= iser_free_fastreg_pool,
	.reg_mem	= iser_fast_reg_mr,
	.unreg_mem	= iser_unreg_mem_fastreg,
	.reg_desc_get	= iser_reg_desc_get_fr,
	.reg_desc_put	= iser_reg_desc_put_fr,
};

static struct iser_reg_ops fmr_ops = {
	.alloc_reg_res	= iser_alloc_fmr_pool,
	.free_reg_res	= iser_free_fmr_pool,
	.reg_mem	= iser_fast_reg_fmr,
	.unreg_mem	= iser_unreg_mem_fmr,
	.reg_desc_get	= iser_reg_desc_get_fmr,
	.reg_desc_put	= iser_reg_desc_put_fmr,
};

int iser_assign_reg_ops(struct iser_device *device)
{
	struct ib_device_attr *dev_attr = &device->dev_attr;

	/* Assign function handles  - based on FMR support */
	if (device->ib_device->alloc_fmr && device->ib_device->dealloc_fmr &&
	    device->ib_device->map_phys_fmr && device->ib_device->unmap_fmr) {
		iser_info("FMR supported, using FMR for registration\n");
		device->reg_ops = &fmr_ops;
	} else
	if (dev_attr->device_cap_flags & IB_DEVICE_MEM_MGT_EXTENSIONS) {
		iser_info("FastReg supported, using FastReg for registration\n");
		device->reg_ops = &fastreg_ops;
	} else {
		iser_err("IB device does not support FMRs nor FastRegs, can't register memory\n");
		return -1;
	}

	return 0;
}

struct iser_fr_desc *
iser_reg_desc_get_fr(struct ib_conn *ib_conn)
{
	struct iser_fr_pool *fr_pool = &ib_conn->fr_pool;
	struct iser_fr_desc *desc;
	unsigned long flags;

	spin_lock_irqsave(&fr_pool->lock, flags);
	desc = list_first_entry(&fr_pool->list,
				struct iser_fr_desc, list);
	list_del(&desc->list);
	spin_unlock_irqrestore(&fr_pool->lock, flags);

	return desc;
}

void
iser_reg_desc_put_fr(struct ib_conn *ib_conn,
		     struct iser_fr_desc *desc)
{
	struct iser_fr_pool *fr_pool = &ib_conn->fr_pool;
	unsigned long flags;

	spin_lock_irqsave(&fr_pool->lock, flags);
	list_add(&desc->list, &fr_pool->list);
	spin_unlock_irqrestore(&fr_pool->lock, flags);
}

struct iser_fr_desc *
iser_reg_desc_get_fmr(struct ib_conn *ib_conn)
{
	struct iser_fr_pool *fr_pool = &ib_conn->fr_pool;

	return list_first_entry(&fr_pool->list,
				struct iser_fr_desc, list);
}

void
iser_reg_desc_put_fmr(struct ib_conn *ib_conn,
		      struct iser_fr_desc *desc)
{
}

#define IS_4K_ALIGNED(addr)	((((unsigned long)addr) & ~MASK_4K) == 0)

/**
 * iser_sg_to_page_vec - Translates scatterlist entries to physical addresses
 * and returns the length of resulting physical address array (may be less than
 * the original due to possible compaction).
 *
 * we build a "page vec" under the assumption that the SG meets the RDMA
 * alignment requirements. Other then the first and last SG elements, all
 * the "internal" elements can be compacted into a list whose elements are
 * dma addresses of physical pages. The code supports also the weird case
 * where --few fragments of the same page-- are present in the SG as
 * consecutive elements. Also, it handles one entry SG.
 */

static int iser_sg_to_page_vec(struct iser_data_buf *data,
			       struct ib_device *ibdev, u64 *pages,
			       int *offset, int *data_size)
{
	struct scatterlist *sg, *sgl = data->sg;
	u64 start_addr, end_addr, page, chunk_start = 0;
	unsigned long total_sz = 0;
	unsigned int dma_len;
	int i, new_chunk, cur_page, last_ent = data->dma_nents - 1;

	/* compute the offset of first element */
	*offset = (u64) sgl[0].offset & ~MASK_4K;

	new_chunk = 1;
	cur_page  = 0;
	for_each_sg(sgl, sg, data->dma_nents, i) {
		start_addr = ib_sg_dma_address(ibdev, sg);
		if (new_chunk)
			chunk_start = start_addr;
		dma_len = ib_sg_dma_len(ibdev, sg);
		end_addr = start_addr + dma_len;
		total_sz += dma_len;

		/* collect page fragments until aligned or end of SG list */
		if (!IS_4K_ALIGNED(end_addr) && i < last_ent) {
			new_chunk = 0;
			continue;
		}
		new_chunk = 1;

		/* address of the first page in the contiguous chunk;
		   masking relevant for the very first SG entry,
		   which might be unaligned */
		page = chunk_start & MASK_4K;
		do {
			pages[cur_page++] = page;
			page += SIZE_4K;
		} while (page < end_addr);
	}

	*data_size = total_sz;
	iser_dbg("page_vec->data_size:%d cur_page %d\n",
		 *data_size, cur_page);
	return cur_page;
}

static void iser_data_buf_dump(struct iser_data_buf *data,
			       struct ib_device *ibdev)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(data->sg, sg, data->dma_nents, i)
		iser_dbg("sg[%d] dma_addr:0x%lX page:0x%p "
			 "off:0x%x sz:0x%x dma_len:0x%x\n",
			 i, (unsigned long)ib_sg_dma_address(ibdev, sg),
			 sg_page(sg), sg->offset,
			 sg->length, ib_sg_dma_len(ibdev, sg));
}

static void iser_dump_page_vec(struct iser_page_vec *page_vec)
{
	int i;

	iser_err("page vec length %d data size %d\n",
		 page_vec->length, page_vec->data_size);
	for (i = 0; i < page_vec->length; i++)
		iser_err("%d %lx\n",i,(unsigned long)page_vec->pages[i]);
}

int iser_dma_map_task_data(struct iscsi_iser_task *iser_task,
			    struct iser_data_buf *data,
			    enum iser_data_dir iser_dir,
			    enum dma_data_direction dma_dir)
{
	struct ib_device *dev;

	iser_task->dir[iser_dir] = 1;
	dev = iser_task->iser_conn->ib_conn.device->ib_device;

	data->dma_nents = ib_dma_map_sg(dev, data->sg, data->size, dma_dir);
	if (data->dma_nents == 0) {
		iser_err("dma_map_sg failed!!!\n");
		return -EINVAL;
	}
	return 0;
}

void iser_dma_unmap_task_data(struct iscsi_iser_task *iser_task,
			      struct iser_data_buf *data,
			      enum dma_data_direction dir)
{
	struct ib_device *dev;

	dev = iser_task->iser_conn->ib_conn.device->ib_device;
	ib_dma_unmap_sg(dev, data->sg, data->size, dir);
}

static int
iser_reg_dma(struct iser_device *device, struct iser_data_buf *mem,
	     struct iser_mem_reg *reg)
{
	struct scatterlist *sg = mem->sg;

	reg->sge.lkey = device->pd->local_dma_lkey;
	reg->rkey = device->mr->rkey;
	reg->sge.addr = ib_sg_dma_address(device->ib_device, &sg[0]);
	reg->sge.length = ib_sg_dma_len(device->ib_device, &sg[0]);

	iser_dbg("Single DMA entry: lkey=0x%x, rkey=0x%x, addr=0x%llx,"
		 " length=0x%x\n", reg->sge.lkey, reg->rkey,
		 reg->sge.addr, reg->sge.length);

	return 0;
}

/**
 * iser_reg_page_vec - Register physical memory
 *
 * returns: 0 on success, errno code on failure
 */
static
int iser_fast_reg_fmr(struct iscsi_iser_task *iser_task,
		      struct iser_data_buf *mem,
		      struct iser_reg_resources *rsc,
		      struct iser_mem_reg *reg)
{
	struct ib_conn *ib_conn = &iser_task->iser_conn->ib_conn;
	struct iser_device *device = ib_conn->device;
	struct iser_page_vec *page_vec = rsc->page_vec;
	struct ib_fmr_pool *fmr_pool = rsc->fmr_pool;
	struct ib_pool_fmr *fmr;
	int ret, plen;

	plen = iser_sg_to_page_vec(mem, device->ib_device,
				   page_vec->pages,
				   &page_vec->offset,
				   &page_vec->data_size);
	page_vec->length = plen;
	if (plen * SIZE_4K < page_vec->data_size) {
		iser_err("page vec too short to hold this SG\n");
		iser_data_buf_dump(mem, device->ib_device);
		iser_dump_page_vec(page_vec);
		return -EINVAL;
	}

	fmr  = ib_fmr_pool_map_phys(fmr_pool,
				    page_vec->pages,
				    page_vec->length,
				    page_vec->pages[0]);
	if (IS_ERR(fmr)) {
		ret = PTR_ERR(fmr);
		iser_err("ib_fmr_pool_map_phys failed: %d\n", ret);
		return ret;
	}

	reg->sge.lkey = fmr->fmr->lkey;
	reg->rkey = fmr->fmr->rkey;
	reg->sge.addr = page_vec->pages[0] + page_vec->offset;
	reg->sge.length = page_vec->data_size;
	reg->mem_h = fmr;

	iser_dbg("fmr reg: lkey=0x%x, rkey=0x%x, addr=0x%llx,"
		 " length=0x%x\n", reg->sge.lkey, reg->rkey,
		 reg->sge.addr, reg->sge.length);

	return 0;
}

/**
 * Unregister (previosuly registered using FMR) memory.
 * If memory is non-FMR does nothing.
 */
void iser_unreg_mem_fmr(struct iscsi_iser_task *iser_task,
			enum iser_data_dir cmd_dir)
{
	struct iser_mem_reg *reg = &iser_task->rdma_reg[cmd_dir];
	int ret;

	if (!reg->mem_h)
		return;

	iser_dbg("PHYSICAL Mem.Unregister mem_h %p\n", reg->mem_h);

	ret = ib_fmr_pool_unmap((struct ib_pool_fmr *)reg->mem_h);
	if (ret)
		iser_err("ib_fmr_pool_unmap failed %d\n", ret);

	reg->mem_h = NULL;
}

void iser_unreg_mem_fastreg(struct iscsi_iser_task *iser_task,
			    enum iser_data_dir cmd_dir)
{
	struct iser_device *device = iser_task->iser_conn->ib_conn.device;
	struct iser_mem_reg *reg = &iser_task->rdma_reg[cmd_dir];

	if (!reg->mem_h)
		return;

	device->reg_ops->reg_desc_put(&iser_task->iser_conn->ib_conn,
				     reg->mem_h);
	reg->mem_h = NULL;
}

static void
iser_set_dif_domain(struct scsi_cmnd *sc, struct ib_sig_attrs *sig_attrs,
		    struct ib_sig_domain *domain)
{
	domain->sig_type = IB_SIG_TYPE_T10_DIF;
	domain->sig.dif.pi_interval = scsi_prot_interval(sc);
	domain->sig.dif.ref_tag = scsi_prot_ref_tag(sc);
	/*
	 * At the moment we hard code those, but in the future
	 * we will take them from sc.
	 */
	domain->sig.dif.apptag_check_mask = 0xffff;
	domain->sig.dif.app_escape = true;
	domain->sig.dif.ref_escape = true;
	if (sc->prot_flags & SCSI_PROT_REF_INCREMENT)
		domain->sig.dif.ref_remap = true;
};

static int
iser_set_sig_attrs(struct scsi_cmnd *sc, struct ib_sig_attrs *sig_attrs)
{
	switch (scsi_get_prot_op(sc)) {
	case SCSI_PROT_WRITE_INSERT:
	case SCSI_PROT_READ_STRIP:
		sig_attrs->mem.sig_type = IB_SIG_TYPE_NONE;
		iser_set_dif_domain(sc, sig_attrs, &sig_attrs->wire);
		sig_attrs->wire.sig.dif.bg_type = IB_T10DIF_CRC;
		break;
	case SCSI_PROT_READ_INSERT:
	case SCSI_PROT_WRITE_STRIP:
		sig_attrs->wire.sig_type = IB_SIG_TYPE_NONE;
		iser_set_dif_domain(sc, sig_attrs, &sig_attrs->mem);
		sig_attrs->mem.sig.dif.bg_type = sc->prot_flags & SCSI_PROT_IP_CHECKSUM ?
						IB_T10DIF_CSUM : IB_T10DIF_CRC;
		break;
	case SCSI_PROT_READ_PASS:
	case SCSI_PROT_WRITE_PASS:
		iser_set_dif_domain(sc, sig_attrs, &sig_attrs->wire);
		sig_attrs->wire.sig.dif.bg_type = IB_T10DIF_CRC;
		iser_set_dif_domain(sc, sig_attrs, &sig_attrs->mem);
		sig_attrs->mem.sig.dif.bg_type = sc->prot_flags & SCSI_PROT_IP_CHECKSUM ?
						IB_T10DIF_CSUM : IB_T10DIF_CRC;
		break;
	default:
		iser_err("Unsupported PI operation %d\n",
			 scsi_get_prot_op(sc));
		return -EINVAL;
	}

	return 0;
}

static inline void
iser_set_prot_checks(struct scsi_cmnd *sc, u8 *mask)
{
	*mask = 0;
	if (sc->prot_flags & SCSI_PROT_REF_CHECK)
		*mask |= ISER_CHECK_REFTAG;
	if (sc->prot_flags & SCSI_PROT_GUARD_CHECK)
		*mask |= ISER_CHECK_GUARD;
}

static void
iser_inv_rkey(struct ib_send_wr *inv_wr, struct ib_mr *mr)
{
	u32 rkey;

	inv_wr->opcode = IB_WR_LOCAL_INV;
	inv_wr->wr_id = ISER_FASTREG_LI_WRID;
	inv_wr->ex.invalidate_rkey = mr->rkey;
	inv_wr->send_flags = 0;
	inv_wr->num_sge = 0;

	rkey = ib_inc_rkey(mr->rkey);
	ib_update_fast_reg_key(mr, rkey);
}

static int
iser_reg_sig_mr(struct iscsi_iser_task *iser_task,
		struct iser_pi_context *pi_ctx,
		struct iser_mem_reg *data_reg,
		struct iser_mem_reg *prot_reg,
		struct iser_mem_reg *sig_reg)
{
	struct iser_tx_desc *tx_desc = &iser_task->desc;
	struct ib_sig_attrs *sig_attrs = &tx_desc->sig_attrs;
	struct ib_sig_handover_wr *wr;
	int ret;

	memset(sig_attrs, 0, sizeof(*sig_attrs));
	ret = iser_set_sig_attrs(iser_task->sc, sig_attrs);
	if (ret)
		goto err;

	iser_set_prot_checks(iser_task->sc, &sig_attrs->check_mask);

	if (!pi_ctx->sig_mr_valid)
		iser_inv_rkey(iser_tx_next_wr(tx_desc), pi_ctx->sig_mr);

	wr = sig_handover_wr(iser_tx_next_wr(tx_desc));
	wr->wr.opcode = IB_WR_REG_SIG_MR;
	wr->wr.wr_id = ISER_FASTREG_LI_WRID;
	wr->wr.sg_list = &data_reg->sge;
	wr->wr.num_sge = 1;
	wr->wr.send_flags = 0;
	wr->sig_attrs = sig_attrs;
	wr->sig_mr = pi_ctx->sig_mr;
	if (scsi_prot_sg_count(iser_task->sc))
		wr->prot = &prot_reg->sge;
	else
		wr->prot = NULL;
	wr->access_flags = IB_ACCESS_LOCAL_WRITE |
			   IB_ACCESS_REMOTE_READ |
			   IB_ACCESS_REMOTE_WRITE;
	pi_ctx->sig_mr_valid = 0;

	sig_reg->sge.lkey = pi_ctx->sig_mr->lkey;
	sig_reg->rkey = pi_ctx->sig_mr->rkey;
	sig_reg->sge.addr = 0;
	sig_reg->sge.length = scsi_transfer_length(iser_task->sc);

	iser_dbg("lkey=0x%x rkey=0x%x addr=0x%llx length=%u\n",
		 sig_reg->sge.lkey, sig_reg->rkey, sig_reg->sge.addr,
		 sig_reg->sge.length);
err:
	return ret;
}

static int iser_fast_reg_mr(struct iscsi_iser_task *iser_task,
			    struct iser_data_buf *mem,
			    struct iser_reg_resources *rsc,
			    struct iser_mem_reg *reg)
{
	struct iser_tx_desc *tx_desc = &iser_task->desc;
	struct ib_mr *mr = rsc->mr;
	struct ib_reg_wr *wr;
	int n;

	if (!rsc->mr_valid)
		iser_inv_rkey(iser_tx_next_wr(tx_desc), mr);

	n = ib_map_mr_sg(mr, mem->sg, mem->size, SIZE_4K);
	if (unlikely(n != mem->size)) {
		iser_err("failed to map sg (%d/%d)\n",
			 n, mem->size);
		return n < 0 ? n : -EINVAL;
	}

	wr = reg_wr(iser_tx_next_wr(tx_desc));
	wr->wr.opcode = IB_WR_REG_MR;
	wr->wr.wr_id = ISER_FASTREG_LI_WRID;
	wr->wr.send_flags = 0;
	wr->wr.num_sge = 0;
	wr->mr = mr;
	wr->key = mr->rkey;
	wr->access = IB_ACCESS_LOCAL_WRITE  |
		     IB_ACCESS_REMOTE_WRITE |
		     IB_ACCESS_REMOTE_READ;

	rsc->mr_valid = 0;

	reg->sge.lkey = mr->lkey;
	reg->rkey = mr->rkey;
	reg->sge.addr = mr->iova;
	reg->sge.length = mr->length;

	iser_dbg("lkey=0x%x rkey=0x%x addr=0x%llx length=0x%x\n",
		 reg->sge.lkey, reg->rkey, reg->sge.addr, reg->sge.length);

	return 0;
}

static int
iser_reg_prot_sg(struct iscsi_iser_task *task,
		 struct iser_data_buf *mem,
		 struct iser_fr_desc *desc,
		 bool use_dma_key,
		 struct iser_mem_reg *reg)
{
	struct iser_device *device = task->iser_conn->ib_conn.device;

	if (use_dma_key)
		return iser_reg_dma(device, mem, reg);

	return device->reg_ops->reg_mem(task, mem, &desc->pi_ctx->rsc, reg);
}

static int
iser_reg_data_sg(struct iscsi_iser_task *task,
		 struct iser_data_buf *mem,
		 struct iser_fr_desc *desc,
		 bool use_dma_key,
		 struct iser_mem_reg *reg)
{
	struct iser_device *device = task->iser_conn->ib_conn.device;

	if (use_dma_key)
		return iser_reg_dma(device, mem, reg);

	return device->reg_ops->reg_mem(task, mem, &desc->rsc, reg);
}

int iser_reg_rdma_mem(struct iscsi_iser_task *task,
		      enum iser_data_dir dir)
{
	struct ib_conn *ib_conn = &task->iser_conn->ib_conn;
	struct iser_device *device = ib_conn->device;
	struct iser_data_buf *mem = &task->data[dir];
	struct iser_mem_reg *reg = &task->rdma_reg[dir];
	struct iser_mem_reg *data_reg;
	struct iser_fr_desc *desc = NULL;
	bool use_dma_key;
	int err;

	use_dma_key = (mem->dma_nents == 1 && !iser_always_reg &&
		       scsi_get_prot_op(task->sc) == SCSI_PROT_NORMAL);

	if (!use_dma_key) {
		desc = device->reg_ops->reg_desc_get(ib_conn);
		reg->mem_h = desc;
	}

	if (scsi_get_prot_op(task->sc) == SCSI_PROT_NORMAL)
		data_reg = reg;
	else
		data_reg = &task->desc.data_reg;

	err = iser_reg_data_sg(task, mem, desc, use_dma_key, data_reg);
	if (unlikely(err))
		goto err_reg;

	if (scsi_get_prot_op(task->sc) != SCSI_PROT_NORMAL) {
		struct iser_mem_reg *prot_reg = &task->desc.prot_reg;

		if (scsi_prot_sg_count(task->sc)) {
			mem = &task->prot[dir];
			err = iser_reg_prot_sg(task, mem, desc,
					       use_dma_key, prot_reg);
			if (unlikely(err))
				goto err_reg;
		}

		err = iser_reg_sig_mr(task, desc->pi_ctx, data_reg,
				      prot_reg, reg);
		if (unlikely(err))
			goto err_reg;

		desc->pi_ctx->sig_protected = 1;
	}

	return 0;

err_reg:
	if (desc)
		device->reg_ops->reg_desc_put(ib_conn, desc);

	return err;
}

void iser_unreg_rdma_mem(struct iscsi_iser_task *task,
			 enum iser_data_dir dir)
{
	struct iser_device *device = task->iser_conn->ib_conn.device;

	device->reg_ops->unreg_mem(task, dir);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright (c) 2004, 2005, 2006 Voltaire, Inc. All rights reserved.
 * Copyright (c) 2005, 2006 Cisco Systems.  All rights reserved.
 * Copyright (c) 2013-2014 Mellanox Technologies. All rights reserved.
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
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "iscsi_iser.h"

#define ISCSI_ISER_MAX_CONN	8
#define ISER_MAX_RX_LEN		(ISER_QP_MAX_RECV_DTOS * ISCSI_ISER_MAX_CONN)
#define ISER_MAX_TX_LEN		(ISER_QP_MAX_REQ_DTOS  * ISCSI_ISER_MAX_CONN)
#define ISER_MAX_CQ_LEN		(ISER_MAX_RX_LEN + ISER_MAX_TX_LEN + \
				 ISCSI_ISER_MAX_CONN)

static int iser_cq_poll_limit = 512;

static void iser_cq_tasklet_fn(unsigned long data);
static void iser_cq_callback(struct ib_cq *cq, void *cq_context);

static void iser_cq_event_callback(struct ib_event *cause, void *context)
{
	iser_err("cq event %s (%d)\n",
		 ib_event_msg(cause->event), cause->event);
}

static void iser_qp_event_callback(struct ib_event *cause, void *context)
{
	iser_err("qp event %s (%d)\n",
		 ib_event_msg(cause->event), cause->event);
}

static void iser_event_handler(struct ib_event_handler *handler,
				struct ib_event *event)
{
	iser_err("async event %s (%d) on device %s port %d\n",
		 ib_event_msg(event->event), event->event,
		 event->device->name, event->element.port_num);
}

/**
 * iser_create_device_ib_res - creates Protection Domain (PD), Completion
 * Queue (CQ), DMA Memory Region (DMA MR) with the device associated with
 * the adapator.
 *
 * returns 0 on success, -1 on failure
 */
static int iser_create_device_ib_res(struct iser_device *device)
{
	struct ib_device_attr *dev_attr = &device->dev_attr;
	int ret, i, max_cqe;

	ret = ib_query_device(device->ib_device, dev_attr);
	if (ret) {
		pr_warn("Query device failed for %s\n", device->ib_device->name);
		return ret;
	}

	ret = iser_assign_reg_ops(device);
	if (ret)
		return ret;

	device->comps_used = min_t(int, num_online_cpus(),
				 device->ib_device->num_comp_vectors);

	device->comps = kcalloc(device->comps_used, sizeof(*device->comps),
				GFP_KERNEL);
	if (!device->comps)
		goto comps_err;

	max_cqe = min(ISER_MAX_CQ_LEN, dev_attr->max_cqe);

	iser_info("using %d CQs, device %s supports %d vectors max_cqe %d\n",
		  device->comps_used, device->ib_device->name,
		  device->ib_device->num_comp_vectors, max_cqe);

	device->pd = ib_alloc_pd(device->ib_device);
	if (IS_ERR(device->pd))
		goto pd_err;

	for (i = 0; i < device->comps_used; i++) {
		struct ib_cq_init_attr cq_attr = {};
		struct iser_comp *comp = &device->comps[i];

		comp->device = device;
		cq_attr.cqe = max_cqe;
		cq_attr.comp_vector = i;
		comp->cq = ib_create_cq(device->ib_device,
					iser_cq_callback,
					iser_cq_event_callback,
					(void *)comp,
					&cq_attr);
		if (IS_ERR(comp->cq)) {
			comp->cq = NULL;
			goto cq_err;
		}

		if (ib_req_notify_cq(comp->cq, IB_CQ_NEXT_COMP))
			goto cq_err;

		tasklet_init(&comp->tasklet, iser_cq_tasklet_fn,
			     (unsigned long)comp);
	}

	if (!iser_always_reg) {
		int access = IB_ACCESS_LOCAL_WRITE |
			     IB_ACCESS_REMOTE_WRITE |
			     IB_ACCESS_REMOTE_READ;

		device->mr = ib_get_dma_mr(device->pd, access);
		if (IS_ERR(device->mr))
			goto dma_mr_err;
	}

	INIT_IB_EVENT_HANDLER(&device->event_handler, device->ib_device,
				iser_event_handler);
	if (ib_register_event_handler(&device->event_handler))
		goto handler_err;

	return 0;

handler_err:
	if (device->mr)
		ib_dereg_mr(device->mr);
dma_mr_err:
	for (i = 0; i < device->comps_used; i++)
		tasklet_kill(&device->comps[i].tasklet);
cq_err:
	for (i = 0; i < device->comps_used; i++) {
		struct iser_comp *comp = &device->comps[i];

		if (comp->cq)
			ib_destroy_cq(comp->cq);
	}
	ib_dealloc_pd(device->pd);
pd_err:
	kfree(device->comps);
comps_err:
	iser_err("failed to allocate an IB resource\n");
	return -1;
}

/**
 * iser_free_device_ib_res - destroy/dealloc/dereg the DMA MR,
 * CQ and PD created with the device associated with the adapator.
 */
static void iser_free_device_ib_res(struct iser_device *device)
{
	int i;

	for (i = 0; i < device->comps_used; i++) {
		struct iser_comp *comp = &device->comps[i];

		tasklet_kill(&comp->tasklet);
		ib_destroy_cq(comp->cq);
		comp->cq = NULL;
	}

	(void)ib_unregister_event_handler(&device->event_handler);
	if (device->mr)
		(void)ib_dereg_mr(device->mr);
	ib_dealloc_pd(device->pd);

	kfree(device->comps);
	device->comps = NULL;

	device->mr = NULL;
	device->pd = NULL;
}

/**
 * iser_alloc_fmr_pool - Creates FMR pool and page_vector
 *
 * returns 0 on success, or errno code on failure
 */
int iser_alloc_fmr_pool(struct ib_conn *ib_conn,
			unsigned cmds_max,
			unsigned int size)
{
	struct iser_device *device = ib_conn->device;
	struct iser_fr_pool *fr_pool = &ib_conn->fr_pool;
	struct iser_page_vec *page_vec;
	struct iser_fr_desc *desc;
	struct ib_fmr_pool *fmr_pool;
	struct ib_fmr_pool_param params;
	int ret;

	INIT_LIST_HEAD(&fr_pool->list);
	spin_lock_init(&fr_pool->lock);

	desc = kzalloc(sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	page_vec = kmalloc(sizeof(*page_vec) + (sizeof(u64) * size),
			   GFP_KERNEL);
	if (!page_vec) {
		ret = -ENOMEM;
		goto err_frpl;
	}

	page_vec->pages = (u64 *)(page_vec + 1);

	params.page_shift        = SHIFT_4K;
	params.max_pages_per_fmr = size;
	/* make the pool size twice the max number of SCSI commands *
	 * the ML is expected to queue, watermark for unmap at 50%  */
	params.pool_size	 = cmds_max * 2;
	params.dirty_watermark	 = cmds_max;
	params.cache		 = 0;
	params.flush_function	 = NULL;
	params.access		 = (IB_ACCESS_LOCAL_WRITE  |
				    IB_ACCESS_REMOTE_WRITE |
				    IB_ACCESS_REMOTE_READ);

	fmr_pool = ib_create_fmr_pool(device->pd, &params);
	if (IS_ERR(fmr_pool)) {
		ret = PTR_ERR(fmr_pool);
		iser_err("FMR allocation failed, err %d\n", ret);
		goto err_fmr;
	}

	desc->rsc.page_vec = page_vec;
	desc->rsc.fmr_pool = fmr_pool;
	list_add(&desc->list, &fr_pool->list);

	return 0;

err_fmr:
	kfree(page_vec);
err_frpl:
	kfree(desc);

	return ret;
}

/**
 * iser_free_fmr_pool - releases the FMR pool and page vec
 */
void iser_free_fmr_pool(struct ib_conn *ib_conn)
{
	struct iser_fr_pool *fr_pool = &ib_conn->fr_pool;
	struct iser_fr_desc *desc;

	desc = list_first_entry(&fr_pool->list,
				struct iser_fr_desc, list);
	list_del(&desc->list);

	iser_info("freeing conn %p fmr pool %p\n",
		  ib_conn, desc->rsc.fmr_pool);

	ib_destroy_fmr_pool(desc->rsc.fmr_pool);
	kfree(desc->rsc.page_vec);
	kfree(desc);
}

static int
iser_alloc_reg_res(struct ib_device *ib_device,
		   struct ib_pd *pd,
		   struct iser_reg_resources *res,
		   unsigned int size)
{
	int ret;

	res->mr = ib_alloc_mr(pd, IB_MR_TYPE_MEM_REG, size);
	if (IS_ERR(res->mr)) {
		ret = PTR_ERR(res->mr);
		iser_err("Failed to allocate ib_fast_reg_mr err=%d\n", ret);
		return ret;
	}
	res->mr_valid = 1;

	return 0;
}

static void
iser_free_reg_res(struct iser_reg_resources *rsc)
{
	ib_dereg_mr(rsc->mr);
}

static int
iser_alloc_pi_ctx(struct ib_device *ib_device,
		  struct ib_pd *pd,
		  struct iser_fr_desc *desc,
		  unsigned int size)
{
	struct iser_pi_context *pi_ctx = NULL;
	int ret;

	desc->pi_ctx = kzalloc(sizeof(*desc->pi_ctx), GFP_KERNEL);
	if (!desc->pi_ctx)
		return -ENOMEM;

	pi_ctx = desc->pi_ctx;

	ret = iser_alloc_reg_res(ib_device, pd, &pi_ctx->rsc, size);
	if (ret) {
		iser_err("failed to allocate reg_resources\n");
		goto alloc_reg_res_err;
	}

	pi_ctx->sig_mr = ib_alloc_mr(pd, IB_MR_TYPE_SIGNATURE, 2);
	if (IS_ERR(pi_ctx->sig_mr)) {
		ret = PTR_ERR(pi_ctx->sig_mr);
		goto sig_mr_failure;
	}
	pi_ctx->sig_mr_valid = 1;
	desc->pi_ctx->sig_protected = 0;

	return 0;

sig_mr_failure:
	iser_free_reg_res(&pi_ctx->rsc);
alloc_reg_res_err:
	kfree(desc->pi_ctx);

	return ret;
}

static void
iser_free_pi_ctx(struct iser_pi_context *pi_ctx)
{
	iser_free_reg_res(&pi_ctx->rsc);
	ib_dereg_mr(pi_ctx->sig_mr);
	kfree(pi_ctx);
}

static struct iser_fr_desc *
iser_create_fastreg_desc(struct ib_device *ib_device,
			 struct ib_pd *pd,
			 bool pi_enable,
			 unsigned int size)
{
	struct iser_fr_desc *desc;
	int ret;

	desc = kzalloc(sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return ERR_PTR(-ENOMEM);

	ret = iser_alloc_reg_res(ib_device, pd, &desc->rsc, size);
	if (ret)
		goto reg_res_alloc_failure;

	if (pi_enable) {
		ret = iser_alloc_pi_ctx(ib_device, pd, desc, size);
		if (ret)
			goto pi_ctx_alloc_failure;
	}

	return desc;

pi_ctx_alloc_failure:
	iser_free_reg_res(&desc->rsc);
reg_res_alloc_failure:
	kfree(desc);

	return ERR_PTR(ret);
}

/**
 * iser_alloc_fastreg_pool - Creates pool of fast_reg descriptors
 * for fast registration work requests.
 * returns 0 on success, or errno code on failure
 */
int iser_alloc_fastreg_pool(struct ib_conn *ib_conn,
			    unsigned cmds_max,
			    unsigned int size)
{
	struct iser_device *device = ib_conn->device;
	struct iser_fr_pool *fr_pool = &ib_conn->fr_pool;
	struct iser_fr_desc *desc;
	int i, ret;

	INIT_LIST_HEAD(&fr_pool->list);
	INIT_LIST_HEAD(&fr_pool->all_list);
	spin_lock_init(&fr_pool->lock);
	fr_pool->size = 0;
	for (i = 0; i < cmds_max; i++) {
		desc = iser_create_fastreg_desc(device->ib_device, device->pd,
						ib_conn->pi_support, size);
		if (IS_ERR(desc)) {
			ret = PTR_ERR(desc);
			goto err;
		}

		list_add_tail(&desc->list, &fr_pool->list);
		list_add_tail(&desc->all_list, &fr_pool->all_list);
		fr_pool->size++;
	}

	return 0;

err:
	iser_free_fastreg_pool(ib_conn);
	return ret;
}

/**
 * iser_free_fastreg_pool - releases the pool of fast_reg descriptors
 */
void iser_free_fastreg_pool(struct ib_conn *ib_conn)
{
	struct iser_fr_pool *fr_pool = &ib_conn->fr_pool;
	struct iser_fr_desc *desc, *tmp;
	int i = 0;

	if (list_empty(&fr_pool->all_list))
		return;

	iser_info("freeing conn %p fr pool\n", ib_conn);

	list_for_each_entry_safe(desc, tmp, &fr_pool->all_list, all_list) {
		list_del(&desc->all_list);
		iser_free_reg_res(&desc->rsc);
		if (desc->pi_ctx)
			iser_free_pi_ctx(desc->pi_ctx);
		kfree(desc);
		++i;
	}

	if (i < fr_pool->size)
		iser_warn("pool still has %d regions registered\n",
			  fr_pool->size - i);
}

/**
 * iser_create_ib_conn_res - Queue-Pair (QP)
 *
 * returns 0 on success, -1 on failure
 */
static int iser_create_ib_conn_res(struct ib_conn *ib_conn)
{
	struct iser_conn *iser_conn = container_of(ib_conn, struct iser_conn,
						   ib_conn);
	struct iser_device	*device;
	struct ib_device_attr *dev_attr;
	struct ib_qp_init_attr	init_attr;
	int			ret = -ENOMEM;
	int index, min_index = 0;

	BUG_ON(ib_conn->device == NULL);

	device = ib_conn->device;
	dev_attr = &device->dev_attr;

	memset(&init_attr, 0, sizeof init_attr);

	mutex_lock(&ig.connlist_mutex);
	/* select the CQ with the minimal number of usages */
	for (index = 0; index < device->comps_used; index++) {
		if (device->comps[index].active_qps <
		    device->comps[min_index].active_qps)
			min_index = index;
	}
	ib_conn->comp = &device->comps[min_index];
	ib_conn->comp->active_qps++;
	mutex_unlock(&ig.connlist_mutex);
	iser_info("cq index %d used for ib_conn %p\n", min_index, ib_conn);

	init_attr.event_handler = iser_qp_event_callback;
	init_attr.qp_context	= (void *)ib_conn;
	init_attr.send_cq	= ib_conn->comp->cq;
	init_attr.recv_cq	= ib_conn->comp->cq;
	init_attr.cap.max_recv_wr  = ISER_QP_MAX_RECV_DTOS;
	init_attr.cap.max_send_sge = 2;
	init_attr.cap.max_recv_sge = 1;
	init_attr.sq_sig_type	= IB_SIGNAL_REQ_WR;
	init_attr.qp_type	= IB_QPT_RC;
	if (ib_conn->pi_support) {
		init_attr.cap.max_send_wr = ISER_QP_SIG_MAX_REQ_DTOS + 1;
		init_attr.create_flags |= IB_QP_CREATE_SIGNATURE_EN;
		iser_conn->max_cmds =
			ISER_GET_MAX_XMIT_CMDS(ISER_QP_SIG_MAX_REQ_DTOS);
	} else {
		if (dev_attr->max_qp_wr > ISER_QP_MAX_REQ_DTOS) {
			init_attr.cap.max_send_wr  = ISER_QP_MAX_REQ_DTOS + 1;
			iser_conn->max_cmds =
				ISER_GET_MAX_XMIT_CMDS(ISER_QP_MAX_REQ_DTOS);
		} else {
			init_attr.cap.max_send_wr = dev_attr->max_qp_wr;
			iser_conn->max_cmds =
				ISER_GET_MAX_XMIT_CMDS(dev_attr->max_qp_wr);
			iser_dbg("device %s supports max_send_wr %d\n",
				 device->ib_device->name, dev_attr->max_qp_wr);
		}
	}

	ret = rdma_create_qp(ib_conn->cma_id, device->pd, &init_attr);
	if (ret)
		goto out_err;

	ib_conn->qp = ib_conn->cma_id->qp;
	iser_info("setting conn %p cma_id %p qp %p\n",
		  ib_conn, ib_conn->cma_id,
		  ib_conn->cma_id->qp);
	return ret;

out_err:
	mutex_lock(&ig.connlist_mutex);
	ib_conn->comp->active_qps--;
	mutex_unlock(&ig.connlist_mutex);
	iser_err("unable to alloc mem or create resource, err %d\n", ret);

	return ret;
}

/**
 * based on the resolved device node GUID see if there already allocated
 * device for this device. If there's no such, create one.
 */
static
struct iser_device *iser_device_find_by_ib_device(struct rdma_cm_id *cma_id)
{
	struct iser_device *device;

	mutex_lock(&ig.device_list_mutex);

	list_for_each_entry(device, &ig.device_list, ig_list)
		/* find if there's a match using the node GUID */
		if (device->ib_device->node_guid == cma_id->device->node_guid)
			goto inc_refcnt;

	device = kzalloc(sizeof *device, GFP_KERNEL);
	if (device == NULL)
		goto out;

	/* assign this device to the device */
	device->ib_device = cma_id->device;
	/* init the device and link it into ig device list */
	if (iser_create_device_ib_res(device)) {
		kfree(device);
		device = NULL;
		goto out;
	}
	list_add(&device->ig_list, &ig.device_list);

inc_refcnt:
	device->refcount++;
out:
	mutex_unlock(&ig.device_list_mutex);
	return device;
}

/* if there's no demand for this device, release it */
static void iser_device_try_release(struct iser_device *device)
{
	mutex_lock(&ig.device_list_mutex);
	device->refcount--;
	iser_info("device %p refcount %d\n", device, device->refcount);
	if (!device->refcount) {
		iser_free_device_ib_res(device);
		list_del(&device->ig_list);
		kfree(device);
	}
	mutex_unlock(&ig.device_list_mutex);
}

/**
 * Called with state mutex held
 **/
static int iser_conn_state_comp_exch(struct iser_conn *iser_conn,
				     enum iser_conn_state comp,
				     enum iser_conn_state exch)
{
	int ret;

	ret = (iser_conn->state == comp);
	if (ret)
		iser_conn->state = exch;

	return ret;
}

void iser_release_work(struct work_struct *work)
{
	struct iser_conn *iser_conn;

	iser_conn = container_of(work, struct iser_conn, release_work);

	/* Wait for conn_stop to complete */
	wait_for_completion(&iser_conn->stop_completion);
	/* Wait for IB resouces cleanup to complete */
	wait_for_completion(&iser_conn->ib_completion);

	mutex_lock(&iser_conn->state_mutex);
	iser_conn->state = ISER_CONN_DOWN;
	mutex_unlock(&iser_conn->state_mutex);

	iser_conn_release(iser_conn);
}

/**
 * iser_free_ib_conn_res - release IB related resources
 * @iser_conn: iser connection struct
 * @destroy: indicator if we need to try to release the
 *     iser device and memory regoins pool (only iscsi
 *     shutdown and DEVICE_REMOVAL will use this).
 *
 * This routine is called with the iser state mutex held
 * so the cm_id removal is out of here. It is Safe to
 * be invoked multiple times.
 */
static void iser_free_ib_conn_res(struct iser_conn *iser_conn,
				  bool destroy)
{
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	struct iser_device *device = ib_conn->device;

	iser_info("freeing conn %p cma_id %p qp %p\n",
		  iser_conn, ib_conn->cma_id, ib_conn->qp);

	if (ib_conn->qp != NULL) {
		ib_conn->comp->active_qps--;
		rdma_destroy_qp(ib_conn->cma_id);
		ib_conn->qp = NULL;
	}

	if (destroy) {
		if (iser_conn->rx_descs)
			iser_free_rx_descriptors(iser_conn);

		if (device != NULL) {
			iser_device_try_release(device);
			ib_conn->device = NULL;
		}
	}
}

/**
 * Frees all conn objects and deallocs conn descriptor
 */
void iser_conn_release(struct iser_conn *iser_conn)
{
	struct ib_conn *ib_conn = &iser_conn->ib_conn;

	mutex_lock(&ig.connlist_mutex);
	list_del(&iser_conn->conn_list);
	mutex_unlock(&ig.connlist_mutex);

	mutex_lock(&iser_conn->state_mutex);
	/* In case we endup here without ep_disconnect being invoked. */
	if (iser_conn->state != ISER_CONN_DOWN) {
		iser_warn("iser conn %p state %d, expected state down.\n",
			  iser_conn, iser_conn->state);
		iscsi_destroy_endpoint(iser_conn->ep);
		iser_conn->state = ISER_CONN_DOWN;
	}
	/*
	 * In case we never got to bind stage, we still need to
	 * release IB resources (which is safe to call more than once).
	 */
	iser_free_ib_conn_res(iser_conn, true);
	mutex_unlock(&iser_conn->state_mutex);

	if (ib_conn->cma_id != NULL) {
		rdma_destroy_id(ib_conn->cma_id);
		ib_conn->cma_id = NULL;
	}

	kfree(iser_conn);
}

/**
 * triggers start of the disconnect procedures and wait for them to be done
 * Called with state mutex held
 */
int iser_conn_terminate(struct iser_conn *iser_conn)
{
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	struct ib_send_wr *bad_wr;
	int err = 0;

	/* terminate the iser conn only if the conn state is UP */
	if (!iser_conn_state_comp_exch(iser_conn, ISER_CONN_UP,
				       ISER_CONN_TERMINATING))
		return 0;

	iser_info("iser_conn %p state %d\n", iser_conn, iser_conn->state);

	/* suspend queuing of new iscsi commands */
	if (iser_conn->iscsi_conn)
		iscsi_suspend_queue(iser_conn->iscsi_conn);

	/*
	 * In case we didn't already clean up the cma_id (peer initiated
	 * a disconnection), we need to Cause the CMA to change the QP
	 * state to ERROR.
	 */
	if (ib_conn->cma_id) {
		err = rdma_disconnect(ib_conn->cma_id);
		if (err)
			iser_err("Failed to disconnect, conn: 0x%p err %d\n",
				 iser_conn, err);

		/* post an indication that all flush errors were consumed */
		err = ib_post_send(ib_conn->qp, &ib_conn->beacon, &bad_wr);
		if (err) {
			iser_err("conn %p failed to post beacon", ib_conn);
			return 1;
		}

		wait_for_completion(&ib_conn->flush_comp);
	}

	return 1;
}

/**
 * Called with state mutex held
 **/
static void iser_connect_error(struct rdma_cm_id *cma_id)
{
	struct iser_conn *iser_conn;

	iser_conn = (struct iser_conn *)cma_id->context;
	iser_conn->state = ISER_CONN_TERMINATING;
}

static void
iser_calc_scsi_params(struct iser_conn *iser_conn,
		      unsigned int max_sectors)
{
	struct iser_device *device = iser_conn->ib_conn.device;
	unsigned short sg_tablesize, sup_sg_tablesize;

	sg_tablesize = DIV_ROUND_UP(max_sectors * 512, SIZE_4K);
	sup_sg_tablesize = min_t(unsigned, ISCSI_ISER_MAX_SG_TABLESIZE,
				 device->dev_attr.max_fast_reg_page_list_len);

	if (sg_tablesize > sup_sg_tablesize) {
		sg_tablesize = sup_sg_tablesize;
		iser_conn->scsi_max_sectors = sg_tablesize * SIZE_4K / 512;
	} else {
		iser_conn->scsi_max_sectors = max_sectors;
	}

	iser_conn->scsi_sg_tablesize = sg_tablesize;

	iser_dbg("iser_conn %p, sg_tablesize %u, max_sectors %u\n",
		 iser_conn, iser_conn->scsi_sg_tablesize,
		 iser_conn->scsi_max_sectors);
}

/**
 * Called with state mutex held
 **/
static void iser_addr_handler(struct rdma_cm_id *cma_id)
{
	struct iser_device *device;
	struct iser_conn   *iser_conn;
	struct ib_conn   *ib_conn;
	int    ret;

	iser_conn = (struct iser_conn *)cma_id->context;
	if (iser_conn->state != ISER_CONN_PENDING)
		/* bailout */
		return;

	ib_conn = &iser_conn->ib_conn;
	device = iser_device_find_by_ib_device(cma_id);
	if (!device) {
		iser_err("device lookup/creation failed\n");
		iser_connect_error(cma_id);
		return;
	}

	ib_conn->device = device;

	/* connection T10-PI support */
	if (iser_pi_enable) {
		if (!(device->dev_attr.device_cap_flags &
		      IB_DEVICE_SIGNATURE_HANDOVER)) {
			iser_warn("T10-PI requested but not supported on %s, "
				  "continue without T10-PI\n",
				  ib_conn->device->ib_device->name);
			ib_conn->pi_support = false;
		} else {
			ib_conn->pi_support = true;
		}
	}

	iser_calc_scsi_params(iser_conn, iser_max_sectors);

	ret = rdma_resolve_route(cma_id, 1000);
	if (ret) {
		iser_err("resolve route failed: %d\n", ret);
		iser_connect_error(cma_id);
		return;
	}
}

/**
 * Called with state mutex held
 **/
static void iser_route_handler(struct rdma_cm_id *cma_id)
{
	struct rdma_conn_param conn_param;
	int    ret;
	struct iser_cm_hdr req_hdr;
	struct iser_conn *iser_conn = (struct iser_conn *)cma_id->context;
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	struct iser_device *device = ib_conn->device;

	if (iser_conn->state != ISER_CONN_PENDING)
		/* bailout */
		return;

	ret = iser_create_ib_conn_res(ib_conn);
	if (ret)
		goto failure;

	memset(&conn_param, 0, sizeof conn_param);
	conn_param.responder_resources = device->dev_attr.max_qp_rd_atom;
	conn_param.initiator_depth     = 1;
	conn_param.retry_count	       = 7;
	conn_param.rnr_retry_count     = 6;

	memset(&req_hdr, 0, sizeof(req_hdr));
	req_hdr.flags = (ISER_ZBVA_NOT_SUPPORTED |
			ISER_SEND_W_INV_NOT_SUPPORTED);
	conn_param.private_data		= (void *)&req_hdr;
	conn_param.private_data_len	= sizeof(struct iser_cm_hdr);

	ret = rdma_connect(cma_id, &conn_param);
	if (ret) {
		iser_err("failure connecting: %d\n", ret);
		goto failure;
	}

	return;
failure:
	iser_connect_error(cma_id);
}

static void iser_connected_handler(struct rdma_cm_id *cma_id)
{
	struct iser_conn *iser_conn;
	struct ib_qp_attr attr;
	struct ib_qp_init_attr init_attr;

	iser_conn = (struct iser_conn *)cma_id->context;
	if (iser_conn->state != ISER_CONN_PENDING)
		/* bailout */
		return;

	(void)ib_query_qp(cma_id->qp, &attr, ~0, &init_attr);
	iser_info("remote qpn:%x my qpn:%x\n", attr.dest_qp_num, cma_id->qp->qp_num);

	iser_conn->state = ISER_CONN_UP;
	complete(&iser_conn->up_completion);
}

static void iser_disconnected_handler(struct rdma_cm_id *cma_id)
{
	struct iser_conn *iser_conn = (struct iser_conn *)cma_id->context;

	if (iser_conn_terminate(iser_conn)) {
		if (iser_conn->iscsi_conn)
			iscsi_conn_failure(iser_conn->iscsi_conn,
					   ISCSI_ERR_CONN_FAILED);
		else
			iser_err("iscsi_iser connection isn't bound\n");
	}
}

static void iser_cleanup_handler(struct rdma_cm_id *cma_id,
				 bool destroy)
{
	struct iser_conn *iser_conn = (struct iser_conn *)cma_id->context;

	/*
	 * We are not guaranteed that we visited disconnected_handler
	 * by now, call it here to be safe that we handle CM drep
	 * and flush errors.
	 */
	iser_disconnected_handler(cma_id);
	iser_free_ib_conn_res(iser_conn, destroy);
	complete(&iser_conn->ib_completion);
};

static int iser_cma_handler(struct rdma_cm_id *cma_id, struct rdma_cm_event *event)
{
	struct iser_conn *iser_conn;
	int ret = 0;

	iser_conn = (struct iser_conn *)cma_id->context;
	iser_info("%s (%d): status %d conn %p id %p\n",
		  rdma_event_msg(event->event), event->event,
		  event->status, cma_id->context, cma_id);

	mutex_lock(&iser_conn->state_mutex);
	switch (event->event) {
	case RDMA_CM_EVENT_ADDR_RESOLVED:
		iser_addr_handler(cma_id);
		break;
	case RDMA_CM_EVENT_ROUTE_RESOLVED:
		iser_route_handler(cma_id);
		break;
	case RDMA_CM_EVENT_ESTABLISHED:
		iser_connected_handler(cma_id);
		break;
	case RDMA_CM_EVENT_ADDR_ERROR:
	case RDMA_CM_EVENT_ROUTE_ERROR:
	case RDMA_CM_EVENT_CONNECT_ERROR:
	case RDMA_CM_EVENT_UNREACHABLE:
	case RDMA_CM_EVENT_REJECTED:
		iser_connect_error(cma_id);
		break;
	case RDMA_CM_EVENT_DISCONNECTED:
	case RDMA_CM_EVENT_ADDR_CHANGE:
	case RDMA_CM_EVENT_TIMEWAIT_EXIT:
		iser_cleanup_handler(cma_id, false);
		break;
	case RDMA_CM_EVENT_DEVICE_REMOVAL:
		/*
		 * we *must* destroy the device as we cannot rely
		 * on iscsid to be around to initiate error handling.
		 * also if we are not in state DOWN implicitly destroy
		 * the cma_id.
		 */
		iser_cleanup_handler(cma_id, true);
		if (iser_conn->state != ISER_CONN_DOWN) {
			iser_conn->ib_conn.cma_id = NULL;
			ret = 1;
		}
		break;
	default:
		iser_err("Unexpected RDMA CM event: %s (%d)\n",
			 rdma_event_msg(event->event), event->event);
		break;
	}
	mutex_unlock(&iser_conn->state_mutex);

	return ret;
}

void iser_conn_init(struct iser_conn *iser_conn)
{
	iser_conn->state = ISER_CONN_INIT;
	iser_conn->ib_conn.post_recv_buf_count = 0;
	init_completion(&iser_conn->ib_conn.flush_comp);
	init_completion(&iser_conn->stop_completion);
	init_completion(&iser_conn->ib_completion);
	init_completion(&iser_conn->up_completion);
	INIT_LIST_HEAD(&iser_conn->conn_list);
	mutex_init(&iser_conn->state_mutex);
}

 /**
 * starts the process of connecting to the target
 * sleeps until the connection is established or rejected
 */
int iser_connect(struct iser_conn   *iser_conn,
		 struct sockaddr    *src_addr,
		 struct sockaddr    *dst_addr,
		 int                 non_blocking)
{
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	int err = 0;

	mutex_lock(&iser_conn->state_mutex);

	sprintf(iser_conn->name, "%pISp", dst_addr);

	iser_info("connecting to: %s\n", iser_conn->name);

	/* the device is known only --after-- address resolution */
	ib_conn->device = NULL;

	iser_conn->state = ISER_CONN_PENDING;

	ib_conn->beacon.wr_id = ISER_BEACON_WRID;
	ib_conn->beacon.opcode = IB_WR_SEND;

	ib_conn->cma_id = rdma_create_id(&init_net, iser_cma_handler,
					 (void *)iser_conn,
					 RDMA_PS_TCP, IB_QPT_RC);
	if (IS_ERR(ib_conn->cma_id)) {
		err = PTR_ERR(ib_conn->cma_id);
		iser_err("rdma_create_id failed: %d\n", err);
		goto id_failure;
	}

	err = rdma_resolve_addr(ib_conn->cma_id, src_addr, dst_addr, 1000);
	if (err) {
		iser_err("rdma_resolve_addr failed: %d\n", err);
		goto addr_failure;
	}

	if (!non_blocking) {
		wait_for_completion_interruptible(&iser_conn->up_completion);

		if (iser_conn->state != ISER_CONN_UP) {
			err =  -EIO;
			goto connect_failure;
		}
	}
	mutex_unlock(&iser_conn->state_mutex);

	mutex_lock(&ig.connlist_mutex);
	list_add(&iser_conn->conn_list, &ig.connlist);
	mutex_unlock(&ig.connlist_mutex);
	return 0;

id_failure:
	ib_conn->cma_id = NULL;
addr_failure:
	iser_conn->state = ISER_CONN_DOWN;
connect_failure:
	mutex_unlock(&iser_conn->state_mutex);
	iser_conn_release(iser_conn);
	return err;
}

int iser_post_recvl(struct iser_conn *iser_conn)
{
	struct ib_recv_wr rx_wr, *rx_wr_failed;
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	struct ib_sge	  sge;
	int ib_ret;

	sge.addr   = iser_conn->login_resp_dma;
	sge.length = ISER_RX_LOGIN_SIZE;
	sge.lkey   = ib_conn->device->pd->local_dma_lkey;

	rx_wr.wr_id   = (uintptr_t)iser_conn->login_resp_buf;
	rx_wr.sg_list = &sge;
	rx_wr.num_sge = 1;
	rx_wr.next    = NULL;

	ib_conn->post_recv_buf_count++;
	ib_ret	= ib_post_recv(ib_conn->qp, &rx_wr, &rx_wr_failed);
	if (ib_ret) {
		iser_err("ib_post_recv failed ret=%d\n", ib_ret);
		ib_conn->post_recv_buf_count--;
	}
	return ib_ret;
}

int iser_post_recvm(struct iser_conn *iser_conn, int count)
{
	struct ib_recv_wr *rx_wr, *rx_wr_failed;
	int i, ib_ret;
	struct ib_conn *ib_conn = &iser_conn->ib_conn;
	unsigned int my_rx_head = iser_conn->rx_desc_head;
	struct iser_rx_desc *rx_desc;

	for (rx_wr = ib_conn->rx_wr, i = 0; i < count; i++, rx_wr++) {
		rx_desc		= &iser_conn->rx_descs[my_rx_head];
		rx_wr->wr_id	= (uintptr_t)rx_desc;
		rx_wr->sg_list	= &rx_desc->rx_sg;
		rx_wr->num_sge	= 1;
		rx_wr->next	= rx_wr + 1;
		my_rx_head = (my_rx_head + 1) & iser_conn->qp_max_recv_dtos_mask;
	}

	rx_wr--;
	rx_wr->next = NULL; /* mark end of work requests list */

	ib_conn->post_recv_buf_count += count;
	ib_ret	= ib_post_recv(ib_conn->qp, ib_conn->rx_wr, &rx_wr_failed);
	if (ib_ret) {
		iser_err("ib_post_recv failed ret=%d\n", ib_ret);
		ib_conn->post_recv_buf_count -= count;
	} else
		iser_conn->rx_desc_head = my_rx_head;
	return ib_ret;
}


/**
 * iser_start_send - Initiate a Send DTO operation
 *
 * returns 0 on success, -1 on failure
 */
int iser_post_send(struct ib_conn *ib_conn, struct iser_tx_desc *tx_desc,
		   bool signal)
{
	struct ib_send_wr *bad_wr, *wr = iser_tx_next_wr(tx_desc);
	int ib_ret;

	ib_dma_sync_single_for_device(ib_conn->device->ib_device,
				      tx_desc->dma_addr, ISER_HEADERS_LEN,
				      DMA_TO_DEVICE);

	wr->next = NULL;
	wr->wr_id = (uintptr_t)tx_desc;
	wr->sg_list = tx_desc->tx_sg;
	wr->num_sge = tx_desc->num_sge;
	wr->opcode = IB_WR_SEND;
	wr->send_flags = signal ? IB_SEND_SIGNALED : 0;

	ib_ret = ib_post_send(ib_conn->qp, &tx_desc->wrs[0].send, &bad_wr);
	if (ib_ret)
		iser_err("ib_post_send failed, ret:%d opcode:%d\n",
			 ib_ret, bad_wr->opcode);

	return ib_ret;
}

/**
 * is_iser_tx_desc - Indicate if the completion wr_id
 *     is a TX descriptor or not.
 * @iser_conn: iser connection
 * @wr_id: completion WR identifier
 *
 * Since we cannot rely on wc opcode in FLUSH errors
 * we must work around it by checking if the wr_id address
 * falls in the iser connection rx_descs buffer. If so
 * it is an RX descriptor, otherwize it is a TX.
 */
static inline bool
is_iser_tx_desc(struct iser_conn *iser_conn, void *wr_id)
{
	void *start = iser_conn->rx_descs;
	int len = iser_conn->num_rx_descs * sizeof(*iser_conn->rx_descs);

	if (wr_id >= start && wr_id < start + len)
		return false;

	return true;
}

/**
 * iser_handle_comp_error() - Handle error completion
 * @ib_conn:   connection RDMA resources
 * @wc:        work completion
 *
 * Notes: We may handle a FLUSH error completion and in this case
 *        we only cleanup in case TX type was DATAOUT. For non-FLUSH
 *        error completion we should also notify iscsi layer that
 *        connection is failed (in case we passed bind stage).
 */
static void
iser_handle_comp_error(struct ib_conn *ib_conn,
		       struct ib_wc *wc)
{
	void *wr_id = (void *)(uintptr_t)wc->wr_id;
	struct iser_conn *iser_conn = container_of(ib_conn, struct iser_conn,
						   ib_conn);

	if (wc->status != IB_WC_WR_FLUSH_ERR)
		if (iser_conn->iscsi_conn)
			iscsi_conn_failure(iser_conn->iscsi_conn,
					   ISCSI_ERR_CONN_FAILED);

	if (wc->wr_id == ISER_FASTREG_LI_WRID)
		return;

	if (is_iser_tx_desc(iser_conn, wr_id)) {
		struct iser_tx_desc *desc = wr_id;

		if (desc->type == ISCSI_TX_DATAOUT)
			kmem_cache_free(ig.desc_cache, desc);
	} else {
		ib_conn->post_recv_buf_count--;
	}
}

/**
 * iser_handle_wc - handle a single work completion
 * @wc: work completion
 *
 * Soft-IRQ context, work completion can be either
 * SEND or RECV, and can turn out successful or
 * with error (or flush error).
 */
static void iser_handle_wc(struct ib_wc *wc)
{
	struct ib_conn *ib_conn;
	struct iser_tx_desc *tx_desc;
	struct iser_rx_desc *rx_desc;

	ib_conn = wc->qp->qp_context;
	if (likely(wc->status == IB_WC_SUCCESS)) {
		if (wc->opcode == IB_WC_RECV) {
			rx_desc = (struct iser_rx_desc *)(uintptr_t)wc->wr_id;
			iser_rcv_completion(rx_desc, wc->byte_len,
					    ib_conn);
		} else
		if (wc->opcode == IB_WC_SEND) {
			tx_desc = (struct iser_tx_desc *)(uintptr_t)wc->wr_id;
			iser_snd_completion(tx_desc, ib_conn);
		} else {
			iser_err("Unknown wc opcode %d\n", wc->opcode);
		}
	} else {
		if (wc->status != IB_WC_WR_FLUSH_ERR)
			iser_err("%s (%d): wr id %llx vend_err %x\n",
				 ib_wc_status_msg(wc->status), wc->status,
				 wc->wr_id, wc->vendor_err);
		else
			iser_dbg("%s (%d): wr id %llx\n",
				 ib_wc_status_msg(wc->status), wc->status,
				 wc->wr_id);

		if (wc->wr_id == ISER_BEACON_WRID)
			/* all flush errors were consumed */
			complete(&ib_conn->flush_comp);
		else
			iser_handle_comp_error(ib_conn, wc);
	}
}

/**
 * iser_cq_tasklet_fn - iSER completion polling loop
 * @data: iSER completion context
 *
 * Soft-IRQ context, polling connection CQ until
 * either CQ was empty or we exausted polling budget
 */
static void iser_cq_tasklet_fn(unsigned long data)
{
	struct iser_comp *comp = (struct iser_comp *)data;
	struct ib_cq *cq = comp->cq;
	struct ib_wc *const wcs = comp->wcs;
	int i, n, completed = 0;

	while ((n = ib_poll_cq(cq, ARRAY_SIZE(comp->wcs), wcs)) > 0) {
		for (i = 0; i < n; i++)
			iser_handle_wc(&wcs[i]);

		completed += n;
		if (completed >= iser_cq_poll_limit)
			break;
	}

	/*
	 * It is assumed here that arming CQ only once its empty
	 * would not cause interrupts to be missed.
	 */
	ib_req_notify_cq(cq, IB_CQ_NEXT_COMP);

	iser_dbg("got %d completions\n", completed);
}

static void iser_cq_callback(struct ib_cq *cq, void *cq_context)
{
	struct iser_comp *comp = cq_context;

	tasklet_schedule(&comp->tasklet);
}

u8 iser_check_task_pi_status(struct iscsi_iser_task *iser_task,
			     enum iser_data_dir cmd_dir, sector_t *sector)
{
	struct iser_mem_reg *reg = &iser_task->rdma_reg[cmd_dir];
	struct iser_fr_desc *desc = reg->mem_h;
	unsigned long sector_size = iser_task->sc->device->sector_size;
	struct ib_mr_status mr_status;
	int ret;

	if (desc && desc->pi_ctx->sig_protected) {
		desc->pi_ctx->sig_protected = 0;
		ret = ib_check_mr_status(desc->pi_ctx->sig_mr,
					 IB_MR_CHECK_SIG_STATUS, &mr_status);
		if (ret) {
			pr_err("ib_check_mr_status failed, ret %d\n", ret);
			/* Not a lot we can do, return ambiguous guard error */
			*sector = 0;
			return 0x1;
		}

		if (mr_status.fail_status & IB_MR_CHECK_SIG_STATUS) {
			sector_t sector_off = mr_status.sig_err.sig_err_offset;

			sector_div(sector_off, sector_size + 8);
			*sector = scsi_get_lba(iser_task->sc) + sector_off;

			pr_err("PI error found type %d at sector %llx "
			       "expected %x vs actual %x\n",
			       mr_status.sig_err.err_type,
			       (unsigned long long)*sector,
			       mr_status.sig_err.expected,
			       mr_status.sig_err.actual);

			switch (mr_status.sig_err.err_type) {
			case IB_SIG_BAD_GUARD:
				return 0x1;
			case IB_SIG_BAD_REFTAG:
				return 0x3;
			case IB_SIG_BAD_APPTAG:
				return 0x2;
			}
		}
	}

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           config INFINIBAND_ISERT
	tristate "iSCSI Extensions for RDMA (iSER) target support"
	depends on INET && INFINIBAND_ADDR_TRANS && TARGET_CORE && ISCSI_TARGET
	---help---
	Support for iSCSI Extensions for RDMA (iSER) Target on Infiniband fabrics.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ccflags-y		:= -Idrivers/target -Idrivers/target/iscsi
obj-$(CONFIG_INFINIBAND_ISERT)	+= ib_isert.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             #include <linux/socket.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>

#define DRV_NAME	"isert"
#define PFX		DRV_NAME ": "

#define isert_dbg(fmt, arg...)				 \
	do {						 \
		if (unlikely(isert_debug_level > 2))	 \
			printk(KERN_DEBUG PFX "%s: " fmt,\
				__func__ , ## arg);	 \
	} while (0)

#define isert_warn(fmt, arg...)				\
	do {						\
		if (unlikely(isert_debug_level > 0))	\
			pr_warn(PFX "%s: " fmt,         \
				__func__ , ## arg);	\
	} while (0)

#define isert_info(fmt, arg...)				\
	do {						\
		if (unlikely(isert_debug_level > 1))	\
			pr_info(PFX "%s: " fmt,         \
				__func__ , ## arg);	\
	} while (0)

#define isert_err(fmt, arg...) \
	pr_err(PFX "%s: " fmt, __func__ , ## arg)

#define ISCSI_ISER_SG_TABLESIZE		256
#define ISER_FASTREG_LI_WRID		0xffffffffffffffffULL
#define ISER_BEACON_WRID               0xfffffffffffffffeULL

enum isert_desc_type {
	ISCSI_TX_CONTROL,
	ISCSI_TX_DATAIN
};

enum iser_ib_op_code {
	ISER_IB_RECV,
	ISER_IB_SEND,
	ISER_IB_RDMA_WRITE,
	ISER_IB_RDMA_READ,
};

enum iser_conn_state {
	ISER_CONN_INIT,
	ISER_CONN_UP,
	ISER_CONN_BOUND,
	ISER_CONN_FULL_FEATURE,
	ISER_CONN_TERMINATING,
	ISER_CONN_DOWN,
};

struct iser_rx_desc {
	struct iser_hdr iser_header;
	struct iscsi_hdr iscsi_header;
	char		data[ISER_RECV_DATA_SEG_LEN];
	u64		dma_addr;
	struct ib_sge	rx_sg;
	char		pad[ISER_RX_PAD_SIZE];
} __packed;

struct iser_tx_desc {
	struct iser_hdr iser_header;
	struct iscsi_hdr iscsi_header;
	enum isert_desc_type type;
	u64		dma_addr;
	struct ib_sge	tx_sg[2];
	int		num_sge;
	struct isert_cmd *isert_cmd;
	struct ib_send_wr send_wr;
} __packed;

enum isert_indicator {
	ISERT_PROTECTED		= 1 << 0,
	ISERT_DATA_KEY_VALID	= 1 << 1,
	ISERT_PROT_KEY_VALID	= 1 << 2,
	ISERT_SIG_KEY_VALID	= 1 << 3,
};

struct pi_context {
	struct ib_mr		       *prot_mr;
	struct ib_mr		       *sig_mr;
};

struct fast_reg_descriptor {
	struct list_head		list;
	struct ib_mr		       *data_mr;
	u8				ind;
	struct pi_context	       *pi_ctx;
};

struct isert_data_buf {
	struct scatterlist     *sg;
	int			nents;
	u32			sg_off;
	u32			len; /* cur_rdma_length */
	u32			offset;
	unsigned int		dma_nents;
	enum dma_data_direction dma_dir;
};

enum {
	DATA = 0,
	PROT = 1,
	SIG = 2,
};

struct isert_rdma_wr {
	struct isert_cmd	*isert_cmd;
	enum iser_ib_op_code	iser_ib_op;
	struct ib_sge		*ib_sge;
	struct ib_sge		s_ib_sge;
	int			rdma_wr_num;
	struct ib_rdma_wr	*rdma_wr;
	struct ib_rdma_wr	s_rdma_wr;
	struct ib_sge		ib_sg[3];
	struct isert_data_buf	data;
	struct isert_data_buf	prot;
	struct fast_reg_descriptor *fr_desc;
};

struct isert_cmd {
	uint32_t		read_stag;
	uint32_t		write_stag;
	uint64_t		read_va;
	uint64_t		write_va;
	u64			pdu_buf_dma;
	u32			pdu_buf_len;
	struct isert_conn	*conn;
	struct iscsi_cmd	*iscsi_cmd;
	struct iser_tx_desc	tx_desc;
	struct iser_rx_desc	*rx_desc;
	struct isert_rdma_wr	rdma_wr;
	struct work_struct	comp_work;
	struct scatterlist	sg;
};

struct isert_device;

struct isert_conn {
	enum iser_conn_state	state;
	u32			responder_resources;
	u32			initiator_depth;
	bool			pi_support;
	u32			max_sge;
	char			*login_buf;
	char			*login_req_buf;
	char			*login_rsp_buf;
	u64			login_req_dma;
	int			login_req_len;
	u64			login_rsp_dma;
	struct iser_rx_desc	*rx_descs;
	struct ib_recv_wr	rx_wr[ISERT_QP_MAX_RECV_DTOS];
	struct iscsi_conn	*conn;
	struct list_head	node;
	struct completion	login_comp;
	struct completion	login_req_comp;
	struct iser_tx_desc	login_tx_desc;
	struct rdma_cm_id	*cm_id;
	struct ib_qp		*qp;
	struct isert_device	*device;
	struct mutex		mutex;
	struct completion	wait;
	struct completion	wait_comp_err;
	struct kref		kref;
	struct list_head	fr_pool;
	int			fr_pool_size;
	/* lock to protect fastreg pool */
	spinlock_t		pool_lock;
	struct work_struct	release_work;
	struct ib_recv_wr       beacon;
	bool                    logout_posted;
};

#define ISERT_MAX_CQ 64

/**
 * struct isert_comp - iSER completion context
 *
 * @device:     pointer to device handle
 * @cq:         completion queue
 * @wcs:        work completion array
 * @active_qps: Number of active QPs attached
 *              to completion context
 * @work:       completion work handle
 */
struct isert_comp {
	struct isert_device     *device;
	struct ib_cq		*cq;
	struct ib_wc		 wcs[16];
	int                      active_qps;
	struct work_struct	 work;
};

struct isert_device {
	int			use_fastreg;
	bool			pi_capable;
	int			refcount;
	struct ib_device	*ib_device;
	struct ib_pd		*pd;
	struct isert_comp	*comps;
	int                     comps_used;
	struct list_head	dev_node;
	struct ib_device_attr	dev_attr;
	int			(*reg_rdma_mem)(struct iscsi_conn *conn,
						    struct iscsi_cmd *cmd,
						    struct isert_rdma_wr *wr);
	void			(*unreg_rdma_mem)(struct isert_cmd *isert_cmd,
						  struct isert_conn *isert_conn);
};

struct isert_np {
	struct iscsi_np         *np;
	struct semaphore	sem;
	struct rdma_cm_id	*cm_id;
	struct mutex		mutex;
	struct list_head	accepted;
	struct list_head	pending;
};
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /* From iscsi_iser.h */

struct iser_hdr {
	u8	flags;
	u8	rsvd[3];
	__be32	write_stag; /* write rkey */
	__be64	write_va;
	__be32	read_stag;  /* read rkey */
	__be64	read_va;
} __packed;

/*Constant PDU lengths calculations */
#define ISER_HEADERS_LEN  (sizeof(struct iser_hdr) + sizeof(struct iscsi_hdr))

#define ISER_RECV_DATA_SEG_LEN  8192
#define ISER_RX_PAYLOAD_SIZE    (ISER_HEADERS_LEN + ISER_RECV_DATA_SEG_LEN)
#define ISER_RX_LOGIN_SIZE      (ISER_HEADERS_LEN + ISCSI_DEF_MAX_RECV_SEG_LEN)

/* QP settings */
/* Maximal bounds on received asynchronous PDUs */
#define ISERT_MAX_TX_MISC_PDUS	4 /* NOOP_IN(2) , ASYNC_EVENT(2)   */

#define ISERT_MAX_RX_MISC_PDUS	6 /* NOOP_OUT(2), TEXT(1),         *
				   * SCSI_TMFUNC(2), LOGOUT(1) */

#define ISCSI_DEF_XMIT_CMDS_MAX 128 /* from libiscsi.h, must be power of 2 */

#define ISERT_QP_MAX_RECV_DTOS	(ISCSI_DEF_XMIT_CMDS_MAX)

#define ISERT_MIN_POSTED_RX	(ISCSI_DEF_XMIT_CMDS_MAX >> 2)

#define ISERT_INFLIGHT_DATAOUTS	8

#define ISERT_QP_MAX_REQ_DTOS	(ISCSI_DEF_XMIT_CMDS_MAX *    \
				(1 + ISERT_INFLIGHT_DATAOUTS) + \
				ISERT_MAX_TX_MISC_PDUS	+ \
				ISERT_MAX_RX_MISC_PDUS)

#define ISER_RX_PAD_SIZE	(ISER_RECV_DATA_SEG_LEN + 4096 - \
		(ISER_RX_PAYLOAD_SIZE + sizeof(u64) + sizeof(struct ib_sge)))

#define ISER_VER	0x10
#define ISER_WSV	0x08
#define ISER_RSV	0x04
#define ISCSI_CTRL	0x10
#define ISER_HELLO	0x20
#define ISER_HELLORPLY	0x30
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  obj-$(CONFIG_INFINIBAND_SRP)			+= ib_srp.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     config INFINIBAND_SRP
	tristate "InfiniBand SCSI RDMA Protocol"
	depends on SCSI
	select SCSI_SRP_ATTRS
	---help---
	  Support for the SCSI RDMA Protocol over InfiniBand.  This
	  allows you to access storage devices that speak SRP over
	  InfiniBand.

	  The SRP protocol is defined by the INCITS T10 technical
	  committee.  See <http://www.t10.org/>.

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
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

#ifndef IB_SRP_H
#define IB_SRP_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/scatterlist.h>

#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>

#include <rdma/ib_verbs.h>
#include <rdma/ib_sa.h>
#include <rdma/ib_cm.h>
#include <rdma/ib_fmr_pool.h>

enum {
	SRP_PATH_REC_TIMEOUT_MS	= 1000,
	SRP_ABORT_TIMEOUT_MS	= 5000,

	SRP_PORT_REDIRECT	= 1,
	SRP_DLID_REDIRECT	= 2,
	SRP_STALE_CONN		= 3,

	SRP_DEF_SG_TABLESIZE	= 12,

	SRP_DEFAULT_QUEUE_SIZE	= 1 << 6,
	SRP_RSP_SQ_SIZE		= 1,
	SRP_TSK_MGMT_SQ_SIZE	= 1,
	SRP_DEFAULT_CMD_SQ_SIZE = SRP_DEFAULT_QUEUE_SIZE - SRP_RSP_SQ_SIZE -
				  SRP_TSK_MGMT_SQ_SIZE,

	SRP_MAX_PAGES_PER_MR	= 512,

	LOCAL_INV_WR_ID_MASK	= 1,
	FAST_REG_WR_ID_MASK	= 2,

	SRP_LAST_WR_ID		= 0xfffffffcU,
};

enum {
	SRP_TAG_NO_REQ		= ~0U,
	SRP_TAG_TSK_MGMT	= BIT(31),
};

enum srp_target_state {
	SRP_TARGET_SCANNING,
	SRP_TARGET_LIVE,
	SRP_TARGET_REMOVED,
};

enum srp_iu_type {
	SRP_IU_CMD,
	SRP_IU_TSK_MGMT,
	SRP_IU_RSP,
};

/*
 * @mr_page_mask: HCA memory registration page mask.
 * @mr_page_size: HCA memory registration page size.
 * @mr_max_size: Maximum size in bytes of a single FMR / FR registration
 *   request.
 */
struct srp_device {
	struct list_head	dev_list;
	struct ib_device       *dev;
	struct ib_pd	       *pd;
	struct ib_mr	       *global_mr;
	u64			mr_page_mask;
	int			mr_page_size;
	int			mr_max_size;
	int			max_pages_per_mr;
	bool			has_fmr;
	bool			has_fr;
	bool			use_fmr;
	bool			use_fast_reg;
};

struct srp_host {
	struct srp_device      *srp_dev;
	u8			port;
	struct device		dev;
	struct list_head	target_list;
	spinlock_t		target_lock;
	struct completion	released;
	struct list_head	list;
	struct mutex		add_target_mutex;
};

struct srp_request {
	struct scsi_cmnd       *scmnd;
	struct srp_iu	       *cmd;
	union {
		struct ib_pool_fmr **fmr_list;
		struct srp_fr_desc **fr_list;
	};
	u64		       *map_page;
	struct srp_direct_buf  *indirect_desc;
	dma_addr_t		indirect_dma_addr;
	short			nmdesc;
};

/**
 * struct srp_rdma_ch
 * @comp_vector: Completion vector used by this RDMA channel.
 */
struct srp_rdma_ch {
	/* These are RW in the hot path, and commonly used together */
	struct list_head	free_tx;
	spinlock_t		lock;
	s32			req_lim;

	/* These are read-only in the hot path */
	struct srp_target_port *target ____cacheline_aligned_in_smp;
	struct ib_cq	       *send_cq;
	struct ib_cq	       *recv_cq;
	struct ib_qp	       *qp;
	union {
		struct ib_fmr_pool     *fmr_pool;
		struct srp_fr_pool     *fr_pool;
	};

	/* Everything above this point is used in the hot path of
	 * command processing. Try to keep them packed into cachelines.
	 */

	struct completion	done;
	int			status;

	struct ib_sa_path_rec	path;
	struct ib_sa_query     *path_query;
	int			path_query_id;

	struct ib_cm_id	       *cm_id;
	struct srp_iu	      **tx_ring;
	struct srp_iu	      **rx_ring;
	struct srp_request     *req_ring;
	int			max_ti_iu_len;
	int			comp_vector;

	u64			tsk_mgmt_tag;
	struct completion	tsk_mgmt_done;
	u8			tsk_mgmt_status;
	bool			connected;
};

/**
 * struct srp_target_port
 * @comp_vector: Completion vector used by the first RDMA channel created for
 *   this target port.
 */
struct srp_target_port {
	/* read and written in the hot path */
	spinlock_t		lock;

	/* read only in the hot path */
	struct ib_mr		*global_mr;
	struct srp_rdma_ch	*ch;
	u32			ch_count;
	u32			lkey;
	enum srp_target_state	state;
	unsigned int		max_iu_len;
	unsigned int		cmd_sg_cnt;
	unsigned int		indirect_size;
	bool			allow_ext_sg;

	/* other member variables */
	union ib_gid		sgid;
	__be64			id_ext;
	__be64			ioc_guid;
	__be64			service_id;
	__be64			initiator_ext;
	u16			io_class;
	struct srp_host	       *srp_host;
	struct Scsi_Host       *scsi_host;
	struct srp_rport       *rport;
	char			target_name[32];
	unsigned int		scsi_id;
	unsigned int		sg_tablesize;
	int			queue_size;
	int			req_ring_size;
	int			comp_vector;
	int			tl_retry_count;

	union ib_gid		orig_dgid;
	__be16			pkey;

	u32			rq_tmo_jiffies;

	int			zero_req_lim;

	struct work_struct	tl_err_work;
	struct work_struct	remove_work;

	struct list_head	list;
	bool			qp_in_error;
};

struct srp_iu {
	struct list_head	list;
	u64			dma;
	void		       *buf;
	size_t			size;
	enum dma_data_direction	direction;
};

/**
 * struct srp_fr_desc - fast registration work request arguments
 * @entry: Entry in srp_fr_pool.free_list.
 * @mr:    Memory region.
 * @frpl:  Fast registration page list.
 */
struct srp_fr_desc {
	struct list_head		entry;
	struct ib_mr			*mr;
};

/**
 * struct srp_fr_pool - pool of fast registration descriptors
 *
 * An entry is available for allocation if and only if it occurs in @free_list.
 *
 * @size:      Number of descriptors in this pool.
 * @max_page_list_len: Maximum fast registration work request page list length.
 * @lock:      Protects free_list.
 * @free_list: List of free descriptors.
 * @desc:      Fast registration descriptor pool.
 */
struct srp_fr_pool {
	int			size;
	int			max_page_list_len;
	spinlock_t		lock;
	struct list_head	free_list;
	struct srp_fr_desc	desc[0];
};

/**
 * struct srp_map_state - per-request DMA memory mapping state
 * @desc:	    Pointer to the element of the SRP buffer descriptor array
 *		    that is being filled in.
 * @pages:	    Array with DMA addresses of pages being considered for
 *		    memory registration.
 * @base_dma_addr:  DMA address of the first page that has not yet been mapped.
 * @dma_len:	    Number of bytes that will be registered with the next
 *		    FMR or FR memory registration call.
 * @total_len:	    Total number of bytes in the sg-list being mapped.
 * @npages:	    Number of page addresses in the pages[] array.
 * @nmdesc:	    Number of FMR or FR memory descriptors used for mapping.
 * @ndesc:	    Number of SRP buffer descriptors that have been filled in.
 */
struct srp_map_state {
	union {
		struct {
			struct ib_pool_fmr **next;
			struct ib_pool_fmr **end;
		} fmr;
		struct {
			struct srp_fr_desc **next;
			struct srp_fr_desc **end;
		} fr;
		struct {
			void		   **next;
			void		   **end;
		} gen;
	};
	struct srp_direct_buf  *desc;
	union {
		u64			*pages;
		struct scatterlist	*sg;
	};
	dma_addr_t		base_dma_addr;
	u32			dma_len;
	u32			total_len;
	unsigned int		npages;
	unsigned int		nmdesc;
	unsigned int		ndesc;
};

#endif /* IB_SRP_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                              config INFINIBAND_SRPT
	tristate "InfiniBand SCSI RDMA Protocol target support"
	depends on INFINIBAND && TARGET_CORE
	---help---

	  Support for the SCSI RDMA Protocol (SRP) Target driver. The
	  SRP protocol is a protocol that allows an initiator to access
	  a block storage device on another host (target) over a network
	  that supports the RDMA protocol. Currently the RDMA protocol is
	  supported by InfiniBand and by iWarp network hardware. More
	  information about the SRP protocol can be found on the website
	  of the INCITS T10 technical committee (http://www.t10.org/).
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       ccflags-y			:= -Idrivers/target
obj-$(CONFIG_INFINIBAND_SRPT)	+= ib_srpt.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright (c) 2006 - 2009 Mellanox Technology Inc.  All rights reserved.
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

#ifndef IB_DM_MAD_H
#define IB_DM_MAD_H

#include <linux/types.h>

#include <rdma/ib_mad.h>

enum {
	/*
	 * See also section 13.4.7 Status Field, table 115 MAD Common Status
	 * Field Bit Values and also section 16.3.1.1 Status Field in the
	 * InfiniBand Architecture Specification.
	 */
	DM_MAD_STATUS_UNSUP_METHOD = 0x0008,
	DM_MAD_STATUS_UNSUP_METHOD_ATTR = 0x000c,
	DM_MAD_STATUS_INVALID_FIELD = 0x001c,
	DM_MAD_STATUS_NO_IOC = 0x0100,

	/*
	 * See also the Device Management chapter, section 16.3.3 Attributes,
	 * table 279 Device Management Attributes in the InfiniBand
	 * Architecture Specification.
	 */
	DM_ATTR_CLASS_PORT_INFO = 0x01,
	DM_ATTR_IOU_INFO = 0x10,
	DM_ATTR_IOC_PROFILE = 0x11,
	DM_ATTR_SVC_ENTRIES = 0x12
};

struct ib_dm_hdr {
	u8 reserved[28];
};

/*
 * Structure of management datagram sent by the SRP target implementation.
 * Contains a management datagram header, reliable multi-packet transaction
 * protocol (RMPP) header and ib_dm_hdr. Notes:
 * - The SRP target implementation does not use RMPP or ib_dm_hdr when sending
 *   management datagrams.
 * - The header size must be exactly 64 bytes (IB_MGMT_DEVICE_HDR), since this
 *   is the header size that is passed to ib_create_send_mad() in ib_srpt.c.
 * - The maximum supported size for a management datagram when not using RMPP
 *   is 256 bytes -- 64 bytes header and 192 (IB_MGMT_DEVICE_DATA) bytes data.
 */
struct ib_dm_mad {
	struct ib_mad_hdr mad_hdr;
	struct ib_rmpp_hdr rmpp_hdr;
	struct ib_dm_hdr dm_hdr;
	u8 data[IB_MGMT_DEVICE_DATA];
};

/*
 * IOUnitInfo as defined in section 16.3.3.3 IOUnitInfo of the InfiniBand
 * Architecture Specification.
 */
struct ib_dm_iou_info {
	__be16 change_id;
	u8 max_controllers;
	u8 op_rom;
	u8 controller_list[128];
};

/*
 * IOControllerprofile as defined in section 16.3.3.4 IOControllerProfile of
 * the InfiniBand Architecture Specification.
 */
struct ib_dm_ioc_profile {
	__be64 guid;
	__be32 vendor_id;
	__be32 device_id;
	__be16 device_version;
	__be16 reserved1;
	__be32 subsys_vendor_id;
	__be32 subsys_device_id;
	__be16 io_class;
	__be16 io_subclass;
	__be16 protocol;
	__be16 protocol_version;
	__be16 service_conn;
	__be16 initiators_supported;
	__be16 send_queue_depth;
	u8 reserved2;
	u8 rdma_read_depth;
	__be32 send_size;
	__be32 rdma_size;
	u8 op_cap_mask;
	u8 svc_cap_mask;
	u8 num_svc_entries;
	u8 reserved3[9];
	u8 id_string[64];
};

struct ib_dm_svc_entry {
	u8 name[40];
	__be64 id;
};

/*
 * See also section 16.3.3.5 ServiceEntries in the InfiniBand Architecture
 * Specification. See also section B.7, table B.8 in the T10 SRP r16a document.
 */
struct ib_dm_svc_entries {
	struct ib_dm_svc_entry service_entries[4];
};

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright (c) 2006 - 2009 Mellanox Technology Inc.  All rights reserved.
 * Copyright (C) 2009 - 2010 Bart Van Assche <bvanassche@acm.org>.
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

#ifndef IB_SRPT_H
#define IB_SRPT_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/wait.h>

#include <rdma/ib_verbs.h>
#include <rdma/ib_sa.h>
#include <rdma/ib_cm.h>

#include <scsi/srp.h>

#include "ib_dm_mad.h"

/*
 * The prefix the ServiceName field must start with in the device management
 * ServiceEntries attribute pair. See also the SRP specification.
 */
#define SRP_SERVICE_NAME_PREFIX		"SRP.T10:"

enum {
	/*
	 * SRP IOControllerProfile attributes for SRP target ports that have
	 * not been defined in <scsi/srp.h>. Source: section B.7, table B.7
	 * in the SRP specification.
	 */
	SRP_PROTOCOL = 0x0108,
	SRP_PROTOCOL_VERSION = 0x0001,
	SRP_IO_SUBCLASS = 0x609e,
	SRP_SEND_TO_IOC = 0x01,
	SRP_SEND_FROM_IOC = 0x02,
	SRP_RDMA_READ_FROM_IOC = 0x08,
	SRP_RDMA_WRITE_FROM_IOC = 0x20,

	/*
	 * srp_login_cmd.req_flags bitmasks. See also table 9 in the SRP
	 * specification.
	 */
	SRP_MTCH_ACTION = 0x03, /* MULTI-CHANNEL ACTION */
	SRP_LOSOLNT = 0x10, /* logout solicited notification */
	SRP_CRSOLNT = 0x20, /* credit request solicited notification */
	SRP_AESOLNT = 0x40, /* asynchronous event solicited notification */

	/*
	 * srp_cmd.sol_nt / srp_tsk_mgmt.sol_not bitmasks. See also tables
	 * 18 and 20 in the SRP specification.
	 */
	SRP_SCSOLNT = 0x02, /* SCSOLNT = successful solicited notification */
	SRP_UCSOLNT = 0x04, /* UCSOLNT = unsuccessful solicited notification */

	/*
	 * srp_rsp.sol_not / srp_t_logout.sol_not bitmasks. See also tables
	 * 16 and 22 in the SRP specification.
	 */
	SRP_SOLNT = 0x01, /* SOLNT = solicited notification */

	/* See also table 24 in the SRP specification. */
	SRP_TSK_MGMT_SUCCESS = 0x00,
	SRP_TSK_MGMT_FUNC_NOT_SUPP = 0x04,
	SRP_TSK_MGMT_FAILED = 0x05,

	/* See also table 21 in the SRP specification. */
	SRP_CMD_SIMPLE_Q = 0x0,
	SRP_CMD_HEAD_OF_Q = 0x1,
	SRP_CMD_ORDERED_Q = 0x2,
	SRP_CMD_ACA = 0x4,

	SRP_LOGIN_RSP_MULTICHAN_NO_CHAN = 0x0,
	SRP_LOGIN_RSP_MULTICHAN_TERMINATED = 0x1,
	SRP_LOGIN_RSP_MULTICHAN_MAINTAINED = 0x2,

	SRPT_DEF_SG_TABLESIZE = 128,
	SRPT_DEF_SG_PER_WQE = 16,

	MIN_SRPT_SQ_SIZE = 16,
	DEF_SRPT_SQ_SIZE = 4096,
	SRPT_RQ_SIZE = 128,
	MIN_SRPT_SRQ_SIZE = 4,
	DEFAULT_SRPT_SRQ_SIZE = 4095,
	MAX_SRPT_SRQ_SIZE = 65535,
	MAX_SRPT_RDMA_SIZE = 1U << 24,
	MAX_SRPT_RSP_SIZE = 1024,

	MIN_MAX_REQ_SIZE = 996,
	DEFAULT_MAX_REQ_SIZE
		= sizeof(struct srp_cmd)/*48*/
		+ sizeof(struct srp_indirect_buf)/*20*/
		+ 128 * sizeof(struct srp_direct_buf)/*16*/,

	MIN_MAX_RSP_SIZE = sizeof(struct srp_rsp)/*36*/ + 4,
	DEFAULT_MAX_RSP_SIZE = 256, /* leaves 220 bytes for sense data */

	DEFAULT_MAX_RDMA_SIZE = 65536,
};

enum srpt_opcode {
	SRPT_RECV,
	SRPT_SEND,
	SRPT_RDMA_MID,
	SRPT_RDMA_ABORT,
	SRPT_RDMA_READ_LAST,
	SRPT_RDMA_WRITE_LAST,
};

static inline u64 encode_wr_id(u8 opcode, u32 idx)
{
	return ((u64)opcode << 32) | idx;
}
static inline enum srpt_opcode opcode_from_wr_id(u64 wr_id)
{
	return wr_id >> 32;
}
static inline u32 idx_from_wr_id(u64 wr_id)
{
	return (u32)wr_id;
}

struct rdma_iu {
	u64		raddr;
	u32		rkey;
	struct ib_sge	*sge;
	u32		sge_cnt;
	int		mem_id;
};

/**
 * enum srpt_command_state - SCSI command state managed by SRPT.
 * @SRPT_STATE_NEW:           New command arrived and is being processed.
 * @SRPT_STATE_NEED_DATA:     Processing a write or bidir command and waiting
 *                            for data arrival.
 * @SRPT_STATE_DATA_IN:       Data for the write or bidir command arrived and is
 *                            being processed.
 * @SRPT_STATE_CMD_RSP_SENT:  SRP_RSP for SRP_CMD has been sent.
 * @SRPT_STATE_MGMT:          Processing a SCSI task management command.
 * @SRPT_STATE_MGMT_RSP_SENT: SRP_RSP for SRP_TSK_MGMT has been sent.
 * @SRPT_STATE_DONE:          Command processing finished successfully, command
 *                            processing has been aborted or command processing
 *                            failed.
 */
enum srpt_command_state {
	SRPT_STATE_NEW		 = 0,
	SRPT_STATE_NEED_DATA	 = 1,
	SRPT_STATE_DATA_IN	 = 2,
	SRPT_STATE_CMD_RSP_SENT	 = 3,
	SRPT_STATE_MGMT		 = 4,
	SRPT_STATE_MGMT_RSP_SENT = 5,
	SRPT_STATE_DONE		 = 6,
};

/**
 * struct srpt_ioctx - Shared SRPT I/O context information.
 * @buf:   Pointer to the buffer.
 * @dma:   DMA address of the buffer.
 * @index: Index of the I/O context in its ioctx_ring array.
 */
struct srpt_ioctx {
	void			*buf;
	dma_addr_t		dma;
	uint32_t		index;
};

/**
 * struct srpt_recv_ioctx - SRPT receive I/O context.
 * @ioctx:     See above.
 * @wait_list: Node for insertion in srpt_rdma_ch.cmd_wait_list.
 */
struct srpt_recv_ioctx {
	struct srpt_ioctx	ioctx;
	struct list_head	wait_list;
};

/**
 * struct srpt_send_ioctx - SRPT send I/O context.
 * @ioctx:       See above.
 * @ch:          Channel pointer.
 * @free_list:   Node in srpt_rdma_ch.free_list.
 * @n_rbuf:      Number of data buffers in the received SRP command.
 * @rbufs:       Pointer to SRP data buffer array.
 * @single_rbuf: SRP data buffer if the command has only a single buffer.
 * @sg:          Pointer to sg-list associated with this I/O context.
 * @sg_cnt:      SG-list size.
 * @mapped_sg_count: ib_dma_map_sg() return value.
 * @n_rdma_ius:  Number of elements in the rdma_ius array.
 * @rdma_ius:    Array with information about the RDMA mapping.
 * @tag:         Tag of the received SRP information unit.
 * @spinlock:    Protects 'state'.
 * @state:       I/O context state.
 * @rdma_aborted: If initiating a multipart RDMA transfer failed, whether
 * 		 the already initiated transfers have finished.
 * @cmd:         Target core command data structure.
 * @sense_data:  SCSI sense data.
 */
struct srpt_send_ioctx {
	struct srpt_ioctx	ioctx;
	struct srpt_rdma_ch	*ch;
	struct rdma_iu		*rdma_ius;
	struct srp_direct_buf	*rbufs;
	struct srp_direct_buf	single_rbuf;
	struct scatterlist	*sg;
	struct list_head	free_list;
	spinlock_t		spinlock;
	enum srpt_command_state	state;
	bool			rdma_aborted;
	struct se_cmd		cmd;
	struct completion	tx_done;
	int			sg_cnt;
	int			mapped_sg_count;
	u16			n_rdma_ius;
	u8			n_rdma;
	u8			n_rbuf;
	bool			queue_status_only;
	u8			sense_data[TRANSPORT_SENSE_BUFFER];
};

/**
 * enum rdma_ch_state - SRP channel state.
 * @CH_CONNECTING:	 QP is in RTR state; waiting for RTU.
 * @CH_LIVE:		 QP is in RTS state.
 * @CH_DISCONNECTING:    DREQ has been received; waiting for DREP
 *                       or DREQ has been send and waiting for DREP
 *                       or .
 * @CH_DRAINING:	 QP is in ERR state; waiting for last WQE event.
 * @CH_RELEASING:	 Last WQE event has been received; releasing resources.
 */
enum rdma_ch_state {
	CH_CONNECTING,
	CH_LIVE,
	CH_DISCONNECTING,
	CH_DRAINING,
	CH_RELEASING
};

/**
 * struct srpt_rdma_ch - RDMA channel.
 * @wait_queue:    Allows the kernel thread to wait for more work.
 * @thread:        Kernel thread that processes the IB queues associated with
 *                 the channel.
 * @cm_id:         IB CM ID associated with the channel.
 * @qp:            IB queue pair used for communicating over this channel.
 * @cq:            IB completion queue for this channel.
 * @rq_size:       IB receive queue size.
 * @rsp_size	   IB response message size in bytes.
 * @sq_wr_avail:   number of work requests available in the send queue.
 * @sport:         pointer to the information of the HCA port used by this
 *                 channel.
 * @i_port_id:     128-bit initiator port identifier copied from SRP_LOGIN_REQ.
 * @t_port_id:     128-bit target port identifier copied from SRP_LOGIN_REQ.
 * @max_ti_iu_len: maximum target-to-initiator information unit length.
 * @req_lim:       request limit: maximum number of requests that may be sent
 *                 by the initiator without having received a response.
 * @req_lim_delta: Number of credits not yet sent back to the initiator.
 * @spinlock:      Protects free_list and state.
 * @free_list:     Head of list with free send I/O contexts.
 * @state:         channel state. See also enum rdma_ch_state.
 * @ioctx_ring:    Send ring.
 * @wc:            IB work completion array for srpt_process_completion().
 * @list:          Node for insertion in the srpt_device.rch_list list.
 * @cmd_wait_list: List of SCSI commands that arrived before the RTU event. This
 *                 list contains struct srpt_ioctx elements and is protected
 *                 against concurrent modification by the cm_id spinlock.
 * @sess:          Session information associated with this SRP channel.
 * @sess_name:     Session name.
 * @release_work:  Allows scheduling of srpt_release_channel().
 * @release_done:  Enables waiting for srpt_release_channel() completion.
 */
struct srpt_rdma_ch {
	wait_queue_head_t	wait_queue;
	struct task_struct	*thread;
	struct ib_cm_id		*cm_id;
	struct ib_qp		*qp;
	struct ib_cq		*cq;
	int			rq_size;
	u32			rsp_size;
	atomic_t		sq_wr_avail;
	struct srpt_port	*sport;
	u8			i_port_id[16];
	u8			t_port_id[16];
	int			max_ti_iu_len;
	atomic_t		req_lim;
	atomic_t		req_lim_delta;
	spinlock_t		spinlock;
	struct list_head	free_list;
	enum rdma_ch_state	state;
	struct srpt_send_ioctx	**ioctx_ring;
	struct ib_wc		wc[16];
	struct list_head	list;
	struct list_head	cmd_wait_list;
	struct se_session	*sess;
	u8			sess_name[36];
	struct work_struct	release_work;
	struct completion	*release_done;
	bool			in_shutdown;
};

/**
 * struct srpt_port_attib - Attributes for SRPT port
 * @srp_max_rdma_size: Maximum size of SRP RDMA transfers for new connections.
 * @srp_max_rsp_size: Maximum size of SRP response messages in bytes.
 * @srp_sq_size: Shared receive queue (SRQ) size.
 */
struct srpt_port_attrib {
	u32			srp_max_rdma_size;
	u32			srp_max_rsp_size;
	u32			srp_sq_size;
};

/**
 * struct srpt_port - Information associated by SRPT with a single IB port.
 * @sdev:      backpointer to the HCA information.
 * @mad_agent: per-port management datagram processing information.
 * @enabled:   Whether or not this target port is enabled.
 * @port_guid: ASCII representation of Port GUID
 * @port:      one-based port number.
 * @sm_lid:    cached value of the port's sm_lid.
 * @lid:       cached value of the port's lid.
 * @gid:       cached value of the port's gid.
 * @port_acl_lock spinlock for port_acl_list:
 * @work:      work structure for refreshing the aforementioned cached values.
 * @port_tpg_1 Target portal group = 1 data.
 * @port_wwn:  Target core WWN data.
 * @port_acl_list: Head of the list with all node ACLs for this port.
 */
struct srpt_port {
	struct srpt_device	*sdev;
	struct ib_mad_agent	*mad_agent;
	bool			enabled;
	u8			port_guid[64];
	u8			port;
	u16			sm_lid;
	u16			lid;
	union ib_gid		gid;
	spinlock_t		port_acl_lock;
	struct work_struct	work;
	struct se_portal_group	port_tpg_1;
	struct se_wwn		port_wwn;
	struct list_head	port_acl_list;
	struct srpt_port_attrib port_attrib;
};

/**
 * struct srpt_device - Information associated by SRPT with a single HCA.
 * @device:        Backpointer to the struct ib_device managed by the IB core.
 * @pd:            IB protection domain.
 * @mr:            L_Key (local key) with write access to all local memory.
 * @srq:           Per-HCA SRQ (shared receive queue).
 * @cm_id:         Connection identifier.
 * @dev_attr:      Attributes of the InfiniBand device as obtained during the
 *                 ib_client.add() callback.
 * @srq_size:      SRQ size.
 * @ioctx_ring:    Per-HCA SRQ.
 * @rch_list:      Per-device channel list -- see also srpt_rdma_ch.list.
 * @ch_releaseQ:   Enables waiting for removal from rch_list.
 * @spinlock:      Protects rch_list and tpg.
 * @port:          Information about the ports owned by this HCA.
 * @event_handler: Per-HCA asynchronous IB event handler.
 * @list:          Node in srpt_dev_list.
 */
struct srpt_device {
	struct ib_device	*device;
	struct ib_pd		*pd;
	struct ib_srq		*srq;
	struct ib_cm_id		*cm_id;
	struct ib_device_attr	dev_attr;
	int			srq_size;
	struct srpt_recv_ioctx	**ioctx_ring;
	struct list_head	rch_list;
	wait_queue_head_t	ch_releaseQ;
	spinlock_t		spinlock;
	struct srpt_port	port[2];
	struct ib_event_handler	event_handler;
	struct list_head	list;
};

/**
 * struct srpt_node_acl - Per-initiator ACL data (managed via configfs).
 * @nacl:      Target core node ACL information.
 * @i_port_id: 128-bit SRP initiator port ID.
 * @sport:     port information.
 * @list:      Element of the per-HCA ACL list.
 */
struct srpt_node_acl {
	struct se_node_acl	nacl;
	u8			i_port_id[16];
	struct srpt_port	*sport;
	struct list_head	list;
};

#endif				/* IB_SRPT_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 #
# Input device configuration
#

menu "Input device support"
	depends on !UML

config INPUT
	tristate "Generic input layer (needed for keyboard, mouse, ...)" if EXPERT
	default y
	help
	  Say Y here if you have any input device (mouse, keyboard, tablet,
	  joystick, steering wheel ...) connected to your system and want
	  it to be available to applications. This includes standard PS/2
	  keyboard and mouse.

	  Say N here if you have a headless (no monitor, no keyboard) system.

	  More information is available: <file:Documentation/input/input.txt>

	  If unsure, say Y.

	  To compile this driver as a module, choose M here: the
	  module will be called input.

if INPUT

config INPUT_LEDS
	tristate "Export input device LEDs in sysfs"
	depends on LEDS_CLASS
	default INPUT
	help
	  Say Y here if you would like to export LEDs on input devices
	  as standard LED class devices in sysfs.

	  If unsure, say Y.

	  To compile this driver as a module, choose M here: the
	  module will be called input-leds.

config INPUT_FF_MEMLESS
	tristate "Support for memoryless force-feedback devices"
	help
	  Say Y here if you have memoryless force-feedback input device
	  such as Logitech WingMan Force 3D, ThrustMaster FireStorm Dual
	  Power 2, or similar. You will also need to enable hardware-specific
	  driver.

	  If unsure, say N.

	  To compile this driver as a module, choose M here: the
	  module will be called ff-memless.

config INPUT_POLLDEV
	tristate "Polled input device skeleton"
	help
	  Say Y here if you are using a driver for an input
	  device that periodically polls hardware state. This
	  option is only useful for out-of-tree drivers since
	  in-tree drivers select it automatically.

	  If unsure, say N.

	  To compile this driver as a module, choose M here: the
	  module will be called input-polldev.

config INPUT_SPARSEKMAP
	tristate "Sparse keymap support library"
	help
	  Say Y here if you are using a driver for an input
	  device that uses sparse keymap. This option is only
	  useful for out-of-tree drivers since in-tree drivers
	  select it automatically.

	  If unsure, say N.

	  To compile this driver as a module, choose M here: the
	  module will be called sparse-keymap.

config INPUT_MATRIXKMAP
	tristate "Matrix keymap support library"
	help
	  Say Y here if you are using a driver for an input
	  device that uses matrix keymap. This option is only
	  useful for out-of-tree drivers since in-tree drivers
	  select it automatically.

	  If unsure, say N.

	  To compile this driver as a module, choose M here: the
	  module will be called matrix-keymap.

comment "Userland interfaces"

config INPUT_MOUSEDEV
	tristate "Mouse interface"
	default y
	help
	  Say Y here if you want your mouse to be accessible as char devices
	  13:32+ - /dev/input/mouseX and 13:63 - /dev/input/mice as an
	  emulated IntelliMouse Explorer PS/2 mouse. That way, all user space
	  programs (including SVGAlib, GPM and X) will be able to use your
	  mouse.

	  If unsure, say Y.

	  To compile this driver as a module, choose M here: the
	  module will be called mousedev.

config INPUT_MOUSEDEV_PSAUX
	bool "Provide legacy /dev/psaux device"
	default y
	depends on INPUT_MOUSEDEV
	help
	  Say Y here if you want your mouse also be accessible as char device
	  10:1 - /dev/psaux. The data available through /dev/psaux is exactly
	  the same as the data from /dev/input/mice.

	  If unsure, say Y.


config INPUT_MOUSEDEV_SCREEN_X
	int "Horizontal screen resolution"
	depends on INPUT_MOUSEDEV
	default "1024"
	help
	  If you're using a digitizer, or a graphic tablet, and want to use
	  it as a mouse then the mousedev driver needs to know the X window
	  screen resolution you are using to correctly scale the data. If
	  you're not using a digitizer, this value is ignored.

config INPUT_MOUSEDEV_SCREEN_Y
	int "Vertical screen resolution"
	depends on INPUT_MOUSEDEV
	default "768"
	help
	  If you're using a digitizer, or a graphic tablet, and want to use
	  it as a mouse then the mousedev driver needs to know the X window
	  screen resolution you are using to correctly scale the data. If
	  you're not using a digitizer, this value is ignored.

config INPUT_JOYDEV
	tristate "Joystick interface"
	help
	  Say Y here if you want your joystick or gamepad to be
	  accessible as char device 13:0+ - /dev/input/jsX device.

	  If unsure, say Y.

	  More information is available: <file:Documentation/input/joystick.txt>

	  To compile this driver as a module, choose M here: the
	  module will be called joydev.

config INPUT_EVDEV
	tristate "Event interface"
	help
	  Say Y here if you want your input device events be accessible
	  under char device 13:64+ - /dev/input/eventX in a generic way.

	  To compile this driver as a module, choose M here: the
	  module will be called evdev.

config INPUT_EVBUG
	tristate "Event debugging"
	help
	  Say Y here if you have a problem with the input subsystem and
	  want all events (keypresses, mouse movements), to be output to
	  the system log. While this is useful for debugging, it's also
	  a security threat - your keypresses include your passwords, of
	  course.

	  If unsure, say N.

	  To compile this driver as a module, choose M here: the
	  module will be called evbug.

config INPUT_APMPOWER
	tristate "Input Power Event -> APM Bridge" if EXPERT
	depends on INPUT && APM_EMULATION
	help
	  Say Y here if you want suspend key events to trigger a user
	  requested suspend through APM. This is useful on embedded
	  systems where such behaviour is desired without userspace
	  interaction. If unsure, say N.

	  To compile this driver as a module, choose M here: the
	  module will be called apm-power.

config INPUT_KEYRESET
	bool "Reset key"
	depends on INPUT
	select INPUT_KEYCOMBO
	---help---
	  Say Y here if you want to reboot when some keys are pressed;

config GLOVE_TOUCH
	tristate "Glove touch"
	help
	  Say Y here if you want glove touch.

config INPUT_TOUCHSCREEN_TCLM
	bool "touchscreen tclm"
	depends on I2C
	help
	  Say Y here if you need to use tclm.

	  If unsure, say N.

config INPUT_TOUCHSCREEN_TCLMV2
	bool "touchscreen tclmv2"
	depends on I2C
	help
	  Say Y here if you need to use tclm.

	  If unsure, say N.

config INPUT_KEYCOMBO
	bool "Key combo"
	depends on INPUT
	---help---
	  Say Y here if you want to take action when some keys are pressed;

config SEC_AUTO_INPUT
	tristate "Samsung auto input"
	depends on !SAMSUNG_PRODUCT_SHIP
	default y
	help
	  Say Y here if you use auto input function.

	  If unsure, say N.

	  To compile this driver as a module, choose M here: the
	  module will be called autoinput.

comment "Input Device Drivers"

source "drivers/input/keyboard/Kconfig"

source "drivers/input/mouse/Kconfig"

source "drivers/input/joystick/Kconfig"

source "drivers/input/tablet/Kconfig"

source "drivers/input/touchscreen/Kconfig"

source "drivers/input/wacom/Kconfig"

source "drivers/input/misc/Kconfig"

endif

menu "Hardware I/O ports"

source "drivers/input/serio/Kconfig"

source "drivers/input/gameport/Kconfig"

endmenu

endmenu

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   #
# Makefile for the input core drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_INPUT)		+= input-core.o
input-core-y := input.o input-compat.o input-mt.o ff-core.o

obj-$(CONFIG_INPUT_TOUCHSCREEN)	+= sec_cmd.o
obj-$(CONFIG_INPUT_TOUCHSCREEN_TCLM)	+= sec_tclm.o
obj-$(CONFIG_INPUT_TOUCHSCREEN_TCLMV2)	+= sec_tclm_v2.o

obj-$(CONFIG_INPUT_FF_MEMLESS)	+= ff-memless.o
obj-$(CONFIG_INPUT_POLLDEV)	+= input-polldev.o
obj-$(CONFIG_INPUT_SPARSEKMAP)	+= sparse-keymap.o
obj-$(CONFIG_INPUT_MATRIXKMAP)	+= matrix-keymap.o

obj-$(CONFIG_INPUT_LEDS)	+= input-leds.o
obj-$(CONFIG_INPUT_MOUSEDEV)	+= mousedev.o
obj-$(CONFIG_INPUT_JOYDEV)	+= joydev.o
obj-$(CONFIG_INPUT_EVDEV)	+= evdev.o
obj-$(CONFIG_INPUT_EVBUG)	+= evbug.o

obj-$(CONFIG_INPUT_KEYBOARD)	+= keyboard/
obj-$(CONFIG_INPUT_MOUSE)	+= mouse/
obj-$(CONFIG_INPUT_JOYSTICK)	+= joystick/
obj-$(CONFIG_INPUT_TABLET)	+= tablet/
obj-$(CONFIG_INPUT_TOUCHSCREEN)	+= touchscreen/
obj-$(CONFIG_INPUT_WACOM)	+= wacom/
obj-$(CONFIG_INPUT_MISC)	+= misc/

obj-$(CONFIG_INPUT_APMPOWER)	+= apm-power.o
obj-$(CONFIG_INPUT_KEYRESET)	+= keyreset.o
obj-$(CONFIG_INPUT_KEYCOMBO)	+= keycombo.o

subdir-ccflags-y := -O3
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 *  Input Power Event -> APM Bridge
 *
 *  Copyright (c) 2007 Richard Purdie
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/apm-emulation.h>

static void system_power_event(unsigned int keycode)
{
	switch (keycode) {
	case KEY_SUSPEND:
		apm_queue_event(APM_USER_SUSPEND);
		pr_info("Requesting system suspend...\n");
		break;
	default:
		break;
	}
}

static void apmpower_event(struct input_handle *handle, unsigned int type,
			   unsigned int code, int value)
{
	/* only react on key down events */
	if (value != 1)
		return;

	switch (type) {
	case EV_PWR:
		system_power_event(code);
		break;

	default:
		break;
	}
}

static int apmpower_connect(struct input_handler *handler,
					  struct input_dev *dev,
					  const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "apm-power";

	error = input_register_handle(handle);
	if (error) {
		pr_err("Failed to register input power handler, error %d\n",
		       error);
		kfree(handle);
		return error;
	}

	error = input_open_device(handle);
	if (error) {
		pr_err("Failed to open input power device, error %d\n", error);
		input_unregister_handle(handle);
		kfree(handle);
		return error;
	}

	return 0;
}

static void apmpower_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id apmpower_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_PWR) },
	},
	{ },
};

MODULE_DEVICE_TABLE(input, apmpower_ids);

static struct input_handler apmpower_handler = {
	.event =	apmpower_event,
	.connect =	apmpower_connect,
	.disconnect =	apmpower_disconnect,
	.name =		"apm-power",
	.id_table =	apmpower_ids,
};

static int __init apmpower_init(void)
{
	return input_register_handler(&apmpower_handler);
}

static void __exit apmpower_exit(void)
{
	input_unregister_handler(&apmpower_handler);
}

module_init(apmpower_init);
module_exit(apmpower_exit);

MODULE_AUTHOR("Richard Purdie <rpurdie@rpsys.net>");
MODULE_DESCRIPTION("Input Power Event -> APM Bridge");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 *  Copyright (c) 1999-2001 Vojtech Pavlik
 */

/*
 *  Input driver event debug module - dumps all events into syslog
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/device.h>

MODULE_AUTHOR("Vojtech Pavlik <vojtech@ucw.cz>");
MODULE_DESCRIPTION("Input driver event debug module");
MODULE_LICENSE("GPL");

static void evbug_event(struct input_handle *handle, unsigned int type, unsigned int code, int value)
{
	printk(KERN_DEBUG pr_fmt("Event. Dev: %s, Type: %d, Code: %d, Value: %d\n"),
	       dev_name(&handle->dev->dev), type, code, value);
}

static int evbug_connect(struct input_handler *handler, struct input_dev *dev,
			 const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "evbug";

	error = input_register_handle(handle);
	if (error)
		goto err_free_handle;

	error = input_open_device(handle);
	if (error)
		goto err_unregister_handle;

	printk(KERN_DEBUG pr_fmt("Connected device: %s (%s at %s)\n"),
	       dev_name(&dev->dev),
	       dev->name ?: "unknown",
	       dev->phys ?: "unknown");

	return 0;

 err_unregister_handle:
	input_unregister_handle(handle);
 err_free_handle:
	kfree(handle);
	return error;
}

static void evbug_disconnect(struct input_handle *handle)
{
	printk(KERN_DEBUG pr_fmt("Disconnected device: %s\n"),
	       dev_name(&handle->dev->dev));

	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id evbug_ids[] = {
	{ .driver_info = 1 },	/* Matches all devices */
	{ },			/* Terminating zero entry */
};

MODULE_DEVICE_TABLE(input, evbug_ids);

static struct input_handler evbug_handler = {
	.event =	evbug_event,
	.connect =	evbug_connect,
	.disconnect =	evbug_disconnect,
	.name =		"evbug",
	.id_table =	evbug_ids,
};

static int __init evbug_init(void)
{
	return input_register_handler(&evbug_handler);
}

static void __exit evbug_exit(void)
{
	input_unregister_handler(&evbug_handler);
}

module_init(evbug_init);
module_exit(evbug_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Event char devices, giving access to raw input device events.
 *
 * Copyright (c) 1999-2002 Vojtech Pavlik
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define EVDEV_MINOR_BASE	64
#define EVDEV_MINORS		32
#define EVDEV_MIN_BUFFER_SIZE	64U
#define EVDEV_BUF_PACKETS	8

#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input/mt.h>
#include <linux/input/input_booster.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "input-compat.h"

enum evdev_clock_type {
	EV_CLK_REAL = 0,
	EV_CLK_MONO,
	EV_CLK_BOOT,
	EV_CLK_MAX
};

struct evdev {
	int open;
	struct input_handle handle;
	wait_queue_head_t wait;
	struct evdev_client __rcu *grab;
	struct list_head client_list;
	spinlock_t client_lock; /* protects client_list */
	struct mutex mutex;
	struct device dev;
	struct cdev cdev;
	bool exist;
};

struct evdev_client {
	unsigned int head;
	unsigned int tail;
	unsigned int packet_head; /* [future] position of the first element of next packet */
	spinlock_t buffer_lock; /* protects access to buffer, head and tail */
	struct fasync_struct *fasync;
	struct evdev *evdev;
	struct list_head node;
	unsigned int clk_type;
	bool revoked;
	unsigned long *evmasks[EV_CNT];
	unsigned int bufsize;
	struct input_event buffer[];
};

static size_t evdev_get_mask_cnt(unsigned int type)
{
	static const size_t counts[EV_CNT] = {
		/* EV_SYN==0 is EV_CNT, _not_ SYN_CNT, see EVIOCGBIT */
		[EV_SYN]	= EV_CNT,
		[EV_KEY]	= KEY_CNT,
		[EV_REL]	= REL_CNT,
		[EV_ABS]	= ABS_CNT,
		[EV_MSC]	= MSC_CNT,
		[EV_SW]		= SW_CNT,
		[EV_LED]	= LED_CNT,
		[EV_SND]	= SND_CNT,
		[EV_FF]		= FF_CNT,
	};

	return (type < EV_CNT) ? counts[type] : 0;
}

/* requires the buffer lock to be held */
static bool __evdev_is_filtered(struct evdev_client *client,
				unsigned int type,
				unsigned int code)
{
	unsigned long *mask;
	size_t cnt;

	/* EV_SYN and unknown codes are never filtered */
	if (type == EV_SYN || type >= EV_CNT)
		return false;

	/* first test whether the type is filtered */
	mask = client->evmasks[0];
	if (mask && !test_bit(type, mask))
		return true;

	/* unknown values are never filtered */
	cnt = evdev_get_mask_cnt(type);
	if (!cnt || code >= cnt)
		return false;

	mask = client->evmasks[type];
	return mask && !test_bit(code, mask);
}

/* flush queued events of type @type, caller must hold client->buffer_lock */
static void __evdev_flush_queue(struct evdev_client *client, unsigned int type)
{
	unsigned int i, head, num;
	unsigned int mask = client->bufsize - 1;
	bool is_report;
	struct input_event *ev;

	BUG_ON(type == EV_SYN);

	head = client->tail;
	client->packet_head = client->tail;

	/* init to 1 so a leading SYN_REPORT will not be dropped */
	num = 1;

	for (i = client->tail; i != client->head; i = (i + 1) & mask) {
		ev = &client->buffer[i];
		is_report = ev->type == EV_SYN && ev->code == SYN_REPORT;

		if (ev->type == type) {
			/* drop matched entry */
			continue;
		} else if (is_report && !num) {
			/* drop empty SYN_REPORT groups */
			continue;
		} else if (head != i) {
			/* move entry to fill the gap */
			client->buffer[head].time = ev->time;
			client->buffer[head].type = ev->type;
			client->buffer[head].code = ev->code;
			client->buffer[head].value = ev->value;
		}

		num++;
		head = (head + 1) & mask;

		if (is_report) {
			num = 0;
			client->packet_head = head;
		}
	}

	client->head = head;
}

static void __evdev_queue_syn_dropped(struct evdev_client *client)
{
	struct input_event ev;
	ktime_t time;

	time = client->clk_type == EV_CLK_REAL ?
			ktime_get_real() :
			client->clk_type == EV_CLK_MONO ?
				ktime_get() :
				ktime_get_boottime();

	ev.time = ktime_to_timeval(time);
	ev.type = EV_SYN;
	ev.code = SYN_DROPPED;
	ev.value = 0;

	client->buffer[client->head++] = ev;
	client->head &= client->bufsize - 1;

	if (unlikely(client->head == client->tail)) {
		/* drop queue but keep our SYN_DROPPED event */
		client->tail = (client->head - 1) & (client->bufsize - 1);
		client->packet_head = client->tail;
	}
}

static void evdev_queue_syn_dropped(struct evdev_client *client)
{
	unsigned long flags;

	spin_lock_irqsave(&client->buffer_lock, flags);
	__evdev_queue_syn_dropped(client);
	spin_unlock_irqrestore(&client->buffer_lock, flags);
}

static int evdev_set_clk_type(struct evdev_client *client, unsigned int clkid)
{
	unsigned long flags;
	unsigned int clk_type;

	switch (clkid) {

	case CLOCK_REALTIME:
		clk_type = EV_CLK_REAL;
		break;
	case CLOCK_MONOTONIC:
		clk_type = EV_CLK_MONO;
		break;
	case CLOCK_BOOTTIME:
		clk_type = EV_CLK_BOOT;
		break;
	default:
		return -EINVAL;
	}

	if (client->clk_type != clk_type) {
		client->clk_type = clk_type;

		/*
		 * Flush pending events and queue SYN_DROPPED event,
		 * but only if the queue is not empty.
		 */
		spin_lock_irqsave(&client->buffer_lock, flags);

		if (client->head != client->tail) {
			client->packet_head = client->head = client->tail;
			__evdev_queue_syn_dropped(client);
		}

		spin_unlock_irqrestore(&client->buffer_lock, flags);
	}

	return 0;
}

static void __pass_event(struct evdev_client *client,
			 const struct input_event *event)
{
	client->buffer[client->head++] = *event;
	client->head &= client->bufsize - 1;

	if (unlikely(client->head == client->tail)) {
		/*
		 * This effectively "drops" all unconsumed events, leaving
		 * EV_SYN/SYN_DROPPED plus the newest event in the queue.
		 */
		client->tail = (client->head - 2) & (client->bufsize - 1);

		client->buffer[client->tail].time = event->time;
		client->buffer[client->tail].type = EV_SYN;
		client->buffer[client->tail].code = SYN_DROPPED;
		client->buffer[client->tail].value = 0;

		client->packet_head = client->tail;
	}

	if (event->type == EV_SYN && event->code == SYN_REPORT) {
		client->packet_head = client->head;
		kill_fasync(&client->fasync, SIGIO, POLL_IN);
	}
}

static void evdev_pass_values(struct evdev_client *client,
			const struct input_value *vals, unsigned int count,
			ktime_t *ev_time)
{
	struct evdev *evdev = client->evdev;
	const struct input_value *v;
	struct input_event event;
	bool wakeup = false;

	if (client->revoked)
		return;

	event.time = ktime_to_timeval(ev_time[client->clk_type]);

	/* Interrupts are disabled, just acquire the lock. */
	spin_lock(&client->buffer_lock);

	for (v = vals; v != vals + count; v++) {
		if (__evdev_is_filtered(client, v->type, v->code))
			continue;

		if (v->type == EV_SYN && v->code == SYN_REPORT) {
			/* drop empty SYN_REPORT */
			if (client->packet_head == client->head)
				continue;

			wakeup = true;
		}

		event.type = v->type;
		event.code = v->code;
		event.value = v->value;
		__pass_event(client, &event);
	}

	spin_unlock(&client->buffer_lock);

	if (wakeup)
		wake_up_interruptible(&evdev->wait);
}

/*
 * Pass incoming events to all connected clients.
 */
static void evdev_events(struct input_handle *handle,
			 const struct input_value *vals, unsigned int count)
{
	struct evdev *evdev = handle->private;
	struct evdev_client *client;
	ktime_t ev_time[EV_CLK_MAX];

	ev_time[EV_CLK_MONO] = ktime_get();
	ev_time[EV_CLK_REAL] = ktime_mono_to_real(ev_time[EV_CLK_MONO]);
	ev_time[EV_CLK_BOOT] = ktime_mono_to_any(ev_time[EV_CLK_MONO],
						 TK_OFFS_BOOT);

	rcu_read_lock();

	client = rcu_dereference(evdev->grab);

	if (client)
		evdev_pass_values(client, vals, count, ev_time);
	else
		list_for_each_entry_rcu(client, &evdev->client_list, node)
			evdev_pass_values(client, vals, count, ev_time);

	rcu_read_unlock();
}

/*
 * Pass incoming event to all connected clients.
 */
static void evdev_event(struct input_handle *handle,
			unsigned int type, unsigned int code, int value)
{
	struct input_value vals[] = { { type, code, value } };

	evdev_events(handle, vals, 1);
}

static int evdev_fasync(int fd, struct file *file, int on)
{
	struct evdev_client *client = file->private_data;

	return fasync_helper(fd, file, on, &client->fasync);
}

static void evdev_free(struct device *dev)
{
	struct evdev *evdev = container_of(dev, struct evdev, dev);

	input_put_device(evdev->handle.dev);
	kfree(evdev);
}

/*
 * Grabs an event device (along with underlying input device).
 * This function is called with evdev->mutex taken.
 */
static int evdev_grab(struct evdev *evdev, struct evdev_client *client)
{
	int error;

	if (evdev->grab)
		return -EBUSY;

	error = input_grab_device(&evdev->handle);
	if (error)
		return error;

	rcu_assign_pointer(evdev->grab, client);

	return 0;
}

static int evdev_ungrab(struct evdev *evdev, struct evdev_client *client)
{
	struct evdev_client *grab = rcu_dereference_protected(evdev->grab,
					lockdep_is_held(&evdev->mutex));

	if (grab != client)
		return  -EINVAL;

	rcu_assign_pointer(evdev->grab, NULL);
	synchronize_rcu();
	input_release_device(&evdev->handle);

	return 0;
}

static void evdev_attach_client(struct evdev *evdev,
				struct evdev_client *client)
{
	spin_lock(&evdev->client_lock);
	list_add_tail_rcu(&client->node, &evdev->client_list);
	spin_unlock(&evdev->client_lock);
}

static void evdev_detach_client(struct evdev *evdev,
				struct evdev_client *client)
{
	spin_lock(&evdev->client_lock);
	list_del_rcu(&client->node);
	spin_unlock(&evdev->client_lock);
	synchronize_rcu();
}

static int evdev_open_device(struct evdev *evdev)
{
	int retval;

	retval = mutex_lock_interruptible(&evdev->mutex);
	if (retval)
		return retval;

	if (!evdev->exist)
		retval = -ENODEV;
	else if (!evdev->open++) {
		retval = input_open_device(&evdev->handle);
		if (retval)
			evdev->open--;
	}

	mutex_unlock(&evdev->mutex);
	return retval;
}

static void evdev_close_device(struct evdev *evdev)
{
	mutex_lock(&evdev->mutex);

	if (evdev->exist && !--evdev->open)
		input_close_device(&evdev->handle);

	mutex_unlock(&evdev->mutex);
}

/*
 * Wake up users waiting for IO so they can disconnect from
 * dead device.
 */
static void evdev_hangup(struct evdev *evdev)
{
	struct evdev_client *client;

	spin_lock(&evdev->client_lock);
	list_for_each_entry(client, &evdev->client_list, node)
		kill_fasync(&client->fasync, SIGIO, POLL_HUP);
	spin_unlock(&evdev->client_lock);

	wake_up_interruptible(&evdev->wait);
}

static int evdev_release(struct inode *inode, struct file *file)
{
	struct evdev_client *client = file->private_data;
	struct evdev *evdev = client->evdev;
	unsigned int i;

	mutex_lock(&evdev->mutex);

	if (evdev->exist && !client->revoked)
		input_flush_device(&evdev->handle, file);

	evdev_ungrab(evdev, client);
	mutex_unlock(&evdev->mutex);

	evdev_detach_client(evdev, client);

	for (i = 0; i < EV_CNT; ++i)
		kfree(client->evmasks[i]);

	kvfree(client);

	evdev_close_device(evdev);

	return 0;
}

static unsigned int evdev_compute_buffer_size(struct input_dev *dev)
{
	unsigned int n_events =
		max(dev->hint_events_per_packet * EVDEV_BUF_PACKETS,
		    EVDEV_MIN_BUFFER_SIZE);

	return roundup_pow_of_two(n_events);
}

static int evdev_open(struct inode *inode, struct file *file)
{
	struct evdev *evdev = container_of(inode->i_cdev, struct evdev, cdev);
	unsigned int bufsize = evdev_compute_buffer_size(evdev->handle.dev);
	unsigned int size = sizeof(struct evdev_client) +
					bufsize * sizeof(struct input_event);
	struct evdev_client *client;
	int error;

	client = kzalloc(size, GFP_KERNEL | __GFP_NOWARN);
	if (!client)
		client = vzalloc(size);
	if (!client)
		return -ENOMEM;

	client->bufsize = bufsize;
	spin_lock_init(&client->buffer_lock);
	client->evdev = evdev;
	evdev_attach_client(evdev, client);

	error = evdev_open_device(evdev);
	if (error)
		goto err_free_client;

	file->private_data = client;
	nonseekable_open(inode, file);

	return 0;

 err_free_client:
	evdev_detach_client(evdev, client);
	kvfree(client);
	return error;
}

static ssize_t evdev_write(struct file *file, const char __user *buffer,
			   size_t count, loff_t *ppos)
{
	struct evdev_client *client = file->private_data;
	struct evdev *evdev = client->evdev;
	struct input_event event;
	int retval = 0;

	if (count != 0 && count < input_event_size())
		return -EINVAL;

	retval = mutex_lock_interruptible(&evdev->mutex);
	if (retval)
		return retval;

	if (!evdev->exist || client->revoked) {
		retval = -ENODEV;
		goto out;
	}

	while (retval + input_event_size() <= count) {

		if (input_event_from_user(buffer + retval, &event)) {
			retval = -EFAULT;
			goto out;
		}
		retval += input_event_size();

		input_inject_event(&evdev->handle,
				   event.type, event.code, event.value);
	}

 out:
	mutex_unlock(&evdev->mutex);
	return retval;
}

static int evdev_fetch_next_event(struct evdev_client *client,
				  struct input_event *event)
{
	int have_event;

	spin_lock_irq(&client->buffer_lock);

	have_event = client->packet_head != client->tail;
	if (have_event) {
		*event = client->buffer[client->tail++];
		client->tail &= client->bufsize - 1;
	}

	spin_unlock_irq(&client->buffer_lock);

	return have_event;
}

static ssize_t evdev_read(struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos)
{
	struct evdev_client *client = file->private_data;
	struct evdev *evdev = client->evdev;
	struct input_event event;
	size_t read = 0;
	int error;

	if (count != 0 && count < input_event_size())
		return -EINVAL;

	for (;;) {
		if (!evdev->exist || client->revoked)
			return -ENODEV;

		if (client->packet_head == client->tail &&
		    (file->f_flags & O_NONBLOCK))
			return -EAGAIN;

		/*
		 * count == 0 is special - no IO is done but we check
		 * for error conditions (see above).
		 */
		if (count == 0)
			break;

		while (read + input_event_size() <= count &&
		       evdev_fetch_next_event(client, &event)) {

			if (input_event_to_user(buffer + read, &event))
				return -EFAULT;

			read += input_event_size();
		}

		if (read)
			break;

		if (!(file->f_flags & O_NONBLOCK)) {
			error = wait_event_interruptible(evdev->wait,
					client->packet_head != client->tail ||
					!evdev->exist || client->revoked);
			if (error)
				return error;
		}
	}

	return read;
}

/* No kernel lock - fine */
static unsigned int evdev_poll(struct file *file, poll_table *wait)
{
	struct evdev_client *client = file->private_data;
	struct evdev *evdev = client->evdev;
	unsigned int mask;

	poll_wait(file, &evdev->wait, wait);

	if (evdev->exist && !client->revoked)
		mask = POLLOUT | POLLWRNORM;
	else
		mask = POLLHUP | POLLERR;

	if (client->packet_head != client->tail)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

#ifdef CONFIG_COMPAT

#define BITS_PER_LONG_COMPAT (sizeof(compat_long_t) * 8)
#define BITS_TO_LONGS_COMPAT(x) ((((x) - 1) / BITS_PER_LONG_COMPAT) + 1)

#ifdef __BIG_ENDIAN
static int bits_to_user(unsigned long *bits, unsigned int maxbit,
			unsigned int maxlen, void __user *p, int compat)
{
	int len, i;

	if (compat) {
		len = BITS_TO_LONGS_COMPAT(maxbit) * sizeof(compat_long_t);
		if (len > maxlen)
			len = maxlen;

		for (i = 0; i < len / sizeof(compat_long_t); i++)
			if (copy_to_user((compat_long_t __user *) p + i,
					 (compat_long_t *) bits +
						i + 1 - ((i % 2) << 1),
					 sizeof(compat_long_t)))
				return -EFAULT;
	} else {
		len = BITS_TO_LONGS(maxbit) * sizeof(long);
		if (len > maxlen)
			len = maxlen;

		if (copy_to_user(p, bits, len))
			return -EFAULT;
	}

	return len;
}

static int bits_from_user(unsigned long *bits, unsigned int maxbit,
			  unsigned int maxlen, const void __user *p, int compat)
{
	int len, i;

	if (compat) {
		if (maxlen % sizeof(compat_long_t))
			return -EINVAL;

		len = BITS_TO_LONGS_COMPAT(maxbit) * sizeof(compat_long_t);
		if (len > maxlen)
			len = maxlen;

		for (i = 0; i < len / sizeof(compat_long_t); i++)
			if (copy_from_user((compat_long_t *) bits +
						i + 1 - ((i % 2) << 1),
					   (compat_long_t __user *) p + i,
					   sizeof(compat_long_t)))
				return -EFAULT;
		if (i % 2)
			*((compat_long_t *) bits + i - 1) = 0;

	} else {
		if (maxlen % sizeof(long))
			return -EINVAL;

		len = BITS_TO_LONGS(maxbit) * sizeof(long);
		if (len > maxlen)
			len = maxlen;

		if (copy_from_user(bits, p, len))
			return -EFAULT;
	}

	return len;
}

#else

static int bits_to_user(unsigned long *bits, unsigned int maxbit,
			unsigned int maxlen, void __user *p, int compat)
{
	int len = compat ?
			BITS_TO_LONGS_COMPAT(maxbit) * sizeof(compat_long_t) :
			BITS_TO_LONGS(maxbit) * sizeof(long);

	if (len > maxlen)
		len = maxlen;

	return copy_to_user(p, bits, len) ? -EFAULT : len;
}

static int bits_from_user(unsigned long *bits, unsigned int maxbit,
			  unsigned int maxlen, const void __user *p, int compat)
{
	size_t chunk_size = compat ? sizeof(compat_long_t) : sizeof(long);
	int len;

	if (maxlen % chunk_size)
		return -EINVAL;

	len = compat ? BITS_TO_LONGS_COMPAT(maxbit) : BITS_TO_LONGS(maxbit);
	len *= chunk_size;
	if (len > maxlen)
		len = maxlen;

	return copy_from_user(bits, p, len) ? -EFAULT : len;
}

#endif /* __BIG_ENDIAN */

#else

static int bits_to_user(unsigned long *bits, unsigned int maxbit,
			unsigned int maxlen, void __user *p, int compat)
{
	int len = BITS_TO_LONGS(maxbit) * sizeof(long);

	if (len > maxlen)
		len = maxlen;

	return copy_to_user(p, bits, len) ? -EFAULT : len;
}

static int bits_from_user(unsigned long *bits, unsigned int maxbit,
			  unsigned int maxlen, const void __user *p, int compat)
{
	int len;

	if (maxlen % sizeof(long))
		return -EINVAL;

	len = BITS_TO_LONGS(maxbit) * sizeof(long);
	if (len > maxlen)
		len = maxlen;

	return copy_from_user(bits, p, len) ? -EFAULT : len;
}

#endif /* CONFIG_COMPAT */

static int str_to_user(const char *str, unsigned int maxlen, void __user *p)
{
	int len;

	if (!str)
		return -ENOENT;

	len = strlen(str) + 1;
	if (len > maxlen)
		len = maxlen;

	return copy_to_user(p, str, len) ? -EFAULT : len;
}

static int handle_eviocgbit(struct input_dev *dev,
			    unsigned int type, unsigned int size,
			    void __user *p, int compat_mode)
{
	unsigned long *bits;
	int len;

	switch (type) {

	case      0: bits = dev->evbit;  len = EV_MAX;  break;
	case EV_KEY: bits = dev->keybit; len = KEY_MAX; break;
	case EV_REL: bits = dev->relbit; len = REL_MAX; break;
	case EV_ABS: bits = dev->absbit; len = ABS_MAX; break;
	case EV_MSC: bits = dev->mscbit; len = MSC_MAX; break;
	case EV_LED: bits = dev->ledbit; len = LED_MAX; break;
	case EV_SND: bits = dev->sndbit; len = SND_MAX; break;
	case EV_FF:  bits = dev->ffbit;  len = FF_MAX;  break;
	case EV_SW:  bits = dev->swbit;  len = SW_MAX;  break;
	default: return -EINVAL;
	}

	return bits_to_user(bits, len, size, p, compat_mode);
}

static int evdev_handle_get_keycode(struct input_dev *dev, void __user *p)
{
	struct input_keymap_entry ke = {
		.len	= sizeof(unsigned int),
		.flags	= 0,
	};
	int __user *ip = (int __user *)p;
	int error;

	/* legacy case */
	if (copy_from_user(ke.scancode, p, sizeof(unsigned int)))
		return -EFAULT;

	error = input_get_keycode(dev, &ke);
	if (error)
		return error;

	if (put_user(ke.keycode, ip + 1))
		return -EFAULT;

	return 0;
}

static int evdev_handle_get_keycode_v2(struct input_dev *dev, void __user *p)
{
	struct input_keymap_entry ke;
	int error;

	if (copy_from_user(&ke, p, sizeof(ke)))
		return -EFAULT;

	error = input_get_keycode(dev, &ke);
	if (error)
		return error;

	if (copy_to_user(p, &ke, sizeof(ke)))
		return -EFAULT;

	return 0;
}

static int evdev_handle_set_keycode(struct input_dev *dev, void __user *p)
{
	struct input_keymap_entry ke = {
		.len	= sizeof(unsigned int),
		.flags	= 0,
	};
	int __user *ip = (int __user *)p;

	if (copy_from_user(ke.scancode, p, sizeof(unsigned int)))
		return -EFAULT;

	if (get_user(ke.keycode, ip + 1))
		return -EFAULT;

	return input_set_keycode(dev, &ke);
}

static int evdev_handle_set_keycode_v2(struct input_dev *dev, void __user *p)
{
	struct input_keymap_entry ke;

	if (copy_from_user(&ke, p, sizeof(ke)))
		return -EFAULT;

	if (ke.len > sizeof(ke.scancode))
		return -EINVAL;

	return input_set_keycode(dev, &ke);
}

/*
 * If we transfer state to the user, we should flush all pending events
 * of the same type from the client's queue. Otherwise, they might end up
 * with duplicate events, which can screw up client's state tracking.
 * If bits_to_user fails after flushing the queue, we queue a SYN_DROPPED
 * event so user-space will notice missing events.
 *
 * LOCKING:
 * We need to take event_lock before buffer_lock to avoid dead-locks. But we
 * need the even_lock only to guarantee consistent state. We can safely release
 * it while flushing the queue. This allows input-core to handle filters while
 * we flush the queue.
 */
static int evdev_handle_get_val(struct evdev_client *client,
				struct input_dev *dev, unsigned int type,
				unsigned long *bits, unsigned int maxbit,
				unsigned int maxlen, void __user *p,
				int compat)
{
	int ret;
	unsigned long *mem;
	size_t len;

	len = BITS_TO_LONGS(maxbit) * sizeof(unsigned long);
	mem = kmalloc(len, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	spin_lock_irq(&dev->event_lock);
	spin_lock(&client->buffer_lock);

	memcpy(mem, bits, len);

	spin_unlock(&dev->event_lock);

	__evdev_flush_queue(client, type);

	spin_unlock_irq(&client->buffer_lock);

	ret = bits_to_user(mem, maxbit, maxlen, p, compat);
	if (ret < 0)
		evdev_queue_syn_dropped(client);

	kfree(mem);

	return ret;
}

static int evdev_handle_mt_request(struct input_dev *dev,
				   unsigned int size,
				   int __user *ip)
{
	const struct input_mt *mt = dev->mt;
	unsigned int code;
	int max_slots;
	int i;

	if (get_user(code, &ip[0]))
		return -EFAULT;
	if (!mt || !input_is_mt_value(code))
		return -EINVAL;

	max_slots = (size - sizeof(__u32)) / sizeof(__s32);
	for (i = 0; i < mt->num_slots && i < max_slots; i++) {
		int value = input_mt_get_value(&mt->slots[i], code);
		if (put_user(value, &ip[1 + i]))
			return -EFAULT;
	}

	return 0;
}

static int evdev_revoke(struct evdev *evdev, struct evdev_client *client,
			struct file *file)
{
	client->revoked = true;
	evdev_ungrab(evdev, client);
	input_flush_device(&evdev->handle, file);
	wake_up_interruptible(&evdev->wait);

	return 0;
}

/* must be called with evdev-mutex held */
static int evdev_set_mask(struct evdev_client *client,
			  unsigned int type,
			  const void __user *codes,
			  u32 codes_size,
			  int compat)
{
	unsigned long flags, *mask, *oldmask;
	size_t cnt;
	int error;

	/* we allow unknown types and 'codes_size > size' for forward-compat */
	cnt = evdev_get_mask_cnt(type);
	if (!cnt)
		return 0;

	mask = kcalloc(sizeof(unsigned long), BITS_TO_LONGS(cnt), GFP_KERNEL);
	if (!mask)
		return -ENOMEM;

	error = bits_from_user(mask, cnt - 1, codes_size, codes, compat);
	if (error < 0) {
		kfree(mask);
		return error;
	}

	spin_lock_irqsave(&client->buffer_lock, flags);
	oldmask = client->evmasks[type];
	client->evmasks[type] = mask;
	spin_unlock_irqrestore(&client->buffer_lock, flags);

	kfree(oldmask);

	return 0;
}

/* must be called with evdev-mutex held */
static int evdev_get_mask(struct evdev_client *client,
			  unsigned int type,
			  void __user *codes,
			  u32 codes_size,
			  int compat)
{
	unsigned long *mask;
	size_t cnt, size, xfer_size;
	int i;
	int error;

	/* we allow unknown types and 'codes_size > size' for forward-compat */
	cnt = evdev_get_mask_cnt(type);
	size = sizeof(unsigned long) * BITS_TO_LONGS(cnt);
	xfer_size = min_t(size_t, codes_size, size);

	if (cnt > 0) {
		mask = client->evmasks[type];
		if (mask) {
			error = bits_to_user(mask, cnt - 1,
					     xfer_size, codes, compat);
			if (error < 0)
				return error;
		} else {
			/* fake mask with all bits set */
			for (i = 0; i < xfer_size; i++)
				if (put_user(0xffU, (u8 __user *)codes + i))
					return -EFAULT;
		}
	}

	if (xfer_size < codes_size)
		if (clear_user(codes + xfer_size, codes_size - xfer_size))
			return -EFAULT;

	return 0;
}

static long evdev_do_ioctl(struct file *file, unsigned int cmd,
			   void __user *p, int compat_mode)
{
	struct evdev_client *client = file->private_data;
	struct evdev *evdev = client->evdev;
	struct input_dev *dev = evdev->handle.dev;
	struct input_absinfo abs;
	struct input_mask mask;
	struct ff_effect effect;
	int __user *ip = (int __user *)p;
	unsigned int i, t, u, v;
	unsigned int size;
	int error;

	/* First we check for fixed-length commands */
	switch (cmd) {

	case EVIOCGVERSION:
		return put_user(EV_VERSION, ip);

	case EVIOCGID:
		if (copy_to_user(p, &dev->id, sizeof(struct input_id)))
			return -EFAULT;
		return 0;

	case EVIOCGREP:
		if (!test_bit(EV_REP, dev->evbit))
			return -ENOSYS;
		if (put_user(dev->rep[REP_DELAY], ip))
			return -EFAULT;
		if (put_user(dev->rep[REP_PERIOD], ip + 1))
			return -EFAULT;
		return 0;

	case EVIOCSREP:
		if (!test_bit(EV_REP, dev->evbit))
			return -ENOSYS;
		if (get_user(u, ip))
			return -EFAULT;
		if (get_user(v, ip + 1))
			return -EFAULT;

		input_inject_event(&evdev->handle, EV_REP, REP_DELAY, u);
		input_inject_event(&evdev->handle, EV_REP, REP_PERIOD, v);

		return 0;

	case EVIOCRMFF:
		return input_ff_erase(dev, (int)(unsigned long) p, file);

	case EVIOCGEFFECTS:
		i = test_bit(EV_FF, dev->evbit) ?
				dev->ff->max_effects : 0;
		if (put_user(i, ip))
			return -EFAULT;
		return 0;

	case EVIOCGRAB:
		if (p)
			return evdev_grab(evdev, client);
		else
			return evdev_ungrab(evdev, client);

	case EVIOCREVOKE:
		if (p)
			return -EINVAL;
		else
			return evdev_revoke(evdev, client, file);

	case EVIOCGMASK: {
		void __user *codes_ptr;

		if (copy_from_user(&mask, p, sizeof(mask)))
			return -EFAULT;

		codes_ptr = (void __user *)(unsigned long)mask.codes_ptr;
		return evdev_get_mask(client,
				      mask.type, codes_ptr, mask.codes_size,
				      compat_mode);
	}

	case EVIOCSMASK: {
		const void __user *codes_ptr;

		if (copy_from_user(&mask, p, sizeof(mask)))
			return -EFAULT;

		codes_ptr = (const void __user *)(unsigned long)mask.codes_ptr;
		return evdev_set_mask(client,
				      mask.type, codes_ptr, mask.codes_size,
				      compat_mode);
	}

	case EVIOCSCLOCKID:
		if (copy_from_user(&i, p, sizeof(unsigned int)))
			return -EFAULT;

		return evdev_set_clk_type(client, i);

	case EVIOCGKEYCODE:
		return evdev_handle_get_keycode(dev, p);

	case EVIOCSKEYCODE:
		return evdev_handle_set_keycode(dev, p);

	case EVIOCGKEYCODE_V2:
		return evdev_handle_get_keycode_v2(dev, p);

	case EVIOCSKEYCODE_V2:
		return evdev_handle_set_keycode_v2(dev, p);
	}

	size = _IOC_SIZE(cmd);

	/* Now check variable-length commands */
#define EVIOC_MASK_SIZE(nr)	((nr) & ~(_IOC_SIZEMASK << _IOC_SIZESHIFT))
	switch (EVIOC_MASK_SIZE(cmd)) {

	case EVIOCGPROP(0):
		return bits_to_user(dev->propbit, INPUT_PROP_MAX,
				    size, p, compat_mode);

	case EVIOCGMTSLOTS(0):
		return evdev_handle_mt_request(dev, size, ip);

	case EVIOCGKEY(0):
		return evdev_handle_get_val(client, dev, EV_KEY, dev->key,
					    KEY_MAX, size, p, compat_mode);

	case EVIOCGLED(0):
		return evdev_handle_get_val(client, dev, EV_LED, dev->led,
					    LED_MAX, size, p, compat_mode);

	case EVIOCGSND(0):
		return evdev_handle_get_val(client, dev, EV_SND, dev->snd,
					    SND_MAX, size, p, compat_mode);

	case EVIOCGSW(0):
		return evdev_handle_get_val(client, dev, EV_SW, dev->sw,
					    SW_MAX, size, p, compat_mode);

	case EVIOCGNAME(0):
		return str_to_user(dev->name, size, p);

	case EVIOCGPHYS(0):
		return str_to_user(dev->phys, size, p);

	case EVIOCGUNIQ(0):
		return str_to_user(dev->uniq, size, p);

	case EVIOC_MASK_SIZE(EVIOCSFF):
		if (input_ff_effect_from_user(p, size, &effect))
			return -EFAULT;

		error = input_ff_upload(dev, &effect, file);
		if (error)
			return error;

		if (put_user(effect.id, &(((struct ff_effect __user *)p)->id)))
			return -EFAULT;

		return 0;
	}

	/* Multi-number variable-length handlers */
	if (_IOC_TYPE(cmd) != 'E')
		return -EINVAL;

	if (_IOC_DIR(cmd) == _IOC_READ) {

		if ((_IOC_NR(cmd) & ~EV_MAX) == _IOC_NR(EVIOCGBIT(0, 0)))
			return handle_eviocgbit(dev,
						_IOC_NR(cmd) & EV_MAX, size,
						p, compat_mode);

		if ((_IOC_NR(cmd) & ~ABS_MAX) == _IOC_NR(EVIOCGABS(0))) {

			if (!dev->absinfo)
				return -EINVAL;

			t = _IOC_NR(cmd) & ABS_MAX;
			abs = dev->absinfo[t];

			if (copy_to_user(p, &abs, min_t(size_t,
					size, sizeof(struct input_absinfo))))
				return -EFAULT;

			return 0;
		}
	}

	if (_IOC_DIR(cmd) == _IOC_WRITE) {

		if ((_IOC_NR(cmd) & ~ABS_MAX) == _IOC_NR(EVIOCSABS(0))) {

			if (!dev->absinfo)
				return -EINVAL;

			t = _IOC_NR(cmd) & ABS_MAX;

			if (copy_from_user(&abs, p, min_t(size_t,
					size, sizeof(struct input_absinfo))))
				return -EFAULT;

			if (size < sizeof(struct input_absinfo))
				abs.resolution = 0;

			/* We can't change number of reserved MT slots */
			if (t == ABS_MT_SLOT)
				return -EINVAL;

			/*
			 * Take event lock to ensure that we are not
			 * changing device parameters in the middle
			 * of event.
			 */
			spin_lock_irq(&dev->event_lock);
			dev->absinfo[t] = abs;
			spin_unlock_irq(&dev->event_lock);

			return 0;
		}
	}

	return -EINVAL;
}

static long evdev_ioctl_handler(struct file *file, unsigned int cmd,
				void __user *p, int compat_mode)
{
	struct evdev_client *client = file->private_data;
	struct evdev *evdev = client->evdev;
	int retval;

	retval = mutex_lock_interruptible(&evdev->mutex);
	if (retval)
		return retval;

	if (!evdev->exist || client->revoked) {
		retval = -ENODEV;
		goto out;
	}

	retval = evdev_do_ioctl(file, cmd, p, compat_mode);

 out:
	mutex_unlock(&evdev->mutex);
	return retval;
}

static long evdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return evdev_ioctl_handler(file, cmd, (void __user *)arg, 0);
}

#ifdef CONFIG_COMPAT
static long evdev_ioctl_compat(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	return evdev_ioctl_handler(file, cmd, compat_ptr(arg), 1);
}
#endif

static const struct file_operations evdev_fops = {
	.owner		= THIS_MODULE,
	.read		= evdev_read,
	.write		= evdev_write,
	.poll		= evdev_poll,
	.open		= evdev_open,
	.release	= evdev_release,
	.unlocked_ioctl	= evdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= evdev_ioctl_compat,
#endif
	.fasync		= evdev_fasync,
	.llseek		= no_llseek,
};

/*
 * Mark device non-existent. This disables writes, ioctls and
 * prevents new users from opening the device. Already posted
 * blocking reads will stay, however new ones will fail.
 */
static void evdev_mark_dead(struct evdev *evdev)
{
	mutex_lock(&evdev->mutex);
	evdev->exist = false;
	mutex_unlock(&evdev->mutex);
}

static void evdev_cleanup(struct evdev *evdev)
{
	struct input_handle *handle = &evdev->handle;

	evdev_mark_dead(evdev);
	evdev_hangup(evdev);

	cdev_del(&evdev->cdev);

	/* evdev is marked dead so no one else accesses evdev->open */
	if (evdev->open) {
		input_flush_device(handle, NULL);
		input_close_device(handle);
	}
}

/*
 * Create new evdev device. Note that input core serializes calls
 * to connect and disconnect.
 */
static int evdev_connect(struct input_handler *handler, struct input_dev *dev,
			 const struct input_device_id *id)
{
	struct evdev *evdev;
	int minor;
	int dev_no;
	int error;

	minor = input_get_new_minor(EVDEV_MINOR_BASE, EVDEV_MINORS, true);
	if (minor < 0) {
		error = minor;
		pr_err("failed to reserve new minor: %d\n", error);
		return error;
	}

	evdev = kzalloc(sizeof(struct evdev), GFP_KERNEL);
	if (!evdev) {
		error = -ENOMEM;
		goto err_free_minor;
	}

	INIT_LIST_HEAD(&evdev->client_list);
	spin_lock_init(&evdev->client_lock);
	mutex_init(&evdev->mutex);
	init_waitqueue_head(&evdev->wait);
	evdev->exist = true;

	dev_no = minor;
	/* Normalize device number if it falls into legacy range */
	if (dev_no < EVDEV_MINOR_BASE + EVDEV_MINORS)
		dev_no -= EVDEV_MINOR_BASE;
	dev_set_name(&evdev->dev, "event%d", dev_no);

	evdev->handle.dev = input_get_device(dev);
	evdev->handle.name = dev_name(&evdev->dev);
	evdev->handle.handler = handler;
	evdev->handle.private = evdev;

	evdev->dev.devt = MKDEV(INPUT_MAJOR, minor);
	evdev->dev.class = &input_class;
	evdev->dev.parent = &dev->dev;
	evdev->dev.release = evdev_free;
	device_initialize(&evdev->dev);

	error = input_register_handle(&evdev->handle);
	if (error)
		goto err_free_evdev;

	cdev_init(&evdev->cdev, &evdev_fops);
	evdev->cdev.kobj.parent = &evdev->dev.kobj;
	error = cdev_add(&evdev->cdev, evdev->dev.devt, 1);
	if (error)
		goto err_unregister_handle;

	error = device_add(&evdev->dev);
	if (error)
		goto err_cleanup_evdev;

	return 0;

 err_cleanup_evdev:
	evdev_cleanup(evdev);
 err_unregister_handle:
	input_unregister_handle(&evdev->handle);
 err_free_evdev:
	put_device(&evdev->dev);
 err_free_minor:
	input_free_minor(minor);
	return error;
}

static void evdev_disconnect(struct input_handle *handle)
{
	struct evdev *evdev = handle->private;

	device_del(&evdev->dev);
	evdev_cleanup(evdev);
	input_free_minor(MINOR(evdev->dev.devt));
	input_unregister_handle(handle);
	put_device(&evdev->dev);
}

static const struct input_device_id evdev_ids[] = {
	{ .driver_info = 1 },	/* Matches all devices */
	{ },			/* Terminating zero entry */
};

MODULE_DEVICE_TABLE(input, evdev_ids);

static struct input_handler evdev_handler = {
	.event		= evdev_event,
	.events		= evdev_events,
	.connect	= evdev_connect,
	.disconnect	= evdev_disconnect,
	.legacy_minors	= true,
	.minor		= EVDEV_MINOR_BASE,
	.name		= "evdev",
	.id_table	= evdev_ids,
};

static int __init evdev_init(void)
{
#ifdef CONFIG_SEC_INPUT_BOOSTER
	input_booster_init();
#endif
	return input_register_handler(&evdev_handler);
}

static void __exit evdev_exit(void)
{
	input_unregister_handler(&evdev_handler);
}

module_init(evdev_init);
module_exit(evdev_exit);

MODULE_AUTHOR("Vojtech Pavlik <vojtech@ucw.cz>");
MODULE_DESCRIPTION("Input driver event char devices");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 *  Force feedback support for Linux input subsystem
 *
 *  Copyright (c) 2006 Anssi Hannula <anssi.hannula@gmail.com>
 *  Copyright (c) 2006 Dmitry Torokhov <dtor@mail.ru>
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* #define DEBUG */

#include <linux/input.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/slab.h>

/*
 * Check that the effect_id is a valid effect and whether the user
 * is the owner
 */
static int check_effect_access(struct ff_device *ff, int effect_id,
				struct file *file)
{
	if (effect_id < 0 || effect_id >= ff->max_effects ||
	    !ff->effect_owners[effect_id])
		return -EINVAL;

	if (file && ff->effect_owners[effect_id] != file)
		return -EACCES;

	return 0;
}

/*
 * Checks whether 2 effects can be combined together
 */
static inline int check_effects_compatible(struct ff_effect *e1,
					   struct ff_effect *e2)
{
	return e1->type == e2->type &&
	       (e1->type != FF_PERIODIC ||
		e1->u.periodic.waveform == e2->u.periodic.waveform);
}

/*
 * Convert an effect into compatible one
 */
static int compat_effect(struct ff_device *ff, struct ff_effect *effect)
{
	int magnitude;

	switch (effect->type) {
	case FF_RUMBLE:
		if (!test_bit(FF_PERIODIC, ff->ffbit))
			return -EINVAL;

		/*
		 * calculate magnitude of sine wave as average of rumble's
		 * 2/3 of strong magnitude and 1/3 of weak magnitude
		 */
		magnitude = effect->u.rumble.strong_magnitude / 3 +
			    effect->u.rumble.weak_magnitude / 6;

		effect->type = FF_PERIODIC;
		effect->u.periodic.waveform = FF_SINE;
		effect->u.periodic.period = 50;
		effect->u.periodic.magnitude = max(magnitude, 0x7fff);
		effect->u.periodic.offset = 0;
		effect->u.periodic.phase = 0;
		effect->u.periodic.envelope.attack_length = 0;
		effect->u.periodic.envelope.attack_level = 0;
		effect->u.periodic.envelope.fade_length = 0;
		effect->u.periodic.envelope.fade_level = 0;

		return 0;

	default:
		/* Let driver handle conversion */
		return 0;
	}
}

/**
 * input_ff_upload() - upload effect into force-feedback device
 * @dev: input device
 * @effect: effect to be uploaded
 * @file: owner of the effect
 */
int input_ff_upload(struct input_dev *dev, struct ff_effect *effect,
		    struct file *file)
{
	struct ff_device *ff = dev->ff;
	struct ff_effect *old;
	int ret = 0;
	int id;

	if (!test_bit(EV_FF, dev->evbit))
		return -ENOSYS;

	if (effect->type < FF_EFFECT_MIN || effect->type > FF_EFFECT_MAX ||
	    !test_bit(effect->type, dev->ffbit)) {
		dev_dbg(&dev->dev, "invalid or not supported effect type in upload\n");
		return -EINVAL;
	}

	if (effect->type == FF_PERIODIC &&
	    (effect->u.periodic.waveform < FF_WAVEFORM_MIN ||
	     effect->u.periodic.waveform > FF_WAVEFORM_MAX ||
	     !test_bit(effect->u.periodic.waveform, dev->ffbit))) {
		dev_dbg(&dev->dev, "invalid or not supported wave form in upload\n");
		return -EINVAL;
	}

	if (!test_bit(effect->type, ff->ffbit)) {
		ret = compat_effect(ff, effect);
		if (ret)
			return ret;
	}

	mutex_lock(&ff->mutex);

	if (effect->id == -1) {
		for (id = 0; id < ff->max_effects; id++)
			if (!ff->effect_owners[id])
				break;

		if (id >= ff->max_effects) {
			ret = -ENOSPC;
			goto out;
		}

		effect->id = id;
		old = NULL;

	} else {
		id = effect->id;

		ret = check_effect_access(ff, id, file);
		if (ret)
			goto out;

		old = &ff->effects[id];

		if (!check_effects_compatible(effect, old)) {
			ret = -EINVAL;
			goto out;
		}
	}

	ret = ff->upload(dev, effect, old);
	if (ret)
		goto out;

	spin_lock_irq(&dev->event_lock);
	ff->effects[id] = *effect;
	ff->effect_owners[id] = file;
	spin_unlock_irq(&dev->event_lock);

 out:
	mutex_unlock(&ff->mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(input_ff_upload);

/*
 * Erases the effect if the requester is also the effect owner. The mutex
 * should already be locked before calling this function.
 */
static int erase_effect(struct input_dev *dev, int effect_id,
			struct file *file)
{
	struct ff_device *ff = dev->ff;
	int error;

	error = check_effect_access(ff, effect_id, file);
	if (error)
		return error;

	spin_lock_irq(&dev->event_lock);
	ff->playback(dev, effect_id, 0);
	ff->effect_owners[effect_id] = NULL;
	spin_unlock_irq(&dev->event_lock);

	if (ff->erase) {
		error = ff->erase(dev, effect_id);
		if (error) {
			spin_lock_irq(&dev->event_lock);
			ff->effect_owners[effect_id] = file;
			spin_unlock_irq(&dev->event_lock);

			return error;
		}
	}

	return 0;
}

/**
 * input_ff_erase - erase a force-feedback effect from device
 * @dev: input device to erase effect from
 * @effect_id: id of the effect to be erased
 * @file: purported owner of the request
 *
 * This function erases a force-feedback effect from specified device.
 * The effect will only be erased if it was uploaded through the same
 * file handle that is requesting erase.
 */
int input_ff_erase(struct input_dev *dev, int effect_id, struct file *file)
{
	struct ff_device *ff = dev->ff;
	int ret;

	if (!test_bit(EV_FF, dev->evbit))
		return -ENOSYS;

	mutex_lock(&ff->mutex);
	ret = erase_effect(dev, effect_id, file);
	mutex_unlock(&ff->mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(input_ff_erase);

/*
 * input_ff_flush - erase all effects owned by a file handle
 * @dev: input device to erase effect from
 * @file: purported owner of the effects
 *
 * This function erases all force-feedback effects associated with
 * the given owner from specified device. Note that @file may be %NULL,
 * in which case all effects will be erased.
 */
int input_ff_flush(struct input_dev *dev, struct file *file)
{
	struct ff_device *ff = dev->ff;
	int i;

	dev_dbg(&dev->dev, "flushing now\n");

	mutex_lock(&ff->mutex);

	for (i = 0; i < ff->max_effects; i++)
		erase_effect(dev, i, file);

	mutex_unlock(&ff->mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(input_ff_flush);

/**
 * input_ff_event() - generic handler for force-feedback events
 * @dev: input device to send the effect to
 * @type: event type (anything but EV_FF is ignored)
 * @code: event code
 * @value: event value
 */
int input_ff_event(struct input_dev *dev, unsigned int type,
		   unsigned int code, int value)
{
	struct ff_device *ff = dev->ff;

	if (type != EV_FF)
		return 0;

	switch (code) {
	case FF_GAIN:
		if (!test_bit(FF_GAIN, dev->ffbit) || value > 0xffffU)
			break;

		ff->set_gain(dev, value);
		break;

	case FF_AUTOCENTER:
		if (!test_bit(FF_AUTOCENTER, dev->ffbit) || value > 0xffffU)
			break;

		ff->set_autocenter(dev, value);
		break;

	default:
		if (check_effect_access(ff, code, NULL) == 0)
			ff->playback(dev, code, value);
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(input_ff_event);

/**
 * input_ff_create() - create force-feedback device
 * @dev: input device supporting force-feedback
 * @max_effects: maximum number of effects supported by the device
 *
 * This function allocates all necessary memory for a force feedback
 * portion of an input device and installs all default handlers.
 * @dev->ffbit should be already set up before calling this function.
 * Once ff device is created you need to setup its upload, erase,
 * playback and other handlers before registering input device
 */
int input_ff_create(struct input_dev *dev, unsigned int max_effects)
{
	struct ff_device *ff;
	size_t ff_dev_size;
	int i;

	if (!max_effects) {
		dev_err(&dev->dev, "cannot allocate device without any effects\n");
		return -EINVAL;
	}

	if (max_effects > FF_MAX_EFFECTS) {
		dev_err(&dev->dev, "cannot allocate more than FF_MAX_EFFECTS effects\n");
		return -EINVAL;
	}

	ff_dev_size = sizeof(struct ff_device) +
				max_effects * sizeof(struct file *);
	if (ff_dev_size < max_effects) /* overflow */
		return -EINVAL;

	ff = kzalloc(ff_dev_size, GFP_KERNEL);
	if (!ff)
		return -ENOMEM;

	ff->effects = kcalloc(max_effects, sizeof(struct ff_effect),
			      GFP_KERNEL);
	if (!ff->effects) {
		kfree(ff);
		return -ENOMEM;
	}

	ff->max_effects = max_effects;
	mutex_init(&ff->mutex);

	dev->ff = ff;
	dev->flush = input_ff_flush;
	dev->event = input_ff_event;
	__set_bit(EV_FF, dev->evbit);

	/* Copy "true" bits into ff device bitmap */
	for_each_set_bit(i, dev->ffbit, FF_CNT)
		__set_bit(i, ff->ffbit);

	/* we can emulate RUMBLE with periodic effects */
	if (test_bit(FF_PERIODIC, ff->ffbit))
		__set_bit(FF_RUMBLE, dev->ffbit);

	return 0;
}
EXPORT_SYMBOL_GPL(input_ff_create);

/**
 * input_ff_destroy() - frees force feedback portion of input device
 * @dev: input device supporting force feedback
 *
 * This function is only needed in error path as input core will
 * automatically free force feedback structures when device is
 * destroyed.
 */
void input_ff_destroy(struct input_dev *dev)
{
	struct ff_device *ff = dev->ff;

	__clear_bit(EV_FF, dev->evbit);
	if (ff) {
		if (ff->destroy)
			ff->destroy(ff);
		kfree(ff->private);
		kfree(ff->effects);
		kfree(ff);
		dev->ff = NULL;
	}
}
EXPORT_SYMBOL_GPL(input_ff_destroy);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 *  Force feedback support for memoryless devices
 *
 *  Copyright (c) 2006 Anssi Hannula <anssi.hannula@gmail.com>
 *  Copyright (c) 2006 Dmitry Torokhov <dtor@mail.ru>
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* #define DEBUG */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/slab.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/fixp-arith.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anssi Hannula <anssi.hannula@gmail.com>");
MODULE_DESCRIPTION("Force feedback support for memoryless devices");

/* Number of effects handled with memoryless devices */
#define FF_MEMLESS_EFFECTS	16

/* Envelope update interval in ms */
#define FF_ENVELOPE_INTERVAL	50

#define FF_EFFECT_STARTED	0
#define FF_EFFECT_PLAYING	1
#define FF_EFFECT_ABORTING	2

struct ml_effect_state {
	struct ff_effect *effect;
	unsigned long flags;	/* effect state (STARTED, PLAYING, etc) */
	int count;		/* loop count of the effect */
	unsigned long play_at;	/* start time */
	unsigned long stop_at;	/* stop time */
	unsigned long adj_at;	/* last time the effect was sent */
};

struct ml_device {
	void *private;
	struct ml_effect_state states[FF_MEMLESS_EFFECTS];
	int gain;
	struct timer_list timer;
	struct input_dev *dev;

	int (*play_effect)(struct input_dev *dev, void *data,
			   struct ff_effect *effect);
};

static const struct ff_envelope *get_envelope(const struct ff_effect *effect)
{
	static const struct ff_envelope empty_envelope;

	switch (effect->type) {
	case FF_PERIODIC:
		return &effect->u.periodic.envelope;

	case FF_CONSTANT:
		return &effect->u.constant.envelope;

	default:
		return &empty_envelope;
	}
}

/*
 * Check for the next time envelope requires an update on memoryless devices
 */
static unsigned long calculate_next_time(struct ml_effect_state *state)
{
	const struct ff_envelope *envelope = get_envelope(state->effect);
	unsigned long attack_stop, fade_start, next_fade;

	if (envelope->attack_length) {
		attack_stop = state->play_at +
			msecs_to_jiffies(envelope->attack_length);
		if (time_before(state->adj_at, attack_stop))
			return state->adj_at +
					msecs_to_jiffies(FF_ENVELOPE_INTERVAL);
	}

	if (state->effect->replay.length) {
		if (envelope->fade_length) {
			/* check when fading should start */
			fade_start = state->stop_at -
					msecs_to_jiffies(envelope->fade_length);

			if (time_before(state->adj_at, fade_start))
				return fade_start;

			/* already fading, advance to next checkpoint */
			next_fade = state->adj_at +
					msecs_to_jiffies(FF_ENVELOPE_INTERVAL);
			if (time_before(next_fade, state->stop_at))
				return next_fade;
		}

		return state->stop_at;
	}

	return state->play_at;
}

static void ml_schedule_timer(struct ml_device *ml)
{
	struct ml_effect_state *state;
	unsigned long now = jiffies;
	unsigned long earliest = 0;
	unsigned long next_at;
	int events = 0;
	int i;

	pr_debug("calculating next timer\n");

	for (i = 0; i < FF_MEMLESS_EFFECTS; i++) {

		state = &ml->states[i];

		if (!test_bit(FF_EFFECT_STARTED, &state->flags))
			continue;

		if (test_bit(FF_EFFECT_PLAYING, &state->flags))
			next_at = calculate_next_time(state);
		else
			next_at = state->play_at;

		if (time_before_eq(now, next_at) &&
		    (++events == 1 || time_before(next_at, earliest)))
			earliest = next_at;
	}

	if (!events) {
		pr_debug("no actions\n");
		del_timer(&ml->timer);
	} else {
		pr_debug("timer set\n");
		mod_timer(&ml->timer, earliest);
	}
}

/*
 * Apply an envelope to a value
 */
static int apply_envelope(struct ml_effect_state *state, int value,
			  struct ff_envelope *envelope)
{
	struct ff_effect *effect = state->effect;
	unsigned long now = jiffies;
	int time_from_level;
	int time_of_envelope;
	int envelope_level;
	int difference;

	if (envelope->attack_length &&
	    time_before(now,
			state->play_at + msecs_to_jiffies(envelope->attack_length))) {
		pr_debug("value = 0x%x, attack_level = 0x%x\n",
			 value, envelope->attack_level);
		time_from_level = jiffies_to_msecs(now - state->play_at);
		time_of_envelope = envelope->attack_length;
		envelope_level = min_t(u16, envelope->attack_level, 0x7fff);

	} else if (envelope->fade_length && effect->replay.length &&
		   time_after(now,
			      state->stop_at - msecs_to_jiffies(envelope->fade_length)) &&
		   time_before(now, state->stop_at)) {
		time_from_level = jiffies_to_msecs(state->stop_at - now);
		time_o