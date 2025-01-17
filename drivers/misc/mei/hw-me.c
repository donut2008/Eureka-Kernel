lse
		ret = 0;

	if (ret == 0) {
		dprintk(1, "%s() freq=%d\n", __func__, c->frequency);
		ret = mxl5005s_SetRfFreqHz(fe, c->frequency);
	}

	return ret;
}

static int mxl5005s_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	dprintk(1, "%s()\n", __func__);

	*frequency = state->RF_IN;

	return 0;
}

static int mxl5005s_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	dprintk(1, "%s()\n", __func__);

	*bandwidth = state->Chan_Bandwidth;

	return 0;
}

static int mxl5005s_get_if_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	dprintk(1, "%s()\n", __func__);

	*frequency = state->IF_OUT;

	return 0;
}

static int mxl5005s_release(struct dvb_frontend *fe)
{
	dprintk(1, "%s()\n", __func__);
	kfree(fe->tuner_priv);
	fe->tuner_priv = NULL;
	return 0;
}

static const struct dvb_tuner_ops mxl5005s_tuner_ops = {
	.info = {
		.name           = "MaxLinear MXL5005S",
		.frequency_min  =  48000000,
		.frequency_max  = 860000000,
		.frequency_step =     50000,
	},

	.release       = mxl5005s_release,
	.init          = mxl5005s_init,

	.set_params    = mxl5005s_set_params,
	.get_frequency = mxl5005s_get_frequency,
	.get_bandwidth = mxl5005s_get_bandwidth,
	.get_if_frequency = mxl5005s_get_if_frequency,
};

struct dvb_frontend *mxl5005s_attach(struct dvb_frontend *fe,
				     struct i2c_adapter *i2c,
				     struct mxl5005s_config *config)
{
	struct mxl5005s_state *state = NULL;
	dprintk(1, "%s()\n", __func__);

	state = kzalloc(sizeof(struct mxl5005s_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	state->frontend = fe;
	state->config = config;
	state->i2c = i2c;

	printk(KERN_INFO "MXL5005S: Attached at address 0x%02x\n",
		config->i2c_address);

	memcpy(&fe->ops.tuner_ops, &mxl5005s_tuner_ops,
		sizeof(struct dvb_tuner_ops));

	fe->tuner_priv = state;
	return fe;
}
EXPORT_SYMBOL(mxl5005s_attach);

MODULE_DESCRIPTION("MaxLinear MXL5005S silicon tuner driver");
MODULE_AUTHOR("Steven Toth");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 * Copyright 2005-2008 Pierre Ossman
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 */
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/scatterlist.h>
#include <linux/bitops.h>
#include <linux/string_helpers.h>
#include <linux/delay.h>
#include <linux/capability.h>
#include <linux/compat.h>
#include <linux/pm_runtime.h>
#include <linux/ioprio.h>

#include <trace/events/mmc.h>

#include <trace/events/mmc.h>

#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/dw_mmc.h>

#include <asm/uaccess.h>

#include "queue.h"
#include "../host/dw_mmc.h"
#include "../host/dw_mmc-exynos.h"
#if defined(CONFIG_SEC_ABC)  
#include <linux/sti/abc_common.h>
#endif

#ifdef CONFIG_MMC_SUPPORT_STLOG
#include <linux/fslog.h>
#else
#define ST_LOG(fmt,...)
#endif

MODULE_ALIAS("mmc:block");
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX "mmcblk."

#define INAND_CMD38_ARG_EXT_CSD  113
#define INAND_CMD38_ARG_ERASE    0x00
#define INAND_CMD38_ARG_TRIM     0x01
#define INAND_CMD38_ARG_SECERASE 0x80
#define INAND_CMD38_ARG_SECTRIM1 0x81
#define INAND_CMD38_ARG_SECTRIM2 0x88
#define MMC_BLK_TIMEOUT_MS  (30 * 1000)        /* 30 sec timeout */
#define MMC_SANITIZE_REQ_TIMEOUT 240000
#define MMC_EXTRACT_INDEX_FROM_ARG(x) ((x & 0x00FF0000) >> 16)
#define MMC_CMDQ_STOP_TIMEOUT_MS 100
#define MMC_CMDQ_FFU_ENTER	0x031E0100
#define MMC_CMDQ_FFU_EXIT	0x031E0000
#define MMC_BLK_CQ_FFU

#define mmc_req_rel_wr(req)	((req->cmd_flags & REQ_FUA) && \
				  (rq_data_dir(req) == WRITE))
#define PACKED_CMD_VER	0x01
#define PACKED_CMD_WR	0x02

static struct mmc_cmdq_req *mmc_cmdq_prep_dcmd(
		struct mmc_queue_req *mqrq, struct mmc_queue *mq);

static DEFINE_MUTEX(block_mutex);

/*
 * The defaults come from config options but can be overriden by module
 * or bootarg options.
 */
static int perdev_minors = CONFIG_MMC_BLOCK_MINORS;

/*
 * We've only got one major, so number of mmcblk devices is
 * limited to (1 << 20) / number of minors per device.  It is also
 * currently limited by the size of the static bitmaps below.
 */
static int max_devices;

#define MAX_DEVICES 256
#define MAX_RETRIES 2

/* TODO: Replace these with struct ida */
static DECLARE_BITMAP(dev_use, MAX_DEVICES);
static DECLARE_BITMAP(name_use, MAX_DEVICES);

/*
 * There is one mmc_blk_data per slot.
 */
struct mmc_blk_data {
	spinlock_t	lock;
	struct gendisk	*disk;
	struct mmc_queue queue;
	struct list_head part;

