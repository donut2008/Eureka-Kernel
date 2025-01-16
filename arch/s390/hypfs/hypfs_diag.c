upt */
#define IMR_MRTC	(1 << RTC_IRQ_NUM)	/* Mask RTC interrupt */
#define	IMR_MKB		(1 << KB_IRQ_NUM)	/* Mask Keyboard Interrupt */
#define IMR_MPWM	(1 << PWM_IRQ_NUM)	/* Mask Pulse-Width Modulator int. */
#define	IMR_MINT0	(1 << INT0_IRQ_NUM)	/* Mask External INT0 */
#define	IMR_MINT1	(1 << INT1_IRQ_NUM)	/* Mask External INT1 */
#define	IMR_MINT2	(1 << INT2_IRQ_NUM)	/* Mask External INT2 */
#define	IMR_MINT3	(1 << INT3_IRQ_NUM)	/* Mask External INT3 */
#define IMR_MIRQ1	(1 << IRQ1_IRQ_NUM)	/* Mask IRQ1 */
#define IMR_MIRQ2	(1 << IRQ2_IRQ_NUM)	/* Mask IRQ2 */
#define IMR_MIRQ3	(1 << IRQ3_IRQ_NUM)	/* Mask IRQ3 */
#define IMR_MIRQ6	(1 << IRQ6_IRQ_NUM)	/* Mask IRQ6 */
#define IMR_MIRQ5	(1 << IRQ5_IRQ_NUM)	/* Mask IRQ5 */
#define IMR_MSAM	(1 << SAM_IRQ_NUM)	/* Mask Sampling Timer for RTC */
#define IMR_MEMIQ	(1 << EMIQ_IRQ_NUM)	/* Mask Emulator Interrupt */

/* '328-compatible definitions */
#define IMR_MSPIM	IMR_MSPI
#define IMR_MTMR1	IMR_MTMR

/* 
 * Interrupt Status Register 
 */
#define ISR_ADDR	0xfffff30c
#define ISR		LONG_REF(ISR_ADDR)

#define ISR_SPI 	(1 << SPI_IRQ_NUM)	/* SPI interrupt */
#define	ISR_TMR		(1 << TMR_IRQ_NUM)	/* Timer interrupt */
#define ISR_UART	(1 << UART_IRQ_NUM)	/* UART interrupt */	
#define	ISR_WDT		(1 << WDT_IRQ_NUM)	/* Watchdog Timer interrupt */
#define ISR_RTC		(1 << RTC_IRQ_NUM)	/* RTC interrupt */
#define	ISR_KB		(1 << KB_IRQ_NUM)	/* Keyboard Interrupt */
#define ISR_PWM		(1 << PWM_IRQ_NUM)	/* Pulse-Width Modulator interrupt */
#define	ISR_INT0	(1 << INT0_IRQ_NUM)	/* External INT0 */
#define	ISR_INT1	(1 << INT1_IRQ_NUM)	/* External INT1 */
#define	ISR_INT2	(1 << INT2_IRQ_NUM)	/* External INT2 */
#define	ISR_INT3	(1 << INT3_IRQ_NUM)	/* External INT3 */
#define ISR_IRQ1	(1 << IRQ1_IRQ_NUM)	/* IRQ1 */
#define ISR_IRQ2	(1 << IRQ2_IRQ_NUM)	/* IRQ2 */
#define ISR_IRQ3	(1 << IRQ3_IRQ_NUM)	/* IRQ3 */
#define ISR_IRQ6	(1 << IRQ6_IRQ_NUM)	/* IRQ6 */
#define ISR_IRQ5	(1 << IRQ5_IRQ_NUM)	/* IRQ5 */
#define ISR_SAM		(1 << SAM_IRQ_NUM)	/* Sampling Timer for RTC */
#define ISR_EMIQ	(1 << EMIQ_IRQ_NUM)	/* Emulator Interrupt */

/* '328-compatible definitions */
#define ISR_SPIM	ISR_SPI
#define ISR_TMR1	ISR_TMR

/* 
 * Interrupt Pending Register 
 */
#define IPR_ADDR	0xfffff30c
#define IPR		LONG_REF(IPR_ADDR)

