 {
	__u64 val;
	struct {
		__u64 rv3  :  2; /* 0-1 */
		__u64 ps   :  6; /* 2-7 */
		__u64 key  : 24; /* 8-31 */
		__u64 rv4  : 32; /* 32-63 */
	};
};

union  ia64_rr {
	__u64 val;
	struct {
		__u64  ve	:  1;  /* enable hw walker */
		__u64  reserved0:  1;  /* reserved */
		__u64  ps	:  6;  /* log page size */
		__u64  rid	: 24;  /* region id */
		__u64  reserved1: 32;  /* reserved */
	};
};

/*
 * CPU type, hardware bug flags, and per-CPU state.  Frequently used
 * state comes earlier:
 */
struct cpuinfo_ia64 {
	unsigned int softirq_pending;
	unsigned long itm_delta;	/* # of clock cycles between clock ticks */
	unsigned long itm_next;		/* interval timer mask value to use for next clock tick */
	unsigned long nsec_per_cyc;	/* (1000000000<<IA64_NSEC_PER_CYC_SHIFT)/itc_freq */
	unsigned long unimpl_va_mask;	/* mask of unimplemented virtual address bits (from PAL) */
	unsigned long unimpl_pa_mask;	/* mask of unimplemented physical address bits (from PAL) */
	unsigned long itc_freq;		/* frequency of ITC counter */
	unsigned long proc_freq;	/* frequency of processor */
	unsigned long cyc_per_usec;	/* itc_freq/1000000 */
	unsigned long ptce_base;
	unsigned int ptce_count[2];
	unsigned int ptce_stride[2];
	struct task_struct *ksoftirqd;	/* kernel softirq daemon for this CPU */

#ifdef CONFIG_SMP
	unsigned long loops_per_jiffy;
	int cpu;
	unsigned int socket_id;	/* physical processor socket id */
	unsigned short core_id;	/* core id */
	unsigned short thread_id; /* thread id */
	unsigned short num_log;	/* Total number of logical processors on
				 * this socket that were successfully booted */
	unsigned char cores_per_socket;	/* Cores per processor socket */
	unsigned char threads_per_core;	/* Threads per core */
#endif

	/* CPUID-derived information: */
	unsigned long ppn;
	unsigned long features;
	unsigned char number;
	unsigned char revision;
	unsigned char model;
	unsigned char family;
	unsigned char archrev;
	char vendor[16];
	char *model_name;

#ifdef CONFIG_NUMA
	struct ia64_node_data *node_data;
#endif
};

DECLARE_PER_CPU(struct cpuinfo_ia64, ia64_cpu_info);

/*
 * The "local" data variable.  It refers to the per-CPU data of the currently executing
 * CPU, much like "current" points to the per-task data of the currently executing task.
 * Do not use the address of local_cpu_data, since it will be different from
 * cpu_data(smp_pr