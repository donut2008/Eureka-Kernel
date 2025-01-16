qib_devdata *dd)
{
	dd->upd_pio_shadow = 1;

	/* not atomic, but if we lose a stat count in a while, that's OK */
	qib_stats.sps_nopiobufs++;
}

/*
 * Common code for normal driver send buffer allocation, and reserved
 * allocation.
 *
 * Do appropriate marking as busy, etc.
 * Returns buffer pointer if one is found, otherwise NULL.
 */
u32 __iomem *qib_getsendbuf_range(struct qib_devdata *dd, u32 *pbufnum,
				  u32 first, u32 last)
{
	unsigned i, j, updated = 0;
	unsigned nbufs;
	unsigned long flags;
	unsigned long *shadow = dd->pioavailshadow;
	u32 __iomem *buf;

	if (!(dd->flags & QIB_PRESENT))
		return NULL;

	nbufs = last - first + 1; /* number in range to check */
	if (dd->upd_pio_shadow) {
update_shadow:
		/*
		 * Minor optimization.  If we had no buffers on last call,
		 * start out by doing the update; continue and do scan even
		 * if no buffers were updated, to be paranoid.
		 */
		update_send_bufs(dd);
		updated++;
	}
	i = first;
	/*
	 * While test_and_set_bit() is atomic, we do that and then the
	 * change_bit(), and the pair is not.  See if this is the cause
	 * of the remaining armlaunch errors.
	 */
	spin_lock_irqsave(&dd->pioavail_lock, flags);
	if (dd->last_pio >= first && dd->last_pio <= last)
		i = dd->last_pio + 1;
	if (!first)
		/* adjust to min possible  */
		nbufs = last - dd->min_kernel_pio + 1;
	for (j = 0; j < nbufs; j++, i++) {
		if (i > last)
			i = !first ? dd->min_kernel_pio : first;
		if (__test_and_set_bit((2 * i) + 1, shadow))
			continue;
		/* flip generation bit */
		__change_bit(2 * i, shadow);
		/* remember that the buffer can be written to now */
		__set_bit(i, dd->pio_writing);
		if (!first && first != last) /* first == last on VL15, avoid */
			dd->last_pio = i;
		break;
	}
	spin_unlock_irqrestore(&dd->pioavail_lock, flags);

	if (j == nbufs) {
		if (!updated)
			/*
			 * First time through; shadow exhausted, but may be
			 * buffers available, try an update and then rescan.
			 */
			goto update_shadow;
		no_send_bufs(dd);
		buf = NULL;
	} else {
		if (i < dd->piobcnt2k)
			buf = (u32 __iomem *)(dd->pio2kbase +
				i * dd->palign);
		else if (i < dd->piobcnt2k + dd->piobcnt4k || !dd->piovl15base)
			buf = (u32 __iomem *)(dd->pio4kbase +
				(i - dd->piobcnt2k) * dd->align4k);
		else
			buf = (u32 __iomem *)(dd->piovl15base +
				(i - (dd->piobcnt2k + dd->piobcnt4k)) *
				dd->align4k);
		if (pbufnum)
			*pbufnum = i;
		dd->upd_pio_shadow = 0;
	}

	return buf;
}

/*
 * Record that the caller is finished writing to the buffer so we don't
 * disarm it while it is being written and disarm it now if needed.
 */
void qib_sendbuf_done(struct qib_devdata *dd, unsigned n)
{
	unsigned long flags;

	spin_lock_irqsave(&dd->pioavail_lock, flags);
	__clear_bit(n, dd->pio_writing);
	if (__test_and_clear_bit(n, dd->pio_need_disarm))
		dd->f_sendctrl(dd->pport, QIB_SENDCTRL_DISARM_BUF(n));
	spin_unlock_irqrestore(&dd->pioavail_lock, flags);
}

/**
 * qib_chg_pioavailkernel - change which send buffers are available for kernel
 * @dd: the qlogic_ib device
 * @start: the starting send buffer number
 * @len: the number of send buffers
 * @avail: true if the buffers are available for kernel use, false otherwise
 */
void qib_chg_pioavailkernel(struct qib_devdata *dd, unsigned start,
	unsigned len, u32 avail, struct qib_ctxtdata *rcd)
{
	unsigned long flags;
	unsigned end;
	unsigned ostart = start;

	/* There are two bits per send buffer (busy and generation) */
	start *= 2;
	end = start + len * 2;

	spin_lock_irqsave(&dd->pioavail_lock, flags);
	/* Set or clear the busy bit in the shadow. */
	while (start < end) {
		if (avail) {
			unsigned long dma;
			int i;

			/*
			 * The BUSY bit will never be set, because we disarm
			 * the user buffers before we hand them back to the
			 * kernel.  We do have to make sure the generation
			 * bit is set correctly in shadow, since it could
			 * have changed many times while allocated to user.
			 * We can't use the bitmap functions on the full
			 * dma array because it is always little-endian, so
			 * we have to flip to host-order first.
			 * BITS_PER_LONG is slightly wrong, since it's
			 * always 64 bits per register in chip...
			 * We only work on 64 bit kernels, so that's OK.
			 */
			i = start / BITS_PER_LONG;
			__clear_bit(QLOGIC_IB_SENDPIOAVAIL_BUSY_SHIFT + start,
				    dd->pioavailshadow);
			dma = (unsigned long)
				le64_to_cpu(dd->pioavailregs_dma[i]);
			if (test_bit((QLOGIC_IB_SENDPIOAVAIL_CHECK_SHIFT +
				      start) % BITS_PER_LONG, &dma))
				__set_bit(QLOGIC_IB_SENDPIOAVAIL_CHECK_SHIFT +
					  start, dd->pioavailshadow);
			else
				__clear_bit(QLOGIC_IB_SENDPIOAVAIL_CHECK_SHIFT
					    + start, dd->pioavailshadow);
			__set_bit(start, dd->pioavailkernel);
			if ((start >> 1) < dd->min_kernel_pio)
				dd->min_kernel_pio = start >> 1;
		} else {
			__set_bit(start + QLOGIC_IB_SENDPIOAVAIL_BUSY_SHIFT,
				  dd->pioavailshadow);
			__clear_bit(start, dd->pioavailkernel);
			if ((start >> 1) > dd->min_kernel_pio)
				dd->min_kernel_pio = start >> 1;
		}
		start += 2;
	}

	if (dd->min_kernel_pio > 0 && dd->last_pio < dd->min_kernel_pio - 1)
		dd->last_pio = dd->min_kernel_pio - 1;
	spin_unlock_irqrestore(&dd->pioavail_lock, flags);

	dd->f_txchk_change(dd, ostart, len, avail, rcd);
}

