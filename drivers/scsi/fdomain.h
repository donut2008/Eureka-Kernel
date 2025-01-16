sc_pool;
		slot->async_tx.phys = dma_desc + idx * MV_XOR_SLOT_SIZE;
		slot->idx = idx++;

		spin_lock_bh(&mv_chan->lock);
		mv_chan->slots_allocated = idx;
		list_add_tail(&slot->node, &mv_chan->free_slots);
		spin_unlock_bh(&mv_chan->lock);
	}

	dev_dbg(mv_chan_to_devp(mv_chan),
		"allocated %d descriptor slots\n",
		mv_chan->slots_allocated);

	return mv_chan->slots_allocated ? : -ENOMEM;
}

static struct dma_async_tx_descriptor *
mv_xor_prep_dma_xor(struct dma_chan *chan, dma_addr_t dest, dma_addr_t *src,
		    unsigned int src_cnt, size_t len, unsigned long flags)
{
	struct mv_xor_chan *mv_chan = to_mv_xor_chan(chan);
	struct mv_xor_desc_slot *sw_desc;

	if (unlikely(len < MV_XOR_MIN_BYTE_COUNT))
		return NULL;

	BUG_ON(len > MV_XOR_MAX_BYTE_COUNT);

	dev_dbg(mv_chan_to_devp(mv_chan),
		"%s src_cnt: %d len: %u dest %pad flags: %ld\n",
		__func__, src_cnt, len, &dest, flags);

	sw_desc = mv_chan_alloc_slot(mv_chan);
	if (sw_desc) {
		sw_desc->type = DMA_XOR;
		sw_desc->async_tx.flags = flags;
		mv_desc_init(sw_desc, dest, len, flags);
		if (mv_chan->op_in_desc == XOR_MOD