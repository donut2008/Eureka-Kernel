l processor mapping */
#define PAL_CACHE_SHARED_INFO	43	/* returns information on caches shared by logical processor */
#define PAL_GET_HW_POLICY	48	/* Get current hardware resource sharing policy */
#define PAL_SET_HW_POLICY	49	/* Set current hardware resource sharing policy */
#define PAL_VP_INFO		50	/* Information about virtual processor features */
#define PAL_MC_HW_TRACKING	51	/* Hardware tracking status */

#define PAL_COPY_PAL		256	/* relocate PAL procedures and PAL PMI */
#define PAL_HALT_INFO		257	/* return the low power capabilities of processor */
#define PAL_TEST_PROC		258	/* perform late processor self-test */
#define PAL_CACHE_READ		259	/* read tag & data of cacheline for diagnostic testing */
#define PAL_CACHE_WRITE		260	/* write tag & data of cacheline for diagnostic testing */
#define PAL_VM_TR_READ		261	/* read contents of translation register */
#define PAL_GET_PSTATE		262	/* get the current P-state */
#define PAL_SET_PSTATE		263	/* set the P-state */
#define PAL_BRAND_INFO		274	/* Processor branding information */

#define PAL_GET_PSTATE_TYPE_LASTSET	0
#define PAL_GET_PSTATE_TYPE_AVGANDRESET	1
#define PAL_GET_PSTATE_TYPE_AVGNORESET	2
#define PAL_GET_PSTATE_TYPE_INSTANT	3

#define PAL_MC_ERROR_INJECT	276	/* Injects processor error or returns injection capabilities */

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <asm/fpu.h>

/*
 * Data types needed to pass information into PAL procedures and
 * interpret information returned by them.
 */

/* Return status from the PAL procedure */
typedef s64				pal_status_t;

#define PAL_STATUS_SUCCESS		0	/* No error */
#define PAL_STATUS_UNIMPLEMENTED	(-1)	/* Unimplemented procedure */
#define PAL_STATUS_EINVAL		(-2)	/* Invalid argument */
#define PAL_STATUS_ERROR		(-3)	/* Error */
#define PAL_STATUS_CACHE_INIT_FAIL	(-4)	/* Could not initialize the
						 * specified level and type of
						 * cache without sideeffects
						 * and "restrict" was 1
						 */
#define PAL_STATUS_REQUIRES_MEMORY	(-9)	/* Call requires PAL memory buffer */

/* Processor cache level in the hierarchy */
typedef u64				pal_cache_level_t;
#define PAL_CACHE_LEVEL_L0		0	/* L0 */
#define PAL_CACHE_LEVEL_L1		1	/* L1 */
#define PAL_CACHE_LEVEL_L2		2	/* L2 */


/* Processor cache type at a particular level in the hierarchy */

typedef u64				pal_cache_type_t;
#define PAL_CACHE_TYPE_INSTRUCTION	1	/* Instruction cache */
#define PAL_CACHE_TYPE_DATA		2	/* Data or unified cache */
#define PAL_CACHE_TYPE_INSTRUCTION_DATA	3	/* Both Data & Instruction */


#define PAL_CACHE_FLUSH_INVALIDATE	1	/* Invalidate clean lines */
#define PAL_CACHE_FLUSH_CHK_INTRS	2	/* check for interrupts/mc while flushing */

/* Processor cache line size in bytes  */
typedef int				pal_cache_line_size_t;

/* Processor cache line state */
typedef u64				pal_cache_line_state_t;
#define PAL_CACHE_LINE_STATE_INVALID	0	/* Invalid */
#define PAL_CACHE_LINE_STATE_SHARED	1	/* Shared */
#define PAL_CACHE_LINE_STATE_EXCLUSIVE	2	/* Exclusive */
#define PAL_CACHE_LINE_STATE_MODIFIED	3	/* Modified */

typedef struct pal_freq_ratio {
	u32 den, num;		/* numerator & denominator */
} itc_ratio, proc_ratio;

typedef	union  pal_cache_config_info_1_s {
	struct {
		u64		u		: 1,	/* 0 Unified cache ? */
				at		: 2,	/* 2-1 Cache mem attr*/
				reserved	: 5,	/* 7-3 Reserved */
				associativity	: 8,	/* 16-8 Associativity*/
				line_size	: 8,	/* 23-17 Line size */
				stride		: 8,	/* 31-24 Stride */
				store_latency	: 8,	/*39-32 Store latency*/
				load_latency	: 8,	/* 47-40 Load latency*/
				store_hints	: 8,	/* 55-48 Store hints*/
				load_hints	: 8;	/* 63-56 Load hints */
	} pcci1_bits;
	u64			pcci1_data;
} pal_cache_config_info_1_t;

