 * Below again cribbed liberally from older version. Do not lean
 * heavily on it.
 */
#define IBA7322_IBC_DLIDLMC_SHIFT QIB_7322_IBCCtrlB_0_IB_DLID_LSB
#define IBA7322_IBC_DLIDLMC_MASK (QIB_7322_IBCCtrlB_0_IB_DLID_RMASK \
	| (QIB_7322_IBCCtrlB_0_IB_DLID_MASK_RMASK << 16))

static int qib_7322_set_ib_cfg(struct qib_pportdata *ppd, int which, u32 val)
{
	struct qib_devdata *dd = ppd->dd;
	u64 maskr; /* right-justified mask */
	int lsb, ret = 0;
	u16 lcmd, licmd;
	unsigned long flags;

	switch (which) {
	case QIB_IB_CFG_LIDLMC:
		/*
		 * Set LID and LMC. Combined to avoid possible hazard
		 * caller puts LMC in 16MSbits, DLID in 16LSbits of val
		 */
		lsb = IBA7322_IBC_DLIDLMC_SHIFT;
		maskr = IBA7322_IBC_DLIDLMC_MASK;
		/*
		 * For header-checking, the SLID in the packet will
		 * be masked with SendIBSLMCMask, and compared
		 * with SendIBSLIDAssignMask. Make sure we do not
		 * set any bits not covered by the mask, or we get
		 * false-positives.
		 */
		qib_write_kreg_port(ppd, krp_sendslid,
				    val & (val >> 16) & SendIBSLIDAssignMask);
		qib_write_kreg_port(ppd, krp_sendslidmask,
				    (val >> 16) & SendIBSLMCMask);
		break;

	case QIB_IB_CFG_LWID_ENB: /* set allowed Link-width */
		ppd->link_width_enabled = val;
		/* convert IB value to chip register value */
		if (val == IB_WIDTH_1X)
			val = 0;
		else if (val == IB_WIDTH_4X)
			val = 1;
		else
			val = 3;
		maskr = SYM_RMASK(IBCCtrlB_0, IB_NUM_CHANNELS);
		lsb = SYM_LSB(IBCCtrlB_0, IB_NUM_CHANNELS);
		break;

	case QIB_IB_CFG_SPD_ENB: /* set allowed Link speeds */
		/*
		 * As with width, only write the actual register if the
		 * link is currently down, otherwise takes effect on next
		 * link change.  Since setting is being explicitly requested
		 * (via MAD or sysfs), clear autoneg failure status if speed
		 * autoneg is enabled.
		 */
		ppd->link_speed_enabled = val;
		val <<= IBA7322_IBC_SPEED_LSB;
		maskr = IBA7322_IBC_SPEED_MASK | IBA7322_IBC_IBTA_1_2_MASK |
			IBA7322_IBC_MAX_SPEED_MASK;
		if (val & (val - 1)) {
			/* Muliple speeds enabled */
			val |= IBA7322_IBC_IBTA_1_2_MASK |
				IBA7322_IBC_MAX_SPEED_MASK;
			spin_lock_irqsave(&ppd->lflags_lock, flags);
			ppd->lflags &= ~QIBL_IB_AUTONEG_FAILED;
			spin_unlock_irqrestore(&ppd->lflags_lock, flags);
		} else if (val & IBA7322_IBC_SPEED_QDR)
			val |= IBA7322_IBC_IBTA_1_2_MASK;
		/* IBTA 1.2 mode + min/max + speed bits are contiguous */
		lsb = SYM_LSB(IBCCtrlB_0, IB_ENHANCED_MODE);
		break;

	case QIB_IB_CFG_RXPOL_ENB: /* set Auto-RX-polarity enable */
		lsb = SYM_LSB(IBCCtrlB_0, IB_POLARITY_REV_SUPP);
		maskr = SYM_RMASK(IBCCtrlB_0, IB_POLARITY_REV_SUPP);
		break;

	case QIB_IB_CFG_LREV_ENB: /* set Auto-Lane-reversal enable */
		lsb = SYM_LSB(IBCCtrlB_0, IB_LANE_REV_SUPPORTED);
		maskr = SYM_RMASK(IBCCtrlB_0, IB_LANE_REV_SUPPORTED);
		break;

	case QIB_IB_CFG_OVERRUN_THRESH: /* IB overrun threshold */
		maskr = SYM_FIELD(ppd->cpspec->ibcctrl_a, IBCCtrlA_0,
				  OverrunThreshold);
		if (maskr != val) {
			ppd->cpspec->ibcctrl_a &=
				~SYM_MASK(IBCCtrlA_0, OverrunThreshold);
			ppd->cpspec->ibcctrl_a |= (u64) val <<
				SYM_LSB(IBCCtrlA_0, OverrunThreshold);
			qib_write_kreg_port(ppd, krp_ibcctrl_a,
					    ppd->cpspec->ibcctrl_a);
			qib_write_kreg(dd, kr_scratch, 0ULL);
		}
		goto bail;

	case QIB_IB_CFG_PHYERR_THRESH: /* IB PHY error threshold */
		maskr = SYM_FIELD(ppd->cpspec->ibcctrl_a, IBCCtrlA_0,
				  PhyerrThreshold);
		if (maskr != val) {
			ppd->cpspec->ibcctrl_a &=
				~SYM_MASK(IBCCtrlA_0, PhyerrThreshold);
			ppd->cpspec->ibcctrl_a |= (u64) val <<
				SYM_LSB(IBCCtrlA_0, PhyerrThreshold);
			qib_write_kreg_port(ppd, krp_ibcctrl_a,
					    ppd->cpspec->ibcctrl_a);
			qib_write_kreg(dd, kr_scratch, 0ULL);
		}
		goto bail;

	case QIB_IB_CFG_PKEYS: /* update pkeys */
		maskr = (u64) ppd->pkeys[0] | ((u64) ppd->pkeys[1] << 16) |
			((u64) ppd->pkeys[2] << 32) |
			((u64) ppd->pkeys[3] << 48);
		qib_write_kreg_port(ppd, krp_partitionkey, maskr);
		goto bail;

	case QIB_IB_CFG_LINKDEFAULT: /* IB link default (sleep/poll) */
		/* will only take effect when the link state changes */
		if (val == IB_LINKINITCMD_POLL)
			ppd->cpspec->ibcctrl_a &=
				~SYM_MASK(IBCCtrlA_0, LinkDownDefaultState);
		else /* SLEEP */
			ppd->cpspec->ibcctrl_a |=
				SYM_MASK(IBCCtrlA_0, LinkDownDefaultState);
		qib_write_kreg_port(ppd, krp_ibcctrl_a, ppd->cpspec->ibcctrl_a);
		qib_write_kreg(dd, kr_scratch, 0ULL);
		goto bail;

	case QIB_IB_CFG_MTU: /* update the MTU in IBC */
		/*
		 * Update our housekeeping variables, and set IBC max
		 * size, same as init code; max IBC is max we allow in
		 * buffer, less the qword pbc, plus 1 for ICRC, in dwords
		 * Set even if it's unchanged, print debug message only
		 * on changes.
		 */
		val = (ppd->ibmaxlen >> 2) + 1;
		ppd->cpspec->ibcctrl_a &= ~SYM_MASK(IBCCtrlA_0, MaxPktLen);
		ppd->cpspec->ibcctrl_a |= (u64)val <<
			SYM_LSB(IBCCtrlA_0, MaxPktLen);
		qib_write_kreg_port(ppd, krp_ibcctrl_a,
				    ppd->cpspec->ibcctrl_a);
		qib_write_kreg(dd, kr_scratch, 0ULL);
		goto bail;

	case QIB_IB_CFG_LSTATE: /* set the IB link state */
		switch (val & 0xffff0000) {
		case IB_LINKCMD_DOWN:
			lcmd = QLOGIC_IB_IBCC_LINKCMD_DOWN;
			ppd->cpspec->ibmalfusesnap = 1;
			ppd->cpspec->ibmalfsnap = read_7322_creg32_port(ppd,
				crp_errlink);
			if (!ppd->cpspec->ibdeltainprog &&
			    qib_compat_ddr_negotiate) {
				ppd->cpspec->ibdeltainprog = 1;
				ppd->cpspec->ibsymsnap =
					read_7322_creg32_port(ppd,
							      crp_ibsymbolerr);
				ppd->cpspec->iblnkerrsnap =
					read_7322_creg32_port(ppd,
						      crp_iblinkerrrecov);
			}
			break;

		case IB_LINKCMD_ARMED:
			lcmd = QLOGIC_IB_IBCC_LINKCMD_ARMED;
			if (ppd->cpspec->ibmalfusesnap) {
				ppd->cpspec->ibmalfusesnap = 0;
				ppd->cpspec->ibmalfdelta +=
					read_7322_creg32_port(ppd,
							      crp_errlink) -
					ppd->cpspec->ibmalfsnap;
			}
			break;

		case IB_LINKCMD_ACTIVE:
			lcmd = QLOGIC_IB_IBCC_LINKCMD_ACTIVE;
			break;

		default:
			ret = -EINVAL;
			qib_dev_err(dd, "bad linkcmd req 0x%x\n", val >> 16);
			goto bail;
		}
		switch (val & 0xffff) {
		case IB_LINKINITCMD_NOP:
			licmd = 0;
			break;

		case IB_LINKINITCMD_POLL:
			licmd = QLOGIC_IB_IBCC_LINKINITCMD_POLL;
			break;

		case IB_LINKINITCMD_SLEEP:
			licmd = QLOGIC_IB_IBCC_LINKINITCMD_SLEEP;
			break;

		case IB_LINKINITCMD_DISABLE:
			licmd = QLOGIC_IB_IBCC_LINKINITCMD_DISABLE;
			ppd->cpspec->chase_end = 0;
			/*
			 * stop state chase counter and timer, if running.
			 * wait forpending timer, but don't clear .data (ppd)!
			 */
			if (ppd->cpspec->chase_timer.expires) {
				del_timer_sync(&ppd->cpspec->chase_timer);
				ppd->cpspec->chase_timer.expires = 0;
			}
			break;

		default:
			ret = -EINVAL;
			qib_dev_err(dd, "bad linkinitcmd req 0x%x\n",
				    val & 0xffff);
			goto bail;
		}
		qib_set_ib_7322_lstate(ppd, lcmd, licmd);
		goto bail;

	case QIB_IB_CFG_OP_VLS:
		if (ppd->vls_operational != val) {
			ppd->vls_operational = val;
			set_vls(ppd);
		}
		goto bail;

	case QIB_IB_CFG_VL_HIGH_LIMIT:
		qib_write_kreg_port(ppd, krp_highprio_limit, val);
		goto bail;

	case QIB_IB_CFG_HRTBT: /* set Heartbeat off/enable/auto */
		if (val > 3) {
			ret = -EINVAL;
			goto bail;
		}
		lsb = IBA7322_IBC_HRTBT_LSB;
		maskr = IBA7322_IBC_HRTBT_RMASK; /* OR of AUTO and ENB */
		break;

	case QIB_IB_CFG_PORT:
		/