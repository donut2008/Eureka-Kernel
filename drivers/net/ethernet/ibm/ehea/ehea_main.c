/* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <linux/suspend.h>
#include <linux/pm_runtime.h>

#if IS_ENABLED(CONFIG_EXYNOS_PMU)
#include <soc/samsung/exynos-pmu.h>
#endif
#if IS_ENABLED(CONFIG_EXYNOS_PMU_IF)
#include <soc/samsung/exynos-pmu-if.h>
#endif

#include <gpex_ifpo.h>
#include <gpex_dvfs.h>
#include <gpex_clock.h>
#include <gpex_qos.h>
#include <gpex_utils.h>
#include <gpex_debug.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_pm.h>
#include <gpex_pm.h>

#include <gpexbe_secure.h>
#include <gpexbe_smc.h>

#include <gpex_tsg.h>
#include <gpex_clboost.h>

#include <gpexwa_wakeup_clock.h>

#if IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_ARM) && IS_ENABLED(CONFIG_SOC_S5E8825)
#include <soc/samsung/exynos-smc.h>
#endif

struct pm_info {
	int runtime_pm_delay_time;
	bool power_status;
	int state;
	bool skip_auto_suspend;
	struct device *dev;
};

static struct pm_info pm;

int gpex_pm_set_state(int state)
{
	if (GPEX_PM_STATE_START <= state && state <= GPEX_PM_STATE_END) {
		pm.state = state;
		GPU_LOG(MALI_EXYNOS_INFO, "gpex_pm: gpex_pm state is set as 0x%X", pm.state);
		return 0;
	}

	GPU_LOG(MALI_EXYNOS_WARNING,
		"gpex_pm: Attempted to set gpex_pm state with invalid value 0x%x", state);
	return -1;
}

int gpex_pm_get_state(int *state)
{
	if (GPEX_PM_STATE_START <= pm.state && pm.state <= GPEX_PM_STATE_END) {
		*state = pm.state;
		return 0;
	}

	GPU_LOG(MALI_EXYNOS_WARNING, "gpex_pm: gpex_pm has invalid state now 0x%x", pm.state);
	return -1;
}

int gpex_pm_get_status(bool clock_lock)
{
	int ret = 0;
	//unsigned int val = 0;

	if (clock_lock)
		gpex_clock_mutex_lock();

	ret = gpexbe_pm_get_status();

	if (clock_lock)
		gpex_clock_mutex_unlock();

	return ret;
}

/* Read the power_status value set by gpex_pm module */
int gpex_pm_get_power_status(void)
{
	return pm.power_status;
}

void gpex_pm_lock(void)
{
	gpexbe_pm_access_lock();
}

void gpex_pm_unlock(void)
{
	gpexbe_pm_access_unlock();
}

static ssize_t show_power_state(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_pm_get_status(true));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_power_state);

/******************************
 * RTPM callback functions
 * ***************************/
int gpex_pm_power_on(struct device *dev)
{
	int ret = 0;

	pm.skip_auto_suspend = false;

#if IS_ENABLED(CONFIG_MALI_EXYNOS_BLOCK_RPM_WHILE_SUSPEND_RESUME)
	if (pm.state == GPEX_PM_STATE_RESUME_BEGIN && gpex_pm_get_status(false) > 0) {
		pm.skip_auto_suspend = true;
	} else {
		gpex_debug_new_record(HIST_RTPM);
		ret = pm_runtime_get_sync(dev);
		gpex_debug_record(HIST_RTPM, 0, PM_RUNTIME_GET_SYNC, ret);
	}
#else
	gpex_debug_new_record(HIST_RTPM);
	ret = pm_runtime_get_sync(dev);
	gpex_debug_record(HIST_RTPM, 0, PM_RUNTIME_GET_SYNC, ret);
#endif

	if (ret >= 0) {
		gpex_ifpo_power_up();
		GPU_LOG_DETAILED(MALI_EXYNOS_INFO, LSI_GPU_RPM_RESUME_API, ret, 0u, "power on\n");
	} else {
		GPU_LOG(MALI_EXYNOS_ERROR, "runtime pm returned %d\n", ret);
		gpex_debug_incr_error_cnt(HIST_RTPM);
	}

	gpex_dvfs_start();

	return ret;
}

void gpex_pm_power_autosuspend(struct device *dev)
{
	int ret = 0;

	gpex_ifpo_power_down();

	if (!pm.skip_auto_suspend) {
		pm_runtime_mark_last_busy(dev);
		ret = pm_runtime_put_autosuspend(dev);
	}

	GPU_LOG_DETAILED(MALI_EXYNOS_INFO, LSI_GPU_RPM_SUSPEND_API, ret, 0u,
			 "power autosuspend prepare\n");
}

void gpex_pm_suspend(struct device *dev)
{
	int ret = 0;

	gpexwa_wakeup_clock_suspend();
	gpex_qos_set_from_clock(0);

	gpex_debug_new_record(HIST_SUSPEND);
	ret = pm_runtime_suspend(dev);
	gpex_debug_record(HIST_SUSPEND, 0, PM_RUNTIME_SUSPEND, ret);

	if (ret < 0)
		gpex_debug_incr_error_cnt(HIST_SUSPEND);

	GPU_LOG_DETAILED(MALI_EXYNOS_INFO, LSI_SUSPEND_CALLBACK, ret, 0u, "power suspend\n");
}

static struct delayed_work gpu_poweroff_delay_set_work;

static void gpu_poweroff_delay_recovery_callback(struct work_struct *data)
{
	if (!pm.dev->power.use_autosuspend)
		return;

	device_lock(pm.dev);
	pm_runtime_set_autosuspend_delay(pm.dev, pm.runtime_pm_delay_time);
	device_unlock(pm.dev);

	gpex_clock_set_user_min_lock_input(0);
	gpex_clock_lock_clock(GPU_CLOCK_MIN_UNLOCK, SYSFS_LOCK, 0);
	GPU_LOG(MALI_EXYNOS_DEBUG, "gpu poweroff delay recovery done & clock min unlock\n");

	gpex_clboost_set_state(CLBOOST_ENABLE);
}

static int gpu_poweroff_delay_recovery(unsigned int period)
{
#define POWEROFF_DELAY_MAX_PERIOD 1500
#define POWEROFF_DELAY_MIN_PERIOD 50

	/* boundary */
	if (period > POWEROFF_DELAY_MAX_PERIOD)
		period = POWEROFF_DELAY_MAX_PERIOD;
	else if (period < POWEROFF_DELAY_MIN_PERIOD)
		period = POWEROFF_DELAY_MIN_PERIOD;

	GPU_LOG(MALI_EXYNOS_DEBUG, "gpu poweroff delay set wq start(%u)\n", period);

	cancel_delayed_work_sync(&gpu_poweroff_delay_set_work);
	schedule_delayed_work(&gpu_poweroff_delay_set_work, msecs_to_jiffies(period));

	return 0;
}

static ssize_t set_sysfs_poweroff_delay(const char *buf, size_t count)
{
	long delay;

	if (!pm.dev->power.use_autosuspend)
		return -EIO;

	if (kstrtol(buf, 10, &delay) != 0 || delay != (int)delay)
		return -EINVAL;

	if (delay < pm.runtime_pm_delay_time)
		delay = pm.runtime_pm_delay_time;

	device_lock(pm.dev);
	pm_runtime_set_autosuspend_delay(pm.dev, delay);
	device_unlock(pm.dev);

	gpu_poweroff_delay_recovery((unsigned int)delay);

	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_sysfs_poweroff_delay)