	unsigned int	flags;
#define MMC_BLK_CMD23	(1 << 0)	/* Can do SET_BLOCK_COUNT for multiblock */
#define MMC_BLK_REL_WR	(1 << 1)	/* MMC Reliable write support */
#define MMC_BLK_PACKED_CMD	(1 << 2)	/* MMC packed command support */
#define MMC_BLK_CMD_QUEUE	(1 << 3) /* MMC command queue support */

	unsigned int	usage;
	unsigned int	read_only;
	unsigned int	part_type;
	unsigned int	name_idx;
	unsigned int	reset_done;
#define MMC_BLK_READ		BIT(0)
#define MMC_BLK_WRITE		BIT(1)
#define MMC_BLK_DISCARD		BIT(2)
#define MMC_BLK_SECDISCARD	BIT(3)
#define MMC_BLK_FLUSH		BIT(4)

	/*
	 * Only set in main mmc_blk_data associated
	 * with mmc_card with dev_set_drvdata, and keeps
	 * track of the current selected device partition.
	 */
	unsigned int	part_curr;
	struct device_attribute force_ro;
	struct device_attribute power_ro_lock;
	int	area_type;
#if defined(CONFIG_MMC_CQ_HCI) && defined(CONFIG_MMC_DATA_LOG)
	sector_t  mmc_system_start;
	sector_t  mmc_system_end;
	bool mmc_sys_log_en;
#endif
};

static DEFINE_MUTEX(open_lock);

enum {
	MMC_PACKED_NR_IDX = -1,
	MMC_PACKED_NR_ZERO,
	MMC_PACKED_NR_SINGLE,
};

module_param(perdev_minors, int, 0444);
MODULE_PARM_DESC(perdev_minors, "Minors numbers to allocate per device");

static inline int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md);
static int get_card_status(struct mmc_card *card, u32 *status, int retries);
static int mmc_blk_cmdq_switch(struct mmc_card *card,
			       struct mmc_blk_data *md, bool enable);

#if defined(CONFIG_MMC_CQ_HCI) && defined(CONFIG_MMC_DATA_LOG)
#define	MMC_BLK_CMDQ_LOG_MAX	512
#define MMC_BLK_CMDQ_DEPTH_MAX	32

static int queuing_req[MMC_BLK_CMDQ_DEPTH_MAX];

struct mmc_data_log_summary {
	u64 start_time;
	u64	end_time;	
	sector_t	sector;
	u64	*virt_addr;
	int	check;

#define	CMDQ_REQ_BUF_SIZE	8
	char	datbuf[CMDQ_REQ_BUF_SIZE];
};

static struct mmc_data_log_summary cmdq_req_logs[MMC_BLK_CMDQ_LOG_MAX] __cacheline_aligned;

static void mmc_blk_cmdq_store_req_log(struct mmc_card *card, struct request *req,
		struct mmc_data *data, bool isStart, int err)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);
	int cpu = raw_smp_processor_id();
	unsigned int index;
	bool read_dir = (rq_data_dir(req) == READ);
	unsigned long flags;
	char *buffer;	// real data
	int i;
	void  *virt_sg_addr;
	unsigned long  magicword[1];
	int id = 0;

	magicword[0] = 0x1F5E3A7069245CBE;

