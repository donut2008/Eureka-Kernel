O(autoclear);
LOOP_ATTR_RO(partscan);
LOOP_ATTR_RO(dio);

static struct attribute *loop_attrs[] = {
	&loop_attr_backing_file.attr,
	&loop_attr_offset.attr,
	&loop_attr_sizelimit.attr,
	&loop_attr_autoclear.attr,
	&loop_attr_partscan.attr,
	&loop_attr_dio.attr,
	NULL,
};

static struct attribute_group loop_attribute_group = {
	.name = "loop",
	.attrs= loop_attrs,
};

static void loop_sysfs_init(struct loop_device *lo)
{
	lo->sysfs_inited = !sysfs_create_group(&disk_to_dev(lo->lo_disk)->kobj,
						&loop_attribute_group);
}

static void loop_sysfs_exit(struct loop_device *lo)
{
	if (lo->sysfs_inited)
		sysfs_remove_group(&disk_to_dev(lo->lo_disk)->kobj,
				   &loop_attribute_group);
}

static void loop_config_discard(struct loop_device *lo)
{
	struct file *file = lo->lo_backing_file;
	struct inode *inode = file->f_mapping->host;
	struct request_queue *q = lo->lo_queue;

	/*
	 * We use punch hole to reclaim the free space used by the
	 * image a.k.a. discard. However we do not support discard if
	 * encryption is enabled, because it may give an attacker
	 * useful information.
	 */
	if ((!file->f_op->fallocate) ||
	    lo->lo_encrypt_key_size) {
		q->limits.discard_granularity = 0;
		q->limits.discard_alignment = 0;
		blk_queue_max_discard_sectors(q, 0);
		q->limits.discard_zeroes_data = 0;
		queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, q);
		return;
	}

	q->limits.discard_granularity = inode->i_sb->s_blocksize;
	q->limits.discard_alignment = 0;
	blk_queue_max_discard_sectors(q, UINT_MAX >> 9);
	q->limits.discard_zeroes_data = 1;
	queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, q);
}

static void loop_unprepare_queue(struct loop_device *lo)
{
	kthread_flush_worker(&lo->worker);
	kthread_stop(lo->worker_task);
}

static int loop_prepare_queue(struct loop_device *lo)
{
	kthread_init_worker(&lo->worker);
	lo->worker_task = kthread_run(kthread_worker_fn,
			&lo->worker, "loop%d", lo->lo_number);
	if (IS_ERR(lo->worker_task))
		return -ENOMEM;
	set_user_nice(lo->worker_task, MIN_NICE);
	return 0;
}

static void loop_update_rotational(struct loop_device *lo)
{
	struct file *file = lo->lo_backing_file;
	struct inode *file_inode = file->f_mapping->host;
	struct block_device *file_bdev = file_inode->i_sb->s_bdev;
	struct request_queue *q = lo->lo_queue;
	bool nonrot = true;

	/* not all filesystems (e.g. tmpfs) have a sb->s_bdev */
	if (file_bdev)
		nonrot = blk_queue_nonrot(bdev_get_queue(file_bdev));

	if (nonrot)
		queue_flag_set_unlocked(QUEUE_FLAG_NONROT, q);
	else
		queue_flag_clear_unlocked(QUEUE_FLAG_NONROT, q);
}

static int loop_set_fd(struct loop_device *lo, fmode_t mode,
		       struct block_device *bdev, unsigned int arg)
{
	struct file	*file;
	struct inode	*inode;
	struct address_space *mapping;
	unsigned lo_blocksize;
	int		lo_flags = 0;
	int		error;
	loff_t		size;

	/* This is safe, since we have a reference from open(). */
	__module_get(THIS_MODULE);

	error = -EBADF;
	file = fget(arg);
	if (!file)
		goto out;

	error = -EBUSY;
	if (lo->lo_state != Lo_unbound)
		goto out_putf;

	error = loop_validate_file(file, bdev);
	if (error)
		goto out_putf;

	mapping = file->f_mapping;
	inode = mapping->host;

	if (!(file->f_mode & FMODE_WRITE) || !(mode & FMODE_WRITE) ||
	    !file->f_op->write_iter)
		lo_flags |= LO_FLAGS_READ_ONLY;

	lo_blocksize = S_ISBLK(inode->i_mode) ?
		inode->i_bdev->bd_block_size : PAGE_SIZE;

	error = -EFBIG;
	size = get_loop_size(lo, file);
	if ((loff_t)(sector_t)size != size)
		goto out_putf;
	error = loop_prepare_queue(lo);
	if (error)
		goto out_putf;

	error = 0;

	set_device_ro(bdev, (lo_flags & LO_FLAGS_READ_ONLY) != 0);

	lo->use_dio = false;
	lo->lo_blocksize = lo_blocksize;
	lo->lo_device = bdev;
	lo->lo_flags = lo_flags;
	lo->lo_backing_file = file;
	lo->transfer = NULL;
	lo->ioctl = NULL;
	lo->lo_sizelimit = 0;
	lo->old_gfp_mask = mapping_gfp_mask(mapping);
	mapping_set_gfp_mask(mapping, lo->old_gfp_mask & ~(__GFP_IO|__GFP_FS));

	if (!(lo_flags & LO_FLAGS_READ_ONLY) && file->f_op->fsync)
		blk_queue_flush(lo->lo_queue, REQ_FLUSH);

	loop_update_rotational(lo);
	loop_update_dio(lo);
	set_capacity(lo->lo_disk, size);
	bd_set_size(bdev, size << 9);
	loop_sysfs_init(lo);
	/* let user-space know about the new size */
	kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);

	set_blocksize(bdev, lo_blocksize);

	lo->lo_state = Lo_bound;
	if (part_shift)
		lo->lo_flags |= LO_FLAGS_PARTSCAN;
	if (lo->lo_flags & LO_FLAGS_PARTSCAN)
		loop_reread_partitions(lo, bdev);

	/* Grab the block_device to prevent its destruction after we
	 * put /dev/loopXX inode. Later in loop_clr_fd() we bdput(bdev).
	 */
	bdgrab(bdev);
	return 0;

 out_putf:
	fput(file);
 out:
	/* This is safe: open() is still holding a reference. */
	module_put(THIS_MODULE);
	return error;
}

static int
loop_release_xfer(struct loop_device *lo)
{
	int err = 0;
	struct loop_func_table *xfer = lo->lo_encryption;

	if (xfer) {
		if (xfer->release)
			err = xfer->release(lo);
		lo->transfer = NULL;
		lo->lo_encryption = NULL;
		module_put(xfer->owner);
	}
	return err;
}

static int
loop_init_xfer(struct loop_device *lo, struct loop_func_table *xfer,
	       const struct loop_info64 *i)
{
	int err = 0;

	if (xfer) {
		struct module *owner = xfer->owner;

		if (!try_module_get(owner))
			return -EINVAL;
		if (xfer->init)
			err = xfer->init(lo, i);
		if (err)
			module_put(owner);
		else
			lo->lo_encryption = xfer;
	}
	return err;
}

static int loop_clr_fd(struct loop_device *lo)
{
	struct file *filp = lo->lo_backing_file;
	gfp_t gfp = lo->old_gfp_mask;
	struct block_device *bdev = lo->lo_device;

	if (lo->lo_state != Lo_bound)
		return -ENXIO;

	/*
	 * If we've explicitly asked to tear down the loop device,
	 * and it has an elevated reference count, set it for auto-teardown when
	 * the last reference goes away. This stops $!~#$@ udev from
	 * preventing teardown because it decided that it needs to run blkid on
	 * the loopback device whenever they appear. xfstests is notorious for
	 * failing tests because blkid via udev races with a losetup
	 * <dev>/do something like mkfs/losetup -d <dev> causing the losetup -d
	 * command to fail with EBUSY.
	 */
	if (atomic_read(&lo->lo_refcnt) > 1) {
		lo->lo_flags |= LO_FLAGS_AUTOCLEAR;
		mutex_unlock(&lo->lo_ctl_mutex);
		return 0;
	}

	if (filp == NULL)
		return -EINVAL;

	/* freeze request queue during the transition */
	blk_mq_freeze_queue(lo->lo_queue);

	spin_lock_irq(&lo->lo_lock);
	lo->lo_state = Lo_rundown;
	lo->lo_backing_file = NULL;
	spin_unlock_irq(&lo->lo_lock);

	loop_release_xfer(lo);
	lo->transfer = NULL;
	lo->ioctl = NULL;
	lo->lo_device = NULL;
	lo->lo_encryption = NULL;
	lo->lo_offset = 0;
	lo->lo_sizelimit = 0;
	lo->lo_encrypt_key_size = 0;
	memset(lo->lo_encrypt_key, 0, LO_KEY_SIZE);
	memset(lo->lo_crypt_name, 0, LO_NAME_SIZE);
	memset(lo->lo_file_name, 0, LO_NAME_SIZE);
	blk_queue_logical_block_size(lo->lo_queue, 512);
	if (bdev) {
		bdput(bdev);
		invalidate_bdev(bdev);
	}
	set_capacity(lo->lo_disk, 0);
	loop_sysfs_exit(lo);
	if (bdev) {
		bd_set_size(bdev, 0);
		/* let user-space know about this change */
		kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);
	}
	mapping_set_gfp_mask(filp->f_mapping, gfp);
	lo->lo_state = Lo_unbound;
	/* This is safe: open() is still holding a reference. */
	module_put(THIS_MODULE);
	blk_mq_unfreeze_queue(lo->lo_queue);

	if (lo->lo_flags & LO_FLAGS_PARTSCAN && bdev)
		loop_reread_partitions(lo, bdev);
	lo->lo_flags = 0;
	if (!part_shift)
		lo->lo_disk->flags |= GENHD_FL_NO_PART_SCAN;
	loop_unprepare_queue(lo);
	mutex_unlock(&lo->lo_ctl_mutex);
	/*
	 * Need not hold lo_ctl_mutex to fput backing file.
	 * Calling fput holding lo_ctl_mutex triggers a circular
	 * lock dependency possibility warning as fput can take
	 * bd_mutex which is usually taken before lo_ctl_mutex.
	 */
	fput(filp);
	return 0;
}

