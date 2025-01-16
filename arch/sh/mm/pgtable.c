ne GET_BASE		(PAGE_OFFSET | AS_GET_SPACE)

/*
 * Convert Memory addresses between various addressing modes.
 */
#define TO_PHYS(x)		(TO_PHYS_MASK & (x))
#define TO_CAC(x)		(CAC_BASE     | TO_PHYS(x))
#ifdef CONFIG_SGI_SN
#define TO_AMO(x)		(AMO_BASE     | TO_PHYS(x))
#define TO_GET(x)		(GET_BASE     | TO_PHYS(x))
#else
#define TO_AMO(x)		({ BUG(); x; })
#define TO_GET(x)		({ BUG(); x; })
#endif

/*
 * Covert from processor physical address to II/TIO physical address:
 *	II - squeeze out the AS bits
 *	TIO- requires a chiplet id in bits 38-39.  For DMA to memory,
 *           the chiplet id is zero.  If we implement TIO-TIO dma, we might need
 *           to insert a chiplet id into this macro.  However, it is our belief
 *           right now that this chiplet id will be ICE, which is also zero.
 */
#define SH1_TIO_PHYS_TO_DMA(x) 						\
	((((u64)(NASID_GET(x))) << 40) | NODE_OFFSET(x))

#define SH2_NETWORK_BANK_OFFSET(x) 					\
        ((u64)(x) & ((1UL << (sn_hub_info->nasid_shift - 4)) -1))

#define SH2_NETWORK_BANK_SELECT(x) 					\
        ((((u64)(x) & (0x3UL << (sn_hub_info->nasid_shift - 4)))	\
        	>> (sn_hub_info->nasid_shift - 4)) << 36)

#define SH2_NETWORK_ADDRESS(x) 						\
	(SH2_NETWORK_BANK_OFFSET(x) | SH2_NETWORK_BANK_SELECT(x))

#define SH2_TIO_PHYS_TO_DMA