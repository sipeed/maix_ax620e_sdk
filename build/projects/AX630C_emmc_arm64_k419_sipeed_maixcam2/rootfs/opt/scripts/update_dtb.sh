#!/bin/bash

FIRMWARE_DIR=$1
if [ $# != 1 ]; then
    echo "Usage:  $0 <firmware_dir>"
    echo "Example:  $0 /root"
    exit 1
fi

dd if=$FIRMWARE_DIR/AX630C_emmc_arm64_k419_sipeed_maixcam2_signed.dtb of=/dev/mmcblk0p12
dd if=$FIRMWARE_DIR/AX630C_emmc_arm64_k419_sipeed_maixcam2_signed.dtb of=/dev/mmcblk0p13