#define IPR_SPI 	(1 << SPI_IRQ_NUM)	/* SPI interrupt */
#define	IPR_TMR		(1 << TMR_IRQ_NUM)	/* Timer interrupt */
#define IPR_UART	(1 << UART_IRQ_NUM)	/* UART interrupt */	
#define	IPR_WDT		(1 << WDT_IRQ_NUM)	/* Watchdog Timer interrupt */
#define IPR_RTC		(1 << RTC_IRQ_NUM)	/* RTC interrupt */
#define	IPR_KB		(1 << KB_IRQ_NUM)	/* Keyboard Interrupt */
#define IPR_PWM		(1 << PWM_IRQ_NUM)	/* Pulse-Width Modulator interrupt */
#define	IPR_INT0	(1 << INT0_IRQ_NUM)	/* External INT0 */
#define	IPR_INT1	(1 << INT1_IRQ_NUM)	/* External INT1 */
#define	IPR_INT2	(1 << INT2_IRQ_NUM)	/* External INT2 */
#define	IPR_INT3	(1 << INT3_IRQ_NUM)	/* External INT3 */
#define IPR_IRQ1	(1 << IRQ1_IRQ_NUM)	/* IRQ1 */
#define IPR_IRQ2	(1 << IRQ2_IRQ_NUM)	/* IRQ2 */
#define IPR_IRQ3	(1 << IRQ3_IRQ_NUM)	/* IRQ3 */
#define IPR_IRQ6	(1 << IRQ6_IRQ_NUM)	/* IRQ6 */
#define IPR_IRQ5	(1 << IRQ5_IRQ_NUM)	/* IRQ5 */
#define IPR_SAM		(1 << SAM_IRQ_NUM)	/* Sampling Timer for RTC */
#define IPR_EMIQ	(1 << EMIQ_IRQ_NUM)	/* Emulator Interrupt */

/* '328-compatible definitions */
#define IPR_SPIM	IPR_SPI
#define IPR_TMR1	IPR_TMR

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
#define PAPUEN_ADDR	0xfffff402		/* Port A Pull-Up enable reg */

#define PADIR		BYTE_REF(PADIR_ADDR)
#define PADATA		BYTE_REF(PADATA_ADDR)
#define PAPUEN		BYTE_REF(PAPUEN_ADDR)

#define PA(x)		(1 << (x))

/* 
 * Port B
 */
#define PBDIR_ADDR	0xfffff408		/* Port B direction reg */
#define PBDATA_ADDR	0xfffff409		/* Port B data register */
#define PBPUEN_ADDR	0xfffff40a		/* Port B Pull-Up enable reg */
#define PBSEL_ADDR	0xfffff40b		/* Port B Select Register */

#define PBDIR		BYTE_REF(PBDIR_ADDR)
#define PBDATA		BYTE_REF(PBDATA_ADDR)
#define PBPUEN		BYTE_REF(PBPUEN_ADDR)
#define PBSEL		BYTE_REF(PBSEL_ADDR)

#define PB(x)		(1 << (x))

#define PB_CSB0		0x01	/* Use CSB0      as PB[0] */
#define PB_CSB1		0x02	/* Use CSB1      as PB[1] */
#define PB_CSC0_RAS0	0x04    /* Use CSC0/RAS0 as PB[2] */	
#define PB_CSC1_RAS1	0x08    /* Use CSC1/RAS1 as PB[3] */	
#define PB_CSD0_CAS0	0x10    /* Use CSD0/CAS0 as PB[4] */	
#define PB_CSD1_CAS1	0x20    /* Use CSD1/CAS1 as PB[5] */
#define PB_TIN_TOUT	0x40	/* Use TIN/TOUT  as PB[6] */
#define PB_PWMO		0x80	/* Use PWMO      as PB[7] */

/* 
 * Port C
 */
#define PCDIR_ADDR	0xfffff410		/* Port C direction reg */
#define PCDATA_ADDR	0xfffff411		/* Port C data register */
#define PCPDEN_ADDR	0xfffff412		/* Port C Pull-Down enb. reg */
#define PCSEL_ADDR	0xfffff413		/* Port C Select Register */

#define PCDIR		BYTE_REF(PCDIR_ADDR)
#define PCDATA		BYTE_REF(PCDATA_ADDR)
#define PCPDEN		BYTE_REF(PCPDEN_ADDR)
#define PCSEL		BYTE_REF(PCSEL_ADDR)

