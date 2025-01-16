oc_pd(struct ib_device *device);

void ib_dealloc_pd(struct ib_pd *pd);

/**
 * ib_create_ah - Creates an address handle for the given address vector.
 * @pd: The protection domain associated with the address handle.
 * @ah_attr: The attributes of the address vector.
 *
 * The address handle is used to reference a local or global destination
 * in all UD QP post sends.
 */
struct ib_ah *ib_create_ah(struct ib_pd *pd, struct ib_ah_attr *ah_attr);

/**
 * ib_init_ah_from_wc - Initializes address handle attributes from a
 *   work completion.
 * @device: Device on which the received message arrived.
 * @port_num: Port on which the received message arrived.
 * @wc: Work completion associated with the received message.
 * @grh: References the received global route header.  This parameter is
 *   ignored unless the work completion indicates that the GRH is valid.
 * @ah_attr: Returned attributes that can be used when creating an address
 *   handle for replying to the message.
 */
int ib_init_ah_from_wc(struct ib_device *device, u8 port_num,
		       const struct ib_wc *wc, const struct ib_grh *grh,
		       struct ib_ah_attr *ah_attr);

/**
 * ib_create_ah_from_wc - Creates an address handle associated with the
 *   sender of the specified work completion.
 * @pd: The protection domain associated with the address handle.
 * @wc: Work completion information associated with a received message.
 * @grh: References the received global route header.  This parameter is
 *   ignored unless the work completion indicates that the GRH is valid.
 * @port_num: The outbound port number to associate with the address.
 *
 * The address handle is used to reference a local or global destination
 * in all UD QP post sends.
 */
struct ib_ah *ib_create_ah_from_wc(struct ib_pd *pd, const struct ib_wc *wc,
				   const struct ib_grh *grh, u8 port_num);

/**
 * ib_modify_ah - Modifies the address vector associated with an address
 *   handle.
 * @ah: The address handle to modify.
 * @ah_attr: The new address vector attributes to associate with the
 *   address handle.
 */
int ib_modify_ah(struct ib_ah *ah, struct ib_ah_attr *ah_attr);

/**
 * ib_query_ah - Queries the address vector associated with an address
 *   handle.
 * @ah: The address handle to query.
 * @ah_attr: The address vector attributes associated with the address
 *   handle.
 */
int ib_query_ah(struct ib_ah *ah, struct ib_ah_attr *ah_attr);

/**
 * ib_destroy_ah - Destroys an address handle.
 * @ah: The address handle to destroy.
 */
int ib_destroy_ah(struct ib_ah *ah);

/**
 * ib_create_srq - Creates a SRQ associated with the 