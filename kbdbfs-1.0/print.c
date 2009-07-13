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
 *  $Id: print.c,v 1.4 2003/12/06 04:54:20 zubair Exp $
 */

/* Print debugging functions */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#ifdef FISTGEN
# include "fist_kdb3fs.h"
#endif /* FISTGEN */
#include "fist.h"
#include "kdb3fs.h"

static int fist_debug_var = 0;


/* get value of debugging variable */
int
fist_get_debug_value(void)
{
	return fist_debug_var;
}

/* set debug level variable and return the previous value */
int
fist_set_debug_value(int val)
{
	int prev = fist_debug_var;

	printk("kdb3fs: setting debug level to %d\n", val);
	fist_debug_var = val;
	return prev;
}

/*
 * Utilities used by both client and server
 * Standard levels:
 * 0) no debugging
 * 1) hard failures
 * 2) soft failures
 * 3) current test software
 * 4) main procedure entry points
 * 5) main procedure exit points
 * 6) utility procedure entry points
 * 7) utility procedure exit points
 * 8) obscure procedure entry points
 * 9) obscure procedure exit points
 * 10) random stuff
 * 11) all <= 1
 * 12) all <= 2
 * 13) all <= 3
 * ...
 */

static char buf[256];
void
fist_dprint_internal(int level, char *str,...)
{
	va_list ap;
	int var = fist_get_debug_value();

	if (var == level || (var > 10 && (var - 10) >= level)) {
		va_start(ap, str);
		vsprintf(buf, str, ap);
		printk("%s", buf);
		va_end(ap);
	}
	return;
}


static int num_indents = 0;
char indent_buf[80] = "                                                                               ";
char *
add_indent(void)
{
	indent_buf[num_indents] = ' ';
	num_indents++;
	if (num_indents > 79)
		num_indents = 79;
	indent_buf[num_indents] = '\0';
	return indent_buf;
}

char *
del_indent(void)
{
	if (num_indents <= 0)
		return "<IBUG>";
	indent_buf[num_indents] = ' ';
	num_indents--;
	indent_buf[num_indents] = '\0';
	return indent_buf;
}

#if 0
void
fist_print_page(char *str, page_t *page)
{
	fist_dprint(8, "        %s: %s=%x\n", str, "next", (int) page->next);
	fist_dprint(8, "        %s: %s=%x\n", str, "prev", (int) page->prev);
	fist_dprint(8, "        %s: %s=%x\n", str, "inode", (int) page->inode);
	fist_dprint(8, "        %s: %s=%x\n", str, "index", (int) page->index);
	fist_dprint(8, "        %s: %s=%x\n", str, "next_hash", (int) page->next_hash);
	fist_dprint(8, "        %s: %s=%x\n", str, "count", (int) page->count);
	fist_dprint(8, "        %s: %s=%x\n", str, "flags", (int) page->flags);
	fist_dprint(8, "        %s: %s=%x\n", str, "dirty", (int) page->dirty);
	fist_dprint(8, "        %s: %s=%x\n", str, "age", (int) page->age);
	fist_dprint(8, "        %s: %s=%x\n", str, "wait", (int) page->wait);
	fist_dprint(8, "        %s: %s=%x\n", str, "prev_hash", (int) page->prev_hash);
	fist_dprint(8, "        %s: %s=%x\n", str, "buffers", (int) page->buffers);
	fist_dprint(8, "        %s: %s=%x\n", str, "swap_unlock_entry", (int) page->swap_unlock_entry);
	fist_dprint(8, "        %s: %s=%x\n", str, "map_nr", (int) page->map_nr);
}
#endif


void
fist_print_page_bytes(char *str, page_t *page)
{
	int i;
	char *cp = (char *) page_address(page);

	fist_dprint(8, "PPB %s (0x%p):", str, page);

	for (i=0;i<10;i++)
		fist_dprint(8, "%c,",cp[i]);
	fist_dprint(8, "\n");
}


void
fist_print_buffer_flags(char *str, struct buffer_head *buffer)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,10)
	/*
	 * XXX: 2.4.10 removed buffer_protected state flag.
	 */
#  define buffer_protected(buffer) (-1)
# endif /* kernel 2.4.10 and newer */

	if (!buffer) {
		printk("PBF %s 0x%p\n", str, buffer);
		return;
	}

	fist_dprint(8, "PBF %s 0x%p: Uptodate:%d Dirty:%d Locked:%d Req:%d Protected:%d\n",
		    str,
		    buffer,
		    buffer_uptodate(buffer),
		    buffer_dirty(buffer),
		    buffer_locked(buffer),
		    buffer_req(buffer),
		    buffer_protected(buffer)
		);
}


void
fist_print_page_flags(char *str, page_t *page)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,10)
	/*
	 * XXX: 2.4.10 forgot to export the ksym to swapper_space,
	 * which is used in PageSwapCache().
	 */
#  define fist_PageSwapCache(page) (-1)
# else /* kernel older than 2.4.10 */
#  define fist_PageSwapCache(page) (PageSwapCache(page) ? 1 : 0)
# endif /* kernel older than 2.4.10 */

