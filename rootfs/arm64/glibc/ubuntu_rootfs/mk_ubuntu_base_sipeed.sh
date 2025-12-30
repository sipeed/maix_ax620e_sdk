#!/bin/bash -e

set -e

function check_sha256()
{
    local file=$1
    local sha256=$2
    local sha256sum=$(sha256sum $file | cut -d' ' -f1)
    if [ "$sha256sum" != "$sha256" ]; then
        echo "sha256sum mismatch: $file"
        exit 1
    fi
}

function check_file_exist()
{
    local file=$1
    local sha256=$2
    local download_url=$3
    if [ -f $file ]; then
        local sha256sum=$(sha256sum $file | cut -d' ' -f1)
        if [ "$sha256sum" != "$sha256" ]; then
            echo "$file found but sha256sum mismatch, auto remove it and download again"
            rm -rf $file
        fi
    fi
    if [ ! -f $file ]; then
        wget -P $WORKING_DIR $download_url -O $file
    fi
    check_sha256 $file $sha256
}

WORKING_DIR=$1
TARGET_ROOTFS_DIR=$WORKING_DIR/ubuntu_rootfs_base
TARGET_ROOTFS_TAR=$WORKING_DIR/ubuntu_rootfs_base.tar.gz
PYTHON_VERSION=3.13.2

