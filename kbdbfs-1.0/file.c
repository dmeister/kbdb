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
 *  $Id: file.c,v 1.18 2003/12/22 18:01:41 zubair Exp $
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#ifdef FISTGEN
# include "fist_kdb3fs.h"
#endif /* FISTGEN */
#include "fist.h"
#include "kdb3fs.h"


/*******************
 * File Operations *
 *******************/


STATIC loff_t
kdb3fs_llseek(file_t *file, loff_t offset, int origin)
{
	loff_t err = 0;
	file_t *hidden_file = NULL;

//#error  FIX THIS! WE NEED SEEK_SET, SEEK_CUR, and SEEK_END TO WORK!

	print_entry_location();
	err = -ENOSYS;
out:
	print_exit_status(err);
	return err;
}

STATIC int
kdb3fs_readdir(file_t *file, void *dirent, filldir_t filldir)
{
	int err = 0;
	file_t *hidden_file = NULL;
	inode_t *inode;
	int ret;
	DBC *dbcp;
	DBT tk, td;

	print_entry_location();
	fist_print_file("IN readdir", file);

	dbcp = ftopd(file)->dirent_cursor;
	
        /* lookup dentry->d_name.name in the dirent.db */
	memset(&tk, 0, sizeof(tk));
	memset(&td, 0, sizeof(td));
	
	while (!err && ((ret = dbcp->c_get(dbcp, &tk, &td, DB_NEXT)) == 0)) {
		//	printk("key=%s, inum=%d\n", (char *)tk.data, *(u_int32_t *)td.data);
		//err = filldir(dirent, tk.data, tk.size - 1, 0, simple_strtoul(td.data, NULL, 0), DT_REG);
		err = filldir(dirent, tk.data, tk.size, 10, 10, 10);//, simple_strtoul(td.data, NULL, 0), DT_REG);
	}
        if (err) {
            ret = dbcp->c_get(dbcp, &tk, &td, DB_PREV);
        }

	file->f_version = file->f_dentry->d_inode->i_version;
	print_exit_status(0);
	return 0;
}

STATIC unsigned int
kdb3fs_poll(file_t *file, poll_table *wait)
{
	unsigned int mask = DEFAULT_POLLMASK;
	file_t *hidden_file = NULL;
	
	print_entry_location();
	print_exit_status(mask);
	return mask;
}


STATIC int
kdb3fs_ioctl(inode_t *inode, file_t *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;		/* don't fail by default */
	file_t *hidden_file = NULL;
	vfs_t *this_vfs;
	vnode_t *this_vnode;
	int val;
#ifdef FIST_COUNT_WRITES
	extern unsigned long count_writes, count_writes_middle;
#endif /* FIST_COUNT_WRITES */

	print_entry_location();


	err = -ENOSYS;

//error we should have debugging left in, so taht we can turn it on and off

#if 0
	this_vfs = inode->i_sb;
	this_vnode = inode;

	/* check if asked for local commands */
	switch (cmd) {
	case FIST_IOCTL_GET_DEBUG_VALUE:
		val = fist_get_debug_value();
		printk("IOCTL GET: send arg %d\n", val);
		err = put_user(val, (int *) arg);
#ifdef FIST_COUNT_WRITES
		printk("COUNT_WRITES:%lu:%lu\n", count_writes, count_writes_middle);
# endif /* FIST_COUNT_WRITES */
		break;

	case FIST_IOCTL_SET_DEBUG_VALUE:
		err = get_user(val, (int *) arg);
		if (err)
			break;
		fist_dprint(6, "IOCTL SET: got arg %d\n", val);
		if (val < 0 || val > 20) {
			err = -EINVAL;
			break;
		}
		fist_set_debug_value(val);
		break;

		/* add non-debugging fist ioctl's here */


	default:
		if (ftopd(file) != NULL) {
			hidden_file = ftohf(file);
		}
		/* pass operation to hidden filesystem, and return status */
		if (hidden_file && hidden_file->f_op && hidden_file->f_op->ioctl)
			err = hidden_file->f_op->ioctl(itohi(inode), hidden_file, cmd, arg);
		else
			err = -ENOTTY;	/* this is an unknown ioctl */
	} /* end of outer switch statement */

#endif

	print_exit_status(err);
	return err;
}


