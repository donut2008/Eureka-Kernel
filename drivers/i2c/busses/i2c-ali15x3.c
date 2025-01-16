ort, "QSFP failed even retrying\n");
	else if (pass)
		qib_dev_porterr(dd, ppd->port, "QSFP retries: %d\n", pass);

	msleep(20);

bail:
	mutex_unlock(&dd->eep_lock);

no_unlock:
	return ret;
}

/*
 * qsfp_write
 * We do not ordinarily write the QSFP, but this is needed to select
 * the page on non-flat QSFPs, and possibly later unusual cases
 */
static int qib_qsfp_write(struct qib_pportdata *ppd, int addr, void *bp,
			  int len)
{
	struct qib_devdata *dd = ppd->dd;
	u32 out, mask;
	int ret, cnt;
	u8 *buff = bp;

	ret = mutex_lock_interruptible(&dd->eep_lock);
	if (ret)
		goto no_unlock;

	if (dd->twsi_eeprom_dev == QIB_TWSI_NO_DEV) {
		ret = -ENXIO;
		goto bail;
	}

	/*
	 * We presume, if we are called at all, that this board has
	 * QSFP. This is on the same i2c chain as the legacy parts,
	 * but only responds if the module is selected via GPIO pins.
	 * Further, there are very long setup and hold requirements
	 * on MODSEL.
	 */
	mask = QSFP_GPIO_MOD_SEL_N | QSFP_GPIO_MOD_RST_N | QSFP_GPIO_LP_MODE;
	out = QSFP_GPIO_MOD_RST_N | QSFP_GPIO_LP_MODE;
	if (ppd->hw_pidx) {
		mask <<= QSFP_GPIO_PORT2_SHIFT;
		out <<= QSFP_GPIO_PORT2_SHIFT;
	}
	dd->f_gpio_mod(dd, out, mask, mask);

	/*
	 * Module could take up to 2 Msec to respond to MOD_SEL,
	 * and there is no way to tell if it is ready, so we must wait.
	 */
	msleep(20);

	/* Make sure TWSI bus is in sane state. */
	ret = qib_twsi_reset(dd);
	if (ret) {
		qib_dev_porterr(dd, ppd->port,
				"QSFP interface Reset for write failed\n");
		ret = -EIO;
		goto deselect;
	}

	/* All QSFP modules are at A0 */

	cnt = 0;
	while (cnt < len) {
		unsigned in_page;
		int wlen = len - cnt;

		in_page = addr % QSFP_PAGESIZE;
		if ((in_page + wlen) > QSFP_PAGESIZE)
			wlen = QSFP_PAGESIZE - in_page;
		ret = qib_twsi_blk_wr(dd, QSFP_DEV, addr, buff + cnt, wlen);
		if (ret) {
			/* qib_twsi_blk_wr() 1 for error, else 0 */
			ret = -EIO;
			goto deselect;
		}
		addr += wlen;
		cnt += wlen;
	}
	ret = cnt;

deselect:
	/*
	 * Module could take up to 10 uSec after transfer before
	 * ready to respond to MOD_SEL negation, and there is no way
	 * to tell if it is ready, so we must wait.
	 */
	udelay(10);
	/* set QSFP MODSEL, RST, LP high */
	dd->f_gpio_mod(dd, mask, mask, mask);
	/*
	 * Module could take up to 2 Msec to respond to MOD_SEL
	 * going away, and there is no way to tell if it is ready.
	 * so we must wait.
	 */
	msleep(20);

bail:
	mutex_unlock(&dd->eep_lock);

no_unlock:
	return ret;
}

/*
 * For validation, we want to check the checksums, even of the
 * fields we do not otherwise use. This function reads the bytes from
 * <first> to <next-1> and returns the 8lsbs of the sum, or <0 for errors
 */
static int qsfp_cks(struct qib_pportdata *ppd, int first, int next)
{
	int ret;
	u16 cks;
	u8 bval;

	cks = 0;
	while (first < next) {
		ret = qsfp_read(ppd, first, &bval, 1);
		if (ret < 0)
			goto bail;
		cks += bval;
		++first;
	}
	ret = cks & 0xFF;
bail:
	return ret;

}

