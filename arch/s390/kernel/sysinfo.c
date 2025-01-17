adm_error_mask and ce_adm_error_summary register bit masks */
#define CE_ADM_ERR_CRM_SSP_REQ_INVALID			(0x1ULL <<  0)
#define CE_ADM_ERR_SSP_REQ_HEADER			(0x1ULL <<  1)
#define CE_ADM_ERR_SSP_RSP_HEADER			(0x1ULL <<  2)
#define CE_ADM_ERR_SSP_PROTOCOL_ERROR			(0x1ULL <<  3)
#define CE_ADM_ERR_SSP_SBE				(0x1ULL <<  4)
#define CE_ADM_ERR_SSP_MBE				(0x1ULL <<  5)
#define CE_ADM_ERR_CXM_CREDIT_OFLOW			(0x1ULL <<  6)
#define CE_ADM_ERR_DRE_SSP_REQ_INVAL			(0x1ULL <<  7)
#define CE_ADM_ERR_SSP_REQ_LONG				(0x1ULL <<  8)
#define CE_ADM_ERR_SSP_REQ_OFLOW			(0x1ULL <<  9)
#define CE_ADM_ERR_SSP_REQ_SHORT			(0x1ULL << 10)
#define CE_ADM_ERR_SSP_REQ_SIDEBAND			(0x1ULL << 11)
#define CE_ADM_ERR_SSP_REQ_ADDR_ERR			(0x1ULL << 12)
#define CE_ADM_ERR_SSP_REQ_BAD_BE			(0x1ULL << 13)
#define CE_ADM_ERR_PCIE_COMPL_TIMEOUT			(0x1ULL << 14)
#define CE_ADM_ERR_PCIE_UNEXP_COMPL			(0x1ULL << 15)
#define CE_ADM_ERR_PCIE_ERR_COMPL			(0x1ULL << 16)
#define CE_ADM_ERR_DRE_CREDIT_OFLOW			(0x1ULL << 17)
#define CE_ADM_ERR_DRE_SRAM_PE				(0x1ULL << 18)
#define CE_ADM_ERR_SSP_RSP_INVALID			(0x1ULL << 19)
#define CE_ADM_ERR_SSP_RSP_LONG				(0x1ULL << 20)
#define CE_ADM_ERR_SSP_RSP_SHORT			(0x1ULL << 21)
#define CE_ADM_ERR_SSP_RSP_SIDEBAND			(0x1ULL << 22)
#define CE_ADM_ERR_URE_SSP_RSP_UNEXP			(0x1ULL << 23)
#define CE_ADM_ERR_URE_SSP_WR_REQ_TIMEOUT		(0x1ULL << 24)
#define CE_ADM_ERR_URE_SSP_RD_REQ_TIMEOUT		(0x1ULL << 25)
#define CE_ADM_ERR_URE_ATE3240_PAGE_FAULT		(0x1ULL << 26)
#define CE_ADM_ERR_URE_ATE40_PAGE_FAULT			(0x1ULL << 27)
#define CE_ADM_ERR_URE_CREDIT_OFLOW			(0x1ULL << 28)
#define CE_ADM_ERR_URE_SRAM_PE				(0x1ULL << 29)
#define CE_ADM_ERR_ADM_SSP_RSP_UNEXP			(0x1ULL << 30)
#define CE_ADM_ERR_ADM_SSP_REQ_TIMEOUT			(0x1ULL << 31)
#define CE_ADM_ERR_MMR_ACCESS_ERROR			(0x1ULL << 32)
#define CE_ADM_ERR_MMR_ADDR_ERROR			(0x1ULL << 33)
#define CE_ADM_ERR_ADM_CREDIT_OFLOW			(0x1ULL << 34)
#define CE_ADM_ERR_ADM_SRAM_PE				(0x1ULL << 35)
#define CE_ADM_ERR_DTL1_MIN_PDATA_CREDIT_ERR		(0x1ULL << 36)
#define CE_ADM_ERR_DTL1_INF_COMPL_CRED_UPDT_ERR		(0x1ULL << 37)
#define CE_ADM_ERR_DTL1_INF_POSTED_CRED_UPDT_ERR	(0x1ULL << 38)
#define CE_ADM_ERR_DTL1_INF_NPOSTED_CRED_UPDT_ERR	(0x1ULL << 39)
#define CE_ADM_ERR_DTL1_COMP_HD_CRED_MAX_ERR		(0x1ULL << 40)
#define CE_ADM_ERR_DTL1_COMP_D_CRED_MAX_ERR		(0x1ULL << 41)
#define CE_ADM_ERR_DTL1_NPOSTED_HD_CRED_MAX_ERR		(0x1ULL << 42)
#define CE_ADM_ERR_DTL1_NPOSTED_D_CRED_MAX_ERR		(0x1ULL << 43)
#define CE_ADM_ERR_DTL1_POSTED_HD_CRED_MAX_ERR		(0x1ULL << 44)
#define CE_ADM_ERR_DTL1_POSTED_D_CRED_MAX_ERR		(0x1ULL << 45)
#define CE_ADM_ERR_DTL2_MIN_PDATA_CREDIT_ERR		(0x1ULL << 46)
#define CE_ADM_ERR_DTL2_INF_COMPL_CRED_UPDT_ERR		(0x1ULL << 47)
#define CE_ADM_ERR_DTL2_INF_POSTED_CRED_UPDT_ERR	(0x1ULL << 48)
#define CE_ADM_ERR_DTL2_INF_NPOSTED_CRED_UPDT_ERR	(0x1ULL << 49)
#define CE_ADM_ERR_DTL2_COMP_HD_CRED_MAX_ERR		(0x1ULL << 50)
#define CE_ADM_ERR_DTL2_COMP_D_CRED_MAX_ERR		(0x1ULL << 51)
#define CE_ADM_ERR_DTL2_NPOSTED_HD_CRED_MAX_ERR		(0x1ULL << 52)
#define CE_ADM_ERR_DTL2_NPOSTED_D_CRED_MAX_ERR		(0x1ULL << 53)
#define CE_ADM_ERR_DTL2_POSTED_HD_CRED_MAX_ERR		(0x1ULL << 54)
#define CE_ADM_ERR_DTL2_POSTED_D_CRED_MAX_ERR		(0x1ULL << 55)
#define CE_ADM_ERR_PORT1_PCIE_COR_ERR			(0x1ULL << 56)
#define CE_ADM_ERR_PORT1_PCIE_NFAT_ERR			(0x1ULL << 57)
#define CE_ADM_ERR_PORT1_PCIE_FAT_ERR			(0x1ULL << 58)
#define CE_ADM_ERR_PORT2_PCIE_COR_ERR			(0x1ULL << 59)
#define CE_ADM_ERR_PORT2_PCIE_NFAT_ERR			(0x1ULL << 60)
#define CE_ADM_ERR_PORT2_PCIE_FAT_ERR			(0x1ULL << 61)

