eout));

	parport_negotiate (lp_table[minor].dev->port, IEEE1284_MODE_COMPAT);
	if (parport_negotiate (lp_table[minor].dev->port,
			       IEEE1284_MODE_NIBBLE)) {
		retval = -EIO;
		goto out;
	}

	while (retval == 0) {
		retval = parport_read (port, kbuf, count);

		if (retval > 0)
			break;

		if (nonblock) {
			retval = -EAGAIN;
			break;
		}

		/* Wait for data. */

		if (lp_table[minor].dev->port->irq == PARPORT_IRQ_NONE) {
			parport_negotiate (lp_table[minor].dev->port,
					   IEEE1284_MODE_COMPAT);
			lp_error (minor);
			if (parport_negotiate (lp_table[minor].dev->port,
					       IEEE1284_MODE_NIBBLE)) {
				retval = -EIO;
				goto out;
			}
		} else {
			prepare_to_wait(&lp_table[minor].waitq, &wait, TASK_INTERRUPTIBLE);
			schedule_timeout(LP_TIMEOUT_POLLED);
			finish_wait(&lp_table[minor].waitq, &wait);
		}

		if (signal_pending (current)) {
			retval = -ERESTARTSYS;
			break;
		}

		cond_resched ();
	}
	parport_negotiate (lp_table[minor].dev->port, IEEE1284_MODE_COMPAT);
 out:
	lp_release_parport (&lp_table[minor]);

	if (retval > 0 && copy_to_user (buf, kbuf, retval))
		retval = -EFAULT;

	mutex_unlock(&lp_table[minor].port_mutex);

	return retval;
}

#endif /* IEEE 1284 support */

static int lp_open(struct inode * inode, struct file * file)
{
	unsigned int minor = iminor(inode);
	int ret = 0;

	mutex_lock(&lp_mutex);
	if (minor >= LP_NO) {
		ret = -ENXIO;
		goto out;
	}
	if ((LP_F(minor) & LP_EXIST) == 0) {
		ret = -ENXIO;
		goto out;
	}
	if (test_and_set_bit(LP_BUSY_BIT_POS, &LP_F(minor))) {
		ret = -EBUSY;
		goto out;
	}
	/* If ABORTOPEN is set and the printer is offline or out of paper,
	   we may still want to open it to perform ioctl()s.  Therefore we
	   have commandeered O_NONBLOCK, even though it is being used in
	   a non-standard manner.  This is strictly a Linux hack, and
	   should most likely only ever be used by the tunelp application. */
	if ((LP_F(minor) & LP_ABORTOPEN) && !(file->f_flags & O_NONBLOCK)) {
		int status;
		lp_claim_parport_or_block (&lp_table[minor]);
		status = r_str(minor);
		lp_release_parport (&lp_table[minor]);
		if (status & LP_POUTPA) {
			printk(KERN_INFO "lp%d out of paper\n", minor);
			LP_F(minor) &= ~LP_BUSY;
			ret = -ENOSPC;
			goto out;
		} else if (!(status & LP_PSELECD)) {
			printk(KERN_INFO "lp%d off-line\n", minor);
			LP_F(minor) &= ~LP_BUSY;
			ret = -EIO;
			goto out;
		} else if (!(status & LP_PERRORP)) {
			printk(KERN_ERR "lp%d printer error\n", minor);
			LP_F(minor) &= ~LP_BUSY;
			ret = -EIO;
			goto out;
		}
	}
	lp_table[minor].lp_buffer = kmalloc(LP_BUFFER_SIZE, GFP_KERNEL);
	if (!lp_table[minor].lp_buffer) {
		LP_F(minor) &= ~LP_BUSY;
		ret = -ENOMEM;
		goto out;
	}
	/* Determine if the peripheral supports ECP mode */
	lp_claim_parport_or_block (&lp_table[minor]);
	if ( (lp_table[minor].dev->port->modes & PARPORT_MODE_ECP) &&
             !parport_negotiate (lp_table[minor].dev->port, 
                                 IEEE1284_MODE_ECP)) {
		printk (KERN_INFO "lp%d: ECP mode\n", minor);
		lp_table[minor].best_mode = IEEE1284_MODE_ECP;
	} else {
		lp_table[minor].best_mode = IEEE1284_MODE_COMPAT;
	}
	/* Leave peripheral in compatibility mode */
	parport_negotiate (lp_table[minor].dev->port, IEEE1284_MODE_COMPAT);
	lp_release_parport (&lp_table[minor]);
	lp_table[minor].current_mode = IEEE1284_MODE_COMPAT;
out:
	mutex_unlock(&lp_mutex);
	return ret;
}

static int lp_release(struct inode * inode, struct file * file)
{
	unsigned int minor = iminor(inode);

	lp_claim_parport_or_block (&lp_table[minor]);
	parport_negotiate (lp_table[minor].dev->port, IEEE1284_MODE_COMPAT);
	lp_table[minor].current_mode = IEEE1284_MODE_COMPAT;
	lp_release_parport (&lp_table[minor]);
	kfree(lp_table[minor].lp_buffer);
	lp_table[minor].lp_buffer = NULL;
	LP_F(minor) &= ~LP_BUSY;
	return 0;
}

static int lp_do_ioctl(unsigned int minor, unsigned int cmd,
	unsigned long arg, void __user *argp)
{
	int status;
	int retval = 0;

#ifdef LP_DEBUG
	printk(KERN_DEBUG "lp%d ioctl, cmd: 0x%x, arg: 0x%lx\n", minor, cmd, arg);
#endif
	if (minor >= LP_NO)
		return -ENODEV;
	if ((LP_F(minor) & LP_EXIST) == 0)
		return -ENODEV;
	switch ( cmd ) {
		case LPTIME:
			if (arg > UINT_MAX / HZ)
				return -EINVAL;
			LP_TIME(minor) = arg * HZ/100;
			break;
		case LPCHAR:
			LP_CHAR(minor) = arg;
			break;
		case LPABORT:
			if (arg)
				LP_F(minor) |= LP_ABORT;
			else
				LP_F(minor) &= ~LP_ABORT;
			break;
		case LPABORTOPEN:
			if (arg)
				LP_F(minor) |= LP_ABORTOPEN;
			else
				LP_F(minor) &= ~LP_ABORTOPEN;
			break;
		case LPCAREFUL:
			if (arg)
				LP_F(minor) |= LP_CAREFUL;
			else
				LP_F(minor) &= ~LP_CAREFUL;
			break;
		case LPWAIT:
			LP_WAIT(minor) = arg;
			break;
		case LPSETIRQ: 
			return -EINVAL;
			break;
		case LPGETIRQ:
			if (copy_to_user(argp, &LP_IRQ(minor),
					sizeof(int)))
				return -EFAULT;
			break;
		case LPGETSTATUS:
			if (mutex_lock_interruptible(&lp_table[minor].port_mutex))
				return -EINTR;
			lp_claim_parport_or_block (&lp_table[minor]);
			status = r_str(minor);
			lp_release_parport (&lp_table[minor]);
			mutex_unlock(&lp_table[minor].port_mutex);

			if (copy_to_user(argp, &status, sizeof(int)))
				return -EFAULT;
			break;
		case LPRESET:
			lp_reset(minor);
			break;
#ifdef LP_STATS
		case LPGETSTATS:
			if (copy_to_user(argp, &LP_STAT(minor),
					sizeof(struct lp_stats)))
				return -EFAULT;
			if (capable(CAP_SYS_ADMIN))
				memset(&LP_STAT(minor), 0,
						sizeof(struct lp_stats));
			break;
#endif
 		case LPGETFLAGS:
 			status = LP_F(minor);
			if (copy_to_user(argp, &status, sizeof(int)))
				return -EFAULT;
			break;

		default:
			retval = -EINVAL;
	}
	return retval;
}

static int lp_set_timeout(unsigned int minor, struct timeval *par_timeout)
{
	long to_jiffies;

	/* Convert to jiffies, place in lp_table */
	if ((par_timeout->tv_sec < 0) ||
	    (par_timeout->tv_usec < 0)) {
		return -EINVAL;
	}
	to_jiffies = DIV_ROUND_UP(par_timeout->tv_usec, 1000000/HZ);
	to_jiffies += par_timeout->tv_sec * (long) HZ;
	if (to_jiffies <= 0) {
		return -EINVAL;
	}
	lp_table[minor].timeout = to_jiffies;
	return 0;
}

static long lp_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	unsigned int minor;
	struct timeval par_timeout;
	int ret;

	minor = iminor(file_inode(file));
	mutex_lock(&lp_mutex);
	switch (cmd) {
	case LPSETTIMEOUT:
		if (copy_from_user(&par_timeout, (void __user *)arg,
					sizeof (struct timeval))) {
			ret = -EFAULT;
			break;
		}
		ret = lp_set_timeout(minor, &par_timeout);
		break;
	default:
		ret = lp_do_ioctl(minor, cmd, arg, (void __user *)arg);
		break;
	}
	mutex_unlock(&lp_mutex);

	return ret;
}

#ifdef CONFIG_COMPAT
static long lp_compat_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	unsigned int minor;
	struct timeval par_timeout;
	int ret;

	minor = iminor(file_inode(file));
	mutex_lock(&lp_mutex);
	switch (cmd) {
	case LPSETTIMEOUT:
		if (compat_get_timeval(&par_timeout, compat_ptr(arg))) {
			ret = -EFAULT;
			break;
		}
		ret = lp_set_timeout(minor, &par_timeout);
		break;
#ifdef LP_STATS
	case LPGETSTATS:
		/* FIXME: add an implementation if you set LP_STATS */
		ret = -EINVAL;
		break;
#endif
	default:
		ret = lp_do_ioctl(minor, cmd, arg, compat_ptr(arg));
		break;
	}
	mutex_unlock(&lp_mutex);

	return ret;
}
#endif

static const struct file_operations lp_fops = {
	.owner		= THIS_MODULE,
	.write		= lp_write,
	.unlocked_ioctl	= lp_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= lp_compat_ioctl,
#endif
	.open		= lp_open,
	.release	= lp_release,
#ifdef CONFIG_PARPORT_1284
	.read		= lp_read,
#endif
	.llseek		= noop_llseek,
};

/* --- support for console on the line printer ----------------- */

#ifdef CONFIG_LP_CONSOLE

#define CONSOLE_LP 0

/* If the printer is out of paper, we can either lose the messages or
 * stall until the printer is happy again.  Define CONSOLE_LP_STRICT
 * non-zero to get the latter behaviour. */
#define CONSOLE_LP_STRICT 1

/* The console must be locked when we get here. */

static void lp_console_write (struct console *co, const char *s,
			      unsigned count)
{
	struct pardevice *dev = lp_table[CONSOLE_LP].dev;
	struct parport *port = dev->port;
	ssize_t written;

	if (parport_claim (dev))
		/* Nothing we can do. */
		return;

	parport_set_timeout (dev, 0);

	/* Go to compatibility mode. */
	parport_negotiate (port, IEEE1284_MODE_COMPAT);

	do {
		/* Write the data, converting LF->CRLF as we go. */
		ssize_t canwrite = count;
		char *lf = memchr (s, '\n', count);
		if (lf)
			canwrite = lf - s;

		if (canwrite > 0) {
			written = parport_write (port, s, canwrite);

			if (written <= 0)
				continue;

			s += written;
			count -= written;
			canwrite -= written;
		}

		if (lf && canwrite <= 0) {
			const char *crlf = "\r\n";
			int i = 2;

			/* Dodge the original '\n', and put '\r\n' instead. */
			s++;
			count--;
			do {
				written = parport_write (port, crlf, i);
				if (written > 0)
					i -= written, crlf += written;
			} while (i > 0 && (CONSOLE_LP_STRICT || written > 0));
		}
	} while (count > 0 && (CONSOLE_LP_STRICT || written > 0));

	parport_release (dev);
}

static struct console lpcons = {
	.name		= "lp",
	.write		= lp_console_write,
	.flags		= CON_PRINTBUFFER,
};

#endif /* console on line printer */

/* --- initialisation code ------------------------------------- */

static int parport_nr[LP_NO] = { [0 ... LP_NO-1] = LP_PARPORT_UNSPEC };
static char *parport[LP_NO];
static bool reset;

module_param_array(parport, charp, NULL, 0);
module_param(reset, bool, 0);

#ifndef MODULE
static int __init lp_setup (char *str)
{
	static int parport_ptr;
	int x;

	if (get_option(&str, &x)) {
		if (x == 0) {
			/* disable driver on "lp=" or "lp=0" */
			parport_nr[0] = LP_PARPORT_OFF;
		} else {
			printk(KERN_WARNING "warning: 'lp=0x%x' is deprecated, ignored\n", x);
			return 0;
		}
	} else if (!strncmp(str, "parport", 7)) {
		int n = simple_strtoul(str+7, NULL, 10);
		if (parport_ptr < LP_NO)
			parport_nr[parport_ptr++] = n;
		else
			printk(KERN_INFO "lp: too many ports, %s ignored.\n",
			       str);
	} else if (!strcmp(str, "auto")) {
		parport_nr[0] = LP_PARPORT_AUTO;
	} else if (!strcmp(str, "none")) {
		if (parport_ptr < LP_NO)
			parport_nr[parport_ptr++] = LP_PARPORT_NONE;
		else
			printk(KERN_INFO "lp: too many ports, %s ignored.\n",
			       str);
	} else if (!strcmp(str, "reset")) {
		reset = 1;
	}
	return 1;
}
#endif

