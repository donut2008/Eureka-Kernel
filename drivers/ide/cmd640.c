
	raw_spin_lock_irqsave(&bank_lock, lock_flags);
	bank = map->bank;
	ret = bank && ipu_read_reg(bank->ipu, bank->status) &
		(1UL << (map->source & 31));
	raw_spin_unlock_irqrestore(&bank_lock, lock_flags);

	return ret;
}

/**
 * ipu_irq_map() - map an IPU interrupt source to an IRQ number
 * @source:	interrupt source bit position (see below)
 * @return:	mapped IRQ number or negative error code
 *
 * The source parameter has to be explained further. On i.MX31 IPU has 137 IRQ
 * sources, they are broken down in 5 32-bit registers, like 32, 32, 24, 32, 17.
 * However, the source argument of this function is not the sequence number of
 * the possible IRQ, but rather its bit position. So, first interrupt in fourth
 * register has source number 96, and not 88. This makes calculations easier,
 * and also provides forward compatibility with any future IPU implementations
 * with any interrupt bit assignments.
 */
int ipu_irq_map(unsigned int source)
{
	int i, ret = -ENOMEM;
	struct ipu_irq_map *map;

	might_sleep();

	mutex_lock(&map_lock);
	map = src2map(source);
	if (map) {
		pr_err("IPU: Source %u already mapped to IRQ %u\n", source, map->irq);
		ret = -EBUSY;
		goto out;
	}

	for (i = 0; i < CONFIG_MX3_IPU_IRQS; i++) {
		if (irq_map[i].source < 0) {
			unsigned long lock_flags;

			raw_spin_lock_irqsave(&bank_lock, lock_flags);
			irq_map[i].source = source;
			irq_map[i].bank = irq_bank + source / 32;
			raw_spin_unlock_irqrestore(&bank_lock, lock_flags);

			ret = irq_map[i].irq;
			pr_debug("IPU: mapped source %u to IRQ %u\n",
				 source, ret);
			break;
		}
	}
out:
	mutex_unlock(&map_lock);

	if (ret < 0)
		pr_err("IPU: couldn't map source %u: %d\n", source, ret);

	return ret;
}

/**
 * ipu_irq_map() - map an IPU interrupt source to an IRQ number
 * @source:	interrupt source bit position (see ipu_irq_map())
 * @return:	0 or negative error code
 */
int ipu_irq_unmap(unsigned int source)
{
	int i, ret = -EINVAL;

	might_sleep();

	mutex_lock(&map_lock);
	for (i = 0; i < CONFIG_MX3_IPU_IRQS; i++) {
		if (irq_map[i].source == source) {
			unsigned long lock_flags;

			pr_debug("IPU: unmapped source %u from IRQ %u\n",
				 source, irq_map[i].irq);

			raw_spin_lock_irqsave(&bank_lock, lock_flags);
			irq_map[i].source = -EINVAL;
			irq_map[i].bank = NULL;
			raw_spin_unlock_irqrestore(&bank_lock, lock_flags);

			ret = 0;
			break;
		}
	}
	mutex_unlock(&map_lock);

	return ret;
}

/* Chained IRQ handler for IPU function and error interrupt */
static void ipu_irq_handler(struct irq_desc *desc)
{
	struct ipu *ipu = irq_desc_get_handler_data(desc);
	u32 status;
	int i, line;

	for (i = 0; i < IPU_IRQ_NR_BANKS; i++) {
		struct ipu_irq_bank *bank = irq_bank + i;

		raw_spin_lock(&bank_lock);
		status = ipu_read_reg(ipu, bank->status);
		/*
		 * Don't think we have to clear all interrupts here, they will
		 * be acked by ->handle_irq() (handle_level_irq). However, we
		 * might want to clear unhandled interrupts after the loop...
		 */
		status &= ipu_read_reg(ipu, bank->control);
		raw_spin_unlock(&bank_lock);
		while ((line = ffs(status))) {
			struct ipu_irq_map *map;
			unsigned int irq;

			line--;
			status &= ~(1UL << line);

			raw_spin_lock(&bank_lock);
			map = src2map(32 * i + line);
			if (!map) {
				raw_spin_unlock(&bank_lock);
				pr_err("IPU: Interrupt on unmapped source %u bank %d\n",
				       line, i);
				continue;
			}
			irq = map->irq;
			raw_spin_unlock(&bank_lock);
			generic_handle_irq(irq);
		}
	}
}

