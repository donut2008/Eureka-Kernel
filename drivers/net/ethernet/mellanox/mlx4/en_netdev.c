ckdep et al. */
		up(&fs_info->uuid_tree_rescan_sem);

		sb->s_flags |= MS_RDONLY;

		/*
		 * Setting MS_RDONLY will put the cleaner thread to
		 * sleep at the next loop if it's already active.
		 * If it's already asleep, we'll leave unused block
		 * groups on disk until we're mounted read-write again
		 * unless we clean them up here.
		 */
		btrfs_delete_unused_bgs(fs_info);

		btrfs_dev_replace_suspend_for_unmount(fs_info);
		btrfs_scrub_cancel(fs_info);
		btrfs_pause_balance(fs_info);

		ret = btrfs_commit_super(root);
		if (ret)
			goto restore;
	} else {
		if (test_bit(BTRFS_FS_STATE_ERROR, &root->fs_info->fs_state)) {
			btrfs_err(fs_info,
				"Remounting read-write after error is not allowed");
			ret = -EINVAL;
			goto restore;
		}
		if (fs_info->fs_devices->rw_devices == 0) {
			ret = -EACCES;
			goto restore;
		}

		if (fs_info->fs_devices->missing_devices >
		     fs_info->num_tolerated_disk_barrier_failures &&
		    !(*flags & MS_RDONLY)) {
			btrfs_warn(fs_info,
				"too many missing devices, writeable remount is not allowed");
			ret = -EACCES;
			goto restore;
		}

		if (btrfs_super_log_root(fs_info->super_copy) != 0) {
			btrfs_warn(fs_info,
		"mount required to replay tree-log, cannot remount read-write");
			ret = -EINVAL;
			goto restore;
		}

		ret = btrfs_cleanup_fs_roots(fs_info);
		if (ret)
			goto restore;

		/* recover relocation */
		mutex_lock(&fs_info->cleaner_mutex);
		ret = btrfs_recover_relocation(root);
		mutex_unlock(&fs_info->cleaner_mutex);
		if (ret)
			goto restore;

		ret = btrfs_resume_balance_async(fs_info);
		if (ret)
			goto restore;

		ret = btrfs_resume_dev_replace_async(fs_info);
		if (ret) {
			btrfs_warn(fs_info, "failed to resume dev_replace");
			goto restore;
		}

		btrfs_qgroup_rescan_resume(fs_info);

		if (!fs_info->uuid_root) {
			btrfs_info(fs_info, "creating UUID tree");
			ret = btrfs_create_uuid_tree(fs_info);
			if (ret) {
				btrfs_warn(fs_info, "failed to create the UUID tree %d", ret);
				goto restore;
			}
		}
		sb->s_flags &= ~MS_RDONLY;
	}
out:
	wake_up_process(fs_info->transaction_kthread);
	btrfs_remount_cleanup(fs_info, old_opts);
	return 0;

restore:
	/* We've hit an error - don't reset MS_RDONLY */
	if (sb->s_flags & MS_RDONLY)
		old_flags |= MS_RDONLY;
	sb->s_flags = old_flags;
	fs_info->mount_opt = old_opts;
	fs_info->compress_type = old_compress_type;
	fs_info->max_inline = old_max_inline;
	mutex_lock(&fs_info->chunk_mutex);
	fs_info->alloc_start = old_alloc_start;
	mutex_unlock(&fs_info->chunk_mutex);
	btrfs_resize_thread_pool(fs_info,
		old_thread_pool_size, fs_info->thread_pool_size);
	fs_info->metadata_ratio = old_metadata_ratio;
	btrfs_remount_cleanup(fs_info, old_opts);
	return ret;
}

/* Used to sort the devices by max_avail(descending sort) */
static int btrfs_cmp_device_free_bytes(const void *dev_info1,
				       const void *dev_info2)
{
	if (((struct btrfs_device_info *)dev_info1)->max_avail >
	    ((struct btrfs_device_info *)dev_info2)->max_avail)
		return -1;
	else if (((struct btrfs_device_info *)dev_info1)->max_avail <
		 ((struct btrfs_device_info *)dev_info2)->max_avail)
		return 1;
	else
	return 0;
}

/*
 * sort the devices by max_avail, in which max free extent size of each device
 * is stored.(Descending Sort)
 */
static inline void btrfs_descending_sort_devices(
					struct btrfs_device_info *devices,
					size_t nr_devices)
{
	sort(devices, nr_devices, sizeof(struct btrfs_device_info),
	     btrfs_cmp_device_free_bytes, NULL);
}

/*
 * The helper to calc the free space on the devices that can be used to store
 * file data.
 */
static int btrfs_calc_avail_data_space(struct btrfs_root *root, u64 *free_bytes)
{
	struct btrfs_fs_info *fs_info = root->fs_info;
	struct btrfs_device_info *devices_info;
	struct btrfs_fs_devices *fs_devices = fs_info->fs_devices;
	struct btrfs_device *device;
	u64 skip_space;
	u64 type;
	u64 avail_space;
	u64 used_space;
	u64 min_stripe_size;
	int min_stripes = 1, num_stripes = 1;
	int i = 0, nr_devices;
	int ret;

	/*
	 * We aren't under the device list lock, so this is racey-ish, but good
	 * enough for our purposes.
	 */
	nr_devices = fs_info->fs_devices->open_devices;
	if (!nr_devices) {
		smp_mb();
		nr_devices = fs_info->fs_devices->open_devices;
		ASSERT(nr_devices);
		if (!nr_devices) {
			*free_bytes = 0;
			return 0;
		}
	}

	devices_info = kmalloc_array(nr_devices, sizeof(*devices_info),
			       GFP_NOFS);
	if (!devices_info)
		return -ENOMEM;

	/* calc min stripe number for data space alloction */
	type = btrfs_get_alloc_profile(root, 1);
	if (type & BTRFS_BLOCK_GROUP_RAID0) {
		min_stripes = 2;
		num_stripes = nr_devices;
	} else if (type & BTRFS_BLOCK_GROUP_RAID1) {
		min_stripes = 2;
		num_stripes = 2;
	} else if (type & BTRFS_BLOCK_GROUP_RAID10) {
		min_stripes = 4;
		num_stripes = 4;
	}

	if (type & BTRFS_BLOCK_GROUP_DUP)
		min_stripe_size = 2 * BTRFS_STRIPE_LEN;
	else
		min_stripe_size = BTRFS_STRIPE_LEN;

	if (fs_info->alloc_start)
		mutex_lock(&fs_devices->device_list_mutex);
	rcu_read_lock();
	list_for_each_entry_rcu(device, &fs_devices->devices, dev_list) {
		if (!device->in_fs_metadata || !device->bdev ||
		    device->is_tgtdev_for_dev_replace)
			continue;

		if (i >= nr_devices)
			break;

		avail_space = device->total_bytes - device->bytes_used;

		/* align with stripe_len */
		avail_space = div_u64(avail_space, BTRFS_STRIPE_LEN);
		avail_space *= BTRFS_STRIPE_LEN;

		/*
		 * In order to avoid overwritting the superblock on the drive,
		 * btrfs starts at an offset of at least 1MB when doing chunk
		 * allocation.
		 */
		skip_space = 1024 * 1024;

		/* user can set the offset in fs_info->alloc_start. */
		if (fs_info->alloc_start &&
		    fs_info->alloc_start + BTRFS_STRIPE_LEN <=
		    device->total_bytes) {
			rcu_read_unlock();
			skip_space = max(fs_info->alloc_start, skip_space);

			/*
			 * btrfs can not use the free space in
			 * [0, skip_space - 1], we must subtract it from the
			 * total. In order to implement it, we account the used
			 * space in this range first.
			 */
			ret = btrfs_account_dev_extents_size(device, 0,
							     skip_space - 1,
							     &used_space);
			if (ret) {
				kfree(devices_info);
				mutex_unlock(&fs_devices->device_list_mutex);
				return ret;
			}

			rcu_read_lock();

			/* calc the free space in [0, skip_space - 1] */
			skip_space -= used_space;
		}

		/*
		 * we can use the free space in [0, skip_space - 1], subtract
		 * it from the total.
		 */
		if (avail_space && avail_space >= skip_space)
			avail_space -= skip_space;
		else
			avail_space = 0;

		if (avail_space < min_stripe_size)
			continue;

		devices_info[i].dev = device;
		devices_info[i].max_avail = avail_space;

		i++;
	}
	rcu_read_unlock();
	if (fs_info->alloc_start)
		mutex_unlock(&fs_devices->device_list_mutex);

	nr_devices = i;

	btrfs_descending_sort_devices(devices_info, nr_devices);

	i = nr_devices - 1;
	avail_space = 0;
	while (nr_devices >= min_stripes) {
		if (num_stripes > nr_devices)
			num_stripes = nr_devices;

		if (devices_info[i].max_avail >= min_stripe_size) {
			int j;
			u64 alloc_size;

			avail_space += devices_info[i].max_avail * num_stripes;
			alloc_size = devices_info[i].max_avail;
			for (j = i + 1 - num_stripes; j <= i; j++)
				devices_info[j].max_avail -= alloc_size;
		}
		i--;
		nr_devices--;
	}

	kfree(devices_info);
	*free_bytes = avail_space;
	return 0;
}

/*
 * Calculate numbers for 'df', pessimistic in case of mixed raid profiles.
 *
 * If there's a redundant raid level at DATA block groups, use the respective
 * multiplier to scale the sizes.
 *
 * Unused device space usage is based on simulating the chunk allocator
 * algorithm that respects the device sizes, order of allocations and the
 * 'alloc_start' value, this is a close approximation of the actual use but
 * there are other factors that may change the result (like a new metadata
 * chunk).
 *
 * If metadata is exhausted, f_bavail will be 0.
 *
 * FIXME: not accurate for mixed block groups, total and free/used are ok,
 * available appears slightly larger.
 */
