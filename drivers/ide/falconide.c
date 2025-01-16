k_t prep_lock;
};

struct ioat_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct dma_chan *, char *);
};

/**
 * struct ioat_sed_ent - wrapper around super extended hardware descriptor
 * @hw: hardware SED
 * @dma: dma address for the SED
 * @parent: point to the dma descriptor that's the parent
 * @hw_pool: descriptor pool index
 */
struct ioat_sed_ent {
	struct ioat_sed_raw_descriptor *hw;
	dma_addr_t dma;
	struct ioat_ring_ent *parent;
	unsigned int hw_pool;
};

/**
 * struct ioat_ring_ent - wrapper around hardware descriptor
 * @hw: hardware DMA descriptor (for memcpy)
 * @xor: hardware xor descriptor
 * @xor_ex: hardware xor extension descriptor
 * @pq: hardware pq descriptor
 * @pq_ex: hardware pq extension descriptor
 * @pqu: hardware pq update descriptor
 * @raw: hardware raw (un-typed) descriptor
 * @txd: the generic software descriptor for all engines
 * @len: total transaction length for unmap
 * @result: asynchronous result of validate operations
 * @id: identifier for debug
 * @sed: pointer to super extended descriptor sw desc
 */

struct ioat_ring_ent {
	union {
		struct ioat_dma_descriptor *hw;
		struct ioat_xor_descriptor *xor;
		struct ioat_xor_ext_descriptor *xor_ex;
		struct ioat_pq_descriptor *pq;
		struct ioat_pq_ext_descriptor *pq_ex;
		struct ioat_pq_update_descriptor *pqu;
		struct ioat_raw_descriptor *raw;
	};
	size_t len;
	struct dma_async_tx_descriptor txd;
	enum sum_check_flags *result;
	#ifdef DEBUG
	int id;
	#endif
	struct ioat_sed_ent *sed;
};

extern const struct sysfs_ops ioat_sysfs_ops;
extern struct ioat_sysfs_entry ioat_version_attr;
extern struct ioat_sysfs_entry ioat_cap_attr;
extern int ioat_pending_level;
extern int ioat_ring_alloc_order;
extern struct kobj_type ioat_ktype;
extern struct kmem_cache *ioat_cache;
extern int ioat_ring_max_alloc_order;
extern struct kmem_cache *ioat_sed_cache;

static inline struct ioatdma_chan *to_ioat_chan(struct dma_chan *c)
{
	return container_of(c, struct ioatdma_chan, dma_chan);
}

/* wrapper around hardware descriptor format + additional software fields */
#ifdef DEBUG
#define set_desc_id(desc, i) ((desc)->id = (i))
#define desc_id(desc) ((desc)->id)
#else
#define set_desc_id(desc, i)
#define desc_id(desc) (0)
#endif

static inline void
__dump_desc_dbg(struct ioatdma_chan *ioat_chan, struct ioat_dma_descriptor *hw,
		struct dma_async_tx_descriptor *tx, int id)
{
	struct device *dev = to_dev(ioat_chan);

	dev_dbg(dev, "desc[%d]: (%#llx->%#llx) cookie: %d flags: %#x"
		" ctl: %#10.8x (op: %#x int_en: %d compl: %d)\n", id,
		(unsigned long long) tx->phys,
		(unsigned long long) hw->next, tx->cookie, tx->flags,
		hw->ctl, hw->ctl_f.op, hw->ctl_f.int_en, hw->ctl_f.compl_write);
}

#define dump_desc_dbg(c, d) \
	({ if (d) __dump_desc_dbg(c, d->hw, &d->txd, desc_id(d)); 0; })

static inline struct ioatdma_chan *
ioat_chan_by_index(struct ioatdma_device *ioat_dma, int index)
{
	return ioat_dma->idx[index];
}

static inline u64 ioat_chansts_32(struct ioatdma_chan *ioat_chan)
{
	u8 ver = ioat_chan->ioat_dma->version;
	u64 status;
	u32 status_lo;

	/* We need to read the low address first as this causes the
	 * chipset to latch the upper bits for the subsequent read
	 */
	status_lo = readl(ioat_chan->reg_base + IOAT_CHANSTS_OFFSET_LOW(ver));
	status = readl(ioat_chan->reg_base + IOAT_CHANSTS_OFFSET_HIGH(ver));
	status <<= 32;
	status |= status_lo;

	return status;
}

#if BITS_PER_LONG == 64

static inline u64 ioat_chansts(struct ioatdma_chan *ioat_chan)
{
	u8 ver = ioat_chan->ioat_dma->version;
	u64 status;

	 /* With IOAT v3.3 the status register is 64bit.  */
	if (ver >= IOAT_VER_3_3)
		status = readq(ioat_chan->reg_base + IOAT_CHANSTS_OFFSET(ver));
	else
		status = ioat_chansts_32(ioat_chan);

	return status;
}

#else
#define ioat_chansts ioat_chansts_32
#endif

static inline u64 ioat_chansts_to_addr(u64 status)
{
	return status & IOAT_CHANSTS_COMPLETED_DESCRIPTOR_ADDR;
}

static inline u32 ioat_chanerr(struc