;
		qib_release_user_pages(&p, 1);
		cnt++;
	}
}

static int qib_close(struct inode *in, struct file *fp)
{
	int ret = 0;
	struct qib_filedata *fd;
	struct qib_ctxtdata *rcd;
	struct qib_devdata *dd;
	unsigned long flags;
	unsigned ctxt;
	pid_t pid;

	mutex_lock(&qib_mutex);

	fd = fp->private_data;
	fp->private_data = NULL;
	rcd = fd->rcd;
	if (!rcd) {
		mutex_unlock(&qib_mutex);
		goto bail;
	}

	dd = rcd->dd;

	/* ensure all pio buffer writes in progress are flushed */
	qib_flush_wc();

	/* drain user sdma queue */
	if (fd->pq) {
		qib_user_sdma_queue_drain(rcd->ppd, fd->pq);
		qib_user_sdma_queue_destroy(fd->pq);
	}

	if (fd->rec_cpu_num != -1)
		__clear_bit(fd->rec_cpu_num, qib_cpulist);

	if (--rcd->cnt) {
		/*
		 * XXX If the master closes the context before the slave(s),
		 * revoke the mmap for the eager receive queue so
		 * the slave(s) don't wait for receive data forever.
		 */
		rcd->active_slaves &= ~(1 << fd->subctxt);
		rcd->subpid[fd->subctxt] = 0;
		mutex_unlock(&qib_mutex);
		goto bail;
	}

	/* early; no interrupt users after this */
	spin_lock_irqsave(&dd->uctxt_lock, flags);
	ctxt = rcd->ctxt;
	dd->rcd[ctxt] = NULL;
	pid = rcd->pid;
	rcd->pid = 0;
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);

	if (rcd->rcvwait_to || rcd->piowait_to ||
	    rcd->rcvnowait || rcd->pionowait) {
		rcd->rcvwait_to = 0;
		rcd->piowait_to = 0;
		rcd->rcvnowait = 0;
		rcd->pionowait = 0;
	}
	if (rcd->flag)
		rcd->flag = 0;

	if (dd->kregbase) {
		/* atomically clear receive enable ctxt and intr avail. */
		dd->f_rcvctrl(rcd->ppd, QIB_RCVCTRL_CTXT_DIS |
				  QIB_RCVCTRL_INTRAVAIL_DIS, ctxt);

		/* clean up the pkeys for this ctxt user */
		qib_clean_part_key(rcd, dd);
		qib_disarm_piobufs(dd, rcd->pio_base, rcd->piocnt);
		qib_chg_pioavailkernel(dd, rcd->pio_base,
				       rcd->piocnt, TXCHK_CHG_TYPE_KERN, NULL);

		dd->f_clear_tids(dd, rcd);

		if (dd->pageshadow)
			unlock_expected_tids(rcd);
		qib_stats.sps_ctxts--;
		dd->freectxts++;
	}

	mutex_unlock(&qib_mutex);
	qib_free_ctxtdata(dd, rcd); /* after releasing the mutex */

bail:
	kfree(fd);
	return ret;
}

static int qib_ctxt_info(struct file *fp, struct qib_ctxt_info __user *uinfo)
{
	struct qib_ctxt_info info;
	int ret;
	size_t sz;
	struct qib_ctxtdata *rcd = ctxt_fp(fp);
	struct qib_filedata *fd;

	fd = fp->private_data;

	info.num_active = qib_count_active_units();
	info.unit = rcd->dd->unit;
	info.port = rcd->ppd->port;
	info.ctxt = rcd->ctxt;
	info.subctxt =  subctxt_fp(fp);
	/* Number of user ctxts available for this device. */
	info.num_ctxts = rcd->dd->cfgctxts - rcd->dd->first_user_ctxt;
	info.num_subctxts = rcd->subctxt_cnt;
	info.rec_cpu = fd->rec_cpu_num;
	sz = sizeof(info);

	if (copy_to_user(uinfo, &info, sz)) {
		ret = -EFAULT;
		goto bail;
	}
	ret = 0;

bail:
	return ret;
}

static int qib_sdma_get_inflight(struct qib_user_sdma_queue *pq,
				 u32 __user *inflightp)
{
	const u32 val = qib_user_sdma_inflight_counter(pq);

	if (put_user(val, inflightp))
		return -EFAULT;

	return 0;
}

static int qib_sdma_get_complete(struct qib_pportdata *ppd,
				 struct qib_user_sdma_queue *pq,
				 u32 __user *completep)
{
	u32 val;
	int err;

	if (!pq)
		return -EINVAL;

	err = qib_user_sdma_make_progress(ppd, pq);
	if (err < 0)
		return err;

	val = qib_user_sdma_complete_counter(pq);
	if (put_user(val, completep))
		return -EFAULT;

	return 0;
}

static int disarm_req_delay(struct qib_ctxtdata *rcd)
{
	int ret = 0;

	if (!usable(rcd->ppd)) {
		int i;
		/*
		 * if link is down, or otherwise not usable, delay
		 * the caller up to 30 seconds, so we don't thrash
		 * in trying to get the chip back to ACTIVE, and
		 * set flag so they make the call again.
		 */
		if (rcd->user_event_mask) {
			/*
			 * subctxt_cnt is 0 if not shared, so do base
			 * separately, first, then remaining subctxt, if any
			 */
			set_bit(_QIB_EVENT_DISARM_BUFS_BIT,
				&rcd->user_event_mask[0]);
			for (i = 1; i < rcd->subctxt_cnt; i++)
				set_bit(_QIB_EVENT_DISARM_BUFS_BIT,
					&rcd->user_event_mask[i]);
		}
		for (i = 0; !usable(rcd->ppd) && i < 300; i++)
			msleep(100);
		ret = -ENETDOWN;
	}
	return ret;
}

/*
 * Find all user contexts in use, and set the specified bit in their
 * event mask.
 * See also find_ctxt() for a similar use, that is specific to send buffers.
 */
int qib_set_uevent_bits(struct qib_pportdata *ppd, const int evtbit)
{
	struct qib_ctxtdata *rcd;
	unsigned ctxt;
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&ppd->dd->uctxt_lock, flags);
	for (ctxt = ppd->dd->first_user_ctxt; ctxt < ppd->dd->cfgctxts;
	     ctxt++) {
		rcd = ppd->dd->rcd[ctxt];
		if (!rcd)
			continue;
		if (rcd->user_event_mask) {
			int i;
			/*
			 * subctxt_cnt is 0 if not shared, so do base
			 * separately, first, then remaining subctxt, if any
			 */
			set_bit(evtbit, &rcd->user_event_mask[0]);
			for (i = 1; i < rcd->subctxt_cnt; i++)
				set_bit(evtbit, &rcd->user_event_mask[i]);
		}
		ret = 1;
		break;
	}
	spin_unlock_irqrestore(&ppd->dd->uctxt_lock, flags);

	return ret;
}

/*
 * clear the event notifier events for this context.
 * For the DISARM_BUFS case, we also take action (this obsoletes
 * the older QIB_CMD_DISARM_BUFS, but we keep 