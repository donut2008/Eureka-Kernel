n event if the solicited bit is set. */
	qib_cq_enter(to_icq(qp->ibqp.recv_cq), &wc,
		     (ohdr->bth[0] &
			cpu_to_be32(IB_BTH_SOLICITED)) != 0);
	return;

drop:
	ibp->n_pkt_drops++;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * Copyright (c) 2006, 2007, 2008, 2009 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/mm.h>
#include <linux/device.h>

#include "qib.h"

static void __qib_release_user_pages(struct page **p, size_t num_pages,
				     int dirty)
{
	size_t i;

	for (i = 0; i < num_pages; i++) {
		if (dirty)
			set_page_dirty_lock(p[i]);
		put_page(p[i]);
	}
}

/*
 * Call with current->mm->mmap_sem held.
 */
static int __qib_get_user_pages(unsigned long start_page, size_t num_pages,
				struct page **p)
{
	unsigned long lock_limit;
	size_t got;
	int ret;

	lock_limit = rlimit(RLIMIT_MEMLOCK) >> PAGE_SHIFT;

	if (num_pages > lock_limit && !capable(CAP_IPC_LOCK)) {
		ret = -ENOMEM;
		goto bail;
	}

	for (got = 0; got < num_pages; got += ret) {
		ret = get_user_pages(current, current->mm,
				     start_page + got * PAGE_SIZE,
				     num_pages - got,
				     FOLL_WRITE | FOLL_FORCE,
				     p + got, NULL);
		if (ret < 0)
			goto bail_release;
	}

	current->mm->pinned_vm += num_pages;

	ret = 0;
	goto bail;

bail_release:
	__qib_release_user_pages(p, got, 0);
bail:
	return ret;
}

/**
 * qib_map_page - a safety wrapper around pci_map_page()
 *
 * A dma_addr of all 0's is interpreted by the chip as "disabled".
 * Unfortunately, it can also be a valid dma_addr returned on some
 * architectures.
 *
 * The powerpc iommu assigns dma_addrs in ascending order, so we don't
 * have to bother with retries or mapping a dummy page to insure we
 * don't just get the same mapping again.
 *
 * I'm sure we won't be so lucky with other iommu's, so FIXME.
 */
int qib_map_page(struct pci_dev *hwdev, struct page *page, dma_addr_t *daddr)
{
	dma_addr_t phys;

	phys = pci_map_page(hwdev, page, 0, PAGE_SIZE, PCI_DMA_FROMDEVICE);
	if (pci_dma_mapping_error(hwdev, phys))
		return -ENOMEM;

	if (!phys) {
		pci_unmap_page(hwdev, phys, PAGE_SIZE, PCI_DMA_FROMDEVICE);
		phys = pci_map_page(hwdev, page, 0, PAGE_SIZE,
				    PCI_DMA_FROMDEVICE);
		if (pci_dma_mapping_error(hwdev, phys))
			return -ENOMEM;
		/*
		 * FIXME: If we get 0 again, we should keep this page,
		 * map another, then free the 0 page.
		 */
	}
	*daddr = phys;
	return 0;
}

/**
 * qib_get_user_pages - lock user pages into memory
 * @start_page: the start page
 * @num_pages: the number of pages
 * @p: the output page structures
 *
 * This function takes a given start page (page aligned user virtual
 * address) and pins it and the following specified number of pages.  For
 * now, num_pages is always 1, but that will probably change at some point
 * (because caller is doing expected sends on a single virtually contiguous
 * buffer, so we can do all pages at once).
 */
int qib_get_user_pages(unsigned long start_page, size_t num_pages,
		       struct page **p)
{
	int ret;

	down_write(&current->mm->mmap_sem);

	ret = __qib_get_user_pages(start_page, num_pages, p);

	up_write(&current->mm->mmap_sem);

	return ret;
}

