 intermediate state (the cpuidle_device's safe state), and wait for
 * all the other cpus to call this function.  Once all coupled cpus are idle,
 * the second stage will start.  Each coupled cpu will spin until all cpus have
 * guaranteed that they will call the target_state.
 *
 * This function must be called with interrupts disabled.  It may enable
 * interrupts while preparing for idle, and it will always return with
 * interrupts enabled.
 */
int cpuidle_enter_state_coupled(struct cpuidle_device *dev,
		struct cpuidle_driver *drv, int next_state)
{
	int entered_state = -1;
	struct cpuidle_coupled *coupled = dev->coupled;
	int w;

	if (!coupled)
		return -EINVAL;

	while (coupled->prevent) {
		cpuidle_coupled_clear_pokes(dev->cpu);
		if (need_resched()) {
			local_irq_enable();
			return entered_state;
		}
		entered_state = cpuidle_enter_state(dev, drv,
			drv->safe_state_index);
		local_irq_disable();
	}

	/* Read barrier ensures online_count is read after prevent is cleared */
	smp_rmb();

reset:
	cpumask_clear_cpu(dev->cpu, &cpuidle_coupled_poked);

	w = cpuidle_coupled_set_waiting(dev->cpu, coupled, next_state);
	/*
	 * If this is the last cpu to enter the waiting state, poke
	 * all the other cpus out of their waiting state so they can
	 * enter a deeper state.  This can race with one of the cpus
	 * exiting the waiting state due to an interrupt and
	 * decrementing waiting_count, see comment below.
	 */
	if (w == coupled->online_count) {
		cpumask_set_cpu(dev->cpu, &cpuidle_coupled_poked);
		cpuidle_coupled_poke_others(dev->cpu, coupled);
	}

retry:
	/*
	 * Wait for all coupled cpus to be idle, using the deepest state
	 * allowed for a single cpu.  If this was not the poking cpu, wait
	 * for at least one poke before leaving to avoid a race where
	 * two cpus could arrive at the waiting loop at the same time,
	 * but the first of the two to arrive could skip the loop without
	 * processing the pokes from the last to arrive.
	 */
	while (!cpuidle_coupled_cpus_waiting(coupled) ||
			!cpumask_test_cpu(dev->cpu, &cpuidle_coupled_poked)) {
		if (cpuidle_coupled_clear_pokes(dev->cpu))
			continue;

		if (need_resched()) {
			cpuidle_coupled_set_not_waiting(dev->cpu, coupled);
			goto out;
		}

		if (coupled->prevent) {
			cpuidle_coupled_set_not_waiting(dev->cpu, coupled);
			goto out;
		}

		entered_state = cpuidle_enter_state(dev, drv,
			drv->safe_state_index);
		local_irq_disable();
	}

	cpuidle_coupled_clear_pokes(dev->cpu);
	if (need_resched()) {
		cpuidle_coupled_set_not_waiting(dev->cpu, coupled);
		goto out;
	}

	/*
	 * Make sure final poke status for this cpu is visible before setting
	 * cpu as ready.
	 */
	smp_wmb();

	/*
	 * All coupled cpus are probably idle.  There is a small chance that
	 * one of the other cpus just became active.  Increment the ready count,
	 * and spin until all coupled cpus have incremented the counter. Once a
	 * cpu has incremented the ready counter, it cannot abort idle and must
	 * spin until either all cpus have incremented the ready counter, or
	 * another cpu leaves idle and decrements the waiting counter.
	 */

	cpuidle_coupled_set_ready(coupled);
	while (!cpuidle_coupled_cpus_ready(coupled)) {
		/* Check if any other cpus bailed out of idle. */
		if (!cpuidle_coupled_cpus_waiting(coupled))
			if (!cpuidle_coupled_set_not_ready(coupled))
				goto retry;

		cpu_relax();
	}

	/*
	 * Make sure read of all cpus ready is done before reading pending pokes
	 */
	smp_rmb();

	/*
	 * There is a small chance that a cpu left and reentered idle after this
	 * cpu saw that all cpus were waiting.  The cpu that reentered idle will
	 * have sent this cpu a poke, which will still be pending after the
	 * ready loop.  The pending interrupt may be lost by the interrupt
	 * controller when entering the deep idle state.  It's not possible to
	 * clear a pending interrupt without turning interrupts on and handling
	 * it, and it's too late to turn on interrupts here, so reset the
	 * coupled idle state of all cpus and retry.
	 */
	if (cpuidle_coupled_any_pokes_pending(coupled)) {
		cpuidle_coupled_set_done(dev->cpu, coupled);
		/* Wait for all cpus to see the pending pokes */
		cpuidle_coupled_parallel_barrier(dev, &coupled->abort_barrier);
		goto reset;
	}

	/* all cpus have acked the coupled state */
	next_state = cpuidle_coupled_get_state(dev, coupled);

	entered_state = cpuidle_enter_state(dev, drv, next_state);

	cpuidle_coupled_set_done(dev->cpu, coupled);

out:
	/*
	 * Normal cpuidle states are expected to return with irqs enabled.
	 * That leads to an inefficiency where a cpu receiving an interrupt
	 * that brings it out of idle will process that interrupt before
	 * exiting the idle enter function and decrementing ready_count.  All
	 * other cpus will need to spin waiting for the cpu that is processing
	 * the interrupt.  If the driver returns with interrupts disabled,
	 * all other cpus will loop back into the safe idle state instead of
	 * spinning, saving power.
	 *
	 * Calling local_irq_enable here allows coupled states to return with
	 * interrupts disabled, but won't cause problems for drivers that
	 * exit with interrupts enabled.
	 */
	local_irq_enable();

	/*
	 * Wait until all coupled cpus have exited idle.  There is no risk that
	 * a cpu exits and re-enters the ready state because this cpu has
	 * already decremented its waiting_count.
	 */
	while (!cpuidle_coupled_no_cpus_ready(coupled))
		cpu_relax();

	return entered_state;
}

static void cpuidle_coupled_update_online_cpus(struct cpuidle_coupled *coupled)
{
	cpumask_t cpus;
	cpumask_and(&cpus, cpu_online_mask, &coupled->coupled_cpus);
	coupled->online_count = cpumask_weight(&cpus);
}

/**
 * cpuidle_coupled_register_device - register a coupled cpuidle device
 * @dev: struct cpuidle_device for the current cpu
 *
 * Called from cpuidle_register_device to handle coupled idle init.  Finds the
 * cpuidle_coupled struct for this set of coupled cpus, or creates one if none
 * exists yet.
 */
int cpuidle_coupled_register_device(struct cpuidle_device *dev)
{
	int cpu;
	struct cpuidle_device *other_dev;
	struct call_single_data *csd;
	struct cpuidle_coupled *coupled;

	if (cpumask_empty(&dev->coupled_cpus))
		return 0;

	for_each_cpu(cpu, &dev->coupled_cpus) {
		other_dev = per_cpu(cpuidle_devices, cpu);
		if (other_dev && other_dev->coupled) {
			coupled = other_dev->coupled;
			goto have_coupled;
		}
	}

	/* No existing coupled info found, create a new one */
	coupled = kzalloc(sizeof(struct cpuidle_coupled), GFP_KERNEL);
	if (!coupled)
		return -ENOMEM;

	coupled->coupled_cpus = dev->coupled_cpus;

have_coupled:
	dev->coupled = coupled;
	if (WARN_ON(!cpumask_equal(&dev->coupled_cpus, &coupled->coupled_cpus)))
		coupled->prevent++;

	cpuidle_coupled_update_online_cpus(coupled);

	coupled->refcnt++;

	csd = &per_cpu(cpuidle_coupled_poke_cb, dev->cpu);
	csd->func = cpuidle_coupled_handle_poke;
	csd->info = (void *)(unsigned long)dev->cpu;

	return 0;
}

/**
 * cpuidle_coupled_unregister_device - unregister a coupled cpuidle device
 * @dev: struct cpuidle_device for the current cpu
 *
 * Called from cpuidle_unregister_device to tear down coupled idle.  Removes the
 * cpu from the coupled idle set, and frees the cpuidle_coupled_info struct if
 * this was the last cpu in the set.
 */
void cpuidle_coupled_unregister_device(struct cpuidle_device *dev)
{
	struct cpuidle_coupled *coupled = dev->coupled;

	if (cpumask_empty(&dev->coupled_cpus))
		return;

	if (--coupled->refcnt)
		kfree(coupled);
	dev->coupled = NULL;
}

/**
 * cpuidle_coupled_prevent_idle - prevent cpus from entering a coupled state
 * @coupled: the struct coupled that contains the cpu that is changing state
 *
 * Disables coupled cpuidle on a coupled set of cpus.  Used to ensure that
 * cpu_online_mask doesn't change while cpus are coordinating coupled idle.
 */
static void cpuidle_coupled_prevent_idle(struct cpuidle_coupled *coupled)
{
	int cpu = get_cpu();

	/* Force all cpus out of the waiting loop. */
	coupled->prevent++;
	cpuidle_coupled_poke_others(cpu, coupled);
	put_cpu();
	while (!cpuidle_coupled_no_cpus_waiting(coupled))
		cpu_relax();
}

/**
 * cpuidle_coupled_allow_idle - allows cpus to enter a coupled state
 * @coupled: the struct coupled that contains the cpu that is changing state
 *
 * Enables coupled cpuidle on a coupled set of cpus.  Used to ensure that
 * cpu_online_mask doesn't change while cpus are coordinating coupled idle.
 */
static void cpuidle_coupled_allow_idle(struct cpuidle_coupled *coupled)
{
	int cpu = get_cpu();

	/*
	 * Write barrier ensures readers see the new online_count when they
	 * see prevent == 0.
	 */
	smp_wmb();
	coupled->prevent--;
	/* Force cpus out of the prevent loop. */
	cpuidle_coupled_poke_others(cpu, coupled);
	put_cpu();
}

/**
 * cpuidle_coupled_cpu_notify - notifier called during hotplug transitions
 * @nb: notifier block
 * @action: hotplug transition
 * @hcpu: target cpu number
 *
 * Called when a cpu is brought on or offline using hotplug.  Updates the
 * coupled cpu set appropriately
 */
static int cpuidle_coupled_cpu_notify(struct notifier_block *nb,
		unsigned long action, void *hcpu)
{
	int cpu = (unsigned long)hcpu;
	struct cpuidle_device *dev;

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_UP_PREPARE:
	case CPU_DOWN_PREPARE:
	case CPU_ONLINE:
	case CPU_DEAD:
	case CPU_UP_CANCELED:
	case CPU_DOWN_FAILED:
		break;
	default:
		return NOTIFY_OK;
	}

	mutex_lock(&cpuidle_lock);

	dev = per_cpu(cpuidle_devices, cpu);
	if (!dev || !dev->coupled)
		goto out;

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_UP_PREPARE:
	case CPU_DOWN_PREPARE:
		cpuidle_coupled_prevent_idle(dev->coupled);
		break;
	case CPU_ONLINE:
	case CPU_DEAD:
		cpuidle_coupled_update_online_cpus(dev->coupled);
		/* Fall through */
	case CPU_UP_CANCELED:
	case CPU_DOWN_FAILED:
		cpuidle_coupled_allow_idle(dev->coupled);
		break;
	}

