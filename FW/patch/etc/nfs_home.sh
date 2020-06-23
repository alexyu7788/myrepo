##!/bin/sh
echo "NFS start"

if [ ! -f /tmp/nfs ];then
    mkdir -p /tmp/nfs
fi

echo "Mount NFS"
mount -t nfs 192.168.88.12:/home/alex/projects/nfs/rpi /tmp/nfs -o nolock,tcp
sleep 1

echo "Bind /usr/sbin"
mount --bind /tmp/nfs/usr/sbin /usr/sbin

echo "Bind /usr/lib"
mount --bind /tmp/nfs/usr/lib /usr/lib
