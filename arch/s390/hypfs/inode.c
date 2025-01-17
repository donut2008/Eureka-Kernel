 4.6875Hz interrupt has occurred */ 
#define RTCISR_SAM1	0x0200	/*   8Hz /   9.3750Hz interrupt has occurred */ 
#define RTCISR_SAM2	0x0400	/*  16Hz /  18.7500Hz interrupt has occurred */ 
#define RTCISR_SAM3	0x0800	/*  32Hz /  37.5000Hz interrupt has occurred */ 
#define RTCISR_SAM4	0x1000	/*  64Hz /  75.0000Hz interrupt has occurred */ 
#define RTCISR_SAM5	0x2000	/* 128Hz / 150.0000Hz interrupt has occurred */ 
#define RTCISR_SAM6	0x4000	/* 256Hz / 300.0000Hz interrupt has occurred */ 
#define RTCISR_SAM7	0x8000	/* 512Hz / 600.0000Hz interrupt has occurred */ 

/*
 * RTC Interrupt Enable Register
 */
#define RTCIENR_ADDR	0xfffffb10
#define RTCIENR		WORD_REF(RTCIENR_ADDR)

#define RTCIENR_SW	0x0001	/* Stopwatch interrupt enable */
#define RTCIENR_MIN	0x0002	/* 1-minute interrupt enable */
#define RTCIENR_ALM	0x0004	/* Alarm interrupt enable */
#define RTCIENR_DAY	0x0008	/* 24-hour rollover interrupt enable */
#define RTCIENR_1HZ	0x0010	/* 1Hz interrupt enable */
#define RTCIENR_HR	0x0020	/* 1-hour interrupt enable */
#define RTCIENR_SAM0	0x0100	/*   4Hz /   4.6875Hz interrupt enable */ 
#define RTCIENR_SAM1	0x0200	/*   8Hz /   9.3750Hz interrupt enable */ 
#define RTCIENR_SAM2	0x0400	/*  16Hz /  18.7500Hz interrupt enable */ 
#define RTCIENR_SAM3	0x0800	/*  32Hz /  37.5000Hz interrupt enable */ 
#define RTCIENR_SAM4	0x1000	/*  64Hz /  75.0000Hz interrupt enable */ 
#define RTCIENR_SAM5	0x2000	/* 128Hz / 150.0000Hz interrupt enable */ 
#define RTCIENR_SAM6	0x4000	/* 256Hz / 300.0000Hz interrupt enable */ 
#define RTCIENR_SAM7	0x8000	/* 512Hz / 600.0000Hz interrupt enable */ 

/* 
 * Stopwatch Minutes Register
 */
#define STPWCH_ADDR	0xfffffb12
#define STPWCH		WORD_REF(STPWCH)

#define STPWCH_CNT_MASK	 0x003f	/* Stopwatch countdown value */
#define SPTWCH_CNT_SHIFT 0

/*
 * RTC Day Count Register 
 */
#define DAYR_ADDR	0xfffffb1a
#define DAYR		WORD_REF(DAYR_ADDR)

#define DAYR_DAYS_MASK	0x1ff	/* Day Setting */
#define DAYR_DAYS_SHIFT 0

/*
 * RTC Day Alarm Register 
 */
#define DAYALARM_ADDR	0xfffffb1c
#define DAYALARM	WORD_REF(DAYALARM_ADDR)

#define DAYALARM_DAYSAL_MASK	0x01ff	/* Day Setting of the Alarm */
#define DAYALARM_DAYSAL_SHIFT 	0

/**********
 *
 * 0xFFFFFCxx -- DRAM Controller
 *
 **********/

/*
 * DRAM Memory Configuration Register 
 */
#define DRAMMC_ADDR	0xfffffc00
#define DRAMMC		WORD_REF(DRAMMC_ADDR)

