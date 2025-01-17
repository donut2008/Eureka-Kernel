uration
	dep.z loc2=loc2,0,61		// convert pal entry point to physical
	tpa r8=r8			// convert rp to physical
	;;
	mov b7 = loc2			// install target to branch reg
	mov ar.rsc=0			// put RSE in enforced lazy, LE mode
	movl r16=PAL_PSR_BITS_TO_CLEAR
	movl r17=PAL_PSR_BITS_TO_SET
	;;
	or loc3=loc3,r17		// add in psr the bits to set
	;;
	andcm r16=loc3,r16		// removes bits to clear from psr
	br.call.sptk.many rp=ia64_switch_mode_phys
	mov rp = r8			// install return address (physical)
	mov loc5 = r19
	mov loc6 = r20
	br.cond.sptk.many b7
1:
	mov ar.rsc=0			// put RSE in enforced lazy, LE mode
	mov r16=loc3			// r16= original psr
	mov r19=loc5
	mov r20=loc6
	br.call.sptk.many rp=ia64_switch_mode_virt // return to virtual mode
	mov psr.l = loc3		// restore init PSR

	mov ar.pfs = loc1
	mov rp = loc0
	;;
	mov ar.rsc=loc4			// restore RSE configuration
	srlz.d				// seralize restoration of psr.l
	br.ret.sptk.many b0
END(ia64_pal_call_phys_static)

/*
 * Make a PAL call using the stacked registers in physical mode.
 *
 * Inputs:
 *	in0         Index of PAL service
 *	in2 - in3   Remaining PAL arguments
 */
GLOBAL_ENTRY(ia64_pal_call_phys_stacked)
	.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(5)
	alloc	loc1 = ar.pfs,5,7,4,0
	movl	loc2 = pal_entry_point
1:	{
	  mov r28  = in0		// copy procedure index
	  mov loc0 = rp			// save rp
	}
	.body
	;;
	ld8 loc2 = [loc2]		// loc2 <- entry point
	mov loc3 = psr			// save psr
	;;
	mov loc4=ar.rsc			// save RSE configuration
	dep.z loc2=loc2,0,61		// convert pal entry point to physical
	;;
	mov ar.rsc=0			// put RSE in enforced lazy, LE mode
	movl r16=PAL_PSR_BITS_TO_CLEAR
	movl r17=PAL_PSR_BITS_TO_SET
	;;
	or loc3=loc3,r17		// add in psr the bits to set
	mov b7 = loc2			// install target to branch reg
	;;
	andcm r16=loc3,r16		// removes bits to clear from psr
	br.call.sptk.many rp=ia64_switch_mode_phys

	mov out0 = in0			// first argument
	mov out1 = in1			// copy arg2
	mov out2 = in2			// copy arg3
	mov out3 = in3			// copy arg3
	mov loc5 = r19
	mov loc6 = r20

	br.call.sptk.many rp=b7		// now make the call

	mov ar.rsc=0			// put RSE in enforced lazy, LE mode
	mov r16=loc3			// r16= original psr
	mov r19=loc5
	mov r20=loc6
	br.call.sptk.many rp=ia64_switch_mode_virt // return to virtual mode

	mov psr.l  = loc3		// restore init PSR
	mov ar.pfs = loc1
	mov rp = loc0
	;;
	mov ar.rsc=loc4			// restore RSE configuration
	srlz.d				// seralize restoration of psr.l
	br.ret.sptk.many b0
END(ia64_pal_call_phys_stacked)

/*
 * Save scratch fp scratch regs which aren't saved in pt_regs already
 * (fp10-fp15).
 *
 * NOTE: We need to do this since firmware (SAL and PAL) may use any of the
 * scratch regs fp-low partition.
 *
 * Inputs:
 *      in0	Address of stack storage for fp regs
 */
GLOBAL_ENTRY(ia64_save_scratch_fpregs)
	alloc r3=ar.pfs,1,0,0,0
	add r2=16,in0
	;;
	stf.spill [in0] = f10,32
	stf.spill [r2]  = f11,32
	;;
	stf.spill [in0] = f12,32
	stf.spill [r2]  = f13,32
	;;
	stf.spill [in0] = f14,32
	stf.spill [r2]  = f15,32
	br.ret.sptk.many rp
END(ia64_save_scratch_fpregs)

/*
 * Load scratch fp scratch regs (fp10-fp15)
 *
 * Inputs:
 *      in0	Address of stack storage for fp regs
 */
GLOBAL_ENTRY(ia64_load_scratch_fpregs)
	alloc r3=ar.pfs,1,0,0,0
	add r2=16,in0
	;;
	ldf.fill  f10 = [in0],32
	ldf.fill  f11 = [r2],32
	;;
	ldf.fill  f12 = [in0],32
	ldf.fill  f13 = [r2],32
	;;
	ldf.fill  f14 = [in0],32
	ldf.fill  f15 = [r2],32
	br.ret.sptk.many rp
END(ia64_load_scratch_fpregs)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * palinfo.c
 *
 * Prints processor specific information reported by PAL.
 * This code is based on specification of PAL as of the
 * Intel IA-64 Architecture Software Developer's Manual v1.0.
 *
 *
 * Copyright (C) 2000-2001, 2003 Hewlett-Packard Co
 *	Stephane Eranian <eranian@hpl.hp.com>
 * Copyright (C) 2004 Intel Corporation
 *  Ashok Raj <ashok.raj@intel.com>
 *
 * 05/26/2000	S.Eranian	initial release
 * 08/21/2000	S.Eranian	updated to July 2000 PAL specs
 * 02/05/2001   S.Eranian	fixed module support
 * 10/23/2001	S.Eranian	updated pal_perf_mon_info bug fixes
 * 03/24/2004	Ashok Raj	updated to work with CPU Hotplug
 * 10/26/2006   Russ Anderson	updated processor features to rev 2.2 spec
 */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/efi.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>

#include <asm/pal.h>
#include <asm/sal.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <linux/smp.h>

MODULE_AUTHOR("Stephane Eranian <eranian@hpl.hp.com>");
MODULE_DESCRIPTION("/proc interface to IA-64 PAL");
MODULE_LICENSE("GPL");

#define PALINFO_VERSION "0.5"

typedef int (*palinfo_func_t)(struct seq_file *);

typedef struct {
	const char		*name;		/* name of the proc entry */
	palinfo_func_t		proc_read;	/* function to call for reading */
	struct proc_dir_entry	*entry;		/* registered entry (removal) */
} palinfo_entry_t;


/*
 *  A bunch of string array to get pretty printing
 */

static const char *cache_types[] = {
	"",			/* not used */
	"Instruction",
	"Data",
	"Data/Instruction"	/* unified */
};

static const char *cache_mattrib[]={
	"WriteThrough",
	"WriteBack",
	"",		/* reserved */
	""		/* reserved */
};