static int btrfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	struct btrfs_fs_info *fs_info = btrfs_sb(dentry->d_sb);
	struct btrfs_super_block *disk_super = fs_info->super_copy;
	struct list_head *head = &fs_info->space_info;
	struct btrfs_space_info *found;
	u64 total_used = 0;
	u64 total_free_data = 0;
	u64 total_free_meta = 0;
	int bits = dentry->d_sb->s_blocksize_bits;
	__be32 *fsid = (__be32 *)fs_info->fsid;
	unsigned factor = 1;
	struct btrfs_block_rsv *block_rsv = &fs_info->global_block_rsv;
	int ret;
	u64 thresh = 0;
	int mixed = 0;

	/*
	 * holding chunk_muext to avoid allocating new chunks, holding
	 * device_list_mutex to avoid the device being removed
	 */
	rcu_read_lock();
	list_for_each_entry_rcu(found, head, list) {
		if (found->flags & BTRFS_BLOCK_GROUP_DATA) {
			int i;

			total_free_data += found->disk_total - found->disk_used;
			total_free_data -=
				btrfs_account_ro_block_groups_free_space(found);

			for (i = 0; i < BTRFS_NR_RAID_TYPES; i++) {
				if (!list_empty(&found->block_groups[i])) {
					switch (i) {
					case BTRFS_RAID_DUP:
					case BTRFS_RAID_RAID1:
					case BTRFS_RAID_RAID10:
						factor = 2;
					}
				}
			}
		}

		/*
		 * Metadata in mixed block goup profiles are accounted in data
		 */
		if (!mixed && found->flags & BTRFS_BLOCK_GROUP_METADATA) {
			if (found->flags & BTRFS_BLOCK_GROUP_DATA)
				mixed = 1;
			else
				total_free_meta += found->disk_total -
					found->disk_used;
		}

		total_used += found->disk_used;
	}

	rcu_read_unlock();

	buf->f_blocks = div_u64(btrfs_super_total_bytes(disk_super), factor);
	buf->f_blocks >>= bits;
	buf->f_bfree = buf->f_blocks - (div_u64(total_used, factor) >> bits);

	/* Account global block reserve as used, it's in logical size already */
	spin_lock(&block_rsv->lock);
	buf->f_bfree -= block_rsv->size >> bits;
	spin_unlock(&block_rsv->lock);

	buf->f_bavail = div_u64(total_free_data, factor);
	ret = btrfs_calc_avail_data_space(fs_info->tree_root, &total_free_data);
	if (ret)
		return ret;
	buf->f_bavail += div_u64(total_free_data, factor);
	buf->f_bavail = buf->f_bavail >> bits;

	/*
	 * We calculate the remaining metadata space minus global reserve. If
	 * this is (supposedly) smaller than zero, there's no space. But this
	 * does not hold in practice, the exhausted state happens where's still
	 * some positive delta. So we apply some guesswork and compare the
	 * delta to a 4M threshold.  (Practically observed delta was ~2M.)
	 *
	 * We probably cannot calculate the exact threshold value because this
	 * depends on the internal reservations requested by various
	 * operations, so some operations that consume a few metadata will
	 * succeed even if the Avail is zero. But this is better than the other
	 * way around.
	 */
	thresh = 4 * 1024 * 1024;

	/*
	 * We only want to claim there's no available space if we can no longer
	 * allocate chunks for our metadata profile and our global reserve will
	 * not fit in the free metadata space.  If we aren't ->full then we
	 * still can allocate chunks and thus are fine using the currently
	 * calculated f_bavail.
	 */
	if (!mixed && block_rsv->space_info->full &&
	    (total_free_meta < thresh || total_free_meta - thresh < block_rsv->size))
		buf->f_bavail = 0;

	buf->f_type = BTRFS_SUPER_MAGIC;
	buf->f_bsize = dentry->d_sb->s_blocksize;
	buf->f_namelen = BTRFS_NAME_LEN;

	/* We treat it as constant endianness (it doesn't matter _which_)
	   because we want the fsid to come out the same whether mounted
	   on a big-endian or little-endian host */
	buf->f_fsid.val[0] = be32_to_cpu(fsid[0]) ^ be32_to_cpu(fsid[2]);
	buf->f_fsid.val[1] = be32_to_cpu(fsid[1]) ^ be32_to_cpu(fsid[3]);
	/* Mask in the root object ID too, to disambiguate subvols */
	buf->f_fsid.val[0] ^= BTRFS_I(d_inode(dentry))->root->objectid >> 32;
	buf->f_fsid.val[1] ^= BTRFS_I(d_inode(dentry))->root->objectid;

	return 0;
}

static void btrfs_kill_super(struct super_block *sb)
{
	struct btrfs_fs_info *fs_info = btrfs_sb(sb);
	kill_anon_super(sb);
	free_fs_info(fs_info);
}

static struct file_system_type btrfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "btrfs",
	.mount		= btrfs_mount,
	.kill_sb	= btrfs_kill_super,
	.fs_flags	= FS_REQUIRES_DEV | FS_BINARY_MOUNTDATA,
};
MODULE_ALIAS_FS("btrfs");

static int btrfs_control_open(struct inode *inode, struct file *file)
{
	/*
	 * The control file's private_data is used to hold the
	 * transaction when it is started and is used to keep
	 * track of whether a transaction is already in progress.
	 */
	file->private_data = NULL;
	return 0;
}

/*
 * used by btrfsctl to scan devices when no FS is mounted
 */
static long btrfs_control_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct btrfs_ioctl_vol_args *vol;
	struct btrfs_fs_devices *fs_devices;
	int ret = -ENOTTY;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	vol = memdup_user((void __user *)arg, sizeof(*vol));
	if (IS_ERR(vol))
		return PTR_ERR(vol);
	vol->name[BTRFS_PATH_NAME_MAX] = '\0';

	switch (cmd) {
	case BTRFS_IOC_SCAN_DEV:
		ret = btrfs_scan_one_device(vol->name, FMODE_READ,
					    &btrfs_fs_type, &fs_devices);
		break;
	case BTRFS_IOC_DEVICES_READY:
		ret = btrfs_scan_one_device(vol->name, FMODE_READ,
					    &btrfs_fs_type, &fs_devices);
		if (ret)
			break;
		ret = !(fs_devices->num_devices == fs_devices->total_devices);
		break;
	}

	kfree(vol);
	return ret;
}

static int btrfs_freeze(struct super_block *sb)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_root *root = btrfs_sb(sb)->tree_root;

	trans = btrfs_attach_transaction_barrier(root);
	if (IS_ERR(trans)) {
		/* no transaction, don't bother */
		if (PTR_ERR(trans) == -ENOENT)
			return 0;
		return PTR_ERR(trans);
	}
	return btrfs_commit_transaction(trans, root);
}

static int btrfs_show_devname(struct seq_file *m, struct dentry *root)
{
	struct btrfs_fs_info *fs_info = btrfs_sb(root->d_sb);
	struct btrfs_fs_devices *cur_devices;
	struct btrfs_device *dev, *first_dev = NULL;
	struct list_head *head;
	struct rcu_string *name;

	mutex_lock(&fs_info->fs_devices->device_list_mutex);
	cur_devices = fs_info->fs_devices;
	while (cur_devices) {
		head = &cur_devices->devices;
		list_for_each_entry(dev, head, dev_list) {
			if (dev->missing)
				continue;
			if (!dev->name)
				continue;
			if (!first_dev || dev->devid < first_dev->devid)
				first_dev = dev;
		}
		cur_devices = cur_devices->seed;
	}

	if (first_dev) {
		rcu_read_lock();
		name = rcu_dereference(first_dev->name);
		seq_escape(m, name->str, " \t\n\\");
		rcu_read_unlock();
	} else {
		WARN_ON(1);
	}
	mutex_unlock(&fs_info->fs_devices->device_list_mutex);
	return 0;
}

static const struct super_operations btrfs_super_ops = {
	.drop_inode	= btrfs_drop_inode,
	.evict_inode	= btrfs_evict_inode,
	.put_super	= btrfs_put_super,
	.sync_fs	= btrfs_sync_fs,
	.show_options	= btrfs_show_options,
	.show_devname	= btrfs_show_devname,
	.write_inode	= btrfs_write_inode,
	.alloc_inode	= btrfs_alloc_inode,
	.destroy_inode	= btrfs_destroy_inode,
	.statfs		= btrfs_statfs,
	.remount_fs	= btrfs_remount,
	.freeze_fs	= btrfs_freeze,
};

static const struct file_operations btrfs_ctl_fops = {
	.open = btrfs_control_open,
	.unlocked_ioctl	 = btrfs_control_ioctl,
	.compat_ioctl = btrfs_control_ioctl,
	.owner	 = THIS_MODULE,
	.llseek = noop_llseek,
};

static struct miscdevice btrfs_misc = {
	.minor		= BTRFS_MINOR,
	.name		= "btrfs-control",
	.fops		= &btrfs_ctl_fops
};

MODULE_ALIAS_MISCDEV(BTRFS_MINOR);
MODULE_ALIAS("devname:btrfs-control");

static int btrfs_interface_init(void)
{
	return misc_register(&btrfs_misc);
}

static void btrfs_interface_exit(void)
{
	misc_deregister(&btrfs_misc);
}

static void btrfs_print_info(void)
{
	printk(KERN_INFO "Btrfs loaded"
#ifdef CONFIG_BTRFS_DEBUG
			", debug=on"
#endif
#ifdef CONFIG_BTRFS_ASSERT
			", assert=on"
#endif
#ifdef CONFIG_BTRFS_FS_CHECK_INTEGRITY
			", integrity-checker=on"
#endif
			"\n");
}

static int btrfs_run_sanity_tests(void)
{
	int ret;

	ret = btrfs_init_test_fs();
	if (ret)
		return ret;

	ret = btrfs_test_free_space_cache();
	if (ret)
		goto out;
	ret = btrfs_test_extent_buffer_operations();
	if (ret)
		goto out;
	ret = btrfs_test_extent_io();
	if (ret)
		goto out;
	ret = btrfs_test_inodes();
	if (ret)
		goto out;
	ret = btrfs_test_qgroups();
out:
	btrfs_destroy_test_fs();
	return ret;
}

static int __init init_btrfs_fs(void)
{
	int err;

	err = btrfs_hash_init();
	if (err)
		return err;

	btrfs_props_init();

	err = btrfs_init_sysfs();
	if (err)
		goto free_hash;

	btrfs_init_compress();

	err = btrfs_init_cachep();
	if (err)
		goto free_compress;

	err = extent_io_init();
	if (err)
		goto free_cachep;

	err = extent_map_init();
	if (err)
		goto free_extent_io;

	err = ordered_data_init();
	if (err)
		goto free_extent_map;

	err = btrfs_delayed_inode_init();
	if (err)
		goto free_ordered_data;

	err = btrfs_auto_defrag_init();
	if (err)
		goto free_delayed_inode;

	err = btrfs_delayed_ref_init();
	if (err)
		goto free_auto_defrag;

	err = btrfs_prelim_ref_init();
	if (err)
		goto free_delayed_ref;

	err = btrfs_end_io_wq_init();
	if (err)
		goto free_prelim_ref;

	err = btrfs_interface_init();
	if (err)
		goto free_end_io_wq;

	btrfs_init_lockdep();

	btrfs_print_info();

	err = btrfs_run_sanity_tests();
	if (err)
		goto unregister_ioctl;

	err = register_filesystem(&btrfs_fs_type);
	if (err)
		goto unregister_ioctl;

	return 0;

unregister_ioctl:
	btrfs_interface_exit();
free_end_io_wq:
	btrfs_end_io_wq_exit();
free_prelim_ref:
	btrfs_prelim_ref_exit();
free_delayed_ref:
	btrfs_delayed_ref_exit();
free_auto_defrag:
	btrfs_auto_defrag_exit();
free_delayed_inode:
	btrfs_delayed_inode_exit();
free_ordered_data:
	ordered_data_exit();
free_extent_map:
	extent_map_exit();
free_extent_io:
	extent_io_exit();
free_cachep:
	btrfs_destroy_cachep();
free_compress:
	btrfs_exit_compress();
	btrfs_exit_sysfs();
free_hash:
	btrfs_hash_exit();
	return err;
}