#define PC(x)		(1 << (x))

#define PC_LD0		0x01	/* Use LD0  as PC[0] */
#define PC_LD1		0x02	/* Use LD1  as PC[1] */
#define PC_LD2		0x04	/* Use LD2  as PC[2] */
#define PC_LD3		0x08	/* Use LD3  as PC[3] */
#define PC_LFLM		0x10	/* Use LFLM as PC[4] */
#define PC_LLP 		0x20	/* Use LLP  as PC[5] */
#define PC_LCLK		0x40	/* Use LCLK as PC[6] */
#define PC_LACD		0x80	/* Use LACD as PC[7] */

/* 
 * Port D
 */
#define PDDIR_ADDR	0xfffff418		/* Port D direction reg */
#define PDDATA_ADDR	0xfffff419		/* Port D data register */
#define PDPUEN_ADDR	0xfffff41a		/* Port D Pull-Up enable reg */
#define PDSEL_ADDR	0xfffff41b		/* Port D Select Register */
#define PDPOL_ADDR	0xfffff41c		/* Port D Polarity Register */
#define PDIRQEN_ADDR	0xfffff41d		/* Port D IRQ enable register */
#define PDKBEN_ADDR	0xfffff41e		/* Port D Keyboard Enable reg */
#define	PDIQEG_ADDR	0xfffff41f		/* Port D IRQ Edge Register */

#define PDDIR		BYTE_REF(PDDIR_ADDR)
#define PDDATA		BYTE_REF(PDDATA_ADDR)
#define PDPUEN		BYTE_REF(PDPUEN_ADDR)
#define PDSEL		BYTE_REF(PDSEL_ADDR)
#define	PDPOL		BYTE_REF(PDPOL_ADDR)
#define PDIRQEN		BYTE_REF(PDIRQEN_ADDR)
#define PDKBEN		BYTE_REF(PDKBEN_ADDR)
#define PDIQEG		BYTE_REF(PDIQEG_ADDR)

#define PD(x)		(1 << (x))

#define PD_INT0		0x01	/* Use INT0 as PD[0] */
#define PD_INT1		0x02	/* Use INT1 as PD[1] */
#define PD_INT2		0x04	/* Use INT2 as PD[2] */
#define PD_INT3		0x08	/* Use INT3 as PD[3] */
#define PD_IRQ1		0x10	/* Use IRQ1 as PD[4] */
#define PD_IRQ2		0x20	/* Use IRQ2 as PD[5] */
#define PD_IRQ3		0x40	/* Use IRQ3 as PD[6] */
#define PD_IRQ6		0x80	/* Use IRQ6 as PD[7] */

/* 
 * Port E
 */
#define PEDIR_ADDR	0xfffff420		/* Port E direction reg */
#define PEDATA_ADDR	0xfffff421		/* Port E data register */
#define PEPUEN_ADDR	0xfffff422		/* Port E Pull-Up enable reg */
#define PESEL_ADDR	0xfffff423		/* Port E Select Register */

#define PEDIR		BYTE_REF(PEDIR_ADDR)
#define PEDATA		BYTE_REF(PEDATA_ADDR)
#define PEPUEN		BYTE_REF(PEPUEN_ADDR)
#define PESEL		BYTE_REF(PESEL_ADDR)

#define PE(x)		(1 << (x))

#define PE_SPMTXD	0x01	/* Use SPMTXD as PE[0] */
#define PE_SPMRXD	0x02	/* Use SPMRXD as PE[1] */
#define PE_SPMCLK	0x04	/* Use SPMCLK as PE[2] */
#define PE_DWE		0x08	/* Use DWE    as PE[3] */
#define PE_RXD		0x10	/* Use RXD    as PE[4] */
#define PE_TXD		0x20	/* Use TXD    as PE[5] */
#define PE_RTS		0x40	/* Use RTS    as PE[6] */
#define PE_CTS		0x80	/* Use CTS    as PE[7] */

/* 
 * Port F
 */
#define PFDIR_ADDR	0xfffff428		/* Port F direction reg */
#define PFDATA_ADDR	0xfffff429		/* Port F data register */
#define PFPUEN_ADDR	0xfffff42a		/* Port F Pull-Up enable reg */
#define PFSEL_ADDR	0xfffff42b		/* Port F Select Register */

