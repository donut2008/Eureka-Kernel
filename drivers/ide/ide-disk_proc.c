
	reg = idmac_read_icreg(ipu, IDMAC_CHA_EN);
	idmac_write_icreg(ipu, reg & ~chan_mask, IDMAC_CHA_EN);

	spin_unlock_irqrestore(&ipu->lock, flags);

	return 0;
}

static struct scatterlist *idmac_sg_next(struct idmac_channel *ichan,
	struct idmac_tx_desc **desc, struct scatterlist *sg)
{
	struct scatterlist *sgnew = sg ? sg_next(sg) : NULL;

	if (sgnew)
		/* next sg-element in this list */
		return sgnew;

	if ((*desc)->list.next == &ichan->queue)
		/* No more descriptors on the queue */
		return NULL;

	/* Fetch next descriptor */
	*desc = list_entry((*desc)->list.next, struct idmac_tx_desc, list);
	return (*desc)->sg;
}

/*
 * We have several possibilities here:
 * current BUF		next BUF
 *
 * not last sg		next not last sg
 * not last sg		next last sg
 * last sg		first sg from next descriptor
 * last sg		NULL
 *
 * Besides, the descriptor queue might be empty or not. We process all these
 * cases carefully.
 */
static irqreturn_t idmac_interrupt(int irq, void *dev_id)
{
	struct idmac_channel *ichan = dev_id;
	struct device *dev = &ichan->dma_chan.dev->device;
	unsigned int chan_id = ichan->dma_chan.chan_id;
	struct scatterlist **sg, *sgnext, *sgnew = NULL;
	/* Next transfer descriptor */
	struct idmac_tx_desc *desc, *descnew;
	dma_async_tx_callback callback;
	void *callback_param;
	bool done = false;
	u32 ready0, ready1, curbuf, err;
	unsigned long flags;

	/* IDMAC has cleared the respective BUFx_RDY bit, we manage the buffer */

	dev_dbg(dev, "IDMAC irq %d, buf %d\n", irq, ichan->active_buffer);

	spin_lock_irqsave(&ipu_data.lock, flags);

	ready0	= idmac_read_ipureg(&ipu_data, IPU_CHA_BUF0_RDY);
	ready1	= idmac_read_ipureg(&ipu_data, IPU_CHA_BUF1_RDY);
	curbuf	= idmac_read_ipureg(&ipu_data, IPU_CHA_CUR_BUF);
	err	= idmac_read_ipureg(&ipu_data, IPU_INT_STAT_4);

	if (err & (1 << chan_id)) {
		idmac_write_ipureg(&ipu_data, 1 << chan_id, IPU_INT_STAT_4);
		spin_unlock_irqrestore(&ipu_data.lock, flags);
		/*
		 * Doing this
		 * ichan->sg[0] = ichan->sg[1] = NULL;
		 * you can force channel re-enable on the next tx_submit(), but
		 * this is dirty - think about descriptors with multiple
		 * sg elements.
		 */
		dev_warn(dev, "NFB4EOF on channel %d, ready %x, %x, cur %x\n",
			 chan_id, ready0, ready1, curbuf);
		return IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&ipu_data.lock, flags);

	/* Other interrupts do not interfere with this channel */
	spin_lock(&ichan->lock);
	if (unlikely((ichan->active_buffer && (ready1 >> chan_id) & 1) ||
		     (!ichan->active_buffer && (ready0 >> chan_id) & 1)
		     )) {
		spin_unlock(&ichan->lock);
		dev_dbg(dev,
			"IRQ with active buffer still ready on channel %x, "
			"active %d, ready %x, %x!\n", chan_id,
			ichan->active_buffer, ready0, ready1);
		return IRQ_NONE;
	}

	if (unlikely(list_empty(&ichan->queue))) {
		ichan->sg[ichan->active_buffer] = NULL;
		spin_unlock(&ichan->lock);
		dev_err(dev,
			"IRQ without queued buffers on channel %x, active %d, "
			"ready %x, %x!\n", chan_id,
			ichan->active_buffer, ready0, ready1);
		return IRQ_NONE;
	}

	/*
	 * active_buffer is a software flag, it shows which buffer we are
	 * currently expecting back from the hardware, IDMAC should be
	 * processing the other buffer already
	 */
	sg = &ichan->sg[ichan->active_buffer];
	sgnext = ichan->sg[!ichan->active_buffer];

	if (!*sg) {
		spin_unlock(&ichan->lock);
		return IRQ_HANDLED;
	}

	desc = list_entry(ichan->queue.next, struct idmac_tx_desc, list);
	descnew = desc;

	dev_dbg(dev, "IDMAC irq %d, dma %#llx, next dma %#llx, current %d, curbuf %#x\n",
		irq, (u64)sg_dma_address(*sg),
		sgnext ? (u64)sg_dma_address(sgnext) : 0,
		ichan->active_buffer, curbuf);

	/* Find the descriptor of sgnext */
	sgnew = idmac_sg_next(ichan, &descnew, *sg);
	if (sgnext != sgnew)
		dev_err(dev, "Submitted buffer %p, next buffer %p\n", sgnext, sgnew);

	/*
	 * if sgnext == NULL sg must be the last element in a scatterlist and
	 * queue must be empty
	 */
	if (unlikely(!sgnext)) {
		if (!WARN_ON(sg_next(*sg)))
			dev_dbg(dev, "Underrun on channel %x\n", chan_id);
		ichan->sg[!ichan->active_buffer] = sgnew;

		if (unlikely(sgnew)) {
			ipu_submit_buffer(ichan, descnew, sgnew, !ichan->active_buffer);
		} else {
			spin_lock_irqsave(&ipu_data.lock, flags);
			ipu_ic_disable_task(&ipu_data, chan_id);
			spin_unlock_irqrestore(&ipu_data.lock, flags);
			ichan->status = IPU_CHANNEL_READY;
			/* Continue to check for complete descriptor */
		}
	}

	/* Calculate and submit the next sg element */
	sgnew = idmac_sg_next(ichan, &descnew, sgnew);

	if (unlikely(!sg_next(*sg)) || !sgnext) {
		/*
		 * Last element in scatterlist done, remove from the queue,
		 * _init for debugging
		 */
		list_del_init(&desc->list);
		done = true;
	}

	*sg =