d->dd;
	struct qib_ctxtdata *rcd;
	void *ptmp = NULL;
	int ret;
	int numa_id;

	assign_ctxt_affinity(fp, dd);

	numa_id = qib_numa_aware ? ((fd->rec_cpu_num != -1) ?
		cpu_to_node(fd->rec_cpu_num) :
		numa_node_id()) : dd->assigned_node_id;

	rcd = qib_create_ctxtdata(ppd, ctxt, numa_id);

	/*
	 * Allocate memory for use in qib_tid_update() at open to
	 * reduce cost of expected send setup per message segment
	 */
	if (rcd)
		ptmp = kmalloc(dd->rcvtidcnt * sizeof(u16) +
			       dd->rcvtidcnt * sizeof(struct page **),
			       GFP_KERNEL);

	if (!rcd || !ptmp) {
		qib_dev_err(dd,
			"Unable to allocate ctxtdata memory, failing open\n");
		ret = -ENOMEM;
		goto bailerr;
	}
	rcd->userversion = uinfo->spu_userversion;
	ret = init_subctxts(dd, rcd, uinfo);
	if (ret)
		goto bailerr;
	rcd->tid_pg_list = ptmp;
	rcd->pid = current->pid;
	init_waitqueue_head(&dd->rcd[ctxt]->wait);
	strlcpy(rcd->comm, current->comm, sizeof(rcd->comm));
	ctxt_fp(fp) = rcd;
	qib_stats.sps_ctxts++;
	dd->freectxts--;
	ret = 0;
	goto bail;

bailerr:
	if (fd->rec_cpu_num != -1)
		__clear_bit(fd->rec_cpu_num, qib_cpulist);

	dd->rcd[ctxt] = NULL;
	kfree(rcd);
	kfree(ptmp);
bail:
	return ret;
}

static inline int usable(struct qib_pportdata *ppd)
{
	struct qib_devdata *dd = ppd->dd;

	return dd && (dd->flags & QIB_PRESENT) && dd->kregbase && ppd->lid &&
		(ppd->lflags & QIBL_LINKACTIVE);
}

/*
 * Select a context on the given device, either using a requested port
 * or the port based on the context number.
 */
static int choose_port_ctxt(struct file *fp, struct qib_devdata *dd, u32 port,
			    const struct qib_user_info *uinfo)
{
	struct qib_pportdata *ppd = NULL;
	int ret, ctxt;

	if (port) {
		if (!usable(dd->pport + port - 1)) {
			ret = -ENETDOWN;
			goto done;
		} else
			ppd = dd->pport + port - 1;
	}
	for (ctxt = dd->first_user_ctxt; ctxt < dd->cfgctxts && dd->rcd[ctxt];
	     ctxt++)
		;
	if (ctxt == dd->cfgctxts) {
		ret = -EBUSY;
		goto done;
	}
	if (!ppd) {
		u32 pidx = ctxt % dd->num_pports;

		if (usable(dd->pport + pidx))
			ppd = dd->pport + pidx;
		else {
			for (pidx = 0; pidx < dd->num_pports && !ppd;
			     pidx++)
				if (usable(dd->pport + pidx))
					ppd = dd->pport + pidx;
		}
	}
	ret = ppd ? setup_ctxt(ppd, ctxt, fp, uinfo) : -ENETDOWN;
done:
	return ret;
}

static int find_free_ctxt(int unit, struct file *fp,
			  const struct qib_user_info *uinfo)
{
	struct qib_devdata *dd = qib_lookup(unit);
	int ret;

	if (!dd || (uinfo->spu_port && uinfo->spu_port > dd->num_pports))
		ret = -ENODEV;
	else
		ret = choose_port_ctxt(fp, dd, uinfo->spu_port, uinfo);

	return ret;
}

