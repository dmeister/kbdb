#
# Makefile for the Linux 2.4 kbdbfs
#

# this should point to where your kernel headers are
TOPINC	= -I/lib/modules/$(shell uname -r)/build/include
# allow custmom override of TOPINC for fistgen developers
-include fistdev.mk

CC	= gcc
CFLAGS = -D__KERNEL__ -DMODULE -DFISTGEN -I. ${TOPINC} ${EXTRACFLAGS} -O2 -Wall -Wno-unused -Werror
UCFLAGS = -DFISTGEN -I. ${TOPINC} -g -O2 -Wall -Wno-unused # -Werror

O_TARGET := kbdbfs.o
ADD_M_SRC = 
# vm_area.o is not needed if we use generic_file_mmap
# attach.o are optionally added
O_OBJS   := fist_kdb3fs.o mmap.o \
	dentry.o file.o inode.o main.o super.o print.o \
	${ADD_M_SRC:.c=.o}

M_OBJS   := $(O_TARGET)

BINS	:=	fist_ioctl 

all:	${O_TARGET} ${BINS}

${O_TARGET}: ${O_OBJS}
	ld -o ${O_TARGET} -r ${O_OBJS}

fist_ioctl: fist_ioctl.c
	${CC} -o $@ $? ${UCFLAGS}



memtest:
	perl match-malloc.pl log.txt

TAGS:
	etags -t *.[hc]

clean:
	rm -f ${O_TARGET} ${O_OBJS} *.d .*.flags \#* *~
	rm -f ${BINS} TAGS

# dependencies
${O_OBJS}: kdb3fs.h fist.h fist_kdb3fs.h 

# additional makefile rules, if any