out:
	mutex_unlock(&cpuidle_lock);
	return NOTIFY_OK;
}

static struct notifier_block cpuidle_coupled_cpu_notifier = {
	.notifier_call = cpuidle_coupled_cpu_notify,
};

static int __init cpuidle_coupled_init(void)
{
	return register_cpu_notifier(&cpuidle_coupled_cpu_notifier);
}
core_initcall(cpuidle_coupled_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * ARM/ARM64 generic CPU idle driver.
 *
 * Copyright (C) 2014 ARM Ltd.
 * Author: Lorenzo Pieralisi <lorenzo.pieralisi@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "CPUidle arm: " fmt

#include <linux/cpuidle.h>
#include <linux/cpumask.h>
#include <linux/cpu_pm.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>

#include <asm/cpuidle.h>

#include "dt_idle_states.h"

/*
 * arm_enter_idle_state - Programs CPU to enter the specified state
 *
 * dev: cpuidle device
 * drv: cpuidle driver
 * idx: state index
 *
 * Called from the CPUidle framework to program the device to the
 * specified target state selected by the governor.
 */
static int arm_enter_idle_state(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int idx)
{
	int ret;

	if (!idx) {
		cpu_do_idle();
		return idx;
	}

	ret = cpu_pm_enter();
	if (!ret) {
		/*
		 * Pass idle state index to cpu_suspend which in turn will
		 * call the CPU ops suspend protocol with idle index as a
		 * parameter.
		 */
		ret = arm_cpuidle_suspend(idx);

		cpu_pm_exit();
	}

	return ret ? -1 : idx;
}

static struct cpuidle_driver arm_idle_driver = {
	.name = "arm_idle",
	.owner = THIS_MODULE,
	/*
	 * State at index 0 is standby wfi and considered standard
	 * on all ARM platforms. If in some platforms simple wfi
	 * can't be used as "state 0", DT bindings must be implemented
	 * to work around this issue and allow installing a special
	 * handler for idle state index 0.
	 */
	.states[0] = {
		.enter                  = arm_enter_idle_state,
		.exit_latency           = 1,
		.target_residency       = 1,
		.power_usage		= UINT_MAX,
		.name                   = "WFI",
		.desc                   = "ARM WFI",
	}
};

static const struct of_device_id arm_idle_state_match[] __initconst = {
	{ .compatible = "arm,idle-state",
	  .data = arm_enter_idle_state },
	{ },
};

/*
 * arm_idle_init
 *
 * Registers the arm specific cpuidle driver with the cpuidle
 * framework. It relies on core code to parse the idle states
 * and initialize them using driver data structures accordingly.
 */
static int __init arm_idle_init(void)
{
	int cpu, ret;
	struct cpuidle_driver *drv = &arm_idle_driver;
	struct cpuidle_device *dev;

	/*
	 * Initialize idle states data, starting at index 1.
	 * This driver is DT only, if no DT idle states are detected (ret == 0)
	 * let the driver initialization fail accordingly since there is no
	 * reason to initialize the idle driver if only wfi is supported.
	 */
	ret = dt_init_idle_driver(drv, arm_idle_state_match, 1);
	if (ret <= 0)
		return ret ? : -ENODEV;

	ret = cpuidle_register_driver(drv);
	if (ret) {
		pr_err("Failed to register cpuidle driver\n");
		return ret;
	}

	/*
	 * Call arch CPU operations in order to initialize
	 * idle states suspend back-end specific data
	 */
	for_each_possible_cpu(cpu) {
		ret = arm_cpuidle_init(cpu);

		/*
		 * Skip the cpuidle device initialization if the reported
		 * failure is a HW misconfiguration/breakage (-ENXIO).
		 */
		if (ret == -ENXIO)
			continue;

		if (ret) {
			pr_err("CPU %d failed to init idle CPU ops\n", cpu);
			goto out_fail;
		}

		dev = kzalloc(sizeof(*dev), GFP_KERNEL);
		if (!dev) {
			pr_err("Failed to allocate cpuidle device\n");
			ret = -ENOMEM;
			goto out_fail;
		}
		dev->cpu = cpu;

		ret = cpuidle_register_device(dev);
		if (ret) {
			pr_err("Failed to register cpuidle device for CPU %d\n",
			       cpu);
			kfree(dev);
			goto out_fail;
		}
	}

	return 0;
out_fail:
	while (--cpu >= 0) {
		dev = per_cpu(cpuidle_devices, cpu);
		cpuidle_unregister_device(dev);
		kfree(dev);
	}

	cpuidle_unregister_driver(drv);

	return ret;
}
device_initcall(arm_idle_init);
                                                                                                                                                                                                                           /*
 * based on arch/arm/mach-kirkwood/cpuidle.c
 *
 * CPU idle support for AT91 SoC
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * The cpu idle uses wait-for-interrupt and RAM self refresh in order
 * to implement two idle states -
 * #1 wait-for-interrupt
 * #2 wait-for-interrupt and RAM self refresh
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/cpuidle.h>
#include <linux/io.h>
#include <linux/export.h>
#include <asm/cpuidle.h>

#define AT91_MAX_STATES	2

static void (*at91_standby)(void);

/* Actual code that puts the SoC in different idle states */
static int at91_enter_idle(struct cpuidle_device *dev,
			struct cpuidle_driver *drv,
			       int index)
{
	at91_standby();
	return index;
}

static struct cpuidle_driver at91_idle_driver = {
	.name			= "at91_idle",
	.owner			= THIS_MODULE,
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	.states[1]		= {
		.enter			= at91_enter_idle,
		.exit_latency		= 10,
		.target_residency	= 10000,
		.name			= "RAM_SR",
		.desc			= "WFI and DDR Self Refresh",
	},
	.state_count = AT91_MAX_STATES,
};

/* Initialize CPU idle by registering the idle states */
static int at91_cpuidle_probe(struct platform_device *dev)
{
	at91_standby = (void *)(dev->dev.platform_data);
	
	return cpuidle_register(&at91_idle_driver, NULL);
}

static struct platform_driver at91_cpuidle_driver = {
	.driver = {
		.name = "cpuidle-at91",
	},
	.probe = at91_cpuidle_probe,
};
builtin_platform_driver(at91_cpuidle_driver);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * Copyright (c) 2013 ARM/Linaro
 *
 * Authors: Daniel Lezcano <daniel.lezcano@linaro.org>
 *          Lorenzo Pieralisi <lorenzo.pieralisi@arm.com>
 *          Nicolas Pitre <nicolas.pitre@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Maintainer: Lorenzo Pieralisi <lorenzo.pieralisi@arm.com>
 * Maintainer: Daniel Lezcano <daniel.lezcano@linaro.org>
 */
#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/slab.h>
#include <linux/of.h>

#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/cpuidle.h>
#include <asm/mcpm.h>
#include <asm/smp_plat.h>
#include <asm/suspend.h>

#include "dt_idle_states.h"

static int bl_enter_powerdown(struct cpuidle_device *dev,
			      struct cpuidle_driver *drv, int idx);

/*
 * NB: Owing to current menu governor behaviour big and LITTLE
 * index 1 states have to define exit_latency and target_residency for
 * cluster state since, when all CPUs in a cluster hit it, the cluster
 * can be shutdown. This means that when a single CPU enters this state
 * the exit_latency and target_residency values are somewhat overkill.
 * There is no notion of cluster states in the menu governor, so CPUs
 * have to define CPU states where possibly the cluster will be shutdown
 * depending on the state of other CPUs. idle states entry and exit happen
 * at random times; however the cluster state provides target_residency
 * values as if all CPUs in a cluster enter the state at once; this is
 * somewhat optimistic and behaviour should be fixed either in the governor
 * or in the MCPM back-ends.
 * To make this driver 100% generic the number of states and the exit_latency
 * target_residency values must be obtained from device tree bindings.
 *
 * exit_latency: refers to the TC2 vexpress test chip and depends on the
 * current cluster operating point. It is the time it takes to get the CPU
 * up and running when the CPU is powered up on cluster wake-up from shutdown.
 * Current values for big and LITTLE clusters are provided for clusters
 * running at default operating points.
 *
 * target_residency: it is the minimum amount of time the cluster has
 * to be down to break even in terms of power consumption. cluster
 * shutdown has inherent dynamic power costs (L2 writebacks to DRAM
 * being the main factor) that depend on the current operating points.
 * The current values for both clusters are provided for a CPU whose half
 * of L2 lines are dirty and require cleaning to DRAM, and takes into
 * account leakage static power values related to the vexpress TC2 testchip.
 */
static struct cpuidle_driver bl_idle_little_driver = {
	.name = "little_idle",
	.owner = THIS_MODULE,
	.states[0] = ARM_CPUIDLE_WFI_STATE,
	.states[1] = {
		.enter			= bl_enter_powerdown,
		.exit_latency		= 700,
		.target_residency	= 2500,
		.flags			= CPUIDLE_FLAG_TIMER_STOP,
		.name			= "C1",
		.desc			= "ARM little-cluster power down",
	},
	.state_count = 2,
};

static const struct of_device_id bl_idle_state_match[] __initconst = {
	{ .compatible = "arm,idle-state",
	  .data = bl_enter_powerdown },
	{ },
};

static struct cpuidle_driver bl_idle_big_driver = {
	.name = "big_idle",
	.owner = THIS_MODULE,
	.states[0] = ARM_CPUIDLE_WFI_STATE,
	.states[1] = {
		.enter			= bl_enter_powerdown,
		.exit_latency		= 500,
		.target_residency	= 2000,
		.flags			= CPUIDLE_FLAG_TIMER_STOP,
		.name			= "C1",
		.desc			= "ARM big-cluster power down",
	},
	.state_count = 2,
};

/*
 * notrace prevents trace shims from getting inserted where they
 * should not. Global jumps and ldrex/strex must not be inserted
 * in power down sequences where caches and MMU may be turned off.
 */
static int notrace bl_powerdown_finisher(unsigned long arg)
{
	/* MCPM works with HW CPU identifiers */
	unsigned int mpidr = read_cpuid_mpidr();
	unsigned int cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
	unsigned int cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);

	mcpm_set_entry_vector(cpu, cluster, cpu_resume);
	mcpm_cpu_suspend();

	/* return value != 0 means failure */
	return 1;
}

