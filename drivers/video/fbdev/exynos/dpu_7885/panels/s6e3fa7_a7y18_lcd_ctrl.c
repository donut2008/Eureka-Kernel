t)
{
	struct sun4i_dma_promise *promise;

	promise = list_first_entry_or_null(&contract->demands,
					   struct sun4i_dma_promise, list);
	if (!promise) {
		list_splice_init(&contract->completed_demands,
				 &contract->demands);
		promise = list_first_entry(&contract->demands,
					   struct sun4i_dma_promise, list);
	}

	return promise;
}

/**
 * Free a contract and all its associated promises
 */
static void sun4i_dma_free_contract(struct virt_dma_desc *vd)
{
	struct sun4i_dma_contract *contract = to_sun4i_dma_contract(vd);
	struct sun4i_dma_promise *promise, *tmp;

	/* Free all the demands and completed demands */
	list_for_each_entry_safe(promise, tmp, &contract->demands, list)
		kfree(promise);

	list_for_each_entry_safe(promise, tmp, &contract->completed_demands, list)
		kfree(promise);

	kfree(contract);
}

static struct dma_async_tx_descriptor *
sun4i_dma_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest,
			  dma_addr_t src, size_t len, unsigned long flags)
{
	struct sun4i_dma_vchan *vchan = to_sun4i_dma_vchan(chan);
	struct dma_slave_config *sconfig = &vchan->cfg;
	struct sun4i_dma_promise *promise;
	struct sun4i_dma_contract *contract;

	contract = generate_dma_contract();
	if (!contract)
		return NULL;

	/*
	 * We can only do the copy to bus aligned addresses, so
	 * choose the best one so we get decent performance. We also
	 * maximize the burst size for this same reason.
	 */
	sconfig->src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	sconfig->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	sconfig->src_maxburst = 8;
	sconfig->dst_maxburst = 8;

	if (vchan->is_dedicated)
		promise = generate_ddma_promise(chan, src, dest, len, sconfig);
	else
		promise = generate_ndma_promise(chan, src, dest, len, sconfig,
						DMA_MEM_TO_MEM);

	if (!promise) {
		kfree(contract);
		return NULL;
	}

	/* Configure memcpy mode */
	if (vchan->is_dedicated) {
		promise->cfg |= SUN4I_DMA_CFG_SRC_DRQ_TYPE(SUN4I_DDMA_DRQ_TYPE_SDRAM) |
				SUN4I_DMA_CFG_DST_DRQ_TYPE(SUN4I_DDMA_DRQ_TYPE_SDRAM);
	} else {
		promise->cfg |= SUN4I_DMA_CFG_SRC_DRQ_TYPE(SUN4I_NDMA_DRQ_TYPE_SDRAM) |
				SUN4I_DMA_CFG_DST_DRQ_TYPE(SUN4I_NDMA_DRQ_TYPE_SDRAM);
	}

	/* Fill the contract with our only promise */
	list_add_tail(&promise->list, &contract->demands);

	/* And add it to the vchan */
	return vchan_tx_prep(&vchan->vc, &contract->vd, flags);
}

static struct dma_async_tx_descriptor *
sun4i_dma_prep_dma_cyclic(struct dma_chan *chan, dma_addr_t buf, size_t len,
			  size_t period_len, enum dma_transfer_direction dir,
			  unsigned long flags)
{
	struct sun4i_dma_vchan *vchan = to_sun4i_dma_vchan(chan);
	struct dma_slave_config *sconfig = &vchan->cfg;
	struct sun4i_dma_promise *promise;
	struct sun4i_dma_contract *contract;
	dma_addr_t src, dest;
	u32 endpoints;
	int nr_periods, offset, plength, i;

	if (!is_slave_direction(dir)) {
		dev_err(chan2dev(chan), "Invalid DMA direction\n");
		return NULL;
	}

	if (vchan->is_dedicated) {
		/*
		 * As we are using this just for audio data, we need to use
		 * normal DMA. There is nothing stopping us from supporting
		 * dedicated DMA here as well, so if a client comes up and
		 * requires it, it will be simple to implement it.
		 */
		dev_err(chan2dev(chan),
			"Cyclic transfers are only supported on Normal DMA\n");
		return NULL;
	}

	contract = generate_dma_contract();
	if (!contract)
		return NULL;

	contract->is_cyclic = 1;

	/* Figure out the endpoints and the address we need */
	if (dir == DMA_MEM_TO_DEV) {
		src = buf;
		dest = sconfig->dst_addr;
		endpoints = SUN4I_DMA_CFG_SRC_DRQ_TYPE(SUN4I_NDMA_DRQ_TYPE_SDRAM) |
			    SUN4I_DMA_CFG_DST_DRQ_TYPE(vchan->endpoint) |
			    SUN4I_DMA_CFG_DST_ADDR_MODE(SUN4I_NDMA_ADDR_MODE_IO);
	} else {
		src = sconfig->src_addr;
		dest = buf;
		endpoints = SUN4I_DMA_CFG_SRC_DRQ_TYPE(vchan->endpoint) |
			    SUN4I_DMA_CFG_SRC_ADDR_MODE(SUN4I_NDMA_ADDR_MODE_IO) |
			    SUN4I_DMA_CFG_DST_DRQ_TYPE(SUN4I_NDMA_DRQ_TYPE_SDRAM);
	}

	/*
	 * We will be using half done interrupts to make two periods
	 * out of a promise, so we need to program the DMA engine less
	 * often
	 */

	/*
	 * The engine can interrupt on half-transfer, so we can use
	 * this feature to program the engine half as often as if we
	 * didn't use it (keep in mind the hardware doesn't support
	 * linked lists).
	 *
	 * Say you have a set of periods (| marks the start/end, I for
	 * interrupt, P for programming the engine to do a new
	 * transfer), the easy but slow way would be to do
	 *
	 *  |---|---|---|---| (periods / promises)
	 *  P  I,P I,P I,P  I
	 *
	 * Using half transfer interrupts you can do
	 *
	 *  |-------|-------| (promises as configured on hw)
	 *  |---|---|---|---| (periods)
	 *  P   I  I,P  I   I
	 *
	 * Which requires half the engine programming for the same
	 * functionality.
	 */
	nr_periods = DIV_ROUND_UP(len / period_len, 2);
	for (i = 0; i < nr_periods; i++) {
		/* Calculate the offset in the buffer and the length needed */
		offset = i * period_len * 2;
		plength = min((len - offset), (period_len * 2));
		if (dir == DMA_MEM_TO_DEV)
			src = buf + offset;
		else
			dest = buf + offset;

		/* Make the promise */
		promise = generate_ndma_promise(chan, src, dest,
						plength, sconfig, dir);
		if (!promise) {
			/* TODO: should we free everything? */
			return NULL;
		}
		promise->cfg |= endpoints;

		/* Then add it to the contract */
		list_add_tail(&promise->list, &contract->demands);
	}

	/* And add it to the vchan */
	return vchan_tx_prep(&vchan->vc, &contract->vd, flags);
}

static struct dma_async_tx_descriptor *
sun4i_dma_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
			unsigned int sg_len, enum dma_transfer_direction dir,
			unsigned long flags, void *context)
{
	struct sun4i_dma_vchan *vchan = to_sun4i_dma_vchan(chan);
	struct dma_slave_config *sconfig = &vchan->cfg;
	struct sun4i_dma_promise *promise;
	struct sun4i_dma_contract *contract;
	u8 ram_type, io_mode, linear_mode;
	struct scatterlist *sg;
	dma_addr_t srcaddr, dstaddr;
	u32 endpoints, para;
	int i;

	if (!sgl)
		return NULL;

	if (!is_slave_direction(dir)) {
		dev_err(chan2dev(chan), "Invalid DMA direction\n");
		return NULL;
	}

	contract = generate_dma_contract();
	if (!contract)
		return NULL;

	if (vchan->is_dedicated) {
		io_mode = SUN4I_DDMA_ADDR_MODE_IO;
		linear_mode = SUN4I_DDMA_ADDR_MODE_LINEAR;
		ram_type = SUN4I_DDMA_DRQ_TYPE_SDRAM;
	} else {
		io_mode = SUN4I_NDMA_ADDR_MODE_IO;
		linear_mode = SUN4I_NDMA_ADDR_MODE_LINEAR;
		ram_type = SUN4I_NDMA_DRQ_TYPE_SDRAM;
	}

	if (dir == DMA_MEM_TO_DEV)
		endpoints = SUN4I_DMA_CFG_DST_DRQ_TYPE(vchan->endpoint) |
			    SUN4I_DMA_CFG_DST_ADDR_MODE(io_mode) |
			    SUN4I_DMA_CFG_SRC_DRQ_TYPE(ram_type) |
			    SUN4I_DMA_CFG_SRC_ADDR_MODE(linear_mode);
	else
		endpoints = SUN4I_DMA_CFG_DST_DRQ_TYPE(ram_type) |
			    SUN4I_DMA_CFG_DST_ADDR_MODE(linear_mode) |
			    SUN4I_DMA_CFG_SRC_DRQ_TYPE(vchan->endpoint) |
			    SUN4I_DMA_CFG_SRC_ADDR_MODE(io_mode);

	for_each_sg(sgl, sg, sg_len, i) {
		/* Figure out addresses */
		if (dir == DMA_MEM_TO_DEV) {
			srcaddr = sg_dma_address(sg);
			dstaddr = sconfig->dst_addr;
		} else {
			srcaddr = sconfig->src_addr;
			dstaddr = sg_dma_address(sg);
		}

		/*
		 * These are the magic DMA engine timings that keep SPI going.
		 * I haven't seen any interface on DMAEngine to configure
		 * timings, and so far they seem to work for everything we
		 * support, so I've kept them here. I don't know if other
		 * devices need different timings because, as usual, we only
		 * have the "para" bitfield meanings, but no comment on what
		 * the values should be when doing a certain operation :|
		 */
		para = SUN4I_DDMA_MAGIC_SPI_PARAMETERS;

		/* And make a suitable promise */
		if (vchan->is_dedicated)
			promise = generate_ddma_promise(chan, srcaddr, dstaddr,
							sg_dma_len(sg),
							sconfig);
		else
			promise = generate_ndma_promise(chan, srcaddr, dstaddr,
							sg_dma_len(sg),
							sconfig, dir);

		if (!promise)
			return NULL; /* TODO: should we free everything? */

		promise->cfg |= endpoints;
		promise->para = para;

		/* Then add it to the contract */
		list_add_tail(&promise->list, &contract->demands);
	}

	/*
	 * Once we've got all the promises ready, add the contract
	 * to the pending list on the vchan
	 */
	return vchan_tx_prep(&vchan->vc, &contract->vd, flags);
}

static int sun4i_dma_terminate_all(struct dma_chan *chan)
{
	struct sun4i_dma_dev *priv = to_sun4i_dma_dev(chan->device);
	struct sun4i_dma_vchan *vchan = to_sun4i_dma_vchan(chan);
	struct sun4i_dma_pchan *pchan = vchan->pchan;
	LIST_HEAD(head);
	unsigned long flags;

	spin_lock_irqsave(&vchan->vc.lock, flags);
	vchan_get_all_descriptors(&vchan->vc, &head);
	spin_unlock_irqrestore(&vchan->vc.lock, flags);

	/*
	 * Clearing the configuration register will halt the pchan. Interrupts
	 * may still trigger, so don't forget to disable them.
	 */
	if (pchan) {
		if (pchan->is_dedicated)
			writel(0, pchan->base + SUN4I_DDMA_CFG_REG);
		else
			writel(0, pchan->base + SUN4I_NDMA_CFG_REG);
		set_pchan_interrupt(priv, pchan, 0, 0);
		release_pchan(priv, pchan);
	}

	spin_lock_irqsave(&vchan->vc.lock, flags);
	vchan_dma_desc_free_list(&vchan->vc, &head);
	/* Clear these so the vchan is usable again */
	vchan->processing = NULL;
	vchan->pchan = NULL;
	spin_unlock_irqrestore(&vchan->vc.lock, flags);

	return 0;
}