static int
loop_set_status(struct loop_device *lo, const struct loop_info64 *info)
{
	int err;
	struct loop_func_table *xfer;
	kuid_t uid = current_uid();

	if (lo->lo_encrypt_key_size &&
	    !uid_eq(lo->lo_key_owner, uid) &&
	    !capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (lo->lo_state != Lo_bound)
		return -ENXIO;
	if ((unsigned int) info->lo_encrypt_key_size > LO_KEY_SIZE)
		return -EINVAL;

	if (lo->lo_offset != info->lo_offset ||
	    lo->lo_sizelimit != info->lo_sizelimit) {
		sync_blockdev(lo->lo_device);
		kill_bdev(lo->lo_device);
	}

	/* I/O need to be drained during transfer transition */
	blk_mq_freeze_queue(lo->lo_queue);

	err = loop_release_xfer(lo);
	if (err)
		goto exit;

	if (info->lo_encrypt_type) {
		unsigned int type = info->lo_encrypt_type;

		if (type >= MAX_LO_CRYPT) {
			err = -EINVAL;
			goto exit;
		}
		xfer = xfer_funcs[type];
		if (xfer == NULL) {
			err = -EINVAL;
			goto exit;
		}
	} else
		xfer = NULL;

	err = loop_init_xfer(lo, xfer, info);
	if (err)
		goto exit;

	if (lo->lo_offset != info->lo_offset ||
	    lo->lo_sizelimit != info->lo_sizelimit) {
		/* kill_bdev should have truncated all the pages */
		if (lo->lo_device->bd_inode->i_mapping->nrpages) {
			err = -EAGAIN;
			pr_warn("%s: loop%d (%s) has still dirty pages (nrpages=%lu)\n",
				__func__, lo->lo_number, lo->lo_file_name,
				lo->lo_device->bd_inode->i_mapping->nrpages);
			goto exit;
		}
		if (figure_loop_size(lo, info->lo_offset, info->lo_sizelimit)) {
			err = -EFBIG;
			goto exit;
		}
	}

	loop_config_discard(lo);

	memcpy(lo->lo_file_name, info->lo_file_name, LO_NAME_SIZE);
	memcpy(lo->lo_crypt_name, info->lo_crypt_name, LO_NAME_SIZE);
	lo->lo_file_name[LO_NAME_SIZE-1] = 0;
	lo->lo_crypt_name[LO_NAME_SIZE-1] = 0;

	if (!xfer)
		xfer = &none_funcs;
	lo->transfer = xfer->transfer;
	lo->ioctl = xfer->ioctl;

	if ((lo->lo_flags & LO_FLAGS_AUTOCLEAR) !=
	     (info->lo_flags & LO_FLAGS_AUTOCLEAR))
		lo->lo_flags ^= LO_FLAGS_AUTOCLEAR;

	lo->lo_encrypt_key_size = info->lo_encrypt_key_size;
	lo->lo_init[0] = info->lo_init[0];
	lo->lo_init[1] = info->lo_init[1];
	if (info->lo_encrypt_key_size) {
		memcpy(lo->lo_encrypt_key, info->lo_encrypt_key,
		       info->lo_encrypt_key_size);
		lo->lo_key_owner = uid;
	}

	/* update dio if lo_offset or transfer is changed */
	__loop_update_dio(lo, lo->use_dio);

 exit:
	blk_mq_unfreeze_queue(lo->lo_queue);

	if (!err && (info->lo_flags & LO_FLAGS_PARTSCAN) &&
	     !(lo->lo_flags & LO_FLAGS_PARTSCAN)) {
		lo->lo_flags |= LO_FLAGS_PARTSCAN;
		lo->lo_disk->flags &= ~GENHD_FL_NO_PART_SCAN;
		loop_reread_partitions(lo, lo->lo_device);
	}

	return err;
}

static int
loop_get_status(struct loop_device *lo, struct loop_info64 *info)
{
	struct file *file = lo->lo_backing_file;
	struct kstat stat;
	int error;

	if (lo->lo_state != Lo_bound)
		return -ENXIO;
	error = vfs_getattr(&file->f_path, &stat);
	if (error)
		return error;
	memset(info, 0, sizeof(*info));
	info->lo_number = lo->lo_number;
	info->lo_device = huge_encode_dev(stat.dev);
	info->lo_inode = stat.ino;
	info->lo_rdevice = huge_encode_dev(lo->lo_device ? stat.rdev : stat.dev);
	info->lo_offset = lo->lo_offset;
	info->lo_sizelimit = lo->lo_sizelimit;
	info->lo_flags = lo->lo_flags;
	memcpy(info->lo_file_name, lo->lo_file_name, LO_NAME_SIZE);
	memcpy(info->lo_crypt_name, lo->lo_crypt_name, LO_NAME_SIZE);
	info->lo_encrypt_type =
		lo->lo_encryption ? lo->lo_encryption->number : 0;
	if (lo->lo_encrypt_key_size && capable(CAP_SYS_ADMIN)) {
		info->lo_encrypt_key_size = lo->lo_encrypt_key_size;
		memcpy(info->lo_encrypt_key, lo->lo_encrypt_key,
		       lo->lo_encrypt_key_size);
	}
	return 0;
}

static void
loop_info64_from_old(const struct loop_info *info, struct loop_info64 *info64)
{
	memset(info64, 0, sizeof(*info64));
	info64->lo_number = info->lo_number;
	info64->lo_device = info->lo_device;
	info64->lo_inode = info->lo_inode;
	info64->lo_rdevice = info->lo_rdevice;
	info64->lo_offset = info->lo_offset;
	info64->lo_sizelimit = 0;
	info64->lo_encrypt_type = info->lo_encrypt_type;
	info64->lo_encrypt_key_size = info->lo_encrypt_key_size;
	info64->lo_flags = info->lo_flags;
	info64->lo_init[0] = info->lo_init[0];
	info64->lo_init[1] = info->lo_init[1];
	if (info->lo_encrypt_type == LO_CRYPT_CRYPTOAPI)
		memcpy(info64->lo_crypt_name, info->lo_name, LO_NAME_SIZE);
	else
		memcpy(info64->lo_file_name, info->lo_name, LO_NAME_SIZE);
	memcpy(info64->lo_encrypt_key, info->lo_encrypt_key, LO_KEY_SIZE);
}

static int
loop_info64_to_old(const struct loop_info64 *info64, struct loop_info *info)
{
	memset(info, 0, sizeof(*info));
	info->lo_number = info64->lo_number;
	info->lo_device = info64->lo_device;
	info->lo_inode = info64->lo_inode;
	info->lo_rdevice = info64->lo_rdevice;
	info->lo_offset = info64->lo_offset;
	info->lo_encrypt_type = info64->lo_encrypt_type;
	info->lo_encrypt_key_size = info64->lo_encrypt_key_size;
	info->lo_flags = info64->lo_flags;
	info->lo_init[0] = info64->lo_init[0];
	info->lo_init[1] = info64->lo_init[1];
	if (info->lo_encrypt_type == LO_CRYPT_CRYPTOAPI)
		memcpy(info->lo_name, info64->lo_crypt_name, LO_NAME_SIZE);
	else
		memcpy(info->lo_name, info64->lo_file_name, LO_NAME_SIZE);
	memcpy(info->lo_encrypt_key, info64->lo_encrypt_key, LO_KEY_SIZE);

	/* error in case values were truncated */
	if (info->lo_device != info64->lo_device ||
	    info->lo_rdevice != info64->lo_rdevice ||
	    info->lo_inode != info64->lo_inode ||
	    info->lo_offset != info64->lo_offset)
		return -EOVERFLOW;

	return 0;
}

static int
loop_set_status_old(struct loop_device *lo, const struct loop_info __user *arg)
{
	struct loop_info info;
	struct loop_info64 info64;

	if (copy_from_user(&info, arg, sizeof (struct loop_info)))
		return -EFAULT;
	loop_info64_from_old(&info, &info64);
	return loop_set_status(lo, &info64);
}

static int
loop_set_status64(struct loop_device *lo, const struct loop_info64 __user *arg)
{
	struct loop_info64 info64;

	if (copy_from_user(&info64, arg, sizeof (struct loop_info64)))
		return -EFAULT;
	return loop_set_status(lo, &info64);
}

static int
loop_get_status_old(struct loop_device *lo, struct loop_info __user *arg) {
	struct loop_info info;
	struct loop_info64 info64;
	int err = 0;

	if (!arg)
		err = -EINVAL;
	if (!err)
		err = loop_get_status(lo, &info64);
	if (!err)
		err = loop_info64_to_old(&info64, &info);
	if (!err && copy_to_user(arg, &info, sizeof(info)))
		err = -EFAULT;

	return err;
}

static int
loop_get_status64(struct loop_device *lo, struct loop_info64 __user *arg) {
	struct loop_info64 info64;
	int err = 0;

	if (!arg)
		err = -EINVAL;
	if (!err)
		err = loop_get_status(lo, &info64);
	if (!err && copy_to_user(arg, &info64, sizeof(info64)))
		err = -EFAULT;

	return err;
}

static int loop_set_capacity(struct loop_device *lo, struct block_device *bdev)
{
	if (unlikely(lo->lo_state != Lo_bound))
		return -ENXIO;

	return figure_loop_size(lo, lo->lo_offset, lo->lo_sizelimit);
}

static int loop_set_dio(struct loop_device *lo, unsigned long arg)
{
	int error = -ENXIO;
	if (lo->lo_state != Lo_bound)
		goto out;

	__loop_update_dio(lo, !!arg);
	if (lo->use_dio == !!arg)
		return 0;
	error = -EINVAL;
 out:
	return error;
}

static int loop_set_block_size(struct loop_device *lo, unsigned long arg)
{
	int err = 0;

	if (lo->lo_state != Lo_bound)
		return -ENXIO;

	if (arg < 512 || arg > PAGE_SIZE || !is_power_of_2(arg))
		return -EINVAL;

	if (lo->lo_queue->limits.logical_block_size != arg) {
		sync_blockdev(lo->lo_device);
		kill_bdev(lo->lo_device);
	}

	blk_mq_freeze_queue(lo->lo_queue);

	/* kill_bdev should have truncated all the pages */
	if (lo->lo_queue->limits.logical_block_size != arg &&
			lo->lo_device->bd_inode->i_mapping->nrpages) {
		err = -EAGAIN;
		pr_warn("%s: loop%d (%s) has still dirty pages (nrpages=%lu)\n",
			__func__, lo->lo_number, lo->lo_file_name,
			lo->lo_device->bd_inode->i_mapping->nrpages);
		goto out_unfreeze;
	}

	blk_queue_logical_block_size(lo->lo_queue, arg);
	loop_update_dio(lo);
out_unfreeze:
	blk_mq_unfreeze_queue(lo->lo_queue);

	return err;
}

static int lo_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	struct loop_device *lo = bdev->bd_disk->private_data;
	int err;

	mutex_lock_nested(&lo->lo_ctl_mutex, 1);
	switch (cmd) {
	case LOOP_SET_FD:
		err = loop_set_fd(lo, mode, bdev, arg);
		break;
	case LOOP_CHANGE_FD:
		err = loop_change_fd(lo, bdev, arg);
		break;
	case LOOP_CLR_FD:
		/* loop_clr_fd would have unlocked lo_ctl_mutex on success */
		err = loop_clr_fd(lo);
		if (!err)
			goto out_unlocked;
		break;
	case LOOP_SET_STATUS:
		err = -EPERM;
		if ((mode & FMODE_WRITE) || capable(CAP_SYS_ADMIN))
			err = loop_set_status_old(lo,
					(struct loop_info __user *)arg);
		break;
	case LOOP_GET_STATUS:
		err = loop_get_status_old(lo, (struct loop_info __user *) arg);
		break;
	case LOOP_SET_STATUS64:
		err = -EPERM;
		if ((mode & FMODE_WRITE) || capable(CAP_SYS_ADMIN))
			err = loop_set_status64(lo,
					(struct loop_info64 __user *) arg);
		break;
	case LOOP_GET_STATUS64:
		err = loop_get_status64(lo, (struct loop_info64 __user *) arg);
		break;
	case LOOP_SET_CAPACITY:
		err = -EPERM;
		if ((mode & FMODE_WRITE) || capable(CAP_SYS_ADMIN))
			err = loop_set_capacity(lo, bdev);
		break;
	case LOOP_SET_DIRECT_IO:
		err = -EPERM;
		if ((mode & FMODE_WRITE) || capable(CAP_SYS_ADMIN))
			err = loop_set_dio(lo, arg);
		break;
	case LOOP_SET_BLOCK_SIZE:
		err = -EPERM;
		if ((mode & FMODE_WRITE) || capable(CAP_SYS_ADMIN))
			err = loop_set_block_size(lo, arg);
		break;
	default:
		err = lo->ioctl ? lo->ioctl(lo, cmd, arg) : -EINVAL;
	}
	mutex_unlock(&lo->lo_ctl_mutex);

out_unlocked:
	return err;
}

#ifdef CONFIG_COMPAT
struct compat_loop_info {
	compat_int_t	lo_number;      /* ioctl r/o */
	compat_dev_t	lo_device;      /* ioctl r/o */
	compat_ulong_t	lo_inode;       /* ioctl r/o */
	compat_dev_t	lo_rdevice;     /* ioctl r/o */
	compat_int_t	lo_offset;
	compat_int_t	lo_encrypt_type;
	compat_int_t	lo_encrypt_key_size;    /* ioctl w/o */
	compat_int_t	lo_flags;       /* ioctl r/o */
	char		lo_name[LO_NAME_SIZE];
	unsigned char	lo_encrypt_key[LO_KEY_SIZE]; /* ioctl w/o */
	compat_ulong_t	lo_init[2];
	char		reserved[4];
};

/*
 * Transfer 32-bit compatibility structure in userspace to 64-bit loop info
 * - noinlined to reduce stack space usage in main part of driver
 */
static noinline int
loop_info64_from_compat(const struct compat_loop_info __user *arg,
			struct loop_info64 *info64)
{
	struct compat_loop_info info;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	memset(info64, 0, sizeof(*info64));
	info64->lo_number = info.lo_number;
	info64->lo_device = info.lo_device;
	info64->lo_inode = info.lo_inode;
	info64->lo_rdevice = info.lo_rdevice;
	info64->lo_offset = info.lo_offset;
	info64->lo_sizelimit = 0;
	info64->lo_encrypt_type = info.lo_encrypt_type;
	info64->lo_encrypt_key_size = info.lo_encrypt_key_size;
	info64->lo_flags = info.lo_flags;
	info64->lo_init[0] = info.lo_init[0];
	info64->lo_init[1] = info.lo_init[1];
	if (info.lo_encrypt_type == LO_CRYPT_CRYPTOAPI)
		memcpy(info64->lo_crypt_name, info.lo_name, LO_NAME_SIZE);
	else
		memcpy(info64->lo_file_name, info.lo_name, LO_NAME_SIZE);
	memcpy(info64->lo_encrypt_key, info.lo_encrypt_key, LO_KEY_SIZE);
	return 0;
}