/*
 * Flush all sends that might be in the ready to send state, as well as any
 * that are in the process of being sent.  Used whenever we need to be
 * sure the send side is idle.  Cleans up all buffer state by canceling
 * all pio buffers, and issuing an abort, which cleans up anything in the
 * launch fifo.  The cancel is superfluous on some chip versions, but
 * it's safer to always do it.
 * PIOAvail bits are updated by the chip as if a normal send had happened.
 */
void qib_cancel_sends(struct qib_pportdata *ppd)
{
	struct qib_devdata *dd = ppd->dd;
	struct qib_ctxtdata *rcd;
	unsigned long flags;
	unsigned ctxt;
	unsigned i;
	unsigned last;

	/*
	 * Tell PSM to disarm buffers again before trying to reuse them.
	 * We need to be sure the rcd doesn't change out from under us
	 * while we do so.  We hold the two locks sequentially.  We might
	 * needlessly set some need_disarm bits as a result, if the
	 * context is closed after we release the uctxt_lock, but that's
	 * fairly benign, and safer than nesting the locks.
	 */
	for (ctxt = dd->first_user_ctxt; ctxt < dd->cfgctxts; ctxt++) {
		spin_lock_irqsave(&dd->uctxt_lock, flags);
		rcd = dd->rcd[ctxt];
		if (rcd && rcd->ppd == ppd) {
			last = rcd->pio_base + rcd->piocnt;
			if (rcd->user_event_mask) {
				/*
				 * subctxt_cnt is 0 if not shared, so do base
				 * separately, first, then remaining subctxt,
				 * if any
				 */
				set_bit(_QIB_EVENT_DISARM_BUFS_BIT,
					&rcd->user_event_mask[0]);
				for (i = 1; i < rcd->subctxt_cnt; i++)
					set_bit(_QIB_EVENT_DISARM_BUFS_BIT,
						&rcd->user_event_mask[i]);
			}
			i = rcd->pio_base;
			spin_unlock_irqrestore(&dd->uctxt_lock, flags);
			spin_lock_irqsave(&dd->pioavail_lock, flags);
			for (; i < last; i++)
				__set_bit(i, dd->pio_need_disarm);
			spin_unlock_irqrestore(&dd->pioavail_lock, flags);
		} else
			spin_unlock_irqrestore(&dd->uctxt_lock, flags);
	}

	if (!(dd->flags & QIB_HAS_SEND_DMA))
		dd->f_sendctrl(ppd, QIB_SENDCTRL_DISARM_ALL |
				    QIB_SENDCTRL_FLUSH);
}

/*
 * Force an update of in-memory copy of the pioavail registers, when
 * needed for any of a variety of reasons.
 * If already off, this routine is a nop, on the assumption that the
 * caller (or set of callers) will "do the right thing".
 * This is a per-device operation, so just the first port.
 */
void qib_force_pio_avail_update(struct qib_devdata *dd)
{
	dd->f_sendctrl(dd->pport, QIB_SENDCTRL_AVAIL_BLIP);
}

void qib_hol_down(struct qib_pportdata *ppd)
{
	/*
	 * Cancel sends when the link goes DOWN so that we aren't doing it
	 * at INIT when we might be trying to send SMI packets.
	 */
	if (!(ppd->lflags & QIBL_IB_AUTONEG_INPROG))
		qib_cancel_sends(ppd);
}

/*
 * Link is at INIT.
 * We start the HoL timer so we can detect stuck packets blocking SMP replies.
 * Timer may already be running, so use mod_timer, not add_timer.
 */
void qib_hol_init(struct qib_pportdata *ppd)
{
	if (ppd->hol_state != QIB_HOL_INIT) {
		ppd->hol_state = QIB_HOL_INIT;
		mod_timer(&ppd->hol_timer,
			  jiffies + msecs_to_jiffies(qib_hol_timeout_ms));
	}
}

/*
 * Link is up, continue any user processes, and ensure timer
 * is a nop, if running.  Let timer keep running, if set; it
 * will nop when it sees the link is up.
 */
void qib_hol_up(struct qib_pportdata *ppd)
{
	ppd->hol_state = QIB_HOL_UP;
}

/*
 * This is only called via the timer.
 */
void qib_hol_event(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)opaque;

	/* If hardware error, etc, skip. */
	if (!(ppd->dd->flags & QIB_INITTED))
		return;

	if (ppd->hol_state != QIB_HOL_UP) {
		/*
		 * Try to flush sends in case a stuck packet is blocking
		 * SMP replies.
		 */
		qib_hol_down(ppd);
		mod_timer(&ppd->hol_timer,
			  jiffies + msecs_to_jiffies(qib_hol_timeout_ms));
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                