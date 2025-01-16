uff.status;
}
#define SN_MEMPROT_ACCESS_CLASS_0		0x14a080
#define SN_MEMPROT_ACCESS_CLASS_1		0x2520c2
#define SN_MEMPROT_ACCESS_CLASS_2		0x14a1ca
#define SN_MEMPROT_ACCESS_CLASS_3		0x14a290
#define SN_MEMPROT_ACCESS_CLASS_6		0x084080
#define SN_MEMPROT_ACCESS_CLASS_7		0x021080

/*
 * Turns off system power.
 */
static inline void
ia64_sn_power_down(void)
{
	struct ia64_sal_retval ret_stuff;
	SAL_CALL(ret_stuff, SN_SAL_SYSTEM_POWER_DOWN, 0, 0, 0, 0, 0, 0, 0);
	while(1)
		cpu_relax();
	/* never returns */
}

/**
 * ia64_sn_fru_capture - tell the system controller to capture hw state
 *
 * This routine will call the SAL which will tell the system controller(s)
 * to capture hw mmr information from each SHub in the system.
 */
static inline u64
ia64_sn_fru_capture(void)
{
        struct ia64_sal_retval isrv;
        SAL_CALL(isrv, SN_SAL_SYSCTL_FRU_CAPTURE, 0, 0, 0, 0, 0, 0, 0);
        if (isrv.status)
                return 0;
        return isrv.v0;
}

/*
 * Performs an operation on a PCI bus or slot -- power up, power down
 * or reset.
 */
static inline u64
ia64_sn_sysctl_iobrick_pci_op(nasid_t n, u64 connection_type, 
			      u64 bus, char slot, 
			      u64 action)
{
	struct ia64_sal_retval rv = {0, 0, 0, 0};

	SAL_CALL_NOLOCK(rv, SN_SAL_SYSCTL_IOBRICK_PCI_OP, connection_type, n, action,
		 bus, (u64) slot, 0, 0);
	if (rv.status)
	    	return rv.v0;
	return 0;
}


/*
 * Open a subchannel for sending arbitrary data to the system
 * controller network via the system controller device associated with
 * 'nasid'.  Return the subchannel number or a negative error code.
 */
static inline int
ia64_sn_irtr_open(nasid_t nasid)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_IROUTER_OP, SAL_IROUTER_OPEN, nasid,
			   0, 0, 0, 0, 0);
	return (int) rv.v0;
}

/*
 * Close system controller subchannel 'subch' previously opened on 'nasid'.
 */
static inline int
ia64_sn_irtr_close(nasid_t nasid, int subch)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_IROUTER_OP, SAL_IROUTER_CLOSE,
			   (u64) nasid, (u64) subch, 0, 0, 0, 0);
	return (int) rv.status;
}

/*
 * Read data from system controller associated with 'nasid' on
 * subchannel 'subch'.  The buffer to be filled is pointed to by
 * 'buf', and its capacity is in the integer pointed to by 'len'.  The
 * referent of 'len' is set to the number of bytes read by the SAL
 * call.  The return value is either SALRET_OK (for bytes read) or
 * SALRET_ERROR (for error or "no data available").
 */
static inline int
ia64_sn_irtr_recv(nasid_t nasid, int subch, char *buf, int *len)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_IROUTER_OP, SAL_IROUTER_RECV,
			   (u64) nasid, (u64) subch, (u64) buf, (u64) len,
			   0, 0);
	return (int) rv.status;
}

/*
 * Write data to the system controller network via the system
 * controller associated with 'nasid' on suchannel 'subch'.  The
 * buffer to be written out is pointed to by 'buf', and 'len' is the
 * number of bytes to be written.  The return value is either the
 * number of bytes written (which could be zero) or a negative error
 * code.
 */
static inline int
ia64_sn_irtr_send(nasid_t nasid, int subch, char *buf, int len)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_IROUTER_OP, SAL_IROUTER_SEND,
			   (u64) nasid, (u64) subch, (u64) buf, (u64) len,
			   0, 0);
	return (int) rv.v0;
}

/*
 * Check whether any interrupts are pending for the system controller
 * associated with 'nasid' and its subchannel 'subch'.  The return
 * value is a mask of pending interrupts (SAL_IROUTER_INTR_XMIT and/or
 * SAL_IROUTER_INTR_RECV).
 */
static inline int
ia64_sn_irtr_intr(nasid_t nasid, int subch)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_IROUTER_OP, SAL_IROUTER_INTR_STATUS,
			   (u64) nasid, (u64) subch, 0, 0, 0, 0);
	return (int) rv.v0;
}

