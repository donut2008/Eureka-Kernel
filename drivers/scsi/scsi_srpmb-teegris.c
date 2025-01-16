 channel statue machine */
	bool byte_align;

	struct dma_pool *desc_pool;	/* Descriptors pool */
};

struct mmp_pdma_phy {
	int idx;
	void __iomem *base;
	struct mmp_pdma_chan *vchan;
};

struct mmp_pdma_device {
	int				dma_channels;
	void __iomem			*base;
	struct device			*dev;
	struct dma_device		device;
	struct mmp_pdma_phy		*phy;
	spinlock_t phy_lock; /* protect alloc/free phy channels */
};

#define tx_to_mmp_pdma_desc(tx)					\
	container_of(tx, struct mmp_pdma_desc_sw, async_tx)
#define to_mmp_pdma_desc(lh)					\
	container_of(lh, struct mmp_pdma_desc_sw, node)
#define to_mmp_pdma_chan(dchan)					\
	container_of(dchan, struct mmp_pdma_chan, chan)
#define to_mmp_pdma_dev(dmadev)					\
	container_of(dmadev, struct mmp_pdma_device, device)

static void set_desc(struct mmp_pdma_phy *phy, dma_addr_t addr)
{
	u32 reg = (phy->idx << 4) + DDADR;

	writel(addr, phy->base + reg);
}

static void enable_chan(struct mmp_pdma_phy *phy)
{
	u32 reg, dalgn;

	if (!phy->vchan)
		return;

	reg = DRCMR(phy->vchan->drcmr);
	writel(DRCMR_MAPVLD | phy->idx, phy->base + reg);

	dalgn = readl(phy->base + DALGN);
	if (phy->vchan->byte_align)
		dalgn |= 1 << phy->idx;
	else
		dalgn &= ~(1 << phy->idx);
	writel(dalgn, phy->base + DALGN);

	reg = (phy->idx << 2) + DCSR;
	writel(readl(phy->base + reg) | DCSR_RUN, phy->base + reg);
}

static void disable_chan(struct mmp_pdma_phy *phy)
{
	u32 reg;

	if (!phy)
		return;

	reg = (phy->idx << 2) + DCSR;
	writel(readl(phy->base + reg) & ~DCSR_RUN, phy->base + reg);
}

static int clear_chan_irq(struct mmp_pdma_phy *phy)
{
	u32 dcsr;
	u32 dint = readl(phy->base + DINT);
	u32 reg = (phy->idx << 2) + DCSR;

	if (!(dint & BIT(phy->idx)))
		return -EAGAIN;

	/* clear irq */
	dcsr = readl(phy->base + reg);
	writel(dcsr, phy->base + reg);
	if ((dcsr & DCSR_BUSERR) && (phy->vchan))
		dev_warn(phy->vchan->dev, "DCSR_BUSERR\n");

	return 0;
}

static irqreturn_t mmp_pdma_chan_handler(int irq, void *dev_id)
{
	struct mmp_pdma_phy *phy = dev_id;

	if (clear_chan_irq(phy) != 0)
		return IRQ_NONE;

	tasklet_schedule(&phy->vchan->tasklet);
	return IRQ_HANDLED;
}

static irqreturn_t mmp_pdma_int_handler(int irq, void *dev_id)
{
	struct mmp_pdma_device *pdev = dev_id;
	struct mmp_pdma_phy *phy;
	u32 dint = readl(pdev->base + DINT);
	int i, ret;
	int irq_num = 0;

	while (dint) {
		i = __ffs(dint);
		/* only handle interrupts belonging to pdma driver*/
		if (i >= pdev->dma_channels)
			break;
		dint &= (dint - 1);
		phy = &pdev->phy[i];
		ret = mmp_pdma_chan_handler(irq, phy);
		if (ret == IRQ_HANDLED)
			irq_num++;
	}

	if (irq_num)
		return IRQ_HANDLED;

	return IRQ_NONE;
}

/* lookup free phy channel as descending priority */
static struct mmp_pdma_phy *lookup_phy(struct mmp_pdma_chan *pchan)
{
	int prio, i;
	struct mmp_pdma_device *pdev = to_mmp_pdma_dev(pchan->chan.device);
	struct mmp_pdma_phy *phy, *found = NULL;
	unsigned long flags;

	/*
	 * dma channel priorities
	 * ch 0 - 3,  16 - 19  <--> (0)
	 * ch 4 - 7,  20 - 23  <--> (1)
	 * ch 8 - 11, 24 - 27  <--> (2)
	 * ch 12 - 15, 28 - 31  <--> (3)
	 */