static int sun4i_dma_config(struct dma_chan *chan,
			    struct dma_slave_config *config)
{
	struct sun4i_dma_vchan *vchan = to_sun4i_dma_vchan(chan);

	memcpy(&vchan->cfg, config, sizeof(*config));

	return 0;
}

static struct dma_chan *sun4i_dma_of_xlate(struct of_phandle_args *dma_spec,
					   struct of_dma *ofdma)
{
	struct sun4i_dma_dev *priv = ofdma->of_dma_data;
	struct sun4i_dma_vchan *vchan;
	struct dma_chan *chan;
	u8 is_dedicated = dma_spec->args[0];
	u8 endpoint = dma_spec->args[1];

	/* Check if type is Normal or Dedicated */
	if (is_dedicated != 0 && is_dedicated != 1)
		return NULL;

	/* Make sure the endpoint looks sane */
	if ((is_dedicated && endpoint >= SUN4I_DDMA_DRQ_TYPE_LIMIT) ||
	    (!is_dedicated && endpoint >= SUN4I_NDMA_DRQ_TYPE_LIMIT))
		return NULL;

	chan = dma_get_any_slave_channel(&priv->slave);
	if (!chan)
		return NULL;

	/* Assign the endpoint to the vchan */
	vchan = to_sun4i_dma_vchan(chan);
	vchan->is_dedicated = is_dedicated;
	vchan->endpoint = endpoint;

	return chan;
}

static enum dma_status sun4i_dma_tx_status(struct dma_chan *chan,
					   dma_cookie_t cookie,
					   struct dma_tx_state *state)
{
	struct sun4i_dma_vchan *vchan = to_sun4i_dma_vchan(chan);
	struct sun4i_dma_pchan *pchan = vchan->pchan;
	struct sun4i_dma_contract *contract;
	struct sun4i_dma_promise *promise;
	struct virt_dma_desc *vd;
	unsigned long flags;
	enum dma_status ret;
	size_t bytes = 0;

	ret = dma_cookie_status(chan, cookie, state);
	if (!state || (ret == DMA_COMPLETE))
		return ret;

	spin_lock_irqsave(&vchan->vc.lock, flags);
	vd = vchan_find_desc(&vchan->vc, cookie);
	if (!vd)
		goto exit;
	contract = to_sun4i_dma_contract(vd);

	list_for_each_entry(promise, &contract->demands, list)
		bytes += promise->len;

	/*
	 * The hardware is configured to return the remaining byte
	 * quantity. If possible, replace the first listed element's
	 * full size with the actual remaining amount
	 */
	promise = list_first_entry_or_null(&contract->demands,
					   struct sun4i_dma_promise, list);
	if (promise && pchan) {
		bytes -= promise->len;
		if (pchan->is_dedicated)
			bytes += readl(pchan->base + SUN4I_DDMA_BYTE_COUNT_REG);
		else
			bytes += readl(pchan->base + SUN4I_NDMA_BYTE_COUNT_REG);
	}

exit:

	dma_set_residue(state, bytes);
	spin_unlock_irqrestore(&vchan->vc.lock, flags);

	return ret;
}

static void sun4i_dma_issue_pending(struct dma_chan *chan)
{
	struct sun4i_dma_dev *priv = to_sun4i_dma_dev(chan->device);
	struct sun4i_dma_vchan *vchan = to_sun4i_dma_vchan(chan);
	unsigned long flags;

	spin_lock_irqsave(&vchan->vc.lock, flags);

	/*
	 * If there are pending transactions for this vchan, push one of
	 * them into the engine to get the ball rolling.
	 */
	if (vchan_issue_pending(&vchan->vc))
		__execute_vchan_pending(priv, vchan);

	spin_unlock_irqrestore(&vchan->vc.lock, flags);
}

static irqreturn_t sun4i_dma_interrupt(int irq, void *dev_id)
{
	struct sun4i_dma_dev *priv = dev_id;
	struct sun4i_dma_pchan *pchans = priv->pchans, *pchan;
	struct sun4i_dma_vchan *vchan;
	struct sun4i_dma_contract *contract;
	struct sun4i_dma_promise *promise;
	unsigned long pendirq, irqs, disableirqs;
	int bit, i, free_room, allow_mitigation = 1;

	pendirq = readl_relaxed(priv->base + SUN4I_DMA_IRQ_PENDING_STATUS_REG);

handle_pending:

	disableirqs = 0;
	free_room = 0;

	for_each_set_bit(bit, &pendirq, 32) {
		pchan = &pchans[bit >> 1];
		vchan = pchan->vchan;
		if (!vchan) /* a terminated channel may still interrupt */
			continue;
		contract = vchan->contract;

		/*
		 * Disable the IRQ and free the pchan if it's an end
		 * interrupt (odd bit)
		 */
		if (bit & 1) {
			spin_lock(&vchan->vc.lock);

			/*
			 * Move the promise into the completed list now that
			 * we're done with it
			 */
			list_del(&vchan->processing->list);
			list_add_tail(&vchan->processing->list,
				      &contract->completed_demands);

			/*
			 * Cyclic DMA transfers are special:
			 * - There's always something we can dispatch
			 * - We need to run the callback
			 * - Latency is very important, as this is used by audio
			 * We therefore just cycle through the list and dispatch
			 * whatever we have here, reusing the pchan. There's
			 * no need to run the thread after this.
			 *
			 * For non-cyclic transfers we need to look around,
			 * so we can program some more work, or notify the
			 * client that their transfers have been completed.
			 */
			if (contract->is_cyclic) {
				promise = get_next_cyclic_promise(contract);
				vchan->processing = promise;
				configure_pchan(pchan, promise);
				vchan_cyclic_callback(&contract->vd);
			} else {
				vchan->processing = NULL;
				vchan->pchan = NULL;

				free_room = 1;
				disableirqs |= BIT(bit);
				release_pchan(priv, pchan);
			}

			spin_unlock(&vchan->vc.lock);
		} else {
			/* Half done interrupt */
			if (contract->is_cyclic)
				vchan_cyclic_callback(&contract->vd);
			else
				disableirqs |= BIT(bit);
		}
	}

	/* Disable the IRQs for events we handled */
	spin_lock(&priv->lock);
	irqs = readl_relaxed(priv->base + SUN4I_DMA_IRQ_ENABLE_REG);
	writel_relaxed(irqs & ~disableirqs,
		       priv->base + SUN4I_DMA_IRQ_ENABLE_REG);
	spin_unlock(&priv->lock);

	/* Writing 1 to the pending field will clear the pending interrupt */
	writel_relaxed(pendirq, priv->base + SUN4I_DMA_IRQ_PENDING_STATUS_REG);

	/*
	 * If a pchan was freed, we may be able to schedule something else,
	 * so have a look around
	 */
	if (free_room) {
		for (i = 0; i < SUN4I_DMA_NR_MAX_VCHANS; i++) {
			vchan = &priv->vchans[i];
			spin_lock(&vchan->vc.lock);
			__execute_vchan_pending(priv, vchan);
			spin_unlock(&vchan->vc.lock);
		}
	}

	/*
	 * Handle newer interrupts if some showed up, but only do it once
	 * to avoid a too long a loop
	 */
	if (allow_mitigation) {
		pendirq = readl_relaxed(priv->base +
					SUN4I_DMA_IRQ_PENDING_STATUS_REG);
		if (pendirq) {
			allow_mitigation = 0;
			goto handle_pending;
		}
	}

	return IRQ_HANDLED;
}

static int sun4i_dma_probe(struct platform_device *pdev)
{
	struct sun4i_dma_dev *priv;
	struct resource *res;
	int i, j, ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
		return priv->irq;
	}

	priv->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(priv->clk)) {
		dev_err(&pdev->dev, "No clock specified\n");
		return PTR_ERR(priv->clk);
	}

	platform_set_drvdata(pdev, priv);
	spin_lock_init(&priv->lock);

	dma_cap_zero(priv->slave.cap_mask);
	dma_cap_set(DMA_PRIVATE, priv->slave.cap_mask);
	dma_cap_set(DMA_MEMCPY, priv->slave.cap_mask);
	dma_cap_set(DMA_CYCLIC, priv->slave.cap_mask);
	dma_cap_set(DMA_SLAVE, priv->slave.cap_mask);

	INIT_LIST_HEAD(&priv->slave.channels);
	priv->slave.device_free_chan_resources	= sun4i_dma_free_chan_resources;
	priv->slave.device_tx_status		= sun4i_dma_tx_status;
	priv->slave.device_issue_pending	= sun4i_dma_issue_pending;
	priv->slave.device_prep_slave_sg	= sun4i_dma_prep_slave_sg;
	priv->slave.device_prep_dma_memcpy	= sun4i_dma_prep_dma_memcpy;
	priv->slave.device_prep_dma_cyclic	= sun4i_dma_prep_dma_cyclic;
	priv->slave.device_config		= sun4i_dma_config;
	priv->slave.device_terminate_all	= sun4i_dma_terminate_all;
	priv->slave.copy_align			= 2;
	priv->slave.src_addr_widths		= BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
						  BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) |
						  BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
	priv->slave.dst_addr_widths		= BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
						  BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) |
						  BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
	priv->slave.directions			= BIT(DMA_DEV_TO_MEM) |
						  BIT(DMA_MEM_TO_DEV);
	priv->slave.residue_granularity		= DMA_RESIDUE_GRANULARITY_BURST;

	priv->slave.dev = &pdev->dev;

	priv->pchans = devm_kcalloc(&pdev->dev, SUN4I_DMA_NR_MAX_CHANNELS,
				    sizeof(struct sun4i_dma_pchan), GFP_KERNEL);
	priv->vchans = devm_kcalloc(&pdev->dev, SUN4I_DMA_NR_MAX_VCHANS,
				    sizeof(struct sun4i_dma_vchan), GFP_KERNEL);
	if (!priv->vchans || !priv->pchans)
		return -ENOMEM;

	/*
	 * [0..SUN4I_NDMA_NR_MAX_CHANNELS) are normal pchans, and
	 * [SUN4I_NDMA_NR_MAX_CHANNELS..SUN4I_DMA_NR_MAX_CHANNELS) are
	 * dedicated ones
	 */
	for (i = 0; i < SUN4I_NDMA_NR_MAX_CHANNELS; i++)
		priv->pchans[i].base = priv->base +
			SUN4I_NDMA_CHANNEL_REG_BASE(i);

	for (j = 0; i < SUN4I_DMA_NR_MAX_CHANNELS; i++, j++) {
		priv->pchans[i].base = priv->base +
			SUN4I_DDMA_CHANNEL_REG_BASE(j);
		priv->pchans[i].is_dedicated = 1;
	}

	for (i = 0; i < SUN4I_DMA_NR_MAX_VCHANS; i++) {
		struct sun4i_dma_vchan *vchan = &priv->vchans[i];

		spin_lock_init(&vchan->vc.lock);
		vchan->vc.desc_free = sun4i_dma_free_contract;
		vchan_init(&vchan->vc, &priv->slave);
	}

	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't enable the clock\n");
		return ret;
	}

	/*
	 * Make sure the IRQs are all disabled and accounted for. The bootloader
	 * likes to leave these dirty
	 */
	writel(0, priv->base + SUN4I_DMA_IRQ_ENABLE_REG);
	writel(0xFFFFFFFF, priv->base + SUN4I_DMA_IRQ_PENDING_STATUS_REG);

	ret = devm_request_irq(&pdev->dev, priv->irq, sun4i_dma_interrupt,
			       0, dev_name(&pdev->dev), priv);
	if (ret) {
		dev_err(&pdev->dev, "Cannot request IRQ\n");
		goto err_clk_disable;
	}

	ret = dma_async_device_register(&priv->slave);
	if (ret) {
		dev_warn(&pdev->dev, "Failed to register DMA engine device\n");
		goto err_clk_disable;
	}

	ret = of_dma_controller_register(pdev->dev.of_node, sun4i_dma_of_xlate,
					 priv);
	if (ret) {
		dev_err(&pdev->dev, "of_dma_controller_register failed\n");
		goto err_dma_unregister;
	}

	dev_dbg(&pdev->dev, "Successfully probed SUN4I_DMA\n");

	return 0;

