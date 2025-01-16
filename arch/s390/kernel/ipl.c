fsetof (struct pt_regs, pr));
	DEFINE(IA64_PT_REGS_B0_OFFSET, offsetof (struct pt_regs, b0));
	DEFINE(IA64_PT_REGS_LOADRS_OFFSET, offsetof (struct pt_regs, loadrs));
	DEFINE(IA64_PT_REGS_R1_OFFSET, offsetof (struct pt_regs, r1));
	DEFINE(IA64_PT_REGS_R12_OFFSET, offsetof (struct pt_regs, r12));
	DEFINE(IA64_PT_REGS_R13_OFFSET, offsetof (struct pt_regs, r13));
	DEFINE(IA64_PT_REGS_AR_FPSR_OFFSET, offsetof (struct pt_regs, ar_fpsr));
	DEFINE(IA64_PT_REGS_R15_OFFSET, offsetof (struct pt_regs, r15));
	DEFINE(IA64_PT_REGS_R14_OFFSET, offsetof (struct pt_regs, r14));
	DEFINE(IA64_PT_REGS_R2_OFFSET, offsetof (struct pt_regs, r2));
	DEFINE(IA64_PT_REGS_R3_OFFSET, offsetof (struct pt_regs, r3));
	DEFINE(IA64_PT_REGS_R16_OFFSET, offsetof (struct pt_regs, r16));
	DEFINE(IA64_PT_REGS_R17_OFFSET, offsetof (struct pt_regs, r17));
	DEFINE(IA64_PT_REGS_R18_OFFSET, offsetof (struct pt_regs, r18));
	DEFINE(IA64_PT_REGS_R19_OFFSET, offsetof (struct pt_regs, r19));
	DEFINE(IA64_PT_REGS_R20_OFFSET, offsetof (struct pt_regs, r20));
	DEFINE(IA64_PT_REGS_R21_OFFSET, offsetof (struct pt_regs, r21));
	DEFINE(IA64_PT_REGS_R22_OFFSET, offsetof (struct pt_regs, r22));
	DEFINE(IA64_PT_REGS_R23_OFFSET, offsetof (struct pt_regs, r23));
	DEFINE(IA64_PT_REGS_R24_OFFSET, offsetof (struct pt_regs, r24));
	DEFINE(IA64_PT_REGS_R25_OFFSET, offsetof (struct pt_regs, r25));
	DEFINE(IA64_PT_REGS_R26_OFFSET, offsetof (struct pt_regs, r26));
	DEFINE(IA64_PT_REGS_R27_OFFSET, offsetof (struct pt_regs, r27));
	DEFINE(IA64_PT_REGS_R28_OFFSET, offsetof (struct pt_regs, r28));
	DEFINE(IA64_PT_REGS_R29_OFFSET, offsetof (struct pt_regs, r29));
	DEFINE(IA64_PT_REGS_R30_OFFSET, offsetof (struct pt_regs, r30));
	DEFINE(IA64_PT_REGS_R31_OFFSET, offsetof (struct pt_regs, r31));
	DEFINE(IA64_PT_REGS_AR_CCV_OFFSET, offsetof (struct pt_regs, ar_ccv));
	DEFINE(IA64_PT_REGS_F6_OFFSET, offsetof (struct pt_regs, f6));
	DEFINE(IA64_PT_REGS_F7_OFFSET, offsetof (struct pt_regs, f7));
	DEFINE(IA64_PT_REGS_F8_OFFSET, offsetof (struct pt_regs, f8));
	DEFINE(IA64_PT_REGS_F9_OFFSET, offsetof (struct pt_regs, f9));
	DEFINE(IA64_PT_REGS_F10_OFFSET, offsetof (struct pt_regs, f10));
	DEFINE(IA64_PT_REGS_F11_OFFSET, offsetof (struct pt_regs, f11));

	BLANK();

	DEFINE(IA64_SWITCH_STACK_CALLER_UNAT_OFFSET, offsetof (struct switch_stack, caller_unat));
	DEFINE(IA64_SWITCH_STACK_AR_FPSR_OFFSET, offsetof (struct switch_stack, ar_fpsr));
	DEFINE(IA64_SWITCH_STACK_F2_OFFSET, offsetof (struct switch_stack, f2));
	DEFINE(IA64_SWITCH_STACK_F3_OFFSET, offsetof (struct switch_stack, f3));
	DEFINE(IA64_SWITCH_STACK_F4_OFFSET, offsetof (struct switch_stack, f4));
	DEFINE(IA64_SWITCH_STACK_F5_OFFSET, offsetof (struct switch_stack, f5));
	DEFINE(IA64_SWITCH_STACK_F12_OFFSET, offsetof (struct switch_stack, f12));
	DEFINE(IA64_SWITCH_STACK_F13_OFFSET, offsetof (struct switch_stack, f13));
	DEFINE(IA64_SWITCH_STACK_F14_OFFSET, offsetof (struct switch_stack, f14));
	DEFINE(IA64_SWITCH_STACK_F15_OFFSET, offsetof (struct switch_stack, f15));
	DEFINE(IA64_SWITCH_STACK_F16_OFFSET, offsetof (struct switch_stack, f16));
	DEFINE(IA64_SWITCH_STACK_F17_OFFSET, offsetof (struct switch_stack, f17));
	DEFINE(IA64_SWITCH_STACK_F18_OFFSET, offsetof (struct switch_stack, f18));
	DEFINE(IA64_SWITCH_STACK_F19_OFFSET, offsetof (struct switch_stack, f19));
	DEFINE(IA64_SWITCH_STACK_F20_OFFSET, offsetof (struct switch_stack, f20));
	DEFINE(IA64_SWITCH_STACK_F21_OFFSET, offsetof (struct switch_stack, f21));
	DEFINE(IA64_SWITCH_STACK_F22_OFFSET, offsetof (struct switch_stack, f22));
	DEFINE(IA64_SWITCH_STACK_F23_OFFSET, offsetof (struct switch_stack, f23));
	DEFINE(IA64_SWITCH_STACK_F24_OFFSET, offsetof (struct switch_stack, f24));
	DEFINE(IA64_SWITCH_STACK_F25_OFFSET, offsetof (struct switch_stack, f25));
	DEFINE(IA64_SWITCH_STACK_F26_OFFSET, offsetof (struct switch_stack, f26));
	DEFINE(IA64_SWITCH_STACK_F27_OFFSET, offsetof (struct switch_stack, f27));
	DEFINE(IA64_SWITCH_STACK_F28_OFFSET, offsetof (struct switch_stack, f28));
	DEFINE(IA64_SWITCH_STACK_F29_OFFSET, offsetof (struct switch_stack, f29));
	DEFINE(IA64_SWITCH_STACK_F30_OFFSET, offsetof (struct switch_stack, f30));
	DEFINE(IA64_SWITCH_STACK_F31_OFFSET, offsetof (struct switch_stack, f31));
	DEFINE(IA64_SWITCH_STACK_R4_OFFSET, offsetof (struct switch_stack, r4));
	DEFINE(IA64_SWITCH_STACK_R5_OFFSET, offsetof (struct switch_stack, r5));
	DEFINE(IA64_SWITCH_STACK_R6_OFFSET, offsetof (struct switch_stack, r6));
	DEFINE(IA64_SWITCH_STACK_R7_OFFSET, offsetof (struct switch_stack, r7));
	DEFINE(IA64_SWITCH_STACK_B0_OFFSET, offsetof (struct switch_stack, b0));
	DEFINE(IA64_SWITCH_STACK_B1_OFFSET, offsetof (struct switch_stack, b1));
	DEFINE(IA64_SWITCH_STACK_B2_OFFSET, offsetof (struct switch_stack, b2));
	DEFINE(IA64_SWITCH_STACK_B3_OFFSET, offsetof (struct switch_stack, b3));
	DEFINE(IA64_SWITCH_STACK_B4_OFFSET, offsetof (struct switch_stack, b4));
	DEFINE(IA64_SWITCH_STACK_B5_OFFSET, offsetof (struct switch_stack, b5));
	DEFINE(IA64_SWITCH_STACK_AR_PFS_OFFSET, offsetof (struct switch_stack, ar_pfs));
	DEFINE(IA64_SWITCH_STACK_AR_LC_OFFSET, offsetof (struct switch_stack, ar_lc));
	DEFINE(IA64_SWITCH_STACK_AR_UNAT_OFFSET, offsetof (struct switch_stack, ar_unat));
	DEFINE(IA64_SWITCH_STACK_AR_RNAT_OFFSET, offsetof (struct switch_stack, ar_rnat));
	DEFINE(IA64_SWITCH_STACK_AR_BSPSTORE_OFFSET, offsetof (struct switch_stack, ar_bspstore));
	DEFINE(IA64_SWITCH_STACK_PR_OFFSET, offsetof (struct switch_stack, pr));

	BLANK();

	DEFINE(IA64_SIGCONTEXT_IP_OFFSET, offsetof (struct sigcontext, sc_ip));
	DEFINE(IA64_SIGCONTEXT_AR_BSP_OFFSET, offsetof (struct sigcontext, sc_ar_bsp));
	DEFINE(IA64_SIGCONTEXT_AR_FPSR_OFFSET, offsetof (struct sigcontext, sc_ar_fpsr));
	DEFINE(IA64_SIGCONTEXT_AR_RNAT_OFFSET, offsetof (struct sigcontext, sc_ar_rnat));
	DEFINE(IA64_SIGCONTEXT_AR_UNAT_OFFSET, offsetof (struct sigcontext, sc_ar_unat));
	DEFINE(IA64_SIGCONTEXT_B0_OFFSET, offsetof (struct sigcontext, sc_br[0]));
	DEFINE(IA64_SIGCONTEXT_CFM_OFFSET, offsetof (struct sigcontext, sc_cfm));
	DEFINE(IA64_SIGCONTEXT_FLAGS_OFFSET, offsetof (struct sigcontext, sc_flags));
	DEFINE(IA64_SIGCONTEXT_FR6_OFFSET, offsetof (struct sigcontext, sc_fr[6]));
	DEFINE(IA64_SIGCONTEXT_PR_OFFSET, offsetof (struct sigcontext, sc_pr));
	DEFINE(IA64_SIGCONTEXT_R12_OFFSET, offsetof (struct sigcontext, sc_gr[12]));
	DEFINE(IA64_SIGCONTEXT_RBS_BASE_OFFSET,offsetof (struct sigcontext, sc_rbs_base));
	DEFINE(IA64_SIGCONTEXT_LOADRS_OFFSET, offsetof (struct sigcontext, sc_loadrs));

	BLANK();

	DEFINE(IA64_SIGPENDING_SIGNAL_OFFSET, offsetof (struct sigpending, signal));

	BLANK();

	DEFINE(IA64_SIGFRAME_ARG0_OFFSET, offsetof (struct sigframe, arg0));
	DEFINE(IA64_SIGFRAME_ARG1_OFFSET, offsetof (struct sigframe, arg1));
	DEFINE(IA64_SIGFRAME_ARG2_OFFSET, offsetof (struct sigframe, arg2));
	DEFINE(IA64_SIGFRAME_HANDLER_OFFSET, offsetof (struct sigframe, handler));
	DEFINE(IA64_SIGFRAME_SIGCONTEXT_OFFSET, offsetof (struct sigframe, sc));
	BLANK();
    /* for assembly files which can't include sched.h: */
	DEFINE(IA64_CLONE_VFORK, CLONE_VFORK);
	DEFINE(IA64_CLONE_VM, CLONE_VM);

	BLANK();
	DEFINE(IA64_CPUINFO_NSEC_PER_CYC_OFFSET,
	       offsetof (struct cpuinfo_ia64, nsec_per_cyc));
	DEFINE(IA64_CPUINFO_PTCE_BASE_OFFSET,
	       offsetof (struct cpuinfo_ia64, ptce_base));
	DEFINE(IA64_CPUINFO_PTCE_COUNT_OFFSET,
	       offsetof (struct cpuinfo_ia64, ptce_count));
	DEFINE(IA64_CPUINFO_PTCE_STRIDE_OFFSET,
	       offsetof (struct cpuinfo_ia64, ptce_stride));
	BLANK();
	DEFINE(IA64_TIMESPEC_TV_NSEC_OFFSET,
	       offsetof (struct timespec, tv_nsec));

	DEFINE(CLONE_SETTLS_BIT, 19);
