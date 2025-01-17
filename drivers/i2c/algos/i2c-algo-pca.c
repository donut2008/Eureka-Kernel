 QIB_6120_XGXSCfg_polarity_inv_LSB 0x13
#define QIB_6120_XGXSCfg_polarity_inv_RMASK 0xF
#define QIB_6120_XGXSCfg_link_sync_mask_LSB 0x9
#define QIB_6120_XGXSCfg_link_sync_mask_RMASK 0x3FF
#define QIB_6120_XGXSCfg_port_addr_LSB 0x4
#define QIB_6120_XGXSCfg_port_addr_RMASK 0x1F
#define QIB_6120_XGXSCfg_mdd_30_LSB 0x3
#define QIB_6120_XGXSCfg_mdd_30_RMASK 0x1
#define QIB_6120_XGXSCfg_xcv_resetn_LSB 0x2
#define QIB_6120_XGXSCfg_xcv_resetn_RMASK 0x1
#define QIB_6120_XGXSCfg_Reserved1_LSB 0x1
#define QIB_6120_XGXSCfg_Reserved1_RMASK 0x1
#define QIB_6120_XGXSCfg_tx_rx_resetn_LSB 0x0
#define QIB_6120_XGXSCfg_tx_rx_resetn_RMASK 0x1

#define QIB_6120_LBIntCnt_OFFS 0x12000

#define QIB_6120_LBFlowStallCnt_OFFS 0x12008

#define QIB_6120_TxUnsupVLErrCnt_OFFS 0x12018

#define QIB_6120_TxDataPktCnt_OFFS 0x12020

#define QIB_6120_TxFlowPktCnt_OFFS 0x12028

#define QIB_6120_TxDwordCnt_OFFS 0x12030

#define QIB_6120_TxLenErrCnt_OFFS 0x12038

#define QIB_6120_TxMaxMinLenErrCnt_OFFS 0x12040

#define QIB_6120_TxUnderrunCnt_OFFS 0x12048

#define QIB_6120_TxFlowStallCnt_OFFS 0x12050

#define QIB_6120_TxDroppedPktCnt_OFFS 0x12058

#define QIB_6120_RxDroppedPktCnt_OFFS 0x12060

#define QIB_6120_RxDataPktCnt_OFFS 0x12068

#define QIB_6120_RxFlowPktCnt_OFFS 0x12070

#define QIB_6120_RxDwordCnt_OFFS 0x12078

#define QIB_6120_RxLenErrCnt_OFFS 0x12080

#define QIB_6120_RxMaxMinLenErrCnt_OFFS 0x12088

#define QIB_6120_RxICRCErrCnt_OFFS 0x12090

#define QIB_6120_RxVCRCErrCnt_OFFS 0x12098

#define QIB_6120_RxFlowCtrlErrCnt_OFFS 0x120A0

#define QIB_6120_RxBadFormatCnt_OFFS 0x120A8

#define QIB_6120_RxLinkProblemCnt_OFFS 0x120B0

#define QIB_6120_RxEBPCnt_OFFS 0x120B8

#define QIB_6120_RxLPCRCErrCnt_OFFS 0x120C0

#define QIB_6120_RxBufOvflCnt_OFFS 0x120C8

#define QIB_6120_RxTIDFullErrCnt_OFFS 0x120D0

#define QIB_6120_RxTIDValidErrCnt_OFFS 0x120D8

#define QIB_6120_RxPKeyMismatchCnt_OFFS 0x120E0

#define QIB_6120_RxP0HdrEgrOvflCnt_OFFS 0x120E8

#define QIB_6120_IBStatusChangeCnt_OFFS 0x12140

#define QIB_6120_IBLinkErrRecoveryCnt_OFFS 0x12148

#define QIB_6120_IBLinkDownedCnt_OFFS 0x12150

#define QIB_6120_IBSymbolErrCnt_OFFS 0x12158

#define QIB_6120_PcieRetryBufDiagQwordCnt_OFFS 0x12170

#define QIB_6120_RcvEgrArray0_OFFS 0x14000

#define QIB_6120_RcvTIDArray0_OFFS 0x54000

#define QIB_6120_PIOLaunchFIFO_OFFS 0x64000

#define QIB_6120_SendPIOpbcCache_OFFS 0x64800

#define QIB_6120_RcvBuf1_OFFS 0x72000

#define QIB_6120_RcvBuf2_OFFS 0x75000

#define QIB_6120_RcvFlags_OFFS 0x77000

#define QIB_6120_RcvLookupBuf1_OFFS 0x79000

#define QIB_6120_RcvDMABuf_OFFS 0x7B000

#define QIB_6120_MiscRXEIntMem_OFFS 0x7C000

#define QIB_6120_PCIERcvBuf_OFFS 0x80000

#define QIB_6120_PCIERetryBuf_OFFS 0x82000

#define QIB_6120_PCIERcvBufRdToWrAddr_OFFS 0x84000

#define QIB_6120_PIOBuf0_MA_OFFS 0x100000
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           #ifndef _QIB_7220_H
#define _QIB_7220_H
/*
 * Copyright (c) 2007, 2009, 2010 QLogic Corporation. All rights reserved.
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

/* grab register-defs auto-generated by HW */
#include "qib_7220_regs.h"

/* The number of eager receive TIDs for context zero. */
#define IBA7220_KRCVEGRCNT      2048U

#define IB_7220_LT_STATE_CFGRCVFCFG      0x09
#define IB_7220_LT_STATE_CFGWAITRMT      0x0a
#define IB_7220_LT_STATE_TXREVLANES      0x0d
#define IB_7220_LT_STATE_CFGENH          0x10

