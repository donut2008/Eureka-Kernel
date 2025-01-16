_HSW1:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW2:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW3:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW4:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW5:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW6:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW7:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW8:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW9:
		return true;
	default:
		return false;
	}

}

static bool is_bdx_ioat(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case PCI_DEVICE_ID_INTEL_IOAT_BDX0:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX1:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX2:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX3:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX4:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX5:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX6:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX7:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX8:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX9:
		return true;
	default:
		return false;
	}
}

static inline bool is_skx_ioat(struct pci_dev *pdev)
{
	return (pdev->device == PCI_DEVICE_ID_INTEL_IOAT_SKX) ? true : false;
}

static bool is_xeon_cb32(struct pci_dev *pdev)
{
	return is_jf_ioat(pdev) || is_snb_ioat(pdev) || is_ivb_ioat(pdev) ||
		is_hsw_ioat(pdev) || is_bdx_ioat(pdev) || is_skx_ioat(pdev);
}

bool is_bwd_ioat(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case PCI_DEVICE_ID_INTEL_IOAT_BWD0:
	case PCI_DEVICE_ID_INTEL_IOAT_BWD1:
	case PCI_DEVICE_ID_INTEL_IOAT_BWD2:
	case PCI_DEVICE_ID_INTEL_IOAT_BWD3:
	/* even though not Atom, BDX-DE has same DMA silicon */
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE0:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE1:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE2:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE3:
		return true;
	default:
		return false;
	}
}

static bool is_bwd_noraid(struct pci_dev *pdev)
{
	switch (pdev->device) {
	case PCI_DEVICE_ID_INTEL_IOAT_BWD2:
	case PCI_DEVICE_ID_INTEL_IOAT_BWD3:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE0:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE1:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE2:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE3:
		return true;
	default:
		return false;
	}

}

/*
 * Perform a IOAT transaction to verify the HW works.
 */
#define IOAT_TEST_SIZE 2000

static void ioat_dma_test_callback(void *dma_async_param)
{
	struct completion *cmp = dma_async_param;

	complete(cmp);
}

/**
 * ioat_dma_self_test - Perform a IOAT transaction to verify the HW works.
 * @ioat_dma: dma device to be tested
 */
static int ioat_dma_self_test(struct ioatdma_device *ioat_dma)
{
	int i;
	u8 *src;
	u8 *dest;
	struct dma_device *dma = &ioat_dma->dma_dev;
	struct device *dev = &ioat_dma->pdev->dev;
	struct dma_chan *dma_chan;
	struct dma_async_tx_descriptor *tx;
	dma_addr_t dma_dest, dma_src;
	dma_cookie_t cookie;
	int err = 0;
	struct completion cmp;
	unsigned long tmo;
	unsigned long flags;

	src = kzalloc(sizeof(u8) * IOAT_TEST_SIZE, GFP_KERNEL);
	if (!src)
		return -ENOMEM;
	dest = kzalloc(sizeof(u8) * IOAT_TEST_SIZE, GFP_KERNEL);
	if (!dest) {
		kfree(src);
		return -ENOMEM;
	}

	/* Fill in src buffer */
	for (i = 0; i < IOAT_TEST_SIZE; i++)
		src[i] = (u8)i;

	/* Start copy, using first DMA channel */
	dma_chan = container_of(dma->channels.next, struct dma_chan,
				device_node);
	if (dma->device_alloc_chan_resources(dma_chan) < 1) {
		dev_err(dev, "selftest cannot allocate chan resource\n");
		err = -ENODEV;
		goto out;
	}

	dma_src = dma_map_single(dev, src, IOAT_TEST_SIZE, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, dma_src)) {
		dev_err(dev, "mapping src buffer failed\n");
		goto free_resources;
	}
	dma_dest = dma_map_single(dev, dest, IOAT_TEST_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, dma_dest)) {
		dev_err(dev, "mapping dest buffer failed\n");
		goto unmap_src;
	}
	flags = DMA_PREP_INTERRUPT;
	tx = ioat_dma->dma_dev.device_prep_dma_memcpy(dma_chan, dma_dest,
						      dma_src, IOAT_TEST_SIZE,
						      flags);
	if (!tx) {
		dev_err(dev, "Self-test prep failed, disabling\n");
		err = -ENODEV;
		goto unmap_dma;
	}

	async_tx_ack(tx);
	init_completion(&cmp);
	tx->callback = ioat_dma_test_callback;
	tx->callback_param = &cmp;
	cookie = tx->tx_submit(tx);
	if (cookie < 0) {
		dev_err(dev, "Self-test setup failed, disabling\n");
		err = -ENODEV;
		goto unmap_dma;
	}
	dma->device_issue_pending(dma_chan);

	tmo = wait_for_completion_timeout(&cmp, msecs_to_jiffies(3000));

	if (tmo == 0 ||
	    dma->device_tx_status(dma_chan, cookie, NULL)
					!= DMA_COMPLETE) {
		dev_err(dev, "Self-test copy timed out, disabling\n");
		err = -ENODEV;
		goto unmap_dma;
	}
	if (memcmp(src, dest, IOAT_TEST_SIZE)) {
		dev_err(dev, "Self-test copy failed compare, disabling\n");
		err = -ENODEV;
		goto unmap_dma;
	}

unmap_dma:
	dma_unmap_single(dev, dma_dest, IOAT_TEST_SIZE, DMA_FROM_DEVICE);
unmap_src:
	dma_unmap_single(dev, dma_src, IOAT_TEST_SIZE, DMA_TO_DEVICE);
free_resources:
	dma->device_free_chan_resources(dma_chan);
out:
	kfree(src);
	kfree(dest);
	return err;
}

/**
 * ioat_dma_setup_interrupts - setup interrupt handler
 * @ioat_dma: ioat dma device
 */
int ioat_dma_setup_interrupts(struct ioatdma_device *ioat_dma)
{
	struct ioatdma_chan *ioat_chan;
	struct pci_dev *pdev = ioat_dma->pdev;
	struct device *dev = &pdev->dev;
	struct msix_entry *msix;
	int i, j, msixcnt;
	int err = -EINVAL;
	u8 intrctrl = 0;

	if (!strcmp(ioat_interrupt_style, "msix"))
		goto msix;
	if (!strcmp(ioat_interrupt_style, "msi"))
		goto msi;
	if (!strcmp(ioat_interrupt_style, "intx"))
		goto intx;
	dev_err(dev, "invalid ioat_interrupt_style %s\n", ioat_interrupt_style);
	goto err_no_irq;

msix:
	/* The number of MSI-X vectors should equal the number of channels */
	msixcnt = ioat_dma->dma_dev.chancnt;
	for (i = 0; i < msixcnt; i++)
		ioat_dma->msix_entries[i].entry = i;

	err = pci_enable_msix_exact(pdev, ioat_dma->msix_entries, msixcnt);
	if (err)
		goto msi;

	for (i = 0; i < msixcnt; i++) {
		msix = &ioat_dma->msix_entries[i];
		ioat_chan = ioat_chan_by_index(ioat_dma, i);
		err = devm_request_irq(dev, msix->vector,
				       ioat_dma_do_interrupt_msix, 0,
				       "ioat-msix", ioat_chan);
		if (err) {
			for (j = 0; j < i; j++) {
				msix = &ioat_dma->msix_entries[j];
				ioat_chan = ioat_chan_by_index(ioat_dma, j);
				devm_free_irq(dev, msix->vector, ioat_chan);
			}
			goto msi;
		}
	}
	intrctrl |= IOAT_INTRCTRL_MSIX_VECTOR_CONTROL;
	ioat_dma->irq_mode = IOAT_MSIX;
	goto done;

msi:
	err = pci_enable_msi(pdev);
	if (err)
		goto intx;

	err = devm_request_irq(dev, pdev->irq, ioat_dma_do_interrupt, 0,
			       "ioat-msi", ioat_dma);
	if (err) {
		pci_disable_msi(pdev);
		goto intx;
	}
	ioat_dma->irq_mode = IOAT_MSI;
	goto done;

intx:
	err = devm_request_irq(dev, pdev->irq, ioat_dma_do_interrupt,
			       IRQF_SHARED, "ioat-intx", ioat_dma);
	if (err)
		goto err_no_irq;

	ioat_dma->irq_mode = IOAT_INTX;
done:
	if (is_bwd_ioat(pdev))
		ioat_intr_quirk(ioat_dma);
	intrctrl |= IOAT_INTRCTRL_MASTER_INT_EN;
	writeb(intrctrl, ioat_dma->reg_base + IOAT_INTRCTRL_OFFSET);
	return 0;

err_no_irq:
	/* Disable all interrupt generation */
	writeb(0, ioat_dma->reg_base + IOAT_INTRCTRL_OFFSET);
	ioat_dma->irq_mode = IOAT_NOIRQ;
	dev_err(dev, "no usable interrupts\n");
	return err;
}

static void ioat_disable_interrupts(struct ioatdma_device *ioat_dma)
{
	/* Disable all interrupt generation */
	writeb(0, ioat_dma->reg_base + IOAT_INTRCTRL_OFFSET);
}