#define PFDIR		BYTE_REF(PFDIR_ADDR)
#define PFDATA		BYTE_REF(PFDATA_ADDR)
#define PFPUEN		BYTE_REF(PFPUEN_ADDR)
#define PFSEL		BYTE_REF(PFSEL_ADDR)

#define PF(x)		(1 << (x))

#define PF_LCONTRAST	0x01	/* Use LCONTRAST as PF[0] */
#define PF_IRQ5         0x02    /* Use IRQ5      as PF[1] */
#define PF_CLKO         0x04    /* Use CLKO      as PF[2] */
#define PF_A20          0x08    /* Use A20       as PF[3] */
#define PF_A21          0x10    /* Use A21       as PF[4] */
#define PF_A22          0x20    /* Use A22       as PF[5] */
#define PF_A23          0x40    /* Use A23       as PF[6] */
#define PF_CSA1		0x80    /* Use CSA1      as PF[7] */

/* 
 * Port G
 */
#define PGDIR_ADDR	0xfffff430		/* Port G direction reg */
#define PGDATA_ADDR	0xfffff431		/* Port G data register */
#define PGPUEN_ADDR	0xfffff432		/* Port G Pull-Up enable reg */
#define PGSEL_ADDR	0xfffff433		/* Port G Select Register */

#define PGDIR		BYTE_REF(PGDIR_ADDR)
#define PGDATA		BYTE_REF(PGDATA_ADDR)
#define PGPUEN		BYTE_REF(PGPUEN_ADDR)
#define PGSEL		BYTE_REF(PGSEL_ADDR)

#define PG(x)		(1 << (x))

#define PG_BUSW_DTACK	0x01	/* Use BUSW/DTACK as PG[0] */
#define PG_A0		0x02	/* Use A0         as PG[1] */
#define PG_EMUIRQ	0x04	/* Use EMUIRQ     as PG[2] */
#define PG_HIZ_P_D	0x08	/* Use HIZ/P/D    as PG[3] */
#define PG_EMUCS        0x10	/* Use EMUCS      as PG[4] */
#define PG_EMUBRK	0x20	/* Use EMUBRK     as PG[5] */

/* 
 * Port J
 */
#define PJDIR_ADDR	0xfffff438		/* Port J direction reg */
#define PJDATA_ADDR	0xfffff439		/* Port J data register */
#define PJPUEN_ADDR	0xfffff43A		/* Port J Pull-Up enb. reg */
#define PJSEL_ADDR	0xfffff43B		/* Port J Select Register */

#define PJDIR		BYTE_REF(PJDIR_ADDR)
#define PJDATA		BYTE_REF(PJDATA_ADDR)
#define PJPUEN		BYTE_REF(PJPUEN_ADDR)
#define PJSEL		BYTE_REF(PJSEL_ADDR)

#define PJ(x)		(1 << (x))

/*
 * Port K
 */
#define PKDIR_ADDR	0xfffff440		/* Port K direction reg */
#define PKDATA_ADDR	0xfffff441		/* Port K data register */
#define PKPUEN_ADDR	0xfffff442		/* Port K Pull-Up enb. reg */
#define PKSEL_ADDR	0xfffff443		/* Port K Select Register */

#define PKDIR		BYTE_REF(PKDIR_ADDR)
#define PKDATA		BYTE_REF(PKDATA_ADDR)
#define PKPUEN		BYTE_REF(PKPUEN_ADDR)
#define PKSEL		BYTE_REF(PKSEL_ADDR)

#define PK(x)		(1 << (x))

#define PK_DATAREADY		0x01	/* Use ~DATA_READY  as PK[0] */
#define PK_PWM2		0x01	/* Use PWM2  as PK[0] */
#define PK_R_W		0x02	/* Use R/W  as PK[1] */
#define PK_LDS		0x04	/* Use /LDS  as PK[2] */
#define PK_UDS		0x08	/* Use /UDS  as PK[3] */
#define PK_LD4		0x10	/* Use LD4 as PK[4] */
#define PK_LD5 		0x20	/* Use LD5  as PK[5] */
#define PK_LD6		0x40	/* Use LD6 as PK[6] */
#define PK_LD7		0x80	/* Use LD7 as PK[7] */

