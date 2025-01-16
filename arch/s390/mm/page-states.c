of the ACIA */
#define ACIA_RDRF 1		/* Receive Data Register Full */
#define ACIA_TDRE (1<<1)	/* Transmit Data Register Empty */
#define ACIA_DCD  (1<<2)	/* Data Carrier Detect */
#define ACIA_CTS  (1<<3)	/* Clear To Send */
#define ACIA_FE   (1<<4)	/* Framing Error */
#define ACIA_OVRN (1<<5)	/* Receiver Overrun */
#define ACIA_PE   (1<<6)	/* Parity Error */
#define ACIA_IRQ  (1<<7)	/* Interrupt Request */

#define ACIA_BAS (0xfffffc00)
struct ACIA
 {
  u_char key_ctrl;
  u_char char_dummy1;
  u_char key_data;
  u_char char_dummy2;
  u_char mid_ctrl;
  u_char char_dummy3;
  u_char mid_data;
 };
# define acia ((*(volatile struct ACIA*)ACIA_BAS))

#define	TT_DMASND_BAS (0xffff8900)
struct TT_DMASND {
	u_char	int_ctrl;	/* Falcon: Interrupt control */
	u_char	ctrl;
	u_char	pad2;
	u_char	bas_hi;
	u_char	pad3;
	u_char	bas_mid;
	u_char	pad4;
	u_char	bas_low;
	u_char	pad5;
	u_char	addr_hi;
	u_char	pad6;
	u_char	addr_mid;
	u_char	pad7;
	u_char	addr_low;
	u_char	pad8;
	u_char	end_hi;
	u_char	pad9;
	u_char	end_mid;
	u_char	pad10;
	u_char	end_low;
	u_char	pad11[12];
	u_char	track_select;	/* Falcon */
	u_char	mode;
	u_char	pad12[14];
	/* Falcon only: */
	u_short	cbar_src;
	u_short cbar_dst;
	u_char	ext_div;
	u_char	int_div;
	u_char	rec_track_select;
	u_char	dac_src;
	u_char	adc_src;
	u_char	input_gain;
	u_short	output_atten;
};
# define tt_dmasnd ((*(volatile struct TT_DMASND *)TT_DMASND_BAS))

#define DMASND_MFP_INT_REPLAY     0x01
#define DMASND_MFP_INT_RECORD     0x02
#define DMASND_TIMERA_INT_REPLAY  0x04
#define DMASND_TIMERA_INT_RECORD  0x08

#define	DMASND_CTRL_OFF		  0x00
#define	DMASND_CTRL_ON		  0x01
#define	DMASND_CTRL_REPEAT	  0x02
#define DMASND_CTRL_RECORD_ON     0x10
#define DMASND_CTRL_RECORD_OFF    0x00
#define DMASND_CTRL_RECORD_REPEAT 0x20
#define DMASND_CTRL_SELECT_REPLAY 0x00
#define DMASND_CTRL_SELECT_RECORD 0x80
#define	DMASND_MODE_MONO	  0x80
#define	DMASND_MODE_STEREO	  0x00
#define DMASND_MODE_8BIT	  0x00
#define DMASND_MODE_16BIT	  0x40	/* Falcon only */
#define	DMASND_MODE_6KHZ	  0x00	/* Falcon: mute */
#define	DMASND_MODE_12KHZ	  0x01
#define	DMASND_MODE_25KHZ	  0x02
#define	DMASND_MODE_50KHZ	  0x03


#define DMASNDSetBase(bufstart)						\
    do {								\
	tt_dmasnd.bas_hi  = (unsigned char)(((bufstart) & 0xff0000) >> 16); \
	tt_dmasnd.bas_mid = (unsigned char