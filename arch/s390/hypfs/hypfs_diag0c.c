atch interrupt enable */
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
#define STPWCH		WORD_REF(STPWCH_ADDR)

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
#define DRAMC_WS_