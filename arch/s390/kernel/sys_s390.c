 (ia64_intri_res)	\
		      : "r" (ptr), "r" (x) : "memory");			\
	ia64_intri_res;							\
})

#define ia64_cmpxchg1_acq(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg1.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg1_rel(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg1.rel %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg2_acq(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg2.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg2_rel(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
											\
	asm volatile ("cmpxchg2.rel %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg4_acq(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg4.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg4_rel(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg4.rel %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg8_acq(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg8.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg8_rel(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
											\
	asm volatile ("cmpxchg8.rel %0=[%1],%2,ar.ccv":					\
			     