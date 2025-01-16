;
	sub r14=r14,r17		// r14 (bspstore1) <- bsp1 - (sc_loadrs >> 16)
	shr.u r17=r17,3		// r17 <- (sc_loadrs >> 19)
	;;
	loadrs			// restore dirty partition
	extr.u r14=r14,3,6	// r14 <- rse_slot_num(bspstore1)
	;;
	add r14=r14,r17		// r14 <- rse_slot_num(bspstore1) + (sc_loadrs >> 19)
	;;
	shr.u r14=r14,6		// r14 <- (rse_slot_num(bspstore1) + (sc_loadrs >> 19))/0x40
	;;
	sub r14=r14,r17		// r14 <- -rse_num_regs(bspstore1, bsp1)
	movl r17=0x8208208208208209
	;;
	add r18=r18,r14		// r18 (delta) <- rse_slot_num(bsp0) - rse_num_regs(bspstore1,bsp1)
	setf.sig f7=r17
	cmp.lt p7,p0=r14,r0	// p7 <- (r14 < 0)?
	;;
(p7)	adds r18=-62,r18	// delta -= 62
	;;
	setf.sig f6=r18
	;;
