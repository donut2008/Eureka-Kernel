/*
 * ADB through the IOP
 * Written by Joshua M. Thompson
 */

/* IOP number and channel number for ADB */

#define ADB_IOP		IOP_NUM_ISM
#define ADB_CHAN	2

/* From the A/UX headers...maybe important, maybe not */

#define ADB_IOP_LISTEN	0x01
#define ADB_IOP_TALK	0x02
#define ADB_IOP_EXISTS	0x04
#define ADB_IOP_FLUSH	0x08
#define ADB_IOP_RESET	0x10
#define ADB_IOP_INT	0x20
#define ADB_IOP_POLL	0x40
#define ADB_IOP_UNINT	0x80

#define AIF_RESET	0x00
#define AIF_FLUSH	0x01
#define AIF_LISTEN	0x08
#define AIF_TALK	0x0C

/* Flag bits in struct adb_iopmsg */

#define ADB_IOP_EXPLICIT	0x80	/* nonzero if explicit command */
#define ADB_IOP_AUTOPOLL	0x40	/* auto/SRQ polling enabled    */
#define ADB_IOP_SRQ		0x04	/* SRQ detected                */
#define ADB_IOP_TIMEOUT		0x02	/* nonzero if timeout          */

#ifndef __ASSEMBLY__

struct adb_iopmsg {
	__u8 flags;		/* ADB flags         */
	__u8 count;		/* no. of data bytes */
	__u8 cmd;		/* ADB command       */
	__u8 data[8];		/* ADB data          */
	__u8 spare[21];		/* spare             */
};

#endif /* __ASSEMBLY__ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
** asm-m68k/amigahw.h -- This header defines some macros and pointers for
**                    the various Amiga custom hardware registers.
**                    The naming conventions used here conform to those
**                    used in the Amiga Hardware Reference Manual, 3rd Edition
**
** Copyright 1992 by Greg Harp
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
** Created: 9/24/92 by Greg Harp
*/

#ifndef _M68K_AMIGAHW_H
#define _M68K_AMIGAHW_H

#include <linux/ioport.h>

#include <asm/bootinfo-amiga.h>


    /*
     *  Chipsets
     */

extern unsigned long amiga_chipset;


    /*
     *  Miscellaneous
     */

extern unsigned long amiga_eclock;	/* 700 kHz E Peripheral Clock */
extern unsigned long amiga_colorclock;	/* 3.5 MHz Color Clock */
extern unsigned long amiga_chip_size;	/* Chip RAM Size (bytes) */
extern unsigned char amiga_vblank;	/* VBLANK Frequency */


#define AMIGAHW_DECLARE(name)	unsigned name : 1
#define AMIGAHW_SET(name)	(amiga_hw_present.name = 1)
#define AMIGAHW_PRESENT(name)	(amiga_hw_present.name)

struct amiga_hw_present {
    /* video hardware */
    AMIGAHW_DECLARE(AMI_VIDEO);		/* Amiga Video */
    AMIGAHW_DECLARE(AMI_BLITTER);	/* Amiga Blitter */
    AMIGAHW_DECLARE(AMBER_FF);		/* Amber Flicker Fixer */
    /* sound hardware */
    AMIGAHW_DECLARE(AMI_AUDIO);		/* Amiga Audio */
    /* disk storage interfaces */
    AMIGAHW_DECLARE(AMI_FLOPPY);	/* Amiga Floppy */
    AMIGAHW_DECLARE(A3000_SCSI);	/* SCSI (wd33c93, A3000 alike) */
    AMIGAHW_DECLARE(A4000_SCSI);	/* SCSI (ncr53c710, A4000T alike) */
    AMIGAHW_DECLARE(A1200_IDE);		/* IDE (A1200 alike) */
    AMIGAHW_DECLARE(A4000_IDE);		/* IDE (A4000 alike) */
    AMIGAHW_DECLARE(CD_ROM);		/* CD ROM drive */
    /* other I/O hardware */
    AMIGAHW_DECLARE(AMI_KEYBOARD);	/* Amiga Keyboard */
    AMIGAHW_DECLARE(AMI_MOUSE);		/* Amiga Mouse */
    AMIGAHW_DECLARE(AMI_SERIAL);	/* Amiga Serial */
    AMIGAHW_DECLARE(AMI_PARALLEL);	/* Amiga Parallel */
    /* real time clocks */
    AMIGAHW_DECLARE(A2000_CLK);		/* Hardware Clock (A2000 alike) */
    AMIGAHW_DECLARE(A3000_CLK);		/* Hardware Clock (A3000 alike) */
    /* supporting hardware */
    AMIGAHW_DECLARE(CHIP_RAM);		/* Chip RAM */
    AMIGAHW_DECLARE(PAULA);		/* Paula (8364) */
    AMIGAHW_DECLARE(DENISE);		/* Denise (8362) */
    AMIGAHW_DECLARE(DENISE_HR);		/* Denise (8373) */
    AMIGAHW_DECLARE(LISA);		/* Lisa (8375) */
    AMIGAHW_DECLARE(AGNUS_PAL);		/* Normal/Fat PAL Agnus (8367/8371) */
    AMIGAHW_DECLARE(AGNUS_NTSC);	/* Normal/Fat NTSC Agnus (8361/8370) */
    AMIGAHW_DECLARE(AGNUS_HR_PAL);	/* Fat Hires PAL Agnus (8372) */
    AMIGAHW_DECLARE(AGNUS_HR_NTSC);	/* Fat Hires NTSC Agnus (8372) */
    AMIGAHW_DECLAR