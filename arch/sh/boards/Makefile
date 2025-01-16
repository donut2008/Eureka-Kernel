;
		return ret;
	}
}

/**
 * ib_dma_free_coherent - Free memory allocated by ib_dma_alloc_coherent()
 * @dev: The device for which the DMA addresses were allocated
 * @size: The size of the region
 * @cpu_addr: the address returned by ib_dma_alloc_coherent()
 * @dma_handle: the DMA address returned by ib_dma_alloc_coherent()
 */
static inline void ib_dma_free_coherent(struct ib_device *dev,
					size_t size, void *cpu_addr,
					u64 dma_handle)
{
	if (dev->dma_ops)
		dev->dma_ops->free_coherent(dev, size, cpu_addr, dma_handle);
	else
		dma_free_coherent(dev->dma_device, size, cpu_addr, dma_handle);
}

/**
 * ib_query_mr - Retrieves information about a specific memory region.
 * @mr: The memory region t