typedef	union  pal_cache_config_info_2_s {
	struct {
		u32		cache_size;		/*cache size in bytes*/


		u32		alias_boundary	: 8,	/* 39-32 aliased addr
							 * separation for max
							 * performance.
							 */
				tag_ls_bit	: 8,	/* 47-40 LSb of addr*/
				tag_ms_bit	: 8,	/* 55-48 MSb of addr*/
				reserved	: 8;	/* 63-56 Reserved */
	} pcci2_bits;
	u64			pcci2_data;
} pal_cache_config_info_2_t;


typedef struct pal_cache_config_info_s {
	pal_status_t			pcci_status;
	pal_cache_config_info_1_t	pcci_info_1;
	pal_cache_config_info_2_t	pcci_info_2;
	u64				pcci_reserved;
} pal_cache_config_info_t;

#define pcci_ld_hints		pcci_info_1.pcci1_bits.load_hints
#define pcci_st_hints		pcci_info_1.pcci1_bits.store_hints
#define pcci_ld_latency		pcci_info_1.pcci1_bits.load_latency
#define pcci_st_latency		pcci_info_1.pcci1_bits.store_latency
#define pcci_stride		pcci_info_1.pcci1_bits.stride
#define pcci_line_size		pcci_info_1.pcci1_bits.line_size
#define pcci_assoc		pcci_info_1.pcci1_bits.associativity
#define pcci_cache_attr		pcci_info_1.pcci1_bits.at
#define pcci_unified		pcci_info_1.pcci1_bits.u
#define pcci_tag_msb		pcci_info_2.pcci2_bits.tag_ms_bit
#define pcci_tag_lsb		pcci_info_2.pcci2_bits.tag_ls_bit
#define pcci_alias_boundary	pcci_info_2.pcci2_bits.alias_boundary
#define pcci_cache_size		pcci_info_2.pcci2_bits.cache_size



/* Possible values for cache attributes */

#define PAL_CACHE_ATTR_WT		0	/* Write through cache */
#define PAL_CACHE_ATTR_WB		1	/* Write back cache */
#define PAL_CACHE_ATTR_WT_OR_WB		2	/* Either write thru or write
						 * back depending on TLB
						 * memory attributes
						 */


/* Possible values for cache hints */

#define PAL_CACHE_HINT_TEMP_1		0	/* Temporal level 1 */
#define PAL_CACHE_HINT_NTEMP_1		1	/* Non-temporal level 1 */
#define PAL_CACHE_HINT_NTEMP_ALL	3	/* Non-temporal all levels */

/* Processor cache protection  information */
typedef union pal_cache_protection_element_u {
	u32			pcpi_data;
	struct {
		u32		data_bits	: 8, /* # data bits covered by
						      * each unit of protection
						      */

				tagprot_lsb	: 6, /* Least -do- */
				tagprot_msb	: 6, /* Most Sig. tag address
						      * bit that this
						      * protection covers.
						      */
				prot_bits	: 6, /* # of protection bits */
				method		: 4, /* Protection method */
				t_d		: 2; /* Indicates which part
						      * of the cache this
						      * protection encoding
						      * applies.
						      */
	} pcp_info;
} pal_cache_protection_element_t;

#define pcpi_cache_prot_part	pcp_info.t_d
#define pcpi_prot_method	pcp_info.method
#define pcpi_prot_bits		pcp_info.prot_bits
#define pcpi_tagprot_msb	pcp_info.tagprot_msb
#define pcpi_tagprot_lsb	pcp_info.tagprot_lsb
#define pcpi_data_bits		pcp_info.data_bits

/* Processor cache part encodings */
#define PAL_CACHE_PROT_PART_DATA	0	/* Data protection  */
#define PAL_CACHE_PROT_PART_TAG		1	/* Tag  protection */
#define PAL_CACHE_PROT_PART_TAG_DATA	2	/* Tag+data protection (tag is
						 * more significant )
						 */
#define PAL_CACHE_PROT_PART_DATA_TAG	3	/* Data+tag protection (data is
						 * more significant )
						 */
#define PAL_CACHE_PROT_PART_MAX		6


typedef struct pal_cache_protection_info_s {
	pal_status_t			pcpi_status;
	pal_cache_protection_element_t	pcp_info[PAL_CACHE_PROT_PART_MAX];
} pal_cache_protection_info_t;