#define DRAMMC_ROW12_MASK	0xc000	/* Row address bit for MD12 */
#define   DRAMMC_ROW12_PA10	0x0000
#define   DRAMMC_ROW12_PA21	0x4000	
#define   DRAMMC_ROW12_PA23	0x8000
#define	DRAMMC_ROW0_MASK	0x3000	/* Row address bit for MD0 */
#define	  DRAMMC_ROW0_PA11	0x0000
#define   DRAMMC_ROW0_PA22	0x1000
#define   DRAMMC_ROW0_PA23	0x2000
#define DRAMMC_ROW11		0x0800	/* Row address bit for MD11 PA20/PA22 */
#define DRAMMC_ROW10		0x0400	/* Row address bit for MD10 PA19/PA21 */
#define	DRAMMC_ROW9		0x0200	/* Row address bit for MD9  PA9/PA19  */
#define DRAMMC_ROW8		0x0100	/* Row address bit for MD8  PA10/PA20 */
#define DRAMMC_COL10		0x0080	/* Col address bit for MD10 PA11/PA0  */
#define DRAMMC_COL9		0x0040	/* Col address bit for MD9  PA10/PA0  */
#define DRAMMC_COL8		0x0020	/* Col address bit for MD8  PA9/PA0   */
#define DRAMMC_REF_MASK		0x001f	/* Reresh Cycle */
#define DRAMMC_REF_SHIFT	0

/*
 * DRAM Control Register
 */
#define DRAMC_ADDR	0xfffffc02
#define DRAMC		WORD_REF(DRAMC_ADDR)

#define DRAMC_DWE	   0x0001	/* DRAM Write Enable */
#define DRAMC_RST	   0x0002	/* Reset Burst Refresh Enable */
#define DRAMC_LPR	   0x0004	/* Low-Power Refresh Enable */
#define DRAMC_SLW	   0x0008	/* Slow RAM */
#define DRAMC_LSP	   0x0010	/* Light Sleep */
#define DRAMC_MSW	   0x0020	/* Slow Multiplexing */
#define DRAMC_WS_MASK	   0x00c0	/* Wait-states */
#define DRAMC_WS_SHIFT	   6
#define DRAMC_PGSZ_MASK    0x0300	/* Page Size for fast page mode */
#define DRAMC_PGSZ_SHIFT   8
#define   DRAMC_PGSZ_256K  0x0000	
#define   DRAMC_PGSZ_512K  0x0100
#define   DRAMC_PGSZ_1024K 0x0200
#define	  DRAMC_PGSZ_2048K 0x0300
#define DRAMC_EDO	   0x0400	/* EDO DRAM */
#define DRAMC_CLK	   0x0800	/* Refresh Timer Clock source select */
#define DRAMC_BC_MASK	   0x3000	/* Page Access Clock Cycle (FP mode) */
#define DRAMC_BC_SHIFT	   12
#define DRAMC_RM	   0x4000	/* Refresh Mode */
#define DRAMC_EN	   0x8000	/* DRAM Controller enable */


/**********
 *
 * 0xFFFFFDxx -- In-Circuit Emulation (ICE)
 *
 **********/

/*
 * ICE Module Address Compare Register
 */
#define ICEMACR_ADDR	0xfffffd00
#define ICEMACR		LONG_REF(ICEMACR_ADDR)

/*
 * ICE Module Address Mask Register
 */
#define ICEMAMR_ADDR	0xfffffd04
#define ICEMAMR		LONG_REF(ICEMAMR_ADDR)

/*
 * ICE Module Control Compare Register
 */
#define ICEMCCR_ADDR	0xfffffd08
#define ICEMCCR		WORD_REF(ICEMCCR_ADDR)

#define ICEMCCR_PD	0x0001	/* Program/Data Cycle Selection */
#define ICEMCCR_RW	0x0002	/* Read/Write Cycle Selection */

/*
 * ICE Module Control Mask Register
 */
#define ICEMCMR_ADDR	0xfffffd0a
#define ICEMCMR		WORD_REF(ICEMCMR_ADDR)