static ssize_t show_sysfs_poweroff_delay(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", pm.runtime_pm_delay_time);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_sysfs_poweroff_delay)

static void gpu_poweroff_delay_wq_init(void)
{
	INIT_DELAYED_WORK(&gpu_poweroff_delay_set_work, gpu_poweroff_delay_recovery_callback);
}

static void gpu_poweroff_delay_wq_deinit(void)
{
	cancel_delayed_work_sync(&gpu_poweroff_delay_set_work);
}

int gpex_pm_runtime_init(struct device *dev)
{
	int ret = 0;

	pm_runtime_set_autosuspend_delay(dev, pm.runtime_pm_delay_time);
	pm_runtime_use_autosuspend(dev);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	gpu_poweroff_delay_wq_init();

	if (!pm_runtime_enabled(dev)) {
		dev_warn(dev, "pm_runtime not enabled");
		ret = -ENOSYS;
	}

	return ret;
}

void gpex_pm_runtime_term(struct device *dev)
{
	pm_runtime_disable(dev);

	gpu_poweroff_delay_wq_deinit();
}

int gpex_pm_runtime_on_prepare(struct device *dev)
{
	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_GPU_ON, 0u, 0u, "runtime on callback\n");

	pm.power_status = true;

	gpexbe_smc_notify_power_on();

	gpexwa_wakeup_clock_restore();

	return 0;
}

/* TODO: 9830 need to store and restore clock before and after power off/on */
#if 0
int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform)
		return -ENODEV;

	GPU_LOG(MALI_EXYNOS_DEBUG, LSI_GPU_ON, 0u, 0u, "runtime on callback\n");

	platform->power_status = true;

	/* Set clock - restore previous g3d clock, after g3d runtime on */
	if (gpex_dvfs_get_status() && platform->wakeup_lock) {
		if (platform->restore_clock > G3D_DVFS_MIDDLE_CLOCK) {
			gpex_clock_set(platform->restore_clock);
			GPU_LOG(MALI_EXYNOS_DEBUG, LSI_GPU_ON, platform->restore_clock, gpex_clock_get_cur_clock(), "runtime on callback - restore clock = %d, cur clock = %d\n", platform->restore_clock, gpex_clock_get_cur_clock());
			platform->restore_clock = 0;
		}
	}
	return 0;
}
#endif

void gpex_pm_runtime_off_prepare(struct device *dev)
{
	CSTD_UNUSED(dev);
	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_GPU_OFF, 0u, 0u, "runtime off callback\n");

	gpexbe_smc_notify_power_off();

	/* power up from ifpo down state before going to full rtpm power off */
	gpex_ifpo_power_up();
	gpex_tsg_reset_count(0);
	gpex_dvfs_stop();

	gpex_clock_prepare_runtime_off();
	gpexwa_wakeup_clock_set();
	gpex_qos_set_from_clock(0);

	pm.power_status = false;
}

int gpex_pm_init(void)
{
	pm.power_status = true;

	pm.runtime_pm_delay_time = gpexbe_devicetree_get_int(gpu_runtime_pm_delay_time);

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(power_state, show_power_state);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_poweroff_delay, show_sysfs_poweroff_delay,
					  set_sysfs_poweroff_delay);

	pm.state = GPEX_PM_STATE_START;
	pm.skip_auto_suspend = false;

	pm.dev = gpex_utils_get_device();

	gpex_utils_get_exynos_context()->pm = &pm;

	return 0;
}

void gpex_pm_term(void)
{
	memset(&pm, 0, sizeof(struct pm_info));

	return;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <linux/version.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include <gpexbe_devicetree.h>
#include <gpex_utils.h>
#include <gpex_debug.h>
#include <gpex_qos.h>
#include <gpex_clock.h>
#include <gpex_tsg.h>
#include <gpexbe_qos.h>
#include <gpexbe_bts.h>
#include <gpex_clboost.h>
#include <gpex_gts.h>
#include <gpexbe_llc_coherency.h>

#include <gpexwa_peak_notify.h>

#define HCM_MODE_A (1 << 0)
#define HCM_MODE_B (1 << 1)
#define HCM_MODE_C (1 << 2)

struct _qos_info {
	bool is_pm_qos_init;
	int mo_min_clock;
	unsigned int is_set_bts; // Check the pair of bts scenario.
	bool gpu_bts_support;
	spinlock_t bts_spinlock;

	int cpu_cluster_count; // is this worth keeping after init?

	int qos_table_size;
	int clqos_table_size;

	/* HCM Stuff */
	int gpu_heavy_compute_cpu0_min_clock;
	int gpu_heavy_compute_vk_cpu0_min_clock;
};

struct _qos_table {
	int gpu_clock;
	int mem_freq;
	int cpu_little_min_freq;
	int cpu_middle_min_freq;
	int cpu_big_max_freq;
	int llc_ways;
};

struct _clqos_table {
	int gpu_clock;
	int mif_min;
	int little_min;
	int middle_min;
	int big_max;
};

static struct _qos_info qos_info;
static struct _qos_table *qos_table;
static struct _clqos_table *clqos_table;

static int gpex_qos_get_table_idx(int clock)
{
	int idx;

	for (idx = 0; idx < qos_info.qos_table_size; idx++) {
		if (qos_table[idx].gpu_clock == clock)
			return idx;
	}

	return -EINVAL;
}

int gpex_qos_set(gpex_qos_flag flags, int val)
{
	if (!qos_info.is_pm_qos_init) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: PM QOS ERROR : pm_qos not initialized\n", __func__);
		return -ENOENT;
	}

	gpexbe_qos_request_update((mali_pmqos_flags)flags, val);

	/* TODO: record PMQOS state somewhere */

	return 0;
}

int gpex_qos_unset(gpex_qos_flag flags)
{
	if (!qos_info.is_pm_qos_init) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: PM QOS ERROR : pm_qos not initialized\n", __func__);
		return -ENOENT;
	}
	gpexbe_qos_request_unset((mali_pmqos_flags)flags);

	/* TODO: record PMQOS state somewhere */

	return 0;
}

int gpex_qos_init(void)
{
	int i = 0;
	gpu_dt *dt = gpexbe_devicetree_get_gpu_dt();

	/* TODO: check dependent backends are initializaed */

	spin_lock_init(&qos_info.bts_spinlock);

	qos_info.gpu_bts_support = (bool)gpexbe_devicetree_get_int(gpu_bts_support);
	qos_info.mo_min_clock = gpexbe_devicetree_get_int(gpu_mo_min_clock);

	qos_info.qos_table_size = dt->gpu_dvfs_table_size.row;
	qos_table = kcalloc(qos_info.qos_table_size, sizeof(*qos_table), GFP_KERNEL);

	for (i = 0; i < qos_info.qos_table_size; i++) {
		qos_table[i].gpu_clock = dt->gpu_dvfs_table[i].clock;
		qos_table[i].mem_freq = dt->gpu_dvfs_table[i].mem_freq;
		qos_table[i].cpu_little_min_freq = dt->gpu_dvfs_table[i].cpu_little_min_freq;
		qos_table[i].cpu_middle_min_freq = dt->gpu_dvfs_table[i].cpu_middle_min_freq;
		qos_table[i].cpu_big_max_freq = dt->gpu_dvfs_table[i].cpu_big_max_freq;
		qos_table[i].llc_ways = dt->gpu_dvfs_table[i].llc_ways;
	}

#if 0
	if (gpex_qos_get_table_idx(qos_info.mo_min_clock) < 0) {
		/* TODO: print error msg */
		BUG();
		return -1;
	}
#endif

	qos_info.clqos_table_size = dt->gpu_cl_pmqos_table_size.row;
	clqos_table = kcalloc(qos_info.clqos_table_size, sizeof(*clqos_table), GFP_KERNEL);

	for (i = 0; i < qos_info.clqos_table_size; i++) {
		clqos_table[i].gpu_clock = dt->gpu_cl_pmqos_table[i].clock;
		clqos_table[i].mif_min = dt->gpu_cl_pmqos_table[i].mif_min;
		clqos_table[i].little_min = dt->gpu_cl_pmqos_table[i].little_min;
		clqos_table[i].middle_min = dt->gpu_cl_pmqos_table[i].middle_min;
		clqos_table[i].big_max = dt->gpu_cl_pmqos_table[i].big_max;
	}

	qos_info.gpu_heavy_compute_cpu0_min_clock =
		gpexbe_devicetree_get_int(gpu_heavy_compute_cpu0_min_clock);
	qos_info.gpu_heavy_compute_vk_cpu0_min_clock =
		gpexbe_devicetree_get_int(gpu_heavy_compute_vk_cpu0_min_clock);

	/* Request to set QOS of other IPs */
	gpexbe_qos_request_add(PMQOS_MIF | PMQOS_LITTLE | PMQOS_MIDDLE | PMQOS_BIG | PMQOS_MIN |
			       PMQOS_MAX);

	qos_info.is_pm_qos_init = true;

	gpex_utils_get_exynos_context()->qos_info = &qos_info;
	gpex_utils_get_exynos_context()->qos_table = qos_table;
	gpex_utils_get_exynos_context()->clqos_table = clqos_table;

	return 0;
}

