#!/bin/bash
LOCAL_PATH=$(pwd)

UBOOT_PACKAGE=u-boot-2020.04.tar.bz2
ATF_PACKAGE=v2.7.0.zip
OPTEE_PACKAGE=3.21.0.zip
UBOOT_PATH=
ATF_PATH=

KERNEL_PACKAGE=linux-4.19.125.tar.gz
KERNEL_PATH=
CAMKIT_TAR_PATH=

usage() {
    echo "
usage:
  -a  ATF source code absolute path
  -o  optee source code absolute path
  -u  u-boot-2020.04 source code absolute path
  -k  linux-4.19.125 source code absolute path
  -d  camkit.tar.gz absolute path
  example:
      ./sdk_unpack.sh -a /path_to/atf/v2.7.0.zip -o /path_to/optee/3.21.0.zip  \
-u /path_to/uboot/u-boot-2020.04.tar.bz2 -k /path_to/linux/linux-4.19.125.tar.gz
"
}
while getopts "a:o:u:k:h:n:d:" arg
do
        case $arg in
             a)
                echo "a's arg:$OPTARG"
                ATF_PATH=$OPTARG
                ;;
             o)
                echo "o's arg:$OPTARG"
                OPTEE_PATH=$OPTARG
                ;;
             u)
                echo "u's arg:$OPTARG"
                UBOOT_PATH=$OPTARG
                ;;
             k)
                echo "k's arg:$OPTARG"
                KERNEL_PATH=$OPTARG
                ;;
             d)
                echo "d's arg:$OPTARG"
                CAMKIT_TAR_PATH=$OPTARG
                ;;
             ?)
                echo "unkonw argument: $arg"
                usage
                exit 1
                ;;
        esac
done

function unpack_msp {
    echo "=================SDK: unpaking msp ==================="
    tar -xvzf package/msp.tar.gz
}

function unpack_rootfs {
    echo "=================SDK: unpaking rootfs ==================="
    tar -xvzf package/rootfs.tar.gz
}

function unpack_build {
    echo "=================SDK: unpaking build ==================="
    tar -xvzf package/build.tar.gz
}

function unpack_tools {
    echo "=================SDK: unpaking tools ==================="
    tar -xvzf package/tools.tar.gz
}

function unpack_app {
    echo "=================SDK: unpaking app ==================="
    tar -xvzf package/app.tar.gz
}

function unpack_3rd_party {
    echo "=================SDK: unpaking third-party ==================="
    tar -xvzf package/third-party.tar.gz
}

function unpack_kernel {
    echo "=================SDK: unpaking kernel ==================="

    if [ "$KERNEL_PATH" != "" ]; then
        if [ ! -f $KERNEL_PATH ]; then
            echo "[ERROR] $KERNEL_PATH does not exist, please input $KERNEL_PACKAGE absolute path"
            exit 1
        fi
        result=$(echo $KERNEL_PATH | grep "${KERNEL_PACKAGE}")
        if [ "$result" == "" ]; then
            echo "[ERROR] please input $KERNEL_PACKAGE absolute path"
            exit 1
        fi
    fi

    echo "kernel path $KERNEL_PATH ..."
    tar -xvzf package/kernel.tar.gz
    cd $LOCAL_PATH/kernel/linux

    ################################################################
    if [ "$KERNEL_PATH" != "" ]; then
        if [ ! -f $KERNEL_PATH ]; then
            echo "[ERROR] $KERNEL_PATH does not exist, please input $KERNEL_PACKAGE absolute path"
            exit 1
        fi
        result=$(echo $KERNEL_PATH | grep "${KERNEL_PACKAGE}")
        if [ "$result" == "" ]; then
            echo "[ERROR] please input $KERNEL_PACKAGE absolute path"
            exit 1
        fi
    fi

    echo "kernel path $KERNEL_PATH ..."
    if [ ! -n "$KERNEL_PATH" ]; then
        echo "download kernel linux-4.19.125 ..."
        wget -t 5 https://mirror.tuna.tsinghua.edu.cn/kernel/v4.x/linux-4.19.125.tar.gz --no-check-certificate
        if [[ $? -ne 0 ]]; then
            echo "download linux kernel failed, please retry..."
            exit 1;
        fi
    else
        cp -f $KERNEL_PATH ./
    fi
    tar -xzf linux-4.19.125.tar.gz
    echo "patching linux-4.19.125 ..."
    patch -Np1 < linux-4.19.125.patch
    cd $LOCAL_PATH
}

function unpack_riscv {
    echo "=================SDK: unpaking riscv ==================="
    tar -xvzf package/riscv.tar.gz
}

