/*
 * DMA Router driver for LPC18xx/43xx DMA MUX
 *
 * Copyright (C) 2015 Joachim Eastwood <manabian@gmail.com>
 *
 * Based on TI DMA Crossbar driver by:
 *   Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com
 *   Author: Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/mfd/syscon.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>

/* CREG register offset and macros for mux manipulation */
#define LPC18XX_CREG_DMAMUX		0x11c
#define LPC18XX_DMAMUX_VAL(v, n)	((v) << (n * 2))
#define LPC18XX_DMAMUX_MASK(n)		(0x3 << (n * 2))
#define LPC18XX_DMAMUX_MAX_VAL		0x3

struct lpc18xx_dmamux {
	u32 value;
	bool busy;
};

struct lpc18xx_dmamux_data {
	struct dma_router dmarouter;
	struct lpc18xx_dmamux *muxes;
	u32 dma_master_requests;
	u32 dma_mux_requests;
	struct regmap *reg;
	spinlock_t lock;
};

static void lpc18xx_dmamux_free(struct device *dev, void *route_data)
{
	struct lpc18xx_dmamux_data *dmamux = dev_get_drvdata(dev);
	struct lpc18xx_dmamux *mux = route_data;
	unsigned long flags;

	spin_lock_irqsave(&dmamux->lock, flags);
	mux->busy = false;
	spin_unlock_irqrestore(&dmamux->lock, flags);
}

static void *lpc18xx_dmamux_reserve(struct of_phandle_args *dma_spec,
				    struct of_dma *ofdma)
{
	struct platform_device *pdev = of_find_device_by_node(ofdma->of_node);
	struct lpc18xx_dmamux_data *dmamux = platform_get_drvdata(pdev);
	unsigned long flags;
	unsigned mux;

	if (dma_spec->args_count != 3) {
		dev_err(&pdev->dev, "invalid number of dma mux args\n");
		return ERR_PTR(-EINVAL);
	}

	mux = dma_spec->args[0];
	if (mux >= dmamux->dma_master_requests) {
		dev_err(&pdev->dev, "invalid mux number: %d\n",
			dma_spec->args[0]);
		return ERR_PTR(-EINVAL);
	}

	if (dma_spec->args[1] > LPC18XX_DMAMUX_MAX_VAL) {
		dev_err(&pdev->dev, "invalid dma mux value: %d\n",
			dma_spec->args[1]);
		return ERR_PTR(-EINVAL);
	}

	/* The of_node_put() will be done in the core for the node */
	dma_spec->np = of_parse_phandle(ofdma->of_node, "dma-masters", 0);
	if (!dma_spec->np) {
		dev_err(&pdev->dev, "can't get dma master\n");
		return ERR_PTR(-EINVAL);
	}

	spin_lock_irqsave(&dmamux->lock, flags);
	if (dmamux->muxes[mux].busy) {
		spin_unlock_irqrestore(&dmamux->lock, flags);
		dev_err(&pdev->dev, "dma request %u busy with %u.%u\n",
			mux, mux, dmamux->muxes[mux].value);
		of_node_put(dma_spec->np);
		return ERR_PTR(-EBUSY);
	}

	dmamux->muxes[mux].busy = true;
	dmamux->muxes[mux].value = dma_spec->args[1];

	regmap_update_bits(dmamux->reg, LPC18XX_CREG_DMAMUX,
			   LPC18XX_DMAMUX_MASK(mux),
			   LPC18XX_DMAMUX_VAL(dmamux->muxes[mux].value, mux));
	spin_unlock_irqrestore(&dmamux->lock, flags);

	dma_spec->args[1] = dma_spec->args[2];
	dma_spec->args_count = 2;

	dev_dbg(&pdev->dev, "mapping dmamux %u.%u to dma request %u\n", mux,
		dmamux->muxes[mux].value, mux);

	return &dmamux->muxes[mux];
}

