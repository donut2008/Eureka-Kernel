endctrl writes, so write
 * the scratch register after writing sendctrl.
 *
 * Which register is written depends on the operation.
 * Most operate on the common register, while
 * SEND_ENB and SEND_DIS operate on the per-port ones.
 * SEND_ENB is included in common because it can change SPCL_TRIG
 */
#define SENDCTRL_COMMON_MODS (\
	QIB_SENDCTRL_CLEAR | \
	QIB_SENDCTRL_AVAIL_DIS | \
	QIB_SENDCTRL_AVAIL_ENB | \
	QIB_SENDCTRL_AVAIL_BLIP | \
	QIB_SENDCTRL_DISARM | \
	QIB_SENDCTRL_DISARM_ALL | \
	QIB_SENDCTRL_SEND_ENB)

#define SENDCTRL_PORT_MODS (\
	QIB_SENDCTRL_CLEAR | \
	QIB_SENDCTRL_SEND_ENB | \
	QIB_SENDCTRL_SEND_DIS | \
	QIB_SENDCTRL_FLUSH)

static void sendctrl_7322_mod(struct qib_pportdata *ppd, u32 op)
{
	struct qib_devdata *dd = ppd->dd;
	u64 tmp_dd_sendctrl;
	unsigned long flags;

	spin_lock_irqsave(&dd->sendctrl_lock, flags);

	/* First the dd ones that are "sticky", saved in shadow */
	if (op & QIB_SENDCTRL_CLEAR)
		dd->sendctrl = 0;
	if (op & QIB_SENDCTRL_AVAIL_DIS)
		dd->sendctrl &= ~SYM_MASK(SendCtrl, SendBufAvailUpd);
	else if (op & QIB_SENDCTRL_AVAIL_ENB) {
		dd->sendctrl |= SYM_MASK(SendCtrl, SendBufAvailUpd);
		if (dd->flags & QIB_USE_SPCL_TRIG)
			dd->sendctrl |= SYM_MASK(SendCtrl, SpecialTriggerEn);
	}

	/* Then the ppd ones that are "sticky", saved in shadow */
	if (op & QIB_SENDCTRL_SEND_DIS)
		ppd->p_sendctrl &= ~SYM_MASK(SendCtrl_0, SendEnable);
	else if (op & QIB_SENDCTRL_SEND_ENB)
		ppd->p_sendctrl |= SYM_MASK(SendCtrl_0, SendEnable);

	if (op & QIB_SENDCTRL_DISARM_ALL) {
		u32 i, last;

		tmp_dd_sendctrl = dd->sendctrl;
		last = dd->piobcnt2k + dd->piobcnt4k + NUM_VL15_BUFS;
		/*
		 * Disarm any buffers that are not yet launched,
		 * disabling updates until done.
		 */
		tmp_dd_sendctrl &= ~SYM_MASK(SendCtrl, SendBufAvailUpd);
		for (i = 0; i < last; i++) {
			qib_write_kreg(dd, kr_sendctrl,
				       tmp_dd_sendctrl |
				       SYM_MASK(SendCtrl, Disarm) | i);
			qib_write_kreg(dd, kr_scratch, 0);
		}
	}

	if (op & QIB_SENDCTRL_FLUSH) {
		u64 tmp_ppd_sendctrl = ppd->p_sendctrl;

		/*
		 * Now drain all the fifos.  The Abort bit should never be
		 * needed, so for now, at least, we don't use it.
		 */
		tmp_ppd_sendctrl |=
			SYM_MASK(SendCtrl_0, TxeDrainRmFifo) |
			SYM_MASK(SendCtrl_0, TxeDrainLaFifo) |
			SYM_MASK(SendCtrl_0, TxeBypassIbc);
		qib_write_kreg_port(ppd, krp_sendctrl, tmp_ppd_sendctrl);
		qib_write_kreg(dd, kr_scratch, 0);
	}

	tmp_dd_sendctrl = dd->sendctrl;

	if (op & QIB_SENDCTRL_DISARM)
		tmp_dd_sendctrl |= SYM_MASK(SendCtrl, Disarm) |
			((op & QIB_7322_SendCtrl_DisarmSendBuf_RMASK) <<
			 SYM_LSB(SendCtrl, DisarmSendBuf));
	if ((op & QIB_SENDCTRL_AVAIL_BLIP) &&
	    (dd->sendctrl & SYM_MASK(SendCtrl, SendBufAvailUpd)))
		tmp_dd_sendctrl &= ~SYM_MASK(SendCtrl, SendBufAvailUpd);

	if (op == 0 || (op & SENDCTRL_COMMON_MODS)) {
		qib_write_kreg(dd, kr_sendctrl, tmp_dd_sendctrl);
		qib_write_kreg(dd, kr_scratch, 0);
	}

	if (op == 0 || (op & SENDCTRL_PORT_MODS)) {
		qib_write_kreg_port(ppd, krp_sendctrl, ppd->p_sendctrl);
		qib_write_kreg(dd, kr_scratch, 0);
	}

	if (op & QIB_SENDCTRL_AVAIL_BLIP) {
		qib_write_kreg(dd, kr_sendctrl, dd->sendctrl);
		qib_write_kreg(dd, kr_scratch, 0);
	}

	spin_unlock_irqrestore(&dd->sendctrl_lock, flags);

	if (op & QIB_SENDCTRL_FLUSH) {
		u32 v;
		/*
		 * ensure writes have hit chip, then do a few
		 * more reads, to allow DMA of pioavail registers
		 * to occur, so in-memory copy is in sync with
		 * the chip.  Not always safe to sleep.
		 */
		v = qib_read_kreg32(dd, kr_scratch);
		qib_write_kreg(dd, kr_scratch, v);
		v = qib_read_kreg32(dd, kr_scratch);
		qib_write_kreg(dd, kr_scratch, v);
		qib_read_kreg32(dd, kr_scratch);
	}
}

