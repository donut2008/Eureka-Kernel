DMA_PRIVATE, mic_dma_dev->dma_dev.cap_mask);
	mic_dma_dev->dma_dev.device_alloc_chan_resources =
		mic_dma_alloc_chan_resources;
	mic_dma_dev->dma_dev.device_free_chan_resources =
		mic_dma_free_chan_resources;
	mic_dma_dev->dma_dev.device_tx_status = mic_dma_tx_status;
	mic_dma_dev->dma_dev.device_prep_dma_memcpy = mic_dma_prep_memcpy_lock;
	mic_dma_dev->dma_dev.device_prep_dma_imm_data =
		mic_dma_prep_status_lock;
	mic_dma_dev->dma_dev.device_prep_dma_interrupt =
		mic_dma_prep_interrupt_lock;
	mic_dma_dev->dma_dev.device_issue_pending = mic_dma_issue_pending;
	mic_dma_dev->dma_dev.copy_align = MIC_DMA_ALIGN_SHIFT;
	INIT_LIST_HEAD(&mic_dma_dev->dma_dev.channels);
	for (i = first_chan; i < first_chan + MIC_DMA_NUM_CHAN; i++) {
		mic_dma_dev->mic_ch[i].api_ch.device = &mic_dma_dev->dma_dev;
		dma_cookie_init(&mic_dma_dev->mic_ch[i].api_ch);
		list_add_tail(&mic_dma_dev->mic_ch[i].api_ch.device_node,
			      &mic_dma_dev->dma_dev.channels);
	}
	return dma_async_device_register(&mic_dma_dev->dma_dev);
}

/*
 * Initializes dma channels and registers the dma device with the
 * dma engine api.
 */
static struct mic_dma_device *mic_dma_dev_reg(struct mbus_device *mbdev,
					      enum mic_dma_chan_owner owner)
{
	struct mic_dma_device *mic_dma_dev;
	int ret;
	struct device *dev = &mbdev->dev;

	mic_dma_dev = kzalloc(sizeof(*mic_dma_dev), GFP_KERNEL);
	if (!mic_dma_dev) {
		ret = -ENOMEM;
		goto alloc_error;
	}
	mic_dma_dev->mbdev = mbdev;
	mic_dma_dev->dma_dev.dev = dev;
	mic_dma_dev->mmio = mbdev->mmio_va;
	if (MIC_DMA_CHAN_HOST == owner) {
		mic_dma_dev->start_ch = 0;
		mic_dma_dev->max_xfer_size = MIC_DMA_MAX_XFER_SIZE_HOST;
	} else {
		mic_dma_dev->start_ch = 4;
		mic_dma_dev->max_xfer_size = MIC_DMA_MAX_XFER_SIZE_CARD;
	}
	ret = mic_dma_init(mic_dma_dev, owner);
	if (ret)
		goto init_error;
	ret = mic_dma_register_dma_device(mic_dma_dev, owner);
	if (ret)
		goto reg_error;
	return mic_dma_dev;
reg_error:
	mic_dma_uninit(mic_dma_dev);
init_error:
	kfree(mic_dma_dev);
	mic_dma_dev = NULL;
alloc_error:
	dev_err(dev, "Error at %s %d ret=%d\n", __func__, __LINE__, ret);
	return mic_dma_dev;
}

static void mic_dma_dev_unreg(struct mic_dma_device *mic_dma_dev)
{
	mic_dma_unregister_dma_device(mic_dma_dev);
	mic_dma_uninit(mic_dma_dev);
	kfree(mic_dma_dev);
}

/* DEBUGFS CODE */
static int mic_dma_reg_seq_show(struct seq_file *s, void *pos)
{
	struct mic_dma_device *mic_dma_dev = s->private;
	int i, chan_num, first_chan = mic_dma_dev->start_ch;
	struct mic_dma_chan *ch;

	seq_printf(s, "SBOX_DCR: %#x\n",
		   mic_dma_mmio_read(&mic_dma_dev->mic_ch[first_chan],
				     MIC_DMA_SBOX_BASE + MIC_DMA_SBOX_DCR));
	seq_puts(s, "DMA Channel Registers\n");
	seq_printf(s, "%-10s| %-10s %-10s %-10s %-10s %-10s",
		   "Channel", "DCAR", "DTPR", "DHPR", "DRAR_HI", "DRAR_LO");
	seq_printf(s, " %-11s %-14s %-10s\n", "DCHERR", "DCHERRMSK", "DSTAT");
	for (i = first_chan; i < first_chan + MIC_DMA_NUM_CHAN; i++) {
		ch = &mic_dma_dev->mic_ch[i];
		chan_num = ch->ch_num;
		seq_printf(s, "%-10i| %-#10x %-#10x %-#10x %-#10x",
			   chan_num,
			   mic_dma_read_reg(ch, MIC_DMA_REG_DCAR),
			   mic_dma_read_reg(ch, MIC_DMA_REG_DTPR),
			   mic_dma_read_reg(ch, MIC_DMA_REG_DHPR),
			   mic_dma_read_reg(ch, MIC_DMA_REG_DRAR_HI));
		seq_printf(s, " %-#10x %-#10x %-#14x %-#10x\n",
			   mic_dma_read_reg(ch, MIC_DMA_REG_DRAR_LO),
			   mic_dma_read_reg(ch, MIC_DMA_REG_DCHERR),
			   mic_dma_read_reg(ch, MIC_DMA_REG_DCHERRMSK),
			   mic_dma_read_reg(ch, MIC_DMA_REG_DSTAT));
	}
	return 0;
}

static int mic_dma_reg_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mic_dma_reg_seq_show, inode->i_private);
}

static int mic_dma_reg_debug_release(struct inode *inode, struct file *file)
{
	return single_release(inode, file);
}

static const struct file_operations mic_dma_reg_ops = {
	.owner   = THIS_MODULE,
	.open    = mic_dma_reg_debug_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = mic_dma_reg_debug_release
};

/* Debugfs parent dir */
static struct dentry *mic_dma_dbg;

static int mic_dma_driver_probe(struct mbus_device *mbdev)
{
	struct mic_dma_device *mic_dma_dev;
	enum mic_dma_chan_owner owner;

	if (MBUS_DEV_DMA_MIC == mbdev->id.device)
		owner = MIC_DMA_CHAN_MIC;
	else
		owner = MIC_DMA_CHAN_HOST;

	mic_dma_dev = mic_dma_dev_reg(mbdev, owner);
	dev_set_drvdata(&mbdev->dev, mic_dma_dev);

	if (mic_dma_dbg) {
		mic_dma_dev->dbg_dir = debugfs_create_dir(dev_name(&mbdev->dev),
							  mic_dma_dbg);
		if (mic_dma_dev->dbg_dir)
			debugfs_create_file("mic_dma_reg", 0444,
					    mic_dma_dev->dbg_dir, mic_dma_dev,
					    &mic_dma_reg_ops);
	}
	return 0;
}

static void mic_dma_driver_remove(struct mbus_device *mbdev)
{
	struct mic_dma_device *mic_dma_dev;

	mic_dma_dev = dev_get_drvdata(&mbdev->dev);
	debugfs_remove_recursive(mic_dma_dev->dbg_dir);
	mic_dma_dev_unreg(mic_dma_dev);
}

static struct mbus_device_id id_table[] = {
	{MBUS_DEV_DMA_MIC, MBUS_DEV_ANY_ID},
	{MBUS_DEV_DMA_HOST, MBUS_DEV_ANY_ID},
	{0},
};

static struct mbus_driver mic_dma_driver = {
	.driver.name =	KBUILD_MODNAME,
	.driver.owner =	THIS_MODULE,
	.id_table = id_table,
	.probe = mic_dma_driver_probe,
	.remove = mic_dma_driver_remove,
};

static int __init mic_x100_dma_init(void)
{
	int rc = mbus_register_driver(&mic_dma_driver);
	if (rc)
		return rc;
	mic_dma_dbg = debugfs_create_dir(KBUILD_MODNAME, NULL);
	return 0;
}

static void __exit mic_x100_dma_exit(void)
{
	debugfs_remove_recursive(mic_dma_dbg);
	mbus_unregister_driver(&mic_dma_driver);
}

module_init(mic_x100_dma_init);
module_exit(mic_x100_dma_exit);

MODULE_DEVICE_TABLE(mbus, id_table);
MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) MIC X100 DMA Driver");
MODULE_LICENSE("GPL v2");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * Intel MIC Platform Software Stack (MPSS)
 *
 * Copyright(c) 2014 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Intel MIC X100 DMA Driver.
 *
 * Adapted from IOAT dma driver.
 */
#ifndef _MIC_X100_DMA_H_
#define _MIC_X100_DMA_H_

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/mic_bus.h>

#include "dmaengine.h"

