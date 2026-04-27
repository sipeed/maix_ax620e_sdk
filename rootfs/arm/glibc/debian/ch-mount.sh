#!/bin/bash

set -e

mnt() {
    echo "MOUNTING $2"
    sudo mount -t proc /proc "${2}/proc"
    sudo mount -t sysfs /sys "${2}/sys"
    sudo mount --bind /dev "${2}/dev"
    sudo mount --bind /dev/pts "${2}/dev/pts"
}

umnt() {
    echo "UNMOUNTING $2"
    sudo umount "${2}/proc" || true
    sudo umount "${2}/sys" || true
    sudo umount "${2}/dev/pts" || true
    sudo umount "${2}/dev" || true
}

if [[ "$1" == "-m" && -n "$2" ]]; then
    mnt "$1" "$2"
elif [[ "$1" == "-u" && -n "$2" ]]; then
    umnt "$1" "$2"
else
    echo "Usage: $0 -m|-u <rootfs_dir>"
    exit 1
fi
