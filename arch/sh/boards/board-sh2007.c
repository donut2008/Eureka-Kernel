y regions.
 * @fmr_list: A linked list of fast memory regions to unmap.
 */
int ib_unmap_fmr(struct list_head *fmr_list);

/**
 * ib_dealloc_fmr - Deallocates a fast memory region.
 * @fmr: The fast memory region to deallocate.
 */
int ib_dealloc_fmr(struct ib_fmr *fmr);

/**
 * ib_attach_mcast - Attaches the specified QP to a multicast group.
 * @qp: QP to attach to the multicast group.  The QP must be type
 *   IB_QPT_UD.
 * @gid: Multicast group GID.
 * @lid: Multicast group LID in host byte order.
 *
 * In order to send and receive multicast packets, subnet
 * administration must have created the multicast group and configured
 * the fabric appropriately.  The port associated with the specified
 * QP must also be a member of the multicast group.
 */
int ib_attach_mcast(struct ib_qp *qp, union ib_gid *gid, u16 lid);

/**
 * ib_detach_mcast - Detaches the specified QP from a multicast group.
 * @qp: QP to detach from the multicast group.
 * @gid: Multicast group GID.
 * @lid: Multicast group LID in host byte order.
 */
int ib_detach_mcast(struct ib_qp *qp, union ib_gid *gid, u16 lid);

/**
 * ib_alloc_xrcd - Allocates an XRC domain.
 * @device: The device on which to allocate the XRC domain.
 */
struct ib_xrcd *ib_alloc_xrcd(struct ib_device *device);

/**
 * ib_dealloc_xrcd - Deallocates an XRC domain.
 * @xrcd: The XRC domain to deallocate.
 */
int ib_dealloc_xrcd(struct ib_xrcd *xrcd);

struct ib_flow *ib_create_flow(struct ib_qp *qp,
			       struct ib_flow_attr *flow_attr, int domain);
int ib_destroy_flow(struct ib_flow *flow_id);

static inline int ib_check_mr_access(int flags)
{
	/*
	 * Local write permission is required if remote write or
	 * remote atomic permission is also requested.
	 */
	if (flags & (IB_ACCESS_REMOTE_ATOMIC | IB_ACCESS_REMOTE_WRITE) &&
	    !(flags & IB_ACCESS_LOCAL_WRITE))
		return -EINVAL;

	return 0;
}

static inline bool ib_access_writable(int access_flags)
{
	/*
	 * We have writable memory backing the MR if any of the following
	 * access flags are set.  "Local write" and "remote write" obviously
	 * require write access.  "Remote atomic" can do things like fetch and
	 * add, which will modify memory, and "MW bind" can change permissions
	 * by binding a window.
	 */
	return access_flags &
		(IB_ACCESS_LOCAL_WRITE   | IB_ACCESS_REMOTE_WRITE |
		 IB_ACCESS_REMOTE_ATOMIC | IB_ACCESS_MW_BIND);
}

/**
 * ib_check_mr_status: lightweight check of MR status.
 *     This routine may provide status checks on a selected
 *     ib_mr. first use is for signature status check.
 *
 * @mr: A memory region.
 * @check_mask: Bitmask of which checks to perform from
 *     ib_mr_status_check enumeration.
 * @mr_status: The container of relevant status checks.
 *     failed checks will be indicated in the status bitmask
 *     and the relevant info shall be in the error item.
 */
int ib_check_mr_status(struct ib_mr *mr, u32 check_mask,
		       struct ib_mr_status *mr_status);

struct net_device *ib_get_net_dev_by_params(struct ib_device *dev, u8 port,
					    u16 pkey, const union ib_gid *gid,
					    const struct sockaddr *addr);

int ib_map_mr_sg(struct ib_mr *mr,
		 struct scatterlist *sg,
		 int sg_nents,
		 unsigned int page_size);

static inline int
ib_map_mr_sg_zbva