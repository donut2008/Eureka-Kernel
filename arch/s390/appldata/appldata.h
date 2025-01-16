/******************************************************************************
 * arch/ia64/include/asm/native/inst.h
 *
 * Copyright (c) 2008 Isaku Yamahata <yamahata at valinux co jp>
 *                    VA Linux Systems Japan K.K.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define DO_SAVE_MIN		IA64_NATIVE_DO_SAVE_MIN

#define MOV_FROM_IFA(reg)	\
	mov reg = cr.ifa

#define MOV_FROM_ITIR(reg)	\
	mov reg = cr.itir

#define MOV_FROM_ISR(reg)	\
	mov reg = cr.isr

#define MOV_FROM_IHA(reg)	\
	mov reg = cr.iha

#define MOV_FROM_IPSR(pred, reg)	\
(pred)	mov reg = cr.ipsr

#define MOV_FROM_IIM(reg)	\
	mov reg = cr.iim

#define MOV_FROM_IIP(reg)	\
	mov reg = cr.iip

#define MOV_FROM_IVR(reg, clob)	\
	mov reg = cr.ivr

#define MOV_FROM_PSR(pred, reg, clob)	\
(pred)	mov reg = psr

#define MOV_FROM_ITC(pred, pred_clob, reg, clob)	\
(pred)	mov reg = ar.itc

#define MOV_TO_IFA(reg, clob)	\
	mov cr.ifa = r