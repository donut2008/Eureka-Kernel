define SPI_IRQ_NUM	SPIM_IRQ_NUM
#define TMR_IRQ_NUM	TMR1_IRQ_NUM
 
/*
 * Here go the bitmasks themselves
 */
#define IMR_MSPIM 	(1 << SPIM_IRQ_NUM)	/* Mask SPI Master interrupt */
#define	IMR_MTMR2	(1 << TMR2_IRQ_NUM)	/* Mask Timer 2 interrupt */
#define IMR_MUART	(1 << UART_IRQ_NUM)	/* Mask UART interrupt */	
#define	IMR_MWDT	(1 << WDT_IRQ_NUM)	/* Mask Watchdog Timer interrupt */
#define IMR_MRTC	(1 << RTC_IRQ_NUM)	/* Mask RTC interrupt */
#define	IMR_MKB		(1 << KB_IRQ_NUM)	/* Mask Keyboard Interrupt */
#define IMR_MPWM	(1 << PWM_IRQ_NUM)	/* Mask Pulse-Width Modulator int. */
#define	IMR_MINT0	(1 << INT0_IRQ_NUM)	/* Mask External INT0 */
#define	IMR_MINT1	(1 << INT1_IRQ_NUM)	/* Mask External INT1 */
#define	IMR_MINT2	(1 << INT2_IRQ_NUM)	/* Mask External INT2 */
#define	IMR_MINT3	(1 << INT3_IRQ_NUM)	/* Mask External INT3 */
#define	IMR_MINT4	(1 << INT4_IRQ_NUM)	/* Mask External INT4 */
#define	IMR_MINT5	(1 << INT5_IRQ_NUM)	/* Mask External INT5 */
#define	IMR_MINT6	(1 << INT6_IRQ_NUM)	/* Mask External INT6 */
#define	IMR_MINT7	(1 << INT7_IRQ_NUM)	/* Mask External INT7 */
#define IMR_MIRQ1	(1 << IRQ1_IRQ_NUM)	/* Mask IRQ1 */
#define IMR_MIRQ2	(1 << IRQ2_IRQ_NUM)	/* Mask IRQ2 */
#define IMR_MIRQ3	(1 << IRQ3_IRQ_NUM)	/* Mask IRQ3 */
#define IMR_MIRQ6	(1 << IRQ6_IRQ_NUM)	/* Mask IRQ6 */
#define IMR_MPEN	(1 << PEN_IRQ_NUM)	/* Mask Pen Interrupt */
#define IMR_MSPIS	(1 << SPIS_IRQ_NUM)	/* Mask SPI Slave Interrupt */
#define IMR_MTMR1	(1 << TMR1_IRQ_NUM)	/* Mask Timer 1 interrupt */
#define IMR_MIRQ7	(1 << IRQ7_IRQ_NUM)	/* Mask IRQ7 */

/* 'EZ328-compatible definitions */
#define IMR_MSPI	IMR_MSPIM
#define IMR_MTMR	IMR_MTMR1

/* 
 * Interrupt Wake-Up Enable Register
 */
#define IWR_ADDR	0xfffff308
#define IWR		LONG_REF(IWR_ADDR)

