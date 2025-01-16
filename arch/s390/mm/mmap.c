igned short bltcon1;
    unsigned short bltafwm;
    unsigned short bltalwm;
    unsigned char  *bltcpt;
    unsigned char  *bltbpt;
    unsigned char  *bltapt;
    unsigned char  *bltdpt;
    unsigned short bltsize;
    unsigned char  pad2d;
    unsigned char  bltcon0l;
    unsigned short bltsizv;
    unsigned short bltsizh;
    unsigned short bltcmod;
    unsigned short bltbmod;
    unsigned short bltamod;
    unsigned short bltdmod;
    unsigned short spare2[4];
    unsigned short bltcdat;
    unsigned short bltbdat;
    unsigned short bltadat;
    unsigned short spare3[3];
    unsigned short deniseid;
    unsigned short dsksync;
    unsigned short *cop1lc;
    unsigned short *cop2lc;
    unsigned short copjmp1;
    unsigned short copjmp2;
    unsigned short copins;
    unsigned short diwstrt;
    unsigned short diwstop;
    unsigned short ddfstrt;
    unsigned short ddfstop;
    unsigned short dmacon;
    unsigned short clxcon;
    unsigned short intena;
    unsigned short intreq;
    unsigned short adkcon;
    struct {
	unsigned short	*audlc;
	unsigned short audlen;
	unsigned short audper;
	unsigned short audvol;
	unsigned short auddat;
	unsigned short audspare[2];
    } aud[4];
    unsigned char  *bplpt[8];
    unsigned short bplcon0;
    unsigned short bplcon1;
    unsigned short bplcon2;
    unsigned short bplcon3;
    unsigned short bpl1mod;
    unsigned short bpl2mod;
    unsigned short bplcon4;
    unsigned short clxcon2;
    unsigned short bpldat[8];
    unsigned char  *sprpt[8];
    struct {
	unsigned short pos;
	unsigned short ctl;
	unsigned short dataa;
	unsigned short datab;
    } spr[8];
    unsigned short color[32];
    unsigned short htotal;
    unsigned short hsstop;
    unsigned short hbstrt;
    unsigned short hbstop;
    unsigned short vtotal;
    unsigned short vsstop;
    unsigned short vbstrt;
    unsigned short vbstop;
    unsigned short sprhstrt;
    unsigned short sprhstop;
    unsigned short bplhstrt;
    unsigned short bplhstop;
    unsigned short hhposw;
    unsigned short hhposr;
    unsigned short beamcon0;
    unsigned short hsstrt;
    unsigned short vsstrt;
    unsigned short hcenter;
    unsigned short diwhigh;
    unsigned short spare4[11];
    unsigned short fmode;
};

/*
 * DMA register bits
 */
#define DMAF_SETCLR		(0x8000)
#define DMAF_AUD0		(0x0001)
#define DMAF_AUD1		(0x0002)
#define DMAF_AUD2		(0x0004)
#define DMAF_AUD3		(0x0008)
#define DMAF_DISK		(0x0010)
#define DMAF_SPRITE		(0x0020)
#define DMAF_BLITTER		(0x0040)
#define DMAF_COPPER		(0x0080)
#define DMAF_RASTER		(0x0100)
#define DMAF_MASTER		(0x0200)
#define DMAF_BLITHOG		(0x0400)
#define DMAF_BLTNZERO		(0x2000)
#define DMAF_BLTDONE		(0x4000)
#define DMAF_ALL		(0x01FF)

struct CIA {
    unsigned char pra;		char pad0[0xff];
    unsigned char prb;		char pad1[0xff];
    unsigned char ddra;		char pad2[0xff];
    unsigned char ddrb;		char pad3[0xff];
    unsigned char talo;		char pad4[0xff];
    unsigned char tahi;		char pad5[0xff];
    unsigned char tblo;		char pad6[0xff];
    unsigned char tbhi;		char pad7[0xff];
    unsigned char todlo;	char pad8[0xff];
    unsigned char todmid;	char pad9[0xff];
    unsigned char todhi;	char pada[0x1ff];
    unsigned char sdr;		char padb[0xff];
    unsigned char icr;		char padc[0xff];
    unsigned char cra;		char padd[0xff];
    unsigned char crb;		char pade[0xff];
};

