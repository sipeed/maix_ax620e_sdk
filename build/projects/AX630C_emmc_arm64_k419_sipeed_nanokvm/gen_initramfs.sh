#!/bin/bash

cd initramfs

out_dir=../../../out/AX630C_emmc_arm64_k419_sipeed_nanokvm/images
out_name=initramfs_rootfs.cpio

chmod +x init
mkdir -p {proc,sys,dev}
mkdir -p $out_dir
find . -print0 | cpio --null -ov --format=newc > $out_dir/$out_name

abs_out_dir="$(readlink -f "$out_dir")"
echo "complete, result in $abs_out_dir/$out_name"