/*
 * Transfer 64-bit loop info to 32-bit compatibility structure in userspace
 * - noinlined to reduce stack space usage in main part of driver
 */
static noinline int
loop_info64_to_compat(const struct loop_info64 *info64,
		      struct compat_loop_info __user *arg)
{
	struct compat_loop_info info;

	memset(&info, 0, sizeof(info));
	info.lo_number = info64->lo_number;
	info.lo_device = info64->lo_device;
	info.lo_inode = info64->lo_inode;
	info.lo_rdevice = info64->lo_rdevice;
	info.lo_offset = info64->lo_offset;
	info.lo_encrypt_type = info64->lo_encrypt_type;
	info.lo_encrypt_key_size = info64->lo_encrypt_key_size;
	info.lo_flags = info64->lo_flags;
	info.lo_init[0] = info64->lo_init[0];
	info.lo_init[1] = info64->lo_init[1];
	if (info.lo_encrypt_type == LO_CRYPT_CRYPTOAPI)
		memcpy(info.lo_name, info64->lo_crypt_name, LO_NAME_SIZE);
	else
		memcpy(info.lo_name, info64->lo_file_name, LO_NAME_SIZE);
	memcpy(info.lo_encrypt_key, info64->lo_encrypt_key, LO_KEY_SIZE);

	/* error in case values were truncated */
	if (info.lo_device != info64->lo_device ||
	    info.lo_rdevice != info64->lo_rdevice ||
	    info.lo_inode != info64->lo_inode ||
	    info.lo_offset != info64->lo_offset ||
	    info.lo_init[0] != info64->lo_init[0] ||
	    info.lo_init[1] != info64->lo_init[1])
		return -EOVERFLOW;

	if (copy_to_user(arg, &info, sizeof(info)))
		return -EFAULT;
	return 0;
}

static int
loop_set_status_compat(struct loop_device *lo,
		       const struct compat_loop_info __user *arg)
{
	struct loop_info64 info64;
	int ret;

	ret = loop_info64_from_compat(arg, &info64);
	if (ret < 0)
		return ret;
	return loop_set_status(lo, &info64);
}

static int
loop_get_status_compat(struct loop_device *lo,
		       struct compat_loop_info __user *arg)
{
	struct loop_info64 info64;
	int err = 0;

	if (!arg)
		err = -EINVAL;
	if (!err)
		err = loop_get_status(lo, &info64);
	if (!err)
		err = loop_info64_to_compat(&info64, arg);
	return err;
}

static int lo_compat_ioctl(struct block_device *bdev, fmode_t mode,
			   unsigned int cmd, unsigned long arg)
{
	struct loop_device *lo = bdev->bd_disk->private_data;
	int err;

	switch(cmd) {
	case LOOP_SET_STATUS:
		mutex_lock(&lo->lo_ctl_mutex);
		err = loop_set_status_compat(
			lo, (const struct compat_loop_info __user *) arg);
		mutex_unlock(&lo->lo_ctl_mutex);
		break;
	case LOOP_GET_STATUS:
		mutex_lock(&lo->lo_ctl_mutex);
		err = loop_get_status_compat(
			lo, (struct compat_loop_info __user *) arg);
		mutex_unlock(&lo->lo_ctl_mutex);
		break;
	case LOOP_SET_CAPACITY:
	case LOOP_CLR_FD:
	case LOOP_GET_STATUS64:
	case LOOP_SET_STATUS64:
		arg = (unsigned long) compat_ptr(arg);
	case LOOP_SET_FD:
	case LOOP_CHANGE_FD:
	case LOOP_SET_BLOCK_SIZE:
	case LOOP_SET_DIRECT_IO:
		err = lo_ioctl(bdev, mode, cmd, arg);
		break;
	default:
		err = -ENOIOCTLCMD;
		break;
	}
	return err;
}
#endif

static int lo_open(struct block_device *bdev, fmode_t mode)
{
	struct loop_device *lo;
	int err = 0;

	mutex_lock(&loop_index_mutex);
	lo = bdev->bd_disk->private_data;
	if (!lo) {
		err = -ENXIO;
		goto out;
	}

	atomic_inc(&lo->lo_refcnt);
out:
	mutex_unlock(&loop_index_mutex);
	return err;
}

static void __lo_release(struct loop_device *lo)
{
	int err;

	if (atomic_dec_return(&lo->lo_refcnt))
		return;

	mutex_lock(&lo->lo_ctl_mutex);
	if (lo->lo_flags & LO_FLAGS_AUTOCLEAR) {
		/*
		 * In autoclear mode, stop the loop thread
		 * and remove configuration after last close.
		 */
		err = loop_clr_fd(lo);
		if (!err)
			return;
	} else {
		/*
		 * Otherwise keep thread (if running) and config,
		 * but flush possible ongoing bios in thread.
		 */
		loop_flush(lo);
	}

	mutex_unlock(&lo->lo_ctl_mutex);
}

static void lo_release(struct gendisk *disk, fmode_t mode)
{
	mutex_lock(&loop_index_mutex);
	__lo_release(disk->private_data);
	mutex_unlock(&loop_index_mutex);
}

static const struct block_device_operations lo_fops = {
	.owner =	THIS_MODULE,
	.open =		lo_open,
	.release =	lo_release,
	.ioctl =	lo_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =	lo_compat_ioctl,
#endif
};

/*
 * And now the modules code and kernel interface.
 */
static int max_loop;
module_param(max_loop, int, S_IRUGO);
MODULE_PARM_DESC(max_loop, "Maximum number of loop devices");
module_param(max_part, int, S_IRUGO);
MODULE_PARM_DESC(max_part, "Maximum number of partitions per loop device");
MODULE_LICENSE("GPL");
MODULE_ALIAS_BLOCKDEV_MAJOR(LOOP_MAJOR);

int loop_register_transfer(struct loop_func_table *funcs)
{
	unsigned int n = funcs->number;

	if (n >= MAX_LO_CRYPT || xfer_funcs[n])
		return -EINVAL;
	xfer_funcs[n] = funcs;
	return 0;
}

static int unregister_transfer_cb(int id, void *ptr, void *data)
{
	struct loop_device *lo = ptr;
	struct loop_func_table *xfer = data;

	mutex_lock(&lo->lo_ctl_mutex);
	if (lo->lo_encryption == xfer)
		loop_release_xfer(lo);
	mutex_unlock(&lo->lo_ctl_mutex);
	return 0;
}

int loop_unregister_transfer(int number)
{
	unsigned int n = number;
	struct loop_func_table *xfer;

	if (n == 0 || n >= MAX_LO_CRYPT || (xfer = xfer_funcs[n]) == NULL)
		return -EINVAL;

	xfer_funcs[n] = NULL;
	idr_for_each(&loop_index_idr, &unregister_transfer_cb, xfer);
	return 0;
}

EXPORT_SYMBOL(loop_register_transfer);
EXPORT_SYMBOL(loop_unregister_transfer);

static int loop_queue_rq(struct blk_mq_hw_ctx *hctx,
		const struct blk_mq_queue_data *bd)
{
	struct loop_cmd *cmd = blk_mq_rq_to_pdu(bd->rq);
	struct loop_device *lo = cmd->rq->q->queuedata;

	blk_mq_start_request(bd->rq);

	if (lo->lo_state != Lo_bound)
		return BLK_MQ_RQ_QUEUE_ERROR;

	if (lo->use_dio && !(cmd->rq->cmd_flags & (REQ_FLUSH |
					REQ_DISCARD)))
		cmd->use_aio = true;
	else
		cmd->use_aio = false;

	kthread_queue_work(&lo->worker, &cmd->work);

	return BLK_MQ_RQ_QUEUE_OK;
}

static void loop_handle_cmd(struct loop_cmd *cmd)
{
	const bool write = cmd->rq->cmd_flags & REQ_WRITE;
	struct loop_device *lo = cmd->rq->q->queuedata;
	int ret = 0;

	if (write && (lo->lo_flags & LO_FLAGS_READ_ONLY)) {
		ret = -EIO;
		goto failed;
	}

	ret = do_req_filebacked(lo, cmd->rq);
 failed:
	/* complete non-aio request */
	if (!cmd->use_aio || ret)
		blk_mq_complete_request(cmd->rq, ret ? -EIO : 0);
}

static void loop_queue_work(struct kthread_work *work)
{
	struct loop_cmd *cmd =
		container_of(work, struct loop_cmd, work);

	loop_handle_cmd(cmd);
}

static int loop_init_request(void *data, struct request *rq,
		unsigned int hctx_idx, unsigned int request_idx,
		unsigned int numa_node)
{
	struct loop_cmd *cmd = blk_mq_rq_to_pdu(rq);

	cmd->rq = rq;
	kthread_init_work(&cmd->work, loop_queue_work);

	return 0;
}

static struct blk_mq_ops loop_mq_ops = {
	.queue_rq       = loop_queue_rq,
	.map_queue      = blk_mq_map_queue,
	.init_request	= loop_init_request,
};

static int loop_add(struct loop_device **l, int i)
{
	struct loop_device *lo;
	struct gendisk *disk;
	int err;

	err = -ENOMEM;
	lo = kzalloc(sizeof(*lo), GFP_KERNEL);
	if (!lo)
		goto out;

	lo->lo_state = Lo_unbound;

	/* allocate id, if @id >= 0, we're requesting that specific id */
	if (i >= 0) {
		err = idr_alloc(&loop_index_idr, lo, i, i + 1, GFP_KERNEL);
		if (err == -ENOSPC)
			err = -EEXIST;
	} else {
		err = idr_alloc(&loop_index_idr, lo, 0, 0, GFP_KERNEL);
	}
	if (err < 0)
		goto out_free_dev;
	i = err;

	err = -ENOMEM;
	lo->tag_set.ops = &loop_mq_ops;
	lo->tag_set.nr_hw_queues = 1;
	lo->tag_set.queue_depth = 128;
	lo->tag_set.numa_node = NUMA_NO_NODE;
	lo->tag_set.cmd_size = sizeof(struct loop_cmd);
	lo->tag_set.flags = BLK_MQ_F_SHOULD_MERGE | BLK_MQ_F_SG_MERGE;
	lo->tag_set.driver_data = lo;

	err = blk_mq_alloc_tag_set(&lo->tag_set);
	if (err)
		goto out_free_idr;

	lo->lo_queue = blk_mq_init_queue(&lo->tag_set);
	if (IS_ERR_OR_NULL(lo->lo_queue)) {
		err = PTR_ERR(lo->lo_queue);
		goto out_cleanup_tags;
	}
	lo->lo_queue->queuedata = lo;

	blk_queue_max_hw_sectors(lo->lo_queue, BLK_DEF_MAX_SECTORS);
	/*
	 * It doesn't make sense to enable merge because the I/O
	 * submitted to backing file is handled page by page.
	 */
	queue_flag_set_unlocked(QUEUE_FLAG_NOMERGES, lo->lo_queue);

	disk = lo->lo_disk = alloc_disk(1 << part_shift);
	if (!disk)
		goto out_free_queue;

	/*
	 * Disable partition scanning by default. The in-kernel partition
	 * scanning can be requested individually per-device during its
	 * setup. Userspace can always add and remove partitions from all
	 * devices. The needed partition minors are allocated from the
	 * extended minor space, the main loop device numbers will continue
	 * to match the loop minors, regardless of the number of partitions
	 * used.
	 *
	 * If max_part is given, partition scanning is globally enabled for
	 * all loop devices. The minors for the main loop devices will be
	 * multiples of max_part.
	 *
	 * Note: Global-for-all-devices, set-only-at-init, read-only module
	 * parameteters like 'max_loop' and 'max_part' make things needlessly
	 * complicated, are too static, inflexible and may surprise
	 * userspace tools. Parameters like this in general should be avoided.
	 */
	if (!part_shift)
		disk->flags |= GENHD_FL_NO_PART_SCAN;
	disk->flags |= GENHD_FL_EXT_DEVT;
	mutex_init(&lo->lo_ctl_mutex);
	atomic_set(&lo->lo_refcnt, 0);
	lo->lo_number		= i;
	spin_lock_init(&lo->lo_lock);
	disk->major		= LOOP_MAJOR;
	disk->first_minor	= i << part_shift;
	disk->fops		= &lo_fops;
	disk->private_data	= lo;
	disk->queue		= lo->lo_queue;
	sprintf(disk->disk_name, "loop%d", i);
	add_disk(disk);
	*l = lo;
	return lo->lo_number;

out_free_queue:
	blk_cleanup_queue(lo->lo_queue);
out_cleanup_tags:
	blk_mq_free_tag_set(&lo->tag_set);
out_free_idr:
	idr_remove(&loop_index_idr, i);
out_free_dev:
	kfree(lo);
out:
	return err;
}

static void loop_remove(struct loop_device *lo)
{
	blk_cleanup_queue(lo->lo_queue);
	del_gendisk(lo->lo_disk);
	blk_mq_free_tag_set(&lo->tag_set);
	put_disk(lo->lo_disk);
	kfree(lo);
}

static int find_free_cb(int id, void *ptr, void *data)
{
	struct loop_device *lo = ptr;
	struct loop_device **l = data;

	if (lo->lo_state == Lo_unbound) {
		*l = lo;
		return 1;
	}
	return 0;
}

static int loop_lookup(struct loop_device **l, int i)
{
	struct loop_device *lo;
	int ret = -ENODEV;

	if (i < 0) {
		int err;

		err = idr_for_each(&loop_index_idr, &find_free_cb, &lo);
		if (err == 1) {
			*l = lo;
			ret = lo->lo_number;
		}
		goto out;
	}

	/* lookup and return a specific i */
	lo = idr_find(&loop_index_idr, i);
	if (lo) {
		*l = lo;
		ret = lo->lo_number;
	}
out:
	return ret;
}

static struct kobject *loop_probe(dev_t dev, int *part, void *data)
{
	struct loop_device *lo;
	struct kobject *kobj;
	int err;

	mutex_lock(&loop_index_mutex);
	err = loop_lookup(&lo, MINOR(dev) >> part_shift);
	if (err < 0)
		err = loop_add(&lo, MINOR(dev) >> part_shift);
	if (err < 0)
		kobj = NULL;
	else
		kobj = get_disk(lo->lo_disk);
	mutex_unlock(&loop_index_mutex);

	*part = 0;
	return kobj;
}

static long loop_control_ioctl(struct file *file, unsigned int cmd,
			       unsigned long parm)
{
	struct loop_device *lo;
	int ret = -ENOSYS;

	mutex_lock(&loop_index_mutex);
	switch (cmd) {
	case LOOP_CTL_ADD:
		ret = loop_lookup(&lo, parm);
		if (ret >= 0) {
			ret = -EEXIST;
			break;
		}
		ret = loop_add(&lo, parm);
		break;
	case LOOP_CTL_REMOVE:
		ret = loop_lookup(&lo, parm);
		if (ret < 0)
			break;
		mutex_lock(&lo->lo_ctl_mutex);
		if (lo->lo_state != Lo_unbound) {
			ret = -EBUSY;
			mutex_unlock(&lo->lo_ctl_mutex);
			break;
		}
		if (atomic_read(&lo->lo_refcnt) > 0) {
			ret = -EBUSY;
			mutex_unlock(&lo->lo_ctl_mutex);
			break;
		}
		lo->lo_disk->private_data = NULL;
		mutex_unlock(&lo->lo_ctl_mutex);
		idr_remove(&loop_index_idr, lo->lo_number);
		loop_remove(lo);
		break;
	case LOOP_CTL_GET_FREE:
		ret = loop_lookup(&lo, -1);
		if (ret >= 0)
			break;
		ret = loop_add(&lo, -1);
	}
	mutex_unlock(&loop_index_mutex);

	return ret;
}

static const struct file_operations loop_ctl_fops = {
	.open		= nonseekable_open,
	.unlocked_ioctl	= loop_control_ioctl,
	.compat_ioctl	= loop_control_ioctl,
	.owner		= THIS_MODULE,
	.llseek		= noop_llseek,
};

static struct miscdevice loop_misc = {
	.minor		= LOOP_CTRL_MINOR,
	.name		= "loop-control",
	.fops		= &loop_ctl_fops,
};

MODULE_ALIAS_MISCDEV(LOOP_CTRL_MINOR);
MODULE_ALIAS("devname:loop-control");

static int __init loop_init(void)
{
	int i, nr;
	unsigned long range;
	struct loop_device *lo;
	int err;

	err = misc_register(&loop_misc);
	if (err < 0)
		return err;

	part_shift = 0;
	if (max_part > 0) {
		part_shift = fls(max_part);

		/*
		 * Adjust max_part according to part_shift as it is exported
		 * to user space so that user can decide correct minor number
		 * if [s]he want to create more devices.
		 *
		 * Note that -1 is required because partition 0 is reserved
		 * for the whole disk.
		 */
		max_part = (1UL << part_shift) - 1;
	}

	if ((1UL << part_shift) > DISK_MAX_PARTS) {
		err = -EINVAL;
		goto misc_out;
	}

	if (max_loop > 1UL << (MINORBITS - part_shift)) {
		err = -EINVAL;
		goto misc_out;
	}

	/*
	 * If max_loop is specified, create that many devices upfront.
	 * This also becomes a hard limit. If max_loop is not specified,
	 * create CONFIG_BLK_DEV_LOOP_MIN_COUNT loop devices at module
	 * init time. Loop devices can be requested on-demand with the
	 * /dev/loop-control interface, or be instantiated by accessing
	 * a 'dead' device node.
	 */
	if (max_loop) {
		nr = max_loop;
		range = max_loop << part_shift;
	} else {
		nr = CONFIG_BLK_DEV_LOOP_MIN_COUNT;
		range = 1UL << MINORBITS;
	}

	if (register_blkdev(LOOP_MAJOR, "loop")) {
		err = -EIO;
		goto misc_out;
	}

	blk_register_region(MKDEV(LOOP_MAJOR, 0), range,
				  THIS_MODULE, loop_probe, NULL, NULL);

	/* pre-create number of devices given by config or max_loop */
	mutex_lock(&loop_index_mutex);
	for (i = 0; i < nr; i++)
		loop_add(&lo, i);
	mutex_unlock(&loop_index_mutex);

	printk(KERN_INFO "loop: module loaded\n");
	return 0;

misc_out:
	misc_deregister(&loop_misc);
	return err;
}

static int loop_exit_cb(int id, void *ptr, void *data)
{
	struct loop_device *lo = ptr;

	loop_remove(lo);
	return 0;
}

static void __exit loop_exit(void)
{
	unsigned long range;

	range = max_loop ? max_loop << part_shift : 1UL << MINORBITS;

	idr_for_each(&loop_index_idr, &loop_exit_cb, NULL);
	idr_destroy(&loop_index_idr);

	blk_unregister_region(MKDEV(LOOP_MAJOR, 0), range);
	unregister_blkdev(LOOP_MAJOR, "loop");

	misc_deregister(&loop_misc);
}

module_init(loop_init);
module_exit(loop_exit);

#ifndef MODULE
static int __init max_loop_setup(char *str)
{
	max_loop = simple_strtol(str, NULL, 0);
	return 1;
}

__setup("max_loop=", max_loop_setup);
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * loop.h
 *
 * Written by Theodore Ts'o, 3/29/93.
 *
 * Copyright 1993 by Theodore Ts'o.  Redistribution of this file is
 * permitted under the GNU General Public License.
 */
#ifndef _LINUX_LOOP_H
#define _LINUX_LOOP_H

#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <uapi/linux/loop.h>

/* Possible states of device */
enum {
	Lo_unbound,
	Lo_bound,
	Lo_rundown,
};

struct loop_func_table;

struct loop_device {
	int		lo_number;
	atomic_t	lo_refcnt;
	loff_t		lo_offset;
	loff_t		lo_sizelimit;
	int		lo_flags;
	int		(*transfer)(struct loop_device *, int cmd,
				    struct page *raw_page, unsigned raw_off,
				    struct page *loop_page, unsigned loop_off,
				    int size, sector_t real_block);
	char		lo_file_name[LO_NAME_SIZE];
	char		lo_crypt_name[LO_NAME_SIZE];
	char		lo_encrypt_key[LO_KEY_SIZE];
	int		lo_encrypt_key_size;
	struct loop_func_table *lo_encryption;
	__u32           lo_init[2];
	kuid_t		lo_key_owner;	/* Who set the key */
	int		(*ioctl)(struct loop_device *, int cmd, 
				 unsigned long arg); 

	struct file *	lo_backing_file;
	struct block_device *lo_device;
	unsigned	lo_blocksize;
	void		*key_data; 

	gfp_t		old_gfp_mask;

	spinlock_t		lo_lock;
	int			lo_state;
	struct mutex		lo_ctl_mutex;
	struct kthread_worker	worker;
	struct task_struct	*worker_task;
	bool			use_dio;
	bool			sysfs_inited;

	struct request_queue	*lo_queue;
	struct blk_mq_tag_set	tag_set;
	struct gendisk		*lo_disk;
};

struct loop_cmd {
	struct kthread_work work;
	struct request *rq;
	struct list_head list;
	bool use_aio;           /* use AIO interface to handle I/O */
	struct kiocb iocb;
};

/* Support for loadable transfer modules */
struct loop_func_table {
	int number;	/* filter type */ 
	int (*transfer)(struct loop_device *lo, int cmd,
			struct page *raw_page, unsigned raw_off,
			struct page *loop_page, unsigned loop_off,
			int size, sector_t real_block);
	int (*init)(struct loop_device *, const struct loop_info64 *); 
	/* release is called from loop_unregister_transfer or clr_fd */
	int (*release)(struct loop_device *); 
	int (*ioctl)(struct loop_device *, int cmd, unsigned long arg);
	struct module *owner;
}; 

int loop_register_transfer(struct loop_func_table *funcs);
int loop_unregister_transfer(int number); 

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 *  drivers/block/mg_disk.c
 *
 *  Support for the mGine m[g]flash IO mode.
 *  Based on legacy hd.c
 *
 * (c) 2008 mGine Co.,LTD
 * (c) 2008 unsik Kim <donari75@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/ata.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/mg_disk.h>
#include <linux/slab.h>

#define MG_RES_SEC (CONFIG_MG_DISK_RES << 1)

/* name for block device */
#define MG_DISK_NAME "mgd"

#define MG_DISK_MAJ 0
#define MG_DISK_MAX_PART 16
#define MG_SECTOR_SIZE 512
#define MG_MAX_SECTS 256

/* Register offsets */
#define MG_BUFF_OFFSET			0x8000
#define MG_REG_OFFSET			0xC000
#define MG_REG_FEATURE			(MG_REG_OFFSET + 2)	/* write case */
#define MG_REG_ERROR			(MG_REG_OFFSET + 2)	/* read case */
#define MG_REG_SECT_CNT			(MG_REG_OFFSET + 4)
#define MG_REG_SECT_NUM			(MG_REG_OFFSET + 6)
#define MG_REG_CYL_LOW			(MG_REG_OFFSET + 8)
#define MG_REG_CYL_HIGH			(MG_REG_OFFSET + 0xA)
#define MG_REG_DRV_HEAD			(MG_REG_OFFSET + 0xC)
#define MG_REG_COMMAND			(MG_REG_OFFSET + 0xE)	/* write case */
#define MG_REG_STATUS			(MG_REG_OFFSET + 0xE)	/* read  case */
#define MG_REG_DRV_CTRL			(MG_REG_OFFSET + 0x10)
#define MG_REG_BURST_CTRL		(MG_REG_OFFSET + 0x12)

/* handy status */
#define MG_STAT_READY	(ATA_DRDY | ATA_DSC)
#define MG_READY_OK(s)	(((s) & (MG_STAT_READY | (ATA_BUSY | ATA_DF | \
				 ATA_ERR))) == MG_STAT_READY)

/* error code for others */
#define MG_ERR_NONE		0
#define MG_ERR_TIMEOUT		0x100
#define MG_ERR_INIT_STAT	0x101
#define MG_ERR_TRANSLATION	0x102
#define MG_ERR_CTRL_RST		0x103
#define MG_ERR_INV_STAT		0x104
#define MG_ERR_RSTOUT		0x105

#define MG_MAX_ERRORS	6	/* Max read/write errors */

/* command */
#define MG_CMD_RD 0x20
#define MG_CMD_WR 0x30
#define MG_CMD_SLEEP 0x99
#define MG_CMD_WAKEUP 0xC3
#define MG_CMD_ID 0xEC
#define MG_CMD_WR_CONF 0x3C
#define MG_CMD_RD_CONF 0x40