static int ioat_probe(struct ioatdma_device *ioat_dma)
{
	int err = -ENODEV;
	struct dma_device *dma = &ioat_dma->dma_dev;
	struct pci_dev *pdev = ioat_dma->pdev;
	struct device *dev = &pdev->dev;

	/* DMA coherent memory pool for DMA descriptor allocations */
	ioat_dma->dma_pool = pci_pool_create("dma_desc_pool", pdev,
					     sizeof(struct ioat_dma_descriptor),
					     64, 0);
	if (!ioat_dma->dma_pool) {
		err = -ENOMEM;
		goto err_dma_pool;
	}

	ioat_dma->completion_pool = pci_pool_create("completion_pool", pdev,
						    sizeof(u64),
						    SMP_CACHE_BYTES,
						    SMP_CACHE_BYTES);

	if (!ioat_dma->completion_pool) {
		err = -ENOMEM;
		goto err_completion_pool;
	}

	ioat_enumerate_channels(ioat_dma);

	dma_cap_set(DMA_MEMCPY, dma->cap_mask);
	dma->dev = &pdev->dev;

	if (!dma->chancnt) {
		dev_err(dev, "channel enumeration error\n");
		goto err_setup_interrupts;
	}

	err = ioat_dma_setup_interrupts(ioat_dma);
	if (err)
		goto err_setup_interrupts;

	err = ioat3_dma_self_test(ioat_dma);
	if (err)
		goto err_self_test;

	return 0;

err_self_test:
	ioat_disable_interrupts(ioat_dma);
err_setup_interrupts:
	pci_pool_destroy(ioat_dma->completion_pool);
err_completion_pool:
	pci_pool_destroy(ioat_dma->dma_pool);
err_dma_pool:
	return err;
}

static int ioat_register(struct ioatdma_device *ioat_dma)
{
	int err = dma_async_device_register(&ioat_dma->dma_dev);

	if (err) {
		ioat_disable_interrupts(ioat_dma);
		pci_pool_destroy(ioat_dma->completion_pool);
		pci_pool_destroy(ioat_dma->dma_pool);
	}

	return err;
}

static void ioat_dma_remove(struct ioatdma_device *ioat_dma)
{
	struct dma_device *dma = &ioat_dma->dma_dev;

	ioat_disable_interrupts(ioat_dma);

	ioat_kobject_del(ioat_dma);

	dma_async_device_unregister(dma);

	pci_pool_destroy(ioat_dma->dma_pool);
	pci_pool_destroy(ioat_dma->completion_pool);

	INIT_LIST_HEAD(&dma->channels);
}

/**
 * ioat_enumerate_channels - find and initialize the device's channels
 * @ioat_dma: the ioat dma device to be enumerated
 */
static void ioat_enumerate_channels(struct ioatdma_device *ioat_dma)
{
	struct ioatdma_chan *ioat_chan;
	struct device *dev = &ioat_dma->pdev->dev;
	struct dma_device *dma = &ioat_dma->dma_dev;
	u8 xfercap_log;
	int i;

	INIT_LIST_HEAD(&dma->channels);
	dma->chancnt = readb(ioat_dma->reg_base + IOAT_CHANCNT_OFFSET);
	dma->chancnt &= 0x1f; /* bits [4:0] valid */
	if (dma->chancnt > ARRAY_SIZE(ioat_dma->idx)) {
		dev_warn(dev, "(%d) exceeds max supported channels (%zu)\n",
			 dma->chancnt, ARRAY_SIZE(ioat_dma->idx));
		dma->chancnt = ARRAY_SIZE(ioat_dma->idx);
	}
	xfercap_log = readb(ioat_dma->reg_base + IOAT_XFERCAP_OFFSET);
	xfercap_log &= 0x1f; /* bits [4:0] valid */
	if (xfercap_log == 0)
		return;
	dev_dbg(dev, "%s: xfercap = %d\n", __func__, 1 << xfercap_log);

	for (i = 0; i < dma->chancnt; i++) {
		ioat_chan = devm_kzalloc(dev, sizeof(*ioat_chan), GFP_KERNEL);
		if (!ioat_chan)
			break;

		ioat_init_channel(ioat_dma, ioat_chan, i);
		ioat_chan->xfercap_log = xfercap_log;
		spin_lock_init(&ioat_chan->prep_lock);
		if (ioat_reset_hw(ioat_chan)) {
			i = 0;
			break;
		}
	}
	dma->chancnt = i;
}

