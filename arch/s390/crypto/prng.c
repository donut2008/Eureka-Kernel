#ifndef _ASM_IA64_SN_FEATURE_SETS_H
#define _ASM_IA64_SN_FEATURE_SETS_H

/*
 * SN PROM Features
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2005-2006 Silicon Graphics, Inc.  All rights reserved.
 */


/* --------------------- PROM Features -----------------------------*/
extern int sn_prom_feature_available(int id);

#define MAX_PROM_FEATURE_SETS			2

/*
 * The following defines features that may or may not be supported by the
 * current PROM. The OS uses sn_prom_feature_available(feature) to test for
 * the presence of a PROM feature. Down rev (old) PROMs will always test
 * "false" for new features.
 *
 * Use:
 * 		if (sn_prom_feature_available(PRF_XXX))
 * 			...
 */

#define PRF_PAL_CACHE_FLUSH_SAFE	0
#define PRF_DEVICE_FLUSH_LIST		1
#define PRF_HOTPLUG_SUPPORT		2
#define PRF_CPU_DISABLE_SUPPORT		3

/* --------------------- OS Features -------------------------------*/

/*
 * The following defines OS features that are optionally present in
 * the operating system.
 * During boot, PROM is notified of these features via a series of calls:
 *
 * 		ia64_sn_set_os_feature(feature1);
 *
 * Once enabled, a feature cannot be disabled.
 *
 * By default, features are disabled unless explicitly enabled.
 *
 * These defines must be kept in sync with the corresponding
 * PROM definitions in feature_sets.h.
 */
#define  OSF_MCA_SLV_TO_OS_INIT_SLV	0
#define  OSF_FEAT_LOG_SBES		1
#define  OSF_ACPI_ENABLE		2
#define  OSF_PCISEGMENT_ENABLE		3


#endif /* _ASM_IA64_SN_FEATURE_SETS_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                #ifndef _ASM_IA64_SN_SN_SAL_H
#define _ASM_IA64_SN_SN_SAL_H

/*
 * System Abstraction Layer definitions for IA64
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2000-2006 Silicon Graphics, Inc.  All rights reserved.
 */


#include <asm/sal.h>
#include <asm/sn/sn_cpuid.h>
#include <asm/sn/arch.h>
#include <asm/sn/geo.h>
#include <asm/sn/nodepda.h>
#include <asm/sn/shub_mmr.h>

// SGI Specific Calls
#define  SN_SAL_POD_MODE                           0x02000001
#define  SN_SAL_SYSTEM_RESET                       0x02000002
#define  SN_SAL_PROBE                              0x02000003
#define  SN_SAL_GET_MASTER_NASID                   0x02000004
#define	 SN_SAL_GET_KLCONFIG_ADDR		   0x02000005
#define  SN_SAL_LOG_CE				   0x02000006
#define  SN_SAL_REGISTER_CE			   0x02000007
#define  SN_SAL_GET_PARTITION_ADDR		   0x02000009
#define  SN_SAL_XP_ADDR_REGION			   0x0200000f
#define  SN_SAL_NO_FAULT_ZONE_VIRTUAL		   0x02000010
#define  SN_SAL_NO_FAULT_ZONE_PHYSICAL		   0x02000011
#define  SN_SAL_PRINT_ERROR			   0x02000012
#define  SN_SAL_REGISTER_PMI_HANDLER		   0x02000014
#define  SN_SAL_SET_ERROR_HANDLING_FEATURES	   0x0200001a	// reentrant
#define  SN_SAL_GET_FIT_COMPT			   0x0200001b	// reentrant
#define  SN_SAL_GET_SAPIC_INFO                     0x0200001d
#define  SN_SAL_GET_SN_INFO                        0x0200001e
#define  SN_SAL_CONSOLE_PUTC                       0x02000021
#define  SN_SAL_CONSOLE_GETC                       0x02000022
#define  SN_SAL_CONSOLE_PUTS                       0x02000023
#define  SN_SAL_CONSOLE_GETS                       0x02000024
#define  SN_SAL_CONSOLE_GETS_TIMEOUT               0x02000025
#define  SN_SAL_CONSOLE_POLL                       0x02000026
#define  SN_SAL_CONSOLE_INTR                       0x02000027
#define  SN_SAL_CONSOLE_PUTB			   0x02000028
#define  SN_SAL_CONSOLE_XMIT_CHARS		   0x0200002a
#define  SN_SAL_CONSOLE_READC			   0x0200002b
#define  SN_SAL_SYSCTL_OP			   0x02000030
#define  SN_SAL_SYSCTL_MODID_GET	           0x02000031
#define  SN_SAL_SYSCTL_GET                         0x02000032
#define  SN_SAL_SYSCTL_IOBRICK_MODULE_GET          0x02000033
#define  SN_SAL_SYSCTL_IO_PORTSPEED_GET            0x02000035
#define  SN_SAL_SYSCTL_SLAB_GET                    0x02000036
#define  SN_SAL_BUS_CONFIG		   	   0x02000037
#define  SN_SAL_SYS_SERIAL_GET			   0x02000038
#define  SN_SAL_PARTITION_SERIAL_GET		   0x02000039
#define  SN_SAL_SYSCTL_PARTITION_GET               0x0200003a
#define  SN_SAL_SYSTEM_POWER_DOWN		   0x0200003b
#define  SN_SAL_GET_MASTER_BASEIO_NASID		   0x0200003c
#define  SN_SAL_COHERENCE                          0x0200003d
#define  SN_SAL_MEMPROTECT                         0x0200003e
#define  SN_SAL_SYSCTL_FRU_CAPTURE		   0x0200003f