/**
 * bl_enter_powerdown - Programs CPU to enter the specified state
 * @dev: cpuidle device
 * @drv: The target state to be programmed
 * @idx: state index
 *
 * Called from the CPUidle framework to program the device to the
 * specified target state selected by the governor.
 */
static int bl_enter_powerdown(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int idx)
{
	cpu_pm_enter();

	cpu_suspend(0, bl_powerdown_finisher);

	/* signals the MCPM core that CPU is out of low power state */
	mcpm_cpu_powered_up();

	cpu_pm_exit();

	return idx;
}

static int __init bl_idle_driver_init(struct cpuidle_driver *drv, int part_id)
{
	struct cpumask *cpumask;
	int cpu;

	cpumask = kzalloc(cpumask_size(), GFP_KERNEL);
	if (!cpumask)
		return -ENOMEM;

	for_each_possible_cpu(cpu)
		if (smp_cpuid_part(cpu) == part_id)
			cpumask_set_cpu(cpu, cpumask);

	drv->cpumask = cpumask;

	return 0;
}

static const struct of_device_id compatible_machine_match[] = {
	{ .compatible = "arm,vexpress,v2p-ca15_a7" },
	{ .compatible = "samsung,exynos5420" },
	{ .compatible = "samsung,exynos5800" },
	{},
};

static int __init bl_idle_init(void)
{
	int ret;
	struct device_node *root = of_find_node_by_path("/");
	const struct of_device_id *match_id;

	if (!root)
		return -ENODEV;

	/*
	 * Initialize the driver just for a compliant set of machines
	 */
	match_id = of_match_node(compatible_machine_match, root);

	of_node_put(root);

	if (!match_id)
		return -ENODEV;

	if (!mcpm_is_available())
		return -EUNATCH;

	/*
	 * For now the differentiation between little and big cores
	 * is based on the part number. A7 cores are considered little
	 * cores, A15 are considered big cores. This distinction may
	 * evolve in the future with a more generic matching approach.
	 */
	ret = bl_idle_driver_init(&bl_idle_little_driver,
				  ARM_CPU_PART_CORTEX_A7);
	if (ret)
		return ret;

	ret = bl_idle_driver_init(&bl_idle_big_driver, ARM_CPU_PART_CORTEX_A15);
	if (ret)
		goto out_uninit_little;

	/* Start at index 1, index 0 standard WFI */
	ret = dt_init_idle_driver(&bl_idle_big_driver, bl_idle_state_match, 1);
	if (ret < 0)
		goto out_uninit_big;

	/* Start at index 1, index 0 standard WFI */
	ret = dt_init_idle_driver(&bl_idle_little_driver,
				  bl_idle_state_match, 1);
	if (ret < 0)
		goto out_uninit_big;

	ret = cpuidle_register(&bl_idle_little_driver, NULL);
	if (ret)
		goto out_uninit_big;

	ret = cpuidle_register(&bl_idle_big_driver, NULL);
	if (ret)
		goto out_unregister_little;

	return 0;

out_unregister_little:
	cpuidle_unregister(&bl_idle_little_driver);
out_uninit_big:
	kfree(bl_idle_big_driver.cpumask);
out_uninit_little:
	kfree(bl_idle_little_driver.cpumask);

	return ret;
}
device_initcall(bl_idle_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * Copyright 2012 Calxeda, Inc.
 *
 * Based on arch/arm/plat-mxc/cpuidle.c: #v3.7
 * Copyright 2012 Freescale Semiconductor, Inc.
 * Copyright 2012 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Maintainer: Rob Herring <rob.herring@calxeda.com>
 */

#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/psci.h>

#include <asm/cpuidle.h>
#include <asm/suspend.h>

#include <uapi/linux/psci.h>

#define CALXEDA_IDLE_PARAM \
	((0 << PSCI_0_2_POWER_STATE_ID_SHIFT) | \
	 (0 << PSCI_0_2_POWER_STATE_AFFL_SHIFT) | \
	 (PSCI_POWER_STATE_TYPE_POWER_DOWN << PSCI_0_2_POWER_STATE_TYPE_SHIFT))

static int calxeda_idle_finish(unsigned long val)
{
	return psci_ops.cpu_suspend(CALXEDA_IDLE_PARAM, __pa(cpu_resume));
}

static int calxeda_pwrdown_idle(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	cpu_pm_enter();
	cpu_suspend(0, calxeda_idle_finish);
	cpu_pm_exit();

	return index;
}

static struct cpuidle_driver calxeda_idle_driver = {
	.name = "calxeda_idle",
	.states = {
		ARM_CPUIDLE_WFI_STATE,
		{
			.name = "PG",
			.desc = "Power Gate",
			.exit_latency = 30,
			.power_usage = 50,
			.target_residency = 200,
			.enter = calxeda_pwrdown_idle,
		},
	},
	.state_count = 2,
};

static int calxeda_cpuidle_probe(struct platform_device *pdev)
{
	return cpuidle_register(&calxeda_idle_driver, NULL);
}

static struct platform_driver calxeda_cpuidle_plat_driver = {
        .driver = {
                .name = "cpuidle-calxeda",
        },
        .probe = calxeda_cpuidle_probe,
};
builtin_platform_driver(calxeda_cpuidle_plat_driver);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 *  CLPS711X CPU idle driver
 *
 *  Copyright (C) 2014 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/cpuidle.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define CLPS711X_CPUIDLE_NAME	"clps711x-cpuidle"

static void __iomem *clps711x_halt;

static int clps711x_cpuidle_halt(struct cpuidle_device *dev,
				 struct cpuidle_driver *drv, int index)
{
	writel(0xaa, clps711x_halt);

	return index;
}

static struct cpuidle_driver clps711x_idle_driver = {
	.name		= CLPS711X_CPUIDLE_NAME,
	.owner		= THIS_MODULE,
	.states[0]	= {
		.name		= "HALT",
		.desc		= "CLPS711X HALT",
		.enter		= clps711x_cpuidle_halt,
		.exit_latency	= 1,
	},
	.state_count	= 1,
};

static int __init clps711x_cpuidle_probe(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	clps711x_halt = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(clps711x_halt))
		return PTR_ERR(clps711x_halt);

	return cpuidle_register(&clps711x_idle_driver, NULL);
}

