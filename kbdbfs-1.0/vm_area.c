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
 *  $Id: vm_area.c,v 1.1 2003/11/30 22:52:30 aditya Exp $
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#ifdef FISTGEN
# include "fist_kdb3fs.h"
#endif /* FISTGEN */
#include "fist.h"
#include "kdb3fs.h"

#ifdef NOT_NEEDED
# error this is the old mmap method from file.c
STATIC int
kdb3fs_mmap(file_t *file, vm_area_t *vma)
{
	int err = 0;
	file_t *hidden_file = NULL;
	inode_t *inode;
	inode_t *hidden_inode;
	vm_area_t *hidden_vma = (vm_area_t *) 0xdeadc0de;

	print_entry_location();
	if (ftopd(file) != NULL)
		hidden_file = ftohf(file);
	inode = file->f_dentry->d_inode;
	hidden_inode = itohi(inode);

	fist_dprint(6, "MMAP1: inode 0x%x, hidden_inode 0x%x, inode->i_count %d, hidden_inode->i_count %d\n",
		    (int) inode, (int) hidden_inode, atomic_read(&inode->i_count), atomic_read(&hidden_inode->i_count));

	if (!hidden_file->f_op || !hidden_file->f_op->mmap) {
		err = -ENODEV;
		goto out;
	}

	/*
	 * Most of this code comes straight from generic_file_mmap
	 * in mm/filemap.c.
	 */
	if ((vma->vm_flags & VM_SHARED) && (vma->vm_flags & VM_MAYWRITE)) {
		vma->vm_ops = &kdb3fs_shared_vmops;
	} else {
		vma->vm_ops = &kdb3fs_private_vmops;
	}
	if (!inode->i_sb || !S_ISREG(inode->i_mode)) {
		err = -EACCES;
		goto out;
	}
	if (!hidden_inode->i_op || !hidden_inode->i_mapping->a_ops->readpage) {
		err = -ENOEXEC;
		goto out;
	}
	UPDATE_ATIME(inode);

	/*
	 * Now we do the hidden stuff, but only for shared maps.
	 */
	if ((vma->vm_flags & VM_SHARED) && (vma->vm_flags & VM_MAYWRITE)) {
		hidden_vma = KMALLOC(sizeof(vm_area_t), GFP_KERNEL);
		if (!hidden_vma) {
			printk("MMAP: Out of memory\n");
			err = -ENOMEM;
			goto out;
		}
		// ION, is this right?
		memcpy(hidden_vma, vma, sizeof(vm_area_t));
		vmatohvma(vma) = hidden_vma;
		err = hidden_file->f_op->mmap(hidden_file, hidden_vma);
		hidden_vma->vm_file = hidden_file;
		atomic_inc(&hidden_file->f_count);
	}

	/*
	 * XXX: do we need to insert_vm_struct and merge_segments as is
	 * done in do_mmap()?
	 */
out:
	fist_dprint(6, "MMAP2: inode 0x%x, hidden_inode 0x%x, inode->i_count %d, hidden_inode->i_count %d\n",
		    (int) inode, (int) hidden_inode, atomic_read(&inode->i_count), atomic_read(&hidden_inode->i_count));
#if 0
	if (!err) {
		inode->i_mmap = vma;
		hidden_inode->i_mmap = hidden_vma;
	}
#endif
	print_exit_status(err);
	return err;
}


STATIC void
kdb3fs_vm_open(vm_area_t *vma)
{
	vm_area_t *hidden_vma, *hidden_vma2;
	file_t *file;
	file_t *hidden_file = NULL;

	print_entry_location();

	hidden_vma = vmatohvma(vma);
	file = vma->vm_file;
	if (ftopd(file) != NULL)
		hidden_file = ftohf(file);

	fist_dprint(6, "VM_OPEN: file 0x%x, hidden_file 0x%x, file->f_count %d, hidden_file->f_count %d\n",
		    (int) file, (int) hidden_file,
		    (int) atomic_read(&file->f_count),
		    (int) atomic_read(&hidden_file->f_count));

	if (hidden_vma->vm_ops->open)
		hidden_vma->vm_ops->open(hidden_vma);
	atomic_inc(&hidden_file->f_count);

	/* we need to duplicate the private data */
	hidden_vma2 = KMALLOC(sizeof(vm_area_t), GFP_KERNEL);
	if (!hidden_vma2) {
		printk("VM_OPEN: Out of memory\n");
		goto out;
	}
	memcpy(hidden_vma2, hidden_vma, sizeof(vm_area_t));
	vmatohvma(vma) = hidden_vma2;

out:
	fist_dprint(6, "VM_OPEN2: file 0x%x, hidden_file 0x%x, file->f_count %d, hidden_file->f_count %d\n",
		    (int) file, (int) hidden_file,
		    (int) atomic_read(&file->f_count),
		    (int) atomic_read(&hidden_file->f_count));
	print_exit_location();
}


