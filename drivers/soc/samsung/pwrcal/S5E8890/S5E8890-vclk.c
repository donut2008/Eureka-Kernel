/*
 * Hisilicon Platforms Using ACPU CPUFreq Support
 *
 * Copyright (c) 2015 Hisilicon Limited.
 * Copyright (c) 2015 Linaro Limited.
 *
 * Leo Yan <leo.yan@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static int __init hisi_acpu_cpufreq_driver_init(void)
{
	struct platform_device *pdev;

	if (!of_machine_is_compatible("hisilicon,hi6220"))
		return -ENODEV;

	pdev = platform_device_register_simple("cpufreq-dt", -1, NULL, 0);
	return PTR_ERR_OR_ZERO(pdev);
}
module_init(hisi_acpu_cpufreq_driver_init);

MODULE_AUTHOR("Leo Yan <leo.yan@linaro.org>");
MODULE_DESCRIPTION("Hisilicon acpu cpufreq driver");
MODULE_LICENSE("GPL v2");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 * This file provides the ACPI based P-state support. This
 * module works with generic cpufreq infrastructure. Most of
 * the code is based on i386 version
 * (arch/i386/kernel/cpu/cpufreq/acpi-cpufreq.c)
 *
 * Copyright (C) 2005 Intel Corp
 *      Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/pal.h>

#include <linux/acpi.h>
#include <acpi/processor.h>

MODULE_AUTHOR("Venkatesh Pallipadi");
MODULE_DESCRIPTION("ACPI Processor P-States Driver");
MODULE_LICENSE("GPL");


struct cpufreq_acpi_io {
	struct acpi_processor_performance	acpi_data;
	unsigned int				resume;
};

static struct cpufreq_acpi_io	*acpi_io_data[NR_CPUS];

static struct cpufreq_driver acpi_cpufreq_driver;


static int
processor_set_pstate (
	u32	value)
{
	s64 retval;

	pr_debug("processor_set_pstate\n");

	retval = ia64_pal_set_pstate((u64)value);

	if (retval) {
		pr_debug("Failed to set freq to 0x%x, with error 0x%lx\n",
		        value, retval);
		return -ENODEV;
	}
	return (int)retval;
}


static int
processor_get_pstate (
	u32	*value)
{
	u64	pstate_index = 0;
	s64 	retval;

	pr_debug("processor_get_pstate\n");

	retval = ia64_pal_get_pstate(&pstate_index,
	                             PAL_GET_PSTATE_TYPE_INSTANT);
	*value = (u32) pstate_index;

	if (retval)
		pr_debug("Failed to get current freq with "
			"error 0x%lx, idx 0x%x\n", retval, *value);

	return (int)retval;
}


/* To be used only after data->acpi_data is initialized */
static unsigned
extract_clock (
	struct cpufreq_acpi_io *data,
	unsigned value,
	unsigned int cpu)
{
	unsigned long i;

	pr_debug("extract_clock\n");

	for (i = 0; i < data->acpi_data.state_count; i++) {
		if (value == data->acpi_data.states[i].status)
			return data->acpi_data.states[i].core_frequency;
	}
	return data->acpi_data.states[i-1].core_frequency;
}


static unsigned int
processor_get_freq (
	struct cpufreq_acpi_io	*data,
	unsigned int		cpu)
{
	int			ret = 0;
	u32			value = 0;
	cpumask_t		saved_mask;
	unsigned long 		clock_freq;

	pr_debug("processor_get_freq\n");

	saved_mask = current->cpus_allowed;
	set_cpus_allowed_ptr(current, cpumask_of(cpu));
	if (smp_processor_id() != cpu)
		goto migrate_end;

	/* processor_get_pstate gets the instantaneous frequency */
	ret = processor_get_pstate(&value);

	if (ret) {
		set_cpus_allowed_ptr(current, &saved_mask);
		printk(KERN_WARNING "get performance failed with error %d\n",
		       ret);
		ret = 0;
		goto migrate_end;
	}
	clock_freq = extract_clock(data, value, cpu);
	ret = (clock_freq*1000);

migrate_end:
	set_cpus_allowed_ptr(current, &saved_mask);
	return ret;
}


static int
processor_set_freq (
	struct cpufreq_acpi_io	*data,
	struct cpufreq_policy   *policy,
	int			state)
{
	int			ret = 0;
	u32			value = 0;
	cpumask_t		saved_mask;
	int			retval;

	pr_debug("processor_set_freq\n");

	saved_mask = current->cpus_allowed;
	set_cpus_allowed_ptr(current, cpumask_of(policy->cpu));
	if (smp_processor_id() != policy->cpu) {
		retval = -EAGAIN;
		goto migrate_end;
	}

	if (state == data->acpi_data.state) {
		if (unlikely(data->resume)) {
			pr_debug("Called after resume, resetting to P%d\n", state);
			data->resume = 0;
		} else {
			pr_debug("Already at target state (P%d)\n", state);
			retval = 0;
			goto migrate_end;
		}
	}

	pr_debug("Transitioning from P%d to P%d\n",
		data->acpi_data.state, state);

	/*
	 * First we write the target state's 'control' value to the
	 * control_register.
	 */

	value = (u32) data->acpi_data.states[state].control;

	pr_debug("Transitioning to state: 0x%08x\n", value);

	ret = processor_set_pstate(value);
	if (ret) {
		printk(KERN_WARNING "Transition failed with error %d\n", ret);
		retval = -ENODEV;
		goto migrate_end;
	}

	data->acpi_data.state = state;

	retval = 0;

migrate_end:
	set_cpus_allowed_ptr(current, &saved_mask);
	return (retval);
}


static unsigned int
acpi_cpufreq_get (
	unsigned int		cpu)
{
	struct cpufreq_acpi_io *data = acpi_io_data[cpu];

	pr_debug("acpi_cpufreq_get\n");

	return processor_get_freq(data, cpu);
}


static int
acpi_cpufreq_target (
	struct cpufreq_policy   *policy,
	unsigned int index)
{
	return processor_set_freq(acpi_io_data[policy->cpu], policy, index);
}

static int
acpi_cpufreq_cpu_init (
	struct cpufreq_policy   *policy)
{
	unsigned int		i;
	unsigned int		cpu = policy->cpu;
	struct cpufreq_acpi_io	*data;
	unsigned int		result = 0;
	struct cpufreq_frequency_table *freq_table;

	pr_debug("acpi_cpufreq_cpu_init\n");

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return (-ENOMEM);

	acpi_io_data[cpu] = data;

	result = acpi_processor_register_performance(&data->acpi_data, cpu);

	if (result)
		goto err_free;

	/* capability check */
	if (data->acpi_data.state_count <= 1) {
		pr_debug("No P-States\n");
		result = -ENODEV;
		goto err_unreg;
	}

	if ((data->acpi_data.control_register.space_id !=
					ACPI_ADR_SPACE_FIXED_HARDWARE) ||
	    (data->acpi_data.status_register.space_id !=
					ACPI_ADR_SPACE_FIXED_HARDWARE)) {
		pr_debug("Unsupported address space [%d, %d]\n",
			(u32) (data->acpi_data.control_register.space_id),
			(u32) (data->acpi_data.status_register.space_id));
		result = -ENODEV;
		goto err_unreg;
	}

	/* alloc freq_table */
	freq_table = kzalloc(sizeof(*freq_table) *
	                           (data->acpi_data.state_count + 1),
	                           GFP_KERNEL);
	if (!freq_table) {
		result = -ENOMEM;
		goto err_unreg;
	}

	/* detect transition latency */
	policy->cpuinfo.transition_latency = 0;
	for (i=0; i<data->acpi_data.state_count; i++) {
		if ((data->acpi_data.states[i].transition_latency * 1000) >
		    policy->cpuinfo.transition_latency) {
			policy->cpuinfo.transition_latency =
			    data->acpi_data.states[i].transition_latency * 1000;
		}
	}

	/* table init */
	for (i = 0; i <= data->acpi_data.state_count; i++)
	{
		if (i < data->acpi_data.state_count) {
			freq_table[i].frequency =
			      data->acpi_data.states[i].core_frequency * 1000;
		} else {
			freq_table[i].frequency = CPUFREQ_TABLE_END;
		}
	}

	result = cpufreq_table_validate_and_show(policy, freq_table);
	if (result) {
		goto err_freqfree;
	}

	/* notify BIOS that we exist */
	acpi_processor_notify_smm(THIS_MODULE);

	printk(KERN_INFO "acpi-cpufreq: CPU%u - ACPI performance management "
	       "activated.\n", cpu);

	for (i = 0; i < data->acpi_data.state_count; i++)
		pr_debug("     %cP%d: %d MHz, %d mW, %d uS, %d uS, 0x%x 0x%x\n",
			(i == data->acpi_data.state?'*':' '), i,
			(u32) data->acpi_data.states[i].core_frequency,
			(u32) data->acpi_data.states[i].power,
			(u32) data->acpi_data.states[i].transition_latency,
			(u32) data->acpi_data.states[i].bus_master_latency,
			(u32) data->acpi_data.states[i].status,
			(u32) data->acpi_data.states[i].control);

	/* the first call to ->target() should result in us actually
	 * writing something to the appropriate registers. */
	data->resume = 1;

	return (result);

 err_freqfree:
	kfree(freq_table);
 err_unreg:
	acpi_processor_unregister_performance(cpu);
 err_free:
	kfree(data);
	acpi_io_data[cpu] = NULL;

	return (result);
}


static int
acpi_cpufreq_cpu_exit (
	struct cpufreq_policy   *policy)
{
	struct cpufreq_acpi_io *data = acpi_io_data[policy->cpu];

	pr_debug("acpi_cpufreq_cpu_exit\n");

	if (data) {
		acpi_io_data[policy->cpu] = NULL;
		acpi_processor_unregister_performance(policy->cpu);
		kfree(policy->freq_table);
		kfree(data);
	}

	return (0);
}


static struct cpufreq_driver acpi_cpufreq_driver = {
	.verify 	= cpufreq_generic_frequency_table_verify,
	.target_index	= acpi_cpufreq_target,
	.get 		= acpi_cpufreq_get,
	.init		= acpi_cpufreq_cpu_init,
	.exit		= acpi_cpufreq_cpu_exit,
	.name		= "acpi-cpufreq",
	.attr		= cpufreq_generic_attr,
};


static int __init
acpi_cpufreq_init (void)
{
	pr_debug("acpi_cpufreq_init\n");

 	return cpufreq_register_driver(&acpi_cpufreq_driver);
}


static void __exit
acpi_cpufreq_exit (void)
{
	pr_debug("acpi_cpufreq_exit\n");

	cpufreq_unregister_driver(&acpi_cpufreq_driver);
	return;
}


late_initcall(acpi_cpufreq_init);
module_exit(acpi_cpufreq_exit);

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * Copyright (C) 2013 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_opp.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#define PU_SOC_VOLTAGE_NORMAL	1250000
#define PU_SOC_VOLTAGE_HIGH	1275000
#define FREQ_1P2_GHZ		1200000000

static struct regulator *arm_reg;
static struct regulator *pu_reg;
static struct regulator *soc_reg;

static struct clk *arm_clk;
static struct clk *pll1_sys_clk;
static struct clk *pll1_sw_clk;
static struct clk *step_clk;
static struct clk *pll2_pfd2_396m_clk;

/* clk used by i.MX6UL */
static struct clk *pll2_bus_clk;
static struct clk *secondary_sel_clk;

static struct device *cpu_dev;
static bool free_opp;
static struct cpufreq_frequency_table *freq_table;
static unsigned int transition_latency;

static u32 *imx6_soc_volt;
static u32 soc_opp_count;

