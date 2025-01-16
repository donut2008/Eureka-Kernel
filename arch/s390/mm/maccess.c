/*
** linux/atarihw.h -- This header defines some macros and pointers for
**                    the various Atari custom hardware registers.
**
** Copyright 1994 by Björn Brauel
**
** 5/1/94 Roman Hodek:
**   Added definitions for TT specific chips.
**
** 1996-09-13 lars brinkhoff <f93labr@dd.chalmers.se>:
**   Finally added definitions for the matrix/codec and the DSP56001 host
**   interface.
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
*/

#ifndef _LINUX_ATARIHW_H_
#define _LINUX_ATARIHW_H_

#include <linux/types.h>
#include <asm/bootinfo-atari.h>
#include <asm/raw_io.h>

extern u_long atari_mch_cookie;
extern u_long atari_mch_type;
extern u_long atari_switches;
extern int atari_rtc_year_offset;
extern int atari_dont_touch_floppy_select;

extern int atari_SCC_reset_done;

/* convenience macros for testing machine type */
#define MACH_IS_ST	((atari_mch_cookie >> 16) == ATARI_MCH_ST)
#define MACH_IS_STE	((atari_mch_cookie >> 16) == ATARI_MCH_STE && \
			 (atari_mch_cookie & 0xffff) == 0)
#define MACH_IS_MSTE	((atari_mch_cookie >> 16) == ATARI_MCH_STE && \
			 (atari_mch_cookie & 0xffff) == 0x10)
#define MACH_IS_TT	((atari_mch_cookie >> 16) == ATARI_MCH_TT)
#define MACH_IS_FALCON	((atari_mch_cookie >> 16) == ATARI_MCH_FALCON)
#define MACH_IS_MEDUSA	(atari_mch_type == ATARI_MACH_MEDUSA)
#define MACH_IS_AB40	(atari_mch_type == ATARI_MACH_AB40)

/* values for atari_switches */
#define ATARI_SWITCH_IKBD	0x01
#define ATARI_SWITCH_MIDI	0x02
#define ATARI_SWITCH_SND6	0x04
#define ATARI_SWITCH_SND7	0x08
#define ATARI_SWITCH_OVSC_SHIFT	16
#define ATARI_SWITCH_OVSC_IKBD	(ATARI_SWITCH_IKBD << ATARI_SWITCH_OVSC_SHIFT)
#define ATARI_SWITCH_OVSC_MIDI	(ATARI_SWITCH_MIDI << ATARI_SWITCH_OVSC_SHIFT)
#define ATARI_SWITCH_OVSC_SND6	(ATARI_SWITCH_SND6 << ATARI_SWITCH_OVSC_SHIFT)
#define ATARI_SWITCH_OVSC_SND7	(ATARI_SWITCH_SND7 << ATARI_SWITCH_OVSC_SHIFT)
#define ATARI_SWITCH_OVSC_MASK	0xffff0000

/*
 * Define several Hardware-Chips for indication so that for the ATARI we do
 * no longer decide whether it is a Falcon or other machine . It's just
 * important what hardware the machine uses
 */

/* ++roman 08/08/95: rewritten from ORing constants to a C bitfield */

#define ATARIHW_DECLARE(name)	unsigned name : 1
#define ATARIHW_SET(name)	(atari_hw_present.name = 1)
#define ATARIHW_PRESENT(name)	(atari_hw_present.name)

struct atari_hw_present {
    /* video hardware */
    ATARIHW_DECLARE(STND_SHIFTER);	/* ST-Shifter - no base low ! */
    ATARIHW_DECLARE(EXTD_SHIFTER);	/* STe-Shifter - 24 bit address */
    ATARIHW_DECLARE(TT_SHIFTER);	/* TT-Shifter */
    ATARIHW_DECLARE(VIDEL_SHIFTER);	/* Falcon-Shifter */
    /* sound hardware */
    ATARIHW_DECLARE(YM_2149);		/* Yamaha YM 2149 */
    ATARIHW_DECLARE(PCM_8BIT);		/* PCM-Sound in STe-ATARI */
    ATARIHW_DECLARE(CODEC);		/* CODEC Sound (Falcon) */
    /* disk storage interfaces */
    ATARIHW_DECLARE(TT_SCSI);		/* Directly mapped NCR5380 */
    ATARIHW_DECLARE(ST_SCSI);		/* NCR5380 via ST-DMA (Falcon) */
    ATARIHW_DECLARE(ACSI);		/* Standard ACSI like in STs */
    ATARIHW_DECLARE(IDE);		/* IDE Interface */
    ATARIHW_DECLARE(FDCSPEED);		/* 8/16 MHz switch for FDC */
    /* other I/O hardware */
    ATARIHW_DECLARE(ST_MFP);		/* The ST-MFP (there should be no Atari
					   without it... but who knows?) */
    ATARIHW_DECLARE(TT_MFP);		/* 2nd MFP */
    ATARIHW_DECLARE(SCC);		/* Serial Communications Contr. */
    ATARIHW_DECLARE(ST_ESCC);		/* SCC Z83230 in an ST */
    ATARIHW_DECLARE(ANALOG_JOY);	/* Paddle Interface for STe
					   and Falcon */
    ATARIHW_DECLARE(MICROWIRE);		/* Microwire Interface */
    /* DMA */
    ATARIHW_DECLARE(STND_DMA);		/* 24 Bit limited ST-DMA */
    ATARIHW_DECLARE(EXTD_DMA);		/* 32 Bit ST-DMA */
    ATARIHW_DECLARE(SCSI_DMA);		/* DMA for the NCR5380 */
    ATARIHW_DECLARE(SCC_DMA);		/* DMA for the SCC */
    /* real time clocks */
    ATARIHW_DECLARE(TT_CLK);		/* TT compatible clock chip */
    ATARIHW_DECLARE(MSTE_CLK);		/* Mega ST(E) clock chip */
    /* supporting hardware */
    ATARIHW_DECLARE(SCU);		/* System Control Unit */
    ATARIHW_DECLARE(BLITTER);		/* Blitter */
    ATARIHW_DECLARE(VME);		/* VME Bus */
    ATARIHW_DECLARE(DSP56K);		/* DSP56k processor in Falcon */
};

extern struct atari_hw_present atari_hw_present;


/* Reading the MFP port register gives a machine independent delay, since the
 * MFP always has a 8 MHz clock. This avoids problems with the varying length
 * of nops on various machines. Somebody claimed that the tstb takes 600 ns.
 */
#define	MFPDELAY() \
	__asm__ __volatile__ ( "tstb %0" : : "m" (st_mfp.par_dt_reg) : "cc" );

/* Do cache push/invalidate for DMA read/write. This function obeys the
 * snooping on some machines (Medusa) and processors: The Medusa itself can
 * snoop, but only the '040 can source data from its cache to DMA writes i.e.,
 * reads from memory). Both '040 and '060 invalidate cache entries on snooped