/* Processor cache protection method encodings */
#define PAL_CACHE_PROT_METHOD_NONE		0	/* No protection */
#define PAL_CACHE_PROT_METHOD_ODD_PARITY	1	/* Odd parity */
#define PAL_CACHE_PROT_METHOD_EVEN_PARITY	2	/* Even parity */
#define PAL_CACHE_PROT_METHOD_ECC		3	/* ECC protection */


/* Processor cache line identification in the hierarchy */
typedef union pal_cache_line_id_u {
	u64			pclid_data;
	struct {
		u64		cache_type	: 8,	/* 7-0 cache type */
				level		: 8,	/* 15-8 level of the
							 * cache in the
							 * hierarchy.
							 */
				way		: 8,	/* 23-16 way in the set
							 */
				part		: 8,	/* 31-24 part of the
							 * cache
							 */
				reserved	: 32;	/* 63-32 is reserved*/
	} pclid_info_read;
	struct {
		u64		cache_type	: 8,	/* 7-0 cache type */
				level		: 8,	/* 15-8 level of the
							 * cache in the
							 * hierarchy.
							 */
				way		: 8,	/* 23-16 way in the set
							 */
				part		: 8,	/* 31-24 part of the
							 * cache
							 */
				mesi		: 8,	/* 39-32 cache line
							 * state
							 */
				start		: 8,	/* 47-40 lsb of data to
							 * invert
							 */
				length		: 8,	/* 55-48 #bits to
							 * invert
							 */
				trigger		: 8;	/* 63-56 Trigger error
							 * by doing a load
							 * after the write
							 */

	} pclid_info_write;
} pal_cache_line_id_u_t;

#define pclid_read_part		pclid_info_read.part
#define pclid_read_way		pclid_info_read.way
#define pclid_read_level	pclid_info_read.level
#define pclid_read_cache_type	pclid_info_read.cache_type

#define pclid_write_trigger	pclid_info_write.trigger
#define pclid_write_length	pclid_info_write.length
#define pclid_write_start	pclid_info_write.start
#define pclid_write_mesi	pclid_info_write.mesi
#define pclid_write_part	pclid_info_write.part
#define pclid_write_way		pclid_info_write.way
#define pclid_write_level	pclid_info_write.level
#define pclid_write_cache_type	pclid_info_write.cache_type

/* Processor cache line part encodings */
#define PAL_CACHE_LINE_ID_PART_DATA		0	/* Data */
#define PAL_CACHE_LINE_ID_PART_TAG		1	/* Tag */
#define PAL_CACHE_LINE_ID_PART_DATA_PROT	2	/* Data protection */
#define PAL_CACHE_LINE_ID_PART_TAG_PROT		3	/* Tag protection */
#define PAL_CACHE_LINE_ID_PART_DATA_TAG_PROT	4	/* Data+tag
							 * protection
							 */
typedef struct pal_cache_line_info_s {
	pal_status_t		pcli_status;		/* Return status of the read cache line
							 * info call.
							 */
	u64			pcli_data;		/* 64-bit data, tag, protection bits .. */
	u64			pcli_data_len;		/* data length in bits */
	pal_cache_line_state_t	pcli_cache_line_state;	/* mesi state */

} pal_cache_line_info_t;


/* Machine Check related crap */

/* Pending event status bits  */
typedef u64					pal_mc_pending_events_t;

#define PAL_MC_PENDING_MCA			(1 << 0)
#define PAL_MC_PENDING_INIT			(1 << 1)

/* Error information type */
typedef u64					pal_mc_info_index_t;

#define PAL_MC_INFO_PROCESSOR			0	/* Processor */
#define PAL_MC_INFO_CACHE_CHECK			1	/* Cache check */
#define PAL_MC_INFO_TLB_CHECK			2	/* Tlb check */
#define PAL_MC_INFO_BUS_CHECK			3	/* Bus check */
#define PAL_MC_INFO_REQ_ADDR			4	/* Requestor address */
#define PAL_MC_INFO_RESP_ADDR			5	/* Responder address */
#define PAL_MC_INFO_TARGET_ADDR			6	/* Target address */
#define PAL_MC_INFO_IMPL_DEP			7	/* Implementation
							 * dependent
							 */

#define PAL_TLB_CHECK_OP_PURGE			8

