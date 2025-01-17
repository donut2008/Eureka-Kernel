dma->nr_channels)
		return NULL;

	return dma_request_channel(mask, mxs_dma_filter_fn, &param);
}

static int __init mxs_dma_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct platform_device_id *id_entry;
	const struct of_device_id *of_id;
	const struct mxs_dma_type *dma_type;
	struct mxs_dma_engine *mxs_dma;
	struct resource *iores;
	int ret, i;

	mxs_dma = devm_kzalloc(&pdev->dev, sizeof(*mxs_dma), GFP_KERNEL);
	if (!mxs_dma)
		return -ENOMEM;

	ret = of_property_read_u32(np, "dma-channels", &mxs_dma->nr_channels);
	if (ret) {
		dev_err(&pdev->dev, "failed to read dma-channels\n");
		return ret;
	}

	of_id = of_match_device(mxs_dma_dt_ids, &pdev->dev);
	if (of_id)
		id_entry = of_id->data;
	else
		id_entry = platform_get_device_id(pdev);

	dma_type = (struct mxs_dma_type *)id_entry->driver_data;
	mxs_dma->type = dma_type->type;
	mxs_dma->dev_id = dma_type->id;

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mxs_dma->base = devm_ioremap_resource(&pdev->dev, iores);
	if (IS_ERR(mxs_dma->base))
		return PTR_ERR(mxs_dma->base);

	mxs_dma->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(mxs_dma->clk))
		return PTR_ERR(mxs_dma->clk);

	dma_cap_set(DMA_SLAVE, mxs_dma->dma_device.cap_mask);
	dma_cap_set(DMA_CYCLIC, mxs_dma->dma_device.cap_mask);

	INIT_LIST_HEAD(&mxs_dma->dma_device.channels);

	/* Initialize channel parameters */
	for (i = 0; i < MXS_DMA_CHANNELS; i++) {
		struct mxs_dma_chan *mxs_chan = &mxs_dma->mxs_chans[i];

		mxs_chan->mxs_dma = mxs_dma;
		mxs_chan->chan.device = &mxs_dma->dma_device;
		dma_cookie_init(&mxs_chan->chan);

		tasklet_init(&mxs_chan->tasklet, mxs_dma_tasklet,
			     (unsigned long) mxs_chan);


		/* Add the channel to mxs_chan list */
		list_add_tail(&mxs_chan->chan.device_node,
			&mxs_dma->dma_device.channels);
	}

	ret = mxs_dma_init(mxs_dma);
	if (ret)
		return ret;

	mxs_dma->pdev = pdev;
	mxs_dma->dma_device.dev = &pdev->dev;

	/* mxs_dma gets 65535 bytes maximum sg size */
	mxs_dma->dma_device.dev->dma_parms = &mxs_dma->dma_parms;
	dma_set_max_seg_size(mxs_dma->dma_device.dev, MAX_XFER_BYTES);

	mxs_dma->dma_device.device_alloc_chan_resources = mxs_dma_alloc_chan_resources;
	mxs_dma->dma_device.device_free_chan_resources = mxs_dma_free_chan_resources;
	mxs_dma->dma_device.device_tx_status = mxs_dma_tx_status;
	mxs_dma->dma_device.device_prep_slave_sg = mxs_dma_prep_slave_sg;
	mxs_dma->dma_device.device_prep_dma_cyclic = mxs_dma_prep_dma_cyclic;
	mxs_dma->dma_device.device_pause = mxs_dma_pause_chan;
	mxs_dma->dma_device.device_resume = mxs_dma_resume_chan;
	mxs_dma->dma_device.device_terminate_all = mxs_dma_terminate_all;
	mxs_dma->dma_device.src_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
	mxs_dma->dma_device.dst_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
	mxs_dma->dma_device.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	mxs_dma->dma_device.residue_granularity = DMA_RESIDUE_GRANULARITY_BURST;
	mxs_dma->dma_device.device_issue_pending = mxs_dma_enable_chan;

	ret = dma_async_device_register(&mxs_dma->dma_device);
	if (ret) {
		dev_err(mxs_dma->dma_device.dev, "unable to register\n");
		return ret;
	}

	ret = of_dma_controller_register(np, mxs_dma_xlate, mxs_dma);
	if (ret) {
		dev_err(mxs_dma->dma_device.dev,
			"failed to register controller\n");
		dma_async_device_unregister(&mxs_dma->dma_device);
	}

	dev_info(mxs_dma->dma_device.dev, "initialized\n");

	return 0;
}