#define ICEMCMR_PDM	0x0001	/* Program/Data Cycle Mask */
#define ICEMCMR_RWM	0x0002	/* Read/Write Cycle Mask */

/*
 * ICE Module Control Register 
 */
#define ICEMCR_ADDR	0xfffffd0c
#define ICEMCR		WORD_REF(ICEMCR_ADDR)

#define ICEMCR_CEN	0x0001	/* Compare Enable */
#define ICEMCR_PBEN	0x0002	/* Program Break Enable */
#define ICEMCR_SB	0x0004	/* Single Breakpoint */
#define ICEMCR_HMDIS	0x0008	/* HardMap disable */
#define ICEMCR_BBIEN	0x0010	/* Bus Break Interrupt Enable */

/*
 * ICE Module Status Register 
 */
#define ICEMSR_ADDR	0xfffffd0e
#define ICEMSR		WORD_REF(ICEMSR_ADDR)

#define ICEMSR_EMUEN	0x0001	/* Emulation Enable */
#define ICEMSR_BRKIRQ	0x0002	/* A-Line Vector Fetch Detected */
#define ICEMSR_BBIRQ	0x0004	/* Bus Break Interrupt Detected */
#define ICEMSR_EMIRQ	0x0008	/* EMUIRQ Falling Edge Detected */

#endif /* _MC68EZ328_H_ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
/* include/asm-m68knommu/MC68VZ328.h: 'VZ328 control registers
 *
 * Copyright (c) 2000-2001	Lineo Inc. <www.lineo.com>
 * Copyright (c) 2000-2001	Lineo Canada Corp. <www.lineo.ca>
 * Copyright (C) 1999		Vladimir Gurevich <vgurevic@cisco.com>
 * 				Bare & Hare Software, Inc.
 * Based on include/asm-m68knommu/MC68332.h
 * Copyright (C) 1998  Kenneth Albanowski <kjahds@kjahds.com>,
 *                     The Silver Hammer Group, Ltd.
 *
 * M68VZ328 fixes by Evan Stawnyczy <evan@lineo.com>
 * vz multiport fixes by Michael Leslie <mleslie@lineo.com>
 */

#ifndef _MC68VZ328_H_
#define _MC68VZ328_H_

#define BYTE_REF(addr) (*((volatile unsigned char*)addr))
#define WORD_REF(addr) (*((volatile unsigned short*)addr))
#define LONG_REF(addr) (*((volatile unsigned long*)addr))

#define PUT_FIELD(field, val) (((val) << field##_SHIFT) & field##_MASK)
#define GET_FIELD(reg, field) (((reg) & field##_MASK) >> field##_SHIFT)

/********** 
 *
 * 0xFFFFF0xx -- System Control
 *
 **********/
 
/*
 * System Control Register (SCR)
 */
#define SCR_ADDR	0xfffff000
#define SCR		BYTE_REF(SCR_ADDR)

#define SCR_WDTH8	0x01	/* 8-Bit Width Select */
#define SCR_DMAP	0x04	/* Double Map */
#define SCR_SO		0x08	/* Supervisor Only */
#define SCR_BETEN	0x10	/* Bus-Error Time-Out Enable */
#define SCR_PRV		0x20	/* Privilege Violation */
#define SCR_WPV		0x40	/* Write Protect Violation */
#define SCR_BETO	0x80	/* Bus-Error TimeOut */

/*
 * Silicon ID Register (Mask Revision Register (MRR) for '328 Compatibility)
 */
#define MRR_ADDR 0xfffff004
#define MRR	 LONG_REF(MRR_ADDR)

/********** 
 *
 * 0xFFFFF1xx -- Chip-Select logic
 *
 **********/
 
/*
 * Chip Select Group Base Registers 
 */
#define CSGBA_ADDR	0xfffff100
#define CSGBB_ADDR	0xfffff102

#define CSGBC_ADDR	0xfffff104
#define CSGBD_ADDR	0xfffff106