typedef struct pal_process_state_info_s {
	u64		reserved1	: 2,
			rz		: 1,	/* PAL_CHECK processor
						 * rendezvous
						 * successful.
						 */

			ra		: 1,	/* PAL_CHECK attempted
						 * a rendezvous.
						 */
			me		: 1,	/* Distinct multiple
						 * errors occurred
						 */

			mn		: 1,	/* Min. state save
						 * area has been
						 * registered with PAL
						 */

			sy		: 1,	/* Storage integrity
						 * synched
						 */


			co		: 1,	/* Continuable */
			ci		: 1,	/* MC isolated */
			us		: 1,	/* Uncontained storage
						 * damage.
						 */


			hd		: 1,	/* Non-essential hw
						 * lost (no loss of
						 * functionality)
						 * causing the
						 * processor to run in
						 * degraded mode.
						 */

			tl		: 1,	/* 1 => MC occurred
						 * after an instr was
						 * executed but before
						 * the trap that
						 * resulted from instr
						 * execution was
						 * generated.
						 * (Trap Lost )
						 */
			mi		: 1,	/* More information available
						 * call PAL_MC_ERROR_INFO
						 */
			pi		: 1,	/* Precise instruction pointer */
			pm		: 1,	/* Precise min-state save area */

			dy		: 1,	/* Processor dynamic
						 * state valid
						 */


			in		: 1,	/* 0 = MC, 1 = INIT */
			rs		: 1,	/* RSE valid */
			cm		: 1,	/* MC corrected */
			ex		: 1,	/* MC is expected */
			cr		: 1,	/* Control regs valid*/
			pc		: 1,	/* Perf cntrs valid */
			dr		: 1,	/* Debug regs valid */
			tr		: 1,	/* Translation regs
						 * valid
						 */
			rr		: 1,	/* Region regs valid */
			ar		: 1,	/* App regs valid */
			br		: 1,	/* Branch regs valid */
			pr		: 1,	/* Predicate registers
						 * valid
						 */

			fp		: 1,	/* fp registers valid*/
			b1		: 1,	/* Preserved bank one
						 * general registers
						 * are valid
						 */
			b0		: 1,	/* Preserved bank zero
						 * general registers
						 * are valid
						 */
			gr		: 1,	/* General registers
						 * are valid
						 * (excl. banked regs)
						 */
			dsize		: 16,	/* size of dynamic
						 * state returned
						 * by the processor
						 */

			se		: 1,	/* Shared error.  MCA in a
						   shared structure */
			reserved2	: 10,
			cc		: 1,	/* Cache check */
			tc		: 1,	/* TLB check */
			bc		: 1,	/* Bus check */
			rc		: 1,	/* Register file check */
			uc		: 1;	/* Uarch check */

} pal_processor_state_info_t;