static struct platform_driver mxs_dma_driver = {
	.driver		= {
		.name	= "mxs-dma",
		.of_match_table = mxs_dma_dt_ids,
	},
	.id_table	= mxs_dma_ids,
};

static int __init mxs_dma_module_init(void)
{
	return platform_driver_probe(&mxs_dma_driver, mxs_dma_probe);
}
subsys_initcall(mxs_dma_module_init);
                                                                                                                                                                                                                                                                                                                                                          /*
 * Copyright (C) 2013-2014 Renesas Electronics Europe Ltd.
 * Author: Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 */

#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <dt-bindings/dma/nbpfaxi.h>

#include "dmaengine.h"

#define NBPF_REG_CHAN_OFFSET	0
#define NBPF_REG_CHAN_SIZE	0x40

/* Channel Current Transaction Byte register */
#define NBPF_CHAN_CUR_TR_BYTE	0x20

/* Channel Status register */
#define NBPF_CHAN_STAT	0x24
#define NBPF_CHAN_STAT_EN	1
#define NBPF_CHAN_STAT_TACT	4
#define NBPF_CHAN_STAT_ERR	0x10
#define NBPF_CHAN_STAT_END	0x20
#define NBPF_CHAN_STAT_TC	0x40
#define NBPF_CHAN_STAT_DER	0x400

/* Channel Control register */
#define NBPF_CHAN_CTRL	0x28
#define NBPF_CHAN_CTRL_SETEN	1
#define NBPF_CHAN_CTRL_CLREN	2
#define NBPF_CHAN_CTRL_STG	4
#define NBPF_CHAN_CTRL_SWRST	8
#define NBPF_CHAN_CTRL_CLRRQ	0x10
#define NBPF_CHAN_CTRL_CLREND	0x20
#define NBPF_CHAN_CTRL_CLRTC	0x40
#define NBPF_CHAN_CTRL_SETSUS	0x100
#define NBPF_CHAN_CTRL_CLRSUS	0x200

/* Channel Configuration register */
#define NBPF_CHAN_CFG	0x2c
#define NBPF_CHAN_CFG_SEL	7		/* terminal SELect: 0..7 */
#define NBPF_CHAN_CFG_REQD	8		/* REQuest Direction: DMAREQ is 0: input, 1: output */
#define NBPF_CHAN_CFG_LOEN	0x10		/* LOw ENable: low DMA request line is: 0: inactive, 1: active */
#define NBPF_CHAN_CFG_HIEN	0x20		/* HIgh ENable: high DMA request line is: 0: inactive, 1: active */
#define NBPF_CHAN_CFG_LVL	0x40		/* LeVeL: DMA request line is sensed as 0: edge, 1: level */
#define NBPF_CHAN_CFG_AM	0x700		/* ACK Mode: 0: Pulse mode, 1: Level mode, b'1x: Bus Cycle */
#define NBPF_CHAN_CFG_SDS	0xf000		/* Source Data Size: 0: 8 bits,... , 7: 1024 bits */
#define NBPF_CHAN_CFG_DDS	0xf0000		/* Destination Data Size: as above */
#define NBPF_CHAN_CFG_SAD	0x100000	/* Source ADdress counting: 0: increment, 1: fixed */
#define NBPF_CHAN_CFG_DAD	0x200000	/* Destination ADdress counting: 0: increment, 1: fixed */
#define NBPF_CHAN_CFG_TM	0x400000	/* Transfer Mode: 0: single, 1: block TM */
#define NBPF_CHAN_CFG_DEM	0x1000000	/* DMAEND interrupt Mask */
#define NBPF_CHAN_CFG_TCM	0x2000000	/* DMATCO interrupt Mask */
#define NBPF_CHAN_CFG_SBE	0x8000000	/* Sweep Buffer Enable */
#define NBPF_CHAN_CFG_RSEL	0x10000000	/* RM: Register Set sELect */
#define NBPF_CHAN_CFG_RSW	0x20000000	/* RM: Register Select sWitch */
#define NBPF_CHAN_CFG_REN	0x40000000	/* RM: Register Set Enable */
#define NBPF_CHAN_CFG_DMS	0x80000000	/* 0: register mode (RM), 1: link mode (LM) */

