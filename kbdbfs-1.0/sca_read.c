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
 *  $Id: sca_read.c,v 1.1 2003/11/30 22:52:30 aditya Exp $
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "sca_aux.h"
#include "sca_code.h"	/* replace with your definitions of en/decode _page */


int
decode_file(char *name)
{
	char idx[MAXPATHLEN];
	int srcfd;
	struct fistfs_header hdr;
	int i, len;
	unsigned char *out;

	if (name == NULL)
		return(-1);

	if ((srcfd = open(name, O_RDONLY)) < 0)
		return(-1);

	sprintf(idx, "%s.idx", name);
	if ((read_idx(idx, &hdr)) < 0)
		return(-1); /* read the header */

	for (i = 0; i < hdr.num_pages; i++) {
		if ((get_page(srcfd, &hdr, i, &out, &len)) < 0) {
			fprintf(stderr, "get_page returns error!\n");
			return(-1);
		}

		if (len > chunksize) {
			fprintf(stderr, "***decode returned %d bytes\n", len);
		}
		write(1, out, len);
		free(out);
	}

	if (do_fast_tails) {	/* See if there unencoded data at the end */
		/* This get_page will return 0 if there is no unencoded tail */
		if ((get_page(srcfd, &hdr, i, &out, &len)) < 0) {
			fprintf(stderr,"get_page returns error!\n");
			return(-1);
		}
		write(1, out, len);
		free(out);
	}
	return(0);
}


void
usage(const char *progname)
{
	fprintf(stderr,
		"Usage: %s [-c chunksize] [-d] [-f] file1 [file2 file3 ...]\n",
		progname);
}


int
main(int argc, char **argv)
{
	int i, rc, cnt = 0;
	int debug = 0;
	char *name;

	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}

	while ((i = getopt(argc, argv, "c:df")) != EOF) {
		switch(i) {
		case 'c':
			chunksize = atoi(optarg);
			if (chunksize < 0) {
				fprintf(stderr, "Please use a positive chunk size\n");
				exit(1);
			}
			break;
		case 'f':
			do_fast_tails = 1;
			break;
		case 'd':
			debug = 1;
			break;
		default:
			usage(argv[0]);
			exit(1);
		}
	}

	for (i = optind; i < argc; i++) {
		name = strdup(argv[i]);
		if ((rc = decode_file(argv[i])) < 0) {
			fprintf(stderr, "\tError decoding %s (%d)\n", argv[i], rc);
			cnt++;
		}
		free(name);
	}

	if (debug) {
		fprintf(stderr,"encodes: %d\n", encode_counter);
		fprintf(stderr,"decodes: %d\n", decode_counter);
		fprintf(stderr,"writes:  %d\n", write_counter);
	}
	exit(cnt);
}

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
