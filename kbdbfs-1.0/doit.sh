#!/bin/bash -norc
set -x
PATH=/sbin:.:/usr/local/fist:${PATH}
export PATH

DEBUG=18
MNTDIR=/mnt/kdb3fs
LOWERDIR="/n/fsl/kdb3fs"
CIPHER=plain
KBDBMOD=../../../db_NC/build_unix/kdb3.o


if [ -f doitopts ] ; then
	. doitopts
fi
if [ -f doitopts.`uname -n` ] ; then
	. doitopts.`uname -n`
fi

lsmod
insmod $KBDBMOD || exit
insmod ./kdb3fs.o || exit
lsmod

if [ "$NOPAUSE" != "1" ] ; then
	read n
	sleep 1
fi

# regular style mount
mount -t kdb3fs -o debug=$DEBUG,dir=$LOWERDIR $LOWERDIR $MNTDIR || exit

exit
