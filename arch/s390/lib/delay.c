ruct list_head	xrcd_list;
	struct list_head	rule_list;
	int			closing;

	struct pid             *tgid;
#ifdef CONFIG_INFINIBAND_ON_DEMAND_PAGING
	struct rb_root      umem_tree;
	/*
	 * Protects .umem_rbroot and tree, as well as odp_mrs_count and
	 * mmu notifiers registration.
	 */
	struct rw_semaphore	umem_rwsem;
	void (*invalidate_range)(struct ib_umem *umem,
				 unsigned long start, unsigned long end);

	struct mmu_notifier	mn;
	atomic_t		notifier_count;
	/* A list of umems that don't have private mmu notifier counters yet. */
	struct list_head	no_private_counters;
	int                     odp_mrs_count;
#endif
};

struct ib_uobject {
	u64			user_handle;	/* handle given to us by userspace */
	struct ib_ucontext     *context;	/* associated user context */
	void		       *object;		/* containing object */
	struct list_head	list;		/* link to context's list */
	int			id;		/* index into kernel idr */
	struct kref		ref;
	struct rw_semaphore	mutex;		/* protects .live */
	struct rcu_head		rcu;		/* kfree_rcu() overhead */
	int			live;
};

struct ib_udata {
	const void __user *inbuf;
	void __user *outbuf;
	size_t       inlen;
	size_t       outlen;
};

struct ib_pd {
	u32			local_dma_lkey;
	struct ib_device       *device;
	struct ib_uobject      *uobject;
	atomic_t          	usecnt; /* count all resources */
	struct ib_mr	       *local_mr;
};

struct ib_xrcd {
	struct ib_device       *device;
	atomic_t		usecnt; /* count all exposed resources */
	struct inode	       *inode;

	struct mutex		tgt_qp_mutex;
	struct list_head	tgt_qp_list;
};

struct ib_ah {
	struct ib_device	*device;
	struct ib_pd		*pd;
	struct ib_uobject	*uobject;
};

typedef void (*ib_comp_handler)(struct ib_cq *cq, void *cq_context);

struct ib_cq {
	struct ib_device       *device;
	struct ib_uobject      *uobject;
	ib_comp_handler   	comp_handler;
	void                  (*event_handler)(struct ib_event *, void *);
	void                   *cq_context;
	int               	cqe;
	atomic_t          	usecnt; /* count number of work queues */
};

struct ib_srq {
	struct ib_device       *device;
	struct ib_pd	       *pd;
	struct ib_uobject      *uobject;
	void		      (*event_handler)(struct ib_event *, void *);
	void		       *srq_context;
	enum ib_srq_type	srq_type;
	atomic_t		usecnt;

	union {
		struct {
			struct ib_xrcd *xrcd;
			struct ib_cq   *cq;
			u32		srq_num;
		} xrc;
	} ext;
};

struct ib_qp {
	struct ib_device       *device;
	struct ib_pd	       *pd;
	struct ib_cq	       *send_cq;
	struct ib_cq	       *recv_cq;
	struct ib_srq	       *srq;
	struct ib_xrcd	       *xrcd; /* XRC TGT QPs only */
	struct list_head	xrcd_list;
	/* count times opened, mcast attaches, flow attaches */
	atomic_t		usecnt;
	struct list_head	open_list;
	struct ib_qp           *real_qp;
	struct ib_uobject      *uobject;
	void                  (*event_handler)(struct ib_event *, void *);
	void		       *qp_context;
	u32			qp_num;
	enum ib_qp_type		qp_type;
};

struct ib_mr {
	struct ib_device  *device;
	struct ib_pd	  *pd;
	struct ib_uobject *uobject;
	u32		   lkey