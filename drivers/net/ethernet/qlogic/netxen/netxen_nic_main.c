/*
 * Network block device - make block devices work over TCP
 *
 * Note that you can not swap over this thing, yet. Seems to work but
 * deadlocks sometimes - you can not swap over TCP in general.
 * 
 * Copyright 1997-2000, 2008 Pavel Machek <pavel@ucw.cz>
 * Parts copyright 2001 Steven Whitehouse <steve@chygwyn.com>
 *
 * This file is released under GPLv2 or later.
 *
 * (part of code stolen from loop.c)
 */

#include <linux/major.h>

#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <linux/net.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/debugfs.h>

#include <asm/uaccess.h>
#include <asm/types.h>

#include <linux/nbd.h>

struct nbd_device {
	u32 flags;
	struct socket * sock;	/* If == NULL, device is not ready, yet	*/
	int magic;

	spinlock_t queue_lock;
	struct list_head queue_head;	/* Requests waiting result */
	struct request *active_req;
	wait_queue_head_t active_wq;
	struct list_head waiting_queue;	/* Requests to be sent */
	wait_queue_head_t waiting_wq;

	struct mutex tx_lock;
	struct gendisk *disk;
	int blksize;
	loff_t bytesize;
	int xmit_timeout;
	bool disconnect; /* a disconnect has been requested by user */

	struct timer_list timeout_timer;
	spinlock_t tasks_lock;
	struct task_struct *task_recv;
	struct task_struct *task_send;

#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbg_dir;
#endif
};

#if IS_ENABLED(CONFIG_DEBUG_FS)
static struct dentry *nbd_dbg_dir;
#endif

#define nbd_name(nbd) ((nbd)->disk->disk_name)

#define NBD_MAGIC 0x68797548

static unsigned int nbds_max = 16;
static struct nbd_device *nbd_dev;
static int max_part;

/*
 * Use just one lock (or at most 1 per NIC). Two arguments for this:
 * 1. Each NIC is essentially a synchronization point for all servers
 *    accessed through that NIC so there's no need to have more locks
 *    than NICs anyway.
 * 2. More locks lead to more "Dirty cache line bouncing" which will slow
 *    down each lock to the point where they're actually slower than just
 *    a single lock.
 * Thanks go to Jens Axboe and Al Viro for their LKML emails explaining this!
 */
static DEFINE_SPINLOCK(nbd_lock);

static inline struct device *nbd_to_dev(struct nbd_device *nbd)
{
	return disk_to_dev(nbd->disk);
}

static const char *nbdcmd_to_ascii(int cmd)
{
	switch (cmd) {
	case  NBD_CMD_READ: return "read";
	case NBD_CMD_WRITE: return "write";
	case  NBD_CMD_DISC: return "disconnect";
	case NBD_CMD_FLUSH: return "flush";
	case  NBD_CMD_TRIM: return "trim/discard";
	}
	return "invalid";
}

static void nbd_end_request(struct nbd_device *nbd, struct request *req)
{
	int error = req->errors ? -EIO : 0;
	struct request_queue *q = req->q;
	unsigned long flags;

	dev_dbg(nbd_to_dev(nbd), "request %p: %s\n", req,
		error ? "failed" : "done");

	spin_lock_irqsave(q->queue_lock, flags);
	__blk_end_request_all(req, error);
	spin_unlock_irqrestore(q->queue_lock, flags);
}

/*
 * Forcibly shutdown the socket causing all listeners to error
 */
static void sock_shutdown(struct nbd_device *nbd)
{
	if (!nbd->sock)
		return;

	dev_warn(disk_to_dev(nbd->disk), "shutting down socket\n");
	kernel_sock_shutdown(nbd->sock, SHUT_RDWR);
	nbd->sock = NULL;
	del_timer_sync(&nbd->timeout_timer);
}

static void nbd_xmit_timeout(unsigned long arg)
{
	struct nbd_device *nbd = (struct nbd_device *)arg;
	unsigned long flags;

	if (list_empty(&nbd->queue_head))
		return;

	nbd->disconnect = true;

	spin_lock_irqsave(&nbd->tasks_lock, flags);

	if (nbd->task_recv)
		force_sig(SIGKILL, nbd->task_recv);

	if (nbd->task_send)
		force_sig(SIGKILL, nbd->task_send);

	spin_unlock_irqrestore(&nbd->tasks_lock, flags);

	dev_err(nbd_to_dev(nbd), "Connection timed out, killed receiver and sender, shutting down connection\n");
}

/*
 *  Send or receive packet.
 */
static int sock_xmit(struct nbd_device *nbd, int send, void *buf, int size,
		int msg_flags)
{
	struct socket *sock = nbd->sock;
	int result;
	struct msghdr msg;
	struct kvec iov;
	sigset_t blocked, oldset;
	unsigned long pflags = current->flags;

	if (unlikely(!sock)) {
		dev_err(disk_to_dev(nbd->disk),
			"Attempted %s on closed socket in sock_xmit\n",
			(send ? "send" : "recv"));
		return -EINVAL;
	}

	/* Allow interception of SIGKILL only
	 * Don't allow other signals to interrupt the transmission */
	siginitsetinv(&blocked, sigmask(SIGKILL));
	sigprocmask(SIG_SETMASK, &blocked, &oldset);

	current->flags |= PF_MEMALLOC;
	do {
		sock->sk->sk_allocation = GFP_NOIO | __GFP_MEMALLOC;
		iov.iov_base = buf;
		iov.iov_len = size;
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = msg_flags | MSG_NOSIGNAL;

		if (send)
			result = kernel_sendmsg(sock, &msg, &iov, 1, size);
		else
			result = kernel_recvmsg(sock, &msg, &iov, 1, size,
						msg.msg_flags);

		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}
		size -= result;
		buf += result;
	} while (size > 0);

	sigprocmask(SIG_SETMASK, &oldset, NULL);
	tsk_restore_flags(current, pflags, PF_MEMALLOC);

	if (!send && nbd->xmit_timeout)
		mod_timer(&nbd->timeout_timer, jiffies + nbd->xmit_timeout);

	return result;
}

static inline int sock_send_bvec(struct nbd_device *nbd, struct bio_vec *bvec,
		int flags)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	result = sock_xmit(nbd, 1, kaddr + bvec->bv_offset,
			   bvec->bv_len, flags);
	kunmap(bvec->bv_page);
	return result;
}

/* always call with the tx_lock held */
static int nbd_send_req(struct nbd_device *nbd, struct request *req)
{
	int result, flags;
	struct nbd_request request;
	unsigned long size = blk_rq_bytes(req);
	u32 type;

	if (req->cmd_type == REQ_TYPE_DRV_PRIV)
		type = NBD_CMD_DISC;
	else if (req->cmd_flags & REQ_DISCARD)
		type = NBD_CMD_TRIM;
	else if (req->cmd_flags & REQ_FLUSH)
		type = NBD_CMD_FLUSH;
	else if (rq_data_dir(req) == WRITE)
		type = NBD_CMD_WRITE;
	else
		type = NBD_CMD_READ;

	memset(&request, 0, sizeof(request));
	request.magic = htonl(NBD_REQUEST_MAGIC);
	request.type = htonl(type);
	if (type != NBD_CMD_FLUSH && type != NBD_CMD_DISC) {
		request.from = cpu_to_be64((u64)blk_rq_pos(req) << 9);
		request.len = htonl(size);
	}
	memcpy(request.handle, &req, sizeof(req));

	dev_dbg(nbd_to_dev(nbd), "request %p: sending control (%s@%llu,%uB)\n",
		req, nbdcmd_to_ascii(type),
		(unsigned long long)blk_rq_pos(req) << 9, blk_rq_bytes(req));
	result = sock_xmit(nbd, 1, &request, sizeof(request),
			(type == NBD_CMD_WRITE) ? MSG_MORE : 0);
	if (result <= 0) {
		dev_err(disk_to_dev(nbd->disk),
			"Send control failed (result %d)\n", result);
		return -EIO;
	}

	if (type == NBD_CMD_WRITE) {
		struct req_iterator iter;
		struct bio_vec bvec;
		/*
		 * we are really probing at internals to determine
		 * whether to set MSG_MORE or not...
		 */
		rq_for_each_segment(bvec, req, iter) {
			flags = 0;
			if (!rq_iter_last(bvec, iter))
				flags = MSG_MORE;
			dev_dbg(nbd_to_dev(nbd), "request %p: sending %d bytes data\n",
				req, bvec.bv_len);
			result = sock_send_bvec(nbd, &bvec, flags);
			if (result <= 0) {
				dev_err(disk_to_dev(nbd->disk),
					"Send data failed (result %d)\n",
					result);
				return -EIO;
			}
		}
	}
	return 0;
}

static struct request *nbd_find_request(struct nbd_device *nbd,
					struct request *xreq)
{
	struct request *req, *tmp;
	int err;

	err = wait_event_interruptible(nbd->active_wq, nbd->active_req != xreq);
	if (unlikely(err))
		return ERR_PTR(err);

	spin_lock(&nbd->queue_lock);
	list_for_each_entry_safe(req, tmp, &nbd->queue_head, queuelist) {
		if (req != xreq)
			continue;
		list_del_init(&req->queuelist);
		spin_unlock(&nbd->queue_lock);
		return req;
	}
	spin_unlock(&nbd->queue_lock);

	return ERR_PTR(-ENOENT);
}

static inline int sock_recv_bvec(struct nbd_device *nbd, struct bio_vec *bvec)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	result = sock_xmit(nbd, 0, kaddr + bvec->bv_offset, bvec->bv_len,
			MSG_WAITALL);
	kunmap(bvec->bv_page);
	return result;
}

/* NULL returned = something went wrong, inform userspace */
static struct request *nbd_read_stat(struct nbd_device *nbd)
{
	int result;
	struct nbd_reply reply;
	struct request *req;

	reply.magic = 0;
	result = sock_xmit(nbd, 0, &reply, sizeof(reply), MSG_WAITALL);
	if (result <= 0) {
		dev_err(disk_to_dev(nbd->disk),
			"Receive control failed (result %d)\n", result);
		return ERR_PTR(result);
	}

	if (ntohl(reply.magic) != NBD_REPLY_MAGIC) {
		dev_err(disk_to_dev(nbd->disk), "Wrong magic (0x%lx)\n",
				(unsigned long)ntohl(reply.magic));
		return ERR_PTR(-EPROTO);
	}

	req = nbd_find_request(nbd, *(struct request **)reply.handle);
	if (IS_ERR(req)) {
		result = PTR_ERR(req);
		if (result != -ENOENT)
			return ERR_PTR(result);

		dev_err(disk_to_dev(nbd->disk), "Unexpected reply (%p)\n",
			reply.handle);
		return ERR_PTR(-EBADR);
	}

	if (ntohl(reply.error)) {
		dev_err(disk_to_dev(nbd->disk), "Other side returned error (%d)\n",
			ntohl(reply.error));
		req->errors++;
		return req;
	}

	dev_dbg(nbd_to_dev(nbd), "request %p: got reply\n", req);
	if (rq_data_dir(req) != WRITE) {
		struct req_iterator iter;
		struct bio_vec bvec;

		rq_for_each_segment(bvec, req, iter) {
			result = sock_recv_bvec(nbd, &bvec);
			if (result <= 0) {
				dev_err(disk_to_dev(nbd->disk), "Receive data failed (result %d)\n",
					result);
				req->errors++;
				return req;
			}
			dev_dbg(nbd_to_dev(nbd), "request %p: got %d bytes data\n",
				req, bvec.bv_len);
		}
	}
	return req;
}

