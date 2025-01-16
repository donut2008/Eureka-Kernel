GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_CLOCK_VALUE, clk, clk_info.cur_clock,
			 "clock set: %d, clock get: %d\n", clk, clk_info.cur_clock);

err:
	gpex_pm_unlock();

	return ret;
}

int gpex_get_valid_gpu_clock(int clock, bool is_round_up)
{
	int i, min_clock_idx, max_clock_idx;

	/* Zero Value for Unlock*/
	if (clock == 0)
		return 0;

	min_clock_idx = gpex_clock_get_table_idx(gpex_clock_get_min_clock());
	max_clock_idx = gpex_clock_get_table_idx(gpex_clock_get_max_clock());

	if ((clock - gpex_clock_get_min_clock()) < 0)
		return clk_info.table[min_clock_idx].clock;

	if (is_round_up) {
		/* Round Up if min lock sequence */
		/* ex) invalid input 472Mhz -> valid min lock 400Mhz?500Mhz? -> min lock = 500Mhz */
		for (i = min_clock_idx; i >= max_clock_idx; i--)
			if (clock - (int)(clk_info.table[i].clock) <= 0)
				return clk_info.table[i].clock;
	} else {
		/* Round Down if max lock sequence. */
		/* ex) invalid input 472Mhz -> valid max lock 400Mhz?500Mhz? -> max lock = 400Mhz */
		for (i = max_clock_idx; i <= min_clock_idx; i++)
			if (clock - (int)(clk_info.table[i].clock) >= 0)
				return clk_info.table[i].clock;
	}

	return -1;
}

int gpex_clock_update_time_in_state(int clock)
{
	u64 current_time;
	int level = gpex_clock_get_table_idx(clock);

	if (clk_info.prev_time_in_state_time == 0)
		clk_info.prev_time_in_state_time = get_jiffies_64();

	current_time = get_jiffies_64();
	if ((level >= gpex_clock_get_table_idx(clk_info.gpu_max_clock)) &&
	    (level <= gpex_clock_get_table_idx(clk_info.gpu_min_clock))) {
		clk_info.table[level].time += current_time - clk_info.prev_time_in_state_time;
		clk_info.table[level].time_busy +=
			(u64)((current_time - clk_info.prev_time_in_state_time)
					* gpexbe_utilization_get_utilization());
		GPU_LOG(MALI_EXYNOS_DEBUG,
			"%s: util = %d cur_clock[%d] = %d time_busy[%d] = %llu(%llu)\n", __func__,
			gpexbe_utilization_get_utilization(), level, clock, level,
			clk_info.table[level].time_busy / 100, clk_info.table[level].time);
	}

	clk_info.prev_time_in_state_time = current_time;

	return 0;
}

u64 gpex_clock_get_time_in_state_last_update(void)
{
	return clk_info.prev_time_in_state_time;
}

static int gpex_clock_set_helper(int clock)
{
	int ret = 0;
	bool is_up = false;
	static int prev_clock = -1;
	int clk_idx = 0;

	if (gpex_ifpo_get_status() == IFPO_POWER_DOWN) {
		GPU_LOG(MALI_EXYNOS_INFO,
			"%s: can't set clock in the ifpo mode (requested clock %d)\n", __func__,
			clock);
		return 0;
	}

	clk_idx = gpex_clock_get_table_idx(clock);
	if (clk_idx < 0) {
		GPU_LOG(MALI_EXYNOS_DEBUG, "%s: mismatch clock error (%d)\n", __func__, clock);
		return -1;
	}

	is_up = prev_clock < clock;

	/* TODO: is there a need to set PMQOS before or after setting gpu clock?
	 * Why not move this call so pmqos is set in set_clock_using_calapi ?
	 */
	/* TODO: better range checking for platform step */
	if (is_up)
		gpex_qos_set_from_clock(clock);

	set_clock_using_calapi(clock);

	if (!is_up)
		gpex_qos_set_from_clock(clock);

	gpex_clock_update_time_in_state(prev_clock);
	prev_clock = clock;

	return ret;
}

int gpex_clock_init_time_in_state(void)
{
	int i;
	int max_clk_idx = gpex_clock_get_table_idx(clk_info.gpu_max_clock);
	int min_clk_idx = gpex_clock_get_table_idx(clk_info.gpu_min_clock);

	for (i = max_clk_idx; i <= min_clk_idx; i++) {
		clk_info.table[i].time = 0;
		clk_info.table[i].time_busy = 0;
	}

	return 0;
}

static int gpu_check_target_clock(int clock)
{
	int target_clock = clock;

	if (gpex_clock_get_table_idx(target_clock) < 0)
		return -1;

	if (!gpex_dvfs_get_status())
		return target_clock;

	GPU_LOG(MALI_EXYNOS_DEBUG, "clock: %d, min: %d, max: %d\n", clock, clk_info.min_lock,
		clk_info.max_lock);

	if ((clk_info.min_lock > 0) && (gpex_pm_get_power_status()) &&
	    ((target_clock < clk_info.min_lock) || (clk_info.cur_clock < clk_info.min_lock)))
		target_clock = clk_info.min_lock;

	if ((clk_info.max_lock > 0) && (target_clock > clk_info.max_lock))
		target_clock = clk_info.max_lock;

	/* TODO: I don't think this required as it is set in gpex_dvfs_set_clock_callback */
	//gpex_dvfs_set_step(gpex_clock_get_table_idx(target_clock));

	return target_clock;
}

/*******************************************************
 * Interfaced functions
 ******************************************************/
int gpex_clock_init(struct device **dev)
{
	int i = 0;

	mutex_init(&clk_info.clock_lock);
	clk_info.kbdev = container_of(dev, struct kbase_device, dev);
	clk_info.max_lock = 0;
	clk_info.min_lock = 0;
	clk_info.unlock_freqs = false;

	for (i = 0; i < NUMBER_LOCK; i++) {
		clk_info.user_max_lock[i] = 0;
		clk_info.user_min_lock[i] = 0;
	}

	gpex_clock_update_config_data_from_dt();
	gpex_clock_init_time_in_state();
	gpex_clock_sysfs_init(&clk_info);

	gpex_utils_get_exynos_context()->clk_info = &clk_info;

	/* TODO: return proper error when error */
	return 0;
}

void gpex_clock_term(void)
{
	/* TODO: reset other clk_info variables too */
	clk_info.kbdev = NULL;
}

int gpex_clock_get_table_idx(int clock)
{
	int i;

	if (clock < clk_info.gpu_min_clock)
		return -1;

	for (i = 0; i < clk_info.table_size; i++) {
		if (clk_info.table[i].clock == clock)
			return i;
	}

	return -1;
}

int gpex_clock_get_clock_slow(void)
{
	return gpexbe_clock_get_rate();
}

int gpex_clock_set(int clk)
{
	int ret = 0, target_clk = 0;
	int prev_clk = 0;

	if (!gpex_pm_get_status(true)) {
		GPU_LOG(MALI_EXYNOS_INFO,
			"%s: can't set clock and voltage in the power-off state!\n", __func__);
		return -1;
	}

	mutex_lock(&clk_info.clock_lock);

	if (!gpex_pm_get_power_status()) {
		mutex_unlock(&clk_info.clock_lock);
		GPU_LOG(MALI_EXYNOS_INFO, "%s: can't control clock and voltage in power off %d\n",
			__func__, gpex_pm_get_power_status());
		return 0;
	}

	target_clk = gpu_check_target_clock(clk);
	if (target_clk < 0) {
		mutex_unlock(&clk_info.clock_lock);
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: mismatch clock error (source %d, target %d)\n",
			__func__, clk, target_clk);
		return -1;
	}

	gpex_pm_lock();

	if (gpex_pm_get_status(false))
		prev_clk = gpex_clock_get_clock_slow();

	gpex_pm_unlock();

	/* QOS is set here */
	gpex_clock_set_helper(target_clk);

	ret = gpex_dvfs_set_clock_callback();

	mutex_unlock(&clk_info.clock_lock);

	GPU_LOG(MALI_EXYNOS_DEBUG, "clk[%d -> %d]\n", prev_clk, target_clk);

	return ret;
}

int gpex_clock_prepare_runtime_off(void)
{
	gpex_clock_update_time_in_state(clk_info.cur_clock);

	return 0;
}

