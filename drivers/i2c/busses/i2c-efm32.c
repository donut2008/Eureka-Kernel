RIVER_LOAD_MSG "Intel " QIB_DRV_NAME " loaded: "
#define PFX QIB_DRV_NAME ": "

static const struct pci_device_id qib_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_PATHSCALE, PCI_DEVICE_ID_QLOGIC_IB_6120) },
	{ PCI_DEVICE(PCI_VENDOR_ID_QLOGIC, PCI_DEVICE_ID_QLOGIC_IB_7220) },
	{ PCI_DEVICE(PCI_VENDOR_ID_QLOGIC, PCI_DEVICE_ID_QLOGIC_IB_7322) },
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, qib_pci_tbl);

static struct pci_driver qib_driver = {
	.name = QIB_DRV_NAME,
	.probe = qib_init_one,
	.remove = qib_remove_one,
	.id_table = qib_pci_tbl,
	.err_handler = &qib_pci_err_handler,
};

#ifdef CONFIG_INFINIBAND_QIB_DCA

static int qib_notify_dca(struct notifier_block *, unsigned long, void *);
static struct notifier_block dca_notifier = {
	.notifier_call  = qib_notify_dca,
	.next           = NULL,
	.priority       = 0
};

static int qib_notify_dca_device(struct device *device, void *data)
{
	struct qib_devdata *dd = dev_get_drvdata(device);
	unsigned long event = *(unsigned long *)data;

	return dd->f_notify_dca(dd, event);
}

static int qib_notify_dca(struct notifier_block *nb, unsigned long event,
					  void *p)
{
	int rval;

	rval = driver_for_each_device(&qib_driver.driver, NULL,
				      &event, qib_notify_dca_device);
	return rval ? NOTIFY_BAD : NOTIFY_DONE;
}

#endif

/*
 * Do all the generic driver unit- and chip-independent memory
 * allocation and initialization.
 */
static int __init qib_ib_init(void)
{
	int ret;

	ret = qib_dev_init();
	if (ret)
		goto bail;

	/*
	 * These must be called before the driver is registered with
	 * the PCI subsystem.
	 */
	idr_init(&qib_unit_table);

#ifdef CONFIG_INFINIBAND_QIB_DCA
	dca_register_notify(&dca_notifier);
#endif
#ifdef CONFIG_DEBUG_FS
	qib_dbg_init();
#endif
	ret = pci_register_driver(&qib_driver);
	if (ret < 0) {
		pr_err("Unable to register driver: error %d\n", -ret);
		goto bail_dev;
	}

	/* not fatal if it doesn't work */
	if (qib_init_qibfs())
		pr_err("Unable to register ipathfs\n");
	goto bail; /* all OK */

bail_dev:
#ifdef CONFIG_INFINIBAND_QIB_DCA
	dca_unregister_notify(&dca_notifier);
#endif
#ifdef CONFIG_DEBUG_FS
	qib_dbg_exit();
#endif
	idr_destroy(&qib_unit_table);
	qib_dev_cleanup();
bail:
	return ret;
}

module_init(qib_ib_init);

/*
 * Do the non-unit driver cleanup, memory free, etc. at unload.
 */
static void __exit qib_ib_cleanup(void)
{
	int ret;

	ret = qib_exit_qibfs();
	if (ret)
		pr_err(
			"Unable to cleanup counter filesystem: error %d\n",
			-ret);

#ifdef CONFIG_INFINIBAND_QIB_DCA
	dca_unregister_notify(&dca_notifier);
#endif
	pci_unregister_driver(&qib_driver);
#ifdef CONFIG_DEBUG_FS
	qib_dbg_exit();
#endif

	qib_cpulist_count = 0;
	kfree(qib_cpulist);

	idr_destroy(&qib_unit_table);
	qib_dev_cleanup();
}

module_exit(qib_ib_cleanup);

