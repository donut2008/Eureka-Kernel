MASTER              0x0010
#define QIB_RUNTIME_RCHK                0x0020
#define QIB_RUNTIME_NODMA_RTAIL         0x0080
#define QIB_RUNTIME_SPECIAL_TRIGGER     0x0100
#define QIB_RUNTIME_SDMA                0x0200
#define QIB_RUNTIME_FORCE_PIOAVAIL      0x0400
#define QIB_RUNTIME_PIO_REGSWAPPED      0x0800
#define QIB_RUNTIME_CTXT_MSB_IN_QP      0x1000
#define QIB_RUNTIME_CTXT_REDIRECT       0x2000
#define QIB_RUNTIME_HDRSUPP             0x4000

/*
 * This structure is returned by qib_userinit() immediately after
 * open to get implementation-specific info, and info specific to this
 * instance.
 *
 * This struct must have explict pad fields where type sizes
 * may result in different alignments between 32 and 64 bit
 * programs, since the 64 bit * bit kernel requires the user code
 * to have matching offsets
 */
struct qib_base_info {
	/* version of hardware, for feature checking. */
	__u32 spi_hw_version;
	/* version of software, for feature checking. */
	__u32 spi_sw_version;
	/* QLogic_IB context assigned, goes into sent packets */
	__u16 spi_ctxt;
	__u16 spi_subctxt;
	/*
	 * IB MTU, packets IB data must be less than this.
	 * The MTU is in bytes, and will be a multiple of 4 bytes.
	 */
	__u32 spi_mtu;
	/*
	 * Size of a PIO buffer.  Any given packet's total size must be less
	 * than this (in words).  Included is the starting control word, so
	 * if 513 is returned, then total pkt size is 512 words or less.
	 */
	__u32 spi_piosize;
	/* size of the TID cache in qlogic_ib, in entries */
	__u32 spi_tidcnt;
	/* size of the TID Eager list in qlogic_ib, in entries */
	__u32 spi_tidegrcnt;
	/* size of a single receive header queue entry in words. */
	__u32 spi_rcvhdrent_size;
	/*
	 * Count of receive header queue entries allocated.
	 * This may be less than the spu_rcvhdrcnt passed in!.
	 */
	__u32 spi_rcvhdr_cnt;

	/* per-chip and other runtime features bitmap (QIB_RUNTIME_*) */
	__u32 spi_runtime_flags;

	/* address where hardware receive header queue is mapped */
	__u64 spi_rcvhdr_base;

	/* user program. */

	/* base address of eager TID receive buffers used by hardware. */
	__u64 spi_rcv_egrbufs;

	/* Allocated by initialization code, not by protocol. */

	/*
	 * Size of each TID buffer in host memory, starting at
	 * spi_rcv_egrbufs.  The buffers are virtually contiguous.
	 */
	__u32 spi_rcv_egrbufsize;
	/*
	 * The special QP (queue pair) value that identifies an qlogic_ib
	 * protocol packet from standard IB packets.  More, probably much
	 * more, to be added.
	 */
	__u32 spi_qpair;

	/*
	 * User register base for init code, not to be used directly by
	 * protocol or applications.  Always points to chip registers,
	 * for normal or shared context.
	 */
	__u64 spi_uregbase;
	/*
	 * Maximum buffer size in bytes that can be used in a single TID
	 * entry (assuming the buffer is aligned to this boundary).  This is
	 * the minimum of what the hardware and software support Guaranteed
	 * to be a power of 2.
	 */
	__u32 spi_tid_maxsize;
	/*
	 * alignment of each pio send buffer (byte count
	 * to add to spi_piobufbase to get to second buffer)
	 */
	__u32 spi_pioalign;
	/*
	 * The index of the first pio buffer available to this process;
	 * needed to do lookup in spi_pioavailaddr; not added to
	 * spi_piobufbase.
	 */
	__u32 spi_pioindex;
	 /* number of buffers mapped for this process */
	__u32 spi_piocnt;

	/*
	 * Base address of writeonly pio buffers for this process.
	 * Each buffer has spi_piosize words, and is aligned on spi_pioalign
	 * boundaries.  spi_piocnt buffers are mapped from this address
	 */
	__u64 spi_piobufbase;

