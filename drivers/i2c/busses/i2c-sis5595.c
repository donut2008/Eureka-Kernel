kinfo->spi_rcvhdrent_size = dd->rcvhdrentsize;
	kinfo->spi_tidegrcnt = rcd->rcvegrcnt;
	kinfo->spi_rcv_egrbufsize = dd->rcvegrbufsize;
	/*
	 * have to mmap whole thing
	 */
	kinfo->spi_rcv_egrbuftotlen =
		rcd->rcvegrbuf_chunks * rcd->rcvegrbuf_size;
	kinfo->spi_rcv_egrperchunk = rcd->rcvegrbufs_perchunk;
	kinfo->spi_rcv_egrchunksize = kinfo->spi_rcv_egrbuftotlen /
		rcd->rcvegrbuf_chunks;
	kinfo->spi_tidcnt = dd->rcvtidcnt / subctxt_cnt;
	if (master)
		kinfo->spi_tidcnt += dd->rcvtidcnt % subctxt_cnt;
	/*
	 * for this use, may be cfgctxts summed over all chips that
	 * are are configured and present
	 */
	kinfo->spi_nctxts = dd->cfgctxts;
	/* unit (chip/board) our context is on */
	kinfo->spi_unit = dd->unit;
	kinfo->spi_port = ppd->port;
	/* for now, only a single page */
	kinfo->spi_tid_maxsize = PAGE_SIZE;

	/*
	 * Doing this per context, and based on the skip value, etc.  This has
	 * to be the actual buffer size, since the protocol code treats it
	 * as an array.
	 *
	 * These have to be set to user addresses in the user code via mmap.
	 * These values are used on return to user code for the mmap target
	 * addresses only.  For 32 bit, same 44 bit address problem, so use
	 * the physical address, not virtual.  Before 2.6.11, using the
	 * page_address() macro worked, but in 2.6.11, even that returns the
	 * full 64 bit address (upper bits all 1's).  So far, using the
	 * physical addresses (or chip offsets, for chip mapping) works, but
	 * no doubt some future kernel release will change that, and we'll be
	 * on to yet another method of dealing with this.
	 * Normally only one of rcvhdr_tailaddr or rhf_offset is useful
	 * since the chips with non-zero rhf_offset don't normally
	 * enable tail register updates to host memory, but for testing,
	 * both can be enabled and used.
	 */
	kinfo->spi_rcvhdr_base = (u64) rcd->rcvhdrq_phys;
	kinfo->spi_rcvhdr_tailaddr = (u64) rcd->rcvhdrqtailaddr_phys;
	kinfo->spi_rhf_offset = dd->rhf_offset;
	kinfo->spi_rcv_egrbufs = (u64) rcd->rcvegr_phys;
	kinfo->spi_pioavailaddr = (u64) dd->pioavailregs_phys;
	/* setup per-unit (not port) status area for user programs */
	kinfo->spi_status = (u64) kinfo->spi_pioavailaddr +
		(char *) ppd->statusp -
		(char *) dd->pioavailregs_dma;
	kinfo->spi_uregbase = (u64) dd->uregbase + dd->ureg_align * rcd->ctxt;
	if (!shared) {
		kinfo->spi_piocnt = rcd->piocnt;
		kinfo->spi_piobufbase = (u64) rcd->piobufs;
		kinfo->spi_sendbuf_status = cvt_kvaddr(rcd->user_event_mask);
	} else if (master) {
		kinfo->spi_piocnt = (rcd->piocnt / subctxt_cnt) +
				    (rcd->piocnt % subctxt_cnt);
		/* Master's PIO buffers are after all the slave's */
		kinfo->spi_piobufbase = (u64) rcd->piobufs +
			dd->palign *
			(rcd->piocnt - kinfo->spi_piocnt);
	} else {
		unsigned slave = subctxt_fp(fp) - 1;

		kinfo->spi_piocnt = rcd->piocnt / subctxt_cnt;
		kinfo->spi_piobufbase = (u64) rcd->piobufs +
			dd->palign * kinfo->spi_piocnt * slave;
	}

	if (shared) {
		kinfo->spi_sendbuf_status =
			cvt_kvaddr(&rcd->user_event_mask[subctxt_fp(fp)]);
		/* only spi_subctxt_* fields should be set in this block! */
		kinfo->spi_subctxt_uregbase = cvt_kvaddr(rcd->subctxt_uregbase);

		kinfo->spi_subctxt_rcvegrbuf =
			cvt_kvaddr(rcd->subctxt_rcvegrbuf);
		kinfo->spi_subctxt_rcvhdr_base =
			cvt_kvaddr(rcd->subctxt_rcvhdr_base);
	}

	/*
	 * All user buffers are 2KB buffers.  If we ever support
	 * giving 4KB buffers to user processes, this will need some
	 * work.  Can't use piobufbase directly, because it has
	 * both 2K and 4K buffer base values.
	 */
	kinfo->spi_pioindex = (kinfo->spi_piobufbase - dd->pio2k_bufbase) /
		dd->palign;
	kinfo->spi_pioalign = dd->palign;
	kinfo->spi_qpair = QIB_KD_QP;
	/*
	 * user mode PIO buffers are always 2KB, even when 4KB can
	 * be received, and sent via the kernel; this is ibmaxlen
	 * for 2K MTU.
	 */
	kinfo->spi_piosize = dd->piosize2k - 2 * sizeof(u32);
	kinfo->spi_mtu = ppd->ibmaxlen; /* maxlen, not ibmtu */
	kinfo->spi_ctxt = rcd->ctxt;
	kinfo->spi_subctxt = subctxt_fp(fp);
	kinfo->spi_sw_version = QIB_KERN_SWVERSION;
	kinfo->spi_sw_version |= 1U << 31; /* QLogic-built, not kernel.org */
	kinfo->spi_hw_version = dd->revision;

	if (master)
		kinfo->spi_runtime_flags |= QIB_RUNTIME_MASTER;

	sz = (ubase_size < sizeof(*kinfo)) ? ubase_size : sizeof(*kinfo);
	if (copy_to_user(ubase, kinfo, sz))
		ret = -EFAULT;
