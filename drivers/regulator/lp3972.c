pci->dev->kobj, EDAC_PCI_SYMLINK);
	if (err) {
		edac_dbg(0, "sysfs_create_link() returned err= %d\n", err);
		goto symlink_fail;
	}

	return 0;

	/* Error unwind stack */
symlink_fail:
	edac_pci_unregister_sysfs_instance_kobj(pci);

unregister_cleanup:
	edac_pci_main_kobj_teardown();

	return err;
}

/*
 * edac_pci_remove_sysfs
 *
 *	remove the controls and attributes for this EDAC PCI device
 */
void edac_pci_remove_sysfs(struct edac_pci_ctl_info *pci)
{
	edac_dbg(0, "index=%d\n", pci->pci_idx);

	/* Remove the symlink */
	sysfs_remove_link(&pci->kobj, EDAC_PCI_SYMLINK);

	/* remove this PCI instance's sysfs entries */
	edac_pci_unregister_sysfs_instance_kobj(pci);

	/* Call the main unregister function, which will determine
	 * if this 'pci' is the last instance.
	 * If it is, the main kobject will be unregistered as a result
	 */
	edac_dbg(0, "calling edac_pci_main_kobj_teardown()\n");
	edac_pci_main_kobj_teardown();
}

/************************ PCI error handling *************************/
static u16 get_pci_parity_status(struct pci_dev *dev, int secondary)
{
	int where;
	u16 status;

	where = secondary ? PCI_SEC_STATUS : PCI_STATUS;
	pci_read_config_word(dev, where, &status);

	/* If we get back 0xFFFF then we must suspect that the card has been
	 * pulled but the Linux PCI layer has not yet finished cleaning up.
	 * We don't want to report on such devices
	 */

	if (status == 0xFFFF) {
		u32 sanity;

		pci_read_config_dword(dev, 0, &sanity);

		if (sanity == 0xFFFFFFFF)
			return 0;
	}

	status &= PCI_STATUS_DETECTED_PARITY | PCI_STATUS_SIG_SYSTEM_ERROR |
		PCI_STATUS_PARITY;

	if (status)
		/* reset only the bits we are interested in */
		pci_write_config_word(dev, where, status);

	return status;
}


/* Clear any PCI parity errors logged by this device. */
static void edac_pci_dev_parity_clear(struct pci_dev *dev)
{
	u8 header_type;

	get_pci_parity_status(dev, 0);

	/* read the device TYPE, looking for bridges */
	pci_read_config_byte(dev, PCI_HEADER_TYPE, &header_type);

	if ((header_type & 0x7F) == PCI_HEADER_TYPE_BRIDGE)
		get_pci_parity_status(dev, 1);
}

/*
 *  PCI Parity polling
 *
 *	Function to retrieve the current parity status
 *	and decode it
 *
 */
static void edac_pci_dev_parity_test(struct pci_dev *dev)
{
	unsigned long flags;
	u16 status;
	u8 header_type;

	/* stop any interrupts until we can acquire the status */
	local_irq_save(flags);

	/* read the STATUS register on this device */
	status = get_pci_parity_status(dev, 0);

	/* read the device TYPE, looking for bridges */
	pci_read_config_byte(dev, PCI_HEADER_TYPE, &header_type);

	local_irq_restore(flags);

	edac_dbg(4, "PCI STATUS= 0x%04x %s\n", status, dev_name(&dev->dev));

	/* check the status reg for errors on boards NOT marked as broken
	 * if broken, we cannot trust any of the status bits
	 */
	if (status && !dev->broken_parity_status) {
		if (status & (PCI_STATUS_SIG_SYSTEM_ERROR)) {
			edac_printk(KERN_CRIT, EDAC_PCI,
				"Signaled System Error on %s\n",
				pci_name(dev));
			atomic_inc(&pci_nonparity_count);
		}

		if (status & (PCI_STATUS_PARITY)) {
			edac_printk(KERN_CRIT, EDAC_PCI,
				"Master Data Parity Error on %s\n",
				pci_name(dev));

			atomic_inc(&pci_parity_count);
		}

		if (status & (PCI_STATUS_DETECTED_PARITY)) {
			edac_printk(KERN_CRIT, EDAC_PCI,
				"Detected Parity Error on %s\n",
				pci_name(dev));

			atomic_inc(&pci_parity_count);
		}
	}


	edac_dbg(4, "PCI HEADER TYPE= 0x%02x %s\n",
		 header_type, dev_name(&dev->dev));

	if ((header_type & 0x7F) == PCI_HEADER_TYPE_BRIDGE) {
		/* On bridges, need to examine secondary status register  */
		status = get_pci_parity_status(dev, 1);

		edac_dbg(4, "PCI SEC_STATUS= 0x%04x %s\n",
			 status, dev_name(&dev->dev));

		/* check the secondary status reg for errors,
		 * on NOT broken boards
		 */
		if (status && !dev->broken_parity_status) {
			if (status & (PCI_STATUS_SIG_SYSTEM_ERROR)) {
				edac_printk(KERN_CRIT, EDAC_PCI, "Bridge "
					"Signaled System Error on %s\n",
					pci_name(dev));
				atomic_inc(&pci_nonparity_count);
			}

			if (status & (PCI_STATUS_PARITY)) {
				edac_printk(KERN_CRIT, EDAC_PCI, "Bridge "
					"Master Data Parity Error on "
					"%s\n", pci_name(dev));

				atomic_inc(&pci_parity_count);
			}

			if (status & (PCI_STATUS_DETECTED_PARITY)) {
				edac_printk(KERN_CRIT, EDAC_PCI, "Bridge "
					"Detected Parity Error on %s\n",
					pci_name(dev));

				atomic_inc(&pci_parity_count);
			}
		}
	}
}

/* reduce some complexity in definition of the iterator */
typedef void (*pci_parity_check_fn_t) (struct pci_dev *dev);

/*
 * pci_dev parity list iterator
 *
 *	Scan the PCI device list looking for SERRORs, Master Parity ERRORS or
 *	Parity ERRORs on primary or secondary devices.
 */
static inline void edac_pci_dev_parity_iterator(pci_parity_check_fn_t fn)
{
	struct pci_dev *dev = NULL;

	for_each_pci_dev(dev)
		fn(dev);
}

/*
 * edac_pci_do_parity_check
 *
 *	performs the actual PCI parity check operation
 */
void edac_pci_do_parity_check(void)
{
	int before_count;

	edac_dbg(3, "\n");

	/* if policy has PCI check off, leave now */
	if (!check_pci_errors)
		return;

	before_count = atomic_read(&pci_parity_count);

	/* scan all PCI devices looking for a Parity Error on devices and
	 * bridges.
	 * The iterator calls pci_get_device() which might sleep, thus
	 * we cannot disable interrupts in this scan.
	 */
	edac_pci_dev_parity_iterator(edac_pci_dev_parity_test);

	/* Only if operator has selected panic on PCI Error */
	if (edac_pci_get_panic_on_pe()) {
		/* If the count is different 'after' from 'before' */
		if (before_count != atomic_read(&pci_parity_count))
			panic("EDAC: PCI Parity Error");
	}
}

/*
 * edac_pci_clear_parity_errors
 *
 *	function to perform an iteration over the PCI devices
 *	and clearn their current status
 */
void edac_pci_clear_parity_errors(void)
{
	/* Clear any PCI bus parity errors that devices initially have logged
	 * in their registers.
	 */
	edac_pci_dev_parity_iterator(edac_pci_dev_parity_clear);
}

/*
 * edac_pci_handle_pe
 *
 *	Called to handle a PARITY ERROR event
 */
void edac_pci_handle_pe(struct edac_pci_ctl_info *pci, const char *msg)
{

	/* global PE counter incremented by edac_pci_do_parity_check() */
	atomic_inc(&pci->counters.pe_count);

	if (edac_pci_get_log_pe())
		edac_pci_printk(pci, KERN_WARNING,
				"Parity Error ctl: %s %d: %s\n",
				pci->ctl_name, pci->pci_idx, msg);

	/*
	 * poke all PCI devices and see which one is the troublemaker
	 * panic() is called if set
	 */
	edac_pci_do_parity_check();
}
EXPORT_SYMBOL_GPL(edac_pci_handle_pe);


/*
 * edac_pci_handle_npe
 *
 *	Called to handle a NON-PARITY ERROR event
 */
void edac_pci_handle_npe(struct edac_pci_ctl_info *pci, const char *msg)
{

	/* global NPE counter incremented by edac_pci_do_parity_check() */
	atomic_inc(&pci->counters.npe_count);

	if (edac_pci_get_log_npe())
		edac_pci_printk(pci, KERN_WARNING,
				"Non-Parity Error ctl: %s %d: %s\n",
				pci->ctl_name, pci->pci_idx, msg);

	/*
	 * poke all PCI devices and see which one is the troublemaker
	 * panic() is called if set
	 */
	edac_pci_do_parity_check();
}
EXPORT_SYMBOL_GPL(edac_pci_handle_npe);

/*
 * Define the PCI parameter to the module
 */
module_param(check_pci_errors, int, 0644);
MODULE_PARM_DESC(check_pci_errors,
		 "Check for PCI bus parity errors: 0=off 1=on");
module_param(edac_pci_panic_on_pe, int, 0644);
MODULE_PARM_DESC(edac_pci_panic_on_pe,
		 "Panic on PCI Bus Parity error: 0=off 1=on");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*
 * common EDAC components that must be in kernel
 *
 * Author: Dave Jiang <djiang@mvista.com>
 *
 * 2007 (c) MontaVista Software, Inc.
 * 2010 (c) Advanced Micro Devices Inc.
 *	    Borislav Petkov <bp@alien8.de>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 */
#include <linux/module.h>
#include <linux/edac.h>
#include <linux/atomic.h>
#include <linux/device.h>

int edac_op_state = EDAC_OPSTATE_INVAL;
EXPORT_SYMBOL_GPL(edac_op_state);

atomic_t edac_handlers = ATOMIC_INIT(0);
EXPORT_SYMBOL_GPL(edac_handlers);

int edac_err_assert = 0;
EXPORT_SYMBOL_GPL(edac_err_assert);

static atomic_t edac_subsys_valid = ATOMIC_INIT(0);

int edac_report_status = EDAC_REPORTING_ENABLED;
EXPORT_SYMBOL_GPL(edac_report_status);

static int __init edac_report_setup(char *str)
{
	if (!str)
		return -EINVAL;

	if (!strncmp(str, "on", 2))
		set_edac_report_status(EDAC_REPORTING_ENABLED);
	else if (!strncmp(str, "off", 3))
		set_edac_report_status(EDAC_REPORTING_DISABLED);
	else if (!strncmp(str, "force", 5))
		set_edac_report_status(EDAC_REPORTING_FORCE);

	return 0;
}
__setup("edac_report=", edac_report_setup);

/*
 * called to determine if there is an EDAC driver interested in
 * knowing an event (such as NMI) occurred
 */
int edac_handler_set(void)
{
	if (edac_op_state == EDAC_OPSTATE_POLL)
		return 0;

	return atomic_read(&edac_handlers);
}
EXPORT_SYMBOL_GPL(edac_handler_set);

/*
 * handler for NMI type of interrupts to assert error
 */
void edac_atomic_assert_error(void)
{
	edac_err_assert++;
}
EXPORT_SYMBOL_GPL(edac_atomic_assert_error);

/*
 * sysfs object: /sys/devices/system/edac
 *	need to export to other files
 */
struct bus_type edac_subsys = {
	.name = "edac",
	.dev_name = "edac",
};
EXPORT_SYMBOL_GPL(edac_subsys);

/* return pointer to the 'edac' node in sysfs */
struct bus_type *edac_get_sysfs_subsys(void)
{
	int err = 0;

	if (atomic_read(&edac_subsys_valid))
		goto out;

