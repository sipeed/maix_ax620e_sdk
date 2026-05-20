#!/bin/bash -e

INPUT_TARGET_DIR=$1
INPUT_DEBIAN_BASE_DIR=$2
TARGET_ROOTFS_DIR=$INPUT_TARGET_DIR
TARGET_ROOTFS_BASE_TAR=$INPUT_DEBIAN_BASE_DIR/debian_rootfs_base.tar.gz

if [ $# -ne 2 ]; then
	echo "no input parameter, something wrong"
	exit -2
fi

if [ ! -f "$TARGET_ROOTFS_BASE_TAR" ]; then
	echo "no debian_rootfs_base.tar.gz, use mk_debian_base.sh to generate it"
	exit -3
fi

if [ -d "$TARGET_ROOTFS_DIR" ]; then
	rm -rf "$TARGET_ROOTFS_DIR"
fi

mkdir -p "$TARGET_ROOTFS_DIR"
tar -zxpf "$TARGET_ROOTFS_BASE_TAR" -C "$TARGET_ROOTFS_DIR" --exclude='./dev/*'

# cp debian_rootfs specific files
if [ -d "$INPUT_DEBIAN_BASE_DIR/rootfs_overlay" ]; then
	cp -Rf "$INPUT_DEBIAN_BASE_DIR"/rootfs_overlay/* "$TARGET_ROOTFS_DIR" || true
fi

# modify hostname
echo nanoagent > "$TARGET_ROOTFS_DIR/etc/hostname"

# link some bin to busybox
ln -sf /usr/bin/busybox "$TARGET_ROOTFS_DIR/usr/sbin/hwclock"
ln -sf /usr/bin/busybox "$TARGET_ROOTFS_DIR/usr/sbin/devmem"
ln -sf /usr/bin/busybox "$TARGET_ROOTFS_DIR/usr/bin/strings"

ln -sf /lib/systemd/systemd "$TARGET_ROOTFS_DIR/sbin/init"
ln -sf /bin/bash "$TARGET_ROOTFS_DIR/bin/sh"
ln -sf /usr/sbin/etherwake $TARGET_ROOTFS_DIR/usr/sbin/ether-wake

# create rc.local
touch "$TARGET_ROOTFS_DIR/etc/rc.local"
chmod +x "$TARGET_ROOTFS_DIR/etc/rc.local"
cat > "$TARGET_ROOTFS_DIR/etc/rc.local" <<'EOF'
#!/bin/bash

mkdir -p /opt/data/AXSyslog/kernel

chmod 755 /soc/scripts/auto_load_all_drv.sh
bash /soc/scripts/auto_load_all_drv.sh

chmod 755 /soc/scripts/npu_set_bw_limiter.sh
bash /soc/scripts/npu_set_bw_limiter.sh start

bash /opt/scripts/axsyslogd start
bash /opt/scripts/axklogd start

EOF

# modify profile
echo >> "$TARGET_ROOTFS_DIR/etc/profile"
echo 'export PATH=$PATH:"/bin:/sbin:/usr/bin:/usr/sbin:/opt/bin:/opt/usr/bin:/opt/scripts:/soc/bin:/soc/scripts:/usr/local/bin"' >> "$TARGET_ROOTFS_DIR/etc/profile"
echo 'export LD_LIBRARY_PATH="/usr/local/lib:/usr/lib:/opt/lib:/opt/usr/lib:/soc/lib"' >> "$TARGET_ROOTFS_DIR/etc/profile"
echo 'ulimit -s 2048' >> "$TARGET_ROOTFS_DIR/etc/profile"
echo 'ulimit -c unlimited' >> "$TARGET_ROOTFS_DIR/etc/profile"

# modify /etc/environment
cat > "$TARGET_ROOTFS_DIR/etc/environment" <<'EOF'
PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/opt/bin:/opt/usr/bin"
LD_LIBRARY_PATH="/usr/local/lib:/usr/lib:/opt/lib:/opt/usr/lib:/soc/lib"
EOF

# modify coredump path
echo 'kernel.core_pattern=/opt/data/core-%e-%p-%t' >> "$TARGET_ROOTFS_DIR/etc/sysctl.conf"

# modify start/stop timeout
if [ -f "$TARGET_ROOTFS_DIR/etc/systemd/system.conf" ]; then
	sed -i '/DefaultTimeoutStartSec/a DefaultTimeoutStartSec=5s' "$TARGET_ROOTFS_DIR/etc/systemd/system.conf"
	sed -i '/DefaultTimeoutStopSec/a DefaultTimeoutStopSec=5s' "$TARGET_ROOTFS_DIR/etc/systemd/system.conf"
fi

sed -i 's/return/exit 0/g' "$TARGET_ROOTFS_DIR/etc/network/if-up.d/resolved"

sync
