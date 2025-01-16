                (*dealloc_pd)(struct ib_pd *pd);
	struct ib_ah *             (*create_ah)(struct ib_pd *pd,
						struct ib_ah_attr *ah_attr);
	int                        (*modify_ah)(struct ib_ah *ah,
						struct ib_ah_attr *ah_attr);
	int                        (*query_ah)(struct ib_ah *ah,
					       struct ib_ah_attr *ah_attr);
	int                        (*destroy_ah)(struct ib_ah *ah);
	struct ib_srq *            (*create_srq)(struct ib_pd *pd,
						 struct ib_srq_init_attr *srq_init_attr,
						 struct ib_udata *udata);
	int                        (*modify_srq)(struct ib_srq *srq,
						 struct ib_srq_attr *srq_attr,
						 enum ib_srq_attr_mask srq_attr_mask,
						 struct ib_udata *udata);
	int                        (*query_srq)(struct ib_srq *srq,
						struct ib_srq_attr *srq_attr);
	int                        (*destroy_srq)(struct ib_srq *srq);
	int                        (*post_srq_recv)(struct ib_srq *srq,
						    struct ib_recv_wr *recv_wr,
						    struct ib_recv_wr **bad_recv_wr);
	struct ib_qp *             (*create_qp)(struct ib_pd *pd,
						struct ib_qp_init_attr *qp_init_attr,
						struct ib_udata *udata);
	int                        (*modify_qp)(struct ib_qp *qp,
						struct ib_qp_attr *qp_attr,
						int qp_attr_mask,
						struct ib_udata *udata);
	int                        (*query_qp)(struct ib_qp *qp,
					       struct ib_qp_attr *qp_attr,
					       int qp_attr_mask,
					       struct ib_qp_init_attr *qp_init_attr);
	int                        (*destroy_qp)(struct ib_qp *qp);
	int                        (*post_send)(struct ib_qp *qp,
						struct ib_send_wr *send_wr,
						struct ib_send_wr **bad_send_wr);
	int                        (*post_recv)(struct ib_qp *qp,
						struct ib_recv_wr *recv_wr,
						struct ib_recv_wr **bad_recv_wr);
	struct ib_cq *             (*create_cq)(struct ib_device *device,
						const struct ib_cq_init_attr *attr,
						struct ib_ucontext *context,
						struct ib_udata *udata);
	int                        (*modify_cq)(struct ib_cq *cq, u16 cq_count,
						u16 cq_period);
	int                        (*destroy_cq)(struct ib_cq *cq);
	int                        (*resize_cq)(struct ib_cq *cq, int cqe,
						struct ib_udata *udata);
	int                        (*poll_cq)(struct ib_cq *cq, int num_entries,
					      struct ib_wc *wc);
	int                        (*peek_cq)(struct ib_cq *cq, int wc_cnt);
	int                        (*req_notify_cq)(struct ib_cq *cq,
						    enum ib_cq_notify_flags flags);
	int                        (*req_ncomp_notif)(struct ib_cq *cq,
						      int wc_cnt);
	struct ib_mr *             (*get_dma_mr)(struct ib_pd *pd,
						 int mr_access_flags);
	struct ib_mr *             (*reg_phys_mr)(struct ib_pd *pd,
						  struct ib_phys_buf *phys_buf_array,
						  int num_phys_buf,
						  int mr_access_flags,
						  u64 *iova_start);
	struct ib_mr *             (*reg_user_mr)(struct ib_pd *pd,
						  u64 start, u64 length,
						  u64 virt_addr,
						  int mr_access_flags,
						  struct ib_udata *udata);
	int			   (*rereg_user_mr)(struct ib_mr *mr,
						    int flags,
						    u64 start, u64 length,
						    u64 virt_addr,
						    int mr_access_flags,
						    struct ib_pd *pd,
						    struct ib_udata *udata);
	int                        (*query_mr)(struct ib_mr *mr,
					       struct ib_mr_attr *mr_attr);
	int           