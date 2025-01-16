 far).  Return the programmed destination start address in
		 * this case.
		 */
		if (addr == 0)
			addr = omap_dma_chan_read(c, CDSA);
	}

	if (dma_omap1())
		addr |= omap_dma_chan_read(c, CDSA) & 0xffff0000;

	return addr;
}

static enum dma_status omap_dma_tx_status(struct dma_chan *chan,
	dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	struct omap_chan *c = to_omap_dma_chan(chan);
	struct virt_dma_desc *vd;
	enum dma_status ret;
	unsigned long flags;

	ret = dma_cookie_status(chan, cookie, txstate);
	if (ret == DMA_COMPLETE || !txstate)
		return ret;

	spin_lock_irqsave(&c->vc.lock, flags);
	vd = vchan_find_desc(&c->vc, cookie);
	if (vd) {
		txstate->residue = omap_dma_desc_size(to_omap_dma_desc(&vd->tx));
	} else if (c->desc && c->desc->vd.tx.cookie == cookie) {
		struct omap_desc *d = c->desc;
		dma_addr_t pos;

		if (d->dir == DMA_MEM_TO_DEV)
			pos = omap_dma_get_src_pos(c);
		else if (d->dir == DMA_DEV_TO_MEM)
			pos = omap_dma_get_dst_pos(c);
		else
			pos = 0;

		txstate->residue = omap_dma_desc_size_pos(d, pos);
	} else {
		txstate->residue = 0;
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);

	return ret;
}

static void omap_dma_issue_pending(struct dma_chan *chan)
{
	struct omap_chan *c = to_omap_dma_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&c->vc.lock, flags);
	if (vchan_issue_pending(&c->vc) && !c->desc) {
		/*
		 * c->cyclic is used only by audio and in this case the DMA need
		 * to be started without delay.
		 */
		if (!c->cyclic) {
			struct omap_dmadev *d = to_omap_dma_dev(chan->device);
			spin_lock(&d->lock);
			if (list_empty(&c->node))
				list_add_tail(&c->node, &d->pending);
			spin_unlock(&d->lock);
			tasklet_schedule(&d->task);
		} else {
			omap_dma_start_desc(c);
		}
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);
}

static struct dma_async_tx_descriptor *omap_dma_prep_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl, unsigned sglen,
	enum dma_transfer_direction dir, unsigned long tx_flags, void *context)
{
	struct omap_dmadev *od = to_omap_dma_dev(chan->device);
	struct omap_chan *c = to_omap_dma_chan(chan);
	enum dma_slave_buswidth dev_width;
	struct scatterlist *sgent;
	struct omap_desc *d;
	dma_addr_t dev_addr;
	unsigned i, j = 0, es, en, frame_bytes;
	u32 burst;

	if (dir == DMA_DEV_TO_MEM) {
		dev_addr = c->cfg.src_addr;
		dev_width = c->cfg.src_addr_width;
		burst = c->cfg.src_maxburst;
	} else if (dir == DMA_MEM_TO_DEV) {
		dev_addr = c->cfg.dst_addr;
		dev_width = c->cfg.dst_addr_width;
		burst = c->cfg.dst_maxburst;
	} else {
		dev_err(chan->device->dev, "%s: bad direction?\n", __func__);
		return NULL;
	}

	/* Bus width translates to the element size (ES) */
	switch (dev_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		es = CSDP_DATA_TYPE_8;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		es = CSDP_DATA_TYPE_16;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		es = CSDP_DATA_TYPE_32;
		break;
	default: /* not reached */
		return NULL;
	}

	/* Now allocate and setup the descriptor. */
	d = kzalloc(sizeof(*d) + sglen * sizeof(d->sg[0]), GFP_ATOMIC);
	if (!d)
		return NULL;

	d->dir = dir;
	d->dev_addr = dev_addr;
	d->es = es;

	d->ccr = c->ccr | CCR_SYNC_FRAME;
	if (dir == DMA_DEV_TO_MEM)
		d->ccr |= CCR_DST_AMODE_POSTINC | CCR_SRC_AMODE_CONSTANT;
	else
		d->ccr |= CCR_DST_AMODE_CONSTANT | CCR_SRC_AMODE_POSTINC;

	d->cicr = CICR_DROP_IE | CICR_BLOCK_IE;
	d->csdp = es;

	if (dma_omap1()) {
		d->cicr |= CICR_TOUT_IE;

		if (dir == DMA_DEV_TO_MEM)
			d->csdp |= CSDP_DST_PORT_EMIFF | CSDP_SRC_PORT_TIPB;
		else
			d->csdp |= CSDP_DST_PORT_TIPB | CSDP_SRC_PORT_EMIFF;
	} else {
		if (dir == DMA_DEV_TO_MEM)
			d->ccr |= CCR_TRIGGER_SRC;

		d->cicr |= CICR_MISALIGNED_ERR_IE | CICR_TRANS_ERR_IE;
	}
	if (od->plat->errata & DMA_ERRATA_PARALLEL_CHANNELS)
		d->clnk_ctrl = c->dma_ch;