#define _PORT_VIRT_FLAG 0x8000U /* "virtual", need adjustments */
#define _PORT_64BIT_FLAG 0x10000U /* not "virtual", but 64bit */
#define _PORT_CNTR_IDXMASK 0x7fffU /* mask off flags above */

/**
 * qib_portcntr_7322 - read a per-port chip counter
 * @ppd: the qlogic_ib pport
 * @creg: the counter to read (not a chip offset)
 */
static u64 qib_portcntr_7322(struct qib_pportdata *ppd, u32 reg)
{
	struct qib_devdata *dd = ppd->dd;
	u64 ret = 0ULL;
	u16 creg;
	/* 0xffff for unimplemented or synthesized counters */
	static const u32 xlator[] = {
		[QIBPORTCNTR_PKTSEND] = crp_pktsend | _PORT_64BIT_FLAG,
		[QIBPORTCNTR_WORDSEND] = crp_wordsend | _PORT_64BIT_FLAG,
		[QIBPORTCNTR_PSXMITDATA] = crp_psxmitdatacount,
		[QIBPORTCNTR_PSXMITPKTS] = crp_psxmitpktscount,
		[QIBPORTCNTR_PSXMITWAIT] = crp_psxmitwaitcount,
		[QIBPORTCNTR_SENDSTALL] = crp_sendstall,
		[QIBPORTCNTR_PKTRCV] = crp_pktrcv | _PORT_64BIT_FLAG,
		[QIBPORTCNTR_PSRCVDATA] = crp_psrcvdatacount,
		[QIBPORTCNTR_PSRCVPKTS] = crp_psrcvpktscount,
		[QIBPORTCNTR_RCVEBP] = crp_rcvebp,
		[QIBPORTCNTR_RCVOVFL] = crp_rcvovfl,
		[QIBPORTCNTR_WORDRCV] = crp_wordrcv | _PORT_64BIT_FLAG,
		[QIBPORTCNTR_RXDROPPKT] = 0xffff, /* not needed  for 7322 */
		[QIBPORTCNTR_RXLOCALPHYERR] = crp_rxotherlocalphyerr,
		[QIBPORTCNTR_RXVLERR] = crp_rxvlerr,
		[QIBPORTCNTR_ERRICRC] = crp_erricrc,
		[QIBPORTCNTR_ERRVCRC] = crp_errvcrc,
		[QIBPORTCNTR_ERRLPCRC] = crp_errlpcrc,
		[QIBPORTCNTR_BADFORMAT] = crp_badformat,
		[QIBPORTCNTR_ERR_RLEN] = crp_err_rlen,
		[QIBPORTCNTR_IBSYMBOLERR] = crp_ibsymbolerr,
		[QIBPORTCNTR_INVALIDRLEN] = crp_invalidrlen,
		[QIBPORTCNTR_UNSUPVL] = crp_txunsupvl,
		[QIBPORTCNTR_EXCESSBUFOVFL] = crp_excessbufferovfl,
		[QIBPORTCNTR_ERRLINK] = crp_errlink,
		[QIBPORTCNTR_IBLINKDOWN] = crp_iblinkdown,
		[QIBPORTCNTR_IBLINKERRRECOV] = crp_iblinkerrrecov,
		[QIBPORTCNTR_LLI] = crp_locallinkintegrityerr,
		[QIBPORTCNTR_VL15PKTDROP] = crp_vl15droppedpkt,
		[QIBPORTCNTR_ERRPKEY] = crp_errpkey,
		/*
		 * the next 3 aren't really counters, but were implemented
		 * as counters in older chips, so still get accessed as
		 * though they were counters from this code.
		 */
		[QIBPORTCNTR_PSINTERVAL] = krp_psinterval,
		[QIBPORTCNTR_PSSTART] = krp_psstart,
		[QIBPORTCNTR_PSSTAT] = krp_psstat,
		/* pseudo-counter, summed for all ports */
		[QIBPORTCNTR_KHDROVFL] = 0xffff,
	};

	if (reg >= ARRAY_SIZE(xlator)) {
		qib_devinfo(ppd->dd->pcidev,
			 "Unimplemented p