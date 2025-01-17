orm_driver = {
	.probe		= mpc52xx_bcom_probe,
	.remove		= mpc52xx_bcom_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = mpc52xx_bcom_of_match,
	},
};


/* ======================================================================== */
/* Module                                                                   */
/* ======================================================================== */

static int __init
mpc52xx_bcom_init(void)
{
	return platform_driver_register(&mpc52xx_bcom_of_platform_driver);
}

static void __exit
mpc52xx_bcom_exit(void)
{
	platform_driver_unregister(&mpc52xx_bcom_of_platform_driver);
}

/* If we're not a module, we must make sure everything is setup before  */
/* anyone tries to use us ... that's why we use subsys_initcall instead */
/* of module_init. */
subsys_initcall(mpc52xx_bcom_init);
module_exit(mpc52xx_bcom_exit);

MODULE_DESCRIPTION("Freescale MPC52xx BestComm DMA");
MODULE_AUTHOR("Sylvain Munaut <tnt@246tNt.com>");
MODULE_AUTHOR("Andrey Volkov <avolkov@varma-el.com>");
MODULE_AUTHOR("Dale Farnsworth <dfarnsworth@mvista.com>");
MODULE_LICENSE("GPL v2");

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*
 * Bestcomm FEC tasks driver
 *
 *
 * Copyright (C) 2006-2007 Sylvain Munaut <tnt@246tNt.com>
 * Copyright (C) 2003-2004 MontaVista, Software, Inc.
 *                         ( by Dale Farnsworth <dfarnsworth@mvista.com> )
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <asm/io.h>

#include <linux/fsl/bestcomm/bestcomm.h>
#include <linux/fsl/bestcomm/bestcomm_priv.h>
#include <linux/fsl/bestcomm/fec.h>


/* ======================================================================== */
/* Task image/var/inc                                                       */
/* ======================================================================== */

/* fec tasks images */
extern u32 bcom_fec_rx_task[];
extern u32 bcom_fec_tx_task[];

/* rx task vars that need to be set before enabling the task */
struct bcom_fec_rx_var {
	u32 enable;		/* (u16*) address of task's control register */
	u32 fifo;		/* (u32*) address of fec's fifo */
	u32 bd_base;		/* (struct bcom_bd*) beginning of ring buffer */
	u32 bd_last;		/* (struct bcom_bd*) end of ring buffer */
	u32 bd_start;		/* (struct bcom_bd*) current bd */
	u32 buffer_size;	/* size of receive buffer */
};

/* rx task incs that need to be set before enabling the task */
struct bcom_fec_rx_inc {
	u16 pad0;
	s16 incr_bytes;
	u16 pad1;
	s16 incr_dst;
	u16 pad2;
	s16 incr_dst_ma;
};

/* tx task vars that need to be set before enabling the task */
struct bcom_fec_tx_var {
	u32 DRD;		/* (u32*) address of self-modified DRD */
	u32 fifo;		/* (u32*) address of fec's fifo */
	u32 enable;		/* (u16*) address of task's control register */
	u32 bd_base;		/* (struct bcom_bd*) beginning of ring buffer */
	u32 bd_last;		/* (struct bcom_bd*) end of ring buffer */
	u32 bd_start;		/* (struct bcom_bd*) current bd */
	u32 buffer_size;	/* set by uCode for each packet */
};

/* tx task incs that need to be set before enabling the task */
struct bcom_fec_tx_inc {
	u16 pad0;
	s16 incr_bytes;
	u16 pad1;
	s16 incr_src;
	u16 pad2;
	s16 incr_src_ma;
};

/* private structure in the task */
struct bcom_fec_priv {
	phys_addr_t	fifo;
	int		maxbufsize;
};


/* ======================================================================== */
/* Task support code                                                        */
/* ======================================================================== */

struct bcom_task *
bcom_fec_rx_init(int queue_len, phys_addr_t fifo, int maxbufsize)
{
	struct bcom_task *tsk;
	struct bcom_fec_priv *priv;

	tsk = bcom_task_alloc(queue_len, sizeof(struct bcom_fec_bd),
				sizeof(struct bcom_fec_priv));
	if (!tsk)
		return NULL;

	tsk->flags = BCOM_FLAGS_NONE;

	priv = tsk->priv;
	priv->fifo = fifo;
	priv->maxbufsize = maxbufsize;

	if (bcom_fec_rx_reset(tsk)) {
		bcom_task_free(tsk);
		return NULL;
	}

	return tsk;
}
EXPORT_SYMBOL_GPL(bcom_fec_rx_init);

int
bcom_fec_rx_reset(struct bcom_task *tsk)
{
	struct bcom_fec_priv *priv = tsk->priv;
	struct bcom_fec_rx_var *var;
	struct bcom_fec_rx_inc *inc;

	/* Shutdown the task */
	bcom_disable_task(tsk->tasknum);

	/* Reset the microcode */
	var = (struct bcom_fec_rx_var *) bcom_task_var(tsk->tasknum);
	inc = (struct bcom_fec_rx_inc *) bcom_task_inc(tsk->tasknum);

	if (bcom_load_image(tsk->tasknum, bcom_fec_rx_task))
		return -1;

	var->enable	= bcom_eng->regs_base +
				offsetof(struct mpc52xx_sdma, tcr[tsk->tasknum]);
	var->fifo	= (u32) priv->fifo;
	var->bd_base	= tsk->bd_pa;
	var->bd_last	= tsk->bd_pa + ((tsk->num_bd-1) * tsk->bd_size);
	var->bd_start	= tsk->bd_pa;
	var->buffer_size = priv->maxbufsize;

	inc->incr_bytes	= -(s16)sizeof(u32);	/* These should be in the   */
	inc->incr_dst	= sizeof(u32);		/* task image, but we stick */
	inc->incr_dst_ma= sizeof(u8);		/* to the official ones     */

	/* Reset the BDs */
	tsk->index = 0;
	tsk->outdex = 0;

	memset(tsk->bd, 0x00, tsk->num_bd * tsk->bd_size);

	/* Configure some stuff */
	bcom_set_task_pragma(tsk->tasknum, BCOM_FEC_RX_BD_PRAGMA);
	bcom_set_task_auto_start(tsk->tasknum, tsk->tasknum);

	out_8(&bcom_eng->regs->ipr[BCOM_INITIATOR_FEC_RX], BCOM_IPR_FEC_RX);

	out_be32(&bcom_eng->regs->IntPend, 1<<tsk->tasknum);	/* Clear ints */

	return 0;
}
EXPORT_SYMBOL_GPL(bcom_fec_rx_reset);

void
bcom_fec_rx_release(struct bcom_task *tsk)
{
	/* Nothing special for the FEC tasks */
	bcom_task_free(tsk);
}
EXPORT_SYMBOL_GPL(bcom_fec_rx_release);



	/* Return 2nd to last DRD */
	/* This is an ugly hack, but at least it's only done
	   once at initialization */
static u32 *self_modified_drd(int tasknum)
{
	u32 *desc;
	int num_descs;
	int drd_count;
	int i;

	num_descs = bcom_task_num_descs(tasknum);
	desc = bcom_task_desc(tasknum) + num_descs - 1;
	drd_count = 0;
	for (i=0; i<num_descs; i++, desc--)
		if (bcom_desc_is_drd(*desc) && ++drd_count == 3)
			break;
	return desc;
}

struct bcom_task *
bcom_fec_tx_init(int queue_len, phys_addr_t fifo)
{
	struct bcom_task *tsk;
	struct bcom_fec_priv *priv;

	tsk = bcom_task_alloc(queue_len, sizeof(struct bcom_fec_bd),
				sizeof(struct bcom_fec_priv));
	if (!tsk)
		return NULL;

	tsk->flags = BCOM_FLAGS_ENABLE_TASK;

	priv = tsk->priv;
	priv->fifo = fifo;

	if (bcom_fec_tx_reset(tsk)) {
		bcom_task_free(tsk);
		return NULL;
	}

	return tsk;
}
EXPORT_SYMBOL_GPL(bcom_fec_tx_init);

int
bcom_fec_tx_reset(struct bcom_task *tsk)
{
	struct bcom_fec_priv *priv = tsk->priv;
	struct bcom_fec_tx_var *var;
	struct bcom_fec_tx_inc *inc;

	/* Shutdown the task */
	bcom_disable_task(tsk->tasknum);

	/* Reset the microcode */
	var = (struct bcom_fec_tx_var *) bcom_task_var(tsk->tasknum);
	inc = (struct bcom_fec_tx_inc *) bcom_task_inc(tsk->tasknum);

	if (bcom_load_image(tsk->tasknum, bcom_fec_tx_task))
		return -1;

	var->enable	= bcom_eng->regs_base +
				offsetof(struct mpc52xx_sdma, tcr[tsk->tasknum]);
	var->fifo	= (u32) priv->fifo;
	var->DRD	= bcom_sram_va2pa(self_modified_drd(tsk->tasknum));
	var->bd_base	= tsk->bd_pa;
	var->bd_last	= tsk->bd_pa + ((tsk->num_bd-1) * tsk->bd_size);
	var->bd_start	= tsk->bd_pa;

	inc->incr_bytes	= -(s16)sizeof(u32);	/* These should be in the   */
	inc->incr_src	= sizeof(u32);		/* task image, but we stick */
	inc->incr_src_ma= sizeof(u8);		/* to the official ones     */

	/* Reset the BDs */
	tsk->index = 0;
	tsk->outdex = 0;

	memset(tsk->bd, 0x00, tsk->num_bd * tsk->bd_size);

	/* Configure some stuff */
	bcom_set_task_pragma(tsk->tasknum, BCOM_FEC_TX_BD_PRAGMA);
	bcom_set_task_auto_start(tsk->tasknum, tsk->tasknum);

	out_8(&bcom_eng->regs->ipr[BCOM_INITIATOR_FEC_TX], BCOM_IPR_FEC_TX);

	out_be32(&bcom_eng->regs->IntPend, 1<<tsk->tasknum);	/* Clear ints */

	return 0;
}
EXPORT_SYMBOL_GPL(bcom_fec_tx_reset);

void
bcom_fec_tx_release(struct bcom_task *tsk)
{
	/* Nothing special for the FEC tasks */
	bcom_task_free(tsk);
}
EXPORT_SYMBOL_GPL(bcom_fec_tx_release);


MODULE_DESCRIPTION("BestComm FEC tasks driver");
MODULE_AUTHOR("Dale Farnsworth <dfarnsworth@mvista.com>");
MODULE_LICENSE("GPL v2");

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * Driver for MPC52xx processor BestComm General Buffer Descriptor
 *
 * Copyright (C) 2007 Sylvain Munaut <tnt@246tNt.com>
 * Copyright (C) 2006 AppSpec Computer Technologies Corp.
 *                    Jeff Gibbons <jeff.gibbons@appspec.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <asm/errno.h>
#include <asm/io.h>

#include <asm/mpc52xx.h>
#include <asm/mpc52xx_psc.h>

#include <linux/fsl/bestcomm/bestcomm.h>
#include <linux/fsl/bestcomm/bestcomm_priv.h>
#include <linux/fsl/bestcomm/gen_bd.h>


/* ======================================================================== */
/* Task image/var/inc                                                       */
/* ======================================================================== */

/* gen_bd tasks images */
extern u32 bcom_gen_bd_rx_task[];
extern u32 bcom_gen_bd_tx_task[];

/* rx task vars that need to be set before enabling the task */
struct bcom_gen_bd_rx_var {
	u32 enable;		/* (u16*) address of task's control register */
	u32 fifo;		/* (u32*) address of gen_bd's fifo */
	u32 bd_base;		/* (struct bcom_bd*) beginning of ring buffer */
	u32 bd_last;		/* (struct bcom_bd*) end of ring buffer */
	u32 bd_start;		/* (struct bcom_bd*) current bd */
	u32 buffer_size;	/* size of receive buffer */
};

/* rx task incs that need to be set before enabling the task */
struct bcom_gen_bd_rx_inc {
	u16 pad0;
	s16 incr_bytes;
	u16 pad1;
	s16 incr_dst;
};

/* tx task vars that need to be set before enabling the task */
struct bcom_gen_bd_tx_var {
	u32 fifo;		/* (u32*) address of gen_bd's fifo */
	u32 enable;		/* (u16*) address of task's control register */
	u32 bd_base;		/* (struct bcom_bd*) beginning of ring buffer */
	u32 bd_last;		/* (struct bcom_bd*) end of ring buffer */
	u32 bd_start;		/* (struct bcom_bd*) current bd */
	u32 buffer_size;	/* set by uCode for each packet */
};

/* tx task incs that need to be set before enabling the task */
struct bcom_gen_bd_tx_inc {
	u16 pad0;
	s16 incr_bytes;
	u16 pad1;
	s16 incr_src;
	u16 pad2;
	s16 incr_src_ma;
};

/* private structure */
struct bcom_gen_bd_priv {
	phys_addr_t	fifo;
	int		initiator;
	int		ipr;
	int		maxbufsize;
};


/* ======================================================================== */
/* Task support code                                                        */
/* ======================================================================== */

struct bcom_task *
bcom_gen_bd_rx_init(int queue_len, phys_addr_t fifo,
			int initiator, int ipr, int maxbufsize)
{
	struct bcom_task *tsk;
	struct bcom_gen_bd_priv *priv;

	tsk = bcom_task_alloc(queue_len, sizeof(struct bcom_gen_bd),
			sizeof(struct bcom_gen_bd_priv));
	if (!tsk)
		return NULL;

	tsk->flags = BCOM_FLAGS_NONE;

	priv = tsk->priv;
	priv->fifo	= fifo;
	priv->initiator	= initiator;
	priv->ipr	= ipr;
	priv->maxbufsize = maxbufsize;

	if (bcom_gen_bd_rx_reset(tsk)) {
		bcom_task_free(tsk);
		return NULL;
	}

	return tsk;
}
EXPORT_SYMBOL_GPL(bcom_gen_bd_rx_init);

int
bcom_gen_bd_rx_reset(struct bcom_task *tsk)
{
	struct bcom_gen_bd_priv *priv = tsk->priv;
	struct bcom_gen_bd_rx_var *var;
	struct bcom_gen_bd_rx_inc *inc;

	/* Shutdown the task */
	bcom_disable_task(tsk->tasknum);

	/* Reset the microcode */
	var = (struct bcom_gen_bd_rx_var *) bcom_task_var(tsk->tasknum);
	inc = (struct bcom_gen_bd_rx_inc *) bcom_task_inc(tsk->tasknum);

	if (bcom_load_image(tsk->tasknum, bcom_gen_bd_rx_task))
		return -1;

	var->enable	= bcom_eng->regs_base +
				offsetof(struct mpc52xx_sdma, tcr[tsk->tasknum]);
	var->fifo	= (u32) priv->fifo;
	var->bd_base	= tsk->bd_pa;
	var->bd_last	= tsk->bd_pa + ((tsk->num_bd-1) * tsk->bd_size);
	var->bd_start	= tsk->bd_pa;
	var->buffer_size = priv->maxbufsize;

	inc->incr_bytes	= -(s16)sizeof(u32);
	inc->incr_dst	= sizeof(u32);

	/* Reset the BDs */
	tsk->index = 0;
	tsk->outdex = 0;

	memset(tsk->bd, 0x00, tsk->num_bd * tsk->bd_size);

	/* Configure some stuff */
	bcom_set_task_pragma(tsk->tasknum, BCOM_GEN_RX_BD_PRAGMA);
	bcom_set_task_auto_start(tsk->tasknum, tsk->tasknum);

	out_8(&bcom_eng->regs->ipr[priv->initiator], priv->ipr);
	bcom_set_initiator(tsk->tasknum, priv->initiator);

	out_be32(&bcom_eng->regs->IntPend, 1<<tsk->tasknum);	/* Clear ints */

	return 0;
}
EXPORT_SYMBOL_GPL(bcom_gen_bd_rx_reset);