STATIC void
kdb3fs_vm_close(vm_area_t *vma)
{
	vm_area_t *hidden_vma;
	file_t *file;
	file_t *hidden_file = NULL;

	print_entry_location();

	hidden_vma = vmatohvma(vma);
	file = vma->vm_file;
	if (ftopd(file) != NULL)
		hidden_file = ftohf(file);

	fist_dprint(6, "VM_CLOSE1: file 0x%x, hidden_file 0x%x, file->f_count %d, hidden_file->f_count %d\n",
		    (int) file, (int) hidden_file,
		    (int) atomic_read(&file->f_count),
		    (int) atomic_read(&hidden_file->f_count));

	ASSERT(hidden_vma != NULL);

	if (hidden_vma->vm_ops->close)
		hidden_vma->vm_ops->close(hidden_vma);
	fput(hidden_file);
	KFREE(hidden_vma);
	vmatohvma(vma) = NULL;

	fist_dprint(6, "VM_CLOSE2: file 0x%x, hidden_file 0x%x, file->f_count %d, hidden_file->f_count %d\n",
		    (int) file, (int) hidden_file,
		    (int) atomic_read(&file->f_count),
		    (int) atomic_read(&hidden_file->f_count));
	print_exit_location();
}


STATIC void
kdb3fs_vm_shared_unmap(vm_area_t *vma, unsigned long start, size_t len)
{
	vm_area_t *hidden_vma;
	file_t *file;
	file_t *hidden_file = NULL;

	print_entry_location();
	hidden_vma = vmatohvma(vma);
	file = vma->vm_file;
	if (ftopd(file) != NULL)
		hidden_file = ftohf(file);

	fist_dprint(6, "VM_S_UNMAP1: file 0x%x, hidden_file 0x%x, file->f_count %d, hidden_file->f_count %d\n",
		    (int) file, (int) hidden_file,
		    (int) atomic_read(&file->f_count),
		    (int) atomic_read(&hidden_file->f_count));

	/*
	 * call sync (filemap_sync) because the default filemap_unmap
	 * calls it too.
	 */
	filemap_sync(vma, start, len, MS_ASYNC);

	if (hidden_vma->vm_ops->unmap)
		hidden_vma->vm_ops->unmap(hidden_vma, start, len);

	fist_dprint(6, "VM_S_UNMAP2: file 0x%x, hidden_file 0x%x, file->f_count %d, hidden_file->f_count %d\n",
		    (int) file, (int) hidden_file,
		    (int) atomic_read(&file->f_count),
		    (int) atomic_read(&hidden_file->f_count));
	print_exit_location();
}


/*
 * Shared mappings need to be able to do the right thing at
 * close/unmap/sync. They will also use the private file as
 * backing-store for swapping..
 */
struct vm_operations_struct kdb3fs_shared_vmops = {
	open:	 kdb3fs_vm_open,
	close:	 kdb3fs_vm_close,
	unmap:	 kdb3fs_vm_shared_unmap,
	sync:	 filemap_sync,
	nopage:	 filemap_nopage,
	swapout: filemap_swapout,
};


/*
 * Private mappings just need to be able to load in the map.
 *
 * (This is actually used for shared mappings as well, if we
 * know they can't ever get write permissions..)
 */
struct vm_operations_struct kdb3fs_private_vmops = {
	nopage:	filemap_nopage,
};
#endif /* NOT_NEEDED */

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