/*
 * Enable the interrupt indicated by the intr parameter (either
 * SAL_IROUTER_INTR_XMIT or SAL_IROUTER_INTR_RECV).
 */
static inline int
ia64_sn_irtr_intr_enable(nasid_t nasid, int subch, u64 intr)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_IROUTER_OP, SAL_IROUTER_INTR_ON,
			   (u64) nasid, (u64) subch, intr, 0, 0, 0);
	return (int) rv.v0;
}

/*
 * Disable the interrupt indicated by the intr parameter (either
 * SAL_IROUTER_INTR_XMIT or SAL_IROUTER_INTR_RECV).
 */
static inline int
ia64_sn_irtr_intr_disable(nasid_t nasid, int subch, u64 intr)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_IROUTER_OP, SAL_IROUTER_INTR_OFF,
			   (u64) nasid, (u64) subch, intr, 0, 0, 0);
	return (int) rv.v0;
}

/*
 * Set up a node as the point of contact for system controller
 * environmental event delivery.
 */
static inline int
ia64_sn_sysctl_event_init(nasid_t nasid)
{
        struct ia64_sal_retval rv;
        SAL_CALL_REENTRANT(rv, SN_SAL_SYSCTL_EVENT, (u64) nasid,
			   0, 0, 0, 0, 0, 0);
        return (int) rv.v0;
}

/*
 * Ask the system controller on the specified nasid to reset
 * the CX corelet clock.  Only valid on TIO nodes.
 */
static inline int
ia64_sn_sysctl_tio_clock_reset(nasid_t nasid)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_SYSCTL_OP, SAL_SYSCTL_OP_TIO_JLCK_RST,
			nasid, 0, 0, 0, 0, 0);
	if (rv.status != 0)
		return (int)rv.status;
	if (rv.v0 != 0)
		return (int)rv.v0;

	return 0;
}

/*
 * Get the associated ioboard type for a given nasid.
 */
static inline long
ia64_sn_sysctl_ioboard_get(nasid_t nasid, u16 *ioboard)
{
	struct ia64_sal_retval isrv;
	SAL_CALL_REENTRANT(isrv, SN_SAL_SYSCTL_OP, SAL_SYSCTL_OP_IOBOARD,
			   nasid, 0, 0, 0, 0, 0);
	if (isrv.v0 != 0) {
		*ioboard = isrv.v0;
		return isrv.status;
	}
	if (isrv.v1 != 0) {
		*ioboard = isrv.v1;
		return isrv.status;
	}

	return isrv.status;
}

/**
 * ia64_sn_get_fit_compt - read a FIT entry from the PROM header
 * @nasid: NASID of node to read
 * @index: FIT entry index to be retrieved (0..n)
 * @fitentry: 16 byte buffer where FIT entry will be stored.
 * @banbuf: optional buffer for retrieving banner
 * @banlen: length of banner buffer
 *
 * Access to the physical PROM chips needs to be serialized since reads and
 * writes can't occur at the same time, so we need to call into the SAL when
 * we want to look at the FIT entries on the chips.
 *
 * Returns:
 *	%SALRET_OK if ok
 *	%SALRET_INVALID_ARG if index too big
 *	%SALRET_NOT_IMPLEMENTED if running on older PROM
 *	??? if nasid invalid OR banner buffer not large enough
 */
static inline int
ia64_sn_get_fit_compt(u64 nasid, u64 index, void *fitentry, void *banbuf,
		      u64 banlen)
{
	struct ia64_sal_retval rv;
	SAL_CALL_NOLOCK(rv, SN_SAL_GET_FIT_COMPT, nasid, index, fitentry,
			banbuf, banlen, 0, 0);
	return (int) rv.status;
}

/*
 * Initialize the SAL components of the system controller
 * communication driver; specifically pass in a sizable buffer that
 * can be used for allocation of subchannel queues as new subchannels
 * are opened.  "buf" points to the buffer, and "len" specifies its
 * length.
 */
static inline int
ia64_sn_irtr_init(nasid_t nasid, void *buf, int len)
{
	struct ia64_sal_retval rv;
	SAL_CALL_REENTRANT(rv, SN_SAL_IROUTER_OP, SAL_IROUTER_INIT,
			   (u64) nasid, (u64) buf, (u64) len, 0, 0, 0);
	return (int) rv.status;
}