	/*
	 * Build our scatterlist entries: each contains the address,
	 * the number of elements (EN) in each frame, and the number of
	 * frames (FN).  Number of bytes for this entry = ES * EN * FN.
	 *
	 * Burst size translates to number of elements with frame sync.
	 * Note: DMA engine defines burst to be the number of dev-width
	 * transfers.
	 */
	en = burst;
	frame_bytes = es_bytes[es] * en;
	for_each_sg(sgl, sgent, sglen, i) {
		d->sg[j].addr = sg_dma_address(sgent);
		d->sg[j].en = en;
		d->sg[j].fn = sg_dma_len(sgent) / frame_bytes;
		j++;
	}

	d->sglen = j;

	return vchan_tx_prep(&c->vc, &d->vd, tx_flags);
}

static struct dma_async_tx_descriptor *omap_dma_prep_dma_cyclic(
	struct dma_chan *chan, dma_addr_t buf_addr, size_t buf_len,
	size_t period_len, enum dma_transfer_direction dir, unsigned long flags,
	void *context)
{
	struct omap_dmadev *od = to_omap_dma_dev(chan->device);
	struct omap_chan *c = to_omap_dma_chan(chan);
	enum dma_slave_buswidth dev_width;
	struct omap_desc *d;
	dma_addr_t dev_addr;
	unsigned es;
	u32 burst;

	if (dir == DMA_DEV_TO_MEM) {
		dev_addr = c->cfg.src_addr;
		dev_width = c->cfg.src_addr_width;
		burst = c->cfg.src_maxburst;
	} else if (dir == DMA_MEM_TO_DEV) {
		dev_addr = c->cfg.dst_addr;
		dev_width = c->cfg.dst_addr_width;
		burst = c->cfg.dst_maxburst;
	} else {
		dev_err(chan->device->dev, "%s: bad direction?\n", __func__);
		return NULL;
	}

	/* Bus width translates to the element size (ES) */
	switch (dev_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		es = CSDP_DATA_TYPE_8;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		es = CSDP_DATA_TYPE_16;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		es = CSDP_DATA_TYPE_32;
		break;
	default: /* not reached */
		return NULL;
	}

	/* Now allocate and setup the descriptor. */
	d = kzalloc(sizeof(*d) + sizeof(d->sg[0]), GFP_ATOMIC);
	if (!d)
		return NULL;

	d->dir = dir;
	d->dev_addr = dev_addr;
	d->fi = burst;
	d->es = es;
	d->sg[0].addr = buf_addr;
	d->sg[0].en = period_len / es_bytes[es];
	d->sg[0].fn = buf_len / period_len;
	d->sglen = 1;

	d->ccr = c->ccr;
	if (dir == DMA_DEV_TO_MEM)
		d->ccr |= CCR_DST_AMODE_POSTINC | CCR_SRC_AMODE_CONSTANT;
	else
		d->ccr |= CCR_DST_AMODE_CONSTANT | CCR_SRC_AMODE_POSTINC;

	d->cicr = CICR_DROP_IE;
	if (flags & DMA_PREP_INTERRUPT)
		d->cicr |= CICR_FRAME_IE;

	d->csdp = es;

	if (dma_omap1()) {
		d->cicr |= CICR_TOUT_IE;

		if (dir == DMA_DEV_TO_MEM)
			d->csdp |= CSDP_DST_PORT_EMIFF | CSDP_SRC_PORT_MPUI;
		else
			d->csdp |= CSDP_DST_PORT_MPUI | CSDP_SRC_PORT_EMIFF;
	} else {
		if (burst)
			d->ccr |= CCR_SYNC_PACKET;
		else
			d->ccr |= CCR_SYNC_ELEMENT;

		if (dir == DMA_DEV_TO_MEM) {
			d->ccr |= CCR_TRIGGER_SRC;
			d->csdp |= CSDP_DST_PACKED;
		} else {
			d->csdp |= CSDP_SRC_PACKED;
		}

		d->cicr |= CICR_MISALIGNED_ERR_IE | CICR_TRANS_ERR_IE;

		d->csdp |= CSDP_DST_BURST_64 | CSDP_SRC_BURST_64;
	}

	if (__dma_omap15xx(od->plat->dma_attr))
		d->ccr |= CCR_AUTO_INIT | CCR_REPEAT;
	else
		d->clnk_ctrl = c->dma_ch | CLNK_CTRL_ENABLE_LNK;

	c->cyclic = true;

	return vchan_tx_prep(&c->vc, &d->vd, flags);
}