/* operation mode */
#define MG_OP_CASCADE (1 << 0)
#define MG_OP_CASCADE_SYNC_RD (1 << 1)
#define MG_OP_CASCADE_SYNC_WR (1 << 2)
#define MG_OP_INTERLEAVE (1 << 3)

/* synchronous */
#define MG_BURST_LAT_4 (3 << 4)
#define MG_BURST_LAT_5 (4 << 4)
#define MG_BURST_LAT_6 (5 << 4)
#define MG_BURST_LAT_7 (6 << 4)
#define MG_BURST_LAT_8 (7 << 4)
#define MG_BURST_LEN_4 (1 << 1)
#define MG_BURST_LEN_8 (2 << 1)
#define MG_BURST_LEN_16 (3 << 1)
#define MG_BURST_LEN_32 (4 << 1)
#define MG_BURST_LEN_CONT (0 << 1)

/* timeout value (unit: ms) */
#define MG_TMAX_CONF_TO_CMD	1
#define MG_TMAX_WAIT_RD_DRQ	10
#define MG_TMAX_WAIT_WR_DRQ	500
#define MG_TMAX_RST_TO_BUSY	10
#define MG_TMAX_HDRST_TO_RDY	500
#define MG_TMAX_SWRST_TO_RDY	500
#define MG_TMAX_RSTOUT		3000

#define MG_DEV_MASK (MG_BOOT_DEV | MG_STORAGE_DEV | MG_STORAGE_DEV_SKIP_RST)

/* main structure for mflash driver */
struct mg_host {
	struct device *dev;

	struct request_queue *breq;
	struct request *req;
	spinlock_t lock;
	struct gendisk *gd;

	struct timer_list timer;
	void (*mg_do_intr) (struct mg_host *);

	u16 id[ATA_ID_WORDS];

	u16 cyls;
	u16 heads;
	u16 sectors;
	u32 n_sectors;
	u32 nres_sectors;

	void __iomem *dev_base;
	unsigned int irq;
	unsigned int rst;
	unsigned int rstout;

	u32 major;
	u32 error;
};

/*
 * Debugging macro and defines
 */
#undef DO_MG_DEBUG
#ifdef DO_MG_DEBUG
#  define MG_DBG(fmt, args...) \
	printk(KERN_DEBUG "%s:%d "fmt, __func__, __LINE__, ##args)
#else /* CONFIG_MG_DEBUG */
#  define MG_DBG(fmt, args...) do { } while (0)
#endif /* CONFIG_MG_DEBUG */

static void mg_request(struct request_queue *);

static bool mg_end_request(struct mg_host *host, int err, unsigned int nr_bytes)
{
	if (__blk_end_request(host->req, err, nr_bytes))
		return true;

	host->req = NULL;
	return false;
}

static bool mg_end_request_cur(struct mg_host *host, int err)
{
	return mg_end_request(host, err, blk_rq_cur_bytes(host->req));
}

static void mg_dump_status(const char *msg, unsigned int stat,
		struct mg_host *host)
{
	char *name = MG_DISK_NAME;

	if (host->req)
		name = host->req->rq_disk->disk_name;

	printk(KERN_ERR "%s: %s: status=0x%02x { ", name, msg, stat & 0xff);
	if (stat & ATA_BUSY)
		printk("Busy ");
	if (stat & ATA_DRDY)
		printk("DriveReady ");
	if (stat & ATA_DF)
		printk("WriteFault ");
	if (stat & ATA_DSC)
		printk("SeekComplete ");
	if (stat & ATA_DRQ)
		printk("DataRequest ");
	if (stat & ATA_CORR)
		printk("CorrectedError ");
	if (stat & ATA_ERR)
		printk("Error ");
	printk("}\n");
	if ((stat & ATA_ERR) == 0) {
		host->error = 0;
	} else {
		host->error = inb((unsigned long)host->dev_base + MG_REG_ERROR);
		printk(KERN_ERR "%s: %s: error=0x%02x { ", name, msg,
				host->error & 0xff);
		if (host->error & ATA_BBK)
			printk("BadSector ");
		if (host->error & ATA_UNC)
			printk("UncorrectableError ");
		if (host->error & ATA_IDNF)
			printk("SectorIdNotFound ");
		if (host->error & ATA_ABORTED)
			printk("DriveStatusError ");
		if (host->error & ATA_AMNF)
			printk("AddrMarkNotFound ");
		printk("}");
		if (host->error & (ATA_BBK | ATA_UNC | ATA_IDNF | ATA_AMNF)) {
			if (host->req)
				printk(", sector=%u",
				       (unsigned int)blk_rq_pos(host->req));
		}
		printk("\n");
	}
}

static unsigned int mg_wait(struct mg_host *host, u32 expect, u32 msec)
{
	u8 status;
	unsigned long expire, cur_jiffies;
	struct mg_drv_data *prv_data = host->dev->platform_data;

	host->error = MG_ERR_NONE;
	expire = jiffies + msecs_to_jiffies(msec);

	/* These 2 times dummy status read prevents reading invalid
	 * status. A very little time (3 times of mflash operating clk)
	 * is required for busy bit is set. Use dummy read instead of
	 * busy wait, because mflash's PLL is machine dependent.
	 */
	if (prv_data->use_polling) {
		status = inb((unsigned long)host->dev_base + MG_REG_STATUS);
		status = inb((unsigned long)host->dev_base + MG_REG_STATUS);
	}

	status = inb((unsigned long)host->dev_base + MG_REG_STATUS);

	do {
		cur_jiffies = jiffies;
		if (status & ATA_BUSY) {
			if (expect == ATA_BUSY)
				break;
		} else {
			/* Check the error condition! */
			if (status & ATA_ERR) {
				mg_dump_status("mg_wait", status, host);
				break;
			}

			if (expect == MG_STAT_READY)
				if (MG_READY_OK(status))
					break;

			if (expect == ATA_DRQ)
				if (status & ATA_DRQ)
					break;
		}
		if (!msec) {
			mg_dump_status("not ready", status, host);
			return MG_ERR_INV_STAT;
		}

		status = inb((unsigned long)host->dev_base + MG_REG_STATUS);
	} while (time_before(cur_jiffies, expire));

	if (time_after_eq(cur_jiffies, expire) && msec)
		host->error = MG_ERR_TIMEOUT;

	return host->error;
}

static unsigned int mg_wait_rstout(u32 rstout, u32 msec)
{
	unsigned long expire;

	expire = jiffies + msecs_to_jiffies(msec);
	while (time_before(jiffies, expire)) {
		if (gpio_get_value(rstout) == 1)
			return MG_ERR_NONE;
		msleep(10);
	}

	return MG_ERR_RSTOUT;
}

static void mg_unexpected_intr(struct mg_host *host)
{
	u32 status = inb((unsigned long)host->dev_base + MG_REG_STATUS);

	mg_dump_status("mg_unexpected_intr", status, host);
}

static irqreturn_t mg_irq(int irq, void *dev_id)
{
	struct mg_host *host = dev_id;
	void (*handler)(struct mg_host *) = host->mg_do_intr;

	spin_lock(&host->lock);

	host->mg_do_intr = NULL;
	del_timer(&host->timer);
	if (!handler)
		handler = mg_unexpected_intr;
	handler(host);

	spin_unlock(&host->lock);

	return IRQ_HANDLED;
}

/* local copy of ata_id_string() */
static void mg_id_string(const u16 *id, unsigned char *s,
			 unsigned int ofs, unsigned int len)
{
	unsigned int c;

	BUG_ON(len & 1);

	while (len > 0) {
		c = id[ofs] >> 8;
		*s = c;
		s++;

		c = id[ofs] & 0xff;
		*s = c;
		s++;

		ofs++;
		len -= 2;
	}
}

/* local copy of ata_id_c_string() */
static void mg_id_c_string(const u16 *id, unsigned char *s,
			   unsigned int ofs, unsigned int len)
{
	unsigned char *p;

	mg_id_string(id, s, ofs, len - 1);

	p = s + strnlen(s, len - 1);
	while (p > s && p[-1] == ' ')
		p--;
	*p = '\0';
}

static int mg_get_disk_id(struct mg_host *host)
{
	u32 i;
	s32 err;
	const u16 *id = host->id;
	struct mg_drv_data *prv_data = host->dev->platform_data;
	char fwrev[ATA_ID_FW_REV_LEN + 1];
	char model[ATA_ID_PROD_LEN + 1];
	char serial[ATA_ID_SERNO_LEN + 1];

	if (!prv_data->use_polling)
		outb(ATA_NIEN, (unsigned long)host->dev_base + MG_REG_DRV_CTRL);

	outb(MG_CMD_ID, (unsigned long)host->dev_base + MG_REG_COMMAND);
	err = mg_wait(host, ATA_DRQ, MG_TMAX_WAIT_RD_DRQ);
	if (err)
		return err;

	for (i = 0; i < (MG_SECTOR_SIZE >> 1); i++)
		host->id[i] = le16_to_cpu(inw((unsigned long)host->dev_base +
					MG_BUFF_OFFSET + i * 2));

	outb(MG_CMD_RD_CONF, (unsigned long)host->dev_base + MG_REG_COMMAND);
	err = mg_wait(host, MG_STAT_READY, MG_TMAX_CONF_TO_CMD);
	if (err)
		return err;

	if ((id[ATA_ID_FIELD_VALID] & 1) == 0)
		return MG_ERR_TRANSLATION;

	host->n_sectors = ata_id_u32(id, ATA_ID_LBA_CAPACITY);
	host->cyls = id[ATA_ID_CYLS];
	host->heads = id[ATA_ID_HEADS];
	host->sectors = id[ATA_ID_SECTORS];

	if (MG_RES_SEC && host->heads && host->sectors) {
		/* modify cyls, n_sectors */
		host->cyls = (host->n_sectors - MG_RES_SEC) /
			host->heads / host->sectors;
		host->nres_sectors = host->n_sectors - host->cyls *
			host->heads * host->sectors;
		host->n_sectors -= host->nres_sectors;
	}

	mg_id_c_string(id, fwrev, ATA_ID_FW_REV, sizeof(fwrev));
	mg_id_c_string(id, model, ATA_ID_PROD, sizeof(model));
	mg_id_c_string(id, serial, ATA_ID_SERNO, sizeof(serial));
	printk(KERN_INFO "mg_disk: model: %s\n", model);
	printk(KERN_INFO "mg_disk: firm: %.8s\n", fwrev);
	printk(KERN_INFO "mg_disk: serial: %s\n", serial);
	printk(KERN_INFO "mg_disk: %d + reserved %d sectors\n",
			host->n_sectors, host->nres_sectors);

	if (!prv_data->use_polling)
		outb(0, (unsigned long)host->dev_base + MG_REG_DRV_CTRL);

	return err;
}


static int mg_disk_init(struct mg_host *host)
{
	struct mg_drv_data *prv_data = host->dev->platform_data;
	s32 err;
	u8 init_status;

	/* hdd rst low */
	gpio_set_value(host->rst, 0);
	err = mg_wait(host, ATA_BUSY, MG_TMAX_RST_TO_BUSY);
	if (err)
		return err;

	/* hdd rst high */
	gpio_set_value(host->rst, 1);
	err = mg_wait(host, MG_STAT_READY, MG_TMAX_HDRST_TO_RDY);
	if (err)
		return err;

	/* soft reset on */
	outb(ATA_SRST | (prv_data->use_polling ? ATA_NIEN : 0),
			(unsigned long)host->dev_base + MG_REG_DRV_CTRL);
	err = mg_wait(host, ATA_BUSY, MG_TMAX_RST_TO_BUSY);
	if (err)
		return err;

	/* soft reset off */
	outb(prv_data->use_polling ? ATA_NIEN : 0,
			(unsigned long)host->dev_base + MG_REG_DRV_CTRL);
	err = mg_wait(host, MG_STAT_READY, MG_TMAX_SWRST_TO_RDY);
	if (err)
		return err;

	init_status = inb((unsigned long)host->dev_base + MG_REG_STATUS) & 0xf;

	if (init_status == 0xf)
		return MG_ERR_INIT_STAT;

	return err;
}

static void mg_bad_rw_intr(struct mg_host *host)
{
	if (host->req)
		if (++host->req->errors >= MG_MAX_ERRORS ||
		    host->error == MG_ERR_TIMEOUT)
			mg_end_request_cur(host, -EIO);
}

static unsigned int mg_out(struct mg_host *host,
		unsigned int sect_num,
		unsigned int sect_cnt,
		unsigned int cmd,
		void (*intr_addr)(struct mg_host *))
{
	struct mg_drv_data *prv_data = host->dev->platform_data;

	if (mg_wait(host, MG_STAT_READY, MG_TMAX_CONF_TO_CMD))
		return host->error;

	if (!prv_data->use_polling) {
		host->mg_do_intr = intr_addr;
		mod_timer(&host->timer, jiffies + 3 * HZ);
	}
	if (MG_RES_SEC)
		sect_num += MG_RES_SEC;
	outb((u8)sect_cnt, (unsigned long)host->dev_base + MG_REG_SECT_CNT);
	outb((u8)sect_num, (unsigned long)host->dev_base + MG_REG_SECT_NUM);
	outb((u8)(sect_num >> 8), (unsigned long)host->dev_base +
			MG_REG_CYL_LOW);
	outb((u8)(sect_num >> 16), (unsigned long)host->dev_base +
			MG_REG_CYL_HIGH);
	outb((u8)((sect_num >> 24) | ATA_LBA | ATA_DEVICE_OBS),
			(unsigned long)host->dev_base + MG_REG_DRV_HEAD);
	outb(cmd, (unsigned long)host->dev_base + MG_REG_COMMAND);
	return MG_ERR_NONE;
}

static void mg_read_one(struct mg_host *host, struct request *req)
{
	u16 *buff = (u16 *)bio_data(req->bio);
	u32 i;

	for (i = 0; i < MG_SECTOR_SIZE >> 1; i++)
		*buff++ = inw((unsigned long)host->dev_base + MG_BUFF_OFFSET +
			      (i << 1));
}

static void mg_read(struct request *req)
{
	struct mg_host *host = req->rq_disk->private_data;

	if (mg_out(host, blk_rq_pos(req), blk_rq_sectors(req),
		   MG_CMD_RD, NULL) != MG_ERR_NONE)
		mg_bad_rw_intr(host);

	MG_DBG("requested %d sects (from %ld), buffer=0x%p\n",
	       blk_rq_sectors(req), blk_rq_pos(req), bio_data(req->bio));

	do {
		if (mg_wait(host, ATA_DRQ,
			    MG_TMAX_WAIT_RD_DRQ) != MG_ERR_NONE) {
			mg_bad_rw_intr(host);
			return;
		}

		mg_read_one(host, req);

		outb(MG_CMD_RD_CONF, (unsigned long)host->dev_base +
				MG_REG_COMMAND);
	} while (mg_end_request(host, 0, MG_SECTOR_SIZE));
}

static void mg_write_one(struct mg_host *host, struct request *req)
{
	u16 *buff = (u16 *)bio_data(req->bio);
	u32 i;

	for (i = 0; i < MG_SECTOR_SIZE >> 1; i++)
		outw(*buff++, (unsigned long)host->dev_base + MG_BUFF_OFFSET +
		     (i << 1));
}

static void mg_write(struct request *req)
{
	struct mg_host *host = req->rq_disk->private_data;
	unsigned int rem = blk_rq_sectors(req);

	if (mg_out(host, blk_rq_pos(req), rem,
		   MG_CMD_WR, NULL) != MG_ERR_NONE) {
		mg_bad_rw_intr(host);
		return;
	}

	MG_DBG("requested %d sects (from %ld), buffer=0x%p\n",
	       rem, blk_rq_pos(req), bio_data(req->bio));

	if (mg_wait(host, ATA_DRQ,
		    MG_TMAX_WAIT_WR_DRQ) != MG_ERR_NONE) {
		mg_bad_rw_intr(host);
		return;
	}

	do {
		mg_write_one(host, req);

		outb(MG_CMD_WR_CONF, (unsigned long)host->dev_base +
				MG_REG_COMMAND);

		rem--;
		if (rem > 1 && mg_wait(host, ATA_DRQ,
					MG_TMAX_WAIT_WR_DRQ) != MG_ERR_NONE) {
			mg_bad_rw_intr(host);
			return;
		} else if (mg_wait(host, MG_STAT_READY,
					MG_TMAX_WAIT_WR_DRQ) != MG_ERR_NONE) {
			mg_bad_rw_intr(host);
			return;
		}
	} while (mg_end_request(host, 0, MG_SECTOR_SIZE));
}

static void mg_read_intr(struct mg_host *host)
{
	struct request *req = host->req;
	u32 i;

	/* check status */
	do {
		i = inb((unsigned long)host->dev_base + MG_REG_STATUS);
		if (i & ATA_BUSY)
			break;
		if (!MG_READY_OK(i))
			break;
		if (i & ATA_DRQ)
			goto ok_to_read;
	} while (0);
	mg_dump_status("mg_read_intr", i, host);
	mg_bad_rw_intr(host);
	mg_request(host->breq);
	return;

ok_to_read:
	mg_read_one(host, req);

	MG_DBG("sector %ld, remaining=%ld, buffer=0x%p\n",
	       blk_rq_pos(req), blk_rq_sectors(req) - 1, bio_data(req->bio));

	/* send read confirm */
	outb(MG_CMD_RD_CONF, (unsigned long)host->dev_base + MG_REG_COMMAND);

	if (mg_end_request(host, 0, MG_SECTOR_SIZE)) {
		/* set handler if read remains */
		host->mg_do_intr = mg_read_intr;
		mod_timer(&host->timer, jiffies + 3 * HZ);
	} else /* goto next request */
		mg_request(host->breq);
}

static void mg_write_intr(struct mg_host *host)
{
	struct request *req = host->req;
	u32 i;
	bool rem;

	/* check status */
	do {
		i = inb((unsigned long)host->dev_base + MG_REG_STATUS);
		if (i & ATA_BUSY)
			break;
		if (!MG_READY_OK(i))
			break;
		if ((blk_rq_sectors(req) <= 1) || (i & ATA_DRQ))
			goto ok_to_write;
	} while (0);
	mg_dump_status("mg_write_intr", i, host);
	mg_bad_rw_intr(host);
	mg_request(host->breq);
	return;

ok_to_write:
	if ((rem = mg_end_request(host, 0, MG_SECTOR_SIZE))) {
		/* write 1 sector and set handler if remains */
		mg_write_one(host, req);
		MG_DBG("sector %ld, remaining=%ld, buffer=0x%p\n",
		       blk_rq_pos(req), blk_rq_sectors(req), bio_data(req->bio));
		host->mg_do_intr = mg_write_intr;
		mod_timer(&host->timer, jiffies + 3 * HZ);
	}

	/* send write confirm */
	outb(MG_CMD_WR_CONF, (unsigned long)host->dev_base + MG_REG_COMMAND);

	if (!rem)
		mg_request(host->breq);
}

static void mg_times_out(unsigned long data)
{
	struct mg_host *host = (struct mg_host *)data;
	char *name;

	spin_lock_irq(&host->lock);

	if (!host->req)
		goto out_unlock;

	host->mg_do_intr = NULL;

	name = host->req->rq_disk->disk_name;
	printk(KERN_DEBUG "%s: timeout\n", name);

	host->error = MG_ERR_TIMEOUT;
	mg_bad_rw_intr(host);

out_unlock:
	mg_request(host->breq);
	spin_unlock_irq(&host->lock);
}

static void mg_request_poll(struct request_queue *q)
{
	struct mg_host *host = q->queuedata;

	while (1) {
		if (!host->req) {
			host->req = blk_fetch_request(q);
			if (!host->req)
				break;
		}

		if (unlikely(host->req->cmd_type != REQ_TYPE_FS)) {
			mg_end_request_cur(host, -EIO);
			continue;
		}

		if (rq_data_dir(host->req) == READ)
			mg_read(host->req);
		else
			mg_write(host->req);
	}
}

static unsigned int mg_issue_req(struct request *req,
		struct mg_host *host,
		unsigned int sect_num,
		unsigned int sect_cnt)
{
	switch (rq_data_dir(req)) {
	case READ:
		if (mg_out(host, sect_num, sect_cnt, MG_CMD_RD, &mg_read_intr)
				!= MG_ERR_NONE) {
			mg_bad_rw_intr(host);
			return host->error;
		}
		break;
	case WRITE:
		/* TODO : handler */
		outb(ATA_NIEN, (unsigned long)host->dev_base + MG_REG_DRV_CTRL);
		if (mg_out(host, sect_num, sect_cnt, MG_CMD_WR, &mg_write_intr)
				!= MG_ERR_NONE) {
			mg_bad_rw_intr(host);
			return host->error;
		}
		del_timer(&host->timer);
		mg_wait(host, ATA_DRQ, MG_TMAX_WAIT_WR_DRQ);
		outb(0, (unsigned long)host->dev_base + MG_REG_DRV_CTRL);
		if (host->error) {
			mg_bad_rw_intr(host);
			return host->error;
		}
		mg_write_one(host, req);
		mod_timer(&host->timer, jiffies + 3 * HZ);
		outb(MG_CMD_WR_CONF, (unsigned long)host->dev_base +
				MG_REG_COMMAND);
		break;
	}
	return MG_ERR_NONE;
}

/* This function also called from IRQ context */
static void mg_request(struct request_queue *q)
{
	struct mg_host *host = q->queuedata;
	struct request *req;
	u32 sect_num, sect_cnt;

	while (1) {
		if (!host->req) {
			host->req = blk_fetch_request(q);
			if (!host->req)
				break;
		}
		req = host->req;

		/* check unwanted request call */
		if (host->mg_do_intr)
			return;

		del_timer(&host->timer);

		sect_num = blk_rq_pos(req);
		/* deal whole segments */
		sect_cnt = blk_rq_sectors(req);

		/* sanity check */
		if (sect_num >= get_capacity(req->rq_disk) ||
				((sect_num + sect_cnt) >
				 get_capacity(req->rq_disk))) {
			printk(KERN_WARNING
					"%s: bad access: sector=%d, count=%d\n",
					req->rq_disk->disk_name,
					sect_num, sect_cnt);
			mg_end_request_cur(host, -EIO);
			continue;
		}

		if (unlikely(req->cmd_type != REQ_TYPE_FS)) {
			mg_end_request_cur(host, -EIO);
			continue;
		}

		if (!mg_issue_req(req, host, sect_num, sect_cnt))
			return;
	}
}

static int mg_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct mg_host *host = bdev->bd_disk->private_data;

	geo->cylinders = (unsigned short)host->cyls;
	geo->heads = (unsigned char)host->heads;
	geo->sectors = (unsigned char)host->sectors;
	return 0;
}