static int get_a_ctxt(struct file *fp, const struct qib_user_info *uinfo,
		      unsigned alg)
{
	struct qib_devdata *udd = NULL;
	int ret = 0, devmax, npresent, nup, ndev, dusable = 0, i;
	u32 port = uinfo->spu_port, ctxt;

	devmax = qib_count_units(&npresent, &nup);
	if (!npresent) {
		ret = -ENXIO;
		goto done;
	}
	if (nup == 0) {
		ret = -ENETDOWN;
		goto done;
	}

	if (alg == QIB_PORT_ALG_ACROSS) {
		unsigned inuse = ~0U;

		/* find device (with ACTIVE ports) with fewest ctxts in use */
		for (ndev = 0; ndev < devmax; ndev++) {
			struct qib_devdata *dd = qib_lookup(ndev);
			unsigned cused = 0, cfree = 0, pusable = 0;

			if (!dd)
				continue;
			if (port && port <= dd->num_pports &&
			    usable(dd->pport + port - 1))
				pusable = 1;
			else
				for (i = 0; i < dd->num_pports; i++)
					if (usable(dd->pport + i))
						pusable++;
			if (!pusable)
				continue;
			for (ctxt = dd->first_user_ctxt; ctxt < dd->cfgctxts;
			     ctxt++)
				if (dd->rcd[ctxt])
					cused++;
				else
					cfree++;
			if (cfree && cused < inuse) {
				udd = dd;
				inuse = cused;
			}
		}
		if (udd) {
			ret = choose_port_ctxt(fp, udd, port, uinfo);
			goto done;
		}
	} else {
		for (ndev = 0; ndev < devmax; ndev++) {
			struct qib_devdata *dd = qib_lookup(ndev);

			if (dd) {
				ret = choose_port_ctxt(fp, dd, port, uinfo);
				if (!ret)
					goto done;
				if (ret == -EBUSY)
					dusable++;
			}
		}
	}
	ret = dusable ? -EBUSY : -ENETDOWN;

done:
	return ret;
}

static int find_shared_ctxt(struct file *fp,
			    const struct qib_user_info *uinfo)
{
	int devmax, ndev, i;
	int ret = 0;

	devmax = qib_count_units(NULL, NULL);

	for (ndev = 0; ndev < devmax; ndev++) {
		struct qib_devdata *dd = qib_lookup(ndev);

		/* device portion of usable() */
		if (!(dd && (dd->flags & QIB_PRESENT) && dd->kregbase))
			continue;
		for (i = dd->first_user_ctxt; i < dd->cfgctxts; i++) {
			struct qib_ctxtdata *rcd = dd->rcd[i];

			/* Skip ctxts which are not yet open */
			if (!rcd || !rcd->cnt)
				continue;
			/* Skip ctxt if it doesn't match the requested one */
			if (rcd->subctxt_id != uinfo->spu_subctxt_id)
				continue;
			/* Verify the sharing process matches the master */
			if (rcd->subctxt_cnt != uinfo->spu_subctxt_cnt ||
			    rcd->userversion != uinfo->spu_userversion ||
			    rcd->cnt >= rcd->subctxt_cnt) {
				ret = -EINVAL;
				goto done;
			}
			ctxt_fp(fp) = rcd;
			subctxt_fp(fp) = rcd->cnt++;
			rcd->subpid[subctxt_fp(fp)] = current->pid;
			tidcursor_fp(fp) = 0;
			rcd->active_slaves |= 1 << subctxt_fp(fp);
			ret = 1;
			goto done;
		}
	}

done:
	return ret;
}

static int qib_open(struct inode *in, struct file *fp)
{
	/* The real work is performed later in qib_assign_ctxt() */
	fp->private_data = kzalloc(sizeof(struct qib_filedata), GFP_KERNEL);
	if (fp->private_data) /* no cpu affinity by default */
		((struct qib_filedata *)fp->private_data)->rec_cpu_num = -1;
	return fp->private_data ? 0 : -ENOMEM;
}