STATIC int
kdb3fs_open(inode_t *inode, file_t *file)
{
	int err = 0;
	DBC *dbcp;
	int ret;
	
	print_entry_location();

        ftopd(file) = KMALLOC(sizeof(struct kdb3fs_file_info *), GFP_KERNEL);
	if (!ftopd(file)) {
	    err = -ENOMEM;
	    goto out;
	}

        if (S_ISDIR(inode->i_mode)) {
	    if ((err = dirent_dbp->cursor(dirent_dbp, NULL, &dbcp, 0)) == 0) {
	        ftopd(file)->dirent_cursor = dbcp;
	    }
        } else {
            ftopd(file)->dirent_cursor = NULL;    
        }
out:
	if (err < 0 && ftopd(file)) {
		KFREE(ftopd(file));
	}
	
	print_exit_status(err);
	return err;
}


STATIC int
kdb3fs_flush(file_t *file)
{
	int err = 0;		/* assume ok (see open.c:close_fp) */
	file_t *hidden_file = NULL;

	print_entry_location();
	print_exit_status(err);
	return err;
}


STATIC int
kdb3fs_release(inode_t *inode, file_t *file)
{
	int err = 0;
	DBC *dbcp;

	print_entry_location();

	if (ftopd(file) != NULL) {
		dbcp = ftopd(file)->dirent_cursor;
                if (dbcp) {
		    dbcp->c_close(dbcp);
                }
		KFREE(ftopd(file));
	}

	print_exit_status(err);
	return err;
}


STATIC int
kdb3fs_fsync(file_t *file, dentry_t *dentry, int datasync)
{
	int err = -EINVAL;
	file_t *hidden_file = NULL;
	dentry_t *hidden_dentry;

	print_entry_location();

	err = do_sync(SYNC_DATA | SYNC_MD | SYNC_DIRENT);

out:
	print_exit_status(err);
	return err;
}


STATIC int
kdb3fs_fasync(int fd, file_t *file, int flag)
{
	int err = 0;
	file_t *hidden_file = NULL;

	print_entry_location();
	
	err = -ENOSYS;
	print_exit_status(err);
	return err;
}


STATIC int
kdb3fs_lock(file_t *file, int cmd, struct file_lock *fl)
{
	int err = 0;
	file_t *hidden_file = NULL;

	print_entry_location();
	err = -ENOSYS;
	print_exit_status(err);
	return err;
}


struct file_operations kdb3fs_dir_fops =
{
	llseek:	 kdb3fs_llseek,
	readdir: kdb3fs_readdir,
	poll:	 kdb3fs_poll,
	ioctl:	 kdb3fs_ioctl,
	mmap:	 generic_file_mmap,
	open:	 kdb3fs_open,
	flush:	 kdb3fs_flush,
	release: kdb3fs_release,
	fsync:	 kdb3fs_fsync,
	fasync:	 kdb3fs_fasync,
	lock:	 kdb3fs_lock,
	/* not needed: readv */
	/* not needed: writev */
	/* not implemented: sendpage */
	/* not implemented: get_unmapped_area */
};

struct file_operations kdb3fs_main_fops =
{
	llseek:	 generic_file_llseek,
	read:	 generic_file_read,
	write:	 generic_file_write,
	readdir: kdb3fs_readdir,
	poll:	 kdb3fs_poll,
	mmap:	 generic_file_mmap,
	open:	 kdb3fs_open,
	flush:	 kdb3fs_flush,
	release: kdb3fs_release,
	fsync:	 kdb3fs_fsync,
	fasync:	 kdb3fs_fasync,
	lock:	 kdb3fs_lock,
	/* not needed: readv */
	/* not needed: writev */
	/* not implemented: sendpage */
	/* not implemented: get_unmapped_area */
};

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */