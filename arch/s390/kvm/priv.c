/*
 * File:	mca_drv.h
 * Purpose:	Define helpers for Generic MCA handling
 *
 * Copyright (C) 2004 FUJITSU LIMITED
 * Copyright (C) 2004 Hidetoshi Seto <seto.hidetoshi@jp.fujitsu.com>
 */
/*
 * Processor error section:
 *
 *  +-sal_log_processor_info_t *info-------------+
 *  | sal_log_section_hdr_t header;              |
 *  | ...                                        |
 *  | sal_log_mod_error_info_t info[0];          |
 *  +-+----------------+-------------------------+
 *    | CACHE_CHECK    |  ^ num_cache_check v
 *    +----------------+
 *    | TLB_CHECK      |  ^ num_tlb_check v
 *    +----------------+
 *    | BUS_CHECK      |  ^ num_bus_check v
 *    +----------------+
 *    | REG_FILE_CHECK |  ^ num_reg_file_check v
 *    +----------------+
 *    | MS_CHECK       |  ^ num_ms_check v
 *  +-struct cpuid_info *id----------------------+
 *  | regs[5];                                   |
 *  | reserved;                                  |
 *  +-sal_processor_static_info_t *regs----------+
 *  | valid;                                     |
 *  | ...                                        |
 *  | fr[128];                                   |
 *  +--------------------------------------------+
 */

/* peidx: index of processor error section */
typedef struct peidx_table {
	sal_log_processor_info_t        *info;
	struct sal_cpuid_info           *id;
	sal_processor_static_info_t     *regs;
} peidx_table_t;

#define peidx_head(p)   (((p)->info))
#define peidx_mid(p)    (((p)->id))
#define peidx_bottom(p) (((p)->regs))

#define peidx_psp(p)           (&(peidx_head(p)->proc_state_parameter))
#define peidx_field_valid(p)   (&(peidx_head(p)->valid))
#define peidx_minstate_area(p) (&(peidx_bottom(p)->min_state_area))

#define peidx_cache_check_num(p)    (peidx_head(p)->valid.num_cache_check)
#define peidx_tlb_check_num(p)      (peidx_head(p)->valid.num_tlb_check)
#define peidx_bus_check_num(p)      (peidx_head(p)->valid.num_bus_check)
#define peidx_reg_file_check_num(p) (peidx_head(p)->valid.num_reg_file_check)
#define peidx_ms_check_num(p)       (peidx_head(p)->valid.num_ms_check)

#define peidx_cache_check_idx(p, n)    (n)
#define peidx_tlb_check_idx(p, n)      (peidx_cache_check_idx(p, peidx_cache_check_num(p)) + n)
#define peidx_bus_check_idx(p, n)      (peidx_tlb_check_idx(p, peidx_tlb_check_num(p)) + n)
#define peidx_reg_file_check_idx(p, n) (peidx_bus_check_idx(p, peidx_bus_check_num(p)) + n)
#define peidx_ms_check_idx(p, n)       (peidx_reg_file_check_idx(p, peidx_reg_file_check_num(p)) + n)

