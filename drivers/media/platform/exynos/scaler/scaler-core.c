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

/* Implements */
#include <gpexbe_secure.h>

/* Uses */
#include <linux/protected_mode_switcher.h>

#include <mali_kbase.h>
#include <device/mali_kbase_device.h>
#include <mali_kbase_hwaccess_jm.h>

#include <gpexbe_smc.h>
#include <gpex_utils.h>

static int exynos_secure_mode_enable(struct protected_mode_device *pdev)
{
	CSTD_UNUSED(pdev);

	return gpexbe_smc_protection_enable();
}

static int exynos_secure_mode_disable(struct protected_mode_device *pdev)
{
	struct kbase_device *kbdev = pdev->data;

	if (kbdev->protected_mode)
		return gpexbe_smc_protection_disable();
	else
		return kbase_pm_protected_mode_disable(kbdev);
}

struct protected_mode_ops *gpexbe_secure_get_protected_mode_ops()
{
	static struct protected_mode_ops exynos_protected_ops = {
		.protected_mode_enable = &exynos_secure_mode_enable,
		.protected_mode_disable = &exynos_secure_mode_disable
	};

	return &exynos_protected_ops;
}

static void kbasep_js_cacheclean(struct kbase_device *kbdev)
{
	/* Limit the number of loops to avoid a hang if the interrupt is missed */
	u32 max_loops = KBASE_CLEAN_CACHE_MAX_LOOPS;

	/* use GPU_COMMAND completion solution */
	/* clean the caches */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_CLEAN_CACHES);

	/* wait for cache flush to complete before continuing */
	while (--max_loops && (kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_RAWSTAT)) &
			       CLEAN_CACHES_COMPLETED) == 0)
		;

	/* clear the CLEAN_CACHES_COMPLETED irq */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_CLEAR), CLEAN_CACHES_COMPLETED);
}

int gpexbe_secure_legacy_jm_enter_protected_mode(struct kbase_device *kbdev)
{
	if (!kbdev)
		return -EINVAL;

	if (kbase_gpu_atoms_submitted_any(kbdev))
		return -EAGAIN;

	if (kbdev->protected_ops) {
		int err = 0;

		/* Switch GPU to protected mode */
		kbasep_js_cacheclean(kbdev);
		err = exynos_secure_mode_enable(kbdev->protected_dev);

		if (err)
			dev_warn(kbdev->dev, "Failed to enable protected mode: %d\n", err);
		else
			kbdev->protected_mode = true;
	}

	return 0;
}

int gpexbe_secure_legacy_jm_exit_protected_mode(struct kbase_device *kbdev)
{
	if (!kbdev)
		return -EINVAL;

	if (!kbdev->protected_mode)
		return kbase_pm_protected_mode_disable(kbdev);

	if (kbase_gpu_atoms_submitted_any(kbdev))
		return -EAGAIN;

	if (kbdev->protected_ops) {
		int err = 0;

		/* Switch GPU to protected mode */
		kbasep_js_cacheclean(kbdev);
		err = exynos_secure_mode_disable(kbdev->protected_dev);

		if (err)
			dev_warn(kbdev->dev, "Failed to disable protected mode: %d\n", err);
		else
			kbdev->protected_mode = false;
	}

	return 0;
}

int gpexbe_secure_legacy_pm_exit_protected_mode(struct kbase_device *kbdev)
{
	int err = 0;

	if (!kbdev)
		return -EINVAL;

	if (kbdev->protected_mode == true) {
		/* Switch GPU to non-secure mode */
		err = exynos_secure_mode_disable(kbdev->protected_dev);
		if (err)
			dev_warn(kbdev->dev, "Failed to disable protected mode: %d\n", err);
		else
			kbdev->protected_mode = false;
	}

	return err;
}
                                                                                                                                                                                                                                                                                               /* SPDX-License-Identifier: GPL-2.0 */

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
#include <gpexbe_smc.h>

/* Uses */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/spinlock.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
#include <linux/smc.h>
#else
#include <soc/samsung/exynos-smc.h>
#endif

#include <gpex_utils.h>

struct _smc_info {
	bool protection_enabled;
	spinlock_t lock;
};

static struct _smc_info smc_info;

int gpexbe_smc_protection_enable(void)
{
	int err;
	unsigned long flags;

	spin_lock_irqsave(&smc_info.lock, flags);
	if (smc_info.protection_enabled) {
		spin_unlock_irqrestore(&smc_info.lock, flags);
		return 0;
	}

	err = exynos_smc(SMC_PROTECTION_SET, 0, PROT_G3D, SMC_PROTECTION_ENABLE);

	if (!err)
		smc_info.protection_enabled = true;

	spin_unlock_irqrestore(&smc_info.lock, flags);

	if (!err)
		GPU_LOG(MALI_EXYNOS_INFO, "%s: Enter Secure World by GPU\n", __func__);
	else
		GPU_LOG_DETAILED(MALI_EXYNOS_ERROR, LSI_GPU_SECURE, 0u, 0u,
				 "%s: failed to enter secure world ret : %d\n", __func__, err);

	return err;
}

int gpexbe_smc_protection_disable(void)
{
	int err;
	unsigned long flags;

	spin_lock_irqsave(&smc_info.lock, flags);
	if (!smc_info.protection_enabled) {
		spin_unlock_irqrestore(&smc_info.lock, flags);
		return 0;
	}

	err = exynos_smc(SMC_PROTECTION_SET, 0, PROT_G3D, SMC_PROTECTION_DISABLE);

	if (!err)
		smc_info.protection_enabled = false;

	spin_unlock_irqrestore(&smc_info.lock, flags);

	if (!err)
		GPU_LOG(MALI_EXYNOS_INFO, "%s: Exit Secure World by GPU\n", __func__);
	else
		GPU_LOG_DETAILED(MALI_EXYNOS_ERROR, LSI_GPU_SECURE, 0u, 0u,
				 "%s: failed to exit secure world ret : %d\n", __func__, err);

	return err;
}

#if IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_SMC_NOTIFY_GPU)

#ifndef SMC_DRM_G3D_POWER_ON
/* Older kernel versions have SMC_DRM_G3D_POWER_ON as SMC_DRM_G3D_PPCFW_RESTORE
 * but they share same value
 */
#define SMC_DRM_G3D_POWER_ON SMC_DRM_G3D_PPCFW_RESTORE
#endif

void gpexbe_smc_notify_power_on()
{
	exynos_smc(SMC_DRM_G3D_POWER_ON, 0, 0, 0);
}

void gpexbe_smc_notify_power_off()
{
	exynos_smc(SMC_DRM_G3D_POWER_OFF, 0, 0, 0);
}
#else
void gpexbe_smc_notify_power_on(void)
{
}
void gpexbe_smc_notify_power_off(void)
{
}
#endif

int gpexbe_smc_init(void)
{
	spin_lock_init(&smc_info.lock);
	smc_info.protection_enabled = false;

	gpex_utils_get_exynos_context()->smc_info = &smc_info;

	return 0;
}

