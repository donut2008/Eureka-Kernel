 system equips
	 * SRAT table, pxm is already found and node is ready.
  	 * So, just pxm_to_nid(pxm) is OK.
	 * This code here is for the system which doesn't have full SRAT
  	 * table for possible cpus.
	 */
	node_cpuid[cpu].phys_id = physid;
	node_cpuid[cpu].nid = acpi_get_node(handle);
#endif
	return 0;
}

int additional_cpus __initdata = -1;

static __init int setup_additional_cpus(char *s)
{
	if (s)
		additional_cpus = simple_strtol(s, NULL, 0);

	return 0;
}

early_param("additional_cpus", setup_additional_cpus);

/*
 * cpu_possible_mask should be static, it cannot change as CPUs
 * are onlined, or offlined. The reason is per-cpu data-structures
 * are allocated by some modules at init time, and dont expect to
 * do this dynamically on cpu arrival/departure.
 * cpu_present_mask on the other hand can change dynamically.
 * In case when cpu_hotplug is not compiled, then we resort to current
 * behaviour, which is cpu_possible == cpu_present.
 * - Ashok Raj
 *
 * Three ways to find out the number of additional hotplug CPUs:
 * - If the BIOS specified disabled CPUs in ACPI/mptables use that.
 * - The user can overwrite it with additional_cpus=NUM
 * - Otherwise don't reserve additional CPUs.
 */
__init void prefill_possible_map(void)
{
	int i;
	int possible, disabled_cpus;

	disabled_cpus = total_cpus - available_cpus;

 	if (additional_cpus == -1) {
 		if (disabled_cpus > 0)
			additional_cpus = disabled_cpus;
 		else
			additional_cpus = 0;
 	}

	possible = available_cpus + additional_cpus;

	if (possible > nr_cpu_ids)
		possible = nr_cpu_ids;

	printk(KERN_INFO "SMP: Allowing %d CPUs, %d hotplug CPUs\n",
		possible, max((possible - available_cpus), 0));

	for (i = 0; i < possible; i++)
		set_cpu_possible(i, true);
}

static int _acpi_map_lsapic(acpi_handle handle, int physid, int *pcpu)
{
	cpumask_t tmp_map;
	int cpu;

	cpumask_complement(&tmp_map, cpu_present_mask);
	cpu = cpumask_first(&tmp_map);
	if (cpu >= nr_cpu_ids)
		return -EINVAL;

	acpi_map_cpu2node(handle, cpu, physid);

	set_cpu_present(cpu, true);
	ia64_cpu_to_sapicid[cpu] = physid;

	acpi_processor_set_pdc(handle);

	*pcpu = cpu;
	return (0);
}

/* wrapper to silence section mismatch warning */
int __ref acpi_map_cpu(acpi_handle handle, phys_cpuid_t physid, int *pcpu)
{
	return _acpi_map_lsapic(handle, physid, pcpu);
}
EXPORT_SYMBOL(acpi_map_cpu);

int acpi_unmap_cpu(int cpu)
{
	ia64_cpu_to_sapicid[cpu] = -1;
	set_cpu_present(cpu, false);

#ifdef CONFIG_ACPI_NUMA
	/* NUMA specific cleanup's */
#endif

	return (0);
}
EXPORT_SYMBOL(acpi_unmap_cpu);
#endif				/* CONFIG_ACPI_HOTPLUG_CPU */

#ifdef CONFIG_ACPI_NUMA
static acpi_status acpi_map_iosapic(acpi_handle handle, u32 depth,
				    void *context, void **ret)
{
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *obj;
	struct acpi_madt_io_sapic *iosapic;
	unsigned int gsi_base;
	int node;

	/* Only care about objects w/ a method that returns the MADT */
	if (ACPI_FAILURE(acpi_evaluate_object(handle, "_MAT", NULL, &buffer)))
		return AE_OK;

	if (!buffer.length || !buffer.pointer)
		return AE_OK;

	obj = buffer.pointer;
	if (obj->type != ACPI_TYPE_BUFFER ||
	    obj->buffer.length < sizeof(*iosapic)) {
		kfree(buffer.pointer);
		return AE_OK;
	}

	iosapic = (struct acpi_madt_io_sapic *)obj->buffer.pointer;

	if (iosapic->header.type != ACPI_MADT_TYPE_IO_SAPIC) {
		kfree(buffer.pointer);
		return AE_OK;
	}

	gsi_base = iosapic->global_irq_base;

	kfree(buffer.pointer);

	/* OK, it's an IOSAPIC MADT entry; associate it with a node */
	node = acpi_get_node(handle);
	if (node == NUMA_NO_NODE || !node_online(node) ||
	    cpumask_empty(cpumask_of_node(node)))
		return AE_OK;

	/* We know a gsi to node mapping! */
	map_iosapic_to_node(gsi_base, node);
	return AE_OK;
}