#define  SN_SAL_SYSCTL_IOBRICK_PCI_OP		   0x02000042	// reentrant
#define	 SN_SAL_IROUTER_OP			   0x02000043
#define  SN_SAL_SYSCTL_EVENT                       0x02000044
#define  SN_SAL_IOIF_INTERRUPT			   0x0200004a
#define  SN_SAL_HWPERF_OP			   0x02000050   // lock
#define  SN_SAL_IOIF_ERROR_INTERRUPT		   0x02000051
#define  SN_SAL_IOIF_PCI_SAFE			   0x02000052
#define  SN_SAL_IOIF_SLOT_ENABLE		   0x02000053
#define  SN_SAL_IOIF_SLOT_DISABLE		   0x02000054
#define  SN_SAL_IOIF_GET_HUBDEV_INFO		   0x02000055
#define  SN_SAL_IOIF_GET_PCIBUS_INFO		   0x02000056
#define  SN_SAL_IOIF_GET_PCIDEV_INFO		   0x02000057
#define  SN_SAL_IOIF_GET_WIDGET_DMAFLUSH_LIST	   0x02000058	// deprecated
#define  SN_SAL_IOIF_GET_DEVICE_DMAFLUSH_LIST	   0x0200005a

#define SN_SAL_IOIF_INIT			   0x0200005f
#define SN_SAL_HUB_ERROR_INTERRUPT		   0x02000060
#define SN_SAL_BTE_RECOVER			   0x02000061
#define SN_SAL_RESERVED_DO_NOT_USE		   0x02000062
#define SN_SAL_IOIF_GET_PCI_TOPOLOGY		   0x02000064

#define  SN_SAL_GET_PROM_FEATURE_SET		   0x02000065
#define  SN_SAL_SET_OS_FEATURE_SET		   0x02000066
#define  SN_SAL_INJECT_ERROR			   0x02000067
#define  SN_SAL_SET_CPU_NUMBER			   0x02000068

#define  SN_SAL_KERNEL_LAUNCH_EVENT		   0x02000069
#define  SN_SAL_WATCHLIST_ALLOC			   0x02000070
#define  SN_SAL_WATCHLIST_FREE			   0x02000071

/*
 * Service-specific constants
 */

/* Console interrupt manipulation */
	/* action codes */
#define SAL_CONSOLE_INTR_OFF    0       /* turn the interrupt off */
#define SAL_CONSOLE_INTR_ON     1       /* turn the interrupt on */
#define SAL_CONSOLE_INTR_STATUS 2	/* retrieve the interrupt status */
	/* interrupt specification & status return codes */