int gpex_clock_lock_clock(gpex_clock_lock_cmd_t lock_command, gpex_clock_lock_type_t lock_type,
			  int clock)
{
	int i;
	bool dirty = false;
	unsigned long flags;
	int max_lock_clk = 0;
	int valid_clock = 0;

	/* TODO: there's no need to check dvfs status anymore since dvfs and clock setting is separate */
	//if (!gpex_dvfs_get_status())
	//	return 0;

	if ((lock_type < TMU_LOCK) || (lock_type >= NUMBER_LOCK)) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: invalid lock type is called (%d)\n", __func__,
			lock_type);
		return -1;
	}

	valid_clock = clock;

	switch (lock_command) {
	case GPU_CLOCK_MAX_LOCK:
		gpex_dvfs_spin_lock(&flags);
		if (gpex_clock_get_table_idx(clock) < 0) {
			valid_clock = gpex_get_valid_gpu_clock(clock, false);
			if (valid_clock < 0) {
				gpex_dvfs_spin_unlock(&flags);
				GPU_LOG(MALI_EXYNOS_ERROR,
					"clock locking(type:%d) error: invalid clock value %d \n",
					lock_command, clock);
				return -1;
			}
			GPU_LOG(MALI_EXYNOS_DEBUG, "clock is changed to valid value[%d->%d]", clock,
				valid_clock);
		}
		clk_info.user_max_lock[lock_type] = valid_clock;
		clk_info.max_lock = valid_clock;

		if (clk_info.max_lock > 0) {
			for (i = 0; i < NUMBER_LOCK; i++) {
				if (clk_info.user_max_lock[i] > 0)
					clk_info.max_lock =
						MIN(clk_info.max_lock, clk_info.user_max_lock[i]);
			}
		} else {
			clk_info.max_lock = valid_clock;
		}

		gpex_dvfs_spin_unlock(&flags);

		if ((clk_info.max_lock > 0) && (gpex_clock_get_cur_clock() >= clk_info.max_lock))
			gpex_clock_set(clk_info.max_lock);

		GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_GPU_MAX_LOCK, lock_type, clock,
				 "lock max clk[%d], user lock[%d], current clk[%d]\n",
				 clk_info.max_lock, clk_info.user_max_lock[lock_type],
				 gpex_clock_get_cur_clock());
		break;
	case GPU_CLOCK_MIN_LOCK:
		gpex_dvfs_spin_lock(&flags);
		if (gpex_clock_get_table_idx(clock) < 0) {
			valid_clock = gpex_get_valid_gpu_clock(clock, true);
			if (valid_clock < 0) {
				gpex_dvfs_spin_unlock(&flags);
				GPU_LOG(MALI_EXYNOS_ERROR,
					"clock locking(type:%d) error: invalid clock value %d \n",
					lock_command, clock);
				return -1;
			}
			GPU_LOG(MALI_EXYNOS_DEBUG, "clock is changed to valid value[%d->%d]", clock,
				valid_clock);
		}
		clk_info.user_min_lock[lock_type] = valid_clock;
		clk_info.min_lock = valid_clock;

		if (clk_info.min_lock > 0) {
			for (i = 0; i < NUMBER_LOCK; i++) {
				if (clk_info.user_min_lock[i] > 0)
					clk_info.min_lock =
						MAX(clk_info.min_lock, clk_info.user_min_lock[i]);
			}
		} else {
			clk_info.min_lock = valid_clock;
		}

		max_lock_clk =
			(clk_info.max_lock == 0) ? gpex_clock_get_max_clock() : clk_info.max_lock;

		gpex_dvfs_spin_unlock(&flags);

		if ((clk_info.min_lock > 0) && (gpex_clock_get_cur_clock() < clk_info.min_lock) &&
		    (clk_info.min_lock <= max_lock_clk))
			gpex_clock_set(clk_info.min_lock);

		GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_GPU_MIN_LOCK, lock_type, clock,
				 "lock min clk[%d], user lock[%d], current clk[%d]\n",
				 clk_info.min_lock, clk_info.user_min_lock[lock_type],
				 gpex_clock_get_cur_clock());
		break;
	case GPU_CLOCK_MAX_UNLOCK:
		gpex_dvfs_spin_lock(&flags);
		clk_info.user_max_lock[lock_type] = 0;
		clk_info.max_lock = gpex_clock_get_max_clock();

		for (i = 0; i < NUMBER_LOCK; i++) {
			if (clk_info.user_max_lock[i] > 0) {
				dirty = true;
				clk_info.max_lock =
					MIN(clk_info.user_max_lock[i], clk_info.max_lock);
			}
		}

		if (!dirty)
			clk_info.max_lock = 0;

		gpex_dvfs_spin_unlock(&flags);
		GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_GPU_MAX_LOCK, lock_type, clock,
				 "unlock max clk\n");
		break;
	case GPU_CLOCK_MIN_UNLOCK:
		gpex_dvfs_spin_lock(&flags);
		clk_info.user_min_lock[lock_type] = 0;
		clk_info.min_lock = gpex_clock_get_min_clock();

		for (i = 0; i < NUMBER_LOCK; i++) {
			if (clk_info.user_min_lock[i] > 0) {
				dirty = true;
				clk_info.min_lock =
					MAX(clk_info.user_min_lock[i], clk_info.min_lock);
			}
		}

		if (!dirty)
			clk_info.min_lock = 0;

		gpex_dvfs_spin_unlock(&flags);
		GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_GPU_MIN_LOCK, lock_type, clock,
				 "unlock min clk\n");
		break;
	default:
		break;
	}

	return 0;
}

void gpex_clock_mutex_lock(void)
{
	mutex_lock(&clk_info.clock_lock);
}

void gpex_clock_mutex_unlock(void)
{
	mutex_unlock(&clk_info.clock_lock);
}

int gpex_clock_get_voltage(int clk)
{
	int idx = gpex_clock_get_table_idx(clk);

	if (idx >= 0 && idx < clk_info.table_size)
		return clk_info.table[idx].voltage;
	else {
		/* TODO: print error msg */
		return -EINVAL;
	}
}

void gpex_clock_set_user_min_lock_input(int clock)
{
	clk_info.user_min_lock_input = clock;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /* SPDX-License-Identifier: GPL-2.0 */

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

#ifndef _MALI_EXYNOS_CLOCK_INTERNAL_H_
#define _MALI_EXYNOS_CLOCK_INTERNAL_H_

#include <gpex_utils.h>
#include <gpex_clock.h>

typedef struct _gpu_clock_info {
	unsigned int clock;
	unsigned int voltage;
	u64 time;
	u64 time_busy;
} gpu_clock_info;

struct _clock_info {
	struct kbase_device *kbdev;
	int gpu_max_clock;
	int gpu_min_clock;
	int gpu_max_clock_limit;
	int boot_clock; // was known as gpu_dvfs_start_clock
	int cur_clock;
	gpu_clock_info *table;
	int table_size;

	struct mutex clock_lock;

	int max_lock;
	int min_lock;
	int user_max_lock[NUMBER_LOCK];
	int user_min_lock[NUMBER_LOCK];
	int user_max_lock_input;
	int user_min_lock_input;

	u64 prev_time_in_state_time;

	bool unlock_freqs;
};

int gpex_clock_sysfs_init(struct _clock_info *_clk_info);

int gpex_clock_update_time_in_state(int clock);
int gpex_clock_init_time_in_state(void);

#endif /* _MALI_EXYNOS_CLOCK_INTERNAL_H_ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /* SPDX-License-Identifier: GPL-2.0 */

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

#include <gpex_clock.h>
#include <gpex_pm.h>
#include <gpex_dvfs.h>
#include <gpex_utils.h>
#include <gpex_clboost.h>

#include <linux/throttle_limit.h>
#include <linux/regulator/consumer.h>

#include "gpex_clock_internal.h"

static struct _clock_info *clk_info;
static struct regulator *g3d_regulator;

/* for ondemand gov */
unsigned int gpu_up_threshold = 75;
bool gpu_boost = true;
unsigned int gpu_down_threshold = 0;
#define DOWN_THRESHOLD_MARGIN			(25)
#define GPU_MIN_UP_THRESHOLD		(40)
#define GPU_MAX_UP_THRESHOLD		(100)
#define GPU_FREQ_STEP_0			(260)
#define GPU_FREQ_STEP_1			(338)

/*************************************
 * sysfs node functions
 *************************************/
GPEX_STATIC ssize_t show_clock(char *buf)
{
	ssize_t len = 0;
	int clock = 0;

	gpex_pm_lock();
	if (gpex_pm_get_status(false))
		clock = gpex_clock_get_clock_slow();
	gpex_pm_unlock();

	len += snprintf(buf + len, PAGE_SIZE - len, "%d", clock);

	return gpex_utils_sysfs_endbuf(buf, len);
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_clock)
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_clock)

GPEX_STATIC ssize_t set_clock(const char *buf, size_t count)
{
	unsigned int clk = 0;
	int ret;

	ret = kstrtoint(buf, 0, &clk);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if (clk == 0) {
		gpex_dvfs_enable();
	} else {
		gpex_dvfs_disable();
		gpex_clock_set(clk);
	}

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_clock)

GPEX_STATIC int gpu_get_asv_table(char *buf, size_t buf_size)
{
	int i = 0;
	int len = 0;
	int min_clock_idx = 0;

	if (buf == NULL)
		return 0;

	len += snprintf(buf + len, buf_size - len, "GPU, vol\n");

	min_clock_idx = gpex_clock_get_table_idx(clk_info->gpu_min_clock);

	for (i = gpex_clock_get_table_idx(clk_info->gpu_max_clock); i <= min_clock_idx; i++) {
		len += snprintf(buf + len, buf_size - len, "%d, %7d\n", clk_info->table[i].clock,
				clk_info->table[i].voltage);
	}

	return len;
}