#define PJDIR_ADDR	0xfffff438		/* Port J direction reg */
#define PJDATA_ADDR	0xfffff439		/* Port J data register */
#define PJPUEN_ADDR	0xfffff43A		/* Port J Pull-Up enable reg */
#define PJSEL_ADDR	0xfffff43B		/* Port J Select Register */

#define PJDIR		BYTE_REF(PJDIR_ADDR)
#define PJDATA		BYTE_REF(PJDATA_ADDR)
#define PJPUEN		BYTE_REF(PJPUEN_ADDR)
#define PJSEL		BYTE_REF(PJSEL_ADDR)

#define PJ(x)		(1 << (x))

#define PJ_MOSI 	0x01	/* Use MOSI       as PJ[0] */
#define PJ_MISO		0x02	/* Use MISO       as PJ[1] */
#define PJ_SPICLK1  	0x04	/* Use SPICLK1    as PJ[2] */
#define PJ_SS   	0x08	/* Use SS         as PJ[3] */
#define PJ_RXD2         0x10	/* Use RXD2       as PJ[4] */
#define PJ_TXD2  	0x20	/* Use TXD2       as PJ[5] */
#define PJ_RTS2  	0x40	/* Use RTS2       as PJ[5] */
#define PJ_CTS2  	0x80	/* Use CTS2       as PJ[5] */

/*
 * Port M
 */
#define PMDIR_ADDR	0xfffff448		/* Port M direction reg */
#define PMDATA_ADDR	0xfffff449		/* Port M data register */
#define PMPUEN_ADDR	0xfffff44a		/* Port M Pull-Up enable reg */
#define PMSEL_ADDR	0xfffff44b		/* Port M Select Register */

#define PMDIR		BYTE_REF(PMDIR_ADDR)
#define PMDATA		BYTE_REF(PMDATA_ADDR)
#define PMPUEN		BYTE_REF(PMPUEN_ADDR)
#define PMSEL		BYTE_REF(PMSEL_ADDR)

#define PM(x)		(1 << (x))

#define PM_SDCLK	0x01	/* Use SDCLK      as PM[0] */
#define PM_SDCE		0x02	/* Use SDCE       as PM[1] */
#define PM_DQMH 	0x04	/* Use DQMH       as PM[2] */
#define PM_DQML 	0x08	/* Use DQML       as PM[3] */
#define PM_SDA10        0x10	/* Use SDA10      as PM[4] */
#define PM_DMOE 	0x20	/* Use DMOE       as PM[5] */

/**********
 *
 * 0xFFFFF5xx -- Pulse-Width Modulator (PWM)
 *
 **********/

/*
 * PWM Control Register
 */
#define PWMC_ADDR	0xfffff500
#define PWMC		WORD_REF(PWMC_ADDR)

#define PWMC_CLKSEL_MASK	0x0003	/* Clock Selection */
#define PWMC_CLKSEL_SHIFT	0
#define PWMC_REPEAT_MASK	0x000c	/* Sample Repeats */
#define PWMC_REPEAT_SHIFT	2
#define PWMC_EN			0x0010	/* Enable PWM */
#define PMNC_FIFOAV		0x0020	/* FIFO Available */
#define PWMC_IRQEN		0x0040	/* Interrupt Request Enable */
#define PWMC_IRQ		0x0080	/* Interrupt Request (FIFO empty) */
#define PWMC_PRESCALER_MASK	0x7f00	/* Incoming Clock prescaler */
#define PWMC_PRESCALER_SHIFT	8
#define PWMC_CLKSRC		0x8000	/* Clock Source Select */

/* '328-compatible definitions */
#define PWMC_PWMEN	PWMC_EN

/*
 * PWM Sample Register 
 */
#define PWMS_ADDR	0xfffff502
#define PWMS		WORD_REF(PWMS_ADDR)

/*
 * PWM Period Register
 */
#define PWMP_ADDR	0xfffff504
#define PWMP		BYTE_REF(PWMP_ADDR)

/*
 * PWM Counter Register
 */
#define PWMCNT_ADDR	0xfffff505
#define PWMCNT		BYTE_REF(PWMCNT_ADDR)

/**********
 *
 * 0xFFFFF6xx -- General-Purpose Timer
 *
 **********/

/* 
 * Timer Control register
 */
