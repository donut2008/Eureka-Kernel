_query_srq(struct ib_srq *srq,
		 struct ib_srq_attr *srq_attr);

/**
 * ib_destroy_srq - Destroys the specified SRQ.
 * @srq: The SRQ to destroy.
 */
int ib_destroy_srq(struct ib_srq *srq);

/**
 * ib_post_srq_recv - Posts a list of work requests to the specified SRQ.
 * @srq: The SRQ to post the work request on.
 * @recv_wr: A list of work requests to post on the receive queue.
 * @bad_recv_wr: On an immediate failure, this parameter will reference
 *   the work request that failed to be posted on the QP.
 */
static inline int ib_post_srq_recv(struct ib_srq *srq,
				   struct ib_recv_wr *recv_wr,
				   struct ib_recv_wr **bad_recv_wr)
{
	return srq->device->post_srq_recv(srq, recv_wr, bad_recv_wr);
}

/**
 * ib_create_qp - Creates a QP associated with the specified protection
 *   domain.
 * @pd: The protection domain associated with the QP.
 * @qp_init_attr: A list of initial attributes required to create the
 *   QP.  If QP creation succeeds, then the attributes are updated to
 *   the actual capabilities of the created QP.
 */
struct ib_qp *ib_create_qp(struct ib_pd *pd,
			   struct ib_qp_init_attr *qp_init_attr);

/**
 * ib_modify_qp - Modifies the attributes for the specified QP and then
 *   transitions the QP to the given state.
 * @qp: The QP to modify.
 * @qp_attr: On input, specifies the QP attributes to modify.  On output,
 *   the current values of selected QP attributes are returned.
 * @qp_attr_mask: A bit-mask used to specify which attributes of the QP
 *   are being modified.
 */
int ib_modify_qp(struct ib_qp *qp,
		 struct ib_qp_attr *qp_attr,
		 int qp_attr_mask);

/**
 * ib_query_qp - Returns the attribute list and current values for the
 *   specified QP.
 * @qp: The QP to query.
 * @qp_attr: The attributes of the specified QP.
 * @qp_attr_mask: A bit-mask used to select specific attributes to query.
 * @qp_init_attr: Additional attributes of the selected QP.
 *
 * The qp_attr_mask may be used to limit the query to gathering only the
 * selected attributes.
 */
int ib_query_qp(struct ib_qp *qp,
		struct ib_qp_attr *qp_attr,
		int qp_attr_mask,
		struct ib_qp_init_attr *qp_init_attr);

/**
 * ib_destroy_qp - Destroys the specified QP.
 * @qp: The QP to destroy.
 */
int ib_destroy_qp(struct ib_qp *qp);

/**
 * ib_open_qp - Obtain a reference to an existing sharable QP.
 * @xrcd - XRC domain
 * @qp_open_attr: Attributes identifying the QP to open.
 *
 * Returns a reference to a sharable QP.
 */
struct ib_qp *ib_open_qp(struct ib_xrcd *xrcd,
			 struct ib_qp_open_attr *qp_open_attr);

/**
 * ib_close_qp - Release an external reference to a QP.
 * @qp: The QP handle to release
 *
 * The opened QP handle is released by the caller.  The underlying
 * shared QP is not destroyed until all internal references are released.
 */
int ib_close_qp(struct ib_qp *qp);

/**
 * ib_post_send - Posts a list of work requests to the send queue of
 *   the specified QP.
 * @qp: The QP to post the work request on.
 * @send_wr: A list of work requests to post on the send queue.
 * @bad_send_wr: On an immediate failure, this parameter will reference
 *   the work request that failed to be posted on the QP.
 *
 * While IBA Vol. 1 section 11.4.1.1 specifies that if an immediate
 * error is returned, the QP state shall not be affected,
 * ib_post_send() will return an immediate error after queueing any
 * earlier work requests in the list.
 */
static inline int ib_post_send(struct ib_qp *qp,
			       struct ib_send_wr *send_wr,
			       struct ib_send_wr **bad_send_wr)
{
	return qp->device->post_send(qp, send_wr, bad_send_wr);
}

/**
 * ib_post_recv - Pos