void
bcom_gen_bd_rx_release(struct bcom_task *tsk)
{
	/* Nothing special for the GenBD tasks */
	bcom_task_free(tsk);
}
EXPORT_SYMBOL_GPL(bcom_gen_bd_rx_release);


extern struct bcom_task *
bcom_gen_bd_tx_init(int queue_len, phys_addr_t fifo,
			int initiator, int ipr)
{
	struct bcom_task *tsk;
	struct bcom_gen_bd_priv *priv;

	tsk = bcom_task_alloc(queue_len, sizeof(struct bcom_gen_bd),
			sizeof(struct bcom_gen_bd_priv));
	if (!tsk)
		return NULL;

	tsk->flags = BCOM_FLAGS_NONE;

	priv = tsk->priv;
	priv->fifo	= fifo;
	priv->initiator	= initiator;
	priv->ipr	= ipr;

	if (bcom_gen_bd_tx_reset(tsk)) {
		bcom_task_free(tsk);
		return NULL;
	}

	return tsk;
}
EXPORT_SYMBOL_GPL(bcom_gen_bd_tx_init);

int
bcom_gen_bd_tx_reset(struct bcom_task *tsk)
{
	struct bcom_gen_bd_priv *priv = tsk->priv;
	struct bcom_gen_bd_tx_var *var;
	struct bcom_gen_bd_tx_inc *inc;

	/* Shutdown the task */
	bcom_disable_task(tsk->tasknum);

	/* Reset the microcode */
	var = (struct bcom_gen_bd_tx_var *) bcom_task_var(tsk->tasknum);
	inc = (struct bcom_gen_bd_tx_inc *) bcom_task_inc(tsk->tasknum);

	if (bcom_load_image(tsk->tasknum, bcom_gen_bd_tx_task))
		return -1;

	var->enable	= bcom_eng->regs_base +
				offsetof(struct mpc52xx_sdma, tcr[tsk->tasknum]);
	var->fifo	= (u32) priv->fifo;
	var->bd_base	= tsk->bd_pa;
	var->bd_last	= tsk->bd_pa + ((tsk->num_bd-1) * tsk->bd_size);
	var->bd_start	= tsk->bd_pa;

	inc->incr_bytes	= -(s16)sizeof(u32);
	inc->incr_src	= sizeof(u32);
	inc->incr_src_ma = sizeof(u8);

	/* Reset the BDs */
	tsk->index = 0;
	tsk->outdex = 0;

	memset(tsk->bd, 0x00, tsk->num_bd * tsk->bd_size);

	/* Configure some stuff */
	bcom_set_task_pragma(tsk->tasknum, BCOM_GEN_TX_BD_PRAGMA);
	bcom_set_task_auto_start(tsk->tasknum, tsk->tasknum);

	out_8(&bcom_eng->regs->ipr[priv->initiator], priv->ipr);
	bcom_set_initiator(tsk->tasknum, priv->initiator);

	out_be32(&bcom_eng->regs->IntPend, 1<<tsk->tasknum);	/* Clear ints */

	return 0;
}
EXPORT_SYMBOL_GPL(bcom_gen_bd_tx_reset);

void
bcom_gen_bd_tx_release(struct bcom_task *tsk)
{
	/* Nothing special for the GenBD tasks */
	bcom_task_free(tsk);
}
EXPORT_SYMBOL_GPL(bcom_gen_bd_tx_release);

/* ---------------------------------------------------------------------
 * PSC support code
 */

/**
 * bcom_psc_parameters - Bestcomm initialization value table for PSC devices
 *
 * This structure is only used internally.  It is a lookup table for PSC
 * specific parameters to bestcomm tasks.
 */
static struct bcom_psc_params {
	int rx_initiator;
	int rx_ipr;
	int tx_initiator;
	int tx_ipr;
} bcom_psc_params[] = {
	[0] = {
		.rx_initiator = BCOM_INITIATOR_PSC1_RX,
		.rx_ipr = BCOM_IPR_PSC1_RX,
		.tx_initiator = BCOM_INITIATOR_PSC1_TX,
		.tx_ipr = BCOM_IPR_PSC1_TX,
	},
	[1] = {
		.rx_initiator = BCOM_INITIATOR_PSC2_RX,
		.rx_ipr = BCOM_IPR_PSC2_RX,
		.tx_initiator = BCOM_INITIATOR_PSC2_TX,
		.tx_ipr = BCOM_IPR_PSC2_TX,
	},
	[2] = {
		.rx_initiator = BCOM_INITIATOR_PSC3_RX,
		.rx_ipr = BCOM_IPR_PSC3_RX,
		.tx_initiator = BCOM_INITIATOR_PSC3_TX,
		.tx_ipr = BCOM_IPR_PSC3_TX,
	},
	[3] = {
		.rx_initiator = BCOM_INITIATOR_PSC4_RX,
		.rx_ipr = BCOM_IPR_PSC4_RX,
		.tx_initiator = BCOM_INITIATOR_PSC4_TX,
		.tx_ipr = BCOM_IPR_PSC4_TX,
	},
	[4] = {
		.rx_initiator = BCOM_INITIATOR_PSC5_RX,
		.rx_ipr = BCOM_IPR_PSC5_RX,
		.tx_initiator = BCOM_INITIATOR_PSC5_TX,
		.tx_ipr = BCOM_IPR_PSC5_TX,
	},
	[5] = {
		.rx_initiator = BCOM_INITIATOR_PSC6_RX,
		.rx_ipr = BCOM_IPR_PSC6_RX,
		.tx_initiator = BCOM_INITIATOR_PSC6_TX,
		.tx_ipr = BCOM_IPR_PSC6_TX,
	},
};

/**
 * bcom_psc_gen_bd_rx_init - Allocate a receive bcom_task for a PSC port
 * @psc_num:	Number of the PSC to allocate a task for
 * @queue_len:	number of buffer descriptors to allocate for the task
 * @fifo:	physical address of FIFO register
 * @maxbufsize:	Maximum receive data size in bytes.
 *
 * Allocate a bestcomm task structure for receiving data from a PSC.
 */
struct bcom_task * bcom_psc_gen_bd_rx_init(unsigned psc_num, int queue_len,
					   phys_addr_t fifo, int maxbufsize)
{
	if (psc_num >= MPC52xx_PSC_MAXNUM)
		return NULL;

	return bcom_gen_bd_rx_init(queue_len, fifo,
				   bcom_psc_params[psc_num].rx_initiator,
				   bcom_psc_params[psc_num].rx_ipr,
				   maxbufsize);
}
EXPORT_SYMBOL_GPL(bcom_psc_gen_bd_rx_init);

/**
 * bcom_psc_gen_bd_tx_init - Allocate a transmit bcom_task for a PSC port
 * @psc_num:	Number of the PSC to allocate a task for
 * @queue_len:	number of buffer descriptors to allocate for the task
 * @fifo:	physical address of FIFO register
 *
 * Allocate a bestcomm task structure for transmitting data to a PSC.
 */
struct bcom_task *
bcom_psc_gen_bd_tx_init(unsigned psc_num, int queue_len, phys_addr_t fifo)
{
	struct psc;
	return bcom_gen_bd_tx_init(queue_len, fifo,
				   bcom_psc_params[psc_num].tx_initiator,
				   bcom_psc_params[psc_num].tx_ipr);
}
EXPORT_SYMBOL_GPL(bcom_psc_gen_bd_tx_init);


MODULE_DESCRIPTION("BestComm General Buffer Descriptor tasks driver");
MODULE_AUTHOR("Jeff Gibbons <jeff.gibbons@appspec.com>");
MODULE_LICENSE("GPL v2");

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * Simple memory allocator for on-board SRAM
 *
 *
 * Maintainer : Sylvain Munaut <tnt@246tNt.com>
 *
 * Copyright (C) 2005 Sylvain Munaut <tnt@246tNt.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/io.h>
#include <asm/mmu.h>

#include <linux/fsl/bestcomm/sram.h>


/* Struct keeping our 'state' */
struct bcom_sram *bcom_sram = NULL;
EXPORT_SYMBOL_GPL(bcom_sram);	/* needed for inline functions */


/* ======================================================================== */
/* Public API                                                               */
/* ======================================================================== */
/* DO NOT USE in interrupts, if needed in irq handler, we should use the
   _irqsave version of the spin_locks */

int bcom_sram_init(struct device_node *sram_node, char *owner)
{
	int rv;
	const u32 *regaddr_p;
	u64 regaddr64, size64;
	unsigned int psize;

	/* Create our state struct */
	if (bcom_sram) {
		printk(KERN_ERR "%s: bcom_sram_init: "
			"Already initialized !\n", owner);
		return -EBUSY;
	}

	bcom_sram = kmalloc(sizeof(struct bcom_sram), GFP_KERNEL);
	if (!bcom_sram) {
		printk(KERN_ERR "%s: bcom_sram_init: "
			"Couldn't allocate internal state !\n", owner);
		return -ENOMEM;
	}

	/* Get address and size of the sram */
	regaddr_p = of_get_address(sram_node, 0, &size64, NULL);
	if (!regaddr_p) {
		printk(KERN_ERR "%s: bcom_sram_init: "
			"Invalid device node !\n", owner);
		rv = -EINVAL;
		goto error_free;
	}

	regaddr64 = of_translate_address(sram_node, regaddr_p);

	bcom_sram->base_phys = (phys_addr_t) regaddr64;
	bcom_sram->size = (unsigned int) size64;

	/* Request region */
	if (!request_mem_region(bcom_sram->base_phys, bcom_sram->size, owner)) {
		printk(KERN_ERR "%s: bcom_sram_init: "
			"Couldn't request region !\n", owner);
		rv = -EBUSY;
		goto error_free;
	}

	/* Map SRAM */
		/* sram is not really __iomem */
	bcom_sram->base_virt = (void*) ioremap(bcom_sram->base_phys, bcom_sram->size);

	if (!bcom_sram->base_virt) {
		printk(KERN_ERR "%s: bcom_sram_init: "
			"Map error SRAM zone 0x%08lx (0x%0x)!\n",
			owner, (long)bcom_sram->base_phys, bcom_sram->size );
		rv = -ENOMEM;
		goto error_release;
	}

	/* Create an rheap (defaults to 32 bits word alignment) */
	bcom_sram->rh = rh_create(4);

	/* Attach the free zones */
#if 0
	/* Currently disabled ... for future use only */
	reg_addr_p = of_get_property(sram_node, "available", &psize);
#else
	regaddr_p = NULL;
	psize = 0;
#endif

	if (!regaddr_p || !psize) {
		/* Attach the whole zone */
		rh_attach_region(bcom_sram->rh, 0, bcom_sram->size);
	} else {
		/* Attach each zone independently */
		while (psize >= 2 * sizeof(u32)) {
			phys_addr_t zbase = of_translate_address(sram_node, regaddr_p);
			rh_attach_region(bcom_sram->rh, zbase - bcom_sram->base_phys, regaddr_p[1]);
			regaddr_p += 2;
			psize -= 2 * sizeof(u32);
		}
	}

	/* Init our spinlock */
	spin_lock_init(&bcom_sram->lock);

	return 0;

error_release:
	release_mem_region(bcom_sram->base_phys, bcom_sram->size);
error_free:
	kfree(bcom_sram);
	bcom_sram = NULL;

	return rv;
}
EXPORT_SYMBOL_GPL(bcom_sram_init);

void bcom_sram_cleanup(void)
{
	/* Free resources */
	if (bcom_sram) {
		rh_destroy(bcom_sram->rh);
		iounmap((void __iomem *)bcom_sram->base_virt);
		release_mem_region(bcom_sram->base_phys, bcom_sram->size);
		kfree(bcom_sram);
		bcom_sram = NULL;
	}
}
EXPORT_SYMBOL_GPL(bcom_sram_cleanup);

void* bcom_sram_alloc(int size, int align, phys_addr_t *phys)
{
	unsigned long offset;

	spin_lock(&bcom_sram->lock);
	offset = rh_alloc_align(bcom_sram->rh, size, align, NULL);
	spin_unlock(&bcom_sram->lock);

	if (IS_ERR_VALUE(offset))
		return NULL;

	*phys = bcom_sram->base_phys + offset;
	return bcom_sram->base_virt + offset;
}
EXPORT_SYMBOL_GPL(bcom_sram_alloc);

void bcom_sram_free(void *ptr)
{
	unsigned long offset;

	if (!ptr)
		return;

	offset = ptr - bcom_sram->base_virt;

	spin_lock(&bcom_sram->lock);
	rh_free(bcom_sram->rh, offset);
	spin_unlock(&bcom_sram->lock);
}
EXPORT_SYMBOL_GPL(bcom_sram_free);

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (C) 2007-2013 ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 * DMA driver for COH 901 318
 * Author: Per Friden <per.friden@stericsson.com>
 */

#ifndef COH901318_H
#define COH901318_H

#define MAX_DMA_PACKET_SIZE_SHIFT 11
#define MAX_DMA_PACKET_SIZE (1 << MAX_DMA_PACKET_SIZE_SHIFT)

struct device;

struct coh901318_pool {
	spinlock_t lock;
	struct dma_pool *dmapool;
	struct device *dev;

#ifdef CONFIG_DEBUG_FS
	int debugfs_pool_counter;
#endif
};

/**
 * struct coh901318_lli - linked list item for DMAC
 * @control: control settings for DMAC
 * @src_addr: transfer source address
 * @dst_addr: transfer destination address
 * @link_addr:  physical address to next lli
 * @virt_link_addr: virtual address of next lli (only used by pool_free)
 * @phy_this: physical address of current lli (only used by pool_free)
 */
struct coh901318_lli {
	u32 control;
	dma_addr_t src_addr;
	dma_addr_t dst_addr;
	dma_addr_t link_addr;

	void *virt_link_addr;
	dma_addr_t phy_this;
};

/**
 * coh901318_pool_create() - Creates an dma pool for lli:s
 * @pool: pool handle
 * @dev: dma device
 * @lli_nbr: number of lli:s in the pool
 * @algin: address alignemtn of lli:s
 * returns 0 on success otherwise none zero
 */
int coh901318_pool_create(struct coh901318_pool *pool,
			  struct device *dev,
			  size_t lli_nbr, size_t align);

