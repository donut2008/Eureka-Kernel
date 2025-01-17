/* This file is part of the Emulex RoCE Device Driver for
 * RoCE (RDMA over Converged Ethernet) adapters.
 * Copyright (C) 2012-2015 Emulex. All rights reserved.
 * EMULEX and SLI are trademarks of Emulex.
 * www.emulex.com
 *
 * This software is available to you under a choice of one of two licenses.
 * You may choose to be licensed under the terms of the GNU General Public
 * License (GPL) Version 2, available from the file COPYING in the main
 * directory of this source tree, or the BSD license below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contact Information:
 * linux-drivers@emulex.com
 *
 * Emulex
 * 3333 Susan Street
 * Costa Mesa, CA 92626
 */

#ifndef __OCRDMA_VERBS_H__
#define __OCRDMA_VERBS_H__

int ocrdma_post_send(struct ib_qp *, struct ib_send_wr *,
		     struct ib_send_wr **bad_wr);
int ocrdma_post_recv(struct ib_qp *, struct ib_recv_wr *,
		     struct ib_recv_wr **bad_wr);

int ocrdma_poll_cq(struct ib_cq *, int num_entries, struct ib_wc *wc);
int ocrdma_arm_cq(struct ib_cq *, enum ib_cq_notify_flags flags);

int ocrdma_query_device(struct ib_device *, struct ib_device_attr *props,
			struct ib_udata *uhw);
int ocrdma_query_port(struct ib_device *, u8 port, struct ib_port_attr *props);
int ocrdma_modify_port(struct ib_device *, u8 port, int mask,
		       struct ib_port_modify *props);

enum rdma_protocol_type
ocrdma_query_protocol(struct ib_device *device, u8 port_num);

void ocrdma_get_guid(struct ocrdma_dev *, u8 *guid);
int ocrdma_query_gid(struct ib_device *, u8 port,
		     int index, union ib_gid *gid);
struct net_device *ocrdma_get_netdev(struct ib_device *device, u8 port_num);
int ocrdma_add_gid(struct ib_device *device,
		   u8 port_num,
		   unsigned int index,
		   const union ib_gid *gid,
		   const struct ib_gid_attr *attr,
		   void **context);
int  ocrdma_del_gid(struct ib_device *device,
		    u8 port_num,
		    unsigned int index,
		    void **context);
int ocrdma_query_pkey(struct ib_device *, u8 port, u16 index, u16 *pkey);

struct ib_ucontext *ocrdma_alloc_ucontext(struct ib_device *,
					  struct ib_udata *);
int ocrdma_dealloc_ucontext(struct ib_ucontext *);

int ocrdma_mmap(struct ib_ucontext *, struct vm_area_struct *vma);

struct ib_pd *ocrdma_alloc_pd(struct ib_device *,
			      struct ib_ucontext *, struct ib_udata *);
int ocrdma_dealloc_pd(struct ib_pd *pd);

struct ib_cq *ocrdma_create_cq(struct ib_device *ibdev,
			       const struct ib_cq_init_attr *attr,
			       struct ib_ucontext *ib_ctx,
			       struct ib_udata *udata);
int ocrdma_resize_cq(struct ib_cq *, int cqe, struct ib_udata *);
int ocrdma_destroy_cq(struct ib_cq *);

struct ib_qp *ocrdma_create_qp(struct ib_pd *,
			       struct ib_qp_init_attr *attrs,
			       struct ib_udata *);
int _ocrdma_modify_qp(struct ib_qp *, struct ib_qp_attr *attr,
		      int attr_mask);
int ocrdma_modify_qp(struct ib_qp *, struct ib_qp_attr *attr,
		     int attr_mask, struct ib_udata *udata);
int ocrdma_query_qp(struct ib_qp *,
		    struct ib_qp_attr *qp_attr,
		    int qp_attr_mask, struct ib_qp_init_attr *);
int ocrdma_destroy_qp(struct ib_qp *);
void ocrdma_del_flush_qp(struct ocrdma_qp *qp);

struct ib_srq *ocrdma_create_srq(struct ib_pd *, struct ib_srq_init_attr *,
				 struct ib_udata *);
int ocrdma_modify_srq(struct ib_srq *, struct ib_srq_attr *,
		      enum ib_srq_attr_mask, struct ib_udata *);
int ocrdma_query_srq(struct ib_srq *, struct ib_srq_attr *);
int ocrdma_destroy_srq(struct ib_srq *);
int ocrdma_post_srq_recv(struct ib_srq *, struct ib_recv_wr *,
			 struct ib_recv_wr **bad_recv_wr);

int ocrdma_dereg_mr(struct ib_mr *);
struct ib_mr *ocrdma_get_dma_mr(struct ib_pd *, int acc);
struct ib_mr *ocrdma_reg_kernel_mr(struct ib_pd *,
				   struct ib_phys_buf *buffer_list,
				   int num_phys_buf, int acc, u64 *iova_start);
struct ib_mr *ocrdma_reg_user_mr(struct ib_pd *, u64 start, u64 length,
				 u64 virt, int acc, struct ib_udata *);
struct ib_mr *ocrdma_alloc_mr(struct ib_pd *pd,
			      enum ib_mr_type mr_type,
			      u32 max_num_sg);
int ocrdma_map_mr_sg(struct ib_mr *ibmr,
		     struct scatterlist *sg,
		     int sg_nents);

#endif				/* __OCRDMA_VERBS_H__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     config INFINIBAND_QIB
	tristate "Intel PCIe HCA support"
	depends on 64BIT
	---help---
	This is a low-level driver for Intel PCIe QLE InfiniBand host
	channel adapters.  This driver does not support the Intel
	HyperTransport card (model QHT7140).

config INFINIBAND_QIB_DCA
	bool "QIB DCA support"
	depends on INFINIBAND_QIB && DCA && SMP && !(INFINIBAND_QIB=y && DCA=m)
	default y
	---help---
	Setting this enables DCA support on some Intel chip sets
	with the iba7322 HCA.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     obj-$(CONFIG_INFINIBAND_QIB) += ib_qib.o

ib_qib-y := qib_cq.o qib_diag.o qib_dma.o qib_driver.o qib_eeprom.o \
	qib_file_ops.o qib_fs.o qib_init.o qib_intr.o qib_keys.o \
	qib_mad.o qib_mmap.o qib_mr.o qib_pcie.o qib_pio_copy.o \
	qib_qp.o qib_qsfp.o qib_rc.o qib_ruc.o qib_sdma.o qib_srq.o \
	qib_sysfs.o qib_twsi.o qib_tx.o qib_uc.o qib_ud.o \
	qib_user_pages.o qib_user_sdma.o qib_verbs_mcast.o qib_iba7220.o \
	qib_sd7220.o qib_iba7322.o qib_verbs.o

# 6120 has no fallback if no MSI interrupts, others can do INTx
ib_qib-$(CONFIG_PCI_MSI) += qib_iba6120.o

ib_qib-$(CONFIG_X86_64) += qib_wc_x86_64.o
ib_qib-$(CONFIG_PPC64) += qib_wc_ppc64.o
ib_qib-$(CONFIG_DEBUG_FS) += qib_debugfs.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              #ifndef _QIB_KERNEL_H
#define _QIB_KERNEL_H
/*
 * Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * This header file is the base header file for qlogic_ib kernel code
 * qib_user.h serves a similar purpose for user code.
 */

#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/kref.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "qib_common.h"
#include "qib_verbs.h"

/* only s/w major version of QLogic_IB we can handle */
#define QIB_CHIP_VERS_MAJ 2U

/* don't care about this except printing */
#define QIB_CHIP_VERS_MIN 0U

/* The Organization Unique Identifier (Mfg code), and its position in GUID */
#define QIB_OUI 0x001175
#define QIB_OUI_LSB 40

/*
 * per driver stats, either not device nor port-specific, or
 * summed over all of the devices and ports.
 * They are described by name via ipathfs filesystem, so layout
 * and number of elements can change without breaking compatibility.
 * If members are added or deleted qib_statnames[] in qib_fs.c must
 * change to match.
 */
struct qlogic_ib_stats {
	__u64 sps_ints; /* number of interrupts handled */
	__u64 sps_errints; /* number of error interrupts */
	__u64 sps_txerrs; /* tx-related packet errors */
	__u64 sps_rcverrs; /* non-crc rcv packet errors */
	__u64 sps_hwerrs; /* hardware errors reported (parity, etc.) */
	__u64 sps_nopiobufs; /* no pio bufs avail from kernel */
	__u64 sps_ctxts; /* number of contexts currently open */
	__u64 sps_lenerrs; /* number of kernel packets where RHF != LRH len */
	__u64 sps_buffull;
	__u64 sps_hdrfull;
};

extern struct qlogic_ib_stats qib_stats;
extern const struct pci_error_handlers qib_pci_err_handler;

#define QIB_CHIP_SWVERSION QIB_CHIP_VERS_MAJ
/*
 * First-cut critierion for "device is active" is
 * two thousand dwords combined Tx, Rx traffic per
 * 5-second interval. SMA packets are 64 dwords,
 * and occur "a few per second", presumably each way.
 */
#define QIB_TRAFFIC_ACTIVE_THRESHOLD (2000)

/*
 * Struct used to indicate which errors are logged in each of the
 * error-counters that are logged to EEPROM. A counter is incremented
 * _once_ (saturating at 255) for each event with any bits set in
 * the error or hwerror register masks below.
 */