static struct irq_chip ipu_irq_chip = {
	.name		= "ipu_irq",
	.irq_ack	= ipu_irq_ack,
	.irq_mask	= ipu_irq_mask,
	.irq_unmask	= ipu_irq_unmask,
};

/* Install the IRQ handler */
int __init ipu_irq_attach_irq(struct ipu *ipu, struct platform_device *dev)
{
	unsigned int irq, i;
	int irq_base = irq_alloc_descs(-1, 0, CONFIG_MX3_IPU_IRQS,
				       numa_node_id());

	if (irq_base < 0)
		return irq_base;

	for (i = 0; i < IPU_IRQ_NR_BANKS; i++)
		irq_bank[i].ipu = ipu;

	for (i = 0; i < CONFIG_MX3_IPU_IRQS; i++) {
		int ret;

		irq = irq_base + i;
		ret = irq_set_chip(irq, &ipu_irq_chip);
		if (ret < 0)
			return ret;
		ret = irq_set_chip_data(irq, irq_map + i);
		if (ret < 0)
			return ret;
		irq_map[i].ipu = ipu;
		irq_map[i].irq = irq;
		irq_map[i].source = -EINVAL;
		irq_set_handler(irq, handle_level_irq);
		irq_clear_status_flags(irq, IRQ_NOREQUEST | IRQ_NOPROBE);
	}

	irq_set_chained_handler_and_data(ipu->irq_fn, ipu_irq_handler, ipu);

	irq_set_chained_handler_and_data(ipu->irq_err, ipu_irq_handler, ipu);

	ipu->irq_base = irq_base;

	return 0;
}