static const char *cache_st_hints[]={
	"Temporal, level 1",
	"Reserved",
	"Reserved",
	"Non-temporal, all levels",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

static const char *cache_ld_hints[]={
	"Temporal, level 1",
	"Non-temporal, level 1",
	"Reserved",
	"Non-temporal, all levels",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

static const char *rse_hints[]={
	"enforced lazy",
	"eager stores",
	"eager loads",
	"eager loads and stores"
};

#define RSE_HINTS_COUNT ARRAY_SIZE(rse_hints)

static const char *mem_attrib[]={
	"WB",		/* 000 */
	"SW",		/* 001 */
	"010",		/* 010 */
	"011",		/* 011 */
	"UC",		/* 100 */
	"UCE",		/* 101 */
	"WC",		/* 110 */
	"NaTPage"	/* 111 */
};

/*
 * Take a 64bit vector and produces a string such that
 * if bit n is set then 2^n in clear text is generated. The adjustment
 * to the right unit is also done.
 *
 * Input:
 *	- a pointer to a buffer to hold the string
 *	- a 64-bit vector
 * Ouput:
 *	- a pointer to the end of the buffer
 *
 */
static void bitvector_process(struct seq_file *m, u64 vector)
{
	int i,j;
	static const char *units[]={ "", "K", "M", "G", "T" };

	for (i=0, j=0; i < 64; i++ , j=i/10) {
		if (vector & 0x1)
			seq_printf(m, "%d%s ", 1 << (i-j*10), units[j]);
		vector >>= 1;
	}
}

/*
 * Take a 64bit vector and produces a string such that
 * if bit n is set then register n is present. The function
 * takes into account consecutive registers and prints out ranges.
 *
 * Input:
 *	- a pointer to a buffer to hold the string
 *	- a 64-bit vector
 * Ouput:
 *	- a pointer to the end of the buffer
 *
 */
static void bitregister_process(struct seq_file *m, u64 *reg_info, int max)
{
	int i, begin, skip = 0;
	u64 value = reg_info[0];

	value >>= i = begin = ffs(value) - 1;

	for(; i < max; i++ ) {

		if (i != 0 && (i%64) == 0) value = *++reg_info;

		if ((value & 0x1) == 0 && skip == 0) {
			if (begin  <= i - 2)
				seq_printf(m, "%d-%d ", begin, i-1);
			else
				seq_printf(m, "%d ", i-1);
			skip  = 1;
			begin = -1;
		} else if ((value & 0x1) && skip == 1) {
			skip = 0;
			begin = i;
		}
		value >>=1;
	}
	if (begin > -1) {
		if (begin < 127)
			seq_printf(m, "%d-127", begin);
		else
			seq_puts(m, "127");
	}
}

static int power_info(struct seq_file *m)
{
	s64 status;
	u64 halt_info_buffer[8];
	pal_power_mgmt_info_u_t *halt_info =(pal_power_mgmt_info_u_t *)halt_info_buffer;
	int i;

	status = ia64_pal_halt_info(halt_info);
	if (status != 0) return 0;

	for (i=0; i < 8 ; i++ ) {
		if (halt_info[i].pal_power_mgmt_info_s.im == 1) {
			seq_printf(m,
				   "Power level %d:\n"
				   "\tentry_latency       : %d cycles\n"
				   "\texit_latency        : %d cycles\n"
				   "\tpower consumption   : %d mW\n"
				   "\tCache+TLB coherency : %s\n", i,
				   halt_info[i].pal_power_mgmt_info_s.entry_latency,
				   halt_info[i].pal_power_mgmt_info_s.exit_latency,
				   halt_info[i].pal_power_mgmt_info_s.power_consumption,
				   halt_info[i].pal_power_mgmt_info_s.co ? "Yes" : "No");
		} else {
			seq_printf(m,"Power level %d: not implemented\n", i);
		}
	}
	return 0;
}

static int cache_info(struct seq_file *m)
{
	unsigned long i, levels, unique_caches;
	pal_cache_config_info_t cci;
	int j, k;
	long status;

	if ((status = ia64_pal_cache_summary(&levels, &unique_caches)) != 0) {
		printk(KERN_ERR "ia64_pal_cache_summary=%ld\n", status);
		return 0;
	}

	seq_printf(m, "Cache levels  : %ld\nUnique caches : %ld\n\n",
		   levels, unique_caches);

	for (i=0; i < levels; i++) {
		for (j=2; j >0 ; j--) {
			/* even without unification some level may not be present */
			if ((status=ia64_pal_cache_config_info(i,j, &cci)) != 0)
				continue;

			seq_printf(m,
				   "%s Cache level %lu:\n"
				   "\tSize           : %u bytes\n"
				   "\tAttributes     : ",
				   cache_types[j+cci.pcci_unified], i+1,
				   cci.pcci_cache_size);

			if (cci.pcci_unified)
				seq_puts(m, "Unified ");

			seq_printf(m, "%s\n", cache_mattrib[cci.pcci_cache_attr]);

			seq_printf(m,
				   "\tAssociativity  : %d\n"
				   "\tLine size      : %d bytes\n"
				   "\tStride         : %d bytes\n",
				   cci.pcci_assoc,
				   1<<cci.pcci_line_size,
				   1<<cci.pcci_stride);
			if (j == 1)
				seq_puts(m, "\tStore latency  : N/A\n");
			else
				seq_printf(m, "\tStore latency  : %d cycle(s)\n",
					   cci.pcci_st_latency);

			seq_printf(m,
				   "\tLoad latency   : %d cycle(s)\n"
				   "\tStore hints    : ", cci.pcci_ld_latency);

			for(k=0; k < 8; k++ ) {
				if ( cci.pcci_st_hints & 0x1)
					seq_printf(m, "[%s]", cache_st_hints[k]);
				cci.pcci_st_hints >>=1;
			}
			seq_puts(m, "\n\tLoad hints     : ");

			for(k=0; k < 8; k++ ) {
				if (cci.pcci_ld_hints & 0x1)
					seq_printf(m, "[%s]", cache_ld_hints[k]);
				cci.pcci_ld_hints >>=1;
			}
			seq_printf(m,
				   "\n\tAlias boundary : %d byte(s)\n"
				   "\tTag LSB        : %d\n"
				   "\tTag MSB        : %d\n",
				   1<<cci.pcci_alias_boundary, cci.pcci_tag_lsb,
				   cci.pcci_tag_msb);

			/* when unified, data(j=2) is enough */
			if (cci.pcci_unified)
				break;
		}
	}
	return 0;
}


static int vm_info(struct seq_file *m)
{
	u64 tr_pages =0, vw_pages=0, tc_pages;
	u64 attrib;
	pal_vm_info_1_u_t vm_info_1;
	pal_vm_info_2_u_t vm_info_2;
	pal_tc_info_u_t	tc_info;
	ia64_ptce_info_t ptce;
	const char *sep;
	int i, j;
	long status;

	if ((status = ia64_pal_vm_summary(&vm_info_1, &vm_info_2)) !=0) {
		printk(KERN_ERR "ia64_pal_vm_summary=%ld\n", status);
	} else {

		seq_printf(m,
		     "Physical Address Space         : %d bits\n"
		     "Virtual Address Space          : %d bits\n"
		     "Protection Key Registers(PKR)  : %d\n"
		     "Implemented bits in PKR.key    : %d\n"
		     "Hash Tag ID                    : 0x%x\n"
		     "Size of RR.rid                 : %d\n"
		     "Max Purges                     : ",
		     vm_info_1.pal_vm_info_1_s.phys_add_size,
		     vm_info_2.pal_vm_info_2_s.impl_va_msb+1,
		     vm_info_1.pal_vm_info_1_s.max_pkr+1,
		     vm_info_1.pal_vm_info_1_s.key_size,
		     vm_info_1.pal_vm_info_1_s.hash_tag_id,
		     vm_info_2.pal_vm_info_2_s.rid_size);
		if (vm_info_2.pal_vm_info_2_s.max_purges == PAL_MAX_PURGES)
			seq_puts(m, "unlimited\n");
		else
			seq_printf(m, "%d\n",
		     		vm_info_2.pal_vm_info_2_s.max_purges ?
				vm_info_2.pal_vm_info_2_s.max_purges : 1);
	}

	if (ia64_pal_mem_attrib(&attrib) == 0) {
		seq_puts(m, "Supported memory attributes    : ");
		sep = "";
		for (i = 0; i < 8; i++) {
			if (attrib & (1 << i)) {
				seq_printf(m, "%s%s", sep, mem_attrib[i]);
				sep = ", ";
			}
		}
		seq_putc(m, '\n');
	}

	if ((status = ia64_pal_vm_page_size(&tr_pages, &vw_pages)) !=0) {
		printk(KERN_ERR "ia64_pal_vm_page_size=%ld\n", status);
	} else {

		seq_printf(m,
			   "\nTLB walker                     : %simplemented\n"
			   "Number of DTR                  : %d\n"
			   "Number of ITR                  : %d\n"
			   "TLB insertable page sizes      : ",
			   vm_info_1.pal_vm_info_1_s.vw ? "" : "not ",
			   vm_info_1.pal_vm_info_1_s.max_dtr_entry+1,
			   vm_info_1.pal_vm_info_1_s.max_itr_entry+1);

		bitvector_process(m, tr_pages);

		seq_puts(m, "\nTLB purgeable page sizes       : ");

		bitvector_process(m, vw_pages);
	}

	if ((status = ia64_get_ptce(&ptce)) != 0) {
		printk(KERN_ERR "ia64_get_ptce=%ld\n", status);
	} else {
		seq_printf(m,
		     "\nPurge base address             : 0x%016lx\n"
		     "Purge outer loop count         : %d\n"
		     "Purge inner loop count         : %d\n"
		     "Purge outer loop stride        : %d\n"
		     "Purge inner loop stride        : %d\n",
		     ptce.base, ptce.count[0], ptce.count[1],
		     ptce.stride[0], ptce.stride[1]);

		seq_printf(m,
		     "TC Levels                      : %d\n"
		     "Unique TC(s)                   : %d\n",
		     vm_info_1.pal_vm_info_1_s.num_tc_levels,
		     vm_info_1.pal_vm_info_1_s.max_unique_tcs);

		for(i=0; i < vm_info_1.pal_vm_info_1_s.num_tc_levels; i++) {
			for (j=2; j>0 ; j--) {
				tc_pages = 0; /* just in case */

				/* even without unification, some levels may not be present */
				if ((status=ia64_pal_vm_info(i,j, &tc_info, &tc_pages)) != 0)
					continue;

				seq_printf(m,
				     "\n%s Translation Cache Level %d:\n"
				     "\tHash sets           : %d\n"
				     "\tAssociativity       : %d\n"
				     "\tNumber of entries   : %d\n"
				     "\tFlags               : ",
				     cache_types[j+tc_info.tc_unified], i+1,
				     tc_info.tc_num_sets,
				     tc_info.tc_associativity,
				     tc_info.tc_num_entries);

				if (tc_info.tc_pf)
					seq_puts(m, "PreferredPageSizeOptimized ");
				if (tc_info.tc_unified)
					seq_puts(m, "Unified ");
				if (tc_info.tc_reduce_tr)
					seq_puts(m, "TCReduction");

				seq_puts(m, "\n\tSupported page sizes: ");

				bitvector_process(m, tc_pages);

				/* when unified date (j=2) is enough */
				if (tc_info.tc_unified)
					break;
			}
		}
	}

	seq_putc(m, '\n');
	return 0;
}


static int register_info(struct seq_file *m)
{
	u64 reg_info[2];
	u64 info;
	unsigned long phys_stacked;
	pal_hints_u_t hints;
	unsigned long iregs, dregs;
	static const char * const info_type[] = {
		"Implemented AR(s)",
		"AR(s) with read side-effects",
		"Implemented CR(s)",
		"CR(s) with read side-effects",
	};

	for(info=0; info < 4; info++) {
		if (ia64_pal_register_info(info, &reg_info[0], &reg_info[1]) != 0)
			return 0;
		seq_printf(m, "%-32s : ", info_type[info]);
		bitregister_process(m, reg_info, 128);
		seq_putc(m, '\n');
	}

	if (ia64_pal_rse_info(&phys_stacked, &hints) == 0)
		seq_printf(m,
			   "RSE stacked physical registers   : %ld\n"
			   "RSE load/store hints             : %ld (%s)\n",
			   phys_stacked, hints.ph_data,
			   hints.ph_data < RSE_HINTS_COUNT ? rse_hints[hints.ph_data]: "(??)");

	if (ia64_pal_debug_info(&iregs, &dregs))
		return 0;

	seq_printf(m,
		   "Instruction debug register pairs : %ld\n"
		   "Data debug register pairs        : %ld\n", iregs, dregs);

	return 0;
}

static const char *const proc_features_0[]={		/* Feature set 0 */
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,
	"Unimplemented instruction address fault",
	"INIT, PMI, and LINT pins",
	"Simple unimplemented instr addresses",
	"Variable P-state performance",
	"Virtual machine features implemented",
	"XIP,XPSR,XFS implemented",
	"XR1-XR3 implemented",
	"Disable dynamic predicate prediction",
	"Disable processor physical number",
	"Disable dynamic data cache prefetch",
	"Disable dynamic inst cache prefetch",
	"Disable dynamic branch prediction",
	NULL, NULL, NULL, NULL,
	"Disable P-states",
	"Enable MCA on Data Poisoning",
	"Enable vmsw instruction",
	"Enable extern environmental notification",
	"Disable BINIT on processor time-out",
	"Disable dynamic power management (DPM)",
	"Disable coherency",
	"Disable cache",
	"Enable CMCI promotion",
	"Enable MCA to BINIT promotion",
	"Enable MCA promotion",
	"Enable BERR promotion"
};

static const char *const proc_features_16[]={		/* Feature set 16 */
	"Disable ETM",
	"Enable ETM",
	"Enable MCA on half-way timer",
	"Enable snoop WC",
	NULL,
	"Enable Fast Deferral",
	"Disable MCA on memory aliasing",
	"Enable RSB",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"DP system processor",
	"Low Voltage",
	"HT supported",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL
};

static const char *const *const proc_features[]={
	proc_features_0,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	proc_features_16,
	NULL, NULL, NULL, NULL,
};

static void feature_set_info(struct seq_file *m, u64 avail, u64 status, u64 control,
			     unsigned long set)
{
	const char *const *vf, *const *v;
	int i;

	vf = v = proc_features[set];
	for(i=0; i < 64; i++, avail >>=1, status >>=1, control >>=1) {

		if (!(control))		/* No remaining bits set */
			break;
		if (!(avail & 0x1))	/* Print only bits that are available */
			continue;
		if (vf)
			v = vf + i;
		if ( v && *v ) {
			seq_printf(m, "%-40s : %s %s\n", *v,
				avail & 0x1 ? (status & 0x1 ?
					      "On " : "Off"): "",
				avail & 0x1 ? (control & 0x1 ?
						"Ctrl" : "NoCtrl"): "");
		} else {
			seq_printf(m, "Feature set %2ld bit %2d\t\t\t"
					" : %s %s\n",
				set, i,
				avail & 0x1 ? (status & 0x1 ?
						"On " : "Off"): "",
				avail & 0x1 ? (control & 0x1 ?
						"Ctrl" : "NoCtrl"): "");
		}
	}
}

static int processor_info(struct seq_file *m)
{
	u64 avail=1, status=1, control=1, feature_set=0;
	s64 ret;

	do {
		ret = ia64_pal_proc_get_features(&avail, &status, &control,
						feature_set);
		if (ret < 0)
			return 0;

		if (ret == 1) {
			feature_set++;
			continue;
		}

		feature_set_info(m, avail, status, control, feature_set);
		feature_set++;
	} while(1);

	return 0;
}

static const char *const bus_features[]={
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,
	"Request  Bus Parking",
	"Bus Lock Mask",
	"Enable Half Transfer",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	"Enable Cache Line Repl. Shared",
	"Enable Cache Line Repl. Exclusive",
	"Disable Transaction Queuing",
	"Disable Response Error Checking",
	"Disable Bus Error Checking",
	"Disable Bus Requester Internal Error Signalling",
	"Disable Bus Requester Error Signalling",
	"Disable Bus Initialization Event Checking",
	"Disable Bus Initialization Event Signalling",
	"Disable Bus Address Error Checking",
	"Disable Bus Address Error Signalling",
	"Disable Bus Data Error Checking"
};


static int bus_info(struct seq_file *m)
{
	const char *const *v = bus_features;
	pal_bus_features_u_t av, st, ct;
	u64 avail, status, control;
	int i;
	s64 ret;

	if ((ret=ia64_pal_bus_get_features(&av, &st, &ct)) != 0)
		return 0;

	avail   = av.pal_bus_features_val;
	status  = st.pal_bus_features_val;
	control = ct.pal_bus_features_val;

	for(i=0; i < 64; i++, v++, avail >>=1, status >>=1, control >>=1) {
		if ( ! *v )
			continue;
		seq_printf(m, "%-48s : %s%s %s\n", *v,
			   avail & 0x1 ? "" : "NotImpl",
			   avail & 0x1 ? (status  & 0x1 ? "On" : "Off"): "",
			   avail & 0x1 ? (control & 0x1 ? "Ctrl" : "NoCtrl"): "");
	}
	return 0;
}

static int version_info(struct seq_file *m)
{
	pal_version_u_t min_ver, cur_ver;

	if (ia64_pal_version(&min_ver, &cur_ver) != 0)
		return 0;

	seq_printf(m,
		   "PAL_vendor : 0x%02x (min=0x%02x)\n"
		   "PAL_A      : %02x.%02x (min=%02x.%02x)\n"
		   "PAL_B      : %02x.%02x (min=%02x.%02x)\n",
		   cur_ver.pal_version_s.pv_pal_vendor,
		   min_ver.pal_version_s.pv_pal_vendor,
		   cur_ver.pal_version_s.pv_pal_a_model,
		   cur_ver.pal_version_s.pv_pal_a_rev,
		   min_ver.pal_version_s.pv_pal_a_model,
		   min_ver.pal_version_s.pv_pal_a_rev,
		   cur_ver.pal_version_s.pv_pal_b_model,
		   cur_ver.pal_version_s.pv_pal_b_rev,
		   min_ver.pal_version_s.pv_pal_b_model,
		   min_ver.pal_version_s.pv_pal_b_rev);
	return 0;
}

static int perfmon_info(struct seq_file *m)
{
	u64 pm_buffer[16];
	pal_perf_mon_info_u_t pm_info;

	if (ia64_pal_perf_mon_info(pm_buffer, &pm_info) != 0)
		return 0;

	seq_printf(m,
		   "PMC/PMD pairs                 : %d\n"
		   "Counter width                 : %d bits\n"
		   "Cycle event number            : %d\n"
		   "Retired event number          : %d\n"
		   "Implemented PMC               : ",
		   pm_info.pal_perf_mon_info_s.generic,
		   pm_info.pal_perf_mon_info_s.width,
		   pm_info.pal_perf_mon_info_s.cycles,
		   pm_info.pal_perf_mon_info_s.retired);

	bitregister_process(m, pm_buffer, 256);
	seq_puts(m, "\nImplemented PMD               : ");
	bitregister_process(m, pm_buffer+4, 256);
	seq_puts(m, "\nCycles count capable          : ");
	bitregister_process(m, pm_buffer+8, 256);
	seq_puts(m, "\nRetired bundles count capable : ");

#ifdef CONFIG_ITANIUM
	/*
	 * PAL_PERF_MON_INFO reports that only PMC4 can be used to count CPU_CYCLES
	 * which is wrong, both PMC4 and PMD5 support it.
	 */
	if (pm_buffer[12] == 0x10)
		pm_buffer[12]=0x30;
#endif

	bitregister_process(m, pm_buffer+12, 256);
	seq_putc(m, '\n');
	return 0;
}

static int frequency_info(struct seq_file *m)
{
	struct pal_freq_ratio proc, itc, bus;
	unsigned long base;

	if (ia64_pal_freq_base(&base) == -1)
		seq_puts(m, "Output clock            : not implemented\n");
	else
		seq_printf(m, "Output clock            : %ld ticks/s\n", base);

	if (ia64_pal_freq_ratios(&proc, &bus, &itc) != 0) return 0;

	seq_printf(m,
		     "Processor/Clock ratio   : %d/%d\n"
		     "Bus/Clock ratio         : %d/%d\n"
		     "ITC/Clock ratio         : %d/%d\n",
		     proc.num, proc.den, bus.num, bus.den, itc.num, itc.den);
	return 0;
}

static int tr_info(struct seq_file *m)
{
	long status;
	pal_tr_valid_u_t tr_valid;
	u64 tr_buffer[4];
	pal_vm_info_1_u_t vm_info_1;
	pal_vm_info_2_u_t vm_info_2;
	unsigned long i, j;
	unsigned long max[3], pgm;
	struct ifa_reg {
		unsigned long valid:1;
		unsigned long ig:11;
		unsigned long vpn:52;
	} *ifa_reg;
	struct itir_reg {
		unsigned long rv1:2;
		unsigned long ps:6;
		unsigned long key:24;
		unsigned long rv2:32;
	} *itir_reg;
	struct gr_reg {
		unsigned long p:1;
		unsigned long rv1:1;
		unsigned long ma:3;
		unsigned long a:1;
		unsigned long d:1;
		unsigned long pl:2;
		unsigned long ar:3;
		unsigned long ppn:38;
		unsigned long rv2:2;
		unsigned long ed:1;
		unsigned long ig:11;
	} *gr_reg;
	struct rid_reg {
		unsigned long ig1:1;
		unsigned long rv1:1;
		unsigned long ig2:6;
		unsigned long rid:24;
		unsigned long rv2:32;
	} *rid_reg;

	if ((status = ia64_pal_vm_summary(&vm_info_1, &vm_info_2)) !=0) {
		printk(KERN_ERR "ia64_pal_vm_summary=%ld\n", status);
		return 0;
	}
	max[0] = vm_info_1.pal_vm_info_1_s.max_itr_entry+1;
	max[1] = vm_info_1.pal_vm_info_1_s.max_dtr_entry+1;

	for (i=0; i < 2; i++ ) {
		for (j=0; j < max[i]; j++) {

		status = ia64_pal_tr_read(j, i, tr_buffer, &tr_valid);
		if (status != 0) {
			printk(KERN_ERR "palinfo: pal call failed on tr[%lu:%lu]=%ld\n",
			       i, j, status);
			continue;
		}

		ifa_reg  = (struct ifa_reg *)&tr_buffer[2];

		if (ifa_reg->valid == 0)
			continue;

		gr_reg   = (struct gr_reg *)tr_buffer;
		itir_reg = (struct itir_reg *)&tr_buffer[1];
		rid_reg  = (struct rid_reg *)&tr_buffer[3];

		pgm	 = -1 << (itir_reg->ps - 12);
		seq_printf(m,
			   "%cTR%lu: av=%d pv=%d dv=%d mv=%d\n"
			   "\tppn  : 0x%lx\n"
			   "\tvpn  : 0x%lx\n"
			   "\tps   : ",
			   "ID"[i], j,
			   tr_valid.pal_tr_valid_s.access_rights_valid,
			   tr_valid.pal_tr_valid_s.priv_level_valid,
			   tr_valid.pal_tr_valid_s.dirty_bit_valid,
			   tr_valid.pal_tr_valid_s.mem_attr_valid,
			   (gr_reg->ppn & pgm)<< 12, (ifa_reg->vpn & pgm)<< 12);

		bitvector_process(m, 1<< itir_reg->ps);

		seq_printf(m,
			   "\n\tpl   : %d\n"
			   "\tar   : %d\n"
			   "\trid  : %x\n"
			   "\tp    : %d\n"
			   "\tma   : %d\n"
			   "\td    : %d\n",
			   gr_reg->pl, gr_reg->ar, rid_reg->rid, gr_reg->p, gr_reg->ma,
			   gr_reg->d);
		}
	}
	return 0;
}



/*
 * List {name,function} pairs for every entry in /proc/palinfo/cpu*
 */
static const palinfo_entry_t palinfo_entries[]={
	{ "version_info",	version_info, },
	{ "vm_info",		vm_info, },
	{ "cache_info",		cache_info, },
	{ "power_info",		power_info, },
	{ "register_info",	register_info, },
	{ "processor_info",	processor_info, },
	{ "perfmon_info",	perfmon_info, },
	{ "frequency_info",	frequency_info, },
	{ "bus_info",		bus_info },
	{ "tr_info",		tr_info, }
};

#define NR_PALINFO_ENTRIES	(int) ARRAY_SIZE(palinfo_entries)

static struct proc_dir_entry *palinfo_dir;

/*
 * This data structure is used to pass which cpu,function is being requested
 * It must fit in a 64bit quantity to be passed to the proc callback routine
 *
 * In SMP mode, when we get a request for another CPU, we must call that
 * other CPU using IPI and wait for the result before returning.
 */
typedef union {
	u64 value;
	struct {
		unsigned	req_cpu: 32;	/* for which CPU this info is */
		unsigned	func_id: 32;	/* which function is requested */
	} pal_func_cpu;
} pal_func_cpu_u_t;

#define req_cpu	pal_func_cpu.req_cpu
#define func_id pal_func_cpu.func_id

#ifdef CONFIG_SMP

/*
 * used to hold information about final function to call
 */
typedef struct {
	palinfo_func_t	func;	/* pointer to function to call */
	struct seq_file *m;	/* buffer to store results */
	int		ret;	/* return value from call */
} palinfo_smp_data_t;


/*
 * this function does the actual final call and he called
 * from the smp code, i.e., this is the palinfo callback routine
 */
static void
palinfo_smp_call(void *info)
{
	palinfo_smp_data_t *data = (palinfo_smp_data_t *)info;
	data->ret = (*data->func)(data->m);
}

/*
 * function called to trigger the IPI, we need to access a remote CPU
 * Return:
 *	0 : error or nothing to output
 *	otherwise how many bytes in the "page" buffer were written
 */
static
int palinfo_handle_smp(struct seq_file *m, pal_func_cpu_u_t *f)
{
	palinfo_smp_data_t ptr;
	int ret;

	ptr.func = palinfo_entries[f->func_id].proc_read;
	ptr.m = m;
	ptr.ret  = 0; /* just in case */


	/* will send IPI to other CPU and wait for completion of remote call */
	if ((ret=smp_call_function_single(f->req_cpu, palinfo_smp_call, &ptr, 1))) {
		printk(KERN_ERR "palinfo: remote CPU call from %d to %d on function %d: "
		       "error %d\n", smp_processor_id(), f->req_cpu, f->func_id, ret);
		return 0;
	}
	return ptr.ret;
}
#else /* ! CONFIG_SMP */
static
int palinfo_handle_smp(struct seq_file *m, pal_func_cpu_u_t *f)
{
	printk(KERN_ERR "palinfo: should not be called with non SMP kernel\n");
	return 0;
}
#endif /* CONFIG_SMP */

/*
 * Entry point routine: all calls go through this function
 */
static int proc_palinfo_show(struct seq_file *m, void *v)
{
	pal_func_cpu_u_t *f = (pal_func_cpu_u_t *)&m->private;

	/*
	 * in SMP mode, we may need to call another CPU to get correct
	 * information. PAL, by definition, is processor specific
	 */
	if (f->req_cpu == get_cpu())
		(*palinfo_entries[f->func_id].proc_read)(m);
	else
		palinfo_handle_smp(m, f);

	put_cpu();
	return 0;
}

static int proc_palinfo_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_palinfo_show, PDE_DATA(inode));
}

