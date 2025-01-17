

		/*
		 * Release a claim on the device.  The holder fields
		 * are protected with bdev_lock.  bd_mutex is to
		 * synchronize disk_holder unlinking.
		 */
		spin_lock(&bdev_lock);

		WARN_ON_ONCE(--bdev->bd_holders < 0);
		WARN_ON_ONCE(--bdev->bd_contains->bd_holders < 0);

		/* bd_contains might point to self, check in a separate step */
		if ((bdev_free = !bdev->bd_holders))
			bdev->bd_holder = NULL;
		if (!bdev->bd_contains->bd_holders)
			bdev->bd_contains->bd_holder = NULL;

		spin_unlock(&bdev_lock);

		/*
		 * If this was the last claim, remove holder link and
		 * unblock evpoll if it was a write holder.
		 */
		if (bdev_free && bdev->bd_write_holder) {
			disk_unblock_events(bdev->bd_disk);
			bdev->bd_write_holder = false;
		}
	}

	/*
	 * Trigger event checking and tell drivers to flush MEDIA_CHANGE
	 * event.  This is to ensure detection of media removal commanded
	 * from userland - e.g. eject(1).
	 */
	disk_flush_events(bdev->bd_disk, DISK_EVENT_MEDIA_CHANGE);

	mutex_unlock(&bdev->bd_mutex);

	__blkdev_put(bdev, mode, 0);
}
EXPORT_SYMBOL(blkdev_put);

static int blkdev_close(struct inode * inode, struct file * filp)
{
	struct block_device *bdev = I_BDEV(filp->f_mapping->host);
	blkdev_put(bdev, filp->f_mode);
	return 0;
}

static long block_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	struct block_device *bdev = I_BDEV(file->f_mapping->host);
	fmode_t mode = file->f_mode;

	/*
	 * O_NDELAY can be altered using fcntl(.., F_SETFL, ..), so we have
	 * to updated it before every ioctl.
	 */
	if (file->f_flags & O_NDELAY)
		mode |= FMODE_NDELAY;
	else
		mode &= ~FMODE_NDELAY;

	return blkdev_ioctl(bdev, mode, cmd, arg);
}

/*
 * Write data to the block device.  Only intended for the block device itself
 * and the raw driver which basically is a fake block device.
 *
 * Does not take i_mutex for the write and thus is not for general purpose
 * use.
 */
ssize_t blkdev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	struct file *file = iocb->ki_filp;
	struct inode *bd_inode = file->f_mapping->host;
	loff_t size = i_size_read(bd_inode);
	struct blk_plug plug;
	ssize_t ret;

	if (bdev_read_only(I_BDEV(bd_inode)))
		return -EPERM;

	if (!iov_iter_count(from))
		return 0;

	if (iocb->ki_pos >= size)
		return -ENOSPC;

	iov_iter_truncate(from, size - iocb->ki_pos);

	blk_start_plug(&plug);
	ret = __generic_file_write_iter(iocb, from);
	if (ret > 0) {
		ssize_t err;
		err = generic_write_sync(file, iocb->ki_pos - ret, ret);
		if (err < 0)
			ret = err;
	}
	blk_finish_plug(&plug);
	return ret;
}
EXPORT_SYMBOL_GPL(blkdev_write_iter);

ssize_t blkdev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	struct file *file = iocb->ki_filp;
	struct inode *bd_inode = file->f_mapping->host;
	loff_t size = i_size_read(bd_inode);
	loff_t pos = iocb->ki_pos;

	if (pos >= size)
		return 0;

	size -= pos;
	iov_iter_truncate(to, size);
	return generic_file_read_iter(iocb, to);
}
EXPORT_SYMBOL_GPL(blkdev_read_iter);

/*
 * Try to release a page associated with block device when the system
 * is under memory pressure.
 */
static int blkdev_releasepage(struct page *page, gfp_t wait)
{
	struct super_block *super = BDEV_I(page->mapping->host)->bdev.bd_super;

	if (super && super->s_op->bdev_try_to_free_page)
		return super->s_op->bdev_try_to_free_page(super, page, wait);

	return try_to_free_buffers(page);
}

static const struct address_space_operations def_blk_aops = {
	.readpage	= blkdev_readpage,
	.readpages	= blkdev_readpages,
	.writepage	= blkdev_writepage,
	.write_begin	= blkdev_write_begin,
	.write_end	= blkdev_write_end,
	.writepages	= generic_writepages,
	.releasepage	= blkdev_releasepage,
	.direct_IO	= blkdev_direct_IO,
	.is_dirty_writeback = buffer_check_dirty_writeback,
};

const struct file_operations def_blk_fops = {
	.open		= blkdev_open,
	.release	= blkdev_close,
	.llseek		= block_llseek,
	.read_iter	= blkdev_read_iter,
	.write_iter	= blkdev_write_iter,
	.mmap		= generic_file_mmap,
	.fsync		= blkdev_fsync,
	.unlocked_ioctl	= block_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= compat_blkdev_ioctl,
#endif
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
};

int ioctl_by_bdev(struct block_device *bdev, unsigned cmd, unsigned long arg)
{
	int res;
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);
	res = blkdev_ioctl(bdev, 0, cmd, arg);
	set_fs(old_fs);
	return res;
}

EXPORT_SYMBOL(ioctl_by_bdev);

/**
 * lookup_bdev  - lookup a struct block_device by name
 * @pathname:	special file representing the block device
 *
 * Get a reference to the blockdevice at @pathname in the current
 * namespace if possible and return it.  Return ERR_PTR(error)
 * otherwise.
 */
struct block_device *lookup_bdev(const char *pathname)
{
	struct block_device *bdev;
	struct inode *inode;
	struct path path;
	int error;

	if (!pathname || !*pathname)
		return ERR_PTR(-EINVAL);

	error = kern_path(pathname, LOOKUP_FOLLOW, &path);
	if (error)
		return ERR_PTR(error);

	inode = d_backing_inode(path.dentry);
	error = -ENOTBLK;
	if (!S_ISBLK(inode->i_mode))
		goto fail;
	error = -EACCES;
	if (!may_open_dev(&path))
		goto fail;
	error = -ENOMEM;
	bdev = bd_acquire(inode);
	if (!bdev)
		goto fail;
out:
	path_put(&path);
	return bdev;
fail:
	bdev = ERR_PTR(error);
	goto out;
}
EXPORT_SYMBOL(lookup_bdev);

int __invalidate_device(struct block_device *bdev, bool kill_dirty)
{
	struct super_block *sb = get_super(bdev);
	int res = 0;

	if (sb) {
		/*
		 * no need to lock the super, get_super holds the
		 * read mutex so the filesystem cannot go away
		 * under us (->put_super runs with the write lock
		 * hold).
		 */
		shrink_dcache_sb(sb);
		res = invalidate_inodes(sb, kill_dirty);
		drop_super(sb);
	}
	invalidate_bdev(bdev);
	return res;
}
EXPORT_SYMBOL(__invalidate_device);