#define QIB_EEP_LOG_CNT (4)
struct qib_eep_log_mask {
	u64 errs_to_log;
	u64 hwerrs_to_log;
};

/*
 * Below contains all data related to a single context (formerly called port).
 */

#ifdef CONFIG_DEBUG_FS
struct qib_opcode_stats_perctx;
#endif

struct qib_ctxtdata {
	void **rcvegrbuf;
	dma_addr_t *rcvegrbuf_phys;
	/* rcvhdrq base, needs mmap before useful */
	void *rcvhdrq;
	/* kernel virtual address where hdrqtail is updated */
	void *rcvhdrtail_kvaddr;
	/*
	 * temp buffer for expected send setup, allocated at open, instead
	 * of each setup call
	 */
	void *tid_pg_list;
	/*
	 * Shared page for kernel to signal user processes that send buffers
	 * need disarming.  The process should call QIB_CMD_DISARM_BUFS
	 * or QIB_CMD_ACK_EVENT with IPATH_EVENT_DISARM_BUFS set.
	 */
	unsigned long *user_event_mask;
	/* when waiting for rcv or pioavail */
	wait_queue_head_t wait;
	/*
	 * rcvegr bufs base, physical, must fit
	 * in 44 bits so 32 bit programs mmap64 44 bit works)
	 */
	dma_addr_t rcvegr_phys;
	/* mmap of hdrq, must fit in 44 bits */
	dma_addr_t rcvhdrq_phys;
	dma_addr_t rcvhdrqtailaddr_phys;

	/*
	 * number of opens (including slave sub-contexts) on this instance
	 * (ignoring forks, dup, etc. for now)
	 */
	int cnt;
	/*
	 * how much space to leave at start of eager TID entries for
	 * protocol use, on each TID
	 */
	/* instead of calculating it */
	unsigned ctxt;
	/* local node of context */
	int node_id;
	/* non-zero if ctxt is being shared. */
	u16 subctxt_cnt;
	/* non-zero if ctxt is being shared. */
	u16 subctxt_id;
	/* number of eager TID entries. */
	u16 rcvegrcnt;
	/* index of first eager TID entry. */
	u16 rcvegr_tid_base;
	/* number of pio bufs for this ctxt (all procs, if shared) */
	u32 piocnt;
	/* first pio buffer for this ctxt */
	u32 pio_base;
	/* chip offset of PIO buffers for this ctxt */
	u32 piobufs;
	/* how many alloc_pages() chunks in rcvegrbuf_pages */
	u32 rcvegrbuf_chunks;
	/* how many egrbufs per chunk */
	u16 rcvegrbufs_perchunk;
	/* ilog2 of above */
	u16 rcvegrbufs_perchunk_shift;
	/* order for rcvegrbuf_pages */
	size_t rcvegrbuf_size;
	/* rcvhdrq size (for freeing) */
	size_t rcvhdrq_size;
	/* per-context flags for fileops/intr communication */
	unsigned long flag;
	/* next expected TID to check when looking for free */
	u32 tidcursor;
	/* WAIT_RCV that timed out, no interrupt */
	u32 rcvwait_to;
	/* WAIT_PIO that timed out, no interrupt */
	u32 piowait_to;
	/* WAIT_RCV already happened, no wait */
	u32 rcvnowait;
	/* WAIT_PIO already happened, no wait */
	u32 pionowait;
	/* total number of polled urgent packets */
	u32 urgent;
	/* saved total number of polled urgent packets for poll edge trigger */
	u32 urgent_poll;
	/* pid of process using this ctxt */
	pid_t pid;
	pid_t subpid[QLOGIC_IB_MAX_SUBCTXT];
	/* same size as task_struct .comm[], command that opened context */
	char comm[16];
	/* pkeys set by this use of this ctxt */
	u16 pkeys[4];
	/* so file ops can get at unit */
	struct qib_devdata *dd;
	/* so funcs that need physical port can get it easily */
	struct qib_pportdata *ppd;
	/* A page of memory for rcvhdrhead, rcvegrhead, rcvegrtail * N */
	void *subctxt_uregbase;
	/* An array of pages for the eager receive buffers * N */
	void *subctxt_rcvegrbuf;
	/* An array of pages for the eager header queue entries * N */
	void *subctxt_rcvhdr_base;
	/* The version of the library which opened this ctxt */
	u32 userversion;
	/* Bitmask of active slaves */
	u32 active_slaves;
	/* Type of packets or conditions we want to poll for */
	u16 poll_type;
	/* receive packet sequence counter */
	u8 seq_cnt;
	u8 redirect_seq_cnt;
	/* ctxt rcvhdrq head offset */
	u32 head;
	/* lookaside fields */
	struct qib_qp *lookaside_qp;
	u32 lookaside_qpn;
	/* QPs waiting for context processing */
	struct list_head qp_wait_list;
#ifdef CONFIG_DEBUG_FS
	/* verbs stats per CTX */
	struct qib_opcode_stats_perctx *opstats;
#endif
};

