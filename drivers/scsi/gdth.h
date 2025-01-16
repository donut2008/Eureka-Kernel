     struct dma_slave_config *config)
{
	struct nbpf_channel *chan = nbpf_to_chan(dchan);

	dev_dbg(dchan->device->dev, "Entry %s\n", __func__);

	/*
	 * We could check config->slave_id to match chan->terminal here,
	 * but with DT they would be coming from the same source, so
	 * such a check would be superflous
	 */

	chan->slave_dst_addr = config->dst_addr;
	chan->slave_dst_width = nbpf_xfer_size(chan->nbpf,
					       config->dst_addr_width, 1);
	chan->slave_dst_burst = nbpf_xfer_size(chan->nbpf,
					       config->dst_addr_width,
					       config->dst_maxburst);
	chan->slave_src_addr = config->src_addr;
	chan->slave_src_width = nbpf_xfer_size(chan->nbpf,
					       config->src_addr_width, 1);
	chan->slave_src_burst = nbpf_xfer_size(chan->nbpf,
					       config->src_addr_width,
					       config->src_maxburst);

	return 0;
}

static struct dma_async_tx_descriptor *nbpf_prep_sg(struct nbpf_channel *chan,
		struct scatterlist *src_sg, struct scatterlist *dst_sg,
		size_t len, enum dma_transfer_direction direction,
		unsigned long flags)
{
	struct nbpf_link_desc *ldesc;
	struct scatterlist *mem_sg;
	struct nbpf_desc *desc;
	bool inc_src, inc_dst;
	size_t data_len = 0;
	int i = 0;

	switch (direction) {
	case DMA_DEV_TO_MEM:
		mem_sg = dst_sg;
		inc_src = false;
		inc_dst = true;
		break;

	case DMA_MEM_TO_DEV:
		mem_sg = src_sg;
		inc_src = true;
		inc_dst = false;
		break;

	default:
	case DMA_MEM_TO_MEM:
		mem_sg = src_sg;
		inc_src = true;
		inc_dst = true;
	}

	desc = nbpf_desc_get(chan, len);
	if (!desc)
		return NULL;

	desc->async_tx.flags = flags;
	desc->async_tx.cookie = -EBUSY;
	desc->user_wait = false;

	/*
	 * This is a private descriptor list, and we own the descriptor. No need
	 * to lock.
	 */
	list_for_each_entry(ldesc, &desc->sg, node) {
		int ret = nbpf_prep_one(ldesc, direction,
					sg_dma_address(src_sg),
					sg_dma_address(dst_sg),
					sg_dma_len(mem_sg),
					i == len - 1);
		if (ret < 0) {
			nbpf_desc_put(desc);
			return NULL;
		}
		data_len += sg_dma_len(mem_sg);
		if (inc_src)
			src_sg = sg_next(src_sg);
		if (inc_dst)
			dst_sg = sg_next(dst_sg);
		mem_sg = direction == DMA_DEV_TO_MEM ? dst_sg : src_sg;
		i++;
	}

	desc->length = data_len;

	/* The user has to return the descriptor to us ASAP via .tx_submit() */
	return &desc->async_tx;
}

static struct dma_async_tx_descriptor *nbpf_prep_memcpy(
	struct dma_chan *dchan, dma_addr_t dst, dma_addr_t src,
	size_t len, unsigned long flags)
{
	struct nbpf_channel *chan = nbpf_to_chan(dchan);
	struct scatterlist dst_sg;
	struct scatterlist src_sg;

	sg_init_table(&dst_sg, 1);
	sg_init_table(&src_sg, 1);

	sg_dma_address(&dst_sg) = dst;
	sg_dma_address(&src_sg) = src;

	sg_dma_len(&dst_sg) = len;
	sg_dma_len(&src_sg) = len;

	dev_dbg(dchan->device->dev, "%s(): %zu @ %pad -> %pad\n",
		__func__, len, &src, &dst);

	return nbpf_prep_sg(chan, &src_sg, &dst_sg, 1,
			    DMA_MEM_TO_MEM, flags);
}

static struct dma_async_tx_descriptor *nbpf_prep_memcpy_sg(
	struct dma_chan *dchan,
	struct scatterlist *dst_sg, unsigned int dst_nents,
	struct scatterlist *src_sg, unsigned int src_nents,
	unsigned long flags)
{
	struct nbpf_channel *chan = nbpf_to_chan(dchan);

	if (dst_nents != src_nents)
		return NULL;

	return nbpf_prep_sg(chan, src_sg, dst_sg, src_nents,
			    DMA_MEM_TO_MEM, flags);
}

static struct dma_async_tx_descriptor *nbpf_prep_slave_sg(
	struct dma_chan *dchan, struct scatterlist *sgl, unsigned int sg_len,
	enum dma_transfer_direction direction, unsigned long flags, void *context)
{
	struct nbpf_channel *chan = nbpf_to_chan(dchan);
	struct scatterlist slave_sg;

	dev_dbg(dchan->device->dev, "Entry %s()\n", __func__);

	sg_init_table(&slave_sg, 1);

	switch (direction) {
	case DMA_MEM_TO_DEV:
		sg_dma_address(&slave_sg) = chan->slave_dst_addr;
		return nbpf_prep_sg(chan, sgl, &slave_sg, sg_len,
				    direction, flags);

	case DMA_DEV_TO_MEM:
		sg_dma_address(&slave_sg) = chan->slave_src_addr;
		return nbpf_prep_sg(chan, &slave_sg, sgl, sg_len,
				    direction, flags);

	default:
		return NULL;
	}
}

static int nbpf_alloc_chan_resources(struct dma_chan *dchan)
{
	struct nbpf_channel *chan = nbpf_to_chan(dchan);
	int ret;

	INIT_LIST_HEAD(&chan->free);
	INIT_LIST_HEAD(&chan->free_links);
	INIT_LIST_HEAD(&chan->queued);
	INIT_LIST_HEAD(&chan->active);
	INIT_LIST_HEAD(&chan->done);

	ret = nbpf_desc_page_alloc(chan);
	if (ret < 0)
		return ret;

	dev_dbg(dchan->device->dev, "Entry %s(): terminal %u\n", __func__,
		chan->terminal);

	nbpf_chan_configure(chan);

	return ret;
}

static void nbpf_free_chan_resources(struct dma_chan *dchan)
{
	struct nbpf_channel *chan = nbpf_to_chan(dchan);
	struct nbpf_desc_page *dpage, *tmp;

	dev_dbg(dchan->device->dev, "Entry %s()\n", __func__);

	nbpf_chan_halt(chan);
	nbpf_chan_idle(chan);
	/* Clean up for if a channel is re-used for MEMCPY after slave DMA */
	nbpf_chan_prepare_default(chan);

	list_for_each_entry_safe(dpage, tmp, &chan->desc_page, node) {
		struct nbpf_link_desc *ldesc;
		int i;
		list_del(&dpage->node);
		for (i = 0, ldesc = dpage->ldesc;
		     i < ARRAY_SIZE(dpage->ldesc);
		     i++, ldesc++)
			dma_unmap_single(dchan->device->dev, ldesc->hwdesc_dma_addr,
					 sizeof(*ldesc->hwdesc), DMA_TO_DEVICE);
		free_page((unsigned long)dpage);
	}
}

static struct dma_chan *nbpf_of_xlate(struct of_phandle_args *dma_spec,
				      struct of_dma *ofdma)
{
	struct nbpf_device *nbpf = ofdma->of_dma_data;
	struct dma_chan *dchan;
	struct nbpf_channel *chan;