if [ $# -ne 1 ]; then
	echo "no input parameter, something wrong"
	exit 2
fi

if [ -f $TARGET_ROOTFS_TAR ]; then
	echo "${TARGET_ROOTFS_TAR} is exist, not need to do it again"
	exit
fi

if [ -d $TARGET_ROOTFS_DIR ]; then
	sudo rm -rf $TARGET_ROOTFS_DIR
fi
mkdir -p  $TARGET_ROOTFS_DIR

#get ubuntu_arm64-base
pkg_filename=ubuntu-base-22.04-base-arm64.tar.gz
pkg_filepath=$WORKING_DIR/pkgs/$pkg_filename
pkg_sha256="6dd67ec02fdc64b5bba4125066462d01e66a2ae14c4c9e571541fba617d7e721"
check_file_exist $pkg_filepath $pkg_sha256 http://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/
tar -zxf $pkg_filepath -C $TARGET_ROOTFS_DIR

# # downlaod python source
# if [ -f "$WORKING_DIR/Python-${PYTHON_VERSION}.tgz" ]; then
#     sha256=$(sha256sum "$WORKING_DIR/Python-${PYTHON_VERSION}.tgz" | cut -d' ' -f1)
#     if [ "$sha256" != "b8d79530e3b7c96a5cb2d40d431ddb512af4a563e863728d8713039aa50203f9" ]; then
#         rm -rf "$WORKING_DIR/Python-${PYTHON_VERSION}.tgz"
#     fi
# fi
# if [ ! -f "$WORKING_DIR/Python-${PYTHON_VERSION}.tgz" ]; then
#   wget -P $WORKING_DIR https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz
# fi

# download python bin files
python_filename=python3.13.2_maixcam2_gcc11.4.0.tar.xz
python_filepath=${WORKING_DIR}/pkgs/${python_filename}
python_sha256=a37165fddf7401b3932b94a10e9cbb166d9ee825e34e23a27de2dadcacce411a
check_file_exist $python_filepath $python_sha256 "https://github.com/sipeed/MaixCDK/releases/download/v0.0.0/${python_filename}"
tar -Jxvf $python_filepath -C $TARGET_ROOTFS_DIR/usr/local

# downlaod opencv minimum version from MaixCDK
opencv_filename=opencv4_lib_maixcam2_glibc_4.11.0.tar.xz
opencv_filepath=${WORKING_DIR}/pkgs/${opencv_filename}
opencv_dirname=opencv4_lib_maixcam2_glibc_4.11.0
opencv_sha256=4aefecf397adf2efda8fcdca1ddf4c4b2b9a7af2fb478c6a6d251966161ab72c
check_file_exist $opencv_filepath $opencv_sha256 "https://github.com/sipeed/MaixCDK/releases/download/v0.0.0/$opencv_filename"
tar -Jxvf ${opencv_filepath} -C .
mv ${opencv_dirname}/bin/* $TARGET_ROOTFS_DIR/usr/bin
mv ${opencv_dirname}/include/* $TARGET_ROOTFS_DIR/usr/include
mv ${opencv_dirname}/dl_lib/* $TARGET_ROOTFS_DIR/usr/lib
mv ${opencv_dirname}/share/* $TARGET_ROOTFS_DIR/usr/share
rm -rf ${opencv_dirname}

# download ffmpeg minimum version from MaixCDK
pkg_filename=ffmpeg_maixcam2_libs_n6.1.1.1.tar.xz
pkg_dirname=ffmpeg_maixcam2_libs_n6.1.1.1
pkg_filepath=${WORKING_DIR}/pkgs/${pkg_filename}
pkg_sha256=a768eef0ab5841d63cb99dc4ee66b0b252b56ecce0bb8d344eb68bb8d6e49735
check_file_exist $pkg_filepath $pkg_sha256 "https://github.com/sipeed/MaixCDK/releases/download/v0.0.0/$pkg_filename"
tar -Jxvf $pkg_filepath -C $WORKING_DIR
cp -rf $WORKING_DIR/$pkg_dirname/* $TARGET_ROOTFS_DIR/usr/local
rm -rf $WORKING_DIR/$pkg_dirname

# download pyaxengine
pkg_filename=axengine-0.1.3-py3-none-any.whl
pkg_sha256=9b402f8e52fa940c47256c3efa564a66314badfe0048ed80db2c1054da53ff8a
pkg_filepath=${WORKING_DIR}/pkgs/${pkg_filename}
check_file_exist $pkg_filepath $pkg_sha256 "https://github.com/AXERA-TECH/pyaxengine/releases/download/0.1.3.rc2/$pkg_filename"
cp $pkg_filepath $TARGET_ROOTFS_DIR/usr/local

#cp qemu-aarch64-static and resolv.conf to use qemu and host DNS server
cp /usr/bin/qemu-aarch64-static  $TARGET_ROOTFS_DIR/usr/bin
cp /etc/resolv.conf $TARGET_ROOTFS_DIR/etc/
cp requirements.txt $TARGET_ROOTFS_DIR/requirements.txt
# tar -zxf $WORKING_DIR/Python-${PYTHON_VERSION}.tgz -C $TARGET_ROOTFS_DIR

finish() {
	bash $WORKING_DIR/ch-mount.sh -u $TARGET_ROOTFS_DIR/
	echo -e "error exit"
	# sudo rm -rf $TARGET_ROOTFS_DIR
	sudo rm -f $TARGET_ROOTFS_TAR
	# rm -f $WORKING_DIR/ubuntu-base-24.04.2-base-arm64.tar.gz
	exit 1
}
trap finish ERR

bash $WORKING_DIR/ch-mount.sh -m $TARGET_ROOTFS_DIR/

cat <<EOF | sudo chroot $TARGET_ROOTFS_DIR/
/bin/bash
set -e
export DEBIAN_FRONTEND=noninteractive
chmod 1777 /tmp
apt update --allow-insecure-repositories
locale-gen zh_CN.UTF-8
# install basic packages
# optee and ifplugd
apt install -y --allow-unauthenticated vim bash-completion systemd sudo kmod net-tools ethtool resolvconf ifupdown isc-dhcp-server \
        language-pack-en-base htop bc udev ssh rsyslog lrzsz inetutils-ping \
        tee-supplicant ifplugd \
        python3 libopus0 portaudio19-dev iperf3 \
        iptables wget curl \
        alsa-utils udhcpd wpasupplicant avahi-daemon systemd-timesyncd \
        i2c-tools spi-tools portaudio19-dev udhcpc \
        hostapd rsync neovim arp-scan ripgrep picocom etherwake netcat-traditional\
        bluez evtest usbutils fdisk

if [ -L "/etc/systemd/system/multi-user.target.wants/hostapd.service" ]; then
    unlink "/etc/systemd/system/multi-user.target.wants/hostapd.service"
fi

#install docker first install iptables and change to iptables-legacy
update-alternatives --set iptables /usr/sbin/iptables-legacy
# #Install docker :https://docs.docker.com/engine/install/ubuntu/#install-from-a-package
# apt update
# apt install -y ca-certificates curl
# install -m 0755 -d /etc/apt/keyrings
# curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
# chmod a+r /etc/apt/keyrings/docker.asc
# echo \
#   "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
#   jammy stable" | \
#   tee /etc/apt/sources.list.d/docker.list > /dev/null
# apt update
# apt install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

#install rtc ntp
ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
echo "Asia/Shanghai" > /etc/timezone

#install tailscale
curl -fsSL https://tailscale.com/install.sh | sh

#add user
# useradd -m -s /bin/bash sipeed
# echo "sipeed:sipeed" | chpasswd
# usermod -aG sudo sipeed
# usermod -aG dialout sipeed
# usermod -aG kmem sipeed
# passwd -l root
# grep -q "^PermitRootLogin" /etc/ssh/sshd_config && \
#   sed -i 's/^PermitRootLogin.*/PermitRootLogin no/' /etc/ssh/sshd_config || \
#   echo "PermitRootLogin no" >> /etc/ssh/sshd_config

echo "PermitRootLogin yes" >> /etc/ssh/sshd_config
echo "root:sipeed" | chpasswd

# install python packages
apt install -y \
    build-essential \
    libffi-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev \
    libssl-dev \
    libgdbm-dev \
    libncurses5-dev \
    libncursesw5-dev \
    tk-dev \
    uuid-dev \
    zlib1g-dev \
    libffi-dev \
    libgdbm-compat-dev \
    libxml2-dev \
    libxmlsec1-dev \
    liblzma-dev \
    lzma \
    xz-utils \
    pkg-config \
    libgl1

apt install -y \
    libevent-dev libjpeg-dev libbsd-dev libsystemd-dev \
    libmd-dev libdrm-dev libspeexdsp-dev libopus-dev libcjson-dev \
    libcurl4-openssl-dev liblua5.3-0 libmicrohttpd12 libnanomsg5 \
    libogg0 libpaho-mqtt1.3 librabbitmq4 libsofia-sip-ua0 libusrsctp2 \
    lua-ansicolors lua-json lua-lpeg ssl-cert libconfig-dev libnice-dev \
    libjansson-dev libsrtp2-dev libwebsockets-dev libdbus-1-dev \
    libxkbcommon-dev tesseract-ocr tesseract-ocr-eng nginx ttyd libusb-1.0-0-dev

# cd /Python-*
# ./configure --enable-optimizations --prefix=/usr/local --enable-shared
# make -j$(nproc)
# make install
ln -sf /usr/local/bin/python3.13 /usr/bin/python3
ln -sf /usr/local/bin/pip3.13 /usr/bin/pip3
ln -sf /usr/local/bin/pip3.13 /usr/bin/pip
update-alternatives --install /usr/bin/python3 python3 /usr/local/bin/python3.12 100
update-alternatives --install /usr/bin/python3 python3 /usr/local/bin/python3.13 200
ln -sf /usr/local/bin/python3 /usr/bin/python
pip install -i https://pypi.mirrors.ustc.edu.cn/simple -U pip
pip install -i https://pypi.mirrors.ustc.edu.cn/simple -r /requirements.txt

pip install /usr/local/axengine-0.1.3-py3-none-any.whl

cd /
rm -rf /root/.cache
rm -rf /requirements.txt
rm -rf /Python-*

#exit and clean
apt autoremove -yq --purge
apt clean -yq
rm -rf /var/lib/apt/lists/*
rm -rf /tmp/*
sync
history -c
EOF

# umount
bash $WORKING_DIR/ch-mount.sh -u $TARGET_ROOTFS_DIR/

# clear temp tiles
rm $TARGET_ROOTFS_DIR/usr/bin/qemu-aarch64-static
sudo tar -zcpf $TARGET_ROOTFS_TAR -C $TARGET_ROOTFS_DIR .
sudo chown $(stat -c '%U:%U' $WORKING_DIR) $TARGET_ROOTFS_TAR
sudo rm -rf $TARGET_ROOTFS_DIR
# rm $WORKING_DIR/ubuntu-base-24.04.2-base-arm64.tar.gz