static const struct block_device_operations mg_disk_ops = {
	.getgeo = mg_getgeo
};

#ifdef CONFIG_PM_SLEEP
static int mg_suspend(struct device *dev)
{
	struct mg_drv_data *prv_data = dev->platform_data;
	struct mg_host *host = prv_data->host;

	if (mg_wait(host, MG_STAT_READY, MG_TMAX_CONF_TO_CMD))
		return -EIO;

	if (!prv_data->use_polling)
		outb(ATA_NIEN, (unsigned long)host->dev_base + MG_REG_DRV_CTRL);

	outb(MG_CMD_SLEEP, (unsigned long)host->dev_base + MG_REG_COMMAND);
	/* wait until mflash deep sleep */
	msleep(1);

	if (mg_wait(host, MG_STAT_READY, MG_TMAX_CONF_TO_CMD)) {
		if (!prv_data->use_polling)
			outb(0, (unsigned long)host->dev_base + MG_REG_DRV_CTRL);
		return -EIO;
	}

	return 0;
}

static int mg_resume(struct device *dev)
{
	struct mg_drv_data *prv_data = dev->platform_data;
	struct mg_host *host = prv_data->host;

	if (mg_wait(host, MG_STAT_READY, MG_TMAX_CONF_TO_CMD))
		return -EIO;

	outb(MG_CMD_WAKEUP, (unsigned long)host->dev_base + MG_REG_COMMAND);
	/* wait until mflash wakeup */
	msleep(1);

	if (mg_wait(host, MG_STAT_READY, MG_TMAX_CONF_TO_CMD))
		return -EIO;

	if (!prv_data->use_polling)
		outb(0, (unsigned long)host->dev_base + MG_REG_DRV_CTRL);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mg_pm, mg_suspend, mg_resume);