	spin_lock_irqsave(&pdev->phy_lock, flags);
	for (prio = 0; prio <= ((pdev->dma_channels - 1) & 0xf) >> 2; prio++) {
		for (i = 0; i < pdev->dma_channels; i++) {
			if (prio != (i & 0xf) >> 2)
				continue;
			phy = &pdev->phy[i];
			if (!phy->vchan) {
				phy->vchan = pchan;
				found = phy;
				goto out_unlock;
			}
		}
	}

out_unlock:
	spin_unlock_irqrestore(&pdev->phy_lock, flags);
	return found;
}

static void mmp_pdma_free_phy(struct mmp_pdma_chan *pchan)
{
	struct mmp_pdma_device *pdev = to_mmp_pdma_dev(pchan->chan.device);
	unsigned long flags;
	u32 reg;

	if (!pchan->phy)
		return;

	/* clear the channel mapping in DRCMR */
	reg = DRCMR(pchan->drcmr);
	writel(0, pchan->phy->base + reg);

	spin_lock_irqsave(&pdev->phy_lock, flags);
	pchan->phy->vchan = NULL;
	pchan->phy = NULL;
	spin_unlock_irqrestore(&pdev->phy_lock, flags);
}

/**
 * start_pending_queue - transfer any pending transactions
 * pending list ==> running list
 */
static void start_pending_queue(struct mmp_pdma_chan *chan)
{
	struct mmp_pdma_desc_sw *desc;

	/* still in running, irq will start the pending list */
	if (!chan->idle) {
		dev_dbg(chan->dev, "DMA controller still busy\n");
		return;
	}

	if (list_empty(&chan->chain_pending)) {
		/* chance to re-fetch phy channel with higher prio */
		mmp_pdma_free_phy(chan);
		dev_dbg(chan->dev, "no pending list\n");
		return;
	}

	if (!chan->phy) {
		chan->phy = lookup_phy(chan);
		if (!chan->phy) {
			dev_dbg(chan->dev, "no free dma channel\n");
			return;
		}
	}

	/*
	 * pending -> running
	 * reintilize pending list
	 */
	desc = list_first_entry(&chan->chain_pending,
				struct mmp_pdma_desc_sw, node);
	list_splice_tail_init(&chan->chain_pending, &chan->chain_running);

	/*
	 * Program the descriptor's address into the DMA controller,
	 * then start the DMA transaction
	 */
	set_desc(chan->phy, desc->async_tx.phys);
	enable_chan(chan->phy);
	chan->idle = false;
}


/* desc->tx_list ==> pending list */
static dma_cookie_t mmp_pdma_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(tx->chan);
	struct mmp_pdma_desc_sw *desc = tx_to_mmp_pdma_desc(tx);
	struct mmp_pdma_desc_sw *child;
	unsigned long flags;
	dma_cookie_t cookie = -EBUSY;

	spin_lock_irqsave(&chan->desc_lock, flags);

	list_for_each_entry(child, &desc->tx_list, node) {
		cookie = dma_cookie_assign(&child->async_tx);
	}

	/* softly link to pending list - desc->tx_list ==> pending list */
	list_splice_tail_init(&desc->tx_list, &chan->chain_pending);

	spin_unlock_irqrestore(&chan->desc_lock, flags);

	return cookie;
}

static struct mmp_pdma_desc_sw *
mmp_pdma_alloc_descriptor(struct mmp_pdma_chan *chan)
{
	struct mmp_pdma_desc_sw *desc;
	dma_addr_t pdesc;

	desc = dma_pool_alloc(chan->desc_pool, GFP_ATOMIC, &pdesc);
	if (!desc) {
		dev_err(chan->dev, "out of memory for link descriptor\n");
		return NULL;
	}

	memset(desc, 0, sizeof(*desc));
	INIT_LIST_HEAD(&desc->tx_list);
	dma_async_tx_descriptor_init(&desc->async_tx, &chan->chan);
	/* each desc has submit */
	desc->async_tx.tx_submit = mmp_pdma_tx_submit;
	desc->async_tx.phys = pdesc;

	return desc;
}

/**
 * mmp_pdma_alloc_chan_resources - Allocate resources for DMA channel.
 *
 * This function will create a dma pool for descriptor allocation.
 * Request irq only when channel is requested
 * Return - The number of allocated descriptors.
 */

static int mmp_pdma_alloc_chan_resources(struct dma_chan *dchan)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);

	if (chan->desc_pool)
		return 1;

	chan->desc_pool = dma_pool_create(dev_name(&dchan->dev->device),
					  chan->dev,
					  sizeof(struct mmp_pdma_desc_sw),
					  __alignof__(struct mmp_pdma_desc_sw),
					  0);
	if (!chan->desc_pool) {
		dev_err(chan->dev, "unable to allocate descriptor pool\n");
		return -ENOMEM;
	}

	mmp_pdma_free_phy(chan);
	chan->idle = true;
	chan->dev_addr = 0;
	return 1;
}

