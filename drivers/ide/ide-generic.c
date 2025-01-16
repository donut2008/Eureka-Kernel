eturn true;
}

/**
 * ioat_check_space_lock - verify space and grab ring producer lock
 * @ioat: ioat,3 channel (ring) to operate on
 * @num_descs: allocation length
 */
int ioat_check_space_lock(struct ioatdma_chan *ioat_chan, int num_descs)
	__acquires(&ioat_chan->prep_lock)
{
	bool retry;

 retry:
	spin_lock_bh(&ioat_chan->prep_lock);
	/* never allow the last descriptor to be consumed, we need at
	 * least one free at all times to allow for on-the-fly ring
	 * resizing.
	 */
	if (likely(ioat_ring_space(ioat_chan) > num_descs)) {
		dev_dbg(to_dev(ioat_chan), "%s: num_descs: %d (%x:%x:%x)\n",
			__func__, num_descs, ioat_chan->head,
			ioat_chan->tail, ioat_chan->issued);
		ioat_chan->produce = num_descs;
		return 0;  /* with ioat->prep_lock held */
	}
	retry = test_and_set_bit(IOAT_RESHAPE_PENDING, &ioat_chan->state);
	spin_unlock_bh(&ioat_chan->prep_lock);

	/* is another cpu already trying to expand the ring? */
	if (retry)
		goto retry;

	spin_lock_bh(&ioat_chan->cleanup_lock);
	spin_lock_bh(&ioat_chan->prep_lock);
	retry = reshape_ring(ioat_chan, ioat_chan->alloc_order + 1);
	clear_bit(IOAT_RESHAPE_PENDING, &ioat_chan->state);
	spin_unlock_bh(&ioat_chan->prep_lock);
	spin_unlock_bh(&ioat_chan->cleanup_lock);

	/* if we were able to expand the ring retry the allocation */
	if (retry)
		goto retry;

	dev_dbg_ratelimited(to_dev(ioat_chan),
			    "%s: ring full! num_descs: %d (%x:%x:%x)\n",
			    __func__, num_descs, ioat_chan->head,
			    ioat_chan->tail, ioat_chan->issued);

	/* progress reclaim in the allocation failure case we may be
	 * called under bh_disabled so we need to trigger the timer
	 * event directly
	 */
	if (time_is_before_jiffies(ioat_chan->timer.expires)
	    && timer_pending(&ioat_chan->timer)) {
		mod_timer(&ioat_chan->timer, jiffies + COMPLETION_TIMEOUT);
		ioat_timer_event((unsigned long)ioat_chan);
	}

	return -ENOMEM;
}

static bool desc_has_ext(struct ioat_ring_ent *desc)
{
	struct ioat_dma_descriptor *hw = desc->hw;

	if (hw->ctl_f.op == IOAT_OP_XOR ||
	    hw->ctl_f.op == IOAT_OP_XOR_VAL) {
		struct ioat_xor_descriptor *xor = desc->xor;

		if (src_cnt_to_sw(xor->ctl_f.src_cnt) > 5)
			return true;
	} else if (hw->ctl_f.op == IOAT_OP_PQ ||
		   hw->ctl_f.op == IOAT_OP_PQ_VAL) {
		struct ioat_pq_descriptor *pq = desc->pq;

		if (src_cnt_to_sw(pq->ctl_f.src_cnt) > 3)
			return true;
	}

	return false;
}

static void
ioat_free_sed(struct ioatdma_device *ioat_dma, struct ioat_sed_ent *sed)
{
	if (!sed)
		return;

	dma_pool_free(ioat_dma->sed_hw_pool[sed->hw_pool], sed->hw, sed->dma);
	kmem_cache_free(ioat_sed_cache, sed);
}

static u64 ioat_get_current_completion(struct ioatdma_chan *ioat_chan)
{
	u64 phys_complete;
	u64 completion;

	completion = *ioat_chan->completion;
	phys_complete = ioat_chansts_to_addr(completion);

	dev_dbg(to_dev(ioat_chan), "%s: phys_complete: %#llx\n", __func__,
		(unsigned long long) phys_complete);

	return phys_complete;
}

static bool ioat_cleanup_preamble(struct ioatdma_chan *ioat_chan,
				   u64 *phys_complete)
{
	*phys_complete = ioat_get_current_completion(ioat_chan);
	if (*phys_complete == ioat_chan->last_completion)
		return false;

	clear_bit(IOAT_COMPLETION_ACK, &ioat_chan->state);
	mod_timer(&ioat_chan->timer, jiffies + COMPLETION_TIMEOUT);

	return true;
}

static void
desc_get_errstat(struct ioatdma_chan *ioat_chan, struct ioat_ring_ent *desc)
{
	struct ioat_dma_descriptor *hw = desc->hw;

	switch (hw->ctl_f.op) {
	case IOAT_OP_PQ_VAL:
	case IOAT_OP_PQ_VAL_16S:
	{
		struct ioat_pq_descriptor *pq = desc->pq;

		/* check if there's error written */
		if (!pq->dwbes_f.wbes)
			return;

		/* need to set a chanerr var for checking to clear later */

		if (pq->dwbes_f.p_val_err)
			*desc->result |= SUM_CHECK_P_RESULT;

		if (pq->dwbes_f.q_val_err)
			*desc->result |= SUM_CHECK_Q_RESULT;

		return;
	}
	default:
		return;
	}
}

/**
 * __cleanup - reclaim used descriptors
 * @ioat: channel (ring) to clean
 */
static void __clean