void gpexbe_smc_term(void)
{
	gpexbe_smc_protection_disable();
	smc_info.protection_enabled = false;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /* SPDX-License-Identifier: GPL-2.0 */

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

#include <mali_kbase.h>
#include <gpexbe_utilization.h>
#include <gpex_gts.h>

/* Mali pm metrics uses 256ns as a unit */
#define KBASE_PM_TIME_SHIFT 8

struct _util_info {
	struct kbase_device *kbdev;
	int pure_compute_time_rate;
	atomic_t time_compute_jobs;
	atomic_t time_vertex_jobs;
	atomic_t time_fragment_jobs;
	atomic_t cnt_compute_jobs;
	atomic_t cnt_fragment_jobs;
	atomic_t cnt_vertex_jobs;
	int cur_utilization;
};

static struct _util_info util_info;

static inline void atomic_add_shifted(u64 val, atomic_t *res)
{
	atomic_add(val >> KBASE_PM_TIME_SHIFT, res);
}

static inline void update_compute_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_compute_jobs);
}

static inline void update_fragment_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_fragment_jobs);
}

static inline void update_vertex_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_vertex_jobs);
}

static inline void increment_compute_job_cnt(void)
{
	atomic_inc(&util_info.cnt_compute_jobs);
}

static inline void increment_fragment_job_cnt(void)
{
	atomic_inc(&util_info.cnt_fragment_jobs);
}

static inline void increment_vertex_job_cnt(void)
{
	atomic_inc(&util_info.cnt_vertex_jobs);
}

static inline bool is_pure_compute_job(struct kbase_jd_atom *katom)
{
	return katom->core_req & BASE_JD_REQ_ONLY_COMPUTE;
}

static inline bool is_fragment_job(struct kbase_jd_atom *katom)
{
	return katom->core_req & BASE_JD_REQ_FS;
}

static inline bool is_compute_job(struct kbase_jd_atom *katom)
{
	/* Includes vertex shader, geometry shader and actual compute shader job */
	return katom->core_req & BASE_JD_REQ_CS;
}

/* Precondition: katom and end_timestamp are not NULL */
void gpexbe_utilization_update_job_load(struct kbase_jd_atom *katom, ktime_t *end_timestamp)
{
	u64 ns_spent = ktime_to_ns(ktime_sub(*end_timestamp, katom->start_timestamp));

	if (is_pure_compute_job(katom)) {
		update_compute_job_load(ns_spent);
		increment_compute_job_cnt();
	} else if (is_fragment_job(katom)) {
		update_fragment_job_load(ns_spent);
		increment_fragment_job_cnt();
	} else if (is_compute_job(katom)) {
		update_vertex_job_load(ns_spent);
		increment_vertex_job_cnt();
	}
}

int gpexbe_utilization_get_compute_job_time(void)
{
	return atomic_read(&util_info.time_compute_jobs);
}

int gpexbe_utilization_get_vertex_job_time(void)
{
	return atomic_read(&util_info.time_vertex_jobs);
}

int gpexbe_utilization_get_fragment_job_time(void)
{
	return atomic_read(&util_info.time_fragment_jobs);
}

int gpexbe_utilization_get_compute_job_cnt(void)
{
	return atomic_read(&util_info.cnt_compute_jobs);
}

int gpexbe_utilization_get_vertex_job_cnt(void)
{
	return atomic_read(&util_info.cnt_vertex_jobs);
}

int gpexbe_utilization_get_fragment_job_cnt(void)
{
	return atomic_read(&util_info.cnt_fragment_jobs);
}

int gpexbe_utilization_get_utilization(void)
{
	return util_info.cur_utilization;
}

int gpexbe_utilization_get_pure_compute_time_rate(void)
{
	return util_info.pure_compute_time_rate;
}

void gpexbe_utilization_calculate_compute_ratio(void)
{
	int compute_time = atomic_read(&util_info.time_compute_jobs);
	int vertex_time = atomic_read(&util_info.time_vertex_jobs);
	int fragment_time = atomic_read(&util_info.time_fragment_jobs);
	int total_time = compute_time + vertex_time + fragment_time;

	if (compute_time > 0 && total_time > 0)
		util_info.pure_compute_time_rate = (100 * compute_time) / total_time;
	else
		util_info.pure_compute_time_rate = 0;

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
}

/* TODO: Refactor this function */
int gpexbe_utilization_calc_utilization(void)
{
	unsigned long flags;
	int utilisation = 0;
	struct kbase_device *kbdev = util_info.kbdev;

	ktime_t now = ktime_get();
	ktime_t diff;
	u32 ns_time;

	spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
	diff = ktime_sub(now, kbdev->pm.backend.metrics.time_period_start);
	ns_time = (u32)(ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);

	if (kbdev->pm.backend.metrics.gpu_active) {
		kbdev->pm.backend.metrics.values.time_busy += ns_time;
		/* TODO: busy_cl can be a static global here */
		kbdev->pm.backend.metrics.values.busy_cl[0] +=
			ns_time * kbdev->pm.backend.metrics.active_cl_ctx[0];
		kbdev->pm.backend.metrics.values.busy_cl[1] +=
			ns_time * kbdev->pm.backend.metrics.active_cl_ctx[1];

		kbdev->pm.backend.metrics.time_period_start = now;
	} else {
		kbdev->pm.backend.metrics.values.time_idle += ns_time;
		kbdev->pm.backend.metrics.time_period_start = now;
	}

	gpex_gts_update_jobslot_util(kbdev->pm.backend.metrics.gpu_active, ns_time);

	spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);

	if (kbdev->pm.backend.metrics.values.time_idle +
		    kbdev->pm.backend.metrics.values.time_busy ==
	    0) {
		/* No data - so we return NOP */
		utilisation = -1;
		goto out;
	}

	utilisation = (100 * kbdev->pm.backend.metrics.values.time_busy) /
		      (kbdev->pm.backend.metrics.values.time_idle +
		       kbdev->pm.backend.metrics.values.time_busy);

	gpex_gts_update_gpu_data();

out:
	spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
	kbdev->pm.backend.metrics.values.time_idle = 0;
	kbdev->pm.backend.metrics.values.time_busy = 0;
	kbdev->pm.backend.metrics.values.busy_cl[0] = 0;
	kbdev->pm.backend.metrics.values.busy_cl[1] = 0;
	kbdev->pm.backend.metrics.values.busy_gl = 0;

	gpex_gts_clear();
	util_info.cur_utilization = utilisation;
	spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);

	return utilisation;
}

int gpexbe_utilization_init(struct device **dev)
{
	util_info.kbdev = container_of(dev, struct kbase_device, dev);

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
	atomic_set(&util_info.cnt_compute_jobs, 0);
	atomic_set(&util_info.cnt_fragment_jobs, 0);
	atomic_set(&util_info.cnt_vertex_jobs, 0);

	util_info.pure_compute_time_rate = 0;

	return 0;
}

