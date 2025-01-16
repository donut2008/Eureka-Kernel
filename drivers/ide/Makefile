_start, iop_chan, len);
		iop_desc_set_dest_addr(grp_start, iop_chan, dma_dest);
		sw_desc->async_tx.flags = flags;
		while (src_cnt--)
			iop_desc_set_xor_src_addr(grp_start, src_cnt,
						  dma_src[src_cnt]);
	}
	spin_unlock_bh(&iop_chan->lock);

	return sw_desc ? &sw_desc->async_tx : NULL;
}

static struct dma_async_tx_descriptor *
iop_adma_prep_dma_xor_val(struct dma_chan *chan, dma_addr_t *dma_src,
			  unsigned int src_cnt, size_t len, u32 *result,
			  unsigned long flags)
{
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(chan);
	struct iop_adma_desc_slot *sw_desc, *grp_start;
	int slot_cnt, slots_per_op;

	if (unlikely(!len))
		return NULL;

	dev_dbg(iop_chan->device->common.dev, "%s src_cnt: %d len: %zu\n",
		__func__, src_cnt, len);

	spin_lock_bh(&iop_chan->lock);
	slot_cnt = iop_chan_zero_sum_slot_count(len, src_cnt, &slots_per_op);
	sw_desc = iop_adma_alloc_slots(iop_chan, slot_cnt, slots_per_op);
	if (sw_desc) {
		grp_start = sw_desc->group_head;
		iop_desc_init_zero_sum(grp_start, src_cnt, flags);
		iop_desc_set_zero_sum_byte_count(grp_start, len);
		grp_start->xor_check_result = result;
		pr_debug("\t%s: grp_start->xor_check_result: %p\n",
			__func__, grp_start->xor_check_result);
		sw_desc->async_tx.flags = flags;
		while (src_cnt--)
			iop_desc_set_zero_sum_src_addr(grp_start, src_cnt,
						       dma_src[src_cnt]);
	}
	spin_unlock_bh(&iop_chan->lock);

	return sw_desc ? &sw_desc->async_tx : NULL;
}

static struct dma_async_tx_descriptor *
iop_adma_prep_dma_pq(struct dma_chan *chan, dma_addr_t *dst, dma_addr_t *src,
		     unsigned int src_cnt, const unsigned char *scf, size_t len,
		     unsigned long flags)
{
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(chan);
	struct iop_adma_desc_slot *sw_desc, *g;
	int slot_cnt, slots_per_op;
	int continue_srcs;

	if (unlikely(!len))
		return NULL;
	BUG_ON(len > IOP_ADMA_XOR_MAX_BYTE_COUNT);

	dev_dbg(iop_chan->device->common.dev,
		"%s src_cnt: %d len: %zu flags: %lx\n",
		__func__, src_cnt, len, flags);

	if (dmaf_p_disabled_continue(flags))
		continue_srcs = 1+src_cnt;
	else if (dmaf_continue(flags))
		continue_srcs = 3+src_cnt;
	else
		continue_srcs = 0+src_cnt;

	spin_lock_bh(&iop_chan->lock);
	slot_cnt = iop_chan_pq_slot_count(len, continue_srcs, &slots_per_op);
	sw_desc = iop_adma_alloc_slots(iop_chan, slot_cnt, slots_per_op);
	if (sw_desc) {
		int i;

		g = sw_desc->group_head;
		iop_desc_set_byte_count(g, iop_chan, len);

		/* even if P is disabled its destination address (bits
		 * [3:0]) must match Q.  It is ok if P points to an
		 * invalid address, it won't be written.
		 */
		if (flags & DMA_PREP_PQ_DISABLE_P)
			dst[0] = dst[1] & 0x7;

		iop_desc_set_pq_addr(g, dst);
		sw_desc->async_tx.flags = flags;
		for (i = 0; i < src_cnt; i++)
			iop_desc_set_pq_src_addr(g, i, src[i], scf[i]);

		/* if we are continuing a previous operation factor in
		 * the old p and q values, see the comment for dma_maxpq
		 * in include/linux/dmaengine.h
		 */
		if (dmaf_p_disabled_continue(flags))
			iop_desc_set_pq_src_addr(g, i++, dst[1], 1);
		else if (dmaf_continue(flags)) {
			iop_desc_set_pq_src_addr(g, i++, dst[0], 0);
			iop_desc_set_pq_src_addr(g, i++, dst[1], 1);
			iop_desc_set_pq_src_addr(g, i++, dst[1], 0);
		}
		iop_desc_init_pq(g, i, flags);
	}
	spin_unlock_bh(&iop_chan->lock);

	return sw_desc ? &sw_desc->async_tx : NULL;
}

static struct dma_async_tx_descriptor *
iop_adma_prep_dma_pq_val(struct dma_chan *chan, dma_addr_t *pq, dma_addr_t *src,
			 unsigned int src_cnt, const unsigned char *scf,
			 size_t len, enum sum_check_flags *pqres,
			 unsigned long flags)
{
	struct iop_adma_chan *iop_chan = to_iop_adma_chan(chan);
	struct iop_adma_desc_slot *sw_desc, *g;
	int slot_cnt, slots_per_op;

	if (unlikely(!len))
		return NULL;
	BUG_ON(len > IOP_ADMA_XOR_MAX_BYTE_COUNT);

	dev_dbg(iop_chan->device->common.dev, "%s src_cnt: %d len: %zu\n",
		__func__, src_cnt, len);

	spin_lock_b