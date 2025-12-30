#!/bin/bash

FIRMWARE_DIR=$1
if [ $# != 1 ]; then
    echo "Usage:  $0 <firmware_dir>"
    echo "Example:  $0 /root"
    exit 1
fi

dd if=$FIRMWARE_DIR/boot_signed.bin of=/dev/mmcblk0p14
dd if=$FIRMWARE_DIR/boot_signed.bin of=/dev/mmcblk0p15