#if CLONE_SETTLS != (1<<19)
# error "CLONE_SETTLS_BIT incorrect, please fix"
#endif

	BLANK();
	DEFINE(IA64_MCA_CPU_MCA_STACK_OFFSET,
	       offsetof (struct ia64_mca_cpu, mca_stack));
	DEFINE(IA64_MCA_CPU_INIT_STACK_OFFSET,
	       offsetof (struct ia64_mca_cpu, init_stack));
	BLANK();
	DEFINE(IA64_SAL_OS_STATE_OS_GP_OFFSET,
	       offsetof (struct ia64_sal_os_state, os_gp));
	DEFINE(IA64_SAL_OS_STATE_PROC_STATE_PARAM_OFFSET,
	       offsetof (struct ia64_sal_os_state, proc_state_param));
	DEFINE(IA64_SAL_OS_STATE_SAL_RA_OFFSET,
	       offsetof (struct ia64_sal_os_state, sal_ra));
	DEFINE(IA64_SAL_OS_STATE_SAL_GP_OFFSET,
	       offsetof (struct ia64_sal_os_state, sal_gp));
	DEFINE(IA64_SAL_OS_STATE_PAL_MIN_STATE_OFFSET,
	       offsetof (struct ia64_sal_os_state, pal_min_state));
	DEFINE(IA64_SAL_OS_STATE_OS_STATUS_OFFSET,
	       offsetof (struct ia64_sal_os_state, os_status));
	DEFINE(IA64_SAL_OS_STATE_CONTEXT_OFFSET,
	       offsetof (struct ia64_sal_os_state, context));
	DEFINE(IA64_SAL_OS_STATE_SIZE,
	       sizeof (struct ia64_sal_os_state));
	BLANK();

	DEFINE(IA64_PMSA_GR_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_gr));
	DEFINE(IA64_PMSA_BANK1_GR_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_bank1_gr));
	DEFINE(IA64_PMSA_PR_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_pr));
	DEFINE(IA64_PMSA_BR0_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_br0));
	DEFINE(IA64_PMSA_RSC_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_rsc));
	DEFINE(IA64_PMSA_IIP_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_iip));
	DEFINE(IA64_PMSA_IPSR_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_ipsr));
	DEFINE(IA64_PMSA_IFS_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_ifs));
	DEFINE(IA64_PMSA_XIP_OFFSET,
	       offsetof (struct pal_min_state_area_s, pmsa_xip));
	BLANK();

	/* used by fsys_gettimeofday in arch/ia64/kernel/fsys.S */
	DEFINE(IA64_GTOD_SEQ_OFFSET,
	       offsetof (struct fsyscall_gtod_data_t, seq));
	DEFINE(IA64_GTOD_WALL_TIME_OFFSET,
		offsetof (struct fsyscall_gtod_data_t, wall_time));
	DEFINE(IA64_GTOD_MONO_TIME_OFFSET,
		offsetof (struct fsyscall_gtod_data_t, monotonic_time));
	DEFINE(IA64_CLKSRC_MASK_OFFSET,
		offsetof (struct fsyscall_gtod_data_t, clk_mask));
	DEFINE(IA64_CLKSRC_MULT_OFFSET,
		offsetof (struct fsyscall_gtod_data_t, clk_mult));
	DEFINE(IA64_CLKSRC_SHIFT_OFFSET,
		offsetof (struct fsyscall_gtod_data_t, clk_shift));
	DEFINE(IA64_CLKSRC_MMIO_OFFSET,
		offsetof (struct fsyscall_gtod_data_t, clk_fsys_mmio));
	DEFINE(IA64_CLKSRC_CYCLE_LAST_OFFSET,
		offsetof (struct fsyscall_gtod_data_t, clk_cycle_last));
	DEFINE(IA64_ITC_JITTER_OFFSET,
		offsetof (struct itc_jitter_data_t, itc_jitter));
	DEFINE(IA64_ITC_LASTCYCLE_OFFSET,
		offsetof (struct itc_jitter_data_t, itc_lastcycle));

}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 #include <linux/init.h>
#include <linux/types.h>
#include <linux/audit.h>
#include <asm/unistd.h>