static struct platform_driver clps711x_cpuidle_driver = {
	.driver	= {
		.name	= CLPS711X_CPUIDLE_NAME,
	},
};
module_platform_driver_probe(clps711x_cpuidle_driver, clps711x_cpuidle_probe);

MODULE_AUTHOR("Alexander Shiyan <shc_work@mail.ru>");
MODULE_DESCRIPTION("CLPS711X CPU idle driver");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * Copyright (C) 2014 Imagination Technologies
 * Author: Paul Burton <paul.burton@imgtec.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/cpu_pm.h>
#include <linux/cpuidle.h>
#include <linux/init.h>

#include <asm/idle.h>
#include <asm/pm-cps.h>

/* Enumeration of the various idle states this driver may enter */
enum cps_idle_state {
	STATE_WAIT = 0,		/* MIPS wait instruction, coherent */
	STATE_NC_WAIT,		/* MIPS wait instruction, non-coherent */
	STATE_CLOCK_GATED,	/* Core clock gated */
	STATE_POWER_GATED,	/* Core power gated */
	STATE_COUNT
};

static int cps_nc_enter(struct cpuidle_device *dev,
			struct cpuidle_driver *drv, int index)
{
	enum cps_pm_state pm_state;
	int err;

	/*
	 * At least one core must remain powered up & clocked in order for the
	 * system to have any hope of functioning.
	 *
	 * TODO: don't treat core 0 specially, just prevent the final core
	 * TODO: remap interrupt affinity temporarily
	 */
	if (!cpu_data[dev->cpu].core && (index > STATE_NC_WAIT))
		index = STATE_NC_WAIT;

	/* Select the appropriate cps_pm_state */
	switch (index) {
	case STATE_NC_WAIT:
		pm_state = CPS_PM_NC_WAIT;
		break;
	case STATE_CLOCK_GATED:
		pm_state = CPS_PM_CLOCK_GATED;
		break;
	case STATE_POWER_GATED:
		pm_state = CPS_PM_POWER_GATED;
		break;
	default:
		BUG();
		return -EINVAL;
	}

	/* Notify listeners the CPU is about to power down */
	if ((pm_state == CPS_PM_POWER_GATED) && cpu_pm_enter())
		return -EINTR;

	/* Enter that state */
	err = cps_pm_enter_state(pm_state);

	/* Notify listeners the CPU is back up */
	if (pm_state == CPS_PM_POWER_GATED)
		cpu_pm_exit();

	return err ?: index;
}

static struct cpuidle_driver cps_driver = {
	.name			= "cpc_cpuidle",
	.owner			= THIS_MODULE,
	.states = {
		[STATE_WAIT] = MIPS_CPUIDLE_WAIT_STATE,
		[STATE_NC_WAIT] = {
			.enter	= cps_nc_enter,
			.exit_latency		= 200,
			.target_residency	= 450,
			.name	= "nc-wait",
			.desc	= "non-coherent MIPS wait",
		},
		[STATE_CLOCK_GATED] = {
			.enter	= cps_nc_enter,
			.exit_latency		= 300,
			.target_residency	= 700,
			.flags	= CPUIDLE_FLAG_TIMER_STOP,
			.name	= "clock-gated",
			.desc	= "core clock gated",
		},
		[STATE_POWER_GATED] = {
			.enter	= cps_nc_enter,
			.exit_latency		= 600,
			.target_residency	= 1000,
			.flags	= CPUIDLE_FLAG_TIMER_STOP,
			.name	= "power-gated",
			.desc	= "core power gated",
		},
	},
	.state_count		= STATE_COUNT,
	.safe_state_index	= 0,
};

static void __init cps_cpuidle_unregister(void)
{
	int cpu;
	struct cpuidle_device *device;

	for_each_possible_cpu(cpu) {
		device = &per_cpu(cpuidle_dev, cpu);
		cpuidle_unregister_device(device);
	}

	cpuidle_unregister_driver(&cps_driver);
}

