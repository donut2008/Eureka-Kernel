
obj-$(CONFIG_GOOGLE_SMI)		+= gsmi.o
obj-$(CONFIG_GOOGLE_MEMCONSOLE)		+= memconsole.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*
 * Copyright 2010 Google Inc. All Rights Reserved.
 * Author: dlaurie@google.com (Duncan Laurie)
 *
 * Re-worked to expose sysfs APIs by mikew@google.com (Mike Waychison)
 *
 * EFI SMI interface for Google platforms
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/dmi.h>
#include <linux/kdebug.h>
#include <linux/reboot.h>
#include <linux/efi.h>
#include <linux/module.h>
#include <linux/ucs2_string.h>

#define GSMI_SHUTDOWN_CLEAN	0	/* Clean Shutdown */
/* TODO(mikew@google.com): Tie in HARDLOCKUP_DETECTOR with NMIWDT */
#define GSMI_SHUTDOWN_NMIWDT	1	/* NMI Watchdog */
#define GSMI_SHUTDOWN_PANIC	2	/* Panic */
#define GSMI_SHUTDOWN_OOPS	3	/* Oops */
#define GSMI_SHUTDOWN_DIE	4	/* Die -- No longer meaningful */
#define GSMI_SHUTDOWN_MCE	5	/* Machine Check */
#define GSMI_SHUTDOWN_SOFTWDT	6	/* Software Watchdog */
#define GSMI_SHUTDOWN_MBE	7	/* Uncorrected ECC */
#define GSMI_SHUTDOWN_TRIPLE	8	/* Triple Fault */

#define DRIVER_VERSION		"1.0"
#define GSMI_GUID_SIZE		16
#define GSMI_BUF_SIZE		1024
#define GSMI_BUF_ALIGN		sizeof(u64)
#define GSMI_CALLBACK		0xef

/* SMI return codes */
#define GSMI_SUCCESS		0x00
#define GSMI_UNSUPPORTED2	0x03
#define GSMI_LOG_FULL		0x0b
#define GSMI_VAR_NOT_FOUND	0x0e
#define GSMI_HANDSHAKE_SPIN	0x7d
#define GSMI_HANDSHAKE_CF	0x7e
#define GSMI_HANDSHAKE_NONE	0x7f
#define GSMI_INVALID_PARAMETER	0x82
#define GSMI_UNSUPPORTED	0x83
#define GSMI_BUFFER_TOO_SMALL	0x85
#define GSMI_NOT_READY		0x86
#define GSMI_DEVICE_ERROR	0x87
#define GSMI_NOT_FOUND		0x8e

#define QUIRKY_BOARD_HASH 0x78a30a50

/* Internally used commands passed to the firmware */
#define GSMI_CMD_GET_NVRAM_VAR		0x01
#define GSMI_CMD_GET_NEXT_VAR		0x02
#define GSMI_CMD_SET_NVRAM_VAR		0x03
#define GSMI_CMD_SET_EVENT_LOG		0x08
#define GSMI_CMD_CLEAR_EVENT_LOG	0x09
#define GSMI_CMD_CLEAR_CONFIG		0x20
#define GSMI_CMD_HANDSHAKE_TYPE		0xC1

/* Magic entry type for kernel events */
#define GSMI_LOG_ENTRY_TYPE_KERNEL     0xDEAD

/* SMI buffers must be in 32bit physical address space */
struct gsmi_buf {
	u8 *start;			/* start of buffer */
	size_t length;			/* length of buffer */
	dma_addr_t handle;		/* dma allocation handle */
	u32 address;			/* physical address of buffer */
};

struct gsmi_device {
	struct platform_device *pdev;	/* platform device */
	struct gsmi_buf *name_buf;	/* variable name buffer */
	struct gsmi_buf *data_buf;	/* generic data buffer */
	struct gsmi_buf *param_buf;	/* parameter buffer */
	spinlock_t lock;		/* serialize access to SMIs */
	u16 smi_cmd;			/* SMI command port */
	int handshake_type;		/* firmware handler interlock type */
	struct dma_pool *dma_pool;	/* DMA buffer pool */
} gsmi_dev;

/* Packed structures for communicating with the firmware */
struct gsmi_nvram_var_param {
	efi_guid_t	guid;
	u32		name_ptr;
	u32		attributes;
	u32		data_len;
	u32		data_ptr;
} __packed;

struct gsmi_get_next_var_param {
	u8	guid[GSMI_GUID_SIZE];
	u32	name_ptr;
	u32	name_len;
} __packed;

struct gsmi_set_eventlog_param {
	u32	data_ptr;
	u32	data_len;
	u32	type;
} __packed;

/* Event log formats */
struct gsmi_log_entry_type_1 {
	u16	type;
	u32	instance;
} __packed;


/*
 * Some platforms don't have explicit SMI handshake
 * and need to wait for SMI to complete.
 */
#define GSMI_DEFAULT_SPINCOUNT	0x10000
static unsigned int spincount = GSMI_DEFAULT_SPINCOUNT;
module_param(spincount, uint, 0600);
MODULE_PARM_DESC(spincount,
	"The number of loop iterations to use when using the spin handshake.");

static struct gsmi_buf *gsmi_buf_alloc(void)
{
	struct gsmi_buf *smibuf;

	smibuf = kzalloc(sizeof(*smibuf), GFP_KERNEL);
	if (!smibuf) {
		printk(KERN_ERR "gsmi: out of memory\n");
		return NULL;
	}

	/* allocate buffer in 32bit address space */
	smibuf->start = dma_pool_alloc(gsmi_dev.dma_pool, GFP_KERNEL,
				       &smibuf->handle);
	if (!smibuf->start) {
		printk(KERN_ERR "gsmi: failed to allocate name buffer\n");
		kfree(smibuf);
		return NULL;
	}

	/* fill in the buffer handle */
	smibuf->length = GSMI_BUF_SIZE;
	smibuf->address = (u32)virt_to_phys(smibuf->start);

	return smibuf;
}

static void gsmi_buf_free(struct gsmi_buf *smibuf)
{
	if (smibuf) {
		if (smibuf->start)
			dma_pool_free(gsmi_dev.dma_pool, smibuf->start,
				      smibuf->handle);
		kfree(smibuf);
	}
}

/*
 * Make a call to gsmi func(sub).  GSMI error codes are translated to
 * in-kernel errnos (0 on success, -ERRNO on error).
 */
static int gsmi_exec(u8 func, u8 sub)
{
	u16 cmd = (sub << 8) | func;
	u16 result = 0;
	int rc = 0;

	/*
	 * AH  : Subfunction number
	 * AL  : Function number
	 * EBX : Parameter block address
	 * DX  : SMI command port
	 *
	 * Three protocols here. See also the comment in gsmi_init().
	 */
	if (gsmi_dev.handshake_type == GSMI_HANDSHAKE_CF) {
		/*
		 * If handshake_type == HANDSHAKE_CF then set CF on the
		 * way in and wait for the handler to clear it; this avoids
		 * corrupting register state on those chipsets which have
		 * a delay between writing the SMI trigger register and
		 * entering SMM.
		 */
		asm volatile (
			"stc\n"
			"outb %%al, %%dx\n"
		"1:      jc 1b\n"
			: "=a" (result)
			: "0" (cmd),
			  "d" (gsmi_dev.smi_cmd),
			  "b" (gsmi_dev.param_buf->address)
			: "memory", "cc"
		);
	} else if (gsmi_dev.handshake_type == GSMI_HANDSHAKE_SPIN) {
		/*
		 * If handshake_type == HANDSHAKE_SPIN we spin a
		 * hundred-ish usecs to ensure the SMI has triggered.
		 */
		asm volatile (
			"outb %%al, %%dx\n"
		"1:      loop 1b\n"
			: "=a" (result)
			: "0" (cmd),
			  "d" (gsmi_dev.smi_cmd),
			  "b" (gsmi_dev.param_buf->address),
			  "c" (spincount)
			: "memory", "cc"
		);
	} else {
		/*
		 * If handshake_type == HANDSHAKE_NONE we do nothing;
		 * either we don't need to or it's legacy firmware that
		 * doesn't understand the CF protocol.
		 */
		asm volatile (
			"outb %%al, %%dx\n\t"
			: "=a" (result)
			: "0" (cmd),
			  "d" (gsmi_dev.smi_cmd),
			  "b" (gsmi_dev.param_buf->address)
			: "memory", "cc"
		);
	}

	/* check return code from SMI handler */
	switch (result) {
	case GSMI_SUCCESS:
		break;
	case GSMI_VAR_NOT_FOUND:
		/* not really an error, but let the caller know */
		rc = 1;
		break;
	case GSMI_INVALID_PARAMETER:
		printk(KERN_ERR "gsmi: exec 0x%04x: Invalid parameter\n", cmd);
		rc = -EINVAL;
		break;
	case GSMI_BUFFER_TOO_SMALL:
		printk(KERN_ERR "gsmi: exec 0x%04x: Buffer too small\n", cmd);
		rc = -ENOMEM;
		break;
	case GSMI_UNSUPPORTED:
	case GSMI_UNSUPPORTED2:
		if (sub != GSMI_CMD_HANDSHAKE_TYPE)
			printk(KERN_ERR "gsmi: exec 0x%04x: Not supported\n",
			       cmd);
		rc = -ENOSYS;
		break;
	case GSMI_NOT_READY:
		printk(KERN_ERR "gsmi: exec 0x%04x: Not ready\n", cmd);
		rc = -EBUSY;
		break;
	case GSMI_DEVICE_ERROR:
		printk(KERN_ERR "gsmi: exec 0x%04x: Device error\n", cmd);
		rc = -EFAULT;
		break;
	case GSMI_NOT_FOUND:
		printk(KERN_ERR "gsmi: exec 0x%04x: Data not found\n", cmd);
		rc = -ENOENT;
		break;
	case GSMI_LOG_FULL:
		printk(KERN_ERR "gsmi: exec 0x%04x: Log full\n", cmd);
		rc = -ENOSPC;
		break;
	case GSMI_HANDSHAKE_CF:
	case GSMI_HANDSHAKE_SPIN:
	case GSMI_HANDSHAKE_NONE:
		rc = result;
		break;
	default:
		printk(KERN_ERR "gsmi: exec 0x%04x: Unknown error 0x%04x\n",
		       cmd, result);
		rc = -ENXIO;
	}

	return rc;
}