static unsigned dir_class[] = {
#include <asm-generic/audit_dir_write.h>
~0U
};

static unsigned read_class[] = {
#include <asm-generic/audit_read.h>
~0U
};

static unsigned write_class[] = {
#include <asm-generic/audit_write.h>
~0U
};

static unsigned chattr_class[] = {
#include <asm-generic/audit_change_attr.h>
~0U
};

static unsigned signal_class[] = {
#include <asm-generic/audit_signal.h>
~0U
};

int audit_classify_arch(int arch)
{
	return 0;
}

int audit_classify_syscall(int abi, unsigned syscall)
{
	switch(syscall) {
	case __NR_open:
		return 2;
	case __NR_openat:
		return 3;
	case __NR_execve:
		return 5;
	default:
		return 0;
	}
}

static int __init audit_classes_init(void)
{
	audit_register_class(AUDIT_CLASS_WRITE, write_class);
	audit_register_class(AUDIT_CLASS_READ, read_class);
	audit_register_class(AUDIT_CLASS_DIR_WRITE, dir_class);
	audit_register_class(AUDIT_CLASS_CHATTR, chattr_class);
	audit_register_class(AUDIT_CLASS_SIGNAL, signal_class);
	return 0;
}

__initcall(audit_classes_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
 *  Emulation of the "brl" instruction for IA64 processors that
 *  don't support it in hardware.
 *  Author: Stephan Zeisset, Intel Corp. <Stephan.Zeisset@intel.com>
 *
 *    02/22/02	D. Mosberger	Clear si_flgs, si_isr, and si_imm to avoid
 *				leaking kernel bits.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/processor.h>

extern char ia64_set_b1, ia64_set_b2, ia64_set_b3, ia64_set_b4, ia64_set_b5;

struct illegal_op_return {
	unsigned long fkt, arg1, arg2, arg3;
};

/*
 *  The unimplemented bits of a virtual address must be set
 *  to the value of the most significant implemented bit.
 *  unimpl_va_mask includes all unimplemented bits and
 *  the most significant implemented bit, so the result
 *  of an and operation with the mask must be all 0's
 *  or all 1's for the address to be valid.
 */
#define unimplemented_virtual_address(va) (						\
	((va) & local_cpu_data->unimpl_va_mask) != 0 &&					\
	((va) & local_cpu_data->unimpl_va_mask) != local_cpu_data->unimpl_va_mask	\
)