	/*
	 * Base address of readonly memory copy of the pioavail registers.
	 * There are 2 bits for each buffer.
	 */
	__u64 spi_pioavailaddr;

	/*
	 * Address where driver updates a copy of the interface and driver
	 * status (QIB_STATUS_*) as a 64 bit value.  It's followed by a
	 * link status qword (formerly combined with driver status), then a
	 * string indicating hardware error, if there was one.
	 */
	__u64 spi_status;

	/* number of chip ctxts available to user processes */
	__u32 spi_nctxts;
	__u16 spi_unit; /* unit number of chip we are using */
	__u16 spi_port; /* IB port number we are using */
	/* num bufs in each contiguous set */
	__u32 spi_rcv_egrperchunk;
	/* size in bytes of each contiguous set */
	__u32 spi_rcv_egrchunksize;
	/* total size of mmap to cover full rcvegrbuffers */
	__u32 spi_rcv_egrbuftotlen;
	__u32 spi_rhf_offset; /* dword offset in hdrqent for rcvhdr flags */
	/* address of readonly memory copy of the rcvhdrq tail register. */
	__u64 spi_rcvhdr_tailaddr;

	/*
	 * shared memory pages for subctxts if ctxt is shared; these cover
	 * all the processes in the group sharing a single context.
	 * all have enough space for the num_subcontexts value on this job.
	 */
	__u64 spi_subctxt_uregbase;
	__u64 spi_subctxt_rcvegrbuf;
	__u64 spi_subctxt_rcvhdr_base;

	/* shared memory page for send buffer disarm status */
	__u64 spi_sendbuf_status;
} __aligned(8);

/*
 * This version number is given to the driver by the user code during
 * initialization in the spu_userversion field of qib_user_info, so
 * the driver can check for compatibility with user code.
 *
 * The major version changes when data structures
 * change in an incompatible way.  The driver must be the same or higher
 * for initialization to succeed.  In some cases, a higher version
 * driver will not interoperate with older software, and initialization
 * will return an error.
 */
#define QIB_USER_SWMAJOR 1

/*
 * Minor version differences are always compatible
 * a within a major version, however if user software is larger
 * than driver software, some new features and/or structure fields
 * may not be implemented; the user code must deal with this if it
 * cares, or it must abort after initialization reports the difference.
 */
#define QIB_USER_SWMINOR 13

#define QIB_USER_SWVERSION ((QIB_USER_SWMAJOR << 16) | QIB_USER_SWMINOR)

#ifndef QIB_KERN_TYPE
#define QIB_KERN_TYPE 0
#endif

/*
 * Similarly, this is the kernel version going back to the user.  It's
 * slightly different, in that we want to tell if the driver was built as
 * part of a QLogic release, or from the driver from openfabrics.org,
 * kernel.org, or a standard distribution, for support reasons.
 * The high bit is 0 for non-QLogic and 1 for QLogic-built/supplied.
 *
 * It's returned by the driver to the user code during initialization in the
 * spi_sw_version field of qib_base_info, so the user code can in turn
 * check for compatibility with the kernel.
*/
#define QIB_KERN_SWVERSION ((QIB_KERN_TYPE << 31) | QIB_USER_SWVERSION)

/*
 * Define the driver version number.  This is something that refers only
 * to the driver itself, not the software interfaces it supports.
 */
#define QIB_DRIVER_VERSION_BASE "1.11"

/* create the final driver version string */
#ifdef QIB_IDSTR
#define QIB_DRIVER_VERSION QIB_DRIVER_VERSION_BASE " " QIB_IDSTR
#else
#define QIB_DRIVER_VERSION QIB_DRIVER_VERSION_BASE
#endif

/*
 * If the unit is specified via open, HCA choice is fixed.  If port is
 * specified, it's also fixed.  Otherwise we try to spread contexts
 * across ports and HCAs, using different algorithims.  WITHIN is
 * the old default, prior to this mechanism.
 */
#define QIB_PORT_ALG_ACROSS 0 /* round robin contexts across HCAs, then
			       * ports; this is the default */