#define TCTL_ADDR	0xfffff600
#define TCTL		WORD_REF(TCTL_ADDR)

#define	TCTL_TEN		0x0001	/* Timer Enable  */
#define TCTL_CLKSOURCE_MASK 	0x000e	/* Clock Source: */
#define   TCTL_CLKSOURCE_STOP	   0x0000	/* Stop count (disabled)    */
#define   TCTL_CLKSOURCE_SYSCLK	   0x0002	/* SYSCLK to prescaler      */
#define   TCTL_CLKSOURCE_SYSCLK_16 0x0004	/* SYSCLK/16 to prescaler   */
#define   TCTL_CLKSOURCE_TIN	   0x0006	/* TIN to prescaler         */
#define   TCTL_CLKSOURCE_32KHZ	   0x0008	/* 32kHz clock to prescaler */
#define TCTL_IRQEN		0x0010	/* IRQ Enable    */
#define TCTL_OM			0x0020	/* Output Mode   */
#define TCTL_CAP_MASK		0x00c0	/* Capture Edge: */
#define	  TCTL_CAP_RE		0x0040		/* Capture on rizing edge   */
#define   TCTL_CAP_FE		0x0080		/* Capture on falling edge  */
#define TCTL_FRR		0x0010	/* Free-Run Mode */

/* '328-compatible definitions */
#define TCTL1_ADDR	TCTL_ADDR
#define TCTL1		TCTL

/*
 * Timer Prescaler Register
 */
#define TPRER_ADDR	0xfffff602
#define TPRER		WORD_REF(TPRER_ADDR)

/* '328-compatible definitions */
#define TPRER1_ADDR	TPRER_ADDR
#define TPRER1		TPRER

/*
 * Timer Compare Register
 */
#define TCMP_ADDR	0xfffff604
#define TCMP		WORD_REF(TCMP_ADDR)

/* '328-compatible definitions */
#define TCMP1_ADDR	TCMP_ADDR
#define TCMP1		TCMP

/*
 * Timer Capture register
 */
#define TCR_ADDR	0xfffff606
#define TCR		WORD_REF(TCR_ADDR)

/* '328-compatible definitions */
#define TCR1_ADDR	TCR_ADDR
#define TCR1		TCR

/*
 * Timer Counter Register
 */
#define TCN_ADDR	0xfffff608
#define TCN		WORD_REF(TCN_ADDR)

/* '328-compatible definitions */
#define TCN1_ADDR	TCN_ADDR
#define TCN1		TCN

/*
 * Timer Status Register
 */
#define TSTAT_ADDR	0xfffff60a
#define TSTAT		WORD_REF(TSTAT_ADDR)

#define TSTAT_COMP	0x0001		/* Compare Event occurred */
#define TSTAT_CAPT	0x0001		/* Capture Event occurred */

/* '328-compatible definitions */
#define TSTAT1_ADDR	TSTAT_ADDR
#define TSTAT1		TSTAT

/**********
 *
 * 0xFFFFF8xx -- Serial Periferial Interface Master (SPIM)
 *
 **********/

/*
 * SPIM Data Register
 */
#define SPIMDATA_ADDR	0xfffff800
#define SPIMDATA	WORD_REF(SPIMDATA_ADDR)

/*
 * SPIM Control/Status Register
 */
#define SPIMCONT_ADDR	0xfffff802
#define SPIMCONT	WORD_REF(SPIMCONT_ADDR)

#define SPIMCONT_BIT_COUNT_MASK	 0x000f	/* Transfer Length in Bytes */
#define SPIMCONT_BIT_COUNT_SHIFT 0
#define SPIMCONT_POL		 0x0010	/* SPMCLK Signel Polarity */
#define	SPIMCONT_PHA		 0x0020	/* Clock/Data phase relationship */
#define SPIMCONT_IRQEN		 0x0040 /* IRQ Enable */
#define SPIMCONT_IRQ		 0x0080	/* Interrupt Request */
#define SPIMCONT_XCH		 0x0100	/* Exchange */
#define SPIMCONT_ENABLE		 0x0200	/* Enable SPIM */
#define SPIMCONT_DATA_RATE_MASK	 0xe000	/* SPIM Data Rate */
#define SPIMCONT_DATA_RATE_SHIFT 13

