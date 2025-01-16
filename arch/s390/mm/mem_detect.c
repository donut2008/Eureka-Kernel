((*(volatile char *)0xfffffa39))

/* TT SCC DMA Controller (same chip as SCSI DMA) */

#define	TT_SCC_DMA_BAS	(0xffff8c00)
#define	tt_scc_dma	((*(volatile struct TT_DMA *)TT_SCC_DMA_BAS))

/*
** VIDEL Palette Register
 */

#define FPL_BAS (0xffff9800)
struct VIDEL_PALETTE
 {
  u_long reg[256];
 };
# define videl_palette ((*(volatile struct VIDEL_PALETTE*)FPL_BAS))


/*
** Falcon DSP Host Interface
 */

#define DSP56K_HOST_INTERFACE_BASE (0xffffa200)
struct DSP56K_HOST_INTERFACE {
  u_char icr;
#define DSP56K_ICR_RREQ	0x01
#define DSP56K_ICR_TREQ	0x02
#define DSP56K_ICR_HF0	0x08
#define DSP56K_ICR_HF1	0x10
#define DSP56K_ICR_HM0	0x20
#define DSP56K_ICR_HM1	0x40
#define DSP56K_ICR_INIT	0x80

  u_char cvr;
#define DSP56K_CVR_HV_MASK 0x1f
#define DSP56K_CVR_HC	0x80

  u_char isr;
#define DSP56K_ISR_RXDF	0x01
#define DSP56K_ISR_TXDE	0x02
#define DSP56K_ISR_TRDY	0x04
#define DSP56K_ISR_HF2	0x08
#define DSP56K_ISR_HF3	0x10
#define DSP56K_ISR_DMA	0x40
#define DSP56K_ISR_HREQ	0x80

  u_char ivr;

  union {
    u_char b[4];
    u_short w[2];
    u_long l;
  } data;
};
#define dsp56k_host_interface ((*(volatile struct DSP56K_HOST_INTERFACE *)DSP56K_HOST_INTERFACE_BASE))

/*
** MFP 68901
 */

#define MFP_BAS (0xfffffa01)
struct MFP
 {
  u_char par_dt_reg;
  u_char char_dummy1;
  u_char active_edge;
  u_char char_dummy2;
  u_char data_dir;
  u_char char_dummy3;
  u_char int_en_a;
  u_char char_dummy4;
  u_char int_en_b;
  u_char char_dummy5;
  u_char in