#if 0
	magicword[1] = 0x4977C72608FCE51F;
	magicword[2] = 0x524E74A9005F0D1B;
	magicword[3] = 0x05D3D11A26CF3142;
#endif

	if((req->__sector >= md->mmc_system_start) && (req->__sector < md->mmc_system_end) && read_dir) {
		if (isStart) {
			for (id = 0; id < MMC_BLK_CMDQ_LOG_MAX; id++){
				index = atomic_inc_return(&card->log_count) & (MMC_BLK_CMDQ_LOG_MAX - 1);
				if (cmdq_req_logs[index].check != 1)
					break;				
			}
			queuing_req[req->tag] = index;
			cmdq_req_logs[index].start_time = cpu_clock(cpu);
			cmdq_req_logs[index].sector = -1;
			memset(cmdq_req_logs[index].datbuf, 0,
					sizeof(cmdq_req_logs[index].datbuf));
			virt_sg_addr = sg_virt(data->sg);
			memcpy(virt_sg_addr, magicword, 8);
			cmdq_req_logs[index].check = 1;
		} else {
			index = queuing_req[req->tag];
			queuing_req[req->tag] = 0;
			local_irq_save(flags);
			cmdq_req_logs[index].end_time = cpu_clock(cpu);
			cmdq_req_logs[index].sector = req->__sector;
			cmdq_req_logs[index].virt_addr = sg_virt(data->sg);
			buffer = kmap_atomic(sg_page(data->sg)) + data->sg->offset;
			for (i = 0; i < CMDQ_REQ_BUF_SIZE; i++)
				cmdq_req_logs[index].datbuf[i] = buffer[i];
			cmdq_req_logs[index].check = 2;
			kunmap_atomic(buffer);
			local_irq_restore(flags);
		}
	}
}
#endif

static inline void mmc_blk_clear_packed(struct mmc_queue_req *mqrq)
{
	struct mmc_packed *packed = mqrq->packed;

	BUG_ON(!packed);

	mqrq->cmd_type = MMC_PACKED_NONE;
	packed->nr_entries = MMC_PACKED_NR_ZERO;
	packed->idx_failure = MMC_PACKED_NR_IDX;
	packed->retries = 0;
	packed->blocks = 0;
}

static struct mmc_blk_data *mmc_blk_get(struct gendisk *disk)
{
	struct mmc_blk_data *md;

	mutex_lock(&open_lock);
	md = disk->private_data;
	if (md && md->usage == 0)
		md = NULL;
	if (md)
		md->usage++;
	mutex_unlock(&open_lock);

	return md;
}

static inline int mmc_get_devidx(struct gendisk *disk)
{
	int devidx = disk->first_minor / perdev_minors;
	return devidx;
}

static void mmc_blk_put(struct mmc_blk_data *md)
{
	mutex_lock(&open_lock);
	md->usage--;
	if (md->usage == 0) {
		int devidx = mmc_get_devidx(md->disk);
		blk_cleanup_queue(md->queue.queue);

		__clear_bit(devidx, dev_use);

		put_disk(md->disk);
		kfree(md);
	}
	mutex_unlock(&open_lock);
}

static ssize_t power_ro_lock_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	struct mmc_card *card = md->queue.card;
	int locked = 0;

	if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PERM_WP_EN)
		locked = 2;
	else if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PWR_WP_EN)
		locked = 1;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", locked);

	mmc_blk_put(md);

	return ret;
}

static ssize_t power_ro_lock_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct mmc_blk_data *md, *part_md;
	struct mmc_card *card;
	unsigned long set;

	if (kstrtoul(buf, 0, &set))
		return -EINVAL;

	if (set != 1)
		return count;

	md = mmc_blk_get(dev_to_disk(dev));
	if (!md)
		return -EINVAL;
	card = md->queue.card;

	mmc_get_card(card);

	ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BOOT_WP,
				card->ext_csd.boot_ro_lock |
				EXT_CSD_BOOT_WP_B_PWR_WP_EN,
				card->ext_csd.part_time);
	if (ret)
		pr_err("%s: Locking boot partition ro until next power on failed: %d\n", md->disk->disk_name, ret);
	else
		card->ext_csd.boot_ro_lock |= EXT_CSD_BOOT_WP_B_PWR_WP_EN;

	mmc_put_card(card);

	if (!ret) {
		pr_info("%s: Locking boot partition ro until next power on\n",
			md->disk->disk_name);
		set_disk_ro(md->disk, 1);

		list_for_each_entry(part_md, &md->part, part)
			if (part_md->area_type == MMC_BLK_DATA_AREA_BOOT) {
				pr_info("%s: Locking boot partition ro until next power on\n", part_md->disk->disk_name);
				set_disk_ro(part_md->disk, 1);
			}
	}

	mmc_blk_put(md);
	return count;
}