#define IWR_SPIM 	(1 << SPIM_IRQ_NUM)	/* SPI Master interrupt */
#define	IWR_TMR2	(1 << TMR2_IRQ_NUM)	/* Timer 2 interrupt */
#define IWR_UART	(1 << UART_IRQ_NUM)	/* UART interrupt */	
#define	IWR_WDT		(1 << WDT_IRQ_NUM)	/* Watchdog Timer interrupt */
#define IWR_RTC		(1 << RTC_IRQ_NUM)	/* RTC interrupt */
#define	IWR_KB		(1 << KB_IRQ_NUM)	/* Keyboard Interrupt */
#define IWR_PWM		(1 << PWM_IRQ_NUM)	/* Pulse-Width Modulator int. */
#define	IWR_INT0	(1 << INT0_IRQ_NUM)	/* External INT0 */
#define	IWR_INT1	(1 << INT1_IRQ_NUM)	/* External INT1 */
#define	IWR_INT2	(1 << INT2_IRQ_NUM)	/* External INT2 */
#define	IWR_INT3	(1 << INT3_IRQ_NUM)	/* External INT3 */
#define	IWR_INT4	(1 << INT4_IRQ_NUM)	/* External INT4 */
#define	IWR_INT5	(1 << INT5_IRQ_NUM)	/* External INT5 */
#define	IWR_INT6	(1 << INT6_IRQ_NUM)	/* External INT6 */
#define	IWR_INT7	(1 << INT7_IRQ_NUM)	/* External INT7 */
#define IWR_IRQ1	(1 << IRQ1_IRQ_NUM)	/* IRQ1 */
#define IWR_IRQ2	(1 << IRQ2_IRQ_NUM)	/* IRQ2 */
#define IWR_IRQ3	(1 << IRQ3_IRQ_NUM)	/* IRQ3 */
#define IWR_IRQ6	(1 << IRQ6_IRQ_NUM)	/* IRQ6 */
#define IWR_PEN		(1 << PEN_IRQ_NUM)	/* Pen Interrupt */
#define IWR_SPIS	(1 << SPIS_IRQ_NUM)	/* SPI Slave Interrupt */
#define IWR_TMR1	(1 << TMR1_IRQ_NUM)	/* Timer 1 interrupt */
#define IWR_IRQ7	(1 << IRQ7_IRQ_NUM)	/* IRQ7 */

/* 
 * Interrupt Status Register 
 */
#define ISR_ADDR	0xfffff30c
#define ISR		LONG_REF(ISR_ADDR)

#define ISR_SPIM 	(1 << SPIM_IRQ_NUM)	/* SPI Master interrupt */
#define	ISR_TMR2	(1 << TMR2_IRQ_NUM)	/* Timer 2 interrupt */
#define ISR_UART	(1 << UART_IRQ_NUM)	/* UART interrupt */	
#define	ISR_WDT		(1 << WDT_IRQ_NUM)	/* Watchdog Timer interrupt */
#define ISR_RTC		(1 << RTC_IRQ_NUM)	/* RTC interrupt */
#define	ISR_KB		(1 << KB_IRQ_NUM)	/* Keyboard Interrupt */
#define ISR_PWM		(1 << PWM_IRQ_NUM)	/* Pulse-Width Modulator int. */
#define	ISR_INT0	(1 << INT0_IRQ_NUM)	/* External INT0 */
#define	ISR_INT1	(1 << INT1_IRQ_NUM)	/* External INT1 */
#define	ISR_INT2	(1 << INT2_IRQ_NUM)	/* External INT2 */
#define	ISR_INT3	(1 << INT3_IRQ_NUM)	/* External INT3 */
#define	ISR_INT4	(1 << INT4_IRQ_NUM)	/* External INT4 */
#define	ISR_INT5	(1 << INT5_IRQ_NUM)	/* External INT5 */
#define	ISR_INT6	(1 << INT6_IRQ_NUM)	/* External INT6 */
#define	ISR_INT7	(1 << INT7_IRQ_NUM)	/* External INT7 */
#define ISR_IRQ1	(1 << IRQ1_IRQ_NUM)	/* IRQ1 */
#define ISR_IRQ2	(1 << IRQ2_IRQ_NUM)	/* IRQ2 */
#define ISR_IRQ3	(1 << IRQ3_IRQ_NUM)	/* IRQ3 */
#define ISR_IRQ6	(1 << IRQ6_IRQ_NUM)	/* IRQ6 */
#define ISR_PEN		(1 << PEN_IRQ_NUM)	/* Pen Interrupt */
#define ISR_SPIS	(1 << SPIS_IRQ_NUM)	/* SPI Slave Interrupt */
#define ISR_TMR1	(1 << TMR1_IRQ_NUM)	/* Timer 1 interrupt */
#define ISR_IRQ7	(1 << IRQ7_IRQ_NUM)	/* IRQ7 */

/* 'EZ328-compatible definitions */
#define ISR_SPI	ISR_SPIM
#define ISR_TMR	ISR_TMR1