#define CSGBA		WORD_REF(CSGBA_ADDR)
#define CSGBB		WORD_REF(CSGBB_ADDR)
#define CSGBC		WORD_REF(CSGBC_ADDR)
#define CSGBD		WORD_REF(CSGBD_ADDR)

/*
 * Chip Select Registers 
 */
#define CSA_ADDR	0xfffff110
#define CSB_ADDR	0xfffff112
#define CSC_ADDR	0xfffff114
#define CSD_ADDR	0xfffff116

#define CSA		WORD_REF(CSA_ADDR)
#define CSB		WORD_REF(CSB_ADDR)
#define CSC		WORD_REF(CSC_ADDR)
#define CSD		WORD_REF(CSD_ADDR)

#define CSA_EN		0x0001		/* Chip-Select Enable */
#define CSA_SIZ_MASK	0x000e		/* Chip-Select Size */
#define CSA_SIZ_SHIFT   1
#define CSA_WS_MASK	0x0070		/* Wait State */
#define CSA_WS_SHIFT    4
#define CSA_BSW		0x0080		/* Data Bus Width */
#define CSA_FLASH	0x0100		/* FLASH Memory Support */
#define CSA_RO		0x8000		/* Read-Only */

#define CSB_EN		0x0001		/* Chip-Select Enable */
#define CSB_SIZ_MASK	0x000e		/* Chip-Select Size */
#define CSB_SIZ_SHIFT   1
#define CSB_WS_MASK	0x0070		/* Wait State */
#define CSB_WS_SHIFT    4
#define CSB_BSW		0x0080		/* Data Bus Width */
#define CSB_FLASH	0x0100		/* FLASH Memory Support */
#define CSB_UPSIZ_MASK	0x1800		/* Unprotected memory block size */
#define CSB_UPSIZ_SHIFT 11
#define CSB_ROP		0x2000		/* Readonly if protected */
#define CSB_SOP		0x4000		/* Supervisor only if protected */
#define CSB_RO		0x8000		/* Read-Only */

#define CSC_EN		0x0001		/* Chip-Select Enable */
#define CSC_SIZ_MASK	0x000e		/* Chip-Select Size */
#define CSC_SIZ_SHIFT   1
#define CSC_WS_MASK	0x0070		/* Wait State */
#define CSC_WS_SHIFT    4
#define CSC_BSW		0x0080		/* Data Bus Width */
#define CSC_FLASH	0x0100		/* FLASH Memory Support */
#define CSC_UPSIZ_MASK	0x1800		/* Unprotected memory block size */
#define CSC_UPSIZ_SHIFT 11
#define CSC_ROP		0x2000		/* Readonly if protected */
#define CSC_SOP		0x4000		/* Supervisor only if protected */
#define CSC_RO		0x8000		/* Read-Only */

#define CSD_EN		0x0001		/* Chip-Select Enable */
#define CSD_SIZ_MASK	0x000e		/* Chip-Select Size */
#define CSD_SIZ_SHIFT   1
#define CSD_WS_MASK	0x0070		/* Wait State */
#define CSD_WS_SHIFT    4
#define CSD_BSW		0x0080		/* Data Bus Width */
#define CSD_FLASH	0x0100		/* FLASH Memory Support */
#define CSD_DRAM	0x0200		/* Dram Selection */
#define	CSD_COMB	0x0400		/* Combining */
#define CSD_UPSIZ_MASK	0x1800		/* Unprotected memory block size */
#define CSD_UPSIZ_SHIFT 11
#define CSD_ROP		0x2000		/* Readonly if protected */
#define CSD_SOP		0x4000		/* Supervisor only if protected */
#define CSD_RO		0x8000		/* Read-Only */

/*
 * Emulation Chip-Select Register 
 */
#define EMUCS_ADDR	0xfffff118
#define EMUCS		WORD_REF(EMUCS_ADDR)

#define EMUCS_WS_MASK	0x0070
#define EMUC