static int __init
acpi_map_iosapics (void)
{
	acpi_get_devices(NULL, acpi_map_iosapic, NULL, NULL);
	return 0;
}

fs_initcall(acpi_map_iosapics);
#endif				/* CONFIG_ACPI_NUMA */

int __ref acpi_register_ioapic(acpi_handle handle, u64 phys_addr, u32 gsi_base)
{
	int err;

	if ((err = iosapic_init(phys_addr, gsi_base)))
		return err;

#ifdef CONFIG_ACPI_NUMA
	acpi_map_iosapic(handle, 0, NULL, NULL);
#endif				/* CONFIG_ACPI_NUMA */

	return 0;
}

EXPORT_SYMBOL(acpi_register_ioapic);

int acpi_unregister_ioapic(acpi_handle handle, u32 gsi_base)
{
	return iosapic_remove(gsi_base);
}

EXPORT_SYMBOL(acpi_unregister_ioapic);

/*
 * acpi_suspend_lowlevel() - save kernel state and suspend.
 *
 * TBD when when IA64 starts to support suspend...
 */
int acpi_suspend_lowlevel(void) { return 0; }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed
 * to extract and format the required data.
 */

#define ASM_OFFSETS_C 1

#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/clocksource.h>
#include <linux/kbuild.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/siginfo.h>
#include <asm/sigcontext.h>
#include <asm/mca.h>

#include "../kernel/sigframe.h"
#include "../kernel/fsyscall_gtod_data.h"

void foo(void)
{
	DEFINE(IA64_TASK_SIZE, sizeof (struct task_struct));
	DEFINE(IA64_THREAD_INFO_SIZE, sizeof (struct thread_info));
	DEFINE(IA64_PT_REGS_SIZE, sizeof (struct pt_regs));
	DEFINE(IA64_SWITCH_STACK_SIZE, sizeof (struct switch_stack));
	DEFINE(IA64_SIGINFO_SIZE, sizeof (struct siginfo));
	DEFINE(IA64_CPU_SIZE, sizeof (struct cpuinfo_ia64));
	DEFINE(SIGFRAME_SIZE, sizeof (struct sigframe));
	DEFINE(UNW_FRAME_INFO_SIZE, sizeof (struct unw_frame_info));

	BUILD_BUG_ON(sizeof(struct upid) != 32);
	DEFINE(IA64_UPID_SHIFT, 5);

	BLANK();

	DEFINE(TI_FLAGS, offsetof(struct thread_info, flags));
	DEFINE(TI_CPU, offsetof(struct thread_info, cpu));
	DEFINE(TI_PRE_COUNT, offsetof(struct thread_info, preempt_count));
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
	DEFINE(TI_AC_STAMP, offsetof(struct thread_info, ac_stamp));
	DEFINE(TI_AC_LEAVE, offsetof(struct thread_info, ac_leave));
	DEFINE(TI_AC_STIME, offsetof(struct thread_info, ac_stime));
	DEFINE(TI_AC_UTIME, offsetof(struct thread_info, ac_utime));
#endif

	BLANK();

	DEFINE(IA64_TASK_BLOCKED_OFFSET,offsetof (struct task_struct, blocked));
	DEFINE(IA64_TASK_CLEAR_CHILD_TID_OFFSET,offsetof (struct task_struct, clear_child_tid));
	DEFINE(IA64_TASK_GROUP_LEADER_OFFSET, offsetof (struct task_struct, group_leader));
	DEFINE(IA64_TASK_TGIDLINK_OFFSET, offsetof (struct task_struct, pids[PIDTYPE_PID].pid));
	DEFINE(IA64_PID_LEVEL_OFFSET, offsetof (struct pid, level));
	DEFINE(IA64_PID_UPID_OFFSET, offsetof (struct pid, numbers[0]));
	DEFINE(IA64_TASK_PENDING_OFFSET,offsetof (struc