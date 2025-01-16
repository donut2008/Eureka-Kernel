/*
 * Ingenic JZ4780 DMA controller
 *
 * Copyright (c) 2015 Imagination Technologies
 * Author: Alex Smith <alex@alex-smith.me.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/clk.h>
#include <linux/dmapool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "dmaengine.h"
#include "virt-dma.h"

#define JZ_DMA_NR_CHANNELS	32

/* Global registers. */
#define JZ_DMA_REG_DMAC		0x1000
#define JZ_DMA_REG_DIRQP	0x1004
#define JZ_DMA_REG_DDR		0x1008
#define JZ_DMA_REG_DDRS		0x100c
#define JZ_DMA_REG_DMACP	0x101c
#define JZ_DMA_REG_DSIRQP	0x1020
#define JZ_DMA_REG_DSIRQM	0x1024
#define JZ_DMA_REG_DCIRQP	0x1028
#define JZ_DMA_REG_DCIRQM	0x102c

/* Per-channel registers. */
#define JZ_DMA_REG_CHAN(n)	(n * 0x20)
#define JZ_DMA_REG_DSA(n)	(0x00 + JZ_DMA_REG_CHAN(n))
#define JZ_DMA_REG_DTA(n)	(0x04 + JZ_DMA_REG_CHAN(n))
#define JZ_DMA_REG_DTC(n)	(0x08 + JZ_DMA_REG_CHAN(n))
#define JZ_DMA_REG_DRT(n)	(0x0c + JZ_DMA_REG_CHAN(n))
#define JZ_DMA_REG_DCS(n)	(0x10 + JZ_DMA_REG_CHAN(n))
#define JZ_DMA_REG_DCM(n)	(0x14 + JZ_DMA_REG_CHAN(n))
#define JZ_DMA_REG_DDA(n)	(0x18 + JZ_DMA_REG_CHAN(n))
#define JZ_DMA_REG_DSD(n)	(0x1c + JZ_DMA_REG_CHAN(n))

#define JZ_DMA_DMAC_DMAE	BIT(0)
#define JZ_DMA_DMAC_AR		BIT(2)
#define JZ_DMA_DMAC_HLT		BIT(3)
#define JZ_DMA_DMAC_FMSC	BIT(31)

#define JZ_DMA_DRT_AUTO		0x8

#define JZ_DMA_DCS_CTE		BIT(0)
#define JZ_DMA_DCS_HLT		BIT(2)
#define JZ_DMA_DCS_TT		BIT(3)
#define JZ_DMA_DCS_AR		BIT(4)
#define JZ_DMA_DCS_DES8		BIT(30)

#define JZ_DMA_DCM_LINK		BIT(0)
#define JZ_DMA_DCM_TIE		BIT(1)
#define JZ_DMA_DCM_STDE		BIT(2)
#define JZ_DMA_DCM_TSZ_SHIFT	8
#define JZ_DMA_DCM_TSZ_MASK	(0x7 << JZ_DMA_DCM_TSZ_SHIFT)
#define JZ_DMA_DCM_DP_SHIFT	12
#define JZ_DMA_DCM_SP_SHIFT	14
#define JZ_DMA_DCM_DAI		BIT(22)
#define JZ_DMA_DCM_SAI		BIT(23)

#define JZ_DMA_SIZE_4_BYTE	0x0
#define JZ_DMA_SIZE_1_BYTE	0x1
#define JZ_DMA_SIZE_2_BYTE	0x2
#define JZ_DMA_SIZE_16_BYTE	0x3
#define JZ_DMA_SIZE_32_BYTE	0x4
#define JZ_DMA_SIZE_64_BYTE	0x5
#define JZ_DMA_SIZE_128_BYTE	0x6

#define JZ_DMA_WIDTH_32_BIT	0x0
#define JZ_DMA_WIDTH_8_BIT	0x1
#define JZ_DMA_WIDTH_16_BIT	0x2

#define JZ_DMA_BUSWIDTHS	(BIT(DMA_SLAVE_BUSWIDTH_1_BYTE)	 | \
				 BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) | \
				 BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))

/**
 * struct jz4780_dma_hwdesc - descriptor structure read by the DMA controller.
 * @dcm: value for the DCM (channel command) register
 * @dsa: source address
 * @dta: target address
 * @dtc: transfer count (number of blocks of the transfer size specified in DCM
 * to transfer) in the low 24 bits, offset of the next descriptor from the
 * descriptor base address in the upper 8 bits.
 * @sd: target/source stride difference (in stride transfer mode).
 * @drt: request type
 */
struct jz4780_dma_hwdesc {
	uint32_t dcm;
	uint32_t dsa;
	uint32_t dta;
	uint32_t dtc;
	uint32_t sd;
	uint32_t drt;
	uint32_t reserved[2];
};

/* Size of allocations for hardware descriptor blocks. */
#define JZ_DMA_DESC_BLOCK_SIZE	PAGE_SIZE
#define JZ_DMA_MAX_DESC		\
	(JZ_DMA_DESC_BLOCK_SIZE / sizeof(struct jz4780_dma_hwdesc))

struct jz4780_dma_desc {
	struct virt_dma_desc vdesc;

	struct jz4780_dma_hwdesc *desc;
	dma_addr_t desc_phys;
	unsigned int count;
	enum dma_transaction_type type;
	uint32_t status;
};

struct jz4780_dma_chan {
	struct virt_dma_chan vchan;
	unsigned int id;
	struct dma_pool *desc_pool;

	uint32_t transfer_type;
	uint32_t transfer_shift;
	struct dma_slave_config	config;

	struct jz4780_dma_desc *desc;
	unsigned int curr_hwdesc;
};

struct jz4780_dma_dev {
	struct dma_device dma_device;
	void __iomem *base;
	struct clk *clk;
	unsigned int irq;

	uint32_t chan_reserved;
	struct jz4780_dma_chan chan[JZ_DMA_NR_CHANNELS];
};

struct jz4780_dma_filter_data {
	struct device_node *of_node;
	uint32_t transfer_type;
	int channel;
};

static inline struct jz4780_dma_chan *to_jz4780_dma_chan(struct dma_chan *chan)
{
	return container_of(chan, struct jz4780_dma_chan, vchan.chan);
}

static inline struct jz4780_dma_desc *to_jz4780_dma_desc(
	struct virt_dma_desc *vdesc)
{
	return container_of(vdesc, struct jz4780_dma_desc, vdesc);
}

static inline struct jz4780_dma_dev *jz4780_dma_chan_parent(
	struct jz4780_dma_chan *jzchan)
{
	return container_of(jzchan->vchan.chan.device, struct jz4780_dma_dev,
			    dma_device);
}

static inline uint32_t jz4780_dma_readl(struct jz4780_dma_dev *jzdma,
	unsigned int reg)
{
	return readl(jzdma->base + reg);
}

static inline void jz4780_dma_writel(struct jz4780_dma_dev *jzdma,
	unsigned int reg, uint32_t val)
{
	writel(val, jzdma->base + reg);
}

static struct jz4780_dma_desc *jz4780_dma_desc_alloc(
	struct jz4780_dma_chan *jzchan, unsigned int count,
	enum dma_transaction_type type)
{
	struct jz4780_dma_desc *desc;

	if (count > JZ_DMA_MAX_DESC)
		return NULL;

	desc = kzalloc(sizeof(*desc), GFP_NOWAIT);
	if (!desc)
		return NULL;

	desc->desc = dma_pool_alloc(jzchan->desc_pool, GFP_NOWAIT,
				    &desc->desc_phys);
	if (!desc->desc) {
		kfree(desc);
		return NULL;
	}

	desc->count = count;
	desc->type = type;
	return desc;
}

static void jz4780_dma_desc_free(struct virt_dma_desc *vdesc)
{
	struct jz4780_dma_desc *desc = to_jz4780_dma_desc(vdesc);
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(vdesc->tx.chan);

	dma_pool_free(jzchan->desc_pool, desc->desc, desc->desc_phys);
	kfree(desc);
}

static uint32_t jz4780_dma_transfer_size(unsigned long val, uint32_t *shift)
{
	int ord = ffs(val) - 1;

	/*
	 * 8 byte transfer sizes unsupported so fall back on 4. If it's larger
	 * than the maximum, just limit it. It is perfectly safe to fall back
	 * in this way since we won't exceed the maximum burst size supported
	 * by the device, the only effect is reduced efficiency. This is better
	 * than refusing to perform the request at all.
	 */
	if (ord == 3)
		ord = 2;
	else if (ord > 7)
		ord = 7;

	*shift = ord;

	switch (ord) {
	case 0:
		return JZ_DMA_SIZE_1_BYTE;
	case 1:
		return JZ_DMA_SIZE_2_BYTE;
	case 2:
		return JZ_DMA_SIZE_4_BYTE;
	case 4:
		return JZ_DMA_SIZE_16_BYTE;
	case 5:
		return JZ_DMA_SIZE_32_BYTE;
	case 6:
		return JZ_DMA_SIZE_64_BYTE;
	default:
		return JZ_DMA_SIZE_128_BYTE;
	}
}

