#!/bin/bash

if [ $# -ne 1 ]; then
	echo "no input parameter"
	exit 2
fi

TARGET_ROOTFS_DIR=$1
./ch-mount.sh -m ${TARGET_ROOTFS_DIR}/

cp /usr/bin/qemu-aarch64-static  $TARGET_ROOTFS_DIR/usr/bin
cp /etc/resolv.conf $TARGET_ROOTFS_DIR/etc/

sudo chroot --userspec=0:0 $1



