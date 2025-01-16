(&wqe->wr, wr, sizeof(wqe->wr));

	wqe->length = 0;
	j = 0;
	if (wr->num_sge) {
		acc = wr->opcode >= IB_WR_RDMA_READ ?
			IB_ACCESS_LOCAL_WRITE : 0;
		for (i = 0; i < wr->num_sge; i++) {
			u32 length = wr->sg_list[i].length;
			int ok;

			if (length == 0)
				continue;
			ok = qib_lkey_ok(rkt, pd, &wqe->sg_list[j],
					 &wr->sg_list[i], acc);
			if (!ok)
				goto bail_inval_free;
			wqe->length += length;
			j++;
		}
		wqe->wr.num_sge = j;
	}
	if (qp->ibqp.qp_type == IB_QPT_UC ||
	    qp->ibqp.qp_type == IB_QPT_RC) {
		if (wqe->length > 0x80000000U)
			goto bail_inval_free;
	} else if (wqe->length > (dd_from_ibdev(qp->ibqp.device)->pport +
				  qp->port_num - 1)->ibmtu)
		goto bail_inval_free;
	else
		atomic_inc(&to_iah(ud_wr(wr)->ah)->refcount);
	wqe->ssn = qp->s_ssn++;
	qp->s_head = next;

	ret = 0;
	goto bail;

bail_inval_free:
	while (j) {
		struct qib_sge *sge = &wqe->sg_list[--j];

		qib_put_mr(sge->mr);
	}
bail_inval:
	ret = -EINVAL;
bail:
	if (!ret && !wr->next &&
	 !qib_sdma_empty(
	   dd_from_ibdev(qp->ibqp.device)->pport + qp->port_num - 1)) {
		qib_schedule_send(qp);
		*scheduled = 1;
	}
	spin_unlock_irqrestore(&qp->s_lock, flags);
	return ret;
}

/**
 * qib_post_send - post a send on a QP
 * @ibqp: the QP to post the send on
 * @wr: the list of work requests to post
 * @bad_wr: the first bad WR is put here
 *
 * This may be called from interrupt context.
 */
static int qib_post_send(struct ib_qp *ibqp, struct ib_send_wr *wr,
			 struct ib_send_wr **bad_wr)
{
	struct qib_qp *qp = to_iqp(ibqp);
	int err = 0;
	int scheduled = 0;

	for (; wr; wr = wr->next) {
		err = qib_post_one_send(qp, wr, &scheduled);
		if (err) {
			*bad_wr = wr;
			goto bail;
		}
	}

	/* Try to do the send work in the caller's context. */
	if (!scheduled)
		qib_do_send(&qp->s_work);

bail:
	return err;
}

/**
 * qib_post_receive - post a receive on a QP
 * @ibqp: the QP to post the receive on
 * @wr: the WR to post
 * @bad_wr: the first bad WR is put here
 *
 * This may be called from interrupt context.
 */
static int qib_post_receive(struct ib_qp *ibqp, struct ib_recv_wr *wr,
			    struct ib_recv_wr **bad_wr)
{
	struct qib_qp *qp = to_iqp(ibqp);
	struct qib_rwq *wq = qp->r_rq.wq;
	unsigned long flags;
	int ret;

	/* Check that state is OK to post receive. */
	if (!(ib_qib_state_ops[qp->state] & QIB_POST_RECV_OK) || !wq) {
		*bad_wr = wr;
		ret = -EINVAL;
		goto bail;
	}

	for (; wr; wr = wr->next) {
		struct qib_rwqe *wqe;
		u32 next;
		int i;

		if ((unsigned) wr->num_sge > qp->r_rq.max_sge) {
			*bad_wr = wr;
			ret = -EINVAL;
			goto bail;
		}

		spin_lock_irqsave(&qp->r_rq.lock, flags);
		next = wq->head + 1;
		if (next >= qp->r_rq.size)
			next = 0;
		if (next == wq->tail) {
			spin_unlock_irqrestore(&qp->r_rq.lock, flags);
			*bad_wr = wr;
			ret = -ENOMEM;
			goto bail;
		}

		wqe = get_rwqe_ptr(&qp->r_rq, wq->head);
		wqe->wr_id = wr->wr_id;
		wqe->num_sge = wr->num_sge;
		for (i = 0; i < wr->num_sge; i++)
			wqe->sg_list[i] = wr->sg_list[i];
		/* Make sure queue entry is written before the head index. */
		smp_wmb();
		wq->head = next;
		spin_unlock_irqrestore(&qp->r_rq.lock, flags);
	}
	ret = 0;

bail:
	return ret;
}