/* ce_adm_ure_ups_buf_barrier_flush register bit masks and shifts */
#define FLUSH_SEL_PORT1_PIPE0_SHFT	0
#define FLUSH_SEL_PORT1_PIPE1_SHFT	4
#define FLUSH_SEL_PORT1_PIPE2_SHFT	8
#define FLUSH_SEL_PORT1_PIPE3_SHFT	12
#define FLUSH_SEL_PORT2_PIPE0_SHFT	16
#define FLUSH_SEL_PORT2_PIPE1_SHFT	20
#define FLUSH_SEL_PORT2_PIPE2_SHFT	24
#define FLUSH_SEL_PORT2_PIPE3_SHFT	28

/* ce_dre_config1 register bit masks and shifts */
#define CE_DRE_RO_ENABLE		(0x1ULL << 0)
#define CE_DRE_DYN_RO_ENABLE		(0x1ULL << 1)
#define CE_DRE_SUP_CONFIG_COMP_ERROR	(0x1ULL << 2)
#define CE_DRE_SUP_IO_COMP_ERROR	(0x1ULL << 3)
#define CE_DRE_ADDR_MODE_SHFT		4

/* ce_dre_config_req_status register bit masks */
#define CE_DRE_LAST_CONFIG_COMPLETION	(0x7ULL << 0)
#define CE_DRE_DOWNSTREAM_CONFIG_ERROR	(0x1ULL << 3)
#define CE_DRE_CONFIG_COMPLETION_VALID	(0x1ULL << 4)
#define CE_DRE_CONFIG_REQUEST_ACTIVE	(0x1ULL << 5)

