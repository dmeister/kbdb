#ifndef _NFSDB_H_
#define _NFSDB_H_

//#define DBRANDTOFH	"/mnt/crackdb/randtofh.db"
//#define DBFHTORAND	"/mnt/crackdb/fhtorand.db"
#define DBRANDTOFH	"/randtofh.db"
#define DBFHTORAND	"/fhtorand.db"
#define DBRANDSIZE	db_rsize_proc
#define MYPAGESIZE	PAGE_SIZE

#ifdef NFSPRINT
#define dprintk(args...)       printk(KERN_EMERG ##args)
#endif

#include <db.h>

extern DB *dbrfhp, *dbfhrp;
extern int db_rsize_proc;
extern int numins_rfh;
extern int numins_fhr;

extern void nfscrack_dprint(char *str,...);

#define ASSERT(EX)	\
do {	\
    if (!(EX)) {	\
	printk(KERN_CRIT "ASSERTION FAILED: %s at %s:%d (%s)\n", #EX,	\
	       __FILE__, __LINE__, __FUNCTION__);	\
	(*((char *)0))=0;	\
    }	\
} while (0)

static inline void printhexstr(const unsigned char *str, int len) {
#ifdef NFSPRINT
        int i;

        nfscrack_dprint("0x");
        for (i = 0; i < len; i++) {
                nfscrack_dprint("%02x ", str[i]);
        }
        nfscrack_dprint("\n");
#endif
}


int nfscrack_db_open(void);
void nfscrack_db_close(void);

inline int nfscrack_rand2fh_svc_fh(struct svc_fh *f);
inline int nfscrack_rand2fh_knfsd_fh(struct knfsd_fh *f);
int nfscrack_rand2fh_raw(char *infh, int insize, char *outfh, int *outsize);

#define DO_INSERT 1
#define NO_INSERT 0

inline int nfscrack_fh2rand_knfsd_fh(struct knfsd_fh *f, int doinsert);
inline int nfscrack_fh2rand_svc_fh(struct svc_fh *f, int doinsert);
int nfscrack_fh2rand_raw(char *infh, int insize, char *outfh, int *outsize, int doinsert);

#endif
