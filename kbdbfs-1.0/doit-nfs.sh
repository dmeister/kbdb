#!/bin/bash -norc
set -x
# overlay mount on top of NFS example

PATH=/sbin:.:/usr/local/fist:${PATH}
export PATH

#make module_install
#make module_install_nocheck
#make install
lsmod
insmod ./kdb3fs.o || exit
lsmod

#read n
#sleep 1

mount -t kdb3fs -o dir=/mnt/kdb3fs /mnt/kdb3fs /mnt/kdb3fs || exit

#read n
#sleep 1
fist_ioctl -d /mnt/kdb3fs ${1:-18} || exit
fist_ioctl -f /mnt/kdb3fs 1 || exit

if test -f fist_setkey ; then
    read n
    echo abrakadabra | ./fist_setkey /mnt/kdb3fs
    echo
fi