static void __exit exit_btrfs_fs(void)
{
	btrfs_destroy_cachep();
	btrfs_delayed_ref_exit();
	btrfs_auto_defrag_exit();
	btrfs_delayed_inode_exit();
	btrfs_prelim_ref_exit();
	ordered_data_exit();
	extent_map_exit();
	extent_io_exit();
	btrfs_interface_exit();
	btrfs_end_io_wq_exit();
	unregister_filesystem(&btrfs_fs_type);
	btrfs_exit_sysfs();
	btrfs_cleanup_fs_uuids();
	btrfs_exit_compress();
	btrfs_hash_exit();
}

late_initcall(init_btrfs_fs);
module_exit(exit_btrfs_fs)

MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/buffer_head.h>
#include <linux/kobject.h>
#include <linux/bug.h>
#include <linux/genhd.h>
#include <linux/debugfs.h>

#include "ctree.h"
#include "disk-io.h"
#include "transaction.h"
#include "sysfs.h"
#include "volumes.h"

static inline struct btrfs_fs_info *to_fs_info(struct kobject *kobj);
static inline struct btrfs_fs_devices *to_fs_devs(struct kobject *kobj);

static u64 get_features(struct btrfs_fs_info *fs_info,
			enum btrfs_feature_set set)
{
	struct btrfs_super_block *disk_super = fs_info->super_copy;
	if (set == FEAT_COMPAT)
		return btrfs_super_compat_flags(disk_super);
	else if (set == FEAT_COMPAT_RO)
		return btrfs_super_compat_ro_flags(disk_super);
	else
		return btrfs_super_incompat_flags(disk_super);
}

static void set_features(struct btrfs_fs_info *fs_info,
			 enum btrfs_feature_set set, u64 features)
{
	struct btrfs_super_block *disk_super = fs_info->super_copy;
	if (set == FEAT_COMPAT)
		btrfs_set_super_compat_flags(disk_super, features);
	else if (set == FEAT_COMPAT_RO)
		btrfs_set_super_compat_ro_flags(disk_super, features);
	else
		btrfs_set_super_incompat_flags(disk_super, features);
}

static int can_modify_feature(struct btrfs_feature_attr *fa)
{
	int val = 0;
	u64 set, clear;
	switch (fa->feature_set) {
	case FEAT_COMPAT:
		set = BTRFS_FEATURE_COMPAT_SAFE_SET;
		clear = BTRFS_FEATURE_COMPAT_SAFE_CLEAR;
		break;
	case FEAT_COMPAT_RO:
		set = BTRFS_FEATURE_COMPAT_RO_SAFE_SET;
		clear = BTRFS_FEATURE_COMPAT_RO_SAFE_CLEAR;
		break;
	case FEAT_INCOMPAT:
		set = BTRFS_FEATURE_INCOMPAT_SAFE_SET;
		clear = BTRFS_FEATURE_INCOMPAT_SAFE_CLEAR;
		break;
	default:
		printk(KERN_WARNING "btrfs: sysfs: unknown feature set %d\n",
				fa->feature_set);
		return 0;
	}

	if (set & fa->feature_bit)
		val |= 1;
	if (clear & fa->feature_bit)
		val |= 2;

	return val;
}

static ssize_t btrfs_feature_attr_show(struct kobject *kobj,
				       struct kobj_attribute *a, char *buf)
{
	int val = 0;
	struct btrfs_fs_info *fs_info = to_fs_info(kobj);
	struct btrfs_feature_attr *fa = to_btrfs_feature_attr(a);
	if (fs_info) {
		u64 features = get_features(fs_info, fa->feature_set);
		if (features & fa->feature_bit)
			val = 1;
	} else
		val = can_modify_feature(fa);

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t btrfs_feature_attr_store(struct kobject *kobj,
					struct kobj_attribute *a,
					const char *buf, size_t count)
{
	struct btrfs_fs_info *fs_info;
	struct btrfs_feature_attr *fa = to_btrfs_feature_attr(a);
	u64 features, set, clear;
	unsigned long val;
	int ret;

	fs_info = to_fs_info(kobj);
	if (!fs_info)
		return -EPERM;

	ret = kstrtoul(skip_spaces(buf), 0, &val);
	if (ret)
		return ret;

	if (fa->feature_set == FEAT_COMPAT) {
		set = BTRFS_FEATURE_COMPAT_SAFE_SET;
		clear = BTRFS_FEATURE_COMPAT_SAFE_CLEAR;
	} else if (fa->feature_set == FEAT_COMPAT_RO) {
		set = BTRFS_FEATURE_COMPAT_RO_SAFE_SET;
		clear = BTRFS_FEATURE_COMPAT_RO_SAFE_CLEAR;
	} else {
		set = BTRFS_FEATURE_INCOMPAT_SAFE_SET;
		clear = BTRFS_FEATURE_INCOMPAT_SAFE_CLEAR;
	}

	features = get_features(fs_info, fa->feature_set);

	/* Nothing to do */
	if ((val && (features & fa->feature_bit)) ||
	    (!val && !(features & fa->feature_bit)))
		return count;

	if ((val && !(set & fa->feature_bit)) ||
	    (!val && !(clear & fa->feature_bit))) {
		btrfs_info(fs_info,
			"%sabling feature %s on mounted fs is not supported.",
			val ? "En" : "Dis", fa->kobj_attr.attr.name);
		return -EPERM;
	}

	btrfs_info(fs_info, "%s %s feature flag",
		   val ? "Setting" : "Clearing", fa->kobj_attr.attr.name);

	spin_lock(&fs_info->super_lock);
	features = get_features(fs_info, fa->feature_set);
	if (val)
		features |= fa->feature_bit;
	else
		features &= ~fa->feature_bit;
	set_features(fs_info, fa->feature_set, features);
	spin_unlock(&fs_info->super_lock);

	/*
	 * We don't want to do full transaction commit from inside sysfs
	 */
	btrfs_set_pending(fs_info, COMMIT);
	wake_up_process(fs_info->transaction_kthread);

	return count;
}

static umode_t btrfs_feature_visible(struct kobject *kobj,
				     struct attribute *attr, int unused)
{
	struct btrfs_fs_info *fs_info = to_fs_info(kobj);
	umode_t mode = attr->mode;

	if (fs_info) {
		struct btrfs_feature_attr *fa;
		u64 features;

		fa = attr_to_btrfs_feature_attr(attr);
		features = get_features(fs_info, fa->feature_set);

		if (can_modify_feature(fa))
			mode |= S_IWUSR;
		else if (!(features & fa->feature_bit))
			mode = 0;
	}

	return mode;
}

BTRFS_FEAT_ATTR_INCOMPAT(mixed_backref, MIXED_BACKREF);
BTRFS_FEAT_ATTR_INCOMPAT(default_subvol, DEFAULT_SUBVOL);
BTRFS_FEAT_ATTR_INCOMPAT(mixed_groups, MIXED_GROUPS);
BTRFS_FEAT_ATTR_INCOMPAT(compress_lzo, COMPRESS_LZO);
BTRFS_FEAT_ATTR_INCOMPAT(big_metadata, BIG_METADATA);
BTRFS_FEAT_ATTR_INCOMPAT(extended_iref, EXTENDED_IREF);
BTRFS_FEAT_ATTR_INCOMPAT(raid56, RAID56);
BTRFS_FEAT_ATTR_INCOMPAT(skinny_metadata, SKINNY_METADATA);
BTRFS_FEAT_ATTR_INCOMPAT(no_holes, NO_HOLES);

static struct attribute *btrfs_supported_feature_attrs[] = {
	BTRFS_FEAT_ATTR_PTR(mixed_backref),
	BTRFS_FEAT_ATTR_PTR(default_subvol),
	BTRFS_FEAT_ATTR_PTR(mixed_groups),
	BTRFS_FEAT_ATTR_PTR(compress_lzo),
	BTRFS_FEAT_ATTR_PTR(big_metadata),
	BTRFS_FEAT_ATTR_PTR(extended_iref),
	BTRFS_FEAT_ATTR_PTR(raid56),
	BTRFS_FEAT_ATTR_PTR(skinny_metadata),
	BTRFS_FEAT_ATTR_PTR(no_holes),
	NULL
};

static const struct attribute_group btrfs_feature_attr_group = {
	.name = "features",
	.is_visible = btrfs_feature_visible,
	.attrs = btrfs_supported_feature_attrs,
};

static ssize_t btrfs_show_u64(u64 *value_ptr, spinlock_t *lock, char *buf)
{
	u64 val;
	if (lock)
		spin_lock(lock);
	val = *value_ptr;
	if (lock)
		spin_unlock(lock);
	return snprintf(buf, PAGE_SIZE, "%llu\n", val);
}

static ssize_t global_rsv_size_show(struct kobject *kobj,
				    struct kobj_attribute *ka, char *buf)
{
	struct btrfs_fs_info *fs_info = to_fs_info(kobj->parent);
	struct btrfs_block_rsv *block_rsv = &fs_info->global_block_rsv;
	return btrfs_show_u64(&block_rsv->size, &block_rsv->lock, buf);
}
BTRFS_ATTR(global_rsv_size, global_rsv_size_show);

static ssize_t global_rsv_reserved_show(struct kobject *kobj,
					struct kobj_attribute *a, char *buf)
{
	struct btrfs_fs_info *fs_info = to_fs_info(kobj->parent);
	struct btrfs_block_rsv *block_rsv = &fs_info->global_block_rsv;
	return btrfs_show_u64(&block_rsv->reserved, &block_rsv->lock, buf);
}
BTRFS_ATTR(global_rsv_reserved, global_rsv_reserved_show);

#define to_space_info(_kobj) container_of(_kobj, struct btrfs_space_info, kobj)
#define to_raid_kobj(_kobj) container_of(_kobj, struct raid_kobject, kobj)

static ssize_t raid_bytes_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf);
BTRFS_RAID_ATTR(total_bytes, raid_bytes_show);
BTRFS_RAID_ATTR(used_bytes, raid_bytes_show);

static ssize_t raid_bytes_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)

{
	struct btrfs_space_info *sinfo = to_space_info(kobj->parent);
	struct btrfs_block_group_cache *block_group;
	int index = to_raid_kobj(kobj)->raid_type;
	u64 val = 0;

	down_read(&sinfo->groups_sem);
	list_for_each_entry(block_group, &sinfo->block_groups[index], list) {
		if (&attr->attr == BTRFS_RAID_ATTR_PTR(total_bytes))
			val += block_group->key.offset;
		else
			val += btrfs_block_group_used(&block_group->item);
	}
	up_read(&sinfo->groups_sem);
	return snprintf(buf, PAGE_SIZE, "%llu\n", val);
}

