#!/bin/bash -e

INPUT_TARGET_DIR=$1
INPUT_DEBIAN_BASE_DIR=$2
TARGET_ROOTFS_DIR=$INPUT_TARGET_DIR
TARGET_ROOTFS_BASE_TAR="${INPUT_DEBIAN_BASE_DIR}/debian_rootfs_base.tar.gz"

if [ $# -ne 2 ]; then
    echo "no input parameter, something wrong"
    exit 2
fi

if [ ! -f "${TARGET_ROOTFS_BASE_TAR}" ]; then
    echo "no debian_rootfs_base.tar.gz, use mk_debian_base.sh to generate it"
    exit 3
fi

if [ -d "${TARGET_ROOTFS_DIR}" ]; then
    rm -rf "${TARGET_ROOTFS_DIR}"
fi

mkdir -p "${TARGET_ROOTFS_DIR}"
tar -zxpf "${TARGET_ROOTFS_BASE_TAR}" -C "${TARGET_ROOTFS_DIR}"

if [ -d "${INPUT_DEBIAN_BASE_DIR}/rootfs_overlay" ]; then
    cp -Rf "${INPUT_DEBIAN_BASE_DIR}/rootfs_overlay/"* "${TARGET_ROOTFS_DIR}" || true
fi

# modify hostname
if [ -d "${TARGET_ROOTFS_DIR}/etc" ]; then
    echo ax620q > "${TARGET_ROOTFS_DIR}/etc/hostname"
fi

# create init link
ln -sf /lib/systemd/systemd "${TARGET_ROOTFS_DIR}/sbin/init"
# link sh to bash
ln -sf /bin/bash "${TARGET_ROOTFS_DIR}/bin/sh"

sync
