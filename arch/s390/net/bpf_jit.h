rt K
 */
#define PKDIR_ADDR	0xfffff440		/* Port K direction reg */
#define PKDATA_ADDR	0xfffff441		/* Port K data register */
#define PKPUEN_ADDR	0xfffff442		/* Port K Pull-Up enable reg */
#define PKSEL_ADDR	0xfffff443		/* Port K Select Register */

#define PKDIR		BYTE_REF(PKDIR_ADDR)
#define PKDATA		BYTE_REF(PKDATA_ADDR)
#define PKPUEN		BYTE_REF(PKPUEN_ADDR)
#define PKSEL		BYTE_REF(PKSEL_ADDR)

#define PK(x)           (1 << (x))

/* 
 * Port M
 */
#define PMDIR_ADDR	0xfffff438		/* Port M direction reg */
#define PMDATA_ADDR	0xfffff439		/* Port M data register */
#define PMPUEN_ADDR	0xfffff43a		/* Port M Pull-Up enable reg */
#define PMSEL_ADDR	0xfffff43b		/* Port M Select Register */

#define PMDIR		BYTE_REF(PMDIR_ADDR)
#define PMDATA		BYTE_REF(PMDATA_ADDR)
#define PMPUEN		BYTE_REF(PMPUEN_ADDR)
#define PMSEL		BYTE_REF(PMSEL_ADDR)

#define PM(x)           (1 << (x))

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

#define PWMC_CLKSEL_MASK	0x0007	/* Clock Selection */
#define PWMC_CLKSEL_SHIFT	0
#define PWMC_PWMEN		0x0010	/* Enable PWM */
#define PMNC_POL		0x0020	/* PWM Output Bit Polarity */
#define PWMC_PIN		0x0080	/* Current PWM output pin status */
#define PWMC_LOAD		0x0100	/* Force a new period */
#define PWMC_IRQEN		0x4000	/* Interrupt Request Enable */
#define PWMC_CLKSRC		0x8000	/* Clock Source Select */

/* 'EZ328-compatible definitions */
#define PWMC_EN	PWMC_PWMEN

/*
 * PWM Period Register
 */
#define PWMP_ADDR	0xfffff502
#define PWMP		WORD_REF(PWMP_ADDR)

/* 
 * PWM Width Register 
 */
#define PWMW_ADDR	0xfffff504
#define PWMW		WORD_REF(PWMW_ADDR)

/*
 * PWM Counter Register
 */
#define PWMCNT_ADDR	0xfffff506
#define PWMCNT		WORD_REF(PWMCNT_ADDR)

/**********
 *
 * 0xFFFFF6xx -- General-Purpose Timers
 *
 **********/

/* 
 * Timer Unit 1 and 2 Control Registers
 */
#define TCTL1_ADDR	0xfffff600
#define TCTL1		WORD_REF(TCTL1_ADDR)
#define TCTL2_ADDR	0xfffff60c
#define TCTL2		WORD_REF(TCTL2_ADDR)

#define	TCTL_TEN		0x0001	/* T