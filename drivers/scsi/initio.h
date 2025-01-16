_relaxed(XOR_ACTIVATION(chan));
	dev_err(mv_chan_to_devp(chan), "activation   0x%08x\n", val);

	val = readl_relaxed(XOR_INTR_CAUSE(chan));
	dev_err(mv_chan_to_devp(chan), "intr cause   0x%08x\n", val);

	val = readl_relaxed(XOR_INTR_MASK(chan));
	dev_err(mv_chan_to_devp(chan), "intr mask    0x%08x\n", val);

	val = readl_relaxed(XOR_ERROR_CAUSE(chan));
	dev_err(mv_chan_to_devp(chan), "error cause  0x%08x\n", val);

	val = readl_relaxed(XOR_ERROR_ADDR(chan));
	dev_err(mv_chan_to_devp(chan), "error addr   0x%08x\n", val);
}

static void mv_chan_err_interrupt_handler(struct mv_xor_chan *chan,
					  u32 intr_cause)
{
	if (intr_cause & XOR_INT_ERR_DECODE) {
		dev_dbg(mv_chan_to_devp(chan), "ignoring address decode error\n");
		return;
	}

	dev_err(mv_chan_to_devp(chan), "error on chan %d. intr cause 0x%08x\n",
		chan->idx, intr_cause);

	mv_chan_dump_regs(chan);
	WARN_ON(1);
}

static irqreturn_t mv_xor_interrupt_handler(int irq, void *data)
{
	struct mv_xor_chan *chan = data;
	u32 intr_cause = mv_chan_get_intr_cause(chan);

	dev_dbg(mv_chan_to_devp(chan), "intr cause %x\n", intr_cause);

	if (intr_cause & XOR_INTR_ERRORS)
		mv_chan_err_interrupt_handler(chan, intr_cause);

	tasklet_schedule(&chan->irq_tasklet);

	mv_chan_clear_eoc_cause(chan);

	return IRQ_HANDLED;
}

static void mv_xor_issue_pending(struct dma_chan *chan)
{
	struct mv_xor_chan *mv_chan = to_mv_xor_chan(chan);

	if (mv_chan->pending >= MV_XOR_THRESHOLD) {
		mv_chan->pending = 0;
		mv_chan_activate(mv_chan);
	}
}

/*
 * Perform a transaction to verify the HW works.
 */