static struct dma_async_tx_descriptor *omap_dma_prep_dma_memcpy(
	struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
	size_t len, unsigned long tx_flags)
{
	struct omap_chan *c = to_omap_dma_chan(chan);
	struct omap_desc *d;
	uint8_t data_type;

	d = kzalloc(sizeof(*d) + sizeof(d->sg[0]), GFP_ATOMIC);
	if (!d)
		return NULL;

	data_type = __ffs((src | dest | len));
	if (data_type > CSDP_DATA_TYPE_32)
		data_type = CSDP_DATA_TYPE_32;

	d->dir = DMA_MEM_TO_MEM;
	d->dev_addr = src;
	d->fi = 0;
	d->es = data_type;
	d->sg[0].en = len / BIT(data_type);
	d->sg[0].fn = 1;
	d->sg[0].addr = dest;
	d->sglen = 1;
	d->ccr = c->ccr;
	d->ccr |= CCR_DST_AMODE_POSTINC | CCR_SRC_AMODE_POSTINC;

	d->cicr = CICR_DROP_IE;
	if (tx_flags & DMA_PREP_INTERRUPT)
		d->cicr |= CICR_FRAME_IE;

	d->csdp = data_type;

	if (dma_omap1()) {
		d->cicr |= CICR_TOUT_IE;
		d->csdp |= CSDP_DST_PORT_EMIFF | CSDP_SRC_PORT_EMIFF;
	} else {
		d->csdp |= CSDP_DST_PACKED | CSDP_SRC_PACKED;
		d->cicr |= CICR_MISALIGNED_ERR_IE | CICR_TRANS_ERR_IE;
		d->csdp |= CSDP_DST_BURST_64 | CSDP_SRC_BURST_64;
	}

	return vchan_tx_prep(&c->vc, &d->vd, tx_flags);
}

static int omap_dma_slave_config(struct dma_chan *chan, struct dma_slave_config *cfg)
{
	struct omap_chan *c = to_omap_dma_chan(chan);

	if (cfg->src_addr_width == DMA_SLAVE_BUSWIDTH_8_BYTES ||
	    cfg->dst_addr_width == DMA_SLAVE_BUSWIDTH_8_BYTES)
		return -EINVAL;

	memcpy(&c->cfg, cfg, sizeof(c->cfg));

	return 0;
}

static int omap_dma_terminate_all(struct dma_chan *chan)
{
	struct omap_chan *c = to_omap_dma_chan(chan);
	struct omap_dmadev *d = to_omap_dma_dev(c->vc.chan.device);
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&c->vc.lock, flags);

	/* Prevent this channel being scheduled */
	spin_lock(&d->lock);
	list_del_init(&c->node);
	spin_unlock(&d->lock);

	/*
	 * Stop DMA activity: we assume the callback will not be called
	 * after omap_dma_stop() returns (even if it does, it will see
	 * c->desc is NULL and exit.)
	 */
	if (c->desc) {
		omap_dma_desc_free(&c->desc->vd);
		c->desc = NULL;
		/* Avoid stopping the dma twice */
		if (!c->paused)
			omap_dma_stop(c);
	}

	if (c->cyclic) {
		c->cyclic = false;
		c->paused = false;
	}

	vchan_get_all_descriptors(&c->vc, &head);
	spin_unlock_irqrestore(&c->vc.lock, flags);
	vchan_dma_desc_free_list(&c->vc, &head);

	return 0;
}

static int omap_dma_pause(struct dma_chan *chan)
{
	struct omap_chan *c = to_omap_dma_chan(chan);

	/* Pause/Resume only allowed with cyclic mode */
	if (!c->cyclic)
		return -EINVAL;

	if (!c->paused) {
		omap_dma_stop(c);
		c->paused = true;
	}

	return 0;
}

static int omap_dma_resume(struct dma_chan *chan)
{
	struct omap_chan *c = to_omap_dma_chan(chan);

	/* Pause/Resume only allowed with cyclic mode */
	if (!c->cyclic)
		return -EINVAL;

	if (c->paused) {
		mb();

		/* Restore channel link register */
		omap_dma_chan_write(c, CLNK_CTRL, c->desc->clnk_ctrl);

		omap_dma_start(c, c->desc);
		c->paused = false;
	}

	return 0;
}

static int omap_dma_chan_init(struct omap_dmadev *od)
{
	struct omap_chan *c;

	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	c->reg_map = od->reg_map;
	c->vc.desc_free = omap_dma_desc_free;
	vchan_init(&c->vc, &od->ddev);
	INIT_LIST_HEAD(&c->node);

	return 0;
}