static int imx6q_set_target(struct cpufreq_policy *policy, unsigned int index)
{
	struct dev_pm_opp *opp;
	unsigned long freq_hz, volt, volt_old;
	unsigned int old_freq, new_freq;
	int ret;

	new_freq = freq_table[index].frequency;
	freq_hz = new_freq * 1000;
	old_freq = clk_get_rate(arm_clk) / 1000;

	rcu_read_lock();
	opp = dev_pm_opp_find_freq_ceil(cpu_dev, &freq_hz);
	if (IS_ERR(opp)) {
		rcu_read_unlock();
		dev_err(cpu_dev, "failed to find OPP for %ld\n", freq_hz);
		return PTR_ERR(opp);
	}

	volt = dev_pm_opp_get_voltage(opp);
	rcu_read_unlock();
	volt_old = regulator_get_voltage(arm_reg);

	dev_dbg(cpu_dev, "%u MHz, %ld mV --> %u MHz, %ld mV\n",
		old_freq / 1000, volt_old / 1000,
		new_freq / 1000, volt / 1000);

	/* scaling up?  scale voltage before frequency */
	if (new_freq > old_freq) {
		if (!IS_ERR(pu_reg)) {
			ret = regulator_set_voltage_tol(pu_reg, imx6_soc_volt[index], 0);
			if (ret) {
				dev_err(cpu_dev, "failed to scale vddpu up: %d\n", ret);
				return ret;
			}
		}
		ret = regulator_set_voltage_tol(soc_reg, imx6_soc_volt[index], 0);
		if (ret) {
			dev_err(cpu_dev, "failed to scale vddsoc up: %d\n", ret);
			return ret;
		}
		ret = regulator_set_voltage_tol(arm_reg, volt, 0);
		if (ret) {
			dev_err(cpu_dev,
				"failed to scale vddarm up: %d\n", ret);
			return ret;
		}
	}

	/*
	 * The setpoints are selected per PLL/PDF frequencies, so we need to
	 * reprogram PLL for frequency scaling.  The procedure of reprogramming
	 * PLL1 is as below.
	 * For i.MX6UL, it has a secondary clk mux, the cpu frequency change
	 * flow is slightly different from other i.MX6 OSC.
	 * The cpu frequeny change flow for i.MX6(except i.MX6UL) is as below:
	 *  - Enable pll2_pfd2_396m_clk and reparent pll1_sw_clk to it
	 *  - Reprogram pll1_sys_clk and reparent pll1_sw_clk back to it
	 *  - Disable pll2_pfd2_396m_clk
	 */
	if (of_machine_is_compatible("fsl,imx6ul")) {
		/*
		 * When changing pll1_sw_clk's parent to pll1_sys_clk,
		 * CPU may run at higher than 528MHz, this will lead to
		 * the system unstable if the voltage is lower than the
		 * voltage of 528MHz, so lower the CPU frequency to one
		 * half before changing CPU frequency.
		 */
		clk_set_rate(arm_clk, (old_freq >> 1) * 1000);
		clk_set_parent(pll1_sw_clk, pll1_sys_clk);
		if (freq_hz > clk_get_rate(pll2_pfd2_396m_clk))
			clk_set_parent(secondary_sel_clk, pll2_bus_clk);
		else
			clk_set_parent(secondary_sel_clk, pll2_pfd2_396m_clk);
		clk_set_parent(step_clk, secondary_sel_clk);
		clk_set_parent(pll1_sw_clk, step_clk);
	} else {
		clk_set_parent(step_clk, pll2_pfd2_396m_clk);
		clk_set_parent(pll1_sw_clk, step_clk);
		if (freq_hz > clk_get_rate(pll2_pfd2_396m_clk)) {
			clk_set_rate(pll1_sys_clk, new_freq * 1000);
			clk_set_parent(pll1_sw_clk, pll1_sys_clk);
		}
	}

	/* Ensure the arm clock divider is what we expect */
	ret = clk_set_rate(arm_clk, new_freq * 1000);
	if (ret) {
		int ret1;

		dev_err(cpu_dev, "failed to set clock rate: %d\n", ret);
		ret1 = regulator_set_voltage_tol(arm_reg, volt_old, 0);
		if (ret1)
			dev_warn(cpu_dev,
				 "failed to restore vddarm voltage: %d\n", ret1);
		return ret;
	}

	/* scaling down?  scale voltage after frequency */
	if (new_freq < old_freq) {
		ret = regulator_set_voltage_tol(arm_reg, volt, 0);
		if (ret) {
			dev_warn(cpu_dev,
				 "failed to scale vddarm down: %d\n", ret);
			ret = 0;
		}
		ret = regulator_set_voltage_tol(soc_reg, imx6_soc_volt[index], 0);
		if (ret) {
			dev_warn(cpu_dev, "failed to scale vddsoc down: %d\n", ret);
			ret = 0;
		}
		if (!IS_ERR(pu_reg)) {
			ret = regulator_set_voltage_tol(pu_reg, imx6_soc_volt[index], 0);
			if (ret) {
				dev_warn(cpu_dev, "failed to scale vddpu down: %d\n", ret);
				ret = 0;
			}
		}
	}

	return 0;
}

static int imx6q_cpufreq_init(struct cpufreq_policy *policy)
{
	policy->clk = arm_clk;
	return cpufreq_generic_init(policy, freq_table, transition_latency);
}

static struct cpufreq_driver imx6q_cpufreq_driver = {
	.flags = CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify = cpufreq_generic_frequency_table_verify,
	.target_index = imx6q_set_target,
	.get = cpufreq_generic_get,
	.init = imx6q_cpufreq_init,
	.name = "imx6q-cpufreq",
	.attr = cpufreq_generic_attr,
};

static int imx6q_cpufreq_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct dev_pm_opp *opp;
	unsigned long min_volt, max_volt;
	int num, ret;
	const struct property *prop;
	const __be32 *val;
	u32 nr, i, j;

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev) {
		pr_err("failed to get cpu0 device\n");
		return -ENODEV;
	}

	np = of_node_get(cpu_dev->of_node);
	if (!np) {
		dev_err(cpu_dev, "failed to find cpu0 node\n");
		return -ENOENT;
	}

	arm_clk = clk_get(cpu_dev, "arm");
	pll1_sys_clk = clk_get(cpu_dev, "pll1_sys");
	pll1_sw_clk = clk_get(cpu_dev, "pll1_sw");
	step_clk = clk_get(cpu_dev, "step");
	pll2_pfd2_396m_clk = clk_get(cpu_dev, "pll2_pfd2_396m");
	if (IS_ERR(arm_clk) || IS_ERR(pll1_sys_clk) || IS_ERR(pll1_sw_clk) ||
	    IS_ERR(step_clk) || IS_ERR(pll2_pfd2_396m_clk)) {
		dev_err(cpu_dev, "failed to get clocks\n");
		ret = -ENOENT;
		goto put_clk;
	}

	if (of_machine_is_compatible("fsl,imx6ul")) {
		pll2_bus_clk = clk_get(cpu_dev, "pll2_bus");
		secondary_sel_clk = clk_get(cpu_dev, "secondary_sel");
		if (IS_ERR(pll2_bus_clk) || IS_ERR(secondary_sel_clk)) {
			dev_err(cpu_dev, "failed to get clocks specific to imx6ul\n");
			ret = -ENOENT;
			goto put_clk;
		}
	}

	arm_reg = regulator_get(cpu_dev, "arm");
	pu_reg = regulator_get_optional(cpu_dev, "pu");
	soc_reg = regulator_get(cpu_dev, "soc");
	if (IS_ERR(arm_reg) || IS_ERR(soc_reg)) {
		dev_err(cpu_dev, "failed to get regulators\n");
		ret = -ENOENT;
		goto put_reg;
	}

	/*
	 * We expect an OPP table supplied by platform.
	 * Just, incase the platform did not supply the OPP
	 * table, it will try to get it.
	 */
	num = dev_pm_opp_get_opp_count(cpu_dev);
	if (num < 0) {
		ret = dev_pm_opp_of_add_table(cpu_dev);
		if (ret < 0) {
			dev_err(cpu_dev, "failed to init OPP table: %d\n", ret);
			goto put_reg;
		}

		/* Because we have added the OPPs here, we must free them */
		free_opp = true;

		num = dev_pm_opp_get_opp_count(cpu_dev);
		if (num < 0) {
			ret = num;
			dev_err(cpu_dev, "no OPP table is found: %d\n", ret);
			goto out_free_opp;
		}
	}

	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &freq_table);
	if (ret) {
		dev_err(cpu_dev, "failed to init cpufreq table: %d\n", ret);
		goto put_reg;
	}

	/* Make imx6_soc_volt array's size same as arm opp number */
	imx6_soc_volt = devm_kzalloc(cpu_dev, sizeof(*imx6_soc_volt) * num, GFP_KERNEL);
	if (imx6_soc_volt == NULL) {
		ret = -ENOMEM;
		goto free_freq_table;
	}

	prop = of_find_property(np, "fsl,soc-operating-points", NULL);
	if (!prop || !prop->value)
		goto soc_opp_out;

	/*
	 * Each OPP is a set of tuples consisting of frequency and
	 * voltage like <freq-kHz vol-uV>.
	 */
	nr = prop->length / sizeof(u32);
	if (nr % 2 || (nr / 2) < num)
		goto soc_opp_out;

	for (j = 0; j < num; j++) {
		val = prop->value;
		for (i = 0; i < nr / 2; i++) {
			unsigned long freq = be32_to_cpup(val++);
			unsigned long volt = be32_to_cpup(val++);
			if (freq_table[j].frequency == freq) {
				imx6_soc_volt[soc_opp_count++] = volt;
				break;
			}
		}
	}

soc_opp_out:
	/* use fixed soc opp volt if no valid soc opp info found in dtb */
	if (soc_opp_count != num) {
		dev_warn(cpu_dev, "can NOT find valid fsl,soc-operating-points property in dtb, use default value!\n");
		for (j = 0; j < num; j++)
			imx6_soc_volt[j] = PU_SOC_VOLTAGE_NORMAL;
		if (freq_table[num - 1].frequency * 1000 == FREQ_1P2_GHZ)
			imx6_soc_volt[num - 1] = PU_SOC_VOLTAGE_HIGH;
	}

	if (of_property_read_u32(np, "clock-latency", &transition_latency))
		transition_latency = CPUFREQ_ETERNAL;

	/*
	 * Calculate the ramp time for max voltage change in the
	 * VDDSOC and VDDPU regulators.
	 */
	ret = regulator_set_voltage_time(soc_reg, imx6_soc_volt[0], imx6_soc_volt[num - 1]);
	if (ret > 0)
		transition_latency += ret * 1000;
	if (!IS_ERR(pu_reg)) {
		ret = regulator_set_voltage_time(pu_reg, imx6_soc_volt[0], imx6_soc_volt[num - 1]);
		if (ret > 0)
			transition_latency += ret * 1000;
	}

	/*
	 * OPP is maintained in order of increasing frequency, and
	 * freq_table initialised from OPP is therefore sorted in the
	 * same order.
	 */
	rcu_read_lock();
	opp = dev_pm_opp_find_freq_exact(cpu_dev,
				  freq_table[0].frequency * 1000, true);
	min_volt = dev_pm_opp_get_voltage(opp);
	opp = dev_pm_opp_find_freq_exact(cpu_dev,
				  freq_table[--num].frequency * 1000, true);
	max_volt = dev_pm_opp_get_voltage(opp);
	rcu_read_unlock();
	ret = regulator_set_voltage_time(arm_reg, min_volt, max_volt);
	if (ret > 0)
		transition_latency += ret * 1000;

	ret = cpufreq_register_driver(&imx6q_cpufreq_driver);
	if (ret) {
		dev_err(cpu_dev, "failed register driver: %d\n", ret);
		goto free_freq_table;
	}

	of_node_put(np);
	return 0;

free_freq_table:
	dev_pm_opp_free_cpufreq_table(cpu_dev, &freq_table);
out_free_opp:
	if (free_opp)
		dev_pm_opp_of_remove_table(cpu_dev);
put_reg:
	if (!IS_ERR(arm_reg))
		regulator_put(arm_reg);
	if (!IS_ERR(pu_reg))
		regulator_put(pu_reg);
	if (!IS_ERR(soc_reg))
		regulator_put(soc_reg);
put_clk:
	if (!IS_ERR(arm_clk))
		clk_put(arm_clk);
	if (!IS_ERR(pll1_sys_clk))
		clk_put(pll1_sys_clk);
	if (!IS_ERR(pll1_sw_clk))
		clk_put(pll1_sw_clk);
	if (!IS_ERR(step_clk))
		clk_put(step_clk);
	if (!IS_ERR(pll2_pfd2_396m_clk))
		clk_put(pll2_pfd2_396m_clk);
	if (!IS_ERR(pll2_bus_clk))
		clk_put(pll2_bus_clk);
	if (!IS_ERR(secondary_sel_clk))
		clk_put(secondary_sel_clk);
	of_node_put(np);
	return ret;
}

static int imx6q_cpufreq_remove(struct platform_device *pdev)
{
	cpufreq_unregister_driver(&imx6q_cpufreq_driver);
	dev_pm_opp_free_cpufreq_table(cpu_dev, &freq_table);
	if (free_opp)
		dev_pm_opp_of_remove_table(cpu_dev);
	regulator_put(arm_reg);
	if (!IS_ERR(pu_reg))
		regulator_put(pu_reg);
	regulator_put(soc_reg);
	clk_put(arm_clk);
	clk_put(pll1_sys_clk);
	clk_put(pll1_sw_clk);
	clk_put(step_clk);
	clk_put(pll2_pfd2_396m_clk);
	clk_put(pll2_bus_clk);
	clk_put(secondary_sel_clk);

	return 0;
}

static struct platform_driver imx6q_cpufreq_platdrv = {
	.driver = {
		.name	= "imx6q-cpufreq",
	},
	.probe		= imx6q_cpufreq_probe,
	.remove		= imx6q_cpufreq_remove,
};
module_platform_driver(imx6q_cpufreq_platdrv);

MODULE_AUTHOR("Shawn Guo <shawn.guo@linaro.org>");
MODULE_DESCRIPTION("Freescale i.MX6Q cpufreq driver");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 *  Copyright (C) 2001-2002 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * CPU support functions
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/mach-types.h>
#include <asm/hardware/icst.h>

static void __iomem *cm_base;
/* The cpufreq driver only use the OSC register */
#define INTEGRATOR_HDR_OSC_OFFSET       0x08
#define INTEGRATOR_HDR_LOCK_OFFSET      0x14

static struct cpufreq_driver integrator_driver;