/**
 * qib_qp_rcv - processing an incoming packet on a QP
 * @rcd: the context pointer
 * @hdr: the packet header
 * @has_grh: true if the packet has a GRH
 * @data: the packet data
 * @tlen: the packet length
 * @qp: the QP the packet came on
 *
 * This is called from qib_ib_rcv() to process an incoming packet
 * for the given QP.
 * Called at interrupt level.
 */
static void qib_qp_rcv(struct qib_ctxtdata *rcd, struct qib_ib_header *hdr,
		       int has_grh, void *data, u32 tlen, struct qib_qp *qp)
{
	struct qib_ibport *ibp = &rcd->ppd->ibport_data;

	spin_lock(&qp->r_lock);

	/* Check for valid receive state. */
	if (!(ib_qib_state_ops[qp->state] & QIB_PROCESS_RECV_OK)) {
		ibp->n_pkt_drops++;
		goto unlock;
	}

	switch (qp->ibqp.qp_type) {
	case IB_QPT_SMI:
	case IB_QPT_GSI:
		if (ib_qib_disable_sma)
			break;
		/* FALLTHROUGH */
	case IB_QPT_UD:
		qib_ud_rcv(ibp, hdr, has_grh, data, tlen, qp);
		break;

	case IB_QPT_RC:
		qib_rc_rcv(rcd, hdr, has_grh, data, tlen, qp);
		break;

	case IB_QPT_UC:
		qib_uc_rcv(ibp, hdr, has_grh, data, tlen, qp);
		break;

	default:
		break;
	}

unlock:
	spin_unlock(&qp->r_lock);
}

/**
 * qib_ib_rcv - process an incoming packet
 * @rcd: the context pointer
 * @rhdr: the header of the packet
 * @data: the packet payload
 * @tlen: the packet length
 *
 * This is called from qib_kreceive() to process an incoming packet at
 * interrupt level. Tlen is the length of the header + data + CRC in bytes.
 */
void qib_ib_rcv(struct qib_ctxtdata *rcd, void *rhdr, void *data, u32 tlen)
{
	struct qib_pportdata *ppd = rcd->ppd;
	struct qib_ibport *ibp = &ppd->ibport_data;
	struct qib_ib_header *hdr = rhdr;
	struct qib_other_headers *ohdr;
	struct qib_qp *qp;
	u32 qp_num;
	int lnh;
	u8 opcode;
	u16 lid;

	/* 24 == LRH+BTH+CRC */
	if (unlikely(tlen < 24))
		goto drop;

	/* Check for a valid destination LID (see ch. 7.11.1). */
	lid = be16_to_cpu(hdr->lrh[1]);
	if (lid < QIB_MULTICAST_LID_BASE) {
		lid &= ~((1 << ppd->lmc) - 1);
		if (unlikely(lid != ppd->lid))
			goto drop;
	}

	/* Check for GRH */
	lnh = be16_to_cpu(hdr->lrh[0]) & 3;
	if (lnh == QIB_LRH_BTH)
		ohdr = &hdr->u.oth;
	else if (lnh == QIB_LRH_GRH) {
		u32 vtf;

		ohdr = &hdr->u.l.oth;
		if (hdr->u.l.grh.next_hdr != IB_GRH_NEXT_HDR)
			goto drop;
		vtf = be32_to_cpu(hdr->u.l.grh.version_tclass_flow);
		if ((vtf >> IB_GRH_VERSION_SHIFT) != IB_GRH_VERSION)
			goto drop;
	} else
		goto drop;

	opcode = (be32_to_cpu(ohdr->bth[0]) >> 24) & 0x7f;
#ifdef CONFIG_DEBUG_FS
	rcd->opstats->stats[opcode].n_bytes += tlen;
	rcd->opstats->stats[opcode].n_packets++;
#endif

	/* Get the destination QP number. */
	qp_num = be32_to_cpu(ohdr->bth[1]) & QIB_QPN_MASK;
	if (qp_num == QIB_MULTICAST_QPN) {
		struct qib_mcast *mcast;
		struct qib_mcast_qp *p;

		if (lnh != QIB_LRH_GRH)
			goto drop;
		mcast = qib_mcast_find(ibp, &hdr->u.l.grh.dgid);
		if (mcast == NULL)
			goto drop;
		this_cpu_inc(ibp->pmastats->n_multicast_rcv);
		list_for_each_entry_rcu(p, &mcast->qp_list, list)
			qib_qp_rcv(rcd, hdr, 1, data, tlen, p->qp);
		/*
		 * Notify qib_multicast_detach() if it is waiting for us
		 * to finish.
		 */
		if (atomic_dec_return(&mcast->refcount) <= 1)
			wake_up(&mcast->wait);
	} else {
		if (rcd->lookaside_qp) {
			if (rcd->lookaside_qpn != qp_num) {
				if (atomic_dec_and_test(
					&rcd->lookaside_qp->refcount))
					wake_up(
					 &rcd->lookaside_qp->wait);
				rcd->lookaside_qp = NULL;
			}
		}
		if (!rcd->lookaside_qp) {
			qp = qib_lookup_qpn(ibp, qp_num);
			if (!qp)
				goto drop;
			rcd->lookaside_qp = qp;
			rcd->lookaside_qpn = qp_num;
		} else
			qp = rcd->lookaside_qp;
		this_cpu_inc(ibp->pmastats->n_unicast_rcv);
		qib_qp_rcv(rcd, hdr, lnh == QIB_LRH_GRH, data, tlen, qp);
	}
	return;

drop:
	ibp->n_pkt_drops++;
}

/*
 * This is called from a timer to check for QPs
 * which need kernel memory in order to send a packet.
 */
static void mem_timer(unsigned long data)
{
	struct qib_ibdev *dev = (struct qib_ibdev *) data;
	struct list_head *list = &dev->memwait;
	struct qib_qp *qp = NULL;
	unsigned long flags;

	spin_lock_irqsave(&dev->pending_lock, flags);
	if (!list_empty(list)) {
		qp = list_entry(list->next, struct qib_qp, iowait);
		list_del_init(&qp->iowait);
		atomic_inc(&qp->refcount);
		if (!list_empty(list))
			mod_timer(&dev->mem_timer, jiffies + 1);
	}
	spin_unlock_irqrestore(&dev->pending_lock, flags);

	if (qp) {
		spin_lock_irqsave(&qp->s_lock, flags);
		if (qp->s_flags & QIB_S_WAIT_KMEM) {
			qp->s_flags &= ~QIB_S_WAIT_KMEM;
			qib_schedule_send(qp);
		}
		spin_unlock_irqrestore(&qp->s_lock, flags);
		if (atomic_dec_and_test(&qp->refcount))
			wake_up(&qp->wait);
	}
}

static void update_sge(struct qib_sge_state *ss, u32 length)
{
	struct qib_sge *sge = &ss->sge;

	sge->vaddr += length;
	sge->length -= length;
	sge->sge_length -= length;
	if (sge->sge_length == 0) {
		if (--ss->num_sge)
			*sge = *ss->sg_list++;
	} else if (sge->length == 0 && sge->mr->lkey) {
		if (++sge->n >= QIB_SEGSZ) {
			if (++sge->m >= sge->mr->mapsz)
				return;
			sge->n = 0;
		}
		sge->vaddr = sge->mr->map[sge->m]->segs[sge->n].vaddr;
		sge->length = sge->mr->map[sge->m]->segs[sge->n].length;
	}
}