	if (dma_spec->args_count != 2)
		return NULL;

	dchan = dma_get_any_slave_channel(&nbpf->dma_dev);
	if (!dchan)
		return NULL;

	dev_dbg(dchan->device->dev, "Entry %s(%s)\n", __func__,
		dma_spec->np->name);

	chan = nbpf_to_chan(dchan);

	chan->terminal = dma_spec->args[0];
	chan->flags = dma_spec->args[1];

	nbpf_chan_prepare(chan);
	nbpf_chan_configure(chan);

	return dchan;
}

static void nbpf_chan_tasklet(unsigned long data)
{
	struct nbpf_channel *chan = (struct nbpf_channel *)data;
	struct nbpf_desc *desc, *tmp;
	dma_async_tx_callback callback;
	void *param;

	while (!list_empty(&chan->done)) {
		bool found = false, must_put, recycling = false;

		spin_lock_irq(&chan->lock);

		list_for_each_entry_safe(desc, tmp, &chan->done, node) {
			if (!desc->user_wait) {
				/* Newly completed descriptor, have to process */
				found = true;
				break;
			} else if (async_tx_test_ack(&desc->async_tx)) {
				/*
				 * This descriptor was waiting for a user ACK,
				 * it can be recycled now.
				 */
				list_del(&desc->node);
				spin_unlock_irq(&chan->lock);
				nbpf_desc_put(desc);
				recycling = true;
				break;
			}
		}

		if (recycling)
			continue;

		if (!found) {
			/* This can happen if TERMINATE_ALL has been called */
			spin_unlock_irq(&chan->lock);
			break;
		}

		dma_cookie_complete(&desc->async_tx);

		/*
		 * With released lock we cannot dereference desc, maybe it's
		 * still on the "done" list
		 */
		if (async_tx_test_ack(&desc->async_tx)) {
			list_del(&desc->node);
			must_put = true;
		} else {
			desc->user_wait = true;
			must_put = false;
		}

		callback = desc->async_tx.callback;
		param = desc->async_tx.callback_param;

		/* ack and callback completed descriptor */
		spin_unlock_irq(&chan->lock);

		if (callback)
			callback(param);

		if (must_put)
			nbpf_desc_put(desc);
	}
}

static irqreturn_t nbpf_chan_irq(int irq, void *dev)
{
	struct nbpf_channel *chan = dev;
	bool done = nbpf_status_get(chan);
	struct nbpf_desc *desc;
	irqreturn_t ret;
	bool bh = false;

	if (!done)
		return IRQ_NONE;

	nbpf_status_ack(chan);

	dev_dbg(&chan->dma_chan.dev->device, "%s()\n", __func__);

	spin_lock(&chan->lock);
	desc = chan->running;
	if (WARN_ON(!desc)) {
		ret = IRQ_NONE;
		goto unlock;
	} else {
		ret = IRQ_HANDLED;
		bh = true;
	}

	list_move_tail(&desc->node, &chan->done);
	chan->running = NULL;

	if (!list_empty(&chan->active)) {
		desc = list_first_entry(&chan->active,
					struct nbpf_desc, node);
		if (!nbpf_start(desc))
			chan->running = desc;
	}

unlock:
	spin_unlock(&chan->lock);

	if (bh)
		tasklet_schedule(&chan->tasklet);

	return ret;
}

static irqreturn_t nbpf_err_irq(int irq, void *dev)
{
	struct nbpf_device *nbpf = dev;
	u32 error = nbpf_error_get(nbpf);

	dev_warn(nbpf->dma_dev.dev, "DMA error IRQ %u\n", irq);

	if (!error)
		return IRQ_NONE;

	do {
		struct nbpf_channel *chan = nbpf_error_get_channel(nbpf, error);
		/* On error: abort all queued transfers, no callback */
		nbpf_error_clear(chan);
		nbpf_chan_idle(chan);
		error = nbpf_error_get(nbpf);
	} while (error);

	return IRQ_HANDLED;
}

static int nbpf_chan_probe(struct nbpf_device *nbpf, int n)
{
	struct dma_device *dma_dev = &nbpf->dma_dev;
	struct nbpf_channel *chan = nbpf->chan + n;
	int ret;

	chan->nbpf = nbpf;
	chan->base = nbpf->base + NBPF_REG_CHAN_OFFSET + NBPF_REG_CHAN_SIZE * n;
	INIT_LIST_HEAD(&chan->desc_page);
	spin_lock_init(&chan->lock);
	chan->dma_chan.device = dma_dev;
	dma_cookie_init(&chan->dma_chan);
	nbpf_chan_prepare_default(chan);

	dev_dbg(dma_dev->dev, "%s(): channel %d: -> %p\n", __func__, n, chan->base);

	snprintf(chan->name, sizeof(chan->name), "nbpf %d", n);

	tasklet_init(&chan->tasklet, nbpf_chan_tasklet, (unsigned long)chan);
	ret = devm_request_irq(dma_dev->dev, chan->irq,
			nbpf_chan_irq, IRQF_SHARED,
			chan->name, chan);
	if (ret < 0)
		return ret;

	/* Add the channel to DMA device channel list */
	list_add_tail(&chan->dma_chan.device_node,
		      &dma_dev->channels);

	return 0;
}

static const struct of_device_id nbpf_match[] = {
	{.compatible = "renesas,nbpfaxi64dmac1b4",	.data = &nbpf_cfg[NBPF1B4]},
	{.compatible = "renesas,nbpfaxi64dmac1b8",	.data = &nbpf_cfg[NBPF1B8]},
	{.compatible = "renesas,nbpfaxi64dmac1b16",	.data = &nbpf_cfg[NBPF1B16]},
	{.compatible = "renesas,nbpfaxi64dmac4b4",	.data = &nbpf_cfg[NBPF4B4]},
	{.compatible = "renesas,nbpfaxi64dmac4b8",	.data = &nbpf_cfg[NBPF4B8]},
	{.compatible = "renesas,nbpfaxi64dmac4b16",	.data = &nbpf_cfg[NBPF4B16]},
	{.compatible = "renesas,nbpfaxi64dmac8b4",	.data = &nbpf_cfg[NBPF8B4]},
	{.compatible = "renesas,nbpfaxi64dmac8b8",	.data = &nbpf_cfg[NBPF8B8]},
	{.compatible = "renesas,nbpfaxi64dmac8b16",	.data = &nbpf_cfg[NBPF8B16]},
	{}
};
MODULE_DEVICE_TABLE(of, nbpf_match);

