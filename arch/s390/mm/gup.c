/*
** amigaints.h -- Amiga Linux interrupt handling structs and prototypes
**
** Copyright 1992 by Greg Harp
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
** Created 10/2/92 by Greg Harp
*/

#ifndef _ASMm68k_AMIGAINTS_H_
#define _ASMm68k_AMIGAINTS_H_

#include <asm/irq.h>

/*
** Amiga Interrupt sources.
**
*/

#define AUTO_IRQS           (8)
#define AMI_STD_IRQS        (14)
#define CIA_IRQS            (5)
#define AMI_IRQS            (32) /* AUTO_IRQS+AMI_STD_IRQS+2*CIA_IRQS */

/* builtin serial port interrupts */
#define IRQ_AMIGA_TBE		(IRQ_USER+0)
#define IRQ_AMIGA_RBF		(IRQ_USER+11)

/* floppy disk interrupts */
#define IRQ_AMIGA_DSKBLK	(IRQ_USER+1)
#define IRQ_AMIGA_DSKSYN	(IRQ_USER+12)

/* software interrupts */
#define IRQ_AMIGA_SOFT		(IRQ_USER+2)

/* interrupts from external hardware */
#define IRQ_AMIGA_PORTS		IRQ_AUTO_2
#define IRQ_AMIGA_EXTER		IRQ_AUTO_6

/* copper interrupt */
#define IRQ_AMIGA_COPPER	(IRQ_USER+4)

/* vertical blanking interrupt */
#define IRQ_AMIGA_VERTB		(IRQ_USER+5)

/* Blitter done interrupt */
#define IRQ_AMIGA_BLIT		(IRQ_USER+6)

/* Audio interrupts */
#define IRQ_AMIGA_AUD0		(IRQ_USER+7)
#define IRQ_AMIGA_AUD1		(IRQ_USER+8)
#define IRQ_AMIGA_AUD2		(IRQ_USER+9)
#define IRQ_AMIGA_AUD3		(IRQ_USER+10)

/* CIA interrupt sources */
#define IRQ_AMIGA_CIAA		(IRQ_USER+14)
#define IRQ_AMIGA_CIAA_TA	(IRQ_USER+14)
#define IRQ_AMIGA_CIAA_TB	(IRQ_USER+15)
#define IRQ_AMIGA_CIAA_ALRM	(IRQ_USER+16)
#define IRQ_AMIGA_CIAA_SP	(IRQ_USER+17)
#define IRQ_AMIGA_CIAA_FLG	(IRQ_USER+18)
#define IRQ_AMIGA_CIAB		(IRQ_USER+19)
#define IRQ_AMIGA_CIAB_TA	(IRQ_USER+19)
#define IRQ_AMIGA_CIAB_TB	(IRQ_USER+20)
#define IRQ_AMIGA_CIAB_ALRM	(IRQ_USER+21)
#define IRQ_AMIGA_CIAB_SP	(IRQ_USER+22)
#define IRQ_AMIGA_CIAB_FLG	(IRQ_USER+23)


/* INTREQR masks */
#define IF_SETCLR   0x8000      /* set/clr bit */
#define IF_INTEN    0x4000	/* master interrupt bit in INT* registers */
#define IF_EXTER    0x2000	/* external level 6 and CIA B interrupt */
#define IF_DSKSYN   0x1000	/* disk sync interrupt */
#define IF_RBF	    0x0800	/* serial receive buffer full interrupt */
#define IF_AUD3     0x0400	/* audio channel 3 done interrupt */
#define IF_AUD2     0x0200	/* audio channel 2 done interrupt */
#define IF_AUD1     0x0100	/* audio channel 1 done interrupt */
#define IF_AUD0     0x0080	/* audio channel 0 done interrupt */
#define IF_BLIT     0x0040	/* blitter done interrupt */
#define IF_VERTB    0x0020	/* vertical blanking interrupt */
#define IF_COPER    0x0010	/* copper interrupt */
#define IF_PORTS    0x0008	/* external level 2 and CIA A interrupt */
#define IF_SOFT     0x0004	/* software initiated interrupt */
#define IF_DSKBLK   0x0002	/* diskblock DMA finished */
#define IF_TBE	    0x0001	/* serial transmit buffer empty interrupt */

