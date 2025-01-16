/*
 * Copyright (C) 2008
 * Guennadi Liakhovetski, DENX Software Engineering, <lg@denx.de>
 *
 * Copyright (C) 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/dma/ipu-dma.h>

#include "../dmaengine.h"
#include "ipu_intern.h"

#define FS_VF_IN_VALID	0x00000002
#define FS_ENC_IN_VALID	0x00000001

static int ipu_disable_channel(struct idmac *idmac, struct idmac_channel *ichan,
			       bool wait_for_stop);

/*
 * There can be only one, we could allocate it dynamically, but then we'd have
 * to add an extra parameter to some functions, and use something as ugly as
 *	struct ipu *ipu = to_ipu(to_idmac(ichan->dma_chan.device));
 * in the ISR
 */
static struct ipu ipu_data;

#define to_ipu(id) container_of(id, struct ipu, idmac)

static u32 __idmac_read_icreg(struct ipu *ipu, unsigned long reg)
{
	return __raw_readl(ipu->reg_ic + reg);
}

#define idmac_read_icreg(ipu, reg) __idmac_read_icreg(ipu, reg - IC_CONF)

static void __idmac_write_icreg(struct ipu *ipu, u32 value, unsigned long reg)
{
	__raw_writel(value, ipu->reg_ic + reg);
}

#define idmac_write_icreg(ipu, v, reg) __idmac_write_icreg(ipu, v, reg - IC_CONF)

static u32 idmac_read_ipureg(struct ipu *ipu, unsigned long reg)
{
	return __raw_readl(ipu->reg_ipu + reg);
}

static void idmac_write_ipureg(struct ipu *ipu, u32 value, unsigned long reg)
{
	__raw_writel(value, ipu->reg_ipu + reg);
}

/*****************************************************************************
 * IPU / IC common functions
 */
static void dump_idmac_reg(struct ipu *ipu)
{
	dev_dbg(ipu->dev, "IDMAC_CONF 0x%x, IC_CONF 0x%x, IDMAC_CHA_EN 0x%x, "
		"IDMAC_CHA_PRI 0x%x, IDMAC_CHA_BUSY 0x%x\n",
		idmac_read_icreg(ipu, IDMAC_CONF),
		idmac_read_icreg(ipu, IC_CONF),
		idmac_read_icreg(ipu, IDMAC_CHA_EN),
		idmac_read_icreg(ipu, IDMAC_CHA_PRI),
		idmac_read_icreg(ipu, IDMAC_CHA_BUSY));
	dev_dbg(ipu->dev, "BUF0_RDY 0x%x, BUF1_RDY 0x%x, CUR_BUF 0x%x, "
		"DB_MODE 0x%x, TASKS_STAT 0x%x\n",
		idmac_read_ipureg(ipu, IPU_CHA_BUF0_RDY),
		idmac_read_ipureg(ipu, IPU_CHA_BUF1_RDY),
		idmac_read_ipureg(ipu, IPU_CHA_CUR_BUF),
		idmac_read_ipureg(ipu, IPU_CHA_DB_MODE_SEL),
		idmac_read_ipureg(ipu, IPU_TASKS_STAT));
}

static uint32_t bytes_per_pixel(enum pixel_fmt fmt)
{
	switch (fmt) {
	case IPU_PIX_FMT_GENERIC:	/* generic data */
	case IPU_PIX_FMT_RGB332:
	case IPU_PIX_FMT_YUV420P:
	case IPU_PIX_FMT_YUV422P:
	default:
		return 1;
	case IPU_PIX_FMT_RGB565:
	case IPU_PIX_FMT_YUYV:
	case IPU_PIX_FMT_UYVY:
		return 2;
	case IPU_PIX_FMT_BGR24:
	case IPU_PIX_FMT_RGB24:
		return 3;
	case IPU_PIX_FMT_GENERIC_32:	/* generic data */
	case IPU_PIX_FMT_BGR32:
	case IPU_PIX_FMT_RGB32:
	case IPU_PIX_FMT_ABGR32:
		return 4;
	}
}

/* Enable direct write to memory by the Camera Sensor Interface */
static void ipu_ic_enable_task(struct ipu *ipu, enum ipu_channel channel)
{
	uint32_t ic_conf, mask;

	switch (channel) {
	case IDMAC_IC_0:
		mask = IC_CONF_PRPENC_EN;
		break;
	case IDMAC_IC_7:
		mask = IC_CONF_RWS_EN | IC_CONF_PRPENC_EN;
		break;
	default:
		return;
	}
	ic_conf = idmac_read_icreg(ipu, IC_CONF) | mask;
	idmac_write_icreg(ipu, ic_conf, IC_CONF);
}

/* Called under spin_lock_irqsave(&ipu_data.lock) */
static void ipu_ic_disable_task(struct ipu *ipu, enum ipu_channel channel)
{
	uint32_t ic_conf, mask;

	switch (channel) {
	case IDMAC_IC_0:
		mask = IC_CONF_PRPENC_EN;
		break;
	case IDMAC_IC_7:
		mask = IC_CONF_RWS_EN | IC_CONF_PRPENC_EN;
		break;
	default:
		return;
	}
	ic_conf = idmac_read_icreg(ipu, IC_CONF) & ~mask;
	idmac_write_icreg(ipu, ic_conf, IC_CONF);
}

static uint32_t ipu_channel_status(struct ipu *ipu, enum ipu_channel channel)
{
	uint32_t stat = TASK_STAT_IDLE;
	uint32_t task_stat_reg = idmac_read_ipureg(ipu, IPU_TASKS_STAT);

	switch (channel) {
	case IDMAC_IC_7:
		stat = (task_stat_reg & TSTAT_CSI2MEM_MASK) >>
			TSTAT_CSI2MEM_OFFSET;
		break;
	case IDMAC_IC_0:
	case IDMAC_SDC_0:
	case IDMAC_SDC_1:
	default:
		break;
	}
	return stat;
}

struct chan_param_mem_planar {
	/* Word 0 */
	u32	xv:10;
	u32	yv:10;
	u32	xb:12;

	u32	yb:12;
	u32	res1:2;
	u32	nsb:1;
	u32	lnpb:6;
	u32	ubo_l:11;

	u32	ubo_h:15;
	u32	vbo_l:17;

	u32	vbo_h:9;
	u32	res2:3;
	u32	fw:12;
	u32	fh_l:8;

	u32	fh_h:4;
	u32	res3:28;

	/* Word 1 */
	u32	eba0;

	u32	eba1;

	u32	bpp:3;
	u32	sl:14;
	u32	pfs:3;
	u32	bam:3;
	u32	res4:2;
	u32	npb:6;
	u32	res5:1;

	u32	sat:2;
	u32	res6:30;
} __attribute__ ((packed));

struct chan_param_mem_interleaved {
	/* Word 0 */
	u32	xv:10;
	u32	yv:10;
	u32	xb:12;

	u32	yb:12;
	u32	sce:1;
	u32	res1:1;
	u32	nsb:1;
	u32	lnpb:6;
	u32	sx:10;
	u32	sy_l:1;

	u32	sy_h:9;
	u32	ns:10;
	u32	sm:10;
	u32	sdx_l:3;

	u32	sdx_h:2;
	u32	sdy:5;
	u32	sdrx:1;
	u32	sdry:1;
	u32	sdr1:1;
	u32	res2:2;
	u32	fw:12;
	u32	fh_l:8;

	u32	fh_h:4;
	u32	res3:28;

	/* Word 1 */
	u32	eba0;

	u32	eba1;

	u32	bpp:3;
	u32	sl:14;
	u32	pfs:3;
	u32	bam:3;
	u32	res4:2;
	u32	npb:6;
	u32	res5:1;

	u32	sat:2;
	u32	scc:1;
	u32	ofs0:5;
	u32	ofs1:5;
	u32	ofs2:5;
	u32	ofs3:5;
	u32	wid0:3;
	u32	wid1:3;
	u32	wid2:3;

	u32	wid3:3;
	u32	dec_sel:1;
	u32	res6:28;
} __attribute__ ((packed));

union chan_param_mem {
	struct chan_param_mem_planar		pp;
	struct chan_param_mem_interleaved	ip;
};

static void ipu_ch_param_set_plane_offset(union chan_param_mem *params,
					  u32 u_offset, u32 v_offset)
{
	params->pp.ubo_l = u_offset & 0x7ff;
	params->pp.ubo_h = u_offset >> 11;
	params->pp.vbo_l = v_offset & 0x1ffff;
	params->pp.vbo_h = v_offset >> 17;
}