static ssize_t force_ro_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int ret;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));

	if (!md)
		return -EINVAL;

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
		       get_disk_ro(dev_to_disk(dev)) ^
		       md->read_only);
	mmc_blk_put(md);
	return ret;
}

static ssize_t force_ro_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	char *end;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	unsigned long set = simple_strtoul(buf, &end, 0);

	if (!md)
		return -EINVAL;

	if (end == buf) {
		ret = -EINVAL;
		goto out;
	}

	set_disk_ro(dev_to_disk(dev), set || md->read_only);
	ret = count;
out:
	mmc_blk_put(md);
	return ret;
}

#ifdef CONFIG_MMC_SIMULATE_MAX_SPEED

static int max_read_speed, max_write_speed, cache_size = 4;

module_param(max_read_speed, int, S_IRUSR | S_IRGRP);
MODULE_PARM_DESC(max_read_speed, "maximum KB/s read speed 0=off");
module_param(max_write_speed, int, S_IRUSR | S_IRGRP);
MODULE_PARM_DESC(max_write_speed, "maximum KB/s write speed 0=off");
module_param(cache_size, int, S_IRUSR | S_IRGRP);
MODULE_PARM_DESC(cache_size, "MB high speed memory or SLC cache");

/*
 * helper macros and expectations:
 *  size    - unsigned long number of bytes
 *  jiffies - unsigned long HZ timestamp difference
 *  speed   - unsigned KB/s transfer rate
 */
#define size_and_speed_to_jiffies(size, speed) \
		((size) * HZ / (speed) / 1024UL)
#define jiffies_and_speed_to_size(jiffies, speed) \
		(((speed) * (jiffies) * 1024UL) / HZ)
#define jiffies_and_size_to_speed(jiffies, size) \
		((size) * HZ / (jiffies) / 1024UL)

/* Limits to report warning */
/* jiffies_and_size_to_speed(10*HZ, queue_max_hw_sectors(q) * 512UL) ~ 25 */
#define MIN_SPEED(q) 250 /* 10 times faster than a floppy disk */
#define MAX_SPEED(q) jiffies_and_size_to_speed(1, queue_max_sectors(q) * 512UL)

#define speed_valid(speed) ((speed) > 0)

static const char off[] = "off\n";

static int max_speed_show(int speed, char *buf)
{
	if (speed)
		return scnprintf(buf, PAGE_SIZE, "%uKB/s\n", speed);
	else
		return scnprintf(buf, PAGE_SIZE, off);
}

static int max_speed_store(const char *buf, struct request_queue *q)
{
	unsigned int limit, set = 0;

	if (!strncasecmp(off, buf, sizeof(off) - 2))
		return set;
	if (kstrtouint(buf, 0, &set) || (set > INT_MAX))
		return -EINVAL;
	if (set == 0)
		return set;
	limit = MAX_SPEED(q);
	if (set > limit)
		pr_warn("max speed %u ineffective above %u\n", set, limit);
	limit = MIN_SPEED(q);
	if (set < limit)
		pr_warn("max speed %u painful below %u\n", set, limit);
	return set;
}

static ssize_t max_write_speed_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	int ret = max_speed_show(atomic_read(&md->queue.max_write_speed), buf);

	mmc_blk_put(md);
	return ret;
}

static ssize_t max_write_speed_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	int set = max_speed_store(buf, md->queue.queue);

	if (set < 0) {
		mmc_blk_put(md);
		return set;
	}

	atomic_set(&md->queue.max_write_speed, set);
	mmc_blk_put(md);
	return count;
}

static const DEVICE_ATTR(max_write_speed, S_IRUGO | S_IWUSR,
	max_write_speed_show, max_write_speed_store);