static int nbpf_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id = of_match_device(nbpf_match, dev);
	struct device_node *np = dev->of_node;
	struct nbpf_device *nbpf;
	struct dma_device *dma_dev;
	struct resource *iomem, *irq_res;
	const struct nbpf_config *cfg;
	int num_channels;
	int ret, irq, eirq, i;
	int irqbuf[9] /* maximum 8 channels + error IRQ */;
	unsigned int irqs = 0;

	BUILD_BUG_ON(sizeof(struct nbpf_desc_page) > PAGE_SIZE);

	/* DT only */
	if (!np || !of_id || !of_id->data)
		return -ENODEV;

	cfg = of_id->data;
	num_channels = cfg->num_channels;

	nbpf = devm_kzalloc(dev, sizeof(*nbpf) + num_channels *
			    sizeof(nbpf->chan[0]), GFP_KERNEL);
	if (!nbpf) {
		dev_err(dev, "Memory allocation failed\n");
		return -ENOMEM;
	}
	dma_dev = &nbpf->dma_dev;
	dma_dev->dev = dev;

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	nbpf->base = devm_ioremap_resource(dev, iomem);
	if (IS_ERR(nbpf->base))
		return PTR_ERR(nbpf->base);

	nbpf->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(nbpf->clk))
		return PTR_ERR(nbpf->clk);

	nbpf->config = cfg;

	for (i = 0; irqs < ARRAY_SIZE(irqbuf); i++) {
		irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (!irq_res)
			break;

		for (irq = irq_res->start; irq <= irq_res->end;
		     irq++, irqs++)
			irqbuf[irqs] = irq;
	}

	/*
	 * 3 IRQ resource schemes are supported:
	 * 1. 1 shared IRQ for error and all channels
	 * 2. 2 IRQs: one for error and one shared for all channels
	 * 3. 1 IRQ for error and an own IRQ for each channel
	 */
	if (irqs != 1 && irqs != 2 && irqs != num_channels + 1)
		return -ENXIO;

	if (irqs == 1) {
		eirq = irqbuf[0];

		for (i = 0; i <= num_channels; i++)
			nbpf->chan[i].irq = irqbuf[0];
	} else {
		eirq = platform_get_irq_byname(pdev, "error");
		if (eirq < 0)
			return eirq;

		if (irqs == num_channels + 1) {
			struct nbpf_channel *chan;

			for (i = 0, chan = nbpf->chan; i <= num_channels;
			     i++, chan++) {
				/* Skip the error IRQ */
				if (irqbuf[i] == eirq)
					i++;
				chan->irq = irqbuf[i];
			}

			if (chan != nbpf->chan + num_channels)
				return -EINVAL;
		} else {
			/* 2 IRQs and more than one channel */
			if (irqbuf[0] == eirq)
				irq = irqbuf[1];
			else
				irq = irqbuf[0];

			for (i = 0; i <= num_channels; i++)
				nbpf->chan[i].irq = irq;
		}
	}

	ret = devm_request_irq(dev, eirq, nbpf_err_irq,
			       IRQF_SHARED, "dma error", nbpf);
	if (ret < 0)
		return ret;

	INIT_LIST_HEAD(&dma_dev->channels);

	/* Create DMA Channel */
	for (i = 0; i < num_channels; i++) {
		ret = nbpf_chan_probe(nbpf, i);
		if (ret < 0)
			return ret;
	}

	dma_cap_set(DMA_MEMCPY, dma_dev->cap_mask);
	dma_cap_set(DMA_SLAVE, dma_dev->cap_mask);
	dma_cap_set(DMA_PRIVATE, dma_dev->cap_mask);
	dma_cap_set(DMA_SG, dma_dev->cap_mask);

	/* Common and MEMCPY operations */
	dma_dev->device_alloc_chan_resources
		= nbpf_alloc_chan_resources;
	dma_dev->device_free_chan_resources = nbpf_free_chan_resources;
	dma_dev->device_prep_dma_sg = nbpf_prep_memcpy_sg;
	dma_dev->device_prep_dma_memcpy = nbpf_prep_memcpy;
	dma_dev->device_tx_status = nbpf_tx_status;
	dma_dev->device_issue_pending = nbpf_issue_pending;

	/*
	 * If we drop support for unaligned MEMCPY buffer addresses and / or
	 * lengths by setting
	 * dma_dev->copy_align = 4;
	 * then we can set transfer length to 4 bytes in nbpf_prep_one() for
	 * DMA_MEM_TO_MEM
	 */

	/* Compulsory for DMA_SLAVE fields */
	dma_dev->device_prep_slave_sg = nbpf_prep_slave_sg;
	dma_dev->device_config = nbpf_config;
	dma_dev->device_pause = nbpf_pause;
	dma_dev->device_terminate_all = nbpf_terminate_all;

	dma_dev->src_addr_widths = NBPF_DMA_BUSWIDTHS;
	dma_dev->dst_addr_widths = NBPF_DMA_BUSWIDTHS;
	dma_dev->directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);

	platform_set_drvdata(pdev, nbpf);

	ret = clk_prepare_enable(nbpf->clk);
	if (ret < 0)
		return ret;

	nbpf_configure(nbpf);

	ret = dma_async_device_register(dma_dev);
	if (ret < 0)
		goto e_clk_off;

	ret = of_dma_controller_register(np, nbpf_of_xlate, nbpf);
	if (ret < 0)
		goto e_dma_dev_unreg;

	return 0;

e_dma_dev_unreg:
	dma_async_device_unregister(dma_dev);
e_clk_off:
	clk_disable_unprepare(nbpf->clk);

	return ret;
}

static int nbpf_remove(struct platform_device *pdev)
{
	struct nbpf_device *nbpf = platform_get_drvdata(pdev);

	of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&nbpf->dma_dev);
	clk_disable_unprepare(nbpf->clk);

	return 0;
}

static const struct platform_device_id nbpf_ids[] = {
	{"nbpfaxi64dmac1b4",	(kernel_ulong_t)&nbpf_cfg[NBPF1B4]},
	{"nbpfaxi64dmac1b8",	(kernel_ulong_t)&nbpf_cfg[NBPF1B8]},
	{"nbpfaxi64dmac1b16",	(kernel_ulong_t)&nbpf_cfg[NBPF1B16]},
	{"nbpfaxi64dmac4b4",	(kernel_ulong_t)&nbpf_cfg[NBPF4B4]},
	{"nbpfaxi64dmac4b8",	(kernel_ulong_t)&nbpf_cfg[NBPF4B8]},
	{"nbpfaxi64dmac4b16",	(kernel_ulong_t)&nbpf_cfg[NBPF4B16]},
	{"nbpfaxi64dmac8b4",	(kernel_ulong_t)&nbpf_cfg[NBPF8B4]},
	{"nbpfaxi64dmac8b8",	(kernel_ulong_t)&nbpf_cfg[NBPF8B8]},
	{"nbpfaxi64dmac8b16",	(kernel_ulong_t)&nbpf_cfg[NBPF8B16]},
	{},
};
MODULE_DEVICE_TABLE(platform, nbpf_ids);

#ifdef CONFIG_PM
static int nbpf_runtime_suspend(struct device *dev)
{
	struct nbpf_device *nbpf = platform_get_drvdata(to_platform_device(dev));
	clk_disable_unprepare(nbpf->clk);
	return 0;
}

static int nbpf_runtime_resume(struct device *dev)
{
	struct nbpf_device *nbpf = platform_get_drvdata(to_platform_device(dev));
	return clk_prepare_enable(nbpf->clk);
}
#endif

static const struct dev_pm_ops nbpf_pm_ops = {
	SET_RUNTIME_PM_OPS(nbpf_runtime_suspend, nbpf_runtime_resume, NULL)
};

static struct platform_driver nbpf_driver = {
	.driver = {
		.name = "dma-nbpf",
		.of_match_table = nbpf_match,
		.pm = &nbpf_pm_ops,
	},
	.id_table = nbpf_ids,
	.probe = nbpf_probe,
	.remove = nbpf_remove,
};

module_platform_driver(nbpf_driver);

