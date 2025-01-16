vm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmov.l		&0x00000200,%fpcr		# enable inexact
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.l		&0x50000000,DATA+0x0(%a6)
	mov.l		&0x80000000,DATA+0x4(%a6)
	mov.l		&0x00000000,DATA+0x8(%a6)
	fmovm.x		DATA(%a6),&0x80

	mov.w		&0x0000,%cc
inex_0_pc:
	fadd.b		&0x2,%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0x50000000,IFPREGS+0x0(%a6)
	mov.l		&0x80000000,IFPREGS+0x4(%a6)
	mov.l		&0x00000000,IFPREGS+0x8(%a6)
	mov.l		&0x00000208,IFPCREGS+0x4(%a6)
	lea		inex_0_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

	clr.l		%d0
	rts

#####################################################################

snan_str:
	string		"\tEnabled SNAN..."

	align		0x4
snan_0:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmov.l		&0x00004000,%fpcr		# enable SNAN
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.l		&0xffff0000,DATA+0x0(%a6)
	mov.l		&0x00000000,DATA+0x4(%a6)
	mov.l		&0x00000001,DATA+0x8(%a6)
	fmovm.x		DATA(%a6),&0x80

	mov.w		&0x0000,%cc
snan_0_pc:
	fadd.b		&0x2,%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0xffff0000,IFPREGS+0x0(%a6)
	mov.l		&0x00000000,IFPREGS+0x4(%a6)
	mov.l		&0x00000001,IFPREGS+0x8(%a6)
	mov.l		&0x09004080,IFPCREGS+0x4(%a6)
	lea		snan_0_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

	clr.l		%d0
	rts

#####################################################################

operr_str:
	string		"\tEnabled OPERR..."

	align		0x4
operr_0:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmov.l		&0x00002000,%fpcr		# enable OPERR
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.l		&0xffff0000,DATA+0x0(%a6)
	mov.l		&0x00000000,DATA+0x4(%a6)
	mov.l		&0x00000000,DATA+0x8(%a6)
	fmovm.x		DATA(%a6),&0x80

	mov.w		&0x0000,%cc
operr_0_pc:
	fadd.s		&0x7f800000,%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0xffff0000,IFPREGS+0x0(%a6)
	mov.l		&0x00000000,IFPREGS+0x4(%a6)
	mov.l		&0x00000000,IFPREGS+0x8(%a6)
	mov.l		&0x01002080,IFPCREGS+0x4(%a6)
	lea		operr_0_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

	clr.l		%d0
	rts

#####################################################################

dz_str:
	string		"\tEnabled DZ..."

	align		0x4
dz_0:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmov.l		&0x00000400,%fpcr		# enable DZ
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.l		&0x40000000,DATA+0x0(%a6)
	mov.l		&0x80000000,DATA+0x4(%a6)
	mov.l		&0x00000000,DATA+0x8(%a6)
	fmovm.x		DATA(%a6),&0x80

	mov.w		&0x0000,%cc
dz_0_pc:
	fdiv.b		&0x0,%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0x40000000,IFPREGS+0x0(%a6)
	mov.l		&0x80000000,IFPREGS+0x4(%a6)
	mov.l		&0x00000000,IFPREGS+0x8(%a6)
	mov.l		&0x02000410,IFPCREGS+0x4(%a6)
	lea		dz_0_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

	clr.l		%d0
	rts

#####################################################################

unsupp_str:
	string		"\tUnimplemented data type/format..."

# an unnormalized number
	align		0x4
unsupp_0:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.l		&0xc03f0000,DATA+0x0(%a6)
	mov.l		&0x00000000,DATA+0x4(%a6)
	mov.l		&0x00000001,DATA+0x8(%a6)
	fmov.b		&0x2,%fp0
	mov.w		&0x0000,%cc
unsupp_0_pc:
	fmul.x		DATA(%a6),%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0xc0010000,IFPREGS+0x0(%a6)
	mov.l		&0x80000000,IFPREGS+0x4(%a6)
	mov.l		&0x00000000,IFPREGS+0x8(%a6)
	mov.l		&0x08000000,IFPCREGS+0x4(%a6)
	lea		unsupp_0_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

# a denormalized number
unsupp_1:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.l		&0x80000000,DATA+0x0(%a6)
	mov.l		&0x01000000,DATA+0x4(%a6)
	mov.l		&0x00000000,DATA+0x8(%a6)
	fmov.l		&0x7fffffff,%fp0

	mov.w		&0x0000,%cc
unsupp_1_pc:
	fmul.x		DATA(%a6),%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0x80170000,IFPREGS+0x0(%a6)
	mov.l		&0xfffffffe,IFPREGS+0x4(%a6)
	mov.l		&0x00000000,IFPREGS+0x8(%a6)
	mov.l		&0x08000000,IFPCREGS+0x4(%a6)
	lea		unsupp_1_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