void gpex_qos_term(void)
{
	gpexbe_qos_request_remove(PMQOS_MIF | PMQOS_LITTLE | PMQOS_MIDDLE | PMQOS_BIG | PMQOS_MIN |
				  PMQOS_MAX);
	kfree(qos_table);
	kfree(clqos_table);
	qos_info.is_pm_qos_init = false;
}

int gpex_qos_set_bts_mo(int clock)
{
	int ret = 0;
	unsigned long flags;

	if (!qos_info.gpu_bts_support) {
		if (qos_info.is_set_bts) {
			/* TODO: print error */
			return -1;
		}
		return 0;
	}

	spin_lock_irqsave(&qos_info.bts_spinlock, flags);

	if (clock >= qos_info.mo_min_clock && !qos_info.is_set_bts) {
		gpex_debug_new_record(HIST_BTS);

		ret = gpexbe_bts_set_bts_mo(1);
		gpex_debug_record(HIST_BTS, qos_info.is_set_bts, 1, ret);

		if (ret) {
			GPU_LOG(MALI_EXYNOS_WARNING, "BTS MO could not be set to gpu performance");
			gpex_debug_incr_error_cnt(HIST_BTS);
		} else
			qos_info.is_set_bts = 1;

	} else if ((clock == 0 || clock < qos_info.mo_min_clock) && qos_info.is_set_bts) {
		gpex_debug_new_record(HIST_BTS);

		ret = gpexbe_bts_set_bts_mo(0);
		gpex_debug_record(HIST_BTS, qos_info.is_set_bts, 0, ret);

		if (ret) {
			GPU_LOG(MALI_EXYNOS_WARNING, "BTS MO could not be unset from gpu performance");
			gpex_debug_incr_error_cnt(HIST_BTS);
		} else
			qos_info.is_set_bts = 0;
	}

	spin_unlock_irqrestore(&qos_info.bts_spinlock, flags);

	return ret;
}

int gpex_qos_set_from_clock(int gpu_clock)
{
	int idx = 0;

	if (gpu_clock == 0) {
		gpex_qos_unset(QOS_MIF | QOS_LITTLE | QOS_MIDDLE | QOS_MIN);
		gpex_qos_set_bts_mo(gpu_clock);
		gpexbe_llc_coherency_set_ways(0);

		return 0;
	}

	idx = gpex_qos_get_table_idx(gpu_clock);

	if (idx < 0) {
		/* TODO: print error msg */
		return -EINVAL;
	}

	if (gpex_clboost_check_activation_condition()) {
		gpex_qos_set(QOS_MIF | QOS_MIN, clqos_table[idx].mif_min);
		gpex_qos_set(QOS_LITTLE | QOS_MIN, clqos_table[idx].little_min);
		gpex_qos_set(QOS_MIDDLE | QOS_MIN, clqos_table[idx].middle_min);
		gpex_qos_set(QOS_BIG | QOS_MAX, INT_MAX);
		/* TODO: revamp the qos interface so default max min can be set without knowing the clock */
		//gpex_qos_set(QOS_BIG | QOS_MAX, BIG_MAX);
	} else {
		if (gpex_tsg_get_pmqos() == true) {
			gpex_qos_set(QOS_MIF | QOS_MIN, 0);
			gpex_qos_set(QOS_LITTLE | QOS_MIN, 0);
			gpex_qos_set(QOS_MIDDLE | QOS_MIN, 0);
			gpex_qos_set(QOS_BIG | QOS_MAX, INT_MAX);
		} else {
			gpex_qos_set(QOS_MIF | QOS_MIN, qos_table[idx].mem_freq);

			if (!(gpex_gts_get_hcm_mode() & (HCM_MODE_A | HCM_MODE_C))) {
				gpex_qos_set(QOS_LITTLE | QOS_MIN,
					     qos_table[idx].cpu_little_min_freq);
			} else if (gpex_gts_get_hcm_mode() & HCM_MODE_A) {
				gpex_qos_set(QOS_LITTLE | QOS_MIN,
					     qos_info.gpu_heavy_compute_cpu0_min_clock);
				//pr_info("HCM: mode A QOS");
			} else if (gpex_gts_get_hcm_mode() & HCM_MODE_C) {
				gpex_qos_set(QOS_LITTLE | QOS_MIN,
					     qos_info.gpu_heavy_compute_vk_cpu0_min_clock);
				//pr_info("HCM: mode B QOS");
			}

			gpex_qos_set(QOS_MIDDLE | QOS_MIN, qos_table[idx].cpu_middle_min_freq);
			gpex_qos_set(QOS_BIG | QOS_MAX, qos_table[idx].cpu_big_max_freq);
		}

		gpexwa_peak_notify_update();
	}

	gpex_qos_set_bts_mo(gpu_clock);
	gpexbe_llc_coherency_set_ways(qos_table[idx].llc_ways);

	return 0;
}
            /* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <linux/notifier.h>
#include <linux/thermal.h>

#include <gpex_utils.h>
#include <gpex_thermal.h>
#include <gpex_clock.h>
#include <gpexbe_debug.h>

struct _thermal_info {
	bool tmu_enabled;
};

static struct _thermal_info thermal;

void gpex_thermal_set_status(bool status)
{
	thermal.tmu_enabled = status;
}

int gpex_thermal_gpu_normal(void)
{
	int ret = 0;
	ret = gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, TMU_LOCK, 0);

	if (ret) {
		/* TODO: couldn't handle thermal throttling. print error log */
		;
	}

	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_TMU_VALUE, 0u, event,
			 "tmu event normal, remove gpu max lock\n");

	return ret;
}

int gpex_thermal_gpu_throttle(int freq)
{
	int ret = 0;

	if (!thermal.tmu_enabled) {
		ret = gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, TMU_LOCK, 0);
		/* TODO: print warning that gpu must be throttled, but gpu thermal is disabled */
		return ret;
	}

	ret = gpex_clock_lock_clock(GPU_CLOCK_MAX_LOCK, TMU_LOCK, freq);

	if (ret) {
		/* TODO: couldn't handle thermal throttling. print error log */
		;
	}

	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_TMU_VALUE, 0u, event,
			 "tmu event throttling, frequency %d\n", freq);

	gpexbe_debug_dbg_snapshot_thermal(freq);

	return ret;
}

/***********************************************************************
 * SYSFS FUNCTIONS
 ***********************************************************************/
static ssize_t show_tmu(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", thermal.tmu_enabled);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_tmu);

static ssize_t set_tmu_control(const char *buf, size_t count)
{
	if (sysfs_streq("0", buf)) {
		gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, TMU_LOCK, 0);
		thermal.tmu_enabled = false;
	} else if (sysfs_streq("1", buf))
		thermal.tmu_enabled = true;
	else
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value - only [0 or 1] is available\n",
			__func__);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_tmu_control);

static ssize_t show_kernel_sysfs_gpu_temp(char *buf)
{
	ssize_t ret = 0;
	int err = 0;
	int gpu_temp = 0;
	int gpu_temp_int = 0;
	int gpu_temp_point = 0;
	static struct thermal_zone_device *tz;

	/* TODO: move thermal_zone related funcs to backend */
	if (!tz)
		tz = thermal_zone_get_zone_by_name("G3D");

	err = thermal_zone_get_temp(tz, &gpu_temp);
	if (err) {
		GPU_LOG(MALI_EXYNOS_ERROR, "Error reading temp of gpu thermal zone: %d\n", err);
		return -ENODEV;
	}

	gpu_temp_int = gpu_temp / 1000;
	gpu_temp_point = gpu_temp % gpu_temp_int;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d.%d", gpu_temp_int, gpu_temp_point);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_gpu_temp);

static void gpex_thermal_create_sysfs_file(void)
{
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(tmu, show_tmu, set_tmu_control);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_tmu, show_kernel_sysfs_gpu_temp);
}