MODULE_AUTHOR("Guennadi Liakhovetski <g.liakhovetski@gmx.de>");
MODULE_DESCRIPTION("dmaengine driver for NBPFAXI64* DMACs");
MODULE_LICENSE("GPL v2");
                                                                                                                /*
 * Device tree helpers for DMA request / controller
 *
 * Based on of_gpio.c
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_dma.h>

static LIST_HEAD(of_dma_list);
static DEFINE_MUTEX(of_dma_lock);

/**
 * of_dma_find_controller - Get a DMA controller in DT DMA helpers list
 * @dma_spec:	pointer to DMA specifier as found in the device tree
 *
 * Finds a DMA controller with matching device node and number for dma cells
 * in a list of registered DMA controllers. If a match is found a valid pointer
 * to the DMA data stored is retuned. A NULL pointer is returned if no match is
 * found.
 */
static struct of_dma *of_dma_find_controller(struct of_phandle_args *dma_spec)
{
	struct of_dma *ofdma;

	list_for_each_entry(ofdma, &of_dma_list, of_dma_controllers)
		if (ofdma->of_node == dma_spec->np)
			return ofdma;

	pr_debug("%s: can't find DMA controller %s\n", __func__,
		 dma_spec->np->full_name);

	return NULL;
}

/**
 * of_dma_router_xlate - translation function for router devices
 * @dma_spec:	pointer to DMA specifier as found in the device tree
 * @of_dma:	pointer to DMA controller data (router information)
 *
 * The function creates new dma_spec to be passed to the router driver's
 * of_dma_route_allocate() function to prepare a dma_spec which will be used
 * to request channel from the real DMA controller.
 */
static struct dma_chan *of_dma_router_xlate(struct of_phandle_args *dma_spec,
					    struct of_dma *ofdma)
{
	struct dma_chan		*chan;
	struct of_dma		*ofdma_target;
	struct of_phandle_args	dma_spec_target;
	void			*route_data;

	/* translate the request for the real DMA controller */
	memcpy(&dma_spec_target, dma_spec, sizeof(dma_spec_target));
	route_data = ofdma->of_dma_route_allocate(&dma_spec_target, ofdma);
	if (IS_ERR(route_data))
		return NULL;

	ofdma_target = of_dma_find_controller(&dma_spec_target);
	if (!ofdma_target) {
		ofdma->dma_router->route_free(ofdma->dma_router->dev,
					      route_data);
		chan = ERR_PTR(-EPROBE_DEFER);
		goto err;
	}

	chan = ofdma_target->of_dma_xlate(&dma_spec_target, ofdma_target);
	if (IS_ERR_OR_NULL(chan)) {
		ofdma->dma_router->route_free(ofdma->dma_router->dev,
					      route_data);
	} else {
		chan->router = ofdma->dma_router;
		chan->route_data = route_data;
	}

err:
	/*
	 * Need to put the node back since the ofdma->of_dma_route_allocate
	 * has taken it for generating the new, translated dma_spec
	 */
	of_node_put(dma_spec_target.np);
	return chan;
}

/**
 * of_dma_controller_register - Register a DMA controller to DT DMA helpers
 * @np:			device node of DMA controller
 * @of_dma_xlate:	translation function which converts a phandle
 *			arguments list into a dma_chan structure
 * @data		pointer to controller specific data to be used by
 *			translation function
 *
 * Returns 0 on success or appropriate errno value on error.
 *
 * Allocated memory should be freed with appropriate of_dma_controller_free()
 * call.
 */
