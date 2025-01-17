= ctxt_fp(fp);
	if (!rcd && cmd.type != QIB_CMD_ASSIGN_CTXT) {
		ret = -EINVAL;
		goto bail;
	}

	switch (cmd.type) {
	case QIB_CMD_ASSIGN_CTXT:
		ret = qib_assign_ctxt(fp, &cmd.cmd.user_info);
		if (ret)
			goto bail;
		break;

	case QIB_CMD_USER_INIT:
		ret = qib_do_user_init(fp, &cmd.cmd.user_info);
		if (ret)
			goto bail;
		ret = qib_get_base_info(fp, (void __user *) (unsigned long)
					cmd.cmd.user_info.spu_base_info,
					cmd.cmd.user_info.spu_base_info_size);
		break;

	case QIB_CMD_RECV_CTRL:
		ret = qib_manage_rcvq(rcd, subctxt_fp(fp), cmd.cmd.recv_ctrl);
		break;

	case QIB_CMD_CTXT_INFO:
		ret = qib_ctxt_info(fp, (struct qib_ctxt_info __user *)
				    (unsigned long) cmd.cmd.ctxt_info);
		break;

	case QIB_CMD_TID_UPDATE:
		ret = qib_tid_update(rcd, fp, &cmd.cmd.tid_info);
		break;

	case QIB_CMD_TID_FREE:
		ret = qib_tid_free(rcd, subctxt_fp(fp), &cmd.cmd.tid_info);
		break;

	case QIB_CMD_SET_PART_KEY:
		ret = qib_set_part_key(rcd, cmd.cmd.part_key);
		break;

	case QIB_CMD_DISARM_BUFS:
		(void)qib_disarm_piobufs_ifneeded(rcd);
		ret = disarm_req_delay(rcd);
		break;

	case QIB_CMD_PIOAVAILUPD:
		qib_force_pio_avail_update(rcd->dd);
		break;

	case QIB_CMD_POLL_TYPE:
		rcd->poll_type = cmd.cmd.poll_type;
		break;

	case QIB_CMD_ARMLAUNCH_CTRL:
		rcd->dd->f_set_armlaunch(rcd->dd, cmd.cmd.armlaunch_ctrl);
		break;

	case QIB_CMD_SDMA_INFLIGHT:
		ret = qib_sdma_get_inflight(user_sdma_queue_fp(fp),
					    (u32 __user *) (unsigned long)
					    cmd.cmd.sdma_inflight);
		break;

	case QIB_CMD_SDMA_COMPLETE:
		ret = qib_sdma_get_complete(rcd->ppd,
					    user_sdma_queue_fp(fp),
					    (u32 __user *) (unsigned long)
					    cmd.cmd.sdma_complete);
		break;

	case QIB_CMD_ACK_EVENT:
		ret = qib_user_event_ack(rcd, subctxt_fp(fp),
					 cmd.cmd.event_mask);
		break;
	}

	if (ret >= 0)
		ret = consumed;

bail:
	return ret;
}

static ssize_t qib_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	struct qib_filedata *fp = iocb->ki_filp->private_data;
	struct qib_ctxtdata *rcd = ctxt_fp(iocb->ki_filp);
	struct qib_user_sdma_queue *pq = fp->pq;

	if (!iter_is_iovec(from) || !from->nr_segs || !pq)
		return -EINVAL;
			 
	return qib_user_sdma_writev(rcd, pq, from->iov, from->nr_segs);
}

static struct class *qib_class;
static dev_t qib_dev;

int qib_cdev_init(int minor, const char *name,
		  const struct file_operations *fops,
		  struct cdev **cdevp, struct device **devp)
{
	const dev_t dev = MKDEV(MAJOR(qib_dev), minor);
	struct cdev *cdev;
	struct device *device = NULL;
	int ret;

	cdev = cdev_alloc();
	if (!cdev) {
		pr_err("Could not allocate cdev for minor %d, %s\n",
		       minor, name);
		ret = -ENOMEM;
		goto done;
	}

	cdev->owner = THIS_MODULE;
	cdev->ops = fops;
	kobject_set_name(&cdev->kobj, name);

	ret = cdev_add(cdev, dev, 1);
	if (ret < 0) {
		pr_err("Could not add cdev for minor %d, %s (err %d)\n",
		       minor, name, -ret);
		goto err_cdev;
	}

	device = device_create(qib_class, NULL, dev, NULL, "%s", name);
	if (!IS_ERR(device))
		goto done;
	ret = PTR_ERR(device);
	device = NULL;
	pr_err("Could not create device for minor %d, %s (err %d)\n",
	       minor, name, -ret);
err_cdev:
	cdev_del(cdev);
	cdev = NULL;
done:
	*cdevp = cdev;
	*devp = device;
	return ret;
}

void qib_cdev_cleanup(struct cdev **cdevp, struct device **devp)
{
	struct device *device = *devp;

	if (device) {
		device_unregister(device);
		*devp = NULL;
	}

	if (*cdevp) {
		cdev_del(*cdevp);
		*cdevp = NULL;
	}
}

static struct cdev *wildcard_cdev;
static struct device *wildcard_device;

int __init qib_dev_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&qib_dev, 0, QIB_NMINORS, QIB_DRV_NAME);
	if (ret < 0) {
		pr_err("Could not allocate chrdev region (err %d)\n", -ret);
		goto done;
	}

	qib_class = class_create(THIS_MODULE, "ipath");
	if (IS_ERR(qib_class)) {
		ret = PTR_ERR(qib_class);
		pr_err("Could not create device class (err %d)\n", -ret);
		unregister_chrdev_region(qib_dev, QIB_NMINORS);
	}

done:
	return ret;
}

void qib_dev_cleanup(void)
{
	if (qib_class) {
		class_destroy(qib_class);
		qib_class = NULL;
	}

	unregister_chrdev_region(qib_dev, QIB_NMINORS);
}

static atomic_t user_count = ATOMIC_INIT(0);

static void qib_user_remove(struct qib_devdata *dd)
{
	if (atomic_dec_return(&user_count) == 0)
		qib_cdev_cleanup(&wildcard_cdev, &wildcard_device);

	qib_cdev_cleanup(&dd->user_cdev, &dd->user_device);
}

static int qib_user_add(struct qib_devdata *dd)
{
	char name[10];
	int ret;

	if (atomic_inc_return(&user_count) == 1) {
		ret = qib_cdev_init(0, "ipath", &qib_file_ops,
				    &wildcard_cdev, &wildcard_device);
		if (ret)
			goto done;
	}

	snprintf(name, sizeof(name), "ipath%d", dd->unit);
	ret = qib_cdev_init(dd->unit + 1, name, &qib_file_ops,
			    &dd->user_cdev, &dd->user_device);
	if (ret)
		qib_user_remove(dd);
done:
	return ret;
}

/*
 * Create per-unit files in /dev
 */
int qib_device_create(struct qib_devdata *dd)
{
	int r, ret;

	r = qib_user_add(dd);
	ret = qib_diag_add(dd);
	if (r && !ret)
		ret = r;
	return ret;
}

/*
 * Remove per-unit files in /dev
 * void, core kernel returns no errors for this stuff
 */
void qib_device_remove(struct qib_devdata *dd)
{
	qib_user_remove(dd);
	qib_diag_remove(dd);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * Copyright (c) 2012 Intel Corporation. All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
