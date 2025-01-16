D_PM		.pfm_context =		NULL,     \
				.pfm_needs_checking =	0UL,
#else
# define INIT_THREAD_PM
#endif
	unsigned long dbr[IA64_NUM_DBG_REGS];
	unsigned long ibr[IA64_NUM_DBG_REGS];
	struct ia64_fpreg fph[96];	/* saved/loaded on demand */
};

#define INIT_THREAD {						\
	.flags =	0,					\
	.on_ustack =	0,					\
	.ksp =		0,					\
	.map_base =	