static ssize_t max_read_speed_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	int ret = max_speed_show(atomic_read(&md->queue.max_read_speed), buf);

	mmc_blk_put(md);
	return ret;
}

static ssize_t max_read_speed_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	int set = max_speed_store(buf, md->queue.queue);

	if (set < 0) {
		mmc_blk_put(md);
		return set;
	}

	atomic_set(&md->queue.max_read_speed, set);
	mmc_blk_put(md);
	return count;
}

static const DEVICE_ATTR(max_read_speed, S_IRUGO | S_IWUSR,
	max_read_speed_show, max_read_speed_store);

static ssize_t cache_size_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	struct mmc_queue *mq = &md->queue;
	int cache_size = atomic_read(&mq->cache_size);
	int ret;

	if (!cache_size)
		ret = scnprintf(buf, PAGE_SIZE, off);
	else {
		int speed = atomic_read(&mq->max_write_speed);

		if (!speed_valid(speed))
			ret = scnprintf(buf, PAGE_SIZE, "%uMB\n", cache_size);
		else { /* We accept race between cache_jiffies and cache_used */
			unsigned long size = jiffies_and_speed_to_size(
				jiffies - mq->cache_jiffies, speed);
			long used = atomic_long_read(&mq->cache_used);

			if (size >= used)
				size = 0;
			else
				size = (used - size) * 100 / cache_size
					/ 1024UL / 1024UL;

			ret = scnprintf(buf, PAGE_SIZE, "%uMB %lu%% used\n",
				cache_size, size);
		}
	}

	mmc_blk_put(md);
	return ret;
}

static ssize_t cache_size_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct mmc_blk_data *md;
	unsigned int set = 0;

	if (strncasecmp(off, buf, sizeof(off) - 2)
	 && (kstrtouint(buf, 0, &set) || (set > INT_MAX)))
		return -EINVAL;

	md = mmc_blk_get(dev_to_disk(dev));
	atomic_set(&md->queue.cache_size, set);
	mmc_blk_put(md);
	return count;
}

static const DEVICE_ATTR(cache_size, S_IRUGO | S_IWUSR,
	cache_size_show, cache_size_store);

/* correct for write-back */
static long mmc_blk_cache_used(struct mmc_queue *mq, unsigned long waitfor)
{
	long used = 0;
	int speed = atomic_read(&mq->max_write_speed);

	if (speed_valid(speed)) {
		unsigned long size = jiffies_and_speed_to_size(
					waitfor - mq->cache_jiffies, speed);
		used = atomic_long_read(&mq->cache_used);

		if (size >= used)
			used = 0;
		else
			used -= size;
	}

	atomic_long_set(&mq->cache_used, used);
	mq->cache_jiffies = waitfor;

	return used;
}

static void mmc_blk_simulate_delay(
	struct mmc_queue *mq,
	struct request *req,
	unsigned long waitfor)
{
	int max_speed;

	if (!req)
		return;

	max_speed = (rq_data_dir(req) == READ)
		? atomic_read(&mq->max_read_speed)
		: atomic_read(&mq->max_write_speed);
	if (speed_valid(max_speed)) {
		unsigned long bytes = blk_rq_bytes(req);

		if (rq_data_dir(req) != READ) {
			int cache_size = atomic_read(&mq->cache_size);

			if (cache_size) {
				unsigned long size = cache_size * 1024L * 1024L;
				long used = mmc_blk_cache_used(mq, waitfor);

				used += bytes;
				atomic_long_set(&mq->cache_used, used);
				bytes = 0;
				if (used > size)
					bytes = used - size;
			}
		}
		waitfor += size_and_speed_to_jiffies(bytes, max_speed);
		if (time_is_after_jiffies(waitfor)) {
			long msecs = jiffies_to_msecs(waitfor - jiffies);

			if (likely(msecs > 0))
				msleep(msecs);
		}
	}
}

#else

#define mmc_blk_simulate_delay(mq, req, waitfor)

#endif

static int mmc_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct mmc_blk_data *md = mmc_blk_get(bdev->bd_disk);
	int ret = -ENXIO;

	mutex_lock(&block_mutex);
	if (md) {
		if (md->usage == 2)
			check_disk_change(bdev);
		ret = 0;

		if ((mode & FMODE_WRITE) && md->read_only) {
			mmc_blk_put(md);
			ret = -EROFS;
		}
	}
	mutex_unlock(&block_mutex);

	return ret;
}