err_dma_unregister:
	dma_async_device_unregister(&priv->slave);
err_clk_disable:
	clk_disable_unprepare(priv->clk);
	return ret;
}

static int sun4i_dma_remove(struct platform_device *pdev)
{
	struct sun4i_dma_dev *priv = platform_get_drvdata(pdev);

	/* Disable IRQ so no more work is scheduled */
	disable_irq(priv->irq);

	of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&priv->slave);

	clk_disable_unprepare(priv->clk);

	return 0;
}

static const struct of_device_id sun4i_dma_match[] = {
	{ .compatible = "allwinner,sun4i-a10-dma" },
	{ /* sentinel */ },
};

static struct platform_driver sun4i_dma_driver = {
	.probe	= sun4i_dma_probe,
	.remove	= sun4i_dma_remove,
	.driver	= {
		.name		= "sun4i-dma",
		.of_match_table	= sun4i_dma_match,
	},
};

module_platform_driver(sun4i_dma_driver);

MODULE_DESCRIPTION("Allwinner A10 Dedicated DMA Controller Driver");
MODULE_AUTHOR("Emilio López <emilio@elopez.com.ar>");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                     /*
 * Copyright (C) 2013-2014 Allwinner Tech Co., Ltd
 * Author: Sugar <shuge@allwinnertech.com>
 *
 * Copyright (C) 2014 Maxime Ripard
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_dma.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "virt-dma.h"

/*
 * Common registers
 */
#define DMA_IRQ_EN(x)		((x) * 0x04)
#define DMA_IRQ_HALF			BIT(0)
#define DMA_IRQ_PKG			BIT(1)
#define DMA_IRQ_QUEUE			BIT(2)

#define DMA_IRQ_CHAN_NR			8
#define DMA_IRQ_CHAN_WIDTH		4


#define DMA_IRQ_STAT(x)		((x) * 0x04 + 0x10)

#define DMA_STAT		0x30

/*
 * sun8i specific registers
 */
#define SUN8I_DMA_GATE		0x20
#define SUN8I_DMA_GATE_ENABLE	0x4

/*
 * Channels specific registers
 */
#define DMA_CHAN_ENABLE		0x00
#define DMA_CHAN_ENABLE_START		BIT(0)
#define DMA_CHAN_ENABLE_STOP		0

#define DMA_CHAN_PAUSE		0x04
#define DMA_CHAN_PAUSE_PAUSE		BIT(1)
#define DMA_CHAN_PAUSE_RESUME		0

#define DMA_CHAN_LLI_ADDR	0x08

#define DMA_CHAN_CUR_CFG	0x0c
#define DMA_CHAN_CFG_SRC_DRQ(x)		((x) & 0x1f)
#define DMA_CHAN_CFG_SRC_IO_MODE	BIT(5)
#define DMA_CHAN_CFG_SRC_LINEAR_MODE	(0 << 5)
#define DMA_CHAN_CFG_SRC_BURST(x)	(((x) & 0x3) << 7)
#define DMA_CHAN_CFG_SRC_WIDTH(x)	(((x) & 0x3) << 9)

#define DMA_CHAN_CFG_DST_DRQ(x)		(DMA_CHAN_CFG_SRC_DRQ(x) << 16)
#define DMA_CHAN_CFG_DST_IO_MODE	(DMA_CHAN_CFG_SRC_IO_MODE << 16)
#define DMA_CHAN_CFG_DST_LINEAR_MODE	(DMA_CHAN_CFG_SRC_LINEAR_MODE << 16)
#define DMA_CHAN_CFG_DST_BURST(x)	(DMA_CHAN_CFG_SRC_BURST(x) << 16)
#define DMA_CHAN_CFG_DST_WIDTH(x)	(DMA_CHAN_CFG_SRC_WIDTH(x) << 16)

#define DMA_CHAN_CUR_SRC	0x10

#define DMA_CHAN_CUR_DST	0x14

#define DMA_CHAN_CUR_CNT	0x18

#define DMA_CHAN_CUR_PARA	0x1c


/*
 * Various hardware related defines
 */
#define LLI_LAST_ITEM	0xfffff800
#define NORMAL_WAIT	8
#define DRQ_SDRAM	1

/*
 * Hardware channels / ports representation
 *
 * The hardware is used in several SoCs, with differing numbers
 * of channels and endpoints. This structure ties those numbers
 * to a certain compatible string.
 */
struct sun6i_dma_config {
	u32 nr_max_channels;
	u32 nr_max_requests;
	u32 nr_max_vchans;
};

/*
 * Hardware representation of the LLI
 *
 * The hardware will be fed the physical address of this structure,
 * and read its content in order to start the transfer.
 */
struct sun6i_dma_lli {
	u32			cfg;
	u32			src;
	u32			dst;
	u32			len;
	u32			para;
	u32			p_lli_next;

	/*
	 * This field is not used by the DMA controller, but will be
	 * used by the CPU to go through the list (mostly for dumping
	 * or freeing it).
	 */
	struct sun6i_dma_lli	*v_lli_next;
};


struct sun6i_desc {
	struct virt_dma_desc	vd;
	dma_addr_t		p_lli;
	struct sun6i_dma_lli	*v_lli;
};

struct sun6i_pchan {
	u32			idx;
	void __iomem		*base;
	struct sun6i_vchan	*vchan;
	struct sun6i_desc	*desc;
	struct sun6i_desc	*done;
};

struct sun6i_vchan {
	struct virt_dma_chan	vc;
	struct list_head	node;
	struct dma_slave_config	cfg;
	struct sun6i_pchan	*phy;
	u8			port;
};

struct sun6i_dma_dev {
	struct dma_device	slave;
	void __iomem		*base;
	struct clk		*clk;
	int			irq;
	spinlock_t		lock;
	struct reset_control	*rstc;
	struct tasklet_struct	task;
	atomic_t		tasklet_shutdown;
	struct list_head	pending;
	struct dma_pool		*pool;
	struct sun6i_pchan	*pchans;
	struct sun6i_vchan	*vchans;
	const struct sun6i_dma_config *cfg;
};

static struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static inline struct sun6i_dma_dev *to_sun6i_dma_dev(struct dma_device *d)
{
	return container_of(d, struct sun6i_dma_dev, slave);
}

static inline struct sun6i_vchan *to_sun6i_vchan(struct dma_chan *chan)
{
	return container_of(chan, struct sun6i_vchan, vc.chan);
}

static inline struct sun6i_desc *
to_sun6i_desc(struct dma_async_tx_descriptor *tx)
{
	return container_of(tx, struct sun6i_desc, vd.tx);
}

static inline void sun6i_dma_dump_com_regs(struct sun6i_dma_dev *sdev)
{
	dev_dbg(sdev->slave.dev, "Common register:\n"
		"\tmask0(%04x): 0x%08x\n"
		"\tmask1(%04x): 0x%08x\n"
		"\tpend0(%04x): 0x%08x\n"
		"\tpend1(%04x): 0x%08x\n"
		"\tstats(%04x): 0x%08x\n",
		DMA_IRQ_EN(0), readl(sdev->base + DMA_IRQ_EN(0)),
		DMA_IRQ_EN(1), readl(sdev->base + DMA_IRQ_EN(1)),
		DMA_IRQ_STAT(0), readl(sdev->base + DMA_IRQ_STAT(0)),
		DMA_IRQ_STAT(1), readl(sdev->base + DMA_IRQ_STAT(1)),
		DMA_STAT, readl(sdev->base + DMA_STAT));
}

static inline void sun6i_dma_dump_chan_regs(struct sun6i_dma_dev *sdev,
					    struct sun6i_pchan *pchan)
{
	phys_addr_t reg = virt_to_phys(pchan->base);

	dev_dbg(sdev->slave.dev, "Chan %d reg: %pa\n"
		"\t___en(%04x): \t0x%08x\n"
		"\tpause(%04x): \t0x%08x\n"
		"\tstart(%04x): \t0x%08x\n"
		"\t__cfg(%04x): \t0x%08x\n"
		"\t__src(%04x): \t0x%08x\n"
		"\t__dst(%04x): \t0x%08x\n"
		"\tcount(%04x): \t0x%08x\n"
		"\t_para(%04x): \t0x%08x\n\n",
		pchan->idx, &reg,
		DMA_CHAN_ENABLE,
		readl(pchan->base + DMA_CHAN_ENABLE),
		DMA_CHAN_PAUSE,
		readl(pchan->base + DMA_CHAN_PAUSE),
		DMA_CHAN_LLI_ADDR,
		readl(pchan->base + DMA_CHAN_LLI_ADDR),
		DMA_CHAN_CUR_CFG,
		readl(pchan->base + DMA_CHAN_CUR_CFG),
		DMA_CHAN_CUR_SRC,
		readl(pchan->base + DMA_CHAN_CUR_SRC),
		DMA_CHAN_CUR_DST,
		readl(pchan->base + DMA_CHAN_CUR_DST),
		DMA_CHAN_CUR_CNT,
		readl(pchan->base + DMA_CHAN_CUR_CNT),
		DMA_CHAN_CUR_PARA,
		readl(pchan->base + DMA_CHAN_CUR_PARA));
}

static inline s8 convert_burst(u32 maxburst)
{
	switch (maxburst) {
	case 1:
		return 0;
	case 8:
		return 2;
	default:
		return -EINVAL;
	}
}

static inline s8 convert_buswidth(enum dma_slave_buswidth addr_width)
{
	if ((addr_width < DMA_SLAVE_BUSWIDTH_1_BYTE) ||
	    (addr_width > DMA_SLAVE_BUSWIDTH_4_BYTES))
		return -EINVAL;

	return addr_width >> 1;
}

static void *sun6i_dma_lli_add(struct sun6i_dma_lli *prev,
			       struct sun6i_dma_lli *next,
			       dma_addr_t next_phy,
			       struct sun6i_desc *txd)
{
	if ((!prev && !txd) || !next)
		return NULL;

	if (!prev) {
		txd->p_lli = next_phy;
		txd->v_lli = next;
	} else {
		prev->p_lli_next = next_phy;
		prev->v_lli_next = next;
	}

	next->p_lli_next = LLI_LAST_ITEM;
	next->v_lli_next = NULL;

	return next;
}

static inline int sun6i_dma_cfg_lli(struct sun6i_dma_lli *lli,
				    dma_addr_t src,
				    dma_addr_t dst, u32 len,
				    struct dma_slave_config *config)
{
	u8 src_width, dst_width, src_burst, dst_burst;

	if (!config)
		return -EINVAL;

	src_burst = convert_burst(config->src_maxburst);
	if (src_burst)
		return src_burst;

	dst_burst = convert_burst(config->dst_maxburst);
	if (dst_burst)
		return dst_burst;

	src_width = convert_buswidth(config->src_addr_width);
	if (src_width)
		return src_width;

	dst_width = convert_buswidth(config->dst_addr_width);
	if (dst_width)
		return dst_width;

	lli->cfg = DMA_CHAN_CFG_SRC_BURST(src_burst) |
		DMA_CHAN_CFG_SRC_WIDTH(src_width) |
		DMA_CHAN_CFG_DST_BURST(dst_burst) |
		DMA_CHAN_CFG_DST_WIDTH(dst_width);

	lli->src = src;
	lli->dst = dst;
	lli->len = len;
	lli->para = NORMAL_WAIT;

