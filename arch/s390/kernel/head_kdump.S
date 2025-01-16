/*
 * I/O SAPIC support.
 *
 * Copyright (C) 1999 Intel Corp.
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 2000-2002 J.I. Lee <jung-ik.lee@intel.com>
 * Copyright (C) 1999-2000, 2002-2003 Hewlett-Packard Co.
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999,2000 Walt Drummond <drummond@valinux.com>
 *
 * 00/04/19	D. Mosberger	Rewritten to mirror more closely the x86 I/O
 *				APIC code.  In particular, we now have separate
 *				handlers for edge and level triggered
 *				interrupts.
 * 00/10/27	Asit Mallick, Goutham Rao <goutham.rao@intel.com> IRQ vector
 *				allocation PCI to vector mapping, shared PCI
 *				interrupts.
 * 00/10/27	D. Mosberger	Document things a bit more to make them more
 *				understandable.  Clean up much of the old
 *				IOSAPIC cruft.
 * 01/07/27	J.I. Lee	PCI irq routing, Platform/Legacy interrupts
 *				and fixes for ACPI S5(SoftOff) support.
 * 02/01/23	J.I. Lee	iosapic pgm fixes for PCI irq routing from _PRT
 * 02/01/07     E. Focht        <efocht@ess.nec.de> Redirectable interrupt
 *				vectors in iosapic_set_affinity(),
 *				initializations for /proc/irq/#/smp_affinity
 * 02/04/02	P. Diefenbaugh	Cleaned up ACPI PCI IRQ routing.
 * 02/04/18	J.I. Lee	bug fix in iosapic_init_pci_irq
 * 02/04/30	J.I. Lee	bug fix in find_iosapic to fix ACPI PCI IRQ to
 *				IOSAPIC mapping error
 * 02/07/29	T. Kochi	Allocate interrupt vectors dynamically
 * 02/08/04	T. Kochi	Cleaned up terminology (irq, global system
 *				interrupt, vector, etc.)
 * 02/09/20	D. Mosberger	Simplified by taking advantage of ACPI's
 *				pci_irq code.
 * 03/02/19	B. Helgaas	Make pcat_compat system-wide, not per-IOSAPIC.
 *				Remove iosapic_address & gsi_base from
 *				external interfaces.  Rationalize
 *				__init/__devinit attributes.
 * 04/12/04 Ashok Raj	<ashok.raj@intel.com> Intel Corporation 2004
 *				Updated to work with irq migration necessary
 *				for CPU Hotplug
 */
/*
 * Here is what the interrupt logic between a PCI device and the kernel looks
 * like:
 *
 * (1) A PCI device raises one of the four interrupt pins (INTA, INTB, INTC,
 *     INTD).  The device is uniquely identified by its bus-, and slot-number
 *     (the function number does not matter here because all functions 