/* 
 * Interrupt Pending Register 
 */
#define IPR_ADDR	0xfffff310
#define IPR		LONG_REF(IPR_ADDR)

#define IPR_SPIM 	(1 << SPIM_IRQ_NUM)	/* SPI Master interrupt */
#define	IPR_TMR2	(1 << TMR2_IRQ_NUM)	/* Timer 2 interrupt */
#define IPR_UART	(1 << UART_IRQ_NUM)	/* UART interrupt */	
#define	IPR_WDT		(1 << WDT_IRQ_NUM)	/* Watchdog Timer interrupt */
#define IPR_RTC		(1 << RTC_IRQ_NUM)	/* RTC interrupt */
#define	IPR_KB		(1 << KB_IRQ_NUM)	/* Keyboard Interrupt */
#define IPR_PWM		(1 << PWM_IRQ_NUM)	/* Pulse-Width Modulator int. */
#define	IPR_INT0	(1 << INT0_IRQ_NUM)	/* External INT0 */
#define	IPR_INT1	(1 << INT1_IRQ_NUM)	/* External INT1 */
#define	IPR_INT2	(1 << INT2_IRQ_NUM)	/* External INT2 */
#define	IPR_INT3	(1 << INT3_IRQ_NUM)	/* External INT3 */
#define	IPR_INT4	(1 << INT4_IRQ_NUM)	/* External INT4 */
#define	IPR_INT5	(1 << INT5_IRQ_NUM)	/* External INT5 */
#define	IPR_INT6	(1 << INT6_IRQ_NUM)	/* External INT6 */
#define	IPR_INT7	(1 << INT7_IRQ_NUM)	/* External INT7 */
#define IPR_IRQ1	(1 << IRQ1_IRQ_NUM)	/* IRQ1 */
#define IPR_IRQ2	(1 << IRQ2_IRQ_NUM)	/* IRQ2 */
#define IPR_IRQ3	(1 << IRQ3_IRQ_NUM)	/* IRQ3 */
#define IPR_IRQ6	(1 << IRQ6_IRQ_NUM)	/* IRQ6 */
#define IPR_PEN		(1 << PEN_IRQ_NUM)	/* Pen Interrupt */
#define IPR_SPIS	(1 << SPIS_IRQ_NUM)	/* SPI Slave Interrupt */
#define IPR_TMR1	(1 << TMR1_IRQ_NUM)	/* Timer 1 interrupt */
#define IPR_IRQ7	(1 << IRQ7_IRQ_NUM)	/* IRQ7 */

/* 'EZ328-compatible definitions */
#define IPR_SPI	IPR_SPIM
#define IPR_TMR	IPR_TMR1

/**********
 *
 * 0xFFFFF4xx -- Parallel Ports
 *
 **********/

/*
 * Port A
 */
#define PADIR_ADDR	0xfffff400		/* Port A direction reg */
#define PADATA_ADDR	0xfffff401		/* Port A data register */
#define PASEL_ADDR	0xfffff403		/* Port A Select register */

#define PADIR		BYTE_REF(PADIR_ADDR)
#define PADATA		BYTE_REF(PADATA_ADDR)
#define PASEL		BYTE_REF(PASEL_ADDR)

#define PA(x)           (1 << (x))
#define PA_A(x)		PA((x) - 16)	/* This is specific to PA only! */

#define PA_A16		PA(0)		/* Use A16 as PA(0) */
#define PA_A17		PA(1)		/* Use A17 as PA(1) */
#define PA_A18		PA(2)		/* Use A18 as PA(2) */
#define PA_A19		PA(3)		/* Use A19 as PA(3) */
#define PA_A20		PA(4)		/* Use A20 as PA(4) */
#define PA_A21		PA(5)		/* Use A21 as PA(5) */
#define PA_A22		PA(6)		/* Use A22 as PA(6) */
#define PA_A23		PA(7)		/* Use A23 as PA(7) */

/* 
 * Port B
 */
