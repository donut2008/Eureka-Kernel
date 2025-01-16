#ifndef _ASM_IA64_SPINLOCK_H
#define _ASM_IA64_SPINLOCK_H

/*
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 *
 * This file is used for SMP configurations only.
 */

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/bitops.h>

#include <linux/atomic.h>
#include <asm/intrinsics.h>

#define arch_spin_lock_init(x)			((x)->lock = 0)

/*
 * Ticket locks are conceptually two parts, one indicating the current head of
 * the queue, and the other indicating the current tail. The lock is acquired
 * by atomically noting the tail and incrementing it by one (thus adding
 * ourself to the queue and noting our position), then waiting until the head
 * becomes equal to the the initial value of the tail.
 * The pad bits in the middle are used to prevent the next_ticket number
 * overflowing into the now_serving number.
 *
 *   31             17  16    15  14                    0
 *  +----------------------------------------------------+
 *  |  now_serving     | padding |   next_ticket         |
 *  +----------------------------------------------------+
 */

#define TICKET_SHIFT	17
#define TICKET_BITS	15
#define	TICKET_MASK	((1 << TICKET_BITS) - 1)

static __always_inline void __ticket_spin_lock(arch_spinlock_t *lock)
{
	int	*p = (int *)&lock->lock, ticket, serve;

	ticket = ia64_fetchadd(1, p, acq);

	if (!(((ticket >> TICKET_SHIFT) ^ ticket) & TICKET_MASK))
		return;

	ia64_invala();

	for (;;) {
		asm volatile ("ld4.c.nc %0=[%1]" : "=r"(serve) : "r"(p) : "memory");

		if (!(((serve >> TICKET_SHIFT) ^ ticket) & TICKET_MASK))
			return;
		cpu_relax();
	}
}

static __always_inline int __ticket_spin_trylock(arch_spinlock_t *lock)
{
	int tmp = ACCESS_ONCE(lock->lock);

	if (!(((tmp >> TICKET_SHIFT) ^ tmp) & TICKET_MASK))
		return ia64_cmpxchg(acq, &lock->lock, tmp, tmp + 1, sizeof (tmp)) == tmp;
	return 0;
}

static __always_inline void __ticket_spin_unlock(arch_spinlock_t *lock)
{
	unsigned short	*p = (unsigned short *)&lock->lock + 1, tmp;

	asm volatile ("ld2.bias %0=[%1]" : "=r"(tmp) : "r"(p));
	ACCESS_ONCE(*p) = (tmp + 2) & ~1;
}

static __always_inline void __ticket_spin_unlock_wait(arch_spinlock_t *lock)
{
	int	*p = (int *)&lock->lock, ticket;

	ia64_invala();

	for (;;) {
		asm volatile ("ld4.c.nc %0=[%1]" : "=r"(ticket) : "r"(p) : "memory");
		if (!(((ticket >> TICKET_SHIFT) ^ tic