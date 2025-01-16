b_client *client,
			 void *data);

static inline int ib_copy_from_udata(void *dest, struct ib_udata *udata, size_t len)
{
	return copy_from_user(dest, udata->inbuf, len) ? -EFAULT : 0;
}

static inline int ib_copy_to_udata(struct ib_udata *udata, void *src, size_t len)
{
	return copy_to_user(udata->outbuf, src, len) ? -EFAULT : 0;
}

/**
 * ib_modify_qp_is_ok - Check that the supplied attribute mask
 * contains all required attributes and no attributes not allowed for
 * the given QP state transition.
 * @cur_state: Current QP state
 * @next_state: Next QP state
 * @type: QP type
 * @mask: Mask of supplied QP attributes
 * @ll : link layer of port
 *
 * This function is a helper function that a low-level driver's
 * modify_qp method can use to validate the consumer's input.  It
 * checks that cur_state and next_state are valid QP states, that a
 * transition from cur_state to next_state is allowed by the IB spec,
 * and that the attribute mask supplied is allowed for the transition.
 */
int ib_modify_qp_is_ok(enum ib_qp_state cur_state, enum ib_qp_state next_state,
		       enum ib_qp_type type, enum ib_qp_attr_mask mask,
		       enum rdma_link_layer ll);

int ib_register_event_handler  (struct ib_event_handler *event_handler);
int ib_unregister_event_handler(struct ib_event_handler *event_handler);
void ib_dispatch_event(struct ib_event *event);

int ib_query_device(struct ib_device *device,
		    struct ib_device_attr *device_attr);

int ib_query_port(struct ib_device *device,
		  u8 port_num, struct ib_port_attr *port_attr);

enum rdma_link_layer rdma_port_get_link_layer(struct ib_device *device,
					       u8 port_num);

/**
 * rdma_cap_ib_switch - Check if the device is IB switch
 * @device: Device to check
 *
 * Device driver is responsible for setting is_switch bit on
 * in ib_device structure at init time.
 *
 * Return: true if the device is IB switch.
 */
static inline bool rdma_cap_ib_switch(const struct ib_device *device)
{
	return device->is_switch;
}

/**
 * rdma_start_port - Return the first valid port number for the device
 * specified
 *
 * @device: Device to be checked
 *
 * Return start port number
 */
static inline u8 rdma_start_port(const struct ib_device *device)
{
	return rdma_cap_ib_switch(device) ? 0 : 1;
}

/**
 * rdma_end_port - Return the last valid port number for the device
 * specified
 *
 * @device: Device to be checked
 *
 * Return last port number
 */
static inline u8 rdma_end_port(const struct ib_device *device)
{
	return rdma_cap_ib_switch(device) ? 0 : device->phys_port_cnt;
}

static inline bool rdma_protocol_ib(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_PROT_IB;
}

static inline bool rdma_protocol_roce(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_PROT_ROCE;
}

static inline bool rdma_protocol_iwarp(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_PROT_IWARP;
}

static inline bool rdma_ib_or_roce(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags &
		(RDMA_CORE_CAP_PROT_IB | RDMA_CORE_CAP_PROT_ROCE);
}

/**
 * rdma_cap_ib_mad - Check if the port of a device supports Infiniband
 * Management Datagrams.
 * @device: Device to check
 * @port_num: Port number to check
 *
 * Management Datagrams (MAD) are a required part of the InfiniBand
 * specification and are supported on all InfiniBand devices.  A slightly
 * extended version are also supported on OPA interfaces.
 *
 * Return: true if the port supports sending/receiving of MAD packets.
 */
static inline bool rdma_cap_ib_mad(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_IB_MAD;
}

/**
 * rdma_cap_opa_mad - Check if the port of device provides support for OPA
 * Management Datagrams.
 * @device: Device to check
 * @port_num: Port number to check
 *
 * Intel OmniPath devices extend and/or replace the InfiniBand Management
 * datagrams with their own versions.  These OPA MADs share many but not all of
 * the characteristics of InfiniBand MADs.
 *
 * OPA MADs differ in the following ways:
 *
 *    1) MADs are variable size up to 2K
 *       IBTA defined MADs remain fixed at 256 bytes
 *    2) OPA SMPs must carry valid PKeys
 *    3) OPA SMP packets are a different format
 *
 * Return: true if the port supports OPA MAD packet formats.
 */
static inline bool rdma_cap_opa_mad(struct ib_device *device, u8 port_num)
{
	return (device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_OPA_MAD)
		== RDMA_CORE_CAP_OPA_MAD;
}

