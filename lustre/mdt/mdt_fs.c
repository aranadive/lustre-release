// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2011, 2015, Intel Corporation.
 */

/*
 * This file is part of Lustre, http://www.lustre.org/
 *
 * Lustre Metadata Server (MDS) filesystem interface code
 */

#define DEBUG_SUBSYSTEM S_MDS

#include <linux/fs.h>
#include <libcfs/linux/linux-fs.h>
#include "mdt_internal.h"

static const struct proc_ops mdt_open_files_seq_fops = {
	PROC_OWNER(THIS_MODULE)
	.proc_open		= lprocfs_mdt_open_files_seq_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release		= single_release,
};

/**
 * Initialize MDT per-export statistics.
 *
 * This function sets up procfs entries for various MDT export counters. These
 * counters are for per-client statistics tracked on the server.
 *
 * \param[in] obd	OBD device
 * \param[in] exp	OBD export
 * \param[in] localdata	NID of client
 *
 * \retval		0 if successful
 * \retval		negative value on error
 */
int mdt_export_stats_init(struct obd_device *obd, struct obd_export *exp,
			  void *localdata)
{
	struct lnet_nid *client_nid = localdata;
	struct nid_stat *stats;
	int rc;
	ENTRY;

	rc = lprocfs_exp_setup(exp, client_nid);

	if (rc != 0)
		/* Mask error for already created /proc entries */
		RETURN(rc == -EALREADY ? 0 : rc);

	stats = exp->exp_nid_stats;
	stats->nid_stats = lprocfs_stats_alloc(LPROC_MDT_LAST,
						LPROCFS_STATS_FLAG_NOPERCPU);
	if (stats->nid_stats == NULL)
		RETURN(-ENOMEM);

	mdt_stats_counter_init(stats->nid_stats, 0, LPROCFS_CNTR_HISTOGRAM);

	rc = lprocfs_stats_register(stats->nid_proc, "stats", stats->nid_stats);
	if (rc != 0) {
		lprocfs_stats_free(&stats->nid_stats);
		GOTO(out, rc);
	}

	rc = lprocfs_nid_ldlm_stats_init(stats);
	if (rc != 0)
		GOTO(out, rc);

	rc = lprocfs_seq_create(stats->nid_proc, "open_files",
				0444, &mdt_open_files_seq_fops, stats);
	if (rc != 0) {
		CWARN("%s: error adding the open_files file: rc = %d\n",
			obd->obd_name, rc);
		GOTO(out, rc);
	}
out:
	RETURN(rc);
}
