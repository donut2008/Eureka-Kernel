ansactions, where all
		 * descriptors have the ENDIRQEN bit set, and for which we
		 * can't have multiple transactions on one channel anyway.
		 */
		if (cyclic || !(sw->desc.dcmd & DCMD_ENDIRQEN))
			continue;

		if (sw->async_tx.cookie == cookie) {
			return residue;
		} else {
			residue = 0;
			passed = false;
		}
	}

	/* We should only get here in case of cyclic transactions */
	return residue;
}

static enum dma_status mmp_pdma_tx_status(struct dma_chan