static int mg_probe(struct platform_device *plat_dev)
{
	struct mg_host *host;
	struct resource *rsc;
	struct mg_drv_data *prv_data = plat_dev->dev.platform_data;
	int err = 0;

	if (!prv_data) {
		printk(KERN_ERR	"%s:%d fail (no driver_data)\n",
				__func__, __LINE__);
		err = -EINVAL;
		goto probe_err;
	}

	/* alloc mg_host */
	host = kzalloc(sizeof(struct mg_host), GFP_KERNEL);
	if (!host) {
		printk(KERN_ERR "%s:%d fail (no memory for mg_host)\n",
				__func__, __LINE__);
		err = -ENOMEM;
		goto probe_err;
	}
	host->major = MG_DISK_MAJ;

	/* link each other */
	prv_data->host = host;
	host->dev = &plat_dev->dev;

	/* io remap */
	rsc = platform_get_resource(plat_dev, IORESOURCE_MEM, 0);
	if (!rsc) {
		printk(KERN_ERR "%s:%d platform_get_resource fail\n",
				__func__, __LINE__);
		err = -EINVAL;
		goto probe_err_2;
	}
	host->dev_base = ioremap(rsc->start, resource_size(rsc));
	if (!host->dev_base) {
		printk(KERN_ERR "%s:%d ioremap fail\n",
				__func__, __LINE__);
		err = -EIO;
		goto probe_err_2;
	}
	MG_DBG("dev_base = 0x%x\n", (u32)host->dev_base);

	/* get reset pin */
	rsc = platform_get_resource_byname(plat_dev, IORESOURCE_IO,
			MG_RST_PIN);
	if (!rsc) {
		printk(KERN_ERR "%s:%d get reset pin fail\n",
				__func__, __LINE__);
		err = -EIO;
		goto probe_err_3;
	}
	host->rst = rsc->start;

	/* init rst pin */
	err = gpio_request(host->rst, MG_RST_PIN);
	if (err)
		goto probe_err_3;
	gpio_direction_output(host->rst, 1);

	/* reset out pin */
	if (!(prv_data->dev_attr & MG_DEV_MASK)) {
		err = -EINVAL;
		goto probe_err_3a;
	}

	if (prv_data->dev_attr != MG_BOOT_DEV) {
		rsc = platform_get_resource_byname(plat_dev, IORESOURCE_IO,
				MG_RSTOUT_PIN);
		if (!rsc) {
			printk(KERN_ERR "%s:%d get reset-out pin fail\n",
					__func__, __LINE__);
			err = -EIO;
			goto probe_err_3a;
		}
		host->rstout = rsc->start;
		err = gpio_request(host->rstout, MG_RSTOUT_PIN);
		if (err)
			goto probe_err_3a;
		gpio_direction_input(host->rstout);
	}

	/* disk reset */
	if (prv_data->dev_attr == MG_STORAGE_DEV) {
		/* If POR seq. not yet finished, wait */
		err = mg_wait_rstout(host->rstout, MG_TMAX_RSTOUT);
		if (err)
			goto probe_err_3b;
		err = mg_disk_init(host);
		if (err) {
			printk(KERN_ERR "%s:%d fail (err code : %d)\n",
					__func__, __LINE__, err);
			err = -EIO;
			goto probe_err_3b;
		}
	}

	/* get irq resource */
	if (!prv_data->use_polling) {
		host->irq = platform_get_irq(plat_dev, 0);
		if (host->irq == -ENXIO) {
			err = host->irq;
			goto probe_err_3b;
		}
		err = request_irq(host->irq, mg_irq,
				IRQF_TRIGGER_RISING,
				MG_DEV_NAME, host);
		if (err) {
			printk(KERN_ERR "%s:%d fail (request_irq err=%d)\n",
					__func__, __LINE__, err);
			goto probe_err_3b;
		}

	}

	/* get disk id */
	err = mg_get_disk_id(host);
	if (err) {
		printk(KERN_ERR "%s:%d fail (err code : %d)\n",
				__func__, __LINE__, err);
		err = -EIO;
		goto probe_err_4;
	}

	err = register_blkdev(host->major, MG_DISK_NAME);
	if (err < 0) {
		printk(KERN_ERR "%s:%d register_blkdev fail (err code : %d)\n",
				__func__, __LINE__, err);
		goto probe_err_4;
	}
	if (!host->major)
		host->major = err;

	spin_lock_init(&host->lock);

	if (prv_data->use_polling)
		host->breq = blk_init_queue(mg_request_poll, &host->lock);
	else
		host->breq = blk_init_queue(mg_request, &host->lock);

	if (!host->breq) {
		err = -ENOMEM;
		printk(KERN_ERR "%s:%d (blk_init_queue) fail\n",
				__func__, __LINE__);
		goto probe_err_5;
	}
	host->breq->queuedata = host;

	/* mflash is random device, thanx for the noop */
	err = elevator_change(host->breq, "noop");
	if (err) {
		printk(KERN_ERR "%s:%d (elevator_init) fail\n",
				__func__, __LINE__);
		goto probe_err_6;
	}
	blk_queue_max_hw_sectors(host->breq, MG_MAX_SECTS);
	blk_queue_logical_block_size(host->breq, MG_SECTOR_SIZE);

	init_timer(&host->timer);
	host->timer.function = mg_times_out;
	host->timer.data = (unsigned long)host;

	host->gd = alloc_disk(MG_DISK_MAX_PART);
	if (!host->gd) {
		printk(KERN_ERR "%s:%d (alloc_disk) fail\n",
				__func__, __LINE__);
		err = -ENOMEM;
		goto probe_err_7;
	}
	host->gd->major = host->major;
	host->gd->first_minor = 0;
	host->gd->fops = &mg_disk_ops;
	host->gd->queue = host->breq;
	host->gd->private_data = host;
	sprintf(host->gd->disk_name, MG_DISK_NAME"a");

	set_capacity(host->gd, host->n_sectors);

	add_disk(host->gd);

	return err;

probe_err_7:
	del_timer_sync(&host->timer);
probe_err_6:
	blk_cleanup_queue(host->breq);
probe_err_5:
	unregister_blkdev(MG_DISK_MAJ, MG_DISK_NAME);
probe_err_4:
	if (!prv_data->use_polling)
		free_irq(host->irq, host);
probe_err_3b:
	gpio_free(host->rstout);
probe_err_3a:
	gpio_free(host->rst);
probe_err_3:
	iounmap(host->dev_base);
probe_err_2:
	kfree(host);
probe_err:
	return err;
}

static int mg_remove(struct platform_device *plat_dev)
{
	struct mg_drv_data *prv_data = plat_dev->dev.platform_data;
	struct mg_host *host = prv_data->host;
	int err = 0;

	/* delete timer */
	del_timer_sync(&host->timer);

	/* remove disk */
	if (host->gd) {
		del_gendisk(host->gd);
		put_disk(host->gd);
	}
	/* remove queue */
	if (host->breq)
		blk_cleanup_queue(host->breq);

	/* unregister blk device */
	unregister_blkdev(host->major, MG_DISK_NAME);

	/* free irq */
	if (!prv_data->use_polling)
		free_irq(host->irq, host);

	/* free reset-out pin */
	if (prv_data->dev_attr != MG_BOOT_DEV)
		gpio_free(host->rstout);

	/* free rst pin */
	if (host->rst)
		gpio_free(host->rst);

	/* unmap io */
	if (host->dev_base)
		iounmap(host->dev_base);

	/* free mg_host */
	kfree(host);

	return err;
}

static struct platform_driver mg_disk_driver = {
	.probe = mg_probe,
	.remove = mg_remove,
	.driver = {
		.name = MG_DEV_NAME,
		.pm = &mg_pm,
	}
};

/****************************************************************************
 *
 * Module stuff
 *
 ****************************************************************************/

static int __init mg_init(void)
{
	printk(KERN_INFO "mGine mflash driver, (c) 2008 mGine Co.\n");
	return platform_driver_register(&mg_disk_driver);
}

static void __exit mg_exit(void)
{
	printk(KERN_INFO "mflash driver : bye bye\n");
	platform_driver_unregister(&mg_disk_driver);
}

module_init(mg_init);
module_exit(mg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("unsik Kim <donari75@gmail.com>");
MODULE_DESCRIPTION("mGine m[g]flash device driver");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        #
# mtip32xx device driver configuration
#

config BLK_DEV_PCIESSD_MTIP32XX
	tristate "Block Device Driver for Micron PCIe SSDs"
	depends on PCI
	help
          This enables the block driver for Micron PCIe SSDs.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           #
# Makefile for  Block device driver for Micron PCIe SSD
#

obj-$(CONFIG_BLK_DEV_PCIESSD_MTIP32XX) += mtip32xx.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*
 * mtip32xx.h - Header file for the P320 SSD Block Driver
 *   Copyright (C) 2011 Micron Technology, Inc.
 *
 * Portions of this code were derived from works subjected to the
 * following copyright:
 *    Copyright (C) 2009 Integrated Device Technology, Inc.
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
 */

#ifndef __MTIP32XX_H__
#define __MTIP32XX_H__

#include <linux/spinlock.h>
#include <linux/rwsem.h>
#include <linux/ata.h>
#include <linux/interrupt.h>
#include <linux/genhd.h>

/* Offset of Subsystem Device ID in pci confoguration space */
#define PCI_SUBSYSTEM_DEVICEID	0x2E

/* offset of Device Control register in PCIe extended capabilites space */
#define PCIE_CONFIG_EXT_DEVICE_CONTROL_OFFSET	0x48

/* check for erase mode support during secure erase */
#define MTIP_SEC_ERASE_MODE     0x2

/* # of times to retry timed out/failed IOs */
#define MTIP_MAX_RETRIES	2

/* Various timeout values in ms */
#define MTIP_NCQ_CMD_TIMEOUT_MS      15000
#define MTIP_IOCTL_CMD_TIMEOUT_MS    5000
#define MTIP_INT_CMD_TIMEOUT_MS      5000
#define MTIP_QUIESCE_IO_TIMEOUT_MS   (MTIP_NCQ_CMD_TIMEOUT_MS * \
				     (MTIP_MAX_RETRIES + 1))

/* check for timeouts every 500ms */
#define MTIP_TIMEOUT_CHECK_PERIOD	500

/* ftl rebuild */
#define MTIP_FTL_REBUILD_OFFSET		142
#define MTIP_FTL_REBUILD_MAGIC		0xED51
#define MTIP_FTL_REBUILD_TIMEOUT_MS	2400000

/* unaligned IO handling */
#define MTIP_MAX_UNALIGNED_SLOTS	2

/* Macro to extract the tag bit number from a tag value. */
#define MTIP_TAG_BIT(tag)	(tag & 0x1F)

/*
 * Macro to extract the tag index from a tag value. The index
 * is used to access the correct s_active/Command Issue register based
 * on the tag value.
 */
#define MTIP_TAG_INDEX(tag)	(tag >> 5)

/*
 * Maximum number of scatter gather entries
 * a single command may have.
 */
#define MTIP_MAX_SG		504

/*
 * Maximum number of slot groups (Command Issue & s_active registers)
 * NOTE: This is the driver maximum; check dd->slot_groups for actual value.
 */
#define MTIP_MAX_SLOT_GROUPS	8

/* Internal command tag. */
#define MTIP_TAG_INTERNAL	0

/* Micron Vendor ID & P320x SSD Device ID */
#define PCI_VENDOR_ID_MICRON    0x1344
#define P320H_DEVICE_ID		0x5150
#define P320M_DEVICE_ID		0x5151
#define P320S_DEVICE_ID		0x5152
#define P325M_DEVICE_ID		0x5153
#define P420H_DEVICE_ID		0x5160
#define P420M_DEVICE_ID		0x5161
#define P425M_DEVICE_ID		0x5163

/* Driver name and version strings */
#define MTIP_DRV_NAME		"mtip32xx"
#define MTIP_DRV_VERSION	"1.3.1"

/* Maximum number of minor device numbers per device. */
#define MTIP_MAX_MINORS		16

/* Maximum number of supported command slots. */
#define MTIP_MAX_COMMAND_SLOTS	(MTIP_MAX_SLOT_GROUPS * 32)

/*
 * Per-tag bitfield size in longs.
 * Linux bit manipulation functions
 * (i.e. test_and_set_bit, find_next_zero_bit)
 * manipulate memory in longs, so we try to make the math work.
 * take the slot groups and find the number of longs, rounding up.
 * Careful! i386 and x86_64 use different size longs!
 */
#define U32_PER_LONG	(sizeof(long) / sizeof(u32))
#define SLOTBITS_IN_LONGS ((MTIP_MAX_SLOT_GROUPS + \
					(U32_PER_LONG-1))/U32_PER_LONG)

/* BAR number used to access the HBA registers. */
#define MTIP_ABAR		5