/***********************************************************************
 * INIT, TERM FUNCTIONS
 ***********************************************************************/
int gpex_thermal_init(void)
{
	gpex_thermal_create_sysfs_file();

	gpex_utils_get_exynos_context()->thermal = &thermal;

	return 0;
}

void gpex_thermal_term(void)
{
	thermal.tmu_enabled = false;

	/* TODO: unregister tmu notifier */
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <gpex_tsg.h>

#include <gpex_utils.h>
#include <gpex_dvfs.h>
#include <gpex_pm.h>
#include <gpex_clock.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_utilization.h>

#include "gpex_tsg_internal.h"
#include "gpex_dvfs_internal.h"
#include "gpu_dvfs_governor.h"

static struct _tsg_info tsg;

void gpex_tsg_input_nr_acc_cnt(void)
{
	tsg.input_job_nr_acc += tsg.input_job_nr;
}

void gpex_tsg_reset_acc_count(void)
{
	tsg.input_job_nr_acc = 0;
	gpex_tsg_set_queued_time_tick(0, 0);
	gpex_tsg_set_queued_time_tick(1, 0);
}

/* SETTER */
void gpex_tsg_set_migov_mode(int mode)
{
	tsg.migov_mode = mode;
	return;
}

void gpex_tsg_set_freq_margin(int margin)
{
	tsg.freq_margin = margin;
	return;
}

void gpex_tsg_set_util_history(int idx, int order, int input)
{
	tsg.prediction.util_history[idx][order] = input;
	return;
}

void gpex_tsg_set_weight_util(int idx, int input)
{
	tsg.prediction.weight_util[idx] = input;
	return;
}

void gpex_tsg_set_weight_freq(int input)
{
	tsg.prediction.weight_freq = input;
	return;
}

void gpex_tsg_set_en_signal(bool input)
{
	tsg.prediction.en_signal = input;
	return;
}

void gpex_tsg_set_pmqos(bool input)
{
	tsg.is_pm_qos_tsg = input;
	return;
}

void gpex_tsg_set_weight_table_idx(int idx, int input)
{
	if (idx == 0)
		tsg.weight_table_idx_0 = input;
	else
		tsg.weight_table_idx_1 = input;
	return;
}

void gpex_tsg_set_is_gov_set(int input)
{
	tsg.is_gov_set = input;
	return;
}

void gpex_tsg_set_saved_polling_speed(int input)
{
	tsg.migov_saved_polling_speed = input;
	return;
}

void gpex_tsg_set_governor_type_init(int input)
{
	tsg.governor_type_init = input;
	return;
}

void gpex_tsg_set_amigo_flags(int input)
{
	tsg.amigo_flags = input;
	return;
}

void gpex_tsg_set_queued_threshold(int idx, uint32_t input)
{
	tsg.queue.queued_threshold[idx] = input;
	return;
}

void gpex_tsg_set_queued_time_tick(int idx, ktime_t input)
{
	tsg.queue.queued_time_tick[idx] = input;
	return;
}

void gpex_tsg_set_queued_time(int idx, ktime_t input)
{
	tsg.queue.queued_time[idx] = input;
}

void gpex_tsg_set_queued_last_updated(ktime_t input)
{
	tsg.queue.last_updated = input;
}

void gpex_tsg_set_queue_nr(int type, int variation)
{
	/* GPEX_TSG_QUEUE_JOB/IN/OUT: 0, 1, 2*/
	/* GPEX_TSG_QUEUE_INC/DEC/RST: 3, 4, 5*/

	if (variation == GPEX_TSG_RST)
		atomic_set(&tsg.queue.nr[type], 0);
	else
		(variation == GPEX_TSG_INC) ? atomic_inc(&tsg.queue.nr[type]) :
						    atomic_dec(&tsg.queue.nr[type]);
}

/* GETTER */
int gpex_tsg_get_migov_mode(void)
{
	return tsg.migov_mode;
}

int gpex_tsg_get_freq_margin(void)
{
	return tsg.freq_margin;
}

int gpex_tsg_get_util_history(int idx, int order)
{
	return tsg.prediction.util_history[idx][order];
}

int gpex_tsg_get_weight_util(int idx)
{
	return tsg.prediction.weight_util[idx];
}

int gpex_tsg_get_weight_freq(void)
{
	return tsg.prediction.weight_freq;
}

bool gpex_tsg_get_en_signal(void)
{
	return tsg.prediction.en_signal;
}

bool gpex_tsg_get_pmqos(void)
{
	return tsg.is_pm_qos_tsg;
}

int gpex_tsg_get_weight_table_idx(int idx)
{
	return (idx == 0) ? tsg.weight_table_idx_0 : tsg.weight_table_idx_1;
}

int gpex_tsg_get_is_gov_set(void)
{
	return tsg.is_gov_set;
}

int gpex_tsg_get_saved_polling_speed(void)
{
	return tsg.migov_saved_polling_speed;
}

int gpex_tsg_get_governor_type_init(void)
{
	return tsg.governor_type_init;
}

int gpex_tsg_get_amigo_flags(void)
{
	return tsg.amigo_flags;
}

uint32_t gpex_tsg_get_queued_threshold(int idx)
{
	return tsg.queue.queued_threshold[idx];
}

ktime_t gpex_tsg_get_queued_time_tick(int idx)
{
	return tsg.queue.queued_time_tick[idx];
}

ktime_t gpex_tsg_get_queued_time(int idx)
{
	return tsg.queue.queued_time[idx];
}

ktime_t *gpex_tsg_get_queued_time_array(void)
{
	return tsg.queue.queued_time;
}

ktime_t gpex_tsg_get_queued_last_updated(void)
{
	return tsg.queue.last_updated;
}

int gpex_tsg_get_queue_nr(int type)
{
	return atomic_read(&tsg.queue.nr[type]);
}

struct atomic_notifier_head *gpex_tsg_get_frag_utils_change_notifier_list(void)
{
	return &(tsg.frag_utils_change_notifier_list);
}

int gpex_tsg_notify_frag_utils_change(u32 js0_utils)
{
	int ret = 0;
	ret = atomic_notifier_call_chain(gpex_tsg_get_frag_utils_change_notifier_list(), js0_utils,
					 NULL);
	return ret;
}

int gpex_tsg_spin_lock(void)
{
	return raw_spin_trylock(&tsg.spinlock);
}

void gpex_tsg_spin_unlock(void)
{
	raw_spin_unlock(&tsg.spinlock);
}

/* Function of Kbase */

static inline bool both_q_active(int in_nr, int out_nr)
{
	return in_nr > 0 && out_nr > 0;
}

static inline bool hw_q_active(int out_nr)
{
	return out_nr > 0;
}

static void accum_queued_time(ktime_t current_time, bool accum0, bool accum1)
{
	int time_diff = 0;
	time_diff = current_time - gpex_tsg_get_queued_last_updated();

	if (accum0 == true)
		gpex_tsg_set_queued_time_tick(0, gpex_tsg_get_queued_time_tick(0) + time_diff);
	if (accum1 == true)
		gpex_tsg_set_queued_time_tick(1, gpex_tsg_get_queued_time_tick(1) + time_diff);

	gpex_tsg_set_queued_last_updated(current_time);
}

int gpex_tsg_reset_count(int powered)
{
	if (powered)
		return 0;

	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_JOB, GPEX_TSG_RST);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_RST);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_RST);
	gpex_tsg_set_queued_last_updated(0);

	tsg.input_job_nr = 0;

	return !powered;
}