/*
 *  The unimplemented bits of a physical address must be 0.
 *  unimpl_pa_mask includes all unimplemented bits, so the result
 *  of an and operation with the mask must be all 0's for the
 *  address to be valid.
 */
#define unimplemented_physical_address(pa) (		\
	((pa) & local_cpu_data->unimpl_pa_mask) != 0	\
)

/*
 *  Handle an illegal operation fault that was caused by an
 *  unimplemented "brl" instruction.
 *  If we are not successful (e.g because the illegal operation
 *  wasn't caused by a "brl" after all), we return -1.
 *  If we are successful, we return either 0 or the address
 *  of a "fixup" function for manipulating preserved register
 *  state.
 */

struct illegal_op_return
ia64_emulate_brl (struct pt_regs *regs, unsigned long ar_ec)
{
	unsigned long bundle[2];
	unsigned long opcode, btype, qp, offset, cpl;
	unsigned long next_ip;
	struct siginfo siginfo;
	struct illegal_op_return rv;
	long tmp_taken, unimplemented_address;

	rv.fkt = (unsigned long) -1;

	/*
	 *  Decode the instruction bundle.
	 */

	if (copy_from_user(bundle, (void *) (regs->cr_iip), sizeof(bundle)))
		return rv;

	next_ip = (unsigned long) regs->cr_iip + 16;

	/* "brl" must be in slot 2. */
	if (ia64_psr(regs)->ri != 1) return rv;

	/* Must be "mlx" template */
	if ((bundle[0] & 0x1e) != 0x4) return rv;

	opcode = (bundle[1] >> 60);
	btype = ((bundle[1] >> 29) & 0x7);
	qp = ((bundle[1] >> 23) & 0x3f);
	offset = ((bundle[1] & 0x0800000000000000L) << 4)
		| ((bundle[1] & 0x00fffff000000000L) >> 32)
		| ((bundle[1] & 0x00000000007fffffL) << 40)
		| ((bundle[0] & 0xffff000000000000L) >> 24);

	tmp_taken = regs->pr & (1L << qp);

	switch(opcode) {

		case 0xC:
			/*
			 *  Long Branch.
			 */
			if (btype != 0) return rv;
			rv.fkt = 0;
			if (!(tmp_taken)) {
				/*
				 *  Qualifying predicate is 0.
				 *  Skip instruction.
				 */
				regs->cr_iip = next_ip;
				ia64_psr(regs)->ri = 0;
				return rv;
			}
			break;

		case 0xD:
			/*
			 *  Long Call.
			 */
			rv.fkt = 0;
			if (!(tmp_taken)) {
				/*
				 *  Qualifying predicate is 0.
				 *  Skip instruction.
				 */
				regs->cr_iip = next_ip;
				ia64_psr(regs)->ri = 0;
				return rv;
			}

			/*
			 *  BR[btype] = IP+16
			 */
			switch(btype) {
				case 0:
					regs->b0 = next_ip;
					break;
				case 1:
					rv.fkt = (unsigned long) &ia64_set_b1;
					break;
				case 2:
					rv.fkt = (unsigned long) &ia64_set_b2;
					break;
				case 3:
					rv.fkt = (unsigned long) &ia64_set_b3;
					break;
				case 4:
					rv.fkt = (unsigned long) &ia64_set_b4;
					break;
				case 5:
					rv.fkt = (unsigned long) &ia64_set_b5;
					break;
				case 6:
					regs->b6 = next_ip;
					break;
				case 7:
					regs->b7 = next_ip;
					break;
			}
			rv.arg1 = next_ip;

			/*
			 *  AR[PFS].pfm = CFM
			 *  AR[PFS].pec = AR[EC]
			 *  AR[PFS].ppl = PSR.cpl
			 */
			cpl = ia64_psr(regs)->cpl;
			regs->ar_pfs = ((regs->cr_ifs & 0x3fffffffff)
					| (ar_ec << 52) | (cpl << 62));

			/*
			 *  CFM.sof -= CFM.sol
			 *  CFM.sol = 0
			 *  CFM.sor = 0
			 *  CFM.rrb.gr = 0
			 *  CFM.rrb.fr = 0
			 *  CFM.rrb.pr = 0
			 */
			regs->cr_ifs = ((regs->cr_ifs & 0xffffffc00000007f)
					- ((regs->cr_ifs >> 7) & 0x7f));

			break;

		default:
			/*
			 *  Unknown opcode.
			 */
			return rv;

	}

	regs->cr_iip += offset;
	ia64_psr(regs)->ri = 0;

	if (ia64_psr(regs)->it == 0)
		unimplemented_address = unimplemented_physical_address(regs->cr_iip);
	else
		unimplemented_address = unimplemented_virtual_address(regs->cr_iip);

	if (unimplemented_address) {
		/*
		 *  The target address contains unimplemented bits.
		 */
		printk(KERN_DEBUG "Woah! Unimplemented Instruction Address Trap!\n");
		siginfo.si_signo = SIGILL;
		siginfo.si_errno = 0;
		siginfo.si_flags = 0;
		siginfo.si_isr = 0;
		siginfo.si_imm = 0;
		siginfo.si_code = ILL_BADIADDR;
		force_sig_info(SIGILL, &siginfo, current);
	} else if (ia64_psr(regs)->tb) {
		/*
		 *  Branch Tracing is enabled.
		 *  Force a taken branch signal.
		 */
		siginfo.si_signo = SIGTRAP;
		siginfo.si_errno = 0;
		siginfo.si_code = TRAP_BRANCH;
		siginfo.si_flags = 0;
		siginfo.si_isr = 0;
		siginfo.si_addr = 0;
		siginfo.si_imm = 0;
		force_sig_info(SIGTRAP, &siginfo, current);
	} else if (ia64_psr(regs)->ss) {
		/*
		 *  Single Step is enabled.
		 *  Force a trace signal.
		 */
		siginfo.si_signo = SIGTRAP;
		siginfo.si_errno = 0;
		siginfo.si_code = TRAP_TRACE;
		siginfo.si_flags = 0;
		siginfo.si_isr = 0;
		siginfo.si_addr = 0;
		siginfo.si_imm = 0;
		force_sig_info(SIGTRAP, &siginfo, current);
	}
	return rv;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * arch/ia64/kernel/crash.c
 *
 * Architecture specific (ia64) functions for kexec based crash dumps.
 *
 * Created by: Khalid Aziz <khalid.aziz@hp.com>
 * Copyright (C) 2005 Hewlett-Packard Development Company, L.P.
 * Copyright (C) 2005 Intel Corp	Zou Nan hai <nanhai.zou@intel.com>
 *
 */
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/crash_dump.h>
#include <linux/bootmem.h>
#include <linux/kexec.h>
#include <linux/elfcore.h>
#include <linux/sysctl.h>
#include <linux/init.h>
#include <linux/kdebug.h>

#include <asm/mca.h>

int kdump_status[NR_CPUS];
static atomic_t kdump_cpu_frozen;
atomic_t kdump_in_progress;
static int kdump_freeze_monarch;
static int kdump_on_init = 1;
static int kdump_on_fatal_mca = 1;

static inline Elf64_Word
*append_elf_note(Elf64_Word *buf, char *name, unsigned type, void *data,
		size_t data_len)
{
	struct elf_note *note = (struct elf_note *)buf;
	note->n_namesz = strlen(name) + 1;
	note->n_descsz = data_len;
	note->n_type   = type;
	buf += (sizeof(*note) + 3)/4;
	memcpy(buf, name, note->n_namesz);
	buf += (note->n_namesz + 3)/4;
	memcpy(buf, data, data_len);
	buf += (data_len + 3)/4;
	return buf;
}

static void
final_note(void *buf)
{
	memset(buf, 0, sizeof(struct elf_note));
}

extern void ia64_dump_cpu_regs(void *);

static DEFINE_PER_CPU(struct elf_prstatus, elf_prstatus);

void
crash_save_this_cpu(void)
{
	void *buf;
	unsigned long cfm, sof, sol;

	int cpu = smp_processor_id();
	struct elf_prstatus *prstatus = &per_cpu(elf_prstatus, cpu);

	elf_greg_t *dst = (elf_greg_t *)&(prstatus->pr_reg);
	memset(prstatus, 0, sizeof(*prstatus));
	prstatus->pr_pid = current->pid;

	ia64_dump_cpu_regs(dst);
	cfm = dst[43];
	sol = (cfm >> 7) & 0x7f;
	sof = cfm & 0x7f;
	dst[46] = (unsigned long)ia64_rse_skip_regs((unsigned long *)dst[46],
			sof - sol);

	buf = (u64 *) per_cpu_ptr(crash_notes, cpu);
	if (!buf)
		return;
	buf = append_elf_note(buf, KEXEC_CORE_NOTE_NAME, NT_PRSTATUS, prstatus,
			sizeof(*prstatus));
	final_note(buf);
}

#ifdef CONFIG_SMP
static int
kdump_wait_cpu_freeze(void)
{
	int cpu_num = num_online_cpus() - 1;
	int timeout = 1000;
	while(timeout-- > 0) {
		if (atomic_read(&kdump_cpu_frozen) == cpu_num)
			return 0;
		udelay(1000);
	}
	return 1;
}
#endif

void
machine_crash_shutdown(struct pt_regs *pt)
{
	/* This function is only called after the system
	 * has paniced or is otherwise in a critical state.
	 * The minimum amount of code to allow a kexec'd kernel
	 * to run successfully needs to happen here.
	 *
	 * In practice this means shooting down the other cpus in
	 * an SMP system.
	 */
	kexec_disable_iosapic();
#ifdef CONFIG_SMP
	/*
	 * If kdump_on_init is set and an INIT is asserted here, kdump will
	 * be started again via INIT monarch.
	 */
	local_irq_disable();
	ia64_set_psr_mc();	/* mask MCA/INIT */
	if (atomic_inc_return(&kdump_in_progress) != 1)
		unw_init_running(kdump_cpu_freeze, NULL);

	/*
	 * Now this cpu is ready for kdump.
	 * Stop all others by IPI or INIT.  They could receive INIT from
	 * outside and might be INIT monarch, but only thing they have to
	 * do is falling into kdump_cpu_freeze().
	 *
	 * If an INIT is asserted here:
	 * - All receivers might be slaves, since some of cpus could already
	 *   be frozen and INIT might be masked on monarch.  In this case,
	 *   all slaves will be frozen soon since kdump_in_progress will let
	 *   them into DIE_INIT_SLAVE_LEAVE.
	 * - One might be a monarch, but INIT rendezvous will fail since
	 *   at least this cpu already have INIT masked so it never join
	 *   to the rendezvous.  In this case, all slaves and monarch will
	 *   be frozen soon with no wait since the INIT rendezvous is skipped
	 *   by kdump_in_progress.
	 */
	kdump_smp_send_stop();
	/* not all cpu response to IPI, send INIT to freeze them */
	if (kdump_wait_cpu_freeze()) {
		kdump_smp_send_init();
		/* wait again, don't go ahead if possible */
		kdump_wait_cpu_freeze();
	}
#endif
}

static void
machine_kdump_on_init(void)
{
	crash_save_vmcoreinfo();
	local_irq_disable();
	kexec_disable_iosapic();
	machine_kexec(ia64_kimage);
}

void
kdump_cpu_freeze(struct unw_frame_info *info, void *arg)
{
	int cpuid;

	local_irq_disable();
	cpuid = smp_processor_id();
	crash_save_this_cpu();
	current->thread.ksp = (__u64)info->sw - 16;

	ia64_set_psr_mc();	/* mask MCA/INIT and stop reentrance */

	atomic_inc(&kdump_cpu_frozen);
	kdump_status[cpuid] = 1;
	mb();
	for (;;)
		cpu_relax();
}

static int
kdump_init_notifier(struct notifier_block *self, unsigned long val, void *data)
{
	struct ia64_mca_notify_die *nd;
	struct die_args *args = data;

	if (atomic_read(&kdump_in_progress)) {
		switch (val) {
		case DIE_INIT_MONARCH_LEAVE:
			if (!kdump_freeze_monarch)
				break;
			/* fall through */
		case DIE_INIT_SLAVE_LEAVE:
		case DIE_INIT_MONARCH_ENTER:
		case DIE_MCA_RENDZVOUS_LEAVE:
			unw_init_running(kdump_cpu_freeze, NULL);
			break;
		}
	}

	if (!kdump_on_init && !kdump_on_fatal_mca)
		return NOTIFY_DONE;

	if (!ia64_kimage) {
		if (val == DIE_INIT_MONARCH_LEAVE)
			ia64_mca_printk(KERN_NOTICE
					"%s: kdump not configured\n",
					__func__);
		return NOTIFY_DONE;
	}

	if (val != DIE_INIT_MONARCH_LEAVE &&
	    val != DIE_INIT_MONARCH_PROCESS &&
	    val != DIE_MCA_MONARCH_LEAVE)
		return NOTIFY_DONE;

	nd = (struct ia64_mca_notify_die *)args->err;

	switch (val) {
	case DIE_INIT_MONARCH_PROCESS:
		/* Reason code 1 means machine check rendezvous*/
		if (kdump_on_init && (nd->sos->rv_rc != 1)) {
			if (atomic_inc_return(&kdump_in_progress) != 1)
				kdump_freeze_monarch = 1;
		}
		break;
	case DIE_INIT_MONARCH_LEAVE:
		/* Reason code 1 means machine check rendezvous*/
		if (kdump_on_init && (nd->sos->rv_rc != 1))
			machine_kdump_on_init();
		break;
	case DIE_MCA_MONARCH_LEAVE:
		/* *(nd->data) indicate if MCA is recoverable */
		if (kdump_on_fatal_mca && !(*(nd->data))) {
			if (atomic_inc_return(&kdump_in_progress) == 1)
				machine_kdump_on_init();
			/* We got fatal MCA while kdump!? No way!! */
		}
		break;
	}
	return NOTIFY_DONE;
}

#ifdef CONFIG_SYSCTL
static struct ctl_table kdump_ctl_table[] = {
	{
		.procname = "kdump_on_init",
		.data = &kdump_on_init,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "kdump_on_fatal_mca",
		.data = &kdump_on_fatal_mca,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{ }
};

static struct ctl_table sys_table[] = {
	{
	  .procname = "kernel",
	  .mode = 0555,
	  .child = kdump_ctl_table,
	},
	{ }
};
#endif

static int
machine_crash_setup(void)
{
	/* be notified before default_monarch_init_process */
	static struct notifier_block kdump_init_notifier_nb = {
		.notifier_call = kdump_init_notifier,
		.priority = 1,
	};
	int ret;
	if((ret = register_die_notifier(&kdump_init_notifier_nb)) != 0)
		return ret;
#ifdef CONFIG_SYSCTL
	register_sysctl_table(sys_table);
#endif
	return 0;
}

__initcall(machine_crash_setup);

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 *	kernel/crash_dump.c - Memory preserving reboot related code.
 *
 *	Created by: Simon Horman <horms@verge.net.au>
 *	Original code moved from kernel/crash.c
 *	Original code comment copied from the i386 version of this file
 */

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/crash_dump.h>

#include <asm/page.h>
#include <asm/uaccess.h>

/**
 * copy_oldmem_page - copy one page from "oldmem"
 * @pfn: page frame number to be copied
 * @buf: target memory address for the copy; this can be in kernel address
 *	space or user address space (see @userbuf)
 * @csize: number of bytes to copy
 * @offset: offset in bytes into the page (based on pfn) to begin the copy
 * @userbuf: if set, @buf is in user address space, use copy_to_user(),
 *	otherwise @buf is in kernel address space, use memcpy().
 *
 * Copy a page from "oldmem". For this page, there is no pte mapped
 * in the current kernel. We stitch up a pte, similar to kmap_atomic.
 *
 * Calling copy_to_user() in atomic context is not desirable. Hence first
 * copying the data to a pre-allocated kernel page and then copying to user
 * space in non-atomic context.
 */
ssize_t
copy_oldmem_page(unsigned long pfn, char *buf,
		size_t csize, unsigned long offset, int userbuf)
{
	void  *vaddr;

	if (!csize)
		return 0;
	vaddr = __va(pfn<<PAGE_SHIFT);
	if (userbuf) {
		if (copy_to_user(buf, (vaddr + offset), csize)) {
			return -EFAULT;
		}
	} else
		memcpy(buf, (vaddr + offset), csize);
	return csize;
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                #include <linux/module.h>
#include <linux/smp.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/timex.h>
#include <linux/clocksource.h>
#include <linux/io.h>

/* IBM Summit (EXA) Cyclone counter code*/
#define CYCLONE_CBAR_ADDR 0xFEB00CD0
#define CYCLONE_PMCC_OFFSET 0x51A0
#define CYCLONE_MPMC_OFFSET 0x51D0
#define CYCLONE_MPCS_OFFSET 0x51A8
#define CYCLONE_TIMER_FREQ 100000000

int use_cyclone;
void __init cyclone_setup(void)
{
	use_cyclone = 1;
}

static void __iomem *cyclone_mc;

static cycle_t read_cyclone(struct clocksource *cs)
{
	return (cycle_t)readq((void __iomem *)cyclone_mc);
}

static struct clocksource clocksource_cyclone = {
        .name           = "cyclone",
        .rating         = 300,
        .read           = read_cyclone,
        .mask           = (1LL << 40) - 1,
        .flags          = CLOCK_SOURCE_IS_CONTINUOUS,
};

int __init init_cyclone_clock(void)
{
	u64 __iomem *reg;
	u64 base;	/* saved cyclone base address */
	u64 offset;	/* offset from pageaddr to cyclone_timer register */
	int i;
	u32 __iomem *cyclone_timer;	/* Cyclone MPMC0 register */

	if (!use_cyclone)
		return 0;

	printk(KERN_INFO "Summit chipset: Starting Cyclone Counter.\n");

	/* find base address */
	offset = (CYCLONE_CBAR_ADDR);
	reg = ioremap_nocache(offset, sizeof(u64));
	if(!reg){
		printk(KERN_ERR "Summit chipset: Could not find valid CBAR"
				" register.\n");
		use_cyclone = 0;
		return -ENODEV;
	}
	base = readq(reg);
	iounmap(reg);
	if(!base){
		printk(KERN_ERR "Summit chipset: Could not find valid CBAR"
				" value.\n");
		use_cyclone = 0;
		return -ENODEV;
	}

	/* setup PMCC */
	offset = (base + CYCLONE_PMCC_OFFSET);
	reg = ioremap_nocache(offset, sizeof(u64));
	if(!reg){
		printk(KERN_ERR "Summit chipset: Could not find valid PMCC"
				" register.\n");
		use_cyclone = 0;
		return -ENODEV;
	}
	writel(0x00000001,reg);
	iounmap(reg);

	/* setup MPCS */
	offset = (base + CYCLONE_MPCS_OFFSET);
	reg = ioremap_nocache(offset, sizeof(u64));
	if(!reg){
		printk(KERN_ERR "Summit chipset: Could not find valid MPCS"
				" register.\n");
		use_cyclone = 0;
		return -ENODEV;
	}
	writel(0x00000001,reg);
	iounmap(reg);

	/* map in cyclone_timer */
	offset = (base + CYCLONE_MPMC_OFFSET);
	cyclone_timer = ioremap_nocache(offset, sizeof(u32));
	if(!cyclone_timer){
		printk(KERN_ERR "Summit chipset: Could not find valid MPMC"
				" register.\n");
		use_cyclone = 0;
		return -ENODEV;
	}

	/*quick test to make sure its ticking*/
	for(i=0; i<3; i++){
		u32 old = readl(cyclone_timer);
		int stall = 100;
		while(stall--) barrier();
		if(readl(cyclone_timer) == old){
			printk(KERN_ERR "Summit chipset: Counter not counting!"
					" DISABLED\n");
			iounmap(cyclone_timer);
			cyclone_timer = NULL;
			use_cyclone = 0;
			return -ENODEV;
		}
	}
	/* initialize last tick */
	cyclone_mc = cyclone_timer;
	clocksource_cyclone.archdata.fsys_mmio = cyclone_timer;
	clocksource_register_hz(&clocksource_cyclone, CYCLONE_TIMER_FREQ);

	return 0;
}

__initcall(init_cyclone_clock);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     #include <linux/dma-mapping.h>
#include <linux/export.h>

/* Set this to 1 if there is a HW IOMMU in the system */
int iommu_detected __read_mostly;

struct dma_map_ops *dma_ops;
EXPORT_SYMBOL(dma_ops);

#define PREALLOC_DMA_DEBUG_ENTRIES (1 << 16)

static int __init dma_init(void)
{
	dma_debug_init(PREALLOC_DMA_DEBUG_ENTRIES);

	return 0;
}
fs_initcall(dma_init);

struct dma_map_ops *dma_get_ops(struct device *dev)
{
	return dma_ops;
}
EXPORT_SYMBOL(dma_get_ops);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Extensible Firmware Interface
 *
 * Based on Extensible Firmware Interface Specification version 0.9
 * April 30, 1999
 *
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 * Copyright (C) 1999-2003 Hewlett-Packard Co.
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 * (c) Copyright 2006 Hewlett-Packard Development Company, L.P.
 *	Bjorn Helgaas <bjorn.helgaas@hp.com>
 *
 * All EFI Runtime Services are not implemented yet as EFI only
 * supports physical mode addressing on SoftSDV. This is to be fixed
 * in a future version.  --drummond 1999-07-20
 *
 * Implemented EFI runtime services and virtual mode calls.  --davidm
 *
 * Goutham Rao: <goutham.rao@intel.com>
 *	Skip non-WB memory and ignore empty memory ranges.
 */
#include <linux/module.h>
#include <linux/bootmem.h>
#include <linux/crash_dump.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/efi.h>
#include <linux/kexec.h>
#include <linux/mm.h>

#include <asm/io.h>
#include <asm/kregs.h>
#include <asm/meminit.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/mca.h>
#include <asm/setup.h>
#include <asm/tlbflush.h>

#define EFI_DEBUG	0

static __initdata unsigned long palo_phys;

static __initdata efi_config_table_type_t arch_tables[] = {
	{PROCESSOR_ABSTRACTION_LAYER_OVERWRITE_GUID, "PALO", &palo_phys},
	{NULL_GUID, NULL, 0},
};

extern efi_status_t efi_call_phys (void *, ...);

static efi_runtime_services_t *runtime;
static u64 mem_limit = ~0UL, max_addr = ~0UL, min_addr = 0UL;

#define efi_call_virt(f, args...)	(*(f))(args)

#define STUB_GET_TIME(prefix, adjust_arg)				       \
static efi_status_t							       \
prefix##_get_time (efi_time_t *tm, efi_time_cap_t *tc)			       \
{									       \
	struct ia64_fpreg fr[6];					       \
	efi_time_cap_t *atc = NULL;					       \
	efi_status_t ret;						       \
									       \
	if (tc)								       \
		atc = adjust_arg(tc);					       \
	ia64_save_scratch_fpregs(fr);					       \
	ret = efi_call_##prefix((efi_get_time_t *) __va(runtime->get_time),    \
				adjust_arg(tm), atc);			       \
	ia64_load_scratch_fpregs(fr);					       \
	return ret;							       \
}

#define STUB_SET_TIME(prefix, adjust_arg)				       \
static efi_status_t							       \
prefix##_set_time (efi_time_t *tm)					       \
{									       \
	struct ia64_fpreg fr[6];					       \
	efi_status_t ret;						       \
									       \
	ia64_save_scratch_fpregs(fr);					       \
	ret = efi_call_##prefix((efi_set_time_t *) __va(runtime->set_time),    \
				adjust_arg(tm));			       \
	ia64_load_scratch_fpregs(fr);					       \
	return ret;							       \
}

#define STUB_GET_WAKEUP_TIME(prefix, adjust_arg)			       \
static efi_status_t							       \
prefix##_get_wakeup_time (efi_bool_t *enabled, efi_bool_t *pending,	       \
			  efi_time_t *tm)				       \
{									       \
	struct ia64_fpreg fr[6];					       \
	efi_status_t ret;						       \
									       \
	ia64_save_scratch_fpregs(fr);					       \
	ret = efi_call_##prefix(					       \
		(efi_get_wakeup_time_t *) __va(runtime->get_wakeup_time),      \
		adjust_arg(enabled), adjust_arg(pending), adjust_arg(tm));     \
	ia64_load_scratch_fpregs(fr);					       \
	return ret;							       \
}

#define STUB_SET_WAKEUP_TIME(prefix, adjust_arg)			       \
static efi_status_t							       \
prefix##_set_wakeup_time (efi_bool_t enabled, efi_time_t *tm)		       \
{									       \
	struct ia64_fpreg fr[6];					       \
	efi_time_t *atm = NULL;						       \
	efi_status_t ret;						       \
									       \
	if (tm)								       \
		atm = adjust_arg(tm);					       \
	ia64_save_scratch_fpregs(fr);					       \
	ret = efi_call_##prefix(					       \
		(efi_set_wakeup_time_t *) __va(runtime->set_wakeup_time),      \
		enabled, atm);						       \
	ia64_load_scratch_fpregs(fr);					       \
	return ret;							       \
}

#define STUB_GET_VARIABLE(prefix, adjust_arg)				       \
static efi_status_t							       \
prefix##_get_variable (efi_char16_t *name, efi_guid_t *vendor, u32 *attr,      \
		       unsigned long *data_size, void *data)		       \
{									       \
	struct ia64_fpreg fr[6];					       \
	u32 *aattr = NULL;						       \
	efi_status_t ret;						       \
									       \
	if (attr)							       \
		aattr = adjust_arg(attr);				       \
	ia64_save_scratch_fpregs(fr);					       \
	ret = efi_call_##prefix(					       \
		(efi_get_variable_t *) __va(runtime->get_variable),	       \
		adjust_arg(name), adjust_arg(vendor), aattr,		       \
		adjust_arg(data_size), adjust_arg(data));		       \
	ia64_load_scratch_fpregs(fr);					       \
	return ret;							       \
}

