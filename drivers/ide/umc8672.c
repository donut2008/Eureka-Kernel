a_chan *ioat_chan)
{
	u8 ver = ioat_chan->ioat_dma->version;

	writeb(IOAT_CHANCMD_SUSPEND,
	       ioat_chan->reg_base + IOAT_CHANCMD_OFFSET(ver));
}

static inline void ioat_reset(struct ioatdma_chan *ioat_chan)
{
	u8 ver = ioat_chan->ioat_dma->version;

	writeb(IOAT_CHANCMD_RESET,
	       ioat_chan->reg_base + IOAT_CHANCMD_OFFSET(ver));
}

static inline bool ioat_reset_pending(struct ioatdma_chan *ioat_chan)
{
	u8 ver = ioat_chan->ioat_dma->version;
	u8 cmd;

	cmd = readb(ioat_chan->reg_base + IOAT_CHANCMD_OFFSET(ver));
	return (cmd & IOAT_CHANCMD_RESET) == IOAT_CHANCMD_RESET;
}

static inline bool is_ioat_active(unsigned long status)
{
	return ((status & IOAT_CHANSTS_STATUS) == IOAT_CHANSTS_ACTIVE);
}

static inline bool is_ioat_idle(unsigned long status)
{
	return ((status & IOAT_CHANSTS_STATUS) == IOAT_CHANSTS_DONE);
}

static inline bool is_ioat_halted(unsigned long status)
{
	return ((status & IOAT_CHANSTS_STATUS) == IOAT_CHANSTS_HALTED);
}

static inline bool is_ioat_suspended(unsigned long status)
{
	return ((status & IOAT_CHANSTS_STATUS) == IOAT_CHANSTS_SUSPENDED);
}

/* channel was fatally programmed */
static inline bool is_ioat_bug(unsigned long err)
{
	return !!err;
}

#define IOAT_MAX_ORDER 16
#define ioat_get_alloc_order() \
	(min(ioat_ring_alloc_order, IOAT_MAX_ORDER))
#define ioat_get_max_alloc_order() \
	(min(ioat_ring_max_alloc_order, IOAT_MAX_ORDER))

static inline u32 ioat_ring_size(struct ioatdma_chan *ioat_chan)
{
	return 1 << ioat_chan->alloc_order;
}

/* count of descriptors in flight with the engine */
static inline u16 ioat_ring_active(struct ioatdma_chan *ioat_chan)
{
	return CIRC_CNT(ioat_chan->head, ioat_chan->tail,
			ioat_ring_size(ioat_chan));
}

/* count of descriptors pending submission to hardware */
static inline u16 ioat_ring_pending(struct ioatdma_chan *ioat_chan)
{
	return CIRC_CNT(ioat_chan->head, ioat_chan->issued,
			ioat_ring_size(ioat_chan));
}

static inline u32 ioat_ring_space(struct ioatdma_chan *ioat_chan)
{
	return ioat_ring_size(ioat_chan) - ioat_ring_active(ioat_chan);
}

static inline u16
ioat_xferlen_to_descs(struct ioatdma_chan *ioat_chan, size_t len)
{
	u16 num_descs = len >> ioat_chan->xfercap_log;

	num_descs += !!(len & ((1 << ioat_chan->xfercap_log) - 1));
	return num_descs;
}

static inline struct ioat_ring_ent *
ioat_get_ring_ent(struct ioatdma_chan *ioat_chan, u16 idx)
{
	return ioat_chan->ring[idx & (ioat_ring_size(ioat_chan) - 1)];
}

static inline void
ioat_set_chainaddr(struct ioatdma_chan *ioat_chan, u64 addr)
{
	writel(addr & 0x00000000FFFFFFFF,
	       ioat_chan->reg_base + IOAT2_CHAINADDR_OFFSET_LOW);
	writel(addr >> 32,
	       ioat_chan->reg_base + IOAT2_CHAINADDR_OFFSET_HIGH);
}

/* IOAT Prep functions */
struct dma_async_tx_descriptor *
ioat_dma_prep_memcpy_lock(struct dma_chan *c, dma_addr_t dma_dest,
			   dma_addr_t dma_src, size_t len, unsigned long flags);
struct dma_async_tx_descriptor *
ioat_prep_interrupt_lock(struct dma_chan *c, unsigned long flags);
struct dma_async_tx_descriptor *
ioat_prep_xor(struct dma_chan *chan, dma_addr_t dest, dma_addr_t *src,
	       unsigned int src_cnt, size_t len, unsigned long flags);
struct dma_async_tx_descriptor *
ioat_prep_xor_val(struct dma_chan *chan, dma_addr_t *src,
		    unsigned int src_cnt, size_t len,
		    enum sum_check_flags *result, unsigned long flags);
struct dma_async_tx_descriptor *
ioat_prep_pq(struct dma_chan *chan, dma_addr_t *dst, dma_addr_t *src,
	      unsigned int src_cnt, const unsigned char *scf, size_t len,
	      unsigned long flags);
struct dma_async_tx_descriptor *
ioat_prep_pq_val(struct dma_chan *chan, dma_addr_t *pq, dma_addr_t *src,
		  unsigned int src_cnt, const unsigned char *scf, size_t len,
		  enum sum_check_flags *pqres, unsigned long flags);
struct dma_async_tx_descriptor *
ioat_prep_pqxor(struct dma_chan *chan, dma_addr_t dst, dma_addr_t *src,
		 unsigned int src_cnt, size_t len, unsigned long flags);
struct dma_async_tx_descriptor *
ioat_prep_pqxor_val(struct dma_chan *chan, dma_addr_t *src,
		     unsigned int src_cnt, size_t len,
		     enum sum_check_flags *result, unsigned long flags);

/* IOAT Operation functions */
irqreturn_t ioat_dma_do_interrupt(int irq, void *data);
irqreturn_t ioat_dma_do_interrupt_msix(int irq, void *data);
struct ioat_ring_ent **
ioat_alloc_ring(struct dma_chan *c, int order, gfp_t flags);
void ioat_start_null_desc(struct ioatdma_chan *ioat_chan);
void ioat_free_ring_ent(struct ioat_ring_ent *desc, struct dma_chan *chan);
int ioat_reset_hw(struct ioatdma_chan *ioat_chan);
enum dma_status
ioat_tx_status(struct dma_chan *c, dma_cookie_t cookie,
		struct dma_tx_state *txstate);
void ioat_cleanup_event(unsigned long data);
void ioat_timer_event(unsigned long data);
int ioat_check_space_lock(struct ioatdma_chan *ioat_chan, int num_descs);
void ioat_issue_pending(struct dma_chan *chan);
void ioat_timer_event(unsigned long data);

/* IOAT Init functions */
bool is_bwd_ioat(struct pci_dev *pdev);
struct dca_provider *ioat_dca_init(struct pci_dev *pdev, void __iomem *iobase);
void ioat_kobject_add(struct ioatdma