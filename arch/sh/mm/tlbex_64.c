, 0, 0, 0, 0, 0, 0, 0);
	return isrv.status;
}

/*
 * Clear the processor and platform information logged by SAL with respect to the machine
 * state at the time of MCA's, INITs, CMCs, or CPEs.
 */
static inline s64
ia64_sal_clear_state_info (u64 sal_info_type)
{
	struct ia64_sal_retval isrv;
	SAL_CALL_REENTRANT(isrv, SAL_CLEAR_STATE_INFO, sal_info_type, 0,
	              0, 0, 0, 0, 0);
	return isrv.status;
}


/* Get the processor and platform information logged by SAL with respect to the machine
 * state at the time of the MCAs, INITs, CMCs, or CPEs.
 */
static inline u64
ia64_sal_get_state_info (u64 sal_info_type, u64 *sal_info)
{
	struct ia64_sal_retval isrv;
	SAL_CALL_REENTRANT(isrv, SAL_GET_STATE_INFO, sal_info_type, 0,
	              sal_info, 0, 0, 0, 0);
	if (isrv.status)
		return 0;

	return isrv.v0;
}

/*
 * Get the maximum size of the information logged by SAL with respect to the machine state
 * at the time of MCAs, INITs, CMCs, or CPEs.
 */
static inline u64
ia64_sal_get_state_info_size (u64 sal_info_type)
{
	struct ia64_sal_retval isrv;
	SAL_CALL_REENTRANT(isrv, SAL_GET_STATE_INFO_SIZE, sal_info_type, 0,
	              0, 0, 0, 0, 0);
	if (isrv.status)
		return 0;
	return isrv.v0;
}

/*
 * Causes the processor to go into a spin loop within SAL where SAL awaits a wakeup from
 * the monarch processor.  Must not lock, because it will not return on any cpu until the
 * monarch processor sends a wake up.
 */
static inline s64
ia64_sal_mc_rendez (void)
{
	struct ia64_sal_retval isrv;
	SAL_CALL_NOLOCK(isrv, SAL_MC_RENDEZ, 0, 0, 0, 0, 0, 0, 0);
	return isrv.status;
}

/*
 * Allow the OS to specify the interrupt number to be used by SAL to interrupt OS during
 * the machine check rendezvous sequence as well as the mechanism to wake up the
 * non-monarch processor at the end of machine check processing.
 * Returns the complete ia64_sal_retval because some calls return more than just a status
 * value.
 */
static inline struct ia64_sal_retval
ia64_sal_mc_set_params (u64 param_type, u64 i_or_m, u64 i_or_m_val, u64 timeout, u64 rz_always)
{
	struct ia64_sal_retval isrv;
	SAL_CALL(isrv, SAL_MC_SET_PARAMS, param_type, i_or_m, i_or_m_val,
		 timeout, rz_always, 0, 0);
	return isrv;
}

/* Read from PCI configuration space */
static inline s64
ia64_sal_pci_config_read (u64 pci_config_addr, int type, u64 size, u64 *value)
{
	struct ia64_sal_retval isrv;
	SAL_CALL(isrv, SAL_PCI_CONFIG_READ, pci_config_addr, size, type, 0, 0, 0, 0);
	if (value)
		*value = isrv.v0;
	return isrv.status;
}

/* Write to PCI configuration space */
static inline s64
ia64_sal_pci_config_write (u64 pci_config_addr, int type, u64 size, u64 value)
{
	struct ia64_sal_retval isrv;
	SAL_CALL(isrv, SAL_PCI_CONFIG_WRITE, pci_config_addr, size, value,
	         type, 0, 0, 0);
	return isrv.status;
}

/*
 * Register physical addresses of locations needed by SAL when SAL procedures are invoked
 * in virtual mode.
 */
static inline s64
ia64_sal_register_physical_addr (u64 phys_entry, u64 phys_addr)
{
	struct ia64_sal_retval isrv;
	SAL_CALL(isrv, SAL_REGISTER_PHYSICAL_ADDR, phys_entry, phys_addr,
	         0, 0, 0, 0, 0);
	return isrv.status;
}

/*
 * Register software dependent code locations within SAL. These locations are handlers or
 * entry points where SAL will pass control for the specified event. These event handlers
 * are for the bott rendezvous, MCAs and INIT scenarios.
 */
static inline s64
ia64_sal_set_vectors (u64 vector_type,
		      u64 handler_addr1, u64 gp1, u64 handler_len1,
		      u64 handler_addr2, u64 gp2, u64 handler_len2)
{
	struct ia64_sal_retval isrv;
	SAL_CALL(isrv, SAL_SET_VECTORS, vector_type,
			handler_addr1, gp1, handler_len1,
			handler_addr2, gp2, handler_len2);

	return isrv.status;
}

/* Update the contents of PAL block in the non-volatile storage device */
static inline s64
ia64_sal_update_pal (u64 param_buf, u64 scratch_buf, u64 scratch_buf_size,
		     u64 *error_code, u64 *scratch_buf_size_needed)
{
	struct ia64_sal_retval isrv;
	SAL_CALL(isrv, SAL_UPDATE_PAL, param_buf, scratch_buf, scratch_buf_size,
	         0, 0, 0, 0);
	if (error_code)
		*error_code = isrv.v0;
	if (scratch_buf_size_needed)
		*scratch_buf_size_needed = isrv.v1;
	return isrv.status;
}

/* Get physical processor die mapping in the platform. */
static inline s64
ia64_sal_physical_id_info(u16 *splid)
{
	struct ia64_sal_retval isrv;

	if (sal_revision < SAL_VERSION_CODE(3,2))
		return -1;

	SAL_CALL(isrv, SAL_PHYSICAL_ID_INFO, 0, 0, 0, 0, 0, 0, 0);
	if (splid)
		*splid = isrv.v0;
	return isrv.status;
}

extern unsigned long sal_platform_features;

extern int (*salinfo_platform_oemdata)(const u8 *, u8 **, u64 *);

struct sal_ret_values {
	long r8; long r9; long r10; long r11;
};

#define IA64_SAL_OEMFUNC_MIN		0x02000000
#define IA64_SAL_OEMFUNC_MAX		0x03ffffff

extern int ia64_sal_oe