# packed
unsupp_2:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.l		&0xc1230001,DATA+0x0(%a6)
	mov.l		&0x23456789,DATA+0x4(%a6)
	mov.l		&0x12345678,DATA+0x8(%a6)

	mov.w		&0x0000,%cc
unsupp_2_pc:
	fabs.p		DATA(%a6),%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0x3e660000,IFPREGS+0x0(%a6)
	mov.l		&0xd0ed23e8,IFPREGS+0x4(%a6)
	mov.l		&0xd14035bc,IFPREGS+0x8(%a6)
	mov.l		&0x00000108,IFPCREGS+0x4(%a6)
	lea		unsupp_2_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

	clr.l		%d0
	rts

###########################################################
###########################################################

chkregs:
	lea		IREGS(%a6),%a0
	lea		SREGS(%a6),%a1
	mov.l		&14,%d0
chkregs_loop:
	cmp.l		(%a0)+,(%a1)+
	bne.l		chkregs_error
	dbra.w		%d0,chkregs_loop

	mov.w		ICCR(%a6),%d0
	mov.w		SCCR(%a6),%d1
	cmp.w		%d0,%d1
	bne.l		chkregs_error

	clr.l		%d0
	rts

chkregs_error:
	movq.l		&0x1,%d0
	rts

error:
	mov.l		TESTCTR(%a6),%d1
	movq.l		&0x1,%d0
	rts

chkfpregs:
	lea		IFPREGS(%a6),%a0
	lea		SFPREGS(%a6),%a1
	mov.l		&23,%d0
chkfpregs_loop:
	cmp.l		(%a0)+,(%a1)+
	bne.l		chkfpregs_error
	dbra.w		%d0,chkfpregs_loop

	lea		IFPCREGS(%a6),%a0
	lea		SFPCREGS(%a6),%a1
	cmp.l		(%a0)+,(%a1)+
	bne.l		chkfpregs_error
	cmp.l		(%a0)+,(%a1)+
	bne.l		chkfpregs_error
	cmp.l		(%a0)+,(%a1)+
	bne.l		chkfpregs_error

	clr.l		%d0
	rts

chkfpregs_error:
	movq.l		&0x1,%d0
	rts

DEF_REGS:
	long		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac
	long		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac

	long		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac
	long		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac

DEF_FPREGS:
	long		0x7fff0000, 0xffffffff, 0xffffffff
	long		0x7fff0000, 0xffffffff, 0xffffffff
	long		0x7fff0000, 0xffffffff, 0xffffffff
	long		0x7fff0000, 0xffffffff, 0xffffffff
	long		0x7fff0000, 0xffffffff, 0xffffffff
	long		0x7fff0000, 0xffffffff, 0xffffffff
	long		0x7fff0000, 0xffffffff, 0xffffffff
	long		0x7fff0000, 0xffffffff, 0xffffffff

DEF_FPCREGS:
	long		0x00000000, 0x00000000, 0x00000000

############################################################

_print_str:
	mov.l		%d0,-(%sp)
	mov.l		(TESTTOP-0x80+0x0,%pc),%d0
	pea		(TESTTOP-0x80,%pc,%d0)
	mov.l		0x4(%sp),%d0
	rtd		&0x4

_print_num:
	mov.l		%d0,-(%sp)
	mov.l		(TESTTOP-0x80+0x4,%pc),%d0
	pea		(TESTTOP-0x80,%pc,%d0)
	mov.l		0x4(%sp),%d0
	rtd		&0x4

############################################################
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MOTOROLA MICROPROCESSOR & MEMORY TECHNOLOGY GROUP
M68000 Hi-Performance Microprocessor Division
M68060 Software Package
Production Release P1.00 -- October 10, 1994

M68060 Software Package Copyright © 1993, 1994 Motorola Inc.  All rights reserved.

THE SOFTWARE is provided on an "AS IS" basis and without warranty.
To the maximum extent permitted by applicable law,
MOTOROLA DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
and any warranty against infringement with regard to the SOFTWARE
(INCLUDING ANY MODIFIED VERSIONS THEREOF) and any accompanying written materials.

To the maximum extent permitted by applicable law,
IN NO EVENT SHALL MOTOROLA BE LIABLE FOR ANY DAMAGES WHATSOEVER
(INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS)
ARISING OF THE USE OR INABILITY TO USE THE SOFTWARE.
Motorola assumes no responsibility for the maintenance and support of the SOFTWARE.

You are hereby granted a copyright license to use, modify, and distribute the SOFTWARE
so long as this entire notice is retained without alteration in any modified and/or
redistributed versio