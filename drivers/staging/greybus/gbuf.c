/*
 * Greybus gbuf handling
 *
 * Copyright 2014 Google Inc.
 *
 * Released under the GPLv2 only.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>

#include "greybus.h"

static struct kmem_cache *gbuf_head_cache;

/**
 * greybus_alloc_gbuf - allocate a greybus buffer
 *
 * @gmod: greybus device that wants to allocate this
 * @cport: cport to send the data to
 * @complete: callback when the gbuf is finished with
 * @size: size of the buffer
 * @gfp_mask: allocation mask
 *
 * TODO: someday it will be nice to handle DMA, but for now, due to the
 * architecture we are stuck with, the greybus core has to allocate the buffer
 * that the driver can then fill up with the data to be sent out.  Curse
 * hardware designers for this issue...
 */
struct gbuf *greybus_alloc_gbuf(struct greybus_host_device *hd,
				u16 dest_cport_id,
				unsigned int size,
				gfp_t gfp_mask)
{
	return kmem_cache_zalloc(gbuf_head_cache, gfp_mask);
}
EXPORT_SYMBOL_GPL(greybus_alloc_gbuf);

void greybus_free_gbuf(struct gbuf *gbuf)
{
	kmem_cache_free(gbuf_head_cache, gbuf);
}
EXPORT_SYMBOL_GPL(greybus_free_gbuf);

int greybus_submit_gbuf(struct gbuf *gbuf, gfp_t gfp_mask)
{
	gbuf->status = -EINPROGRESS;

	return gbuf->hd->driver->submit_gbuf(gbuf, gfp_mask);
}

void greybus_kill_gbuf(struct gbuf *gbuf)
{
	if (gbuf->status != -EINPROGRESS)
		return;

	gbuf->hd->driver->kill_gbuf(gbuf);
}

void greybus_cport_in(struct greybus_host_device *hd, u16 cport_id,
			u8 *data, size_t length)
{
	struct gb_connection *connection;

	connection = gb_hd_connection_find(hd, cport_id);
	if (!connection) {
		dev_err(hd->parent,
			"nonexistent connection (%zu bytes dropped)\n", length);
		return;
	}
	gb_connection_operation_recv(connection, data, length);
}
EXPORT_SYMBOL_GPL(greybus_cport_in);

int gb_gbuf_init(void)
{
	gbuf_head_cache = kmem_cache_create("gbuf_head_cache",
					    sizeof(struct gbuf), 0, 0, NULL);
	return 0;
}

void gb_gbuf_exit(void)
{
	kmem_cache_destroy(gbuf_head_cache);
	gbuf_head_cache = NULL;
}