static const struct icst_params lclk_params = {
	.ref		= 24000000,
	.vco_max	= ICST525_VCO_MAX_5V,
	.vco_min	= ICST525_VCO_MIN,
	.vd_min		= 8,
	.vd_max		= 132,
	.rd_min		= 24,
	.rd_max		= 24,
	.s2div		= icst525_s2div,
	.idx2s		= icst525_idx2s,
};

static const struct icst_params cclk_params = {
	.ref		= 24000000,
	.vco_max	= ICST525_VCO_MAX_5V,
	.vco_min	= ICST525_VCO_MIN,
	.vd_min		= 12,
	.vd_max		= 160,
	.rd_min		= 24,
	.rd_max		= 24,
	.s2div		= icst525_s2div,
	.idx2s		= icst525_idx2s,
};

/*
 * Validate the speed policy.
 */
static int integrator_verify_policy(struct cpufreq_policy *policy)
{
	struct icst_vco vco;

	cpufreq_verify_within_cpu_limits(policy);

	vco = icst_hz_to_vco(&cclk_params, policy->max * 1000);
	policy->max = icst_hz(&cclk_params, vco) / 1000;

	vco = icst_hz_to_vco(&cclk_params, policy->min * 1000);
	policy->min = icst_hz(&cclk_params, vco) / 1000;

	cpufreq_verify_within_cpu_limits(policy);
	return 0;
}


static int integrator_set_target(struct cpufreq_policy *policy,
				 unsigned int target_freq,
				 unsigned int relation)
{
	cpumask_t cpus_allowed;
	int cpu = policy->cpu;
	struct icst_vco vco;
	struct cpufreq_freqs freqs;
	u_int cm_osc;

	/*
	 * Save this threads cpus_allowed mask.
	 */
	cpus_allowed = current->cpus_allowed;

	/*
	 * Bind to the specified CPU.  When this call returns,
	 * we should be running on the right CPU.
	 */
	set_cpus_allowed_ptr(current, cpumask_of(cpu));
	BUG_ON(cpu != smp_processor_id());

	/* get current setting */
	cm_osc = __raw_readl(cm_base + INTEGRATOR_HDR_OSC_OFFSET);

	if (machine_is_integrator())
		vco.s = (cm_osc >> 8) & 7;
	else if (machine_is_cintegrator())
		vco.s = 1;
	vco.v = cm_osc & 255;
	vco.r = 22;
	freqs.old = icst_hz(&cclk_params, vco) / 1000;

	/* icst_hz_to_vco rounds down -- so we need the next
	 * larger freq in case of CPUFREQ_RELATION_L.
	 */
	if (relation == CPUFREQ_RELATION_L)
		target_freq += 999;
	if (target_freq > policy->max)
		target_freq = policy->max;
	vco = icst_hz_to_vco(&cclk_params, target_freq * 1000);
	freqs.new = icst_hz(&cclk_params, vco) / 1000;

	if (freqs.old == freqs.new) {
		set_cpus_allowed_ptr(current, &cpus_allowed);
		return 0;
	}

	cpufreq_freq_transition_begin(policy, &freqs);

	cm_osc = __raw_readl(cm_base + INTEGRATOR_HDR_OSC_OFFSET);

	if (machine_is_integrator()) {
		cm_osc &= 0xfffff800;
		cm_osc |= vco.s << 8;
	} else if (machine_is_cintegrator()) {
		cm_osc &= 0xffffff00;
	}
	cm_osc |= vco.v;

	__raw_writel(0xa05f, cm_base + INTEGRATOR_HDR_LOCK_OFFSET);
	__raw_writel(cm_osc, cm_base + INTEGRATOR_HDR_OSC_OFFSET);
	__raw_writel(0, cm_base + INTEGRATOR_HDR_LOCK_OFFSET);

	/*
	 * Restore the CPUs allowed mask.
	 */
	set_cpus_allowed_ptr(current, &cpus_allowed);

	cpufreq_freq_transition_end(policy, &freqs, 0);

	return 0;
}

static unsigned int integrator_get(unsigned int cpu)
{
	cpumask_t cpus_allowed;
	unsigned int current_freq;
	u_int cm_osc;
	struct icst_vco vco;

	cpus_allowed = current->cpus_allowed;

	set_cpus_allowed_ptr(current, cpumask_of(cpu));
	BUG_ON(cpu != smp_processor_id());

	/* detect memory etc. */
	cm_osc = __raw_readl(cm_base + INTEGRATOR_HDR_OSC_OFFSET);

	if (machine_is_integrator())
		vco.s = (cm_osc >> 8) & 7;
	else
		vco.s = 1;
	vco.v = cm_osc & 255;
	vco.r = 22;

	current_freq = icst_hz(&cclk_params, vco) / 1000; /* current freq */

	set_cpus_allowed_ptr(current, &cpus_allowed);

	return current_freq;
}

static int integrator_cpufreq_init(struct cpufreq_policy *policy)
{

	/* set default policy and cpuinfo */
	policy->max = policy->cpuinfo.max_freq = 160000;
	policy->min = policy->cpuinfo.min_freq = 12000;
	policy->cpuinfo.transition_latency = 1000000; /* 1 ms, assumed */

	return 0;
}

static struct cpufreq_driver integrator_driver = {
	.flags		= CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= integrator_verify_policy,
	.target		= integrator_set_target,
	.get		= integrator_get,
	.init		= integrator_cpufreq_init,
	.name		= "integrator",
};

static int __init integrator_cpufreq_probe(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	cm_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!cm_base)
		return -ENODEV;

	return cpufreq_register_driver(&integrator_driver);
}

static int __exit integrator_cpufreq_remove(struct platform_device *pdev)
{
	return cpufreq_unregister_driver(&integrator_driver);
}

static const struct of_device_id integrator_cpufreq_match[] = {
	{ .compatible = "arm,core-module-integrator"},
	{ },
};

MODULE_DEVICE_TABLE(of, integrator_cpufreq_match);

static struct platform_driver integrator_cpufreq_driver = {
	.driver = {
		.name = "integrator-cpufreq",
		.of_match_table = integrator_cpufreq_match,
	},
	.remove = __exit_p(integrator_cpufreq_remove),
};

module_platform_driver_probe(integrator_cpufreq_driver,
			     integrator_cpufreq_probe);

MODULE_AUTHOR("Russell M. King");
MODULE_DESCRIPTION("cpufreq driver for ARM Integrator CPUs");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * intel_pstate.c: Native P state management for Intel processors
 *
 * (C) Copyright 2012 Intel Corporation
 * Author: Dirk Brandewie <dirk.j.brandewie@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/acpi.h>
#include <linux/vmalloc.h>
#include <trace/events/power.h>

#include <asm/div64.h>
#include <asm/msr.h>
#include <asm/cpu_device_id.h>
#include <asm/cpufeature.h>

#define ATOM_RATIOS		0x66a
#define ATOM_VIDS		0x66b
#define ATOM_TURBO_RATIOS	0x66c
#define ATOM_TURBO_VIDS		0x66d

#define FRAC_BITS 8
#define int_tofp(X) ((int64_t)(X) << FRAC_BITS)
#define fp_toint(X) ((X) >> FRAC_BITS)

static inline int32_t mul_fp(int32_t x, int32_t y)
{
	return ((int64_t)x * (int64_t)y) >> FRAC_BITS;
}

static inline int32_t div_fp(s64 x, s64 y)
{
	return div64_s64((int64_t)x << FRAC_BITS, y);
}

static inline int ceiling_fp(int32_t x)
{
	int mask, ret;

	ret = fp_toint(x);
	mask = (1 << FRAC_BITS) - 1;
	if (x & mask)
		ret += 1;
	return ret;
}

struct sample {
	int32_t core_pct_busy;
	u64 aperf;
	u64 mperf;
	u64 tsc;
	int freq;
	ktime_t time;
};

struct pstate_data {
	int	current_pstate;
	int	min_pstate;
	int	max_pstate;
	int	max_pstate_physical;
	int	scaling;
	int	turbo_pstate;
};

struct vid_data {
	int min;
	int max;
	int turbo;
	int32_t ratio;
};

struct _pid {
	int setpoint;
	int32_t integral;
	int32_t p_gain;
	int32_t i_gain;
	int32_t d_gain;
	int deadband;
	int32_t last_err;
};

struct cpudata {
	int cpu;

	struct timer_list timer;

	struct pstate_data pstate;
	struct vid_data vid;
	struct _pid pid;

	ktime_t last_sample_time;
	u64	prev_aperf;
	u64	prev_mperf;
	u64	prev_tsc;
	struct sample sample;
};

static struct cpudata **all_cpu_data;
struct pstate_adjust_policy {
	int sample_rate_ms;
	int deadband;
	int setpoint;
	int p_gain_pct;
	int d_gain_pct;
	int i_gain_pct;
};

struct pstate_funcs {
	int (*get_max)(void);
	int (*get_max_physical)(void);
	int (*get_min)(void);
	int (*get_turbo)(void);
	int (*get_scaling)(void);
	void (*set)(struct cpudata*, int pstate);
	void (*get_vid)(struct cpudata *);
};

struct cpu_defaults {
	struct pstate_adjust_policy pid_policy;
	struct pstate_funcs funcs;
};

static struct pstate_adjust_policy pid_params;
static struct pstate_funcs pstate_funcs;
static int hwp_active;

struct perf_limits {
	int no_turbo;
	int turbo_disabled;
	int max_perf_pct;
	int min_perf_pct;
	int32_t max_perf;
	int32_t min_perf;
	int max_policy_pct;
	int max_sysfs_pct;
	int min_policy_pct;
	int min_sysfs_pct;
};

static struct perf_limits performance_limits = {
	.no_turbo = 0,
	.turbo_disabled = 0,
	.max_perf_pct = 100,
	.max_perf = int_tofp(1),
	.min_perf_pct = 100,
	.min_perf = int_tofp(1),
	.max_policy_pct = 100,
	.max_sysfs_pct = 100,
	.min_policy_pct = 0,
	.min_sysfs_pct = 0,
};

static struct perf_limits powersave_limits = {
	.no_turbo = 0,
	.turbo_disabled = 0,
	.max_perf_pct = 100,
	.max_perf = int_tofp(1),
	.min_perf_pct = 0,
	.min_perf = 0,
	.max_policy_pct = 100,
	.max_sysfs_pct = 100,
	.min_policy_pct = 0,
	.min_sysfs_pct = 0,
};

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_PERFORMANCE
static struct perf_limits *limits = &performance_limits;
#else
static struct perf_limits *limits = &powersave_limits;
#endif

static inline void pid_reset(struct _pid *pid, int setpoint, int busy,
			     int deadband, int integral) {
	pid->setpoint = setpoint;
	pid->deadband  = deadband;
	pid->integral  = int_tofp(integral);
	pid->last_err  = int_tofp(setpoint) - int_tofp(busy);
}

static inline void pid_p_gain_set(struct _pid *pid, int percent)
{
	pid->p_gain = div_fp(int_tofp(percent), int_tofp(100));
}

static inline void pid_i_gain_set(struct _pid *pid, int percent)
{
	pid->i_gain = div_fp(int_tofp(percent), int_tofp(100));
}

static inline void pid_d_gain_set(struct _pid *pid, int percent)
{
	pid->d_gain = div_fp(int_tofp(percent), int_tofp(100));
}

static signed int pid_calc(struct _pid *pid, int32_t busy)
{
	signed int result;
	int32_t pterm, dterm, fp_error;
	int32_t integral_limit;

	fp_error = int_tofp(pid->setpoint) - busy;

	if (abs(fp_error) <= int_tofp(pid->deadband))
		return 0;

	pterm = mul_fp(pid->p_gain, fp_error);

	pid->integral += fp_error;

	/*
	 * We limit the integral here so that it will never
	 * get higher than 30.  This prevents it from becoming
	 * too large an input over long periods of time and allows
	 * it to get factored out sooner.
	 *
	 * The value of 30 was chosen through experimentation.
	 */
	integral_limit = int_tofp(30);
	if (pid->integral > integral_limit)
		pid->integral = integral_limit;
	if (pid->integral < -integral_limit)
		pid->integral = -integral_limit;

	dterm = mul_fp(pid->d_gain, fp_error - pid->last_err);
	pid->last_err = fp_error;

	result = pterm + mul_fp(pid->integral, pid->i_gain) + dterm;
	result = result + (1 << (FRAC_BITS-1));
	return (signed int)fp_toint(result);
}

static inline void intel_pstate_busy_pid_reset(struct cpudata *cpu)
{
	pid_p_gain_set(&cpu->pid, pid_params.p_gain_pct);
	pid_d_gain_set(&cpu->pid, pid_params.d_gain_pct);
	pid_i_gain_set(&cpu->pid, pid_params.i_gain_pct);

	pid_reset(&cpu->pid, pid_params.setpoint, 100, pid_params.deadband, 0);
}

static inline void intel_pstate_reset_all_pid(void)
{
	unsigned int cpu;

	for_each_online_cpu(cpu) {
		if (all_cpu_data[cpu])
			intel_pstate_busy_pid_reset(all_cpu_data[cpu]);
	}
}