	return 0;
}

static inline void sun6i_dma_dump_lli(struct sun6i_vchan *vchan,
				      struct sun6i_dma_lli *lli)
{
	phys_addr_t p_lli = virt_to_phys(lli);

	dev_dbg(chan2dev(&vchan->vc.chan),
		"\n\tdesc:   p - %pa v - 0x%p\n"
		"\t\tc - 0x%08x s - 0x%08x d - 0x%08x\n"
		"\t\tl - 0x%08x p - 0x%08x n - 0x%08x\n",
		&p_lli, lli,
		lli->cfg, lli->src, lli->dst,
		lli->len, lli->para, lli->p_lli_next);
}

static void sun6i_dma_free_desc(struct virt_dma_desc *vd)
{
	struct sun6i_desc *txd = to_sun6i_desc(&vd->tx);
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(vd->tx.chan->device);
	struct sun6i_dma_lli *v_lli, *v_next;
	dma_addr_t p_lli, p_next;

	if (unlikely(!txd))
		return;

	p_lli = txd->p_lli;
	v_lli = txd->v_lli;

	while (v_lli) {
		v_next = v_lli->v_lli_next;
		p_next = v_lli->p_lli_next;

		dma_pool_free(sdev->pool, v_lli, p_lli);

		v_lli = v_next;
		p_lli = p_next;
	}

	kfree(txd);
}

static int sun6i_dma_start_desc(struct sun6i_vchan *vchan)
{
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(vchan->vc.chan.device);
	struct virt_dma_desc *desc = vchan_next_desc(&vchan->vc);
	struct sun6i_pchan *pchan = vchan->phy;
	u32 irq_val, irq_reg, irq_offset;

	if (!pchan)
		return -EAGAIN;

	if (!desc) {
		pchan->desc = NULL;
		pchan->done = NULL;
		return -EAGAIN;
	}

	list_del(&desc->node);

	pchan->desc = to_sun6i_desc(&desc->tx);
	pchan->done = NULL;

	sun6i_dma_dump_lli(vchan, pchan->desc->v_lli);

	irq_reg = pchan->idx / DMA_IRQ_CHAN_NR;
	irq_offset = pchan->idx % DMA_IRQ_CHAN_NR;

	irq_val = readl(sdev->base + DMA_IRQ_EN(irq_offset));
	irq_val |= DMA_IRQ_QUEUE << (irq_offset * DMA_IRQ_CHAN_WIDTH);
	writel(irq_val, sdev->base + DMA_IRQ_EN(irq_offset));

	writel(pchan->desc->p_lli, pchan->base + DMA_CHAN_LLI_ADDR);
	writel(DMA_CHAN_ENABLE_START, pchan->base + DMA_CHAN_ENABLE);

	sun6i_dma_dump_com_regs(sdev);
	sun6i_dma_dump_chan_regs(sdev, pchan);

	return 0;
}

static void sun6i_dma_tasklet(unsigned long data)
{
	struct sun6i_dma_dev *sdev = (struct sun6i_dma_dev *)data;
	const struct sun6i_dma_config *cfg = sdev->cfg;
	struct sun6i_vchan *vchan;
	struct sun6i_pchan *pchan;
	unsigned int pchan_alloc = 0;
	unsigned int pchan_idx;

	list_for_each_entry(vchan, &sdev->slave.channels, vc.chan.device_node) {
		spin_lock_irq(&vchan->vc.lock);

		pchan = vchan->phy;

		if (pchan && pchan->done) {
			if (sun6i_dma_start_desc(vchan)) {
				/*
				 * No current txd associated with this channel
				 */
				dev_dbg(sdev->slave.dev, "pchan %u: free\n",
					pchan->idx);

				/* Mark this channel free */
				vchan->phy = NULL;
				pchan->vchan = NULL;
			}
		}
		spin_unlock_irq(&vchan->vc.lock);
	}

	spin_lock_irq(&sdev->lock);
	for (pchan_idx = 0; pchan_idx < cfg->nr_max_channels; pchan_idx++) {
		pchan = &sdev->pchans[pchan_idx];

		if (pchan->vchan || list_empty(&sdev->pending))
			continue;

		vchan = list_first_entry(&sdev->pending,
					 struct sun6i_vchan, node);

		/* Remove from pending channels */
		list_del_init(&vchan->node);
		pchan_alloc |= BIT(pchan_idx);

		/* Mark this channel allocated */
		pchan->vchan = vchan;
		vchan->phy = pchan;
		dev_dbg(sdev->slave.dev, "pchan %u: alloc vchan %p\n",
			pchan->idx, &vchan->vc);
	}
	spin_unlock_irq(&sdev->lock);

	for (pchan_idx = 0; pchan_idx < cfg->nr_max_channels; pchan_idx++) {
		if (!(pchan_alloc & BIT(pchan_idx)))
			continue;

		pchan = sdev->pchans + pchan_idx;
		vchan = pchan->vchan;
		if (vchan) {
			spin_lock_irq(&vchan->vc.lock);
			sun6i_dma_start_desc(vchan);
			spin_unlock_irq(&vchan->vc.lock);
		}
	}
}

static irqreturn_t sun6i_dma_interrupt(int irq, void *dev_id)
{
	struct sun6i_dma_dev *sdev = dev_id;
	struct sun6i_vchan *vchan;
	struct sun6i_pchan *pchan;
	int i, j, ret = IRQ_NONE;
	u32 status;

	for (i = 0; i < sdev->cfg->nr_max_channels / DMA_IRQ_CHAN_NR; i++) {
		status = readl(sdev->base + DMA_IRQ_STAT(i));
		if (!status)
			continue;

		dev_dbg(sdev->slave.dev, "DMA irq status %s: 0x%x\n",
			i ? "high" : "low", status);

		writel(status, sdev->base + DMA_IRQ_STAT(i));

		for (j = 0; (j < DMA_IRQ_CHAN_NR) && status; j++) {
			if (status & DMA_IRQ_QUEUE) {
				pchan = sdev->pchans + j;
				vchan = pchan->vchan;

				if (vchan) {
					spin_lock(&vchan->vc.lock);
					vchan_cookie_complete(&pchan->desc->vd);
					pchan->done = pchan->desc;
					spin_unlock(&vchan->vc.lock);
				}
			}

			status = status >> DMA_IRQ_CHAN_WIDTH;
		}

		if (!atomic_read(&sdev->tasklet_shutdown))
			tasklet_schedule(&sdev->task);
		ret = IRQ_HANDLED;
	}

	return ret;
}

static struct dma_async_tx_descriptor *sun6i_dma_prep_dma_memcpy(
		struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
		size_t len, unsigned long flags)
{
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(chan->device);
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);
	struct sun6i_dma_lli *v_lli;
	struct sun6i_desc *txd;
	dma_addr_t p_lli;
	s8 burst, width;

	dev_dbg(chan2dev(chan),
		"%s; chan: %d, dest: %pad, src: %pad, len: %zu. flags: 0x%08lx\n",
		__func__, vchan->vc.chan.chan_id, &dest, &src, len, flags);

	if (!len)
		return NULL;

	txd = kzalloc(sizeof(*txd), GFP_NOWAIT);
	if (!txd)
		return NULL;

	v_lli = dma_pool_alloc(sdev->pool, GFP_NOWAIT, &p_lli);
	if (!v_lli) {
		dev_err(sdev->slave.dev, "Failed to alloc lli memory\n");
		goto err_txd_free;
	}

	v_lli->src = src;
	v_lli->dst = dest;
	v_lli->len = len;
	v_lli->para = NORMAL_WAIT;

	burst = convert_burst(8);
	width = convert_buswidth(DMA_SLAVE_BUSWIDTH_4_BYTES);
	v_lli->cfg |= DMA_CHAN_CFG_SRC_DRQ(DRQ_SDRAM) |
		DMA_CHAN_CFG_DST_DRQ(DRQ_SDRAM) |
		DMA_CHAN_CFG_DST_LINEAR_MODE |
		DMA_CHAN_CFG_SRC_LINEAR_MODE |
		DMA_CHAN_CFG_SRC_BURST(burst) |
		DMA_CHAN_CFG_SRC_WIDTH(width) |
		DMA_CHAN_CFG_DST_BURST(burst) |
		DMA_CHAN_CFG_DST_WIDTH(width);

	sun6i_dma_lli_add(NULL, v_lli, p_lli, txd);

	sun6i_dma_dump_lli(vchan, v_lli);

	return vchan_tx_prep(&vchan->vc, &txd->vd, flags);

err_txd_free:
	kfree(txd);
	return NULL;
}

static struct dma_async_tx_descriptor *sun6i_dma_prep_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction dir,
		unsigned long flags, void *context)
{
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(chan->device);
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);
	struct dma_slave_config *sconfig = &vchan->cfg;
	struct sun6i_dma_lli *v_lli, *prev = NULL;
	struct sun6i_desc *txd;
	struct scatterlist *sg;
	dma_addr_t p_lli;
	int i, ret;

	if (!sgl)
		return NULL;

	if (!is_slave_direction(dir)) {
		dev_err(chan2dev(chan), "Invalid DMA direction\n");
		return NULL;
	}

	txd = kzalloc(sizeof(*txd), GFP_NOWAIT);
	if (!txd)
		return NULL;

	for_each_sg(sgl, sg, sg_len, i) {
		v_lli = dma_pool_alloc(sdev->pool, GFP_NOWAIT, &p_lli);
		if (!v_lli)
			goto err_lli_free;

		if (dir == DMA_MEM_TO_DEV) {
			ret = sun6i_dma_cfg_lli(v_lli, sg_dma_address(sg),
						sconfig->dst_addr, sg_dma_len(sg),
						sconfig);
			if (ret)
				goto err_cur_lli_free;

			v_lli->cfg |= DMA_CHAN_CFG_DST_IO_MODE |
				DMA_CHAN_CFG_SRC_LINEAR_MODE |
				DMA_CHAN_CFG_SRC_DRQ(DRQ_SDRAM) |
				DMA_CHAN_CFG_DST_DRQ(vchan->port);

			dev_dbg(chan2dev(chan),
				"%s; chan: %d, dest: %pad, src: %pad, len: %u. flags: 0x%08lx\n",
				__func__, vchan->vc.chan.chan_id,
				&sconfig->dst_addr, &sg_dma_address(sg),
				sg_dma_len(sg), flags);

		} else {
			ret = sun6i_dma_cfg_lli(v_lli, sconfig->src_addr,
						sg_dma_address(sg), sg_dma_len(sg),
						sconfig);
			if (ret)
				goto err_cur_lli_free;

			v_lli->cfg |= DMA_CHAN_CFG_DST_LINEAR_MODE |
				DMA_CHAN_CFG_SRC_IO_MODE |
				DMA_CHAN_CFG_DST_DRQ(DRQ_SDRAM) |
				DMA_CHAN_CFG_SRC_DRQ(vchan->port);

			dev_dbg(chan2dev(chan),
				"%s; chan: %d, dest: %pad, src: %pad, len: %u. flags: 0x%08lx\n",
				__func__, vchan->vc.chan.chan_id,
				&sg_dma_address(sg), &sconfig->src_addr,
				sg_dma_len(sg), flags);
		}

		prev = sun6i_dma_lli_add(prev, v_lli, p_lli, txd);
	}

	dev_dbg(chan2dev(chan), "First: %pad\n", &txd->p_lli);
	for (prev = txd->v_lli; prev; prev = prev->v_lli_next)
		sun6i_dma_dump_lli(vchan, prev);

	return vchan_tx_prep(&vchan->vc, &txd->vd, flags);

err_cur_lli_free:
	dma_pool_free(sdev->pool, v_lli, p_lli);
err_lli_free:
	for (prev = txd->v_lli; prev; prev = prev->v_lli_next)
		dma_pool_free(sdev->pool, prev, virt_to_phys(prev));
	kfree(txd);
	return NULL;
}

static int sun6i_dma_config(struct dma_chan *chan,
			    struct dma_slave_config *config)
{
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);

	memcpy(&vchan->cfg, config, sizeof(*config));

	return 0;
}

static int sun6i_dma_pause(struct dma_chan *chan)
{
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(chan->device);
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);
	struct sun6i_pchan *pchan = vchan->phy;

	dev_dbg(chan2dev(chan), "vchan %p: pause\n", &vchan->vc);

	if (pchan) {
		writel(DMA_CHAN_PAUSE_PAUSE,
		       pchan->base + DMA_CHAN_PAUSE);
	} else {
		spin_lock(&sdev->lock);
		list_del_init(&vchan->node);
		spin_unlock(&sdev->lock);
	}

	return 0;
}

static int sun6i_dma_resume(struct dma_chan *chan)
{
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(chan->device);
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);
	struct sun6i_pchan *pchan = vchan->phy;
	unsigned long flags;

	dev_dbg(chan2dev(chan), "vchan %p: resume\n", &vchan->vc);

	spin_lock_irqsave(&vchan->vc.lock, flags);

	if (pchan) {
		writel(DMA_CHAN_PAUSE_RESUME,
		       pchan->base + DMA_CHAN_PAUSE);
	} else if (!list_empty(&vchan->vc.desc_issued)) {
		spin_lock(&sdev->lock);
		list_add_tail(&vchan->node, &sdev->pending);
		spin_unlock(&sdev->lock);
	}

	spin_unlock_irqrestore(&vchan->vc.lock, flags);

	return 0;
}

static int sun6i_dma_terminate_all(struct dma_chan *chan)
{
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(chan->device);
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);
	struct sun6i_pchan *pchan = vchan->phy;
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock(&sdev->lock);
	list_del_init(&vchan->node);
	spin_unlock(&sdev->lock);

	spin_lock_irqsave(&vchan->vc.lock, flags);

	vchan_get_all_descriptors(&vchan->vc, &head);

	if (pchan) {
		writel(DMA_CHAN_ENABLE_STOP, pchan->base + DMA_CHAN_ENABLE);
		writel(DMA_CHAN_PAUSE_RESUME, pchan->base + DMA_CHAN_PAUSE);

		vchan->phy = NULL;
		pchan->vchan = NULL;
		pchan->desc = NULL;
		pchan->done = NULL;
	}

	spin_unlock_irqrestore(&vchan->vc.lock, flags);

	vchan_dma_desc_free_list(&vchan->vc, &head);

	return 0;
}

static enum dma_status sun6i_dma_tx_status(struct dma_chan *chan,
					   dma_cookie_t cookie,
					   struct dma_tx_state *state)
{
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);
	struct sun6i_pchan *pchan = vchan->phy;
	struct sun6i_dma_lli *lli;
	struct virt_dma_desc *vd;
	struct sun6i_desc *txd;
	enum dma_status ret;
	unsigned long flags;
	size_t bytes = 0;

	ret = dma_cookie_status(chan, cookie, state);
	if (ret == DMA_COMPLETE)
		return ret;

	spin_lock_irqsave(&vchan->vc.lock, flags);

	vd = vchan_find_desc(&vchan->vc, cookie);
	txd = to_sun6i_desc(&vd->tx);

	if (vd) {
		for (lli = txd->v_lli; lli != NULL; lli = lli->v_lli_next)
			bytes += lli->len;
	} else if (!pchan || !pchan->desc) {
		bytes = 0;
	} else {
		bytes = readl(pchan->base + DMA_CHAN_CUR_CNT);
	}

	spin_unlock_irqrestore(&vchan->vc.lock, flags);

	dma_set_residue(state, bytes);

	return ret;
}

static void sun6i_dma_issue_pending(struct dma_chan *chan)
{
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(chan->device);
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);
	unsigned long flags;

	spin_lock_irqsave(&vchan->vc.lock, flags);

	if (vchan_issue_pending(&vchan->vc)) {
		spin_lock(&sdev->lock);

		if (!vchan->phy && list_empty(&vchan->node)) {
			list_add_tail(&vchan->node, &sdev->pending);
			tasklet_schedule(&sdev->task);
			dev_dbg(chan2dev(chan), "vchan %p: issued\n",
				&vchan->vc);
		}

		spin_unlock(&sdev->lock);
	} else {
		dev_dbg(chan2dev(chan), "vchan %p: nothing to issue\n",
			&vchan->vc);
	}

	spin_unlock_irqrestore(&vchan->vc.lock, flags);
}

static void sun6i_dma_free_chan_resources(struct dma_chan *chan)
{
	struct sun6i_dma_dev *sdev = to_sun6i_dma_dev(chan->device);
	struct sun6i_vchan *vchan = to_sun6i_vchan(chan);
	unsigned long flags;

	spin_lock_irqsave(&sdev->lock, flags);
	list_del_init(&vchan->node);
	spin_unlock_irqrestore(&sdev->lock, flags);

	vchan_free_chan_resources(&vchan->vc);
}

static struct dma_chan *sun6i_dma_of_xlate(struct of_phandle_args *dma_spec,
					   struct of_dma *ofdma)
{
	struct sun6i_dma_dev *sdev = ofdma->of_dma_data;
	struct sun6i_vchan *vchan;
	struct dma_chan *chan;
	u8 port = dma_spec->args[0];

	if (port > sdev->cfg->nr_max_requests)
		return NULL;

	chan = dma_get_any_slave_channel(&sdev->slave);
	if (!chan)
		return NULL;

	vchan = to_sun6i_vchan(chan);
	vchan->port = port;

	return chan;
}

static inline void sun6i_kill_tasklet(struct sun6i_dma_dev *sdev)
{
	/* Disable all interrupts from DMA */
	writel(0, sdev->base + DMA_IRQ_EN(0));
	writel(0, sdev->base + DMA_IRQ_EN(1));

	/* Prevent spurious interrupts from scheduling the tasklet */
	atomic_inc(&sdev->tasklet_shutdown);

	/* Make sure we won't have any further interrupts */
	devm_free_irq(sdev->slave.dev, sdev->irq, sdev);

	/* Actually prevent the tasklet from being scheduled */
	tasklet_kill(&sdev->task);
}

static inline void sun6i_dma_free(struct sun6i_dma_dev *sdev)
{
	int i;

	for (i = 0; i < sdev->cfg->nr_max_vchans; i++) {
		struct sun6i_vchan *vchan = &sdev->vchans[i];

		list_del(&vchan->vc.chan.device_node);
		tasklet_kill(&vchan->vc.task);
	}
}

/*
 * For A31:
 *
 * There's 16 physical channels that can work in parallel.
 *
 * However we have 30 different endpoints for our requests.
 *
 * Since the channels are able to handle only an unidirectional
 * transfer, we need to allocate more virtual channels so that
 * everyone can grab one channel.
 *
 * Some devices can't work in both direction (mostly because it
 * wouldn't make sense), so we have a bit fewer virtual channels than
 * 2 channels per endpoints.
 */

static struct sun6i_dma_config sun6i_a31_dma_cfg = {
	.nr_max_channels = 16,
	.nr_max_requests = 30,
	.nr_max_vchans   = 53,
};

/*
 * The A23 only has 8 physical channels, a maximum DRQ port id of 24,
 * and a total of 37 usable source and destination endpoints.
 */

static struct sun6i_dma_config sun8i_a23_dma_cfg = {
	.nr_max_channels = 8,
	.nr_max_requests = 24,
	.nr_max_vchans   = 37,
};

/*
 * The H3 has 12 physical channels, a maximum DRQ port id of 27,
 * and a total of 34 usable source and destination endpoints.
 */

static struct sun6i_dma_config sun8i_h3_dma_cfg = {
	.nr_max_channels = 12,
	.nr_max_requests = 27,
	.nr_max_vchans   = 34,
};

static const struct of_device_id sun6i_dma_match[] = {
	{ .compatible = "allwinner,sun6i-a31-dma", .data = &sun6i_a31_dma_cfg },
	{ .compatible = "allwinner,sun8i-a23-dma", .data = &sun8i_a23_dma_cfg },
	{ .compatible = "allwinner,sun8i-h3-dma", .data = &sun8i_h3_dma_cfg },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sun6i_dma_match);

static int sun6i_dma_probe(struct platform_device *pdev)
{
	const struct of_device_id *device;
	struct sun6i_dma_dev *sdc;
	struct resource *res;
	int ret, i;

	sdc = devm_kzalloc(&pdev->dev, sizeof(*sdc), GFP_KERNEL);
	if (!sdc)
		return -ENOMEM;

	device = of_match_device(sun6i_dma_match, &pdev->dev);
	if (!device)
		return -ENODEV;
	sdc->cfg = device->data;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sdc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(sdc->base))
		return PTR_ERR(sdc->base);

	sdc->irq = platform_get_irq(pdev, 0);
	if (sdc->irq < 0) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
		return sdc->irq;
	}

	sdc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(sdc->clk)) {
		dev_err(&pdev->dev, "No clock specified\n");
		return PTR_ERR(sdc->clk);
	}

	sdc->rstc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(sdc->rstc)) {
		dev_err(&pdev->dev, "No reset controller specified\n");
		return PTR_ERR(sdc->rstc);
	}

	sdc->pool = dmam_pool_create(dev_name(&pdev->dev), &pdev->dev,
				     sizeof(struct sun6i_dma_lli), 4, 0);
	if (!sdc->pool) {
		dev_err(&pdev->dev, "No memory for descriptors dma pool\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, sdc);
	INIT_LIST_HEAD(&sdc->pending);
	spin_lock_init(&sdc->lock);

	dma_cap_set(DMA_PRIVATE, sdc->slave.cap_mask);
	dma_cap_set(DMA_MEMCPY, sdc->slave.cap_mask);
	dma_cap_set(DMA_SLAVE, sdc->slave.cap_mask);

	INIT_LIST_HEAD(&sdc->slave.channels);
	sdc->slave.device_free_chan_resources	= sun6i_dma_free_chan_resources;
	sdc->slave.device_tx_status		= sun6i_dma_tx_status;
	sdc->slave.device_issue_pending		= sun6i_dma_issue_pending;
	sdc->slave.device_prep_slave_sg		= sun6i_dma_prep_slave_sg;
	sdc->slave.device_prep_dma_memcpy	= sun6i_dma_prep_dma_memcpy;
	sdc->slave.copy_align			= DMAENGINE_ALIGN_4_BYTES;
	sdc->slave.device_config		= sun6i_dma_config;
	sdc->slave.device_pause			= sun6i_dma_pause;
	sdc->slave.device_resume		= sun6i_dma_resume;
	sdc->slave.device_terminate_all		= sun6i_dma_terminate_all;
	sdc->slave.src_addr_widths		= BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
						  BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) |
						  BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
	sdc->slave.dst_addr_widths		= BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
						  BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) |
						  BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
	sdc->slave.directions			= BIT(DMA_DEV_TO_MEM) |
						  BIT(DMA_MEM_TO_DEV);
	sdc->slave.residue_granularity		= DMA_RESIDUE_GRANULARITY_BURST;
	sdc->slave.dev = &pdev->dev;

	sdc->pchans = devm_kcalloc(&pdev->dev, sdc->cfg->nr_max_channels,
				   sizeof(struct sun6i_pchan), GFP_KERNEL);
	if (!sdc->pchans)
		return -ENOMEM;

	sdc->vchans = devm_kcalloc(&pdev->dev, sdc->cfg->nr_max_vchans,
				   sizeof(struct sun6i_vchan), GFP_KERNEL);
	if (!sdc->vchans)
		return -ENOMEM;

	tasklet_init(&sdc->task, sun6i_dma_tasklet, (unsigned long)sdc);

	for (i = 0; i < sdc->cfg->nr_max_channels; i++) {
		struct sun6i_pchan *pchan = &sdc->pchans[i];

		pchan->idx = i;
		pchan->base = sdc->base + 0x100 + i * 0x40;
	}

	for (i = 0; i < sdc->cfg->nr_max_vchans; i++) {
		struct sun6i_vchan *vchan = &sdc->vchans[i];

		INIT_LIST_HEAD(&vchan->node);
		vchan->vc.desc_free = sun6i_dma_free_desc;
		vchan_init(&vchan->vc, &sdc->slave);
	}

	ret = reset_control_deassert(sdc->rstc);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't deassert the device from reset\n");
		goto err_chan_free;
	}

	ret = clk_prepare_enable(sdc->clk);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't enable the clock\n");
		goto err_reset_assert;
	}

	ret = devm_request_irq(&pdev->dev, sdc->irq, sun6i_dma_interrupt, 0,
			       dev_name(&pdev->dev), sdc);
	if (ret) {
		dev_err(&pdev->dev, "Cannot request IRQ\n");
		goto err_clk_disable;
	}

	ret = dma_async_device_register(&sdc->slave);
	if (ret) {
		dev_warn(&pdev->dev, "Failed to register DMA engine device\n");
		goto err_irq_disable;
	}

	ret = of_dma_controller_register(pdev->dev.of_node, sun6i_dma_of_xlate,
					 sdc);
	if (ret) {
		dev_err(&pdev->dev, "of_dma_controller_register failed\n");
		goto err_dma_unregister;
	}

	/*
	 * sun8i variant requires us to toggle a dma gating register,
	 * as seen in Allwinner's SDK. This register is not documented
	 * in the A23 user manual.
	 */
	if (of_device_is_compatible(pdev->dev.of_node,
				    "allwinner,sun8i-a23-dma"))
		writel(SUN8I_DMA_GATE_ENABLE, sdc->base + SUN8I_DMA_GATE);

	return 0;

err_dma_unregister:
	dma_async_device_unregister(&sdc->slave);
err_irq_disable:
	sun6i_kill_tasklet(sdc);
err_clk_disable:
	clk_disable_unprepare(sdc->clk);
err_reset_assert:
	reset_control_assert(sdc->rstc);
err_chan_free:
	sun6i_dma_free(sdc);
	return ret;
}

static int sun6i_dma_remove(struct platform_device *pdev)
{
	struct sun6i_dma_dev *sdc = platform_get_drvdata(pdev);

	of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&sdc->slave);

	sun6i_kill_tasklet(sdc);

	clk_disable_unprepare(sdc->clk);
	reset_control_assert(sdc->rstc);

	sun6i_dma_free(sdc);

	return 0;
}

static struct platform_driver sun6i_dma_driver = {
	.probe		= sun6i_dma_probe,
	.remove		= sun6i_dma_remove,
	.driver = {
		.name		= "sun6i-dma",
		.of_match_table	= sun6i_dma_match,
	},
};
module_platform_driver(sun6i_dma_driver);

MODULE_DESCRIPTION("Allwinner A31 DMA Controller Driver");
MODULE_AUTHOR("Sugar <shuge@allwinnertech.com>");
MODULE_AUTHOR("Maxime Ripard <maxime.ripard@free-electrons.com>");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * DMA driver for Nvidia's Tegra20 APB DMA controller.
 *
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/slab.h>

#include "dmaengine.h"

#define TEGRA_APBDMA_GENERAL			0x0
#define TEGRA_APBDMA_GENERAL_ENABLE		BIT(31)

#define TEGRA_APBDMA_CONTROL			0x010
#define TEGRA_APBDMA_IRQ_MASK			0x01c
#define TEGRA_APBDMA_IRQ_MASK_SET		0x020

/* CSR register */
#define TEGRA_APBDMA_CHAN_CSR			0x00
#define TEGRA_APBDMA_CSR_ENB			BIT(31)
#define TEGRA_APBDMA_CSR_IE_EOC			BIT(30)
#define TEGRA_APBDMA_CSR_HOLD			BIT(29)
#define TEGRA_APBDMA_CSR_DIR			BIT(28)
#define TEGRA_APBDMA_CSR_ONCE			BIT(27)
#define TEGRA_APBDMA_CSR_FLOW			BIT(21)
#define TEGRA_APBDMA_CSR_REQ_SEL_SHIFT		16
#define TEGRA_APBDMA_CSR_WCOUNT_MASK		0xFFFC

/* STATUS register */
#define TEGRA_APBDMA_CHAN_STATUS		0x004
#define TEGRA_APBDMA_STATUS_BUSY		BIT(31)
#define TEGRA_APBDMA_STATUS_ISE_EOC		BIT(30)
#define TEGRA_APBDMA_STATUS_HALT		BIT(29)
#define TEGRA_APBDMA_STATUS_PING_PONG		BIT(28)
#define TEGRA_APBDMA_STATUS_COUNT_SHIFT		2
#define TEGRA_APBDMA_STATUS_COUNT_MASK		0xFFFC

#define TEGRA_APBDMA_CHAN_CSRE			0x00C
#define TEGRA_APBDMA_CHAN_CSRE_PAUSE		(1 << 31)

/* AHB memory address */
#define TEGRA_APBDMA_CHAN_AHBPTR		0x010

/* AHB sequence register */
#define TEGRA_APBDMA_CHAN_AHBSEQ		0x14
#define TEGRA_APBDMA_AHBSEQ_INTR_ENB		BIT(31)
#define TEGRA_APBDMA_AHBSEQ_BUS_WIDTH_8		(0 << 28)
#define TEGRA_APBDMA_AHBSEQ_BUS_WIDTH_16	(1 << 28)
#define TEGRA_APBDMA_AHBSEQ_BUS_WIDTH_32	(2 << 28)
#define TEGRA_APBDMA_AHBSEQ_BUS_WIDTH_64	(3 << 28)
#define TEGRA_APBDMA_AHBSEQ_BUS_WIDTH_128	(4 << 28)
#define TEGRA_APBDMA_AHBSEQ_DATA_SWAP		BIT(27)
#define TEGRA_APBDMA_AHBSEQ_BURST_1		(4 << 24)
#define TEGRA_APBDMA_AHBSEQ_BURST_4		(5 << 24)
#define TEGRA_APBDMA_AHBSEQ_BURST_8		(6 << 24)
#define TEGRA_APBDMA_AHBSEQ_DBL_BUF		BIT(19)
#define TEGRA_APBDMA_AHBSEQ_WRAP_SHIFT		16
#define TEGRA_APBDMA_AHBSEQ_WRAP_NONE		0

/* APB address */
#define TEGRA_APBDMA_CHAN_APBPTR		0x018

/* APB sequence register */
#define TEGRA_APBDMA_CHAN_APBSEQ		0x01c
#define TEGRA_APBDMA_APBSEQ_BUS_WIDTH_8		(0 << 28)
#define TEGRA_APBDMA_APBSEQ_BUS_WIDTH_16	(1 << 28)
#define TEGRA_APBDMA_APBSEQ_BUS_WIDTH_32	(2 << 28)
#define TEGRA_APBDMA_APBSEQ_BUS_WIDTH_64	(3 << 28)
#define TEGRA_APBDMA_APBSEQ_BUS_WIDTH_128	(4 << 28)
#define TEGRA_APBDMA_APBSEQ_DATA_SWAP		BIT(27)
#define TEGRA_APBDMA_APBSEQ_WRAP_WORD_1		(1 << 16)

/* Tegra148 specific registers */
#define TEGRA_APBDMA_CHAN_WCOUNT		0x20

#define TEGRA_APBDMA_CHAN_WORD_TRANSFER		0x24

/*
 * If any burst is in flight and DMA paused then this is the time to complete
 * on-flight burst and update DMA status register.
 */
#define TEGRA_APBDMA_BURST_COMPLETE_TIME	20

/* Channel base address offset from APBDMA base address */
#define TEGRA_APBDMA_CHANNEL_BASE_ADD_OFFSET	0x1000

struct tegra_dma;

/*
 * tegra_dma_chip_data Tegra chip specific DMA data
 * @nr_channels: Number of channels available in the controller.
 * @channel_reg_size: Channel register size/stride.
 * @max_dma_count: Maximum DMA transfer count supported by DMA controller.
 * @support_channel_pause: Support channel wise pause of dma.
 * @support_separate_wcount_reg: Support separate word count register.
 */
struct tegra_dma_chip_data {
	int nr_channels;
	int channel_reg_size;
	int max_dma_count;
	bool support_channel_pause;
	bool support_separate_wcount_reg;
};

/* DMA channel registers */
struct tegra_dma_channel_regs {
	unsigned long	csr;
	unsigned long	ahb_ptr;
	unsigned long	apb_ptr;
	unsigned long	ahb_seq;
	unsigned long	apb_seq;
	unsigned long	wcount;
};

/*
 * tegra_dma_sg_req: Dma request details to configure hardware. This
 * contains the details for one transfer to configure DMA hw.
 * The client's request for data transfer can be broken into multiple
 * sub-transfer as per requester details and hw support.
 * This sub transfer get added in the list of transfer and point to Tegra
 * DMA descriptor which manages the transfer details.
 */
struct tegra_dma_sg_req {
	struct tegra_dma_channel_regs	ch_regs;
	int				req_len;
	bool				configured;
	bool				last_sg;
	struct list_head		node;
	struct tegra_dma_desc		*dma_desc;
};

/*
 * tegra_dma_desc: Tegra DMA descriptors which manages the client requests.
 * This descriptor keep track of transfer status, callbacks and request
 * counts etc.
 */
struct tegra_dma_desc {
	struct dma_async_tx_descriptor	txd;
	int				bytes_requested;
	int				bytes_transferred;
	enum dma_status			dma_status;
	struct list_head		node;
	struct list_head		tx_list;
	struct list_head		cb_node;
	int				cb_count;
};

struct tegra_dma_channel;

typedef void (*dma_isr_handler)(struct tegra_dma_channel *tdc,
				bool to_terminate);

/* tegra_dma_channel: Channel specific information */
struct tegra_dma_channel {
	struct dma_chan		dma_chan;
	char			name[30];
	bool			config_init;
	int			id;
	int			irq;
	void __iomem		*chan_addr;
	spinlock_t		lock;
	bool			busy;
	struct tegra_dma	*tdma;
	bool			cyclic;

	/* Different lists for managing the requests */
	struct list_head	free_sg_req;
	struct list_head	pending_sg_req;
	struct list_head	free_dma_desc;
	struct list_head	cb_desc;

	/* ISR handler and tasklet for bottom half of isr handling */
	dma_isr_handler		isr_handler;
	struct tasklet_struct	tasklet;

	/* Channel-slave specific configuration */
	unsigned int slave_id;
	struct dma_slave_config dma_sconfig;
	struct tegra_dma_channel_regs	channel_reg;
};

/* tegra_dma: Tegra DMA specific information */
struct tegra_dma {
	struct dma_device		dma_dev;
	struct device			*dev;
	struct clk			*dma_clk;
	struct reset_control		*rst;
	spinlock_t			global_lock;
	void __iomem			*base_addr;
	const struct tegra_dma_chip_data *chip_data;

	/*
	 * Counter for managing global pausing of the DMA controller.
	 * Only applicable for devices that don't support individual
	 * channel pausing.
	 */
	u32				global_pause_count;

	/* Some register need to be cache before suspend */
	u32				reg_gen;

	/* Last member of the structure */
	struct tegra_dma_channel channels[0];
};

static inline void tdma_write(struct tegra_dma *tdma, u32 reg, u32 val)
{
	writel(val, tdma->base_addr + reg);
}

static inline u32 tdma_read(struct tegra_dma *tdma, u32 reg)
{
	return readl(tdma->base_addr + reg);
}

static inline void tdc_write(struct tegra_dma_channel *tdc,
		u32 reg, u32 val)
{
	writel(val, tdc->chan_addr + reg);
}

static inline u32 tdc_read(struct tegra_dma_channel *tdc, u32 reg)
{
	return readl(tdc->chan_addr + reg);
}

static inline struct tegra_dma_channel *to_tegra_dma_chan(struct dma_chan *dc)
{
	return container_of(dc, struct tegra_dma_channel, dma_chan);
}

static inline struct tegra_dma_desc *txd_to_tegra_dma_desc(
		struct dma_async_tx_descriptor *td)
{
	return container_of(td, struct tegra_dma_desc, txd);
}

static inline struct device *tdc2dev(struct tegra_dma_channel *tdc)
{
	return &tdc->dma_chan.dev->device;
}

static dma_cookie_t tegra_dma_tx_submit(struct dma_async_tx_descriptor *tx);
static int tegra_dma_runtime_suspend(struct device *dev);
static int tegra_dma_runtime_resume(struct device *dev);

/* Get DMA desc from free list, if not there then allocate it.  */
static struct tegra_dma_desc *tegra_dma_desc_get(
		struct tegra_dma_channel *tdc)
{
	struct tegra_dma_desc *dma_desc;
	unsigned long flags;

	spin_lock_irqsave(&tdc->lock, flags);

	/* Do not allocate if desc are waiting for ack */
	list_for_each_entry(dma_desc, &tdc->free_dma_desc, node) {
		if (async_tx_test_ack(&dma_desc->txd) && !dma_desc->cb_count) {
			list_del(&dma_desc->node);
			spin_unlock_irqrestore(&tdc->lock, flags);
			dma_desc->txd.flags = 0;
			return dma_desc;
		}
	}

	spin_unlock_irqrestore(&tdc->lock, flags);

	/* Allocate DMA desc */
	dma_desc = kzalloc(sizeof(*dma_desc), GFP_ATOMIC);
	if (!dma_desc) {
		dev_err(tdc2dev(tdc), "dma_desc alloc failed\n");
		return NULL;
	}

	dma_async_tx_descriptor_init(&dma_desc->txd, &tdc->dma_chan);
	dma_desc->txd.tx_submit = tegra_dma_tx_submit;
	dma_desc->txd.flags = 0;
	return dma_desc;
}

static void tegra_dma_desc_put(struct tegra_dma_channel *tdc,
		struct tegra_dma_desc *dma_desc)
{
	unsigned long flags;

	spin_lock_irqsave(&tdc->lock, flags);
	if (!list_empty(&dma_desc->tx_list))
		list_splice_init(&dma_desc->tx_list, &tdc->free_sg_req);
	list_add_tail(&dma_desc->node, &tdc->free_dma_desc);
	spin_unlock_irqrestore(&tdc->lock, flags);
}

static struct tegra_dma_sg_req *tegra_dma_sg_req_get(
		struct tegra_dma_channel *tdc)
{
	struct tegra_dma_sg_req *sg_req = NULL;
	unsigned long flags;

	spin_lock_irqsave(&tdc->lock, flags);
	if (!list_empty(&tdc->free_sg_req)) {
		sg_req = list_first_entry(&tdc->free_sg_req,
					typeof(*sg_req), node);
		list_del(&sg_req->node);
		spin_unlock_irqrestore(&tdc->lock, flags);
		return sg_req;
	}
	spin_unlock_irqrestore(&tdc->lock, flags);

	sg_req = kzalloc(sizeof(struct tegra_dma_sg_req), GFP_ATOMIC);
	if (!sg_req)
		dev_err(tdc2dev(tdc), "sg_req alloc failed\n");
	return sg_req;
}

static int tegra_dma_slave_config(struct dma_chan *dc,
		struct dma_slave_config *sconfig)
{
	struct tegra_dma_channel *tdc = to_tegra_dma_chan(dc);

	if (!list_empty(&tdc->pending_sg_req)) {
		dev_err(tdc2dev(tdc), "Configuration not allowed\n");
		return -EBUSY;
	}

	memcpy(&tdc->dma_sconfig, sconfig, sizeof(*sconfig));
	if (!tdc->slave_id)
		tdc->slave_id = sconfig->slave_id;
	tdc->config_init = true;
	return 0;
}

static void tegra_dma_global_pause(struct tegra_dma_channel *tdc,
	bool wait_for_burst_complete)
{
	struct tegra_dma *tdma = tdc->tdma;

	spin_lock(&tdma->global_lock);

	if (tdc->tdma->global_pause_count == 0) {
		tdma_write(tdma, TEGRA_APBDMA_GENERAL, 0);
		if (wait_for_burst_complete)
			udelay(TEGRA_APBDMA_BURST_COMPLETE_TIME);
	}

	tdc->tdma->global_pause_count++;

	spin_unlock(&tdma->global_lock);
}

static void tegra_dma_global_resume(struct tegra_dma_channel *tdc)
{
	struct tegra_dma *tdma = tdc->tdma;

	spin_lock(&tdma->global_lock);

	if (WARN_ON(tdc->tdma->global_pause_count == 0))
		goto out;

	if (--tdc->tdma->global_pause_count == 0)
		tdma_write(tdma, TEGRA_APBDMA_GENERAL,
			   TEGRA_APBDMA_GENERAL_ENABLE);

out:
	spin_unlock(&tdma->global_lock);
}

static void tegra_dma_pause(struct tegra_dma_channel *tdc,
	bool wait_for_burst_complete)
{
	struct tegra_dma *tdma = tdc->tdma;

	if (tdma->chip_data->support_channel_pause) {
		tdc_write(tdc, TEGRA_APBDMA_CHAN_CSRE,
				TEGRA_APBDMA_CHAN_CSRE_PAUSE);
		if (wait_for_burst_complete)
			udelay(TEGRA_APBDMA_BURST_COMPLETE_TIME);
	} else {
		tegra_dma_global_pause(tdc, wait_for_burst_complete);
	}
}

static void tegra_dma_resume(struct tegra_dma_channel *tdc)
{
	struct tegra_dma *tdma = tdc->tdma;

	if (tdma->chip_data->support_channel_pause) {
		tdc_write(tdc, TEGRA_APBDMA_CHAN_CSRE, 0);
	} else {
		tegra_dma_global_resume(tdc);
	}
}

static void tegra_dma_stop(struct tegra_dma_channel *tdc)
{
	u32 csr;
	u32 status;

	/* Disable interrupts */
	csr = tdc_read(tdc, TEGRA_APBDMA_CHAN_CSR);
	csr &= ~TEGRA_APBDMA_CSR_IE_EOC;
	tdc_write(tdc, TEGRA_APBDMA_CHAN_CSR, csr);

	/* Disable DMA */
	csr &= ~TEGRA_APBDMA_CSR_ENB;
	tdc_write(tdc, TEGRA_APBDMA_CHAN_CSR, csr);

	/* Clear interrupt status if it is there */
	status = tdc_read(tdc, TEGRA_APBDMA_CHAN_STATUS);
	if (status & TEGRA_APBDMA_STATUS_ISE_EOC) {
		dev_dbg(tdc2dev(tdc), "%s():clearing interrupt\n", __func__);
		tdc_write(tdc, TEGRA_APBDMA_CHAN_STATUS, status);
	}
	tdc->busy = false;
}

static void tegra_dma_start(struct tegra_dma_channel *tdc,
		struct tegra_dma_sg_req *sg_req)
{
	struct tegra_dma_channel_regs *ch_regs = &sg_req->ch_regs;

	tdc_write(tdc, TEGRA_APBDMA_CHAN_CSR, ch_regs->csr);
	tdc_write(tdc, TEGRA_APBDMA_CHAN_APBSEQ, ch_regs->apb_seq);
	tdc_write(tdc, TEGRA_APBDMA_CHAN_APBPTR, ch_regs->apb_ptr);
	tdc_write(tdc, TEGRA_APBDMA_CHAN_AHBSEQ, ch_regs->ahb_seq);
	tdc_write(tdc, TEGRA_APBDMA_CHAN_AHBPTR, ch_regs->ahb_ptr);
	if (tdc->tdma->chip_data->support_separate_wcount_reg)
		tdc_write(tdc, TEGRA_APBDMA_CHAN_WCOUNT, ch_regs->wcount);

	/* Start DMA */
	tdc_write(tdc, TEGRA_APBDMA_CHAN_CSR,
				ch_regs->csr | TEGRA_APBDMA_CSR_ENB);
}

static void tegra_dma_configure_for_next(struct tegra_dma_channel *tdc,
		struct tegra_dma_sg_req *nsg_req)
{
	unsigned long status;

	/*
	 * The DMA controller reloads the new configuration for next transfer
	 * after last burst of current transfer completes.
	 * If there is no IEC status then this makes sure that last burst
	 * has not be completed. There may be case that last burst is on
	 * flight and so it can complete but because DMA is paused, it
	 * will not generates interrupt as well as not reload the new
	 * configuration.
	 * If there is already IEC status then interrupt handler need to
	 * load new configuration.
	 */
	tegra_dma_pause(tdc, false);
	status  = tdc_read(tdc, TEGRA_APBDMA_CHAN_STATUS);

	/*
	 * If interrupt is pending then do nothing as the ISR will handle
	 * the programing for new request.
	 */
	if (status & TEGRA_APBDMA_STATUS_ISE_EOC) {
		dev_err(tdc2dev(tdc),
			"Skipping new configuration as interrupt is pending\n");
		tegra_dma_resume(tdc);
		return;
	}

	/* Safe to program new configuration */
	tdc_write(tdc, TEGRA_APBDMA_CHAN_APBPTR, nsg_req->ch_regs.apb_ptr);
	tdc_write(tdc, TEGRA_APBDMA_CHAN_AHBPTR, nsg_req->ch_regs.ahb_ptr);
	if (tdc->tdma->chip_data->support_separate_wcount_reg)
		tdc_write(tdc, TEGRA_APBDMA_CHAN_WCOUNT,
						nsg_req->ch_regs.wcount);
	tdc_write(tdc, TEGRA_APBDMA_CHAN_CSR,
				nsg_req->ch_regs.csr | TEGRA_APBDMA_CSR_ENB);
	nsg_req->configured = true;

	tegra_dma_resume(tdc);
}

static void tdc_start_head_req(struct tegra_dma_channel *tdc)
{
	struct tegra_dma_sg_req *sg_req;

	if (list_empty(&tdc->pending_sg_req))
		return;

	sg_req = list_first_entry(&tdc->pending_sg_req,
					typeof(*sg_req), node);
	tegra_dma_start(tdc, sg_req);
	sg_req->configured = true;
	tdc->busy = true;
}

static void tdc_configure_next_head_desc(struct tegra_dma_channel *tdc)
{
	struct tegra_dma_sg_req *hsgreq;
	struct tegra_dma_sg_req *hnsgreq;

	if (list_empty(&tdc->pending_sg_req))
		return;

	hsgreq = list_first_entry(&tdc->pending_sg_req, typeof(*hsgreq), node);
	if (!list_is_last(&hsgreq->node, &tdc->pending_sg_req)) {
		hnsgreq = list_first_entry(&hsgreq->node,
					typeof(*hnsgreq), node);
		tegra_dma_configure_for_next(tdc, hnsgreq);
	}
}

static inline int get_current_xferred_count(struct tegra_dma_channel *tdc,
	struct tegra_dma_sg_req *sg_req, unsigned long status)
{
	return sg_req->req_len - (status & TEGRA_APBDMA_STATUS_COUNT_MASK) - 4;
}

