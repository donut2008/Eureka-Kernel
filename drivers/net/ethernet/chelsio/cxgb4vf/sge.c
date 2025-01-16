/*
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * CPU frequency scaling for S5PC110/S5PV210
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/cpufreq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/regulator/consumer.h>

static void __iomem *clk_base;
static void __iomem *dmc_base[2];

#define S5P_CLKREG(x)		(clk_base + (x))

#define S5P_APLL_LOCK		S5P_CLKREG(0x00)
#define S5P_APLL_CON		S5P_CLKREG(0x100)
#define S5P_CLK_SRC0		S5P_CLKREG(0x200)
#define S5P_CLK_SRC2		S5P_CLKREG(0x208)
#define S5P_CLK_DIV0		S5P_CLKREG(0x300)
#define S5P_CLK_DIV2		S5P_CLKREG(0x308)
#define S5P_CLK_DIV6		S5P_CLKREG(0x318)
#define S5P_CLKDIV_STAT0	S5P_CLKREG(0x1000)
#define S5P_CLKDIV_STAT1	S5P_CLKREG(0x1004)
#define S5P_CLKMUX_STAT0	S5P_CLKREG(0x1100)
#define S5P_CLKMUX_STAT1	S5P_CLKREG(0x1104)

#define S5P_ARM_MCS_CON		S5P_CLKREG(0x6100)

/* CLKSRC0 */
#define S5P_CLKSRC0_MUX200_SHIFT	(16)
#define S5P_CLKSRC0_MUX200_MASK		(0x1 << S5P_CLKSRC0_MUX200_SHIFT)
#define S5P_CLKSRC0_MUX166_MASK		(0x1<<20)
#define S5P_CLKSRC0_MUX133_MASK		(0x1<<24)

/* CLKSRC2 */
#define S5P_CLKSRC2_G3D_SHIFT           (0)
#define S5P_CLKSRC2_G3D_MASK            (0x3 << S5P_CLKSRC2_G3D_SHIFT)
#define S5P_CLKSRC2_MFC_SHIFT           (4)
#define S5P_CLKSRC2_MFC_MASK            (0x3 << S5P_CLKSRC2_MFC_SHIFT)

/* CLKDIV0 */
#define S5P_CLKDIV0_APLL_SHIFT		(0)
#define S5P_CLKDIV0_APLL_MASK		(0x7 << S5P_CLKDIV0_APLL_SHIFT)
#define S5P_CLKDIV0_A2M_SHIFT		(4)
#define S5P_CLKDIV0_A2M_MASK		(0x7 << S5P_CLKDIV0_A2M_SHIFT)
#define S5P_CLKDIV0_HCLK200_SHIFT	(8)
#define S5P_CLKDIV0_HCLK200_MASK	(0x7 << S5P_CLKDIV0_HCLK200_SHIFT)
#define S5P_CLKDIV0_PCLK100_SHIFT	(12)
#define S5P_CLKDIV0_PCLK100_MASK	(0x7 << S5P_CLKDIV0_PCLK100_SHIFT)
#define S5P_CLKDIV0_HCLK166_SHIFT	(16)
#define S5P_CLKDIV0_HCLK166_MASK	(0xF << S5P_CLKDIV0_HCLK166_SHIFT)
#define S5P_CLKDIV0_PCLK83_SHIFT	(20)
#define S5P_CLKDIV0_PCLK83_MASK		(0x7 << S5P_CLKDIV0_PCLK83_SHIFT)
#define S5P_CLKDIV0_HCLK133_SHIFT	(24)
#define S5P_CLKDIV0_HCLK133_MASK	(0xF << S5P_CLKDIV0_HCLK133_SHIFT)
#define S5P_CLKDIV0_PCLK66_SHIFT	(28)
#define S5P_CLKDIV0_PCLK66_MASK		(0x7 << S5P_CLKDIV0_PCLK66_SHIFT)

/* CLKDIV2 */
#define S5P_CLKDIV2_G3D_SHIFT           (0)
#define S5P_CLKDIV2_G3D_MASK            (0xF << S5P_CLKDIV2_G3D_SHIFT)
#define S5P_CLKDIV2_MFC_SHIFT           (4)
#define S5P_CLKDIV2_MFC_MASK            (0xF << S5P_CLKDIV2_MFC_SHIFT)

/* CLKDIV6 */
#define S5P_CLKDIV6_ONEDRAM_SHIFT       (28)
#define S5P_CLKDIV6_ONEDRAM_MASK        (0xF << S5P_CLKDIV6_ONEDRAM_SHIFT)

static struct clk *dmc0_clk;
static struct clk *dmc1_clk;
static DEFINE_MUTEX(set_freq_lock);

/* APLL M,P,S values for 1G/800Mhz */
#define APLL_VAL_1000	((1 << 31) | (125 << 16) | (3 << 8) | 1)
#define APLL_VAL_800	((1 << 31) | (100 << 16) | (3 << 8) | 1)

/* Use 800MHz when entering sleep mode */
#define SLEEP_FREQ	(800 * 1000)

/* Tracks if cpu freqency can be updated anymore */
static bool no_cpufreq_access;

/*
 * DRAM configurations to calculate refresh counter for changing
 * frequency of memory.
 */
struct dram_conf {
	unsigned long freq;	/* HZ */
	unsigned long refresh;	/* DRAM refresh counter * 1000 */
};

/* DRAM configuration (DMC0 and DMC1) */
static struct dram_conf s5pv210_dram_conf[2];

enum perf_level {
	L0, L1, L2, L3, L4,
};

enum s5pv210_mem_type {
	LPDDR	= 0x1,
	LPDDR2	= 0x2,
	DDR2	= 0x4,
};

enum s5pv210_dmc_port {
	DMC0 = 0,
	DMC1,
};

static struct cpufreq_frequency_table s5pv210_freq_table[] = {
	{0, L0, 1000*1000},
	{0, L1, 800*1000},
	{0, L2, 400*1000},
	{0, L3, 200*1000},
	{0, L4, 100*1000},
	{0, 0, CPUFREQ_TABLE_END},
};

static struct regulator *arm_regulator;
static struct regulator *int_regulator;

struct s5pv210_dvs_conf {
	int arm_volt;	/* uV */
	int int_volt;	/* uV */
};

static const int arm_volt_max = 1350000;
static const int int_volt_max = 1250000;

static struct s5pv210_dvs_conf dvs_conf[] = {
	[L0] = {
		.arm_volt	= 1250000,
		.int_volt	= 1100000,
	},
	[L1] = {
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	},
	[L2] = {
		.arm_volt	= 1050000,
		.int_volt	= 1100000,
	},
	[L3] = {
		.arm_volt	= 950000,
		.int_volt	= 1100000,
	},
	[L4] = {
		.arm_volt	= 950000,
		.int_volt	= 1000000,
	},
};

static u32 clkdiv_val[5][11] = {
	/*
	 * Clock divider value for following
	 * { APLL, A2M, HCLK_MSYS, PCLK_MSYS,
	 *   HCLK_DSYS, PCLK_DSYS, HCLK_PSYS, PCLK_PSYS,
	 *   ONEDRAM, MFC, G3D }
	 */

	/* L0 : [1000/200/100][166/83][133/66][200/200] */
	{0, 4, 4, 1, 3, 1, 4, 1, 3, 0, 0},

	/* L1 : [800/200/100][166/83][133/66][200/200] */
	{0, 3, 3, 1, 3, 1, 4, 1, 3, 0, 0},

	/* L2 : [400/200/100][166/83][133/66][200/200] */
	{1, 3, 1, 1, 3, 1, 4, 1, 3, 0, 0},

	/* L3 : [200/200/100][166/83][133/66][200/200] */
	{3, 3, 1, 1, 3, 1, 4, 1, 3, 0, 0},

	/* L4 : [100/100/100][83/83][66/66][100/100] */
	{7, 7, 0, 0, 7, 0, 9, 0, 7, 0, 0},
};

/*
 * This function set DRAM refresh counter
 * accoriding to operating frequency of DRAM
 * ch: DMC port number 0 or 1
 * freq: Operating frequency of DRAM(KHz)
 */
static void s5pv210_set_refresh(enum s5pv210_dmc_port ch, unsigned long freq)
{
	unsigned long tmp, tmp1;
	void __iomem *reg = NULL;

	if (ch == DMC0) {
		reg = (dmc_base[0] + 0x30);
	} else if (ch == DMC1) {
		reg = (dmc_base[1] + 0x30);
	} else {
		printk(KERN_ERR "Cannot find DMC port\n");
		return;
	}

	/* Find current DRAM frequency */
	tmp = s5pv210_dram_conf[ch].freq;

	tmp /= freq;

	tmp1 = s5pv210_dram_conf[ch].refresh;

	tmp1 /= tmp;

	__raw_writel(tmp1, reg);
}