static int lpc18xx_dmamux_probe(struct platform_device *pdev)
{
	struct device_node *dma_np, *np = pdev->dev.of_node;
	struct lpc18xx_dmamux_data *dmamux;
	int ret;

	dmamux = devm_kzalloc(&pdev->dev, sizeof(*dmamux), GFP_KERNEL);
	if (!dmamux)
		return -ENOMEM;

	dmamux->reg = syscon_regmap_lookup_by_compatible("nxp,lpc1850-creg");
	if (IS_ERR(dmamux->reg)) {
		dev_err(&pdev->dev, "syscon lookup failed\n");
		return PTR_ERR(dmamux->reg);
	}

	ret = of_property_read_u32(np, "dma-requests",
				   &dmamux->dma_mux_requests);
	if (ret) {
		dev_err(&pdev->dev, "missing dma-requests property\n");
		return ret;
	}

	dma_np = of_parse_phandle(np, "dma-masters", 0);
	if (!dma_np) {
		dev_err(&pdev->dev, "can't get dma master\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(dma_np, "dma-requests",
				   &dmamux->dma_master_requests);
	of_node_put(dma_np);
	if (ret) {
		dev_err(&pdev->dev, "missing master dma-requests property\n");
		return ret;
	}

	dmamux->muxes = devm_kcalloc(&pdev->dev, dmamux->dma_master_requests,
				     sizeof(struct lpc18xx_dmamux),
				     GFP_KERNEL);
	if (!dmamux->muxes)
		return -ENOMEM;

	spin_lock_init(&dmamux->lock);
	platform_set_drvdata(pdev, dmamux);
	dmamux->dmarouter.dev = &pdev->dev;
	dmamux->dmarouter.route_free = lpc18xx_dmamux_free;

	return of_dma_router_register(np, lpc18xx_dmamux_reserve,
				      &dmamux->dmarouter);
}

static const struct of_device_id lpc18xx_dmamux_match[] = {
	{ .compatible = "nxp,lpc1850-dmamux" },
	{},
};

static struct platform_driver lpc18xx_dmamux_driver = {
	.probe	= lpc18xx_dmamux_probe,
	.driver = {
		.name = "lpc18xx-dmamux",
		.of_match_table = lpc18xx_dmamux_match,
	},
};

static int __init lpc18xx_dmamux_init(void)
{
	return platform_driver_register(&lpc18xx_dmamux_driver);
}
arch_initcall(lpc18xx_dmamux_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
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
#include <linux/module.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>

#include "mic_x100_dma.h"

#define MIC_DMA_MAX_XFER_SIZE_CARD  (1 * 1024 * 1024 -\
				       MIC_DMA_ALIGN_BYTES)
#define MIC_DMA_MAX_XFER_SIZE_HOST  (1 * 1024 * 1024 >> 1)
#define MIC_DMA_DESC_TYPE_SHIFT	60
#define MIC_DMA_MEMCPY_LEN_SHIFT 46
#define MIC_DMA_STAT_INTR_SHIFT 59

/* high-water mark for pushing dma descriptors */
static int mic_dma_pending_level = 4;

/* Status descriptor is used to write a 64 bit value to a memory location */
enum mic_dma_desc_format_type {
	MIC_DMA_MEMCPY = 1,
	MIC_DMA_STATUS,
};

static inline u32 mic_dma_hw_ring_inc(u32 val)
{
	return (val + 1) % MIC_DMA_DESC_RX_SIZE;
}

static inline u32 mic_dma_hw_ring_dec(u32 val)
{
	return val ? val - 1 : MIC_DMA_DESC_RX_SIZE - 1;
}

static inline void mic_dma_hw_ring_inc_head(struct mic_dma_chan *ch)
{
	ch->head = mic_dma_hw_ring_inc(ch->head);
}

/* Prepare a memcpy desc */
static inline void mic_dma_memcpy_desc(struct mic_dma_desc *desc,
	dma_addr_t src_phys, dma_addr_t dst_phys, u64 size)
{
	u64 qw0, qw1;

	qw0 = src_phys;
	qw0 |= (size >> MIC_DMA_ALIGN_SHIFT) << MIC_DMA_MEMCPY_LEN_SHIFT;
	qw1 = MIC_DMA_MEMCPY;
	qw1 <<= MIC_DMA_DESC_TYPE_SHIFT;
	qw1 |= dst_phys;
	desc->qw0 = qw0;
	desc->qw1 = qw1;
}

/* Prepare a status desc. with @data to be written at @dst_phys */
static inline void mic_dma_prep_status_desc(struct mic_dma_desc *desc, u64 data,
	dma_addr_t dst_phys, bool generate_intr)
{
	u64 qw0, qw1;

	qw0 = data;
	qw1 = (u64) MIC_DMA_STATUS << MIC_DMA_DESC_TYPE_SHIFT | dst_phys;
	if (generate_intr)
		qw1 |= (1ULL << MIC_DMA_STAT_INTR_SHIFT);
	desc->qw0 = qw0;
	desc->qw1 = qw1;
}

static void mic_dma_cleanup(struct mic_dma_chan *ch)
{
	struct dma_async_tx_descriptor *tx;
	u32 tail;
	u32 last_tail;

	spin_lock(&ch->cleanup_lock);
	tail = mic_dma_read_cmp_cnt(ch);
	/*
	 * This is the barrier pair for smp_wmb() in fn.
	 * mic_dma_tx_submit_unlock. It's required so that we read the
	 * updated cookie value from tx->cookie.
	 */
	smp_rmb();
	for (last_tail = ch->last_tail; tail != last_tail;) {
		tx = &ch->tx_array[last_tail];
		if (tx->cookie) {
			dma_cookie_complete(tx);
			if (tx->callback) {
				tx->callback(tx->callback_param);
				tx->callback = NULL;
			}
		}
		last_tail = mic_dma_hw_ring_inc(last_tail);
	}
	/* finish all completion callbacks before incrementing tail */
	smp_mb();
	ch->last_tail = last_tail;
	spin_unlock(&ch->cleanup_lock);
}

static u32 mic_dma_ring_count(u32 head, u32 tail)
{
	u32 count;

	if (head >= tail)
		count = (tail - 0) + (MIC_DMA_DESC_RX_SIZE - head);
	else
		count = tail - head;
	return count - 1;
}

/* Returns the num. of free descriptors on success, -ENOMEM on failure */
static int mic_dma_avail_desc_ring_space(struct mic_dma_chan *ch, int required)
{
	struct device *dev = mic_dma_ch_to_device(ch);
	u32 count;

	count = mic_dma_ring_count(ch->head, ch->last_tail);
	if (count < required) {
		mic_dma_cleanup(ch);
		count = mic_dma_ring_count(ch->head, ch->last_tail);
	}

	if (count < required) {
		dev_dbg(dev, "Not enough desc space");
		dev_dbg(dev, "%s %d required=%u, avail=%u\n",
			__func__, __LINE__, required, count);
		return -ENOMEM;
	} else {
		return count;
	}
}

/* Program memcpy descriptors into the descriptor ring and update s/w head ptr*/
static int mic_dma_prog_memcpy_desc(struct mic_dma_chan *ch, dma_addr_t src,
				    dma_addr_t dst, size_t len)
{
	size_t current_transfer_len;
	size_t max_xfer_size = to_mic_dma_dev(ch)->max_xfer_size;
	/* 3 is added to make sure we have enough space for status desc */
	int num_desc = len / max_xfer_size + 3;
	int ret;

	if (len % max_xfer_size)
		num_desc++;

	ret = mic_dma_avail_desc_ring_space(ch, num_desc);
	if (ret < 0)
		return ret;
	do {
		current_transfer_len = min(len, max_xfer_size);
		mic_dma_memcpy_desc(&ch->desc_ring[ch->head],
				    src, dst, current_transfer_len);
		mic_dma_hw_ring_inc_head(ch);
		len -= current_transfer_len;
		dst = dst + current_transfer_len;
		src = src + current_transfer_len;
	} while (len > 0);
	return 0;
}

/* It's a h/w quirk and h/w needs 2 status descriptors for every status desc */
static void mic_dma_prog_intr(struct mic_dma_chan *ch)
{
	mic_dma_prep_status_desc(&ch->desc_ring[ch->head], 0,
				 ch->status_dest_micpa, false);
	mic_dma_hw_ring_inc_head(ch);
	mic_dma_prep_status_desc(&ch->desc_ring[ch->head], 0,
				 ch->status_dest_micpa, true);
	mic_dma_hw_ring_inc_head(ch);
}

/* Wrapper function to program memcpy descriptors/status descriptors */
static int mic_dma_do_dma(struct mic_dma_chan *ch, int flags, dma_addr_t src,
			  dma_addr_t dst, size_t len)
{
	if (len && -ENOMEM == mic_dma_prog_memcpy_desc(ch, src, dst, len)) {
		return -ENOMEM;
	} else {
		/* 3 is the maximum number of status descriptors */
		int ret = mic_dma_avail_desc_ring_space(ch, 3);

		if (ret < 0)
			return ret;
	}

	/* Above mic_dma_prog_memcpy_desc() makes sure we have enough space */
	if (flags & DMA_PREP_FENCE) {
		mic_dma_prep_status_desc(&ch->desc_ring[ch->head], 0,
					 ch->status_dest_micpa, false);
		mic_dma_hw_ring_inc_head(ch);
	}

	if (flags & DMA_PREP_INTERRUPT)
		mic_dma_prog_intr(ch);

	return 0;
}

static inline void mic_dma_issue_pending(struct dma_chan *ch)
{
	struct mic_dma_chan *mic_ch = to_mic_dma_chan(ch);

	spin_lock(&mic_ch->issue_lock);
	/*
	 * Write to head triggers h/w to act on the descriptors.
	 * On MIC, writing the same head value twice causes
	 * a h/w error. On second write, h/w assumes we filled
	 * the entire ring & overwrote some of the descriptors.
	 */
	if (mic_ch->issued == mic_ch->submitted)
		goto out;
	mic_ch->issued = mic_ch->submitted;
	/*
	 * make descriptor updates visible before advancing head,
	 * this is purposefully not smp_wmb() since we are also
	 * publishing the descriptor updates to a dma device
	 */
	wmb();
	mic_dma_write_reg(mic_ch, MIC_DMA_REG_DHPR, mic_ch->issued);
out:
	spin_unlock(&mic_ch->issue_lock);
}

static inline void mic_dma_update_pending(struct mic_dma_chan *ch)
{
	if (mic_dma_ring_count(ch->issued, ch->submitted)
			> mic_dma_pending_level)
		mic_dma_issue_pending(&ch->api_ch);
}

static dma_cookie_t mic_dma_tx_submit_unlock(struct dma_async_tx_descriptor *tx)
{
	struct mic_dma_chan *mic_ch = to_mic_dma_chan(tx->chan);
	dma_cookie_t cookie;

	dma_cookie_assign(tx);
	cookie = tx->cookie;
	/*
	 * We need an smp write barrier here because another CPU might see
	 * an update to submitted and update h/w head even before we
	 * assigned a cookie to this tx.
	 */
	smp_wmb();
	mic_ch->submitted = mic_ch->head;
	spin_unlock(&mic_ch->prep_lock);
	mic_dma_update_pending(mic_ch);
	return cookie;
}

static inline struct dma_async_tx_descriptor *
allocate_tx(struct mic_dma_chan *ch)
{
	u32 idx = mic_dma_hw_ring_dec(ch->head);
	struct dma_async_tx_descriptor *tx = &ch->tx_array[idx];

	dma_async_tx_descriptor_init(tx, &ch->api_ch);
	tx->tx_submit = mic_dma_tx_submit_unlock;
	return tx;
}

/* Program a status descriptor with dst as address and value to be written */
static struct dma_async_tx_descriptor *
mic_dma_prep_status_lock(struct dma_chan *ch, dma_addr_t dst, u64 src_val,
			 unsigned long flags)
{
	struct mic_dma_chan *mic_ch = to_mic_dma_chan(ch);
	int result;

	spin_lock(&mic_ch->prep_lock);
	result = mic_dma_avail_desc_ring_space(mic_ch, 4);
	if (result < 0)
		goto error;
	mic_dma_prep_status_desc(&mic_ch->desc_ring[mic_ch->head], src_val, dst,
				 false);
	mic_dma_hw_ring_inc_head(mic_ch);
	result = mic_dma_do_dma(mic_ch, flags, 0, 0, 0);
	if (result < 0)
		goto error;

	return allocate_tx(mic_ch);
error:
	dev_err(mic_dma_ch_to_device(mic_ch),
		"Error enqueueing dma status descriptor, error=%d\n", result);
	spin_unlock(&mic_ch->prep_lock);
	return NULL;
}

/*
 * Prepare a memcpy descriptor to be added to the ring.
 * Note that the temporary descriptor adds an extra overhead of copying the
 * descriptor to ring. So, we copy directly to the descriptor ring
 */
static struct dma_async_tx_descriptor *
mic_dma_prep_memcpy_lock(struct dma_chan *ch, dma_addr_t dma_dest,
			 dma_addr_t dma_src, size_t len, unsigned long flags)
{
	struct mic_dma_chan *mic_ch = to_mic_dma_chan(ch);
	struct device *dev = mic_dma_ch_to_device(mic_ch);
	int result;

	if (!len && !flags)
		return NULL;

	spin_lock(&mic_ch->prep_lock);
	result = mic_dma_do_dma(mic_ch, flags, dma_src, dma_dest, len);
	if (result >= 0)
		return allocate_tx(mic_ch);
	dev_err(dev, "Error enqueueing dma, error=%d\n", result);
	spin_unlock(&mic_ch->prep_lock);
	return NULL;
}

static struct dma_async_tx_descriptor *
mic_dma_prep_interrupt_lock(struct dma_chan *ch, unsigned long flags)
{
	struct mic_dma_chan *mic_ch = to_mic_dma_chan(ch);
	int ret;

	spin_lock(&mic_ch->prep_lock);
	ret = mic_dma_do_dma(mic_ch, flags, 0, 0, 0);
	if (!ret)
		return allocate_tx(mic_ch);
	spin_unlock(&mic_ch->prep_lock);
	return NULL;
}

/* Return the status of the transaction */
static enum dma_status
mic_dma_tx_status(struct dma_chan *ch, dma_cookie_t cookie,
		  struct dma_tx_state *txstate)
{
	struct mic_dma_chan *mic_ch = to_mic_dma_chan(ch);

	if (DMA_COMPLETE != dma_cookie_status(ch, cookie, txstate))
		mic_dma_cleanup(mic_ch);

	return dma_cookie_status(ch, cookie, txstate);
}

static irqreturn_t mic_dma_thread_fn(int irq, void *data)
{
	mic_dma_cleanup((struct mic_dma_chan *)data);
	return IRQ_HANDLED;
}

static irqreturn_t mic_dma_intr_handler(int irq, void *data)
{
	struct mic_dma_chan *ch = ((struct mic_dma_chan *)data);

	mic_dma_ack_interrupt(ch);
	return IRQ_WAKE_THREAD;
}

static int mic_dma_alloc_desc_ring(struct mic_dma_chan *ch)
{
	u64 desc_ring_size = MIC_DMA_DESC_RX_SIZE * sizeof(*ch->desc_ring);
	struct device *dev = &to_mbus_device(ch)->dev;

	desc_ring_size = ALIGN(desc_ring_size, MIC_DMA_ALIGN_BYTES);
	ch->desc_ring = kzalloc(desc_ring_size, GFP_KERNEL);

	if (!ch->desc_ring)
		return -ENOMEM;

	ch->desc_ring_micpa = dma_map_single(dev, ch->desc_ring,
					     desc_ring_size, DMA_BIDIRECTIONAL);
	if (dma_mapping_error(dev, ch->desc_ring_micpa))
		goto map_error;

	ch->tx_array = vzalloc(MIC_DMA_DESC_RX_SIZE * sizeof(*ch->tx_array));
	if (!ch->tx_array)
		goto tx_error;
	return 0;
tx_error:
	dma_unmap_single(dev, ch->desc_ring_micpa, desc_ring_size,
			 DMA_BIDIRECTIONAL);
map_error:
	kfree(ch->desc_ring);
	return -ENOMEM;
}

static void mic_dma_free_desc_ring(struct mic_dma_chan *ch)
{
	u64 desc_ring_size = MIC_DMA_DESC_RX_SIZE * sizeof(*ch->desc_ring);

	vfree(ch->tx_array);
	desc_ring_size = ALIGN(desc_ring_size, MIC_DMA_ALIGN_BYTES);
	dma_unmap_single(&to_mbus_device(ch)->dev, ch->desc_ring_micpa,
			 desc_ring_size, DMA_BIDIRECTIONAL);
	kfree(ch->desc_ring);
	ch->desc_ring = NULL;
}

static void mic_dma_free_status_dest(struct mic_dma_chan *ch)
{
	dma_unmap_single(&to_mbus_device(ch)->dev, ch->status_dest_micpa,
			 L1_CACHE_BYTES, DMA_BIDIRECTIONAL);
	kfree(ch->status_dest);
}

static int mic_dma_alloc_status_dest(struct mic_dma_chan *ch)
{
	struct device *dev = &to_mbus_device(ch)->dev;

	ch->status_dest = kzalloc(L1_CACHE_BYTES, GFP_KERNEL);
	if (!ch->status_dest)
		return -ENOMEM;
	ch->status_dest_micpa = dma_map_single(dev, ch->status_dest,
					L1_CACHE_BYTES, DMA_BIDIRECTIONAL);
	if (dma_mapping_error(dev, ch->status_dest_micpa)) {
		kfree(ch->status_dest);
		ch->status_dest = NULL;
		return -ENOMEM;
	}
	return 0;
}

static int mic_dma_check_chan(struct mic_dma_chan *ch)
{
	if (mic_dma_read_reg(ch, MIC_DMA_REG_DCHERR) ||
	    mic_dma_read_reg(ch, MIC_DMA_REG_DSTAT) & MIC_DMA_CHAN_QUIESCE) {
		mic_dma_disable_chan(ch);
		mic_dma_chan_mask_intr(ch);
		dev_err(mic_dma_ch_to_device(ch),
			"%s %d error setting up mic dma chan %d\n",
			__func__, __LINE__, ch->ch_num);
		return -EBUSY;
	}
	return 0;
}

static int mic_dma_chan_setup(struct mic_dma_chan *ch)
{
	if (MIC_DMA_CHAN_MIC == ch->owner)
		mic_dma_chan_set_owner(ch);
	mic_dma_disable_chan(ch);
	mic_dma_chan_mask_intr(ch);
	mic_dma_write_reg(ch, MIC_DMA_REG_DCHERRMSK, 0);
	mic_dma_chan_set_desc_ring(ch);
	ch->last_tail = mic_dma_read_reg(ch, MIC_DMA_REG_DTPR);
	ch->head = ch->last_tail;
	ch->issued = 0;
	mic_dma_chan_unmask_intr(ch);
	mic_dma_enable_chan(ch);
	return mic_dma_check_chan(ch);
}

static void mic_dma_chan_destroy(struct mic_dma_chan *ch)
{
	mic_dma_disable_chan(ch);
	mic_dma_chan_mask_intr(ch);
}

static void mic_dma_unregister_dma_device(struct mic_dma_device *mic_dma_dev)
{
	dma_async_device_unregister(&mic_dma_dev->dma_dev);
}

static int mic_dma_setup_irq(struct mic_dma_chan *ch