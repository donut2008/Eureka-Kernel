AT_PP_ROT_MASK	0x00300000
#define TSTAT_PP_ROT_OFFSET	20
#define TSTAT_PF_MASK		0x00C00000
#define TSTAT_PF_OFFSET		22
#define TSTAT_ADCSYS1_MASK	0x03000000
#define TSTAT_ADCSYS1_OFFSET	24
#define TSTAT_ADCSYS2_MASK	0x0C000000
#define TSTAT_ADCSYS2_OFFSET	26

#define TASK_STAT_IDLE		0
#define TASK_STAT_ACTIVE	1
#define TASK_STAT_WAIT4READY	2

struct idmac {
	struct dma_device	dma;
};

struct ipu {
	void __iomem		*reg_ipu;
	void __iomem		*reg_ic;
	unsigned int		irq_fn;		/* IPU Function IRQ to the CPU */
	unsigned int		irq_err;	/* IPU Error IRQ to the CPU */
	unsigned int		irq_base;	/* Beginning of the IPU IRQ range */
	unsigned long		channel_init_mask;
	spinlock_t		lock;
	struct clk		*ipu_clk;
	struct device		*dev;
	struct idmac		idmac;
	struct idmac_channel	channel[IPU_CHANNELS_NUM];
	struct tasklet_struct	tasklet;
};

#define to_idmac(d) container_of(d, struct idmac, dma)

extern int ipu_irq_attach_irq(struct ipu *ipu, struct platform_device *dev);
extern void ipu_irq_detach_irq(struct ipu *ipu, struct platform_device *dev);

extern bool ipu_irq_status(uint32_t irq);
extern int ipu_irq_map(unsigned int source);
extern int ipu_irq_unmap(unsigned int source);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * Copyright (C) 2008
 * Guennadi Liakhovetski, DENX Software Engineering, <lg@denx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/dma/ipu-dma.h>

#include "ipu_intern.h"

/*
 * Register read / write - shall be inlined by the compiler
 */
static u32 ipu_read_reg(struct ipu *ipu, unsigned long reg)
{
	return __raw_readl(ipu->reg_ipu + reg);
}

static void ipu_write_reg(struct ipu *ipu, u32 value, unsigned long reg)
{
	__raw_writel(value, ipu->reg_ipu + reg);
}


/*
 * IPU IRQ chip driver
 */

#define IPU_IRQ_NR_FN_BANKS 3
#define IPU_IRQ_NR_ERR_BANKS 2
#define IPU_IRQ_NR_BANKS (IPU_IRQ_NR_FN_BANKS + IPU_IRQ_NR_ERR_BANKS)

struct ipu_irq_bank {
	unsigned int	control;
	unsigned int	status;
	struct ipu	*ipu;
};

static struct ipu_irq_bank irq_bank[IPU_IRQ_NR_BANKS] = {
	/* 3 groups of functional interrupts */
	{
		.control	= IPU_INT_CTRL_1,
		.status		= IPU_INT_STAT_1,
	}, {
		.control	= IPU_INT_CTRL_2,
		.status		= IPU_INT_STAT_2,
	}, {
		.control	= IPU_INT_CTRL_3,
		.status		= IPU_INT_STAT_3,
	},
	/* 2 groups of error interrupts */
	{
		.control	= IPU_INT_CTRL_4,
		.status		= IPU_INT_STAT_4,
	}, {
		.control	= IPU_INT_CTRL_5,
		.status		= IPU_INT_STAT_5,
	},
};

struct ipu_irq_map {
