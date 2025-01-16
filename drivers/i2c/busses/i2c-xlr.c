	struct list_head *l = dev->txreq_free.next;

		list_del(l);
		spin_unlock(&dev->pending_lock);
		spin_unlock_irqrestore(&qp->s_lock, flags);
		tx = list_entry(l, struct qib_verbs_txreq, txreq.list);
	} else {
		if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK &&
		    list_empty(&qp->iowait)) {
			dev->n_txwait++;
			qp->s_flags |= QIB_S_WAIT_TX;
			list_add_tail(&qp->iowait, &dev->txwait);
		}
		qp->s_flags &= ~QIB_S_BUSY;
		spin_unlock(&dev->pending_lock);
		spin_unlock_irqrestore(&qp->s_lock, flags);
		tx = ERR_PTR(-EBUSY);
	}
	return tx;
}

static inline struct qib_verbs_txreq *get_txreq(struct qib_ibdev *dev,
					 struct qib_qp *qp)
{
	struct qib_verbs_txreq *tx;
	unsigned long flags;

	spin_lock_irqsave(&dev->pending_lock, flags);
	/* assume the list non empty */
	if (likely(!list_empty(&dev->txreq_free))) {
		struct list_head *l = dev->txreq_free.next;

		list_del(l);
		spin_unlock_irqrestore(&dev->pending_lock, flags);
		tx = list_entry(l, struct qib_verbs_txreq, txreq.list);
	} else {
		/* call slow path to get the extra lock */
		spin_unlock_irqrestore(&dev->pending_lock, flags);
		tx =  __get_txreq(dev, qp);
	}
	return tx;
}

void qib_put_txreq(struct qib_verbs_txreq *tx)
{
	struct qib_ibdev *dev;
	struct qib_qp *qp;
	unsigned long flags;

	qp = tx->qp;
	dev = to_idev(qp->ibqp.device);

	if (atomic_dec_and_test(&qp->refcount))
		wake_up(&qp->wait);
	if (tx->mr) {
		qib_put_mr(tx->mr);
		tx->mr = NULL;
	}
	if (tx->txreq.flags & QIB_SDMA_TXREQ_F_FREEBUF) {
		tx->txreq.flags &= ~QIB_SDMA_TXREQ_F_FREEBUF;
		dma_unmap_single(&dd_from_dev(dev)->pcidev->dev,
				 tx->txreq.addr, tx->hdr_dwords << 2,
				 DMA_TO_DEVICE);
		kfree(tx->align_buf);
	}

	spin_lock_irqsave(&dev->pending_lock, flags);

	/* Put struct back on free list */
	list_add(&tx->txreq.list, &dev->txreq_free);

	if (!list_empty(&dev->txwait)) {
		/* Wake up first QP wanting a free struct */
		qp = list_entry(dev->txwait.next, struct qib_qp, iowait);
		list_del_init(&qp->iowait);
		atomic_inc(&qp->refcount);
		spin_unlock_irqrestore(&dev->pending_lock, flags);

		spin_lock_irqsave(&qp->s_lock, flags);
		if (qp->s_flags & QIB_S_WAIT_TX) {
			qp->s_flags &= ~QIB_S_WAIT_TX;
			qib_schedule_send(qp);
		}
		spin_unlock_irqrestore(&qp->s_lock, flags);

		if (atomic_dec_and_test(&qp->refcount))
			wake_up(&qp->wait);
	} else
		spin_unlock_irqrestore(&dev->pending_lock, flags);
}

/*
 * This is called when there are send DMA descriptors that might be
 * available.
 *
 * This is called with ppd->sdma_lock held.
 */
void qib_verbs_sdma_desc_avail(struct qib_pportdata *ppd, unsigned avail)
{
	struct qib_qp *qp, *nqp;
	struct qib_qp *qps[20];
	struct qib_ibdev *dev;
	unsigned i, n;

	n = 0;
	dev = &ppd->dd->verbs_dev;
	spin_lock(&dev->pending_lock);

	/* Search wait list for first QP wanting DMA descriptors. */
	list_for_each_entry_safe(qp, nqp, &dev->dmawait, iowait) {
		if (qp->port_num != ppd->port)
			continue;
		if (n == ARRAY_SIZE(qps))
			break;
		if (qp->s_tx->txreq.sg_count > avail)
			break;
		avail -= qp->s_tx->txreq.sg_count;
		list_del_init(&qp->iowait);
		atomic_inc(&qp->refcount);
		qps[n++] = qp;
	}

	spin_unlock(&dev->pending_lock);

	for (i = 0; i < n; i++) {
		qp = qps[i];
		spin_lock(&qp->s_lock);
		if (qp->s_flags & QIB_S_WAIT_DMA_DESC) {
			qp->s_flags &= ~QIB_S_WAIT_DMA_DESC;
			qib_schedule_send(qp);
		}
		spin_unlock(&qp->s_lock);
		if (atomic_dec_and_test(&qp->refcount))
			wake_up(&qp->wait);
	}
}

/*
 * This is called with ppd->sdma_lock held.
 */
