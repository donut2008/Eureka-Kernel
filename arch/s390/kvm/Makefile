/*
 * PAL Firmware support
 * IA-64 Processor Programmers Reference Vol 2
 *
 * Copyright (C) 1999 Don Dugger <don.dugger@intel.com>
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 * Copyright (C) 1999-2001, 2003 Hewlett-Packard Co
 *	David Mosberger <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 *
 * 05/22/2000 eranian Added support for stacked register calls
 * 05/24/2000 eranian Added support for physical mode static calls
 */

#include <asm/asmmacro.h>
#include <asm/processor.h>

	.data
pal_entry_point:
	data8 ia64_pal_default_ha