bail:
	kfree(kinfo);
	return ret;
}

/**
 * qib_tid_update - update a context TID
 * @rcd: the context
 * @fp: the qib device file
 * @ti: the TID information
 *
 * The new implementation as of Oct 2004 is that the driver assigns
 * the tid and returns it to the caller.   To reduce search time, we
 * keep a cursor for each context, walking the shadow tid array to find
 * one that's not in use.
 *
 * For now, if we can't allocate the full list, we fail, although
 * in the long run, we'll allocate as many as we can, and the
 * caller will deal with that by trying the remaining pages later.
 * That means that when we fail, we have to mark the tids as not in
 * use again, in our shadow copy.
 *
 * It's up to the caller to free the tids when they are done.
 * We'll unlock the pages as they free them.
 *
 * Also, right now we are locking one page at a time, but since
 * the intended use of this routine is for a single group of
 * virtually contiguous pages, that should change to improve
 * performance.
 */
static int qib_tid_update(struct qib_ctxtdata *rcd, struct file *fp,
			  const struct qib_tid_info *ti)
{
	int ret = 0, ntids;
	u32 tid, ctxttid, cnt, i, tidcnt, tidoff;
	u16 *tidlist;
	struct qib_devdata *dd = rcd->dd;
	u64 physaddr;
	unsigned long vaddr;
	u64 __iomem *tidbase;
	unsigned long tidmap[8];
	struct page **pagep = NULL;
	unsigned subctxt = subctxt_fp(fp);

	if (!dd->pageshadow) {
		ret = -ENOMEM;
		goto done;
	}

	cnt = ti->tidcnt;
	if (!cnt) {
		ret = -EFAULT;
		goto done;
	}
	ctxttid = rcd->ctxt * dd->rcvtidcnt;
	if (!rcd->subctxt_cnt) {
		tidcnt = dd->rcvtidcnt;
		tid = rcd->tidcursor;
		tidoff = 0;
	} else if (!subctxt) {
		tidcnt = (dd->rcvtidcnt / rcd->subctxt_cnt) +
			 (dd->rcvtidcnt % rcd->subctxt_cnt);
		tidoff = dd->rcvtidcnt - tidcnt;
		ctxttid += tidoff;
		tid = tidcursor_fp(fp);
	} else {
		tidcnt = dd->rcvtidcnt / rcd->subctxt_cnt;
		tidoff = tidcnt * (subctxt - 1);
		ctxttid += tidoff;
		tid = tidcursor_fp(fp);
	}
	if (cnt > tidcnt) {
		/* make sure it all fits in tid_pg_list */
		qib_devinfo(dd->pcidev,
			"Process tried to allocate %u TIDs, only trying max (%u)\n",
			cnt, tidcnt);
		cnt = tidcnt;
	}
	pagep = (struct page **) rcd->tid_pg_list;
	tidlist = (u16 *) &pagep[dd->rcvtidcnt];
	pagep += tidoff;
	tidlist += tidoff;

	memset(tidmap, 0, sizeof(tidmap));
	/* before decrement; chip actual # */
	ntids = tidcnt;
	tidbase = (u64 __iomem *) (((char __iomem *) dd->kregbase) +
				   dd->rcvtidbase +
				   ctxttid * sizeof(*tidbase));

	/* virtual address of first page in transfer */
	vaddr = ti->tidvaddr;
	if (!access_ok(VERIFY_WRITE, (void __user *) vaddr,
		       cnt * PAGE_SIZE)) {
		ret = -EFAULT;
		goto done;
	}
	ret = qib_get_user_pages(vaddr, cnt, pagep);
	if (ret) {
		/*
		 * if (ret == -EBUSY)
		 * We can't continue because the pagep array won't be
		 * initialized. This should never happen,
		 * unless perhaps the user has mpin'ed the pages
		 * themselves.
		 */
		qib_devinfo(
			dd->pcidev,
			"Failed to lock addr %p, %u pages: errno %d\n",
			(void *) vaddr, cnt, -ret);
		goto done;
	}
	for (i = 0; i < cnt; i++, vaddr += PAGE_SIZE) {
		dma_addr_t daddr;

		for (; ntids--; tid++) {
			if (tid == tidcnt)
				tid = 0;
			if (!dd->pageshadow[ctxttid + tid])
				break;
		}
		if (ntids < 0) {
			/*
			 * Oops, wrapped all the way through their TIDs,
			 * and didn't have enough free; see comments at
			 * start of routine
			 */
			i--;    /* last tidlist[i] not filled in */
			ret = -ENOMEM;
			break;
		}
		ret = qib_map_page(dd->pcidev, pagep[i], &daddr);
		if (ret)
			break;

		tidlist[i] = tid + tidoff;
		/* we "know" system pages and TID pages are same size */
		dd->pageshadow[ctxttid + tid] = pagep[i];
		dd->physshadow[ctxttid + tid] = daddr;
		/*
		 * don't need atomic or it's overhead
		 */
		__set_bit(tid, tidmap);
		physaddr = dd->physshadow[ctxttid + tid];
		/* PERFORMANCE: below should almost certainly be cached */
		dd->f_put_tid(dd, &tidbase[tid],
				  RCVHQ_RCV_TYPE_EXPECTED, physaddr);
		/*
		 * don't check this tid in qib_ctxtshadow, since we
		 * just filled it in; start with the next one.
		 */
		tid++;
	}

	if (ret) {
		u32 limit;
cleanup:
		/* jump here if copy out of updated info failed... */
		/* same code that's in qib_free_tid() */
		limit = sizeof(tidmap) * BITS_PER_BYTE;
		if (limit > tidcnt)
			/* just in case size changes in future */
			limit = tidcnt;
		tid = find_first_bit((const unsigned long *)tidmap, limit);
		for (; tid < limit; tid++) {
			if (!test_bit(tid, tidmap))
				continue;
			if (dd->pageshadow[ctxttid + tid]) {
				dma_addr_t phys;

				phys = dd->physshadow[ctxttid + tid];
				dd->physshadow[ctxttid + tid] = dd->tidinvalid;
				/* PERFORMANCE: below should almost certainly
				 * be cached
				 */
				dd->f_put_tid(dd, &tidbase[tid],
					      RCVHQ_RCV_TYPE_EXPECTED,
					      dd->tidinvalid);
				pci_unmap_page(dd->pcidev, phys, PAGE_SIZE,
					       PCI_DMA_FROMDEVICE);
				dd->pageshadow[ctxttid + tid] = NULL;
			}
		}
		qib_release_user_pages(pagep, cnt);
	} else {
		/*
		 * Copy the updated array, with qib_tid's filled in, back
		 * to user.  Since we did the copy in already, this "should
		 * never fail" If it does, we have to clean up...
		 */
		if (copy_to_user((void __user *)
				 (unsigned long) ti->tidlist,
				 tidlist, cnt * sizeof(*tidlist))) {
			ret = -EFAULT;
			goto cleanup;
		}
		if (copy_to_user((void __user *) (unsigned long) ti->tidmap,
				 tidmap, sizeof(tidmap))) {
			ret = -EFAULT;
			goto cleanup;
		}
		if (tid == tidcnt)
			tid = 0;
		if (!rcd->subctxt_cnt)
			rcd->tidcursor = tid;
		else
			tidcursor_fp(fp) = tid;
	}

done:
	return ret;
}