/*
 * Returns the nasid, subnode & slice corresponding to a SAPIC ID
 *
 *  In:
 *	arg0 - SN_SAL_GET_SAPIC_INFO
 *	arg1 - sapicid (lid >> 16) 
 *  Out:
 *	v0 - nasid
 *	v1 - subnode
 *	v2 - slice
 */
static inline u64
ia64_sn_get_sapic_info(int sapicid, int *nasid, int *subnode, int *slice)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_GET_SAPIC_INFO, sapicid, 0, 0, 0, 0, 0, 0);

/***** BEGIN HACK - temp til old proms no longer supported ********/
	if (ret_stuff.status == SALRET_NOT_IMPLEMENTED) {
		if (nasid) *nasid = sapicid & 0xfff;
		if (subnode) *subnode = (sapicid >> 13) & 1;
		if (slice) *slice = (sapicid >> 12) & 3;
		return 0;
	}
/***** END HACK *******/

	if (ret_stuff.status < 0)
		return ret_stuff.status;

	if (nasid) *nasid = (int) ret_stuff.v0;
	if (subnode) *subnode = (int) ret_stuff.v1;
	if (slice) *slice = (int) ret_stuff.v2;
	return 0;
}
 
/*
 * Returns information about the HUB/SHUB.
 *  In:
 *	arg0 - SN_SAL_GET_SN_INFO
 * 	arg1 - 0 (other values reserved for future use)
 *  Out:
 *	v0 
 *		[7:0]   - shub type (0=shub1, 1=shub2)
 *		[15:8]  - Log2 max number of nodes in entire system (includes
 *			  C-bricks, I-bricks, etc)
 *		[23:16] - Log2 of nodes per sharing domain			 
 * 		[31:24] - partition ID
 * 		[39:32] - coherency_id
 * 		[47:40] - regionsize
 *	v1 
 *		[15:0]  - nasid mask (ex., 0x7ff for 11 bit nasid)
 *	 	[23:15] - bit position of low nasid bit
 */
static inline u64
ia64_sn_get_sn_info(int fc, u8 *shubtype, u16 *nasid_bitmask, u8 *nasid_shift, 
		u8 *systemsize, u8 *sharing_domain_size, u8 *partid, u8 *coher, u8 *reg)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_GET_SN_INFO, fc, 0, 0, 0, 0, 0, 0);

/***** BEGIN HACK - temp til old proms no longer supported ********/
	if (ret_stuff.status == SALRET_NOT_IMPLEMENTED) {
		int nasid = get_sapicid() & 0xfff;
#define SH_SHUB_ID_NODES_PER_BIT_MASK 0x001f000000000000UL
#define SH_SHUB_ID_NODES_PER_BIT_SHFT 48
		if (shubtype) *shubtype = 0;
		if (nasid_bitmask) *nasid_bitmask = 0x7ff;
		if (nasid_shift) *nasid_shift = 38;
		if (systemsize) *systemsize = 10;
		if (sharing_domain_size) *sharing_domain_size = 8;
		if (partid) *partid = ia64_sn_sysctl_partition_get(nasid);
		if (coher) *coher = nasid >> 9;
		if (reg) *reg = (HUB_L((u64 *) LOCAL_MMR_ADDR(SH1_SHUB_ID)) & SH_SHUB_ID_NODES_PER_BIT_MASK) >>
			SH_SHUB_ID_NODES_PER_BIT_SHFT;
		return 0;
	}
/***** END HACK *******/

	if (ret_stuff.status < 0)
		return ret_stuff.status;

	if (shubtype) *shubtype = ret_stuff.v0 & 0xff;
	if (systemsize) *systemsize = (ret_stuff.v0 >> 8) & 0xff;
	if (sharing_domain_size) *sharing_domain_size = (ret_stuff.v0 >> 16) & 0xff;
	if (partid) *partid = (ret_stuff.v0 >> 24) & 0xff;
	if (coher) *coher = (ret_stuff.v0 >> 32) & 0xff;
	if (reg) *reg = (ret_stuff.v0 >> 40) & 0xff;
	if (nasid_bitmask) *nasid_bitmask = (ret_stuff.v1 & 0xffff);
	if (nasid_shift) *nasid_shift = (ret_stuff.v1 >> 16) & 0xff;
	return 0;
}
 
/*
 * This is the access point to the Altix PROM hardware performance
 * and status monitoring interface. For info on using this, see
 * arch/ia64/include/asm/sn/sn2/sn_hwperf.h
 */
