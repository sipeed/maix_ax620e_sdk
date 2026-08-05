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
sudo cp requirements.txt "${TARGET_ROOTFS_DIR}/requirements.txt"

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
    systemd-sysv systemd-resolved systemd-zram-generator sudo kmod udev dialog \
    busybox net-tools ifupdown wget curl iperf3 avahi-daemon iproute2 iptables \
    iputils-ping openssh-server ca-certificates vim chrony bash-completion \
    tzdata udhcpc udhcpd wpasupplicant rsync evtest usbutils binutils e2fsprogs \
    iw htop fdisk fastfetch file python-is-python3 xxd hostapd i2c-tools \
    etherwake arping exfatprogs libevdev2 python3-pip python3-dbus libavformat61 \
    libcurl4t64 dirmngr gnupg gnupg-l10n gnupg-utils gpg gpg-agent gpg-wks-client \
    gpgconf gpgsm gpgv libassuan9 libgcrypt20 libgpg-error-l10n libgpg-error0 \
    libksba8 libnpth0t64 pinentry-curses libffi8 libssl3t64 libsrtp2-1

wget -O /tmp/tailscale.deb https://pkgs.tailscale.com/stable/debian/pool/tailscale_1.98.10_armhf.deb
apt-get install -y --no-install-recommends /tmp/tailscale.deb
rm -f /tmp/tailscale.deb

apt-get install -y --no-install-recommends \
    gcc python3-dev pkg-config libssl-dev libffi-dev libsrtp2-dev

pip install --break-system-packages --no-cache-dir -i https://pypi.mirrors.ustc.edu.cn/simple -r /requirements.txt
rm -f /requirements.txt

apt-get purge -y --auto-remove \
    -o APT::AutoRemove::RecommendsImportant=false \
    gcc python3-dev pkg-config libssl-dev libffi-dev libsrtp2-dev

ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
echo "Asia/Shanghai" > /etc/timezone

echo "root:sipeed" | chpasswd
chmod 4755 /usr/bin/sudo

systemctl set-default multi-user.target
systemctl disable wpa_supplicant.service
systemctl disable tailscaled.service
systemctl mask apt-daily.timer
systemctl mask apt-daily-upgrade.timer

update-alternatives --set iptables /usr/sbin/iptables-legacy
update-alternatives --set ip6tables /usr/sbin/ip6tables-legacy

apt-get clean
rm -rf /var/lib/apt/lists/*
sync
EOF

bash "${WORKING_DIR}/ch-mount.sh" -u "${TARGET_ROOTFS_DIR}"

sudo rm -f "${TARGET_ROOTFS_DIR}/usr/bin/qemu-arm-static"
sudo tar -zcpf "${TARGET_ROOTFS_TAR}" -C "${TARGET_ROOTFS_DIR}" .
sudo chown "$(stat -c '%U:%G' "${WORKING_DIR}")" "${TARGET_ROOTFS_TAR}"
sudo rm -rf "${TARGET_ROOTFS_DIR}"
