che_protection_info;

extern pal_cache_config_info_t		pal_cache_config_info_get(pal_cache_level_t,
								  pal_cache_type_t);

extern pal_cache_protection_info_t	pal_cache_protection_info_get(pal_cache_level_t,
								      pal_cache_type_t);


extern void				pal_error(int);


/* Useful wrappers for the current list of pal procedures */

typedef union pal_bus_features_u {
	u64	pal_bus_features_val;
	struct {
		u64	pbf_reserved1				:	29;
		u64	pbf_req_bus_parking			:	1;
		u64	pbf_bus_lock_mask			:	1;
		u64	pbf_enable_half_xfer_rate		:	1;
		u64	pbf_reserved2				:	20;
		u64	pbf_enable_shared_line_replace		:	1;
		u64	pbf_enable_exclusive_line_replace	:	1;
		u64	pbf_disable_xaction_queueing		:	1;
		u64	pbf_disable_resp_err_check		:	1;
		u64	pbf_disable_berr_check			:	1;
		u64	pbf_disable_bus_req_internal_err_signal	:	1;
		u64	pbf_disable_bus_req_berr_signal		:	1;
		u64	pbf_disable_bus_init_event_check	:	1;
		u64	pbf_disable_bus_init_event_signal	:	1;
		u64	pbf_disable_bus_addr_err_check		:	1;
		u64	pbf_disable_bus_addr_err_signal		:	1;
		u64	pbf_disable_bus_data_err_check		:	1;
	} pal_bus_features_s;
} pal_bus_features_u_t;

extern void pal_bus_features_print (u64);

/* Provide information about configurable processor bus features */
static inline s64
ia64_pal_bus_get_features (pal_bus_features_u_t *features_avail,
			   pal_bus_features_u_t *features_status,
			   pal_bus_features_u_t *features_control)
{
	struct ia64_pal_retval iprv;
	PAL_CALL_PHYS(iprv, PAL_BUS_GET_FEATURES, 0, 0, 0);
	if (features_avail)
		features_avail->pal_bus_features_val = iprv.v0;
	if (features_status)
		features_status->pal_bus_features_val = iprv.v1;
	if (features_control)
		features_control->pal_bus_features_val = iprv.v2;
	return iprv.status;
}

/* Enables/disables specific processor bus features */
static inline s64
ia64_pal_bus_set_features (pal_bus_features_u_t feature_select)
{
	struct ia64_pal_retval iprv;
	PAL_CALL_PHYS(iprv, PAL_BUS_SET_FEATURES, feature_select.pal_bus_features_val, 0, 0);
	return iprv.status;
}

/* Get detailed cache information */
static inline s64
ia64_pal_cache_config_info (u64 cache_level, u64 cache_type, pal_cache_config_info_t *conf)
{
	struct ia64_pal_retval iprv;

	PAL_CALL(iprv, PAL_CACHE_INFO, cache_level, cache_type, 0);

	if (iprv.status == 0) {
		conf->pcci_status                 = iprv.status;
		conf->pcci_info_1.pcci1_data      = iprv.v0;
		conf->pcci_info_2.pcci2_data      = iprv.v1;
		conf->pcci_reserved               = iprv.v2;
	}
	return iprv.status;

}

/* Get detailed cche protection information */
static inline s64
ia64_pal_cache_prot_info (u64 cache_level, u64 cache_type, pal_cache_protection_info_t *prot)
{
	struct ia64_pal_retval iprv;

	PAL_CALL(iprv, PAL_CACHE_PROT_INFO, cache_level, cache_type, 0);

	if (iprv.status == 0) {
		prot->pcpi_status           = iprv.status;
		prot->pcp_info[0].pcpi_data = iprv.v0 & 0xffffffff;
		prot->pcp_info[1].pcpi_data = iprv.v0 >> 32;
		prot->pcp_info[2].pcpi_data = iprv.v1 & 0xffffffff;
		prot->pcp_info[3].pcpi_data = iprv.v1 >> 32;
		prot->pcp_info[4].pcpi_data = iprv.v2 & 0xffffffff;
		prot->pcp_info[5].pcpi_data = iprv.v2 >> 32;
	}
	return iprv.status;
}

/*
 * Flush the processor instruction or data caches.  *PROGRESS must be
 * initialized to zero before calling this for the first time..
 */
static inline s64
ia64_pal_cache_flush (u64 cache_type, u64 invalidate, u64 *progress, u64 *vector)
{
	struct ia64_pal_retval iprv;
	PAL_CALL(iprv, PAL_CACHE_FLUSH, cache_type, invalidate, *progress);
	if (vector)
		*vector = iprv.v0;
	*progress = iprv.v1;
	return iprv.status;
}


/