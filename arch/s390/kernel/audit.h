#ifndef _ASM_IA64_INTEL_INTRIN_H
#define _ASM_IA64_INTEL_INTRIN_H
/*
 * Intel Compiler Intrinsics
 *
 * Copyright (C) 2002,2003 Jun Nakajima <jun.nakajima@intel.com>
 * Copyright (C) 2002,2003 Suresh Siddha <suresh.b.siddha@intel.com>
 * Copyright (C) 2005,2006 Hongjiu Lu <hongjiu.lu@intel.com>
 *
 */
#include <ia64intrin.h>

#define ia64_barrier()		__memory_barrier()

#define ia64_stop()	/*