#ifdef PageDecrAfter
#  define fist_PageDecrAfter(page)	(PageDecrAfter(page) ? 1 : 0)
# else  /* not PageDecrAfter */
#  define fist_PageDecrAfter(page)	(-1)
# endif /* not PageDecrAfter */

	fist_dprint(8, "PPF %s 0x%p/0x%x: Locked:%d Error:%d Referenced:%d Uptodate:%d DecrAfter:%d Slab:%d SwapCache:%d Reserved:%d\n",
		    str,
		    page,
		    page->index,
		    (PageLocked(page) ? 1 : 0),
		    (PageError(page) ? 1 : 0),
		    (PageReferenced(page) ? 1 : 0),
		    (Page_Uptodate(page) ? 1 : 0),
		    fist_PageDecrAfter(page),
		    (PageSlab(page) ? 1 : 0),
		    fist_PageSwapCache(page),
		    (PageReserved(page) ? 1 : 0)
		);
}


void
fist_print_inode(char *str, const inode_t *inode)
{
	if (!inode) {
		printk("PI:%s: NULL INODE PASSED!\n", str);
		return;
	}
	fist_dprint(8, "PI:%s: %s=%lu\n", str, "i_ino", inode->i_ino);
	fist_dprint(8, "PI:%s: %s=%u\n", str, "i_count", atomic_read(&inode->i_count));
	fist_dprint(8, "PI:%s: %s=%u\n", str, "i_nlink", inode->i_nlink);
	if (S_ISDIR(inode->i_mode)) 
		fist_dprint(8, "inode is a directory!\n");
	else 
		fist_dprint(8, "inode is a regular file!\n");	
	fist_dprint(8, "PI:%s: %s=%x\n", str, "i_mode", inode->i_mode);
	fist_dprint(8, "PI:%s: %s=%p\n", str, "i_op", inode->i_op);
	fist_dprint(8, "PI:%s: %s=%p (%s)\n", str, "i_sb",
		    inode->i_sb,
		    (inode->i_sb ? sbt(inode->i_sb) : "NullTypeSB")
		);
	fist_dprint(8, "\n");
	//    fist_dprint(8, "PI:%s: %s=%p\n", str, "itopd(inode)", itopd(inode));
}


void
fist_print_pte_flags(char *str, const page_t *page)
{
	unsigned long address;

	ASSERT(page != NULL);
	address = page->index;
	fist_dprint(8, "PTE-FL:%s index=0x%x\n", str, (int) address);
}


void
fist_print_vma(char *str, const vm_area_t *vma)
{
	return;
}


void
fist_print_file(char *str, const file_t *file)
{
	if (!file) {
		fist_dprint(8, "PF:%s: NULL FILE PASSED!\n", str);
		return;
	}
	fist_dprint(8, "PF:%s: %s=0x%x\n", str, "f_dentry", file->f_dentry);
	fist_dprint(8, "PF:%s: %s=%s\n", str, "name", file->f_dentry->d_name.name);
	fist_dprint(8, "PF:%s: %s=0x%x\n", str, "f_op", file->f_op);
	fist_dprint(8, "PF:%s: %s=0x%x\n", str, "f_mode", file->f_mode);
	if (S_ISDIR(file->f_dentry->d_inode->i_mode)) 
		fist_dprint(8, "file is a directory!\n");
	else 
		fist_dprint(8, "file is a regular file!\n");	
	fist_dprint(8, "PF:%s: %s=0x%x\n", str, "f_pos", file->f_pos);
	fist_dprint(8, "PF:%s: %s=%u\n", str, "f_count", file->f_count);
	fist_dprint(8, "PF:%s: %s=0x%x\n", str, "f_flags", file->f_flags);
	fist_dprint(8, "PF:%s: %s=0x%x\n", str, "f_reada", file->f_reada);
	fist_dprint(8, "PF:%s: %s=%d\n", str, "f_ramax", file->f_ramax);
	fist_dprint(8, "PF:%s: %s=%d\n", str, "f_raend", file->f_raend);
	fist_dprint(8, "PF:%s: %s=%d\n", str, "f_ralen", file->f_ralen);
	fist_dprint(8, "PF:%s: %s=%d\n", str, "f_rawin", file->f_rawin);
	fist_dprint(8, "PF:%s: %s=%lu\n", str, "f_version", file->f_version);
	fist_dprint(8, "\n");
}


