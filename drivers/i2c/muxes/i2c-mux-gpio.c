_umem32(struct qib_devdata *dd, void __user *uaddr,
			   u32 regoffs, size_t count)
{
	const u32 __iomem *reg_addr;
	const u32 __iomem *reg_end;
	u32 limit;
	int ret;

	reg_addr = qib_remap_ioaddr32(dd, regoffs, &limit);
	if (reg_addr == NULL || limit == 0 || !(dd->flags & QIB_PRESENT)) {
		ret = -EINVAL;
		goto bail;
	}
	if (count >= limit)
		count = limit;
	reg_end = reg_addr + (count / sizeof(u32));

	/* not very efficient, but it works for now */
	while (reg_addr < reg_end) {
		u32 data = readl(reg_addr);

		if (copy_to_user(uaddr, &data, sizeof(data))) {
			ret = -EFAULT;
			goto bail;
		}

		reg_addr++;
		uaddr += sizeof(u32);

	}
	ret = 0;
bail:
	return ret;
}

/*
 * qib_write_umem32 - write a 32-bit quantity to the chip from user space
 * @dd: the qlogic_ib device
 * @regoffs: the offset from BAR0 (_NOT_ full pointer, anymore)
 * @uaddr: the source of the data in user memory
 * @count: number of bytes to copy
 *
 * write 32 bit values, not 64 bit; for memories that only
 * support 32 bit write; usually a single dword.
 */

static int qib_write_umem32(struct qib_devdata *dd, u32 regoffs,
			    const void __user *uaddr, size_t count)
{
	u32 __iomem *reg_addr;
	const u32 __iomem *reg_end;
	u32 limit;
	int ret;

	reg_addr = qib_remap_ioaddr32(dd, regoffs, &limit);
	if (reg_addr == NULL || limit == 0 || !(dd->flags & QIB_PRESENT)) {
		ret = -EINVAL;
		goto bail;
	}
	if (count >= limit)
		count = limit;
	reg_end = reg_addr + (count / sizeof(u32));

	while (reg_addr < reg_end) {
		u32 data;

		if (copy_from_user(&data, uaddr, sizeof(data))) {
			ret = -EFAULT;
			goto bail;
		}
		writel(data, reg_addr);

		reg_addr++;
		uaddr += sizeof(u32);
	}
	ret = 0;
bail:
	return ret;
}

static int qib_diag_open(struct inode *in, struct file *fp)
{
	int unit = iminor(in) - QIB_DIAG_MINOR_BASE;
	struct qib_devdata *dd;
	struct qib_diag_client *dc;
	int ret;

	mutex_lock(&qib_mutex);

	dd = qib_lookup(unit);

	if (dd == NULL || !(dd->flags & QIB_PRESENT) ||
	    !dd->kregbase) {
		ret = -ENODEV;
		goto bail;
	}

	dc = get_client(dd);
	if (!dc) {
		ret = -ENOMEM;
		goto bail;
	}
	dc->next = dd->diag_client;
	dd->diag_client = dc;
	fp->private_data = dc;
	ret = 0;
bail:
	mutex_unlock(&qib_mutex);

	return ret;
}

/**
 * qib_diagpkt_write - write an IB packet
 * @fp: the diag data device file pointer
 * @data: qib_diag_pkt structure saying where to get the packet
 * @count: size of data to write
 * @off: unused by this code
 */