#define NBPF_CHAN_NXLA	0x38
#define NBPF_CHAN_CRLA	0x3c

/* Link Header field */
#define NBPF_HEADER_LV	1
#define NBPF_HEADER_LE	2
#define NBPF_HEADER_WBD	4
#define NBPF_HEADER_DIM	8

#define NBPF_CTRL	0x300
#define NBPF_CTRL_PR	1		/* 0: fixed priority, 1: round robin */
#define NBPF_CTRL_LVINT	2		/* DMAEND and DMAERR signalling: 0: pulse, 1: level */

#define NBPF_DSTAT_ER	0x314
#define NBPF_DSTAT_END	0x318

#define NBPF_DMA_BUSWIDTHS \
	(BIT(DMA_SLAVE_BUSWIDTH_UNDEFINED) | \
	 BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) | \
	 BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) | \
	 BIT(DMA_SLAVE_BUSWIDTH_4_BYTES) | \
	 BIT(DMA_SLAVE_BUSWIDTH_8_BYTES))

struct nbpf_config {
	int num_channels;
	int buffer_size;
};

/*
 * We've got 3 types of objects, used to describe DMA transfers:
 * 1. high-level descriptor, containing a struct dma_async_tx_descriptor object
 *	in it, used to communicate with the user
 * 2. hardware DMA link descriptors, that we pass to DMAC for DMA transfer
 *	queuing, these must be DMAable, using either the streaming DMA API or
 *	allocated from coherent memory - one per SG segment
 * 3. one per SG segment descriptors, used to manage HW link descriptors from
 *	(2). They do not have to be DMAable. They can either be (a) allocated
 *	together with link descriptors as mixed (DMA / CPU) objects, or (b)
 *	separately. Even if allocated separately it would be best to link them
 *	to link descriptors once during channel resource allocation and always
 *	use them as a single object.
 * Therefore for both cases (a) and (b) at run-time objects (2) and (3) shall be
 * treated as a single SG segment descriptor.
 */

struct nbpf_link_reg {
	u32	header;
	u32	src_addr;
	u32	dst_addr;
	u32	transaction_size;
	u32	config;
	u32	interval;
	u32	extension;
	u32	next;
} __packed;

struct nbpf_device;
struct nbpf_channel;
struct nbpf_desc;

struct nbpf_link_desc {
	struct nbpf_link_reg *hwdesc;
	dma_addr_t hwdesc_dma_addr;
	struct nbpf_desc *desc;
	struct list_head node;
};

/**
 * struct nbpf_desc - DMA transfer descriptor
 * @async_tx:	dmaengine object
 * @user_wait:	waiting for a user ack
 * @length:	total transfer length
 * @sg:		list of hardware descriptors, represented by struct nbpf_link_desc
 * @node:	member in channel descriptor lists
 */
struct nbpf_desc {
	struct dma_async_tx_descriptor async_tx;
	bool user_wait;
	size_t length;
	struct nbpf_channel *chan;
	struct list_head sg;
	struct list_head node;
};

