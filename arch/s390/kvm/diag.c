FPCREGS(%a6)
	mov.w		&0xffff,IREGS+28+2(%a6)
	mov.l		&0x0f008080,IFPCREGS+0x4(%a6)
	lea		unimp_4_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

# ftrapcc
unimp_5:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	fmov.l		&0x0f000000,%fpsr

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.w		&0x0000,%cc
unimp_5_pc:
	ftpgt.l		&0xabcdef01

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)
	mov.l		&0x0f008080,IFPCREGS+0x4(%a6)
	lea		unimp_5_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

	clr.l		%d0
	rts

#############################################

effadd_str:
	string		"\tUnimplemented <ea>..."

	align		0x4
effadd_0:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	fmov.b		&0x2,%fp0

	mov.w		&0x0000,%cc
effadd_0_pc:
	fmul.x		&0xc00000008000000000000000,%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0xc0010000,IFPREGS+0x0(%a6)
	mov.l		&0x80000000,IFPREGS+0x4(%a6)
	mov.l		&0x00000000,IFPREGS+0x8(%a6)
	mov.l		&0x08000000,IFPCREGS+0x4(%a6)
	lea		effadd_0_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

effadd_1:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.w		&0x0000,%cc
effadd_1_pc:
	fabs.p		&0xc12300012345678912345678,%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0x3e660000,IFPREGS+0x0(%a6)
	mov.l		&0xd0ed23e8,IFPREGS+0x4(%a6)
	mov.l		&0xd14035bc,IFPREGS+0x8(%a6)
	mov.l		&0x00000108,IFPCREGS+0x4(%a6)
	lea		effadd_1_pc(%pc),%a0
	mov.l		%a0,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

fmovml_0:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	mov.w		&0x0000,%cc
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	fmovm.l		&0xffffffffffffffff,%fpcr,%fpsr

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)
	mov.l		&0x0000fff0,IFPCREGS+0x0(%a6)
	mov.l		&0x0ffffff8,IFPCREGS+0x4(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

fmovml_1:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	mov.w		&0x0000,%cc
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	fmovm.l		&0xffffffffffffffff,%fpcr,%fpiar

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)
	mov.l		&0x0000fff0,IFPCREGS+0x0(%a6)
	mov.l		&0xffffffff,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

fmovml_2:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	mov.w		&0x0000,%cc
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	fmovm.l		&0xffffffffffffffff,%fpsr,%fpiar

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)
	mov.l		&0x0ffffff8,IFPCREGS+0x4(%a6)
	mov.l		&0xffffffff,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

fmovml_3:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	mov.w		&0x0000,ICCR(%a6)
	mov.w		&0x0000,%cc
	movm.l		&0x7fff,IREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	fmovm.l		&0xffffffffffffffffffffffff,%fpcr,%fpsr,%fpiar

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)
	mov.l		&0x0000fff0,IFPCREGS+0x0(%a6)
	mov.l		&0x0ffffff8,IFPCREGS+0x4(%a6)
	mov.l		&0xffffffff,IFPCREGS+0x8(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

# fmovmx dynamic
fmovmx_0:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	fmov.b		&0x1,%fp0
	fmov.b		&0x2,%fp1
	fmov.b		&0x3,%fp2
	fmov.b		&0x4,%fp3
	fmov.b		&0x5,%fp4
	fmov.b		&0x6,%fp5
	fmov.b		&0x7,%fp6
	fmov.b		&0x8,%fp7

	fmov.l		&0x0,%fpiar
	mov.l		&0xffffffaa,%d0

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0xffff,IREGS(%a6)

	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)

	mov.w		&0x0000,%cc

	fmovm.x		%d0,-(%sp)

	mov.w		%cc,SCCR(%a6)

	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	fmov.s		&0x7f800000,%fp1
	fmov.s		&0x7f800000,%fp3
	fmov.s		&0x7f800000,%fp5
	fmov.s		&0x7f800000,%fp7

	fmov.x		(%sp)+,%fp1
	fmov.x		(%sp)+,%fp3
	fmov.x		(%sp)+,%fp5
	fmov.x		(%sp)+,%fp7

	movm.l		&0xffff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

fmovmx_1:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	fmov.b		&0x1,%fp0
	fmov.b		&0x2,%fp1
	fmov.b		&0x3,%fp2
	fmov.b		&0x4,%fp3
	fmov.b		&0x5,%fp4
	fmov.b		&0x6,%fp5
	fmov.b		&0x7,%fp6
	fmov.b		&0x8,%fp7

	fmov.x		%fp6,-(%sp)
	fmov.x		%fp4,-(%sp)
	fmov.x		%fp2,-(%sp)
	fmov.x		%fp0,-(%sp)

	fmovm.x		&0xff,IFPREGS(%a6)

	fmov.s		&0x7f800000,%fp6
	fmov.s		&0x7f800000,%fp4
	fmov.s		&0x7f800000,%fp2
	fmov.s		&0x7f800000,%fp0

	fmov.l		&0x0,%fpiar
	fmov.l		&0x0,%fpsr
	mov.l		&0xffffffaa,%d0

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0xffff,IREGS(%a6)

	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)

	mov.w		&0x0000,%cc

	fmovm.x		(%sp)+,%d0

	mov.w		%cc,SCCR(%a6)

	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	movm.l		&0xffff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)

	bsr.l		chkregs
	tst.b		%d0
	bne.l		error

	bsr.l		chkfpregs
	tst.b		%d0
	bne.l		error

fmovmx_2:
	addq.l		&0x1,TESTCTR(%a6)

	movm.l		DEF_REGS(%pc),&0x3fff
	fmovm.x		DEF_FPREGS(%pc),&0xff
	fmovm.l		DEF_FPCREGS(%pc),%fpcr,%fpsr,%fpiar

	fmov.b		&0x1,%fp0
	fmov.b		&0x2,%fp1
	fmov.b		&0x3,%fp2
	fmov.b		&0x4,%fp3
	fmov.b		&0x5,%fp4
	fmov.b		&0x6,%fp5
	fmov.b		&0x7,%fp6
	fmov.b		&0x8,%fp7

	fmov.l		&0x0,%fpiar
	mov.l		&0xffffff00,%d0

	mov.w		&0x0000,ICCR(%a6)
	movm.l		&0xffff,IREGS(%a6)

	fmovm.l		%fpcr,%fpsr,%fpiar,IFPCREGS(%a6)
	fmovm.x		&0xff,IFPREGS(%a6)

	mov.w		&0x0000,%cc

	fmovm.x		%d0,-(%sp)

	mov.w		%cc,SCCR(%a6)

	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	movm.l		&0xffff,SREGS(%a6)
	fmovm.x