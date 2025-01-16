nstr_translation_cache_fail	pme_tlb.itc
#define pmci_tlb_data_translation_cache_fail	pme_tlb.dtc
#define pmci_tlb_instr_translation_reg_fail	pme_tlb.itr
#define pmci_tlb_data_translation_reg_fail	pme_tlb.dtr
#define pmci_tlb_translation_reg_slot		pme_tlb.tr_slot
#define pmci_tlb_mc				pme_tlb.mc

#define pmci_bus_status_info			pme_bus.bsi
#define pmci_bus_req_address_valid		pme_bus.rq
#define pmci_bus_resp_address_valid		pme_bus.rp
#define pmci_bus_target_address_valid		pme_bus.tv
#define pmci_bus_error_severity			pme_bus.sev
#define pmci_bus_transaction_type		pme_bus.type
#define pmci_bus_cache_cache_transfer		pme_bus.cc
#define pmci_bus_transaction_size		pme_bus.size
#define pmci_bus_internal_error			pme_bus.ib
#define pmci_bus_external_error			pme_bus.eb
#define pmci_bus_mc				pme_bus.mc

/*
 * NOTE: this min_state_save area struct only includes the 1KB
 * architectural state save area.  The other 3 KB is scratch space
 * for PAL.
 */

typedef struct pal_min_state_area_s {
	u64	pmsa_nat_bits;		/* nat bits for saved GRs  */
	u64	pmsa_gr[15];		/* GR1	- GR15		   */
	u64	pmsa_bank0_gr[16];	/* GR16 - GR31		   */
	u64	pmsa_bank1_gr[16];	/* GR16 - GR31		   */
	u64	pmsa_pr;		/* predicate registers	   */
	u64	pmsa_br0;		/* branch register 0	   */
	u64	pmsa_rsc;		/* ar.rsc		   */
	u64	pmsa_iip;		/* cr.iip		   */
	u64	pmsa_ipsr;		/* cr.ipsr		   */
	u64	pmsa_ifs;		/* cr.ifs		   */
	u64	pmsa_xip;		/* previous iip		   */
	u64	pmsa_xpsr;		/* previous psr		   */
	u64	pmsa_xfs;		/* previous ifs		   */
	u64	pmsa_br1;		/* branch register 1	   */
	u64	pmsa_reserved[70];	/* pal_min_state_area should total to 1KB */
} pal_min_state_area_t;


struct ia64_pal_retval {
	/*
	 * A zero status value indicates call completed without error.
	 * A negative status value indicates reason of call failure.
	 * A positive status value indicates success but an
	 * informational value should be printed (e.g., "reboot for
	 * change to take effect").
	 */
	s64 status;
	u64 v0;
	u64 v1;
	u64 v2;
};

/*
 * Note: Currently unused PAL arguments are generally labeled
 * "reserved" so the value specified in the PAL documentation
 * (generally 0) MUST be passed.  Reserved parameters are not optional
 * parameters.
 */
extern struct ia64_pal_retval ia64_pal_call_static (u64, u64, u64, u64);
extern struct ia64_pal_retval ia64_pal_call_stacked (u64, u64, u64, u64);
extern struct ia64_pal_retval ia64_pal_call_phys_static (u64, u64, u64, u64);
extern struct ia64_pal_retval ia64_pal_call_phys_stacked (u64, u64, u64, u64);
extern void ia64_save_scratch_fpregs (struct ia64_fpreg *);
extern void ia64_load_scratch_fpregs (struct ia64_fpreg *);

#define PAL_C