#!/bin/sh

echo ""
WARN "ALL THE SOUCE FILES WILL BE DELETED, FILES YOU MOTIFIED/ADDED WILL BE LOST !!!"
echo ""

#echo "To continue, type 'Yes' and then press ENTER ..."

#read choice
#[ x$choice != xYes ] && exit 1

set +e

echo "clean build"
rm build -frv

echo "clean tools"
rm tools -frv

echo "clean app"
rm app -frv

echo "clean rootfs"
rm rootfs -frv

echo "clean boot"
rm boot -frv

echo "clean kernel"
rm kernel -frv

echo "clean riscv"
rm riscv -frv

echo "clean msp"
rm msp -frv

echo "clean third-party"
rm third-party -frv

echo "clean camkit"
rm camera-kit -frv
