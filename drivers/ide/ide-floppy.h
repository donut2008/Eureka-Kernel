/*
 * Copyright 2012 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/platform_data/mmp_dma.h>
#include <linux/dmapool.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/of.h>
#include <linux/dma/mmp-pdma.h>

#include "dmaengine.h"

#define DCSR		0x0000
#define DALGN		0x00a0
#define DINT		0x00f0
#define DDADR		0x0200
#define DSADR(n)	(0x0204 + ((n) << 4))
#define DTADR(n)	(0x0208 + ((n) << 4))
#define DCMD		0x020c

#define DCSR_RUN	BIT(31)	/* Run Bit (read / write) */
#define DCSR_NODESC	BIT(30)	/* No-Descriptor Fetch (read / write) */
#define DCSR_STOPIRQEN	BIT(29)	/* Stop Interrupt Enable (read / write) */
#defi