static int lp_register(int nr, struct parport *port)
{
	lp_table[nr].dev = parport_register_device(port, "lp", 
						   lp_preempt, NULL, NULL, 0,
						   (void *) &lp_table[nr]);
	if (lp_table[nr].dev == NULL)
		return 1;
	lp_table[nr].flags |= LP_EXIST;

	if (reset)
		lp_reset(nr);

	device_create(lp_class, port->dev, MKDEV(LP_MAJOR, nr), NULL,
		      "lp%d", nr);

	printk(KERN_INFO "lp%d: using %s (%s).\n", nr, port->name, 
	       (port->irq == PARPORT_IRQ_NONE)?"polling":"interrupt-driven");

#ifdef CONFIG_LP_CONSOLE
	if (!nr) {
		if (port->modes & PARPORT_MODE_SAFEININT) {
			register_console(&lpcons);
			console_registered = port;
			printk (KERN_INFO "lp%d: console ready\n", CONSOLE_LP);
		} else
			printk (KERN_ERR "lp%d: cannot run console on %s\n",
				CONSOLE_LP, port->name);
	}
#endif

	return 0;
}

static void lp_attach (struct parport *port)
{
	unsigned int i;

	switch (parport_nr[0]) {
	case LP_PARPORT_UNSPEC:
	case LP_PARPORT_AUTO:
		if (parport_nr[0] == LP_PARPORT_AUTO &&
		    port->probe_info[0].class != PARPORT_CLASS_PRINTER)
			return;
		if (lp_count == LP_NO) {
			printk(KERN_INFO "lp: ignoring parallel port (max. %d)\n",LP_NO);
			return;
		}
		if (!lp_register(lp_count, port))
			lp_count++;
		break;

	default:
		for (i = 0; i < LP_NO; i++) {
			if (port->number == parport_nr[i]) {
				if (!lp_register(i, port))
					lp_count++;
				break;
			}
		}
		break;
	}
}

static void lp_detach (struct parport *port)
{
	/* Write this some day. */
#ifdef CONFIG_LP_CONSOLE
	if (console_registered == port) {
		unregister_console(&lpcons);
		console_registered = NULL;
	}
#endif /* CONFIG_LP_CONSOLE */
}

static struct parport_driver lp_driver = {
	.name = "lp",
	.attach = lp_attach,
	.detach = lp_detach,
};

static int __init lp_init (void)
{
	int i, err = 0;

	if (parport_nr[0] == LP_PARPORT_OFF)
		return 0;

	for (i = 0; i < LP_NO; i++) {
		lp_table[i].dev = NULL;
		lp_table[i].flags = 0;
		lp_table[i].chars = LP_INIT_CHAR;
		lp_table[i].time = LP_INIT_TIME;
		lp_table[i].wait = LP_INIT_WAIT;
		lp_table[i].lp_buffer = NULL;
#ifdef LP_STATS
		lp_table[i].lastcall = 0;
		lp_table[i].runchars = 0;
		memset (&lp_table[i].stats, 0, sizeof (struct lp_stats));
#endif
		lp_table[i].last_error = 0;
		init_waitqueue_head (&lp_table[i].waitq);
		init_waitqueue_head (&lp_table[i].dataq);
		mutex_init(&lp_table[i].port_mutex);
		lp_table[i].timeout = 10 * HZ;
	}

	if (register_chrdev (LP_MAJOR, "lp", &lp_fops)) {
		printk (KERN_ERR "lp: unable to get major %d\n", LP_MAJOR);
		return -EIO;
	}

	lp_class = class_create(THIS_MODULE, "printer");
	if (IS_ERR(lp_class)) {
		err = PTR_ERR(lp_class);
		goto out_reg;
	}

	if (parport_register_driver (&lp_driver)) {
		printk (KERN_ERR "lp: unable to register with parport\n");
		err = -EIO;
		goto out_class;
	}

	if (!lp_count) {
		printk (KERN_INFO "lp: driver loaded but no devices found\n");
#ifndef CONFIG_PARPORT_1284
		if (parport_nr[0] == LP_PARPORT_AUTO)
			printk (KERN_INFO "lp: (is IEEE 1284 support enabled?)\n");
#endif
	}

	return 0;

out_class:
	class_destroy(lp_class);
out_reg:
	unregister_chrdev(LP_MAJOR, "lp");
	return err;
}

static int __init lp_init_module (void)
{
	if (parport[0]) {
		/* The user gave some parameters.  Let's see what they were.  */
		if (!strncmp(parport[0], "auto", 4))
			parport_nr[0] = LP_PARPORT_AUTO;
		else {
			int n;
			for (n = 0; n < LP_NO && parport[n]; n++) {
				if (!strncmp(parport[n], "none", 4))
					parport_nr[n] = LP_PARPORT_NONE;
				else {
					char *ep;
					unsigned long r = simple_strtoul(parport[n], &ep, 0);
					if (ep != parport[n]) 
						parport_nr[n] = r;
					else {
						printk(KERN_ERR "lp: bad port specifier `%s'\n", parport[n]);
						return -ENODEV;
					}
				}
			}
		}
	}

	return lp_init();
}

static void lp_cleanup_module (void)
{
	unsigned int offset;

	parport_unregister_driver (&lp_driver);

#ifdef CONFIG_LP_CONSOLE
	unregister_console (&lpcons);
#endif

	unregister_chrdev(LP_MAJOR, "lp");
	for (offset = 0; offset < LP_NO; offset++) {
		if (lp_table[offset].dev == NULL)
			continue;
		parport_unregister_device(lp_table[offset].dev);
		device_destroy(lp_class, MKDEV(LP_MAJOR, offset));
	}
	class_destroy(lp_class);
}

__setup("lp=", lp_setup);
module_init(lp_init_module);
module_exit(lp_cleanup_module);

MODULE_ALIAS_CHARDEV_MAJOR(LP_MAJOR);
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2005 Silicon Graphics, Inc.  All rights reserved.
 */

/*
 *	MOATB Core Services driver.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/uio.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/sn/addrs.h>
#include <asm/sn/intr.h>
#include <asm/sn/tiocx.h>
#include "mbcs.h"

#define MBCS_DEBUG 0
#if MBCS_DEBUG
#define DBG(fmt...)    printk(KERN_ALERT fmt)
#else
#define DBG(fmt...)
#endif
static DEFINE_MUTEX(mbcs_mutex);
static int mbcs_major;

static LIST_HEAD(soft_list);

/*
 * file operations
 */
static const struct file_operations mbcs_ops = {
	.open = mbcs_open,
	.llseek = mbcs_sram_llseek,
	.read = mbcs_sram_read,
	.write = mbcs_sram_write,
	.mmap = mbcs_gscr_mmap,
};

struct mbcs_callback_arg {
	int minor;
	struct cx_dev *cx_dev;
};

static inline void mbcs_getdma_init(struct getdma *gdma)
{
	memset(gdma, 0, sizeof(struct getdma));
	gdma->DoneIntEnable = 1;
}

static inline void mbcs_putdma_init(struct putdma *pdma)
{
	memset(pdma, 0, sizeof(struct putdma));
	pdma->DoneIntEnable = 1;
}

static inline void mbcs_algo_init(struct algoblock *algo_soft)
{
	memset(algo_soft, 0, sizeof(struct algoblock));
}

static inline void mbcs_getdma_set(void *mmr,
		       uint64_t hostAddr,
		       uint64_t localAddr,
		       uint64_t localRamSel,
		       uint64_t numPkts,
		       uint64_t amoEnable,
		       uint64_t intrEnable,
		       uint64_t peerIO,
		       uint64_t amoHostDest,
		       uint64_t amoModType, uint64_t intrHostDest,
		       uint64_t intrVector)
{
	union dma_control rdma_control;
	union dma_amo_dest amo_dest;
	union intr_dest intr_dest;
	union dma_localaddr local_addr;
	union dma_hostaddr host_addr;

	rdma_control.dma_control_reg = 0;
	amo_dest.dma_amo_dest_reg = 0;
	intr_dest.intr_dest_reg = 0;
	local_addr.dma_localaddr_reg = 0;
	host_addr.dma_hostaddr_reg = 0;

	host_addr.dma_sys_addr = hostAddr;
	MBCS_MMR_SET(mmr, MBCS_RD_DMA_SYS_ADDR, host_addr.dma_hostaddr_reg);

	local_addr.dma_ram_addr = localAddr;
	local_addr.dma_ram_sel = localRamSel;
	MBCS_MMR_SET(mmr, MBCS_RD_DMA_LOC_ADDR, local_addr.dma_localaddr_reg);

	rdma_control.dma_op_length = numPkts;
	rdma_control.done_amo_en = amoEnable;
	rdma_control.done_int_en = intrEnable;
	rdma_control.pio_mem_n = peerIO;
	MBCS_MMR_SET(mmr, MBCS_RD_DMA_CTRL, rdma_control.dma_control_reg);

	amo_dest.dma_amo_sys_addr = amoHostDest;
	amo_dest.dma_amo_mod_type = amoModType;
	MBCS_MMR_SET(mmr, MBCS_RD_DMA_AMO_DEST, amo_dest.dma_amo_dest_reg);

	intr_dest.address = intrHostDest;
	intr_dest.int_vector = intrVector;
	MBCS_MMR_SET(mmr, MBCS_RD_DMA_INT_DEST, intr_dest.intr_dest_reg);

}

static inline void mbcs_putdma_set(void *mmr,
		       uint64_t hostAddr,
		       uint64_t localAddr,
		       uint64_t localRamSel,
		       uint64_t numPkts,
		       uint64_t amoEnable,
		       uint64_t intrEnable,
		       uint64_t peerIO,
		       uint64_t amoHostDest,
		       uint64_t amoModType,
		       uint64_t intrHostDest, uint64_t intrVector)
{
	union dma_control wdma_control;
	union dma_amo_dest amo_dest;
	union intr_dest intr_dest;
	union dma_localaddr local_addr;
	union dma_hostaddr host_addr;

	wdma_control.dma_control_reg = 0;
	amo_dest.dma_amo_dest_reg = 0;
	intr_dest.intr_dest_reg = 0;
	local_addr.dma_localaddr_reg = 0;
	host_addr.dma_hostaddr_reg = 0;

	host_addr.dma_sys_addr = hostAddr;
	MBCS_MMR_SET(mmr, MBCS_WR_DMA_SYS_ADDR, host_addr.dma_hostaddr_reg);

	local_addr.dma_ram_addr = localAddr;
	local_addr.dma_ram_sel = localRamSel;
	MBCS_MMR_SET(mmr, MBCS_WR_DMA_LOC_ADDR, local_addr.dma_localaddr_reg);

	wdma_control.dma_op_length = numPkts;
	wdma_control.done_amo_en = amoEnable;
	wdma_control.done_int_en = intrEnable;
	wdma_control.pio_mem_n = peerIO;
	MBCS_MMR_SET(mmr, MBCS_WR_DMA_CTRL, wdma_control.dma_control_reg);

	amo_dest.dma_amo_sys_addr = amoHostDest;
	amo_dest.dma_amo_mod_type = amoModType;
	MBCS_MMR_SET(mmr, MBCS_WR_DMA_AMO_DEST, amo_dest.dma_amo_dest_reg);

	intr_dest.address = intrHostDest;
	intr_dest.int_vector = intrVector;
	MBCS_MMR_SET(mmr, MBCS_WR_DMA_INT_DEST, intr_dest.intr_dest_reg);

}

static inline void mbcs_algo_set(void *mmr,
		     uint64_t amoHostDest,
		     uint64_t amoModType,
		     uint64_t intrHostDest,
		     uint64_t intrVector, uint64_t algoStepCount)
{
	union dma_amo_dest amo_dest;
	union intr_dest intr_dest;
	union algo_step step;

	step.algo_step_reg = 0;
	intr_dest.intr_dest_reg = 0;
	amo_dest.dma_amo_dest_reg = 0;

	amo_dest.dma_amo_sys_addr = amoHostDest;
	amo_dest.dma_amo_mod_type = amoModType;
	MBCS_MMR_SET(mmr, MBCS_ALG_AMO_DEST, amo_dest.dma_amo_dest_reg);

	intr_dest.address = intrHostDest;
	intr_dest.int_vector = intrVector;
	MBCS_MMR_SET(mmr, MBCS_ALG_INT_DEST, intr_dest.intr_dest_reg);

	step.alg_step_cnt = algoStepCount;
	MBCS_MMR_SET(mmr, MBCS_ALG_STEP, step.algo_step_reg);
}

static inline int mbcs_getdma_start(struct mbcs_soft *soft)
{
	void *mmr_base;
	struct getdma *gdma;
	uint64_t numPkts;
	union cm_control cm_control;

	mmr_base = soft->mmr_base;
	gdma = &soft->getdma;

	/* check that host address got setup */
	if (!gdma->hostAddr)
		return -1;

	numPkts =
	    (gdma->bytes + (MBCS_CACHELINE_SIZE - 1)) / MBCS_CACHELINE_SIZE;

	/* program engine */
	mbcs_getdma_set(mmr_base, tiocx_dma_addr(gdma->hostAddr),
		   gdma->localAddr,
		   (gdma->localAddr < MB2) ? 0 :
		   (gdma->localAddr < MB4) ? 1 :
		   (gdma->localAddr < MB6) ? 2 : 3,
		   numPkts,
		   gdma->DoneAmoEnable,
		   gdma->DoneIntEnable,
		   gdma->peerIO,
		   gdma->amoHostDest,
		   gdma->amoModType,
		   gdma->intrHostDest, gdma->intrVector);

	/* start engine */
	cm_control.cm_control_reg = MBCS_MMR_GET(mmr_base, MBCS_CM_CONTROL);
	cm_control.rd_dma_go = 1;
	MBCS_MMR_SET(mmr_base, MBCS_CM_CONTROL, cm_control.cm_control_reg);

	return 0;

}