int of_dma_controller_register(struct device_node *np,
				struct dma_chan *(*of_dma_xlate)
				(struct of_phandle_args *, struct of_dma *),
				void *data)
{
	struct of_dma	*ofdma;

	if (!np || !of_dma_xlate) {
		pr_err("%s: not enough information provided\n", __func__);
		return -EINVAL;
	}

	ofdma = kzalloc(sizeof(*ofdma), GFP_KERNEL);
	if (!ofdma)
		return -ENOMEM;

	ofdma->of_node = np;
	ofdma->of_dma_xlate = of_dma_xlate;
	ofdma->of_dma_data = data;

	/* Now queue of_dma controller structure in list */
	mutex_lock(&of_dma_lock);
	list_add_tail(&ofdma->of_dma_controllers, &of_dma_list);
	mutex_unlock(&of_dma_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(of_dma_controller_register);

/**
 * of_dma_controller_free - Remove a DMA controller from DT DMA helpers list
 * @np:		device node of DMA controller
 *
 * Memory allocated by of_dma_controller_register() is freed here.
 */
void of_dma_controller_free(struct device_node *np)
{
	struct of_dma *ofdma;

	mutex_lock(&of_dma_lock);

	list_for_each_entry(ofdma, &of_dma_list, of_dma_controllers)
		if (ofdma->of_node == np) {
			list_del(&ofdma->of_dma_controllers);
			kfree(ofdma);
			break;
		}

	mutex_unlock(&of_dma_lock);
}
EXPORT_SYMBOL_GPL(of_dma_controller_free);

/**
 * of_dma_router_register - Register a DMA router to DT DMA helpers as a
 *			    controller
 * @np:				device node of DMA router
 * @of_dma_route_allocate:	setup function for the router which need to
 *				modify the dma_spec for the DMA controller to
 *				use and to set up the requested route.
 * @dma_router:			pointer to dma_router structure to be used when
 *				the route need to be free up.
 *
 * Returns 0 on success or appropriate errno value on error.
 *
 * Allocated memory should be freed with appropriate of_dma_controller_free()
 * call.
 */
int of_dma_router_register(struct device_node *np,
			   void *(*of_dma_route_allocate)
			   (struct of_phandle_args *, struct of_dma *),
			   struct dma_router *dma_router)
{
	struct of_dma	*ofdma;

	if (!np || !of_dma_route_allocate || !dma_router) {
		pr_err("%s: not enough information provided\n", __func__);
		return -EINVAL;
	}

	ofdma = kzalloc(sizeof(*ofdma), GFP_KERNEL);
	if (!ofdma)
		return -ENOMEM;

	ofdma->of_node = np;
	ofdma->of_dma_xlate = of_dma_router_xlate;
	ofdma->of_dma_route_allocate = of_dma_route_allocate;
	ofdma->dma_router = dma_router;

	/* Now queue of_dma controller structure in list */
	mutex_lock(&of_dma_lock);
	list_add_tail(&ofdma->of_dma_controllers, &of_dma_list);
	mutex_unlock(&of_dma_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(of_dma_router_register);

/**
 * of_dma_match_channel - Check if a DMA specifier matches name
 * @np:		device node to look for DMA channels
 * @name:	channel name to be matched
 * @index:	index of DMA specifier in list of DMA specifiers
 * @dma_spec:	pointer to DMA specifier as found in the device tree
 *
 * Check if the DMA specifier pointed to by the index in a list of DMA
 * specifiers, matches the name provided. Returns 0 if the name matches and
 * a valid pointer to the DMA specifier is found. Otherwise returns -ENODEV.
 */
static int of_dma_match_channel(struct device_node *np, const char *name,
				int index, struct of_phandle_args *dma_spec)
{
	const char *s;

	if (of_property_read_string_index(np, "dma-names", index, &s))
		return -ENODEV;

	if (strcmp(name, s))
		return -ENODEV;

	if (of_parse_phandle_with_args(np, "dmas", "#dma-cells", index,
				       dma_spec))
		return -ENODEV;

	return 0;
}

/**
 * of_dma_get_mcode_addr - Get the DMA micro code buffer address.
 * @np:		device node of DMA controller
 *
 * Return the physical address.
 */
unsigned int of_dma_get_mcode_addr(struct device_node *np)
{
	unsigned int addr = 0;
	const __be32	*prop;

	prop = of_get_property(np, "#dma-mcode-addr", NULL);
	if (prop)
		addr = be32_to_cpup(prop);

	return addr;
}
EXPORT_SYMBOL_GPL(of_dma_get_mcode_addr);

/**
 * of_dma_secure_dma_ch- Get the DMA micro code buffer address.
 * @np:		device node of DMA controller
 *
 * Return the physical address.
 */
bool of_dma_secure_mode(struct device_node *np)
{
	bool ret = 0;
	const __be32	*prop;

	prop = of_get_property(np, "#dma-secure-mode", NULL);
	if (prop)
		ret = be32_to_cpup(prop);

	return ret;
}
EXPORT_SYMBOL_GPL(of_dma_secure_mode);

#ifdef CONFIG_SOC_EXYNOS8895
void __iomem *of_dma_get_sel_chan_address(struct device_node *np)
{
	const __be32 *reg_list;
	int ret = 0;

	reg_list = of_get_property(np, "dma-selchan", NULL);

	if (!reg_list)
		return NULL;

	ret = be32_to_cpup(reg_list);
	if (!ret)
		return NULL;

	return ioremap(ret, SZ_32);
}
EXPORT_SYMBOL_GPL(of_dma_get_sel_chan_address);
#endif

/**
 * of_dma_get_arwrapper_address - Get the DMA WAPPER AR address
 * @np:		device node of DMA controller
 * @num:	DMA channel thread number
 *
 * Return the virtual address.
 */
void __iomem *of_dma_get_arwrapper_address(struct device_node *np, unsigned int num)
{
	const __be32 *reg_list;
	unsigned int length, count;

	reg_list = of_get_property(np, "dma-arwrapper", &length);
	count = (unsigned int)(length / sizeof(unsigned int));

	if (!reg_list || num >= count)
		return NULL;

	return ioremap(be32_to_cpup(reg_list + num), SZ_32);
}
EXPORT_SYMBOL_GPL(of_dma_get_arwrapper_address);

/**
 * of_dma_get_arwrapper_address - Get the DMA WAPPER AW address
 * @np:		device node of DMA controller
 * @num:	DMA channel thread number
 *
 * Return the virtual address.
 */
void __iomem *of_dma_get_awwrapper_address(struct device_node *np, unsigned int num)
{
	const __be32 *reg_list;
	unsigned int length, count;

	reg_list = of_get_property(np, "dma-awwrapper", &length);
	count = (unsigned int)(length / sizeof(unsigned int));

	if (!reg_list || num >= count)
		return NULL;

	return ioremap(be32_to_cpup(reg_list + num), SZ_32);
}
EXPORT_SYMBOL_GPL(of_dma_get_awwrapper_address);

/**
 * of_dma_get_arwrapper_address - Get the DMA WAPPER AR address of DMA instruction
 * @np:		device node of DMA controller
 *
 * Return the virtual address.
 */
void __iomem *of_dma_get_instwrapper_address(struct device_node *np)
{
	const __be32 *reg_list;
	int ret = 0;

	reg_list = of_get_property(np, "dma-instwrapper", NULL);

	if (!reg_list)
		return NULL;

	ret = be32_to_cpup(reg_list);
	if (!ret)
		return NULL;

	return ioremap(ret, SZ_32);
}
EXPORT_SYMBOL_GPL(of_dma_get_instwrapper_address);

/**
 * of_dma_get_arwrapper_address - Get the DMA WAPPER availableilable
 * @np:		device node of DMA controller
 *
 */
bool of_dma_get_wrapper_available(struct device_node *np)
{
	const __be32 *reg_list;
	int ret = 0;

	reg_list = of_get_property(np, "dma-instwrapper", NULL);

	if (!reg_list)
		return false;

	ret = be32_to_cpup(reg_list);
	if (ret)
		return true;
	else
		return false;
}
EXPORT_SYMBOL_GPL(of_dma_get_wrapper_available);

/**
 * of_dma_get_arwrapper_address - Get the DMA WAPPER availableilable
 * @np:		device node of DMA controller
 *
 */
u64 of_dma_get_mask(struct device_node *np, char *name)
{
	int bit_cnt = 0;

	of_property_read_u32(np, name, &bit_cnt);

	if (bit_cnt)
		return ((u64)1 << bit_cnt) - 1;
	else
		return -1;
}
EXPORT_SYMBOL_GPL(of_dma_get_mask);

/**
 * of_dma_request_slave_channel - Get the DMA slave channel
 * @np:		device node to get DMA request from
 * @name:	name of desired channel
 *
 * Returns pointer to appropriate DMA channel on success or an error pointer.
 */
struct dma_chan *of_dma_request_slave_channel(struct device_node *np,
					      const char *name)
{
	struct of_phandle_args	dma_spec;
	struct of_dma		*ofdma;
	struct dma_chan		*chan;
	int			count, i;
	int			ret_no_channel = -ENODEV;

	if (!np || !name) {
		pr_err("%s: not enough information provided\n", __func__);
		return ERR_PTR(-ENODEV);
	}

	/* Silently fail if there is not even the "dmas" property */
	if (!of_find_property(np, "dmas", NULL))
		return ERR_PTR(-ENODEV);

	count = of_property_count_strings(np, "dma-names");
	if (count < 0) {
		pr_err("%s: dma-names property of node '%s' missing or empty\n",
			__func__, np->full_name);
		return ERR_PTR(-ENODEV);
	}

	for (i = 0; i < count; i++) {
		if (of_dma_match_channel(np, name, i, &dma_spec))
			continue;

		mutex_lock(&of_dma_lock);
		ofdma = of_dma_find_controller(&dma_spec);

		if (ofdma) {
			chan = ofdma->of_dma_xlate(&dma_spec, ofdma);
		} else {
			ret_no_channel = -EPROBE_DEFER;
			chan = NULL;
		}

		mutex_unlock(&of_dma_lock);

		of_node_put(dma_spec.np);

		if (chan)
			return chan;
	}

	return ERR_PTR(ret_no_channel);
}
EXPORT_SYMBOL_GPL(of_dma_request_slave_channel);

/**
 * of_dma_simple_xlate - Simple DMA engine translation function
 * @dma_spec:	pointer to DMA specifier as found in the device tree
 * @of_dma:	pointer to DMA controller data
 *
 * A simple translation function for devices that use a 32-bit value for the
 * filter_param when calling the DMA engine dma_request_channel() function.
 * Note that this translation function requires that #dma-cells is equal to 1
 * and the argument of the dma specifier is the 32-bit filter_param. Returns
 * pointer to appropriate dma channel on success or NULL on error.
 */
struct dma_chan *of_dma_simple_xlate(struct of_phandle_args *dma_spec,
						struct of_dma *ofdma)
{
	int count = dma_spec->args_count;
	struct of_dma_filter_info *info = ofdma->of_dma_data;

	if (!info || !info->filter_fn)
		return NULL;

	if (count != 1)
		return NULL;

	return dma_request_channel(info->dma_cap, info->filter_fn,
			&dma_spec->args[0]);
}
EXPORT_SYMBOL_GPL(of_dma_simple_xlate);

/**
 * of_dma_xlate_by_chan_id - Translate dt property to DMA channel by channel id
 * @dma_spec:	pointer to DMA specifier as found in the device tree
 * @of_dma:	pointer to DMA controller data
 *
 * This function can be used as the of xlate callback for DMA driver which wants
 * to match the channel based on the channel id. When using this xlate function
 * the #dma-cells propety of the DMA controller dt node needs to be set to 1.
 * The data parameter of of_dma_controller_register must be a pointer to the
 * dma_device struct the function should match upon.
 *
 * Returns pointer to appropriate dma channel on success or NULL on error.
 */
struct dma_chan *of_dma_xlate_by_chan_id(struct of_phandle_args *dma_spec,
					 struct of_dma *ofdma)
{
	struct dma_device *dev = ofdma->of_dma_data;
	struct dma_chan *chan, *candidate = NULL;

	if (!dev || dma_spec->args_count != 1)
		return NULL;

	list_for_each_entry(chan, &dev->channels, device_node)
		if (chan->chan_id == dma_spec->args[0]) {
			candidate = chan;
			break;
		}

	if (!candidate)
		return NULL;

	return dma_get_slave_channel(candidate);
}
EXPORT_SYMBOL_GPL(of_dma_xlate_by_chan_id);

bool of_dma_multi_irq(struct device_node *np)
{
	bool ret = 0;
	const __be32	*prop;

	prop = of_get_property(np, "#dma-multi-irq", NULL);
	if (prop)
		ret = be32_to_cpup(prop);

	return ret;
}
EXPORT_SYMBOL_GPL(of_dma_multi_irq);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * OMAP DMAengine support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/omap-dma.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/of_dma.h>
#include <linux/of_device.h>

#include "virt-dma.h"

#define OMAP_SDMA_REQUESTS	127
#define OMAP_SDMA_CHANNELS	32

struct omap_dmadev {
	struct dma_device ddev;
	spinlock_t lock;
	struct tasklet_struct task;
	struct list_head pending;
	void __iomem *base;
	const struct omap_dma_reg *reg_map;
	struct omap_system_dma_plat_info *plat;
	bool legacy;
	unsigned dma_requests;
	spinlock_t irq_lock;
	uint32_t irq_enable_mask;
	struct omap_chan *lch_map[OMAP_SDMA_CHANNELS];
};

struct omap_chan {
	struct virt_dma_chan vc;
	struct list_head node;
	void __iomem *channel_base;
	const struct omap_dma_reg *reg_map;
	uint32_t ccr;

	struct dma_slave_config	cfg;
	unsigned dma_sig;
	bool cyclic;
	bool paused;

	int dma_ch;
	struct omap_desc *desc;
	unsigned sgidx;
};

struct omap_sg {
	dma_addr_t addr;
	uint32_t en;		/* number of elements (24-bit) */
	uint32_t fn;		/* number of frames (16-bit) */
};

struct omap_desc {
	struct virt_dma_desc vd;
	enum dma_transfer_direction dir;
	dma_addr_t dev_addr;

	int16_t fi;		/* for OMAP_DMA_SYNC_PACKET */
	uint8_t es;		/* CSDP_DATA_TYPE_xxx */
	uint32_t ccr;		/* CCR value */
	uint16_t clnk_ctrl;	/* CLNK_CTRL value */
	uint16_t cicr;		/* CICR value */
	uint32_t csdp;		/* CSDP value */

	unsigned sglen;
	struct omap_sg sg[0];
};

enum {
	CCR_FS			= BIT(5),
	CCR_READ_PRIORITY	= BIT(6),
	CCR_ENABLE		= BIT(7),
	CCR_AUTO_INIT		= BIT(8),	/* OMAP1 only */
	CCR_REPEAT		= BIT(9),	/* OMAP1 only */
	CCR_OMAP31_DISABLE	= BIT(10),	/* OMAP1 only */
	CCR_SUSPEND_SENSITIVE	= BIT(8),	/* OMAP2+ only */
	CCR_RD_ACTIVE		= BIT(9),	/* OMAP2+ only */
	CCR_WR_ACTIVE		= BIT(10),	/* OMAP2+ only */
	CCR_SRC_AMODE_CONSTANT	= 0 << 12,
	CCR_SRC_AMODE_POSTINC	= 1 << 12,
	CCR_SRC_AMODE_SGLIDX	= 2 << 12,
	CCR_SRC_AMODE_DBLIDX	= 3 << 12,
	CCR_DST_AMODE_CONSTANT	= 0 << 14,
	CCR_DST_AMODE_POSTINC	= 1 << 14,
	CCR_DST_AMODE_SGLIDX	= 2 << 14,
	CCR_DST_AMODE_DBLIDX	= 3 << 14,
	CCR_CONSTANT_FILL	= BIT(16),
	CCR_TRANSPARENT_COPY	= BIT(17),
	CCR_BS			= BIT(18),
	CCR_SUPERVISOR		= BIT(22),
	CCR_PREFETCH		= BIT(23),
	CCR_TRIGGER_SRC		= BIT(24),
	CCR_BUFFERING_DISABLE	= BIT(25),
	CCR_WRITE_PRIORITY	= BIT(26),
	CCR_SYNC_ELEMENT	= 0,
	CCR_SYNC_FRAME		= CCR_FS,
	CCR_SYNC_BLOCK		= CCR_BS,
	CCR_SYNC_PACKET		= CCR_BS | CCR_FS,

	CSDP_DATA_TYPE_8	= 0,
	CSDP_DATA_TYPE_16	= 1,
	CSDP_DATA_TYPE_32	= 2,
	CSDP_SRC_PORT_EMIFF	= 0 << 2, /* OMAP1 only */
	CSDP_SRC_PORT_EMIFS	= 1 << 2, /* OMAP1 only */
	CSDP_SRC_PORT_OCP_T1	= 2 << 2, /* OMAP1 only */
	CSDP_SRC_PORT_TIPB	= 3 << 2, /* OMAP1 only */
	CSDP_SRC_PORT_OCP_T2	= 4 << 2, /* OMAP1 only */
	CSDP_SRC_PORT_MPUI	= 5 << 2, /* OMAP1 only */
	CSDP_SRC_PACKED		= BIT(6),
	CSDP_SRC_BURST_1	= 0 << 7,
	CSDP_SRC_BURST_16	= 1 << 7,
	CSDP_SRC_BURST_32	= 2 << 7,
	CSDP_SRC_BURST_64	= 3 << 7,
	CSDP_DST_PORT_EMIFF	= 0 << 9, /* OMAP1 only */
	CSDP_DST_PORT_EMIFS	= 1 << 9, /* OMAP1 only */
	CSDP_DST_PORT_OCP_T1	= 2 << 9, /* OMAP1 only */
	CSDP_DST_PORT_TIPB	= 3 << 9, /* OMAP1 only */
	CSDP_DST_PORT_OCP_T2	= 4 << 9, /* OMAP1 only */
	CSDP_DST_PORT_MPUI	= 5 << 9, /* OMAP1 only */
	CSDP_DST_PACKED		= BIT(13),
	CSDP_DST_BURST_1	= 0 << 14,
	CSDP_DST_BURST_16	= 1 << 14,
	CSDP_DST_BURST_32	= 2 << 14,
	CSDP_DST_BURST_64	= 3 << 14,

	CICR_TOUT_IE		= BIT(0),	/* OMAP1 only */
	CICR_DROP_IE		= BIT(1),
	CICR_HALF_IE		= BIT(2),
	CICR_FRAME_IE		= BIT(3),
	CICR_LAST_IE		= BIT(4),
	CICR_BLOCK_IE		= BIT(5),
	CICR_PKT_IE		= BIT(7),	/* OMAP2+ only */
	CICR_TRANS_ERR_IE	= BIT(8),	/* OMAP2+ only */
	CICR_SUPERVISOR_ERR_IE	= BIT(10),	/* OMAP2+ only */
	CICR_MISALIGNED_ERR_IE	= BIT(11),	/* OMAP2+ only */
	CICR_DRAIN_IE		= BIT(12),	/* OMAP2+ only */
	CICR_SUPER_BLOCK_IE	= BIT(14),	/* OMAP2+ only */

	CLNK_CTRL_ENABLE_LNK	= BIT(15),
};

static const unsigned es_bytes[] = {
	[CSDP_DATA_TYPE_8] = 1,
	[CSDP_DATA_TYPE_16] = 2,
	[CSDP_DATA_TYPE_32] = 4,
};

static struct of_dma_filter_info omap_dma_info = {
	.filter_fn = omap_dma_filter_fn,
};

static inline struct omap_dmadev *to_omap_dma_dev(struct dma_device *d)
{
	return container_of(d, struct omap_dmadev, ddev);
}

static inline struct omap_chan *to_omap_dma_chan(struct dma_chan *c)
{
	return container_of(c, struct omap_chan, vc.chan);
}

static inline struct omap_desc *to_omap_dma_desc(struct dma_async_tx_descriptor *t)
{
	return container_of(t, struct omap_desc, vd.tx);
}

static void omap_dma_desc_free(struct virt_dma_desc *vd)
{
	kfree(container_of(vd, struct omap_desc, vd));
}

static void omap_dma_write(uint32_t val, unsigned type, void __iomem *addr)
{
	switch (type) {
	case OMAP_DMA_REG_16BIT:
		writew_relaxed(val, addr);
		break;
	case OMAP_DMA_REG_2X16BIT:
		writew_relaxed(val, addr);
		writew_relaxed(val >> 16, addr + 2);
		break;
	case OMAP_DMA_REG_32BIT:
		writel_relaxed(val, addr);
		break;
	default:
		WARN_ON(1);
	}
}

static unsigned omap_dma_read(unsigned type, void __iomem *addr)
{
	unsigned val;

	switch (type) {
	case OMAP_DMA_REG_16BIT:
		val = readw_relaxed(addr);
		break;
	case OMAP_DMA_REG_2X16BIT:
		val = readw_relaxed(addr);
		val |= readw_relaxed(addr + 2) << 16;
		break;
	case OMAP_DMA_REG_32BIT:
		val = readl_relaxed(addr);
		break;
	default:
		WARN_ON(1);
		val = 0;
	}

	return val;
}

static void omap_dma_glbl_write(struct omap_dmadev *od, unsigned reg, unsigned val)
{
	const struct omap_dma_reg *r = od->reg_map + reg;

	WARN_ON(r->stride);

	omap_dma_write(val, r->type, od->base + r->offset);
}

static unsigned omap_dma_glbl_read(struct omap_dmadev *od, unsigned reg)
{
	const struct omap_dma_reg *r = od->reg_map + reg;

	WARN_ON(r->stride);

	return omap_dma_read(r->type, od->base + r->offset);
}

static void omap_dma_chan_write(struct omap_chan *c, unsigned reg, unsigned val)
{
	const struct omap_dma_reg *r = c->reg_map + reg;

	omap_dma_write(val, r->type, c->channel_base + r->offset);
}

static unsigned omap_dma_chan_read(struct omap_chan *c, unsigned reg)
{
	const struct omap_dma_reg *r = c->reg_map + reg;

	return omap_dma_read(r->type, c->channel_base + r->offset);
}

static void omap_dma_clear_csr(struct omap_chan *c)
{
	if (dma_omap1())
		omap_dma_chan_read(c, CSR);
	else
		omap_dma_chan_write(c, CSR, ~0);
}

static unsigned omap_dma_get_csr(struct omap_chan *c)
{
	unsigned val = omap_dma_chan_read(c, CSR);

	if (!dma_omap1())
		omap_dma_chan_write(c, CSR, val);

	return val;
}

static void omap_dma_assign(struct omap_dmadev *od, struct omap_chan *c,
	unsigned lch)
{
	c->channel_base = od->base + od->plat->channel_stride * lch;

	od->lch_map[lch] = c;
}

static void omap_dma_start(struct omap_chan *c, struct omap_desc *d)
{
	struct omap_dmadev *od = to_omap_dma_dev(c->vc.chan.device);

	if (__dma_omap15xx(od->plat->dma_attr))
		omap_dma_chan_write(c, CPC, 0);
	else
		omap_dma_chan_write(c, CDAC, 0);

	omap_dma_clear_csr(c);

	/* Enable interrupts */
	omap_dma_chan_write(c, CICR, d->cicr);

	/* Enable channel */
	omap_dma_chan_write(c, CCR, d->ccr | CCR_ENABLE);
}

static void omap_dma_stop(struct omap_chan *c)
{
	struct omap_dmadev *od = to_omap_dma_dev(c->vc.chan.device);
	uint32_t val;

	/* disable irq */
	omap_dma_chan_write(c, CICR, 0);

	omap_dma_clear_csr(c);

	val = omap_dma_chan_read(c, CCR);
	if (od->plat->errata & DMA_ERRATA_i541 && val & CCR_TRIGGER_SRC) {
		uint32_t sysconfig;
		unsigned i;

		sysconfig = omap_dma_glbl_read(od, OCP_SYSCONFIG);
		val = sysconfig & ~DMA_SYSCONFIG_MIDLEMODE_MASK;
		val |= DMA_SYSCONFIG_MIDLEMODE(DMA_IDLEMODE_NO_IDLE);
		omap_dma_glbl_write(od, OCP_SYSCONFIG, val);

		val = omap_dma_chan_read(c, CCR);
		val &= ~CCR_ENABLE;
		omap_dma_chan_write(c, CCR, val);

		/* Wait for sDMA FIFO to drain */
		for (i = 0; ; i++) {
			val = omap_dma_chan_read(c, CCR);
			if (!(val & (CCR_RD_ACTIVE | CCR_WR_ACTIVE)))
				break;

			if (i > 100)
				break;

			udelay(5);
		}

		if (val & (CCR_RD_ACTIVE | CCR_WR_ACTIVE))
			dev_err(c->vc.chan.device->dev,
				"DMA drain did not complete on lch %d\n",
			        c->dma_ch);

		omap_dma_glbl_write(od, OCP_SYSCONFIG, sysconfig);
	} else {
		val &= ~CCR_ENABLE;
		omap_dma_chan_write(c, CCR, val);
	}

	mb();

	if (!__dma_omap15xx(od->plat->dma_attr) && c->cyclic) {
		val = omap_dma_chan_read(c, CLNK_CTRL);

		if (dma_omap1())
			val |= 1 << 14; /* set the STOP_LNK bit */
		else
			val &= ~CLNK_CTRL_ENABLE_LNK;

		omap_dma_chan_write(c, CLNK_CTRL, val);
	}
}

static void omap_dma_start_sg(struct omap_chan *c, struct omap_desc *d,
	unsigned idx)
{
	struct omap_sg *sg = d->sg + idx;
	unsigned cxsa, cxei, cxfi;

	if (d->dir == DMA_DEV_TO_MEM || d->dir == DMA_MEM_TO_MEM) {
		cxsa = CDSA;
		cxei = CDEI;
		cxfi = CDFI;
	} else {
		cxsa = CSSA;
		cxei = CSEI;
		cxfi = CSFI;
	}

	omap_dma_chan_write(c, cxsa, sg->addr);
	omap_dma_chan_write(c, cxei, 0);
	omap_dma_chan_write(c, cxfi, 0);
	omap_dma_chan_write(c, CEN, sg->en);
	omap_dma_chan_write(c, CFN, sg->fn);

	omap_dma_start(c, d);
}

static void omap_dma_start_desc(struct omap_chan *c)
{
	struct virt_dma_desc *vd = vchan_next_desc(&c->vc);
	struct omap_desc *d;
	unsigned cxsa, cxei, cxfi;

	if (!vd) {
		c->desc = NULL;
		return;
	}

	list_del(&vd->node);

	c->desc = d = to_omap_dma_desc(&vd->tx);
	c->sgidx = 0;

	/*
	 * This provides the necessary barrier to ensure data held in
	 * DMA coherent memory is visible to the DMA engine prior to
	 * the transfer starting.
	 */
	mb();

	omap_dma_chan_write(c, CCR, d->ccr);
	if (dma_omap1())
		omap_dma_chan_write(c, CCR2, d->ccr >> 16);

	if (d->dir == DMA_DEV_TO_MEM || d->dir == DMA_MEM_TO_MEM) {
		cxsa = CSSA;
		cxei = CSEI;
		cxfi = CSFI;
	} else {
		cxsa = CDSA;
		cxei = CDEI;
		cxfi = CDFI;
	}

	omap_dma_chan_write(c, cxsa, d->dev_addr);
	omap_dma_chan_write(c, cxei, 0);
	omap_dma_chan_write(c, cxfi, d->fi);
	omap_dma_chan_write(c, CSDP, d->csdp);
	omap_dma_chan_write(c, CLNK_CTRL, d->clnk_ctrl);

	omap_dma_start_sg(c, d, 0);
}

static void omap_dma_callback(int ch, u16 status, void *data)
{
	struct omap_chan *c = data;
	struct omap_desc *d;
	unsigned long flags;

	spin_lock_irqsave(&c->vc.lock, flags);
	d = c->desc;
	if (d) {
		if (!c->cyclic) {
			if (++c->sgidx < d->sglen) {
				omap_dma_start_sg(c, d, c->sgidx);
			} else {
				omap_dma_start_desc(c);
				vchan_cookie_complete(&d->vd);
			}
		} else {
			vchan_cyclic_callback(&d->vd);
		}
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);
}

/*
 * This callback schedules all pending channels.  We could be more
 * clever here by postponing allocation of the real DMA channels to
 * this point, and freeing them when our virtual channel becomes idle.
 *
 * We would then need to deal with 'all channels in-use'
 */
static void omap_dma_sched(unsigned long data)
{
	struct omap_dmadev *d = (struct omap_dmadev *)data;
	LIST_HEAD(head);

	spin_lock_irq(&d->lock);
	list_splice_tail_init(&d->pending, &head);
	spin_unlock_irq(&d->lock);

	while (!list_empty(&head)) {
		struct omap_chan *c = list_first_entry(&head,
			struct omap_chan, node);

		spin_lock_irq(&c->vc.lock);
		list_del_init(&c->node);
		omap_dma_start_desc(c);
		spin_unlock_irq(&c->vc.lock);
	}
}

static irqreturn_t omap_dma_irq(int irq, void *devid)
{
	struct omap_dmadev *od = devid;
	unsigned status, channel;

	spin_lock(&od->irq_lock);

	status = omap_dma_glbl_read(od, IRQSTATUS_L1);
	status &= od->irq_enable_mask;
	if (status == 0) {
		spin_unlock(&od->irq_lock);
		return IRQ_NONE;
	}

	while ((channel = ffs(status)) != 0) {
		unsigned mask, csr;
		struct omap_chan *c;

		channel -= 1;
		mask = BIT(channel);
		status &= ~mask;

		c = od->lch_map[channel];
		if (c == NULL) {
			/* This should never happen */
			dev_err(od->ddev.dev, "invalid channel %u\n", channel);
			continue;
		}

		csr = omap_dma_get_csr(c);
		omap_dma_glbl_write(od, IRQSTATUS_L1, mask);

		omap_dma_callback(channel, csr, c);
	}

	spin_unlock(&od->irq_lock);

	return IRQ_HANDLED;
}

static int omap_dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct omap_dmadev *od = to_omap_dma_dev(chan->device);
	struct omap_chan *c = to_omap_dma_chan(chan);
	int ret;

	if (od->legacy) {
		ret = omap_request_dma(c->dma_sig, "DMA engine",
				       omap_dma_callback, c, &c->dma_ch);
	} else {
		ret = omap_request_dma(c->dma_sig, "DMA engine", NULL, NULL,
				       &c->dma_ch);
	}

	dev_dbg(od->ddev.dev, "allocating channel %u for %u\n",
		c->dma_ch, c->dma_sig);

	if (ret >= 0) {
		omap_dma_assign(od, c, c->dma_ch);

		if (!od->legacy) {
			unsigned val;

			spin_lock_irq(&od->irq_lock);
			val = BIT(c->dma_ch);
			omap_dma_glbl_write(od, IRQSTATUS_L1, val);
			od->irq_enable_mask |= val;
			omap_dma_glbl_write(od, IRQENABLE_L1, od->irq_enable_mask);

			val = omap_dma_glbl_read(od, IRQENABLE_L0);
			val &= ~BIT(c->dma_ch);
			omap_dma_glbl_write(od, IRQENABLE_L0, val);
			spin_unlock_irq(&od->irq_lock);
		}
	}

	if (dma_omap1()) {
		if (__dma_omap16xx(od->plat->dma_attr)) {
			c->ccr = CCR_OMAP31_DISABLE;
			/* Duplicate what plat-omap/dma.c does */
			c->ccr |= c->dma_ch + 1;
		} else {
			c->ccr = c->dma_sig & 0x1f;
		}
	} else {
		c->ccr = c->dma_sig & 0x1f;
		c->ccr |= (c->dma_sig & ~0x1f) << 14;
	}
	if (od->plat->errata & DMA_ERRATA_IFRAME_BUFFERING)
		c->ccr |= CCR_BUFFERING_DISABLE;

	return ret;
}

static void omap_dma_free_chan_resources(struct dma_chan *chan)
{
	struct omap_dmadev *od = to_omap_dma_dev(chan->device);
	struct omap_chan *c = to_omap_dma_chan(chan);

	if (!od->legacy) {
		spin_lock_irq(&od->irq_lock);
		od->irq_enable_mask &= ~BIT(c->dma_ch);
		omap_dma_glbl_write(od, IRQENABLE_L1, od->irq_enable_mask);
		spin_unlock_irq(&od->irq_lock);
	}

	c->channel_base = NULL;
	od->lch_map[c->dma_ch] = NULL;
	vchan_free_chan_resources(&c->vc);
	omap_free_dma(c->dma_ch);

	dev_dbg(od->ddev.dev, "freeing channel for %u\n", c->dma_sig);
	c->dma_sig = 0;
}

static size_t omap_dma_sg_size(struct omap_sg *sg)
{
	return sg->en * sg->fn;
}

static 