#define STUB_GET_NEXT_VARIABLE(prefix, adjust_arg)			       \
static efi_status_t							       \
prefix##_get_next_variable (unsigned long *name_size, efi_char16_t *name,      \
			    efi_guid_t *vendor)				       \
{									       \
	struct ia64_fpreg fr[6];					       \
	efi_status_t ret;						       \
									       \
	ia64_save_scratch_fpregs(fr);					       \
	ret = efi_call_##prefix(					       \
		(efi_get_next_variable_t *) __va(runtime->get_next_variable),  \
		adjust_arg(name_size), adjust_arg(name), adjust_arg(vendor));  \
	ia64_load_scratch_fpregs(fr);					       \
	return ret;							       \
}

#define STUB_SET_VARIABLE(prefix, adjust_arg)				       \
static efi_status_t							       \
prefix##_set_variable (efi_char16_t *name, efi_guid_t *vendor,		       \
		       u32 attr, unsigned long data_size,		       \
		       void *data)					       \
{									       \
	struct ia64_fpreg fr[6];					       \
	efi_status_t ret;						       \
									       \
	ia64_save_scratch_fpregs(fr);					       \
	ret = efi_call_##prefix(					       \
		(efi_set_variable_t *) __va(runtime->set_variable),	       \
		adjust_arg(name), adjust_arg(vendor), attr, data_size,	       \
		adjust_arg(data));					       \
	ia64_load_scratch_fpregs(fr);					       \
	return ret;							       \
}