#define QIB_PORT_ALG_WITHIN 1 /* use all contexts on an HCA (round robin
			       * active ports within), then next HCA */
#define QIB_PORT_ALG_COUNT 2 /* number of algorithm choices */

/*
 * This structure is passed to qib_userinit() to tell the driver where
 * user code buffers are, sizes, etc.   The offsets and sizes of the
 * fields must remain unchanged, for binary compatibility.  It can
 * be extended, if userversion is changed so user code can tell, if needed
 */
struct qib_user_info {
	/*
	 * version of user software, to detect compatibility issues.
	 * Should be set to QIB_USER_SWVERSION.
	 */
	__u32 spu_userversion;

	__u32 _spu_unused2;

	/* size of struct base_info to write to */
	__u32 spu_base_info_size;

	__u32 spu_port_alg; /* which QIB_PORT_ALG_*; unused user minor < 11 */

	/*
	 * If two or more processes wish to share a context, each process
	 * must set the spu_subctxt_cnt and spu_subctxt_id to the same
	 * values.  The only restriction on the spu_subctxt_id is that
	 * it be unique for a given node.
	 */
	__u16 spu_subctxt_cnt;
	__u16 spu_subctxt_id;

	__u32 spu_port; /* IB port requested by user if > 0 */

	/*
	 * address of struct base_info to write to
	 */
	__u64 spu_base_info;

} __aligned(8);

/* User commands. */

/* 16 available, was: old set up userspace (for old user code) */
#define QIB_CMD_CTXT_INFO       17      /* find out what resources we got */
#define QIB_CMD_RECV_CTRL       18      /* control receipt of packets */
#define QIB_CMD_TID_UPDATE      19      /* update expected TID entries */
#define QIB_CMD_TID_FREE        20      /* free expected TID entries */
#define QIB_CMD_SET_PART_KEY    21      /* add partition key */
/* 22 available, was: return info on slave processes (for old user code) */
#define QIB_CMD_ASSIGN_CTXT     23      /* allocate HCA and ctxt */
#define QIB_CMD_USER_INIT       24      /* set up userspace */
#define QIB_CMD_UNUSED_1        25
#define QIB_CMD_UNUSED_2        26
#define QIB_CMD_PIOAVAILUPD     27      /* force an update of PIOAvail reg */
#define QIB_CMD_POLL_TYPE       28      /* set the kind of polling we want */
#define QIB_CMD_ARMLAUNCH_CTRL  29      /* armlaunch detection control */
/* 30 is unused */
#define QIB_CMD_SDMA_INFLIGHT   31      /* sdma inflight counter request */
#define QIB_CMD_SDMA_COMPLETE   32      /* sdma completion counter request */
/* 33 available, was a testing feature  */
#define QIB_CMD_DISARM_BUFS     34      /* disarm send buffers w/ errors */
#define QIB_CMD_ACK_EVENT       35      /* ack & clear bits */
#define QIB_CMD_CPUS_LIST       36      /* list of cpus allocated, for pinned
					 * processes: qib_cpus_list */

/*
 * QIB_CMD_ACK_EVENT obsoletes QIB_CMD_DISARM_BUFS, but we keep it for
 * compatibility with libraries from previous release.   The ACK_EVENT
 * will take appropriate driver action (if any, just DISARM for now),
 * then clear the bits passed in as part of the mask.  These bits are
 * in the first 64bit word at spi_sendbuf_status, and are passed to
 * the driver in the event_mask union as well.
 */
#define _QIB_EVENT_DISARM_BUFS_BIT	0
#define _QIB_EVENT_LINKDOWN_BIT		1
#define _QIB_EVENT_LID_CHANGE_BIT	2
#define _QIB_EVENT_LMC_CHANGE_BIT	3
#define _QIB_EVENT_SL2VL_CHANGE_BIT	4
#define _QIB_MAX_EVENT_BIT _QIB_EVENT_SL2VL_CHANGE_BIT