static void omap_dma_free(struct omap_dmadev *od)
{
	tasklet_kill(&od->task);
	while (!list_empty(&od->ddev.channels)) {
		struct omap_chan *c = list_first_entry(&od->ddev.channels,
			struct omap_chan, vc.chan.device_node);

		list_del(&c->vc.chan.device_node);
		tasklet_kill(&c->vc.task);
		kfree(c);
	}
}

#define OMAP_DMA_BUSWIDTHS	(BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) | \
				 BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) | \
				 BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))

static int omap_dma_probe(struct platform_device *pdev)
{
	struct omap_dmadev *od;
	struct resource *res;
	int rc, i, irq;

	od = devm_kzalloc(&pdev->dev, sizeof(*od), GFP_KERNEL);
	if (!od)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	od->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(od->base))
		return PTR_ERR(od->base);

	od->plat = omap_get_plat_info();
	if (!od->plat)
		return -EPROBE_DEFER;

	od->reg_map = od->plat->reg_map;

	dma_cap_set(DMA_SLAVE, od->ddev.cap_mask);
	dma_cap_set(DMA_CYCLIC, od->ddev.cap_mask);
	dma_cap_set(DMA_MEMCPY, od->ddev.cap_mask);
	od->ddev.device_alloc_chan_resources = omap_dma_alloc_chan_resources;
	od->ddev.device_free_chan_resources = omap_dma_free_chan_resources;
	od->ddev.device_tx_status = omap_dma_tx_status;
	od->ddev.device_issue_pending = omap_dma_issue_pending;
	od->ddev.device_prep_slave_sg = omap_dma_prep_slave_sg;
	od->ddev.device_prep_dma_cyclic = omap_dma_prep_dma_cyclic;
	od->ddev.device_prep_dma_memcpy = omap_dma_prep_dma_memcpy;
	od->ddev.device_config = omap_dma_slave_config;
	od->ddev.device_pause = omap_dma_pause;
	od->ddev.device_resume = omap_dma_resume;
	od->ddev.device_terminate_all = omap_dma_terminate_all;
	od->ddev.src_addr_widths = OMAP_DMA_BUSWIDTHS;
	od->ddev.dst_addr_widths = OMAP_DMA_BUSWIDTHS;
	od->ddev.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	od->ddev.residue_granularity = DMA_RESIDUE_GRANULARITY_BURST;
	od->ddev.dev = &pdev->dev;
	INIT_LIST_HEAD(&od->ddev.channels);
	INIT_LIST_HEAD(&od->pending);
	spin_lock_init(&od->lock);
	spin_lock_init(&od->irq_lock);

	tasklet_init(&od->task, omap_dma_sched, (unsigned long)od);

	od->dma_requests = OMAP_SDMA_REQUESTS;
	if (pdev->dev.of_node && of_property_read_u32(pdev->dev.of_node,
						      "dma-requests",
						      &od->dma_requests)) {
		dev_info(&pdev->dev,
			 "Missing dma-requests property, using %u.\n",
			 OMAP_SDMA_REQUESTS);
	}

	for (i = 0; i < OMAP_SDMA_CHANNELS; i++) {
		rc = omap_dma_chan_init(od);
		if (rc) {
			omap_dma_free(od);
			return rc;
		}
	}

	irq = platform_get_irq(pdev, 1);
	if (irq <= 0) {
		dev_info(&pdev->dev, "failed to get L1 IRQ: %d\n", irq);
		od->legacy = true;
	} else {
		/* Disable all interrupts */
		od->irq_enable_mask = 0;
		omap_dma_glbl_write(od, IRQENABLE_L1, 0);

		rc = devm_request_irq(&pdev->dev, irq, omap_dma_irq,
				      IRQF_SHARED, "omap-dma-engine", od);
		if (rc) {
			omap_dma_free(od);
			return rc;
		}
	}

	rc = dma_async_device_register(&od->ddev);
	if (rc) {
		pr_warn("OMAP-DMA: failed to register slave DMA engine device: %d\n",
			rc);
		omap_dma_free(od);
		return rc;
	}

	platform_set_drvdata(pdev, od);

	if (pdev->dev.of_node) {
		omap_dma_info.dma_cap = od->ddev.cap_mask;

		/* Device-tree DMA controller registration */
		rc = of_dma_controller_register(pdev->dev.of_node,
				of_dma_simple_xlate, &omap_dma_info);
		if (rc) {
			pr_warn("OMAP-DMA: failed to register DMA controller\n");
			dma_async_device_unregister(&od->ddev);
			omap_dma_free(od);
		}
	}

	dev_info(&pdev->dev, "OMAP DMA engine driver\n");

	return rc;
}

static int omap_dma_remove(struct platform_device *pdev)
{
	struct omap_dmadev *od = platform_get_drvdata(pdev);

	if (pdev->dev.of_node)
		of_dma_controller_free(pdev->dev.of_node);

	dma_async_device_unregister(&od->ddev);

	if (!od->legacy) {
		/* Disable all interrupts */
		omap_dma_glbl_write(od, IRQENABLE_L0, 0);
	}

	omap_dma_free(od);

	return 0;
}