void gpexbe_utilization_term(void)
{
	util_info.kbdev = NULL;

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
	atomic_set(&util_info.cnt_compute_jobs, 0);
	atomic_set(&util_info.cnt_fragment_jobs, 0);
	atomic_set(&util_info.cnt_vertex_jobs, 0);

	util_info.pure_compute_time_rate = 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         // SPDX-License-Identifier: GPL-2.0

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
#include <gpex_debug.h>

int gpex_debug_init(struct device **dev)
{
	return 0;
}

void gpex_debug_dump_hist(enum hist_type ht)
{
	return;
}

void gpex_debug_new_record(enum hist_type ht)
{
	return;
}

void gpex_debug_record_time(enum hist_type ht)
{
	return;
}

void gpex_debug_record_prev_data(enum hist_type ht, int prev_data)
{
	return;
}

void gpex_debug_record_new_data(enum hist_type ht, int new_data)
{
	return;
}

void gpex_debug_record_code(enum hist_type ht, int code)
{
	return;
}

void gpex_debug_record(enum hist_type ht, int prev_data, int new_data, int code)
{
	return;
}

void gpex_debug_incr_error_cnt(enum hist_type ht)
{
	return;
}

void gpex_debug_dump_error_cnt(void)
{
	return;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     // SPDX-License-Identifier: GPL-2.0

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
#include <gpex_debug.h>

/* Uses */
#include <gpex_utils.h>
#include <mali_kbase.h>
#include <linux/ktime.h>

struct record {
	int prev_data;
	int new_data;
	struct timespec64 ts;
	int code;
};

#define CLK_HIST_SIZE 8
#define LLC_HIST_SIZE 8
#define BTS_HIST_SIZE 8
#define RTPM_HIST_SIZE 8
#define SUSPEND_HIST_SIZE 8

static const char* hist_name[HIST_TYPE_SIZE] = {"clk", "llc", "bts", "rtpm", "suspend"};
static const int hist_size[HIST_TYPE_SIZE] = {
	CLK_HIST_SIZE, LLC_HIST_SIZE, BTS_HIST_SIZE, RTPM_HIST_SIZE, SUSPEND_HIST_SIZE };
static struct record * hist_arr[HIST_TYPE_SIZE];

static struct _debug_info {
	int hist_idx[HIST_TYPE_SIZE];
	int hist_errors[HIST_TYPE_SIZE];

	struct record clk_hist[CLK_HIST_SIZE];
	struct record llc_hist[LLC_HIST_SIZE];
	struct record bts_hist[BTS_HIST_SIZE];
	struct record rtpm_hist[RTPM_HIST_SIZE];
	struct record suspend_hist[SUSPEND_HIST_SIZE];

	struct device *dev;
} debug_info;

int gpex_debug_init(struct device **dev)
{
	int idx;

	debug_info.dev = *dev;

	hist_arr[0] = debug_info.clk_hist;
	hist_arr[1] = debug_info.llc_hist;
	hist_arr[2] = debug_info.bts_hist;
	hist_arr[3] = debug_info.rtpm_hist;
	hist_arr[4] = debug_info.suspend_hist;

	for (idx = 0; idx < HIST_TYPE_SIZE; idx++) {
		memset(hist_arr[idx], -1, sizeof(*hist_arr[idx]) * hist_size[idx]);
	}

	gpex_utils_get_exynos_context()->debug_info = &debug_info;

	return 0;
}

void gpex_debug_dump_hist(enum hist_type ht)
{
	int idx = debug_info.hist_idx[ht];
	int len = hist_size[ht];

	if (debug_info.dev == NULL)
		return;

	while (len-- > 0) {
		struct record rec = hist_arr[ht][idx];
		dev_warn(debug_info.dev, "%s,%d.%.6d,%d,%d,%d",
				hist_name[ht],
				(int)rec.ts.tv_sec,
				(int)(rec.ts.tv_nsec / 1000),
				rec.prev_data,
				rec.new_data,
				rec.code);

		idx--;
		if (idx < 0)
			idx = hist_size[ht]  - 1;
	}
}

static inline struct record *get_record(enum hist_type ht)
{
	return &hist_arr[ht][debug_info.hist_idx[ht]];
}

void gpex_debug_new_record(enum hist_type ht)
{
	debug_info.hist_idx[ht]++;
	if (debug_info.hist_idx[ht] >= hist_size[ht])
		debug_info.hist_idx[ht] = 0;

	memset(get_record(ht), 0xAA, sizeof(struct record));
}

void gpex_debug_record_time(enum hist_type ht)
{
	ktime_get_ts64(&get_record(ht)->ts);
}

void gpex_debug_record_prev_data(enum hist_type ht, int prev_data)
{
	get_record(ht)->prev_data = prev_data;
}

void gpex_debug_record_new_data(enum hist_type ht, int new_data)
{
	get_record(ht)->new_data = new_data;
}

void gpex_debug_record_code(enum hist_type ht, int code)
{
	get_record(ht)->code = code;
}

void gpex_debug_record(enum hist_type ht, int prev_data, int new_data, int code)
{
	struct record *rec = get_record(ht);

	ktime_get_ts64(&rec->ts);
	rec->prev_data = prev_data;
	rec->new_data = new_data;
	rec->code = code;
}

void gpex_debug_incr_error_cnt(enum hist_type ht)
{
	debug_info.hist_errors[ht]++;
}

void gpex_debug_dump_error_cnt(void)
{
	int idx = 0;

	if (debug_info.dev == NULL)
		return;

	for (idx = 0; idx < HIST_TYPE_SIZE; idx++) {
		dev_warn(debug_info.dev, "%s errors: %d", hist_name[idx], debug_info.hist_errors[idx]);
	}
}
                                                                                                                                                                       /* SPDX-License-Identifier: GPL-2.0 */

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
#include <gpex_utils.h>

/* Uses */
#include <linux/device.h>
#include <linux/sysfs.h>

#include <mali_kbase.h>

#define MAX_ATTRS 128
#define SYSFS_KOBJECT_GROUP_NAME "gpu"

struct kbase_device *pkbdev;
struct exynos_context mali_exynos_ctx;

struct _utils_info {
	struct device *dev;
	int debug_level;
	sysfs_device_read_func show_gpu_model_cb;
};

static struct _utils_info utils_info;

static struct kobject *external_kobj;
static struct kobj_attribute kobj_attrs[MAX_ATTRS];
static struct attribute *attrs[MAX_ATTRS];
static int kobj_last_attr_idx;

static struct device_attribute device_attrs[MAX_ATTRS];
static int device_last_attr_idx;

/************************************************************************
 * DEVICE SYSFS functions
 ************************************************************************/
static int gpex_utils_sysfs_device_init(void);
static void gpex_utils_sysfs_device_files_remove(int remove_cnt);
static void gpex_utils_sysfs_device_term(void);

static int gpex_utils_sysfs_device_init(void)
{
	device_last_attr_idx = 0;
	return 0;
}

/* Returns number of kobject files added so far */
int gpex_utils_sysfs_device_file_add(char *name, sysfs_device_read_func read_func,
				     sysfs_device_write_func write_func)
{
	int permission = 0;

	if (!read_func && !write_func) {
		/* TODO: print error. no funcs */
		return -EINVAL;
	}

	if (read_func)
		permission |= S_IRUGO;

	if (write_func)
		permission |= S_IWUSR;

	device_attrs[device_last_attr_idx].attr.name = name;
	device_attrs[device_last_attr_idx].attr.mode = permission;
	device_attrs[device_last_attr_idx].show = read_func;
	device_attrs[device_last_attr_idx].store = write_func;

	device_last_attr_idx++;

	return device_last_attr_idx;
}

int gpex_utils_sysfs_device_files_create(void)
{
	int i = 0;

	for (i = 0; i < device_last_attr_idx; i++) {
		if (device_create_file(utils_info.dev, &device_attrs[i])) {
			GPU_LOG(MALI_EXYNOS_ERROR, "couldn't create sysfs file [debug_level]\n");
			goto error_cleanup;
		}
	}

	return 0;

error_cleanup:
	gpex_utils_sysfs_device_files_remove(i);

	return -1;
}

static void gpex_utils_sysfs_device_files_remove(int remove_cnt)
{
	int i = 0;

	for (i = 0; i < remove_cnt; i++) {
		device_remove_file(utils_info.dev, &device_attrs[i]);
	}
}

static void gpex_utils_sysfs_device_term(void)
{
	gpex_utils_sysfs_device_files_remove(device_last_attr_idx);
}

/************************************************************************
 * KOBJECT SYSFS functions
 ************************************************************************/

static int gpex_utils_sysfs_kobject_init(void);
static void gpex_utils_sysfs_kobject_term(void);

static int gpex_utils_sysfs_kobject_init(void)
{
	kobj_last_attr_idx = 0;
	external_kobj = kobject_create_and_add(SYSFS_KOBJECT_GROUP_NAME, kernel_kobj);

	if (!external_kobj) {
		GPU_LOG(MALI_EXYNOS_ERROR, "couldn't create Kobj for group [KERNEL - GPU]\n");
	}

	return 0;
}

/* Returns number of kobject files added so far */
int gpex_utils_sysfs_kobject_file_add(char *name, sysfs_kobject_read_func read_func,
				      sysfs_kobject_write_func write_func)
{
	int permission = 0;

	if (!read_func && !write_func) {
		/* TODO: print error. no funcs */
		return -EINVAL;
	}

	if (read_func)
		permission |= S_IRUGO;

	if (write_func)
		permission |= S_IWUSR;

	kobj_attrs[kobj_last_attr_idx].attr.name = name;
	kobj_attrs[kobj_last_attr_idx].attr.mode = permission;
	kobj_attrs[kobj_last_attr_idx].show = (sysfs_kobject_read_func)read_func;
	kobj_attrs[kobj_last_attr_idx].store = (sysfs_kobject_write_func)write_func;

	attrs[kobj_last_attr_idx] = &kobj_attrs[kobj_last_attr_idx].attr;
	kobj_last_attr_idx++;

	return kobj_last_attr_idx;
}

int gpex_utils_sysfs_kobject_files_create(void)
{
	int err = 0;

	static struct attribute_group attr_group = {
		.attrs = attrs,
	};

	err = sysfs_create_group(external_kobj, &attr_group);

	return err;
}

static void gpex_utils_sysfs_kobject_term(void)
{
	if (external_kobj)
		kobject_put(external_kobj);
}

/************************************************************************
 * MALI EXYNOS UTILS SYSFS FUNCTIONS
 ************************************************************************/

void gpex_utils_sysfs_set_gpu_model_callback(sysfs_device_read_func show_gpu_model_fn)
{
	utils_info.show_gpu_model_cb = show_gpu_model_fn;
}

static ssize_t show_gpu_model(char *buf)
{
	if (utils_info.show_gpu_model_cb)
		return utils_info.show_gpu_model_cb(utils_info.dev, NULL, buf);

	return 0;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_gpu_model);

static ssize_t show_debug_level(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "[Current] %d (%d ~ %d)",
			utils_info.debug_level, MALI_EXYNOS_DEBUG_START + 1,
			MALI_EXYNOS_DEBUG_END - 1);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_debug_level);
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_debug_level);

