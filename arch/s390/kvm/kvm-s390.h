generic-y += barrier.h
generic-y += bitsperlong.h
generic-y += clkdev.h
generic-y += cputime.h
generic-y += device.h
generic-y += emergency-restart.h
generic-y += errno.h
generic-y += exec.h
generic-y += futex.h
generic-y += hw_irq.h
generic-y += ioctl.h
generic-y += ipcbuf.h
generic-y += irq_regs.h
generic-y += irq_work.h
generic-y += kdebug.h
generic-y += kmap_types.h
generic-y += kvm_para.h
generic-y += local.h
generic-y += local64.h
generic-y += mcs_spinlock.h
generic-y += mm-arch-hooks.h
generic-y += mman.h
generic-y += mutex.h
generic-y += percpu.h
generic-y += preempt.h
generic-y += resource.h
generic-y += sections.h
generic-y += shmparam.h
generic-y += siginfo.h
generic-y += spinlock.h
generic-y += statfs.h
generic-y += termios.h
generic-y += topology.h
generic-y += trace_clock.h
generic-y += types.h
generic-y += word-at-a-time.h
generic-y += xor.h
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
/* include/asm-m68knommu/MC68328.h: '328 control registers
 *
 * Copyright (C) 1999  Vladimir Gurevich <vgurevic@cisco.com>
 *                     Bear & Hare Software, Inc.
 *
 * Based on include/asm-m68knommu/MC68332.h
 * Copyright (C) 1998  Kenneth Albanowski <kjahds@kjahds.com>,
 *
 */

#ifndef _MC68328_H_
#define _MC68328_H_

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
 * Mask Revision Register
 */
#define MRR_ADDR 0xfffff004
#define MRR      LONG_REF(MRR_ADDR)
 
/********** 
 *
 * 0xFFFFF1xx -- Chip-Select logic
 *
 **********/

/********** 
 *
 * 0xFFFFF2xx -- Phase Locked Loop (PLL) & Power Control
 *
 **********/

/*
 * Group Base Address Registers
 */
#define GRPBASEA_ADDR	0xfffff100
#define GRPBASEB_ADDR	0xfffff102
#define GRPBASEC_ADDR	0xfffff104
#define GRPBASED_ADDR	0xfffff106

#define GRPBASEA	WORD_REF(GRPBASEA_ADDR)
#define GRPBASEB	WORD_REF(GRPBASEB_ADDR)
#define GRPBASEC	WORD_REF(GRPBASEC_ADDR)
#define GRPBASED	WORD_REF(GRPBASED_ADDR)

#define GRPBASE_V	  0x0001	/* Valid */
#define GRPBASE_GBA_MASK  0xfff0	/* Group Base Address (bits 31-20) */

/*
 * Group Base Address Mask Registers 
 */
#define GRPMASKA_ADDR	0xfffff108
#define GRPMASKB_ADDR	0xfffff10a
#define GRPMASKC_ADDR	0xfffff10c
#define GRPMASKD_ADDR	0xfffff10e

#define GRPMASKA	WORD_REF(GRPMASKA_ADDR)
#define GRPMASKB	WORD_REF(GRPMASKB_ADDR)
#define GRPMASKC	WORD_REF(GRPMASKC_ADDR)
#define GRPMASKD	WORD_REF(GRPMASKD_ADDR)

#define GRMMASK_GMA_MASK 0xfffff0	/* Group Base Mask (bits 31-20) */

/*
 * Chip-Select Option Registers (group A)
 */
#define CSA0_ADDR	0xfffff110
#define CSA1_ADDR	0xfffff114
#define CSA2_ADDR	0xfffff118
#define CSA3_ADDR	0xfffff11c

#define CSA0		LONG_REF(CSA0_ADDR)
#define CSA1		LONG_REF(CSA1_ADDR)
#define CSA2		LONG_REF(CSA2_ADDR)
#define CSA3		LONG_REF(CSA3_ADDR)

#define CSA_WAIT_MASK	0x00000007	/* Wait State Selection */
#define CSA_WAIT_SHIFT	0
#define CSA_RO		0x00000008	/* Read-Only */
#define CSA_AM_MASK	0x0000ff00	/* Address Mask (bits 23-16) */
#define CSA_AM_SHIFT	8
#define CSA_BUSW	0x00010000	/* Bus Width Select */
#define CSA_AC_MASK	0xff000000	/* Address Compare (bits 23-16) */
#define CSA_AC_SHIFT	24

/*
 * Chip-Select Option Registers (group B)
 */
#define CSB0_ADDR	0xfffff120
#define CSB1_ADDR	0xfffff124
#define CSB2_ADDR	0xfffff128
#define CSB3_ADDR	0xfffff12c

#define CSB0		LONG_REF(CSB0_ADDR)
#define CSB1		LONG_REF(CSB1_ADDR)
#define CSB2		LONG_REF(CSB2_ADDR)
#define CSB3		LONG_REF(CSB3_ADDR)

#define CSB_WAIT_MASK	0x00000007	/* Wait State Selection */
#define CSB_WAIT_SHIFT	0
#define CSB_RO		0x00000008	/* Read-Only */
#define CSB_AM_MASK	0x0000ff00	/* Address Mask (bits 23-16) */
#define CSB_AM_SHIFT	8
#define CSB_BUSW	0x00010000	/* Bus Width Select */
#define CSB_AC_MASK	0xff000000	/* Address Compare (bits 23-16) */
#define CSB_AC_SHIFT	24