static const struct file_operations proc_palinfo_fops = {
	.open		= proc_palinfo_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void
create_palinfo_proc_entries(unsigned int cpu)
{
	pal_func_cpu_u_t f;
	struct proc_dir_entry *cpu_dir;
	int j;
	char cpustr[3+4+1];	/* cpu numbers are up to 4095 on itanic */
	sprintf(cpustr, "cpu%d", cpu);

	cpu_dir = proc_mkdir(cpustr, palinfo_dir);
	if (!cpu_dir)
		return;

	f.req_cpu = cpu;

	for (j=0; j < NR_PALINFO_ENTRIES; j++) {
		f.func_id = j;
		proc_create_data(palinfo_entries[j].name, 0, cpu_dir,
				 &proc_palinfo_fops, (void *)f.value);
	}
}

static void
remove_palinfo_proc_entries(unsigned int hcpu)
{
	char cpustr[3+4+1];	/* cpu numbers are up to 4095 on itanic */
	sprintf(cpustr, "cpu%d", hcpu);
	remove_proc_subtree(cpustr, palinfo_dir);
}

static int palinfo_cpu_callback(struct notifier_block *nfb,
					unsigned long action, void *hcpu)
{
	unsigned int hotcpu = (unsigned long)hcpu;

	switch (action) {
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		create_palinfo_proc_entries(hotcpu);
		break;
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		remove_palinfo_proc_entries(hotcpu);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block __refdata palinfo_cpu_notifier =
{
	.notifier_call = palinfo_cpu_callback,
	.priority = 0,
};

static int __init
palinfo_init(void)
{
	int i = 0;

	printk(KERN_INFO "PAL Information Facility v%s\n", PALINFO_VERSION);
	palinfo_dir = proc_mkdir("pal", NULL);
	if (!palinfo_dir)
		return -ENOMEM;

	cpu_notifier_register_begin();

	/* Create palinfo dirs in /proc for all online cpus */
	for_each_online_cpu(i) {
		create_palinfo_proc_entries(i);
	}

	/* Register for future delivery via notify registration */
	__register_hotcpu_notifier(&palinfo_cpu_notifier);

	cpu_notifier_register_done();

	return 0;
}

static void __exit
palinfo_exit(void)
{
	unregister_hotcpu_notifier(&palinfo_cpu_notifier);
	remove_proc_subtree("pal", NULL);
}

module_init(palinfo_init);
module_exit(palinfo_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * Instruction-patching support.
 *
 * Copyright (C) 2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */
#include <linux/init.h>
#include <linux/string.h>

#include <asm/patch.h>
#include <asm/processor.h>
#include <asm/sections.h>
#include <asm/unistd.h>

/*
 * This was adapted from code written by Tony Luck:
 *
 * The 64-bit value in a "movl reg=value" is scattered between the two words of the bundle
 * like this:
 *
 * 6  6         5         4         3         2         1
 * 3210987654321098765432109876543210987654321098765432109876543210
 * ABBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCDEEEEEFFFFFFFFFGGGGGGG
 *
 * CCCCCCCCCCCCCCCCCCxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 * xxxxAFFFFFFFFFEEEEEDxGGGGGGGxxxxxxxxxxxxxBBBBBBBBBBBBBBBBBBBBBBB
 */
static u64
get_imm64 (u64 insn_addr)
{
	u64 *p = (u64 *) (insn_addr & -16);	/* mask out slot number */

	return ( (p[1] & 0x0800000000000000UL) << 4)  | /*A*/
		((p[1] & 0x00000000007fffffUL) << 40) | /*B*/
		((p[0] & 0xffffc00000000000UL) >> 24) | /*C*/
		((p[1] & 0x0000100000000000UL) >> 23) | /*D*/
		((p[1] & 0x0003e00000000000UL) >> 29) | /*E*/
		((p[1] & 0x07fc000000000000UL) >> 43) | /*F*/
		((p[1] & 0x000007f000000000UL) >> 36);  /*G*/
}

/* Patch instruction with "val" where "mask" has 1 bits. */
void
ia64_patch (u64 insn_addr, u64 mask, u64 val)
{
	u64 m0, m1, v0, v1, b0, b1, *b = (u64 *) (insn_addr & -16);
#	define insn_mask ((1UL << 41) - 1)
	unsigned long shift;

	b0 = b[0]; b1 = b[1];
	shift = 5 + 41 * (insn_addr % 16); /* 5 bits of template, then 3 x 41-bit instructions */
	if (shift >= 64) {
		m1 = mask << (shift - 64);
		v1 = val << (shift - 64);
	} else {
		m0 = mask << shift; m1 = mask >> (64 - shift);
		v0 = val  << shift; v1 = val >> (64 - shift);
		b[0] = (b0 & ~m0) | (v0 & m0);
	}
	b[1] = (b1 & ~m1) | (v1 & m1);
}

void
ia64_patch_imm64 (u64 insn_addr, u64 val)
{
	/* The assembler may generate offset pointing to either slot 1
	   or slot 2 for a long (2-slot) instruction, occupying slots 1
	   and 2.  */
  	insn_addr &= -16UL;
	ia64_patch(insn_addr + 2,
		   0x01fffefe000UL, (  ((val & 0x8000000000000000UL) >> 27) /* bit 63 -> 36 */
				     | ((val & 0x0000000000200000UL) <<  0) /* bit 21 -> 21 */
				     | ((val & 0x00000000001f0000UL) <<  6) /* bit 16 -> 22 */
				     | ((val & 0x000000000000ff80UL) << 20) /* bit  7 -> 27 */
				     | ((val & 0x000000000000007fUL) << 13) /* bit  0 -> 13 */));
	ia64_patch(insn_addr + 1, 0x1ffffffffffUL, val >> 22);
}

void
ia64_patch_imm60 (u64 insn_addr, u64 val)
{
	/* The assembler may generate offset pointing to either slot 1
	   or slot 2 for a long (2-slot) instruction, occupying slots 1
	   and 2.  */
  	insn_addr &= -16UL;
	ia64_patch(insn_addr + 2,
		   0x011ffffe000UL, (  ((val & 0x0800000000000000UL) >> 23) /* bit 59 -> 36 */
				     | ((val & 0x00000000000fffffUL) << 13) /* bit  0 -> 13 */));
	ia64_patch(insn_addr + 1, 0x1fffffffffcUL, val >> 18);
}

/*
 * We need sometimes to load the physical address of a kernel
 * object.  Often we can convert the virtual address to physical
 * at execution time, but sometimes (either for performance reasons
 * or during error recovery) we cannot to this.  Patch the marked
 * bundles to load the physical address.
 */
void __init
ia64_patch_vtop (unsigned long start, unsigned long end)
{
	s32 *offp = (s32 *) start;
	u64 ip;

	while (offp < (s32 *) end) {
		ip = (u64) offp + *offp;

		/* replace virtual address with corresponding physical address: */
		ia64_patch_imm64(ip, ia64_tpa(get_imm64(ip)));
		ia64_fc((void *) ip);
		++offp;
	}
	ia64_sync_i();
	ia64_srlz_i();
}

/*
 * Disable the RSE workaround by turning the conditional branch
 * that we tagged in each place the workaround was used into an
 * unconditional branch.
 */
void __init
ia64_patch_rse (unsigned long start, unsigned long end)
{
	s32 *offp = (s32 *) start;
	u64 ip, *b;

	while (offp < (s32 *) end) {
		ip = (u64) offp + *offp;

		b = (u64 *)(ip & -16);
		b[1] &= ~0xf800000L;
		ia64_fc((void *) ip);
		++offp;
	}
	ia64_sync_i();
	ia64_srlz_i();
}

void __init
ia64_patch_mckinley_e9 (unsigned long start, unsigned long end)
{
	static int first_time = 1;
	int need_workaround;
	s32 *offp = (s32 *) start;
	u64 *wp;

	need_workaround = (local_cpu_data->family == 0x1f && local_cpu_data->model == 0);

	if (first_time) {
		first_time = 0;
		if (need_workaround)
			printk(KERN_INFO "Leaving McKinley Errata 9 workaround enabled\n");
	}
	if (need_workaround)
		return;

	while (offp < (s32 *) end) {
		wp = (u64 *) ia64_imva((char *) offp + *offp);
		wp[0] = 0x0000000100000011UL; /* nop.m 0; nop.i 0; br.ret.sptk.many b6 */
		wp[1] = 0x0084006880000200UL;
		wp[2] = 0x0000000100000000UL; /* nop.m 0; nop.i 0; nop.i 0 */
		wp[3] = 0x0004000000000200UL;
		ia64_fc(wp); ia64_fc(wp + 2);
		++offp;
	}
	ia64_sync_i();
	ia64_srlz_i();
}

static void __init
patch_fsyscall_table (unsigned long start, unsigned long end)
{
	extern unsigned long fsyscall_table[NR_syscalls];
	s32 *offp = (s32 *) start;
	u64 ip;

	while (offp < (s32 *) end) {
		ip = (u64) ia64_imva((char *) offp + *offp);
		ia64_patch_imm64(ip, (u64) fsyscall_table);
		ia64_fc((void *) ip);
		++offp;
	}
	ia64_sync_i();
	ia64_srlz_i();
}

static void __init
patch_brl_fsys_bubble_down (unsigned long start, unsigned long end)
{
	extern char fsys_bubble_down[];
	s32 *offp = (s32 *) start;
	u64 ip;

	while (offp < (s32 *) end) {
		ip = (u64) offp + *offp;
		ia64_patch_imm60((u64) ia64_imva((void *) ip),
				 (u64) (fsys_bubble_down - (ip & -16)) / 16);
		ia64_fc((void *) ip);
		++offp;
	}
	ia64_sync_i();
	ia64_srlz_i();
}

void __init
ia64_patch_gate (void)
{
#	define START(name)	((unsigned long) __start_gate_##name##_patchlist)
#	define END(name)	((unsigned long)__end_gate_##name##_patchlist)

	patch_fsyscall_table(START(fsyscall), END(fsyscall));
	patch_brl_fsys_bubble_down(START(brl_fsys_bubble_down), END(brl_fsys_bubble_down));
	ia64_patch_vtop(START(vtop), END(vtop));
	ia64_patch_mckinley_e9(START(mckinley_e9), END(mckinley_e9));
}

void ia64_patch_phys_stack_reg(unsigned long val)
{
	s32 * offp = (s32 *) __start___phys_stack_reg_patchlist;
	s32 * end = (s32 *) __end___phys_stack_reg_patchlist;
	u64 ip, mask, imm;

	/* see instruction format A4: adds r1 = imm13, r3 */
	mask = (0x3fUL << 27) | (0x7f << 13);
	imm = (((val >> 7) & 0x3f) << 27) | (val & 0x7f) << 13;

	while (offp < end) {
		ip = (u64) offp + *offp;
		ia64_patch(ip, mask, imm);
		ia64_fc((void *)ip);
		++offp;
	}
	ia64_sync_i();
	ia64_srlz_i();
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Dynamic DMA mapping support.
 */

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/dmar.h>
#include <asm/iommu.h>
#include <asm/machvec.h>
#include <linux/dma-mapping.h>


#ifdef CONFIG_INTEL_IOMMU

#include <linux/kernel.h>

#include <asm/page.h>

dma_addr_t bad_dma_address __read_mostly;
EXPORT_SYMBOL(bad_dma_address);

static int iommu_sac_force __read_mostly;

int no_iommu __read_mostly;
#ifdef CONFIG_IOMMU_DEBUG
int force_iommu __read_mostly = 1;
#else
int force_iommu __read_mostly;
#endif

int iommu_pass_through;

extern struct dma_map_ops intel_dma_ops;

static int __init pci_iommu_init(void)
{
	if (iommu_detected)
		intel_iommu_init();

	return 0;
}

/* Must execute after PCI subsystem */
fs_initcall(pci_iommu_init);

void pci_iommu_shutdown(void)
{
	return;
}

void __init
iommu_dma_init(void)
{
	return;
}

int iommu_dma_supported(struct device *dev, u64 mask)
{
	/* Copied from i386. Doesn't make much sense, because it will
	   only work for pci_alloc_coherent.
	   The caller just has to use GFP_DMA in this case. */
	if (mask < DMA_BIT_MASK(24))
		return 0;

	/* Tell the device to use SAC when IOMMU force is on.  This
	   allows the driver to use cheaper accesses in some cases.

	   Problem with this is that if we overflow the IOMMU area and
	   return DAC as fallback address the device may not handle it
	   correctly.

	   As a special case some controllers have a 39bit address
	   mode that is as efficient as 32bit (aic79xx). Don't force
	   SAC for these.  Assume all masks <= 40 bits are of this
	   type. Normally this doesn't make any difference, but gives
	   more gentle handling of IOMMU overflow. */
	if (iommu_sac_force && (mask >= DMA_BIT_MASK(40))) {
		dev_info(dev, "Force SAC with mask %llx\n", mask);
		return 0;
	}

	return 1;
}
EXPORT_SYMBOL(iommu_dma_supported);

void __init pci_iommu_alloc(void)
{
	dma_ops = &intel_dma_ops;

	dma_ops->sync_single_for_cpu = machvec_dma_sync_single;
	dma_ops->sync_sg_for_cpu = machvec_dma_sync_sg;
	dma_ops->sync_single_for_device = machvec_dma_sync_single;
	dma_ops->sync_sg_for_device = machvec_dma_sync_sg;
	dma_ops->dma_supported = iommu_dma_supported;

	/*
	 * The order of these functions is important for
	 * fall-back/fail-over reasons
	 */
	detect_intel_iommu();

#ifdef CONFIG_SWIOTLB
	pci_swiotlb_init();
#endif
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /* Glue code to lib/swiotlb.c */

#include <linux/pci.h>
#include <linux/gfp.h>
#include <linux/cache.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include <asm/swiotlb.h>
#include <asm/dma.h>
#include <asm/iommu.h>
#include <asm/machvec.h>

int swiotlb __read_mostly;
EXPORT_SYMBOL(swiotlb);

static void *ia64_swiotlb_alloc_coherent(struct device *dev, size_t size,
					 dma_addr_t *dma_handle, gfp_t gfp,
					 struct dma_attrs *attrs)
{
	if (dev->coherent_dma_mask != DMA_BIT_MASK(64))
		gfp |= GFP_DMA;
	return swiotlb_alloc_coherent(dev, size, dma_handle, gfp);
}

static void ia64_swiotlb_free_coherent(struct device *dev, size_t size,
				       void *vaddr, dma_addr_t dma_addr,
				       struct dma_attrs *attrs)
{
	swiotlb_free_coherent(dev, size, vaddr, dma_addr);
}

struct dma_map_ops swiotlb_dma_ops = {
	.alloc = ia64_swiotlb_alloc_coherent,
	.free = ia64_swiotlb_free_coherent,
	.map_page = swiotlb_map_page,
	.unmap_page = swiotlb_unmap_page,
	.map_sg = swiotlb_map_sg_attrs,
	.unmap_sg = swiotlb_unmap_sg_attrs,
	.sync_single_for_cpu = swiotlb_sync_single_for_cpu,
	.sync_single_for_device = swiotlb_sync_single_for_device,
	.sync_sg_for_cpu = swiotlb_sync_sg_for_cpu,
	.sync_sg_for_device = swiotlb_sync_sg_for_device,
	.dma_supported = swiotlb_dma_supported,
	.mapping_error = swiotlb_dma_mapping_error,
};

void __init swiotlb_dma_init(void)
{
	dma_ops = &swiotlb_dma_ops;
	swiotlb_init(1);
}

void __init pci_swiotlb_init(void)
{
	if (!iommu_detected) {
#ifdef CONFIG_IA64_GENERIC
		swiotlb = 1;
		printk(KERN_INFO "PCI-DMA: Re-initialize machine vector.\n");
		machvec_init("dig");
		swiotlb_init(1);
		dma_ops = &swiotlb_dma_ops;
#else
		panic("Unable to find Intel IOMMU");
#endif
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * Copyright (C) 2002-2003 Hewlett-Packard Co
 *               Stephane Eranian <eranian@hpl.hp.com>
 *
 * This file implements the default sampling buffer format
 * for the Linux/ia64 perfmon-2 subsystem.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/delay.h>
#include <linux/smp.h>

#include <asm/perfmon.h>
#include <asm/perfmon_default_smpl.h>

MODULE_AUTHOR("Stephane Eranian <eranian@hpl.hp.com>");
MODULE_DESCRIPTION("perfmon default sampling format");
MODULE_LICENSE("GPL");

#define DEFAULT_DEBUG 1

#ifdef DEFAULT_DEBUG
#define DPRINT(a) \
	do { \
		if (unlikely(pfm_sysctl.debug >0)) { printk("%s.%d: CPU%d ", __func__, __LINE__, smp_processor_id()); printk a; } \
	} while (0)

#define DPRINT_ovfl(a) \
	do { \
		if (unlikely(pfm_sysctl.debug > 0 && pfm_sysctl.debug_ovfl >0)) { printk("%s.%d: CPU%d ", __func__, __LINE__, smp_processor_id()); printk a; } \
	} while (0)

#else
#define DPRINT(a)
#define DPRINT_ovfl(a)
#endif

static int
default_validate(struct task_struct *task, unsigned int flags, int cpu, void *data)
{
	pfm_default_smpl_arg_t *arg = (pfm_default_smpl_arg_t*)data;
	int ret = 0;

	if (data == NULL) {
		DPRINT(("[%d] no argument passed\n", task_pid_nr(task)));
		return -EINVAL;
	}

	DPRINT(("[%d] validate flags=0x%x CPU%d\n", task_pid_nr(task), flags, cpu));

	/*
	 * must hold at least the buffer header + one minimally sized entry
	 */
	if (arg->buf_size < PFM_DEFAULT_SMPL_MIN_BUF_SIZE) return -EINVAL;

	DPRINT(("buf_size=%lu\n", arg->buf_size));

	return ret;
}

static int
default_get_size(struct task_struct *task, unsigned int flags, int cpu, void *data, unsigned long *size)
{
	pfm_default_smpl_arg_t *arg = (pfm_default_smpl_arg_t *)data;

	/*
	 * size has been validated in default_validate
	 */
	*size = arg->buf_size;

	return 0;
}

static int
default_init(struct task_struct *task, void *buf, unsigned int flags, int cpu, void *data)
{
	pfm_default_smpl_hdr_t *hdr;
	pfm_default_smpl_arg_t *arg = (pfm_default_smpl_arg_t *)data;

	hdr = (pfm_default_smpl_hdr_t *)buf;

	hdr->hdr_version      = PFM_DEFAULT_SMPL_VERSION;
	hdr->hdr_buf_size     = arg->buf_size;
	hdr->hdr_cur_offs     = sizeof(*hdr);
	hdr->hdr_overflows    = 0UL;
	hdr->hdr_count        = 0UL;

	DPRINT(("[%d] buffer=%p buf_size=%lu hdr_size=%lu hdr_version=%u cur_offs=%lu\n",
		task_pid_nr(task),
		buf,
		hdr->hdr_buf_size,
		sizeof(*hdr),
		hdr->hdr_version,
		hdr->hdr_cur_offs));

	return 0;
}

static int
default_handler(struct task_struct *task, void *buf, pfm_ovfl_arg_t *arg, struct pt_regs *regs, unsigned long stamp)
{
	pfm_default_smpl_hdr_t *hdr;
	pfm_default_smpl_entry_t *ent;
	void *cur, *last;
	unsigned long *e, entry_size;
	unsigned int npmds, i;
	unsigned char ovfl_pmd;
	unsigned char ovfl_notify;

	if (unlikely(buf == NULL || arg == NULL|| regs == NULL || task == NULL)) {
		DPRINT(("[%d] invalid arguments buf=%p arg=%p\n", task->pid, buf, arg));
		return -EINVAL;
	}

	hdr         = (pfm_default_smpl_hdr_t *)buf;
	cur         = buf+hdr->hdr_cur_offs;
	last        = buf+hdr->hdr_buf_size;
	ovfl_pmd    = arg->ovfl_pmd;
	ovfl_notify = arg->ovfl_notify;

	/*
	 * precheck for sanity
	 */
	if ((last - cur) < PFM_DEFAULT_MAX_ENTRY_SIZE) goto full;

	npmds = hweight64(arg->smpl_pmds[0]);

	ent = (pfm_default_smpl_entry_t *)cur;

	prefetch(arg->smpl_pmds_values);

	entry_size = sizeof(*ent) + (npmds << 3);

	/* position for first pmd */
	e = (unsigned long *)(ent+1);

	hdr->hdr_count++;

	DPRINT_ovfl(("[%d] count=%lu cur=%p last=%p free_bytes=%lu ovfl_pmd=%d ovfl_notify=%d npmds=%u\n",
			task->pid,
			hdr->hdr_count,
			cur, last,
			last-cur,
			ovfl_pmd,
			ovfl_notify, npmds));

	/*
	 * current = task running at the time of the overflow.
	 *
	 * per-task mode:
	 * 	- this is usually the task being monitored.
	 * 	  Under certain conditions, it might be a different task
	 *
	 * system-wide:
	 * 	- this is not necessarily the task controlling the session
	 */
	ent->pid            = current->pid;
	ent->ovfl_pmd  	    = ovfl_pmd;
	ent->last_reset_val = arg->pmd_last_reset; //pmd[0].reg_last_reset_val;

	/*
	 * where did the fault happen (includes slot number)
	 */
	ent->ip = regs->cr_iip | ((regs->cr_ipsr >> 41) & 0x3);

	ent->tstamp    = stamp;
	ent->cpu       = smp_processor_id();
	ent->set       = arg->active_set;
	ent->tgid      = current->tgid;

	/*
	 * selectively store PMDs in increasing index number
	 */
	if (npmds) {
		unsigned long *val = arg->smpl_pmds_values;
		for(i=0; i < npmds; i++) {
			*e++ = *val++;
		}
	}

	/*
	 * update position for next entry
	 */
	hdr->hdr_cur_offs += entry_size;
	cur               += entry_size;

	/*
	 * post check to avoid losing the last sample
	 */
	if ((last - cur) < PFM_DEFAULT_MAX_ENTRY_SIZE) goto full;

	/*
	 * keep same ovfl_pmds, ovfl_notify
	 */
	arg->ovfl_ctrl.bits.notify_user     = 0;
	arg->ovfl_ctrl.bits.block_task      = 0;
	arg->ovfl_ctrl.bits.mask_monitoring = 0;
	arg->ovfl_ctrl.bits.reset_ovfl_pmds = 1; /* reset before returning from interrupt handler */

	return 0;
full:
	DPRINT_ovfl(("sampling buffer full free=%lu, count=%lu, ovfl_notify=%d\n", last-cur, hdr->hdr_count, ovfl_notify));

	/*
	 * increment number of buffer overflow.
	 * important to detect duplicate set of samples.
	 */
	hdr->hdr_overflows++;

	/*
	 * if no notification requested, then we saturate the buffer
	 */
	if (ovfl_notify == 0) {
		arg->ovfl_ctrl.bits.notify_user     = 0;
		arg->ovfl_ctrl.bits.block_task      = 0;
		arg->ovfl_ctrl.bits.mask_monitoring = 1;
		arg->ovfl_ctrl.bits.reset_ovfl_pmds = 0;
	} else {
		arg->ovfl_ctrl.bits.notify_user     = 1;
		arg->ovfl_ctrl.bits.block_task      = 1; /* ignored for non-blocking context */
		arg->ovfl_ctrl.bits.mask_monitoring = 1;
		arg->ovfl_ctrl.bits.reset_ovfl_pmds = 0; /* no reset now */
	}
	return -1; /* we are full, sorry */
}

static int
default_restart(struct task_struct *task, pfm_ovfl_ctrl_t *ctrl, void *buf, struct pt_regs *regs)
{
	pfm_default_smpl_hdr_t *hdr;

	hdr = (pfm_default_smpl_hdr_t *)buf;

	hdr->hdr_count    = 0UL;
	hdr->hdr_cur_offs = sizeof(*hdr);

	ctrl->bits.mask_monitoring = 0;
	ctrl->bits.reset_ovfl_pmds = 1; /* uses long-reset values */

	return 0;
}

static int
default_exit(struct task_struct *task, void *buf, struct pt_regs *regs)
{
	DPRINT(("[%d] exit(%p)\n", task_pid_nr(task), buf));
	return 0;
}

static pfm_buffer_fmt_t default_fmt={
 	.fmt_name 	    = "default_format",
 	.fmt_uuid	    = PFM_DEFAULT_SMPL_UUID,
 	.fmt_arg_size	    = sizeof(pfm_default_smpl_arg_t),
 	.fmt_validate	    = default_validate,
 	.fmt_getsize	    = default_get_size,
 	.fmt_init	    = default_init,
 	.fmt_handler	    = default_handler,
 	.fmt_restart	    = default_restart,
 	.fmt_restart_active = default_restart,
 	.fmt_exit	    = default_exit,
};

static int __init
pfm_default_smpl_init_module(void)
{
	int ret;

	ret = pfm_register_buffer_fmt(&default_fmt);
	if (ret == 0) {
		printk("perfmon_default_smpl: %s v%u.%u registered\n",
			default_fmt.fmt_name,
			PFM_DEFAULT_SMPL_VERSION_MAJ,
			PFM_DEFAULT_SMPL_VERSION_MIN);
	} else {
		printk("perfmon_default_smpl: %s cannot register ret=%d\n",
			default_fmt.fmt_name,
			ret);
	}

	return ret;
}

static void __exit
pfm_default_smpl_cleanup_module(void)
{
	int ret;
	ret = pfm_unregister_buffer_fmt(default_fmt.fmt_uuid);

	printk("perfmon_default_smpl: unregister %s=%d\n", default_fmt.fmt_name, ret);
}

module_init(pfm_default_smpl_init_module);
module_exit(pfm_default_smpl_cleanup_module);

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MOTOROLA MICROPROCESSOR & MEMORY TECHNOLOGY GROUP
M68000 Hi-Performance Microprocessor Division
M68060 Software Package
Production Release P1.00 -- October 10, 1994

M68060 Software Package Copyright © 1993, 1994 Motorola Inc.  All rights reserved.

THE SOFTWARE is provided on an "AS IS" basis and without warranty.
To the maximum extent permitted by applicable law,
MOTOROLA DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
and any warranty against infringement with regard to the SOFTWARE
(INCLUDING ANY MODIFIED VERSIONS THEREOF) and any accompanying written materials.

To the maximum extent permitted by applicable law,
IN NO EVENT SHALL MOTOROLA BE LIABLE FOR ANY DAMAGES WHATSOEVER
(INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS)
ARISING OF THE USE OR INABILITY TO USE THE SOFTWARE.
Motorola assumes no responsibility for the maintenance and support of the SOFTWARE.

You are hereby granted a copyright license to use, modify, and distribute the SOFTWARE
so long as this entire notice is retained without alteration in any modified and/or
redistributed versions, and that such modified versions are clearly identified as such.
No licenses are granted by implication, estoppel or otherwise under any patents
or trademarks of Motorola, Inc.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#############################################
set	SREGS,		-64
set	IREGS,		-128
set	IFPREGS,	-224
set	SFPREGS,	-320
set	IFPCREGS,	-332
set	SFPCREGS,	-344
set	ICCR,		-346
set	SCCR,		-348
set	TESTCTR,	-352
set	DATA,		-384

#############################################
TESTTOP:
	bra.l		_060TESTS_
	short		0x0000

	bra.l		_060TESTS_unimp
	short		0x0000

	bra.l		_060TESTS_enable
	short		0x0000

start_str:
	string		"Testing 68060 FPSP started:\n"

start_str_unimp:
	string		"Testing 68060 FPSP unimplemented instruction started:\n"

start_str_enable:
	string		"Testing 68060 FPSP exception enabled started:\n"

pass_str:
	string		"passed\n"

fail_str:
	string		" failed\n"

	align		0x4
chk_test:
	tst.l		%d0
	bne.b		test_fail
test_pass:
	pea		pass_str(%pc)
	bsr.l		_print_str
	addq.l		&0x4,%sp
	rts
test_fail:
	mov.l		%d1,-(%sp)
	bsr.l		_print_num
	addq.l		&0x4,%sp

	pea		fail_str(%pc)
	bsr.l		_print_str
	addq.l		&0x4,%sp
	rts

#############################################
_060TESTS_:
	link		%a6,&-384

	movm.l		&0x3f3c,-(%sp)
	fmovm.x		&0xff,-(%sp)

	pea		start_str(%pc)
	bsr.l		_print_str
	addq.l		&0x4,%sp

### effadd
	clr.l		TESTCTR(%a6)
	pea		effadd_str(%pc)
	bsr.l		_print_str
	addq.l		&0x4,%sp

	bsr.l		effadd_0

	bsr.l		chk_test

### unsupp
	clr.l		TESTCTR(%a6)
	pea		unsupp_str(%pc)
	bsr.l		_print_str
	addq.l		&0x4,%sp

	bsr.l		unsupp_0

	bsr.l		chk_test

### ovfl non-maskable
	clr.l		TESTCTR(%a6)
	pea		ovfl_nm_str(%pc)
	bsr.l		_print_str
	bsr.l		ovfl_nm_0

	bsr.l		chk_test

### unfl non-maskable
	clr.l		TESTCTR(%a6)
	pea		unfl_nm_str(%pc)
	bsr.l		_print_str
	bsr.l		unfl_nm_0

	bsr.l		chk_test

	movm.l		(%sp)+,&0x3cfc
	fmovm.x		(%sp)+,&0xff

	unlk		%a6
	rts

_060TESTS_unimp:
	link		%a6,&-384

	movm.l		&0x3f3c,-(%sp)
	fmovm.x		&0xff,-(%sp)

	pea		start_str_unimp(%pc)
	bsr.l		_print_str
	addq.l		&0x4,%sp

### unimp
	clr.l		TESTCTR(%a6)
	pea		unimp_str(%pc)
	bsr.l		_print_str
	addq.l		&0x4,%sp

	bsr.l		unimp_0

	bsr.l		chk_test

	movm.l		(%sp)+,&0x3cfc
	fmovm.x		(%sp)+,&0xff

	unlk		%a6
	rts

_060TESTS_enable:
	link		%a6,&-384

	movm.l		&0x3f3c,-(%sp)
	fmovm.x		&0xff,-(%sp)

	pea		start_str_enable(%pc)
	bsr.l		_print_str
	addq.l		&0x4,%sp

### snan
	clr.l		TESTCTR(%a6)
	pea		snan_str(%pc)
	bsr.l		_print_str
	bsr.l		snan_0

	bsr.l		chk_test

### operr
	clr.l		TESTCTR(%a6)
	pea		operr_str(%pc)
	bsr.l		_print_str
	bsr.l		operr_0

	bsr.l		chk_test

### ovfl
	clr.l		TESTCTR(%a6)
	pea		ovfl_str(%pc)
	bsr.l		_print_str
	bsr.l		ovfl_0

	bsr.l		chk_test

### unfl
	clr.l		TESTCTR(%a6)
	pea		unfl_str(%pc)
	bsr.l		_print_str
	bsr.l		unfl_0

	bsr.l		chk_test

### dz
	clr.l		TESTCTR(%a6)
	pea		dz_str(%pc)
	bsr.l		_print_str
	bsr.l		dz_0

	bsr.l		chk_test

### inexact
	clr.l		TESTCTR(%a6)
	pea		inex_str(%pc)
	bsr.l		_print_str
	bsr.l		inex_0

	bsr.l		chk_test

	movm.l		(%sp)+,&0x3cfc
	fmovm.x		(%sp)+,&0xff

	unlk		%a6
	rts

#########################################