static int jz4780_dma_setup_hwdesc(struct jz4780_dma_chan *jzchan,
	struct jz4780_dma_hwdesc *desc, dma_addr_t addr, size_t len,
	enum dma_transfer_direction direction)
{
	struct dma_slave_config *config = &jzchan->config;
	uint32_t width, maxburst, tsz;

	if (direction == DMA_MEM_TO_DEV) {
		desc->dcm = JZ_DMA_DCM_SAI;
		desc->dsa = addr;
		desc->dta = config->dst_addr;
		desc->drt = jzchan->transfer_type;

		width = config->dst_addr_width;
		maxburst = config->dst_maxburst;
	} else {
		desc->dcm = JZ_DMA_DCM_DAI;
		desc->dsa = config->src_addr;
		desc->dta = addr;
		desc->drt = jzchan->transfer_type;

		width = config->src_addr_width;
		maxburst = config->src_maxburst;
	}

	/*
	 * This calculates the maximum transfer size that can be used with the
	 * given address, length, width and maximum burst size. The address
	 * must be aligned to the transfer size, the total length must be
	 * divisible by the transfer size, and we must not use more than the
	 * maximum burst specified by the user.
	 */
	tsz = jz4780_dma_transfer_size(addr | len | (width * maxburst),
				       &jzchan->transfer_shift);

	switch (width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		width = JZ_DMA_WIDTH_32_BIT;
		break;
	default:
		return -EINVAL;
	}

	desc->dcm |= tsz << JZ_DMA_DCM_TSZ_SHIFT;
	desc->dcm |= width << JZ_DMA_DCM_SP_SHIFT;
	desc->dcm |= width << JZ_DMA_DCM_DP_SHIFT;

	desc->dtc = len >> jzchan->transfer_shift;
	return 0;
}

static struct dma_async_tx_descriptor *jz4780_dma_prep_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl, unsigned int sg_len,
	enum dma_transfer_direction direction, unsigned long flags,
	void *context)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);
	struct jz4780_dma_desc *desc;
	unsigned int i;
	int err;

	desc = jz4780_dma_desc_alloc(jzchan, sg_len, DMA_SLAVE);
	if (!desc)
		return NULL;

	for (i = 0; i < sg_len; i++) {
		err = jz4780_dma_setup_hwdesc(jzchan, &desc->desc[i],
					      sg_dma_address(&sgl[i]),
					      sg_dma_len(&sgl[i]),
					      direction);
		if (err < 0)
			return NULL;

		desc->desc[i].dcm |= JZ_DMA_DCM_TIE;

		if (i != (sg_len - 1)) {
			/* Automatically proceeed to the next descriptor. */
			desc->desc[i].dcm |= JZ_DMA_DCM_LINK;

			/*
			 * The upper 8 bits of the DTC field in the descriptor
			 * must be set to (offset from descriptor base of next
			 * descriptor >> 4).
			 */
			desc->desc[i].dtc |=
				(((i + 1) * sizeof(*desc->desc)) >> 4) << 24;
		}
	}

	return vchan_tx_prep(&jzchan->vchan, &desc->vdesc, flags);
}

static struct dma_async_tx_descriptor *jz4780_dma_prep_dma_cyclic(
	struct dma_chan *chan, dma_addr_t buf_addr, size_t buf_len,
	size_t period_len, enum dma_transfer_direction direction,
	unsigned long flags)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);
	struct jz4780_dma_desc *desc;
	unsigned int periods, i;
	int err;

	if (buf_len % period_len)
		return NULL;

	periods = buf_len / period_len;

	desc = jz4780_dma_desc_alloc(jzchan, periods, DMA_CYCLIC);
	if (!desc)
		return NULL;

	for (i = 0; i < periods; i++) {
		err = jz4780_dma_setup_hwdesc(jzchan, &desc->desc[i], buf_addr,
					      period_len, direction);
		if (err < 0)
			return NULL;

		buf_addr += period_len;

		/*
		 * Set the link bit to indicate that the controller should
		 * automatically proceed to the next descriptor. In
		 * jz4780_dma_begin(), this will be cleared if we need to issue
		 * an interrupt after each period.
		 */
		desc->desc[i].dcm |= JZ_DMA_DCM_TIE | JZ_DMA_DCM_LINK;

		/*
		 * The upper 8 bits of the DTC field in the descriptor must be
		 * set to (offset from descriptor base of next descriptor >> 4).
		 * If this is the last descriptor, link it back to the first,
		 * i.e. leave offset set to 0, otherwise point to the next one.
		 */
		if (i != (periods - 1)) {
			desc->desc[i].dtc |=
				(((i + 1) * sizeof(*desc->desc)) >> 4) << 24;
		}
	}

	return vchan_tx_prep(&jzchan->vchan, &desc->vdesc, flags);
}

struct dma_async_tx_descriptor *jz4780_dma_prep_dma_memcpy(
	struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
	size_t len, unsigned long flags)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);
	struct jz4780_dma_desc *desc;
	uint32_t tsz;

	desc = jz4780_dma_desc_alloc(jzchan, 1, DMA_MEMCPY);
	if (!desc)
		return NULL;

	tsz = jz4780_dma_transfer_size(dest | src | len,
				       &jzchan->transfer_shift);

	desc->desc[0].dsa = src;
	desc->desc[0].dta = dest;
	desc->desc[0].drt = JZ_DMA_DRT_AUTO;
	desc->desc[0].dcm = JZ_DMA_DCM_TIE | JZ_DMA_DCM_SAI | JZ_DMA_DCM_DAI |
			    tsz << JZ_DMA_DCM_TSZ_SHIFT |
			    JZ_DMA_WIDTH_32_BIT << JZ_DMA_DCM_SP_SHIFT |
			    JZ_DMA_WIDTH_32_BIT << JZ_DMA_DCM_DP_SHIFT;
	desc->desc[0].dtc = len >> jzchan->transfer_shift;

	return vchan_tx_prep(&jzchan->vchan, &desc->vdesc, flags);
}

static void jz4780_dma_begin(struct jz4780_dma_chan *jzchan)
{
	struct jz4780_dma_dev *jzdma = jz4780_dma_chan_parent(jzchan);
	struct virt_dma_desc *vdesc;
	unsigned int i;
	dma_addr_t desc_phys;

	if (!jzchan->desc) {
		vdesc = vchan_next_desc(&jzchan->vchan);
		if (!vdesc)
			return;

		list_del(&vdesc->node);

		jzchan->desc = to_jz4780_dma_desc(vdesc);
		jzchan->curr_hwdesc = 0;

		if (jzchan->desc->type == DMA_CYCLIC && vdesc->tx.callback) {
			/*
			 * The DMA controller doesn't support triggering an
			 * interrupt after processing each descriptor, only
			 * after processing an entire terminated list of
			 * descriptors. For a cyclic DMA setup the list of
			 * descriptors is not terminated so we can never get an
			 * interrupt.
			 *
			 * If the user requested a callback for a cyclic DMA
			 * setup then we workaround this hardware limitation
			 * here by degrading to a set of unlinked descriptors
			 * which we will submit in sequence in response to the
			 * completion of processing the previous descriptor.
			 */
			for (i = 0; i < jzchan->desc->count; i++)
				jzchan->desc->desc[i].dcm &= ~JZ_DMA_DCM_LINK;
		}
	} else {
		/*
		 * There is an existing transfer, therefore this must be one
		 * for which we unlinked the descriptors above. Advance to the
		 * next one in the list.
		 */
		jzchan->curr_hwdesc =
			(jzchan->curr_hwdesc + 1) % jzchan->desc->count;
	}

	/* Use 8-word descriptors. */
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DCS(jzchan->id), JZ_DMA_DCS_DES8);

	/* Write descriptor address and initiate descriptor fetch. */
	desc_phys = jzchan->desc->desc_phys +
		    (jzchan->curr_hwdesc * sizeof(*jzchan->desc->desc));
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DDA(jzchan->id), desc_phys);
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DDRS, BIT(jzchan->id));

	/* Enable the channel. */
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DCS(jzchan->id),
			  JZ_DMA_DCS_DES8 | JZ_DMA_DCS_CTE);
}

static void jz4780_dma_issue_pending(struct dma_chan *chan)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&jzchan->vchan.lock, flags);

	if (vchan_issue_pending(&jzchan->vchan) && !jzchan->desc)
		jz4780_dma_begin(jzchan);

	spin_unlock_irqrestore(&jzchan->vchan.lock, flags);
}

static int jz4780_dma_terminate_all(struct dma_chan *chan)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);
	struct jz4780_dma_dev *jzdma = jz4780_dma_chan_parent(jzchan);
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&jzchan->vchan.lock, flags);

	/* Clear the DMA status and stop the transfer. */
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DCS(jzchan->id), 0);
	if (jzchan->desc) {
		jz4780_dma_desc_free(&jzchan->desc->vdesc);
		jzchan->desc = NULL;
	}

	vchan_get_all_descriptors(&jzchan->vchan, &head);

	spin_unlock_irqrestore(&jzchan->vchan.lock, flags);

	vchan_dma_desc_free_list(&jzchan->vchan, &head);
	return 0;
}

