#!/usr/bin/env bash
set -euo pipefail

PROJECT="AX620Q_emmc_arm32_k419_sipeed_nanoagent"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
OUT_BASE="${SDK_ROOT}/build/out/${PROJECT}"
IMAGES_DIR="${OUT_BASE}/images"
OBJS_DIR="${OUT_BASE}/objs"

BOOT_PACK_DIR="${IMAGES_DIR}/sd_boot_pack"
EXTRA_BOOTFS_DIR="${SCRIPT_DIR}/bootfs"
DEBIAN_ROOTFS_DIR="${OBJS_DIR}/debian_rootfs"
UBUNTU_ROOTFS_DIR="${OBJS_DIR}/ubuntu_rootfs"
BUILDROOT_ROOTFS_DIR="${OBJS_DIR}/buildroot_rootfs"
ROOTFS_FALLBACK_DIR="${OBJS_DIR}/rootfs"

IMG_SIZE="1024M"
BOOT_PART_SIZE_MIB=128
BUILD_TS="$(date +%Y%m%d_%H%M%S)"
OUTPUT_IMG="${SDK_ROOT}/build/out/${PROJECT}_sdcard_${BUILD_TS}.img"

usage() {
    cat <<EOF
Usage: $0 [options]

Generate an SD card image for ${PROJECT}.

Options:
  -o, --output <file>      Output image path (default: ${OUTPUT_IMG})
  -s, --size <size>        Image size (default: ${IMG_SIZE}, examples: 4G, 8192M)
  -r, --rootfs <dir>       Rootfs source dir (default: auto debian_rootfs -> ubuntu_rootfs -> buildroot_rootfs -> rootfs)
  -h, --help               Show this help

Notes:
  1) Requires tools: parted, losetup, mkfs.vfat, mkfs.ext4, rsync, mount, umount
  2) Usually needs root privileges to setup loop device and mount filesystems
  3) Partition layout (MBR):
     - p1: FAT32, ${BOOT_PART_SIZE_MIB}MiB, contains sd_boot_pack + boot.bin
     - p2: ext4, remaining size, contains full rootfs
EOF
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "[ERROR] Missing command: $1"
        exit 1
    }
}

ROOTFS_DIR=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -o|--output)
            OUTPUT_IMG="$2"
            shift 2
            ;;
        -s|--size)
            IMG_SIZE="$2"
            shift 2
            ;;
        -r|--rootfs)
            ROOTFS_DIR="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "[ERROR] Unknown argument: $1"
            usage
            exit 1
            ;;
    esac
done

if [[ -z "${ROOTFS_DIR}" ]]; then
    if [[ -d "${DEBIAN_ROOTFS_DIR}" ]]; then
        ROOTFS_DIR="${DEBIAN_ROOTFS_DIR}"
    elif [[ -d "${UBUNTU_ROOTFS_DIR}" ]]; then
        ROOTFS_DIR="${UBUNTU_ROOTFS_DIR}"
    elif [[ -d "${BUILDROOT_ROOTFS_DIR}" ]]; then
        ROOTFS_DIR="${BUILDROOT_ROOTFS_DIR}"
    elif [[ -d "${ROOTFS_FALLBACK_DIR}" ]]; then
        ROOTFS_DIR="${ROOTFS_FALLBACK_DIR}"
    else
        echo "[ERROR] Cannot find rootfs dir. Expected one of:"
        echo "        ${DEBIAN_ROOTFS_DIR}"
        echo "        ${UBUNTU_ROOTFS_DIR}"
        echo "        ${BUILDROOT_ROOTFS_DIR}"
        echo "        ${ROOTFS_FALLBACK_DIR}"
        exit 1
    fi
fi

if [[ ! -d "${BOOT_PACK_DIR}" ]]; then
    echo "[ERROR] Missing sd boot pack dir: ${BOOT_PACK_DIR}"
    echo "        Please build project first."
    exit 1
fi

if [[ ! -d "${ROOTFS_DIR}" ]]; then
    echo "[ERROR] rootfs dir does not exist: ${ROOTFS_DIR}"
    exit 1
fi

SPL_BIN="${IMAGES_DIR}/spl_${PROJECT}_sd_signed.bin"
if [[ ! -f "${SPL_BIN}" ]]; then
    SPL_BIN="${IMAGES_DIR}/spl_${PROJECT}_enc_sd_signed.bin"
fi
if [[ ! -f "${SPL_BIN}" ]]; then
    echo "[ERROR] Cannot find SD SPL signed bin in: ${IMAGES_DIR}"
    exit 1
fi

require_cmd parted
require_cmd losetup
require_cmd mkfs.vfat
require_cmd mkfs.ext4
require_cmd rsync
require_cmd mount
require_cmd umount

mkdir -p "$(dirname "${OUTPUT_IMG}")"

WORKDIR="$(mktemp -d -t ${PROJECT}_sdimg_XXXXXX)"
LOOP_DEV=""
BOOT_MNT="${WORKDIR}/boot"
ROOTFS_MNT="${WORKDIR}/rootfs"

cleanup() {
    set +e
    sync
    if mountpoint -q "${BOOT_MNT}"; then umount "${BOOT_MNT}"; fi
    if mountpoint -q "${ROOTFS_MNT}"; then umount "${ROOTFS_MNT}"; fi
    if [[ -n "${LOOP_DEV}" ]]; then losetup -d "${LOOP_DEV}"; fi
    rm -rf "${WORKDIR}"
}
trap cleanup EXIT

if [[ -f "${OUTPUT_IMG}" ]]; then
    rm -f "${OUTPUT_IMG}"
fi

truncate -s "${IMG_SIZE}" "${OUTPUT_IMG}"

parted -s "${OUTPUT_IMG}" \
    mklabel msdos \
    mkpart primary fat32 1MiB "$((BOOT_PART_SIZE_MIB + 1))MiB" \
    mkpart primary ext4 "$((BOOT_PART_SIZE_MIB + 1))MiB" 100% \
    set 1 boot on

LOOP_DEV="$(losetup --show -fP "${OUTPUT_IMG}")"

BOOT_PART="${LOOP_DEV}p1"
ROOTFS_PART="${LOOP_DEV}p2"

if [[ ! -b "${BOOT_PART}" || ! -b "${ROOTFS_PART}" ]]; then
    echo "[ERROR] Loop partitions not found: ${BOOT_PART}, ${ROOTFS_PART}"
    exit 1
fi

mkfs.vfat -F 32 -n BOOT "${BOOT_PART}"
mkfs.ext4 -F -L rootfs "${ROOTFS_PART}"

mkdir -p "${BOOT_MNT}" "${ROOTFS_MNT}"
mount "${BOOT_PART}" "${BOOT_MNT}"
mount "${ROOTFS_PART}" "${ROOTFS_MNT}"

rsync -rltD --delete "${BOOT_PACK_DIR}/" "${BOOT_MNT}/"
cp -f "${SPL_BIN}" "${BOOT_MNT}/boot.bin"
if [[ -d "${EXTRA_BOOTFS_DIR}" ]]; then
    rsync -rltD "${EXTRA_BOOTFS_DIR}/" "${BOOT_MNT}/"
else
    echo "[WARN] extra bootfs dir not found, skip: ${EXTRA_BOOTFS_DIR}"
fi

rsync -aHAX --no-owner --no-group "${ROOTFS_DIR}/" "${ROOTFS_MNT}/"

find "${ROOTFS_MNT}" -xdev -uid 1000 -exec chown -h 0 {} +
find "${ROOTFS_MNT}" -xdev -gid 1000 -exec chgrp -h 0 {} +

chown 0:0 "${ROOTFS_MNT}" || true
chmod 755 "${ROOTFS_MNT}" || true
if [[ -d "${ROOTFS_MNT}/tmp" ]]; then chmod 1777 "${ROOTFS_MNT}/tmp"; fi
if [[ -d "${ROOTFS_MNT}/var/tmp" ]]; then chmod 1777 "${ROOTFS_MNT}/var/tmp"; fi
if [[ -d "${ROOTFS_MNT}/root" ]]; then chmod 700 "${ROOTFS_MNT}/root"; fi

sync

echo "[OK] SD image generated: ${OUTPUT_IMG}"
echo "[INFO] boot.bin source: ${SPL_BIN}"
echo "[INFO] extra bootfs source: ${EXTRA_BOOTFS_DIR}"
echo "[INFO] rootfs source: ${ROOTFS_DIR}"