static inline void update_turbo_state(void)
{
	u64 misc_en;
	struct cpudata *cpu;

	cpu = all_cpu_data[0];
	rdmsrl(MSR_IA32_MISC_ENABLE, misc_en);
	limits->turbo_disabled =
		(misc_en & MSR_IA32_MISC_ENABLE_TURBO_DISABLE ||
		 cpu->pstate.max_pstate == cpu->pstate.turbo_pstate);
}

static void intel_pstate_hwp_set(void)
{
	int min, hw_min, max, hw_max, cpu, range, adj_range;
	u64 value, cap;

	get_online_cpus();

	for_each_online_cpu(cpu) {
		rdmsrl_on_cpu(cpu, MSR_HWP_CAPABILITIES, &cap);
		hw_min = HWP_LOWEST_PERF(cap);
		hw_max = HWP_HIGHEST_PERF(cap);
		range = hw_max - hw_min;

		rdmsrl_on_cpu(cpu, MSR_HWP_REQUEST, &value);
		adj_range = limits->min_perf_pct * range / 100;
		min = hw_min + adj_range;
		value &= ~HWP_MIN_PERF(~0L);
		value |= HWP_MIN_PERF(min);

		adj_range = limits->max_perf_pct * range / 100;
		max = hw_min + adj_range;
		if (limits->no_turbo) {
			hw_max = HWP_GUARANTEED_PERF(cap);
			if (hw_max < max)
				max = hw_max;
		}

		value &= ~HWP_MAX_PERF(~0L);
		value |= HWP_MAX_PERF(max);
		wrmsrl_on_cpu(cpu, MSR_HWP_REQUEST, value);
	}

	put_online_cpus();
}

/************************** debugfs begin ************************/
static int pid_param_set(void *data, u64 val)
{
	*(u32 *)data = val;
	intel_pstate_reset_all_pid();
	return 0;
}

static int pid_param_get(void *data, u64 *val)
{
	*val = *(u32 *)data;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_pid_param, pid_param_get, pid_param_set, "%llu\n");

struct pid_param {
	char *name;
	void *value;
};

static struct pid_param pid_files[] = {
	{"sample_rate_ms", &pid_params.sample_rate_ms},
	{"d_gain_pct", &pid_params.d_gain_pct},
	{"i_gain_pct", &pid_params.i_gain_pct},
	{"deadband", &pid_params.deadband},
	{"setpoint", &pid_params.setpoint},
	{"p_gain_pct", &pid_params.p_gain_pct},
	{NULL, NULL}
};

static void __init intel_pstate_debug_expose_params(void)
{
	struct dentry *debugfs_parent;
	int i = 0;

	if (hwp_active)
		return;
	debugfs_parent = debugfs_create_dir("pstate_snb", NULL);
	if (IS_ERR_OR_NULL(debugfs_parent))
		return;
	while (pid_files[i].name) {
		debugfs_create_file(pid_files[i].name, 0660,
				    debugfs_parent, pid_files[i].value,
				    &fops_pid_param);
		i++;
	}
}

/************************** debugfs end ************************/