function unpack_uboot {
    echo "=================SDK: unpaking uboot ==================="

    if [ "$UBOOT_PATH" != "" ]; then
        if [ ! -f $UBOOT_PATH ]; then
            echo "[ERROR] $UBOOT_PATH does not exist, please input $UBOOT_PACKAGE absolute path"
            exit 1
        fi
        result=$(echo $UBOOT_PATH | grep "${UBOOT_PACKAGE}")
        if [ "$result" == "" ]; then
            echo "[ERROR] please input $UBOOT_PACKAGE absolute path"
            exit 1
        fi
    fi

    cd $LOCAL_PATH/boot/uboot
    if [ ! -n "$UBOOT_PATH" ]; then
        wget -t 5 https://ftp.denx.de/pub/u-boot/u-boot-2020.04.tar.bz2
        if [[ $? -ne 0 ]]; then
            echo "download uboot u-boot-2020.04 failed, please retry..."
            exit 1;
        fi
    else
        cp -f $UBOOT_PATH ./
    fi

    tar -xvf u-boot-2020.04.tar.bz2
    echo "patch u-boot-2020.04 ..."
    patch -Np1 < u-boot-2020.04.patch
    cp -f $LOCAL_PATH/boot/uboot/axera_logo.bmp $LOCAL_PATH/boot/uboot/u-boot-2020.04/tools/logos/axera_logo.bmp

}

function unpack_atf {
    echo "=================SDK: unpaking atf ==================="
    if [ "$ATF_PATH" != "" ]; then
        if [ ! -f $ATF_PATH ]; then
            echo "[ERROR] $ATF_PATH does not exist, please input $ATF_PACKAGE absolute path"
            exit 1
        fi
        result=$(echo $ATF_PATH | grep "${ATF_PACKAGE}")
        if [ "$result" == "" ]; then
            echo "[ERROR] please input $ATF_PACKAGE absolute path"
            exit 1
        fi
    fi
    cd $LOCAL_PATH/boot/atf
    if [ ! -n "$ATF_PATH" ]; then
        echo "download atf arm-trusted-firmware-v2.7.0 ..."
        wget -t 5 https://github.com/ARM-software/arm-trusted-firmware/archive/refs/tags/v2.7.0.zip
        if [[ $? -ne 0 ]]; then
            echo "download arm-trusted-firmware-v2.7.0 failed, please retry..."
            exit 1;
        fi
        ATF_PATH=v2.7.0.zip
    else
        cp -f $ATF_PATH ./
    fi

    unzip $ATF_PATH
    mv arm-trusted-firmware-2.7.0 arm-trusted-firmware-2.7
    echo "patch arm-trusted-firmware ..."
    patch -Np1 < arm-trusted-firmware-2.7.patch

}

function unpack_optee {
    echo "=================SDK: unpaking optee ==================="

    if [ "$OPTEE_PATH" != "" ]; then
        if [ ! -f $OPTEE_PATH ]; then
            echo "[ERROR] $OPTEE_PATH does not exist, please input $OPTEE_PACKAGE absolute path"
            exit 1
        fi
        result=$(echo $OPTEE_PATH | grep "${OPTEE_PACKAGE}")
        if [ "$result" == "" ]; then
            echo "[ERROR] please input $OPTEE_PACKAGE absolute path"
            exit 1
        fi
    fi

    cd $LOCAL_PATH/boot/optee
    if [ ! -n "$OPTEE_PATH" ]; then
        echo "download optee 3.21.0 ..."
        wget -t 5 https://github.com/OP-TEE/optee_os/archive/refs/tags/3.21.0.zip
        if [[ $? -ne 0 ]]; then
            echo "download optee 3.21.0 failed, please retry..."
            exit 1;
        fi
    else
        cp -f $OPTEE_PATH ./
    fi

    unzip 3.21.0.zip
    echo "patch optee_os-3.21.0 ..."
    patch -Np1 < optee_os-3.21.0.patch

}

function unpack_boot {
    echo "=================SDK: unpaking boot ==================="

    cd $LOCAL_PATH
    tar -xvzf package/boot.tar.gz
    unpack_uboot
    unpack_atf
    unpack_optee
}

function unpack_camera_kit {
    if [ "$CAMKIT_TAR_PATH" == "" ]; then
        return 0
	fi

    tar tf "$CAMKIT_TAR_PATH" &> /dev/null
    if [ $? -ne 0 ]; then
        echo "$CAMKIT_TAR_PATH is not a valid tar.gz file."
        return 1
	fi

    echo "=================SDK: unpaking camera kit ==================="
    cd $LOCAL_PATH
    TAR_TOP=$(tar -tzf $CAMKIT_TAR_PATH | grep -o '^[^/]*/' | uniq)
    tar -xvzf $CAMKIT_TAR_PATH -C ./
    tar -xvzf $TAR_TOP/package/camera_kit.tar.gz
    rm -rf $TAR_TOP
}

function sdk_unpack {
  unpack_msp
  unpack_rootfs
  unpack_build
  unpack_tools
  unpack_app
  unpack_3rd_party
  unpack_kernel
  unpack_riscv
  unpack_boot
  unpack_camera_kit
}

sdk_unpack
