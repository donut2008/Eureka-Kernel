cv(qp, recv_wr, bad_recv_wr);
}

/**
 * ib_create_cq - Creates a CQ on the specified device.
 * @device: The device on which to create the CQ.
 * @comp_handler: A user-specified callback that is invoked when a
 *   completion event occurs on the CQ.
 * @event_handler: A user-specified callback that is invoked when an
 *   asynchronous event not associated with a completion occurs on the CQ.
 * @cq_context: Context associated with the CQ returned to the user via
 *   the associated completion and event handlers.
 * @cq_attr: The attributes the CQ should be created upon.
 *
 * Users can examine the cq structure to determine the actual CQ size.
 */
struct ib_cq *ib_create_cq(struct ib_device *device,
			   ib_comp_handler comp_handler,
			   void (*event_handler)(struct ib_event *, void *),
			   void *cq_context,
			   const struct ib_cq_init_attr *cq_attr);

/**
 * ib_resize_cq - Modifies the capacity of the CQ.
 * @cq: The CQ to resize.
 * @cqe: The minimum size of the CQ.
 *
 * Users can examine the cq structure to determine the actual CQ size.
 */
int ib_resize_cq(struct ib_cq *cq, int cqe);

/**
 * ib_modify_cq - Modifies moderation params of the CQ
 * @cq: The CQ to modify.
 * @cq_count: number of CQEs that will trigger an event
 * @cq_period: max period of time in usec before triggering an event
 *
 */
int ib_modify_cq(struct ib_cq *cq, u16 cq_count, u16 cq_period);

/**
 * ib_destroy_cq - Destroys the specified CQ.
 * @cq: The CQ to destroy.
 */
int ib_destroy_cq(struct ib_cq *cq);

/**
 * ib_poll_cq - poll a CQ for completion(s)
 * @cq:the CQ being polled
 * @num_entries:maximum number of completions to return
 * @wc:array of at least @num_entries &struct ib_wc where completions
 *   will be returned
 *
 * Poll a CQ for (possibly multiple) completions.  If the return value
 * is < 0, an error occurred.  If the return value is >= 0, it is the
 * number of completions returned.  If the return value is
 * non-negative and < num_entries, then the CQ was emptied.
 */
static inline int ib_poll_cq(struct ib_cq *cq, int num_entries,
			     struct ib_wc *wc)
{
	return cq->device->poll_cq(cq, num_entries, wc);
}

/**
 * ib_peek_cq - Returns the number of unreaped completions currently
 *   on the specified CQ.
 * @cq: The CQ to peek.
 * @wc_cnt: A minimum number of unreaped completions to check for.
 *
 * If the number of unreaped completions is greater than or equal to wc_cnt,
 * this function returns wc_cnt, otherwise, it returns the actual number of
 * unreaped completions.
 */
int ib_peek_cq(struct ib_cq *cq, int wc_cnt);

/**
 * ib_req_notify_cq - Request completion notification on a CQ.
 * @cq: The CQ to generate an event for.
 * @flags:
 *   Must contain exactly one of %IB_CQ_SOLICITED or %IB_CQ_NEXT_COMP
 *   to request an event on the next solicited event or next work
 *   completion at any type, respectively. %IB_CQ_REPORT_MISSED_EVENTS
 *   may also be |ed in to request a hint about missed events, as
 *   described below.
 *
 * Return Value:
 *    < 0 means an error occurred while requesting notification
 *   == 0 means notification was requested successfully, and if
 *        IB_CQ_REPORT_MISSED_EVENTS was passed in, then no events
 *        were missed and it is safe to wait for another event.  In
 *        this case is it guaranteed that any work completions added
 *        to the CQ since the last CQ poll will trigger a completion
 *        notification event.
 *    > 0 is only returned if IB_CQ_REPORT_MISSED_EVENTS was passed
 *        in.  It means that the consumer must poll the CQ again to
 *        make sure it is empty to avoid missing an event because of a
 *        race between requesting notification and an entry being
 *        added to the CQ.  This return value means it is possible
 *        (but not guaranteed) that a work completion has been added
 *        to the CQ since the last poll without triggering a
 *        completion notification event.
 */
static inline int ib_req_notify_cq(struct ib_cq *cq,
				   enum ib_cq_notify_flags flags)
{
	return cq->device->req_notify_cq(cq, flags);
}

/**
 * ib_req_ncomp_notif - Request completion notification when there are
 *   at least the specified number of unreaped completions on the CQ.
 * @cq: The CQ to generate an event for.
 * @wc_cnt: The number of unreaped completions that should be on the
 *   CQ before an event is generated.
 */
static inline int ib_req_ncomp_notif(struct ib_cq *cq, int wc_cnt)
{
	return cq->device->req_ncomp_notif ?
		cq->device->req_ncomp_notif(cq, wc_cnt) :
		-ENOSYS;
}

/**
 * ib_get_dma_mr - Returns a memory region for system memory that is
 *   usable for DMA.
 * @pd: The protection domain associated with the memory region.
 * @mr_access_flags: Specifies the memory access rights.
 *
 * Note that the ib_dma_*() functions defined below must be used
 * to create/destroy addresses used with the Lkey or Rkey returned
 * by ib_get_dma_mr().
 */
struct ib_mr *ib_get_dma_mr(struct ib_pd *pd, int mr_access_flags);

/**
 * ib_dma_mapping_error - check a DMA addr for error
 * @dev: The device for which the dma_addr was created
 * @dma_addr: The DMA address to check
 */
static inline int ib_dma_mapping_error(struct ib_device *dev, u64 dma_addr)
{
