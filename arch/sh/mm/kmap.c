/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1992-1999,2001-2005 Silicon Graphics, Inc. All rights reserved.
 */

#ifndef _ASM_IA64_SN_ADDRS_H
#define _ASM_IA64_SN_ADDRS_H

#include <asm/percpu.h>
#include <asm/sn/types.h>
#include <asm/sn/arch.h>
#include <asm/sn/pda.h>

/*
 *  Memory/SHUB Address Format:
 *  +-+---------+--+--------------+
 *  |0|  NASID  |AS| NodeOffset   |
 *  +-+---------+--+--------------+
 *
 *  NASID: (low NASID bit is 0) Memory and SHUB MMRs
 *   AS: 2-bit Address Space Identifier. Used only if low NASID bit is 0
 *     00: Local Resources and MMR space
 *           Top bit of NodeOffset
 *               0: Local resources space
 *                  node id:
 *                        0: IA64/NT compatibility space
 *                        2: Local MMR Space
 *                        4: Local memory, regardless of local node id
 *               1: Global MMR space
 *     01: GET space.
 *     10: AMO space.
 *     11: Cacheable memory space.
 *
 *   NodeOffset: byte offset
 *
 *
 *  TIO address format:
 *  +-+----------+--+--------------+
 *  |0|  NASID   |AS| Nodeoffset   |
 *  +-+----------+--+--------------+
 *
 *  NASID: (low NASID bit is 1) TIO
 *   AS: 2-bit Chiplet Identifier
 *     00: TIO LB (Indicates TIO MMR access.)
 *     01: TIO ICE (indicates coretalk space access.)
 * 
 *   NodeOffset: top bit must be set.
 *
 *
 * Note that in both of the above address formats, the low
 * NASID bit indicates if the reference is to the SHUB or TIO MMRs.
 */


/*
 * Define basic shift & mask constants fo