#define QIB_EVENT_DISARM_BUFS_BIT	(1UL << _QIB_EVENT_DISARM_BUFS_BIT)
#define QIB_EVENT_LINKDOWN_BIT		(1UL << _QIB_EVENT_LINKDOWN_BIT)
#define QIB_EVENT_LID_CHANGE_BIT	(1UL << _QIB_EVENT_LID_CHANGE_BIT)
#define QIB_EVENT_LMC_CHANGE_BIT	(1UL << _QIB_EVENT_LMC_CHANGE_BIT)
#define QIB_EVENT_SL2VL_CHANGE_BIT	(1UL << _QIB_EVENT_SL2VL_CHANGE_BIT)


/*
 * Poll types
 */
#define QIB_POLL_TYPE_ANYRCV     0x0
#define QIB_POLL_TYPE_URGENT     0x1

struct qib_ctxt_info {
	__u16 num_active;       /* number of active units */
	__u16 unit;             /* unit (chip) assigned to caller */
	__u16 port;             /* IB port assigned to caller (1-based) */
	__u16 ctxt;             /* ctxt on unit assigned to caller */
	__u16 subctxt;          /* subctxt on unit assigned to caller */
	__u16 num_ctxts;        /* number of ctxts available on unit */
	__u16 num_subctxts;     /* number of subctxts opened on ctxt */
	__u16 rec_cpu;          /* cpu # for affinity (ffff if none) */
};

struct qib_tid_info {
	__u32 tidcnt;
	/* make structure same size in 32 and 64 bit */
	__u32 tid__unused;
	/* virtual address of first page in transfer */
	__u64 tidvaddr;
	/* pointer (same size 32/64 bit) to __u16 tid array */
	__u64 tidlist;

	/*
	 * pointer (same size 32/64 bit) to bitmap of TIDs used
	 * for this call; checked for being large enough at open
	 */
	__u64 tidmap;
};

struct qib_cmd {
	__u32 type;                     /* command type */
	union {
		struct qib_tid_info tid_info;
		struct qib_user_info user_info;

		/*
		 * address in userspace where we should put the sdma
		 * inflight counter
		 */
		__u64 sdma_inflight;
		/*
		 * address in userspace where we should put the sdma
		 * completion counter
		 */
		__u64 sdma_complete;
		/* address in userspace of struct qib_ctxt_info to
		   write result to */
		__u64 ctxt_info;
		/* enable/disable receipt of packets */
		__u32 recv_ctrl;
		/* enable/disable armlaunch errors (non-zero to enable) */
		__u32 armlaunch_ctrl;
		/* partition key to set */
		__u16 part_key;
		/* user address of __u32 bitmask of active slaves */
		__u64 slave_mask_addr;
		/* type of polling we want */
		__u16 poll_type;
		/* back pressure enable bit for one particular context */
		__u8 ctxt_bp;
		/* qib_user_event_ack(), IPATH_EVENT_* bits */
		__u64 event_mask;
	} cmd;
};

struct qib_iovec {
	/* Pointer to data, but same size 32 and 64 bit */
	__u64 iov_base;

	/*
	 * Length of data; don't need 64 bits, but want
	 * qib_sendpkt to remain same size as before 32 bit changes, so...
	 */
	__u64 iov_len;
};

/*
 * Describes a single packet for send.  Each packet can have one or more
 * buffers, but the total length (exclusive of IB headers) must be less
 * than the MTU, and if using the PIO method, entire packet length,
 * including IB headers, must be less than the qib_piosize value (words).
 * Use of this necessitates including sys/uio.h
 */
struct __qib_sendpkt {
	__u32 sps_flags;        /* flags for packet (TBD) */
	__u32 sps_cnt;          /* number of entries to use in sps_iov */
	/* array of iov's describing packet. TEMPORARY */
	struct qib_iovec sps_iov[4];
};

/*
 * Diagnostics can send a packet by "writing" the following
 * structs to the diag data special file.
 * This allows a custom
 * pbc (+ static rate) qword, so that special modes and deliberate
 * changes to CRCs can be used. The elements were also re-ordered
 * for better alignment and to avoid padding issues.
 */
#define _DIAG_XPKT_VERS 3
struct qib_diag_xpkt {
	__u16 version;
	__u16 unit;
	__u16 port;
	__u16 len;
	__u64 data;
	__u64 pbc_wd;
};