#define PBDIR_ADDR	0xfffff408		/* Port B direction reg */
#define PBDATA_ADDR	0xfffff409		/* Port B data register */
#define PBSEL_ADDR	0xfffff40b		/* Port B Select Register */

#define PBDIR		BYTE_REF(PBDIR_ADDR)
#define PBDATA		BYTE_REF(PBDATA_ADDR)
#define PBSEL		BYTE_REF(PBSEL_ADDR)

#define PB(x)           (1 << (x))
#define PB_D(x)		PB(x)		/* This is specific to port B only */

#define PB_D0		PB(0)		/* Use D0 as PB(0) */
#define PB_D1		PB(1)		/* Use D1 as PB(1) */
#define PB_D2		PB(2)		/* Use D2 as PB(2) */
#define PB_D3		PB(3)		/* Use D3 as PB(3) */
#define PB_D4		PB(4)		/* Use D4 as PB(4) */
#define PB_D5		PB(5)		/* Use D5 as PB(5) */
#define PB_D6		PB(6)		/* Use D6 as PB(6) */
#define PB_D7		PB(7)		/* Use D7 as PB(7) */

/* 
 * Port C
 */
#define PCDIR_ADDR	0xfffff410		/* Port C direction reg */
#define PCDATA_ADDR	0xfffff411		/* Port C data register */
#define PCSEL_ADDR	0xfffff413		/* Port C Select Register */

#define PCDIR		BYTE_REF(PCDIR_ADDR)
#define PCDATA		BYTE_REF(PCDATA_ADDR)
#define PCSEL		BYTE_REF(PCSEL_ADDR)

#define PC(x)           (1 << (x))

#define PC_WE		PC(6)		/* Use WE    as PC(6) */
#define PC_DTACK	PC(5)		/* Use DTACK as PC(5) */
#define PC_IRQ7		PC(4)		/* Use IRQ7  as PC(4) */
#define PC_LDS		PC(2)		/* Use LDS   as PC(2) */
#define PC_UDS		PC(1)		/* Use UDS   as PC(1) */
#define PC_MOCLK	PC(0)		/* Use MOCLK as PC(0) */

/* 
 * Port D
 */
#define PDDIR_ADDR	0xfffff418		/* Port D direction reg */
#define PDDATA_ADDR	0xfffff419		/* Port D data register */
#define PDPUEN_ADDR	0xfffff41a		/* Port D Pull-Up enable reg */
#define PDPOL_ADDR	0xfffff41c		/* Port D Polarity Register */
#define PDIRQEN_ADDR	0xfffff41d		/* Port D IRQ enable register */
#define	PDIQEG_ADDR	0xfffff41f		/* Port D IRQ Edge Register */

#define PDDIR		BYTE_REF(PDDIR_ADDR)
#define PDDATA		BYTE_REF(PDDATA_ADDR)
#define PDPUEN		BYTE_REF(PDPUEN_ADDR)
#define	PDPOL		BYTE_REF(PDPOL_ADDR)
#define PDIRQEN		BYTE_REF(PDIRQEN_ADDR)
#define PDIQEG		BYTE_REF(PDIQEG_ADDR)

#define PD(x)           (1 << (x))
#define PD_KB(x)	PD(x)		/* This is specific for Port D only */

#define PD_KB0		PD(0)	/* Use KB0 as PD(0) */
#define PD_KB1		PD(1)	/* Use KB1 as PD(1) */
#define PD_KB2		PD(2)	/* Use KB2 as PD(2) */
#define PD_KB3		PD(3)	/* Use KB3 as PD(3) */
#define PD_KB4		PD(4)	/* Use KB4 as PD(4) */
#define PD_KB5		PD(5)	/* Use KB5 as PD(5) */
#define PD_KB6		PD(6)	/* Use KB6 as PD(6) */
#define PD_KB7		PD(7)	/* Use KB7 as PD(7) */

/* 
 * Port E
 */
#define PEDIR_ADDR	0xfffff420		/* Port E direction reg */
#define PEDATA_ADDR	0xfffff421		/* Port E data register */
#define PEPUEN_ADDR	0xfffff42