ring_ent(ioat_chan, idx + i);
		pq = desc->pq;

		/* save a branch by unconditionally retrieving the
		 * extended descriptor pq_set_src() knows to not write
		 * to it in the single descriptor case
		 */
		ext = ioat_get_ring_ent(ioat_chan, idx + i + with_ext);
		pq_ex = ext->pq_ex;

		descs[0] = (struct ioat_raw_descriptor *) pq;
		descs[1] = (struct ioat_raw_descriptor *) pq_ex;

		for (s = 0; s < src_cnt; s++)
			pq_set_src(descs, src[s], offset, scf[s], s);

		/* see the comment for dma_maxpq in include/linux/dmaengine.h */
		if (dmaf_p_disabled_continue(flags))
			pq_set_src(descs, dst[1], offset, 1, s++);
		else if (dmaf_continue(flags)) {
			pq_set_src(descs, dst[0], offset, 0, s++);
			pq_set_src(descs, dst[1], offset, 1, s++);
			pq_set_src(descs, dst[1], offset, 0, s++);
		}
		pq->size = xfer_size;
		pq->p_addr = dst[0] + offset;
		pq->q_addr = dst[1] + offset;
		pq->ctl = 0;
		pq->ctl_f.op = op;
		/* we turn on descriptor write back error status */
		if (ioat_dma->cap & IOAT_CAP_DWBES)
			pq->ctl_f.wb_en = result ? 1 : 0;
		pq->ctl_f.src_cnt = src_cnt_to_hw(s);
		pq->ctl_f.p_disable = !!(flags & DMA_PREP_PQ_DISABLE_P);
		pq->ctl_f.q_disable = !!(flags & DMA_PREP_PQ_DISABLE_Q);

		len -= xfer_size;
		offset += xfer_size;
	} while ((i += 1 + with_ext) < num_descs);

	/* last pq descriptor carries the unmap parameters and fence bit */
	desc->txd.flags = flags;
	desc->len = total_len;
	if (result)
		desc->result = result;
	pq->ctl_f.fence = !!(flags & DMA_PREP_FENCE);
	dump_pq_desc_dbg(ioat_chan, desc, ext);

	if (!cb32) {
		pq->ctl_f.int_en = !!(flags & DMA_PREP_INTERRUPT);
		pq->ctl_f.compl_write = 1;
		compl_desc = desc;
	} else {
		/* completion descriptor carries interrupt bit */
		compl_desc = ioat_get_ring_ent(ioat_chan, idx + i);
		compl_desc->txd.flags = flags & DMA_PREP_INTERRUPT;
		hw = compl_desc->hw;
		hw->ctl = 0;
		hw->ctl_f.null = 1;
		hw->ctl_f.int_en = !!(flags & DMA_PREP_INTERRUPT);
		hw->ctl_f.compl_write = 1;
		hw->size = NULL_DESC_BUFFER_SIZE;
		dump_desc_dbg(ioat_chan, compl_desc);
	}


	/* we leave the channel locked to ensure in order submission */
	return &compl_desc->txd;
}

static struct dma_async_tx_descriptor *
__ioat_prep_pq16_lock(struct dma_chan *c, enum sum_check_flags *result,
		       const dma_addr_t *dst, const dma_addr_t *src,
		       unsigned int src_cnt, const unsigned char *scf,
		       size_t len, unsigned long flags)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);
	struct ioatdma_device *ioat_dma = ioat_chan->ioat_dma;
	struct ioat_ring_ent *desc;
	size_t total_len = len;
	struct ioat_pq_descriptor *pq;
	u32 offset = 0;
	u8 op;
	int i, s, idx, num_descs;

	/* this function is only called with 9-16 sources */
	op = result ? IOAT_OP_PQ_VAL_16S : IOAT_OP_PQ_16S;

	dev_dbg(to_dev(ioat_chan), "%s\n", __func__);

	num_descs = ioat_xferlen_to_descs(ioat_chan, len);

	/*
	 * 16 source pq is only available on cb3.3 and has no completion
	 * write hw bug.
	 */
	if (num_descs && ioat_check_space_lock(ioat_chan, num_descs) == 0)
		idx = ioat_chan->head;
	else
		return NULL;

	i = 0;

	do {
		struct ioat_raw_descriptor *descs[4];
		size_t xfer_size = min_t(size_t, len,
					 1 << ioat_chan->xfercap_log);

		desc = ioat_get_ring_ent(ioat_chan, idx + i);
		pq = desc->pq;

		descs[0] = (struct ioat_raw_descriptor *) pq;

		desc->sed = ioat3_alloc_sed(ioat_dma, (src_cnt-2) >> 3);
		if (!desc->sed) {
			dev_err(to_dev(ioat_chan),
				"%s: no free sed entries\n", __func__);
			return NULL;
		}

		pq->sed_addr = desc->sed->dma;
		desc->sed->parent = desc;

		descs[1] = (struct ioat_raw_descriptor *)desc->sed->hw;
		descs[2] = (void *)descs[1] + 64;

		for (s = 0; s < src_cnt; s++)
			pq16_set_src(descs, src[s], offset, scf[s], s);

		/* see the comment for dma_maxpq in include/linux/dmaengine.h */
		if (dmaf_p_disabled_continue(flags))
			pq16_set_src(descs, dst[1], offset, 1, s++);
		else if (dmaf_continue(flags)) {
			pq16_set_src(descs, dst[0], offset, 0, s++);
			pq16_set_src(descs, dst[1], offset, 1, s++);
			pq16_set_src(descs, dst[1], offset, 0, s++);
		}

		pq->size = xfer_size;
		pq->p_addr = dst[0] + offset;
		pq->q_addr = dst[1] + offset;
		pq->ctl = 0;
		pq->ctl_f.op = op;
		pq->ctl_f.src_cnt = src16_cnt_to_hw(s);
		/* we turn on descriptor write back error status */
		if (ioat_dma->cap & IOAT_CAP_DWBES)
			pq->ctl_f.wb_en = result ? 1 : 0;
		pq->ctl_f.p_disable = !!(flags & DMA_PREP_PQ_DISABLE_P);
		pq->ctl_f.q_disable = !!