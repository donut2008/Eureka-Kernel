.
//--

ia64_old_stack:
	add regs=MCA_PT_REGS_OFFSET, r3
	mov b0=r2			// save return address
	GET_IA64_MCA_DATA(temp2)
	LOAD_PHYSICAL(p0,temp1,1f)
	;;
	mov cr.ipsr=r0
	mov cr.ifs=r0
	mov cr.iip=temp1
	;;
	invala
	rfi
1:

	add regs=regs, temp2		// struct pt_regs on MCA or INIT stack
	;;
	add temp1=PT(LOADRS), regs
	;;
	ld8 temp2=[temp1],PT(AR_BSPSTORE)-PT(LOADRS)	// restore loadrs
	;;
	ld8 temp3=[temp1],PT(AR_RNAT)-PT(AR_BSPSTORE)	// restore ar.bspstore
	mov ar.rsc=temp2
	;;
	loadrs
	ld8 temp4=[temp1]		// restore ar.rnat
	;;
	mov ar.bspstore=temp3		// back to old stack
	;;
	mov ar.rnat=temp4
	;;

	br.sptk b0

//EndStub//////////////////////////////////////////////////////////////////////


//++
// Name:
//	ia64_set_kernel_registers()
//
// Stub Description:
//
//	Set the registers that are required by the C code in order to run on an
//	MCA/INIT stack.
//
//	r2 contains the return address, r3 contains either
//	IA64_MCA_CPU_MCA_STACK_OFFSET or IA64_MCA_CPU_INIT_STACK_OFFSET.
//
//--

ia64_set_kernel_registers:
	add temp3=MCA_SP_OFFSET, r3
	mov b0=r2		// save return address
	GET_IA64_MCA_DATA(temp1)
	;;
	add r12=temp1, temp3	// kernel stack pointer on MCA/INIT stack
	add r13=temp1, r3	// set current to start of MCA/INIT stack
	add r20=temp1, r3	// physical start of MCA/INIT stack
	;;
	DATA_PA_TO_VA(r12,temp2)
	DATA_PA_TO_VA(r13,temp3)
	;;
	mov IA64_KR(CURRENT)=r13

	/* Wire IA64_TR_CURRENT_STACK to the MCA/INIT handler stack.  To avoid
	 * any dependencies on the algorithm in ia64_switch_to(), just purge
	 * any existing CURRENT_STACK mapping and insert the new one.
	 */

	mov r16=IA64_KR(CURRENT_STACK)		// physical granule mapped by IA64_TR_CURRENT_STACK
	;;
	shl r16=r16,IA64_GRANULE_SHIFT
	;;
	dep r16=-1,r16,61,3			// virtual granule
	mov r18=IA64_GRANULE_SHIFT<<2		// for cr.itir.ps
	;;
	ptr.d r16,r18
	;;
	srlz.d

	shr.u r16=r20,IA64_GRANULE_SHIFT	// r20 = physical start of MCA/INIT stack
	movl r21=PAGE_KERNEL			// page properties
	;;
	mov IA64_KR(CURRENT_STACK)=r16
	or r21=r20,r21				// construct PA | page properties
	;;
	mov cr.itir=r18
	mov cr.ifa=r13
	mov r20=IA64_TR_CURRENT_STACK

	movl r17=FPSR_DEFAULT
	;;
	mov.m ar.fpsr=r17			// set ar.fpsr to kernel default value
	;;
	itr.d dtr[r20]=r21
	;;
	srlz.d

	br.sptk b0

//EndStub//////////////////////////////////////////////////////////////////////

#undef	ms
#undef	regs
#undef	temp1
#undef	temp2
#undef	temp3
#undef	temp4


// Support function for mca.c, it is here to avoid using inline asm.  Given the
// address of an rnat slot, if that address is below the current ar.bspstore
// then return the contents of that slot, otherwise return the contents of
// ar.rnat.
GLOBAL_ENTRY(ia64_get_rnat)
	alloc r14=ar.pfs,1,0,0,0
	mov ar.rsc=0
	;;
	mov r14=ar.bspstore
	;;
	cmp.lt p6,p7=in0,r14
	;;
(p6)	ld8 r8=[in0]
(p7)	mov r8=ar.rnat
	mov ar.rsc=3
	br.ret.sptk.many rp
END(ia64_get_rnat)


// void ia64_set_psr_mc(void)
//
// Set psr.mc bit to mask MCA/INIT.
GLOBAL_ENTRY(ia64_set_psr_mc)
	rsm psr.i | psr.ic		// disable interrupts
	;;
	srlz.d
	;;
	mov r14 = psr			// get psr{36:35,31:0}
	movl r15 = 1f
	;;
	dep r14 = -1, r14, PSR_MC, 1	// set psr.mc
	;;
	dep r14 = -1, r14, PSR_IC, 1	// set psr.ic
	;;
	dep r14 = -1, r14, PSR_BN, 1	// keep bank1 in use
	;;
	mov cr.ipsr = r14
	mov cr.ifs = r0
	mov cr.iip = r15
	;;
	rfi
