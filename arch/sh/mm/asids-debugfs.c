ible.
 */
typedef struct ia64_ptce_info_s {
	unsigned long	base;
	u32		count[2];
	u32		stride[2];
} ia64_ptce_info_t;

/* Return the information required for the architected loop used to purge
 * (initialize) the entire TC
 */
static inline s64
ia64_get_ptce (ia64_ptce_info_t *ptce)
{
	struct ia64_pal_retval iprv;

	if (!ptce)
		return -1;

	PAL_CALL(iprv, PAL_PTCE_INFO, 0, 0, 0);
	if (iprv.status == 0) {
		ptce->base = iprv.v0;
		ptce->count[0] = iprv.v1 >> 32;
		ptce->count[1] = iprv.v1 & 0xffffffff;
		ptce->stride[0] = iprv.v2 >> 32;
		ptce->stride[1] = iprv.v2 & 0xffffffff;
	}
	return iprv.status;
}

/* Return info about implemented application and control registers. */
static inline s64
ia64_pal_register_info (u64 info_request, u64 *reg_info_1, u64 *reg_info_2)
{
	struct ia64_pal_retval iprv;
	PAL_CALL(iprv, PAL_REGISTER_INFO, info_request, 0, 0);
	if (reg_info_1)
		*reg_info_1 = iprv.v0;
	if (reg_info_2)
		*reg_info_2 = iprv.v1;
	return iprv.status;
}

typedef union pal_hints_u {
	unsigned long		ph_data;
	struct {
	       unsigned long	si		: 1,
				li		: 1,
				reserved	: 62;
	} pal_hints_s;
} pal_hints_u_t;

/* Return information about the register stack and RSE for this processor
 * implementation.
 */
static inline long ia64_pal_rse_info(unsigned long *num_phys_stacked,
							pal_hints_u_t *hints)
{
	struct ia64_pal_retval iprv;
	PAL_CALL(iprv, PAL_RSE_INFO, 0, 0, 0);
	if (num_phys_stacked)
		*num_phys_stacked = iprv.v0;
	if (hints)
		hints->ph_data = iprv.v1;
	return iprv.status;
}

/*
 * Set the current hardware resource sharing policy of the processor
 */
static inline s64
ia64_pal_set_hw_policy (u64 policy)
{
	struct ia64_pal_retval iprv;
	PAL_CALL(iprv, PAL_SET_HW_POLICY, policy, 0, 0);
	return iprv.status;
}

/* Cause the processor to enter	SHUTDOWN state, where prefetching and execution are
 * sus