/* Take a wild guess: allocate 4 segments per descriptor */
#define NBPF_SEGMENTS_PER_DESC 4
#define NBPF_DESCS_PER_PAGE ((PAGE_SIZE - sizeof(struct list_head)) /	\
	(sizeof(struct nbpf_desc) +					\
	 NBPF_SEGMENTS_PER_DESC *					\
	 (sizeof(struct nbpf_link_desc) + sizeof(struct nbpf_link_reg))))
#define NBPF_SEGMENTS_PER_PAGE (NBPF_SEGMENTS_PER_DESC * NBPF_DESCS_PER_PAGE)

struct nbpf_desc_page {
	struct list_head node;
	struct nbpf_desc desc[NBPF_DESCS_PER_PAGE];
	struct nbpf_link_desc ldesc[NBPF_SEGMENTS_PER_PAGE];
	struct nbpf_link_reg hwdesc[NBPF_SEGMENTS_PER_PAGE];
};

/**
 * struct nbpf_channel - one DMAC channel
 * @dma_chan:	standard dmaengine channel object
 * @base:	register address base
 * @nbpf:	DMAC
 * @name:	IRQ name
 * @irq:	IRQ number
 * @slave_addr:	address for slave DMA
 * @slave_width:slave data size in bytes
 * @slave_burst:maximum slave burst size in bytes
 * @terminal:	DMA terminal, assigned to this channel
 * @dmarq_cfg:	DMA request line configuration - high / low, edge / level for NBPF_CHAN_CFG
 * @flags:	configuration flags from DT
 * @lock:	protect descriptor lists
 * @free_links:	list of free link descriptors
 * @free:	list of free descriptors
 * @queued:	list of queued descriptors
 * @active:	list of descriptors, scheduled for processing
 * @done:	list of completed descriptors, waiting post-processing
 * @desc_page:	list of additionally allocated descriptor pages - if any
 */
struct nbpf_channel {
	struct dma_chan dma_chan;
	struct tasklet_struct tasklet;
	void __iomem *base;
	struct nbpf_device *nbpf;
	char name[16];
	int irq;
	dma_addr_t slave_src_addr;
	size_t slave_src_width;
	size_t slave_src_burst;
	dma_addr_t slave_dst_addr;
	size_t slave_dst_width;
	size_t slave_dst_burst;
	unsigned int terminal;
	u32 dmarq_cfg;
	unsigned long flags;
	spinlock_t lock;
	struct list_head free_links;
	struct list_head free;
	struct list_head queued;
	struct list_head active;
	struct list_head done;
	struct list_head desc_page;
	struct nbpf_desc *running;
	bool paused;
};

struct nbpf_device {
	struct dma_device dma_dev;
	void __iomem *base;
	struct clk *clk;
	const struct nbpf_config *config;
	struct nbpf_channel chan[];
};

enum nbpf_model {
	NBPF1B4,
	NBPF1B8,
	NBPF1B16,
	NBPF4B4,
	NBPF4B8,
	NBPF4B16,
	NBPF8B4,
	NBPF8B8,
	NBPF8B16,
};

static struct nbpf_config nbpf_cfg[] = {
	[NBPF1B4] = {
		.num_channels = 1,
		.buffer_size = 4,
	},
	[NBPF1B8] = {
		.num_channels = 1,
		.buffer_size = 8,
	},
	[NBPF1B16] = {
		.num_channels = 1,
		.buffer_size = 16,
	},
	[NBPF4B4] = {
		.num_channels = 4,
		.buffer_size = 4,
	},
	[NBPF4B8] = {
		.num_channels = 4,
		.buffer_size = 8,
	},
	[NBPF4B16] = {
		.num_channels = 4,
		.buffer_size = 16,
	},
	[NBPF8B4] = {
		.num_channels = 8,
		.buffer_size = 4,
	},
	[NBPF8B8] = {
		.num_channels = 8,
		.buffer_size = 8,
	},
	[NBPF8B16] = {
		.num_channels = 8,
		.buffer_size = 16,
	},
};