GPEX_STATIC ssize_t show_asv_table(char *buf)
{
	ssize_t ret = 0;

	ret += gpu_get_asv_table(buf + ret, (size_t)PAGE_SIZE - ret);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_asv_table)

GPEX_STATIC ssize_t show_time_in_state(char *buf)
{
	ssize_t ret = 0;
	int i;

	gpex_clock_update_time_in_state(gpex_pm_get_status(true) * clk_info->cur_clock);

	for (i = gpex_clock_get_table_idx(clk_info->gpu_min_clock);
	     i >= gpex_clock_get_table_idx(clk_info->gpu_max_clock); i--) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d %llu %llu\n",
				clk_info->table[i].clock, clk_info->table[i].time,
				clk_info->table[i].time_busy / 100);
	}

	if (ret >= PAGE_SIZE - 1) {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_time_in_state)

GPEX_STATIC ssize_t reset_time_in_state(const char *buf, size_t count)
{
	gpex_clock_init_time_in_state();

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(reset_time_in_state)

#define SUSTAINABLE_FREQ 450000 // KHz
GPEX_STATIC ssize_t set_max_lock_dvfs(const char *buf, size_t count)
{
	int ret, clock = 0;

	if (gpex_clock_get_unlock_freqs_status() || sysfs_streq("0", buf)) {
		clk_info->user_max_lock_input = 0;
		gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, SYSFS_LOCK, 0);
	} else {
		ret = kstrtoint(buf, 0, &clock);
		if (ret) {
			GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
			return -ENOENT;
		}

		if (clock < SUSTAINABLE_FREQ)
			clock = SUSTAINABLE_FREQ;

		clk_info->user_max_lock_input = clock;

		clock = gpex_get_valid_gpu_clock(clock, false);

		ret = gpex_clock_get_table_idx(clock);
		if ((ret < gpex_clock_get_table_idx(gpex_clock_get_max_clock())) ||
		    (ret > gpex_clock_get_table_idx(gpex_clock_get_min_clock()))) {
			GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid clock value (%d)\n", __func__,
				clock);
			return -ENOENT;
		}

		if (clock == gpex_clock_get_max_clock())
			gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, SYSFS_LOCK, 0);
		else
			gpex_clock_lock_clock(GPU_CLOCK_MAX_LOCK, SYSFS_LOCK, clock);
	}

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_max_lock_dvfs)
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_max_lock_dvfs)

GPEX_STATIC ssize_t show_max_lock_dvfs(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;
	int user_locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = clk_info->max_lock;
	user_locked_clock = clk_info->user_max_lock_input;
	gpex_dvfs_spin_unlock(&flags);

	if (locked_clock > 0)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d / %d", locked_clock,
				user_locked_clock);
	else
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "-1");

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_max_lock_dvfs)

GPEX_STATIC ssize_t show_max_lock_dvfs_kobj(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = clk_info->max_lock;
	gpex_dvfs_spin_unlock(&flags);

	if (locked_clock > 0)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", locked_clock);
	else
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", clk_info->gpu_max_clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_max_lock_dvfs_kobj)

GPEX_STATIC ssize_t set_min_lock_dvfs(const char *buf, size_t count)
{
	int ret, clock = 0;

	if (gpex_clock_get_unlock_freqs_status() || sysfs_streq("0", buf)) {
		clk_info->user_min_lock_input = 0;
		gpex_clock_lock_clock(GPU_CLOCK_MIN_UNLOCK, SYSFS_LOCK, 0);
	} else {
		ret = kstrtoint(buf, 0, &clock);
		if (ret) {
			GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
			return -ENOENT;
		}

		clk_info->user_min_lock_input = clock;

		clock = gpex_get_valid_gpu_clock(clock, true);

		ret = gpex_clock_get_table_idx(clock);
		if ((ret < gpex_clock_get_table_idx(gpex_clock_get_max_clock())) ||
		    (ret > gpex_clock_get_table_idx(gpex_clock_get_min_clock()))) {
			GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid clock value (%d)\n", __func__,
				clock);
			return -ENOENT;
		}

		if (clock > gpex_clock_get_max_clock_limit())
			clock = gpex_clock_get_max_clock_limit();

		if (clock == gpex_clock_get_min_clock())
			gpex_clock_lock_clock(GPU_CLOCK_MIN_UNLOCK, SYSFS_LOCK, 0);
		else
			gpex_clock_lock_clock(GPU_CLOCK_MIN_LOCK, SYSFS_LOCK, clock);
	}

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_min_lock_dvfs)
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_min_lock_dvfs)

GPEX_STATIC ssize_t show_min_lock_dvfs(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;
	int user_locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = clk_info->min_lock;
	user_locked_clock = clk_info->user_min_lock_input;
	gpex_dvfs_spin_unlock(&flags);

	if (locked_clock > 0)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d / %d", locked_clock,
				user_locked_clock);
	else
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "-1");

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_min_lock_dvfs)

GPEX_STATIC ssize_t show_min_lock_dvfs_kobj(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = clk_info->min_lock;
	gpex_dvfs_spin_unlock(&flags);

	if (locked_clock > 0)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", locked_clock);
	else
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", clk_info->gpu_min_clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_min_lock_dvfs_kobj)

GPEX_STATIC ssize_t set_mm_min_lock_dvfs(const char *buf, size_t count)
{
	int ret, clock = 0;

	if (sysfs_streq("0", buf)) {
		clk_info->user_min_lock_input = 0;
		gpex_clock_lock_clock(GPU_CLOCK_MIN_UNLOCK, SYSFS_LOCK, 0);
	} else {
		ret = kstrtoint(buf, 0, &clock);
		if (ret) {
			GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
			return -ENOENT;
		}

		clk_info->user_min_lock_input = clock;

		clock = gpex_get_valid_gpu_clock(clock, true);

		ret = gpex_clock_get_table_idx(clock);
		if ((ret < gpex_clock_get_table_idx(gpex_clock_get_max_clock())) ||
		    (ret > gpex_clock_get_table_idx(gpex_clock_get_min_clock()))) {
			GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid clock value (%d)\n", __func__,
				clock);
			return -ENOENT;
		}

		if (clock > gpex_clock_get_max_clock_limit())
			clock = gpex_clock_get_max_clock_limit();

		gpex_clboost_set_state(CLBOOST_DISABLE);

		if (clock == gpex_clock_get_min_clock())
			gpex_clock_lock_clock(GPU_CLOCK_MIN_UNLOCK, SYSFS_LOCK, 0);
		else
			gpex_clock_lock_clock(GPU_CLOCK_MIN_LOCK, SYSFS_LOCK, clock);
	}

	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_mm_min_lock_dvfs)

GPEX_STATIC ssize_t show_mm_min_lock_dvfs(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = clk_info->min_lock;
	gpex_dvfs_spin_unlock(&flags);

	if (locked_clock > 0)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", locked_clock);
	else
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_clock_get_min_clock());

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_mm_min_lock_dvfs)

GPEX_STATIC ssize_t show_max_lock_status(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int i;
	int max_lock_status[NUMBER_LOCK];

	gpex_dvfs_spin_lock(&flags);
	for (i = 0; i < NUMBER_LOCK; i++)
		max_lock_status[i] = clk_info->user_max_lock[i];
	gpex_dvfs_spin_unlock(&flags);

	for (i = 0; i < NUMBER_LOCK; i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "[%d:%d]", i, max_lock_status[i]);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_max_lock_status)

GPEX_STATIC ssize_t show_min_lock_status(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int i;
	int min_lock_status[NUMBER_LOCK];

	gpex_dvfs_spin_lock(&flags);
	for (i = 0; i < NUMBER_LOCK; i++)
		min_lock_status[i] = clk_info->user_min_lock[i];
	gpex_dvfs_spin_unlock(&flags);

	for (i = 0; i < NUMBER_LOCK; i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "[%d:%d]", i, min_lock_status[i]);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_min_lock_status)

GPEX_STATIC ssize_t show_gpu_freq_table(char *buf)
{
	ssize_t ret = 0;
	int i = 0;

	/* TODO: find a better way to make sure this table is initialized...
	 * May be freq table should be initializesd even if DVFS is disabled?
	 * */
	if (!clk_info->table)
		return ret;

	for (i = gpex_clock_get_table_idx(clk_info->gpu_min_clock);
	     i >= gpex_clock_get_table_idx(clk_info->gpu_max_clock); i--) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d ", clk_info->table[i].clock);
	}

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_gpu_freq_table)

GPEX_STATIC ssize_t show_unlock_freqs(char *buf)
{
	ssize_t len = 0;
	bool unlock;

	unlock = clk_info->unlock_freqs;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d", unlock);

	return gpex_utils_sysfs_endbuf(buf, len);
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_unlock_freqs)

GPEX_STATIC ssize_t set_unlock_freqs(const char *buf, size_t count)
{
	bool unlock = false;
	int ret;

	ret = kstrtobool(buf, &unlock);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	clk_info->unlock_freqs = unlock;

	clk_info->user_max_lock_input = 0;
	gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, SYSFS_LOCK, 0);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_unlock_freqs)

GPEX_STATIC ssize_t show_volt(char *buf)
{
	ssize_t len = 0;
	int volt = 0;

	gpex_pm_lock();
	if (g3d_regulator)
		volt = regulator_get_voltage(g3d_regulator);
	else if (gpex_pm_get_status(false))
		volt = clk_info->table[gpex_clock_get_table_idx(gpex_clock_get_clock_slow())].voltage;
	gpex_pm_unlock();

	if (volt < 0)
		volt = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d", volt);

	return gpex_utils_sysfs_endbuf(buf, len);
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_volt)
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_volt)

GPEX_STATIC ssize_t show_kernel_sysfs_boost(char *buf)
{
	sprintf(buf, "%s[enabled] \t[%s]\n", buf, gpu_boost ? "Y" : "N");
	return strlen(buf);
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_boost)

GPEX_STATIC ssize_t set_kernel_sysfs_boost(const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	gpu_boost = input;
	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_kernel_sysfs_boost)

void calc_gpu_down_threshold(void)
{
	gpu_down_threshold = ((gpu_up_threshold * GPU_FREQ_STEP_0 / GPU_FREQ_STEP_1) - DOWN_THRESHOLD_MARGIN);
}

GPEX_STATIC ssize_t show_kernel_sysfs_up_threshold(char *buf)
{
	sprintf(buf, "%s[up_threshold] \t[%u]\n", buf, gpu_up_threshold);
	return strlen(buf);
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_up_threshold)

GPEX_STATIC ssize_t set_kernel_sysfs_up_threshold(const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > GPU_MAX_UP_THRESHOLD ||
			input < GPU_MIN_UP_THRESHOLD)
		return -EINVAL;

	gpu_up_threshold = input;

	/* update gpu_down_threshold */
	calc_gpu_down_threshold();

	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_kernel_sysfs_up_threshold)

int gpex_clock_sysfs_init(struct _clock_info *_clk_info)
{
	clk_info = _clk_info;
	g3d_regulator = regulator_get(NULL, "vdd_g3d");

	if (IS_ERR(g3d_regulator))
		g3d_regulator = NULL;

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(clock, show_clock, set_clock);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(asv_table, show_asv_table);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(time_in_state, show_time_in_state, reset_time_in_state);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(dvfs_max_lock, show_max_lock_dvfs, set_max_lock_dvfs);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(dvfs_min_lock, show_min_lock_dvfs, set_min_lock_dvfs);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(dvfs_max_lock_status, show_max_lock_status);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(dvfs_min_lock_status, show_min_lock_status);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(unlock_freqs, show_unlock_freqs, set_unlock_freqs);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(volt, show_volt);

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_max_clock, show_max_lock_dvfs_kobj,
					  set_max_lock_dvfs);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_min_clock, show_min_lock_dvfs_kobj,
					  set_min_lock_dvfs);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_mm_min_clock, show_mm_min_lock_dvfs,
					  set_mm_min_lock_dvfs);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_clock, show_clock);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_freq_table, show_gpu_freq_table);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_volt, show_volt);

        GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(boost, show_kernel_sysfs_boost, set_kernel_sysfs_boost);
        GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(up_threshold, show_kernel_sysfs_up_threshold, set_kernel_sysfs_up_threshold);

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /* SPDX-License-Identifier: GPL-2.0 */

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

/* Implements */
#include <gpex_cmar_boost.h>

/* Uses */
#include <linux/sched.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <uapi/linux/sched/types.h>
#endif

#include <mali_exynos_ioctl.h>
#include <gpex_utils.h>

int gpex_cmar_boost_set_flag(struct platform_context *pctx, int request)
{
	if (!pctx)
		return -EINVAL;

	switch (request) {
	case CMAR_BOOST_SET_RT:
		pctx->cmar_boost |= CMAR_BOOST_SET_RT;
		GPU_LOG(MALI_EXYNOS_DEBUG, "cmar boost SET requested for pid(%d)", pctx->pid);
		break;

	case CMAR_BOOST_SET_DEFAULT:
		pctx->cmar_boost |= CMAR_BOOST_SET_DEFAULT;
		GPU_LOG(MALI_EXYNOS_DEBUG, "cmar boost UNSET requested for pid(%d)", pctx->pid);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static bool current_priority_is_rt(void)
{
	return current->policy > 0;
}

void gpex_cmar_boost_set_thread_priority(struct platform_context *pctx)
{
	if (!pctx)
		return;

	if (pctx->cmar_boost == (CMAR_BOOST_SET_RT | CMAR_BOOST_DEFAULT) &&
	    !current_priority_is_rt()) {
		struct sched_param param = { .sched_priority = 1 };
		int ret = 0;

		ret = sched_setscheduler_nocheck(current, SCHED_FIFO, &param);
		if (ret) {
			pctx->cmar_boost = CMAR_BOOST_DEFAULT;

			GPU_LOG(MALI_EXYNOS_WARNING, "failed to set RT SCHED_CLASS cmar-backend\n");
		} else {
			pctx->cmar_boost = CMAR_BOOST_USER_RT;

			GPU_LOG(MALI_EXYNOS_DEBUG,
				"rt priority set for tid(%d) policy(%d) prio(%d) static_prio(%d) normal_prio(%d) rt_priority(%d)",
				current->pid, current->policy, current->prio, current->static_prio,
				current->normal_prio, current->rt_priority);
		}
	} else if (pctx->cmar_boost == (CMAR_BOOST_SET_DEFAULT | CMAR_BOOST_USER_RT) &&
		   current_priority_is_rt()) {
		struct sched_param param = { .sched_priority = 0 };
		int ret = 0;

		ret = sched_setscheduler_nocheck(current, SCHED_NORMAL, &param);
		if (ret) {
			GPU_LOG(MALI_EXYNOS_WARNING,
				"failed to set default SCHED_CLASS cmar-backend\n");
		} else {
			pctx->cmar_boost = CMAR_BOOST_DEFAULT;

			GPU_LOG(MALI_EXYNOS_DEBUG,
				"default priority set for tid(%d) policy(%d) prio(%d) static_prio(%d) normal_prio(%d) rt_priority(%d)",
				current->pid, current->policy, current->prio, current->static_prio,
				current->normal_prio, current->rt_priority);
		}
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            // SPDX-License-Identifier: GPL-2.0

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

/* Implements */
#include <gpex_cmar_sched.h>

/* Uses */
#include <linux/sched.h>

#include <gpex_utils.h>

static struct cpumask mask;
static struct cpumask full_mask;
static struct cpumask forced_mask;
static int is_forced_sched_enabled;
static int forced_min_index;
static int forced_max_index;
static int total_cpu_count;

int gpex_cmar_sched_set_forced_sched(int mode)
{
	if (mode) {
		if (is_forced_sched_enabled) {
			mask = forced_mask;
			GPU_LOG(MALI_EXYNOS_INFO, "cmar sched set forced AMIGO On: cpu mask=0x%x, min=%d, max=%d",
				is_forced_sched_enabled, mask, forced_min_index, forced_max_index);
		} else
			GPU_LOG(MALI_EXYNOS_INFO, "cmar sched set forced disabled");
	} else {
		mask = full_mask;
		GPU_LOG(MALI_EXYNOS_INFO, "cmar sched set forced AMIGO Off: cpu mask=0x%x", mask);
	}

	return 0;
}

int gpex_cmar_sched_set_cpu(int min, int max, struct cpumask *mask)
{
	int index = 0;

	if ((min < 0) || (min > total_cpu_count - 1) || (max <= min) || (max > total_cpu_count)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value  min:%d, max:%d", __func__, min, max);
		return -ENOENT;
	}

	cpumask_clear(mask);
	for (index = min; index < max; index++)
		cpumask_set_cpu(index, mask);

	return 0;
}

int gpex_cmar_sched_set_affinity(void)
{
	return set_cpus_allowed_ptr(current, &mask);
}

static ssize_t show_cmar_forced_sched_enable(char *buf)
{
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d", is_forced_sched_enabled);

	return gpex_utils_sysfs_endbuf(buf, len);
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_cmar_forced_sched_enable);

static ssize_t show_cmar_sched_min_index(char *buf)
{
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d", forced_min_index);

	return gpex_utils_sysfs_endbuf(buf, len);
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_cmar_sched_min_index);

static ssize_t show_cmar_sched_max_index(char *buf)
{
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d", forced_max_index);

	return gpex_utils_sysfs_endbuf(buf, len);
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_cmar_sched_max_index);

static ssize_t set_cmar_forced_sched_enable(const char *buf, size_t count)
{
	int ret, flag;

	ret = kstrtoint(buf, 0, &flag);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	is_forced_sched_enabled = flag;

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_cmar_forced_sched_enable);

static ssize_t set_cmar_sched_min_index(const char *buf, size_t count)
{
	int ret, index;

	ret = kstrtoint(buf, 0, &index);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if (gpex_cmar_sched_set_cpu(index, forced_max_index, &forced_mask) == 0)
		forced_min_index = index;

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_cmar_sched_min_index);

static ssize_t set_cmar_sched_max_index(const char *buf, size_t count)
{
	int ret, index;

	ret = kstrtoint(buf, 0, &index);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if (gpex_cmar_sched_set_cpu(forced_min_index, index, &forced_mask) == 0)
		forced_max_index = index;

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_cmar_sched_max_index);

int gpex_cmar_sched_sysfs_init(void)
{
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(cmar_forced_sched_enable, show_cmar_forced_sched_enable,
					 set_cmar_forced_sched_enable);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(cmar_sched_min_index, show_cmar_sched_min_index,
					 set_cmar_sched_min_index);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(cmar_sched_max_index, show_cmar_sched_max_index,
					 set_cmar_sched_max_index);

	return 0;
}

int gpex_cmar_sched_init(void)
{
	is_forced_sched_enabled = 1;
	forced_min_index = 0;
	forced_max_index = 6;
	total_cpu_count = 8;

	gpex_cmar_sched_set_cpu(forced_min_index, forced_max_index, &forced_mask);
	gpex_cmar_sched_set_cpu(0, total_cpu_count, &full_mask);
	mask = full_mask;

	gpex_cmar_sched_sysfs_init();

	return 0;
}

void gpex_cmar_sched_term(void)
{
	mask = full_mask;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /* SPDX-License-Identifier: GPL-2.0 */

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

#include <linux/slab.h>

#include <gpex_dvfs.h>
#include <gpex_utils.h>
#include <gpex_clock.h>
#include <gpex_pm.h>

#include <gpexbe_pm.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_utilization.h>
#include <gpex_qos.h>

#include "gpu_dvfs_governor.h"
#include "gpex_dvfs_internal.h"

static struct dvfs_info dvfs;

static int gpu_dvfs_handler_init(void);
static int gpu_dvfs_handler_deinit(void);

static void gpex_dvfs_context_init(struct device **dev)
{
	int i;
	const char *of_string;
	gpu_dt *dt = gpexbe_devicetree_get_gpu_dt();
	dvfs.kbdev = container_of(dev, struct kbase_device, dev);
	dvfs.table_size = dt->gpu_dvfs_table_size.row;
	dvfs.table = kcalloc(dvfs.table_size, sizeof(*dvfs.table), GFP_KERNEL);

	for (i = 0; i < dvfs.table_size; i++) {
		dvfs.table[i].clock = dt->gpu_dvfs_table[i].clock;
		dvfs.table[i].voltage = 0; // get it from gpex_clock
		dvfs.table[i].min_threshold = dt->gpu_dvfs_table[i].min_threshold;
		dvfs.table[i].max_threshold = dt->gpu_dvfs_table[i].max_threshold;
		dvfs.table[i].down_staycount = dt->gpu_dvfs_table[i].down_staycount;
	}

	of_string = gpexbe_devicetree_get_str(governor);

	if (!strncmp("interactive", of_string, strlen("interactive"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_INTERACTIVE;
		dvfs.interactive.highspeed_clock =
			gpexbe_devicetree_get_int(interactive_info.highspeed_clock);
		dvfs.interactive.highspeed_load =
			gpexbe_devicetree_get_int(interactive_info.highspeed_load);
		dvfs.interactive.highspeed_delay =
			gpexbe_devicetree_get_int(interactive_info.highspeed_delay);
	} else if (!strncmp("joint", of_string, strlen("joint"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_JOINT;
	} else if (!strncmp("static", of_string, strlen("static"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_STATIC;
	} else if (!strncmp("booster", of_string, strlen("booster"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_BOOSTER;
	} else if (!strncmp("dynamic", of_string, strlen("dynamic"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_DYNAMIC;
        } else if (!strncmp("ondemand", of_string, strlen("ondemand"))) {
                dvfs.governor_type = G3D_DVFS_GOVERNOR_ONDEMAND;
	} else {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_DEFAULT;
	}

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++) {
		gpu_dvfs_update_start_clk(i, gpex_clock_get_boot_clock());
	}

	dvfs.gpu_dvfs_config_clock = gpexbe_devicetree_get_int(gpu_dvfs_bl_config_clock);
	dvfs.polling_speed = gpexbe_devicetree_get_int(gpu_dvfs_polling_time);
}

static int gpu_dvfs_calculate_env_data(void)
{
	unsigned long flags;
	static int polling_period;

	spin_lock_irqsave(&dvfs.spinlock, flags);
	dvfs.env_data.utilization = gpexbe_utilization_calc_utilization();
	gpexbe_utilization_calculate_compute_ratio();
	spin_unlock_irqrestore(&dvfs.spinlock, flags);

	polling_period -= dvfs.polling_speed;
	if (polling_period > 0)
		return 0;

	return 0;
}

static int kbase_platform_dvfs_event(u32 utilisation)
{
	/* TODO: find out why this is needed and add back if necessary */
#if 0
	char *env[2] = {"FEATURE=GPUI", NULL};
	if(platform->fault_count >= 5 && platform->bigdata_uevent_is_sent == false)
	{
		platform->bigdata_uevent_is_sent = true;
		kobject_uevent_env(&kbdev->dev->kobj, KOBJ_CHANGE, env);
	}
#endif

	mutex_lock(&dvfs.handler_lock);
	if (gpex_pm_get_status(true)) {
		int clk = 0;
		gpu_dvfs_calculate_env_data();
		clk = gpu_dvfs_decide_next_freq(dvfs.env_data.utilization);
		gpex_clock_set(clk);
	}
	mutex_unlock(&dvfs.handler_lock);

	GPU_LOG(MALI_EXYNOS_DEBUG, "dvfs hanlder is called\n");

	return 0;
}

static void dvfs_callback(struct work_struct *data)
{
	unsigned long flags;

	//KBASE_DEBUG_ASSERT(data != NULL);

	kbase_platform_dvfs_event(0);

	spin_lock_irqsave(&dvfs.spinlock, flags);

	if (dvfs.timer_active)
		queue_delayed_work_on(0, dvfs.dvfs_wq, &dvfs.dvfs_work,
				      msecs_to_jiffies(dvfs.polling_speed));

	spin_unlock_irqrestore(&dvfs.spinlock, flags);
}

static void gpu_dvfs_timer_control(bool timer_state)
{
	unsigned long flags;

	if (!dvfs.status) {
		GPU_LOG(MALI_EXYNOS_DEBUG, "%s: DVFS is disabled\n", __func__);
		return;
	}
	if (dvfs.timer_active && !timer_state) {
		cancel_delayed_work(&dvfs.dvfs_work);
		flush_workqueue(dvfs.dvfs_wq);
	} else if (!dvfs.timer_active && timer_state) {
		gpex_qos_set_from_clock(gpex_clock_get_cur_clock());

		queue_delayed_work_on(0, dvfs.dvfs_wq, &dvfs.dvfs_work,
				      msecs_to_jiffies(dvfs.polling_speed));
		spin_lock_irqsave(&dvfs.spinlock, flags);
		dvfs.down_requirement = dvfs.table[dvfs.step].down_staycount;
		dvfs.interactive.delay_count = 0;
		spin_unlock_irqrestore(&dvfs.spinlock, flags);
	}

	spin_lock_irqsave(&dvfs.spinlock, flags);
	dvfs.timer_active = timer_state;
	spin_unlock_irqrestore(&dvfs.spinlock, flags);
}

void gpex_dvfs_start(void)
{
	gpu_dvfs_timer_control(true);
}

void gpex_dvfs_stop(void)
{
	gpu_dvfs_timer_control(false);
}
/* TODO */
/* MIN_POLLING_SPEED is dependant to Operating System(kernel timer, HZ) */
/* it is rarely changed from 4 to other values, but it would be better to make it follow kernel env */
int gpex_dvfs_set_polling_speed(int polling_speed)
{
	if ((polling_speed < GPEX_DVFS_MIN_POLLING_SPEED) ||
	    (polling_speed > GPEX_DVFS_MAX_POLLING_SPEED)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: out of range [4~1000] (%d)\n", __func__,
			polling_speed);
		return -ENOENT;
	}

	dvfs.polling_speed = polling_speed;

	return 0;
}

static int gpu_dvfs_on_off(bool enable)
{
	/* TODO get proper return values from gpu_dvfs_handler_init */
	if (enable && !dvfs.status) {
		mutex_lock(&dvfs.handler_lock);
		gpex_clock_set(gpex_clock_get_cur_clock());
		gpu_dvfs_handler_init();
		mutex_unlock(&dvfs.handler_lock);

		gpex_dvfs_start();
	} else if (!enable && dvfs.status) {
		gpex_dvfs_stop();

		mutex_lock(&dvfs.handler_lock);
		gpu_dvfs_handler_deinit();
		gpex_clock_set(dvfs.gpu_dvfs_config_clock);
		mutex_unlock(&dvfs.handler_lock);
	}

	return 0;
}

int gpex_dvfs_enable(void)
{
	return gpu_dvfs_on_off(true);
}

int gpex_dvfs_disable(void)
{
	return gpu_dvfs_on_off(false);
}

static int gpu_dvfs_handler_init(void)
{
	if (!dvfs.status)
		dvfs.status = true;

	gpex_clock_set(dvfs.table[dvfs.step].clock);

	dvfs.timer_active = true;

	GPU_LOG(MALI_EXYNOS_INFO, "dvfs handler initialized\n");
	return 0;
}

static int gpu_dvfs_handler_deinit(void)
{
	if (dvfs.status)
		dvfs.status = false;

	dvfs.timer_active = false;

	GPU_LOG(MALI_EXYNOS_INFO, "dvfs handler de-initialized\n");
	return 0;
}

static int gpu_pm_metrics_init(void)
{
	INIT_DELAYED_WORK(&dvfs.dvfs_work, dvfs_callback);
	dvfs.dvfs_wq = create_workqueue("g3d_dvfs");

	queue_delayed_work_on(0, dvfs.dvfs_wq, &dvfs.dvfs_work,
			      msecs_to_jiffies(dvfs.polling_speed));

	return 0;
}

static void gpu_pm_metrics_term(void)
{
	cancel_delayed_work(&dvfs.dvfs_work);
	flush_workqueue(dvfs.dvfs_wq);
	destroy_workqueue(dvfs.dvfs_wq);
}

int gpex_dvfs_init(struct device **dev)
{
	spin_lock_init(&dvfs.spinlock);
	mutex_init(&dvfs.handler_lock);

	gpex_dvfs_context_init(dev);

	gpu_dvfs_governor_init(&dvfs);

	gpu_dvfs_handler_init();

	/* dvfs wq start */
	gpu_pm_metrics_init();

	gpex_dvfs_sysfs_init(&dvfs);

	gpex_dvfs_external_init(&dvfs);

	gpex_utils_get_exynos_context()->dvfs = &dvfs;

	return 0;
}

void gpex_dvfs_term(void)
{
	/* DVFS stuff */
	gpu_pm_metrics_term();
	gpu_dvfs_handler_deinit();
	dvfs.kbdev = NULL;
}

int gpex_dvfs_get_status(void)
{
	return dvfs.status;
}

int gpex_dvfs_get_governor_type(void)
{
	return dvfs.governor_type;
}

int gpex_dvfs_get_utilization(void)
{
	return dvfs.env_data.utilization;
}

int gpex_dvfs_get_polling_speed(void)
{
	return dvfs.polling_speed;
}

void gpex_dvfs_spin_lock(unsigned long *flags)
{
	spin_lock_irqsave(&dvfs.spinlock, *flags);
}

void gpex_dvfs_spin_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&dvfs.spinlock, *flags);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /* SPDX-License-Identifier: GPL-2.0 */

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

#include <gpex_dvfs.h>
#include <gpex_clock.h>
#include <gpex_pm.h>

#include <mali_exynos_if.h>

#include "gpex_dvfs_internal.h"
static struct dvfs_info *dvfs;

int gpu_dvfs_get_clock(int level)
{
	if ((level < 0) || (level >= dvfs->table_size))
		return -1;

	return dvfs->table[level].clock;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_clock);

int gpu_dvfs_get_voltage(int clock)
{
	return gpex_clock_get_voltage(clock);
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_voltage);

int gpu_dvfs_get_step(void)
{
	return dvfs->table_size;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_step);

int gpu_dvfs_get_cur_clock(void)
{
	int clock = 0;

	gpex_pm_lock();
	if (gpex_pm_get_status(false))
		clock = gpex_clock_get_clock_slow();
	gpex_pm_unlock();

	return clock;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_cur_clock);

int gpu_dvfs_get_utilization(void)
{
	int util = 0;

	if (gpex_pm_get_status(true) == 1)
		util = dvfs->env_data.utilization;

	return util;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_utilization);

int gpu_dvfs_get_min_freq(void)
{
	return gpex_clock_get_min_clock();
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_min_freq);

int gpu_dvfs_get_max_freq(void)
{
	return gpex_clock_get_max_clock();
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_max_freq);

int gpu_dvfs_get_max_locked_freq(void)
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
EXPORT_SYMBOL(gpu_dvfs_get_max_locked_freq);

int gpu_dvfs_get_min_locked_freq(void)
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
EXPORT_SYMBOL(gpu_dvfs_get_min_locked_freq);

/* TODO: make a stub version for when dvfs is disabled */
/* Needed by 9830 as SUSTAINABLE_OPT feature */
int gpu_dvfs_get_sustainable_info_array(int index)
{
	CSTD_UNUSED(index);
	return 0;
}

/* TODO: make a stub version for when dvfs is disabled */
/* Needed by 9830 by exynos_perf_gmc.c */
int gpu_dvfs_get_max_lock(void)
{
	return gpex_clock_get_max_lock();
}

/* Needed by 9830 by exynos_perf_gmc.c (NEGATIVE_BOOST) */
bool gpu_dvfs_get_need_cpu_qos(void)
{
	return false;
}

int gpex_dvfs_external_init(struct dvfs_info *_dvfs)
{
	dvfs = _dvfs;

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /* SPDX-License-Identifier: GPL-2.0 */

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

#ifndef _GPEX_DVFS_INTERNAL_H_
#define _GPEX_DVFS_INTERNAL_H_

#include <gpex_clock.h>

#define DVFS_ASSERT(x)                                                                             \
	do {                                                                                       \
		if (x)                                                                             \
			break;                                                                     \
		printk(KERN_EMERG "### ASSERTION FAILED %s: %s: %d: %s\n", __FILE__, __func__,     \
		       __LINE__, #x);                                                              \
		dump_stack();                                                                      \
	} while (0)

#define GPEX_DVFS_MIN_POLLING_SPEED 4
#define GPEX_DVFS_MAX_POLLING_SPEED 1000

typedef struct _gpu_dvfs_env_data {
	int utilization;
} gpu_dvfs_env_data;

typedef struct _gpu_dvfs_governor_info {
	int id;
	char *name;
	void *governor;
	int table_size;
	int start_clk;
} gpu_dvfs_governor_info;

typedef struct _dvfs_clock_info {
	unsigned int clock;
	unsigned int voltage;
	int min_threshold;
	int max_threshold;
	int down_staycount;
} dvfs_clock_info;

struct dvfs_info {
	struct kbase_device *kbdev;
	int step;
	dvfs_clock_info *table;
	int table_size;
	bool timer_active;
	bool status;
	int governor_type;
	int gpu_dvfs_config_clock; // starting clock when enabling dvfs using sysfs
	int polling_speed;
	spinlock_t spinlock;
	gpu_dvfs_env_data env_data;

	struct mutex handler_lock;

	struct delayed_work dvfs_work;
	struct workqueue_struct *dvfs_wq;
	int down_requirement;

	/* For the interactive governor */
	struct {
		int highspeed_clock;
		int highspeed_load;
		int highspeed_delay;
		int delay_count;
	} interactive;
};

int gpex_dvfs_sysfs_init(struct dvfs_info *_dvfs);
int gpex_dvfs_external_init(struct dvfs_info *_dvfs);
int gpu_dvfs_governor_init(struct dvfs_info *_dvfs);

#endif /* _GPEX_DVFS_INTERNAL_H_ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /* SPDX-License-Identifier: GPL-2.0 */

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

#include <gpex_pm.h>
#include <gpex_dvfs.h>
#include <gpex_clock.h>
#include <gpex_utils.h>

#include "gpex_dvfs_internal.h"
#include "gpu_dvfs_governor.h"

static struct dvfs_info *dvfs;

static int gpu_dvfs_governor_change(int governor_type)
{
	mutex_lock(&dvfs->handler_lock);
	gpu_dvfs_governor_setting(governor_type);
	mutex_unlock(&dvfs->handler_lock);

	return 0;
}

static int gpu_get_dvfs_table(char *buf, size_t buf_size)
{
	int i, cnt = 0;

	if (buf == NULL)
		return 0;

	for (i = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	     i <= gpex_clock_get_table_idx(gpex_clock_get_min_clock()); i++)
		cnt += snprintf(buf + cnt, buf_size - cnt, " %d", dvfs->table[i].clock);

	cnt += snprintf(buf + cnt, buf_size - cnt, "\n");

	return cnt;
}

static ssize_t show_dvfs_table(char *buf)
{
	ssize_t ret = 0;

	ret += gpu_get_dvfs_table(buf + ret, (size_t)PAGE_SIZE - ret);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_dvfs_table);

static ssize_t show_dvfs(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", dvfs->status);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_dvfs);

static ssize_t set_dvfs(const char *buf, size_t count)
{
	if (sysfs_streq("0", buf))
		gpex_dvfs_disable();
	else if (sysfs_streq("1", buf))
		gpex_dvfs_enable();

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_dvfs);

static ssize_t show_governor(char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info;
	int i;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s\n", governor_info[i].name);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "[Current Governor] %s",
			governor_info[dvfs->governor_type].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_governor);

static ssize_t set_governor(const char *buf, size_t count)
{
	int ret;
	int next_governor_type;

	ret = kstrtoint(buf, 0, &next_governor_type);

	if ((next_governor_type < 0) || (next_governor_type >= G3D_MAX_GOVERNOR_NUM)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpu_dvfs_governor_change(next_governor_type);

	if (ret < 0) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: fail to set the new governor (%d)\n", __func__,
			next_governor_type);
		return -ENOENT;
	}

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_governor);

static ssize_t show_down_staycount(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int i = -1;

	spin_lock_irqsave(&dvfs->spinlock, flags);
	for (i = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	     i <= gpex_clock_get_table_idx(gpex_clock_get_min_clock()); i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "Clock %d - %d\n", dvfs->table[i].clock,
				dvfs->table[i].down_staycount);
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_down_staycount);

#define MIN_DOWN_STAYCOUNT 1
#define MAX_DOWN_STAYCOUNT 10
static ssize_t set_down_staycount(const char *buf, size_t count)
{
	unsigned long flags;
	char tmpbuf[32];
	char *sptr, *tok;
	int ret = -1;
	int clock = -1, level = -1, down_staycount = 0;
	unsigned int len = 0;

	len = (unsigned int)min(count, sizeof(tmpbuf) - 1);
	memcpy(tmpbuf, buf, len);
	tmpbuf[len] = '\0';
	sptr = tmpbuf;

	tok = strsep(&sptr, " ,");
	if (tok == NULL) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid input\n", __func__);
		return -ENOENT;
	}

	ret = kstrtoint(tok, 0, &clock);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid input %d\n", __func__, clock);
		return -ENOENT;
	}

	tok = strsep(&sptr, " ,");
	if (tok == NULL) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid input\n", __func__);
		return -ENOENT;
	}

	ret = kstrtoint(tok, 0, &down_staycount);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid input %d\n", __func__, down_staycount);
		return -ENOENT;
	}

	level = gpex_clock_get_table_idx(clock);
	if (level < 0) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid clock value (%d)\n", __func__, clock);
		return -ENOENT;
	}

	if ((down_staycount < MIN_DOWN_STAYCOUNT) || (down_staycount > MAX_DOWN_STAYCOUNT)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: down_staycount is out of range (%d, %d ~ %d)\n",
			__func__, down_staycount, MIN_DOWN_STAYCOUNT, MAX_DOWN_STAYCOUNT);
		return -ENOENT;
	}

	spin_lock_irqsave(&dvfs->spinlock, flags);
	dvfs->table[level].down_staycount = down_staycount;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_down_staycount);

static ssize_t show_highspeed_clock(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_clock = -1;

	spin_lock_irqsave(&dvfs->spinlock, flags);
	highspeed_clock = dvfs->interactive.highspeed_clock;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", highspeed_clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_highspeed_clock);

static ssize_t set_highspeed_clock(const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_clock = -1;

	ret = kstrtoint(buf, 0, &highspeed_clock);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpex_clock_get_table_idx(highspeed_clock);
	if ((ret < gpex_clock_get_table_idx(gpex_clock_get_max_clock())) ||
	    (ret > gpex_clock_get_table_idx(gpex_clock_get_min_clock()))) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid clock value (%d)\n", __func__,
			highspeed_clock);
		return -ENOENT;
	}

	if (highspeed_clock > gpex_clock_get_max_clock_limit())
		highspeed_clock = gpex_clock_get_max_clock_limit();

	spin_lock_irqsave(&dvfs->spinlock, flags);
	dvfs->interactive.highspeed_clock = highspeed_clock;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_highspeed_clock);

static ssize_t show_highspeed_load(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_load = -1;

	spin_lock_irqsave(&dvfs->spinlock, flags);
	highspeed_load = dvfs->interactive.highspeed_load;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", highspeed_load);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_highspeed_load);

static ssize_t set_highspeed_load(const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_load = -1;

	ret = kstrtoint(buf, 0, &highspeed_load);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((highspeed_load < 0) || (highspeed_load > 100)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid load value (%d)\n", __func__,
			highspeed_load);
		return -ENOENT;
	}

	spin_lock_irqsave(&dvfs->spinlock, flags);
	dvfs->interactive.highspeed_load = highspeed_load;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_highspeed_load);

static ssize_t show_highspeed_delay(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_delay = -1;

	spin_lock_irqsave(&dvfs->spinlock, flags);
	highspeed_delay = dvfs->interactive.highspeed_delay;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", highspeed_delay);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_highspeed_delay);

static ssize_t set_highspeed_delay(const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_delay = -1;

	ret = kstrtoint(buf, 0, &highspeed_delay);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((highspeed_delay < 0) || (highspeed_delay > 5)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid load value (%d)\n", __func__,
			highspeed_delay);
		return -ENOENT;
	}

	spin_lock_irqsave(&dvfs->spinlock, flags);
	dvfs->interactive.highspeed_delay = highspeed_delay;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_highspeed_delay);

static ssize_t show_polling_speed(char *buf)
{
	ssize_t ret = 0;

	int polling_speed = gpex_dvfs_get_polling_speed();
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", polling_speed);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_polling_speed);

static ssize_t set_polling_speed(const char *buf, size_t count)
{
	int ret, polling_speed;

	ret = kstrtoint(buf, 0, &polling_speed);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_dvfs_set_polling_speed(polling_speed);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_polling_speed);

static ssize_t show_utilization(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d",
			gpex_pm_get_status(true) * dvfs->env_data.utilization);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_utilization);

static ssize_t show_kernel_sysfs_utilization(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%3d%%", dvfs->env_data.utilization);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_utilization)

#define BUF_SIZE 1000
static ssize_t show_kernel_sysfs_available_governor(char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info;
	int i;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s ", governor_info[i].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_available_governor)

static ssize_t show_kernel_sysfs_governor(char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info = NULL;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s", governor_info[dvfs->governor_type].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_governor)

static ssize_t set_kernel_sysfs_governor(const char *buf, size_t count)
{
	int ret;
	int i = 0;
	int next_governor_type = -1;
	size_t governor_name_size = 0;
	gpu_dvfs_governor_info *governor_info = NULL;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++) {
		governor_name_size = strlen(governor_info[i].name);
		if (!strncmp(buf, governor_info[i].name, governor_name_size)) {
			next_governor_type = i;
			break;
		}
	}

	if ((next_governor_type < 0) || (next_governor_type >= G3D_MAX_GOVERNOR_NUM)) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpu_dvfs_governor_change(next_governor_type);

	if (ret < 0) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: fail to set the new governor (%d)\n", __func__,
			next_governor_type);
		return -ENOENT;
	}

	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_kernel_sysfs_governor)

int gpex_dvfs_sysfs_init(struct dvfs_info *_dvfs)
{
	dvfs = _dvfs;

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(dvfs, show_dvfs, set_dvfs);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(dvfs_governor, show_governor, set_governor);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(down_staycount, show_down_staycount, set_down_staycount);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(highspeed_clock, show_highspeed_clock,
					 set_highspeed_clock);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(highspeed_load, show_highspeed_load, set_highspeed_load);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(highspeed_delay, show_highspeed_delay,
					 set_highspeed_delay);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(polling_speed, show_polling_speed, set_polling_speed);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(dvfs_table, show_dvfs_table);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(utilization, show_utilization);

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_governor, show_kernel_sysfs_governor,
					  set_kernel_sysfs_governor);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_available_governor,
					     show_kernel_sysfs_available_governor);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_busy, show_kernel_sysfs_utilization);

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            // SPDX-License-Identifier: GPL-2.0

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

#include <gpex_clock.h>
#include <gpex_tsg.h>
#include <gpex_gts.h>
#include <gpexbe_utilization.h>
#include <gpexbe_devicetree.h>

#include <mali_exynos_ioctl.h>

struct _gts_info {
	int heavy_compute_mode;
	atomic_t heavy_compute_util_count;

	int highspeed_clock;

	u32 busy_js;
	u32 idle_js;
	bool jobslot_active;
};

static struct _gts_info gts_info;

#define LCNT 4

struct gpu_shared_d {
	u64 out_data[LCNT];
	u64 input;
	u64 input2;
	u64 freq;
	u64 flimit;
};

struct gpu_data {
	void (*get_out_data)(u64 *cnt);
	u64 last_out_data;
	u64 freq;
	u64 input;
	u64 input2;
	int update_idx;
	ktime_t last_update_t;
	struct gpu_shared_d sd;
	int odidx;
};

struct gpu_data gpud;

void gpu_register_out_data(void (*fn)(u64 *cnt))
{
	gpud.get_out_data = fn;
}
EXPORT_SYMBOL_GPL(gpu_register_out_data);

static inline int get_outd_idx(int idx)
{
	if (++idx >= 4)
		return 0;
	return idx;
}

int gpex_gts_get_ioctl_gts_info(struct mali_exynos_ioctl_gts_info *info)
{
	int i = 0;
#if IS_ENABLED(CONFIG_SMP)
	info->util_avg = READ_ONCE(current->se.avg.util_avg);
#else
	info->util_avg = 0;
#endif
	for (i = 0; i < LCNT; i++)
		info->out_data[i] = gpud.sd.out_data[i];

	info->input = gpud.sd.input / 10;
	info->input2 = gpud.sd.input2;
	info->freq = gpud.sd.freq;
	info->flimit = gts_info.highspeed_clock;
	info->hcm_mode = gts_info.heavy_compute_mode;

	return 0;
}

static int calculate_jobslot_util(void)
{
	return 100 * gts_info.busy_js / (gts_info.idle_js + gts_info.busy_js);
}

static void calculate_heavy_compute_count(void)
{
	/* HCM STUFF */
	int heavy_compute_count = 0;
	int compute_time;
	int vertex_time;
	int fragment_time;
	int total_time;

	compute_time = gpexbe_utilization_get_compute_job_time();
	vertex_time = gpexbe_utilization_get_vertex_job_time();
	fragment_time = gpexbe_utilization_get_fragment_job_time();
	total_time = compute_time + vertex_time + fragment_time;

	heavy_compute_count = atomic_read(&gts_info.heavy_compute_util_count);

	if (total_time > 0 && fragment_time > 0) {
		int compute_cnt = gpexbe_utilization_get_compute_job_cnt();
		int vertex_cnt = gpexbe_utilization_get_vertex_job_cnt();
		int fragment_cnt = gpexbe_utilization_get_fragment_job_cnt();
		int vf_ratio = 0;

		if (fragment_cnt > 0) {
			if ((compute_cnt + vertex_cnt) > 0)
				vf_ratio = (compute_cnt + vertex_cnt) * 100 / fragment_cnt;
			else
				vf_ratio = 0;
		} else {
			if ((compute_cnt + vertex_cnt) > 0)
				vf_ratio = 10;
			else
				vf_ratio = 0;
		}

		if (vf_ratio > 94 && ((vertex_time + compute_time) * 100 / fragment_time) > 60) {
			if (heavy_compute_count < 10) {
				atomic_inc(&gts_info.heavy_compute_util_count);
			}
		} else {
			if (heavy_compute_count > 0) {
				atomic_dec(&gts_info.heavy_compute_util_count);
			}
		}
	}

	/* HCM Stuff end */
}

bool gpex_gts_update_gpu_data(void)
{
	ktime_t now = ktime_get();
	u64 out_data;

	int jsutil;
	int input2;

	if (unlikely(!gpud.get_out_data)) {
		gpud.sd.input = 0;
		gpud.sd.input2 = 0;
		return false;
	}

	jsutil = calculate_jobslot_util();
	calculate_heavy_compute_count();
	input2 = atomic_read(&gts_info.heavy_compute_util_count);

#if IS_ENABLED(CONFIG_MALI_NOTIFY_UTILISATION)
	if (gts_info.heavy_compute_mode & HCM_MODE_B &&
	    gpex_clock_get_cur_clock() > gpex_clock_get_clock(1))
		gpex_tsg_notify_frag_utils_change(jsutil >> 1);
	else
		gpex_tsg_notify_frag_utils_change(jsutil); // Futher update utilizations
#endif

	gpud.freq += gpex_clock_get_cur_clock();
	gpud.input += jsutil;
	gpud.update_idx++;

#define UPDATE_PERIOD 250
	if (UPDATE_PERIOD > ktime_to_ms(ktime_sub(now, gpud.last_update_t)) || !gpud.update_idx)
		return false;

	gpud.sd.freq = gpud.freq / gpud.update_idx;
	gpud.sd.input = gpud.input / gpud.update_idx;
	gpud.sd.input2 = input2;

	gpud.freq = 0;
	gpud.input = 0;
	gpud.update_idx = 0;

	gpud.get_out_data(&out_data);
	gpud.odidx = get_outd_idx(gpud.odidx);
	gpud.sd.out_data[gpud.odidx] = out_data - gpud.last_out_data;
	gpud.last_out_data = out_data;
	gpud.last_update_t = now;

	return true;
}

void gpex_gts_update_jobslot_util(bool gpu_active, u32 ns_time)
{
	if (gpu_active && gts_info.jobslot_active)
		gts_info.busy_js += ns_time;
	else
		gts_info.idle_js += ns_time;
}

void gpex_gts_set_jobslot_status(bool is_active)
{
	gts_info.jobslot_active = is_active;
}

void gpex_gts_clear(void)
{
	gts_info.busy_js = 0;
	gts_info.idle_js = 0;
}

void gpex_gts_set_hcm_mode(int hcm_mode_val)
{
	gts_info.heavy_compute_mode = hcm_mode_val;
}

int gpex_gts_get_hcm_mode(void)
{
	return gts_info.heavy_compute_mode;
}

int gpex_gts_init(struct device **dev)
{
	atomic_set(&gts_info.heavy_compute_util_count, 0);

	gts_info.highspeed_clock = gpexbe_devicetree_get_int(interactive_info.highspeed_clock);
	gts_info.heavy_compute_mode = 0;
	gts_info.busy_js = 0;
	gts_info.idle_js = 0;

	gpex_utils_get_exynos_context()->gts_info = &gts_info;

	return 0;
}

void gpex_gts_term(void)
{
	gts_info.highspeed_clock = 0;
	gts_info.heavy_compute_mode = 0;
	gts_info.busy_js = 0;
	gts_info.idle_js = 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /* SPDX-License-Identifier: GPL-2.0 */

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

#include <errno.h>

#include <gpex_ifpo.h>
#include <gpex_clock.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_pm.h>

struct ifpo_info {
	ifpo_mode mode;
	ifpo_status status;
};

static struct ifpo_info ifpo;

int gpex_ifpo_power_down()
{
	int ret = 0;

	if (!ifpo.mode)
		return 0;

	gpex_clock_mutex_lock();

	/* inter frame power off */
	ret = gpexbe_pm_pd_control_down();

	if (!ret)
		ifpo.status = IFPO_POWER_DOWN;

	gpex_clock_mutex_unlock();

	return ret;
}

int gpex_ifpo_power_up()
{
	int ret = 0;

	if (!ifpo.mode)
		return 0;

	gpex_clock_mutex_lock();

	/* inter frame power on */
	ret = gpexbe_pm_pd_control_up();

	if (!ret)
		ifpo.status = IFPO_POWER_UP;

	gpex_clock_mutex_unlock();

	return ret;
}

/* return non-zero value when power off for now */
ifpo_status gpex_ifpo_get_status()
{
	return ifpo.status;
}

ifpo_mode gpex_ifpo_get_mode()
{
	return ifpo.mode;
}

int gpex_ifpo_init()
{
	ifpo.mode = (ifpo_mode)gpexbe_devicetree_get_int(gpu_inter_frame_pm);

	/* TODO: delete this line! This is to force enable IFPO regardless of the DT value */
	/* TODO: Or maybe remove inter_frame_pm from DT and let kconfig decide IFPO enable/disable? */
	ifpo.mode = IFPO_ENABLED;
	ifpo.status = IFPO_POWER_UP;

	return 0;
}

void gpex_ifpo_term()
{
	ifpo.mode = IFPO_DISABLED;

	/* TODO: power up if in power down state? */
	if (ifpo.status == IFPO_POWER_DOWN)
		gpex_ifpo_power_up();

	return;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /* SPDX-License-Identifier: GPL-2.0 */

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

#include <gpex_platform.h>
#include <gpex_utils.h>
#include <gpex_debug.h>
#include <gpex_pm.h>
#include <gpex_dvfs.h>
#include <gpex_qos.h>
#include <gpex_thermal.h>
#include <gpex_clock.h>
#include <gpex_ifpo.h>
#include <gpex_tsg.h>
#include <gpex_clboost.h>
#include <gpex_cmar_sched.h>
#include <gpexbe_devicetree.h>

#include <gpexbe_notifier.h>
#include <gpexbe_pm.h>
#include <gpexbe_clock.h>
#include <gpexbe_qos.h>
#include <gpexbe_bts.h>
#include <gpexbe_debug.h>
#include <gpexbe_utilization.h>
#include <gpexbe_llc_coherency.h>
#include <gpexbe_mem_usage.h>
#include <gpexbe_smc.h>
#include <gpex_gts.h>
#include <gpexwa_interactive_boost.h>

#include <runtime_test_runner.h>

int gpex_platform_init(struct device **dev)
{
	/* TODO: check return value */
	/* TODO: becareful with order */
	gpexbe_devicetree_init(*dev);
	gpex_utils_init(dev);
	gpex_debug_init(dev);

	gpexbe_utilization_init(dev);
	gpex_clboost_init();

	gpex_gts_init(dev);

	gpexbe_debug_init();

	gpex_thermal_init();
	gpexbe_notifier_init();

	gpexbe_llc_coherency_init(dev);

	gpexbe_pm_init();
	gpexbe_clock_init();
	gpex_pm_init();
	gpex_clock_init(dev);

	gpexbe_qos_init();
	gpexbe_bts_init();
	gpex_qos_init();

	gpex_ifpo_init();
	gpex_dvfs_init(dev);
	gpexbe_smc_init();
	gpex_cmar_sched_init();
	gpex_tsg_init(dev);

	gpexbe_mem_usage_init();

	gpexwa_interactive_boost_init();

	runtime_test_runner_init();

	gpex_utils_sysfs_kobject_files_create();
	gpex_utils_sysfs_device_files_create();

	return 0;
}

void gpex_platform_term(void)
{
	runtime_test_runner_term();

	gpexbe_mem_usage_term();

	gpexwa_interactive_boost_term();

	gpex_tsg_ter