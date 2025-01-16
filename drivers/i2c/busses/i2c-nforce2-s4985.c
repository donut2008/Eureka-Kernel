_sges = 128;
module_param_named(max_srq_sges, ib_qib_max_srq_sges, uint, S_IRUGO);
MODULE_PARM_DESC(max_srq_sges, "Maximum number of SRQ SGEs to support");

unsigned int ib_qib_max_srq_wrs = 0x1FFFF;
module_param_named(max_srq_wrs, ib_qib_max_srq_wrs, uint, S_IRUGO);
MODULE_PARM_DESC(max_srq_wrs, "Maximum number of SRQ WRs support");

static unsigned int ib_qib_disable_sma;
module_param_named(disable_sma, ib_qib_disable_sma, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(disable_sma, "Disable the SMA");

/*
 * Note that it is OK to post send work requests in the SQE and ERR
 * states; qib_do_send() will process them and generate error
 * completions as per IB 1.2 C10-96.
 */
const int ib_qib_state_ops[IB_QPS_ERR + 1] = {
	[IB_QPS_RESET] = 0,
	[IB_QPS_INIT] = QIB_POST_RECV_OK,
	[IB_QPS_RTR] = QIB_POST_RECV_OK | QIB_PROCESS_RECV_OK,
	[IB_QPS_RTS] = QIB_POST_RECV_OK | QIB_PROCESS_RECV_OK |
	    QIB_POST_SEND_OK | QIB_PROCESS_SEND_OK |
	    QIB_PROCESS_NEXT_SEND_OK,
	[IB_QPS_SQD] = QIB_POST_RECV_OK | QIB_PROCESS_RECV_OK |
	    QIB_POST_SEND_OK | QIB_PROCESS_SEND_OK,
	[IB_QPS_SQE] = QIB_POST_RECV_OK | QIB_PROCESS_RECV_OK |
	    QIB_POST_SEND_OK | QIB_FLUSH_SEND,
	[IB_QPS_ERR] = QIB_POST_RECV_OK | QIB_FLUSH_RECV |
	    QIB_POST_SEND_OK | QIB_FLUSH_SEND,
};

struct qib_ucontext {
	struct ib_ucontext ibucontext;
};

static inline struct qib_ucontext *to_iucontext(struct ib_ucontext
						  *ibucontext)
{
	return container_of(ibucontext, struct qib_ucontext, ibucontext);
}

/*
 * Translate ib_wr_opcode into ib_wc_opcode.
 */
const enum ib_wc_opcode ib_qib_wc_opcode[] = {
	[IB_WR_RDMA_WRITE] = IB_WC_RDMA_WRITE,
	[IB_WR_RDMA_WRITE_WITH_IMM] = IB_WC_RDMA_WRITE,
	[IB_WR_SEND] = IB_WC_SEND,
	[IB_WR_SEND_WITH_IMM] = IB_WC_SEND,
	[IB_WR_RDMA_READ] = IB_WC_RDMA_READ,
	[IB_WR_ATOMIC_CMP_AND_SWP] = IB_WC_COMP_SWAP,
	[IB_WR_ATOMIC_FETCH_AND_ADD] = IB_WC_FETCH_ADD
};

/*
 * System image GUID.
 */
__be64 ib_qib_sys_image_guid;

/**
 * qib_copy_sge - copy data to SGE memory
 * @ss: the SGE state
 * @data: the data to copy
 * @length: the length of the data
 */
void qib_copy_sge(struct qib_sge_state *ss, void *data, u32 length, int release)
{
	struct qib_sge *sge = &ss->sge;

	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		memcpy(sge->vaddr, data, len);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (release)
				qib_put_mr(sge->mr);
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		data += len;
		length -= len;
	}
}

/**
 * qib_skip_sge - skip over SGE memory - XXX almost dup of prev func
 * @ss: the SGE state
 * @length: the number of bytes to skip
 */