/**
 * coh901318_pool_destroy() - Destroys the dma pool
 * @pool: pool handle
 * returns 0 on success otherwise none zero
 */
int coh901318_pool_destroy(struct coh901318_pool *pool);

/**
 * coh901318_lli_alloc() - Allocates a linked list
 *
 * @pool: pool handle
 * @len: length to list
 * return: none NULL if success otherwise NULL
 */
struct coh901318_lli *
coh901318_lli_alloc(struct coh901318_pool *pool,
		    unsigned int len);

/**
 * coh901318_lli_free() - Returns the linked list items to the pool
 * @pool: pool handle
 * @lli: reference to lli pointer to be freed
 */
void coh901318_lli_free(struct coh901318_pool *pool,
			struct coh901318_lli **lli);

/**
 * coh901318_lli_fill_memcpy() - Prepares the lli:s for dma memcpy
 * @pool: pool handle
 * @lli: allocated lli
 * @src: src address
 * @size: transfer size
 * @dst: destination address
 * @ctrl_chained: ctrl for chained lli
 * @ctrl_last: ctrl for the last lli
 * returns number of CPU interrupts for the lli, negative on error.
 */
int
coh901318_lli_fill_memcpy(struct coh901318_pool *pool,
			  struct coh901318_lli *lli,
			  dma_addr_t src, unsigned int size,
			  dma_addr_t dst, u32 ctrl_chained, u32 ctrl_last);

/**
 * coh901318_lli_fill_single() - Prepares the lli:s for dma single transfer
 * @pool: pool handle
 * @lli: allocated lli
 * @buf: transfer buffer
 * @size: transfer size
 * @dev_addr: address of periphal
 * @ctrl_chained: ctrl for chained lli
 * @ctrl_last: ctrl for the last lli
 * @dir: direction of transfer (to or from device)
 * returns number of CPU interrupts for the lli, negative on error.
 */
int
coh901318_lli_fill_single(struct coh901318_pool *pool,
			  struct coh901318_lli *lli,
			  dma_addr_t buf, unsigned int size,
			  dma_addr_t dev_addr, u32 ctrl_chained, u32 ctrl_last,
			  enum dma_transfer_direction dir);

/**
 * coh901318_lli_fill_single() - Prepares the lli:s for dma scatter list transfer
 * @pool: pool handle
 * @lli: allocated lli
 * @sg: scatter gather list
 * @nents: number of entries in sg
 * @dev_addr: address of periphal
 * @ctrl_chained: ctrl for chained lli
 * @ctrl: ctrl of middle lli
 * @ctrl_last: ctrl for the last lli
 * @dir: direction of transfer (to or from device)
 * @ctrl_irq_mask: ctrl mask for CPU interrupt
 * returns number of CPU interrupts for the lli, negative on error.
 */
int
coh901318_lli_fill_sg(struct coh901318_pool *pool,
		      struct coh901318_lli *lli,
		      struct scatterlist *sg, unsigned int nents,
		      dma_addr_t dev_addr, u32 ctrl_chained,
		      u32 ctrl, u32 ctrl_last,
		      enum dma_transfer_direction dir, u32 ctrl_irq_mask);

#endif /* COH901318_H */
                                                                                       /*
 * driver/dma/coh901318_lli.c
 *
 * Copyright (C) 2007-2009 ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 * Support functions for handling lli for dma
 * Author: Per Friden <per.friden@stericsson.com>
 */

#include <linux/spinlock.h>
#include <linux/memory.h>
#include <linux/gfp.h>
#include <linux/dmapool.h>
#include <linux/dmaengine.h>

#include "coh901318.h"

#if (defined(CONFIG_DEBUG_FS) && defined(CONFIG_U300_DEBUG))
#define DEBUGFS_POOL_COUNTER_RESET(pool) (pool->debugfs_pool_counter = 0)
#define DEBUGFS_POOL_COUNTER_ADD(pool, add) (pool->debugfs_pool_counter += add)
#else
#define DEBUGFS_POOL_COUNTER_RESET(pool)
#define DEBUGFS_POOL_COUNTER_ADD(pool, add)
#endif

static struct coh901318_lli *
coh901318_lli_next(struct coh901318_lli *data)
{
	if (data == NULL || data->link_addr == 0)
		return NULL;

	return (struct coh901318_lli *) data->virt_link_addr;
}

int coh901318_pool_create(struct coh901318_pool *pool,
			  struct device *dev,
			  size_t size, size_t align)
{
	spin_lock_init(&pool->lock);
	pool->dev = dev;
	pool->dmapool = dma_pool_create("lli_pool", dev, size, align, 0);

	DEBUGFS_POOL_COUNTER_RESET(pool);
	return 0;
}

int coh901318_pool_destroy(struct coh901318_pool *pool)
{

	dma_pool_destroy(pool->dmapool);
	return 0;
}

struct coh901318_lli *
coh901318_lli_alloc(struct coh901318_pool *pool, unsigned int len)
{
	int i;
	struct coh901318_lli *head;
	struct coh901318_lli *lli;
	struct coh901318_lli *lli_prev;
	dma_addr_t phy;

	if (len == 0)
		return NULL;

	spin_lock(&pool->lock);

	head = dma_pool_alloc(pool->dmapool, GFP_NOWAIT, &phy);

	if (head == NULL)
		goto err;

	DEBUGFS_POOL_COUNTER_ADD(pool, 1);

	lli = head;
	lli->phy_this = phy;
	lli->link_addr = 0x00000000;
	lli->virt_link_addr = 0x00000000U;

	for (i = 1; i < len; i++) {
		lli_prev = lli;

		lli = dma_pool_alloc(pool->dmapool, GFP_NOWAIT, &phy);

		if (lli == NULL)
			goto err_clean_up;

		DEBUGFS_POOL_COUNTER_ADD(pool, 1);
		lli->phy_this = phy;
		lli->link_addr = 0x00000000;
		lli->virt_link_addr = 0x00000000U;

		lli_prev->link_addr = phy;
		lli_prev->virt_link_addr = lli;
	}

	spin_unlock(&pool->lock);

	return head;

 err:
	spin_unlock(&pool->lock);
	return NULL;

 err_clean_up:
	lli_prev->link_addr = 0x00000000U;
	spin_unlock(&pool->lock);
	coh901318_lli_free(pool, &head);
	return NULL;
}

void coh901318_lli_free(struct coh901318_pool *pool,
			struct coh901318_lli **lli)
{
	struct coh901318_lli *l;
	struct coh901318_lli *next;

	if (lli == NULL)
		return;

	l = *lli;

	if (l == NULL)
		return;

	spin_lock(&pool->lock);

	while (l->link_addr) {
		next = l->virt_link_addr;
		dma_pool_free(pool->dmapool, l, l->phy_this);
		DEBUGFS_POOL_COUNTER_ADD(pool, -1);
		l = next;
	}
	dma_pool_free(pool->dmapool, l, l->phy_this);
	DEBUGFS_POOL_COUNTER_ADD(pool, -1);

	spin_unlock(&pool->lock);
	*lli = NULL;
}

int
coh901318_lli_fill_memcpy(struct coh901318_pool *pool,
			  struct coh901318_lli *lli,
			  dma_addr_t source, unsigned int size,
			  dma_addr_t destination, u32 ctrl_chained,
			  u32 ctrl_eom)
{
	int s = size;
	dma_addr_t src = source;
	dma_addr_t dst = destination;

	lli->src_addr = src;
	lli->dst_addr = dst;

	while (lli->link_addr) {
		lli->control = ctrl_chained | MAX_DMA_PACKET_SIZE;
		lli->src_addr = src;
		lli->dst_addr = dst;

		s -= MAX_DMA_PACKET_SIZE;
		lli = coh901318_lli_next(lli);

		src += MAX_DMA_PACKET_SIZE;
		dst += MAX_DMA_PACKET_SIZE;
	}

	lli->control = ctrl_eom | s;
	lli->src_addr = src;
	lli->dst_addr = dst;

	return 0;
}

int
coh901318_lli_fill_single(struct coh901318_pool *pool,
			  struct coh901318_lli *lli,
			  dma_addr_t buf, unsigned int size,
			  dma_addr_t dev_addr, u32 ctrl_chained, u32 ctrl_eom,
			  enum dma_transfer_direction dir)
{
	int s = size;
	dma_addr_t src;
	dma_addr_t dst;


	if (dir == DMA_MEM_TO_DEV) {
		src = buf;
		dst = dev_addr;

	} else if (dir == DMA_DEV_TO_MEM) {

		src = dev_addr;
		dst = buf;
	} else {
		return -EINVAL;
	}

	while (lli->link_addr) {
		size_t block_size = MAX_DMA_PACKET_SIZE;
		lli->control = ctrl_chained | MAX_DMA_PACKET_SIZE;

		/* If we are on the next-to-final block and there will
		 * be less than half a DMA packet left for the last
		 * block, then we want to make this block a little
		 * smaller to balance the sizes. This is meant to
		 * avoid too small transfers if the buffer size is
		 * (MAX_DMA_PACKET_SIZE*N + 1) */
		if (s < (MAX_DMA_PACKET_SIZE + MAX_DMA_PACKET_SIZE/2))
			block_size = MAX_DMA_PACKET_SIZE/2;

		s -= block_size;
		lli->src_addr = src;
		lli->dst_addr = dst;

		lli = coh901318_lli_next(lli);

		if (dir == DMA_MEM_TO_DEV)
			src += block_size;
		else if (dir == DMA_DEV_TO_MEM)
			dst += block_size;
	}

	lli->control = ctrl_eom | s;
	lli->src_addr = src;
	lli->dst_addr = dst;

	return 0;
}

int
coh901318_lli_fill_sg(struct coh901318_pool *pool,
		      struct coh901318_lli *lli,
		      struct scatterlist *sgl, unsigned int nents,
		      dma_addr_t dev_addr, u32 ctrl_chained, u32 ctrl,
		      u32 ctrl_last,
		      enum dma_transfer_direction dir, u32 ctrl_irq_mask)
{
	int i;
	struct scatterlist *sg;
	u32 ctrl_sg;
	dma_addr_t src = 0;
	dma_addr_t dst = 0;
	u32 bytes_to_transfer;
	u32 elem_size;

	if (lli == NULL)
		goto err;

	spin_lock(&pool->lock);

	if (dir == DMA_MEM_TO_DEV)
		dst = dev_addr;
	else if (dir == DMA_DEV_TO_MEM)
		src = dev_addr;
	else
		goto err;

	for_each_sg(sgl, sg, nents, i) {
		if (sg_is_chain(sg)) {
			/* sg continues to the next sg-element don't
			 * send ctrl_finish until the last
			 * sg-element in the chain
			 */
			ctrl_sg = ctrl_chained;
		} else if (i == nents - 1)
			ctrl_sg = ctrl_last;
		else
			ctrl_sg = ctrl ? ctrl : ctrl_last;


		if (dir == DMA_MEM_TO_DEV)
			/* increment source address */
			src = sg_dma_address(sg);
		else
			/* increment destination address */
			dst = sg_dma_address(sg);

		bytes_to_transfer = sg_dma_len(sg);

		while (bytes_to_transfer) {
			u32 val;

			if (bytes_to_transfer > MAX_DMA_PACKET_SIZE) {
				elem_size = MAX_DMA_PACKET_SIZE;
				val = ctrl_chained;
			} else {
				elem_size = bytes_to_transfer;
				val = ctrl_sg;
			}

			lli->control = val | elem_size;
			lli->src_addr = src;
			lli->dst_addr = dst;

			if (dir == DMA_DEV_TO_MEM)
				dst += elem_size;
			else
				src += elem_size;

			BUG_ON(lli->link_addr & 3);

			bytes_to_transfer -= elem_size;
			lli = coh901318_lli_next(lli);
		}

	}
	spin_unlock(&pool->lock);

	return 0;
 err:
	spin_unlock(&pool->lock);
	return -EINVAL;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   #include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/of_dma.h>
#include <linux/of_irq.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include "dmaengine.h"

#define DESC_TYPE	27
#define DESC_TYPE_HOST	0x10
#define DESC_TYPE_TEARD	0x13

#define TD_DESC_IS_RX	(1 << 16)
#define TD_DESC_DMA_NUM	10

#define DESC_LENGTH_BITS_NUM	21

#define DESC_TYPE_USB	(5 << 26)
#define DESC_PD_COMPLETE	(1 << 31)

/* DMA engine */
#define DMA_TDFDQ	4
#define DMA_TXGCR(x)	(0x800 + (x) * 0x20)
#define DMA_RXGCR(x)	(0x808 + (x) * 0x20)
#define RXHPCRA0		4

#define GCR_CHAN_ENABLE		(1 << 31)
#define GCR_TEARDOWN		(1 << 30)
#define GCR_STARV_RETRY		(1 << 24)
#define GCR_DESC_TYPE_HOST	(1 << 14)

/* DMA scheduler */
#define DMA_SCHED_CTRL		0
#define DMA_SCHED_CTRL_EN	(1 << 31)
#define DMA_SCHED_WORD(x)	((x) * 4 + 0x800)

#define SCHED_ENTRY0_CHAN(x)	((x) << 0)
#define SCHED_ENTRY0_IS_RX	(1 << 7)

#define SCHED_ENTRY1_CHAN(x)	((x) << 8)
#define SCHED_ENTRY1_IS_RX	(1 << 15)

#define SCHED_ENTRY2_CHAN(x)	((x) << 16)
#define SCHED_ENTRY2_IS_RX	(1 << 23)

#define SCHED_ENTRY3_CHAN(x)	((x) << 24)
#define SCHED_ENTRY3_IS_RX	(1 << 31)

/* Queue manager */
/* 4 KiB of memory for descriptors, 2 for each endpoint */
#define ALLOC_DECS_NUM		128
#define DESCS_AREAS		1
#define TOTAL_DESCS_NUM		(ALLOC_DECS_NUM * DESCS_AREAS)
#define QMGR_SCRATCH_SIZE	(TOTAL_DESCS_NUM * 4)

#define QMGR_LRAM0_BASE		0x80
#define QMGR_LRAM_SIZE		0x84
#define QMGR_LRAM1_BASE		0x88
#define QMGR_MEMBASE(x)		(0x1000 + (x) * 0x10)
#define QMGR_MEMCTRL(x)		(0x1004 + (x) * 0x10)
#define QMGR_MEMCTRL_IDX_SH	16
#define QMGR_MEMCTRL_DESC_SH	8

#define QMGR_NUM_PEND	5
#define QMGR_PEND(x)	(0x90 + (x) * 4)

#define QMGR_PENDING_SLOT_Q(x)	(x / 32)
#define QMGR_PENDING_BIT_Q(x)	(x % 32)

#define QMGR_QUEUE_A(n)	(0x2000 + (n) * 0x10)
#define QMGR_QUEUE_B(n)	(0x2004 + (n) * 0x10)
#define QMGR_QUEUE_C(n)	(0x2008 + (n) * 0x10)
#define QMGR_QUEUE_D(n)	(0x200c + (n) * 0x10)

/* Glue layer specific */
/* USBSS  / USB AM335x */
#define USBSS_IRQ_STATUS	0x28
#define USBSS_IRQ_ENABLER	0x2c
#define USBSS_IRQ_CLEARR	0x30

#define USBSS_IRQ_PD_COMP	(1 <<  2)

/* Packet Descriptor */
#define PD2_ZERO_LENGTH		(1 << 19)

struct cppi41_channel {
	struct dma_chan chan;
	struct dma_async_tx_descriptor txd;
	struct cppi41_dd *cdd;
	struct cppi41_desc *desc;
	dma_addr_t desc_phys;
	void __iomem *gcr_reg;
	int is_tx;
	u32 residue;