struct qib_chip_specific {
	u64 __iomem *cregbase;
	u64 *cntrs;
	u64 *portcntrs;
	spinlock_t sdepb_lock; /* serdes EPB bus */
	spinlock_t rcvmod_lock; /* protect rcvctrl shadow changes */
	spinlock_t gpio_lock; /* RMW of shadows/regs for ExtCtrl and GPIO */
	u64 hwerrmask;
	u64 errormask;
	u64 gpio_out; /* shadow of kr_gpio_out, for rmw ops */
	u64 gpio_mask; /* shadow the gpio mask register */
	u64 extctrl; /* shadow the gpio output enable, etc... */
	u32 ncntrs;
	u32 nportcntrs;
	u32 cntrnamelen;
	u32 portcntrnamelen;
	u32 numctxts;
	u32 rcvegrcnt;
	u32 autoneg_tries;
	u32 serdes_first_init_done;
	u32 sdmabufcnt;
	u32 lastbuf_for_pio;
	u32 updthresh; /* current AvailUpdThld */
	u32 updthresh_dflt; /* default AvailUpdThld */
	int irq;
	u8 presets_needed;
	u8 relock_timer_active;
	char emsgbuf[128];
	char sdmamsgbuf[192];
	char bitsmsgbuf[64];
	struct timer_list relock_timer;
	unsigned int relock_interval; /* in jiffies */
};

struct qib_chippport_specific {
	struct qib_pportdata pportdata;
	wait_queue_head_t autoneg_wait;
	struct delayed_work autoneg_work;
	struct timer_list chase_timer;
	/*
	 * these 5 fields are used to establish deltas for IB symbol
	 * errors and linkrecovery errors.  They can be reported on
	 * some chips during link negotiation prior to INIT, and with
	 * DDR when faking DDR negotiations with non-IBTA switches.
	 * The chip counters are adjusted at driver unload if there is
	 * a non-zero delta.
	 */
	u64 ibdeltainprog;
	u64 ibsymdelta;
	u64 ibsymsnap;
	u64 iblnkerrdelta;
	u64 iblnkerrsnap;
	u64 ibcctrl; /* kr_ibcctrl shadow */
	u64 ibcddrctrl; /* kr_ibcddrctrl shadow */
	unsigned long chase_end;
	u32 last_delay_mult;
};

/*
 * This header file provides the declarations and common definitions
 * for (mostly) manipulation of the SerDes blocks within the IBA7220.
 * the functions declared should only be called from within other
 * 7220-related files such as qib_iba7220.c or qib_sd7220.c.
 */
int qib_sd7220_presets(struct qib_devdata *dd);
int qib_sd7220_init(struct qib_devdata *dd);
void qib_sd7220_clr_ibpar(struct qib_devdata *);
/*
 * Below used for sdnum parameter, selecting one of the two sections
 * used for PCIe, or the single SerDes used for IB, which is the
 * only one currently used
 */
#define IB_7220_SERDES 2

static inline u32 qib_read_kreg32(const struct qib_devdata *dd,
				  const u16 regno)
{
	if (!dd->kregbase || !(dd->flags & QIB_PRESENT))
		return -1;
	return readl((u32 __iomem *)&dd->kregbase[regno]);
}

static inline u64 qib_read_kreg64(const struct qib_devdata *dd,
				  const u16 regno)
{
	if (!dd->kregbase || !(dd->flags & QIB_PRESENT))
		return -1;

	return readq(&dd->kregbase[regno]);
}

static inline void qib_write_kreg(const struct qib_devdata *dd,
				  const u16 regno, u64 value)
{
	if (dd->kregbase)
		writeq(value, &dd->kregbase[regno]);
}

void set_7220_relock_poll(struct qib_devdata *, int);
void shutdown_7220_relock_poll(struct qib_devdata *);
void toggle_7220_rclkrls(struct qib_devdata *);


#endif /* _QIB_7220_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * Copyright (c) 2006, 2007, 2008, 2009, 2010 QLogic Corporation.
 * All rights reserved.
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

#ifndef _QIB_COMMON_H
#define _QIB_COMMON_H

/*
 * This file contains defines, structures, etc. that are used
 * to communicate between kernel and user code.
 */

/* This is the IEEE-assigned OUI for QLogic Inc. QLogic_IB */
#define QIB_SRC_OUI_1 0x00
#define QIB_SRC_OUI_2 0x11
#define QIB_SRC_OUI_3 0x75

/* version of protocol header (known to chip also). In the long run,
 * we should be able to generate and accept a range of version numbers;
 * for now we only accept one, and it's compiled in.
 */
#define IPS_PROTO_VERSION 2

/*
 * These are compile time constants that you may want to enable or disable
 * if you are trying to debug problems with code or performance.
 * QIB_VERBOSE_TRACING define as 1 if you want additional tracing in
 * fastpath code
 * QIB_TRACE_REGWRITES define as 1 if you want register writes to be
 * traced in faspath code
 * _QIB_TRACING define as 0 if you want to remove all tracing in a
 * compilation unit
 */

/*
 * The value in the BTH QP field that QLogic_IB uses to differentiate
 * an qlogic_ib protocol IB packet vs standard IB transport
 * This it needs to be even (0x656b78), because the LSB is sometimes
 * used for the MSB of context. The change may cause a problem
 * interoperating with older software.
 */
#define QIB_KD_QP 0x656b78

/*
 * These are the status bits readable (in ascii form, 64bit value)
 * from the "status" sysfs file.  For binary compatibility, values
 * must remain as is; removed states can be reused for different
 * purposes.
 */
#define QIB_STATUS_INITTED       0x1    /* basic initialization done */
/* Chip has been found and initted */
#define QIB_STATUS_CHIP_PRESENT 0x20
/* IB link is at ACTIVE, usable for data traffic */
#define QI