static efi_status_t gsmi_get_variable(efi_char16_t *name,
				      efi_guid_t *vendor, u32 *attr,
				      unsigned long *data_size,
				      void *data)
{
	struct gsmi_nvram_var_param param = {
		.name_ptr = gsmi_dev.name_buf->address,
		.data_ptr = gsmi_dev.data_buf->address,
		.data_len = (u32)*data_size,
	};
	efi_status_t ret = EFI_SUCCESS;
	unsigned long flags;
	size_t name_len = ucs2_strnlen(name, GSMI_BUF_SIZE / 2);
	int rc;

	if (name_len >= GSMI_BUF_SIZE / 2)
		return EFI_BAD_BUFFER_SIZE;

	spin_lock_irqsave(&gsmi_dev.lock, flags);

	/* Vendor guid */
	memcpy(&param.guid, vendor, sizeof(param.guid));

	/* variable name, already in UTF-16 */
	memset(gsmi_dev.name_buf->start, 0, gsmi_dev.name_buf->length);
	memcpy(gsmi_dev.name_buf->start, name, name_len * 2);

	/* data pointer */
	memset(gsmi_dev.data_buf->start, 0, gsmi_dev.data_buf->length);

	/* parameter buffer */
	memset(gsmi_dev.param_buf->start, 0, gsmi_dev.param_buf->length);
	memcpy(gsmi_dev.param_buf->start, &param, sizeof(param));

	rc = gsmi_exec(GSMI_CALLBACK, GSMI_CMD_GET_NVRAM_VAR);
	if (rc < 0) {
		printk(KERN_ERR "gsmi: Get Variable failed\n");
		ret = EFI_LOAD_ERROR;
	} else if (rc == 1) {
		/* variable was not found */
		ret = EFI_NOT_FOUND;
	} else {
		/* Get the arguments back */
		memcpy(&param, gsmi_dev.param_buf->start, sizeof(param));

		/* The size reported is the min of all of our buffers */
		*data_size = min_t(unsigned long, *data_size,
						gsmi_dev.data_buf->length);
		*data_size = min_t(unsigned long, *data_size, param.data_len);

		/* Copy data back to return buffer. */
		memcpy(data, gsmi_dev.data_buf->start, *data_size);

		/* All variables are have the following attributes */
		if (attr)
			*attr = EFI_VARIABLE_NON_VOLATILE |
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS;
	}

	spin_unlock_irqrestore(&gsmi_dev.lock, flags);

	return ret;
}

static efi_status_t gsmi_get_next_variable(unsigned long *name_size,
					   efi_char16_t *name,
					   efi_guid_t *vendor)
{
	struct gsmi_get_next_var_param param = {
		.name_ptr = gsmi_dev.name_buf->address,
		.name_len = gsmi_dev.name_buf->length,
	};
	efi_status_t ret = EFI_SUCCESS;
	int rc;
	unsigned long flags;

	/* For the moment, only support buffers that exactly match in size */
	if (*name_size != GSMI_BUF_SIZE)
		return EFI_BAD_BUFFER_SIZE;

	/* Let's make sure the thing is at least null-terminated */
	if (ucs2_strnlen(name, GSMI_BUF_SIZE / 2) == GSMI_BUF_SIZE / 2)
		return EFI_INVALID_PARAMETER;

	spin_lock_irqsave(&gsmi_dev.lock, flags);

	/* guid */
	memcpy(&param.guid, vendor, sizeof(param.guid));

	/* variable name, already in UTF-16 */
	memcpy(gsmi_dev.name_buf->start, name, *name_size);

	/* parameter buffer */
	memset(gsmi_dev.param_buf->start, 0, gsmi_dev.param_buf->length);
	memcpy(gsmi_dev.param_buf->start, &param, sizeof(param));

	rc = gsmi_exec(GSMI_CALLBACK, GSMI_CMD_GET_NEXT_VAR);
	if (rc < 0) {
		printk(KERN_ERR "gsmi: Get Next Variable Name failed\n");
		ret = EFI_LOAD_ERROR;
	} else if (rc == 1) {
		/* variable not found -- end of list */
		ret = EFI_NOT_FOUND;
	} else {
		/* copy variable data back to return buffer */
		memcpy(&param, gsmi_dev.param_buf->start, sizeof(param));

		/* Copy the name back */
		memcpy(name, gsmi_dev.name_buf->start, GSMI_BUF_SIZE);
		*name_size = ucs2_strnlen(name, GSMI_BUF_SIZE / 2) * 2;

		/* copy guid to return buffer */
		memcpy(vendor, &param.guid