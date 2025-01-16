/*
 * File:	mca.c
 * Purpose:	Generic MCA handling layer
 *
 * Copyright (C) 2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *
 * Copyright (C) 2002 Dell Inc.
 * Copyright (C) Matt Domsch <Matt_Domsch@dell.com>
 *
 * Copyright (C) 2002 Intel
 * Copyright (C) Jenna Hall <jenna.s.hall@intel.com>
 *
 * Copyright (C) 2001 Intel
 * Copyright (C) Fred Lewis <frederick.v.lewis@intel.com>
 *
 * Copyright (C) 2000 Intel
 * Copyright (C) Chuck Fleckenstein <cfleck@co.intel.com>
 *
 * Copyright (C) 1999, 2004-2008 Silicon Graphics, Inc.
 * Copyright (C) Vijay Chander <vijay@engr.sgi.com>
 *
 * Copyright (C) 2006 FUJITSU LIMITED
 * Copyright (C) Hidetoshi Seto <seto.hidetoshi@jp.fujitsu.com>
 *
 * 2000-03-29 Chuck Fleckenstein <cfleck@co.intel.com>
 *	      Fixed PAL/SAL update issues, began MCA bug fixes, logging issues,
 *	      added min save state dump, added INIT handler.
 *
 * 2001-01-03 Fred Lewis <frederick.v.lewis@intel.com>
 *	      Added setup of CMCI and CPEI IRQs, logging of corrected platform
 *	      errors, completed code for logging of corrected & uncorrected
 *	      machine check errors, and updated for conformance with Nov. 2000
 *	      revision of the SAL 3.0 spec.
 *
 * 2002-01-04 Jenna Hall <jenna.s.hall@intel.com>
 *	      Aligned MCA stack to 16 bytes, added platform vs. CPU error flag,
 *	      set SAL default return values, changed error record structure to
 *	      linked list, added init call to sal_get_state_info_size().
 *
 * 2002-03-25 Matt Domsch <Matt_Domsch@dell.com>
 *	      GUID cleanups.
 *
 * 2003-04-15 David Mosberger-Tang <davidm@hpl.hp.com>
 *	      Added INIT backtrace support.
 *
 * 2003-12-08 Keith Owens <kaos@sgi.com