/* '328-compatible definitions */
#define SPIMCONT_SPIMIRQ	SPIMCONT_IRQ
#define SPIMCONT_SPIMEN		SPIMCONT_ENABLE

/**********
 *
 * 0xFFFFF9xx -- UART
 *
 **********/

/*
 * UART Status/Control Register
 */

#define USTCNT_ADDR	0xfffff900
#define USTCNT		WORD_REF(USTCNT_ADDR)

#define USTCNT_TXAE	0x0001	/* Transmitter Available Interrupt Enable */
#define USTCNT_TXHE	0x0002	/* Transmitter Half Empty Enable */
#define USTCNT_TXEE	0x0004	/* Transmitter Empty Interrupt Enable */
#define USTCNT_RXRE	0x0008	/* Receiver Ready Interrupt Enable */
#define USTCNT_RXHE	0x0010	/* Receiver Half-Full Interrupt Enable */
#define USTCNT_RXFE	0x0020	/* Receiver Full Interrupt Enable */
#define USTCNT_CTSD	0x0040	/* CTS Delta Interrupt Enable */
#define USTCNT_ODEN	0x0080	/* Old Data Interrupt Enable */
#define USTCNT_8_7	0x0100	/* Eight or seven-bit transmission */
#define USTCNT_STOP	0x0200	/* Stop bit transmission */
#define USTCNT_ODD	0x0400	/* Odd Parity */
#define	USTCNT_PEN	0x0800	/* Parity Enable */
#define USTCNT_CLKM	0x1000	/* Clock Mode Select */
#define	USTCNT_TXEN	0x2000	/* Transmitter Enable */
#define USTCNT_RXEN	0x4000	/* Receiver Enable */
#define USTCNT_UEN	0x8000	/* UART Enable */

/* '328-compatible definitions */
#define USTCNT_TXAVAILEN	USTCNT_TXAE
#define USTCNT_TXHALFEN		USTCNT_TXHE
#define USTCNT_TXEMPTYEN	USTCNT_TXEE
#define USTCNT_RXREADYEN	USTCNT_RXRE
#define USTCNT_RXHALFEN		USTCNT_RXHE
#define USTCNT_RXFULLEN		USTCNT_RXFE
#define USTCNT_CTSDELTAEN	USTCNT_CTSD
#define USTCNT_ODD_EVEN		USTCNT_ODD
#define USTCNT_PARITYEN		USTCNT_PEN
#define USTCNT_CLKMODE		USTCNT_CLKM
#define USTCNT_UARTEN		USTCNT_UEN

/*
 * UART Baud Control Register
 */
#define UBAUD_ADDR	0xfffff902
#define UBAUD		WORD_REF(UBAUD_ADDR)

#define UBAUD_PRESCALER_MASK	0x003f	/* Actual divisor is 65 - PRESCALER */
#define UBAUD_PRESCALER_SHIFT	0
#define UBAUD_DIVIDE_MASK	0x0700	/* Baud Rate freq. divizor */
#define UBAUD_DIVIDE_SHIFT	8
#define UBAUD_BAUD_SRC		0x0800	/* Baud Rate Source */
#define UBAUD_UCLKDIR		0x2000	/* UCLK Direction */

/*
 * UART Receiver Register 
 */
#define URX_ADDR	0xfffff904
#define URX		WORD_REF(URX_ADDR)

#define URX_RXDATA_ADDR	0xfffff905
#define URX_RXDATA	BYTE_REF(URX_RXDATA_ADDR)

#define URX_RXDATA_MASK	 0x00ff	/* Received data */
#define URX_RXDATA_SHIFT 0
#define URX_PARITY_ERROR 0x0100	/* Parity Error */
#define URX_BREAK	 0x0200	/* Break Detected */
#define URX_FRAME_ERROR	 0x0400	/* Framing Error */
#define URX_OVRUN	 0x0800	/* Serial Overrun */
#define URX_OLD_DATA	 0x1000	/* Old data in FIFO */
#define URX_DATA_READY	 0x2000	/* Data Ready (FIFO not empty) */
#define URX_FIFO_HALF	 0x4000 /* FIFO is Half-Full */
#define URX_FIFO_FULL	 0x8000	/* FIFO is Full */

/*
 * UART Transmitter Register 
 */
#define UTX_ADDR	0xfffff9