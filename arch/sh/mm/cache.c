/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000-2005 Silicon Graphics, Inc. All rights reserved.
 */

#ifndef _ASM_IA64_SN_GEO_H
#define _ASM_IA64_SN_GEO_H

/* The geoid_t implementation below is based loosely on the pcfg_t
   implementation in sys/SN/promcfg.h. */

/* Type declaractions */

/* Size of a geoid_t structure (must be before decl. of geoid_u) */
#define GEOID_SIZE	8	/* Would 16 be better?  The size can
				   be different on different platforms. */

#define MAX_SLOTS	0xf	/* slots per module */
#define MAX_SLABS	0xf	/* slabs per slot */

typedef unsigned char	geo_type_t;

/* Fields common to all substructures */
typedef struct geo_common_s {
    moduleid_t	module;		/* The module (box) this h/w lives in */
    geo_type_t	type;		/* What type of h/w is named by this geoid_t */
    slabid_t	slab:4;		/* slab (ASIC), 0 .. 15 within slot */
    slotid_t	slot:4;		/* slot (Blade), 0 .. 15 within module */
} geo_common_t;

/* Additional fields for particular types of hardware */
typedef struct geo_node_s {
    geo_common_t	common;		/* No additional fields needed */
} geo_node_t;

typedef struct geo_rtr_s {
    geo_common_t	common;		/* No additional fields needed */
} geo_rtr_t;

typedef struct geo_iocntl_s {
    geo_common_t	common;		/* No additional fields needed */
} geo_iocntl_t;

typedef struct geo_pcicard_s {
    geo_iocntl_t	common;
    char		bus;	/* Bus/widget number */
    char		slot;	/* PCI slot number */
} geo_pcicard_t;

/* Subcomponents of a node */
typedef struct geo_cpu_s {
    geo_node_t	node;
    char	slice;		/* Which CPU on the node */
} geo_cpu_t;

typedef struct geo_mem_s {
    geo_node_t	node;
    char	membus;		/* The memory bus on the node */
    char	memslot;	/* The memory slot on the bus */
} geo_mem_t;


typedef union geoid_u {
    geo_common_t	common;
    geo_node_t		node;
    geo_iocntl_t	iocntl;
    geo_pcicard_t	pcicard;
    geo_rtr_t		rtr;
    geo_cpu_t		cpu;
    geo_mem_t		mem;
    char		padsize[GEOID_SIZE];
} geoid_t;


/* Preprocessor macros */

#define GEO_MAX_LEN	48	/* max. formatted length, plus some pad:
				   module/001c07/slab/5/node/memory/2/slot/4 */

/* Values for geo_type_t */
#define GEO_TYPE_INVALID	0
#define GEO_TYPE_MODULE		1
#define GEO_TYPE_NODE		2
#define GEO_TYPE_RTR		3
#define GEO_TYPE_IOCNTL		4
#define GEO_TYPE_IOCARD		5
#define GEO_TYPE_CPU		6
#define GEO_TYPE_MEM		7
#define GEO_TYPE_MAX		(GEO_TYPE_MEM+1)

/* Parameter for hwcfg_format_geoid_compt() */
#define GEO_COMPT_MODULE	1
#define GEO_COMPT_SLAB		2
#define GEO_COMPT_IOBUS		3
#define GEO_COMPT_IOSLOT	4
#define GEO_COMPT_CPU		5
#define GEO_COMPT_MEMBUS	6
#define GEO_COMPT_MEMSLOT	7

#define GEO_INVALID_STR		"<invalid>"

#define INVALID_NASID           ((nasid_t)-1)
#define INVALID_CNODEID         ((cnodeid_t)-1)
#define INVALID_PNODEID         ((pnodeid_t)-1)
#define INVALID_SLAB            (slabid_t)-1
#define INVALID_SLOT            (slotid_t)-1
#define INVALID_MODULE          ((moduleid_t)-1)

static inline slabid_t geo_slab(geoid_t g)
{
	return (g.common.type == GEO_TYPE_INVALID) ?
		INVALID_SLAB : g.common.slab;
}

static inline slotid_t geo_slot(geoid_t g)
{
	return (g.common.type == GEO_TYPE_INVALID) ?
		INVALID_SLOT : g.common.slot;
}

static inline moduleid_t geo_module(geoid_t g)
{
	return (g.common.type == GEO_TYPE_INVALID) ?
		INVALID_MODULE : g.common.module;
}

extern geoid_t cnodeid_get_geoid(cnodeid_t cnode);