/**
 * ioat_free_chan_resources - release all the descriptors
 * @chan: the channel to be cleaned
 */
static void ioat_free_chan_resources(struct dma_chan *c)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);
	struct ioatdma_device *ioat_dma = ioat_chan->ioat_dma;
	struct ioat_ring_ent *desc;
	const int total_descs = 1 << ioat_chan->alloc_order;
	int descs;
	int i;

	/* Before freeing channel resources first check
	 * if they have been previously allocated for this channel.
	 */
	if (!ioat_chan->ring)
		return;

	ioat_stop(ioat_chan);
	ioat_reset_hw(ioat_chan);

	spin_lock_bh(&ioat_chan->cleanup_lock);
	spin_lock_bh(&ioat_chan->prep_lock);
	descs = ioat_ring_space(ioat_chan);
	dev_dbg(to_dev(ioat_chan), "freeing %d idle descriptors\n", descs);
	for (i = 0; i < descs; i++) {
		desc = ioat_get_ring_ent(ioat_chan, ioat_chan->head + i);
		ioat_free_ring_ent(desc, c);
	}

	if (descs < total_descs)
		dev_err(to_dev(ioat_chan), "Freeing %d in use descriptors!\n",
			total_descs - descs);

	for (i = 0; i < total_descs - descs; i++) {
		desc = ioat_get_ring_ent(ioat_chan, ioat_chan->tail + i);
		dump_desc_dbg(ioat_chan, desc);
		ioat_free_ring_ent(desc, c);
	}

	kfree(ioat_chan->ring);
	ioat_chan->ring = NULL;
	ioat_chan->alloc_order = 0;
	pci_pool_free(ioat_dma->completion_pool, ioat_chan->completion,
		      ioat_chan->completion_dma);
	spin_unlock_bh(&ioat_chan->prep_lock);
	spin_unlock_bh(&ioat_chan->cleanup_lock);

	ioat_chan->last_completion = 0;
	ioat_chan->completion_dma = 0;
	ioat_chan->dmacount = 0;
}

/* ioat_alloc_chan_resources - allocate/initialize ioat descriptor ring
 * @chan: channel to be initialized
 */
static int ioat_alloc_chan_resources(struct dma_chan *c)
{
	struct ioatdma_chan *ioat_chan = to_ioat_chan(c);
	struct ioat_ring_ent **ring;
	u64 status;
	int order;
	int i = 0;
	u32 chanerr;

	/* have we already been set up? */
	if (ioat_chan->ring)
		return 1 << ioat_chan->alloc_order;

	/* Setup register to interrupt and write completion status on error */
	writew(IOAT_CHANCTRL_RUN, ioat_chan->reg_base + IOAT_CHANCTRL_OFFSET);

	/* allocate a completion writeback area */
	/* doing 2 32bit writes to mmio since 1 64b write doesn't work */
	ioat_chan->completion =
		pci_pool_alloc(ioat_chan->ioat_dma->completion_pool,
			       GFP_KERNEL, &ioat_chan->completion_dma);
	if (!ioat_chan->completion)
		return -ENOMEM;

	memset(ioat_chan->completion, 0, sizeof(*ioat_chan->completion));
	writel(((u64)ioat_chan->completion_dma) & 0x00000000FFFFFFFF,
	       ioat_chan->reg_base + IOAT_CHANCMP_OFFSET_LOW);
	writel(((u64)ioat_chan->completion_dma) >> 32,
	       ioat_chan->reg_base + IOAT_CHANCMP_OFFSET_HIGH);

	order = ioat_get_alloc_order();
	ring = ioat_alloc_ring(c, order, GFP_KERNEL);
	if (!ring)
		return -ENOMEM;

	spin_lock_bh(&ioat_chan->cleanup_lock);
	spin_lock_bh(&ioat_chan->prep_lock);
	ioat_chan->ring = ring;
	ioat_chan->head = 0;
	ioat_chan->issued = 0;
	ioat_chan->tail = 0;
	ioat_chan->alloc_order = order;
	set_bit(IOAT_RUN, &ioat_chan->state);
	spin_unlock_bh(&ioat_chan->prep_lock);
	spin_unlock_bh(&ioat_chan->cleanup_lock);

	ioat_start_null_desc(ioat_chan);

	/* check that we got off the ground */
	do {
		udelay(1);
		status = ioat_chansts(ioat_chan);
	} while (i++ < 20 && !is_ioat_active(status) && !is_ioat_idle(status));

	if (is_ioat_active(status) || is_ioat_idle(status))
		return 1 << ioat_chan->alloc_order;

	chanerr = readl(ioat_chan->reg_base + IOAT_CHANERR_OFFSET);

	dev_WARN(to_dev(ioat_chan),
		 "failed to start channel chanerr: %#x\n", chanerr);
	ioat_free_chan_resources(c);
	return -EFAULT;
}

