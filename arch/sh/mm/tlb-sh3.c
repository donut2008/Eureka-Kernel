evel, tc_type, 0);
	if (tc_info)
		tc_info->pti_val = iprv.v0;
	if (tc_pages)
		*tc_pages = iprv.v1;
	return iprv.status;
}

/* Get page size information about the virtual memory characteristics of the processor
 * implementation.
 */
static inline s64 ia64_pal_vm_page_size(u64 *tr_pages, u64 *vw_pages)
{
	struct ia64_pal_retval iprv;
	PAL_CALL(iprv, PAL_VM_PAGE_SIZE, 0, 0, 0);
	if (tr_pages)
		*tr_pages = iprv.v0;
	if (vw_pages)
		*vw_pages = iprv.v1;
	return iprv.status;
}

typedef union pal_vm_info_1_u {
	u64			pvi1_val;
	struct {
		u64		vw		: 1,
				phys_add_size	: 7,
				key_size	: 8,
				max_pkr		: 8,
				hash_tag_id	: 8,
				max_dtr_entry	: 8,
				max_itr_entry	: 8,
				max_unique_tcs	: 8,
				num_tc_levels	: 8;
	} pal_vm_info_1_s;
} pal_vm_info_1_u_t;

#define PAL_MAX_PURGES		0xFFFF		/* all ones is means unlimited */

typedef union pal_vm_info_2_u {
	u64			pvi2_val;
	struct {
		u64		impl_va_msb	: 8,
				rid_size	: 8,
				max_purges	: 16,
				reserved	: 32;
	} pal_vm_info_2_s;
} pal_vm_info_2_u_t;

/* Get summary information about the virtual memory characteristics of the processor
 * implementation.
 */
static inline s64
ia64_pal_vm_summary (pal_vm_info_1_u_t *vm_info_1, pal_vm_info_2_u_t *vm_info_2)
{
	struct ia64_pal_retval iprv;
	PAL_CALL(iprv, PAL_VM_SUMMARY, 0, 0, 0);
	if (vm_info_1)
		vm_info_1->pvi1_val = iprv.v0;
	if (vm_info_2)
		vm_info_2->pvi2_val = iprv.v1;
	return iprv.status;
}

typedef union pal_vp_info_u {
	u64			pvi_val;
	struct {
		u64		index:		48,	/* virtual feature set info */
				vmm_id:		16;	/* feature set id */
	} pal_vp_info_s;
} pal_vp_info_u_t;

/*
 * Returns information about virtual processor features
 */
static inline s64
ia64_pal_vp_info (u64 feature_set, u64 vp_buffer, u64 *vp_info, u64 *vmm_id)
{
	struct ia64_pal_retval iprv;
	PAL_CALL(iprv, PAL_VP_INFO, feature_set, vp_buffer, 0);
	if (vp_info)
		*vp_info = iprv.v0;
	if (vmm_id)
		*vmm_id = iprv.v1;
	return iprv.status;
}

typedef union pal_itr_valid_u {
	u64			piv_val;
	struct {
	       u64		access_rights_valid	: 1,
				priv_level_valid	: 1,
				dirty_bit_valid		: 1,
				mem_attr_valid		: 1,
				reserved		: 60;
	} pal_tr_valid_s;
} pal_tr_valid_u_t;

/* Read a translation register */
static inline s64
ia64_pal_tr_read (u64 reg_num, u64 