static inline int mbcs_putdma_start(struct mbcs_soft *soft)
{
	void *mmr_base;
	struct putdma *pdma;
	uint64_t numPkts;
	union cm_control cm_control;

	mmr_base = soft->mmr_base;
	pdma = &soft->putdma;

	/* check that host address got setup */
	if (!pdma->hostAddr)
		return -1;

	numPkts =
	    (pdma->bytes + (MBCS_CACHELINE_SIZE - 1)) / MBCS_CACHELINE_SIZE;

	/* program engine */
	mbcs_putdma_set(mmr_base, tiocx_dma_addr(pdma->hostAddr),
		   pdma->localAddr,
		   (pdma->localAddr < MB2) ? 0 :
		   (pdma->localAddr < MB4) ? 1 :
		   (pdma->localAddr < MB6) ? 2 : 3,
		   numPkts,
		   pdma->DoneAmoEnable,
		   pdma->DoneIntEnable,
		   pdma->peerIO,
		   pdma->amoHostDest,
		   pdma->amoModType,
		   pdma->intrHostDest, pdma->intrVector);

	/* start engine */
	cm_control.cm_control_reg = MBCS_MMR_GET(mmr_base, MBCS_CM_CONTROL);
	cm_control.wr_dma_go = 1;
	MBCS_MMR_SET(mmr_base, MBCS_CM_CONTROL, cm_control.cm_control_reg);

	return 0;

}

static inline int mbcs_algo_start(struct mbcs_soft *soft)
{
	struct algoblock *algo_soft = &soft->algo;
	void *mmr_base = soft->mmr_base;
	union cm_control cm_control;

	if (mutex_lock_interruptible(&soft->algolock))
		return -ERESTARTSYS;

	atomic_set(&soft->algo_done, 0);

	mbcs_algo_set(mmr_base,
		 algo_soft->amoHostDest,
		 algo_soft->amoModType,
		 algo_soft->intrHostDest,
		 algo_soft->intrVector, algo_soft->algoStepCount);

	/* start algorithm */
	cm_control.cm_control_reg = MBCS_MMR_GET(mmr_base, MBCS_CM_CONTROL);
	cm_control.alg_done_int_en = 1;
	cm_control.alg_go = 1;
	MBCS_MMR_SET(mmr_base, MBCS_CM_CONTROL, cm_control.cm_control_reg);

	mutex_unlock(&soft->algolock);

	return 0;
}

static inline ssize_t
do_mbcs_sram_dmawrite(struct mbcs_soft *soft, uint64_t hostAddr,
		      size_t len, loff_t * off)
{
	int rv = 0;

	if (mutex_lock_interruptible(&soft->dmawritelock))
		return -ERESTARTSYS;

	atomic_set(&soft->dmawrite_done, 0);

	soft->putdma.hostAddr = hostAddr;
	soft->putdma.localAddr = *off;
	soft->putdma.bytes = len;

	if (mbcs_putdma_start(soft) < 0) {
		DBG(KERN_ALERT "do_mbcs_sram_dmawrite: "
					"mbcs_putdma_start failed\n");
		rv = -EAGAIN;
		goto dmawrite_exit;
	}

	if (wait_event_interruptible(soft->dmawrite_queue,
					atomic_read(&soft->dmawrite_done))) {
		rv = -ERESTARTSYS;
		goto dmawrite_exit;
	}

	rv = len;
	*off += len;

dmawrite_exit:
	mutex_unlock(&soft->dmawritelock);

	return rv;
}

static inline ssize_t
do_mbcs_sram_dmaread(struct mbcs_soft *soft, uint64_t hostAddr,
		     size_t len, loff_t * off)
{
	int rv = 0;

	if (mutex_lock_interruptible(&soft->dmareadlock))
		return -ERESTARTSYS;

	atomic_set(&soft->dmawrite_done, 0);

	soft->getdma.hostAddr = hostAddr;
	soft->getdma.localAddr = *off;
	soft->getdma.bytes = len;

	if (mbcs_getdma_start(soft) < 0) {
		DBG(KERN_ALERT "mbcs_strategy: mbcs_getdma_start failed\n");
		rv = -EAGAIN;
		goto dmaread_exit;
	}

	if (wait_event_interruptible(soft->dmaread_queue,
					atomic_read(&soft->dmaread_done))) {
		rv = -ERESTARTSYS;
		goto dmaread_exit;
	}

	rv = len;
	*off += len;

dmaread_exit:
	mutex_unlock(&soft->dmareadlock);

	return rv;
}

static int mbcs_open(struct inode *ip, struct file *fp)
{
	struct mbcs_soft *soft;
	int minor;

	mutex_lock(&mbcs_mutex);
	minor = iminor(ip);

	/* Nothing protects access to this list... */
	list_for_each_entry(soft, &soft_list, list) {
		if (soft->nasid == minor) {
			fp->private_data = soft->cxdev;
			mutex_unlock(&mbcs_mutex);
			return 0;
		}
	}

	mutex_unlock(&mbcs_mutex);
	return -ENODEV;
}

static ssize_t mbcs_sram_read(struct file * fp, char __user *buf, size_t len, loff_t * off)
{
	struct cx_dev *cx_dev = fp->private_data;
	struct mbcs_soft *soft = cx_dev->soft;
	uint64_t hostAddr;
	int rv = 0;

	hostAddr = __get_dma_pages(GFP_KERNEL, get_order(len));
	if (hostAddr == 0)
		return -ENOMEM;

	rv = do_mbcs_sram_dmawrite(soft, hostAddr, len, off);
	if (rv < 0)
		goto exit;

	if (copy_to_user(buf, (void *)hostAddr, len))
		rv = -EFAULT;

      exit:
	free_pages(hostAddr, get_order(len));

	return rv;
}

static ssize_t
mbcs_sram_write(struct file * fp, const char __user *buf, size_t len, loff_t * off)
{
	struct cx_dev *cx_dev = fp->private_data;
	struct mbcs_soft *soft = cx_dev->soft;
	uint64_t hostAddr;
	int rv = 0;

	hostAddr = __get_dma_pages(GFP_KERNEL, get_order(len));
	if (hostAddr == 0)
		return -ENOMEM;

	if (copy_from_user((void *)hostAddr, buf, len)) {
		rv = -EFAULT;
		goto exit;
	}

	rv = do_mbcs_sram_dmaread(soft, hostAddr, len, off);

      exit:
	free_pages(hostAddr, get_order(len));

	return rv;
}

static loff_t mbcs_sram_llseek(struct file * filp, loff_t off, int whence)
{
	loff_t newpos;

	switch (whence) {
	case SEEK_SET:
		newpos = off;
		break;

	case SEEK_CUR:
		newpos = filp->f_pos + off;
		break;

	case SEEK_END:
		newpos = MBCS_SRAM_SIZE + off;
		break;

	default:		/* can't happen */
		return -EINVAL;
	}

	if (newpos < 0)
		return -EINVAL;

	filp->f_pos = newpos;

	return newpos;
}

static uint64_t mbcs_pioaddr(struct mbcs_soft *soft, uint64_t offset)
{
	uint64_t mmr_base;

	mmr_base = (uint64_t) (soft->mmr_base + offset);

	return mmr_base;
}

static void mbcs_debug_pioaddr_set(struct mbcs_soft *soft)
{
	soft->debug_addr = mbcs_pioaddr(soft, MBCS_DEBUG_START);
}

static void mbcs_gscr_pioaddr_set(struct mbcs_soft *soft)
{
	soft->gscr_addr = mbcs_pioaddr(soft, MBCS_GSCR_START);
}

static int mbcs_gscr_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct cx_dev *cx_dev = fp->private_data;
	struct mbcs_soft *soft = cx_dev->soft;

	if (vma->vm_pgoff != 0)
		return -EINVAL;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    __pa(soft->gscr_addr) >> PAGE_SHIFT,
			    PAGE_SIZE,
			    vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

/**
 * mbcs_completion_intr_handler - Primary completion handler.
 * @irq: irq
 * @arg: soft struct for device
 *
 */
static irqreturn_t
mbcs_completion_intr_handler(int irq, void *arg)
{
	struct mbcs_soft *soft = (struct mbcs_soft *)arg;
	void *mmr_base;
	union cm_status cm_status;
	union cm_control cm_control;

	mmr_base = soft->mmr_base;
	cm_status.cm_status_reg = MBCS_MMR_GET(mmr_base, MBCS_CM_STATUS);

	if (cm_status.rd_dma_done) {
		/* stop dma-read engine, clear status */
		cm_control.cm_control_reg =
		    MBCS_MMR_GET(mmr_base, MBCS_CM_CONTROL);
		cm_control.rd_dma_clr = 1;
		MBCS_MMR_SET(mmr_base, MBCS_CM_CONTROL,
			     cm_control.cm_control_reg);
		atomic_set(&soft->dmaread_done, 1);
		wake_up(&soft->dmaread_queue);
	}
	if (cm_status.wr_dma_done) {
		/* stop dma-write engine, clear status */
		cm_control.cm_control_reg =
		    MBCS_MMR_GET(mmr_base, MBCS_CM_CONTROL);
		cm_control.wr_dma_clr = 1;
		MBCS_MMR_SET(mmr_base, MBCS_CM_CONTROL,
			     cm_control.cm_control_reg);
		atomic_set(&soft->dmawrite_done, 1);
		wake_up(&soft->dmawrite_queue);
	}
	if (cm_status.alg_done) {
		/* clear status */
		cm_control.cm_control_reg =
		    MBCS_MMR_GET(mmr_base, MBCS_CM_CONTROL);
		cm_control.alg_done_clr = 1;
		MBCS_MMR_SET(mmr_base, MBCS_CM_CONTROL,
			     cm_control.cm_control_reg);
		atomic_set(&soft->algo_done, 1);
		wake_up(&soft->algo_queue);
	}

	return IRQ_HANDLED;
}

/**
 * mbcs_intr_alloc - Allocate interrupts.
 * @dev: device pointer
 *
 */
static int mbcs_intr_alloc(struct cx_dev *dev)
{
	struct sn_irq_info *sn_irq;
	struct mbcs_soft *soft;
	struct getdma *getdma;
	struct putdma *putdma;
	struct algoblock *algo;

	soft = dev->soft;
	getdma = &soft->getdma;
	putdma = &soft->putdma;
	algo = &soft->algo;

	soft->get_sn_irq = NULL;
	soft->put_sn_irq = NULL;
	soft->algo_sn_irq = NULL;

	sn_irq = tiocx_irq_alloc(dev->cx_id.nasid, TIOCX_CORELET, -1, -1, -1);
	if (sn_irq == NULL)
		return -EAGAIN;
	soft->get_sn_irq = sn_irq;
	getdma->intrHostDest = sn_irq->irq_xtalkaddr;
	getdma->intrVector = sn_irq->irq_irq;
	if (request_irq(sn_irq->irq_irq,
			(void *)mbcs_completion_intr_handler, IRQF_SHARED,
			"MBCS get intr", (void *)soft)) {
		tiocx_irq_free(soft->get_sn_irq);
		return -EAGAIN;
	}

	sn_irq = tiocx_irq_alloc(dev->cx_id.nasid, TIOCX_CORELET, -1, -1, -1);
	if (sn_irq == NULL) {
		free_irq(soft->get_sn_irq->irq_irq, soft);
		tiocx_irq_free(soft->get_sn_irq);
		return -EAGAIN;
	}
	soft->put_sn_irq = sn_irq;
	putdma->intrHostDest = sn_irq->irq_xtalkaddr;
	putdma->intrVector = sn_irq->irq_irq;
	if (request_irq(sn_irq->irq_irq,
			(void *)mbcs_completion_intr_handler, IRQF_SHARED,
			"MBCS put intr", (void *)soft)) {
		tiocx_irq_free(soft->put_sn_irq);
		free_irq(soft->get_sn_irq->irq_irq, soft);
		tiocx_irq_free(soft->get_sn_irq);
		return -EAGAIN;
	}

	sn_irq = tiocx_irq_alloc(dev->cx_id.nasid, TIOCX_CORELET, -1, -1, -1);
	if (sn_irq == NULL) {
		free_irq(soft->put_sn_irq->irq_irq, soft);
		tiocx_irq_free(soft->put_sn_irq);
		free_irq(soft->get_sn_irq->irq_irq, soft);
		tiocx_irq_free(soft->get_sn_irq);
		return -EAGAIN;
	}
	soft->algo_sn_irq = sn_irq;
	algo->intrHostDest = sn_irq->irq_xtalkaddr;
	algo->intrVector = sn_irq->irq_irq;
	if (request_irq(sn_irq->irq_irq,
			(void *)mbcs_completion_intr_handler, IRQF_SHARED,
			"MBCS algo intr", (void *)soft)) {
		tiocx_irq_free(soft->algo_sn_irq);
		free_irq(soft->put_sn_irq->irq_irq, soft);
		tiocx_irq_free(soft->put_sn_irq);
		free_irq(soft->get_sn_irq->irq_irq, soft);
		tiocx_irq_free(soft->get_sn_irq);
		return -EAGAIN;
	}

	return 0;
}

/**
 * mbcs_intr_dealloc - Remove interrupts.
 * @dev: device pointer
 *
 */
static void mbcs_intr_dealloc(struct cx_dev *dev)
{
	struct mbcs_soft *soft;

	soft = dev->soft;

	free_irq(soft->get_sn_irq->irq_irq, soft);
	tiocx_irq_free(soft->get_sn_irq);
	free_irq(soft->put_sn_irq->irq_irq, soft);
	tiocx_irq_free(soft->put_sn_irq);
	free_irq(soft->algo_sn_irq->irq_irq, soft);
	tiocx_irq_free(soft->algo_sn_irq);
}

static inline int mbcs_hw_init(struct mbcs_soft *soft)
{
	void *mmr_base = soft->mmr_base;
	union cm_control cm_control;
	union cm_req_timeout cm_req_timeout;
	uint64_t err_stat;

	cm_req_timeout.cm_req_timeout_reg =
	    MBCS_MMR_GET(mmr_base, MBCS_CM_REQ_TOUT);

	cm_req_timeout.time_out = MBCS_CM_CONTROL_REQ_TOUT_MASK;
	MBCS_MMR_SET(mmr_base, MBCS_CM_REQ_TOUT,
		     cm_req_timeout.cm_req_timeout_reg);

	mbcs_gscr_pioaddr_set(soft);
	mbcs_debug_pioaddr_set(soft);

	/* clear errors */
	err_stat = MBCS_MMR_GET(mmr_base, MBCS_CM_ERR_STAT);
	MBCS_MMR_SET(mmr_base, MBCS_CM_CLR_ERR_STAT, err_stat);
	MBCS_MMR_ZERO(mmr_base, MBCS_CM_ERROR_DETAIL1);

	/* enable interrupts */
	/* turn off 2^23 (INT_EN_PIO_REQ_ADDR_INV) */
	MBCS_MMR_SET(mmr_base, MBCS_CM_ERR_INT_EN, 0x3ffffff7e00ffUL);

	/* arm status regs and clear engines */
	cm_control.cm_control_reg = MBCS_MMR_GET(mmr_base, MBCS_CM_CONTROL);
	cm_control.rearm_stat_regs = 1;
	cm_control.alg_clr = 1;
	cm_control.wr_dma_clr = 1;
	cm_control.rd_dma_clr = 1;

	MBCS_MMR_SET(mmr_base, MBCS_CM_CONTROL, cm_control.cm_control_reg);

	return 0;
}