static const struct of_device_id omap_dma_match[] = {
	{ .compatible = "ti,omap2420-sdma", },
	{ .compatible = "ti,omap2430-sdma", },
	{ .compatible = "ti,omap3430-sdma", },
	{ .compatible = "ti,omap3630-sdma", },
	{ .compatible = "ti,omap4430-sdma", },
	{},
};
MODULE_DEVICE_TABLE(of, omap_dma_match);

static struct platform_driver omap_dma_driver = {
	.probe	= omap_dma_probe,
	.remove	= omap_dma_remove,
	.driver = {
		.name = "omap-dma-engine",
		.of_match_table = of_match_ptr(omap_dma_match),
	},
};

bool omap_dma_filter_fn(struct dma_chan *chan, void *param)
{
	if (chan->device->dev->driver == &omap_dma_driver.driver) {
		struct omap_dmadev *od = to_omap_dma_dev(chan->device);
		struct omap_chan *c = to_omap_dma_chan(chan);
		unsigned req = *(unsigned *)param;

		if (req <= od->dma_requests) {
			c->dma_sig = req;
			return true;
		}
	}
	return false;
}
EXPORT_SYMBOL_GPL(omap_dma_filter_fn);

static int omap_dma_init(void)
{
	return platform_driver_register(&omap_dma_driver);
}
subsys_initcall(omap_dma_init);

static void __exit omap_dma_exit(void)
{
	platform_driver_unregister(&omap_dma_driver);
}
module_exit(omap_dma_exit);

MODULE_AUTHOR("Russell King");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * Topcliff PCH DMA controller driver
 * Copyright (c) 2010 Intel Corporation
 * Copyright (C) 2011 LAPIS Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pch_dma.h>

#include "dmaengine.h"

#define DRV_NAME "pch-dma"

#define DMA_CTL0_DISABLE		0x0
#define DMA_CTL0_SG			0x1
#define DMA_CTL0_ONESHOT		0x2
#define DMA_CTL0_MODE_MASK_BITS		0x3
#define DMA_CTL0_DIR_SHIFT_BITS		2
#define DMA_CTL0_BITS_PER_CH		4

#define DMA_CTL2_START_SHIFT_BITS	8
#define DMA_CTL2_IRQ_ENABLE_MASK	((1UL << DMA_CTL2_START_SHIFT_BITS) - 1)

#define DMA_STATUS_IDLE			0x0
#define DMA_STATUS_DESC_READ		0x1
#define DMA_STATUS_WAIT			0x2
#define DMA_STATUS_ACCESS		0x3
#define DMA_STATUS_BITS_PER_CH		2
#define DMA_STATUS_MASK_BITS		0x3
#define DMA_STATUS_SHIFT_BITS		16
#define DMA_STATUS_IRQ(x)		(0x1 << (x))
#define DMA_STATUS0_ERR(x)		(0x1 << ((x) + 8))
#define DMA_STATUS2_ERR(x)		(0x1 << (x))

#define DMA_DESC_WIDTH_SHIFT_BITS	12
#define DMA_DESC_WIDTH_1_BYTE		(0x3 << DMA_DESC_WIDTH_SHIFT_BITS)
#define DMA_DESC_WIDTH_2_BYTES		(0x2 << DMA_DESC_WIDTH_SHIFT_BITS)
#define DMA_DESC_WIDTH_4_BYTES		(0x0 << DMA_DESC_WIDTH_SHIFT_BITS)
#define DMA_DESC_MAX_COUNT_1_BYTE	0x3FF
#define DMA_DESC_MAX_COUNT_2_BYTES	0x3FF
#define DMA_DESC_MAX_COUNT_4_BYTES	0x7FF
#define DMA_DESC_END_WITHOUT_IRQ	0x0
#define DMA_DESC_END_WITH_IRQ		0x1
#define DMA_DESC_FOLLOW_WITHOUT_IRQ	0x2
#define DMA_DESC_FOLLOW_WITH_IRQ	0x3

#define MAX_CHAN_NR			12

#define DMA_MASK_CTL0_MODE	0x33333333
#define DMA_MASK_CTL2_MODE	0x00003333

static unsigned int init_nr_desc_per_channel = 64;
module_param(init_nr_desc_per_channel, uint, 0644);
MODULE_PARM_DESC(init_nr_desc_per_channel,
		 "initial descriptors per channel (default: 64)");

struct pch_dma_desc_regs {
	u32	dev_addr;
	u32	mem_addr;
	u32	size;
	u32	next;
};