int qib_refresh_qsfp_cache(struct qib_pportdata *ppd, struct qib_qsfp_cache *cp)
{
	int ret;
	int idx;
	u16 cks;
	u8 peek[4];

	/* ensure sane contents on invalid reads, for cable swaps */
	memset(cp, 0, sizeof(*cp));

	if (!qib_qsfp_mod_present(ppd)) {
		ret = -ENODEV;
		goto bail;
	}

	ret = qsfp_read(ppd, 0, peek, 3);
	if (ret < 0)
		goto bail;
	if ((peek[0] & 0xFE) != 0x0C)
		qib_dev_porterr(ppd->dd, ppd->port,
				"QSFP byte0 is 0x%02X, S/B 0x0C/D\n", peek[0]);

	if ((peek[2] & 4) == 0) {
		/*
		 * If cable is paged, rather than "flat memory", we need to
		 * set the page to zero, Even if it already appears to be zero.
		 */
		u8 poke = 0;

		ret = qib_qsfp_write(ppd, 127, &poke, 1);
		udelay(50);
		if (ret != 1) {
			qib_dev_porterr(ppd->dd, ppd->port,
					"Failed QSFP Page set\n");
			goto bail;
		}
	}

	ret = qsfp_read(ppd, QSFP_MOD_ID_OFFS, &cp->id, 1);
	if (ret < 0)
		goto bail;
	if ((cp->id & 0xFE) != 0x0C)
		qib_dev_porterr(ppd->dd, ppd->port,
				"QSFP ID byte is 0x%02X, S/B 0x0C/D\n", cp->id);
	cks = cp->id;

	ret = qsfp_read(ppd, QSFP_MOD_PWR_OFFS, &cp->pwr, 1);
	if (ret < 0)
		goto bail;
	cks += cp->pwr;

	ret = qsfp_cks(ppd, QSFP_MOD_PWR_OFFS + 1, QSFP_MOD_LEN_OFFS);
	if (ret < 0)
		goto bail;
	cks += ret;

	ret = qsfp_read(ppd, QSFP_MOD_LEN_OFFS, &cp->len, 1);
	if (ret < 0)
		goto bail;
	cks += cp->len;

	ret = qsfp_read(ppd, QSFP_MOD_TECH_OFFS, &cp->tech, 1);
	if (ret < 0)
		goto bail;
	cks += cp->tech;

	ret = qsfp_read(ppd, QSFP_VEND_OFFS, &cp->vendor, QSFP_VEND_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_VEND_LEN; ++idx)
		cks += cp->vendor[idx];

	ret = qsfp_read(ppd, QSFP_IBXCV_OFFS, &cp->xt_xcv, 1);
	if (ret < 0)
		goto bail;
	cks += cp->xt_xcv;

	ret = qsfp_read(ppd, QSFP_VOUI_OFFS, &cp->oui, QSFP_VOUI_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_VOUI_LEN; ++idx)
		cks += cp->oui[idx];

	ret = qsfp_read(ppd, QSFP_PN_OFFS, &cp->partnum, QSFP_PN_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_PN_LEN; ++idx)
		cks += cp->partnum[idx];

	ret = qsfp_read(ppd, QSFP_REV_OFFS, &cp->rev, QSFP_REV_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_REV_LEN; ++idx)
		cks += cp->rev[idx];

	ret = qsfp_read(ppd, QSFP_ATTEN_OFFS, &cp->atten, QSFP_ATTEN_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_ATTEN_LEN; ++idx)
		cks += cp->atten[idx];

	ret = qsfp_cks(ppd, QSFP_ATTEN_OFFS + QSFP_ATTEN_LEN, QSFP_CC_OFFS);
	if (ret < 0)
		goto bail;
	cks += ret;

	cks &= 0xFF;
	ret = qsfp_read(ppd, QSFP_CC_OFFS, &cp->cks1, 1);
	if (ret < 0)
		goto bail;
	if (cks != cp->cks1)
		qib_dev_porterr(ppd->dd, ppd->port,
				"QSFP cks1 is %02X, computed %02X\n", cp->cks1,
				cks);

	/* Second checksum covers 192 to (serial, date, lot) */
	ret = qsfp_cks(ppd, QSFP_CC_OFFS + 1, QSFP_SN_OFFS);
	if (ret < 0)
		goto bail;
	cks = ret;

	ret = qsfp_read(ppd, QSFP_SN_OFFS, &cp->serial, QSFP_SN_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_SN_LEN; ++idx)
		cks += cp->serial[idx];

	ret = qsfp_read(ppd, QSFP_DATE_OFFS, &cp->date, QSFP_DATE_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_DATE_LEN; ++idx)
		cks += cp->date[idx];

	ret = qsfp_read(ppd, QSFP_LOT_OFFS, &cp->lot, QSFP_LOT_LEN);
	if (ret < 0)
		goto bail;
	for (idx = 0; idx < QSFP_LOT_LEN; ++idx)
		cks += cp->lot[idx];

	ret = qsfp_cks(ppd, QSFP_LOT_OFFS + QSFP_LOT_LEN, QSFP_CC_EXT_OFFS);
	if (ret < 0)
		goto bail;
	cks += ret;

	ret = qsfp_read(ppd, QSFP_CC_EXT_OFFS, &cp->cks2, 1);
	if (ret < 0)
		goto bail;
	cks &= 0xFF;
	if (cks != cp->cks2)
		qib_dev_porterr(ppd->dd, ppd->port,
				"QSFP cks2 is %02X, computed %02X\n", cp->cks2,
				cks);
	return 0;

bail:
	cp->id = 0;
	return ret;
}

const char * const qib_qsfp_devtech[16] = {
	"850nm VCSEL", "1310nm VCSEL", "1550nm VCSEL", "1310nm FP",
	"1310nm DFB", "1550nm DFB", "1310nm EML", "1550nm EML",
	"Cu Misc", "1490nm DFB", "Cu NoEq", "Cu Eq",
	"Undef", "Cu Active BothEq", "Cu FarEq", "Cu NearEq"
};

#define QSFP_DUMP_CHUNK 16 /* Holds longest string */
#define QSFP_DEFAULT_HDR_CNT 224

static const char *pwr_codes = "1.5W2.0W2.5W3.5W";

int qib_qsfp_mod_present(struct qib_pportdata *ppd)
{
	u32 mask;
	int ret;

	mask = QSFP_GPIO_MOD_PRS_N <<
		(ppd->hw_pidx * QSFP_GPIO_PORT2_SHIFT);
	ret = ppd->dd->f_gpio_mod(ppd->dd, 0, 0, 0);

	return !((ret & mask) >>
		 ((ppd->hw_pidx * QSFP_GPIO_PORT2_SHIFT) + 3));
}

/*
 * Initialize structures that control access to QSFP. Called once per port
 * on cards that support QSFP.
 */
void qib_qsfp_init(struct qib_qsfp_data *qd,
		   void (*fevent)(struct work_struct *))
{
	u32 mask, highs;

	struct qib_devdata *dd = qd->ppd->dd;

	/* Initialize work struct for later QSFP events */
	INIT_WORK(&qd->work, fevent);

	/*
	 * Later, we may want more validation. For now, just set up pins and
	 * blip reset. If module is present, call qib_refresh_qsfp_cache(),
	 * to do further init.
	 */
	mask = QSFP_GPIO_MOD_SEL_N | QSFP_GPIO_MOD_RST_N | QSFP_GPIO_LP_MODE;
	highs = mask - QSFP_GPIO_MOD_RST_N;
	if (qd->ppd->hw_pidx) {
		mask <<= QSFP_GPIO_PORT2_SHIFT;
		highs <<= QSFP_GPIO_PORT2_SHIFT;
	}
	dd->f_gpio_mod(dd, highs, mask, mask);
	udelay(20); /* Generous RST dwell */

	dd->f_gpio_mod(dd, mask, mask, mask);
}

void qib_qsfp_deinit(struct qib_qsfp_data *qd)
{
	/*
	 * There is nothing to do here for now.  our work is scheduled
	 * with queue_work(), and flush_workqueue() from remove_one
	 * will block until all work setup with queue_work()
	 * completes.
	 */
}

int qib_qsfp_dump(struct qib_pportdata *ppd, char *buf, int len)
{
	struct qib_qsfp_cache cd;
	u8 bin_buff[QSFP_DUMP_CHUNK];
	char lenstr[6];
	int sofar, ret;
	int bidx = 0;

	sofar = 0;
	ret = qib_refresh_qsfp_cache(ppd, &cd);
	if (ret < 0)
		goto bail;

	lenstr[0] = ' ';
	lenstr[1] = '\0';
	if (QSFP_IS_CU(cd.tech))
		sprintf(lenstr, "%dM ", cd.len);

	sofar += scnprintf(buf + sofar, len - sofar, "PWR:%.3sW\n", pwr_codes +
			   (QSFP_PWR(cd.pwr) * 4));

	sofar += scnprintf(buf + sofar, len - sofar, "TECH:%s%s\n", lenstr,
			   qib_qsfp_devtech[cd.tech >> 4]);

	sofar += scnprintf(buf + sofar, len - sofar, "Vendor:%.*s\n",
			   QSFP_VEND_LEN, cd.vendor);

	sofar += scnprintf(buf + sofar, len - sofar, "OUI:%06X\n",
			   QSFP_OUI(cd.oui));

	sofar += scnprintf(buf + sofar, len - sofar, "Part#:%.*s\n",
			   QSFP_PN_LEN, cd.partnum);
	sofar += scnprintf(buf + sofar, len - sofar, "Rev:%.*s\n",
			   QSFP_REV_LEN, cd.rev);
	if (QSFP_IS_CU(cd.tech))
		sofar += scnprintf(buf + sofar, len - sofar, "Atten:%d, %d\n",
				   QSFP_ATTEN_SDR(cd.atten),
				   QSFP_ATTEN_DDR(cd.atten));
	sofar += scnprintf(buf + sofar, len - sofar, "Serial:%.*s\n",
			   QSFP_SN_LEN, cd.serial);
	sofar += scnprintf(buf + sofar, len - sofar, "Date:%.*s\n",
			   QSFP_DATE_LEN, cd.date);
	sofar += scnprintf(buf + sofar, len - sofar, "Lot:%.*s\n",
			   QSFP_LOT_LEN, cd.lot);

	while (bidx < QSFP_DEFAULT_HDR_CNT) {
		int iidx;

		ret = qsfp_read(ppd, bidx, bin_buff, QSFP_DUMP_CHUNK);
		if (ret < 0)
			goto bail;
		for (iidx = 0; iidx < ret; ++iidx) {
			sofar += scnprintf(buf + sofar, len-sofar, " %02X",
				bin_buff[iidx]);
		}
		sofar += scnprintf(buf + sofar, len - sofar, "\n");
		bidx += QSFP_DUMP_CHUNK;
	}
	ret = sofar;
bail:
	return ret;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * Copyright (c) 2006, 2007, 2008, 2009 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* QSFP support common definitions, for ib_qib driver */

#define QSFP_DEV 0xA0
#define QSFP_PWR_LAG_MSEC 2000
#define QSFP_MODPRS_LAG_MSEC 20

/*
 * Below are masks for various QSFP signals, for Port 1.
 * Port2 equivalents are shifted by QSFP_GPIO_PORT2_SHIFT.
 * _N means asserted low
 */
#define QSFP_GPIO_MOD_SEL_N (4)
#define QSFP_GPIO_MOD_PRS_N (8)
#define QSFP_GPIO_INT_N (0x10)
#define QSFP_GPIO_MOD_RST_N (0x20)
#define QSFP_GPIO_LP_MODE (0x40)
#define QSFP_GPIO_PORT2_SHIFT 5

#define QSFP_PAGESIZE 128
/* Defined fields that QLogic requires of qualified cables */
/* Byte 0 is Identifier, not checked */
/* Byte 1 is reserved "status MSB" */
/* Byte 2 is "status LSB" We only care that D2 "Flat Mem" is set. */
/*
 * Rest of first 128 not used, although 127 is reserved for page select
 * if module is not "Flat memory".
 */
/* Byte 128 is Identifier: must be 0x0c for QSFP, or 0x0d for QSFP+ */
#define QSFP_MOD_ID_OFFS 128
/*
 * Byte 129 is "Extended Identifier". We only care about D7,D6: Power class
 *  0:1.5W, 1:2.0W, 2:2.5W, 3:3.5W
 */
#define QSFP_MOD_PWR_OFFS 129
/* Byte 130 is Connector type. Not QLogic req'd */
/* Bytes 131..138 are Transceiver types, bit maps for various tech, none IB */
/* Byte 139 is encoding. code 0x01 is 8b10b. Not QLogic req'd */
/* byte 140 is nominal bit-rate, in units of 100Mbits/sec Not QLogic req'd */
/* Byte 141 is Extended Rate Select. Not QLogic req'd */
/* Bytes 142..145 are lengths for various fiber types. Not QLogic req'd */
/* Byte 146 is length for Copper. Units of 1 meter */
#define QSFP_MOD_LEN_OFFS 146
/*
 * Byte 147 is Device technology. D0..3 not Qlogc req'd
 * D4..7 select from 15 choices, translated by table:
 */
#define QSFP_MOD_TECH_OFFS 147
extern con