static ssize_t pid_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct gendisk *disk = dev_to_disk(dev);
	struct nbd_device *nbd = (struct nbd_device *)disk->private_data;

	return sprintf(buf, "%d\n", task_pid_nr(nbd->task_recv));
}

static struct device_attribute pid_attr = {
	.attr = { .name = "pid", .mode = S_IRUGO},
	.show = pid_show,
};

static int nbd_thread_recv(struct nbd_device *nbd)
{
	struct request *req;
	int ret;
	unsigned long flags;

	BUG_ON(nbd->magic != NBD_MAGIC);

	sk_set_memalloc(nbd->sock->sk);

	spin_lock_irqsave(&nbd->tasks_lock, flags);
	nbd->task_recv = current;
	spin_unlock_irqrestore(&nbd->tasks_lock, flags);

	ret = device_create_file(disk_to_dev(nbd->disk), &pid_attr);
	if (ret) {
		dev_err(disk_to_dev(nbd->disk), "device_create_file failed!\n");

		spin_lock_irqsave(&nbd->tasks_lock, flags);
		nbd->task_recv = NULL;
		spin_unlock_irqrestore(&nbd->tasks_lock, flags);

		return ret;
	}

	while (1) {
		req = nbd_read_stat(nbd);
		if (IS_ERR(req)) {
			ret = PTR_ERR(req);
			break;
		}

		nbd_end_request(nbd, req);
	}

	device_remove_file(disk_to_dev(nbd->disk), &pid_attr);

	spin_lock_irqsave(&nbd->tasks_lock, flags);
	nbd->task_recv = NULL;
	spin_unlock_irqrestore(&nbd->tasks_lock, flags);

	if (signal_pending(current)) {
		ret = kernel_dequeue_signal(NULL);
		dev_warn(nbd_to_dev(nbd), "pid %d, %s, got signal %d\n",
			 task_pid_nr(current), current->comm, ret);
		mutex_lock(&nbd->tx_lock);
		sock_shutdown(nbd);
		mutex_unlock(&nbd->tx_lock);
		ret = -ETIMEDOUT;
	}

	return ret;
}