static void ipu_ch_param_set_size(union chan_param_mem *params,
				  uint32_t pixel_fmt, uint16_t width,
				  uint16_t height, uint16_t stride)
{
	u32 u_offset;
	u32 v_offset;

	params->pp.fw		= width - 1;
	params->pp.fh_l		= height - 1;
	params->pp.fh_h		= (height - 1) >> 8;
	params->pp.sl		= stride - 1;

	switch (pixel_fmt) {
	case IPU_PIX_FMT_GENERIC:
		/*Represents 8-bit Generic data */
		params->pp.bpp	= 3;
		params->pp.pfs	= 7;
		params->pp.npb	= 31;
		params->pp.sat	= 2;		/* SAT = use 32-bit access */
		break;
	case IPU_PIX_FMT_GENERIC_32:
		/*Represents 32-bit Generic data */
		params->pp.bpp	= 0;
		params->pp.pfs	= 7;
		params->pp.npb	= 7;
		params->pp.sat	= 2;		/* SAT = use 32-bit access */
		break;
	case IPU_PIX_FMT_RGB565:
		params->ip.bpp	= 2;
		params->ip.pfs	= 4;
		params->ip.npb	= 15;
		params->ip.sat	= 2;		/* SAT = 32-bit access */
		params->ip.ofs0	= 0;		/* Red bit offset */
		params->ip.ofs1	= 5;		/* Green bit offset */
		params->ip.ofs2	= 11;		/* Blue bit offset */
		params->ip.ofs3	= 16;		/* Alpha bit offset */
		params->ip.wid0	= 4;		/* Red bit width - 1 */
		params->ip.wid1	= 5;		/* Green bit width - 1 */
		params->ip.wid2	= 4;		/* Blue bit width - 1 */
		break;
	case IPU_PIX_FMT_BGR24:
		params->ip.bpp	= 1;		/* 24 BPP & RGB PFS */
		params->ip.pfs	= 4;
		params->ip.npb	= 7;
		params->ip.sat	= 2;		/* SAT = 32-bit access */
		params->ip.ofs0	= 0;		/* Red bit offset */
		params->ip.ofs1	= 8;		/* Green bit offset */
		params->ip.ofs2	= 16;		/* Blue bit offset */
		params->ip.ofs3	= 24;		/* Alpha bit offset */
		params->ip.wid0	= 7;		/* Red bit width - 1 */
		params->ip.wid1	= 7;		/* Green bit width - 1 */
		params->ip.wid2	= 7;		/* Blue bit width - 1 */
		break;
	case IPU_PIX_FMT_RGB24:
		params->ip.bpp	= 1;		/* 24 BPP & RGB PFS */
		params->ip.pfs	= 4;
		params->ip.npb	= 7;
		params->ip.sat	= 2;		/* SAT = 32-bit access */
		params->ip.ofs0	= 16;		/* Red bit offset */
		params->ip.ofs1	= 8;		/* Green bit offset */
		params->ip.ofs2	= 0;		/* Blue bit offset */
		params->ip.ofs3	= 24;		/* Alpha bit offset */
		params->ip.wid0	= 7;		/* Red bit width - 1 */
		params->ip.wid1	= 7;		/* Green bit width - 1 */
		params->ip.wid2	= 7;		/* Blue bit width - 1 */
		break;
	case IPU_PIX_FMT_BGRA32:
	case IPU_PIX_FMT_BGR32:
	case IPU_PIX_FMT_ABGR32:
		params->ip.bpp	= 0;
		params->ip.pfs	= 4;
		params->ip.npb	= 7;
		params->ip.sat	= 2;		/* SAT = 32-bit access */
		params->ip.ofs0	= 8;		/* Red bit offset */
		params->ip.ofs1	= 16;		/* Green bit offset */
		params->ip.ofs2	= 24;		/* Blue bit offset */
		params->ip.ofs3	= 0;		/* Alpha bit offset */
		params->ip.wid0	= 7;		/* Red bit width - 1 */
		params->ip.wid1	= 7;		/* Green bit width - 1 */
		params->ip.wid2	= 7;		/* Blue bit width - 1 */
		params->ip.wid3	= 7;		/* Alpha bit width - 1 */
		break;
	case IPU_PIX_FMT_RGBA32:
	case IPU_PIX_FMT_RGB32:
		params->ip.bpp	= 0;
		params->ip.pfs	= 4;
		params->ip.npb	= 7;
		params->ip.sat	= 2;		/* SAT = 32-bit access */
		params->ip.ofs0	= 24;		/* Red bit offset */
		params->ip.ofs1	= 16;		/* Green bit offset */
		params->ip.ofs2	= 8;		/* Blue bit offset */
		params->ip.ofs3	= 0;		/* Alpha bit offset */
		params->ip.wid0	= 7;		/* Red bit width - 1 */
		params->ip.wid1	= 7;		/* Green bit width - 1 */
		params->ip.wid2	= 7;		/* Blue bit width - 1 */
		params->ip.wid3	= 7;		/* Alpha bit width - 1 */
		break;
	case IPU_PIX_FMT_UYVY:
		params->ip.bpp	= 2;