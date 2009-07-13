#!/bin/bash -norc
set -x

strace -v ./mapper1 /mnt/kdb3fs/BAR.TXT 0 20

exit 0

# attach a node so /mnt/kdb3fs/abc -> /n/fist/zadok
./fist_ioctl +a /mnt/kdb3fs abc /n/fist/zadok
#read n
#./fist_ioctl +a /mnt/kdb3fs XyZ23q /some/place
exit 1

#ls -l /mnt/kdb3fs/uname

#read n
file=foo-$RANDOM

## shift inward test
#echo 'abcdefghijklmnopqrstuvwxyz0123456789' > /mnt/kdb3fs/$file
#read n
#echo 'XXXXXXXXXX' | ~ib42/c/test/write /mnt/kdb3fs/$file 10

## shift outward test
#echo 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa' > /mnt/kdb3fs/$file
cp /bin/ls /mnt/kdb3fs/$file
read n
echo '1234567890' | ~ib42/c/test/write /mnt/kdb3fs/$file 4110

read n
cat <<EOF
aaaaaaaaaa1234567890
aaaaaaaaaaaaaaa
EO>> /mnt/kdb3fs/G

#  touch /mnt/kdb3fs/$file
#  ls -l /mnt/kdb3fs/$file
#  read n
#  #perl -e "print 'a' x 80" >> /mnt/kdb3fs/$file
#  ~ib42/c/test/truncate /mnt/kdb3fs/$file 100
#  read n
#  #perl -e "print 'b' x 4900" >> /mnt/kdb3fs/$file
#  ~ib42/c/test/truncate /mnt/kdb3fs/$file 5000
#  read n
#  date >> /mnt/kdb3fs/$file
#  read n
#  hexdump /mnt/kdb3fs/$file


#echo
#cp /etc/termcap /mnt/kdb3fs/$file
#read n
#od -Ax -h /n/fist/kdb3fs/$file
#ls -l /mnt/kdb3fs/$file
