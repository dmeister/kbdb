/*
 * Copyright (c) 1997-2003 Erez Zadok
 * Copyright (c) 2001-2003 Stony Brook University
 *
 * For specific licensing information, see the COPYING file distributed with
 * this package, or get one from ftp://ftp.filesystems.org/pub/fist/COPYING.
 *
 * This Copyright notice must be kept intact and distributed with all
 * fistgen sources INCLUDING sources generated by fistgen.
 */
/*
 *  $Id: dentry.c,v 1.3 2003/12/03 08:22:10 zubair Exp $
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#ifdef FISTGEN
# include "fist_kdb3fs.h"
#endif /* FISTGEN */
#include "fist.h"
#include "kdb3fs.h"


/*
 * THIS IS A BOOLEAN FUNCTION: returns 1 if valid, 0 otherwise.
 */
STATIC int
kdb3fs_d_revalidate(dentry_t *dentry, int flags)
{
	int err = 1; // default is valid (1); invalid is 0.
	dentry_t *hidden_dentry;

	print_entry_location();

#if 0
	hidden_dentry  = kdb3fs_hidden_dentry(dentry); /* CPW: Moved to after print_entry_location */

	if (!hidden_dentry->d_op || !hidden_dentry->d_op->d_revalidate)
		goto out;

	err = hidden_dentry->d_op->d_revalidate(hidden_dentry, flags);
#endif
out:
	print_exit_status(err);
	return err;
}


STATIC int
kdb3fs_d_hash(dentry_t *dentry, qstr_t *name)
{
	int err = 0;
	dentry_t *hidden_dentry;

	print_entry_location();
#if 0
	hidden_dentry = kdb3fs_hidden_dentry(dentry); /* CPW: Moved to after print_entry_location */

	if (!hidden_dentry->d_op || !hidden_dentry->d_op->d_hash)
		goto out;

	err = hidden_dentry->d_op->d_hash(hidden_dentry, name);
#endif
out:
	print_exit_status(err);
	return err;
}


STATIC int
kdb3fs_d_compare(dentry_t *dentry, qstr_t *a, qstr_t *b)
{
	int err = 0;
	dentry_t *hidden_dentry;

	print_entry_location();
#if 0
	hidden_dentry = kdb3fs_hidden_dentry(dentry); /* CPW: Moved to after print_entry_location */

	if (hidden_dentry->d_op && hidden_dentry->d_op->d_compare) {
		// XXX: WRONG: should encode_filename on a&b strings
		err = hidden_dentry->d_op->d_compare(hidden_dentry, a, b);
	} else {
		err = ((a->len != b->len) || memcmp(a->name, b->name, b->len));
	}
#endif
	print_exit_status(err);
	return err;
}


int
kdb3fs_d_delete(dentry_t *dentry)
{
	dentry_t *hidden_dentry;
	int err = 0;

	print_entry_location();
#if 0
	/* this could be a negative dentry, so check first */
	if (!dtopd(dentry)) {
		fist_dprint(6, "dentry without private data: %*s", dentry->d_name.len, dentry->d_name.name);
		goto out;
	}

	if (!(hidden_dentry = dtohd(dentry))) {
		fist_dprint(6, "dentry without hidden_dentry: %*s", dentry->d_name.len, dentry->d_name.name);
		goto out;
	}

	//    fist_print_dentry("D_DELETE IN", dentry);
	/* added b/c of changes to dput(): it calls d_drop on us */
	if (hidden_dentry->d_op &&
	    hidden_dentry->d_op->d_delete) {
		err = hidden_dentry->d_op->d_delete(hidden_dentry);
	}
#endif
out:
	print_exit_status(err);
	return err;
}


void
kdb3fs_d_release(dentry_t *dentry)
{
	dentry_t *hidden_dentry;

	print_entry_location();
#if 0	
	fist_print_dentry("kdb3fs_d_release IN dentry", dentry);

	/* this could be a negative dentry, so check first */
	if (!dtopd(dentry)) {
		fist_dprint(6, "dentry without private data: %*s", dentry->d_name.len, dentry->d_name.name);
		goto out;
	}
	hidden_dentry = dtohd(dentry);
	fist_print_dentry("kdb3fs_d_release IN hidden_dentry", hidden_dentry);
	if (hidden_dentry->d_inode)
		fist_dprint(6, "kdb3fs_d_release: hidden_inode->i_count %d, i_num %lu.\n",
			    atomic_read(&hidden_dentry->d_inode->i_count),
			    hidden_dentry->d_inode->i_ino);

	/* free private data (kdb3fs_dentry_info) here */
	KFREE(dtopd(dentry));
	dtopd(dentry) = NULL;	/* just to be safe */
	/* decrement hidden dentry's counter and free its inode */
	dput(hidden_dentry);
#endif
out:
	print_exit_location();
}


/*
 * we don't really need kdb3fs_d_iput, because dentry_iput will call iput() if
 * kdb3fs_d_iput is not defined. We left this implemented for ease of
 * tracing/debugging.
 */
void
kdb3fs_d_iput(dentry_t *dentry, inode_t *inode)
{
	print_entry_location();
	iput(inode);
	print_exit_location();
}


struct dentry_operations kdb3fs_dops = {
	d_revalidate:	kdb3fs_d_revalidate,
	d_hash:		kdb3fs_d_hash,
	d_compare:		kdb3fs_d_compare,
	d_release:		kdb3fs_d_release,
	d_delete:		kdb3fs_d_delete,
	d_iput:		kdb3fs_d_iput,
};


/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