/* ce_ure_control register bit masks & shifts */
#define CE_URE_RD_MRG_ENABLE		(0x1ULL << 0)
#define CE_URE_WRT_MRG_ENABLE1		(0x1ULL << 4)
#define CE_URE_WRT_MRG_ENABLE2		(0x1ULL << 5)
#define CE_URE_WRT_MRG_TIMER_SHFT	12
#define CE_URE_WRT_MRG_TIMER_MASK	(0x7FFULL << CE_URE_WRT_MRG_TIMER_SHFT)
#define CE_URE_WRT_MRG_TIMER(x)		(((u64)(x) << \
					  CE_URE_WRT_MRG_TIMER_SHFT) & \
					 CE_URE_WRT_MRG_TIMER_MASK)
#define CE_URE_RSPQ_BYPASS_DISABLE	(0x1ULL << 24)
#define CE_URE_UPS_DAT1_PAR_DISABLE	(0x1ULL << 32)
#define CE_URE_UPS_HDR1_PAR_DISABLE	(0x1ULL << 33)
#define CE_URE_UPS_DAT2_PAR_DISABLE	(0x1ULL << 34)
#define CE_URE_UPS_HDR2_PAR_DISABLE	(0x1ULL << 35)
#define CE_URE_ATE_PAR_DISABLE		(0x1ULL << 36)
#define CE_URE_RCI_PAR_DISABLE		(0x1ULL << 37)
#define CE_URE_RSPQ_PAR_DISABLE		(0x1ULL << 38)
#define CE_URE_DNS_DAT_PAR_DISABLE	(0x1ULL << 39)
#define CE_URE_DNS_HDR_PAR_DISABLE	(0x1ULL << 40)
#define CE_URE_MALFORM_DISABLE		(0x1ULL << 44)
#define CE_URE_UNSUP_DISABLE		(0x1ULL << 45)

/* ce_ure_page_map register bit masks & shifts */
#define CE_URE_ATE3240_ENABLE		(0x1ULL << 0)
#define CE_URE_ATE40_ENABLE 		(0x1ULL << 1)
#define CE_URE_PAGESIZE_SHFT		4
#define CE_URE_PAGESIZE_MASK		(0x7ULL << CE_URE_PAGESIZE_SHFT)
#define CE_URE_4K_PAGESIZE		(0x0ULL << CE_URE_PAGESIZE_SHFT)
#define CE_URE_16K_PAGESIZE		(0x1ULL << CE_URE_PAGESIZE_SHFT)
#define CE_URE_64K_PAGESIZE		(0x2ULL << CE_URE_PAGESIZE_SHFT)
#define CE_URE_128K_PAGESIZE		(0x3ULL << CE_URE_PAGESIZE_SHFT)
#define CE_URE_256K_PAGESIZE		(0x4ULL << CE_URE_PAGESIZE_SHFT)

/* ce_ure_pipe_sel register bit masks & shifts */
#define PKT_TRAFIC_SHRT			16
#define BUS_SRC_ID_SHFT			8
#define DEV_SRC_ID_SHFT			3
#define FNC_SRC_ID_SHFT			0
#define CE_URE_TC_MASK			(0x07ULL << PKT_TRAFIC_SHRT)
#define CE_URE_BUS_MASK			(0xFFULL << BUS_SRC_ID_SHFT)
#define CE_URE_DEV_MASK			(0x1FULL << DEV_SRC_ID_SHFT)
#define CE_URE_FNC_MASK			(0x07ULL << FNC_SRC_ID_SHFT)
#define CE_URE_PIPE_BUS(b)		(((u64)(b) << BUS_SRC_ID_SHFT) & \
					 CE_URE_BUS_MASK)