static void mmc_blk_release(struct gendisk *disk, fmode_t mode)
{
	struct mmc_blk_data *md = disk->private_data;

	mutex_lock(&block_mutex);
	mmc_blk_put(md);
	mutex_unlock(&block_mutex);
}

static int
mmc_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	geo->cylinders = get_capacity(bdev->bd_disk) / (4 * 16);
	geo->heads = 4;
	geo->sectors = 16;
	return 0;
}

struct mmc_blk_ioc_data {
	struct mmc_ioc_cmd ic;
	unsigned char *buf;
	u64 buf_bytes;
};

static struct mmc_blk_ioc_data *mmc_blk_ioctl_copy_from_user(
	struct mmc_ioc_cmd __user *user)
{
	struct mmc_blk_ioc_data *idata;
	int err;

	idata = kmalloc(sizeof(*idata), GFP_KERNEL);
	if (!idata) {
		err = -ENOMEM;
		goto out;
	}

	if (copy_from_user(&idata->ic, user, sizeof(idata->ic))) {
		err = -EFAULT;
		goto idata_err;
	}

	idata->buf_bytes = (u64) idata->ic.blksz * idata->ic.blocks;
	if (idata->buf_bytes > MMC_IOC_MAX_BYTES) {
		err = -EOVERFLOW;
		goto idata_err;
	}

	if (!idata->buf_bytes)
		return idata;

	idata->buf = kmalloc(idata->buf_bytes, GFP_KERNEL);
	if (!idata->buf) {
		err = -ENOMEM;
		goto idata_err;
	}

	if (copy_from_user(idata->buf, (void __user *)(unsigned long)
					idata->ic.data_ptr, idata->buf_bytes)) {
		err = -EFAULT;
		goto copy_err;
	}

	return idata;

copy_err:
	kfree(idata->buf);
idata_err:
	kfree(idata);
out:
	return ERR_PTR(err);
}

static struct mmc_blk_ioc_data *mmc_blk_ioctl_copy_from_kernel(
	struct mmc_ioc_cmd *icmd)
{
	struct mmc_blk_ioc_data *idata;
	int err;

	idata = kzalloc(sizeof(*idata), GFP_NOIO);
	if (!idata) {
		err = -ENOMEM;
		goto out;
	}

	memcpy(&idata->ic, icmd, sizeof(idata->ic));

	idata->buf_bytes = (u64) idata->ic.blksz * idata->ic.blocks;
	if (idata->buf_bytes > MMC_IOC_MAX_BYTES) {
		err = -EOVERFLOW;
		goto idata_err;
	}

	if (!idata->buf_bytes)
		return idata;

	idata->buf = kzalloc(idata->buf_bytes, GFP_NOIO);
	if (!idata->buf) {
		err = -ENOMEM;
		goto idata_err;
	}

	memcpy(idata->buf, (void *)idata->ic.data_ptr, idata->buf_bytes);

	return idata;

idata_err:
	kfree(idata);
out:
	return ERR_PTR(err);
}

static int mmc_blk_ioctl_copy_to_user(struct mmc_ioc_cmd __user *ic_ptr,
				      struct mmc_blk_ioc_data *idata)
{
	struct mmc_ioc_cmd *ic = &idata->ic;

	if (copy_to_user(&(ic_ptr->response), ic->response,
			 sizeof(ic->response)))
		return -EFAULT;

	if (!idata->ic.write_flag) {
		if (copy_to_user((void __user *)(unsigned long)ic->data_ptr,
				 idata->buf, idata->buf_bytes))
			return -EFAULT;
	}

	return 0;
}

static int ioctl_rpmb_card_status_poll(struct mmc_card *card, u32 *status,
				       u32 retries_max)
{
	int err;
	u32 retry_count = 0;

	if (!status || !retries_max)
		return -EINVAL;

	do {
		err = get_card_status(card, status, 5);
		if (err)
			break;

		if (!R1_STATUS(*status) &&
				(R1_CURRENT_STATE(*status) != R1_STATE_PRG))
			break; /* RPMB programming operation complete */

		/*
		 * Rechedule to give the MMC device a chance to continue
		 * processing the previous command without being polled too
		 * frequently.
		 */
		usleep_range(1000, 5000);
	} while (++retry_count < retries_max);

	if (retry_count == retries_max)
		err = -EPERM;

	return err;
}