	unsigned int q_num;
	unsigned int q_comp_num;
	unsigned int port_num;

	unsigned td_retry;
	unsigned td_queued:1;
	unsigned td_seen:1;
	unsigned td_desc_seen:1;
};

struct cppi41_desc {
	u32 pd0;
	u32 pd1;
	u32 pd2;
	u32 pd3;
	u32 pd4;
	u32 pd5;
	u32 pd6;
	u32 pd7;
} __aligned(32);

struct chan_queues {
	u16 submit;
	u16 complete;
};

struct cppi41_dd {
	struct dma_device ddev;

	void *qmgr_scratch;
	dma_addr_t scratch_phys;

	struct cppi41_desc *cd;
	dma_addr_t descs_phys;
	u32 first_td_desc;
	struct cppi41_channel *chan_busy[ALLOC_DECS_NUM];

	void __iomem *usbss_mem;
	void __iomem *ctrl_mem;
	void __iomem *sched_mem;
	void __iomem *qmgr_mem;
	unsigned int irq;
	const struct chan_queues *queues_rx;
	const struct chan_queues *queues_tx;
	struct chan_queues td_queue;

	/* context for suspend/resume */
	unsigned int dma_tdfdq;
};

#define FIST_COMPLETION_QUEUE	93
static struct chan_queues usb_queues_tx[] = {
	/* USB0 ENDP 1 */
	[ 0] = { .submit = 32, .complete =  93},
	[ 1] = { .submit = 34, .complete =  94},
	[ 2] = { .submit = 36, .complete =  95},
	[ 3] = { .submit = 38, .complete =  96},
	[ 4] = { .submit = 40, .complete =  97},
	[ 5] = { .submit = 42, .complete =  98},
	[ 6] = { .submit = 44, .complete =  99},
	[ 7] = { .submit = 46, .complete = 100},
	[ 8] = { .submit = 48, .complete = 101},
	[ 9] = { .submit = 50, .complete = 102},
	[10] = { .submit = 52, .complete = 103},
	[11] = { .submit = 54, .complete = 104},
	[12] = { .submit = 56, .complete = 105},
	[13] = { .submit = 58, .complete = 106},
	[14] = { .submit = 60, .complete = 107},

	/* USB1 ENDP1 */
	[15] = { .submit = 62, .complete = 125},
	[16] = { .submit = 64, .complete = 126},
	[17] = { .submit = 66, .complete = 127},
	[18] = { .submit = 68, .complete = 128},
	[19] = { .submit = 70, .complete = 129},
	[20] = { .submit = 72, .complete = 130},
	[21] = { .submit = 74, .complete = 131},
	[22] = { .submit = 76, .complete = 132},
	[23] = { .submit = 78, .complete = 133},
	[24] = { .submit = 80, .complete = 134},
	[25] = { .submit = 82, .complete = 135},
	[26] = { .submit = 84, .complete = 136},
	[27] = { .submit = 86, .complete = 137},
	[28] = { .submit = 88, .complete = 138},
	[29] = { .submit = 90, .complete = 139},
};

static const struct chan_queues usb_queues_rx[] = {
	/* USB0 ENDP 1 */
	[ 0] = { .submit =  1, .complete = 109},
	[ 1] = { .submit =  2, .complete = 110},
	[ 2] = { .submit =  3, .complete = 111},
	[ 3] = { .submit =  4, .complete = 112},
	[ 4] = { .submit =  5, .complete = 113},
	[ 5] = { .submit =  6, .complete = 114},
	[ 6] = { .submit =  7, .complete = 115},
	[ 7] = { .submit =  8, .complete = 116},
	[ 8] = { .submit =  9, .complete = 117},
	[ 9] = { .submit = 10, .complete = 118},
	[10] = { .submit = 11, .complete = 119},
	[11] = { .submit = 12, .complete = 120},
	[12] = { .submit = 13, .complete = 121},
	[13] = { .submit = 14, .complete = 122},
	[14] = { .submit = 15, .complete = 123},

	/* USB1 ENDP 1 */
	[15] = { .submit = 16, .complete = 141},
	[16] = { .submit = 17, .complete = 142},
	[17] = { .submit = 18, .complete = 143},
	[18] = { .submit = 19, .complete = 144},
	[19] = { .submit = 20, .complete = 145},
	[20] = { .submit = 21, .complete = 146},
	[21] = { .submit = 22, .complete = 147},
	[22] = { .submit = 23, .complete = 148},
	[23] = { .submit = 24, .complete = 149},
	[24] = { .submit = 25, .complete = 150},
	[25] = { .submit = 26, .complete = 151},
	[26] = { .submit = 27, .complete = 152},
	[27] = { .submit = 28, .complete = 153},
	[28] = { .submit = 29, .complete = 154},
	[29] = { .submit = 30, .complete = 155},
};

struct cppi_glue_infos {
	irqreturn_t (*isr)(int irq, void *data);
	const struct chan_queues *queues_rx;
	const struct chan_queues *queues_tx;
	struct chan_queues td_queue;
};

static struct cppi41_channel *to_cpp41_chan(struct dma_chan *c)
{
	return container_of(c, struct cppi41_channel, chan);
}

static struct cppi41_channel *desc_to_chan(struct cppi41_dd *cdd, u32 desc)
{
	struct cppi41_channel *c;
	u32 descs_size;
	u32 desc_num;

	descs_size = sizeof(struct cppi41_desc) * ALLOC_DECS_NUM;

	if (!((desc >= cdd->descs_phys) &&
			(desc < (cdd->descs_phys + descs_size)))) {
		return NULL;
	}

	desc_num = (desc - cdd->descs_phys) / sizeof(struct cppi41_desc);
	BUG_ON(desc_num >= ALLOC_DECS_NUM);
	c = cdd->chan_busy[desc_num];
	cdd->chan_busy[desc_num] = NULL;
	return c;
}

static void cppi_writel(u32 val, void *__iomem *mem)
{
	__raw_writel(val, mem);
}

static u32 cppi_readl(void *__iomem *mem)
{
	return __raw_readl(mem);
}

static u32 pd_trans_len(u32 val)
{
	return val & ((1 << (DESC_LENGTH_BITS_NUM + 1)) - 1);
}

static u32 cppi41_pop_desc(struct cppi41_dd *cdd, unsigned queue_num)
{
	u32 desc;

	desc = cppi_readl(cdd->qmgr_mem + QMGR_QUEUE_D(queue_num));
	desc &= ~0x1f;
	return desc;
}

static irqreturn_t cppi41_irq(int irq, void *data)
{
	struct cppi41_dd *cdd = data;
	struct cppi41_channel *c;
	u32 status;
	int i;

	status = cppi_readl(cdd->usbss_mem + USBSS_IRQ_STATUS);
	if (!(status & USBSS_IRQ_PD_COMP))
		return IRQ_NONE;
	cppi_writel(status, cdd->usbss_mem + USBSS_IRQ_STATUS);

	for (i = QMGR_PENDING_SLOT_Q(FIST_COMPLETION_QUEUE); i < QMGR_NUM_PEND;
			i++) {
		u32 val;
		u32 q_num;

		val = cppi_readl(cdd->qmgr_mem + QMGR_PEND(i));
		if (i == QMGR_PENDING_SLOT_Q(FIST_COMPLETION_QUEUE) && val) {
			u32 mask;
			/* set corresponding bit for completetion Q 93 */
			mask = 1 << QMGR_PENDING_BIT_Q(FIST_COMPLETION_QUEUE);
			/* not set all bits for queues less than Q 93 */
			mask--;
			/* now invert and keep only Q 93+ set */
			val &= ~mask;
		}

		if (val)
			__iormb();

		while (val) {
			u32 desc, len;

			q_num = __fls(val);
			val &= ~(1 << q_num);
			q_num += 32 * i;
			desc = cppi41_pop_desc(cdd, q_num);
			c = desc_to_chan(cdd, desc);
			if (WARN_ON(!c)) {
				pr_err("%s() q %d desc %08x\n", __func__,
						q_num, desc);
				continue;
			}

			if (c->desc->pd2 & PD2_ZERO_LENGTH)
				len = 0;
			else
				len = pd_trans_len(c->desc->pd0);

			c->residue = pd_trans_len(c->desc->pd6) - len;
			dma_cookie_complete(&c->txd);
			c->txd.callback(c->txd.callback_param);
		}
	}
	return IRQ_HANDLED;
}

static dma_cookie_t cppi41_tx_submit(struct dma_async_tx_descriptor *tx)
{
	dma_cookie_t cookie;

	cookie = dma_cookie_assign(tx);

	return cookie;
}

static int cppi41_dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct cppi41_channel *c = to_cpp41_chan(chan);

	dma_cookie_init(chan);
	dma_async_tx_descriptor_init(&c->txd, chan);
	c->txd.tx_submit = cppi41_tx_submit;

	if (!c->is_tx)
		cppi_writel(c->q_num, c->gcr_reg + RXHPCRA0);

	return 0;
}

static void cppi41_dma_free_chan_resources(struct dma_chan *chan)
{
}

static enum dma_status cppi41_dma_tx_status(struct dma_chan *chan,
	dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	struct cppi41_channel *c = to_cpp41_chan(chan);
	enum dma_status ret;

	/* lock */
	ret = dma_cookie_status(chan, cookie, txstate);
	if (txstate && ret == DMA_COMPLETE)
		txstate->residue = c->residue;
	/* unlock */

	return ret;
}

static void push_desc_queue(struct cppi41_channel *c)
{
	struct cppi41_dd *cdd = c->cdd;
	u32 desc_num;
	u32 desc_phys;
	u32 reg;

	desc_phys = lower_32_bits(c->desc_phys);
	desc_num = (desc_phys - cdd->descs_phys) / sizeof(struct cppi41_desc);
	WARN_ON(cdd->chan_busy[desc_num]);
	cdd->chan_busy[desc_num] = c;

	reg = (sizeof(struct cppi41_desc) - 24) / 4;
	reg |= desc_phys;
	cppi_writel(reg, cdd->qmgr_mem + QMGR_QUEUE_D(c->q_num));
}

static void cppi41_dma_issue_pending(struct dma_chan *chan)
{
	struct cppi41_channel *c = to_cpp41_chan(chan);
	u32 reg;

	c->residue = 0;

	reg = GCR_CHAN_ENABLE;
	if (!c->is_tx) {
		reg |= GCR_STARV_RETRY;
		reg |= GCR_DESC_TYPE_HOST;
		reg |= c->q_comp_num;
	}

	cppi_writel(reg, c->gcr_reg);

	/*
	 * We don't use writel() but __raw_writel() so we have to make sure
	 * that the DMA descriptor in coherent memory made to the main memory
	 * before starting the dma engine.
	 */
	__iowmb();
	push_desc_queue(c);
}

static u32 get_host_pd0(u32 length)
{
	u32 reg;

	reg = DESC_TYPE_HOST << DESC_TYPE;
	reg |= length;

	return reg;
}

static u32 get_host_pd1(struct cppi41_channel *c)
{
	u32 reg;

	reg = 0;

	return reg;
}

static u32 get_host_pd2(struct cppi41_channel *c)
{
	u32 reg;

	reg = DESC_TYPE_USB;
	reg |= c->q_comp_num;

	return reg;
}

static u32 get_host_pd3(u32 length)
{
	u32 reg;

	/* PD3 = packet size */
	reg = length;

	return reg;
}

static u32 get_host_pd6(u32 length)
{
	u32 reg;

	/* PD6 buffer size */
	reg = DESC_PD_COMPLETE;
	reg |= length;

	return reg;
}

static u32 get_host_pd4_or_7(u32 addr)
{
	u32 reg;

	reg = addr;

	return reg;
}

static u32 get_host_pd5(void)
{
	u32 reg;

	reg = 0;

	return reg;
}

static struct dma_async_tx_descriptor *cppi41_dma_prep_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl, unsigned sg_len,
	enum dma_transfer_direction dir, unsigned long tx_flags, void *context)
{
	struct cppi41_channel *c = to_cpp41_chan(chan);
	struct cppi41_desc *d;
	struct scatterlist *sg;
	unsigned int i;
	unsigned int num;

	num = 0;
	d = c->desc;
	for_each_sg(sgl, sg, sg_len, i) {
		u32 addr;
		u32 len;

		/* We need to use more than one desc once musb supports sg */
		BUG_ON(num > 0);
		addr = lower_32_bits(sg_dma_address(sg));
		len = sg_dma_len(sg);

		d->pd0 = get_host_pd0(len);
		d->pd1 = get_host_pd1(c);
		d->pd2 = get_host_pd2(c);
		d->pd3 = get_host_pd3(len);
		d->pd4 = get_host_pd4_or_7(addr);
		d->pd5 = get_host_pd5();
		d->pd6 = get_host_pd6(len);
		d->pd7 = get_host_pd4_or_7(addr);

		d++;
	}

	return &c->txd;
}

static void cppi41_compute_td_desc(struct cppi41_desc *d)
{
	d->pd0 = DESC_TYPE_TEARD << DESC_TYPE;
}

