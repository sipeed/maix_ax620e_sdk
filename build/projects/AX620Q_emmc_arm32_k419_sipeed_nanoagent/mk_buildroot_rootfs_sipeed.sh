#!/bin/bash -e

# set -x

INPUT_TARGET_DIR=$1
INPUT_BUILDROOT_BASE_DIR=$2
TARGET_ROOTFS_DIR=$INPUT_TARGET_DIR
TARGET_ROOTFS_BASE=$INPUT_BUILDROOT_BASE_DIR/rootfs.tar

if [ $# -ne 2 ]; then
	echo "no input parameter, something wrong"
	exit -2
fi

if [ ! -f $TARGET_ROOTFS_BASE ]; then
	#bash $INPUT_BUILDROOT_BASE_DIR/mk_buildroot_base.sh $INPUT_BUILDROOT_BASE_DIR
	echo "no ${TARGET_ROOTFS_BASE}, you need build the buildroot"
	exit -3
fi

if [ -d $TARGET_ROOTFS_DIR ]; then
	rm -rf $TARGET_ROOTFS_DIR
fi

mkdir -p $TARGET_ROOTFS_DIR
echo "mkdir -p $TARGET_ROOTFS_DIR"
tar -xf $TARGET_ROOTFS_BASE -C $TARGET_ROOTFS_DIR

#modify hostname
echo kvm > $TARGET_ROOTFS_DIR/etc/hostname

#link some bin to busybox
#create init link
ln -sf /lib/systemd/systemd $TARGET_ROOTFS_DIR/sbin/init
#link sh to bash
ln -sf /bin/bash $TARGET_ROOTFS_DIR/bin/sh
ln -sf /usr/bin/busybox $TARGET_ROOTFS_DIR/usr/sbin/hwclock
ln -sf /usr/bin/busybox $TARGET_ROOTFS_DIR/usr/sbin/devmem
ln -sf /usr/bin/busybox $TARGET_ROOTFS_DIR/usr/bin/strings

#create rc.local to instead rcS
touch $TARGET_ROOTFS_DIR/etc/rc.local
chmod +x $TARGET_ROOTFS_DIR/etc/rc.local
echo "#!/bin/bash" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "mkdir -p /opt/data/AXSyslog/kernel" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "chmod 755 /soc/scripts/auto_load_all_drv.sh" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /soc/scripts/auto_load_all_drv.sh" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "chmod 755 /soc/scripts/npu_set_bw_limiter.sh" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /soc/scripts/npu_set_bw_limiter.sh start" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /opt/scripts/axsyslogd start" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /opt/scripts/axklogd start" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "chmod 755 /opt/scripts/usbdev.sh" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /opt/scripts/usbdev.sh start" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "insmod /soc/ko/lt7911_manage.ko" >> $TARGET_ROOTFS_DIR/etc/rc.local

#modify profile
echo >> $TARGET_ROOTFS_DIR/etc/profile
echo "export PATH=\$PATH:\"/bin:/sbin:/usr/bin:/usr/sbin:/opt/bin:/opt/usr/bin:/opt/scripts:/soc/bin:/soc/scripts:/usr/local/bin\"" >> $TARGET_ROOTFS_DIR/etc/profile
echo "export LD_LIBRARY_PATH=\"/usr/local/lib:/usr/lib:/opt/lib:/opt/usr/lib:/soc/lib\"" >> $TARGET_ROOTFS_DIR/etc/profile
echo "ulimit -s 2048" >> $TARGET_ROOTFS_DIR/etc/profile
echo "ulimit -c unlimited" >> $TARGET_ROOTFS_DIR/etc/profile

#modify /etc/environment
echo "PATH=\"/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/opt/bin:/opt/usr/bin\"" > $TARGET_ROOTFS_DIR/etc/environment
echo "LD_LIBRARY_PATH=\"/usr/local/lib:/usr/lib:/opt/lib:/opt/usr/lib:/soc/lib\"" >> $TARGET_ROOTFS_DIR/etc/environment


#modify coredump path
echo "kernel.core_pattern=/opt/data/core-%e-%p-%t" >> $TARGET_ROOTFS_DIR/etc/sysctl.conf

#modify start/stop timeout
sed -i '/DefaultTimeoutStartSec/a DefaultTimeoutStartSec=5s' $TARGET_ROOTFS_DIR/etc/systemd/system.conf
sed -i '/DefaultTimeoutStopSec/a DefaultTimeoutStopSec=5s' $TARGET_ROOTFS_DIR/etc/systemd/system.conf

#modify network
echo  >> $TARGET_ROOTFS_DIR/etc/network/interfaces
echo  >> $TARGET_ROOTFS_DIR/etc/network/interfaces
echo "allow-hotplug eth0" >> $TARGET_ROOTFS_DIR/etc/network/interfaces
echo "iface eth0 inet manual" >> $TARGET_ROOTFS_DIR/etc/network/interfaces

#remove this file or mac address will be modified all same
rm $TARGET_ROOTFS_DIR/usr/lib/udev/rules.d/80-net-setup-link.rules
rm $TARGET_ROOTFS_DIR/etc/wpa_supplicant.conf

ln -sf /usr/sbin/etherwake $TARGET_ROOTFS_DIR/usr/sbin/ether-wake

sync