static ssize_t qib_diagpkt_write(struct file *fp,
				 const char __user *data,
				 size_t count, loff_t *off)
{
	u32 __iomem *piobuf;
	u32 plen, pbufn, maxlen_reserve;
	struct qib_diag_xpkt dp;
	u32 *tmpbuf = NULL;
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	ssize_t ret = 0;

	if (count != sizeof(dp)) {
		ret = -EINVAL;
		goto bail;
	}
	if (copy_from_user(&dp, data, sizeof(dp))) {
		ret = -EFAULT;
		goto bail;
	}

	dd = qib_lookup(dp.unit);
	if (!dd || !(dd->flags & QIB_PRESENT) || !dd->kregbase) {
		ret = -ENODEV;
		goto bail;
	}
	if (!(dd->flags & QIB_INITTED)) {
		/* no hardware, freeze, etc. */
		ret = -ENODEV;
		goto bail;
	}

	if (dp.version != _DIAG_XPKT_VERS) {
		qib_dev_err(dd, "Invalid version %u for diagpkt_write\n",
			    dp.version);
		ret = -EINVAL;
		goto bail;
	}
	/* send count must be an exact number of dwords */
	if (dp.len & 3) {
		ret = -EINVAL;
		goto bail;
	}
	if (!dp.port || dp.port > dd->num_pports) {
		ret = -EINVAL;
		goto bail;
	}
	ppd = &dd->pport[dp.port - 1];

	/*
	 * need total length before first word written, plus 2 Dwords. One Dword
	 * is for padding so we get the full user data when not aligned on
	 * a word boundary. The other Dword is to make sure we have room for the
	 * ICRC which gets tacked on later.
	 */
	maxlen_reserve = 2 * sizeof(u32);
	if (dp.len > ppd->ibmaxlen - maxlen_reserve) {
		ret = -EINVAL;
		goto bail;
	}

	plen = sizeof(u32) + dp.len;

	tmpbuf = vmalloc(plen);
	if (!tmpbuf) {
		qib_devinfo(dd->pcidev,
			"Unable to allocate tmp buffer, failing\n");
		ret = -ENOMEM;
		goto bail;
	}

	if (copy_from_user(tmpbuf,
			   (const void __user *) (unsigned long) dp.data,
			   dp.len)) {
		ret = -EFAULT;
		goto bail;
	}

	plen >>= 2;             /* in dwords */

	if (dp.pbc_wd == 0)
		dp.pbc_wd = plen;

	piobuf = dd->f_getsendbuf(ppd, dp.pbc_wd, &pbufn);
	if (!piobuf) {
		ret = -EBUSY;
		goto bail;
	}
	/* disarm it just to be extra sure */
	dd->f_sendctrl(dd->pport, QIB_SENDCTRL_DISARM_BUF(pbufn));

	/* disable header check on pbufn for this packet */
	dd->f_txchk_change(dd, pbufn, 1, TXCHK_CHG_TYPE_DIS1, NULL);

	writeq(dp.pbc_wd, piobuf);
	/*
	 * Copy all but the trigger word, then flush, so it's written
	 * to chip before trigger word, then write trigger word, then
	 * flush again, so packet is sent.
	 */
	if (dd->flags & QIB_PIO_FLUSH_WC) {
		qib_flush_wc();
		qib_pio_copy(piobuf + 2, tmpbuf, plen - 1);
		qib_flush_wc();
		__raw_writel(tmpbuf[plen - 1], piobuf + plen + 1);
	} else
		qib_pio_copy(piobuf + 2, tmpbuf, plen);

	if (dd->flags & QIB_USE_SPCL_TRIG) {
		u32 spcl_off = (pbufn >= dd->piobcnt2k) ? 2047 : 1023;

		qib_flush_wc();
		__raw_writel(0xaebecede, piobuf + spcl_off);
	}

	/*
	 * Ensure buffer is written to the chip, then re-enable
	 * header checks (if supported by chip).  The txchk
	 * code will ensure seen by chip before returning.
	 */
	qib_flush_wc();
	qib_sendbuf_done(dd, pbufn);
	dd->f_txchk_change(dd, pbufn, 1, TXCHK_CHG_TYPE_ENAB1, NULL);

	ret = sizeof(dp);

bail:
	vfree(tmpbuf);
	return ret;
}

static int qib_diag_release(struct inode *in, struct file *fp)
{
	mutex_lock(&qib_mutex);
	return_client(fp->private_data);
	fp->private_data = NULL;
	mutex_unlock(&qib_mutex);
	return 0;
}

/*
 * Chip-specific code calls to register its interest in
 * a specific range.
 */
struct diag_observer_list_elt {
	struct diag_observer_list_elt *next;
	const struct diag_observer *op;
};

int qib_register_observer(struct qib_devdata *dd,
			  const struct diag_observer *op)
{
	struct diag_observer_list_elt *olp;
	unsigned long flags;

	if (!dd || !op)
		return -EINVAL;
	olp = vmalloc(sizeof(*olp));
	if (!olp) {
		pr_err("vmalloc for observer failed\n");
		return -ENOMEM;
	}

	spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
	olp->op = op;
	olp->next = dd->diag_observer_list;
	dd->diag_observer_list = olp;
	spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);

	return 0;
}

/* Remove all registered observers when device is closed */
static void qib_unregister_observers(struct qib_devdata *dd)
{
	struct diag_observer_list_elt *olp;
	unsigned long flags;

	spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
	olp = dd->diag_observer_list;
	while (olp) {
		/* Pop one observer, let go of lock */
		dd->diag_observer_list = olp->next;
		spin_unlock_irqrestore(&dd->qib_diag_trans_lock, flags);
		vfree(olp);
		/* try again. */
		spin_lock_irqsave(&dd->qib_diag_trans_lock, flags);
		olp = dd->diag_observer_list;