/* this can only be called after a successful initialization */
static void cleanup_device_data(struct qib_devdata *dd)
{
	int ctxt;
	int pidx;
	struct qib_ctxtdata **tmp;
	unsigned long flags;

	/* users can't do anything more with chip */
	for (pidx = 0; pidx < dd->num_pports; ++pidx) {
		if (dd->pport[pidx].statusp)
			*dd->pport[pidx].statusp &= ~QIB_STATUS_CHIP_PRESENT;

		spin_lock(&dd->pport[pidx].cc_shadow_lock);

		kfree(dd->pport[pidx].congestion_entries);
		dd->pport[pidx].congestion_entries = NULL;
		kfree(dd->pport[pidx].ccti_entries);
		dd->pport[pidx].ccti_entries = NULL;
		kfree(dd->pport[pidx].ccti_entries_shadow);
		dd->pport[pidx].ccti_entries_shadow = NULL;
		kfree(dd->pport[pidx].congestion_entries_shadow);
		dd->pport[pidx].congestion_entries_shadow = NULL;

		spin_unlock(&dd->pport[pidx].cc_shadow_lock);
	}

	qib_disable_wc(dd);

	if (dd->pioavailregs_dma) {
		dma_free_coherent(&dd->pcidev->dev, PAGE_SIZE,
				  (void *) dd->pioavailregs_dma,
				  dd->pioavailregs_phys);
		dd->pioavailregs_dma = NULL;
	}

	if (dd->pageshadow) {
		struct page **tmpp = dd->pageshadow;
		dma_addr_t *tmpd = dd->physshadow;
		int i;

		for (ctxt = 0; ctxt < dd->cfgctxts; ctxt++) {
			int ctxt_tidbase = ctxt * dd->rcvtidcnt;
			int maxtid = ctxt_tidbase + dd->rcvtidcnt;

			for (i = ctxt_tidbase; i < maxtid; i++) {
				if (!tmpp[i])
					continue;
				pci_unmap_page(dd->pcidev, tmpd[i],
					       PAGE_SIZE, PCI_DMA_FROMDEVICE);
				qib_release_user_pages(&tmpp[i], 1);
				tmpp[i] = NULL;
			}
		}

		dd->pageshadow = NULL;
		vfree(tmpp);
		dd->physshadow = NULL;
		vfree(tmpd);
	}

	/*
	 * Free any resources still in use (usually just kernel contexts)
	 * at unload; we do for ctxtcnt, because that's what we allocate.
	 * We acquire lock to be really paranoid that rcd isn't being
	 * accessed from some interrupt-related code (that should not happen,
	 * but best to be sure).
	 */
	spin_lock_irqsave(&dd->uctxt_lock, flags);
	tmp = dd->rcd;
	dd->rcd = NULL;
	spin_unlock_irqrestore(&dd->uctxt_lock, flags);
	for (ctxt = 0; tmp && ctxt < dd->ctxtcnt; ctxt++) {
		struct qib_ctxtdata *rcd = tmp[ctxt];

		tmp[ctxt] = NULL; /* debugging paranoia */
		qib_free_ctxtdata(dd, rcd);
	}
	kfree(tmp);
	kfree(dd->boardname);
	qib_cq_exit(dd);
}

/*
 * Clean up on unit shutdown, or error during unit load after
 * successful initialization.
 */
static void qib_postinit_cleanup(struct qib_devdata *dd)
{
	/*
	 * Clean up chip-specific stuff.
	 * We check for NULL here, because it's outside
	 * the kregbase check, and we need to call it
	 * after the free_irq.  Thus it's possible that
	 * the function pointers were never initialized.
	 */
	if (dd->f_cleanup)
		dd->f_cleanup(dd);

	qib_pcie_ddcleanup(dd);

	cleanup_device_data(dd);

	qib_free_devdata(dd);
}

