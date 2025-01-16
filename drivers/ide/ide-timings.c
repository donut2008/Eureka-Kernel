w_desc) {
		/* for validate operations p and q are tagged onto the
		 * end of the source list
		 */
		int pq_idx = src_cnt;

		g = sw_desc->group_head;
		iop_desc_init_pq_zero_sum(g, src_cnt+2, flags);
		iop_desc_set_pq_zero_sum_byte_count(g, len);
		g->pq_check_result = pqres;
		pr_debug("\t%s: g->pq_check_result: %p\n",
			__func__, g->pq_check_result);
		sw_desc->async_tx.flags = flags;
		while (src_cnt--)
			iop_desc_set_pq_zero_sum_src_addr(g, src_cnt,
							  src[src_cnt],
							  scf[src_cnt]);
		iop_desc_set_pq_zero_sum_addr(g, pq_idx, src);
	}
	spin_unlock_bh(&iop_chan->lock);

	return sw_desc ? &sw_desc->async_tx : NULL;
}

static void iop_adma_free_chan_resources(struct dma_chan *chan)
{
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(chan);
	struct iop_adma_desc_slot *iter, *_iter;
	int in_use_descs = 0;

	iop_adma_slot_cleanup(iop_chan);

	spin_lock_bh(&iop_chan->lock);
	list_for_each_entry_safe(iter, _iter, &iop_chan->chain,
					chain_node) {
		in_use_descs++;
		list_del(&iter->chain_node);
	}
	list_for_each_entry_safe_reverse(
		iter, _iter, &iop_chan->all_slots, slot_node) {
		list_del(&iter->slot_node);
		kfree(iter);
		iop_chan->slots_allocated--;
	}
	iop_chan->last_used = NULL;

	dev_dbg(iop_chan->device->common.dev, "%s slots_allocated %d\n",
		__func__, iop_chan->slots_allocated);
	spin_unlock_bh(&iop_chan->lock);

	/* one is ok since we left it on there on purpose */
	if (in_use_descs > 1)
		printk(KERN_ERR "IOP: Freeing %d in use descriptors!\n",
			in_use_descs - 1);
}

/**
 * iop_adma_status - poll the status of an ADMA transaction
 * @chan: ADMA channel handle
 * @cookie: ADMA transaction identifier
 * @txstate: a holder for the current state of the channel or NULL
 */
static enum dma_status iop_adma_status(struct dma_chan *chan,
					dma_cookie_t cookie,
					struct dma_tx_state *txstate)
{
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(chan);
	int ret;

	ret = dma_cookie_status(chan, cookie, txstate);
	if (ret == DMA_COMPLETE)
		return ret;

	iop_adma_slot_cleanup(iop_chan);

	return dma_cookie_status(chan, cookie, txstate);
}

static irqreturn_t iop_adma_eot_handler(int irq, void *data)
{
	struct iop_adma_chan *chan = data;

	dev_dbg(chan->device->common.dev, "%s\n", __func__);

	tasklet_schedule(&chan->irq_tasklet);

	iop_adma_device_clear_eot_status(chan);

	return IRQ_HANDLED;
}

static irqreturn_t iop_adma_eoc_handler(int irq, void *data)
{
	struct iop_adma_chan *chan = data;

	dev_dbg(chan->device->common.dev, "%s\n", __func__);

	tasklet_schedule(&chan->irq_tasklet);

	iop_adma_device_clear_eoc_status(chan);

	return IRQ_HANDLED;
}

static irqreturn_t iop_adma_err_handler(int irq, void *data)
{
	struct iop_adma_chan *chan = data;
	unsigned long status = iop_chan_get_status(chan);

	dev_err(chan->device->common.dev,
		"error ( %s%s%s%s%s%s%s)\n",
		iop_is_err_int_parity(status, chan) ? "int_parity " : "",
		iop_is_err_mcu_abort(status, chan) ? "mcu_abort " : "",
		iop_is_err_int_tabort(status, chan) ? "int_tabort " : "",
		iop_is_err_int_mabort(status, chan) ? "int_mabort " : "",
		iop_is_err_pci_tabort(status, chan) ? "pci_tabort " : "",
		iop_is_err_pci_mabort(status, chan) ? "pci_mabort " : "",
		iop_is_err_split_tx(status, chan) ? "split_tx " : "");

	iop_adma_device_clear_err_status(chan);

	BUG();

	return IRQ_HANDLED;
}

static void iop_adma_issue_pending(struct dma_chan *chan)
{
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(chan);

	if (iop_chan->pending) {
		iop_chan->pending = 0;
		iop_chan_append(iop_chan);
	}
}

/*
 * Perform a transaction to verify the HW works.
 */
#define IOP_ADMA_TEST_SIZE 2000

static int iop_adma_memcpy_self_test(struct iop_adma_device *device)
{
	int i;
	void *src, *dest;
	dma_addr_t src_dma, dest_dma;
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;
	struct dma_async_tx_descriptor *tx;
	int err = 0;
	struct iop_adma_chan *iop_chan;

	dev_dbg(device->common.dev, "%s\n", __func__);

	src = kmalloc(IOP_ADMA_TEST_SIZE, GFP_KERNEL);
	if (!src)
		return -ENOMEM;
	dest = kzalloc(IOP_ADMA_TEST_SIZE, GFP_KERNEL);
	if (!dest) {
		kfree(src);
		return -ENOMEM;
	}

	/* Fill in src buffer */
	for (i = 0; i < IOP_ADMA_TEST_SIZE; i++)
		((u8 *) src)[i] = (u8)i;

	/* Start copy, using first DMA channel */
	dma_chan = container_of(device->common.channels.next,
				struct dma_chan,
				device_node);
	if (iop_adma_alloc_chan_resources(dma_chan) < 1) {
		err = -ENODEV;
		goto out;
	}

	dest_dma = dma_map_single(dma_chan->device->dev, dest,
				IOP_ADMA_TEST_SIZE, DMA_FROM_DEVICE);
	src_dma = dma_map_single(dma_chan->device->dev, src,
				IOP_ADMA_TEST_SIZE, DMA_TO_DEVICE);
	tx = iop_adma_prep_dma_memcpy(dma_chan, dest_dma, src_dma,
				      IOP_ADMA_TEST_SIZE,
				      DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

	cookie = iop_adma_tx_submit(tx);
	iop_adma_issue_pending(dma_chan);
	msleep(1);

	if (iop_adma_status(dma_chan, cookie, NULL) !=
			DMA_COMPLETE) {
		dev_err(dma_chan->device->dev,
			"Self-test copy timed out, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

	iop_chan = to_iop_adma_chan(dma_chan);
	dma_sync_single_for_cpu(&iop_chan->device->pdev->dev, dest_dma,
		IOP_ADMA_TEST_SIZE, DMA_FROM_DEVICE);
	if (memcmp(src, dest, IOP_ADMA_TEST_SIZE)) {
		dev_err(dma_chan->device->dev,
			"Self-test copy failed compare, disabling\n");
		err = -ENODEV;
		goto free_resources;
	}

free_resources:
	iop_adma_free_chan_resources(dma_chan);
out:
	kfree(src);
	kfree(dest);
	return err;
}

#define IOP_ADMA_NUM_SRC_TEST 4 /* must be <= 15 */
static int
iop_adma_xor_val_self_test(struct iop_adma_device *device)
{
	int i, src_idx;
	struct page *dest;
	struct page *xor_srcs[IOP_ADMA_NUM_SRC_TEST];
	struct page *zero_sum_srcs[IOP_ADMA_NUM_SRC_TEST + 1];
	dma_addr_t dma_srcs[IOP_ADMA_NUM_SRC_TEST + 1];
	dma_addr_t dest_dma;
	struct dma_async_tx_descriptor *tx;
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;
	u8 cmp_byte = 0;
	u32 cmp_word;
	u32 zero_sum_result;
	int err = 0;
	struct iop_adma_chan *iop_chan;

	dev_dbg(device->common.dev, "%s\n", __func__);

	for (src_idx = 0; src_idx < IOP_ADMA_NUM_SRC_TEST; src_idx++) {
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
	for (src_idx = 0; src_idx < IOP_ADMA_NUM_SRC_TEST; src_idx++) {
		u8 *ptr = page_address(xor_srcs[src_idx]);
		for (i = 0; i < PAGE_SIZE; i++)
			ptr[i] = (1 << src_idx);
	}

	for (src_idx = 0; src_idx < IOP_ADMA_NUM_SRC_TEST; src_idx++)
		cmp_byte ^= (u8)