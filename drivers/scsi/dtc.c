 DMA engine */
	dev_set_drvdata(dev, mdma);
	retval = dma_async_device_register(dma);
	if (retval)
		goto err_free2;

	/* Register with OF helpers for DMA lookups (nonfatal) */
	if (dev->of_node) {
		retval = of_dma_controller_register(dev->of_node,
						of_dma_xlate_by_chan_id, mdma);
		if (retval)
			dev_warn(dev, "Could not register for OF lookup\n");
	}

	return 0;

err_free2:
	if (mdma->is_mpc8308)
		free_irq(mdma->irq2, mdma);
err_free1:
	free_irq(mdma->irq, mdma);
err_dispose2:
	if (mdma->is_mpc8308)
		irq_dispose_mapping(mdma->irq2);
err_dispose1:
	irq_dispose_mapping(mdma->irq);
err:
	return retval;
}

static int mpc_dma_remove(struct platform_device *op)
{
	struct device *dev = &op->dev;
	struct mpc_dma *mdma = dev_get_drvdata(dev);

	if (dev->of_node)
		of_dma_controller_free(dev->of_node);
	dma_async_device_unregister(&mdma->dma);
	if (mdma->is_mpc8308) {
		free_irq(mdma->irq2, mdma);
		irq_dispose_mapping(mdma->irq2);
	}
	free_irq(mdma->irq, mdma);
	irq_dispose_mapping(mdma->irq);

	return 0;
}

static const struct of_device_id mpc_dma_match[] = {
	{ .compatible = "fsl,mpc5121-dma", },
	{ .compatible = "fsl,mpc8308-dma", },
	{},
};
MODULE_DEVICE_TABLE(of, mpc_dma_match);

static struct platform_driver mpc_dma_driver = {
	.probe		= mpc_dma_probe,
	.remove		= mpc_dma_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table	= mpc_dma_match,
	},
};

