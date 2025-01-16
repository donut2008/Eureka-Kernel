r       : 1,
		    precise_ip              : 1,
		    reserved                : 59;
	} valid;
	u64 check_info;
	u64 requestor_identifier;
	u64 responder_identifier;
	u64 target_identifier;
	u64 precise_ip;
} sal_log_mod_error_info_t;

typedef struct sal_processor_static_info {
	struct {
		u64 minstate        : 1,
		    br              : 1,
		    cr              : 1,
		    ar              : 1,
		    rr              : 1,
		    fr              : 1,
		    reserved        : 58;
	} valid;
	pal_min_state_area_t min_state_area;
	u64 br[8];
	u64 cr[128];
	u64 ar[128];
	u64 rr[8];
	struct ia64_fpreg __attribute__ ((packed)) fr[128];
} sal_processor_static_info_t;

struct sal_cpuid_info {
	u64 regs[5];
	u64 reserved;
};

typedef struct sal_log_processor_info {
	sal_log_section_hdr_t header;
	struct {
		u64 proc_error_map      : 1,
		    proc_state_param    : 1,
		    proc_cr_lid         : 1,
		    psi_static_struct   : 1,
		    num_cache_check     : 4,
		    num_tlb_check       : 4,
		    num_bus_check       : 4,
		    num_reg_file_check  : 4,
		    num_ms_check        : 4,
		    cpuid_info          : 1,
		    reserved1           : 39;
	} valid;
	u64 proc_error_map;
	u64 proc_state_parameter;
	u64 proc_cr_lid;
	/*
	 * The rest of this structure consists of variable-length arrays, which can't be
	 * expressed in C.
	 */
	sal_log_mod_error_info_t info[0];
	/*
	 * This is what the rest looked like if C supported variable-length arrays:
	 *
	 * sal_log_mod_error_info_t cache_check_info[.valid.num_cache_check];
	 * sal_log_mod_error_info_t tlb_check_info[.valid.num_tlb_check];
	 * sal_log_mod_error_info_t bus_check_info[.valid.num_bus_check];
	 * sal_log_mod_error_info_t reg_file_check_info[.valid.num_reg_file_check];
	 * sal_log_mod_error_info_t ms_check_info[.valid.num_ms_check];
	 * struct sal_cpuid_info cpuid_info;
	 * sal_processor_static_info_t processor_static_info;
	 */
} sal_log_processor_info_t;

/* Given a sal_log_processor_info_t pointer, return a pointer to the processor_static_info: */
#define SAL_LPI_PSI_INFO(l)									\
({	sal_log_processor_info_t *_l = (l);							\
	((sal_processor_static_info_t *)							\
	 ((char *) _l->info + ((_l->valid.num_cache_check + _l->valid.num_tlb_check		\
				+ _l->valid.num_bus_check + _l->valid.num_reg_file_check	\
				+ _l->valid.num_ms_check) * sizeof(sal_log_mod_error_info_t)	\
			       + sizeof(struct sal_cpuid_info))));				\
})

/* platform error log structures */

typedef struct sal_log_mem_dev_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 error_status    : 1,
		    physical_addr   : 1,
		    addr_mask       : 1,
		    node      