#define SAL_CONSOLE_INTR_XMIT	1	/* output interrupt */
#define SAL_CONSOLE_INTR_RECV	2	/* input interrupt */

/* interrupt handling */
#define SAL_INTR_ALLOC		1
#define SAL_INTR_FREE		2
#define SAL_INTR_REDIRECT	3

/*
 * operations available on the generic SN_SAL_SYSCTL_OP
 * runtime service
 */
#define SAL_SYSCTL_OP_IOBOARD		0x0001  /*  retrieve board type */
#define SAL_SYSCTL_OP_TIO_JLCK_RST      0x0002  /* issue TIO clock reset */

/*
 * IRouter (i.e. generalized system controller) operations
 */
#define SAL_IROUTER_OPEN	0	/* open a subchannel */
#define SAL_IROUTER_CLOSE	1	/* close a subchannel */
#define SAL_IROUTER_SEND	2	/* send part of an IRouter packet */
#define SAL_IROUTER_RECV	3	/* receive part of an IRouter packet */
#define SAL_IROUTER_INTR_STATUS	4	/* check the interrupt status for
					 * an open subchannel
					 */
#define SAL_IROUTER_INTR_ON	5	/* enable an interrupt */
#define SAL_IROUTER_INTR_OFF	6	/* disable an interrupt */
#define SAL_IROUTER_INIT	7	/* initialize IRouter driver */

/* IRouter interrupt mask bits */
#define SAL_IROUTER_INTR_XMIT	SAL_CONSOLE_INTR_XMIT
#define SAL_IROUTER_INTR_RECV	SAL_CONSOLE_INTR_RECV

/*
 * Error Handling Features
 */
#define SAL_ERR_FEAT_MCA_SLV_TO_OS_INIT_SLV	0x1	// obsolete
#define SAL_ERR_FEAT_LOG_SBES			0x2	// obsolete
#define SAL_ERR_FEAT_MFR_OVERRIDE		0x4
#define SAL_ERR_FEAT_SBE_THRESHOLD		0xffff0000

/*
 * SAL Error Codes
 */
#define SALRET_MORE_PASSES	1
#define SALRET_OK		0
#define SALRET_NOT_IMPLEMENTED	(-1)
#define SALRET_INVALID_ARG	(-2)
#define SALRET_ERROR		(-3)

#define SN_SAL_FAKE_PROM			   0x02009999

/**
  * sn_sal_revision - get the SGI SAL revision number
  *
  * The SGI PROM stores its version in the sal_[ab]_rev_(major|minor).
  * This routine simply extracts the major and minor values and
  * presents them in a u32 format.
  *
  * For example, version 4.05 would be represented at 0x0405.
  */
static inline u32
sn_sal_rev(void)
{
	struct ia64_sal_systab *systab = __va(efi.sal_systab);

	return (u32)(systab->sal_b_rev_major << 8 | systab->sal_b_rev_minor);
}

/*
 * Returns the master console nasid, if the call fails, return an illegal
 * value.
 */
static inline u64
ia64_sn_get_console_nasid(void)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL(ret_stuff, SN_SAL_GET_MASTER_NASID, 0, 0, 0, 0, 0, 0, 0);

	if (ret_stuff.status < 0)
		return ret_stuff.status;

	/* Master console nasid is in 'v0' */
	return ret_stuff.v0;
}

/*
 * Returns the master baseio nasid, if the call fails, return an illegal
 * value.
 */
static inline u64
ia64_sn_get_master_baseio_nasid(void)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL(ret_stuff, SN_SAL_GET_MASTER_BASEIO_NASID, 0, 0, 0, 0, 0, 0, 0);

	if (ret_stuff.status < 0)
		return ret_stuff.status;

	/* Master baseio nasid is in 'v0' */
	return ret_stuff.v0;
}

static inline void *
ia64_sn_get_klconfig_addr(nasid_t nasid)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL(ret_stuff, SN_SAL_GET_KLCONFIG_ADDR, (u64)nasid, 0, 0, 0, 0, 0, 0);
	return ret_stuff.v0 ? __va(ret_stuff.v0) : NULL;
}

/*
 * Returns the next console character.
 */
static inline u64
ia64_sn_console_getc(int *ch)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_GETC, 0, 0, 0, 0, 0, 0, 0);

	/* character is in 'v0' */
	*ch = (int)ret_stuff.v0;

	return ret_stuff.status;
}

/*
 * Read a character from the SAL console device, after a previous interrupt
 * or poll operation has given us to know that a character is available
 * to be read.
 */
static inline u64
ia64_sn_console_readc(void)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_READC, 0, 0, 0, 0, 0, 0, 0);

	/* character is in 'v0' */
	return ret_stuff.v0;
}

/*
 * Sends the given character to the console.
 */
static inline u64
ia64_sn_console_putc(char ch)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_PUTC, (u64)ch, 0, 0, 0, 0, 0, 0);

	return ret_stuff.status;
}

/*
 * Sends the given buffer to the console.
 */
static inline u64
ia64_sn_console_putb(const char *buf, int len)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0; 
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_PUTB, (u64)buf, (u64)len, 0, 0, 0, 0, 0);

	if ( ret_stuff.status == 0 ) {
		return ret_stuff.v0;
	}
	return (u64)0;
}

/*
 * Print a platform error record
 */
static inline u64
ia64_sn_plat_specific_err_print(int (*hook)(const char*, ...), char *rec)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_REENTRANT(ret_stuff, SN_SAL_PRINT_ERROR, (u64)hook, (u64)rec, 0, 0, 0, 0, 0);

	return ret_stuff.status;
}

/*
 * Check for Platform errors
 */
static inline u64
ia64_sn_plat_cpei_handler(void)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_LOG_CE, 0, 0, 0, 0, 0, 0, 0);

	return ret_stuff.status;
}

/*
 * Set Error Handling Features	(Obsolete)
 */
static inline u64
ia64_sn_plat_set_error_handling_features(void)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_REENTRANT(ret_stuff, SN_SAL_SET_ERROR_HANDLING_FEATURES,
		SAL_ERR_FEAT_LOG_SBES,
		0, 0, 0, 0, 0, 0);

	return ret_stuff.status;
}

/*
 * Checks for console input.
 */
static inline u64
ia64_sn_console_check(int *result)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_POLL, 0, 0, 0, 0, 0, 0, 0);

	/* result is in 'v0' */
	*result = (int)ret_stuff.v0;

	return ret_stuff.status;
}

/*
 * Checks console interrupt status
 */
static inline u64
ia64_sn_console_intr_status(void)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_INTR, 
		 0, SAL_CONSOLE_INTR_STATUS,
		 0, 0, 0, 0, 0);

	if (ret_stuff.status == 0) {
	    return ret_stuff.v0;
	}
	
	return 0;
}

/*
 * Enable an interrupt on the SAL console device.
 */
static inline void
ia64_sn_console_intr_enable(u64 intr)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_INTR, 
		 intr, SAL_CONSOLE_INTR_ON,
		 0, 0, 0, 0, 0);
}

/*
 * Disable an interrupt on the SAL console device.
 */
static inline void
ia64_sn_console_intr_disable(u64 intr)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_INTR, 
		 intr, SAL_CONSOLE_INTR_OFF,
		 0, 0, 0, 0, 0);
}

/*
 * Sends a character buffer to the console asynchronously.
 */
static inline u64
ia64_sn_console_xmit_chars(char *buf, int len)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_CONSOLE_XMIT_CHARS,
		 (u64)buf, (u64)len,
		 0, 0, 0, 0, 0);

	if (ret_stuff.status == 0) {
	    return ret_stuff.v0;
	}

	return 0;
}

/*
 * Returns the iobrick module Id
 */
static inline u64
ia64_sn_sysctl_iobrick_module_get(nasid_t nasid, int *result)
{
	struct ia64_sal_retval ret_stuff;

	ret_stuff.status = 0;
	ret_stuff.v0 = 0;
	ret_stuff.v1 = 0;
	ret_stuff.v2 = 0;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_SYSCTL_IOBRICK_MODULE_GET, nasid, 0, 0, 0, 0, 0, 0);

	/* result is in 'v0' */
	*result = (int)ret_stuff.v0;

	return ret_stuff.status;
}

/**
 * ia64_sn_pod_mode - call the SN_SAL_POD_MODE function
 *
 * SN_SAL_POD_MODE actually takes an argument, but it's always
 * 0 when we call it from the kernel, so we don't have to expose
 * it to the caller.
 */
static inline u64
ia64_sn_pod_mode(void)
{
	struct ia64_sal_retval isrv;
	SAL_CALL_REENTRANT(isrv, SN_SAL_POD_MODE, 0, 0, 0, 0, 0, 0, 0);
	if (isrv.status)
		return 0;
	return isrv.v0;
}

/**
 * ia64_sn_probe_mem - read from memory safely
 * @addr: address to probe
 * @size: number bytes to read (1,2,4,8)
 * @data_ptr: address to store value read by probe (-1 returned if probe fails)
 *
 * Call into the SAL to do a memory read.  If the read generates a machine
 * check, this routine will recover gracefully and return -1 to the caller.
 * @addr is usually a kernel virtual address in uncached space (i.e. the
 * address starts with 0xc), but if called in physical mode, @addr should
 * be a physical address.
 *
 * Return values:
 *  0 - probe successful
 *  1 - probe failed (generated MCA)
 *  2 - Bad arg
 * <0 - PAL error
 */
static inline u64
ia64_sn_probe_mem(long addr, long size, void *data_ptr)
{
	struct ia64_sal_retval isrv;

	SAL_CALL(isrv, SN_SAL_PROBE, addr, size, 0, 0, 0, 0, 0);

	if (data_ptr) {
		switch (size) {
		case 1:
			*((u8*)data_ptr) = (u8)isrv.v0;
			break;
		case 2:
			*((u16*)data_ptr) = (u16)isrv.v0;
			break;
		case 4:
			*((u32*)data_ptr) = (u32)isrv.v0;
			break;
		case 8:
			*((u64*)data_ptr) = (u64)isrv.v0;
			break;
		default:
			isrv.status = 2;
		}
	}
	return isrv.status;
}

/*
 * Retrieve the system serial number as an ASCII string.
 */
static inline u64
ia64_sn_sys_serial_get(char *buf)
{
	struct ia64_sal_retval ret_stuff;
	SAL_CALL_NOLOCK(ret_stuff, SN_SAL_SYS_SERIAL_GET, buf, 0, 0, 0, 0, 0, 0);
	return ret_stuff.status;
}

extern char sn_system_serial_number_string[];
extern u64 sn_partition_serial_number;

static inline char *
sn_system_serial_number(void) {
	if (sn_system_serial_number_string[0]) {
		return(sn_system_serial_number_string);
	} else {
		ia64_sn_sys_serial_get(sn_system_serial_number_string);
		return(sn_system_serial_number_string);
	}
}
	

/*
 * Returns a unique id number for this system and partition (suitable for
 * use with license managers), based in part on the system serial number.
 */
static inline u64
ia64_sn_partition_serial_get(void)
{
	struct ia64_sal_retval ret_stuff;
	ia64_sal_oemcall_reentrant(&ret_stuff, SN_SAL_PARTITION_SERIAL_GET, 0,
				   0, 0, 0, 0, 0, 0);
	if (ret_stuff.status != 0)
	    return 0;
	return ret_stuff.v0;
}

static inline u64
sn_partition_serial_number_val(void) {
	if (unlikely(sn_partition_serial_number == 0)) {
		sn_partition_serial_number = ia64_sn_partition_serial_get();
	}
	return sn_partition_serial_number;
}

/*
 * Returns the partition id of the nasid passed in as an argument,
 * or INVALID_PARTID if the partition id cannot be retrieved.
 */
static inline partid_t
ia64_sn_sysctl_partition_get(nasid_t nasid)
{
	struct ia64_sal_retval ret_stuff;
	SAL_CALL(ret_stuff, SN_SAL_SYSCTL_PARTITION_GET, nasid,
		0, 0, 0, 0, 0, 0);
	if (ret_stuff.status != 0)
	    return -1;
	return ((partid_t)ret_stuff.v0);
}

