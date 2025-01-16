pcr,%fpsr,%fpiar,IFPCREGS(%a6)

	fmov.b		&0x2,%fp0
	mov.l		&0x7ffe0000,DATA+0x0(%a6)
	mov.l		&0x80000000,DATA+0x4(%a6)
	mov.l		&0x00000000,DATA+0x8(%a6)

	mov.w		&0x0000,%cc
ovfl_nm_0_pc:
	fmul.x		DATA(%a6),%fp0

	mov.w		%cc,SCCR(%a6)
	movm.l		&0x7fff,SREGS(%a6)
	fmovm.x		&0xff,SFPREGS(%a6)
	fmovm.l		%fpcr,%fpsr,%fpiar,SFPCREGS(%a6)

	mov.l		&0x7fff0000,IFPREGS+0x0(%a6)
	mov.l		&0x00000000,IFPREGS+0x4(%a6)
	mov.l		&0x00000000,IFPREGS+0x8(%a6)
	mov.l		&0x02001048,