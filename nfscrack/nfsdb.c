/*
 * linux/fs/nfsd/nfsdb.c
 *
 * Convert file handles to random file handles, and vice versa.
 *
 * Copyright (C) 2004 SUNY Stony Brook
 */

#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/locks.h>
#include <linux/fs.h>
#include <linux/ext2_fs.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/version.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/random.h>

#include <linux/sunrpc/svc.h>
#include <linux/nfsd/nfsd.h>
#include <linux/nfsd/cache.h>
#include <linux/nfsd/xdr3.h>
#include <linux/nfs3.h>

#include "nfsdb.h"
#include <db.h>

DB *dbrfhp = NULL;
DB *dbfhrp = NULL;

//default number of random bytes - can be changed in /proc/fs/nfs/nfscrack
int db_rsize_proc = 64;

//number of items inserted - for debugging
int numins_rfh = 0;
int numins_fhr = 0;

void nfscrack_dprint(char *str,...)
{
#ifdef NFSPRINT
	va_list ap;
	char *buf = NULL;
	buf = (char *)kmalloc(256, GFP_KERNEL);
	if (!buf) {
		printk("<0>KMALLOC FAILED!\n");
		ASSERT(NULL);
	} else {
		va_start(ap, str);
		vsprintf(buf, str, ap);
		printk("%s", buf);
		va_end(ap);
		kfree(buf);
	}
#endif
}

struct fh_info {
	unsigned char fb_version;
	unsigned char fb_auth_type;
	unsigned char fb_fsid_type;
	unsigned char fb_fileid_type;
	unsigned int xdev;
	unsigned int xino;
	unsigned int ino;
	unsigned int gen_no;
	unsigned int par_ino_no;
};

int fh_compare2(DB *dbp, const DBT *a, const DBT *b) {
	struct fh_info *fh1, *fh2;

	fh1 = a->data;
	fh2 = b->data;

	ASSERT(fh1->fb_version == 1);
	ASSERT(fh2->fb_version == 1);
	ASSERT(fh1->fb_auth_type == 0);
	ASSERT(fh2->fb_auth_type == 0);
	ASSERT(fh1->fb_fsid_type == 0);
	ASSERT(fh2->fb_fsid_type == 0);
	/* Unsigned chars can't be less than zero. */
	ASSERT(fh1->fb_fileid_type <= 2);
	ASSERT(fh2->fb_fileid_type <= 2);

	nfscrack_dprint("COMPARE: fb_fileid_type: %u, %u\n", fh1->fb_fileid_type, fh2->fb_fileid_type);

	//fh1 is root, fh2 is not => fh1<fh2 => -1
	if ((fh1->fb_fileid_type == 0) && (fh2->fb_fileid_type != 0))
		return -1;
	//fh2 is root, fh1 is not => fh1>fh2 => 1
	if ((fh2->fb_fileid_type == 0) && (fh1->fb_fileid_type != 0))
		return 1;

	//fh1 is type 1, fh2 is type 2 => fh1<fh2 => -1
	if ((fh1->fb_fileid_type == 1) && (fh2->fb_fileid_type == 2))
		return -1;
	//fh1 is type 2, fh2 is type 1 => fh1>fh2 => 1
	if ((fh1->fb_fileid_type == 2) && (fh2->fb_fileid_type == 1))
		return 1;

	//at this point, the two file handles should have the same type
	ASSERT(fh1->fb_fileid_type == fh2->fb_fileid_type);

	//root file handle :: compare xdev and xino
	if (fh1->fb_fileid_type == 0) {
		nfscrack_dprint("COMPARE: xdev: %u, %u\n", fh1->xdev, fh2->xdev);
		nfscrack_dprint("COMPARE: xino: %u, %u\n", fh1->xino, fh2->xino);

		if (fh1->xdev > fh2->xdev) {
			nfscrack_dprint("COMPARE: xdev: fh1>fh2\n");
			return 1;
		}
		if (fh1->xdev < fh2->xdev) {
			nfscrack_dprint("COMPARE: xdev: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->xdev == fh2->xdev);

		if (fh1->xino > fh2->xino) {
			nfscrack_dprint("COMPARE: xino: fh1>fh2\n");
			return 1;
		}
		if (fh1->xino < fh2->xino) {
			nfscrack_dprint("COMPARE: xino: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->xino == fh2->xino);
		return 0;
	}

	//type 1 file handle :: compare xdev, xino, ino, gen
	if (fh1->fb_fileid_type == 1) {
		nfscrack_dprint("COMPARE: xdev: %u, %u\n", fh1->xdev, fh2->xdev);
		nfscrack_dprint("COMPARE: xino: %u, %u\n", fh1->xino, fh2->xino);
		nfscrack_dprint("COMPARE: ino: %u, %u\n", fh1->ino, fh2->ino);
		nfscrack_dprint("COMPARE: gen_no: %u, %u\n", fh1->gen_no, fh2->gen_no);

		if (fh1->xdev > fh2->xdev) {
			nfscrack_dprint("COMPARE: xdev: fh1>fh2\n");
			return 1;
		}
		if (fh1->xdev < fh2->xdev) {
			nfscrack_dprint("COMPARE: xdev: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->xdev == fh2->xdev);

		if (fh1->xino > fh2->xino) {
			nfscrack_dprint("COMPARE: xino: fh1>fh2\n");
			return 1;
		}
		if (fh1->xino < fh2->xino) {
			nfscrack_dprint("COMPARE: xino: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->xino == fh2->xino);

		if (fh1->ino > fh2->ino) {
			nfscrack_dprint("COMPARE: ino: fh1>fh2\n");
			return 1;
		}
		if (fh1->ino < fh2->ino) {
			nfscrack_dprint("COMPARE: ino: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->ino == fh2->ino);

		if (fh1->gen_no > fh2->gen_no) {
			nfscrack_dprint("COMPARE: gen_no: fh1>fh2\n");
			return 1;
		}
		if (fh1->gen_no < fh2->gen_no) {
			nfscrack_dprint("COMPARE: gen_no: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->gen_no == fh2->gen_no);

		return 0;
	}

	//type 2 file handle :: compare xdev, xino, parent, ino, gen
	if (fh1->fb_fileid_type == 2) {
		nfscrack_dprint("COMPARE: xdev: %u, %u\n", fh1->xdev, fh2->xdev);
		nfscrack_dprint("COMPARE: xino: %u, %u\n", fh1->xino, fh2->xino);
		nfscrack_dprint("COMPARE: par_ino_no: %u, %u\n", fh1->par_ino_no, fh2->par_ino_no);
		nfscrack_dprint("COMPARE: ino: %u, %u\n", fh1->ino, fh2->ino);
		nfscrack_dprint("COMPARE: gen_no: %u, %u\n", fh1->gen_no, fh2->gen_no);

		if (fh1->xdev > fh2->xdev) {
			nfscrack_dprint("COMPARE: xdev: fh1>fh2\n");
			return 1;
		}
		if (fh1->xdev < fh2->xdev) {
			nfscrack_dprint("COMPARE: xdev: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->xdev == fh2->xdev);

		if (fh1->xino > fh2->xino) {
			nfscrack_dprint("COMPARE: xino: fh1>fh2\n");
			return 1;
		}
		if (fh1->xino < fh2->xino) {
			nfscrack_dprint("COMPARE: xino: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->xino == fh2->xino);

		if (fh1->par_ino_no > fh2->par_ino_no) {
			nfscrack_dprint("COMPARE: par_ino_no: fh1>fh2\n");
			return 1;
		}
		if (fh1->par_ino_no < fh2->par_ino_no) {
			nfscrack_dprint("COMPARE: par_ino_no: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->par_ino_no == fh2->par_ino_no);

		if (fh1->ino > fh2->ino) {
			nfscrack_dprint("COMPARE: ino: fh1>fh2\n");
			return 1;
		}
		if (fh1->ino < fh2->ino) {
			nfscrack_dprint("COMPARE: ino: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->ino == fh2->ino);

		if (fh1->gen_no > fh2->gen_no) {
			nfscrack_dprint("COMPARE: gen_no: fh1>fh2\n");
			return 1;
		}
		if (fh1->gen_no < fh2->gen_no) {
			nfscrack_dprint("COMPARE: gen_no: fh1<fh2\n");
			return -1;
		}

		ASSERT(fh1->gen_no == fh2->gen_no);

		return 0;
	}
	
	ASSERT(NULL);
	return 0;
}

int fh_compare(DB *dbp, const DBT *a, const DBT *b) {
	int ret;
	nfscrack_dprint("COMPARE: A:");
	printhexstr((unsigned char *) a->data, a->size);
	nfscrack_dprint("COMPARE: B:");
	printhexstr((unsigned char *) b->data, b->size);
	ret = fh_compare2(dbp, a, b);
	nfscrack_dprint("COMPARE: RETURN: %d\n", ret);
	return ret;
}

int nfscrack_db_open() {
	int err = 0;
	int ret;
//	int fillfactor;
//	int cachegbytes = 0;
//	int cachebytes = 16777216;

#ifdef NFSCRACK
	printk("Random file handle size: %d\n", DBRANDSIZE);
	if (!dbrfhp) {
		if ((ret = db_create(&dbrfhp, NULL, 0)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: db_create: random->fh database creation failed.\n");
			err = 1;
			goto out;
		}
		printk("NFSCRACK: Successfully created database random->fh\n");

/*		if ((ret = dbrfhp->set_pagesize(dbrfhp, MYPAGESIZE)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: set_pagesize: %d\n", db_strerror(ret));
			err = -1;
			goto out;
		}
		dprintk("NFSCRACK: Successfully set pagesize to %d\n", MYPAGESIZE);
*/
/*		fillfactor = (MYPAGESIZE - 32) / (DBRANDSIZE + 20 + 8);

		if ((ret = dbrfhp->set_h_ffactor(dbrfhp, fillfactor)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: set_h_ffactor: %d\n", db_strerror(ret));
			err = -1;
			goto out;
		}
		printk("NFSCRACK: Successfully set fillfactor to %d\n", fillfactor);
*/
/*		if ((ret = dbrfhp->set_cachesize(dbrfhp, cachegbytes, cachebytes, 1)) != 0) {
                        nfscrack_dprint("NFSCRACK ERROR: set_cachesize: %d\n", db_strerror(ret));
                        err = -1;
                        goto out;
                }
		printk("NFSCRACK: Successfully set cachesize to %d:%d\n", cachegbytes, cachebytes);
*/
		if ((ret = dbrfhp->open(dbrfhp, NULL, DBRANDTOFH, NULL, DB_HASH, DB_CREATE, 0)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: open: random->fh database opening failed.\n");
			err = -1;
			goto out;
		}
		printk("NFSCRACK: Successfully opened database random->fh\n");
	}

	if (!dbfhrp) {
		if ((ret = db_create(&dbfhrp, NULL, 0)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: db_create: fh->random database creation failed.\n");
			err = -1;
			goto out;
		}
		printk("NFSCRACK: Successfully created database fh->random\n");

/*		if ((ret = dbfhrp->set_pagesize(dbfhrp, MYPAGESIZE)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: set_pagesize: %d\n", db_strerror(ret));
			err = -1;
			goto out;
		}
		printk("NFSCRACK: Successfully set pagesize to %d\n", MYPAGESIZE);
*/
/*		if ((ret = dbfhrp->set_cachesize(dbfhrp, cachegbytes, cachebytes, 1)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: set_cachesize: %d\n", db_strerror(ret));
			err = -1;
			goto out;
		}
		printk("NFSCRACK: Successfully set cachesize to %d:%d\n", cachegbytes, cachebytes);
*/
/*		if ((ret = dbfhrp->set_bt_compare(dbfhrp, &fh_compare)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: set_bt_compare: %d\n", db_strerror(ret));
			err = -1;
			goto out;
		}
		printk("NFSCRACK: Successfully set comparison funcion\n");
*/
		if ((ret = dbfhrp->open(dbfhrp, NULL, DBFHTORAND, NULL, DB_HASH, DB_CREATE, 0)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: open: fh->random database opening failed.\n");
			err = -1;
			goto out;
		}
		printk("NFSCRACK: Successfully opened database fh->random\n");
	}

out:
#endif
	return err;
}

void nfscrack_db_close() {
#ifdef NFSCRACK
	printk("ABOUT TO CLOSE RFH: %p\n", dbrfhp);
	if (dbrfhp) {
		dbrfhp->close(dbrfhp,0);
		dbrfhp = NULL;
	}
	printk("ABOUT TO CLOSE FHR: %p\n", dbfhrp);
	if (dbfhrp) {
		dbfhrp->close(dbfhrp,0);
		dbfhrp = NULL;
	}
	printk("<0>CLOSED.\n");
#endif
}

inline int nfscrack_fh2rand_knfsd_fh(struct knfsd_fh *f, int doinsert) {
	return nfscrack_fh2rand_raw((char *)f->fh_base.fh_pad, f->fh_size, (char *)f->fh_base.fh_pad, &f->fh_size, doinsert);
}

inline int nfscrack_fh2rand_svc_fh(struct svc_fh *f, int doinsert) {
	return nfscrack_fh2rand_raw((char *)f->fh_handle.fh_base.fh_pad, f->fh_handle.fh_size, (char *)f->fh_handle.fh_base.fh_pad, &f->fh_handle.fh_size, doinsert);
}

int nfscrack_fh2rand_raw(char *infh, int insize, char *outfh, int *outsize, int doinsert) {
	int	err = 0;
#ifdef NFSCRACK
	char	*buf = NULL;
	char	*orig = NULL;
	int	fhsize;
	int	ret;
	DBT	keyrfh;
	DBT	datarfh;
	DBT	keyfhr;
	DBT	datafhr;

	//store incoming info (normal fh)
	orig = (char *)kmalloc(insize + 1, GFP_KERNEL);
	if (!orig) {
		err = -ENOMEM;
		goto out;
	}
	memcpy(orig, infh, insize);
	fhsize = insize;

	memset(&keyfhr, 0, sizeof(keyfhr));
	memset(&datafhr, 0, sizeof(datafhr));

	keyfhr.data = orig;
	keyfhr.size = fhsize;
	*outsize = DBRANDSIZE;

	if ((ret = dbfhrp->get(dbfhrp, NULL, &keyfhr, &datafhr, 0)) == 0) {
		nfscrack_dprint("NFSCRACK: The file handle was already in the database, size: %d\n", datafhr.size);
		memcpy(outfh, datafhr.data, datafhr.size);
		*outsize = datafhr.size;
	}
	else if (doinsert) {
		int found = 1;
		int tries = 0;

		nfscrack_dprint("NFSCRACK: The file handle was not in the database\n");
	
		do
		{
			if (tries++ == 10) {
				err = -EIO;
				goto out;
			}

			memset(&keyrfh, 0, sizeof(keyrfh));
			memset(&datarfh, 0, sizeof(datarfh));
			memset(&datafhr, 0, sizeof(datafhr));
	
			buf = (char *)kmalloc(DBRANDSIZE, GFP_KERNEL);
			if (!buf) {
				err = -ENOMEM;
				goto out;
			}
			get_random_bytes(buf, DBRANDSIZE);
	
			//set up random->fh info
			keyrfh.data = buf;
			keyrfh.size = DBRANDSIZE;
			datafhr.data = buf;
			datafhr.size = DBRANDSIZE;
			memset(outfh, '\0', 64);
			memcpy(outfh, buf, DBRANDSIZE);

			if ((ret = dbrfhp->get(dbrfhp, NULL, &keyrfh, &datarfh, 0)) != 0) {
				found = 0;					
			}
		}
		while (found);
		
		datarfh.data = orig;
		datarfh.size = fhsize;
		keyfhr.data = orig;
		keyfhr.size = fhsize;

		printhexstr(keyrfh.data, keyrfh.size);
		printhexstr(datarfh.data, datarfh.size);
		printhexstr(keyfhr.data, keyfhr.size);
		printhexstr(datafhr.data, datafhr.size);

		//insert random -> fh info
		if ((ret = dbrfhp->put(dbrfhp, NULL, &keyrfh, &datarfh, 0)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: inserting value into database random->fh\n");
			err = -ENOMSG;
			goto out;
		}
		else {
			numins_rfh++;
		}

		//insert fh -> random info
		if ((ret = dbfhrp->put(dbfhrp, NULL, &keyfhr, &datafhr, 0)) != 0) {
			nfscrack_dprint("NFSCRACK ERROR: inserting value into database fh->random\n");
			err = -ENOMSG;
			goto out;
		}
		else {
			numins_fhr++;
		}
		nfscrack_dprint("NFSCRACK PUT COUNT: RFH(%d) FHR(%d)\n", numins_rfh, numins_fhr);
	} else {
		err = -ENOENT;
	}
out:
	if (orig) {
		kfree(orig);
	}
	if (buf) {
		kfree(buf);
	}
	nfscrack_dprint("NFSCRACK: SENDING BACK THIS FILE HANDLE: \n");
	printhexstr(outfh, *outsize);

#endif
	return err;
}

inline int nfscrack_rand2fh_knfsd_fh(struct knfsd_fh *f) {
	return nfscrack_rand2fh_raw((char *)f->fh_base.fh_pad, f->fh_size, (char *)f->fh_base.fh_pad, &f->fh_size);
}

inline int nfscrack_rand2fh_svc_fh(struct svc_fh *f) {
	return nfscrack_rand2fh_raw((char *)f->fh_handle.fh_base.fh_pad, f->fh_handle.fh_size, (char *)f->fh_handle.fh_base.fh_pad, &f->fh_handle.fh_size);
}

int nfscrack_rand2fh_raw(char *infh, int insize, char *outfh, int *outsize) {
	int	err = 0;
#ifdef NFSCRACK
	int	ret;
	DBT	keyrfh;
	DBT	datarfh;

	ASSERT(insize == DBRANDSIZE);

	memset(&keyrfh, 0, sizeof(keyrfh));
	memset(&datarfh, 0, sizeof(datarfh));
	keyrfh.data = infh; 
	keyrfh.size = insize;

	if ((ret = dbrfhp->get(dbrfhp, NULL, &keyrfh, &datarfh, 0)) == 0) {
		memcpy(outfh, datarfh.data, datarfh.size);
		*outsize = datarfh.size;
		if (datarfh.size < 64) {
			memset(outfh + datarfh.size, '\0', 64 - datarfh.size);
		}
	} else {
		printk("NFSCRACK ERROR: getattr: getting fh unsuccessful: %s\n", db_strerror(ret));
		err = -ENOENT;
		goto out;
	}

	printhexstr(outfh, *outsize);

out:
#endif
	return err;
}