/*
 * Returns the physical address of the partition's reserved page through
 * an iterative number of calls.
 *
 * On first call, 'cookie' and 'len' should be set to 0, and 'addr'
 * set to the nasid of the partition whose reserved page's address is
 * being sought.
 * On subsequent calls, pass the values, that were passed back on the
 * previous call.
 *
 * While the return status equals SALRET_MORE_PASSES, keep calling
 * this function after first copying 'len' bytes starting at 'addr'
 * into 'buf'. Once the return status equals SALRET_OK, 'addr' will
 * be the physical address of the partition's reserved page. If the
 * return status equals neither of these, an error as occurred.
 */
static inline s64
sn_partition_reserved_page_pa(u64 buf, u64 *cookie, u64 *addr, u64 *len)
{
	struct ia64_sal_retval rv;
	ia64_sal_oemcall_reentrant(&rv, SN_SAL_GET_PARTITION_ADDR, *cookie,
				   *addr, buf, *len, 0, 0, 0);
	*cookie = rv.v0;
	*addr = rv.v1;
	*len = rv.v2;
	return rv.status;
}

/*
 * Register or unregister a physical address range being referenced across
 * a partition boundary for which certain SAL errors should be scanned for,
 * cleaned up and ignored.  This is of value for kernel partitioning code only.
 * Values for the operation argument:
 *	1 = register this address range with SAL
 *	0 = unregister this address range with SAL
 * 
 * SAL maintains a reference count on an address range in case it is registered
 * multiple times.
 * 
 * On success, returns the reference count of the address range after the SAL
 * call has performed the current registration/unregistration.  Returns a
 * negative value if an error occurred.
 */
static inline int
sn_register_xp_addr_region(u64 paddr, u64 len, int operation)
{
	struct ia64_sal_retval ret_stuff;
	ia64_sal_oemcall(&ret_stuff, SN_SAL_XP_ADDR_REGION, paddr, len,
			 (u64)operation, 0, 0, 0, 0);
	return ret_stuff.status;
}

/*
 * Register or unregister an instruction range for which SAL errors should
 * be ignored.  If an error occurs while in the registered range, SAL jumps
 * to return_addr after ignoring the error.  Values for the operation argument:
 *	1 = register this instruction range with SAL
 *	0 = unregister this instruction range with SAL
 *
 * Returns 0 on success, or a negative value if an error occurred.
 */
static inline int
sn_register_nofault_code(u64 start_addr, u64 end_addr, u64 return_addr,
			 int virtual, int operation)
{
	struct ia64_sal_retval ret_stuff;
	u64 call;
	if (virtual) {
		call = SN_SAL_NO_FAULT_ZONE_VIRTUAL;
	} else {
		call = SN_SAL_NO_FAULT_ZONE_PHYSICAL;
	}
	ia64_sal_oemcall(&ret_stuff, call, start_addr, end_addr, return_addr,
			 (u64)1, 0, 0, 0);
	return ret_stuff.status;
}

/*
 * Register or unregister a function to handle a PMI received by a CPU.
 * Before calling the registered handler, SAL sets r1 to the value that
 * was passed in as the global_pointer.
 *
 * If the handler pointer is NULL, then the currently registered handler
 * will be unregistered.
 *
 * Returns 0 on success, or a negative value if an error occurred.
 */
static inline int
sn_register_pmi_handler(u64 handler, u64 global_pointer)
{
	struct ia64_sal_retval ret_stuff;
	ia64_sal_oemcall(&ret_stuff, SN_SAL_REGISTER_PMI_HANDLER, handler,
			 global_pointer, 0, 0, 0, 0, 0);
	return ret_stuff.status;
}

/*
 * Change or query the coherence domain for this partition. Each cpu-based
 * nasid is represented by a bit in an array of 64-bit words:
 *      0 = not in this partition's coherency domain
 *      1 = in this partition's coherency domain
 *
 * It is not possible for the local system's nasids t