static int mv_chan_memcpy_self_test(struct mv_xor_chan *mv_chan)
{
	int i, ret;
	void *src, *dest;
	dma_addr_t src_dma, dest_dma;
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;
	struct dma_async_tx_descriptor *tx;
	struct dmaengine_unmap_data *unmap;
	int err = 0;

	src = kmalloc(sizeof(u8) * PAGE_SIZE, GFP_KERNEL);
	if (!src)
		return -ENOMEM;

	dest = kzalloc(sizeof(u8) * PAGE_SIZE, GFP_KERNEL);
	if (!dest) {
		kfree(src);
		return -ENOMEM;
	}

	/* Fill in src buffer */
	for (i = 0; i < PAGE_SIZE; i++)
		((u8 *) src)[i] = (u8)i;

	dma_chan = &mv_chan->dmachan;
	if (mv_xor_alloc_chan_resources(dma_chan) < 1) {
		err = -ENODEV;
		goto out;
	}

	unmap = dmaengine_get_unmap_data(dma_chan->device->dev, 2, GFP_KERNEL);
	if (!unmap) {
		err = -ENOMEM;
		goto free_resources;
	}

	src_dma = dma_map_page(dma_chan->device->dev, virt_to_page(src), 0,
				 PAGE_SIZE, DMA_TO_DEVICE);
	unmap->addr[0] = src_dma;

	ret = dma_mapping_error(dma_chan->device->dev, src_dma);
	if (ret) {
		err = -ENOMEM;
		goto free_resources;
	}
	unmap->to_cnt = 1;

	dest_dma = dma_map_page(dma_chan->device->dev, virt_to_page(dest), 0,
				  PAGE_SIZE, DMA_FROM_DEVICE);
	unmap->addr[1] = dest_dma;

	ret = dma_mapping_error(dma_chan->device->dev, dest_dma);
	if (ret) {
		err = -ENOMEM;
		goto free_resources;
	}
	unmap->from_cnt = 1;
	unmap->len = PAGE_SIZE;

	tx = mv_xor_prep_dma_memcpy(dma_chan, dest_dma, src_dma,
				    PAGE_SIZE, 0);
	if (!tx) {
		dev_err(dma_chan->device->dev,
			"Self-test cannot prepare operation, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	cookie = mv_xor_tx_submit(tx);
	if (dma_submit_error(cookie)) {
		dev_err(dma_chan->device->dev,
			"Self-test submit error, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	mv_xor_issue_pending(dma_chan);
	async_tx_ack(tx);
	msleep(1);

	if (mv_xor_status(dma_chan, cookie, NULL) !=
	    DMA_COMPLETE) {
		dev_err(dma_chan->device->dev,
			"Self-test copy timed out, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	dma_sync_single_for_cpu(dma_chan->device->dev, dest_dma,
				PAGE_SIZE, DMA_FROM_DEVICE);
	if (memcmp(src, dest, PAGE_SIZE)) {
		dev_err(dma_chan->device->dev,
			"Self-test copy failed compare, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

free_resources:
	dmaengine_unmap_put(unmap);
	mv_xor_free_chan_resources(dma_chan);
out:
	kfree(src);
	kfree(dest);
	return err;
}

#define MV_XOR_NUM_SRC_TEST 4 /* must be <= 15 */
static int
mv_chan_xor_self_test(struct mv_xor_chan *mv_chan)
{
	int i, src_idx, ret;
	struct page *dest;
	struct page *xor_srcs[MV_XOR_NUM_SRC_TEST];
	dma_addr_t dma_srcs[MV_XOR_NUM_SRC_TEST];
	dma_addr_t dest_dma;
	struct dma_async_tx_descriptor *tx;
	struct dmaengine_unmap_data *unmap;
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;
	u8 cmp_byte = 0;
	u32 cmp_word;
	int err = 0;
	int src_count = MV_XOR_NUM_SRC_TEST;

	for (src_idx = 0; src_idx < src_count; src_idx++) {
		xor_srcs[src_idx] = alloc_page(GFP_KERNEL);
		if (!xor_srcs[src_idx]) {
			while (src_idx--)
				__free_page(xor_srcs[src_idx]);
			return -ENOMEM;
		}
	}

	dest = alloc_page(GFP_KERNEL);
	if (!dest) {
		while (src_idx--)
			__free_page(xor_srcs[src_idx]);
		return -ENOMEM;
	}

	/* Fill in src buffers */
	for (src_idx = 0; src_idx < src_count; src_idx++) {
		u8 *ptr = page_address(xor_srcs[src_idx]);
		for (i = 0; i < PAGE_SIZE; i++)
			ptr[i] = (1 << src_idx);
	}

	for (src_idx = 0; src_idx < src_count; src_idx++)
		cmp_byte ^= (u8) (1 << src_idx);

	cmp_word = (cmp_byte << 24) | (cmp_byte << 16) |
		(cmp_byte << 8) | cmp_byte;

	memset(page_address(dest), 0, PAGE_SIZE);

	dma_chan = &mv_chan->dmachan;
	if (mv_xor_alloc_chan_resources(dma_chan) < 1) {
		err = -ENODEV;
		goto out;
	}

	unmap = dmaengine_get_unmap_data(dma_chan->device->dev, src_count + 1,
					 GFP_KERNEL);
	if (!unmap) {
		err = -ENOMEM;
		goto free_resources;
	}

	/* test xor */
	for (i = 0; i < src_count; i++) {
		unmap->addr[i] = dma_map_page(dma_chan->device->dev, xor_srcs[i],
					      0, PAGE_SIZE, DMA_TO_DEVICE);
		dma_srcs[i] = unmap->addr[i];
		ret = dma_mapping_error(dma_chan->device->dev, unmap->addr[i]);
		if (ret) {
			err = -ENOMEM;
			goto free_resources;
		}
		unmap->to_cnt++;
	}

	unmap->addr[src_count] = dma_map_page(dma_chan->device->dev, dest, 0, PAGE_SIZE,
				      DMA_FROM_DEVICE);
	dest_dma = unmap->addr[src_count];
	ret = dma_mapping_error(dma_chan->device->dev, unmap->addr[src_count]);
	if (ret) {
		err = -ENOMEM;
		goto free_resources;
	}
	unmap->from_cnt = 1;
	unmap->len = PAGE_SIZE;

	tx = mv_xor_prep_dma_xor(dma_chan, dest_dma, dma_srcs,
				 src_count, PAGE_SIZE, 0);
	if (!tx) {
		dev_err(dma_chan->device->dev,
			"Self-test cannot prepare operation, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	cookie = mv_xor_tx_submit(tx);
	if (dma_submit_error(cookie)) {
		dev_err(dma_chan->device->dev,
			"Self-test submit error, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	mv_xor_issue_pending(dma_chan);
	async_tx_ack(tx);
	msleep(8);

	if (mv_xor_status(dma_chan, cookie, NULL) !=
	    DMA_COMPLETE) {
		dev_err(dma_chan->device->dev,
			"Self-test xor timed out, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	dma_sync_single_for_cpu(dma_chan->device->dev, dest_dma,
				PAGE_SIZE, DMA_FROM_DEVICE);
	for (i = 0; i < (PAGE_SIZE / sizeof(u32)); i++) {
		u32 *ptr = page_address(dest);
		if (ptr[i] != cmp_word) {
			dev_err(dma_chan->device->dev,
				"Self-test xor failed compare, disabling. index %d, data %x, expected %x\n",
				i, ptr[i], cmp_word);
			err = -ENODEV;
			goto free_resources;
		}
	}

free_resources:
	dmaengine_unmap_put(unmap);
	mv_xor_free_chan_resources(dma_chan);
out:
	src_idx = src_count;
	while (src_idx--)
		__free_page(xor_srcs[src_idx]);
	__free_page(dest);
	return err;
}

static int mv_xor_channel_remove(struct mv_xor_chan *mv_chan)
{
	struct dma_chan *chan, *_chan;
	struct device *dev = mv_chan->dmadev.dev;

	dma_async_device_unregister(&mv_chan->dmadev);

	dma_free_coherent(dev, MV_XOR_POOL_SIZE,
			  mv_chan->dma_desc_pool_virt, mv_chan->dma_desc_pool);
	dma_unmap_single(dev, mv_chan->dummy_src_addr,
			 MV_XOR_MIN_BYTE_COUNT, DMA_FROM_DEVICE);
	dma_unmap_single(dev, mv_chan->dummy_dst_addr,
			 MV_XOR_MIN_BYTE_COUNT, DMA_TO_DEVICE);

	list_for_each_entry_safe(chan, _chan, &mv_chan->dmadev.channels,
				 device_node) {
		list_del(&chan->device_node);
	}

	free_irq(mv_chan->irq, mv_chan);

	return 0;
}

static struct mv_xor_chan *
mv_xor_channel_add(struct mv_xor_device *xordev,
		   struct platform_device *pdev,
		   int idx, dma_cap_mask_t cap_mask, int irq, int op_in_desc)
{
	int ret = 0;
	struct mv_xor_chan *mv_chan;
	struct dma_device *dma_dev;

	mv_chan = devm_kzalloc(&pdev->dev, sizeof(*mv_chan), GFP_KERNEL);
	if (!mv_chan)
		return ERR_PTR(-ENOMEM);

	mv_chan->idx = idx;
	mv_chan->irq = irq;
	mv_chan->op_in_desc = op_in_desc;

	dma_dev = &mv_chan->dmadev;

	/*
	 * These source and destination dummy buffers are used to implement
	 * a DMA_INTERRUPT operation as a minimum-sized XOR operation.
	 * Hence, we only need to map the buffers at initialization-time.
	 */
	mv_chan->dummy_src_addr = dma_map_single(dma_dev->dev,
		mv_chan->dummy_src, MV_XOR_MIN_BYTE_COUNT, DMA_FROM_DEVICE);
	mv_chan->dummy_dst_addr = dma_map_single(dma_dev->dev,
		mv_chan->dummy_dst, MV_XOR_MIN_BYTE_COUNT, DMA_TO_DEVICE);

	/* allocate coherent memory for hardware descriptors
	 * note: writecombine gives slightly better performance, but
	 * requires that we explicitly flush the writes
	 */
	mv_chan->dma_desc_pool_virt =
	  dma_alloc_writecombine(&pdev->dev, MV_XOR_POOL_SIZE,
				 &mv_chan->dma_desc_pool, GFP_KERNEL);
	if (!mv_chan->dma_desc_pool_virt)
		return ERR_PTR(-ENOMEM);

	/* discover transaction capabilites from the platform data */
	dma_dev->cap_mask = cap_mask;

	INIT_LIST_HEAD(&dma_dev->channels);

	/* set base routines */
	dma_dev->device_alloc_chan_resources = mv_xor_alloc_chan_resources;
	dma_dev->device_free_chan_resources = mv_xor_free_chan_resources;
	dma_dev->device_tx_status = mv_xor_status;
	dma_dev->device_issue_pending = mv_xor_issue_pending;
	dma_dev->dev = &pdev->dev;

	/* set prep routines based on capability */
	if (dma_has_cap(DMA_INTERRUPT, dma_dev->cap_mask))
		dma_dev->device_prep_dma_interrupt = mv_xor_prep_dma_interrupt;
	if (dma_has_cap(DMA_MEMCPY, dma_dev->cap_mask))
		dma_dev->device_prep_dma_memcpy = mv_xor_prep_dma_memcpy;
	if (dma_has_cap(DMA_XOR, dma_dev->cap_mask)) {
		dma_dev->max_xor = 8;
		dma_dev->device_prep_dma_xor = mv_xor_prep_dma_xor;
	}

	mv_chan->mmr_base = xordev->xor_base;
	mv_chan->mmr_high_base = xordev->xor_high_base;
	tasklet_init(&mv_chan->irq_tasklet, mv_xor_tasklet, (unsigned long)
		     mv_chan);

	/* clear errors before enabling interrupts */
	mv_chan_clear_err_status(mv_chan);

	ret = request_irq(mv_chan->irq, mv_xor_interrupt_handler,
			  0, dev_name(&pdev->dev), mv_chan);
	if (ret)
		goto err_free_dma;

	mv_chan_unmask_interrupts(mv_chan);

	if (mv_chan->op_in_desc == XOR_MODE_IN_DESC)
		mv_chan_set_mode_to_desc(mv_chan);
	else
		mv_chan_set_mode(mv_chan, DMA_XOR);

	spin_lock_init(&mv_chan->lock);
	INIT_LIST_HEAD(&mv_chan->chain);
	INIT_LIST_HEAD(&mv_chan->completed_slots);
	INIT_LIST_HEAD(&mv_chan->free_slots);
	INIT_LIST_HEAD(&mv_chan->allocated_slots);
	mv_chan->dmachan.device = dma_dev;
	dma_cookie_init(&mv_chan->dmachan);

	list_add_tail(&mv_chan->dmachan.device_node, &dma_dev->channels);

	if (dma_has_cap(DMA_MEMCPY, dma_dev->cap_mask)) {
		ret = mv_chan_memcpy_self_test(mv_chan);
		dev_dbg(&pdev->dev, "memcpy self test returned %d\n", ret);
		if (ret)
			goto err_free_irq;
	}

	if (dma_has_cap(DMA_XOR, dma_dev->cap_mask)) {
		ret = mv_chan_xor_self_test(mv_chan);
		dev_dbg(&pdev->dev, "xor self test returned %d\n", ret);
		if (ret)
			goto err_free_irq;
	}

	dev_info(&pdev->dev, "Marvell XOR (%s): ( %s%s%s)\n",
		 mv_chan->op_in_desc ? "Descriptor Mode" : "Registers Mode",
		 dma_has_cap(DMA_XOR, dma_dev->cap_mask) ? "xor " : "",
		 dma_has_cap(DMA_MEMCPY, dma_dev->cap_mask) ? "cpy " : "",
		 dma_has_cap(DMA_INTERRUPT, dma_dev->cap_mask) ? "intr " : "");

	dma_async_device_register(dma_dev);
	return mv_chan;

err_free_irq:
	free_irq(mv_chan->irq, mv_chan);
 err_free_dma:
	dma_free_coherent(&pdev->dev, MV_XOR_POOL_SIZE,
			  mv_chan->dma_desc_pool_virt, mv_chan->dma_desc_pool);
	return ERR_PTR(ret);
}

static void
mv_xor_conf_mbus_windows(struct mv_xor_device *xordev,
			 const struct mbus_dram_target_info *dram)
{
	void __iomem *base = xordev->xor_high_base;
	u32 win_enable = 0;
	int i;

	for (i = 0; i < 8; i++) {
		writel(0, base + WINDOW_BASE(i));
		writel(0, base + WINDOW_SIZE(i));
		if (i < 4)
			writel(0, base + WINDOW_REMAP_HIGH(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		writel((cs->base & 0xffff0000) |
		       (cs->mbus_attr << 8) |
		       dram->mbus_dram_target_id, base + WINDOW_BASE(i));
		writel((cs->size - 1) & 0xffff0000, base + WINDOW_SIZE(i));

		win_enable |= (1 << i);
		win_enable |= 3 << (16 + (2 * i));
	}

	writel(win_enable, base + WINDOW_BAR_ENABLE(0));
	writel(win_enable, base + WINDOW_BAR_ENABLE(1));
	writel(0, base + WINDOW_OVERRIDE_CTRL(0));
	writel(0, base + WINDOW_OVERRIDE_CTRL(1));
}

static const struct of_device_id mv_xor_dt_ids[] = {
	{ .compatible = "marvell,orion-xor", .data = (void *)XOR_MODE_IN_REG },
	{ .compatible = "marvell,armada-380-xor", .data = (void *)XOR_MODE_IN_DESC },
	{},
};

static unsigned int mv_xor_engine_count;

static int mv_xor_probe(struct platform_device *pdev)
{
	const struct mbus_dram_target_info *dram;
	struct mv_xor_device *xordev;
	struct mv_xor_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct resource *res;
	unsigned int max_engines, max_channels;
	int i, ret;
	int op_in_desc;

	dev_notice(&pdev->dev, "Marvell shared XOR driver\n");

	xordev = devm_kzalloc(&pdev->dev, sizeof(*xordev), GFP_KERNEL);
	if (!xordev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	xordev->xor_base = devm_ioremap(&pdev->dev, res->start,
					resource_size(res));
	if (!xordev->xor_base)
		return -EBUSY;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -ENODEV;

	xordev->xor_high_base = devm_ioremap(&pdev->dev, res->start,
					     resource_size(res));
	if (!xordev->xor_high_base)
		return -EBUSY;

	platform_set_drvdata(pdev, xordev);

	/*
	 * (Re-)program MBUS remapping windows if we are asked to.
	 */
	dram = mv_mbus_dram_info();
	if (dram)
		mv_xor_conf_mbus_windows(xordev, dram);

	/* Not all platforms can gate the clock, so it is not
	 * an error if the clock does not exists.
	 */
	xordev->clk = clk_get(&pdev->dev, NULL);
	if (!IS_ERR(xordev->clk))
		clk_prepare_enable(xordev->clk);

	/*
	 * We don't want to have more than one channel per CPU in
	 * order for async_tx to perform well. So we limit the number
	 * of engines and channels so that we take into account this
	 * constraint. Note that we also want to use channels from
	 * separate engines when possible.
	 */
	max_engines = num_present_cpus();
	max_channels = min_t(unsigned int,
			     MV_XOR_MAX_CHANNELS,
			     DIV_ROUND_UP(num_present_cpus(), 2));

	if (mv_xor_engine_count >= max_engines)
		return 0;

	if (pdev->dev.of_node) {
		struct device_node *np;
		int i = 0;
		const struct of_device_id *of_id =
			of_match_device(mv_xor_dt_ids,
					&pdev->dev);

		for_each_child_of_node(pdev->dev.of_node, np) {
			struct mv_xor_chan *chan;
			dma_cap_mask_t cap_mask;
			int irq;
			op_in_desc = (int)of_id->data;

			if (i >= max_channels)
				continue;

			dma_cap_zero(cap_mask);
			dma_cap_set(DMA_MEMCPY, cap_mask);
			dma_cap_set(DMA_XOR, cap_mask);
			dma_cap_set(DMA_INTERRUPT, cap_mask);

			irq = irq_of_parse_and_map(np, 0);
			if (!irq) {
				ret = -ENODEV;
				goto err_channel_add;
			}

			chan = mv_xor_channel_add(xordev, pdev, i,
						  cap_mask, irq, op_in_desc);
			if (IS_ERR(chan)) {
				ret = PTR_ERR(chan);
				irq_dispose_mapping(irq);
				goto err_channel_add;
			}

			xordev->channels[i] = chan;
			i++;
		}
	} else if (pdata && pdata->channels) {
		for (i = 0; i < max_channels; i++) {
			struct mv_xor_channel_data *cd;
			struct mv_xor_chan *chan;
			int irq;

			cd = &pdata->channels[i];
			if (!cd) {
				ret = -ENODEV;
				goto err_channel_add;
			}

			irq = platform_get_irq(pdev, i);
			if (irq < 0) {
				ret = irq;
				goto err_channel_add;
			}

			chan = mv_xor_channel_add(xordev, pdev, i,
						  cd->cap_mask, irq,
						  XOR_MODE_IN_REG);
			if (IS_ERR(chan)) {
				ret = PTR_ERR(chan);
				goto err_channel_add;
			}

			xordev->channels[i] = chan;
		}
	}

	return 0;

err_channel_add:
	for (i = 0; i < MV_XOR_MAX_CHANNELS; i++)
		if (xordev->channels[i]) {
			mv_xor_channel_remove(xordev->channels[i]);
			if (pdev->dev.of_node)
				irq_dispose_mapping(xordev->channels[i]->irq);
		}

	if (!IS_ERR(xordev->clk)) {
		clk_disable_unprepare(xordev->clk);
		clk_put(xordev->clk);
	}

	return ret;
}

static struct platform_driver mv_xor_driver = {
	.probe		= mv_xor_probe,
	.driver		= {
		.name	        = MV_XOR_NAME,
		.of_match_table = of_match_ptr(mv_xor_dt_ids),
	},
};


static int __init mv_xor_init(void)
{
	return platform_driver_register(&mv_xor_driver);
}
device_initcall(mv_xor_init);

/*
MODULE_AUTHOR("Saeed Bishara <saeed@marvell.com>");
MODULE_DESCRIPTION("DMA engine driver for Marvell's XOR engine");
MODULE_LICENSE("GPL");
*/
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * Copyright (C) 2007, 2008, Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef MV_XOR_H
#define MV_XOR_H

#include <linux/types.h>
#include <linux/io.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>

#define MV_XOR_POOL_SIZE		(MV_XOR_SLOT_SIZE * 3072)
#define MV_XOR_SLOT_SIZE		64
#define MV_XOR_THRESHOLD		1
#define MV_XOR_MAX_CHANNELS             2

#define MV_XOR_MIN_BYTE_COUNT		SZ_128
#define MV_XOR_MAX_BYTE_COUNT		(SZ_16M - 1)

/* Values for the XOR_CONFIG register */
#define XOR_OPERATION_MODE_XOR		0
#define XOR_OPERATION_MODE_MEMCPY	2
#define XOR_OPERATION_MODE_IN_DESC      7
#define XOR_DESCRIPTOR_SWAP		BIT(14)
#define XOR_DESC_SUCCESS		0x40000000

#define XOR_DESC_OPERATION_XOR          (0 << 24)
#define XOR_DESC_OPERATION_CRC32C       (1 << 24)
#define XOR_DESC_OPERATION_MEMCPY       (2 << 24)

#define XOR_DESC_DMA_OWNED		BIT(31)
#define XOR_DESC_EOD_INT_EN		BIT(31)

#define XOR_CURR_DESC(chan)	(chan->mmr_high_base + 0x10 + (chan->idx * 4))
#define XOR_NEXT_DESC(chan)	(chan->mmr_high_base + 0x00 + (chan->idx * 4))
#define XOR_BYTE_COUNT(chan)	(chan->mmr_high_base + 0x20 + (chan->idx * 4))
#define XOR_DEST_POINTER(chan)	(chan->mmr_high_base + 0xB0 + (chan->idx * 4))
#define XOR_BLOCK_SIZE(chan)	(chan->mmr_high_base + 0xC0 + (chan->idx * 4))
#define XOR_INIT_VALUE_LOW(chan)	(chan->mmr_high_base + 0xE0)
#define XOR_INIT_VALUE_HIGH(chan)	(chan->mmr_high_base + 0xE4)

#define XOR_CONFIG(chan)	(chan->mmr_base + 0x10 + (chan->idx * 4))
#define XOR_ACTIVATION(chan)	(chan->mmr_base + 0x20 + (chan->idx * 4))
#define XOR_INTR_CAUSE(chan)	(chan->mmr_base + 0x30)
#define XOR_INTR_MASK(chan)	(chan->mmr_base + 0x40)
#define XOR_ERROR_CAUSE(chan)	(chan->mmr_base + 0x50)
#define XOR_ERROR_ADDR(chan)	(chan->mmr_base + 0x60)

#define XOR_INT_END_OF_DESC	BIT(0)
#define XOR_INT_END_OF_CHAIN	BIT(1)
#define XOR_INT_STOPPED		BIT(2)
#define XOR_INT_PAUSED		BIT(3)
#define XOR_INT_ERR_DECODE	BIT(4)
#define XOR_INT_ERR_RDPROT	BIT(5)
#define XOR_INT_ERR_WRPROT	BIT(6)
#define XOR_INT_ERR_OWN		BIT(7)
#define XOR_INT_ERR_PAR		BIT(8)
#define XOR_INT_ERR_MBUS	BIT(9)

#define XOR_INTR_ERRORS		(XOR_INT_ERR_DECODE | XOR_INT_ERR_RDPROT | \
				 XOR_INT_ERR_WRPROT | XOR_INT_ERR_OWN    | \
				 XOR_INT_ERR_PAR    | XOR_INT_ERR_MBUS)

#define XOR_INTR_MASK_VALUE	(XOR_INT_END_OF_DESC | XOR_INT_END_OF_CHAIN | \
				 XOR_INT_STOPPED     | XOR_INTR_ERRORS)

#define WINDOW_BASE(w)		(0x50 + ((w) << 2))
#define WINDOW_SIZE(w)		(0x70 + ((w) << 2))
#define WINDOW_REMAP_HIGH(w)	(0x90 + ((w) << 2))
#define WINDOW_BAR_ENABLE(chan)	(0x40 + ((chan) << 2))
#define WINDOW_OVERRIDE_CTRL(chan)	(0xA0 + ((chan) << 2))

struct mv_xor_device {
	void __iomem	     *xor_base;
	void __iomem	     *xor_high_base;
	struct clk	     *clk;
	struct mv_xor_chan   *channels[MV_XOR_MAX_CHANNELS];
};

/**
 * struct mv_xor_chan - internal representation of a XOR channel
 * @pending: allows batching of hardware operations
 * @lock: serializes enqueue/dequeue operations to the descriptors pool
 * @mmr_base: memory mapped register base
 * @idx: the index of the xor channel
 * @chain: device chain view of the descriptors
 * @free_slots: free slots usable by the channel
 * @allocated_slots: slots allocated by the driver
 * @completed_slots: slots completed by HW but still need to be acked
 * @device: parent device
 * @common: common dmaengine channel object members
 * @slots_allocated: records the actual size of the descriptor slot pool
 * @irq_tasklet: bottom half where mv_xor_slot_cleanup runs
 * @op_in_desc: new mode of driver, each op is writen to descriptor.
 */
struct mv_xor_chan {
	int			pending;
	spinlock_t		lock; /* protects the descriptor slot pool */
	void __iomem		*mmr_base;
	void __iomem		*mmr_high_base;
	unsigned int		idx;
	int                     irq;
	enum dma_transaction_type	current_type;
	struct list_head	chain;
	struct list_head	free_slots;
	struct list_head	allocated_slots;
	struct list_head	completed_slots;
	dma_addr_t		dma_desc_pool;
	void			*dma_desc_pool_virt;
	size_t                  pool_size;
	struct dma_device	dmadev;
	struct dma_chan		dmachan;
	int			slots_allocated;
	struct tasklet_struct	irq_tasklet;
	int                     op_in_desc;
	char			dummy_src[MV_XOR_MIN_BYTE_COUNT];
	char			dummy_dst[MV_XOR_MIN_BYTE_COUNT];
	dma_addr_t		dummy_src_addr, dummy_dst_addr;
};

/**
 * struct mv_xor_desc_slot - software descriptor
 * @node: node on the mv_xor_chan lists
 * @hw_desc: virtual address of the hardware descriptor chain
 * @phys: hardware address of the hardware descriptor chain
 * @slot_used: slot in use or not
 * @idx: pool index
 * @tx_list: list of slots that make up a multi-descriptor transaction
 * @async_tx: support for the async_tx api
 */
struct mv_xor_desc_slot {
	struct list_head	node;
	enum dma_transaction_type	type;
	void			*hw_desc;
	u16			idx;
	struct dma_async_tx_descriptor	async_tx;
};

/*
 * This structure describes XOR descriptor size 64bytes. The
 * mv_phy_src_idx() macro must be used when indexing the values of the
 * phy_src_addr[] array. This is due to the fact that the 'descriptor
 * swap' feature, used on big endian systems, swaps descriptors data
 * within blocks of 8 bytes. So two consecutive values of the
 * phy_src_addr[] array are actually swapped in big-endian, which
 * explains the different mv_phy_src_idx() implementation.
 */
#if defined(__LITTLE_ENDIAN)
struct mv_xor_desc {
	u32 status;		/* descriptor execution status */
	u32 crc32_result;	/* result of CRC-32 calculation */
	u32 desc_command;	/* type of operation to be carried out */
	u32 phy_next_desc;	/* next descriptor address pointer */
	u32 byte_count;		/* size of src/dst blocks in bytes */
	u32 phy_dest_addr;	/* destination block address */
	u32 phy_src_addr[8];	/* source block addresses */
	u32 reserved0;
	u32 reserved1;
};
#define mv_phy_src_idx(src_idx) (src_idx)
#else
struct mv_xor_desc {
	u32 crc32_result;	/* result of CRC-32 calculation */
	u32 status;		/* descriptor execution status */
	u32 phy_next_desc;	/* next descriptor address pointer */
	u32 desc_command;	/* type of operation to be carried out */
	u32 phy_dest_addr;	/* destination block address */
	u32 byte_count;		/* size of src/dst blocks in bytes */
	u32 phy_src_addr[8];	/* source block addresses */
	u32 reserved1;
	u32 reserved0;
};
#define mv_phy_src_idx(src_idx) (src_idx ^ 1)
#endif

#define to_mv_sw_desc(addr_hw_desc)		\
	container_of(addr_hw_desc, struct mv_xor_desc_slot, hw_desc)

#define mv_hw_desc_slot_idx(hw_desc, idx)	\
	((void *)(((unsigned long)hw_desc) + ((idx) << 5)))

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Refer to drivers/dma/imx-sdma.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dmaengine.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/stmp_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/list.h>

#include <asm/irq.h>

#include "dmaengine.h"

/*
 * NOTE: The term "PIO" throughout the mxs-dma implementation means
 * PIO mode of mxs apbh-dma and apbx-dma.  With this working mode,
 * dma can program the controller registers of peripheral devices.
 */

#define dma_is_apbh(mxs_dma)	((mxs_dma)->type == MXS_DMA_APBH)
#define apbh_is_old(mxs_dma)	((mxs_dma)->dev_id == IMX23_DMA)

#define HW_APBHX_CTRL0				0x000
#define BM_APBH_CTRL0_APB_BURST8_EN		(1 << 29)
#define BM_APBH