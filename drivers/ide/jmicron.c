c void iop_chan_start_null_memcpy(struct iop_adma_chan *iop_chan)
{
	struct iop_adma_desc_slot *sw_desc, *grp_start;
	dma_cookie_t cookie;
	int slot_cnt, slots_per_op;

	dev_dbg(iop_chan->device->common.dev, "%s\n", __func__);

	spin_lock_bh(&iop_chan->lock);
	slot_cnt = iop_chan_memcpy_slot_count(0, &slots_per_op);
	sw_desc = iop_adma_alloc_slots(iop_chan, slot_cnt, slots_per_op);
	if (sw_desc) {
		grp_start = sw_desc->group_head;

		list_splice_init(&sw_desc->tx_list, &iop_chan->chain);
		async_tx_ack(&sw_desc->async_tx);
		iop_desc_init_memcpy(grp_start, 0);
		iop_desc_set_byte_count(grp_start, iop_chan, 0);
		iop_desc_set_dest_addr(grp_start, iop_chan, 0);
		iop_desc_set_memcpy_src_addr(grp_start, 0);

		cookie = dma_cookie_assign(&sw_desc->async_tx);

		/* initialize the completed cookie to be less than
		 * the most recently used cookie
		 */
		iop_chan->common.completed_cookie = cookie - 1;

		/* channel should not be busy */
		BUG_ON(iop_chan_is_busy(iop_chan));

		/* clear any prior error-status bits */
		iop_adma_device_clear_err_status(iop_chan);

		/* disable operation */
		iop_chan_disable(iop_chan);

		/* set the descriptor address */
		iop_chan_set_next_descriptor(iop_chan, sw_desc->async_tx.phys);

		/* 1/ don't add pre-chained descriptors
		 * 2/ dummy read to flush next_desc write
		 */
		BUG_ON(iop_desc_get_next_desc(sw_desc));

		/* run the descriptor */
		iop_chan_enable(iop_chan);
	} else
		dev_err(iop_chan->device->common.dev,
			"failed to allocate null descriptor\n");
	spin_unlock_bh(&iop_chan->lock);
}

static void iop_chan_start_null_xor(struct iop_adma_chan *iop_chan)
{
	struct iop_adma_desc_slot *sw_desc, *grp_start;
	dma_cookie_t cookie;
	int slot_cnt, slots_per_op;

	dev_dbg(iop_chan->device->common.dev, "%s\n", __func__);

	spin_lock_bh(&iop_chan->lock);
	slot_cnt = iop_chan_xor_slot_count(0, 2, &slots_per_op);
	sw_desc = iop_adma_alloc_slots(iop_chan, slot_cnt, slots_per_op);
	if (sw_desc) {
		grp_start = sw_desc->group_head;
		list_splice_init(&sw_desc->tx_list, &iop_chan->chain);
		async_tx_ack(&sw_desc->async_tx);
		iop_desc_init_null_xor(grp_start, 2, 0);
		iop_desc_set_byte_count(grp_start, iop_chan, 0);
		iop_desc_set_dest_addr(grp_start, iop_chan, 0);
		iop_desc_set_xor_src_addr(grp_start, 0, 0);
		iop_desc_set_xor_src_addr(grp_start, 1, 0);

		cookie = dma_cookie_assign(&sw_desc->async_tx);

		/* initialize the completed cookie to be less than
		 * the most recently used cookie
		 */
		iop_chan->common.completed_cookie = cookie - 1;

		/* channel should not be busy */
		BUG_ON(iop_chan_is_busy(iop_chan));

		/* clear any prior error-status bits */
		iop_adma_device_clear_err_status(iop_chan);

		/* disable operation */
		iop_chan_disable(iop_chan);

		/* set the descriptor address */
		iop_chan_set_next_descriptor(iop_chan, sw_desc->async_tx.phys);

		/* 1/ don't add pre-chained descriptors
		 * 2/ dummy read to flush next_desc write
		 */
		BUG_ON(iop_desc_get_next_desc(sw_desc));

		/* run the descriptor */
		iop_chan_enable(iop_chan);
	} else
		dev_err(iop_chan->device->common.dev,
			"failed to allocate null descriptor\n");
	spin_unlock_bh(&iop_chan->lock);
}

static struct platform_driver iop_adma_driver = {
	.probe		= iop_adma_probe,
	.remove		= iop_adma_remove,
	.driver		= {
		.name	= "iop-adma",
	},
};

module_platform_driver(iop_adma_driver);

MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("IOP ADMA Engine Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:iop-adma");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          obj-y	+= ipu_irq.o ipu_idmac.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                   