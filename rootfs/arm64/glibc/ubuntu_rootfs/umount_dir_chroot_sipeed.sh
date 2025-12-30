#!/bin/bash

if [ $# -ne 1 ]; then
	echo "no input parameter"
	exit 2
fi

bash ch-mount.sh -u $1/
rm -rf $1/usr/bin/qemu-aarch64-static


