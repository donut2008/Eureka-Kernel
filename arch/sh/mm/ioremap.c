/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2000-2007 Silicon Graphics, Inc.  All Rights Reserved.
 */


#ifndef _ASM_IA64_SN_BTE_H
#define _ASM_IA64_SN_BTE_H

#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/cache.h>
#include <asm/sn/pda.h>
#include <asm/sn/types.h>
#include <asm/sn/shub_mmr.h>

#define IBCT_NOTIFY             (0x1UL << 4)
#define IBCT_ZFIL_MODE          (0x1UL << 0)

/* #define BTE_DEBUG */
/* #define BTE_DEBUG_VERBOSE */

#ifdef BTE_DEBUG
#  define BTE_PRINTK(x) printk x	/* Terse */
#  ifdef BTE_DEBUG_VERBOSE
#    define BTE_PRINTKV(x) printk x	/* Verbose */
#  else
#    define BTE_PRINTKV(x)
#  endif /* BTE_DEBUG_VERBOSE */
#else
#  define BTE_PRINTK(x)
#  define BTE_PRINTKV(x)
#endif	/* BTE_DEBUG */


/* BTE status register only supports 16 bits for length field */
#define BTE_LEN_BITS (16)
#define BTE_LEN_MASK ((1 << BTE_LEN_BITS) - 1)
#define BTE_MAX_XFER (BTE_LEN_MASK << L1_CACHE_SHIFT)


/* Define hardware */
#define BTES_PER_NODE (is_shub2() ? 4 : 2)
#define MAX_BTES_PER_NODE 4

#define BTE2OFF_CTRL	0
#define BTE2OFF_SRC	(SH2_BT_ENG_SRC_ADDR_0 - SH2_BT_ENG_CSR_0)
#define BTE2OFF_DEST	(SH2_BT_ENG_DEST_ADDR_0 - SH2_BT_ENG_CSR_0)
#define BTE2OFF_NOTIFY	(SH2_BT_ENG_NOTIF_ADDR_0 - SH2_BT_ENG_CSR_0)

#define BTE_BASE_ADDR(interface) 				\
    (is_shub2() ? (interface == 0) ? SH2_BT_ENG_CSR_0 :		\
		  (interface == 1) ? SH2_BT_ENG_CSR_1 :		\
		  (interface == 2) ? SH2_BT_ENG_CSR_2 :		\
		  		     SH2_BT_ENG_CSR_3 		\
		: (interface == 0) ? IIO_IBLS0 : IIO_IBLS1)

#define BTE_SOURCE_ADDR(base)					\
    (is_shub2() ? base + (BTE2OFF_SRC/8) 			\
		: base + (BTEOFF_SRC/8))

#define BTE_DEST_ADDR(base)					\
    (is_shub2() ? base + (BTE2OFF_DEST/8) 			\
		: base + (BTEOFF_DEST/8))

#define BTE_CTRL_ADDR(base)					\
    (is_shub2() ? base + (BTE2OFF_CTRL/8) 			\
		: base + (BTEOFF_CTRL/8))

#define BTE_NOTIF_ADDR(base)					\
    (is_shub2() ? base + (BTE2OFF_NOTIFY/8) 			\
		: base + (BTEOFF_NOTIFY/8))

/* Define hardware modes */
#define BTE_NOTIFY IBCT_NOTIFY
#define BTE_NORMAL BTE_NOTIFY
#define BTE_ZERO_FILL (BTE_NOTIFY | IBCT_ZFIL_MODE)
/* Use a reserved bit to let the caller specify a wait for any BTE */
#define BTE_WACQUIRE 0x4000
/* Use the BTE on the node with the destination memory */
#define BTE_USE_DEST (BTE_WACQUIRE << 1)
/* Use any available BTE interface on any node for the transfer */
#define BTE_USE_ANY (BTE_USE_DEST << 1)
/* macro to force the IBCT0 value valid */
#define BTE_VALID_MODE(x) ((x) & (IBCT_NOTIFY | IBCT_ZFIL_MODE))

#define BTE_ACTIVE		(IBLS_BUSY | IBLS_ERROR)
#define BTE_WORD_AVAILABLE	(IBLS_BUSY << 1)
#define BTE_WORD_BUSY		(~BTE_WORD_AVAILABLE)

/*
 * Some macros to simplify reading.
 * Start with macros to locate the BTE control registers.
 */
#define BTE_LNSTAT_LOAD(_bte)						\
			HUB_L(_bte->bte_base_addr)
#define BTE_LNSTAT_STORE(_bte, _x)					\
			HUB_S(_bte->bte_base_addr, (_x))
#define BTE_SRC_STORE(_bte, _x)						\
({									\
		u64 __addr = ((_x) & ~AS_MASK);				\
		if (is_shub2()) 					\
			__addr = SH2_TIO_PHYS_TO_DMA(__addr);		\
		HUB_S(_bte->bte_source_addr, __addr);			\
})
#define BTE_DEST_STORE(_bte, _x)