#define CE_URE_PIPE_DEV(d)		(((u64)(d) << DEV_SRC_ID_SHFT) & \
					 CE_URE_DEV_MASK)
#define CE_URE_PIPE_FNC(f)		(((u64)(f) << FNC_SRC_ID_SHFT) & \
					 CE_URE_FNC_MASK)

#define CE_URE_SEL1_SHFT		0
#define CE_URE_SEL2_SHFT		20
#define CE_URE_SEL3_SHFT		40
#define CE_URE_SEL1_MASK		(0x7FFFFULL << CE_URE_SEL1_SHFT)
#define CE_URE_SEL2_MASK		(0x7FFFFULL << CE_URE_SEL2_SHFT)
#define CE_URE_SEL3_MASK		(0x7FFFFULL << CE_URE_SEL3_SHFT)


/* ce_ure_pipe_mask register bit masks & shifts */
#define CE_URE_MASK1_SHFT		0
#define CE_URE_MASK2_SHFT		20
#define CE_URE_MASK3_SHFT		40
#define CE_URE_MASK1_MASK		(0x7FFFFULL << CE_URE_MASK1_SHFT)
#define CE_URE_MASK2_MASK		(0x7FFFFULL << CE_URE_MASK2_SHFT)
#define CE_URE_MASK3_MASK		(0x7FFFFULL << CE_URE_MASK3_SHFT)


/* ce_ure_pcie_control1 register bit masks & shifts */
#define CE_URE_SI			(0x1ULL << 0)
#define CE_URE_ELAL_SHFT		4
#define CE_URE_ELAL_MASK		(0x7ULL << CE_URE_ELAL_SHFT)
#define CE_URE_ELAL_SET(n)		(((u64)(n) << CE_URE_ELAL_SHFT) & \
					 CE_URE_ELAL_MASK)
#define CE_URE_ELAL1_SHFT		8
#define CE_URE_ELAL1_MASK		(0x7ULL << CE_URE_ELAL1_SHFT)
#define CE_URE_ELAL1_SET(n)		(((u64)(n) << CE_URE_ELAL1_SHFT) & \
					 CE_URE_ELAL1_MASK)
#define CE_URE_SCC			(0x1ULL << 12)
#define CE_URE_PN1_SHFT			16
#define CE_URE_PN1_MASK			(0xFFULL << CE_URE_PN1_SHFT)
#define CE_URE_PN2_SHFT			24
#define CE_URE_PN2_MASK			(0xFFULL << CE_URE_PN2_SHFT)
#define CE_URE_PN1_SET(n)		(((u64)(n) << CE_URE_PN1_SHFT) & \
					 CE_URE_PN1_MASK)
#define CE_URE_PN2_SET(n)		(((u64)(n) << CE_URE_PN2_SHFT) & \
					 CE_URE_PN2_MASK)

/* ce_ure_pcie_control2 register bit masks & shifts */
#define CE_URE_ABP			(0x1ULL << 0)
#define CE_URE_PCP			(0x1ULL << 1)
#define CE_URE_MSP			(0x1ULL << 2)
#define CE_URE_AIP			(0x1ULL << 3)
#define CE_URE_PIP			(0x1ULL << 4)
#define CE_URE_HPS			(0x1ULL << 5)
#define CE_URE_HPC			(0x1ULL << 6)
#define CE_URE_SPLV_SHFT		7
#define CE_URE_SPLV_MASK		(0xFFULL << CE_URE_SPLV_SHFT)
#define CE_URE_SPLV_SET(n)		(((u64)(n) << CE_URE_SPLV_SHFT) & \
					 CE_URE_SPLV_MASK)
#define CE_URE_SPLS_SHFT		15
#define CE_URE_SPLS_MASK		(0x3ULL << CE_URE_SPLS_SHFT)
#define CE_URE_SPLS_SET(n)		(((u64)(n) << CE_URE_SPLS_SHFT) & \
					 CE_URE_SPLS_MASK)