void
fist_print_dentry(char *str, const dentry_t *dentry)
{
	if (!dentry) {
		fist_dprint(8, "PD:%s: NULL DENTRY PASSED!\n", str);
		return;
	}
	if (IS_ERR(dentry)) {
		fist_dprint(8, "PD:%s: ERROR DENTRY (%ld)!\n", str, PTR_ERR(dentry));
		return;
	}
	fist_dprint(8, "PD:%s: %s=%d\n", str, "d_count",
		    atomic_read(&dentry->d_count));
	fist_dprint(8, "PD:%s: %s=%x\n", str, "d_flags", (int) dentry->d_flags);
	fist_dprint(8, "PD:%s: %s=%p (%s)\n", str, "d_inode",
		    dentry->d_inode,
		    (dentry->d_inode ? sbt(dentry->d_inode->i_sb) : "nil") );
	fist_dprint(8, "PD:%s: %s=%p (%s)\n", str, "d_parent",
		    dentry->d_parent,
		    (dentry->d_parent ? sbt(dentry->d_parent->d_sb) : "nil") );
	//    fist_dprint(8, "PD:%s: %s=%p\n", str, "d_mounts", dentry->d_mounts);
	//    fist_dprint(8, "PD:%s: %s=%p\n", str, "d_covers", dentry->d_covers);
	//    fist_dprint(8, "PD:%s: %s=%p\n", str, "d_hash", & dentry->d_hash);
	//    fist_dprint(8, "PD:%s: %s=%p\n", str, "d_lru", & dentry->d_lru);
	//    fist_dprint(8, "PD:%s: %s=%p\n", str, "d_child", & dentry->d_child);
	//    fist_dprint(8, "PD:%s: %s=%p\n", str, "d_subdirs", & dentry->d_subdirs);
	//    fist_dprint(8, "PD:%s: %s=%p\n", str, "d_alias", & dentry->d_alias);

	//    fist_dprint(8, "PD:%s: %s=%p\n", str, "d_name", (int) & dentry->d_name);
	fist_dprint(8, "PD:  %s: %s=\"%s\"\n", str, "d_name.name", dentry->d_name.name);
	fist_dprint(8, "PD:  %s: %s=%d\n", str, "d_name.len", (int) dentry->d_name.len);
	//    fist_dprint(8, "PD:  %s: %s=%p\n", str, "d_name.hash", (int) dentry->d_name.hash);

	//    fist_dprint(8, "PD:%s: %s=%lu\n", str, "d_time", dentry->d_time);
	fist_dprint(8, "PD:%s: %s=%p\n", str, "d_op", dentry->d_op);
	fist_dprint(8, "PD:%s: %s=%p (%s)\n", str, "d_sb",
		    dentry->d_sb, sbt(dentry->d_sb));
	//    fist_dprint(8, "PD:%s: %s=%lu\n", str, "d_reftime", dentry->d_reftime);
	fist_dprint(8, "PD:%s: %s=%p\n", str, "d_fsdata", dentry->d_fsdata);
	// don't do this, it's not zero-terminated!!!
	//    fist_dprint(8, "PD:%s: %s=\"%s\"\n", str, "d_iname", dentry->d_iname);
	//    fist_dprint(8, "\n");
	fist_dprint(8, "PD:%s: %s=%d\n", str, "list_empty(d_hash)", list_empty(&((dentry_t *)dentry)->d_hash));
}


void
fist_checkinode(inode_t *inode, char *msg)
{
	if (!inode) {
		printk(KERN_WARNING "fist_checkinode - inode is NULL! (%s)\n", msg);
		return;
	}
	if (!itopd(inode)) {
		fist_dprint(8, "fist_checkinode(%ld) - no private data (%s)\n", inode->i_ino, msg);
		return;
	}
	if (!itohi(inode)) {
		fist_dprint(8, "fist_checkinode(%ld) - underlying is NULL! (%s)\n", inode->i_ino, msg);
		return;
	}
	if (!inode->i_sb) {
		fist_dprint(8, "fist_checkinode(%ld) - inode->i_sb is NULL! (%s)\n", inode->i_ino, msg);
		return;
	}
	if (!inode->i_sb->s_type) {
		fist_dprint(8, "fist_checkinode(%ld) - inode->i_sb->s_type is NULL! (%s)\n", inode->i_ino, msg);
		return;
	}
	fist_dprint(6, "CI: %s: inode->i_count = %d, hidden_inode->i_count = %d, inode = %d, sb = %s, hidden_sb = %s\n",
		    msg, atomic_read(&inode->i_count),
		    atomic_read(&itohi(inode)->i_count), inode->i_ino,
		    inode->i_sb->s_type->name, itohi(inode)->i_sb->s_type->name);
}

void
fist_print_sb(char *str, const super_block_t *sb)
{
	if (!sb) {
		fist_dprint(8, "PSB:%s: NULL SB PASSED!\n", str);
		return;
	}
	fist_dprint(8, "PSB:%s: %s=%lu\n", str, "s_blocksize", sb->s_blocksize);
	fist_dprint(8, "PSB:%s: %s=%u\n", str, "s_blocksize_bits", sb->s_blocksize_bits);
	fist_dprint(8, "PSB:%s: %s=0x%x\n", str, "s_flags", (int) sb->s_flags);
	fist_dprint(8, "PSB:%s: %s=0x%x\n", str, "s_magic", (int) sb->s_magic);
	fist_dprint(8, "PSB:%s: %s=%llu\n", str, "s_maxbytes", sb->s_maxbytes);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,8)
	fist_dprint(8, "PSB:%s: %s=%d\n", str, "s_count", (int) sb->s_count);
	fist_dprint(8, "PSB:%s: %s=%d\n", str, "s_active", (int) atomic_read(&sb->s_active));
# endif /* kernels 2.4.8 and newer */
}




/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */