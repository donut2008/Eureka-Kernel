e->label,
			      detail, e->other_detail, e->enable_per_layer_report,
			      e->page_frame_number, e->offset_in_page, e->grain);
	} else {
		snprintf(detail, sizeof(detail),
			"page:0x%lx offset:0x%lx grain:%ld",
			e->page_frame_number, e->offset_in_page, e->grain);

		edac_ue_error(mci, e->error_count, pos, e->msg, e->location, e->label,
			      detail, e->other_detail, e->enable_per_layer_report);
	}


}
EXPORT_SYMBOL_GPL(edac_raw_mc_handle_error);

/**
 * edac_mc_handle_error - reports a memory event to userspace
 *
 * @type:		severity of the error (CE/UE/Fatal)
 * @mci:		a struct mem_ctl_info pointer
 * @error_count:	Number of errors of the same type
 * @page_frame_number:	mem page where the error occurred
 * @offset_in_page:	offset of the error inside the page
 * @syndrome:		ECC syndrome
 * @top_layer:		Memory layer[0] position
 * @mid_layer:		Memory layer[1] position
 * @low_layer:		Memory layer[2] position
 * @msg:		Message meaningful to the end users that
 *			explains the event
 * @other_detail:	Technical details about the event that
 *			may help hardware manufacturers and
 *			EDAC developers to analyse the event
 */
void edac_mc_handle_error(const enum hw_event_mc_err_type type,
			  struct mem_ctl_info *mci,
			  const u16 error_count,
			  const unsigned long page_frame_number,
			  const unsigned long offset_in_page,
			  const unsigned long syndrome,
			  const int top_layer,
			  const int mid_layer,
			  const int low_layer,
			  const char *msg,
			  const char *other_detail)
{
	char *p;
	int row = -1, chan = -1;
	int pos[EDAC_MAX_LAYERS] = { top_layer, mid_layer, low_layer };
	int i, n_labels = 0;
	u8 grain_bits;
	struct edac_raw_error_desc *e = &mci->error_desc;

	edac_dbg(3, "MC%d\n", mci->mc_idx);

	/* Fills the error report buffer */
	memset(e, 0, sizeof (*e));
	e->error_count = error_count;
	e->top_layer = top_layer;
	e->mid_layer = mid_layer;
	e->low_layer = low_layer;
	e->page_frame_number = page_frame_number;
	e->offset_in_page = offset_in_page;
	e->syndrome = syndrome;
	e->msg = msg;
	e->other_detail = other_detail;

	/*
	 * Check if the event report is consistent and if the memory
	 * location is known. If it is known, enable_per_layer_report will be
	 * true, the DIMM(s) label info will be filled and the per-layer
	 * error counters will be incremented.
	 */
	for (i = 0; i < mci->n_layers; i++) {
		if (pos[i] >= (int)mci->layers[i].size) {

			edac_mc_printk(mci, KERN_ERR,
				       "INTERNAL ERROR: %s value is out of range (%d >= %d)\n",
				       edac_layer_name[mci->layers[i].type],
				       pos[i], mci->layers[i].size);
			/*
			 * Instead of just returning it, let's use what's
			 * known about the error. The increment routines and
			 * the DIMM filter logic will do the right thing by
			 * pointing the likely damaged DIMMs.
			 */
			pos[i] = -1;
		}
		if (pos[i] >= 0)
			e->enable_per_layer_report = true;
	}

	/*
	 * Get the dimm label/grain that applies to the match criteria.
	 * As the error algorithm may not be able to point to just one memory
	 * stick, the logic here will get all possible labels that could
	 * pottentially be affected by the error.
	 * On FB-DIMM memory controllers, for uncorrected errors, it is common
	 * to have only the MC channel and the MC dimm (also called "branch")
	 * but the channel is not known, as the memory is arranged in pairs,
	 * where each memory belongs to a separate channel within the same
	 * branch.
	 */
	p = e->label;
	*p = '\0';

	for (i = 0; i < mci->tot_dimms; i++) {
		struct dimm_info *dimm = mci->dimms[i];

		if (top_layer >= 0 && top_layer != dimm->location[0])
			continue;
		if (mid_layer >= 0 && mid_layer != dimm->location[1])
			continue;
		if (low_layer >= 0 && low_layer != dimm->location[2])
			continue;

		/* get the max grain, over the error match range */
		if (dimm->grain > e->grain)
			e->grain = dimm->grain;

		/*
		 * If the error is memory-controller wide, there's no need to
		 * seek for the affected DIMMs because the whole
		 * channel/memory controller/...  may be affected.
		 * Also, don't show errors for empty DIMM slots.
		 */
		if (e->enable_per_layer_report && dimm->nr_pages) {
			if (n_labels >= EDAC_MAX_LABELS) {
				e->enable_per_layer_report = false;
				break;
			}
			n_labels++;
			if (p != e->label) {
				strcpy(p, OTHER_LABEL);
				p += strlen(OTHER_LABEL);
			}
			strcpy(p, dimm->label);
			p += strlen(p);
			*p = '\0';

			/*
			 * get csrow/channel of the DIMM, in order to allow
			 * incrementing the compat API counters
			 */
			edac_dbg(4, "%s csrows map: (%d,%d)\n",
				 mci->csbased ? "rank" : "dimm",
				 dimm->csrow, dimm->cschannel);
			if (row == -1)
				row = dimm->csrow;
			else if (row >= 0 && row != dimm->csrow)
				row = -2;

			if (chan == -1)
				chan = dimm->cschannel;
			else if (chan >= 0 && chan != dimm->cschannel)
				chan = -2;
		}
	}

	if (!e->enable_per_layer_report) {
		strcpy(e->label, "any memory");
	} else {
		edac_dbg(4, "csrow/channel to increment: (%d,%d)\n", row, chan);
		if (p == e->label)
			strcpy(e->label, "unknown memory");
		if (type == HW_EVENT_ERR_CORRECTED) {
			if (row >= 0) {
				mci->csrows[row]->ce_count += error_count;
				if (chan >= 0)
					mci->csrows[row]->channels[chan]->ce_count += error_count;
			}
		} else
			if (row >= 0)
				mci->csrows[row]->ue_count += error_count;
	}

	/* Fill the RAM location data */
	p = e->location;

	for (i = 0; i < mci->n_layers; i++) {
		if (pos[i] < 0)
			continue;

		p += sprintf(p, "%s:%d ",
			     edac_layer_name[mci->layers[i].type],
			     pos[i]);
	}
	if (p > e->location)
		*(p - 1) = '\0';

	/* Report the error via the trace interface */
	grain_bits = fls_long(e->grain) + 1;
	trace_mc_event(type, e->msg, e->label, e->error_count,
		       mci->mc_idx, e->top_layer, e->mid_layer, e->low_layer,
		       (e->page_frame_number << PAGE_SHIFT) | e->offset_in_page,
		       grain_bits, e->syndrome, e->other_detail);

	edac_raw_mc_handle_error(type, mci, e);
}
EXPORT_SYMBOL_GPL(edac_mc_handle_error);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 * edac_mc kernel module
 * (C) 2005-2007 Linux Networx (http://lnxi.com)
 *
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * Written Doug Thompson <norsk5@xmission.com> www.softwarebitmaker.com
 *
 * (c) 2012-2013 - Mauro Carvalho Chehab
 *	The entire API were re-written, and ported to use struct device
 *
 */

#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/edac.h>
#include <linux/bug.h>
#include <linux/pm_runtime.h>
#include <linux/uaccess.h>

#include "edac_core.h"
#include "edac_module.h"

/* MC EDAC Controls, setable by module parameter, and sysfs */
static int edac_mc_log_ue = 1;
static int edac_mc_log_ce = 1;
static int edac_mc_panic_on_ue;
static unsigned int edac_mc_poll_msec = 1000;

/* Getter functions for above */
int edac_mc_get_log_ue(void)
{
	return edac_mc_log_ue;
}

int edac_mc_get_log_ce(void)
{
	return edac_mc_log_ce;
}

int edac_mc_get_panic_on_ue(void)
{
	return edac_mc_panic_on_ue;
}

/* this is temporary */
unsigned int edac_mc_get_poll_msec(void)
{
	return edac_mc_poll_msec;
}

static int edac_set_poll_msec(const char *val, struct kernel_param *kp)
{
	unsigned int i;
	int ret;

	if (!val)
		return -EINVAL;

	ret = kstrtouint(val, 0, &i);
	if (ret)
		return ret;

	if (i < 1000)
		return -EINVAL;

	*((unsigned int *)kp->arg) = i;

	/* notify edac_mc engine to reset the poll period */
	edac_mc_reset_delay_period(i);

	return 0;
}

/* Parameter declarations for above */
module_param(edac_mc_panic_on_ue, int, 0644);
MODULE_PARM_DESC(edac_mc_panic_on_ue, "Panic on uncorrected error: 0=off 1=on");
module_param(edac_mc_log_ue, int, 0644);
MODULE_PARM_DESC(edac_mc_log_ue,
		 "Log uncorrectable error to console: 0=off 1=on");
module_param(edac_mc_log_ce, int, 0644);
MODULE_PARM_DESC(edac_mc_log_ce,
		 "Log correctable error to console: 0=off 1=on");
module_param_call(edac_mc_poll_msec, edac_set_poll_msec, param_get_uint,
		  &edac_mc_poll_msec, 0644);
MODULE_PARM_DESC(edac_mc_poll_msec, "Polling period in milliseconds");

static struct device *mci_pdev;

/*
 * various constants for Memory Controllers
 */
static const char * const mem_types[] = {
	[MEM_EMPTY] = "Empty",
	[MEM_RESERVED] = "Reserved",
	[MEM_UNKNOWN] = "Unknown",
	[MEM_FPM] = "FPM",
	[MEM_EDO] = "EDO",
	[MEM_BEDO] = "BEDO",
	[MEM_SDR] = "Unbuffered-SDR",
	[MEM_RDR] = "Registered-SDR",
	[MEM_DDR] = "Unbuffered-DDR",
	[MEM_RDDR] = "Registered-DDR",
	[MEM_RMBS] = "RMBS",
	[MEM_DDR2] = "Unbuffered-DDR2",
	[MEM_FB_DDR2] = "FullyBuffered-DDR2",
	[MEM_RDDR2] = "Registered-DDR2",
	[MEM_XDR] = "XDR",
	[MEM_DDR3] = "Unbuffered-DDR3",
	[MEM_RDDR3] = "Registered-DDR3",
	[MEM_DDR4] = "Unbuffered-DDR4",
	[MEM_RDDR4] = "Registered-DDR4"
};

static const char * const dev_types[] = {
	[DEV_UNKNOWN] = "Unknown",
	[DEV_X1] = "x1",
	[DEV_X2] = "x2",
	[DEV_X4] = "x4",
	[DEV_X8] = "x8",
	[DEV_X16] = "x16",
	[DEV_X32] = "x32",
	[DEV_X64] = "x64"
};

static const char * const edac_caps[] = {
	[EDAC_UNKNOWN] = "Unknown",
	[EDAC_NONE] = "None",
	[EDAC_RESERVED] = "Reserved",
	[EDAC_PARITY] = "PARITY",
	[EDAC_EC] = "EC",
	[EDAC_SECDED] = "SECDED",
	[EDAC_S2ECD2ED] = "S2ECD2ED",
	[EDAC_S4ECD4ED] = "S4ECD4ED",
	[EDAC_S8ECD8ED] = "S8ECD8ED",
	[EDAC_S16ECD16ED] = "S16ECD16ED"
};

#ifdef CONFIG_EDAC_LEGACY_SYSFS
/*
 * EDAC sysfs CSROW data structures and methods
 */

#define to_csrow(k) container_of(k, struct csrow_info, dev)

/*
 * We need it to avoid namespace conflicts between the legacy API
 * and the per-dimm/per-rank one
 */
#define DEVICE_ATTR_LEGACY(_name, _mode, _show, _store) \
	static struct device_attribute dev_attr_legacy_##_name = __ATTR(_name, _mode, _show, _store)

struct dev_ch_attribute {
	struct device_attribute attr;
	int channel;
};

#define DEVICE_CHANNEL(_name, _mode, _show, _store, _var) \
	static struct dev_ch_attribute dev_attr_legacy_##_name = \
		{ __ATTR(_name, _mode, _show, _store), (_var) }

#define to_channel(k) (container_of(k, struct dev_ch_attribute, attr)->channel)

/* Set of more default csrow<id> attribute show/store functions */
static ssize_t csrow_ue_count_show(struct device *dev,
				   struct device_attribute *mattr, char *data)
{
	struct csrow_info *csrow = to_csrow(dev);

	return sprintf(data, "%u\n", csrow->ue_count);
}

static ssize_t csrow_ce_count_show(struct device *dev,
				   struct device_attribute *mattr, char *data)
{
	struct csrow_info *csrow = to_csrow(dev);

	return sprintf(data, "%u\n", csrow->ce_count);
}

static ssize_t csrow_size_show(struct device *dev,
			       struct device_attribute *mattr, char *data)
{
	struct csrow_info *csrow = to_csrow(dev);
	int i;
	u32 nr_pages = 0;

	for (i = 0; i < csrow->nr_channels; i++)
		nr_pages += csrow->channels[i]->dimm->nr_pages;
	return sprintf(data, "%u\n", PAGES_TO_MiB(nr_pages));
}

static ssize_t csrow_mem_type_show(struct device *dev,
				   struct device_attribute *mattr, char *data)
{
	struct csrow_info *csrow = to_csrow(dev);

	return sprintf(data, "%s\n", mem_types[csrow->channels[0]->dimm->mtype]);
}

static ssize_t csrow_dev_type_show(struct device *dev,
				   struct device_attribute *mattr, char *data)
{
	struct csrow_info *csrow = to_csrow(dev);

	return sprintf(data, "%s\n", dev_types[csrow->channels[0]->dimm->dtype]);
}

static ssize_t csrow_edac_mode_show(struct device *dev,
				    struct device_attribute *mattr,
				    char *data)
{
	struct csrow_info *csrow = to_csrow(dev);

	return sprintf(data, "%s\n", edac_caps[csrow->channels[0]->dimm->edac_mode]);
}

/* show/store function