static int jz4780_dma_config(struct dma_chan *chan,
	struct dma_slave_config *config)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);

	if ((config->src_addr_width == DMA_SLAVE_BUSWIDTH_8_BYTES)
	   || (config->dst_addr_width == DMA_SLAVE_BUSWIDTH_8_BYTES))
		return -EINVAL;

	/* Copy the reset of the slave configuration, it is used later. */
	memcpy(&jzchan->config, config, sizeof(jzchan->config));

	return 0;
}

static size_t jz4780_dma_desc_residue(struct jz4780_dma_chan *jzchan,
	struct jz4780_dma_desc *desc, unsigned int next_sg)
{
	struct jz4780_dma_dev *jzdma = jz4780_dma_chan_parent(jzchan);
	unsigned int residue, count;
	unsigned int i;

	residue = 0;

	for (i = next_sg; i < desc->count; i++)
		residue += desc->desc[i].dtc << jzchan->transfer_shift;

	if (next_sg != 0) {
		count = jz4780_dma_readl(jzdma,
					 JZ_DMA_REG_DTC(jzchan->id));
		residue += count << jzchan->transfer_shift;
	}

	return residue;
}

static enum dma_status jz4780_dma_tx_status(struct dma_chan *chan,
	dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);
	struct virt_dma_desc *vdesc;
	enum dma_status status;
	unsigned long flags;

	spin_lock_irqsave(&jzchan->vchan.lock, flags);

	status = dma_cookie_status(chan, cookie, txstate);
	if ((status == DMA_COMPLETE) || (txstate == NULL))
		goto out_unlock_irqrestore;

	vdesc = vchan_find_desc(&jzchan->vchan, cookie);
	if (vdesc) {
		/* On the issued list, so hasn't been processed yet */
		txstate->residue = jz4780_dma_desc_residue(jzchan,
					to_jz4780_dma_desc(vdesc), 0);
	} else if (cookie == jzchan->desc->vdesc.tx.cookie) {
		txstate->residue = jz4780_dma_desc_residue(jzchan, jzchan->desc,
					jzchan->curr_hwdesc + 1);
	} else
		txstate->residue = 0;

	if (vdesc && jzchan->desc && vdesc == &jzchan->desc->vdesc
	    && jzchan->desc->status & (JZ_DMA_DCS_AR | JZ_DMA_DCS_HLT))
		status = DMA_ERROR;

out_unlock_irqrestore:
	spin_unlock_irqrestore(&jzchan->vchan.lock, flags);
	return status;
}

static void jz4780_dma_chan_irq(struct jz4780_dma_dev *jzdma,
	struct jz4780_dma_chan *jzchan)
{
	uint32_t dcs;

	spin_lock(&jzchan->vchan.lock);

	dcs = jz4780_dma_readl(jzdma, JZ_DMA_REG_DCS(jzchan->id));
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DCS(jzchan->id), 0);

	if (dcs & JZ_DMA_DCS_AR) {
		dev_warn(&jzchan->vchan.chan.dev->device,
			 "address error (DCS=0x%x)\n", dcs);
	}

	if (dcs & JZ_DMA_DCS_HLT) {
		dev_warn(&jzchan->vchan.chan.dev->device,
			 "channel halt (DCS=0x%x)\n", dcs);
	}

	if (jzchan->desc) {
		jzchan->desc->status = dcs;

		if ((dcs & (JZ_DMA_DCS_AR | JZ_DMA_DCS_HLT)) == 0) {
			if (jzchan->desc->type == DMA_CYCLIC) {
				vchan_cyclic_callback(&jzchan->desc->vdesc);
			} else {
				vchan_cookie_complete(&jzchan->desc->vdesc);
				jzchan->desc = NULL;
			}

			jz4780_dma_begin(jzchan);
		}
	} else {
		dev_err(&jzchan->vchan.chan.dev->device,
			"channel IRQ with no active transfer\n");
	}

	spin_unlock(&jzchan->vchan.lock);
}

static irqreturn_t jz4780_dma_irq_handler(int irq, void *data)
{
	struct jz4780_dma_dev *jzdma = data;
	uint32_t pending, dmac;
	int i;

	pending = jz4780_dma_readl(jzdma, JZ_DMA_REG_DIRQP);

	for (i = 0; i < JZ_DMA_NR_CHANNELS; i++) {
		if (!(pending & (1<<i)))
			continue;

		jz4780_dma_chan_irq(jzdma, &jzdma->chan[i]);
	}

	/* Clear halt and address error status of all channels. */
	dmac = jz4780_dma_readl(jzdma, JZ_DMA_REG_DMAC);
	dmac &= ~(JZ_DMA_DMAC_HLT | JZ_DMA_DMAC_AR);
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DMAC, dmac);

	/* Clear interrupt pending status. */
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DIRQP, 0);

	return IRQ_HANDLED;
}

static int jz4780_dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);

	jzchan->desc_pool = dma_pool_create(dev_name(&chan->dev->device),
					    chan->device->dev,
					    JZ_DMA_DESC_BLOCK_SIZE,
					    PAGE_SIZE, 0);
	if (!jzchan->desc_pool) {
		dev_err(&chan->dev->device,
			"failed to allocate descriptor pool\n");
		return -ENOMEM;
	}

	return 0;
}

static void jz4780_dma_free_chan_resources(struct dma_chan *chan)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);

	vchan_free_chan_resources(&jzchan->vchan);
	dma_pool_destroy(jzchan->desc_pool);
	jzchan->desc_pool = NULL;
}

static bool jz4780_dma_filter_fn(struct dma_chan *chan, void *param)
{
	struct jz4780_dma_chan *jzchan = to_jz4780_dma_chan(chan);
	struct jz4780_dma_dev *jzdma = jz4780_dma_chan_parent(jzchan);
	struct jz4780_dma_filter_data *data = param;

	if (jzdma->dma_device.dev->of_node != data->of_node)
		return false;

	if (data->channel > -1) {
		if (data->channel != jzchan->id)
			return false;
	} else if (jzdma->chan_reserved & BIT(jzchan->id)) {
		return false;
	}

	jzchan->transfer_type = data->transfer_type;

	return true;
}

static struct dma_chan *jz4780_of_dma_xlate(struct of_phandle_args *dma_spec,
	struct of_dma *ofdma)
{
	struct jz4780_dma_dev *jzdma = ofdma->of_dma_data;
	dma_cap_mask_t mask = jzdma->dma_device.cap_mask;
	struct jz4780_dma_filter_data data;

	if (dma_spec->args_count != 2)
		return NULL;

	data.of_node = ofdma->of_node;
	data.transfer_type = dma_spec->args[0];
	data.channel = dma_spec->args[1];

	if (data.channel > -1) {
		if (data.channel >= JZ_DMA_NR_CHANNELS) {
			dev_err(jzdma->dma_device.dev,
				"device requested non-existent channel %u\n",
				data.channel);
			return NULL;
		}

		/* Can only select a channel marked as reserved. */
		if (!(jzdma->chan_reserved & BIT(data.channel))) {
			dev_err(jzdma->dma_device.dev,
				"device requested unreserved channel %u\n",
				data.channel);
			return NULL;
		}

		jzdma->chan[data.channel].transfer_type = data.transfer_type;

		return dma_get_slave_channel(
			&jzdma->chan[data.channel].vchan.chan);
	} else {
		return dma_request_channel(mask, jz4780_dma_filter_fn, &data);
	}
}

