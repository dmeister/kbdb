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
 *  $Id: sca_aux.c,v 1.1 2003/11/30 22:52:30 aditya Exp $
 */
#include "sca_aux.h"

int chunksize = DEFAULT_CHUNK_SZ;  /* Size of an encoded chunk */
int do_fast_tails = 0;		   /* Use fast tail algorithm */

int encode_counter = 0;		/* Usage counters */
int decode_counter = 0;
int write_counter = 0;


int
get_page(int gzfd, struct fistfs_header *hdr, int pageno,
	 unsigned char **out, int *outlen)
{
	unsigned char *data = NULL;	/* mmap gzfd */
	struct stat sb;
	unsigned char *ptr;
	int len, rc=0, back;

	if (pageno < 0)
		return(-1);
	if (pageno > hdr->num_pages)
		return(-1);
	if (!(do_fast_tails) && (pageno == hdr->num_pages))
		return(-1);

	if ((fstat(gzfd,&sb)) < 0)
		return(-1);
	if ((data = mmap((void *)data,sb.st_size,PROT_READ,
			 MAP_PRIVATE,gzfd,0)) == NULL) {
		return(-1);
	}

	if (pageno == 0) {
		ptr = data;
		if (hdr->num_pages == 0) {
			len = sb.st_size;	/* Fast-tail append */
			rc = 1;
		} else {
			len = hdr->offsets[0];
		}
	} else {
		ptr = &(data[hdr->offsets[pageno-1]]);
		if (do_fast_tails && (pageno == hdr->num_pages)) { /* Possible fast tail */
			len = sb.st_size - hdr->offsets[pageno - 1];
			rc = 1;
		} else {
			len = hdr->offsets[pageno] - hdr->offsets[pageno-1];
		}
	}

	if (rc) {			/* Fast tail, already done */
		(*out) = (unsigned char *) malloc (len * sizeof(unsigned char));
		if ((*out) == NULL) {
			perror("malloc");
			return(-1);
		}
		memcpy(*out, ptr, len);
		*outlen = len;
	} else {
		if ((rc = sca_decode_page(ptr, len, out, &back)) < 0) {
			fprintf(stderr, "sca_decode_page returns %d\n", rc);
			return(-1);
		}
		*outlen = rc;
	}
	munmap(data, sb.st_size);
	return(*outlen);
}


