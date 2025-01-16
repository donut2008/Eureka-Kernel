egrbufs(struct vm_area_struct *vma,
			   struct qib_ctxtdata *rcd)
{
	struct qib_devdata *dd = rcd->dd;
	unsigned long start, size;
	size_t total_size, i;
	unsigned long pfn;
	int ret;

	size = rcd->rcvegrbuf_size;
	total_size = rcd->rcvegrbuf_chunks * size;
	if ((vma->vm_end - vma->vm_start) > total_size) {
		qib_devinfo(dd->pcidev,
			"FAIL on egr bufs: reqlen %lx > actual %lx\n",
			 vma->vm_end - vma->vm_start,
			 (unsigned long) total_size);
		ret = -EINVAL;
		goto bail;
	}

	if (vma->vm_flags & VM_WRITE) {
		qib_devinfo(dd->pcidev,
			"Can't map eager buffers as writable (flags=%lx)\n",
			vma->vm_flags);
		ret = -EPERM;
		goto bail;
	}
	/* don't allow them to later change to writeable with mprotect */
	vma->vm_flags &= ~VM_MAYWRITE;

	start = vma->vm_start;

	for (i = 0; i < rcd->rcvegrbuf_chunks; i++, start += size) {
		pfn = virt_to_phys(rcd->rcvegrbuf[i]) >> PAGE_SHIFT;
		ret = remap_pfn_range(vma, start, pfn, size,
				      vma->vm_page_prot);
		if (ret < 0)
			goto bail;
	}
	ret = 0;

bail:
	return ret;
}

/*
 * qib_file_vma_fault - handle a VMA page fault.
 */
static int qib_file_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;

	page = vmalloc_to_page((void *)(vmf->pgoff << PAGE_SHIFT));
	if (!page)
		return VM_FAULT_SIGBUS;

	get_page(page);
	vmf->page = page;

	return 0;
}

static const struct vm_operations_struct qib_file_vm_ops = {
	.fault = qib_file_vma_fault,
};

static int mmap_kvaddr(struct vm_area_struct *vma, u64 pgaddr,
		       struct qib_ctxtdata *rcd, unsigned subctxt)
{
	struct qib_devdata *dd = rcd->dd;
	unsigned subctxt_cnt;
	unsigned long len;
	void *addr;
	size_t size;
	int ret = 0;

	subctxt_cnt = rcd->subctxt_cnt;
	size = rcd->rcvegrbuf_chunks * rcd->rcvegrbuf_size;

	/*
	 * Each process has all the subctxt uregbase, rcvhdrq, and
	 * rcvegrbufs mmapped - as an array for all the processes,
	 * and also separately for this process.
	 */
	if (pgaddr == cvt_kvaddr(rcd->subctxt_uregbase)) {
		addr = rcd->subctxt_uregbase;
		size = PAGE_SIZE * subctxt_cnt;
	} else if (pgaddr == cvt_kvaddr(rcd->subctxt_rcvhdr_base)) {
		addr = rcd->subctxt_rcvhdr_base;
		size = rcd->rcvhdrq_size * subctxt_cnt;
	} else if (pgaddr == cvt_kvaddr(rcd->subctxt_rcvegrbuf)) {
		addr = rcd->subctxt_rcvegrbuf;
		size *= subctxt_cnt;
	} else if (pgaddr == cvt_kvaddr(rcd->subctxt_uregbase +
					PAGE_SIZE * subctxt)) {
		addr = rcd->subctxt_uregbase + PAGE_SIZE * subctxt;
		size = PAGE_SIZE;
	} else if (pgaddr == cvt_kvaddr(rcd->subctxt_rcvhdr_base +
					rcd->rcvhdrq_size * subctxt)) {
		addr = rcd->subctxt_rcvhdr_base +
			rcd->rcvhdrq_size * subctxt;
		size = rcd->rcvhdrq_size;
	} else if (pgaddr == cvt_kvaddr(&rcd->user_event_mask[subctxt])) {
		addr = rcd->user_event_mask;
		size = PAGE_SIZE;
	} else if (pgaddr == cvt_kvaddr(rcd->subctxt_rcvegrbuf +
					size * subctxt)) {
		addr = rcd->subctxt_rcvegrbuf + size * subctxt;
		/* rcvegrbufs are read-only on the slave */
		if (vma->vm_flags & VM_WRITE) {
			qib_devinfo(dd->pcidev,
				 "Can't map eager buffers as writable (flags=%lx)\n",
				 vma->vm_flags);
			ret = -EPERM;
			goto bail;
		}
		/*
		 * Don't allow permission to later change to writeable
		 * with mprotect.
		 */
		vma->vm_flags &= ~VM_MAYWRITE;
	} else
		goto bail;
	len = vma->vm_end - vma->vm_start;
	if (len > size) {
		ret = -EINVAL;
		goto bail;
	}