static inline int
ia64_sn_hwperf_op(nasid_t nasid, u64 opcode, u64 a0, u64 a1, u64 a2,
                  u64 a3, u64 a4, int *v0)
{
	struct ia64_sal_retval rv;
	SAL_CALL_NOLOCK(rv, SN_SAL_HWPERF_OP, (u64)nasid,
		opcode, a0, a1, a2, a3, a4);
	if (v0)
		*v0 = (int) rv.v0;
	return (int) rv.status;
}

static inline int
ia64_sn_ioif_get_pci_topology(u64 buf, u64 len)
{
	struct ia64_sal_retval rv;
	SAL_CALL_NOLOCK(rv, SN_SAL_IOIF_GET_PCI_TOPOLOGY, buf, len, 0, 0, 0, 0, 0);
	return (int) rv.status;
}

/*
 * BTE error recovery is implemented in SAL
 */
static inline int
ia64_sn_bte_recovery(nasid_t nasid)
{
	struct ia64_sal_retval rv;

	rv.status = 0;
	SAL_CALL_NOLOCK(rv, SN_SAL_BTE_RECOVER, (u64)nasid, 0, 0, 0, 0, 0, 0);
	if (rv.status == SALRET_NOT_IMPLEMENTED)
		return 0;
	return (int) rv.status;
}

static inline int
ia64_sn_is_fake_prom(void)
{
	struct ia64_sal_retval rv;
	SAL_CALL_NOLOCK(rv, SN_SAL_FAKE_PROM, 0, 0, 0, 0, 0, 0, 0);
	return (rv.status == 0);
}

static inline int
ia64_sn_get_prom_feature_set(int set, unsigned long *feature_set)
{
	struct ia64_sal_retval rv;

	SAL_CALL_NOLOCK(rv, SN_SAL_GET_PROM_FEATURE_SET, set, 0, 0, 0, 0, 0, 0);
	if (rv.status != 0)
		return rv.status;
	*feature_set = rv.v0;
	return 0;
}

static inline int
ia64_sn_set_os_feature(int feature)
{
	struct ia64_sal_retval rv;

	SAL_CALL_NOLOCK(rv, SN_SAL_SET_OS_FEATURE_SET, feature, 0, 0, 0, 0, 0, 0);
	return rv.status;
}

static inline int
sn_inject_error(u64 paddr, u64 *data, u64 *ecc)
{
	struct ia64_sal_retval ret_stuff;

	ia64_sal_oemcall_nolock(&ret_stuff, SN_SAL_INJECT_ERROR, paddr, (u64)data,
				(u64)ecc, 0, 0, 0, 0);
	return ret_stuff.status;
}

static inline int
ia64_sn_set_cpu_number(int cpu)
{
	struct ia64_sal_retval rv;

	SAL_CALL_NOLOCK(rv, SN_SAL_SET_CPU_NUMBER, cpu, 0, 0, 0, 0, 0, 0);
	return rv.status;
}
static inline int
ia64_sn_kernel_launch_event(void)
{
 	struct ia64_sal_retval rv;
	SAL_CALL_NOLOCK(rv, SN_SAL_KERNEL_LAUNCH_EVENT, 0, 0, 0, 0, 0, 0, 0);
	return rv.status;
}

union sn_watchlist_u {
	u64     val;
	struct {
		u64	blade	: 16,
			size	: 32,
			filler	: 16;
	};
};

static inline int
sn_mq_watchlist_alloc(int blade, void *mq, unsigned int mq_size,
				unsigned long *intr_mmr_offset)
{
	struct ia64_sal_retval rv;
	unsigned long addr;
	union sn_watchlist_u size_blade;
	int watchlist;

	addr = (unsigned long)mq;
	size_blade.size = mq_size;
	size_blade.blade = blade;

	/*
	 * bios returns watchlist number or negative error number.
	 */
	ia64_sal_oemcall_nolock(&rv, SN_SAL_WATCHLIST_ALLOC, addr,
			size_blade.val, (u64)intr_mmr_offset,
			(u64)&watchlist, 0, 0, 0);
	if (rv.status < 0)
		return rv.status;

	return watchlist;
}

static inline int
sn_mq_watchlist_free(int blade, int watchlist_num)
{
	struct ia64_sal_retval rv;
	ia64_sal_oemcall_nolock(&rv, SN_SAL_WATCHLIST_FREE, blade,
			watchlist_num, 0, 0, 0, 0, 0);
	return rv.status;
}
#endif /* _ASM_IA64_SN_SN_SAL_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            