static int ioctl_do_sanitize(struct mmc_card *card)
{
	int err;

	if (!mmc_can_sanitize(card)) {
			pr_warn("%s: %s - SANITIZE is not supported\n",
				mmc_hostname(card->host), __func__);
			err = -EOPNOTSUPP;
			goto out;
	}

	pr_debug("%s: %s - SANITIZE IN PROGRESS...\n",
		mmc_hostname(card->host), __func__);

	trace_mmc_blk_erase_start(EXT_CSD_SANITIZE_START, 0, 0);
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_SANITIZE_START, 1,
					MMC_SANITIZE_REQ_TIMEOUT);
	trace_mmc_blk_erase_end(EXT_CSD_SANITIZE_START, 0, 0);

	if (err)
		pr_err("%s: %s - EXT_CSD_SANITIZE_START failed. err=%d\n",
		       mmc_hostname(card->host), __func__, err);

	pr_debug("%s: %s - SANITIZE COMPLETED\n", mmc_hostname(card->host),
					     __func__);
out:
	return err;
}

static int __mmc_blk_ioctl_cmd(struct mmc_card *card, struct mmc_blk_data *md,
			       struct mmc_blk_ioc_data *idata)
{
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct mmc_request mrq = {NULL};
	struct scatterlist sg;
	int err;
	int is_rpmb = false;
	u32 status = 0;
#ifdef MMC_BLK_CQ_FFU
	static bool cqffu_mode = false;
#endif

	if (!card || !md || !idata)
		return -EINVAL;

	if (md->area_type & MMC_BLK_DATA_AREA_RPMB)
		is_rpmb = true;

	cmd.opcode = idata->ic.opcode;
	cmd.arg = idata->ic.arg;
	cmd.flags = idata->ic.flags;

	if (idata->buf_bytes) {
		data.sg = &sg;
		data.sg_len = 1;
		data.blksz = idata->ic.blksz;
		data.blocks = idata->ic.blocks;

		sg_init_one(data.sg, idata->buf, idata->buf_bytes);

		if (idata->ic.write_flag)
			data.flags = MMC_DATA_WRITE;
		else
			data.flags = MMC_DATA_READ;

		/* data.flags must already be set before doing this. */
		mmc_set_data_timeout(&data, card);

		/* Allow overriding the timeout_ns for empirical tuning. */
		if (idata->ic.data_timeout_ns)
			data.timeout_ns = idata->ic.data_timeout_ns;

		if ((cmd.flags & MMC_RSP_R1B) == MMC_RSP_R1B) {
			/*
			 * Pretend this is a data transfer and rely on the
			 * host driver to compute timeout.  When all host
			 * drivers support cmd.cmd_timeout for R1B, this
			 * can be changed to:
			 *
			 *     mrq.data = NULL;
			 *     cmd.cmd_timeout = idata->ic.cmd_timeout_ms;
			 */
			data.timeout_ns = idata->ic.cmd_timeout_ms * 1000000;
		}

		mrq.data = &data;
	}

	mrq.cmd = &cmd;
#ifdef MMC_BLK_CQ_FFU
	/* need to switch cq disable for ffu mode */
	if (mmc_card_cmdq(card) && cmd.arg == MMC_CMDQ_FFU_ENTER) {
		/* disable CQ mode in card */
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_CMDQ, 0,
				 card->ext_csd.generic_cmd6_time);
		if (err) {
			pr_err("%s: failed to switch card to legacy mode: %d\n",
				   __func__, err);
			return err;
		}
		mmc_card_clr_cmdq(card);
		card->cmdq_init = false;
		cqffu_mode = true;

		pr_info("%s: succeeded to switch card to legacy mode\n",
				__func__);
	}