/*
 * MIC has a total of 8 dma channels.
 * Four channels are assigned for host SW use & the remaining for MIC SW.
 * MIC DMA transfer size & addresses need to be 64 byte aligned.
 */
#define MIC_DMA_MAX_NUM_CHAN	8
#define MIC_DMA_NUM_CHAN	4
#define MIC_DMA_ALIGN_SHIFT	DMAENGINE_ALIGN_64_BYTES
#define MIC_DMA_ALIGN_BYTES	(1 << MIC_DMA_ALIGN_SHIFT)
#define MIC_DMA_DESC_RX_SIZE	(128 * 1024 - 4)

/*
 * Register descriptions
 * All the registers are 32 bit registers.
 * DCR is a global register and all others are per-channel.
 * DCR - bits 0, 2, 4, 6, 8, 10, 12, 14 - enable bits for channels 0 to 7
 *	 bits 1, 3, 5, 7, 9, 11, 13, 15 - owner bits for channels 0 to 7
 * DCAR - bit 24 & 25 interrupt masks for mic owned & host owned channels
 * DHPR - head of the descriptor ring updated by s/w
 * DTPR - tail of the descriptor ring updated by h/w
 * DRAR_LO - lower 32 bits of descriptor ring's mic address
 * DRAR_HI - 3:0 - remaining 4 bits of descriptor ring's mic address
 *	     20:4 descriptor ring size
 *	     25:21 mic smpt entry number
 * DSTAT - 16:0 h/w completion count; 31:28 dma engine status
 * DCHERR - this register is non-zero on error
 * DCHERRMSK - interrupt mask register
 */
#define MIC_DMA_HW_CMP_CNT_MASK		0x1ffff
#define MIC_DMA_CHAN_QUIESCE		0x20000000
#define MIC_DMA_SBOX_BASE		0x00010000
#define MIC_DMA_SBOX_DCR		0x0000A280
#define MIC_DMA_SBOX_CH_BASE		0x0001A000
#define MIC_DMA_SBOX_CHAN_OFF		0x40
#define MIC_DMA_SBOX_DCAR_IM0		(0x1 << 24)
#define MIC_DMA_SBOX_DCAR_IM1		(0x1 << 25)
#define MIC_DMA_SBOX_DRARHI_SYS_MASK	(0x1 << 26)
#define MIC_DMA_REG_DCAR		0
#define MIC_DMA_REG_DHPR		4
#define MIC_DMA_REG_DTPR		8
#define MIC_DMA_REG_DRAR_LO		20
#define MIC_DMA_REG_DRAR_HI		24
#define MIC_DMA_REG_DSTAT		32
#define MIC_DMA_REG_DCHERR		44
#define MIC_DMA_REG_DCHERRMSK		48

/* HW dma desc */
struct mic_dma_desc {
	u64 qw0;
	u64 qw1;
};

enum mic_dma_chan_owner {
	MIC_DMA_CHAN_MIC = 0,
	MIC_DMA_CHAN_HOST
};

/*
 * mic_dma_chan - channel specific information
 * @ch_num: channel number
 * @owner: owner of this channel
 * @last_tail: cached value of descriptor ring tail
 * @head: index of next descriptor in desc_ring
 * @issued: hardware notification point
 * @submitted: index that will be used to submit descriptors to h/w
 * @api_ch: dma engine api channel
 * @desc_ring: dma descriptor ring
 * @desc_ring_micpa: mic physical address of desc_ring
 * @status_dest: destination for status (fence) descriptor
 * @status_dest_micpa: mic address for status_dest,
 *		       DMA controller uses this address
 * @tx_array: array of async_tx
 * @cleanup_lock: lock held when processing completed tx
 * @prep_lock: lock held in prep_memcpy & released in tx_submit
 * @issue_lock: lock used to synchronize writes to head
 * @cookie: mic_irq cookie used with mic irq request
 */
struct mic_dma_chan {
	int ch_num;
	enum mic_dma_chan_owner owner;
	u32 last_tail;
	u32 head;
	u32 issued;
	u32 submitted;
	struct dma_chan api_ch;
	struct mic_dma_desc *desc_ring;
	dma_addr_t desc_ring_micpa;
	u64 *status_dest;
	dma_addr_t status_dest_micpa;
	struct dma_async_tx_descriptor *tx_array;
	spinlock_t cleanup_lock;
	spinlock_t prep_lock;
	spinlock_t issue_lock;
	struct mic_irq *cookie;
};

/*
 * struct mic_dma_device - per mic device
 * @mic_ch: dma channels
 * @dma_dev: underlying dma device
 * @mbdev: mic bus dma device
 * @mmio: virtual address of the mmio space
 * @dbg_dir: debugfs directory
 * @start_ch: first channel number that can be used
 * @max_xfer_size: maximum transfer size per dma descriptor
 */
struct mic_dma_device {
	struct mic_dma_chan mic_ch[MIC_DMA_MAX_NUM_CHAN];
	struct dma_device dma_dev;
	struct mbus_device *mbdev;
	void __iomem *mmio;
	struct dentry *dbg_dir;
	int start_ch;
	size_t max_xfer_size;
};

static inline struct mic_dma_chan *to_mic_dma_chan(struct dma_chan *ch)
{
	return container_of(ch, struct mic_dma_chan, api_ch);
}

static inline struct mic_dma_device *to_mic_dma_dev(struct mic_dma_chan *ch)
{
	return
	container_of((const typeof(((struct mic_dma_device *)0)->mic_ch)*)
		     (ch - ch->ch_num), struct mic_dma_device, mic_ch);
}

static inline struct mbus_device *to_mbus_device(struct mic_dma_chan *ch)
{
	return to_mic_dma_dev(ch)->mbdev;
}

static inline struct mbus_hw_ops *to_mbus_hw_ops(struct mic_dma_chan *ch)
{
	return to_mbus_device(ch)->hw_ops;
}

static inline struct device *mic_dma_ch_to_device(struct mic_dma_chan *ch)
{
	return to_mic_dma_dev(ch)->dma_dev.dev;
}

static inline void __iomem *mic_dma_chan_to_mmio(struct mic_dma_chan *ch)
{
	return to_mic_dma_dev(ch)->mmio;
}

static inline u32 mic_dma_read_reg(struct mic_dma_chan *ch, u32 reg)
{
	return ioread32(mic_dma_chan_to_mmio(ch) + MIC_DMA_SBOX_CH_BASE +
			ch->ch_num * MIC_DMA_SBOX_CHAN_OFF + reg);
}

static inline void mic_dma_write_reg(struct mic_dma_chan *ch, u32 reg, u32 val)
{
	iowrite32(val, mic_dma_chan_to_mmio(ch) + MIC_DMA_SBOX_CH_BASE +
		  ch->ch_num * MIC_DMA_SBOX_CHAN_OFF + reg);
}

static inline u32 mic_dma_mmio_read(struct mic_dma_chan *ch, u32 offset)
{
	return ioread32(mic_dma_chan_to_mmio(ch) + offset);
}

static inline void mic_dma_mmio_write(struct mic_dma_chan *ch, u32 val,
				      u32 offset)
{
	iowrite32(val, mic_dma_chan_to_mmio(ch) + offset);
}

static inline u32 mic_dma_read_cmp_cnt(struct mic_dma_chan *ch)
{
	return mic_dma_read_reg(ch, MIC_DMA_REG_DSTAT) &
	       MIC_DMA_HW_CMP_CNT_MASK;
}

static inline void mic_dma_chan_set_owner(struct mic_dma_chan *ch)
{
	u32 dcr = mic_dma_mmio_read(ch, MIC_DMA_SBOX_BASE + MIC_DMA_SBOX_DCR);
	u32 chan_num = ch->ch_num;

	dcr = (dcr & ~(0x1 << (chan_num * 2))) | (ch->owner << (chan_num * 2));
	mic_dma_mmio_write(ch, dcr, MIC_DMA_SBOX_BASE + MIC_DMA_SBOX_DCR);
}

static inline void mic_dma_enable_chan(struct mic_dma_chan *ch)
{
	u32 dcr = mic_dma_mmio_read(ch, MIC_DMA_SBOX_BASE + MIC_DMA_SBOX_DCR);

	dcr |= 2 << (ch->ch_num << 1);
	mic_dma_mmio_write(ch, dcr, MIC_DMA_SBOX_BASE + MIC_DMA_SBOX_DCR);
}

static inline void mic_dma_disable_chan(struct mic_dma_chan *ch)
{
	u32 dcr = mic_dma_mmio_read(ch, MIC_DMA_SBOX_BASE + MIC_DMA_SBOX_DCR);

	dcr &= ~(2 << (ch->ch_num << 1));
	mic_dma_mmio_write(ch, dcr, MIC_DMA_SBOX_BASE + MIC_DMA_SBOX_DCR);
}

static void mic_dma_chan_set_de