	/* create the /sys/devices/system/edac directory */
	err = subsys_system_register(&edac_subsys, NULL);
	if (err) {
		printk(KERN_ERR "Error registering toplevel EDAC sysfs dir\n");
		return NULL;
	}

out:
	atomic_inc(&edac_subsys_valid);
	return &edac_subsys;
}
EXPORT_SYMBOL_GPL(edac_get_sysfs_subsys);

void edac_put_sysfs_subsys(void)
{
	/* last user unregisters it */
	if (atomic_dec_and_test(&edac_subsys_valid))
		bus_unregister(&edac_subsys);
}
EXPORT_SYMBOL_GPL(edac_put_sysfs_subsys);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
 * GHES/EDAC Linux driver
 *
 * This file may be distributed under the terms of the GNU General Public
 * License version 2.
 *
 * Copyright (c) 2013 by Mauro Carvalho Chehab
 *
 * Red Hat Inc. http://www.redhat.com
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <acpi/ghes.h>
#include <linux/edac.h>
#include <linux/dmi.h>
#include "edac_core.h"
#include <ras/ras_event.h>

#define GHES_EDAC_REVISION " Ver: 1.0.0"

struct ghes_edac_pvt {
	struct list_head list;
	struct ghes *ghes;
	struct mem_ctl_info *mci;

	/* Buffers for the error handling routine */
	char detail_location[240];
	char other_detail[160];
	char msg[80];
};

static LIST_HEAD(ghes_reglist);
static DEFINE_MUTEX(ghes_edac_lock);
static int ghes_edac_mc_num;


/* Memory Device - Type 17 of SMBIOS spec */
struct memdev_dmi_entry {
	u8 type;
	u8 length;
	u16 handle;
	u16 phys_mem_array_handle;
	u16 mem_err_info_handle;
	u16 total_width;
	u16 data_width;
	u16 size;
	u8 form_factor;
	u8 device_set;
	u8 device_locator;
	u8 bank_locator;
	u8 memory_type;
	u16 type_detail;
	u16 speed;
	u8 manufacturer;
	u8 serial_number;
	u8 asset_tag;
	u8 part_number;
	u8 attributes;
	u32 extended_size;
	u16 conf_mem_clk_speed;
} __attribute__((__packed__));

struct ghes_edac_dimm_fill {
	struct mem_ctl_info *mci;
	unsigned count;
};

static void ghes_edac_count_dimms(const struct dmi_header *dh, void *arg)
{
	int *num_dimm = arg;

	if (dh->type == DMI_ENTRY_MEM_DEVICE)
		(*num_dimm)++;
}

static void ghes_edac_dmidecode(const struct dmi_header *dh, void *arg)
{
	struct ghes_edac_dimm_fill *dimm_fill = arg;
	struct mem_ctl_info *mci = dimm_fill->mci;

	if (dh->type == DMI_ENTRY_MEM_DEVICE) {
		struct memdev_dmi_entry *entry = (struct memdev_dmi_entry *)dh;
		struct dimm_info *dimm = EDAC_DIMM_PTR(mci->layers, mci->dimms,
						       mci->n_layers,
						       dimm_fill->count, 0, 0);

		if (entry->size == 0xffff) {
			pr_info("Can't get DIMM%i size\n",
				dimm_fill->count);
			dimm->nr_pages = MiB_TO_PAGES(32);/* Unknown */
		} else if (entry->size == 0x7fff) {
			dimm->nr_pages = MiB_TO_PAGES(entry->extended_size);
		} else {
			if (entry->size & 1 << 15)
				dimm->nr_pages = MiB_TO_PAGES((entry->size &
							       0x7fff) << 10);
			else
				dimm->nr_pages = MiB_TO_PAGES(entry->size);
		}

		switch (entry->memory_type) {
		case 0x12:
			if (entry->type_detail & 1 << 13)
				dimm->mtype = MEM_RDDR;
			else
				dimm->mtype = MEM_DDR;
			break;
		case 0x13:
			if (entry->type_detail & 1 << 13)
				dimm->mtype = MEM_RDDR2;
			else
				dimm->mtype = ME