void ipu_irq_detach_irq(struct ipu *ipu, struct platform_device *dev)
{
	unsigned int irq, irq_base;

	irq_base = ipu->irq_base;

	irq_set_chained_handler_and_data(ipu->irq_fn, NULL, NULL);

	irq_set_chained_handler_and_data(ipu->irq_err, NULL, NULL);

	for (irq = irq_base; irq < irq_base + CONFIG_MX3_IPU_IRQS; irq++) {
		irq_set_status_flags(irq, IRQ_NOREQUEST);
		irq_set_chip(irq, NULL);
		irq_set_chip_data(irq, NULL);
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Copyright (c) 2013 Linaro Ltd.
 * Copyright (c) 2013 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_dma.h>

#include "virt-dma.h"

#define DRIVER_NAME		"k3-dma"
#define DMA_MAX_SIZE		0x1ffc

#define INT_STAT		0x00
#define INT_TC1			0x04
#define INT_ERR1		0x0c
#define INT_ERR2		0x10
#define INT_TC1_MASK		0x18
#define INT_ERR1_MASK		0x20
#define INT_ERR2_MASK		0x24
#define INT_TC1_RAW		0x600
#define INT_ERR1_RAW		0x608
#define INT_ERR2_RAW		0x610
#define CH_PRI			0x688
#define CH_STAT			0x690
#define CX_CUR_CNT		0x704
#define CX_LLI			0x800
#define CX_CNT			0x810
#define CX_SRC			0x814
#define CX_DST			0x818
#define CX_CFG			0x81c
#define AXI_CFG			0x820
#define AXI_CFG_DEFAULT		0x201201

#define CX_LLI_CHAIN_EN		0x2
#define CX_CFG_EN		0x1
#define CX_CFG_MEM2PER		(0x1 << 2)
#define CX_CFG_PER2MEM		(0x2 << 2)
#define CX_CFG_SRCINCR		(0x1 << 31)
#define CX_CFG_DSTINCR		(0x1 << 30)

struct k3_desc_hw {
	u32 lli;
	u32 reserved[3];
	u32 count;
	u32 saddr;
	u32 daddr;
	u32 config;
} __aligned(32);

struct k3_dma_desc_sw {
	struct virt_dma_desc	vd;
	dma_addr_t		desc_hw_lli;
	size_t			desc_num;
	size_t			size;
	struct k3_desc_hw	desc_hw[0];
};

struct k3_dma_phy;

struct k3_dma_chan {
	u32			ccfg;
	struct virt_dma_chan	vc;
	struct k3_dma_phy	*phy;
	struct list_head	node;
	enum dma_transfer_direction dir;
	dma_addr_t		dev_addr;
	enum dma_status		status;
};

struct k3_dma_phy {
	u32			idx;
	void __iomem		*base;
	struct k3_dma_chan	*vchan;
	struct k3_dma_desc_sw	*ds_run;
	struct k3_dma_desc_sw	*ds_done;
};

struct k3_dma_dev {
	struct dma_device	slave;
	void __iomem		*base;
	struct tasklet_struct	task;
	spinlock_t		lock;
	struct list_head	chan_pending;
	struct k3_dma_phy	*phy;
	struct k3_dma_chan	*chans;
	struct clk		*clk;
	u32			dma_channels;
	u32			dma_requests;
};

#define to_k3_dma(dmadev) container_of(dmadev, struct k3_dma_dev, slave)

static struct k3_dma_chan *to_k3_chan(struct dma_chan *chan)
{
	return container_of(chan, struct k3_dma_chan, vc.chan);
}

static void k3_dma_pause_dma(struct k3_dma_phy *phy, bool on)
{
	u32 val = 0;

	if (on) {
		val = readl_relaxed(phy->base + CX_CFG);
		val |= CX_CFG_EN;
		writel_relaxed(val, phy->base + CX_CFG);
	} else {
		val = readl_relaxed(phy->base + CX_CFG);
		val &= ~CX_CFG_EN;
		writel_relaxed(val, phy->base + CX_CFG);
	}
}

static void k3_dma_terminate_chan(struct k3_dma_phy *phy, struct k3_dma_dev *d)
{
	u32 val = 0;

	k3_dma_pause_dma(phy, false);

	val = 0x1 << phy->idx;
	writel_relaxed(val, d->base + INT_TC1_RAW);
	writel_relaxed(val, d->base + INT_ERR1_RAW);
	writel_relaxed(val, d->base + INT_ERR2_RAW);
}

static void k3_dma_set_desc(struct k3_dma_phy *phy, struct k3_desc_hw *hw)
{
	writel_relaxed(hw->lli, phy->base + CX_LLI);
	writel_relaxed(hw->count, phy->base + CX_CNT);
	writel_relaxed(hw->saddr, phy->base + CX_SRC);
	writel_relaxed(hw->daddr, phy->base + CX_DST);
	writel_relaxed(AXI_CFG_DEFAULT, phy->base + AXI_CFG);
	writel_relaxed(hw->config, phy->base + CX_CFG);
}

static u32 k3_dma_get_curr_cnt(struct k3_dma_dev *d, struct k3_dma_phy *phy)
{
	u32 cnt = 0;

	cnt = readl_relaxed(d->base + CX_CUR_CNT + phy->idx * 0x10);
	cnt &= 0xffff;
	return cnt;
}

static u32 k3_dma_get_curr_lli(struct k3_dma_phy *phy)
{
	return readl_relaxed(phy->base + CX_LLI);
}

static u32 k3_dma_get_chan_stat(struct k3_dma_dev *d)
{
	return readl_relaxed(d->base + CH_STAT);
}

static void k3_dma_enable_dma(struct k3_dma_dev *d, bool on)
{
	if (on) {
		/* set same priority */
		writel_relaxed(0x0, d->base + CH_PRI);

		/* unmask irq */
		writel_relaxed(0xffff, d->base + INT_TC1_MASK);
		writel_relaxed(0xffff, d->base + INT_ERR1_MASK);
		writel_relaxed(0xffff, d->base + INT_ERR2_MASK);
	} else {
		/* mask irq */
		writel_relaxed(0x0, d->base + INT_TC1_MASK);
		writel_relaxed(0x0, d->base + INT_ERR1_MASK);
		writel_relaxed(0x0, d->base + INT_ERR2_MASK);
	}
}

static irqreturn_t k3_dma_int_handler(int irq, void *dev_id)
{
	struct k3_dma_dev *d = (struct k3_dma_dev *)dev_id;
	struct k3_dma_phy *p;
	struct k3_dma_chan *c;
	u32 stat = readl_relaxed(d->base + INT_STAT);
	u32 tc1  = readl_relaxed(d->base + INT_TC1);
	u32 err1 = readl_relaxed(d->base + INT_ERR1);
	u32 err2 = readl_relaxed(d->base + INT_ERR2);
	u32 i, irq_chan = 0;

	while (stat) {
		i = __ffs(stat);
		stat &= (stat - 1);
		if (likely(tc1 & BIT(i))) {
			p = &d->phy[i];
			c = p->vchan;
			if (c) {
				unsigned long flags;

				spin_lock_irqsave(&c->vc.lock, flags);
				vchan_cookie_complete(&p->ds_run->vd);
				p->ds_done = p->ds_run;
				spin_unlock_irqrestore(&c->vc.lock, flags);
			}
			irq_chan |= BIT(i);
		}
		if (unlikely((err1 & BIT(i)) || (err2 & BIT(i))))
			dev_warn(d->slave.dev, "DMA ERR\n");
	}

	writel_relaxed(irq_chan, d->base + INT_TC1_RAW);
	writel_relaxed(err1, d->base + INT_ERR1_RAW);
	writel_relaxed(err2, d->base + INT_ERR2_RAW);

	if (irq_chan) {
		tasklet_schedule(&d->task);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;
}

static int k3_dma_start_txd(struct k3_dma_chan *c)
{
	struct k3_dma_dev *d = to_k3_dma(c->vc.chan.device);
	struct virt_dma_desc *vd = vchan_next_desc(&c->vc);

	if (!c->phy)
		return -EAGAIN;

	if (BIT(c->phy->idx) & k3_dma_get_chan_stat(d))
		return -EAGAIN;

	if (vd) {
		struct k3_dma_desc_sw *ds =
			container_of(vd, struct k3_dma_desc_sw, vd);
		/*
		 * fetch and remove request from vc->desc_issued
		 * so vc->desc_issued only contains desc pending
		 */
		list_del(&ds->vd.node);
		c->phy->ds_run = ds;
		c->phy->ds_done = NULL;
		/* start dma */
		k3_dma_set_desc(c->phy, &ds->desc_hw[0]);
		return 0;
	}
	c->phy->ds_done = NULL;
	c->phy->ds_run = NULL;
	return -EAGAIN;
}

static void k3_dma_tasklet(unsigned long arg)
{
	struct k3_dma_dev *d = (struct k3_dma_dev *)arg;
	struct k3_dma_phy *p;
	struct k3_dma_chan *c, *cn;
	unsigned pch, pch_alloc = 0;

	/* check new dma request of running channel in vc->desc_issued */
	list_for_each_entry_safe(c, cn, &d->slave.channels, vc.chan.device_node) {
		spin_lock_irq(&c->vc.lock);
		p = c->phy;
		if (p && p->ds_done) {
			if (k3_dma_start_txd(c)) {
				/* No current txd associated with this channel */
				dev_dbg(d->slave.dev, "pchan %u: free\n", p->idx);
				/* Mark this channel free */
				c->phy = NULL;
				p->vchan = NULL;
			}
		}
		spin_unlock_irq(&c->vc.lock);
	}

	/* check new channel request in d->chan_pending */
	spin_lock_irq(&d->lock);
	for (pch = 0; pch < d->dma_channels; pch++) {
		p = &d->phy[pch];

		if (p->vchan == NULL && !list_empty(&d->chan_pending)) {
			c = list_first_entry(&d->chan_pending,
				struct k3_dma_chan, node);
			/* remove from d->chan_pending */
			list_del_init(&c->node);
			pch_alloc |= 1 << pch;
			/* Mark this channel allocated */
			p->vchan = c;
			c->phy = p;
			dev_dbg(d->slave.dev, "pchan %u: alloc vchan %p\n", pch, &c->vc);
		}
	}
	spin_unlock_irq(&d->lock);

	for (pch = 0; pch < d->dma_channels; pch++) {
		if (pch_alloc & (1 << pch)) {
			p = &d->phy[pch];
			c = p->vchan;
			if (c) {
				spin_lock_irq(&c->vc.lock);
				k3_dma_start_txd(c);
				spin_unlock_irq(&c->vc.lock);
			}
		}
	}
}

static void k3_dma_free_chan_resources(struct dma_chan *chan)
{
	struct k3_dma_chan *c = to_k3_chan(chan);
	struct k3_dma_dev *d = to_k3_dma(chan->device);
	unsigned long flags;

	spin_lock_irqsave(&d->lock, flags);
	list_del_init(&c->node);
	spin_unlock_irqrestore(&d->lock, flags);

	vchan_free_chan_resources(&c->vc);
	c->ccfg = 0;
}

static enum dma_status k3_dma_tx_status(struct dma_chan *chan,
	dma_cookie_t cookie, struct dma_tx_state *state)
{
	struct k3_dma_chan *c = to_k3_chan(chan);
	struct k3_dma_dev *d = to_k3_dma(chan->device);
	struct k3_dma_phy *p;
	struct virt_dma_desc *vd;
	unsigned long flags;
	enum dma_status ret;
	size_t bytes = 0;

	ret = dma_cookie_status(&c->vc.chan, cookie, state);
	if (ret == DMA_COMPLETE)
		return ret;

	spin_lock_irqsave(&c->vc.lock, flags);
	p = c->phy;
	ret = c->status;

	/*
	 * If the cookie is on our issue queue, then the residue is
	 * its total size.
	 */
	vd = vchan_find_desc(&c->vc, cookie);
	if (vd) {
		bytes = container_of(vd, struct k3_dma_desc_sw, vd)->size;
	} else if ((!p) || (!p->ds_run)) {
		bytes = 0;
	} else {
		struct k3_dma_desc_sw *ds = p->ds_run;
		u32 clli = 0, index = 0;

		bytes = k3_dma_get_curr_cnt(d, p);
		clli = k3_dma_get_curr_lli(p);
		index = (clli - ds->desc_hw_lli) / sizeof(struct k3_desc_hw);
		for (; index < ds->desc_num; index++) {
			bytes += ds->desc_hw[index].count;
			/* end of lli */
			if (!ds->desc_hw[index].lli)
				break;
		}
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);
	dma_set_residue(state, bytes);
	return ret;
}

static void k3_dma_issue_pending(struct dma_chan *chan)
{
	struct k3_dma_chan *c = to_k3_chan(chan);
	struct k3_dma_dev *d = to_k3_dma(chan->device);
	unsigned long flags;

	spin_lock_irqsave(&c->vc.lock, flags);
	/* add request to vc->desc_issued */
	if (vchan_issue_pending(&c->vc)) {
		spin_lock(&d->lock);
		if (!c->phy) {
			if (list_empty(&c->node)) {
				/* if new channel, add chan_pending */
				list_add_tail(&c->node, &d->chan_pending);
				/* check in tasklet */
				tasklet_schedule(&d->task);
				dev_dbg(d->slave.dev, "vchan %p: issued\n", &c->vc);
			}
		}
		spin_unlock(&d->lock);
	} else
		dev_dbg(d->slave.dev, "vchan %p: nothing to issue\n", &c->vc);
	spin_unlock_irqrestore(&c->vc.lock, flags);
}

static void k3_dma_fill_desc(struct k3_dma_desc_sw *ds, dma_addr_t dst,
			dma_addr_t src, size_t len, u32 num, u32 ccfg)
{
	if ((num + 1) < ds->desc_num)
		ds->desc_hw[num].lli = ds->desc_hw_lli + (num + 1) *
			sizeof(struct k3_desc_hw);
	ds->desc_hw[num].lli |= CX_LLI_CHAIN_EN;
	ds->desc_hw[num].count = len;
	ds->desc_hw[num].saddr = src;
	ds->desc_hw[num].daddr = dst;
	ds->desc_hw[num].config = ccfg;
}

static struct dma_async_tx_descriptor *k3_dma_prep_memcpy(
	struct dma_chan *chan,	dma_addr_t dst, dma_addr_t src,
	size_t len, unsigned long flags)
{
	struct k3_dma_chan *c = to_k3_chan(chan);
	struct k3_dma_desc_sw *ds;
	size_t copy = 0;
	int num = 0;

	if (!len)
		return NULL;

	num = DIV_ROUND_UP(len, DMA_MAX_SIZE);
	ds = kzalloc(sizeof(*ds) + num * sizeof(ds->desc_hw[0]), GFP_ATOMIC);
	if (!ds) {
		dev_dbg(chan->device->dev, "vchan %p: kzalloc fail\n", &c->vc);
		return NULL;
	}
	ds->desc_hw_lli = __virt_to_phys((unsigned long)&ds->desc_hw[0]);
	ds->size = len;
	ds->desc_num = num;
	num = 0;

	if (!c->ccfg) {
		/* default is memtomem, without calling device_config */
		c->ccfg = CX_CFG_SRCINCR | CX_CFG_DSTINCR | CX_CFG_EN;
		c->ccfg |= (0xf << 20) | (0xf << 24);	/* burst = 16 */
		c->ccfg |= (0x3 << 12) | (0x3 << 16);	/* width = 64 bit */
	}

	do {
		copy = min_t(size_t, len, DMA_MAX_SIZE);
		k3_dma_fill_desc(ds, dst, src, copy, num++, c->ccfg);

		if (c->dir == DMA_MEM_TO_DEV) {
			src += copy;
		} else if (c->dir == DMA_DEV_TO_MEM) {
			dst += copy;
		} else {
			src += copy;
			dst += copy;
		}
		len -= copy;
	} while (len);

	ds->desc_hw[num-1].lli = 0;	/* end of link */
	return vchan_tx_prep(&c->vc, &ds->vd, flags);
}

static struct dma_async_tx_descriptor *k3_dma_prep_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl, unsigned int sglen,
	enum dma_transfer_direction dir, unsigned long flags, void *context)
{
	struct k3_dma_chan *c = to_k3_chan(chan);
	struct k3_dma_desc_sw *ds;
	size_t len, avail, total = 0;
	struct scatterlist *sg;
	dma_addr_t addr, src = 0, dst = 0;
	int num = sglen, i;

	if (sgl == NULL)
		return NULL;

	for_each_sg(sgl, sg, sglen, i) {
		avail = sg_dma_len(sg);
		if (avail > DMA_MAX_SIZE)
			num += DIV_ROUND_UP(avail, DMA_MAX_SIZE) - 1;
	}

	ds = kzalloc(sizeof(*ds) + num * sizeof(ds->desc_hw[0]), GFP_ATOMIC);
	if (!ds) {
		dev_dbg(chan->device->dev, "vchan %p: kzalloc fail\n", &c->vc);
		return NULL;
	}
	ds->desc_hw_lli = __virt_to_phys((unsigned long)&ds->desc_hw[0]);
	ds->desc_num = num;
	num = 0;

	for_each_sg(sgl, sg, sglen, i) {
		addr = sg_dma_address(sg);
		avail = sg_dma_len(sg);
		total += avail;

		do {
			len = min_t(size_t, avail, DMA_MAX_SIZE);

			if (dir == DMA_MEM_TO_DEV) {
				src = addr;
				dst = c->dev_addr;
			} else if (dir == DMA_DEV_TO_MEM) {
				src = c->dev_addr;
				dst = addr;
			}

			k3_dma_fill_desc(ds, dst, src, len, num++, c->ccfg);

			addr += len;
			avail -= len;
		} while (avail);
	}

	ds->desc_hw[num-1].lli = 0;	/* end of link */
	ds->size = total;
	return vchan_tx_prep(&c->vc, &ds->vd, flags);
}

static int k3_dma_config(struct dma_chan *chan,
			 struct dma_slave_config *cfg)
{
	struct k3_dma_chan *c = to_k3_chan(chan);
	u32 maxburst = 0, val = 0;
	enum dma_slave_buswidth width = DMA_SLAVE_BUSWIDTH_UNDEFINED;

	if (cfg == NULL)
		return -EINVAL;
	c->dir = cfg->direction;
	if (c->dir == DMA_DEV_TO_MEM) {
		c->ccfg = CX_CFG_DSTINCR;
		c->dev_addr = cfg->src_addr;
		maxburst = cfg->src_maxburst;
		width = cfg->src_addr_width;
	} else if (c->dir == DMA_MEM_TO_DEV) {
		c->ccfg = CX_CFG_SRCINCR;
		c->dev_addr = cfg->dst_addr;
		maxburst = cfg->dst_maxburst;
		width = cfg->dst_addr_width;
	}
	switch (width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
	case DMA_SLAVE_BUSWIDTH_8_BYTES:
		val =  __ffs(width);
		break;
	default:
		val = 3;
		break;
	}
	c->ccfg |= (val << 12) | (val << 16);

	if ((maxburst == 0) || (maxburst > 16))
		val = 16;
	else
		val = maxburst - 1;
	c->ccfg |= (val << 20) | (val << 24);
	c->ccfg |= CX_CFG_MEM2PER | CX_CFG_EN;

	/* specific request line */
	c->ccfg |= c->vc.chan.chan_id << 4;

	return 0;
}

static int k3_dma_terminate_all(struct dma_chan *chan)
{
	struct k3_dma_chan *c = to_k3_chan(chan);
	struct k3_dma_dev *d = to_k3_dma(chan->device);
	struct k3_dma_phy *p = c->phy;
	unsigned long flags;
	LIST_HEAD(head);

	dev_dbg(d->slave.dev, "vchan %p: terminate all\n", &c->vc);

	/* Prevent this channel being scheduled */
	spin_lock(&d->lock);
	list_del_init(&c->node);
	spin_unlock(&d->lock);

	/* Clear the tx descriptor lists */
	spin_lock_irqsave(&c->vc.lock, flags);
	vchan_get_all_descriptors(&c->vc, &head);
	if (p) {
		/* vchan is assigned to a pchan - stop the channel */
		k3_dma_terminate_chan(p, d);
		c->phy = NULL;
		p->vchan = NULL;
		p->ds_run = p->ds_done = NULL;
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);
	vchan_dma_desc_free_list(&c->vc, &head);

	return 0;
}

static int k3_dma_transfer_p