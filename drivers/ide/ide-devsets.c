c_cnt - 1, len, flags);
}

static void
dump_pq_desc_dbg(struct ioatdma_chan *ioat_chan, struct ioat_ring_ent *desc,
		 struct ioat_ring_ent *ext)
{
	struct device *dev = to_dev(ioat_chan);
	struct ioat_pq_descriptor *pq = desc->pq;
	struct ioat_pq_ext_descriptor *pq_ex = ext ? ext->pq_ex : NULL;
	struct ioat_raw_descriptor *descs[] = { (void *) pq, (void *) pq_ex };
	int src_cnt = src_cnt_to_sw(pq->ctl_f.src_cnt);
	int i;

	dev_dbg(dev, "desc[%d]: (%#llx->%#llx) flags: %#x"
		" sz: %#10.8x ctl: %#x (op: %#x int: %d compl: %d pq: '%s%s'"
		" src_cnt: %d)\n",
		desc_id(desc), (unsigned long long) desc->txd.phys,
		(unsigned long long) (pq_ex ? pq_ex->next : pq->next),
		desc->txd.flags, pq->size, pq->ctl, pq->ctl_f.op,
		pq->ctl_f.int_en, pq->ctl_f.compl_write,
		pq->ctl_f.p_disable ? "" : "p", pq->ctl_f.q_disable ? "" : "q",
		pq->ctl_f.src_cnt);
	for (i = 0; i < src_cnt; i++)
		dev_dbg(dev, "\tsrc[%d]: %#llx coef: %#x\n", i,
			(unsigned long long) pq_get_src(descs, i), pq->coef[i]);
	dev_dbg(dev, "\tP: %#llx\n", pq->p_addr);
	dev_dbg(dev, "\tQ: %#llx\n", pq->q_addr);
	dev_dbg(dev, "\tNEXT: %#llx\n", pq->next);
}

static void dump_pq16_desc_dbg(struct ioatdma_chan *ioat_chan,
			       struct ioat_ring_ent *desc)
{
	struct device *dev = to_dev(ioat_chan);
	struct ioat_pq_descriptor *pq = desc->pq;
	struct ioat_raw_descriptor *descs[] = { (void *)pq,
						(void *)pq,
						(void *)pq };
	int src_cnt = src16_cnt_to_sw(pq->ctl_f.src_cnt);
	int i;

	if (desc->sed) {
		descs[1] = (void *)desc->sed->hw;
		descs[2] = (void *)desc->sed->hw + 64;
	}

	dev_dbg(dev, "desc[%d]: (%#llx->%#llx) flags: %#x"
		" sz: %#x ctl: %#x (op: %#x int: %d compl: %d pq: '%s%s'"
		" src_cnt: %d)\n",
		desc_id(desc), (unsigned long long) desc->txd.phys,
		(unsigned long long) pq->next,
		desc->txd.flags, pq->size, pq->ctl,
		pq->ctl_f.op, pq->ctl_f.int_en,
		pq->ctl_f.compl_write,
		pq->ctl_f.p_disable ? "" : "p", pq->ctl_f.q_disable ? "" : "q",
		pq->ctl_f.src_cnt);
	for (i = 0; i < src_cnt; i++) {
		dev_dbg(dev, "\tsrc[%d]: %#llx coef: %#x\n", i,
			(unsigned long long) pq16_get_src(descs, i),
			pq->coef[i]);
	}
	dev_dbg(dev, "\tP: %#llx\n", pq->p_addr);
	dev_dbg(dev, "\tQ: %#llx\n", pq->q_addr);
}

static struct dma_async_tx_descriptor *
__ioat_prep_pq_lock(struct dma_chan *c, enum sum_check_flags *result,
		     const dma_addr_t *dst, const dma_addr_t *src,
		     unsigned int src_cnt, const unsigned char *scf,
		     size_t len, unsigned long flags)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);
	struct ioatdma_device *ioat_dma = ioat_chan->ioat_dma;
	struct ioat_ring_ent *compl_desc;
	struct ioat_ring_ent *desc;
	struct ioat_ring_ent *ext;
	size_t total_len = len;
	struct ioat_pq_descriptor *pq;
	struct ioat_pq_ext_descriptor *pq_ex = NULL;
	struct ioat_dma_descriptor *hw;
	u32 offset = 0;
	u8 op = result ? IOAT_OP_PQ_VAL : IOAT_OP_PQ;
	int i, s, idx, with_ext, num_descs;
	int cb32 = (ioat_dma->version < IOAT_VER_3_3) ? 1 : 0;

	dev_dbg(to_dev(ioat_chan), "%s\n", __func__);
	/* the engine requires at least two sources (we provide
	 * at least 1 implied source in the DMA_PREP_CONTINUE case)
	 */
	BUG_ON(src_cnt + dmaf_continue(flags) < 2);

	num_descs = ioat_xferlen_to_descs(ioat_chan, len);
	/* we need 2x the number of descriptors to cover greater than 3
	 * sources (we need 1 extra source in the q-only continuation
	 * case and 3 extra sources in the p+q continuation case.
	 */
	if (src_cnt + dmaf_p_disabled_continue(flags) > 3 ||
	    (dmaf_continue(flags) && !dmaf_p_disabled_continue(flags))) {
		with_ext = 1;
		num_descs *= 2;
	} else
		with_ext = 0;

	/* completion writes from the raid engine may pass completion
	 * writes from the legacy engine, so we need one extra null
	 * (legacy) descriptor to ensure all completion writes arrive in
	 * order.
	 */
	if (likely(num_descs) &&
	    ioat_check_space_lock(ioat_chan, num_descs + cb32) == 0)
		idx = ioat_chan-