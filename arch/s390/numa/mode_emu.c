__IA64_UL_CONST(0x007fffffffffffff)

/* ==================================================================== */
/*                   Register "SH_PIO_WRITE_STATUS_0|1"                 */
/*                      PIO Write Status for CPU 0 & 1                  */
/* ==================================================================== */
#define SH1_PIO_WRITE_STATUS_0		__IA64_UL_CONST(0x0000000120070200)
#define SH1_PIO_WRITE_STATUS_1		__IA64_UL_CONST(0x0000000120070280)
#define SH2_PIO_WRITE_STATUS_0		__IA64_UL_CONST(0x0000000020070200)
#define SH2_PIO_WRITE_STATUS_1		__IA64_UL_CONST(0x0000000020070280)
#define SH2_PIO_WRITE_STATUS_2		__IA64_UL_CONST(0x0000000020070300)
#define SH2_PIO_WRITE_STATUS_3		__IA64_UL_CONST(0x0000000020070380)

/*   SH_PIO_WRITE_STATUS_0_WRITE_DEADLOCK                               */
/*   Description:  Deadlock response detected                           */
#define SH_PIO_WRITE_STATUS_WRITE_DEADLOCK_SHFT		1
#define SH_PIO_WRITE_STATUS_WRITE_DEADLOCK_MASK \
					__IA64_UL_CONST(0x0000000000000002)

/*   SH_PIO_WRITE_STATUS_0_PENDING_WRITE_COUNT                          */
/*   Description:  Count of currently pending PIO writes                */
#define SH_PIO_WRITE_STATUS_PENDING_WRITE_COUNT_SHFT	56
#define SH_PIO_WRITE_STATUS_PENDING_WRITE_COUNT_MASK \
					__IA64_UL_CONST(0x3f00000000000000)

/* ==================================================================== */
/*                Register "SH_PIO_WRITE_STATUS_0_ALIAS"                */
/* ==================================================================== */
#define SH1_PIO_WRITE_STATUS_0_ALIAS	__IA64_UL_CONST(0x0000000120070208)
#define SH2_PIO_WRITE_STATUS_0_ALIAS	__IA64_UL_CONST(0x0000000020070208)

/* ==================================================================== */
/*                     Register "SH_EVENT_OCCURRED"                     */
/*                    SHub Interrupt Event Occurred                     */
/* ==================================================================== */
/*   SH_EVENT_OCCURRED_UART_INT                                         */
/*   Description:  Pending Junk Bus UART Interrupt                      */
#define SH_EVENT_OCCURRED_UART_INT_SHFT			20
#define SH_EVENT_OCCURRED_UART_INT_MASK	__IA64_UL_CONST(0x0000000000100000)

/*   SH_EVENT_OCCURRED_IPI_INT                                          */
/*   Description:  Pending IPI Interrupt                                */
#define SH_EVENT_OCCURRED_IPI_INT_SHFT			28
#define SH_EVENT_OCCURRED_IPI_INT_MASK	__IA64_UL_CONST(0x0000000010000000)

/*   SH_EVENT_OCCURRED_II_INT0                                          */
/*   Description:  Pending II 0 Interrupt                               */
#define SH_EVENT_OCCURRED_II_INT0_SHFT			29
#define SH_EVENT_OCCURRED_II_INT0_MASK	__IA64_UL_CONST(0x0000000020000000)

/*   SH_EVENT_OCCURRED_II_INT1                                          */
/*   Description:  Pending II 1 Interrupt                               */
#define SH_EVENT_OCCURRED_II_INT1_SHFT			30
#define SH_EVENT_OCCURRED_II_INT1_MASK	__IA64_UL_CONST(0x0000000040000000)

/*   SH2_EVENT_OCCURRED_EXTIO_INT2                                      */
/*   Description:  Pending SHUB 2 EXT IO INT2                           */
#define SH2_EVENT_OCCURRED_EXTIO_INT2_SHFT		33
#define SH2_EVENT_OCCURRED_EXTIO_INT2_MASK __IA64_UL_CONST(0x0000000200000000)

/*   SH2_EVENT_OCCURRED_EXTIO_INT3                                      */
/*   Description:  Pending SHUB 2 EXT IO INT3                           */
#define SH2_EVENT_OCCURRED_EXTIO_INT3_SHFT		34
#define SH2_EVENT_OCCURRED_EXTIO_INT3_MASK __IA64_UL_CONST(0x0000000400000000)

#define SH_ALL_INT_MASK \
	(SH_EVENT_OCCURRED_UART_INT_MASK | SH_EVENT_OCCURRED_IPI_INT_MASK | \
	 SH_EVENT_OCCURRED_II_INT0_MASK | SH_EVENT_OCCURRED_II_INT1_MASK | \
	 SH_EVENT_OCCURRED_II_INT1_MASK | SH2_EVENT_OCCURRED_EXTIO_INT2_MASK | \
	 SH2_EVENT_OCCURRED_EXTIO_INT3_MASK)


