+SIGCONTEXT_OFF;							\
	.vframesp SP_OFF+SIGCONTEXT_OFF

GLOBAL_ENTRY(__kernel_sigtramp)
	// describe the state that is active when we get here:
	.prologue
	SIGTRAMP_SAVES
	.body

	.label_state 1

	adds base0=SIGHANDLER_OFF,sp
	adds base1=RBS_BASE_OFF+SIGCONTEXT_OFF,sp
	br.call.sptk.many rp=1f
1:
	ld8 r17=[base0],(ARG0_OFF-SIGHANDLER_OFF)	// get pointer to signal handler's plabel
	ld8 r15=[base1]					// get address of new RBS base (or NULL)
	cover				// push args in interrupted frame onto backing store
	;;
	cmp.ne p1,p0=r15,r0		// do we need to switch rbs? (note: pr is saved by kernel)
	mov.m r9=ar.bsp			// fetch ar.bsp
	.spillsp.p p1, ar.rnat, RNAT_OFF+SIGCONTEXT_OFF
(p1)	br.cond