static int qib_init_one(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret, j, pidx, initfail;
	struct qib_devdata *dd = NULL;

	ret = qib_pcie_init(pdev, ent);
	if (ret)
		goto bail;

	/*
	 * Do device-specific initialiation, function table setup, dd
	 * allocation, etc.
	 */
	switch (ent->device) {
	case PCI_DEVICE_ID_QLOGIC_IB_6120:
#ifdef CONFIG_PCI_MSI
		dd = qib_init_iba6120_funcs(pdev, ent);
#else
		qib_early_err(&pdev->dev,
			"Intel PCIE device 0x%x cannot work if CONFIG_PCI_MSI is not enabled\n",
			ent->device);
		dd = ERR_PTR(-ENODEV);
#endif
		break;

	case PCI_DEVICE_ID_QLOGIC_IB_7220:
		dd = qib_init_iba7220_funcs(pdev, ent);
		break;

	case PCI_DEVICE_ID_QLOGIC_IB_7322:
		dd = qib_init_iba7322_funcs(pdev, ent);
		break;

	default:
		qib_early_err(&pdev->dev,
			"Failing on unknown Intel deviceid 0x%x\n",
			ent->device);
		ret = -ENODEV;
	}

	if (IS_ERR(dd))
		ret = PTR_ERR(dd);
	if (ret)
		goto bail; /* error already printed */

	ret = qib_create_workqueues(dd);
	if (ret)
		goto bail;

	/* do the generic initialization */
	initfail = qib_init(dd, 0);

	ret = qib_register_ib_device(dd);

	/*
	 * Now ready for use.  this should be cleared whenever we
	 * detect a reset, or initiate one.  If earlier failure,
	 * we still create devices, so diags, etc. can be used
	 * to determine cause of problem.
	 */
	if (!qib_mini_init && !initfail && !ret)
		dd->flags |= QIB_INITTED;

	j = qib_device_create(dd);
	if (j)
		qib_dev_err(dd, "Failed to create /dev devices: %d\n", -j);
	j = qibfs_add(dd);
	if (j)
		qib_dev_err(dd, "Failed filesystem setup for counters: %d\n",
			    -j);

	if (qib_mini_init || initfail || ret) {
		qib_stop_timers(dd);
		flush_workqueue(ib_wq);
		for (pidx = 0; pidx < dd->num_pports; ++pidx)
			dd->f_quiet_serdes(dd->pport + pidx);
		if (qib_mini_init)
			goto bail;
		if (!j) {
			(void) qibfs_remove(dd);
			qib_device_remove(dd);
		}
		if (!ret)
			qib_unregister_ib_device(dd);
		qib_postinit_cleanup(dd);
		if (initfail)
			ret = initfail;
		goto bail;
	}

	ret = qib_enable_wc(dd);
	if (ret) {
		qib_dev_err(dd,
			"Write combining not enabled (err %d): performance may be poor\n",
			-ret);
		ret = 0;
	}

	qib_verify_pioperf(dd);
bail:
	return ret;
}

static void qib_remove_one(struct pci_dev *pdev)
{
	struct qib_devdata *dd = pci_get_drvdata(pdev);
	int ret;

	/* unregister from IB core */
	qib_unregister_ib_device(dd);

	/*
	 * Disable the IB link, disable interrupts on the device,
	 * clear dma engines, etc.
	 */
	if (!qib_mini_init)
		qib_shutdown_device(dd);

	qib_stop_timers(dd);

	/* wait until all of our (qsfp) queue_work() calls complete */
	flush_workqueue(ib_wq);

	ret = qibfs_remove(dd);
	if (ret)
		qib_dev_err(dd, "Failed counters filesystem cleanup: %d\n",
			    -ret);

	qib_device_remove(dd);

	qib_postinit_cleanup(dd);
}

/**
 * qib_create_rcvhdrq - create a receive header queue
 * @dd: the qlogic_ib device
 * @rcd: the context data
 *
 * This must be contiguous memory (from an i/o perspective), and must be
 * DMA'able (which means for some systems, it will go through an IOMMU,
 * or be forced into a low address range).
 */
int qib_create_rcvhdrq(struct qib_devdata *dd, struct qib_ctxtdata *rcd)
{
	unsigned amt;
	int old_node_id;

	if (!rcd->rcvhdrq) {
		dma_addr_t phys_hdrqtail;
		gfp_t gfp_flags;

		amt = ALIGN(dd->rcvhdrcnt * dd->rcvhdrentsize *
			    sizeof(u32), PAGE_SIZE);
		gfp_flags = (rcd->ctxt >= dd->first_user_ctxt) ?
			GFP_USER : GFP_KERNEL;

		old_node_id = dev_to_node(&dd->pcidev->dev);
		set_dev_node(&dd->pcidev->dev, rcd->node_id);
		rcd->rcvhdrq = dma_alloc_coherent(
			&dd->pcidev->dev, amt, &rcd->rcvhdrq_phys,
			gfp_flags | __GFP_COMP);
		set_dev_node(&dd->pcidev->dev, old_node_id);

		if (!rcd->rcvhdrq) {
			qib_dev_err(dd,
				"attempt to allocate %d bytes for ctxt %u rcvhdrq failed\n",
				amt, rcd->ctxt);
			goto bail;
		}

		if (rcd->ctxt >= dd->first_user_ctxt) {
			rcd->user_event_mask = vmalloc_user(PAGE_SIZE);
			if (!rcd->user_event_mask)
				goto bail_free_hdrq;
		}

		if (!(dd->flags & QIB_NODMA_RTAIL)) {
			set_dev_node(&dd->pcidev->dev, rcd->node_id);
			rcd->rcvhdrtail_kvaddr = dma_alloc_coherent(
				&dd->pcidev->dev, PAGE_SIZE, &phys_hdrqtail,
				gfp_flags);
			set_dev_node(&dd->pcidev->dev, old_node_id);
			if (!rcd->rcvhdrtail_kvaddr)
				goto bail_free;
			rcd->rcvhdrqtailaddr_phys = phys_hdrqtail;
		}

		rcd->rcvhdrq_size = amt;
	}

	/* clear for security and sanity on each use */
	memset(rcd->rcvhdrq, 0, rcd->rcvhdrq_size);
	if (rcd->rcvhdrtail_kvaddr)
		memset(rcd->rcvhdrtail_kvaddr, 0, PAGE_SIZE);
	return 0;

bail_free:
	qib_dev_err(dd,
		"attempt to allocate 1 page for ctxt %u rcvhdrqtailaddr failed\n",
		rcd->ctxt);
	vfree(rcd->user_event_mask);
	rcd->user_event_mask = NULL;
bail_free_hdrq:
	dma_free_coherent(&dd->pcidev->dev, amt, rcd->rcvhdrq,
			  rcd->rcvhdrq_phys);
	rcd->rcvhdrq = NULL;
bail:
	return -ENOMEM;
}