/* ==================================================================== */
/*                         LEDS                                         */
/* ==================================================================== */
#define SH1_REAL_JUNK_BUS_LED0			0x7fed00000UL
#define SH1_REAL_JUNK_BUS_LED1			0x7fed10000UL
#define SH1_REAL_JUNK_BUS_LED2			0x7fed20000UL
#define SH1_REAL_JUNK_BUS_LED3			0x7fed30000UL

#define SH2_REAL_JUNK_BUS_LED0			0xf0000000UL
#define SH2_REAL_JUNK_BUS_LED1			0xf0010000UL
#define SH2_REAL_JUNK_BUS_LED2			0xf0020000UL
#define SH2_REAL_JUNK_BUS_LED3			0xf0030000UL

/* ==================================================================== */
/*                         Register "SH1_PTC_0"                         */
/*       Puge Translation Cache Message Configuration Information       */
/* ==================================================================== */
#define SH1_PTC_0			__IA64_UL_CONST(0x00000001101a0000)

/*   SH1_PTC_0_A                                                        */
/*   Description:  Type                                                 */
#define SH1_PTC_0_A_SHFT				0

/*   SH1_PTC_0_PS                                                       */
/*   Description:  Page Size                                            */
#define SH1_PTC_0_PS_SHFT				2

/*   SH1_PTC_0_RID                                                      */
/*   Description:  Region ID                                            */
#define SH1_PTC_0_RID_SHFT				8

/*   SH1_PTC_0_START                                                    */
/*   Description:  Start                                                */
#define SH1_PTC_0_START_SHFT				63

/* ==================================================================== */
/*                         Register "SH1_PTC_1"                         */
/*       Puge Translation Cache Message Configuration Information       */
/* ==================================================================== */
#define SH1_PTC_1			__IA64_UL_CONST(0x00000001101a0080)

/*   SH1_PTC_1_START                                                    */
/*   Description:  PTC_1 Start                                          */
#define SH1_PTC_1_START_SHFT				63

/* ==================================================================== */
/*                         Register "SH2_PTC"                           */
/*       Puge Translation Cache Message Configuration Information       */
/* ==================================================================== */
#define SH2_PTC				__IA64_UL_CONST(0x0000000170000000)

/*   SH2_PTC_A                                                          */
/*   Description:  Type                                                 */
#define SH2_PTC_A_SHFT					0

/*   SH2_PTC_PS                                                         */
/*   Description:  Page Size                                            */
#define SH2_PTC_PS_SHFT					2

/*   SH2_PTC_RID                                                      */
/*   Description:  Region ID                                            */
#define SH2_PTC_RID_SHFT				4

/*   SH2_PTC_START                                                      */
/*   Description:  Start                                                */
#define SH2_PTC_START_SHFT				63

/*   SH2_PTC_ADDR_RID                                                   */
/*   Description:  Region ID                                            */
#define SH2_PTC_ADDR_SHFT				4
#define SH2_PTC_ADDR_MASK		__IA64_UL_CONST(0x1ffffffffffff000)

/* ==================================================================== */
/*                    Register "SH_RTC1_INT_CONFIG"                     */
/*                SHub RTC 1 Interrupt Config Registers                 */
/* ==================================================================== */

#define SH1_RTC1_INT_CONFIG		__IA64_UL_CONST(0x0000000110001480)
#define SH2_RTC1_INT_CONFIG		__IA64_UL_CONST(0x0000000010001480)
#define SH_RTC1_INT_CONFIG_MASK		__IA64_UL_CONST(0x0ff3ffffffefffff)
#define SH_RTC1_INT_CONFIG_INIT		__IA64_UL_CONST(0x0000000000000000)

/*   SH_RTC1_INT_CONFIG_TYPE                                            */
/*   Description:  Type of Interrupt: 0=INT, 2=PMI, 4=NMI, 5=INIT       */
#define SH_RTC1_INT_CONFIG_TYPE_SHFT			0
#define SH_RTC1_INT_CONFIG_TYPE_MASK	__IA64_UL_CONST(0x0000000000000007)

/*   SH_RTC1_INT_CONFIG_AGT                                             */
/*   Description:  Agent, must be 0 for SHub                            */
#define SH_RTC1_INT_CONFIG_AGT_SHFT			3
#define SH_RTC1_INT_CONFIG_AGT_MASK	__IA64_UL_CONST(0x0000000000000008)

/*   SH_RTC1_INT_CONFIG_PID                                             */
/*   Description:  Processor ID, same setting as on targeted McKinley  */
#define SH_RTC1_INT_CONFIG_PID_SHFT			4
#define SH_RTC1_INT_CONFIG_PID_MASK	__IA64_UL_CONST(0x00000000000ffff0)

/*   SH_RTC1_INT_CONFIG_BASE                                            */
/*   Description:  Optional interrupt vector area, 2MB aligned          */
#define SH_RTC1_INT_CONFIG_BASE_SHFT			21
#define SH_RTC1_INT_CONFIG_BASE_MASK	__IA64_UL_CONST(0x0003ffffffe00000)

/*   SH_RTC1_INT_CONFIG_IDX                                             */
/*   Description:  Targeted McKinley interrupt vector                   */
#define SH_RTC1_INT_CONFIG_IDX_SHFT			52
#define SH_RTC1_INT_CONFIG_IDX_MASK	__IA64_UL_CONST(0x0ff0000000000000)

/* ==================================================================== */
/*                    Register "SH_RTC1_INT_ENABLE"                     */
/*                SHub RTC 1 Interrupt Enable Registers                 */
/* ==================================================================== */

#define SH1_RTC1_INT_ENABLE		__IA64_UL_CONST(0x0000000110001500)
#define SH2_RTC1_INT_ENABLE		__IA64_UL_CONST(0x0000000010001500)
#define SH_RTC1_INT_ENABLE_MASK		__IA64_UL_CONST(0x0000000000000001)
#define SH_RTC1_INT_ENABLE_INIT		__IA64_UL_CONST(0x0000000000000000)

/*   SH_RTC1_INT_ENABLE_RTC1_ENABLE                                     */
/*   Description:  Enable RTC 1 Interrupt                               */
#define SH_RTC1_INT_ENABLE_RTC1_ENABLE_SHFT		0
#define SH_RTC1_INT_ENABLE_RTC1_ENABLE_MASK \
					__IA64_UL_CONST(0x0000000000000001)

/* ==================================================================== */
/*                    Register "SH_RTC2_INT_CONFIG"                     */
/*                SHub RTC 2 Interrupt Config Registers                 */
/* ==================================================================== */

#define SH1_RTC2_INT_CONFIG		__IA64_UL_CONST(0x0000000110001580)
#define SH2_RTC2_INT_CONFIG		__IA64_UL_CONST(0x0000000010001580)
#define SH_RTC2_INT_CONFIG_MASK		__IA64_UL_CONST(0x0ff3ffffffefffff)
#define SH_RTC2_INT_CONFIG_INIT		__IA64_UL_CONST(0x0000000000000000)

/*   SH_RTC2_INT_CONFIG_TYPE                                            */
/*   Description:  Type of Interrupt: 0=INT, 2=PMI, 4=NMI, 5=INIT       */
#define SH_RTC2_INT_CONFIG_TYPE_SHFT			0
#define SH_RTC2_INT_CONFIG_TYPE_MASK	__IA64_UL_CONST(0x0000000000000007)

/*   SH_RTC2_INT_CONFIG_AGT                                             */
/*   Description:  Agent, must be 0 for SHub                            */
#define SH_RTC2_INT_CONFIG_AGT_SHFT			3
#define SH_RTC2_INT_CONFIG_AGT_MASK	__IA64_UL_CONST(0x0000000000000008)

/*   SH_RTC2_INT_CONFIG_PID                                             */
/*   Description:  Processor ID, same setting as on targeted McKinley  */
#define SH_RTC2_INT_CONFIG_PID_SHFT			4
#define SH_RTC2_INT_CONFIG_PID_MASK	__IA64_UL_CONST(0x00000000000ffff0)

/*   SH_RTC2_INT_CONFIG_BASE                                            */
/*   Description:  Optional interrupt vector area, 2MB aligned          */
#define SH_RTC2_INT_CONFIG_BASE_SHFT			21
#define SH_RTC2_INT_CONFIG_BASE_MASK	__IA64_UL_CONST(0x0003ffffffe00000)

/*   SH_RTC2_INT_CONFIG_IDX                                             */
/*   Description:  Targeted McKinley interrupt vector                   */
#define SH_RTC2_INT_CONFIG_IDX_SHFT			52
#define SH_RTC2_INT_CONFIG_IDX_MASK	__IA64_UL_CONST(0x0ff0000000000000)

/* ==================================================================== */
/*                    Register "SH_RTC2_INT_ENABLE"                     */
/*                SHub RTC 2 Interrupt Enable Registers                 */
/* ==================================================================== */

#define SH1_RTC2_INT_ENABLE		__IA64_UL_CONST(0x0000000110001600)
#define SH2_RTC2_INT_ENABLE		__IA64_UL_CONST(0x0000000010001600)
#define SH_RTC2_INT_ENABLE_MASK		__IA64_UL_CONST(0x0000000000000001)
#define SH_RTC2_INT_ENABLE_INIT		__IA64_UL_CONST(0x0000000000000000)

/*   SH_RTC2_INT_ENABLE_RTC2_ENABLE                                     */
/*   Description:  Enabl