struct qib_sge_state;

struct qib_sdma_txreq {
	int                 flags;
	int                 sg_count;
	dma_addr_t          addr;
	void              (*callback)(struct qib_sdma_txreq *, int);
	u16                 start_idx;  /* sdma private */
	u16                 next_descq_idx;  /* sdma private */
	struct list_head    list;       /* sdma private */
};

struct qib_sdma_desc {
	__le64 qw[2];
};

struct qib_verbs_txreq {
	struct qib_sdma_txreq   txreq;
	struct qib_qp           *qp;
	struct qib_swqe         *wqe;
	u32                     dwords;
	u16                     hdr_dwords;
	u16                     hdr_inx;
	struct qib_pio_header	*align_buf;
	struct qib_mregion	*mr;
	struct qib_sge_state    *ss;
};

#define QIB_SDMA_TXREQ_F_USELARGEBUF  0x1
#define QIB_SDMA_TXREQ_F_HEADTOHOST   0x2
#define QIB_SDMA_TXREQ_F_INTREQ       0x4
#define QIB_SDMA_TXREQ_F_FREEBUF      0x8
#define QIB_SDMA_TXREQ_F_FREEDESC     0x10

#define QIB_SDMA_TXREQ_S_OK        0
#define QIB_SDMA_TXREQ_S_SENDERROR 1
#define QIB_SDMA_TXREQ_S_ABORTED   2
#define QIB_SDMA_TXREQ_S_SHUTDOWN  3

/*
 * Get/Set IB link-level config parameters for f_get/set_ib_cfg()
 * Mostly for MADs that set or query link parameters, also ipath
 * config interfaces
 */
#define QIB_IB_CFG_LIDLMC 0 /* LID (LS16b) and Mask (MS16b) */
#define QIB_IB_CFG_LWID_ENB 2 /* allowed Link-width */
#define QIB_IB_CFG_LWID 3 /* currently active Link-width */
#define QIB_IB_CFG_SPD_ENB 4 /* allowed Link speeds */
#define QIB_IB_CFG_SPD 5 /* current Link spd */
#define QIB_IB_CFG_RXPOL_ENB 6 /* Auto-RX-polarity enable */
#define QIB_IB_CFG_LREV_ENB 7 /* Auto-Lane-reversal enable */
#define QIB_IB_CFG_LINKLATENCY 8 /* Link Latency (IB1.2 only) */
#define QIB_IB_CFG_HRTBT 9 /* IB heartbeat off/enable/auto; DDR/QDR only */
#define QIB_IB_CFG_OP_VLS 10 /* operational VLs */
#define QIB_IB_CFG_VL_HIGH_CAP 11 /* num of VL high priority weights */
#define QIB_IB_CFG_VL_LOW_CAP 12 /* num of VL low priority weights */
#define QIB_IB_CFG_OVERRUN_THRESH 13 /* IB overrun threshold */
#define QIB_IB_CFG_PHYERR_THRESH 14 /* IB PHY error threshold */
#define QIB_IB_CFG_LINKDEFAULT 15 /* IB link default (sleep/poll) */
#define QIB_IB_CFG_PKEYS 16 /* update partition keys */
#define QIB_IB_CFG_MTU 17 /* update MTU in IBC */
#define QIB_IB_CFG_LSTATE 18 /* update linkcmd and linkinitcmd in IBC */
#define QIB_IB_CFG_VL_HIGH_LIMIT 19
#define QIB_IB_CFG_PMA_TICKS 20 /* PMA sample tick resolution */
#define QIB_IB_CFG_PORT 21 /* switch port we are connected to */

/*
 * for CFG_LSTATE: LINKCMD in upper 16 bits, LINKINITCMD in lower 16
 * IB_LINKINITCMD_POLL and SLEEP are also used as set/get values for
 * QIB_IB_CFG_LINKDEFAULT cmd
 */
#define   IB_LINKCMD_DOWN   (0 << 16)
#define   IB_LINKCMD_ARMED  (1 << 16)
#define   IB_LINKCMD_ACTIVE (2 << 16)
#define   IB_LINKINITCMD_NOP     0
#define   IB_LINKINITCMD_POLL    1
#define   IB_LINKINITCMD_SLEEP   2
#define   IB_LINKINITCMD_DISABLE 3

/*
 * valid states passed to qib_set_linkstate() user call
 */
#define QIB_IB_LINKDOWN         0
#define QIB_IB_LINKARM          1
#define QIB_IB_LINKACTIVE       2
#define QIB_IB_LINKDOWN_ONLY    3
#define QIB_IB_LINKDOWN_SLEEP   4
#define QIB_IB_LINKDOWN