#define peidx_mod_error_info(p, name, n) \
({	int __idx = peidx_##name##_idx(p, n); \
	sal_log_mod_error_info_t *__ret = NULL; \
	if (peidx_##name##_num(p) > n) /*BUG*/ \
		__ret = &(peidx_head(p)->info[__idx]); \
	__ret; })

#define peidx_cache_check(p, n)    peidx_mod_error_info(p, cache_check, n)
#define peidx_tlb_check(p, n)      peidx_mod_error_info(p, tlb_check, n)
#define peidx_bus_check(p, n)      peidx_mod_error_info(p, bus_check, n)
#define peidx_reg_file_check(p, n) peidx_mod_error_info(p, reg_file_check, n)
#define peidx_ms_check(p, n)       peidx_mod_error_info(p, ms_check, n)

#define peidx_check_info(proc, name, n) \
({ \
	sal_log_mod_error_info_t *__info = peidx_mod_error_info(proc, name, n);\
	u64 __temp = __info && __info->valid.check_info \
		? __info->check_info : 0; \
	__temp; })

/* slidx: index of SAL log error record */

typedef struct slidx_list {
	struct list_head list;
	sal_log_section_hdr_t *hdr;
} slidx_list_t;

typedef struct slidx_table {
	sal_log_record_header_t *header;
	int n_sections;			/* # of section headers */
	struct list_head proc_err;
	struct list_head mem_dev_err;
	struct list_head sel_dev_err;
	struct list_head pci_bus_err;
	struct list_head smbios_dev_err;
	struct list_head pci_comp_err;
	struct list_head plat_specific_err;
	struct list_head host_ctlr_err;
	struct list_head plat_bus_err;
	struct list_head unsupported;	/* list of unsupported sections */
} slidx_table_t;

#define slidx_foreach_entry(pos, head) \
	list_for_each_entry(pos, head, list)
#define slidx_first_entry(head) \
	(((head)->next != (head)) ? list_entry((head)->next, typeof(slidx_list_t), list) : NULL)
#define slidx_count(slidx, sec) \
({	int __count = 0; \
	slidx_list_t *__pos; \
	slidx_foreach_entry(__pos, &((slidx)->sec)) { __count++; }\
	__count; })

struct mca_table_entry {
	int start_addr;	/* location-relative starting address of MCA recoverable range */
	int end_addr;	/* location-relative ending address of MCA recoverable range */
};

extern const struct mca_table_entry *search_mca_tables (unsigned long addr);
extern int mca_recover_range(unsigned long);
extern void ia64_mlogbuf_dump(void);

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * File:        mca_drv_asm.S
 * Purpose:     Assembly portion of Generic MCA handling
 *
 * Copyright (C) 2004 FUJITSU LIMITED
 * Copyright (C) 2004 Hidetoshi Seto <seto.hidetoshi@jp.fujitsu.com>
 */
#include <linux/threads.h>

#include <asm/asmmacro.h>
#include <asm/processor.h>
#include <asm/ptrace.h>

GLOBAL_ENTRY(mca_handler_bhhook)
	invala				// clear RSE ?
	cover
	;;
	clrrrb
	;;						
	alloc	r16=ar.pfs,0,2,3,0	// make a new frame
	mov	ar.rsc=0
	mov	r13=IA64_KR(CURRENT)	// current task pointer
	;;
	mov	r2=r13
	;;
	addl	r22=IA64_RBS_OFFSET,r2
	;;
	mov	ar.bspstore=r22
	addl	sp=IA64_STK_OFFSET-IA64_PT_REGS_SIZE,r2
	;;
	adds	r2=IA64_TASK_THREAD_ON_USTACK_OFFSET,r13
	;;
	st1	[r2]=r0		// clear current->thread.on_ustack flag
	mov	loc0=r16
	movl	loc1=mca_handler_bh	// recovery C function
	;;
	mov	out0=r8			// poisoned address
	mov	out1=r9			// iip
	mov	out2=r10		// psr
	mov	b6=loc1
	;;
	mov	loc1=rp
	ssm	psr.ic
	;;
	srlz.i
	;;
	ssm	psr.i
	br.call.sptk.many rp=b6		// does not return ...
	;;
	mov	ar.pfs=loc0
	mov 	rp=loc1
	;;
	mov	r8=r0
	br.ret.sptk.many rp
END(mca_handler_bhhook)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
#include <asm/cache.h>

#include "entry.h"
#include <asm/native/inst.h>

#ifdef CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
/* read ar.itc in advance, and use it before leaving bank 0 */
#define ACCOUNT_GET_STAMP				\
(pUStk) mov.m r20=ar.itc;
#define ACCOUNT_SYS_ENTER				\
(pUStk) br.call.spnt rp=account_sys_enter		\
	;;
#else
#define ACCOUNT_GET_STAMP
#define ACCOUNT_SYS_ENTER
#endif

.section ".data..patch.rse", "a"
.previous

/*
 * DO_SAVE_MIN switches to the kernel stacks (if necessary) and saves
 * the minimum state necessary that allows us to turn psr.ic back
 * on.
 *
 * Assumed state upon entry:
 *	psr.ic: off
 *	r31:	contains saved predicates (pr)
 *
 * Upon exit, the state is as follows:
 *	psr.ic: off
 *	 r2 = points to &pt_regs.r16
 *	 r8 = contents of ar.ccv
 *	 r9 = contents of ar.csd
 *	r10 = contents of ar.ssd
 *	r11 = FPSR_DEFAULT
 *	r12 = kernel sp (kernel virtual address)
 *	r13 = points to current task_struct (kernel virtual address)
 *	p15 = TRUE if psr.i is set in cr.ipsr
 *	predicate registers (other than p2, p3, and p15), b6, r3, r14, r15:
 *		preserved
 *
 * Note that psr.ic is NOT turned on by this macro.  This is so that
 * we can pass interruption state as arguments to a handler.
 */
#define IA64_NATIVE_DO_SAVE_MIN(__COVER,SAVE_IFS,EXTRA,WORKAROUND)				\
	mov r16=IA64_KR(CURRENT);	/* M */							\
	mov r27=ar.rsc;			/* M */							\
	mov r20=r1;			/* A */							\
	mov r25=ar.unat;		/* M */							\
	MOV_FROM_IPSR(p0,r29);		/* M */							\
	mov r26=ar.pfs;			/* I */							\
	MOV_FROM_IIP(r28);			/* M */						\
	mov r21=ar.fpsr;		/* M */							\
	__COVER;				/* B;; (or nothing) */				\
	;;											\
	adds r16=IA64_TASK_THREAD_ON_USTACK_OFFSET,r16;						\
	;;											\
	ld1 r17=[r16];				/* load current->thread.on_ustack flag */	\
	st1 [r16]=r0;				/* clear current->thread.on_ustack flag */	\
	adds r1=-IA64_TASK_THREAD_ON_USTACK_OFFSET,r16						\
	/* switch from user to kernel RBS: */							\
	;;											\
	invala;				/* M */							\
	SAVE_IFS;										\
	cmp.eq pKStk,pUStk=r0,r17;		/* are we in kernel mode already? */		\
	;;											\
(pUStk)	mov ar.rsc=0;		/* set enforced lazy mode, pl 0, little-endian, loadrs=0 */	\
	;;											\
(pUStk)	mov.m r24=ar.rnat;									\
(pUStk)	addl r22=IA64_RBS_OFFSET,r1;			/* compute base of RBS */		\
(pKStk) mov r1=sp;					/* get sp  */				\
	;;											\
(pUStk) lfetch.fault.excl.nt1 [r22];								\
(pUStk)	addl r1=IA64_STK_OFFSET-IA64_PT_REGS_SIZE,r1;	/* compute base of memory stack */	\
(pUStk)	mov r23=ar.bspstore;				/* save ar.bspstore */			\
	;;											\
(pUStk)	mov ar.bspstore=r22;				/* switch to kernel RBS */		\
(pKStk) addl r1=-IA64_PT_REGS_SIZE,r1;			/* if in kernel mode, use sp (r12) */	\
	;;											\
(pUStk)	mov r18=ar.bsp;										\
(pUStk)	mov ar.rsc=0x3;		/* set eager mode, pl 0, little-endian, loadrs=0 */		\
	adds r17=2*L1_CACHE_BYTES,r1;		/* really: biggest cache-line size */		\
	adds r16=PT(CR_IPSR),r1;								\
	;;											\
	lfetch.fault.excl.nt1 [r17],L1_CACHE_BYTES;						\
	st8 [r16]=r29;		/* save cr.ipsr */						\
	;;											\
	lfetch.fault.excl.nt1 [r17];								\
	tbit.nz p15,p0=r29,IA64_PSR_I_BIT;							\
	mov r29=b0										\
	;;											\
	WORKAROUND;										\
	adds r16=PT(R8),r1;	/* initialize first base pointer */				\
	adds r17=PT(R9),r1;	/* initialize second base pointer */				\
(pKStk)	mov r18=r0;		/* make sure r18 isn't NaT */					\
	;;											\
.mem.offset 0,0; st8.spill [r16]=r8,16;								\
.mem.offset 8,0; st8.spill [r17]=r9,16;								\
        ;;											\
.mem.offset 0,0; st8.spill [r16]=r10,24;							\
.mem.offset 8,0; st8.spill [r17]=r11,24;							\
        ;;											\
	st8 [r16]=r28,16;	/* save cr.iip */						\
	st8 [r17]=r30,16;	/* save cr.ifs */						\
(pUStk)	sub r18=r18,r22;	/* r18=RSE.ndirty*8 */						\
	mov r8=ar.ccv;										\
	mov r9=ar.csd;										\
	mov r10=ar.ssd;										\
	movl r11=FPSR_DEFAULT;   /* L-unit */							\
	;;											\
	st8 [r16]=r25,16;	/* save ar.unat */						\
	st8 [r17]=r26,16;	/* save ar.pfs */						\
	shl r18=r18,16;		/* compute ar.rsc to be used for "loadrs" */			\
	;;											\
	st8 [r16]=r27,16;	/* save ar.rsc */						\
(pUStk)	st8 [r17]=r24,16;	/* save ar.rnat */						\
(pKStk)	adds r17=16,r17;	/* skip over ar_rnat field */					\
	;;			/* avoid RAW on r16 & r17 */					\
(pUStk)	st8 [r16]=r23,16;	/* save ar.bspstore */						\
	st8 [r17]=r31,16;	/* save predicates */						\
(pKStk)	adds r16=16,r16;	/* skip over ar_bspstore field */				\
	;;											\
	st8 [r16]=r29,16;	/* save b0 */							\
	st8 [r17]=r18,16;	/* save ar.rsc value for "loadrs" */				\
	cmp.eq pNonSys,pSys=r0,r0	/* initialize pSys=0, pNonSys=1 */			\
	;;											\
.mem.offset 0,0; st8.spill [r16]=r20,16;	/* save original r1 */				\
.mem.offset 8,0; st8.spill [r17]=r12,16;							\
	adds r12=-16,r1;	/* switch to kernel memory stack (with 16 bytes of scratch) */	\
	;;											\
.mem.offset 0,0; st8.spill [r16]=r13,16;							\
.mem.offset 8,0; st8.spill [r17]=r21,16;	/* save ar.fpsr */				\
	mov r13=IA64_KR(CURRENT);	/* establish `current' */				\
	;;											\
.mem.offset 0,0; st8.spill [r16]=r15,16;							\
.mem.offset 8,0; st8.spill [r17]=r14,16;							\
	;;											\
.mem.offset 0,0; st8.spill [r16]=r2,16;								\
.mem.offset 8,0; st8.spill [r17]=r3,16;								\
	ACCOUNT_GET_STAMP									\
	adds r2=IA64_PT_REGS_R16_OFFSET,r1;							\
	;;											\
	EXTRA;											\
	movl r1=__gp;		/* establish kernel global pointer */				\
	;;											\
	ACCOUNT_SYS_ENTER									\
	bsw.1;			/* switch back to bank 1 (must be last in insn group) */	\
	;;

/*
 * SAVE_REST saves the remainder of pt_regs (with psr.ic on).
 *
 * Assumed state upon entry:
 *	psr.ic: on
 *	r2:	points to &pt_regs.r16
 *	r3:	points to &pt_regs.r17
 *	r8:	contents of ar.ccv
 *	r9:	contents of ar.csd
 *	r10:	contents of ar.ssd
 *	r11:	FPSR_DEFAULT
 *
 * Registers r14 and r15 are guaranteed not to be touched by SAVE_REST.
 */
#define SAVE_REST				\
.mem.offset 0,0; st8.spill [r2]=r16,16;		\
.mem.offset 8,0; st8.spill [r3]=r17,16;		\
	;;					\
.mem.offset 0,0; st8.spill [r2]=r18,16;		\
.mem.offset 8,0; st8.spill [r3]=r19,16;		\
	;;					\
.mem.offset 0,0; st8.spill [r2]=r20,16;		\
.mem.offset 8,0; st8.spill [r3]=r21,16;		\
	mov r18=b6;				\
	;;					\
.mem.offset 0,0; st8.spill [r2]=r22,16;		\
.mem.offset 8,0; st8.spill [r3]=r23,16;		\
	mov r19=b7;				\
	;;					\
.mem.offset 0,0; st8.spill [r2]=r24,16;		\
.mem.offset 8,0; st8.spill [r3]=r25,16;		\
	;;					\
.mem.offset 0,0; st8.spill [r2]=r26,16;		\
.mem.offset 8,0; st8.spill [r3]=r27,16;		\
	;;					\
.mem.offset 0,0; st8.spill [r2]=r28,16;		\
.mem.offset 8,0; st8.spill [r3]=r29,16;		\
	;;					\
.mem.offset 0,0; st8.spill [r2]=r30,16;		\
.mem.offset 8,0; st8.spill [r3]=r31,32;		\
	;;					\
	mov ar.fpsr=r11;	/* M-unit */	\
	st8 [r2]=r8,8;		/* ar.ccv */	\
	adds r24=PT(B6)-PT(F7),r3;		\
	;;					\
	stf.spill [r2]=f6,32;			\
	stf.spill [r3]=f7,32;			\
	;;					\
	stf.spill [r2]=f8,32;			\
	stf.spill [r3]=f9,32;			\
	;;					\
	stf.spill [r2]=f10;			\
	stf.spill [r3]=f11;			\
	adds r25=PT(B7)-PT(F11),r3;		\
	;;					\
	st8 [r24]=r18,16;       /* b6 */	\
	st8 [r25]=r19,16;       /* b7 */	\
	;;					\
	st8 [r24]=r9;        	/* ar.csd */	\
	st8 [r25]=r10;      	/* ar.ssd */	\
	;;

#define RSE_WORKAROUND				\
(pUStk) extr.u r17=r18,3,6;			\
(pUStk)	sub r16=r18,r22;			\
[1:](pKStk)	br.cond.sptk.many 1f;		\
	.xdata4 ".data..patch.rse",1b-.		\
	;;					\
	cmp.ge p6,p7 = 33,r17;			\
	;;					\
(p6)	mov r17=0x310;				\
(p7)	mov r17=0x308;				\
	;;					\
	cmp.leu p1,p0=r16,r17;			\
(p1)	br.cond.sptk.many 1f;			\
	dep.z r17=r26,0,62;			\
	movl r16=2f;				\
	;;					\
	mov ar.pfs=r17;				\
	dep r27=r0,r27,16,14;			\
	mov b0=r16;				\
	;;					\
	br.ret.sptk b0;				\
	;;					\
2:						\
	mov ar.rsc=r0				\
	;;					\
	flushrs;				\
	;;					\
	mov ar.bspstore=r22			\
	;;					\
	mov r18=ar.bsp;				\
	;;					\
1:						\
	.pred.rel "mutex", pKStk, pUStk

#define SAVE_MIN_WITH_COVER	DO_SAVE_MIN(COVER, mov r30=cr.ifs, , RSE_WORKAROUND)
#define SAVE_MIN_WITH_COVER_R19	DO_SAVE_MIN(COVER, mov r30=cr.ifs, mov r15=r19, RSE_WORKAROUND)
#define SAVE_MIN			DO_SAVE_MIN(     , mov r30=r0, , )
                               /*
 * IA-64-specific support for kernel module loader.
 *
 * Copyright (C) 2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *
 * Loosely based on patch by Rusty Russell.
 */

/* relocs tested so far:

   DIR64LSB
   FPTR64LSB
   GPREL22
   LDXMOV
   LDXMOV
   LTOFF22
   LTOFF22X
   LTOFF22X
   LTOFF_FPTR22
   PCREL21B	(for br.call only; br.cond is not supported out of modules!)
   PCREL60B	(for brl.cond only; brl.call is not supported for modules!)
   PCREL64LSB
   SECREL32LSB
   SEGREL64LSB
 */


#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/elf.h>
#include <linux/moduleloader.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include <asm/patch.h>
#include <asm/unaligned.h>

#define ARCH_MODULE_DEBUG 0

#if ARCH_MODULE_DEBUG
# define DEBUGP printk
# define inline
#else
# define DEBUGP(fmt , a...)
#endif

#ifdef CONFIG_ITANIUM
# define USE_BRL	0
#else
# define USE_BRL	1
#endif

#define MAX_LTOFF	((uint64_t) (1 << 22))	/* max. allowable linkage-table offset */

/* Define some relocation helper macros/types: */

#define FORMAT_SHIFT	0
#define FORMAT_BITS	3
#define FORMAT_MASK	((1 << FORMAT_BITS) - 1)
#define VALUE_SHIFT	3
#define VALUE_BITS	5
#define VALUE_MASK	((1 << VALUE_BITS) - 1)

enum reloc_target_format {
	/* direct encoded formats: */
	RF_NONE = 0,
	RF_INSN14 = 1,
	RF_INSN22 = 2,
	RF_INSN64 = 3,
	RF_32MSB = 4,
	RF_32LSB = 5,
	RF_64MSB = 6,
	RF_64LSB = 7,

	/* formats that cannot be directly decoded: */
	RF_INSN60,
	RF_INSN21B,	/* imm21 form 1 */
	RF_INSN21M,	/* imm21 form 2 */
	RF_INSN21F	/* imm21 form 3 */
};

enum reloc_value_formula {
	RV_DIRECT = 4,		/* S + A */
	RV_GPREL = 5,		/* @gprel(S + A) */
	RV_LTREL = 6,		/* @ltoff(S + A) */
	RV_PLTREL = 7,		/* @pltoff(S + A) */
	RV_FPTR = 8,		/* @fptr(S + A) */
	RV_PCREL = 9,		/* S + A - P */
	RV_LTREL_FPTR = 10,	/* @ltoff(@fptr(S + A)) */
	RV_SEGREL = 11,		/* @segrel(S + A) */
	RV_SECREL = 12,		/* @secrel(S + A) */
	RV_BDREL = 13,		/* BD + A */
	RV_LTV = 14,		/* S + A (like RV_DIRECT, except frozen at static link-time) */
	RV_PCREL2 = 15,		/* S + A - P */
	RV_SPECIAL = 16,	/* various (see below) */
	RV_RSVD17 = 17,
	RV_TPREL = 18,		/* @tprel(S + A) */
	RV_LTREL_TPREL = 19,	/* @ltoff(@tprel(S + A)) */
	RV_DTPMOD = 20,		/* @dtpmod(S + A) */
	RV_LTREL_DTPMOD = 21,	/* @ltoff(@dtpmod(S + A)) */
	RV_DTPREL = 22,		/* @dtprel(S + A) */
	RV_LTREL_DTPREL = 23,	/* @ltoff(@dtprel(S + A)) */
	RV_RSVD24 = 24,
	RV_RSVD25 = 25,
	RV_RSVD26 = 26,
	RV_RSVD27 = 27
	/* 28-31 reserved for implementation-specific purposes.  */
};

#define N(reloc)	[R_IA64_##reloc] = #reloc

static const char *reloc_name[256] = {
	N(NONE),		N(IMM14),		N(IMM22),		N(IMM64),
	N(DIR32MSB),		N(DIR32LSB),		N(DIR64MSB),		N(DIR64LSB),
	N(GPREL22),		N(GPREL64I),		N(GPREL32MSB),		N(GPREL32LSB),
	N(GPREL64MSB),		N(GPREL64LSB),		N(LTOFF22),		N(LTOFF64I),
	N(PLTOFF22),		N(PLTOFF64I),		N(PLTOFF64MSB),		N(PLTOFF64LSB),
	N(FPTR64I),		N(FPTR32MSB),		N(FPTR32LSB),		N(FPTR64MSB),
	N(FPTR64LSB),		N(PCREL60B),		N(PCREL21B),		N(PCREL21M),
	N(PCREL21F),		N(PCREL32MSB),		N(PCREL32LSB),		N(PCREL64MSB),
	N(PCREL64LSB),		N(LTOFF_FPTR22),	N(LTOFF_FPTR64I),	N(LTOFF_FPTR32MSB),
	N(LTOFF_FPTR32LSB),	N(LTOFF_FPTR64MSB),	N(LTOFF_FPTR64LSB),	N(SEGREL32MSB),
	N(SEGREL32LSB),		N(SEGREL64MSB),		N(SEGREL64LSB),		N(SECREL32MSB),
	N(SECREL32LSB),		N(SECREL64MSB),		N(SECREL64LSB),		N(REL32MSB),
	N(REL32LSB),		N(REL64MSB),		N(REL64LSB),		N(LTV32MSB),
	N(LTV32LSB),		N(LTV64MSB),		N(LTV64LSB),		N(PCREL21BI),
	N(PCREL22),		N(PCREL64I),		N(IPLTMSB),		N(IPLTLSB),
	N(COPY),		N(LTOFF22X),		N(LDXMOV),		N(TPREL14),
	N(TPREL22),		N(TPREL64I),		N(TPREL64MSB),		N(TPREL64LSB),
	N(LTOFF_TPREL22),	N(DTPMOD64MSB),		N(DTPMOD64LSB),		N(LTOFF_DTPMOD22),
	N(DTPREL14),		N(DTPREL22),		N(DTPREL64I),		N(DTPREL32MSB),
	N(DTPREL32LSB),		N(DTPREL64MSB),		N(DTPREL64LSB),		N(LTOFF_DTPREL22)
};

#undef N

/* Opaque struct for insns, to protect against derefs. */
struct insn;

static inline uint64_t
bundle (const struct insn *insn)
{
	return (uint64_t) insn & ~0xfUL;
}

static inline int
slot (const struct insn *insn)
{
	return (uint64_t) insn & 0x3;
}

static int
apply_imm64 (struct module *mod, struct insn *insn, uint64_t val)
{
	if (slot(insn) != 1 && slot(insn) != 2) {
		printk(KERN_ERR "%s: invalid slot number %d for IMM64\n",
		       mod->name, slot(insn));
		return 0;
	}
	ia64_patch_imm64((u64) insn, val);
	return 1;
}

static int
apply_imm60 (struct module *mod, struct insn *insn, uint64_t val)
{
	if (slot(insn) != 1 && slot(insn) != 2) {
		printk(KERN_ERR "%s: invalid slot number %d for IMM60\n",
		       mod->name, slot(insn));
		return 0;
	}
	if (val + ((uint64_t) 1 << 59) >= (1UL << 60)) {
		printk(KERN_ERR "%s: value %ld out of IMM60 range\n",
			mod->name, (long) val);
		return 0;
	}
	ia64_patch_imm60((u64) insn, val);
	return 1;
}

static int
apply_imm22 (struct module *mod, struct insn *insn, uint64_t val)
{
	if (val + (1 << 21) >= (1 << 22)) {
		printk(KERN_ERR "%s: value %li out of IMM22 range\n",
			mod->name, (long)val);
		return 0;
	}
	ia64_patch((u64) insn, 0x01fffcfe000UL, (  ((val & 0x200000UL) << 15) /* bit 21 -> 36 */
					         | ((val & 0x1f0000UL) <<  6) /* bit 16 -> 22 */
					         | ((val & 0x00ff80UL) << 20) /* bit  7 -> 27 */
					         | ((val & 0x00007fUL) << 13) /* bit  0 -> 13 */));
	return 1;
}

static int
apply_imm21b (struct module *mod, struct insn *insn, uint64_t val)
{
	if (val + (1 << 20) >= (1 << 21)) {
		printk(KERN_ERR "%s: value %li out of IMM21b range\n",
			mod->name, (long)val);
		return 0;
	}
	ia64_patch((u64) insn, 0x11ffffe000UL, (  ((val & 0x100000UL) << 16) /* bit 20 -> 36 */
					        | ((val & 0x0fffffUL) << 13) /* bit  0 -> 13 */));
	return 1;
}

#if USE_BRL

struct plt_entry {
	/* Three instruction bundles in PLT. */
 	unsigned char bundle[2][16];
};

static const struct plt_entry ia64_plt_template = {
	{
		{
			0x04, 0x00, 0x00, 0x00, 0x01, 0x00, /* [MLX] nop.m 0 */
			0x00, 0x00, 0x00, 0x00, 0x00, 0x20, /*	     movl gp=TARGET_GP */
			0x00, 0x00, 0x00, 0x60
		},
		{
			0x05, 0x00, 0x00, 0x00, 0x01, 0x00, /* [MLX] nop.m 0 */
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*	     brl.many gp=TARGET_GP */
			0x08, 0x00, 0x00, 0xc0
		}
	}
};

static int
patch_plt (struct module *mod, struct plt_entry *plt, long target_ip, unsigned long target_gp)
{
	if (apply_imm64(mod, (struct insn *) (plt->bundle[0] + 2), target_gp)
	    && apply_imm60(mod, (struct insn *) (plt->bundle[1] + 2),
			   (target_ip - (int64_t) plt->bundle[1]) / 16))
		return 1;
	return 0;
}

unsigned long
plt_target (struct plt_entry *plt)
{
	uint64_t b0, b1, *b = (uint64_t *) plt->bundle[1];
	long off;

	b0 = b[0]; b1 = b[1];
	off = (  ((b1 & 0x00fffff000000000UL) >> 36)		/* imm20b -> bit 0 */
	       | ((b0 >> 48) << 20) | ((b1 & 0x7fffffUL) << 36)	/* imm39 -> bit 20 */
	       | ((b1 & 0x0800000000000000UL) << 0));		/* i -> bit 59 */
	return (long) plt->bundle[1] + 16*off;
}

#else /* !USE_BRL */

struct plt_entry {
	/* Three instruction bundles in PLT. */
 	unsigned char bundle[3][16];
};

static const struct plt_entry ia64_plt_template = {
	{
		{
			0x05, 0x00, 0x00, 0x00, 0x01, 0x00, /* [MLX] nop.m 0 */
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*	     movl r16=TARGET_IP */
			0x02, 0x00, 0x00, 0x60
		},
		{
			0x04, 0x00, 0x00, 0x00, 0x01, 0x00, /* [MLX] nop.m 0 */
			0x00, 0x00, 0x00, 0x00, 0x00, 0x20, /*	     movl gp=TARGET_GP */
			0x00, 0x00, 0x00, 0x60
		},
		{
			0x11, 0x00, 0x00, 0x00, 0x01, 0x00, /* [MIB] nop.m 0 */
			0x60, 0x80, 0x04, 0x80, 0x03, 0x00, /*	     mov b6=r16 */
			0x60, 0x00, 0x80, 0x00		    /*	     br.few b6 */
		}
	}
};

static int
patch_plt (struct module *mod, struct plt_entry *plt, long target_ip, unsigned long target_gp)
{
	if (apply_imm64(mod, (struct insn *) (plt->bundle[0] + 2), target_ip)
	    && apply_imm64(mod, (struct insn *) (plt->bundle[1] + 2), target_gp))
		return 1;
	return 0;
}

unsigned long
plt_target (struct plt_entry *plt)
{
	uint64_t b0, b1, *b = (uint64_t *) plt->bundle[0];

	b0 = b[0]; b1 = b[1];
	return (  ((b1 & 0x000007f000000000) >> 36)	