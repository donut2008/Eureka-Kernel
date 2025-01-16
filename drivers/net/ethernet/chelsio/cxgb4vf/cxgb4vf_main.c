/*
 * Copyright 2013 Freescale Semiconductor, Inc.
 *
 * CPU Frequency Scaling driver for Freescale QorIQ SoCs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/smp.h>

#if !defined(CONFIG_ARM)
#include <asm/smp.h>	/* for get_hard_smp_processor_id() in UP configs */
#endif

/**
 * struct cpu_data
 * @pclk: the parent clock of cpu
 * @table: frequency table
 */
struct cpu_data {
	struct clk **pclk;
	struct cpufreq_frequency_table *table;
};

/**
 * struct soc_data - SoC specific data
 * @freq_mask: mask the disallowed frequencies
 * @flag: unique flags
 */
struct soc_data {
	u32 freq_mask[4];
	u32 flag;
};

#define FREQ_MASK	1
/* see hardware specification for the allowed frqeuencies */
static const struct soc_data sdata[] = {
	{ /* used by p2041 and p3041 */
		.freq_mask = {0x8, 0x8, 0x2, 0x2},
		.flag = FREQ_MASK,
	},
	{ /* used by p5020 */
		.freq_mask = {0x8, 0x2},
		.flag = FREQ_MASK,
	},
	{ /* used by p4080, p5040 */
		.freq_mask = {0},
		.flag = 0,
	},
};

/*
 * the minimum allowed core frequency, in Hz
 * for chassis v1.0, >= platform frequency
 * for chassis v2.0, >= platform frequency / 2
 */
static u32 min_cpufreq;
static const u32 *fmask;

#if defined(CONFIG_ARM)
static int get_cpu_physical_id(int cpu)
{
	return topology_core_id(cpu);
}
#else
static int get_cpu_physical_id(int cpu)
{
	return get_hard_smp_processor_id(cpu);
}
#endif

static u32 get_bus_freq(void)
{
	struct device_node *soc;
	u32 sysfreq;

	soc = of_find_node_by_type(NULL, "soc");
	if (!soc)
		return 0;

	if (of_property_read_u32(soc, "bus-frequency", &sysfreq))
		sysfreq = 0;

	of_node_put(soc);

	return sysfreq;
}

static struct device_node *cpu_to_clk_node(int cpu)
{
	struct device_node *np, *clk_np;

	if (!cpu_present(cpu))
		return NULL;

	np = of_get_cpu_node(cpu, NULL);
	if (!np)
		return NULL;

	clk_np = of_parse_phandle(np, "clocks", 0);
	if (!clk_np)
		return NULL;

	of_node_put(np);

	return clk_np;
}

/* traverse cpu nodes to get cpu mask of sharing clock wire */
static void set_affected_cpus(struct cpufreq_policy *policy)
{
	struct device_node *np, *clk_np;
	struct cpumask *dstp = policy->cpus;
	int i;

	np = cpu_to_clk_node(policy->cpu);
	if (!np)
		return;

	for_each_present_cpu(i) {
		clk_np = cpu_to_clk_node(i);
		if (!clk_np)
			continue;

		if (clk_np == np)
			cpumask_set_cpu(i, dstp);

		of_node_put(clk_np);
	}
	of_node_put(np);
}

/* reduce the duplicated frequencies in frequency table */
static void freq_table_redup(struct cpufreq_frequency_table *freq_table,
		int count)
{
	int i, j;

	for (i = 1; i < count; i++) {
		for (j = 0; j < i; j++) {
			if (freq_table[j].frequency == CPUFREQ_ENTRY_INVALID ||
					freq_table[j].frequency !=
					freq_table[i].frequency)
				continue;

			freq_table[i].frequency = CPUFREQ_ENTRY_INVALID;
			break;
		}
	}
}

/* sort the frequencies in frequency table in descenting order */
static void freq_table_sort(struct cpufreq_frequency_table *freq_table,
		int count)
{
	int i, j, ind;
	unsigned int freq, max_freq;
	struct cpufreq_frequency_table table;

	for (i = 0; i < count - 1; i++) {
		max_freq = freq_table[i].frequency;
		ind = i;
		for (j = i + 1; j < count; j++) {
			freq = freq_table[j].frequency;
			if (freq == CPUFREQ_ENTRY_INVALID ||
					freq <= max_freq)
				continue;
			ind = j;
			max_freq = freq;
		}

		if (ind != i) {
			/* exchange the frequencies */
			table.driver_data = freq_table[i].driver_data;
			table.frequency = freq_table[i].frequency;
			freq_table[i].driver_data = freq_table[ind].driver_data;
			freq_table[i].frequency = freq_table[ind].frequency;
			freq_table[ind].driver_data = table.driver_data;
			freq_table[ind].frequency = table.frequency;
		}
	}
}

static int qoriq_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	struct device_node *np, *pnode;
	int i, count, ret;
	u32 freq, mask;
	struct clk *clk;
	struct cpufreq_frequency_table *table;
	struct cpu_data *data;
	unsigned int cpu = policy->cpu;
	u64 u64temp;

	np = of_get_cpu_node(cpu, NULL);
	if (!np)
		return -ENODEV;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		goto err_np;

	policy->clk = of_clk_get(np, 0);
	if (IS_ERR(policy->clk)) {
		pr_err("%s: no clock information\n", __func__);
		goto err_nomem2;
	}

	pnode = of_parse_phandle(np, "clocks", 0);
	if (!pnode) {
		pr_err("%s: could not get clock information\n", __func__);
		goto err_nomem2;
	}

	count = of_property_count_strings(pnode, "clock-names");
	data->pclk = kcalloc(count, sizeof(struct clk *), GFP_KERNEL);
	if (!data->pclk) {
		pr_err("%s: no memory\n", __func__);
		goto err_node;
	}

	table = kcalloc(count + 1, sizeof(*table), GFP_KERNEL);
	if (!table) {
		pr_err("%s: no memory\n", __func__);
		goto err_pclk;
	}

	if (fmask)
		mask = fmask[get_cpu_physical_id(cpu)];
	else
		mask = 0x0;

	for (i = 0; i < count; i++) {
		clk = of_clk_get(pnode, i);
		data->pclk[i] = clk;
		freq = clk_get_rate(clk);
		/*
		 * the clock is valid if its frequency is not masked
		 * and large than minimum allowed frequency.
		 */
		if (freq < min_cpufreq || (mask & (1 << i)))
			table[i].frequency = CPUFREQ_ENTRY_INVALID;
		else
			table[i].frequency = freq / 1000;
		table[i].driver_data = i;
	}
	freq_table_redup(table, count);
	freq_table_sort(table, count);
	table[i].frequency = CPUFREQ_TABLE_END;

	/* set the min and max frequency properly */
	ret = cpufreq_table_validate_and_show(policy, table);
	if (ret) {
		pr_err("invalid frequency table: %d\n", ret);
		goto err_nomem1;
	}

	data->table = table;

	/* update ->cpus if we have cluster, no harm if not */
	set_affected_cpus(policy);
	policy->driver_data = data;

	/* Minimum transition latency is 12 platform clocks */
	u64temp = 12ULL * NSEC_PER_SEC;
	do_div(u64temp, get_bus_freq());
	policy->cpuinfo.transition_latency = u64temp + 1;

	of_node_put(np);
	of_node_put(pnode);

	return 0;

err_nomem1:
	kfree(table);
err_pclk:
	kfree(data->pclk);
err_node:
	of_node_put(pnode);
err_nomem2:
	policy->driver_data = NULL;
	kfree(data);
err_np:
	of_node_put(np);

	return -ENODEV;
}

static int __exit qoriq_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	struct cpu_data *data = policy->driver_data;

	kfree(data->pclk);
	kfree(data->table);
	kfree(data);
	policy->driver_data = NULL;

	return 0;
}

static int qoriq_cpufreq_target(struct cpufreq_policy *policy,
		unsigned int index)
{
	struct clk *parent;
	struct cpu_data *data = policy->driver_data;

	parent = data->pclk[data->table[index].driver_data];
	return clk_set_parent(policy->clk, parent);
}

static struct cpufreq_driver qoriq_cpufreq_driver = {
	.name		= "qoriq_cpufreq",
	.flags		= CPUFREQ_CONST_LOOPS,
	.init		= qoriq_cpufreq_cpu_init,
	.exit		= __exit_p(qoriq_cpufreq_cpu_exit),
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= qoriq_cpufreq_target,
	.get		= cpufreq_generic_get,
	.attr		= cpufreq_generic_attr,
};

static const struct of_device_id node_matches[] __initconst = {
	{ .compatible = "fsl,p2041-clockgen", .data = &sdata[0], },
	{ .compatible = "fsl,p3041-clockgen", .data = &sdata[0], },
	{ .compatible = "fsl,p5020-clockgen", .data = &sdata[1], },
	{ .compatible = "fsl,p4080-clockgen", .data = &sdata[2], },
	{ .compatible = "fsl,p5040-clockgen", .data = &sdata[2], },
	{ .compatible = "fsl,qoriq-clockgen-2.0", },
	{}
};

static int __init qoriq_cpufreq_init(void)
{
	int ret;
	struct device_node  *np;
	const struct of_device_id *match;
	const struct soc_data *data;

	np = of_find_matching_node(NULL, node_matches);
	if (!np)
		return -ENODEV;

	match = of_match_node(node_matches, np);
	data = match->data;
	if (data) {
		if (data->flag)
			fmask = data->freq_mask;
		min_cpufreq = get_bus_freq();
	} else {
		min_cpufreq = get_bus_freq() / 2;
	}

	of_node_put(np);

	ret = cpufreq_register_driver(&qoriq_cpufreq_driver);
	if (!ret)
		pr_info("Freescale QorIQ CPU frequency scaling driver\n");

	return ret;
}
module_init(qoriq_cpufreq_init);

