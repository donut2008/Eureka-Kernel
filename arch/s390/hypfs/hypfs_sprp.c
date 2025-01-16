RW	0x0002	/* Read/Write Cycle Selection */

/*
 * ICE Module Control Mask Register
 */
#define ICEMCMR_ADDR	0xfffffd0a
#define ICEMCMR		WORD_REF(ICEMCMR_ADDR)

#define ICEMCMR_PDM	0x0001	/* Program/Data Cycle Mask */
#define ICEMCMR_RWM	0x0002	/* Read/Write Cycle Mask */

/*
 * ICE Module Control Register 
 */
#define ICEMCR_ADDR	0xfffffd0c
#define ICEMCR		WORD_REF(ICEMCR_ADDR)

#define ICEMCR_CEN	0x0001	/* Compare Enable */
#define ICEMCR_PBEN	0x0002	/* Program Break Enable */
#define ICEMCR_SB	0x0004	/* Single Breakpoint */
#define ICEMCR_HMDIS	0x0008	/* HardMap disable */
#define ICEMCR_BBIEN	0x0010	/* Bus Break Interrupt Enable */

/*
 * ICE Module Status Register 
 */
#define ICEMSR_ADDR	0xfffffd0e
#define ICEMSR		WORD_REF(ICEMSR_ADDR)

#define ICEMSR_EMUEN	0x0001	/* Emulation Enable */
#define ICEMSR_BRKIRQ	0x0002	/* A-Line Vector Fetch Detected */
#define ICEMSR_BBIRQ	0x0004	/* Bus Break Interrupt Detected */
#define ICEMSR_EMIRQ	0x0008	/* EMUIRQ Falling Edge Detected */

#endif /* _MC68VZ328_H_ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       