typedef struct pal_cache_check_info_s {
	u64		op		: 4,	/* Type of cache
						 * operation that
						 * caused the machine
						 * check.
						 */
			level		: 2,	/* Cache level */
			reserved1	: 2,
			dl		: 1,	/* Failure in data part
						 * of cache line
						 */
			tl		: 1,	/* Failure in tag part
						 * of cache line
						 */
			dc		: 1,	/* Failure in dcache */
			ic		: 1,	/* Failure in icache */
			mesi		: 3,	/* Cache line state */
			mv		: 1,	/* mesi valid */
			way		: 5,	/* Way in which the
						 * error occurred
						 */
			wiv		: 1,	/* Way field valid */
			reserved2	: 1,
			dp		: 1,	/* Data poisoned on MBE */
			reserved3	: 6,
			hlth		: 2,	/* Health indicator */

			index		: 20,	/* Cache line index */
			reserved4	: 2,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			tv		: 1,	/* Target address
						 * structure is valid
						 */
			rq		: 1,	/* Requester identifier
						 * structure is valid
						 */
			rp		: 1,	/* Responder identifier
						 * structure is valid
						 */
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_cache_check_info_t;

typedef struct pal_tlb_check_info_s {

	u64		tr_slot		: 8,	/* Slot# of TR where
						 * error occurred
						 */
			trv		: 1,	/* tr_slot field is valid */
			reserved1	: 1,
			level		: 2,	/* TLB level where failure occurred */
			reserved2	: 4,
			dtr		: 1,	/* Fail in data TR */
			itr		: 1,	/* Fail in inst TR */
			dtc		: 1,	/* Fail in data TC */
			itc		: 1,	/* Fail in inst. TC */
			op		: 4,	/* Cache operation */
			reserved3	: 6,
			hlth		: 2,	/* Health indicator */
			reserved4	: 22,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			tv		: 1,	/* Target address
						 * structure is valid
						 */
			rq		: 1,	/* Requester identifier
						 * structure is valid
						 */
			rp		: 1,	/* Responder identifier
						 * structure is valid
						 */
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_tlb_check_info_t;

typedef struct pal_bus_check_info_s {
	u64		size		: 5,	/* Xaction size */
			ib		: 1,	/* Internal bus error */
			eb		: 1,	/* External bus error */
			cc		: 1,	/* Error occurred
						 * during cache-cache
						 * transfer.
						 */
			type		: 8,	/* Bus xaction type*/
			sev		: 5,	/* Bus error severity*/
			hier		: 2,	/* Bus hierarchy level */
			dp		: 1,	/* Data poisoned on MBE */
			bsi		: 8,	/* Bus error status
						 * info
						 */
			reserved2	: 22,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			tv		: 1,	/* Target address
						 * structure is valid
						 */
			rq		: 1,	/* Requester identifier
						 * structure is valid
						 */
			rp		: 1,	/* Responder identifier
						 * structure is valid
						 */
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_bus_check_info_t;

typedef struct pal_reg_file_check_info_s {
	u64		id		: 4,	/* Register file identifier */
			op		: 4,	/* Type of register
						 * operation that
						 * caused the machine
						 * check.
						 */
			reg_num		: 7,	/* Register number */
			rnv		: 1,	/* reg_num valid */
			reserved2	: 38,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			reserved3	: 3,
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_reg_file_check_info_t;

typedef struct pal_uarch_check_info_s {
	u64		sid		: 5,	/* Structure identification */
			level		: 3,	/* Level of failure */
			array_id	: 4,	/* Array identification */
			op		: 4,	/* Type of
						 * operation that
						 * caused the machine
						 * check.
						 */
			way		: 6,	/* Way of structure */
			wv		: 1,	/* way valid */
			xv		: 1,	/* index valid */
			reserved1	: 6,
			hlth		: 2,	/* Health indicator */
			index		: 8,	/* Index or set of the uarch
						 * structure that failed.
						 */
			reserved2	: 24,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			tv		: 1,	/* Target address
						 * structure is valid
						 */
			rq		: 1,	/* Requester identifier
						 * structure is valid
						 */
			rp		: 1,	/* Responder identifier
						 * structure is valid
						 */
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_uarch_check_info_t;

typedef union pal_mc_error_info_u {
	u64				pmei_data;
	pal_processor_state_info_t	pme_processor;
	pal_cache_check_info_t		pme_cache;
	pal_tlb_check_info_t		pme_tlb;
	pal_bus_check_info_t		pme_bus;
	pal_reg_file_check_info_t	pme_reg_file;
	pal_uarch_check_info_t		pme_uarch;
} pal_mc_error_info_t;

#define pmci_proc_unknown_check			pme_processor.uc
#define pmci_proc_bus_check			pme_processor.bc
#define pmci_proc_tlb_check			pme_processor.tc
#define pmci_proc_cache_check			pme_processor.cc
#define pmci_proc_dynamic_state_size		pme_processor.dsize
#define pmci_proc_gpr_valid			pme_processor.gr
#define pmci_proc_preserved_bank0_gpr_valid	pme_processor.b0
#define pmci_proc_preserved_bank1_gpr_valid	pme_processor.b1
#define pmci_proc_fp_valid			pme_processor.fp
#define pmci_proc_predicate_regs_valid		pme_processor.pr
#define pmci_proc_branch_regs_valid		pme_processor.br
#define pmci_proc_app_regs_valid		pme_processor.ar
#define pmci_proc_region_regs_valid		pme_processor.rr
#define pmci_proc_translation_regs_valid	pme_processor.tr
#define pmci_proc_debug_regs_valid		pme_processor.dr
#define pmci_proc_perf_counters_valid		pme_processor.pc
#define pmci_proc_control_regs_valid		pme_processor.cr
#define pmci_proc_machine_check_expected	pme_processor.ex
#define pmci_proc_machine_check_corrected	pme_processor.cm
#define pmci_proc_rse_valid			pme_processor.rs
#define pmci_proc_machine_check_or_init		pme_processor.in
#define pmci_proc_dynamic_state_valid		pme_processor.dy
#define pmci_proc_operation			pme_processor.op
#define pmci_proc_trap_lost			pme_processor.tl
#define pmci_proc_hardware_damage		pme_processor.hd
#define pmci_proc_uncontained_storage_damage	pme_processor.us
#define pmci_proc_machine_check_isolated	pme_processor.ci
#define pmci_proc_continuable			pme_processor.co
#define pmci_proc_storage_intergrity_synced	pme_processor.sy
#define pmci_proc_min_state_save_area_regd	pme_processor.mn
#define	pmci_proc_distinct_multiple_errors	pme_processor.me
#def