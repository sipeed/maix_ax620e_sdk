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

if [ -f $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz ]; then
	rm -rf $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz
fi

#get ubuntu_arm64-base
mkdir -p  $TARGET_ROOTFS_DIR
wget -P $WORKING_DIR http://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/ubuntu-base-22.04-base-arm64.tar.gz
tar -zxf $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz -C $TARGET_ROOTFS_DIR

#cp qemu-aarch64-static and resolv.conf
cp /usr/bin/qemu-aarch64-static  $TARGET_ROOTFS_DIR/usr/bin
cp /etc/resolv.conf $TARGET_ROOTFS_DIR/etc/

finish() {
	bash $WORKING_DIR/ch-mount.sh -u $TARGET_ROOTFS_DIR/
	echo -e "error exit"
	rm -rf $TARGET_ROOTFS_DIR
	rm -f $TARGET_ROOTFS_TAR
	rm -f $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz
	exit -1
}
trap finish ERR

bash $WORKING_DIR/ch-mount.sh -m $TARGET_ROOTFS_DIR/

cat <<EOF | sudo chroot $TARGET_ROOTFS_DIR/

apt-get update --allow-insecure-repositories

yes|apt-get install vim bash-completion systemd sudo kmod net-tools ethtool resolvconf ifupdown isc-dhcp-server
yes|apt-get install language-pack-en-base htop bc udev ssh rsyslog
yes|apt-get install lrzsz
#optee
yes|apt-get install tee-supplicant
#ping
yes|apt-get install inetutils-ping
#iperf3
yes|apt-get install iperf3
#ifplugd
yes|apt-get install ifplugd

#install docker first install iptables and change to iptables-legacy
yes|apt-get install iptables
update-alternatives --set iptables /usr/sbin/iptables-legacy
#Install docker :https://docs.docker.com/engine/install/ubuntu/#install-from-a-package
apt-get update
yes|apt-get install ca-certificates curl gnupg
install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg
chmod a+r /etc/apt/keyrings/docker.gpg
echo \
  "deb [arch=arm64 signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  "$(. /etc/os-release && echo "$VERSION_CODENAME")" stable" | \
  tee /etc/apt/sources.list.d/docker.list > /dev/null
apt-get update
yes|apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

#install rtc ntp
export DEBIAN_FRONTEND=noninteractive
yes|apt-get install ntp
ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
echo "Asia/Shanghai" > /etc/timezone

#add user and chpasswd
echo "root:123456" | chpasswd
chmod 4755 /usr/bin/sudo

#exit and clean
apt clean
sync
history -c
EOF

bash $WORKING_DIR/ch-mount.sh -u $TARGET_ROOTFS_DIR/

rm $TARGET_ROOTFS_DIR/usr/bin/qemu-aarch64-static
sudo tar -zcpf $TARGET_ROOTFS_TAR -C $TARGET_ROOTFS_DIR .
sudo chown $(stat -c '%U:%U' $WORKING_DIR) $TARGET_ROOTFS_TAR
sudo rm -rf $TARGET_ROOTFS_DIR
rm $WORKING_DIR/ubuntu-base-22.04-base-arm64.tar.gz