#define zTwoBase (0x80000000)
#define ZTWO_PADDR(x) (((unsigned long)(x))-zTwoBase)
#define ZTWO_VADDR(x) ((void __iomem *)(((unsigned long)(x))+zTwoBase))

#define CUSTOM_PHYSADDR     (0xdff000)
#define amiga_custom ((*(volatile struct CUSTOM *)(zTwoBase+CUSTOM_PHYSADDR)))

#define CIAA_PHYSADDR	  (0xbfe001)
#define CIAB_PHYSADDR	  (0xbfd000)
#define ciaa   ((*(volatile struct CIA *)(zTwoBase + CIAA_PHYSADDR)))
#define ciab   ((*(volatile struct CIA *)(zTwoBase + CIAB_PHYSADDR)))

#define CHIP_PHYSADDR	    (0x000000)

void amiga_chip_init (void);
void *amiga_chip_alloc(unsigned long size, const char *name);
void *amiga_chip_alloc_res(unsigned long size, struct resource *res);
void amiga_chip_free(void *ptr);
unsigned long amiga_chip_avail( void ); /*MILAN*/
extern volatile unsigned short amiga_audio_min_period;

static inline void amifb_video_off(void)
{
	if (amiga_chipset == CS_ECS || amiga_chipset == CS_AGA) {
		/* program Denise/Lisa for a higher maximum play rate */
		amiga_custom.htotal = 113;        /* 31 kHz */
		amiga_custom.vtotal = 223;        /* 70 Hz */
		amiga_custom.beamcon0 = 0x4390;   /* HARDDIS, VAR{BEAM,VSY,HSY,CSY}EN */
		/* suspend the monitor */
		amiga_custom.hsstrt = amiga_custom.hsstop = 116;
		amiga_custom.vsstrt = amiga_custom.vsstop = 226;
		amiga_audio_min_period = 57;
	}
}

struct tod3000 {
  unsigned int  :28, second2:4;	/* lower digit */
  unsigned int  :28, second1:4;	/* upper digit */
  unsigned int  :28, minute2:4;	/* lower digit */
  unsigned int  :28, minute1:4;	/* upper digit */
  unsigned int  :28, hour2:4;	/* lower digit */
  unsigned int  :28, hour1:4;	/* upper digit */
  unsigned int  :28, weekday:4;
  unsigned int  :28, day2:4;	/* lower digit */
  unsigned int  :28, day1:4;	/* upper digit */
  unsigned int  :28, month2:4;	/* lower digit */
  unsigned int  :28, month1:4;	/* upper digit */
  unsigned int  :28, year2:4;	/* lower digit */
  unsigned int  :28, year1:4;	/* upper digit */
  unsigned int  :28, cntrl1:4;	/* control-byte 1 */
  unsigned int  :28, cntrl2:4;	/* control-byte 2 */
  unsigned int  :28, cntrl3:4;	/* control-byte 3 */
};
#define TOD3000_CNTRL1_HOLD	0
#define TOD3000_CNTRL1_FREE	9
#define tod_3000 ((*(volatile struct tod3000 *)(zTwoBase+0xDC0000)))

struct tod2000 {
  unsigned int  :28, second2:4;	/* lower digit */
  unsigned int  :28, second1:4;	/* upper digit */
  unsigned int  :28, minute2:4;	/* lower digit */
  unsigned int  :28, minute1:4;	/* upper digit */
  unsigned int  :28, hour2:4;	/* lower digit */
  unsigned int  :28, hour1:4;	/* upper digit */
  unsigned int  :28, day2:4;	/* lower digit */
  unsigned int  :28, day1:4;	/* upper digit */
  unsigned int  :28, month2:4;	/* lower digit */
  unsigned int  :28, month1:4;	/* upper digit */
  unsigned int  :28, year2:4;	/* lower digit */
  unsigned int  :28, year1:4;	/* upper digit */
  unsigned int  :28, weekday:4;
  unsigned int  :28, cntrl1:4;	/* control-byte 1 */
  unsigned int  :28, cntrl2:4;	/* control-byte 2 */
  unsigned int  :28, cntrl3:4;	/* control-byte 3 */
};

#define TOD2000_CNTRL1_HOLD	(1<<0)
#define TOD2000_CNTRL1_BUSY	(1<<1)
#define TOD2000_CNTRL3_24HMODE	(1<<2)
#define TOD2000_HOUR1_PM	(1<<2)
#define tod_2000 ((*(volatile struct tod