void qib_release_user_pages(struct page **p, size_t num_pages)
{
	if (current->mm) /* during close after signal, mm can be NULL */
		down_write(&current->mm->mmap_sem);

	__qib_release_user_pages(p, num_pages, 1);

	if (current->mm) {
		current->mm->pinned_vm -= num_pages;
		up_write(&current->mm->mmap_sem);
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * Copyright (c) 2007, 2008, 2009 QLogic Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/uio.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/overflow.h>

#include "qib.h"
#include "qib_user_sdma.h"

/* minimum size of header */
#define QIB_USER_SDMA_MIN_HEADER_LENGTH 64
/* expected size of headers (for dma_pool) */
#define QIB_USER_SDMA_EXP_HEADER_LENGTH 64
/* attempt to drain the queue for 5secs */
#define QIB_USER_SDMA_DRAIN_TIMEOUT 250

/*
 * track how many times a process open this driver.
 */
static struct rb_root qib_user_sdma_rb_root = RB_ROOT;

struct qib_user_sdma_rb_node {
	struct rb_node node;
	int refcount;
	pid_t pid;
};

struct qib_user_sdma_pkt {
	struct list_head list;  /* list element */

	u8  tiddma;		/* if this is NEW tid-sdma */
	u8  largepkt;		/* this is large pkt from kmalloc */
	u16 frag_size;		/* frag size used by PSM */
	u16 index;              /* last header index or push index */
	u16 naddr;              /* dimension of addr (1..3) ... */
	u16 addrlimit;		/* addr array size */
	u16 tidsmidx;		/* current tidsm index */
	u16 tidsmcount;		/* tidsm array item count */
	u16 payload_size;	/* payload size so far for header */
	u32 bytes_togo;		/* bytes for processing */
	u32 counter;            /* sdma pkts queued counter for this entry */
	struct qib_tid_session_member *tidsm;	/* tid session member array */
	struct qib_user_sdma_queue *pq;	/* which pq this pkt belongs to */
	u64 added;              /* global descq number of entries */

	struct {
		u16 offset;                     /* offset for kvaddr, addr */
		u16 length;                     /* length in page */
		u16 first_desc;			/* first desc */
		u16 last_desc;			/* last desc */
		u16 put_page;                   /* should we put_page? */
		u16 dma_mapped;                 /* is page dma_mapped? */
		u16 dma_length;			/* for dma_unmap_page() */
		u16 padding;
		struct page *page;              /* may be NULL (coherent mem) */
		void *kvaddr;                   /* FIXME: only for pio hack */
		dma_addr_t addr;
	} addr[4];   /* max pages, any more and we coalesce */
};

struct qib_user_sdma_queue {
	/*
	 * pkts sent to dma engine are queued on this
	 * list head.  the type of the elements of this
	 * list are struct qib_user_sdma_pkt...
	 */
	struct list_head sent;

	/*
	 * Because above list will be accessed by both process and
	 * signal handler, we need a spinlock for it.
	 */
	spinlock_t sent_lock ____cacheline_aligned_in_smp;

	/* headers with expected length are allocated from here... */
	char header_cache_name[64];
	struct dma_pool *header_cache;

	/* packets are allocated from the slab cache... */
	char pkt_slab_name[64];
	struct kmem_cache *pkt_slab;

	/* as packets go on the queued queue, they are counted... */
	u32 counter;
	u32 sent_counter;
	/* pending packets, not sending yet */
	u32 num_pending;
	/* sending packets, not complete yet */
	u32 num_sending;
	/* global descq number of entry of last sending packet */
	u64 added;

	/* dma page table */
	struct rb_root dma_pages_root;

	struct qib_user_sdma_rb_node *sdma_rb_node;

	/* protect everything above... */
	struct mutex lock;
};

static struct qib_user_sdma_rb_node *
qib_user_sdma_rb_search(struct rb_root *root, pid_t pid)
{
	struct qib_user_sdma_rb_node *sdma_rb_node;
	struct rb_node *node = root->rb_node;

	while (node) {
		sdma_rb_node = container_of(node,
			struct qib_user_sdma_rb_node, node);
		if (pid < sdma_rb_node->pid)
			node = node->rb_left;
		else if (pid > sdma_rb_node->pid)
			node = node->rb_right;
		else
			return sdma_rb_node;
	}
	return NULL;
}

static int
qib_user_sdma_rb_insert(struct rb_root *root, struct qib_user_sdma_rb_node *new)
{
	struct rb_node **node = &(root->rb_node);
	struct rb_node *parent = NULL;
	struct qib_user_sdma_rb_node *got;

	while (*node) {
		got = container_of(*node, struct qib_user_sdma_rb_node, node);
		parent = *node;
		if (new->pid < got->pid)
			node = &((*node)->rb_left);
		else if (new->pid > got->pid)
			node = &((*node)->rb_right);
		else
			return 0;
	}

	rb_link_node(&new->node, parent, node);
	rb_insert_color(&new->node, root);
	return 1;
}

struct qib_user_sdma_queue *
qib_user_sdma_queue_create(struct device *dev, int unit, int ctxt, int sctxt)
{
	struct qib_user_sdma_queue *pq =
		kmalloc(sizeof(struct qib_user_sdma_queue), GFP_KERNEL);
	struct qib_user_sdma_rb_node *sdma_rb_node;

	if (!pq)
		goto done;

	pq->counter = 0;
	pq->sent_counter = 0;
	pq->num_pending = 0;
	pq->num_sending = 0;
	pq->added = 0;
	pq->sdma_rb_node = NULL;

	INIT_LIST_HEAD(&pq->sent);
	spin_lock_init(&pq->sent_lock);
	mutex_init(&pq->lock);

	snprintf(pq->pkt_slab_name, sizeof(pq->pkt_slab_name),
		 "qib-user-sdma-pkts-%u-%02u.%02u", unit, ctxt, sctxt);
	pq->pkt_slab = kmem_cache_create(pq->pkt_slab_name,
					 sizeof(struct qib_user_sdma_pkt),
					 0, 0, NULL);

	if (!pq->pkt_slab)
		goto err_kfree;

	snprintf(pq->header_cache_name, sizeof(pq->header_cache_name),
		 "qib-user-sdma-headers-%u-%02u.%02u", unit, ctxt, sctxt);
	pq->header_cache = dma_pool_create(pq->header_cache_name,
					   dev,
					   QIB_USER_SDMA_EXP_HEADER_LENGTH,
					   4, 0);
	if (!pq->header_cache)
		goto err_slab;

	pq->dma_pages_root = RB_ROOT;

	sdma_rb_node = qib_user_sdma_rb_search(&qib_user_sdma_rb_root,
					current->pid);
	if (sdma_rb_node) {
		sdma_rb_node->refcount++;
	} else {
		int ret;

		sdma_rb_node = kmalloc(sizeof(
			struct qib_user_sdma_rb_node), GFP_KERNEL);
		if (!sdma_rb_node)
			goto err_rb;

		sdma_rb_node->refcount = 1;
		sdma_rb_node->pid = current->pid;

		ret = qib_user_sdma_rb_insert(&qib_user_sdma_rb_root,
					sdma_rb_node);
		BUG_ON(ret == 0);
	}
	pq->sdma_rb_node = sdma_rb_node;

	goto done;

err_rb:
	dma_pool_destroy(pq->header_cache);
err_slab:
	kmem_cache_destroy(pq->pkt_slab);
err_kfree:
	kfree(pq);
	pq = NULL;

done:
	return pq;
}

static void qib_user_sdma_init_frag(struct qib_user_sdma_pkt *pkt,
				    int i, u16 offset, u16 len,
				    u16 first_desc, u16 last_desc,
				    u16 put_page, u16 dma_mapped,
				    struct page *page, void *kvaddr,
				    dma_addr_t dma_addr, u16 dma_length)
{
	pkt->addr[i].offset = offset;
	pkt->addr[i].length = len;
	pkt->addr[i].first_desc = first_desc;
	pkt->addr[i].last_desc = last_desc;
	pkt->addr[i].put_page = put_page;
	pkt->addr[i].dma_mapped = dma_mapped;
	pkt->addr[i].page = page;
	pkt->addr[i].kvaddr = kvaddr;
	pkt->addr[i].addr = dma_addr;
	pkt->addr[i].dma_length = dma_length;
}

static void *qib_user_sdma_alloc_header(struct qib_user_sdma_queue *pq,
				size_t len, dma_addr_t *dma_addr)
{
	void *hdr;

	if (len == QIB_USER_SDMA_EXP_HEADER_LENGTH)
		hdr = dma_pool_alloc(pq->header_cache, GFP_KERNEL,
					     dma_addr);
	else
		hdr = NULL;

	if (!hdr) {
		hdr = kmalloc(len, GFP_KERNEL);
		if (!hdr)
			return NULL;

		*dma_addr = 0;
	}

	return hdr;
}

static int qib_user_sdma_page_to_frags(const struct qib_devdata *dd,
				       struct qib_user_sdma_queue *pq,
				       struct qib_user_sdma_pkt *pkt,
				       struct page *page, u16 put,
				       u16 offset, u16 len, void *kvaddr)
{
	__le16 *pbc16;
	void *pbcvaddr;
	struct qib_message_header *hdr;
	u16 newlen, pbclen, lastdesc, dma_mapped;
	u32 vcto;
	union qib_seqnum seqnum;
	dma_addr_t pbcdaddr;
	dma_addr_t dma_addr =
		dma_map_page(&dd->pcidev->dev,
			page, offset, len, DMA_TO_DEVICE);
	int ret = 0;

	if (dma_mapping_error(&dd->pcidev->dev, dma_addr)) {
		/*
		 * dma mapping error, pkt has not managed
		 * this page yet, return the page here so
		 * the caller can ignore this page.
		 */
		if (put) {
			put_page(page);
		} else {
			/* coalesce case */
			kunmap(page);
			__free_page(page);
		}
		ret = -ENOMEM;
		goto done;
	}
	offset = 0;
	dma_mapped = 1;


next_fragment:

	/*
	 * In tid-sdma, the transfer length is restricted by
	 * receiver side current tid page length.
	 */
	if (pkt->tiddma && len > pkt->tidsm[pkt->tidsmidx].length)
		newlen = pkt->tidsm[pkt->tidsmidx].length;
	else
		newlen = len;

	/*
	 * Then the transfer length is restricted by MTU.
	 * the last descriptor flag is determined by:
	 * 1. the current packet is at frag size length.
	 * 2. the current tid page is done if tid-sdma.
	 * 3. there is no more byte togo if sdma.
	 */
	lastdesc = 0;
	if ((pkt->payload_size + newlen) >= pkt->frag_size) {
		newlen = pkt->frag_size - pkt->payload_size;
		lastdesc = 1;
	} else if (pkt->tiddma) {
		if (newlen == pkt->tidsm[pkt->tidsmidx].length)
			lastdesc = 1;
	} else {
		if (newlen == pkt->bytes_togo)
			lastdesc = 1;
	}

	/* fill the next fragment in this page */
	qib_user_sdma_init_frag(pkt, pkt->naddr, /* index */
		offset, newlen,		/* offset, len */
		0, lastdesc,		/* first last desc */
		put, dma_mapped,	/* put page, dma mapped */
		page, kvaddr,		/* struct page, virt addr */
		dma_addr, len);		/* dma addr, dma length */
	pkt->bytes_togo -= newlen;
	pkt->payload_size += newlen;
	pkt->naddr++;
	if (pkt->naddr == pkt->addrlimit) {
		ret = -EFAULT;
		goto done;
	}

	/* If there is no more byte togo. (lastdesc==1) */
	if (pkt->bytes_togo == 0) {
		/* The packet is done, header is not dma mapped yet.
		 * it should be from kmalloc */
		if (!pkt->addr[pkt->index].addr) {
			pkt->addr[pkt->index].addr =
				dma_map_single(&dd->pcidev->dev,
					pkt->addr[pkt->index].kvaddr,
					pkt->addr[pkt->index].dma_length,
					DMA_TO_DEVICE);
			if (dma_mapping_error(&dd->pcidev->dev,
					pkt->addr[pkt->index].addr)) {
				ret = -ENOMEM;
				goto done;
			}
			pkt->addr[pkt->index].dma_mapped = 1;
		}

		goto done;
	}

	/* If tid-sdma, advance tid info. */
	if (pkt->tiddma) {
		pkt->tidsm[pkt->tidsmidx].length -= newlen;
		if (pkt->tidsm[pkt->tidsmidx].length) {
			pkt->tidsm[pkt->tidsmidx].offset += newlen;
		} else {
			pkt->tidsmidx++;
			if (pkt->tidsmidx == pkt->tidsmcount) {
				ret = -EFAULT;
				goto done;
			}
		}
	}

	/*
	 * If this is NOT the last descriptor. (newlen==len)
	 * the current packet is not done yet, but the current
	 * send side page is done.
	 */
	if (lastdesc == 0)
		goto done;

	/*
	 * If running this driver under PSM with message size
	 * fitting into one transfer unit, it is not possible
	 * to pass this line. otherwise, it is a buggggg.
	 */

	/*
	 * Since the current packet is done, and there are more
	 * bytes togo, we need to create a new sdma header, copying
	 * from previous sdma header and modify both.
	 */
	pbclen = pkt->addr[pkt->index].length;
	pbcvaddr = qib_user_sdma_alloc_header(pq, pbclen, &pbcdaddr);
	if (!pbcvaddr) {
		ret = -ENOMEM;
		goto done;
	}
	/* Copy the previous sdma header to new sdma header */
	pbc16 = (__le16 *)pkt->addr[pkt->index].kvaddr;
	memcpy(pbcvaddr, pbc16, pbclen);

	/* Modify the previous sdma header */
	hdr = (struct qib_message_header *)&pbc16[4];

	/* New pbc length */
	pbc16[0] = cpu_to_le16(le16_to_cpu(pbc16[0])-(pkt->bytes_togo>>2));

	/* New packet length */
	hdr->lrh[2] = cpu_to_be16(le16_to_cpu(pbc16[0]));

	if (pkt->tiddma) {
		/* turn on the header suppression */
		hdr->iph.pkt_flags =
			cpu_to_le16(le16_to_cpu(hdr->iph.pkt_flags)|0x2);
		/* turn off ACK_REQ: 0x04 and EXPECTED_DONE: 0x20 */
		hdr->flags &= ~(0x04|0x20);
	} else {
		/* turn off extra bytes: 20-21 bits */
		hdr->bth[0] = cpu_to_be32(be32_to_cpu(hdr->bth[0])&0xFFCFFFFF);
		/* turn off ACK_REQ: 0x04 */
		hdr->flags &= ~(0x04);
	}

	/* New kdeth checksum */
	vcto = le32_to_cpu(hdr->iph.ver_ctxt_tid_offset);
	hdr->iph.chksum = cpu_to_le16(QIB_LRH_BTH +
		be16_to_cpu(hdr->lrh[2]) -
		((vcto>>16)&0xFFFF) - (vcto&0xFFFF) -
		le16_to_cpu(hdr->iph.pkt_flags));

	/* The packet is done, header is not dma mapped yet.
	 * it should be from kmalloc */
	if (!pkt->addr[pkt->index].addr) {
		pkt->addr[pkt->index].addr =
			dma_map_single(&dd->pcidev->dev,
				pkt->addr[pkt->index].kvaddr,
				pkt->addr[pkt->index].dma_length,
				DMA_TO_DEVICE);
		if (dma_mapping_error(&dd->pcidev->dev,
				pkt->addr[pkt->index].addr)) {
			ret = -ENOMEM;
			goto done;
		}
		pkt->addr[pkt->index].dma_mapped = 1;
	}

	/* Modify the new sdma header */
	pbc16 = (__le16 *)pbcvaddr;
	hdr = (struct qib_message_header *)&pbc16[4];

	/* New pbc length */
	pbc16[0] = cpu_to_le16(le16_to_cpu(pbc16[0])-(pkt->payload_size>>2));

	/* New packet length */
	hdr->lrh[2] = cpu_to_be16(le16_to_cpu(pbc16[0]));

	if (pkt->tiddma) {
		/* Set new tid and offset for new sdma header */
		hdr->iph.ver_ctxt_tid_offset = cpu_to_le32(
			(le32_to_cpu(hdr->iph.ver_ctxt_tid_offset)&0xFF000000) +
			(pkt->tidsm[pkt->tidsmidx].tid<<QLOGIC_IB_I_TID_SHIFT) +
			(pkt->tidsm[pkt->tidsmidx].offset>>2));
	} else {
		/* Middle protocol new packet offset */
		hdr->uwords[2] += pkt->payload_size;
	}

	/* New kdeth checksum */
	vcto = le32_to_cpu(hdr->iph.ver_ctxt_tid_offset);
	hdr->iph.chksum = cpu_to_le16(QIB_LRH_BTH +
		be16_to_cpu(hdr->lrh[2]) -
		((vcto>>16)&0xFFFF) - (vcto&0xFFFF) -
		le16_to_cpu(hdr->iph.pkt_flags));

	/* Next sequence number in new sdma header */
	seqnum.val = be32_to_cpu(hdr->bth[2]);
	if (pkt->tiddma)
		seqnum.seq++;
	else
		seqnum.pkt++;
	hdr->bth[2] = cpu_to_be32(seqnum.val);

	/* Init new sdma header. */
	qib_user_sdma_init_frag(pkt, pkt->naddr, /* index */
		0, pbclen,		/* offset, len */
		1, 0,			/* first last desc */
		0, 0,			/* put page, dma mapped */
		NULL, pbcvaddr,		/* struct page, virt addr */
		pbcdaddr, pbclen);	/* dma addr, dma length */
	pkt->index = pkt->naddr;
	pkt->payload_size = 0;
	pkt->naddr++;
	if (pkt->naddr == pkt->addrlimit) {
		ret = -EFAULT;
		goto done;
	}

	/* Prepare for next fragment in this page */
	if (newlen != len) {
		if (dma_mapped) {
			put = 0;
			dma_mapped = 0;
			page = NULL;
			kvaddr = NULL;
		}
		len -= newlen;
		offset += newlen;

		goto next_fragment;
	}

done:
	return ret;
}

/* we've too many pages in the iovec, coalesce to a single page */
static int qib_user_sdma_coalesce(const struct qib_devdata *dd,
				  struct qib_user_sdma_queue *pq,
				  struct qib_user_sdma_pkt *pkt,
				  const struct iovec *iov,
				  unsigned long niov)
{
	int ret = 0;
	struct page *page = alloc_page(GFP_KERNEL);
	void *mpage_save;
	char *mpage;
	int i;
	int len = 0;

	if (!page) {
		ret = -ENOMEM;
		goto done;
	}

	mpage = kmap(page);
	mpage_save = mpage;
	for (i = 0; i < niov; i++) {
		int cfur;

		cfur = copy_from_user(mpage,
				      iov[i].iov_base, iov[i].iov_len);
		if (cfur) {
			ret = -EFAULT;
			goto free_unmap;
		}

		mpage += iov[i].iov_len;
		len += iov[i].iov_len;
	}

	ret = qib_user_sdma_page_to_frags(dd, pq, pkt,
			page, 0, 0, len, mpage_save);
	goto done;

free_unmap:
	kunmap(page);
	__free_page(page);
done:
	return ret;
}

/*
 * How many pages in this iovec element?
 */
static size_t qib_user_sdma_num_pages(const struct iovec *iov)
{
	const unsigned long addr  = (unsigned long) iov->iov_base;
	const unsigned long  len  = iov->iov_len;
	const unsigned long spage = addr & PAGE_MASK;
	const unsigned long epage = (addr + len - 1) & PAGE_MASK;

	return 1 + ((epage - spage) >> PAGE_SHIFT);
}

static void qib_user_sdma_free_pkt_frag(struct device *dev,
					struct qib_user_sdma_queue *pq,
					struct qib_user_sdma_pkt *pkt,
					int frag)
{
	const int i = frag;

	if (pkt->addr[i].page) {
		/* only user data has page */
		if (pkt->addr[i].dma_mapped)
			dma_unmap_page(dev,
				       pkt->addr[i].addr,
				       pkt->addr[i].dma_length,
				       DMA_TO_DEVICE);

		if (pkt->addr[i].kvaddr)
			kunmap(pkt->addr[i].page);

		if (pkt->addr[i].put_page)
			put_page(pkt->addr[i].page);
		else
			__free_page(pkt->addr[i].page);
	} else if (pkt->addr[i].kvaddr) {
		/* for headers */
		if (pkt->addr[i].dma_mapped) {
			/* from kmalloc & dma mapped */
			dma_unmap_single(dev,
				       pkt->addr[i].addr,
				       pkt->addr[i].dma_length,
				       DMA_TO_DEVICE);
			kfree(pkt->addr[i].kvaddr);
		} else if (pkt->addr[i].addr) {
			/* free coherent mem from cache... */
			dma_pool_free(pq->header_cache,
			      pkt->addr[i].kvaddr, pkt->addr[i].addr);
		} else {
			/* from kmalloc but not dma mapped */
			kfree(pkt->addr[i].kvaddr);
		}
	}
}

/* return number of pages pinned... */
static int qib_user_sdma_pin_pages(const struct qib_devdata *dd,
				   struct qib_user_sdma_queue *pq,
				   struct qib_user_sdma_pkt *pkt,
				   unsigned long addr, int tlen, size_t npages)
{
	struct page *pages[8];
	int i, j;
	int ret = 0;

	while (npages) {
		if (npages > 8)
			j = 8;
		else
			j = npages;

		ret = get_user_pages_fast(addr, j, 0, pages);
		if (ret != j) {
			i = 0;
			j = ret;
			ret = -ENOMEM;
			goto free_pages;
		}

		for (i = 0; i < j; i++) {
			/* map the pages... */
			unsigned long fofs = addr & ~PAGE_MASK;
			int flen = ((fofs + tlen) > PAGE_SIZE) ?
				(PAGE_SIZE - fofs) : tlen;

			ret = qib_user_sdma_page_to_frags(dd, pq, pkt,
				pages[i], 1, fofs, flen, NULL);
			if (ret < 0) {
				/* current page has beed taken
				 * care of inside above call.
				 */
				i++;
				goto free_pages;
			}

			addr += flen;
			tlen -= flen;
		}

		npages -= j;
	}

	goto done;

	/* if error, return all pages not managed by pkt */
free_pages:
	while (i < j)
		put_page(pages[i++]);

done:
	return ret;
}

static int qib_user_sdma_pin_pkt(const struct qib_devdata *dd,
				 struct qib_user_sdma_queue *pq,
				 struct qib_user_sdma_pkt *pkt,
				 const struct iovec *iov,
				 unsigned long niov)
{
	int ret = 0;
	unsigned long idx;

	for (idx = 0; idx < niov; idx++) {
		const size_t npages = qib_user_sdma_num_pages(iov + idx);
		const unsigned long addr = (unsigned long) iov[idx].iov_base;

		ret = qib_user_sdma_pin_pages(dd, pq, pkt, addr,
					      iov[idx].iov_len, npages);
		if (ret < 0)
			goto free_pkt;
	}

	goto done;

free_pkt:
	/* we need to ignore the first entry here */
	for (idx = 1; idx < pkt->naddr; idx++)
		qib_user_sdma_free_pkt_frag(&dd->pcidev->dev, pq, pkt, idx);

	/* need to dma unmap the first entry, this is to restore to
	 * the original state so that caller can free the memory in
	 * error condition. Caller does not know if dma mapped or not*/
	if (pkt->addr[0].dma_mapped) {
		dma_unmap_single(&dd->pcidev->dev,
		       pkt->addr[0].addr,
		       pkt->addr[0].dma_length,
		       DMA_TO_DEVICE);
		pkt->addr[0].addr = 0;
		pkt->addr[0].dma_mapped = 0;
	}

done:
	return ret;
}

static int qib_user_sdma_init_payload(const struct qib_devdata *dd,
				      struct qib_user_sdma_queue *pq,
				      struct qib_user_sdma_pkt *pkt,
				      const struct iovec *iov,
				      unsigned long niov, int npages)
{
	int ret = 0;

	if (pkt->frag_size == pkt->bytes_togo &&
			npages >= ARRAY_SIZE(pkt->addr))
		ret = qib_user_sdma_coalesce(dd, pq, pkt, iov, niov);
	else
		ret = qib_user_sdma_pin_pkt(dd, pq, pkt, iov, niov);

	return ret;
}

/* free a packet list -- return counter value of last packet */
static void qib_user_sdma_free_pkt_list(struct device *dev,
					struct qib_user_sdma_queue *pq,
					struct list_head *list)
{
	struct qib_user_sdma_pkt *pkt, *pkt_next;

	list_for_each_entry_safe(pkt, pkt_next, list, list) {
		int i;

		for (i = 0; i < pkt->naddr; i++)
			qib_user_sdma_free_pkt_frag(dev, pq, pkt, i);

		if (pkt->largepkt)
			kfree(pkt);
		else
			kmem_cache_free(pq->pkt_slab, pkt);
	}
	INIT_LIST_HEAD(list);
}

/*
 * copy headers, coalesce etc -- pq->lock must be held
 *
 * we queue all the packets to list, returning the
 * number of bytes total.  list must be empty initially,
 * as, if there is an error we clean it...
 */
static int qib_user_sdma_queue_pkts(const struct qib_devdata *dd,
				    struct qib_pportdata *ppd,
				    struct qib_user_sdma_queue *pq,
				    const struct iovec *iov,
				    unsigned long niov,
				    struct list_head *list,
				    int *maxpkts, int *ndesc)
{
	unsigned long idx = 0;
	int ret = 0;
	int npkts = 0;
	__le32 *pbc;
	dma_addr_t dma_addr;
	struct qib_user_sdma_pkt *pkt = NULL;
	size_t len;
	size_t nw;
	u32 counter = pq->counter;
	u16 frag_size;

	while (idx < niov && npkts < *maxpkts) {
		const unsigned long addr = (unsigned long) iov[idx].iov_base;
		const unsigned long idx_save = idx;
		unsigned pktnw;
		unsigned pktnwc;
		int nfrags = 0;
		size_t npages = 0;
		size_t bytes_togo = 0;
		int tiddma = 0;
		int cfur;

		len = iov[idx].iov_len;
		nw = len >> 2;

		if (len < QIB_USER_SDMA_MIN_HEADER_LENGTH ||
		    len > PAGE_SIZE || len & 3 || addr & 3) {
			ret = -EINVAL;
			goto free_list;
		}

		pbc = qib_user_sdma_alloc_header(pq, len, &dma_addr);
		if (!pbc) {
			ret = -ENOMEM;
			goto free_list;
		}

		cfur = copy_from_user(pbc, iov[idx].iov_base, len);
		if (cfur) {
			ret = -EFAULT;
			goto free_pbc;
		}

		/*
		 * This assignment is a bit strange.  it's because the
		 * the pbc counts the number of 32 bit words in the full
		 * packet _except_ the first word of the pbc itself...
		 */
		pktnwc = nw - 1;

		/*
		 * pktnw computation yields the number of 32 bit words
		 * that the caller has indicated in the PBC.  note that
		 * this is one less than the total number of words that
		 * goes to the send DMA engine as the first 32 bit word
		 * of the PBC itself is not counted.  Armed with this count,
		 * we can verify that the packet is consistent with the
		 * iovec lengths.
		 */
		pktnw = le32_to_cpu(*pbc) & 0xFFFF;
		if (pktnw < pktnwc) {
			ret = -EINVAL;
			goto free_pbc;
		}

		idx++;
		while (pktnwc < pktnw && idx < niov) {
			const size_t slen = iov[idx].iov_len;
			const unsigned long faddr =
				(unsigned long) iov[idx].iov_base;

			if (slen & 3 || faddr & 3 || !slen) {
				ret = -EINVAL;
				goto free_pbc;
			}

			npages += qib