static ssize_t show_algo(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct cx_dev *cx_dev = to_cx_dev(dev);
	struct mbcs_soft *soft = cx_dev->soft;
	uint64_t debug0;

	/*
	 * By convention, the first debug register contains the
	 * algorithm number and revision.
	 */
	debug0 = *(uint64_t *) soft->debug_addr;

	return sprintf(buf, "0x%x 0x%x\n",
		       upper_32_bits(debug0), lower_32_bits(debug0));
}

static ssize_t store_algo(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int n;
	struct cx_dev *cx_dev = to_cx_dev(dev);
	struct mbcs_soft *soft = cx_dev->soft;

	if (count <= 0)
		return 0;

	n = simple_strtoul(buf, NULL, 0);

	if (n == 1) {
		mbcs_algo_start(soft);
		if (wait_event_interruptible(soft->algo_queue,
					atomic_read(&soft->algo_done)))
			return -ERESTARTSYS;
	}

	return count;
}

DEVICE_ATTR(algo, 0644, show_algo, store_algo);

/**
 * mbcs_probe - Initialize for device
 * @dev: device pointer
 * @device_id: id table pointer
 *
 */
static int mbcs_probe(struct cx_dev *dev, const struct cx_device_id *id)
{
	struct mbcs_soft *soft;

	dev->soft = NULL;

	soft = kzalloc(sizeof(struct mbcs_soft), GFP_KERNEL);
	if (soft == NULL)
		return -ENOMEM;

	soft->nasid = dev->cx_id.nasid;
	list_add(&soft->list, &soft_list);
	soft->mmr_base = (void *)tiocx_swin_base(dev->cx_id.nasid);
	dev->soft = soft;
	soft->cxdev = dev;

	init_waitqueue_head(&soft->dmawrite_queue);
	init_waitqueue_head(&soft->dmaread_queue);
	init_waitqueue_head(&soft->algo_queue);

	mutex_init(&soft->dmawritelock);
	mutex_init(&soft->dmareadlock);
	mutex_init(&soft->algolock);

	mbcs_getdma_init(&soft->getdma);
	mbcs_putdma_init(&soft->putdma);
	mbcs_algo_init(&soft->algo);

	mbcs_hw_init(soft);

	/* Allocate interrupts */
	mbcs_intr_alloc(dev);

	device_create_file(&dev->dev, &dev_attr_algo);

	return 0;
}

static int mbcs_remove(struct cx_dev *dev)
{
	if (dev->soft) {
		mbcs_intr_dealloc(dev);
		kfree(dev->soft);
	}

	device_remove_file(&dev->dev, &dev_attr_algo);

	return 0;
}

static const struct cx_device_id mbcs_id_table[] = {
	{
	 .part_num = MBCS_PART_NUM,
	 .mfg_num = MBCS_MFG_NUM,
	 },
	{
	 .part_num = MBCS_PART_NUM_ALG0,
	 .mfg_num = MBCS_MFG_NUM,
	 },
	{0, 0}
};

MODULE_DEVICE_TABLE(cx, mbcs_id_table);

static struct cx_drv mbcs_driver = {
	.name = DEVICE_NAME,
	.id_table = mbcs_id_table,
	.probe = mbcs_probe,
	.remove = mbcs_remove,
};

static void __exit mbcs_exit(void)
{
	unregister_chrdev(mbcs_major, DEVICE_NAME);
	cx_driver_unregister(&mbcs_driver);
}

static int __init mbcs_init(void)
{
	int rv;

	if (!ia64_platform_is("sn2"))
		return -ENODEV;

	// Put driver into chrdevs[].  Get major number.
	rv = register_chrdev(mbcs_major, DEVICE_NAME, &mbcs_ops);
	if (rv < 0) {
		DBG(KERN_ALERT "mbcs_init: can't get major number. %d\n", rv);
		return rv;
	}
	mbcs_major = rv;

	return cx_driver_register(&mbcs_driver);
}

module_init(mbcs_init);
module_exit(mbcs_exit);

MODULE_AUTHOR("Bruce Losure <blosure@sgi.com>");
MODULE_DESCRIPTION("Driver for MOATB Core Services");
MODULE_LICENSE("GPL");
                                                                                                                                                                           /*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2005 Silicon Graphics, Inc.  All rights reserved.
 */

#ifndef __MBCS_H__
#define __MBCS_H__

/*
 * General macros
 */
#define MB	(1024*1024)
#define MB2	(2*MB)
#define MB4	(4*MB)
#define MB6	(6*MB)

/*
 * Offsets and masks
 */
#define MBCS_CM_ID		0x0000	/* Identification */
#define MBCS_CM_STATUS		0x0008	/* Status */
#define MBCS_CM_ERROR_DETAIL1	0x0010	/* Error Detail1 */
#define MBCS_CM_ERROR_DETAIL2	0x0018	/* Error Detail2 */
#define MBCS_CM_CONTROL		0x0020	/* Control */
#define MBCS_CM_REQ_TOUT	0x0028	/* Request Time-out */
#define MBCS_CM_ERR_INT_DEST	0x0038	/* Error Interrupt Destination */
#define MBCS_CM_TARG_FL		0x0050	/* Target Flush */
#define MBCS_CM_ERR_STAT	0x0060	/* Error Status */
#define MBCS_CM_CLR_ERR_STAT	0x0068	/* Clear Error Status */
#define MBCS_CM_ERR_INT_EN	0x0070	/* Error Interrupt Enable */
#define MBCS_RD_DMA_SYS_ADDR	0x0100	/* Read DMA System Address */
#define MBCS_RD_DMA_LOC_ADDR	0x0108	/* Read DMA Local Address */
#define MBCS_RD_DMA_CTRL	0x0110	/* Read DMA Control */
#define MBCS_RD_DMA_AMO_DEST	0x0118	/* Read DMA AMO Destination */
#define MBCS_RD_DMA_INT_DEST	0x0120	/* Read DMA Interrupt Destination */
#define MBCS_RD_DMA_AUX_STAT	0x0130	/* Read DMA Auxiliary Status */
#define MBCS_WR_DMA_SYS_ADDR	0x0200	/* Write DMA System Address */
#define MBCS_WR_DMA_LOC_ADDR	0x0208	/* Write DMA Local Address */
#define MBCS_WR_DMA_CTRL	0x0210	/* Write DMA Control */
#define MBCS_WR_DMA_AMO_DEST	0x0218	/* Write DMA AMO Destination */
#define MBCS_WR_DMA_INT_DEST	0x0220	/* Write DMA Interrupt Destination */
#define MBCS_WR_DMA_AUX_STAT	0x0230	/* Write DMA Auxiliary Status */
#define MBCS_ALG_AMO_DEST	0x0300	/* Algorithm AMO Destination */
#define MBCS_ALG_INT_DEST	0x0308	/* Algorithm Interrupt Destination */
#define MBCS_ALG_OFFSETS	0x0310
#define MBCS_ALG_STEP		0x0318	/* Algorithm Step */

#define MBCS_GSCR_START		0x0000000
#define MBCS_DEBUG_START	0x0100000
#define MBCS_RAM0_START		0x0200000
#define MBCS_RAM1_START		0x0400000
#define MBCS_RAM2_START		0x0600000

#define MBCS_CM_CONTROL_REQ_TOUT_MASK 0x0000000000ffffffUL
//#define PIO_BASE_ADDR_BASE_OFFSET_MASK 0x00fffffffff00000UL

#define MBCS_SRAM_SIZE		(1024*1024)
#define MBCS_CACHELINE_SIZE	128

/*
 * MMR get's and put's
 */
#define MBCS_MMR_ADDR(mmr_base, offset)((uint64_t *)(mmr_base + offset))
#define MBCS_MMR_SET(mmr_base, offset, value) {			\
	uint64_t *mbcs_mmr_set_u64p, readback;				\
	mbcs_mmr_set_u64p = (uint64_t *)(mmr_base + offset);	\
	*mbcs_mmr_set_u64p = value;					\
	readback = *mbcs_mmr_set_u64p; \
}
#define MBCS_MMR_GET(mmr_base, offset) *(uint64_t *)(mmr_base + offset)
#define MBCS_MMR_ZERO(mmr_base, offset) MBCS_MMR_SET(mmr_base, offset, 0)

/*
 * MBCS mmr structures
 */
union cm_id {
	uint64_t cm_id_reg;
	struct {
		uint64_t always_one:1,	// 0
		 mfg_id:11,	// 11:1
		 part_num:16,	// 27:12
		 bitstream_rev:8,	// 35:28
		:28;		// 63:36
	};
};

union cm_status {
	uint64_t cm_status_reg;
	struct {
		uint64_t pending_reads:8,	// 7:0
		 pending_writes:8,	// 15:8
		 ice_rsp_credits:8,	// 23:16
		 ice_req_credits:8,	// 31:24
		 cm_req_credits:8,	// 39:32
		:1,		// 40
		 rd_dma_in_progress:1,	// 41
		 rd_dma_done:1,	// 42
		:1,		// 43
		 wr_dma_in_progress:1,	// 44
		 wr_dma_done:1,	// 45
		 alg_waiting:1,	// 46
		 alg_pipe_running:1,	// 47
		 alg_done:1,	// 48
		:3,		// 51:49
		 pending_int_reqs:8,	// 59:52
		:3,		// 62:60
		 alg_half_speed_sel:1;	// 63
	};
};

union cm_error_detail1 {
	uint64_t cm_error_detail1_reg;
	struct {
		uint64_t packet_type:4,	// 3:0
		 source_id:2,	// 5:4
		 data_size:2,	// 7:6
		 tnum:8,	// 15:8
		 byte_enable:8,	// 23:16
		 gfx_cred:8,	// 31:24
		 read_type:2,	// 33:32
		 pio_or_memory:1,	// 34
		 head_cw_error:1,	// 35
		:12,		// 47:36
		 head_error_bit:1,	// 48
		 data_error_bit:1,	// 49
		:13,		// 62:50
		 valid:1;	// 63
	};
};

union cm_error_detail2 {
	uint64_t cm_error_detail2_reg;
	struct {
		uint64_t address:56,	// 55:0
		:8;		// 63:56
	};
};

union cm_control {
	uint64_t cm_control_reg;
	struct {
		uint64_t cm_id:2,	// 1:0
		:2,		// 3:2
		 max_trans:5,	// 8:4
		:3,		// 11:9
		 address_mode:1,	// 12
		:7,		// 19:13
		 credit_limit:8,	// 27:20
		:5,		// 32:28
		 rearm_stat_regs:1,	// 33
		 prescalar_byp:1,	// 34
		 force_gap_war:1,	// 35
		 rd_dma_go:1,	// 36
		 wr_dma_go:1,	// 37
		 alg_go:1,	// 38
		 rd_dma_clr:1,	// 39
		 wr_dma_clr:1,	// 40
		 alg_clr:1,	// 41
		:2,		// 43:42
		 alg_wait_step:1,	// 44
		 alg_done_amo_en:1,	// 45
		 alg_done_int_en:1,	// 46
		:1,		// 47
		 alg_sram0_locked:1,	// 48
		 alg_sram1_locked:1,	// 49
		 alg_sram2_locked:1,	// 50
		 alg_done_clr:1,	// 51
		:12;		// 63:52
	};
};

union cm_req_timeout {
	uint64_t cm_req_timeout_reg;
	struct {
		uint64_t time_out:24,	// 23:0
		:40;		// 63:24
	};
};

union intr_dest {
	uint64_t intr_dest_reg;
	struct {
		uint64_t address:56,	// 55:0
		 int_vector:8;	// 63:56
	};
};

union cm_error_status {
	uint64_t cm_error_status_reg;
	struct {
		uint64_t ecc_sbe:1,	// 0
		 ecc_mbe:1,	// 1
		 unsupported_req:1,	// 2
		 unexpected_rsp:1,	// 3
		 bad_length:1,	// 4
		 bad_datavalid:1,	// 5
		 buffer_overflow:1,	// 6
		 request_timeout:1,	// 7
		:8,		// 15:8
		 head_inv_data_size:1,	// 16
		 rsp_pactype_inv:1,	// 17
		 head_sb_err:1,	// 18
		 missing_head:1,	// 19
		 head_inv_rd_type:1,	// 20
		 head_cmd_err_bit:1,	// 21
		 req_addr_align_inv:1,	// 22
		 pio_req_addr_inv:1,	// 23
		 req_range_dsize_inv:1,	// 24
		 early_term:1,	// 25
		 early_tail:1,	// 26
		 missing_tail:1,	// 27
		 data_flit_sb_err:1,	// 28
		 cm2hcm_req_cred_of:1,	// 29
		 cm2hcm_rsp_cred_of:1,	// 30
		 rx_bad_didn:1,	// 31
		 rd_dma_err_rsp:1,	// 32
		 rd_dma_tnum_tout:1,	// 33
		 rd_dma_multi_tnum_tou:1,	// 34
		 wr_dma_err_rsp:1,	// 35
		 wr_dma_tnum_tout:1,	// 36
		 wr_dma_multi_tnum_tou:1,	// 37
		 alg_data_overflow:1,	// 38
		 alg_data_underflow:1,	// 39
		 ram0_access_conflict:1,	// 40
		 ram1_access_conflict:1,	// 41
		 ram2_access_conflict:1,	// 42
		 ram0_perr:1,	// 43
		 ram1_perr:1,	// 44
		 ram2_perr:1,	// 45
		 int_gen_rsp_err:1,	// 46
		 int_gen_tnum_tout:1,	// 47
		 rd_dma_prog_err:1,	// 48
		 wr_dma_prog_err:1,	// 49
		:14;		// 63:50
	};
};

