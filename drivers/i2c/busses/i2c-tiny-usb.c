d->physshadow[ctxttid + tid] = dd->tidinvalid;
			/* PERFORMANCE: below should almost certainly be
			 * cached
			 */
			dd->f_put_tid(dd, &tidbase[tid],
				      RCVHQ_RCV_TYPE_EXPECTED, dd->tidinvalid);
			pci_unmap_page(dd->pcidev, phys, PAGE_SIZE,
				       PCI_DMA_FROMDEVICE);
			qib_release_user_pages(&p, 1);
		}
	}
done:
	return ret;
}

/**
 * qib_set_part_key - set a partition key
 * @rcd: the context
 * @key: the key
 *
 * We can have up to 4 active at a time (other than the default, which is
 * always allowed).  This is somewhat tricky, since multiple contexts may set
 * the same key, so we reference count them, and clean up at exit.  All 4
 * partition keys are packed into a single qlogic_ib register.  It's an
 * error for a process to set the same pkey multiple times.  We provide no
 * mechanism to de-allocate a pkey at this time, we may eventually need to
 * do that.  I've used the atomic operations, and no locking, and only make
 * a single pass through what's available.  This should be more than
 * adequate for some time. I'll think about spinlocks or the like if and as
 * it's necessary.
 */
static int qib_set_part_key(struct qib_ctxtdata *rcd, u16 key)
{
	struct qib_pportdata *ppd = rcd->ppd;
	int i, any = 0, pidx = -1;
	u16 lkey = key & 0x7FFF;
	int ret;

	if (lkey == (QIB_DEFAULT_P_KEY & 0x7FFF)) {
		/* nothing to do; this key always valid */
		ret = 0;
		goto bail;
	}

	if (!lkey) {
		ret = -EINVAL;
		goto bail;
	}

	/*
	 * Set the full membership bit, because it has to be
	 * set in the register or the packet, and it seems
	 * cleaner to set in the register than to force all
	 * callers to set it.
	 */
	key |= 0x8000;

	for (i = 0; i < ARRAY_SIZE(rcd->pkeys); i++) {
		if (!rcd->pkeys[i] && pidx == -1)
			pidx = i;
		if (rcd->pkeys[i] == key) {
			ret = -EEXIST;
			goto bail;
		}
	}
	if (pidx == -1) {
		ret = -EBUSY;
		goto bail;
	}
	for (any = i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (!ppd->pkeys[i]) {
			any++;
			continue;
		}
		if (ppd->pkeys[i] == key) {
			atomic_t *pkrefs = &ppd->pkeyrefs[i];

			if (atomic_inc_return(pkrefs) > 1) {
				rcd->pkeys[pidx] = key;
				ret = 0;
				goto bail;
			} else {
				/*
				 * lost race, decrement count, catch below
				 */
				atomic_dec(pkrefs);
				any++;
			}
		}
		if ((ppd->pkeys[i] & 0x7FFF) == lkey) {
			/*
			 * It makes no sense to have both the limited and
			 * full membership PKEY set at the same time since
			 * the unlimited one will disable the limited one.
			 */
			ret = -EEXIST;
			goto bail;
		}
	}
	if (!any) {
		ret = -EBUSY;
		goto bail;
	}
	for (any = i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (!ppd->pkeys[i] &&
		    atomic_inc_return(&ppd->pkeyrefs[i]) == 1) {
			rcd->pkeys[pidx] = key;
			ppd->pkeys[i] = key;
			(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PKEYS, 0);
			ret = 0;
			goto bail;
		}
	}
	ret = -EBUSY;

bail:
	return ret;
}

/**
 * qib_manage_rcvq - manage a context's receive queue
 * @rcd: the context
 * @subctxt: the subcontext
 * @start_stop: action to carry out
 *
 * start_stop == 0 disables receive on the context, for use in queue
 * overflow conditions.  start_stop==1 re-enables, to be used to
 * re-init the software copy of the head register
 */
static int qib_manage_rcvq(struct qib_ctxtdata *rcd, unsigned subctxt,
			   int start_stop)
{
	struct qib_devdata *dd = rcd->dd;
	unsigned int rcvctrl_op;

	if (subctxt)
		goto bail;
	/* atomically clear receive enable ctxt. */
	if (start_stop) {
		/*
		 * On enable, force in-memory copy of the tail register to
		 * 0, so that protocol code doesn't have to worry about
		 * whether or not the chip has yet updated the in-memory
		 * copy or not on return from the system call. The chip
		 * always resets it's tail register back to 0 on a
		 * transition from disabled to enabled.
		 */
		if (rcd->rcvhdrtail_kvaddr)
			qib_clear_rcvhdrtail(rcd);
		rcvctrl_op = QIB_RCVCTRL_CTXT_ENB;
	} else
		rcvctrl_op = QIB_RCVCTRL_CTXT_DIS;
	dd->f_rcvctrl(rcd->ppd, rcvctrl_op, rcd->ctxt);
	/* always; new head should be equal to new tail; see above */
bail:
	return 0;
}