/************************** sysfs begin ************************/
#define show_one(file_name, object)					\
	static ssize_t show_##file_name					\
	(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
	{								\
		return sprintf(buf, "%u\n", limits->object);		\
	}

static ssize_t show_turbo_pct(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct cpudata *cpu;
	int total, no_turbo, turbo_pct;
	uint32_t turbo_fp;

	cpu = all_cpu_data[0];

	total = cpu->pstate.turbo_pstate - cpu->pstate.min_pstate + 1;
	no_turbo = cpu->pstate.max_pstate - cpu->pstate.min_pstate + 1;
	turbo_fp = div_fp(int_tofp(no_turbo), int_tofp(total));
	turbo_pct = 100 - fp_toint(mul_fp(turbo_fp, int_tofp(100)));
	return sprintf(buf, "%u\n", turbo_pct);
}

static ssize_t show_num_pstates(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct cpudata *cpu;
	int total;

	cpu = all_cpu_data[0];
	total = cpu->pstate.turbo_pstate - cpu->pstate.min_pstate + 1;
	return sprintf(buf, "%u\n", total);
}

static ssize_t show_no_turbo(struct kobject *kobj,
			     struct kobj_attribute *attr, char *buf)
{
	ssize_t ret;

	update_turbo_state();
	if (limits->turbo_disabled)
		ret = sprintf(buf, "%u\n", limits->turbo_disabled);
	else
		ret = sprintf(buf, "%u\n", limits->no_turbo);

	return ret;
}

static ssize_t store_no_turbo(struct kobject *a, struct kobj_attribute *b,
			      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	update_turbo_state();
	if (limits->turbo_disabled) {
		pr_warn("intel_pstate: Turbo disabled by BIOS or unavailable on processor\n");
		return -EPERM;
	}

	limits->no_turbo = clamp_t(int, input, 0, 1);

	if (hwp_active)
		intel_pstate_hwp_set();

	return count;
}

static ssize_t store_max_perf_pct(struct kobject *a, struct kobj_attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	limits->max_sysfs_pct = clamp_t(int, input, 0 , 100);
	limits->max_perf_pct = min(limits->max_policy_pct,
				   limits->max_sysfs_pct);
	limits->max_perf_pct = max(limits->min_policy_pct,
				   limits->max_perf_pct);
	limits->max_perf_pct = max(limits->min_perf_pct,
				   limits->max_perf_pct);
	limits->max_perf = div_fp(int_tofp(limits->max_perf_pct),
				  int_tofp(100));

	if (hwp_active)
		intel_pstate_hwp_set();
	return count;
}

static ssize_t store_min_perf_pct(struct kobject *a, struct kobj_attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	limits->min_sysfs_pct = clamp_t(int, input, 0 , 100);
	limits->min_perf_pct = max(limits->min_policy_pct,
				   limits->min_sysfs_pct);
	limits->min_perf_pct = min(limits->max_policy_pct,
				   limits->min_perf_pct);
	limits->min_perf_pct = min(limits->max_perf_pct,
				   limits->min_perf_pct);
	limits->min_perf = div_fp(int_tofp(limits->min_perf_pct),
				  int_tofp(100));

	if (hwp_active)
		intel_pstate_hwp_set();
	return count;
}

show_one(max_perf_pct, max_perf_pct);
show_one(min_perf_pct, min_perf_pct);

define_one_global_rw(no_turbo);
define_one_global_rw(max_perf_pct);
define_one_global_rw(min_perf_pct);
define_one_global_ro(turbo_pct);
define_one_global_ro(num_pstates);

static struct attribute *intel_pstate_attributes[] = {
	&no_turbo.attr,
	&max_perf_pct.attr,
	&min_perf_pct.attr,
	&turbo_pct.attr,
	&num_pstates.attr,
	NULL
};

static struct attribute_group intel_pstate_attr_group = {
	.attrs = intel_pstate_attributes,
};

static void __init intel_pstate_sysfs_expose_params(void)
{
	struct kobject *intel_pstate_kobject;
	int rc;

	intel_pstate_kobject = kobject_create_and_add("intel_pstate",
						&cpu_subsys.dev_root->kobj);
	BUG_ON(!intel_pstate_kobject);
	rc = sysfs_create_group(intel_pstate_kobject, &intel_pstate_attr_group);
	BUG_ON(rc);
}
/************************** sysfs end ************************/

static void intel_pstate_hwp_enable(struct cpudata *cpudata)
{
	wrmsrl_on_cpu(cpudata->cpu, MSR_PM_ENABLE, 0x1);
}

static int atom_get_min_pstate(void)
{
	u64 value;

	rdmsrl(ATOM_RATIOS, value);
	return (value >> 8) & 0x7F;
}

static int atom_get_max_pstate(void)
{
	u64 value;

	rdmsrl(ATOM_RATIOS, value);
	return (value >> 16) & 0x7F;
}

static int atom_get_turbo_pstate(void)
{
	u64 value;

	rdmsrl(ATOM_TURBO_RATIOS, value);
	return value & 0x7F;
}

static void atom_set_pstate(struct cpudata *cpudata, int pstate)
{
	u64 val;
	int32_t vid_fp;
	u32 vid;

	val = (u64)pstate << 8;
	if (limits->no_turbo && !limits->turbo_disabled)
		val |= (u64)1 << 32;

	vid_fp = cpudata->vid.min + mul_fp(
		int_tofp(pstate - cpudata->pstate.min_pstate),
		cpudata->vid.ratio);

	vid_fp = clamp_t(int32_t, vid_fp, cpudata->vid.min, cpudata->vid.max);
	vid = ceiling_fp(vid_fp);

	if (pstate > cpudata->pstate.max_pstate)
		vid = cpudata->vid.turbo;

	val |= vid;

	wrmsrl_on_cpu(cpudata->cpu, MSR_IA32_PERF_CTL, val);
}

static int silvermont_get_scaling(void)
{
	u64 value;
	int i;
	/* Defined in Table 35-6 from SDM (Sept 2015) */
	static int silvermont_freq_table[] = {
		83300, 100000, 133300, 116700, 80000};

	rdmsrl(MSR_FSB_FREQ, value);
	i = value & 0x7;
	WARN_ON(i > 4);

	return silvermont_freq_table[i];
}

static int airmont_get_scaling(void)
{
	u64 value;
	int i;
	/* Defined in Table 35-10 from SDM (Sept 2015) */
	static int airmont_freq_table[] = {
		83300, 100000, 133300, 116700, 80000,
		93300, 90000, 88900, 87500};

	rdmsrl(MSR_FSB_FREQ, value);
	i = value & 0xF;
	WARN_ON(i > 8);

	return airmont_freq_table[i];
}

static void atom_get_vid(struct cpudata *cpudata)
{
	u64 value;

	rdmsrl(ATOM_VIDS, value);
	cpudata->vid.min = int_tofp((value >> 8) & 0x7f);
	cpudata->vid.max = int_tofp((value >> 16) & 0x7f);
	cpudata->vid.ratio = div_fp(
		cpudata->vid.max - cpudata->vid.min,
		int_tofp(cpudata->pstate.max_pstate -
			cpudata->pstate.min_pstate));

	rdmsrl(ATOM_TURBO_VIDS, value);
	cpudata->vid.turbo = value & 0x7f;
}

static int core_get_min_pstate(void)
{
	u64 value;

	rdmsrl(MSR_PLATFORM_INFO, value);
	return (value >> 40) & 0xFF;
}

static int core_get_max_pstate_physical(void)
{
	u64 value;

	rdmsrl(MSR_PLATFORM_INFO, value);
	return (value >> 8) & 0xFF;
}

static int core_get_max_pstate(void)
{
	u64 tar;
	u64 plat_info;
	int max_pstate;
	int err;

	rdmsrl(MSR_PLATFORM_INFO, plat_info);
	max_pstate = (plat_info >> 8) & 0xFF;

	err = rdmsrl_safe(MSR_TURBO_ACTIVATION_RATIO, &tar);
	if (!err) {
		/* Do some sanity checking for safety */
		if (plat_info & 0x600000000) {
			u64 tdp_ctrl;
			u64 tdp_ratio;
			int tdp_msr;

			err = rdmsrl_safe(MSR_CONFIG_TDP_CONTROL, &tdp_ctrl);
			if (err)
				goto skip_tar;

			tdp_msr = MSR_CONFIG_TDP_NOMINAL + (tdp_ctrl & 0x3);
			err = rdmsrl_safe(tdp_msr, &tdp_ratio);
			if (err)
				goto skip_tar;

			/* For level 1 and 2, bits[23:16] contain the ratio */
			if (tdp_ctrl)
				tdp_ratio >>= 16;

			tdp_ratio &= 0xff; /* ratios are only 8 bits long */
			if (tdp_ratio - 1 == tar) {
				max_pstate = tar;
				pr_debug("max_pstate=TAC %x\n", max_pstate);
			} else {
				goto skip_tar;
			}
		}
	}

skip_tar:
	return max_pstate;
}

static int core_get_turbo_pstate(void)
{
	u64 value;
	int nont, ret;

	rdmsrl(MSR_NHM_TURBO_RATIO_LIMIT, value);
	nont = core_get_max_pstate();
	ret = (value) & 255;
	if (ret <= nont)
		ret = nont;
	return ret;
}

static inline int core_get_scaling(void)
{
	return 100000;
}

static void core_set_pstate(struct cpudata *cpudata, int pstate)
{
	u64 val;

	val = (u64)pstate << 8;
	if (limits->no_turbo && !limits->turbo_disabled)
		val |= (u64)1 << 32;

	wrmsrl_on_cpu(cpudata->cpu, MSR_IA32_PERF_CTL, val);
}

static int knl_get_turbo_pstate(void)
{
	u64 value;
	int nont, ret;

	rdmsrl(MSR_NHM_TURBO_RATIO_LIMIT, value);
	nont = core_get_max_pstate();
	ret = (((value) >> 8) & 0xFF);
	if (ret <= nont)
		ret = nont;
	return ret;
}

static struct cpu_defaults core_params = {
	.pid_policy = {
		.sample_rate_ms = 10,
		.deadband = 0,
		.setpoint = 97,
		.p_gain_pct = 20,
		.d_gain_pct = 0,
		.i_gain_pct = 0,
	},
	.funcs = {
		.get_max = core_get_max_pstate,
		.get_max_physical = core_get_max_pstate_physical,
		.get_min = core_get_min_pstate,
		.get_turbo = core_get_turbo_pstate,
		.get_scaling = core_get_scaling,
		.set = core_set_pstate,
	},
};

static struct cpu_defaults silvermont_params = {
	.pid_policy = {
		.sample_rate_ms = 10,
		.deadband = 0,
		.setpoint = 60,
		.p_gain_pct = 14,
		.d_gain_pct = 0,
		.i_gain_pct = 4,
	},
	.funcs = {
		.get_max = atom_get_max_pstate,
		.get_max_physical = atom_get_max_pstate,
		.get_min = atom_get_min_pstate,
		.get_turbo = atom_get_turbo_pstate,
		.set = atom_set_pstate,
		.get_scaling = silvermont_get_scaling,
		.get_vid = atom_get_vid,
	},
};

static struct cpu_defaults airmont_params = {
	.pid_policy = {
		.sample_rate_ms = 10,
		.deadband = 0,
		.setpoint = 60,
		.p_gain_pct = 14,
		.d_gain_pct = 0,
		.i_gain_pct = 4,
	},
	.funcs = {
		.get_max = atom_get_max_pstate,
		.get_max_physical = atom_get_max_pstate,
		.get_min = atom_get_min_pstate,
		.get_turbo = atom_get_turbo_pstate,
		.set = atom_set_pstate,
		.get_scaling = airmont_get_scaling,
		.get_vid = atom_get_vid,
	},
};

static struct cpu_defaults knl_params = {
	.pid_policy = {
		.sample_rate_ms = 10,
		.deadband = 0,
		.setpoint = 97,
		.p_gain_pct = 20,
		.d_gain_pct = 0,
		.i_gain_pct = 0,
	},
	.funcs = {
		.get_max = core_get_max_pstate,
		.get_max_physical = core_get_max_pstate_physical,
		.get_min = core_get_min_pstate,
		.get_turbo = knl_get_turbo_pstate,
		.get_scaling = core_get_scaling,
		.set = core_set_pstate,
	},
};

static void intel_pstate_get_min_max(struct cpudata *cpu, int *min, int *max)
{
	int max_perf = cpu->pstate.turbo_pstate;
	int max_perf_adj;
	int min_perf;

	if (limits->no_turbo || limits->turbo_disabled)
		max_perf = cpu->pstate.max_pstate;

	/*
	 * performance can be limited by user through sysfs, by cpufreq
	 * policy, or by cpu specific default values determined through
	 * experimentation.
	 */
	max_perf_adj = fp_toint(mul_fp(int_tofp(max_perf), limits->max_perf));
	*max = clamp_t(int, max_perf_adj,
			cpu->pstate.min_pstate, cpu->pstate.turbo_pstate);

	min_perf = fp_toint(mul_fp(int_tofp(max_perf), limits->min_perf));
	*min = clamp_t(int, min_perf, cpu->pstate.min_pstate, max_perf);
}

static void intel_pstate_set_pstate(struct cpudata *cpu, int pstate, bool force)
{
	int max_perf, min_perf;

	if (force) {
		update_turbo_state();

		intel_pstate_get_min_max(cpu, &min_perf, &max_perf);

		pstate = clamp_t(int, pstate, min_perf, max_perf);

		if (pstate == cpu->pstate.current_pstate)
			return;
	}
	trace_cpu_frequency(pstate * cpu->pstate.scaling, cpu->cpu);

	cpu->pstate.current_pstate = pstate;

	pstate_funcs.set(cpu, pstate);
}

static void intel_pstate_get_cpu_pstates(struct cpudata *cpu)
{
	cpu->pstate.min_pstate = pstate_funcs.get_min();
	cpu->pstate.max_pstate = pstate_funcs.get_max();
	cpu->pstate.max_pstate_physical = pstate_funcs.get_max_physical();
	cpu->pstate.turbo_pstate = pstate_funcs.get_turbo();
	cpu->pstate.scaling = pstate_funcs.get_scaling();

	if (pstate_funcs.get_vid)
		pstate_funcs.get_vid(cpu);
	intel_pstate_set_pstate(cpu, cpu->pstate.min_pstate, false);
}

static inline void intel_pstate_calc_busy(struct cpudata *cpu)
{
	struct sample *sample = &cpu->sample;
	int64_t core_pct;

	core_pct = int_tofp(sample->aperf) * int_tofp(100);
	core_pct = div64_u64(core_pct, int_tofp(sample->mperf));

	sample->freq = fp_toint(
		mul_fp(int_tofp(
			cpu->pstate.max_pstate_physical *
			cpu->pstate.scaling / 100),
			core_pct));

	sample->core_pct_busy = (int32_t)core_pct;
}

static inline void intel_pstate_sample(struct cpudata *cpu)
{
	u64 aperf, mperf;
	unsigned long flags;
	u64 tsc;

	local_irq_save(flags);
	rdmsrl(MSR_IA32_APERF, aperf);
	rdmsrl(MSR_IA32_MPERF, mperf);
	if (cpu->prev_mperf == mperf) {
		local_irq_restore(flags);
		return;
	}

	tsc = rdtsc();
	local_irq_restore(flags);

	cpu->last_sample_time = cpu->sample.time;
	cpu->sample.time = ktime_get();
	cpu->sample.aperf = aperf;
	cpu->sample.mperf = mperf;
	cpu->sample.tsc =  tsc;
	cpu->sample.aperf -= cpu->prev_aperf;
	cpu->sample.mperf -= cpu->prev_mperf;
	cpu->sample.tsc -= cpu->prev_tsc;

	intel_pstate_calc_busy(cpu);

	cpu->prev_aperf = aperf;
	cpu->prev_mperf = mperf;
	cpu->prev_tsc = tsc;
}

static inline void intel_hwp_set_sample_time(struct cpudata *cpu)
{
	int delay;

	delay = msecs_to_jiffies(50);
	mod_timer_pinned(&cpu->timer, jiffies + delay);
}

static inline void intel_pstate_set_sample_time(struct cpudata *cpu)
{
	int delay;

	delay = msecs_to_jiffies(pid_params.sample_rate_ms);
	mod_timer_pinned(&cpu->timer, jiffies + delay);
}

static inline int32_t intel_pstate_get_scaled_busy(struct cpudata *cpu)
{
	int32_t core_busy, max_pstate, current_pstate, sample_ratio;
	s64 duration_us;
	u32 sample_time;

	/*
	 * core_busy is the ratio of actual performance to max
	 * max_pstate is the max non turbo pstate available
	 * current_pstate was the pstate that was requested during
	 * 	the last sample period.
	 *
	 * We normalize core_busy, which was our actual percent
	 * performance to what we requested during the last sample
	 * period. The result will be a percentage of busy at a
	 * specified pstate.
	 */
	core_busy = cpu->sample.core_pct_busy;
	max_pstate = int_tofp(cpu->pstate.max_pstate_physical);
	current_pstate = int_tofp(cpu->pstate.current_pstate);
	core_busy = mul_fp(core_busy, div_fp(max_pstate, current_pstate));

	/*
	 * Since we have a deferred timer, it will not fire unless
	 * we are in C0.  So, determine if the actual elapsed time
	 * is significantly greater (3x) than our sample interval.  If it
	 * is, then we were idle for a long enough period of time
	 * to adjust our busyness.
	 */
	sample_time = pid_params.sample_rate_ms  * USEC_PER_MSEC;
	duration_us = ktime_us_delta(cpu->sample.time,
				     cpu->last_sample_time);
	if (duration_us > sample_time * 3) {
		sample_ratio = div_fp(int_tofp(sample_time),
				      int_tofp(duration_us));
		core_busy = mul_fp(core_busy, sample_ratio);
	}

	return core_busy;
}

static inline void intel_pstate_adjust_busy_pstate(struct cpudata *cpu)
{
	int32_t busy_scaled;
	struct _pid *pid;
	signed int ctl;
	int from;
	struct sample *sample;

	from = cpu->pstate.current_pstate;

	pid = &cpu->pid;
	busy_scaled = intel_pstate_get_scaled_busy(cpu);

	ctl = pid_calc(pid, busy_scaled);

	/* Negative values of ctl increase the pstate and vice versa */
	intel_pstate_set_pstate(cpu, cpu->pstate.current_pstate - ctl, true);

	sample = &cpu->sample;
	trace_pstate_sample(fp_toint(sample->core_pct_busy),
		fp_toint(busy_scaled),
		from,
		cpu->pstate.current_pstate,
		sample->mperf,
		sample->aperf,
		sample->tsc,
		sample->freq);
}

static void intel_hwp_timer_func(unsigned long __data)
{
	struct cpudata *cpu = (struct cpudata *) __data;

	intel_pstate_sample(cpu);
	intel_hwp_set_sample_time(cpu);
}

static void intel_pstate_timer_func(unsigned long __data)
{
	struct cpudata *cpu = (struct cpudata *) __data;

	intel_pstate_sample(cpu);

	intel_pstate_adjust_busy_pstate(cpu);

	intel_pstate_set_sample_time(cpu);
}

#define ICPU(model, policy) \
	{ X86_VENDOR_INTEL, 6, model, X86_FEATURE_APERFMPERF,\
			(unsigned long)&policy }

static const struct x86_cpu_id intel_pstate_cpu_ids[] = {
	ICPU(0x2a, core_params),
	ICPU(0x2d, core_params),
	ICPU(0x37, silvermont_params),
	ICPU(0x3a, core_params),
	ICPU(0x3c, core_params),
	ICPU(0x3d, core_params),
	ICPU(0x3e, core_params),
	ICPU(0x3f, core_params),
	ICPU(0x45, core_params),
	ICPU(0x46, core_params),
	ICPU(0x47, core_params),
	ICPU(0x4c, airmont_params),
	ICPU(0x4e, core_params),
	ICPU(0x4f, core_params),
	ICPU(0x5e, core_params),
	ICPU(0x56, core_params),
	ICPU(0x57, knl_params),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, intel_pstate_cpu_ids);

static const struct x86_cpu_id intel_pstate_cpu_oob_ids[] = {
	ICPU(0x56, core_params),
	{}
};

static int intel_pstate_init_cpu(unsigned int cpunum)
{
	struct cpudata *cpu;

	if (!all_cpu_data[cpunum])
		all_cpu_data[cpunum] = kzalloc(sizeof(struct cpudata),
					       GFP_KERNEL);
	if (!all_cpu_data[cpunum])
		return -ENOMEM;

	cpu = all_cpu_data[cpunum];

	cpu->cpu = cpunum;

	if (hwp_active)
		intel_pstate_hwp_enable(cpu);

	intel_pstate_get_cpu_pstates(cpu);

	init_timer_deferrable(&cpu->timer);
	cpu->timer.data = (unsigned long)cpu;
	cpu->timer.expires = jiffies + HZ/100;

	if (!hwp_active)
		cpu->timer.function = intel_pstate_timer_func;
	else
		cpu->timer.function = intel_hwp_timer_func;

	intel_pstate_busy_pid_reset(cpu);
	intel_pstate_sample(cpu);

	add_timer_on(&cpu->timer, cpunum);

	pr_debug("intel_pstate: controlling: cpu %d\n", cpunum);

	return 0;
}

static unsigned int intel_pstate_get(unsigned int cpu_num)
{
	struct sample *sample;
	struct cpudata *cpu;

	cpu = all_cpu_data[cpu_num];
	if (!cpu)
		return 0;
	sample = &cpu->sample;
	return sample->freq;
}

static int intel_pstate_set_policy(struct cpufreq_policy *policy)
{
	if (!policy->cpuinfo.max_freq)
		return -ENODEV;

	if (policy->policy == CPUFREQ_POLICY_PERFORMANCE &&
	    policy->max >= policy->cpuinfo.max_freq) {
		pr_debug("intel_pstate: set performance\n");
		limits = &performance_limits;
		if (hwp_active)
			intel_pstate_hwp_set();
		return 0;
	}

	pr_debug("intel_pstate: set powersave\n");
	limits = &powersave_limits;
	limits->min_policy_pct = (policy->min * 100) / policy->cpuinfo.max_freq;
	limits->min_policy_pct = clamp_t(int, limits->min_policy_pct, 0 , 100);
	limits->max_policy_pct = DIV_ROUND_UP(policy->max * 100,
					      policy->cpuinfo.max_freq);
	limits->max_policy_pct = clamp_t(int, limits->max_policy_pct, 0 , 100);

	/* Normalize user input to [min_policy_pct, max_policy_pct] */
	limits->min_perf_pct = max(limits->min_policy_pct,
				   limits->min_sysfs_pct);
	limits->min_perf_pct = min(limits->max_policy_pct,
				   limits->min_perf_pct);
	limits->max_perf_pct = min(limits->max_policy_pct,
				   limits->max_sysfs_pct);
	limits->max_perf_pct = max(limits->min_policy_pct,
				   limits->max_perf_pct);
	limits->max_perf = round_up(limits->max_perf, FRAC_BITS);

	/* Make sure min_perf_pct <= max_perf_pct */
	limits->min_perf_pct = min(limits->max_perf_pct, limits->min_perf_pct);

	limits->min_perf = div_fp(int_tofp(limits->min_perf_pct),
				  int_tofp(100));
	limits->max_perf = div_fp(int_tofp(limits->max_perf_pct),
				  int_tofp(100));

	if (hwp_active)
		intel_pstate_hwp_set();

	return 0;
}

static int intel_pstate_verify_policy(struct cpufreq_policy *policy)
{
	cpufreq_verify_within_cpu_limits(policy);

	if (policy->policy != CPUFREQ_POLICY_POWERSAVE &&
	    policy->policy != CPUFREQ_POLICY_PERFORMANCE)
		return -EINVAL;

	return 0;
}

static void intel_pstate_stop_cpu(struct cpufreq_policy *policy)
{
	int cpu_num = policy->cpu;
	struct cpudata *cpu = all_cpu_data[cpu_num];

	pr_debug("intel_pstate: CPU %d exiting\n", cpu_num);

	del_timer_sync(&all_cpu_data[cpu_num]->timer);
	if (hwp_active)
		return;

	intel_pstate_set_pstate(cpu, cpu->pstate.min_pstate, false);
}

static int intel_pstate_cpu_init(struct cpufreq_policy *policy)
{
	struct cpudata *cpu;
	int rc;

	rc = intel_pstate_init_cpu(policy->cpu);
	if (rc)
		return rc;

	cpu = all_cpu_data[policy->cpu];

	if (limits->min_perf_pct == 100 && limits->max_perf_pct == 100)
		policy->policy = CPUFREQ_POLICY_PERFORMANCE;
	else
		policy->policy = CPUFREQ_POLICY_POWERSAVE;

	policy->min = cpu->pstate.min_pstate * cpu->pstate.scaling;
	policy->max = cpu->pstate.turbo_pstate * cpu->pstate.scaling;

	/* cpuinfo and default policy values */
	policy->cpuinfo.min_freq = cpu->pstate.min_pstate * cpu->pstate.scaling;
	policy->cpuinfo.max_freq =
		cpu->pstate.turbo_pstate * cpu->pstate.scaling;
	policy->cpuinfo.transition_latency = CPUFREQ_ETERNAL;
	cpumask_set_cpu(policy->cpu, policy->cpus);

	return 0;
}

static struct cpufreq_driver intel_pstate_driver = {
	.flags		= CPUFREQ_CONST_LOOPS,
	.verify		= intel_pstate_verify_policy,
	.setpolicy	= intel_pstate_set_policy,
	.get		= intel_pstate_get,
	.init		= intel_pstate_cpu_init,
	.stop_cpu	= intel_pstate_stop_cpu,
	.name		= "intel_pstate",
};

static int __initdata no_load;
static int __initdata no_hwp;
static int __initdata hwp_only;
static unsigned int force_load;

static int intel_pstate_msrs_not_valid(void)
{
	if (!pstate_funcs.get_max() ||
	    !pstate_funcs.get_min() ||
	    !pstate_funcs.get_turbo())
		return -ENODEV;

	return 0;
}

static void copy_pid_params(struct pstate_adjust_policy *policy)
{
	pid_params.sample_rate_ms = policy->sample_rate_ms;
	pid_params.p_gain_pct = policy->p_gain_pct;
	pid_params.i_gain_pct = policy->i_gain_pct;
	pid_params.d_gain_pct = policy->d_gain_pct;
	pid_params.deadband = policy->deadband;
	pid_params.setpoint = policy->setpoint;
}

static void copy_cpu_funcs(struct pstate_funcs *funcs)
{
	pstate_funcs.get_max   = funcs->get_max;
	pstate_funcs.get_max_physical = funcs->get_max_physical;
	pstate_funcs.get_min   = funcs->get_min;
	pstate_funcs.get_turbo = funcs->get_turbo;
	pstate_funcs.get_scaling = funcs->get_scaling;
	pstate_funcs.set       = funcs->set;
	pstate_funcs.get_vid   = funcs->get_vid;
}

#if IS_ENABLED(CONFIG_ACPI)
#include <acpi/processor.h>

static bool intel_pstate_no_acpi_pss(void)
{
	int i;

	for_each_possible_cpu(i) {
		acpi_status status;
		union acpi_object *pss;
		struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
		struct acpi_processor *pr = per_cpu(processors, i);

		if (!pr)
			continue;

		status = acpi_evaluate_object(pr->handle, "_PSS", NULL, &buffer);
		if (ACPI_FAILURE(status))
			continue;

		pss = buffer.pointer;
		if (pss && pss->type == ACPI_TYPE_PACKAGE) {
			kfree(pss);
			return false;
		}

		kfree(pss);
	}

	return true;
}

static bool intel_pstate_has_acpi_ppc(void)
{
	int i;

	for_each_possible_cpu(i) {
		struct acpi_processor *pr = per_cpu(processors, i);

		if (!pr)
			continue;
		if (acpi_has_method(pr->handle, "_PPC"))
			return true;
	}
	return false;
}

enum {
	PSS,
	PPC,
};

struct hw_vendor_info {
	u16  valid;
	char oem_id[ACPI_OEM_ID_SIZE];
	char oem_table_id[ACPI_OEM_TABLE_ID_SIZE];
	int  oem_pwr_table;
};

/* Hardware vendor-specific info that has its own power management modes */
static struct hw_vendor_info vendor_info[] = {
	{1, "HP    ", "ProLiant", PSS},
	{1, "ORACLE", "X4-2    ", PPC},
	{1, "ORACLE", "X4-2L   ", PPC},
	{1, "ORACLE", "X4-2B   ", PPC},
	{1, "ORACLE", "X3-2    ", PPC},
	{1, "ORACLE", "X3-2L   ", PPC},
	{1, "ORACLE", "X3-2B   ", PPC},
	{1, "ORACLE", "X4470M2 ", PPC},
	{1, "ORACLE", "X4270M3 ", PPC},
	{1, "ORACLE", "X4270M2 ", PPC},
	{1, "ORACLE", "X4170M2 ", PPC},
	{1, "ORACLE", "X4170 M3", PPC},
	{1, "ORACLE", "X4275 M3", PPC},
	{1, "ORACLE", "X6-2    ", PPC},
	{1, "ORACLE", "Sudbury ", PPC},
	{0, "", ""},
};

static bool intel_pstate_platform_pwr_mgmt_exists(void)
{
	struct acpi_table_header hdr;
	struct hw_vendor_info *v_info;
	const struct x86_cpu_id *id;
	u64 misc_pwr;

	id = x86_match_cpu(intel_pstate_cpu_oob_ids);
	if (id) {
		rdmsrl(MSR_MISC_PWR_MGMT, misc_pwr);
		if ( misc_pwr & (1 << 8))
			return true;
	}

	if (acpi_disabled ||
	    ACPI_FAILURE(acpi_get_table_header(ACPI_SIG_FADT, 0, &hdr)))
		return false;

	for (v_info = vendor_info; v_info->valid; v_info++) {
		if (!strncmp(hdr.oem_id, v_info->oem_id, ACPI_OEM_ID_SIZE) &&
			!strncmp(hdr.oem_table_id, v_info->oem_table_id,
						ACPI_OEM_TABLE_ID_SIZE))
			switch (v_info->oem_pwr_table) {
			case PSS:
				return intel_pstate_no_acpi_pss();
			case PPC:
				return intel_pstate_has_acpi_ppc() &&
					(!force_load);
			}
	}

	return false;
}
#else /* CONFIG_ACPI not enabled */
static inline bool intel_pstate_platform_pwr_mgmt_exists(void) { return false; }
static inline bool intel_pstate_has_acpi_ppc(void) { return false; }
#endif /* CONFIG_ACPI */

static const struct x86_cpu_id hwp_support_ids[] __initconst = {
	{ X86_VENDOR_INTEL, 6, X86_MODEL_ANY, X86_FEATURE_HWP },
	{}
};

static int __init intel_pstate_init(void)
{
	int cpu, rc = 0;
	const struct x86_cpu_id *id;
	struct cpu_defaults *cpu_def;

	if (no_load)
		return -ENODEV;

	if (x86_match_cpu(hwp_support_ids) && !no_hwp) {
		copy_cpu_funcs(&core_params.funcs);
		hwp_active++;
		goto hwp_cpu_matched;
	}

	id = x86_match_cpu(intel_pstate_cpu_ids);
	if (!id)
		return -ENODEV;

	cpu_def = (struct cpu_defaults *)id->driver_data;

	copy_pid_params(&cpu_def->pid_policy);
	copy_cpu_funcs(&cpu_def->funcs);

	if (intel_pstate_msrs_not_valid())
		return -ENODEV;

hwp_cpu_matched:
	/*
	 * The Intel pstate driver will be ignored if the platform
	 * firmware has its own power management modes.
	 */
	if (intel_pstate_platform_pwr_mgmt_exists())
		return -ENODEV;

	pr_info("Intel P-state driver initializing.\n");

	all_cpu_data = vzalloc(sizeof(void *) * num_possible_cpus());
	if (!all_cpu_data)
		return -ENOMEM;

	if (!hwp_active && hwp_only)
		goto out;

	rc = cpufreq_register_driver(&intel_pstate_driver);
	if (rc)
		goto out;

	intel_pstate_debug_expose_params();
	intel_pstate_sysfs_expose_params();

	if (hwp_active)
		pr_info("intel_pstate: HWP enabled\n");

	return rc;
out:
	get_online_cpus();
	for_each_online_cpu(cpu) {
		if (all_cpu_data[cpu]) {
			del_timer_sync(&all_cpu_data[cpu]->timer);
			kfree(all_cpu_data[cpu]);
		}
	}

	put_online_cpus();
	vfree(all_cpu_data);
	return -ENODEV;
}
device_initcall(intel_pstate_init);

static int __init intel_pstate_setup(char *str)
{
	if (!str)
		return -EINVAL;

	if (!strcmp(str, "disable"))
		no_load = 1;
	if (!strcmp(str, "no_hwp")) {
		pr_info("intel_pstate: HWP disabled\n");
		no_hwp = 1;
	}
	if (!strcmp(str, "force"))
		force_load = 1;
	if (!strcmp(str, "hwp_only"))
		hwp_only = 1;
	return 0;
}
early_param("intel_pstate", intel_pstate_setup);

MODULE_AUTHOR("Dirk Brandewie <dirk.j.brandewie@intel.com>");
MODULE_DESCRIPTION("'intel_pstate' - P state driver Intel Core processors");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 *	kirkwood_freq.c: cpufreq driver for the Marvell kirkwood
 *
 *	Copyright (C) 2013 Andrew Lunn <andrew@lunn.ch>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <asm/proc-fns.h>

#define CPU_SW_INT_BLK BIT(28)

static struct priv
{
	struct clk *cpu_clk;
	struct clk *ddr_clk;
	struct clk *powersave_clk;
	struct device *dev;
	void __iomem *base;
} priv;

#define STATE_CPU_FREQ 0x01
#define STATE_DDR_FREQ 0x02

/*
 * Kirkwood can swap the clock to the CPU between two clocks:
 *
 * - cpu clk
 * - ddr clk
 *
 * The frequencies are set at runtime before registering this table.
 */
static struct cpufreq_frequency_table kirkwood_freq_table[] = {
	{0, STATE_CPU_FREQ,	0}, /* CPU uses cpuclk */
	{0, STATE_DDR_FREQ,	0}, /* CPU uses ddrclk */
	{0, 0,			CPUFREQ_TABLE_END},
};

static unsigned int kirkwood_cpufreq_get_cpu_frequency(unsigned int cpu)
{
	return clk_get_rate(priv.powersave_clk) / 1000;
}

static int kirkwood_cpufreq_target(struct cpufreq_policy *policy,
			    unsigned int index)
{
	unsigned int state = kirkwood_freq_table[index].driver_data;
	unsigned long reg;

	local_irq_disable();

	/* Disable interrupts to the CPU */
	reg = readl_relaxed(priv.base);
	reg |= CPU_SW_INT_BLK;
	writel_relaxed(reg, priv.base);

	switch (state) {
	case STATE_CPU_FREQ:
		clk_set_parent(priv.powersave_clk, priv.cpu_clk);
		break;
	case STATE_DDR_FREQ:
		clk_set_parent(priv.powersave_clk, priv.ddr_clk);
		break;
	}

	/* Wait-for-Interrupt, while the hardware changes frequency */
	cpu_do_idle();

	/* Enable interrupts to the CPU */
	reg = readl_relaxed(priv.base);
	reg &= ~CPU_SW_INT_BLK;
	writel_relaxed(reg, priv.base);

	local_irq_enable();

	return 0;
}

/* Module init and exit code */
static int kirkwood_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	return cpufreq_generic_init(policy, kirkwood_freq_table, 5000);
}

