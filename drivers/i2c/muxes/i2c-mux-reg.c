t qib_devdata *dd = dd_from_dev(ibd);

	if (v == SEQ_START_TOKEN)
		return pos;

	++*pos;
	if (*pos >= dd->first_user_ctxt)
		return NULL;
	return pos;
}

static void _ctx_stats_seq_stop(struct seq_file *s, void *v)
{
	/* nothing allocated */
}

static int _ctx_stats_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos;
	loff_t i, j;
	u64 n_packets = 0;
	struct qib_ibdev *ibd = (struct qib_ibdev *)s->private;
	struct qib_devdata *dd = dd_from_dev(ibd);

	if (v == SEQ_START_TOKEN) {
		seq_puts(s, "Ctx:npkts\n");
		return 0;
	}

	spos = v;
	i = *spos;

	if (!dd->rcd[i])
		return SEQ_SKIP;

	for (j = 0; j < ARRAY_SIZE(dd->rcd[i]->opstats->stats); j++)
		n_packets += dd->rcd[i]->opstats->stats[j].n_packets;

	if (!n_packets)
		return SEQ_SKIP;

	seq_printf(s, "  %llu:%llu\n", i, n_packets);
	return 0;
}

DEBUGFS_FILE(ctx_stats)

static void *_qp_stats_seq_start(struct seq_file *s, loff_t *pos)
{
	struct qib_qp_iter *iter;
	loff_t n = *pos;

	rcu_read_lock();
	iter = qib_qp_iter_init(s->private);
	if (!iter)
		return NULL;

	while (n--) {
		if (qib_qp_iter_next(iter)) {
			kfree(iter);
			return NULL;
		}
	}

	return iter;
}

static void *_qp_stats_seq_next(struct seq_file *s, void *iter_ptr,
				   loff_t *pos)
{
	struct qib_qp_iter *iter = iter_ptr;

	(*pos)++;

	if (qib_qp_iter_next(iter)) {
		kfree(iter);
		return NULL;
	}

	return iter;
}

static void _qp_stats_seq_stop(struct seq_file *s, void *iter_ptr)
{
	rcu_read_unlock();
}

static int _qp_stats_seq_show(struct seq_file *s, void *iter_ptr)
{
	struct qib_qp_iter *iter = iter_ptr;

	if (!iter)
		return 0;

	qib_qp_iter_print(s, iter);

	return 0;
}

DEBUGFS_FILE(qp_stats)

void qib_dbg_ibdev_init(struct qib_ibdev *ibd)
{
	char name[10];

	snprintf(name, sizeof(name), "qib%d", dd_from_dev(ibd)->unit);
	ibd->qib_ibdev_dbg = debugfs_create_dir(name, qib_dbg_root);
	if (!ibd->qib_ibdev_dbg) {
		pr_warn("create of %s failed\n", name);
		return;
	}
	DEBUGFS_FILE_CREATE(opcode_stats);
	DEBUGFS_FILE_CREATE(ctx_stats);
	DEBUGFS_FILE_CREATE(qp_stats);
}

void qib_dbg_ibdev_exit(struct qib_ibdev *ibd)
{
	if (!qib_dbg_root)
		goto out;
	debugfs_remove_recursive(ibd->qib_ibdev_dbg);
out:
	ibd->qib_ibdev_dbg = NULL;
}

void qib_dbg_init(void)
{
	qib_dbg_root = debugfs_create_dir(QIB_DRV_NAME, NULL);
	if (!qib_dbg_root)
		pr_warn("init of debugfs failed\n");
}

void qib_dbg_exit(void)
{
	debugfs_remove_recursive(qib_dbg_root);
	qib_dbg_root = NULL;
}

#endif

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      #ifndef _QIB_DEBUGFS_H
#define _QIB_DEBUGFS_H

#ifdef CONFIG_DEBUG_FS
/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

struct qib_ibdev;
void qib_dbg_ibdev_init(struct qib_ibdev *ibd);
void qib_dbg_ibdev_exit(struct qib_ibdev *ibd);
void qib_dbg_init(void);
void qib_dbg_exit(void);

#endif

#endif                          /* _QIB_DEBUGFS_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           