#define nbpf_to_chan(d) container_of(d, struct nbpf_channel, dma_chan)

/*
 * dmaengine drivers seem to have a lot in common and instead of sharing more
 * code, they reimplement those common algorithms independently. In this driver
 * we try to separate the hardware-specific part from the (largely) generic
 * part. This improves code readability and makes it possible in the future to
 * reuse the generic code in form of a helper library. That generic code should
 * be suitable for various DMA controllers, using transfer descriptors in RAM
 * and pushing one SG list at a time to the DMA controller.
 */

/*		Hardware-specific part		*/

static inline u32 nbpf_chan_read(struct nbpf_channel *chan,
				 unsigned int offset)
{
	u32 data = ioread32(chan->base + offset);
	dev_dbg(chan->dma_chan.device->dev, "%s(0x%p + 0x%x) = 0x%x\n",
		__func__, chan->base, offset, data);
	return data;
}

static inline void nbpf_chan_write(struct nbpf_channel *chan,
				   unsigned int offset, u32 data)
{
	iowrite32(data, chan->base + offset);
	dev_dbg(chan->dma_chan.device->dev, "%s(0x%p + 0x%x) = 0x%x\n",
		__func__, chan->base, offset, data);
}

static inline u32 nbpf_read(struct nbpf_device *nbpf,
			    unsigned int offset)
{
	u32 data = ioread32(nbpf->base + offset);
	dev_dbg(nbpf->dma_dev.dev, "%s(0x%p + 0x%x) = 0x%x\n",
		__func__, nbpf->base, offset, data);
	return data;
}

static inline void nbpf_write(struct nbpf_device *nbpf,
			      unsigned int offset, u32 data)
{
	iowrite32(data, nbpf->base + offset);
	dev_dbg(nbpf->dma_dev.dev, "%s(0x%p + 0x%x) = 0x%x\n",
		__func__, nbpf->base, offset, data);
}

static void nbpf_chan_halt(struct nbpf_channel *chan)
{
	nbpf_chan_write(chan, NBPF_CHAN_CTRL, NBPF_CHAN_CTRL_CLREN);
}

static bool nbpf_status_get(struct nbpf_channel *chan)
{
	u32 status = nbpf_read(chan->nbpf, NBPF_DSTAT_END);

	return status & BIT(chan - chan->nbpf->chan);
}

static void nbpf_status_ack(struct nbpf_channel *chan)
{
	nbpf_chan_write(chan, NBPF_CHAN_CTRL, NBPF_CHAN_CTRL_CLREND);
}

static u32 nbpf_error_get(struct nbpf_device *nbpf)
{
	return nbpf_read(nbpf, NBPF_DSTAT_ER);
}

static struct nbpf_channel *nbpf_error_get_channel(struct nbpf_device *nbpf, u32 error)
{
	return nbpf->chan + __ffs(error);
}

static void nbpf_error_clear(struct nbpf_channel *chan)
{
	u32 status;
	int i;

	/* Stop the channel, make sure DMA has been aborted */
	nbpf_chan_halt(chan);

	for (i = 1000; i; i--) {
		status = nbpf_chan_read(chan, NBPF_CHAN_STAT);
		if (!(status & NBPF_CHAN_STAT_TACT))
			break;
		cpu_relax();
	}

	if (!i)
		dev_err(chan->dma_chan.device->dev,
			"%s(): abort timeout, channel status 0x%x\n", __func__, status);

	nbpf_chan_write(chan, NBPF_CHAN_CTRL, NBPF_CHAN_CTRL_SWRST);
}

