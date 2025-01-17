/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2004 Silicon Graphics, Inc. All rights reserved.
 *
 * Data types used by the SN_SAL_HWPERF_OP SAL call for monitoring
 * SGI Altix node and router hardware
 *
 * Mark Goodwin <markgw@sgi.com> Mon Aug 30 12:23:46 EST 2004
 */

#ifndef SN_HWPERF_H
#define SN_HWPERF_H

/*
 * object structure. SN_HWPERF_ENUM_OBJECTS and SN_HWPERF_GET_CPU_INFO
 * return an array of these. Do not change this without also
 * changing the corresponding SAL code.
 */
#define SN_HWPERF_MAXSTRING		128
struct sn_hwperf_object_info {
	u32 id;
	union {
		struct {
			u64 this_part:1;
			u64 is_shared:1;
		} fields;
		struct {
			u64 flags;
			u64 reserved;
		} b;
	} f;
	char name[SN_HWPERF_MAXSTRING];
	char location[SN_HWPERF_MAXSTRING];
	u32 ports;
};

#define sn_hwp_this_part	f.fields.this_part
#define sn_hwp_is_shared	f.fields.is_shared
#define sn_hwp_flags		f.b.flags

/* macros for object classification */
#define SN_HWPERF_IS_NODE(x)		((x) && strstr((x)->name, "SHub"))
#define SN_HWPERF_IS_NODE_SHUB2(x)	((x) && strstr((x)->name, "SHub 2."))
#define SN_HWPERF_IS_IONODE(x)		((x) && strstr((x)->name, "TIO"))
#define SN_HWPERF_IS_NL3ROUTER(x)	((x) && strstr((x)->name, "NL3Router"))
#define SN_HWPERF_IS_NL4ROUTER(x)	((x) && strstr((x)->name, "NL4Router"))
#define SN_HWPERF_IS_OLDROUTER(x)	((x) && strstr((x)->name, "Router"))
#define SN_HWPERF_IS_ROUTER(x)		(SN_HWPERF_IS_NL3ROUTER(x) || 		\
					 	SN_HWPERF_IS_NL4ROUTER(x) || 	\
					 	SN_HWPERF_IS_OLDROUTER(x))
#define SN_HWPERF_FOREIGN(x)		((x) && !(x)->sn_hwp_this_part && !(x)->sn_hwp_is_shared)
#define SN_HWPERF_SAME_OBJTYPE(x,y)	((SN_HWPERF_IS_NODE(x) && SN_HWPERF_IS_NODE(y)) ||\
					(SN_HWPERF_IS_IONODE(x) && SN_HWPERF_IS_IONODE(y)) ||\
					(SN_HWPERF_IS_ROUTER(x) && SN_HWPERF_IS_ROUTER(y)))

/* numa port structure, SN_HWPERF_ENUM_PORTS returns an array of these */
struct sn_hwperf_port_info {
	u32 port;
	u32 conn_id;
	u32 conn_port;
};

/* for HWPERF_{GET,SET}_MMRS */
struct sn_hwperf_data {
	u64 addr;
	u64 data;
};

/* user ioctl() argument, see below */
struct sn_hwperf_ioctl_args {
        u64 arg;		/* argument, usually an object id */
        u64 sz;                 /* size of transfer */
        void *ptr;              /* pointer to source/target */
        u32 v0;			/* second return value */
};

/*
 * For SN_HWPERF_{GET,SET}_MMRS and SN_HWPERF_OBJECT_DISTANCE,
 * sn_hwperf_ioctl_args.arg can be used to specify a CPU on which
 * to call SAL, and whether to use an interprocessor interrupt
 * or task migration in order to do so. If the CPU specified is
 * SN_HWPERF_ARG_ANY_CPU, then the current CPU will be used.
 */
#define SN_HWPERF_ARG_ANY_CPU		0x7fffffffUL
#define SN_HWPERF_ARG_CPU_MASK		0x7fffffff00000000ULL
#define SN_HWPERF_ARG_USE_IPI_MASK	0x8000000000000000ULL
#define SN_HWPERF_ARG_OBJID_MASK	0x00000000ffffffffULL

/* 
 * ioctl requests on the "sn_hwperf" misc device that call SAL.
 */
#define SN_HWPERF_OP_MEM_COPYIN		0x1000
#define SN_HWPERF_OP_MEM_COPYOUT	0x2000
#define SN_HWPERF_OP_MASK		0x0fff

/*
 * Determine mem requirement.
 * arg	don't care
 * sz	8
 * p	pointer to u64 integer
 */
#define	SN_HWPERF_GET_HEAPSIZE		1

/*
 * Install mem for SAL drvr
 * arg	don't care
 * sz	sizeof buffer pointed to by p
 * p	pointer to buffer for scratch area
 */
#define SN_HWPERF_INSTALL_HEAP		2

/*
 * Determine number of objects
 * arg	don't care
 * sz	8
 * p	pointer to u64 integer
 */
#define SN_HWPERF_OBJECT_COUNT		(10|SN_HWPERF_OP_MEM_COPYOUT)

/*
 * Determine object "distance", relative to a cpu. This operation can
 * execute on a designated logical cpu number, using either an IPI or
 * via task migration. If the cpu number is SN_HWPERF_ANY_CPU, then
 * the current CPU is used. See the SN_HWPERF_ARG_* macros above.
 *
 * arg	bitmap of IPI flag, cpu number and object id
 * sz	8
 * p	pointer to u64 integer
 */
#define SN_HWPERF_OBJECT_DISTANCE	(11|SN_HWPERF_OP_MEM_COPYOUT)

/*
 * Enumerate objects. Special case if sz == 8, returns the required
 * buffer size.
 * arg	don't care
 * sz	sizeof buffer pointed to by p
 * p	poin