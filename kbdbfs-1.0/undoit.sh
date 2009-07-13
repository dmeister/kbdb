#!/bin/bash -norc
set -x
PATH=/sbin:.:/usr/local/fist:${PATH}
export PATH

#fist_ioctl -d /mnt/kdb3fs ${1:-18}

#/bin/rm /mnt/kdb3fs/X-*
#cp -p /n/fist/ext2fs/X-* /mnt/kdb3fs
#cp -p /n/fist/ext2fs/X-26 /mnt/kdb3fs

umount /mnt/kdb3fs
lsmod
rmmod kdb3fs
lsmod
rmmod kdb3