static void tegra_dma_abort_all(struct tegra_dma_channel *tdc)
{
	struct tegra_dma_sg_req *sgreq;
	struct tegra_dma_desc *dma_desc;

	while (!list_empty(&tdc->pending_sg_req)) {
		sgreq = list_first_entry(&tdc->pending_sg_req,
						typeof(*sgreq), node);
		list_move_tail(&sgreq->node, &tdc->free_sg_req);
		if (sgreq->last_sg) {
			dma_desc = sgreq->dma_desc;
			dma_desc->dma_status = DMA_ERROR;
			list_add_tail(&dma_desc->node, &tdc->free_dma_desc);

			/* Add in cb list if it is not there. */
			if (!dma_desc->cb_count)
				list_add_tail(&dma_desc->cb_node,
							&tdc->cb_desc);
			dma_desc->cb_count++;
		}
	}
	tdc->isr_handler = NULL;
}

static bool handle_continuous_head_request(struct tegra_dma_channel *tdc,
		struct tegra_dma_sg_req *last_sg_req, bool to_terminate)
{
	struct tegra_dma_sg_req *hsgreq = NULL;

	if (list_empty(&tdc->pending_sg_req)) {
		dev_err(tdc2dev(tdc), "Dma is running without req\n");
		tegra_dma_stop(tdc);
		return false;
	}

	/*
	 * Check that head req on list should be in flight.
	 * If it is not in flight then abort transfer as
	 * looping of transfer can not continue.
	 */
	hsgreq = list_first_entry(&tdc->pending_sg_req, typeof(*hsgreq), node);
	if (!hsgreq->configured) {
		tegra_dma_stop(tdc);
		dev_err(tdc2dev(tdc), "Error in dma transfer, aborting dma\n");
		tegra_dma_abort_all(tdc);
		return false;
	}

	/* Configure next request */
	if (!to_terminate)
		tdc_configure_next_head_desc(tdc);
	return true;
}

static void handle_once_dma_done(struct tegra_dma_channel *tdc,
	bool to_terminate)
{
	struct tegra_dma_sg_req *sgreq;
	struct tegra_dma_desc *dma_desc;

	tdc->busy = false;
	sgreq = list_first_entry(&tdc->pending_sg_req, typeof(*sgreq), node);
	dma_desc = sgreq->dma_desc;
	dma_desc->bytes_transferred += sgreq->req_len;

	list_del(&sgreq->node);
	if (sgreq->last_sg) {
		dma_desc->dma_status = DMA_COMPLETE;
		dma_cookie_complete(&dma_desc->txd);
		if (!dma_desc->cb_count)
			list_add_tail(&dma_desc->cb_node, &tdc->cb_desc);
		dma_desc->cb_count++;
		list_add_tail(&dma_desc->node, &tdc->free_dma_desc);
	}
	list_add_tail(&sgreq->node, &tdc->free_sg_req);

	/* Do not start DMA if it is going to be terminate */
	if (to_terminate || list_empty(&tdc->pending_sg_req))
		return;

	tdc_start_head_req(tdc);
}

static void handle_cont_sngl_cycle_dma_done(struct tegra_dma_channel *tdc,
		bool to_terminate)
{
	struct tegra_dma_sg_req *sgreq;
	struct tegra_dma_desc *dma_desc;
	bool st;

	sgreq = list_first_entry(&tdc->pending_sg_req, typeof(*sgreq), node);
	dma_desc = sgreq->dma_desc;
	/* if we dma for long enough the transfer count will wrap */
	dma_desc->bytes_transferred =
		(dma_desc->bytes_transferred + sgreq->req_len) %
		dma_desc->bytes_requested;

	/* Callback need to be call */
	if (!dma_desc->cb_count)
		list_add_tail(&dma_desc->cb_node, &tdc->cb_desc);
	dma_desc->cb_count++;

	/* If not last req then put at end of pending list */
	if (!list_is_last(&sgreq->node, &tdc->pending_sg_req)) {
		list_move_tail(&sgreq->node, &tdc->pending_sg_req);
		sgreq->configured = false;
		st = handle_continuous_head_request(tdc, sgreq, to_terminate);
		if (!st)
			dma_desc->dma_status = DMA_ERROR;
	}
}

static void tegra_dma_tasklet(unsigned long data)
{
	struct tegra_dma_channel *tdc = (struct tegra_dma_channel *)data;
	dma_async_tx_callback callback = NULL;
	void *callback_param = NULL;
	struct tegra_dma_desc *dma_desc;
	unsigned long flags;
	int cb_count;

	spin_lock_irqsave(&tdc->lock, flags);
	while (!list_empty(&tdc->cb_desc)) {
		dma_desc  = list_first_entry(&tdc->cb_desc,
					typeof(*dma_desc), cb_node);
		list_del(&dma_desc->cb_node);
		callback = dma_desc->txd.callback;
		callback_param = dma_desc->txd.callback_param;
		cb_count = dma_desc->cb_count;
		dma_desc->cb_count = 0;
		spin_unlock_irqrestore(&tdc->lock, flags);
		while (cb_count-- && callback)
			callback(callback_param);
		spin_lock_irqsave(&tdc->lock, flags);
	}
	spin_unlock_irqrestore(&tdc->lock, flags);
}

static irqreturn_t tegra_dma_isr(int irq, void *dev_id)
{
	struct tegra_dma_channel *tdc = dev_id;
	unsigned long status;
	unsigned long flags;

	spin_lock_irqsave(&tdc->lock, flags);

	status = tdc_read(tdc, TEGRA_APBDMA_CHAN_STATUS);
	if (status & TEGRA_APBDMA_STATUS_ISE_EOC) {
		tdc_write(tdc, TEGRA_APBDMA_CHAN_STATUS, status);
		tdc->isr_handler(tdc, false);
		tasklet_schedule(&tdc->tasklet);
		spin_unlock_irqrestore(&tdc->lock, flags);
		return IRQ_HANDLED;
	}

	spin_unlock_irqrestore(&tdc->lock, flags);
	dev_info(tdc2dev(tdc),
		"Interrupt already served status 0x%08lx\n", status);
	return IRQ_NONE;
}

static dma_cookie_t tegra_dma_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct tegra_dma_desc *dma_desc = txd_to_tegra_dma_desc(txd);
	struct tegra_dma_channel *tdc = to_tegra_dma_chan(txd->chan);
	unsigned long flags;
	dma_cookie_t cookie;

	spin_lock_irqsave(&tdc->lock, flags);
	dma_desc->dma_status = DMA_IN_PROGRESS;
	cookie = dma_cookie_assign(&dma_desc->txd);
	list_splice_tail_init(&dma_desc->tx_list, &tdc->pending_sg_req);
	spin_unlock_irqrestore(&tdc->lock, flags);
	return cookie;
}

static void tegra_dma_issue_pending(struct dma_chan *dc)
{
	struct tegra_dma_channel *tdc = to_tegra_dma_chan(dc);
	unsigned long flags;

	spin_lock_irqsave(&tdc->lock, flags);
	if (list_empty(&tdc->pending_sg_req)) {
		dev_err(tdc2dev(tdc), "No DMA request\n");
		goto end;
	}
	if (!tdc->busy) {
		tdc_start_head_req(tdc);

		/* Continuous single mode: Configure next req */
		if (tdc->cyclic) {
			/*
			 * Wait for 1 burst time for configure DMA for
			 * next transfer.
			 */
			udelay(TEGRA_APBDMA_BURST_COMPLETE_TIME);
			tdc_configure_next_head_desc(tdc);
		}
	}
end:
	spin_unlock_irqrestore(&tdc->lock, flags);
}

static int tegra_dma_terminate_all(struct dma_chan *dc)
{
	struct tegra_dma_channel *tdc = to_tegra_dma_chan(dc);
	struct tegra_dma_sg_req *sgreq;
	struct tegra_dma_desc *dma_desc;
	unsigned long flags;
	unsigned long status;
	unsigned long wcount;
	bool was_busy;

	spin_lock_irqsave(&tdc->lock, flags);

	if (!tdc->busy)
		goto skip_dma_stop;

	/* Pause DMA before checking the queue status */
	tegra_dma_pause(tdc, true);

	status = tdc_read(tdc, TEGRA_APBDMA_CHAN_STATUS);
	if (status & TEGRA_APBDMA_STATUS_ISE_EOC) {
		dev_dbg(tdc2dev(tdc), "%s():handling isr\n", __func__);
		tdc->isr_handler(tdc, true);
		status = tdc_read(tdc, TEGRA_APBDMA_CHAN_STATUS);
	}
	if (tdc->tdma->chip_data->support_separate_wcount_reg)
		wcount = tdc_read(tdc, TEGRA_APBDMA_CHAN_WORD_TRANSFER);
	else
		wcount = status;

	was_busy = tdc->busy;
	tegra_dma_stop(tdc);

	if (!list_empty(&tdc->pending_sg_req) && was_busy) {
		sgreq = list_first_entry(&tdc->pending_sg_req,
					typeof(*sgreq), node);
		sgreq->dma_desc->bytes_transferred +=
				get_current_xferred_count(tdc, sgreq, wcount);
	}
	tegra_dma_resume(tdc);

skip_dma_stop:
	tegra_dma_abort_all(tdc);

	while (!list_empty(&tdc->cb_desc)) {
		dma_desc  = list_first_entry(&tdc->cb_desc,
					typeof(*dma_desc), cb_node);
		list_del(&dma_desc->cb_node);
		dma_desc->cb_count = 0;
	}
	spin_unlock_irqrestore(&tdc->lock, flags);
	return 0;
}

static enum dma_status tegra_dma_tx_status(struct dma_chan *dc,
	dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	struct tegra_dma_channel *tdc = to_tegra_dma_chan(dc);
	struct tegra_dma_desc *dma_desc;
	struct tegra_dma_sg_req *sg_req;
	enum dma_status ret;
	unsigned long flags;
	unsigned int residual;

	ret = dma_cookie_status(dc, cookie, txstate);
	if (ret == DMA_COMPLETE)
		return ret;

	spin_lock_irqsave(&tdc->lock, flags);

	/* Check on wait_ack desc status */
	list_for_each_entry(dma_desc, &tdc->free_dma_desc, node) {
		if (dma_desc->txd.cookie == cookie) {
			residual =  dma_desc->bytes_requested -
					(dma_desc->bytes_transferred %
						dma_desc->bytes_requested);
			dma_set_residue(txstate, residual);
			ret = dma_desc->dma_status;
			spin_unlock_irqrestore(&tdc->lock, flags);
			return ret;
		}
	}

	/* Check in pending list */
	list_for_each_entry(sg_req, &tdc->pending_sg_req, node) {
		dma_desc = sg_req->dma_desc;
		if (dma_desc->txd.cookie == cookie) {
			residual =  dma_desc->bytes_requested -
					(dma_desc->bytes_transferred %
						dma_desc->bytes_requested);
			dma_set_residue(txstate, residual);
			ret = dma_desc->dma_status;
			spin_unlock_irqrestore(&tdc->lock, flags);
			return ret;
		}
	}

	dev_dbg(tdc2dev(tdc), "cookie %d does not found\n", cookie);
	spin_unlock_irqrestore(&tdc->lock, flags);
	return ret;
}

static inline int get_bus_width(struct tegra_dma_channel *tdc,
		enum dma_slave_buswidth slave_bw)
{
	switch (slave_bw) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		return TEGRA_APBDMA_APBSEQ_BUS_WIDTH_8;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		return TEGRA_APBDMA_APBSEQ_BUS_WIDTH_16;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		return TEGRA_APBDMA_APBSEQ_BUS_WIDTH_32;
	case DMA_SLAVE_BUSWIDTH_8_BYTES:
		return TEGRA_APBDMA_APBSEQ_BUS_WIDTH_64;
	default:
		dev_warn(tdc2dev(tdc),
			"slave bw is not supported, using 32bits\n");
		return TEGRA_APBDMA_APBSEQ_BUS_WIDTH_32;
	}
}

static inline int get_burst_size(struct tegra_dma_channel *tdc,
	u32 burst_size, enum dma_slave_buswidth slave_bw, int len)
{
	int burst_byte;
	int burst_ahb_width;

	/*
	 * burst_size from client is in terms of the bus_width.
	 * convert them into AHB memory wi