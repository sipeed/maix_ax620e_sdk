#!/bin/bash

FIRMWARE_DIR=$1
if [ $# != 1 ]; then
    echo "Usage:  $0 <firmware_dir>"
    echo "Example:  $0 /root"
    exit 1
fi

dd if=$FIRMWARE_DIR/spl_AX630C_emmc_arm64_k419_sipeed_nanokvm_signed.bin of=/dev/mmcblk0p1
dd if=$FIRMWARE_DIR/ddrinit_AX630C_emmc_arm64_k419_sipeed_nanokvm_signed.bin of=/dev/mmcblk0p2