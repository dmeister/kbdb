
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)
/* This is lifted from fs/xattr.c */
static void *
xattr_alloc(size_t size, size_t limit)
{
	void *ptr;

	if (size > limit)
		return ERR_PTR(-E2BIG);

	if (!size)	/* size request, no buffer is needed */
		return NULL;
	else if (size <= PAGE_SIZE)
		ptr = KMALLOC((unsigned long) size, GFP_KERNEL);
	else
		ptr = vmalloc((unsigned long) size);
	if (!ptr)
		return ERR_PTR(-ENOMEM);
	return ptr;
}

static void
xattr_free(void *ptr, size_t size)
{
	if (!size)	/* size request, no buffer was needed */
		return;
	else if (size <= PAGE_SIZE)
		KFREE(ptr);
	else
		vfree(ptr);
}

/* BKL held by caller.
 * dentry->d_inode->i_sem down
 */
STATIC int
kdb3fs_getxattr(struct dentry *dentry, const char *name, void *value, size_t size) {
	struct dentry *hidden_dentry = NULL;
	int err = -ENOTSUPP;
	char *encoded_suffix = NULL;
	int namelen = 0;
	int sufflen = 0;
	int prelen = 0;
	/* Define these anyway so we don't need as much ifdef'ed code. */
	char *encoded_name = NULL;
	char *encoded_value = NULL;

	print_entry_location();
#if 0
	hidden_dentry = dtohd(dentry);

	ASSERT(hidden_dentry);
	ASSERT(hidden_dentry->d_inode);
	ASSERT(hidden_dentry->d_inode->i_op);

	fist_dprint(18, "getxattr: name=\"%s\", value %d bytes\n", name, size);

	if (hidden_dentry->d_inode->i_op->getxattr) {
		const char *suffix;

		namelen = strlen(name);

		suffix = strchr(name, '.');
		if (suffix) {
			suffix ++;
			prelen = (suffix - name) - 1;
			sufflen = namelen - (prelen + 1);
		} else {
			prelen = 0;
			sufflen = namelen;
			suffix = name;
		}

		sufflen = kdb3fs_encode_filename(suffix, sufflen, &encoded_suffix, DO_DOTS, dentry->d_inode, dentry->d_inode->i_sb) - 1;
		encoded_name = xattr_alloc(prelen + sufflen + 2, XATTR_SIZE_MAX);
		if (IS_ERR(encoded_name)) {
			err = PTR_ERR(encoded_name);
			encoded_name = NULL;
			goto out;
		}
		if (prelen) {
			memcpy(encoded_name, name, prelen);
			encoded_name[prelen] = '.';
			memcpy(encoded_name + prelen + 1, encoded_suffix, sufflen);
			encoded_name[prelen + 1 + sufflen] = '\0';
		} else {
			memcpy(encoded_name + prelen, encoded_suffix, sufflen);
			encoded_name[sufflen] = '\0';
		}

		fist_dprint(18, "ENCODED XATTR NAME: %s\n", encoded_name);

		encoded_value = xattr_alloc(size, XATTR_SIZE_MAX);
		if (IS_ERR(encoded_value)) {
			err = PTR_ERR(encoded_value);
			encoded_name = NULL;
			goto out;
		}


		down(&hidden_dentry->d_inode->i_sem);
		/* lock_kernel() already done by caller. */
		err = hidden_dentry->d_inode->i_op->getxattr(hidden_dentry, encoded_name, encoded_value, size);
		/* unlock_kernel() will be done by caller. */
		up(&hidden_dentry->d_inode->i_sem);

		/* Decode the value. */
		if ((err > 0) && size) {
			int ret;

			ret = kdb3fs_encode_block(encoded_value, value, err, dentry->d_inode, dentry->d_inode->i_sb, 0);
			if (ret < 0) {
				err = ret;
				goto out;
			}
		}
	}

out:
	if (encoded_name) {
		xattr_free(encoded_name, namelen + 1);
	}
	if (encoded_value) {
		xattr_free(encoded_value, size);
	}
#endif
	print_exit_status(err);
	return err;
}

/* BKL held by caller.
 * dentry->d_inode->i_sem down
 */
STATIC int
/* This is the prototype used by 2.4.20--22, but the extended attributes
 * patch to 2.4.21 from Andreas Gruenbacher uses a const void * for the value.
 * If you want to actually use the EA's you will need to apply that patch,
 * then fix this definition.
 */