struct pch_dma_regs {
	u32	dma_ctl0;
	u32	dma_ctl1;
	u32	dma_ctl2;
	u32	dma_ctl3;
	u32	dma_sts0;
	u32	dma_sts1;
	u32	dma_sts2;
	u32	reserved3;
	struct pch_dma_desc_regs desc[MAX_CHAN_NR];
};

struct pch_dma_desc {
	struct pch_dma_desc_regs regs;
	struct dma_async_tx_descriptor txd;
	struct list_head	desc_node;
	struct list_head	tx_list;
};

struct pch_dma_chan {
	struct dma_chan		chan;
	void __iomem *membase;
	enum dma_transfer_direction dir;
	struct tasklet_struct	tasklet;
	unsigned long		err_status;

	spinlock_t		lock;

	struct list_head	active_list;
	struct list_head	queue;
	struct list_head	free_list;
	unsigned int		descs_allocated;
};

#define PDC_DEV_ADDR	0x00
#define PDC_MEM_ADDR	0x04
#define PDC_SIZE	0x08
#define PDC_NEXT	0x0C

#define channel_readl(pdc, name) \
	readl((pdc)->membase + PDC_##name)
#define channel_writel(pdc, name, val) \
	writel((val), (pdc)->membase + PDC_##name)

struct pch_dma {
	struct dma_device	dma;
	void __iomem *membase;
	struct pci_pool		*pool;
	struct pch_dma_regs	regs;
	struct pch_dma_desc_regs ch_regs[MAX_CHAN_NR];
	struct pch_dma_chan	channels[MAX_CHAN_NR];
};

#define PCH_DMA_CTL0	0x00
#define PCH_DMA_CTL1	0x04
#define PCH_DMA_CTL2	0x08
#define PCH_DMA_CTL3	0x0C
#define PCH_DMA_STS0	0x10
#define PCH_DMA_STS1	0x14
#define PCH_DMA_STS2	0x18

#define dma_readl(pd, name) \
	readl((pd)->membase + PCH_DMA_##name)
#define dma_writel(pd, name, val) \
	writel((val), (pd)->membase + PCH_DMA_##name)

static inline
struct pch_dma_desc *to_pd_desc(struct dma_async_tx_descriptor *txd)
{
	return container_of(txd, struct pch_dma_desc, txd);
}

static inline struct pch_dma_chan *to_pd_chan(struct dma_chan *chan)
{
	return container_of(chan, struct pch_dma_chan, chan);
}

static inline struct pch_dma *to_pd(struct dma_device *ddev)
{
	return container_of(ddev, struct pch_dma, dma);
}

static inline struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static inline struct device *chan2parent(struct dma_chan *chan)
{
	return chan->dev->device.parent;
}

static inline
struct pch_dma_desc *pdc_first_active(struct pch_dma_chan *pd_chan)
{
	return list_first_entry(&pd_chan->active_list,
				struct pch_dma_desc, desc_node);
}

static inline
struct pch_dma_desc *pdc_first_queued(struct pch_dma_chan *pd_chan)
{
	return list_first_entry(&pd_chan->queue,
				struct pch_dma_desc, desc_node);
}

static void pdc_enable_irq(struct dma_chan *chan, int enable)
{
	struct pch_dma *pd = to_pd(chan->device);
	u32 val;
	int pos;

	if (chan->chan_id < 8)
		pos = chan->chan_id;
	else
		pos = chan->chan_id + 8;

	val = dma_readl(pd, CTL2);

	if (enable)
		val |= 0x1 << pos;
	else
		val &= ~(0x1 << pos);

	dma_writel(pd, CTL2, val);

	dev_dbg(chan2dev(chan), "pdc_enable_irq: chan %d -> %x\n",
		chan->chan_id, val);
}

static void pdc_set_dir(struct dma_chan *chan)
{
	struct pch_dma_chan *pd_chan = to_pd_chan(chan);
	struct pch_dma *pd = to_pd(chan->device);
	u32 val;
	u32 mask_mode;
	u32 mask_ctl;

	if (chan->chan_id < 8) {
		val = dma_readl(pd, CTL0);

		mask_mode = DMA_CTL0_MODE_MASK_BITS <<
					(DMA_CTL0_BITS_PER_CH * chan->chan_id);
		mask_ctl = DMA_MASK_CTL0_MODE & ~(DMA_CTL0_MODE_MASK_BITS <<
				       (DMA_CTL0_BITS_PER_CH * chan->chan_id));
		val &= mask_mode;
		if (pd_chan->dir == DMA_MEM_TO_DEV)
			val |= 0x1 << (DMA_CTL0_BITS_PER_CH * chan->chan_id +
				       DMA_CTL0_DIR_SHIFT_BITS);
		else
			val &= ~(0x1 << (DMA_CTL0_BITS_PER_CH * chan->chan_id +
					 DMA_CTL0_DIR_SHIFT_BITS));

		val |= mask_ctl;
		dma_writel(pd, CTL0, val);
	} else {
		int ch = chan->chan_id - 8; /* ch8-->0 ch9-->1 ... ch11->3 */
		val = dma_readl(pd, CTL3);

		mask_mode = DMA_CTL0_MODE_MASK_BITS <<
						(DMA_CTL0_BITS_PER_CH * ch);
		mask_ctl = DMA_MASK_CTL2_MODE & ~(DMA_CTL0_MODE_MASK_BITS <<
						 (DMA_CTL0_BITS_PER_CH * ch));
		val &= mask_mode;
		if (pd_chan->dir == DMA_MEM_TO_DEV)
			val |= 0x1 << (DMA_CTL0_BITS_PER_CH * ch +
				       DMA_CTL0_DIR_SHIFT_BITS);
		else
			val &= ~(0x1 << (DMA_CTL0_BITS_PER_CH * ch +
					 DMA_CTL0_DIR_SHIFT_BITS));
		val |= mask_ctl;
		dma_writel(pd, CTL3, val);
	}

	dev_dbg(chan2dev(chan), "pdc_set_dir: chan %d -> %x\n",
		chan->chan_id, val);
}

static void pdc_set_mode(struct dma_chan *chan, u32 mode)
{
	struct pch_dma *pd = to_pd(chan->device);
	u32 val;
	u32 mask_ctl;
	u32 mask_dir;

	if (chan->chan_id < 8) {
		mask_ctl = DMA_MASK_CTL0_MODE & ~(DMA_CTL0_MODE_MASK_BITS <<
			   (DMA_CTL0_BITS_PER_CH * chan->chan_id));
		mask_dir = 1 << (DMA_CTL0_BITS_PER_CH * chan->chan_id +\
				 DMA_CTL0_DIR_SHIFT_BITS);
		val = dma_readl(pd, CTL0);
		val &= mask_dir;
		val |= mode << (DMA_CTL0_BITS_PER_CH * chan->chan_id);
		val |= mask_ctl;
		dma_writel(pd, CTL0, val);
	} else {
		int ch = chan->chan_id - 8; /* ch8-->0 ch9-->1 ... ch11->3 */
		mask_ctl = DMA_MASK_CTL2_MODE & ~(DMA_CTL0_MODE_MASK_BITS <<
						 (DMA_CTL0_BITS_PER_CH * ch));
		mask_dir = 1 << (DMA_CTL0_BITS_PER_CH * ch +\
				 DMA_CTL0_DIR_SHIFT_BITS);
		val = dma_readl(pd, CTL3);
		val &= mask_dir;
		val |= mode << (DMA_CTL0_BITS_PER_CH * ch);
		val |= mask_ctl;
		dma_writel(pd, CTL3, val);
	}

	dev_dbg(chan2dev(chan), "pdc_set_mode: chan %d -> %x\n",
		chan->chan_id, val);
}

static u32 pdc_get_status0(struct pch_dma_chan *pd_chan)
{
	struct pch_dma *pd = to_pd(pd_chan->chan.device);
	u32 val;

	val = dma_readl(pd, STS0);
	return DMA_STATUS_MASK_BITS & (val >> (DMA_STATUS_SHIFT_BITS +
			DMA_STATUS_BITS_PER_CH * pd_chan->chan.chan_id));
}

static u32 pdc_get_status2(struct pch_dma_chan *pd_chan)
{
	struct pch_dma *pd = to_pd(pd_chan->chan.device);
	u32 val;

	val = dma_readl(pd, STS2);
	return DMA_STATUS_MASK_BITS & (val >> (DMA_STATUS_SHIFT_BITS +
			DMA_STATUS_BITS_PER_CH * (pd_chan->chan.chan_id - 8)));
}

static bool pdc_is_idle(struct pch_dma_chan *pd_chan)
{
	u32 sts;

	if (pd_chan->chan.chan_id < 8)
		sts = pdc_get_status0(pd_chan);
	else
		sts = pdc_get_status2(pd_chan);


	if (sts == DMA_STATUS_IDLE)
		return true;
	else
		return false;
}

static void pdc_dostart(struct pch_dma_chan *pd_chan, struct pch_dma_desc* desc)
{
	if (!pdc_is_idle(pd_chan)) {
		dev_err(chan2dev(&pd_chan->chan),
			"BUG: Attempt to start non-idle channel\n");
		return;
	}

	dev_dbg(chan2dev(&pd_chan->chan), "chan %d -> dev_addr: %x\n",
		pd_chan->chan.chan_id, desc->regs.dev_addr);
	dev_dbg(chan2dev(&pd_chan->chan), "chan %d -> mem_addr: %x\n",
		pd_chan->chan.chan_id, desc->regs.mem_addr);
	dev_dbg(chan2dev(&pd_chan->chan), "chan %d -> size: %x\n",
		pd_chan->chan.chan_id, desc->regs.size);
	dev_dbg(chan2dev(&pd_chan->chan), "chan %d -> next: %x\n",
		pd_chan->chan.chan_id, desc->regs.next);

	if (list_empty(&desc->tx_list)) {
		channel_writel(pd_chan, DEV_ADDR, desc->regs.dev_addr);
		channel_writel(pd_chan, MEM_ADDR, desc->regs.mem_addr);
		channel_writel(pd_chan, SIZE, desc->regs.size);
		channel_writel(pd_chan, NEXT, desc->regs.next);
		pdc_set_mode(&pd_chan->chan, DMA_CTL0_ONESHOT);
	} else {
		channel_writel(pd_chan, NEXT, desc->txd.phys);
		pdc_set_mode(&pd_chan->chan, DMA_CTL0_SG);
	}
}

static void pdc_chain_complete(struct pch_dma_chan *pd_chan,
			       struct pch_dma_desc *desc)
{
	struct dma_async_tx_descriptor *txd = &desc->txd;
	dma_async_tx_callback callback = txd->callback;
	void *param = txd->callback_param;

	list_splice_init(&desc->tx_list, &pd_chan->free_list);
	list_move(&desc->desc_node, &pd_chan->free_list);

	if (callback)
		callback(param);
}

static void pdc_complete_all(struct pch_dma_chan *pd_chan)
{
	struct pch_dma_desc *desc, *_d;
	LIST_HEAD(list);

	BUG_ON(!pdc_is_idle(pd_chan));

	if (!list_empty(&pd_chan->queue))
		pdc_dostart(pd_chan, pdc_first_queued(pd_chan));

	list_splice_init(&pd_chan->active_list, &list);
	list_splice_init(&pd_chan->queue, &pd_chan->active_list);

	list_for_each_entry_safe(desc, _d, &list, desc_node)
		pdc_chain_complete(pd_chan, desc);
}

static void pdc_handle_error(struct pch_dma_chan *pd_chan)
{
	struct pch_dma_desc *bad_desc;

	bad_desc = pdc_first_active(pd_chan);
	list_del(&bad_desc->desc_node);

	list_splice_init(&pd_chan->queue, pd_chan->active_list.prev);

	if (!list_empty(&pd_chan->active_list))
		pdc_dostart(pd_chan, pdc_first_active(pd_chan));

	dev_crit(chan2dev(&pd_chan->chan), "Bad descriptor submitted\n");
	dev_crit(chan2dev(&pd_chan->chan), "descriptor cookie: %d\n",
		 bad_desc->txd.cookie);

	pdc_chain_complete(pd_chan, bad_desc);
}

static void pdc_advance_work(struct pch_dma_chan *pd_chan)
{
	if (list_empty(&pd_chan->active_list) ||
		list_is_singular(&pd_chan->active_list)) {
		pdc_complete_all(pd_chan);
	} else {
		pdc_chain_complete(pd_chan, pdc_first_active(pd_chan));
		pdc_dostart(pd_chan, pdc_first_active(pd_chan));
	}
}

static dma_cookie_t pd_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct pch_dma_desc *desc = to_pd_desc(txd);
	struct pch_dma_chan *pd_chan = to_pd_chan(txd->chan);
	dma_cookie_t cookie;

	spin_lock(&pd_chan->lock);
	cookie = dma_cookie_assign(txd);

	if (list_empty(&pd_chan->active_list)) {
		list_add_tail(&desc->desc_node, &pd_chan->active_list);
		pdc_dostart(pd_chan, desc);
	} else {
		list_add_tail(&desc->desc_node, &pd_chan->queue);
	}

	spin_unlock(&pd_chan->lock);
	return 0;
}

static struct pch_dma_desc *pdc_alloc_desc(struct dma_chan *chan, gfp_t flags)
{
	struct pch_dma_desc *desc = NULL;
	struct pch_dma *pd = to_pd(chan->device);
	dma_addr_t addr;

	desc = pci_pool_alloc(pd->pool, flags, &addr);
	if (desc) {
		memset(desc, 0, sizeof(struct pch_dma_desc));
		INIT_LIST_HEAD(&desc->tx_list);
		dma_async_tx_descriptor_init(&desc->txd, chan);
		desc->txd.tx_submit = pd_tx_submit;
		desc->txd.flags = DMA_CTRL_ACK;
		desc->txd.phys = addr;
	}

	return desc;
}

static struct pch_dma_desc *pdc_desc_get(struct pch_dma_chan *pd_chan)
{
	struct pch_dma_desc *desc, *_d;
	struct pch_dma_desc *ret = NULL;
	int i = 0;

	spin_lock(&pd_chan->lock);
	list_for_each_entry_safe(desc, _d, 