static void qib_clean_part_key(struct qib_ctxtdata *rcd,
			       struct qib_devdata *dd)
{
	int i, j, pchanged = 0;
	u64 oldpkey;
	struct qib_pportdata *ppd = rcd->ppd;

	/* for debugging only */
	oldpkey = (u64) ppd->pkeys[0] |
		((u64) ppd->pkeys[1] << 16) |
		((u64) ppd->pkeys[2] << 32) |
		((u64) ppd->pkeys[3] << 48);

	for (i = 0; i < ARRAY_SIZE(rcd->pkeys); i++) {
		if (!rcd->pkeys[i])
			continue;
		for (j = 0; j < ARRAY_SIZE(ppd->pkeys); j++) {
			/* check for match independent of the global bit */
			if ((ppd->pkeys[j] & 0x7fff) !=
			    (rcd->pkeys[i] & 0x7fff))
				continue;
			if (atomic_dec_and_test(&ppd->pkeyrefs[j])) {
				ppd->pkeys[j] = 0;
				pchanged++;
			}
			break;
		}
		rcd->pkeys[i] = 0;
	}
	if (pchanged)
		(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PKEYS, 0);
}

/* common code for the mappings on dma_alloc_coherent mem */
static int qib_mmap_mem(struct vm_area_struct *vma, struct qib_ctxtdata *rcd,
			unsigned len, void *kvaddr, u32 write_ok, char *what)
{
	struct qib_devdata *dd = rcd->dd;
	unsigned long pfn;
	int ret;

	if ((vma->vm_end - vma->vm_start) > len) {
		qib_devinfo(dd->pcidev,
			 "FAIL on %s: len %lx > %x\n", what,
			 vma->vm_end - vma->vm_start, len);
		ret = -EFAULT;
		goto bail;
	}

	/*
	 * shared context user code requires rcvhdrq mapped r/w, others
	 * only allowed readonly mapping.
	 */
	if (!write_ok) {
		if (vma->vm_flags & VM_WRITE) {
			qib_devinfo(dd->pcidev,
				 "%s must be mapped readonly\n", what);
			ret = -EPERM;
			goto bail;
		}

		/* don't allow them to later change with mprotect */
		vma->vm_flags &= ~VM_MAYWRITE;
	}

	pfn = virt_to_phys(kvaddr) >> PAGE_SHIFT;
	ret = remap_pfn_range(vma, vma->vm_start, pfn,
			      len, vma->vm_page_prot);
	if (ret)
		qib_devinfo(dd->pcidev,
			"%s ctxt%u mmap of %lx, %x bytes failed: %d\n",
			what, rcd->ctxt, pfn, len, ret);
bail:
	return ret;
}

static int mmap_ureg(struct vm_area_struct *vma, struct qib_devdata *dd,
		     u64 ureg)
{
	unsigned long phys;
	unsigned long sz;
	int ret;

	/*
	 * This is real hardware, so use io_remap.  This is the mechanism
	 * for the user process to update the head registers for their ctxt
	 * in the chip.
	 */
	sz = dd->flags & QIB_HAS_HDRSUPP ? 2 * PAGE_SIZE : PAGE_SIZE;
	if ((vma->vm_end - vma->vm_start) > sz) {
		qib_devinfo(dd->pcidev,
			"FAIL mmap userreg: reqlen %lx > PAGE\n",
			vma->vm_end - vma->vm_start);
		ret = -EFAULT;
	} else {
		phys = dd->physaddr + ureg;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

		vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND;
		ret = io_remap_pfn_range(vma, vma->vm_start,
					 phys >> PAGE_SHIFT,
					 vma->vm_end - vma->vm_start,
					 vma->vm_page_prot);
	}
	return ret;
}

static int mmap_piobufs(struct vm_area_struct *vma,
			struct qib_devdata *dd,
			struct qib_ctxtdata *rcd,
			unsigned piobufs, unsigned piocnt)
{
	unsigned long phys;
	int ret;

	/*
	 * When we map the PIO buffers in the chip, we want to map them as
	 * writeonly, no read possible; unfortunately, x86 doesn't allow
	 * for this in hardware, but we still prevent users from asking
	 * for it.
	 */
	if ((vma->vm_end - vma->vm_start) > (piocnt * dd->palign)) {
		qib_devinfo(dd->pcidev,
			"FAIL mmap piobufs: reqlen %lx > PAGE\n",
			 vma->vm_end - vma->vm_start);
		ret = -EINVAL;
		goto bail;
	}

	phys = dd->physaddr + piobufs;

#if defined(__powerpc__)
	/* There isn't a generic way to specify writethrough mappings */
	pgprot_val(vma->vm_page_prot) |= _PAGE_NO_CACHE;
	pgprot_val(vma->vm_page_prot) |= _PAGE_WRITETHRU;
	pgprot_val(vma->vm_page_prot) &= ~_PAGE_GUARDED;
#endif

	/*
	 * don't allow them to later change to readable with mprotect (for when
	 * not initially mapped readable, as is normally the case)
	 */
	vma->vm_flags &= ~VM_MAYREAD;
	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND;

	/* We used PAT if wc_cookie == 0 */
	if (!dd->wc_cookie)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	ret = io_r