static int jz4780_dma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct jz4780_dma_dev *jzdma;
	struct jz4780_dma_chan *jzchan;
	struct dma_device *dd;
	struct resource *res;
	int i, ret;

	if (!dev->of_node) {
		dev_err(dev, "This driver must be probed from devicetree\n");
		return -EINVAL;
	}

	jzdma = devm_kzalloc(dev, sizeof(*jzdma), GFP_KERNEL);
	if (!jzdma)
		return -ENOMEM;

	platform_set_drvdata(pdev, jzdma);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "failed to get I/O memory\n");
		return -EINVAL;
	}

	jzdma->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(jzdma->base))
		return PTR_ERR(jzdma->base);

	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(dev, "failed to get IRQ: %d\n", ret);
		return ret;
	}

	jzdma->irq = ret;

	ret = request_irq(jzdma->irq, jz4780_dma_irq_handler, 0, dev_name(dev),
			  jzdma);
	if (ret) {
		dev_err(dev, "failed to request IRQ %u!\n", jzdma->irq);
		return ret;
	}

	jzdma->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(jzdma->clk)) {
		dev_err(dev, "failed to get clock\n");
		ret = PTR_ERR(jzdma->clk);
		goto err_free_irq;
	}

	clk_prepare_enable(jzdma->clk);

	/* Property is optional, if it doesn't exist the value will remain 0. */
	of_property_read_u32_index(dev->of_node, "ingenic,reserved-channels",
				   0, &jzdma->chan_reserved);

	dd = &jzdma->dma_device;

	dma_cap_set(DMA_MEMCPY, dd->cap_mask);
	dma_cap_set(DMA_SLAVE, dd->cap_mask);
	dma_cap_set(DMA_CYCLIC, dd->cap_mask);

	dd->dev = dev;
	dd->copy_align = DMAENGINE_ALIGN_4_BYTES;
	dd->device_alloc_chan_resources = jz4780_dma_alloc_chan_resources;
	dd->device_free_chan_resources = jz4780_dma_free_chan_resources;
	dd->device_prep_slave_sg = jz4780_dma_prep_slave_sg;
	dd->device_prep_dma_cyclic = jz4780_dma_prep_dma_cyclic;
	dd->device_prep_dma_memcpy = jz4780_dma_prep_dma_memcpy;
	dd->device_config = jz4780_dma_config;
	dd->device_terminate_all = jz4780_dma_terminate_all;
	dd->device_tx_status = jz4780_dma_tx_status;
	dd->device_issue_pending = jz4780_dma_issue_pending;
	dd->src_addr_widths = JZ_DMA_BUSWIDTHS;
	dd->dst_addr_widths = JZ_DMA_BUSWIDTHS;
	dd->directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	dd->residue_granularity = DMA_RESIDUE_GRANULARITY_BURST;

	/*
	 * Enable DMA controller, mark all channels as not programmable.
	 * Also set the FMSC bit - it increases MSC performance, so it makes
	 * little sense not to enable it.
	 */
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DMAC,
			  JZ_DMA_DMAC_DMAE | JZ_DMA_DMAC_FMSC);
	jz4780_dma_writel(jzdma, JZ_DMA_REG_DMACP, 0);

	INIT_LIST_HEAD(&dd->channels);

	for (i = 0; i < JZ_DMA_NR_CHANNELS; i++) {
		jzchan = &jzdma->chan[i];
		jzchan->id = i;

		vchan_init(&jzchan->vchan, dd);
		jzchan->vchan.desc_free = jz4780_dma_desc_free;
	}

	ret = dma_async_device_register(dd);
	if (ret) {
		dev_err(dev, "failed to register device\n");
		goto err_disable_clk;
	}

	/* Register with OF DMA helpers. */
	ret = of_dma_controller_register(dev->of_node, jz4780_of_dma_xlate,
					 jzdma);
	if (ret) {
		dev_err(dev, "failed to register OF DMA controller\n");
		goto err_unregister_dev;
	}

	dev_info(dev, "JZ4780 DMA controller initialised\n");
	return 0;

err_unregister_dev:
	dma_async_device_unregister(dd);

err_disable_clk:
	clk_disable_unprepare(jzdma->clk);

err_free_irq:
	free_irq(jzdma->irq, jzdma);
	return ret;
}

static int jz4780_dma_remove(struct platform_device *pdev)
{
	struct jz4780_dma_dev *jzdma = platform_get_drvdata(pdev);
	int i;

	of_dma_controller_free(pdev->dev.of_node);

	free_irq(jzdma->irq, jzdma);

	for (i = 0; i < JZ_DMA_NR_CHANNELS; i++)
		tasklet_kill(&jzdma->chan[i].vchan.task);

	dma_async_device_unregister(&jzdma->dma_device);
	return 0;
}

static const struct of_device_id jz4780_dma_dt_match[] = {
	{ .compatible = "ingenic,jz4780-dma", .data = NULL },
	{},
};
MODULE_DEVICE_TABLE(of, jz4780_dma_dt_match);

static struct platform_driver jz4780_dma_driver = {
	.probe		= jz4780_dma_probe,
	.remove		= jz4780_dma_remove,
	.driver	= {
		.name	= "jz4780-dma",
		.of_match_table = of_match_ptr(jz4780_dma_dt_match),
	},
};

static int __init jz4780_dma_init(void)
{
	return platform_driver_register(&jz4780_dma_driver);
}
subsys_initcall(jz4780_dma_init);

static void __exit jz4780_dma_exit(void)
{
	platform_driver_unregister(&jz4780_dma_driver);
}
module_exit(jz4780_dma_exit);

MODULE_AUTHOR("Alex Smith <alex@alex-smith.me.uk>");
MODULE_DESCRIPTION("Ingenic JZ4780 DMA controller driver");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright(c) 2004 - 2006 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called COPYING.
 */

/*
 * This code implements the DMA subsystem. It provides a HW-neutral interface
 * for other kernel code to use asynchronous memory copy capabilities,
 * if present, and allows different HW DMA drivers to register as providing
 * this capability.
 *
 * Due to the fact we are accelerating what is already a relatively fast
 * operation, the code goes to great lengths to avoid additional overhead,
 * such as locking.
 *
 * LOCKING:
 *
 * The subsystem keeps a global list of dma_device structs it is protected by a
 * mutex, dma_list_mutex.
 *
 * A subsystem can get access to a channel by calling dmaengine_get() followed
 * by dma_find_channel(), or if it has need for an exclusive channel it can call
 * dma_request_channel().  Once a channel is allocated a reference is taken
 * against its corresponding driver to disable removal.
 *
 * Each device has a channels list, which runs unlocked but is never modified
 * once the device is registered, it's just setup by the driver.
 *
 * See Documentation/dmaengine.txt for more details
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/hardirq.h>
#include <linux/spinlock.h>
#include <linux/percpu.h>
#include <linux/rcupdate.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/rculist.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/acpi_dma.h>
#include <linux/of_dma.h>
#include <linux/mempool.h>

static DEFINE_MUTEX(dma_list_mutex);
static DEFINE_IDR(dma_idr);
static LIST_HEAD(dma_device_list);
static long dmaengine_ref_count;

/* --- sysfs implementation --- */

/**
 * dev_to_dma_chan - convert a device pointer to the its sysfs container object
 * @dev - device node
 *
 * Must be called under dma_list_mutex
 */
static struct dma_chan *dev_to_dma_chan(struct device *dev)
{
	struct dma_chan_dev *chan_dev;

	chan_dev = container_of(dev, typeof(*chan_dev), device);
	return chan_dev->chan;
}

static ssize_t memcpy_count_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct dma_chan *chan;
	unsigned long count = 0;
	int i;
	int err;

	mutex_lock(&dma_list_mutex);
	chan = dev_to_dma_chan(dev);
	if (chan) {
		for_each_possible_cpu(i)
			count += per_cpu_ptr(chan->local, i)->memcpy_count;
		err = sprintf(buf, "%lu\n", count);
	} else
		err = -ENODEV;
	mutex_unlock(&dma_list_mutex);

	return err;
}
static DEVICE_ATTR_RO(memcpy_count);

static ssize_t bytes_transferred_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct dma_chan *chan;
	unsigned long count = 0;
	int i;
	int err;

	mutex_lock(&dma_list_mutex);
	chan = dev_to_dma_chan(dev);
	if (chan) {
		for_each_possible_cpu(i)
			count += per_cpu_ptr(chan->local, i)->bytes_transferred;
		err = sprintf(buf, "%lu\n", count);
	} else
		err = -ENODEV;
	mutex_unlock(&dma_list_mutex);

	return err;
}
static DEVICE_ATTR_RO(bytes_transferred);

static ssize_t in_use_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct dma_chan *chan;
	int err;

	mutex_lock(&dma_list_mutex);
	chan = dev_to_dma_chan(dev);
	if (chan)
		err = sprintf(buf, "%d\n", chan->client_count);
	else
		err = -ENODEV;
	mutex_unlock(&dma_list_mutex);

	return err;
}
static DEVICE_ATTR_RO(in_use);

static struct attribute *dma_dev_attrs[] = {
	&dev_attr_memcpy_count.attr,
	&dev_attr_bytes_transferred.attr,
	&dev_attr_in_use.attr,
	NULL,
};
ATTRIBUTE_GROUPS(dma_dev);

static void chan_dev_release(struct device *dev)
{
	struct dma_chan_dev *chan_dev;

	chan_dev = container_of(dev, typeof(*chan_dev), device);
	if (atomic_dec_and_test(chan_dev->idr_ref)) {
		mutex_lock(&dma_list_mutex);
		idr_remove(&dma_idr, chan_dev->dev_id);
		mutex_unlock(&dma_list_mutex);
		kfree(chan_dev->idr_ref);
	}
	kfree(chan_dev);
}

static struct class dma_devclass = {
	.name		= "dma",
	.dev_groups	= dma_dev_groups,
	.dev_release	= chan_dev_release,
};

/* --- client and device registration --- */

#define dma_device_satisfies_mask(device, mask) \
	__dma_device_satisfies_mask((device), &(mask))
static int
__dma_device_satisfies_mask(struct dma_device *device,
			    const dma_cap_mask_t *want)
{
	dma_cap_mask_t has;

	bitmap_and(has.bits, want->bits, device->cap_mask.bits,
		DMA_TX_TYPE_END);
	return bitmap_equal(want->bits, has.bits, DMA_TX_TYPE_END);
}

static struct module *dma_chan_to_owner(struct dma_chan *chan)
{
	return chan->device->dev->driver->owner;
}

/**
 * balance_ref_count - catch up the channel reference count
 * @chan - channel to balance ->client_count versus dmaengine_ref_count
 *
 * balance_ref_count must be called under dma_list_mutex
 */
static void balance_ref_count(struct dma_chan *chan)
{
	struct module *owner = dma_chan_to_owner(chan);

	while (chan->client_count < dmaengine_ref_count) {
		__module_get(owner);
		chan->client_count++;
	}
}