union cm_clr_error_status {
	uint64_t cm_clr_error_status_reg;
	struct {
		uint64_t clr_ecc_sbe:1,	// 0
		 clr_ecc_mbe:1,	// 1
		 clr_unsupported_req:1,	// 2
		 clr_unexpected_rsp:1,	// 3
		 clr_bad_length:1,	// 4
		 clr_bad_datavalid:1,	// 5
		 clr_buffer_overflow:1,	// 6
		 clr_request_timeout:1,	// 7
		:8,		// 15:8
		 clr_head_inv_data_siz:1,	// 16
		 clr_rsp_pactype_inv:1,	// 17
		 clr_head_sb_err:1,	// 18
		 clr_missing_head:1,	// 19
		 clr_head_inv_rd_type:1,	// 20
		 clr_head_cmd_err_bit:1,	// 21
		 clr_req_addr_align_in:1,	// 22
		 clr_pio_req_addr_inv:1,	// 23
		 clr_req_range_dsize_i:1,	// 24
		 clr_early_term:1,	// 25
		 clr_early_tail:1,	// 26
		 clr_missing_tail:1,	// 27
		 clr_data_flit_sb_err:1,	// 28
		 clr_cm2hcm_req_cred_o:1,	// 29
		 clr_cm2hcm_rsp_cred_o:1,	// 30
		 clr_rx_bad_didn:1,	// 31
		 clr_rd_dma_err_rsp:1,	// 32
		 clr_rd_dma_tnum_tout:1,	// 33
		 clr_rd_dma_multi_tnum:1,	// 34
		 clr_wr_dma_err_rsp:1,	// 35
		 clr_wr_dma_tnum_tout:1,	// 36
		 clr_wr_dma_multi_tnum:1,	// 37
		 clr_alg_data_overflow:1,	// 38
		 clr_alg_data_underflo:1,	// 39
		 clr_ram0_access_confl:1,	// 40
		 clr_ram1_access_confl:1,	// 41
		 clr_ram2_access_confl:1,	// 42
		 clr_ram0_perr:1,	// 43
		 clr_ram1_perr:1,	// 44
		 clr_ram2_perr:1,	// 45
		 clr_int_gen_rsp_err:1,	// 46
		 clr_int_gen_tnum_tout:1,	// 47
		 clr_rd_dma_prog_err:1,	// 48
		 clr_wr_dma_prog_err:1,	// 49
		:14;		// 63:50
	};
};

union cm_error_intr_enable {
	uint64_t cm_error_intr_enable_reg;
	struct {
		uint64_t int_en_ecc_sbe:1,	// 0
		 int_en_ecc_mbe:1,	// 1
		 int_en_unsupported_re:1,	// 2
		 int_en_unexpected_rsp:1,	// 3
		 int_en_bad_length:1,	// 4
		 int_en_bad_datavalid:1,	// 5
		 int_en_buffer_overflo:1,	// 6
		 int_en_request_timeou:1,	// 7
		:8,		// 15:8
		 int_en_head_inv_data_:1,	// 16
		 int_en_rsp_pactype_in:1,	// 17
		 int_en_head_sb_err:1,	// 18
		 int_en_missing_head:1,	// 19
		 int_en_head_inv_rd_ty:1,	// 20
		 int_en_head_cmd_err_b:1,	// 21
		 int_en_req_addr_align:1,	// 22
		 int_en_pio_req_addr_i:1,	// 23
		 int_en_req_range_dsiz:1,	// 24
		 int_en_early_term:1,	// 25
		 int_en_early_tail:1,	// 26
		 int_en_missing_tail:1,	// 27
		 int_en_data_flit_sb_e:1,	// 28
		 int_en_cm2hcm_req_cre:1,	// 29
		 int_en_cm2hcm_rsp_cre:1,	// 30
		 int_en_rx_bad_didn:1,	// 31
		 int_en_rd_dma_err_rsp:1,	// 32
		 int_en_rd_dma_tnum_to:1,	// 33
		 int_en_rd_dma_multi_t:1,	// 34
		 int_en_wr_dma_err_rsp:1,	// 35
		 int_en_wr_dma_tnum_to:1,	// 36
		 int_en_wr_dma_multi_t:1,	// 37
		 int_en_alg_data_overf:1,	// 38
		 int_en_alg_data_under:1,	// 39
		 int_en_ram0_access_co:1,	// 40
		 int_en_ram1_access_co:1,	// 41
		 int_en_ram2_access_co:1,	// 42
		 int_en_ram0_perr:1,	// 43
		 int_en_ram1_perr:1,	// 44
		 int_en_ram2_perr:1,	// 45
		 int_en_int_gen_rsp_er:1,	// 46
		 int_en_int_gen_tnum_t:1,	// 47
		 int_en_rd_dma_prog_er:1,	// 48
		 int_en_wr_dma_prog_er:1,	// 49
		:14;		// 63:50
	};
};

struct cm_mmr {
	union cm_id id;
	union cm_status status;
	union cm_error_detail1 err_detail1;
	union cm_error_detail2 err_detail2;
	union cm_control control;
	union cm_req_timeout req_timeout;
	uint64_t reserved1[1];
	union intr_dest int_dest;
	uint64_t reserved2[2];
	uint64_t targ_flush;
	uint64_t reserved3[1];
	union cm_error_status err_status;
	union cm_clr_error_status clr_err_status;
	union cm_error_intr_enable int_enable;
};

union dma_hostaddr {
	uint64_t dma_hostaddr_reg;
	struct {
		uint64_t dma_sys_addr:56,	// 55:0
		:8;		// 63:56
	};
};

union dma_localaddr {
	uint64_t dma_localaddr_reg;
	struct {
		uint64_t dma_ram_addr:21,	// 20:0
		 dma_ram_sel:2,	// 22:21
		:41;		// 63:23
	};
};

union dma_control {
	uint64_t dma_control_reg;
	struct {
		uint64_t dma_op_length:16,	// 15:0
		:18,		// 33:16
		 done_amo_en:1,	// 34
		 done_int_en:1,	// 35
		:1,		// 36
		 pio_mem_n:1,	// 37
		:26;		// 63:38
	};
};

union dma_amo_dest {
	uint64_t dma_amo_dest_reg;
	struct {
		uint64_t dma_amo_sys_addr:56,	// 55:0
		 dma_amo_mod_type:3,	// 58:56
		:5;		// 63:59
	};
};

union rdma_aux_status {
	uint64_t rdma_aux_status_reg;
	struct {
		uint64_t op_num_pacs_left:17,	// 16:0
		:5,		// 21:17
		 lrsp_buff_empty:1,	// 22
		:17,		// 39:23
		 pending_reqs_left:6,	// 45:40
		:18;		// 63:46
	};
};

struct rdma_mmr {
	union dma_hostaddr host_addr;
	union dma_localaddr local_addr;
	union dma_control control;
	union dma_amo_dest amo_dest;
	union intr_dest intr_dest;
	union rdma_aux_status aux_status;
};

union wdma_aux_status {
	uint64_t wdma_aux_status_reg;
	struct {
		uint64_t op_num_pacs_left:17,	// 16:0
		:4,		// 20:17
		 lreq_buff_empty:1,	// 21
		:18,		// 39:22
		 pending_reqs_left:6,	// 45:40
		:18;		// 63:46
	};
};

struct wdma_mmr {
	union dma_hostaddr host_addr;
	union dma_localaddr local_addr;
	union dma_control control;
	union dma_amo_dest amo_dest;
	union intr_dest intr_dest;
	union wdma_aux_status aux_status;
};

union algo_step {
	uint64_t algo_step_reg;
	struct {
		uint64_t alg_step_cnt:16,	// 15:0
		:48;		// 63:16
	};
};

struct algo_mmr {
	union dma_amo_dest amo_dest;
	union intr_dest intr_dest;
	union {
		uint64_t algo_offset_reg;
		struct {
			uint64_t sram0_offset:7,	// 6:0
			reserved0:1,	// 7
			sram1_offset:7,	// 14:8
			reserved1:1,	// 15
			sram2_offset:7,	// 22:16
			reserved2:14;	// 63:23
		};
	} sram_offset;
	union algo_step step;
};

struct mbcs_mmr {
	struct cm_mmr cm;
	uint64_t reserved1[17];
	struct rdma_mmr rdDma;
	uint64_t reserved2[25];
	struct wdma_mmr wrDma;
	uint64_t reserved3[25];
	struct algo_mmr algo;
	uint64_t reserved4[156];
};

/*
 * defines
 */
#define DEVICE_NAME "mbcs"
#define MBCS_PART_NUM 0xfff0
#define MBCS_PART_NUM_ALG0 0xf001
#define MBCS_MFG_NUM  0x1

struct algoblock {
	uint64_t amoHostDest;
	uint64_t amoModType;
	uint64_t intrHostDest;
	uint64_t intrVector;
	uint64_t algoStepCount;
};

struct getdma {
	uint64_t hostAddr;
	uint64_t localAddr;
	uint64_t bytes;
	uint64_t DoneAmoEnable;
	uint64_t DoneIntEnable;
	uint64_t peerIO;
	uint64_t amoHostDest;
	uint64_t amoModType;
	uint64_t intrHostDest;
	uint64_t intrVector;
};

struct putdma {
	uint64_t hostAddr;
	uint64_t localAddr;
	uint64_t bytes;
	uint64_t DoneAmoEnable;
	uint64_t DoneIntEnable;
	uint64_t peerIO;
	uint64_t amoHostDest;
	uint64_t amoModType;
	uint64_t intrHostDest;
	uint64_t intrVector;
};

struct mbcs_soft {
	struct list_head list;
	struct cx_dev *cxdev;
	int major;
	int nasid;
	void *mmr_base;
	wait_queue_head_t dmawrite_queue;
	wait_queue_head_t dmaread_queue;
	wait_queue_head_t algo_queue;
	struct sn_irq_info *get_sn_irq;
	struct sn_irq_info *put_sn_irq;
	struct sn_irq_info *algo_sn_irq;
	struct getdma getdma;
	struct putdma putdma;
	struct algoblock algo;
	uint64_t gscr_addr;	// pio addr
	uint64_t ram0_addr;	// pio addr
	uint64_t ram1_addr;	// pio addr
	uint64_t ram2_addr;	// pio addr
	uint64_t debug_addr;	// pio addr
	atomic_t dmawrite_done;
	atomic_t dmaread_done;
	atomic_t algo_done;
	struct mutex dmawritelock;
	struct mutex dmareadlock;
	struct mutex algolock;
};

static int mbcs_open(struct inode *ip, struct file *fp);
static ssize_t mbcs_sram_read(struct file *fp, char __user *buf, size_t len,
			      loff_t * off);
static ssize_t mbcs_sram_write(struct file *fp, const char __user *buf, size_t len,
			       loff_t * off);
static loff_t mbcs_sram_llseek(struct file *filp, loff_t off, int whence);
static int mbcs_gscr_mmap(struct file *fp, struct vm_area_struct *vma);

#endif				// __MBCS_H__
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 *  linux/drivers/char/mem.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Added devfs support.
 *    Jan-11-1998, C. Scott Ananian <cananian@alumni.princeton.edu>
 *  Shared /dev/zero mmapping support, Feb 2000, Kanoj Sarcar <kanoj@sgi.com>
 */

#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/backing-dev.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/uio.h>

#ifdef CONFIG_KNOX_KAP
#include <linux/knox_kap.h>
#endif

#ifdef CONFIG_MST_LDO
#include <linux/mst_ctrl.h>
#endif

#include <linux/uaccess.h>

#ifdef CONFIG_IA64
# include <linux/efi.h>
#endif

#define DEVPORT_MINOR	4

#ifdef CONFIG_SRANDOM
#include <../drivers/char/srandom/srandom.h>
#endif

static inline unsigned long size_inside_page(unsigned long start,
					     unsigned long size)
{
	unsigned long sz;

	sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));

	return min(sz, size);
}

#ifndef ARCH_HAS_VALID_PHYS_ADDR_RANGE
static inline int valid_phys_addr_range(phys_addr_t addr, size_t count)
{
	return addr + count <= __pa(high_memory);
}

static inline int valid_mmap_phys_addr_range(unsigned long pfn, size_t size)
{
	return 1;
}
#endif

#ifdef CONFIG_STRICT_DEVMEM
static inline int page_is_allowed(unsigned long pfn)
{
	return devmem_is_allowed(pfn);
}
static inline int range_is_allowed(unsigned long pfn, unsigned long size)
{
	u64 from = ((u64)pfn) << PAGE_SHIFT;
	u64 to = from + size;
	u64 cursor = from;

	while (cursor < to) {
		if (!devmem_is_allowed(pfn))
			return 0;
		cursor += PAGE_SIZE;
		pfn++;
	}
	return 1;
}
#else
static inline int page_is_allowed(unsigned long pfn)
{
	return 1;
}
static inline int range_is_allowed(unsigned long pfn, unsigned long size)
{
	return 1;
}
#endif

#ifndef unxlate_dev_mem_ptr
#define unxlate_dev_mem_ptr unxlate_dev_mem_ptr
void __weak unxlate_dev_mem_ptr(phys_addr_t phys, void *addr)
{
}
#endif

static inline bool should_stop_iteration(void)
{
	if (need_resched())
		cond_resched();
	return fatal_signal_pending(current);
}

/*
 * This funcion reads the *physical* memory. The f_pos points directly to the
 * memory location.
 */