#endif
	err = mmc_blk_part_switch(card, md);
	if (err)
		return err;

	if (idata->ic.is_acmd) {
		err = mmc_app_cmd(card->host, card);
		if (err)
			return err;
	}

	if (is_rpmb) {
		err = mmc_set_blockcount(card, data.blocks,
			idata->ic.write_flag & (1 << 31));
		if (err)
			return err;
	}

	if ((MMC_EXTRACT_INDEX_FROM_ARG(cmd.arg) == EXT_CSD_SANITIZE_START) &&
	    (cmd.opcode == MMC_SWITCH)) {
		err = ioctl_do_sanitize(card);

		if (err)
			pr_err("%s: ioctl_do_sanitize() failed. err = %d",
			       __func__, err);

		return err;
	}

	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error) {
		dev_err(mmc_dev(card->host), "%s: cmd error %d\n",
						__func__, cmd.error);
		return cmd.error;
	}
	if (data.error) {
		dev_err(mmc_dev(card->host), "%s: data error %d\n",
						__func__, data.error);
		return data.error;
	}

	/*
	 * According to the SD specs, some commands require a delay after
	 * issuing the command.
	 */
	if (idata->ic.postsleep_min_us)
		usleep_range(idata->ic.postsleep_min_us, idata->ic.postsleep_max_us);

	memcpy(&(idata->ic.response), cmd.resp, sizeof(cmd.resp));

	if (is_rpmb) {
		/*
		 * Ensure RPMB command has completed by polling CMD13
		 * "Send Status".
		 */
		err = ioctl_rpmb_card_status_poll(card, &status, 5);
		if (err)
			dev_err(mmc_dev(card->host),
					"%s: Card Status=0x%08X, error %d\n",
					__func__, status, err);
	}
#ifdef MMC_BLK_CQ_FFU
	if (!mmc_card_cmdq(card) && cqffu_mode &&
			cmd.arg == MMC_CMDQ_FFU_EXIT) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
		 			EXT_CSD_CMDQ, 1,
					card->ext_csd.generic_cmd6_time);
		card->cmdq_init = true;
		cqffu_mode = false;
		if(err) {
			pr_err("%s: cmdq mode %sable failed %d\n", md->disk->disk_name, 1 ? "en" : "dis", err);
			return err;
		}
		mmc_card_set_cmdq(card);
		card->host->cmdq_ops->enable(card->host);
		pr_info("%s: succeeded to switch card to cmdq mode\n",
				__func__);
	}
#endif
	return err;
}

static int mmc_blk_ioctl_cmd(struct block_device *bdev,
			     struct mmc_ioc_cmd __user *ic_ptr)
{
	struct mmc_blk_ioc_data *idata;
	struct mmc_blk_data *md;
	struct mmc_card *card;
	struct mmc_host *host;
	int err = 0, ioc_err = 0;

	/*
	 * The caller must have CAP_SYS_RAWIO, and must be calling this on the
	 * whole block device, not on a partition.  This prevents overspray
	 * between sibling partitions.
	 */
	if ((!capable(CAP_SYS_RAWIO)) || (bdev != bdev->bd_contains))
		return -EPERM;

	idata = mmc_blk_ioctl_copy_from_user(ic_ptr);
	if (IS_ERR(idata))
		return PTR_ERR(idata);

	md = mmc_blk_get(bdev->bd_disk);
	if (!md) {
		err = -EINVAL;
		goto cmd_err;
	}

	card = md->queue.card;
	if (IS_ERR(card)) {
		err = PTR_ERR(card);
		goto cmd_done;
	}
	host = card->host;

	mmc_get_card(card);

	if (mmc_card_cmdq(card)) {
		pr_err("%s: CMDQ : try halt for legacy ioctl cmd\n", mmc_hostname(card->host));
		err = mmc_cmdq_halt_on_empty_queue(card->host);
		if (err) {
			pr_err("%s: halt failed while doing %s err (%d)\n",
					mmc_hostname(card->host), __func__,
					err);
			mmc_put_card(card);
			goto out_free_halt;
		}
		host->cmdq_ops->disable(host, true);
	}
	ioc_err = __mmc_blk_ioctl_cmd(card, md, idata);

	if (mmc_card_cmdq(card)) {
		pr_err("%s: CMDQ : try unhalt after legacy ioctl cmd\n", mmc_hostname(card->host));
		if (mmc_cmdq_halt(card->host, false))
			pr_err("%s: %s: cmdq unhalt failed\n",
					mmc_hostname(card->host), __func__);
		host->cmdq_ops->enable(host);
	}
	mmc_put_card(card);

	err = mmc_blk_ioctl_copy_to_user(ic_ptr, idata);

cmd_done:
	mmc_blk_put(md);
out_free_halt:
	if (mmc_card_cmdq(card)) {
		if (mmc_cmdq_halt(card->host, false))
			pr_err("%s: %s: cmdq unhalt failed\n",
					mmc_hostname(card->host), __func__);
	}
cmd_err:
	kfree(idata->buf);
	kfree(idata);
	return io