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
DEFAULT_SDCARD_IMG="${SDK_ROOT}/build/out/${PROJECT}_sdcard_${BUILD_TS}.img"
DEFAULT_UPDATE_IMG="${SDK_ROOT}/build/out/${PROJECT}_sd_update_${BUILD_TS}.img"
OUTPUT_IMG=""
UPDATE_MODE=0

usage() {
    cat <<EOF
Usage: $0 [options]

Generate an SD card image for ${PROJECT}.

Options:
      --update             Generate a raw ext4 rootfs image for initramfs USB update
  -o, --output <file>      Output image path (default: sdcard or sd_update image under build/out)
  -s, --size <size>        Image size (default: ${IMG_SIZE}, examples: 4G, 8192M)
  -r, --rootfs <dir>       Rootfs source dir (default: auto debian_rootfs -> ubuntu_rootfs -> buildroot_rootfs -> rootfs)
  -h, --help               Show this help

Notes:
  1) Normal mode generates a full SD card image with an MBR partition table.
  2) --update mode generates only the ext4 rootfs partition image written by initramfs to /dev/mmcblk1p2.
  3) Normal mode requires parted, losetup, mkfs.vfat, mkfs.ext4, rsync, mount, umount.
     --update mode only requires mkfs.ext4, rsync, mount, umount.
  4) Usually needs root privileges to setup loop device and mount filesystems.
  5) Normal partition layout (MBR):
     - p1: FAT32, ${BOOT_PART_SIZE_MIB}MiB, contains recovery sd_boot_pack + boot.bin
     - p2: ext4, remaining size, contains full rootfs + /boot/kernel.img + /boot/dtb.img + /boot/configs
  6) In --update mode, --size is the ext4 rootfs image size, not a whole-card size.
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
        --update)
            UPDATE_MODE=1
            shift
            ;;
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

if [[ -z "${OUTPUT_IMG}" ]]; then
    if [[ "${UPDATE_MODE}" -eq 1 ]]; then
        OUTPUT_IMG="${DEFAULT_UPDATE_IMG}"
    else
        OUTPUT_IMG="${DEFAULT_SDCARD_IMG}"
    fi
fi

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
for boot_image in kernel.img dtb.img; do
    if [[ ! -f "${BOOT_PACK_DIR}/${boot_image}" ]]; then
        echo "[ERROR] Missing ${boot_image} in sd boot pack dir: ${BOOT_PACK_DIR}"
        echo "        Please build project first."
        exit 1
    fi
done

if [[ ! -d "${ROOTFS_DIR}" ]]; then
    echo "[ERROR] rootfs dir does not exist: ${ROOTFS_DIR}"
    exit 1
fi

if [[ "${UPDATE_MODE}" -eq 0 ]]; then
    SPL_BIN="${IMAGES_DIR}/spl_${PROJECT}_sd_signed.bin"
    if [[ ! -f "${SPL_BIN}" ]]; then
        SPL_BIN="${IMAGES_DIR}/spl_${PROJECT}_enc_sd_signed.bin"
    fi
    if [[ ! -f "${SPL_BIN}" ]]; then
        echo "[ERROR] Cannot find SD SPL signed bin in: ${IMAGES_DIR}"
        exit 1
    fi
fi

require_cmd mkfs.ext4
require_cmd rsync
require_cmd mount
require_cmd umount
if [[ "${UPDATE_MODE}" -eq 0 ]]; then
    require_cmd parted
    require_cmd losetup
    require_cmd mkfs.vfat
fi

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

populate_rootfs() {
    rsync -aHAX --no-owner --no-group "${ROOTFS_DIR}/" "${ROOTFS_MNT}/"

    ROOTFS_BOOT_DIR="${ROOTFS_MNT}/boot"
    mkdir -p "${ROOTFS_BOOT_DIR}"
    if [[ -d "${EXTRA_BOOTFS_DIR}" ]]; then
        rsync -rltD --delete "${EXTRA_BOOTFS_DIR}/" "${ROOTFS_BOOT_DIR}/"
    else
        echo "[WARN] extra bootfs dir not found, skip: ${EXTRA_BOOTFS_DIR}"
    fi
    cp -f "${BOOT_PACK_DIR}/kernel.img" "${ROOTFS_BOOT_DIR}/kernel.img"
    cp -f "${BOOT_PACK_DIR}/dtb.img" "${ROOTFS_BOOT_DIR}/dtb.img"

    find "${ROOTFS_MNT}" -xdev -uid 1000 -exec chown -h 0 {} +
    find "${ROOTFS_MNT}" -xdev -gid 1000 -exec chgrp -h 0 {} +

    chown 0:0 "${ROOTFS_MNT}" || true
    chmod 755 "${ROOTFS_MNT}" || true
    if [[ -d "${ROOTFS_MNT}/tmp" ]]; then chmod 1777 "${ROOTFS_MNT}/tmp"; fi
    if [[ -d "${ROOTFS_MNT}/var/tmp" ]]; then chmod 1777 "${ROOTFS_MNT}/var/tmp"; fi
    if [[ -d "${ROOTFS_MNT}/root" ]]; then chmod 700 "${ROOTFS_MNT}/root"; fi
}

if [[ -f "${OUTPUT_IMG}" ]]; then
    rm -f "${OUTPUT_IMG}"
fi

truncate -s "${IMG_SIZE}" "${OUTPUT_IMG}"

if [[ "${UPDATE_MODE}" -eq 1 ]]; then
    mkfs.ext4 -F -L rootfs "${OUTPUT_IMG}"

    mkdir -p "${ROOTFS_MNT}"
    mount -o loop "${OUTPUT_IMG}" "${ROOTFS_MNT}"

    populate_rootfs

    sync

    echo "[OK] SD update rootfs image generated: ${OUTPUT_IMG}"
    echo "[INFO] format: raw ext4 partition image, no MBR partition table"
    echo "[INFO] initramfs target: /dev/mmcblk1p2"
    echo "[INFO] /boot source: ${BOOT_PACK_DIR}/kernel.img, ${BOOT_PACK_DIR}/dtb.img, ${EXTRA_BOOTFS_DIR}"
    echo "[INFO] rootfs source: ${ROOTFS_DIR}"
    exit 0
fi

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

populate_rootfs

sync

echo "[OK] SD image generated: ${OUTPUT_IMG}"
echo "[INFO] boot.bin source: ${SPL_BIN}"
echo "[INFO] p1 recovery boot source: ${BOOT_PACK_DIR}"
echo "[INFO] p2 /boot source: ${BOOT_PACK_DIR}/kernel.img, ${BOOT_PACK_DIR}/dtb.img, ${EXTRA_BOOTFS_DIR}"
echo "[INFO] rootfs source: ${ROOTFS_DIR}"