/**
 * qib_tid_free - free a context TID
 * @rcd: the context
 * @subctxt: the subcontext
 * @ti: the TID info
 *
 * right now we are unlocking one page at a time, but since
 * the intended use of this routine is for a single group of
 * virtually contiguous pages, that should change to improve
 * performance.  We check that the TID is in range for this context
 * but otherwise don't check validity; if user has an error and
 * frees the wrong tid, it's only their own data that can thereby
 * be corrupted.  We do check that the TID was in use, for sanity
 * We always use our idea of the saved address, not the address that
 * they pass in to us.
 */
static int qib_tid_free(struct qib_ctxtdata *rcd, unsigned subctxt,
			const struct qib_tid_info *ti)
{
	int ret = 0;
	u32 tid, ctxttid, cnt, limit, tidcnt;
	struct qib_devdata *dd = rcd->dd;
	u64 __iomem *tidbase;
	unsigned long tidmap[8];

	if (!dd->pageshadow) {
		ret = -ENOMEM;
		goto done;
	}

	if (copy_from_user(tidmap, (void __user *)(unsigned long)ti->tidmap,
			   sizeof(tidmap))) {
		ret = -EFAULT;
		goto done;
	}

	ctxttid = rcd->ctxt * dd->rcvtidcnt;
	if (!rcd->subctxt_cnt)
		tidcnt = dd->rcvtidcnt;
	else if (!subctxt) {
		tidcnt = (dd->rcvtidcnt / rcd->subctxt_cnt) +
			 (dd->rcvtidcnt % rcd->subctxt_cnt);
		ctxttid += dd->rcvtidcnt - tidcnt;
	} else {
		tidcnt = dd->rcvtidcnt / rcd->subctxt_cnt;
		ctxttid += tidcnt * (subctxt - 1);
	}
	tidbase = (u64 __iomem *) ((char __iomem *)(dd->kregbase) +
				   dd->rcvtidbase +
				   ctxttid * sizeof(*tidbase));

	limit = sizeof(tidmap) * BITS_PER_BYTE;
	if (limit > tidcnt)
		/* just in case size c