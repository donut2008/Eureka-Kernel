		      struct ib_mw_bind *mw_bind);
	int                        (*dealloc_mw)(struct ib_mw *mw);
	struct ib_fmr *	           (*alloc_fmr)(struct ib_pd *pd,
						int mr_access_flags,
						struct ib_fmr_attr *fmr_attr);
	int		           (*map_phys_fmr)(struct ib_fmr *fmr,
						   u64 *page_list, int list_len,
						   u64 iova);
	int		           (*unmap_fmr)(struct list_head *fmr_list);
	int		           (*dealloc_fmr)(struct ib_fmr *fmr);
	int                        (*attach_mcast)(struct ib_qp *qp,
						   union ib_gid *gid,
						   u16 lid);
	int                        (*detach_mcast)(struct ib_qp *qp,
						   union ib_gid *gid,
						   u16 lid);
	int                        (*process_mad)(struct ib_device *device,
						  int process_mad_flags,
						  u8 port_num,
						  const struct ib_wc *in_wc,
						  const struct ib_grh *in_grh,
						  const struct ib_mad_hdr *in_mad,
						  size_t in_mad_size,
						  struct ib_mad_hdr *out_mad,
						  size_t *out_mad_size,
						  u16 *out_mad_pkey_index);
	struct ib_xrcd *	   (*alloc_xrcd)(struct ib_device *device,
						 struct ib_ucontext *ucontext,
						 struct ib_udata *udata);
	int			   (*dealloc_xrcd)(struct ib_xrcd *xrcd);
	struct ib_flow *	   (*create_flow)(struct ib_qp *qp,
						  struct ib_flow_attr
						  *flow_attr,
						  int domain);
	int			   (*destroy_flow)(struct ib_flow *flow_id);
	int			   (*check_mr_status)(struct ib_mr *mr, u32 check_mask,
						      struct ib_mr_status *mr_status);
	void			   (*disassociate_ucontext)(struct ib_ucontext *ibcontext);

	struct ib_dma_mapping_ops   *dma_ops;

	struct module               *owner;
	struct device                dev;
	struct kobject               *ports_parent;
	struct list_head             port_list;

	enum {
		IB_DEV_UNINITIALIZED,
		IB_DEV_REGISTERED,
		IB_DEV_UNREGISTERED
	}                            reg_state;

	int			   