static void nbd_clear_que(struct nbd_device *nbd)
{
	struct request *req;

	BUG_ON(nbd->magic != NBD_MAGIC);

	/*
	 * Because we have set nbd->sock to NULL under the tx_lock, all
	 * modifications to the list must have completed by now.  For
	 * the same reason, the active_req must be NULL.
	 *
	 * As a consequence, we don't need to take the spin lock while
	 * purging the list here.
	 */
	BUG_ON(nbd->sock);
	BUG_ON(nbd->active_req);

	while (!list_empty(&nbd->queue_head)) {
		req = list_entry(nbd->queue_head.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		req->errors++;
		nbd_end_request(nbd, req);
	}

	while (!list_empty(&nbd->waiting_queue)) {
		req = list_entry(nbd->waiting_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		req->errors++;
		nbd_end_request(nbd, req);
	}
	dev_dbg(disk_to_dev(nbd->disk), "queue cleared\n");
}


static void nbd_handle_req(struct nbd_device *nbd, struct request *req)
{
	if (req->cmd_type != REQ_TYPE_FS)
		goto error_out;

	if (rq_data_dir(req) == WRITE &&
	    (nbd->flags & NBD_FLAG_READ_ONLY)) {
		dev_err(disk_to_dev(nbd->disk),
			"Write on read-only\n");
		goto error_out;
	}

	req->errors = 0;

	mutex_lock(&nbd->tx_lock);
	if (unlikely(!nbd->sock)) {
		mutex_unlock(&nbd->tx_lock);
		dev_err(disk_to_dev(nbd->disk),
			"Attempted send on closed socket\n");
		goto error_out;
	}

	nbd->active_req = req;

	if (nbd->xmit_timeout && list_empty_careful(&nbd->queue_head))
		mod_timer(&nbd->timeout_timer, jiffies + nbd->xmit_timeout);

	if (nbd_send_req(nbd, req) != 0) {
		dev_err(disk_to_dev(nbd->disk), "Request send failed\n");
		req->errors++;
		nbd_end_request(nbd, req);
	} else {
		spin_lock(&nbd->queue_lock);
		list_add_tail(&req->queuelist, &nbd->queue_head);
		spin_unlock(&nbd->queue_lock);
	}

	nbd->active_req = NULL;
	mutex_unlock(&nbd->tx_lock);
	wake_up_all(&nbd->active_wq);

	return;

error_out:
	req->errors++;
	nbd_end_request(nbd, req);
}

static int nbd_thread_send(void *data)
{
	struct nbd_device *nbd = data;
	struct request *req;
	unsigned long flags;

	spin_lock_irqsave(&nbd->tasks_lock, flags);
	nbd->task_send = current;
	spin_unlock_irqrestore(&nbd->tasks_lock, flags);

	set_user_nice(current, MIN_NICE);
	while (!kthread_should_stop() || !list_empty(&nbd->waiting_queue)) {
		/* wait for something to do */
		wait_event_interruptible(nbd->waiting_wq,
					 kthread_should_stop() ||
					 !list_empty(&nbd->waiting_queue));

		if (signal_pending(current)) {
			int ret = kernel_dequeue_signal(NULL);

			dev_warn(nbd_to_dev(nbd), "pid %d, %s, got signal %d\n",
				 task_pid_nr(current), current->comm, ret);
			mutex_lock(&nbd->tx_lock);
			sock_shutdown(nbd);
			mutex_unlock(&nbd->tx_lock);
			break;
		}

		/* extract request */
		if (list_empty(&nbd->waiting_queue))
			continue;

		spin_lock_irq(&nbd->queue_lock);
		req = list_entry(nbd->waiting_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		spin_unlock_irq(&nbd->queue_lock);

		/* handle request */
		nbd_handle_req(nbd, req);
	}

	spin_lock_irqsave(&nbd->tasks_lock, flags);
	nbd->task_send = NULL;
	spin_unlock_irqrestore(&nbd->tasks_lock, flags);

	/* Clear maybe pending signals */
	if (signal_pending(current))
		kernel_dequeue_signal(NULL);

	return 0;
}

/*
 * We always wait for result of write, for now. It would be nice to make it optional
 * in future
 * if ((rq_data_dir(req) == WRITE) && (nbd->flags & NBD_WRITE_NOCHK))
 *   { printk( "Warning: Ignoring result!\n"); nbd_end_request( req ); }
 */

static void nbd_request_handler(struct request_queue *q)
		__releases(q->queue_lock) __acquires(q->queue_lock)
{
	struct request *req;
	
	while ((req = blk_fetch_request(q)) != NULL) {
		struct nbd_device *nbd;

		spin_unlock_irq(q->queue_lock);

		nbd = req->rq_disk->private_data;

		BUG_ON(nbd->magic != NBD_MAGIC);

		dev_dbg(nbd_to_dev(nbd), "request %p: dequeued (flags=%x)\n",
			req, req->cmd_type);

		if (unlikely(!nbd->sock)) {
			dev_err_ratelimited(disk_to_dev(nbd->disk),
					    "Attempted send on closed socket\n");
			req->errors++;
			nbd_end_request(nbd, req);
			spin_lock_irq(q->queue_lock);
			continue;
		}

		spin_lock_irq(&nbd->queue_lock);
		list_add_tail(&req->queuelist, &nbd->waiting_queue);
		spin_unlock_irq(&nbd->queue_lock);

		wake_up(&nbd->waiting_wq);

		spin_lock_irq(q->queue_lock);
	}
}

static int nbd_dev_dbg_init(struct nbd_device *nbd);
static void nbd_dev_dbg_close(struct nbd_device *nbd);

/* Must be called with tx_lock held */

static int __nbd_ioctl(struct block_device *bdev, struct nbd_device *nbd,
		       unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case NBD_DISCONNECT: {
		struct request sreq;

		dev_info(disk_to_dev(nbd->disk), "NBD_DISCONNECT\n");
		if (!nbd->sock)
			return -EINVAL;

		mutex_unlock(&nbd->tx_lock);
		fsync_bdev(bdev);
		mutex_lock(&nbd->tx_lock);
		blk_rq_init(NULL, &sreq);
		sreq.cmd_type = REQ_TYPE_DRV_PRIV;

		/* Check again after getting mutex back.  */
		if (!nbd->sock)
			return -EINVAL;

		nbd->disconnect = true;

		nbd_send_req(nbd, &sreq);
		return 0;
	}
 
	case NBD_CLEAR_SOCK: {
		struct socket *sock = nbd->sock;
		nbd->sock = NULL;
		nbd_clear_que(nbd);
		BUG_ON(!list_empty(&nbd->queue_head));
		BUG_ON(!list_empty(&nbd->waiting_queue));
		kill_bdev(bdev);
		if (sock)
			sockfd_put(sock);
		return 0;
	}

	case NBD_SET_SOCK: {
		struct socket *sock;
		int err;
		if (nbd->sock)
			return -EBUSY;
		sock = sockfd_lookup(arg, &err);
		if (sock) {
			nbd->sock = sock;
			if (max_part > 0)
				bdev->bd_invalidated = 1;
			nbd->disconnect = false; /* we're connected now */
			return 0;
		}
		return -EINVAL;
	}

	case NBD_SET_BLKSIZE:
		nbd->blksize = arg;
		nbd->bytesize &= ~(nbd->blksize-1);
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_SET_SIZE:
		nbd->bytesize = arg & ~(nbd->blksize-1);
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_SET_TIMEOUT:
		nbd->xmit_timeout = arg * HZ;
		if (arg)
			mod_timer(&nbd->timeout_timer,
				  jiffies + nbd->xmit_timeout);
		else
			del_timer_sync(&nbd->timeout_timer);

		return 0;

	case NBD_SET_FLAGS:
		nbd->flags = arg;
		return 0;

	case NBD_SET_SIZE_BLOCKS:
		nbd->bytesize = ((u64) arg) * nbd->blksize;
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_DO_IT: {
		struct task_struct *thread;
		struct socket *sock;
		int error;

		if (nbd->task_recv)
			return -EBUSY;
		if (!nbd->sock)
			return -EINVAL;

		mutex_unlock(&nbd->tx_lock);

		if (nbd->flags & NBD_FLAG_READ_ONLY)
			set_device_ro(bdev, true);
		if (nbd->flags & NBD_FLAG_SEND_TRIM)
			queue_flag_set_unlocked(QUEUE_FLAG_DISCARD,
				nbd->disk->queue);
		if (nbd->flags & NBD_FLAG_SEND_FLUSH)
			blk_queue_flush(nbd->disk->queue, REQ_FLUSH);
		else
			blk_queue_flush(nbd->disk->queue, 0);

		thread = kthread_run(nbd_thread_send, nbd, "%s",
				     nbd_name(nbd));
		if (IS_ERR(thread)) {
			mutex_lock(&nbd->tx_lock);
			return PTR_ERR(thread);
		}

		nbd_dev_dbg_init(nbd);
		error = nbd_thread_recv(nbd);
		nbd_dev_dbg_close(nbd);
		kthread_stop(thread);

		mutex_lock(&nbd->tx_lock);

		sock_shutdown(nbd);
		sock = nbd->sock;
		nbd->sock = NULL;
		nbd_clear_que(nbd);
		kill_bdev(bdev);
		queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, nbd->disk->queue);
		set_device_ro(bdev, false);
		if (sock)
			sockfd_put(sock);
		nbd->flags = 0;
		nbd->bytesize = 0;
		bdev->bd_inode->i_size = 0;
		set_capacity(nbd->disk, 0);
		if (max_part > 0)
			blkdev_reread_part(bdev);
		if (nbd->disconnect) /* user requested, ignore socket errors */
			return 0;
		return error;
	}

	case NBD_CLEAR_QUE:
		/*
		 * This is for compatibility only.  The queue is always cleared
		 * by NBD_DO_IT or NBD_CLEAR_SOCK.
		 */
		return 0;

	case NBD_PRINT_DEBUG:
		dev_info(disk_to_dev(nbd->disk),
			"next = %p, prev = %p, head = %p\n",
			nbd->queue_head.next, nbd->queue_head.prev,
			&nbd->queue_head);
		return 0;
	}
	return -ENOTTY;
}

static int nbd_ioctl(struct block_device *bdev, fmode_t mode,
		     unsigned int cmd, unsigned long arg)
{
	struct nbd_device *nbd = bdev->bd_disk->private_data;
	int error;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	BUG_ON(nbd->magic != NBD_MAGIC);

	mutex_lock(&nbd->tx_lock);
	error = __nbd_ioctl(bdev, nbd, cmd, arg);
	mutex_unlock(&nbd->tx_lock);

	return error;
}

static const struct block_device_operations nbd_fops =
{
	.owner =	THIS_MODULE,
	.ioctl =	nbd_ioctl,
};

#if IS_ENABLED(CONFIG_DEBUG_FS)

static int nbd_dbg_tasks_show(struct seq_file *s, void *unused)
{
	struct nbd_device *nbd = s->private;

	if (nbd->task_recv)
		seq_printf(s, "recv: %d\n", task_pid_nr(nbd->task_recv));
	if (nbd->task_send)
		seq_printf(s, "send: %d\n", task_pid_nr(nbd->task_send));

	return 0;
}

static int nbd_dbg_tasks_open(struct inode *inode, struct file *file)
{
	return single_open(file, nbd_dbg_tasks_show, inode->i_private);
}

static const struct file_operations nbd_dbg_tasks_ops = {
	.open = nbd_dbg_tasks_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int nbd_dbg_flags_show(struct seq_file *s, void *unused)
{
	struct nbd_device *nbd = s->private;
	u32 flags = nbd->flags;

	seq_printf(s, "Hex: 0x%08x\n\n", flags);

	seq_puts(s, "Known flags:\n");

	if (flags & NBD_FLAG_HAS_FLAGS)
		seq_puts(s, "NBD_FLAG_HAS_FLAGS\n");
	if (flags & NBD_FLAG_READ_ONLY)
		seq_puts(s, "NBD_FLAG_READ_ONLY\n");
	if (flags & NBD_FLAG_SEND_FLUSH)
		seq_puts(s, "NBD_FLAG_SEND_FLUSH\n");
	if (flags & NBD_FLAG_SEND_TRIM)
		seq_puts(s, "NBD_FLAG_SEND_TRIM\n");

	return 0;
}

static int nbd_dbg_flags_open(struct inode *inode, struct file *file)
{
	return single_open(file, nbd_dbg_flags_show, inode->i_private);
}

static const struct file_operations nbd_dbg_flags_ops = {
	.open = nbd_dbg_flags_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int nbd_dev_dbg_init(struct nbd_device *nbd)
{
	struct dentry *dir;
	struct dentry *f;

	dir = debugfs_create_dir(nbd_name(nbd), nbd_dbg_dir);
	if (IS_ERR_OR_NULL(dir)) {
		dev_err(nbd_to_dev(nbd), "Failed to create debugfs dir for '%s' (%ld)\n",
			nbd_name(nbd), PTR_ERR(dir));
		return PTR_ERR(dir);
	}
	nbd->dbg_dir = dir;

	f = debugfs_create_file("tasks", 0444, dir, nbd, &nbd_dbg_tasks_ops);
	if (IS_ERR_OR_NULL(f)) {
		dev_err(nbd_to_dev(nbd), "Failed to create debugfs file 'tasks', %ld\n",
			PTR_ERR(f));
		return PTR_ERR(f);
	}

	f = debugfs_create_u64("size_bytes", 0444, dir, &nbd->bytesize);
	if (IS_ERR_OR_NULL(f)) {
		dev_err(nbd_to_dev(nbd), "Failed to create debugfs file 'size_bytes', %ld\n",
			PTR_ERR(f));
		return PTR_ERR(f);
	}

	f = debugfs_create_u32("timeout", 0444, dir, &nbd->xmit_timeout);
	if (IS_ERR_OR_NULL(f)) {
		dev_err(nbd_to_dev(nbd), "Failed to create debugfs file 'timeout', %ld\n",
			PTR_ERR(f));
		return PTR_ERR(f);
	}

	f = debugfs_create_u32("blocksize", 0444, dir, &nbd->blksize);
	if (IS_ERR_OR_NULL(f)) {
		dev_err(nbd_to_dev(nbd), "Failed to create debugfs file 'blocksize', %ld\n",
			PTR_ERR(f));
		return PTR_ERR(f);
	}

	f = debugfs_create_file("flags", 0444, dir, &nbd, &nbd_dbg_flags_ops);
	if (IS_ERR_OR_NULL(f)) {
		dev_err(nbd_to_dev(nbd), "Failed to create debugfs file 'flags', %ld\n",
			PTR_ERR(f));
		return PTR_ERR(f);
	}

	return 0;
}

static void nbd_dev_dbg_close(struct nbd_device *nbd)
{
	debugfs_remove_recursive(nbd->dbg_dir);
}

static int nbd_dbg_init(void)
{
	struct dentry *dbg_dir;

	dbg_dir = debugfs_create_dir("nbd", NULL);
	if (IS_ERR(dbg_dir))
		return PTR_ERR(dbg_dir);

	nbd_dbg_dir = dbg_dir;

	return 0;
}

static void nbd_dbg_close(void)
{
	debugfs_remove_recursive(nbd_dbg_dir);
}

#else  /* IS_ENABLED(CONFIG_DEBUG_FS) */

static int nbd_dev_dbg_init(struct nbd_device *nbd)
{
	return 0;
}

static void nbd_dev_dbg_close(struct nbd_device *nbd)
{
}

static int nbd_dbg_init(void)
{
	return 0;
}

static void nbd_dbg_close(void)
{
}

#endif

/*
 * And here should be modules and kernel interface 
 *  (Just smiley confuses emacs :-)
 */

static int __init nbd_init(void)
{
	int err = -ENOMEM;
	int i;
	int part_shift;

	BUILD_BUG_ON(sizeof(struct nbd_request) != 28);

	if (max_part < 0) {
		printk(KERN_ERR "nbd: max_part must be >= 0\n");
		return -EINVAL;
	}

	part_shift = 0;
	if (max_part > 0) {
		part_shift = fls(max_part);

		/*
		 * Adjust max_part according to part_shift as it is exported
		 * to user space so that user can know the max number of
		 * partition kernel should be able to manage.
		 *
		 * Note that -1 is required because partition 0 is reserved
		 * for the whole disk.
		 */
		max_part = (1UL << part_shift) - 1;
	}

	if ((1UL << part_shift) > DISK_MAX_PARTS)
		return -EINVAL;

	if (nbds_max > 1UL << (MINORBITS - part_shift))
		return -EINVAL;

	nbd_dev = kcalloc(nbds_max, sizeof(*nbd_dev), GFP_KERNEL);
	if (!nbd_dev)
		return -ENOMEM;

	for (i = 0; i < nbds_max; i++) {
		struct gendisk *disk = alloc_disk(1 << part_shift);
		if (!disk)
			goto out;
		nbd_dev[i].disk = disk;
		/*
		 * The new linux 2.5 block layer implementation requires
		 * every gendisk to have its very own request_queue struct.
		 * These structs are big so we dynamically allocate them.
		 */
		disk->queue = blk_init_queue(nbd_request_handler, &nbd_lock);
		if (!disk->queue) {
			put_disk(disk);
			goto out;
		}
		/*
		 * Tell the block layer that we are not a rotational device
		 */
		queue_flag_set_unlocked(QUEUE_FLAG_NONROT, disk->queue);
		queue_flag_clear_unlocked(QUEUE_FLAG_ADD_RANDOM, disk->queue);
		disk->queue->limits.discard_granularity = 512;
		blk_queue_max_discard_sectors(disk->queue, UINT_MAX);
		disk->queue->limits.discard_zeroes_data = 0;
		blk_queue_max_hw_sectors(disk->queue, 65536);
		disk->queue->limits.max_sectors = 256;
	}

	if (register_blkdev(NBD_MAJOR, "nbd")) {
		err = -EIO;
		goto out;
	}

	printk(KERN_INFO "nbd: registered device at major %d\n", NBD_MAJOR);

	nbd_dbg_init();

	for (i = 0; i < nbds_max; i++) {
		struct gendisk *disk = nbd_dev[i].disk;
		nbd_dev[i].magic = NBD_MAGIC;
		INIT_LIST_HEAD(&nbd_dev[i].waiting_queue);
		spin_lock_init(&nbd_dev[i].queue_lock);
		spin_lock_init(&nbd_dev[i].tasks_lock);
		INIT_LIST_HEAD(&nbd_dev[i].queue_head);
		mutex_init(&nbd_dev[i].tx_lock);
		init_timer(&nbd_dev[i].timeout_timer);
		nbd_dev[i].timeout_timer.function = nbd_xmit_timeout;
		nbd_dev[i].timeout_timer.data = (unsigned long)&nbd_dev[i];
		init_waitqueue_head(&nbd_dev[i].active_wq);
		init_waitqueue_head(&nbd_dev[i].waiting_wq);
		nbd_dev[i].blksize = 1024;
		nbd_dev[i].bytesize = 0;
		disk->major = NBD_MAJOR;
		disk->first_minor = i << part_shift;
		disk->fops = &nbd_fops;
		disk->private_data = &nbd_dev[i];
		sprintf(disk->disk_name, "nbd%d", i);
		set_capacity(disk, 0);
		add_disk(disk);
	}

	return 0;
out:
	while (i--) {
		blk_cleanup_queue(nbd_dev[i].disk->queue);
		put_disk(nbd_dev[i].disk);
	}
	kfree(nbd_dev);
	return err;
}

static void __exit nbd_cleanup(void)
{
	int i;

	nbd_dbg_close();

	for (i = 0; i < nbds_max; i++) {
		struct gendisk *disk = nbd_dev[i].disk;
		nbd_dev[i].magic = 0;
		if (disk) {
			del_gendisk(disk);
			blk_cleanup_queue(disk->queue);
			put_disk(disk);
		}
	}
	unregister_blkdev(NBD_MAJOR, "nbd");
	kfree(nbd_dev);
	printk(KERN_INFO "nbd: unregistered device at major %d\n", NBD_MAJOR);
}

module_init(nbd_init);
module_exit(nbd_cleanup);

MODULE_DESCRIPTION("Network Block Device");
MODULE_LICENSE("GPL");

module_param(nbds_max, int, 0444);
MODULE_PARM_DESC(nbds_max, "number of network block devices to initialize (default: 16)");
module_param(max_part, int, 0444);
MODULE_PARM_DESC(max_part, "number of partitions per device (default: 0)");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                #include <linux/module.h>

#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/blk-mq.h>
#include <linux/hrtimer.h>
#include <linux/lightnvm.h>

struct nullb_cmd {
	struct list_head list;
	struct llist_node ll_list;
	struct call_single_data csd;
	struct request *rq;
	struct bio *bio;
	unsigned int tag;
	struct nullb_queue *nq;
	struct hrtimer timer;
};

struct nullb_queue {
	unsigned long *tag_map;
	wait_queue_head_t wait;
	unsigned int queue_depth;

	struct nullb_cmd *cmds;
};

struct nullb {
	struct list_head list;
	unsigned int index;
	struct request_queue *q;
	struct gendisk *disk;
	struct blk_mq_tag_set tag_set;
	struct hrtimer timer;
	unsigned int queue_depth;
	spinlock_t lock;

	struct nullb_queue *queues;
	unsigned int nr_queues;
	char disk_name[DISK_NAME_LEN];
};

static LIST_HEAD(nullb_list);
static struct mutex lock;
static int null_major;
static int nullb_indexes;
static struct kmem_cache *ppa_cache;

enum {
	NULL_IRQ_NONE		= 0,
	NULL_IRQ_SOFTIRQ	= 1,
	NULL_IRQ_TIMER		= 2,
};

enum {
	NULL_Q_BIO		= 0,
	NULL_Q_RQ		= 1,
	NULL_Q_MQ		= 2,
};

static int submit_queues;
module_param(submit_queues, int, S_IRUGO);
MODULE_PARM_DESC(submit_queues, "Number of submission queues");

static int home_node = NUMA_NO_NODE;
module_param(home_node, int, S_IRUGO);
MODULE_PARM_DESC(home_node, "Home node for the device");

static int queue_mode = NULL_Q_MQ;

static int null_param_store_val(const char *str, int *val, int min, int max)
{
	int ret, new_val;

	ret = kstrtoint(str, 10, &new_val);
	if (ret)
		return -EINVAL;

	if (new_val < min || new_val > max)
		return -EINVAL;

	*val = new_val;
	return 0;
}

static int null_set_queue_mode(const char *str, const struct kernel_param *kp)
{
	return null_param_store_val(str, &queue_mode, NULL_Q_BIO, NULL_Q_MQ);
}

static const struct kernel_param_ops null_queue_mode_param_ops = {
	.set	= null_set_queue_mode,
	.get	= param_get_int,
};

device_param_cb(queue_mode, &null_queue_mode_param_ops, &queue_mode, S_IRUGO);
MODULE_PARM_DESC(queue_mode, "Block interface to use (0=bio,1=rq,2=multiqueue)");

static int gb = 250;
module_param(gb, int, S_IRUGO);
MODULE_PARM_DESC(gb, "Size in GB");

static int bs = 512;
module_param(bs, int, S_IRUGO);
MODULE_PARM_DESC(bs, "Block size (in bytes)");

static int nr_devices = 2;
module_param(nr_devices, int, S_IRUGO);
MODULE_PARM_DESC(nr_devices, "Number of devices to register");

static bool use_lightnvm;
module_param(use_lightnvm, bool, S_IRUGO);
MODULE_PARM_DESC(use_lightnvm, "Register as a LightNVM device");

static int irqmode = NULL_IRQ_SOFTIRQ;

static int null_set_irqmode(const char *str, const struct kernel_param *kp)
{
	return null_param_store_val(str, &irqmode, NULL_IRQ_NONE,
					NULL_IRQ_TIMER);
}

static const struct kernel_param_ops null_irqmode_param_ops = {
	.set	= null_set_irqmode,
	.get	= param_get_int,
};

device_param_cb(irqmode, &null_irqmode_param_ops, &irqmode, S_IRUGO);
MODULE_PARM_DESC(irqmode, "IRQ completion handler. 0-none, 1-softirq, 2-timer");

static unsigned long completion_nsec = 10000;
module_param(completion_nsec, ulong, S_IRUGO);
MODULE_PARM_DESC(completion_nsec, "Time in ns to complete a request in hardware. Default: 10,000ns");

static int hw_queue_depth = 64;
module_param(hw_queue_depth, int, S_IRUGO);
MODULE_PARM_DESC(hw_queue_depth, "Queue depth for each hardware queue. Default: 64");

static bool use_per_node_hctx = false;
module_param(use_per_node_hctx, bool, S_IRUGO);
MODULE_PARM_DESC(use_per_node_hctx, "Use per-node allocation for hardware context queues. Default: false");

static void put_tag(struct nullb_queue *nq, unsigned int tag)
{
	clear_bit_unlock(tag, nq->tag_map);

	if (waitqueue_active(&nq->wait))
		wake_up(&nq->wait);
}

static unsigned int get_tag(struct nullb_queue *nq)
{
	unsigned int tag;

	do {
		tag = find_first_zero_bit(nq->tag_map, nq->queue_depth);
		if (tag >= nq->queue_depth)
			return -1U;
	} while (test_and_set_bit_lock(tag, nq->tag_map));

	return tag;
}

static void free_cmd(struct nullb_cmd *cmd)
{
	put_tag(cmd->nq, cmd->tag);
}

static enum hrtimer_restart null_cmd_timer_expired(struct hrtimer *timer);

static struct nullb_cmd *__alloc_cmd(struct nullb_queue *nq)
{
	struct nullb_cmd *cmd;
	unsigned int tag;

	tag = get_tag(nq);
	if (tag != -1U) {
		cmd = &nq->cmds[tag];
		cmd->tag = tag;
		cmd->nq = nq;
		if (irqmode == NULL_IRQ_TIMER) {
			hrtimer_init(&cmd->timer, CLOCK_MONOTONIC,
				     HRTIMER_MODE_REL);
			cmd->timer.function = null_cmd_timer_expired;
		}
		return cmd;
	}

	return NULL;
}

static struct nullb_cmd *alloc_cmd(struct nullb_queue *nq, int can_wait)
{
	struct nullb_cmd *cmd;
	DEFINE_WAIT(wait);

	cmd = __alloc_cmd(nq);
	if (cmd || !can_wait)
		return cmd;

	do {
		prepare_to_wait(&nq->wait, &wait, TASK_UNINTERRUPTIBLE);
		cmd = __alloc_cmd(nq);
		if (cmd)
			break;

		io_schedule();
	} while (1);

	finish_wait(&nq->wait, &wait);
	return cmd;
}

static void end_cmd(struct nullb_cmd *cmd)
{
	struct request_queue *q = NULL;

	if (cmd->rq)
		q = cmd->rq->q;

	switch (queue_mode)  {
	case NULL_Q_MQ:
		blk_mq_end_request(cmd->rq, 0);
		return;
	case NULL_Q_RQ:
		INIT_LIST_HEAD(&cmd->rq->queuelist);
		blk_end_request_all(cmd->rq, 0);
		break;
	case NULL_Q_BIO:
		bio_endio(cmd->bio);
		break;
	}

	free_cmd(cmd);

	/* Restart queue if needed, as we are freeing a tag */
	if (queue_mode == NULL_Q_RQ && blk_queue_stopped(q)) {
		unsigned long flags;

		spin_lock_irqsave(q->queue_lock, flags);
		blk_start_queue_async(q);
		spin_unlock_irqrestore(q->queue_lock, flags);
	}
}

static enum hrtimer_restart null_cmd_timer_expired(struct hrtimer *timer)
{
	end_cmd(container_of(timer, struct nullb_cmd, timer));

	return HRTIMER_NORESTART;
}

static void null_cmd_end_timer(struct nullb_cmd *cmd)
{
	ktime_t kt = ktime_set(0, completion_nsec);

	hrtimer_start(&cmd->timer, kt, HRTIMER_MODE_REL);
}

static void null_softirq_done_fn(struct request *rq)
{
	if (queue_mode == NULL_Q_MQ)
		end_cmd(blk_mq_rq_to_pdu(rq));
	else
		end_cmd(rq->special);
}

static inline void null_handle_cmd(struct nullb_cmd *cmd)
{
	/* Complete IO by inline, softirq or timer */
	switch (irqmode) {
	case NULL_IRQ_SOFTIRQ:
		switch (queue_mode)  {
		case NULL_Q_MQ:
			blk_mq_complete_request(cmd->rq, cmd->rq->errors);
			break;
		case NULL_Q_RQ:
			blk_complete_request(cmd->rq);
			break;
		case NULL_Q_BIO:
			/*
			 * XXX: no proper submitting cpu information available.
			 */
			end_cmd(cmd);
			break;
		}
		break;
	case NULL_IRQ_NONE:
		end_cmd(cmd);
		break;
	case NULL_IRQ_TIMER:
		null_cmd_end_timer(cmd);
		break;
	}
}

static struct nullb_queue *nullb_to_queue(struct nullb *nullb)
{
	int index = 0;

	if (nullb->nr_queues != 1)
		index = raw_smp_processor_id() / ((nr_cpu_ids + nullb->nr_queues - 1) / nullb->nr_queues);

	return &nullb->queues[index];
}

static blk_qc_t null_queue_bio(struct request_queue *q, struct bio *bio)
{
	struct nullb *nullb = q->queuedata;
	struct nullb_queue *nq = nullb_to_queue(nullb);
	struct nullb_cmd *cmd;

	cmd = alloc_cmd(nq, 1);
	cmd->bio = bio;

	null_handle_cmd(cmd);
	return BLK_QC_T_NONE;
}

static int null_rq_prep_fn(struct request_queue *q, struct request *req)
{
	struct nullb *nullb = q->queuedata;
	struct nullb_queue *nq = nullb_to_queue(nullb);
	struct nullb_cmd *cmd;

	cmd = alloc_cmd(nq, 0);
	if (cmd) {
		cmd->rq = req;
		req->special = cmd;
		return BLKPREP_OK;
	}
	blk_stop_queue(q);

	return BLKPREP_DEFER;
}

static void null_request_fn(struct request_queue *q)
{
	struct request *rq;

	while ((rq = blk_fetch_request(q)) != NULL) {
		struct nullb_cmd *cmd = rq->special;

		spin_unlock_irq(q->queue_lock);
		null_handle_cmd(cmd);
		spin_lock_irq(q->queue_lock);
	}
}

static int null_queue_rq(struct blk_mq_hw_ctx *hctx,
			 const struct blk_mq_queue_data *bd)
{
	struct nullb_cmd *cmd = blk_mq_rq_to_pdu(bd->rq);

	if (irqmode == NULL_IRQ_TIMER) {
		hrtimer_init(&cmd->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		cmd->timer.function = null_cmd_timer_expired;
	}
	cmd->rq = bd->rq;
	cmd->nq = hctx->driver_data;

	blk_mq_start_request(bd->rq);

	null_handle_cmd(cmd);
	return BLK_MQ_RQ_QUEUE_OK;
}

static void null_init_queue(struct nullb *nullb, struct nullb_queue *nq)
{
	BUG_ON(!nullb);
	BUG_ON(!nq);

	init_waitqueue_head(&nq->wait);
	nq->queue_depth = nullb->queue_depth;
}

static int null_init_hctx(struct blk_mq_hw_ctx *hctx, void *data,
			  unsigned int index)
{
	struct nullb *nullb = data;
	struct nullb_queue *nq = &nullb->queues[index];

	hctx->driver_data = nq;
	null_init_queue(nullb, nq);
	nullb->nr_queues++;

	return 0;
}

static struct blk_mq_ops null_mq_ops = {
	.queue_rq       = null_queue_rq,
	.map_queue      = blk_mq_map_queue,
	.init_hctx	= null_init_hctx,
	.complete	= null_softirq_done_fn,
};

static void cleanup_queue(struct nullb_queue *nq)
{
	kfree(nq->tag_map);
	kfree(nq->cmds);
}

static void cleanup_queues(struct nullb *nullb)
{
	int i;

	for (i = 0; i < nullb->nr_queues; i++)
		cleanup_queue(&nullb->queues[i]);

	kfree(nullb->queues);
}

static void null_del_dev(struct nullb *nullb)
{
	list_del_init(&nullb->list);

	if (use_lightnvm)
		nvm_unregister(nullb->disk_name);
	else
		del_gendisk(nullb->disk);
	blk_cleanup_queue(nullb->q);
	if (queue_mode == NULL_Q_MQ)
		blk_mq_free_tag_set(&nullb->tag_set);
	if (!use_lightnvm)
		put_disk(nullb->disk);
	cleanup_queues(nullb);
	kfree(nullb);
}

#ifdef CONFIG_NVM

static void null_lnvm_end_io(struct request *rq, int error)
{
	struct nvm_rq *rqd = rq->end_io_data;
	struct nvm_dev *dev = rqd->dev;

	dev->mt->end_io(rqd, error);

	blk_put_request(rq);
}

static int null_lnvm_submit_io(struct nvm_dev *dev, struct nvm_rq *rqd)
{
	struct request_queue *q = dev->q;
	struct request *rq;
	struct bio *bio = rqd->bio;

	rq = blk_mq_alloc_request(q, bio_rw(bio), GFP_KERNEL, 0);
	if (IS_ERR(rq))
		return -ENOMEM;

	rq->cmd_type = REQ_TYPE_DRV_PRIV;
	rq->__sector = bio->bi_iter.bi_sector;
	rq->ioprio = bio_prio(bio);

	if (bio_has_data(bio))
		rq->nr_phys_segments = bio_phys_segments(q, bio);

	rq->__data_len = bio->bi_iter.bi_size;
	rq->bio = rq->biotail = bio;

	rq->end_io_data = rqd;

	blk_execute_rq_nowait(q, NULL, rq, 0, null_lnvm_end_io);

	return 0;
}

static int null_lnvm_id(struct nvm_dev *dev, struct nvm_id *id)
{
	sector_t size = gb * 1024 * 1024 * 1024ULL;
	sector_t blksize;
	struct nvm_id_group *grp;

	id->ver_id = 0x1;
	id->vmnt = 0;
	id->cgrps = 1;
	id->cap = 0x3;
	id->dom = 0x1;

	id->ppaf.blk_offset = 0;
	id->ppaf.blk_len = 16;
	id->ppaf.pg_offset = 16;
	id->ppaf.pg_len = 16;
	id->ppaf.sect_offset = 32;
	id->ppaf.sect_len = 8;
	id->ppaf.pln_offset = 40;
	id->ppaf.pln_len = 8;
	id->ppaf.lun_offset = 48;
	id->ppaf.lun_len = 8;
	id->ppaf.ch_offset = 56;
	id->ppaf.ch_len = 8;

	do_div(size, bs); /* convert size to pages */
	do_div(size, 256); /* concert size to pgs pr blk */
	grp = &id->groups[0];
	grp->mtype = 0;
	grp->fmtype = 0;
	grp->num_ch = 1;
	grp->num_pg = 256;
	blksize = size;
	do_div(size, (1 << 16));
	grp->num_lun = size + 1;
	do_div(blksize, grp->num_lun);
	grp->num_blk = blksize;
	grp->num_pln = 1;

	grp->fpg_sz = bs;
	grp->csecs = bs;
	grp->trdt = 25000;
	grp->trdm = 25000;
	grp->tprt = 500000;
	grp->tprm = 500000;
	grp->tbet = 1500000;
	grp->tbem = 1500000;
	grp->mpos = 0x010101; /* single plane rwe */
	grp->cpar = hw_queue_depth;

	return 0;
}

static void *null_lnvm_create_dma_pool(struct nvm_dev *dev, char *name)
{
	mempool_t *virtmem_pool;

	virtmem_pool = mempool_create_slab_pool(64, ppa_cache);
	if (!virtmem_pool) {
		pr_err("null_blk: Unable to create virtual memory pool\n");
		return NULL;
	}

	return virtmem_pool;
}

static void null_lnvm_destroy_dma_pool(void *pool)
{
	mempool_destroy(pool);
}

static void *null_lnvm_dev_dma_alloc(struct nvm_dev *dev, void *pool,
				gfp_t mem_flags, dma_addr_t *dma_handler)
{
	return mempool_alloc(pool, mem_flags);
}

static void null_lnvm_dev_dma_free(void *pool, void *entry,
							dma_addr_t dma_handler)
{
	mempool_free(entry, pool);
}

static struct nvm_dev_ops null_lnvm_dev_ops = {
	.identity		= null_lnvm_id,
	.submit_io		= null_lnvm_submit_io,

	.create_dma_pool	= null_lnvm_create_dma_pool,
	.destroy_dma_pool	= null_lnvm_destroy_dma_pool,
	.dev_dma_alloc		= null_lnvm_dev_dma_alloc,
	.dev_dma_free		= null_lnvm_dev_dma_free,

	/* Simulate nvme protocol restriction */
	.max_phys_sect		= 64,
};
#else
static struct nvm_dev_ops null_lnvm_dev_ops;
#endif /* CONFIG_NVM */

static int null_open(struct block_device *bdev, fmode_t mode)
{
	return 0;
}

static void null_release(struct gendisk *disk, fmode_t mode)
{
}

static const struct block_device_operations null_fops = {
	.owner =	THIS_MODULE,
	.open =		null_open,
	.release =	null_release,
};

static int setup_commands(struct nullb_queue *nq)
{
	struct nullb_cmd *cmd;
	int i, tag_size;

	nq->cmds = kzalloc(nq->queue_depth * sizeof(*cmd), GFP_KERNEL);
	if (!nq->cmds)
		return -ENOMEM;

	tag_size = ALIGN(nq->queue_depth, BITS_PER_LONG) / BITS_PER_LONG;
	nq->tag_map = kzalloc(tag_size * sizeof(unsigned long), GFP_KERNEL);
	if (!nq->tag_map) {
		kfree(nq->cmds);
		return -ENOMEM;
	}

	for (i = 0; i < nq->queue_depth; i++) {
		cmd = &nq->cmds[i];
		INIT_LIST_HEAD(&cmd->list);
		cmd->ll_list.next = NULL;
		cmd->tag = -1U;
	}

	return 0;
}

static int setup_queues(struct nullb *nullb)
{
	nullb->queues = kzalloc(submit_queues * sizeof(struct nullb_queue),
								GFP_KERNEL);
	if (!nullb->queues)
		return -ENOMEM;

	nullb->nr_queues = 0;
	nullb->queue_depth = hw_queue_depth;

	return 0;
}

static int init_driver_queues(struct nullb *nullb)
{
	struct nullb_queue *nq;
	int i, ret = 0;

	for (i = 0; i < submit_queues; i++) {
		nq = &nullb->queues[i];

		null_init_queue(nullb, nq);

		ret = setup_commands(nq);
		if (ret)
			return ret;
		nullb->nr_queues++;
	}
	return 0;
}

static int null_add_dev(void)
{
	struct gendisk *disk;
	struct nullb *nullb;
	sector_t size;
	int rv;

	nullb = kzalloc_node(sizeof(*nullb), GFP_KERNEL, home_node);
	if (!nullb) {
		rv = -ENOMEM;
		goto out;
	}

	spin_lock_init(&nullb->lock);

	if (queue_mode == NULL_Q_MQ && use_per_node_hctx)
		submit_queues = nr_online_nodes;

	rv = setup_queues(nullb);
	if (rv)
		goto out_free_nullb;

	if (queue_mode == NULL_Q_MQ) {
		nullb->tag_set.ops = &null_mq_ops;
		nullb->tag_set.nr_hw_queues = submit_queues;
		nullb->tag_set.queue_depth = hw_queue_depth;
		nullb->tag_set.numa_node = home_node;
		nullb->tag_set.cmd_size	= sizeof(struct nullb_cmd);
		nullb->tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
		nullb->tag_set.driver_data = nullb;

		rv = blk_mq_alloc_tag_set(&nullb->tag_set);
		if (rv)
			goto out_cleanup_queues;

		nullb->q = blk_mq_init_queue(&nullb->tag_set);
		if (IS_ERR(nullb->q)) {
			rv = -ENOMEM;
			goto out_cleanup_tags;
		}
	} else if (queue_mode == NULL_Q_BIO) {
		nullb->q = blk_alloc_queue_node(GFP_KERNEL, home_node);
		if (!nullb->q) {
			rv = -ENOMEM;
			goto out_cleanup_queues;
		}
		blk_queue_make_request(nullb->q, null_queue_bio);
		rv = init_driver_queues(nullb);
		if (rv)
			goto out_cleanup_blk_queue;
	} else {
		nullb->q = blk_init_queue_node(null_request_fn, &nullb->lock, home_node);
		if (!nullb->q) {
			rv = -ENOMEM;
			goto out_cleanup_queues;
		}
		blk_queue_prep_rq(nullb->q, null_rq_prep_fn);
		blk_queue_softirq_done(nullb->q, null_softirq_done_fn);
		rv = init_driver_queues(nullb);
		if (rv)
			goto out_cleanup_blk_queue;
	}

	nullb->q->queuedata = nullb;
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, nullb->q);
	queue_flag_clear_unlocked(QUEUE_FLAG_ADD_RANDOM, nullb->q);


	mutex_lock(&lock);
	list_add_tail(&nullb->list, &nullb_list);
	nullb->index = nullb_indexes++;
	mutex_unlock(&lock);

	blk_queue_logical_block_size(nullb->q, bs);
	blk_queue_physical_block_size(nullb->q, bs);

	sprintf(nullb->disk_name, "nullb%d", nullb->index);

	if (use_lightnvm) {
		rv = nvm_register(nullb->q, nullb->disk_name,
							&null_lnvm_dev_ops);
		if (rv)
			goto out_cleanup_blk_queue;
		goto done;
	}

	disk = nullb->disk = alloc_disk_node(1, home_node);
	if (!disk) {
		rv = -ENOMEM;
		goto out_cleanup_lightnvm;
	}
	size = gb * 1024 * 1024 * 1024ULL;
	set_capacity(disk, size >> 9);

	disk->flags |= GENHD_FL_EXT_DEVT | GENHD_FL_SUPPRESS_PARTITION_INFO;
	disk->major		= null_major;
	disk->first_minor	= nullb->index;
	disk->fops		= &null_fops;
	disk->private_data	= nullb;
	disk->queue		= nullb->q;
	strncpy(disk->disk_name, nullb->disk_name, DISK_NAME_LEN);

	add_disk(disk);
done:
	return 0;

out_cleanup_lightnvm:
	if (use_lightnvm)
		nvm_unregister(nullb->disk_name);
out_cleanup_blk_queue:
	blk_cleanup_queue(nullb->q);
out_cleanup_tags:
	if (queue_mode == NULL_Q_MQ)
		blk_mq_free_tag_set(&nullb->tag_set);
out_cleanup_queues:
	cleanup_queues(nullb);
out_free_nullb:
	kfree(nullb);
out:
	return rv;
}

static int __init null_init(void)
{
	int ret = 0;
	unsigned int i;
	struct nullb *nullb;

	if (bs > PAGE_SIZE) {
		pr_warn("null_blk: invalid block size\n");
		pr_warn("null_blk: defaults block size to %lu\n", PAGE_SIZE);
		bs = PAGE_SIZE;
	}

	if (use_lightnvm && bs != 4096) {
		pr_warn("null_blk: LightNVM only supports 4k block size\n");
		pr_warn("null_blk: defaults block size to 4k\n");
		bs = 4096;
	}

	if (use_lightnvm && queue_mode != NULL_Q_MQ) {
		pr_warn("null_blk: LightNVM only supported for blk-mq\n");
		pr_warn("null_blk: defaults queue mode to blk-mq\n");
		queue_mode = NULL_Q_MQ;
	}

	if (queue_mode == NULL_Q_MQ && use_per_node_hctx) {
		if (submit_queues < nr_online_nodes) {
			pr_warn("null_blk: submit_queues param is set to %u.",
							nr_online_nodes);
			submit_queues = nr_online_nodes;
		}
	} else if (submit_queues > nr_cpu_ids)
		submit_queues = nr_cpu_ids;
	else if (!submit_queues)
		submit_queues = 1;

	mutex_init(&lock);

	null_major = register_blkdev(0, "nullb");
	if (null_major < 0)
		return null_major;

	if (use_lightnvm) {
		ppa_cache = kmem_cache_create("ppa_cache", 64 * sizeof(u64),
								0, 0, NULL);
		if (!ppa_cache) {
			pr_err("null_blk: unable to create ppa cache\n");
			ret = -ENOMEM;
			goto err_ppa;
		}
	}

	for (i = 0; i < nr_devices; i++) {
		ret = null_add_dev();
		if (ret)
			goto err_dev;
	}

	pr_info("null: module loaded\n");
	return 0;

err_dev:
	while (!list_empty(&nullb_list)) {
		nullb = list_entry(nullb_list.next, struct nullb, list);
		null_del_dev(nullb);
	}
	kmem_cache_destroy(ppa_cache);
err_ppa:
	unregister_blkdev(null_major, "nullb");
	return ret;
}

static void __exit null_exit(void)
{
	struct nullb *nullb;

	unregister_blkdev(null_major, "nullb");

	mutex_lock(&lock);
	while (!list_empty(&nullb_list)) {
		nullb = list_entry(nullb_list.next, struct nullb, list);
		null_del_dev(nullb);
	}
	mutex_unlock(&lock);

	kmem_cache_destroy(ppa_cache);
}

module_init(null_init);
module_exit(null_exit);

MODULE_AUTHOR("Jens Axboe <jaxboe@fusionio.com>");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
/*
   osdblk.c -- Export a single SCSI OSD object as a Linux block device


   Copyright 2009 Red Hat, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.


   Instructions for use
   --------------------

   1) Map a Linux block device to an existing OSD object.

      In this example, we will use partition id 1234, object id 5678,
      OSD device /dev/osd1.

      $ echo "1234 5678 /dev/osd1" > /sys/class/osdblk/add


   2) List all active blkdev<->object mappings.

      In this example, we have performed step #1 twice, creating two blkdevs,
      mapped to two separate OSD objects.

      $ cat /sys/class/osdblk/list
      0 174 1234 5678 /dev/osd1
      1 179 1994 897123 /dev/osd0

      The columns, in order, are:
      - blkdev unique id
      - blkdev assigned major
      - OSD object partition id
      - OSD object id
      - OSD device


   3) Remove an active blkdev<->object mapping.

      In this example, we remove the mapping with blkdev unique id 1.

      $ echo 1 > /sys/class/osdblk/remove


   NOTE:  The actual creation and deletion of OSD objects is outside the scope
   of this driver.

 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <scsi/osd_initiator.h>
#include <scsi/osd_attributes.h>
#include <scsi/osd_sec.h>
#include <scsi/scsi_device.h>

#define DRV_NAME "osdblk"
#define PFX DRV_NAME ": "

/* #define _OSDBLK_DEBUG */
#ifdef _OSDBLK_DEBUG
#define OSDBLK_DEBUG(fmt, a...) \
	printk(KERN_NOTICE "osdblk @%s:%d: " fmt, __func__, __LINE__, ##a)
#else
#define OSDBLK_DEBUG(fmt, a...) \
	do { if (0) printk(fmt, ##a); } while (0)
#endif

MODULE_AUTHOR("Jeff Garzik <jeff@garzik.org>");
MODULE_DESCRIPTION("block device inside an OSD object osdblk.ko");
MODULE_LICENSE("GPL");

struct osdblk_device;

enum {
	OSDBLK_MINORS_PER_MAJOR	= 256,		/* max minors per blkdev */
	OSDBLK_MAX_REQ		= 32,		/* max parallel requests */
	OSDBLK_OP_TIMEOUT	= 4 * 60,	/* sync OSD req timeout */
};

struct osdblk_request {
	struct request		*rq;		/* blk layer request */
	struct bio		*bio;		/* cloned bio */
	struct osdblk_device	*osdev;		/* associated blkdev */
};

struct osdblk_device {
	int			id;		/* blkdev unique id */

	int			major;		/* blkdev assigned major */
	struct gendisk		*disk;		/* blkdev's gendisk and rq */
	struct request_queue	*q;

	struct osd_dev		*osd;		/* associated OSD */

	char			name[32];	/* blkdev name, e.g. osdblk34 */

	spinlock_t		lock;		/* queue lock */

	struct osd_obj_id	obj;		/* OSD partition, obj id */
	uint8_t			obj_cred[OSD_CAP_LEN]; /* OSD cred */

	struct osdblk_request	req[OSDBLK_MAX_REQ]; /* request table */

	struct list_head	node;

	char			osd_path[0];	/* OSD device path */
};

static struct class *class_osdblk;		/* /sys/class/osdblk */
static DEFINE_MUTEX(ctl_mutex);	/* Serialize open/close/setup/teardown */
static LIST_HEAD(osdblkdev_list);

static const struct block_device_operations osdblk_bd_ops = {
	.owner		= THIS_MODULE,
};

static const struct osd_attr g_attr_logical_length = ATTR_DEF(
	OSD_APAGE_OBJECT_INFORMATION, OSD_ATTR_OI_LOGICAL_LENGTH, 8);

static void osdblk_make_credential(u8 cred_a[OSD_CAP_LEN],
				   const struct osd_obj_id *obj)
{
	osd_sec_init_nosec_doall_caps(cred_a, obj, false, true);
}

/* copied from exofs; move to libosd? */
/*
 * Perform a synchronous OSD operation.  copied from exofs; move to libosd?
 */
static int osd_sync_op(struct osd_request *or, int timeout, uint8_t *credential)
{
	int ret;

	or->timeout = timeout;
	ret = osd_finalize_request(or, 0, credential, NULL);
	if (ret)
		return ret;

	ret = osd_execute_request(or);

	/* osd_req_decode_sense(or, ret); */
	return ret;
}

/*
 * Perform an asynchronous OSD operation.  copied from exofs; move to libosd?
 */
static int osd_async_op(struct osd_request *or, osd_req_done_fn *async_done,
		   void *caller_context, u8 *cred)
{
	int ret;

	ret = osd_finalize_request(or, 0, cred, NULL);
	if (ret)
		return ret;

	ret = osd_execute_request_async(or, async_done, caller_context);

	return ret;
}

/* copied from exofs; move to libosd? */
static int extract_attr_from_req(struct osd_request *or, struct osd_attr *attr)
{
	struct osd_attr cur_attr = {.attr_page = 0}; /* start with zeros */
	void *iter = NULL;
	int nelem;

	do {
		nelem = 1;
		osd_req_decode_get_attr_list(or, &cur_attr, &nelem, &iter);
		if ((cur_attr.attr_page == attr->attr_page) &&
		    (cur_attr.attr_id == attr->attr_id)) {
			attr->len = cur_attr.len;
			attr->val_ptr = cur_attr.val_ptr;
			return 0;
		}
	} while (iter);

	return -EIO;
}

static int osdblk_get_obj_size(struct osdblk_device *osdev, u64 *size_out)
{
	struct osd_request *or;
	struct osd_attr attr;
	int ret;

	/* start request */
	or = osd_start_request(osdev->osd, GFP_KERNEL);
	if (!or)
		return -ENOMEM;

	/* create a get-attributes(length) request */
	osd_req_get_attributes(or, &osdev->obj);

	osd_req_add_get_attr_list(or, &g_attr_logical_length, 1);

	/* execute op synchronously */
	ret = osd_sync_op(or, OSDBLK_OP_TIMEOUT, osdev->obj_cred);
	if (ret)
		goto out;

	/* extract length from returned attribute info */
	attr = g_attr_logical_length;
	ret = extract_attr_from_req(or, &attr);
	if (ret)
		goto out;

	*size_out = get_unaligned_be64(attr.val_ptr);

out:
	osd_end_request(or);
	return ret;

}

static void osdblk_osd_complete(struct osd_request *or, void *private)
{
	struct osdblk_request *orq = private;
	struct osd_sense_info osi;
	int ret = osd_req_decode_sense(or, &osi);

	if (ret) {
		ret = -EIO;
		OSDBLK_DEBUG("osdblk_osd_complete with err=%d\n", ret);
	}

	/* complete OSD request */
	osd_end_request(or);

	/* complete request passed to osdblk by block layer */
	__blk_end_request_all(orq->rq, ret);
}

static void bio_chain_put(struct bio *chain)
{
	struct bio *tmp;

	while (chain) {
		tmp = chain;
		chain = chain->bi_next;

		bio_put(tmp);
	}
}

static struct bio *bio_chain_clone(struct bio *old_chain, gfp_t gfpmask)
{
	struct bio *tmp, *new_chain = NULL, *tail = NULL;

	while (old_chain) {
		tmp = bio_clone_kmalloc(old_chain, gfpmask);
		if (!tmp)
			goto err_out;

		tmp->bi_bdev = NULL;
		gfpmask &= ~__GFP_DIRECT_RECLAIM;
		tmp->bi_next = NULL;

		if (!new_chain)
			new_chain = tail = tmp;
		else {
			tail->bi_next = tmp;
			tail = tmp;
		}

		old_chain = old_chain->bi_next;
	}

	return new_chain;

err_out:
	OSDBLK_DEBUG("bio_chain_clone with err\n");
	bio_chain_put(new_chain);
	return NULL;
}

static void osdblk_rq_fn(struct request_queue *q)
{
	struct osdblk_device *osdev = q->queuedata;

	while (1) {
		struct request *rq;
		struct osdblk_request *orq;
		struct osd_request *or;
		struct bio *bio;
		bool do_write, do_flush;

		/* peek at request from block layer */
		rq = blk_fetch_request(q);
		if (!rq)
			break;

		/* filter out block requests we don't understand */
		if (rq->cmd_type != REQ_TYPE_FS) {
			blk_end_request_all(rq, 0);
			continue;
		}

		/* deduce our operation (read, write, flush) */
		/* I wish the block layer simplified cmd_type/cmd_flags/cmd[]
		 * into a clearly defined set of RPC commands:
		 * read, write, flush, scsi command, power mgmt req,
		 * driver-specific, etc.
		 */

		do_flush = rq->cmd_flags & REQ_FLUSH;
		do_write = (rq_data_dir(rq) == WRITE);

		if (!do_flush) { /* osd_flush does not use a bio */
			/* a bio clone to be passed down to OSD request */
			bio = bio_chain_clone(rq->bio, GFP_ATOMIC);
			if (!bio)
				break;
		} else
			bio = NULL;

		/* alloc internal OSD request, for OSD command execution */
		or = osd_start_request(osdev->osd, GFP_ATOMIC);
		if (!or) {
			bio_chain_put(bio);
			OSDBLK_DEBUG("osd_start_request with err\n");
			break;
		}

		orq = &osdev->req[rq->tag];
		orq->rq = rq;
		orq->bio = bio;
		orq->osdev = osdev;

		/* init OSD command: flush, write or read */
		if (do_flush)
			osd_req_flush_object(or, &osdev->obj,
					     OSD_CDB_FLUSH_ALL, 0, 0);
		else if (do_write)
			osd_req_write(or, &osdev->obj, blk_rq_pos(rq) * 512ULL,
				      bio, blk_rq_bytes(rq));
		else
			osd_req_read(or, &osdev->obj, blk_rq_pos(rq) * 512ULL,
				     bio, blk_rq_bytes(rq));

		OSDBLK_DEBUG("%s 0x%x bytes at 0x%llx\n",
			do_flush ? "flush" : do_write ?
				"write" : "read", blk_rq_bytes(rq),
			blk_rq_pos(rq) * 512ULL);

		/* begin OSD command execution */
		if (osd_async_op(or, osdblk_osd_complete, orq,
				 osdev->obj_cred)) {
			osd_end_request(or);
			blk_requeue_request(q, rq);
			bio_chain_put(bio);
			OSDBLK_DEBUG("osd_execute_request_async with err\n");
			break;
		}

		/* remove the special 'flush' marker, now that the command
		 * is executing
		 */
		rq->special = NULL;
	}
}

static void osdblk_free_disk(struct osdblk_device *osdev)
{
	struct gendisk *disk = osdev->disk;

	if (!disk)
		return;

	if (disk->flags & GENHD_FL_UP)
		del_gendisk(disk);
	if (disk->queue)
		blk_cleanup_queue(disk->queue);
	put_disk(disk);
}

static int osdblk_init_disk(struct osdblk_device *osdev)
{
	struct gendisk *disk;
	struct request_queue *q;
	int rc;
	u64 obj_size = 0;

	/* contact OSD, request size info about the object being mapped */
	rc = osdblk_get_obj_size(osdev, &obj_size);
	if (rc)
		return rc;

	/* create gendisk info */
	disk = alloc_disk(OSDBLK_MINORS_PER_MAJOR);
	if (!disk)
		return -ENOMEM;

	sprintf(disk->disk_name, DRV_NAME "%d", osdev->id);
	disk->major = osdev->major;
	disk->first_minor = 0;
	disk->fops = &osdblk_bd_ops;
	disk->private_data = osdev;

	/* init rq */
	q = blk_init_queue(osdblk_rq_fn, &osdev->lock);
	if (!q) {
		put_disk(disk);
		return -ENOMEM;
	}

	/* switch queue to TCQ mode; allocate tag map */
	rc = blk_queue_init_tags(q, OSDBLK_MAX_REQ, NULL, BLK_TAG_ALLOC_FIFO);
	if (rc) {
		blk_cleanup_queue(q);
		put_disk(disk);
		return rc;
	}

	/* Set our limits to the lower device limits, because osdblk cannot
	 * sleep when allocating a lower-request and therefore cannot be
	 * bouncing.
	 */
	blk_queue_stack_limits(q, osd_request_queue(osdev->osd));

	blk_queue_prep_rq(q, blk_queue_start_tag);
	blk_queue_flush(q, REQ_FLUSH);

	disk->queue = q;

	q->queuedata = osdev;

	osdev->disk = disk;
	osdev->q = q;

	/* finally, announce the disk to the world */
	set_capacity(disk, obj_size / 512ULL);
	add_disk(disk);

	printk(KERN_INFO "%s: Added of size 0x%llx\n",
		disk->disk_name, (unsigned long long)obj_size);

	return 0;
}

/********************************************************************
 * /sys/class/osdblk/
 *                   add	map OSD object to blkdev
 *                   remove	unmap OSD object
 *                   list	show mappings
 *******************************************************************/

static void class_osdblk_release(struct class *cls)
{
	kfree(cls);
}

static ssize_t class_osdblk_list(struct class *c,
				struct class_attribute *attr,
				char *data)
{
	int n = 0;
	struct list_head *tmp;

	mutex_lock_nested(&ctl_mutex, SINGLE_DEPTH_NESTING);

	list_for_each(tmp, &osdblkdev_list) {
		struct osdblk_device *osdev;

		osdev = list_entry(tmp, struct osdblk_device, node);

		n += sprintf(data+n, "%d %d %llu %llu %s\n",
			osdev->id,
			osdev->major,
			osdev->obj.partition,
			osdev->obj.id,
			osdev->osd_path);
	}

	mutex_unlock(&ctl_mutex);
	return n;
}

static ssize_t class_osdblk_add(struct class *c,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	struct osdblk_device *osdev;
	ssize_t rc;
	int irc, new_id = 0;
	struct list_head *tmp;

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	/* new osdblk_device object */
	osdev = kzalloc(sizeof(*osdev) + strlen(buf) + 1, GFP_KERNEL);
	if (!osdev) {
		rc = -ENOMEM;
		goto err_out_mod;
	}

	/* static osdblk_device initialization */
	spin_lock_init(&osdev->lock);
	INIT_LIST_HEAD(&osdev->node);

	/* generate unique id: find highest unique id, add one */

	mutex_lock_nested(&ctl_mutex, SINGLE_DEPTH_NESTING);

	list_for_each(tmp, &osdblkdev_list) {
		struct osdblk_device *osdev;

		osdev = list_entry(tmp, struct osdblk_device, node);
		if (osdev->id > new_id)
			new_id = osdev->id + 1;
	}

	osdev->id = new_id;

	/* add to global list */
	list_add_tail(&osdev->node, &osdblkdev_list);

	mutex_unlock(&ctl_mutex);

	/* parse add command */
	if (sscanf(buf, "%llu %llu %s", &osdev->obj.partition, &osdev->obj.id,
		   osdev->osd_path) != 3) {
		rc = -EINVAL;
		goto err_out_slot;
	}

	/* initialize rest of new object */
	sprintf(osdev->name, DRV_NAME "%d", osdev->id);

	/* contact requested OSD */
	osdev->osd = osduld_path_lookup(osdev->osd_path);
	if (IS_ERR(osdev->osd)) {
		rc = PTR_ERR(osdev->osd);
		goto err_out_slot;
	}

	/* build OSD credential */
	osdblk_make_credential(osdev->obj_cred, &osdev->obj);

	/* register our block device */
	irc = register_blkdev(0, osdev->name);
	if (irc < 0) {
		rc = irc;
		goto err_out_osd;
	}

	osdev->major = irc;

	/* set up and announce blkdev mapping */
	rc = osdblk_init_disk(osdev);
	if (rc)
		goto err_out_blkdev;

	return count;

err_out_blkdev:
	unregister_blkdev(osdev->major, osdev->name);
err_out_osd:
	osduld_put_device(osdev->osd);
err_out_slot:
	mutex_lock_nested(&ctl_mutex, SINGLE_DEPTH_NESTING);
	list_del_init(&osdev->node);
	mutex_unlock(&ctl_mutex);

	kfree(osdev);
err_out_mod:
	OSDBLK_DEBUG("Error adding device %s\n", buf);
	module_put(THIS_MODULE);
	return rc;
}

static ssize_t class_osdblk_remove(struct class *c,
					struct class_attribute *attr,
					const char *buf,
					size_t count)
{
	struct osdblk_device *osdev = NULL;
	int target_id, rc;
	unsigned long ul;
	struct list_head *tmp;

	rc = kstrtoul(buf, 10, &ul);
	if (rc)
		return rc;

	/* convert to int; abort if we lost anything in the conversion */
	target_id = (int) ul;
	if (target_id != ul)
		return -EINVAL;

	/* remove object from list immediately */
	mutex_lock_nested(&ctl_mutex, SINGLE_DEPTH_NESTING);

	list_for_each(tmp, &osdblkdev_list) {
		osdev = list_entry(tmp, struct osdblk_device, node);
		if (osdev->id == target_id) {
			list_del_init(&osdev->node);
			break;
		}
		osdev = NULL;
	}

	mutex_unlock(&ctl_mutex);

	if (!osdev)
		return -ENOENT;

	/* clean up and free blkdev and associated OSD connection */
	osdblk_free_disk(osdev);
	unregister_blkdev(osdev->major, osdev->name);
	osduld_put_device(osdev->osd);
	kfree(osdev);

	/* release module ref */
	module_put(THIS_MODULE);

	return count;
}

static struct class_attribute class_osdblk_attrs[] = {
	__ATTR(add,	0200, NULL, class_osdblk_add),
	__ATTR(remove,	0200, NULL, class_osdblk_remove),
	__ATTR(list,	0444, class_osdblk_list, NULL),
	__ATTR_NULL
};

static int osdblk_sysfs_init(void)
{
	int ret = 0;

	/*
	 * create control files in sysfs
	 * /sys/class/osdblk/...
	 */
	class_osdblk = kzalloc(sizeof(*class_osdblk), GFP_KERNEL);
	if (!class_osdblk)
		return -ENOMEM;

	class_osdblk->name = DRV_NAME;
	class_osdblk->owner = THIS_MODULE;
	class_osdblk->class_release = class_osdblk_release;
	class_osdblk->class_attrs = class_osdblk_attrs;

	ret = class_register(class_osdblk);
	if (ret) {
		kfree(class_osdblk);
		class_osdblk = NULL;
		printk(PFX "failed to create class osdblk\n");
		return ret;
	}

	return 0;
}

static void osdblk_sysfs_cleanup(void)
{
	if (class_osdblk)
		class_destroy(class_osdblk);
	class_osdblk = NULL;
}

static int __init osdblk_init(void)
{
	int rc;

	rc = osdblk_sysfs_init();
	if (rc)
		return rc;

	return 0;
}

static void __exit osdblk_exit(void)
{
	osdblk_sysfs_cleanup();
}

module_init(osdblk_init);
module_exit(osdblk_exit);

                                                                                                                                                                                                                                                                                                                                                                                                                 #
# PARIDE configuration
#
# PARIDE doesn't need PARPORT, but if PARPORT is configured as a module,
# PARIDE must also be a module.
# PARIDE only supports PC style parports. Tough for USB or other parports...

comment "Parallel IDE high-level drivers"
	depends on PARIDE

config PARIDE_PD
	tristate "Parallel port IDE disks"
	depends on PARIDE
	help
	  This option enables the high-level driver for IDE-type disk devices
	  connected through a parallel port. If you chose to build PARIDE
	  support into your kernel, you may answer Y here to build in the
	  parallel port IDE driver, otherwise you should answer M to build
	  it as a loadable module. The module will be called pd. You
	  must also have at least one parallel port protocol driver in your
	  system. Among the devices supported by this driver are the SyQuest
	  EZ-135, EZ-230 and SparQ drives, the Avatar Shark and the backpack
	  hard drives from MicroSolutions.

config PARIDE_PCD
	tristate "Parallel port ATAPI CD-ROMs"
	depends on PARIDE
	---help---
	  This option enables the high-level driver for ATAPI CD-ROM devices
	  connected through a parallel port. If you chose to build PARIDE
	  support into your kernel, you may answer Y here to build in the
	  parallel port ATAPI CD-ROM driver, otherwise you should answer M to
	  build it as a loadable module. The module will be called pcd. You
	  must also have at least one parallel port protocol driver in your
	  system. Among the devices supported by this driver are the
	  MicroSolutions backpack CD-ROM drives and the Freecom Power CD. If
	  you have such a CD-ROM drive, you should also say Y or M to "ISO
	  9660 CD-ROM file system support" below, because that's the file
	  system used on CD-ROMs.

config PARIDE_PF
	tristate "Parallel port ATAPI disks"
	depends on PARIDE
	help
	  This option enables the high-level driver for ATAPI disk devices
	  connected through a parallel port. If you chose to build PARIDE
	  support into your kernel, you may answer Y here to build in the
	  parallel port ATAPI disk driver, otherwise you should answer M
	  to build it as a loadable module. The module will be called pf.
	  You must also have at least one parallel port protocol driver in
	  your system. Among the devices supported by this driver are the
	  MicroSolutions backpack PD/CD drive and the Imation Superdisk
	  LS-120 drive.

config PARIDE_PT
	tristate "Parallel port ATAPI tapes"
	depends on PARIDE
	help
	  This option enables the high-level driver for ATAPI tape devices
	  connected through a parallel port. If you chose to build PARIDE
	  support into your kernel, you may answer Y here to build in the
	  parallel port ATAPI disk driver, otherwise you should answer M
	  to build it as a loadable module. The module will be called pt.
	  You must also have at least one parallel port protocol driver in
	  your system. Among the devices supported by this driver is the
	  parallel port version of the HP 5GB drive.

config PARIDE_PG
	tristate "Parallel port generic ATAPI devices"
	depends on PARIDE
	---help---
	  This option enables a special high-level driver for generic ATAPI
	  devices connected through a parallel port. The driver allows user
	  programs, such as cdrtools, to send ATAPI commands directly to a
	  device.

	  If you chose to build PARIDE support into your kernel, you may
	  answer Y here to build in the parallel port generic ATAPI driver,
	  otherwise you should answer M to build it as a loadable module. The
	  module will be called pg.

	  You must also have at least one parallel port protocol driver in
	  your system.

	  This driver implements an API loosely related to the generic SCSI
	  driver. See <file:include/linux/pg.h>. for details.

	  You can obtain the most recent version of cdrtools from
	  <ftp://ftp.berlios.de/pub/cdrecord/>. Versions 1.6.1a3 and
	  later fully support this driver.

comment "Parallel IDE protocol modules"
	depends on PARIDE

config PARIDE_ATEN
	tristate "ATEN EH-100 protocol"
	depends on PARIDE
	help
	  This option enables support for the ATEN EH-100 parallel port IDE
	  protocol. This protocol is used in some inexpensive low performance
	  parallel port kits made in Hong Kong. If you chose to build PARIDE
	  support into your kernel, you may answer Y here to build in the
	  protocol driver, otherwise you should answer M to build it as a
	  loadable module. The module will be called aten. You must also
	  have a high-level driver for the type of device that you want to
	  support.

config PARIDE_BPCK
	tristate "MicroSolutions backpack (Series 5) protocol"
	depends on PARIDE
	---help---
	  This option enables support for the Micro Solutions BACKPACK
	  parallel port Series 5 IDE protocol.  (Most BACKPACK drives made
	  before 1999 were Series 5) Series 5 drives will NOT always have the
	  Series noted on the bottom of the drive. Series 6 drivers will.

	  In other words, if your BACKPACK drive doesn't say "Series 6" on the
	  bottom, enable this option.

	  If you chose to build PARIDE support into your kernel, you may
	  answer Y here to build in the protocol driver, otherwise you should
	  answer M to build it as a loadable module.  The module will be
	  called bpck.  You must also have a high-level driver for the type
	  of device that you want to support.

config PARIDE_BPCK6
	tristate "MicroSolutions backpack (Series 6) protocol"
	depends on PARIDE && !64BIT
	---help---
	  This option enables support for the Micro Solutions BACKPACK
	  parallel port Series 6 IDE protocol.  (Most BACKPACK drives made
	  after 1999 were Series 6) Series 6 drives will have the Series noted
	  on the bottom of the drive.  Series 5 drivers don't always have it
	  noted.

	  In other words, if your BACKPACK drive says "Series 6" on the
	  bottom, enable this option.

	  If you chose to build PARIDE support into your kernel, you may
	  answer Y here to build in the protocol driver, otherwise you should
	  answer M to build it as a loadable module.  The module will be
	  called bpck6.  You must also have a high-level driver for the type
	  of device that you want to support.

config PARIDE_COMM
	tristate "DataStor Commuter protocol"
	depends on PARIDE
	help
	  This option enables support for the Commuter parallel port IDE
	  protocol from DataStor. If you chose to build PARIDE support
	  into your kernel, you may answer Y here to build in the protocol
	  driver, otherwise you should answer M to build it as a loadable
	  module. The module will be called comm. You must also have
	  a high-level driver for the type of device that you want to support.

config PARIDE_DSTR
	tristate "DataStor EP-2000 protocol"
	depends on PARIDE
	help
	  This option enables support for the EP-2000 parallel port IDE
	  protocol from DataStor. If you chose to build PARIDE support
	  into your kernel, you may answer Y here to build in the protocol
	  driver, otherwise you should answer M to build it as a loadable
	  module. The module will be called dstr. You must also have
	  a high-level driver for the type of device that you want to support.

config PARIDE_FIT2
	tristate "FIT TD-2000 protocol"
	depends on PARIDE
	help
	  This option enables support for the TD-2000 parallel port IDE
	  protocol from Fidelity International Technology. This is a simple
	  (low speed) adapter that is used in some portable hard drives. If
	  you chose to build PARIDE support into your kernel, you may answer Y
	  here to build in the protocol driver, otherwise you should answer M
	  to build it as a loadable module. The module will be called ktti.
	  You must also have a high-level driver for the type of device that
	  you want to support.

config PARIDE_FIT3
	tristate "FIT TD-3000 protocol"
	depends on PARIDE
	help
	  This option enables support for the TD-3000 parallel port IDE
	  protocol from Fidelity International Technology. This protocol is
	  used in newer models of their portable disk, CD-ROM and PD/CD
	  devices. If you chose to build PARIDE support into your kernel, you
	  may answer Y here to build in the protocol driver, otherwise you
	  should answer M to build it as a loadable module. The module will be
	  called fit3. You must also have a high-level driver for the type
	  of device that you want to support.

config PARIDE_EPAT
	tristate "Shuttle EPAT/EPEZ protocol"
	depends on PARIDE
	help
	  This option enables support for the EPAT parallel port IDE protocol.
	  EPAT is a parallel port IDE adapter manufactured by Shuttle
	  Technology and widely used in devices from major vendors such as
	  Hewlett-Packard, SyQuest, Imation and Avatar. If you chose to build
	  PARIDE support into your kernel, you may answer Y here to build in
	  the protocol driver, otherwise you should answer M to build it as a
	  loadable module. The module will be called epat. You must also
	  have a high-level driver for the type of device that you want to
	  support.

config PARIDE_EPATC8
	bool "Support c7/c8 chips"
	depends on PARIDE_EPAT
	help
	  This option enables support for the newer Shuttle EP1284 (aka c7 and
	  c8) chip. You need this if you are using any recent Imation SuperDisk
	  (LS-120) drive.

config PARIDE_EPIA
	tristate "Shuttle EPIA protocol"
	depends on PARIDE
	help
	  This option enables support for the (obsolete) EPIA parallel port
	  IDE protocol from Shuttle Technology. This adapter can still be
	  found in some no-name kits. If you chose to build PARIDE support
	  into your kernel, you may answer Y here to build in the protocol
	  driver, otherwise you should answer M to build it as a loadable
	  module. The module will be called epia. You must also have a
	  high-level driver for the type of device that you want to support.

config PARIDE_FRIQ
	tristate "Freecom IQ ASIC-2 protocol"
	depends on PARIDE
	help
	  This option enables support for version 2 of the Freecom IQ parallel
	  port IDE adapter.  This adapter is used by the Maxell Superdisk
	  drive.  If you chose to build PARIDE support into your kernel, you
	  may answer Y here to build in the protocol driver, otherwise you
	  should answer M to build it as a loadable module. The module will be
	  called friq. You must also have a high-level driver for the type
	  of device that you want to support.

config PARIDE_FRPW
	tristate "FreeCom power protocol"
	depends on PARIDE
	help
	  This option enables support for the Freecom power parallel port IDE
	  protocol. If you chose to build PARIDE support into your kernel, you
	  may answer Y here to build in the protocol driver, otherwise you
	  should answer M to build it as a loadable module. The module will be
	  called frpw. You must also have a high-level driver for the type
	  of device that you want to support.

config PARIDE_KBIC
	tristate "KingByte KBIC-951A/971A protocols"
	depends on PARIDE
	help
	  This option enables support for the KBIC-951A and KBIC-971A parallel
	  port IDE protocols from KingByte Information Corp. KingByte's
	  adapters appear in many no-name portable disk and CD-ROM products,
	  especially in Europe. If you chose to build PARIDE support into your
	  kernel, you may answer Y here to build in the protocol driver,
	  otherwise you should answer M to build it as a loadable module. The
	  module will be called kbic. You must also have a high-level driver
	  for the type of device that you want to support.

config PARIDE_KTTI
	tristate "KT PHd protocol"
	depends on PARIDE
	help
	  This option enables support for the "PHd" parallel port IDE protocol
	  from KT Technology. This is a simple (low speed) adapter that is
	  used in some 2.5" portable hard drives. If you chose to build PARIDE
	  support into your kernel, you may answer Y here to build in the
	  protocol driver, otherwise you should answer M to build it as a
	  loadable module. The module will be called ktti. You must also
	  have a high-level driver for the type of device that you want to
	  support.

config PARIDE_ON20
	tristate "OnSpec 90c20 protocol"
	depends on PARIDE
	help
	  This option enables support for the (obsolete) 90c20 parallel port
	  IDE protocol from OnSpec (often marketed under the ValuStore brand
	  name). If you chose to build PARIDE support into your kernel, you
	  may answer Y here to build in the protocol driver, otherwise you
	  should answer M to build it as a loadable module. The module will
	  be called on20. You must also have a high-level driver for the
	  type of device that you want to support.

config PARIDE_ON26
	tristate "OnSpec 90c26 protocol"
	depends on PARIDE
	help
	  This option enables support for the 90c26 parallel port IDE protocol
	  from OnSpec Electronics (often marketed under the ValuStore brand
	  name). If you chose to build PARIDE support into your kernel, you
	  may answer Y here to build in the protocol driver, otherwise you
	  should answer M to build it as a loadable module. The module will be
	  called on26. You must also have a high-level driver for the type
	  of device that you want to support.

#
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #
# Makefile for Parallel port IDE device drivers.
#
# 7 October 2000, Bartlomiej Zolnierkiewicz <bkz@linux-ide.org>
# Rewritten to use lists instead of if-statements.
#

obj-$(CONFIG_PARIDE)		+= paride.o
obj-$(CONFIG_PARIDE_ATEN)	+= aten.o
obj-$(CONFIG_PARIDE_BPCK)	+= bpck.o
obj-$(CONFIG_PARIDE_COMM)	+= comm.o
obj-$(CONFIG_PARIDE_DSTR)	+= dstr.o
obj-$(CONFIG_PARIDE_KBIC)	+= kbic.o
obj-$(CONFIG_PARIDE_EPAT)	+= epat.o
obj-$(CONFIG_PARIDE_EPIA)	+= epia.o
obj-$(CONFIG_PARIDE_FRPW)	+= frpw.o
obj-$(CONFIG_PARIDE_FRIQ)	+= friq.o
obj-$(CONFIG_PARIDE_FIT2)	+= fit2.o
obj-$(CONFIG_PARIDE_FIT3)	+= fit3.o
obj-$(CONFIG_PARIDE_ON20)	+= on20.o
obj-$(CONFIG_PARIDE_ON26)	+= on26.o
obj-$(CONFIG_PARIDE_KTTI)	+= ktti.o
obj-$(CONFIG_PARIDE_BPCK6)	+= bpck6.o
obj-$(CONFIG_PARIDE_PD)		+= pd.o
obj-$(CONFIG_PARIDE_PCD)	+= pcd.o
obj-$(CONFIG_PARIDE_PF)		+= pf.o
obj-$(CONFIG_PARIDE_PT)		+= pt.o
obj-$(CONFIG_PARIDE_PG)		+= pg.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               Lemma 1:
	If ps_tq is scheduled, ps_tq_ac