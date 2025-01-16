DR	0xfffffa23
#define LACDRC		BYTE_REF(LACDRC_ADDR)

#define LACDRC_ACDSLT	 0x80	/* Signal Source Select */
#define LACDRC_ACD_MASK	 0x0f	/* Alternate Crystal Direction Control */
#define LACDRC_ACD_SHIFT 0

/*
 * LCD Pixel Clock Divider Register
 */
#define LPXCD_ADDR	0xfffffa25
#define LPXCD		BYTE_REF(LPXCD_ADDR)

#define	LPXCD_PCD_MASK	0x3f 	/* Pixel Clock Divider */
#define LPXCD_PCD_SHIFT	0

/*
 * LCD Clocking Control Register
 */
#define LCKCON_ADDR	0xfffffa27
#define LCKCON		BYTE_REF(LCKCON_ADDR)

#define LCKCON_DWS_MASK	 0x0f	/* Display Wait-State */
#define LCKCON_DWS_SHIFT 0
#define LCKCON_DWIDTH	 0x40	/* Display Memory Width  */
#define LCKCON_LCDON	 0x80	/* Enable LCD Controller */

/* '328-compatible definitions */
#define LCKCON_DW_MASK  LCKCON_DWS_MASK
#define LCKCON_DW_SHIFT LCKCON_DWS_SHIFT
 
/*
 * LCD Refresh Rate Adjustment Register 
 */
#define LRRA_ADDR	0xfffffa29
#define LRRA		BYTE_REF(LRRA_ADDR)

/*
 * LCD Panning Offset Register
 */
#define LPOSR_ADDR	0xfffffa2d
#define LPOSR		BYTE_REF(LPOSR_ADDR)

#define LPOSR_POS_MASK	0x0f	/* Pixel Offset Code */
#define LPOSR_POS_SHIFT	0

/*
 * LCD Frame Rate Control Modulation Register
 */
#define LFRCM_ADDR	0xfffffa31
#define LFRCM		BYTE_REF(LFRCM_ADDR)

#define LFRCM_YMOD_MASK	 0x0f	/* Vertical Modulation */
#define LFRCM_YMOD_SHIFT 0
#define LFRCM_XMOD_MASK	 0xf0	/* Horizontal Modulation */
#define LFRCM_XMOD_SHIFT 4

/*
 * LCD Gray Palette Mapping Register
 */
#define LGPMR_ADDR	0xfffffa33
#define LGPMR		BYTE_REF(LGPMR_ADDR)

#define LGPMR_G1_MASK	0x0f
#define LGPMR_G1_SHIFT	0
#define LGPMR_G2_MASK	0xf0
#define LGPMR_G2_SHIFT	4

/* 
 * PWM Contrast Control Register
 */
#define PWMR_ADDR	0xfffffa36
#define PWMR		WORD_REF(PWMR_ADDR)

#define PWMR_PW_MASK	0x00ff	/* Pulse Width */
#define PWMR_PW_SHIFT	0
#define PWMR_CCPEN	0x0100	/* Contrast Control Enable */
#define PWMR_SRC_MASK	0x0600	/* Input Clock Source */
#define   PWMR_SRC_LINE	  0x0000	/* Line Pulse  */
#define   PWMR_SRC_PIXEL  0x0200	/* Pixel Clock */
#define   PWMR_SRC_LCD    0x4000	/* LCD clock   */

/**********
 *
 * 0xFFFFFBxx -- Real-Time Clock (RTC)
 *
 **********/

/*
 * RTC Hours Minutes and Seconds Register
 */
#define RTCTIME_ADDR	0xfffffb00
#define RTCTIME		LONG_REF(RTCTIME_ADDR)

#define RTCTIME_SECONDS_MASK	0x0000003f	/* Seconds */
#define RTCTIME_SECONDS_SHIFT	0
#define RTCTIME_MINUTES_MASK	0x003f0000	/* Minutes */
#define RTCTIME_MINUTES_SHIFT	16
#define RTCTIME_HOURS_MASK	0x1f000000	/* Hours */
#define RTCTIME_HOURS_SHIFT	24

/*
 *  RTC Alarm Register 
 */
#define RTCALRM_ADDR    0xfffffb04
#define RTCALRM         LONG_REF(RTCALRM_ADDR)

#define RTCALRM_SECONDS_MASK    0x0000003f      /* Seconds */
#define RTCALRM_SECONDS_SHIFT   0
#define RTCALRM_MINUTES_MASK    0x003f0000      /* Minutes */
#define RTCALRM_MINUTES_SHIFT   16
#define RTCALRM_HOURS_MASK      0x1f000000      /* Hours */
#define RTCALRM_HOURS_SHIFT     24

/*
 * Watchdog Timer Register 
 */
#define WATCHDOG_ADDR	0xfffffb0a
#define WATCHDOG	WORD_REF(WATCHDOG_ADDR)

#define WATCHDOG_EN	0x0001	/* Watchdog Enabled */
#define WATCHDOG_ISEL	0x0002	/* Select the watchdog interrupt */
#define WATCHDOG_INTF	0x0080	/* Watchdog interrupt occurred */
#define WATCHDOG_CNT_MASK  0x0300	/* Watchdog Counter */
#define WATCHDOG_CNT_SHIFT 8

/*
 * RTC Control Register
 */
#define RTCCTL_ADDR	0xfffffb0c
#define RTCCTL		WORD_REF(RTCCTL_ADDR)

#define RTCCTL_XTL	0x0020	/* Crystal Selection */
#define RTCCT