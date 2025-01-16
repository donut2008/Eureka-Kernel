 r12=[temp1],16	// sal_ra
	ld8 r9=[temp2],16	// sal_gp
	;;
	ld8 r22=[temp1],16	// pal_min_state, virtual
	ld8 r13=[temp2],16	// prev_IA64_KR_CURRENT
	;;
	ld8 r16=[temp1],16	// prev_IA64_KR_CURRENT_STACK
	ld8 r20=[temp2],16	// prev_task
	;;
	ld8 temp3=[temp1],16	// cr.isr
	ld8 temp4=[temp2],16	// cr.ifa
	;;
	mov cr.isr=temp3
	mov cr.ifa=temp4
	ld8 temp3=[temp1],16	// cr.itir
	ld8 temp4=[temp2],16	// cr.iipa
	;;
	mov cr.itir=temp3
	mov cr.iipa=temp4
	ld8 temp3=[temp1]	// cr.iim
	ld8 temp4=[temp2]		// cr.iha
	add temp1=SOS(OS_STATUS), regs
	add temp2=SOS(CONTEXT), regs
	;;
	mov cr.iim=temp3
	mov cr.iha=temp4
	dep r22=0,r22,62,1	// pal_min_state, physical, uncached
	mov IA64_KR(CURRENT)=r13
	ld8 r8=[temp1]		// os_status
	ld8 r10=[temp2]		// context

	/* Wire IA64_TR_CURRENT_STACK to the stack that we are resuming to.  To
	 * avoid any dependencies on the algorithm in ia64_switch_to(), just
	 * purge any existing CURRENT_STACK mapping and insert the new one.
	 *
	 * r16 contains prev_IA64_KR_CURRENT_STACK, r13 contains
	 * prev_IA64_KR_CURRENT, these values may have been changed by the C
	 * code.  Do not use r8, r9, r10, r22, they contain values ready for
	 * the return to SAL.
	 */

	mov r15=IA64_KR(CURRENT_STACK)		// physical granule mapped by IA64_TR_CURRENT_STACK
	;;
	shl r15=r15,IA64_GRANULE_SHIFT
	;;
	dep r15=-1,r15,61,3			// virtual granule
	mov r18=IA64_GRANULE_SHIFT<<2		// for cr.itir.ps
	;;
	ptr.d r15,r18
	;;
	srlz.d

	extr.u r19=r13,61,3			// r13 = prev_IA64_KR_CURRENT
	shl r20=r16,IA64_GRANULE_SHIFT		// r16 = prev_IA64_KR_CURRENT_STACK
	movl r21=PAGE_KERNEL			// page properties
	;;
	mov IA64_KR(CURRENT_STACK)=r16
	cmp.ne p6,p0=RGN_KERNEL,r19		// new stack is in the kernel region?
	or r21=r20,r21				// construct PA | page properties
(p6)	br.spnt 1f				// the dreaded cpu 0 idle task in region 5:(
	;;
	mov cr.itir=r18
	mov cr.ifa=r13
	mov r20=IA64_TR_CURRENT_STACK
	;;
	itr.d dtr[r20]=r21
	;;
	srlz.d
1:

	br.sptk b0

//EndStub//////////////////////////////////////////////////////////////////////


//++
// Name:
//	ia64_new_stack()
//
// Stub Description:
//
//	Switch to the MCA/INIT stack.
//
//	r2 contains the return address, r3 contains either
//	IA64_MCA_CPU_MCA_STACK_OFFSET or IA64_MCA_CPU_INIT_STACK_OFFSET.
//
//	On entry RBS is still on the original stack, this routine switches RBS
//	to use the MCA/INIT stack.
//
//	On entry, sos->pal_min_state is physica