static ssize_t set_debug_level(const char *buf, size_t count)
{
	int debug_level, ret;

	ret = kstrtoint(buf, 0, &debug_level);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((debug_level <= MALI_EXYNOS_DEBUG_START) || (debug_level >= MALI_EXYNOS_DEBUG_END)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid debug level (%d)\n", __func__,
			debug_level);
		return -ENOENT;
	}

	utils_info.debug_level = debug_level;

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_debug_level);
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_debug_level);

/****************
 * GETTERS
 * *************/
int gpex_utils_get_debug_level(void)
{
	return utils_info.debug_level;
}

struct device *gpex_utils_get_device(void)
{
	return utils_info.dev;
}

struct kbase_device *gpex_utils_get_kbase_device(void)
{
	return pkbdev;
}

struct exynos_context *gpex_utils_get_exynos_context(void)
{
	return &mali_exynos_ctx;
}

/************************************************************************
 * INIT and TERM functions
 ************************************************************************/

int gpex_utils_init(struct device **dev)
{
	utils_info.dev = *dev;
	pkbdev = container_of(dev, struct kbase_device, dev);

	utils_info.debug_level = WARNING;

	utils_info.show_gpu_model_cb = NULL;

	gpex_utils_sysfs_kobject_init();
	gpex_utils_sysfs_device_init();

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(debug_level, show_debug_level, set_debug_level);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(debug_level, show_debug_level, set_debug_level);

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_model, show_gpu_model);

	mali_exynos_ctx.utils_info = &utils_info;

	return 0;
}

void gpex_utils_term(void)
{
	gpex_utils_sysfs_kobject_term();
	gpex_utils_sysfs_device_term();
	utils_info.dev = NULL;
	utils_info.show_gpu_model_cb = NULL;

	memset(&mali_exynos_ctx, 0, sizeof(struct exynos_context));
}
                                                                                                                                                                                                                                                                                                                                                                                               /* SPDX-License-Identifier: GPL-2.0 */

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
#include <gpex_clboost.h>

int gpex_clboost_check_activation_condition(void)
{
	return 0;
}

void gpex_clboost_set_state(clboost_state state)
{
	return;
}

int gpex_clboost_init(void)
{
	return 0;
}

