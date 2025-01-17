/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000-2005 Silicon Graphics, Inc. All rights reserved.
 */
#ifndef _ASM_IA64_SN_PDA_H
#define _ASM_IA64_SN_PDA_H

#include <linux/cache.h>
#include <asm/percpu.h>


/*
 * CPU-specific data structure.
 *
 * One of these structures is allocated for each cpu of a NUMA system.
 *
 * This structure provides a convenient way of keeping together 
 * all SN per-cpu data structures. 
 */

typedef struct pda_s {

	/*
	 * Support for SN LEDs
	 */
	volatile short	*led_address;
	u8		led_state;
	u8		hb_state;	/* supports blinking heartbeat leds */
	unsigned int	hb_count;

	unsigned int	idle_flag;
	
	volatile unsigned long *bedrock_rev_id;
	volatile unsigned long *pio_write_status_addr;
	unsigned long pio_write_status_val;
	volatile unsigned long *pio_shub_war_cam_addr;

	unsigned long	sn_in_service_ivecs[4];
	int		sn_lb_int_war_ticks;
	int		sn_last_irq;
	int		sn_first_irq;
} pda_t;


#define CACHE_ALIGN(x)	(((x) + SMP_CACHE_BYTES-1) & ~(SMP_CACHE_BYTES-1))

/*
 * PDA
 * Per-cpu private data area for each cpu. The PDA is located immediately after
 * the IA64 cpu_data area. A full page is allocated for the cp_data area for each
 * cpu but only a small amout of the page is actually used. We put the SNIA PDA
 * in the same page as the cpu_data area. Note that there is a check in the setup
 * code to verify that we don't overflow the page.
 *
 * Seems like we should should cache-line align the pda so that any changes in the
 * size of the cpu_data area don't change cache layout. Should we align to 32, 64, 128
 * or 512 boundary. Each has merits. For now, pick 128 but should be revisited later.
 */
DECLARE_PER_CPU(struct pda_s, pda_percpu);

#define pda		(&__ia64_per_cpu_var(pda_percpu))

#define pdacpu(cpu)	(&per_cpu(pda_percpu, cpu))

#endif /* _ASM_IA64_SN_PDA_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000-2003 Silicon Graphics, Inc. All rights reserved.
 */
#ifndef _ASM_IA64_SN_PCI_PIC_H
#define _ASM_IA64_SN_PCI_PIC_H

/*
 * PIC AS DEVICE ZERO
 * ------------------
 *
 * PIC handles PCI/X busses.  PCI/X requires that the 'bridge' (i.e. PIC)
 * be designated as 'device 0'.   That is a departure from earlier SGI
 * PCI bridges.  Because of that we use config space 1 to access the
 * config space of the first actual PCI device on the bus.
 * Here's what the PIC manual says:
 *
 *     The current PCI-X bus specification now defines that the parent
 *     hosts bus bridge (PIC for example) must be device 0 on bus 0. PIC
 *     reduced the total number of devices from 8 to 4 and removed the
 *     device registers and windows, now only supporting devices 0,1,2, and
 *     3. PIC did leave all 8 configuration space windows. The reason was
 *     there was nothing to gain by removing them. Here in lies the problem.
 *     The device numbering we do using 0 through 3 is unrelated to the device
 *     numbering which PCI-X requires in configuration space. In the past we
 *     correlated Configs pace and our device space 0 <-> 0, 1 <-> 1, etc.
 *     PCI-X requires we start a 1, not 0 and currently the PX brick
 *     does associate our:
 *
 *         device 0 with configuration space window 1,
 *         device 1 with configuration space window 2,
 *         device 2 with configuration space window 3,
 *         device 3 with configuration space window 4.
 *
 * The net effect is that all config space access are off-by-one with
 * relation to other per-slot accesses on the PIC.
 * Here is a table that shows some of that:
 *
 *                               Internal Slot#
 *           |
 *           |     0         1        2         3
 * ----------|---------------------------------------
 * config    |  0x21000   0x22000  0x23000   0x24000
 *           |
 * even rrb  |  0[0]      n/a      1[0]      n/a	[] == implied even/odd
 *           |
 * odd rrb   |  n/a       0[1]     n/a       1[1]
 *           |
 * int dev   |  00       01        10        11
 *           |
 * ext slot# |  1        2         3         4
 * ----------|---------------------------------------
 */

#define PIC_ATE_TARGETID_SHFT           8
#define PIC_HOST_INTR_ADDR              0x0000FFFFFFFFFFFFUL
#define PIC_PCI64_ATTR_TARG_SHFT        60


/*****************************************************************************
 *********************** PIC MMR structure mapping ***************************
 *****************************************************************************/

/* NOTE: PIC WAR. PV#854697.  PIC does not allow writes just to [31:0]
 * of a 64-bit register.  When writing PIC registers, always write the
 * entire 64 bits.
 */

struct pic {

    /* 0x000000-0x00FFFF -- Local Registers */

    /* 0x000000-0x000057 -- Standard Widget Configuration */
    u64		p_wid_id;			/* 0x000000 */
    u64		p_wid_stat;			/* 0x000008 */
    u64		p_wid_err_upper;		/* 0x000010 */
    u64		p_wid_err_lower;		/* 0x000018 */
    #define p_wid_err p_wid_err_lower
    u64		p_wid_control;			/* 0x000020 */
    u64		p_wid_req_timeout;		/* 0x000028 */
    u64		p_wid_int_upper;		/* 0x000030 */
    u64		p_wid_int_lower;		/* 0x000038 */
    #define p_wid_int p_wid_int_lower
    u64		p_wid_err_cmdword;		/* 0x000040 */
    u64		p_wid_llp;			/* 0x000048 */
    u64		p_wid_tflush;			/* 0x000050 */

    /* 0x000058-0x00007F -- Bridge-specific Widget Configuration */
    u64		p_wid_aux_err;			/* 0x000058 */
    u64		p_wid_resp_upper;		/* 0x000060 */
    u64		p_wid_resp_lower;		/* 0x000068 */
    #define p_wid_resp p_wid_resp_lower
    u64		p_wid_tst_pin_ctrl;		/* 0x000070 */
    u64		p_wid_addr_lkerr;		/* 0x000078 */

    /* 0x000080-0x00008F -- PMU & MAP */
    u64		p_dir_map;			/* 0x000080 */
    u64		_pad_000088;			/* 0x000088 */

    /* 0x000090-0x00009F -- SSRAM */
    u64		p_map_fault;			/* 0x000090 */
    u64		_pad_000098;			/* 0x000098 */

    /* 0x0000A0-0x0000AF -- Arbitration */
    u64		p_arb;				/* 0x0000A0 */
    u64		_pad_0000A8;			/* 0x0000A8 */

    /* 0x0000B0-0x0000BF -- Number In A Can or ATE Parity Error */
    u64		p_ate_parity_err;		/* 0x0000B0 */
    u64		_pad_0000B8;			/* 0x0000B8 */

    /* 0x0000C0-0x0000FF -- PCI/GIO */
    u64		p_bus_timeout;			/* 0x0000C0 */
    u64		p_pci_cfg;			/* 0x0000C8 */
    u64		p_pci_err_upper;		/* 0x0000D0 */
    u64		p_pci_err_lower;		/* 0x0000D8 */
    #define p_pci_err p_pci_err_lower
   