void qib_skip_sge(struct qib_sge_state *ss, u32 length, int release)
{
	struct qib_sge *sge = &ss->sge;

	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (release)
				qib_put_mr(sge->mr);
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		length -= len;
	}
}

/*
 * Count the number of DMA descriptors needed to send length bytes of data.
 * Don't modify the qib_sge_state to get the count.
 * Return zero if any of the segments is not aligned.
 */
static u32 qib_count_sge(struct qib_sge_state *ss, u32 length)
{
	struct qib_sge *sg_list = ss->sg_list;
	struct qib_sge sge = ss->sge;
	u8 num_sge = ss->num_sge;
	u32 ndesc = 1;  /* count the header */

	while (length) {
		u32 len = sge.length;

		if (len > length)
			len = length;
		if (len > sge.sge_length)
			len = sge.sge_length;
		BUG_ON(len == 0);
		if (((long) sge.vaddr & (sizeof(u32) - 1)) ||
		    (len != length && (len & (sizeof(u32) - 1)))) {
			ndesc = 0;
			break;
		}
		ndesc++;
		sge.vaddr += len;
		sge.length -= len;
		sge.sge_length -= len;
		if (sge.sge_length == 0) {
			if (--num_sge)
				sge = *sg_list++;
		} else if (sge.length == 0 && sge.mr->lkey) {
			if (++sge.n >= QIB_SEGSZ) {
				if (++sge.m >= sge.mr->mapsz)
					break;
				sge.n = 0;
			}
			sge.vaddr =
				sge.mr->map[sge.m]->segs[sge.n].vaddr;
			sge.length =
				sge.mr->map[sge.m]->segs[sge.n].length;
		}
		length -= len;
	}
	return ndesc;
}

/*
 * Copy from the SGEs to the data buffer.
 */
static void qib_copy_from_sge(void *data, struct qib_sge_state *ss, u32 length)
{
	struct qib_sge *sge = &ss->sge;

	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		memcpy(data, sge->vaddr, len);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		data += len;
		length -= len;
	}
}

/**
 * qib_post_one_send - post one RC, UC, or UD send work request
 * @qp: the QP to post on
 * @wr: the work request to send
 */
static int qib_post_one_send(struct qib_qp *qp, struct ib_send_wr *wr,
	int *scheduled)
{
	struct qib_swqe *wqe;
	u32 next;
	int i;
	int j;
	int acc;
	int ret;
	unsigned long flags;
	struct qib_lkey_table *rkt;
	struct qib_pd *pd;

	spin_lock_irqsave(&qp->s_lock, flags);

	/* Check that state is OK to post send. */
	if (unlikely(!(ib_qib_state_ops[qp->state] & QIB_POST_SEND_OK)))
		goto bail_inval;

	/* IB spec says that num_sge == 0 is OK. */
	if (wr->num_sge > qp->s_max_sge)
		goto bail_inval;

	/*
	 * Don't allow RDMA reads or atomic operations on UC or
	 * undefined operations.
	 * Make sure buffer is large enough to hold the result for atomics.
	 */
	if (wr->opcode == IB_WR_REG_MR) {
		if (qib_reg_mr(qp, reg_wr(wr)))
			goto bail_inval;
	} else if (qp->ibqp.qp_type == IB_QPT_UC) {
		if ((unsigned) wr->opcode >= IB_WR_RDMA_READ)
			goto bail_inval;
	} else if (qp->ibqp.qp_type != IB_QPT_RC) {
		/* Check IB_QPT_SMI, IB_QPT_GSI, IB_QPT_UD opcode */
		if (wr->opcode != IB_WR_SEND &&
		    wr->opcode != IB_WR_SEND_WITH_IMM)
			goto bail_inval;
		/* Check UD destination address PD */
		if (qp->ibqp.pd != ud_wr(wr)->ah->pd)
			goto bail_inval;
	} else if ((unsigned) wr->opcode > IB_WR_ATOMIC_FETCH_AND_ADD)
		goto bail_inval;
	else if (wr->opcode >= IB_WR_ATOMIC_CMP_AND_SWP &&
		   (wr