/* common channel initialization */
static void
ioat_init_channel(struct ioatdma_device *ioat_dma,
		  struct ioatdma_chan *ioat_chan, int idx)
{
	struct dma_device *dma = &ioat_dma->dma_dev;
	struct dma_chan *c = &ioat_chan->dma_chan;
	unsigned long data = (unsigned long) c;

	ioat_chan->ioat_dma = ioat_dma;
	ioat_chan->reg_base = ioat_dma->reg_base + (0x80 * (idx + 1));
	spin_lock_init(&ioat_chan->cleanup_lock);
	ioat_chan->dma_chan.device = dma;
	dma_cookie_init(&ioat_chan->dma_chan);
	list_add_tail(&ioat_chan->dma_chan.device_node, &dma->channels);
	ioat_dma->idx[idx] = ioat_chan;
	init_timer(&ioat_chan->timer);
	ioat_chan->timer.function = ioat_timer_event;
	ioat_chan->timer.data = data;
	tasklet_init(&ioat_chan->cleanup_task, ioat_cleanup_event, data);
}

#define IOAT_NUM_SRC_TEST 6 /* must be <= 8 */
static int ioat_xor_val_self_test(struct ioatdma_device *ioat_dma)
{
	int i, src_idx;
	struct page *dest;
	struct page *xor_srcs[IOAT_NUM_SRC_TEST];
	struct page *xor_val_srcs[IOAT_NUM_SRC_TEST + 1];
	dma_addr_t dma_srcs[IOAT_NUM_SRC_TEST + 1];
	dma_addr_t dest_dma;
	struct dma_async_tx_descriptor *tx;
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;
	u8 cmp_byte = 0;
	u32 cmp_word;
	u32 xor_val_result;
	int err = 0;
	struct completion cmp;
	unsigned long tmo;
	struct device *dev = &ioat_dma->pdev->dev;
	struct dma_device *dma = &ioat_dma->dma_dev;
	u8 op = 0;

	dev_dbg(dev, "%s\n", __func__);

	if (!dma_has_cap(DMA_XOR, dma->cap_mask))
		return 0;

	for (src_idx = 0; src_idx < IOAT_NUM_SRC_TEST; src_idx++) {
		xor_srcs[src_idx] = alloc_page(GFP_KERNEL);
		if (!xor_srcs[src_idx]) {
			while (src_idx--)
				__free_page(xor_srcs[src_idx]);
			return -ENOMEM;
		}
	}

	dest = alloc_page(GFP_KERNEL);
	if (!dest) {
		while (src_idx--)
			__free_page(xor_srcs[src_idx]);
		return -ENOMEM;
	}

	/* Fill in src buffers */
	for (src_idx = 0; src_idx < IOAT_NUM_SRC_TEST; src_idx++) {
		u8 *ptr = page_address(xor_srcs[src_idx]);

		for (i = 0; i < PAGE_SIZE; i++)
			ptr[i] = (1 << src_idx);
	}

	for (src_idx = 0; src_idx < IOAT_NUM_SRC_TEST; src_idx++)
		cmp_byte ^= (u8) (1 << src_idx);

	cmp_word = (cmp_byte << 24) | (cmp_byte << 16) |
			(cmp_byte << 8) | cmp_byte;

	memset(page_address(dest), 0, PAGE_SIZE);

	dma_chan = container_of(dma->channels.next, struct dma_chan,
				device_node);
	if (dma->device_alloc_chan_resources(dma_chan) < 1) {
		err = -ENODEV;
		goto out;
	}

	/* test xor */
	op = IOAT_OP_XOR;

	dest_dma = dma_map_page(dev, dest, 0, PAGE_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, dest_dma))
		goto dma_unmap;

	for (i = 0; i < IOAT_NUM_SRC_TEST; i++)
		dma_srcs[i] = DMA_ERROR_CODE;
	for (i = 0; i < IOAT_NUM_SRC_TEST; i++) {
		dma_srcs[i] = dma_map_page(dev, xor_srcs[i], 0, PAGE_SIZE,
					   DMA_TO_DEVICE);
		if (dma_mapping_error(dev, dma_srcs[i]))
			goto dma_unmap;
	}
	tx = dma->device_prep_dma_xor(dma_chan, dest_dma, dma_srcs,
				      IOAT_NUM_SRC_TEST, PAG