#ifdef __LITTLE_ENDIAN
static inline u32 get_upper_bits(u32 data, u32 shift)
{
	return data >> shift;
}

static inline u32 set_upper_bits(u32 data, u32 shift)
{
	return data << shift;
}

static inline u32 clear_upper_bytes(u32 data, u32 n, u32 off)
{
	data <<= ((sizeof(u32) - n) * BITS_PER_BYTE);
	data >>= ((sizeof(u32) - n - off) * BITS_PER_BYTE);
	return data;
}
#else
static inline u32 get_upper_bits(u32 data, u32 shift)
{
	return data << shift;
}

static inline u32 set_upper_bits(u32 data, u32 shift)
{
	return data >> shift;
}

static inline u32 clear_upper_bytes(u32 data, u32 n, u32 off)
{
	data >>= ((sizeof(u32) - n) * BITS_PER_BYTE);
	data <<= ((sizeof(u32) - n - off) * BITS_PER_BYTE);
	return data;
}
#endif

static void copy_io(u32 __iomem *piobuf, struct qib_sge_state *ss,
		    u32 length, unsigned flush_wc)
{
	u32 extra = 0;
	u32 data = 0;
	u32 last;

	while (1) {
		u32 len = ss->sge.length;
		u32 off;

		if (len > length)
			len = length;
		if (len > ss->sge.sge_length)
			len = ss->sge.sge_length;
		BUG_ON(len == 0);
		/* If the source address is not aligned, try to align it. */
		off = (unsigned long)ss->sge.vaddr & (sizeof(u32) - 1);
		if (off) {
			u32 *addr = (u32 *)((unsigned long)ss->sge.vaddr &
					    ~(sizeof(u32) - 1));
			u32 v = get_upper_bits(*addr, off * BITS_PER_BYTE);
			u32 y;

			y = sizeof(u32) - off;
			if (len > y)
				len = y;
			if (len + extra >= sizeof(u32)) {
				data |= set_upper_bits(v, extra *
						       BITS_PER_BYTE);
				len = sizeof(u32) - extra;
				if (len == length) {
					last = data;
					break;
				}
				__raw_writel(data, piobuf);
				piobuf++;
				extra = 0;
				data = 0;
			} else {
				/* Clear unused upper bytes */
				data |= clear_upper_bytes(v, len, extra);
				if (len == length) {
					last = data;
					break;
				}
				extra += len;
			}
		} else if (extra) {
			/* Source address is aligned. */
			u32 *addr = (u32 *) ss->sge.vaddr;
			int shift = extra * BITS_PER_BYTE;
			int ushift = 32 - shift;
			u32 l = len;

			while (l >= sizeof(u32)) {
				u32 v = *addr;

				data |= set_upper_bits(v, shift);
				__raw_writel(data, piobuf);
				data = get_upper_bits(v, ushift);
				piobuf++;
				addr++;
				l -= sizeof(u32);
			}
			/*
			 * We still have 'extra' number of bytes leftover.
			 */
			if (l) {
				u32 v = *addr;

				if (l + extra >= sizeof(u32)) {
					data |= set_upper_bits(v, shift);
					len -= l + extra - sizeof(u32);
					if (len == length) {
						last = data;
						break;
					}
					__raw_writel(data, piobuf);
					piobuf++;
					extra = 0;
					data = 0;
				} else {
					/* Clear unused upper bytes */
					data |= clear_upper_bytes(v, l, extra);
					if (len == length) {
						last = data;
						break;
					}
					extra += l;
				}
			} else if (len == length) {
				last = data;
				break;
			}
		} else if (len == length) {
			u32 w;

			/*
			 * Need to round up for the last dword in the
			 * packet.
			 */
			w = (len + 3) >> 2;
			qib_pio_copy(piobuf, ss->sge.vaddr, w - 1);
			piobuf += w - 1;
			last = ((u32 *) ss->sge.vaddr)[w - 1];
			break;
		} else {
			u32 w = len >> 2;

			qib_pio_copy(piobuf, ss->sge.vaddr, w);
			piobuf += w;