static int cppi41_tear_down_chan(struct cppi41_channel *c)
{
	struct cppi41_dd *cdd = c->cdd;
	struct cppi41_desc *td;
	u32 reg;
	u32 desc_phys;
	u32 td_desc_phys;

	td = cdd->cd;
	td += cdd->first_td_desc;

	td_desc_phys = cdd->descs_phys;
	td_desc_phys += cdd->first_td_desc * sizeof(struct cppi41_desc);

	if (!c->td_queued) {
		cppi41_compute_td_desc(td);
		__iowmb();

		reg = (sizeof(struct cppi41_desc) - 24) / 4;
		reg |= td_desc_phys;
		cppi_writel(reg, cdd->qmgr_mem +
				QMGR_QUEUE_D(cdd->td_queue.submit));

		reg = GCR_CHAN_ENABLE;
		if (!c->is_tx) {
			reg |= GCR_STARV_RETRY;
			reg |= GCR_DESC_TYPE_HOST;
			reg |= c->q_comp_num;
		}
		reg |= GCR_TEARDOWN;
		cppi_writel(reg, c->gcr_reg);
		c->td_queued = 1;
		c->td_retry = 500;
	}

	if (!c->td_seen || !c->td_desc_seen) {

		desc_phys = cppi41_pop_desc(cdd, cdd->td_queue.complete);
		if (!desc_phys)
			desc_phys = cppi41_pop_desc(cdd, c->q_comp_num);

		if (desc_phys == c->desc_phys) {
			c->td_desc_seen = 1;

		} else if (desc_phys == td_desc_phys) {
			u32 pd0;

			__iormb();
			pd0 = td->pd0;
			WARN_ON((pd0 >> DESC_TYPE) != DESC_TYPE_TEARD);
			WARN_ON(!c->is_tx && !(pd0 & TD_DESC_IS_RX));
			WARN_ON((pd0 & 0x1f) != c->port_num);
			c->td_seen = 1;
		} else if (desc_phys) {
			WARN_ON_ONCE(1);
		}
	}
	c->td_retry--;
	/*
	 * If the TX descriptor / channel is in use, the caller needs to poke
	 * his TD bit multiple times. After that he hardware releases the
	 * transfer descriptor followed by TD descriptor. Waiting seems not to
	 * cause any difference.
	 * RX seems to be thrown out right away. However once the TearDown
	 * descriptor gets through we are done. If we have seens the transfer
	 * descriptor before the TD we fetch it from enqueue, it has to be
	 * there waiting for us.
	 */
	if (!c->td_seen && c->td_retry) {
		udelay(1);
		return -EAGAIN;
	}
	WARN_ON(!c->td_retry);

	if (!c->td_desc_seen) {
		desc_phys = cppi41_pop_desc(cdd, c->q_num);
		if (!desc_phys)
			desc_phys = cppi41_pop_desc(cdd, c->q_comp_num);
		WARN_ON(!desc_phys);
	}

	c->td_queued = 0;
	c->td_seen = 0;
	c->td_desc_seen = 0;
	cppi_writel(0, c->gcr_reg);
	return 0;
}

static int cppi41_stop_chan(struct dma_chan *chan)
{
	struct cppi41_channel *c = to_cpp41_chan(chan);
	struct cppi41_dd *cdd = c->cdd;
	u32 desc_num;
	u32 desc_phys;
	int ret;

	desc_phys = lower_32_bits(c->desc_phys);
	desc_num = (desc_phys - cdd->descs_phys) / sizeof(struct cppi41_desc);
	if (!cdd->chan_busy[desc_num])
		return 0;

	ret = cppi41_tear_down_chan(c);
	if (ret)
		return ret;

	WARN_ON(!cdd->chan_busy[desc_num]);
	cdd->chan_busy[desc_num] = NULL;

	return 0;
}

static void cleanup_chans(struct cppi41_dd *cdd)
{
	while (!list_empty(&cdd->ddev.channels)) {
		struct cppi41_channel *cchan;

		cchan = list_first_entry(&cdd->ddev.channels,
				struct cppi41_channel, chan.device_node);
		list_del(&cchan->chan.device_node);
		kfree(cchan);
	}
}

static int cppi41_add_chans(struct device *dev, struct cppi41_dd *cdd)
{
	struct cppi41_channel *cchan;
	int i;
	int ret;
	u32 n_chans;

	ret = of_property_read_u32(dev->of_node, "#dma-channels",
			&n_chans);
	if (ret)
		return ret;
	/*
	 * The channels can only be used as TX or as RX. So we add twice
	 * that much dma channels because USB can only do RX or TX.
	 */
	n_chans *= 2;

	for (i = 0; i < n_chans; i++) {
		cchan = kzalloc(sizeof(*cchan), GFP_KERNEL);
		if (!cchan)
			goto err;

		cchan->cdd = cdd;
		if (i & 1) {
			cchan->gcr_reg = cdd->ctrl_mem + DMA_TXGCR(i >> 1);
			cchan->is_tx = 1;
		} else {
			cchan->gcr_reg = cdd->ctrl_mem + DMA_RXGCR(i >> 1);
			cchan->is_tx = 0;
		}
		cchan->port_num = i >> 1;
		cchan->desc = &cdd->cd[i];
		cchan->desc_phys = cdd->descs_phys;
		cchan->desc_phys += i * sizeof(struct cppi41_desc);
		cchan->chan.device = &cdd->ddev;
		list_add_tail(&cchan->chan.device_node, &cdd->ddev.channels);
	}
	cdd->first_td_desc = n_chans;

	return 0;
err:
	cleanup_chans(cdd);
	return -ENOMEM;
}

static void purge_descs(struct device *dev, struct cppi41_dd *cdd)
{
	unsigned int mem_decs;
	int i;

	mem_decs = ALLOC_DECS_NUM * sizeof(struct cppi41_desc);

	for (i = 0; i < DESCS_AREAS; i++) {

		cppi_writel(0, cdd->qmgr_mem + QMGR_MEMBASE(i));
		cppi_writel(0, cdd->qmgr_mem + QMGR_MEMCTRL(i));

		dma_free_coherent(dev, mem_decs, cdd->cd,
				cdd->descs_phys);
	}
}

static void disable_sched(struct cppi41_dd *cdd)
{
	cppi_writel(0, cdd->sched_mem + DMA_SCHED_CTRL);
}

static void deinit_cppi41(struct device *dev, struct cppi41_dd *cdd)
{
	disable_sched(cdd);

	purge_descs(dev, cdd);

	cppi_writel(0, cdd->qmgr_mem + QMGR_LRAM0_BASE);
	cppi_writel(0, cdd->qmgr_mem + QMGR_LRAM0_BASE);
	dma_free_coherent(dev, QMGR_SCRATCH_SIZE, cdd->qmgr_scratch,
			cdd->scratch_phys);
}

static int init_descs(struct device *dev, struct cppi41_dd *cdd)
{
	unsigned int desc_size;
	unsigned int mem_decs;
	int i;
	u32 reg;
	u32 idx;

	BUILD_BUG_ON(sizeof(struct cppi41_desc) &
			(sizeof(struct cppi41_desc) - 1));
	BUILD_BUG_ON(sizeof(struct cppi41_desc) < 32);
	BUILD_BUG_ON(ALLOC_DECS_NUM < 32);

	desc_size = sizeof(struct cppi41_desc);
	mem_decs = ALLOC_DECS_NUM * desc_size;

	idx = 0;
	for (i = 0; i < DESCS_AREAS; i++) {

		reg = idx << QMGR_MEMCTRL_IDX_SH;
		reg |= (ilog2(desc_size) - 5) << QMGR_MEMCTRL_DESC_SH;
		reg |= ilog2(ALLOC_DECS_NUM) - 5;

		BUILD_BUG_ON(DESCS_AREAS != 1);
		cdd->cd = dma_alloc_coherent(dev, mem_decs,
				&cdd->descs_phys, GFP_KERNEL);
		if (!cdd->cd)
			return -ENOMEM;

		cppi_writel(cdd->descs_phys, cdd->qmgr_mem + QMGR_MEMBASE(i));
		cppi_writel(reg, cdd->qmgr_mem + QMGR_MEMCTRL(i));

		idx += ALLOC_DECS_NUM;
	}
	return 0;
}

static void init_sched(struct cppi41_dd *cdd)
{
	unsigned ch;
	unsigned word;
	u32 reg;

	word = 0;
	cppi_writel(0, cdd->sched_mem + DMA_SCHED_CTRL);
	for (ch = 0; ch < 15 * 2; ch += 2) {

		reg = SCHED_ENTRY0_CHAN(ch);
		reg |= SCHED_ENTRY1_CHAN(ch) | SCHED_ENTRY1_IS_RX;

		reg |= SCHED_ENTRY2_CHAN(ch + 1);
		reg |= SCHED_ENTRY3_CHAN(ch + 1) | SCHED_ENTRY3_IS_RX;
		cppi_writel(reg, cdd->sched_mem + DMA_SCHED_WORD(word));
		word++;
	}
	reg = 15 * 2 * 2 - 1;
	reg |= DMA_SCHED_CTRL_EN;
	cppi_writel(reg, cdd->sched_mem + DMA_SCHED_CTRL);
}

static int init_cppi41(struct device *dev, struct cppi41_dd *cdd)
{
	int ret;

	BUILD_BUG_ON(QMGR_SCRATCH_SIZE > ((1 << 14) - 1));
	cdd->qmgr_scratch = dma_alloc_coherent(dev, QMGR_SCRATCH_SIZE,
			&cdd->scratch_phys, GFP_KERNEL);
	if (!cdd->qmgr_scratch)
		return -ENOMEM;

	cppi_writel(cdd->scratch_phys, cdd->qmgr_mem + QMGR_LRAM0_BASE);
	cppi_writel(QMGR_SCRATCH_SIZE, cdd->qmgr_mem + QMGR_LRAM_SIZE);
	cppi_writel(0, cdd->qmgr_mem + QMGR_LRAM1_BASE);

	ret = init_descs(dev, cdd);
	if (ret)
		goto err_td;

	cppi_writel(cdd->td_queue.submit, cdd->ctrl_mem + DMA_TDFDQ);
	init_sched(cdd);
	return 0;
err_td:
	deinit_cppi41(dev, cdd);
	return ret;
}

static struct platform_driver cpp41_dma_driver;
/*
 * The param format is:
 * X Y
 * X: Port
 * Y: 0 = RX else TX
 */
#define INFO_PORT	0
#define INFO_IS_TX	1

static bool cpp41_dma_filter_fn(struct dma_chan *chan, void *param)
{
	struct cppi41_channel *cchan;
	struct cppi41_dd *cdd;
	const struct chan_queues *queues;
	u32 *num = param;

	if (chan->device->dev->driver != &cpp41_dma_driver.driver)
		return false;

	cchan = to_cpp41_chan(chan);

	if (cchan->port_num != num[INFO_PORT])
		return false;

	if (cchan->is_tx && !num[INFO_IS_TX])
		return false;
	cdd = cchan->cdd;
	if (cchan->is_tx)
		queues = cdd->queues_tx;
	else
		queues = cdd->queues_rx;

	BUILD_BUG_ON(ARRAY_SIZE(usb_queues_rx) != ARRAY_SIZE(usb_queues_tx));
	if (WARN_ON(cchan->port_num > ARRAY_SIZE(usb_queues_rx)))
		return false;

	cchan->q_num = queues[cchan->port_num].submit;
	cchan->q_comp_num = queues[cchan->port_num].complete;
	return true;
}

static struct of_dma_filter_info cpp41_dma_info = {
	.filter_fn = cpp41_dma_filter_fn,
};

static struct dma_chan *cppi41_dma_xlate(struct of_phandle_args *dma_spec,
		struct of_dma *ofdma)
{
	int count = dma_spec->args_count;
	struct of_dma_filter_info *info = ofdma->of_dma_data;

	if (!info || !info->filter_fn)
		return NULL;

	if (count != 2)
		return NULL;

	return dma_request_channel(info->dma_cap, info->filter_fn,
			&dma_spec->args[0]);
}

static const struct cppi_glue_infos usb_infos = {
	.isr = cppi41_irq,
	.queues_rx = usb_queues_rx,
	.queues_tx = usb_queues_tx,
	.td_queue = { .submit = 31, .complete = 0 },
};

static const struct of_device_id cppi41_dma_ids[] = {
	{ .compatible = "ti,am3359-cppi41", .data = &usb_infos},
	{},
};
MODULE_DEVICE_TABLE(of, cppi41_dma_ids);

static const struct cppi_glue_infos *get_glue_info(struct device *dev)
{
	const struct of_device_id *of_id;

	of_id = of_match_node(cppi41_dma_ids, dev->of_node);
	if (!of_id)
		return NULL;
	return of_id->data;
}

#define CPPI41_DMA_BUSWIDTHS	(BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) | \
				BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) | \
				BIT(DMA_SLAVE_BUSWIDTH_3_BYTES) | \
				BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))

static int cppi41_dma_probe(struct platform_device *pdev)
{
	struct cppi41_dd *cdd;
	struct device *dev = &pdev->dev;
	const struct cppi_glue_infos *glue_info;
	int irq;
	int ret;

	glue_info = get_glue_info(dev);
	if (!glue_info)
		return -EINVAL;

	cdd = devm_kzalloc(&pdev->dev, sizeof(*cdd), GFP_KERNEL);
	if (!cdd)
		return -ENOMEM;

	dma_cap_set(DMA_SLAVE, cdd->ddev.cap_mask);
	cdd->ddev.device_alloc_chan_resources = cppi41_dma_alloc_chan_resources;
	cdd->ddev.device_free_chan_resources = cppi41_dma_free_chan_resources;
	cdd->ddev.device_tx_status = cppi41_dma_tx_status;
	cdd->ddev.device_issue_pending = cppi41_dma_issue_pending;
	cdd->ddev.device_prep_slave_sg = cppi41_dma_prep_slave_sg;
	cdd->ddev.device_terminate_all = cppi41_stop_chan;
	cdd->ddev.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	cdd->ddev.src_addr_widths = CPPI41_DMA_BUSWIDTHS;
	cdd->ddev.dst_addr_widths = CPPI41_DMA_BUSWIDTHS;
	cdd->ddev.residue_granularity = DMA_RESIDUE_GRANULARITY_BURST;
	cdd->ddev.dev = dev;
	INIT_LIST_HEAD(&cdd->ddev.channels);
	cpp41_dma_info.dma_cap = cdd->ddev.cap_mask;

	cdd->usbss_mem = of_iomap(dev->of_node, 0);
	cdd->ctrl_mem = of_iomap(dev->of_node, 1);
	cdd->sched_mem = of_iomap(dev->of_node, 2);
	cdd->qmgr_mem = of_iomap(dev->of_node, 3);

	if (!cdd->usbss_mem || !cdd->ctrl_mem || !cdd->sched_mem ||
			!cdd->qmgr_mem)
		return -ENXIO;

	pm_runtime_enable(dev);
	ret = pm_runtime_get_sync(dev);
	if (ret < 0)
		goto err_get_sync;

	cdd->queues_rx = glue_info->queues_rx;
	cdd->queues_tx = glue_info->queues_tx;
	cdd->td_queue = glue_info->td_queue;

	ret = init_cppi41(dev, cdd);
	if (ret)
		goto err_init_cppi;

	ret = cppi41_add_chans(dev, cdd);
	if (ret)
		goto err_chans;

	irq = irq_of_parse_and_map(dev->of_node, 0);
	if (!irq) {
		ret = -EINVAL;
		goto err_irq;
	}

	cppi_writel(USBSS_IRQ_PD_COMP, cdd->usbss_mem + USBSS_IRQ_ENABLER);

	ret = devm_request_irq(&pdev->dev, irq, glue_info->isr, IRQF_SHARED,
			dev_name(dev), cdd);
	if (ret)
		goto err_irq;
	cdd->irq = irq;

	ret = dma_async_device_register(&cdd->ddev);
	if (ret)
		goto err_dma_reg;

	ret = of_dma_controller_register(dev->of_node,
			cppi41_dma_xlate, &cpp41_dma_info);
	if (ret)
		goto err_of;

	platform_set_drvdata(pdev, cdd);
	return 0;
err_of:
	dma_async_device_unregister(&cdd->ddev);
err_dma_reg:
err_irq:
	cppi_writel(0, cdd->usbss_mem + USBSS_IRQ_CLEARR);
	cleanup_chans(cdd);
err_chans:
	deinit_cppi41(dev, cdd);
err_init_cppi:
	pm_runtime_put(dev);
err_get_sync:
	pm_runtime_disable(dev);
	iounmap(cdd->usbss_mem);
	iounmap(cdd->ctrl_mem);
	iounmap(cdd->sched_mem);
	iounmap(cdd->qmgr_mem);
	return ret;
}