static struct cpufreq_driver kirkwood_cpufreq_driver = {
	.flags	= CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.get	= kirkwood_cpufreq_get_cpu_frequency,
	.verify	= cpufreq_generic_frequency_table_verify,
	.target_index = kirkwood_cpufreq_target,
	.init	= kirkwood_cpufreq_cpu_init,
	.name	= "kirkwood-cpufreq",
	.attr	= cpufreq_generic_attr,
};

static int kirkwood_cpufreq_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct resource *res;
	int err;

	priv.dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv.base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv.base))
		return PTR_ERR(priv.base);

	np = of_cpu_device_node_get(0);
	if (!np) {
		dev_err(&pdev->dev, "failed to get cpu device node\n");
		return -ENODEV;
	}

	priv.cpu_clk = of_clk_get_by_name(np, "cpu_clk");
	if (IS_ERR(priv.cpu_clk)) {
		dev_err(priv.dev, "Unable to get cpuclk");
		return PTR_ERR(priv.cpu_clk);
	}

	clk_prepare_enable(priv.cpu_clk);
	kirkwood_freq_table[0].frequency = clk_get_rate(priv.cpu_clk) / 1000;

	priv.ddr_clk = of_clk_get_by_name(np, "ddrclk");
	if (IS_ERR(priv.ddr_clk)) {
		dev_err(priv.dev, "Unable to get ddrclk");
		err = PTR_ERR(priv.ddr_clk);
		goto out_cpu;
	}

	clk_prepare_enable(priv.ddr_clk);
	kirkwood_freq_table[1].frequency = clk_get_rate(priv.ddr_clk) / 1000;

	priv.powersave_clk = of_clk_get_by_name(np, "powersave");
	if (IS_ERR(priv.powersave_clk)) {
		dev_err(priv.dev, "Unable to get powersave");
		err = PTR_ERR(priv.powersave_clk);
		goto out_ddr;
	}
	clk_prepare_enable(priv.powersave_clk);

	of_node_put(np);
	np = NULL;

	err = cpufreq_register_driver(&kirkwood_cpufreq_driver);
	if (!err)
		return 0;

	dev_err(priv.dev, "Failed to register cpufreq driver");

	clk_disable_unprepare(priv.powersave_clk);