static ssize_t read_mem(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	phys_addr_t p = *ppos;
	ssize_t read, sz;
	void *ptr;

	if (p != *ppos)
		return 0;

	if (!valid_phys_addr_range(p, count))
		return -EFAULT;
	read = 0;
#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (p < PAGE_SIZE) {
		sz = size_inside_page(p, count);
		if (sz > 0) {
			if (clear_user(buf, sz))
				return -EFAULT;
			buf += sz;
			p += sz;
			count -= sz;
			read += sz;
		}
	}
#endif

	while (count > 0) {
		unsigned long remaining;
		int allowed;

		sz = size_inside_page(p, count);

		allowed = page_is_allowed(p >> PAGE_SHIFT);
		if (!allowed)
			return -EPERM;
		if (allowed == 2) {
			/* Show zeros for restricted memory. */
			remaining = clear_user(buf, sz);
		} else {
			/*
			 * On ia64 if a page has been mapped somewhere as
			 * uncached, then it must also be accessed uncached
			 * by the kernel or data corruption may occur.
			 */
			ptr = xlate_dev_mem_ptr(p);
			if (!ptr)
				return -EFAULT;

			remaining = copy_to_user(buf, ptr, sz);

			unxlate_dev_mem_ptr(p, ptr);
		}

		if (remaining)
			return -EFAULT;

		buf += sz;
		p += sz;
		count -= sz;
		read += sz;
		if (should_stop_iteration())
			break;
	}

	*ppos += read;
	return read;
}

static ssize_t write_mem(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	phys_addr_t p = *ppos;
	ssize_t written, sz;
	unsigned long copied;
	void *ptr;

	if (p != *ppos)
		return -EFBIG;

	if (!valid_phys_addr_range(p, count))
		return -EFAULT;

	written = 0;

#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (p < PAGE_SIZE) {
		sz = size_inside_page(p, count);
		/* Hmm. Do something? */
		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
	}
#endif

	while (count > 0) {
		int allowed;

		sz = size_inside_page(p, count);

		allowed = page_is_allowed(p >> PAGE_SHIFT);
		if (!allowed)
			return -EPERM;

		/* Skip actual writing when a page is marked as restricted. */
		if (allowed == 1) {
			/*
			 * On ia64 if a page has been mapped somewhere as
			 * uncached, then it must also be accessed uncached
			 * by the kernel or data corruption may occur.
			 */
			ptr = xlate_dev_mem_ptr(p);
			if (!ptr) {
				if (written)
					break;
				return -EFAULT;
			}

			copied = copy_from_user(ptr, buf, sz);
			unxlate_dev_mem_ptr(p, ptr);
			if (copied) {
				written += sz - copied;
				if (written)
					break;
				return -EFAULT;
			}
		}

		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
		if (should_stop_iteration())
			break;
	}

	*ppos += written;
	return written;
}

int __weak phys_mem_access_prot_allowed(struct file *file,
	unsigned long pfn, unsigned long size, pgprot_t *vma_prot)
{
	return 1;
}

#ifndef __HAVE_PHYS_MEM_ACCESS_PROT

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 *
 */
#ifdef pgprot_noncached
static int uncached_access(struct file *file, phys_addr_t addr)
{
#if defined(CONFIG_IA64)
	/*
	 * On ia64, we ignore O_DSYNC because we cannot tolerate memory
	 * attribute aliases.
	 */
	return !(efi_mem_attributes(addr) & EFI_MEMORY_WB);
#elif defined(CONFIG_MIPS)
	{
		extern int __uncached_access(struct file *file,
					     unsigned long addr);

		return __uncached_access(file, addr);
	}
#else
	/*
	 * Accessing memory above the top the kernel knows about or through a
	 * file pointer
	 * that was marked O_DSYNC will be done non-cached.
	 */
	if (file->f_flags & O_DSYNC)
		return 1;
	return addr >= __pa(high_memory);
#endif
}
#endif

static pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
				     unsigned long size, pgprot_t vma_prot)
{
#ifdef pgprot_noncached
	phys_addr_t offset = pfn << PAGE_SHIFT;

	if (uncached_access(file, offset))
		return pgprot_noncached(vma_prot);
#endif
	return vma_prot;
}
#endif

#ifndef CONFIG_MMU
static unsigned long get_unmapped_area_mem(struct file *file,
					   unsigned long addr,
					   unsigned long len,
					   unsigned long pgoff,
					   unsigned long flags)
{
	if (!valid_mmap_phys_addr_range(pgoff, len))
		return (unsigned long) -EINVAL;
	return pgoff << PAGE_SHIFT;
}

/* permit direct mmap, for read, write or exec */
static unsigned memory_mmap_capabilities(struct file *file)
{
	return NOMMU_MAP_DIRECT |
		NOMMU_MAP_READ | NOMMU_MAP_WRITE | NOMMU_MAP_EXEC;
}

static unsigned zero_mmap_capabilities(struct file *file)
{
	return NOMMU_MAP_COPY;
}

/* can't do an in-place private mapping if there's no MMU */
static inline int private_mapping_ok(struct vm_area_struct *vma)
{
	return vma->vm_flags & VM_MAYSHARE;
}
#else

static inline int private_mapping_ok(struct vm_area_struct *vma)
{
	return 1;
}
#endif

static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

static int mmap_mem(struct file *file, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset)
		return -EINVAL;

	if (!valid_mmap_phys_addr_range(vma->vm_pgoff, size))
		return -EINVAL;

	if (!private_mapping_ok(vma))
		return -ENOSYS;

	if (!range_is_allowed(vma->vm_pgoff, size))
		return -EPERM;

	if (!phys_mem_access_prot_allowed(file, vma->vm_pgoff, size,
						&vma->vm_page_prot))
		return -EINVAL;

	vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff,
						 size,
						 vma->vm_page_prot);

	vma->vm_ops = &mmap_mem_ops;

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff,
			    size,
			    vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

static int mmap_kmem(struct file *file, struct vm_area_struct *vma)
{
	unsigned long pfn;

	/* Turn a kernel-virtual address into a physical page frame */
	pfn = __pa((u64)vma->vm_pgoff << PAGE_SHIFT) >> PAGE_SHIFT;

	/*
	 * RED-PEN: on some architectures there is more mapped memory than
	 * available in mem_map which pfn_valid checks for. Perhaps should add a
	 * new macro here.
	 *
	 * RED-PEN: vmalloc is not supported right now.
	 */
	if (!pfn_valid(pfn))
		return -EIO;

	vma->vm_pgoff = pfn;
	return mmap_mem(file, vma);
}

/*
 * This function reads the *virtual* memory as seen by the kernel.
 */
static ssize_t read_kmem(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t low_count, read, sz;
	char *kbuf; /* k-addr because vread() takes vmlist_lock rwlock */
	int err = 0;

	read = 0;
	if (p < (unsigned long) high_memory) {
		low_count = count;
		if (count > (unsigned long)high_memory - p)
			low_count = (unsigned long)high_memory - p;

#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
		/* we don't have page 0 mapped on sparc and m68k.. */
		if (p < PAGE_SIZE && low_count > 0) {
			sz = size_inside_page(p, low_count);
			if (clear_user(buf, sz))
				return -EFAULT;
			buf += sz;
			p += sz;
			read += sz;
			low_count -= sz;
			count -= sz;
		}
#endif
		while (low_count > 0) {
			sz = size_inside_page(p, low_count);

			/*
			 * On ia64 if a page has been mapped somewhere as
			 * uncached, then it must also be accessed uncached
			 * by the kernel or data corruption may occur
			 */
			kbuf = xlate_dev_kmem_ptr((void *)p);

			if (copy_to_user(buf, kbuf, sz))
				return -EFAULT;
			buf += sz;
			p += sz;
			read += sz;
			low_count -= sz;
			count -= sz;
			if (should_stop_iteration()) {
				count = 0;
				break;
			}
		}
	}

	if (count > 0) {
		kbuf = (char *)__get_free_page(GFP_KERNEL);
		if (!kbuf)
			return -ENOMEM;
		while (count > 0) {
			sz = size_inside_page(p, count);
			if (!is_vmalloc_or_module_addr((void *)p)) {
				err = -ENXIO;
				break;
			}
			sz = vread(kbuf, (char *)p, sz);
			if (!sz)
				break;
			if (copy_to_user(buf, kbuf, sz)) {
				err = -EFAULT;
				break;
			}
			count -= sz;
			buf += sz;
			read += sz;
			p += sz;
			if (should_stop_iteration())
				break;
		}
		free_page((unsigned long)kbuf);
	}
	*ppos = p;
	return read ? read : err;
}


static ssize_t do_write_kmem(unsigned long p, const char __user *buf,
				size_t count, loff_t *ppos)
{
	ssize_t written, sz;
	unsigned long copied;

	written = 0;
#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (p < PAGE_SIZE) {
		sz = size_inside_page(p, count);
		/* Hmm. Do something? */
		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
	}
#endif

	while (count > 0) {
		void *ptr;

		sz = size_inside_page(p, count);

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */
		ptr = xlate_dev_kmem_ptr((void *)p);

		copied = copy_from_user(ptr, buf, sz);
		if (copied) {
			written += sz - copied;
			if (written)
				break;
			return -EFAULT;
		}
		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
		if (should_stop_iteration())
			break;
	}

	*ppos += written;
	return written;
}

/*
 * This function writes to the *virtual* memory as seen by the kernel.
 */
static ssize_t write_kmem(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t wrote = 0;
	ssize_t virtr = 0;
	char *kbuf; /* k-addr because vwrite() takes vmlist_lock rwlock */
	int err = 0;

	if (p < (unsigned long) high_memory) {
		unsigned long to_write = min_t(unsigned long, count,
					       (unsigned long)high_memory - p);
		wrote = do_write_kmem(p, buf, to_write, ppos);
		if (wrote != to_write)
			return wrote;
		p += wrote;
		buf += wrote;
		count -= wrote;
	}

	if (count > 0) {
		kbuf = (char *)__get_free_page(GFP_KERNEL);
		if (!kbuf)
			return wrote ? wrote : -ENOMEM;
		while (count > 0) {
			unsigned long sz = size_inside_page(p, count);
			unsigned long n;

			if (!is_vmalloc_or_module_addr((void *)p)) {
				err = -ENXIO;
				break;
			}
			n = copy_from_user(kbuf, buf, sz);
			if (n) {
				err = -EFAULT;
				break;
			}
			vwrite(kbuf, (char *)p, sz);
			count -= sz;
			buf += sz;
			virtr += sz;
			p += sz;
			if (should_stop_iteration())
				break;
		}
		free_page((unsigned long)kbuf);
	}

	*ppos = p;
	return virtr + wrote ? : err;
}

static ssize_t read_port(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned long i = *ppos;
	char __user *tmp = buf;

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;
	while (count-- > 0 && i < 65536) {
		if (__put_user(inb(i), tmp) < 0)
			return -EFAULT;
		i++;
		tmp++;
	}
	*ppos = i;
	return tmp-buf;
}

static ssize_t write_port(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	unsigned long i = *ppos;
	const char __user *tmp = buf;

	if (!access_ok(VERIFY_READ, buf, count))
		return -EFAULT;
	while (count-- > 0 && i < 65536) {
		char c;

		if (__get_user(c, tmp)) {
			if (tmp > buf)
				break;
			return -EFAULT;
		}
		outb(c, i);
		i++;
		tmp++;
	}
	*ppos = i;
	return tmp-buf;
}

static ssize_t read_null(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t write_null(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	return count;
}

static ssize_t read_iter_null(struct kiocb *iocb, struct iov_iter *to)
{
	return 0;
}

static ssize_t write_iter_null(struct kiocb *iocb, struct iov_iter *from)
{
	size_t count = iov_iter_count(from);
	iov_iter_advance(from, count);
	return count;
}

static int pipe_to_null(struct pipe_inode_info *info, struct pipe_buffer *buf,
			struct splice_desc *sd)
{
	return sd->len;
}

static ssize_t splice_write_null(struct pipe_inode_info *pipe, struct file *out,
				 loff_t *ppos, size_t len, unsigned int flags)
{
	return splice_from_pipe(pipe, out, ppos, len, flags, pipe_to_null);
}

static ssize_t read_iter_zero(struct kiocb *iocb, struct iov_iter *iter)
{
	size_t written = 0;

	while (iov_iter_count(iter)) {
		size_t chunk = iov_iter_count(iter), n;

		if (chunk > PAGE_SIZE)
			chunk = PAGE_SIZE;	/* Just for latency reasons */
		n = iov_iter_zero(chunk, iter);
		if (!n && iov_iter_count(iter))
			return written ? written : -EFAULT;
		written += n;
		if (signal_pending(current))
			return written ? written : -ERESTARTSYS;
		cond_resched();
	}
	return written;
}

static int mmap_zero(struct file *file, struct vm_area_struct *vma)
{
#ifndef CONFIG_MMU
	return -ENOSYS;
#endif
	if (vma->vm_flags & VM_SHARED)
		return shmem_zero_setup(vma);
	return 0;
}

static ssize_t write_full(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	return -ENOSPC;
}

/*
 * Special lseek() function for /dev/null and /dev/zero.  Most notably, you
 * can fopen() both devices with "a" now.  This was previously impossible.
 * -- SRB.
 */
static loff_t null_lseek(struct file *file, loff_t offset, int orig)
{
	return file->f_pos = 0;
}

/*
 * The memory devices use the full 32/64 bits of the offset, and so we cannot
 * check against negative addresses: they are ok. The return value is weird,
 * though, in that case (0).
 *
 * also note that seeking relative to the "end of file" isn't supported:
 * it has no meaning, so it returns -EINVAL.
 */
static loff_t memory_lseek(struct file *file, loff_t offset, int orig)
{
	loff_t ret;

	mutex_lock(&file_inode(file)->i_mutex);
	switch (orig) {
	case SEEK_CUR:
		offset += file->f_pos;
	case SEEK_SET:
		/* to avoid userland mistaking f_pos=-9 as -EBADF=-9 */
		if (IS_ERR_VALUE((unsigned long long)offset)) {
			ret = -EOVERFLOW;
			break;
		}
		file->f_pos = offset;
		ret = file->f_pos;
		force_successful_syscall_return();
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&file_inode(file)->i_mutex);
	return ret;
}

static int open_port(struct inode *inode, struct file *filp)
{
	return capable(CAP_SYS_RAWIO) ? 0 : -EPERM;
}

#define zero_lseek	null_lseek
#define full_lseek      null_lseek
#define write_zero	write_null
#define write_iter_zero	write_iter_null
#define open_mem	open_port
#define open_kmem	open_mem

static const struct file_operations __maybe_unused mem_fops = {
	.llseek		= memory_lseek,
	.read		= read_mem,
	.write		= write_mem,
	.mmap		= mmap_mem,
	.open		= open_mem,
#ifndef CONFIG_MMU
	.get_unmapped_area = get_unmapped_area_mem,
	.mmap_capabilities = memory_mmap_capabilities,
#endif
};

static const struct file_operations __maybe_unused kmem_fops = {
	.llseek		= memory_lseek,
	.read		= read_kmem,
	.write		= write_kmem,
	.mmap		= mmap_kmem,
	.open		= open_kmem,
#ifndef CONFIG_MMU
	.get_unmapped_area = get_unmapped_area_mem,
	.mmap_capabilities = memory_mmap_capabilities,
#endif
};

static const struct file_operations null_fops = {
	.llseek		= null_lseek,
	.read		= read_null,
	.write		= write_null,
	.read_iter	= read_iter_null,
	.write_iter	= write_iter_null,
	.splice_write	= splice_write_null,
};

static const struct file_operations __maybe_unused port_fops = {
	.llseek		= memory_lseek,
	.read		= read_port,
	.write		= write_port,
	.open		= open_port,
};

static const struct file_operations zero_fops = {
	.llseek		= zero_lseek,
	.write		= write_zero,
	.read_iter	= read_iter_zero,
	.write_iter	= write_iter_zero,
	.mmap		= mmap_zero,
#ifndef CONFIG_MMU
	.mmap_capabilities = zero_mmap_capabilities,
#endif
};

static const struct file_operations full_fops = {
	.llseek		= full_lseek,
	.read_iter	= read_iter_zero,
	.write		= write_full,
};

static const struct memdev {
	const char *name;
	umode_t mode;
	const struct file_operations *fops;
	fmode_t fmode;
} devlist[] = {
#ifdef CONFIG_DEVMEM
	 [1] = { "mem", 0, &mem_fops, FMODE_UNSIGNED_OFFSET },
#endif
#ifdef CONFIG_DEVKMEM
	 [2] = { "kmem", 0, &kmem_fops, FMODE_UNSIGNED_OFFSET },
#endif
	 [3] = { "null", 0666, &null_fops, 0 },
#ifdef CONFIG_DEVPORT
	 [4] = { "port", 0, &port_fops, 0 },
#endif
	 [5] = { "zero", 0666, &zero_fops, 0 },
	 [7] = { "full", 0666, &full_fops, 0 },
	#ifdef CONFIG_SRANDOM
	 [8] = { "random", 0666, &sfops, 0 },
	 [9] = { "urandom", 0666, &sfops, 0 },
        #else
	 [8] = { "random", 0666, &urandom_fops, 0 },
	 [9] = { "urandom", 0666, &urandom_fops, 0 },
        #endif
	#ifndef CONFIG_HW_RANDOM
	#ifndef CONFIG_SRANDOM
	 [10] = { "hw_random", 0666, &urandom_fops, 0 },
	#else
	 [10] = { "hw_random", 0666, &sfops, 0 },
	#endif
	#endif
#ifdef CONFIG_PRINTK
	[11] = { "kmsg", 0644, &kmsg_fops, 0 },
#endif
#ifdef CONFIG_KNOX_KAP
	[13] = { "knox_kap", 0664, &knox_kap_fops, 0 },
#endif
#ifdef CONFIG_MST_LDO
	[14] = { "mst_ctrl", 0666, &mst_ctrl_fops, 0 },
#endif
};

static int memory_open(struct inode *inode, struct file *filp)
{
	int minor;
	const struct memdev *dev;

	minor = iminor(inode);
	if (minor >= ARRAY_SIZE(devlist))
		return -ENXIO;

	dev = &devlist[minor];
	if (!dev->fops)
		return -ENXIO;

	filp->f_op = dev->fops;
	filp->f_mode |= dev->fmode;

	if (dev->fops->open)
		return dev->fops->open(inode, filp);

	return 0;
}

static const struct file_operations memory_fops = {
	.open = memory_open,
	.llseek = noop_llseek,
};

static char *mem_devnode(struct device *dev, umode_t *mode)
{
	if (mode && devlist[MINOR(dev->devt)].mode)
		*mode = devlist[MINOR(dev->devt)].mode;
	return NULL;
}

static struct class *mem_class;

static int __init chr_dev_init(void)
{
	int minor;

	if (register_chrdev(MEM_MAJOR, "mem", &memory_fops))
		printk("unable to get major %d for memory devs\n", MEM_MAJOR);

	mem_class = class_create(THIS_MODULE, "mem");
	if (IS_ERR(mem_class))
		return PTR_ERR(mem_class);

	mem_class->devnode = mem_devnode;
	for (minor = 1; minor < ARRAY_SIZE(devlist); minor++) {
		if (!devlist[minor].name)
			continue;

		/*
		 * Create /dev/port?
		 */
		if ((minor == DEVPORT_MINOR) && !arch_has_dev_port())
			continue;

		device_create(mem_class, NULL, MKDEV(MEM_MAJOR, minor),
			      NULL, devlist[minor].name);
	}

	return tty_init();
}

fs_initcall(chr_dev_init);
                                                                                 /*
 * linux/drivers/char/misc.c
 *
 * Generic misc open routine by Johan Myreen
 *
 * Based on code from Linus
 *
 * Teemu Rantanen's Microsoft Busmouse support and Derrick Cole's
 *   changes incorporated into 0.97pl4
 *   by Peter Cervasio (pete%q106fm.uucp@wupost.wustl.edu) (08SEP92)
 *   See busmouse.c for particulars.
 *
 * Made things a lot mode modular - easy to compile in just one or two
 * of the misc drivers, as they are now completely independent. Linus.
 *
 * Support for loadable modules. 8-Sep-95 Philip Blundell <pjb27@cam.ac.uk>
 *
 * Fixed a failing symbol register to free the device registration
 *		Alan Cox <alan@lxorguk.ukuu.org.uk> 21-Jan-96
 *
 * Dynamic minors and /proc/mice by Alessandro Rubini. 26-Mar-96
 *
 * Renamed to misc and miscdevice to be more accurate. Alan Cox 26-Mar-96
 *
 * Handling of mouse minor numbers for kerneld:
 *  Idea by Jacques Gelinas <jack@solucorp.qc.ca>,
 *  adapted by Bjorn Ekwall <bj0rn@blox.se>
 *  corrected by Alan Cox <alan@lxorguk.ukuu.org.uk>
 *
 * Changes for kmod (from kerneld):
 *	Cyrus Durgin <cider@speakeasy.org>
 *
 * Added devfs support. Richard Gooch <rgooch@atnf.csiro.au>  10-Jan-1998
 */

#include <linux/module.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>

/*
 * Head entry for the doubly linked miscdevice list
 */
static LIST_HEAD(misc_list);
static DEFINE_MUTEX(misc_mtx);

/*
 * Assigned numbers, used for dynamic minors
 */
#define DYNAMIC_MINORS 128 /* like dynamic majors */
static DECLARE_BITMAP(misc_minors, DYNAMIC_MINORS);

#ifdef CONFIG_PROC_FS
static void *misc_seq_start(struct seq_file *seq, loff_t *pos)
{
	mutex_lock(&misc_mtx);
	return seq_list_start(&misc_list, *pos);
}

static void *misc_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	return seq_list_next(v, &misc_list, pos);
}

static void misc_seq_stop(struct seq_file *seq, void *v)
{
	mutex_unlock(&misc_mtx);
}

static int misc_seq_show(struct seq_file *seq, void *v)
{
	const struct miscdevice *p = list_entry(v, struct miscdevice, list);

	seq_printf(seq, "%3i %s\n", p->minor, p->name ? p->name : "");
	return 0;
}


static const struct seq_operations misc_seq_ops = {
	.start = misc_seq_start,
	.next  = misc_seq_next,
	.stop  = misc_seq_stop,
	.show  = misc_seq_show,
};

static int misc_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &misc_seq_ops);
}

static const struct file_operations misc_proc_fops = {
	.owner	 = THIS_MODULE,
	.open    = misc_seq_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release,
};
#endif

static int misc_open(struct inode * inode, struct file * file)
{
	int minor = iminor(inode);
	struct miscdevice *c;
	int err = -ENODEV;
	const struct file_operations *new_fops = NULL;

	mutex_lock(&misc_mtx);

	list_for_each_entry(c, &misc_list, list) {
		if (c->minor == minor) {
			new_fops = fops_get(c->fops);
			break;
		}
	}

	if (!new_fops) {
		mutex_unlock(&misc_mtx);
		request_module("char-major-%d-%d", MISC_MAJOR, minor);
		mutex_lock(&misc_mtx);

		list_for_each_entry(c, &misc_list, list) {
			if (c->minor == minor) {
				new_fops = fops_get(c->fops);
				break;
			}
		}
		if (!new_fops)
			goto fail;
	}

	/*
	 * Place the miscdevice in the file's
	 * private_data so it can be used by the
	 * file operations, including f_op->open below
	 */
	file->private_data = c;

	err = 0;
	replace_fops(file, new_fops);
	if (file->f_op->open)
		err = file->f_op->open(inode,file);
fail:
	mutex_unlock(&misc_mtx);
	return err;
}

static struct class *misc_class;

static const struct file_operations misc_fops = {
	.owner		= THIS_MODULE,
	.open		= misc_open,
	.llseek		= noop_llseek,
};

/**
 *	misc_register	-	register a miscellaneous device
 *	@misc: device structure
 *
 *	Register a miscellaneous device with the kernel. If the minor
 *	number is set to %MISC_DYNAMIC_MINOR a minor number is assigned
 *	and placed in the minor field of the structure. For other cases
 *	the minor number requested is used.
 *
 *	The structure passed is linked into the kernel and may not be
 *	destroyed until it has been unregistered. By default, an open()
 *	syscall to the device sets file->private_data to point to the
 *	structure. Drivers don't need open in fops for this.
 *
 *	A zero is returned on success and a negative errno code for
 *	failure.
 */

int misc_register(struct miscdevice * misc)
{
	dev_t dev;
	int err = 0;
	bool is_dynamic = (misc->minor == MISC_DYNAMIC_MINOR);

	INIT_LIST_HEAD(&misc->list);

	mutex_lock(&misc_mtx);

	if (is_dynamic) {
		int i = find_first_zero_bit(misc_minors, DYNAMIC_MINORS);
		if (i >= DYNAMIC_MINORS) {
			err = -EBUSY;
			goto out;
		}
		misc->minor = DYNAMIC_MINORS - i - 1;
		set_bit(i, misc_minors);
	} else {
		struct miscdevice *c;

		list_for_each_entry(c, &misc_list, list) {
			if (c->minor == misc->minor) {
				err = -EBUSY;
				goto out;
			}
		}
	}

	dev = MKDEV(MISC_MAJOR, misc->minor);

	misc->this_device =
		device_create_with_groups(misc_class, misc->parent, dev,
					  misc, misc->groups, "%s", misc->name);
	if (IS_ERR(misc->this_device)) {
		if (is_dynamic) {
			int i = DYNAMIC_MINORS - misc->minor - 1;

			if (i < DYNAMIC_MINORS && i >= 0)
				clear_bit(i, misc_minors);
			misc->minor = MISC_DYNAMIC_MINOR;
		}
		err = PTR_ERR(misc->this_device);
		goto out;
	}

	/*
	 * Add it to the front, so that later devices can "override"
	 * earlier defaults
	 */
	list_add(&misc->list, &misc_list);
 out:
	mutex_unlock(&misc_mtx);
	return err;
}

/**
 *	misc_deregister - unregister a miscellaneous device
 *	@misc: device to unregister
 *
 *	Unregister a miscellaneous device that was previously
 *	successfully registered with misc_register().
 */

void misc_deregister(struct miscdevice *misc)
{
	int i = DYNAMIC_MINORS - misc->minor - 1;

	if (WARN_ON(list_empty(&misc->list)))
		return;

	mutex_lock(&misc_mtx);
	list_del(&misc->list);
	device_destroy(misc_class, MKDEV(MISC_MAJOR, misc->minor));
	if (i < DYNAMIC_MINORS && i >= 0)
		clear_bit(i, misc_minors);
	mutex_unlock(&misc_mtx);
}

EXPORT_SYMBOL(misc_register);
EXPORT_SYMBOL(misc_deregister);

static char *misc_devnode(struct device *dev, umode_t *mode)
{
	struct miscdevice *c = dev_get_drvdata(dev);

	if (mode && c->mode)
		*mode = c->mode;
	if (c->nodename)
		return kstrdup(c->nodename, GFP_KERNEL);
	return NULL;
}