	vma->vm_pgoff = (unsigned long) addr >> PAGE_SHIFT;
	vma->vm_ops = &qib_file_vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	ret = 1;

bail:
	return ret;
}

/**
 * qib_mmapf - mmap various structures into user space
 * @fp: the file pointer
 * @vma: the VM area
 *
 * We use this to have a shared buffer between the kernel and the user code
 * for the rcvhdr queue, egr buffers, and the per-context user regs and pio
 * buffers in the chip.  We have the open and close entries so we can bump
 * the ref count and keep the driver from being unloaded while still mapped.
 */
static int qib_mmapf(struct file *fp, struct vm_area_struct *vma)
{
	struct qib_ctxtdata *rcd;
	struct qib_devdata *dd;
	u64 pgaddr, ureg;
	unsigned piobufs, piocnt;
	int ret, match = 1;

	rcd = ctxt_fp(fp);
	if (!rcd || !(vma->vm_flags & VM_SHARED)) {
		ret = -EINVAL;
		goto bail;
	}
	dd = rcd->dd;

	/*
	 * This is the qib_do_user_init() code, mapping the shared buffers
	 * and per-context user registers into the user process. The address
	 * referred to by vm_pgoff is the file offset passed via mmap().
	 * For shared contexts, this is the kernel vmalloc() address of the
	 * pages to share with the master.
	 * For non-shared or master ctxts, this is a physical address.
	 * We only do one mmap for each space mapped.
	 */
	pgaddr = vma->vm_pgoff << PAGE_SHIFT;

	/*
	 * Check for 0 in case one of the allocations failed, but user
	 * called mmap anyway.
	 */
	if (!pgaddr)  {
		ret = -EINVAL;
		goto bail;
	}

	/*
	 * Physical addresses must fit in 40 bits for our hardware.
	 * Check for kernel virtual addresses first, anything else must
	 * match a HW or memory address.
	 */
	ret = mmap_kvaddr(vma, pgaddr, rcd, subctxt_fp(fp));
	if (ret) {
		if (ret > 0)
			ret = 0;
		goto bail;
	}

	ureg = dd->uregbase + dd->ureg_align * rcd->ctxt;
	if (!rcd->subctxt_cnt) {
		/* ctxt is not shared */
		piocnt = rcd->piocnt;
		piobufs = rcd->piobufs;
	} else if (!subctxt_fp(fp)) {
		/* caller is the master */
		piocnt = (rcd->piocnt / rcd->subctxt_cnt) +
			 (rcd->piocnt % rcd->subctxt_cnt);
		piobufs = rcd->piobufs +
			dd->palign * (rcd->piocnt - piocnt);
	} else {
		unsigned slave = subctxt_fp(fp) - 1;

		/* caller is a slave */
		piocnt = rcd->piocnt / rcd->subctxt_cnt;
		piobufs = rcd->piobufs + dd->palign * piocnt * slave;
	}

	if (pgaddr == ureg)
		ret = mmap_ureg(vma, dd, ureg);
	else if (pgaddr == piobufs)
		ret = mmap_piobufs(vma, dd, rcd, piobufs, piocnt);
	else if (pgaddr == dd->pioavailregs_phys)
		/* in-memory copy of pioavail registers */
		ret = qib_mmap_mem(vma, rcd, PAGE_SIZE,
				   (void *) dd->pioavailregs_dma, 0,
				   "pioavail registers");
	else if (pgaddr == rcd->rcvegr_phys)
		ret = mmap_rcvegrbufs(vma, rcd);
	else if (pgaddr == (u64) rcd->rcvhdrq_phys)
		/*
		 * The rcvhdrq itself; multiple pages, contiguous
		 * from an i/o perspective.  Shared contexts need
		 * to map r/w, so we allow writing.
		 */
		ret = qib_mmap_mem(vma, rcd, rcd->rcvhdrq_size,
				   rcd->rcvhdrq, 1, "rcvhdrq");
	else if (pgaddr == (u64) rcd->rcvhdrqtailaddr_phys)
		/* in-memory copy of rcvhdrq tail register */
		ret = qib_mmap_mem(vma, rcd, PAGE_SIZE,
				   rcd->rcvhdrtail_kvaddr, 0,
				   "rcvhdrq tail");
	else
		match = 0;
	if (!match)
		ret = -EINVAL;

	vma->vm_private_data = NULL;

	if (ret < 0)
		qib_devinfo(dd->pcidev,
			 "mmap Failure %d: off %llx len %lx\n",
			 -ret, (unsigned long long)pgaddr,
			 vma->vm_end - vma->vm_start);
bail:
	return ret;
}

