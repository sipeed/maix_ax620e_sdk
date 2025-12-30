#!/bin/bash -e

INPUT_TARGET_DIR=$1
INPUT_UBUNTU_BASE_DIR=$2
TARGET_ROOTFS_DIR=$INPUT_TARGET_DIR
TARGET_ROOTFS_BASE_TAR=$INPUT_UBUNTU_BASE_DIR/ubuntu_rootfs_base.tar.gz
TARGET_ROOTFS_TAR="ubuntu_rootfs.tar.gz"

if [ $# -ne 2 ]; then
	echo "no input parameter, something wrong"
	exit -2
fi

if [ ! -f $TARGET_ROOTFS_BASE_TAR ]; then
	#bash $INPUT_UBUNTU_BASE_DIR/mk_ubuntu_base.sh $INPUT_UBUNTU_BASE_DIR
	echo "no ubuntu_rootfs_base.tar.gz, use mk_ubuntun_base.sh to generate it"
	exit -3
fi

if [ -d $TARGET_ROOTFS_DIR ]; then
	rm -rf $TARGET_ROOTFS_DIR
fi

mkdir -p $TARGET_ROOTFS_DIR
tar -zxpf $TARGET_ROOTFS_BASE_TAR -C $TARGET_ROOTFS_DIR

#modify hostname
echo kvm > $TARGET_ROOTFS_DIR/etc/hostname

#cp ubuntu_rootfs specific files
cp -Rf $2/rootfs_overlay/* $TARGET_ROOTFS_DIR

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
#echo "mount -t pstore pstore /sys/fs/pstore" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /etc/init.d/axemac.sh" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
#echo "hwclock -s" >> $TARGET_ROOTFS_DIR/etc/rc.local
#echo  >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "chmod 755 /soc/scripts/auto_load_all_drv.sh" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /soc/scripts/auto_load_all_drv.sh" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "chmod 755 /soc/scripts/npu_set_bw_limiter.sh" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /soc/scripts/npu_set_bw_limiter.sh start" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "devmem 0x10030028 32 0x000006A0" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /etc/init.d/axsyslogd start" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /etc/init.d/axklogd start" >> $TARGET_ROOTFS_DIR/etc/rc.local

echo "chmod 755 /etc/init.d/S99checkboot" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /etc/init.d/S99checkboot start" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "chmod 755 /etc/init.d/S99checkota" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "bash /etc/init.d/S99checkota start" >> $TARGET_ROOTFS_DIR/etc/rc.local

echo >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "if ! systemctl is-active --quiet sysdev.service; then" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "    systemctl enable --now sysdev.service" >> $TARGET_ROOTFS_DIR/etc/rc.local
echo "fi" >> $TARGET_ROOTFS_DIR/etc/rc.local

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

#serial not login
# sed -i '/ExecStart/s/^/#/' $TARGET_ROOTFS_DIR/lib/systemd/system/serial-getty@.service
# sed -i '/ExecStart/a ExecStart=-/sbin/agetty --autologin sipeed --noclear %I $TERM' $TARGET_ROOTFS_DIR/lib/systemd/system/serial-getty@.service

#modify network
echo  >> $TARGET_ROOTFS_DIR/etc/network/interfaces
echo  >> $TARGET_ROOTFS_DIR/etc/network/interfaces
echo "allow-hotplug eth0" >> $TARGET_ROOTFS_DIR/etc/network/interfaces
echo "iface eth0 inet dhcp" >> $TARGET_ROOTFS_DIR/etc/network/interfaces

#remove this file or mac address will be modified all same
rm $TARGET_ROOTFS_DIR/usr/lib/udev/rules.d/80-net-setup-link.rules

#modify dhclient timeout @baolin
sed -i 's/timeout 300/timeout 5/g' $TARGET_ROOTFS_DIR/etc/dhcp/dhclient.conf
sed -i 's/#retry 60/retry 3/g' $TARGET_ROOTFS_DIR/etc/dhcp/dhclient.conf

#modify ifplugd
sed -i '/^INTERFACES=""$/s/.*/INTERFACES="eth0"/'  $TARGET_ROOTFS_DIR/etc/default/ifplugd
sed -i '/ARGS=/s/.*/ARGS="-q -f -u0 -d0 -w -I"/'  $TARGET_ROOTFS_DIR/etc/default/ifplugd

#modify networking service
sed -i '/TimeoutStartSec=/s/.*/TimeoutStartSec=10sec/' $TARGET_ROOTFS_DIR/usr/lib/systemd/system/networking.service

#mv dev_ip_flush to directory
#mv dev_ip_flush $TARGET_ROOTFS_DIR/etc/network/if-post-down.d/

#modify "raise network interface fail"
sed -i '/mystatedir statedir ifindex interface/s/^/#/' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved
sed -i '/mystatedir statedir ifindex interface/s/^/#/' $TARGET_ROOTFS_DIR/etc/network/if-down.d/resolved
sed -i '/return/s/return/exit 0/' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved
sed -i '/return/s/return/exit 0/' $TARGET_ROOTFS_DIR/etc/network/if-down.d/resolved
sed -i 's/DNS=DNS/DNS=\$DNS/g' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved
sed -i 's/DOMAINS=DOMAINS/DOMAINS=\$DOMAINS/g' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved
sed -i 's/DNS=DNS6/DNS=\$DNS6/g' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved
sed -i 's/DOMAINS=DOMAINS6/DOMAINS=\$DOMAINS6/g' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved
sed -i 's/"\$DNS"="\$NEW_DNS"/DNS="\$NEW_DNS"/g' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved
sed -i 's/"\$DOMAINS"="\$NEW_DOMAINS"/DOMAINS="\$NEW_DOMAINS"/g' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved
sed -i '/DNS DNS6 DOMAINS DOMAINS6 DEFAULT_ROUTE/s/^/#/' $TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved

ln -sf /usr/sbin/etherwake $TARGET_ROOTFS_DIR/usr/sbin/ether-wake

cat <<EOF > ${TARGET_ROOTFS_DIR}/etc/avahi/services/ssh.service
<?xml version="1.0" standalone='no'?>
<!DOCTYPE service-group SYSTEM "avahi-service.dtd">
<service-group>
  <name replace-wildcards="yes">%h SSH</name>
  <service>
    <type>_ssh._tcp</type>
    <port>22</port>
  </service>
</service-group>
EOF

sync