#endif /* _ASM_IA64_SN_GEO_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000-2006 Silicon Graphics, Inc. All rights reserved.
 */

#ifndef _ASM_IA64_SN_INTR_H
#define _ASM_IA64_SN_INTR_H

#include <linux/rcupdate.h>
#include <asm/sn/types.h>

#define SGI_UART_VECTOR		0xe9

/* Reserved IRQs : Note, not to exceed IA64_SN2_FIRST_DEVICE_VECTOR */
#define SGI_XPC_ACTIVATE	0x30
#define SGI_II_ERROR		0x31
#define SGI_XBOW_ERROR		0x32
#define SGI_PCIASIC_ERROR	0x33
#define SGI_ACPI_SCI_INT	0x34
#define SGI_TIOCA_ERROR		0x35
#define SGI_TIO_ERROR		0x36
#define SGI_TIOCX_ERROR		0x37
#define SGI_MMTIMER_VECTOR	0x38
#define SGI_XPC_NOTIFY		0xe7

#define IA64_SN2_FIRST_DEVICE_VECTOR	0x3c
#define IA64_SN2_LAST_DEVICE_VECTOR	0xe6

#define SN2_IRQ_RESERVED	0x1
#define SN2_IRQ_CONNECTED	0x2
#define SN2_IRQ_SHARED		0x4

// The SN PROM irq struct
struct sn_irq_info {
	struct sn_irq_info *irq_next;	/* deprecated DO NOT USE     */
	short		irq_nasid;	/* Nasid IRQ is assigned to  */
	int		irq_slice;	/* slice IRQ is assigned to  */
	int		irq_cpuid;	/* kernel logical cpuid	     */
	int		irq_irq;	/* the IRQ number */
	int		irq_int_bit;	/* Bridge interrupt pin */
					/* <0 means MSI */
	u64	irq_xtalkaddr;	/* xtalkaddr IRQ is sent to  */
	int		irq_bridge_type;/* pciio asic type (pciio.h) */
	void	       *irq_bridge;	/* bridge generating irq     */
	void	       *irq_pciioinfo;	/* associated pciio_info_t   */
	int		irq_last_intr;	/* For Shub lb lost intr WAR */
	int		irq_cookie;	/* unique cookie 	     */
	int		irq_flags;	/* flags */
	int		irq_share_cnt;	/* num devices sharing IRQ   */
	struct list_head	list;	/* list of sn_irq_info structs */
	struct rcu_head		rcu;	/* rcu callback list */
};

extern void sn_send_IPI_phys(int, long, int, int);
extern u64 sn_intr_alloc(nasid_t, int,
			      struct sn_irq_info *,
			      int, nasid_t, int);
extern void sn_intr_free(nasid_t, int, struct sn_irq_info *);
extern struct sn_irq_info *sn_retarget_vector(struct sn_irq_info *, nasid_t, int);
extern void sn_set_err_irq_affinity(unsigned int);
extern struct list_head **sn_irq_lh;

#define CPU_VECTOR_TO_IRQ(cpuid,vector) (vector)

#endif /* _ASM_IA64_SN_INTR_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /* 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000-2004 Silicon Graphics, Inc. All rights reserved.
 */

#ifndef _ASM_SN_IO_H
#define _ASM_SN_IO_H
#include <linux/compiler.h>
#include <asm/intrinsics.h>

extern void * sn_io_addr(unsigned long port) __attribute_const__; /* Forward definition */
extern void __sn_mmiowb(void); /* Forward definition */

extern int num_cnodes;

#define __sn_mf_a()   ia64_mfa()

extern void sn_dma_flush(unsigned long);

#define __sn_inb ___sn_inb
#define __sn_inw ___sn_inw
#define __sn_inl ___sn_inl
#define __sn_outb ___sn_outb
#define __sn_outw ___sn_outw
#define __sn_outl ___sn_outl
#define __sn_readb ___sn_readb
#define __sn_readw ___sn_readw
#define __sn_readl ___sn_readl
#define __sn_readq ___sn_readq
#define __sn_readb_relaxed ___sn_readb_relaxed
#define __sn_readw_relaxed ___sn_readw_relaxed
#define __sn_readl_relaxed ___sn_readl_relaxed
#define __sn_readq_relaxed ___sn_readq_relaxed

/*
 * Convenience macros for setting/clearing bits using the above accessors
 */

#define __sn_setq_relaxed(addr, val) \
	writeq((__sn_readq_relaxed(addr) | (val)), (addr))
#define __sn_clrq_relaxed(