static unsigned int qib_poll_urgent(struct qib_ctxtdata *rcd,
				    struct file *fp,
				    struct poll_table_struct *pt)
{
	struct qib_devdata *dd = rcd->dd;
	unsigned pollflag;

	poll_wait(fp, &rcd->wait, pt);

	spin_lock_irq(&dd->uctxt_lock);
	if (rcd->urgent != rcd->urgent_poll) {
		pollflag = POLLIN | POLLRDNORM;
		rcd->urgent_poll = rcd->urgent;
	} else {
		pollflag = 0;
		set_bit(QIB_CTXT_WAITING_URG, &rcd->flag);
	}
	spin_unlock_irq(&dd->uctxt_lock);

	return pollflag;
}

static unsigned int qib_poll_next(struct qib_ctxtdata *rcd,
				  struct file *fp,
				  struct poll_table_struct *pt)
{
	struct qib_devdata *dd = rcd->dd;
	unsigned pollflag;

	poll_wait(fp, &rcd->wait, pt);

	spin_lock_irq(&dd->uctxt_lock);
	if (dd->f_hdrqempty(rcd)) {
		set_bit(QIB_CTXT_WAITING_RCV, &rcd->flag);
		dd->f_rcvctrl(rcd->ppd, QIB_RCVCTRL_INTRAVAIL_ENB, rcd->ctxt);
		pollflag = 0;
	} else
		pollflag = POLLIN | POLLRDNORM;
	spin_unlock_irq(&dd->uctxt_lock);

	return pollflag;
}

static unsigned int qib_poll(struct file *fp, struct poll_table_struct *pt)
{
	struct qib_ctxtdata *rcd;
	unsigned pollflag;

	rcd = ctxt_fp(fp);
	if (!rcd)
		pollflag = POLLERR;
	else if (rcd->poll_type == QIB_POLL_TYPE_URGENT)
		pollflag = qib_poll_urgent(rcd, fp, pt);
	else  if (rcd->poll_type == QIB_POLL_TYPE_ANYRCV)
		pollflag = qib_poll_next(rcd, fp, pt);
	else /* invalid */
		pollflag = POLLERR;

	return pollflag;
}

static void assign_ctxt_affinity(struct file *fp, struct qib_devdata *dd)
{
	struct qib_filedata *fd = fp->private_data;
	const unsigned int weight = cpumask_weight(&current->cpus_allowed);
	const struct cpumask *local_mask = cpumask_of_pcibus(dd->pcidev->bus);
	int local_cpu;

	/*
	 * If process has NOT already set it's affinity, select and
	 * reserve a processor for it on the local NUMA node.
	 */
	if ((weight >= qib_cpulist_count) &&
		(cpumask_weight(local_mask) <= qib_cpulist_count)) {
		for_each_cpu(local_cpu, local_mask)
			if (!test_and_set_bit(local_cpu, qib_cpulist)) {
				fd->rec_cpu_num = local_cpu;
				return;
			}
	}

	/*
	 * If process has NOT already set it's affinity, select and
	 * reserve a