/**
 * rdma_cap_ib_smi - Check if the port of a device provides an Infiniband
 * Subnet Management Agent (SMA) on the Subnet Management Interface (SMI).
 * @device: Device to check
 * @port_num: Port number to check
 *
 * Each InfiniBand node is required to provide a Subnet Management Agent
 * that the subnet manager can access.  Prior to the fabric being fully
 * configured by the subnet manager, the SMA is accessed via a well known
 * interface called the Subnet Management Interface (SMI).  This interface
 * uses directed route packets to communicate with the SM to get around the
 * chicken and egg problem of the SM needing to know what's on the fabric
 * in order to configure the fabric, and needing to configure the fabric in
 * order to send packets to the devices on the fabric.  These directed
 * route packets do not need the fabric fully configured in order to reach
 * their destination.  The SMI is the only method allowed to send
 * directed route packets on an InfiniBand fabric.
 *
 * Return: true if the port provides an SMI.
 */
static inline bool rdma_cap_ib_smi(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_IB_SMI;
}

/**
 * rdma_cap_ib_cm - Check if the port of device has the capability Infiniband
 * Communication Manager.
 * @device: Device to check
 * @port_num: Port number to check
 *
 * The InfiniBand Communication Manager is one of many pre-defined General
 * Service Agents (GSA) that are accessed via the General Service
 * Interface (GSI).  It's role is to facilitate establishment of connections
 * between nodes as well as other management related tasks for established
 * connections.
 *
 * Return: true if the port supports an IB CM (this does not guarantee that
 * a CM is actually running however).
 */
static inline bool rdma_cap_ib_cm(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_IB_CM;
}

/**
 * rdma_cap_iw_cm - Check if the port of device has the capability IWARP
 * Communication Manager.
 * @device: Device to check
 * @port_num: Port number to check
 *
 * Similar to above, but specific to iWARP connections which have a different
 * managment protocol than InfiniBand.
 *
 * Return: true if the port supports an iWARP CM (this does not guarantee that
 * a CM is actually running however).
 */
static inline bool rdma_cap_iw_cm(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_IW_CM;
}

/**
 * rdma_cap_ib_sa - Check if the port of device has the capability Infiniband
 * Subnet Administration.
 * @device: Device to check
 * @port_num: Port number to check
 *
 * An InfiniBand Subnet Administration (SA) service is a pre-defined General
 * Service Agent (GSA) provided by the Subnet Manager (SM).  On InfiniBand
 * fabrics, devices should resolve routes to other hosts by contacting the
 * SA to query the proper route.
 *
 * Return: true if the port should act as a client to the fabric Subnet
 * Administration interface.  This does not imply that the SA service is
 * running locally.
 */
static inline bool rdma_cap_ib_sa(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_IB_SA;
}

/**
 * rdma_cap_ib_mcast - Check if the port of device has the capability Infiniband
 * Multicast.
 * @device: Device to check
 * @port_num: Port number to check
 *
 * InfiniBand multicast registration is more complex than normal IPv4 or
 * IPv6 multicast registration.  Each Host Channel Adapter must register
 * with the Subnet Manager when it wishes to join a multicast group.  It
 * should do so only once regardless of how many queue pairs it subscribes
 * to this group.  And it should leave the group only after all queue pairs
 * attached to the group have been detached.
 *
 * Return: true if the port must undertake the additional adminstrative
 * overhead of registering/unregistering with the SM and tracking of the
 * total number of queue pairs attached to the multicast group.
 */
static inline bool rdma_cap_ib_mcast(const struct ib_device *device, u8 port_num)
{
	return rdma_cap_ib_sa(device, port_num);
}

/**
 * rdma_cap_af_ib - Check if the port of device has the capability
 * Native Infiniband Address.
 * @device: Device to check
 * @port_num: Port number to check
 *
 * InfiniBand addressing uses a port's GUID + Subnet Prefix to make a default
 * GID.  RoCE uses a different mechanism, but still generates a GID via
 * a prescribed mechanism and port specific data.
 *
 * Return: true if the port uses a GID address to identify devices on the
 * network.
 */
static inline bool rdma_cap_af_ib(const struct ib_device *device, u8 port_num)
{
	return device->port_immutable[port_num].core_cap_flags & RDMA_CORE_CAP_AF_IB;
}

/**
 * rdma_cap_eth_ah - Check if the port of device has the capability
 * Ethernet Address Handle.
 * @device: Device to check
 * @port_num: Port number to check
 *
 * RoCE is InfiniBand over Ethernet, and it uses a well defined technique
 * to fabricate GIDs over Ethernet/IP specific addresses native to the
 * port.  Normally, packet headers are generated by the sending host
 * adapter, but when sending connectionless datagrams, we must manually