static int s5pv210_target(struct cpufreq_policy *policy, unsigned int index)
{
	unsigned long reg;
	unsigned int priv_index;
	unsigned int pll_changing = 0;
	unsigned int bus_speed_changing = 0;
	unsigned int old_freq, new_freq;
	int arm_volt, int_volt;
	int ret = 0;

	mutex_lock(&set_freq_lock);

	if (no_cpufreq_access) {
		pr_err("Denied access to %s as it is disabled temporarily\n",
		       __func__);
		ret = -EINVAL;
		goto exit;
	}

	old_freq = policy->cur;
	new_freq = s5pv210_freq_table[index].frequency;

	/* Finding current running level index */
	if (cpufreq_frequency_table_target(policy, s5pv210_freq_table,
					   old_freq, CPUFREQ_RELATION_H,
					   &priv_index)) {
		ret = -EINVAL;
		goto exit;
	}

	arm_volt = dvs_conf[index].arm_volt;
	int_volt = dvs_conf[index].int_volt;

	if (new_freq > old_freq) {
		ret = regulator_set_voltage(arm_regulator,
				arm_volt, arm_volt_max);
		if (ret)
			goto exit;

		ret = regulator_set_voltage(int_regulator,
				int_volt, int_volt_max);
		if (ret)
			goto exit;
	}

	/* Check if there need to change PLL */
	if ((index == L0) || (priv_index == L0))
		pll_changing = 1;

	/* Check if there need to change System bus clock */
	if ((index == L4) || (priv_index == L4))
		bus_speed_changing = 1;

	if (bus_speed_changing) {
		/*
		 * Reconfigure DRAM refresh counter value for minimum
		 * temporary clock while changing divider.
		 * expected clock is 83Mhz : 7.8usec/(1/83Mhz) = 0x287
		 */
		if (pll_changing)
			s5pv210_set_refresh(DMC1, 83000);
		else
			s5pv210_set_refresh(DMC1, 100000);

		s5pv210_set_refresh(DMC0, 83000);
	}

	/*
	 * APLL should be changed in this level
	 * APLL -> MPLL(for stable transition) -> APLL
	 * Some clock source's clock API are not prepared.
	 * Do not use clock API in below code.
	 */
	if (pll_changing) {
		/*
		 * 1. Temporary Change divider for MFC and G3D
		 * SCLKA2M(200/1=200)->(200/4=50)Mhz
		 */
		reg = __raw_readl(S5P_CLK_DIV2);
		reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
		reg |= (3 << S5P_CLKDIV2_G3D_SHIFT) |
			(3 << S5P_CLKDIV2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV2);

		/* For MFC, G3D dividing */
		do {
			reg = __raw_readl(S5P_CLKDIV_STAT0);
		} while (reg & ((1 << 16) | (1 << 17)));

		/*
		 * 2. Change SCLKA2M(200Mhz)to SCLKMPLL in MFC_MUX, G3D MUX
		 * (200/4=50)->(667/4=166)Mhz
		 */
		reg = __raw_readl(S5P_CLK_SRC2);
		reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
		reg |= (1 << S5P_CLKSRC2_G3D_SHIFT) |
			(1 << S5P_CLKSRC2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC2);

		do {
			reg = __raw_readl(S5P_CLKMUX_STAT1);
		} while (reg & ((1 << 7) | (1 << 3)));

		/*
		 * 3. DMC1 refresh count for 133Mhz if (index == L4) is
		 * true refresh counter is already programed in upper
		 * code. 0x287@83Mhz
		 */
		if (!bus_speed_changing)
			s5pv210_set_refresh(DMC1, 133000);

		/* 4. SCLKAPLL -> SCLKMPLL */
		reg = __raw_readl(S5P_CLK_SRC0);
		reg &= ~(S5P_CLKSRC0_MUX200_MASK);
		reg |= (0x1 << S5P_CLKSRC0_MUX200_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC0);

		do {
			reg = __raw_readl(S5P_CLKMUX_STAT0);
		} while (reg & (0x1 << 18));

	}

	/* Change divider */
	reg = __raw_readl(S5P_CLK_DIV0);

	reg &= ~(S5P_CLKDIV0_APLL_MASK | S5P_CLKDIV0_A2M_MASK |
		S5P_CLKDIV0_HCLK200_MASK | S5P_CLKDIV0_PCLK100_MASK |
		S5P_CLKDIV0_HCLK166_MASK | S5P_CLKDIV0_PCLK83_MASK |
		S5P_CLKDIV0_HCLK133_MASK | S5P_CLKDIV0_PCLK66_MASK);

	reg |= ((clkdiv_val[index][0] << S5P_CLKDIV0_APLL_SHIFT) |
		(clkdiv_val[index][1] << S5P_CLKDIV0_A2M_SHIFT) |
		(clkdiv_val[index][2] << S5P_CLKDIV0_HCLK200_SHIFT) |
		(clkdiv_val[index][3] << S5P_CLKDIV0_PCLK100_SHIFT) |
		(clkdiv_val[index][4] << S5P_CLKDIV0_HCLK166_SHIFT) |
		(clkdiv_val[index][5] << S5P_CLKDIV0_PCLK83_SHIFT) |
		(clkdiv_val[index][6] << S5P_CLKDIV0_HCLK133_SHIFT) |
		(clkdiv_val[index][7] << S5P_CLKDIV0_PCLK66_SHIFT));

	__raw_writel(reg, S5P_CLK_DIV0);

	do {
		reg = __raw_readl(S5P_CLKDIV_STAT0);
	} while (reg & 0xff);

	/* ARM MCS value changed */
	reg = __raw_readl(S5P_ARM_MCS_CON);
	reg &= ~0x3;
	if (index >= L3)
		reg |= 0x3;
	else
		reg |= 0x1;

	__raw_writel(reg, S5P_ARM_MCS_CON);

	if (pll_changing) {
		/* 5. Set Lock time = 30us*24Mhz = 0x2cf */
		__raw_writel(0x2cf, S5P_APLL_LOCK);

		/*
		 * 6. Turn on APLL
		 * 6-1. Set PMS values
		 * 6-2. Wait untile the PLL is locked
		 */
		if (index == L0)
			__raw_writel(APLL_VAL_1000, S5P_APLL_CON);
		else
			__raw_writel(APLL_VAL_800, S5P_APLL_CON);

		do {
			reg = __raw_readl(S5P_APLL_CON);
		} while (!(reg & (0x1 << 29)));

		/*
		 * 7. Change souce clock from SCLKMPLL(667Mhz)
		 * to SCLKA2M(200Mhz) in MFC_MUX and G3D MUX
		 * (667/4=166)->(200/4=50)Mhz
		 */
		reg = __raw_readl(S5P_CLK_SRC2);
		reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
		reg |= (0 << S5P_CLKSRC2_G3D_SHIFT) |
			(0 << S5P_CLKSRC2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC2);

		do {
			reg = __raw_readl(S5P_CLKMUX_STAT1);
		} while (reg & ((1 << 7) | (1 << 3)));

		/*
		 * 8. Change divider for MFC and G3D
		 * (200/4=50)->(200/1=200)Mhz
		 */
		reg = __raw_readl(S5P_CLK_DIV2);
		reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
		reg |= (clkdiv_val[index][10] << S5P_CLKDIV2_G3D_SHIFT) |
			(clkdiv_val[index][9] << S5P_CLKDIV2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV2);

		/* For MFC, G3D dividing */
		do {
			reg = __raw_readl(S5P_CLKDIV_STAT0);
		} while (reg & ((1 << 16) | (1 << 17)));

		/* 9. Change MPLL to APLL in MSYS_MUX */
		reg = __raw_readl(S5P_CLK_SRC0);
		reg &= ~(S5P_CLKSRC0_MUX200_MASK);
		reg |= (0x0 << S5P_CLKSRC0_MUX200_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC0);

		do {
			reg = __raw_readl(S5P_CLKMUX_STAT0);
		} while (reg & (0x1 << 18));

		/*
		 * 10. DMC1 refresh counter
		 * L4 : DMC1 = 100Mhz 7.8us/(1/100) = 0x30c
		 * Others : DMC1 = 200Mhz 7.8us/(1/200) = 0x618
		 */
		if (!bus_speed_changing)
			s5pv210_set_refresh(DMC1, 200000);
	}

	/*
	 * L4 level need to change memory bus speed, hence onedram clock divier
	 * and memory refresh parameter should be changed
	 */
	if (bus_speed_changing) {
		reg = __raw_readl(S5P_CLK_DIV6);
		reg &= ~S5P_CLKDIV6_ONEDRAM_MASK;
		reg |= (clkdiv_val[index][8] << S5P_CLKDIV6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV6);

		do {
			reg = __raw_readl(S5P_CLKDIV_STAT1);
		} while (reg & (1 << 15));

		/* Reconfigure DRAM refresh counter value */
		if (index != L4) {
			/*
			 * DMC0 : 166Mhz
			 * DMC1 : 200Mhz
			 */
			s5pv210_set_refresh(DMC0, 166000);
			s5pv210_set_refresh(DMC1, 200000);
		} else {
			/*
			 * DMC0 : 83Mhz
			 * DMC1 : 100Mhz
			 */
			s5pv210_set_refresh(DMC0, 83000);
			s5pv210_set_refresh(DMC1, 100000);
		}
	}

	if (new_freq < old_freq) {
		regulator_set_voltage(int_regulator,
				int_volt, int_volt_max);

		regulator_set_voltage(arm_regulator,
				arm_volt, arm_volt_max);
	}

	printk(KERN_DEBUG "Perf changed[L%d]\n", index);

exit:
	mutex_unlock(&set_freq_lock);
	return ret;
}

static int check_mem_type(void __iomem *dmc_reg)
{
	unsigned long val;

	val = __raw_readl(dmc_reg + 0x4);
	val = (val & (0xf << 8));

	return val >> 8;
}

static int s5pv210_cpu_init(struct cpufreq_policy *policy)
{
	unsigned long mem_type;
	int ret;

	policy->clk = clk_get(NULL, "armclk");
	if (IS_ERR(policy->clk))
		return PTR_ERR(policy->clk);

	dmc0_clk = clk_get(NULL, "sclk_dmc0");
	if (IS_ERR(dmc0_clk)) {
		ret = PTR_ERR(dmc0_clk);
		goto out_dmc0;
	}

	dmc1_clk = clk_get(NULL, "hclk_msys");
	if (IS_ERR(dmc1_clk)) {
		ret = PTR_ERR(dmc1_clk);
		goto out_dmc1;
	}

	if (policy->cpu != 0) {
		ret = -EINVAL;
		goto out_dmc1;
	}

	/*
	 * check_mem_type : This driver only support LPDDR & LPDDR2.
	 * other memory type is not supported.
	 */
	mem_type = check_mem_type(dmc_base[0]);

	if ((mem_type != LPDDR) && (mem_type != LPDDR2)) {
		printk(KERN_ERR "CPUFreq doesn't support this memory type\n");
		ret = -EINVAL;
		goto out_dmc1;
	}

	/* Find current refresh counter and frequency each DMC */
	s5pv210_dram_conf[0].refresh = (__raw_readl(dmc_base[0] + 0x30) * 1000);
	s5pv210_dram_conf[0].freq = clk_get_rate(dmc0_clk);

	s5pv210_dram_conf[1].refresh = (__raw_readl(dmc_base[1] + 0x30) * 1000);
	s5pv210_dram_conf[1].freq = clk_get_rate(dmc1_clk);

	policy->suspend_freq = SLEEP_FREQ;
	return cpufreq_generic_init(policy, s5pv210_freq_table, 40000);

out_dmc1:
	clk_put(dmc0_clk);
out_dmc0:
	clk_put(policy->clk);
	return ret;
}

static int s5pv210_cpufreq_reboot_notifier_event(struct notifier_block *this,
						 unsigned long event, void *ptr)
{
	int ret;

	ret = cpufreq_driver_target(cpufreq_cpu_get(0), SLEEP_FREQ, 0);
	if (ret < 0)
		return NOTIFY_BAD;

	no_cpufreq_access = true;
	return NOTIFY_DONE;
}

static struct cpufreq_driver s5pv210_driver = {
	.flags		= CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= s5pv210_target,
	.get		= cpufreq_generic_get,
	.init		= s5pv210_cpu_init,
	.name		= "s5pv210",
#ifdef CONFIG_PM
	.suspend	= cpufreq_generic_suspend,
	.resume		= cpufreq_generic_suspend, /* We need to set SLEEP FREQ again */
#endif
};

static struct notifier_block s5pv210_cpufreq_reboot_notifier = {
	.notifier_call = s5pv210_cpufreq_reboot_notifier_event,
};

static int s5pv210_cpufreq_probe(struct platform_device *pdev)
{
	struct device_node *np;
	int id;

	/*
	 * HACK: This is a temporary workaround to get access to clock
	 * and DMC controller registers directly and remove static mappings
	 * and dependencies on platform headers. It is necessary to enable
	 * S5PV210 multi-platform support and will be removed together with
	 * this whole driver as soon as S5PV210 gets migrated to use
	 * cpufreq-dt driver.
	 */
	np = of_find_compatible_node(NULL, NULL, "samsung,s5pv210-clock");
	if (!np) {
		pr_err("%s: failed to find clock controller DT node\n",
			__func__);
		return -ENODEV;
	}

	clk_base = of_iomap(np, 0);
	if (!clk_base) {
		pr_err("%s: failed to map clock registers\n", __func__);
		return -EFAULT;
	}

	for_each_compatible_node(np, NULL, "samsung,s5pv210-dmc") {
		id = of_alias_get_id(np, "dmc");
		if (id < 0 || id >= ARRAY_SIZE(dmc_base)) {
			pr_err("%s: failed to get alias of dmc node '%s'\n",
				__func__, np->name);
			return id;
		}

		dmc_base[id] = of_iomap(np, 0);
		if (!dmc_base[id]) {
			pr_err("%s: failed to map dmc%d registers\n",
				__func__, id);
			return -EFAULT;
		}
	}

	for (id = 0; id < ARRAY_SIZE(dmc_base); ++id) {
		if (!dmc_base[id]) {
			pr_err("%s: failed to find dmc%d node\n", __func__, id);
			return -ENODEV;
		}
	}

	arm_regulator = regulator_get(NULL, "vddarm");
	if (IS_ERR(arm_regulator)) {
		pr_err("failed to get regulator vddarm");
		return PTR_ERR(arm_regulator);
	}

	int_regulator = regulator_get(NULL, "vddint");
	if (IS_ERR(int_regulator)) {
		pr_err("failed to get regulator vddint");
		regulator_put(arm_regulator);
		return PTR_ERR(int_regulator);
	}

	register_reboot_notifier(&s5pv210_cpufreq_reboot_notifier);

	return cpufreq_register_driver(&s5pv210_driver);
}