void gpex_clboost_term(void)
{
	return;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /* SPDX-License-Identifier: GPL-2.0 */

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

#include <linux/module.h>
#include <gpex_clock.h>

struct clk;
_Bool __clk_is_enabled(struct clk *clk)
{
	return 1;
}
EXPORT_SYMBOL(__clk_is_enabled);

int gpex_clock_init(struct device **dev)
{
	return 0;
}

void gpex_clock_term(void)
{
	return;
}

int gpex_clock_prepare_runtime_off(void)
{
	return 0;
}

int gpex_clock_set(int clk)
{
	return 0;
}

int gpex_clock_lock_clock(gpex_clock_lock_cmd_t lock_command, gpex_clock_lock_type_t lock_type,
			  int clock)
{
	return 0;
}

void gpex_clock_set_user_min_lock_input(int clock)
{
	return;
}

int gpex_clock_get_clock_slow(void)
{
	return 0;
}

int gpex_clock_get_table_idx(int clock)
{
	return 0;
}

int gpex_clock_get_table_idx_cur(void)
{
	return 0;
}

int gpex_clock_get_boot_clock(void)
{
	return 0;
}

int gpex_clock_get_max_clock(void)
{
	return 0;
}

int gpex_clock_get_max_clock_limit(void)
{
	return 0;
}

int gpex_clock_get_min_clock(void)
{
	return 0;
}

int gpex_clock_get_cur_clock(void)
{
	return 0;
}

int gpex_clock_get_min_lock(void)
{
	return 0;
}

int gpex_clock_get_max_lock(void)
{
	return 0;
}

void gpex_clock_mutex_lock(void)
{
	return;
}

void gpex_clock_mutex_unlock(void)
{
	return;
}

int gpex_clock_get_voltage(int clk)
{
	return 0;
}

u64 gpex_clock_get_time(int level)
{
	return 0;
}

u64 gpex_clock_get_time_busy(int level)
{
	return 0;
}

int gpex_clock_get_clock(int level)
{
	return 0;
}
int gpex_get_valid_gpu_clock(int clock, bool is_round_up)
{
	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         // SPDX-License-Identifier: GPL-2.0

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

int gpex_cmar_sched_set_forced_sched(int mode)
{
	return 0;
}

int gpex_cmar_sched_set_affinity(void)
{
	return 0;
}

int gpex_cmar_sched_init(void)
{
	return 0;
}

void gpex_cmar_sched_term(void)
{
	return;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /* SPDX-License-Identifier: GPL-2.0 */

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

#include <linux/device.h>
#include <gpex_dvfs.h>
#include "../gpex_dvfs_internal.h"

int gpex_dvfs_init(struct device **dev)
{
	return 0;
}

void gpex_dvfs_term(void)
{
	return;
}

int gpex_dvfs_set_clock_callback(void)
{
	return 0;
}

int gpex_dvfs_enable(void)
{
	return 0;
}

int gpex_dvfs_disable(void)
{
	return 0;
}

void gpex_dvfs_start(void)
{
	return;
}

void gpex_dvfs_stop(void)
{
	return;
}

int gpex_dvfs_set_polling_speed(int polling_speed)
{
	return 0;
}

int gpex_dvfs_get_status(void)
{
	return 0;
}

int gpex_dvfs_get_governor_type(void)
{
	return 0;
}

int gpex_dvfs_get_utilization(void)
{
	return 0;
}

int gpex_dvfs_get_polling_speed(void)
{
	return 0;
}

void gpex_dvfs_spin_lock(unsigned long *flags)
{
	return;
}

void gpex_dvfs_spin_unlock(unsigned long *flags)
{
	return;
}

// for external
int gpu_dvfs_get_clock(int level)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_clock);

int gpu_dvfs_get_voltage(int clock)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_voltage);

int gpu_dvfs_get_step(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_step);

int gpu_dvfs_get_cur_clock(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_cur_clock);

int gpu_dvfs_get_utilization(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_utilization);

int gpu_dvfs_get_min_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_min_freq);

int gpu_dvfs_get_max_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_max_freq);

int gpu_dvfs_get_min_locked_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_min_locked_freq);

int gpu_dvfs_get_max_locked_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_max_locked_freq);

int gpu_dvfs_get_sustainable_info_array(int index)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_sustainable_info_array);

int gpu_dvfs_get_max_lock(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_max_lock);

bool gpu_dvfs_get_need_cpu_qos(void)
{
	return true;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_need_cpu_qos);

int gpex_dvfs_external_init(struct dvfs_info *_dvfs)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpex_dvfs_external_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /* SPDX-License-Identifier: GPL-2.0 */

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

#include <linux/module.h>
#include <gpex_gts.h>

int gpex_gts_get_ioctl_gts_info(struct mali_exynos_ioctl_gts_info *info)
{
	return 0;
}
void gpex_gts_update_jobslot_util(bool gpu_active, u32 ns_time)
{
	return;
}

void gpex_gts_set_jobslot_status(bool is_active)
{
	return;
}

void gpex_gts_clear(void)
{
	return;
}

void gpex_gts_set_hcm_mode(int hcm_mode_val)
{
	return;
}

int gpex_gts_get_hcm_mode(void)
{
	return 0;
}

int gpex_gts_init(struct device **dev)
{
	return 0;
}

void gpex_gts_term(void)
{
	return;
}

bool gpex_gts_update_gpu_data(void)
{
	return true;
}

void gpu_register_out_data(void (*fn)(u64 *cnt))
{
	return;
}
EXPORT_SYMBOL_GPL(gpu_register_out_data);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /* SPDX-License-Identifier: GPL-2.0 */

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

#include <gpex_ifpo.h>

int gpex_ifpo_init(void)
{
	return 0;
}

void gpex_ifpo_term(void)
{
	return;
}

int gpex_ifpo_power_up(void)
{
	return 0;
}

int gpex_ifpo_power_down(void)
{
	return 0;
}

ifpo_status gpex_ifpo_get_status(void)
{
	return IFPO_POWER_UP;
}

ifpo_mode gpex_ifpo_get_mode(void)
{
	return IFPO_DISABLED;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /* SPDX-License-Identifier: GPL-2.0 */

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

#include <mali_kbase_config.h>

static struct kbase_platform_config dummy_platform_config;

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &dummy_platform_config;
}

int kbase_platform_register(void)
{
	return 0;
}