/*
 * Chip-Select Option Registers (group C)
 */
#define CSC0_ADDR	0xfffff130
#define CSC1_ADDR	0xfffff134
#define CSC2_ADDR	0xfffff138
#define CSC3_ADDR	0xfffff13c

#define CSC0		LONG_REF(CSC0_ADDR)
#define CSC1		LONG_REF(CSC1_ADDR)
#define CSC2		LONG_REF(CSC2_ADDR)
#define CSC3		LONG_REF(CSC3_ADDR)

#define CSC_WAIT_MASK	0x00000007	/* Wait State Selection */
#define CSC_WAIT_SHIFT	0
#define CSC_RO		0x00000008	/* Read-Only */
#define CSC_AM_MASK	0x0000fff0	/* Address Mask (bits 23-12) */
#define CSC_AM_SHIFT	4
#define CSC_BUSW	0x00010000	/* Bus Width Select */
#define CSC_AC_MASK	0xfff00000	/* Address Compare (bits 23-12) */
#define CSC_AC_SHIFT	20

/*
 * Chip-Select Option Registers (group D)
 */
#define CSD0_ADDR	0xfffff140
#define CSD1_ADDR	0xfffff144
#define CSD2_ADDR	0xfffff148
#define CSD3_ADDR	0xfffff14c

#define CSD0		LONG_REF(CSD0_ADDR)
#define CSD1		LONG_REF(CSD1_ADDR)
#define CSD2		LONG_REF(CSD2_ADDR)
#define CSD3		LONG_REF(CSD3_ADDR)

#define CSD_WAIT_MASK	0x00000007	/* Wait State Selection */
#define CSD_WAIT_SHIFT	0
#define CSD_RO		0x00000008	/* Read-Only */
#define CSD_AM_MASK	0x0000fff0	/* Address Mask (bits 23-12) */
#define CSD_AM_SHIFT	4
#define CSD_BUSW	0x00010000	/* Bus Width Select */
#define CSD_AC_MASK	0xfff00000	/* Address Compare (bits 23-12) */
#define CSD_AC_SHIFT	20

/**********
 *
 * 0xFFFFF2xx -- Phase Locked Loop (PLL) & Power Control
 *
 **********/
 
/*
 * PLL Control Register 
 */
#define PLLCR_ADDR	0xfffff200
#define PLLCR		WORD_REF(PLLCR_ADDR)

#define PLLCR_DISPLL	       0x0008	/* Disable PLL */
#define PLLCR_CLKEN	       0x0010	/* Clock (CLKO pin) enable */
#define PLLCR_SYSCLK_SEL_MASK  0x0700	/* System Clock Selection */
#define PLLCR_SYSCLK_SEL_SHIFT 8
#define PLLCR_PIXCLK_SEL_MASK  0x3800	/* LCD Clock Selection */
#define PLLCR_PIXCLK_SEL_SHIFT 11

/* 'EZ328-compatible definitions */
#define PLLCR_LCDCLK_SEL_MASK	PLLCR_PIXCLK_SEL_MASK
#define PLLCR_LCDCLK_SEL_SHIFT	PLLCR_PIXCLK_SEL_SHIFT

/*
 * PLL Frequency Select Register
 */
#define PLLFSR_ADDR	0xfffff202
#define PLLFSR		WORD_REF(PLLFSR_ADDR)

#define PLLFSR_PC_MASK	0x00ff		/* P Count */
#define PLLFSR_PC_SHIFT 0
#define PLLFSR_QC_MASK	0x0f00		/* Q Count */
#define PLLFSR_QC_SHIFT 8
#define PLLFSR_PROT	0x4000		/* Protect P & Q */
#define PLLFSR_CLK32	0x8000		/* Clock 32 (kHz) */

/*
 * Power Control Register
 */
#define PCTRL_ADDR	0xfffff207
#define PCTRL		BYTE_REF(PCTRL_ADDR)

#define PCTRL_WIDTH_MASK	0x1f	/* CPU Clock bursts width */
#define PCTRL_WIDTH_SHIFT	0
#define PCTRL_STOP		0x40	/* Enter power-save mode immediately */ 
#define PCTRL_PCEN		0x80	/* Power Control Enable */

/**********
 *
 * 0xFFFFF3xx -- Interrupt Controller
 *
 **********/

/* 
 * Interrupt Vector Register
 */
#define IVR_ADDR	0xfffff300
#define IVR		BYTE_REF(IVR_ADDR)

#define IVR_VECTOR_MASK 0xF8

/*
 * Interrupt control Register
 */
#define ICR_ADRR	0xfffff302
#define ICR		WORD_REF(ICR_ADDR)

#define ICR_ET6		0x0100	/* Edge Trigger Select for IRQ6 */
#define ICR_ET3		0x0200	/* Edge Trigger Select for IRQ3 */
#define ICR_ET2		0x0400	/* Edge Trigger Select for IRQ2 */
#define ICR_ET1		0x0800	/* Edge Trigger Select for IRQ1 */
#define ICR_POL6	0x1000	/* Polarity Control for IRQ6 */
#define ICR_POL3	0x2000	/* Polarity Control for IRQ3 */
#define ICR_PO