/*
 * Data layout in I2C flash (for GUID, etc.)
 * All fields are little-endian binary unless otherwise stated
 */
#define QIB_FLASH_VERSION 2
struct qib_flash {
	/* flash layout version (QIB_FLASH_VERSION) */
	__u8 if_fversion;
	/* checksum protecting if_length bytes */
	__u8 if_csum;
	/*
	 * valid length (in use, protected by if_csum), including
	 * if_fversion and if_csum themselves)
	 */
	__u8 if_length;
	/* the GUID, in network order */
	__u8 if_guid[8];
	/* number of GUIDs to use, starting from if_guid */
	__u8 if_numguid;
	/* the (last 10 characters of) board serial number, in ASCII */
	char if_serial[12];
	/* board mfg date (YYYYMMDD ASCII) */
	char if_mfgdate[8];
	/* last board rework/test date (YYYYMMDD ASCII) */
	char if_testdate[8];
	/* logging of error counts, TBD */
	__u8 if_errcntp[4];
	/* powered on hours, updated at driver unload */
	__u8 if_powerhour[2];
	/* ASCII free-form comment field */
	char if_comment[32];
	/* Backwards compatible prefix for longer QLogic Serial Numbers */
	char if_sprefix[4];
	/* 82 bytes used, min flash size is 128 bytes */
	__u8 if_future[46];
};

/*
 * These are the counters implemented in the chip, and are listed in order.
 * The InterCaps naming is taken straight from the chip spec.
 */
struct qlogic_ib_counters {
	__u64 LBIntCnt;
	__u64 LBFlowStallCnt;
	__u64 TxSDmaDescCnt;    /* was Reserved1 */
	__u64 TxUnsupVLErrCnt;
	__u64 TxDataPktCnt;
	__u64 TxFlowPktCnt;
	__u64 TxDwordCnt;
	__u64 TxLenErrCnt;
	__u64 TxMaxMinLenErrCnt;
	__u64 TxUnderrunCnt;
	__u64 TxFlowStallCnt;
	__u64 TxDroppedPktCnt;
	__u64 RxDroppedPktCnt;
	__u64 RxDataPktCnt;
	__u64 RxFlowPktCnt;
	__u64 RxDwordCnt;
	__u64 RxLenErrCnt;
	__u64 RxMaxMinLenErrCnt;
	__u64 RxICRCErrCnt;
	__u64 RxVCRCErrCnt;
	__u64 RxFlowCtrlErrCnt;
	__u64 RxBadFormatCnt;
	__u64 RxLinkProblemCnt;
	__u64 RxEBPCnt;
	__u64 RxLPCRCErrCnt;
	__u64 RxBufOvflCnt;
	__u64 RxTIDFullErrCnt;
	__u64 RxTIDValidErrCnt;
	__u64 RxPKeyMismatchCnt;
	__u64 RxP0HdrEgrOvflCnt;
	__u64 RxP1HdrEgrOvflCnt;
	__u64 RxP2HdrEgrOvflCnt;
	__u64 RxP3HdrEgrOvflCnt;
	__u64 RxP4HdrEgrOvflCnt;
	__u64 RxP5HdrEgrOvflCnt;
	__u64 RxP6HdrEgrOvflCnt;
	__u64 RxP7HdrEgrOvflCnt;
	__u64 RxP8HdrEgrOvflCnt;
	__u64 RxP9HdrEgrOvflCnt;
	__u64 RxP10HdrEgrOvflCnt;
	__u64 RxP11HdrEgrOvflCnt;
	__u64 RxP12HdrEgrOvflCnt;
	__u64 RxP13HdrEgrOvflCnt;
	__u64 RxP14HdrEgrOvflCnt;
	__u64 RxP15HdrEgrOvflCnt;
	__u64 RxP16HdrEgrOvflCnt;
	__u64 IBStatusChangeCnt;
	__u64 IBLinkErrRecoveryCnt;
	__u64 IBLinkDownedCnt;
	__u64 IBSymbolErrCnt;
	__u64 RxVL15DroppedPktCnt;
	__u64 RxOtherLocalPhyErrCnt;