#ifndef FIST_SETXATTR_CONSTVOID
kdb3fs_setxattr(struct dentry *dentry, const char *name, void *value, size_t size, int flags)
#  else /* not FIST_SETXATTR_CONSTVOID */
kdb3fs_setxattr(struct dentry *dentry, const char *name, const void *value, size_t size, int flags)
#  endif /* FIST_SETXATTR_CONSTVOID */
{
	struct dentry *hidden_dentry = NULL;
	int err = -ENOTSUPP;
	char *encoded_suffix = NULL;
	int namelen = 0;
	int sufflen = 0;
	int prelen = 0;
	/* Define these anyway, so we don't have as much ifdef'ed code. */
	char *encoded_value = NULL;
	char *encoded_name = NULL;

	print_entry_location();
#if 0
	hidden_dentry = dtohd(dentry);

	ASSERT(hidden_dentry);
	ASSERT(hidden_dentry->d_inode);
	ASSERT(hidden_dentry->d_inode->i_op);

	fist_dprint(18, "setxattr: name=\"%s\", value %d bytes, flags=%x\n", name, size, flags);

	if (hidden_dentry->d_inode->i_op->setxattr) {
		const char *suffix;

		namelen = strlen(name);

		suffix = strchr(name, '.');
		if (suffix) {
			suffix ++;
			prelen = (suffix - name) - 1;
			sufflen = namelen - (prelen + 1);
		} else {
			prelen = 0;
			sufflen = namelen;
			suffix = name;
		}

		sufflen = kdb3fs_encode_filename(suffix, sufflen, &encoded_suffix, DO_DOTS, dentry->d_inode, dentry->d_inode->i_sb) - 1;
		encoded_name = xattr_alloc(prelen + sufflen + 2, XATTR_SIZE_MAX);
		if (IS_ERR(encoded_name)) {
			err = PTR_ERR(encoded_name);
			encoded_name = NULL;
			goto out;
		}
		if (prelen) {
			memcpy(encoded_name, name, prelen);
			encoded_name[prelen] = '.';
			memcpy(encoded_name + prelen + 1, encoded_suffix, sufflen);
			encoded_name[prelen + 1 + sufflen] = '\0';
		} else {
			memcpy(encoded_name + prelen, encoded_suffix, sufflen);
			encoded_name[sufflen] = '\0';
		}

		fist_dprint(18, "ENCODED XATTR NAME: %s\n", encoded_name);

		/* Encode the value. */
		encoded_value = xattr_alloc(size, XATTR_SIZE_MAX);
		if (IS_ERR(encoded_value)) {
			err = PTR_ERR(encoded_value);
			encoded_value = NULL;
			goto out;
		}

		err = kdb3fs_encode_block(value, encoded_value, size, dentry->d_inode, dentry->d_inode->i_sb, 0);
		if (err < 0) {
			goto out;
		}

		down(&hidden_dentry->d_inode->i_sem);
		/* lock_kernel() already done by caller. */
		err = hidden_dentry->d_inode->i_op->setxattr(hidden_dentry, encoded_name, encoded_value, size, flags);
		/* unlock_kernel() will be done by caller. */
		up(&hidden_dentry->d_inode->i_sem);
	}

out:
	if (encoded_name) {
		xattr_free(encoded_name, namelen + 1);
	}
	if (encoded_suffix) {
		KFREE(encoded_suffix);
	}
	if (encoded_value) {
		xattr_free(encoded_value, size);
	}
#endif
	print_exit_status(err);
	return err;
}

/* BKL held by caller.
 * dentry->d_inode->i_sem down
 */