int gpex_tsg_set_count(u32 status, bool stop)
{
	int prev_out_nr, prev_in_nr;
	int cur_out_nr, cur_in_nr;
	bool need_update = false;
	ktime_t current_time = 0;

	if (gpex_tsg_get_queued_last_updated() == 0)
		gpex_tsg_set_queued_last_updated(ktime_get());

	prev_out_nr = gpex_tsg_get_queue_nr(GPEX_TSG_QUEUE_OUT);
	prev_in_nr = gpex_tsg_get_queue_nr(GPEX_TSG_QUEUE_IN);

	if (stop) {
		gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_INC);
		gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_DEC);
		tsg.input_job_nr++;
	} else {
		switch (status) {
		case GPEX_TSG_ATOM_STATE_QUEUED:
			gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_INC);
			tsg.input_job_nr++;
			break;
		case GPEX_TSG_ATOM_STATE_IN_JS:
			gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_DEC);
			gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_INC);
			tsg.input_job_nr--;
			if (tsg.input_job_nr < 0)
				tsg.input_job_nr = 0;
			break;
		case GPEX_TSG_ATOM_STATE_HW_COMPLETED:
			gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_DEC);
			break;
		default:
			break;
		}
	}

	cur_in_nr = gpex_tsg_get_queue_nr(GPEX_TSG_QUEUE_IN);
	cur_out_nr = gpex_tsg_get_queue_nr(GPEX_TSG_QUEUE_OUT);

	if ((!both_q_active(prev_in_nr, prev_out_nr) && both_q_active(cur_in_nr, cur_out_nr)) ||
	    (!hw_q_active(prev_out_nr) && hw_q_active(cur_out_nr)))
		need_update = true;
	else if ((both_q_active(prev_in_nr, prev_out_nr) &&
		  !both_q_active(cur_in_nr, cur_out_nr)) ||
		 (hw_q_active(prev_out_nr) && !hw_q_active(cur_out_nr)))
		need_update = true;
	else if (prev_out_nr > cur_out_nr) {
		current_time = ktime_get();
		need_update =
			current_time - (gpex_tsg_get_queued_last_updated() > 2) * GPEX_TSG_PERIOD;
	}

	if (need_update) {
		current_time = (current_time == 0) ? ktime_get() : current_time;
		if (gpex_tsg_spin_lock()) {
			accum_queued_time(current_time, both_q_active(prev_in_nr, prev_out_nr),
					  hw_q_active(prev_out_nr));
			gpex_tsg_spin_unlock();
		}
	}

	if ((cur_in_nr + cur_out_nr) < 0)
		gpex_tsg_reset_count(0);

	return 0;
}

static void gpex_tsg_context_init(void)
{
	gpex_tsg_set_weight_table_idx(0, gpexbe_devicetree_get_int(gpu_weight_table_idx_0));
	gpex_tsg_set_weight_table_idx(1, gpexbe_devicetree_get_int(gpu_weight_table_idx_1));
	gpex_tsg_set_governor_type_init(gpex_dvfs_get_governor_type());
	gpex_tsg_set_queued_threshold(0, 0);
	gpex_tsg_set_queued_threshold(1, 0);
	gpex_tsg_set_queued_time_tick(0, 0);
	gpex_tsg_set_queued_time_tick(1, 0);
	gpex_tsg_set_queued_time(0, 0);
	gpex_tsg_set_queued_time(1, 0);
	gpex_tsg_set_queued_last_updated(0);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_JOB, GPEX_TSG_RST);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_RST);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_RST);

	tsg.input_job_nr = 0;
	tsg.input_job_nr_acc = 0;
}

int gpex_tsg_init(struct device **dev)
{
	raw_spin_lock_init(&tsg.spinlock);
	tsg.kbdev = container_of(dev, struct kbase_device, dev);

	gpex_tsg_context_init();
	gpex_tsg_external_init(&tsg);
	gpex_tsg_sysfs_init(&tsg);

	gpex_utils_get_exynos_context()->tsg = &tsg;

	return 0;
}

int gpex_tsg_term(void)
{
	gpex_tsg_sysfs_term();
	gpex_tsg_external_term();
	tsg.kbdev = NULL;

	return 0;
}

void gpex_tsg_update_firstjob_time(void)
{
}

void gpex_tsg_update_lastjob_time(int slot_nr)
{
	CSTD_UNUSED(slot_nr);
}

void gpex_tsg_update_jobsubmit_time(void)
{
}

void gpex_tsg_sum_jobs_time(int slot_nr)
{
	CSTD_UNUSED(slot_nr);
}

int gpex_tsg_amigo_interframe_sw_update(ktime_t start, ktime_t end)
{
	CSTD_UNUSED(start);
	CSTD_UNUSED(end);

	return 0;
}

int gpex_tsg_amigo_interframe_hw_update_eof(void)
{
	return 0;
}

int gpex_tsg_amigo_interframe_hw_update(void)
{
	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <linux/notifier.h>
#include <linux/ktime.h>

#include <gpex_tsg.h>
#include <gpex_dvfs.h>
#include <gpex_utils.h>
#include <gpex_pm.h>
#include <gpex_ifpo.h>
#include <gpex_clock.h>
#include <gpexbe_pm.h>

#include "gpex_tsg_internal.h"

#define DVFS_TABLE_ROW_MAX 20

static struct _tsg_info *tsg_info;

unsigned long exynos_stats_get_job_state_cnt(void)
{
	return tsg_info->input_job_nr_acc;
}
EXPORT_SYMBOL(exynos_stats_get_job_state_cnt);

int exynos_stats_get_gpu_cur_idx(void)
{
	int i;
	int level = -1;
	int clock = 0;
	int idx_max_clk, idx_min_clk;

	if (gpexbe_pm_get_exynos_pm_domain()) {
		if (gpexbe_pm_get_status() == 1) {
			clock = gpex_clock_get_cur_clock();
		} else {
			GPU_LOG(MALI_EXYNOS_ERROR, "%s: can't get dvfs cur clock\n", __func__);
			clock = 0;
		}
	} else {
		if (gpex_pm_get_status(true) == 1) {
			if (gpex_ifpo_get_mode() && !gpex_ifpo_get_status()) {
				GPU_LOG(MALI_EXYNOS_ERROR, "%s: can't get dvfs cur clock\n",
					__func__);
				clock = 0;
			} else {
				clock = gpex_clock_get_cur_clock();
			}
		}
	}

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. max_clock_level : %d, min_clock_level : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return -1;
	}

	if (clock == 0)
		return idx_min_clk - idx_max_clk;

	for (i = idx_max_clk; i <= idx_min_clk; i++) {
		if (gpex_clock_get_clock(i) == clock) {
			level = i;
			break;
		}
	}

	return (level - idx_max_clk);
}
EXPORT_SYMBOL(exynos_stats_get_gpu_cur_idx);

int exynos_stats_get_gpu_coeff(void)
{
	int coef = 6144;

	return coef;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_coeff);

uint32_t exynos_stats_get_gpu_table_size(void)
{
	return (gpex_clock_get_table_idx(gpex_clock_get_min_clock()) -
		gpex_clock_get_table_idx(gpex_clock_get_max_clock()) + 1);
}
EXPORT_SYMBOL(exynos_stats_get_gpu_table_size);

static uint32_t freqs[DVFS_TABLE_ROW_MAX];
uint32_t *exynos_stats_get_gpu_freq_table(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return freqs;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++)
		freqs[i - idx_max_clk] = (uint32_t)gpex_clock_get_clock(i);

	return freqs;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_freq_table);

static uint32_t volts[DVFS_TABLE_ROW_MAX];
uint32_t *exynos_stats_get_gpu_volt_table(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return volts;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++)
		volts[i - idx_max_clk] = (uint32_t)gpex_clock_get_voltage(gpex_clock_get_clock(i));

	return volts;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_volt_table);

static ktime_t time_in_state[DVFS_TABLE_ROW_MAX];
ktime_t *exynos_stats_get_gpu_time_in_state(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return time_in_state;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++) {
		time_in_state[i - idx_max_clk] =
			ms_to_ktime((u64)(gpex_clock_get_time_busy(i) * 4) / 100);
	}

	return time_in_state;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_time_in_state);

int exynos_stats_get_gpu_max_lock(void)
{
	unsigned long flags;
	int locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = gpex_clock_get_max_lock();
	if (locked_clock <= 0)
		locked_clock = gpex_clock_get_max_clock();
	gpex_dvfs_spin_unlock(&flags);

	return locked_clock;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_max_lock);

