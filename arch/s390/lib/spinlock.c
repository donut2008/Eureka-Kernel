 L4 headers*/
	IB_FLOW_SPEC_TCP	= 0x40,
	IB_FLOW_SPEC_UDP	= 0x41
};
#define IB_FLOW_SPEC_LAYER_MASK	0xF0
#define IB_FLOW_SPEC_SUPPORT_LAYERS 4

/* Flow steering rule priority is set according to it's domain.
 * Lower domain value means higher priority.
 */
enum ib_flow_domain {
	IB_FLOW_DOMAIN_USER,
	IB_FLOW_DOMAIN_ETHTOOL,
	IB_FLOW_DOMAIN_RFS,
	IB_FLOW_DOMAIN_NIC,
	IB_FLOW_DOMAIN_NUM /* Must be last */
};

struct ib_flow_eth_filter {
	u8	dst_mac[6];
	u8	src_mac[6];
	__be16	ether_type;
	__be16	vlan_tag;
};

struct ib_flow_spec_eth {
	enum ib_flow_spec_type	  type;
	u16			  size;
	struct ib_flow_eth_filter val;
	struct ib_flow_eth_filter mask;
};

struct ib_flow_ib_filter {
	__be16 dlid;
	__u8   sl;
};

struct ib_flow_spec_ib {
	enum ib_flow_spec_type	 type;
	u16			 size;
	struct ib_flow_ib_filter val;
	struct ib_flow_ib_filter mask;
};

struct ib_flow_ipv4_filter {
	__be32	src_ip;
	__be32	dst_ip;
};

struct ib_flow_spec_ipv4 {
	enum ib_flow_spec_type	   type;
	u16			   size;
	struct ib_flow_ipv4_filter val;
	struct ib_flow_ipv4_filter mask;
};

struct ib_flow_tcp_udp_filter {
	__be16	dst_port;
	__be16	src_port;
};

struct ib_flow_spec_tcp_udp {
	enum ib_flow_spec_type	      type;
	u16			      size;
	struct ib_flow_tcp_udp_filter val;
	struct ib_flow_tcp_udp_filter mask;
};

union ib_flow_spec {
	struct {
		enum ib_flow_spec_type	type;
		u16			size;
	};
	struct ib_flow_spec_eth		eth;
	struct ib_flow_spec_ib		ib;
	struct ib_flow_spec_ipv4        ipv4;
	struct ib_flow_spec_tcp_udp	tcp_udp;
};

struct ib_flow_attr {
	enum ib_flow_attr_type type;
	u16	     size;
	u16	     priority;
	u32	     flags;
	u8	     num_of_specs;
	u8	     port;
	/* Following are the optional layers according to user request
	 * struct ib_flow_spec_xxx
	 * struct ib_flow_spec_yyy
	 */
};

struct ib_flow {
	struct ib_qp		*qp;
	struct ib_uobject	*uobject;
};

struct ib_mad_hdr;
struct ib_grh;

enum ib_process_mad_flags {
	IB_MAD_IGNORE_MKEY	= 1,
	IB_MAD_IGNORE_BKEY	= 2,
	IB_MAD_IGNORE_ALL	= IB_MAD_IGNORE_MKEY | IB_MAD_IGNORE_BKEY
};

enum ib_mad_result {
	IB_MAD_RESULT_FAILURE  = 0,      /* (!SUCCESS is the important flag) */
	IB_MAD_RESULT_SUCCESS  = 1 << 0, /* MAD was successfully processed   */
	IB_MAD_RESULT_REPLY    = 1 << 1, /* Reply packet needs to be sent    */
	IB_MAD_RESULT_CONSUMED = 1 << 2  /* Packet consumed: stop processing */
};

#define IB_DEVICE_NAME_MAX 64

struct ib_cache {
	rwlock_t                lock;
	struct ib_event_handler event_handler;
	struct ib_pkey_cache  **pkey_cache;
	struct ib_gid_table   **gid_cache;
	u8                     *lmc_cache;
};

struct ib_dma_mapping_ops {
	int		(*mapping_error)(struct ib_device *dev,
					 u64 dma_addr);
	u64		(*map_single)(struct ib_device *dev,
				      void *ptr, size_t size,
				      enum dma_data_direction direction);
	void		(*unmap_single)(struct ib_device *dev,
					u64 addr, size_t size,
					enum dma_data_direction direction);
	u64		(*map_page)(struct ib_device *dev,
				    struct page *page, unsigned long offset,
				    size_t size,
				    enum dma_data_direction direction);
	void		(*unmap_page)(struct ib_device *dev,
				      u64 addr, size_t size,
				      enum dma_data_direction direction);
	int		(*map_sg)(struct ib_device *dev,
				  struct scatterlist *sg, int nents,
				  enum dma_data_direction direction);
	void		(*unmap_sg)(struct ib_device *dev,
				    struct scatterlist *sg, int nents,
				    enum dma_data_direction direction);
	void		(*sync_single_for_cpu)(struct ib_device *dev,
					       u64 dma_handle,
					       size_t size,
					       enum dma_data_direction dir);
	void		(*sync_single_for_device)(struct ib_device *dev,
						  u64 dma_handle,
						  size_t size,
						  enum dma_data_direction dir);
	void		*(*alloc_coherent)(struct ib_device *dev,
					   size_t size,
					   u64 *dma_handle,
					   gfp_t flag);
	void		(*free_coherent)(struct ib_device *dev,
					 size_t size, void *cpu_addr,
					 u64 dma_handle);
};

struct iw_cm_verbs;

struct ib_port_immutable {
	int                           pkey_tbl_len;
	int                           gid_tbl_len;
	u32                           core_cap_flags;
	u32                           max_mad_size;
};

struct ib_device {
	struct device                *dma_device;

	char                          name[IB_DEVICE_NAME_MAX];

	struct list_head              event_handler_list;
	spinlock_t                    event_handler_lock;

	spinlock_t                    client_data_lock;
	struct list_head              core_list;
	/* Access to the client_data_list is protected by the client_data_lock
	 * spinlock and the lists_rwsem read-write semaphore */
	struct list_head              client_data_list;

	struct ib_cache               cache;
	/**
	 * port_immutable is indexed by port number
	 */
	struct ib_port_immutable     *port_immutable;

	int			      num_comp_vectors;

	struct iw_cm_verbs	     *iwcm;

	int		           (*get_protocol_stats)(struct ib_device *device,
							 union rdma_protocol_stats *stats);
	int		           (*query_device)(struct ib_device *device,
						   struct ib_device_attr *device_attr,
						   struct ib_udata *udata);
	int		           (*query_port)(struct ib_device *device,
						 u8 port_num,
						 struct ib_port_attr *port_attr);
	enum rdma_link_layer	   (*get_link_layer)(struct ib_device *device,
						     u8 port_num);
	/* When calling get_netdev, the HW vendor's driver should return the
	 * net device of device @device at port @port_num or NULL if such
	 * a net device doesn't exist. The vendor driver should call dev_hold
	 * on this net device. The HW vendor's device driver must guarantee
	 * that this function returns NULL before the net device reaches
	 * NETDEV_UNREGISTER_FINAL state.
	 */
	struct net_device	  *(*get_netdev)(struct ib_device *device,
						 u8 port_num);
	int		           (*query_gid)(struct ib_device *device,
						u8 port_num, int index,
						union ib_gid *gid);
	/* When calling add_gid, the HW vendor's driver should
	 * add the gid of device @device at gid ind