/**
 * dma_chan_get - try to grab a dma channel's parent driver module
 * @chan - channel to grab
 *
 * Must be called under dma_list_mutex
 */
static int dma_chan_get(struct dma_chan *chan)
{
	struct module *owner = dma_chan_to_owner(chan);
	int ret;

	/* The channel is already in use, update client count */
	if (chan->client_count) {
		__module_get(owner);
		chan->client_count++;
		return 0;
	}

	if (!try_module_get(owner))
		return -ENODEV;

	/* allocate upon first client reference */
	if (chan->device->device_alloc_chan_resources) {
		ret = chan->device->device_alloc_chan_resources(chan);
		if (ret < 0)
			goto err_out;
	}

	chan->client_count++;

	if (!dma_has_cap(DMA_PRIVATE, chan->device->cap_mask))
		balance_ref_count(chan);

	return 0;

err_out:
	module_put(owner);
	return ret;
}

/**
 * dma_chan_put - drop a reference to a dma channel's parent driver module
 * @chan - channel to release
 *
 * Must be called under dma_list_mutex
 */
static void dma_chan_put(struct dma_chan *chan)
{
	/* This channel is not in use, bail out */
	if (!chan->client_count)
		return;

	chan->client_count--;
	module_put(dma_chan_to_owner(chan));

	/* This channel is not in use anymore, free it */
	if (!chan->client_count && chan->device->device_free_chan_resources)
		chan->device->device_free_chan_resources(chan);

	/* If the channel is used via a DMA request router, free the mapping */
	if (chan->router && chan->router->route_free) {
		chan->router->route_free(chan->router->dev, chan->route_data);
		chan->router = NULL;
		chan->route_data = NULL;
	}
}

enum dma_status dma_sync_wait(struct dma_chan *chan, dma_cookie_t cookie)
{
	enum dma_status status;
	unsigned long dma_sync_wait_timeout = jiffies + msecs_to_jiffies(5000);

	dma_async_issue_pending(chan);
	do {
		status = dma_async_is_tx_complete(chan, cookie, NULL, NULL);
		if (time_after_eq(jiffies, dma_sync_wait_timeout)) {
			pr_err("%s: timeout!\n", __func__);
			return DMA_ERROR;
		}
		if (status != DMA_IN_PROGRESS)
			break;
		cpu_relax();
	} while (1);

	return status;
}
EXPORT_SYMBOL(dma_sync_wait);

/**
 * dma_cap_mask_all - enable iteration over all operation types
 */
static dma_cap_mask_t dma_cap_mask_all;

/**
 * dma_chan_tbl_ent - tracks channel allocations per core/operation
 * @chan - associated channel for this entry
 */
struct dma_chan_tbl_ent {
	struct dma_chan *chan;
};

/**
 * channel_table - percpu lookup table for memory-to-memory offload providers
 */
static struct dma_chan_tbl_ent __percpu *channel_table[DMA_TX_TYPE_END];

static int __init dma_channel_table_init(void)
{
	enum dma_transaction_type cap;
	int err = 0;

	bitmap_fill(dma_cap_mask_all.bits, DMA_TX_TYPE_END);

	/* 'interrupt', 'private', and 'slave' are channel capabilities,
	 * but are not associated with an operation so they do not need
	 * an entry in the channel_table
	 */
	clear_bit(DMA_INTERRUPT, dma_cap_mask_all.bits);
	clear_bit(DMA_PRIVATE, dma_cap_mask_all.bits);
	clear_bit(DMA_SLAVE, dma_cap_mask_all.bits);

	for_each_dma_cap_mask(cap, dma_cap_mask_all) {
		channel_table[cap] = alloc_percpu(struct dma_chan_tbl_ent);
		if (!channel_table[cap]) {
			err = -ENOMEM;
			break;
		}
	}

	if (err) {
		pr_err("initialization failure\n");
		for_each_dma_cap_mask(cap, dma_cap_mask_all)
			free_percpu(channel_table[cap]);
	}

	return err;
}
arch_initcall(dma_channel_table_init);

/**
 * dma_find_channel - find a channel to carry out the operation
 * @tx_type: transaction type
 */
struct dma_chan *dma_find_channel(enum dma_transaction_type tx_type)
{
	return this_cpu_read(channel_table[tx_type]->chan);
}
EXPORT_SYMBOL(dma_find_channel);

/**
 * dma_issue_pending_all - flush all pending operations across all channels
 */
void dma_issue_pending_all(void)
{
	struct dma_device *device;
	struct dma_chan *chan;

	rcu_read_lock();
	list_for_each_entry_rcu(device, &dma_device_list, global_node) {
		if (dma_has_cap(DMA_PRIVATE, device->cap_mask))
			continue;
		list_for_each_entry(chan, &device->channels, device_node)
			if (chan->client_count)
				device->device_issue_pending(chan);
	}
	rcu_read_unlock();
}
EXPORT_SYMBOL(dma_issue_pending_all);

/**
 * dma_chan_is_local - returns true if the channel is in the same numa-node as the cpu
 */
static bool dma_chan_is_local(struct dma_chan *chan, int cpu)
{
	int node = dev_to_node(chan->device->dev);
	return node == -1 || cpumask_test_cpu(cpu, cpumask_of_node(node));
}

/**
 * min_chan - returns the channel with min count and in the same numa-node as the cpu
 * @cap: capability to match
 * @cpu: cpu index which the channel should be close to
 *
 * If some channels are close to the given cpu, the one with the lowest
 * reference count is returned. Otherwise, cpu is ignored and only the
 * reference count is taken into account.
 * Must be called under dma_list_mutex.
 */
static struct dma_chan *min_chan(enum dma_transaction_type cap, int cpu)
{
	struct dma_device *device;
	struct dma_chan *chan;
	struct dma_chan *min = NULL;
	struct dma_chan *localmin = NULL;

	list_for_each_entry(device, &dma_device_list, global_node) {
		if (!dma_has_cap(cap, device->cap_mask) ||
		    dma_has_cap(DMA_PRIVATE, device->cap_mask))
			continue;
		list_for_each_entry(chan, &device->channels, device_node) {
			if (!chan->client_count)
				continue;
			if (!min || chan->table_count < min->table_count)
				min = chan;

			if (dma_chan_is_local(chan, cpu))
				if (!localmin ||
				    chan->table_count < localmin->table_count)
					localmin = chan;
		}
	}

	chan = localmin ? localmin : min;

	if (chan)
		chan->table_count++;

	return chan;
}

/**
 * dma_channel_rebalance - redistribute the available channels
 *
 * Optimize for cpu isolation (each cpu gets a dedicated channel for an
 * operation type) in the SMP case,  and operation isolation (avoid
 * multi-tasking channels) in the non-SMP case.  Must be called under
 * dma_list_mutex.
 */
static void dma_channel_rebalance(void)
{
	struct dma_chan *chan;
	struct dma_device *device;
	int cpu;
	int cap;

	/* undo the last distribution */
	for_each_dma_cap_mask(cap, dma_cap_mask_all)
		for_each_possible_cpu(cpu)
			per_cpu_ptr(channel_table[cap], cpu)->chan = NULL;

	list_for_each_entry(device, &dma_device_list, global_node) {
		if (dma_has_cap(DMA_PRIVATE, device->cap_mask))
			continue;
		list_for_each_entry(chan, &device->channels, device_node)
			chan->table_count = 0;
	}

	/* don't populate the channel_table if no clients are available */
	if (!dmaengine_ref_count)
		return;

	/* redistribute available channels */
	for_each_dma_cap_mask(cap, dma_cap_mask_all)
		for_each_online_cpu(cpu) {
			chan = min_chan(cap, cpu);
			per_cpu_ptr(channel_table[cap], cpu)->chan = chan;
		}
}

int dma_get_slave_caps(struct dma_chan *chan, struct dma_slave_caps *caps)
{
	struct dma_device *device;

	if (!chan || !caps)
		return -EINVAL;

	device = chan->device;

	/* check if the channel supports slave transactions */
	if (!test_bit(DMA_SLAVE, device->cap_mask.bits))
		return -ENXIO;

	/*
	 * Check whether it reports it uses the generic slave
	 * capabilities, if not, that means it doesn't support any
	 * kind of slave capabilities reporting.
	 */
	if (!device->directions)
		return -ENXIO;

	caps->src_addr_widths = device->src_addr_widths;
	caps->dst_addr_widths = device->dst_addr_widths;
	caps->directions = device->directions;
	caps->residue_granularity = device->residue_granularity;

	/*
	 * Some devices implement only pause (e.g. to get residuum) but no
	 * resume. However cmd_pause is advertised as pause AND resume.
	 */
	caps->cmd_pause = !!(device->device_pause && device->device_resume);
	caps->cmd_terminate = !!device->device_terminate_all;

	return 0;
}
EXPORT_SYMBOL_GPL(dma_get_slave_caps);