static void sdma_complete(struct qib_sdma_txreq *cookie, int status)
{
	struct qib_verbs_txreq *tx =
		container_of(cookie, struct qib_verbs_txreq, txreq);
	struct qib_qp *qp = tx->qp;

	spin_lock(&qp->s_lock);
	if (tx->wqe)
		qib_send_complete(qp, tx->wqe, IB_WC_SUCCESS);
	else if (qp->ibqp.qp_type == IB_QPT_RC) {
		struct qib_ib_header *hdr;

		if (tx->txreq.flags & QIB_SDMA_TXREQ_F_FREEBUF)
			hdr = &tx->align_buf->hdr;
		else {
			struct qib_ibdev *dev = to_idev(qp->ibqp.device);

			hdr = &dev->pio_hdrs[tx->hdr_inx].hdr;
		}
		qib_rc_send_complete(qp, hdr);
	}
	if (atomic_dec_and_test(&qp->s_dma_busy)) {
		if (qp->state == IB_QPS_RESET)
			wake_up(&qp->wait_dma);
		else if (qp->s_flags & QIB_S_WAIT_DMA) {
			qp->s_flags &= ~QIB_S_WAIT_DMA;
			qib_schedule_send(qp);
		}
	}
	spin_unlock(&qp->s_lock);

	qib_put_txreq(tx);
}

static int wait_kmem(struct qib_ibdev *dev, struct qib_qp *qp)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&qp->s_lock, flags);
	if (ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK) {
		spin_lock(&dev->pending_lock);
		if (list_empty(&qp->iowait)) {
			if (list_empty(&dev->memwait))
				mod_timer(&dev->mem_timer, jiffies + 1);
			qp->s_flags |= QIB_S_WAIT_KMEM;
			list_add_tail(&qp->iowait, &dev->memwait);
		}
		spin_unlock(&dev->pending_lock);
		qp->s_flags &= ~QIB_S_BUSY;
		ret = -EBUSY;
	}
	spin_unlock_irqrestore(&qp->s_lock, flags);

	return ret;
}

static int qib_verbs_send_dma(struct qib_qp *qp, struct qib_ib_header *hdr,
			      u32 hdrwords, struct qib_sge_state *ss, u32 len,
			      u32 plen, u32 dwords)
{
	struct qib_ibdev *dev = to_idev(qp->ibqp.device);
	struct qib_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(qp->ibqp.device, qp->port_num);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_txreq *tx;
	struct qib_pio_header *phdr;
	u32 control;
	u32 ndesc;
	int ret;

	tx = qp->s_tx;
	if (tx) {
		qp->s_tx = NULL;
		/* resend previously constructed packet */
		ret = qib_sdma_verbs_send(ppd, tx->ss, tx->dwords, tx);
		goto bail;
	}

	tx = get_txreq(dev, qp);
	if (IS_ERR(tx))
		goto bail_tx;

	control = dd->f_setpbc_control(ppd, plen, qp->s_srate,
				       be16_to_cpu(hdr->lrh[0]) >> 12);
	tx->qp = qp;
	atomic_inc(&qp->refcount);
	tx->wqe = qp->s_wqe;
	tx->mr = qp->s_rdma_mr;
	if (qp->s_rdma_mr)
		qp->s_rdma_mr = NULL;
	tx->txreq.callback = sdma_complete;
	if (dd->flags & QIB_HAS_SDMA_TIMEOUT)
		tx->txreq.flags = QIB_SDMA_TXREQ_F_HEADTOHOST;
	else
		tx->txreq.flags = QIB_SDMA_TXREQ_F_INTREQ;
	if (plen + 1 > dd->piosize2kmax_dwords)
		tx->txreq.flags |= QIB_SDMA_TXREQ_F_USELARGEBUF;

	if (len) {
		/*
		 * Don't try to DMA if it takes more descriptors than
		 * the queue holds.
		 */
		ndesc = qib_count_sge(ss, len);
		if (ndesc >= ppd->sdma_descq_cnt)
			ndesc = 0;
	} else
		ndesc = 1;
	if (ndesc) {
		phdr = &dev->pio_hdrs[tx->hdr_inx];
		phdr->pbc[0] = cpu_to_le32(plen);
		phdr->pbc[1] = cpu_to_le32(control);
		memcpy(&phdr->hdr, hdr, hdrwords << 2);
		tx->txreq.flags |= QIB_SDMA_TXREQ_F_FREEDESC;
		tx->txreq.sg_count = ndesc;
		tx->txreq.addr = dev->pio_hdrs_phys +
			tx->hdr_inx * sizeof(struct qib_pio_header);
		tx->hdr_dwords = hdrwords + 2; /* add PBC length */
		ret = qib_sdma_verbs_send(ppd, ss, dwords, tx);
		goto bail;
	}

	/* Allocate a buffer and copy the header and payload to it. */
	tx->hdr_dwords = plen + 1;
	phdr = kmalloc(tx->hdr_dwords << 2, GFP_ATOMIC);
	if (!phdr)
		got