static int cppi41_dma_remove(struct platform_device *pdev)
{
	struct cppi41_dd *cdd = platform_get_drvdata(pdev);

	of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&cdd->ddev);

	cppi_writel(0, cdd->usbss_mem + USBSS_IRQ_CLEARR);
	devm_free_irq(&pdev->dev, cdd->irq, cdd);
	cleanup_chans(cdd);
	deinit_cppi41(&pdev->dev, cdd);
	iounmap(cdd->usbss_mem);
	iounmap(cdd->ctrl_mem);
	iounmap(cdd->sched_mem);
	iounmap(cdd->qmgr_mem);
	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cppi41_suspend(struct device *dev)
{
	struct cppi41_dd *cdd = dev_get_drvdata(dev);

	cdd->dma_tdfdq = cppi_readl(cdd->ctrl_mem + DMA_TDFDQ);
	cppi_writel(0, cdd->usbss_mem + USBSS_IRQ_CLEARR);
	disable_sched(cdd);

	return 0;
}

static int cppi41_resume(struct device *dev)
{
	struct cppi41_dd *cdd = dev_get_drvdata(dev);
	struct cppi41_channel *c;
	int i;

	for (i = 0; i < DESCS_AREAS; i++)
		cppi_writel(cdd->descs_phys, cdd->qmgr_mem + QMGR_MEMBASE(i));

	list_for_each_entry(c, &cdd->ddev.channels, chan.device_node)
		if (!c->is_tx)
			cppi_writel(c->q_num, c->gcr_reg + RXHPCRA0);

	init_sched(cdd);

	cppi_writel(cdd->dma_tdfdq, cdd->ctrl_mem + DMA_TDFDQ);
	cppi_writel(cdd->scratch_phys, cdd->qmgr_mem + QMGR_LRAM0_BASE);
	cppi_writel(QMGR_SCRATCH_SIZE, cdd->qmgr_mem + QMGR_LRAM_SIZE);
	cppi_writel(0, cdd->qmgr_mem + QMGR_LRAM1_BASE);

	cppi_writel(USBSS_IRQ_PD_COMP, cdd->usbss_mem + USBSS_IRQ_ENABLER);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(cppi41_pm_ops, cppi41_suspend, cppi41_resume);

static struct platform_driver cpp41_dma_driver = {
	.probe  = cppi41_dma_probe,
	.remove = cppi41_dma_remove,
	.driver = {
		.name = "cppi41-dma-engine",
		.pm = &cppi41_pm_ops,
		.of_match_table = of_match_ptr(cppi41_dma_ids),
	},
};

module_platform_driver(cpp41_dma_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sebastian Andrzej Siewior <bigeasy@linutronix.de>");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * Driver for the Analog Devices AXI-DMAC core
 *
 * Copyright 2013-2015 Analog Devices Inc.
 *  Author: Lars-Peter Clausen <lars@metafoo.de>
 *
 * Licensed under the GPL-2.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <dt-bindings/dma/axi-dmac.h>

#include "dmaengine.h"
#include "virt-dma.h"

/*
 * The AXI-DMAC is a soft IP core that is used in FPGA designs. The core has
 * various instantiation parameters which decided the exact feature set support
 * by the core.
 *
 * Each channel of the core has a source interface and a destination interface.
 * The number of channels and the type of the channel interfaces is selected at
 * configuration time. A interface can either be a connected to a central memory
 * interconnect, which allows access to system memory, or it can be connected to
 * a dedicated bus which is directly connected to a data port on a peripheral.
 * Given that those are configuration options of the core that are selected when
 * it is instantiated this means that they can not be changed by software at
 * runtime. By extension this means that each channel is uni-directional. It can
 * either be device to memory or memory to device, but not both. Also since the
 * device side is a dedicated data bus only connected to a single peripheral
 * there is no address than can or needs to be configured for the device side.
 */

#define AXI_DMAC_REG_IRQ_MASK		0x80
#define AXI_DMAC_REG_IRQ_PENDING	0x84
#define AXI_DMAC_REG_IRQ_SOURCE		0x88

#define AXI_DMAC_REG_CTRL		0x400
#define AXI_DMAC_REG_TRANSFER_ID	0x404
#define AXI_DMAC_REG_START_TRANSFER	0x408
#define AXI_DMAC_REG_FLAGS		0x40c
#define AXI_DMAC_REG_DEST_ADDRESS	0x410
#define AXI_DMAC_REG_SRC_ADDRESS	0x414
#define AXI_DMAC_REG_X_LENGTH		0x418
#define AXI_DMAC_REG_Y_LENGTH		0x41c
#define AXI_DMAC_REG_DEST_STRIDE	0x420
#define AXI_DMAC_REG_SRC_STRIDE		0x424
#define AXI_DMAC_REG_TRANSFER_DONE	0x428
#define AXI_DMAC_REG_ACTIVE_TRANSFER_ID 0x42c
#define AXI_DMAC_REG_STATUS		0x430
#define AXI_DMAC_REG_CURRENT_SRC_ADDR	0x434
#define AXI_DMAC_REG_CURRENT_DEST_ADDR	0x438

#define AXI_DMAC_CTRL_ENABLE		BIT(0)
#define AXI_DMAC_CTRL_PAUSE		BIT(1)

#define AXI_DMAC_IRQ_SOT		BIT(0)
#define AXI_DMAC_IRQ_EOT		BIT(1)

#define AXI_DMAC_FLAG_CYCLIC		BIT(0)

struct axi_dmac_sg {
	dma_addr_t src_addr;
	dma_addr_t dest_addr;
	unsigned int x_len;
	unsigned int y_len;
	unsigned int dest_stride;
	unsigned int src_stride;
	unsigned int id;
};

struct axi_dmac_desc {
	struct virt_dma_desc vdesc;
	bool cyclic;

	unsigned int num_submitted;
	unsigned int num_completed;
	unsigned int num_sgs;
	struct axi_dmac_sg sg[];
};

struct axi_dmac_chan {
	struct virt_dma_chan vchan;

	struct axi_dmac_desc *next_desc;
	struct list_head active_descs;
	enum dma_transfer_direction direction;

	unsigned int src_width;
	unsigned int dest_width;
	unsigned int src_type;
	unsigned int dest_type;

	unsigned int max_length;
	unsigned int align_mask;

	bool hw_cyclic;
	bool hw_2d;
};

struct axi_dmac {
	void __iomem *base;
	int irq;

	struct clk *clk;

	struct dma_device dma_dev;
	struct axi_dmac_chan chan;

	struct device_dma_parameters dma_parms;
};

static struct axi_dmac *chan_to_axi_dmac(struct axi_dmac_chan *chan)
{
	return container_of(chan->vchan.chan.device, struct axi_dmac,
		dma_dev);
}

static struct axi_dmac_chan *to_axi_dmac_chan(struct dma_chan *c)
{
	return container_of(c, struct axi_dmac_chan, vchan.chan);
}

static struct axi_dmac_desc *to_axi_dmac_desc(struct virt_dma_desc *vdesc)
{
	return container_of(vdesc, struct axi_dmac_desc, vdesc);
}

static void axi_dmac_write(struct axi_dmac *axi_dmac, unsigned int reg,
	unsigned int val)
{
	writel(val, axi_dmac->base + reg);
}

static int axi_dmac_read(struct axi_dmac *axi_dmac, unsigned int reg)
{
	return readl(axi_dmac->base + reg);
}

static int axi_dmac_src_is_mem(struct axi_dmac_chan *chan)
{
	return chan->src_type == AXI_DMAC_BUS_TYPE_AXI_MM;
}

static int axi_dmac_dest_is_mem(struct axi_dmac_chan *chan)
{
	return chan->dest_type == AXI_DMAC_BUS_TYPE_AXI_MM;
}

static bool axi_dmac_check_len(struct axi_dmac_chan *chan, unsigned int len)
{
	if (len == 0 || len > chan->max_length)
		return false;
	if ((len & chan->align_mask) != 0) /* Not aligned */
		return false;
	return true;
}

static bool axi_dmac_check_addr(struct axi_dmac_chan *chan, dma_addr_t addr)
{
	if ((addr & chan->align_mask) != 0) /* Not aligned */
		return false;
	return true;
}

static void axi_dmac_start_transfer(struct axi_dmac_chan *chan)
{
	struct axi_dmac *dmac = chan_to_axi_dmac(chan);
	struct virt_dma_desc *vdesc;
	struct axi_dmac_desc *desc;
	struct axi_dmac_sg *sg;
	unsigned int flags = 0;
	unsigned int val;

	val = axi_dmac_read(dmac, AXI_DMAC_REG_START_TRANSFER);
	if (val) /* Queue is full, wait for the next SOT IRQ */
		return;

	desc = chan->next_desc;

	if (!desc) {
		vdesc = vchan_next_desc(&chan->vchan);
		if (!vdesc)
			return;
		list_move_tail(&vdesc->node, &chan->active_descs);
		desc = to_axi_dmac_desc(vdesc);
	}
	sg = &desc->sg[desc->num_submitted];

	desc->num_submitted++;
	if (desc->num_submitted == desc->num_sgs)
		chan->next_desc = NULL;
	else
		chan->next_desc = desc;

	sg->id = axi_dmac_read(dmac, AXI_DMAC_REG_TRANSFER_ID);

	if (axi_dmac_dest_is_mem(chan)) {
		axi_dmac_write(dmac, AXI_DMAC_REG_DEST_ADDRESS, sg->dest_addr);
		axi_dmac_write(dmac, AXI_DMAC_REG_DEST_STRIDE, sg->dest_stride);
	}

	if (axi_dmac_src_is_mem(chan)) {
		axi_dmac_write(dmac, AXI_DMAC_REG_SRC_ADDRESS, sg->src_addr);
		axi_dmac_write(dmac, AXI_DMAC_REG_SRC_STRIDE, sg->src_stride);
	}

	/*
	 * If the hardware supports cyclic transfers and there is no callback to
	 * call, enable hw cyclic mode to avoid unnecessary interrupts.
	 */
	if (chan->hw_cyclic && desc->cyclic && !desc->vdesc.tx.callback)
		flags |= AXI_DMAC_FLAG_CYCLIC;

	axi_dmac_write(dmac, AXI_DMAC_REG_X_LENGTH, sg->x_len - 1);
	axi_dmac_write(dmac, AXI_DMAC_REG_Y_LENGTH, sg->y_len - 1);
	axi_dmac_write(dmac, AXI_DMAC_REG_FLAGS, flags);
	axi_dmac_write(dmac, AXI_DMAC_REG_START_TRANSFER, 1);
}

static struct axi_dmac_desc *axi_dmac_active_desc(struct axi_dmac_chan *chan)
{
	return list_first_entry_or_null(&chan->active_descs,
		struct axi_dmac_desc, vdesc.node);
}

static void axi_dmac_transfer_done(struct axi_dmac_chan *chan,
	unsigned int completed_transfers)
{
	struct axi_dmac_desc *active;
	struct axi_dmac_sg *sg;

	active = axi_dmac_active_desc(chan);
	if (!active)
		return;

	if (active->cyclic) {
		vchan_cyclic_callback(&active->vdesc);
	} else {
		do {
			sg = &active->sg[active->num_completed];
			if (!(BIT(sg->id) & completed_transfers))
				break;
			active->num_completed++;
			if (active->num_completed == active->num_sgs) {
				list_del(&active->vdesc.node);
				vchan_cookie_complete(&active->vdesc);
				active = axi_dmac_active_desc(chan);
			}
		} while (active);
	}
}

static irqreturn_t axi_dmac_interrupt_handler(int irq, void *devid)
{
	struct axi_dmac *dmac = devid;
	unsigned int pending;

	pending = axi_dmac_read(dmac, AXI_DMAC_REG_IRQ_PENDING);
	axi_dmac_write(dmac, AXI_DMAC_REG_IRQ_PENDING, pending);

	spin_lock(&dmac->chan.vchan.lock);
	/* One or more transfers have finished */
	if (pending & AXI_DMAC_IRQ_EOT) {
		unsigned int completed;

		completed = axi_dmac_read(dmac, AXI_DMAC_REG_TRANSFER_DONE);
		axi_dmac_transfer_done(&dmac->chan, completed);
	}
	/* Space has become available in the descriptor queue */
	if (pending & AXI_DMAC_IRQ_SOT)
		axi_dmac_start_transfer(&dmac->chan);
	spin_unlock(&dmac->chan.vchan.lock);

	return IRQ_HANDLED;
}

static int axi_dmac_terminate_all(struct dma_chan *c)
{
	struct axi_dmac_chan *chan = to_axi_dmac_chan(c);
	struct axi_dmac *dmac = chan_to_axi_dmac(chan);
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&chan->vchan.lock, flags);
	axi_dmac_write(dmac, AXI_DMAC_REG_CTRL, 0);
	chan->next_desc = NULL;
	vchan_get_all_descriptors(&chan->vchan, &head);
	list_splice_tail_init(&chan->active_descs, &head);
	spin_unlock_irqrestore(&chan->vchan.lock, flags);

	vchan_dma_desc_free_list(&chan->vchan, &head);

	return 0;
}

static void axi_dmac_issue_pending(struct dma_chan *c)
{
	struct axi_dmac_chan *chan = to_axi_dmac_chan(c);
	struct axi_dmac *dmac = chan_to_axi_dmac(chan);
	unsigned long flags;

	axi_dmac_write(dmac, AXI_DMAC_REG_CTRL, AXI_DMAC_CTRL_ENABLE);

	spin_lock_irqsave(&chan->vchan.lock, flags);
	if (vchan_issue_pending(&chan->vchan))
		axi_dmac_start_transfer(chan);
	spin_unlock_irqrestore(&chan->vchan.lock, flags);
}

static struct axi_dmac_desc *axi_dmac_alloc_desc(unsigned int num_sgs)
{
	struct axi_dmac_desc *desc;

	desc = kzalloc(sizeof(struct axi_dmac_desc) +
		sizeof(struct axi_dmac_sg) * num_sgs, GFP_NOWAIT);
	if (!desc)
		return NULL;

	desc->num_sgs = num_sgs;

	return desc;
}

static struct dma_async_tx_descriptor *axi_dmac_prep_slave_sg(
	struct dma_chan *c, struct scatterlist *sgl,
	unsigned int sg_len, enum dma_transfer_direction direction,
	unsigned long flags, void *context)
{
	struct axi_dmac_chan *chan = to_axi_dmac_chan(c);
	struct axi_dmac_desc *desc;
	struct scatterlist *sg;
	unsigned int i;

	if (direction != chan->direction)
		return NULL;

	desc = axi_dmac_alloc_desc(sg_len);
	if (!desc)
		return NULL;

	for_each_sg(sgl, sg, sg_len, i) {
		if (!axi_dmac_check_addr(chan, sg_dma_address(sg)) ||
		    !axi_dmac_check_len(chan, sg_dma_len(sg))) {
			kfree(desc);
			return NULL;
		}

		if (direction == DMA_DEV_TO_MEM)
			desc->sg[i].dest_addr = sg_dma_address(sg);
		else
			desc->sg[i].src_addr = sg_dma_address(sg);
		desc->sg[i].x_len = sg_dma_len(sg);
		desc->sg[i].y_len = 1;
	}

	desc->cyclic = false;

	return vchan_tx_prep(&chan->vchan, &desc->vdesc, flags);
}

static struct dma_async_tx_descriptor *axi_dmac_prep_dma_cyclic(
	struct dma_chan *c, dma_addr_t buf_addr, size_t buf_len,
	size_t period_len, enum dma_transfer_direction direction,
	unsigned long flags)
{
	struct axi_dmac_chan *chan = to_axi_dmac_chan(c);
	struct axi_dmac_desc *desc;
	unsigned int num_periods, i;

	if (direction != chan->direction)
		return NULL;

	if (!axi_dmac_check_len(chan, buf_len) ||
	    !axi_dmac_check_addr(chan, buf_addr))
		return NULL;

	if (period_len == 0 || buf_len % period_len)
		return NULL;

	num_periods = buf_len / period_len;

	desc = axi_dmac_alloc_desc(num_periods);
	if (!desc)
		return NULL;

	for (i = 0; i < num_periods; i++) {
		if (direction == DMA_DEV_TO_MEM)
			desc->sg[i].dest_addr = buf_addr;
		else
			desc->sg[i].src_addr = buf_addr;
		desc->sg[i].x_len = period_len;
		desc->sg[i].y_len = 1;
		buf_addr += period_len;
	}

	desc->cyclic = true;

	return vchan_tx_prep(&chan->vchan, &desc->vdesc, flags);
}

static struct dma_async_tx_descriptor *axi_dmac_prep_interleaved(
	struct dma_chan *c, struct dma_interleaved_template *xt,
	unsigned long flags)
{
	struct axi_dmac_chan *chan = to_axi_dmac_chan(c);
	struct axi_dmac_desc *desc;
	size_t dst_icg, src_icg;

	if (xt->frame_size != 1)
		return NULL;

	if (xt->dir != chan->direction)
		return NULL;

	if (axi_dmac_src_is_mem(chan)) {
		if (!xt->src_inc || !axi_dmac_check_addr(chan, xt->src_start))
			return NULL;
	}

	if (axi_dmac_dest_is_mem(chan)) {
		if (!xt->dst_inc || !axi_dmac_check_addr(chan, xt->dst_start))
			return NULL;
	}

	dst_icg = dmaengine_get_dst_icg(xt, &xt->sgl[0]);
	src_icg = dmaengine_get_src_icg(xt, &xt->sgl[0]);

	if (chan->hw_2d) {
		if (!axi_dmac_check_len(chan, xt->sgl[0].size) ||
		    xt->numf == 0)
			return NULL;
		if (xt->sgl[0].size + dst_icg > chan->max_length ||
		    xt->sgl[0].size + src_icg > chan->max_length)
			return NULL;
	} else {
		if (dst_icg != 0 || src_icg != 0)
			return NULL;
		if (chan->max_length / xt->sgl[0].size < xt->numf)
			return NULL;
		if (!axi_dmac_check_len(chan, xt->sgl[0].size * xt->numf))
			return NULL;
	}

	desc = axi_dmac_alloc_desc(1);
	if (!desc)
		return NULL;

	if (axi_dmac_src_is_mem(chan)) {
		desc->sg[0].src_addr = xt->src_start;
		desc->sg[0].src_stride = xt->sgl[0].size + src_icg;
	}

	if (axi_dmac_dest_is_mem(chan)) {
		desc->sg[0].dest_addr = xt->dst_start;
		desc->sg[0].dest_stride = xt->sgl[0].size + dst_icg;
	}

	if (chan->hw_2d) {
		desc->sg[0].x_len = xt->sgl[0].size;
		desc->sg[0].y_len = xt->numf;
	} else {
		desc->sg[0].x_len = xt->sgl[0].size * xt->numf;
		desc->sg[0].y_len = 1;
	}

	return vchan_tx_prep(&chan->vchan, &desc->vdesc, flags);
}

static void axi_dmac_free_chan_resources(struct dma_chan *c)
{
	vchan_free_chan_resources(to_virt_chan(c));
}

static void axi_dmac_desc_free(struct virt_dma_desc *vdesc)
{
	kfree(container_of(vdesc, struct axi_dmac_desc, vdesc));
}

/*
 * The configuration stored in the devicetree matches the configuration
 * parameters of the peripheral instance and allows the driver to know which
 * features are implemented and how it should behave.
 */
static int axi_dmac_parse_chan_dt(struct device_node *of_chan,
	struct axi_dmac_chan *chan)
{
	u32 val;
	int ret;

	ret = of_property_read_u32(of_chan, "reg", &val);
	if (ret)
		return ret;

	/* We only support 1 channel for now */
	if (val != 0)
		return -EINVAL;

	ret = of_property_read_u32(of_chan, "adi,source-bus-type", &val);
	if (ret)
		return ret;
	if (val > AXI_DMAC_BUS_TYPE_FIFO)
		return -EINVAL;
	chan->src_type = val;

	ret = of_property_read_u32(of_chan, "adi,destination-bus-type", &val);
	if (ret)
		return ret;
	if (val > AXI_DMAC_BUS_TYPE_FIFO)
		return -EINVAL;
	chan->dest_type = val;

	ret = of_property_read_u32(of_chan, "adi,source-bus-width", &val);
	if (ret)
		return ret;
	chan->src_width = val / 8;

	ret = of_property_read_u32(of_chan, "adi,destination-bus-width", &val);
	if (ret)
		return ret;
	chan->dest_width = val / 8;

	ret = of_property_read_u32(of_chan, "adi,length-width", &val);
	if (ret)
		return ret;

	if (val >= 32)
		chan->max_length = UINT_MAX;
	else
		chan->max_length = (1ULL << val) - 1;

	chan->align_mask = max(chan->dest_width, chan->src_width) - 1;

	if (axi_dmac_dest_is_mem(chan) && axi_dmac_src_is_mem(chan))
		chan->direction = DMA_MEM_TO_MEM;
	else if (!axi_dmac_dest_is_mem(chan) && axi_dmac_src_is_mem(chan))
		chan->direction = DMA_MEM_TO_DEV;
	else if (axi_dmac_dest_is_mem(chan) && !axi_dmac_src_is_mem(chan))
		chan->direction = DMA_DEV_TO_MEM;
	else
		chan->direction = DMA_DEV_TO_DEV;

	chan->hw_cyclic = of_property_read_bool(of_chan, "adi,cyclic");
	chan->hw_2d = of_property_read_bool(of_chan, "adi,2d");

	return 0;
}

static int axi_dmac_probe(struct platform_device *pdev)
{
	struct device_node *of_channels, *of_chan;
	struct dma_device *dma_dev;
	struct axi_dmac *dmac;
	struct resource *res;
	int ret;

	dmac = devm_kzalloc(&pdev->dev, sizeof(*dmac), GFP_KERNEL);
	if (!dmac)
		return -ENOMEM;

	dmac->irq = platform_get_irq(pdev, 0);
	if (dmac->irq <= 0)
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dmac->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dmac->base))
		return PTR_ERR(dmac->base);

	dmac->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(dmac->clk))
		return PTR_ERR(dmac->clk);

	INIT_LIST_HEAD(&dmac->chan.active_descs);

	of_channels = of_get_child_by_name(pdev->dev.of_node, "adi,channels");
	if (of_channels == NULL)
		return -ENODEV;

	for_each_child_of_node(of_channels, of_chan) {
		ret = axi_dmac_parse_chan_dt(of_chan, &dmac->chan);
		if (ret) {
			of_node_put(of_chan);
			of_node_put(of_channels);
			return -EINVAL;
		}
	}
	of_node_put(of_channels);

	pdev->dev.dma_parms = &dmac->dma_parms;
	dma_set_max_seg_size(&pdev->dev, dmac->chan.max_length);

	dma_dev = &dmac->dma_dev;
	dma_cap_set(DMA_SLAVE, dma_dev->cap_mask);
	dma_cap_set(DMA_CYCLIC, dma_dev->cap_mask);
	dma_dev->device_free_chan_resources = axi_dmac_free_chan_resources;
	dma_dev->device_tx_status = dma_cookie_status;
	dma_dev->device_issue_pending = axi_dmac_issue_pending;
	dma_dev->device_prep_slave_sg = axi_dmac_prep_slave_sg;
	dma_dev->device_prep_dma_cyclic = axi_dmac_prep_dma_cyclic;
	dma_dev->device_prep_interleaved_dma = axi_dmac_prep_interleaved;
	dma_dev->device_terminate_all = axi_dmac_terminate_all;
	dma_dev->dev = &pdev->dev;
	dma_dev->chancnt = 1;
	dma_dev->src_addr_widths = BIT(dmac->chan.src_width);
	dma_dev->dst_addr_widths = BIT(dmac->chan.dest_width);
	dma_dev->directions = BIT(dmac->chan.direction);
	dma_dev->residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;
	INIT_LIST_HEAD(&dma_dev->channels);

	dmac->chan.vchan.desc_free = axi_dmac_desc_free;
	vchan_init(&dmac->chan.vchan, dma_dev);

	ret = clk_prepare_enable(dmac->clk);
	if (ret < 0)
		return ret;

	axi_dmac_write(dmac, AXI_DMAC_REG_IRQ_MASK, 0x00);

	ret = dma_async_device_register(dma_dev);
	if (ret)
		goto err_clk_disable;

	ret = of_dma_controller_register(pdev->dev.of_node,
		of_dma_xlate_by_chan_id, dma_dev);
	if (ret)
		goto err_unregister_device;

	ret = request_irq(dmac->irq, axi_dmac_interrupt_handler, 0,
		dev_name(&pdev->dev), dmac);
	if (ret)
		goto err_unregister_of;

	platform_set_drvdata(pdev, dmac);

	return 0;

