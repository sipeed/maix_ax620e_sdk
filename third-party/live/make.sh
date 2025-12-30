#!/bin/sh
#live.2020.08.19.tar.gz

WORK_DIR=$(pwd)
NJOBS=4

arch=arm64
libc=glibc

# arm64 or arm
if [ $# -gt 1 ]; then
    arch=$1
    libc=$2
else
    echo "Usage: ./make.sh <arm|arm64> <glibc|uclibc>"
    exit
fi

if [ "${arch}" = "arm" ]; then
if [ "${libc}" = "glibc" ]; then
export TOOLCHAIN=/usr/local/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf
export CROSS=arm-linux-gnueabihf-
else
export TOOLCHAIN=/usr/local/arm-AX620E-linux-uclibcgnueabihf/arm-AX620E-linux-uclibcgnueabihf
export CROSS=arm-AX620E-linux-uclibcgnueabihf-
fi
else
export TOOLCHAIN=/usr/local/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu
export CROSS=aarch64-none-linux-gnu-
fi

export CC=${CROSS}gcc
export CXX=${CROSS}g++
export CPP="${CROSS}g++ -E"
export LD=${CROSS}ld
export AS=${CROSS}as
export AR=${CROSS}ar
export STRIP=${CROSS}strip
export NM=${CROSS}nm
export RANLIB=${CROSS}ranlib
export PKG_CONFIG_PATH=$WORK_DIR/out/${arch}/${libc}/lib/pkgconfig
export PKG_CONFIG_LIBDIR=$WORK_DIR/out/${arch}/${libc}/lib/pkgconfig

cd ${WORK_DIR}/src

if [ "${arch}" = "arm" ]; then
if [ "${libc}" = "glibc" ]; then
./genMakefiles armlinux-no-openssl arm glibc
else
./genMakefiles uclibc-armlinux-no-openssl arm uclibc
fi
else
./genMakefiles aarch64-no-openssl arm64 glibc
fi

make clean

make

make install

make distclean
