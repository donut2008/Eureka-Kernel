0, H(15)}, /* IOB8ST Recording */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFC8S, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_IOB8ST, 0, 0, H(16)}, /* IOB8ST  */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFC8S, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_IOB8ST_1, 0, 0, H(17)}, /* IOB8ST  */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFC8S, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_HFC8S, 0, 0, H(18)}, /* 8S */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFC8S, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_OV8S, 0, 0, H(30)}, /* OpenVox 8 */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFC8S, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_JH8S, 0, 0, H(32)}, /* Junganns 8S  */


	/* Cards with HFC-E1 Chip */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFCE1, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_BNE1, 0, 0, H(19)}, /* BNE1 */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFCE1, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_BNE1M, 0, 0, H(20)}, /* BNE1 mini PCI */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFCE1, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_BNE1DP, 0, 0, H(21)}, /* BNE1 + (Dual) */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFCE1, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_BNE1D, 0, 0, H(22)}, /* BNE1 (Dual) */

	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFCE1, PCI_VENDOR_ID_CCD,
	  PCI_DEVICE_ID_CCD_HFCE1, 0, 0, H(23)}, /* Old Eval */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFCE1, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_IOB1E1, 0, 0, H(24)}, /* IOB1E1 */
	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFCE1, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_HFCE1, 0, 0, H(25)}, /* E1 */

	{ PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX_9030, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_SPD4S, 0, 0, H(26)}, /* PLX PCI Bridge */
	{ PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX_9030, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_SPDE1, 0, 0, H(27)}, /* PLX PCI Bridge */

	{ PCI_VENDOR_ID_CCD, PCI_DEVICE_ID_CCD_HFCE1, PCI_VENDOR_ID_CCD,
	  PCI_SUBDEVICE_ID_CCD_JHSE1, 0, 0, H(25)}, /* Junghanns E1 */

	{ PCI_VDEVICE(CCD, PCI_DEVICE_ID_CCD_HFC4S), 0 },
	{ PCI_VDEVICE(CCD, PCI_DEVICE_ID_CCD_HFC8S), 0 },
	{ PCI_VDEVICE(CCD, PCI_DEVICE_ID_CCD_HFCE1), 0 },
	{0, }
};
#undef H

MODULE_DEVICE_TABLE(pci, hfmultipci_ids);

static int
hfcmulti_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct hm_map	*m = (struct hm_map *)ent->driver_data;
	int		ret;

	if (m == NULL && ent->vendor == PCI_VENDOR_ID_CCD && (
		    ent->device == PCI_DEVICE_ID_CCD_HFC4S ||
		    ent->device == PCI_DEVICE_ID_CCD_HFC8S ||
		    ent->device == PCI_DEVICE_ID_CCD_HFCE1)) {
		printk(KERN_ERR
		       "Unknown HFC multiport controller (vendor:%04x device:%04x "
		       "subvendor:%04x subdevice:%04x)\n", pdev->vendor,
		       pdev->device, pdev->subsystem_vendor,
		       pdev->subsystem_device);
		printk(KERN_ERR
		       "Please contact the driver maintainer for support.\n");
		return -ENODEV;
	}
	ret = hfcmulti_init(m, pdev, ent);
	if (ret)
		return ret;
	HFC_cnt++;
	printk(KERN_INFO "%d devices registered\n", HFC_cnt);
	return 0;
}

static struct pci_driver hfcmultipci_driver = {
	.name		= "hfc_multi",
	.probe		= hfcmulti_probe,
	.remove		= hfc_remove_pci,
	.id_table	= hfmultipci_ids,
};

static void __exit
HFCmulti_cleanup(void)
{
	struct hfc_multi *card, *next;

	/* get rid of all devices of this driver */
	list_for_each_entry_safe(card, next, &HFClist, list)
		release_card(card);
	pci_unregister_driver(&hfcmultipci_driver);
}

static int __init
HFCmulti_init(void)
{
	int err;
	int i, xhfc = 0;
	struct hm_map m;

	printk(KERN_INFO "mISDN: HFC-multi driver %s\n", HFC_MULTI_VERSION);

#ifdef IRQ_DEBUG
	printk(KERN_DEBUG "%s: IRQ_DEBUG IS ENABLED!\n", __func__);
#endif

	spin_lock_init(&HFClock);
	spin_lock_init(&plx_lock);

	if (debug & DEBUG_HFCMULTI_INIT)
		printk(KERN_DEBUG "%s: init entered\n", __func__);

	switch (poll) {
	case 0:
		poll_timer = 6;
		poll = 128;
		break;
	case 8:
		poll_timer = 2;
		break;
	case 16:
		poll_timer = 3;
		break;
	case 32:
		poll_timer = 4;
		break;
	case 64:
		poll_timer = 5;
		break;
	case 128:
		poll_timer = 6;
		break;
	case 256:
		poll_timer = 7;
		break;
	default:
		printk(KERN_ERR
		       "%s: Wrong poll value (%d).\n", __func__, poll);
		err = -EINVAL;
		return err;

	}

	if (!clock)
		clock = 1;

	/* Register the embedded devices.
	 * This should be done before the PCI cards registration */
	switch (hwid) {
	case HWID_MINIP4:
		xhfc = 1;
		m = hfcm_map[31];
		break;
	case HWID_MINIP8:
		xhfc = 2;
		m = hfcm_map[31];
		break;
	case HWID_MINIP16:
		xhfc = 4;
		m = hfcm_map[31];
		break;
	default:
		xhfc = 0;
	}

	for (i = 0; i < xhfc; ++i) {
		err = hfcmulti_init(&m, NULL, NULL);
		if (err) {
			printk(KERN_ERR "error registering embedded driver: "
			       "%x\n", err);
			return err;
		}
		HFC_cnt++;
		printk(KERN_INFO "%d devices registered\n", HFC_cnt);
	}

	/* Register the PCI cards */
	err = pci_register_driver(&hfcmultipci_driver);
	if (err < 0) {
		printk(KERN_ERR "error registering pci driver: %x\n", err);
		return err;
	}

	return 0;
}


module_init(HFCmulti_init);
module_exit(HFCmulti_cleanup);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /* $Id: l3dss1.c,v 2.32.2.3 2004/01/13 14:31:25 keil Exp $
 *
 * EURO/DSS1 D-channel protocol
 *
 * German 1TR6 D-channel protocol
 *
 * Author       Karsten Keil
 *              based on the teles driver from Jan den Ouden
 * Copyright    by Karsten Keil      <keil@isdn4linux.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * For changes and modifications please read
 * Documentation/isdn/HiSax.cert
 *
 * Thanks to    Jan den Ouden
 *              Fritz Elfert
 *
 */

#include "hisax.h"
#include "isdnl3.h"
#include "l3dss1.h"
#include <linux/ctype.h>
#include <linux/slab.h>

extern char *HiSax_getrev(const char *revision);
static const char *dss1_revision = "$Revision: 2.32.2.3 $";

#define EXT_BEARER_CAPS 1

#define	MsgHead(ptr, cref, mty)			\
	*ptr++ = 0x8;				\
	if (cref == -1) {			\
		*ptr++ = 0x0;			\
	} else {				\
		*ptr++ = 0x1;			\
		*ptr++ = cref^0x80;		\
	}					\
	*ptr++ = mty


/**********************************************/
/* get a new invoke id for remote operations. */
/* Only a return value != 0 is valid          */
/**********************************************/
static unsigned char new_invoke_id(struct PStack *p)
{
	unsigned char retval;
	int i;

	i = 32; /* maximum search depth */

	retval = p->prot.dss1.last_invoke_id + 1; /* try new id */
	while ((i) && (p->prot.dss1.invoke_used[retval >> 3] == 0xFF)) {
		p->prot.dss1.last_invoke_id = (retval & 0xF8) + 8;
		i--;
	}
	if (i) {
		while (p->prot.dss1.invoke_used[retval >> 3] & (1 << (retval & 7)))
			retval++;
	} else
		retval = 0;
	p->prot.dss1.last_invoke_id = retval;
	p->prot.dss1.invoke_used[retval >> 3] |= (1 << (retval & 7));
	return (retval);
} /* new_invoke_id */

/*************************/
/* free a used invoke id */
/*************************/
static void free_invoke_id(struct PStack *p, unsigned char id)
{

	if (!id) return; /* 0 = invalid value */

	p->prot.dss1.invoke_used[id >> 3] &= ~(1 << (id & 7));
} /* free_invoke_id */


/**********************************************************/
/* create a new l3 process and fill in dss1 specific data */
/**********************************************************/
static struct l3_process
*dss1_new_l3_process(struct PStack *st, int cr)
{  struct l3_process *proc;

	if (!(proc = new_l3_process(st, cr)))
		return (NULL);

	proc->prot.dss1.invoke_id = 0;
	proc->prot.dss1.remote_operation = 0;
	proc->prot.dss1.uus1_data[0] = '\0';

	return (proc);
} /* dss1_new_l3_process */

/************************************************/
/* free a l3 process and all dss1 specific data */
/************************************************/
static void
dss1_release_l3_process(struct l3_process *p)
{
	free_invoke_id(p->st, p->prot.dss1.invoke_id);
	release_l3_process(p);
} /* dss1_release_l3_process */

/********************************************************/
/* search a process with invoke id id and dummy callref */
/********************************************************/
static struct l3_process *
l3dss1_search_dummy_proc(struct PStack *st, int id)
{ struct l3_process *pc = st->l3.proc; /* start of processes */

	if (!id) return (NULL);

	while (pc)
	{ if ((pc->callref == -1) && (pc->prot.dss1.invoke_id == id))
			return (pc);
		pc = pc->next;
	}
	return (NULL);
} /* l3dss1_search_dummy_proc */

/*******************************************************************/
/* called when a facility message with a dummy callref is received */
/* and a return result is delivered. id specifies the invoke id.   */
/*******************************************************************/
static void
l3dss1_dummy_return_result(struct PStack *st, int id, u_char *p, u_char nlen)
{ isdn_ctrl ic;
	struct IsdnCardState *cs;
	struct l3_process *pc = NULL;

	if ((pc = l3dss1_search_dummy_proc(st, id)))
	{ L3DelTimer(&pc->timer); /* remove timer */

		cs = pc->st->l1.hardware;
		ic.driver = cs->myid;
		ic.command = ISDN_STAT_PROT;
		ic.arg = DSS1_STAT_INVOKE_RES;
		ic.parm.dss1_io.hl_id = pc->prot.dss1.invoke_id;
		ic.parm.dss1_io.ll_id = pc->prot.dss1.ll_id;
		ic.parm.dss1_io.proc = pc->prot.dss1.proc;
		ic.parm.dss1_io.timeout = 0;
		ic.parm.dss1_io.datalen = nlen;
		ic.parm.dss1_io.data = p;
		free_invoke_id(pc->st, pc->prot.dss1.invoke_id);
		pc->prot.dss1.invoke_id = 0; /* reset id */

		cs->iif.statcallb(&ic);
		dss1_release_l3_process(pc);
	}
	else
		l3_debug(st, "dummy return result id=0x%x result len=%d", id, nlen);
} /* l3dss1_dummy_return_result */

/*******************************************************************/
/* called when a facility message with a dummy callref is received */
/* and a return error is delivered. id specifies the invoke id.    */
/*******************************************************************/
static void
l3dss1_dummy_error_return(struct PStack *st, int id, ulong error)
{ isdn_ctrl ic;
	struct IsdnCardState *cs;
	struct l3_process *pc = NULL;

	if ((pc = l3dss1_search_dummy_proc(st, id)))
	{ L3DelTimer(&pc->timer); /* remove timer */

		cs = pc->st->l1.hardware;
		ic.driver = cs->myid;
		ic.command = ISDN_STAT_PROT;
		ic.arg = DSS1_STAT_INVOKE_ERR;
		ic.parm.dss1_io.hl_id = pc->prot.dss1.invoke_id;
		ic.parm.dss1_io.ll_id = pc->prot.dss1.ll_id;
		ic.parm.dss1_io.proc = pc->prot.dss1.proc;
		ic.parm.dss1_io.timeout = error;
		ic.parm.dss1_io.datalen = 0;
		ic.parm.dss1_io.data = NULL;
		free_invoke_id(pc->st, pc->prot.dss1.invoke_id);
		pc->prot.dss1.invoke_id = 0; /* reset id */

		cs->iif.statcallb(&ic);
		dss1_release_l3_process(pc);
	}
	else
		l3_debug(st, "dummy return error id=0x%x error=0x%lx", id, error);
} /* l3dss1_error_return */

/*******************************************************************/
/* called when a facility message with a dummy callref is received */
/* and a invoke is delivered. id specifies the invoke id.          */
/*******************************************************************/
static void
l3dss1_dummy_invoke(struct PStack *st, int cr, int id,
		    int ident, u_char *p, u_char nlen)
{ isdn_ctrl ic;
	struct IsdnCardState *cs;

	l3_debug(st, "dummy invoke %s id=0x%x ident=0x%x datalen=%d",
		 (cr == -1) ? "local" : "broadcast", id, ident, nlen);
	if (cr >= -1) return; /* ignore local data */

	cs = st->l1.hardware;
	ic.driver = cs->myid;
	ic.command = ISDN_STAT_PROT;
	ic.arg = DSS1_STAT_INVOKE_BRD;
	ic.parm.dss1_io.hl_id = id;
	ic.parm.dss1_io.ll_id = 0;
	ic.parm.dss1_io.proc = ident;
	ic.parm.dss1_io.timeout = 0;
	ic.parm.dss1_io.datalen = nlen;
	ic.parm.dss1_io.data = p;

	cs->iif.statcallb(&ic);
} /* l3dss1_dummy_invoke */

static void
l3dss1_parse_facility(struct PStack *st, struct l3_process *pc,
		      int cr, u_char *p)
{
	int qd_len = 0;
	unsigned char nlen = 0, ilen, cp_tag;
	int ident, id;
	ulong err_ret;

	if (pc)
		st = pc->st; /* valid Stack */
	else
		if ((!st) || (cr >= 0)) return; /* neither pc nor st specified */

	p++;
	qd_len = *p++;
	if (qd_len == 0) {
		l3_debug(st, "qd_len == 0");
		return;
	}
	if ((*p & 0x1F) != 0x11) {	/* Service discriminator, supplementary service */
		l3_debug(st, "supplementary service != 0x11");
		return;
	}
	while (qd_len > 0 && !(*p & 0x80)) {	/* extension ? */
		p++;
		qd_len--;
	}
	if (qd_len < 2) {
		l3_debug(st, "qd_len < 2");
		return;
	}
	p++;
	qd_len--;
	if ((*p & 0xE0) != 0xA0) {	/* class and form */
		l3_debug(st, "class and form != 0xA0");
		return;
	}

	cp_tag = *p & 0x1F; /* remember tag value */

	p++;
	qd_len--;
	if (qd_len < 1)
	{ l3_debug(st, "qd_len < 1");
		return;
	}
	if (*p & 0x80)
	{ /* length format indefinite or limited */
		nlen = *p++ & 0x7F; /* number of len bytes or indefinite */
		if ((qd_len-- < ((!nlen) ? 3 : (1 + nlen))) ||
		    (nlen > 1))
		{ l3_debug(st, "length format error or not implemented");
			return;
		}
		if (nlen == 1)
		{ nlen = *p++; /* complete length */
			qd_len--;
		}
		else
		{ qd_len -= 2; /* trailing null bytes */
			if ((*(p + qd_len)) || (*(p + qd_len + 1)))
			{ l3_debug(st, "length format indefinite error");
				return;
			}
			nlen = qd_len;
		}
	}
	else
	{ nlen = *p++;
		qd_len--;
	}
	if (qd_len < nlen)
	{ l3_debug(st, "qd_len < nlen");
		return;
	}
	qd_len -= nlen;

	if (nlen < 2)
	{ l3_debug(st, "nlen < 2");
		return;
	}
	if (*p != 0x02)
	{  /* invoke identifier tag */
		l3_debug(st, "invoke identifier tag !=0x02");
		return;
	}
	p++;
	nlen--;
	if (*p & 0x80)
	{ /* length format */
		l3_debug(st, "invoke id length format 2");
		return;
	}
	ilen = *p++;
	nlen--;
	if (ilen > nlen || ilen == 0)
	{ l3_debug(st, "ilen > nlen || ilen == 0");
		return;
	}
	nlen -= ilen;
	id = 0;
	while (ilen > 0)
	{ id = (id << 8) | (*p++ & 0xFF);	/* invoke identifier */
		ilen--;
	}

	switch (cp_tag) {	/* component tag */
	case 1:	/* invoke */
		if (nlen < 2) {
			l3_debug(st, "nlen < 2 22");
			return;
		}
		if (*p != 0x02) {	/* operation value */
			l3_debug(st, "operation value !=0x02");
			return;
		}
		p++;
		nlen--;
		ilen = *p++;
		nlen--;
		if (ilen > nlen || ilen == 0) {
			l3_debug(st, "ilen > nlen || ilen == 0 22");
			return;
		}
		nlen -= ilen;
		ident = 0;
		while (ilen > 0) {
			ident = (ident << 8) | (*p++ & 0xFF);
			ilen--;
		}

		if (!pc)
		{ l3dss1_dummy_invoke(st, cr, id, ident, p, nlen);
			return;
		}
#ifdef CONFIG_DE_AOC
		{

#define FOO1(s, a, b)							\
			while (nlen > 1) {				\
				int ilen = p[1];			\
				if (nlen < ilen + 2) {			\
					l3_debug(st, "FOO1  nlen < ilen+2"); \
					return;				\
				}					\
				nlen -= ilen + 2;			\
				if ((*p & 0xFF) == (a)) {		\
					int nlen = ilen;		\
					p += 2;				\
					b;				\
				} else {				\
					p += ilen + 2;			\
				}					\
			}

			switch (ident) {
			case 0x22:	/* during */
				FOO1("1A", 0x30, FOO1("1C", 0xA1, FOO1("1D", 0x30, FOO1("1E", 0x02, ( {
										ident = 0;
										nlen = (nlen) ? nlen : 0; /* Make gcc happy */
										while (ilen > 0) {
											ident = (ident << 8) | *p++;
											ilen--;
										}
										if (ident > pc->para.chargeinfo) {
											pc->para.chargeinfo = ident;
											st->l3.l3l4(st, CC_CHARGE | INDICATION, pc);
										}
										if (st->l3.debug & L3_DEB_CHARGE) {
											if (*(p + 2) == 0) {
												l3_debug(st, "charging info during %d", pc->para.chargeinfo);
											}
											else {
												l3_debug(st, "charging info final %d", pc->para.chargeinfo);
											}
										}
									}
									)))))
					break;
			case 0x24:	/* final */
				FOO1("2A", 0x30, FOO1("2B", 0x30, FOO1("2C", 0xA1, FOO1("2D", 0x30, FOO1("2E", 0x02, ( {
											ident = 0;
											nlen = (nlen) ? nlen : 0; /* Make gcc happy */
											while (ilen > 0) {
												ident = (ident << 8) | *p++;
												ilen--;
											}
											if (ident > pc->para.chargeinfo) {
												pc->para.chargeinfo = ident;
												st->l3.l3l4(st, CC_CHARGE | INDICATION, pc);
											}
											if (st->l3.debug & L3_DEB_CHARGE) {
												l3_debug(st, "charging info final %d", pc->para.chargeinfo);
											}
										}
										))))))
					break;
			default:
				l3_debug(st, "invoke break invalid ident %02x", ident);
				break;
			}
#undef FOO1

		}
#else  /* not CONFIG_DE_AOC */
		l3_debug(st, "invoke break");
#endif /* not CONFIG_DE_AOC */
		break;
	case 2:	/* return result */
		/* if no process available handle separately */
		if (!pc)
		{ if (cr == -1)
				l3dss1_dummy_return_result(st, id, p, nlen);
			return;
		}
		if ((pc->prot.dss1.invoke_id) && (pc->prot.dss1.invoke_id == id))
		{ /* Diversion successful */
			free_invoke_id(st, pc->prot.dss1.invoke_id);
			pc->prot.dss1.remote_result = 0; /* success */
			pc->prot.dss1.invoke_id = 0;
			pc->redir_result = pc->prot.dss1.remote_result;
			st->l3.l3l4(st, CC_REDIR | INDICATION, pc);                                  } /* Diversion successful */
		else
			l3_debug(st, "return error unknown identifier");
		break;
	case 3:	/* return error */
		err_ret = 0;
		if (nlen < 2)
		{ l3_debug(st, "return error nlen < 2");
			return;
		}
		if (*p != 0x02)
		{ /* result tag */
			l3_debug(st, "invoke error tag !=0x02");
			return;
		}
		p++;
		nlen--;
		if (*p > 4)
		{ /* length format */
			l3_debug(st, "invoke return errlen > 4 ");
			return;
		}
		ilen = *p++;
		nlen--;
		if (ilen > nlen || ilen == 0)
		{ l3_debug(st, "error return ilen > nlen || ilen == 0");
			return;
		}
		nlen -= ilen;
		while (ilen > 0)
		{ err_ret = (err_ret << 8) | (*p++ & 0xFF);	/* error value */
			ilen--;
		}
		/* if no process available handle separately */
		if (!pc)
		{ if (cr == -1)
				l3dss1_dummy_error_return(st, id, err_ret);
			return;
		}
		if ((pc->prot.dss1.invoke_id) && (pc->prot.dss1.invoke_id == id))
		{ /* Deflection error */
			free_invoke_id(st, pc->prot.dss1.invoke_id);
			pc->prot.dss1.remote_result = err_ret; /* result */
			pc->prot.dss1.invoke_id = 0;
			pc->redir_result = pc->prot.dss1.remote_result;
			st->l3.l3l4(st, CC_REDIR | INDICATION, pc);
		} /* Deflection error */
		else
			l3_debug(st, "return result unknown identifier");
		break;
	default:
		l3_debug(st, "facility default break tag=0x%02x", cp_tag);
		break;
	}
}

static void
l3dss1_message(struct l3_process *pc, u_char mt)
{
	struct sk_buff *skb;
	u_char *p;

	if (!(skb = l3_alloc_skb(4)))
		return;
	p = skb_put(skb, 4);
	MsgHead(p, pc->callref, mt);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3dss1_message_cause(struct l3_process *pc, u_char mt, u_char cause)
{
	struct sk_buff *skb;
	u_char tmp[16];
	u_char *p = tmp;
	int l;

	MsgHead(p, pc->callref, mt);
	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = cause | 0x80;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3dss1_status_send(struct l3_process *pc, u_char pr, void *arg)
{
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	struct sk_buff *skb;

	MsgHead(p, pc->callref, MT_STATUS);

	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = pc->para.cause | 0x80;

	*p++ = IE_CALL_STATE;
	*p++ = 0x1;
	*p++ = pc->state & 0x3f;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3dss1_msg_without_setup(struct l3_process *pc, u_char pr, void *arg)
{
	/* This routine is called if here was no SETUP made (checks in dss1up and in
	 * l3dss1_setup) and a RELEASE_COMPLETE have to be sent with an error code
	 * MT_STATUS_ENQUIRE in the NULL state is handled too
	 */
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	struct sk_buff *skb;

	switch (pc->para.cause) {
	case 81:	/* invalid callreference */
	case 88:	/* incomp destination */
	case 96:	/* mandory IE missing */
	case 100:       /* invalid IE contents */
	case 101:	/* incompatible Callstate */
		MsgHead(p, pc->callref, MT_RELEASE_COMPLETE);
		*p++ = IE_CAUSE;
		*p++ = 0x2;
		*p++ = 0x80;
		*p++ = pc->para.cause | 0x80;
		break;
	default:
		printk(KERN_ERR "HiSax l3dss1_msg_without_setup wrong cause %d\n",
		       pc->para.cause);
		return;
	}
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	dss1_release_l3_process(pc);
}

static int ie_ALERTING[] = {IE_BEARER, IE_CHANNEL_ID | IE_MANDATORY_1,
			    IE_FACILITY, IE_PROGRESS, IE_DISPLAY, IE_SIGNAL, IE_HLC,
			    IE_USER_USER, -1};
static int ie_CALL_PROCEEDING[] = {IE_BEARER, IE_CHANNEL_ID | IE_MANDATORY_1,
				   IE_FACILITY, IE_PROGRESS, IE_DISPLAY, IE_HLC, -1};
static int ie_CONNECT[] = {IE_BEARER, IE_CHANNEL_ID | IE_MANDATORY_1,
			   IE_FACILITY, IE_PROGRESS, IE_DISPLAY, IE_DATE, IE_SIGNAL,
			   IE_CONNECT_PN, IE_CONNECT_SUB, IE_LLC, IE_HLC, IE_USER_USER, -1};
static int ie_CONNECT_ACKNOWLEDGE[] = {IE_CHANNEL_ID, IE_DISPLAY, IE_SIGNAL, -1};
static int ie_DISCONNECT[] = {IE_CAUSE | IE_MANDATORY, IE_FACILITY,
			      IE_PROGRESS, IE_DISPLAY, IE_SIGNAL, IE_USER_USER, -1};
static int ie_INFORMATION[] = {IE_COMPLETE, IE_DISPLAY, IE_KEYPAD, IE_SIGNAL,
			       IE_CALLED_PN, -1};
static int ie_NOTIFY[] = {IE_BEARER, IE_NOTIFY | IE_MANDATORY, IE_DISPLAY, -1};
static int ie_PROGRESS[] = {IE_BEARER, IE_CAUSE, IE_FACILITY, IE_PROGRESS |
			    IE_MANDATORY, IE_DISPLAY, IE_HLC, IE_USER_USER, -1};
static int ie_RELEASE[] = {IE_CAUSE | IE_MANDATORY_1, IE_FACILITY, IE_DISPLAY,
			   IE_SIGNAL, IE_USER_USER, -1};
/* a RELEASE_COMPLETE with errors don't require special actions
   static int ie_RELEASE_COMPLETE[] = {IE_CAUSE | IE_MANDATORY_1, IE_DISPLAY, IE_SIGNAL, IE_USER_USER, -1};
*/
static int ie_RESUME_ACKNOWLEDGE[] = {IE_CHANNEL_ID | IE_MANDATORY, IE_FACILITY,
				      IE_DISPLAY, -1};
static int ie_RESUME_REJECT[] = {IE_CAUSE | IE_MANDATORY, IE_DISPLAY, -1};
static int ie_SETUP[] = {IE_COMPLETE, IE_BEARER  | IE_MANDATORY,
			 IE_CHANNEL_ID | IE_MANDATORY, IE_FACILITY, IE_PROGRESS,
			 IE_NET_FAC, IE_DISPLAY, IE_KEYPAD, IE_SIGNAL, IE_CALLING_PN,
			 IE_CALLING_SUB, IE_CALLED_PN, IE_CALLED_SUB, IE_REDIR_NR,
			 IE_LLC, IE_HLC, IE_USER_USER, -1};
static int ie_SETUP_ACKNOWLEDGE[] = {IE_CHANNEL_ID | IE_MANDATORY, IE_FACILITY,
				     IE_PROGRESS, IE_DISPLAY, IE_SIGNAL, -1};
static int ie_STATUS[] = {IE_CAUSE | IE_MANDATORY, IE_CALL_STATE |
			  IE_MANDATORY, IE_DISPLAY, -1};
static int ie_STATUS_ENQUIRY[] = {IE_DISPLAY, -1};
static int ie_SUSPEND_ACKNOWLEDGE[] = {IE_DISPLAY, IE_FACILITY, -1};
static int ie_SUSPEND_REJECT[] = {IE_CAUSE | IE_MANDATORY, IE_DISPLAY, -1};
/* not used
 * static int ie_CONGESTION_CONTROL[] = {IE_CONGESTION | IE_MANDATORY,
 *		IE_CAUSE | IE_MANDATORY, IE_DISPLAY, -1};
 * static int ie_USER_INFORMATION[] = {IE_MORE_DATA, IE_USER_USER | IE_MANDATORY, -1};
 * static int ie_RESTART[] = {IE_CHANNEL_ID, IE_DISPLAY, IE_RESTART_IND |
 *		IE_MANDATORY, -1};
 */
static int ie_FACILITY[] = {IE_FACILITY | IE_MANDATORY, IE_DISPLAY, -1};
static int comp_required[] = {1, 2, 3, 5, 6, 7, 9, 10, 11, 14, 15, -1};
static int l3_valid_states[] = {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 15, 17, 19, 25, -1};

struct ie_len {
	int ie;
	int len;
};

static
struct ie_len max_ie_len[] = {
	{IE_SEGMENT, 4},
	{IE_BEARER, 12},
	{IE_CAUSE, 32},
	{IE_CALL_ID, 10},
	{IE_CALL_STATE, 3},
	{IE_CHANNEL_ID,	34},
	{IE_FACILITY, 255},
	{IE_PROGRESS, 4},
	{IE_NET_FAC, 255},
	{IE_NOTIFY, 3},
	{IE_DISPLAY, 82},
	{IE_DATE, 8},
	{IE_KEYPAD, 34},
	{IE_SIGNAL, 3},
	{IE_INFORATE, 6},
	{IE_E2E_TDELAY, 11},
	{IE_TDELAY_SEL, 5},
	{IE_PACK_BINPARA, 3},
	{IE_PACK_WINSIZE, 4},
	{IE_PACK_SIZE, 4},
	{IE_CUG, 7},
	{IE_REV_CHARGE, 3},
	{IE_CALLING_PN, 24},
	{IE_CALLING_SUB, 23},
	{IE_CALLED_PN, 24},
	{IE_CALLED_SUB, 23},
	{IE_REDIR_NR, 255},
	{IE_TRANS_SEL, 255},
	{IE_RESTART_IND, 3},
	{IE_LLC, 18},
	{IE_HLC, 5},
	{IE_USER_USER, 131},
	{-1, 0},
};

static int
getmax_ie_len(u_char ie) {
	int i = 0;
	while (max_ie_len[i].ie != -1) {
		if (max_ie_len[i].ie == ie)
			return (max_ie_len[i].len);
		i++;
	}
	return (255);
}

static int
ie_in_set(struct l3_process *pc, u_char ie, int *checklist) {
	int ret = 1;

	while (*checklist != -1) {
		if ((*checklist & 0xff) == ie) {
			if (ie & 0x80)
				return (-ret);
			else
				return (ret);
		}
		ret++;
		checklist++;
	}
	return (0);
}

static int
check_infoelements(struct l3_process *pc, struct sk_buff *skb, int *checklist)
{
	int *cl = checklist;
	u_char mt;
	u_char *p, ie;
	int l, newpos, oldpos;
	int err_seq = 0, err_len = 0, err_compr = 0, err_ureg = 0;
	u_char codeset = 0;
	u_char old_codeset = 0;
	u_char codelock = 1;

	p = skb->data;
	/* skip cr */
	p++;
	l = (*p++) & 0xf;
	p += l;
	mt = *p++;
	oldpos = 0;
	while ((p - skb->data) < skb->len) {
		if ((*p & 0xf0) == 0x90) { /* shift codeset */
			old_codeset = codeset;
			codeset = *p & 7;
			if (*p & 0x08)
				codelock = 0;
			else
				codelock = 1;
			if (pc->debug & L3_DEB_CHECK)
				l3_debug(pc->st, "check IE shift%scodeset %d->%d",
					 codelock ? " locking " : " ", old_codeset, codeset);
			p++;
			continue;
		}
		if (!codeset) { /* only codeset 0 */
			if ((newpos = ie_in_set(pc, *p, cl))) {
				if (newpos > 0) {
					if (newpos < oldpos)
						err_seq++;
					else
						oldpos = newpos;
				}
			} else {
				if (ie_in_set(pc, *p, comp_required))
					err_compr++;
				else
					err_ureg++;
			}
		}
		ie = *p++;
		if (ie & 0x80) {
			l = 1;
		} else {
			l = *p++;
			p += l;
			l += 2;
		}
		if (!codeset && (l > getmax_ie_len(ie)))
			err_len++;
		if (!codelock) {
			if (pc->debug & L3_DEB_CHECK)
				l3_debug(pc->st, "check IE shift back codeset %d->%d",
					 codeset, old_codeset);
			codeset = old_codeset;
			codelock = 1;
		}
	}
	if (err_compr | err_ureg | err_len | err_seq) {
		if (pc->debug & L3_DEB_CHECK)
			l3_debug(pc->st, "check IE MT(%x) %d/%d/%d/%d",
				 mt, err_compr, err_ureg, err_len, err_seq);
		if (err_compr)
			return (ERR_IE_COMPREHENSION);
		if (err_ureg)
			return (ERR_IE_UNRECOGNIZED);
		if (err_len)
			return (ERR_IE_LENGTH);
		if (err_seq)
			return (ERR_IE_SEQUENCE);
	}
	return (0);
}

/* verify if a message type exists and contain no IE error */
static int
l3dss1_check_messagetype_validity(struct l3_process *pc, int mt, void *arg)
{
	switch (mt) {
	case MT_ALERTING:
	case MT_CALL_PROCEEDING:
	case MT_CONNECT:
	case MT_CONNECT_ACKNOWLEDGE:
	case MT_DISCONNECT:
	case MT_INFORMATION:
	case MT_FACILITY:
	case MT_NOTIFY:
	case MT_PROGRESS:
	case MT_RELEASE:
	case MT_RELEASE_COMPLETE:
	case MT_SETUP:
	case MT_SETUP_ACKNOWLEDGE:
	case MT_RESUME_ACKNOWLEDGE:
	case MT_RESUME_REJECT:
	case MT_SUSPEND_ACKNOWLEDGE:
	case MT_SUSPEND_REJECT:
	case MT_USER_INFORMATION:
	case MT_RESTART:
	case MT_RESTART_ACKNOWLEDGE:
	case MT_CONGESTION_CONTROL:
	case MT_STATUS:
	case MT_STATUS_ENQUIRY:
		if (pc->debug & L3_DEB_CHECK)
			l3_debug(pc->st, "l3dss1_check_messagetype_validity mt(%x) OK", mt);
		break;
	case MT_RESUME: /* RESUME only in user->net */
	case MT_SUSPEND: /* SUSPEND only in user->net */
	default:
		if (pc->debug & (L3_DEB_CHECK | L3_DEB_WARN))
			l3_debug(pc->st, "l3dss1_check_messagetype_validity mt(%x) fail", mt);
		pc->para.cause = 97;
		l3dss1_status_send(pc, 0, NULL);
		return (1);
	}
	return (0);
}

static void
l3dss1_std_ie_err(struct l3_process *pc, int ret) {

	if (pc->debug & L3_DEB_CHECK)
		l3_debug(pc->st, "check_infoelements ret %d", ret);
	switch (ret) {
	case 0:
		break;
	case ERR_IE_COMPREHENSION:
		pc->para.cause = 96;
		l3dss1_status_send(pc, 0, NULL);
		break;
	case ERR_IE_UNRECOGNIZED:
		pc->para.cause = 99;
		l3dss1_status_send(pc, 0, NULL);
		break;
	case ERR_IE_LENGTH:
		pc->para.cause = 100;
		l3dss1_status_send(pc, 0, NULL);
		break;
	case ERR_IE_SEQUENCE:
	default:
		break;
	}
}

static int
l3dss1_get_channel_id(struct l3_process *pc, struct sk_buff *skb) {
	u_char *p;

	p = skb->data;
	if ((p = findie(p, skb->len, IE_CHANNEL_ID, 0))) {
		p++;
		if (*p != 1) { /* len for BRI = 1 */
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "wrong chid len %d", *p);
			return (-2);
		}
		p++;
		if (*p & 0x60) { /* only base rate interface */
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "wrong chid %x", *p);
			return (-3);
		}
		return (*p & 0x3);
	} else
		return (-1);
}

static int
l3dss1_get_cause(struct l3_process *pc, struct sk_buff *skb) {
	u_char l, i = 0;
	u_char *p;

	p = skb->data;
	pc->para.cause = 31;
	pc->para.loc = 0;
	if ((p = findie(p, skb->len, IE_CAUSE, 0))) {
		p++;
		l = *p++;
		if (l > 30)
			return (1);
		if (l) {
			pc->para.loc = *p++;
			l--;
		} else {
			return (2);
		}
		if (l && !(pc->para.loc & 0x80)) {
			l--;
			p++; /* skip recommendation */
		}
		if (l) {
			pc->para.cause = *p++;
			l--;
			if (!(pc->para.cause & 0x80))
				return (3);
		} else
			return (4);
		while (l && (i < 6)) {
			pc->para.diag[i++] = *p++;
			l--;
		}
	} else
		return (-1);
	return (0);
}

static void
l3dss1_msg_with_uus(struct l3_process *pc, u_char cmd)
{
	struct sk_buff *skb;
	u_char tmp[16 + 40];
	u_char *p = tmp;
	int l;

	MsgHead(p, pc->callref, cmd);

	if (pc->prot.dss1.uus1_data[0])
	{ *p++ = IE_USER_USER; /* UUS info element */
		*p++ = strlen(pc->prot.dss1.uus1_data) + 1;
		*p++ = 0x04; /* IA5 chars */
		strcpy(p, pc->prot.dss1.uus1_data);
		p += strlen(pc->prot.dss1.uus1_data);
		pc->prot.dss1.uus1_data[0] = '\0';
	}

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
} /* l3dss1_msg_with_uus */

static void
l3dss1_release_req(struct l3_process *pc, u_char pr, void *arg)
{
	StopAllL3Timer(pc);
	newl3state(pc, 19);
	if (!pc->prot.dss1.uus1_data[0])
		l3dss1_message(pc, MT_RELEASE);
	else
		l3dss1_msg_with_uus(pc, MT_RELEASE);
	L3AddTimer(&pc->timer, T308, CC_T308_1);
}

static void
l3dss1_release_cmpl(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	if ((ret = l3dss1_get_cause(pc, skb)) > 0) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "RELCMPL get_cause ret(%d)", ret);
	} else if (ret < 0)
		pc->para.cause = NO_CAUSE;
	StopAllL3Timer(pc);
	newl3state(pc, 0);
	pc->st->l3.l3l4(pc->st, CC_RELEASE | CONFIRM, pc);
	dss1_release_l3_process(pc);
}

#ifdef EXT_BEARER_CAPS

static u_char *
EncodeASyncParams(u_char *p, u_char si2)
{				// 7c 06 88  90 21 42 00 bb

	p[0] = 0;
	p[1] = 0x40;		// Intermediate rate: 16 kbit/s jj 2000.02.19
	p[2] = 0x80;
	if (si2 & 32)		// 7 data bits

		p[2] += 16;
	else			// 8 data bits

		p[2] += 24;

	if (si2 & 16)		// 2 stop bits

		p[2] += 96;
	else			// 1 stop bit

		p[2] += 32;

	if (si2 & 8)		// even parity

		p[2] += 2;
	else			// no parity

		p[2] += 3;

	switch (si2 & 0x07) {
	case 0:
		p[0] = 66;	// 1200 bit/s

		break;
	case 1:
		p[0] = 88;	// 1200/75 bit/s

		break;
	case 2:
		p[0] = 87;	// 75/1200 bit/s

		break;
	case 3:
		p[0] = 67;	// 2400 bit/s

		break;
	case 4:
		p[0] = 69;	// 4800 bit/s

		break;
	case 5:
		p[0] = 72;	// 9600 bit/s

		break;
	case 6:
		p[0] = 73;	// 14400 bit/s

		break;
	case 7:
		p[0] = 75;	// 19200 bit/s

		break;
	}
	return p + 3;
}

static  u_char
EncodeSyncParams(u_char si2, u_char ai)
{

	switch (si2) {
	case 0:
		return ai + 2;	// 1200 bit/s

	case 1:
		return ai + 24;		// 1200/75 bit/s

	case 2:
		return ai + 23;		// 75/1200 bit/s

	case 3:
		return ai + 3;	// 2400 bit/s

	case 4:
		return ai + 5;	// 4800 bit/s

	case 5:
		return ai + 8;	// 9600 bit/s

	case 6:
		return ai + 9;	// 14400 bit/s

	case 7:
		return ai + 11;		// 19200 bit/s

	case 8:
		return ai + 14;		// 48000 bit/s

	case 9:
		return ai + 15;		// 56000 bit/s

	case 15:
		return ai + 40;		// negotiate bit/s

	default:
		break;
	}
	return ai;
}


static u_char
DecodeASyncParams(u_char si2, u_char *p)
{
	u_char info;

	switch (p[5]) {
	case 66:	// 1200 bit/s

		break;	// si2 don't change

	case 88:	// 1200/75 bit/s

		si2 += 1;
		break;
	case 87:	// 75/1200 bit/s

		si2 += 2;
		break;
	case 67:	// 2400 bit/s

		si2 += 3;
		break;
	case 69:	// 4800 bit/s

		si2 += 4;
		break;
	case 72:	// 9600 bit/s

		si2 += 5;
		break;
	case 73:	// 14400 bit/s

		si2 += 6;
		break;
	case 75:	// 19200 bit/s

		si2 += 7;
		break;
	}

	info = p[7] & 0x7f;
	if ((info & 16) && (!(info & 8)))	// 7 data bits

		si2 += 32;	// else 8 data bits

	if ((info & 96) == 96)	// 2 stop bits

		si2 += 16;	// else 1 stop bit

	if ((info & 2) && (!(info & 1)))	// even parity

		si2 += 8;	// else no parity

	return si2;
}


static u_char
DecodeSyncParams(u_char si2, u_char info)
{
	info &= 0x7f;
	switch (info) {
	case 40:	// bit/s negotiation failed  ai := 165 not 175!

		return si2 + 15;
	case 15:	// 56000 bit/s failed, ai := 0 not 169 !

		return si2 + 9;
	case 14:	// 48000 bit/s

		return si2 + 8;
	case 11:	// 19200 bit/s

		return si2 + 7;
	case 9:	// 14400 bit/s

		return si2 + 6;
	case 8:	// 9600  bit/s

		return si2 + 5;
	case 5:	// 4800  bit/s

		return si2 + 4;
	case 3:	// 2400  bit/s

		return si2 + 3;
	case 23:	// 75/1200 bit/s

		return si2 + 2;
	case 24:	// 1200/75 bit/s

		return si2 + 1;
	default:	// 1200 bit/s

		return si2;
	}
}

static u_char
DecodeSI2(struct sk_buff *skb)
{
	u_char *p;		//, *pend=skb->data + skb->len;

	if ((p = findie(skb->data, skb->len, 0x7c, 0))) {
		switch (p[4] & 0x0f) {
		case 0x01:
			if (p[1] == 0x04)	// sync. Bitratenadaption

				return DecodeSyncParams(160, p[5]);	// V.110/X.30

			else if (p[1] == 0x06)	// async. Bitratenadaption

				return DecodeASyncParams(192, p);	// V.110/X.30

			break;
		case 0x08:	// if (p[5] == 0x02) // sync. Bitratenadaption
			if (p[1] > 3)
				return DecodeSyncParams(176, p[5]);	// V.120
			break;
		}
	}
	return 0;
}

#endif


static void
l3dss1_setup_req(struct l3_process *pc, u_char pr,
		 void *arg)
{
	struct sk_buff *skb;
	u_char tmp[128];
	u_char *p = tmp;
	u_char channel = 0;

	u_char send_keypad;
	u_char screen = 0x80;
	u_char *teln;
	u_char *msn;
	u_char *sub;
	u_char *sp;
	int l;

	MsgHead(p, pc->callref, MT_SETUP);

	teln = pc->para.setup.phone;
#ifndef CONFIG_HISAX_NO_KEYPAD
	send_keypad = (strchr(teln, '*') || strchr(teln, '#')) ? 1 : 0;
#else
	send_keypad = 0;
#endif
#ifndef CONFIG_HISAX_NO_SENDCOMPLETE
	if (!send_keypad)
		*p++ = 0xa1;		/* complete indicator */
#endif
	/*
	 * Set Bearer Capability, Map info from 1TR6-convention to EDSS1
	 */
	switch (pc->para.setup.si1) {
	case 1:	                  /* Telephony                                */
		*p++ = IE_BEARER;
		*p++ = 0x3;	  /* Length                                   */
		*p++ = 0x90;	  /* Coding Std. CCITT, 3.1 kHz audio         */
		*p++ = 0x90;	  /* Circuit-Mode 64kbps                      */
		*p++ = 0xa3;	  /* A-Law Audio                              */
		break;
	case 5:	                  /* Datatransmission 64k, BTX                */
	case 7:	                  /* Datatransmission 64k                     */
	default:
		*p++ = IE_BEARER;
		*p++ = 0x2;	  /* Length                                   */
		*p++ = 0x88;	  /* Coding Std. CCITT, unrestr. dig. Inform. */
		*p++ = 0x90;	  /* Circuit-Mode 64kbps                      */
		break;
	}

	if (send_keypad) {
		*p++ = IE_KEYPAD;
		*p++ = strlen(teln);
		while (*teln)
			*p++ = (*teln++) & 0x7F;
	}

	/*
	 * What about info2? Mapping to High-Layer-Compatibility?
	 */
	if ((*teln) && (!send_keypad)) {
		/* parse number for special things */
		if (!isdigit(*teln)) {
			switch (0x5f & *teln) {
			case 'C':
				channel = 0x08;
			case 'P':
				channel |= 0x80;
				teln++;
				if (*teln == '1')
					channel |= 0x01;
				else
					channel |= 0x02;
				break;
			case 'R':
				screen = 0xA0;
				break;
			case 'D':
				screen = 0x80;
				break;

			default:
				if (pc->debug & L3_DEB_WARN)
					l3_debug(pc->st, "Wrong MSN Code");
				break;
			}
			teln++;
		}
	}
	if (channel) {
		*p++ = IE_CHANNEL_ID;
		*p++ = 1;
		*p++ = channel;
	}
	msn = pc->para.setup.eazmsn;
	sub = NULL;
	sp = msn;
	while (*sp) {
		if ('.' == *sp) {
			sub = sp;
			*sp = 0;
		} else
			sp++;
	}
	if (*msn) {
		*p++ = IE_CALLING_PN;
		*p++ = strlen(msn) + (screen ? 2 : 1);
		/* Classify as AnyPref. */
		if (screen) {
			*p++ = 0x01;	/* Ext = '0'B, Type = '000'B, Plan = '0001'B. */
			*p++ = screen;
		} else
			*p++ = 0x81;	/* Ext = '1'B, Type = '000'B, Plan = '0001'B. */
		while (*msn)
			*p++ = *msn++ & 0x7f;
	}
	if (sub) {
		*sub++ = '.';
		*p++ = IE_CALLING_SUB;
		*p++ = strlen(sub) + 2;
		*p++ = 0x80;	/* NSAP coded */
		*p++ = 0x50;	/* local IDI format */
		while (*sub)
			*p++ = *sub++ & 0x7f;
	}
	sub = NULL;
	sp = teln;
	while (*sp) {
		if ('.' == *sp) {
			sub = sp;
			*sp = 0;
		} else
			sp++;
	}

	if (!send_keypad) {
		*p++ = IE_CALLED_PN;
		*p++ = strlen(teln) + 1;
		/* Classify as AnyPref. */
		*p++ = 0x81;		/* Ext = '1'B, Type = '000'B, Plan = '0001'B. */
		while (*teln)
			*p++ = *teln++ & 0x7f;

		if (sub) {
			*sub++ = '.';
			*p++ = IE_CALLED_SUB;
			*p++ = strlen(sub) + 2;
			*p++ = 0x80;	/* NSAP coded */
			*p++ = 0x50;	/* local IDI format */
			while (*sub)
				*p++ = *sub++ & 0x7f;
		}
	}
#ifdef EXT_BEARER_CAPS
	if ((pc->para.setup.si2 >= 160) && (pc->para.setup.si2 <= 175)) {	// sync. Bitratenadaption, V.110/X.30

		*p++ = IE_LLC;
		*p++ = 0x04;
		*p++ = 0x88;
		*p++ = 0x90;
		*p++ = 0x21;
		*p++ = EncodeSyncParams(pc->para.setup.si2 - 160, 0x80);
	} else if ((pc->para.setup.si2 >= 176) && (pc->para.setup.si2 <= 191)) {	// sync. Bitratenadaption, V.120

		*p++ = IE_LLC;
		*p++ = 0x05;
		*p++ = 0x88;
		*p++ = 0x90;
		*p++ = 0x28;
		*p++ = EncodeSyncParams(pc->para.setup.si2 - 176, 0);
		*p++ = 0x82;
	} else if (pc->para.setup.si2 >= 192) {		// async. Bitratenadaption, V.110/X.30

		*p++ = IE_LLC;
		*p++ = 0x06;
		*p++ = 0x88;
		*p++ = 0x90;
		*p++ = 0x21;
		p = EncodeASyncParams(p, pc->para.setup.si2 - 192);
#ifndef CONFIG_HISAX_NO_LLC
	} else {
		switch (pc->para.setup.si1) {
		case 1:	                /* Telephony                                */
			*p++ = IE_LLC;
			*p++ = 0x3;	/* Length                                   */
			*p++ = 0x90;	/* Coding Std. CCITT, 3.1 kHz audio         */
			*p++ = 0x90;	/* Circuit-Mode 64kbps                      */
			*p++ = 0xa3;	/* A-Law Audio                              */
			break;
		case 5:	                /* Datatransmission 64k, BTX                */
		case 7:	                /* Datatransmission 64k                     */
		default:
			*p++ = IE_LLC;
			*p++ = 0x2;	/* Length                                   */
			*p++ = 0x88;	/* Coding Std. CCITT, unrestr. dig. Inform. */
			*p++ = 0x90;	/* Circuit-Mode 64kbps                      */
			break;
		}
#endif
	}
#endif
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, T303, CC_T303);
	newl3state(pc, 1);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3dss1_call_proc(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int id, ret;

	if ((id = l3dss1_get_channel_id(pc, skb)) >= 0) {
		if ((0 == id) || ((3 == id) && (0x10 == pc->para.moderate))) {
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "setup answer with wrong chid %x", id);
			pc->para.cause = 100;
			l3dss1_status_send(pc, pr, NULL);
			return;
		}
		pc->para.bchannel = id;
	} else if (1 == pc->state) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "setup answer wrong chid (ret %d)", id);
		if (id == -1)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3dss1_status_send(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	ret = check_infoelements(pc, skb, ie_CALL_PROCEEDING);
	if (ERR_IE_COMPREHENSION == ret) {
		l3dss1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	newl3state(pc, 3);
	L3AddTimer(&pc->timer, T310, CC_T310);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3dss1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_PROCEEDING | INDICATION, pc);
}

static void
l3dss1_setup_ack(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int id, ret;

	if ((id = l3dss1_get_channel_id(pc, skb)) >= 0) {
		if ((0 == id) || ((3 == id) && (0x10 == pc->para.moderate))) {
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "setup answer with wrong chid %x", id);
			pc->para.cause = 100;
			l3dss1_status_send(pc, pr, NULL);
			return;
		}
		pc->para.bchannel = id;
	} else {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "setup answer wrong chid (ret %d)", id);
		if (id == -1)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3dss1_status_send(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	ret = check_infoelements(pc, skb, ie_SETUP_ACKNOWLEDGE);
	if (ERR_IE_COMPREHENSION == ret) {
		l3dss1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	newl3state(pc, 2);
	L3AddTimer(&pc->timer, T304, CC_T304);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3dss1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_MORE_INFO | INDICATION, pc);
}

static void
l3dss1_disconnect(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	u_char *p;
	int ret;
	u_char cause = 0;

	StopAllL3Timer(pc);
	if ((ret = l3dss1_get_cause(pc, skb))) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "DISC get_cause ret(%d)", ret);
		if (ret < 0)
			cause = 96;
		else if (ret > 0)
			cause = 100;
	}
	if ((p = findie(skb->data, skb->len, IE_FACILITY, 0)))
		l3dss1_parse_facility(pc->st, pc, pc->callref, p);
	ret = check_infoelements(pc, skb, ie_DISCONNECT);
	if (ERR_IE_COMPREHENSION == ret)
		cause = 96;
	else if ((!cause) && (ERR_IE_UNRECOGNIZED == ret))
		cause = 99;
	ret = pc->state;
	newl3state(pc, 12);
	if (cause)
		newl3state(pc, 19);
	if (11 != ret)
		pc->st->l3.l3l4(pc->st, CC_DISCONNECT | INDICATION, pc);
	else if (!cause)
		l3dss1_release_req(pc, pr, NULL);
	if (cause) {
		l3dss1_message_cause(pc, MT_RELEASE, cause);
		L3AddTimer(&pc->timer, T308, CC_T308_1);
	}
}

static void
l3dss1_connect(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	ret = check_infoelements(pc, skb, ie_CONNECT);
	if (ERR_IE_COMPREHENSION == ret) {
		l3dss1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);	/* T310 */
	newl3state(pc, 10);
	pc->para.chargeinfo = 0;
	/* here should inserted COLP handling KKe */
	if (ret)
		l3dss1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_SETUP | CONFIRM, pc);
}

static void
l3dss1_alerting(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	ret = check_infoelements(pc, skb, ie_ALERTING);
	if (ERR_IE_COMPREHENSION == ret) {
		l3dss1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);	/* T304 */
	newl3state(pc, 4);
	if (ret)
		l3dss1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_ALERTING | INDICATION, pc);
}

static void
l3dss1_setup(struct l3_process *pc, u_char pr, void *arg)
{
	u_char *p;
	int bcfound = 0;
	char tmp[80];
	struct sk_buff *skb = arg;
	int id;
	int err = 0;

	/*
	 * Bearer Capabilities
	 */
	p = skb->data;
	/* only the first occurrence 'll be detected ! */
	if ((p = findie(p, skb->len, 0x04, 0))) {
		if ((p[1] < 2) || (p[1] > 11))
			err = 1;
		else {
			pc->para.setup.si2 = 0;
			switch (p[2] & 0x7f) {
			case 0x00: /* Speech */
			case 0x10: /* 3.1 Khz audio */
				pc->para.setup.si1 = 1;
				break;
			case 0x08: /* Unrestricted digital information */
				pc->para.setup.si1 = 7;
/* JIM, 05.11.97 I wanna set service indicator 2 */
#ifdef EXT_BEARER_CAPS
				pc->para.setup.si2 = DecodeSI2(skb);
#endif
				break;
			case 0x09: /* Restricted digital information */
				pc->para.setup.si1 = 2;
				break;
			case 0x11:
				/* Unrestr. digital information  with
				 * tones/announcements ( or 7 kHz audio
				 */
				pc->para.setup.si1 = 3;
				break;
			case 0x18: /* Video */
				pc->para.setup.si1 = 4;
				break;
			default:
				err = 2;
				break;
			}
			switch (p[3] & 0x7f) {
			case 0x40: /* packed mode */
				pc->para.setup.si1 = 8;
				break;
			case 0x10: /* 64 kbit */
			case 0x11: /* 2*64 kbit */
			case 0x13: /* 384 kbit */
			case 0x15: /* 1536 kbit */
			case 0x17: /* 1920 kbit */
				pc->para.moderate = p[3] & 0x7f;
				break;
			default:
				err = 3;
				break;
			}
		}
		if (pc->debug & L3_DEB_SI)
			l3_debug(pc->st, "SI=%d, AI=%d",
				 pc->para.setup.si1, pc->para.setup.si2);
		if (err) {
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "setup with wrong bearer(l=%d:%x,%x)",
					 p[1], p[2], p[3]);
			pc->para.cause = 100;
			l3dss1_msg_without_setup(pc, pr, NULL);
			return;
		}
	} else {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "setup without bearer capabilities");
		/* ETS 300-104 1.3.3 */
		pc->para.cause = 96;
		l3dss1_msg_without_setup(pc, pr, NULL);
		return;
	}
	/*
	 * Channel Identification
	 */
	if ((id = l3dss1_get_channel_id(pc, skb)) >= 0) {
		if ((pc->para.bchannel = id)) {
			if ((3 == id) && (0x10 == pc->para.moderate)) {
				if (pc->debug & L3_DEB_WARN)
					l3_debug(pc->st, "setup with wrong chid %x",
						 id);
				pc->para.cause = 100;
				l3dss1_msg_without_setup(pc, pr, NULL);
				return;
			}
			bcfound++;
		} else
		{ if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "setup without bchannel, call waiting");
			bcfound++;
		}
	} else {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "setup with wrong chid ret %d", id);
		if (id == -1)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3dss1_msg_without_setup(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	err = check_infoelements(pc, skb, ie_SETUP);
	if (ERR_IE_COMPREHENSION == err) {
		pc->para.cause = 96;
		l3dss1_msg_without_setup(pc, pr, NULL);
		return;
	}
	p = skb->data;
	if ((p = findie(p, skb->len, 0x70, 0)))
		iecpy(pc->para.setup.eazmsn, p, 1);
	else
		pc->para.setup.eazmsn[0] = 0;

	p = skb->data;
	if ((p = findie(p, skb->len, 0x71, 0))) {
		/* Called party subaddress */
		if ((p[1] >= 2) && (p[2] == 0x80) && (p[3] == 0x50)) {
			tmp[0] = '.';
			iecpy(&tmp[1], p, 2);
			strcat(pc->para.setup.eazmsn, tmp);
		} else if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "wrong called subaddress");
	}
	p = skb->data;
	if ((p = findie(p, skb->len, 0x6c, 0))) {
		pc->para.setup.plan = p[2];
		if (p[2] & 0x80) {
			iecpy(pc->para.setup.phone, p, 1);
			pc->para.setup.screen = 0;
		} else {
			iecpy(pc->para.setup.phone, p, 2);
			pc->para.setup.screen = p[3];
		}
	} else {
		pc->para.setup.phone[0] = 0;
		pc->para.setup.plan = 0;
		pc->para.setup.screen = 0;
	}
	p = skb->data;
	if ((p = findie(p, skb->len, 0x6d, 0))) {
		/* Calling party subaddress */
		if ((p[1] >= 2) && (p[2] == 0x80) && (p[3] == 0x50)) {
			tmp[0] = '.';
			iecpy(&tmp[1], p, 2);
			strcat(pc->para.setup.phone, tmp);
		} else if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "wrong calling subaddress");
	}
	newl3state(pc, 6);
	if (err) /* STATUS for none mandatory IE errors after actions are taken */
		l3dss1_std_ie_err(pc, err);
	pc->st->l3.l3l4(pc->st, CC_SETUP | INDICATION, pc);
}

static void
l3dss1_reset(struct l3_process *pc, u_char pr, void *arg)
{
	dss1_release_l3_process(pc);
}

static void
l3dss1_disconnect_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[16 + 40];
	u_char *p = tmp;
	int l;
	u_char cause = 16;

	if (pc->para.cause != NO_CAUSE)
		cause = pc->para.cause;

	StopAllL3Timer(pc);

	MsgHead(p, pc->callref, MT_DISCONNECT);

	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = cause | 0x80;

	if (pc->prot.dss1.uus1_data[0])
	{ *p++ = IE_USER_USER; /* UUS info element */
		*p++ = strlen(pc->prot.dss1.uus1_data) + 1;
		*p++ = 0x04; /* IA5 chars */
		strcpy(p, pc->prot.dss1.uus1_data);
		p += strlen(pc->prot.dss1.uus1_data);
		pc->prot.dss1.uus1_data[0] = '\0';
	}

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	newl3state(pc, 11);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	L3AddTimer(&pc->timer, T305, CC_T305);
}

static void
l3dss1_setup_rsp(struct l3_process *pc, u_char pr,
		 void *arg)
{
	if (!pc->para.bchannel)
	{ if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "D-chan connect for waiting call");
		l3dss1_disconnect_req(pc, pr, arg);
		return;
	}
	newl3state(pc, 8);
	l3dss1_message(pc, MT_CONNECT);
	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, T313, CC_T313);
}

static void
l3dss1_connect_ack(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	ret = check_infoelements(pc, skb, ie_CONNECT_ACKNOWLEDGE);
	if (ERR_IE_COMPREHENSION == ret) {
		l3dss1_std_ie_err(pc, ret);
		return;
	}
	newl3state(pc, 10);
	L3DelTimer(&pc->timer);
	if (ret)
		l3dss1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_SETUP_COMPL | INDICATION, pc);
}

static void
l3dss1_reject_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	u_char cause = 21;

	if (pc->para.cause != NO_CAUSE)
		cause = pc->para.cause;

	MsgHead(p, pc->callref, MT_RELEASE_COMPLETE);

	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = cause | 0x80;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
	newl3state(pc, 0);
	dss1_release_l3_process(pc);
}

static void
l3dss1_release(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	u_char *p;
	int ret, cause = 0;

	StopAllL3Timer(pc);
	if ((ret = l3dss1_get_cause(pc, skb)) > 0) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "REL get_cause ret(%d)", ret);
	} else if (ret < 0)
		pc->para.cause = NO_CAUSE;
	if ((p = findie(skb->data, skb->len, IE_FACILITY, 0))) {
		l3dss1_parse_facility(pc->st, pc, pc->callref, p);
	}
	if ((ret < 0) && (pc->state != 11))
		cause = 96;
	else if (ret > 0)
		cause = 100;
	ret = check_infoelements(pc, skb, ie_RELEASE);
	if (ERR_IE_COMPREHENSION == ret)
		cause = 96;
	else if ((ERR_IE_UNRECOGNIZED == ret) && (!cause))
		cause = 99;
	if (cause)
		l3dss1_message_cause(pc, MT_RELEASE_COMPLETE, cause);
	else
		l3dss1_message(pc, MT_RELEASE_COMPLETE);
	pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
	newl3state(pc, 0);
	dss1_release_l3_process(pc);
}

static void
l3dss1_alert_req(struct l3_process *pc, u_char pr,
		 void *arg)
{
	newl3state(pc, 7);
	if (!pc->prot.dss1.uus1_data[0])
		l3dss1_message(pc, MT_ALERTING);
	else
		l3dss1_msg_with_uus(pc, MT_ALERTING);
}

static void
l3dss1_proceed_req(struct l3_process *pc, u_char pr,
		   void *arg)
{
	newl3state(pc, 9);
	l3dss1_message(pc, MT_CALL_PROCEEDING);
	pc->st->l3.l3l4(pc->st, CC_PROCEED_SEND | INDICATION, pc);
}

static void
l3dss1_setup_ack_req(struct l3_process *pc, u_char pr,
		     void *arg)
{
	newl3state(pc, 25);
	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, T302, CC_T302);
	l3dss1_message(pc, MT_SETUP_ACKNOWLEDGE);
}

/********************************************/
/* deliver a incoming display message to HL */
/********************************************/
static void
l3dss1_deliver_display(struct l3_process *pc, int pr, u_char *infp)
{       u_char len;
	isdn_ctrl ic;
	struct IsdnCardState *cs;
	char *p;

	if (*infp++ != IE_DISPLAY) return;
	if ((len = *infp++) > 80) return; /* total length <= 82 */
	if (!pc->chan) return;

	p = ic.parm.display;
	while (len--)
		*p++ = *infp++;
	*p = '\0';
	ic.command = ISDN_STAT_DISPLAY;
	cs = pc->st->l1.hardware;
	ic.driver = cs->myid;
	ic.arg = pc->chan->chan;
	cs->iif.statcallb(&ic);
} /* l3dss1_deliver_display */


static void
l3dss1_progress(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int err = 0;
	u_char *p;

	if ((p = findie(skb->data, skb->len, IE_PROGRESS, 0))) {
		if (p[1] != 2) {
			err = 1;
			pc->para.cause = 100;
		} else if (!(p[2] & 0x70)) {
			switch (p[2]) {
			case 0x80:
			case 0x81:
			case 0x82:
			case 0x84:
			case 0x85:
			case 0x87:
			case 0x8a:
				switch (p[3]) {
				case 0x81:
				case 0x82:
				case 0x83:
				case 0x84:
				case 0x88:
					break;
				default:
					err = 2;
					pc->para.cause = 100;
					break;
				}
				break;
			default:
				err = 3;
				pc->para.cause = 100;
				break;
			}
		}
	} else {
		pc->para.cause = 96;
		err = 4;
	}
	if (err) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "progress error %d", err);
		l3dss1_status_send(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	err = check_infoelements(pc, skb, ie_PROGRESS);
	if (err)
		l3dss1_std_ie_err(pc, err);
	if (ERR_IE_COMPREHENSION != err)
		pc->st->l3.l3l4(pc->st, CC_PROGRESS | INDICATION, pc);
}

static void
l3dss1_notify(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int err = 0;
	u_char *p;

	if ((p = findie(skb->data, skb->len, IE_NOTIFY, 0))) {
		if (p[1] != 1) {
			err = 1;
			pc->para.cause = 100;
		} else {
			switch (p[2]) {
			case 0x80:
			case 0x81:
			case 0x82:
				break;
			default:
				pc->para.cause = 100;
				err = 2;
				break;
			}
		}
	} else {
		pc->para.cause = 96;
		err = 3;
	}
	if (err) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "notify error %d", err);
		l3dss1_status_send(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	err = check_infoelements(pc, skb, ie_NOTIFY);
	if (err)
		l3dss1_std_ie_err(pc, err);
	if (ERR_IE_COMPREHENSION != err)
		pc->st->l3.l3l4(pc->st, CC_NOTIFY | INDICATION, pc);
}

static void
l3dss1_status_enq(struct l3_process *pc, u_char pr, void *arg)
{
	int ret;
	struct sk_buff *skb = arg;

	ret = check_infoelements(pc, skb, ie_STATUS_ENQUIRY);
	l3dss1_std_ie_err(pc, ret);
	pc->para.cause = 30; /* response to STATUS_ENQUIRY */
	l3dss1_status_send(pc, pr, NULL);
}

static void
l3dss1_information(struct l3_process *pc, u_char pr, void *arg)
{
	int ret;
	struct sk_buff *skb = arg;
	u_char *p;
	char tmp[32];

	ret = check_infoelements(pc, skb, ie_INFORMATION);
	if (ret)
		l3dss1_std_ie_err(pc, ret);
	if (pc->state == 25) { /* overlap receiving */
		L3DelTimer(&pc->timer);
		p = skb->data;
		if ((p = findie(p, skb->len, 0x70, 0))) {
			iecpy(tmp, p, 1);
			strcat(pc->para.setup.eazmsn, tmp);
			pc->st->l3.l3l4(pc->st, CC_MORE_INFO | INDICATION, pc);
		}
		L3AddTimer(&pc->timer, T302, CC_T302);
	}
}

/******************************/
/* handle deflection requests */
/******************************/
static void l3dss1_redir_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[128];
	u_char *p = tmp;
	u_char *subp;
	u_char len_phone = 0;
	u_char len_sub = 0;
	int l;


	strcpy(pc->prot.dss1.uus1_data, pc->chan->setup.eazmsn); /* copy uus element if available */
	if (!pc->chan->setup.phone[0])
	{ pc->para.cause = -1;
		l3dss1_disconnect_req(pc, pr, arg); /* disconnect immediately */
		return;
	} /* only uus */

	if (pc->prot.dss1.invoke_id)
		free_invoke_id(pc->st, pc->prot.dss1.invoke_id);

	if (!(pc->prot.dss1.invoke_id = new_invoke_id(pc->st)))
		return;

	MsgHead(p, pc->callref, MT_FACILITY);

	for (subp = pc->chan->setup.phone; (*subp) && (*subp != '.'); subp++) len_phone++; /* len of phone number */
	if (*subp++ == '.') len_sub = strlen(subp) + 2; /* length including info subaddress element */

	*p++ = 0x1c;   /* Facility info element */
	*p++ = len_phone + len_sub + 2 + 2 + 8 + 3 + 3; /* length of element */
	*p++ = 0x91;  /* remote operations protocol */
	*p++ = 0xa1;  /* invoke component */

	*p++ = len_phone + len_sub + 2 + 2 + 8 + 3; /* length of data */
	*p++ = 0x02;  /* invoke id tag, integer */
	*p++ = 0x01;  /* length */
	*p++ = pc->prot.dss1.invoke_id;  /* invoke id */
	*p++ = 0x02;  /* operation value tag, integer */
	*p++ = 0x01;  /* length */
	*p++ = 0x0D;  /* Call Deflect */

	*p++ = 0x30;  /* sequence phone number */
	*p++ = len_phone + 2 + 2 + 3 + len_sub; /* length */

	*p++ = 0x30;  /* Deflected to UserNumber */
	*p++ = len_phone + 2 + len_sub; /* length */
	*p++ = 0x80; /* NumberDigits */
	*p++ = len_phone; /* length */
	for (l = 0; l < len_phone; l++)
		*p++ = pc->chan->setup.phone[l];

	if (len_sub)
	{ *p++ = 0x04; /* called party subaddress */
		*p++ = len_sub - 2;
		while (*subp) *p++ = *subp++;
	}

	*p++ = 0x01; /* screening identifier */
	*p++ = 0x01;
	*p++ = pc->chan->setup.screen;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l))) return;
	memcpy(skb_put(skb, l), tmp, l);

	l3_msg(pc->st, DL_DATA | REQUEST, skb);
} /* l3dss1_redir_req */

/********************************************/
/* handle deflection request in early state */
/********************************************/
static void l3dss1_redir_req_early(struct l3_process *pc, u_char pr, void *arg)
{
	l3dss1_proceed_req(pc, pr, arg);
	l3dss1_redir_req(pc, pr, arg);
} /* l3dss1_redir_req_early */

/***********************************************/
/* handle special commands for this protocol.  */
/* Examples are call independent services like */
/* remote operations with dummy  callref.      */
/***********************************************/
static int l3dss1_cmd_global(struct PStack *st, isdn_ctrl *ic)
{ u_char id;
	u_char temp[265];
	u_char *p = temp;
	int i, l, proc_len;
	struct sk_buff *skb;
	struct l3_process *pc = NULL;

	switch (ic->arg)
	{ case DSS1_CMD_INVOKE:
			if (ic->parm.dss1_io.datalen < 0) return (-2); /* invalid parameter */

			for (proc_len = 1, i = ic->parm.dss1_io.proc >> 8; i; i++)
				i = i >> 8; /* add one byte */
			l = ic->parm.dss1_io.datalen + proc_len + 8; /* length excluding ie header */
			if (l > 255)
				return (-2); /* too long */

			if (!(id = new_invoke_id(st)))
				return (0); /* first get a invoke id -> return if no available */

			i = -1;
			MsgHead(p, i, MT_FACILITY); /* build message head */
			*p++ = 0x1C; /* Facility IE */
			*p++ = l; /* length of ie */
			*p++ = 0x91; /* remote operations */
			*p++ = 0xA1; /* invoke */
			*p++ = l - 3; /* length of invoke */
			*p++ = 0x02; /* invoke id tag */
			*p++ = 0x01; /* length is 1 */
			*p++ = id; /* invoke id */
			*p++ = 0x02; /* operation */
			*p++ = proc_len; /* length of operation */

			for (i = proc_len; i; i--)
				*p++ = (ic->parm.dss1_io.proc >> (i - 1)) & 0xFF;
			memcpy(p, ic->parm.dss1_io.data, ic->parm.dss1_io.datalen); /* copy data */
			l = (p - temp) + ic->parm.dss1_io.datalen; /* total length */

			if (ic->parm.dss1_io.timeout > 0)
				if (!(pc = dss1_new_l3_process(st, -1)))
				{ free_invoke_id(st, id);
					return (-2);
				}
			pc->prot.dss1.ll_id = ic->parm.dss1_io.ll_id; /* remember id */
			pc->prot.dss1.proc = ic->parm.dss1_io.proc; /* and procedure */

			if (!(skb = l3_alloc_skb(l)))
			{ free_invoke_id(st, id);
				if (pc) dss1_release_l3_process(pc);
				return (-2);
			}
			memcpy(skb_put(skb, l), temp, l);

			if (pc)
			{ pc->prot.dss1.invoke_id = id; /* remember id */
				L3AddTimer(&pc->timer, ic->parm.dss1_io.timeout, CC_TDSS1_IO | REQUEST);
			}

			l3_msg(st, DL_DATA | REQUEST, skb);
			ic->parm.dss1_io.hl_id = id; /* return id */
			return (0);

	case DSS1_CMD_INVOKE_ABORT:
		if ((pc = l3dss1_search_dummy_proc(st, ic->parm.dss1_io.hl_id)))
		{ L3DelTimer(&pc->timer); /* remove timer */
			dss1_release_l3_process(pc);
			return (0);
		}
		else
		{ l3_debug(st, "l3dss1_cmd_global abort unknown id");
			return (-2);
		}
		break;

	default:
		l3_debug(st, "l3dss1_cmd_global unknown cmd 0x%lx", ic->arg);
		return (-1);
	} /* switch ic-> arg */
	return (-1);
} /* l3dss1_cmd_global */

static void
l3dss1_io_timer(struct l3_process *pc)
{ isdn_ctrl ic;
	struct IsdnCardState *cs = pc->st->l1.hardware;

	L3DelTimer(&pc->timer); /* remove timer */

	ic.driver = cs->myid;
	ic.command = ISDN_STAT_PROT;
	ic.arg = DSS1_STAT_INVOKE_ERR;
	ic.parm.dss1_io.hl_id = pc->prot.dss1.invoke_id;
	ic.parm.dss1_io.ll_id = pc->prot.dss1.ll_id;
	ic.parm.dss1_io.proc = pc->prot.dss1.proc;
	ic.parm.dss1_io.timeout = -1;
	ic.parm.dss1_io.datalen = 0;
	ic.parm.dss1_io.data = NULL;
	free_invoke_id(pc->st, pc->prot.dss1.invoke_id);
	pc->prot.dss1.invoke_id = 0; /* reset id */

	cs->iif.statcallb(&ic);

	dss1_release_l3_process(pc);
} /* l3dss1_io_timer */

static void
l3dss1_release_ind(struct l3_process *pc, u_char pr, void *arg)
{
	u_char *p;
	struct sk_buff *skb = arg;
	int callState = 0;
	p = skb->data;

	if ((p = findie(p, skb->len, IE_CALL_STATE, 0))) {
		p++;
		if (1 == *p++)
			callState = *p;
	}
	if (callState == 0) {
		/* ETS 300-104 7.6.1, 8.6.1, 10.6.1... and 16.1
		 * set down layer 3 without sending any message
		 */
		pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
		newl3state(pc, 0);
		dss1_release_l3_process(pc);
	} else {
		pc->st->l3.l3l4(pc->st, CC_IGNORE | INDICATION, pc);
	}
}

static void
l3dss1_dummy(struct l3_process *pc, u_char pr, void *arg)
{
}

static void
l3dss1_t302(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.loc = 0;
	pc->para.cause = 28; /* invalid number */
	l3dss1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_SETUP_ERR, pc);
}

static void
l3dss1_t303(struct l3_process *pc, u_char pr, void *arg)
{
	if (pc->N303 > 0) {
		pc->N303--;
		L3DelTimer(&pc->timer);
		l3dss1_setup_req(pc, pr, arg);
	} else {
		L3DelTimer(&pc->timer);
		l3dss1_message_cause(pc, MT_RELEASE_COMPLETE, 102);
		pc->st->l3.l3l4(pc->st, CC_NOSETUP_RSP, pc);
		dss1_release_l3_process(pc);
	}
}

static void
l3dss1_t304(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.loc = 0;
	pc->para.cause = 102;
	l3dss1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_SETUP_ERR, pc);

}

static void
l3dss1_t305(struct l3_process *pc, u_char pr, void *arg)
{
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	struct sk_buff *skb;
	u_char cause = 16;

	L3DelTimer(&pc->timer);
	if (pc->para.cause != NO_CAUSE)
		cause = pc->para.cause;

	MsgHead(p, pc->callref, MT_RELEASE);

	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = cause | 0x80;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	newl3state(pc, 19);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	L3AddTimer(&pc->timer, T308, CC_T308_1);
}

static void
l3dss1_t310(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.loc = 0;
	pc->para.cause = 102;
	l3dss1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_SETUP_ERR, pc);
}

static void
l3dss1_t313(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.loc = 0;
	pc->para.cause = 102;
	l3dss1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_CONNECT_ERR, pc);
}

static void
l3dss1_t308_1(struct l3_process *pc, u_char pr, void *arg)
{
	newl3state(pc, 19);
	L3DelTimer(&pc->timer);
	l3dss1_message(pc, MT_RELEASE);
	L3AddTimer(&pc->timer, T308, CC_T308_2);
}

static void
l3dss1_t308_2(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_RELEASE_ERR, pc);
	dss1_release_l3_process(pc);
}

static void
l3dss1_t318(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.cause = 102;	/* Timer expiry */
	pc->para.loc = 0;	/* local */
	pc->st->l3.l3l4(pc->st, CC_RESUME_ERR, pc);
	newl3state(pc, 19);
	l3dss1_message(pc, MT_RELEASE);
	L3AddTimer(&pc->timer, T308, CC_T308_1);
}

static void
l3dss1_t319(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.cause = 102;	/* Timer expiry */
	pc->para.loc = 0;	/* local */
	pc->st->l3.l3l4(pc->st, CC_SUSPEND_ERR, pc);
	newl3state(pc, 10);
}

static void
l3dss1_restart(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
	dss1_release_l3_process(pc);
}

static void
l3dss1_status(struct l3_process *pc, u_char pr, void *arg)
{
	u_char *p;
	struct sk_buff *skb = arg;
	int ret;
	u_char cause = 0, callState = 0;

	if ((ret = l3dss1_get_cause(pc, skb))) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "STATUS get_cause ret(%d)", ret);
		if (ret < 0)
			cause = 96;
		else if (ret > 0)
			cause = 100;
	}
	if ((p = findie(skb->data, skb->len, IE_CALL_STATE, 0))) {
		p++;
		if (1 == *p++) {
			callState = *p;
			if (!ie_in_set(pc, *p, l3_valid_states))
				cause = 100;
		} else
			cause = 100;
	} else
		cause = 96;
	if (!cause) { /*  no error before */
		ret = check_infoelements(pc, skb, ie_STATUS);
		if (ERR_IE_COMPREHENSION == ret)
			cause = 96;
		else if (ERR_IE_UNRECOGNIZED == ret)
			cause = 99;
	}
	if (cause) {
		u_char tmp;

		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "STATUS error(%d/%d)", ret, cause);
		tmp = pc->para.cause;
		pc->para.cause = cause;
		l3dss1_status_send(pc, 0, NULL);
		if (cause == 99)
			pc->para.cause = tmp;
		else
			return;
	}
	cause = pc->para.cause;
	if (((cause & 0x7f) == 111) && (callState == 0)) {
		/* ETS 300-104 7.6.1, 8.6.1, 10.6.1...
		 * if received MT_STATUS with cause == 111 and call
		 * state == 0, then we must set down layer 3
		 */
		pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
		newl3state(pc, 0);
		dss1_release_l3_process(pc);
	}
}

static void
l3dss1_facility(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	ret = check_infoelements(pc, skb, ie_FACILITY);
	l3dss1_std_ie_err(pc, ret);
	{
		u_char *p;
		if ((p = findie(skb->data, skb->len, IE_FACILITY, 0)))
			l3dss1_parse_facility(pc->st, pc, pc->callref, p);
	}
}

static void
l3dss1_suspend_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[32];
	u_char *p = tmp;
	u_char i, l;
	u_char *msg = pc->chan->setup.phone;

	MsgHead(p, pc->callref, MT_SUSPEND);
	l = *msg++;
	if (l && (l <= 10)) {	/* Max length 10 octets */
		*p++ = IE_CALL_ID;
		*p++ = l;
		for (i = 0; i < l; i++)
			*p++ = *msg++;
	} else if (l) {
		l3_debug(pc->st, "SUS wrong CALL_ID len %d", l);
		return;
	}
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	newl3state(pc, 15);
	L3AddTimer(&pc->timer, T319, CC_T319);
}

static void
l3dss1_suspend_ack(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	L3DelTimer(&pc->timer);
	newl3state(pc, 0);
	pc->para.cause = NO_CAUSE;
	pc->st->l3.l3l4(pc->st, CC_SUSPEND | CONFIRM, pc);
	/* We don't handle suspend_ack for IE errors now */
	if ((ret = check_infoelements(pc, skb, ie_SUSPEND_ACKNOWLEDGE)))
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "SUSPACK check ie(%d)", ret);
	dss1_release_l3_process(pc);
}

static void
l3dss1_suspend_rej(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	if ((ret = l3dss1_get_cause(pc, skb))) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "SUSP_REJ get_cause ret(%d)", ret);
		if (ret < 0)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3dss1_status_send(pc, pr, NULL);
		return;
	}
	ret = check_infoelements(pc, skb, ie_SUSPEND_REJECT);
	if (ERR_IE_COMPREHENSION == ret) {
		l3dss1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_SUSPEND_ERR, pc);
	newl3state(pc, 10);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3dss1_std_ie_err(pc, ret);
}

static void
l3dss1_resume_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[32];
	u_char *p = tmp;
	u_char i, l;
	u_char *msg = pc->para.setup.phone;

	MsgHead(p, pc->callref, MT_RESUME);

	l = *msg++;
	if (l && (l <= 10)) {	/* Max length 10 octets */
		*p++ = IE_CALL_ID;
		*p++ = l;
		for (i = 0; i < l; i++)
			*p++ = *msg++;
	} else if (l) {
		l3_debug(pc->st, "RES wrong CALL_ID len %d", l);
		return;
	}
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	newl3state(pc, 17);
	L3AddTimer(&pc->timer, T318, CC_T318);
}

static void
l3dss1_resume_ack(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int id, ret;

	if ((id = l3dss1_get_channel_id(pc, skb)) > 0) {
		if ((0 == id) || ((3 == id) && (0x10 == pc->para.moderate))) {
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "resume ack with wrong chid %x", id);
			pc->para.cause = 100;
			l3dss1_status_send(pc, pr, NULL);
			return;
		}
		pc->para.bchannel = id;
	} else if (1 == pc->state) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "resume ack without chid (ret %d)", id);
		pc->para.cause = 96;
		l3dss1_status_send(pc, pr, NULL);
		return;
	}
	ret = check_infoelements(pc, skb, ie_RESUME_ACKNOWLEDGE);
	if (ERR_IE_COMPREHENSION == ret) {
		l3dss1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_RESUME | CONFIRM, pc);
	newl3state(pc, 10);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3dss1_std_ie_err(pc, ret);
}

static void
l3dss1_resume_rej(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	if ((ret = l3dss1_get_cause(pc, skb))) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "RES_REJ get_cause ret(%d)", ret);
		if (ret < 0)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3dss1_status_send(pc, pr, NULL);
		return;
	}
	ret = check_infoelements(pc, skb, ie_RESUME_REJECT);
	if (ERR_IE_COMPREHENSION == ret) {
		l3dss1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_RESUME_ERR, pc);
	newl3state(pc, 0);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3dss1_std_ie_err(pc, ret);
	dss1_release_l3_process(pc);
}

static void
l3dss1_global_restart(struct l3_process *pc, u_char pr, void *arg)
{
	u_char tmp[32];
	u_char *p;
	u_char ri, ch = 0, chan = 0;
	int l;
	struct sk_buff *skb = arg;
	struct l3_process *up;

	newl3state(pc, 2);
	L3DelTimer(&pc->timer);
	p = skb->data;
	if ((p = findie(p, skb->len, IE_RESTART_IND, 0))) {
		ri = p[2];
		l3_debug(pc->st, "Restart %x", ri);
	} else {
		l3_debug(pc->st, "Restart without restart IE");
		ri = 0x86;
	}
	p = skb->data;
	if ((p = findie(p, skb->len, IE_CHANNEL_ID, 0))) {
		chan = p[2] & 3;
		ch = p[2];
		if (pc->st->l3.debug)
			l3_debug(pc->st, "Restart for channel %d", chan);
	}
	newl3state(pc, 2);
	up = pc->st->l3.proc;
	while (up) {
		if ((ri & 7) == 7)
			up->st->lli.l4l3(up->st, CC_RESTART | REQUEST, up);
		else if (up->para.bchannel == chan)
			up->st->lli.l4l3(up->st, CC_RESTART | REQUEST, up);
		up = up->next;
	}
	p = tmp;
	MsgHead(p, pc->callref, MT_RESTART_ACKNOWLEDGE);
	if (chan) {
		*p++ = IE_CHANNEL_ID;
		*p++ = 1;
		*p++ = ch | 0x80;
	}
	*p++ = 0x79;		/* RESTART Ind */
	*p++ = 1;
	*p++ = ri;
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	newl3state(pc, 0);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3dss1_dl_reset(struct l3_process *pc, u_char pr, void *arg)
{
	pc->para.cause = 0x29;          /* Temporary failure */
	pc->para.loc = 0;
	l3dss1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_SETUP_ERR, pc);
}

static void
l3dss1_dl_release(struct l3_process *pc, u_char pr, void *arg)
{
	newl3state(pc, 0);
	pc->para.cause = 0x1b;          /* Destination out of order */
	pc->para.loc = 0;
	pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
	release_l3_process(pc);
}

static void
l3dss1_dl_reestablish(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, T309, CC_T309);
	l3_msg(pc->st, DL_ESTABLISH | REQUEST, NULL);
}

static void
l3dss1_dl_reest_status(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);

	pc->para.cause = 0x1F; /* normal, unspecified */
	l3dss1_status_send(pc, 0, NULL);
}

/* *INDENT-OFF* */
static struct stateentry downstatelist[] =
{
	{SBIT(0),
	 CC_SETUP | REQUEST, l3dss1_setup_req},
	{SBIT(0),
	 CC_RESUME | REQUEST, l3dss1_resume_req},
	{SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4) | SBIT(6) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) | SBIT(25),
	 CC_DISCONNECT | REQUEST, l3dss1_disconnect_req},
	{SBIT(12),
	 CC_RELEASE | REQUEST, l3dss1_release_req},
	{ALL_STATES,
	 CC_RESTART | REQUEST, l3dss1_restart},
	{SBIT(6) | SBIT(25),
	 CC_IGNORE | REQUEST, l3dss1_reset},
	{SBIT(6) | SBIT(25),
	 CC_REJECT | REQUEST, l3dss1_reject_req},
	{SBIT(6) | SBIT(25),
	 CC_PROCEED_SEND | REQUEST, l3dss1_proceed_req},
	{SBIT(6),
	 CC_MORE_INFO | REQUEST, l3dss1_setup_ack_req},
	{SBIT(25),
	 CC_MORE_INFO | REQUEST, l3dss1_dummy},
	{SBIT(6) | SBIT(9) | SBIT(25),
	 CC_ALERTING | REQUEST, l3dss1_alert_req},
	{SBIT(6) | SBIT(7) | SBIT(9) | SBIT(25),
	 CC_SETUP | RESPONSE, l3dss1_setup_rsp},
	{SBIT(10),
	 CC_SUSPEND | REQUEST, l3dss1_suspend_req},
	{SBIT(7) | SBIT(9) | SBIT(25),
	 CC_REDIR | REQUEST, l3dss1_redir_req},
	{SBIT(6),
	 CC_REDIR | REQUEST, l3dss1_redir_req_early},
	{SBIT(9) | SBIT(25),
	 CC_DISCONNECT | REQUEST, l3dss1_disconnect_req},
	{SBIT(25),
	 CC_T302, l3dss1_t302},
	{SBIT(1),
	 CC_T303, l3dss1_t303},
	{SBIT(2),
	 CC_T304, l3dss1_t304},
	{SBIT(3),
	 CC_T310, l3dss1_t310},
	{SBIT(8),
	 CC_T313, l3dss1_t313},
	{SBIT(11),
	 CC_T305, l3dss1_t305},
	{SBIT(15),
	 CC_T319, l3dss1_t319},
	{SBIT(17),
	 CC_T318, l3dss1_t318},
	{SBIT(19),
	 CC_T308_1, l3dss1_t308_1},
	{SBIT(19),
	 CC_T308_2, l3dss1_t308_2},
	{SBIT(10),
	 CC_T309, l3dss1_dl_release},
};

static struct stateentry datastatelist[] =
{
	{ALL_STATES,
	 MT_STATUS_ENQUIRY, l3dss1_status_enq},
	{ALL_STATES,
	 MT_FACILITY, l3dss1_facility},
	{SBIT(19),
	 MT_STATUS, l3dss1_release_ind},
	{ALL_STATES,
	 MT_STATUS, l3dss1_status},
	{SBIT(0),
	 MT_SETUP, l3dss1_setup},
	{SBIT(6) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) | SBIT(11) | SBIT(12) |
	 SBIT(15) | SBIT(17) | SBIT(19) | SBIT(25),
	 MT_SETUP, l3dss1_dummy},
	{SBIT(1) | SBIT(2),
	 MT_CALL_PROCEEDING, l3dss1_call_proc},
	{SBIT(1),
	 MT_SETUP_ACKNOWLEDGE, l3dss1_setup_ack},
	{SBIT(2) | SBIT(3),
	 MT_ALERTING, l3dss1_alerting},
	{SBIT(2) | SBIT(3),
	 MT_PROGRESS, l3dss1_progress},
	{SBIT(2) | SBIT(3) | SBIT(4) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) |
	 SBIT(11) | SBIT(12) | SBIT(15) | SBIT(17) | SBIT(19) | SBIT(25),
	 MT_INFORMATION, l3dss1_information},
	{SBIT(10) | SBIT(11) | SBIT(15),
	 MT_NOTIFY, l3dss1_notify},
	{SBIT(0) | SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4) | SBIT(7) | SBIT(8) | SBIT(10) |
	 SBIT(11) | SBIT(12) | SBIT(15) | SBIT(17) | SBIT(19) | SBIT(25),
	 MT_RELEASE_COMPLETE, l3dss1_release_cmpl},
	{SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) | SBIT(11) | SBIT(12) | SBIT(15) | SBIT(17) | SBIT(25),
	 MT_RELEASE, l3dss1_release},
	{SBIT(19),  MT_RELEASE, l3dss1_release_ind},
	{SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) | SBIT(11) | SBIT(15) | SBIT(17) | SBIT(25),
	 MT_DISCONNECT, l3dss1_disconnect},
	{SBIT(19),
	 MT_DISCONNECT, l3dss1_dummy},
	{SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4),
	 MT_CONNECT, l3dss1_connect},
	{SBIT(8),
	 MT_CONNECT_ACKNOWLEDGE, l3dss1_connect_ack},
	{SBIT(15),
	 MT_SUSPEND_ACKNOWLEDGE, l3dss1_suspend_ack},
	{SBIT(15),
	 MT_SUSPEND_REJECT, l3dss1_suspend_rej},
	{SBIT(17),
	 MT_RESUME_ACKNOWLEDGE, l3dss1_resume_ack},
	{SBIT(17),
	 MT_RESUME_REJECT, l3dss1_resume_rej},
};

static struct stateentry globalmes_list[] =
{
	{ALL_STATES,
	 MT_STATUS, l3dss1_status},
	{SBIT(0),
	 MT_RESTART, l3dss1_global_restart},
/*	{SBIT(1),
	MT_RESTART_ACKNOWLEDGE, l3dss1_restart_ack},
*/
};

static struct stateentry manstatelist[] =
{
	{SBIT(2),
	 DL_ESTABLISH | INDICATION, l3dss1_dl_reset},
	{SBIT(10),
	 DL_ESTABLISH | CONFIRM, l3dss1_dl_reest_status},
	{SBIT(10),
	 DL_RELEASE | INDICATION, l3dss1_dl_reestablish},
	{ALL_STATES,
	 DL_RELEASE | INDICATION, l3dss1_dl_release},
};

/* *INDENT-ON* */


static void
global_handler(struct PStack *st, int mt, struct sk_buff *skb)
{
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	int i;
	struct l3_process *proc = st->l3.global;

	proc->callref = skb->data[2]; /* cr flag */
	for (i = 0; i < ARRAY_SIZE(globalmes_list); i++)
		if ((mt == globalmes_list[i].primitive) &&
		    ((1 << proc->state) & globalmes_list[i].state))
			break;
	if (i == ARRAY_SIZE(globalmes_list)) {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "dss1 global state %d mt %x unhandled",
				 proc->state, mt);
		}
		MsgHead(p, proc->callref, MT_STATUS);
		*p++ = IE_CAUSE;
		*p++ = 0x2;
		*p++ = 0x80;
		*p++ = 81 | 0x80;	/* invalid cr */
		*p++ = 0x14;		/* CallState */
		*p++ = 0x1;
		*p++ = proc->state & 0x3f;
		l = p - tmp;
		if (!(skb = l3_alloc_skb(l)))
			return;
		memcpy(skb_put(skb, l), tmp, l);
		l3_msg(proc->st, DL_DATA | REQUEST, skb);
	} else {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "dss1 global %d mt %x",
				 proc->state, mt);
		}
		globalmes_list[i].rout(proc, mt, skb);
	}
}

static void
dss1up(struct PStack *st, int pr, void *arg)
{
	int i, mt, cr, callState;
	char *ptr;
	u_char *p;
	struct sk_buff *skb = arg;
	struct l3_process *proc;

	switch (pr) {
	case (DL_DATA | INDICATION):
	case (DL_UNIT_DATA | INDICATION):
		break;
	case (DL_ESTABLISH | CONFIRM):
	case (DL_ESTABLISH | INDICATION):
	case (DL_RELEASE | INDICATION):
	case (DL_RELEASE | CONFIRM):
		l3_msg(st, pr, arg);
		return;
		break;
	default:
		printk(KERN_ERR "HiSax dss1up unknown pr=%04x\n", pr);
		return;
	}
	if (skb->len < 3) {
		l3_debug(st, "dss1up frame too short(%d)", skb->len);
		dev_kfree_skb(skb);
		return;
	}

	if (skb->data[0] != PROTO_DIS_EURO) {
		if (st->l3.debug & L3_DEB_PROTERR) {
			l3_debug(st, "dss1up%sunexpected discriminator %x message len %d",
				 (pr == (DL_DATA | INDICATION)) ? " " : "(broadcast) ",
				 skb->data[0], skb->len);
		}
		dev_kfree_skb(skb);
		return;
	}
	cr = getcallref(skb->data);
	if (skb->len < ((skb->data[1] & 0x0f) + 3)) {
		l3_debug(st, "dss1up frame too short(%d)", skb->len);
		dev_kfree_skb(skb);
		return;
	}
	mt = skb->data[skb->data[1] + 2];
	if (st->l3.debug & L3_DEB_STATE)
		l3_debug(st, "dss1up cr %d", cr);
	if (cr == -2) {  /* wrong Callref */
		if (st->l3.debug & L3_DEB_WARN)
			l3_debug(st, "dss1up wrong Callref");
		dev_kfree_skb(skb);
		return;
	} else if (cr == -1) {	/* Dummy Callref */
		if (mt == MT_FACILITY)
			if ((p = findie(skb->data, skb->len, IE_FACILITY, 0))) {
				l3dss1_parse_facility(st, NULL,
						      (pr == (DL_DATA | INDICATION)) ? -1 : -2, p);
				dev_kfree_skb(skb);
				return;
			}
		if (st->l3.debug & L3_DEB_WARN)
			l3_debug(st, "dss1up dummy Callref (no facility msg or ie)");
		dev_kfree_skb(skb);
		return;
	} else if ((((skb->data[1] & 0x0f) == 1) && (0 == (cr & 0x7f))) ||
		   (((skb->data[1] & 0x0f) == 2) && (0 == (cr & 0x7fff)))) {	/* Global CallRef */
		if (st->l3.debug & L3_DEB_STATE)
			l3_debug(st, "dss1up Global CallRef");
		global_handler(st, mt, skb);
		dev_kfree_skb(skb);
		return;
	} else if (!(proc = getl3proc(st, cr))) {
		/* No transaction process exist, that means no call with
		 * this callreference is active
		 */
		if (mt == MT_SETUP) {
			/* Setup creates a new transaction process */
			if (skb->data[2] & 0x80) {
				/* Setup with wrong CREF flag */
				if (st->l3.debug & L3_DEB_STATE)
					l3_debug(st, "dss1up wrong CRef flag");
				dev_kfree_skb(skb);
				return;
			}
			if (!(proc = dss1_new_l3_process(st, cr))) {
				/* May be to answer with RELEASE_COMPLETE and
				 * CAUSE 0x2f "Resource unavailable", but this
				 * need a new_l3_process too ... arghh
				 */
				dev_kfree_skb(skb);
				return;
			}
		} else if (mt == MT_STATUS) {
			if ((ptr = findie(skb->data, skb->len, IE_CAUSE, 0)) != NULL) {
				ptr++;
				if (*ptr++ == 2)
					ptr++;
			}
			callState = 0;
			if ((ptr = findie(skb->data, skb->len, IE_CALL_STATE, 0)) != NULL) {
				ptr++;
				if (*ptr++ == 2)
					ptr++;
				callState = *ptr;
			}
			/* ETS 300-104 part 2.4.1
			 * if setup has not been made and a message type
			 * MT_STATUS is received with call state == 0,
			 * we must send nothing
			 */
			if (callState != 0) {
				/* ETS 300-104 part 2.4.2
				 * if setup has not been made and a message type
				 * MT_STATUS is received with call state != 0,
				 * we must send MT_RELEASE_COMPLETE cause 101
				 */
				if ((proc = dss1_new_l3_process(st, cr))) {
					proc->para.cause = 101;
					l3dss1_msg_without_setup(proc, 0, NULL);
				}
			}
			dev_kfree_skb(skb);
			return;
		} else if (mt == MT_RELEASE_COMPLETE) {
			dev_kfree_skb(skb);
			return;
		} else {
			/* ETS 300-104 part 2
			 * if setup has not been made and a message type
			 * (except MT_SETUP and RELEASE_COMPLETE) is received,
			 * we must send MT_RELEASE_COMPLETE cause 81 */
			dev_kfree_skb(skb);
			if ((proc = dss1_new_l3_process(st, cr))) {
				proc->para.cause = 81;
				l3dss1_msg_without_setup(proc, 0, NULL);
			}
			return;
		}
	}
	if (l3dss1_check_messagetype_validity(proc, mt, skb)) {
		dev_kfree_skb(skb);
		return;
	}
	if ((p = findie(skb->data, skb->len, IE_DISPLAY, 0)) != NULL)
		l3dss1_deliver_display(proc, pr, p); /* Display IE included */
	for (i = 0; i < ARRAY_SIZE(datastatelist); i++)
		if ((mt == datastatelist[i].primitive) &&
		    ((1 << proc->state) & datastatelist[i].state))
			break;
	if (i == ARRAY_SIZE(datastatelist)) {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "dss1up%sstate %d mt %#x unhandled",
				 (pr == (DL_DATA | INDICATION)) ? " " : "(broadcast) ",
				 proc->state, mt);
		}
		if ((MT_RELEASE_COMPLETE != mt) && (MT_RELEASE != mt)) {
			proc->para.cause = 101;
			l3dss1_status_send(proc, pr, skb);
		}
	} else {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "dss1up%sstate %d mt %x",
				 (pr == (DL_DATA | INDICATION)) ? " " : "(broadcast) ",
				 proc->state, mt);
		}
		datastatelist[i].rout(proc, pr, skb);
	}
	dev_kfree_skb(skb);
	return;
}

static void
dss1down(struct PStack *st, int pr, void *arg)
{
	int i, cr;
	struct l3_process *proc;
	struct Channel *chan;

	if ((DL_ESTABLISH | REQUEST) == pr) {
		l3_msg(st, pr, NULL);
		return;
	} else if (((CC_SETUP | REQUEST) == pr) || ((CC_RESUME | REQUEST) == pr)) {
		chan = arg;
		cr = newcallref();
		cr |= 0x80;
		if ((proc = dss1_new_l3_process(st, cr))) {
			proc->chan = chan;
			chan->proc = proc;
			memcpy(&proc->para.setup, &chan->setup, sizeof(setup_parm));
			proc->callref = cr;
		}
	} else {
		proc = arg;
	}
	if (!proc) {
		printk(KERN_ERR "HiSax dss1down without proc pr=%04x\n", pr);
		return;
	}

	if (pr == (CC_TDSS1_IO | REQUEST)) {
		l3dss1_io_timer(proc); /* timer expires */
		return;
	}

	for (i = 0; i < ARRAY_SIZE(downstatelist); i++)
		if ((pr == downstatelist[i].primitive) &&
		    ((1 << proc->state) & downstatelist[i].state))
			break;
	if (i == ARRAY_SIZE(downstatelist)) {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "dss1down state %d prim %#x unhandled",
				 proc->state, pr);
		}
	} else {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "dss1down state %d prim %#x",
				 proc->state, pr);
		}
		downstatelist[i].rout(proc, pr, arg);
	}
}

static void
dss1man(struct PStack *st, int pr, void *arg)
{
	int i;
	struct l3_process *proc = arg;

	if (!proc) {
		printk(KERN_ERR "HiSax dss1man without proc pr=%04x\n", pr);
		return;
	}
	for (i = 0; i < ARRAY_SIZE(manstatelist); i++)
		if ((pr == manstatelist[i].primitive) &&
		    ((1 << proc->state) & manstatelist[i].state))
			break;
	if (i == ARRAY_SIZE(manstatelist)) {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "cr %d dss1man state %d prim %#x unhandled",
				 proc->callref & 0x7f, proc->state, pr);
		}
	} else {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "cr %d dss1man state %d prim %#x",
				 proc->callref & 0x7f, proc->state, pr);
		}
		manstatelist[i].rout(proc, pr, arg);
	}
}

void
setstack_dss1(struct PStack *st)
{
	char tmp[64];
	int i;

	st->lli.l4l3 = dss1down;
	st->lli.l4l3_proto = l3dss1_cmd_global;
	st->l2.l2l3 = dss1up;
	st->l3.l3ml3 = dss1man;
	st->l3.N303 = 1;
	st->prot.dss1.last_invoke_id = 0;
	st->prot.dss1.invoke_used[0] = 1; /* Bit 0 must always be set to 1 */
	i = 1;
	while (i < 32)
		st->prot.dss1.invoke_used[i++] = 0;

	if (!(st->l3.global = kmalloc(sizeof(struct l3_process), GFP_ATOMIC))) {
		printk(KERN_ERR "HiSax can't get memory for dss1 global CR\n");
	} else {
		st->l3.global->state = 0;
		st->l3.global->callref = 0;
		st->l3.global->next = NULL;
		st->l3.global->debug = L3_DEB_WARN;
		st->l3.global->st = st;
		st->l3.global->N303 = 1;
		st->l3.global->prot.dss1.invoke_id = 0;

		L3InitTimer(st->l3.global, &st->l3.global->timer);
	}
	strcpy(tmp, dss1_revision);
	printk(KERN_INFO "HiSax: DSS1 Rev. %s\n", HiSax_getrev(tmp));
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /* $Id: l3ni1.c,v 2.8.2.3 2004/01/13 14:31:25 keil Exp $
 *
 * NI1 D-channel protocol
 *
 * Author       Matt Henderson & Guy Ellis
 * Copyright    by Traverse Technologies Pty Ltd, www.travers.com.au
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * 2000.6.6 Initial implementation of routines for US NI1
 * Layer 3 protocol based on the EURO/DSS1 D-channel protocol
 * driver written by Karsten Keil et al.
 * NI-1 Hall of Fame - Thanks to....
 * Ragnar Paulson - for some handy code fragments
 * Will Scales - beta tester extraordinaire
 * Brett Whittacre - beta tester and remote devel system in Vegas
 *
 */

#include "hisax.h"
#include "isdnl3.h"
#include "l3ni1.h"
#include <linux/ctype.h>
#include <linux/slab.h>

extern char *HiSax_getrev(const char *revision);
static const char *ni1_revision = "$Revision: 2.8.2.3 $";

#define EXT_BEARER_CAPS 1

#define	MsgHead(ptr, cref, mty)			\
	*ptr++ = 0x8;				\
	if (cref == -1) {			\
		*ptr++ = 0x0;			\
	} else {				\
		*ptr++ = 0x1;			\
		*ptr++ = cref^0x80;		\
	}					\
	*ptr++ = mty


/**********************************************/
/* get a new invoke id for remote operations. */
/* Only a return value != 0 is valid          */
/**********************************************/
static unsigned char new_invoke_id(struct PStack *p)
{
	unsigned char retval;
	int i;

	i = 32; /* maximum search depth */

	retval = p->prot.ni1.last_invoke_id + 1; /* try new id */
	while ((i) && (p->prot.ni1.invoke_used[retval >> 3] == 0xFF)) {
		p->prot.ni1.last_invoke_id = (retval & 0xF8) + 8;
		i--;
	}
	if (i) {
		while (p->prot.ni1.invoke_used[retval >> 3] & (1 << (retval & 7)))
			retval++;
	} else
		retval = 0;
	p->prot.ni1.last_invoke_id = retval;
	p->prot.ni1.invoke_used[retval >> 3] |= (1 << (retval & 7));
	return (retval);
} /* new_invoke_id */

/*************************/
/* free a used invoke id */
/*************************/
static void free_invoke_id(struct PStack *p, unsigned char id)
{

	if (!id) return; /* 0 = invalid value */

	p->prot.ni1.invoke_used[id >> 3] &= ~(1 << (id & 7));
} /* free_invoke_id */


/**********************************************************/
/* create a new l3 process and fill in ni1 specific data */
/**********************************************************/
static struct l3_process
*ni1_new_l3_process(struct PStack *st, int cr)
{  struct l3_process *proc;

	if (!(proc = new_l3_process(st, cr)))
		return (NULL);

	proc->prot.ni1.invoke_id = 0;
	proc->prot.ni1.remote_operation = 0;
	proc->prot.ni1.uus1_data[0] = '\0';

	return (proc);
} /* ni1_new_l3_process */

/************************************************/
/* free a l3 process and all ni1 specific data */
/************************************************/
static void
ni1_release_l3_process(struct l3_process *p)
{
	free_invoke_id(p->st, p->prot.ni1.invoke_id);
	release_l3_process(p);
} /* ni1_release_l3_process */

/********************************************************/
/* search a process with invoke id id and dummy callref */
/********************************************************/
static struct l3_process *
l3ni1_search_dummy_proc(struct PStack *st, int id)
{ struct l3_process *pc = st->l3.proc; /* start of processes */

	if (!id) return (NULL);

	while (pc)
	{ if ((pc->callref == -1) && (pc->prot.ni1.invoke_id == id))
			return (pc);
		pc = pc->next;
	}
	return (NULL);
} /* l3ni1_search_dummy_proc */

/*******************************************************************/
/* called when a facility message with a dummy callref is received */
/* and a return result is delivered. id specifies the invoke id.   */
/*******************************************************************/
static void
l3ni1_dummy_return_result(struct PStack *st, int id, u_char *p, u_char nlen)
{ isdn_ctrl ic;
	struct IsdnCardState *cs;
	struct l3_process *pc = NULL;

	if ((pc = l3ni1_search_dummy_proc(st, id)))
	{ L3DelTimer(&pc->timer); /* remove timer */

		cs = pc->st->l1.hardware;
		ic.driver = cs->myid;
		ic.command = ISDN_STAT_PROT;
		ic.arg = NI1_STAT_INVOKE_RES;
		ic.parm.ni1_io.hl_id = pc->prot.ni1.invoke_id;
		ic.parm.ni1_io.ll_id = pc->prot.ni1.ll_id;
		ic.parm.ni1_io.proc = pc->prot.ni1.proc;
		ic.parm.ni1_io.timeout = 0;
		ic.parm.ni1_io.datalen = nlen;
		ic.parm.ni1_io.data = p;
		free_invoke_id(pc->st, pc->prot.ni1.invoke_id);
		pc->prot.ni1.invoke_id = 0; /* reset id */

		cs->iif.statcallb(&ic);
		ni1_release_l3_process(pc);
	}
	else
		l3_debug(st, "dummy return result id=0x%x result len=%d", id, nlen);
} /* l3ni1_dummy_return_result */

/*******************************************************************/
/* called when a facility message with a dummy callref is received */
/* and a return error is delivered. id specifies the invoke id.    */
/*******************************************************************/
static void
l3ni1_dummy_error_return(struct PStack *st, int id, ulong error)
{ isdn_ctrl ic;
	struct IsdnCardState *cs;
	struct l3_process *pc = NULL;

	if ((pc = l3ni1_search_dummy_proc(st, id)))
	{ L3DelTimer(&pc->timer); /* remove timer */

		cs = pc->st->l1.hardware;
		ic.driver = cs->myid;
		ic.command = ISDN_STAT_PROT;
		ic.arg = NI1_STAT_INVOKE_ERR;
		ic.parm.ni1_io.hl_id = pc->prot.ni1.invoke_id;
		ic.parm.ni1_io.ll_id = pc->prot.ni1.ll_id;
		ic.parm.ni1_io.proc = pc->prot.ni1.proc;
		ic.parm.ni1_io.timeout = error;
		ic.parm.ni1_io.datalen = 0;
		ic.parm.ni1_io.data = NULL;
		free_invoke_id(pc->st, pc->prot.ni1.invoke_id);
		pc->prot.ni1.invoke_id = 0; /* reset id */

		cs->iif.statcallb(&ic);
		ni1_release_l3_process(pc);
	}
	else
		l3_debug(st, "dummy return error id=0x%x error=0x%lx", id, error);
} /* l3ni1_error_return */

/*******************************************************************/
/* called when a facility message with a dummy callref is received */
/* and a invoke is delivered. id specifies the invoke id.          */
/*******************************************************************/
static void
l3ni1_dummy_invoke(struct PStack *st, int cr, int id,
		   int ident, u_char *p, u_char nlen)
{ isdn_ctrl ic;
	struct IsdnCardState *cs;

	l3_debug(st, "dummy invoke %s id=0x%x ident=0x%x datalen=%d",
		 (cr == -1) ? "local" : "broadcast", id, ident, nlen);
	if (cr >= -1) return; /* ignore local data */

	cs = st->l1.hardware;
	ic.driver = cs->myid;
	ic.command = ISDN_STAT_PROT;
	ic.arg = NI1_STAT_INVOKE_BRD;
	ic.parm.ni1_io.hl_id = id;
	ic.parm.ni1_io.ll_id = 0;
	ic.parm.ni1_io.proc = ident;
	ic.parm.ni1_io.timeout = 0;
	ic.parm.ni1_io.datalen = nlen;
	ic.parm.ni1_io.data = p;

	cs->iif.statcallb(&ic);
} /* l3ni1_dummy_invoke */

static void
l3ni1_parse_facility(struct PStack *st, struct l3_process *pc,
		     int cr, u_char *p)
{
	int qd_len = 0;
	unsigned char nlen = 0, ilen, cp_tag;
	int ident, id;
	ulong err_ret;

	if (pc)
		st = pc->st; /* valid Stack */
	else
		if ((!st) || (cr >= 0)) return; /* neither pc nor st specified */

	p++;
	qd_len = *p++;
	if (qd_len == 0) {
		l3_debug(st, "qd_len == 0");
		return;
	}
	if ((*p & 0x1F) != 0x11) {	/* Service discriminator, supplementary service */
		l3_debug(st, "supplementary service != 0x11");
		return;
	}
	while (qd_len > 0 && !(*p & 0x80)) {	/* extension ? */
		p++;
		qd_len--;
	}
	if (qd_len < 2) {
		l3_debug(st, "qd_len < 2");
		return;
	}
	p++;
	qd_len--;
	if ((*p & 0xE0) != 0xA0) {	/* class and form */
		l3_debug(st, "class and form != 0xA0");
		return;
	}

	cp_tag = *p & 0x1F; /* remember tag value */

	p++;
	qd_len--;
	if (qd_len < 1)
	{ l3_debug(st, "qd_len < 1");
		return;
	}
	if (*p & 0x80)
	{ /* length format indefinite or limited */
		nlen = *p++ & 0x7F; /* number of len bytes or indefinite */
		if ((qd_len-- < ((!nlen) ? 3 : (1 + nlen))) ||
		    (nlen > 1))
		{ l3_debug(st, "length format error or not implemented");
			return;
		}
		if (nlen == 1)
		{ nlen = *p++; /* complete length */
			qd_len--;
		}
		else
		{ qd_len -= 2; /* trailing null bytes */
			if ((*(p + qd_len)) || (*(p + qd_len + 1)))
			{ l3_debug(st, "length format indefinite error");
				return;
			}
			nlen = qd_len;
		}
	}
	else
	{ nlen = *p++;
		qd_len--;
	}
	if (qd_len < nlen)
	{ l3_debug(st, "qd_len < nlen");
		return;
	}
	qd_len -= nlen;

	if (nlen < 2)
	{ l3_debug(st, "nlen < 2");
		return;
	}
	if (*p != 0x02)
	{  /* invoke identifier tag */
		l3_debug(st, "invoke identifier tag !=0x02");
		return;
	}
	p++;
	nlen--;
	if (*p & 0x80)
	{ /* length format */
		l3_debug(st, "invoke id length format 2");
		return;
	}
	ilen = *p++;
	nlen--;
	if (ilen > nlen || ilen == 0)
	{ l3_debug(st, "ilen > nlen || ilen == 0");
		return;
	}
	nlen -= ilen;
	id = 0;
	while (ilen > 0)
	{ id = (id << 8) | (*p++ & 0xFF);	/* invoke identifier */
		ilen--;
	}

	switch (cp_tag) {	/* component tag */
	case 1:	/* invoke */
		if (nlen < 2) {
			l3_debug(st, "nlen < 2 22");
			return;
		}
		if (*p != 0x02) {	/* operation value */
			l3_debug(st, "operation value !=0x02");
			return;
		}
		p++;
		nlen--;
		ilen = *p++;
		nlen--;
		if (ilen > nlen || ilen == 0) {
			l3_debug(st, "ilen > nlen || ilen == 0 22");
			return;
		}
		nlen -= ilen;
		ident = 0;
		while (ilen > 0) {
			ident = (ident << 8) | (*p++ & 0xFF);
			ilen--;
		}

		if (!pc)
		{
			l3ni1_dummy_invoke(st, cr, id, ident, p, nlen);
			return;
		}
		l3_debug(st, "invoke break");
		break;
	case 2:	/* return result */
		/* if no process available handle separately */
		if (!pc)
		{ if (cr == -1)
				l3ni1_dummy_return_result(st, id, p, nlen);
			return;
		}
		if ((pc->prot.ni1.invoke_id) && (pc->prot.ni1.invoke_id == id))
		{ /* Diversion successful */
			free_invoke_id(st, pc->prot.ni1.invoke_id);
			pc->prot.ni1.remote_result = 0; /* success */
			pc->prot.ni1.invoke_id = 0;
			pc->redir_result = pc->prot.ni1.remote_result;
			st->l3.l3l4(st, CC_REDIR | INDICATION, pc); } /* Diversion successful */
		else
			l3_debug(st, "return error unknown identifier");
		break;
	case 3:	/* return error */
		err_ret = 0;
		if (nlen < 2)
		{ l3_debug(st, "return error nlen < 2");
			return;
		}
		if (*p != 0x02)
		{ /* result tag */
			l3_debug(st, "invoke error tag !=0x02");
			return;
		}
		p++;
		nlen--;
		if (*p > 4)
		{ /* length format */
			l3_debug(st, "invoke return errlen > 4 ");
			return;
		}
		ilen = *p++;
		nlen--;
		if (ilen > nlen || ilen == 0)
		{ l3_debug(st, "error return ilen > nlen || ilen == 0");
			return;
		}
		nlen -= ilen;
		while (ilen > 0)
		{ err_ret = (err_ret << 8) | (*p++ & 0xFF);	/* error value */
			ilen--;
		}
		/* if no process available handle separately */
		if (!pc)
		{ if (cr == -1)
				l3ni1_dummy_error_return(st, id, err_ret);
			return;
		}
		if ((pc->prot.ni1.invoke_id) && (pc->prot.ni1.invoke_id == id))
		{ /* Deflection error */
			free_invoke_id(st, pc->prot.ni1.invoke_id);
			pc->prot.ni1.remote_result = err_ret; /* result */
			pc->prot.ni1.invoke_id = 0;
			pc->redir_result = pc->prot.ni1.remote_result;
			st->l3.l3l4(st, CC_REDIR | INDICATION, pc);
		} /* Deflection error */
		else
			l3_debug(st, "return result unknown identifier");
		break;
	default:
		l3_debug(st, "facility default break tag=0x%02x", cp_tag);
		break;
	}
}

static void
l3ni1_message(struct l3_process *pc, u_char mt)
{
	struct sk_buff *skb;
	u_char *p;

	if (!(skb = l3_alloc_skb(4)))
		return;
	p = skb_put(skb, 4);
	MsgHead(p, pc->callref, mt);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3ni1_message_plus_chid(struct l3_process *pc, u_char mt)
/* sends an l3 messages plus channel id -  added GE 05/09/00 */
{
	struct sk_buff *skb;
	u_char tmp[16];
	u_char *p = tmp;
	u_char chid;

	chid = (u_char)(pc->para.bchannel & 0x03) | 0x88;
	MsgHead(p, pc->callref, mt);
	*p++ = IE_CHANNEL_ID;
	*p++ = 0x01;
	*p++ = chid;

	if (!(skb = l3_alloc_skb(7)))
		return;
	memcpy(skb_put(skb, 7), tmp, 7);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3ni1_message_cause(struct l3_process *pc, u_char mt, u_char cause)
{
	struct sk_buff *skb;
	u_char tmp[16];
	u_char *p = tmp;
	int l;

	MsgHead(p, pc->callref, mt);
	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = cause | 0x80;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3ni1_status_send(struct l3_process *pc, u_char pr, void *arg)
{
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	struct sk_buff *skb;

	MsgHead(p, pc->callref, MT_STATUS);

	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = pc->para.cause | 0x80;

	*p++ = IE_CALL_STATE;
	*p++ = 0x1;
	*p++ = pc->state & 0x3f;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3ni1_msg_without_setup(struct l3_process *pc, u_char pr, void *arg)
{
	/* This routine is called if here was no SETUP made (checks in ni1up and in
	 * l3ni1_setup) and a RELEASE_COMPLETE have to be sent with an error code
	 * MT_STATUS_ENQUIRE in the NULL state is handled too
	 */
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	struct sk_buff *skb;

	switch (pc->para.cause) {
	case 81:	/* invalid callreference */
	case 88:	/* incomp destination */
	case 96:	/* mandory IE missing */
	case 100:       /* invalid IE contents */
	case 101:	/* incompatible Callstate */
		MsgHead(p, pc->callref, MT_RELEASE_COMPLETE);
		*p++ = IE_CAUSE;
		*p++ = 0x2;
		*p++ = 0x80;
		*p++ = pc->para.cause | 0x80;
		break;
	default:
		printk(KERN_ERR "HiSax l3ni1_msg_without_setup wrong cause %d\n",
		       pc->para.cause);
		return;
	}
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	ni1_release_l3_process(pc);
}

static int ie_ALERTING[] = {IE_BEARER, IE_CHANNEL_ID | IE_MANDATORY_1,
			    IE_FACILITY, IE_PROGRESS, IE_DISPLAY, IE_SIGNAL, IE_HLC,
			    IE_USER_USER, -1};
static int ie_CALL_PROCEEDING[] = {IE_BEARER, IE_CHANNEL_ID | IE_MANDATORY_1,
				   IE_FACILITY, IE_PROGRESS, IE_DISPLAY, IE_HLC, -1};
static int ie_CONNECT[] = {IE_BEARER, IE_CHANNEL_ID | IE_MANDATORY_1,
			   IE_FACILITY, IE_PROGRESS, IE_DISPLAY, IE_DATE, IE_SIGNAL,
			   IE_CONNECT_PN, IE_CONNECT_SUB, IE_LLC, IE_HLC, IE_USER_USER, -1};
static int ie_CONNECT_ACKNOWLEDGE[] = {IE_CHANNEL_ID, IE_DISPLAY, IE_SIGNAL, -1};
static int ie_DISCONNECT[] = {IE_CAUSE | IE_MANDATORY, IE_FACILITY,
			      IE_PROGRESS, IE_DISPLAY, IE_SIGNAL, IE_USER_USER, -1};
static int ie_INFORMATION[] = {IE_COMPLETE, IE_DISPLAY, IE_KEYPAD, IE_SIGNAL,
			       IE_CALLED_PN, -1};
static int ie_NOTIFY[] = {IE_BEARER, IE_NOTIFY | IE_MANDATORY, IE_DISPLAY, -1};
static int ie_PROGRESS[] = {IE_BEARER, IE_CAUSE, IE_FACILITY, IE_PROGRESS |
			    IE_MANDATORY, IE_DISPLAY, IE_HLC, IE_USER_USER, -1};
static int ie_RELEASE[] = {IE_CAUSE | IE_MANDATORY_1, IE_FACILITY, IE_DISPLAY,
			   IE_SIGNAL, IE_USER_USER, -1};
/* a RELEASE_COMPLETE with errors don't require special actions
   static int ie_RELEASE_COMPLETE[] = {IE_CAUSE | IE_MANDATORY_1, IE_DISPLAY, IE_SIGNAL, IE_USER_USER, -1};
*/
static int ie_RESUME_ACKNOWLEDGE[] = {IE_CHANNEL_ID | IE_MANDATORY, IE_FACILITY,
				      IE_DISPLAY, -1};
static int ie_RESUME_REJECT[] = {IE_CAUSE | IE_MANDATORY, IE_DISPLAY, -1};
static int ie_SETUP[] = {IE_COMPLETE, IE_BEARER  | IE_MANDATORY,
			 IE_CHANNEL_ID | IE_MANDATORY, IE_FACILITY, IE_PROGRESS,
			 IE_NET_FAC, IE_DISPLAY, IE_KEYPAD, IE_SIGNAL, IE_CALLING_PN,
			 IE_CALLING_SUB, IE_CALLED_PN, IE_CALLED_SUB, IE_REDIR_NR,
			 IE_LLC, IE_HLC, IE_USER_USER, -1};
static int ie_SETUP_ACKNOWLEDGE[] = {IE_CHANNEL_ID | IE_MANDATORY, IE_FACILITY,
				     IE_PROGRESS, IE_DISPLAY, IE_SIGNAL, -1};
static int ie_STATUS[] = {IE_CAUSE | IE_MANDATORY, IE_CALL_STATE |
			  IE_MANDATORY, IE_DISPLAY, -1};
static int ie_STATUS_ENQUIRY[] = {IE_DISPLAY, -1};
static int ie_SUSPEND_ACKNOWLEDGE[] = {IE_DISPLAY, IE_FACILITY, -1};
static int ie_SUSPEND_REJECT[] = {IE_CAUSE | IE_MANDATORY, IE_DISPLAY, -1};
/* not used
 * static int ie_CONGESTION_CONTROL[] = {IE_CONGESTION | IE_MANDATORY,
 *		IE_CAUSE | IE_MANDATORY, IE_DISPLAY, -1};
 * static int ie_USER_INFORMATION[] = {IE_MORE_DATA, IE_USER_USER | IE_MANDATORY, -1};
 * static int ie_RESTART[] = {IE_CHANNEL_ID, IE_DISPLAY, IE_RESTART_IND |
 *		IE_MANDATORY, -1};
 */
static int ie_FACILITY[] = {IE_FACILITY | IE_MANDATORY, IE_DISPLAY, -1};
static int comp_required[] = {1, 2, 3, 5, 6, 7, 9, 10, 11, 14, 15, -1};
static int l3_valid_states[] = {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 15, 17, 19, 25, -1};

struct ie_len {
	int ie;
	int len;
};

static
struct ie_len max_ie_len[] = {
	{IE_SEGMENT, 4},
	{IE_BEARER, 12},
	{IE_CAUSE, 32},
	{IE_CALL_ID, 10},
	{IE_CALL_STATE, 3},
	{IE_CHANNEL_ID,	34},
	{IE_FACILITY, 255},
	{IE_PROGRESS, 4},
	{IE_NET_FAC, 255},
	{IE_NOTIFY, 3},
	{IE_DISPLAY, 82},
	{IE_DATE, 8},
	{IE_KEYPAD, 34},
	{IE_SIGNAL, 3},
	{IE_INFORATE, 6},
	{IE_E2E_TDELAY, 11},
	{IE_TDELAY_SEL, 5},
	{IE_PACK_BINPARA, 3},
	{IE_PACK_WINSIZE, 4},
	{IE_PACK_SIZE, 4},
	{IE_CUG, 7},
	{IE_REV_CHARGE, 3},
	{IE_CALLING_PN, 24},
	{IE_CALLING_SUB, 23},
	{IE_CALLED_PN, 24},
	{IE_CALLED_SUB, 23},
	{IE_REDIR_NR, 255},
	{IE_TRANS_SEL, 255},
	{IE_RESTART_IND, 3},
	{IE_LLC, 18},
	{IE_HLC, 5},
	{IE_USER_USER, 131},
	{-1, 0},
};

static int
getmax_ie_len(u_char ie) {
	int i = 0;
	while (max_ie_len[i].ie != -1) {
		if (max_ie_len[i].ie == ie)
			return (max_ie_len[i].len);
		i++;
	}
	return (255);
}

static int
ie_in_set(struct l3_process *pc, u_char ie, int *checklist) {
	int ret = 1;

	while (*checklist != -1) {
		if ((*checklist & 0xff) == ie) {
			if (ie & 0x80)
				return (-ret);
			else
				return (ret);
		}
		ret++;
		checklist++;
	}
	return (0);
}

static int
check_infoelements(struct l3_process *pc, struct sk_buff *skb, int *checklist)
{
	int *cl = checklist;
	u_char mt;
	u_char *p, ie;
	int l, newpos, oldpos;
	int err_seq = 0, err_len = 0, err_compr = 0, err_ureg = 0;
	u_char codeset = 0;
	u_char old_codeset = 0;
	u_char codelock = 1;

	p = skb->data;
	/* skip cr */
	p++;
	l = (*p++) & 0xf;
	p += l;
	mt = *p++;
	oldpos = 0;
	while ((p - skb->data) < skb->len) {
		if ((*p & 0xf0) == 0x90) { /* shift codeset */
			old_codeset = codeset;
			codeset = *p & 7;
			if (*p & 0x08)
				codelock = 0;
			else
				codelock = 1;
			if (pc->debug & L3_DEB_CHECK)
				l3_debug(pc->st, "check IE shift%scodeset %d->%d",
					 codelock ? " locking " : " ", old_codeset, codeset);
			p++;
			continue;
		}
		if (!codeset) { /* only codeset 0 */
			if ((newpos = ie_in_set(pc, *p, cl))) {
				if (newpos > 0) {
					if (newpos < oldpos)
						err_seq++;
					else
						oldpos = newpos;
				}
			} else {
				if (ie_in_set(pc, *p, comp_required))
					err_compr++;
				else
					err_ureg++;
			}
		}
		ie = *p++;
		if (ie & 0x80) {
			l = 1;
		} else {
			l = *p++;
			p += l;
			l += 2;
		}
		if (!codeset && (l > getmax_ie_len(ie)))
			err_len++;
		if (!codelock) {
			if (pc->debug & L3_DEB_CHECK)
				l3_debug(pc->st, "check IE shift back codeset %d->%d",
					 codeset, old_codeset);
			codeset = old_codeset;
			codelock = 1;
		}
	}
	if (err_compr | err_ureg | err_len | err_seq) {
		if (pc->debug & L3_DEB_CHECK)
			l3_debug(pc->st, "check IE MT(%x) %d/%d/%d/%d",
				 mt, err_compr, err_ureg, err_len, err_seq);
		if (err_compr)
			return (ERR_IE_COMPREHENSION);
		if (err_ureg)
			return (ERR_IE_UNRECOGNIZED);
		if (err_len)
			return (ERR_IE_LENGTH);
		if (err_seq)
			return (ERR_IE_SEQUENCE);
	}
	return (0);
}

/* verify if a message type exists and contain no IE error */
static int
l3ni1_check_messagetype_validity(struct l3_process *pc, int mt, void *arg)
{
	switch (mt) {
	case MT_ALERTING:
	case MT_CALL_PROCEEDING:
	case MT_CONNECT:
	case MT_CONNECT_ACKNOWLEDGE:
	case MT_DISCONNECT:
	case MT_INFORMATION:
	case MT_FACILITY:
	case MT_NOTIFY:
	case MT_PROGRESS:
	case MT_RELEASE:
	case MT_RELEASE_COMPLETE:
	case MT_SETUP:
	case MT_SETUP_ACKNOWLEDGE:
	case MT_RESUME_ACKNOWLEDGE:
	case MT_RESUME_REJECT:
	case MT_SUSPEND_ACKNOWLEDGE:
	case MT_SUSPEND_REJECT:
	case MT_USER_INFORMATION:
	case MT_RESTART:
	case MT_RESTART_ACKNOWLEDGE:
	case MT_CONGESTION_CONTROL:
	case MT_STATUS:
	case MT_STATUS_ENQUIRY:
		if (pc->debug & L3_DEB_CHECK)
			l3_debug(pc->st, "l3ni1_check_messagetype_validity mt(%x) OK", mt);
		break;
	case MT_RESUME: /* RESUME only in user->net */
	case MT_SUSPEND: /* SUSPEND only in user->net */
	default:
		if (pc->debug & (L3_DEB_CHECK | L3_DEB_WARN))
			l3_debug(pc->st, "l3ni1_check_messagetype_validity mt(%x) fail", mt);
		pc->para.cause = 97;
		l3ni1_status_send(pc, 0, NULL);
		return (1);
	}
	return (0);
}

static void
l3ni1_std_ie_err(struct l3_process *pc, int ret) {

	if (pc->debug & L3_DEB_CHECK)
		l3_debug(pc->st, "check_infoelements ret %d", ret);
	switch (ret) {
	case 0:
		break;
	case ERR_IE_COMPREHENSION:
		pc->para.cause = 96;
		l3ni1_status_send(pc, 0, NULL);
		break;
	case ERR_IE_UNRECOGNIZED:
		pc->para.cause = 99;
		l3ni1_status_send(pc, 0, NULL);
		break;
	case ERR_IE_LENGTH:
		pc->para.cause = 100;
		l3ni1_status_send(pc, 0, NULL);
		break;
	case ERR_IE_SEQUENCE:
	default:
		break;
	}
}

static int
l3ni1_get_channel_id(struct l3_process *pc, struct sk_buff *skb) {
	u_char *p;

	p = skb->data;
	if ((p = findie(p, skb->len, IE_CHANNEL_ID, 0))) {
		p++;
		if (*p != 1) { /* len for BRI = 1 */
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "wrong chid len %d", *p);
			return (-2);
		}
		p++;
		if (*p & 0x60) { /* only base rate interface */
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "wrong chid %x", *p);
			return (-3);
		}
		return (*p & 0x3);
	} else
		return (-1);
}

static int
l3ni1_get_cause(struct l3_process *pc, struct sk_buff *skb) {
	u_char l, i = 0;
	u_char *p;

	p = skb->data;
	pc->para.cause = 31;
	pc->para.loc = 0;
	if ((p = findie(p, skb->len, IE_CAUSE, 0))) {
		p++;
		l = *p++;
		if (l > 30)
			return (1);
		if (l) {
			pc->para.loc = *p++;
			l--;
		} else {
			return (2);
		}
		if (l && !(pc->para.loc & 0x80)) {
			l--;
			p++; /* skip recommendation */
		}
		if (l) {
			pc->para.cause = *p++;
			l--;
			if (!(pc->para.cause & 0x80))
				return (3);
		} else
			return (4);
		while (l && (i < 6)) {
			pc->para.diag[i++] = *p++;
			l--;
		}
	} else
		return (-1);
	return (0);
}

static void
l3ni1_msg_with_uus(struct l3_process *pc, u_char cmd)
{
	struct sk_buff *skb;
	u_char tmp[16 + 40];
	u_char *p = tmp;
	int l;

	MsgHead(p, pc->callref, cmd);

	if (pc->prot.ni1.uus1_data[0])
	{ *p++ = IE_USER_USER; /* UUS info element */
		*p++ = strlen(pc->prot.ni1.uus1_data) + 1;
		*p++ = 0x04; /* IA5 chars */
		strcpy(p, pc->prot.ni1.uus1_data);
		p += strlen(pc->prot.ni1.uus1_data);
		pc->prot.ni1.uus1_data[0] = '\0';
	}

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
} /* l3ni1_msg_with_uus */

static void
l3ni1_release_req(struct l3_process *pc, u_char pr, void *arg)
{
	StopAllL3Timer(pc);
	newl3state(pc, 19);
	if (!pc->prot.ni1.uus1_data[0])
		l3ni1_message(pc, MT_RELEASE);
	else
		l3ni1_msg_with_uus(pc, MT_RELEASE);
	L3AddTimer(&pc->timer, T308, CC_T308_1);
}

static void
l3ni1_release_cmpl(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	if ((ret = l3ni1_get_cause(pc, skb)) > 0) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "RELCMPL get_cause ret(%d)", ret);
	} else if (ret < 0)
		pc->para.cause = NO_CAUSE;
	StopAllL3Timer(pc);
	newl3state(pc, 0);
	pc->st->l3.l3l4(pc->st, CC_RELEASE | CONFIRM, pc);
	ni1_release_l3_process(pc);
}

#if EXT_BEARER_CAPS

static u_char *
EncodeASyncParams(u_char *p, u_char si2)
{				// 7c 06 88  90 21 42 00 bb

	p[0] = 0;
	p[1] = 0x40;		// Intermediate rate: 16 kbit/s jj 2000.02.19
	p[2] = 0x80;
	if (si2 & 32)		// 7 data bits

		p[2] += 16;
	else			// 8 data bits

		p[2] += 24;

	if (si2 & 16)		// 2 stop bits

		p[2] += 96;
	else			// 1 stop bit

		p[2] += 32;

	if (si2 & 8)		// even parity

		p[2] += 2;
	else			// no parity

		p[2] += 3;

	switch (si2 & 0x07) {
	case 0:
		p[0] = 66;	// 1200 bit/s

		break;
	case 1:
		p[0] = 88;	// 1200/75 bit/s

		break;
	case 2:
		p[0] = 87;	// 75/1200 bit/s

		break;
	case 3:
		p[0] = 67;	// 2400 bit/s

		break;
	case 4:
		p[0] = 69;	// 4800 bit/s

		break;
	case 5:
		p[0] = 72;	// 9600 bit/s

		break;
	case 6:
		p[0] = 73;	// 14400 bit/s

		break;
	case 7:
		p[0] = 75;	// 19200 bit/s

		break;
	}
	return p + 3;
}

static u_char
EncodeSyncParams(u_char si2, u_char ai)
{

	switch (si2) {
	case 0:
		return ai + 2;	// 1200 bit/s

	case 1:
		return ai + 24;		// 1200/75 bit/s

	case 2:
		return ai + 23;		// 75/1200 bit/s

	case 3:
		return ai + 3;	// 2400 bit/s

	case 4:
		return ai + 5;	// 4800 bit/s

	case 5:
		return ai + 8;	// 9600 bit/s

	case 6:
		return ai + 9;	// 14400 bit/s

	case 7:
		return ai + 11;		// 19200 bit/s

	case 8:
		return ai + 14;		// 48000 bit/s

	case 9:
		return ai + 15;		// 56000 bit/s

	case 15:
		return ai + 40;		// negotiate bit/s

	default:
		break;
	}
	return ai;
}


static u_char
DecodeASyncParams(u_char si2, u_char *p)
{
	u_char info;

	switch (p[5]) {
	case 66:	// 1200 bit/s

		break;	// si2 don't change

	case 88:	// 1200/75 bit/s

		si2 += 1;
		break;
	case 87:	// 75/1200 bit/s

		si2 += 2;
		break;
	case 67:	// 2400 bit/s

		si2 += 3;
		break;
	case 69:	// 4800 bit/s

		si2 += 4;
		break;
	case 72:	// 9600 bit/s

		si2 += 5;
		break;
	case 73:	// 14400 bit/s

		si2 += 6;
		break;
	case 75:	// 19200 bit/s

		si2 += 7;
		break;
	}

	info = p[7] & 0x7f;
	if ((info & 16) && (!(info & 8)))	// 7 data bits

		si2 += 32;	// else 8 data bits

	if ((info & 96) == 96)	// 2 stop bits

		si2 += 16;	// else 1 stop bit

	if ((info & 2) && (!(info & 1)))	// even parity

		si2 += 8;	// else no parity

	return si2;
}


static u_char
DecodeSyncParams(u_char si2, u_char info)
{
	info &= 0x7f;
	switch (info) {
	case 40:	// bit/s negotiation failed  ai := 165 not 175!

		return si2 + 15;
	case 15:	// 56000 bit/s failed, ai := 0 not 169 !

		return si2 + 9;
	case 14:	// 48000 bit/s

		return si2 + 8;
	case 11:	// 19200 bit/s

		return si2 + 7;
	case 9:	// 14400 bit/s

		return si2 + 6;
	case 8:	// 9600  bit/s

		return si2 + 5;
	case 5:	// 4800  bit/s

		return si2 + 4;
	case 3:	// 2400  bit/s

		return si2 + 3;
	case 23:	// 75/1200 bit/s

		return si2 + 2;
	case 24:	// 1200/75 bit/s

		return si2 + 1;
	default:	// 1200 bit/s

		return si2;
	}
}

static u_char
DecodeSI2(struct sk_buff *skb)
{
	u_char *p;		//, *pend=skb->data + skb->len;

	if ((p = findie(skb->data, skb->len, 0x7c, 0))) {
		switch (p[4] & 0x0f) {
		case 0x01:
			if (p[1] == 0x04)	// sync. Bitratenadaption

				return DecodeSyncParams(160, p[5]);	// V.110/X.30

			else if (p[1] == 0x06)	// async. Bitratenadaption

				return DecodeASyncParams(192, p);	// V.110/X.30

			break;
		case 0x08:	// if (p[5] == 0x02) // sync. Bitratenadaption
			if (p[1] > 3)
				return DecodeSyncParams(176, p[5]);	// V.120
			break;
		}
	}
	return 0;
}

#endif


static void
l3ni1_setup_req(struct l3_process *pc, u_char pr,
		void *arg)
{
	struct sk_buff *skb;
	u_char tmp[128];
	u_char *p = tmp;

	u_char *teln;
	u_char *sub;
	u_char *sp;
	int l;

	MsgHead(p, pc->callref, MT_SETUP);

	teln = pc->para.setup.phone;

	*p++ = 0xa1;		/* complete indicator */
	/*
	 * Set Bearer Capability, Map info from 1TR6-convention to NI1
	 */
	switch (pc->para.setup.si1) {
	case 1:	                  /* Telephony                                */
		*p++ = IE_BEARER;
		*p++ = 0x3;	  /* Length                                   */
		*p++ = 0x90;	  /* 3.1khz Audio			      */
		*p++ = 0x90;	  /* Circuit-Mode 64kbps                      */
		*p++ = 0xa2;	  /* u-Law Audio                              */
		break;
	case 5:	                  /* Datatransmission 64k, BTX                */
	case 7:	                  /* Datatransmission 64k                     */
	default:
		*p++ = IE_BEARER;
		*p++ = 0x2;	  /* Length                                   */
		*p++ = 0x88;	  /* Coding Std. CCITT, unrestr. dig. Inform. */
		*p++ = 0x90;	  /* Circuit-Mode 64kbps                      */
		break;
	}

	sub = NULL;
	sp = teln;
	while (*sp) {
		if ('.' == *sp) {
			sub = sp;
			*sp = 0;
		} else
			sp++;
	}

	*p++ = IE_KEYPAD;
	*p++ = strlen(teln);
	while (*teln)
		*p++ = (*teln++) & 0x7F;

	if (sub)
		*sub++ = '.';

#if EXT_BEARER_CAPS
	if ((pc->para.setup.si2 >= 160) && (pc->para.setup.si2 <= 175)) {	// sync. Bitratenadaption, V.110/X.30

		*p++ = IE_LLC;
		*p++ = 0x04;
		*p++ = 0x88;
		*p++ = 0x90;
		*p++ = 0x21;
		*p++ = EncodeSyncParams(pc->para.setup.si2 - 160, 0x80);
	} else if ((pc->para.setup.si2 >= 176) && (pc->para.setup.si2 <= 191)) {	// sync. Bitratenadaption, V.120

		*p++ = IE_LLC;
		*p++ = 0x05;
		*p++ = 0x88;
		*p++ = 0x90;
		*p++ = 0x28;
		*p++ = EncodeSyncParams(pc->para.setup.si2 - 176, 0);
		*p++ = 0x82;
	} else if (pc->para.setup.si2 >= 192) {		// async. Bitratenadaption, V.110/X.30

		*p++ = IE_LLC;
		*p++ = 0x06;
		*p++ = 0x88;
		*p++ = 0x90;
		*p++ = 0x21;
		p = EncodeASyncParams(p, pc->para.setup.si2 - 192);
	} else {
		switch (pc->para.setup.si1) {
		case 1:	                /* Telephony                                */
			*p++ = IE_LLC;
			*p++ = 0x3;	/* Length                                   */
			*p++ = 0x90;	/* Coding Std. CCITT, 3.1 kHz audio         */
			*p++ = 0x90;	/* Circuit-Mode 64kbps                      */
			*p++ = 0xa2;	/* u-Law Audio                              */
			break;
		case 5:	                /* Datatransmission 64k, BTX                */
		case 7:	                /* Datatransmission 64k                     */
		default:
			*p++ = IE_LLC;
			*p++ = 0x2;	/* Length                                   */
			*p++ = 0x88;	/* Coding Std. CCITT, unrestr. dig. Inform. */
			*p++ = 0x90;	/* Circuit-Mode 64kbps                      */
			break;
		}
	}
#endif
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
	{
		return;
	}
	memcpy(skb_put(skb, l), tmp, l);
	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, T303, CC_T303);
	newl3state(pc, 1);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3ni1_call_proc(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int id, ret;

	if ((id = l3ni1_get_channel_id(pc, skb)) >= 0) {
		if ((0 == id) || ((3 == id) && (0x10 == pc->para.moderate))) {
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "setup answer with wrong chid %x", id);
			pc->para.cause = 100;
			l3ni1_status_send(pc, pr, NULL);
			return;
		}
		pc->para.bchannel = id;
	} else if (1 == pc->state) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "setup answer wrong chid (ret %d)", id);
		if (id == -1)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3ni1_status_send(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	ret = check_infoelements(pc, skb, ie_CALL_PROCEEDING);
	if (ERR_IE_COMPREHENSION == ret) {
		l3ni1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	newl3state(pc, 3);
	L3AddTimer(&pc->timer, T310, CC_T310);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3ni1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_PROCEEDING | INDICATION, pc);
}

static void
l3ni1_setup_ack(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int id, ret;

	if ((id = l3ni1_get_channel_id(pc, skb)) >= 0) {
		if ((0 == id) || ((3 == id) && (0x10 == pc->para.moderate))) {
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "setup answer with wrong chid %x", id);
			pc->para.cause = 100;
			l3ni1_status_send(pc, pr, NULL);
			return;
		}
		pc->para.bchannel = id;
	} else {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "setup answer wrong chid (ret %d)", id);
		if (id == -1)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3ni1_status_send(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	ret = check_infoelements(pc, skb, ie_SETUP_ACKNOWLEDGE);
	if (ERR_IE_COMPREHENSION == ret) {
		l3ni1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	newl3state(pc, 2);
	L3AddTimer(&pc->timer, T304, CC_T304);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3ni1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_MORE_INFO | INDICATION, pc);
}

static void
l3ni1_disconnect(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	u_char *p;
	int ret;
	u_char cause = 0;

	StopAllL3Timer(pc);
	if ((ret = l3ni1_get_cause(pc, skb))) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "DISC get_cause ret(%d)", ret);
		if (ret < 0)
			cause = 96;
		else if (ret > 0)
			cause = 100;
	}
	if ((p = findie(skb->data, skb->len, IE_FACILITY, 0)))
		l3ni1_parse_facility(pc->st, pc, pc->callref, p);
	ret = check_infoelements(pc, skb, ie_DISCONNECT);
	if (ERR_IE_COMPREHENSION == ret)
		cause = 96;
	else if ((!cause) && (ERR_IE_UNRECOGNIZED == ret))
		cause = 99;
	ret = pc->state;
	newl3state(pc, 12);
	if (cause)
		newl3state(pc, 19);
	if (11 != ret)
		pc->st->l3.l3l4(pc->st, CC_DISCONNECT | INDICATION, pc);
	else if (!cause)
		l3ni1_release_req(pc, pr, NULL);
	if (cause) {
		l3ni1_message_cause(pc, MT_RELEASE, cause);
		L3AddTimer(&pc->timer, T308, CC_T308_1);
	}
}

static void
l3ni1_connect(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	ret = check_infoelements(pc, skb, ie_CONNECT);
	if (ERR_IE_COMPREHENSION == ret) {
		l3ni1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);	/* T310 */
	newl3state(pc, 10);
	pc->para.chargeinfo = 0;
	/* here should inserted COLP handling KKe */
	if (ret)
		l3ni1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_SETUP | CONFIRM, pc);
}

static void
l3ni1_alerting(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	ret = check_infoelements(pc, skb, ie_ALERTING);
	if (ERR_IE_COMPREHENSION == ret) {
		l3ni1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);	/* T304 */
	newl3state(pc, 4);
	if (ret)
		l3ni1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_ALERTING | INDICATION, pc);
}

static void
l3ni1_setup(struct l3_process *pc, u_char pr, void *arg)
{
	u_char *p;
	int bcfound = 0;
	char tmp[80];
	struct sk_buff *skb = arg;
	int id;
	int err = 0;

	/*
	 * Bearer Capabilities
	 */
	p = skb->data;
	/* only the first occurrence 'll be detected ! */
	if ((p = findie(p, skb->len, 0x04, 0))) {
		if ((p[1] < 2) || (p[1] > 11))
			err = 1;
		else {
			pc->para.setup.si2 = 0;
			switch (p[2] & 0x7f) {
			case 0x00: /* Speech */
			case 0x10: /* 3.1 Khz audio */
				pc->para.setup.si1 = 1;
				break;
			case 0x08: /* Unrestricted digital information */
				pc->para.setup.si1 = 7;
/* JIM, 05.11.97 I wanna set service indicator 2 */
#if EXT_BEARER_CAPS
				pc->para.setup.si2 = DecodeSI2(skb);
#endif
				break;
			case 0x09: /* Restricted digital information */
				pc->para.setup.si1 = 2;
				break;
			case 0x11:
				/* Unrestr. digital information  with
				 * tones/announcements ( or 7 kHz audio
				 */
				pc->para.setup.si1 = 3;
				break;
			case 0x18: /* Video */
				pc->para.setup.si1 = 4;
				break;
			default:
				err = 2;
				break;
			}
			switch (p[3] & 0x7f) {
			case 0x40: /* packed mode */
				pc->para.setup.si1 = 8;
				break;
			case 0x10: /* 64 kbit */
			case 0x11: /* 2*64 kbit */
			case 0x13: /* 384 kbit */
			case 0x15: /* 1536 kbit */
			case 0x17: /* 1920 kbit */
				pc->para.moderate = p[3] & 0x7f;
				break;
			default:
				err = 3;
				break;
			}
		}
		if (pc->debug & L3_DEB_SI)
			l3_debug(pc->st, "SI=%d, AI=%d",
				 pc->para.setup.si1, pc->para.setup.si2);
		if (err) {
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "setup with wrong bearer(l=%d:%x,%x)",
					 p[1], p[2], p[3]);
			pc->para.cause = 100;
			l3ni1_msg_without_setup(pc, pr, NULL);
			return;
		}
	} else {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "setup without bearer capabilities");
		/* ETS 300-104 1.3.3 */
		pc->para.cause = 96;
		l3ni1_msg_without_setup(pc, pr, NULL);
		return;
	}
	/*
	 * Channel Identification
	 */
	if ((id = l3ni1_get_channel_id(pc, skb)) >= 0) {
		if ((pc->para.bchannel = id)) {
			if ((3 == id) && (0x10 == pc->para.moderate)) {
				if (pc->debug & L3_DEB_WARN)
					l3_debug(pc->st, "setup with wrong chid %x",
						 id);
				pc->para.cause = 100;
				l3ni1_msg_without_setup(pc, pr, NULL);
				return;
			}
			bcfound++;
		} else
		{ if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "setup without bchannel, call waiting");
			bcfound++;
		}
	} else {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "setup with wrong chid ret %d", id);
		if (id == -1)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3ni1_msg_without_setup(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	err = check_infoelements(pc, skb, ie_SETUP);
	if (ERR_IE_COMPREHENSION == err) {
		pc->para.cause = 96;
		l3ni1_msg_without_setup(pc, pr, NULL);
		return;
	}
	p = skb->data;
	if ((p = findie(p, skb->len, 0x70, 0)))
		iecpy(pc->para.setup.eazmsn, p, 1);
	else
		pc->para.setup.eazmsn[0] = 0;

	p = skb->data;
	if ((p = findie(p, skb->len, 0x71, 0))) {
		/* Called party subaddress */
		if ((p[1] >= 2) && (p[2] == 0x80) && (p[3] == 0x50)) {
			tmp[0] = '.';
			iecpy(&tmp[1], p, 2);
			strcat(pc->para.setup.eazmsn, tmp);
		} else if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "wrong called subaddress");
	}
	p = skb->data;
	if ((p = findie(p, skb->len, 0x6c, 0))) {
		pc->para.setup.plan = p[2];
		if (p[2] & 0x80) {
			iecpy(pc->para.setup.phone, p, 1);
			pc->para.setup.screen = 0;
		} else {
			iecpy(pc->para.setup.phone, p, 2);
			pc->para.setup.screen = p[3];
		}
	} else {
		pc->para.setup.phone[0] = 0;
		pc->para.setup.plan = 0;
		pc->para.setup.screen = 0;
	}
	p = skb->data;
	if ((p = findie(p, skb->len, 0x6d, 0))) {
		/* Calling party subaddress */
		if ((p[1] >= 2) && (p[2] == 0x80) && (p[3] == 0x50)) {
			tmp[0] = '.';
			iecpy(&tmp[1], p, 2);
			strcat(pc->para.setup.phone, tmp);
		} else if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "wrong calling subaddress");
	}
	newl3state(pc, 6);
	if (err) /* STATUS for none mandatory IE errors after actions are taken */
		l3ni1_std_ie_err(pc, err);
	pc->st->l3.l3l4(pc->st, CC_SETUP | INDICATION, pc);
}

static void
l3ni1_reset(struct l3_process *pc, u_char pr, void *arg)
{
	ni1_release_l3_process(pc);
}

static void
l3ni1_disconnect_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[16 + 40];
	u_char *p = tmp;
	int l;
	u_char cause = 16;

	if (pc->para.cause != NO_CAUSE)
		cause = pc->para.cause;

	StopAllL3Timer(pc);

	MsgHead(p, pc->callref, MT_DISCONNECT);

	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = cause | 0x80;

	if (pc->prot.ni1.uus1_data[0])
	{ *p++ = IE_USER_USER; /* UUS info element */
		*p++ = strlen(pc->prot.ni1.uus1_data) + 1;
		*p++ = 0x04; /* IA5 chars */
		strcpy(p, pc->prot.ni1.uus1_data);
		p += strlen(pc->prot.ni1.uus1_data);
		pc->prot.ni1.uus1_data[0] = '\0';
	}

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	newl3state(pc, 11);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	L3AddTimer(&pc->timer, T305, CC_T305);
}

static void
l3ni1_setup_rsp(struct l3_process *pc, u_char pr,
		void *arg)
{
	if (!pc->para.bchannel)
	{ if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "D-chan connect for waiting call");
		l3ni1_disconnect_req(pc, pr, arg);
		return;
	}
	newl3state(pc, 8);
	if (pc->debug & L3_DEB_WARN)
		l3_debug(pc->st, "D-chan connect for waiting call");
	l3ni1_message_plus_chid(pc, MT_CONNECT); /* GE 05/09/00 */
	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, T313, CC_T313);
}

static void
l3ni1_connect_ack(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	ret = check_infoelements(pc, skb, ie_CONNECT_ACKNOWLEDGE);
	if (ERR_IE_COMPREHENSION == ret) {
		l3ni1_std_ie_err(pc, ret);
		return;
	}
	newl3state(pc, 10);
	L3DelTimer(&pc->timer);
	if (ret)
		l3ni1_std_ie_err(pc, ret);
	pc->st->l3.l3l4(pc->st, CC_SETUP_COMPL | INDICATION, pc);
}

static void
l3ni1_reject_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	u_char cause = 21;

	if (pc->para.cause != NO_CAUSE)
		cause = pc->para.cause;

	MsgHead(p, pc->callref, MT_RELEASE_COMPLETE);

	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = cause | 0x80;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
	newl3state(pc, 0);
	ni1_release_l3_process(pc);
}

static void
l3ni1_release(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	u_char *p;
	int ret, cause = 0;

	StopAllL3Timer(pc);
	if ((ret = l3ni1_get_cause(pc, skb)) > 0) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "REL get_cause ret(%d)", ret);
	} else if (ret < 0)
		pc->para.cause = NO_CAUSE;
	if ((p = findie(skb->data, skb->len, IE_FACILITY, 0))) {
		l3ni1_parse_facility(pc->st, pc, pc->callref, p);
	}
	if ((ret < 0) && (pc->state != 11))
		cause = 96;
	else if (ret > 0)
		cause = 100;
	ret = check_infoelements(pc, skb, ie_RELEASE);
	if (ERR_IE_COMPREHENSION == ret)
		cause = 96;
	else if ((ERR_IE_UNRECOGNIZED == ret) && (!cause))
		cause = 99;
	if (cause)
		l3ni1_message_cause(pc, MT_RELEASE_COMPLETE, cause);
	else
		l3ni1_message(pc, MT_RELEASE_COMPLETE);
	pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
	newl3state(pc, 0);
	ni1_release_l3_process(pc);
}

static void
l3ni1_alert_req(struct l3_process *pc, u_char pr,
		void *arg)
{
	newl3state(pc, 7);
	if (!pc->prot.ni1.uus1_data[0])
		l3ni1_message(pc, MT_ALERTING);
	else
		l3ni1_msg_with_uus(pc, MT_ALERTING);
}

static void
l3ni1_proceed_req(struct l3_process *pc, u_char pr,
		  void *arg)
{
	newl3state(pc, 9);
	l3ni1_message(pc, MT_CALL_PROCEEDING);
	pc->st->l3.l3l4(pc->st, CC_PROCEED_SEND | INDICATION, pc);
}

static void
l3ni1_setup_ack_req(struct l3_process *pc, u_char pr,
		    void *arg)
{
	newl3state(pc, 25);
	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, T302, CC_T302);
	l3ni1_message(pc, MT_SETUP_ACKNOWLEDGE);
}

/********************************************/
/* deliver a incoming display message to HL */
/********************************************/
static void
l3ni1_deliver_display(struct l3_process *pc, int pr, u_char *infp)
{       u_char len;
	isdn_ctrl ic;
	struct IsdnCardState *cs;
	char *p;

	if (*infp++ != IE_DISPLAY) return;
	if ((len = *infp++) > 80) return; /* total length <= 82 */
	if (!pc->chan) return;

	p = ic.parm.display;
	while (len--)
		*p++ = *infp++;
	*p = '\0';
	ic.command = ISDN_STAT_DISPLAY;
	cs = pc->st->l1.hardware;
	ic.driver = cs->myid;
	ic.arg = pc->chan->chan;
	cs->iif.statcallb(&ic);
} /* l3ni1_deliver_display */


static void
l3ni1_progress(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int err = 0;
	u_char *p;

	if ((p = findie(skb->data, skb->len, IE_PROGRESS, 0))) {
		if (p[1] != 2) {
			err = 1;
			pc->para.cause = 100;
		} else if (!(p[2] & 0x70)) {
			switch (p[2]) {
			case 0x80:
			case 0x81:
			case 0x82:
			case 0x84:
			case 0x85:
			case 0x87:
			case 0x8a:
				switch (p[3]) {
				case 0x81:
				case 0x82:
				case 0x83:
				case 0x84:
				case 0x88:
					break;
				default:
					err = 2;
					pc->para.cause = 100;
					break;
				}
				break;
			default:
				err = 3;
				pc->para.cause = 100;
				break;
			}
		}
	} else {
		pc->para.cause = 96;
		err = 4;
	}
	if (err) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "progress error %d", err);
		l3ni1_status_send(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	err = check_infoelements(pc, skb, ie_PROGRESS);
	if (err)
		l3ni1_std_ie_err(pc, err);
	if (ERR_IE_COMPREHENSION != err)
		pc->st->l3.l3l4(pc->st, CC_PROGRESS | INDICATION, pc);
}

static void
l3ni1_notify(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int err = 0;
	u_char *p;

	if ((p = findie(skb->data, skb->len, IE_NOTIFY, 0))) {
		if (p[1] != 1) {
			err = 1;
			pc->para.cause = 100;
		} else {
			switch (p[2]) {
			case 0x80:
			case 0x81:
			case 0x82:
				break;
			default:
				pc->para.cause = 100;
				err = 2;
				break;
			}
		}
	} else {
		pc->para.cause = 96;
		err = 3;
	}
	if (err) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "notify error %d", err);
		l3ni1_status_send(pc, pr, NULL);
		return;
	}
	/* Now we are on none mandatory IEs */
	err = check_infoelements(pc, skb, ie_NOTIFY);
	if (err)
		l3ni1_std_ie_err(pc, err);
	if (ERR_IE_COMPREHENSION != err)
		pc->st->l3.l3l4(pc->st, CC_NOTIFY | INDICATION, pc);
}

static void
l3ni1_status_enq(struct l3_process *pc, u_char pr, void *arg)
{
	int ret;
	struct sk_buff *skb = arg;

	ret = check_infoelements(pc, skb, ie_STATUS_ENQUIRY);
	l3ni1_std_ie_err(pc, ret);
	pc->para.cause = 30; /* response to STATUS_ENQUIRY */
	l3ni1_status_send(pc, pr, NULL);
}

static void
l3ni1_information(struct l3_process *pc, u_char pr, void *arg)
{
	int ret;
	struct sk_buff *skb = arg;
	u_char *p;
	char tmp[32];

	ret = check_infoelements(pc, skb, ie_INFORMATION);
	if (ret)
		l3ni1_std_ie_err(pc, ret);
	if (pc->state == 25) { /* overlap receiving */
		L3DelTimer(&pc->timer);
		p = skb->data;
		if ((p = findie(p, skb->len, 0x70, 0))) {
			iecpy(tmp, p, 1);
			strcat(pc->para.setup.eazmsn, tmp);
			pc->st->l3.l3l4(pc->st, CC_MORE_INFO | INDICATION, pc);
		}
		L3AddTimer(&pc->timer, T302, CC_T302);
	}
}

/******************************/
/* handle deflection requests */
/******************************/
static void l3ni1_redir_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[128];
	u_char *p = tmp;
	u_char *subp;
	u_char len_phone = 0;
	u_char len_sub = 0;
	int l;


	strcpy(pc->prot.ni1.uus1_data, pc->chan->setup.eazmsn); /* copy uus element if available */
	if (!pc->chan->setup.phone[0])
	{ pc->para.cause = -1;
		l3ni1_disconnect_req(pc, pr, arg); /* disconnect immediately */
		return;
	} /* only uus */

	if (pc->prot.ni1.invoke_id)
		free_invoke_id(pc->st, pc->prot.ni1.invoke_id);

	if (!(pc->prot.ni1.invoke_id = new_invoke_id(pc->st)))
		return;

	MsgHead(p, pc->callref, MT_FACILITY);

	for (subp = pc->chan->setup.phone; (*subp) && (*subp != '.'); subp++) len_phone++; /* len of phone number */
	if (*subp++ == '.') len_sub = strlen(subp) + 2; /* length including info subaddress element */

	*p++ = 0x1c;   /* Facility info element */
	*p++ = len_phone + len_sub + 2 + 2 + 8 + 3 + 3; /* length of element */
	*p++ = 0x91;  /* remote operations protocol */
	*p++ = 0xa1;  /* invoke component */

	*p++ = len_phone + len_sub + 2 + 2 + 8 + 3; /* length of data */
	*p++ = 0x02;  /* invoke id tag, integer */
	*p++ = 0x01;  /* length */
	*p++ = pc->prot.ni1.invoke_id;  /* invoke id */
	*p++ = 0x02;  /* operation value tag, integer */
	*p++ = 0x01;  /* length */
	*p++ = 0x0D;  /* Call Deflect */

	*p++ = 0x30;  /* sequence phone number */
	*p++ = len_phone + 2 + 2 + 3 + len_sub; /* length */

	*p++ = 0x30;  /* Deflected to UserNumber */
	*p++ = len_phone + 2 + len_sub; /* length */
	*p++ = 0x80; /* NumberDigits */
	*p++ = len_phone; /* length */
	for (l = 0; l < len_phone; l++)
		*p++ = pc->chan->setup.phone[l];

	if (len_sub)
	{ *p++ = 0x04; /* called party subaddress */
		*p++ = len_sub - 2;
		while (*subp) *p++ = *subp++;
	}

	*p++ = 0x01; /* screening identifier */
	*p++ = 0x01;
	*p++ = pc->chan->setup.screen;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l))) return;
	memcpy(skb_put(skb, l), tmp, l);

	l3_msg(pc->st, DL_DATA | REQUEST, skb);
} /* l3ni1_redir_req */

/********************************************/
/* handle deflection request in early state */
/********************************************/
static void l3ni1_redir_req_early(struct l3_process *pc, u_char pr, void *arg)
{
	l3ni1_proceed_req(pc, pr, arg);
	l3ni1_redir_req(pc, pr, arg);
} /* l3ni1_redir_req_early */

/***********************************************/
/* handle special commands for this protocol.  */
/* Examples are call independent services like */
/* remote operations with dummy  callref.      */
/***********************************************/
static int l3ni1_cmd_global(struct PStack *st, isdn_ctrl *ic)
{ u_char id;
	u_char temp[265];
	u_char *p = temp;
	int i, l, proc_len;
	struct sk_buff *skb;
	struct l3_process *pc = NULL;

	switch (ic->arg)
	{ case NI1_CMD_INVOKE:
			if (ic->parm.ni1_io.datalen < 0) return (-2); /* invalid parameter */

			for (proc_len = 1, i = ic->parm.ni1_io.proc >> 8; i; i++)
				i = i >> 8; /* add one byte */
			l = ic->parm.ni1_io.datalen + proc_len + 8; /* length excluding ie header */
			if (l > 255)
				return (-2); /* too long */

			if (!(id = new_invoke_id(st)))
				return (0); /* first get a invoke id -> return if no available */

			i = -1;
			MsgHead(p, i, MT_FACILITY); /* build message head */
			*p++ = 0x1C; /* Facility IE */
			*p++ = l; /* length of ie */
			*p++ = 0x91; /* remote operations */
			*p++ = 0xA1; /* invoke */
			*p++ = l - 3; /* length of invoke */
			*p++ = 0x02; /* invoke id tag */
			*p++ = 0x01; /* length is 1 */
			*p++ = id; /* invoke id */
			*p++ = 0x02; /* operation */
			*p++ = proc_len; /* length of operation */

			for (i = proc_len; i; i--)
				*p++ = (ic->parm.ni1_io.proc >> (i - 1)) & 0xFF;
			memcpy(p, ic->parm.ni1_io.data, ic->parm.ni1_io.datalen); /* copy data */
			l = (p - temp) + ic->parm.ni1_io.datalen; /* total length */

			if (ic->parm.ni1_io.timeout > 0) {
				pc = ni1_new_l3_process(st, -1);
				if (!pc) {
					free_invoke_id(st, id);
					return (-2);
				}
				/* remember id */
				pc->prot.ni1.ll_id = ic->parm.ni1_io.ll_id;
				/* and procedure */
				pc->prot.ni1.proc = ic->parm.ni1_io.proc;
			}

			if (!(skb = l3_alloc_skb(l)))
			{ free_invoke_id(st, id);
				if (pc) ni1_release_l3_process(pc);
				return (-2);
			}
			memcpy(skb_put(skb, l), temp, l);

			if (pc)
			{ pc->prot.ni1.invoke_id = id; /* remember id */
				L3AddTimer(&pc->timer, ic->parm.ni1_io.timeout, CC_TNI1_IO | REQUEST);
			}

			l3_msg(st, DL_DATA | REQUEST, skb);
			ic->parm.ni1_io.hl_id = id; /* return id */
			return (0);

	case NI1_CMD_INVOKE_ABORT:
		if ((pc = l3ni1_search_dummy_proc(st, ic->parm.ni1_io.hl_id)))
		{ L3DelTimer(&pc->timer); /* remove timer */
			ni1_release_l3_process(pc);
			return (0);
		}
		else
		{ l3_debug(st, "l3ni1_cmd_global abort unknown id");
			return (-2);
		}
		break;

	default:
		l3_debug(st, "l3ni1_cmd_global unknown cmd 0x%lx", ic->arg);
		return (-1);
	} /* switch ic-> arg */
	return (-1);
} /* l3ni1_cmd_global */

static void
l3ni1_io_timer(struct l3_process *pc)
{ isdn_ctrl ic;
	struct IsdnCardState *cs = pc->st->l1.hardware;

	L3DelTimer(&pc->timer); /* remove timer */

	ic.driver = cs->myid;
	ic.command = ISDN_STAT_PROT;
	ic.arg = NI1_STAT_INVOKE_ERR;
	ic.parm.ni1_io.hl_id = pc->prot.ni1.invoke_id;
	ic.parm.ni1_io.ll_id = pc->prot.ni1.ll_id;
	ic.parm.ni1_io.proc = pc->prot.ni1.proc;
	ic.parm.ni1_io.timeout = -1;
	ic.parm.ni1_io.datalen = 0;
	ic.parm.ni1_io.data = NULL;
	free_invoke_id(pc->st, pc->prot.ni1.invoke_id);
	pc->prot.ni1.invoke_id = 0; /* reset id */

	cs->iif.statcallb(&ic);

	ni1_release_l3_process(pc);
} /* l3ni1_io_timer */

static void
l3ni1_release_ind(struct l3_process *pc, u_char pr, void *arg)
{
	u_char *p;
	struct sk_buff *skb = arg;
	int callState = 0;
	p = skb->data;

	if ((p = findie(p, skb->len, IE_CALL_STATE, 0))) {
		p++;
		if (1 == *p++)
			callState = *p;
	}
	if (callState == 0) {
		/* ETS 300-104 7.6.1, 8.6.1, 10.6.1... and 16.1
		 * set down layer 3 without sending any message
		 */
		pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
		newl3state(pc, 0);
		ni1_release_l3_process(pc);
	} else {
		pc->st->l3.l3l4(pc->st, CC_IGNORE | INDICATION, pc);
	}
}

static void
l3ni1_dummy(struct l3_process *pc, u_char pr, void *arg)
{
}

static void
l3ni1_t302(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.loc = 0;
	pc->para.cause = 28; /* invalid number */
	l3ni1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_SETUP_ERR, pc);
}

static void
l3ni1_t303(struct l3_process *pc, u_char pr, void *arg)
{
	if (pc->N303 > 0) {
		pc->N303--;
		L3DelTimer(&pc->timer);
		l3ni1_setup_req(pc, pr, arg);
	} else {
		L3DelTimer(&pc->timer);
		l3ni1_message_cause(pc, MT_RELEASE_COMPLETE, 102);
		pc->st->l3.l3l4(pc->st, CC_NOSETUP_RSP, pc);
		ni1_release_l3_process(pc);
	}
}

static void
l3ni1_t304(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.loc = 0;
	pc->para.cause = 102;
	l3ni1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_SETUP_ERR, pc);

}

static void
l3ni1_t305(struct l3_process *pc, u_char pr, void *arg)
{
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	struct sk_buff *skb;
	u_char cause = 16;

	L3DelTimer(&pc->timer);
	if (pc->para.cause != NO_CAUSE)
		cause = pc->para.cause;

	MsgHead(p, pc->callref, MT_RELEASE);

	*p++ = IE_CAUSE;
	*p++ = 0x2;
	*p++ = 0x80;
	*p++ = cause | 0x80;

	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	newl3state(pc, 19);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	L3AddTimer(&pc->timer, T308, CC_T308_1);
}

static void
l3ni1_t310(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.loc = 0;
	pc->para.cause = 102;
	l3ni1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_SETUP_ERR, pc);
}

static void
l3ni1_t313(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.loc = 0;
	pc->para.cause = 102;
	l3ni1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_CONNECT_ERR, pc);
}

static void
l3ni1_t308_1(struct l3_process *pc, u_char pr, void *arg)
{
	newl3state(pc, 19);
	L3DelTimer(&pc->timer);
	l3ni1_message(pc, MT_RELEASE);
	L3AddTimer(&pc->timer, T308, CC_T308_2);
}

static void
l3ni1_t308_2(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_RELEASE_ERR, pc);
	ni1_release_l3_process(pc);
}

static void
l3ni1_t318(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.cause = 102;	/* Timer expiry */
	pc->para.loc = 0;	/* local */
	pc->st->l3.l3l4(pc->st, CC_RESUME_ERR, pc);
	newl3state(pc, 19);
	l3ni1_message(pc, MT_RELEASE);
	L3AddTimer(&pc->timer, T308, CC_T308_1);
}

static void
l3ni1_t319(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->para.cause = 102;	/* Timer expiry */
	pc->para.loc = 0;	/* local */
	pc->st->l3.l3l4(pc->st, CC_SUSPEND_ERR, pc);
	newl3state(pc, 10);
}

static void
l3ni1_restart(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
	ni1_release_l3_process(pc);
}

static void
l3ni1_status(struct l3_process *pc, u_char pr, void *arg)
{
	u_char *p;
	struct sk_buff *skb = arg;
	int ret;
	u_char cause = 0, callState = 0;

	if ((ret = l3ni1_get_cause(pc, skb))) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "STATUS get_cause ret(%d)", ret);
		if (ret < 0)
			cause = 96;
		else if (ret > 0)
			cause = 100;
	}
	if ((p = findie(skb->data, skb->len, IE_CALL_STATE, 0))) {
		p++;
		if (1 == *p++) {
			callState = *p;
			if (!ie_in_set(pc, *p, l3_valid_states))
				cause = 100;
		} else
			cause = 100;
	} else
		cause = 96;
	if (!cause) { /*  no error before */
		ret = check_infoelements(pc, skb, ie_STATUS);
		if (ERR_IE_COMPREHENSION == ret)
			cause = 96;
		else if (ERR_IE_UNRECOGNIZED == ret)
			cause = 99;
	}
	if (cause) {
		u_char tmp;

		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "STATUS error(%d/%d)", ret, cause);
		tmp = pc->para.cause;
		pc->para.cause = cause;
		l3ni1_status_send(pc, 0, NULL);
		if (cause == 99)
			pc->para.cause = tmp;
		else
			return;
	}
	cause = pc->para.cause;
	if (((cause & 0x7f) == 111) && (callState == 0)) {
		/* ETS 300-104 7.6.1, 8.6.1, 10.6.1...
		 * if received MT_STATUS with cause == 111 and call
		 * state == 0, then we must set down layer 3
		 */
		pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
		newl3state(pc, 0);
		ni1_release_l3_process(pc);
	}
}

static void
l3ni1_facility(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	ret = check_infoelements(pc, skb, ie_FACILITY);
	l3ni1_std_ie_err(pc, ret);
	{
		u_char *p;
		if ((p = findie(skb->data, skb->len, IE_FACILITY, 0)))
			l3ni1_parse_facility(pc->st, pc, pc->callref, p);
	}
}

static void
l3ni1_suspend_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[32];
	u_char *p = tmp;
	u_char i, l;
	u_char *msg = pc->chan->setup.phone;

	MsgHead(p, pc->callref, MT_SUSPEND);
	l = *msg++;
	if (l && (l <= 10)) {	/* Max length 10 octets */
		*p++ = IE_CALL_ID;
		*p++ = l;
		for (i = 0; i < l; i++)
			*p++ = *msg++;
	} else if (l) {
		l3_debug(pc->st, "SUS wrong CALL_ID len %d", l);
		return;
	}
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	newl3state(pc, 15);
	L3AddTimer(&pc->timer, T319, CC_T319);
}

static void
l3ni1_suspend_ack(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	L3DelTimer(&pc->timer);
	newl3state(pc, 0);
	pc->para.cause = NO_CAUSE;
	pc->st->l3.l3l4(pc->st, CC_SUSPEND | CONFIRM, pc);
	/* We don't handle suspend_ack for IE errors now */
	if ((ret = check_infoelements(pc, skb, ie_SUSPEND_ACKNOWLEDGE)))
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "SUSPACK check ie(%d)", ret);
	ni1_release_l3_process(pc);
}

static void
l3ni1_suspend_rej(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	if ((ret = l3ni1_get_cause(pc, skb))) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "SUSP_REJ get_cause ret(%d)", ret);
		if (ret < 0)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3ni1_status_send(pc, pr, NULL);
		return;
	}
	ret = check_infoelements(pc, skb, ie_SUSPEND_REJECT);
	if (ERR_IE_COMPREHENSION == ret) {
		l3ni1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_SUSPEND_ERR, pc);
	newl3state(pc, 10);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3ni1_std_ie_err(pc, ret);
}

static void
l3ni1_resume_req(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb;
	u_char tmp[32];
	u_char *p = tmp;
	u_char i, l;
	u_char *msg = pc->para.setup.phone;

	MsgHead(p, pc->callref, MT_RESUME);

	l = *msg++;
	if (l && (l <= 10)) {	/* Max length 10 octets */
		*p++ = IE_CALL_ID;
		*p++ = l;
		for (i = 0; i < l; i++)
			*p++ = *msg++;
	} else if (l) {
		l3_debug(pc->st, "RES wrong CALL_ID len %d", l);
		return;
	}
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
	newl3state(pc, 17);
	L3AddTimer(&pc->timer, T318, CC_T318);
}

static void
l3ni1_resume_ack(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int id, ret;

	if ((id = l3ni1_get_channel_id(pc, skb)) > 0) {
		if ((0 == id) || ((3 == id) && (0x10 == pc->para.moderate))) {
			if (pc->debug & L3_DEB_WARN)
				l3_debug(pc->st, "resume ack with wrong chid %x", id);
			pc->para.cause = 100;
			l3ni1_status_send(pc, pr, NULL);
			return;
		}
		pc->para.bchannel = id;
	} else if (1 == pc->state) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "resume ack without chid (ret %d)", id);
		pc->para.cause = 96;
		l3ni1_status_send(pc, pr, NULL);
		return;
	}
	ret = check_infoelements(pc, skb, ie_RESUME_ACKNOWLEDGE);
	if (ERR_IE_COMPREHENSION == ret) {
		l3ni1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_RESUME | CONFIRM, pc);
	newl3state(pc, 10);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3ni1_std_ie_err(pc, ret);
}

static void
l3ni1_resume_rej(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;
	int ret;

	if ((ret = l3ni1_get_cause(pc, skb))) {
		if (pc->debug & L3_DEB_WARN)
			l3_debug(pc->st, "RES_REJ get_cause ret(%d)", ret);
		if (ret < 0)
			pc->para.cause = 96;
		else
			pc->para.cause = 100;
		l3ni1_status_send(pc, pr, NULL);
		return;
	}
	ret = check_infoelements(pc, skb, ie_RESUME_REJECT);
	if (ERR_IE_COMPREHENSION == ret) {
		l3ni1_std_ie_err(pc, ret);
		return;
	}
	L3DelTimer(&pc->timer);
	pc->st->l3.l3l4(pc->st, CC_RESUME_ERR, pc);
	newl3state(pc, 0);
	if (ret) /* STATUS for none mandatory IE errors after actions are taken */
		l3ni1_std_ie_err(pc, ret);
	ni1_release_l3_process(pc);
}

static void
l3ni1_global_restart(struct l3_process *pc, u_char pr, void *arg)
{
	u_char tmp[32];
	u_char *p;
	u_char ri, ch = 0, chan = 0;
	int l;
	struct sk_buff *skb = arg;
	struct l3_process *up;

	newl3state(pc, 2);
	L3DelTimer(&pc->timer);
	p = skb->data;
	if ((p = findie(p, skb->len, IE_RESTART_IND, 0))) {
		ri = p[2];
		l3_debug(pc->st, "Restart %x", ri);
	} else {
		l3_debug(pc->st, "Restart without restart IE");
		ri = 0x86;
	}
	p = skb->data;
	if ((p = findie(p, skb->len, IE_CHANNEL_ID, 0))) {
		chan = p[2] & 3;
		ch = p[2];
		if (pc->st->l3.debug)
			l3_debug(pc->st, "Restart for channel %d", chan);
	}
	newl3state(pc, 2);
	up = pc->st->l3.proc;
	while (up) {
		if ((ri & 7) == 7)
			up->st->lli.l4l3(up->st, CC_RESTART | REQUEST, up);
		else if (up->para.bchannel == chan)
			up->st->lli.l4l3(up->st, CC_RESTART | REQUEST, up);

		up = up->next;
	}
	p = tmp;
	MsgHead(p, pc->callref, MT_RESTART_ACKNOWLEDGE);
	if (chan) {
		*p++ = IE_CHANNEL_ID;
		*p++ = 1;
		*p++ = ch | 0x80;
	}
	*p++ = 0x79;		/* RESTART Ind */
	*p++ = 1;
	*p++ = ri;
	l = p - tmp;
	if (!(skb = l3_alloc_skb(l)))
		return;
	memcpy(skb_put(skb, l), tmp, l);
	newl3state(pc, 0);
	l3_msg(pc->st, DL_DATA | REQUEST, skb);
}

static void
l3ni1_dl_reset(struct l3_process *pc, u_char pr, void *arg)
{
	pc->para.cause = 0x29;          /* Temporary failure */
	pc->para.loc = 0;
	l3ni1_disconnect_req(pc, pr, NULL);
	pc->st->l3.l3l4(pc->st, CC_SETUP_ERR, pc);
}

static void
l3ni1_dl_release(struct l3_process *pc, u_char pr, void *arg)
{
	newl3state(pc, 0);
	pc->para.cause = 0x1b;          /* Destination out of order */
	pc->para.loc = 0;
	pc->st->l3.l3l4(pc->st, CC_RELEASE | INDICATION, pc);
	release_l3_process(pc);
}

static void
l3ni1_dl_reestablish(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, T309, CC_T309);
	l3_msg(pc->st, DL_ESTABLISH | REQUEST, NULL);
}

static void
l3ni1_dl_reest_status(struct l3_process *pc, u_char pr, void *arg)
{
	L3DelTimer(&pc->timer);

	pc->para.cause = 0x1F; /* normal, unspecified */
	l3ni1_status_send(pc, 0, NULL);
}

static void l3ni1_SendSpid(struct l3_process *pc, u_char pr, struct sk_buff *skb, int iNewState)
{
	u_char *p;
	char *pSPID;
	struct Channel *pChan = pc->st->lli.userdata;
	int l;

	if (skb)
		dev_kfree_skb(skb);

	if (!(pSPID = strchr(pChan->setup.eazmsn, ':')))
	{
		printk(KERN_ERR "SPID not supplied in EAZMSN %s\n", pChan->setup.eazmsn);
		newl3state(pc, 0);
		pc->st->l3.l3l2(pc->st, DL_RELEASE | REQUEST, NULL);
		return;
	}

	l = strlen(++pSPID);
	if (!(skb = l3_alloc_skb(5 + l)))
	{
		printk(KERN_ERR "HiSax can't get memory to send SPID\n");
		return;
	}

	p = skb_put(skb, 5);
	*p++ = PROTO_DIS_EURO;
	*p++ = 0;
	*p++ = MT_INFORMATION;
	*p++ = IE_SPID;
	*p++ = l;

	memcpy(skb_put(skb, l), pSPID, l);

	newl3state(pc, iNewState);

	L3DelTimer(&pc->timer);
	L3AddTimer(&pc->timer, TSPID, CC_TSPID);

	pc->st->l3.l3l2(pc->st, DL_DATA | REQUEST, skb);
}

static void l3ni1_spid_send(struct l3_process *pc, u_char pr, void *arg)
{
	l3ni1_SendSpid(pc, pr, arg, 20);
}

static void l3ni1_spid_epid(struct l3_process *pc, u_char pr, void *arg)
{
	struct sk_buff *skb = arg;

	if (skb->data[1] == 0)
		if (skb->data[3] == IE_ENDPOINT_ID)
		{
			L3DelTimer(&pc->timer);
			newl3state(pc, 0);
			l3_msg(pc->st, DL_ESTABLISH | CONFIRM, NULL);
		}
	dev_kfree_skb(skb);
}

static void l3ni1_spid_tout(struct l3_process *pc, u_char pr, void *arg)
{
	if (pc->state < 22)
		l3ni1_SendSpid(pc, pr, arg, pc->state + 1);
	else
	{
		L3DelTimer(&pc->timer);
		dev_kfree_skb(arg);

		printk(KERN_ERR "SPID not accepted\n");
		newl3state(pc, 0);
		pc->st->l3.l3l2(pc->st, DL_RELEASE | REQUEST, NULL);
	}
}

/* *INDENT-OFF* */
static struct stateentry downstatelist[] =
{
	{SBIT(0),
	 CC_SETUP | REQUEST, l3ni1_setup_req},
	{SBIT(0),
	 CC_RESUME | REQUEST, l3ni1_resume_req},
	{SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4) | SBIT(6) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) | SBIT(25),
	 CC_DISCONNECT | REQUEST, l3ni1_disconnect_req},
	{SBIT(12),
	 CC_RELEASE | REQUEST, l3ni1_release_req},
	{ALL_STATES,
	 CC_RESTART | REQUEST, l3ni1_restart},
	{SBIT(6) | SBIT(25),
	 CC_IGNORE | REQUEST, l3ni1_reset},
	{SBIT(6) | SBIT(25),
	 CC_REJECT | REQUEST, l3ni1_reject_req},
	{SBIT(6) | SBIT(25),
	 CC_PROCEED_SEND | REQUEST, l3ni1_proceed_req},
	{SBIT(6),
	 CC_MORE_INFO | REQUEST, l3ni1_setup_ack_req},
	{SBIT(25),
	 CC_MORE_INFO | REQUEST, l3ni1_dummy},
	{SBIT(6) | SBIT(9) | SBIT(25),
	 CC_ALERTING | REQUEST, l3ni1_alert_req},
	{SBIT(6) | SBIT(7) | SBIT(9) | SBIT(25),
	 CC_SETUP | RESPONSE, l3ni1_setup_rsp},
	{SBIT(10),
	 CC_SUSPEND | REQUEST, l3ni1_suspend_req},
	{SBIT(7) | SBIT(9) | SBIT(25),
	 CC_REDIR | REQUEST, l3ni1_redir_req},
	{SBIT(6),
	 CC_REDIR | REQUEST, l3ni1_redir_req_early},
	{SBIT(9) | SBIT(25),
	 CC_DISCONNECT | REQUEST, l3ni1_disconnect_req},
	{SBIT(25),
	 CC_T302, l3ni1_t302},
	{SBIT(1),
	 CC_T303, l3ni1_t303},
	{SBIT(2),
	 CC_T304, l3ni1_t304},
	{SBIT(3),
	 CC_T310, l3ni1_t310},
	{SBIT(8),
	 CC_T313, l3ni1_t313},
	{SBIT(11),
	 CC_T305, l3ni1_t305},
	{SBIT(15),
	 CC_T319, l3ni1_t319},
	{SBIT(17),
	 CC_T318, l3ni1_t318},
	{SBIT(19),
	 CC_T308_1, l3ni1_t308_1},
	{SBIT(19),
	 CC_T308_2, l3ni1_t308_2},
	{SBIT(10),
	 CC_T309, l3ni1_dl_release},
	{ SBIT(20) | SBIT(21) | SBIT(22),
	  CC_TSPID, l3ni1_spid_tout },
};

static struct stateentry datastatelist[] =
{
	{ALL_STATES,
	 MT_STATUS_ENQUIRY, l3ni1_status_enq},
	{ALL_STATES,
	 MT_FACILITY, l3ni1_facility},
	{SBIT(19),
	 MT_STATUS, l3ni1_release_ind},
	{ALL_STATES,
	 MT_STATUS, l3ni1_status},
	{SBIT(0),
	 MT_SETUP, l3ni1_setup},
	{SBIT(6) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) | SBIT(11) | SBIT(12) |
	 SBIT(15) | SBIT(17) | SBIT(19) | SBIT(25),
	 MT_SETUP, l3ni1_dummy},
	{SBIT(1) | SBIT(2),
	 MT_CALL_PROCEEDING, l3ni1_call_proc},
	{SBIT(1),
	 MT_SETUP_ACKNOWLEDGE, l3ni1_setup_ack},
	{SBIT(2) | SBIT(3),
	 MT_ALERTING, l3ni1_alerting},
	{SBIT(2) | SBIT(3),
	 MT_PROGRESS, l3ni1_progress},
	{SBIT(2) | SBIT(3) | SBIT(4) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) |
	 SBIT(11) | SBIT(12) | SBIT(15) | SBIT(17) | SBIT(19) | SBIT(25),
	 MT_INFORMATION, l3ni1_information},
	{SBIT(10) | SBIT(11) | SBIT(15),
	 MT_NOTIFY, l3ni1_notify},
	{SBIT(0) | SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4) | SBIT(7) | SBIT(8) | SBIT(10) |
	 SBIT(11) | SBIT(12) | SBIT(15) | SBIT(17) | SBIT(19) | SBIT(25),
	 MT_RELEASE_COMPLETE, l3ni1_release_cmpl},
	{SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) | SBIT(11) | SBIT(12) | SBIT(15) | SBIT(17) | SBIT(25),
	 MT_RELEASE, l3ni1_release},
	{SBIT(19),  MT_RELEASE, l3ni1_release_ind},
	{SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4) | SBIT(7) | SBIT(8) | SBIT(9) | SBIT(10) | SBIT(11) | SBIT(15) | SBIT(17) | SBIT(25),
	 MT_DISCONNECT, l3ni1_disconnect},
	{SBIT(19),
	 MT_DISCONNECT, l3ni1_dummy},
	{SBIT(1) | SBIT(2) | SBIT(3) | SBIT(4),
	 MT_CONNECT, l3ni1_connect},
	{SBIT(8),
	 MT_CONNECT_ACKNOWLEDGE, l3ni1_connect_ack},
	{SBIT(15),
	 MT_SUSPEND_ACKNOWLEDGE, l3ni1_suspend_ack},
	{SBIT(15),
	 MT_SUSPEND_REJECT, l3ni1_suspend_rej},
	{SBIT(17),
	 MT_RESUME_ACKNOWLEDGE, l3ni1_resume_ack},
	{SBIT(17),
	 MT_RESUME_REJECT, l3ni1_resume_rej},
};

static struct stateentry globalmes_list[] =
{
	{ALL_STATES,
	 MT_STATUS, l3ni1_status},
	{SBIT(0),
	 MT_RESTART, l3ni1_global_restart},
/*	{SBIT(1),
	MT_RESTART_ACKNOWLEDGE, l3ni1_restart_ack},
*/
	{ SBIT(0), MT_DL_ESTABLISHED, l3ni1_spid_send },
	{ SBIT(20) | SBIT(21) | SBIT(22), MT_INFORMATION, l3ni1_spid_epid },
};

static struct stateentry manstatelist[] =
{
	{SBIT(2),
	 DL_ESTABLISH | INDICATION, l3ni1_dl_reset},
	{SBIT(10),
	 DL_ESTABLISH | CONFIRM, l3ni1_dl_reest_status},
	{SBIT(10),
	 DL_RELEASE | INDICATION, l3ni1_dl_reestablish},
	{ALL_STATES,
	 DL_RELEASE | INDICATION, l3ni1_dl_release},
};

/* *INDENT-ON* */


static void
global_handler(struct PStack *st, int mt, struct sk_buff *skb)
{
	u_char tmp[16];
	u_char *p = tmp;
	int l;
	int i;
	struct l3_process *proc = st->l3.global;

	if (skb)
		proc->callref = skb->data[2]; /* cr flag */
	else
		proc->callref = 0;
	for (i = 0; i < ARRAY_SIZE(globalmes_list); i++)
		if ((mt == globalmes_list[i].primitive) &&
		    ((1 << proc->state) & globalmes_list[i].state))
			break;
	if (i == ARRAY_SIZE(globalmes_list)) {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "ni1 global state %d mt %x unhandled",
				 proc->state, mt);
		}
		MsgHead(p, proc->callref, MT_STATUS);
		*p++ = IE_CAUSE;
		*p++ = 0x2;
		*p++ = 0x80;
		*p++ = 81 | 0x80;	/* invalid cr */
		*p++ = 0x14;		/* CallState */
		*p++ = 0x1;
		*p++ = proc->state & 0x3f;
		l = p - tmp;
		if (!(skb = l3_alloc_skb(l)))
			return;
		memcpy(skb_put(skb, l), tmp, l);
		l3_msg(proc->st, DL_DATA | REQUEST, skb);
	} else {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "ni1 global %d mt %x",
				 proc->state, mt);
		}
		globalmes_list[i].rout(proc, mt, skb);
	}
}

static void
ni1up(struct PStack *st, int pr, void *arg)
{
	int i, mt, cr, callState;
	char *ptr;
	u_char *p;
	struct sk_buff *skb = arg;
	struct l3_process *proc;

	switch (pr) {
	case (DL_DATA | INDICATION):
	case (DL_UNIT_DATA | INDICATION):
		break;
	case (DL_ESTABLISH | INDICATION):
	case (DL_RELEASE | INDICATION):
	case (DL_RELEASE | CONFIRM):
		l3_msg(st, pr, arg);
		return;
		break;

	case (DL_ESTABLISH | CONFIRM):
		global_handler(st, MT_DL_ESTABLISHED, NULL);
		return;

	default:
		printk(KERN_ERR "HiSax ni1up unknown pr=%04x\n", pr);
		return;
	}
	if (skb->len < 3) {
		l3_debug(st, "ni1up frame too short(%d)", skb->len);
		dev_kfree_skb(skb);
		return;
	}

	if (skb->data[0] != PROTO_DIS_EURO) {
		if (st->l3.debug & L3_DEB_PROTERR) {
			l3_debug(st, "ni1up%sunexpected discriminator %x message len %d",
				 (pr == (DL_DATA | INDICATION)) ? " " : "(broadcast) ",
				 skb->data[0], skb->len);
		}
		dev_kfree_skb(skb);
		return;
	}
	cr = getcallref(skb->data);
	if (skb->len < ((skb->data[1] & 0x0f) + 3)) {
		l3_debug(st, "ni1up frame too short(%d)", skb->len);
		dev_kfree_skb(skb);
		return;
	}
	mt = skb->data[skb->data[1] + 2];
	if (st->l3.debug & L3_DEB_STATE)
		l3_debug(st, "ni1up cr %d", cr);
	if (cr == -2) {  /* wrong Callref */
		if (st->l3.debug & L3_DEB_WARN)
			l3_debug(st, "ni1up wrong Callref");
		dev_kfree_skb(skb);
		return;
	} else if (cr == -1) {	/* Dummy Callref */
		if (mt == MT_FACILITY)
		{
			if ((p = findie(skb->data, skb->len, IE_FACILITY, 0))) {
				l3ni1_parse_facility(st, NULL,
						     (pr == (DL_DATA | INDICATION)) ? -1 : -2, p);
				dev_kfree_skb(skb);
				return;
			}
		}
		else
		{
			global_handler(st, mt, skb);
			return;
		}

		if (st->l3.debug & L3_DEB_WARN)
			l3_debug(st, "ni1up dummy Callref (no facility msg or ie)");
		dev_kfree_skb(skb);
		return;
	} else if ((((skb->data[1] & 0x0f) == 1) && (0 == (cr & 0x7f))) ||
		   (((skb->data[1] & 0x0f) == 2) && (0 == (cr & 0x7fff)))) {	/* Global CallRef */
		if (st->l3.debug & L3_DEB_STATE)
			l3_debug(st, "ni1up Global CallRef");
		global_handler(st, mt, skb);
		dev_kfree_skb(skb);
		return;
	} else if (!(proc = getl3proc(st, cr))) {
		/* No transaction process exist, that means no call with
		 * this callreference is active
		 */
		if (mt == MT_SETUP) {
			/* Setup creates a new transaction process */
			if (skb->data[2] & 0x80) {
				/* Setup with wrong CREF flag */
				if (st->l3.debug & L3_DEB_STATE)
					l3_debug(st, "ni1up wrong CRef flag");
				dev_kfree_skb(skb);
				return;
			}
			if (!(proc = ni1_new_l3_process(st, cr))) {
				/* May be to answer with RELEASE_COMPLETE and
				 * CAUSE 0x2f "Resource unavailable", but this
				 * need a new_l3_process too ... arghh
				 */
				dev_kfree_skb(skb);
				return;
			}
		} else if (mt == MT_STATUS) {
			if ((ptr = findie(skb->data, skb->len, IE_CAUSE, 0)) != NULL) {
				ptr++;
				if (*ptr++ == 2)
					ptr++;
			}
			callState = 0;
			if ((ptr = findie(skb->data, skb->len, IE_CALL_STATE, 0)) != NULL) {
				ptr++;
				if (*ptr++ == 2)
					ptr++;
				callState = *ptr;
			}
			/* ETS 300-104 part 2.4.1
			 * if setup has not been made and a message type
			 * MT_STATUS is received with call state == 0,
			 * we must send nothing
			 */
			if (callState != 0) {
				/* ETS 300-104 part 2.4.2
				 * if setup has not been made and a message type
				 * MT_STATUS is received with call state != 0,
				 * we must send MT_RELEASE_COMPLETE cause 101
				 */
				if ((proc = ni1_new_l3_process(st, cr))) {
					proc->para.cause = 101;
					l3ni1_msg_without_setup(proc, 0, NULL);
				}
			}
			dev_kfree_skb(skb);
			return;
		} else if (mt == MT_RELEASE_COMPLETE) {
			dev_kfree_skb(skb);
			return;
		} else {
			/* ETS 300-104 part 2
			 * if setup has not been made and a message type
			 * (except MT_SETUP and RELEASE_COMPLETE) is received,
			 * we must send MT_RELEASE_COMPLETE cause 81 */
			dev_kfree_skb(skb);
			if ((proc = ni1_new_l3_process(st, cr))) {
				proc->para.cause = 81;
				l3ni1_msg_without_setup(proc, 0, NULL);
			}
			return;
		}
	}
	if (l3ni1_check_messagetype_validity(proc, mt, skb)) {
		dev_kfree_skb(skb);
		return;
	}
	if ((p = findie(skb->data, skb->len, IE_DISPLAY, 0)) != NULL)
		l3ni1_deliver_display(proc, pr, p); /* Display IE included */
	for (i = 0; i < ARRAY_SIZE(datastatelist); i++)
		if ((mt == datastatelist[i].primitive) &&
		    ((1 << proc->state) & datastatelist[i].state))
			break;
	if (i == ARRAY_SIZE(datastatelist)) {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "ni1up%sstate %d mt %#x unhandled",
				 (pr == (DL_DATA | INDICATION)) ? " " : "(broadcast) ",
				 proc->state, mt);
		}
		if ((MT_RELEASE_COMPLETE != mt) && (MT_RELEASE != mt)) {
			proc->para.cause = 101;
			l3ni1_status_send(proc, pr, skb);
		}
	} else {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "ni1up%sstate %d mt %x",
				 (pr == (DL_DATA | INDICATION)) ? " " : "(broadcast) ",
				 proc->state, mt);
		}
		datastatelist[i].rout(proc, pr, skb);
	}
	dev_kfree_skb(skb);
	return;
}

static void
ni1down(struct PStack *st, int pr, void *arg)
{
	int i, cr;
	struct l3_process *proc;
	struct Channel *chan;

	if ((DL_ESTABLISH | REQUEST) == pr) {
		l3_msg(st, pr, NULL);
		return;
	} else if (((CC_SETUP | REQUEST) == pr) || ((CC_RESUME | REQUEST) == pr)) {
		chan = arg;
		cr = newcallref();
		cr |= 0x80;
		if ((proc = ni1_new_l3_process(st, cr))) {
			proc->chan = chan;
			chan->proc = proc;
			memcpy(&proc->para.setup, &chan->setup, sizeof(setup_parm));
			proc->callref = cr;
		}
	} else {
		proc = arg;
	}
	if (!proc) {
		printk(KERN_ERR "HiSax ni1down without proc pr=%04x\n", pr);
		return;
	}

	if (pr == (CC_TNI1_IO | REQUEST)) {
		l3ni1_io_timer(proc); /* timer expires */
		return;
	}

	for (i = 0; i < ARRAY_SIZE(downstatelist); i++)
		if ((pr == downstatelist[i].primitive) &&
		    ((1 << proc->state) & downstatelist[i].state))
			break;
	if (i == ARRAY_SIZE(downstatelist)) {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "ni1down state %d prim %#x unhandled",
				 proc->state, pr);
		}
	} else {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "ni1down state %d prim %#x",
				 proc->state, pr);
		}
		downstatelist[i].rout(proc, pr, arg);
	}
}

static void
ni1man(struct PStack *st, int pr, void *arg)
{
	int i;
	struct l3_process *proc = arg;

	if (!proc) {
		printk(KERN_ERR "HiSax ni1man without proc pr=%04x\n", pr);
		return;
	}
	for (i = 0; i < ARRAY_SIZE(manstatelist); i++)
		if ((pr == manstatelist[i].primitive) &&
		    ((1 << proc->state) & manstatelist[i].state))
			break;
	if (i == ARRAY_SIZE(manstatelist)) {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "cr %d ni1man state %d prim %#x unhandled",
				 proc->callref & 0x7f, proc->state, pr);
		}
	} else {
		if (st->l3.debug & L3_DEB_STATE) {
			l3_debug(st, "cr %d ni1man state %d prim %#x",
				 proc->callref & 0x7f, proc->state, pr);
		}
		manstatelist[i].rout(proc, pr, arg);
	}
}

void
setstack_ni1(struct PStack *st)
{
	char tmp[64];
	int i;

	st->lli.l4l3 = ni1down;
	st->lli.l4l3_proto = l3ni1_cmd_global;
	st->l2.l2l3 = ni1up;
	st->l3.l3ml3 = ni1man;
	st->l3.N303 = 1;
	st->prot.ni1.last_invoke_id = 0;
	st->prot.ni1.invoke_used[0] = 1; /* Bit 0 must always be set to 1 */
	i = 1;
	while (i < 32)
		st->prot.ni1.invoke_used[i++] = 0;

	if (!(st->l3.global = kmalloc(sizeof(struct l3_process), GFP_ATOMIC))) {
		printk(KERN_ERR "HiSax can't get memory for ni1 global CR\n");
	} else {
		st->l3.global->state = 0;
		st->l3.global->callref = 0;
		st->l3.global->next = NULL;
		st->l3.global->debug = L3_DEB_WARN;
		st->l3.global->st = st;
		st->l3.global->N303 = 1;
		st->l3.global->prot.ni1.invoke_id = 0;

		L3InitTimer(st->l3.global, &st->l3.global->timer);
	}
	strcpy(tmp, ni1_revision);
	printk(KERN_INFO "HiSax: National ISDN-1 Rev. %s\n", HiSax_getrev(tmp));
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    _descr_szE _ZN11__sanitizer16struct_arpreq_szE _ZN11__sanitizer15struct_ifreq_szE _ZN11__sanitizer14struct_mtop_szE _ZN11__sanitizer16struct_termio_szE _ZN11__sanitizer26struct_cdrom_read_audio_szE _ZN11__sanitizer17struct_sysinfo_szE _ZN11__sanitizer23struct_input_absinfo_szE _ZN11__sanitizer19struct_midi_info_szE _ZN11__sanitizer20struct_synth_info_szE _ZN11__sanitizer28struct_cdrom_multisession_szE _ZN11__sanitizer21struct_sched_param_szE _ZN11__sanitizer23struct_cdrom_volctrl_szE _ZN11__sanitizer23struct_cdrom_subchnl_szE _ZN11__sanitizer19struct_itimerval_szE _ZN11__sanitizer10timeval_szE _ZN11__sanitizer18struct_cdrom_ti_szE _ZN11__sanitizer18struct_copr_msg_szE _ZN11__sanitizer24struct_copr_debug_buf_szE _ZN11__sanitizer19struct_cdrom_msf_szE _ZN11__sanitizer20struct_vt_consize_szE _ZN11__sanitizer17struct_winsize_szE _ZN11__sanitizer26struct_floppy_fdc_state_szE _ZN11__sanitizer16struct_rusage_szE _ZN11__sanitizer17struct_vt_mode_szE _ZN11__sanitizer24struct_floppy_raw_cmd_szE _ZN11__sanitizer20struct_hd_driveid_szE _ZN11__sanitizer18struct_input_id_szE _ZN11__sanitizer23struct_seq_event_rec_szE _ZN11__sanitizer20struct_itimerspec_szE _ZN11__sanitizer18struct_timespec_szE _ZN11__sanitizer17current_verbosityE _ZN14__interception11real_strcpyE _ZN14__interception12real_strncpyE _ZN14__interception11real_memcpyE _ZN14__interception16real_sem_destroyE _ZN6__asan28asan_flags_dont_use_directlyE _ZN14__interception32real_pthread_attr_getschedpolicyE _ZN14__interception10real_indexE _ZN14__interception14real_strtoumaxE _ZN14__interception14real_strtoimaxE _ZN11__sanitizer8path_maxE _ZN14__interception16real___cxa_throwE _ZN14__interception22real_process_vm_writevE _ZN14__interception11real_writevE _ZN14__interception21real_process_vm_readvE _ZN14__interception10real_readvE _ZN14__interception9real_recvE _ZN14__interception13real_sem_postE _ZN14__interception15real_getsockoptE _ZN14__interception11real_acceptE _ZN14__interception13real_getutentE _ZN14__interception15real_gethostentE _ZL13write_hostentPvPN11__sanitizer19__sanitizer_hostentE _ZN14__interception14real_getmntentE _ZN14__interception17real___cxa_atexitE _ZN14__interception10real__exitE _ZN14__interception13real_sem_initE _ZN14__interception16real_sem_trywaitE _ZN14__interception12real_sigwaitE _ZN14__interception18real_sem_timedwaitE _ZN14__interception13real_sem_waitE _ZN14__interception9real_waitE _ZN14__interception11real_memsetE _ZN11__sanitizer7af_inetE _ZN14__interception9real_statE _ZN14__interception11real_wcscatE _ZN14__interception11real_strcatE _ZN14__interception12real_wcsncatE _ZN14__interception12real_strncatE _ZN6__asan10AsanThread11ThreadStartEmPN11__sanitizer16atomic_uintptr_tE _ZN14__interception15real_initgroupsE _ZN14__interception14real_getgroupsE _ZN14__interception11real_sincosE _ZN14__interception17real_clock_getresE _ZN14__interception10real_timesE _ZN14__interception13real_mbstowcsE _ZN14__interception14real_mbsrtowcsE _ZN14__interception13real_wcstombsE _ZN14__interception14real_wcsrtombsE _ZN14__interception15real_llistxattrE _ZN14__interception15real_flistxattrE _ZN14__interception14real_listxattrE _ZN14__interception14real_lgetxattrE _ZN14__interception14real_fgetxattrE _ZN14__interception13real_getxattrE _ZN14__interception11real_strstrE _ZN14__interception15real_strcasestrE _ZN14__interception13real_strerrorE _ZN14__interception12real_opendirE _ZN14__interception12real_readdirE _ZN14__interception11real_strchrE _ZN14__interception12real_strrchrE _ZN14__interception12real_memrchrE _ZN14__interception11real_memchrE _ZN14__interception14real_setitimerE _ZN14__interception14real_getitimerE _ZN14__interception18real_gethostbyaddrE _ZN14__interception15real_strerror_rE _ZN14__interception14real_readdir_rE _ZN14__interception14real_lgammaf_rE _ZN14__interception13real_gmtime_rE _ZN14__interception16real_localtime_rE _ZN14__interception14real_asctime_rE _ZN14__interception12real_ctime_rE _ZN14__interception14real_ttyname_rE _ZN14__interception20real_gethostbyname_rE _ZN14__interception13real_lgamma_rE _ZN14__interception10real_frexpE _Z/*
 * Copyright (C) 2014-2015 Toradex AG
 * Author: Stefan Agner <stefan@agner.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * IRQ chip driver for MSCM interrupt router available on Vybrid SoC's.
 * The interrupt router is between the CPU's interrupt controller and the
 * peripheral. The router allows to route the peripheral interrupts to
 * one of the two available CPU's on Vybrid VF6xx SoC's (Cortex-A5 or
 * Cortex-M4). The router will be configured transparently on a IRQ
 * request.
 *
 * o All peripheral interrupts of the Vybrid SoC can be routed to
 *   CPU 0, CPU 1 or both. The routing is useful for dual-core
 *   variants of Vybrid SoC such as VF6xx. This driver routes the
 *   requested interrupt to the CPU currently running on.
 *
 * o It is required to setup the interrupt router even on single-core
 *   variants of Vybrid.
 */

#include <linux/cpu_pm.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/mfd/syscon.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/regmap.h>

#define MSCM_CPxNUM		0x4

#define MSCM_IRSPRC(n)		(0x80 + 2 * (n))
#define MSCM_IRSPRC_CPEN_MASK	0x3

#define MSCM_IRSPRC_NUM		112

struct vf610_mscm_ir_chip_data {
	void __iomem *mscm_ir_base;
	u16 cpu_mask;
	u16 saved_irsprc[MSCM_IRSPRC_NUM];
	bool is_nvic;
};

static struct vf610_mscm_ir_chip_data *mscm_ir_data;

static inline void vf610_mscm_ir_save(struct vf610_mscm_ir_chip_data *data)
{
	int i;

	for (i = 0; i < MSCM_IRSPRC_NUM; i++)
		data->saved_irsprc[i] = readw_relaxed(data->mscm_ir_base + MSCM_IRSPRC(i));
}

static inline void vf610_mscm_ir_restore(struct vf610_mscm_ir_chip_data *data)
{
	int i;

	for (i = 0; i < MSCM_IRSPRC_NUM; i++)
		writew_relaxed(data->saved_irsprc[i], data->mscm_ir_base + MSCM_IRSPRC(i));
}

static int vf610_mscm_ir_notifier(struct notifier_block *self,
				  unsigned long cmd, void *v)
{
	switch (cmd) {
	case CPU_CLUSTER_PM_ENTER:
		vf610_mscm_ir_save(mscm_ir_data);
		break;
	case CPU_CLUSTER_PM_ENTER_FAILED:
	case CPU_CLUSTER_PM_EXIT:
		vf610_mscm_ir_restore(mscm_ir_data);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block mscm_ir_notifier_block = {
	.notifier_call = vf610_mscm_ir_notifier,
};

static void vf610_mscm_ir_enable(struct irq_data *data)
{
	irq_hw_number_t hwirq = data->hwirq;
	struct vf610_mscm_ir_chip_data *chip_data = data->chip_data;
	u16 irsprc;

	irsprc = readw_relaxed(chip_data->mscm_ir_base + MSCM_IRSPRC(hwirq));
	irsprc &= MSCM_IRSPRC_CPEN_MASK;

	WARN_ON(irsprc & ~chip_data->cpu_mask);

	writew_relaxed(chip_data->cpu_mask,
		       chip_data->mscm_ir_base + MSCM_IRSPRC(hwirq));

	irq_chip_enable_parent(data);
}

static void vf610_mscm_ir_disable(struct irq_data *data)
{
	irq_hw_number_t hwirq = data->hwirq;
	struct vf610_mscm_ir_chip_data *chip_data = data->chip_data;

	writew_relaxed(0x0, chip_data->mscm_ir_base + MSCM_IRSPRC(hwirq));

	irq_chip_disable_parent(data);
}

static struct irq_chip vf610_mscm_ir_irq_chip = {
	.name			= "mscm-ir",
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_enable		= vf610_mscm_ir_enable,
	.irq_disable		= vf610_mscm_ir_disable,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
};

static int vf610_mscm_ir_domain_alloc(struct irq_domain *domain, unsigned int virq,
				      unsigned int nr_irqs, void *arg)
{
	int i;
	irq_hw_number_t hwirq;
	struct irq_fwspec *fwspec = arg;
	struct irq_fwspec parent_fwspec;

	if (!irq_domain_get_of_node(domain->parent))
		return -EINVAL;

	if (fwspec->param_count != 2)
		return -EINVAL;

	hwirq = fwspec->param[0];
	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_hwirq_and_chip(domain, virq + i, hwirq + i,
					      &vf610_mscm_ir_irq_chip,
					      domain->host_data);

	parent_fwspec.fwnode = domain->parent->fwnode;

	if (mscm_ir_data->is_nvic) {
		parent_fwspec.param_count = 1;
		parent_fwspec.param[0] = fwspec->param[0];
	} else {
		parent_fwspec.param_count = 3;
		parent_fwspec.param[0] = GIC_SPI;
		parent_fwspec.param[1] = fwspec->param[0];
		parent_fwspec.param[2] = fwspec->param[1];
	}

	return irq_domain_alloc_irqs_parent(domain, virq, nr_irqs,
					    &parent_fwspec);
}

static int vf610_mscm_ir_domain_translate(struct irq_domain *d,
					  struct irq_fwspec *fwspec,
					  unsigned long *hwirq,
					  unsigned int *type)
{
	if (WARN_ON(fwspec->param_count < 2))
		return -EINVAL;
	*hwirq = fwspec->param[0];
	*type = fwspec->param[1] & IRQ_TYPE_SENSE_MASK;
	return 0;
}

static const struct irq_domain_ops mscm_irq_domain_ops = {
	.translate = vf610_mscm_ir_domain_translate,
	.alloc = vf610_mscm_ir_domain_alloc,
	.free = irq_domain_free_irqs_common,
};

static int __init vf610_mscm_ir_of_init(struct device_node *node,
			       struct device_node *parent)
{
	struct irq_domain *domain, *domain_parent;
	struct regmap *mscm_cp_regmap;
	int ret, cpuid;

	domain_parent = irq_find_host(parent);
	if (!domain_parent) {
		pr_err("vf610_mscm_ir: interrupt-parent not found\n");
		return -EINVAL;
	}

	mscm_ir_data = kzalloc(sizeof(*mscm_ir_data), GFP_KERNEL);
	if (!mscm_ir_data)
		return -ENOMEM;

	mscm_ir_data->mscm_ir_base = of_io_request_and_map(node, 0, "mscm-ir");
	if (IS_ERR(mscm_ir_data->mscm_ir_base)) {
		pr_err("vf610_mscm_ir: unable to map mscm register\n");
		ret = PTR_ERR(mscm_ir_data->mscm_ir_base);
		goto out_free;
	}

	mscm_cp_regmap = syscon_regmap_lookup_by_phandle(node, "fsl,cpucfg");
	if (IS_ERR(mscm_cp_regmap)) {
		ret = PTR_ERR(mscm_cp_regmap);
		pr_err("vf610_mscm_ir: regmap lookup for cpucfg failed\n");
		goto out_unmap;
	}

	regmap_read(mscm_cp_regmap, MSCM_CPxNUM, &cpuid);
	mscm_ir_data->cpu_mask = 0x1 << cpuid;

	domain = irq_domain_add_hierarchy(domain_parent, 0,
					  MSCM_IRSPRC_NUM, node,
					  &mscm_irq_domain_ops, mscm_ir_data);
	if (!domain) {
		ret = -ENOMEM;
		goto out_unmap;
	}

	if (of_device_is_compatible(irq_domain_get_of_node(domain->parent),
				    "arm,armv7m-nvic"))
		mscm_ir_data->is_nvic = true;

	cpu_pm_register_notifier(&mscm_ir_notifier_block);

	return 0;

out_unmap:
	iounmap(mscm_ir_data->mscm_ir_base);
out_free:
	kfree(mscm_ir_data);
	return ret;
}
IRQCHIP_DECLARE(vf610_mscm_ir, "fsl,vf610-mscm-ir", vf610_mscm_ir_of_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 *  linux/arch/arm/common/vic.c
 *
 *  Copyright (C) 1999 - 2003 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/export.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/syscore_ops.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/irqchip/arm-vic.h>

#include <asm/exception.h>
#include <asm/irq.h>

#define VIC_IRQ_STATUS			0x00
#define VIC_FIQ_STATUS			0x04
#define VIC_INT_SELECT			0x0c	/* 1 = FIQ, 0 = IRQ */
#define VIC_INT_SOFT			0x18
#define VIC_INT_SOFT_CLEAR		0x1c
#define VIC_PROTECT			0x20
#define VIC_PL190_VECT_ADDR		0x30	/* PL190 only */
#define VIC_PL190_DEF_VECT_ADDR		0x34	/* PL190 only */

#define VIC_VECT_ADDR0			0x100	/* 0 to 15 (0..31 PL192) */
#define VIC_VECT_CNTL0			0x200	/* 0 to 15 (0..31 PL192) */
#define VIC_ITCR			0x300	/* VIC test control register */

#define VIC_VECT_CNTL_ENABLE		(1 << 5)

#define VIC_PL192_VECT_ADDR		0xF00

/**
 * struct vic_device - VIC PM device
 * @parent_irq: The parent IRQ number of the VIC if cascaded, or 0.
 * @irq: The IRQ number for the base of the VIC.
 * @base: The register base for the VIC.
 * @valid_sources: A bitmask of valid interrupts
 * @resume_sources: A bitmask of interrupts for resume.
 * @resume_irqs: The IRQs enabled for resume.
 * @int_select: Save for VIC_INT_SELECT.
 * @int_enable: Save for VIC_INT_ENABLE.
 * @soft_int: Save for VIC_INT_SOFT.
 * @protect: Save for VIC_PROTECT.
 * @domain: The IRQ domain for the VIC.
 */
struct vic_device {
	void __iomem	*base;
	int		irq;
	u32		valid_sources;
	u32		resume_sources;
	u32		resume_irqs;
	u32		int_select;
	u32		int_enable;
	u32		soft_int;
	u32		protect;
	struct irq_domain *domain;
};

/* we cannot allocate memory when VICs are initially registered */
static struct vic_device vic_devices[CONFIG_ARM_VIC_NR];

static int vic_id;

static void vic_handle_irq(struct pt_regs *regs);

/**
 * vic_init2 - common initialisation code
 * @base: Base of the VIC.
 *
 * Common initialisation code for registration
 * and resume.
*/
static void vic_init2(void __iomem *base)
{
	int i;

	for (i = 0; i < 16; i++) {
		void __iomem *reg = base + VIC_VECT_CNTL0 + (i * 4);
		writel(VIC_VECT_CNTL_ENABLE | i, reg);
	}

	writel(32, base + VIC_PL190_DEF_VECT_ADDR);
}

#ifdef CONFIG_PM
static void resume_one_vic(struct vic_device *vic)
{
	void __iomem *base = vic->base;

	printk(KERN_DEBUG "%s: resuming vic at %p\n", __func__, base);

	/* re-initialise static settings */
	vic_init2(base);

	writel(vic->int_select, base + VIC_INT_SELECT);
	writel(vic->protect, base + VIC_PROTECT);

	/* set the enabled ints and then clear the non-enabled */
	writel(vic->int_enable, base + VIC_INT_ENABLE);
	writel(~vic->int_enable, base + VIC_INT_ENABLE_CLEAR);

	/* and the same for the soft-int register */

	writel(vic->soft_int, base + VIC_INT_SOFT);
	writel(~vic->soft_int, base + VIC_INT_SOFT_CLEAR);
}

static void vic_resume(void)
{
	int id;

	for (id = vic_id - 1; id >= 0; id--)
		resume_one_vic(vic_devices + id);
}

static void suspend_one_vic(struct vic_device *vic)
{
	void __iomem *base = vic->base;

	printk(KERN_DEBUG "%s: suspending vic at %p\n", __func__, base);

	vic->int_select = readl(base + VIC_INT_SELECT);
	vic->int_enable = readl(base + VIC_INT_ENABLE);
	vic->soft_int = readl(base + VIC_INT_SOFT);
	vic->protect = readl(base + VIC_PROTECT);

	/* set the interrupts (if any) that are used for
	 * resuming the system */

	writel(vic->resume_irqs, base + VIC_INT_ENABLE);
	writel(~vic->resume_irqs, base + VIC_INT_ENABLE_CLEAR);
}

static int vic_suspend(void)
{
	int id;

	for (id = 0; id < vic_id; id++)
		suspend_one_vic(vic_devices + id);

	return 0;
}

struct syscore_ops vic_syscore_ops = {
	.suspend	= vic_suspend,
	.resume		= vic_resume,
};

/**
 * vic_pm_init - initicall to register VIC pm
 *
 * This is called via late_initcall() to register
 * the resources for the VICs due to the early
 * nature of the VIC's registration.
*/
static int __init vic_pm_init(void)
{
	if (vic_id > 0)
		register_syscore_ops(&vic_syscore_ops);

	return 0;
}
late_initcall(vic_pm_init);
#endif /* CONFIG_PM */

static struct irq_chip vic_chip;

static int vic_irqdomain_map(struct irq_domain *d, unsigned int irq,
			     irq_hw_number_t hwirq)
{
	struct vic_device *v = d->host_data;

	/* Skip invalid IRQs, only register handlers for the real ones */
	if (!(v->valid_sources & (1 << hwirq)))
		return -EPERM;
	irq_set_chip_and_handler(irq, &vic_chip, handle_level_irq);
	irq_set_chip_data(irq, v->base);
	irq_set_probe(irq);
	return 0;
}

/*
 * Handle each interrupt in a single VIC.  Returns non-zero if we've
 * handled at least one interrupt.  This reads the status register
 * before handling each interrupt, which is necessary given that
 * handle_IRQ may briefly re-enable interrupts for soft IRQ handling.
 */
static int handle_one_vic(struct vic_device *vic, struct pt_regs *regs)
{
	u32 stat, irq;
	int handled = 0;

	while ((stat = readl_relaxed(vic->base + VIC_IRQ_STATUS))) {
		irq = ffs(stat) - 1;
		handle_domain_irq(vic->domain, irq, regs);
		handled = 1;
	}

	return handled;
}

static void vic_handle_irq_cascaded(struct irq_desc *desc)
{
	u32 stat, hwirq;
	struct irq_chip *host_chip = irq_desc_get_chip(desc);
	struct vic_device *vic = irq_desc_get_handler_data(desc);

	chained_irq_enter(host_chip, desc);

	while ((stat = readl_relaxed(vic->base + VIC_IRQ_STATUS))) {
		hwirq = ffs(stat) - 1;
		generic_handle_irq(irq_find_mapping(vic->domain, hwirq));
	}

	chained_irq_exit(host_chip, desc);
}

/*
 * Keep iterating over all registered VIC's until there are no pending
 * interrupts.
 */
static void __exception_irq_entry vic_handle_irq(struct pt_regs *regs)
{
	int i, handled;

	do {
		for (i = 0, handled = 0; i < vic_id; ++i)
			handled |= handle_one_vic(&vic_devices[i], regs);
	} while (handled);
}

static const struct irq_domain_ops vic_irqdomain_ops = {
	.map = vic_irqdomain_map,
	.xlate = irq_domain_xlate_onetwocell,
};

/**
 * vic_register() - Register a VIC.
 * @base: The base address of the VIC.
 * @parent_irq: The parent IRQ if cascaded, else 0.
 * @irq: The base IRQ for the VIC.
 * @valid_sources: bitmask of valid interrupts
 * @resume_sources: bitmask of interrupts allowed for resume sources.
 * @node: The device tree node associated with the VIC.
 *
 * Register the VIC with the system device tree so that it can be notified
 * of suspend and resume requests and ensure that the correct actions are
 * taken to re-instate the settings on resume.
 *
 * This also configures the IRQ domain for the VIC.
 */
static void __init vic_register(void __iomem *base, unsigned int parent_irq,
				unsigned int irq,
				u32 valid_sources, u32 resume_sources,
				struct device_node *node)
{
	struct vic_device *v;
	int i;

	if (vic_id >= ARRAY_SIZE(vic_devices)) {
		printk(KERN_ERR "%s: too few VICs, increase CONFIG_ARM_VIC_NR\n", __func__);
		return;
	}

	v = &vic_devices[vic_id];
	v->base = base;
	v->valid_sources = valid_sources;
	v->resume_sources = resume_sources;
	set_handle_irq(vic_handle_irq);
	vic_id++;

	if (parent_irq) {
		irq_set_chained_handler_and_data(parent_irq,
						 vic_handle_irq_cascaded, v);
	}

	v->domain = irq_domain_add_simple(node, fls(valid_sources), irq,
					  &vic_irqdomain_ops, v);
	/* create an IRQ mapping for each valid IRQ */
	for (i = 0; i < fls(valid_sources); i++)
		if (valid_sources & (1 << i))
			irq_create_mapping(v->domain, i);
	/* If no base IRQ was passed, figure out our allocated base */
	if (irq)
		v->irq = irq;
	else
		v->irq = irq_find_mapping(v->domain, 0);
}

static void vic_ack_irq(struct irq_data *d)
{
	void __iomem *base = irq_data_get_irq_chip_data(d);
	unsigned int irq = d->hwirq;
	writel(1 << irq, base + VIC_INT_ENABLE_CLEAR);
	/* moreover, clear the soft-triggered, in case it was the reason */
	writel(1 << irq, base + VIC_INT_SOFT_CLEAR);
}

static void vic_mask_irq(struct irq_data *d)
{
	void __iomem *base = irq_data_get_irq_chip_data(d);
	unsigned int irq = d->hwirq;
	writel(1 << irq, base + VIC_INT_ENABLE_CLEAR);
}

static void vic_unmask_irq(struct irq_data *d)
{
	void __iomem *base = irq_data_get_irq_chip_data(d);
	unsigned int irq = d->hwirq;
	writel(1 << irq, base + VIC_INT_ENABLE);
}

#if defined(CONFIG_PM)
static struct vic_device *vic_from_irq(unsigned int irq)
{
        struct vic_device *v = vic_devices;
	unsigned int base_irq = irq & ~31;
	int id;

	for (id = 0; id < vic_id; id++, v++) {
		if (v->irq == base_irq)
			return v;
	}

	return NULL;
}

static int vic_set_wake(struct irq_data *d, unsigned int on)
{
	struct vic_device *v = vic_from_irq(d->irq);
	unsigned int off = d->hwirq;
	u32 bit = 1 << off;

	if (!v)
		return -EINVAL;

	if (!(bit & v->resume_sources))
		return -EINVAL;

	if (on)
		v->resume_irqs |= bit;
	else
		v->resume_irqs &= ~bit;

	return 0;
}
#else
#define vic_set_wake NULL
#endif /* CONFIG_PM */

static struct irq_chip vic_chip = {
	.name		= "VIC",
	.irq_ack	= vic_ack_irq,
	.irq_mask	= vic_mask_irq,
	.irq_unmask	= vic_unmask_irq,
	.irq_set_wake	= vic_set_wake,
};

static void __init vic_disable(void __iomem *base)
{
	writel(0, base + VIC_INT_SELECT);
	writel(0, base + VIC_INT_ENABLE);
	writel(~0, base + VIC_INT_ENABLE_CLEAR);
	writel(0, base + VIC_ITCR);
	writel(~0, base + VIC_INT_SOFT_CLEAR);
}

static void __init vic_clear_interrupts(void __iomem *base)
{
	unsigned int i;

	writel(0, base + VIC_PL190_VECT_ADDR);
	for (i = 0; i < 19; i++) {
		unsigned int value;

		value = readl(base + VIC_PL190_VECT_ADDR);
		writel(value, base + VIC_PL190_VECT_ADDR);
	}
}

/*
 * The PL190 cell from ARM has been modified by ST to handle 64 interrupts.
 * The original cell has 32 interrupts, while the modified one has 64,
 * replocating two blocks 0x00..0x1f in 0x20..0x3f. In that case
 * the probe function is called twice, with base set to offset 000
 *  and 020 within the page. We call this "second block".
 */
static void __init vic_init_st(void __iomem *base, unsigned int irq_start,
			       u32 vic_sources, struct device_node *node)
{
	unsigned int i;
	int vic_2nd_block = ((unsigned long)base & ~PAGE_MASK) != 0;

	/* Disable all interrupts initially. */
	vic_disable(base);

	/*
	 * Make sure we clear all existing interrupts. The vector registers
	 * in this cell are after the second block of general registers,
	 * so we can address them using standard offsets, but only from
	 * the second base address, which is 0x20 in the page
	 */
	if (vic_2nd_block) {
		vic_clear_interrupts(base);

		/* ST has 16 vectors as well, but we don't enable them by now */
		for (i = 0; i < 16; i++) {
			void __iomem *reg = base + VIC_VECT_CNTL0 + (i * 4);
			writel(0, reg);
		}

		writel(32, base + VIC_PL190_DEF_VECT_ADDR);
	}

	vic_register(base, 0, irq_start, vic_sources, 0, node);
}

void __init __vic_init(void __iomem *base, int parent_irq, int irq_start,
			      u32 vic_sources, u32 resume_sources,
			      struct device_node *node)
{
	unsigned int i;
	u32 cellid = 0;
	enum amba_vendor vendor;

	/* Identify which VIC cell this one is, by reading the ID */
	for (i = 0; i < 4; i++) {
		void __iomem *addr;
		addr = (void __iomem *)((u32)base & PAGE_MASK) + 0xfe0 + (i * 4);
		cellid |= (readl(addr) & 0xff) << (8 * i);
	}
	vendor = (cellid >> 12) & 0xff;
	printk(KERN_INFO "VIC @%p: id 0x%08x, vendor 0x%02x\n",
	       base, cellid, vendor);

	switch(vendor) {
	case AMBA_VENDOR_ST:
		vic_init_st(base, irq_start, vic_sources, node);
		return;
	default:
		printk(KERN_WARNING "VIC: unknown vendor, continuing anyways\n");
		/* fall through */
	case AMBA_VENDOR_ARM:
		break;
	}

	/* Disable all interrupts initially. */
	vic_disable(base);

	/* Make sure we clear all existing interrupts */
	vic_clear_interrupts(base);

	vic_init2(base);

	vic_register(base, parent_irq, irq_start, vic_sources, resume_sources, node);
}

/**
 * vic_init() - initialise a vectored interrupt controller
 * @base: iomem base address
 * @irq_start: starting interrupt number, must be muliple of 32
 * @vic_sources: bitmask of interrupt sources to allow
 * @resume_sources: bitmask of interrupt sources to allow for resume
 */
void __init vic_init(void __iomem *base, unsigned int irq_start,
		     u32 vic_sources, u32 resume_sources)
{
	__vic_init(base, 0, irq_start, vic_sources, resume_sources, NULL);
}

/**
 * vic_init_cascaded() - initialise a cascaded vectored interrupt controller
 * @base: iomem base address
 * @parent_irq: the parent IRQ we're cascaded off
 * @irq_start: starting interrupt number, must be muliple of 32
 * @vic_sources: bitmask of interrupt sources to allow
 * @resume_sources: bitmask of interrupt sources to allow for resume
 *
 * This returns the base for the new interrupts or negative on error.
 */
int __init vic_init_cascaded(void __iomem *base, unsigned int parent_irq,
			      u32 vic_sources, u32 resume_sources)
{
	struct vic_device *v;

	v = &vic_devices[vic_id];
	__vic_init(base, parent_irq, 0, vic_sources, resume_sources, NULL);
	/* Return out acquired base */
	return v->irq;
}
EXPORT_SYMBOL_GPL(vic_init_cascaded);

#ifdef CONFIG_OF
int __init vic_of_init(struct device_node *node, struct device_node *parent)
{
	void __iomem *regs;
	u32 interrupt_mask = ~0;
	u32 wakeup_mask = ~0;

	if (WARN(parent, "non-root VICs are not supported"))
		return -EINVAL;

	regs = of_iomap(node, 0);
	if (WARN_ON(!regs))
		return -EIO;

	of_property_read_u32(node, "valid-mask", &interrupt_mask);
	of_property_read_u32(node, "valid-wakeup-mask", &wakeup_mask);

	/*
	 * Passing 0 as first IRQ makes the simple domain allocate descriptors
	 */
	__vic_init(regs, 0, 0, interrupt_mask, wakeup_mask, node);

	return 0;
}
IRQCHIP_DECLARE(arm_pl190_vic, "arm,pl190-vic", vic_of_init);
IRQCHIP_DECLARE(arm_pl192_vic, "arm,pl192-vic", vic_of_init);
IRQCHIP_DECLARE(arm_versatile_vic, "arm,versatile-vic", vic_of_init);
#endif /* CONFIG OF */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 *  arch/arm/mach-vt8500/irq.c
 *
 *  Copyright (C) 2012 Tony Prisk <linux@prisktech.co.nz>
 *  Copyright (C) 2010 Alexey Charkov <alchark@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This file is copied and modified from the original irq.c provided by
 * Alexey Charkov. Minor changes have been made for Device Tree Support.
 */

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <asm/irq.h>
#include <asm/exception.h>
#include <asm/mach/irq.h>

#define VT8500_ICPC_IRQ		0x20
#define VT8500_ICPC_FIQ		0x24
#define VT8500_ICDC		0x40		/* Destination Control 64*u32 */
#define VT8500_ICIS		0x80		/* Interrupt status, 16*u32 */

/* ICPC */
#define ICPC_MASK		0x3F
#define ICPC_ROTATE		BIT(6)

/* IC_DCTR */
#define ICDC_IRQ		0x00
#define ICDC_FIQ		0x01
#define ICDC_DSS0		0x02
#define ICDC_DSS1		0x03
#define ICDC_DSS2		0x04
#define ICDC_DSS3		0x05
#define ICDC_DSS4		0x06
#define ICDC_DSS5		0x07

#define VT8500_INT_DISABLE	0
#define VT8500_INT_ENABLE	BIT(3)

#define VT8500_TRIGGER_HIGH	0
#define VT8500_TRIGGER_RISING	BIT(5)
#define VT8500_TRIGGER_FALLING	BIT(6)
#define VT8500_EDGE		( VT8500_TRIGGER_RISING \
				| VT8500_TRIGGER_FALLING)

/* vt8500 has 1 intc, wm8505 and wm8650 have 2 */
#define VT8500_INTC_MAX		2

struct vt8500_irq_data {
	void __iomem 		*base;		/* IO Memory base address */
	struct irq_domain	*domain;	/* Domain for this controller */
};

/* Global variable for accessing io-mem addresses */
static struct vt8500_irq_data intc[VT8500_INTC_MAX];
static u32 active_cnt = 0;

static void vt8500_irq_mask(struct irq_data *d)
{
	struct vt8500_irq_data *priv = d->domain->host_data;
	void __iomem *base = priv->base;
	void __iomem *stat_reg = base + VT8500_ICIS + (d->hwirq < 32 ? 0 : 4);
	u8 edge, dctr;
	u32 status;

	edge = readb(base + VT8500_ICDC + d->hwirq) & VT8500_EDGE;
	if (edge) {
		status = readl(stat_reg);

		status |= (1 << (d->hwirq & 0x1f));
		writel(status, stat_reg);
	} else {
		dctr = readb(base + VT8500_ICDC + d->hwirq);
		dctr &= ~VT8500_INT_ENABLE;
		writeb(dctr, base + VT8500_ICDC + d->hwirq);
	}
}

static void vt8500_irq_unmask(struct irq_data *d)
{
	struct vt8500_irq_data *priv = d->domain->host_data;
	void __iomem *base = priv->base;
	u8 dctr;

	dctr = readb(base + VT8500_ICDC + d->hwirq);
	dctr |= VT8500_INT_ENABLE;
	writeb(dctr, base + VT8500_ICDC + d->hwirq);
}

static int vt8500_irq_set_type(struct irq_data *d, unsigned int flow_type)
{
	struct vt8500_irq_data *priv = d->domain->host_data;
	void __iomem *base = priv->base;
	u8 dctr;

	dctr = readb(base + VT8500_ICDC + d->hwirq);
	dctr &= ~VT8500_EDGE;

	switch (flow_type) {
	case IRQF_TRIGGER_LOW:
		return -EINVAL;
	case IRQF_TRIGGER_HIGH:
		dctr |= VT8500_TRIGGER_HIGH;
		irq_set_handler_locked(d, handle_level_irq);
		break;
	case IRQF_TRIGGER_FALLING:
		dctr |= VT8500_TRIGGER_FALLING;
		irq_set_handler_locked(d, handle_edge_irq);
		break;
	case IRQF_TRIGGER_RISING:
		dctr |= VT8500_TRIGGER_RISING;
		irq_set_handler_locked(d, handle_edge_irq);
		break;
	}
	writeb(dctr, base + VT8500_ICDC + d->hwirq);

	return 0;
}

static struct irq_chip vt8500_irq_chip = {
	.name = "vt8500",
	.irq_ack = vt8500_irq_mask,
	.irq_mask = vt8500_irq_mask,
	.irq_unmask = vt8500_irq_unmask,
	.irq_set_type = vt8500_irq_set_type,
};

static void __init vt8500_init_irq_hw(void __iomem *base)
{
	u32 i;

	/* Enable rotating priority for IRQ */
	writel(ICPC_ROTATE, base + VT8500_ICPC_IRQ);
	writel(0x00, base + VT8500_ICPC_FIQ);

	/* Disable all interrupts and route them to IRQ */
	for (i = 0; i < 64; i++)
		writeb(VT8500_INT_DISABLE | ICDC_IRQ, base + VT8500_ICDC + i);
}

static int vt8500_irq_map(struct irq_domain *h, unsigned int virq,
							irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq, &vt8500_irq_chip, handle_level_irq);

	return 0;
}

static const struct irq_domain_ops vt8500_irq_domain_ops = {
	.map = vt8500_irq_map,
	.xlate = irq_domain_xlate_onecell,
};

static void __exception_irq_entry vt8500_handle_irq(struct pt_regs *regs)
{
	u32 stat, i;
	int irqnr;
	void __iomem *base;

	/* Loop through each active controller */
	for (i=0; i<active_cnt; i++) {
		base = intc[i].base;
		irqnr = readl_relaxed(base) & 0x3F;
		/*
		  Highest Priority register default = 63, so check that this
		  is a real interrupt by checking the status register
		*/
		if (irqnr == 63) {
			stat = readl_relaxed(base + VT8500_ICIS + 4);
			if (!(stat & BIT(31)))
				continue;
		}

		handle_domain_irq(intc[i].domain, irqnr, regs);
	}
}

static int __init vt8500_irq_init(struct device_node *node,
				  struct device_node *parent)
{
	int irq, i;
	struct device_node *np = node;

	if (active_cnt == VT8500_INTC_MAX) {
		pr_err("%s: Interrupt controllers > VT8500_INTC_MAX\n",
								__func__);
		goto out;
	}

	intc[active_cnt].base = of_iomap(np, 0);
	intc[active_cnt].domain = irq_domain_add_linear(node, 64,
			&vt8500_irq_domain_ops,	&intc[active_cnt]);

	if (!intc[active_cnt].base) {
		pr_err("%s: Unable to map IO memory\n", __func__);
		goto out;
	}

	if (!intc[active_cnt].domain) {
		pr_err("%s: Unable to add irq domain!\n", __func__);
		goto out;
	}

	set_handle_irq(vt8500_handle_irq);

	vt8500_init_irq_hw(intc[active_cnt].base);

	pr_info("vt8500-irq: Added interrupt controller\n");

	active_cnt++;

	/* check if this is a slaved controller */
	if (of_irq_count(np) != 0) {
		/* check that we have the correct number of interrupts */
		if (of_irq_count(np) != 8) {
			pr_err("%s: Incorrect IRQ map for slaved controller\n",
					__func__);
			return -EINVAL;
		}

		for (i = 0; i < 8; i++) {
			irq = irq_of_parse_and_map(np, i);
			enable_irq(irq);
		}

		pr_info("vt8500-irq: Enabled slave->parent interrupts\n");
	}
out:
	return 0;
}

IRQCHIP_DECLARE(vt8500_irq, "via,vt8500-intc", vt8500_irq_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
 * Xtensa MX interrupt distributor
 *
 * Copyright (C) 2002 - 2013 Tensilica, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/of.h>

#include <asm/mxregs.h>

#define HW_IRQ_IPI_COUNT 2
#define HW_IRQ_MX_BASE 2
#define HW_IRQ_EXTERN_BASE 3

static DEFINE_PER_CPU(unsigned int, cached_irq_mask);

static int xtensa_mx_irq_map(struct irq_domain *d, unsigned int irq,
		irq_hw_number_t hw)
{
	if (hw < HW_IRQ_IPI_COUNT) {
		struct irq_chip *irq_chip = d->host_data;
		irq_set_chip_and_handler_name(irq, irq_chip,
				handle_percpu_irq, "ipi");
		irq_set_status_flags(irq, IRQ_LEVEL);
		return 0;
	}
	return xtensa_irq_map(d, irq, hw);
}

/*
 * Device Tree IRQ specifier translation function which works with one or
 * two cell bindings. First cell value maps directly to the hwirq number.
 * Second cell if present specifies whether hwirq number is external (1) or
 * internal (0).
 */
static int xtensa_mx_irq_domain_xlate(struct irq_domain *d,
		struct device_node *ctrlr,
		const u32 *intspec, unsigned int intsize,
		unsigned long *out_hwirq, unsigned int *out_type)
{
	return xtensa_irq_domain_xlate(intspec, intsize,
			intspec[0], intspec[0] + HW_IRQ_EXTERN_BASE,
			out_hwirq, out_type);
}

static const struct irq_domain_ops xtensa_mx_irq_domain_ops = {
	.xlate = xtensa_mx_irq_domain_xlate,
	.map = xtensa_mx_irq_map,
};

void secondary_init_irq(void)
{
	__this_cpu_write(cached_irq_mask,
			XCHAL_INTTYPE_MASK_EXTERN_EDGE |
			XCHAL_INTTYPE_MASK_EXTERN_LEVEL);
	set_sr(XCHAL_INTTYPE_MASK_EXTERN_EDGE |
			XCHAL_INTTYPE_MASK_EXTERN_LEVEL, intenable);
}

static void xtensa_mx_irq_mask(struct irq_data *d)
{
	unsigned int mask = 1u << d->hwirq;

	if (mask & (XCHAL_INTTYPE_MASK_EXTERN_EDGE |
				XCHAL_INTTYPE_MASK_EXTERN_LEVEL)) {
		set_er(1u << (xtensa_get_ext_irq_no(d->hwirq) -
					HW_IRQ_MX_BASE), MIENG);
	} else {
		mask = __this_cpu_read(cached_irq_mask) & ~mask;
		__this_cpu_write(cached_irq_mask, mask);
		set_sr(mask, intenable);
	}
}

static void xtensa_mx_irq_unmask(struct irq_data *d)
{
	unsigned int mask = 1u << d->hwirq;

	if (mask & (XCHAL_INTTYPE_MASK_EXTERN_EDGE |
				XCHAL_INTTYPE_MASK_EXTERN_LEVEL)) {
		set_er(1u << (xtensa_get_ext_irq_no(d->hwirq) -
					HW_IRQ_MX_BASE), MIENGSET);
	} else {
		mask |= __this_cpu_read(cached_irq_mask);
		__this_cpu_write(cached_irq_mask, mask);
		set_sr(mask, intenable);
	}
}

static void xtensa_mx_irq_enable(struct irq_data *d)
{
	variant_irq_enable(d->hwirq);
	xtensa_mx_irq_unmask(d);
}

static void xtensa_mx_irq_disable(struct irq_data *d)
{
	xtensa_mx_irq_mask(d);
	variant_irq_disable(d->hwirq);
}

static void xtensa_mx_irq_ack(struct irq_data *d)
{
	set_sr(1 << d->hwirq, intclear);
}

static int xtensa_mx_irq_retrigger(struct irq_data *d)
{
	set_sr(1 << d->hwirq, intset);
	return 1;
}

static int xtensa_mx_irq_set_affinity(struct irq_data *d,
		const struct cpumask *dest, bool force)
{
	unsigned mask = 1u << cpumask_any_and(dest, cpu_online_mask);

	set_er(mask, MIROUT(d->hwirq - HW_IRQ_MX_BASE));
	return 0;

}

static struct irq_chip xtensa_mx_irq_chip = {
	.name		= "xtensa-mx",
	.irq_enable	= xtensa_mx_irq_enable,
	.irq_disable	= xtensa_mx_irq_disable,
	.irq_mask	= xtensa_mx_irq_mask,
	.irq_unmask	= xtensa_mx_irq_unmask,
	.irq_ack	= xtensa_mx_irq_ack,
	.irq_retrigger	= xtensa_mx_irq_retrigger,
	.irq_set_affinity = xtensa_mx_irq_set_affinity,
};

static void __init xtensa_mx_init_common(struct irq_domain *root_domain)
{
	unsigned int i;

	irq_set_default_host(root_domain);
	secondary_init_irq();

	/* Initialize default IRQ routing to CPU 0 */
	for (i = 0; i < XCHAL_NUM_EXTINTERRUPTS; ++i)
		set_er(1, MIROUT(i));
}

int __init xtensa_mx_init_legacy(struct device_node *interrupt_parent)
{
	struct irq_domain *root_domain =
		irq_domain_add_legacy(NULL, NR_IRQS - 1, 1, 0,
				&xtensa_mx_irq_domain_ops,
				&xtensa_mx_irq_chip);
	xtensa_mx_init_common(root_domain);
	return 0;
}

static int __init xtensa_mx_init(struct device_node *np,
		struct device_node *interrupt_parent)
{
	struct irq_domain *root_domain =
		irq_domain_add_linear(np, NR_IRQS, &xtensa_mx_irq_domain_ops,
				&xtensa_mx_irq_chip);
	xtensa_mx_init_common(root_domain);
	return 0;
}
IRQCHIP_DECLARE(xtensa_mx_irq_chip, "cdns,xtensa-mx", xtensa_mx_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * Xtensa built-in interrupt controller
 *
 * Copyright (C) 2002 - 2013 Tensilica, Inc.
 * Copyright (C) 1992, 1998 Linus Torvalds, Ingo Molnar
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Chris Zankel <chris@zankel.net>
 * Kevin Chea
 */

#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/of.h>

unsigned int cached_irq_mask;

/*
 * Device Tree IRQ specifier translation function which works with one or
 * two cell bindings. First cell value maps directly to the hwirq number.
 * Second cell if present specifies whether hwirq number is external (1) or
 * internal (0).
 */
static int xtensa_pic_irq_domain_xlate(struct irq_domain *d,
		struct device_node *ctrlr,
		const u32 *intspec, unsigned int intsize,
		unsigned long *out_hwirq, unsigned int *out_type)
{
	return xtensa_irq_domain_xlate(intspec, intsize,
			intspec[0], intspec[0],
			out_hwirq, out_type);
}

static const struct irq_domain_ops xtensa_irq_domain_ops = {
	.xlate = xtensa_pic_irq_domain_xlate,
	.map = xtensa_irq_map,
};

static void xtensa_irq_mask(struct irq_data *d)
{
	cached_irq_mask &= ~(1 << d->hwirq);
	set_sr(cached_irq_mask, intenable);
}

static void xtensa_irq_unmask(struct irq_data *d)
{
	cached_irq_mask |= 1 << d->hwirq;
	set_sr(cached_irq_mask, intenable);
}

static void xtensa_irq_enable(struct irq_data *d)
{
	variant_irq_enable(d->hwirq);
	xtensa_irq_unmask(d);
}

static void xtensa_irq_disable(struct irq_data *d)
{
	xtensa_irq_mask(d);
	variant_irq_disable(d->hwirq);
}

static void xtensa_irq_ack(struct irq_data *d)
{
	set_sr(1 << d->hwirq, intclear);
}

static int xtensa_irq_retrigger(struct irq_data *d)
{
	set_sr(1 << d->hwirq, intset);
	return 1;
}

static struct irq_chip xtensa_irq_chip = {
	.name		= "xtensa",
	.irq_enable	= xtensa_irq_enable,
	.irq_disable	= xtensa_irq_disable,
	.irq_mask	= xtensa_irq_mask,
	.irq_unmask	= xtensa_irq_unmask,
	.irq_ack	= xtensa_irq_ack,
	.irq_retrigger	= xtensa_irq_retrigger,
};

int __init xtensa_pic_init_legacy(struct device_node *interrupt_parent)
{
	struct irq_domain *root_domain =
		irq_domain_add_legacy(NULL, NR_IRQS - 1, 1, 0,
				&xtensa_irq_domain_ops, &xtensa_irq_chip);
	irq_set_default_host(root_domain);
	return 0;
}

static int __init xtensa_pic_init(struct device_node *np,
		struct device_node *interrupt_parent)
{
	struct irq_domain *root_domain =
		irq_domain_add_linear(np, NR_IRQS, &xtensa_irq_domain_ops,
				&xtensa_irq_chip);
	irq_set_default_host(root_domain);
	return 0;
}
IRQCHIP_DECLARE(xtensa_irq_chip, "cdns,xtensa-pic", xtensa_pic_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 *  linux/drivers/irqchip/irq-zevio.c
 *
 *  Copyright (C) 2013 Daniel Tang <tangrs@tangrs.id.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */

#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <asm/mach/irq.h>
#include <asm/exception.h>

#define IO_STATUS	0x000
#define IO_RAW_STATUS	0x004
#define IO_ENABLE	0x008
#define IO_DISABLE	0x00C
#define IO_CURRENT	0x020
#define IO_RESET	0x028
#define IO_MAX_PRIOTY	0x02C

#define IO_IRQ_BASE	0x000
#define IO_FIQ_BASE	0x100

#define IO_INVERT_SEL	0x200
#define IO_STICKY_SEL	0x204
#define IO_PRIORITY_SEL	0x300

#define MAX_INTRS	32
#define FIQ_START	MAX_INTRS

static struct irq_domain *zevio_irq_domain;
static void __iomem *zevio_irq_io;

static void zevio_irq_ack(struct irq_data *irqd)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(irqd);
	struct irq_chip_regs *regs =
		&container_of(irqd->chip, struct irq_chip_type, chip)->regs;

	readl(gc->reg_base + regs->ack);
}

static void __exception_irq_entry zevio_handle_irq(struct pt_regs *regs)
{
	int irqnr;

	while (readl(zevio_irq_io + IO_STATUS)) {
		irqnr = readl(zevio_irq_io + IO_CURRENT);
		handle_domain_irq(zevio_irq_domain, irqnr, regs);
	};
}

static void __init zevio_init_irq_base(void __iomem *base)
{
	/* Disable all interrupts */
	writel(~0, base + IO_DISABLE);

	/* Accept interrupts of all priorities */
	writel(0xF, base + IO_MAX_PRIOTY);

	/* Reset existing interrupts */
	readl(base + IO_RESET);
}

static int __init zevio_of_init(struct device_node *node,
				struct device_node *parent)
{
	unsigned int clr = IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;
	struct irq_chip_generic *gc;
	int ret;

	if (WARN_ON(zevio_irq_io || zevio_irq_domain))
		return -EBUSY;

	zevio_irq_io = of_iomap(node, 0);
	BUG_ON(!zevio_irq_io);

	/* Do not invert interrupt status bits */
	writel(~0, zevio_irq_io + IO_INVERT_SEL);

	/* Disable sticky interrupts */
	writel(0, zevio_irq_io + IO_STICKY_SEL);

	/* We don't use IRQ priorities. Set each IRQ to highest priority. */
	memset_io(zevio_irq_io + IO_PRIORITY_SEL, 0, MAX_INTRS * sizeof(u32));

	/* Init IRQ and FIQ */
	zevio_init_irq_base(zevio_irq_io + IO_IRQ_BASE);
	zevio_init_irq_base(zevio_irq_io + IO_FIQ_BASE);

	zevio_irq_domain = irq_domain_add_linear(node, MAX_INTRS,
						 &irq_generic_chip_ops, NULL);
	BUG_ON(!zevio_irq_domain);

	ret = irq_alloc_domain_generic_chips(zevio_irq_domain, MAX_INTRS, 1,
					     "zevio_intc", handle_level_irq,
					     clr, 0, IRQ_GC_INIT_MASK_CACHE);
	BUG_ON(ret);

	gc = irq_get_domain_generic_chip(zevio_irq_domain, 0);
	gc->reg_base				= zevio_irq_io;
	gc->chip_types[0].chip.irq_ack		= zevio_irq_ack;
	gc->chip_types[0].chip.irq_mask		= irq_gc_mask_disable_reg;
	gc->chip_types[0].chip.irq_unmask	= irq_gc_unmask_enable_reg;
	gc->chip_types[0].regs.mask		= IO_IRQ_BASE + IO_ENABLE;
	gc->chip_types[0].regs.enable		= IO_IRQ_BASE + IO_ENABLE;
	gc->chip_types[0].regs.disable		= IO_IRQ_BASE + IO_DISABLE;
	gc->chip_types[0].regs.ack		= IO_IRQ_BASE + IO_RESET;

	set_handle_irq(zevio_handle_irq);

	pr_info("TI-NSPIRE classic IRQ controller\n");
	return 0;
}

IRQCHIP_DECLARE(zevio_irq, "lsi,zevio-intc", zevio_of_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright (C) 2012 Thomas Petazzoni
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>

/*
 * This special of_device_id is the sentinel at the end of the
 * of_device_id[] array of all irqchips. It is automatically placed at
 * the end of the array by the linker, thanks to being part of a
 * special section.
 */
static const struct of_device_id
irqchip_of_match_end __used __section(__irqchip_of_table_end);

extern struct of_device_id __irqchip_of_table[];

void __init irqchip_init(void)
{
	of_irq_init(__irqchip_of_table);
	acpi_probe_device_table(irqchip);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * SPEAr platform shared irq layer source file
 *
 * Copyright (C) 2009-2012 ST Microelectronics
 * Viresh Kumar <vireshk@kernel.org>
 *
 * Copyright (C) 2012 ST Microelectronics
 * Shiraz Hashim <shiraz.linux.kernel@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/err.h>
#include <linux/export.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>

/*
 * struct spear_shirq: shared irq structure
 *
 * base:	Base register address
 * status_reg:	Status register offset for chained interrupt handler
 * mask_reg:	Mask register offset for irq chip
 * mask:	Mask to apply to the status register
 * virq_base:	Base virtual interrupt number
 * nr_irqs:	Number of interrupts handled by this block
 * offset:	Bit offset of the first interrupt
 * irq_chip:	Interrupt controller chip used for this instance,
 *		if NULL group is disabled, but accounted
 */
struct spear_shirq {
	void __iomem		*base;
	u32			status_reg;
	u32			mask_reg;
	u32			mask;
	u32			virq_base;
	u32			nr_irqs;
	u32			offset;
	struct irq_chip		*irq_chip;
};

/* spear300 shared irq registers offsets and masks */
#define SPEAR300_INT_ENB_MASK_REG	0x54
#define SPEAR300_INT_STS_MASK_REG	0x58

static DEFINE_RAW_SPINLOCK(shirq_lock);

static void shirq_irq_mask(struct irq_data *d)
{
	struct spear_shirq *shirq = irq_data_get_irq_chip_data(d);
	u32 val, shift = d->irq - shirq->virq_base + shirq->offset;
	u32 __iomem *reg = shirq->base + shirq->mask_reg;

	raw_spin_lock(&shirq_lock);
	val = readl(reg) & ~(0x1 << shift);
	writel(val, reg);
	raw_spin_unlock(&shirq_lock);
}

static void shirq_irq_unmask(struct irq_data *d)
{
	struct spear_shirq *shirq = irq_data_get_irq_chip_data(d);
	u32 val, shift = d->irq - shirq->virq_base + shirq->offset;
	u32 __iomem *reg = shirq->base + shirq->mask_reg;

	raw_spin_lock(&shirq_lock);
	val = readl(reg) | (0x1 << shift);
	writel(val, reg);
	raw_spin_unlock(&shirq_lock);
}

static struct irq_chip shirq_chip = {
	.name		= "spear-shirq",
	.irq_mask	= shirq_irq_mask,
	.irq_unmask	= shirq_irq_unmask,
};

static struct spear_shirq spear300_shirq_ras1 = {
	.offset		= 0,
	.nr_irqs	= 9,
	.mask		= ((0x1 << 9) - 1) << 0,
	.irq_chip	= &shirq_chip,
	.status_reg	= SPEAR300_INT_STS_MASK_REG,
	.mask_reg	= SPEAR300_INT_ENB_MASK_REG,
};

static struct spear_shirq *spear300_shirq_blocks[] = {
	&spear300_shirq_ras1,
};

/* spear310 shared irq registers offsets and masks */
#define SPEAR310_INT_STS_MASK_REG	0x04

static struct spear_shirq spear310_shirq_ras1 = {
	.offset		= 0,
	.nr_irqs	= 8,
	.mask		= ((0x1 << 8) - 1) << 0,
	.irq_chip	= &dummy_irq_chip,
	.status_reg	= SPEAR310_INT_STS_MASK_REG,
};

static struct spear_shirq spear310_shirq_ras2 = {
	.offset		= 8,
	.nr_irqs	= 5,
	.mask		= ((0x1 << 5) - 1) << 8,
	.irq_chip	= &dummy_irq_chip,
	.status_reg	= SPEAR310_INT_STS_MASK_REG,
};

static struct spear_shirq spear310_shirq_ras3 = {
	.offset		= 13,
	.nr_irqs	= 1,
	.mask		= ((0x1 << 1) - 1) << 13,
	.irq_chip	= &dummy_irq_chip,
	.status_reg	= SPEAR310_INT_STS_MASK_REG,
};

static struct spear_shirq spear310_shirq_intrcomm_ras = {
	.offset		= 14,
	.nr_irqs	= 3,
	.mask		= ((0x1 << 3) - 1) << 14,
	.irq_chip	= &dummy_irq_chip,
	.status_reg	= SPEAR310_INT_STS_MASK_REG,
};

static struct spear_shirq *spear310_shirq_blocks[] = {
	&spear310_shirq_ras1,
	&spear310_shirq_ras2,
	&spear310_shirq_ras3,
	&spear310_shirq_intrcomm_ras,
};

/* spear320 shared irq registers offsets and masks */
#define SPEAR320_INT_STS_MASK_REG		0x04
#define SPEAR320_INT_CLR_MASK_REG		0x04
#define SPEAR320_INT_ENB_MASK_REG		0x08

static struct spear_shirq spear320_shirq_ras3 = {
	.offset		= 0,
	.nr_irqs	= 7,
	.mask		= ((0x1 << 7) - 1) << 0,
};

static struct spear_shirq spear320_shirq_ras1 = {
	.offset		= 7,
	.nr_irqs	= 3,
	.mask		= ((0x1 << 3) - 1) << 7,
	.irq_chip	= &dummy_irq_chip,
	.status_reg	= SPEAR320_INT_STS_MASK_REG,
};

static struct spear_shirq spear320_shirq_ras2 = {
	.offset		= 10,
	.nr_irqs	= 1,
	.mask		= ((0x1 << 1) - 1) << 10,
	.irq_chip	= &dummy_irq_chip,
	.status_reg	= SPEAR320_INT_STS_MASK_REG,
};

static struct spear_shirq spear320_shirq_intrcomm_ras = {
	.offset		= 11,
	.nr_irqs	= 11,
	.mask		= ((0x1 << 11) - 1) << 11,
	.irq_chip	= &dummy_irq_chip,
	.status_reg	= SPEAR320_INT_STS_MASK_REG,
};

static struct spear_shirq *spear320_shirq_blocks[] = {
	&spear320_shirq_ras3,
	&spear320_shirq_ras1,
	&spear320_shirq_ras2,
	&spear320_shirq_intrcomm_ras,
};

static void shirq_handler(struct irq_desc *desc)
{
	struct spear_shirq *shirq = irq_desc_get_handler_data(desc);
	u32 pend;

	pend = readl(shirq->base + shirq->status_reg) & shirq->mask;
	pend >>= shirq->offset;

	while (pend) {
		int irq = __ffs(pend);

		pend &= ~(0x1 << irq);
		generic_handle_irq(shirq->virq_base + irq);
	}
}

static void __init spear_shirq_register(struct spear_shirq *shirq,
					int parent_irq)
{
	int i;

	if (!shirq->irq_chip)
		return;

	irq_set_chained_handler_and_data(parent_irq, shirq_handler, shirq);

	for (i = 0; i < shirq->nr_irqs; i++) {
		irq_set_chip_and_handler(shirq->virq_base + i,
					 shirq->irq_chip, handle_simple_irq);
		irq_set_chip_data(shirq->virq_base + i, shirq);
	}
}

static int __init shirq_init(struct spear_shirq **shirq_blocks, int block_nr,
		struct device_node *np)
{
	int i, parent_irq, virq_base, hwirq = 0, nr_irqs = 0;
	struct irq_domain *shirq_domain;
	void __iomem *base;

	base = of_iomap(np, 0);
	if (!base) {
		pr_err("%s: failed to map shirq registers\n", __func__);
		return -ENXIO;
	}

	for (i = 0; i < block_nr; i++)
		nr_irqs += shirq_blocks[i]->nr_irqs;

	virq_base = irq_alloc_descs(-1, 0, nr_irqs, 0);
	if (IS_ERR_VALUE(virq_base)) {
		pr_err("%s: irq desc alloc failed\n", __func__);
		goto err_unmap;
	}

	shirq_domain = irq_domain_add_legacy(np, nr_irqs, virq_base, 0,
			&irq_domain_simple_ops, NULL);
	if (WARN_ON(!shirq_domain)) {
		pr_warn("%s: irq domain init failed\n", __func__);
		goto err_free_desc;
	}

	for (i = 0; i < block_nr; i++) {
		shirq_blocks[i]->base = base;
		shirq_blocks[i]->virq_base = irq_find_mapping(shirq_domain,
				hwirq);

		parent_irq = irq_of_parse_and_map(np, i);
		spear_shirq_register(shirq_blocks[i], parent_irq);
		hwirq += shirq_blocks[i]->nr_irqs;
	}

	return 0;

err_free_desc:
	irq_free_descs(virq_base, nr_irqs);
err_unmap:
	iounmap(base);
	return -ENXIO;
}

static int __init spear300_shirq_of_init(struct device_node *np,
					 struct device_node *parent)
{
	return shirq_init(spear300_shirq_blocks,
			ARRAY_SIZE(spear300_shirq_blocks), np);
}
IRQCHIP_DECLARE(spear300_shirq, "st,spear300-shirq", spear300_shirq_of_init);

static int __init spear310_shirq_of_init(struct device_node *np,
					 struct device_node *parent)
{
	return shirq_init(spear310_shirq_blocks,
			ARRAY_SIZE(spear310_shirq_blocks), np);
}
IRQCHIP_DECLARE(spear310_shirq, "st,spear310-shirq", spear310_shirq_of_init);

static int __init spear320_shirq_of_init(struct device_node *np,
					 struct device_node *parent)
{
	return shirq_init(spear320_shirq_blocks,
			ARRAY_SIZE(spear320_shirq_blocks), np);
}
IRQCHIP_DECLARE(spear320_shirq, "st,spear320-shirq", spear320_shirq_of_init);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #
# ISDN device configuration
#

menuconfig ISDN
	bool "ISDN support"
	depends on NET && NETDEVICES
	depends on !S390 && !UML
	---help---
	  ISDN ("Integrated Services Digital Network", called RNIS in France)
	  is a fully digital telephone service that can be used for voice and
	  data connections.  If your computer is equipped with an ISDN
	  adapter you can use it to connect to your Internet service provider
	  (with SLIP or PPP) faster than via a conventional telephone modem
	  (though still much slower than with DSL) or to make and accept
	  voice calls (eg. turning your PC into a software answering machine
	  or PABX).

	  Select this option if you want your kernel to support ISDN.

if ISDN

menuconfig ISDN_I4L
	tristate "Old ISDN4Linux (deprecated)"
	depends on TTY
	---help---
	  This driver allows you to use an ISDN adapter for networking
	  connections and as dialin/out device.  The isdn-tty's have a built
	  in AT-compatible modem emulator.  Network devices support autodial,
	  channel-bundling, callback and caller-authentication without having
	  a daemon running.  A reduced T.70 protocol is supported with tty's
	  suitable for German BTX.  On D-Channel, the protocols EDSS1
	  (Euro-ISDN) and 1TR6 (German style) are supported.  See
	  <file:Documentation/isdn/README> for more information.

	  ISDN support in the linux kernel is moving towards a new API,
	  called CAPI (Common ISDN Application Programming Interface).
	  Therefore the old ISDN4Linux layer will eventually become obsolete.
	  It is still available, though, for use with adapters that are not
	  supported by the new CAPI subsystem yet.

source "drivers/isdn/i4l/Kconfig"

menuconfig ISDN_CAPI
	tristate "CAPI 2.0 subsystem"
	help
	  This provides CAPI (the Common ISDN Application Programming
	  Interface) Version 2.0, a standard making it easy for programs to
	  access ISDN hardware in a device independent way. (For details see
	  <http://www.capi.org/>.)  CAPI supports making and accepting voice
	  and data connections, controlling call options and protocols,
	  as well as ISDN supplementary services like call forwarding or
	  three-party conferences (if supported by the specific hardware
	  driver).

	  Select this option and the appropriate hardware driver below if
	  you have an ISDN adapter supported by the CAPI subsystem.

if ISDN_CAPI

source "drivers/isdn/capi/Kconfig"

source "drivers/isdn/hardware/Kconfig"

endif # ISDN_CAPI

source "drivers/isdn/gigaset/Kconfig"

source "drivers/isdn/hysdn/Kconfig"

source "drivers/isdn/mISDN/Kconfig"

config ISDN_HDLC
	tristate
	select CRC_CCITT
	select BITREVERSE

endif # ISDN
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                # Makefile for the kernel ISDN subsystem and device drivers.

# Object files in subdirectories

obj-$(CONFIG_ISDN_I4L)			+= i4l/
obj-$(CONFIG_ISDN_CAPI)			+= capi/
obj-$(CONFIG_MISDN)			+= mISDN/
obj-$(CONFIG_ISDN)			+= hardware/
obj-$(CONFIG_ISDN_DIVERSION)		+= divert/
obj-$(CONFIG_ISDN_DRV_HISAX)		+= hisax/
obj-$(CONFIG_ISDN_DRV_ICN)		+= icn/
obj-$(CONFIG_ISDN_DRV_PCBIT)		+= pcbit/
obj-$(CONFIG_ISDN_DRV_SC)		+= sc/
obj-$(CONFIG_ISDN_DRV_LOOP)		+= isdnloop/
obj-$(CONFIG_ISDN_DRV_ACT2000)		+= act2000/
obj-$(CONFIG_HYSDN)			+= hysdn/
obj-$(CONFIG_ISDN_DRV_GIGASET)		+= gigaset/
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         config ISDN_DRV_ACT2000
	tristate "IBM Active 2000 support"
	depends on ISA
	help
	  Say Y here if you have an IBM Active 2000 ISDN card. In order to use
	  this card, additional firmware is necessary, which has to be loaded
	  into the card using a utility which is part of the latest
	  isdn4k-utils package. Please read the file
	  <file:Documentation/isdn/README.act2000> for more information.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  # Makefile for the act2000 ISDN device driver

# Each configuration option enables a list of files.

obj-$(CONFIG_ISDN_DRV_ACT2000)	+= act2000.o

# Multipart objects.

act2000-y			:= module.o capi.o act2000_isa.o
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /* $Id: act2000.h,v 1.8.6.3 2001/09/23 22:24:32 kai Exp $
 *
 * ISDN lowlevel-module for the IBM ISDN-S0 Active 2000.
 *
 * Author       Fritz Elfert
 * Copyright    by Fritz Elfert      <fritz@isdn4linux.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Thanks to Friedemann Baitinger and IBM Germany
 *
 */

#ifndef act2000_h
#define act2000_h

#include <linux/compiler.h>

#define ACT2000_IOCTL_SETPORT    1
#define ACT2000_IOCTL_GETPORT    2
#define ACT2000_IOCTL_SETIRQ     3
#define ACT2000_IOCTL_GETIRQ     4
#define ACT2000_IOCTL_SETBUS     5
#define ACT2000_IOCTL_GETBUS     6
#define ACT2000_IOCTL_SETPROTO   7
#define ACT2000_IOCTL_GETPROTO   8
#define ACT2000_IOCTL_SETMSN     9
#define ACT2000_IOCTL_GETMSN    10
#define ACT2000_IOCTL_LOADBOOT  11
#define ACT2000_IOCTL_ADDCARD   12

#define ACT2000_IOCTL_TEST      98
#define ACT2000_IOCTL_DEBUGVAR  99

#define ACT2000_BUS_ISA          1
#define ACT2000_BUS_MCA          2
#define ACT2000_BUS_PCMCIA       3

/* Struct for adding new cards */
typedef struct act2000_cdef {
	int bus;
	int port;
	int irq;
	char id[10];
} act2000_cdef;

/* Struct for downloading firmware */
typedef struct act2000_ddef {
	int length;             /* Length of code */
	char __user *buffer;    /* Ptr. to code   */
} act2000_ddef;

typedef struct act2000_fwid {
	char isdn[4];
	char revlen[2];
	char revision[504];
} act2000_fwid;

#if defined(__KERNEL__) || defined(__DEBUGVAR__)

#ifdef __KERNEL__
/* Kernel includes */

#include <linux/sched.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/skbuff.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <asm/io.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/isdnif.h>

#endif                           /* __KERNEL__ */

#define ACT2000_PORTLEN        8

#define ACT2000_FLAGS_RUNNING  1 /* Cards driver activated */
#define ACT2000_FLAGS_PVALID   2 /* Cards port is valid    */
#define ACT2000_FLAGS_IVALID   4 /* Cards irq is valid     */
#define ACT2000_FLAGS_LOADED   8 /* Firmware loaded        */

#define ACT2000_BCH            2 /* # of channels per card */

/* D-Channel states */
#define ACT2000_STATE_NULL     0
#define ACT2000_STATE_ICALL    1
#define ACT2000_STATE_OCALL    2
#define ACT2000_STATE_IWAIT    3
#define ACT2000_STATE_OWAIT    4
#define ACT2000_STATE_IBWAIT   5
#define ACT2000_STATE_OBWAIT   6
#define ACT2000_STATE_BWAIT    7
#define ACT2000_STATE_BHWAIT   8
#define ACT2000_STATE_BHWAIT2  9
#define ACT2000_STATE_DHWAIT  10
#define ACT2000_STATE_DHWAIT2 11
#define ACT2000_STATE_BSETUP  12
#define ACT2000_STATE_ACTIVE  13

#define ACT2000_MAX_QUEUED  8000 /* 2 * maxbuff */

#define ACT2000_LOCK_TX 0
#define ACT2000_LOCK_RX 1

typedef struct act2000_chan {
	unsigned short callref;          /* Call Reference              */
	unsigned short fsm_state;        /* Current D-Channel state     */
	unsigned short eazmask;          /* EAZ-Mask for this Channel   */
	short queued;                    /* User-Data Bytes in TX queue */
	unsigned short plci;
	unsigned short ncci;
	unsigned char  l2prot;           /* Layer 2 protocol            */
	unsigned char  l3prot;           /* Layer 3 protocol            */
} act2000_chan;

typedef struct msn_entry {
	char eaz;
	char msn[16];
	struct msn_entry *next;
} msn_entry;

typedef struct irq_data_isa {
	__u8           *rcvptr;
	__u16           rcvidx;
	__u16           rcvlen;
	struct sk_buff *rcvskb;
	__u8            rcvignore;
	__u8            rcvhdr[8];
} irq_data_isa;

typedef union act2000_irq_data {
	irq_data_isa isa;
} act2000_irq_data;

/*
 * Per card driver data
 */
typedef struct act2000_card {
	unsigned short port;		/* Base-port-address                */
	unsigned short irq;		/* Interrupt                        */
	u_char ptype;			/* Protocol type (1TR6 or Euro)     */
	u_char bus;			/* Cardtype (ISA, MCA, PCMCIA)      */
	struct act2000_card *next;	/* Pointer to next device struct    */
	spinlock_t lock;		/* protect critical operations      */
	int myid;			/* Driver-Nr. assigned by linklevel */
	unsigned long flags;		/* Statusflags                      */
	unsigned long ilock;		/* Semaphores for IRQ-Routines      */
	struct sk_buff_head rcvq;	/* Receive-Message queue            */
	struct sk_buff_head sndq;	/* Send-Message queue               */
	struct sk_buff_head ackq;	/* Data-Ack-Message queue           */
	u_char *ack_msg;		/* Ptr to User Data in User skb     */
	__u16 need_b3ack;		/* Flag: Need ACK for current skb   */
	struct sk_buff *sbuf;		/* skb which is currently sent      */
	struct timer_list ptimer;	/* Poll timer                       */
	struct work_struct snd_tq;	/* Task struct for xmit bh          */
	struct work_struct rcv_tq;	/* Task struct for rcv bh           */
	struct work_struct poll_tq;	/* Task struct for polled rcv bh    */
	msn_entry *msn_list;
	unsigned short msgnum;		/* Message number for sending       */
	spinlock_t mnlock;		/* lock for msgnum                  */
	act2000_chan bch[ACT2000_BCH];	/* B-Channel status/control         */
	char   status_buf[256];		/* Buffer for status messages       */
	char   *status_buf_read;
	char   *status_buf_write;
	char   *status_buf_end;
	act2000_irq_data idat;		/* Data used for IRQ handler        */
	isdn_if interface;		/* Interface to upper layer         */
	char regname[35];		/* Name used for request_region     */
} act2000_card;

static inline void act2000_schedule_tx(act2000_card *card)
{
	schedule_work(&card->snd_tq);
}

static inline void act2000_schedule_rx(act2000_card *card)
{
	schedule_work(&card->rcv_tq);
}

static inline void act2000_schedule_poll(act2000_card *card)
{
	schedule_work(&card->poll_tq);
}

extern char *act2000_find_eaz(act2000_card *, char);

#endif                          /* defined(__KERNEL__) || defined(__DEBUGVAR__) */
#endif                          /* act2000_h */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /* $Id: act2000_isa.c,v 1.11.6.3 2001/09/23 22:24:32 kai Exp $
 *
 * ISDN lowlevel-module for the IBM ISDN-S0 Active 2000 (ISA-Version).
 *
 * Author       Fritz Elfert
 * Copyright    by Fritz Elfert      <fritz@isdn4linux.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Thanks to Friedemann Baitinger and IBM Germany
 *
 */

#include "act2000.h"
#include "act2000_isa.h"
#include "capi.h"

/*
 * Reset Controller, then try to read the Card's signature.
 + Return:
 *   1 = Signature found.
 *   0 = Signature not found.
 */
static int
act2000_isa_reset(unsigned short portbase)
{
	unsigned char reg;
	int i;
	int found;
	int serial = 0;

	found = 0;
	if ((reg = inb(portbase + ISA_COR)) != 0xff) {
		outb(reg | ISA_COR_RESET, portbase + ISA_COR);
		mdelay(10);
		outb(reg, portbase + ISA_COR);
		mdelay(10);

		for (i = 0; i < 16; i++) {
			if (inb(portbase + ISA_ISR) & ISA_ISR_SERIAL)
				serial |= 0x10000;
			serial >>= 1;
		}
		if (serial == ISA_SER_ID)
			found++;
	}
	return found;
}

int
act2000_isa_detect(unsigned short portbase)
{
	int ret = 0;

	if (request_region(portbase, ACT2000_PORTLEN, "act2000isa")) {
		ret = act2000_isa_reset(portbase);
		release_region(portbase, ISA_REGION);
	}
	return ret;
}

static irqreturn_t
act2000_isa_interrupt(int dummy, void *dev_id)
{
	act2000_card *card = dev_id;
	u_char istatus;

	istatus = (inb(ISA_PORT_ISR) & 0x07);
	if (istatus & ISA_ISR_OUT) {
		/* RX fifo has data */
		istatus &= ISA_ISR_OUT_MASK;
		outb(0, ISA_PORT_SIS);
		act2000_isa_receive(card);
		outb(ISA_SIS_INT, ISA_PORT_SIS);
	}
	if (istatus & ISA_ISR_ERR) {
		/* Error Interrupt */
		istatus &= ISA_ISR_ERR_MASK;
		printk(KERN_WARNING "act2000: errIRQ\n");
	}
	if (istatus)
		printk(KERN_DEBUG "act2000: ?IRQ %d %02x\n", card->irq, istatus);
	return IRQ_HANDLED;
}

static void
act2000_isa_select_irq(act2000_card *card)
{
	unsigned char reg;

	reg = (inb(ISA_PORT_COR) & ~ISA_COR_IRQOFF) | ISA_COR_PERR;
	switch (card->irq) {
	case 3:
		reg = ISA_COR_IRQ03;
		break;
	case 5:
		reg = ISA_COR_IRQ05;
		break;
	case 7:
		reg = ISA_COR_IRQ07;
		break;
	case 10:
		reg = ISA_COR_IRQ10;
		break;
	case 11:
		reg = ISA_COR_IRQ11;
		break;
	case 12:
		reg = ISA_COR_IRQ12;
		break;
	case 15:
		reg = ISA_COR_IRQ15;
		break;
	}
	outb(reg, ISA_PORT_COR);
}

static void
act2000_isa_enable_irq(act2000_card *card)
{
	act2000_isa_select_irq(card);
	/* Enable READ irq */
	outb(ISA_SIS_INT, ISA_PORT_SIS);
}

/*
 * Install interrupt handler, enable irq on card.
 * If irq is -1, choose next free irq, else irq is given explicitly.
 */
int
act2000_isa_config_irq(act2000_card *card, short irq)
{
	int old_irq;

	if (card->flags & ACT2000_FLAGS_IVALID) {
		free_irq(card->irq, card);
	}
	card->flags &= ~ACT2000_FLAGS_IVALID;
	outb(ISA_COR_IRQOFF, ISA_PORT_COR);
	if (!irq)
		return 0;

	old_irq = card->irq;
	card->irq = irq;
	if (request_irq(irq, &act2000_isa_interrupt, 0, card->regname, card)) {
		card->irq = old_irq;
		card->flags |= ACT2000_FLAGS_IVALID;
		printk(KERN_WARNING
		       "act2000: Could not request irq %d\n", irq);
		return -EBUSY;
	} else {
		act2000_isa_select_irq(card);
		/* Disable READ and WRITE irq */
		outb(0, ISA_PORT_SIS);
		outb(0, ISA_PORT_SOS);
	}
	return 0;
}

int
act2000_isa_config_port(act2000_card *card, unsigned short portbase)
{
	if (card->flags & ACT2000_FLAGS_PVALID) {
		release_region(card->port, ISA_REGION);
		card->flags &= ~ACT2000_FLAGS_PVALID;
	}
	if (request_region(portbase, ACT2000_PORTLEN, card->regname) == NULL)
		return -EBUSY;
	else {
		card->port = portbase;
		card->flags |= ACT2000_FLAGS_PVALID;
		return 0;
	}
}

/*
 * Release ressources, used by an adaptor.
 */
void
act2000_isa_release(act2000_card *card)
{
	unsigned long flags;

	spin_lock_irqsave(&card->lock, flags);
	if (card->flags & ACT2000_FLAGS_IVALID)
		free_irq(card->irq, card);

	card->flags &= ~ACT2000_FLAGS_IVALID;
	if (card->flags & ACT2000_FLAGS_PVALID)
		release_region(card->port, ISA_REGION);
	card->flags &= ~ACT2000_FLAGS_PVALID;
	spin_unlock_irqrestore(&card->lock, flags);
}

static int
act2000_isa_writeb(act2000_card *card, u_char data)
{
	u_char timeout = 40;

	while (timeout) {
		if (inb(ISA_PORT_SOS) & ISA_SOS_READY) {
			outb(data, ISA_PORT_SDO);
			return 0;
		} else {
			timeout--;
			udelay(10);
		}
	}
	return 1;
}

static int
act2000_isa_readb(act2000_card *card, u_char *data)
{
	u_char timeout = 40;

	while (timeout) {
		if (inb(ISA_PORT_SIS) & ISA_SIS_READY) {
			*data = inb(ISA_PORT_SDI);
			return 0;
		} else {
			timeout--;
			udelay(10);
		}
	}
	return 1;
}

void
act2000_isa_receive(act2000_card *card)
{
	u_char c;

	if (test_and_set_bit(ACT2000_LOCK_RX, (void *) &card->ilock) != 0)
		return;
	while (!act2000_isa_readb(card, &c)) {
		if (card->idat.isa.rcvidx < 8) {
			card->idat.isa.rcvhdr[card->idat.isa.rcvidx++] = c;
			if (card->idat.isa.rcvidx == 8) {
				int valid = actcapi_chkhdr(card, (actcapi_msghdr *)&card->idat.isa.rcvhdr);

				if (valid) {
					card->idat.isa.rcvlen = ((actcapi_msghdr *)&card->idat.isa.rcvhdr)->len;
					card->idat.isa.rcvskb = dev_alloc_skb(card->idat.isa.rcvlen);
					if (card->idat.isa.rcvskb == NULL) {
						card->idat.isa.rcvignore = 1;
						printk(KERN_WARNING
						       "act2000_isa_receive: no memory\n");
						test_and_clear_bit(ACT2000_LOCK_RX, (void *) &card->ilock);
						return;
					}
					memcpy(skb_put(card->idat.isa.rcvskb, 8), card->idat.isa.rcvhdr, 8);
					card->idat.isa.rcvptr = skb_put(card->idat.isa.rcvskb, card->idat.isa.rcvlen - 8);
				} else {
					card->idat.isa.rcvidx = 0;
					printk(KERN_WARNING
					       "act2000_isa_receive: Invalid CAPI msg\n");
					{
						int i; __u8 *p; __u8 *t; __u8 tmp[30];
						for (i = 0, p = (__u8 *)&card->idat.isa.rcvhdr, t = tmp; i < 8; i++)
							t += sprintf(t, "%02x ", *(p++));
						printk(KERN_WARNING "act2000_isa_receive: %s\n", tmp);
					}
				}
			}
		} else {
			if (!card->idat.isa.rcvignore)
				*card->idat.isa.rcvptr++ = c;
			if (++card->idat.isa.rcvidx >= card->idat.isa.rcvlen) {
				if (!card->idat.isa.rcvignore) {
					skb_queue_tail(&card->rcvq, card->idat.isa.rcvskb);
					act2000_schedule_rx(card);
				}
				card->idat.isa.rcvidx = 0;
				card->idat.isa.rcvlen = 8;
				card->idat.isa.rcvignore = 0;
				card->idat.isa.rcvskb = NULL;
				card->idat.isa.rcvptr = card->idat.isa.rcvhdr;
			}
		}
	}
	if (!(card->flags & ACT2000_FLAGS_IVALID)) {
		/* In polling mode, schedule myself */
		if ((card->idat.isa.rcvidx) &&
		    (card->idat.isa.rcvignore ||
		     (card->idat.isa.rcvidx < card->idat.isa.rcvlen)))
			act2000_schedule_poll(card);
	}
	test_and_clear_bit(ACT2000_LOCK_RX, (void *) &card->ilock);
}

void
act2000_isa_send(act2000_card *card)
{
	unsigned long flags;
	struct sk_buff *skb;
	actcapi_msg *msg;
	int l;

	if (test_and_set_bit(ACT2000_LOCK_TX, (void *) &card->ilock) != 0)
		return;
	while (1) {
		spin_lock_irqsave(&card->lock, flags);
		if (!(card->sbuf)) {
			if ((card->sbuf = skb_dequeue(&card->sndq))) {
				card->ack_msg = card->sbuf->data;
				msg = (actcapi_msg *)card->sbuf->data;
				if ((msg->hdr.cmd.cmd == 0x86) &&
				    (msg->hdr.cmd.subcmd == 0)) {
					/* Save flags in message */
					card->need_b3ack = msg->msg.data_b3_req.flags;
					msg->msg.data_b3_req.flags = 0;
				}
			}
		}
		spin_unlock_irqrestore(&card->lock, flags);
		if (!(card->sbuf)) {
			/* No more data to send */
			test_and_clear_bit(ACT2000_LOCK_TX, (void *) &card->ilock);
			return;
		}
		skb = card->sbuf;
		l = 0;
		while (skb->len) {
			if (act2000_isa_writeb(card, *(skb->data))) {
				/* Fifo is full, but more data to send */
				test_and_clear_bit(ACT2000_LOCK_TX, (void *) &card->ilock);
				/* Schedule myself */
				act2000_schedule_tx(card);
				return;
			}
			skb_pull(skb, 1);
			l++;
		}
		msg = (actcapi_msg *)card->ack_msg;
		if ((msg->hdr.cmd.cmd == 0x86) &&
		    (msg->hdr.cmd.subcmd == 0)) {
			/*
			 * If it's user data, reset data-ptr
			 * and put skb into ackq.
			 */
			skb->data = card->ack_msg;
			/* Restore flags in message */
			msg->msg.data_b3_req.flags = card->need_b3ack;
			skb_queue_tail(&card->ackq, skb);
		} else
			dev_kfree_skb(skb);
		card->sbuf = NULL;
	}
}

/*
 * Get firmware ID, check for 'ISDN' signature.
 */
static int
act2000_isa_getid(act2000_card *card)
{

	act2000_fwid fid;
	u_char *p = (u_char *)&fid;
	int count = 0;

	while (1) {
		if (count > 510)
			return -EPROTO;
		if (act2000_isa_readb(card, p++))
			break;
		count++;
	}
	if (count <= 20) {
		printk(KERN_WARNING "act2000: No Firmware-ID!\n");
		return -ETIME;
	}
	*p = '\0';
	fid.revlen[0] = '\0';
	if (strcmp(fid.isdn, "ISDN")) {
		printk(KERN_WARNING "act2000: Wrong Firmware-ID!\n");
		return -EPROTO;
	}
	if ((p = strchr(fid.revision, '\n')))
		*p = '\0';
	printk(KERN_INFO "act2000: Firmware-ID: %s\n", fid.revision);
	if (card->flags & ACT2000_FLAGS_IVALID) {
		printk(KERN_DEBUG "Enabling Interrupts ...\n");
		act2000_isa_enable_irq(card);
	}
	return 0;
}

/*
 * Download microcode into card, check Firmware signature.
 */
int
act2000_isa_download(act2000_card *card, act2000_ddef __user *cb)
{
	unsigned int length;
	int l;
	int c;
	long timeout;
	u_char *b;
	u_char __user *p;
	u_char *buf;
	act2000_ddef cblock;

	if (!act2000_isa_reset(card->port))
		return -ENXIO;
	msleep_interruptible(500);
	if (copy_from_user(&cblock, cb, sizeof(cblock)))
		return -EFAULT;
	length = cblock.length;
	p = cblock.buffer;
	if (!access_ok(VERIFY_READ, p, length))
		return -EFAULT;
	buf = kmalloc(1024, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	timeout = 0;
	while (length) {
		l = (length > 1024) ? 1024 : length;
		c = 0;
		b = buf;
		if (copy_from_user(buf, p, l)) {
			kfree(buf);
			return -EFAULT;
		}
		while (c < l) {
			if (act2000_isa_writeb(card, *b++)) {
				printk(KERN_WARNING
				       "act2000: loader timed out"
				       " len=%d c=%d\n", length, c);
				kfree(buf);
				return -ETIME;
			}
			c++;
		}
		length -= l;
		p += l;
	}
	kfree(buf);
	msleep_interruptible(500);
	return (act2000_isa_getid(card));
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /* $Id: act2000_isa.h,v 1.4.6.1 2001/09/23 22:24:32 kai Exp $
 *
 * ISDN lowlevel-module for the IBM ISDN-S0 Active 2000 (ISA-Version).
 *
 * Author       Fritz Elfert
 * Copyright    by Fritz Elfert      <fritz@isdn4linux.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Thanks to Friedemann Baitinger and IBM Germany
 *
 */

#ifndef act2000_isa_h
#define act2000_isa_h

#define ISA_POLL_LOOP 40        /* Try to read-write before give up */

typedef enum {
	INT_NO_CHANGE = 0,      /* Do not change the Mask */
	INT_ON = 1,             /* Set to Enable */
	INT_OFF = 2,            /* Set to Disable */
} ISA_INT_T;

/**************************************************************************/
/*      Configuration Register COR (RW)                                   */
/**************************************************************************/
/*    7    |   6    |    5   |   4    |    3   |    2   |    1   |    0   */
/* Soft Res|  IRQM  |        IRQ Select        |   N/A  |  WAIT  |Proc err */
/**************************************************************************/
#define        ISA_COR             0	/* Offset for ISA config register */
#define        ISA_COR_PERR     0x01	/* Processor Error Enabled        */
#define        ISA_COR_WS       0x02	/* Insert Wait State if 1         */
#define        ISA_COR_IRQOFF   0x38	/* No Interrupt                   */
#define        ISA_COR_IRQ07    0x30	/* IRQ 7 Enable                   */
#define        ISA_COR_IRQ05    0x28	/* IRQ 5 Enable                   */
#define        ISA_COR_IRQ03    0x20	/* IRQ 3 Enable                   */
#define        ISA_COR_IRQ10    0x18	/* IRQ 10 Enable                  */
#define        ISA_COR_IRQ11    0x10	/* IRQ 11 Enable                  */
#define        ISA_COR_IRQ12    0x08	/* IRQ 12 Enable                  */
#define        ISA_COR_IRQ15    0x00	/* IRQ 15 Enable                  */
#define        ISA_COR_IRQPULSE 0x40	/* 0 = Level 1 = Pulse Interrupt  */
#define        ISA_COR_RESET    0x80	/* Soft Reset for Transputer      */

/**************************************************************************/
/*      Interrupt Source Register ISR (RO)                                */
/**************************************************************************/
/*    7    |   6    |    5   |   4    |    3   |    2   |    1   |    0   */
/*   N/A   |  N/A   |   N/A  |Err sig |Ser ID  |IN Intr |Out Intr| Error  */
/**************************************************************************/
#define        ISA_ISR             1	/* Offset for Interrupt Register  */
#define        ISA_ISR_ERR      0x01	/* Error Interrupt                */
#define        ISA_ISR_OUT      0x02	/* Output Interrupt               */
#define        ISA_ISR_INP      0x04	/* Input Interrupt                */
#define        ISA_ISR_SERIAL   0x08	/* Read out Serial ID after Reset */
#define        ISA_ISR_ERRSIG   0x10	/* Error Signal Input             */
#define        ISA_ISR_ERR_MASK 0xfe    /* Mask Error Interrupt           */
#define        ISA_ISR_OUT_MASK 0xfd    /* Mask Output Interrupt          */
#define        ISA_ISR_INP_MASK 0xfb    /* Mask Input Interrupt           */

/* Signature delivered after Reset at ISA_ISR_SERIAL (LSB first)          */
#define        ISA_SER_ID     0x0201	/* ID for ISA Card                */

/**************************************************************************/
/*      EEPROM Register EPR (RW)                                          */
/**************************************************************************/
/*    7    |   6    |    5   |   4    |    3   |    2   |    1   |    0   */
/*   N/A   |  N/A   |   N/A  |ROM Hold| ROM CS |ROM CLK | ROM IN |ROM Out */
/**************************************************************************/
#define        ISA_EPR             2	/* Offset for this Register       */
#define        ISA_EPR_OUT      0x01	/* Rome Register Out (RO)         */
#define        ISA_EPR_IN       0x02	/* Rom Register In (WR)           */
#define        ISA_EPR_CLK      0x04	/* Rom Clock (WR)                 */
#define        ISA_EPR_CS       0x08	/* Rom Cip Select (WR)            */
#define        ISA_EPR_HOLD     0x10	/* Rom Hold Signal (WR)           */

/**************************************************************************/
/*      EEPROM enable Register EER (unused)                               */
/**************************************************************************/
#define        ISA_EER             3	/* Offset for this Register       */

/**************************************************************************/
/*      SLC Data Input SDI (RO)                                           */
/**************************************************************************/
#define        ISA_SDI             4	/* Offset for this Register       */

/**************************************************************************/
/*      SLC Data Output SDO (WO)                                          */
/**************************************************************************/
#define        ISA_SDO             5	/* Offset for this Register       */

/**************************************************************************/
/*      IMS C011 Mode 2 Input Status Register for INMOS CPU SIS (RW)      */
/**************************************************************************/
/*    7    |   6    |    5   |   4    |    3   |    2   |    1   |    0   */
/*   N/A   |  N/A   |   N/A  |  N/A   |   N/A  |   N/A  |Int Ena |Data Pre */
/**************************************************************************/
#define        ISA_SIS             6	/* Offset for this Register       */
#define        ISA_SIS_READY    0x01	/* If 1 : data is available       */
#define        ISA_SIS_INT      0x02	/* Enable Interrupt for READ      */

/**************************************************************************/
/*      IMS C011 Mode 2 Output Status Register from INMOS CPU SOS (RW)    */
/**************************************************************************/
/*    7    |   6    |    5   |   4    |    3   |    2   |    1   |    0   */
/*   N/A   |  N/A   |   N/A  |  N/A   |   N/A  |   N/A  |Int Ena |Out Rdy */
/**************************************************************************/
#define        ISA_SOS             7	/* Offset for this Register       */
#define        ISA_SOS_READY    0x01	/* If 1 : we can write Data       */
#define        ISA_SOS_INT      0x02	/* Enable Interrupt for WRITE     */

#define        ISA_REGION          8	/* Number of Registers            */


/* Macros for accessing ports */
#define ISA_PORT_COR (card->port + ISA_COR)
#define ISA_PORT_ISR (card->port + ISA_ISR)
#define ISA_PORT_EPR (card->port + ISA_EPR)
#define ISA_PORT_EER (card->port + ISA_EER)
#define ISA_PORT_SDI (card->port + ISA_SDI)
#define ISA_PORT_SDO (card->port + ISA_SDO)
#define ISA_PORT_SIS (card->port + ISA_SIS)
#define ISA_PORT_SOS (card->port + ISA_SOS)

/* Prototypes */

extern int act2000_isa_detect(unsigned short portbase);
extern int act2000_isa_config_irq(act2000_card *card, short irq);
extern int act2000_isa_config_port(act2000_card *card, unsigned short portbase);
extern int act2000_isa_download(act2000_card *card, act2000_ddef __user *cb);
extern void act2000_isa_release(act2000_card *card);
extern void act2000_isa_receive(act2000_card *card);
extern void act2000_isa_send(act2000_card *card);

#endif                          /* act2000_isa_h */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /* $Id: capi.c,v 1.9.6.2 2001/09/23 22:24:32 kai Exp $
 *
 * ISDN lowlevel-module for the IBM ISDN-S0 Active 2000.
 * CAPI encoder/decoder
 *
 * Author       Fritz Elfert
 * Copyright    by Fritz Elfert      <fritz@isdn4linux.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Thanks to Friedemann Baitinger and IBM Germany
 *
 */

#include "act2000.h"
#include "capi.h"

static actcapi_msgdsc valid_msg[] = {
	{{ 0x86, 0x02}, "DATA_B3_IND"},       /* DATA_B3_IND/CONF must be first because of speed!!! */
	{{ 0x86, 0x01}, "DATA_B3_CONF"},
	{{ 0x02, 0x01}, "CONNECT_CONF"},
	{{ 0x02, 0x02}, "CONNECT_IND"},
	{{ 0x09, 0x01}, "CONNECT_INFO_CONF"},
	{{ 0x03, 0x02}, "CONNECT_ACTIVE_IND"},
	{{ 0x04, 0x01}, "DISCONNECT_CONF"},
	{{ 0x04, 0x02}, "DISCONNECT_IND"},
	{{ 0x05, 0x01}, "LISTEN_CONF"},
	{{ 0x06, 0x01}, "GET_PARAMS_CONF"},
	{{ 0x07, 0x01}, "INFO_CONF"},
	{{ 0x07, 0x02}, "INFO_IND"},
	{{ 0x08, 0x01}, "DATA_CONF"},
	{{ 0x08, 0x02}, "DATA_IND"},
	{{ 0x40, 0x01}, "SELECT_B2_PROTOCOL_CONF"},
	{{ 0x80, 0x01}, "SELECT_B3_PROTOCOL_CONF"},
	{{ 0x81, 0x01}, "LISTEN_B3_CONF"},
	{{ 0x82, 0x01}, "CONNECT_B3_CONF"},
	{{ 0x82, 0x02}, "CONNECT_B3_IND"},
	{{ 0x83, 0x02}, "CONNECT_B3_ACTIVE_IND"},
	{{ 0x84, 0x01}, "DISCONNECT_B3_CONF"},
	{{ 0x84, 0x02}, "DISCONNECT_B3_IND"},
	{{ 0x85, 0x01}, "GET_B3_PARAMS_CONF"},
	{{ 0x01, 0x01}, "RESET_B3_CONF"},
	{{ 0x01, 0x02}, "RESET_B3_IND"},
	/* {{ 0x87, 0x02, "HANDSET_IND"}, not implemented */
	{{ 0xff, 0x01}, "MANUFACTURER_CONF"},
	{{ 0xff, 0x02}, "MANUFACTURER_IND"},
#ifdef DEBUG_MSG
	/* Requests */
	{{ 0x01, 0x00}, "RESET_B3_REQ"},
	{{ 0x02, 0x00}, "CONNECT_REQ"},
	{{ 0x04, 0x00}, "DISCONNECT_REQ"},
	{{ 0x05, 0x00}, "LISTEN_REQ"},
	{{ 0x06, 0x00}, "GET_PARAMS_REQ"},
	{{ 0x07, 0x00}, "INFO_REQ"},
	{{ 0x08, 0x00}, "DATA_REQ"},
	{{ 0x09, 0x00}, "CONNECT_INFO_REQ"},
	{{ 0x40, 0x00}, "SELECT_B2_PROTOCOL_REQ"},
	{{ 0x80, 0x00}, "SELECT_B3_PROTOCOL_REQ"},
	{{ 0x81, 0x00}, "LISTEN_B3_REQ"},
	{{ 0x82, 0x00}, "CONNECT_B3_REQ"},
	{{ 0x84, 0x00}, "DISCONNECT_B3_REQ"},
	{{ 0x85, 0x00}, "GET_B3_PARAMS_REQ"},
	{{ 0x86, 0x00}, "DATA_B3_REQ"},
	{{ 0xff, 0x00}, "MANUFACTURER_REQ"},
	/* Responses */
	{{ 0x01, 0x03}, "RESET_B3_RESP"},
	{{ 0x02, 0x03}, "CONNECT_RESP"},
	{{ 0x03, 0x03}, "CONNECT_ACTIVE_RESP"},
	{{ 0x04, 0x03}, "DISCONNECT_RESP"},
	{{ 0x07, 0x03}, "INFO_RESP"},
	{{ 0x08, 0x03}, "DATA_RESP"},
	{{ 0x82, 0x03}, "CONNECT_B3_RESP"},
	{{ 0x83, 0x03}, "CONNECT_B3_ACTIVE_RESP"},
	{{ 0x84, 0x03}, "DISCONNECT_B3_RESP"},
	{{ 0x86, 0x03}, "DATA_B3_RESP"},
	{{ 0xff, 0x03}, "MANUFACTURER_RESP"},
#endif
	{{ 0x00, 0x00}, NULL},
};
#define num_valid_imsg 27 /* MANUFACTURER_IND */

/*
 * Check for a valid incoming CAPI message.
 * Return:
 *   0 = Invalid message
 *   1 = Valid message, no B-Channel-data
 *   2 = Valid message, B-Channel-data
 */
int
actcapi_chkhdr(act2000_card *card, actcapi_msghdr *hdr)
{
	int i;

	if (hdr->applicationID != 1)
		return 0;
	if (hdr->len < 9)
		return 0;
	for (i = 0; i < num_valid_imsg; i++)
		if ((hdr->cmd.cmd == valid_msg[i].cmd.cmd) &&
		    (hdr->cmd.subcmd == valid_msg[i].cmd.subcmd)) {
			return (i ? 1 : 2);
		}
	return 0;
}

#define ACTCAPI_MKHDR(l, c, s) {				\
		skb = alloc_skb(l + 8, GFP_ATOMIC);		\
		if (skb) {					\
			m = (actcapi_msg *)skb_put(skb, l + 8); \
			m->hdr.len = l + 8;			\
			m->hdr.applicationID = 1;		\
			m->hdr.cmd.cmd = c;			\
			m->hdr.cmd.subcmd = s;			\
			m->hdr.msgnum = actcapi_nextsmsg(card); \
		} else m = NULL;				\
	}

#define ACTCAPI_CHKSKB if (!skb) {					\
		printk(KERN_WARNING "actcapi: alloc_skb failed\n");	\
		return;							\
	}

#define ACTCAPI_QUEUE_TX {				\
		actcapi_debug_msg(skb, 1);		\
		skb_queue_tail(&card->sndq, skb);	\
		act2000_schedule_tx(card);		\
	}

int
actcapi_listen_req(act2000_card *card)
{
	__u16 eazmask = 0;
	int i;
	actcapi_msg *m;
	struct sk_buff *skb;

	for (i = 0; i < ACT2000_BCH; i++)
		eazmask |= card->bch[i].eazmask;
	ACTCAPI_MKHDR(9, 0x05, 0x00);
	if (!skb) {
		printk(KERN_WARNING "actcapi: alloc_skb failed\n");
		return -ENOMEM;
	}
	m->msg.listen_req.controller = 0;
	m->msg.listen_req.infomask = 0x3f; /* All information */
	m->msg.listen_req.eazmask = eazmask;
	m->msg.listen_req.simask = (eazmask) ? 0x86 : 0; /* All SI's  */
	ACTCAPI_QUEUE_TX;
	return 0;
}

int
actcapi_connect_req(act2000_card *card, act2000_chan *chan, char *phone,
		    char eaz, int si1, int si2)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR((11 + strlen(phone)), 0x02, 0x00);
	if (!skb) {
		printk(KERN_WARNING "actcapi: alloc_skb failed\n");
		chan->fsm_state = ACT2000_STATE_NULL;
		return -ENOMEM;
	}
	m->msg.connect_req.controller = 0;
	m->msg.connect_req.bchan = 0x83;
	m->msg.connect_req.infomask = 0x3f;
	m->msg.connect_req.si1 = si1;
	m->msg.connect_req.si2 = si2;
	m->msg.connect_req.eaz = eaz ? eaz : '0';
	m->msg.connect_req.addr.len = strlen(phone) + 1;
	m->msg.connect_req.addr.tnp = 0x81;
	memcpy(m->msg.connect_req.addr.num, phone, strlen(phone));
	chan->callref = m->hdr.msgnum;
	ACTCAPI_QUEUE_TX;
	return 0;
}

static void
actcapi_connect_b3_req(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(17, 0x82, 0x00);
	ACTCAPI_CHKSKB;
	m->msg.connect_b3_req.plci = chan->plci;
	memset(&m->msg.connect_b3_req.ncpi, 0,
	       sizeof(m->msg.connect_b3_req.ncpi));
	m->msg.connect_b3_req.ncpi.len = 13;
	m->msg.connect_b3_req.ncpi.modulo = 8;
	ACTCAPI_QUEUE_TX;
}

/*
 * Set net type (1TR6) or (EDSS1)
 */
int
actcapi_manufacturer_req_net(act2000_card *card)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(5, 0xff, 0x00);
	if (!skb) {
		printk(KERN_WARNING "actcapi: alloc_skb failed\n");
		return -ENOMEM;
	}
	m->msg.manufacturer_req_net.manuf_msg = 0x11;
	m->msg.manufacturer_req_net.controller = 1;
	m->msg.manufacturer_req_net.nettype = (card->ptype == ISDN_PTYPE_EURO) ? 1 : 0;
	ACTCAPI_QUEUE_TX;
	printk(KERN_INFO "act2000 %s: D-channel protocol now %s\n",
	       card->interface.id, (card->ptype == ISDN_PTYPE_EURO) ? "euro" : "1tr6");
	card->interface.features &=
		~(ISDN_FEATURE_P_UNKNOWN | ISDN_FEATURE_P_EURO | ISDN_FEATURE_P_1TR6);
	card->interface.features |=
		((card->ptype == ISDN_PTYPE_EURO) ? ISDN_FEATURE_P_EURO : ISDN_FEATURE_P_1TR6);
	return 0;
}

/*
 * Switch V.42 on or off
 */
#if 0
int
actcapi_manufacturer_req_v42(act2000_card *card, ulong arg)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(8, 0xff, 0x00);
	if (!skb) {

		printk(KERN_WARNING "actcapi: alloc_skb failed\n");
		return -ENOMEM;
	}
	m->msg.manufacturer_req_v42.manuf_msg = 0x10;
	m->msg.manufacturer_req_v42.controller = 0;
	m->msg.manufacturer_req_v42.v42control = (arg ? 1 : 0);
	ACTCAPI_QUEUE_TX;
	return 0;
}
#endif  /*  0  */

/*
 * Set error-handler
 */
int
actcapi_manufacturer_req_errh(act2000_card *card)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(4, 0xff, 0x00);
	if (!skb) {

		printk(KERN_WARNING "actcapi: alloc_skb failed\n");
		return -ENOMEM;
	}
	m->msg.manufacturer_req_err.manuf_msg = 0x03;
	m->msg.manufacturer_req_err.controller = 0;
	ACTCAPI_QUEUE_TX;
	return 0;
}

/*
 * Set MSN-Mapping.
 */
int
actcapi_manufacturer_req_msn(act2000_card *card)
{
	msn_entry *p = card->msn_list;
	actcapi_msg *m;
	struct sk_buff *skb;
	int len;

	while (p) {
		int i;

		len = strlen(p->msn);
		for (i = 0; i < 2; i++) {
			ACTCAPI_MKHDR(6 + len, 0xff, 0x00);
			if (!skb) {
				printk(KERN_WARNING "actcapi: alloc_skb failed\n");
				return -ENOMEM;
			}
			m->msg.manufacturer_req_msn.manuf_msg = 0x13 + i;
			m->msg.manufacturer_req_msn.controller = 0;
			m->msg.manufacturer_req_msn.msnmap.eaz = p->eaz;
			m->msg.manufacturer_req_msn.msnmap.len = len;
			memcpy(m->msg.manufacturer_req_msn.msnmap.msn, p->msn, len);
			ACTCAPI_QUEUE_TX;
		}
		p = p->next;
	}
	return 0;
}

void
actcapi_select_b2_protocol_req(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(10, 0x40, 0x00);
	ACTCAPI_CHKSKB;
	m->msg.select_b2_protocol_req.plci = chan->plci;
	memset(&m->msg.select_b2_protocol_req.dlpd, 0,
	       sizeof(m->msg.select_b2_protocol_req.dlpd));
	m->msg.select_b2_protocol_req.dlpd.len = 6;
	switch (chan->l2prot) {
	case ISDN_PROTO_L2_TRANS:
		m->msg.select_b2_protocol_req.protocol = 0x03;
		m->msg.select_b2_protocol_req.dlpd.dlen = 4000;
		break;
	case ISDN_PROTO_L2_HDLC:
		m->msg.select_b2_protocol_req.protocol = 0x02;
		m->msg.select_b2_protocol_req.dlpd.dlen = 4000;
		break;
	case ISDN_PROTO_L2_X75I:
	case ISDN_PROTO_L2_X75UI:
	case ISDN_PROTO_L2_X75BUI:
		m->msg.select_b2_protocol_req.protocol = 0x01;
		m->msg.select_b2_protocol_req.dlpd.dlen = 4000;
		m->msg.select_b2_protocol_req.dlpd.laa = 3;
		m->msg.select_b2_protocol_req.dlpd.lab = 1;
		m->msg.select_b2_protocol_req.dlpd.win = 7;
		m->msg.select_b2_protocol_req.dlpd.modulo = 8;
		break;
	}
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_select_b3_protocol_req(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(17, 0x80, 0x00);
	ACTCAPI_CHKSKB;
	m->msg.select_b3_protocol_req.plci = chan->plci;
	memset(&m->msg.select_b3_protocol_req.ncpd, 0,
	       sizeof(m->msg.select_b3_protocol_req.ncpd));
	switch (chan->l3prot) {
	case ISDN_PROTO_L3_TRANS:
		m->msg.select_b3_protocol_req.protocol = 0x04;
		m->msg.select_b3_protocol_req.ncpd.len = 13;
		m->msg.select_b3_protocol_req.ncpd.modulo = 8;
		break;
	}
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_listen_b3_req(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(2, 0x81, 0x00);
	ACTCAPI_CHKSKB;
	m->msg.listen_b3_req.plci = chan->plci;
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_disconnect_req(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(3, 0x04, 0x00);
	ACTCAPI_CHKSKB;
	m->msg.disconnect_req.plci = chan->plci;
	m->msg.disconnect_req.cause = 0;
	ACTCAPI_QUEUE_TX;
}

void
actcapi_disconnect_b3_req(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(17, 0x84, 0x00);
	ACTCAPI_CHKSKB;
	m->msg.disconnect_b3_req.ncci = chan->ncci;
	memset(&m->msg.disconnect_b3_req.ncpi, 0,
	       sizeof(m->msg.disconnect_b3_req.ncpi));
	m->msg.disconnect_b3_req.ncpi.len = 13;
	m->msg.disconnect_b3_req.ncpi.modulo = 8;
	chan->fsm_state = ACT2000_STATE_BHWAIT;
	ACTCAPI_QUEUE_TX;
}

void
actcapi_connect_resp(act2000_card *card, act2000_chan *chan, __u8 cause)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(3, 0x02, 0x03);
	ACTCAPI_CHKSKB;
	m->msg.connect_resp.plci = chan->plci;
	m->msg.connect_resp.rejectcause = cause;
	if (cause) {
		chan->fsm_state = ACT2000_STATE_NULL;
		chan->plci = 0x8000;
	} else
		chan->fsm_state = ACT2000_STATE_IWAIT;
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_connect_active_resp(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(2, 0x03, 0x03);
	ACTCAPI_CHKSKB;
	m->msg.connect_resp.plci = chan->plci;
	if (chan->fsm_state == ACT2000_STATE_IWAIT)
		chan->fsm_state = ACT2000_STATE_IBWAIT;
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_connect_b3_resp(act2000_card *card, act2000_chan *chan, __u8 rejectcause)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR((rejectcause ? 3 : 17), 0x82, 0x03);
	ACTCAPI_CHKSKB;
	m->msg.connect_b3_resp.ncci = chan->ncci;
	m->msg.connect_b3_resp.rejectcause = rejectcause;
	if (!rejectcause) {
		memset(&m->msg.connect_b3_resp.ncpi, 0,
		       sizeof(m->msg.connect_b3_resp.ncpi));
		m->msg.connect_b3_resp.ncpi.len = 13;
		m->msg.connect_b3_resp.ncpi.modulo = 8;
		chan->fsm_state = ACT2000_STATE_BWAIT;
	}
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_connect_b3_active_resp(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(2, 0x83, 0x03);
	ACTCAPI_CHKSKB;
	m->msg.connect_b3_active_resp.ncci = chan->ncci;
	chan->fsm_state = ACT2000_STATE_ACTIVE;
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_info_resp(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(2, 0x07, 0x03);
	ACTCAPI_CHKSKB;
	m->msg.info_resp.plci = chan->plci;
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_disconnect_b3_resp(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(2, 0x84, 0x03);
	ACTCAPI_CHKSKB;
	m->msg.disconnect_b3_resp.ncci = chan->ncci;
	chan->ncci = 0x8000;
	chan->queued = 0;
	ACTCAPI_QUEUE_TX;
}

static void
actcapi_disconnect_resp(act2000_card *card, act2000_chan *chan)
{
	actcapi_msg *m;
	struct sk_buff *skb;

	ACTCAPI_MKHDR(2, 0x04, 0x03);
	ACTCAPI_CHKSKB;
	m->msg.disconnect_resp.plci = chan->plci;
	chan->plci = 0x8000;
	ACTCAPI_QUEUE_TX;
}

static int
new_plci(act2000_card *card, __u16 plci)
{
	int i;
	for (i = 0; i < ACT2000_BCH; i++)
		if (card->bch[i].plci == 0x8000) {
			card->bch[i].plci = plci;
			return i;
		}
	return -1;
}

static int
find_plci(act2000_card *card, __u16 plci)
{
	int i;
	for (i = 0; i < ACT2000_BCH; i++)
		if (card->bch[i].plci == plci)
			return i;
	return -1;
}

static int
find_ncci(act2000_card *card, __u16 ncci)
{
	int i;
	for (i = 0; i < ACT2000_BCH; i++)
		if (card->bch[i].ncci == ncci)
			return i;
	return -1;
}

static int
find_dialing(act2000_card *card, __u16 callref)
{
	int i;
	for (i = 0; i < ACT2000_BCH; i++)
		if ((card->bch[i].callref == callref) &&
		    (card->bch[i].fsm_state == ACT2000_STATE_OCALL))
			return i;
	return -1;
}

static int
actcapi_data_b3_ind(act2000_card *card, struct sk_buff *skb) {
	__u16 plci;
	__u16 ncci;
	__u16 controller;
	__u8  blocknr;
	int chan;
	actcapi_msg *msg = (actcapi_msg *)skb->data;

	EVAL_NCCI(msg->msg.data_b3_ind.fakencci, plci, controller, ncci);
	chan = find_ncci(card, ncci);
	if (chan < 0)
		return 0;
	if (card->bch[chan].fsm_state != ACT2000_STATE_ACTIVE)
		return 0;
	if (card->bch[chan].plci != plci)
		return 0;
	blocknr = msg->msg.data_b3_ind.blocknr;
	skb_pull(skb, 19);
	card->interface.rcvcallb_skb(card->myid, chan, skb);
	if (!(skb = alloc_skb(11, GFP_ATOMIC))) {
		printk(KERN_WARNING "actcapi: alloc_skb failed\n");
		return 1;
	}
	msg = (actcapi_msg *)skb_put(skb, 11);
	msg->hdr.len = 11;
	msg->hdr.applicationID = 1;
	msg->hdr.cmd.cmd = 0x86;
	msg->hdr.cmd.subcmd = 0x03;
	msg->hdr.msgnum = actcapi_nextsmsg(card);
	msg->msg.data_b3_resp.ncci = ncci;
	msg->msg.data_b3_resp.blocknr = blocknr;
	ACTCAPI_QUEUE_TX;
	return 1;
}

/*
 * Walk over ackq, unlink DATA_B3_REQ from it, if
 * ncci and blocknr are matching.
 * Decrement queued-bytes counter.
 */
static int
handle_ack(act2000_card *card, act2000_chan *chan, __u8 blocknr) {
	unsigned long flags;
	struct sk_buff *skb;
	struct sk_buff *tmp;
	struct actcapi_msg *m;
	int ret = 0;

	spin_lock_irqsave(&card->lock, flags);
	skb = skb_peek(&card->ackq);
	spin_unlock_irqrestore(&card->lock, flags);
	if (!skb) {
		printk(KERN_WARNING "act2000: handle_ack nothing found!\n");
		return 0;
	}
	tmp = skb;
	while (1) {
		m = (actcapi_msg *)tmp->data;
		if ((((m->msg.data_b3_req.fakencci >> 8) & 0xff) == chan->ncci) &&
		    (m->msg.data_b3_req.blocknr == blocknr)) {
			/* found corresponding DATA_B3_REQ */
			skb_unlink(tmp, &card->ackq);
			chan->queued -= m->msg.data_b3_req.datalen;
			if (m->msg.data_b3_req.flags)
				ret = m->msg.data_b3_req.datalen;
			dev_kfree_skb(tmp);
			if (chan->queued < 0)
				chan->queued = 0;
			return ret;
		}
		spin_lock_irqsave(&card->lock, flags);
		tmp = skb_peek((struct sk_buff_head *)tmp);
		spin_unlock_irqrestore(&card->lock, flags);
		if ((tmp == skb) || (tmp == NULL)) {
			/* reached end of queue */
			printk(KERN_WARNING "act2000: handle_ack nothing found!\n");
			return 0;
		}
	}
}

void
actcapi_dispatch(struct work_struct *work)
{
	struct act2000_card *card =
		container_of(work, struct act2000_card, rcv_tq);
	struct sk_buff *skb;
	actcapi_msg *msg;
	__u16 ccmd;
	int chan;
	int len;
	act2000_chan *ctmp;
	isdn_ctrl cmd;
	char tmp[170];

	while ((skb = skb_dequeue(&card->rcvq))) {
		actcapi_debug_msg(skb, 0);
		msg = (actcapi_msg *)skb->data;
		ccmd = ((msg->hdr.cmd.cmd << 8) | msg->hdr.cmd.subcmd);
		switch (ccmd) {
		case 0x8602:
			/* DATA_B3_IND */
			if (actcapi_data_b3_ind(card, skb))
				return;
			break;
		case 0x8601:
			/* DATA_B3_CONF */
			chan = find_ncci(card, msg->msg.data_b3_conf.ncci);
			if ((chan >= 0) && (card->bch[chan].fsm_state == ACT2000_STATE_ACTIVE)) {
				if (msg->msg.data_b3_conf.info != 0)
					printk(KERN_WARNING "act2000: DATA_B3_CONF: %04x\n",
					       msg->msg.data_b3_conf.info);
				len = handle_ack(card, &card->bch[chan],
						 msg->msg.data_b3_conf.blocknr);
				if (len) {
					cmd.driver = card->myid;
					cmd.command = ISDN_STAT_BSENT;
					cmd.arg = chan;
					cmd.parm.length = len;
					card->interface.statcallb(&cmd);
				}
			}
			break;
		case 0x0201:
			/* CONNECT_CONF */
			chan = find_dialing(card, msg->hdr.msgnum);
			if (chan >= 0) {
				if (msg->msg.connect_conf.info) {
					card->bch[chan].fsm_state = ACT2000_STATE_NULL;
					cmd.driver = card->myid;
					cmd.command = ISDN_STAT_DHUP;
					cmd.arg = chan;
					card->interface.statcallb(&cmd);
				} else {
					card->bch[chan].fsm_state = ACT2000_STATE_OWAIT;
					card->bch[chan].plci = msg->msg.connect_conf.plci;
				}
			}
			break;
		case 0x0202:
			/* CONNECT_IND */
			chan = new_plci(card, msg->msg.connect_ind.plci);
			if (chan < 0) {
				ctmp = (act2000_chan *)tmp;
				ctmp->plci = msg->msg.connect_ind.plci;
				actcapi_connect_resp(card, ctmp, 0x11); /* All Card-Cannels busy */
			} else {
				card->bch[chan].fsm_state = ACT2000_STATE_ICALL;
				cmd.driver = card->myid;
				cmd.command = ISDN_STAT_ICALL;
				cmd.arg = chan;
				cmd.parm.setup.si1 = msg->msg.connect_ind.si1;
				cmd.parm.setup.si2 = msg->msg.connect_ind.si2;
				if (card->ptype == ISDN_PTYPE_EURO)
					strcpy(cmd.parm.setup.eazmsn,
					       act2000_find_eaz(card, msg->msg.connect_ind.eaz));
				else {
					cmd.parm.setup.eazmsn[0] = msg->msg.connect_ind.eaz;
					cmd.parm.setup.eazmsn[1] = 0;
				}
				memset(cmd.parm.setup.phone, 0, sizeof(cmd.parm.setup.phone));
				memcpy(cmd.parm.setup.phone, msg->msg.connect_ind.addr.num,
				       msg->msg.connect_ind.addr.len - 1);
				cmd.parm.setup.plan = msg->msg.connect_ind.addr.tnp;
				cmd.parm.setup.screen = 0;
				if (card->interface.statcallb(&cmd) == 2)
					actcapi_connect_resp(card, &card->bch[chan], 0x15); /* Reject Call */
			}
			break;
		case 0x0302:
			/* CONNECT_ACTIVE_IND */
			chan = find_plci(card, msg->msg.connect_active_ind.plci);
			if (chan >= 0)
				switch (card->bch[chan].fsm_state) {
				case ACT2000_STATE_IWAIT:
					actcapi_connect_active_resp(card, &card->bch[chan]);
					break;
				case ACT2000_STATE_OWAIT:
					actcapi_connect_active_resp(card, &card->bch[chan]);
					actcapi_select_b2_protocol_req(card, &card->bch[chan]);
					break;
				}
			break;
		case 0x8202:
			/* CONNECT_B3_IND */
			chan = find_plci(card, msg->msg.connect_b3_ind.plci);
			if ((chan >= 0) && (card->bch[chan].fsm_state == ACT2000_STATE_IBWAIT)) {
				card->bch[chan].ncci = msg->msg.connect_b3_ind.ncci;
				actcapi_connect_b3_resp(card, &card->bch[chan], 0);
			} else {
				ctmp = (act2000_chan *)tmp;
				ctmp->ncci = msg->msg.connect_b3_ind.ncci;
				actcapi_connect_b3_resp(card, ctmp, 0x11); /* All Card-Cannels busy */
			}
			break;
		case 0x8302:
			/* CONNECT_B3_ACTIVE_IND */
			chan = find_ncci(card, msg->msg.connect_b3_active_ind.ncci);
			if ((chan >= 0) && (card->bch[chan].fsm_state == ACT2000_STATE_BWAIT)) {
				actcapi_connect_b3_active_resp(card, &card->bch[chan]);
				cmd.driver = card->myid;
				cmd.command = ISDN_STAT_BCONN;
				cmd.arg = chan;
				card->interface.statcallb(&cmd);
			}
			break;
		case 0x8402:
			/* DISCONNECT_B3_IND */
			chan = find_ncci(card, msg->msg.disconnect_b3_ind.ncci);
			if (chan >= 0) {
				ctmp = &card->bch[chan];
				actcapi_disconnect_b3_resp(card, ctmp);
				switch (ctmp->fsm_state) {
				case ACT2000_STATE_ACTIVE:
					ctmp->fsm_state = ACT2000_STATE_DHWAIT2;
					cmd.driver = card->myid;
					cmd.command = ISDN_STAT_BHUP;
					cmd.arg = chan;
					card->interface.statcallb(&cmd);
					break;
				case ACT2000_STATE_BHWAIT2:
					actcapi_disconnect_req(card, ctmp);
					ctmp->fsm_state = ACT2000_STATE_DHWAIT;
					cmd.driver = card->myid;
					cmd.command = ISDN_STAT_BHUP;
					cmd.arg = chan;
					card->interface.statcallb(&cmd);
					break;
				}
			}
			break;
		case 0x0402:
			/* DISCONNECT_IND */
			chan = find_plci(card, msg->msg.disconnect_ind.plci);
			if (chan >= 0) {
				ctmp = &card->bch[chan];
				actcapi_disconnect_resp(card, ctmp);
				ctmp->fsm_state = ACT2000_STATE_NULL;
				cmd.driver = card->myid;
				cmd.command = ISDN_STAT_DHUP;
				cmd.arg = chan;
				card->interface.statcallb(&cmd);
			} else {
				ctmp = (act2000_chan *)tmp;
				ctmp->plci = msg->msg.disconnect_ind.plci;
				actcapi_disconnect_resp(card, ctmp);
			}
			break;
		case 0x4001:
			/* SELECT_B2_PROTOCOL_CONF */
			chan = find_plci(card, msg->msg.select_b2_protocol_conf.plci);
			if (chan >= 0)
				switch (card->bch[chan].fsm_state) {
				case ACT2000_STATE_ICALL:
				case ACT2000_STATE_OWAIT:
					ctmp = &card->bch[chan];
					if (msg->msg.select_b2_protocol_conf.info == 0)
						actcapi_select_b3_protocol_req(card, ctmp);
					else {
						ctmp->fsm_state = ACT2000_STATE_NULL;
						cmd.driver = card->myid;
						cmd.command = ISDN_STAT_DHUP;
						cmd.arg = chan;
						card->interface.statcallb(&cmd);
					}
					break;
				}
			break;
		case 0x8001:
			/* SELECT_B3_PROTOCOL_CONF */
			chan = find_plci(card, msg->msg.select_b3_protocol_conf.plci);
			if (chan >= 0)
				switch (card->bch[chan].fsm_state) {
				case ACT2000_STATE_ICALL:
				case ACT2000_STATE_OWAIT:
					ctmp = &card->bch[chan];
					if (msg->msg.select_b3_protocol_conf.info == 0)
						actcapi_listen_b3_req(card, ctmp);
					else {
						ctmp->fsm_state = ACT2000_STATE_NULL;
						cmd.driver = card->myid;
						cmd.command = ISDN_STAT_DHUP;
						cmd.arg = chan;
						card->interface.statcallb(&cmd);
					}
				}
			break;
		case 0x8101:
			/* LISTEN_B3_CONF */
			chan = find_plci(card, msg->msg.listen_b3_conf.plci);
			if (chan >= 0)
				switch (card->bch[chan].fsm_state) {
				case ACT2000_STATE_ICALL:
					ctmp = &card->bch[chan];
					if (msg->msg.listen_b3_conf.info == 0)
						actcapi_connect_resp(card, ctmp, 0);
					else {
						ctmp->fsm_state = ACT2000_STATE_NULL;
						cmd.driver = card->myid;
						cmd.command = ISDN_STAT_DHUP;
						cmd.arg = chan;
						card->interface.statcallb(&cmd);
					}
					break;
				case ACT2000_STATE_OWAIT:
					ctmp = &card->bch[chan];
					if (msg->msg.listen_b3_conf.info == 0) {
						actcapi_connect_b3_req(card, ctmp);
						ctmp->fsm_state = ACT2000_STATE_OBWAIT;
						cmd.driver = card->myid;
						cmd.command = ISDN_STAT_DCONN;
						cmd.arg = chan;
						card->interface.statcallb(&cmd);
					} else {
						ctmp->fsm_state = ACT2000_STATE_NULL;
						cmd.driver = card->myid;
						cmd.command = ISDN_STAT_DHUP;
						cmd.arg = chan;
						card->interface.statcallb(&cmd);
					}
					break;
				}
			break;
		case 0x8201:
			/* CONNECT_B3_CONF */
			chan = find_plci(card, msg->msg.connect_b3_conf.plci);
			if ((chan >= 0) && (card->bch[chan].fsm_state == ACT2000_STATE_OBWAIT)) {
				ctmp = &card->bch[chan];
				if (msg->msg.connect_b3_conf.info) {
					ctmp->fsm_state = ACT2000_STATE_NULL;
					cmd.driver = card->myid;
					cmd.command = ISDN_STAT_DHUP;
					cmd.arg = chan;
					card->interface.statcallb(&cmd);
				} else {
					ctmp->ncci = msg->msg.connect_b3_conf.ncci;
					ctmp->fsm_state = ACT2000_STATE_BWAIT;
				}
			}
			break;
		case 0x8401:
			/* DISCONNECT_B3_CONF */
			chan = find_ncci(card, msg->msg.disconnect_b3_conf.ncci);
			if ((chan >= 0) && (card->bch[chan].fsm_state == ACT2000_STATE_BHWAIT))
				card->bch[chan].fsm_state = ACT2000_STATE_BHWAIT2;
			break;
		case 0x0702:
			/* INFO_IND */
			chan = find_plci(card, msg->msg.info_ind.plci);
			if (chan >= 0)
				/* TODO: Eval Charging info / cause */
				actcapi_info_resp(card, &card->bch[chan]);
			break;
		case 0x0401:
			/* LISTEN_CONF */
		case 0x0501:
			/* LISTEN_CONF */
		case 0xff01:
			/* MANUFACTURER_CONF */
			break;
		case 0xff02:
			/* MANUFACTURER_IND */
			if (msg->msg.manuf_msg == 3) {
				memset(tmp, 0, sizeof(tmp));
				strncpy(tmp,
					&msg->msg.manufacturer_ind_err.errstring,
					msg->hdr.len - 16);
				if (msg->msg.manufacturer_ind_err.errcode)
					printk(KERN_WARNING "act2000: %s\n", tmp);
				else {
					printk(KERN_DEBUG "act2000: %s\n", tmp);
					if ((!strncmp(tmp, "INFO: Trace buffer con", 22)) ||
					    (!strncmp(tmp, "INFO: Compile Date/Tim", 22))) {
						card->flags |= ACT2000_FLAGS_RUNNING;
						cmd.command = ISDN_STAT_RUN;
						cmd.driver = card->myid;
						cmd.arg = 0;
						actcapi_manufacturer_req_net(card);
						actcapi_manufacturer_req_msn(card);
						actcapi_listen_req(card);
						card->interface.statcallb(&cmd);
					}
				}
			}
			break;
		default:
			printk(KERN_WARNING "act2000: UNHANDLED Message %04x\n", ccmd);
			break;
		}
		dev_kfree_skb(skb);
	}
}

#ifdef DEBUG_MSG
static void
actcapi_debug_caddr(actcapi_addr *addr)
{
	char tmp[30];

	printk(KERN_DEBUG " Alen  = %d\n", addr->len);
	if (addr->len > 0)
		printk(KERN_DEBUG " Atnp  = 0x%02x\n", addr->tnp);
	if (addr->len > 1) {
		memset(tmp, 0, 30);
		memcpy(tmp, addr->num, addr->len - 1);
		printk(KERN_DEBUG " Anum  = '%s'\n", tmp);
	}
}

static void
actcapi_debug_ncpi(actcapi_ncpi *ncpi)
{
	printk(KERN_DEBUG " ncpi.len = %d\n", ncpi->len);
	if (ncpi->len >= 2)
		printk(KERN_DEBUG " ncpi.lic = 0x%04x\n", ncpi->lic);
	if (ncpi->len >= 4)
		printk(KERN_DEBUG " ncpi.hic = 0x%04x\n", ncpi->hic);
	if (ncpi->len >= 6)
		printk(KERN_DEBUG " ncpi.ltc = 0x%04x\n", ncpi->ltc);
	if (ncpi->len >= 8)
		printk(KERN_DEBUG " ncpi.htc = 0x%04x\n", ncpi->htc);
	if (ncpi->len >= 10)
		printk(KERN_DEBUG " ncpi.loc = 0x%04x\n", ncpi->loc);
	if (ncpi->len >= 12)
		printk(KERN_DEBUG " ncpi.hoc = 0x%04x\n", ncpi->hoc);
	if (ncpi->len >= 13)
		printk(KERN_DEBUG " ncpi.mod = %d\n", ncpi->modulo);
}

static void
actcapi_debug_dlpd(actcapi_dlpd *dlpd)
{
	printk(KERN_DEBUG " dlpd.len = %d\n", dlpd->len);
	if (dlpd->len >= 2)
		printk(KERN_DEBUG " dlpd.dlen   = 0x%04x\n", dlpd->dlen);
	if (dlpd->len >= 3)
		printk(KERN_DEBUG " dlpd.laa    = 0x%02x\n", dlpd->laa);
	if (dlpd->len >= 4)
		printk(KERN_DEBUG " dlpd.lab    = 0x%02x\n", dlpd->lab);
	if (dlpd->len >= 5)
		printk(KERN_DEBUG " dlpd.modulo = %d\n", dlpd->modulo);
	if (dlpd->len >= 6)
		printk(KERN_DEBUG " dlpd.win    = %d\n", dlpd->win);
}

#ifdef DEBUG_DUMP_SKB
static void dump_skb(struct sk_buff *skb) {
	char tmp[80];
	char *p = skb->data;
	char *t = tmp;
	int i;

	for (i = 0; i < skb->len; i++) {
		t += sprintf(t, "%02x ", *p++ & 0xff);
		if ((i & 0x0f) == 8) {
			printk(KERN_DEBUG "dump: %s\n", tmp);
			t = tmp;
		}
	}
	if (i & 0x07)
		printk(KERN_DEBUG "dump: %s\n", tmp);
}
#endif

void
actcapi_debug_msg(struct sk_buff *skb, int direction)
{
	actcapi_msg *msg = (actcapi_msg *)skb->data;
	char *descr;
	int i;
	char tmp[170];

#ifndef DEBUG_DATA_MSG
	if (msg->hdr.cmd.cmd == 0x86)
		return;
#endif
	descr = "INVALID";
#ifdef DEBUG_DUMP_SKB
	dump_skb(skb);
#endif
	for (i = 0; i < ARRAY_SIZE(valid_msg); i++)
		if ((msg->hdr.cmd.cmd == valid_msg[i].cmd.cmd) &&
		    (msg->hdr.cmd.subcmd == valid_msg[i].cmd.subcmd)) {
			descr = valid_msg[i].description;
			break;
		}
	printk(KERN_DEBUG "%s %s msg\n", direction ? "Outgoing" : "Incoming", descr);
	printk(KERN_DEBUG " ApplID = %d\n", msg->hdr.applicationID);
	printk(KERN_DEBUG " Len    = %d\n", msg->hdr.len);
	printk(KERN_DEBUG " MsgNum = 0x%04x\n", msg->hdr.msgnum);
	printk(KERN_DEBUG " Cmd    = 0x%02x\n", msg->hdr.cmd.cmd);
	printk(KERN_DEBUG " SubCmd = 0x%02x\n", msg->hdr.cmd.subcmd);
	switch (i) {
	case 0:
		/* DATA B3 IND */
		printk(KERN_DEBUG " BLOCK = 0x%02x\n",
		       msg->msg.data_b3_ind.blocknr);
		break;
	case 2:
		/* CONNECT CONF */
		printk(KERN_DEBUG " PLCI = 0x%04x\n",
		       msg->msg.connect_conf.plci);
		printk(KERN_DEBUG " Info = 0x%04x\n",
		       msg->msg.connect_conf.info);
		break;
	case 3:
		/* CONNECT IND */
		printk(KERN_DEBUG " PLCI = 0x%04x\n",
		       msg->msg.connect_ind.plci);
		printk(KERN_DEBUG " Contr = %d\n",
		       msg->msg.connect_ind.controller);
		printk(KERN_DEBUG " SI1   = %d\n",
		       msg->msg.connect_ind.si1);
		printk(KERN_DEBUG " SI2   = %d\n",
		       msg->msg.connect_ind.si2);
		printk(KERN_DEBUG " EAZ   = '%c'\n",
		       msg->msg.connect_ind.eaz);
		actcapi_debug_caddr(&msg->msg.connect_ind.addr);
		break;
	case 5:
		/* CONNECT ACTIVE IND */
		printk(KERN_DEBUG " PLCI = 0x%04x\n",
		       msg->msg.connect_active_ind.plci);
		actcapi_debug_caddr(&msg->msg.connect_active_ind.addr);
		break;
	case 8:
		/* LISTEN CONF */
		printk(KERN_DEBUG " Contr = %d\n",
		       msg->msg.listen_conf.controller);
		printk(KERN_DEBUG " Info = 0x%04x\n",
		       msg->msg.listen_conf.info);
		break;
	case 11:
		/* INFO IND */
		printk(KERN_DEBUG " PLCI = 0x%04x\n",
		       msg->msg.info_ind.plci);
		printk(KERN_DEBUG " Imsk = 0x%04x\n",
		       msg->msg.info_ind.nr.mask);
		if (msg->hdr.len > 12) {
			int l = msg->hdr.len - 12;
			int j;
			char *p = tmp;
			for (j = 0; j < l; j++)
				p += sprintf(p, "%02x ", msg->msg.info_ind.el.display[j]);
			printk(KERN_DEBUG " D = '%s'\n", tmp);
		}
		break;
	case 14:
		/* SELECT B2 PROTOCOL CONF */
		printk(KERN_DEBUG " PLCI = 0x%04x\n",
		       msg->msg.select_b2_protocol_conf.plci);
		printk(KERN_DEBUG " Info = 0x%04x\n",
		       msg->msg.select_b2_protocol_conf.info);
		break;
	case 15:
		/* SELECT B3 PROTOCOL CONF */
		printk(KERN_DEBUG " PLCI = 0x%04x\n",
		       msg->msg.select_b3_protocol_conf.plci);
		printk(KERN_DEBUG " Info = 0x%04x\n",
		       msg->msg.select_b3_protocol_conf.info);
		break;
	case 16:
		/* LISTEN B3 CONF */
		printk(KERN_DEBUG " PLCI = 0x%04x\n",
		       msg->msg.listen_b3_conf.plci);
		printk(KERN_DEBUG " Info = 0x%04x\n",
		       msg->msg.listen_b3_conf.info);
		break;
	case 18:
		/* CONNECT B3 IND */
		printk(KERN_DEBUG " NCCI = 0x%04x\n",
		       msg->msg.connect_b3_ind.ncci);
		printk(KERN_DEBUG " PLCI = 0x%04x\n",
		       msg->msg.connect_b3_ind.plci);
		actcapi_debug_ncpi(&msg->msg.connect_b3_ind.ncpi);
		break;
	case 19:
		/* CONNECT B3 ACTIVE IND */
		printk(KERN_DEBUG " NCCI = 0x%04x\n",
		       msg->msg.connect_b3_active_ind.ncci);
		actcapi_debug_ncpi(&msg->msg.connect_b3_active_ind.ncpi);
		break;
	case 26:
		/* MANUFACTURER IND */
		printk(KERN_DEBUG " Mmsg = 0x%02x\n",
		       msg->msg.manufacturer_ind_err.manuf_msg);
		switch (msg->msg.manufacturer_ind_err.manuf_msg) {
		case 3:
			printk(KERN_DEBUG " Contr = %d\n",
			       msg->msg.manufacturer_ind_err.controller);
			printk(KERN_DEBUG " Code = 0x%08x\n",
			       msg->msg.manufacturer_ind_err.errcode);
			memset(tmp, 0, sizeof(tmp));
			strncpy(tmp, &msg->msg.manufacturer_ind_err.errstring,
				msg->hdr.len - 16);
			printk(KERN_DEBUG " Emsg = '%s'\n", tmp);
			break;
		}
		break;
	case 30:
		/* LISTEN REQ */
		printk(KERN_DEBUG " Imsk = 0x%08x\n",
		       msg->msg.listen_req.infomask);
		printk(KERN_DEBUG " Emsk = 0x%04x\n",
		       msg->msg.listen_req.eazmask);
		printk(KERN_DEBUG " Smsk = 0x%04x\n",
		       msg->msg.listen_req.simask);
		break;
	case 35:
		/* SELECT_B2_PROTOCOL_REQ */
		printk(KERN_DEBUG " PLCI  = 0x%04x\n",
		       msg->msg.select_b2_protocol_req.plci);
		printk(KERN_DEBUG " prot  = 0x%02x\n",
		       msg->msg.select_b2_protocol_req.protocol);
		if (msg->hdr.len >= 11)
			printk(KERN_DEBUG "No dlpd\n");
		else
			actcapi_debug_dlpd(&msg->msg.select_b2_protocol_req.dlpd);
		break;
	case 44:
		/* CONNECT RESP */
		printk(KERN_DEBUG " PLCI  = 0x%04x\n",
		       msg->msg.connect_resp.plci);
		printk(KERN_DEBUG " CAUSE = 0x%02x\n",
		       msg->msg.connect_resp.rejectcause);
		break;
	case 45:
		/* CONNECT ACTIVE RESP */
		printk(KERN_DEBUG " PLCI  = 0x%04x\n",
		       msg->msg.connect_active_resp.plci);
		break;
	}
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /* $Id: capi.h,v 1.6.6.2 2001/09/23 22:24:32 kai Exp $
 *
 * ISDN lowlevel-module for the IBM ISDN-S0 Active 2000.
 *
 * Author       Fritz Elfert
 * Copyright    by Fritz Elfert      <fritz@isdn4linux.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Thanks to Friedemann Baitinger and IBM Germany
 *
 */

#ifndef CAPI_H
#define CAPI_H

/* Command-part of a CAPI message */
typedef struct actcapi_msgcmd {
	__u8 cmd;
	__u8 subcmd;
} actcapi_msgcmd;

/* CAPI message header */
typedef struct actcapi_msghdr {
	__u16 len;
	__u16 applicationID;
	actcapi_msgcmd cmd;
	__u16 msgnum;
} actcapi_msghdr;

/* CAPI message description (for debugging) */
typedef struct actcapi_msgdsc {
	actcapi_msgcmd cmd;
	char *description;
} actcapi_msgdsc;

/* CAPI Address */
typedef struct actcapi_addr {
	__u8 len;                            /* Length of element            */
	__u8 tnp;                            /* Type/Numbering Plan          */
	__u8 num[20];                        /* Caller ID                    */
} actcapi_addr;

/* CAPI INFO element mask */
typedef  union actcapi_infonr {              /* info number                  */
	__u16 mask;                          /* info-mask field              */
	struct bmask {                       /* bit definitions              */
		unsigned  codes:3;           /* code set                     */
		unsigned  rsvd:5;            /* reserved                     */
		unsigned  svind:1;           /* single, variable length ind. */
		unsigned  wtype:7;           /* W-element type               */
	} bmask;
} actcapi_infonr;

/* CAPI INFO element */
typedef union  actcapi_infoel {              /* info element                 */
	__u8 len;                            /* length of info element       */
	__u8 display[40];                    /* display contents             */
	__u8 uuinfo[40];                     /* User-user info field         */
	struct cause {                       /* Cause information            */
		unsigned ext2:1;             /* extension                    */
		unsigned cod:2;              /* coding standard              */
		unsigned spare:1;            /* spare                        */
		unsigned loc:4;              /* location                     */
		unsigned ext1:1;             /* extension                    */
		unsigned cval:7;             /* Cause value                  */
	} cause;
	struct charge {                      /* Charging information         */
		__u8 toc;                    /* type of charging info        */
		__u8 unit[10];               /* charging units               */
	} charge;
	__u8 date[20];                       /* date fields                  */
	__u8 stat;                           /* state of remote party        */
} actcapi_infoel;

/* Message for EAZ<->MSN Mapping */
typedef struct actcapi_msn {
	__u8 eaz;
	__u8 len;                            /* Length of MSN                */
	__u8 msn[15];
}  __attribute__((packed)) actcapi_msn;

typedef struct actcapi_dlpd {
	__u8 len;                            /* Length of structure          */
	__u16 dlen;                          /* Data Length                  */
	__u8 laa;                            /* Link Address A               */
	__u8 lab;                            /* Link Address B               */
	__u8 modulo;                         /* Modulo Mode                  */
	__u8 win;                            /* Window size                  */
	__u8 xid[100];                       /* XID Information              */
} __attribute__((packed)) actcapi_dlpd;

typedef struct actcapi_ncpd {
	__u8   len;                          /* Length of structure          */
	__u16  lic;
	__u16  hic;
	__u16  ltc;
	__u16  htc;
	__u16  loc;
	__u16  hoc;
	__u8   modulo;
} __attribute__((packed)) actcapi_ncpd;
#define actcapi_ncpi actcapi_ncpd

/*
 * Layout of NCCI field in a B3 DATA CAPI message is different from
 * standard at act2000:
 *
 * Bit 0-4  = PLCI
 * Bit 5-7  = Controller
 * Bit 8-15 = NCCI
 */
#define MAKE_NCCI(plci, contr, ncci)					\
	((plci & 0x1f) | ((contr & 0x7) << 5) | ((ncci & 0xff) << 8))

#define EVAL_NCCI(fakencci, plci, contr, ncci) {	\
		plci  = fakencci & 0x1f;		\
		contr = (fakencci >> 5) & 0x7;		\
		ncci  = (fakencci >> 8) & 0xff;		\
	}

/*
 * Layout of PLCI field in a B3 DATA CAPI message is different from
 * standard at act2000:
 *
 * Bit 0-4  = PLCI
 * Bit 5-7  = Controller
 * Bit 8-15 = reserved (must be 0)
 */
#define MAKE_PLCI(plci, contr)			\
	((plci & 0x1f) | ((contr & 0x7) << 5))

#define EVAL_PLCI(fakeplci, plci, contr) {	\
		plci  = fakeplci & 0x1f;	\
		contr = (fakeplci >> 5) & 0x7;	\
	}

typedef struct actcapi_msg {
	actcapi_msghdr hdr;
	union {
		__u16 manuf_msg;
		struct manufacturer_req_net {
			__u16 manuf_msg;
			__u16 controller;
			__u8  nettype;
		} manufacturer_req_net;
		struct manufacturer_req_v42 {
			__u16 manuf_msg;
			__u16 controller;
			__u32 v42control;
		} manufacturer_req_v42;
		struct manufacturer_conf_v42 {
			__u16 manuf_msg;
			__u16 controller;
		} manufacturer_conf_v42;
		struct manufacturer_req_err {
			__u16 manuf_msg;
			__u16 controller;
		} manufacturer_req_err;
		struct manufacturer_ind_err {
			__u16 manuf_msg;
			__u16 controller;
			__u32 errcode;
			__u8  errstring; /* actually up to 160 */
		} manufacturer_ind_err;
		struct manufacturer_req_msn {
			__u16 manuf_msg;
			__u16 controller;
			actcapi_msn msnmap;
		} __attribute ((packed)) manufacturer_req_msn;
		/* TODO: TraceInit-req/conf/ind/resp and
		 *       TraceDump-req/conf/ind/resp
		 */
		struct connect_req {
			__u8  controller;
			__u8  bchan;
			__u32 infomask;
			__u8  si1;
			__u8  si2;
			__u8  eaz;
			actcapi_addr addr;
		} __attribute__ ((packed)) connect_req;
		struct connect_conf {
			__u16 plci;
			__u16 info;
		} connect_conf;
		struct connect_ind {
			__u16 plci;
			__u8  controller;
			__u8  si1;
			__u8  si2;
			__u8  eaz;
			actcapi_addr addr;
		} __attribute__ ((packed)) connect_ind;
		struct connect_resp {
			__u16 plci;
			__u8  rejectcause;
		} connect_resp;
		struct connect_active_ind {
			__u16 plci;
			actcapi_addr addr;
		} __attribute__ ((packed)) connect_active_ind;
		struct connect_active_resp {
			__u16 plci;
		} connect_active_resp;
		struct connect_b3_req {
			__u16 plci;
			actcapi_ncpi ncpi;
		} __attribute__ ((packed)) connect_b3_req;
		struct connect_b3_conf {
			__u16 plci;
			__u16 ncci;
			__u16 info;
		} connect_b3_conf;
		struct connect_b3_ind {
			__u16 ncci;
			__u16 plci;
			actcapi_ncpi ncpi;
		} __attribute__ ((packed)) connect_b3_ind;
		struct connect_b3_resp {
			__u16 ncci;
			__u8  rejectcause;
			actcapi_ncpi ncpi;
		} __attribute__ ((packed)) connect_b3_resp;
		struct disconnect_req {
			__u16 plci;
			__u8  cause;
		} disconnect_req;
		struct disconnect_conf {
			__u16 plci;
			__u16 info;
		} disconnect_conf;
		struct disconnect_ind {
			__u16 plci;
			__u16 info;
		} disconnect_ind;
		struct disconnect_resp {
			__u16 plci;
		} disconnect_resp;
		struct connect_b3_active_ind {
			__u16 ncci;
			actcapi_ncpi ncpi;
		} __attribute__ ((packed)) connect_b3_active_ind;
		struct connect_b3_active_resp {
			__u16 ncci;
		} connect_b3_active_resp;
		struct disconnect_b3_req {
			__u16 ncci;
			actcapi_ncpi ncpi;
		} __attribute__ ((packed)) disconnect_b3_req;
		struct disconnect_b3_conf {
			__u16 ncci;
			__u16 info;
		} disconnect_b3_conf;
		struct disconnect_b3_ind {
			__u16 ncci;
			__u16 info;
			actcapi_ncpi ncpi;
		} __attribute__ ((packed)) disconnect_b3_ind;
		struct disconnect_b3_resp {
			__u16 ncci;
		} disconnect_b3_resp;
		struct info_ind {
			__u16 plci;
			actcapi_infonr nr;
			actcapi_infoel el;
		} __attribute__ ((packed)) info_ind;
		struct info_resp {
			__u16 plci;
		} info_resp;
		struct listen_b3_req {
			__u16 plci;
		} listen_b3_req;
		struct listen_b3_conf {
			__u16 plci;
			__u16 info;
		} listen_b3_conf;
		struct select_b2_protocol_req {
			__u16 plci;
			__u8  protocol;
			actcapi_dlpd dlpd;
		} __attribute__ ((packed)) select_b2_protocol_req;
		struct select_b2_protocol_conf {
			__u16 plci;
			__u16 info;
		} select_b2_protocol_conf;
		struct select_b3_protocol_req {
			__u16 plci;
			__u8  protocol;
			actcapi_ncpd ncpd;
		} __attribute__ ((packed)) select_b3_protocol_req;
		struct select_b3_protocol_conf {
			__u16 plci;
			__u16 info;
		} select_b3_protocol_conf;
		struct listen_req {
			__u8  controller;
			__u32 infomask;
			__u16 eazmask;
			__u16 simask;
		} __attribute__ ((packed)) listen_req;
		struct listen_conf {
			__u8  controller;
			__u16 info;
		} __attribute__ ((packed)) listen_conf;
		struct data_b3_req {
			__u16 fakencci;
			__u16 datalen;
			__u32 unused;
			__u8  blocknr;
			__u16 flags;
		} __attribute ((packed)) data_b3_req;
		struct data_b3_ind {
			__u16 fakencci;
			__u16 datalen;
			__u32 unused;
			__u8  blocknr;
			__u16 flags;
		} __attribute__ ((packed)) data_b3_ind;
		struct data_b3_resp {
			__u16 ncci;
			__u8  blocknr;
		} __attribute__ ((packed)) data_b3_resp;
		struct data_b3_conf {
			__u16 ncci;
			__u8  blocknr;
			__u16 info;
		} __attribute__ ((packed)) data_b3_conf;
	} msg;
} __attribute__ ((packed)) actcapi_msg;

static inline unsigned short
actcapi_nextsmsg(act2000_card *card)
{
	unsigned long flags;
	unsigned short n;

	spin_lock_irqsave(&card->mnlock, flags);
	n = card->msgnum;
	card->msgnum++;
	card->msgnum &= 0x7fff;
	spin_unlock_irqrestore(&card->mnlock, flags);
	return n;
}
#define DEBUG_MSG
#undef DEBUG_DATA_MSG
#undef DEBUG_DUMP_SKB

extern int actcapi_chkhdr(act2000_card *, actcapi_msghdr *);
extern int actcapi_listen_req(act2000_card *);
extern int actcapi_manufacturer_req_net(act2000_card *);
extern int actcapi_manufacturer_req_errh(act2000_card *);
extern int actcapi_manufacturer_req_msn(act2000_card *);
extern int actcapi_connect_req(act2000_card *, act2000_chan *, char *, char, int, int);
extern void actcapi_select_b2_protocol_req(act2000_card *, act2000_chan *);
extern void actcapi_disconnect_b3_req(act2000_card *, act2000_chan *);
extern void actcapi_connect_resp(act2000_card *, act2000_chan *, __u8);
extern void actcapi_dispatch(struct work_struct *);
#ifdef DEBUG_MSG
extern void actcapi_debug_msg(struct sk_buff *skb, int);
#else
#define actcapi_debug_msg(skb, len)
#endif
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /* $Id: module.c,v 1.14.6.4 2001/09/23 22:24:32 kai Exp $
 *
 * ISDN lowlevel-module for the IBM ISDN-S0 Active 2000.
 *
 * Author       Fritz Elfert
 * Copyright    by Fritz Elfert      <fritz@isdn4linux.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Thanks to Friedemann Baitinger and IBM Germany
 *
 */

#include "act2000.h"
#include "act2000_isa.h"
#include "capi.h"
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

static unsigned short act2000_isa_ports[] =
{
	0x0200, 0x0240, 0x0280, 0x02c0, 0x0300, 0x0340, 0x0380,
	0xcfe0, 0xcfa0, 0xcf60, 0xcf20, 0xcee0, 0xcea0, 0xce60,
};

static act2000_card *cards = (act2000_card *) NULL;

/* Parameters to be set by insmod */
static int   act_bus  =  0;
static int   act_port = -1;  /* -1 = Autoprobe  */
static int   act_irq  = -1;
static char *act_id   = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

MODULE_DESCRIPTION("ISDN4Linux: Driver for IBM Active 2000 ISDN card");
MODULE_AUTHOR("Fritz Elfert");
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(act_bus, "BusType of first card, 1=ISA, 2=MCA, 3=PCMCIA, currently only ISA");
MODULE_PARM_DESC(membase, "Base port address of first card");
MODULE_PARM_DESC(act_irq, "IRQ of first card");
MODULE_PARM_DESC(act_id, "ID-String of first card");
module_param(act_bus, int, 0);
module_param(act_port, int, 0);
module_param(act_irq, int, 0);
module_param(act_id, charp, 0);

static int act2000_addcard(int, int, int, char *);

static act2000_chan *
find_channel(act2000_card *card, int channel)
{
	if ((channel >= 0) && (channel < ACT2000_BCH))
		return &(card->bch[channel]);
	printk(KERN_WARNING "act2000: Invalid channel %d\n", channel);
	return NULL;
}

/*
 * Free MSN list
 */
static void
act2000_clear_msn(act2000_card *card)
{
	struct msn_entry *p = card->msn_list;
	struct msn_entry *q;
	unsigned long flags;

	spin_lock_irqsave(&card->lock, flags);
	card->msn_list = NULL;
	spin_unlock_irqrestore(&card->lock, flags);
	while (p) {
		q  = p->next;
		kfree(p);
		p = q;
	}
}

/*
 * Find an MSN entry in the list.
 * If ia5 != 0, return IA5-encoded EAZ, else
 * return a bitmask with corresponding bit set.
 */
static __u16
act2000_find_msn(act2000_card *card, char *msn, int ia5)
{
	struct msn_entry *p = card->msn_list;
	__u8 eaz = '0';

	while (p) {
		if (!strcmp(p->msn, msn)) {
			eaz = p->eaz;
			break;
		}
		p = p->next;
	}
	if (!ia5)
		return (1 << (eaz - '0'));
	else
		return eaz;
}

/*
 * Find an EAZ entry in the list.
 * return a string with corresponding msn.
 */
char *
act2000_find_eaz(act2000_card *card, char eaz)
{
	struct msn_entry *p = card->msn_list;

	while (p) {
		if (p->eaz == eaz)
			return (p->msn);
		p = p->next;
	}
	return ("\0");
}

/*
 * Add or delete an MSN to the MSN list
 *
 * First character of msneaz is EAZ, rest is MSN.
 * If length of eazmsn is 1, delete that entry.
 */
static int
act2000_set_msn(act2000_card *card, char *eazmsn)
{
	struct msn_entry *p = card->msn_list;
	struct msn_entry *q = NULL;
	unsigned long flags;
	int i;

	if (!strlen(eazmsn))
		return 0;
	if (strlen(eazmsn) > 16)
		return -EINVAL;
	for (i = 0; i < strlen(eazmsn); i++)
		if (!isdigit(eazmsn[i]))
			return -EINVAL;
	if (strlen(eazmsn) == 1) {
		/* Delete a single MSN */
		while (p) {
			if (p->eaz == eazmsn[0]) {
				spin_lock_irqsave(&card->lock, flags);
				if (q)
					q->next = p->next;
				else
					card->msn_list = p->next;
				spin_unlock_irqrestore(&card->lock, flags);
				kfree(p);
				printk(KERN_DEBUG
				       "Mapping for EAZ %c deleted\n",
				       eazmsn[0]);
				return 0;
			}
			q = p;
			p = p->next;
		}
		return 0;
	}
	/* Add a single MSN */
	while (p) {
		/* Found in list, replace MSN */
		if (p->eaz == eazmsn[0]) {
			spin_lock_irqsave(&card->lock, flags);
			strcpy(p->msn, &eazmsn[1]);
			spin_unlock_irqrestore(&card->lock, flags);
			printk(KERN_DEBUG
			       "Mapping for EAZ %c changed to %s\n",
			       eazmsn[0],
			       &eazmsn[1]);
			return 0;
		}
		p = p->next;
	}
	/* Not found in list, add new entry */
	p = kmalloc(sizeof(msn_entry), GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	p->eaz = eazmsn[0];
	strcpy(p->msn, &eazmsn[1]);
	p->next = card->msn_list;
	spin_lock_irqsave(&card->lock, flags);
	card->msn_list = p;
	spin_unlock_irqrestore(&card->lock, flags);
	printk(KERN_DEBUG
	       "Mapping %c -> %s added\n",
	       eazmsn[0],
	       &eazmsn[1]);
	return 0;
}

static void
act2000_transmit(struct work_struct *work)
{
	struct act2000_card *card =
		container_of(work, struct act2000_card, snd_tq);

	switch (card->bus) {
	case ACT2000_BUS_ISA:
		act2000_isa_send(card);
		break;
	case ACT2000_BUS_PCMCIA:
	case ACT2000_BUS_MCA:
	default:
		printk(KERN_WARNING
		       "act2000_transmit: Illegal bustype %d\n", card->bus);
	}
}

static void
act2000_receive(struct work_struct *work)
{
	struct act2000_card *card =
		container_of(work, struct act2000_card, poll_tq);

	switch (card->bus) {
	case ACT2000_BUS_ISA:
		act2000_isa_receive(card);
		break;
	case ACT2000_BUS_PCMCIA:
	case ACT2000_BUS_MCA:
	default:
		printk(KERN_WARNING
		       "act2000_receive: Illegal bustype %d\n", card->bus);
	}
}

static void
act2000_poll(unsigned long data)
{
	act2000_card *card = (act2000_card *)data;
	unsigned long flags;

	act2000_receive(&card->poll_tq);
	spin_lock_irqsave(&card->lock, flags);
	mod_timer(&card->ptimer, jiffies + 3);
	spin_unlock_irqrestore(&card->lock, flags);
}

static int
act2000_command(act2000_card *card, isdn_ctrl *c)
{
	ulong a;
	act2000_chan *chan;
	act2000_cdef cdef;
	isdn_ctrl cmd;
	char tmp[17];
	int ret;
	unsigned long flags;
	void __user *arg;

	switch (c->command) {
	case ISDN_CMD_IOCTL:
		memcpy(&a, c->parm.num, sizeof(ulong));
	