err_unregister_of:
	of_dma_controller_free(pdev->dev.of_node);
err_unregister_device:
	dma_async_device_unregister(&dmac->dma_dev);
err_clk_disable:
	clk_disable_unprepare(dmac->clk);

	return ret;
}

static int axi_dmac_remove(struct platform_device *pdev)
{
	struct axi_dmac *dmac = platform_get_drvdata(pdev);

	of_dma_controller_free(pdev->dev.of_node);
	free_irq(dmac->irq, dmac);
	tasklet_kill(&dmac->chan.vchan.task);
	dma_async_device_unregister(&dmac->dma_dev);
	clk_disable_unprepare(dmac->clk);

	return 0;
}

static const struct of_device_id axi_dmac_of_match_table[] = {
	{ .compatible = "adi,axi-dmac-1.00.a" },
	{ },
};

static struct platform_driver axi_dmac_driver = {
	.driver = {
		.name = "dma-axi-dmac",
		.of_match_table = axi_dmac_of_match_table,
	},
	.probe = axi_dmac_probe,
	.remove = axi_dmac_remove,
};
module_platform_driver(axi_dmac_driver);

MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_DESCRIPTION("DMA controller driver for the AXI-DMAC controller");
MODULE_LICENSE("GPL v2");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 *  Copyright (C) 2013, Lars-Peter Clausen <lars@metafoo.de>
 *  JZ4740 DMAC support
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General	 Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/clk.h>

#include <asm/mach-jz4740/dma.h>

#include "virt-dma.h"

#define JZ_DMA_NR_CHANS 6

#define JZ_REG_DMA_SRC_ADDR(x)		(0x00 + (x) * 0x20)
#define JZ_REG_DMA_DST_ADDR(x)		(0x04 + (x) * 0x20)
#define JZ_REG_DMA_TRANSFER_COUNT(x)	(0x08 + (x) * 0x20)
#define JZ_REG_DMA_REQ_TYPE(x)		(0x0C + (x) * 0x20)
#define JZ_REG_DMA_STATUS_CTRL(x)	(0x10 + (x) * 0x20)
#define JZ_REG_DMA_CMD(x)		(0x14 + (x) * 0x20)
#define JZ_REG_DMA_DESC_ADDR(x)		(0x18 + (x) * 0x20)

#define JZ_REG_DMA_CTRL			0x300
#define JZ_REG_DMA_IRQ			0x304
#define JZ_REG_DMA_DOORBELL		0x308
#define JZ_REG_DMA_DOORBELL_SET		0x30C

#define JZ_DMA_STATUS_CTRL_NO_DESC		BIT(31)
#define JZ_DMA_STATUS_CTRL_DESC_INV		BIT(6)
#define JZ_DMA_STATUS_CTRL_ADDR_ERR		BIT(4)
#define JZ_DMA_STATUS_CTRL_TRANSFER_DONE	BIT(3)
#define JZ_DMA_STATUS_CTRL_HALT			BIT(2)
#define JZ_DMA_STATUS_CTRL_COUNT_TERMINATE	BIT(1)
#define JZ_DMA_STATUS_CTRL_ENABLE		BIT(0)

#define JZ_DMA_CMD_SRC_INC			BIT(23)
#define JZ_DMA_CMD_DST_INC			BIT(22)
#define JZ_DMA_CMD_RDIL_MASK			(0xf << 16)
#define JZ_DMA_CMD_SRC_WIDTH_MASK		(0x3 << 14)
#define JZ_DMA_CMD_DST_WIDTH_MASK		(0x3 << 12)
#define JZ_DMA_CMD_INTERVAL_LENGTH_MASK		(0x7 << 8)
#define JZ_DMA_CMD_BLOCK_MODE			BIT(7)
#define JZ_DMA_CMD_DESC_VALID			BIT(4)
#define JZ_DMA_CMD_DESC_VALID_MODE		BIT(3)
#define JZ_DMA_CMD_VALID_IRQ_ENABLE		BIT(2)
#define JZ_DMA_CMD_TRANSFER_IRQ_ENABLE		BIT(1)
#define JZ_DMA_CMD_LINK_ENABLE			BIT(0)

#define JZ_DMA_CMD_FLAGS_OFFSET 22
#define JZ_DMA_CMD_RDIL_OFFSET 16
#define JZ_DMA_CMD_SRC_WIDTH_OFFSET 14
#define JZ_DMA_CMD_DST_WIDTH_OFFSET 12
#define JZ_DMA_CMD_TRANSFER_SIZE_OFFSET 8
#define JZ_DMA_CMD_MODE_OFFSET 7

#define JZ_DMA_CTRL_PRIORITY_MASK		(0x3 << 8)
#define JZ_DMA_CTRL_HALT			BIT(3)
#define JZ_DMA_CTRL_ADDRESS_ERROR		BIT(2)
#define JZ_DMA_CTRL_ENABLE			BIT(0)

enum jz4740_dma_width {
	JZ4740_DMA_WIDTH_32BIT	= 0,
	JZ4740_DMA_WIDTH_8BIT	= 1,
	JZ4740_DMA_WIDTH_16BIT	= 2,
};

