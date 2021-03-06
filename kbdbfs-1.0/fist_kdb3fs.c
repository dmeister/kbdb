/*
 * kdb3fs.c
 * Do not edit by hand.
 * Automatically generated by fistgen.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#ifdef FISTGEN
# include "fist_kdb3fs.h"
#endif /* FISTGEN */
#include "fist.h"
#include "kdb3fs.h"

/* OPTIONAL CODE FOLLOWS */


int
kdb3fs_encode_block(const char *base, ino_t inum, unsigned long page_index, int len)
{
	DBT  data, key;
	struct dbkey keydata;
	int err=0;

	print_entry_location();

	memset(&key, 0, sizeof(key)); 
	memset(&data, 0, sizeof(data)); 

	keydata.inum = inum;
	keydata.p_index = page_index;

	key.data = &keydata;
	key.size = sizeof(keydata);

	data.data = (void*)base;
	data.size = len;
	
	if ((err = data_dbp->put(data_dbp, NULL, &key, &data, 0)) == 0) {
#ifdef KERNEL_DEBUG
		//printk("kdb3fs_encode_block key stored=%s, data=%s, data.size=%d\n", (char *)key.data, (char *)data.data, (int)data.size); 
#endif
	} else { 
		printk("error in kdb3fs_encode_block in putting %d\n", (int)keydata.inum);
	} 
	print_exit_location();
	return len;
}


int
kdb3fs_decode_block(ino_t inum, unsigned long page_index, char *to, int len)
{
	DBT  data, key;
	struct dbkey keydata;
	int err;

	/* The same thing happens here! */

	print_entry_location();

	memset(&key, 0, sizeof(key)); 
	memset(&data, 0, sizeof(data)); 
	
	keydata.inum = inum;
	keydata.p_index = page_index;

	key.data = &keydata;
	key.size = sizeof(keydata);
	if ((err = data_dbp->get(data_dbp, NULL, &key, &data, 0)) == 0) {
#ifdef KERNEL_DEBUG
		//	printk("kdb3fs_decode_block, db: %s: key retrieved: data was %s, size=%d\n", (char *)key.data, (char *)data.data, data.size); 
#endif
	} else {
		printk("error in kdb3fs_decode_block in getting %d unsucessful\n", (int)inum); 
	}

	memcpy(to, data.data, data.size);

	print_exit_location();
	return data.size;
}


int
kdb3fs_encode_filename(const char *name, int length, char **encoded_name, int skip_dots, const vnode_t *vp, const vfs_t *vfsp)
{
	int encoded_length;

	encoded_length = length + 1;
	*encoded_name = kmalloc(encoded_length, GFP_KERNEL);
	memcpy(*encoded_name, name, length);
	(*encoded_name)[length] = '\0';

	return encoded_length;
}


/* returns length of decoded string, or -1 if error */
int
kdb3fs_decode_filename(const char *name, int length, char **decoded_name, int skip_dots, const vnode_t *vp, const vfs_t *vfsp)
{
	int error = 0;

	*decoded_name = kmalloc(length, GFP_KERNEL);
	memcpy(*decoded_name, name, length);
	error = length;

	return error;
}


/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