static int nbpf_start(struct nbpf_desc *desc)
{
	struct nbpf_channel *chan = desc->chan;
	struct nbpf_link_desc *ldesc = list_first_entry(&desc->sg, struct nbpf_link_desc, node);

	nbpf_chan_write(chan, NBPF_CHAN_NXLA, (u32)ldesc->hwdesc_dma_addr);
	nbpf_chan_write(chan, NBPF_CHAN_CTRL, NBPF_CHAN_CTRL_SETEN | NBPF_CHAN_CTRL_CLRSUS);
	chan->paused = false;

	/* Software trigger MEMCPY - only MEMCPY uses the block mode */
	if (ldesc->hwdesc->config & NBPF_CHAN_CFG_TM)
		nbpf_chan_write(chan, NBPF_CHAN_CTRL, NBPF_CHAN_CTRL_STG);

	dev_dbg(chan->nbpf->dma_dev.dev, "%s(): next 0x%x, cur 0x%x\n", __func__,
		nbpf_chan_read(chan, NBPF_CHAN_NXLA), nbpf_chan_read(chan, NBPF_CHAN_CRLA));

	return 0;
}

static void nbpf_chan_prepare(struct nbpf_channel *chan)
{
	chan->dmarq_cfg = (chan->flags & NBPF_SLAVE_RQ_HIGH ? NBPF_CHAN_CFG_HIEN : 0) |
		(chan->flags & NBPF_SLAVE_RQ_LOW ? NBPF_CHAN_CFG_LOEN : 0) |
		(chan->flags & NBPF_SLAVE_RQ_LEVEL ?
		 NBPF_CHAN_CFG_LVL | (NBPF_CHAN_CFG_AM & 0x200) : 0) |
		chan->terminal;
}

static void nbpf_chan_prepare_default(struct nbpf_channel *chan)
{
	/* Don't output DMAACK */
	chan->dmarq_cfg = NBPF_CHAN_CFG_AM & 0x400;
	chan->terminal = 0;
	chan->flags = 0;
}

static void nbpf_chan_configure(struct nbpf_channel *chan)
{
	/*
	 * We assume, that only the link mode and DMA request line configuration
	 * have to be set in the configuration register manually. Dynamic
	 * per-transfer configuration will be loaded from transfer descriptors.
	 */
	nbpf_chan_write(chan, NBPF_CHAN_CFG, NBPF_CHAN_CFG_DMS | chan->dmarq_cfg);
}

static u32 nbpf_xfer_ds(struct nbpf_device *nbpf, size_t size)
{
	/* Maximum supported bursts depend on the buffer size */
	return min_t(int, __ffs(size), ilog2(nbpf->config->buffer_size * 8));
}

static size_t nbpf_xfer_size(struct nbpf_device *nbpf,
			     enum dma_slave_buswidth width, u32 burst)
{
	size_t size;

	if (!burst)
		burst = 1;

	switch (width) {
	case DMA_SLAVE_BUSWIDTH_8_BYTES:
		size = 8 * burst;
		break;

	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		size = 4 * burst;
		break;

	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		size = 2 * burst;
		break;

	default:
		pr_warn("%s(): invalid bus width %u\n", __func__, width);
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		size = burst;
	}

	return nbpf_xfer_ds(nbpf, size);
}

/*
 * We need a way to recognise slaves, whose data is sent "raw" over the bus,
 * i.e. it isn't known in advance how many bytes will be received. Therefore
 * the slave driver has to provide a "large enough" buffer and either read the
 * buffer, when it is full, or detect, that some data has arrived, then wait for
 * a timeout, if no more data arrives - receive what's already there. We want to
 * handle such slaves in a special way to allow an optimised mode for other
 * users, for whom the amount of data is known in advance. So far there's no way
 * to recognise such slaves. We use a data-width check to distinguish between
 * the SD host and the PL011 UART.
 */