static struct attribute *raid_attributes[] = {
	BTRFS_RAID_ATTR_PTR(total_bytes),
	BTRFS_RAID_ATTR_PTR(used_bytes),
	NULL
};

static void release_raid_kobj(struct kobject *kobj)
{
	kfree(to_raid_kobj(kobj));
}

struct kobj_type btrfs_raid_ktype = {
	.sysfs_ops = &kobj_sysfs_ops,
	.release = release_raid_kobj,
	.default_attrs = raid_attributes,
};

#define SPACE_INFO_ATTR(field)						\
static ssize_t btrfs_space_info_show_##field(struct kobject *kobj,	\
					     struct kobj_attribute *a,	\
					     char *buf)			\
{									\
	struct btrfs_space_info *sinfo = to_space_info(kobj);		\
	return btrfs_show_u64(&sinfo->field, &sinfo->lock, buf);	\
}									\
BTRFS_ATTR(field, btrfs_space_info_show_##field)

static ssize_t btrfs_space_info_show_total_bytes_pinned(struct kobject *kobj,
						       struct kobj_attribute *a,
						       char *buf)
{
	struct btrfs_space_info *sinfo = to_space_info(kobj);
	s64 val = percpu_counter_sum(&sinfo->total_bytes_pinned);
	return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

SPACE_INFO_ATTR(flags);
SPACE_INFO_ATTR(total_bytes);
SPACE_INFO_ATTR(bytes_used);
SPACE_INFO_ATTR(bytes_pinned);
SPACE_INFO_ATTR(bytes_reserved);
SPACE_INFO_ATTR(bytes_may_use);
SPACE_INFO_ATTR(disk_used);
SPACE_INFO_ATTR(disk_total);
BTRFS_ATTR(total_bytes_pinned, btrfs_space_info_show_total_bytes_pinned);

static struct attribute *space_info_attrs[] = {
	BTRFS_ATTR_PTR(flags),
	BTRFS_ATTR_PTR(total_bytes),
	BTRFS_ATTR_PTR(bytes_used),
	BTRFS_ATTR_PTR(bytes_pinned),
	BTRFS_ATTR_PTR(bytes_reserved),
	BTRFS_ATTR_PTR(bytes_may_use),
	BTRFS_ATTR_PTR(disk_used),
	BTRFS_ATTR_PTR(disk_total),
	BTRFS_ATTR_PTR(total_bytes_pinned),
	NULL,
};

static void space_info_release(struct kobject *kobj)
{
	struct btrfs_space_info *sinfo = to_space_info(kobj);
	percpu_counter_destroy(&sinfo->total_bytes_pinned);
	kfree(sinfo);
}

struct kobj_type space_info_ktype = {
	.sysfs_ops = &kobj_sysfs_ops,
	.release = space_info_release,
	.default_attrs = space_info_attrs,
};

static const struct attribute *allocation_attrs[] = {
	BTRFS_ATTR_PTR(global_rsv_reserved),
	BTRFS_ATTR_PTR(global_rsv_size),
	NULL,
};

static ssize_t btrfs_label_show(struct kobject *kobj,
				struct kobj_attribute *a, char *buf)
{
	struct btrfs_fs_info *fs_info = to_fs_info(kobj);
	char *label = fs_info->super_copy->label;
	return snprintf(buf, PAGE_SIZE, label[0] ? "%s\n" : "%s", label);
}

static ssize_t btrfs_label_store(struct kobject *kobj,
				 struct kobj_attribute *a,
				 const char *buf, size_t len)
{
	struct btrfs_fs_info *fs_info = to_fs_info(kobj);
	size_t p_len;

	if (fs_info->sb->s_flags & MS_RDONLY)
		return -EROFS;

	/*
	 * p_len is the len until the first occurrence of either
	 * '\n' or '\0'
	 */
	p_len = strcspn(buf, "\n");

	if (p_len >= BTRFS_LABEL_SIZE)
		return -EINVAL;

	spin_lock(&fs_info->super_lock);
	memset(fs_info->super_copy->label, 0, BTRFS_LABEL_SIZE);
	memcpy(fs_info->super_copy->label, buf, p_len);
	spin_unlock(&fs_info->super_lock);

	/*
	 * We don't want to do full transaction commit from inside sysfs
	 */
	btrfs_set_pending(fs_info, COMMIT);
	wake_up_process(fs_info->transaction_kthread);

	return len;
}
BTRFS_ATTR_RW(label, btrfs_label_show, btrfs_label_store);

static ssize_t btrfs_nodesize_show(struct kobject *kobj,
				struct kobj_attribute *a, char *buf)
{
	struct btrfs_fs_info *fs_info = to_fs_info(kobj);

	return snprintf(buf, PAGE_SIZE, "%u\n", fs_info->super_copy->nodesize);
}

BTRFS_ATTR(nodesize, btrfs_nodesize_show);

static ssize_t btrfs_sectorsize_show(struct kobject *kobj,
				struct kobj_attribute *a, char *buf)
{
	struct btrfs_fs_info *fs_info = to_fs_info(kobj);

	return snprintf(buf, PAGE_SIZE, "%u\n", fs_info->super_copy->sectorsize);
}

BTRFS_ATTR(sectorsize, btrfs_sectorsize_show);

static ssize_t btrfs_clone_alignment_show(struct kobject *kobj,
				struct kobj_attribute *a, char *buf)
{
	struct btrfs_fs_info *fs_info = to_fs_info(kobj);

	return snprintf(buf, PAGE_SIZE, "%u\n", fs_info->super_copy->sectorsize);
}

BTRFS_ATTR(clone_alignment, btrfs_clone_alignment_show);

static const struct attribute *btrfs_attrs[] = {
	BTRFS_ATTR_PTR(label),
	BTRFS_ATTR_PTR(nodesize),
	BTRFS_ATTR_PTR(sectorsize),
	BTRFS_ATTR_PTR(clone_alignment),
	NULL,
};

static void btrfs_release_fsid_kobj(struct kobject *kobj)
{
	struct btrfs_fs_devices *fs_devs = to_fs_devs(kobj);

	memset(&fs_devs->fsid_kobj, 0, sizeof(struct kobject));
	complete(&fs_devs->kobj_unregister);
}

static struct kobj_type btrfs_ktype = {
	.sysfs_ops	= &kobj_sysfs_ops,
	.release	= btrfs_release_fsid_kobj,
};

static inline struct btrfs_fs_devices *to_fs_devs(struct kobject *kobj)
{
	if (kobj->ktype != &btrfs_ktype)
		return NULL;
	return container_of(kobj, struct btrfs_fs_devices, fsid_kobj);
}

static inline struct btrfs_fs_info *to_fs_info(struct kobject *kobj)
{
	if (kobj->ktype != &btrfs_ktype)
		return NULL;
	return to_fs_devs(kobj)->fs_info;
}

#define NUM_FEATURE_BITS 64
static char btrfs_unknown_feature_names[3][NUM_FEATURE_BITS][13];
static struct btrfs_feature_attr btrfs_feature_attrs[3][NUM_FEATURE_BITS];

static const u64 supported_feature_masks[3] = {
	[FEAT_COMPAT]    = BTRFS_FEATURE_COMPAT_SUPP,
	[FEAT_COMPAT_RO] = BTRFS_FEATURE_COMPAT_RO_SUPP,
	[FEAT_INCOMPAT]  = BTRFS_FEATURE_INCOMPAT_SUPP,
};

static int addrm_unknown_feature_attrs(struct btrfs_fs_info *fs_info, bool add)
{
	int set;

	for (set = 0; set < FEAT_MAX; set++) {
		int i;
		struct attribute *attrs[2];
		struct attribute_group agroup = {
			.name = "features",
			.attrs = attrs,
		};
		u64 features = get_features(fs_info, set);
		features &= ~supported_feature_masks[set];

		if (!features)
			continue;

		attrs[1] = NULL;
		for (i = 0; i < NUM_FEATURE_BITS; i++) {
			struct btrfs_feature_attr *fa;

			if (!(features & (1ULL << i)))
				continue;

			fa = &btrfs_feature_attrs[set][i];
			attrs[0] = &fa->kobj_attr.attr;
			if (add) {
				int ret;
				ret = sysfs_merge_group(&fs_info->fs_devices->fsid_kobj,
							&agroup);
				if (ret)
					return ret;
			} else
				sysfs_unmerge_group(&fs_info->fs_devices->fsid_kobj,
						    &agroup);
		}

	}
	return 0;
}

static void __btrfs_sysfs_remove_fsid(struct btrfs_fs_devices *fs_devs)
{
	if (fs_devs->device_dir_kobj) {
		kobject_del(fs_devs->device_dir_kobj);
		kobject_put(fs_devs->device_dir_kobj);
		fs_devs->device_dir_kobj = NULL;
	}

	if (fs_devs->fsid_kobj.state_initialized) {
		kobject_del(&fs_devs->fsid_kobj);
		kobject_put(&fs_devs->fsid_kobj);
		wait_for_completion(&fs_devs->kobj_unregister);
	}
}

/* when fs_devs is NULL it will remove all fsid kobject */
void btrfs_sysfs_remove_fsid(struct btrfs_fs_devices *fs_devs)
{
	struct list_head *fs_uuids = btrfs_get_fs_uuids();

	if (fs_devs) {
		__btrfs_sysfs_remove_fsid(fs_devs);
		return;
	}

	list_for_each_entry(fs_devs, fs_uuids, list) {
		__btrfs_sysfs_remove_fsid(fs_devs);
	}
}

void btrfs_sysfs_remove_mounted(struct btrfs_fs_info *fs_info)
{
	btrfs_reset_fs_info_ptr(fs_info);

	if (fs_info->space_info_kobj) {
		sysfs_remove_files(fs_info->space_info_kobj, allocation_attrs);
		kobject_del(fs_info->space_info_kobj);
		kobject_put(fs_info->space_info_kobj);
	}
	addrm_unknown_feature_attrs(fs_info, false);
	sysfs_remove_group(&fs_info->fs_devices->fsid_kobj, &btrfs_feature_attr_group);
	sysfs_remove_files(&fs_info->fs_devices->fsid_kobj, btrfs_attrs);
	btrfs_sysfs_rm_device_link(fs_info->fs_devices, NULL);
}

const char * const btrfs_feature_set_names[3] = {
	[FEAT_COMPAT]	 = "compat",
	[FEAT_COMPAT_RO] = "compat_ro",
	[FEAT_INCOMPAT]	 = "incompat",
};

char *btrfs_printable_features(enum btrfs_feature_set set, u64 flags)
{
	size_t bufsize = 4096; /* safe max, 64 names * 64 bytes */
	int len = 0;
	int i;
	char *str;

	str = kmalloc(bufsize, GFP_KERNEL);
	if (!str)
		return str;

	for (i = 0; i < ARRAY_SIZE(btrfs_feature_attrs[set]); i++) {
		const char *name;

		if (!(flags & (1ULL << i)))
			continue;

		name = btrfs_feature_attrs[set][i].kobj_attr.attr.name;
		len += snprintf(str + len, bufsize - len, "%s%s",
				len ? "," : "", name);
	}

	return str;
}

static void init_feature_attrs(void)
{
	struct btrfs_feature_attr *fa;
	int set, i;

	BUILD_BUG_ON(ARRAY_SIZE(btrfs_unknown_feature_names) !=
		     ARRAY_SIZE(btrfs_feature_attrs));
	BUILD_BUG_ON(ARRAY_SIZE(btrfs_unknown_feature_names[0]) !=
		     ARRAY_SIZE(btrfs_feature_attrs[0]));

	memset(btrfs_feature_attrs, 0, sizeof(btrfs_feature_attrs));
	memset(btrfs_unknown_feature_names, 0,
	       sizeof(btrfs_unknown_feature_names));

	for (i = 0; btrfs_supported_feature_attrs[i]; i++) {
		struct btrfs_feature_attr *sfa;
		struct attribute *a = btrfs_supported_feature_attrs[i];
		int bit;
		sfa = attr_to_btrfs_feature_attr(a);
		bit = ilog2(sfa->feature_bit);
		fa = &btrfs_feature_attrs[sfa->feature_set][bit];

		fa->kobj_attr.attr.name = sfa->kobj_attr.attr.name;
	}

	for (set = 0; set < FEAT_MAX; set++) {
		for (i = 0; i < ARRAY_SIZE(btrfs_feature_attrs[set]); i++) {
			char *name = btrfs_unknown_feature_names[set][i];
			fa = &btrfs_feature_attrs[set][i];

			if (fa->kobj_attr.attr.name)
				continue;

			snprintf(name, 13, "%s:%u",
				 btrfs_feature_set_names[set], i);

			fa->kobj_attr.attr.name = name;
			fa->kobj_attr.attr.mode = S_IRUGO;
			fa->feature_set = set;
			fa->feature_bit = 1ULL << i;
		}
	}
}

/* when one_device is NULL, it removes all device links */

int btrfs_sysfs_rm_device_link(struct btrfs_fs_devices *fs_devices,
		struct btrfs_device *one_device)
{
	struct hd_struct *disk;
	struct kobject *disk_kobj;

	if (!fs_devices->device_dir_kobj)
		return -EINVAL;

	if (one_device && one_device->bdev) {
		disk = one_device->bdev->bd_part;
		disk_kobj = &part_to_dev(disk)->kobj;

		sysfs_remove_link(fs_devices->device_dir_kobj,
						disk_kobj->name);
	}

	if (one_device)
		return 0;

	list_for_each_entry(one_device,
			&fs_devices->devices, dev_list) {
		if (!one_device->bdev)
			continue;
		disk = one_device->bdev->bd_part;
		disk_kobj = &part_to_dev(disk)->kobj;

		sysfs_remove_link(fs_devices->device_dir_kobj,
						disk_kobj->name);
	}

	return 0;
}

int btrfs_sysfs_add_device(struct btrfs_fs_devices *fs_devs)
{
	if (!fs_devs->device_dir_kobj)
		fs_devs->device_dir_kobj = kobject_create_and_add("devices",
						&fs_devs->fsid_kobj);

	if (!fs_devs->device_dir_kobj)
		return -ENOMEM;

	return 0;
}

int btrfs_sysfs_add_device_link(struct btrfs_fs_devices *fs_devices,
				struct btrfs_device *one_device)
{
	int error = 0;
	struct btrfs_device *dev;

	list_for_each_entry(dev, &fs_devices->devices, dev_list) {
		struct hd_struct *disk;
		struct kobject *disk_kobj;

		if (!dev->bdev)
			continue;

		if (one_device && one_device != dev)
			continue;

		disk = dev->bdev->bd_part;
		disk_kobj = &part_to_dev(disk)->kobj;

		error = sysfs_create_link(fs_devices->device_dir_kobj,
					  disk_kobj, disk_kobj->name);
		if (error)
			break;
	}

	return error;
}

/* /sys/fs/btrfs/ entry */
static struct kset *btrfs_kset;

/* /sys/kernel/debug/btrfs */
static struct dentry *btrfs_debugfs_root_dentry;

/* Debugging tunables and exported data */
u64 btrfs_debugfs_test;

/*
 * Can be called by the device discovery thread.
 * And parent can be specified for seed device
 */
int btrfs_sysfs_add_fsid(struct btrfs_fs_devices *fs_devs,
				struct kobject *parent)
{
	int error;

	init_completion(&fs_devs->kobj_unregister);
	fs_devs->fsid_kobj.kset = btrfs_kset;
	error = kobject_init_and_add(&fs_devs->fsid_kobj,
				&btrfs_ktype, parent, "%pU", fs_devs->fsid);
	if (error) {
		kobject_put(&fs_devs->fsid_kobj);
		return error;
	}

	return 0;
}

int btrfs_sysfs_add_mounted(struct btrfs_fs_info *fs_info)
{
	int error;
	struct btrfs_fs_devices *fs_devs = fs_info->fs_devices;
	struct kobject *fsid_kobj = &fs_devs->fsid_kobj;

	btrfs_set_fs_info_ptr(fs_info);

	error = btrfs_sysfs_add_device_link(fs_devs, NULL);
	if (error)
		return error;

	error = sysfs_create_files(fsid_kobj, btrfs_attrs);
	if (error) {
		btrfs_sysfs_rm_device_link(fs_devs, NULL);
		return error;
	}

	error = sysfs_create_group(fsid_kobj,
				   &btrfs_feature_attr_group);
	if (error)
		goto failure;

	error = addrm_unknown_feature_attrs(fs_info, true);
	if (error)
		goto failure;

	fs_info->space_info_kobj = kobject_create_and_add("allocation",
						  fsid_kobj);
	if (!fs_info->space_info_kobj) {
		error = -ENOMEM;
		goto failure;
	}

	error = sysfs_create_files(fs_info->space_info_kobj, allocation_attrs);
	if (error)
		goto failure;

	return 0;
failure:
	btrfs_sysfs_remove_mounted(fs_info);
	return error;
}

static int btrfs_init_debugfs(void)
{
#ifdef CONFIG_DEBUG_FS
	btrfs_debugfs_root_dentry = debugfs_create_dir("btrfs", NULL);
	if (!btrfs_debugfs_root_dentry)
		return -ENOMEM;

	debugfs_create_u64("test", S_IRUGO | S_IWUGO, btrfs_debugfs_root_dentry,
			&btrfs_debugfs_test);
#endif
	return 0;
}

int btrfs_init_sysfs(void)
{
	int ret;

	btrfs_kset = kset_create_and_add("btrfs", NULL, fs_kobj);
	if (!btrfs_kset)
		return -ENOMEM;

	ret = btrfs_init_debugfs();
	if (ret)
		goto out1;

	init_feature_attrs();
	ret = sysfs_create_group(&btrfs_kset->kobj, &btrfs_feature_attr_group);
	if (ret)
		goto out2;

	return 0;
out2:
	debugfs_remove_recursive(btrfs_debugfs_root_dentry);
out1:
	kset_unregister(btrfs_kset);

	return ret;
}

void btrfs_exit_sysfs(void)
{
	sysfs_remove_group(&btrfs_kset->kobj, &btrfs_feature_attr_group);
	kset_unregister(btrfs_kset);
	debugfs_remove_recursive(btrfs_debugfs_root_dentry);
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 #ifndef _BTRFS_SYSFS_H_
#define _BTRFS_SYSFS_H_

/*
 * Data exported through sysfs
 */
extern u64 btrfs_debugfs_test;

enum btrfs_feature_set {
	FEAT_COMPAT,
	FEAT_COMPAT_RO,
	FEAT_INCOMPAT,
	FEAT_MAX
};

#define __INIT_KOBJ_ATTR(_name, _mode, _show, _store)			\
{									\
	.attr	= { .name = __stringify(_name), .mode = _mode },	\
	.show	= _show,						\
	.store	= _store,						\
}

#define BTRFS_ATTR_RW(_name, _show, _store)			\
	static struct kobj_attribute btrfs_attr_##_name =		\
			__INIT_KOBJ_ATTR(_name, 0644, _show, _store)

#define BTRFS_ATTR(_name, _show)					\
	static struct kobj_attribute btrfs_attr_##_name =		\
			__INIT_KOBJ_ATTR(_name, 0444, _show, NULL)

#define BTRFS_ATTR_PTR(_name)    (&btrfs_attr_##_name.attr)

#define BTRFS_RAID_ATTR(_name, _show)					\
	static struct kobj_attribute btrfs_raid_attr_##_name =		\
			__INIT_KOBJ_ATTR(_name, 0444, _show, NULL)

#define BTRFS_RAID_ATTR_PTR(_name)    (&btrfs_raid_attr_##_name.attr)


struct btrfs_feature_attr {
	struct kobj_attribute kobj_attr;
	enum btrfs_feature_set feature_set;
	u64 feature_bit;
};

#define BTRFS_FEAT_ATTR(_name, _feature_set, _prefix, _feature_bit)	     \
static struct btrfs_feature_attr btrfs_attr_##_name = {			     \
	.kobj_attr = __INIT_KOBJ_ATTR(_name, S_IRUGO,			     \
				      btrfs_feature_attr_show,		     \
				      btrfs_feature_attr_store),	     \
	.feature_set	= _feature_set,					     \
	.feature_bit	= _prefix ##_## _feature_bit,			     \
}
#define BTRFS_FEAT_ATTR_PTR(_name)    (&btrfs_attr_##_name.kobj_attr.attr)

#define BTRFS_FEAT_ATTR_COMPAT(name, feature) \
	BTRFS_FEAT_ATTR(name, FEAT_COMPAT, BTRFS_FEATURE_COMPAT, feature)
#define BTRFS_FEAT_ATTR_COMPAT_RO(name, feature) \
	BTRFS_FEAT_ATTR(name, FEAT_COMPAT_RO, BTRFS_FEATURE_COMPAT, feature)
#define BTRFS_FEAT_ATTR_INCOMPAT(name, feature) \
	BTRFS_FEAT_ATTR(name, FEAT_INCOMPAT, BTRFS_FEATURE_INCOMPAT, feature)

/* convert from attribute */
static inline struct btrfs_feature_attr *
to_btrfs_feature_attr(struct kobj_attribute *a)
{
	return container_of(a, struct btrfs_feature_attr, kobj_attr);
}

static inline struct kobj_attribute *attr_to_btrfs_attr(struct attribute *attr)
{
	return container_of(attr, struct kobj_attribute, attr);
}

static inline struct btrfs_feature_attr *
attr_to_btrfs_feature_attr(struct attribute *attr)
{
	return to_btrfs_feature_attr(attr_to_btrfs_attr(attr));
}

char *btrfs_printable_features(enum btrfs_feature_set set, u64 flags);
extern const char * const btrfs_feature_set_names[3];
extern struct kobj_type space_info_ktype;
extern struct kobj_type btrfs_raid_ktype;
int btrfs_sysfs_add_device_link(struct btrfs_fs_devices *fs_devices,
		struct btrfs_device *one_device);
int btrfs_sysfs_rm_device_link(struct btrfs_fs_devices *fs_devices,
                struct btrfs_device *one_device);
int btrfs_sysfs_add_fsid(struct btrfs_fs_devices *fs_devs,
				struct kobject *parent);
int btrfs_sysfs_add_device(struct btrfs_fs_devices *fs_devs);
void btrfs_sysfs_remove_fsid(struct btrfs_fs_devices *fs_devs);
#endif /* _BTRFS_SYSFS_H_ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright (C) 2013 Fusion IO.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/magic.h>
#include "btrfs-tests.h"
#include "../ctree.h"
#include "../volumes.h"
#include "../disk-io.h"
#include "../qgroup.h"

static struct vfsmount *test_mnt = NULL;

static const struct super_operations btrfs_test_super_ops = {
	.alloc_inode	= btrfs_alloc_inode,
	.destroy_inode	= btrfs_test_destroy_inode,
};

static struct dentry *btrfs_test_mount(struct file_system_type *fs_type,
				       int flags, const char *dev_name,
				       void *data)
{
	return mount_pseudo(fs_type, "btrfs_test:", &btrfs_test_super_ops,
			    NULL, BTRFS_TEST_MAGIC);
}

static struct file_system_type test_type = {
	.name		= "btrfs_test_fs",
	.mount		= btrfs_test_mount,
	.kill_sb	= kill_anon_super,
};

struct inode *btrfs_new_test_inode(void)
{
	struct inode *inode;

	inode = new_inode(test_mnt->mnt_sb);
	if (inode)
		inode_init_owner(inode, NULL, S_IFREG);

	return inode;
}

int btrfs_init_test_fs(void)
{
	int ret;

	ret = register_filesystem(&test_type);
	if (ret) {
		printk(KERN_ERR "btrfs: cannot register test file system\n");
		return ret;
	}

	test_mnt = kern_mount(&test_type);
	if (IS_ERR(test_mnt)) {
		printk(KERN_ERR "btrfs: cannot mount test file system\n");
		unregister_filesystem(&test_type);
		return ret;
	}
	return 0;
}

void btrfs_destroy_test_fs(void)
{
	kern_unmount(test_mnt);
	unregister_filesystem(&test_type);
}

struct btrfs_fs_info *btrfs_alloc_dummy_fs_info(void)
{
	struct btrfs_fs_info *fs_info = kzalloc(sizeof(struct btrfs_fs_info),
						GFP_NOFS);

	if (!fs_info)
		return fs_info;
	fs_info->fs_devices = kzalloc(sizeof(struct btrfs_fs_devices),
				      GFP_NOFS);
	if (!fs_info->fs_devices) {
		kfree(fs_info);
		return NULL;
	}
	fs_info->super_copy = kzalloc(sizeof(struct btrfs_super_block),
				      GFP_NOFS);
	if (!fs_info->super_copy) {
		kfree(fs_info->fs_devices);
		kfree(fs_info);
		return NULL;
	}

	if (init_srcu_struct(&fs_info->subvol_srcu)) {
		kfree(fs_info->fs_devices);
		kfree(fs_info->super_copy);
		kfree(fs_info);
		return NULL;
	}

	spin_lock_init(&fs_info->buffer_lock);
	spin_lock_init(&fs_info->qgroup_lock);
	spin_lock_init(&fs_info->qgroup_op_lock);
	spin_lock_init(&fs_info->super_lock);
	spin_lock_init(&fs_info->fs_roots_radix_lock);
	mutex_init(&fs_info->qgroup_ioctl_lock);
	mutex_init(&fs_info->qgroup_rescan_lock);
	rwlock_init(&fs_info->tree_mod_log_lock);
	fs_info->running_transaction = NULL;
	fs_info->qgroup_tree = RB_ROOT;
	fs_info->qgroup_ulist = NULL;
	atomic64_set(&fs_info->tree_mod_seq, 0);
	INIT_LIST_HEAD(&fs_info->dirty_qgroups);
	INIT_LIST_HEAD(&fs_info->dead_roots);
	INIT_LIST_HEAD(&fs_info->tree_mod_seq_list);
	INIT_RADIX_TREE(&fs_info->buffer_radix, GFP_ATOMIC);
	INIT_RADIX_TREE(&fs_info->fs_roots_radix, GFP_ATOMIC);
	return fs_info;
}

static void btrfs_free_dummy_fs_info(struct btrfs_fs_info *fs_info)
{
	struct radix_tree_iter iter;
	void **slot;

	spin_lock(&fs_info->buffer_lock);
restart:
	radix_tree_for_each_slot(slot, &fs_info->buffer_radix, &iter, 0) {
		struct extent_buffer *eb;

		eb = radix_tree_deref_slot_protected(slot, &fs_info->buffer_lock);
		if (!eb)
			continue;
		/* Shouldn't happen but that kind of thinking creates CVE's */
		if (radix_tree_exception(eb)) {
			if (radix_tree_deref_retry(eb))
				goto restart;
			continue;
		}
		spin_unlock(&fs_info->buffer_lock);
		free_extent_buffer_stale(eb);
		spin_lock(&fs_info->buffer_lock);
	}
	spin_unlock(&fs_info->buffer_lock);

	btrfs_free_qgroup_config(fs_info);
	btrfs_free_fs_roots(fs_info);
	cleanup_srcu_struct(&fs_info->subvol_srcu);
	kfree(fs_info->super_copy);
	kfree(fs_info->fs_devices);
	kfree(fs_info);
}

void btrfs_free_dummy_root(struct btrfs_root *root)
{
	if (IS_ERR_OR_NULL(root))
		return;
	if (root->node)
		free_extent_buffer(root->node);
	if (root->fs_info)
		btrfs_free_dummy_fs_info(root->fs_info);
	kfree(root);
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 * Copyright (C) 2013 Fusion IO.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#ifndef __BTRFS_TESTS
#define __BTRFS_TESTS

#ifdef CONFIG_BTRFS_FS_RUN_SANITY_TESTS

#define test_msg(fmt, ...) pr_info("BTRFS: selftest: " fmt, ##__VA_ARGS__)

struct btrfs_root;

int btrfs_test_free_space_cache(void);
int btrfs_test_extent_buffer_operations(void);
int btrfs_test_extent_io(void);
int btrfs_test_inodes(void);
int btrfs_test_qgroups(void);
int btrfs_init_test_fs(void);
void btrfs_destroy_test_fs(void);
struct inode *btrfs_new_test_inode(void);
struct btrfs_fs_info *btrfs_alloc_dummy_fs_info(void);
void btrfs_free_dummy_root(struct btrfs_root *root);
#else
static inline int btrfs_test_free_space_cache(void)
{
	return 0;
}
static inline int btrfs_test_extent_buffer_operations(void)
{
	return 0;
}
static inline int btrfs_init_test_fs(void)
{
	return 0;
}
static inline void btrfs_destroy_test_fs(void)
{
}
static inline int btrfs_test_extent_io(void)
{
	return 0;
}
static inline int btrfs_test_inodes(void)
{
	return 0;
}
static inline int btrfs_test_qgroups(void)
{
	return 0;
}
#endif

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * Copyright (C) 2013 Fusion IO.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/slab.h>
#include "btrfs-tests.h"
#include "../ctree.h"
#include "../extent_io.h"
#include "../disk-io.h"

static int test_btrfs_split_item(void)
{
	struct btrfs_path *path;
	struct btrfs_root *root;
	struct extent_buffer *eb;
	struct btrfs_item *item;
	char *value = "mary had a little lamb";
	char *split1 = "mary had a little";
	char *split2 = " lamb";
	char *split3 = "mary";
	char *split4 = " had a little";
	char buf[32];
	struct btrfs_key key;
	u32 value_len = strlen(value);
	int ret = 0;

	test_msg("Running btrfs_split_item tests\n");

	root = btrfs_alloc_dummy_root();
	if (IS_ERR(root)) {
		test_msg("Could not allocate root\n");
		return PTR_ERR(root);
	}

	path = btrfs_alloc_path();
	if (!path) {
		test_msg("Could not allocate path\n");
		kfree(root);
		return -ENOMEM;
	}

	path->nodes[0] = eb = alloc_dummy_extent_buffer(NULL, 4096);
	if (!eb) {
		test_msg("Could not allocate dummy buffer\n");
		ret = -ENOMEM;
		goto out;
	}
	path->slots[0] = 0;

	key.objectid = 0;
	key.type = BTRFS_EXTENT_CSUM_KEY;
	key.offset = 0;

	setup_items_for_insert(root, path, &key, &value_len, value_len,
			       value_len + sizeof(struct btrfs_item), 1);
	item = btrfs_item_nr(0);
	write_extent_buffer(eb, value, btrfs_item_ptr_offset(eb, 0),
			    value_len);

	key.offset = 3;

	/*
	 * Passing NULL trans here should be safe because we have plenty of
	 * space in this leaf to split the item without having to split the
	 * leaf.
	 */
	ret = btrfs_split_item(NULL, root, path, &key, 17);
	if (ret) {
		test_msg("Split item failed %d\n", ret);
		goto out;
	}

	/*
	 * Read the first slot, it should have the original key and contain only
	 * 'mary had a little'
	 */
	btrfs_item_key_to_cpu(eb, &key, 0);
	if (key.objectid != 0 || key.type != BTRFS_EXTENT_CSUM_KEY ||
	    key.offset != 0) {
		test_msg("Invalid key at slot 0\n");
		ret = -EINVAL;
		goto out;
	}

	item = btrfs_item_nr(0);
	if (btrfs_item_size(eb, item) != strlen(split1)) {
		test_msg("Invalid len in the first split\n");
		ret = -EINVAL;
		goto out;
	}

	read_extent_buffer(eb, buf, btrfs_item_ptr_offset(eb, 0),
			   strlen(split1));
	if (memcmp(buf, split1, strlen(split1))) {
		test_msg("Data in the buffer doesn't match what it should "
			 "in the first split have='%.*s' want '%s'\n",
			 (int)strlen(split1), buf, split1);
		ret = -EINVAL;
		goto out;
	}

	btrfs_item_key_to_cpu(eb, &key, 1);
	if (key.objectid != 0 || key.type != BTRFS_EXTENT_CSUM_KEY ||
	    key.offset != 3) {
		test_msg("Invalid key at slot 1\n");
		ret = -EINVAL;
		goto out;
	}

	item = btrfs_item_nr(1);
	if (btrfs_item_size(eb, item) != strlen(split2)) {
		test_msg("Invalid len in the second split\n");
		ret = -EINVAL;
		goto out;
	}

	read_extent_buffer(eb, buf, btrfs_item_ptr_offset(eb, 1),
			   strlen(split2));
	if (memcmp(buf, split2, strlen(split2))) {
		test_msg("Data in the buffer doesn't match what it should "
			 "in the second split\n");
		ret = -EINVAL;
		goto out;
	}

	key.offset = 1;
	/* Do it again so we test memmoving the other items in the leaf */
	ret = btrfs_split_item(NULL, root, path, &key, 4);
	if (ret) {
		test_msg("Second split item failed %d\n", ret);
		goto out;
	}

	btrfs_item_key_to_cpu(eb, &key, 0);
	if (key.objectid != 0 || key.type != BTRFS_EXTENT_CSUM_KEY ||
	    key.offset != 0) {
		test_msg("Invalid key at slot 0\n");
		ret = -EINVAL;
		goto out;
	}

	item = btrfs_item_nr(0);
	if (btrfs_item_size(eb, item) != strlen(split3)) {
		test_msg("Invalid len in the first split\n");
		ret = -EINVAL;
		goto out;
	}

	read_extent_buffer(eb, buf, btrfs_item_ptr_offset(eb, 0),
			   strlen(split3));
	if (memcmp(buf, split3, strlen(split3))) {
		test_msg("Data in the buffer doesn't match what it should "
			 "in the third split");
		ret = -EINVAL;
		goto out;
	}

	btrfs_item_key_to_cpu(eb, &key, 1);
	if (key.objectid != 0 || key.type != BTRFS_EXTENT_CSUM_KEY ||
	    key.offset != 1) {
		test_msg("Invalid key at slot 1\n");
		ret = -EINVAL;
		goto out;
	}

	item = btrfs_item_nr(1);
	if (btrfs_item_size(eb, item) != strlen(split4)) {
		test_msg("Invalid len in the second split\n");
		ret = -EINVAL;
		goto out;
	}

	read_extent_buffer(eb, buf, btrfs_item_ptr_offset(eb, 1),
			   strlen(split4));
	if (memcmp(buf, split4, strlen(split4))) {
		test_msg("Data in the buffer doesn't match what it should "
			 "in the fourth split\n");
		ret = -EINVAL;
		goto out;
	}

	btrfs_item_key_to_cpu(eb, &key, 2);
	if (key.objectid != 0 || key.type != BTRFS_EXTENT_CSUM_KEY ||
	    key.offset != 3) {
		test_msg("Invalid key at slot 2\n");
		ret = -EINVAL;
		goto out;
	}

	item = btrfs_item_nr(2);
	if (btrfs_item_size(eb, item) != strlen(split2)) {
		test_msg("Invalid len in the second split\n");
		ret = -EINVAL;
		goto out;
	}

	read_extent_buffer(eb, buf, btrfs_item_ptr_offset(eb, 2),
			   strlen(split2));
	if (memcmp(buf, split2, strlen(split2))) {
		test_msg("Data in the buffer doesn't match what it should "
			 "in the last chunk\n");
		ret = -EINVAL;
		goto out;
	}
out:
	btrfs_free_path(path);
	kfree(root);
	return ret;
}

int btrfs_test_extent_buffer_operations(void)
{
	test_msg("Running extent buffer operation tests");
	return test_btrfs_split_item();
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright (C) 2013 Fusion IO.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/pagemap.h>
#include <linux/sched.h>
#include "btrfs-tests.h"
#include "../extent_io.h"

#define PROCESS_UNLOCK		(1 << 0)
#define PROCESS_RELEASE		(1 << 1)
#define PROCESS_TEST_LOCKED	(1 << 2)

static noinline int process_page_range(struct inode *inode, u64 start, u64 end,
				       unsigned long flags)
{
	int ret;
	struct page *pages[16];
	unsigned long index = start >> PAGE_CACHE_SHIFT;
	unsigned long end_index = end >> PAGE_CACHE_SHIFT;
	unsigned long nr_pages = end_index - index + 1;
	int i;
	int count = 0;
	int loops = 0;

	while (nr_pages > 0) {
		ret = find_get_pages_contig(inode->i_mapping, index,
				     min_t(unsigned long, nr_pages,
				     ARRAY_SIZE(pages)), pages);
		for (i = 0; i < ret; i++) {
			if (flags & PROCESS_TEST_LOCKED &&
			    !PageLocked(pages[i]))
				count++;
			if (flags & PROCESS_UNLOCK && PageLocked(pages[i]))
				unlock_page(pages[i]);
			page_cache_release(pages[i]);
			if (flags & PROCESS_RELEASE)
				page_cache_release(pages[i]);
		}
		nr_pages -= ret;
		index += ret;
		cond_resched();
		loops++;
		if (loops > 100000) {
			printk(KERN_ERR "stuck in a loop, start %Lu, end %Lu, nr_pages %lu, ret %d\n", start, end, nr_pages, ret);
			break;
		}
	}
	return count;
}

static int test_find_delalloc(void)
{
	struct inode *inode;
	struct extent_io_tree tmp;
	struct page *page;
	struct page *locked_page = NULL;
	unsigned long index = 0;
	u64 total_dirty = 256 * 1024 * 1024;
	u64 max_bytes = 128 * 1024 * 1024;
	u64 start, end, test_start;
	u64 found;
	int ret = -EINVAL;

	inode = btrfs_new_test_inode();
	if (!inode) {
		test_msg("Failed to allocate test inode\n");
		return -ENOMEM;
	}

	extent_io_tree_init(&tmp, &inode->i_data);

	/*
	 * First go through and create and mark all of our pages dirty, we pin
	 * everything to make sure our pages don't get evicted and screw up our
	 * test.
	 */
	for (index = 0; index < (total_dirty >> PAGE_CACHE_SHIFT); index++) {
		page = find_or_create_page(inode->i_mapping, index, GFP_NOFS);
		if (!page) {
			test_msg("Failed to allocate test page\n");
			ret = -ENOMEM;
			goto out;
		}
		SetPageDirty(page);
		if (index) {
			unlock_page(page);
		} else {
			page_cache_get(page);
			locked_page = page;
		}
	}

	/* Test this scenario
	 * |--- delalloc ---|
	 * |---  search  ---|
	 */
	set_extent_delalloc(&tmp, 0, 4095, NULL, GFP_NOFS);
	start = 0;
	end = 0;
	found = find_lock_delalloc_range(inode, &tmp, locked_page, &start,
					 &end, max_bytes);
	if (!found) {
		test_msg("Should have found at least one delalloc\n");
		goto out_bits;
	}
	if (start != 0 || end != 4095) {
		test_msg("Expected start 0 end 4095, got start %Lu end %Lu\n",
			 start, end);
		goto out_bits;
	}
	unlock_extent(&tmp, start, end);
	unlock_page(locked_page);
	page_cache_release(locked_page);

	/*
	 * Test this scenario
	 *
	 * |--- delalloc ---|
	 *           |--- search ---|
	 */
	test_start = 64 * 1024 * 1024;
	locked_page = find_lock_page(inode->i_mapping,
				     test_start >> PAGE_CACHE_SHIFT);
	if (!locked_page) {
		test_msg("Couldn't find the locked page\n");
		goto out_bits;
	}
	set_extent_delalloc(&tmp, 4096, max_bytes - 1, NULL, GFP_NOFS);
	start = test_start;
	end = 0;
	found = find_lock_delalloc_range(inode, &tmp, locked_page, &start,
					 &end, max_bytes);
	if (!found) {
		test_msg("Couldn't find delalloc in our range\n");
		goto out_bits;
	}
	if (start != test_start || end != max_bytes - 1) {
		test_msg("Expected start %Lu end %Lu, got start %Lu, end "
			 "%Lu\n", test_start, max_bytes - 1, start, end);
		goto out_bits;
	}
	if (process_page_range(inode, start, end,
			       PROCESS_TEST_LOCKED | PROCESS_UNLOCK)) {
		test_msg("There were unlocked pages in the range\n");
		goto out_bits;
	}
	unlock_extent(&tmp, start, end);
	/* locked_page was unlocked above */
	page_cache_release(locked_page);

	/*
	 * Test this scenario
	 * |--- delalloc ---|
	 *                    |--- search ---|
	 */
	test_start = max_bytes + 4096;
	locked_page = find_lock_page(inode->i_mapping, test_start >>
				     PAGE_CACHE_SHIFT);
	if (!locked_page) {
		test_msg("Could'nt find the locked page\n");
		goto out_bits;
	}
	start = test_start;
	end = 0;
	found = find_lock_delalloc_range(inode, &tmp, locked_page, &start,
					 &end, max_bytes);
	if (found) {
		test_msg("Found range when we shouldn't have\n");
		goto out_bits;
	}
	if (end != (u64)-1) {
		test_msg("Did not return the proper end offset\n");
		goto out_bits;
	}

	/*
	 * Test this scenario
	 * [------- delalloc -------|
	 * [max_bytes]|-- search--|
	 *
	 * We are re-using our test_start from above since it works out well.
	 */
	set_extent_delalloc(&tmp, max_bytes, total_dirty - 1, NULL, GFP_NOFS);
	start = test_start;
	end = 0;
	found = find_lock_delalloc_range(inode, &tmp, locked_page, &start,
					 &end, max_bytes);
	if (!found) {
		test_msg("Didn't find our range\n");
		goto out_bits;
	}
	if (start != test_start || end != total_dirty - 1) {
		test_msg("Expected start %Lu end %Lu, got start %Lu end %Lu\n",
			 test_start, total_dirty - 1, start, end);
		goto out_bits;
	}
	if (process_page_range(inode, start, end,
			       PROCESS_TEST_LOCKED | PROCESS_UNLOCK)) {
		test_msg("Pages in range were not all locked\n");
		goto out_bits;
	}
	unlock_extent(&tmp, start, end);

	/*
	 * Now to test where we run into a page that is no longer dirty in the
	 * range we want to find.
	 */
	page = find_get_page(inode->i_mapping, (max_bytes + (1 * 1024 * 1024))
			     >> PAGE_CACHE_SHIFT);
	if (!page) {
		test_msg("Couldn't find our page\n");
		goto out_bits;
	}
	ClearPageDirty(page);
	page_cache_release(page);

	/* We unlocked it in the previous test */
	lock_page(locked_page);
	start = test_start;
	end = 0;
	/*
	 * Currently if we fail to find dirty pages in the delalloc range we
	 * will adjust max_bytes down to PAGE_CACHE_SIZE and then re-search.  If
	 * this changes at any point in the future we will need to fix this
	 * tests expected behavior.
	 */
	found = find_lock_delalloc_range(inode, &tmp, locked_page, &start,
					 &end, max_bytes);
	if (!found) {
		test_msg("Didn't find our range\n");
		goto out_bits;
	}
	if (start != test_start && end != test_start + PAGE_CACHE_SIZE - 1) {
		test_msg("Expected start %Lu end %Lu, got start %Lu end %Lu\n",
			 test_start, test_start + PAGE_CACHE_SIZE - 1, start,
			 end);
		goto out_bits;
	}
	if (process_page_range(inode, start, end, PROCESS_TEST_LOCKED |
			       PROCESS_UNLOCK)) {
		test_msg("Pages in range were not all locked\n");
		goto out_bits;
	}
	ret = 0;
out_bits:
	clear_extent_bits(&tmp, 0, total_dirty - 1, (unsigned)-1, GFP_NOFS);
out:
	if (locked_page)
		page_cache_release(locked_page);
	process_page_range(inode, 0, total_dirty - 1,
			   PROCESS_UNLOCK | PROCESS_RELEASE);
	iput(inode);
	return ret;
}

int btrfs_test_extent_io(void)
{
	test_msg("Running find delalloc tests\n");
	return test_find_delalloc();
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Copyright (C) 2013 Fusion IO.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/slab.h>
#include "btrfs-tests.h"
#include "../ctree.h"
#include "../disk-io.h"
#include "../free-space-cache.h"

#define BITS_PER_BITMAP		(PAGE_CACHE_SIZE * 8)
static struct btrfs_block_group_cache *init_test_block_group(void)
{
	struct btrfs_block_group_cache *cache;

	cache = kzalloc(sizeof(*cache), GFP_NOFS);
	if (!cache)
		return NULL;
	cache->free_space_ctl = kzalloc(sizeof(*cache->free_space_ctl),
					GFP_NOFS);
	if (!cache->free_space_ctl) {
		kfree(cache);
		return NULL;
	}
	cache->fs_info = btrfs_alloc_dummy_fs_info();
	if (!cache->fs_info) {
		kfree(cache->free_space_ctl);
		kfree(cache);
		return NULL;
	}

	cache->key.objectid = 0;
	cache->key.offset = 1024 * 1024 * 1024;
	cache->key.type = BTRFS_BLOCK_GROUP_ITEM_KEY;
	cache->sectorsize = 4096;
	cache->full_stripe_len = 4096;

	spin_lock_init(&cache->lock);
	INIT_LIST_HEAD(&cache->list);
	INIT_LIST_HEAD(&cache->cluster_list);
	INIT_LIST_HEAD(&cache->bg_list);

	btrfs_init_free_space_ctl(cache);

	return cache;
}

/*
 * This test just does basic sanity checking, making sure we can add an exten
 * entry and remove space from either end and the middle, and make sure we can
 * remove space that covers adjacent extent entries.
 */
static int test_extents(struct btrfs_block_group_cache *cache)
{
	int ret = 0;

	test_msg("Running extent only tests\n");

	/* First just make sure we can remove an entire entry */
	ret = btrfs_add_free_space(cache, 0, 4 * 1024 * 1024);
	if (ret) {
		test_msg("Error adding initial extents %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 0, 4 * 1024 * 1024);
	if (ret) {
		test_msg("Error removing extent %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, 0, 4 * 1024 * 1024)) {
		test_msg("Full remove left some lingering space\n");
		return -1;
	}

	/* Ok edge and middle cases now */
	ret = btrfs_add_free_space(cache, 0, 4 * 1024 * 1024);
	if (ret) {
		test_msg("Error adding half extent %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 3 * 1024 * 1024, 1 * 1024 * 1024);
	if (ret) {
		test_msg("Error removing tail end %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 0, 1 * 1024 * 1024);
	if (ret) {
		test_msg("Error removing front end %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 2 * 1024 * 1024, 4096);
	if (ret) {
		test_msg("Error removing middle piece %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, 0, 1 * 1024 * 1024)) {
		test_msg("Still have space at the front\n");
		return -1;
	}

	if (test_check_exists(cache, 2 * 1024 * 1024, 4096)) {
		test_msg("Still have space in the middle\n");
		return -1;
	}

	if (test_check_exists(cache, 3 * 1024 * 1024, 1 * 1024 * 1024)) {
		test_msg("Still have space at the end\n");
		return -1;
	}

	/* Cleanup */
	__btrfs_remove_free_space_cache(cache->free_space_ctl);

	return 0;
}

static int test_bitmaps(struct btrfs_block_group_cache *cache)
{
	u64 next_bitmap_offset;
	int ret;

	test_msg("Running bitmap only tests\n");

	ret = test_add_free_space_entry(cache, 0, 4 * 1024 * 1024, 1);
	if (ret) {
		test_msg("Couldn't create a bitmap entry %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 0, 4 * 1024 * 1024);
	if (ret) {
		test_msg("Error removing bitmap full range %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, 0, 4 * 1024 * 1024)) {
		test_msg("Left some space in bitmap\n");
		return -1;
	}

	ret = test_add_free_space_entry(cache, 0, 4 * 1024 * 1024, 1);
	if (ret) {
		test_msg("Couldn't add to our bitmap entry %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 1 * 1024 * 1024, 2 * 1024 * 1024);
	if (ret) {
		test_msg("Couldn't remove middle chunk %d\n", ret);
		return ret;
	}

	/*
	 * The first bitmap we have starts at offset 0 so the next one is just
	 * at the end of the first bitmap.
	 */
	next_bitmap_offset = (u64)(BITS_PER_BITMAP * 4096);

	/* Test a bit straddling two bitmaps */
	ret = test_add_free_space_entry(cache, next_bitmap_offset -
				   (2 * 1024 * 1024), 4 * 1024 * 1024, 1);
	if (ret) {
		test_msg("Couldn't add space that straddles two bitmaps %d\n",
				ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, next_bitmap_offset -
				      (1 * 1024 * 1024), 2 * 1024 * 1024);
	if (ret) {
		test_msg("Couldn't remove overlapping space %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, next_bitmap_offset - (1 * 1024 * 1024),
			 2 * 1024 * 1024)) {
		test_msg("Left some space when removing overlapping\n");
		return -1;
	}

	__btrfs_remove_free_space_cache(cache->free_space_ctl);

	return 0;
}

/* This is the high grade jackassery */
static int test_bitmaps_and_extents(struct btrfs_block_group_cache *cache)
{
	u64 bitmap_offset = (u64)(BITS_PER_BITMAP * 4096);
	int ret;

	test_msg("Running bitmap and extent tests\n");

	/*
	 * First let's do something simple, an extent at the same offset as the
	 * bitmap, but the free space completely in the extent and then
	 * completely in the bitmap.
	 */
	ret = test_add_free_space_entry(cache, 4 * 1024 * 1024, 1 * 1024 * 1024, 1);
	if (ret) {
		test_msg("Couldn't create bitmap entry %d\n", ret);
		return ret;
	}

	ret = test_add_free_space_entry(cache, 0, 1 * 1024 * 1024, 0);
	if (ret) {
		test_msg("Couldn't add extent entry %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 0, 1 * 1024 * 1024);
	if (ret) {
		test_msg("Couldn't remove extent entry %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, 0, 1 * 1024 * 1024)) {
		test_msg("Left remnants after our remove\n");
		return -1;
	}

	/* Now to add back the extent entry and remove from the bitmap */
	ret = test_add_free_space_entry(cache, 0, 1 * 1024 * 1024, 0);
	if (ret) {
		test_msg("Couldn't re-add extent entry %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 4 * 1024 * 1024, 1 * 1024 * 1024);
	if (ret) {
		test_msg("Couldn't remove from bitmap %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, 4 * 1024 * 1024, 1 * 1024 * 1024)) {
		test_msg("Left remnants in the bitmap\n");
		return -1;
	}

	/*
	 * Ok so a little more evil, extent entry and bitmap at the same offset,
	 * removing an overlapping chunk.
	 */
	ret = test_add_free_space_entry(cache, 1 * 1024 * 1024, 4 * 1024 * 1024, 1);
	if (ret) {
		test_msg("Couldn't add to a bitmap %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 512 * 1024, 3 * 1024 * 1024);
	if (ret) {
		test_msg("Couldn't remove overlapping space %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, 512 * 1024, 3 * 1024 * 1024)) {
		test_msg("Left over pieces after removing overlapping\n");
		return -1;
	}

	__btrfs_remove_free_space_cache(cache->free_space_ctl);

	/* Now with the extent entry offset into the bitmap */
	ret = test_add_free_space_entry(cache, 4 * 1024 * 1024, 4 * 1024 * 1024, 1);
	if (ret) {
		test_msg("Couldn't add space to the bitmap %d\n", ret);
		return ret;
	}

	ret = test_add_free_space_entry(cache, 2 * 1024 * 1024, 2 * 1024 * 1024, 0);
	if (ret) {
		test_msg("Couldn't add extent to the cache %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 3 * 1024 * 1024, 4 * 1024 * 1024);
	if (ret) {
		test_msg("Problem removing overlapping space %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, 3 * 1024 * 1024, 4 * 1024 * 1024)) {
		test_msg("Left something behind when removing space");
		return -1;
	}

	/*
	 * This has blown up in the past, the extent entry starts before the
	 * bitmap entry, but we're trying to remove an offset that falls
	 * completely within the bitmap range and is in both the extent entry
	 * and the bitmap entry, looks like this
	 *
	 *   [ extent ]
	 *      [ bitmap ]
	 *        [ del ]
	 */
	__btrfs_remove_free_space_cache(cache->free_space_ctl);
	ret = test_add_free_space_entry(cache, bitmap_offset + 4 * 1024 * 1024,
				   4 * 1024 * 1024, 1);
	if (ret) {
		test_msg("Couldn't add bitmap %d\n", ret);
		return ret;
	}

	ret = test_add_free_space_entry(cache, bitmap_offset - 1 * 1024 * 1024,
				   5 * 1024 * 1024, 0);
	if (ret) {
		test_msg("Couldn't add extent entry %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, bitmap_offset + 1 * 1024 * 1024,
				      5 * 1024 * 1024);
	if (ret) {
		test_msg("Failed to free our space %d\n", ret);
		return ret;
	}

	if (test_check_exists(cache, bitmap_offset + 1 * 1024 * 1024,
			 5 * 1024 * 1024)) {
		test_msg("Left stuff over\n");
		return -1;
	}

	__btrfs_remove_free_space_cache(cache->free_space_ctl);

	/*
	 * This blew up before, we have part of the free space in a bitmap and
	 * then the entirety of the rest of the space in an extent.  This used
	 * to return -EAGAIN back from btrfs_remove_extent, make sure this
	 * doesn't happen.
	 */
	ret = test_add_free_space_entry(cache, 1 * 1024 * 1024, 2 * 1024 * 1024, 1);
	if (ret) {
		test_msg("Couldn't add bitmap entry %d\n", ret);
		return ret;
	}

	ret = test_add_free_space_entry(cache, 3 * 1024 * 1024, 1 * 1024 * 1024, 0);
	if (ret) {
		test_msg("Couldn't add extent entry %d\n", ret);
		return ret;
	}

	ret = btrfs_remove_free_space(cache, 1 * 1024 * 1024, 3 * 1024 * 1024);
	if (ret) {
		test_msg("Error removing bitmap and extent overlapping %d\n", ret);
		return ret;
	}

	__btrfs_remove_free_space_cache(cache->free_space_ctl);
	return 0;
}

/* Used by test_steal_space_from_bitmap_to_extent(). */
static bool test_use_bitmap(struct btrfs_free_space_ctl *ctl,
			    struct btrfs_free