#define NIPR_STEP_VALUE_SHIFT	0
#define NIPR_SELECT_MASK	0x0700	/* Tap Selection */
#define NIPR_SELECT_SHIFT	8
#define NIPR_PRE_SEL		0x8000	/* Non-integer prescaler select */


/* generalization of uart control registers to support multiple ports: */
typedef struct {
  volatile unsigned short int ustcnt;
  volatile unsigned short int ubaud;
  union {
    volatile unsigned short int w;
    struct {
      volatile unsigned char status;
      volatile unsigned char rxdata;
    } b;
  } urx;
  union {
    volatile unsigned short int w;
    struct {
      volatile unsigned char status;
      volatile unsigned char txdata;
    } b;
  } utx;
  volatile unsigned short int umisc;
  volatile unsigned short int nipr;
  volatile unsigned short int hmark;
  volatile unsigned short int unused;
} __attribute__((packed)) m68328_uart;




/**********
 *
 * 0xFFFFFAxx -- LCD Controller
 *
 **********/

/*
 * LCD Screen Starting Address Register 
 */
#define LSSA_ADDR	0xfffffa00
#define LSSA		LONG_REF(LSSA_ADDR)

#define LSSA_SSA_MASK	0x1ffffffe	/* Bits 0 and 29-31 are reserved */

/*
 * LCD Virtual Page Width Register 
 */
#define LVPW_ADDR	0xfffffa05
#define LVPW		BYTE_REF(LVPW_ADDR)

/*
 * LCD Screen Width Register (not compatible with '328 !!!) 
 */
#define LXMAX_ADDR	0xfffffa08
#define LXMAX		WORD_REF(LXMAX_ADDR)

#define LXMAX_XM_MASK	0x02f0		/* Bits 0-3 and 10-15 are reserved */

/*
 * LCD Screen Height Register
 */
#define LYMAX_ADDR	0xfffffa0a
#define LYMAX		WORD_REF(LYMAX_ADDR)

#define LYMAX_YM_MASK	0x01ff		/* Bits 9-15 are reserved */

/*
 * LCD Cursor X Position Register
 */
#define LCXP_ADDR	0xfffffa18
#define LCXP		WORD_REF(LCXP_ADDR)

#define LCXP_CC_MASK	0xc000		/* Cursor Control */
#define   LCXP_CC_TRAMSPARENT	0x0000
#define   LCXP_CC_BLACK		0x4000
#define   LCXP_CC_REVERSED	0x8000
#define   LCXP_CC_WHITE		0xc000
#define LCXP_CXP_MASK	0x02ff		/* Cursor X position */

/*
 * LCD Cursor Y Position Register
 */
#define LCYP_ADDR	0xfffffa1a
#define LCYP		WORD_REF(LCYP_ADDR)

#define LCYP_CYP_MASK	0x01ff		/* Cursor Y Position */

/*
 * LCD Cursor Width and Heigth Register
 */
#define LCWCH_ADDR	0xfffffa1c
#define LCWCH		WORD_RE