static void mmp_pdma_free_desc_list(struct mmp_pdma_chan *chan,
				    struct list_head *list)
{
	struct mmp_pdma_desc_sw *desc, *_desc;

	list_for_each_entry_safe(desc, _desc, list, node) {
		list_del(&desc->node);
		dma_pool_free(chan->desc_pool, desc, desc->async_tx.phys);
	}
}

static void mmp_pdma_free_chan_resources(struct dma_chan *dchan)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);
	unsigned long flags;

	spin_lock_irqsave(&chan->desc_lock, flags);
	mmp_pdma_free_desc_list(chan, &chan->chain_pending);
	mmp_pdma_free_desc_list(chan, &chan->chain_running);
	spin_unlock_irqrestore(&chan->desc_lock, flags);

	dma_pool_destroy(chan->desc_pool);
	chan->desc_pool = NULL;
	chan->idle = true;
	chan->dev_addr = 0;
	mmp_pdma_free_phy(chan);
	return;
}

static struct dma_async_tx_descriptor *
mmp_pdma_prep_memcpy(struct dma_chan *dchan,
		     dma_addr_t dma_dst, dma_addr_t dma_src,
		     size_t len, unsigned long flags)
{
	struct mmp_pdma_chan *chan;
	struct mmp_pdma_desc_sw *first = NULL, *prev = NULL, *new;
	size_t copy = 0;

	if (!dchan)
		return NULL;

	if (!len)
		return NULL;

	chan = to_mmp_pdma_chan(dchan);
	chan->byte_align = false;

	if (!chan->dir) {
		chan->dir = DMA_MEM_TO_MEM;
		chan->dcmd = DCMD_INCTRGADDR | DCMD_INCSRCADDR;
		chan->dcmd |= DCMD_BURST32;
	}

	do {
		/* Allocate the link descriptor from DMA pool */
		new = mmp_pdma_alloc_descriptor(chan);
		if (!new) {
			dev_err(chan->dev, "no memory for desc\n");
			goto fail;
		}

		copy = min_t(size_t, len, PDMA_MAX_DESC_BYTES);
		if (dma_src & 0x7 || dma_dst & 0x7)
			chan->byte_align = true;

		new->desc.dcmd = chan->dcmd | (DCMD_LENGTH & copy);
		new->desc.dsadr = dma_src;
		new->desc.dtadr = dma_dst;

		if (!first)
			first = new;
		else
			prev->desc.ddadr = new->async_tx.phys;

		new->async_tx.cookie = 0;
		async_tx_ack(&new->async_tx);

		prev = new;
		len -= copy;

		if (chan->dir == DMA_MEM_TO_DEV) {
			dma_src += copy;
		} else if (chan->dir == DMA_DEV_TO_MEM) {
			dma_dst += copy;
		} else if (chan->dir == DMA_MEM_TO_MEM) {
			dma_src += copy;
			dma_dst += copy;
		}

		/* Insert the link descriptor to the LD ring */
		list_add_tail(&new->node, &first->tx_list);
	} while (len);

	first->async_tx.flags = flags; /* client is in control of this ack */
	first->async_tx.cookie = -EBUSY;

	/* last desc and fire IRQ */
	new->desc.ddadr = DDADR_STOP;
	new->desc.dcmd |= DCMD_ENDIRQEN;

	chan->cyclic_first = NULL;

	return &first->async_tx;

fail:
	if (first)
		mmp_pdma_free_desc_list(chan, &first->tx_list);
	return NULL;
}

static struct dma_async_tx_descriptor *
mmp_pdma_prep_slave_sg(struct dma_chan *dchan, struct scatterlist *sgl,
		       unsigned int sg_len, enum dma_transfer_direction dir,
		       unsigned long flags, void *context)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);
	struct mmp_pdma_desc_sw *first = NULL, *prev = NULL, *new = NULL;
	size_t len, avail;
	struct scatterlist *sg;
	dma_addr_t addr;
	int i;

	if ((sgl == NULL) || (sg_len == 0))
		return NULL;

	chan->byte_align = false;

	for_each_sg(sgl, sg, sg_len, i) {
		addr = sg_dma_address(sg);
		avail = sg_dma_len(sgl);

		do {
			len = min_t(size_t, avail, PDMA_MAX_DESC_BYTES);
			if (addr & 0x7)
				chan->byte_align = true;

			/* allocate and populate the descriptor */
			new = mmp_pdma_alloc_descriptor(chan);
			if (!new) {
				dev_err(chan->dev, "no memory for desc\n");
				goto fail;
			}

			new->desc.dcmd = chan->dcmd | (DCMD_LENGTH & len);
			if (dir == DMA_MEM_TO_DEV) {
				new->desc.dsadr = addr;
				new->desc.dtadr = chan->dev_addr;
			} else {
				new->desc.dsadr = chan->dev_addr;
				new->desc.dtadr = addr;
			}

			if (!first)
				first = new;
			else
				prev->desc.ddadr = new->async_tx.phys;

			new->async_tx.cookie = 0;
			async_tx_ack(&new->async_tx);
			prev = new;

			/* Insert the link descriptor to the LD ring */
			list_add_tail(&new->node, &first->tx_list);

			/* update metadata */
			addr += len;
			avail -= len;
		} while (avail);
	}

	first->async_tx.cookie = -EBUSY;
	first->async_tx.flags = flags;

	/* last desc and fire IRQ */
	new->desc.ddadr = DDADR_STOP;
	new->desc.dcmd |= DCMD_ENDIRQEN;

	chan->dir = dir;
	chan->cyclic_first = NULL;

	return &first->async_tx;

fail:
	if (first)
		mmp_pdma_free_desc_list(chan, &first->tx_list);
	return NULL;
}

static struct dma_async_tx_descriptor *
mmp_pdma_prep_dma_cyclic(struct dma_chan *dchan,
			 dma_addr_t buf_addr, size_t len, size_t period_len,
			 enum dma_transfer_direction direction,
			 unsigned long flags, void *context)
{
	struct mmp_pdma_chan *chan;
	struct mmp_pdma_desc_sw *first = NULL, *prev = NULL, *new;
	dma_addr_t dma_src, dma_dst;

	if (!dchan || !len || !period_len)
		return NULL;

	/* the buffer length must be a multiple of period_len */
	if (len % period_len != 0)
		return NULL;

	if (period_len > PDMA_MAX_DESC_BYTES)
		return NULL;

	chan = to_mmp_pdma_chan(dchan);

	switch (direction) {
	case DMA_MEM_TO_DEV:
		dma_src = buf_addr;
		dma_dst = chan->dev_addr;
		break;
	case DMA_DEV_TO_MEM:
		dma_dst = buf_addr;
		dma_src = chan->dev_addr;
		break;
	default:
		dev_err(chan->dev, "Unsupported direction for cyclic DMA\n");
		return NULL;
	}

	chan->dir = direction;

	do {
		/* Allocate the link descriptor from DMA pool */
		new = mmp_pdma_alloc_descriptor(chan);
		if (!new) {
			dev_err(chan->dev, "no memory for desc\n");
			goto fail;
		}

		new->desc.dcmd = (chan->dcmd | DCMD_ENDIRQEN |
				  (DCMD_LENGTH & period_len));
		new->desc.dsadr = dma_src;
		new->desc.dtadr = dma_dst;

		if (!first)
			first = new;
		else
			prev->desc.ddadr = new->async_tx.phys;

		new->async_tx.cookie = 0;
		async_tx_ack(&new->async_tx);

		prev = new;
		len -= period_len;

		if (chan->dir == DMA_MEM_TO_DEV)
			dma_src += period_len;
		else
			dma_dst += period_len;

		/* Insert the link descriptor to the LD ring */
		list_add_tail(&new->node, &first->tx_list);
	} while (len);

	first->async_tx.flags = flags; /* client is in control of this ack */
	first->async_tx.cookie = -EBUSY;

	/* make the cyclic link */
	new->desc.ddadr = first->async_tx.phys;
	chan->cyclic_first = first;

	return &first->async_tx;

fail:
	if (first)
		mmp_pdma_free_desc_list(chan, &first->tx_list);
	return NULL;
}

static int mmp_pdma_config(struct dma_chan *dchan,
			   struct dma_slave_config *cfg)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);
	u32 maxburst = 0, addr = 0;
	enum dma_slave_buswidth width = DMA_SLAVE_BUSWIDTH_UNDEFINED;

	if (!dchan)
		return -EINVAL;

	if (cfg->direction == DMA_DEV_TO_MEM) {
		chan->dcmd = DCMD_INCTRGADDR | DCMD_FLOWSRC;
		maxburst = cfg->src_maxburst;
		width = cfg->src_addr_width;
		addr = cfg->src_addr;
	} else if (cfg->direction == DMA_MEM_TO_DEV) {
		chan->dcmd = DCMD_INCSRCADDR | DCMD_FLOWTRG;
		maxburst = cfg->dst_maxburst;
		width = cfg->dst_addr_width;
		addr = cfg->dst_addr;
	}

	if (width == DMA_SLAVE_BUSWIDTH_1_BYTE)
		chan->dcmd |= DCMD_WIDTH1;
	else if (width == DMA_SLAVE_BUSWIDTH_2_BYTES)
		chan->dcmd |= DCMD_WIDTH2;
	else if (width == DMA_SLAVE_BUSWIDTH_4_BYTES)
		chan->dcmd |= DCMD_WIDTH4;

	if (maxburst == 8)
		chan->dcmd |= DCMD_BURST8;
	else if (maxburst == 16)
		chan->dcmd |=