STATIC int
kdb3fs_removexattr(struct dentry *dentry, const char *name) {
	struct dentry *hidden_dentry = NULL;
	int err = -ENOTSUPP;
	char *encoded_name;
	char *encoded_suffix = NULL;
	int namelen = 0;
	int sufflen = 0;
	int prelen = 0;
	print_entry_location();

	hidden_dentry = dtohd(dentry);

	ASSERT(hidden_dentry);
	ASSERT(hidden_dentry->d_inode);
	ASSERT(hidden_dentry->d_inode->i_op);

	fist_dprint(18, "removexattr: name=\"%s\"\n", name);

	if (hidden_dentry->d_inode->i_op->removexattr) {
		const char *suffix;

		namelen = strlen(name);

		suffix = strchr(name, '.');
		if (suffix) {
			suffix ++;
			prelen = (suffix - name) - 1;
			sufflen = namelen - (prelen + 1);
		} else {
			prelen = 0;
			sufflen = namelen;
			suffix = name;
		}

		sufflen = kdb3fs_encode_filename(suffix, sufflen, &encoded_suffix, DO_DOTS, dentry->d_inode, dentry->d_inode->i_sb) - 1;
		encoded_name = xattr_alloc(prelen + sufflen + 2, XATTR_SIZE_MAX);
		if (IS_ERR(encoded_name)) {
			err = PTR_ERR(encoded_name);
			encoded_name = NULL;
			goto out;
		}
		if (prelen) {
			memcpy(encoded_name, name, prelen);
			encoded_name[prelen] = '.';
			memcpy(encoded_name + prelen + 1, encoded_suffix, sufflen);
			encoded_name[prelen + 1 + sufflen] = '\0';
		} else {
			memcpy(encoded_name + prelen, encoded_suffix, sufflen);
			encoded_name[sufflen] = '\0';
		}

		fist_dprint(18, "ENCODED XATTR NAME: %s\n", encoded_name);

		down(&hidden_dentry->d_inode->i_sem);
		/* lock_kernel() already done by caller. */
		err = hidden_dentry->d_inode->i_op->removexattr(hidden_dentry, encoded_name);
		/* unlock_kernel() will be done by caller. */
		up(&hidden_dentry->d_inode->i_sem);
	}

out:
	print_exit_status(err);
	return err;
}

/* BKL held by caller.
 * dentry->d_inode->i_sem down
 */
STATIC int
kdb3fs_listxattr(struct dentry *dentry, char *list, size_t size) {
	struct dentry *hidden_dentry = NULL;
	int err = -ENOTSUPP;
	char *encoded_list = NULL;

	print_entry_location();

	hidden_dentry = dtohd(dentry);

	ASSERT(hidden_dentry);
	ASSERT(hidden_dentry->d_inode);
	ASSERT(hidden_dentry->d_inode->i_op);

	if (hidden_dentry->d_inode->i_op->listxattr) {
		encoded_list = xattr_alloc(size, XATTR_LIST_MAX);
		if (IS_ERR(encoded_list)) {
			err = PTR_ERR(encoded_list);
			encoded_list = NULL;
			goto out;
		}
		down(&hidden_dentry->d_inode->i_sem);
		/* lock_kernel() already done by caller. */
		err = hidden_dentry->d_inode->i_op->listxattr(hidden_dentry, encoded_list, size);
		/* unlock_kernel() will be done by caller. */
		up(&hidden_dentry->d_inode->i_sem);
		/* The problem is we don't know how big the xattrs
		 * will be without decoding them ourselves.  So,
		 * we just have to copy it attribute by attribute
		 * to figure out.
		 *
		 * XXX: If the buffer was set to zero then we
		 * actually end up lying and passing through the
		 * error.  For some transformations this works, but
		 * you may need to change it (e.g., using an SCA
		 * in the name ala cryptfs).
		 */
		if (encoded_list)
		{
			int i;
			int newsize = 0;
			char *cur = encoded_list;
			char *encoded_suffix;

			while (cur && (cur < (encoded_list + err))) {
				const char *suffix;
				int namelen, sufflen, prelen;

				ASSERT(cur);
				printk("cur: %s\n", cur);

				namelen = strlen(cur);

				suffix = strchr(cur, '.');
				if (suffix) {
					suffix ++;
					prelen = (suffix - cur) - 1;
					sufflen = namelen - (prelen + 1);
				} else {
					prelen = 0;
					sufflen = namelen;
					suffix = cur;
				}

				sufflen = kdb3fs_decode_filename(suffix, sufflen, &encoded_suffix, DO_DOTS, dentry->d_inode, dentry->d_inode->i_sb);

				if (prelen) {
					if (newsize + sufflen + prelen + 2 < size) {
						/* COPY IT IN */
						memcpy(list + newsize, cur, prelen);
						list[newsize + prelen] = '.';
						memcpy(list + newsize + prelen + 1, encoded_suffix, sufflen);
						list[newsize + prelen + 1 + sufflen] = '\0';
					}
					newsize += sufflen + prelen + 2;
				} else {
					if (newsize + sufflen + 1 < size) {
						memcpy(list + newsize, encoded_suffix, sufflen);
						list[newsize + sufflen] = '\0';
					}
					newsize += sufflen + 1;
				}

				cur += (namelen + 1);
			}

			err = newsize;
		}
	}

out:
	if (encoded_list) {
		xattr_free(encoded_list, size);
	}
	print_exit_status(err);
	return err;
}
# endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20) */
