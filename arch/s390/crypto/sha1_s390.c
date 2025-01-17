 have
	 * NODE_ID, CHIPLET_ID, and SYS_ADDR in the correct locations.
	 */

	return ((paddr) >> 12) | (1UL << 63);
}

/**
 * tioca_physpage_to_gart - Map a host physical page for SGI CA based DMA
 * @page_addr: system page address to map
 */

static inline unsigned long
tioca_physpage_to_gart(u64 page_addr)
{
	u64 coretalk_addr;

	coretalk_addr = PHYS_TO_TIODMA(page_addr);
	if (!coretalk_addr) {
		return 0;
	}

	return tioca_paddr_to_gart(coretalk_addr);
}

/**
 * tioca_tlbflush - invalidate cached SGI CA GART TLB entries
 * @tioca_kernel: CA context 
 *
 * Invalidate tlb entries for a given CA GART.  Main complexity is to account
 * for revA bug.
 */
static inline void
tioca_tlbflush(struct tioca_kernel *tioca_kernel)
{
	volatile u64 tmp;
	volatile struct tioca __iomem *ca_base;
	struct tioca_common *tioca_common;

	tioca_common = tioca_kernel->ca_common;
	ca_base = (struct tioca __iomem *)tioca_common->ca_common.bs_base;

	/*
	 * Explicit flushes not needed if GART is in cached mode
	 */
	if (tioca_kernel->ca_gart_iscoherent) {
		if (TIOCA_WAR_ENABLED(PV910244, tioca_common)) {
			/*
			 * PV910244:  RevA CA needs explicit flushes.
			 * Need to put GART into uncached mode before
			 * flushing otherwise the explicit flush is ignored.
			 *
			 * Alternate WAR would be to leave GART cached and
			 * touch every CL aligned GART entry.
			 */

			__sn_clrq_relaxed(&ca_base->ca_control2, CA_GART_MEM_PARAM);
			__sn_setq_relaxed(&ca_base->ca_control2, CA_GART_FLUSH_TLB);
			__sn_setq_relaxed(&ca_base->ca_control2,
			    (0x2ull << CA_GART_MEM_PARAM_SHFT));
			tmp = __sn_readq_relaxed(&ca_base->ca_control2);
		}

		return;
	}

	/*
	 * Gart in uncached mode ... need an explicit flush.
	 */

	__sn_setq_relaxed(&ca_base->ca_control2, CA_GART_FLUSH_TLB);
	tmp = __sn_readq_relaxed(&ca_base->ca_control2);
}

extern u32	tioca_gart_found;
extern struct list_head tioca_list;
extern int tioca_init_provider(void);
extern void tioca_fastwrite_enable(struct tioca_kernel *tioca_kern);
#endif /* _ASM_IA64_SN_TIO_CA_AGP_PROVIDER_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         