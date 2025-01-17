ex), "r"(val) : "memory")

#define ia64_set_pkr(index, val)						\
	asm volatile ("mov pkr[%0]=%1" :: "r"(index), "r"(val) : "memory")

#define ia64_set_pmc(index, val)						\
	asm volatile ("mov pmc[%0]=%1" :: "r"(index), "r"(val) : "memory")

#define ia64_set_pmd(index, val)						\
	asm volatile ("mov pmd[%0]=%1" :: "r"(index), "r"(val) : "memory")

#define ia64_native_set_rr(index, val)							\
	asm volatile ("mov rr[%0]=%1" :: "r"(index), "r"(val) : "memory");

#define ia64_native_get_cpuid(index)							\
({											\
	unsigned long ia64_intri_res;							\
	asm volatile ("mov %0=cpuid[%r1]" : "=r"(ia64_intri_res) : "rO"(index));	\
	ia64_intri_res;									\
})

#define __ia64_get_dbr(index)							\
({										\
	unsigned long ia64_intri_res;						\
	asm volatile ("mov %0=dbr[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})

#define ia64_get_ibr(index)							\
({										\
	unsigned long ia64_intri_res;						\
	asm volatile ("mov %0=ibr[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})

#define ia64_get_pkr(index)							\
({										\
	unsigned long ia64_intri_res;						\
	asm volatile ("mov %0=pkr[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})

#define ia64_get_pmc(index)							\
({										\
	unsigned long ia64_intri_res;						\
	asm volatile ("mov %0=pmc[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})


#define ia64_native_get_pmd(index)						\
({										\
	unsigned long ia64_intri_res;						\
	asm volatile ("mov %0=pmd[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})

#define ia64_native_get_rr(index)						\
({										\
	unsigned long ia64_intri_res;						\
	asm volatile ("mov %0=rr[%1]" : "=r"(ia64_intri_res) : "r" (index));	\
	ia64_intri_res;								\
})

#define ia64_native_fc(addr)	asm volatile ("fc %0" :: "r"(addr) : "memory")


#define ia64_sync_i()	asm volatile (";; sync.i" ::: "memory")

#define ia64_native_ssm(mask)	asm volatile ("ssm %0":: "i"((mask)) : "memory")
#define ia64_native_rsm(mask)	asm volatile ("rsm %0":: "i"((mask)) : "memory")
#define ia64_sum(mask)	asm volatile ("sum %0":: "i"((mask)) : "memory")
#define ia64_rum(mask)	asm volatile ("rum %0":: "i"((mask)) : "memory")

#define ia64_ptce(addr)	asm volatile ("ptc.e %0" :: "r"(addr))

#define ia64_native_ptcga(addr, size)						\
do {										\
	asm volatile ("ptc.ga %0,%1" :: "r"(addr), "r"(size) : "memory");	\
	ia64_dv_serialize_data();						\
} while (0)

#define ia64_ptcl(addr, size)							\
do {										\
	asm volatile ("ptc.l %0,%1" :: "r"(addr), "r"(size) : "memory");	\
	ia64_dv_serialize_data();						\
} while (0)

#define ia64_ptri(addr, size)						\
	asm volatile ("ptr.i %0,%1" :: "r"(addr), "r"(size) : "memory")

#define ia64_ptrd(addr, size)						\
	asm volatile ("ptr.d %0,%1" :: "r"(addr), "r"(size) : "memory")

#define ia64_ttag(addr)							\
({									  \
	__u64 ia64_intri_res;						   \
	asm volatile ("ttag %0=%1" : "=r"(ia64_intri_res) : "r" (addr));   \
	ia64_intri_res;							 \
})


/* Values for lfhint in ia64_lfetch and ia64_lfetch_fault */

#define ia64_lfhint_none   0
#define ia64_lfhint_nt1    1
#define ia64_lfhint_nt2    2
#define ia64_lfhint_nta    3

#define ia64_lfetch(lfhint, y)					\
({								\
        switch (lfhint) {					\
        case ia64_lfhint_none:					\
                asm volatile ("lfetch [%0]" : : "r"(y));	\
                break;						\
        case ia64_lfhint_nt1:					\
                asm volatile ("lfetch.nt1 [%0]" : : "r"(y));	\
                break;						\
        case ia64_lfhint_nt2:					\
                asm volatile ("lfetch.nt2 [%0]" : : "r"(y));	\
                break;						\
        case ia64_lfhint_nta:					\
                asm volatile ("lfetch.nta [%0]" : : "r"(y));	\
                break;						\
        }							\
})

#define ia64_lfetch_excl(lfhint, y)					\
({									\
        switch (lfhint) {						\
        case ia64_lfhint_none:						\
                asm volatile ("lfetch.excl [%0]" :: "r"(y));		\
                break;							\
        case ia64_lfhint_nt1:						\
                asm volatile ("lfetch.excl.nt1 [%0]" :: "r"(y));	\
                break;							\
        case ia64_lfhint_nt2:						\
                asm volatile ("lfetch.excl.nt2 [%0]" :: "r"(y));	\
                break;							\
        case ia64_lfhint_nta:						\
                asm volatile ("lfetch.excl.nta [%0]" :: "r"(y));	\
                break;							\
        }								\
})

#define ia64_lfetch_fault(lfhint, y)					\
({									\
        switch (lfhint) {						\
        case ia64_lfhint_none:						\
                asm volatile ("lfetch.fault [%0]" : : "r"(y));		\
                break;							\
        case ia64_lfhint_nt1:						\
                asm volatile ("lfetch.fault.nt1 [%0]" : : "r"(y));	\
                break;							\
        case ia64_lfhint_nt2:						\
                asm volatile ("lfetch.fault.nt2 [%0]" : : "r"(y));	\
                break;							\
        case ia64_lfhint_nta:						\
                asm volatile ("lfetch.fault.nta [%0]" : : "r"(y));	\
                break;							\
        }								\
})

#define ia64_lfetch_fault_excl(lfhint, y)				\
({									\
        switch (lfhint) {						\
        case ia64_lfhint_none:						\
                asm volatile ("lfetch.fault.excl [%0]" :: "r"(y));	\
                break;							\
        case ia64_lfhint_nt1:						\
                asm volatile ("lfetch.fault.excl.nt1 [%0]" :: "r"(y));	\
                break;							\
        case ia64_lfhint_nt2:						\
                asm volatile ("lfetch.fault.excl.nt2 [%0]" :: "r"(y));	\
                break;							\
        case ia64_lfhint_nta:						\
                asm volatile ("lfetch.fault.excl.nta [%0]" :: "r"(y));	\
                break;							\
        }								\
})

#define ia64_native_intrin_local_irq_restore(x)			\
do {								\
	asm volatile (";;   cmp.ne p6,p7=%0,r0;;"		\
		      "(p6) ssm psr.i;"				\
		      "(p7) rsm psr.i;;"			\
		      "(p6) srlz.d"				\
		      :: "r"((x)) : "p6", "p7", "memory");	\
} while (0)

#endif /* _UAPI_ASM_IA64_GCC_INTRIN_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * Copyright (C) 2002,2003 Intel Corp.
 *      Jun Nakajima <jun.nakajima@intel.com>
 *      Suresh Siddha <suresh.b.siddha@intel.com>
 */

#ifndef _ASM_IA64_IA64REGS_H
#define _ASM_IA64_IA64REGS_H

/*
 * Register Names for getreg() and setreg().
 *
 * The "magic" numbers happen to match the values used by the Intel compiler's
 * getreg()/setreg() intrinsics.
 */

/* Special Registers */

#define _IA64_REG_IP		1016	/* getreg only */
#define _IA64_REG_PSR		1019
#define _IA64_REG_PSR_L		1019

/* General Integer Registers */

#define _IA64_REG_GP		1025	/* R1 */
#define _IA64_REG_R8		1032	/* R8 */
#define _IA64_REG_R9		1033	/* R9 */
#define _IA64_REG_SP		1036	/* R12 */
#define _IA64_REG_TP		1037	/* R13 */

/* Application Registers */

#define _IA64_REG_AR_KR0	3072
#define _IA64_REG_AR_KR1	3073
#define _IA64_REG_AR_KR2	3074
#define _IA64_REG_AR_KR3	3075
#define _IA64_REG_AR_KR4	3076
#define _IA64_REG_AR_KR5	3077
#define _IA64_REG_AR_KR6	3078
#define _IA64_REG_AR_KR7	3079
#define _IA64_REG_AR_RSC	3088
#define _IA64_REG_AR_BSP	3089
#define _IA64_REG_AR_BSPSTORE	3090
#define _IA64_REG_AR_RNAT	3091
#define _IA64_REG_AR_FCR	3093
#define _IA64_REG_AR_EFLAG	3096
#define _IA64_REG_AR_CSD	3097
#define _IA64_REG_AR_SSD	3098
#define _IA64_REG_AR_CFLAG	3099
#define _IA64_REG_AR_FSR	3100
#define _IA64_REG_AR_FIR	3101
#define _IA64_REG_AR_FDR	3102
#define _IA64_REG_AR_CCV	3104
#define _IA64_REG_AR_UNAT	3108
#define _IA64_REG_AR_FPSR	3112
#define _IA64_REG_AR_ITC	3116
#define _IA64_REG_AR_PFS	3136
#define _IA64_REG_AR_LC		3137
#define _IA64_REG_AR_EC		3138

/* Control Registers */

#define _IA64_REG_CR_DCR	4096
#define _IA64_REG_CR_ITM	4097
#define _IA64_REG_CR_IVA	4098
#define _IA64_REG_CR_PTA	4104
#define _IA64_REG_CR_IPSR	4112
#define _IA64_REG_CR_ISR	4113
#define _IA64_REG_CR_IIP	4115
#define _IA64_REG_CR_IFA	4116
#define _IA64_REG_CR_ITIR	4117
#define _IA64_REG_CR_IIPA	4118
#define _IA64_REG_CR_IFS	4119
#define _IA64_REG_CR_IIM	4120
#define _IA64_REG_CR_IHA	4121
#define _IA64_REG_CR_LID	4160
#define _IA64_REG_CR_IVR	4161	/* getreg only */
#define _IA64_REG_CR_TPR	4162
#define _IA64_REG_CR_EOI	4163
#define _IA64_REG_CR_IRR0	4164	/* getreg only */
#define _IA64_REG_CR_IRR1	4165	/* getreg only */
#define _IA64_REG_CR_IRR2	4166	/* getreg only */
#define _IA64_REG_CR_IRR3	4167	/* getreg only */
#define _IA64_REG_CR_ITV	4168
#define _IA64_REG_CR_PMV	4169
#define _IA64_REG_CR_CMCV	4170
#define _IA64_REG_CR_LRR0	4176
#define _IA64_REG_CR_LRR1	4177