module_platform_driver(mpc_dma_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Piotr Ziecik <kosmo@semihalf.com>");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * offload engine driver for the Marvell XOR engine
 * Copyright (C) 2007, 2008, Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/memory.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/cpumask.h>
#include <linux/platform_data/dma-mv_xor.h>

#include "dmaengine.h"
#include "mv_xor.h"

enum mv_xor_mode {
	XOR_MODE_IN_REG,
	XOR_MODE_IN_DESC,
};

static void mv_xor_issue_pending(struct dma_chan *chan);

#define to_mv_xor_chan(chan)		\
	container_of(chan, struct mv_xor_chan, dmachan)

#define to_mv_xor_slot(tx)		\
	container_of(tx, struct mv_xor_desc_slot, async_tx)

#define mv_chan_to_devp(chan)           \
	((chan)->dmadev.dev)

static void mv_desc_init(struct mv_xor_desc_slot *desc,
			 dma_addr_t addr, u32 byte_count,
			 enum dma_ctrl_flags flags)
{
	struct mv_xor_desc *hw_desc = desc->hw_desc;

	hw_desc->status = XOR_DESC_DMA_OWNED;
	hw_desc->phy_next_desc = 0;
	/* Enable end-of-descriptor interrupts only for DMA_PREP_INTERRUPT */
	hw_desc->desc_command = (flags & DMA_PREP_INTERRUPT) ?
				XOR_DESC_EOD_INT_EN : 0;
	hw_desc->phy_dest_addr = addr;
	hw_desc->byte_count = byte_count;
}

static void mv_desc_set_mode(struct mv_xor_desc_slot *desc)
{
	struct mv_xor_desc *hw_desc = desc->hw_desc;

	switch (desc->type) {
	case DMA_XOR:
	case DMA_INTERRUPT:
		hw_desc->desc_command |= XOR_DESC_OPERATION_XOR;
		break;
	case DMA_MEMCPY:
		hw_desc->desc_command |= XOR_DESC_OPERATION_MEMCPY;
		break;
	default:
		BUG();
		return;
	}
}

static void mv_desc_set_next_desc(struct mv_xor_desc_slot *desc,
				  u32 next_desc_addr)
{
	struct mv_xor_desc *hw_desc = desc->hw_desc;
	BUG_ON(hw_desc->phy_next_desc);
	hw_desc->phy_next_desc = next_desc_addr;
}

static void mv_desc_set_src_addr(struct mv_xor_desc_slot *desc,
				 int index, dma_addr_t addr)
{
	struct mv_xor_desc *hw_desc = desc->hw_desc;
	hw_desc->phy_src_addr[mv_phy_src_idx(index)] = addr;
	if (desc->type == DMA_XOR)
		hw_desc->desc_command |= (1 << index);
}

static u32 mv_chan_get_current_desc(struct mv_xor_chan *chan)
{
	return readl_relaxed(XOR_CURR_DESC(chan));
}

static void mv_chan_set_next_descriptor(struct mv_xor_chan *chan,
					u32 next_desc_addr)
{
	writel_relaxed(next_desc_addr, XOR_NEXT_DESC(chan));
}

static void mv_chan_unmask_interrupts(struct mv_xor_chan *chan)
{
	u32 val = readl_relaxed(XOR_INTR_MASK(chan));
	val |= XOR_INTR_MASK_VALUE << (chan->idx * 16);
	writel_relaxed(val, XOR_INTR_MASK(chan));
}

static u32 mv_chan_get_intr_cause(struct mv_xor_chan *chan)
{
	u32 intr_cause = readl_relaxed(XOR_INTR_CAUSE(chan));
	intr_cause = (intr_cause >> (chan->idx * 16)) & 0xFFFF;
	return intr_cause;
}

static void mv_chan_clear_eoc_cause(struct mv_xor_chan *chan)
{
	u32 val;

	val = XOR_INT_END_OF_DESC | XOR_INT_END_OF_CHAIN | XOR_INT_STOPPED;
	val = ~(val << (chan->idx * 16));
	dev_dbg(mv_chan_to_devp(chan), "%s, val 0x%08x\n", __func__, val);
	writel_relaxed(val, XOR_INTR_CAUSE(chan));
}

static void mv_chan_clear_err_status(struct mv_xor_chan *chan)
{
	u32 val = 0xFFFF0000 >> (chan->idx * 16);
	writel_relaxed(val, XOR_INTR_CAUSE(chan));
}

static void mv_chan_set_mode(struct mv_xor_chan *chan,
			     enum dma_transaction_type type)
{
	u32 op_mode;
	u32 config = readl_relaxed(XOR_CONFIG(chan));

	switch (type) {
	case DMA_XOR:
		op_mode = XOR_OPERATION_MODE_XOR;
		break;
	case DMA_MEMCPY:
		op_mode = XOR_OPERATION_MODE_MEMCPY;
		break;
	default:
		dev_err(mv_chan_to_devp(chan),
			"error: unsupported operation %d\n",
			type);
		BUG();
		return;
	}

	config &= ~0x7;
	config |= op_mode;

#if defined(__BIG_ENDIAN)
	config |= XOR_DESCRIPTOR_SWAP;
#else
	config &= ~XOR_DESCRIPTOR_SWAP;
#endif

	writel_relaxed(config, XOR_CONFIG(chan));
	chan->current_type = type;
}

static void mv_chan_set_mode_to_desc(struct mv_xor_chan *chan)
{
	u32 op_mode;
	u32 config = readl_relaxed(XOR_CONFIG(chan));

	op_mode = XOR_OPERATION_MODE_IN_DESC;

	config &= ~0x7;
	config |= op_mode;

#if defined(__BIG_ENDIAN)
	config |= XOR_DESCRIPTOR_SWAP;
#else
	config &= ~XOR_DESCRIPTOR_SWAP;
#endif

	writel_relaxed(config, XOR_CONFIG(chan));
}

static void mv_chan_activate(struct mv_xor_chan *chan)
{
	dev_dbg(mv_chan_to_devp(chan), " activate chan.\n");

	/* writel ensures all descriptors are flushed before activation */
	writel(BIT(0), XOR_ACTIVATION(chan));
}

static char mv_chan_is_busy(struct mv_xor_chan *chan)
{
	u32 state = readl_relaxed(XOR_ACTIVATION(chan));

	state = (state >> 4) & 0x3;

	return (state == 1) ? 1 : 0;
}

/*
 * mv_chan_start_new_chain - program the engine to operate on new
 * chain headed by sw_desc
 * Caller must hold &mv_chan->lock while calling this function
 */
static void mv_chan_start_new_chain(struct mv_xor_chan *mv_chan,
				    struct mv_xor_desc_slot *sw_desc)
{
	dev_dbg(mv_chan_to_devp(mv_chan), "%s %d: sw_desc %p\n",
		__func__, __LINE__, sw_desc);

	/* set the hardware chain */
	mv_chan_set_next_descriptor(mv_chan, sw_desc->async_tx.phys);

	mv_chan->pending++;
	mv_xor_issue_pending(&mv_chan->dmachan);
}

static dma_cookie_t
mv_desc_run_tx_complete_actions(struct mv_xor_desc_slot *desc,
				struct mv_xor_chan *mv_chan,
				dma_cookie_t cookie)
{
	BUG_ON(desc->async_tx.cookie < 0);

	if (desc->async_tx.cookie > 0) {
		cookie = desc->async_tx.cookie;

		/* call the callback (must not sleep or submit new
		 * operations to this channel)
		 */
		if (desc->async_tx.callback)
			desc->async_tx.callback(
				desc->async_tx.callback_param);

		dma_descriptor_unmap(&desc->async_tx);
	}

	/* run dependent operations */
	dma_run_dependencies(&desc->async_tx);

	return cookie;
}

static int
mv_chan_clean_completed_slots(struct mv_xor_chan *mv_chan)
{
	struct mv_xor_desc_slot *iter, *_iter;

	dev_dbg(mv_chan_to_devp(mv_chan), "%s %d\n", __func__, __LINE__);
	list_for_each_entry_safe(iter, _iter, &mv_chan->completed_slots,
				 node) {

		if (async_tx_test_ack(&iter->async_tx))
			list_move_tail(&iter->node, &mv_chan->free_slots);
	}
	return 0;
}

static int
mv_desc_clean_slot(struct mv_xor_desc_slot *desc,
		   struct mv_xor_chan *mv_chan)
{
	dev_dbg(mv_chan_to_devp(mv_chan), "%s %d: desc %p flags %d\n",
		__func__, __LINE__, desc, desc->async_tx.flags);

	/* the client is allowed to attach dependent operations
	 * until 'ack' is set
	 */
	if (!async_tx_test_ack(&desc->async_tx))
		/* move this slot to the completed_slots */
		list_move_tail(&desc->node, &mv_chan->completed_slots);
	else
		list_move_tail(&desc->node, &mv_chan->free_slots);

	return 0;
}

/* This function must be called with the mv_xor_chan spinlock held */
static void mv_chan_slot_cleanup(struct mv_xor_chan *mv_chan)
{
	struct mv_xor_desc_slot *iter, *_iter;
	dma_cookie_t cookie = 0;
	int busy = mv_chan_is_busy(mv_chan);
	u32 current_desc = mv_chan_get_current_desc(mv_chan);
	int current_cleaned = 0;
	struct mv_xor_desc *hw_desc;

	dev_dbg(mv_chan_to_devp(mv_chan), "%s %d\n", __func__, __LINE__);
	dev_dbg(mv_chan_to_devp(mv_chan), "current_desc %x\n", current_desc);
	mv_chan_clean_completed_slots(mv_chan);

	/* free completed slots from the chain starting with
	 * the oldest descriptor
	 */

	list_for_each_entry_safe(iter, _iter, &mv_chan->chain,
				 node) {

		/* clean finished descriptors */
		hw_desc = iter->hw_desc;
		if (hw_desc->status & XOR_DESC_SUCCESS) {
			cookie = mv_desc_run_tx_complete_actions(iter, mv_chan,
								 cookie);

			/* done processing desc, clean slot */
			mv_desc_clean_slot(iter, mv_chan);

			/* break if we did cleaned the current */
			if (iter->async_tx.phys == current_desc) {
				current_cleaned = 1;
				break;
			}
		} else {
			if (iter->async_tx.phys == current_desc) {
				current_cleaned = 0;
				break;
			}
		}
	}

	if ((busy == 0) && !list_empty(&mv_chan->chain)) {
		if (current_cleaned) {
			/*
			 * current descriptor clean