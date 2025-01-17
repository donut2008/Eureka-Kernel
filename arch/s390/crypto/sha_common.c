 SHUB1 */
/* and SHUB2 that it makes sense to define a geberic name for the MMR.  */
/* It is acceptable to use (for example) SH_IPI_INT to reference the    */
/* the IPI MMR. The value of SH_IPI_INT is determined at runtime based  */
/* on the type of the SHUB. Do not use these #defines in performance    */
/* critical code  or loops - there is a small performance penalty.      */
/* ==================================================================== */
#define shubmmr(a,b) 		(is_shub2() ? a##2_##b : a##1_##b)

#define SH_REAL_JUNK_BUS_LED0	shubmmr(SH, REAL_JUNK_BUS_LED0)
#define SH_IPI_INT		shubmmr(SH, IPI_INT)
#define SH_EVENT_OCCURRED	shubmmr(SH, EVENT_OCCURRED)
#define SH_EVENT_OCCURRED_ALIAS	shubmmr(SH, EVENT_OCCURRED_ALIAS)
#define SH_RTC			shubmmr(SH, RTC)
#define SH_RTC1_INT_CONFIG	shubmmr(SH, RTC1_INT_CONFIG)
#define SH_RTC1_INT_ENABLE	shubmmr(SH, RTC1_INT_ENABLE)
#define SH_RTC2_INT_CONFIG	shubmmr(SH, RTC2_INT_CONFIG)
#define SH_RTC2_INT_ENABLE	shubmmr(SH, RTC2_INT_ENABLE)
#define SH_RTC3_INT_CONFIG	shubmmr(SH, RTC3_INT_CONFIG)
#define SH_RTC3_INT_ENABLE	shubmmr(SH, RTC3_INT_ENABLE)
#define SH_INT_CMPB		shubmmr(SH, INT_CMPB)
#define SH_INT_CMPC		shubmmr(SH, INT_CMPC)
#define SH_INT_CMPD		shubmmr(SH, INT_CMPD)

/* ========================================================================== */
/*                        Register "SH2_BT_ENG_CSR_0"                         */
/*                    Engine 0 Control and Status Register                    */
/* ========================================================================== */

#define SH2_BT_ENG_CSR_0		__IA64_UL_CONST(0x0000000030040000)
#define SH2_BT_ENG_SRC_ADDR_0		__IA64_UL_CONST(0x0000000030040080)
#define SH2_BT_ENG_DEST_ADDR_0		__IA64_UL_CONST(0x0000000030040100)
#define SH2_BT_ENG_NOTIF_ADDR_0		__IA64_UL_CONST(0x0000000030040180)

/* ========================================================================== */
/*                       BTE interfaces 1-3                                   */
/* ========================================================================== */

#define SH2_BT_ENG_CSR_1		__IA64_UL_CONST(0x0000000030050000)
#define SH2_BT_ENG_CSR_2		__IA64_UL_CONST(0x0000000030060000)
#define SH2_BT_ENG_CSR_3		__IA64_UL_CONST(0x0000000030070000)

#endif /* _ASM_IA64_SN_SHUB_MMR_H */
                                                                                                                                                                                                                                                                                                                                          