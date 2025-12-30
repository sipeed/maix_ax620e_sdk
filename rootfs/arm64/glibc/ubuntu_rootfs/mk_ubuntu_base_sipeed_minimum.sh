#!/bin/bash -e

WORKING_DIR=$1
TARGET_ROOTFS_DIR=$WORKING_DIR/ubuntu_rootfs_base
TARGET_ROOTFS_TAR=$WORKING_DIR/ubuntu_rootfs_base.tar.gz

if [ $# -ne 1 ]; then
	echo "no input parameter, something wrong"
	exit -2
fi

if [ -f $TARGET_ROOTFS_TAR ]; then
	echo "ubuntu_rootfs.tar.gz is exist, not need to do it again"
	exit
fi

if [ -d $TARGET_ROOTFS_DIR ]; then
	rm -rf $TARGET_ROOTFS_DIR
fi
mkdir -p  $TARGET_ROOTFS_DIR

if [ -f $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz ]; then
    sha256=$(sha256sum "$WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz" | cut -d' ' -f1)
    if [ "$sha256" != "6dd67ec02fdc64b5bba4125066462d01e66a2ae14c4c9e571541fba617d7e721" ]; then
        rm -rf "$WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz"
    fi
fi


#get ubuntu_arm64-base
mkdir -p  $TARGET_ROOTFS_DIR
if [ ! -f "$WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz" ]; then
    wget -P $WORKING_DIR http://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/ubuntu-base-22.04-base-arm64.tar.gz
fi
tar -zxf $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz -C $TARGET_ROOTFS_DIR

#cp qemu-aarch64-static and resolv.conf to use qemu and host DNS server
cp /usr/bin/qemu-aarch64-static  $TARGET_ROOTFS_DIR/usr/bin
cp /etc/resolv.conf $TARGET_ROOTFS_DIR/etc/

finish() {
	bash $WORKING_DIR/ch-mount.sh -u $TARGET_ROOTFS_DIR/
	echo -e "error exit"
	rm -rf $TARGET_ROOTFS_DIR
	rm -f $TARGET_ROOTFS_TAR
	rm -f $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz
	exit 1
}
trap finish ERR

bash $WORKING_DIR/ch-mount.sh -m $TARGET_ROOTFS_DIR/

cat <<EOF | sudo chroot $TARGET_ROOTFS_DIR/

export DEBIAN_FRONTEND=noninteractive
chmod 1777 /tmp
apt update --allow-insecure-repositories
apt install -y --allow-unauthenticated vim bash-completion systemd sudo kmod net-tools ethtool resolvconf ifupdown isc-dhcp-server \
        language-pack-en-base htop bc udev ssh rsyslog lrzsz inetutils-ping
#optee and ifplugd
apt install -y tee-supplicant ifplugd

#install rtc ntp
apt install -y ntp
ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
echo "Asia/Shanghai" > /etc/timezone

#exit and clean
apt autoremove -yq --purge
apt clean -yq
rm -rf /var/lib/apt/lists/*
rm -rf /tmp/*
sync
history -c
EOF

bash $WORKING_DIR/ch-mount.sh -u $TARGET_ROOTFS_DIR/

rm $TARGET_ROOTFS_DIR/usr/bin/qemu-aarch64-static
sudo tar -zcpf $TARGET_ROOTFS_TAR -C $TARGET_ROOTFS_DIR .
sudo chown $(stat -c '%U:%U' $WORKING_DIR) $TARGET_ROOTFS_TAR
sudo rm -rf $TARGET_ROOTFS_DIR
# rm $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz
