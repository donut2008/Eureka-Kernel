ma_async_tx_descriptor_init(&desc->async_tx, dchan);
		desc->async_tx.tx_submit = nbpf_tx_submit;
		desc->chan = chan;
		INIT_LIST_HEAD(&desc->sg);
		list_add_tail(&desc->node, &head);
	}

	/*
	 * This function cannot be called from interrupt context, so, no need to
	 * save flags
	 */
	spin_lock_irq(&chan->lock);
	list_splice_tail(&lhead, &chan->free_links);
	list_splice_tail(&head, &chan->free);
	list_add(&dpage->node, &chan->desc_page);
	spin_unlock_irq(&chan->lock);

	return ARRAY_SIZE(dpage->desc);
}

static void nbpf_desc_put(struct nbpf_desc *desc)
{
	struct nbpf_channel *chan = desc->chan;
	struct nbpf_link_desc *ldesc, *tmp;
	unsigned long flags;

	spin_lock_irqsave(&chan->lock, flags);
	list_for_each_entry_safe(ldesc, tmp, &desc->sg, node)
		list_move(&ldesc->node, &chan->free_links);

	list_add(&desc->node, &chan->free);
	spin_unlock_irqrestore(&chan->lock, flags);
}

static void nbpf_scan_acked(struct nbpf_channel *chan)
{
	struct nbpf_desc *desc, *tmp;
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&chan->lock, flags);
	list_for_each_entry_safe(desc, tmp, &chan->done, node)
		if (async_tx_test_ack(&desc->async_tx) && desc->user_wait) {
			list_move(&desc->node, &head);
			desc->user_wait = false;
		}
	spin_unlock_irqrestore(&chan->lock, flags);

	list_for_each_entry_safe(desc, tmp, &head, node) {
		list_del(&desc->node);
		nbpf_desc_put(desc);
	}
}

/*
 * We have to allocate descriptors with the channel lock dropped. This means,
 * before we re-acquire the lock buffers can be taken already, so we have to
 * re-check after re-acquiring the lock and possibly retry, if buffers are gone
 * again.
 */
static struct nbpf_desc *nbpf_desc_get(struct nbpf_channel *chan, size_t len)
{
	struct nbpf_desc *desc = NULL;
	struct nbpf_link_desc *ldesc, *prev = NULL;

	nbpf_scan_acked(chan);

	spin_lock_irq(&chan->lock);

	do {
		int i = 0, ret;

		if (list_empty(&chan->free)) {
			/* No more free descriptors */
			spin_unlock_irq(&chan->lock);
			ret = nbpf_desc_page_alloc(chan);
			if (ret < 0)
				return NULL;
			spin_lock_irq(&chan->lock);
			continue;
		}
		desc = list_first_entry(&chan->free, struct nbpf_desc, node);
		list_del(&desc->node);

		do {
			if (list_empty(&chan->free_links)) {
				/* No more free link descriptors */
				spin_unlock_irq(&chan->lock);
				ret = nbpf_desc_page_alloc(chan);
				if (ret < 0) {
					nbpf_desc_put(desc);
					return NULL;
				}
				spin_lock_irq(&chan->lock);
				continue;
			}

			ldesc = list_first_entry(&chan->free_links,
						 struct nbpf_link_desc, node);
			ldesc->desc = desc;
			if (prev)
				prev->hwdesc->next = (u32)ldesc->hwdesc_dma_addr;

			prev = ldesc;
			list_move_tail(&ldesc->node, &desc->sg);

			i++;
		} while (i < len);
	} while (!desc);

	prev->hwdesc->next = 0;

	spin_unlock_irq(&chan->lock);

	return desc;
}

static void nbpf_chan_idle(struct nbpf_channel *chan)
{
	struct nbpf_desc *desc, *tmp;
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&chan->lock, flags);

	list_splice_init(&chan->done, &head);
	list_splice_init(&chan->active, &head);
	list_splice_init(&chan->queued, &head);

	chan->running = NULL;

	spin_unlock_irqrestore(&chan->lock, flags);

	list_for_each_entry_safe(desc, tmp, &head, node) {
		dev_dbg(chan->nbpf->dma_dev.dev, "%s(): force-free desc %p cookie %d\n",
			__func__, desc, desc->async_tx.cookie);
		list_del(&desc->node);
		nbpf_desc_put(desc);
	}
}

static int nbpf_pause(struct dma_chan *dchan)
{
	struct nbpf_channel *chan = nbpf_to_chan(dchan);

	dev_dbg(dchan->device->dev, "Entry %s\n", __func__);

	chan->paused = true;
	nbpf_chan_write(chan, NBPF_CHAN_CTRL, NBPF_CHAN_CTRL_SETSUS);
	/* See comment in nbpf_prep_one() */
	nbpf_chan_write(chan, NBPF_CHAN_CTRL, NBPF_CHAN_CTRL_CLREN);

	return 0;
}

static int nbpf_terminate_all(struct dma_chan *dchan)
{
	struct nbpf_channel *chan = nbpf_to_chan(dchan);

	dev_dbg(dchan->device->dev, "Entry %s\n", __func__);
	dev_dbg(dchan->dev