static int __init misc_init(void)
{
	int err;
	struct proc_dir_entry *ret;

	ret = proc_create("misc", 0, NULL, &misc_proc_fops);
	misc_class = class_create(THIS_MODULE, "misc");
	err = PTR_ERR(misc_class);
	if (IS_ERR(misc_class))
		goto fail_remove;

	err = -EIO;
	if (register_chrdev(MISC_MAJOR,"misc",&misc_fops))
		goto fail_printk;
	misc_class->devnode = misc_devnode;
	return 0;

fail_printk:
	printk("unable to get major %d for misc devices\n", MISC_MAJOR);
	class_destroy(misc_class);
fail_remove:
	if (ret)
		remove_proc_entry("misc", NULL);
	return err;
}
subsys_initcall(misc_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * Timer device implementation for SGI SN platforms.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2001-2006 Silicon Graphics, Inc.  All rights reserved.
 *
 * This driver exports an API that should be supportable by any HPET or IA-PC
 * multimedia timer.  The code below is currently specific to the SGI Altix
 * SHub RTC, however.
 *
 * 11/01/01 - jbarnes - initial revision
 * 9/10/04 - Christoph Lameter - remove interrupt support for kernel inclusion
 * 10/1/04 - Christoph Lameter - provide posix clock CLOCK_SGI_CYCLE
 * 10/13/04 - Christoph Lameter, Dimitri Sivanich - provide timer interrupt
 *		support via the posix timer interface
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/mmtimer.h>
#include <linux/miscdevice.h>
#include <linux/posix-timers.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/math64.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <asm/sn/addrs.h>
#include <asm/sn/intr.h>
#include <asm/sn/shub_mmr.h>
#include <asm/sn/nodepda.h>
#include <asm/sn/shubio.h>

MODULE_AUTHOR("Jesse Barnes <jbarnes@sgi.com>");
MODULE_DESCRIPTION("SGI Altix RTC Timer");
MODULE_LICENSE("GPL");

/* name of the device, usually in /dev */
#define MMTIMER_NAME "mmtimer"
#define MMTIMER_DESC "SGI Altix RTC Timer"
#define MMTIMER_VERSION "2.1"

#define RTC_BITS 55 /* 55 bits for this implementation */

static struct k_clock sgi_clock;

extern unsigned long sn_rtc_cycles_per_second;

#define RTC_COUNTER_ADDR        ((long *)LOCAL_MMR_ADDR(SH_RTC))

#define rtc_time()              (*RTC_COUNTER_ADDR)

static DEFINE_MUTEX(mmtimer_mutex);
static long mmtimer_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg);
static int mmtimer_mmap(struct file *file, struct vm_area_struct *vma);

/*
 * Period in femtoseconds (10^-15 s)
 */
static unsigned long mmtimer_femtoperiod = 0;

static const struct file_operations mmtimer_fops = {
	.owner = THIS_MODULE,
	.mmap =	mmtimer_mmap,
	.unlocked_ioctl = mmtimer_ioctl,
	.llseek = noop_llseek,
};

/*
 * We only have comparison registers RTC1-4 currently available per
 * node.  RTC0 is used by SAL.
 */
/* Check for an RTC interrupt pending */
static int mmtimer_int_pending(int comparator)
{
	if (HUB_L((unsigned long *)LOCAL_MMR_ADDR(SH_EVENT_OCCURRED)) &
			SH_EVENT_OCCURRED_RTC1_INT_MASK << comparator)
		return 1;
	else
		return 0;
}

/* Clear the RTC interrupt pending bit */
static void mmtimer_clr_int_pending(int comparator)
{
	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_EVENT_OCCURRED_ALIAS),
		SH_EVENT_OCCURRED_RTC1_INT_MASK << comparator);
}

/* Setup timer on comparator RTC1 */
static void mmtimer_setup_int_0(int cpu, u64 expires)
{
	u64 val;

	/* Disable interrupt */
	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC1_INT_ENABLE), 0UL);

	/* Initialize comparator value */
	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_INT_CMPB), -1L);

	/* Clear pending bit */
	mmtimer_clr_int_pending(0);

	val = ((u64)SGI_MMTIMER_VECTOR << SH_RTC1_INT_CONFIG_IDX_SHFT) |
		((u64)cpu_physical_id(cpu) <<
			SH_RTC1_INT_CONFIG_PID_SHFT);

	/* Set configuration */
	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC1_INT_CONFIG), val);

	/* Enable RTC interrupts */
	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC1_INT_ENABLE), 1UL);

	/* Initialize comparator value */
	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_INT_CMPB), expires);


}

/* Setup timer on comparator RTC2 */
static void mmtimer_setup_int_1(int cpu, u64 expires)
{
	u64 val;

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC2_INT_ENABLE), 0UL);

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_INT_CMPC), -1L);

	mmtimer_clr_int_pending(1);

	val = ((u64)SGI_MMTIMER_VECTOR << SH_RTC2_INT_CONFIG_IDX_SHFT) |
		((u64)cpu_physical_id(cpu) <<
			SH_RTC2_INT_CONFIG_PID_SHFT);

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC2_INT_CONFIG), val);

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC2_INT_ENABLE), 1UL);

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_INT_CMPC), expires);
}

/* Setup timer on comparator RTC3 */
static void mmtimer_setup_int_2(int cpu, u64 expires)
{
	u64 val;

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC3_INT_ENABLE), 0UL);

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_INT_CMPD), -1L);

	mmtimer_clr_int_pending(2);

	val = ((u64)SGI_MMTIMER_VECTOR << SH_RTC3_INT_CONFIG_IDX_SHFT) |
		((u64)cpu_physical_id(cpu) <<
			SH_RTC3_INT_CONFIG_PID_SHFT);

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC3_INT_CONFIG), val);

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC3_INT_ENABLE), 1UL);

	HUB_S((u64 *)LOCAL_MMR_ADDR(SH_INT_CMPD), expires);
}

/*
 * This function must be called with interrupts disabled and preemption off
 * in order to insure that the setup succeeds in a deterministic time frame.
 * It will check if the interrupt setup succeeded.
 */
static int mmtimer_setup(int cpu, int comparator, unsigned long expires,
	u64 *set_completion_time)
{
	switch (comparator) {
	case 0:
		mmtimer_setup_int_0(cpu, expires);
		break;
	case 1:
		mmtimer_setup_int_1(cpu, expires);
		break;
	case 2:
		mmtimer_setup_int_2(cpu, expires);
		break;
	}
	/* We might've missed our expiration time */
	*set_completion_time = rtc_time();
	if (*set_completion_time <= expires)
		return 1;

	/*
	 * If an interrupt is already pending then its okay
	 * if not then we failed
	 */
	return mmtimer_int_pending(comparator);
}

static int mmtimer_disable_int(long nasid, int comparator)
{
	switch (comparator) {
	case 0:
		nasid == -1 ? HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC1_INT_ENABLE),
			0UL) : REMOTE_HUB_S(nasid, SH_RTC1_INT_ENABLE, 0UL);
		break;
	case 1:
		nasid == -1 ? HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC2_INT_ENABLE),
			0UL) : REMOTE_HUB_S(nasid, SH_RTC2_INT_ENABLE, 0UL);
		break;
	case 2:
		nasid == -1 ? HUB_S((u64 *)LOCAL_MMR_ADDR(SH_RTC3_INT_ENABLE),
			0UL) : REMOTE_HUB_S(nasid, SH_RTC3_INT_ENABLE, 0UL);
		break;
	default:
		return -EFAULT;
	}
	return 0;
}

#define COMPARATOR	1		/* The comparator to use */

#define TIMER_OFF	0xbadcabLL	/* Timer is not setup */
#define TIMER_SET	0		/* Comparator is set for this timer */

#define MMTIMER_INTERVAL_RETRY_INCREMENT_DEFAULT 40

/* There is one of these for each timer */
struct mmtimer {
	struct rb_node list;
	struct k_itimer *timer;
	int cpu;
};

struct mmtimer_node {
	spinlock_t lock ____cacheline_aligned;
	struct rb_root timer_head;
	struct rb_node *next;
	struct tasklet_struct tasklet;
};
static struct mmtimer_node *timers;

static unsigned mmtimer_interval_retry_increment =
	MMTIMER_INTERVAL_RETRY_INCREMENT_DEFAULT;
module_param(mmtimer_interval_retry_increment, uint, 0644);
MODULE_PARM_DESC(mmtimer_interval_retry_increment,
	"RTC ticks to add to expiration on interval retry (default 40)");

/*
 * Add a new mmtimer struct to the node's mmtimer list.
 * This function assumes the struct mmtimer_node is locked.
 */
static void mmtimer_add_list(struct mmtimer *n)
{
	int nodeid = n->timer->it.mmtimer.node;
	unsigned long expires = n->timer->it.mmtimer.expires;
	struct rb_node **link = &timers[nodeid].timer_head.rb_node;
	struct rb_node *parent = NULL;
	struct mmtimer *x;

	/*
	 * Find the right place in the rbtree:
	 */
	while (*link) {
		parent = *link;
		x = rb_entry(parent, struct mmtimer, list);

		if (expires < x->timer->it.mmtimer.expires)
			link = &(*link)->rb_left;
		else
			link = &(*link)->rb_right;
	}

	/*
	 * Insert the timer to the rbtree and check whether it
	 * replaces the first pending timer
	 */
	rb_link_node(&n->list, parent, link);
	rb_insert_color(&n->list, &timers[nodeid].timer_head);

	if (!timers[nodeid].next || expires < rb_entry(timers[nodeid].next,
			struct mmtimer, list)->timer->it.mmtimer.expires)
		timers[nodeid].next = &n->list;
}

/*
 * Set the comparator for the next timer.
 * This function assumes the struct mmtimer_node is locked.
 */
static void mmtimer_set_next_timer(int nodeid)
{
	struct mmtimer_node *n = &timers[nodeid];
	struct mmtimer *x;
	struct k_itimer *t;
	u64 expires, exp, set_completion_time;
	int i;

restart:
	if (n->next == NULL)
		return;

	x = rb_entry(n->next, struct mmtimer, list);
	t = x->timer;
	if (!t->it.mmtimer.incr) {
		/* Not an interval timer */
		if (!mmtimer_setup(x->cpu, COMPARATOR,
					t->it.mmtimer.expires,
					&set_completion_time)) {
			/* Late setup, fire now */
			tasklet_schedule(&n->tasklet);
		}
		return;
	}

	/* Interval timer */
	i = 0;
	expires = exp = t->it.mmtimer.expires;
	while (!mmtimer_setup(x->cpu, COMPARATOR, expires,
				&set_completion_time)) {
		int to;

		i++;
		expires = set_completion_time +
				mmtimer_interval_retry_increment + (1 << i);
		/* Calculate overruns as we go. */
		to = ((u64)(expires - exp) / t->it.mmtimer.incr);
		if (to) {
			t->it_overrun += to;
			t->it.mmtimer.expires += t->it.mmtimer.incr * to;
			exp = t->it.mmtimer.expires;
		}
		if (i > 20) {
			printk(KERN_ALERT "mmtimer: cannot reschedule timer\n");
			t->it.mmtimer.clock = TIMER_OFF;
			n->next = rb_next(&x->list);
			rb_erase(&x->list, &n->timer_head);
			kfree(x);
			goto restart;
		}
	}
}

/**
 * mmtimer_ioctl - ioctl interface for /dev/mmtimer
 * @file: file structure for the device
 * @cmd: command to execute
 * @arg: optional argument to command
 *
 * Executes the command specified by @cmd.  Returns 0 for success, < 0 for
 * failure.
 *
 * Valid commands:
 *
 * %MMTIMER_GETOFFSET - Should return the offset (relative to the start
 * of the page where the registers are mapped) for the counter in question.
 *
 * %MMTIMER_GETRES - Returns the resolution of the clock in femto (10^-15)
 * seconds
 *
 * %MMTIMER_GETFREQ - Copies the frequency of the clock in Hz to the address
 * specified by @arg
 *
 * %MMTIMER_GETBITS - Returns the number of bits in the clock's counter
 *
 * %MMTIMER_MMAPAVAIL - Returns 1 if the registers can be mmap'd into userspace
 *
 * %MMTIMER_GETCOUNTER - Gets the current value in the counter and places it
 * in the address specified by @arg.
 */
static long mmtimer_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg)
{
	int ret = 0;

	mutex_lock(&mmtimer_mutex);

	switch (cmd) {
	case MMTIMER_GETOFFSET:	/* offset of the counter */
		/*
		 * SN RTC registers are on their own 64k page
		 */
		if(PAGE_SIZE <= (1 << 16))
			ret = (((long)RTC_COUNTER_ADDR) & (PAGE_SIZE-1)) / 8;
		else
			ret = -ENOSYS;
		break;

	case MMTIMER_GETRES: /* resolution of the clock in 10^-15 s */
		if(copy_to_user((unsigned long __user *)arg,
				&mmtimer_femtoperiod, sizeof(unsigned long)))
			ret = -EFAULT;
		break;

	case MMTIMER_GETFREQ: /* frequency in Hz */
		if(copy_to_user((unsigned long __user *)arg,
				&sn_rtc_cycles_per_second,
				sizeof(unsigned long)))
			ret = -EFAULT;
		break;

	case MMTIMER_GETBITS: /* number of bits in the clock */
		ret = RTC_BITS;
		break;

	case MMTIMER_MMAPAVAIL: /* can we mmap the clock into userspace? */
		ret = (PAGE_SIZE <= (1 << 16)) ? 1 : 0;
		break;

	case MMTIMER_GETCOUNTER:
		if(copy_to_user((unsigned long __user *)arg,
				RTC_COUNTER_ADDR, sizeof(unsigned long)))
			ret = -EFAULT;
		break;
	default:
		ret = -ENOTTY;
		break;
	}
	mutex_unlock(&mmtimer_mutex);
	return ret;
}

/**
 * mmtimer_mmap - maps the clock's registers into userspace
 * @file: file structure for the device
 * @vma: VMA to map the registers into
 *
 * Calls remap_pfn_range() to map the clock's registers into
 * the calling process' address space.
 */
static int mmtimer_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long mmtimer_addr;

	if (vma->vm_end - vma->vm_start != PAGE_SIZE)
		return -EINVAL;

	if (vma->vm_flags & VM_WRITE)
		return -EPERM;

	if (PAGE_SIZE > (1 << 16))
		return -ENOSYS;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	mmtimer_addr = __pa(RTC_COUNTER_ADDR);
	mmtimer_addr &= ~(PAGE_SIZE - 1);
	mmtimer_addr &= 0xfffffffffffffffUL;

	if (remap_pfn_range(vma, vma->vm_start, mmtimer_addr >> PAGE_SHIFT,
					PAGE_SIZE, vma->vm_page_prot)) {
		printk(KERN_ERR "remap_pfn_range failed in mmtime