int exynos_stats_get_gpu_min_lock(void)
{
	unsigned long flags;
	int locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = gpex_clock_get_min_lock();
	if (locked_clock <= 0)
		locked_clock = gpex_clock_get_min_clock();
	gpex_dvfs_spin_unlock(&flags);

	return locked_clock;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_min_lock);

int exynos_stats_set_queued_threshold_0(uint32_t threshold)
{
	gpex_tsg_set_queued_threshold(0, threshold);
	return gpex_tsg_get_queued_threshold(0);
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_0);

int exynos_stats_set_queued_threshold_1(uint32_t threshold)
{
	gpex_tsg_set_queued_threshold(1, threshold);
	return gpex_tsg_get_queued_threshold(1);
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_1);

ktime_t *exynos_stats_get_gpu_queued_job_time(void)
{
	int i;
	for (i = 0; i < 2; i++) {
		gpex_tsg_set_queued_time(i, gpex_tsg_get_queued_time_tick(i));
	}
	return gpex_tsg_get_queued_time_array();
}
EXPORT_SYMBOL(exynos_stats_get_gpu_queued_job_time);

ktime_t exynos_stats_get_gpu_queued_last_updated(void)
{
	return gpex_tsg_get_queued_last_updated();
}
EXPORT_SYMBOL(exynos_stats_get_gpu_queued_last_updated);

void exynos_stats_set_gpu_polling_speed(int polling_speed)
{
	gpex_dvfs_set_polling_speed(polling_speed);
}
EXPORT_SYMBOL(exynos_stats_set_gpu_polling_speed);

int exynos_stats_get_gpu_polling_speed(void)
{
	return gpex_dvfs_get_polling_speed();
}
EXPORT_SYMBOL(exynos_stats_get_gpu_polling_speed);

void exynos_migov_set_mode(int mode)
{
	gpex_tsg_set_migov_mode(mode);
}
EXPORT_SYMBOL(exynos_migov_set_mode);

void exynos_migov_set_gpu_margin(int margin)
{
	gpex_tsg_set_freq_margin(margin);
}
EXPORT_SYMBOL(exynos_migov_set_gpu_margin);

int register_frag_utils_change_notifier(struct notifier_block *nb)
{
	int ret = 0;
	ret = atomic_notifier_chain_register(gpex_tsg_get_frag_utils_change_notifier_list(), nb);
	return ret;
}
EXPORT_SYMBOL(register_frag_utils_change_notifier);

int unregister_frag_utils_change_notifier(struct notifier_block *nb)
{
	int ret = 0;
	ret = atomic_notifier_chain_unregister(gpex_tsg_get_frag_utils_change_notifier_list(), nb);
	return ret;
}
EXPORT_SYMBOL(unregister_frag_utils_change_notifier);

/* TODO: this sysfs function use external fucntion. */
/* Actually, Using external function in internal module is not ideal with the Refactoring rules */
/* So, if we can modify outer modules such as 'migov, cooling, ...' in the future, */
/* fix it following the rules*/
static ssize_t show_feedback_governor_impl(char *buf)
{
	ssize_t ret = 0;
	int i;
	uint32_t *freqs;
	uint32_t *volts;
	ktime_t *time_in_state;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "feedback governer implementation\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_table_size(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "     +- int gpu_dvfs_get_step(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n",
			exynos_stats_get_gpu_table_size());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_cur_idx(void)\n");

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "     +- int gpu_dvfs_get_cur_level(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- clock=%u\n",
			gpex_clock_get_cur_clock());
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- level=%d\n",
			exynos_stats_get_gpu_cur_idx());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_coeff(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- int gpu_dvfs_get_coefficient_value(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n",
			exynos_stats_get_gpu_coeff());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int *exynos_stats_get_gpu_freq_table(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- unsigned int *gpu_dvfs_get_freqs(void)\n");
	freqs = exynos_stats_get_gpu_freq_table();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n", freqs[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int *exynos_stats_get_gpu_volt_table(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- unsigned int *gpu_dvfs_get_volts(void)\n");
	volts = exynos_stats_get_gpu_volt_table();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n", volts[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- ktime_t *exynos_stats_get_gpu_time_in_state(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- ktime_t *gpu_dvfs_get_time_in_state(void)\n");
	time_in_state = exynos_stats_get_gpu_time_in_state();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n", time_in_state[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- ktime_t *exynos_stats_get_gpu_queued_job_time(void)\n");
	time_in_state = exynos_stats_get_gpu_queued_job_time();
	for (i = 0; i < 2; i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n", time_in_state[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret, " +- queued_threshold_check\n");
	for (i = 0; i < 2; i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n",
				gpex_tsg_get_queued_threshold(i));
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- int exynos_stats_get_gpu_polling_speed(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %d\n",
			exynos_stats_get_gpu_polling_speed());

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_feedback_governor_impl);

int gpex_tsg_external_init(struct _tsg_info *_tsg_info)
{
	tsg_info = _tsg_info;
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(feedback_governor_impl, show_feedback_governor_impl);
	return 0;
}

int gpex_tsg_external_term(void)
{
	tsg_info = (void *)0;

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <linux/notifier.h>
#include <linux/ktime.h>

#include <gpex_tsg.h>
#include <gpex_dvfs.h>
#include <gpex_utils.h>
#include <gpex_pm.h>
#include <gpex_ifpo.h>
#include <gpex_clock.h>
#include <gpexbe_pm.h>
#include <gpex_cmar_sched.h>

#include <soc/samsung/exynos-migov.h>
#include <soc/samsung/exynos-profiler.h>

#include "gpex_tsg_internal.h"

#define DVFS_TABLE_ROW_MAX 20

static struct _tsg_info *tsg_info;

unsigned long exynos_stats_get_job_state_cnt(void)
{
	return tsg_info->input_job_nr_acc;
}
EXPORT_SYMBOL(exynos_stats_get_job_state_cnt);

int exynos_stats_get_gpu_cur_idx(void)
{
	int i;
	int level = -1;
	int clock = 0;
	int idx_max_clk, idx_min_clk;

	if (gpexbe_pm_get_exynos_pm_domain()) {
		if (gpexbe_pm_get_status() == 1) {
			clock = gpex_clock_get_cur_clock();
		} else {
			GPU_LOG(MALI_EXYNOS_ERROR, "%s: can't get dvfs cur clock\n", __func__);
			clock = 0;
		}
	} else {
		if (gpex_pm_get_status(true) == 1) {
			if (gpex_ifpo_get_mode() && !gpex_ifpo_get_status()) {
				GPU_LOG(MALI_EXYNOS_ERROR, "%s: can't get dvfs cur clock\n",
					__func__);
				clock = 0;
			} else {
				clock = gpex_clock_get_cur_clock();
			}
		}
	}

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. max_clock_level : %d, min_clock_level : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return -1;
	}

	if (clock == 0)
		return idx_min_clk - idx_max_clk;

	for (i = idx_max_clk; i <= idx_min_clk; i++) {
		if (gpex_clock_get_clock(i) == clock) {
			level = i;
			break;
		}
	}

	return (level - idx_max_clk);
}
EXPORT_SYMBOL(exynos_stats_get_gpu_cur_idx);

int exynos_stats_get_gpu_coeff(void)
{
	int coef = 6144;

	return coef;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_coeff);

uint32_t exynos_stats_get_gpu_table_size(void)
{
	return (gpex_clock_get_table_idx(gpex_clock_get_min_clock()) -
		gpex_clock_get_table_idx(gpex_clock_get_max_clock()) + 1);
}
EXPORT_SYMBOL(exynos_stats_get_gpu_table_size);

static uint32_t freqs[DVFS_TABLE_ROW_MAX];
uint32_t *gpu_dvfs_get_freq_table(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return freqs;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++)
		freqs[i - idx_max_clk] = (uint32_t)gpex_clock_get_clock(i);

	return freqs;
}
EXPORT_SYMBOL(gpu_dvfs_get_freq_table);

static uint32_t volts[DVFS_TABLE_ROW_MAX];
uint32_t *exynos_stats_get_gpu_volt_table(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return volts;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++)
		volts[i - idx_max_clk] = (uint32_t)gpex_clock_get_voltage(gpex_clock_get_clock(i));

	return volts;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_volt_table);

static ktime_t time_in_state[DVFS_TABLE_ROW_MAX];
ktime_t tis_last_update;
ktime_t *gpu_dvfs_get_time_in_state(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return time_in_state;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++) {
		time_in_state[i - idx_max_clk] =
			ms_to_ktime((u64)(gpex_clock_get_time_busy(i) * 4) / 100);
	}

	return time_in_state;
}
EXPORT_SYMBOL(gpu_dvfs_get_time_in_state);

ktime_t gpu_dvfs_get_tis_last_update(void)
{
	return (ktime_t)(gpex_clock_get_time_in_state_last_update());
}
EXPORT_SYMBOL(gpu_dvfs_get_tis_last_update);

int exynos_stats_set_queued_threshold_0(uint32_t threshold)
{
	gpex_tsg_set_queued_threshold(0, threshold);
	return gpex_tsg_get_queued_threshold(0);
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_0);

int exynos_stats_set_queued_threshold_1(uint32_t threshold)
{
	gpex_tsg_set_queued_threshold(1, threshold);

	return gpex_tsg_get_queued_threshold(1);
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_1);

ktime_t *gpu_dvfs_get_job_queue_count(void)
{
	int i;
	for (i = 0; i < 2; i++) {
		gpex_tsg_set_queued_time(i, gpex_tsg_get_queued_time_tick(i));
	}
	return gpex_tsg_get_queued_time_array();
}
EXPORT_SYMBOL(gpu_dvfs_get_job_queue_count);

ktime_t gpu_dvfs_get_job_queue_last_updated(void)
{
	return gpex_tsg_get_queued_last_updated();
}
EXPORT_SYMBOL(gpu_dvfs_get_job_queue_last_updated);

void exynos_stats_set_gpu_polling_speed(int polling_speed)
{
	gpex_dvfs_set_polling_speed(polling_speed);
}
EXPORT_SYMBOL(exynos_stats_set_gpu_polling_speed);

int exynos_stats_get_gpu_polling_speed(void)
{
	return gpex_dvfs_get_polling_speed();
}
EXPORT_SYMBOL(exynos_stats_get_gpu_polling_speed);

void gpu_dvfs_set_amigo_governor(int mode)
{
	gpex_tsg_set_migov_mode(mode);

	if (mode)
		gpex_cmar_sched_set_forced_sched(1);
	else
		gpex_cmar_sched_set_forced_sched(0);
}
EXPORT_SYMBOL(gpu_dvfs_set_amigo_governor);

void gpu_dvfs_set_freq_margin(int margin)
{
	gpex_tsg_set_freq_margin(margin);
}
EXPORT_SYMBOL(gpu_dvfs_set_freq_margin);

void exynos_stats_get_run_times(u64 *times)
{
	gpex_tsg_stats_get_run_times(times);
}
EXPORT_SYMBOL(exynos_stats_get_run_times);

void exynos_stats_get_pid_list(u16 *pidlist)
{
	gpex_tsg_stats_get_pid_list(pidlist);
}
EXPORT_SYMBOL(exynos_stats_get_pid_list);

void exynos_stats_set_vsync(ktime_t ktime_us)
{
	gpex_tsg_stats_set_vsync(ktime_us);
}
EXPORT_SYMBOL(exynos_stats_set_vsync);

void exynos_stats_get_frame_info(s32 *nrframe, u64 *nrvsync, u64 *delta_ms)
{
	gpex_tsg_stats_get_frame_info(nrframe, nrvsync, delta_ms);
}
EXPORT_SYMBOL(exynos_stats_get_frame_info);

void exynos_migov_set_targetframetime(int us)
{
	gpex_tsg_migov_set_targetframetime(us);
}
EXPORT_SYMBOL(exynos_migov_set_targetframetime);

void exynos_migov_set_targettime_margin(int us)
{
	gpex_tsg_migov_set_targettime_margin(us);
}
EXPORT_SYMBOL(exynos_migov_set_targettime_margin);

void exynos_migov_set_util_margin(int percentage)
{
	gpex_tsg_migov_set_util_margin(percentage);
}
EXPORT_SYMBOL(exynos_migov_set_util_margin);

void exynos_migov_set_decon_time(int us)
{
	gpex_tsg_migov_set_decon_time(us);
}
EXPORT_SYMBOL(exynos_migov_set_decon_time);

void exynos_migov_set_comb_ctrl(int val)
{
	gpex_tsg_migov_set_comb_ctrl(val);
}
EXPORT_SYMBOL(exynos_migov_set_comb_ctrl);

void exynos_sdp_set_powertable(int id, int cnt, struct freq_table *table)
{
	gpex_tsg_sdp_set_powertable(id, cnt, table);
}
EXPORT_SYMBOL(exynos_sdp_set_powertable);

void exynos_sdp_set_busy_domain(int id)
{
	gpex_tsg_sdp_set_busy_domain(id);
}
EXPORT_SYMBOL(exynos_sdp_set_busy_domain);

void exynos_sdp_set_cur_freqlv(int id, int idx)
{
	gpex_tsg_sdp_set_cur_freqlv(id, idx);
}
EXPORT_SYMBOL(exynos_sdp_set_cur_freqlv);

int exynos_gpu_stc_config_show(int page_size, char *buf)
{
	return gpex_tsg_stc_config_show(page_size, buf);
}
EXPORT_SYMBOL(exynos_gpu_stc_config_show);

int exynos_gpu_stc_config_store(const char *buf)
{
	return gpex_tsg_stc_config_store(buf);
}
EXPORT_SYMBOL(exynos_gpu_stc_config_store);

int gpu_dvfs_register_utilization_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(gpex_tsg_get_frag_utils_change_notifier_list(), nb);
}
EXPORT_SYMBOL(gpu_dvfs_register_utilization_notifier);

int gpu_dvfs_unregister_utilization_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(gpex_tsg_get_frag_utils_change_notifier_list(), nb);
}
EXPORT_SYMBOL(gpu_dvfs_unregister_utilization_notifier);

/* TODO: this sysfs function use external fucntion. */
/* Actually, Using external function in internal module is not ideal with the Refactoring rules */
/* So, if we can modify outer modules such as 'migov, cooling, ...' in the future, */
/* fix it following the rules*/
static ssize_t show_feedback_governor_impl(char *buf)
{
	ssize_t ret = 0;
	int i;
	uint32_t *freqs;
	uint32_t *volts;
	ktime_t *time_in_state;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "feedback governer implementation\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_table_size(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "     +- int gpu_dvfs_get_step(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n",
			exynos_stats_get_gpu_table_size());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_cur_idx(void)\n");

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "     +- int gpu_dvfs_get_cur_level(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- clock=%u\n",
			gpex_clock_get_cur_clock());
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- level=%d\n",
			exynos_stats_get_gpu_cur_idx());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_coeff(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- int gpu_dvfs_get_coefficient_value(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n",
			exynos_stats_get_gpu_coeff());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int *gpu_dvfs_get_freq_table(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- uint32_t *gpu_dvfs_get_freqs(void)\n");
	freqs = gpu_dvfs_get_freq_table();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n", freqs[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int *exynos_stats_get_gpu_volt_table(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- unsigned int *gpu_dvfs_get_volts(void)\n");
	volts = exynos_stats_get_gpu_volt_table();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n", volts[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- ktime_t *gpu_dvfs_get_time_in_state(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- ktime_t *gpu_dvfs_get_time_in_state(void)\n");
	time_in_state = gpu_dvfs_get_time_in_state();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n", time_in_state[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- ktime_t *gpu_dvfs_get_job_queue_count(void)\n");
	time_in_state = gpu_dvfs_get_job_queue_count();
	for (i = 0; i < 2; i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n", time_in_state[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret, " +- queued_threshold_check\n");
	for (i = 0; i < 2; i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n",
				gpex_tsg_get_queued_threshold(i));
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- int exynos_stats_get_gpu_polling_speed(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %d\n",
			exynos_stats_get_gpu_polling_speed());

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_feedback_governor_impl);

int gpex_tsg_external_init(struct _tsg_info *_tsg_info)
{
	tsg_info = _tsg_info;
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(feedback_governor_impl, show_feedback_governor_impl);
	return 0;
}

int gpex_tsg_external_term(void)
{
	tsg_info = (void *)0;

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#ifndef _GPEX_TSG_INTERNAL_H_
#define _GPEX_TSG_INTERNAL_H_
#include <linux/ktime.h>

struct _tsg_info {
	struct kbase_device *kbdev;
	struct {
		int util_history[2][WINDOW];
		int freq_history[WINDOW];
		int average_util;
		int average_freq;
		int diff_util;
		int diff_freq;
		int weight_util[2];
		int weight_freq;
		int next_util;
		int next_freq;
		int pre_util;
		int pre_freq;
		bool en_signal;
	} prediction;

	struct {
		/* job queued time, index represents queued_time each threshold */
		uint32_t queued_threshold[2];
		ktime_t queued_time_tick[2];
		ktime_t queued_time[2];
		ktime_t last_updated;
		atomic_t nr[3]; /* Legacy: job_nr, in_nr, out_nr */
	} queue;

	uint64_t input_job_nr_acc;
	int input_job_nr;

	int freq_margin;
	int migov_mode;
	int weight_table_idx_0;
	int weight_table_idx_1;
	int migov_saved_polling_speed;
	bool is_pm_qos_tsg;
	int amigo_flags;

	int governor_type_init;
	int is_gov_set;

	/* GPU Profiler */
	int nr_q;
	int lastshowidx;
	ktime_t prev_swap_timestamp;
	ktime_t first_job_timestamp;
	ktime_t gpu_timestamps[3];
	u32 js_occupy;
	ktime_t lastjob_starttimestamp;
	ktime_t sum_jobs_time;
	ktime_t last_jobs_time;

	int vsync_interval;
	/* End for GPU Profiler */

	raw_spinlock_t spinlock;
	struct atomic_notifier_head frag_utils_change_notifier_list;
};

struct amigo_freq_estpower {
	int freq;
	int power;
};

int gpex_tsg_external_init(struct _tsg_info *_tsg_info);
int gpex_tsg_sysfs_init(struct _tsg_info *_tsg_info);

int gpex_tsg_external_term(void);
int gpex_tsg_sysfs_term(void);

void gpex_tsg_inc_rtp_vsync_swapcall_counter(void);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <gpex_tsg.h>
#include <gpex_utils.h>

#include "gpex_tsg_internal.h"

static struct _tsg_info *tsg_info;

static ssize_t show_weight_table_idx_0(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_weight_table_idx(0));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_weight_table_idx_0);

static ssize_t show_weight_table_idx_1(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_weight_table_idx(1));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_weight_table_idx_1);

static ssize_t show_queued_threshold_0(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_queued_threshold(0));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_queued_threshold_0);

static ssize_t show_queued_threshold_1(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_queued_threshold(1));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_queued_threshold_1);

static ssize_t set_weight_table_idx_0(const char *buf, size_t count)
{
	int ret, table_idx_0;
	ret = kstrtoint(buf, 0, &table_idx_0);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_tsg_set_weight_table_idx(0, table_idx_0);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_weight_table_idx_0);

static ssize_t set_weight_table_idx_1(const char *buf, size_t count)
{
	int ret, table_idx_1;
	ret = kstrtoint(buf, 0, &table_idx_1);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_tsg_set_weight_table_idx(1, table_idx_1);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_weight_table_idx_1);

static ssize_t set_queued_threshold_0(const char *buf, size_t count)
{
	unsigned int threshold = 0;
	int ret;

	ret = kstrtoint(buf, 0, &threshold);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}
	gpex_tsg_set_queued_threshold(0, threshold);

	GPU_LOG(MALI_EXYNOS_ERROR, "%s: queued_threshold_0 = %d\n", __func__,
		gpex_tsg_get_queued_threshold(0));

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_queued_threshold_0);

static ssize_t set_queued_threshold_1(const char *buf, size_t count)
{
	unsigned int threshold = 0;
	int ret;

	ret = kstrtoint(buf, 0, &threshold);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_tsg_set_queued_threshold(1, threshold);

	GPU_LOG(MALI_EXYNOS_ERROR, "%s: queued_threshold_1 = %d\n", __func__,
		gpex_tsg_get_queued_threshold(1));

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_queued_threshold_1);

int gpex_tsg_sysfs_init(struct _tsg_info *_tsg_info)
{
	tsg_info = _tsg_info;

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(weight_table_idx_0, show_weight_table_idx_0,
					 set_weight_table_idx_0);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(weight_table_idx_1, show_weight_table_idx_1,
					 set_weight_table_idx_1);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(queued_threshold_0, show_queued_threshold_0,
					 set_queued_threshold_0);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(queued_threshold_1, show_queued_threshold_1,
					 set_queued_threshold_1);

	return 0;
}

int gpex_tsg_sysfs_term(void)
{
	tsg_info = (void *)0;

	return 0;
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <gpex_tsg.h>
#include <gpex_utils.h>

#include "gpex_tsg_internal.h"

static struct _tsg_info *tsg_info;

static ssize_t show_weight_table_idx_0(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_weight_table_idx(0));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_weight_table_idx_0);

static ssize_t show_weight_table_idx_1(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_weight_table_idx(1));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_weight_table_idx_1);

static ssize_t show_queued_threshold_0(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_queued_threshold(0));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_queued_threshold_0);

static ssize_t show_queued_threshold_1(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_queued_threshold(1));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_queued_threshold_1);

static ssize_t show_egp_profile(char *buf)
{
	struct amigo_interframe_data *dst;
	ssize_t count = 0;
	int id = 0;
	int target_frametime = gpex_tsg_amigo_get_target_frametime();

	while ((dst = gpex_tsg_amigo_get_next_frameinfo()) != NULL) {
		if (dst->nrq > 0) {
			ktime_t avg_pre = dst->sum_pre / dst->nrq;
			ktime_t avg_cpu = dst->sum_cpu / dst->nrq;
			ktime_t avg_v2s = dst->sum_v2s / dst->nrq;
			ktime_t avg_gpu = dst->sum_gpu / dst->nrq;
			ktime_t avg_v2f = dst->sum_v2f / dst->nrq;

			count += scnprintf(buf + count, PAGE_SIZE - count,
				"%4d, %6llu, %3u, %6lu,%6lu, %6lu,%6lu, %6lu,%6lu, %6lu,%6lu, %6lu,%6lu, %6lu,%6lu, %6d, %d, %7d,%7d, %7d,%7d\n",
				id++, dst->vsync_interval, dst->nrq,
				avg_pre, dst->max_pre,
				avg_cpu, dst->max_cpu,
				avg_v2s, dst->max_v2s,
				avg_gpu, dst->max_gpu,
				avg_v2f, dst->max_v2f,
				dst->cputime, dst->gputime,
				target_frametime, dst->sdp_next_cpuid,
				dst->sdp_cur_fcpu, dst->sdp_cur_fgpu,
				dst->sdp_next_fcpu, dst->sdp_next_fgpu);
		}
	}

	return count;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_egp_profile);

static ssize_t set_weight_table_idx_0(const char *buf, size_t count)
{
	int ret, table_idx_0;
	ret = kstrtoint(buf, 0, &table_idx_0);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_tsg_set_weight_table_idx(0, table_idx_0);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_weight_table_idx_0);

static ssize_t set_weight_table_idx_1(const char *buf, size_t count)
{
	int ret, table_idx_1;
	ret = kstrtoint(buf, 0, &table_idx_1);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_tsg_set_weight_table_idx(1, table_idx_1);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_weight_table_idx_1);

static ssize_t set_queued_threshold_0(const char *buf, size_t count)
{
	unsigned int threshold = 0;
	int ret;

	ret = kstrtoint(buf, 0, &threshold);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}
	gpex_tsg_set_queued_threshold(0, threshold);

	GPU_LOG(MALI_EXYNOS_ERROR, "%s: queued_threshold_0 = %d\n", __func__,
		gpex_tsg_get_queued_threshold(0));

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_queued_threshold_0);

static ssize_t set_queued_threshold_1(const char *buf, siz