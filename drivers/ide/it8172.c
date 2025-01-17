ma_async_tx_descriptor *
ioat_prep_pqxor_val(struct dma_chan *chan, dma_addr_t *src,
		     unsigned int src_cnt, size_t len,
		     enum sum_check_flags *result, unsigned long flags)
{
	unsigned char scf[MAX_SCF];
	dma_addr_t pq[2];
	struct ioatdma_chan *ioat_chan = to_ioat_chan(chan);

	if (test_bit(IOAT_CHAN_DOWN, &ioat_chan->state))
		return NULL;

	if (src_cnt > MAX_SCF)
		return NULL;

	/* the cleanup routine only sets bits on validate failure, it
	 * does not clear bits on validate success... so clear it here
	 */
	*result = 0;

	memset(scf, 0, src_cnt);
	pq[0] = src[0];
	flags |= DMA_PREP_PQ_DISABLE_Q;
	pq[1] = pq[0]; /* specify valid address for disabled result */

	return src_cnt_flags(src_cnt, flags) > 8 ?
		__ioat_prep_pq16_lock(chan, result, pq, &src[1], src_cnt - 1,
				       scf, len, flags) :
		__ioat_prep_pq_lock(chan, result, pq, &src[1], src_cnt - 1,
				     scf, len, flags);
}

struct dma_async_tx_descriptor *
ioat_prep_interrupt_lock(struct dma_chan *c, unsigned long flags)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);
	struct ioat_ring_ent *desc;
	struct ioat_dma_descriptor *hw;

	if (test_bit(IOAT_CHAN_DOWN, &ioat_chan->state))
		return NULL;

	if (ioat_check_space_lock(ioat_chan, 1) == 0)
		desc = ioat_get_ring_ent(ioat_chan, ioat_chan->head);
	else
		return NULL;

	hw = desc->hw;
	hw->ctl = 0;
	hw->ctl_f.null = 1;
	hw->ctl_f.int_en = 1;
	hw->ctl_f.fence = !!(flags & DMA_PREP_FENCE);
	hw->ctl_f.compl_write = 1;
	hw->size = NULL_DESC_BUFFER_SIZE;
	hw->src_addr = 0;
	hw->dst_addr = 0;

	desc->txd.flags = flags;
	desc->len = 1;

	dump_desc_dbg(ioat_chan, desc);

	/* we leave the channel locked to ensure in order submission */
	return &desc->txd;
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright(c) 2004 - 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called COPYING.
 */
#ifndef _IOAT_REGISTERS_H_
#define _IOAT_REGISTERS_H_

#define IOAT_PCI_DMACTRL_OFFSET			0x48
#define IOAT_PCI_DMACTRL_DMA_EN			0x00000001
#define IOAT_PCI_DMACTRL_MSI_EN			0x00000002

#define IOAT_PCI_DEVICE_ID_OFFSET		0x02
#define IOAT_PCI_DMAUNCERRSTS_OFFSET		0x148
#define IOAT_PCI_CHANERR_INT_OFFSET		0x180
#define IOAT_PCI_CHANERRMASK_INT_OFFSET		0x184

/* MMIO Device Registers */
#define IOAT_CH