void kbase_platform_unregister(void)
{
	return;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /* SPDX-License-Identifier: GPL-2.0 */

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

int gpex_pm_init(void)
{
	return 0;
}

void gpex_pm_term(void)
{
	return;
}

int gpex_pm_get_status(bool clock_lock)
{
	return 1;
}

int gpex_pm_get_power_status(void)
{
	return 1;
}

void gpex_pm_lock(void)
{
	return;
}

void gpex_pm_unlock(void)
{
	return;
}

int gpex_pm_set_state(int state)
{
	return 0;
}

int gpex_pm_get_state(int *state)
{
	return 0;
}

/***************************
 * RTPM helper functions
 **************************/
int gpex_pm_power_on(struct device *dev)
{
	gpex_dvfs_start();

	return 0;
}

void gpex_pm_power_autosuspend(struct device *dev)
{
	return;
}

void gpex_pm_suspend(struct device *dev)
{
	return;
}

int gpex_pm_runtime_init(struct device *dev)
{
	return 0;
}

void gpex_pm_runtime_term(struct device *dev)
{
	return;
}

void gpex_pm_runtime_off_prepare(struct device *dev)
{
	gpex_dvfs_stop();

	return;
}

int gpex_pm_runtime_on_prepare(struct device *dev)
{
	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /* SPDX-License-Identifier: GPL-2.0 */

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

#include <gpex_qos.h>

int gpex_qos_init(void)
{
	return 0;
}

void gpex_qos_term(void)
{
	return;
}

int gpex_qos_set(gpex_qos_flag flags, int val)
{
	return 0;
}

int gpex_qos_unset(gpex_qos_flag flags)
{
	return 0;
}

int gpex_qos_set_from_clock(int clk)
{
	return 0;
}

int gpex_qos_set_bts_mo(int gpu_clock)
{
	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /* SPDX-License-Identifier: GPL-2.0 */

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

#include <gpex_thermal.h>

int gpex_thermal_init(void)
{
	return 0;
}

void gpex_thermal_term(void)
{
	return;
}

int gpex_thermal_gpu_normal(void)
{
	return 0;
}

int gpex_thermal_gpu_throttle(int frequency)
{
	return 0;
}

void gpex_thermal_set_status(bool status)
{
	return;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /* SPDX-License-Identifier: GPL-2.0 */

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

#include <linux/ktime.h>
#include <gpex_utils.h>
#include <gpex_tsg.h>

#define DVFS_TABLE_ROW_MAX 9

static ktime_t kt;

int exynos_stats_get_gpu_cur_idx(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_cur_idx);

int exynos_stats_get_gpu_coeff(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_coeff);

uint32_t exynos_stats_get_gpu_table_size(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_table_size);

/* TODO: so far, this stub function is dependant to product, so needs to be diverged for each product */
/* or, just return zero value when GPU Profiler is disabled. */
uint32_t freqs[DVFS_TABLE_ROW_MAX];
uint32_t *exynos_stats_get_gpu_freq_table(void)
{
	freqs[0] = 858000;
	freqs[1] = 767000;
	freqs[2] = 676000;
	freqs[3] = 585000;
	freqs[4] = 494000;
	freqs[5] = 403000;
	freqs[6] = 312000;
	freqs[7] = 221000;
	freqs[8] = 130000;

	return freqs;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_freq_table);

uint32_t volts[DVFS_TABLE_ROW_MAX];
uint32_t *exynos_stats_get_gpu_volt_table(void)
{
	return volts;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_volt_table);

ktime_t time_in_state[DVFS_TABLE_ROW_MAX];
ktime_t *exynos_stats_get_gpu_time_in_state(void)
{
	return time_in_state;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_time_in_state);

int exynos_stats_get_gpu_max_lock(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_max_lock);

int exynos_stats_get_gpu_min_lock(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_min_lock);

int exynos_stats_set_queued_threshold_0(unsigned int threshold)
{
	CSTD_UNUSED(threshold);
	return 0;
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_0);

int exynos_stats_set_queued_threshold_1(unsigned int threshold)
{
	CSTD_UNUSED(threshold);
	return 0;
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_1);

ktime_t *exynos_stats_get_gpu_queued_job_time(void)
{
	return NULL;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_queued_job_time);

ktime_t exynos_stats_get_gpu_queued_last_updated(void)
{
	return kt;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_queued_last_updated);

void exynos_stats_set_gpu_polling_speed(int polling_speed)
{
	CSTD_UNUSED(polling_speed);
}
EXPORT_SYMBOL(exynos_stats_set_gpu_polling_speed);

int exynos_stats_get_gpu_polling_speed(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_polling_speed);

void exynos_migov_set_mode(int mode)
{
	CSTD_UNUSED(mode);
}
EXPORT_SYMBOL(exynos_migov_set_mode);

void exynos_migov_set_gpu_margin(int margin)
{
	CSTD_UNUSED(margin);
}
EXPORT_SYMBOL(exynos_migov_set_gpu_margin);

int register_frag_utils_change_notifier(struct notifier_block *nb)
{
	CSTD_UNUSED(nb);
	return 0;
}
EXPORT_SYMBOL(register_frag_utils_change_notifier);

int unregister_frag_utils_change_notifier(struct notifier_block *nb)
{
	CSTD_UNUSED(nb);
	return 0;
}
EXPORT_SYMBOL(unregister_frag_utils_change_notifier);

void gpex_tsg_set_migov_mode(int mode)
{
	CSTD_UNUSED(mode);
}

void gpex_tsg_set_freq_margin(int margin)
{
	CSTD_UNUSED(margin);
}

void gpex_tsg_set_util_history(int idx, int order, int input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(order);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_weight_util(int idx, int input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_weight_freq(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_en_signal(bool input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_pmqos(bool pm_qos_tsg)
{
	CSTD_UNUSED(pm_qos_tsg);
}

void gpex_tsg_set_weight_table_idx(int idx, int input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_is_gov_set(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_saved_polling_speed(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_governor_type_init(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_amigo_flags(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queued_threshold(int idx, uint32_t input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queued_time_tick(int idx, ktime_t input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queued_time(int idx, ktime_t input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queued_last_updated(ktime_t input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queue_nr(int type, int variation)
{
	CSTD_UNUSED(type);
	CSTD_UNUSED(variation);
}

int gpex_tsg_get_migov_mode(void)
{
	return 0;
}

int gpex_tsg_get_freq_margin(void)
{
	return 0;
}

bool gpex_tsg_get_pmqos(void)
{
	return false;
}

int gpex_tsg_get_util_history(int idx, int order)
{
	return 0;
}

int gpex_tsg_get_weight_util(int idx)
{
	return 0;
}

int gpex_tsg_get_weight_freq(void)
{
	return 0;
}

bool gpex_tsg_get_en_signal(void)
{
	return 0;
}

int gpex_tsg_get_weight_table_idx(int idx)
{
	return 0;
}

int gpex_tsg_get_is_gov_set(void)
{
	return 0;
}

int gpex_tsg_get_saved_polling_speed(void)
{
	return 0;
}

int gpex_tsg_get_governor_type_init(void)
{
	return 0;
}

int gpex_tsg_get_amigo_flags(void)
{
	return 0;
}

uint32_t gpex_tsg_get_queued_threshold(int idx)
{
	CSTD_UNUSED(idx);
	return 0;
}

ktime_t gpex_tsg_get_queued_time_tick(int idx)
{
	CSTD_UNUSED(idx);
	return kt;
}

ktime_t gpex_tsg_get_queued_time(int idx)
{
	CSTD_UNUSED(idx);
	return kt;
}

ktime_t *gpex_tsg_get_queued_time_array(void)
{
	return &kt;
}

ktime_t gpex_tsg_get_queued_last_updated(void)
{
	return kt;
}

int gpex_tsg_get_queue_nr(int type)
{
	CSTD_UNUSED(type);
	return 0;
}

struct atomic_notifier_head *gpex_tsg_get_frag_utils_change_notifier_list(void)
{
	return 0;
}

int gpex_tsg_notify_frag_utils_change(u32 js0_utils)
{
	CSTD_UNUSED(js0_utils);
	return 0;
}

int gpex_tsg_spin_lock(void)
{
	return 0;
}

void gpex_tsg_spin_unlock(void)
{
	return;
}

int gpex_tsg_reset_count(int powered)
{
	return 0;
}

int gpex_tsg_set_count(u32 status, bool stop)
{
	return 0;
}

int gpex_tsg_init(struct device **dev)
{
	memset(&kt, 0, sizeof(ktime_t));
	return 0;
}
int gpex_tsg_term(void)
{
	return 0;
}

void gpex_tsg_input_nr_acc_cnt(void)
{
	return;
}

void gpex_tsg_reset_acc_count(void)
{
	return;
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
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /* SPDX-License-Identifier: GPL-2.0 */

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

#include <linux/ktime.h>
#include <gpex_utils.h>
#include <gpex_tsg.h>

#include <soc/samsung/exynos-profiler.h>

#define DVFS_TABLE_ROW_MAX 9

static ktime_t kt;

int exynos_stats_get_gpu_cur_idx(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_cur_idx);

int exynos_stats_get_gpu_coeff(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_coeff);

uint32_t exynos_stats_get_gpu_table_size(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_table_size);

/* TODO: so far, this stub function is dependant to product, so needs to be diverged for each product */
/* or, just return zero value when GPU Profiler is disabled. */
uint32_t freqs[DVFS_TABLE_ROW_MAX];
uint32_t *gpu_dvfs_get_freq_table(void)
{
	freqs[0] = 858000;
	freqs[1] = 767000;
	freqs[2] = 676000;
	freqs[3] = 585000;
	freqs[4] = 494000;
	freqs[5] = 403000;
	freqs[6] = 312000;
	freqs[7] = 221000;
	freqs[8] = 130000;

	return freqs;
}
EXPORT_SYMBOL(gpu_dvfs_get_freq_table);

uint32_t volts[DVFS_TABLE_ROW_MAX];
uint32_t *exynos_stats_get_gpu_volt_table(void)
{
	return volts;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_volt_table);

ktime_t time_in_state[DVFS_TABLE_ROW_MAX];
ktime_t tis_last_update;
ktime_t *gpu_dvfs_get_time_in_state(void)
{
	return time_in_state;
}
EXPORT_SYMBOL(gpu_dvfs_get_time_in_state);

ktime_t gpu_dvfs_get_tis_last_update(void)
{
	return tis_last_update;
}
EXPORT_SYMBOL(gpu_dvfs_get_tis_last_update);

int gpu_dvfs_get_max_freq(void)
{
	return 0;
}
EXPORT_SYMBOL(gpu_dvfs_get_max_freq);

int gpu_dvfs_get_min_freq(void)
{
	return 0;
}
EXPORT_SYMBOL(gpu_dvfs_get_min_freq);

int exynos_stats_set_queued_threshold_0(unsigned int threshold)
{
	CSTD_UNUSED(threshold);
	return 0;
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_0);

int exynos_stats_set_queued_threshold_1(unsigned int threshold)
{
	CSTD_UNUSED(threshold);
	return 0;
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_1);

ktime_t *gpu_dvfs_get_job_queue_count(void)
{
	return NULL;
}
EXPORT_SYMBOL(gpu_dvfs_get_job_queue_count);

ktime_t gpu_dvfs_get_job_queue_last_updated(void)
{
	return 0;
}
EXPORT_SYMBOL(gpu_dvfs_get_job_queue_last_updated);

void exynos_stats_set_gpu_polling_speed(int polling_speed)
{
	CSTD_UNUSED(polling_speed);
}
EXPORT_SYMBOL(exynos_stats_set_gpu_polling_speed);

int exynos_stats_get_gpu_polling_speed(void)
{
	return 0;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_polling_speed);

void gpu_dvfs_set_amigo_governor(int mode)
{
	CSTD_UNUSED(mode);
}
EXPORT_SYMBOL(gpu_dvfs_set_amigo_governor);

void gpu_dvfs_set_freq_margin(int margin)
{
	CSTD_UNUSED(margin);
}
EXPORT_SYMBOL(gpu_dvfs_set_freq_margin);

void exynos_stats_get_run_times(u64 *times)
{
	CSTD_UNUSED(times);
}
EXPORT_SYMBOL(exynos_stats_get_run_times);

void exynos_stats_get_pid_list(u16 *pidlist)
{
	CSTD_UNUSED(pidlist);
}
EXPORT_SYMBOL(exynos_stats_get_pid_list);

void exynos_stats_set_vsync(ktime_t timestamp)
{
	CSTD_UNUSED(timestamp);
}
EXPORT_SYMBOL(exynos_stats_set_vsync);

void exynos_migov_set_targetframetime(int us)
{
	CSTD_UNUSED(us);
}
EXPORT_SYMBOL(exynos_migov_set_targetframetime);

void exynos_sdp_set_powertable(int id, int cnt, struct freq_table *table)
{
	CSTD_UNUSED(id);
	CSTD_UNUSED(cnt);
	CSTD_UNUSED(table);
}
EXPORT_SYMBOL(exynos_sdp_set_powertable);

void exynos_sdp_set_busy_domain(int id)
{
	CSTD_UNUSED(id);
}
EXPORT_SYMBOL(exynos_sdp_set_busy_domain);

void exynos_sdp_set_cur_freqlv(int id, int idx)
{
	CSTD_UNUSED(idx);
}
EXPORT_SYMBOL(exynos_sdp_set_cur_freqlv);

void exynos_migov_set_targettime_margin(int us)
{
	CSTD_UNUSED(us);
}
EXPORT_SYMBOL(exynos_migov_set_targettime_margin);

void exynos_migov_set_util_margin(int percentage)
{
	CSTD_UNUSED(percentage);
}
EXPORT_SYMBOL(exynos_migov_set_util_margin);

void exynos_migov_set_decon_time(int us)
{
	CSTD_UNUSED(us);
}
EXPORT_SYMBOL(exynos_migov_set_decon_time);

void exynos_migov_set_comb_ctrl(int enable)
{
	CSTD_UNUSED(enable);
}
EXPORT_SYMBOL(exynos_migov_set_comb_ctrl);

int exynos_gpu_stc_config_show(int page_size, char *buf)
{
	CSTD_UNUSED(page_size);
	CSTD_UNUSED(buf);
	return 0;
}
EXPORT_SYMBOL(exynos_gpu_stc_config_show);

int exynos_gpu_stc_config_store(const char *buf)
{
	CSTD_UNUSED(buf);
	return 0;
}
EXPORT_SYMBOL(exynos_gpu_stc_config_store);

int gpu_dvfs_register_utilization_notifier(struct notifier_block *nb)
{
	CSTD_UNUSED(nb);
	return 0;
}
EXPORT_SYMBOL(gpu_dvfs_register_utilization_notifier);

int gpu_dvfs_unregister_utilization_notifier(struct notifier_block *nb)
{
	CSTD_UNUSED(nb);
	return 0;
}
EXPORT_SYMBOL(gpu_dvfs_unregister_utilization_notifier);

void gpex_tsg_set_migov_mode(int mode)
{
	CSTD_UNUSED(mode);
}

void gpex_tsg_set_freq_margin(int margin)
{
	CSTD_UNUSED(margin);
}

void gpex_tsg_set_util_history(int idx, int order, int input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(order);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_weight_util(int idx, int input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_weight_freq(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_en_signal(bool input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_pmqos(bool pm_qos_tsg)
{
	CSTD_UNUSED(pm_qos_tsg);
}

void gpex_tsg_set_weight_table_idx(int idx, int input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_is_gov_set(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_saved_polling_speed(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_governor_type_init(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_amigo_flags(int input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queued_threshold(int idx, uint32_t input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queued_time_tick(int idx, ktime_t input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queued_time(int idx, ktime_t input)
{
	CSTD_UNUSED(idx);
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queued_last_updated(ktime_t input)
{
	CSTD_UNUSED(input);
}

void gpex_tsg_set_queue_nr(int type, int variation)
{
	CSTD_UNUSED(type);
	CSTD_UNUSED(variation);
}

int gpex_tsg_get_migov_mode(void)
{
	return 0;
}

int gpex_tsg_get_freq_margin(void)
{
	return 0;
}

bool gpex_tsg_get_pmqos(void)
{
	return false;
}

int gpex_tsg_get_util_history(int idx, int order)
{
	return 0;
}

int gpex_tsg_get_weight_util(int idx)
{
	return 0;
}

int gpex_tsg_get_weight_freq(void)
{
	return 0;
}

bool gpex_tsg_get_en_signal(void)
{
	return 0;
}

int gpex_tsg_get_weight_table_idx(int idx)
{
	return 0;
}

int gpex_tsg_get_is_gov_set(void)
{
	return 0;
}

int gpex_tsg_get_saved_polling_speed(void)
{
	return 0;
}

int gpex_tsg_get_governor_type_init(void)
{
	return 0;
}

int gpex_tsg_get_amigo_flags(void)
{
	return 0;
}

uint32_t gpex_tsg_get_queued_threshold(int idx)
{
	CSTD_UNUSED(idx);
	return 0;
}

ktime_t gpex_tsg_get_queued_time_tick(int idx)
{
	CSTD_UNUSED(idx);
	return kt;
}

ktime_t gpex_tsg_get_queued_time(int idx)
{
	CSTD_UNUSED(idx);
	return kt;
}

ktime_t *gpex_tsg_get_queued_time_array(void)
{
	return &kt;
}

ktime_t gpex_tsg_get_queued_last_updated(void)
{
	return kt;
}

int gpex_tsg_get_queue_nr(int type)
{
	CSTD_UNUSED(type);
	return 0;
}

struct atomic_notifier_head *gpex_tsg_get_frag_utils_change_notifier_list(void)
{
	return 0;
}

int gpex_tsg_notify_frag_utils_change(u32 js0_utils)
{
	CSTD_UNUSED(js0_utils);
	return 0;
}

int gpex_tsg_spin_lock(void)
{
	return 0;
}

void gpex_tsg_spin_unlock(void)
{
	return;
}

int gpex_tsg_reset_count(int powered)
{
	return 0;
}

int gpex_tsg_set_count(u32 status, bool stop)
{
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

int gpex_tsg_init(struct device **dev)
{
	memset(&kt, 0, sizeof(ktime_t));
	return 0;
}
int gpex_tsg_term(void)
{
	return 0;
}

void gpex_tsg_input_nr_acc_cnt(void)
{
	return;
}

void gpex_tsg_reset_acc_count(void)
{
	return;
}

void exynos_stats_get_frame_info(s32 *nrframe, u64 *nrvsync, u64 *delta_ms)
{
	CSTD_UNUSED(nrframe);
	CSTD_UNUSED(nrvsync);
	CSTD_UNUSED(delta_ms);
}
EXPORT_SYMBOL(exynos_stats_get_frame_info);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /* SPDX-License-Identifier: GPL-2.0 */

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
#include <gpex_clboost.h>

/* Uses */
#include <gpexbe_utilization.h>
#include <gpex_utils.h>

struct _clboost_info {
	int activation_compute_ratio;
	clboost_state state;
};

static struct _clboost_info clboost_info;

int gpex_clboost_check_activation_condition(void)
{
	if (clboost_info.state == CLBOOST_DISABLE)
		return false;

	return gpexbe_utilization_get_pure_compute_time_rate() >=
	       clboost_info.activation_compute_ratio;
}

void gpex_clboost_set_state(clboost_state state)
{
	clboost_info.state = state;
}

static ssize_t sysfs_wrapup(ssize_t count, char *buf)
{
	if (count < PAGE_SIZE - 1) {
		count += snprintf(buf + count, PAGE_SIZE - count, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		count = PAGE_SIZE - 1;
	}

	return count;
}

static ssize_t show_clboost_state(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "enabled(%d) active(%d) compute_ratio(%d)",
			clboost_info.state, gpex_clboost_check_activation_condition(),
			gpexbe_utilization_get_pure_compute_time_rate());

	return sysfs_wrapup(ret, buf);
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_clboost_state)

static ssize_t show_clboost_disable(char *buf)
{
	ssize_t ret = 0;
	bool clboost_disabled = false;

	if (clboost_info.state == CLBOOST_DISABLE)
		clboost_disabled = true;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", clboost_disabled);

	return sysfs_wrapup(ret, buf);
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_clboost_disable)

static ssize_t set_clboost_disable(const char *buf, size_t count)
{
	unsigned int clboost_disable = 0;
	int ret;

	ret = kstrtoint(buf, 0, &clboost_disable);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if (clboost_disable == 0)
		clboost_info.state = CLBOOST_ENABLE;
	else
		clboost_info.state = CLBOOST_DISABLE;

	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_clboost_disable)

int gpex_clboost_init(void)
{
	clboost_info.state = CLBOOST_ENABLE;
	clboost_info.activation_compute_ratio = 100;

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_cl_boost_disable, show_clboost_disable,
					  set_clboost_disable);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_cl_boost_state, show_clboost_state);

	gpex_utils_get_exynos_context()->clboost_info = &clboost_info;

	return 0;
}

void gpex_clboost_term(void)
{
	clboost_info.state = CLBOOST_DISABLE;
	clboost_info.activation_compute_ratio = 101;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /* SPDX-License-Identifier: GPL-2.0 */

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

#include <gpex_clock.h>
#include <gpex_qos.h>
#include <gpex_pm.h>
#include <gpex_dvfs.h>
#include <gpex_ifpo.h>
#include <gpex_utils.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_clock.h>
#include <gpexbe_utilization.h>
#include <gpexbe_debug.h>

#include "gpex_clock_internal.h"

#define CPU_MAX INT_MAX

static struct _clock_info clk_info;

int gpex_clock_get_boot_clock(void)
{
	return clk_info.boot_clock;
}
int gpex_clock_get_max_clock(void)
{
	return clk_info.gpu_max_clock;
}
int gpex_clock_get_max_clock_limit(void)
{
	return clk_info.gpu_max_clock_limit;
}
int gpex_clock_get_min_clock(void)
{
	return clk_info.gpu_min_clock;
}
int gpex_clock_get_cur_clock(void)
{
	return clk_info.cur_clock;
}
int gpex_clock_get_max_lock(void)
{
	return clk_info.max_lock;
}
int gpex_clock_get_min_lock(void)
{
	return clk_info.min_lock;
}
int gpex_clock_get_clock(int level)
{
	return clk_info.table[level].clock;
}
u64 gpex_clock_get_time(int level)
{
	return clk_info.table[level].time;
}
u64 gpex_clock_get_time_busy(int level)
{
	return clk_info.table[level].time_busy;
}
bool gpex_clock_get_unlock_freqs_status(void)
{
	return clk_info.unlock_freqs;
}
/*******************************************
 * static helper functions
 ******************************************/
static int gpex_clock_update_config_data_from_dt(void)
{
	dt_clock_item *dt_clock_table = gpexbe_devicetree_get_clock_table();
	int ret = 0;
	struct freq_volt *fv_array;
	int asv_lv_num;
	int i, j;
	int new_size;

	clk_info.gpu_max_clock = 1100000;
	clk_info.gpu_min_clock = 450000;
	clk_info.boot_clock = 450000;
	clk_info.gpu_max_clock_limit = 1100000;

	/* TODO: rename the table_size variable to something more sensible like  row_cnt */
	new_size = gpexbe_devicetree_get_int(gpu_dvfs_table_size.row);
	if (!clk_info.table || clk_info.table_size != new_size) {
		clk_info.table_size = new_size;
		clk_info.table = kcalloc(clk_info.table_size, sizeof(gpu_clock_info), GFP_KERNEL);
	}

	asv_lv_num = gpexbe_clock_get_level_num();
	fv_array = kcalloc(asv_lv_num, sizeof(*fv_array), GFP_KERNEL);

	if (!fv_array)
		return -ENOMEM;

	ret = gpexbe_clock_get_rate_asv_table(fv_array, asv_lv_num);
	if (!ret)
		GPU_LOG(MALI_EXYNOS_ERROR, "Failed to get G3D ASV table from CAL IF\n");

	for (j = 0; j < clk_info.table_size; j++) {
		if (dt_clock_table[j].clock <= clk_info.gpu_max_clock && dt_clock_table[j].clock >= clk_info.gpu_min_clock) {
			clk_info.table[j].clock = dt_clock_table[j].clock;
			for (i = 0; i < asv_lv_num; i++)
				if (fv_array[i].freq == clk_info