1:
	br.ret.sptk.many rp
END(ia64_set_psr_mc)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * File:	mca_drv.c
 * Purpose:	Generic MCA handling layer
 *
 * Copyright (C) 2004 FUJITSU LIMITED
 * Copyright (C) 2004 Hidetoshi Seto <seto.hidetoshi@jp.fujitsu.com>
 * Copyright (C) 2005 Silicon Graphics, Inc
 * Copyright (C) 2005 Keith Owens <kaos@sgi.com>
 * Copyright (C) 2006 Russ Anderson <rja@sgi.com>
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kallsyms.h>
#include <linux/bootmem.h>
#include <linux/acpi.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <asm/delay.h>
#include <asm/machvec.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/sal.h>
#include <asm/mca.h>

#include <asm/irq.h>
#include <asm/hw_irq.h>

#include "mca_drv.h"

/* max size of SAL error record (default) */
static int sal_rec_max = 10000;

/* from mca_drv_asm.S */
extern void *mca_handler_bhhook(void);

static DEFINE_SPINLOCK(mca_bh_lock);

typedef enum {
	MCA_IS_LOCAL  = 0,
	MCA_IS_GLOBAL = 1
} mca_type_t;

#define MAX_PAGE_ISOLATE 1024

static struct page *page_isolate[MAX_PAGE_ISOLATE];
static int num_page_isolate = 0;

typedef enum {
	ISOLATE_NG,
	ISOLATE_OK,
	ISOLATE_NONE
} isolate_status_t;

typedef enum {
	MCA_NOT_RECOVERED = 0,
	MCA_RECOVERED	  = 1
} recovery_status_t;

/*
 *  This pool keeps pointers to the section part of SAL error record
 */
static struct {
	slidx_list_t *buffer; /* section pointer list pool */
	int	     cur_idx; /* Current index of section pointer list pool */
	int	     max_idx; /* Maximum index of section pointer list pool */
} slidx_pool;

static int
fatal_mca(const char *fmt, ...)
{
	va_list args;
	char buf[256];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	ia64_mca_printk(KERN_ALERT "MCA: %s\n", buf);

	return MCA_NOT_RECOVERED;
}

static int
mca_recovered(const char *fmt, ...)
{
	va_list args;
	char buf[256];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	ia64_mca_printk(KERN_INFO "MCA: %s\n", buf);

	return MCA_RECOVERED;
}

/**
 * mca_page_isolate - isolate a poisoned page in order not to use it later
 * @paddr:	poisoned memory location
 *
 * Return value:
 *	one of isolate_status_t, ISOLATE_OK/NG/NONE.
 */

static isolate_status_t
mca_page_isolate(unsigned long paddr)
{
	int i;
	struct page *p;

	/* whether physical address is valid or not */
	if (!ia64_phys_addr_valid(paddr))
		return ISOLATE_NONE;

	if (!pfn_valid(paddr >> PAGE_SHIFT))
		return ISOLATE_NONE;

	/* convert physical address to physical page number */
	p = pfn_to_page(paddr>>PAGE_SHIFT);

	/* check whether a page number have been already registered or not */
	for (i = 0; i < num_page_isolate; i++)
		if (page_isolate[i] == p)
			return ISOLATE_OK; /* already listed */

	/* limitation check */
	if (num_page_isolate == MAX_PAGE_ISOLATE)
		return ISOLATE_NG;

	/* kick pages having attribute 'SLAB' or 'Reserved' */
	if (PageSlab(p) || PageReserved(p))
		return ISOLATE_NG;

	/* add attribute 'Reserved' and register the page */
	get_page(p);
	SetPageReserved(p);
	page_isolate[num_page_isolate++] = p;

	return ISOLATE_OK;
}

/**
 * mca_hanlder_bh - Kill the process which occurred memory read error
 * @paddr:	poisoned address received from MCA Handler
 */

void
mca_handler_bh(unsigned long paddr, void *iip, unsigned long ipsr)
{
	ia64_mlogbuf_dump();
	printk(KERN_ERR "OS_MCA: process [cpu %d, pid: %d, uid: %d, "
		"iip: %p, psr: 0x%lx,paddr: 0x%lx](%s) encounters MCA.\n",
	       raw_smp_processor_id(), current->pid,
		from_kuid(&init_user_ns, current_uid()),
		iip, ipsr, paddr, current->comm);

	spin_lock(&mca_bh_lock);
	switch (mca_page_isolate(paddr)) {
	case ISOLATE_OK:
		printk(KERN_DEBUG "Page isolation: ( %lx ) success.\n", paddr);
		break;
	case ISOLATE_NG:
		printk(KERN_CRIT "Page isolation: ( %lx ) failure.\n", paddr);
		break;
	default:
		break;
	}
	spin_unlock(&mca_bh_lock);

	/* This process is about to be killed itself */
	do_exit(SIGKILL);
}

/**
 * mca_make_peidx - Make inde