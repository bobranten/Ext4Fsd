// SPDX-License-Identifier: GPL-2.0+
/*
 * linux/fs/jbd2/transaction.c
 *
 * Written by Stephen C. Tweedie <sct@redhat.com>, 1998
 *
 * Copyright 1998 Red Hat corp --- All Rights Reserved
 *
 * Generic filesystem transaction handling code; part of the ext2fs
 * journaling system.
 *
 * This file manages transactions (compound commits managed by the
 * journaling code) and handles (individual atomic operations by the
 * filesystem).
 */

//#include <linux/time.h>
#include <linux/fs.h>
#include <linux/jbd2.h>
#include <linux/errno.h>
//#include <linux/slab.h>
//#include <linux/timer.h>
//#include <linux/mm.h>
//#include <linux/highmem.h>
//#include <linux/hrtimer.h>
//#include <linux/backing-dev.h>
//#include <linux/bug.h>
#include <linux/module.h>
//#include <linux/sched/mm.h>

//#include <trace/events/jbd2.h>

static void __jbd2_journal_temp_unlink_buffer(struct journal_head *jh);
static void __jbd2_journal_unfile_buffer(struct journal_head *jh);

static struct kmem_cache *transaction_cache;
int __init jbd2_journal_init_transaction_cache(void)
{
	J_ASSERT(!transaction_cache);
	transaction_cache = kmem_cache_create("jbd2_transaction_s",
					sizeof(transaction_t),
					0,
					SLAB_HWCACHE_ALIGN|SLAB_TEMPORARY,
					NULL);
	if (transaction_cache)
		return 0;
	return -ENOMEM;
}

void jbd2_journal_destroy_transaction_cache(void)
{
	kmem_cache_destroy(transaction_cache);
	transaction_cache = NULL;
}