static void __exit qoriq_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&qoriq_cpufreq_driver);
}
module_exit(qoriq_cpufreq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tang Yuantian <Yuantian.Tang@freescale.com>");
MODULE_DESCRIPTION("cpufreq driver for Freescale QorIQ series SoCs");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * Copyright (c) 2006-2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2410 CPU Frequency scaling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/regs-clock.h>

#include <plat/cpu.h>
#include <plat/cpu-freq-core.h>

/* Note, 2410A has an extra mode for 1:4:4 ratio, bit 2 of CLKDIV */

static void s3c2410_cpufreq_setdivs(struct s3c_cpufreq_config *cfg)
{
	u32 clkdiv = 0;

	if (cfg->divs.h_divisor == 2)
		clkdiv |= S3C2410_CLKDIVN_HDIVN;

	if (cfg->divs.p_divisor != cfg->divs.h_divisor)
		clkdiv |= S3C2410_CLKDIVN_PDIVN;

	__raw_writel(clkdiv, S3C2410_CLKDIVN);
}

static int s3c2410_cpufreq_calcdivs(struct s3c_cpufreq_config *cfg)
{
	unsigned long hclk, fclk, pclk;
	unsigned int hdiv, pdiv;
	unsigned long hclk_max;

	fclk = cfg->freq.fclk;
	hclk_max = cfg->max.hclk;

	cfg->freq.armclk = fclk;

	s3c_freq_dbg("%s: fclk is %lu, max hclk %lu\n",
		      __func__, fclk, hclk_max);

	hdiv = (fclk > cfg->max.hclk) ? 2 : 1;
	hclk = fclk / hdiv;

	if (hclk > cfg->max.hclk) {
		s3c_freq_dbg("%s: hclk too big\n", __func__);
		return -EINVAL;
	}

	pdiv = (hclk > cfg->max.pclk) ? 2 : 1;
	pclk = hclk / pdiv;

	if (pclk > cfg->max.pclk) {
		s3c_freq_dbg("%s: pclk too big\n", __func__);
		return -EINVAL;
	}

	pdiv *= hdiv;

	/* record the result */
	cfg->divs.p_divisor = pdiv;
	cfg->divs.h_divisor = hdiv;

	return 0;
}

static struct s3c_cpufreq_info s3c2410_cpufreq_info = {
	.max		= {
		.fclk	= 200000000,
		.hclk	= 100000000,
		.pclk	=  50000000,
	},

	/* transition latency is about 5ms worst-case, so
	 * set 10ms to be sure */
	.latency	= 10000000,

	.locktime_m	= 150,
	.locktime_u	= 150,
	.locktime_bits	= 12,

	.need_pll	= 1,

	.name		= "s3c2410",
	.calc_iotiming	= s3c2410_iotiming_calc,
	.set_iotiming	= s3c2410_iotiming_set,
	.get_iotiming	= s3c2410_iotiming_get,

	.set_fvco	= s3c2410_set_fvco,
	.set_refresh	= s3c2410_cpufreq_setrefresh,
	.set_divs	= s3c2410_cpufreq_setdivs,
	.calc_divs	= s3c2410_cpufreq_calcdivs,

	.debug_io_show	= s3c_cpufreq_debugfs_call(s3c2410_iotiming_debugfs),
};

static int s3c2410_cpufreq_add(struct device *dev,
			       struct subsys_interface *sif)
{
	return s3c_cpufreq_register(&s3c2410_cpufreq_info);
}

static struct subsys_interface s3c2410_cpufreq_interface = {
	.name		= "s3c2410_cpufreq",
	.subsys		= &s3c2410_subsys,
	.add_dev	= s3c2410_cpufreq_add,
};

static int __init s3c2410_cpufreq_init(void)
{
	return subsys_interface_register(&s3c2410_cpufreq_interface);
}
arch_initcall(s3c2410_cpufreq_init);

static int s3c2410a_cpufreq_add(struct device *dev,
				struct subsys_interface *sif)
{
	/* alter the maximum freq settings for S3C2410A. If a board knows
	 * it only has a maximum of 200, then it should register its own
	 * limits. */

	s3c2410_cpufreq_info.max.fclk = 266000000;
	s3c2410_cpufreq_info.max.hclk = 133000000;
	s3c2410_cpufreq_info.max.pclk =  66500000;
	s3c2410_cpufreq_info.name = "s3c2410a";

	return s3c2410_cpufreq_add(dev, sif);
}

static struct subsys_interface s3c2410a_cpufreq_interface = {
	.name		= "s3c2410a_cpufreq",
	.subsys		= &s3c2410a_subsys,
	.add_dev	= s3c2410a_cpufreq_add,
};

static int __init s3c2410a_cpufreq_init(void)
{
	return subsys_interface_register(&s3c2410a_cpufreq_interface);
}
arch_initcall(s3c2410a_cpufreq_init);
                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2412 CPU Frequency scalling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/regs-clock.h>
#include <mach/s3c2412.h>

#include <plat/cpu.h>
#include <plat/cpu-freq-core.h>

/* our clock resources. */
static struct clk *xtal;
static struct clk *fclk;
static struct clk *hclk;
static struct clk *armclk;

/* HDIV: 1, 2, 3, 4, 6, 8 */

static int s3c2412_cpufreq_calcdivs(struct s3c_cpufreq_config *cfg)
{
	unsigned int hdiv, pdiv, armdiv, dvs;
	unsigned long hclk, fclk, armclk, armdiv_clk;
	unsigned long hclk_max;

	fclk = cfg->freq.fclk;
	armclk = cfg->freq.armclk;
	hclk_max = cfg->max.hclk;

	/* We can't run hclk above armclk as at the best we have to
	 * have armclk and hclk in dvs mode. */

	if (hclk_max > armclk)
		hclk_max = armclk;

	s3c_freq_dbg("%s: fclk=%lu, armclk=%lu, hclk_max=%lu\n",
		     __func__, fclk, armclk, hclk_max);
	s3c_freq_dbg("%s: want f=%lu, arm=%lu, h=%lu, p=%lu\n",
		     __func__, cfg->freq.fclk, cfg->freq.armclk,
		     cfg->freq.hclk, cfg->freq.pclk);

	armdiv = fclk / armclk;

	if (armdiv < 1)
		armdiv = 1;
	if (armdiv > 2)
		armdiv = 2;

	cfg->divs.arm_divisor = armdiv;
	armdiv_clk = fclk / armdiv;

	hdiv = armdiv_clk / hclk_max;
	if (hdiv < 1)
		hdiv = 1;

	cfg->freq.hclk = hclk = armdiv_clk / hdiv;

	/* set dvs depending on whether we reached armclk or not. */
	cfg->divs.dvs = dvs = armclk < armdiv_clk;

	/* update the actual armclk we achieved. */
	cfg->freq.armclk = dvs ? hclk : armdiv_clk;

	s3c_freq_dbg("%s: armclk %lu, hclk %lu, armdiv %d, hdiv %d, dvs %d\n",
		     __func__, armclk, hclk, armdiv, hdiv, cfg->divs.dvs);

	if (hdiv > 4)
		goto invalid;

	pdiv = (hclk > cfg->max.pclk) ? 2 : 1;

	if ((hclk / pdiv) > cfg->max.pclk)
		pdiv++;

	cfg->freq.pclk = hclk / pdiv;

	s3c_freq_dbg("%s: pdiv %d\n", __func__, pdiv);

	if (pdiv > 2)
		goto invalid;

	pdiv *= hdiv;

	/* store the result, and then return */

	cfg->divs.h_divisor = hdiv * armdiv;
	cfg->divs.p_divisor = pdiv * armdiv;

	return 0;

invalid:
	return -EINVAL;
}

static void s3c2412_cpufreq_setdivs(struct s3c_cpufreq_config *cfg)
{
	unsigned long clkdiv;
	unsigned long olddiv;

	olddiv = clkdiv = __raw_readl(S3C2410_CLKDIVN);

	/* clear off current clock info */

	clkdiv &= ~S3C2412_CLKDIVN_ARMDIVN;
	clkdiv &= ~S3C2412_CLKDIVN_HDIVN_MASK;
	clkdiv &= ~S3C2412_CLKDIVN_PDIVN;

	if (cfg->divs.arm_divisor == 2)
		clkdiv |= S3C2412_CLKDIVN_ARMDIVN;

	clkdiv |= ((cfg->divs.h_divisor / cfg->divs.arm_divisor) - 1);

	if (cfg->divs.p_divisor != cfg->divs.h_divisor)
		clkdiv |= S3C2412_CLKDIVN_PDIVN;

	s3c_freq_dbg("%s: div %08lx => %08lx\n", __func__, olddiv, clkdiv);
	__raw_writel(clkdiv, S3C2410_CLKDIVN);

	clk_set_parent(armclk, cfg->divs.dvs ? hclk : fclk);
}

static void s3c2412_cpufreq_setrefresh(struct s3c_cpufreq_config *cfg)
{
	struct s3c_cpufreq_board *board = cfg->board;
	unsigned long refresh;

	s3c_freq_dbg("%s: refresh %u ns, hclk %lu\n", __func__,
		     board->refresh, cfg->freq.hclk);

	/* Reduce both the refresh time (in ns) and the frequency (in MHz)
	 * by 10 each to ensure that we do not overflow 32 bit numbers. This
	 * should work for HCLK up to 133MHz and refresh period up to 30usec.
	 */

	refresh = (board->refresh / 10);
	refresh *= (cfg->freq.hclk / 100);
	refresh /= (1 * 1000 * 1000);	/* 10^6 */

	s3c_freq_dbg("%s: setting refresh 0x%08lx\n", __func__, refresh);
	__raw_writel(refresh, S3C2412_REFRESH);
}

/* set the default cpu frequency information, based on an 200MHz part
 * as we have no other way of detecting the speed rating in software.
 */

static struct s3c_cpufreq_info s3c2412_cpufreq_info = {
	.max		= {
		.fclk	= 200000000,
		.hclk	= 100000000,
		.pclk	=  50000000,
	},

	.latency	= 5000000, /* 5ms */

	.locktime_m	= 150,
	.locktime_u	= 150,
	.locktime_bits	= 16,

	.name		= "s3c2412",
	.set_refresh	= s3c2412_cpufreq_setrefresh,
	.set_divs	= s3c2412_cpufreq_setdivs,
	.calc_divs	= s3c2412_cpufreq_calcdivs,

	.calc_iotiming	= s3c2412_iotiming_calc,
	.set_iotiming	= s3c2412_iotiming_set,
	.get_iotiming	= s3c2412_iotiming_get,

	.debug_io_show  = s3c_cpufreq_debugfs_call(s3c2412_iotiming_debugfs),
};

static int s3c2412_cpufreq_add(struct device *dev,
			       struct subsys_interface *sif)
{
	unsigned long fclk_rate;

	hclk = clk_get(NULL, "hclk");
	if (IS_ERR(hclk)) {
		printk(KERN_ERR "%s: cannot find hclk clock\n", __func__);
		return -ENOENT;
	}

	fclk = clk_get(NULL, "fclk");
	if (IS_ERR(fclk)) {
		printk(KERN_ERR "%s: cannot find fclk clock\n", __func__);
		goto err_fclk;
	}

	fclk_rate = clk_get_rate(fclk);
	if (fclk_rate > 200000000) {
		printk(KERN_INFO
		       "%s: fclk %ld MHz, assuming 266MHz capable part\n",
		       __func__, fclk_rate / 1000000);
		s3c2412_cpufreq_info.max.fclk = 266000000;
		s3c2412_cpufreq_info.max.hclk = 133000000;
		s3c2412_cpufreq_info.max.pclk =  66000000;
	}

	armclk = clk_get(NULL, "armclk");
	if (IS_ERR(armclk)) {
		printk(KERN_ERR "%s: cannot find arm clock\n", __func__);
		goto err_armclk;
	}

	xtal = clk_get(NULL, "xtal");
	if (IS_ERR(xtal)) {
		printk(KERN_ERR "%s: cannot find xtal clock\n", __func__);
		goto err_xtal;
	}

	return s3c_cpufreq_register(&s3c2412_cpufreq_info);

err_xtal:
	clk_put(armclk);
err_armclk:
	clk_put(fclk);
err_fclk:
	clk_put(hclk);

	return -ENOENT;
}

static struct subsys_interface s3c2412_cpufreq_interface = {
	.name		= "s3c2412_cpufreq",
	.subsys		= &s3c2412_subsys,
	.add_dev	= s3c2412_cpufreq_add,
};

static int s3c2412_cpufreq_init(void)
{
	return subsys_interface_register(&s3c2412_cpufreq_interface);
}
arch_initcall(s3c2412_cpufreq_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * S3C2416/2450 CPUfreq Support
 *
 * Copyright 2011 Heiko Stuebner <heiko@sntech.de>
 *
 * based on s3c64xx_cpufreq.c
 *
 * Copyright 2009 Wolfson Microelectronics plc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/reboot.h>
#include <linux/module.h>

static DEFINE_MUTEX(cpufreq_lock);

struct s3c2416_data {
	struct clk *armdiv;
	struct clk *armclk;
	struct clk *hclk;

	unsigned long regulator_latency;
#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
	struct regulator *vddarm;
#endif

	struct cpufreq_frequency_table *freq_table;

	bool is_dvs;
	bool disable_dvs;
};

static struct s3c2416_data s3c2416_cpufreq;

struct s3c2416_dvfs {
	unsigned int vddarm_min;
	unsigned int vddarm_max;
};

/* pseudo-frequency for dvs mode */
#define FREQ_DVS	132333

/* frequency to sleep and reboot in
 * it's essential to leave dvs, as some boards do not reconfigure the
 * regulator on reboot
 */
#define FREQ_SLEEP	133333

/* Sources for the ARMCLK */
#define SOURCE_HCLK	0
#define SOURCE_ARMDIV	1

#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
/* S3C2416 only supports changing the voltage in the dvs-mode.
 * Voltages down to 1.0V seem to work, so we take what the regulator
 * can get us.
 */
static struct s3c2416_dvfs s3c2416_dvfs_table[] = {
	[SOURCE_HCLK] = {  950000, 1250000 },
	[SOURCE_ARMDIV] = { 1250000, 1350000 },
};
#endif

static struct cpufreq_frequency_table s3c2416_freq_table[] = {
	{ 0, SOURCE_HCLK, FREQ_DVS },
	{ 0, SOURCE_ARMDIV, 133333 },
	{ 0, SOURCE_ARMDIV, 266666 },
	{ 0, SOURCE_ARMDIV, 400000 },
	{ 0, 0, CPUFREQ_TABLE_END },
};

static struct cpufreq_frequency_table s3c2450_freq_table[] = {
	{ 0, SOURCE_HCLK, FREQ_DVS },
	{ 0, SOURCE_ARMDIV, 133500 },
	{ 0, SOURCE_ARMDIV, 267000 },
	{ 0, SOURCE_ARMDIV, 534000 },
	{ 0, 0, CPUFREQ_TABLE_END },
};

static unsigned int s3c2416_cpufreq_get_speed(unsigned int cpu)
{
	struct s3c2416_data *s3c_freq = &s3c2416_cpufreq;

	if (cpu != 0)
		return 0;

	/* return our pseudo-frequency when in dvs mode */
	if (s3c_freq->is_dvs)
		return FREQ_DVS;

	return clk_get_rate(s3c_freq->armclk) / 1000;
}

static int s3c2416_cpufreq_set_armdiv(struct s3c2416_data *s3c_freq,
				      unsigned int freq)
{
	int ret;

	if (clk_get_rate(s3c_freq->armdiv) / 1000 != freq) {
		ret = clk_set_rate(s3c_freq->armdiv, freq * 1000);
		if (ret < 0) {
			pr_err("cpufreq: Failed to set armdiv rate %dkHz: %d\n",
			       freq, ret);
			return ret;
		}
	}

	return 0;
}

static int s3c2416_cpufreq_enter_dvs(struct s3c2416_data *s3c_freq, int idx)
{
#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
	struct s3c2416_dvfs *dvfs;
#endif
	int ret;

	if (s3c_freq->is_dvs) {
		pr_debug("cpufreq: already in dvs mode, nothing to do\n");
		return 0;
	}

	pr_debug("cpufreq: switching armclk to hclk (%lukHz)\n",
		 clk_get_rate(s3c_freq->hclk) / 1000);
	ret = clk_set_parent(s3c_freq->armclk, s3c_freq->hclk);
	if (ret < 0) {
		pr_err("cpufreq: Failed to switch armclk to hclk: %d\n", ret);
		return ret;
	}

#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
	/* changing the core voltage is only allowed when in dvs mode */
	if (s3c_freq->vddarm) {
		dvfs = &s3c2416_dvfs_table[idx];

		pr_debug("cpufreq: setting regulator to %d-%d\n",
			 dvfs->vddarm_min, dvfs->vddarm_max);
		ret = regulator_set_voltage(s3c_freq->vddarm,
					    dvfs->vddarm_min,
					    dvfs->vddarm_max);

		/* when lowering the voltage failed, there is nothing to do */
		if (ret != 0)
			pr_err("cpufreq: Failed to set VDDARM: %d\n", ret);
	}
#endif

	s3c_freq->is_dvs = 1;

	return 0;
}

static int s3c2416_cpufreq_leave_dvs(struct s3c2416_data *s3c_freq, int idx)
{
#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
	struct s3c2416_dvfs *dvfs;
#endif
	int ret;

	if (!s3c_freq->is_dvs) {
		pr_debug("cpufreq: not in dvs mode, so can't leave\n");
		return 0;
	}

#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
	if (s3c_freq->vddarm) {
		dvfs = &s3c2416_dvfs_table[idx];

		pr_debug("cpufreq: setting regulator to %d-%d\n",
			 dvfs->vddarm_min, dvfs->vddarm_max);
		ret = regulator_set_voltage(s3c_freq->vddarm,
					    dvfs->vddarm_min,
					    dvfs->vddarm_max);
		if (ret != 0) {
			pr_err("cpufreq: Failed to set VDDARM: %d\n", ret);
			return ret;
		}
	}
#endif

	/* force armdiv to hclk frequency for transition from dvs*/
	if (clk_get_rate(s3c_freq->armdiv) > clk_get_rate(s3c_freq->hclk)) {
		pr_debug("cpufreq: force armdiv to hclk frequency (%lukHz)\n",
			 clk_get_rate(s3c_freq->hclk) / 1000);
		ret = s3c2416_cpufreq_set_armdiv(s3c_freq,
					clk_get_rate(s3c_freq->hclk) / 1000);
		if (ret < 0) {
			pr_err("cpufreq: Failed to set the armdiv to %lukHz: %d\n",
			       clk_get_rate(s3c_freq->hclk) / 1000, ret);
			return ret;
		}
	}

	pr_debug("cpufreq: switching armclk parent to armdiv (%lukHz)\n",
			clk_get_rate(s3c_freq->armdiv) / 1000);

	ret = clk_set_parent(s3c_freq->armclk, s3c_freq->armdiv);
	if (ret < 0) {
		pr_err("cpufreq: Failed to switch armclk clock parent to armdiv: %d\n",
		       ret);
		return ret;
	}

	s3c_freq->is_dvs = 0;

	return 0;
}

static int s3c2416_cpufreq_set_target(struct cpufreq_policy *policy,
				      unsigned int index)
{
	struct s3c2416_data *s3c_freq = &s3c2416_cpufreq;
	unsigned int new_freq;
	int idx, ret, to_dvs = 0;

	mutex_lock(&cpufreq_lock);

	idx = s3c_freq->freq_table[index].driver_data;

	if (idx == SOURCE_HCLK)
		to_dvs = 1;

	/* switching to dvs when it's not allowed */
	if (to_dvs && s3c_freq->disable_dvs) {
		pr_debug("cpufreq: entering dvs mode not allowed\n");
		ret = -EINVAL;
		goto out;
	}

	/* When leavin dvs mode, always switch the armdiv to the hclk rate
	 * The S3C2416 has stability issues when switching directly to
	 * higher frequencies.
	 */
	new_freq = (s3c_freq->is_dvs && !to_dvs)
				? clk_get_rate(s3c_freq->hclk) / 1000
				: s3c_freq->freq_table[index].frequency;

	if (to_dvs) {
		pr_debug("cpufreq: enter dvs\n");
		ret = s3c2416_cpufreq_enter_dvs(s3c_freq, idx);
	} else if (s3c_freq->is_dvs) {
		pr_debug("cpufreq: leave dvs\n");
		ret = s3c2416_cpufreq_leave_dvs(s3c_freq, idx);
	} else {
		pr_debug("cpufreq: change armdiv to %dkHz\n", new_freq);
		ret = s3c2416_cpufreq_set_armdiv(s3c_freq, new_freq);
	}

out:
	mutex_unlock(&cpufreq_lock);

	return ret;
}

#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
static void s3c2416_cpufreq_cfg_regulator(struct s3c2416_data *s3c_freq)
{
	int count, v, i, found;
	struct cpufreq_frequency_table *pos;
	struct s3c2416_dvfs *dvfs;

	count = regulator_count_voltages(s3c_freq->vddarm);
	if (count < 0) {
		pr_err("cpufreq: Unable to check supported voltages\n");
		return;
	}

	if (!count)
		goto out;

	cpufreq_for_each_valid_entry(pos, s3c_freq->freq_table) {
		dvfs = &s3c2416_dvfs_table[pos->driver_data];
		found = 0;

		/* Check only the min-voltage, more is always ok on S3C2416 */
		for (i = 0; i < count; i++) {
			v = regulator_list_voltage(s3c_freq->vddarm, i);
			if (v >= dvfs->vddarm_min)
				found = 1;
		}

		if (!found) {
			pr_debug("cpufreq: %dkHz unsupported by regulator\n",
				 pos->frequency);
			pos->frequency = CPUFREQ_ENTRY_INVALID;
		}
	}

out:
	/* Guessed */
	s3c_freq->regulator_latency = 1 * 1000 * 1000;
}
#endif

static int s3c2416_cpufreq_reboot_notifier_evt(struct notifier_block *this,
					       unsigned long event, void *ptr)
{
	struct s3c2416_data *s3c_freq = &s3c2416_cpufreq;
	int ret;

	mutex_lock(&cpufreq_lock);

	/* disable further changes */
	s3c_freq->disable_dvs = 1;

	mutex_unlock(&cpufreq_lock);

	/* some boards don't reconfigure the regulator on reboot, which
	 * could lead to undervolting the cpu when the clock is reset.
	 * Therefore we always leave the DVS mode on reboot.
	 */
	if (s3c_freq->is_dvs) {
		pr_debug("cpufreq: leave dvs on reboot\n");
		ret = cpufreq_driver_target(cpufreq_cpu_get(0), FREQ_SLEEP, 0);
		if (ret < 0)
			return NOTIFY_BAD;
	}

	return NOTIFY_DONE;
}

static struct notifier_block s3c2416_cpufreq_reboot_notifier = {
	.notifier_call = s3c2416_cpufreq_reboot_notifier_evt,
};

static int s3c2416_cpufreq_driver_init(struct cpufreq_policy *policy)
{
	struct s3c2416_data *s3c_freq = &s3c2416_cpufreq;
	struct cpufreq_frequency_table *pos;
	struct clk *msysclk;
	unsigned long rate;
	int ret;

	if (policy->cpu != 0)
		return -EINVAL;

	msysclk = clk_get(NULL, "msysclk");
	if (IS_ERR(msysclk)) {
		ret = PTR_ERR(msysclk);
		pr_err("cpufreq: Unable to obtain msysclk: %d\n", ret);
		return ret;
	}

	/*
	 * S3C2416 and S3C2450 share the same processor-ID and also provide no
	 * other means to distinguish them other than through the rate of
	 * msysclk. On S3C2416 msysclk runs at 800MHz and on S3C2450 at 533MHz.
	 */
	rate = clk_get_rate(msysclk);
	if (rate == 800 * 1000 * 1000) {
		pr_info("cpufreq: msysclk running at %lukHz, using S3C2416 frequency table\n",
			rate / 1000);
		s3c_freq->freq_table = s3c2416_freq_table;
		policy->cpuinfo.max_freq = 400000;
	} else if (rate / 1000 == 534000) {
		pr_info("cpufreq: msysclk running at %lukHz, using S3C2450 frequency table\n",
			rate / 1000);
		s3c_freq->freq_table = s3c2450_freq_table;
		policy->cpuinfo.max_freq = 534000;
	}

	/* not needed anymore */
	clk_put(msysclk);

	if (s3c_freq->freq_table == NULL) {
		pr_err("cpufreq: No frequency information for this CPU, msysclk at %lukHz\n",
		       rate / 1000);
		return -ENODEV;
	}

	s3c_freq->is_dvs = 0;

	s3c_freq->armdiv = clk_get(NULL, "armdiv");
	if (IS_ERR(s3c_freq->armdiv)) {
		ret = PTR_ERR(s3c_freq->armdiv);
		pr_err("cpufreq: Unable to obtain ARMDIV: %d\n", ret);
		return ret;
	}

	s3c_freq->hclk = clk_get(NULL, "hclk");
	if (IS_ERR(s3c_freq->hclk)) {
		ret = PTR_ERR(s3c_freq->hclk);
		pr_err("cpufreq: Unable to obtain HCLK: %d\n", ret);
		goto err_hclk;
	}

	/* chech hclk rate, we only support the common 133MHz for now
	 * hclk could also run at 66MHz, but this not often used
	 */
	rate = clk_get_rate(s3c_freq->hclk);
	if (rate < 133 * 1000 * 1000) {
		pr_err("cpufreq: HCLK not at 133MHz\n");
		ret = -EINVAL;
		goto err_armclk;
	}

	s3c_freq->armclk = clk_get(NULL, "armclk");
	if (IS_ERR(s3c_freq->armclk)) {
		ret = PTR_ERR(s3c_freq->armclk);
		pr_err("cpufreq: Unable to obtain ARMCLK: %d\n", ret);
		goto err_armclk;
	}

#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
	s3c_freq->vddarm = regulator_get(NULL, "vddarm");
	if (IS_ERR(s3c_freq->vddarm)) {
		ret = PTR_ERR(s3c_freq->vddarm);
		pr_err("cpufreq: Failed to obtain VDDARM: %d\n", ret);
		goto err_vddarm;
	}

	s3c2416_cpufreq_cfg_regulator(s3c_freq);
#else
	s3c_freq->regulator_latency = 0;
#endif

	cpufreq_for_each_entry(pos, s3c_freq->freq_table) {
		/* special handling for dvs mode */
		if (pos->driver_data == 0) {
			if (!s3c_freq->hclk) {
				pr_debug("cpufreq: %dkHz unsupported as it would need unavailable dvs mode\n",
					 pos->frequency);
				pos->frequency = CPUFREQ_ENTRY_INVALID;
			} else {
				continue;
			}
		}

		/* Check for frequencies we can generate */
		rate = clk_round_rate(s3c_freq->armdiv,
				      pos->frequency * 1000);
		rate /= 1000;
		if (rate != pos->frequency) {
			pr_debug("cpufreq: %dkHz unsupported by clock (clk_round_rate return %lu)\n",
				pos->frequency, rate);
			pos->frequency = CPUFREQ_ENTRY_INVALID;
		}
	}

	/* Datasheet says PLL stabalisation time must be at least 300us,
	 * so but add some fudge. (reference in LOCKCON0 register description)
	 */
	ret = cpufreq_generic_init(policy, s3c_freq->freq_table,
			(500 * 1000) + s3c_freq->regulator_latency);
	if (ret)
		goto err_freq_table;

	register_reboot_notifier(&s3c2416_cpufreq_reboot_notifier);

	return 0;

err_freq_table:
#ifdef CONFIG_ARM_S3C2416_CPUFREQ_VCORESCALE
	regulator_put(s3c_freq->vddarm);
err_vddarm:
#endif
	clk_put(s3c_freq->armclk);
err_armclk:
	clk_put(s3c_freq->hclk);
err_hclk:
	clk_put(s3c_freq->armdiv);

	return ret;
}

static struct cpufreq_driver s3c2416_cpufreq_driver = {
	.flags		= CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= s3c2416_cpufreq_set_target,
	.get		= s3c2416_cpufreq_get_speed,
	.init		= s3c2416_cpufreq_driver_init,
	.name		= "s3c2416",
	.attr		= cpufreq_generic_attr,
};

static int __init s3c2416_cpufreq_init(void)
{
	return cpufreq_register_driver(&s3c2416_cpufreq_driver);
}
module_init(s3c2416_cpufreq_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Copyright (c) 2006-2009 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Vincent Sanders <vince@simtec.co.uk>
 *
 * S3C2440/S3C2442 CPU Frequency scaling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/regs-clock.h>

#include <plat/cpu.h>
#include <plat/cpu-freq-core.h>

static struct clk *xtal;
static struct clk *fclk;
static struct clk *hclk;
static struct clk *armclk;

/* HDIV: 1, 2, 3, 4, 6, 8 */

static inline int within_khz(unsigned long a, unsigned long b)
{
	long diff = a - b;

	return (diff >= -1000 && diff <= 1000);
}

/**
 * s3c2440_cpufreq_calcdivs - calculate divider settings
 * @cfg: The cpu frequency settings.
 *
 * Calcualte the divider values for the given frequency settings
 * specified in @cfg. The values are stored in @cfg for later use
 * by the relevant set routine if the request settings can be reached.
 */
static int s3c2440_cpufreq_calcdivs(struct s3c_cpufreq_config *cfg)
{
	unsigned int hdiv, pdiv;
	unsigned long hclk, fclk, armclk;
	unsigned long hclk_max;

	fclk = cfg->freq.fclk;
	armclk = cfg->freq.armclk;
	hclk_max = cfg->max.hclk;

	s3c_freq_dbg("%s: fclk is %lu, armclk %lu, max hclk %lu\n",
		     __func__, fclk, armclk, hclk_max);

	if (armclk > fclk) {
		printk(KERN_WARNING "%s: armclk > fclk\n", __func__);
		armclk = fclk;
	}

	/* if we are in DVS, we need HCLK to be <= ARMCLK */
	if (armclk < fclk && armclk < hclk_max)
		hclk_max = armclk;

	for (hdiv = 1; hdiv < 9; hdiv++) {
		if (hdiv == 5 || hdiv == 7)
			hdiv++;

		hclk = (fclk / hdiv);
		if (hclk <= hclk_max || within_khz(hclk, hclk_max))
			break;
	}

	s3c_freq_dbg("%s: hclk %lu, div %d\n", __func__, hclk, hdiv);

	if (hdiv > 8)
		goto invalid;

	pdiv = (hclk > cfg->max.pclk) ? 2 : 1;

	if ((hclk / pdiv) > cfg->max.pclk)
		pdiv++;

	s3c_freq_dbg("%s: pdiv %d\n", __func__, pdiv);

	if (pdiv > 2)
		goto invalid;

	pdiv *= hdiv;

	/* calculate a valid armclk */

	if (armclk < hclk)
		armclk = hclk;

	/* if we're running armclk lower than fclk, this really means
	 * that the system should go into dvs mode, which means that
	 * armclk is connected to hclk. */
	if (armclk < fclk) {
		cfg->divs.dvs = 1;
		armclk = hclk;
	} else
		cfg->divs.dvs = 0;

	cfg->freq.armclk = armclk;

	/* store the result, and then return */

	cfg->divs.h_divisor = hdiv;
	cfg->divs.p_divisor = pdiv;

	return 0;

 invalid:
	return -EINVAL;
}

#define CAMDIVN_HCLK_HALF (S3C2440_CAMDIVN_HCLK3_HALF | \
			   S3C2440_CAMDIVN_HCLK4_HALF)

/**
 * s3c2440_cpufreq_setdivs - set the cpu frequency divider settings
 * @cfg: The cpu frequency settings.
 *
 * Set the divisors from the settings in @cfg, which where generated
 * during the calculation phase by s3c2440_cpufreq_calcdivs().
 */
static void s3c2440_cpufreq_setdivs(struct s3c_cpufreq_config *cfg)
{
	unsigned long clkdiv, camdiv;

	s3c_freq_dbg("%s: divsiors: h=%d, p=%d\n", __func__,
		     cfg->divs.h_divisor, cfg->divs.p_divisor);

	clkdiv = __raw_readl(S3C2410_CLKDIVN);
	camdiv = __raw_readl(S3C2440_CAMDIVN);

	clkdiv &= ~(S3C2440_CLKDIVN_HDIVN_MASK | S3C2440_CLKDIVN_PDIVN);
	camdiv &= ~CAMDIVN_HCLK_HALF;

	switch (cfg->divs.h_divisor) {
	case 1:
		clkdiv |= S3C2440_CLKDIVN_HDIVN_1;
		break;

	case 2:
		clkdiv |= S3C2440_CLKDIVN_HDIVN_2;
		break;

	case 6:
		camdiv |= S3C2440_CAMDIVN_HCLK3_HALF;
	case 3:
		clkdiv |= S3C2440_CLKDIVN_HDIVN_3_6;
		break;

	case 8:
		camdiv |= S3C2440_CAMDIVN_HCLK4_HALF;
	case 4:
		clkdiv |= S3C2440_CLKDIVN_HDIVN_4_8;
		break;

	default:
		BUG();	/* we don't expect to get here. */
	}

	if (cfg->divs.p_divisor != cfg->divs.h_divisor)
		clkdiv |= S3C2440_CLKDIVN_PDIVN;

	/* todo - set pclk. */

	/* Write the divisors first with hclk intentionally halved so that
	 * when we write clkdiv we will under-frequency instead of over. We
	 * then make a short delay and remove the hclk halving if necessary.
	 */

	__raw_writel(camdiv | CAMDIVN_HCLK_HALF, S3C2440_CAMDIVN);
	__raw_writel(clkdiv, S3C2410_CLKDIVN);

	ndelay(20);
	__raw_writel(camdiv, S3C2440_CAMDIVN);

	clk_set_parent(armclk, cfg->divs.dvs ? hclk : fclk);
}

static int run_freq_for(unsigned long max_hclk, unsigned long fclk,
			int *divs,
			struct cpufreq_frequency_table *table,
			size_t table_size)
{
	unsigned long freq;
	int index = 0;
	int div;

	for (div = *divs; div > 0; div = *divs++) {
		freq = fclk / div;

		if (freq > max_hclk && div != 1)
			continue;

		freq /= 1000; /* table is in kHz */
		index = s3c_cpufreq_addfreq(table, index, table_size, freq);
		if (index < 0)
			break;
	}

	return index;
}

static int hclk_divs[] = { 1, 2, 3, 4, 6, 8, -1 };

static int s3c2440_cpufreq_calctable(struct s3c_cpufreq_config *cfg,
				     struct cpufreq_frequency_table *table,
				     size_t table_size)
{
	int ret;

	WARN_ON(cfg->info == NULL);
	WARN_ON(cfg->board == NULL);

	ret = run_freq_for(cfg->info->max.hclk,
			   cfg->info->max.fclk,
			   hclk_divs,
			   table, table_size);

	s3c_freq_dbg("%s: returning %d\n", __func__, ret);

	return ret;
}

static struct s3c_cpufreq_info s3c2440_cpufreq_info = {
	.max		= {
		.fclk	= 400000000,
		.hclk	= 133333333,
		.pclk	=  66666666,
	},

	.locktime_m	= 300,
	.locktime_u	= 300,
	.locktime_bits	= 16,

	.name		= "s3c244x",
	.calc_iotiming	= s3c2410_iotiming_calc,
	.set_iotiming	= s3c2410_iotiming_set,
	.get_iotiming	= s3c2410_iotiming_get,
	.set_fvco	= s3c2410_set_fvco,

	.set_refresh	= s3c2410_cpufreq_setrefresh,
	.set_divs	= s3c2440_cpufreq_setdivs,
	.calc_divs	= s3c2440_cpufreq_calcdivs,
	.calc_freqtable	= s3c2440_cpufreq_calctable,

	.debug_io_show  = s3c_cpufreq_debugfs_call(s3c2410_iotiming_debugfs),
};

static int s3c2440_cpufreq_add(struct device *dev,
			       struct subsys_interface *sif)
{
	xtal = s3c_cpufreq_clk_get(NULL, "xtal");
	hclk = s3c_cpufreq_clk_get(NULL, "hclk");
	fclk = s3c_cpufreq_clk_get(NULL, "fclk");
	armclk = s3c_cpufreq_clk_get(NULL, "armclk");

	if (IS_ERR(xtal) || IS_ERR(hclk) || IS_ERR(fclk) || IS_ERR(armclk)) {
		printk(KERN_ERR "%s: failed to get clocks\n", __func__);
		return -ENOENT;
	}

	return s3c_cpufreq_register(&s3c2440_cpufreq_info);
}

static struct subsys_interface s3c2440_cpufreq_interface = {
	.name		= "s3c2440_cpufreq",
	.subsys		= &s3c2440_subsys,
	.add_dev	= s3c2440_cpufreq_add,
};

static int s3c2440_cpufreq_init(void)
{
	return subsys_interface_register(&s3c2440_cpufreq_interface);
}

/* arch_initcall adds the clocks we need, so use subsys_initcall. */
subsys_initcall(s3c2440_cpufreq_init);

static struct subsys_interface s3c2442_cpufreq_interface = {
	.name		= "s3c2442_cpufreq",
	.subsys		= &s3c2442_subsys,
	.add_dev	= s3c2440_cpufreq_add,
};

static int s3c2442_cpufreq_init(void)
{
	return subsys_interface_register(&s3c2442_cpufreq_interface);
}
subsys_initcall(s3c2442_cpufreq_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * Copyright (c) 2009 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX CPU Frequency scaling - debugfs status support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/export.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/err.h>

#include <plat/cpu-freq-core.h>

static struct dentry *dbgfs_root;
static struct dentry *dbgfs_file_io;
static struct dentry *dbgfs_file_info;
static struct dentry *dbgfs_file_board;

#define print_ns(x) ((x) / 10), ((x) % 10)

static void show_max(struct seq_file *seq, struct s3c_freq *f)
{
	seq_printf(seq, "MAX: F=%lu, H=%lu, P=%lu, A=%lu\n",
		   f->fclk, f->hclk, f->pclk, f->armclk);
}

static int board_show(struct seq_file *seq, void *p)
{
	struct s3c_cpufreq_config *cfg;
	struct s3c_cpufreq_board *brd;

	cfg = s3c_cpufreq_getconfig();
	if (!cfg) {
		seq_printf(seq, "no configuration registered\n");
		return 0;
	}

	brd = cfg->board;
	if (!brd) {
		seq_printf(seq, "no board definition set?\n");
		return 0;
	}

	seq_printf(seq, "SDRAM refresh %u ns\n", brd->refresh);
	seq_printf(seq, "auto_io=%u\n", brd->auto_io);
	seq_printf(seq, "need_io=%u\n", brd->need_io);

	show_max(seq, &brd->max);


	return 0;
}

static int fops_board_open(struct inode *inode, struct file *file)
{
	return single_open(file, board_show, NULL);
}

static const struct file_operations fops_board = {
	.open		= fops_board_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static int info_show(struct seq_file *seq, void *p)
{
	struct s3c_cpufreq_config *cfg;

	cfg = s3c_cpufreq_getconfig();
	if (!cfg) {
		seq_printf(seq, "no configuration registered\n");
		return 0;
	}

	seq_printf(seq, "  FCLK %ld Hz\n", cfg->freq.fclk);
	seq_printf(seq, "  HCLK %ld Hz (%lu.%lu ns)\n",
		   cfg->freq.hclk, print_ns(cfg->freq.hclk_tns));
	seq_printf(seq, "  PCLK %ld Hz\n", cfg->freq.hclk);
	seq_printf(seq, "ARMCLK %ld Hz\n", cfg->freq.armclk);
	seq_printf(seq, "\n");

	show_max(seq, &cfg->max);

	seq_printf(seq, "Divisors: P=%d, H=%d, A=%d, dvs=%s\n",
		   cfg->divs.h_divisor, cfg->divs.p_divisor,
		   cfg->divs.arm_divisor, cfg->divs.dvs ? "on" : "off");
	seq_printf(seq, "\n");

	seq_printf(seq, "lock_pll=%u\n", cfg->lock_pll);

	return 0;
}

static int fops_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, info_show, NULL);
}

static const struct file_operations fops_info = {
	.open		= fops_info_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static int io_show(struct seq_file *seq, void *p)
{
	void (*show_bank)(struct seq_file *, struct s3c_cpufreq_config *, union s3c_iobank *);
	struct s3c_cpufreq_config *cfg;
	struct s3c_iotimings *iot;
	union s3c_iobank *iob;
	int bank;

	cfg = s3c_cpufreq_getconfig();
	if (!cfg) {
		seq_printf(seq, "no configuration registered\n");
		return 0;
	}

	show_bank = cfg->info->debug_io_show;
	if (!show_bank) {
		seq_printf(seq, "no code to show bank timing\n");
		return 0;
	}

	iot = s3c_cpufreq_getiotimings();
	if (!iot) {
		seq_printf(seq, "no io timings registered\n");
		return 0;
	}

	seq_printf(seq, "hclk period is %lu.%lu ns\n", print_ns(cfg->freq.hclk_tns));

	for (bank = 0; bank < MAX_BANKS; bank++) {
		iob = &iot->bank[bank];

		seq_printf(seq, "bank %d: ", bank);

		if (!iob->io_2410) {
			seq_printf(seq, "nothing set\n");
			continue;
		}

		show_bank(seq, cfg, iob);
	}

	return 0;
}

static int fops_io_open(struct inode *inode, struct file *file)
{
	return single_open(file, io_show, NULL);
}

static const struct file_operations fops_io = {
	.open		= fops_io_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};


static int __init s3c_freq_debugfs_init(void)
{
	dbgfs_root = debugfs_create_dir("s3c-cpufreq", NULL);
	if (IS_ERR(dbgfs_root)) {
		printk(KERN_ERR "%s: error creating debugfs root\n", __func__);
		return PTR_ERR(dbgfs_root);
	}

	dbgfs_file_io = debugfs_create_file("io-timing", S_IRUGO, dbgfs_root,
					    NULL, &fops_io);

	dbgfs_file_info = debugfs_create_file("info", S_IRUGO, dbgfs_root,
					      NULL, &fops_info);

	dbgfs_file_board = debugfs_create_file("board", S_IRUGO, dbgfs_root,
					       NULL, &fops_board);

	return 0;
}

late_initcall(s3c_freq_debugfs_init);

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright (c) 2006-2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX CPU Frequency scaling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/cpu.h>
#include <plat/cpu-freq-core.h>

#include <mach/regs-clock.h>

/* note, cpufreq support deals in kHz, no Hz */

static struct cpufreq_driver s3c24xx_driver;
static struct s3c_cpufreq_config cpu_cur;
static struct s3c_iotimings s3c24xx_iotiming;
static struct cpufreq_frequency_table *pll_reg;
static unsigned int last_target = ~0;
static unsigned int ftab_size;
static struct cpufreq_frequency_table *ftab;

static struct clk *_clk_mpll;
static struct clk *_clk_xtal;
static struct clk *clk_fclk;
static struct clk *clk_hclk;
static struct clk *clk_pclk;
static struct clk *clk_arm;

#ifdef CONFIG_ARM_S3C24XX_CPUFREQ_DEBUGFS
struct s3c_cpufreq_config *s3c_cpufreq_getconfig(void)
{
	return &cpu_cur;
}

struct s3c_iotimings *s3c_cpufreq_getiotimings(void)
{
	return &s3c24xx_iotiming;
}
#endif /* CONFIG_ARM_S3C24XX_CPUFREQ_DEBUGFS */

static void s3c_cpufreq_getcur(struct s3c_cpufreq_config *cfg)
{
	unsigned long fclk, pclk, hclk, armclk;

	cfg->freq.fclk = fclk = clk_get_rate(clk_fclk);
	cfg->freq.hclk = hclk = clk_get_rate(clk_hclk);
	cfg->freq.pclk = pclk = clk_get_rate(clk_pclk);
	cfg->freq.armclk = armclk = clk_get_rate(clk_arm);

	cfg->pll.driver_data = __raw_readl(S3C2410_MPLLCON);
	cfg->pll.frequency = fclk;

	cfg->freq.hclk_tns = 1000000000 / (cfg->freq.hclk / 10);

	cfg->divs.h_divisor = fclk / hclk;
	cfg->divs.p_divisor = fclk / pclk;
}

static inline void s3c_cpufreq_calc(struct s3c_cpufreq_config *cfg)
{
	unsigned long pll = cfg->pll.frequency;

	cfg->freq.fclk = pll;
	cfg->freq.hclk = pll / cfg->divs.h_divisor;
	cfg->freq.pclk = pll / cfg->divs.p_divisor;

	/* convert hclk into 10ths of nanoseconds for io calcs */
	cfg->freq.hclk_tns = 1000000000 / (cfg->freq.hclk / 10);
}

static inline int closer(unsigned int target, unsigned int n, unsigned int c)
{
	int diff_cur = abs(target - c);
	int diff_new = abs(target - n);

	return (diff_new < diff_cur);
}

static void s3c_cpufreq_show(const char *pfx,
				 struct s3c_cpufreq_config *cfg)
{
	s3c_freq_dbg("%s: Fvco=%u, F=%lu, A=%lu, H=%lu (%u), P=%lu (%u)\n",
		     pfx, cfg->pll.frequency, cfg->freq.fclk, cfg->freq.armclk,
		     cfg->freq.hclk, cfg->divs.h_divisor,
		     cfg->freq.pclk, cfg->divs.p_divisor);
}

/* functions to wrapper the driver info calls to do the cpu specific work */

static void s3c_cpufreq_setio(struct s3c_cpufreq_config *cfg)
{
	if (cfg->info->set_iotiming)
		(cfg->info->set_iotiming)(cfg, &s3c24xx_iotiming);
}

static int s3c_cpufreq_calcio(struct s3c_cpufreq_config *cfg)
{
	if (cfg->info->calc_iotiming)
		return (cfg->info->calc_iotiming)(cfg, &s3c24xx_iotiming);

	return 0;
}

static void s3c_cpufreq_setrefresh(struct s3c_cpufreq_config *cfg)
{
	(cfg->info->set_refresh)(cfg);
}

static void s3c_cpufreq_setdivs(struct s3c_cpufreq_config *cfg)
{
	(cfg->info->set_divs)(cfg);
}

static int s3c_cpufreq_calcdivs(struct s3c_cpufreq_config *cfg)
{
	return (cfg->info->calc_divs)(cfg);
}

static void s3c_cpufreq_setfvco(struct s3c_cpufreq_config *cfg)
{
	cfg->mpll = _clk_mpll;
	(cfg->info->set_fvco)(cfg);
}

static inline void s3c_cpufreq_updateclk(struct clk *clk,
					 unsigned int freq)
{
	clk_set_rate(clk, freq);
}

static int s3c_cpufreq_settarget(struct cpufreq_policy *policy,
				 unsigned int target_freq,
				 struct cpufreq_frequency_table *pll)
{
	struct s3c_cpufreq_freqs freqs;
	struct s3c_cpufreq_config cpu_new;
	unsigned long flags;

	cpu_new = cpu_cur;  /* copy new from current */

	s3c_cpufreq_show("cur", &cpu_cur);

	/* TODO - check for DMA currently outstanding */

	cpu_new.pll = pll ? *pll : cpu_cur.pll;

	if (pll)
		freqs.pll_changing = 1;

	/* update our frequencies */

	cpu_new.freq.armclk = target_freq;
	cpu_new.freq.fclk = cpu_new.pll.frequency;

	if (s3c_cpufreq_calcdivs(&cpu_new) < 0) {
		printk(KERN_ERR "no divisors for %d\n", target_freq);
		goto err_notpossible;
	}

	s3c_freq_dbg("%s: got divs\n", __func__);

	s3c_cpufreq_calc(&cpu_new);

	s3c_freq_dbg("%s: calculated frequencies for new\n", __func__);

	if (cpu_new.freq.hclk != cpu_cur.freq.hclk) {
		if (s3c_cpufreq_calcio(&cpu_new) < 0) {
			printk(KERN_ERR "%s: no IO timings\n", __func__);
			goto err_notpossible;
		}
	}

	s3c_cpufreq_show("new", &cpu_new);

	/* setup our cpufreq parameters */

	freqs.old = cpu_cur.freq;
	freqs.new = cpu_new.freq;

	freqs.freqs.old = cpu_cur.freq.armclk / 1000;
	freqs.freqs.new = cpu_new.freq.armclk / 1000;

	/* update f/h/p clock settings before we issue the change
	 * notification, so that drivers do not need to do anything
	 * special if they want to recalculate on CPUFREQ_PRECHANGE. */

	s3c_cpufreq_updateclk(_clk_mpll, cpu_new.pll.frequency);
	s3c_cpufreq_updateclk(clk_fclk, cpu_new.freq.fclk);
	s3c_cpufreq_updateclk(clk_hclk, cpu_new.freq.hclk);
	s3c_cpufreq_updateclk(clk_pclk, cpu_new.freq.pclk);

	/* start the frequency change */
	cpufreq_freq_transition_begin(policy, &freqs.freqs);

	/* If hclk is staying the same, then we do not need to
	 * re-write the IO or the refresh timings whilst we are changing
	 * speed. */

	local_irq_save(flags);

	/* is our memory clock slowing down? */
	if (cpu_new.freq.hclk < cpu_cur.freq.hclk) {
		s3c_cpufreq_setrefresh(&cpu_new);
		s3c_cpufreq_setio(&cpu_new);
	}

	if (cpu_new.freq.fclk == cpu_cur.freq.fclk) {
		/* not changing PLL, just set the divisors */

		s3c_cpufreq_setdivs(&cpu_new);
	} else {
		if (cpu_new.freq.fclk < cpu_cur.freq.fclk) {
			/* slow the cpu down, then set divisors */

			s3c_cpufreq_setfvco(&cpu_new);
			s3c_cpufreq_setdivs(&cpu_new);
		} else {
			/* set the divisors, then speed up */

			s3c_cpufreq_setdivs(&cpu_new);
			s3c_cpufreq_setfvco(&cpu_new);
		}
	}

	/* did our memory clock speed up */
	if (cpu_new.freq.hclk > cpu_cur.freq.hclk) {
		s3c_cpufreq_setrefresh(&cpu_new);
		s3c_cpufreq_setio(&cpu_new);
	}

	/* update our current settings */
	cpu_cur = cpu_new;

	local_irq_restore(flags);

	/* notify everyone we've done this */
	cpufreq_freq_transition_end(policy, &freqs.freqs, 0);

	s3c_freq_dbg("%s: finished\n", __func__);
	return 0;

 err_notpossible:
	printk(KERN_ERR "no compatible settings for %d\n", target_freq);
	return -EINVAL;
}

/* s3c_cpufreq_target
 *
 * called by the cpufreq core to adjust the frequency that the CPU
 * is currently running at.
 */

static int s3c_cpufreq_target(struct cpufreq_policy *policy,
			      unsigned int target_freq,
			      unsigned int relation)
{
	struct cpufreq_frequency_table *pll;
	unsigned int index;

	/* avoid repeated calls which cause a needless amout of duplicated
	 * logging output (and CPU time as the calculation process is
	 * done) */
	if (target_freq == last_target)
		return 0;

	last_target = target_freq;

	s3c_freq_dbg("%s: policy %p, target %u, relation %u\n",
		     __func__, policy, target_freq, relation);

	if (ftab) {
		if (cpufreq_frequency_table_target(policy, ftab,
						   target_freq, relation,
						   &index)) {
			s3c_freq_dbg("%s: table failed\n", __func__);
			return -EINVAL;
		}

		s3c_freq_dbg("%s: adjust %d to entry %d (%u)\n", __func__,
			     target_freq, index, ftab[index].frequency);
		target_freq = ftab[index].frequency;
	}

	target_freq *= 1000;  /* convert target to Hz */

	/* find the settings for our new frequency */

	if (!pll_reg || cpu_cur.lock_pll) {
		/* either we've not got any PLL values, or we've locked
		 * to the current one. */
		pll = NULL;
	} else {
		struct cpufreq_policy tmp_policy;
		int ret;

		/* we keep the cpu pll table in Hz, to ensure we get an
		 * accurate value for the PLL output. */

		tmp_policy.min = policy->min * 1000;
		tmp_policy.max = policy->max * 1000;
		tmp_policy.cpu = policy->cpu;

		/* cpufreq_frequency_table_target uses a pointer to 'index'
		 * which is the number of the table entry, not the value of
		 * the table entry's index field. */

		ret = cpufreq_frequency_table_target(&tmp_policy, pll_reg,
						     target_freq, relation,
						     &index);

		if (ret < 0) {
			printk(KERN_ERR "%s: no PLL available\n", __func__);
			goto err_notpossible;
		}

		pll = pll_reg + index;

		s3c_freq_dbg("%s: target %u => %u\n",
			     __func__, target_freq, pll->frequency);

		target_freq = pll->frequency;
	}

	return s3c_cpufreq_settarget(policy, target_freq, pll);

 err_notpossible:
	printk(KERN_ERR "no compatible settings for %d\n", target_freq);
	return -EINVAL;
}

struct clk *s3c_cpufreq_clk_get(struct device *dev, const char *name)
{
	struct clk *clk;

	clk = clk_get(dev, name);
	if (IS_ERR(clk))
		printk(KERN_ERR "cpufreq: failed to get clock '%s'\n", name);

	return clk;
}

static int s3c_cpufreq_init(struct cpufreq_policy *policy)
{
	policy->clk = clk_arm;

	policy->cpuinfo.transition_latency = cpu_cur.info->latency;

	if (ftab)
		return cpufreq_table_validate_and_show(policy, ftab);

	return 0;
}

static int __init s3c_cpufreq_initclks(void)
{
	_clk_mpll = s3c_cpufreq_clk_get(NULL, "mpll");
	_clk_xtal = s3c_cpufreq_clk_get(NULL, "xtal");
	clk_fclk = s3c_cpufreq_clk_get(NULL, "fclk");
	clk_hclk = s3c_cpufreq_clk_get(NULL, "hclk");
	clk_pclk = s3c_cpufreq_clk_get(NULL, "pclk");
	clk_arm = s3c_cpufreq_clk_get(NULL, "armclk");

	if (IS_ERR(clk_fclk) || IS_ERR(clk_hclk) || IS_ERR(clk_pclk) ||
	    IS_ERR(_clk_mpll) || IS_ERR(clk_arm) || IS_ERR(_clk_xtal)) {
		printk(KERN_ERR "%s: could not get clock(s)\n", __func__);
		return -ENOENT;
	}

	printk(KERN_INFO "%s: clocks f=%lu,h=%lu,p=%lu,a=%lu\n", __func__,
	       clk_get_rate(clk_fclk) / 1000,
	       clk_get_rate(clk_hclk) / 1000,
	       clk_get_rate(clk_pclk) / 1000,
	       clk_get_rate(clk_arm) / 1000);

	return 0;
}

#ifdef CONFIG_PM
static struct cpufreq_frequency_table suspend_pll;
static unsigned int suspend_freq;

static int s3c_cpufreq_suspend(struct cpufreq_policy *policy)
{
	suspend_pll.frequency = clk_get_rate(_clk_mpll);
	suspend_pll.driver_data = __raw_readl(S3C2410_MPLLCON);
	suspend_freq = clk_get_rate(clk_arm);

	return 0;
}

static int s3c_cpufreq_resume(struct cpufreq_policy *policy)
{
	int ret;

	s3c_freq_dbg("%s: resuming with policy %p\n", __func__, policy);

	last_target = ~0;	/* invalidate last_target setting */

	/* whilst we will be called later on, we try and re-set the
	 * cpu frequencies as soon as possible so that we do not end
	 * up resuming devices and then immediately having to re-set
	 * a number of settings once these devices have restarted.
	 *
	 * as a note, it is expected devices are not used until they
	 * have been un-suspended and at that time they should have
	 * used the updated clock settings.
	 */

	ret = s3c_cpufreq_settarget(NULL, suspend_freq, &suspend_pll);
	if (ret) {
		printk(KERN_ERR "%s: failed to reset pll/freq\n", __func__);
		return ret;
	}

	return 0;
}
#else
#define s3c_cpufreq_resume NULL
#define s3c_cpufreq_suspend NULL
#endif

static struct cpufreq_driver s3c24xx_driver = {
	.flags		= CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.target		= s3c_cpufreq_target,
	.get		= cpufreq_generic_get,
	.init		= s3c_cpufreq_init,
	.suspend	= s3c_cpufreq_suspend,
	.resume		= s3c_cpufreq_resume,
	.name		= "s3c24xx",
};


int s3c_cpufreq_register(struct s3c_cpufreq_info *info)
{
	if (!info || !info->name) {
		printk(KERN_ERR "%s: failed to pass valid information\n",
		       __func__);
		return -EINVAL;
	}

	printk(KERN_INFO "S3C24XX CPU Frequency driver, %s cpu support\n",
	       info->name);

	/* check our driver info has valid data */

	BUG_ON(info->set_refresh == NULL);
	BUG_ON(info->set_divs == NULL);
	BUG_ON(info->calc_divs == NULL);

	/* info->set_fvco is optional, depending on whether there
	 * is a need to set the clock code. */

	cpu_cur.info = info;

	/* Note, driver registering should probably update locktime */

	return 0;
}

int __init s3c_cpufreq_setboard(struct s3c_cpufreq_board *board)
{
	struct s3c_cpufreq_board *ours;

	if (!board) {
		printk(KERN_INFO "%s: no board data\n", __func__);
		return -EINVAL;
	}

	/* Copy the board information so that each board can make this
	 * initdata. */

	ours = kzalloc(sizeof(*ours), GFP_KERNEL);
	if (ours == NULL) {
		printk(KERN_ERR "%s: no memory\n", __func__);
		return -ENOMEM;
	}

	*ours = *board;
	cpu_cur.board = ours;

	return 0;
}

static int __init s3c_cpufreq_auto_io(void)
{
	int ret;

	if (!cpu_cur.info->get_iotiming) {
		printk(KERN_ERR "%s: get_iotiming undefined\n", __func__);
		return -ENOENT;
	}

	printk(KERN_INFO "%s: working out IO settings\n", __func__);

	ret = (cpu_cur.info->get_iotiming)(&cpu_cur, &s3c24xx_iotiming);
	if (ret)
		printk(KERN_ERR "%s: failed to get timings\n", __func__);

	return ret;
}

/* if one or is zero, then return the other, otherwise return the min */
#define do_min(_a, _b) ((_a) == 0 ? (_b) : (_b) == 0 ? (_a) : min(_a, _b))

/**
 * s3c_cpufreq_freq_min - find the minimum settings for the given freq.
 * @dst: The destination structure
 * @a: One argument.
 * @b: The other argument.
 *
 * Create a minimum of each frequency entry in the 'struct s3c_freq',
 * unless the entry is zero when it is ignored and the non-zero argument
 * used.
 */
static void s3c_cpufreq_freq_min(struct s3c_freq *dst,
				 struct s3c_freq *a, struct s3c_freq *b)
{
	dst->fclk = do_min(a->fclk, b->fclk);
	dst->hclk = do_min(a->hclk, b->hclk);
	dst->pclk = do_min(a->pclk, b->pclk);
	dst->armclk = do_min(a->armclk, b->armclk);
}

static inline u32 calc_locktime(u32 freq, u32 time_us)
{
	u32 result;

	result = freq * time_us;
	result = DIV_ROUND_UP(result, 1000 * 1000);

	return result;
}

static void s3c_cpufreq_update_loctkime(void)
{
	unsigned int bits = cpu_cur.info->locktime_bits;
	u32 rate = (u32)clk_get_rate(_clk_xtal);
	u32 val;

	if (bits == 0) {
		WARN_ON(1);
		return;
	}

	val = calc_locktime(rate, cpu_cur.info->locktime_u) << bits;
	val |= calc_locktime(rate, cpu_cur.info->locktime_m);

	printk(KERN_INFO "%s: new locktime is 0x%08x\n", __func__, val);
	__raw_writel(val, S3C2410_LOCKTIME);
}

static int s3c_cpufreq_build_freq(void)
{
	int size, ret;

	if (!cpu_cur.info->calc_freqtable)
		return -EINVAL;

	kfree(ftab);
	ftab = NULL;

	size = cpu_cur.info->calc_freqtable(&cpu_cur, NULL, 0);
	size++;

	ftab = kzalloc(sizeof(*ftab) * size, GFP_KERNEL);
	if (!ftab) {
		printk(KERN_ERR "%s: no memory for tables\n", __func__);
		return -ENOMEM;
	}

	ftab_size = size;

	ret = cpu_cur.info->calc_freqtable(&cpu_cur, ftab, size);
	s3c_cpufreq_addfreq(ftab, ret, size, CPUFREQ_TABLE_END);

	return 0;
}

static int __init s3c_cpufreq_initcall(void)
{
	int ret = 0;

	if (cpu_cur.info && cpu_cur.board) {
		ret = s3c_cpufreq_initclks();
		if (ret)
			goto out;

		/* get current settings */
		s3c_cpufreq_getcur(&cpu_cur);
		s3c_cpufreq_show("cur", &cpu_cur);

		if (cpu_cur.board->auto_io) {
			ret = s3c_cpufreq_auto_io();
			if (ret) {
				printk(KERN_ERR "%s: failed to get io timing\n",
				       __func__);
				goto out;
			}
		}

		if (cpu_cur.board->need_io && !cpu_cur.info->set_iotiming) {
			printk(KERN_ERR "%s: no IO support registered\n",
			       __func__);
			ret = -EINVAL;
			goto out;
		}

		if (!cpu_cur.info->need_pll)
			cpu_cur.lock_pll = 1;

		s3c_cpufreq_update_loctkime();

		s3c_cpufreq_freq_min(&cpu_cur.max, &cpu_cur.board->max,
				     &cpu_cur.info->max);

		if (cpu_cur.info->calc_freqtable)
			s3c_cpufreq_build_freq();

		ret = cpufreq_register_driver(&s3c24xx_driver);
	}

 out:
	return ret;
}

late_initcall(s3c_cpufreq_initcall);

/**
 * s3c_plltab_register - register CPU PLL table.
 * @plls: The list of PLL entries.
 * @plls_no: The size of the PLL entries @plls.
 *
 * Register the given set of PLLs with the system.
 */
int s3c_plltab_register(struct cpufreq_frequency_table *plls,
			       unsigned int plls_no)
{
	struct cpufreq_frequency_table *vals;
	unsigned int size;

	size = sizeof(*vals) * (plls_no + 1);

	vals = kzalloc(size, GFP_KERNEL);
	if (vals) {
		memcpy(vals, plls, size);
		pll_reg = vals;

		/* write a terminating entry, we don't store it in the
		 * table that is stored in the kernel */
		vals += plls_no;
		vals->frequency = CPUFREQ_TABLE_END;

		printk(KERN_INFO "cpufreq: %d PLL entries\n", plls_no);
	} else
		printk(KERN_ERR "cpufreq: no memory for PLL tables\n");

	return vals ? 0 : -ENOMEM;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * Copyright 2009 Wolfson Microelectronics plc
 *
 * S3C64xx CPUfreq Support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "cpufreq: " fmt

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>

static struct regulator *vddarm;
static unsigned long regulator_latency;

#ifdef CONFIG_CPU_S3C6410
struct s3c64xx_dvfs {
	unsigned int vddarm_min;
	unsigned int vddarm_max;
};

static struct s3c64xx_dvfs s3c64xx_dvfs_table[] = {
	[0] = { 1000000, 1150000 },
	[1] = { 1050000, 1150000 },
	[2] = { 1100000, 1150000 },
	[3] = { 1200000, 1350000 },
	[4] = { 1300000, 1350000 },
};

static struct cpufreq_frequency_table s3c64xx_freq_table[] = {
	{ 0, 0,  66000 },
	{ 0, 0, 100000 },
	{ 0, 0, 133000 },
	{ 0, 1, 200000 },
	{ 0, 1, 222000 },
	{ 0, 1, 266000 },
	{ 0, 2, 333000 },
	{ 0, 2, 400000 },
	{ 0, 2, 532000 },
	{ 0, 2, 533000 },
	{ 0, 3, 667000 },
	{ 0, 4, 800000 },
	{ 0, 0, CPUFREQ_TABLE_END },
};
#endif

static int s3c64xx_cpufreq_set_target(struct cpufreq_policy *policy,
				      unsigned int index)
{
	struct s3c64xx_dvfs *dvfs;
	unsigned int old_freq, new_freq;
	int ret;

	old_freq = clk_get_rate(policy->clk) / 1000;
	new_freq = s3c64xx_freq_table[index].frequency;
	dvfs = &s3c64xx_dvfs_table[s3c64xx_freq_table[index].driver_data];

#ifdef CONFIG_REGULATOR
	if (vddarm && new_freq > old_freq) {
		ret = regulator_set_voltage(vddarm,
					    dvfs->vddarm_min,
					    dvfs->vddarm_max);
		if (ret != 0) {
			pr_err("Failed to set VDDARM for %dkHz: %d\n",
			       new_freq, ret);
			return ret;
		}
	}
#endif

	ret = clk_set_rate(policy->clk, new_freq * 1000);
	if (ret < 0) {
		pr_err("Failed to set rate %dkHz: %d\n",
		       new_freq, ret);
		return ret;
	}

#ifdef CONFIG_REGULATOR
	if (vddarm && new_freq < old_freq) {
		ret = regulator_set_voltage(vddarm,
					    dvfs->vddarm_min,
					    dvfs->vddarm_max);
		if (ret != 0) {
			pr_err("Failed to set VDDARM for %dkHz: %d\n",
			       new_freq, ret);
			if (clk_set_rate(policy->clk, old_freq * 1000) < 0)
				pr_err("Failed to restore original clock rate\n");

			return ret;
		}
	}
#endif

	pr_debug("Set actual frequency %lukHz\n",
		 clk_get_rate(policy->clk) / 1000);

	return 0;
}

#ifdef CONFIG_REGULATOR
static void __init s3c64xx_cpufreq_config_regulator(void)
{
	int count, v, i, found;
	struct cpufreq_frequency_table *freq;
	struct s3c64xx_dvfs *dvfs;

	count = regulator_count_voltages(vddarm);
	if (count < 0) {
		pr_err("Unable to check supported voltages\n");
	}

	if (!count)
		goto out;

	cpufreq_for_each_valid_entry(freq, s3c64xx_freq_table) {
		dvfs = &s3c64xx_dvfs_table[freq->driver_data];
		found = 0;

		for (i = 0; i < count; i++) {
			v = regulator_list_voltage(vddarm, i);
			if (v >= dvfs->vddarm_min && v <= dvfs->vddarm_max)
				found = 1;
		}

		if (!found) {
			pr_debug("%dkHz unsupported by regulator\n",
				 freq->frequency);
			freq->frequency = CPUFREQ_ENTRY_INVALID;
		}
	}

out:
	/* Guess based on having to do an I2C/SPI write; in future we
	 * will be able to query the regulator performance here. */
	regulator_latency = 1 * 1000 * 1000;
}
#endif

static int s3c64xx_cpufreq_driver_init(struct cpufreq_policy *policy)
{
	int ret;
	struct cpufreq_frequency_table *freq;

	if (policy->cpu != 0)
		return -EINVAL;

	if (s3c64xx_freq_table == NULL) {
		pr_err("No frequency information for this CPU\n");
		return -ENODEV;
	}

	policy->clk = clk_get(NULL, "armclk");
	if (IS_ERR(policy->clk)) {
		pr_err("Unable to obtain ARMCLK: %ld\n",
		       PTR_ERR(policy->clk));
		return PTR_ERR(policy->clk);
	}

#ifdef CONFIG_REGULATOR
	vddarm = regulator_get(NULL, "vddarm");
	if (IS_ERR(vddarm)) {
		ret = PTR_ERR(vddarm);
		pr_err("Failed to obtain VDDARM: %d\n", ret);
		pr_err("Only frequency scaling available\n");
		vddarm = NULL;
	} else {
		s3c64xx_cpufreq_config_regulator();
	}
#endif

	cpufreq_for_each_entry(freq, s3c64xx_freq_table) {
		unsigned long r;

		/* Check for frequencies we can generate */
		r = clk_round_rate(policy->clk, freq->frequency * 1000);
		r /= 1000;
		if (r != freq->frequency) {
			pr_debug("%dkHz unsupported by clock\n",
				 freq->frequency);
			freq->frequency = CPUFREQ_ENTRY_INVALID;
		}

		/* If we have no regulator then assume startup
		 * frequency is the maximum we can support. */
		if (!vddarm && freq->frequency > clk_get_rate(policy->clk) / 1000)
			freq->frequency = CPUFREQ_ENTRY_INVALID;
	}

	/* Datasheet says PLL stabalisation time (if we were to use
	 * the PLLs, which we don't currently) is ~300us worst case,
	 * but add some fudge.
	 */
	ret = cpufreq_generic_init(policy, s3c64xx_freq_table,
			(500 * 1000) + regulator_latency);
	if (ret != 0) {
		pr_err("Failed to configure frequency table: %d\n",
		       ret);
		regulator_put(vddarm);
		clk_put(policy->clk);
	}

	return ret;
}

static struct cpufreq_driver s3c64xx_cpufreq_driver = {
	.flags		= CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= s3c64xx_cpufreq_set_target,
	.get		= cpufreq_generic_get,
	.init		= s3c64xx_cpufreq_driver_init,
	.name		= "s3c",
};

static int __init s3c64xx_cpufreq_init(void)
{
	return cpufreq_register_driver(&s3c64xx_cpufreq_driver);
}
module_init(s3c64xx_cpufreq_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             