enum jz4740_dma_transfer_size {
	JZ4740_DMA_TRANSFER_SIZE_4BYTE	= 0,
	JZ4740_DMA_TRANSFER_SIZE_1BYTE	= 1,
	JZ4740_DMA_TRANSFER_SIZE_2BYTE	= 2,
	JZ4740_DMA_TRANSFER_SIZE_16BYTE = 3,
	JZ4740_DMA_TRANSFER_SIZE_32BYTE = 4,
};

enum jz4740_dma_flags {
	JZ4740_DMA_SRC_AUTOINC = 0x2,
	JZ4740_DMA_DST_AUTOINC = 0x1,
};

enum jz4740_dma_mode {
	JZ4740_DMA_MODE_SINGLE	= 0,
	JZ4740_DMA_MODE_BLOCK	= 1,
};

struct jz4740_dma_sg {
	dma_addr_t addr;
	unsigned int len;
};

struct jz4740_dma_desc {
	struct virt_dma_desc vdesc;

	enum dma_transfer_direction direction;
	bool cyclic;

	unsigned int num_sgs;
	struct jz4740_dma_sg sg[];
};

struct jz4740_dmaengine_chan {
	struct virt_dma_chan vchan;
	unsigned int id;

	dma_addr_t fifo_addr;
	unsigned int transfer_shift;

	struct jz4740_dma_desc *desc;
	unsigned int next_sg;
};

struct jz4740_dma_dev {
	struct dma_device ddev;
	void __iomem *base;
	struct clk *clk;

	struct jz4740_dmaengine_chan chan[JZ_DMA_NR_CHANS];
};

static struct jz4740_dma_dev *jz4740_dma_chan_get_dev(
	struct jz4740_dmaengine_chan *chan)
{
	return container_of(chan->vchan.chan.device, struct jz4740_dma_dev,
		ddev);
}

static struct jz4740_dmaengine_chan *to_jz4740_dma_chan(struct dma_chan *c)
{
	return container_of(c, struct jz4740_dmaengine_chan, vchan.chan);
}

static struct jz4740_dma_desc *to_jz4740_dma_desc(struct virt_dma_desc *vdesc)
{
	return container_of(vdesc, struct jz4740_dma_desc, vdesc);
}

static inline uint32_t jz4740_dma_read(struct jz4740_dma_dev *dmadev,
	unsigned int reg)
{
	return readl(dmadev->base + reg);
}

static inline void jz4740_dma_write(struct jz4740_dma_dev *dmadev,
	unsigned reg, uint32_t val)
{
	writel(val, dmadev->base + reg);
}

static inline void jz4740_dma_write_mask(struct jz4740_dma_dev *dmadev,
	unsigned int reg, uint32_t val, uint32_t mask)
{
	uint32_t tmp;

	tmp = jz4740_dma_read(dmadev, reg);
	tmp &= ~mask;
	tmp |= val;
	jz4740_dma_write(dmadev, reg, tmp);
}

static struct jz4740_dma_desc *jz4740_dma_alloc_desc(unsigned int num_sgs)
{
	return kzalloc(sizeof(struct jz4740_dma_desc) +
		sizeof(struct jz4740_dma_sg) * num_sgs, GFP_ATOMIC);
}

static enum jz4740_dma_width jz4740_dma_width(enum dma_slave_buswidth width)
{
	switch (width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		return JZ4740_DMA_WIDTH_8BIT;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		return JZ4740_DMA_WIDTH_16BIT;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		return JZ4740_DMA_WIDTH_32BIT;
	default:
		return JZ4740_DMA_WIDTH_32BIT;
	}
}

static enum jz4740_dma_transfer_size jz4740_dma_maxburst(u32 maxburst)
{
	if (maxburst <= 1)
		return JZ4740_DMA_TRANSFER_SIZE_1BYTE;
	else if (maxburst <= 3)
		return JZ4740_DMA_TRANSFER_SIZE_2BYTE;
	else if (maxburst <= 15)
		return JZ4740_DMA_TRANSFER_SIZE_4BYTE;
	else if (maxburst <= 31)
		return JZ4740_DMA_TRANSFER_SIZE_16BYTE;

	return JZ4740_DMA_TRANSFER_SIZE_32BYTE;
}

static int jz4740_dma_slave_config(struct dma_chan *c,
				   struct dma_slave_config *config)
{
	struct jz4740_dmaengine_chan *chan = to_jz4740_dma_chan(c);
	struct jz4740_dma_dev *dmadev = jz4740_dma_chan_get_dev(chan);
	enum jz4740_dma_width src_width;
	enum jz4740_dma_width dst_width;
	enum jz4740_dma_transfer_size transfer_size;
	enum jz4740_dma_flags flags;
	uint32_t cmd;

	switch (config->direction) {
	case DMA_MEM_TO_DEV:
		flags = JZ4740_DMA_SRC_AUTOINC;
		transfer_size = jz4740_dma_maxburst(config->dst_maxburst);
		chan->fifo_addr = config->dst_addr;
		break;
	case DMA_DEV_TO_MEM:
		flags = JZ4740_DMA_DST_AUTOINC;
		transfer_size = jz4740_dma_maxburst(config->src_maxburst);
		chan->fifo_addr = config->src_addr;
		break;
	default:
		return -EINVAL;
	}

	src_width = jz4740_dma_width(config->src_addr_width);
	dst_width = jz4740_dma_width(config->dst_addr_width);

	switch (transfer_size) {
	case JZ4740_DMA_TRANSFER_SIZE_2BYTE:
		chan->transfer_shift = 1;
		break;
	case JZ4740_DMA_TRANSFER_SIZE_4BYTE:
		chan->transfer_shift = 2;
		break;
	case JZ4740_DMA_TRANSFER_SIZE_16BYTE:
		chan->transfer_shift = 4;
		break;
	case JZ4740_DMA_TRANSFER_SIZE_32BYTE:
		chan->transfer_shift = 5;
		break;
	default:
		chan->transfer_shift = 0;
		break;
	}

	cmd = flags << JZ_DMA_CMD_FLAGS_OFFSET;
	cmd |= src_width << JZ_DMA_CMD_SRC_WIDTH_OFFSET;
	cmd |= dst_width << JZ_DMA_CMD_DST_WIDTH_OFFSET;
	cmd |= transfer_size << JZ_DMA_CMD_TRANSFER_SIZE_OFFSET;
	cmd |= JZ4740_DMA_MODE_SINGLE << JZ_DMA_CMD_MODE_OFFSET;
	cmd |= JZ_DMA_CMD_TRANSFER_IRQ_ENABLE;

	jz4740_dma_write(dmadev, JZ_REG_DMA_CMD(chan->id), cmd);
	jz4740_dma_write(dmadev, JZ_REG_DMA_STATUS_CTRL(chan->id), 0);
	jz4740_dma_write(dmadev, JZ_REG_DMA_REQ_TYPE(chan->id),
		config->slave_id);

	return 0;
}

static int jz4740_dma_terminate_all(struct dma_chan *c)
{
	struct jz4740_dmaengine_chan *chan = to_jz4740_dma_chan(c);
	struct jz4740_dma_dev *dmadev = jz4740_dma_chan_get_dev(chan);
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&chan->vchan.lock, flags);
	jz4740_dma_write_mask(dmadev, JZ_REG_DMA_STATUS_CTRL(chan->id), 0,
			JZ_DMA_STATUS_CTRL_ENABLE);
	chan->desc = NULL;
	vchan_get_all_descriptors(&chan->vchan, &head);
	spin_unlock_irqrestore(&chan->vchan.lock, flags);

	vchan_dma_desc_free_list(&chan->vchan, &head);

	return 0;
}

static int jz4740_dma_start_transfer(struct jz4740_dmaengine_chan *chan)
{
	struct jz4740_dma_dev *dmadev = jz4740_dma_chan_get_dev(chan);
	dma_addr_t src_addr, dst_addr;
	struct virt_dma_desc *vdesc;
	struct jz4740_dma_sg *sg;

	jz4740_dma_write_mask(dmadev, JZ_REG_DMA_STATUS_CTRL(chan->id), 0,
			JZ_DMA_STATUS_CTRL_ENABLE);

	if (!chan->desc) {
		vdesc = vchan_next_desc(&chan->vchan);
		if (!vdesc)
			return 0;
		chan->desc = to_jz4740_dma_desc(vdesc);
		chan->next_sg = 0;
	}

	if (chan->next_sg == chan->desc->num_sgs)
		chan->next_sg = 0;

	sg = &chan->desc->sg[chan->next_sg];

	if (chan->desc->direction == DMA_MEM_TO_DEV) {
		src_addr = sg->addr;
		dst_addr = chan->fifo_addr;
	} else {
		src_addr = chan->fifo_addr;
		dst_addr = sg->addr;
	}
	jz4740_dma_write(dmadev, JZ_REG_DMA_SRC_ADDR(chan->id), src_addr);
	jz4740_dma_write(dmadev, JZ_REG_DMA_DST_ADDR(chan->id), dst_addr);
	jz4740_dma_write(dmadev, JZ_REG_DMA_TRANSFER_COUNT(chan->id),
			sg->len >> chan->transfer_shift);

	chan->next_sg++;

	jz4740_dma_write_mask(dmadev, JZ_REG_DMA_STATUS_CTRL(chan->id),
			JZ_DMA_STATUS_CTRL_NO_DESC | JZ_DMA_STATUS_CTRL_ENABLE,
			JZ_DMA_STATUS_CTRL_HALT | JZ_DMA_STATUS_CTRL_NO_DESC |
			JZ_DMA_STATUS_CTRL_ENABLE);

	jz4740_dma_write_mask(dmadev, JZ_REG_DMA_CTRL,
			JZ_DMA_CTRL_ENABLE,
			JZ_DMA_CTRL_HALT | JZ_DMA_CTRL_ENABLE);

	return 0;
}

static void jz4740_dma_chan_irq(struct jz4740_dmaengine_chan *chan)
{
	spin_lock(&chan->vchan.lock);
	if (chan->desc) {
		if (chan->desc->cyclic) {
			vchan_cyclic_callback(&chan->desc->vdesc);
		} else {
			if (chan->next_sg == chan->desc->num_sgs) {
				list_del(&chan->desc->vdesc.node);
				vchan_cookie_complete(&chan->desc->vdesc);
				chan->desc = NULL;
			}
		}
	}
	jz4740_dma_start_transfer(chan);
	spin_unlock(&chan->vchan.lock);
}

static irqreturn_t jz4740_dma_irq(int irq, void *devid)
{
	struct jz4740_dma_dev *dmadev = devid;
	uint32_t irq_status;
	unsigned int i;

	irq_status = readl(dmadev->base + JZ_REG_DMA_IRQ);

	for (i = 0; i < 6; ++i) {
		if (irq_status & (1 << i)) {
			jz4740_dma_write_mask(dmadev,
				JZ_REG_DMA_STATUS_CTRL(i), 0,
				JZ_DMA_STATUS_CTRL_ENABLE |
				JZ_DMA_STATUS_CTRL_TRANSFER_DONE);

			jz4740_dma_chan_irq(&dmadev->chan[i]);
		}
	}

	return IRQ_HANDLED;
}

static void jz4740_dma_issue_pending(struct dma_chan *c)
{
	struct jz4740_dmaengine_chan *chan = to_jz4740_dma_chan(c);
	unsigned long flags;

	spin_lock_irqsave(&chan->vchan.lock, flags);
	if (vchan_issue_pending(&chan->vchan) && !chan->desc)
		jz4740_dma_start_transfer(chan);
	spin_unlock_irqrestore(&chan->vchan.lock, flags);
}

static struct dma_async_tx_descriptor *jz4740_dma_prep_slave_sg(
	struct dma_chan *c, struct scatterlist *sgl,
	unsigned int sg_len, enum dma_transfer_direction direction,
	unsigned long flags, void *context)
{
	struct jz4740_dmaengine_chan *chan = to_jz4740_dma_chan(c);
	struct jz4740_dma_desc *desc;
	struct scatterlist *sg;
	unsigned int i;

	desc = jz4740_dma_alloc_desc(sg_len);
	if (!desc)
		return NULL;

	for_each_sg(sgl, sg, sg_len, i) {
		desc->sg[i].addr = sg_dma_address(sg);
		desc->sg[i].len = sg_dma_len(sg);
	}

	desc->num_sgs = sg_len;
	desc->direction = direction;
	desc->cyclic = false;

	return vchan_tx_prep(&chan->vchan, &desc->vdesc, flags);
}

static struct dma_async_tx_descriptor *jz4740_dma_prep_dma_cyclic(
	struct dma_chan *c, dma_addr_t buf_addr, size_t buf_len,
	size_t period_len, enum dma_transfer_direction direction,
	unsigned long flags, void *context)
{
	struct jz4740_dmaengine_chan *chan = to_jz4740_dma_chan(c);
	struct jz4740_dma_desc *desc;
	unsigned int num_periods, i;

	if (buf_len % period_len)
		return NULL;

	num_periods = buf_len / period_len;

	desc = jz4740_dma_alloc_desc(num_periods);
	if (!desc)
		return NULL;

	for (i = 0; i < num_periods; i++) {
		desc->sg[i].addr = buf_addr;
		desc->sg[i].len = period_len;
		buf_addr += period_len;
	}

	desc->num_sgs = num_periods;
	desc->direction = direction;
	desc->cyclic = true;

	return vchan_tx_prep(&chan->vchan, &desc->vdesc, flags);
}

static size_t jz4740_dma_desc_residue(struct jz4740_dmaengine_chan *chan,
	struct jz4740_dma_desc *desc, unsigned int next_sg)
{
	struct jz4740_dma_dev *dmadev = jz4740_dma_chan_get_dev(chan);
	unsigned int residue, count;
	unsigned int i;

	residue = 0;

	for (i = next_sg; i < desc->num_sgs; i++)
		residue += desc->sg[i].len;

	if (next_sg != 0) {
		count = jz4740_dma_read(dmadev,
			JZ_REG_DMA_TRANSFER_COUNT(chan->id));
		residue += count << chan->transfer_shift;
	}

	return residue;
}

static enum dma_status jz4740_dma_tx_status(struct dma_chan *c,
	dma_cookie_t cookie, struct dma_tx_state *state)
{
	struct jz4740_dmaengine_chan *chan = to_jz4740_dma_chan(c);
	struct virt_dma_desc *vdesc;
	enum dma_status status;
	unsigned long flags;

	status = dma_cookie_status(c, cookie, state);
	if (status == DMA_COMPLETE || !state)
		return status;

	spin_lock_irqsave(&chan->vchan.lock, flags);
	vdesc = vchan_find_desc(&chan->vchan, cookie);
	if (cookie == chan->desc->vdesc.tx.cookie) {
		state->residue = jz4740_dma_desc_residue(chan, chan->desc,
				chan->next_sg);
	} else if (vdesc) {
		state->residue = jz4740_dma_desc_residue(chan,
				to_jz4740_dma_desc(vdesc), 0);
	} else {
		state->res