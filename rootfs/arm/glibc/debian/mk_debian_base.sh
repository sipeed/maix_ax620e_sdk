#!/bin/bash -e

WORKING_DIR=$1
TARGET_ROOTFS_DIR="${WORKING_DIR}/debian_rootfs_base"
TARGET_ROOTFS_TAR="${WORKING_DIR}/debian_rootfs_base.tar.gz"
TARGET_ARCH="${TARGET_ARCH:-armhf}"
DEBIAN_RELEASE="${DEBIAN_RELEASE:-trixie}"
DEBIAN_MIRROR="${DEBIAN_MIRROR:-http://deb.debian.org/debian}"

if [ $# -ne 1 ]; then
    echo "no input parameter, something wrong"
    exit 2
fi

if [ -f "${TARGET_ROOTFS_TAR}" ]; then
    echo "${TARGET_ROOTFS_TAR} is exist, not need to do it again"
    exit 0
fi

if ! command -v debootstrap >/dev/null 2>&1; then
    echo "debootstrap not found, please install debootstrap first"
    exit 3
fi

if [ -d "${TARGET_ROOTFS_DIR}" ]; then
    sudo rm -rf "${TARGET_ROOTFS_DIR}"
fi
mkdir -p "${TARGET_ROOTFS_DIR}"

sudo debootstrap --arch="${TARGET_ARCH}" --foreign --variant=minbase \
    "${DEBIAN_RELEASE}" "${TARGET_ROOTFS_DIR}" "${DEBIAN_MIRROR}"

if [ ! -f /usr/bin/qemu-arm-static ]; then
    echo "/usr/bin/qemu-arm-static not found, please install qemu-user-static"
    exit 4
fi

sudo cp /usr/bin/qemu-arm-static "${TARGET_ROOTFS_DIR}/usr/bin/"
sudo cp /etc/resolv.conf "${TARGET_ROOTFS_DIR}/etc/"

finish() {
    bash "${WORKING_DIR}/ch-mount.sh" -u "${TARGET_ROOTFS_DIR}"
    echo "error exit"
    sudo rm -rf "${TARGET_ROOTFS_DIR}"
    sudo rm -f "${TARGET_ROOTFS_TAR}"
    exit 1
}
trap finish ERR

bash "${WORKING_DIR}/ch-mount.sh" -m "${TARGET_ROOTFS_DIR}"

cat <<EOF | sudo chroot "${TARGET_ROOTFS_DIR}" /bin/bash
set -e
export DEBIAN_FRONTEND=noninteractive
/debootstrap/debootstrap --second-stage
EOF

bash "${WORKING_DIR}/ch-mount.sh" -u "${TARGET_ROOTFS_DIR}"
bash "${WORKING_DIR}/ch-mount.sh" -m "${TARGET_ROOTFS_DIR}"

cat <<EOF | sudo chroot "${TARGET_ROOTFS_DIR}" /bin/bash
set -e
export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y --no-install-recommends \
    systemd-sysv systemd-resolved sudo kmod udev dialog busybox net-tools ifupdown wget curl iperf3 avahi-daemon \
    iproute2 iptables iputils-ping openssh-server ca-certificates vim chrony \
    bash-completion tzdata udhcpc udhcpd wpasupplicant rsync evtest usbutils \
    binutils e2fsprogs iw htop fdisk fastfetch file python-is-python3

ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
echo "Asia/Shanghai" > /etc/timezone

echo "root:sipeed" | chpasswd
chmod 4755 /usr/bin/sudo

cat > /etc/network/interfaces <<'NETEOF'
auto lo
iface lo inet loopback

allow-hotplug eth0
iface eth0 inet dhcp
NETEOF

systemctl mask apt-daily.timer
systemctl mask apt-daily-upgrade.timer

apt-get clean
rm -rf /var/lib/apt/lists/*
sync
EOF

bash "${WORKING_DIR}/ch-mount.sh" -u "${TARGET_ROOTFS_DIR}"

sudo rm -f "${TARGET_ROOTFS_DIR}/usr/bin/qemu-arm-static"
sudo tar -zcpf "${TARGET_ROOTFS_TAR}" -C "${TARGET_ROOTFS_DIR}" .
sudo chown "$(stat -c '%U:%G' "${WORKING_DIR}")" "${TARGET_ROOTFS_TAR}"
sudo rm -rf "${TARGET_ROOTFS_DIR}"