#define CE_URE_PSN1_SHFT		19
#define CE_URE_PSN1_MASK		(0x1FFFULL << CE_URE_PSN1_SHFT)
#define CE_URE_PSN2_SHFT		32
#define CE_URE_PSN2_MASK		(0x1FFFULL << CE_URE_PSN2_SHFT)
#define CE_URE_PSN1_SET(n)		(((u64)(n) << CE_URE_PSN1_SHFT) & \
					 CE_URE_PSN1_MASK)
#define CE_URE_PSN2_SET(n)		(((u64)(n) << CE_URE_PSN2_SHFT) & \
					 CE_URE_PSN2_MASK)

/*
 * PIO address space ranges for CE
 */

/* Local CE Registers Space */
#define CE_PIO_MMR			0x00000000
#define CE_PIO_MMR_LEN			0x04000000

/* PCI Compatible Config Space */
#define CE_PIO_CONFIG_SPACE		0x04000000
#define CE_PIO_CONFIG_SPACE_LEN		0x04000000

/* PCI I/O Space Alias */
#define CE_PIO_IO_SPACE_ALIAS		0x08000000
#define CE_PIO_IO_SPACE_ALIAS_LEN	0x08000000

/* PCI Enhanced Config Space */
#define CE_PIO_E_CONFIG_SPACE		0x10000000
#define CE_PIO_E_CONFIG_SPACE_LEN	0x10000000

/* PCI I/O Space */
#define CE_PIO_IO_SPACE			0x100000000
#define CE_PIO_IO_SPACE_LEN		0x100000000

/* PCI MEM Space */
#define CE_PIO_MEM_SPACE		0x200000000
#define CE_PIO_MEM_SPACE_LEN		TIO_HWIN_SIZE


/*
 * CE PCI Enhanced Config Space shifts & masks
 */
#define CE_E_CONFIG_BUS_SHFT		20
#define CE_E_CONFIG_BUS_MASK		(0xFF << CE_E_CONFIG_BUS_SHFT)
#define CE_E_CONFIG_DEVICE_SHFT		15
#define CE_E_CONFIG_DEVICE_MASK		(0x1F << CE_E_CONFIG_DEVICE_SHFT)
#define CE_E_CONFIG_FUNC_SHFT		12
#define CE_E_CONFIG_FUNC_MASK		(0x7  << CE_E_CONFIG_FUNC_SHFT)

#endif /* __ASM_IA64_SN_TIOCE_H__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2003-2005 Silicon Graphics, Inc. All rights reserved.
 */

#ifndef _ASM_IA64_SN_CE_PROVIDER_H
#define _ASM_IA64_SN_CE_PROVIDER_H

#include <asm/sn/pcibus_provider_defs.h>
#include <asm/sn/tioce.h>

/*
 * Common TIOCE structure shared between the prom and kernel
 *
 * DO NOT CHANGE THIS STRUCT WITHOUT MAKING CORRESPONDING CHANGES TO THE
 * PROM VERSION.
 */
struct tioce_common {
	struct pcibus_bussoft	ce_pcibus;	/* common pciio header */

	u32		ce_rev;
	u64		ce_kernel_private;
	u64		ce_prom_private;
};

struct tioce_kernel {
	struct tioce_common	*ce_common;
	spinlock_t		ce_lock;
	struct list_head	ce_dmamap_list;

	u64		ce_ate40_shadow[TIOCE_NUM_M40_ATES];
	u64		ce_ate3240_shadow[TIOCE_NUM_M3240_ATES];
	u32		ce_ate3240_pagesize;

	u8			ce_port1_secondary;

	/* per-port resources */
	struct {
		int 		dirmap_refcnt;
		u64	dirmap_shadow;
	} ce_port[TIOCE_NUM_PORTS];
};

struct tioce_dmamap {
	struct list_head	ce_dmamap_list;	/* headed by tioce_kernel */
	u32		refcnt;

	u64		nbytes;		/* # bytes mapped */