#define STUB_GET_NEXT_HIGH_MONO_COUNT(prefix, adjust_arg)		       \
static efi_status_t							       \
prefix##_get_next_high_mono_count (u32 *count)				       \
{									       \
	struct ia64_fpreg fr[6];					       \
	efi_status_t ret;						       \
									       \
	ia64_save_scratch_fpregs(fr);					       \
	ret = efi_call_##prefix((efi_get_next_high_mono_count_t *)	       \
				__va(runtime->get_next_high_mono_count),       \
				adjust_arg(count));			       \
	ia64_load_scratch_fpregs(fr);					       \
	return ret;							       \
}

#define STUB_RESET_SYSTEM(prefix, adjust_arg)				       \
static void								       \
prefix##_reset_system (int reset_type, efi_status_t status,		       \
		       unsigned long data_size, efi_char16_t *data)	       \
{									       \
	struct ia64_fpreg fr[6];					       \
	efi_char16_t *adata = NULL;					       \
									       \
	if (data)							       \
		adata = adjust_arg(data);				       \
									       \
	ia64_save_scratch_fpregs(fr);					       \
	efi_call_##prefix(						       \
		(efi_reset_system_t *) __va(runtime->reset_system),	       \
		reset_type, status, data_size, adata);			       \
	/* should not return, but just in case... */			       \
	ia64_load_scratch_fpregs(fr);					       \
}

