bits.num_log
#define overview_tpc		overview.overview_bits.tpc
#define overview_cpp		overview.overview_bits.cpp
#define overview_ppid		overview.overview_bits.ppid
#define log1_tid		ppli1.ppli1_bits.tid
#define log1_cid		ppli1.ppli1_bits.cid
#define log2_la			ppli2.ppli2_bits.la

/* Get information on logical to physical processor mappings. */
static inline s64
ia64_pal_logical_to_phys(u64 proc_number, pal_logical_to_physical_t *mapping)
{
	struct ia64_pal_retval iprv;

	PAL_CALL(iprv, PAL_LOGICAL_TO_PHYSICAL, proc_number, 0, 0);

	if (iprv.status == PAL_STATUS_SUCCESS)
	{
		mapping->overview.overview_data = iprv.v0;
		mapping->ppli1.ppli1_data = iprv.v1;
		mapping->ppli2.ppli2_data = iprv.v2;
	}

	return iprv.status;
}

typedef struct pal_cache_shared_info_s
{
	u64 num_shared;
	pal_proc_n_log_info1_t ppli1;
	pal_proc_n_log_info2_t ppli2;
} pal_cache_shared_info_t;

/* Get information on logical to physical processor mappings. */
static inline s64
ia64_pal_cache_shared_info(u64 level,
		u64 type,
		u64 proc_number,
		pal_cache_shared_info_t *info)
{
	struct ia64_pal_retval iprv;

	PAL_CALL(iprv, PAL_CACHE_SHARED_INFO, level, type, proc_number);

	if (iprv.status == PAL_STATUS_SUCCESS) {
		info->num_shared = iprv.v0;
		info->ppli1.ppli1_data = iprv.v1;
		info->ppli2.ppli2_data = iprv.v2;
	}

	return iprv.status;
}
#endif /* __ASSEMBLY__ */

#endif /* _ASM_IA64_PAL_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
 * Fundamental kernel parameters.
 *
 * Based on <asm-i386/param.h>.
 *
 * Modified 1998, 1999, 2002-2003
 *	David Mosberger-Tang <davidm@hpl.hp.com>, Hewlett-Packard Co
 */
#ifndef _ASM_IA64_PARAM_H
#define _ASM_IA64_PARAM_H

#include <uapi/asm/param.h>

# define HZ		CONFIG_HZ
# define USER_HZ	HZ
# define CLOCKS_PER_SEC	HZ	/* frequency at which times() counts */
#endif /* _ASM_IA64_PARAM_H */
                                                                                                                                                                                                                                                              