void iterate_bdevs(void (*func)(struct block_device *, void *), void *arg)
{
	struct inode *inode, *old_inode = NULL;

	spin_lock(&blockdev_superblock->s_inode_list_lock);
	list_for_each_entry(inode, &blockdev_superblock->s_inodes, i_sb_list) {
		struct address_space *mapping = inode->i_mapping;
		struct block_device *bdev;

		spin_lock(&inode->i_lock);
		if (inode->i_state & (I_FREEING|I_WILL_FREE|I_NEW) ||
		    mapping->nrpages == 0) {
			spin_unlock(&inode->i_lock);
			continue;
		}
		__iget(inode);
		spin_unlock(&inode->i_lock);
		spin_unlock(&blockdev_superblock->s_inode_list_lock);
		/*
		 * We hold a reference to 'inode' so it couldn't have been
		 * removed from s_inodes list while we dropped the
		 * s_inode_list_lock  We cannot iput the inode now as we can
		 * be holding the last reference and we cannot iput it under
		 * s_inode_list_lock. So we keep the reference and iput it
		 * later.
		 */
		iput(old_inode);
		old_inode = inode;
		bdev = I_BDEV(inode);

		mutex_lock(&bdev->bd_mutex);
		if (bdev->bd_openers)
			func(bdev, arg);
		mutex_unlock(&bdev->bd_mutex);

		spin_lock(&blockdev_superblock->s_inode_list_lock);
	}
	spin_unlock(&blockdev_superblock->s_inode_list_lock);
	iput(old_inode);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     config BTRFS_FS
	tristate "Btrfs filesystem support"
	select CRYPTO
	select CRYPTO_CRC32C
	select ZLIB_INFLATE
	select ZLIB_DEFLATE
	select LZO_COMPRESS
	select LZO_DECOMPRESS
	select RAID6_PQ
	select XOR_BLOCKS
	select SRCU
	depends on !PPC_256K_PAGES	# powerpc
	depends on !PAGE_SIZE_256KB	# hexagon

	help
	  Btrfs is a general purpose copy-on-write filesystem with extents,
	  writable snapshotting, support for multiple devices and many more
	  features focused on fault tolerance, repair and easy administration.

	  The filesystem disk format is no longer unstable, and it's not
	  expected to change unless there are strong reasons to do so. If there
	  is a format change, file systems with a unchanged format will
	  continue to be mountable and usable by newer kernels.

	  For more information, please see the web pages at
	  http://btrfs.wiki.kernel.org.

	  To compile this file system support as a module, choose M here. The
	  module will be called btrfs.

	  If unsure, say N.

config BTRFS_FS_POSIX_ACL
	bool "Btrfs POSIX Access Control Lists"
	depends on BTRFS_FS
	select FS_POSIX_ACL
	help
	  POSIX Access Control Lists (ACLs) support permissions for users and
	  groups beyond the owner/group/world scheme.

	  To learn more about Access Control Lists, visit the POSIX ACLs for
	  Linux website <http://acl.bestbits.at/>.

	  If you don't know what Access Control Lists are, say N

config BTRFS_FS_CHECK_INTEGRITY
	bool "Btrfs with integrity check tool compiled in (DANGEROUS)"
	depends on BTRFS_FS
	help
	  Adds code that examines all block write requests (including
	  writes of the super block). The goal is to verify that the
	  state of the filesystem on disk is always consistent, i.e.,
	  after a power-loss or kernel panic event the filesystem is
	  in a consistent state.

	  If the integrity check tool is included and activated in
	  the mount options, plenty of kernel memory is used, and
	  plenty of additional CPU cycles are spent. Enabling this
	  functionality is not intended for normal use.

	  In most cases, unless you are a btrfs developer who needs
	  to verify the integrity of (super)-block write requests
	  during the run of a regression test, say N

config BTRFS_FS_RUN_SANITY_TESTS
	bool "Btrfs will run sanity tests upon loading"
	depends on BTRFS_FS
	help
	  This will run some basic sanity tests on the free space cache
	  code to make sure it is acting as it should.  These are mostly
	  regression tests and are only really interesting to btrfs
	  developers.

	  If unsure, say N.

config BTRFS_DEBUG
	bool "Btrfs debugging support"
	depends on BTRFS_FS
	help
	  Enable run-time debugging support for the btrfs filesystem. This may
	  enable additional and expensive checks with negative impact on
	  performance, or export extra information via sysfs.

	  If unsure, say N.

config BTRFS_ASSERT
	bool "Btrfs assert support"
	depends on BTRFS_FS
	help
	  Enable run-time assertion checking.  This will result in panics if
	  any of the assertions trip.  This is meant for btrfs developers only.

	  If unsure, say N.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
obj-$(CONFIG_BTRFS_FS) := btrfs.o

btrfs-y += super.o ctree.o extent-tree.o print-tree.o root-tree.o dir-item.o \
	   file-item.o inode-item.o inode-map.o disk-io.o \
	   transaction.o inode.o file.o tree-defrag.o \
	   extent_map.o sysfs.o struct-funcs.o xattr.o ordered-data.o \
	   extent_io.o volumes.o async-thread.o ioctl.o locking.o orphan.o \
	   export.o tree-log.o free-space-cache.o zlib.o lzo.o \
	   compression.o delayed-ref.o relocation.o delayed-inode.o scrub.o \
	   reada.o backref.o ulist.o qgroup.o send.o dev-replace.o raid56.o \
	   uuid-tree.o props.o hash.o tree-checker.o

btrfs-$(CONFIG_BTRFS_FS_POSIX_ACL) += acl.o
btrfs-$(CONFIG_BTRFS_FS_CHECK_INTEGRITY) += check-integrity.o

btrfs-$(CONFIG_BTRFS_FS_RUN_SANITY_TESTS) += tests/free-space-tests.o \
	tests/extent-buffer-tests.o tests/btrfs-tests.o \
	tests/extent-io-tests.o tests/inode-tests.o tests/qgroup-tests.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Copyright (C) 2007 Red Hat.  All rights reserved.
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
#include <linux/string.h>
#include <linux/xattr.h>
#include <linux/posix_acl_xattr.h>
#include <linux/posix_acl.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "ctree.h"
#include "btrfs_inode.h"
#include "xattr.h"

struct posix_acl *btrfs_get_acl(struct inode *inode, int type)
{
	int size;
	const char *name;
	char *value = NULL;
	struct posix_acl *acl;

	switch (type) {
	case ACL_TYPE_ACCESS:
		name = POSIX_ACL_XATTR_ACCESS;
		break;
	case ACL_TYPE_DEFAULT:
		name = POSIX_ACL_XATTR_DEFAULT;
		break;
	default:
		BUG();
	}

	size = __btrfs_getxattr(inode, name, "", 0);
	if (size > 0) {
		value = kzalloc(size, GFP_NOFS);
		if (!value)
			return ERR_PTR(-ENOMEM);
		size = __btrfs_getxattr(inode, name, value, size);
	}
	if (size > 0) {
		acl = posix_acl_from_xattr(&init_user_ns, value, size);
	} else if (size == -ENOENT || size == -ENODATA || size == 0) {
		/* FIXME, who returns -ENOENT?  I think nobody */
		acl = NULL;
	} else {
		acl = ERR_PTR(-EIO);
	}
	kfree(value);

	if (!IS_ERR(acl))
		set_cached_acl(inode, type, acl);

	return acl;
}

/*
 * Needs to be called with fs_mutex held
 */
static int __btrfs_set_acl(struct btrfs_trans_handle *trans,
			 struct inode *inode, struct posix_acl *acl, int type)
{
	int ret, size = 0;
	const char *name;
	char *value = NULL;

	switch (type) {
	case ACL_TYPE_ACCESS:
		name = POSIX_ACL_XATTR_ACCESS;
		break;
	case ACL_TYPE_DEFAULT:
		if (!S_ISDIR(inode->i_mode))
			return acl ? -EINVAL : 0;
		name = POSIX_ACL_XATTR_DEFAULT;
		break;
	default:
		return -EINVAL;
	}

	if (acl) {
		size = posix_acl_xattr_size(acl->a_count);
		value = kmalloc(size, GFP_NOFS);
		if (!value) {
			ret = -ENOMEM;
			goto out;
		}

		ret = posix_acl_to_xattr(&init_user_ns, acl, value, size);
		if (ret < 0)
			goto out;
	}

	ret = __btrfs_setxattr(trans, inode, name, value, size, 0);
out:
	kfree(value);

	if (!ret)
		set_cached_acl(inode, type, acl);

	return ret;
}

int btrfs_set_acl(struct inode *inode, struct posix_acl *acl, int type)
{
	int ret;
	umode_t old_mode = inode->i_mode;

	if (type == ACL_TYPE_ACCESS && acl) {
		ret = posix_acl_update_mode(inode, &inode->i_mode, &acl);
		if (ret)
			return ret;
	}
	ret = __btrfs_set_acl(NULL, inode, acl, type);
	if (ret)
		inode->i_mode = old_mode;
	return ret;
}

/*
 * btrfs_init_acl is already generally called under fs_mutex, so the locking
 * stuff has been fixed to work with that.  If the locking stuff changes, we
 * need to re-evaluate the acl locking stuff.
 */
int btrfs_init_acl(struct btrfs_trans_handle *trans,
		   struct inode *inode, struct inode *dir)
{
	struct posix_acl *default_acl, *acl;
	int ret = 0;

	/* this happens with subvols */
	if (!dir)
		return 0;

	ret = posix_acl_create(dir, &inode->i_mode, &default_acl, &acl);
	if (ret)
		return ret;

	if (default_acl) {
		ret = __btrfs_set_acl(trans, inode, default_acl,
				      ACL_TYPE_DEFAULT);
		posix_acl_release(default_acl);
	}

	if (acl) {
		if (!ret)
			ret = __btrfs_set_acl(trans, inode, acl,
					      ACL_TYPE_ACCESS);
		posix_acl_release(acl);
	}

	if (!default_acl && !acl)
		cache_no_acl(inode);
	return ret;
}
                                                                                                                                                                                                                                                               /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 * Copyright (C) 2014 Fujitsu.  All rights reserved.
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

#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/freezer.h>
#include "async-thread.h"
#include "ctree.h"

#define WORK_DONE_BIT 0
#define WORK_ORDER_DONE_BIT 1
#define WORK_HIGH_PRIO_BIT 2

#define NO_THRESHOLD (-1)
#define DFT_THRESHOLD (32)

struct __btrfs_workqueue {
	struct workqueue_struct *normal_wq;
	/* List head pointing to ordered work list */
	struct list_head ordered_list;

	/* Spinlock for ordered_list */
	spinlock_t list_lock;

	/* Thresholding related variants */
	atomic_t pending;

	/* Up limit of concurrency workers */
	int limit_active;

	/* Current number of concurrency workers */
	int current_active;

	/* Threshold to change current_active */
	int thresh;
	unsigned int count;
	spinlock_t thres_lock;
};

struct btrfs_workqueue {
	struct __btrfs_workqueue *normal;
	struct __btrfs_workqueue *high;
};

static void normal_work_helper(struct btrfs_work *work);

#define BTRFS_WORK_HELPER(name)					\
void btrfs_##name(struct work_struct *arg)				\
{									\
	struct btrfs_work *work = container_of(arg, struct btrfs_work,	\
					       normal_work);		\
	normal_work_helper(work);					\
}

bool btrfs_workqueue_normal_congested(struct btrfs_workqueue *wq)
{
	/*
	 * We could compare wq->normal->pending with num_online_cpus()
	 * to support "thresh == NO_THRESHOLD" case, but it requires
	 * moving up atomic_inc/dec in thresh_queue/exec_hook. Let's
	 * postpone it until someone needs the support of that case.
	 */
	if (wq->normal->thresh == NO_THRESHOLD)
		return false;

	return atomic_read(&wq->normal->pending) > wq->normal->thresh * 2;
}

BTRFS_WORK_HELPER(worker_helper);
BTRFS_WORK_HELPER(delalloc_helper);
BTRFS_WORK_HELPER(flush_delalloc_helper);
BTRFS_WORK_HELPER(cache_helper);
BTRFS_WORK_HELPER(submit_helper);
BTRFS_WORK_HELPER(fixup_helper);
BTRFS_WORK_HELPER(endio_helper);
BTRFS_WORK_HELPER(endio_meta_helper);
BTRFS_WORK_HELPER(endio_meta_write_helper);
BTRFS_WORK_HELPER(endio_raid56_helper);
BTRFS_WORK_HELPER(endio_repair_helper);
BTRFS_WORK_HELPER(rmw_helper);
BTRFS_WORK_HELPER(endio_write_helper);
BTRFS_WORK_HELPER(freespace_write_helper);
BTRFS_WORK_HELPER(delayed_meta_helper);
BTRFS_WORK_HELPER(readahead_helper);
BTRFS_WORK_HELPER(qgroup_rescan_helper);
BTRFS_WORK_HELPER(extent_refs_helper);
BTRFS_WORK_HELPER(scrub_helper);
BTRFS_WORK_HELPER(scrubwrc_helper);
BTRFS_WORK_HELPER(scrubnc_helper);
BTRFS_WORK_HELPER(scrubparity_helper);

static struct __btrfs_workqueue *
__btrfs_alloc_workqueue(const char *name, unsigned int flags, int limit_active,
			 int thresh)
{
	struct __btrfs_workqueue *ret = kzalloc(sizeof(*ret), GFP_NOFS);

	if (!ret)
		return NULL;

	ret->limit_active = limit_active;
	atomic_set(&ret->pending, 0);
	if (thresh == 0)
		thresh = DFT_THRESHOLD;
	/* For low threshold, disabling threshold is a better choice */
	if (thresh < DFT_THRESHOLD) {
		ret->current_active = limit_active;
		ret->thresh = NO_THRESHOLD;
	} else {
		/*
		 * For threshold-able wq, let its concurrency grow on demand.
		 * Use minimal max_active at alloc time to reduce resource
		 * usage.
		 */
		ret->current_active = 1;
		ret->thresh = thresh;
	}

	if (flags & WQ_HIGHPRI)
		ret->normal_wq = alloc_workqueue("%s-%s-high", flags,
						 ret->current_active, "btrfs",
						 name);
	else
		ret->normal_wq = alloc_workqueue("%s-%s", flags,
						 ret->current_active, "btrfs",
						 name);
	if (!ret->normal_wq) {
		kfree(ret);
		return NULL;
	}

	INIT_LIST_HEAD(&ret->ordered_list);
	spin_lock_init(&ret->list_lock);
	spin_lock_init(&ret->thres_lock);
	trace_btrfs_workqueue_alloc(ret, name, flags & WQ_HIGHPRI);
	return ret;
}

static inline void
__btrfs_destroy_workqueue(struct __btrfs_workqueue *wq);

struct btrfs_workqueue *btrfs_alloc_workqueue(const char *name,
					      unsigned int flags,
					      int limit_active,
					      int thresh)
{
	struct btrfs_workqueue *ret = kzalloc(sizeof(*ret), GFP_NOFS);

	if (!ret)
		return NULL;

	ret->normal = __btrfs_alloc_workqueue(name, flags & ~WQ_HIGHPRI,
					      limit_active, thresh);
	if (!ret->normal) {
		kfree(ret);
		return NULL;
	}

	if (flags & WQ_HIGHPRI) {
		ret->high = __btrfs_alloc_workqueue(name, flags, limit_active,
						    thresh);
		if (!ret->high) {
			__btrfs_destroy_workqueue(ret->normal);
			kfree(ret);
			return NULL;
		}
	}
	return ret;
}

/*
 * Hook for threshold which will be called in btrfs_queue_work.
 * This hook WILL be called in IRQ handler context,
 * so workqueue_set_max_active MUST NOT be called in this hook
 */
static inline void thresh_queue_hook(struct __btrfs_workqueue *wq)
{
	if (wq->thresh == NO_THRESHOLD)
		return;
	atomic_inc(&wq->pending);
}

/*
 * Hook for threshold which will be called before executing the work,
 * This hook is called in kthread content.
 * So workqueue_set_max_active is called here.
 */
static inline void thresh_exec_hook(struct __btrfs_workqueue *wq)
{
	int new_current_active;
	long pending;
	int need_change = 0;

	if (wq->thresh == NO_THRESHOLD)
		return;

	atomic_dec(&wq->pending);
	spin_lock(&wq->thres_lock);
	/*
	 * Use wq->count to limit the calling frequency of
	 * workqueue_set_max_active.
	 */
	wq->count++;
	wq->count %= (wq->thresh / 4);
	if (!wq->count)
		goto  out;
	new_current_active = wq->current_active;

	/*
	 * pending may be changed later, but it's OK since we really
	 * don't need it so accurate to calculate new_max_active.
	 */
	pending = atomic_read(&wq->pending);
	if (pending > wq->thresh)
		new_current_active++;
	if (pending < wq->thresh / 2)
		new_current_active--;
	new_current_active = clamp_val(new_current_active, 1, wq->limit_active);
	if (new_current_active != wq->current_active)  {
		need_change = 1;
		wq->current_active = new_current_active;
	}
out:
	spin_unlock(&wq->thres_lock);

	if (need_change) {
		workqueue_set_max_active(wq->normal_wq, wq->current_active);
	}
}

static void run_ordered_work(struct __btrfs_workqueue *wq)
{
	struct list_head *list = &wq->ordered_list;
	struct btrfs_work *work;
	spinlock_t *lock = &wq->list_lock;
	unsigned long flags;

	while (1) {
		spin_lock_irqsave(lock, flags);
		if (list_empty(list))
			break;
		work = list_entry(list->next, struct btrfs_work,
				  ordered_list);
		if (!test_bit(WORK_DONE_BIT, &work->flags))
			break;
		/*
		 * Orders all subsequent loads after reading WORK_DONE_BIT,
		 * paired with the smp_mb__before_atomic in btrfs_work_helper
		 * this guarantees that the ordered function will see all
		 * updates from ordinary work function.
		 */
		smp_rmb();

		/*
		 * we are going to call the ordered done function, but
		 * we leave the work item on the list as a barrier so
		 * that later work items that are done don't have their
		 * functions called before this one returns
		 */
		if (test_and_set_bit(WORK_ORDER_DONE_BIT, &work->flags))
			break;
		trace_btrfs_ordered_sched(work);
		spin_unlock_irqrestore(lock, flags);
		work->ordered_func(work);

		/* now take the lock again and drop our item from the list */
		spin_lock_irqsave(lock, flags);
		list_del(&work->ordered_list);
		spin_unlock_irqrestore(lock, flags);

		/*
		 * we don't want to call the ordered free functions
		 * with the lock held though
		 */
		work->ordered_free(work);
		trace_btrfs_all_work_done(work);
	}
	spin_unlock_irqrestore(lock, flags);
}

static void normal_work_helper(struct btrfs_work *work)
{
	struct __btrfs_workqueue *wq;
	int need_order = 0;

	/*
	 * We should not touch things inside work in the following cases:
	 * 1) after work->func() if it has no ordered_free
	 *    Since the struct is freed in work->func().
	 * 2) after setting WORK_DONE_BIT
	 *    The work may be freed in other threads almost instantly.
	 * So we save the needed things here.
	 */
	if (work->ordered_func)
		need_order = 1;
	wq = work->wq;

	trace_btrfs_work_sched(work);
	thresh_exec_hook(wq);
	work->func(work);
	if (need_order) {
		/*
		 * Ensures all memory accesses done in the work function are
		 * ordered before setting the WORK_DONE_BIT. Ensuring the thread
		 * which is going to executed the ordered work sees them.
		 * Pairs with the smp_rmb in run_ordered_work.
		 */
		smp_mb__before_atomic();
		set_bit(WORK_DONE_BIT, &work->flags);
		run_ordered_work(wq);
	}
	if (!need_order)
		trace_btrfs_all_work_done(work);
}

void btrfs_init_work(struct btrfs_work *work, btrfs_work_func_t uniq_func,
		     btrfs_func_t func,
		     btrfs_func_t ordered_func,
		     btrfs_func_t ordered_free)
{
	work->func = func;
	work->ordered_func = ordered_func;
	work->ordered_free = ordered_free;
	INIT_WORK(&work->normal_work, uniq_func);
	INIT_LIST_HEAD(&work->ordered_list);
	work->flags = 0;
}

static inline void __btrfs_queue_work(struct __btrfs_workqueue *wq,
				      struct btrfs_work *work)
{
	unsigned long flags;

	work->wq = wq;
	thresh_queue_hook(wq);
	if (work->ordered_func) {
		spin_lock_irqsave(&wq->list_lock, flags);
		list_add_tail(&work->ordered_list, &wq->ordered_list);
		spin_unlock_irqrestore(&wq->list_lock, flags);
	}
	trace_btrfs_work_queued(work);
	queue_work(wq->normal_wq, &work->normal_work);
}

void btrfs_queue_work(struct btrfs_workqueue *wq,
		      struct btrfs_work *work)
{
	struct __btrfs_workqueue *dest_wq;

	if (test_bit(WORK_HIGH_PRIO_BIT, &work->flags) && wq->high)
		dest_wq = wq->high;
	else
		dest_wq = wq->normal;
	__btrfs_queue_work(dest_wq, work);
}

static inline void
__btrfs_destroy_workqueue(struct __btrfs_workqueue *wq)
{
	destroy_workqueue(wq->normal_wq);
	trace_btrfs_workqueue_destroy(wq);
	kfree(wq);
}

void btrfs_destroy_workqueue(struct btrfs_workqueue *wq)
{
	if (!wq)
		return;
	if (wq->high)
		__btrfs_destroy_workqueue(wq->high);
	__btrfs_destroy_workqueue(wq->normal);
	kfree(wq);
}

void btrfs_workqueue_set_max(struct btrfs_workqueue *wq, int limit_active)
{
	if (!wq)
		return;
	wq->normal->limit_active = limit_active;
	if (wq->high)
		wq->high->limit_active = limit_active;
}

void btrfs_set_work_high_priority(struct btrfs_work *work)
{
	set_bit(WORK_HIGH_PRIO_BIT, &work->flags);
}

void btrfs_flush_workqueue(struct btrfs_workqueue *wq)
{
	if (wq->high)
		flush_workqueue(wq->high->normal_wq);

	flush_workqueue(wq->normal->normal_wq);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 * Copyright (C) 2014 Fujitsu.  All rights reserved.
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

#ifndef __BTRFS_ASYNC_THREAD_
#define __BTRFS_ASYNC_THREAD_
#include <linux/workqueue.h>

struct btrfs_workqueue;
/* Internal use only */
struct __btrfs_workqueue;
struct btrfs_work;
typedef void (*btrfs_func_t)(struct btrfs_work *arg);
typedef void (*btrfs_work_func_t)(struct work_struct *arg);

struct btrfs_work {
	btrfs_func_t func;
	btrfs_func_t ordered_func;
	btrfs_func_t ordered_free;

	/* Don't touch things below */
	struct work_struct normal_work;
	struct list_head ordered_list;
	struct __btrfs_workqueue *wq;
	unsigned long flags;
};

#define BTRFS_WORK_HELPER_PROTO(name)					\
void btrfs_##name(struct work_struct *arg)

BTRFS_WORK_HELPER_PROTO(worker_helper);
BTRFS_WORK_HELPER_PROTO(delalloc_helper);
BTRFS_WORK_HELPER_PROTO(flush_delalloc_helper);
BTRFS_WORK_HELPER_PROTO(cache_helper);
BTRFS_WORK_HELPER_PROTO(submit_helper);
BTRFS_WORK_HELPER_PROTO(fixup_helper);
BTRFS_WORK_HELPER_PROTO(endio_helper);
BTRFS_WORK_HELPER_PROTO(endio_meta_helper);
BTRFS_WORK_HELPER_PROTO(endio_meta_write_helper);
BTRFS_WORK_HELPER_PROTO(endio_raid56_helper);
BTRFS_WORK_HELPER_PROTO(endio_repair_helper);
BTRFS_WORK_HELPER_PROTO(rmw_helper);
BTRFS_WORK_HELPER_PROTO(endio_write_helper);
BTRFS_WORK_HELPER_PROTO(freespace_write_helper);
BTRFS_WORK_HELPER_PROTO(delayed_meta_helper);
BTRFS_WORK_HELPER_PROTO(readahead_helper);
BTRFS_WORK_HELPER_PROTO(qgroup_rescan_helper);
BTRFS_WORK_HELPER_PROTO(extent_refs_helper);
BTRFS_WORK_HELPER_PROTO(scrub_helper);
BTRFS_WORK_HELPER_PROTO(scrubwrc_helper);
BTRFS_WORK_HELPER_PROTO(scrubnc_helper);
BTRFS_WORK_HELPER_PROTO(scrubparity_helper);


struct btrfs_workqueue *btrfs_alloc_workqueue(const char *name,
					      unsigned int flags,
					      int limit_active,
					      int thresh);
void btrfs_init_work(struct btrfs_work *work, btrfs_work_func_t helper,
		     btrfs_func_t func,
		     btrfs_func_t ordered_func,
		     btrfs_func_t ordered_free);
void btrfs_queue_work(struct btrfs_workqueue *wq,
		      struct btrfs_work *work);
void btrfs_destroy_workqueue(struct btrfs_workqueue *wq);
void btrfs_workqueue_set_max(struct btrfs_workqueue *wq, int max);
void btrfs_set_work_high_priority(struct btrfs_work *work);
bool btrfs_workqueue_normal_congested(struct btrfs_workqueue *wq);
void btrfs_flush_workqueue(struct btrfs_workqueue *wq);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * Copyright (C) 2011 STRATO.  All rights reserved.
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

#include <linux/vmalloc.h>
#include "ctree.h"
#include "disk-io.h"
#include "backref.h"
#include "ulist.h"
#include "transaction.h"
#include "delayed-ref.h"
#include "locking.h"

/* Just an arbitrary number so we can be sure this happened */
#define BACKREF_FOUND_SHARED 6

struct extent_inode_elem {
	u64 inum;
	u64 offset;
	struct extent_inode_elem *next;
};

static int check_extent_in_eb(struct btrfs_key *key, struct extent_buffer *eb,
				struct btrfs_file_extent_item *fi,
				u64 extent_item_pos,
				struct extent_inode_elem **eie)
{
	u64 offset = 0;
	struct extent_inode_elem *e;

	if (!btrfs_file_extent_compression(eb, fi) &&
	    !btrfs_file_extent_encryption(eb, fi) &&
	    !btrfs_file_extent_other_encoding(eb, fi)) {
		u64 data_offset;
		u64 data_len;

		data_offset = btrfs_file_extent_offset(eb, fi);
		data_len = btrfs_file_extent_num_bytes(eb, fi);

		if (extent_item_pos < data_offset ||
		    extent_item_pos >= data_offset + data_len)
			return 1;
		offset = extent_item_pos - data_offset;
	}

	e = kmalloc(sizeof(*e), GFP_NOFS);
	if (!e)
		return -ENOMEM;

	e->next = *eie;
	e->inum = key->objectid;
	e->offset = key->offset + offset;
	*eie = e;

	return 0;
}

static void free_inode_elem_list(struct extent_inode_elem *eie)
{
	struct extent_inode_elem *eie_next;

	for (; eie; eie = eie_next) {
		eie_next = eie->next;
		kfree(eie);
	}
}

static int find_extent_in_eb(struct extent_buffer *eb, u64 wanted_disk_byte,
				u64 extent_item_pos,
				struct extent_inode_elem **eie)
{
	u64 disk_byte;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	int slot;
	int nritems;
	int extent_type;
	int ret;

	/*
	 * from the shared data ref, we only have the leaf but we need
	 * the key. thus, we must look into all items and see that we
	 * find one (some) with a reference to our extent item.
	 */
	nritems = btrfs_header_nritems(eb);
	for (slot = 0; slot < nritems; ++slot) {
		btrfs_item_key_to_cpu(eb, &key, slot);
		if (key.type != BTRFS_EXTENT_DATA_KEY)
			continue;
		fi = btrfs_item_ptr(eb, slot, struct btrfs_file_extent_item);
		extent_type = btrfs_file_extent_type(eb, fi);
		if (extent_type == BTRFS_FILE_EXTENT_INLINE)
			continue;
		/* don't skip BTRFS_FILE_EXTENT_PREALLOC, we can handle that */
		disk_byte = btrfs_file_extent_disk_bytenr(eb, fi);
		if (disk_byte != wanted_disk_byte)
			continue;

		ret = check_extent_in_eb(&key, eb, fi, extent_item_pos, eie);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/*
 * this structure records all encountered refs on the way up to the root
 */
struct __prelim_ref {
	struct list_head list;
	u64 root_id;
	struct btrfs_key key_for_search;
	int level;
	int count;
	struct extent_inode_elem *inode_list;
	u64 parent;
	u64 wanted_disk_byte;
};

static struct kmem_cache *btrfs_prelim_ref_cache;

int __init btrfs_prelim_ref_init(void)
{
	btrfs_prelim_ref_cache = kmem_cache_create("btrfs_prelim_ref",
					sizeof(struct __prelim_ref),
					0,
					SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD,
					NULL);
	if (!btrfs_prelim_ref_cache)
		return -ENOMEM;
	return 0;
}

void btrfs_prelim_ref_exit(void)
{
	if (btrfs_prelim_ref_cache)
		kmem_cache_destroy(btrfs_prelim_ref_cache);
}

/*
 * the rules for all callers of this function are:
 * - obtaining the parent is the goal
 * - if you add a key, you must know that it is a correct key
 * - if you cannot add the parent or a correct key, then we will look into the
 *   block later to set a correct key
 *
 * delayed refs
 * ============
 *        backref type | shared | indirect | shared | indirect
 * information         |   tree |     tree |   data |     data
 * --------------------+--------+----------+--------+----------
 *      parent logical |    y   |     -    |    -   |     -
 *      key to resolve |    -   |     y    |    y   |     y
 *  tree block logical |    -   |     -    |    -   |     -
 *  root for resolving |    y   |     y    |    y   |     y
 *
 * - column 1:       we've the parent -> done
 * - column 2, 3, 4: we use the key to find the parent
 *
 * on disk refs (inline or keyed)
 * ==============================
 *        backref type | shared | indirect | shared | indirect
 * information         |   tree |     tree |   data |     data
 * --------------------+--------+----------+--------+----------
 *      parent logical |    y   |     -    |    y   |     -
 *      key to resolve |    -   |     -    |    -   |     y
 *  tree block logical |    y   |     y    |    y   |     y
 *  root for resolving |    -   |     y    |    y   |     y
 *
 * - column 1, 3: we've the parent -> done
 * - column 2:    we take the first key from the block to find the parent
 *                (see __add_missing_keys)
 * - column 4:    we use the key to find the parent
 *
 * additional information that's available but not required to find the parent
 * block might help in merging entries to gain some speed.
 */

static int __add_prelim_ref(struct list_head *head, u64 root_id,
			    struct btrfs_key *key, int level,
			    u64 parent, u64 wanted_disk_byte, int count,
			    gfp_t gfp_mask)
{
	struct __prelim_ref *ref;

	if (root_id == BTRFS_DATA_RELOC_TREE_OBJECTID)
		return 0;

	ref = kmem_cache_alloc(btrfs_prelim_ref_cache, gfp_mask);
	if (!ref)
		return -ENOMEM;

	ref->root_id = root_id;
	if (key) {
		ref->key_for_search = *key;
		/*
		 * We can often find data backrefs with an offset that is too
		 * large (>= LLONG_MAX, maximum allowed file offset) due to
		 * underflows when subtracting a file's offset with the data
		 * offset of its corresponding extent data item. This can
		 * happen for example in the clone ioctl.
		 * So if we detect such case we set the search key's offset to
		 * zero to make sure we will find the matching file extent item
		 * at add_all_parents(), otherwise we will miss it because the
		 * offset taken form the backref is much larger then the offset
		 * of the file extent item. This can make us scan a very large
		 * number of file extent items, but at least it will not make
		 * us miss any.
		 * This is an ugly workaround for a behaviour that should have
		 * never existed, but it does and a fix for the clone ioctl
		 * would touch a lot of places, cause backwards incompatibility
		 * and would not fix the problem for extents cloned with older
		 * kernels.
		 */
		if (ref->key_for_search.type == BTRFS_EXTENT_DATA_KEY &&
		    ref->key_for_search.offset >= LLONG_MAX)
			ref->key_for_search.offset = 0;
	} else {
		memset(&ref->key_for_search, 0, sizeof(ref->key_for_search));
	}

	ref->inode_list = NULL;
	ref->level = level;
	ref->count = count;
	ref->parent = parent;
	ref->wanted_disk_byte = wanted_disk_byte;
	list_add_tail(&ref->list, head);

	return 0;
}

static int add_all_parents(struct btrfs_root *root, struct btrfs_path *path,
			   struct ulist *parents, struct __prelim_ref *ref,
			   int level, u64 time_seq, const u64 *extent_item_pos,
			   u64 total_refs)
{
	int ret = 0;
	int slot;
	struct extent_buffer *eb;
	struct btrfs_key key;
	struct btrfs_key *key_for_search = &ref->key_for_search;
	struct btrfs_file_extent_item *fi;
	struct extent_inode_elem *eie = NULL, *old = NULL;
	u64 disk_byte;
	u64 wanted_disk_byte = ref->wanted_disk_byte;
	u64 count = 0;

	if (level != 0) {
		eb = path->nodes[level];
		ret = ulist_add(parents, eb->start, 0, GFP_NOFS);
		if (ret < 0)
			return ret;
		return 0;
	}

	/*
	 * We normally enter this function with the path already pointing to
	 * the first item to check. But sometimes, we may enter it with
	 * slot==nritems. In that case, go to the next leaf before we continue.
	 */
	if (path->slots[0] >= btrfs_header_nritems(path->nodes[0])) {
		if (time_seq == (u64)-1)
			ret = btrfs_next_leaf(root, path);
		else
			ret = btrfs_next_old_leaf(root, path, time_seq);
	}

	while (!ret && count < total_refs) {
		eb = path->nodes[0];
		slot = path->slots[0];

		btrfs_item_key_to_cpu(eb, &key, slot);

		if (key.objectid != key_for_search->objectid ||
		    key.type != BTRFS_EXTENT_DATA_KEY)
			break;

		fi = btrfs_item_ptr(eb, slot, struct btrfs_file_extent_item);
		disk_byte = btrfs_file_extent_disk_bytenr(eb, fi);

		if (disk_byte == wanted_disk_byte) {
			eie = NULL;
			old = NULL;
			count++;
			if (extent_item_pos) {
				ret = check_extent_in_eb(&key, eb, fi,
						*extent_item_pos,
						&eie);
				if (ret < 0)
					break;
			}
			if (ret > 0)
				goto next;
			ret = ulist_add_merge_ptr(parents, eb->start,
						  eie, (void **)&old, GFP_NOFS);
			if (ret < 0)
				break;
			if (!ret && extent_item_pos) {
				while (old->next)
					old = old->next;
				old->next = eie;
			}
			eie = NULL;
		}
next:
		if (time_seq == (u64)-1)
			ret = btrfs_next_item(root, path);
		else
			ret = btrfs_next_old_item(root, path, time_seq);
	}

	if (ret > 0)
		ret = 0;
	else if (ret < 0)
		free_inode_elem_list(eie);
	return ret;
}

/*
 * resolve an indirect backref in the form (root_id, key, level)
 * to a logical address
 */
static int __resolve_indirect_ref(struct btrfs_fs_info *fs_info,
				  struct btrfs_path *path, u64 time_seq,
				  struct __prelim_ref *ref,
				  struct ulist *parents,
				  const u64 *extent_item_pos, u64 total_refs)
{
	struct btrfs_root *root;
	struct btrfs_key root_key;
	struct extent_buffer *eb;
	int ret = 0;
	int root_level;
	int level = ref->level;
	int index;

	root_key.objectid = ref->root_id;
	root_key.type = BTRFS_ROOT_ITEM_KEY;
	root_key.offset = (u64)-1;

	index = srcu_read_lock(&fs_info->subvol_srcu);

	root = btrfs_get_fs_root(fs_info, &root_key, false);
	if (IS_ERR(root)) {
		srcu_read_unlock(&fs_info->subvol_srcu, index);
		ret = PTR_ERR(root);
		goto out;
	}

	if (btrfs_test_is_dummy_root(root)) {
		srcu_read_unlock(&fs_info->subvol_srcu, index);
		ret = -ENOENT;
		goto out;
	}

	if (path->search_commit_root)
		root_level = btrfs_header_level(root->commit_root);
	else if (time_seq == (u64)-1)
		root_level = btrfs_header_level(root->node);
	else
		root_level = btrfs_old_root_level(root, time_seq);

	if (root_level + 1 == level) {
		srcu_read_unlock(&fs_info->subvol_srcu, index);
		goto out;
	}

	path->lowest_level = level;
	if (time_seq == (u64)-1)
		ret = btrfs_search_slot(NULL, root, &ref->key_for_search, path,
					0, 0);
	else
		ret = btrfs_search_old_slot(root, &ref->key_for_search, path,
					    time_seq);

	/* root node has been locked, we can release @subvol_srcu safely here */
	srcu_read_unlock(&fs_info->subvol_srcu, index);

	pr_debug("search slot in root %llu (level %d, ref count %d) returned "
		 "%d for key (%llu %u %llu)\n",
		 ref->root_id, level, ref->count, ret,
		 ref->key_for_search.objectid, ref->key_for_search.type,
		 ref->key_for_search.offset);
	if (ret < 0)
		goto out;

	eb = path->nodes[level];
	while (!eb) {
		if (WARN_ON(!level)) {
			ret = 1;
			goto out;
		}
		level--;
		eb = path->nodes[level];
	}

	ret = add_all_parents(root, path, parents, ref, level, time_seq,
			      extent_item_pos, total_refs);
out:
	path->lowest_level = 0;
	btrfs_release_path(path);
	return ret;
}

/*
 * resolve all indirect backrefs from the list
 */
static int __resolve_indirect_refs(struct btrfs_fs_info *fs_info,
				   struct btrfs_path *path, u64 time_seq,
				   struct list_head *head,
				   const u64 *extent_item_pos, u64 total_refs,
				   u64 root_objectid)
{
	int err;
	int ret = 0;
	struct __prelim_ref *ref;
	struct __prelim_ref *ref_safe;
	struct __prelim_ref *new_ref;
	struct ulist *parents;
	struct ulist_node *node;
	struct ulist_iterator uiter;

	parents = ulist_alloc(GFP_NOFS);
	if (!parents)
		return -ENOMEM;

	/*
	 * _safe allows us to insert directly after the current item without
	 * iterating over the newly inserted items.
	 * we're also allowed to re-assign ref during iteration.
	 */
	list_for_each_entry_safe(ref, ref_safe, head, list) {
		if (ref->parent)	/* already direct */
			continue;
		if (ref->count == 0)
			continue;
		if (root_objectid && ref->root_id != root_objectid) {
			ret = BACKREF_FOUND_SHARED;
			goto out;
		}
		err = __resolve_indirect_ref(fs_info, path, time_seq, ref,
					     parents, extent_item_pos,
					     total_refs);
		/*
		 * we can only tolerate ENOENT,otherwise,we should catch error
		 * and return directly.
		 */
		if (err == -ENOENT) {
			continue;
		} else if (err) {
			ret = err;
			goto out;
		}

		/* we put the first parent into the ref at hand */
		ULIST_ITER_INIT(&uiter);
		node = ulist_next(parents, &uiter);
		ref->parent = node ? node->val : 0;
		ref->inode_list = node ?
			(struct extent_inode_elem *)(uintptr_t)node->aux : NULL;

		/* additional parents require new refs being added here */
		while ((node = ulist_next(parents, &uiter))) {
			new_ref = kmem_cache_alloc(btrfs_prelim_ref_cache,
						   GFP_NOFS);
			if (!new_ref) {
				ret = -ENOMEM;
				goto out;
			}
			memcpy(new_ref, ref, sizeof(*ref));
			new_ref->parent = node->val;
			new_ref->inode_list = (struct extent_inode_elem *)
							(uintptr_t)node->aux;
			list_add(&new_ref->list, &ref->list);
		}
		ulist_reinit(parents);
	}
out:
	ulist_free(parents);
	return ret;
}

static inline int ref_for_same_block(struct __prelim_ref *ref1,
				     struct __prelim_ref *ref2)
{
	if (ref1->level != ref2->level)
		return 0;
	if (ref1->root_id != ref2->root_id)
		return 0;
	if (ref1->key_for_search.type != ref2->key_for_search.type)
		return 0;
	if (ref1->key_for_search.objectid != ref2->key_for_search.objectid)
		return 0;
	if (ref1->key_for_search.offset != ref2->key_for_search.offset)
		return 0;
	if (ref1->parent != ref2->parent)
		return 0;

	return 1;
}

/*
 * read tree blocks and add keys where required.
 */
static int __add_missing_keys(struct btrfs_fs_info *fs_info,
			      struct list_head *head)
{
	struct list_head *pos;
	struct extent_buffer *eb;

	list_for_each(pos, head) {
		struct __prelim_ref *ref;
		ref = list_entry(pos, struct __prelim_ref, list);

		if (ref->parent)
			continue;
		if (ref->key_for_search.type)
			continue;
		BUG_ON(!ref->wanted_disk_byte);
		eb = read_tree_block(fs_info->tree_root, ref->wanted_disk_byte,
				     0);
		if (IS_ERR(eb)) {
			return PTR_ERR(eb);
		} else if (!extent_buffer_uptodate(eb)) {
			free_extent_buffer(eb);
			return -EIO;
		}
		btrfs_tree_read_lock(eb);
		if (btrfs_header_level(eb) == 0)
			btrfs_item_key_to_cpu(eb, &ref->key_for_search, 0);
		else
			btrfs_node_key_to_cpu(eb, &ref->key_for_search, 0);
		btrfs_tree_read_unlock(eb);
		free_extent_buffer(eb);
	}
	return 0;
}

/*
 * merge backrefs and adjust counts accordingly
 *
 * mode = 1: merge identical keys, if key is set
 *    FIXME: if we add more keys in __add_prelim_ref, we can merge more here.
 *           additionally, we could even add a key range for the blocks we
 *           looked into to merge even more (-> replace unresolved refs by those
 *           having a parent).
 * mode = 2: merge identical parents
 */
static void __merge_refs(struct list_head *head, int mode)
{
	struct list_head *pos1;

	list_for_each(pos1, head) {
		struct list_head *n2;
		struct list_head *pos2;
		struct __prelim_ref *ref1;

		ref1 = list_entry(pos1, struct __prelim_ref, list);

		for (pos2 = pos1->next, n2 = pos2->next; pos2 != head;
		     pos2 = n2, n2 = pos2->next) {
			struct __prelim_ref *ref2;
			struct __prelim_ref *xchg;
			struct extent_inode_elem *eie;

			ref2 = list_entry(pos2, struct __prelim_ref, list);

			if (!ref_for_same_block(ref1, ref2))
				continue;
			if (mode == 1) {
				if (!ref1->parent && ref2->parent) {
					xchg = ref1;
					ref1 = ref2;
					ref2 = xchg;
				}
			} else {
				if (ref1->parent != ref2->parent)
					continue;
			}

			eie = ref1->inode_list;
			while (eie && eie->next)
				eie = eie->next;
			if (eie)
				eie->next = ref2->inode_list;
			else
				ref1->inode_list = ref2->inode_list;
			ref1->count += ref2->count;

			list_del(&ref2->list);
			kmem_cache_free(btrfs_prelim_ref_cache, ref2);
		}

	}
}

/*
 * add all currently queued delayed refs from this head whose seq nr is
 * smaller or equal that seq to the list
 */
static int __add_delayed_refs(struct btrfs_delayed_ref_head *head, u64 seq,
			      struct list_head *prefs, u64 *total_refs,
			      u64 inum)
{
	struct btrfs_delayed_ref_node *node;
	struct btrfs_delayed_extent_op *extent_op = head->extent_op;
	struct btrfs_key key;
	struct btrfs_key op_key = {0};
	int sgn;
	int ret = 0;

	if (extent_op && extent_op->update_key)
		btrfs_disk_key_to_cpu(&op_key, &extent_op->key);

	spin_lock(&head->lock);
	list_for_each_entry(node, &head->ref_list, list) {
		if (node->seq > seq)
			continue;

		switch (node->action) {
		case BTRFS_ADD_DELAYED_EXTENT:
		case BTRFS_UPDATE_DELAYED_HEAD:
			WARN_ON(1);
			continue;
		case BTRFS_ADD_DELAYED_REF:
			sgn = 1;
			break;
		case BTRFS_DROP_DELAYED_REF:
			sgn = -1;
			break;
		default:
			BUG_ON(1);
		}
		*total_refs += (node->ref_mod * sgn);
		switch (node->type) {
		case BTRFS_TREE_BLOCK_REF_KEY: {
			struct btrfs_delayed_tree_ref *ref;

			ref = btrfs_delayed_node_to_tree_ref(node);
			ret = __add_prelim_ref(prefs, ref->root, &op_key,
					       ref->level + 1, 0, node->bytenr,
					       node->ref_mod * sgn, GFP_ATOMIC);
			break;
		}
		case BTRFS_SHARED_BLOCK_REF_KEY: {
			struct btrfs_delayed_tree_ref *ref;

			ref = btrfs_delayed_node_to_tree_ref(node);
			ret = __add_prelim_ref(prefs, 0, NULL,
					       ref->level + 1, ref->parent,
					       node->bytenr,
					       node->ref_mod * sgn, GFP_ATOMIC);
			break;
		}
		case BTRFS_EXTENT_DATA_REF_KEY: {
			struct btrfs_delayed_data_ref *ref;
			ref = btrfs_delayed_node_to_data_ref(node);

			key.objectid = ref->objectid;
			key.type = BTRFS_EXTENT_DATA_KEY;
			key.offset = ref->offset;

			/*
			 * Found a inum that doesn't match our known inum, we
			 * know it's shared.
			 */
			if (inum && ref->objectid != inum) {
				ret = BACKREF_FOUND_SHARED;
				break;
			}

			ret = __add_prelim_ref(prefs, ref->root, &key, 0, 0,
					       node->bytenr,
					       node->ref_mod * sgn, GFP_ATOMIC);
			break;
		}
		case BTRFS_SHARED_DATA_REF_KEY: {
			struct btrfs_delayed_data_ref *ref;

			ref = btrfs_delayed_node_to_data_ref(node);
			ret = __add_prelim_ref(prefs, 0, NULL, 0,
					       ref->parent, node->bytenr,
					       node->ref_mod * sgn, GFP_ATOMIC);
			break;
		}
		default:
			WARN_ON(1);
		}
		if (ret)
			break;
	}
	spin_unlock(&head->lock);
	return ret;
}

/*
 * add all inline backrefs for bytenr to the list
 */
static int __add_inline_refs(struct btrfs_fs_info *fs_info,
			     struct btrfs_path *path, u64 bytenr,
			     int *info_level, struct list_head *prefs,
			     u64 *total_refs, u64 inum)
{
	int ret = 0;
	int slot;
	struct extent_buffer *leaf;
	struct btrfs_key key;
	struct btrfs_key found_key;
	unsigned long ptr;
	unsigned long end;
	struct btrfs_extent_item *ei;
	u64 flags;
	u64 item_size;

	/*
	 * enumerate all inline refs
	 */
	leaf = path->nodes[0];
	slot = path->slots[0];

	item_size = btrfs_item_size_nr(leaf, slot);
	BUG_ON(item_size < sizeof(*ei));

	ei = btrfs_item_ptr(leaf, slot, struct btrfs_extent_item);
	flags = btrfs_extent_flags(leaf, ei);
	*total_refs += btrfs_extent_refs(leaf, ei);
	btrfs_item_key_to_cpu(leaf, &found_key, slot);

	ptr = (unsigned long)(ei + 1);
	end = (unsigned long)ei + item_size;

	if (found_key.type == BTRFS_EXTENT_ITEM_KEY &&
	    flags & BTRFS_EXTENT_FLAG_TREE_BLOCK) {
		struct btrfs_tree_block_info *info;

		info = (struct btrfs_tree_block_info *)ptr;
		*info_level = btrfs_tree_block_level(leaf, info);
		ptr += sizeof(struct btrfs_tree_block_info);
		BUG_ON(ptr > end);
	} else if (found_key.type == BTRFS_METADATA_ITEM_KEY) {
		*info_level = found_key.offset;
	} else {
		BUG_ON(!(flags & BTRFS_EXTENT_FLAG_DATA));
	}

	while (ptr < end) {
		struct btrfs_extent_inline_ref *iref;
		u64 offset;
		int type;

		iref = (struct btrfs_extent_inline_ref *)ptr;
		type = btrfs_extent_inline_ref_type(leaf, iref);
		offset = btrfs_extent_inline_ref_offset(leaf, iref);

		switch (type) {
		case BTRFS_SHARED_BLOCK_REF_KEY:
			ret = __add_prelim_ref(prefs, 0, NULL,
						*info_level + 1, offset,
						bytenr, 1, GFP_NOFS);
			break;
		case BTRFS_SHARED_DATA_REF_KEY: {
			struct btrfs_shared_data_ref *sdref;
			int count;

			sdref = (struct btrfs_shared_data_ref *)(iref + 1);
			count = btrfs_shared_data_ref_count(leaf, sdref);
			ret = __add_prelim_ref(prefs, 0, NULL, 0, offset,
					       bytenr, count, GFP_NOFS);
			break;
		}
		case BTRFS_TREE_BLOCK_REF_KEY:
			ret = __add_prelim_ref(prefs, offset, NULL,
					       *info_level + 1, 0,
					       bytenr, 1, GFP_NOFS);
			break;
		case BTRFS_EXTENT_DATA_REF_KEY: {
			struct btrfs_extent_data_ref *dref;
			int count;
			u64 root;

			dref = (struct btrfs_extent_data_ref *)(&iref->offset);
			count = btrfs_extent_data_ref_count(leaf, dref);
			key.objectid = btrfs_extent_data_ref_objectid(leaf,
								      dref);
			key.type = BTRFS_EXTENT_DATA_KEY;
			key.offset = btrfs_extent_data_ref_offset(leaf, dref);

			if (inum && key.objectid != inum) {
				ret = BACKREF_FOUND_SHARED;
				break;
			}

			root = btrfs_extent_data_ref_root(leaf, dref);
			ret = __add_prelim_ref(prefs, root, &key, 0, 0,
					       bytenr, count, GFP_NOFS);
			break;
		}
		default:
			WARN_ON(1);
		}
		if (ret)
			return ret;
		ptr += btrfs_extent_inline_ref_size(type);
	}

	return 0;
}

/*
 * add all non-inline backrefs for bytenr to the list
 */
static int __add_keyed_refs(struct btrfs_fs_info *fs_info,
			    struct btrfs_path *path, u64 bytenr,
			    int info_level, struct list_head *prefs, u64 inum)
{
	struct btrfs_root *extent_root = fs_info->extent_root;
	int ret;
	int slot;
	struct extent_buffer *leaf;
	struct btrfs_key key;

	while (1) {
		ret = btrfs_next_item(extent_root, path);
		if (ret < 0)
			break;
		if (ret) {
			ret = 0;
			break;
		}

		slot = path->slots[0];
		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &key, slot);

		if (key.objectid != bytenr)
			break;
		if (key.type < BTRFS_TREE_BLOCK_REF_KEY)
			continue;
		if (key.type > BTRFS_SHARED_DATA_REF_KEY)
			break;

		switch (key.type) {
		case BTRFS_SHARED_BLOCK_REF_KEY:
			ret = __add_prelim_ref(prefs, 0, NULL,
						info_level + 1, key.offset,
						bytenr, 1, GFP_NOFS);
			break;
		case BTRFS_SHARED_DATA_REF_KEY: {
			struct btrfs_shared_data_ref *sdref;
			int count;

			sdref = btrfs_item_ptr(leaf, slot,
					      struct btrfs_shared_data_ref);
			count = btrfs_shared_data_ref_count(leaf, sdref);
			ret = __add_prelim_ref(prefs, 0, NULL, 0, key.offset,
						bytenr, count, GFP_NOFS);
			break;
		}
		case BTRFS_TREE_BLOCK_REF_KEY:
			ret = __add_prelim_ref(prefs, key.offset, NULL,
					       info_level + 1, 0,
					       bytenr, 1, GFP_NOFS);
			break;
		case BTRFS_EXTENT_DATA_REF_KEY: {
			struct btrfs_extent_data_ref *dref;
			int count;
			u64 root;

			dref = btrfs_item_ptr(leaf, slot,
					      struct btrfs_extent_data_ref);
			count = btrfs_extent_data_ref_count(leaf, dref);
			key.objectid = btrfs_extent_data_ref_objectid(leaf,
								      dref);
			key.type = BTRFS_EXTENT_DATA_KEY;
			key.offset = btrfs_extent_data_ref_offset(leaf, dref);

			if (inum && key.objectid != inum) {
				ret = BACKREF_FOUND_SHARED;
				break;
			}

			root = btrfs_extent_data_ref_root(leaf, dref);
			ret = __add_prelim_ref(prefs, root, &key, 0, 0,
					       bytenr, count, GFP_NOFS);
			break;
		}
		default:
			WARN_ON(1);
		}
		if (ret)
			return ret;

	}

	return ret;
}

/*
 * this adds all existing backrefs (inline backrefs, backrefs and delayed
 * refs) for the given bytenr to the refs list, merges duplicates and resolves
 * indirect refs to their parent bytenr.
 * When roots are found, they're added to the roots list
 *
 * NOTE: This can return values > 0
 *
 * If time_seq is set to (u64)-1, it will not search delayed_refs, and behave
 * much like trans == NULL case, the difference only lies in it will not
 * commit root.
 * The special case is for qgroup to search roots in commit_transaction().
 *
 * FIXME some caching might speed things up
 */
static int find_parent_nodes(struct btrfs_trans_handle *trans,
			     struct btrfs_fs_info *fs_info, u64 bytenr,
			     u64 time_seq, struct ulist *refs,
			     struct ulist *roots, const u64 *extent_item_pos,
			     u64 root_objectid, u64 inum)
{
	struct btrfs_key key;
	struct btrfs_path *path;
	struct btrfs_delayed_ref_root *delayed_refs = NULL;
	struct btrfs_delayed_ref_head *head;
	int info_level = 0;
	int ret;
	struct list_head prefs_delayed;
	struct list_head prefs;
	struct __prelim_ref *ref;
	struct extent_inode_elem *eie = NULL;
	u64 total_refs = 0;

	INIT_LIST_HEAD(&prefs);
	INIT_LIST_HEAD(&prefs_delayed);

	key.objectid = bytenr;
	key.offset = (u64)-1;
	if (btrfs_fs_incompat(fs_info, SKINNY_METADATA))
		key.type = BTRFS_METADATA_ITEM_KEY;
	else
		key.type = BTRFS_EXTENT_ITEM_KEY;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	if (!trans) {
		path->search_commit_root = 1;
		path->skip_locking = 1;
	}

	if (time_seq == (u64)-1)
		path->skip_locking = 1;

	/*
	 * grab both a lock on the path and a lock on the delayed ref head.
	 * We need both to get a consistent picture of how the refs look
	 * at a specified point in time
	 */
again:
	head = NULL;

	ret = btrfs_search_slot(trans, fs_info->extent_root, &key, path, 0, 0);
	if (ret < 0)
		goto out;
	if (ret == 0) {
		/* This shouldn't happen, indicates a bug or fs corruption. */
		ASSERT(ret != 0);
		ret = -EUCLEAN;
		goto out;
	}

#ifdef CONFIG_BTRFS_FS_RUN_SANITY_TESTS
	if (trans && likely(trans->type != __TRANS_DUMMY) &&
	    time_seq != (u64)-1) {
#else
	if (trans && time_seq != (u64)-1) {
#endif
		/*
		 * look if there are updates for this ref queued and lock the
		 * head
		 */
		delayed_refs = &trans->transaction->delayed_refs;
		spin_lock(&delayed_refs->lock);
		head = btrfs_find_delayed_ref_head(trans, bytenr);
		if (head) {
			if (!mutex_trylock(&head->mutex)) {
				atomic_inc(&head->node.refs);
				spin_unlock(&delayed_refs->lock);

				btrfs_release_path(path);

				/*
				 * Mutex was contended, block until it's
				 * released and try again
				 */
				mutex_lock(&head->mutex);
				mutex_unlock(&head->mutex);
				btrfs_put_delayed_ref(&head->node);
				goto again;
			}
			spin_unlock(&delayed_refs->lock);
			ret = __add_delayed_refs(head, time_seq,
						 &prefs_delayed, &total_refs,
						 inum);
			mutex_unlock(&head->mutex);
			if (ret)
				goto out;
		} else {
			spin_unlock(&delayed_refs->lock);
		}
	}

	if (path->slots[0]) {
		struct extent_buffer *leaf;
		int slot;

		path->slots[0]--;
		leaf = path->nodes[0];
		slot = path->slots[0];
		btrfs_item_key_to_cpu(leaf, &key, slot);
		if (key.objectid == bytenr &&
		    (key.type == BTRFS_EXTENT_ITEM_KEY ||
		     key.type == BTRFS_METADATA_ITEM_KEY)) {
			ret = __add_inline_refs(fs_info, path, bytenr,
						&info_level, &prefs,
						&total_refs, inum);
			if (ret)
				goto out;
			ret = __add_keyed_refs(fs_info, path, bytenr,
					       info_level, &prefs, inum);
			if (ret)
				goto out;
		}
	}
	btrfs_release_path(path);

	list_splice_init(&prefs_delayed, &prefs);

	ret = __add_missing_keys(fs_info, &prefs);
	if (ret)
		goto out;

	__merge_refs(&prefs, 1);

	ret = __resolve_indirect_refs(fs_info, path, time_seq, &prefs,
				      extent_item_pos, total_refs,
				      root_objectid);
	if (ret)
		goto out;

	__merge_refs(&prefs, 2);

	while (!list_empty(&prefs)) {
		ref = list_first_entry(&prefs, struct __prelim_ref, list);
		WARN_ON(ref->count < 0);
		if (roots && ref->count && ref->root_id && ref->parent == 0) {
			if (root_objectid && ref->root_id != root_objectid) {
				ret = BACKREF_FOUND_SHARED;
				goto out;
			}

			/* no parent == root of tree */
			ret = ulist_add(roots, ref->root_id, 0, GFP_NOFS);
			if (ret < 0)
				goto out;
		}
		if (ref->count && ref->parent) {
			if (extent_item_pos && !ref->inode_list &&
			    ref->level == 0) {
				struct extent_buffer *eb;

				eb = read_tree_block(fs_info->extent_root,
							   ref->parent, 0);
				if (IS_ERR(eb)) {
					ret = PTR_ERR(eb);
					goto out;
				} else if (!extent_buffer_uptodate(eb)) {
					free_extent_buffer(eb);
					ret = -EIO;
					goto out;
				}
				btrfs_tree_read_lock(eb);
				btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
				ret = find_extent_in_eb(eb, bytenr,
							*extent_item_pos, &eie);
				btrfs_tree_read_unlock_blocking(eb);
				free_extent_buffer(eb);
				if (ret < 0)
					goto out;
				ref->inode_list = eie;
			}
			ret = ulist_add_merge_ptr(refs, ref->parent,
						  ref->inode_list,
						  (void **)&eie, GFP_NOFS);
			if (ret < 0)
				goto out;
			if (!ret && extent_item_pos) {
				/*
				 * We've recorded that parent, so we must extend
				 * its inode list here.
				 *
				 * However if there was corruption we may not
				 * have found an eie, return an error in this
				 * case.
				 */
				ASSERT(eie);
				if (!eie) {
					ret = -EUCLEAN;
					goto out;
				}
				while (eie->next)
					eie = eie->next;
				eie->next = ref->inode_list;
			}
			eie = NULL;
		}
		list_del(&ref->list);
		kmem_cache_free(btrfs_prelim_ref_cache, ref);
	}

out:
	btrfs_free_path(path);
	while (!list_empty(&prefs)) {
		ref = list_first_entry(&prefs, struct __prelim_ref, list);
		list_del(&ref->list);
		kmem_cache_free(btrfs_prelim_ref_cache, ref);
	}
	while (!list_empty(&prefs_delayed)) {
		ref = list_first_entry(&prefs_delayed, struct __prelim_ref,
				       list);
		list_del(&ref->list);
		kmem_cache_free(btrfs_prelim_ref_cache, ref);
	}
	if (ret < 0)
		free_inode_elem_list(eie);
	return ret;
}

static void free_leaf_list(struct ulist *blocks)
{
	struct ulist_node *node = NULL;
	struct extent_inode_elem *eie;
	struct ulist_iterator uiter;

	ULIST_ITER_INIT(&uiter);
	while ((node = ulist_next(blocks, &uiter))) {
		if (!node->aux)
			continue;
		eie = (struct extent_inode_elem *)(uintptr_t)node->aux;
		free_inode_elem_list(eie);
		node->aux = 0;
	}

	ulist_free(blocks);
}

/*
 * Finds all leafs with a reference to the specified combination of bytenr and
 * offset. key_list_head will point to a list of corresponding keys (caller must
 * free each list element). The leafs will be stored in the leafs ulist, which
 * must be freed with ulist_free.
 *
 * returns 0 on success, <0 on error
 */
static int btrfs_find_all_leafs(struct btrfs_trans_handle *trans,
				struct btrfs_fs_info *fs_info, u64 bytenr,
				u64 time_seq, struct ulist **leafs,
				const u64 *extent_item_pos)
{
	int ret;

	*leafs = ulist_alloc(GFP_NOFS);
	if (!*leafs)
		return -ENOMEM;

	ret = find_parent_nodes(trans, fs_info, bytenr,
				time_seq, *leafs, NULL, extent_item_pos, 0, 0);
	if (ret < 0 && ret != -ENOENT) {
		free_leaf_list(*leafs);
		return ret;
	}

	return 0;
}

/*
 * walk all backrefs for a given extent to find all roots that reference this
 * extent. Walking a backref means finding all extents that reference this
 * extent and in turn walk the backrefs of those, too. Naturally this is a
 * recursive process, but here it is implemented in an iterative fashion: We
 * find all referencing extents for the extent in question and put them on a
 * list. In turn, we find all referencing extents for those, further appending
 * to the list. The way we iterate the list allows adding more elements after
 * the current while iterating. The process stops when we reach the end of the
 * list. Found roots are added to the roots list.
 *
 * returns 0 on success, < 0 on error.
 */
static int __btrfs_find_all_roots(struct btrfs_trans_handle *trans,
				  struct btrfs_fs_info *fs_info, u64 bytenr,
				  u64 time_seq, struct ulist **roots)
{
	struct ulist *tmp;
	struct ulist_node *node = NULL;
	struct ulist_iterator uiter;
	int ret;

	tmp = ulist_alloc(GFP_NOFS);
	if (!tmp)
		return -ENOMEM;
	*roots = ulist_alloc(GFP_NOFS);
	if (!*roots) {
		ulist_free(tmp);
		return -ENOMEM;
	}

	ULIST_ITER_INIT(&uiter);
	while (1) {
		ret = find_parent_nodes(trans, fs_info, bytenr,
					time_seq, tmp, *roots, NULL, 0, 0);
		if (ret < 0 && ret != -ENOENT) {
			ulist_free(tmp);
			ulist_free(*roots);
			*roots = NULL;
			return ret;
		}
		node = ulist_next(tmp, &uiter);
		if (!node)
			break;
		bytenr = node->val;
		cond_resched();
	}

	ulist_free(tmp);
	return 0;
}

int btrfs_find_all_roots(struct btrfs_trans_handle *trans,
			 struct btrfs_fs_info *fs_info, u64 bytenr,
			 u64 time_seq, struct ulist **roots)
{
	int ret;

	if (!trans)
		down_read(&fs_info->commit_root_sem);
	ret = __btrfs_find_all_roots(trans, fs_info, bytenr, time_seq, roots);
	if (!trans)
		up_read(&fs_info->commit_root_sem);
	return ret;
}

/**
 * btrfs_check_shared - tell us whether an extent is shared
 *
 * @trans: optional trans handle
 *
 * btrfs_check_shared uses the backref walking code but will short
 * circuit as soon as it finds a root or inode that doesn't match the
 * one passed in. This provides a significant performance benefit for
 * callers (such as fiemap) which want to know whether the extent is
 * shared but do not need a ref count.
 *
 * Return: 0 if extent is not shared, 1 if it is shared, < 0 on error.
 */
int btrfs_check_shared(struct btrfs_trans_handle *trans,
		       struct btrfs_fs_info *fs_info, u64 root_objectid,
		       u64 inum, u64 bytenr)
{
	struct ulist *tmp = NULL;
	struct ulist *roots = NULL;
	struct ulist_iterator uiter;
	struct ulist_node *node;
	struct seq_list elem = SEQ_LIST_INIT(elem);
	int ret = 0;

	tmp = ulist_alloc(GFP_NOFS);
	roots = ulist_alloc(GFP_NOFS);
	if (!tmp || !roots) {
		ulist_free(tmp);
		ulist_free(roots);
		return -ENOMEM;
	}

	if (trans)
		btrfs_get_tree_mod_seq(fs_info, &elem);
	else
		down_read(&fs_info->commit_root_sem);
	ULIST_ITER_INIT(&uiter);
	while (1) {
		ret = find_parent_nodes(trans, fs_info, bytenr, elem.seq, tmp,
					roots, NULL, root_objectid, inum);
		if (ret == BACKREF_FOUND_SHARED) {
			/* this is the only condition under which we return 1 */
			ret = 1;
			break;
		}
		if (ret < 0 && ret != -ENOENT)
			break;
		ret = 0;
		node = ulist_next(tmp, &uiter);
		if (!node)
			break;
		bytenr = node->val;
		cond_resched();
	}
	if (trans)
		btrfs_put_tree_mod_seq(fs_info, &elem);
	else
		up_read(&fs_info->commit_root_sem);
	ulist_free(tmp);
	ulist_free(roots);
	return ret;
}

int btrfs_find_one_extref(struct btrfs_root *root, u64 inode_objectid,
			  u64 start_off, struct btrfs_path *path,
			  struct btrfs_inode_extref **ret_extref,
			  u64 *found_off)
{
	int ret, slot;
	struct btrfs_key key;
	struct btrfs_key found_key;
	struct btrfs_inode_extref *extref;
	struct extent_buffer *leaf;
	unsigned long ptr;

	key.objectid = inode_objectid;
	key.type = BTRFS_INODE_EXTREF_KEY;
	key.offset = start_off;

	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		return ret;

	while (1) {
		leaf = path->nodes[0];
		slot = path->slots[0];
		if (slot >= btrfs_header_nritems(leaf)) {
			/*
			 * If the item at offset is not found,
			 * btrfs_search_slot will point us to the slot
			 * where it should be inserted. In our case
			 * that will be the slot directly before the
			 * next INODE_REF_KEY_V2 item. In the case
			 * that we're pointing to the last slot in a
			 * leaf, we must move one leaf over.
			 */
			ret = btrfs_next_leaf(root, path);
			if (ret) {
				if (ret >= 1)
					ret = -ENOENT;
				break;
			}
			continue;
		}

		btrfs_item_key_to_cpu(leaf, &found_key, slot);

		/*
		 * Check that we're still looking at an extended ref key for
		 * this particular objectid. If we have different
		 * objectid or type then there are no more to be found
		 * in the tree and we can exit.
		 */
		ret = -ENOENT;
		if (found_key.objectid != inode_objectid)
			break;
		if (found_key.type != BTRFS_INODE_EXTREF_KEY)
			break;

		ret = 0;
		ptr = btrfs_item_ptr_offset(leaf, path->slots[0]);
		extref = (struct btrfs_inode_extref *)ptr;
		*ret_extref = extref;
		if (found_off)
			*found_off = found_key.offset;
		break;
	}

	return ret;
}

/*
 * this iterates to turn a name (from iref/extref) into a full filesystem path.
 * Elements of the path are separated by '/' and the path is guaranteed to be
 * 0-terminated. the path is only given within the current file system.
 * Therefore, it never starts with a '/'. the caller is responsible to provide
 * "size" bytes in "dest". the dest buffer will be filled backwards. finally,
 * the start point of the resulting string is returned. this pointer is within
 * dest, normally.
 * in case the path buffer would overflow, the pointer is decremented further
 * as if output was written to the buffer, though no more output is actually
 * generated. that way, the caller can determine how much space would be
 * required for the path to fit into the buffer. in that case, the returned
 * value will be smaller than dest. callers must check this!
 */
char *btrfs_ref_to_path(struct btrfs_root *fs_root, struct btrfs_path *path,
			u32 name_len, unsigned long name_off,
			struct extent_buffer *eb_in, u64 parent,
			char *dest, u32 size)
{
	int slot;
	u64 next_inum;
	int ret;
	s64 bytes_left = ((s64)size) - 1;
	struct extent_buffer *eb = eb_in;
	struct btrfs_key found_key;
	int leave_spinning = path->leave_spinning;
	struct btrfs_inode_ref *iref;

	if (bytes_left >= 0)
		dest[bytes_left] = '\0';

	path->leave_spinning = 1;
	while (1) {
		bytes_left -= name_len;
		if (bytes_left >= 0)
			read_extent_buffer(eb, dest + bytes_left,
					   name_off, name_len);
		if (eb != eb_in) {
			if (!path->skip_locking)
				btrfs_tree_read_unlock_blocking(eb);
			free_extent_buffer(eb);
		}
		ret = btrfs_find_item(fs_root, path, parent, 0,
				BTRFS_INODE_REF_KEY, &found_key);
		if (ret > 0)
			ret = -ENOENT;
		if (ret)
			break;

		next_inum = found_key.offset;

		/* regular exit ahead */
		if (parent == next_inum)
			break;

		slot = path->slots[0];
		eb = path->nodes[0];
		/* make sure we can use eb after releasing the path */
		if (eb != eb_in) {
			if (!path->skip_locking)
				btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
			path->nodes[0] = NULL;
			path->locks[0] = 0;
		}
		btrfs_release_path(path);
		iref = btrfs_item_ptr(eb, slot, struct btrfs_inode_ref);

		name_len = btrfs_inode_ref_name_len(eb, iref);
		name_off = (unsigned long)(iref + 1);

		parent = next_inum;
		--bytes_left;
		if (bytes_left >= 0)
			dest[bytes_left] = '/';
	}

	btrfs_release_path(path);
	path->leave_spinning = leave_spinning;

	if (ret)
		return ERR_PTR(ret);

	return dest + bytes_left;
}

/*
 * this makes the path point to (logical EXTENT_ITEM *)
 * returns BTRFS_EXTENT_FLAG_DATA for data, BTRFS_EXTENT_FLAG_TREE_BLOCK for
 * tree blocks and <0 on error.
 */
int extent_from_logical(struct btrfs_fs_info *fs_info, u64 logical,
			struct btrfs_path *path, struct btrfs_key *found_key,
			u64 *flags_ret)
{
	int ret;
	u64 flags;
	u64 size = 0;
	u32 item_size;
	struct extent_buffer *eb;
	struct btrfs_extent_item *ei;
	struct btrfs_key key;

	if (btrfs_fs_incompat(fs_info, SKINNY_METADATA))
		key.type = BTRFS_METADATA_ITEM_KEY;
	else
		key.type = BTRFS_EXTENT_ITEM_KEY;
	key.objectid = logical;
	key.offset = (u64)-1;

	ret = btrfs_search_slot(NULL, fs_info->extent_root, &key, path, 0, 0);
	if (ret < 0)
		return ret;

	ret = btrfs_previous_extent_item(fs_info->extent_root, path, 0);
	if (ret) {
		if (ret > 0)
			ret = -ENOENT;
		return ret;
	}
	btrfs_item_key_to_cpu(path->nodes[0], found_key, path->slots[0]);
	if (found_key->type == BTRFS_METADATA_ITEM_KEY)
		size = fs_info->extent_root->nodesize;
	else if (found_key->type == BTRFS_EXTENT_ITEM_KEY)
		size = found_key->offset;

	if (found_key->objectid > logical ||
	    found_key->objectid + size <= logical) {
		pr_debug("logical %llu is not within any extent\n", logical);
		return -ENOENT;
	}

	eb = path->nodes[0];
	item_size = btrfs_item_size_nr(eb, path->slots[0]);
	BUG_ON(item_size < sizeof(*ei));

	ei = btrfs_item_ptr(eb, path->slots[0], struct btrfs_extent_item);
	flags = btrfs_extent_flags(eb, ei);

	pr_debug("logical %llu is at position %llu within the extent (%llu "
		 "EXTENT_ITEM %llu) flags %#llx size %u\n",
		 logical, logical - found_key->objectid, found_key->objectid,
		 found_key->offset, flags, item_size);

	WARN_ON(!flags_ret);
	if (flags_ret) {
		if (flags & BTRFS_EXTENT_FLAG_TREE_BLOCK)
			*flags_ret = BTRFS_EXTENT_FLAG_TREE_BLOCK;
		else if (flags & BTRFS_EXTENT_FLAG_DATA)
			*flags_ret = BTRFS_EXTENT_FLAG_DATA;
		else
			BUG_ON(1);
		return 0;
	}

	return -EIO;
}

/*
 * helper function to iterate extent inline refs. ptr must point to a 0 value
 * for the first call and may be modified. it is used to track state.
 * if more refs exist, 0 is returned and the next call to
 * __get_extent_inline_ref must pass the modified ptr parameter to get the
 * next ref. after the last ref was processed, 1 is returned.
 * returns <0 on error
 */
static int __get_extent_inline_ref(unsigned long *ptr, struct extent_buffer *eb,
				   struct btrfs_key *key,
				   struct btrfs_extent_item *ei, u32 item_size,
				   struct btrfs_extent_inline_ref **out_eiref,
				   int *out_type)
{
	unsigned long end;
	u64 flags;
	struct btrfs_tree_block_info *info;

	if (!*ptr) {
		/* first call */
		flags = btrfs_extent_flags(eb, ei);
		if (flags & BTRFS_EXTENT_FLAG_TREE_BLOCK) {
			if (key->type == BTRFS_METADATA_ITEM_KEY) {
				/* a skinny metadata extent */
				*out_eiref =
				     (struct btrfs_extent_inline_ref *)(ei + 1);
			} else {
				WARN_ON(key->type != BTRFS_EXTENT_ITEM_KEY);
				info = (struct btrfs_tree_block_info *)(ei + 1);
				*out_eiref =
				   (struct btrfs_extent_inline_ref *)(info + 1);
			}
		} else {
			*out_eiref = (struct btrfs_extent_inline_ref *)(ei + 1);
		}
		*ptr = (unsigned long)*out_eiref;
		if ((unsigned long)(*ptr) >= (unsigned long)ei + item_size)
			return -ENOENT;
	}

	end = (unsigned long)ei + item_size;
	*out_eiref = (struct btrfs_extent_inline_ref *)(*ptr);
	*out_type = btrfs_extent_inline_ref_type(eb, *out_eiref);

	*ptr += btrfs_extent_inline_ref_size(*out_type);
	WARN_ON(*ptr > end);
	if (*ptr == end)
		return 1; /* last */

	return 0;
}

/*
 * reads the tree block backref for an extent. tree level and root are returned
 * through out_level and out_root. ptr must point to a 0 value for the first
 * call and may be modified (see __get_extent_inline_ref comment).
 * returns 0 if data was provided, 1 if there was no more data to provide or
 * <0 on error.
 */
int tree_backref_for_extent(unsigned long *ptr, struct extent_buffer *eb,
			    struct btrfs_key *key, struct btrfs_extent_item *ei,
			    u32 item_size, u64 *out_root, u8 *out_level)
{
	int ret;
	int type;
	struct btrfs_extent_inline_ref *eiref;

	if (*ptr == (unsigned long)-1)
		return 1;

	while (1) {
		ret = __get_extent_inline_ref(ptr, eb, key, ei, item_size,
					      &eiref, &type);
		if (ret < 0)
			return ret;

		if (type == BTRFS_TREE_BLOCK_REF_KEY ||
		    type == BTRFS_SHARED_BLOCK_REF_KEY)
			break;

		if (ret == 1)
			return 1;
	}

	/* we can treat both ref types equally here */
	*out_root = btrfs_extent_inline_ref_offset(eb, eiref);

	if (key->type == BTRFS_EXTENT_ITEM_KEY) {
		struct btrfs_tree_block_info *info;

		info = (struct btrfs_tree_block_info *)(ei + 1);
		*out_level = btrfs_tree_block_level(eb, info);
	} else {
		ASSERT(key->type == BTRFS_METADATA_ITEM_KEY);
		*out_level = (u8)key->offset;
	}

	if (ret == 1)
		*ptr = (unsigned long)-1;

	return 0;
}

static int iterate_leaf_refs(struct extent_inode_elem *inode_list,
				u64 root, u64 extent_item_objectid,
				iterate_extent_inodes_t *iterate, void *ctx)
{
	struct extent_inode_elem *eie;
	int ret = 0;

	for (eie = inode_list; eie; eie = eie->next) {
		pr_debug("ref for %llu resolved, key (%llu EXTEND_DATA %llu), "
			 "root %llu\n", extent_item_objectid,
			 eie->inum, eie->offset, root);
		ret = iterate(eie->inum, eie->offset, root, ctx);
		if (ret) {
			pr_debug("stopping iteration for %llu due to ret=%d\n",
				 extent_item_objectid, ret);
			break;
		}
	}

	return ret;
}

/*
 * calls iterate() for every inode that references the extent identified by
 * the given parameters.
 * when the iterator function returns a non-zero value, iteration stops.
 */
int iterate_extent_inodes(struct btrfs_fs_info *fs_info,
				u64 extent_item_objectid, u64 extent_item_pos,
				int search_commit_root,
				iterate_extent_inodes_t *iterate, void *ctx)
{
	int ret;
	struct btrfs_trans_handle *trans = NULL;
	struct ulist *refs = NULL;
	struct ulist *roots = NULL;
	struct ulist_node *ref_node = NULL;
	struct ulist_node *root_node = NULL;
	struct seq_list tree_mod_seq_elem = SEQ_LIST_INIT(tree_mod_seq_elem);
	struct ulist_iterator ref_uiter;
	struct ulist_iterator root_uiter;

	pr_debug("resolving all inodes for extent %llu\n",
			extent_item_objectid);

	if (!search_commit_root) {
		trans = btrfs_attach_transaction(fs_info->extent_root);
		if (IS_ERR(trans)) {
			if (PTR_ERR(trans) != -ENOENT &&
			    PTR_ERR(trans) != -EROFS)
				return PTR_ERR(trans);
			trans = NULL;
		}
	}

	if (trans)
		btrfs_get_tree_mod_seq(fs_info, &tree_mod_seq_elem);
	else
		down_read(&fs_info->commit_root_sem);

	ret = btrfs_find_all_leafs(trans, fs_info, extent_item_objectid,
				   tree_mod_seq_elem.seq, &refs,
				   &extent_item_pos);
	if (ret)
		goto out;

	ULIST_ITER_INIT(&ref_uiter);
	while (!ret && (ref_node = ulist_next(refs, &ref_uiter))) {
		ret = __btrfs_find_all_roots(trans, fs_info, ref_node->val,
					     tree_mod_seq_elem.seq, &roots);
		if (ret)
			break;
		ULIST_ITER_INIT(&root_uiter);
		while (!ret && (root_node = ulist_next(roots, &root_uiter))) {
			pr_debug("root %llu references leaf %llu, data list "
				 "%#llx\n", root_node->val, ref_node->val,
				 ref_node->aux);
			ret = iterate_leaf_refs((struct extent_inode_elem *)
						(uintptr_t)ref_node->aux,
						root_node->val,
						extent_item_objectid,
						iterate, ctx);
		}
		ulist_free(roots);
	}

	free_leaf_list(refs);
out:
	if (trans) {
		btrfs_put_tree_mod_seq(fs_info, &tree_mod_seq_elem);
		btrfs_end_transaction(trans, fs_info->extent_root);
	} else {
		up_read(&fs_info->commit_root_sem);
	}

	return ret;
}

int iterate_inodes_from_logical(u64 logical, struct btrfs_fs_info *fs_info,
				struct btrfs_path *path,
				iterate_extent_inodes_t *iterate, void *ctx)
{
	int ret;
	u64 extent_item_pos;
	u64 flags = 0;
	struct btrfs_key found_key;
	int search_commit_root = path->search_commit_root;

	ret = extent_from_logical(fs_info, logical, path, &found_key, &flags);
	btrfs_release_path(path);
	if (ret < 0)
		return ret;
	if (flags & BTRFS_EXTENT_FLAG_TREE_BLOCK)
		return -EINVAL;

	extent_item_pos = logical - found_key.objectid;
	ret = iterate_extent_inodes(fs_info, found_key.objectid,
					extent_item_pos, search_commit_root,
					iterate, ctx);

	return ret;
}

typedef int (iterate_irefs_t)(u64 parent, u32 name_len, unsigned long name_off,
			      struct extent_buffer *eb, void *ctx);

static int iterate_inode_refs(u64 inum, struct btrfs_root *fs_root,
			      struct btrfs_path *path,
			      iterate_irefs_t *iterate, void *ctx)
{
	int ret = 0;
	int slot;
	u32 cur;
	u32 len;
	u32 name_len;
	u64 parent = 0;
	int found = 0;
	struct extent_buffer *eb;
	struct btrfs_item *item;
	struct btrfs_inode_ref *iref;
	struct btrfs_key found_key;

	while (!ret) {
		ret = btrfs_find_item(fs_root, path, inum,
				parent ? parent + 1 : 0, BTRFS_INODE_REF_KEY,
				&found_key);

		if (ret < 0)
			break;
		if (ret) {
			ret = found ? 0 : -ENOENT;
			break;
		}
		++found;

		parent = found_key.offset;
		slot = path->slots[0];
		eb = btrfs_clone_extent_buffer(path->nodes[0]);
		if (!eb) {
			ret = -ENOMEM;
			break;
		}
		extent_buffer_get(eb);
		btrfs_tree_read_lock(eb);
		btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
		btrfs_release_path(path);

		item = btrfs_item_nr(slot);
		iref = btrfs_item_ptr(eb, slot, struct btrfs_inode_ref);

		for (cur = 0; cur < btrfs_item_size(eb, item); cur += len) {
			name_len = btrfs_inode_ref_name_len(eb, iref);
			/* path must be released before calling iterate()! */
			pr_debug("following ref at offset %u for inode %llu in "
				 "tree %llu\n", cur, found_key.objectid,
				 fs_root->objectid);
			ret = iterate(parent, name_len,
				      (unsigned long)(iref + 1), eb, ctx);
			if (ret)
				break;
			len = sizeof(*iref) + name_len;
			iref = (struct btrfs_inode_ref *)((char *)iref + len);
		}
		btrfs_tree_read_unlock_blocking(eb);
		free_extent_buffer(eb);
	}

	btrfs_release_path(path);

	return ret;
}

static int iterate_inode_extrefs(u64 inum, struct btrfs_root *fs_root,
				 struct btrfs_path *path,
				 iterate_irefs_t *iterate, void *ctx)
{
	int ret;
	int slot;
	u64 offset = 0;
	u64 parent;
	int found = 0;
	struct extent_buffer *eb;
	struct btrfs_inode_extref *extref;
	u32 item_size;
	u32 cur_offset;
	unsigned long ptr;

	while (1) {
		ret = btrfs_find_one_extref(fs_root, inum, offset, path, &extref,
					    &offset);
		if (ret < 0)
			break;
		if (ret) {
			ret = found ? 0 : -ENOENT;
			break;
		}
		++found;

		slot = path->slots[0];
		eb = btrfs_clone_extent_buffer(path->nodes[0]);
		if (!eb) {
			ret = -ENOMEM;
			break;
		}
		extent_buffer_get(eb);

		btrfs_tree_read_lock(eb);
		btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
		btrfs_release_path(path);

		item_size = btrfs_item_size_nr(eb, slot);
		ptr = btrfs_item_ptr_offset(eb, slot);
		cur_offset = 0;

		while (cur_offset < item_size) {
			u32 name_len;

			extref = (struct btrfs_inode_extref *)(ptr + cur_offset);
			parent = btrfs_inode_extref_parent(eb, extref);
			name_len = btrfs_inode_extref_name_len(eb, extref);
			ret = iterate(parent, name_len,
				      (unsigned long)&extref->name, eb, ctx);
			if (ret)
				break;

			cur_offset += btrfs_inode_extref_name_len(eb, extref);
			cur_offset += sizeof(*extref);
		}
		btrfs_tree_read_unlock_blocking(eb);
		free_extent_buffer(eb);

		offset++;
	}

	btrfs_release_path(path);

	return ret;
}

static int iterate_irefs(u64 inum, struct btrfs_root *fs_root,
			 struct btrfs_path *path, iterate_irefs_t *iterate,
			 void *ctx)
{
	int ret;
	int found_refs = 0;

	ret = iterate_inode_refs(inum, fs_root, path, iterate, ctx);
	if (!ret)
		++found_refs;
	else if (ret != -ENOENT)
		return ret;

	ret = iterate_inode_extrefs(inum, fs_root, path, iterate, ctx);
	if (ret == -ENOENT && found_refs)
		return 0;

	return ret;
}

/*
 * returns 0 if the path could be dumped (probably truncated)
 * returns <0 in case of an error
 */
static int inode_to_path(u64 inum, u32 name_len, unsigned long name_off,
			 struct extent_buffer *eb, void *ctx)
{
	struct inode_fs_paths *ipath = ctx;
	char *fspath;
	char *fspath_min;
	int i = ipath->fspath->elem_cnt;
	const int s_ptr = sizeof(char *);
	u32 bytes_left;

	bytes_left = ipath->fspath->bytes_left > s_ptr ?
					ipath->fspath->bytes_left - s_ptr : 0;

	fspath_min = (char *)ipath->fspath->val + (i + 1) * s_ptr;
	fspath = btrfs_ref_to_path(ipath->fs_root, ipath->btrfs_path, name_len,
				   name_off, eb, inum, fspath_min, bytes_left);
	if (IS_ERR(fspath))
		return PTR_ERR(fspath);

	if (fspath > fspath_min) {
		ipath->fspath->val[i] = (u64)(unsigned long)fspath;
		++ipath->fspath->elem_cnt;
		ipath->fspath->bytes_left = fspath - fspath_min;
	} else {
		++ipath->fspath->elem_missed;
		ipath->fspath->bytes_missing += fspath_min - fspath;
		ipath->fspath->bytes_left = 0;
	}

	return 0;
}

/*
 * this dumps all file system paths to the inode into the ipath struct, provided
 * is has been created large enough. each path is zero-terminated and accessed
 * from ipath->fspath->val[i].
 * when it returns, there are ipath->fspath->elem_cnt number of paths available
 * in ipath->fspath->val[]. when the allocated space wasn't sufficient, the
 * number of missed paths in recored in ipath->fspath->elem_missed, otherwise,
 * it's zero. ipath->fspath->bytes_missing holds the number of bytes that would
 * have been needed to return all paths.
 */
int paths_from_inode(u64 inum, struct inode_fs_paths *ipath)
{
	return iterate_irefs(inum, ipath->fs_root, ipath->btrfs_path,
			     inode_to_path, ipath);
}

struct btrfs_data_container *init_data_container(u32 total_bytes)
{
	struct btrfs_data_container *data;
	size_t alloc_bytes;

	alloc_bytes = max_t(size_t, total_bytes, sizeof(*data));
	data = vmalloc(alloc_bytes);
	if (!data)
		return ERR_PTR(-ENOMEM);

	if (total_bytes >= sizeof(*data)) {
		data->bytes_left = total_bytes - sizeof(*data);
		data->bytes_missing = 0;
	} else {
		data->bytes_missing = sizeof(*data) - total_bytes;
		data->bytes_left = 0;
	}

	data->elem_cnt = 0;
	data->elem_missed = 0;

	return data;
}

/*
 * allocates space to return multiple file system paths for an inode.
 * total_bytes to allocate are passed, note that space usable for actual path
 * information will be total_bytes - sizeof(struct inode_fs_paths).
 * the returned pointer must be freed with free_ipath() in the end.
 */
struct inode_fs_paths *init_ipath(s32 total_bytes, struct btrfs_root *fs_root,
					struct btrfs_path *path)
{
	struct inode_fs_paths *ifp;
	struct btrfs_data_container *fspath;

	fspath = init_data_container(total_bytes);
	if (IS_ERR(fspath))
		return (void *)fspath;

	ifp = kmalloc(sizeof(*ifp), GFP_NOFS);
	if (!ifp) {
		kfree(fspath);
		return ERR_PTR(-ENOMEM);
	}

	ifp->btrfs_path = path;
	ifp->fspath = fspath;
	ifp->fs_root = fs_root;

	return ifp;
}

void free_ipath(struct inode_fs_paths *ipath)
{
	if (!ipath)
		return;
	vfree(ipath->fspath);
	kfree(ipath);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright (C) 2011 STRATO.  All rights reserved.
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

#ifndef __BTRFS_BACKREF__
#define __BTRFS_BACKREF__

#include <linux/btrfs.h>
#include "ulist.h"
#include "extent_io.h"

struct inode_fs_paths {
	struct btrfs_path		*btrfs_path;
	struct btrfs_root		*fs_root;
	struct btrfs_data_container	*fspath;
};

typedef int (iterate_extent_inodes_t)(u64 inum, u64 offset, u64 root,
		void *ctx);

int extent_from_logical(struct btrfs_fs_info *fs_info, u64 logical,
			struct btrfs_path *path, struct btrfs_key *found_key,
			u64 *flags);

int tree_backref_for_extent(unsigned long *ptr, struct extent_buffer *eb,
			    struct btrfs_key *key, struct btrfs_extent_item *ei,
			    u32 item_size, u64 *out_root, u8 *out_level);

int iterate_extent_inodes(struct btrfs_fs_info *fs_info,
				u64 extent_item_objectid,
				u64 extent_offset, int search_commit_root,
				iterate_extent_inodes_t *iterate, void *ctx);

int iterate_inodes_from_logical(u64 logical, struct btrfs_fs_info *fs_info,
				struct btrfs_path *path,
				iterate_extent_inodes_t *iterate, void *ctx);

int paths_from_inode(u64 inum, struct inode_fs_paths *ipath);

int btrfs_find_all_roots(struct btrfs_trans_handle *trans,
			 struct btrfs_fs_info *fs_info, u64 bytenr,
			 u64 time_seq, struct ulist **roots);
char *btrfs_ref_to_path(struct btrfs_root *fs_root, struct btrfs_path *path,
			u32 name_len, unsigned long name_off,
			struct extent_buffer *eb_in, u64 parent,
			char *dest, u32 size);

struct btrfs_data_container *init_data_container(u32 total_bytes);
struct inode_fs_paths *init_ipath(s32 total_bytes, struct btrfs_root *fs_root,
					struct btrfs_path *path);
void free_ipath(struct inode_fs_paths *ipath);

int btrfs_find_one_extref(struct btrfs_root *root, u64 inode_objectid,
			  u64 start_off, struct btrfs_path *path,
			  struct btrfs_inode_extref **ret_extref,
			  u64 *found_off);
int btrfs_check_shared(struct btrfs_trans_handle *trans,
		       struct btrfs_fs_info *fs_info, u64 root_objectid,
		       u64 inum, u64 bytenr);

int __init btrfs_prelim_ref_init(void);
void btrfs_prelim_ref_exit(void);
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
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

#ifndef __BTRFS_I__
#define __BTRFS_I__

#include <linux/hash.h>
#include "extent_map.h"
#include "extent_io.h"
#include "ordered-data.h"
#include "delayed-inode.h"

/*
 * ordered_data_close is set by truncate when a file that used
 * to have good data has been truncated to zero.  When it is set
 * the btrfs file release call will add this inode to the
 * ordered operations list so that we make sure to flush out any
 * new data the application may have written before commit.
 */
#define BTRFS_INODE_ORDERED_DATA_CLOSE		0
#define BTRFS_INODE_ORPHAN_META_RESERVED	1
#define BTRFS_INODE_DUMMY			2
#define BTRFS_INODE_IN_DEFRAG			3
#define BTRFS_INODE_DELALLOC_META_RESERVED	4
#define BTRFS_INODE_HAS_ORPHAN_ITEM		5
#define BTRFS_INODE_HAS_ASYNC_EXTENT		6
#define BTRFS_INODE_NEEDS_FULL_SYNC		7
#define BTRFS_INODE_COPY_EVERYTHING		8
#define BTRFS_INODE_IN_DELALLOC_LIST		9
#define BTRFS_INODE_READDIO_NEED_LOCK		10
#define BTRFS_INODE_HAS_PROPS		        11
/*
 * The following 3 bits are meant only for the btree inode.
 * When any of them is set, it means an error happened while writing an
 * extent buffer belonging to:
 * 1) a non-log btree
 * 2) a log btree and first log sub-transaction
 * 3) a log btree and second log sub-transaction
 */
#define BTRFS_INODE_BTREE_ERR		        12
#define BTRFS_INODE_BTREE_LOG1_ERR		13
#define BTRFS_INODE_BTREE_LOG2_ERR		14

/* in memory btrfs inode */
struct btrfs_inode {
	/* which subvolume this inode belongs to */
	struct btrfs_root *root;

	/* key used to find this inode on disk.  This is used by the code
	 * to read in roots of subvolumes
	 */
	struct btrfs_key location;

	/*
	 * Lock for counters and all fields used to determine if the inode is in
	 * the log or not (last_trans, last_sub_trans, last_log_commit,
	 * logged_trans).
	 */
	spinlock_t lock;

	/* the extent_tree has caches of all the extent mappings to disk */
	struct extent_map_tree extent_tree;

	/* the io_tree does range state (DIRTY, LOCKED etc) */
	struct extent_io_tree io_tree;

	/* special utility tree used to record which mirrors have already been
	 * tried when checksums fail for a given block
	 */
	struct extent_io_tree io_failure_tree;

	/* held while logging the inode in tree-log.c */
	struct mutex log_mutex;

	/* held while doing delalloc reservations */
	struct mutex delalloc_mutex;

	/* used to order data wrt metadata */
	struct btrfs_ordered_inode_tree ordered_tree;

	/* list of all the delalloc inodes in the FS.  There are times we need
	 * to write all the delalloc pages to disk, and this list is used
	 * to walk them all.
	 */
	struct list_head delalloc_inodes;

	/* node for the red-black tree that links inodes in subvolume root */
	struct rb_node rb_node;

	unsigned long runtime_flags;

	/* Keep track of who's O_SYNC/fsyncing currently */
	atomic_t sync_writers;

	/* full 64 bit generation number, struct vfs_inode doesn't have a big
	 * enough field for this.
	 */
	u64 generation;

	/*
	 * transid of the trans_handle that last modified this inode
	 */
	u64 last_trans;

	/*
	 * transid that last logged this inode
	 */
	u64 logged_trans;

	/*
	 * log transid when this inode was last modified
	 */
	int last_sub_trans;

	/* a local copy of root's last_log_commit */
	int last_log_commit;

	/* total number of bytes pending delalloc, used by stat to calc the
	 * real block usage of the file
	 */
	u64 delalloc_bytes;

	/*
	 * total number of bytes pending defrag, used by stat to check whether
	 * it needs COW.
	 */
	u64 defrag_bytes;

	/*
	 * the size of the file stored in the metadata on disk.  data=ordered
	 * means the in-memory i_size might be larger than the size on disk
	 * because not all the blocks are written yet.
	 */
	u64 disk_i_size;

	/*
	 * if this is a directory then index_cnt is the counter for the index
	 * number for new files that are created
	 */
	u64 index_cnt;

	/* Cache the directory index number to speed the dir/file remove */
	u64 dir_index;

	/* the fsync log has some corner cases that mean we have to check
	 * directories to see if any unlinks have been done before
	 * the directory was logged.  See tree-log.c for all the
	 * details
	 */
	u64 last_unlink_trans;

	/*
	 * Number of bytes outstanding that are going to need csums.  This is
	 * used in ENOSPC accounting.
	 */
	u64 csum_bytes;

	/* flags field from the on disk inode */
	u32 flags;

	/*
	 * Counters to keep track of the number of extent item's we may use due
	 * to delalloc and such.  outstanding_extents is the number of extent
	 * items we think we'll end up using, and reserved_extents is the number
	 * of extent items we've reserved metadata for.
	 */
	unsigned outstanding_extents;
	unsigned reserved_extents;

	/*
	 * always compress this one file
	 */
	unsigned force_compress;

	struct btrfs_delayed_node *delayed_node;

	/* File creation time. */
	struct timespec i_otime;

	struct inode vfs_inode;
};

extern unsigned char btrfs_filetype_table[];

static inline struct btrfs_inode *BTRFS_I(struct inode *inode)
{
	return container_of(inode, struct btrfs_inode, vfs_inode);
}

static inline unsigned long btrfs_inode_hash(u64 objectid,
					     const struct btrfs_root *root)
{
	u64 h = objectid ^ (root->objectid * GOLDEN_RATIO_PRIME);

#if BITS_PER_LONG == 32
	h = (h >> 32) ^ (h & 0xffffffff);
#endif

	return (unsigned long)h;
}

static inline void btrfs_insert_inode_hash(struct inode *inode)
{
	unsigned long h = btrfs_inode_hash(inode->i_ino, BTRFS_I(inode)->root);

	__insert_inode_hash(inode, h);
}

static inline u64 btrfs_ino(struct inode *inode)
{
	u64 ino = BTRFS_I(inode)->location.objectid;

	/*
	 * !ino: btree_inode
	 * type == BTRFS_ROOT_ITEM_KEY: subvol dir
	 */
	if (!ino || BTRFS_I(inode)->location.type == BTRFS_ROOT_ITEM_KEY)
		ino = inode->i_ino;
	return ino;
}

static inline void btrfs_i_size_write(struct inode *inode, u64 size)
{
	i_size_write(inode, size);
	BTRFS_I(inode)->disk_i_size = size;
}

static inline bool btrfs_is_free_space_inode(struct inode *inode)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;

	if (root == root->fs_info->tree_root &&
	    btrfs_ino(inode) != BTRFS_BTREE_INODE_OBJECTID)
		return true;
	if (BTRFS_I(inode)->location.objectid == BTRFS_FREE_INO_OBJECTID)
		return true;
	return false;
}

static inline int btrfs_inode_in_log(struct inode *inode, u64 generation)
{
	int ret = 0;

	spin_lock(&BTRFS_I(inode)->lock);
	if (BTRFS_I(inode)->logged_trans == generation &&
	    BTRFS_I(inode)->last_sub_trans <=
	    BTRFS_I(inode)->last_log_commit &&
	    BTRFS_I(inode)->last_sub_trans <=
	    BTRFS_I(inode)->root->last_log_commit) {
		/*
		 * After a ranged fsync we might have left some extent maps
		 * (that fall outside the fsync's range). So return false
		 * here if the list isn't empty, to make sure btrfs_log_inode()
		 * will be called and process those extent maps.
		 */
		smp_mb();
		if (list_empty(&BTRFS_I(inode)->extent_tree.modified_extents))
			ret = 1;
	}
	spin_unlock(&BTRFS_I(inode)->lock);
	return ret;
}

#define BTRFS_DIO_ORIG_BIO_SUBMITTED	0x1

struct btrfs_dio_private {
	struct inode *inode;
	unsigned long flags;
	u64 logical_offset;
	u64 disk_bytenr;
	u64 bytes;
	void *private;

	/* number of bios pending for this dio */
	atomic_t pending_bios;

	/* IO errors */
	int errors;

	/* orig_bio is our btrfs_io_bio */
	struct bio *orig_bio;

	/* dio_bio came from fs/direct-io.c */
	struct bio *dio_bio;

	/*
	 * The original bio may be splited to several sub-bios, this is
	 * done during endio of sub-bios
	 */
	int (*subio_endio)(struct inode *, struct btrfs_io_bio *, int);
};

/*
 * Disable DIO read nolock optimization, so new dio readers will be forced
 * to grab i_mutex. It is used to avoid the endless truncate due to
 * nonlocked dio read.
 */
static inline void btrfs_inode_block_unlocked_dio(struct inode *inode)
{
	set_bit(BTRFS_INODE_READDIO_NEED_LOCK, &BTRFS_I(inode)->runtime_flags);
	smp_mb();
}

static inline void btrfs_inode_resume_unlocked_dio(struct inode *inode)
{
	smp_mb__before_atomic();
	clear_bit(BTRFS_INODE_READDIO_NEED_LOCK,
		  &BTRFS_I(inode)->runtime_flags);
}

bool btrfs_page_exists_in_range(struct inode *inode, loff_t start, loff_t end);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * Copyright (C) STRATO AG 2011.  All rights reserved.
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

#if !defined(__BTRFS_CHECK_INTEGRITY__)
#define __BTRFS_CHECK_INTEGRITY__

#ifdef CONFIG_BTRFS_FS_CHECK_INTEGRITY
int btrfsic_submit_bh(int rw, struct buffer_head *bh);
void btrfsic_submit_bio(int rw, struct bio *bio);
int btrfsic_submit_bio_wait(int rw, struct bio *bio);
#else
#define btrfsic_submit_bh submit_bh
#define btrfsic_submit_bio submit_bio
#define btrfsic_submit_bio_wait submit_bio_wait
#endif

int btrfsic_mount(struct btrfs_root *root,
		  struct btrfs_fs_devices *fs_devices,
		  int including_extent_data, u32 print_mask);
void btrfsic_unmount(struct btrfs_root *root,
		     struct btrfs_fs_devices *fs_devices);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * Copyright (C) 2008 Oracle.  All rights reserved.
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

#include <linux/kernel.h>
#include <linux/bio.h>
#include <linux/buffer_head.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/mpage.h>
#include <linux/swap.h>
#include <linux/writeback.h>
#include <linux/bit_spinlock.h>
#include <linux/slab.h>
#include "ctree.h"
#include "disk-io.h"
#include "transaction.h"
#include "btrfs_inode.h"
#include "volumes.h"
#include "ordered-data.h"
#include "compression.h"
#include "extent_io.h"
#include "extent_map.h"

struct compressed_bio {
	/* number of bios pending for this compressed extent */
	atomic_t pending_bios;

	/* the pages with the compressed data on them */
	struct page **compressed_pages;

	/* inode that owns this data */
	struct inode *inode;

	/* starting offset in the inode for our pages */
	u64 start;

	/* number of bytes in the inode we're working on */
	unsigned long len;

	/* number of bytes on disk */
	unsigned long compressed_len;

	/* the compression algorithm for this bio */
	int compress_type;

	/* number of compressed pages in the array */
	unsigned long nr_pages;

	/* IO errors */
	int errors;
	int mirror_num;

	/* for reads, this is the bio we are copying the data into */
	struct bio *orig_bio;

	/*
	 * the start of a variable length array of checksums only
	 * used by reads
	 */
	u32 sums;
};

static int btrfs_decompress_biovec(int type, struct page **pages_in,
				   u64 disk_start, struct bio_vec *bvec,
				   int vcnt, size_t srclen);

static inline int compressed_bio_size(struct btrfs_root *root,
				      unsigned long disk_size)
{
	u16 csum_size = btrfs_super_csum_size(root->fs_info->super_copy);

	return sizeof(struct compressed_bio) +
		(DIV_ROUND_UP(disk_size, root->sectorsize)) * csum_size;
}

static struct bio *compressed_bio_alloc(struct block_device *bdev,
					u64 first_byte, gfp_t gfp_flags)
{
	return btrfs_bio_alloc(bdev, first_byte >> 9, BIO_MAX_PAGES, gfp_flags);
}

static int check_compressed_csum(struct inode *inode,
				 struct compressed_bio *cb,
				 u64 disk_start)
{
	int ret;
	struct page *page;
	unsigned long i;
	char *kaddr;
	u32 csum;
	u32 *cb_sum = &cb->sums;

	if (BTRFS_I(inode)->flags & BTRFS_INODE_NODATASUM)
		return 0;

	for (i = 0; i < cb->nr_pages; i++) {
		page = cb->compressed_pages[i];
		csum = ~(u32)0;

		kaddr = kmap_atomic(page);
		csum = btrfs_csum_data(kaddr, csum, PAGE_CACHE_SIZE);
		btrfs_csum_final(csum, (char *)&csum);
		kunmap_atomic(kaddr);

		if (csum != *cb_sum) {
			btrfs_info(BTRFS_I(inode)->root->fs_info,
			   "csum failed ino %llu extent %llu csum %u wanted %u mirror %d",
			   btrfs_ino(inode), disk_start, csum, *cb_sum,
			   cb->mirror_num);
			ret = -EIO;
			goto fail;
		}
		cb_sum++;

	}
	ret = 0;
fail:
	return ret;
}

/* when we finish reading compressed pages from the disk, we
 * decompress them and then run the bio end_io routines on the
 * decompressed pages (in the inode address space).
 *
 * This allows the checksumming and other IO error handling routines
 * to work normally
 *
 * The compressed pages are freed here, and it must be run
 * in process context
 */
static void end_compressed_bio_read(struct bio *bio)
{
	struct compressed_bio *cb = bio->bi_private;
	struct inode *inode;
	struct page *page;
	unsigned long index;
	int ret;

	if (bio->bi_error)
		cb->errors = 1;

	/* if there are more bios still pending for this compressed
	 * extent, just exit
	 */
	if (!atomic_dec_and_test(&cb->pending_bios))
		goto out;

	inode = cb->inode;
	ret = check_compressed_csum(inode, cb,
				    (u64)bio->bi_iter.bi_sector << 9);
	if (ret)
		goto csum_failed;

	/* ok, we're the last bio for this extent, lets start
	 * the decompression.
	 */
	ret = btrfs_decompress_biovec(cb->compress_type,
				      cb->compressed_pages,
				      cb->start,
				      cb->orig_bio->bi_io_vec,
				      cb->orig_bio->bi_vcnt,
				      cb->compressed_len);
csum_failed:
	if (ret)
		cb->errors = 1;

	/* release the compressed pages */
	index = 0;
	for (index = 0; index < cb->nr_pages; index++) {
		page = cb->compressed_pages[index];
		page->mapping = NULL;
		page_cache_release(page);
	}

	/* do io completion on the original bio */
	if (cb->errors) {
		bio_io_error(cb->orig_bio);
	} else {
		int i;
		struct bio_vec *bvec;

		/*
		 * we have verified the checksum already, set page
		 * checked so the end_io handlers know about it
		 */
		bio_for_each_segment_all(bvec, cb->orig_bio, i)
			SetPageChecked(bvec->bv_page);

		bio_endio(cb->orig_bio);
	}

	/* finally free the cb struct */
	kfree(cb->compressed_pages);
	kfree(cb);
out:
	bio_put(bio);
}

/*
 * Clear the writeback bits on all of the file
 * pages for a compressed write
 */
static noinline void end_compressed_writeback(struct inode *inode,
					      const struct compressed_bio *cb)
{
	unsigned long index = cb->start >> PAGE_CACHE_SHIFT;
	unsigned long end_index = (cb->start + cb->len - 1) >> PAGE_CACHE_SHIFT;
	struct page *pages[16];
	unsigned long nr_pages = end_index - index + 1;
	int i;
	int ret;

	if (cb->errors)
		mapping_set_error(inode->i_mapping, -EIO);

	while (nr_pages > 0) {
		ret = find_get_pages_contig(inode->i_mapping, index,
				     min_t(unsigned long,
				     nr_pages, ARRAY_SIZE(pages)), pages);
		if (ret == 0) {
			nr_pages -= 1;
			index += 1;
			continue;
		}
		for (i = 0; i < ret; i++) {
			if (cb->errors)
				SetPageError(pages[i]);
			end_page_writeback(pages[i]);
			page_cache_release(pages[i]);
		}
		nr_pages -= ret;
		index += ret;
	}
	/* the inode may be gone now */
}

/*
 * do the cleanup once all the compressed pages hit the disk.
 * This will clear writeback on the file pages and free the compressed
 * pages.
 *
 * This also calls the writeback end hooks for the file pages so that
 * metadata and checksums can be updated in the file.
 */
static void end_compressed_bio_write(struct bio *bio)
{
	struct extent_io_tree *tree;
	struct compressed_bio *cb = bio->bi_private;
	struct inode *inode;
	struct page *page;
	unsigned long index;

	if (bio->bi_error)
		cb->errors = 1;

	/* if there are more bios still pending for this compressed
	 * extent, just exit
	 */
	if (!atomic_dec_and_test(&cb->pending_bios))
		goto out;

	/* ok, we're the last bio for this extent, step one is to
	 * call back into the FS and do all the end_io operations
	 */
	inode = cb->inode;
	tree = &BTRFS_I(inode)->io_tree;
	cb->compressed_pages[0]->mapping = cb->inode->i_mapping;
	tree->ops->writepage_end_io_hook(cb->compressed_pages[0],
					 cb->start,
					 cb->start + cb->len - 1,
					 NULL,
					 !cb->errors);
	cb->compressed_pages[0]->mapping = NULL;

	end_compressed_writeback(inode, cb);
	/* note, our inode could be gone now */

	/*
	 * release the compressed pages, these came from alloc_page and
	 * are not attached to the inode at all
	 */
	index = 0;
	for (index = 0; index < cb->nr_pages; index++) {
		page = cb->compressed_pages[index];
		page->mapping = NULL;
		page_cache_release(page);
	}

	/* finally free the cb struct */
	kfree(cb->compressed_pages);
	kfree(cb);
out:
	bio_put(bio);
}

/*
 * worker function to build and submit bios for previously compressed pages.
 * The corresponding pages in the inode should be marked for writeback
 * and the compressed pages should have a reference on them for dropping
 * when the IO is complete.
 *
 * This also checksums the file bytes and gets things ready for
 * the end io hooks.
 */
int btrfs_submit_compressed_write(struct inode *inode, u64 start,
				 unsigned long len, u64 disk_start,
				 unsigned long compressed_len,
				 struct page **compressed_pages,
				 unsigned long nr_pages)
{
	struct bio *bio = NULL;
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct compressed_bio *cb;
	unsigned long bytes_left;
	struct extent_io_tree *io_tree = &BTRFS_I(inode)->io_tree;
	int pg_index = 0;
	struct page *page;
	u64 first_byte = disk_start;
	struct block_device *bdev;
	int ret;
	int skip_sum = BTRFS_I(inode)->flags & BTRFS_INODE_NODATASUM;

	WARN_ON(start & ((u64)PAGE_CACHE_SIZE - 1));
	cb = kmalloc(compressed_bio_size(root, compressed_len), GFP_NOFS);
	if (!cb)
		return -ENOMEM;
	atomic_set(&cb->pending_bios, 0);
	cb->errors = 0;
	cb->inode = inode;
	cb->start = start;
	cb->len = len;
	cb->mirror_num = 0;
	cb->compressed_pages = compressed_pages;
	cb->compressed_len = compressed_len;
	cb->orig_bio = NULL;
	cb->nr_pages = nr_pages;

	bdev = BTRFS_I(inode)->root->fs_info->fs_devices->latest_bdev;

	bio = compressed_bio_alloc(bdev, first_byte, GFP_NOFS);
	if (!bio) {
		kfree(cb);
		return -ENOMEM;
	}
	bio->bi_private = cb;
	bio->bi_end_io = end_compressed_bio_write;
	atomic_inc(&cb->pending_bios);

	/* create and submit bios for the compressed pages */
	bytes_left = compressed_len;
	for (pg_index = 0; pg_index < cb->nr_pages; pg_index++) {
		page = compressed_pages[pg_index];
		page->mapping = inode->i_mapping;
		if (bio->bi_iter.bi_size)
			ret = io_tree->ops->merge_bio_hook(WRITE, page, 0,
							   PAGE_CACHE_SIZE,
							   bio, 0);
		else
			ret = 0;

		page->mapping = NULL;
		if (ret || bio_add_page(bio, page, PAGE_CACHE_SIZE, 0) <
		    PAGE_CACHE_SIZE) {
			bio_get(bio);

			/*
			 * inc the count before we submit the bio so
			 * we know the end IO handler won't happen before
			 * we inc the count.  Otherwise, the cb might get
			 * freed before we're done setting it up
			 */
			atomic_inc(&cb->pending_bios);
			ret = btrfs_bio_wq_end_io(root->fs_info, bio,
					BTRFS_WQ_ENDIO_DATA);
			BUG_ON(ret); /* -ENOMEM */

			if (!skip_sum) {
				ret = btrfs_csum_one_bio(root, inode, bio,
							 start, 1);
				BUG_ON(ret); /* -ENOMEM */
			}

			ret = btrfs_map_bio(root, WRITE, bio, 0, 1);
			BUG_ON(ret); /* -ENOMEM */

			bio_put(bio);

			bio = compressed_bio_alloc(bdev, first_byte, GFP_NOFS);
			BUG_ON(!bio);
			bio->bi_private = cb;
			bio->bi_end_io = end_compressed_bio_write;
			bio_add_page(bio, page, PAGE_CACHE_SIZE, 0);
		}
		if (bytes_left < PAGE_CACHE_SIZE) {
			btrfs_info(BTRFS_I(inode)->root->fs_info,
					"bytes left %lu compress len %lu nr %lu",
			       bytes_left, cb->compressed_len, cb->nr_pages);
		}
		bytes_left -= PAGE_CACHE_SIZE;
		first_byte += PAGE_CACHE_SIZE;
		cond_resched();
	}
	bio_get(bio);

	ret = btrfs_bio_wq_end_io(root->fs_info, bio, BTRFS_WQ_ENDIO_DATA);
	BUG_ON(ret); /* -ENOMEM */

	if (!skip_sum) {
		ret = btrfs_csum_one_bio(root, inode, bio, start, 1);
		BUG_ON(ret); /* -ENOMEM */
	}

	ret = btrfs_map_bio(root, WRITE, bio, 0, 1);
	BUG_ON(ret); /* -ENOMEM */

	bio_put(bio);
	return 0;
}

static noinline int add_ra_bio_pages(struct inode *inode,
				     u64 compressed_end,
				     struct compressed_bio *cb)
{
	unsigned long end_index;
	unsigned long pg_index;
	u64 last_offset;
	u64 isize = i_size_read(inode);
	int ret;
	struct page *page;
	unsigned long nr_pages = 0;
	struct extent_map *em;
	struct address_space *mapping = inode->i_mapping;
	struct extent_map_tree *em_tree;
	struct extent_io_tree *tree;
	u64 end;
	int misses = 0;

	page = cb->orig_bio->bi_io_vec[cb->orig_bio->bi_vcnt - 1].bv_page;
	last_offset = (page_offset(page) + PAGE_CACHE_SIZE);
	em_tree = &BTRFS_I(inode)->extent_tree;
	tree = &BTRFS_I(inode)->io_tree;

	if (isize == 0)
		return 0;

	end_index = (i_size_read(inode) - 1) >> PAGE_CACHE_SHIFT;

	while (last_offset < compressed_end) {
		pg_index = last_offset >> PAGE_CACHE_SHIFT;

		if (pg_index > end_index)
			break;

		rcu_read_lock();
		page = radix_tree_lookup(&mapping->page_tree, pg_index);
		rcu_read_unlock();
		if (page && !radix_tree_exceptional_entry(page)) {
			misses++;
			if (misses > 4)
				break;
			goto next;
		}

		page = __page_cache_alloc(mapping_gfp_constraint(mapping,
								 ~__GFP_FS));
		if (!page)
			break;

		if (add_to_page_cache_lru(page, mapping, pg_index, GFP_NOFS)) {
			page_cache_release(page);
			goto next;
		}

		end = last_offset + PAGE_CACHE_SIZE - 1;
		/*
		 * at this point, we have a locked page in the page cache
		 * for these bytes in the file.  But, we have to make
		 * sure they map to this compressed extent on disk.
		 */
		set_page_extent_mapped(page);
		lock_extent(tree, last_offset, end);
		read_lock(&em_tree->lock);
		em = lookup_extent_mapping(em_tree, last_offset,
					   PAGE_CACHE_SIZE);
		read_unlock(&em_tree->lock);

		if (!em || last_offset < em->start ||
		    (last_offset + PAGE_CACHE_SIZE > extent_map_end(em)) ||
		    (em->block_start >> 9) != cb->orig_bio->bi_iter.bi_sector) {
			free_extent_map(em);
			unlock_extent(tree, last_offset, end);
			unlock_page(page);
			page_cache_release(page);
			break;
		}
		free_extent_map(em);

		if (page->index == end_index) {
			char *userpage;
			size_t zero_offset = isize & (PAGE_CACHE_SIZE - 1);

			if (zero_offset) {
				int zeros;
				zeros = PAGE_CACHE_SIZE - zero_offset;
				userpage = kmap_atomic(page);
				memset(userpage + zero_offset, 0, zeros);
				flush_dcache_page(page);
				kunmap_atomic(userpage);
			}
		}

		ret = bio_add_page(cb->orig_bio, page,
				   PAGE_CACHE_SIZE, 0);

		if (ret == PAGE_CACHE_SIZE) {
			nr_pages++;
			page_cache_release(page);
		} else {
			unlock_extent(tree, last_offset, end);
			unlock_page(page);
			page_cache_release(page);
			break;
		}
next:
		last_offset += PAGE_CACHE_SIZE;
	}
	return 0;
}

/*
 * for a compressed read, the bio we get passed has all the inode pages
 * in it.  We don't actually do IO on those pages but allocate new ones
 * to hold the compressed pages on disk.
 *
 * bio->bi_iter.bi_sector points to the compressed extent on disk
 * bio->bi_io_vec points to all of the inode pages
 * bio->bi_vcnt is a count of pages
 *
 * After the compressed pages are read, we copy the bytes into the
 * bio we were passed and then call the bio end_io calls
 */
int btrfs_submit_compressed_read(struct inode *inode, struct bio *bio,
				 int mirror_num, unsigned long bio_flags)
{
	struct extent_io_tree *tree;
	struct extent_map_tree *em_tree;
	struct compressed_bio *cb;
	struct btrfs_root *root = BTRFS_I(inode)->root;
	unsigned long uncompressed_len = bio->bi_vcnt * PAGE_CACHE_SIZE;
	unsigned long compressed_len;
	unsigned long nr_pages;
	unsigned long pg_index;
	struct page *page;
	struct block_device *bdev;
	struct bio *comp_bio;
	u64 cur_disk_byte = (u64)bio->bi_iter.bi_sector << 9;
	u64 em_len;
	u64 em_start;
	struct extent_map *em;
	int ret = -ENOMEM;
	int faili = 0;
	u32 *sums;

	tree = &BTRFS_I(inode)->io_tree;
	em_tree = &BTRFS_I(inode)->extent_tree;

	/* we need the actual starting offset of this extent in the file */
	read_lock(&em_tree->lock);
	em = lookup_extent_mapping(em_tree,
				   page_offset(bio->bi_io_vec->bv_page),
				   PAGE_CACHE_SIZE);
	read_unlock(&em_tree->lock);
	if (!em)
		return -EIO;

	compressed_len = em->block_len;
	cb = kmalloc(compressed_bio_size(root, compressed_len), GFP_NOFS);
	if (!cb)
		goto out;

	atomic_set(&cb->pending_bios, 0);
	cb->errors = 0;
	cb->inode = inode;
	cb->mirror_num = mirror_num;
	sums = &cb->sums;

	cb->start = em->orig_start;
	em_len = em->len;
	em_start = em->start;

	free_extent_map(em);
	em = NULL;

	cb->len = uncompressed_len;
	cb->compressed_len = compressed_len;
	cb->compress_type = extent_compress_type(bio_flags);
	cb->orig_bio = bio;

	nr_pages = DIV_ROUND_UP(compressed_len, PAGE_CACHE_SIZE);
	cb->compressed_pages = kcalloc(nr_pages, sizeof(struct page *),
				       GFP_NOFS);
	if (!cb->compressed_pages)
		goto fail1;

	bdev = BTRFS_I(inode)->root->fs_info->fs_devices->latest_bdev;

	for (pg_index = 0; pg_index < nr_pages; pg_index++) {
		cb->compressed_pages[pg_index] = alloc_page(GFP_NOFS |
							      __GFP_HIGHMEM);
		if (!cb->compressed_pages[pg_index]) {
			faili = pg_index - 1;
			ret = -ENOMEM;
			goto fail2;
		}
	}
	faili = nr_pages - 1;
	cb->nr_pages = nr_pages;

	/* In the parent-locked case, we only locked the range we are
	 * interested in.  In all other cases, we can opportunistically
	 * cache decompressed data that goes beyond the requested range. */
	if (!(bio_flags & EXTENT_BIO_PARENT_LOCKED))
		add_ra_bio_pages(inode, em_start + em_len, cb);

	/* include any pages we added in add_ra-bio_pages */
	uncompressed_len = bio->bi_vcnt * PAGE_CACHE_SIZE;
	cb->len = uncompressed_len;

	comp_bio = compressed_bio_alloc(bdev, cur_disk_byte, GFP_NOFS);
	if (!comp_bio)
		goto fail2;
	comp_bio->bi_private = cb;
	comp_bio->bi_end_io = end_compressed_bio_read;
	atomic_inc(&cb->pending_bios);

	for (pg_index = 0; pg_index < nr_pages; pg_index++) {
		page = cb->compressed_pages[pg_index];
		page->mapping = inode->i_mapping;
		page->index = em_start >> PAGE_CACHE_SHIFT;

		if (comp_bio->bi_iter.bi_size)
			ret = tree->ops->merge_bio_hook(READ, page, 0,
							PAGE_CACHE_SIZE,
							comp_bio, 0);
		else
			ret = 0;

		page->mapping = NULL;
		if (ret || bio_add_page(comp_bio, page, PAGE_CACHE_SIZE, 0) <
		    PAGE_CACHE_SIZE) {
			bio_get(comp_bio);

			ret = btrfs_bio_wq_end_io(root->fs_info, comp_bio,
					BTRFS_WQ_ENDIO_DATA);
			BUG_ON(ret); /* -ENOMEM */

			/*
			 * inc the count before we submit the bio so
			 * we know the end IO handler won't happen before
			 * we inc the count.  Otherwise, the cb might get
			 * freed before we're done setting it up
			 */
			atomic_inc(&cb->pending_bios);

			if (!(BTRFS_I(inode)->flags & BTRFS_INODE_NODATASUM)) {
				ret = btrfs_lookup_bio_sums(root, inode,
							comp_bio, sums);
				BUG_ON(ret); /* -ENOMEM */
			}
			sums += DIV_ROUND_UP(comp_bio->bi_iter.bi_size,
					     root->sectorsize);

			ret = btrfs_map_bio(root, READ, comp_bio,
					    mirror_num, 0);
			if (ret) {
				comp_bio->bi_error = ret;
				bio_endio(comp_bio);
			}

			bio_put(comp_bio);

			comp_bio = compressed_bio_alloc(bdev, cur_disk_byte,
							GFP_NOFS);
			BUG_ON(!comp_bio);
			comp_bio->bi_private = cb;
			comp_bio->bi_end_io = end_compressed_bio_read;

			bio_add_page(comp_bio, page, PAGE_CACHE_SIZE, 0);
		}
		cur_disk_byte += PAGE_CACHE_SIZE;
	}
	bio_get(comp_bio);

	ret = btrfs_bio_wq_end_io(root->fs_info, comp_bio,
			BTRFS_WQ_ENDIO_DATA);
	BUG_ON(ret); /* -ENOMEM */

	if (!(BTRFS_I(inode)->flags & BTRFS_INODE_NODATASUM)) {
		ret = btrfs_lookup_bio_sums(root, inode, comp_bio, sums);
		BUG_ON(ret); /* -ENOMEM */
	}

	ret = btrfs_map_bio(root, READ, comp_bio, mirror_num, 0);
	if (ret) {
		comp_bio->bi_error = ret;
		bio_endio(comp_bio);
	}

	bio_put(comp_bio);
	return 0;

fail2:
	while (faili >= 0) {
		__free_page(cb->compressed_pages[faili]);
		faili--;
	}

	kfree(cb->compressed_pages);
fail1:
	kfree(cb);
out:
	free_extent_map(em);
	return ret;
}

static struct {
	struct list_head idle_ws;
	spinlock_t ws_lock;
	int num_ws;
	atomic_t alloc_ws;
	wait_queue_head_t ws_wait;
} btrfs_comp_ws[BTRFS_COMPRESS_TYPES];

static const struct btrfs_compress_op * const btrfs_compress_op[] = {
	&btrfs_zlib_compress,
	&btrfs_lzo_compress,
};

void __init btrfs_init_compress(void)
{
	int i;

	for (i = 0; i < BTRFS_COMPRESS_TYPES; i++) {
		INIT_LIST_HEAD(&btrfs_comp_ws[i].idle_ws);
		spin_lock_init(&btrfs_comp_ws[i].ws_lock);
		atomic_set(&btrfs_comp_ws[i].alloc_ws, 0);
		init_waitqueue_head(&btrfs_comp_ws[i].ws_wait);
	}
}

/*
 * this finds an available workspace or allocates a new one
 * ERR_PTR is returned if things go bad.
 */
static struct list_head *find_workspace(int type)
{
	struct list_head *workspace;
	int cpus = num_online_cpus();
	int idx = type - 1;

	struct list_head *idle_ws	= &btrfs_comp_ws[idx].idle_ws;
	spinlock_t *ws_lock		= &btrfs_comp_ws[idx].ws_lock;
	atomic_t *alloc_ws		= &btrfs_comp_ws[idx].alloc_ws;
	wait_queue_head_t *ws_wait	= &btrfs_comp_ws[idx].ws_wait;
	int *num_ws			= &btrfs_comp_ws[idx].num_ws;
again:
	spin_lock(ws_lock);
	if (!list_empty(idle_ws)) {
		workspace = idle_ws->next;
		list_del(workspace);
		(*num_ws)--;
		spin_unlock(ws_lock);
		return workspace;

	}
	if (atomic_read(alloc_ws) > cpus) {
		DEFINE_WAIT(wait);

		spin_unlock(ws_lock);
		prepare_to_wait(ws_wait, &wait, TASK_UNINTERRUPTIBLE);
		if (atomic_read(alloc_ws) > cpus && !*num_ws)
			schedule();
		finish_wait(ws_wait, &wait);
		goto again;
	}
	atomic_inc(alloc_ws);
	spin_unlock(ws_lock);

	workspace = btrfs_compress_op[idx]->alloc_workspace();
	if (IS_ERR(workspace)) {
		atomic_dec(alloc_ws);
		wake_up(ws_wait);
	}
	return workspace;
}

/*
 * put a workspace struct back on the list or free it if we have enough
 * idle ones sitting around
 */
static void free_workspace(int type, struct list_head *workspace)
{
	int idx = type - 1;
	struct list_head *idle_ws	= &btrfs_comp_ws[idx].idle_ws;
	spinlock_t *ws_lock		= &btrfs_comp_ws[idx].ws_lock;
	atomic_t *alloc_ws		= &btrfs_comp_ws[idx].alloc_ws;
	wait_queue_head_t *ws_wait	= &btrfs_comp_ws[idx].ws_wait;
	int *num_ws			= &btrfs_comp_ws[idx].num_ws;

	spin_lock(ws_lock);
	if (*num_ws < num_online_cpus()) {
		list_add(workspace, idle_ws);
		(*num_ws)++;
		spin_unlock(ws_lock);
		goto wake;
	}
	spin_unlock(ws_lock);

	btrfs_compress_op[idx]->free_workspace(workspace);
	atomic_dec(alloc_ws);
wake:
	/*
	 * Make sure counter is updated before we wake up waiters.
	 */
	smp_mb();
	if (waitqueue_active(ws_wait))
		wake_up(ws_wait);
}

/*
 * cleanup function for module exit
 */
static void free_workspaces(void)
{
	struct list_head *workspace;
	int i;

	for (i = 0; i < BTRFS_COMPRESS_TYPES; i++) {
		while (!list_empty(&btrfs_comp_ws[i].idle_ws)) {
			workspace = btrfs_comp_ws[i].idle_ws.next;
			list_del(workspace);
			btrfs_compress_op[i]->free_workspace(workspace);
			atomic_dec(&btrfs_comp_ws[i].alloc_ws);
		}
	}
}

/*
 * given an address space and start/len, compress the bytes.
 *
 * pages are allocated to hold the compressed result and stored
 * in 'pages'
 *
 * out_pages is used to return the number of pages allocated.  There
 * may be pages allocated even if we return an error
 *
 * total_in is used to return the number of bytes actually read.  It
 * may be smaller then len if we had to exit early because we
 * ran out of room in the pages array or because we cross the
 * max_out threshold.
 *
 * total_out is used to return the total number of compressed bytes
 *
 * max_out tells us the max number of bytes that we're allowed to
 * stuff into pages
 */
int btrfs_compress_pages(int type, struct address_space *mapping,
			 u64 start, unsigned long len,
			 struct page **pages,
			 unsigned long nr_dest_pages,
			 unsigned long *out_pages,
			 unsigned long *total_in,
			 unsigned long *total_out,
			 unsigned long max_out)
{
	struct list_head *workspace;
	int ret;

	workspace = find_workspace(type);
	if (IS_ERR(workspace))
		return PTR_ERR(workspace);

	ret = btrfs_compress_op[type-1]->compress_pages(workspace, mapping,
						      start, len, pages,
						      nr_dest_pages, out_pages,
						      total_in, total_out,
						      max_out);
	free_workspace(type, workspace);
	return ret;
}

/*
 * pages_in is an array of pages with compressed data.
 *
 * disk_start is the starting logical offset of this array in the file
 *
 * bvec is a bio_vec of pages from the file that we want to decompress into
 *
 * vcnt is the count of pages in the biovec
 *
 * srclen is the number of bytes in pages_in
 *
 * The basic idea is that we have a bio that was created by readpages.
 * The pages in the bio are for the uncompressed data, and they may not
 * be contiguous.  They all correspond to the range of bytes covered by
 * the compressed extent.
 */
static int btrfs_decompress_biovec(int type, struct page **pages_in,
				   u64 disk_start, struct bio_vec *bvec,
				   int vcnt, size_t srclen)
{
	struct list_head *workspace;
	int ret;

	workspace = find_workspace(type);
	if (IS_ERR(workspace))
		return PTR_ERR(workspace);

	ret = btrfs_compress_op[type-1]->decompress_biovec(workspace, pages_in,
							 disk_start,
							 bvec, vcnt, srclen);
	free_workspace(type, workspace);
	return ret;
}

/*
 * a less complex decompression routine.  Our compressed data fits in a
 * single page, and we want to read a single page out of it.
 * start_byte tells us the offset into the compressed data we're interested in
 */
int btrfs_decompress(int type, unsigned char *data_in, struct page *dest_page,
		     unsigned long start_byte, size_t srclen, size_t destlen)
{
	struct list_head *workspace;
	int ret;

	workspace = find_workspace(type);
	if (IS_ERR(workspace))
		return PTR_ERR(workspace);

	ret = btrfs_compress_op[type-1]->decompress(workspace, data_in,
						  dest_page, start_byte,
						  srclen, destlen);

	free_workspace(type, workspace);
	return ret;
}

void btrfs_exit_compress(void)
{
	free_workspaces();
}

/*
 * Copy uncompressed data from working buffer to pages.
 *
 * buf_start is the byte offset we're of the start of our workspace buffer.
 *
 * total_out is the last byte of the buffer
 */
int btrfs_decompress_buf2page(char *buf, unsigned long buf_start,
			      unsigned long total_out, u64 disk_start,
			      struct bio_vec *bvec, int vcnt,
			      unsigned long *pg_index,
			      unsigned long *pg_offset)
{
	unsigned long buf_offset;
	unsigned long current_buf_start;
	unsigned long start_byte;
	unsigned long working_bytes = total_out - buf_start;
	unsigned long bytes;
	char *kaddr;
	struct page *page_out = bvec[*pg_index].bv_page;

	/*
	 * start byte is the first byte of the page we're currently
	 * copying into relative to the start of the compressed data.
	 */
	start_byte = page_offset(page_out) - disk_start;

	/* we haven't yet hit data corresponding to this page */
	if (total_out <= start_byte)
		return 1;

	/*
	 * the start of the data we care about is offset into
	 * the middle of our working buffer
	 */
	if (total_out > start_byte && buf_start < start_byte) {
		buf_offset = start_byte - buf_start;
		working_bytes -= buf_offset;
	} else {
		buf_offset = 0;
	}
	current_buf_start = buf_start;

	/* copy bytes from the working buffer into the pages */
	while (working_bytes > 0) {
		bytes = min(PAGE_CACHE_SIZE - *pg_offset,
			    PAGE_CACHE_SIZE - buf_offset);
		bytes = min(bytes, working_bytes);
		kaddr = kmap_atomic(page_out);
		memcpy(kaddr + *pg_offset, buf + buf_offset, bytes);
		kunmap_atomic(kaddr);
		flush_dcache_page(page_out);

		*pg_offset += bytes;
		buf_offset += bytes;
		working_bytes -= bytes;
		current_buf_start += bytes;

		/* check if we need to pick another page */
		if (*pg_offset == PAGE_CACHE_SIZE) {
			(*pg_index)++;
			if (*pg_index >= vcnt)
				return 0;

			page_out = bvec[*pg_index].bv_page;
			*pg_offset = 0;
			start_byte = page_offset(page_out) - disk_start;

			/*
			 * make sure our new page is covered by this
			 * working buffer
			 */
			if (total_out <= start_byte)
				return 1;

			/*
			 * the next page in the biovec might not be adjacent
			 * to the last page, but it might still be found
			 * inside this working buffer. bump our offset pointer
			 */
			if (total_out > start_byte &&
			    current_buf_start < start_byte) {
				buf_offset = start_byte - buf_start;
				working_bytes = total_out - start_byte;
				current_buf_start = buf_start + buf_offset;
			}
		}
	}

	return 1;
}

/*
 * When uncompressing data, we need to make sure and zero any parts of
 * the biovec that were not filled in by the decompression code.  pg_index
 * and pg_offset indicate the last page and the last offset of that page
 * that have been filled in.  This will zero everything remaining in the
 * biovec.
 */
void btrfs_clear_biovec_end(struct bio_vec *bvec, int vcnt,
				   unsigned long pg_index,
				   unsigned long pg_offset)
{
	while (pg_index < vcnt) {
		struct page *page = bvec[pg_index].bv_page;
		unsigned long off = bvec[pg_index].bv_offset;
		unsigned long len = bvec[pg_index].bv_len;

		if (pg_offset < off)
			pg_offset = off;
		if (pg_offset < off + len) {
			unsigned long bytes = off + len - pg_offset;
			char *kaddr;

			kaddr = kmap_atomic(page);
			memset(kaddr + pg_offset, 0, bytes);
			kunmap_atomic(kaddr);
		}
		pg_index++;
		pg_offset = 0;
	}
}
                                                                               /*
 * Copyright (C) 2008 Oracle.  All rights reserved.
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

#ifndef __BTRFS_COMPRESSION_
#define __BTRFS_COMPRESSION_

void btrfs_init_compress(void);
void btrfs_exit_compress(void);

int btrfs_compress_pages(int type, struct address_space *mapping,
			 u64 start, unsigned long len,
			 struct page **pages,
			 unsigned long nr_dest_pages,
			 unsigned long *out_pages,
			 unsigned long *total_in,
			 unsigned long *total_out,
			 unsigned long max_out);
int btrfs_decompress(int type, unsigned char *data_in, struct page *dest_page,
		     unsigned long start_byte, size_t srclen, size_t destlen);
int btrfs_decompress_buf2page(char *buf, unsigned long buf_start,
			      unsigned long total_out, u64 disk_start,
			      struct bio_vec *bvec, int vcnt,
			      unsigned long *pg_index,
			      unsigned long *pg_offset);

int btrfs_submit_compressed_write(struct inode *inode, u64 start,
				  unsigned long len, u64 disk_start,
				  unsigned long compressed_len,
				  struct page **compressed_pages,
				  unsigned long nr_pages);
int btrfs_submit_compressed_read(struct inode *inode, struct bio *bio,
				 int mirror_num, unsigned long bio_flags);
void btrfs_clear_biovec_end(struct bio_vec *bvec, int vcnt,
				   unsigned long pg_index,
				   unsigned long pg_offset);
struct btrfs_compress_op {
	struct list_head *(*alloc_workspace)(void);

	void (*free_workspace)(struct list_head *workspace);

	int (*compress_pages)(struct list_head *workspace,
			      struct address_space *mapping,
			      u64 start, unsigned long len,
			      struct page **pages,
			      unsigned long nr_dest_pages,
			      unsigned long *out_pages,
			      unsigned long *total_in,
			      unsigned long *total_out,
			      unsigned long max_out);

	int (*decompress_biovec)(struct list_head *workspace,
				 struct page **pages_in,
				 u64 disk_start,
				 struct bio_vec *bvec,
				 int vcnt,
				 size_t srclen);

	int (*decompress)(struct list_head *workspace,
			  unsigned char *data_in,
			  struct page *dest_page,
			  unsigned long start_byte,
			  size_t srclen, size_t destlen);
};

extern const struct btrfs_compress_op btrfs_zlib_compress;
extern const struct btrfs_compress_op btrfs_lzo_compress;

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Copyright (C) 2011 Fujitsu.  All rights reserved.
 * Written by Miao Xie <miaox@cn.fujitsu.com>
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
#include "delayed-inode.h"
#include "disk-io.h"
#include "transaction.h"
#include "ctree.h"

#define BTRFS_DELAYED_WRITEBACK		512
#define BTRFS_DELAYED_BACKGROUND	128
#define BTRFS_DELAYED_BATCH		16

static struct kmem_cache *delayed_node_cache;

int __init btrfs_delayed_inode_init(void)
{
	delayed_node_cache = kmem_cache_create("btrfs_delayed_node",
					sizeof(struct btrfs_delayed_node),
					0,
					SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD,
					NULL);
	if (!delayed_node_cache)
		return -ENOMEM;
	return 0;
}

void btrfs_delayed_inode_exit(void)
{
	if (delayed_node_cache)
		kmem_cache_destroy(delayed_node_cache);
}

static inline void btrfs_init_delayed_node(
				struct btrfs_delayed_node *delayed_node,
				struct btrfs_root *root, u64 inode_id)
{
	delayed_node->root = root;
	delayed_node->inode_id = inode_id;
	atomic_set(&delayed_node->refs, 0);
	delayed_node->count = 0;
	delayed_node->flags = 0;
	delayed_node->ins_root = RB_ROOT;
	delayed_node->del_root = RB_ROOT;
	mutex_init(&delayed_node->mutex);
	delayed_node->index_cnt = 0;
	INIT_LIST_HEAD(&delayed_node->n_list);
	INIT_LIST_HEAD(&delayed_node->p_list);
	delayed_node->bytes_reserved = 0;
	memset(&delayed_node->inode_item, 0, sizeof(delayed_node->inode_item));
}

static inline int btrfs_is_continuous_delayed_item(
					struct btrfs_delayed_item *item1,
					struct btrfs_delayed_item *item2)
{
	if (item1->key.type == BTRFS_DIR_INDEX_KEY &&
	    item1->key.objectid == item2->key.objectid &&
	    item1->key.type == item2->key.type &&
	    item1->key.offset + 1 == item2->key.offset)
		return 1;
	return 0;
}

static inline struct btrfs_delayed_root *btrfs_get_delayed_root(
							struct btrfs_root *root)
{
	return root->fs_info->delayed_root;
}

static struct btrfs_delayed_node *btrfs_get_delayed_node(struct inode *inode)
{
	struct btrfs_inode *btrfs_inode = BTRFS_I(inode);
	struct btrfs_root *root = btrfs_inode->root;
	u64 ino = btrfs_ino(inode);
	struct btrfs_delayed_node *node;

	node = ACCESS_ONCE(btrfs_inode->delayed_node);
	if (node) {
		atomic_inc(&node->refs);
		return node;
	}

	spin_lock(&root->inode_lock);
	node = radix_tree_lookup(&root->delayed_nodes_tree, ino);
	if (node) {
		if (btrfs_inode->delayed_node) {
			atomic_inc(&node->refs);	/* can be accessed */
			BUG_ON(btrfs_inode->delayed_node != node);
			spin_unlock(&root->inode_lock);
			return node;
		}
		btrfs_inode->delayed_node = node;
		/* can be accessed and cached in the inode */
		atomic_add(2, &node->refs);
		spin_unlock(&root->inode_lock);
		return node;
	}
	spin_unlock(&root->inode_lock);

	return NULL;
}

/* Will return either the node or PTR_ERR(-ENOMEM) */
static struct btrfs_delayed_node *btrfs_get_or_create_delayed_node(
							struct inode *inode)
{
	struct btrfs_delayed_node *node;
	struct btrfs_inode *btrfs_inode = BTRFS_I(inode);
	struct btrfs_root *root = btrfs_inode->root;
	u64 ino = btrfs_ino(inode);
	int ret;

again:
	node = btrfs_get_delayed_node(inode);
	if (node)
		return node;

	node = kmem_cache_alloc(delayed_node_cache, GFP_NOFS);
	if (!node)
		return ERR_PTR(-ENOMEM);
	btrfs_init_delayed_node(node, root, ino);

	/* cached in the btrfs inode and can be accessed */
	atomic_add(2, &node->refs);

	ret = radix_tree_preload(GFP_NOFS & ~__GFP_HIGHMEM);
	if (ret) {
		kmem_cache_free(delayed_node_cache, node);
		return ERR_PTR(ret);
	}

	spin_lock(&root->inode_lock);
	ret = radix_tree_insert(&root->delayed_nodes_tree, ino, node);
	if (ret == -EEXIST) {
		spin_unlock(&root->inode_lock);
		kmem_cache_free(delayed_node_cache, node);
		radix_tree_preload_end();
		goto again;
	}
	btrfs_inode->delayed_node = node;
	spin_unlock(&root->inode_lock);
	radix_tree_preload_end();

	return node;
}

/*
 * Call it when holding delayed_node->mutex
 *
 * If mod = 1, add this node into the prepared list.
 */
static void btrfs_queue_delayed_node(struct btrfs_delayed_root *root,
				     struct btrfs_delayed_node *node,
				     int mod)
{
	spin_lock(&root->lock);
	if (test_bit(BTRFS_DELAYED_NODE_IN_LIST, &node->flags)) {
		if (!list_empty(&node->p_list))
			list_move_tail(&node->p_list, &root->prepare_list);
		else if (mod)
			list_add_tail(&node->p_list, &root->prepare_list);
	} else {
		list_add_tail(&node->n_list, &root->node_list);
		list_add_tail(&node->p_list, &root->prepare_list);
		atomic_inc(&node->refs);	/* inserted into list */
		root->nodes++;
		set_bit(BTRFS_DELAYED_NODE_IN_LIST, &node->flags);
	}
	spin_unlock(&root->lock);
}

/* Call it when holding delayed_node->mutex */
static void btrfs_dequeue_delayed_node(struct btrfs_delayed_root *root,
				       struct btrfs_delayed_node *node)
{
	spin_lock(&root->lock);
	if (test_bit(BTRFS_DELAYED_NODE_IN_LIST, &node->flags)) {
		root->nodes--;
		atomic_dec(&node->refs);	/* not in the list */
		list_del_init(&node->n_list);
		if (!list_empty(&node->p_list))
			list_del_init(&node->p_list);
		clear_bit(BTRFS_DELAYED_NODE_IN_LIST, &node->flags);
	}
	spin_unlock(&root->lock);
}

static struct btrfs_delayed_node *btrfs_first_delayed_node(
			struct btrfs_delayed_root *delayed_root)
{
	struct list_head *p;
	struct btrfs_delayed_node *node = NULL;

	spin_lock(&delayed_root->lock);
	if (list_empty(&delayed_root->node_list))
		goto out;

	p = delayed_root->node_list.next;
	node = list_entry(p, struct btrfs_delayed_node, n_list);
	atomic_inc(&node->refs);
out:
	spin_unlock(&delayed_root->lock);

	return node;
}

static struct btrfs_delayed_node *btrfs_next_delayed_node(
						struct btrfs_delayed_node *node)
{
	struct btrfs_delayed_root *delayed_root;
	struct list_head *p;
	struct btrfs_delayed_node *next = NULL;

	delayed_root = node->root->fs_info->delayed_root;
	spin_lock(&delayed_root->lock);
	if (!test_bit(BTRFS_DELAYED_NODE_IN_LIST, &node->flags)) {
		/* not in the list */
		if (list_empty(&delayed_root->node_list))
			goto out;
		p = delayed_root->node_list.next;
	} else if (list_is_last(&node->n_list, &delayed_root->node_list))
		goto out;
	else
		p = node->n_list.next;

	next = list_entry(p, struct btrfs_delayed_node, n_list);
	atomic_inc(&next->refs);
out:
	spin_unlock(&delayed_root->lock);

	return next;
}

static void __btrfs_release_delayed_node(
				struct btrfs_delayed_node *delayed_node,
				int mod)
{
	struct btrfs_delayed_root *delayed_root;

	if (!delayed_node)
		return;

	delayed_root = delayed_node->root->fs_info->delayed_root;

	mutex_lock(&delayed_node->mutex);
	if (delayed_node->count)
		btrfs_queue_delayed_node(delayed_root, delayed_node, mod);
	else
		btrfs_dequeue_delayed_node(delayed_root, delayed_node);
	mutex_unlock(&delayed_node->mutex);

	if (atomic_dec_and_test(&delayed_node->refs)) {
		bool free = false;
		struct btrfs_root *root = delayed_node->root;
		spin_lock(&root->inode_lock);
		if (atomic_read(&delayed_node->refs) == 0) {
			radix_tree_delete(&root->delayed_nodes_tree,
					  delayed_node->inode_id);
			free = true;
		}
		spin_unlock(&root->inode_lock);
		if (free)
			kmem_cache_free(delayed_node_cache, delayed_node);
	}
}

static inline void btrfs_release_delayed_node(struct btrfs_delayed_node *node)
{
	__btrfs_release_delayed_node(node, 0);
}

static struct btrfs_delayed_node *btrfs_first_prepared_delayed_node(
					struct btrfs_delayed_root *delayed_root)
{
	struct list_head *p;
	struct btrfs_delayed_node *node = NULL;

	spin_lock(&delayed_root->lock);
	if (list_empty(&delayed_root->prepare_list))
		goto out;

	p = delayed_root->prepare_list.next;
	list_del_init(p);
	node = list_entry(p, struct btrfs_delayed_node, p_list);
	atomic_inc(&node->refs);
out:
	spin_unlock(&delayed_root->lock);

	return node;
}

static inline void btrfs_release_prepared_delayed_node(
					struct btrfs_delayed_node *node)
{
	__btrfs_release_delayed_node(node, 1);
}

static struct btrfs_delayed_item *btrfs_alloc_delayed_item(u32 data_len)
{
	struct btrfs_delayed_item *item;
	item = kmalloc(sizeof(*item) + data_len, GFP_NOFS);
	if (item) {
		item->data_len = data_len;
		item->ins_or_del = 0;
		item->bytes_reserved = 0;
		item->delayed_node = NULL;
		atomic_set(&item->refs, 1);
	}
	return item;
}

/*
 * __btrfs_lookup_delayed_item - look up the delayed item by key
 * @delayed_node: pointer to the delayed node
 * @key:	  the key to look up
 * @prev:	  used to store the prev item if the right item isn't found
 * @next:	  used to store the next item if the right item isn't found
 *
 * Note: if we don't find the right item, we will return the prev item and
 * the next item.
 */
static struct btrfs_delayed_item *__btrfs_lookup_delayed_item(
				struct rb_root *root,
				struct btrfs_key *key,
				struct btrfs_delayed_item **prev,
				struct btrfs_delayed_item **next)
{
	struct rb_node *node, *prev_node = NULL;
	struct btrfs_delayed_item *delayed_item = NULL;
	int ret = 0;

	node = root->rb_node;

	while (node) {
		delayed_item = rb_entry(node, struct btrfs_delayed_item,
					rb_node);
		prev_node = node;
		ret = btrfs_comp_cpu_keys(&delayed_item->key, key);
		if (ret < 0)
			node = node->rb_right;
		else if (ret > 0)
			node = node->rb_left;
		else
			return delayed_item;
	}

	if (prev) {
		if (!prev_node)
			*prev = NULL;
		else if (ret < 0)
			*prev = delayed_item;
		else if ((node = rb_prev(prev_node)) != NULL) {
			*prev = rb_entry(node, struct btrfs_delayed_item,
					 rb_node);
		} else
			*prev = NULL;
	}

	if (next) {
		if (!prev_node)
			*next = NULL;
		else if (ret > 0)
			*next = delayed_item;
		else if ((node = rb_next(prev_node)) != NULL) {
			*next = rb_entry(node, struct btrfs_delayed_item,
					 rb_node);
		} else
			*next = NULL;
	}
	return NULL;
}

static struct btrfs_delayed_item *__btrfs_lookup_delayed_insertion_item(
					struct btrfs_delayed_node *delayed_node,
					struct btrfs_key *key)
{
	struct btrfs_delayed_item *item;

	item = __btrfs_lookup_delayed_item(&delayed_node->ins_root, key,
					   NULL, NULL);
	return item;
}

static int __btrfs_add_delayed_item(struct btrfs_delayed_node *delayed_node,
				    struct btrfs_delayed_item *ins,
				    int action)
{
	struct rb_node **p, *node;
	struct rb_node *parent_node = NULL;
	struct rb_root *root;
	struct btrfs_delayed_item *item;
	int cmp;

	if (action == BTRFS_DELAYED_INSERTION_ITEM)
		root = &delayed_node->ins_root;
	else if (action == BTRFS_DELAYED_DELETION_ITEM)
		root = &delayed_node->del_root;
	else
		BUG();
	p = &root->rb_node;
	node = &ins->rb_node;

	while (*p) {
		parent_node = *p;
		item = rb_entry(parent_node, struct btrfs_delayed_item,
				 rb_node);

		cmp = btrfs_comp_cpu_keys(&item->key, &ins->key);
		if (cmp < 0)
			p = &(*p)->rb_right;
		else if (cmp > 0)
			p = &(*p)->rb_left;
		else
			return -EEXIST;
	}

	rb_link_node(node, parent_node, p);
	rb_insert_color(node, root);
	ins->delayed_node = delayed_node;
	ins->ins_or_del = action;

	if (ins->key.type == BTRFS_DIR_INDEX_KEY &&
	    action == BTRFS_DELAYED_INSERTION_ITEM &&
	    ins->key.offset >= delayed_node->index_cnt)
			delayed_node->index_cnt = ins->key.offset + 1;

	delayed_node->count++;
	atomic_inc(&delayed_node->root->fs_info->delayed_root->items);
	return 0;
}

static int __btrfs_add_delayed_insertion_item(struct btrfs_delayed_node *node,
					      struct btrfs_delayed_item *item)
{
	return __btrfs_add_delayed_item(node, item,
					BTRFS_DELAYED_INSERTION_ITEM);
}

static int __btrfs_add_delayed_deletion_item(struct btrfs_delayed_node *node,
					     struct btrfs_delayed_item *item)
{
	return __btrfs_add_delayed_item(node, item,
					BTRFS_DELAYED_DELETION_ITEM);
}

static void finish_one_item(struct btrfs_delayed_root *delayed_root)
{
	int seq = atomic_inc_return(&delayed_root->items_seq);

	/*
	 * atomic_dec_return implies a barrier for waitqueue_active
	 */
	if ((atomic_dec_return(&delayed_root->items) <
	    BTRFS_DELAYED_BACKGROUND || seq % BTRFS_DELAYED_BATCH == 0) &&
	    waitqueue_active(&delayed_root->wait))
		wake_up(&delayed_root->wait);
}

static void __btrfs_remove_delayed_item(struct btrfs_delayed_item *delayed_item)
{
	struct rb_root *root;
	struct btrfs_delayed_root *delayed_root;

	delayed_root = delayed_item->delayed_node->root->fs_info->delayed_root;

	BUG_ON(!delayed_root);
	BUG_ON(delayed_item->ins_or_del != BTRFS_DELAYED_DELETION_ITEM &&
	       delayed_item->ins_or_del != BTRFS_DELAYED_INSERTION_ITEM);

	if (delayed_item->ins_or_del == BTRFS_DELAYED_INSERTION_ITEM)
		root = &delayed_item->delayed_node->ins_root;
	else
		root = &delayed_item->delayed_node->del_root;

	rb_erase(&delayed_item->rb_node, root);
	delayed_item->delayed_node->count--;

	finish_one_item(delayed_root);
}

static void btrfs_release_delayed_item(struct btrfs_delayed_item *item)
{
	if (item) {
		__btrfs_remove_delayed_item(item);
		if (atomic_dec_and_test(&item->refs))
			kfree(item);
	}
}

static struct btrfs_delayed_item *__btrfs_first_delayed_insertion_item(
					struct btrfs_delayed_node *delayed_node)
{
	struct rb_node *p;
	struct btrfs_delayed_item *item = NULL;

	p = rb_first(&delayed_node->ins_root);
	if (p)
		item = rb_entry(p, struct btrfs_delayed_item, rb_node);

	return item;
}

static struct btrfs_delayed_item *__btrfs_first_delayed_deletion_item(
					struct btrfs_delayed_node *delayed_node)
{
	struct rb_node *p;
	struct btrfs_delayed_item *item = NULL;

	p = rb_first(&delayed_node->del_root);
	if (p)
		item = rb_entry(p, struct btrfs_delayed_item, rb_node);

	return item;
}

static struct btrfs_delayed_item *__btrfs_next_delayed_item(
						struct btrfs_delayed_item *item)
{
	struct rb_node *p;
	struct btrfs_delayed_item *next = NULL;

	p = rb_next(&item->rb_node);
	if (p)
		next = rb_entry(p, struct btrfs_delayed_item, rb_node);

	return next;
}

static int btrfs_delayed_item_reserve_metadata(struct btrfs_trans_handle *trans,
					       struct btrfs_root *root,
					       struct btrfs_delayed_item *item)
{
	struct btrfs_block_rsv *src_rsv;
	struct btrfs_block_rsv *dst_rsv;
	u64 num_bytes;
	int ret;

	if (!trans->bytes_reserved)
		return 0;

	src_rsv = trans->block_rsv;
	dst_rsv = &root->fs_info->delayed_block_rsv;

	num_bytes = btrfs_calc_trans_metadata_size(root, 1);
	ret = btrfs_block_rsv_migrate(src_rsv, dst_rsv, num_bytes);
	if (!ret) {
		trace_btrfs_space_reservation(root->fs_info, "delayed_item",
					      item->key.objectid,
					      num_bytes, 1);
		item->bytes_reserved = num_bytes;
	}

	return ret;
}

static void btrfs_delayed_item_release_metadata(struct btrfs_root *root,
						struct btrfs_delayed_item *item)
{
	struct btrfs_block_rsv *rsv;

	if (!item->bytes_reserved)
		return;

	rsv = &root->fs_info->delayed_block_rsv;
	trace_btrfs_space_reservation(root->fs_info, "delayed_item",
				      item->key.objectid, item->bytes_reserved,
				      0);
	btrfs_block_rsv_release(root, rsv,
				item->bytes_reserved);
}

static int btrfs_delayed_inode_reserve_metadata(
					struct btrfs_trans_handle *trans,
					struct btrfs_root *root,
					struct inode *inode,
					struct btrfs_delayed_node *node)
{
	struct btrfs_block_rsv *src_rsv;
	struct btrfs_block_rsv *dst_rsv;
	u64 num_bytes;
	int ret;
	bool release = false;

	src_rsv = trans->block_rsv;
	dst_rsv = &root->fs_info->delayed_block_rsv;

	num_bytes = btrfs_calc_trans_metadata_size(root, 1)