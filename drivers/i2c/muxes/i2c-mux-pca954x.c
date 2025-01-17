;
		spin_unlock_irq(&dev->pending_lock);
	}

	/*
	 * ib_create_cq() will initialize cq->ibcq except for cq->ibcq.cqe.
	 * The number of entries should be >= the number requested or return
	 * an error.
	 */
	cq->dd = dd_from_dev(dev);
	cq->ibcq.cqe = entries;
	cq->notify = IB_CQ_NONE;
	cq->triggered = 0;
	spin_lock_init(&cq->lock);
	kthread_init_work(&cq->comptask, send_complete);
	wc->head = 0;
	wc->tail = 0;
	cq->queue = wc;

	ret = &cq->ibcq;

	goto done;

bail_ip:
	kfree(cq->ip);
bail_wc:
	vfree(wc);
bail_cq:
	kfree(cq);
done:
	return ret;
}

/**
 * qib_destroy_cq - destroy a completion queue
 * @ibcq: the completion queue to destroy.
 *
 * Returns 0 for success.
 *
 * Called by ib_destroy_cq() in the generic verbs code.
 */
int qib_destroy_cq(struct ib_cq *ibcq)
{
	struct qib_ibdev *dev = to_idev(ibcq->device);
	struct qib_cq *cq = to_icq(ibcq);

	kthread_flush_work(&cq->comptask);
	spin_lock(&dev->n_cqs_lock);
	dev->n_cqs_allocated--;
	spin_unlock(&dev->n_cqs_lock);
	if (cq->ip)
		kref_put(&cq->ip->ref, qib_release_mmap_info);
	else
		vfree(cq->queue);
	kfree(cq);

	return 0;
}

/**
 * qib_req_notify_cq - change the notification type for a completion queue
 * @ibcq: the completion queue
 * @notify_flags: the type of notification to request
 *
 * Returns 0 for success.
 *
 * This may be called from interrupt context.  Also called by
 * ib_req_notify_cq() in the generic verbs code.
 */
int qib_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags notify_flags)
{
	struct qib_cq *cq = to_icq(ibcq);
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&cq->lock, flags);
	/*
	 * Don't change IB_CQ_NEXT_COMP to IB_CQ_SOLICITED but allow
	 * any other transitions (see C11-31 and C11-32 in ch. 11.4.2.2).
	 */
	if (cq->notify != IB_CQ_NEXT_COMP)
		cq->notify = notify_flags & IB_CQ_SOLICITED_MASK;

	if ((notify_flags & IB_CQ_REPORT_MISSED_EVENTS) &&
	    cq->queue->head != cq->queue->tail)
		ret = 1;

	spin_unlock_irqrestore(&cq->lock, flags);

	return ret;
}

/**
 * qib_resize_cq - change the size of the CQ
 * @ibcq: the completion queue
 *
 * Returns 0 for success.
 */
int qib_resize_cq(struct ib_cq *ibcq, int cqe, struct ib_udata *udata)
{
	struct qib_cq *cq = to_icq(ibcq);
	struct qib_cq_wc *old_wc;
	struct qib_cq_wc *wc;
	u32 head, tail, n;
	int ret;
	u32 sz;

	if (cqe < 1 || cqe > ib_qib_max_cqes) {
		ret = -EINVAL;
		goto bail;
	}

	/*
	 * Need to use vmalloc() if we want to support large #s of entries.
	 */
	sz = sizeof(*wc);
	if (udata && udata->outlen >= sizeof(__u64))
		sz += sizeof(struct ib_uverbs_wc) * (cqe + 1);
	else
		sz += sizeof(struct ib_wc) * (cqe + 1);
	wc = vmalloc_user(sz);
	if (!wc) {
		ret = -ENOMEM;
		goto bail;
	}

	/* Check that we can write the offset to mmap. */
	if (udata && udata->outlen >= sizeof(__u64)) {
		__u64 offset = 0;

		ret = ib_copy_to_udata(udata, &offset, sizeof(offset));
		if (ret)
			goto bail_free;
	}

	spin_lock_irq(&cq->lock);
	/*
	 * Make sure head and tail are sane since they
	 * might be user writable.
	 */
	old_wc = cq->queue;
	head = old_wc->head;
	if (head > (u32) cq->ibcq.cqe)
		head = (u32) cq->ibcq.cqe;
	tail = old_wc->tail;
	if (tail > (u32) cq->ibcq.cqe)
		tail = (u32) cq->ibcq.cqe;
	if (head < tail)
		n = cq->ibcq.cqe + 1 + head - tail;
	else
		n = head - tail;
	if (unlikely((u32)cqe < n)) {
		ret = -EINVAL;
		goto bail_unlock;
	}
	for (n = 0; tail != head; n++) {
		if (cq->ip)
			wc->uqueue[n] = old_wc->uqueue[tail];
		else
			wc->kqueue[n] = old_wc->kqueue[tail];
		if (tail == (u32) cq->ibcq.cqe)
			tail = 0;
		else
			tail++;
	}
	cq->ibcq.cqe = cqe;
	wc->head = n;
	wc->tail = 0;
	cq->queue = wc;
	spin_unlock_irq(&cq->lock);

	vfree(old_wc);

	if (cq->ip) {
		struct qib_ibdev *dev = to_idev(ibcq->device);
		struct qib_mmap_info *ip = cq->ip;

		qib_update_mmap_info(dev, ip, sz, wc);

		/*
		 * Return the offset to mmap.
		 * See qib_mmap() for details.
		 */
		if (udata && udata->outlen >= sizeof(__u64)) {
			ret = ib_copy_to_udata(udata, &ip->offset,
					       sizeof(ip->offset));
			if (ret)
				goto bail;
		}

		spin_lock_irq(&dev->pending_lock);
		if (list_empty(&ip->pending_mmaps))
			list_add(&ip->pending_mmaps, &dev->pending_mmaps);
		spin_unlock_irq(&dev->pending_lock);
	}

	ret = 0;
	goto bail;

bail_unlock:
	spin_unlock_irq(&cq->lock);
bail_free:
	vfree(wc);
bail:
	return ret;
}

int qib_cq_init(struct qib_devdata *dd)
{
	int ret = 0;
	int cpu;
	struct task_struct *task;

	if (dd->worker)
		return 0;
	dd->worker = kzalloc(sizeof(*dd->worker), GFP_KERNEL);
	if (!dd->worker)
		return -ENOMEM;
	kthread_init_worker(dd->worker);
	task = kthread_create_on_node(
		kthread_worker_fn,
		dd->worker,
		dd->assigned_node_id,
		"qib_cq%d", dd->unit);
	if (IS_ERR(task))
		goto task_fail;
	cpu = cpumask_first(cpumask_of_node(dd->assigned_node_id));
	kthread_bind(task, cpu);
	wake_up_process(task);
out:
	return ret;
task_fail:
	ret = PTR_ERR(task);
	kfree(dd->worker);
	dd->worker = NULL;
	goto out;
}

void qib_cq_exit(struct qib_devdata *dd)
{
	struct kthread_worker *worker;

	worker = dd->worker;
	if (!worker)
		return;
	/* blocks future queuing from send_complete() */
	dd->worker = NULL;
	smp_wmb();
	kthread_flush_worker(worker);
	kthread_stop(worker->task);
	kfree(worker);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          