out_ddr:
	clk_disable_unprepare(priv.ddr_clk);
out_cpu:
	clk_disable_unprepare(priv.cpu_clk);
	of_node_put(np);

	return err;
}

static int kirkwood_cpufreq_remove(struct platform_device *pdev)
{
	cpufreq_unregister_driver(&kirkwood_cpufreq_driver);

	clk_disable_unprepare(priv.powersave_clk);
	clk_disable_unprepare(priv.ddr_clk);
	clk_disable_unprepare(priv.cpu_clk);

	return 0;
}

static struct platform_driver kirkwood_cpufreq_platform_driver = {
	.probe = kirkwood_cpufreq_probe,
	.remove = kirkwood_cpufreq_remove,
	.driver = {
		.name = "kirkwood-cpufreq",
	},
};

module_platform_driver(kirkwood_cpufreq_platform_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Andrew Lunn <andrew@lunn.ch");
MODULE_DESCRIPTION("cpufreq driver for Marvell's kirkwood CPU");
MODULE_ALIAS("platform:kirkwood-cpufreq");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 *  (C) 2001-2004  Dave Jones.
 *  (C) 2002  Padraig Brady. <padraig@antefacto.com>
 *
 *  Licensed under the terms of the GNU GPL License version 2.
 *  Based upon datasheets & sample CPUs kindly provided by VIA.
 *
 *  VIA have currently 3 different versions of Longhaul.
 *  Version 1 (Longhaul) uses the BCR2 MSR at 0x1147.
 *   It is present only in Samuel 1 (C5A), Samuel 2 (C5B) stepping 0.
 *  Version 2 of longhaul is backward compatible with v1, but adds
 *   LONGHAUL MSR for purpose of both frequency and voltage scaling.
 *   Present in Samuel 2 (steppings 1-7 only) (C5B), and Ezra (C5C).
 *  Version 3 of longhaul got renamed to Powersaver and redesigned
 *   to use only the POWERSAVER MSR at 0x110a.
 *   It is present in Ezra-T (C5M), Nehemiah (C5X) and above.
 *   It's pretty much the same feature wise to longhaul v2, though
 *   there is provision for scaling FSB too, but this doesn't work
 *   too well in practice so we don't even try to use this.
 *
 *  BIG FAT DISCLAIMER: Work in progress code. Possibly *dangerous*
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/timex.h>
#include <linux/io.h>
#include <linux/acpi.h>

#include <asm/msr.h>
#include <asm/cpu_device_id.h>
#include <acpi/processor.h>

#include "longhaul.h"

#define PFX "longhaul: "

#define TYPE_LONGHAUL_V1	1
#define TYPE_LONGHAUL_V2	2
#define TYPE_POWERSAVER		3

#define	CPU_SAMUEL	1
#define	CPU_SAMUEL2	2
#define	CPU_EZRA	3
#define	CPU_EZRA_T	4
#define	CPU_NEHEMIAH	5
#define	CPU_NEHEMIAH_C	6

/* Flags */
#define USE_ACPI_C3		(1 << 1)
#define USE_NORTHBRIDGE		(1 << 2)

static int cpu_model;
static unsigned int numscales = 16;
static unsigned int fsb;

static const struct mV_pos *vrm_mV_table;
static const unsigned char *mV_vrm_table;

static unsigned int highest_speed, lowest_speed; /* kHz */
static unsigned int minmult, maxmult;
static int can_scale_voltage;
static struct acpi_processor *pr;
static struct acpi_processor_cx *cx;
static u32 acpi_regs_addr;
static u8 longhaul_flags;
static unsigned int longhaul_index;

/* Module parameters */
static int scale_voltage;
static int disable_acpi_c3;
static int revid_errata;
static int enable;

/* Clock ratios multiplied by 10 */
static int mults[32];
static int eblcr[32];
static int longhaul_version;
static struct cpufreq_frequency_table *longhaul_table;

static char speedbuffer[8];

static char *print_speed(int speed)
{
	if (speed < 1000) {
		snprintf(speedbuffer, sizeof(speedbuffer), "%dMHz", speed);
		return speedbuffer;
	}

	if (speed%1000 == 0)
		snprintf(speedbuffer, sizeof(speedbuffer),
			"%dGHz", speed/1000);
	else
		snprintf(speedbuffer, sizeof(speedbuffer),
			"%d.%dGHz", speed/1000, (speed%1000)/100);

	return speedbuffer;
}


static unsigned int calc_speed(int mult)
{
	int khz;
	khz = (mult/10)*fsb;
	if (mult%10)
		khz += fsb/2;
	khz *= 1000;
	return khz;
}


static int longhaul_get_cpu_mult(void)
{
	unsigned long invalue = 0, lo, hi;

	rdmsr(MSR_IA32_EBL_CR_POWERON, lo, hi);
	invalue = (lo & (1<<22|1<<23|1<<24|1<<25))>>22;
	if (longhaul_version == TYPE_LONGHAUL_V2 ||
	    longhaul_version == TYPE_POWERSAVER) {
		if (lo & (1<<27))
			invalue += 16;
	}
	return eblcr[invalue];
}

/* For processor with BCR2 MSR */

static void do_longhaul1(unsigned int mults_index)
{
	union msr_bcr2 bcr2;

	rdmsrl(MSR_VIA_BCR2, bcr2.val);
	/* Enable software clock multiplier */
	bcr2.bits.ESOFTBF = 1;
	bcr2.bits.CLOCKMUL = mults_index & 0xff;

	/* Sync to timer tick */
	safe_halt();
	/* Change frequency on next halt or sleep */
	wrmsrl(MSR_VIA_BCR2, bcr2.val);
	/* Invoke transition */
	ACPI_FLUSH_CPU_CACHE();
	halt();

	/* Disable software clock multiplier */
	local_irq_disable();
	rdmsrl(MSR_VIA_BCR2, bcr2.val);
	bcr2.bits.ESOFTBF = 0;
	wrmsrl(MSR_VIA_BCR2, bcr2.val);
}

/* For processor with Longhaul MSR */

static void do_powersaver(int cx_address, unsigned int mults_index,
			  unsigned int dir)
{
	union msr_longhaul longhaul;
	u32 t;

	rdmsrl(MSR_VIA_LONGHAUL, longhaul.val);
	/* Setup new frequency */
	if (!revid_errata)
		longhaul.bits.RevisionKey = longhaul.bits.RevisionID;
	else
		longhaul.bits.RevisionKey = 0;
	longhaul.bits.SoftBusRatio = mults_index & 0xf;
	longhaul.bits.SoftBusRatio4 = (mults_index & 0x10) >> 4;
	/* Setup new voltage */
	if (can_scale_voltage)
		longhaul.bits.SoftVID = (mults_index >> 8) & 0x1f;
	/* Sync to timer tick */
	safe_halt();
	/* Raise voltage if necessary */
	if (can_scale_voltage && dir) {
		longhaul.bits.EnableSoftVID = 1;
		wrmsrl(MSR_VIA_LONGHAUL, longhaul.val);
		/* Change voltage */
		if (!cx_address) {
			ACPI_FLUSH_CPU_CACHE();
			halt();
		} else {
			ACPI_FLUSH_CPU_CACHE();
			/* Invoke C3 */
			inb(cx_address);
			/* Dummy op - must do something useless after P_LVL3
			 * read */
			t = inl(acpi_gbl_FADT.xpm_timer_block.address);
		}
		longhaul.bits.EnableSoftVID = 0;
		wrmsrl(MSR_VIA_LONGHAUL, longhaul.val);
	}

	/* Change frequency on next halt or sleep */
	longhaul.bits.EnableSoftBusRatio = 1;
	wrmsrl(MSR_VIA_LONGHAUL, longhaul.val);
	if (!cx_address) {
		ACPI_FLUSH_CPU_CACHE();
		halt();
	} else {
		ACPI_FLUSH_CPU_CACHE();
		/* Invoke C3 */
		inb(cx_address);
		/* Dummy op - must do something useless after P_LVL3 read */
		t = inl(acpi_gbl_FADT.xpm_timer_block.address);
	}
	/* Disable bus ratio bit */
	longhaul.bits.EnableSoftBusRatio = 0;
	wrmsrl(MSR_VIA_LONGHAUL, longhaul.val);

	/* Reduce voltage if necessary */
	if (can_scale_voltage && !dir) {
		longhaul.bits.EnableSoftVID = 1;
		wrmsrl(MSR_VIA_LONGHAUL, longhaul.val);
		/* Change voltage */
		if (!cx_address) {
			ACPI_FLUSH_CPU_CACHE();
			halt();
		} else {
			ACPI_FLUSH_CPU_CACHE();
			/* Invoke C3 */
			inb(cx_address);
			/* Dummy op - must do something useless after P_LVL3
			 * read */
			t = inl(acpi_gbl_FADT.xpm_timer_block.address);
		}
		longhaul.bits.EnableSoftVID = 0;
		wrmsrl(MSR_VIA_LONGHAUL, longhaul.val);
	}
}

/**
 * longhaul_set_cpu_frequency()
 * @mults_index : bitpattern of the new multiplier.
 *
 * Sets a new clock ratio.
 */

static int longhaul_setstate(struct cpufreq_policy *policy,
		unsigned int table_index)
{
	unsigned int mults_index;
	int speed, mult;
	struct cpufreq_freqs freqs;
	unsigned long flags;
	unsigned int pic1_mask, pic2_mask;
	u16 bm_status = 0;
	u32 bm_timeout = 1000;
	unsigned int dir = 0;

	mults_index = longhaul_table[table_index].driver_data;
	/* Safety precautions */
	mult = mults[mults_index & 0x1f];
	if (mult == -1)
		return -EINVAL;

	speed = calc_speed(mult);
	if ((speed > highest_speed) || (speed < lowest_speed))
		return -EINVAL;

	/* Voltage transition before frequency transition? */
	if (can_scale_voltage && longhaul_index < table_index)
		dir = 1;

	freqs.old = calc_speed(longhaul_get_cpu_mult());
	freqs.new = speed;

	pr_debug("Setting to FSB:%dMHz Mult:%d.%dx (%s)\n",
			fsb, mult/10, mult%10, print_speed(speed/1000));
retry_loop:
	preempt_disable();
	local_irq_save(flags);

	pic2_mask = inb(0xA1);
	pic1_mask = inb(0x21);	/* works on C3. save mask. */
	outb(0xFF, 0xA1);	/* Overkill */
	outb(0xFE, 0x21);	/* TMR0 only */

	/* Wait while PCI bus is busy. */
	if (acpi_regs_addr && (longhaul_flags & USE_NORTHBRIDGE
	    || ((pr != NULL) && pr->flags.bm_control))) {
		bm_status = inw(acpi_regs_addr);
		bm_status &= 1 << 4;
		while (bm_status && bm_timeout) {
			outw(1 << 4, acpi_regs_addr);
			bm_timeout--;
			bm_status = inw(acpi_regs_addr);
			bm_status &= 1 << 4;
		}
	}

	if (longhaul_flags & USE_NORTHBRIDGE) {
		/* Disable AGP and PCI arbiters */
		outb(3, 0x22);
	} else if ((pr != NULL) && pr->flags.bm_control) {
		/* Disable bus master arbitration */
		acpi_write_bit_register(ACPI_BITREG_ARB_DISABLE, 1);
	}
	switch (longhaul_version) {

	/*
	 * Longhaul v1. (Samuel[C5A] and Samuel2 stepping 0[C5B])
	 * Software controlled multipliers only.
	 */
	case TYPE_LONGHAUL_V1:
		do_longhaul1(mults_index);
		break;

	/*
	 * Longhaul v2 appears in Samuel2 Steppings 1->7 [C5B] and Ezra [C5C]
	 *
	 * Longhaul v3 (aka Powersaver). (Ezra-T [C5M] & Nehemiah [C5N])
	 * Nehemiah can do FSB scaling too, but this has never been proven
	 * to work in practice.
	 */
	case TYPE_LONGHAUL_V2:
	case TYPE_POWERSAVER:
		if (longhaul_flags & USE_ACPI_C3) {
			/* Don't allow wakeup */
			acpi_write_bit_register(ACPI_BITREG_BUS_MASTER_RLD, 0);
			do_powersaver(cx->address, mults_index, dir);
		} else {
			do_powersaver(0, mults_index, dir);
		}
		break;
	}

	if (longhaul_flags & USE_NORTHBRIDGE) {
		/* Enable arbiters */
		outb(0, 0x22);
	} else if ((pr != NULL) && pr->flags.bm_control) {
		/* Enable bus master arbitration */
		acpi_write_bit_register(ACPI_BITREG_ARB_DISABLE, 0);
	}
	outb(pic2_mask, 0xA1);	/* restore mask */
	outb(pic1_mask, 0x21);

	local_irq_restore(flags);
	preempt_enable();

	freqs.new = calc_speed(longhaul_get_cpu_mult());
	/* Check if requested frequency is set. */
	if (unlikely(freqs.new != speed)) {
		printk(KERN_INFO PFX "Failed to set requested frequency!\n");
		/* Revision ID = 1 but processor is expecting revision key
		 * equal to 0. Jumpers at the bottom of processor will change
		 * multiplier and FSB, but will not change bits in Longhaul
		 * MSR nor enable voltage scaling. */
		if (!revid_errata) {
			printk(KERN_INFO PFX "Enabling \"Ignore Revision ID\" "
						"option.\n");
			revid_errata = 1;
			msleep(200);
			goto retry_loop;
		}
		/* Why ACPI C3 sometimes doesn't work is a mystery for me.
		 * But it does happen. Processor is entering ACPI C3 state,
		 * but it doesn't change frequency. I tried poking various
		 * bits in northbridge registers, but without success. */
		if (longhaul_flags & USE_ACPI_C3) {
			printk(KERN_INFO PFX "Disabling ACPI C3 support.\n");
			longhaul_flags &= ~USE_ACPI_C3;
			if (revid_errata) {
				printk(KERN_INFO PFX "Disabling \"Ignore "
						"Revision ID\" option.\n");
				revid_errata = 0;
			}
			msleep(200);
			goto retry_loop;
		}
		/* This shouldn't happen. Longhaul ver. 2 was reported not
		 * working on processors without voltage scaling, but with
		 * RevID = 1. RevID errata will make things right. Just
		 * to be 100% sure. */
		if (longhaul_version == TYPE_LONGHAUL_V2) {
			printk(KERN_INFO PFX "Switching to Longhaul ver. 1\n");
			longhaul_version = TYPE_LONGHAUL_V1;
			msleep(200);
			goto retry_loop;
		}
	}

	if (!bm_timeout) {
		printk(KERN_INFO PFX "Warning: Timeout while waiting for "
				"idle PCI bus.\n");
		return -EBUSY;
	}

	return 0;
}

/*
 * Centaur decided to make life a little more tricky.
 * Only longhaul v1 is allowed to read EBLCR BSEL[0:1].
 * Samuel2 and above have to try and guess what the FSB is.
 * We do this by assuming we booted at maximum multiplier, and interpolate
 * between that value multiplied by possible FSBs and cpu_mhz which
 * was calculated at boot time. Really ugly, but no other way to do this.
 */

#define ROUNDING	0xf

static int guess_fsb(int mult)
{
	int speed = cpu_khz / 1000;
	int i;
	int speeds[] = { 666, 1000, 1333, 2000 };
	int f_max, f_min;

	for (i = 0; i < 4; i++) {
		f_max = ((speeds[i] * mult) + 50) / 100;
		f_max += (ROUNDING / 2);
		f_min = f_max - ROUNDING;
		if ((speed <= f_max) && (speed >= f_min))
			return speeds[i] / 10;
	}
	return 0;
}


static int longhaul_get_ranges(void)
{
	unsigned int i, j, k = 0;
	unsigned int ratio;
	int mult;

	/* Get current frequency */
	mult = longhaul_get_cpu_mult();
	if (mult == -1) {
		printk(KERN_INFO PFX "Invalid (reserved) multiplier!\n");
		return -EINVAL;
	}
	fsb = guess_fsb(mult);
	if (fsb == 0) {
		printk(KERN_INFO PFX "Invalid (reserved) FSB!\n");
		return -EINVAL;
	}
	/* Get max multiplier - as we always did.
	 * Longhaul MSR is useful only when voltage scaling is enabled.
	 * C3 is booting at max anyway. */
	maxmult = mult;
	/* Get min multiplier */
	switch (cpu_model) {
	case CPU_NEHEMIAH:
		minmult = 50;
		break;
	case CPU_NEHEMIAH_C:
		minmult = 40;
		break;
	default:
		minmult = 30;
		break;
	}

	pr_debug("MinMult:%d.%dx MaxMult:%d.%dx\n",
		 minmult/10, minmult%10, maxmult/10, maxmult%10);

	highest_speed = calc_speed(maxmult);
	lowest_speed = calc_speed(minmult);
	pr_debug("FSB:%dMHz  Lowest speed: %s   Highest speed:%s\n", fsb,
		 print_speed(lowest_speed/1000),
		 print_speed(highest_speed/1000));

	if (lowest_speed == highest_speed) {
		printk(KERN_INFO PFX "highestspeed == lowest, aborting.\n");
		return -EINVAL;
	}
	if (lowest_speed > highest_speed) {
		printk(KERN_INFO PFX "nonsense! lowest (%d > %d) !\n",
			lowest_speed, highest_speed);
		return -EINVAL;
	}

	longhaul_table = kzalloc((numscales + 1) * sizeof(*longhaul_table),
			GFP_KERNEL);
	if (!longhaul_table)
		return -ENOMEM;

	for (j = 0; j < numscales; j++) {
		ratio = mults[j];
		if (ratio == -1)
			continue;
		if (ratio > maxmult || ratio < minmult)
			continue;
		longhaul_table[k].frequency = calc_speed(ratio);
		longhaul_table[k].driver_data	= j;
		k++;
	}
	if (k <= 1) {
		kfree(longhaul_table);
		return -ENODEV;
	}
	/* Sort */
	for (j = 0; j < k - 1; j++) {
		unsigned int min_f, min_i;
		min_f = longhaul_table[j].frequency;
		min_i = j;
		for (i = j + 1; i < k; i++) {
			if (longhaul_table[i].frequency < min_f) {
				min_f = longhaul_table[i].frequency;
				min_i = i;
			}
		}
		if (min_i != j) {
			swap(longhaul_table[j].frequency,
			     longhaul_table[min_i].frequency);
			swap(longhaul_table[j].driver_data,
			     longhaul_table[min_i].driver_data);
		}
	}

	longhaul_table[k].frequency = CPUFREQ_TABLE_END;

	/* Find index we are running on */
	for (j = 0; j < k; j++) {
		if (mults[longhaul_table[j].driver_data & 0x1f] == mult) {
			longhaul_index = j;
			break;
		}
	}
	return 0;
}


static void longhaul_setup_voltagescaling(void)
{
	struct cpufreq_frequency_table *freq_pos;
	union msr_longhaul longhaul;
	struct mV_pos minvid, maxvid, vid;
	unsigned int j, speed, pos, kHz_step, numvscales;
	int min_vid_speed;

	rdmsrl(MSR_VIA_LONGHAUL, longhaul.val);
	if (!(longhaul.bits.RevisionID & 1)) {
		printk(KERN_INFO PFX "Voltage scaling not supported by CPU.\n");
		return;
	}

	if (!longhaul.bits.VRMRev) {
		printk(KERN_INFO PFX "VRM 8.5\n");
		vrm_mV_table = &vrm85_mV[0];
		mV_vrm_table = &mV_vrm85[0];
	} else {
		printk(KERN_INFO PFX "Mobile VRM\n");
		if (cpu_model < CPU_NEHEMIAH)
			return;
		vrm_mV_table = &mobilevrm_mV[0];
		mV_vrm_table = &mV_mobilevrm[0];
	}

	minvid = vrm_mV_table[longhaul.bits.MinimumVID];
	maxvid = vrm_mV_table[longhaul.bits.MaximumVID];

	if (minvid.mV == 0 || maxvid.mV == 0 || minvid.mV > maxvid.mV) {
		printk(KERN_INFO PFX "Bogus values Min:%d.%03d Max:%d.%03d. "
					"Voltage scaling disabled.\n",
					minvid.mV/1000, minvid.mV%1000,
					maxvid.mV/1000, maxvid.mV%1000);
		return;
	}

	if (minvid.mV == maxvid.mV) {
		printk(KERN_INFO PFX "Claims to support voltage scaling but "
				"min & max are both %d.%03d. "
				"Voltage scaling disabled\n",
				maxvid.mV/1000, maxvid.mV%1000);
		return;
	}

	/* How many voltage steps*/
	numvscales = maxvid.pos - minvid.pos + 1;
	printk(KERN_INFO PFX
		"Max VID=%d.%03d  "
		"Min VID=%d.%03d, "
		"%d possible voltage scales\n",
		maxvid.mV/1000, maxvid.mV%1000,
		minvid.mV/1000, minvid.mV%1000,
		numvscales);

	/* Calculate max frequency at min voltage */
	j = longhaul.bits.MinMHzBR;
	if (longhaul.bits.MinMHzBR4)
		j += 16;
	min_vid_speed = eblcr[j];
	if (min_vid_speed == -1)
		return;
	switch (longhaul.bits.MinMHzFSB) {
	case 0:
		min_vid_speed *= 13333;
		break;
	case 1:
		min_vid_speed *= 10000;
		break;
	case 3:
		min_vid_speed *= 6666;
		break;
	default:
		return;
		break;
	}
	if (min_vid_speed >= highest_speed)
		return;
	/* Calculate kHz for one voltage step */
	kHz_step = (highest_speed - min_vid_speed) / numvscales;

	cpufreq_for_each_entry(freq_pos, longhaul_table) {
		speed = freq_pos->frequency;
		if (speed > min_vid_speed)
			pos = (speed - min_vid_speed) / kHz_step + minvid.pos;
		else
			pos = minvid.pos;
		freq_pos->driver_data |= mV_vrm_table[pos] << 8;
		vid = vrm_mV_table[mV_vrm_table[pos]];
		printk(KERN_INFO PFX "f: %d kHz, index: %d, vid: %d mV\n",
			speed, (int)(freq_pos - longhaul_table), vid.mV);
	}

	can_scale_voltage = 1;
	printk(KERN_INFO PFX "Voltage scaling enabled.\n");
}


static int longhaul_target(struct cpufreq_policy *policy,
			    unsigned int table_index)
{
	unsigned int i;
	unsigned int dir = 0;
	u8 vid, current_vid;
	int retval = 0;

	if (!can_scale_voltage)
		retval = longhaul_setstate(policy, table_index);
	else {
		/* On test system voltage transitions exceeding single
		 * step up or down were turning motherboard off. Both
		 * "ondemand" and "userspace" are unsafe. C7 is doing
		 * this in hardware, C3 is old and we need to do this
		 * in software. */
		i = longhaul_index;
		current_vid = (longhaul_table[longhaul_index].driver_data >> 8);
		current_vid &= 0x1f;
		if (table_index > longhaul_index)
			dir = 1;
		while (i != table_index) {
			vid = (longhaul_table[i].driver_data >> 8) & 0x1f;
			if (vid != current_vid) {
				retval = longhaul_setstate(policy, i);
				current_vid = vid;
				msleep(200);
			}
			if (dir)
				i++;
			else
				i--;
		}
		retval = longhaul_setstate(policy, table_index);
	}

	longhaul_index = table_index;
	return retval;
}


static unsigned int longhaul_get(unsigned int cpu)
{
	if (cpu)
		return 0;
	return calc_speed(longhaul_get_cpu_mult());
}

static acpi_status longhaul_walk_callback(acpi_handle obj_handle,
					  u32 nesting_level,
					  void *context, void **return_value)
{
	struct acpi_device *d;

	if (acpi_bus_get_device(obj_handle, &d))
		return 0;

	*return_value = acpi_driver_data(d);
	return 1;
}

/* VIA don't support PM2 reg, but have something similar */
static int enable_arbiter_disable(void)
{
	struct pci_dev *dev;
	int status = 1;
	int reg;
	u8 pci_cmd;

	/* Find PLE133 host bridge */
	reg = 0x78;
	dev = pci_get_device(PCI_VENDOR_ID_VIA, PCI_DEVICE_ID_VIA_8601_0,
			     NULL);
	/* Find PM133/VT8605 host bridge */
	if (dev == NULL)
		dev = pci_get_device(PCI_VENDOR_ID_VIA,
				     PCI_DEVICE_ID_VIA_8605_0, NULL);
	/* Find CLE266 host bridge */
	if (dev == NULL) {
		reg = 0x76;
		dev = pci_get_device(PCI_VENDOR_ID_VIA,
				     PCI_DEVICE_ID_VIA_862X_0, NULL)