static int nbpf_prep_one(struct nbpf_link_desc *ldesc,
			 enum dma_transfer_direction direction,
			 dma_addr_t src, dma_addr_t dst, size_t size, bool last)
{
	struct nbpf_link_reg *hwdesc = ldesc->hwdesc;
	struct nbpf_desc *desc = ldesc->desc;
	struct nbpf_channel *chan = desc->chan;
	struct device *dev = chan->dma_chan.device->dev;
	size_t mem_xfer, slave_xfer;
	bool can_burst;

	hwdesc->header = NBPF_HEADER_WBD | NBPF_HEADER_LV |
		(last ? NBPF_HEADER_LE : 0);

	hwdesc->src_addr = src;
	hwdesc->dst_addr = dst;
	hwdesc->transaction_size = size;

	/*
	 * set config: SAD, DAD, DDS, SDS, etc.
	 * Note on transfer sizes: the DMAC can perform unaligned DMA transfers,
	 * but it is important to have transaction size a multiple of both
	 * receiver and transmitter transfer sizes. It is also possible to use
	 * different RAM and device transfer sizes, and it does work well with
	 * some devices, e.g. with V08R07S01E SD host controllers, which can use
	 * 128 byte transfers. But this doesn't work with other devices,
	 * especially when the transaction size is unknown. This is the case,
	 * e.g. with serial drivers like amba-pl011.c. For reception it sets up
	 * the transaction size of 4K and if fewer bytes are received, it
	 * pauses DMA and reads out data received via DMA as well as those left
	 * in the Rx FIFO. For this to work with the RAM side using burst
	 * transfers we enable the SBE bit and terminate the transfer in our
	 * .device_pause handler.
	 */
	mem_xfer = nbpf_xfer_ds(chan->nbpf, size);

	switch (direction) {
	case DMA_DEV_TO_MEM:
		can_burst = chan->slave_src_width >= 3;
		slave_xfer = min(mem_xfer, can_burst ?
				 chan->slave_src_burst : chan->slave_src_width);
		/*
		 * Is the slave narrower than 64 bits, i.e. isn't using the full
		 * bus width and cannot use bursts?
		 */
		if (mem_xfer > chan->slave_src_burst && !can_burst)
			mem_xfer = chan->slave_src_burst;
		/* Device-to-RAM DMA is unreliable without REQD set */
		hwdesc->config = NBPF_CHAN_CFG_SAD | (NBPF_CHAN_CFG_DDS & (mem_xfer << 16)) |
			(NBPF_CHAN_CFG_SDS & (slave_xfer << 12)) | NBPF_CHAN_CFG_REQD |
			NBPF_CHAN_CFG_SBE;
		break;

	case DMA_MEM_TO_DEV:
		slave_xfer = min(mem_xfer, chan->slave_dst_width >= 3 ?
				 chan->slave_dst_burst : chan->slave_dst_width);
		hwdesc->config = NBPF_CHAN_CFG_DAD | (NBPF_CHAN_CFG_SDS & (mem_xfer << 12)) |
			(NBPF_CHAN_CFG_DDS & (slave_xfer << 16)) | NBPF_CHAN_CFG_REQD;
		break;

	case DMA_MEM_TO_MEM:
		hwdesc->config = NBPF_CHAN_CFG_TCM | NBPF_CHAN_CFG_TM |
			(NBPF_CHAN_CFG_SDS & (mem_xfer << 12)) |
			(NBPF_CHAN_CFG_DDS & (mem_xfer << 16));
		break;

	default:
		return -EINVAL;
	}

	hwdesc->config |= chan->dmarq_cfg | (last ? 0 : NBPF_CHAN_CFG_DEM) |
		NBPF_CHAN_CFG_DMS;

	dev_dbg(dev, "%s(): desc @ %pad: hdr 0x%x, cfg 0x%x, %zu @ %pad -> %pad\n",
		__func__, &ldesc->hwdesc_dma_addr, hwdesc->header,
		hwdesc->config, size, &src, &dst);

	dma_sync_single_for_device(dev, ldesc->hwdesc_dma_addr, sizeof(*hwdesc),
				   DMA_TO_DEVICE);

	return 0;
}

static size_t nbpf_bytes_left(struct nbpf_channel *chan)
{
	return nbpf_chan_read(chan, NBPF_CHAN_CUR_TR_BYTE);
}

static void nbpf_configure(struct nbpf_device *nbpf)
{
	nbpf_wri