static struct dma_chan *private_candidate(const dma_cap_mask_t *mask,
					  struct dma_device *dev,
					  dma_filter_fn fn, void *fn_param)
{
	struct dma_chan *chan;

	if (!__dma_device_satisfies_mask(dev, mask)) {
		pr_debug("%s: wrong capabilities\n", __func__);
		return NULL;
	}
	/* devices with multiple channels need special handling as we need to
	 * ensure that all channels are either private or public.
	 */
	if (dev->chancnt > 1 && !dma_has_cap(DMA_PRIVATE, dev->cap_mask))
		list_for_each_entry(chan, &dev->channels, device_node) {
			/* some channels are already publicly allocated */
			if (chan->client_count)
				return NULL;
		}

	list_for_each_entry(chan, &dev->channels, device_node) {
		if (chan->client_count) {
			pr_debug("%s: %s busy\n",
				 __func__, dma_chan_name(chan));
			continue;
		}
		if (fn && !fn(chan, fn_param)) {
			pr_debug("%s: %s filter said false\n",
				 __func__, dma_chan_name(chan));
			continue;
		}
		return chan;
	}

	return NULL;
}

/**
 * dma_get_slave_channel - try to get specific channel exclusively
 * @chan: target channel
 */
struct dma_chan *dma_get_slave_channel(struct dma_chan *chan)
{
	int err = -EBUSY;

	/* lock against __dma_request_channel */
	mutex_lock(&dma_list_mutex);

	if (chan->client_count == 0) {
		struct dma_device *device = chan->device;

		dma_cap_set(DMA_PRIVATE, device->cap_mask);
		device->privatecnt++;
		err = dma_chan_get(chan);
		if (err) {
			pr_debug("%s: failed to get %s: (%d)\n",
				__func__, dma_chan_name(chan), err);
			chan = NULL;
			if (--device->privatecnt == 0)
				dma_cap_clear(DMA_PRIVATE, device->cap_mask);
		}
	} else
		chan = NULL;

	mutex_unlock(&dma_list_mutex);


	return chan;
}
EXPORT_SYMBOL_GPL(dma_get_slave_channel);

struct dma_chan *dma_get_any_slave_channel(struct dma_device *device)
{
	dma_cap_mask_t mask;
	struct dma_chan *chan;
	int err;

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	/* lock against __dma_request_channel */
	mutex_lock(&dma_list_mutex);

	chan = private_candidate(&mask, device, NULL, NULL);
	if (chan) {
		dma_cap_set(DMA_PRIVATE, device->cap_mask);
		device->privatecnt++;
		err = dma_chan_get(chan);
		if (err) {
			pr_debug("%s: failed to get %s: (%d)\n",
				__func__, dma_chan_name(chan), err);
			chan = NULL;
			if (--device->privatecnt == 0)
				dma_cap_clear(DMA_PRIVATE, device->cap_mask);
		}
	}

	mutex_unlock(&dma_list_mutex);

	return chan;
}
EXPORT_SYMBOL_GPL(dma_get_any_slave_channel);

/**
 * __dma_request_channel - try to allocate an exclusive channel
 * @mask: capabilities that the channel must satisfy
 * @fn: optional callback to disposition available channels
 * @fn_param: opaque parameter to pass to dma_filter_fn
 *
 * Returns pointer to appropriate DMA channel on success or NULL.
 */
struct dma_chan *__dma_request_channel(const dma_cap_mask_t *mask,
				       dma_filter_fn fn, void *fn_param)
{
	struct dma_device *device, *_d;
	struct dma_chan *chan = NULL;
	int err;

	/* Find a channel */
	mutex_lock(&dma_list_mutex);
	list_for_each_entry_safe(device, _d, &dma_device_list, global_node) {
		chan = private_candidate(mask, device, fn, fn_param);
		if (chan) {
			/* Found a suitable channel, try to grab, prep, and
			 * return it.  We first set DMA_PRIVATE to disable
			 * balance_ref_count as this channel will not be
			 * published in the general-purpose allocator
			 */
			dma_cap_set(DMA_PRIVATE, device->cap_mask);
			device->privatecnt++;
			err = dma_chan_get(chan);

			if (err == -ENODEV) {
				pr_debug("%s: %s module removed\n",
					 __func__, dma_chan_name(chan));
				list_del_rcu(&device->global_node);
			} else if (err)
				pr_debug("%s: failed to get %s: (%d)\n",
					 __func__, dma_chan_name(chan), err);
			else
				break;
			if (--device->privatecnt == 0)
				dma_cap_clear(DMA_PRIVATE, device->cap_mask);
			chan = NULL;
		}
	}
	mutex_unlock(&dma_list_mutex);

	pr_debug("%s: %s (%s)\n",
		 __func__,
		 chan ? "success" : "fail",
		 chan ? dma_chan_name(chan) : NULL);

	return chan;
}
EXPORT_SYMBOL_GPL(__dma_request_channel);

/**
 * dma_request_slave_channel_reason - try to allocate an exclusive slave channel
 * @dev:	pointer to client device structure
 * @name:	slave channel name
 *
 * Returns pointer to appropriate DMA channel on success or an error pointer.
 */
struct dma_chan *dma_request_slave_channel_reason(struct device *dev,
						  const char *name)
{
	/* If device-tree is present get slave info from here */
	if (dev->of_node)
		return of_dma_request_slave_channel(dev->of_node, name);

	/* If device was enumerated by ACPI get slave info from here */
	if (ACPI_HANDLE(dev))
		return acpi_dma_request_slave_chan_by_name(dev, name);

	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL_GPL(dma_request_slave_channel_reason);

/**
 * dma_request_slave_channel - try to allocate an exclusive slave channel
 * @dev:	pointer to client device structure
 * @name:	slave channel name
 *
 * Returns pointer to appropriate DMA channel on success or NULL.
 */
struct dma_chan *dma_request_slave_channel(struct device *dev,
					   const char *name)
{
	struct dma_chan *ch = dma_request_slave_channel_reason(dev, name);
	if (IS_ERR(ch))
		return NULL;

	dma_cap_set(DMA_PRIVATE, ch->device->cap_mask);
	ch->device->privatecnt++;

	return ch;
}
EXPORT_SYMBOL_GPL(dma_request_slave_channel);

void dma_release_channel(struct dma_chan *chan)
{
	mutex_lock(&dma_list_mutex);
	WARN_ONCE(chan->client_count != 1,
		  "chan reference count %d != 1\n", chan->client_count);
	dma_chan_put(chan);
	/* drop PRIVATE cap enabled by __dma_request_channel() */
	if (--chan->device->privatecnt == 0)
		dma_cap_clear(DMA_PRIVATE, chan->device->cap_mask);
	mutex_unlock(&dma_list_mutex);
}
EXPORT_SYMBOL_GPL(dma_release_channel);

/**
 * dmaengine_get - register interest in dma_channels
 */
void dmaengine_get(void)
{
	struct dma_device *device, *_d;
	struct dma_chan *chan;
	int err;

	mutex_lock(&dma_list_mutex);
	dmaengine_ref_count++;

	/* try to grab channels */
	list_for_each_entry_safe(device, _d, &dma_device_list, global_node) {
		if (dma_has_cap(DMA_PRIVATE, device->cap_mask))
			continue;
		list_for_each_entry(chan, &device->channels, device_node) {
			err = dma_chan_get(chan);
			if (err == -ENODEV) {
				/* module removed before we could use it */
				list_del_rcu(&device->global_node);
				break;
			} else if (err)
				pr_debug("%s: failed to get %s: (%d)\n",
				       __func__, dma_chan_name(chan), err);
		}
	}

	/* if this is the first reference and there were channels
	 * waiting we need to rebalance to get those channels
	 * incorporated into the channel table
	 */
	if (dmaengine_ref_count == 1)
		dma_channel_rebalance();
	mutex_unlock(&dma_list_mutex);
}
EXPORT_SYMBOL(dmaengine_get);

/**
 * dmaengine_put - let dma drivers be removed when ref_count == 0
 */
void dmaengine_put(void)
{
	struct dma_device *device;
	struct dma_chan *chan;

	mutex_lock(&dma_list_mutex);
	dmaengine_ref_count--;
	BUG_ON(dmaengine_ref_count < 0);
	/* drop channel references */
	list_for_each_entry(device, &dma_device_list, global_node) {
		if (dma_has_cap(DMA_PRIVATE, device->cap_mask))
			continue;
		list_for_each_entry(chan, &device->channels, device_node)
			dma_chan_put(chan);
	}
	mutex_unlock(&dma_list_mutex);
}
EXPORT_SYMBOL(dmaengine_put);

static bool device_has_all_tx_types(struct dma_device *device)
{
	/* A device that satisfies this test has channels that will never cause
	 * an async_tx channel switch event as all possible operation types can
	 * be handled.
	 */
	#ifdef CONFIG_ASYNC_TX_DMA
	if (!dma_has_cap(DMA_INTERRUPT, device->cap_mask))
		return false;
	#endif

	#if defined(CONFIG_ASYNC_MEMCPY) || defined(CONFIG_ASYNC_MEMCPY_MODULE)
	if (!dma_has_cap(DMA_MEMCPY, device->cap_mask))
		return false;
	#endif

	#if defined(CONFIG_ASYNC_XOR) || defined(CONFIG_ASYNC_XOR_MODULE)
	if (!dma_has_cap(DMA_XOR, device->cap_mask))
		return false;

	#ifndef CONFIG_ASYNC_TX_DISABLE_XOR_VAL_DMA
	if (!dma_has_cap(DMA_XOR_VAL, device->cap_mask))
		return false;
	#endif
	#endif

	#if defined(CONFIG_ASYNC_PQ) || defined(CONFIG_ASYNC_PQ_MODULE)
	if (!dma_has_cap(DMA_PQ, device->cap_mask))
		return false;

	#ifndef CONFIG_ASYNC_TX_DISABLE_PQ_VAL_DMA
	if (!dma_has_cap(DMA_PQ_VAL, device->cap_mask))
		return false;
	#endif
	#endif

	return true;
}

static int get_dma_id(struct dma_device *device)
{
	int rc;

	mutex_lock(&dma_list_mutex);

	rc = idr_alloc(&dma_idr, NULL, 0, 0, GFP_KERNEL);
	if (rc >= 0)
		device->dev_id = rc;

	mutex_unlock(&dma_list_mutex);
	return rc < 0 ? rc : 0;
}

/**
 * dma_async_device_register - registers DMA devices found
 * @device: &dma_device
 */
int dma_async_device_register(struct dma_device *device)
{
	int chancnt = 0, rc;
	struct dma_chan* chan;
	atomic_t *idr_ref;

	if (!device)
		return -ENODEV;

	/* validate device routines */
	BUG_ON(dma_has_cap(DMA_MEMCPY, device->cap_mask) &&
		!device->device_prep_dma_memcpy);
	BUG_ON(dma_has_cap(DMA_XOR, device->cap_mask) &&
		!device->device_prep_dma_xor);
	BUG_ON(dma_has_cap(DMA_XOR_VAL, device->cap_mask) &&
		!device->device_prep_dma_xor_val);
	BUG_ON(dma_has_cap(DMA_PQ, device->cap_mask) &&
		!device->device_prep_dma_pq);
	BUG_ON(dma_has_cap(DMA_PQ_VAL, device->cap_mask) &&
		!device->device_prep_dma_pq_val);
	BUG_ON(dma_has_cap(DMA_MEMSET, device->cap_mask) &&
		!device->device_prep_dma_memset);
	BUG_ON(dma_has_cap(DMA_INTERRUPT, device->cap_mask) &&
		!device->device_prep_dma_interrupt);
	BUG_ON(dma_has_cap(DMA_SG, device->cap_mask) &&
		!device->device_prep_dma_sg);
	BUG_ON(dma_has_cap(DMA_CYCLIC, device->cap_mask) &&
		!device->device_prep_dma_cyclic);
	BUG_ON(dma_has_cap(DMA_INTERLEAVE, device->cap_mask) &&
		!device->device_prep_interleaved_dma);

	BUG_ON(!device->device_tx_status);
	BUG_ON(!device->device_issue_pending);
	BUG_ON(!device->dev);

	/* note: this only matters in the
	 * CONFIG_ASYNC_TX_ENABLE_CHANNEL_SWITCH=n case
	 */
	if (device_has_all_tx_types(device))
		dma_cap_set(DMA_ASYNC_TX, device->cap_mask);

	idr_ref = kmalloc(sizeof(*idr_ref), GFP_KERNEL);
	if (!idr_ref)
		return -ENOMEM;
	rc = get_dma_id(device);
	if (rc != 0) {
		kfree(idr_ref);
		return rc;
	}

	atomic_set(idr_ref, 0);

	/* represent channels in sysfs. Probably want devs too */
	list_for_each_entry(chan, &device->channels, device_node) {
		rc = -ENOMEM;
		chan->local = alloc_percpu(typeof(*chan->local));
		if (chan->local == NULL)
			goto err_out;
		chan->dev = kzalloc(sizeof(*chan->dev), GFP_KERNEL);
		if (chan->dev == NULL) {
			free_percpu(chan->local);
			chan->local = NULL;
			goto err_out;
		}

		chan->chan_id = chancnt++;
		chan->dev->device.class = &dma_devclass;
		chan->dev->device.parent = device->dev;
		chan->dev->chan = chan;
		chan->dev->idr_ref = idr_ref;
		chan->dev->dev_id = device->dev_id;
		atomic_inc(idr_ref);
		dev_set_name(&chan->dev->device, "dma%dchan%d",
			     device->dev_id, chan->chan_id);

		rc = device_register(&chan->dev->device);
		if (rc) {
			free_percpu(chan->local);
			chan->local = NULL;
			kfree(chan->dev);
			atomic_dec(idr_ref);
			goto err_out;
		}
		chan->client_count = 0;
	}
	device->chancnt = chancnt;

	mutex_lock(&dma_list_mutex);
	/* take references on public channels */
	if (dmaengine_ref_count && !dma_has_cap(DMA_PRIVATE, device->cap_mask))
		list_for_each_entry(chan, &device->channels, device_node) {
			/* if clients are already waiting for channels we need
			 * to take references on their behalf
			 */
			if (dma_chan_get(chan) == -ENODEV) {
				/* note we can only get here for the first
				 * channel as the remaining channels are
				 * guaranteed to get a reference
				 */
				rc = -ENODEV;
				mutex_unlock(&dma_list_mutex);
				goto err_out;
			}
		}
	list_add_tail_rcu(&device->global_node, &dma_device_list);
	if (dma_has_cap(DMA_PRIVATE, device->cap_mask))
		device->privatecnt++;	/* Always private */
	dma_channel_rebalance();
	kfree(idr_ref);
	mutex_unlock(&dma_list_mutex);

	if (!chancnt)
		kfree(idr_ref);

	return 0;

err_out:
	/* if we never registered a channel just release the idr */
	if (atomic_read(idr_ref) == 0) {
		mutex_lock(&dma_list_mutex);
		idr_remove(&dma_idr, device->dev_id);
		mutex_unlock(&dma_list_mutex);
		kfree(idr_ref);
		return rc;
	}

	list_for_each_entry(chan, &device->channels, device_node) {
		if (chan->local == NULL)
			continue;
		mutex_lock(&dma_list_mutex);
		chan->dev->chan = NULL;
		mutex_unlock(&dma_list_mutex);
		device_unregister(&chan->dev->device);
		free_percpu(chan->local);
	}
	return rc;
}
EXPORT_SYMBOL(dma_async_device_register);

/**
 * dma_async_device_unregister - unregister a DMA device
 * @device: &dma_device
 *
 * This routine is called by dma driver exit routines, dmaengine holds module
 * references to prevent it being called while channels are in use.
 */
void dma_async_device_unregister(struct dma_device *device)
{
	struct dma_chan *chan;

	mutex_lock(&dma_list_mutex);
	list_del_rcu(&device->global_node);
	dma_channel_rebalance();
	mutex_unlock(&dma_list_mutex);

	list_for_each_entry(chan, &device->channels, device_node) {
		WARN_ONCE(chan->client_count,
			  "%s called while %d clients hold a reference\n",
			  __func__, chan->client_count);
		mutex_lock(&dma_list_mutex);
		chan->dev->chan = NULL;
		mutex_unlock(&dma_list_mutex);
		device_unregister(&chan->dev->device);
		free_percpu(chan->local);
	}
}
EXPORT_SYMBOL(dma_async_device_unregister);

struct dmaengine_unmap_pool {
	struct kmem_cache *cache;
	const char *name;
	mempool_t *pool;
	size_t size;
};

#define __UNMAP_POOL(x) { .size = x, .name = "dmaengine-unmap-" __stringify(x) }
static struct dmaengine_unmap_pool unmap_pool[] = {
	__UNMAP_POOL(2),
	#if IS_ENABLED(CONFIG_DMA_ENGINE_RAID)
	__UNMAP_POOL(16),
	__UNMAP_POOL(128),
	__UNMAP_POOL(256),
	#endif
};

static struct dmaengine_unmap_pool *__get_unmap_pool(int nr)
{
	int order = get_count_order(nr);

	switch (order) {
	case 0 ... 1:
		return &unmap_pool[0];
#if IS_ENABLED(CONFIG_DMA_ENGINE_RAID)
	case 2 ... 4:
		return &unmap_pool[1];
	case 5 ... 7:
		return &unmap_pool[2];
	case 8:
		return &unmap_pool[3];
#endif
	default:
		BUG();
		return NULL;
	}
}

static void dmaengine_unmap(struct kref *kref)
{
	struct dmaengine_unmap_data *unmap = container_of(kref, typeof(*unmap), kref);
	struct device *dev = unmap->dev;
	int cnt, i;

	cnt = unmap->to_cnt;
	for (i = 0; i < cnt; i++)
		dma_unmap_page(dev, unmap->addr[i], unmap->len,
			       DMA_TO_DEVICE);
	cnt += unmap->from_cnt;
	for (; i < cnt; i++)
		dma_unmap_page(dev, unmap->addr[i], unmap->len,
			       DMA_FROM_DEVICE);
	cnt += unmap->bidi_cnt;
	for (; i < cnt; i++) {
		if (unmap->addr[i] == 0)
			continue;
		dma_unmap_page(dev, unmap->addr[i], unmap->len,
			       DMA_BIDIRECTIONAL);
	}
	cnt = unmap->map_cnt;
	mempool_free(unmap, __get_unmap_pool(cnt)->pool);
}

void dmaengine_unmap_put(struct dmaengine_unmap_data *unmap)
{
	if (unmap)
		kref_put(&unmap->kref, dmaengine_unmap);
}
EXPORT_SYMBOL_GPL(dmaengine_unmap_put);

static void dmaengine_destroy_unmap_pool(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(unmap_pool); i++) {
		struct dmaengine_unmap_pool *p = &unmap_pool[i];

		mempool_destroy(p->pool);
		p->pool = NULL;
		kmem_cache_destroy(p->cache);
		p->cache = NULL;
	}
}

static int __init dmaengine_init_unmap_pool(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(unmap_pool); i++) {
		struct dmaengine_unmap_pool *p = &unmap_pool[i];
		size_t size;

		size = sizeof(struct dmaengine_unmap_data) +
		       sizeof(dma_addr_t) * p->size;

		p->cache = kmem_cache_create(p->name, size, 0,
					     SLAB_HWCACHE_ALIGN, NULL);
		if (!p->cache)
			break;
		p->pool = mempool_create_slab_pool(1, p->cache);
		if (!p->pool)
			break;
	}

	if (i == ARRAY_SIZE(unmap_pool))
		return 0;

	dmaengine_destroy_unmap_pool();
	return -ENOMEM;
}

struct dmaengine_unmap_data *
dmaengine_get_unmap_data(struct device *dev, int nr, gfp_t flags)
{
	struct dmaengine_unmap_data *unmap;

	unmap = mempool_alloc(__get_unmap_pool(nr)->pool, flags);
	if (!unmap)
		return NULL;

	memset(unmap, 0, sizeof(*unmap));
	kref_init(&unmap->kref);
	unmap->dev = dev;
	unmap->map_cnt = nr;

	return unmap;
}
EXPORT_SYMBOL(dmaengine_get_unmap_data);

void dma_async_tx_descriptor_init(struct dma_async_tx_descriptor *tx,
	struct dma_chan *chan)
{
	tx->chan = chan;
	#ifdef CONFIG_ASYNC_TX_ENABLE_CHANNEL_SWITCH
	spin_lock_init(&tx->lock);
	#endif
}
EXPORT_SYMBOL(dma_async_tx_descriptor_init);

/* dma_wait_for_async_tx - spin wait for a transaction to complete
 * @tx: in-flight transaction to wait on
 */
enum dma_status
dma_wait_for_async_tx(struct dma_async_tx_descriptor *tx)
{
	unsigned long dma_sync_wait_timeout = jiffies + msecs_to_jiffies(5000);

	if (!tx)
		return DMA_COMPLETE;

	while (tx->cookie == -EBUSY) {
		if (time_after_eq(jiffies, dma_sync_wait_timeout)) {
			pr_err("%s timeout waiting for descriptor submission\n",
			       __func__);
			return DMA_ERROR;
		}
		cpu_relax();
	}
	return dma_sync_wait(tx->chan, tx->cookie);
}
EXPORT_SYMBOL_GPL(dma_wait_for_async_tx);

/* dma_run_dependencies - helper routine for dma drivers to process
 *	(start) dependent operations on their target channel
 * @tx: transaction with dependencies
 */
void dma_run_dependencies(struct dma_async_tx_descriptor *tx)
{
	struct dma_async_tx_descriptor *dep = txd_next(tx);
	struct dma_async_tx_descriptor *dep_next;
	struct dma_chan *chan;

	if (!dep)
		return;

	/* we'll submit tx->next now, so clear the link */
	txd_clear_next(tx);
	chan = dep->chan;

	/* keep submitting up until a channel switch is detected
	 * in that case we will be called again as a result of
	 * processing the interrupt from async_tx_channel_switch
	 */
	for (; dep; dep = dep_next) {
		txd_lock(dep);
		txd_clear_parent(dep);
		dep_next = txd_next(dep);
		if (dep_next && dep_next->chan == chan)
			txd_clear_next(dep); /* ->next will be submitted */
		else
			dep_next = NULL; /* submit current dep and terminate */
		txd_unlock(dep);

		dep->tx_submit(dep);
	}

	chan->device->device_issue_pending(chan);
}
EXPORT_SYMBOL_GPL(dma_run_dependencies);

static int __init dma_bus_init(void)
{
	int err = dmaengine_init_unmap_pool();

	if (err)
		return err;
	return class_register(&dma_devclass);
}
arch_initcall(dma_bus_init);


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * The contents of this file are private to DMA engine drivers, and is not
 * part of the API to be used by DMA engine users.
 */
#ifndef DMAENGINE_H
#define DMAENGINE_H

#include <linux/bug.h>
#include <linux/dmaengine.h>

/**
 * dma_cookie_init - initialize the cookies for a DMA channel
 * @chan: dma channel to initialize
 */
static inline void dma_cookie_init(struct dma_chan *chan)
{
	chan->cookie = DMA_MIN_COOKIE;
	chan->completed_cookie = DMA_MIN_COOKIE;
}

/**
 * dma_cookie_assign - assign a DMA engine cookie to the descriptor
 * @tx: descriptor needing cookie
 *
 * Assign a unique non-zero per-channel cookie to the descriptor.
 * Note: caller is expected to hold a lock to prevent concurrency.
 */
static inline dma_cookie_t dma_cookie_assign(struct dma_async_tx_descriptor *tx)
{
	struct dma_chan *chan = tx->chan;
	dma_cookie_t cookie;

	cookie = chan->cookie + 1;
	if (cookie < DMA_MIN_COOKIE)
		cookie = DMA_MIN_COOKIE;
	tx->cookie = chan->cookie = cookie;

	return cookie;
}

/**
 * dma_cookie_complete - complete a descriptor
 * @tx: descriptor to complete
 *
 * Mark this descriptor complete by updating the channels completed
 * cookie marker.  Zero the descriptors cookie to prevent accidental
 * repeated completions.
 *
 * Note: caller is expected to hold a lock to prevent concurrency.
 */
static inline void dma_cookie_complete(struct dma_async_tx_descriptor *tx)
{
	BUG_ON(tx->cookie < DMA_MIN_COOKIE);
	tx->chan->completed_cookie = tx->cookie;
	tx->cookie = 0;
}

/**
 * dma_cookie_status - report cookie status
 * @chan: dma channel
 * @cookie: cookie we are interested in
 * @state: dma_tx_state structure to return last/used cookies
 *
 * Report the status of the cookie, filling in the state structure if
 * non-NULL.  No locking is required.
 */
static inline enum dma_status dma_cookie_status(struct dma_chan *chan,
	dma_cookie_t cookie, struct dma_tx_state *state)
{
	dma_cookie_t used, complete;

	used = chan->cookie;
	complete = chan->completed_cookie;
	barrier();
	if (state) {
		state->last = complete;
		state->used = used;
		state->residue = 0;
	}
	return dma_async_is_complete(cookie, complete, used);
}

static inline void dma_set_residue(struct dma_tx_state *state, u32 residue)
{
	if (state)
		state->residue = residue;
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * DMA Engine test module
 *
 * Copyright (C) 2007 Atmel Corporation
 * Copyright (C) 2013 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/freezer.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/wait.h>

static unsigned int test_buf_size = 16384;
module_param(test_buf_size, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(test_buf_size, "Size of the memcpy test buffer");

static char test_channel[20];
module_param_string(channel, test_channel, sizeof(test_channel),
		S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(channel, "Bus ID of the channel to test (default: any)");

static char test_device[32];
module_param_string(device, test_device, sizeof(test_device),
		S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(device, "Bus ID of the DMA Engine to test (default: any)");

static unsigned int threads_per_chan = 1;
module_param(threads_per_chan, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(threads_per_chan,
		"Number of threads to start per channel (default: 1)");

static unsigned int max_channels;
module_param(max_channels, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(max_channels,
		"Maximum number of channels to use (default: all)");

static unsigned int iterations;
module_param(iterations, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iterations,
		"Iterations before stopping test (default: infinite)");

static unsigned int xor_sources = 3;
module_param(xor_sources, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(xor_sources,
		"Number of xor source buffers (default: 3)");

static unsigned int pq_sources = 3;
module_param(pq_sources, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(pq_sources,
		"Number of p+q source buffers (default: 3)");

static int timeout = 3000;
module_param(timeout, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(timeout, "Transfer Timeout in msec (default: 3000), "
		 "Pass -1 for infinite timeout");

static bool noverify;
module_param(noverify, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(noverify, "Disable random data setup and verification");

static bool verbose;
module_param(verbose, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(verbose, "Enable \"success\" result messages (default: off)");

/**
 * struct dmatest_params - test parameters.
 * @buf_size:		size of the memcpy test buffer
 * @channel:		bus ID of the channel to test
 * @device:		bus ID of the DMA Engine to test
 * @threads_per_chan:	number of threads to start per channel
 * @max_channels:	maximum number of channels to use
 * @iterations:		iterations before stopping test
 * @xor_sources:	number of xor source buffers
 * @pq_sources:		number of p+q source buffers
 * @timeout:		transfer 