/* CIA interrupt control register bits */

#define CIA_ICR_TA	0x01
#define CIA_ICR_TB	0x02
#define CIA_ICR_ALRM	0x04
#define CIA_ICR_SP	0x08
#define CIA_ICR_FLG	0x10
#define CIA_ICR_ALL	0x1f
#define CIA_ICR_SETCLR	0x80

extern void amiga_init_IRQ(void);

/* to access the interrupt control registers of CIA's use only
** these functions, they behave exactly like the amiga os routines
*/

extern struct ciabase ciaa_base, ciab_base;

extern void cia_init_IRQ(struct ciabase *base);
extern unsigned char cia_set_irq(struct ciabase *base, unsigned char mask);
extern unsigned char cia_able_irq(struct ciabase *base, unsigned char mask);

#endif /* asm-m68k/amigaints.h */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
** asm-m68k/amigayle.h -- This header defines the registers of the gayle chip
**                        found on the Amiga 1200
**                        This information was found by disassembling card.resource,
**                        so the definitions may not be 100% correct
**                        anyone has an official doc ?
**
** Copyright 1997 by Alain Malek
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
** Created: 11/28/97 by Alain Malek
*/

#ifndef _M68K_AMIGAYLE_H_
#define _M68K_AMIGAYLE_H_

#include <linux/types.h>
#include <asm/amigahw.h>

/* memory layout */

#define GAYLE_RAM		(0x600000+zTwoBase)
#define GAYLE_RAMSIZE		(0x400000)
#define GAYLE_ATTRIBUTE		(0xa00000+zTwoBase)
#define GAYLE_ATTRIBUTESIZE	(0x020000)
#define GAYLE_IO		(0xa20000+zTwoBase)	/* 16bit and even 8bit registers */
#define GAYLE_IOSIZE		(0x010000)
#define GAYLE_IO_8BITODD	(0xa30000+zTwoBase)	/* odd 8bit registers */

/* offset for accessing odd IO registers */
#define GAYLE_ODD		(GAYLE_IO_8BITODD-GAYLE_IO-1)

/* GAYLE registers */

struct GAYLE {
	u_char cardstatus;
	u_char pad0[0x1000-1];

	u_char intreq;
	u_char pad1[0x1000-1];

	u_char inten;
	u_char pad2[0x1000-1];

	u_char config;
	u_char pad3[0x1000-1];
};

#define GAYLE_ADDRESS	(0xda8000)	/* gayle main registers base address */

#define GAYLE_RESET	(0xa40000)	/* write 0x00 to start reset,
                                           read 1 byte to stop reset */

#define gayle (*(volatile struct GAYLE *)(zTwoBase+GAYLE_ADDRESS))
#define gayle_reset (*(volatile u_char *)(zTwoBase+GAYLE_RESET))

#define gayle_attribute ((volatile u_char *)(GAYLE_ATTRIBUTE))

#if 0
#define gayle_inb(a) readb( GAYLE_IO+(a)+(((a)&1)*GAYLE_ODD) )
#define gayle_outb(v,a) writeb( v, GAYLE_IO+(a)+(((a)&1)*GAYLE_ODD) )

#define gayle_inw(a) readw( GAYLE_IO+(a) )
#define gayle_outw(v,a) writew( v, GAYLE_IO+(a) )
#endif

/* GAYLE_CARDSTATUS bit def */

#define GAYLE_CS_CCDET		0x40	/* credit card detect */
#define GAYLE_CS_BVD1		0x20	/* battery voltage detect 1 */
#define GAYLE_CS_SC		0x20	/* credit card status change */
#define GAYLE_CS_BVD2		0x10	/* battery voltage detect 2 */
#define GAYLE_CS_DA		0x10	/* digital audio */
#define GAYLE_CS_WR		0x08	/* write enable (1 == enabled) */
#define GAYLE_CS_BSY		0x04	/* credit card busy */
#define GAYLE_CS_IRQ		0x04	/* interrupt request */

/* GAYLE_IRQ bit def */

#define GAYLE_IRQ_IDE		0x80
#def