/**
 * allocate eager buffers, both kernel and user contexts.
 * @rcd: the context we are setting up.
 *
 * Allocate the eager TID buffers and program them into hip.
 * They are no longer completely contiguous, we do multiple allocation
 * calls.  Otherwise we get the OOM code involved, by asking for too
 * much per call, with disastrous results on some kernels.
 */
int qib_setup_eagerbufs(struct qib_ctxtdata *rcd)
{
	struct qib_devdata *dd = rcd->dd;
	unsigned e, egrcnt, egrperchunk, chunk, egrsize, egroff;
	size_t size;
	gfp_t gfp_flags;
	int old_node_id;

	/*
	 * GFP_USER, but without GFP_FS, so buffer cache can be
	 * coalesced (we hope); otherwise, even at order 4,
	 * heavy filesystem activity makes these fail, and we can
	 * use compound pages.
	 */
	gfp_flags = __GFP_RECLAIM | __GFP_IO | __GFP_COMP;

	egrcnt = rcd->rcvegrcnt;
	egroff = rcd->rcvegr_tid_base;
	egrsize = dd->rcvegrbufsize;

	chunk = rcd->rcvegrbuf_chunks;
	egrperchunk = rcd->rcvegrbufs_perchunk;
	size = rcd->rcvegrbuf_size;
	if (!rcd->rcvegrbuf) {
		rcd->rcvegrbuf =
			kzalloc_node(chunk * sizeof(rcd->rcvegrbuf[0]),
				GFP_KERNEL, rcd->node_id);
		if (!rcd->rcvegrbuf)
			goto bail;
	}
	if (!rcd->rcvegrbuf_phys) {
		rcd->rcvegrbuf_phys =
			kmalloc_node(chunk * sizeof(rcd->rcvegrbuf_phys[0]),
				GFP_KERNEL, rcd->node_id);
		if (!rcd->rcvegrbuf_phys)
			goto bail_rcvegrbuf;
	}
	for (e = 0; e < rcd->rcvegrbuf_chunks; e++) {
		if (rcd->rcvegrbuf[e])
			continue;

		old_node_id = dev_to_node(&dd->pcidev->dev);
		set_dev_node(&dd->pcidev->dev, rcd->node_id);
		rcd->rcvegrbuf[e] =
			dma_alloc_coherent(&dd->pcidev->dev, size,
					   &rcd->rcvegrbuf_phys[e],
					   gfp_flags);
		set_dev_node(&dd->pcidev->dev, old_node_id);
		if (!rcd->rcvegrbuf[e])
			goto bail_rcvegrbuf_phys;
	}

	rcd->rcvegr_phys = rcd->rcvegrbuf_phys[0];

	for (e = chunk = 0; chunk < rcd->rcvegrbuf_chunks; chunk++) {
		dma_addr_t pa = rcd->rcvegrbuf_phys[chunk];
		unsigned i;

		/* clear for security and sanity on each use */
		memset(rcd->rcvegrbuf[chunk], 0, size);

		for (i = 0; e < egrcnt && i < egrperchunk; e++, i++) {
			dd->f_put_tid(dd, e + egroff +
					  (u64 __iomem *)
					  ((char __iomem *)
					   dd->kregbase +
					   dd->rcvegrbase),
					  RCVHQ_RCV_TYPE_EAGER, pa);
			p