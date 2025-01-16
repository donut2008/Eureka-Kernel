addresses
 * @dev: The device for which the DMA addresses are to be created
 * @sg: The array of scatter/gather entries
 * @nents: The number of scatter/gather entries
 * @direction: The direction of the DMA
 */
static inline int ib_dma_map_sg(struct ib_device *dev,
				struct scatterlist *sg, int nents,
				enum dma_data_direction direction)
{
	if (dev->dma_ops)
		return dev->dma_ops->map_sg(dev, sg, nents, direction);
	return dma_map_sg(dev->dma_device, sg, nents, direction);
}

/**
 * ib_dma_unmap_sg - Unmap a scatter/gather list of DMA addresses
 * @dev: The device for which the DMA addresses were created
 * @sg: The array of scatter/gather entries
 * @nents: The number of scatter/gather entries
 * @direction: The direction of the DMA
 */
static inline void ib_dma_unmap_sg(struct ib_device *dev,
				   struct scatterlist *sg, int nents,
				   enum dma_data_direction direction)
{
	if (dev->dma_ops)
		dev->dma_ops->unmap_sg(dev, sg, nents, direction);
	else
		dma_unmap_sg(dev->dma_device, sg, nents, direction);
}

static inline int ib_dma_map_sg_attrs(struct ib_device *dev,
				      struct scatterlist *sg, int nents,
				      enum dma_data_direction direction,
				      struct dma_attrs *attrs)
{
	return dma_map_sg_attrs(dev->dma_device, sg, nents, direction, attrs);
}

static inline void ib_dma_unmap_sg_attrs(struct ib_device *dev,
					 struct scatterlist *sg, int nents,
					 enum dma_data_direction direction,
					 struct dma_attrs *attrs)
{
	dma_unmap_sg_attrs(dev->dma_device, sg, nents, direction, attrs);
}
/**
 * ib_sg_dma_address - Return the DMA address from a scatter/gather entry
 * @dev: The device for which the DMA addresses were created
 * @sg: The scatter/gather entry
 *
 * Note: this function is