#define phys_ptr(arg)	((__typeof__(arg)) ia64_tpa(arg))

STUB_GET_TIME(phys, phys_ptr)
STUB_SET_TIME(phys, phys_ptr)
STUB_GET_WAKEUP_TIME(phys, phys_ptr)
STUB_SET_WAKEUP_TIME(phys, phys_ptr)
STUB_GET_VARIABLE(phys, phys_ptr)
STUB_GET_NEXT_VARIABLE(phys, phys_ptr)
STUB_SET_VARIABLE(phys, phys_ptr)
STUB_GET_NEXT_HIGH_MONO_COUNT(phys, phys_ptr)
STUB_RESET_SYSTEM(phys, phys_ptr)

#define id(arg)	arg

STUB_GET_TIME(virt, id)
STUB_SET_TIME(virt, id)
STUB_GET_WAKEUP_TIME(virt, id)
STUB_SET_WAKEUP_TIME(virt, id)
STUB_GET_VARIABLE(virt, id)
STUB_GET_NEXT_VARIABLE(virt, id)
STUB_SET_VARIABLE(virt, id)
STUB_GET_NEXT_HIGH_MONO_COUNT(virt, id)
STUB_RESET_SYSTEM(virt, id)

void
efi_gettimeofday (struct timespec *ts)
{
	efi_time_t tm;

	if ((*efi.get_time)(&tm, NULL) != EFI_SUCCESS) {
		memset(ts, 0, sizeof(*ts));
		return;
	}

	ts->tv_sec = mktime(tm.year, tm.month, tm.day,
			    tm.hour, tm.minute, tm.second);
	ts->tv_nsec = tm.nanosecond;
}

static int
is_memory_available (efi_memory_desc_t *md)
{
	if (!(md->attribute & EFI_MEMORY_WB))
		return 0;

	switch (md->type) {
	      case EFI_LOADER_CODE:
	      case EFI_LOADER_DATA:
	      case EFI_BOOT_SERVICES_CODE:
	      case EFI_BOOT_SERVICES_DATA:
	      case EFI_CONVENTIONAL_MEMORY:
		return 1;
	}
	return 0;
}

typedef struct kern_memdesc {
	u64 attribute;
	u64 