static struct platform_driver s5pv210_cpufreq_platdrv = {
	.driver = {
		.name	= "s5pv210-cpufreq",
	},
	.probe = s5pv210_cpufreq_probe,
};
builtin_platform_driver(s5pv210_cpufreq_platdrv);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * cpu-sa1100.c: clock scaling for the SA1100
 *
 * Copyright (C) 2000 2001, The Delft University of Technology
 *
 * Authors:
 * - Johan Pouwelse (J.A.Pouwelse@its.tudelft.nl): initial version
 * - Erik Mouw (J.A.K.Mouw@its.tudelft.nl):
 *   - major rewrite for linux-2.3.99
 *   - rewritten for the more generic power management scheme in
 *     linux-2.4.5-rmk1
 *
 * This software has been developed while working on the LART
 * computing board (http://www.lartmaker.nl/), which is
 * sponsored by the Mobile Multi-media Communications
 * (http://www.mobimedia.org/) and Ubiquitous Communications
 * (http://www.ubicom.tudelft.nl/) projects.
 *
 * The authors can be reached at:
 *
 *  Erik Mouw
 *  Information and Communication Theory Group
 *  Faculty of Information Technology and Systems
 *  Delft University of Technology
 *  P.O. Box 5031
 *  2600 GA Delft
 *  The Netherlands
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Theory of operations
 * ====================
 *
 * Clock scaling can be used to lower the power consumption of the CPU
 * core. This will give you a somewhat longer running time.
 *
 * The SA-1100 has a single register to change the core clock speed:
 *
 *   PPCR      0x90020014    PLL config
 *
 * However, the DRAM timings are closely related to the core clock
 * speed, so we need to change these, too. The used registers are:
 *
 *   MDCNFG    0xA0000000    DRAM config
 *   MDCAS0    0xA0000004    Access waveform
 *   MDCAS1    0xA0000008    Access waveform
 *   MDCAS2    0xA000000C    Access waveform
 *
 * Care must be taken to change the DRAM parameters the correct way,
 * because otherwise the DRAM becomes unusable and the kernel will
 * crash.
 *
 * The simple solution to avoid a kernel crash is to put the actual
 * clock change in ROM and jump to that code from the kernel. The main
 * disadvantage is that the ROM has to be modified, which is not
 * possible on all SA-1100 platforms. Another disadvantage is that
 * jumping to ROM makes clock switching unnecessary complicated.
 *
 * The idea behind this driver is that the memory configuration can be
 * changed while running from DRAM (even with interrupts turned on!)
 * as long as all re-configuration steps yield a valid DRAM
 * configuration. The advantages are clear: it will run on all SA-1100
 * platforms, and the code is very simple.
 *
 * If you really want to understand what is going on in
 * sa1100_update_dram_timings(), you'll have to read sections 8.2,
 * 9.5.7.3, and 10.2 from the "Intel StrongARM SA-1100 Microprocessor
 * Developers Manual" (available for free from Intel).
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/io.h>

#include <asm/cputype.h>

#include <mach/generic.h>
#include <mach/hardware.h>

struct sa1100_dram_regs {
	int speed;
	u32 mdcnfg;
	u32 mdcas0;
	u32 mdcas1;
	u32 mdcas2;
};


static struct cpufreq_driver sa1100_driver;

static struct sa1100_dram_regs sa1100_dram_settings[] = {
	/*speed,     mdcnfg,     mdcas0,     mdcas1,     mdcas2,   clock freq */
	{ 59000, 0x00dc88a3, 0xcccccccf, 0xfffffffc, 0xffffffff},/*  59.0 MHz */
	{ 73700, 0x011490a3, 0xcccccccf, 0xfffffffc, 0xffffffff},/*  73.7 MHz */
	{ 88500, 0x014e90a3, 0xcccccccf, 0xfffffffc, 0xffffffff},/*  88.5 MHz */
	{103200, 0x01889923, 0xcccccccf, 0xfffffffc, 0xffffffff},/* 103.2 MHz */
	{118000, 0x01c29923, 0x9999998f, 0xfffffff9, 0xffffffff},/* 118.0 MHz */
	{132700, 0x01fb2123, 0x9999998f, 0xfffffff9, 0xffffffff},/* 132.7 MHz */
	{147500, 0x02352123, 0x3333330f, 0xfffffff3, 0xffffffff},/* 147.5 MHz */
	{162200, 0x026b29a3, 0x38e38e1f, 0xfff8e38e, 0xffffffff},/* 162.2 MHz */
	{176900, 0x02a329a3, 0x71c71c1f, 0xfff1c71c, 0xffffffff},/* 176.9 MHz */
	{191700, 0x02dd31a3, 0xe38e383f, 0xffe38e38, 0xffffffff},/* 191.7 MHz */
	{206400, 0x03153223, 0xc71c703f, 0xffc71c71, 0xffffffff},/* 206.4 MHz */
	{221200, 0x034fba23, 0xc71c703f, 0xffc71c71, 0xffffffff},/* 221.2 MHz */
	{235900, 0x03853a23, 0xe1e1e07f, 0xe1e1e1e1, 0xffffffe1},/* 235.9 MHz */
	{250700, 0x03bf3aa3, 0xc3c3c07f, 0xc3c3c3c3, 0xffffffc3},/* 250.7 MHz */
	{265400, 0x03f7c2a3, 0xc3c3c07f, 0xc3c3c3c3, 0xffffffc3},/* 265.4 MHz */
	{280200, 0x0431c2a3, 0x878780ff, 0x87878787, 0xffffff87},/* 280.2 MHz */
	{ 0, 0, 0, 0, 0 } /* last entry */
};

static void sa1100_update_dram_timings(int current_speed, int new_speed)
{
	struct sa1100_dram_regs *settings = sa1100_dram_settings;

	/* find speed */
	while (settings->speed != 0) {
		if (new_speed == settings->speed)
			break;

		settings++;
	}

	if (settings->speed == 0) {
		panic("%s: couldn't find dram setting for speed %d\n",
		      __func__, new_speed);
	}

	/* No risk, no fun: run with interrupts on! */
	if (new_speed > current_speed) {
		/* We're going FASTER, so first relax the memory
		 * timings before changing the core frequency
		 */

		/* Half the memory access clock */
		MDCNFG |= MDCNFG_CDB2;

		/* The order of these statements IS important, keep 8
		 * pulses!!
		 */
		MDCAS2 = settings->mdcas2;
		MDCAS1 = settings->mdcas1;
		MDCAS0 = settings->mdcas0;
		MDCNFG = settings->mdcnfg;
	} else {
		/* We're going SLOWER: first decrease the core
		 * frequency and then tighten the memory settings.
		 */

		/* Half the memory access clock */
		MDCNFG |= MDCNFG_CDB2;

		/* The order of these statements IS important, keep 8
		 * pulses!!
		 */
		MDCAS0 = settings->mdcas0;
		MDCAS1 = settings->mdcas1;
		MDCAS2 = settings->mdcas2;
		MDCNFG = settings->mdcnfg;
	}
}

static int sa1100_target(struct cpufreq_policy *policy, unsigned int ppcr)
{
	unsigned int cur = sa11x0_getspeed(0);
	unsigned int new_freq;

	new_freq = sa11x0_freq_table[ppcr].frequency;

	if (new_freq > cur)
		sa1100_update_dram_timings(cur, new_freq);

	PPCR = ppcr;

	if (new_freq < cur)
		sa1100_update_dram_timings(cur, new_freq);

	return 0;
}

static int __init sa1100_cpu_init(struct cpufreq_policy *policy)
{
	return cpufreq_generic_init(policy, sa11x0_freq_table, CPUFREQ_ETERNAL);
}

static struct cpufreq_driver sa1100_driver __refdata = {
	.flags		= CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= sa1100_target,
	.get		= sa11x0_getspeed,
	.init		= sa1100_cpu_init,
	.name		= "sa1100",
};

static int __init sa1100_dram_init(void)
{
	if (cpu_is_sa1100())
		return cpufreq_register_driver(&sa1100_driver);
	else
		return -ENODEV;
}

arch_initcall(sa1100_dram_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 *  linux/arch/arm/mach-sa1100/cpu-sa1110.c
 *
 *  Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Note: there are two erratas that apply to the SA1110 here:
 *  7 - SDRAM auto-power-up failure (rev A0)
 * 13 - Corruption of internal register reads/writes following
 *      SDRAM reads (rev A0, B0, B1)
 *
 * We ignore rev. A0 and B0 devices; I don't think they're worth supporting.
 *
 * The SDRAM type can be passed on the command line as cpu_sa1110.sdram=type
 */
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>

#include <asm/cputype.h>
#include <asm/mach-types.h>

#include <mach/generic.h>
#include <mach/hardware.h>

#undef DEBUG

struct sdram_params {
	const char name[20];
	u_char  rows;		/* bits				 */
	u_char  cas_latency;	/* cycles			 */
	u_char  tck;		/* clock cycle time (ns)	 */
	u_char  trcd;		/* activate to r/w (ns)		 */
	u_char  trp;		/* precharge to activate (ns)	 */
	u_char  twr;		/* write recovery time (ns)	 */
	u_short refresh;	/* refresh time for array (us)	 */
};

struct sdram_info {
	u_int	mdcnfg;
	u_int	mdrefr;
	u_int	mdcas[3];
};

static struct sdram_params sdram_tbl[] __initdata = {
	{	/* Toshiba TC59SM716 CL2 */
		.name		= "TC59SM716-CL2",
		.rows		= 12,
		.tck		= 10,
		.trcd		= 20,
		.trp		= 20,
		.twr		= 10,
		.refresh	= 64000,
		.cas_latency	= 2,
	}, {	/* Toshiba TC59SM716 CL3 */
		.name		= "TC59SM716-CL3",
		.rows		= 12,
		.tck		= 8,
		.trcd		= 20,
		.trp		= 20,
		.twr		= 8,
		.refresh	= 64000,
		.cas_latency	= 3,
	}, {	/* Samsung K4S641632D TC75 */
		.name		= "K4S641632D",
		.rows		= 14,
		.tck		= 9,
		.trcd		= 27,
		.trp		= 20,
		.twr		= 9,
		.refresh	= 64000,
		.cas_latency	= 3,
	}, {	/* Samsung K4S281632B-1H */
		.name           = "K4S281632B-1H",
		.rows		= 12,
		.tck		= 10,
		.trp		= 20,
		.twr		= 10,
		.refresh	= 64000,
		.cas_latency	= 3,
	}, {	/* Samsung KM416S4030CT */
		.name		= "KM416S4030CT",
		.rows		= 13,
		.tck		= 8,
		.trcd		= 24,	/* 3 CLKs */
		.trp		= 24,	/* 3 CLKs */
		.twr		= 16,	/* Trdl: 2 CLKs */
		.refresh	= 64000,
		.cas_latency	= 3,
	}, {	/* Winbond W982516AH75L CL3 */
		.name		= "W982516AH75L",
		.rows		= 16,
		.tck		= 8,
		.trcd		= 20,
		.trp		= 20,
		.twr		= 8,
		.refresh	= 64000,
		.cas_latency	= 3,
	}, {	/* Micron MT48LC8M16A2TG-75 */
		.name		= "MT48LC8M16A2TG-75",
		.rows		= 12,
		.tck		= 8,
		.trcd		= 20,
		.trp		= 20,
		.twr		= 8,
		.refresh	= 64000,
		.cas_latency	= 3,
	},
};

static struct sdram_params sdram_params;

/*
 * Given a period in ns and frequency in khz, calculate the number of
 * cycles of frequency in period.  Note that we round up to the next
 * cycle, even if we are only slightly over.
 */
static inline u_int ns_to_cycles(u_int ns, u_int khz)
{
	return (ns * khz + 999999) / 1000000;
}

/*
 * Create the MDCAS register bit pattern.
 */
static inline void set_mdcas(u_int *mdcas, int delayed, u_int rcd)
{
	u_int shift;

	rcd = 2 * rcd - 1;
	shift = delayed + 1 + rcd;

	mdcas[0]  = (1 << rcd) - 1;
	mdcas[0] |= 0x55555555 << shift;
	mdcas[1]  = mdcas[2] = 0x55555555 << (shift & 1);
}

static void
sdram_calculate_timing(struct sdram_info *sd, u_int cpu_khz,
		       struct sdram_params *sdram)
{
	u_int mem_khz, sd_khz, trp, twr;

	mem_khz = cpu_khz / 2;
	sd_khz = mem_khz;

	/*
	 * If SDCLK would invalidate the SDRAM timings,
	 * run SDCLK at half speed.
	 *
	 * CPU steppings prior to B2 must either run the memory at
	 * half speed or use delayed read latching (errata 13).
	 */
	if ((ns_to_cycles(sdram->tck, sd_khz) > 1) ||
	    (CPU_REVISION < CPU_SA1110_B2 && sd_khz < 62000))
		sd_khz /= 2;

	sd->mdcnfg = MDCNFG & 0x007f007f;

	twr = ns_to_cycles(sdram->twr, mem_khz);

	/* trp should always be >1 */
	trp = ns_to_cycles(sdram->trp, mem_khz) - 1;
	if (trp < 1)
		trp = 1;

	sd->mdcnfg |= trp << 8;
	sd->mdcnfg |= trp << 24;
	sd->mdcnfg |= sdram->cas_latency << 12;
	sd->mdcnfg |= sdram->cas_latency << 28;
	sd->mdcnfg |= twr << 14;
	sd->mdcnfg |= twr << 30;

	sd->mdrefr = MDREFR & 0xffbffff0;
	sd->mdrefr |= 7;

	if (sd_khz != mem_khz)
		sd->mdrefr |= MDREFR_K1DB2;

	/* initial number of '1's in MDCAS + 1 */
	set_mdcas(sd->mdcas, sd_khz >= 62000,
		ns_to_cycles(sdram->trcd, mem_khz));

#ifdef DEBUG
	printk(KERN_DEBUG "MDCNFG: %08x MDREFR: %08x MDCAS0: %08x MDCAS1: %08x MDCAS2: %08x\n",
		sd->mdcnfg, sd->mdrefr, sd->mdcas[0], sd->mdcas[1],
		sd->mdcas[2]);
#endif
}

/*
 * Set the SDRAM refresh rate.
 */
static inline void sdram_set_refresh(u_int dri)
{
	MDREFR = (MDREFR & 0xffff000f) | (dri << 4);
	(void) MDREFR;
}

/*
 * Update the refresh period.  We do this such that we always refresh
 * the SDRAMs within their permissible period.  The refresh period is
 * always a multiple of the memory clock (fixed at cpu_clock / 2).
 *
 * FIXME: we don't currently take account of burst accesses here,
 * but neither do Intels DM nor Angel.
 */
static void
sdram_update_refresh(u_int cpu_khz, struct sdram_params *sdram)
{
	u_int ns_row = (sdram->refresh * 1000) >> sdram->rows;
	u_int dri = ns_to_cycles(ns_row, cpu_khz / 2) / 32;

#ifdef DEBUG
	mdelay(250);
	printk(KERN_DEBUG "new dri value = %d\n", dri);
#endif

	sdram_set_refresh(dri);
}

/*
 * Ok, set the CPU frequency.
 */
static int sa1110_target(struct cpufreq_policy *policy, unsigned int ppcr)
{
	struct sdram_params *sdram = &sdram_params;
	struct sdram_info sd;
	unsigned long flags;
	unsigned int unused;

	sdram_calculate_timing(&sd, sa11x0_freq_table[ppcr].frequency, sdram);

#if 0
	/*
	 * These values are wrong according to the SA1110 documentation
	 * and errata, but they seem to work.  Need to get a storage
	 * scope on to the SDRAM signals to work out why.
	 */
	if (policy->max < 147500) {
		sd.mdrefr |= MDREFR_K1DB2;
		sd.mdcas[0] = 0xaaaaaa7f;
	} else {
		sd.mdrefr &= ~MDREFR_K1DB2;
		sd.mdcas[0] = 0xaaaaaa9f;
	}
	sd.mdcas[1] = 0xaaaaaaaa;
	sd.mdcas[2] = 0xaaaaaaaa;
#endif

	/*
	 * The clock could be going away for some time.  Set the SDRAMs
	 * to refresh rapidly (every 64 memory clock cycles).  To get
	 * through the whole array, we need to wait 262144 mclk cycles.
	 * We wait 20ms to be safe.
	 */
	sdram_set_refresh(2);
	if (!irqs_disabled())
		msleep(20);
	else
		mdelay(20);

	/*
	 * Reprogram the DRAM timings with interrupts disabled, and
	 * ensure that we are doing this within a complete cache line.
	 * This means that we won't access SDRAM for the duration of
	 * the programming.
	 */
	local_irq_save(flags);
	asm("mcr p15, 0, %0, c7, c10, 4" : : "r" (0));
	udelay(10);
	__asm__ __volatile__("\n\
		b	2f					\n\
		.align	5					\n\
1:		str	%3, [%1, #0]		@ MDCNFG	\n\
		str	%4, [%1, #28]		@ MDREFR	\n\
		str	%5, [%1, #4]		@ MDCAS0	\n\
		str	%6, [%1, #8]		@ MDCAS1	\n\
		str	%7, [%1, #12]		@ MDCAS2	\n\
		str	%8, [%2, #0]		@ PPCR		\n\
		ldr	%0, [%1, #0]				\n\
		b	3f					\n\
2:		b	1b					\n\
3:		nop						\n\
		nop"
		: "=&r" (unused)
		: "r" (&MDCNFG), "r" (&PPCR), "0" (sd.mdcnfg),
		  "r" (sd.mdrefr), "r" (sd.mdcas[0]),
		  "r" (sd.mdcas[1]), "r" (sd.mdcas[2]), "r" (ppcr));
	local_irq_restore(flags);

	/*
	 * Now, return the SDRAM refresh back to normal.
	 */
	sdram_update_refresh(sa11x0_freq_table[ppcr].frequency, sdram);

	return 0;
}

static int __init sa1110_cpu_init(struct cpufreq_policy *policy)
{
	return cpufreq_generic_init(policy, sa11x0_freq_table, CPUFREQ_ETERNAL);
}

/* sa1110_driver needs __refdata because it must remain after init registers
 * it with cpufreq_register_driver() */
static struct cpufreq_driver sa1110_driver __refdata = {
	.flags		= CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= sa1110_target,
	.get		= sa11x0_getspeed,
	.init		= sa1110_cpu_init,
	.name		= "sa1110",
};

static struct sdram_params *sa1110_find_sdram(const char *name)
{
	struct sdram_params *sdram;

	for (sdram = sdram_tbl; sdram < sdram_tbl + ARRAY_SIZE(sdram_tbl);
	     sdram++)
		if (strcmp(name, sdram->name) == 0)
			return sdram;

	return NULL;
}

static char sdram_name[16];

static int __init sa1110_clk_init(void)
{
	struct sdram_params *sdram;
	const char *name = sdram_name;

	if (!cpu_is_sa1110())
		return -ENODEV;

	if (!name[0]) {
		if (machine_is_assabet())
			name = "TC59SM716-CL3";
		if (machine_is_pt_system3())
			name = "K4S641632D";
		if (machine_is_h3100())
			name = "KM416S4030CT";
		if (machine_is_jornada720() || machine_is_h3600())
			name = "K4S281632B-1H";
		if (machine_is_nanoengine())
			name = "MT48LC8M16A2TG-75";
	}

	sdram = sa1110_find_sdram(name);
	if (sdram) {
		printk(KERN_DEBUG "SDRAM: tck: %d trcd: %d trp: %d"
			" twr: %d refresh: %d cas_latency: %d\n",
			sdram->tck, sdram->trcd, sdram->trp,
			sdram->twr, sdram->refresh, sdram->cas_latency);

		memcpy(&sdram_params, sdram, sizeof(sdram_params));

		return cpufreq_register_driver(&sa1110_driver);
	}

	return 0;
}

module_param_string(sdram, sdram_name, sizeof(sdram_name), 0);
arch_initcall(sa1110_clk_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 *	sc520_freq.c: cpufreq driver for the AMD Elan sc520
 *
 *	Copyright (C) 2005 Sean Young <sean@mess.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Based on elanfreq.c
 *
 *	2005-03-30: - initial revision
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/delay.h>
#include <linux/cpufreq.h>
#include <linux/timex.h>
#include <linux/io.h>

#include <asm/cpu_device_id.h>
#include <asm/msr.h>

#define MMCR_BASE	0xfffef000	/* The default base address */
#define OFFS_CPUCTL	0x2   /* CPU Control Register */

static __u8 __iomem *cpuctl;

#define PFX "sc520_freq: "

static struct cpufreq_frequency_table sc520_freq_table[] = {
	{0, 0x01,	100000},
	{0, 0x02,	133000},
	{0, 0,	CPUFREQ_TABLE_END},
};

static unsigned int sc520_freq_get_cpu_frequency(unsigned int cpu)
{
	u8 clockspeed_reg = *cpuctl;

	switch (clockspeed_reg & 0x03) {
	default:
		printk(KERN_ERR PFX "error: cpuctl register has unexpected "
				"value %02x\n", clockspeed_reg);
	case 0x01:
		return 100000;
	case 0x02:
		return 133000;
	}
}

static int sc520_freq_target(struct cpufreq_policy *policy, unsigned int state)
{

	u8 clockspeed_reg;

	local_irq_disable();

	clockspeed_reg = *cpuctl & ~0x03;
	*cpuctl = clockspeed_reg | sc520_freq_table[state].driver_data;

	local_irq_enable();

	return 0;
}

/*
 *	Module init and exit code
 */

static int sc520_freq_cpu_init(struct cpufreq_policy *policy)
{
	struct cpuinfo_x86 *c = &cpu_data(0);

	/* capability check */
	if (c->x86_vendor != X86_VENDOR_AMD ||
	    c->x86 != 4 || c->x86_model != 9)
		return -ENODEV;

	/* cpuinfo and default policy values */
	policy->cpuinfo.transition_latency = 1000000; /* 1ms */

	return cpufreq_table_validate_and_show(policy, sc520_freq_table);
}


static struct cpufreq_driver sc520_freq_driver = {
	.get	= sc520_freq_get_cpu_frequency,
	.verify	= cpufreq_generic_frequency_table_verify,
	.target_index = sc520_freq_target,
	.init	= sc520_freq_cpu_init,
	.name	= "sc520_freq",
	.attr	= cpufreq_generic_attr,
};

static const struct x86_cpu_id sc520_ids[] = {
	{ X86_VENDOR_AMD, 4, 9 },
	{}
};
MODULE_DEVICE_TABLE(x86cpu, sc520_ids);

static int __init sc520_freq_init(void)
{
	int err;

	if (!x86_match_cpu(sc520_ids))
		return -ENODEV;

	cpuctl = ioremap((unsigned long)(MMCR_BASE + OFFS_CPUCTL), 1);
	if (!cpuctl) {
		printk(KERN_ERR "sc520_freq: error: failed to remap memory\n");
		return -ENOMEM;
	}

	err = cpufreq_register_driver(&sc520_freq_driver);
	if (err)
		iounmap(cpuctl);

	return err;
}


static void __exit sc520_freq_exit(void)
{
	cpufreq_unregister_driver(&sc520_freq_driver);
	iounmap(cpuctl);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sean Young <sean@mess.org>");
MODULE_DESCRIPTION("cpufreq driver for AMD's Elan sc520 CPU");

module_init(sc520_freq_init);
module_exit(sc520_freq_exit);

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * System Control and Power Interface (SCPI) based CPUFreq Interface driver
 *
 * It provides necessary ops to arm_big_little cpufreq driver.
 *
 * Copyright (C) 2015 ARM Ltd.
 * Sudeep Holla <sudeep.holla@arm.com>
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

#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/scpi_protocol.h>
#include <linux/types.h>

#include "arm_big_little.h"

static struct scpi_ops *scpi_ops;

static struct scpi_dvfs_info *scpi_get_dvfs_info(struct device *cpu_dev)
{
	int domain = topology_physical_package_id(cpu_dev->id);

	if (domain < 0)
		return ERR_PTR(-EINVAL);
	return scpi_ops->dvfs_get_info(domain);
}

static int scpi_opp_table_ops(struct device *cpu_dev, bool remove)
{
	int idx, ret = 0;
	struct scpi_opp *opp;
	struct scpi_dvfs_info *info = scpi_get_dvfs_info(cpu_dev);

	if (IS_ERR(info))
		return PTR_ERR(info);

	if (!info->opps)
		return -EIO;

	for (opp = info->opps, idx = 0; idx < info->count; idx++, opp++) {
		if (remove)
			dev_pm_opp_remove(cpu_dev, opp->freq);
		else
			ret = dev_pm_opp_add(cpu_dev, opp->freq,
					     opp->m_volt * 1000);
		if (ret) {
			dev_warn(cpu_dev, "failed to add opp %uHz %umV\n",
				 opp->freq, opp->m_volt);
			while (idx-- > 0)
				dev_pm_opp_remove(cpu_dev, (--opp)->freq);
			return ret;
		}
	}
	return ret;
}

static int scpi_get_transition_latency(struct device *cpu_dev)
{
	struct scpi_dvfs_info *info = scpi_get_dvfs_info(cpu_dev);

	if (IS_ERR(info))
		return PTR_ERR(info);
	return info->latency;
}

static int scpi_init_opp_table(struct device *cpu_dev)
{
	return scpi_opp_table_ops(cpu_dev, false);
}

static void scpi_free_opp_table(struct device *cpu_dev)
{
	scpi_opp_table_ops(cpu_dev, true);
}

static struct cpufreq_arm_bL_ops scpi_cpufreq_ops = {
	.name	= "scpi",
	.get_transition_latency = scpi_get_transition_latency,
	.init_opp_table = scpi_init_opp_table,
	.free_opp_table = scpi_free_opp_table,
};

static int scpi_cpufreq_probe(struct platform_device *pdev)
{
	scpi_ops = get_scpi_ops();
	if (!scpi_ops)
		return -EIO;

	return bL_cpufreq_register(&scpi_cpufreq_ops);
}

static int scpi_cpufreq_remove(struct platform_device *pdev)
{
	bL_cpufreq_unregister(&scpi_cpufreq_ops);
	scpi_ops = NULL;
	return 0;
}

static struct platform_driver scpi_cpufreq_platdrv = {
	.driver = {
		.name	= "scpi-cpufreq",
		.owner	= THIS_MODULE,
	},
	.probe		= scpi_cpufreq_probe,
	.remove		= scpi_cpufreq_remove,
};
module_platform_driver(scpi_cpufreq_platdrv);

MODULE_ALIAS("platform:scpi-cpufreq");
MODULE_AUTHOR("Sudeep Holla <sudeep.holla@arm.com>");
MODULE_DESCRIPTION("ARM SCPI CPUFreq interface driver");
MODULE_LICENSE("GPL v2");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 *  SFI Performance States Driver
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  Author: Vishwesh M Rudramuni <vishwesh.m.rudramuni@intel.com>
 *  Author: Srinidhi Kasagar <srinidhi.kasagar@intel.com>
 */

#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sfi.h>
#include <linux/slab.h>
#include <linux/smp.h>

#include <asm/msr.h>

struct cpufreq_frequency_table *freq_table;
static struct sfi_freq_table_entry *sfi_cpufreq_array;
static int num_freq_table_entries;

static int sfi_parse_freq(struct sfi_table_header *table)
{
	struct sfi_table_simple *sb;
	struct sfi_freq_table_entry *pentry;
	int totallen;

	sb = (struct sfi_table_simple *)table;
	num_freq_table_entries = SFI_GET_NUM_ENTRIES(sb,
			struct sfi_freq_table_entry);
	if (num_freq_table_entries <= 1) {
		pr_err("No p-states discovered\n");
		return -ENODEV;
	}

	pentry = (struct sfi_freq_table_entry *)sb->pentry;
	totallen = num_freq_table_entries * sizeof(*pentry);

	sfi_cpufreq_array = kmemdup(pentry, totallen, GFP_KERNEL);
	if (!sfi_cpufreq_array)
		return -ENOMEM;

	return 0;
}

static int sfi_cpufreq_target(struct cpufreq_policy *policy, unsigned int index)
{
	unsigned int next_perf_state = 0; /* Index into perf table */
	u32 lo, hi;

	next_perf_state = policy->freq_table[index].driver_data;

	rdmsr_on_cpu(policy->cpu, MSR_IA32_PERF_CTL, &lo, &hi);
	lo = (lo & ~INTEL_PERF_CTL_MASK) |
		((u32) sfi_cpufreq_array[next_perf_state].ctrl_val &
		INTEL_PERF_CTL_MASK);
	wrmsr_on_cpu(policy->cpu, MSR_IA32_PERF_CTL, lo, hi);

	return 0;
}

static int sfi_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	policy->shared_type = CPUFREQ_SHARED_TYPE_HW;
	policy->cpuinfo.transition_latency = 100000;	/* 100us */

	return cpufreq_table_validate_and_show(policy, freq_table);
}

static struct cpufreq_driver sfi_cpufreq_driver = {
	.flags		= CPUFREQ_CONST_LOOPS,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= sfi_cpufreq_target,
	.init		= sfi_cpufreq_cpu_init,
	.name		= "sfi-cpufreq",
	.attr		= cpufreq_generic_attr,
};

static int __init sfi_cpufreq_init(void)
{
	int ret, i;

	/* parse the freq table from SFI */
	ret = sfi_table_parse(SFI_SIG_FREQ, NULL, NULL, sfi_parse_freq);
	if (ret)
		return ret;

	freq_table = kzalloc(sizeof(*freq_table) *
			(num_freq_table_entries + 1), GFP_KERNEL);
	if (!freq_table) {
		ret = -ENOMEM;
		goto err_free_array;
	}

	for (i = 0; i < num_freq_table_entries; i++) {
		freq_table[i].driver_data = i;
		freq_table[i].frequency = sfi_cpufreq_array[i].freq_mhz * 1000;
	}
	freq_table[i].frequency = CPUFREQ_TABLE_END;

	ret = cpufreq_register_driver(&sfi_cpufreq_driver);
	if (ret)
		goto err_free_tbl;

	return ret;

err_free_tbl:
	kfree(freq_table);
err_free_array:
	kfree(sfi_cpufreq_array);
	return ret;
}
late_initcall(sfi_cpufreq_init);

static void __exit sfi_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&sfi_cpufreq_driver);
	kfree(freq_table);
	kfree(sfi_cpufreq_array);
}
module_exit(sfi_cpufreq_exit);

MODULE_AUTHOR("Vishwesh M Rudramuni <vishwesh.m.rudramuni@intel.com>");
MODULE_DESCRIPTION("SFI Performance-States Driver");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * cpufreq driver for the SuperH processors.
 *
 * Copyright (C) 2002 - 2012 Paul Mundt
 * Copyright (C) 2002 M. R. Brown
 *
 * Clock framework bits from arch/avr32/mach-at32ap/cpufreq.c
 *
 *   Copyright (C) 2004-2007 Atmel Corporation
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#define pr_fmt(fmt) "cpufreq: " fmt

#include <linux/types.h>
#include <linux/cpufreq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/sched.h>	/* set_cpus_allowed() */
#include <linux/clk.h>
#include <linux/percpu.h>
#include <linux/sh_clk.h>

static DEFINE_PER_CPU(struct clk, sh_cpuclk);

struct cpufreq_target {
	struct cpufreq_policy	*policy;
	unsigned int		freq;
};

static unsigned int sh_cpufreq_get(unsigned int cpu)
{
	return (clk_get_rate(&per_cpu(sh_cpuclk, cpu)) + 500) / 1000;
}

static long __sh_cpufreq_target(void *arg)
{
	struct cpufreq_target *target = arg;
	struct cpufreq_policy *policy = target->policy;
	int cpu = policy->cpu;
	struct clk *cpuclk = &per_cpu(sh_cpuclk, cpu);
	struct cpufreq_freqs freqs;
	struct device *dev;
	long freq;

	if (smp_processor_id() != cpu)
		return -ENODEV;

	dev = get_cpu_device(cpu);

	/* Convert target_freq from kHz to Hz */
	freq = clk_round_rate(cpuclk, target->freq * 1000);

	if (freq < (policy->min * 1000) || freq > (policy->max * 1000))
		return -EINVAL;

	dev_dbg(dev, "requested frequency %u Hz\n", target->freq * 1000);

	freqs.old	= sh_cpufreq_get(cpu);
	freqs.new	= (freq + 500) / 1000;
	freqs.flags	= 0;

	cpufreq_freq_transition_begin(target->policy, &freqs);
	clk_set_rate(cpuclk, freq);
	cpufreq_freq_transition_end(target->policy, &freqs, 0);

	dev_dbg(dev, "set frequency %lu Hz\n", freq);
	return 0;
}

/*
 * Here we notify other drivers of the proposed change and the final change.
 */
static int sh_cpufreq_target(struct cpufreq_policy *policy,
			     unsigned int target_freq,
			     unsigned int relation)
{
	struct cpufreq_target data = { .policy = policy, .freq = target_freq };

	return work_on_cpu(policy->cpu, __sh_cpufreq_target, &data);
}

static int sh_cpufreq_verify(struct cpufreq_policy *policy)
{
	struct clk *cpuclk = &per_cpu(sh_cpuclk, policy->cpu);
	struct cpufreq_frequency_table *freq_table;

	freq_table = cpuclk->nr_freqs ? cpuclk->freq_table : NULL;
	if (freq_table)
		return cpufreq_frequency_table_verify(policy, freq_table);

	cpufreq_verify_within_cpu_limits(policy);

	policy->min = (clk_round_rate(cpuclk, 1) + 500) / 1000;
	policy->max = (clk_round_rate(cpuclk, ~0UL) + 500) / 1000;

	cpufreq_verify_within_cpu_limits(policy);
	return 0;
}

static int sh_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	unsigned int cpu = policy->cpu;
	struct clk *cpuclk = &per_cpu(sh_cpuclk, cpu);
	struct cpufreq_frequency_table *freq_table;
	struct device *dev;

	dev = get_cpu_device(cpu);

	cpuclk = clk_get(dev, "cpu_clk");
	if (IS_ERR(cpuclk)) {
		dev_err(dev, "couldn't get CPU clk\n");
		return PTR_ERR(cpuclk);
	}

	freq_table = cpuclk->nr_freqs ? cpuclk->freq_table : NULL;
	if (freq_table) {
		int result;

		result = cpufreq_table_validate_and_show(policy, freq_table);
		if (result)
			return result;
	} else {
		dev_notice(dev, "no frequency table found, falling back "
			   "to rate rounding.\n");

		policy->min = policy->cpuinfo.min_freq =
			(clk_round_rate(cpuclk, 1) + 500) / 1000;
		policy->max = policy->cpuinfo.max_freq =
			(clk_round_rate(cpuclk, ~0UL) + 500) / 1000;
	}

	policy->cpuinfo.transition_latency = CPUFREQ_ETERNAL;

	dev_info(dev, "CPU Frequencies - Minimum %u.%03u MHz, "
	       "Maximum %u.%03u MHz.\n",
	       policy->min / 1000, policy->min % 1000,
	       policy->max / 1000, policy->max % 1000);

	return 0;
}

static int sh_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	unsigned int cpu = policy->cpu;
	struct clk *cpuclk = &per_cpu(sh_cpuclk, cpu);

	clk_put(cpuclk);

	return 0;
}

static struct cpufreq_driver sh_cpufreq_driver = {
	.name		= "sh",
	.get		= sh_cpufreq_get,
	.target		= sh_cpufreq_target,
	.verify		= sh_cpufreq_verify,
	.init		= sh_cpufreq_cpu_init,
	.exit		= sh_cpufreq_cpu_exit,
	.attr		= cpufreq_generic_attr,
};

static int __init sh_cpufreq_module_init(void)
{
	pr_notice("SuperH CPU frequency driver.\n");
	return cpufreq_register_driver(&sh_cpufreq_driver);
}

static void __exit sh_cpufreq_module_exit(void)
{
	cpufreq_unregister_driver(&sh_cpufreq_driver);
}

module_init(sh_cpufreq_module_init);
module_exit(sh_cpufreq_module_exit);

MODULE_AUTHOR("Paul Mundt <lethal@linux-sh.org>");
MODULE_DESCRIPTION("cpufreq driver for SuperH");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /* us2e_cpufreq.c: UltraSPARC-IIe cpu frequency support
 *
 * Copyright (C) 2003 David S. Miller (davem@redhat.com)
 *
 * Many thanks to Dominik Brodowski for fixing up the cpufreq
 * infrastructure in order to make this driver easier to implement.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/cpufreq.h>
#include <linux/threads.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>

#include <asm/asi.h>
#include <asm/timer.h>

static struct cpufreq_driver *cpufreq_us2e_driver;

struct us2e_freq_percpu_info {
	struct cpufreq_frequency_table table[6];
};

/* Indexed by cpu number. */
static struct us2e_freq_percpu_info *us2e_freq_table;

#define HBIRD_MEM_CNTL0_ADDR	0x1fe0000f010UL
#define HBIRD_ESTAR_MODE_ADDR	0x1fe0000f080UL

/* UltraSPARC-IIe has five dividers: 1, 2, 4, 6, and 8.  These are controlled
 * in the ESTAR mode control register.
 */
#define ESTAR_MODE_DIV_1	0x0000000000000000UL
#define ESTAR_MODE_DIV_2	0x0000000000000001UL
#define ESTAR_MODE_DIV_4	0x0000000000000003UL
#define ESTAR_MODE_DIV_6	0x0000000000000002UL
#define ESTAR_MODE_DIV_8	0x0000000000000004UL
#define ESTAR_MODE_DIV_MASK	0x0000000000000007UL

#define MCTRL0_SREFRESH_ENAB	0x0000000000010000UL
#define MCTRL0_REFR_COUNT_MASK	0x0000000000007f00UL
#define MCTRL0_REFR_COUNT_SHIFT	8
#define MCTRL0_REFR_INTERVAL	7800
#define MCTRL0_REFR_CLKS_P_CNT	64

static unsigned long read_hbreg(unsigned long addr)
{
	unsigned long ret;

	__asm__ __volatile__("ldxa	[%1] %2, %0"
			     : "=&r" (ret)
			     : "r" (addr), "i" (ASI_PHYS_BYPASS_EC_E));
	return ret;
}

static void write_hbreg(unsigned long addr, unsigned long val)
{
	__asm__ __volatile__("stxa	%0, [%1] %2\n\t"
			     "membar	#Sync"
			     : /* no outputs */
			     : "r" (val), "r" (addr), "i" (ASI_PHYS_BYPASS_EC_E)
			     : "memory");
	if (addr == HBIRD_ESTAR_MODE_ADDR) {
		/* Need to wait 16 clock cycles for the PLL to lock.  */
		udelay(1);
	}
}

static void self_refresh_ctl(int enable)
{
	unsigned long mctrl = read_hbreg(HBIRD_MEM_CNTL0_ADDR);

	if (enable)
		mctrl |= MCTRL0_SREFRESH_ENAB;
	else
		mctrl &= ~MCTRL0_SREFRESH_ENAB;
	write_hbreg(HBIRD_MEM_CNTL0_ADDR, mctrl);
	(void) read_hbreg(HBIRD_MEM_CNTL0_ADDR);
}

static void frob_mem_refresh(int cpu_slowing_down,
			     unsigned long clock_tick,
			     unsigned long old_divisor, unsigned long divisor)
{
	unsigned long old_refr_count, refr_count, mctrl;

	refr_count  = (clock_tick * MCTRL0_REFR_INTERVAL);
	refr_count /= (MCTRL0_REFR_CLKS_P_CNT * divisor * 1000000000UL);

	mctrl = read_hbreg(HBIRD_MEM_CNTL0_ADDR);
	old_refr_count = (mctrl & MCTRL0_REFR_COUNT_MASK)
		>> MCTRL0_REFR_COUNT_SHIFT;

	mctrl &= ~MCTRL0_REFR_COUNT_MASK;
	mctrl |= refr_count << MCTRL0_REFR_COUNT_SHIFT;
	write_hbreg(HBIRD_MEM_CNTL0_ADDR, mctrl);
	mctrl = read_hbreg(HBIRD_MEM_CNTL0_ADDR);

	if (cpu_slowing_down && !(mctrl & MCTRL0_SREFRESH_ENAB)) {
		unsigned long usecs;

		/* We have to wait for both refresh counts (old
		 * and new) to go to zero.
		 */
		usecs = (MCTRL0_REFR_CLKS_P_CNT *
			 (refr_count + old_refr_count) *
			 1000000UL *
			 old_divisor) / clock_tick;
		udelay(usecs + 1UL);
	}
}

static void us2e_transition(unsigned long estar, unsigned long new_bits,
			    unsigned long clock_tick,
			    unsigned long old_divisor, unsigned long divisor)
{
	unsigned long flags;

	local_irq_save(flags);

	estar &= ~ESTAR_MODE_DIV_MASK;

	/* This is based upon the state transition diagram in the IIe manual.  */
	if (old_divisor == 2 && divisor == 1) {
		self_refresh_ctl(0);
		write_hbreg(HBIRD_ESTAR_MODE_ADDR, estar | new_bits);
		frob_mem_refresh(0, clock_tick, old_divisor, divisor);
	} else if (old_divisor == 1 && divisor == 2) {
		frob_mem_refresh(1, clock_tick, old_divisor, divisor);
		write_hbreg(HBIRD_ESTAR_MODE_ADDR, estar | new_bits);
		self_refresh_ctl(1);
	} else if (old_divisor == 1 && divisor > 2) {
		us2e_transition(estar, ESTAR_MODE_DIV_2, clock_tick,
				1, 2);
		us2e_transition(estar, new_bits, clock_tick,
				2, divisor);
	} else if (old_divisor > 2 && divisor == 1) {
		us2e_transition(estar, ESTAR_MODE_DIV_2, clock_tick,
				old_divisor, 2);
		us2e_transition(estar, new_bits, clock_tick,
				2, divisor);
	} else if (old_divisor < divisor) {
		frob_mem_refresh(0, clock_tick, old_divisor, divisor);
		write_hbreg(HBIRD_ESTAR_MODE_ADDR, estar | new_bits);
	} else if (old_divisor > divisor) {
		write_hbreg(HBIRD_ESTAR_MODE_ADDR, estar | new_bits);
		frob_mem_refresh(1, clock_tick, old_divisor, divisor);
	} else {
		BUG();
	}

	local_irq_restore(flags);
}

static unsigned long index_to_estar_mode(unsigned int index)
{
	switch (index) {
	case 0:
		return ESTAR_MODE_DIV_1;

	case 1:
		return ESTAR_MODE_DIV_2;

	case 2:
		return ESTAR_MODE_DIV_4;

	case 3:
		return ESTAR_MODE_DIV_6;

	case 4:
		return ESTAR_MODE_DIV_8;

	default:
		BUG();
	}
}

static unsigned long index_to_divisor(unsigned int index)
{
	switch (index) {
	case 0:
		return 1;

	case 1:
		return 2;

	case 2:
		return 4;

	case 3:
		return 6;

	case 4:
		return 8;

	default:
		BUG();
	}
}

static unsigned long estar_to_divisor(unsigned long estar)
{
	unsigned long ret;

	switch (estar & ESTAR_MODE_DIV_MASK) {
	case ESTAR_MODE_DIV_1:
		ret = 1;
		break;
	case ESTAR_MODE_DIV_2:
		ret = 2;
		break;
	case ESTAR_MODE_DIV_4:
		ret = 4;
		break;
	case ESTAR_MODE_DIV_6:
		ret = 6;
		break;
	case ESTAR_MODE_DIV_8:
		ret = 8;
		break;
	default:
		BUG();
	}

	return ret;
}

static unsigned int us2e_freq_get(unsigned int cpu)
{
	cpumask_t cpus_allowed;
	unsigned long clock_tick, estar;

	cpumask_copy(&cpus_allowed, tsk_cpus_allowed(current));
	set_cpus_allowed_ptr(current, cpumask_of(cpu));

	clock_tick = sparc64_get_clock_tick(cpu) / 1000;
	estar = read_hbreg(HBIRD_ESTAR_MODE_ADDR);

	set_cpus_allowed_ptr(current, &cpus_allowed);

	return clock_tick / estar_to_divisor(estar);
}

static int us2e_freq_target(struct cpufreq_policy *policy, unsigned int index)
{
	unsigned int cpu = policy->cpu;
	unsigned long new_bits, new_freq;
	unsigned long clock_tick, divisor, old_divisor, estar;
	cpumask_t cpus_allowed;

	cpumask_copy(&cpus_allowed, tsk_cpus_allowed(current));
	set_cpus_allowed_ptr(current, cpumask_of(cpu));

	new_freq = clock_tick = sparc64_get_clock_tick(cpu) / 1000;
	new_bits = index_to_estar_mode(index);
	divisor = index_to_divisor(index);
	new_freq /= divisor;

	estar = read_hbreg(HBIRD_ESTAR_MODE_ADDR);

	old_divisor = estar_to_divisor(estar);

	if (old_divisor != divisor)
		us2e_transition(estar, new_bits, clock_tick * 1000,
				old_divisor, divisor);

	set_cpus_allowed_ptr(current, &cpus_allowed);

	return 0;
}

static int __init us2e_freq_cpu_init(struct cpufreq_policy *policy)
{
	unsigned int cpu = policy->cpu;
	unsigned long clock_tick = sparc64_get_clock_tick(cpu) / 1000;
	struct cpufreq_frequency_table *table =
		&us2e_freq_table[cpu].table[0];

	table[0].driver_data = 0;
	table[0].frequency = clock_tick / 1;
	table[1].driver_data = 1;
	table[1].frequency = clock_tick / 2;
	table[2].driver_data = 2;
	table[2].frequency = clock_tick / 4;
	table[2].driver_data = 3;
	table[2].frequency = clock_tick / 6;
	table[2].driver_data = 4;
	table[2].frequency = clock_tick / 8;
	table[2].driver_data = 5;
	table[3].frequency = CPUFREQ_TABLE_END;

	policy->cpuinfo.transition_latency = 0;
	policy->cur = clock_tick;

	return cpufreq_table_validate_and_show(policy, table);
}

static int us2e_freq_cpu_exit(struct cpufreq_policy *policy)
{
	if (cpufreq_us2e_driver)
		us2e_freq_target(policy, 0);

	return 0;
}

static int __init us2e_freq_init(void)
{
	unsigned long manuf, impl, ver;
	int ret;

	if (tlb_type != spitfire)
		return -ENODEV;

	__asm__("rdpr %%ver, %0" : "=r" (ver));
	manuf = ((ver >> 48) & 0xffff);
	impl  = ((ver >> 32) & 0xffff);

	if (manuf == 0x17 && impl == 0x13) {
		struct cpufreq_driver *driver;

		ret = -ENOMEM;
		driver = kzalloc(sizeof(*driver), GFP_KERNEL);
		if (!driver)
			goto err_out;

		us2e_freq_table = kzalloc((NR_CPUS * sizeof(*us2e_freq_table)),
			GFP_KERNEL);
		if (!us2e_freq_table)
			goto err_out;

		driver->init = us2e_freq_cpu_init;
		driver->verify = cpufreq_generic_frequency_table_verify;
		driver->target_index = us2e_freq_target;
		driver->get = us2e_freq_get;
		driver->exit = us2e_freq_cpu_exit;
		strcpy(driver->name, "UltraSPARC-IIe");

		cpufreq_us2e_driver = driver;
		ret = cpufreq_register_driver(driver);
		if (ret)
			goto err_out;

		return 0;

err_out:
		if (driver) {
			kfree(driver);
			cpufreq_us2e_driver = NULL;
		}
		kfree(us2e_freq_table);
		us2e_freq_table = NULL;
		return ret;
	}

	return -ENODEV;
}

static void __exit us2e_freq_exit(void)
{
	if (cpufreq_us2e_driver) {
		cpufreq_unregister_driver(cpufreq_us2e_driver);
		kfree(cpufreq_us2e_driver);
		cpufreq_us2e_driver = NULL;
		kfree(us2e_freq_table);
		us2e_freq_table = NULL;
	}
}

MODULE_AUTHOR("David S. Miller <davem@redhat.com>");
MODULE_DESCRIPTION("cpufreq driver for UltraSPARC-IIe");
MODULE_LICENSE("GPL");

module_init(us2e_freq_init);
module_exit(us2e_freq_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /* us3_cpufreq.c: UltraSPARC-III cpu frequency support
 *
 * Copyright (C) 2003 David S. Miller (davem@redhat.com)
 *
 * Many thanks to Dominik Brodowski for fixing up the cpufreq
 * infrastructure in order to make this driver easier to implement.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/cpufreq.h>
#include <linux/threads.h>
#include <linux/slab.h>
#include <linux/init.h>

#include <asm/head.h>
#include <asm/timer.h>

static struct cpufreq_driver *cpufreq_us3_driver;

struct us3_freq_percpu_info {
	struct cpufreq_frequency_table table[4];
};

/* Indexed by cpu number. */
static struct us3_freq_percpu_info *us3_freq_table;

/* UltraSPARC-III has three dividers: 1, 2, and 32.  These are controlled
 * in the Safari config register.
 */
#define SAFARI_CFG_DIV_1	0x0000000000000000UL
#define SAFARI_CFG_DIV_2	0x0000000040000000UL
#define SAFARI_CFG_DIV_32	0x0000000080000000UL
#define SAFARI_CFG_DIV_MASK	0x00000000C0000000UL

static unsigned long read_safari_cfg(void)
{
	unsigned long ret;

	__asm__ __volatile__("ldxa	[%%g0] %1, %0"
			     : "=&r" (ret)
			     : "i" (ASI_SAFARI_CONFIG));
	return ret;
}

static void write_safari_cfg(unsigned long val)
{
	__asm__ __volatile__("stxa	%0, [%%g0] %1\n\t"
			     "membar	#Sync"
			     : /* no outputs */
			     : "r" (val), "i" (ASI_SAFARI_CONFIG)
			     : "memory");
}

static unsigned long get_current_freq(unsigned int cpu, unsigned long safari_cfg)
{
	unsigned long clock_tick = sparc64_get_clock_tick(cpu) / 1000;
	unsigned long ret;

	switch (safari_cfg & SAFARI_CFG_DIV_MASK) {
	case SAFARI_CFG_DIV_1:
		ret = clock_tick / 1;
		break;
	case SAFARI_CFG_DIV_2:
		ret = clock_tick / 2;
		break;
	case SAFARI_CFG_DIV_32:
		ret = clock_tick / 32;
		break;
	default:
		BUG();
	}

	return ret;
}

static unsigned int us3_freq_get(unsigned int cpu)
{
	cpumask_t cpus_allowed;
	unsigned long reg;
	unsigned int ret;

	cpumask_copy(&cpus_allowed, tsk_cpus_allowed(current));
	set_cpus_allowed_ptr(current, cpumask_of(cpu));

	reg = read_safari_cfg();
	ret = get_current_freq(cpu, reg);

	set_cpus_allowed_ptr(current, &cpus_allowed);

	return ret;
}

static int us3_freq_target(struct cpufreq_policy *policy, unsigned int index)
{
	unsigned int cpu = policy->cpu;
	unsigned long new_bits, new_freq, reg;
	cpumask_t cpus_allowed;

	cpumask_copy(&cpus_allowed, tsk_cpus_allowed(current));
	set_cpus_allowed_ptr(current, cpumask_of(cpu));

	new_freq = sparc64_get_clock_tick(cpu) / 1000;
	switch (index) {
	case 0:
		new_bits = SAFARI_CFG_DIV_1;
		new_freq /= 1;
		break;
	case 1:
		new_bits = SAFARI_CFG_DIV_2;
		new_freq /= 2;
		break;
	case 2:
		new_bits = SAFARI_CFG_DIV_32;
		new_freq /= 32;
		break;

	default:
		BUG();
	}

	reg = read_safari_cfg();

	reg &= ~SAFARI_CFG_DIV_MASK;
	reg |= new_bits;
	write_safari_cfg(reg);

	set_cpus_allowed_ptr(current, &cpus_allowed);

	return 0;
}

static int __init us3_freq_cpu_init(struct cpufreq_policy *policy)
{
	unsigned int cpu = policy->cpu;
	unsigned long clock_tick = sparc64_get_clock_tick(cpu) / 1000;
	struct cpufreq_frequency_table *table =
		&us3_freq_table[cpu].table[0];

	table[0].driver_data = 0;
	table[0].frequency = clock_tick / 1;
	table[1].driver_data = 1;
	table[1].frequency = clock_tick / 2;
	table[2].driver_data = 2;
	table[2].frequency = clock_tick / 32;
	table[3].driver_data = 0;
	table[3].frequency = CPUFREQ_TABLE_END;

	policy->cpuinfo.transition_latency = 0;
	policy->cur = clock_tick;

	return cpufreq_table_validate_and_show(policy, table);
}

static int us3_freq_cpu_exit(struct cpufreq_policy *policy)
{
	if (cpufreq_us3_driver)
		us3_freq_target(policy, 0);

	return 0;
}

static int __init us3_freq_init(void)
{
	unsigned long manuf, impl, ver;
	int ret;

	if (tlb_type != cheetah && tlb_type != cheetah_plus)
		return -ENODEV;

	__asm__("rdpr %%ver, %0" : "=r" (ver));
	manuf = ((ver >> 48) & 0xffff);
	impl  = ((ver >> 32) & 0xffff);

	if (manuf == CHEETAH_MANUF &&
	    (impl == CHEETAH_IMPL ||
	     impl == CHEETAH_PLUS_IMPL ||
	     impl == JAGUAR_IMPL ||
	     impl == PANTHER_IMPL)) {
		struct cpufreq_driver *driver;

		ret = -ENOMEM;
		driver = kzalloc(sizeof(*driver), GFP_KERNEL);
		if (!driver)
			goto err_out;

		us3_freq_table = kzalloc((NR_CPUS * sizeof(*us3_freq_table)),
			GFP_KERNEL);
		if (!us3_freq_table)
			goto err_out;

		driver->init = us3_freq_cpu_init;
		driver->verify = cpufreq_generic_frequency_table_verify;
		driver->target_index = us3_freq_target;
		driver->get = us3_freq_get;
		driver->exit = us3_freq_cpu_exit;
		strcpy(driver->name, "UltraSPARC-III");

		cpufreq_us3_driver = driver;
		ret = cpufreq_register_driver(driver);
		if (ret)
			goto err_out;

		return 0;

err_out:
		if (driver) {
			kfree(driver);
			cpufreq_us3_driver = NULL;
		}
		kfree(us3_freq_table);
		us3_freq_table = NULL;
		return ret;
	}

	return -ENODEV;
}

static void __exit us3_freq_exit(void)
{
	if (cpufreq_us3_driver) {
		cpufreq_unregister_driver(cpufreq_us3_driver);
		kfree(cpufreq_us3_driver);
		cpufreq_us3_driver = NULL;
		kfree(us3_freq_table);
		us3_freq_table = NULL;
	}
}

MODULE_AUTHOR("David S. Miller <davem@redhat.com>");
MODULE_DESCRIPTION("cpufreq driver for UltraSPARC-III");
MODULE_LICENSE("GPL");

module_init(us3_freq_init);
module_exit(us3_freq_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * drivers/cpufreq/spear-cpufreq.c
 *
 * CPU Frequency Scaling for SPEAr platform
 *
 * Copyright (C) 2012 ST Microelectronics
 * Deepak Sikri <deepak.sikri@st.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/clk.h>
#include <linux/cpufreq.h>
#include