static int __init cps_cpuidle_init(void)
{
	int err, cpu, core, i;
	struct cpuidle_device *device;

	/* Detect supported states */
	if (!cps_pm_support_state(CPS_PM_POWER_GATED))
		cps_driver.state_count = STATE_CLOCK_GATED + 1;
	if (!cps_pm_support_state(CPS_PM_CLOCK_GATED))
		cps_driver.state_count = STATE_NC_WAIT + 1;
	if (!cps_pm_support_state(CPS_PM_NC_WAIT))
		cps_driver.state_count = STATE_WAIT + 1;

	/* Inform the user if some states are unavailable */
	if (cps_driver.state_count < STATE_COUNT) {
		pr_info("cpuidle-cps: limited to ");
		switch (cps_driver.state_count - 1) {
		case STATE_WAIT:
			pr_cont("coherent wait\n");
			break;
		case STATE_NC_WAIT:
			pr_cont("non-coherent wait\n");
			break;
		case STATE_CLOCK_GATED:
			pr_cont("clock gating\n");
			break;
		}
	}

	/*
	 * Set the coupled flag on the appropriate states if this system
	 * requires it.
	 */
	if (coupled_coherence)
		for (i = STATE_NC_WAIT; i < cps_driver.state_count; i++)
			cps_driver.states[i].flags |= CPUIDLE_FLAG_COUPLED;

	err = cpuidle_register_driver(&cps_driver);
	if (err) {
		pr_err("Failed to register CPS cpuidle driver\n");
		return err;
	}

	for_each_possible_cpu(cpu) {
		core = cpu_data[cpu].core;
		device = &per_cpu(cpuidle_dev, cpu);
		device->cpu = cpu;
#ifdef CONFIG_MIPS_MT
		cpumask_copy(&device->coupled_cpus, &cpu_sibling_map[cpu]);
#endif

		err = cpuidle_register_device(device);
		if (err) {
			pr_err("Failed to register CPU%d cpuidle device\n",
			       cpu);
			goto err_out;
		}
	}

	return 0;
err_out:
	cps_cpuidle_unregister();
	return err;
}
device_initcall(cps_cpuidle_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (c) 2011-2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Coupled cpuidle support based on the work of:
 *	Colin Cross <ccross@android.com>
 *	Daniel Lezcano <daniel.lezcano@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/platform_data/cpuidle-exynos.h>

#include <asm/suspend.h>
#include <asm/cpuidle.h>

static atomic_t exynos_idle_barrier;

static struct cpuidle_exynos_data *exynos_cpuidle_pdata;
static void (*exynos_enter_aftr)(void);

static int exynos_enter_coupled_lowpower(struct cpuidle_device *dev,
					 struct cpuidle_driver *drv,
					 int index)
{
	int ret;

	exynos_cpuidle_pdata->pre_enter_aftr();

	/*
	 * Waiting all cpus to reach this point at the same moment
	 */
	cpuidle_coupled_parallel_barrier(dev, &exynos_idle_barrier);

	/*
	 * Both cpus will reach this point at the same time
	 */
	ret = dev->cpu ? exynos_cpuidle_pdata->cpu1_powerdown()
		       : exynos_cpuidle_pdata->cpu0_enter_aftr();
	if (ret)
		index = ret;

	/*
	 * Waiting all cpus to finish the power sequence before going further
	 */
	cpuidle_coupled_parallel_barrier(dev, &exynos_idle_barrier);

	exynos_cpuidle_pdata->post_enter_aftr();

	return index;
}

static int exynos_enter_lowpower(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	int new_index = index;

	/* AFTR can only be entered when cores other than CPU0 are offline */
	if (num_online_cpus() > 1 || dev->cpu != 0)
		new_index = drv->safe_state_index;

	if (new_index == 0)
		return arm_cpuidle_simple_enter(dev, drv, new_index);

	exynos_enter_aftr();

	return new_index;
}

static struct cpuidle_driver exynos_idle_driver = {
	.name			= "exynos_idle",
	.owner			= THIS_MODULE,
	.states = {
		[0] = ARM_CPUIDLE_WFI_STATE,
		[1] = {
			.enter			= exynos_enter_lowpower,
			.exit_latency		= 300,
			.target_residency	= 100000,
			.name			= "C1",
			.desc			= "ARM power down",
		},
	},
	.state_count = 2,
	.safe_state_index = 0,
};

static struct cpuidle_driver exynos_coupled_idle_driver = {
	.name			= "exynos_coupled_idle",
	.owner			= THIS_MODULE,
	.states = {
		[0] = ARM_CPUIDLE_WFI_STATE,
		[1] = {
			.enter			= exynos_enter_coupled_lowpower,
			.exit_latency		= 5000,
			.target_residency	= 10000,
			.flags			= CPUIDLE_FLAG_COUPLED |
						  CPUIDLE_FLAG_TIMER_STOP,
			.name			= "C1",
			.desc			= "ARM power down",
		},
	},
	.state_count = 2,
	.safe_state_index = 0,
};

static int exynos_cpuidle_probe(struct platform_device *pdev)
{
	int ret;

	if (IS_ENABLED(CONFIG_SMP) &&
	    of_machine_is_compatible("samsung,exynos4210")) {
		exynos_cpuidle_pdata = pdev->dev.platform_data;

		ret = cpuidle_register(&exynos_coupled_idle_driver,
				       cpu_possible_mask);
	} else {
		exynos_enter_aftr = (void *)(pdev->dev.platform_data);

		ret = cpuidle_register(&exynos_idle_driver, NULL);
	}

	if (ret) {
		dev_err(&pdev->dev, "failed to register cpuidle driver\n");
		return ret;
	}

	return 0;
}

static struct platform_driver exynos_cpuidle_driver = {
	.probe	= exynos_cpuidle_probe,
	.driver = {
		.name = "exynos_cpuidle",
	},
};

module_platform_driver(exynos_cpuidle_driver);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * CPUIDLE driver for exynos 64bit
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/suspend.h>
#include <linux/cpu.h>
#include <linux/reboot.h>
#include <linux/of.h>
#include <linux/psci.h>
#include <linux/cpuidle_profiler.h>

#include <asm/tlbflush.h>
#include <asm/cpuidle.h>
#include <asm/topology.h>

#include <soc/samsung/exynos-powermode.h>

#include "dt_idle_states.h"

/*
 * Exynos cpuidle driver supports the below idle states
 *
 * IDLE_C1 : WFI(Wait For Interrupt) low-power state
 * IDLE_C2 : Local CPU power gating
 */
enum idle_states {
	IDLE_C1 = 0,
	IDLE_C2,
	IDLE_STATE_MAX,
};

/***************************************************************************
 *                           Cpuidle state handler                         *
 ***************************************************************************/
static unsigned int prepare_idle(unsigned int cpu, int index)
{
	unsigned int entry_state = 0;

	if (index > 0) {
		cpu_pm_enter();
		entry_state = exynos_cpu_pm_enter(cpu, index);
	}

	cpuidle_profile_start(cpu, index, entry_state);

	return entry_state;
}

static void post_idle(unsigned int cpu, int index, int fail)
{
	cpuidle_profile_finish(cpu, fail);

	if (!index)
		return;

	exynos_cpu_pm_exit(cpu, fail);
	cpu_pm_exit();
}

static int enter_idle(unsigned int index)
{
	/*
	 * idle state index 0 corresponds to wfi, should never be called
	 * from the cpu_suspend operations
	 */
	if (!index) {
		cpu_do_idle();
		return 0;
	}

	return arm_cpuidle_suspend(index);
}

static int exynos_enter_idle(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int index)
{
	int entry_state, ret = 0;

	entry_state = prepare_idle(dev->cpu, index);

	ret = enter_idle(entry_state);

	post_idle(dev->cpu, index, ret);

	/*
	 * If cpu fail to enter idle, it should not update state usage
	 * count. Driver have to return an error value to
	 * cpuidle_enter_state().
	 */
	if (ret < 0)
		return ret;
	else
		return index;
}

/***************************************************************************
 *                            Define notifier call                         *
 ***************************************************************************/
static int exynos_cpuidle_reboot_notifier(struct notifier_block *this,
				unsigned long event, void *_cmd)
{
	switch (event) {
	case SYSTEM_POWER_OFF:
	case SYS_RESTART:
		cpuidle_pause();
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_cpuidle_reboot_nb = {
	.notifier_call = exynos_cpuidle_reboot_notifier,
};

/***************************************************************************
 *                         Initialize cpuidle driver                       *
 ***************************************************************************/
#define exynos_idle_wfi_state(state)					\
	do {								\
		state.enter = exynos_enter_idle;			\
		state.exit_latency = 1;					\
		state.target_residency = 1;				\
		state.power_usage = UINT_MAX;				\
		strncpy(state.name, "WFI", CPUIDLE_NAME_LEN - 1);	\
		strncpy(state.desc, "c1", CPUIDLE_DESC_LEN - 1);	\
	} while (0)

static struct cpuidle_driver exynos_idle_driver[NR_CPUS];

static const struct of_device_id exynos_idle_state_match[] __initconst = {
	{ .compatible = "exynos,idle-state",
	  .data = exynos_enter_idle },
	{ },
};

static int __init exynos_idle_driver_init(struct cpuidle_driver *drv,
					   struct cpumask* cpumask)
{
	int cpu = cpumask_first(cpumask);

	drv->name = kzalloc(sizeof("exynos_idleX"), GFP_KERNEL);
	if (!drv->name)
		return -ENOMEM;

	scnprintf((char *)drv->name, 12, "exynos_idle%d", cpu);
	drv->owner = THIS_MODULE;
	drv->cpumask = cpumask;
	exynos_idle_wfi_state(drv->states[0]);

	return 0;
}

static int __init exynos_idle_init(void)
{
	int ret, cpu, i;

	for_each_possible_cpu(cpu) {
		ret = exynos_idle_driver_init(&exynos_idle_driver[cpu],
					      topology_sibling_cpumask(cpu));

		if (ret) {
			pr_err("failed to initialize cpuidle driver for cpu%d",
					cpu);
			goto out_unregister;
		}

		/*
		 * Initialize idle states data, starting at index 1.
		 * This driver is DT only, if no DT idle states are detected
		 * (ret == 0) let the driver initialization fail accordingly
		 * since there is no reason to initialize the idle driver
		 * if only wfi is supported.
		 */
		ret = dt_init_idle_driver(&exynos_idle_driver[cpu],
					exynos_idle_state_match, 1);
		if (ret < 0) {
			pr_err("failed to initialize idle state for cpu%d\n", cpu);
			goto out_unregister;
		}

		/*
		 * Call arch CPU operations in order to initialize
		 * idle states suspend back-end specific data
		 */
		ret = arm_cpuidle_init(cpu);
		if (ret) {
			pr_err("failed to initialize idle operation for cpu%d\n", cpu);
			goto out_unregister;
		}

		ret = cpuidle_register(&exynos_idle_driver[cpu], NULL);
		if (ret) {
			pr_err("failed to register cpuidle for cpu%d\n", cpu);
			goto out_unregister;
		}
	}

	register_reboot_notifier(&exynos_cpuidle_reboot_nb);

	cpuidle_profile_register(&exynos_idle_driver[0]);

	pr_info("Exynos cpuidle driver Initialized\n");

	return 0;

out_unregister:
	for (i = 0; i <= cpu; i++) {
		if (exynos_idle_driver[i].name)
			kfree(exynos_idle_driver[i].name);

		/*
		 * Cpuidle driver of variable "cpu" is always not registered.
		 * "cpu" should not call cpuidle_unregister().
		 */
		if (i < cpu)
			cpuidle_unregister(&exynos_idle_driver[i]);
	}

	return ret;
}
device_initcall(exynos_idle_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * CPU idle Marvell Kirkwood SoCs
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * The cpu idle uses wait-for-interrupt and DDR self refresh in order
 * to implement two idle states -
 * #1 wait-for-interrupt
 * #2 wait-for-interrupt and DDR self refresh
 *
 * Maintainer: Jason Cooper <jason@lakedaemon.net>
 * Maintainer: Andrew Lunn <andrew@lunn.ch>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/cpuidle.h>
#include <linux/io.h>
#include <linux/export.h>
#include <asm/cpuidle.h>

#define KIRKWOOD_MAX_STATES	2

static void __iomem *ddr_operation_base;

/* Actual code that puts the SoC in different idle states */
static int kirkwood_enter_idle(struct cpuidle_device *dev,
			       struct cpuidle_driver *drv,
			       int index)
{
	writel(0x7, ddr_operation_base);
	cpu_do_idle();

	return index;
}

static struct cpuidle_driver kirkwood_idle_driver = {
	.name			= "kirkwood_idle",
	.owner			= THIS_MODULE,
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	.states[1]		= {
		.enter			= kirkwood_enter_idle,
		.exit_latency		= 10,
		.target_residency	= 100000,
		.name			= "DDR SR",
		.desc			= "WFI and DDR Self Refresh",
	},
	.state_count = KIRKWOOD_MAX_STATES,
};

/* Initialize CPU idle by registering the idle states */
static int kirkwood_cpuidle_probe(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ddr_operation_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ddr_operation_base))
		return PTR_ERR(ddr_operation_base);

	return cpuidle_register(&kirkwood_idle_driver, NULL);
}

static int kirkwood_cpuidle_remove(struct platform_device *pdev)
{
	cpuidle_unregister(&kirkwood_idle_driver);
	return 0;
}

static struct platform_driver kirkwood_cpuidle_driver = {
	.probe = kirkwood_cpuidle_probe,
	.remove = kirkwood_cpuidle_remove,
	.driver = {
		   .name = "kirkwood_cpuidle",
		   },
};

module_platform_driver(kirkwood_cpuidle_driver);

MODULE_AUTHOR("Andrew Lunn <andrew@lunn.ch>");
MODULE_DESCRIPTION("Kirkwood cpu idle driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:kirkwood-cpuidle");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Marvell Armada 370, 38x and XP SoC cpuidle driver
 *
 * Copyright (C) 2014 Marvell
 *
 * Nadav Haklai <nadavh@marvell.com>
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * Maintainer: Gregory CLEMENT <gregory.clement@free-electrons.com>
 */

#include <linux/cpu_pm.h>
#include <linux/cpuidle.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>
#include <asm/cpuidle.h>

#define MVEBU_V7_FLAG_DEEP_IDLE	0x10000

static int (*mvebu_v7_cpu_suspend)(int);

static int mvebu_v7_enter_idle(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	int ret;
	bool deepidle = false;
	cpu_pm_enter();

	if (drv->states[index].flags & MVEBU_V7_FLAG_DEEP_IDLE)
		deepidle = true;

	ret = mvebu_v7_cpu_suspend(deepidle);
	cpu_pm_exit();

	if (ret)
		return ret;

	return index;
}

static struct cpuidle_driver armadaxp_idle_driver = {
	.name			= "armada_xp_idle",
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	.states[1]		= {
		.enter			= mvebu_v7_enter_idle,
		.exit_latency		= 100,
		.power_usage		= 50,
		.target_residency	= 1000,
		.name			= "MV CPU IDLE",
		.desc			= "CPU power down",
	},
	.states[2]		= {
		.enter			= mvebu_v7_enter_idle,
		.exit_latency		= 1000,
		.power_usage		= 5,
		.target_residency	= 10000,
		.flags			= MVEBU_V7_FLAG_DEEP_IDLE,
		.name			= "MV CPU DEEP IDLE",
		.desc			= "CPU and L2 Fabric power down",
	},
	.state_count = 3,
};

static struct cpuidle_driver armada370_idle_driver = {
	.name			= "armada_370_idle",
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	.states[1]		= {
		.enter			= mvebu_v7_enter_idle,
		.exit_latency		= 100,
		.power_usage		= 5,
		.target_residency	= 1000,
		.flags			= MVEBU_V7_FLAG_DEEP_IDLE,
		.name			= "Deep Idle",
		.desc			= "CPU and L2 Fabric power down",
	},
	.state_count = 2,
};

static struct cpuidle_driver armada38x_idle_driver = {
	.name			= "armada_38x_idle",
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	.states[1]		= {
		.enter			= mvebu_v7_enter_idle,
		.exit_latency		= 10,
		.power_usage		= 5,
		.target_residency	= 100,
		.name			= "Idle",
		.desc			= "CPU and SCU power down",
	},
	.state_count = 2,
};

static int mvebu_v7_cpuidle_probe(struct platform_device *pdev)
{
	const struct platform_device_id *id = pdev->id_entry;

	if (!id)
		return -EINVAL;

	mvebu_v7_cpu_suspend = pdev->dev.platform_data;

	return cpuidle_register((struct cpuidle_driver *)id->driver_data, NULL);
}

static const struct platform_device_id mvebu_cpuidle_ids[] = {
	{
		.name = "cpuidle-armada-xp",
		.driver_data = (unsigned long)&armadaxp_idle_driver,
	}, {
		.name = "cpuidle-armada-370",
		.driver_data = (unsigned long)&armada370_idle_driver,
	}, {
		.name = "cpuidle-armada-38x",
		.driver_data = (unsigned long)&armada38x_idle_driver,
	},
	{}
};

static struct platform_driver mvebu_cpuidle_driver = {
	.probe = mvebu_v7_cpuidle_probe,
	.driver = {
		.name = "cpuidle-mbevu",
		.suppress_bind_attrs = true,
	},
	.id_table = mvebu_cpuidle_ids,
};

builtin_platform_driver(mvebu_cpuidle_driver);

MODULE_AUTHOR("Gregory CLEMENT <gregory.clement@free-electrons.com>");
MODULE_DESCRIPTION("Marvell EBU v7 cpuidle driver");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 *  cpuidle-powernv - idle state cpuidle driver.
 *  Adapted from drivers/cpuidle/cpuidle-pseries
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/cpuidle.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/clockchips.h>
#include <linux/of.h>
#include <linux/slab.h>

#include <asm/machdep.h>
#include <asm/firmware.h>
#include <asm/opal.h>
#include <asm/runlatch.h>

#define MAX_POWERNV_IDLE_STATES	8

struct cpuidle_driver powernv_idle_driver = {
	.name             = "powernv_idle",
	.owner            = THIS_MODULE,
};

static int max_idle_state;
static struct cpuidle_state *cpuidle_state_table;
static u64 default_snooze_timeout;
static bool snooze_timeout_en;

static u64 get_snooze_timeout(struct cpuidle_device *dev,
			      struct cpuidle_driver *drv,
			      int index)
{
	int i;

	if (unlikely(!snooze_timeout_en))
		return default_snooze_timeout;

	for (i = index + 1; i < drv->state_count; i++) {
		struct cpuidle_state *s = &drv->states[i];
		struct cpuidle_state_usage *su = &dev->states_usage[i];

		if (s->disabled || su->disable)
			continue;

		return s->target_residency * tb_ticks_per_usec;
	}

	return default_snooze_timeout;
}

static int snooze_loop(struct cpuidle_device *dev,
			struct cpuidle_driver *drv,
			int index)
{
	u64 snooze_exit_time;

	local_irq_enable();
	set_thread_flag(TIF_POLLING_NRFLAG);

	snooze_exit_time = get_tb() + get_snooze_timeout(dev, drv, index);
	ppc64_runlatch_off();
	while (!need_resched()) {
		HMT_low();
		HMT_very_low();
		if (snooze_timeout_en && get_tb() > snooze_exit_time)
			break;
	}

	HMT_medium();
	ppc64_runlatch_on();
	clear_thread_flag(TIF_POLLING_NRFLAG);
	smp_mb();
	return index;
}

static int nap_loop(struct cpuidle_device *dev,
			struct cpuidle_driver *drv,
			int index)
{
	ppc64_runlatch_off();
	power7_idle();
	ppc64_runlatch_on();
	return index;
}

/* Register for fastsleep only in oneshot mode of broadcast */
#ifdef CONFIG_TICK_ONESHOT
static int fastsleep_loop(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	unsigned long old_lpcr = mfspr(SPRN_LPCR);
	unsigned long new_lpcr;

	if (unlikely(system_state < SYSTEM_RUNNING))
		return index;

	new_lpcr = old_lpcr;
	/* Do not exit powersave upon decrementer as we've setup the timer
	 * offload.
	 */
	new_lpcr &= ~LPCR_PECE1;

	mtspr(SPRN_LPCR, new_lpcr);
	power7_sleep();

	mtspr(SPRN_LPCR, old_lpcr);

	return index;
}
#endif
/*
 * States for dedicated partition case.
 */
static struct cpuidle_state powernv_states[MAX_POWERNV_IDLE_STATES] = {
	{ /* Snooze */
		.name = "snooze",
		.desc = "snooze",
		.exit_latency = 0,
		.target_residency = 0,
		.enter = &snooze_loop },
};

static int powernv_cpuidle_add_cpu_notifier(struct notifier_block *n,
			unsigned long action, void *hcpu)
{
	int hotcpu = (unsigned long)hcpu;
	struct cpuidle_device *dev =
				per_cpu(cpuidle_devices, hotcpu);

	if (dev && cpuidle_get_driver()) {
		switch (action) {
		case CPU_ONLINE:
		case CPU_ONLINE_FROZEN:
			cpuidle_pause_and_lock();
			cpuidle_enable_device(dev);
			cpuidle_resume_and_unlock();
			break;

		case CPU_DEAD:
		case CPU_DEAD_FROZEN:
			cpuidle_pause_and_lock();
			cpuidle_disable_device(dev);
			cpuidle_resume_and_unlock();
			break;

		default:
			return NOTIFY_DONE;
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block setup_hotplug_notifier = {
	.notifier_call = powernv_cpuidle_add_cpu_notifier,
};

/*
 * powernv_cpuidle_driver_init()
 */
static int powernv_cpuidle_driver_init(void)
{
	int idle_state;
	struct cpuidle_driver *drv = &powernv_idle_driver;

	drv->state_count = 0;

	for (idle_state = 0; idle_state < max_idle_state; ++idle_state) {
		/* Is the state not enabled? */
		if (cpuidle_state_table[idle_state].enter == NULL)
			continue;

		drv->states[drv->state_count] =	/* structure copy */
			cpuidle_state_table[idle_state];

		drv->state_count += 1;
	}

	/*
	 * On the PowerNV platform cpu_present may be less than cpu_possible in
	 * cases when firmware detects the CPU, but it is not available to the
	 * OS.  If CONFIG_HOTPLUG_CPU=n, then such CPUs are not hotplugable at
	 * run time and hence cpu_devices are not created for those CPUs by the
	 * generic topology_init().
	 *
	 * drv->cpumask defaults to cpu_possible_mask in
	 * __cpuidle_driver_init().  This breaks cpuidle on PowerNV where
	 * cpu_devices are not created for CPUs in cpu_possible_mask that
	 * cannot be hot-added later at run time.
	 *
	 * Trying cpuidle_register_device() on a CPU without a cpu_device is
	 * incorrect, so pass a correct CPU mask to the generic cpuidle driver.
	 */

	drv->cpumask = (struct cpumask *)cpu_present_mask;

	return 0;
}

static int powernv_add_idle_states(void)
{
	struct device_node *power_mgt;
	int nr_idle_states = 1; /* Snooze */
	int dt_idle_states;
	u32 *latency_ns, *residency_ns, *flags;
	int i, rc;

	/* Currently we have snooze statically defined */

	power_mgt = of_find_node_by_path("/ibm,opal/power-mgt");
	if (!power_mgt) {
		pr_warn("opal: PowerMgmt Node not found\n");
		goto out;
	}

	/* Read values of any property to determine the num of idle states */
	dt_idle_states = of_property_count_u32_elems(power_mgt, "ibm,cpu-idle-state-flags");
	if (dt_idle_states < 0) {
		pr_warn("cpuidle-powernv