/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2015 ARM Limited
 */

#define pr_fmt(fmt) "psci: " fmt

#include <linux/arm-smccc.h>
#include <linux/errno.h>
#include <linux/linkage.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/printk.h>
#include <linux/psci.h>
#include <linux/reboot.h>
#include <linux/suspend.h>

#include <uapi/linux/psci.h>

#include <asm/cputype.h>
#include <asm/system_misc.h>
#include <asm/smp_plat.h>
#include <asm/suspend.h>

/*
 * While a 64-bit OS can make calls with SMC32 calling conventions, for some
 * calls it is necessary to use SMC64 to pass or return 64-bit values.
 * For such calls PSCI_FN_NATIVE(version, name) will choose the appropriate
 * (native-width) function ID.
 */
#ifdef CONFIG_64BIT
#define PSCI_FN_NATIVE(version, name)	PSCI_##version##_FN64_##name
#else
#define PSCI_FN_NATIVE(version, name)	PSCI_##version##_FN_##name
#endif

/*
 * The CPU any Trusted OS is resident on. The trusted OS may reject CPU_OFF
 * calls to its resident CPU, so we must avoid issuing those. We never migrate
 * a Trusted OS even if it claims to be capable of migration -- doing so will
 * require cooperation with a Trusted OS driver.
 */
static int resident_cpu = -1;

bool psci_tos_resident_on(int cpu)
{
	return cpu == resident_cpu;
}

struct psci_operations psci_ops = {
	.conduit = PSCI_CONDUIT_NONE,
	.smccc_version = SMCCC_VERSION_1_0,
};

enum arm_smccc_conduit arm_smccc_1_1_get_conduit(void)
{
	if (psci_ops.smccc_version < SMCCC_VERSION_1_1)
		return SMCCC_CONDUIT_NONE;

	switch (psci_ops.conduit) {
	case PSCI_CONDUIT_SMC:
		return SMCCC_CONDUIT_SMC;
	case PSCI_CONDUIT_HVC:
		return SMCCC_CONDUIT_HVC;
	default:
		return SMCCC_CONDUIT_NONE;
	}
}

typedef unsigned long (psci_fn)(unsigned long, unsigned long,
				unsigned long, unsigned long);
static psci_fn *invoke_psci_fn;

enum psci_function {
	PSCI_FN_CPU_SUSPEND,
	PSCI_FN_CPU_ON,
	PSCI_FN_CPU_OFF,
	PSCI_FN_MIGRATE,
	PSCI_FN_MAX,
};

static u32 psci_function_id[PSCI_FN_MAX];

#define PSCI_0_2_POWER_STATE_MASK		\
				(PSCI_0_2_POW