#ifdef DEBUG
 #define dbg_printk(format, arg...)	\
	printk(pr_fmt(format), ##arg);
#else
 #define dbg_printk(format, arg...)
#endif

#define MTIP_DFS_MAX_BUF_SIZE 1024

#define __force_bit2int (unsigned int __force)

enum {
	/* below are bit numbers in 'flags' defined in mtip_port */
	MTIP_PF_IC_ACTIVE_BIT       = 0, /* pio/ioctl */
	MTIP_PF_EH_ACTIVE_BIT       = 1, /* error handling */
	MTIP_PF_SE_ACTIVE_BIT       = 2, /* secure erase */
	MTIP_PF_DM_ACTIVE_BIT       = 3, /* download microcde */
	MTIP_PF_TO_ACTIVE_BIT       = 9, /* timeout handling */
	MTIP_PF_PAUSE_IO      =	((1 << MTIP_PF_IC_ACTIVE_BIT) |
				(1 << MTIP_PF_EH_ACTIVE_BIT) |
				(1 << MTIP_PF_SE_ACTIVE_BIT) |
				(1 << MTIP_PF_DM_ACTIVE_BIT) |
				(1 << MTIP_PF_TO_ACTIVE_BIT)),

	MTIP_PF_SVC_THD_ACTIVE_BIT  = 4,
	MTIP_PF_ISSUE_CMDS_BIT      = 5,
	MTIP_PF_REBUILD_BIT         = 6,
	MTIP_PF_SVC_THD_STOP_BIT    = 8,

	MTIP_PF_SVC_THD_WORK	= ((1 << MTIP_PF_EH_ACTIVE_BIT) |
				  (1 << MTIP_PF_ISSUE_CMDS_BIT) |
				  (1 << MTIP_PF_REBUILD_BIT) |
				  (1 << MTIP_PF_SVC_THD_STOP_BIT) |
				  (1 << MTIP_PF_TO_ACTIVE_BIT)),

	/* below are bit numbers in 'dd_flag' defined in driver_data */
	MTIP_DDF_SEC_LOCK_BIT	    = 0,
	MTIP_DDF_REMOVE_PENDING_BIT = 1,
	MTIP_DDF_OVER_TEMP_BIT      = 2,
	MTIP_DDF_WRITE_PROTECT_BIT  = 3,
	MTIP_DDF_CLEANUP_BIT        = 5,
	MTIP_DDF_RESUME_BIT         = 6,
	MTIP_DDF_INIT_DONE_BIT      = 7,
	MTIP_DDF_REBUILD_FAILED_BIT = 8,
	MTIP_DDF_REMOVAL_BIT	    = 9,

	MTIP_DDF_STOP_IO      = ((1 << MTIP_DDF_REMOVE_PENDING_BIT) |
				(1 << MTIP_DDF_SEC_LOCK_BIT) |
				(1 << MTIP_DDF_OVER_TEMP_BIT) |
				(1 << MTIP_DDF_WRITE_PROTECT_BIT) |
				(1 << MTIP_DDF_REBUILD_FAILED_BIT)),

};

struct smart_attr {
	u8 attr_id;
	u16 flags;
	u8 cur;
	u8 worst;
	u32 data;
	u8 res[3];
} __packed;

struct mtip_work {
	struct work_struct work;
	void *port;
	int cpu_binding;
	u32 completed;
} ____cacheline_aligned_in_smp;

#define DEFINE_HANDLER(group)                                  \
	void mtip_workq_sdbf##group(struct work_struct *work)       \
	{                                                      \
		struct mtip_work *w = (struct mtip_work *) work;         \
		mtip_workq_sdbfx(w->port, group, w->completed);     \
	}

#define MTIP_TRIM_TIMEOUT_MS		240000
#define MTIP_MAX_TRIM_ENTRIES		8
#define MTIP_MAX_TRIM_ENTRY_LEN		0xfff8

struct mtip_trim_entry {
	u32 lba;   /* starting lba of region */
	u16 rsvd;  /* unused */
	u16 range; /* # of 512b blocks to trim */
} __packed;

struct mtip_trim {
	/* Array of regions to trim */
	struct mtip_trim_entry entry[MTIP_MAX_TRIM_ENTRIES];
} __packed;

/* Register Frame Information Structure (FIS), host to device. */
struct host_to_dev_fis {
	/*
	 * FIS type.
	 * - 27h Register FIS, host to device.
	 * - 34h Register FIS, device to host.
	 * - 39h DMA Activate FIS, device to host.
	 * - 41h DMA Setup FIS, bi-directional.
	 * - 46h Data FIS, bi-directional.
	 * - 58h BIST Activate FIS, bi-directional.
	 * - 5Fh PIO Setup FIS, device to host.
	 * - A1h Set Device Bits FIS, device to host.
	 */
	unsigned char type;
	unsigned char opts;
	unsigned char command;
	unsigned char features;

	union {
		unsigned char lba_low;
		unsigned char sector;
	};
	union {
		unsigned char lba_mid;
		unsigned char cyl_low;
	};
	union {
		unsigned char lba_hi;
		unsigned char cyl_hi;
	};
	union {
		unsigned char device;
		unsigned char head;
	};

	union {
		unsigned char lba_low_ex;
		unsigned char sector_ex;
	};
	union {
		unsigned char lba_mid_ex;
		unsigned char cyl_low_ex;
	};
	union {
		unsigned char lba_hi_ex;
		unsigned char cyl_hi_ex;
	};
	unsigned char features_ex;

	unsigned char sect_count;
	unsigned char sect_cnt_ex;
	unsigned char res2;
	unsigned char control;

	unsigned int res3;
};

/* Command header structure. */
struct mtip_cmd_hdr {
	/*
	 * Command options.
	 * - Bits 31:16 Number of PRD entries.
	 * - Bits 15:8 Unused in this implementation.
	 * - Bit 7 Prefetch bit, informs the drive to prefetch PRD entries.
	 * - Bit 6 Write bit, should be set when writing data to the device.
	 * - Bit 5 Unused in this implementation.
	 * - Bits 4:0 Length of the command FIS in DWords (DWord = 4 bytes).
	 */
	unsigned int opts;
	/* This field is unsed when using NCQ. */
	union {
		unsigned int byte_count;
		unsigned int status;
	};
	/*
	 * Lower 32 bits of the command table address associated with this
	 * header. The command table addresses must be 128 byte aligned.
	 */
	unsigned int ctba;
	/*
	 * If 64 bit addressing is used this field is the upper 32 bits
	 * of the command table address associated with this command.
	 */
	unsigned int ctbau;
	/* Reserved and unused. */
	unsigned int res[4];
};

/* Command scatter gather structure (PRD). */
struct mtip_cmd_sg {
	/*
	 * Low 32 bits of the data buffer address. For P320 this
	 * address must be 8 byte aligned signified by bits 2:0 being
	 * set to 0.
	 */
	unsigned int dba;
	/*
	 * When 64 bit addressing is used this field is the upper
	 * 32 bits of the data buffer address.
	 */
	unsigned int dba_upper;
	/* Unused. */
	unsigned int reserved;
	/*
	 * Bit 31: interrupt when this data block has been transferred.
	 * Bits 30..22: reserved
	 * Bits 21..0: byte count (minus 1).  For P320 the byte count must be
	 * 8 byte aligned signified by bits 2:0 being set to 1.
	 */
	unsigned int info;
};
struct mtip_port;

/* Structure used to describe a command. */
struct mtip_cmd {

	struct mtip_cmd_hdr *command_header; /* ptr to command header entry */

	dma_addr_t command_header_dma; /* corresponding physical address */

	void *command; /* ptr to command table entry */

	dma_addr_t command_dma; /* corresponding physical address */

	void *comp_data; /* data passed to completion function comp_func() */
	/*
	 * Completion function called by the ISR upon completion of
	 * a command.
	 */
	void (*comp_func)(struct mtip_port *port,
				int tag,
				struct mtip_cmd *cmd,
				int status);

	int scatter_ents; /* Number of scatter list entries used */

	int unaligned; /* command is unaligned on 4k boundary */

	struct scatterlist sg[MTIP_MAX_SG]; /* Scatter list entries */

	int retries; /* The number of retries left for this command. */

	int direction; /* Data transfer direction */
};

/* Structure used to describe a port. */
struct mtip_port {
	/* Pointer back to the driver data for this port. */
	struct driver_data *dd;
	/*
	 * Used to determine if the data pointed to by the
	 * identify field is valid.
	 */
	unsigned long identify_valid;
	/* Base address of the memory mapped IO for the port. */
	void __iomem *mmio;
	/* Array of pointers to the memory mapped s_active registers. */
	void __iomem *s_active[MTIP_MAX_SLOT_GROUPS];
	/* Array of pointers to the memory mapped completed registers. */
	void __iomem *completed[MTIP_MAX_SLOT_GROUPS];
	/* Array of pointers to the memory mapped Command Issue registers. */
	void __iomem *cmd_issue[MTIP_MAX_SLOT_GROUPS];
	/*
	 * Pointer to the beginning of the command header memory as used
	 * by the driver.
	 */
	void *command_list;
	/*
	 * Pointer to the beginning of the command header memory as used
	 * by the DMA.
	 */
	dma_addr_t command_list_dma;
	/*
	 * Pointer to the beginning of the RX FIS memory as used
	 * by the driver.
	 */
	void *rxfis;
	/*
	 * Pointer to the beginning of the RX FIS memory as used
	 * by the DMA.
	 */
	dma_addr_t rxfis_dma;
	/*
	 * Pointer to the DMA region for RX Fis, Identify, RLE10, and SMART
	 */
	void *block1;
	/*
	 * DMA address of region for RX Fis, Identify, RLE10, and SMART
	 */
	dma_addr_t block1_dma;
	/*
	 * Pointer to the beginning of the identify data memory as used
	 * by the driver.
	 */
	u16 *identify;
	/*
	 * Pointer to the beginning of the identify data memory as used
	 * by the DMA.
	 */
	dma_addr_t identify_dma;
	/*
	 * Pointer to the beginning of a sector buffer that is used
	 * by the driver when issuing internal commands.
	 */
	u16 *sector_buffer;
	/*
	 * Pointer to the beginning of a sector buffer that is used
	 * by the DMA when the driver issues internal commands.
	 */
	dma_addr_t sector_buffer_dma;

	u16 *log_buf;
	dma_addr_t log_buf_dma;

	u8 *smart_buf;
	dma_addr_t smart_buf_dma;

	/*
	 * used to queue commands when an internal command is in progress
	 * or error handling is active
	 */
	unsigned long cmds_to_issue[SLOTBITS_IN_LONGS];
	/* Used by mtip_service_thread to wait for an event */
	wait_queue_head_t svc_wait;
	/*
	 * indicates the state of the port. Also, helps the service thread
	 * to determine its action on wake up.
	 */
	unsigned long flags;
	/*
	 * Timer used to complete commands that have been active for too long.
	 */
	unsigned long ic_pause_timer;

	/* Semaphore to control queue depth of unaligned IOs */
	struct semaphore cmd_slot_unal;

	/* Spinlock for working around command-issue bug. */
	spinlock_t cmd_issue_lock[MTIP_MAX_SLOT_GROUPS];
};

/*
 * Driver private data structure.
 *
 * One structure is allocated per probed device.
 */
struct driver_data {
	void __iomem *mmio; /* Base address of the HBA registers. */

	int major; /* Major device number. */

	int instance; /* Instance number. First device probed is 0, ... */

	struct gendisk *disk; /* Pointer to our gendisk structure. */

	struct pci_dev *pdev; /* Pointer to the PCI device structure. */

	struct request_queue *queue; /* Our request queue. */

	struct blk_mq_tag_set tags; /* blk_mq tags */

	struct mtip_port *port; /* Pointer to the port data structure. */

	unsigned product_type; /* magic value declaring the product type */

	unsigned slot_groups; /* number of slot groups the product supports */

	unsigned long index; /* Index to determine the disk name */

	unsigned long dd_flag; /* NOTE: use atomic bit operations on this */

	struct task_struct *mtip_svc_handler; /* task_struct of svc thd */

	struct dentry *dfs_node;

	bool trim_supp; /* flag indicating trim support */

	bool sr;

	int numa_node; /* NUMA support */

	char workq_name[32];

	struct workqueue_struct *isr_workq;

	atomic_t irq_workers_active;

	struct mtip_work work[MTIP_MAX_SLOT_GROUPS];

	int isr_binding;

	struct block_device *bdev;

	struct list_head online_list; /* linkage for online list */

	struct list_head remove_list; /* linkage for removing list */

	int unal_qdepth; /* qdepth of unaligned IO queue */
};

#endif
                                                                                                                                                                                                                                                                         