int
put_page(int gzfd, struct fistfs_header *hdr, int pageno,
	 unsigned char *data, int datalen)
{
	unsigned char *encdata = NULL;	/* The new data to write */
	unsigned char *tmppage = NULL;	/* Temp page for shifting */
	int rc, curlen, delta, cnt, enclen, tmp;
	struct stat sb;

	write_counter++;

	if (fstat(gzfd, &sb) < 0) {
		return(-1);
	}

	if ((datalen < chunksize) &&
	    (pageno == hdr->num_pages) &&
	    (do_fast_tails)) {
		/* A fast tail, just write it at the end */
		if (pageno > 0) {
			cnt = hdr->offsets[hdr->num_pages-1];
			hdr->real_size -= (sb.st_size - cnt);
			hdr->real_size += datalen;
		}
		else {
			hdr->real_size = datalen;
			cnt = 0;
		}
		lseek(gzfd, cnt, SEEK_SET); /* The beginning is the end */
		rc = 0; tmp = 0;
		while (tmp < datalen) {
			rc = write(gzfd, data, datalen);
			if (rc < 0) {
				return(-1);
			} else {
				tmp += rc;
			}
		}
		ftruncate(gzfd, cnt+datalen);
		return(0);
	}

	if ((enclen = sca_encode_page(data, datalen, &encdata, &enclen)) < 0) {
		return(enclen);
	}

	if (pageno < hdr->num_pages) { /* Replace an existing page */
		if ((tmppage = (unsigned char *) malloc(sizeof(unsigned char) * chunksize))
		    == NULL) {
			return(-1);
		}

		if (pageno == 0) {	/* replace first page */
			curlen = hdr->offsets[0];
		} else {
			curlen = hdr->offsets[pageno] - hdr->offsets[pageno-1];
		}
		delta = curlen - enclen; /* Positive, it got smaller, shift in from
					    here to end by delta bytes.  Negative,
					    it got bigger, shift out from end to
					    here by delta bytes */

		/* Update size  */
		hdr->real_size -= delta;

		/* Do shifting as appropriate */
		if (delta > 0) {
			tmp = lseek(gzfd, hdr->offsets[pageno], SEEK_SET);
			while (tmp<sb.st_size) {
				rc = read(gzfd, tmppage, chunksize);
				lseek(gzfd, tmp-delta, SEEK_SET);
				write(gzfd, tmppage, rc);
				tmp += rc;
				lseek(gzfd, tmp, SEEK_SET);
			}
			ftruncate(gzfd, sb.st_size-delta);
		} else if (delta < 0) {
			tmp = sb.st_size - hdr->offsets[pageno];
			while (tmp > 0) {
				if (tmp > chunksize) {
					lseek(gzfd, hdr->offsets[pageno] + tmp - chunksize, SEEK_SET);
					rc = read(gzfd, tmppage, chunksize);
					lseek(gzfd,hdr->offsets[pageno] + tmp + - chunksize +(-delta),
					      SEEK_SET);
					write(gzfd, tmppage, rc);
				} else {
					lseek(gzfd, hdr->offsets[pageno], SEEK_SET);
					rc = read(gzfd, tmppage, tmp);
					lseek(gzfd, hdr->offsets[pageno] + (-delta), SEEK_SET);
					write(gzfd, tmppage, tmp);
				}
				tmp -= chunksize;
			}
		} /* Else it happens to fit exactly! no shifting necessary! */

		/* Update index table */
		for (tmp=pageno; tmp<hdr->num_pages; tmp++) {
			hdr->offsets[tmp] -= delta;
		}

		lseek(gzfd, hdr->offsets[pageno-1], SEEK_SET); /* move to correct offset */
		free(tmppage);
	} else {			/* Add a new page somewhere later in the file */
		delta = pageno - hdr->num_pages;

		hdr->real_size += datalen;

		hdr->num_pages += (delta + 1);
		if ((hdr->offsets = realloc(hdr->offsets, (hdr->num_pages * sizeof(off_t))))
		    == NULL) {
			return(-1);
		}

		for (; delta>0; delta--) {
			/* Insert an empty page and an index entry for same */
			fprintf(stderr, "Should insert an empty page here\n");
			return(-1);		/* For now */
		}

		if (hdr->num_pages == 1) {
			hdr->offsets[pageno] = enclen;
			lseek(gzfd, 0, SEEK_SET);
		} else {
			hdr->offsets[pageno] = enclen + hdr->offsets[pageno-1];
			lseek(gzfd, hdr->offsets[pageno-1], SEEK_SET);
		}
	}

	cnt = 0;
	while (cnt < enclen) {
		rc = write(gzfd, encdata, enclen);
		if (rc > 0) {
			cnt+=rc;
		} else {
			/* Write error */
			return(-1);
		}
	}

	free(encdata);		/* This was malloc'ed for us */
	return(0);
}


int
read_idx(char *filename, struct fistfs_header *hdr)
{
	int fd, i;

	if (filename == NULL)
		return(-1);
	if ((fd = open(filename, O_RDONLY)) < 0) {
		return(-1);
	}

	if ((read(fd, &(hdr->num_pages), sizeof(off_t))) < (sizeof(off_t))) {
		fprintf(stderr, "Short read from index file!\n");
		return(-1);
	}

	hdr->flags = hdr->num_pages & 0xfff00000;
	hdr->num_pages &= 0xfffff;

	if ((read(fd, &(hdr->real_size), sizeof(off_t))) < (sizeof(off_t))) {
		fprintf(stderr, "Short read from index file!\n");
		return(-1);
	}

	hdr->offsets = (off_t *) malloc(sizeof(off_t) * hdr->num_pages);
	if (hdr->offsets == NULL) {
		return(-1);
	}

	for (i = 0; i < hdr->num_pages; i++) {
		if ((read(fd, &(hdr->offsets[i]), sizeof(off_t))) != (sizeof(off_t))) {
			fprintf(stderr, "Short read from index file!\n");
			return(-1);
		}
	}
	close(fd);
	return(hdr->num_pages);
}


int
write_idx(char *filename, struct fistfs_header *hdr)
{
	int fd, i;

	if ((fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)
		return(-1);

	if (write(fd, &(hdr->num_pages), sizeof(int)) != (sizeof(int))) {
		fprintf(stderr, "Short write to index file!\n");
		return(-1);
	}

	if (write(fd, &(hdr->real_size), sizeof(int)) != (sizeof(int))) {
		fprintf(stderr, "Short write to index file!\n");
		return(-1);
	}

	for (i = 0; i < hdr->num_pages; i++) {
		if (write(fd, &(hdr->offsets[i]), sizeof(off_t)) != (sizeof(off_t))) {
			fprintf(stderr, "Short write to index file!\n");
			return(-1);
		}
	}

	close(fd);
	return(hdr->num_pages);
}

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */