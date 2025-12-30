#!/bin/bash
### SIPEED EDIT ###
DIST_IP=$1
DIST_PATH=$2
DSIT_URL=root@$DIST_IP:$DIST_PATH

if [ $# != 2 ]; then
    echo "Usage:  $0 <dist_ip> <dist_path>"
    echo "Example:  $0 192.168.10.70 /root"
    exit 1
fi

set -x
sshpass -p sipeed scp ../../build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/spl_AX630C_emmc_arm64_k419_sipeed_maixcam2_signed.bin     $DSIT_URL
# sshpass -p sipeed scp ../../build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/fdl_AX630C_emmc_arm64_k419_sipeed_maixcam2_signed.bin     $DSIT_URL
sshpass -p sipeed scp ../../build/out/AX630C_emmc_arm64_k419_sipeed_maixcam2/images/ddrinit_AX630C_emmc_arm64_k419_sipeed_maixcam2_signed.bin $DSIT_URL
### SIPEED EDIT END ###