static int find_hca(unsigned int cpu, int *unit)
{
	int ret = 0, devmax, npresent, nup, ndev;

	*unit = -1;

	devmax = qib_count_units(&npresent, &nup);
	if (!npresent) {
		ret = -ENXIO;
		goto done;
	}
	if (!nup) {
		ret = -ENETDOWN;
		goto done;
	}
	for (ndev = 0; ndev < devmax; ndev++) {
		struct qib_devdata *dd = qib_lookup(ndev);

		if (dd) {
			if (pcibus_to_node(dd->pcidev->bus) < 0) {
				ret = -EINVAL;
				goto done;
			}
			if (cpu_to_node(cpu) ==
				pcibus_to_node(dd->pcidev->bus)) {
				*unit = ndev;
				goto done;
			}
		}
	}
done:
	return ret;
}

static int do_qib_user_sdma_queue_create(struct file *fp)
{
	struct qib_filedata *fd = fp->private_data;
	struct qib_ctxtdata *rcd = fd->rcd;
	struct qib_devdata *dd = rcd->dd;

	if (dd->flags & QIB_HAS_SEND_DMA) {

		fd->pq = qib_user_sdma_queue_create(&dd->pcidev->dev,
						    dd->unit,
						    rcd->ctxt,
						    fd->subctxt);
		if (!fd->pq)
			return -ENOMEM;
	}

	return 0;
}

/*
 * Get ctxt early, so can set affinity prior to memory allocation.
 */
static int qib_assign_ctxt(struct file *fp, const struct qib_user_info *uinfo)
{
	int ret;
	int i_minor;
	unsigned swmajor, swminor, alg = QIB_PORT_ALG_ACROSS;

	/* Check to be sure we haven't already initialized this file */
	if (ctxt_fp(fp)) {
		ret = -EINVAL;
		goto done;
	}

	/* for now, if major version is different, bail */
	swmajor = uinfo->spu_userversion >> 16;
	if (swmajor != QIB_USER_SWMAJOR) {
		ret = -ENODEV;
		goto done;
	}

	swminor = uinfo->spu_userversion & 0xffff;

	if (swminor >= 11 && uinfo->spu_port_alg < QIB_PORT_ALG_COUNT)
		alg = uinfo->spu_port_alg;

	mutex_lock(&qib_mutex);

	if (qib_compatible_subctxts(swmajor, swminor) &&
	    uinfo->spu_subctxt_cnt) {
		ret = find_shared_ctxt(fp, uinfo);
		if (ret > 0) {
			ret = do_qib_user_sdma_queue_create(fp);
			if (!ret)
				assign_ctxt_affinity(fp, (ctxt_fp(fp))->dd);
			goto done_ok;
		}
	}

	i_minor = iminor(file_inode(fp)) - QIB_USER_MINOR_BASE;
	if (i_minor)
		ret = find_free_ctxt(i_minor - 1, fp, uinfo);
	else {
		int unit;
		const unsigned int cpu = cpumask_first(&current->cpus_allowed);
		const unsigned int weight =
			cpumask_weight(&current->cpus_allowed);

		if (weight == 1 && !test_bit(cpu, qib_cpulist))
			if (!find_hca(cpu, &unit) && unit >= 0)
				if (!find_free_ctxt(unit, fp, uinfo)) {
					ret = 0;
					goto done_chk_sdma;
				}
		ret = get_a_ctxt(fp, uinfo, alg);
	}

done_chk_sdma:
	if (!ret)
		ret = do_qib_user_sdma_queue_create(fp);
done_ok:
	mutex_unlock(&qib_mutex);

done:
	return ret;
}


static int qib_do_user_init(struct file *fp,
			    const struct qib_user_info *uinfo)
{
	int ret;
	struct qib_ctxtdata *rcd = ctxt_fp(fp);
	struct qib_devdata *dd;
	unsigned uctxt;

	/* Subctxts don't need to initialize anything since master did it. */
	if (subctxt_fp(fp)) {
		ret = wait_event_interruptible(rcd->wait,
			!test_bit(QIB_CTXT_MASTER_UNINIT, &rcd->flag));
		goto bail;
	}

	dd = rcd->dd;

	/* some ctxts may get extra buffers, calculate that here */
	uctxt = rcd->ctxt - dd->first_user_ctxt;
	if (uctxt < dd->ctxts_extrabuf) {
		rcd->piocnt = dd->pbufsctxt + 1;
		rcd->pio_base = rcd->piocnt * uctxt;
	} else {
		rcd->piocnt = dd->pbufsctxt;
		rcd->pio_base = rcd->piocnt * uctxt +
			dd->ctxts_extrabuf;
	}

	/*
	 * All user buffers are 2KB buffers.  If we ever support
	 * giving 4KB buffers to user processes, this will need some
	 * work.  Can't use piobufbase directly, because it has
	 * both 2K and 4K buffer base values.  So check and handle.
	 */
	if ((rcd->pio_base + rcd->piocnt) > dd->piobcnt2k) {
		if (rcd->pio_base >= dd->piobcnt2k) {
			qib_dev_err(dd,
				    "%u:ctxt%u: no 2KB buffers available\n",
				    dd->unit, rcd->ctxt);
			ret = -ENOBUFS;
			goto bail;
		}
		rcd->piocnt = dd->piobcnt2k - rcd->pio_base;
		qib_dev_err(dd, "Ctxt%u: would use 4KB bufs, using %u\n",
			    rcd->ctxt, rcd->piocnt);
	}

	rcd->piobufs = dd->pio2k_bufbase + rcd->pio_base * dd->palign;
	qib_chg_pioavailkernel(dd, rcd->pio_base, rcd->piocnt,
			       TXCHK_CHG_TYPE_USER, rcd);
	/*
	 * try to ensure that processes start up with consistent avail update
	 * for their own range, at least.   If system very quiet, it might
	 * have the in-memory copy out of date at startup for this range of
	 * buffers, when a context gets re-used.  Do after the chg_pioavail
	 * and before the rest of setup, so it's "almost certain" the dma
	 * will have occurred (can't 100% guarantee, but should be many
	 * decimals of 9s, with this ordering), given how much else happens
	 * after this.
	 */
	dd->f_sendctrl(dd->pport, QIB_SENDCTRL_AVAIL_BLIP);

	/*
	 * Now allocate the rcvhdr Q and eager TIDs; skip the TID
	 * array for time being.  If rcd->ctxt > chip-supported,
	 * we need to do extra stuff here to handle by handling overflow
	 * through ctxt 0, someday
	 */
	ret = qib_create_rcvhdrq(dd, rcd);
	if (!ret)
		ret = qib_setup_eagerbufs(rcd);
	if (ret)
		goto bail_pio;

	rcd->tidcursor = 0; /* start at beginning after open */

	/* initialize poll variables... */
	rcd->urgent = 0;
	rcd->urgent_poll = 0;

	/*
	 * Now enable the ctxt for receive.
	 * For chips that are set to DMA the tail register to memory
	 * when they change (and when the update bit transitions from
	 * 0 to 1.  So for those chips, we turn it off and then back on.
	 * This will (very briefly) affect any other open ctxts, but the
	 * duration is very short, and therefore isn't an issue.  We
	 * explicitly set the in-memory tail copy to 0 beforehand, so we
	 * don't have to wait to be sure the DMA update has happened
	 * (chip resets head/tail to 0 on transition to enable).
	 */
	if (rcd->rcvhdrtail_kvaddr)
		qib_clear_rcvhdrtail(rcd);

	dd->f_rcvctrl(rcd->ppd, QIB_RCVCTRL_CTXT_ENB | QIB_RCVCTRL_TIDFLOW_ENB,
		      rcd->ctxt);

	/* Notify any waiting slaves */
	if (rcd->subctxt_cnt) {
		clear_bit(QIB_CTXT_MASTER_UNINIT, &rcd->flag);
		wake_up(&rcd->wait);
	}
	return 0;

bail_pio:
	qib_chg_pioavailkernel(dd, rcd->pio_base, rcd->piocnt,
			       TXCHK_CHG_TYPE_KERN