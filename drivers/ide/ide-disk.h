lse
		dcar &= ~MIC_DMA_SBOX_DCAR_IM1;
	mic_dma_write_reg(ch, MIC_DMA_REG_DCAR, dcar);
}

static void mic_dma_ack_interrupt(struct mic_dma_chan *ch)
{
	if (MIC_DMA_CHAN_MIC == ch->owner) {
		/* HW errata */
		mic_dma_chan_mask_intr(ch);
		mic_dma_chan_unmask_intr(ch);
	}
	to_mbus_hw_ops(ch)->ack_interrupt(to_mbus_device(ch), ch->ch_num);
}
#endif
                                                                                                                                                                                                                                                                                                                              