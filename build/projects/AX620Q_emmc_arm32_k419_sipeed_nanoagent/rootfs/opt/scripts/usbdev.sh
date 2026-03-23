#!/bin/bash

### !!! 不要使用 SDK 的 /bin/sh, 请使用 /bin/bash !!!

gen_dhcpd_conf() {
    interface="${1}"
    ipv4_prefix="${2}"              # 示例: 10.173.106 (自动补全为 10.173.106.0/24)
    router="${3:-${ipv4_prefix}.1}" # 可选参数: 默认网关（默认 ${ipv4_prefix}.1）
    dns="${4:-8.8.8.8 8.8.4.4}"     # 可选参数: DNS 服务器（默认 8.8.8.8 8.8.4.4）
    lease_time="${5:-864000}"       # 可选参数: 租期时间（默认 864000 秒）

    # 生成 dhcpd.conf 配置
    cat <<EOF
# DHCP Server Configuration for interface ${interface}
authoritative;
default-lease-time ${lease_time};
max-lease-time ${lease_time};

subnet ${ipv4_prefix}.0 netmask 255.255.255.0 {
    range ${ipv4_prefix}.100 ${ipv4_prefix}.200;
    option subnet-mask 255.255.255.0;
    option routers ${router};
    option domain-name-servers ${dns};
    option broadcast-address ${ipv4_prefix}.255;
}
EOF
}

gen_mac_addr() {
    local mode=$1
    local side=$2
    local mac_file="/boot/${mode}.${side}_mac"

    if [ -f "$mac_file" ]; then
        cat "$mac_file"
        return 0
    fi

    local hash
    hash=$( (cat /proc/ax_proc/uid; echo "$mode") | sha512sum | awk '{print $1}')

    local b1 b2 b3 b4 b5 b6
    b1=${hash:0:2}
    b2=${hash:2:2}
    b3=${hash:4:2}
    b4=${hash:6:2}
    b5=${hash:8:2}

    b1=$(printf "%02x" $(( (0x$b1 & 0xFE) | 0x02 )))

    case "$side" in
        host)
            b6="01"
            ;;
        dev)
            b6="02"
            ;;
        *)
            b6="03"
            ;;
    esac

    echo "${b1}:${b2}:${b3}:${b4}:${b5}:${b6}"
}

gen_ipv4_prefix() {
    local mode=$1

    id2=$(printf "%d" 0x$(sha512sum /proc/ax_proc/uid | head -c 2))
    id3=$(printf "%d" 0x$(sha512sum /proc/ax_proc/uid | head -c 4 | tail -c 2))

    if [ "$mode" = "ncm" ]; then
        id3=$((id3 + 1))
    fi

    if [ "$id2" = "$id3" ]; then
        if [ "$mode" = "rndis" ]; then
            id2=$((id2 + 2))
        else
            id2=$((id2 + 1))
        fi
    fi

    if [ "$id2" -ge 255 ]; then
        if [ "$mode" = "rndis" ]; then
            id2=252
        else
            id2=253
        fi
    fi

    if [ "$id3" -ge 255 ]; then
        if [ "$mode" = "rndis" ]; then
            id3=251
        else
            id3=254
        fi
    fi

    echo 10.$id2.$id3
}

gen_udhcpd_conf() {
    interface=${1}
    ipv4_prefix=${2}
    gateway=${3:-${ipv4_prefix}.1}
    dns=${4:-8.8.8.8 8.8.4.4}

    echo "start ${ipv4_prefix}.100"
    echo "end ${ipv4_prefix}.200"
    echo "max_leases 101"
    echo "interface ${interface}"
    echo "pidfile /var/run/udhcpd.${interface}.pid"
    echo "lease_file /var/lib/misc/udhcpd.${interface}.leases"
    echo "option subnet 255.255.255.0"
    echo "option lease 864000"
    echo "option router ${gateway}"
    echo "option dns ${dns}"
}

start_dhcp_client() {
    local mode=$1
    local GADGET_PATH="/sys/kernel/config/usb_gadget/g0"

    local ifname
    if [ "$mode" = "ncm" ]; then
        ifname=$(cat ${GADGET_PATH}/functions/ncm.usb0/ifname 2>/dev/null || echo "usb0")
    else
        ifname=$(cat ${GADGET_PATH}/functions/rndis.usb0/ifname 2>/dev/null || echo "usb0")
    fi

    ip link set $ifname up

    if [ ! -f /var/run/udhcpc.${ifname}.pid ]; then
        udhcpc -i $ifname -p /var/run/udhcpc.${ifname}.pid -b >/dev/null 2>&1 &
    else
        echo "udhcpc already running for ${ifname}"
    fi

    echo "${mode^^} DHCP Client Started"
}

start_dhcp_service() {
    local mode=$1 # rndis 或 ncm
    local GADGET_PATH="/sys/kernel/config/usb_gadget/g0"

    local ifname
    if [ "$mode" = "ncm" ]; then
        ifname=$(cat ${GADGET_PATH}/functions/ncm.usb0/ifname 2>/dev/null || echo "usb0")
    else
        ifname=$(cat ${GADGET_PATH}/functions/rndis.usb0/ifname 2>/dev/null || echo "usb0")
    fi

    # 自定义设备IP地址（支持完整IP+子网掩码，如 10.10.10.1/16）
    if [ -e /boot/${mode}.ipv4 ]; then
        device_ip=$(cat /boot/${mode}.ipv4)
        ipv4_prefix=$(echo $device_ip | cut -d'/' -f1 | cut -d'.' -f1-3)
    else
        ipv4_prefix=$(gen_ipv4_prefix "$mode")
        device_ip="$ipv4_prefix.1/24"
    fi

    gateway=$(echo $device_ip | cut -d'/' -f1)
    if [ -e /boot/${mode}.dns ]; then
        dns=$(cat /boot/${mode}.dns | tr '\n' ' ' | sed 's/  */ /g' | sed 's/^ //;s/ $//')
    else
        dns="8.8.8.8 8.8.4.4"
    fi

    if [ -e /boot/${mode}.forward ]; then
        forward_iface=$(cat /boot/${mode}.forward)

        echo 1 > /proc/sys/net/ipv4/ip_forward

        iptables -t nat -C POSTROUTING -o ${forward_iface} -j MASQUERADE 2>/dev/null || \
            iptables -t nat -A POSTROUTING -o ${forward_iface} -j MASQUERADE

        iptables -C FORWARD -i ${forward_iface} -o ${ifname} -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || \
            iptables -A FORWARD -i ${forward_iface} -o ${ifname} -m state --state RELATED,ESTABLISHED -j ACCEPT

        iptables -C FORWARD -i ${ifname} -o ${forward_iface} -j ACCEPT 2>/dev/null || \
            iptables -A FORWARD -i ${ifname} -o ${forward_iface} -j ACCEPT

        echo "NAT forwarding enabled: ${ifname} -> ${forward_iface}"
    fi

    # 生成配置并启动服务
    gen_udhcpd_conf $ifname "${ipv4_prefix}" "${gateway}" "${dns}" > /etc/udhcpd.${ifname}.conf
    ip link set $ifname up
    ip addr add ${device_ip} dev $ifname
    udhcpd -S /etc/udhcpd.${ifname}.conf
    echo "${mode^^} DHCP Server OK"
}

stop_dhcp_service() {
    local mode=$1
    local GADGET_PATH="/sys/kernel/config/usb_gadget/g0"

    local ifname
    if [ "$mode" = "ncm" ]; then
        ifname=$(cat ${GADGET_PATH}/functions/ncm.usb0/ifname 2>/dev/null || echo "usb0")
    else
        ifname=$(cat ${GADGET_PATH}/functions/rndis.usb0/ifname 2>/dev/null || echo "usb0")
    fi

    for forward_iface in $(ip link show | awk -F': ' -v ifname="$ifname" '/^[0-9]+: / { iface=$2; sub(/@.*/, "", iface); if (iface != "lo" && iface != ifname) print iface }'); do
        if iptables -t nat -C POSTROUTING -o ${forward_iface} -j MASQUERADE 2>/dev/null; then
            if iptables -C FORWARD -i ${ifname} -o ${forward_iface} -j ACCEPT 2>/dev/null; then
                iptables -t nat -D POSTROUTING -o ${forward_iface} -j MASQUERADE 2>/dev/null || true
                iptables -D FORWARD -i ${forward_iface} -o ${ifname} -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || true
                iptables -D FORWARD -i ${ifname} -o ${forward_iface} -j ACCEPT 2>/dev/null || true
                echo "NAT forwarding rules removed: ${ifname} -> ${forward_iface}"
                break
            fi
        fi
    done

    if [ -e /boot/${mode}.ipv4 ]; then
        device_ip=$(cat /boot/${mode}.ipv4)
    else
        ipv4_prefix=$(gen_ipv4_prefix "$mode")
        device_ip="$ipv4_prefix.1/24"
    fi

    ip addr del ${device_ip} dev $ifname 2>/dev/null || true

    ip addr flush dev $ifname 2>/dev/null || true

    ip link set $ifname down

    if [ -f /var/run/udhcpd.${ifname}.pid ]; then
        kill -15 $(cat /var/run/udhcpd.${ifname}.pid) 2>/dev/null
        rm /var/run/udhcpd.${ifname}.pid 2>/dev/null
    fi

    if [ -f /var/run/udhcpc.${ifname}.pid ]; then
        kill -15 $(cat /var/run/udhcpc.${ifname}.pid) 2>/dev/null
        rm /var/run/udhcpc.${ifname}.pid 2>/dev/null
    fi
}

set_usb_role() {
    local role="${1:-device}"
    local role_path="/sys/class/usb_role/8000000.dwc3-role-switch/role"

    if [ ! -e "$role_path" ]; then
        echo "usb_role switch not found, skipping"
        return 0
    fi

    echo "$role" > "$role_path"
    echo "USB role set to: $role"
}

enable_hid_wakeup_on_write() {
    local hid_func="$1"
    local wakeup_path="functions/${hid_func}/wakeup_on_write"

    if [ -e "$wakeup_path" ]; then
        echo 1 > "$wakeup_path"
    fi
}

hid_start() {
    echo "usb mode: device"
    set_usb_role "device"

    cd /sys/kernel/config/usb_gadget
    mkdir g0
    cd g0

    echo 0x0200 > bcdUSB

    if [ -e /boot/usb.bcddevice ]; then
        cat /boot/usb.bcddevice > bcdDevice
    else
        echo 0x0623 > bcdDevice
    fi

    if [ -e /boot/usb.vid ]; then
        cat /boot/usb.vid > idVendor
    else
        echo 0x3346 > idVendor
    fi

    if [ -e /boot/usb.pid ]; then
        cat /boot/usb.pid > idProduct
    else
        echo 0x1009 > idProduct
    fi

    mkdir strings/0x409
    if [ -e /boot/usb.serialnumber ]; then
        cat /boot/usb.serialnumber > strings/0x409/serialnumber
    else
        echo '0123456789ABCDEF' > strings/0x409/serialnumber
    fi
    if [ -e /boot/usb.manufacturer ]; then
        cat /boot/usb.manufacturer > strings/0x409/manufacturer
    else
        echo 'sipeed' > strings/0x409/manufacturer
    fi
    if [ -e /boot/usb.product ]; then
        cat /boot/usb.product > strings/0x409/product
    else
        echo 'NanoKVMPro' > strings/0x409/product
    fi

    mkdir configs/c.1
    echo 0xA0 > configs/c.1/bmAttributes
    echo 200 > configs/c.1/MaxPower
    mkdir configs/c.1/strings/0x409
    echo "NanoKVMPro" > configs/c.1/strings/0x409/configuration

    if [ -e /boot/usb.udisp ] && [ ! -e /boot/usb.uac2 ]; then
        mkdir functions/Loopback.0
        mkdir functions/SourceSink.0

        mkdir configs/c.2
        mkdir configs/c.2/strings/0x409
        echo "NanoKVMPro" > configs/c.2/strings/0x409/configuration

        ln -s functions/Loopback.0 configs/c.1
        ln -s functions/SourceSink.0 configs/c.2
    fi

    if [ -e /boot/usb.rndis ]; then
        mkdir -p functions/rndis.usb0
        echo $(gen_mac_addr rndis dev) > functions/rndis.usb0/dev_addr
        echo $(gen_mac_addr rndis host) > functions/rndis.usb0/host_addr
        echo RNDIS > functions/rndis.usb0/os_desc/interface.rndis/compatible_id
        echo 5162001 > functions/rndis.usb0/os_desc/interface.rndis/sub_compatible_id
        ln -s functions/rndis.usb0 configs/c.1/
    fi

    if [ -e /boot/usb.ncm ]; then
        mkdir -p functions/ncm.usb0
        echo $(gen_mac_addr ncm dev) > functions/ncm.usb0/dev_addr
        echo $(gen_mac_addr ncm host) > functions/ncm.usb0/host_addr
        echo WINNCM > functions/ncm.usb0/os_desc/interface.ncm/compatible_id
        ln -s functions/ncm.usb0 configs/c.1/
    fi

    if ( [ -e /boot/usb.ncm ] || [ -e /boot/usb.rndis ] ) && [ ! -e /boot/usb.udisp ]; then
        # Microsoft OS 2.0 Descriptor
        echo 1 > os_desc/use
        echo 0xCD > os_desc/b_vendor_code
        echo MSFT100 > os_desc/qw_sign
        ln -s configs/c.1 os_desc
    fi

    # keyboard
    mkdir functions/hid.GS0
    echo 1 > functions/hid.GS0/no_out_endpoint
    enable_hid_wakeup_on_write "hid.GS0"
    echo 1 > functions/hid.GS0/protocol
    echo 1 > functions/hid.GS0/subclass
    echo 8 > functions/hid.GS0/report_length
    echo -ne \\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\xE7\\x05\\x07\\x19\\x00\\x29\\xE7\\x81\\x00\\xc0 > functions/hid.GS0/report_desc
    ln -s functions/hid.GS0 configs/c.1

    # mouse
    mkdir functions/hid.GS1
    echo 1 > functions/hid.GS1/no_out_endpoint
    enable_hid_wakeup_on_write "hid.GS1"
    echo 2 > functions/hid.GS1/protocol
    echo 1 > functions/hid.GS1/subclass
    echo 4 > functions/hid.GS1/report_length
    echo -ne \\x5\\x1\\x9\\x2\\xa1\\x1\\x9\\x1\\xa1\\x0\\x5\\x9\\x19\\x1\\x29\\x3\\x15\\x0\\x25\\x1\\x95\\x3\\x75\\x1\\x81\\x2\\x95\\x1\\x75\\x5\\x81\\x3\\x5\\x1\\x9\\x30\\x9\\x31\\x9\\x38\\x15\\x81\\x25\\x7f\\x75\\x8\\x95\\x3\\x81\\x6\\xc0\\xc0 > functions/hid.GS1/report_desc
    ln -s functions/hid.GS1 configs/c.1

    # touchpad
    mkdir functions/hid.GS2
    echo 1 > functions/hid.GS2/no_out_endpoint
    enable_hid_wakeup_on_write "hid.GS2"
    echo 2 > functions/hid.GS2/protocol
    echo 1 > functions/hid.GS2/subclass
    echo 6 > functions/hid.GS2/report_length
    echo -ne \\x05\\x01\\x09\\x02\\xa1\\x01\\x09\\x01\\xa1\\x00\\x05\\x09\\x19\\x01\\x29\\x05\\x15\\x00\\x25\\x01\\x95\\x05\\x75\\x01\\x81\\x02\\x95\\x01\\x75\\x03\\x81\\x01\\x05\\x01\\x09\\x30\\x09\\x31\\x15\\x00\\x26\\xff\\x7f\\x35\\x00\\x46\\xff\\x7f\\x75\\x10\\x95\\x02\\x81\\x02\\x05\\x01\\x09\\x38\\x15\\x81\\x25\\x7f\\x35\\x00\\x45\\x00\\x75\\x08\\x95\\x01\\x81\\x06\\xc0\\xc0 > functions/hid.GS2/report_desc
    ln -s functions/hid.GS2 configs/c.1

    if [ -e /boot/usb.acm ]; then
        mkdir functions/acm.usb0
        ln -s functions/acm.usb0 configs/c.1/
    fi

    if [ -e /boot/usb.disk0 ]; then
        mkdir functions/mass_storage.disk0
        ln -s functions/mass_storage.disk0 configs/c.1/
        echo 1 > functions/mass_storage.disk0/lun.0/removable

        if [ -e /boot/disk.inquiry_string ]; then
            cat /boot/disk.inquiry_string > functions/mass_storage.disk0/lun.0/inquiry_string
        else
            echo -n "sipeed  Flash Drive     1.00" > functions/mass_storage.disk0/lun.0/inquiry_string
        fi

        if [ -e /boot/disk.inquiry_string_cdrom ]; then
            cat /boot/disk.inquiry_string_cdrom > functions/mass_storage.disk0/lun.0/inquiry_string_cdrom
        else
            echo -n "sipeed  Optical Drive   1.00" > functions/mass_storage.disk0/lun.0/inquiry_string_cdrom
        fi

        if [ -e /boot/usb.disk0.ro ]; then
            echo 1 > functions/mass_storage.disk0/lun.0/ro
            echo 0 > functions/mass_storage.disk0/lun.0/cdrom
        fi
    fi

    if ( [ -e /boot/usb.disk1.sd ] || [ -e /boot/usb.disk1 ] || [ -e /boot/usb.disk1.emmc ] ) && \
       [ ! -e /boot/usb.disk0 ] && \
       ( [ -e /dev/mmcblk1 ] || [ -e /exfat.img ] ); then

        mkdir functions/mass_storage.disk1
        ln -s functions/mass_storage.disk1 configs/c.1/
        echo 1 > functions/mass_storage.disk1/lun.0/removable

        if [ -e /boot/disk.inquiry_string ]; then
            cat /boot/disk.inquiry_string > functions/mass_storage.disk1/lun.0/inquiry_string
        else
            echo -n "sipeed  Flash Drive     1.00" > functions/mass_storage.disk1/lun.0/inquiry_string
        fi

        if [ -e /boot/disk.inquiry_string_cdrom ]; then
            cat /boot/disk.inquiry_string_cdrom > functions/mass_storage.disk1/lun.0/inquiry_string_cdrom
        else
            echo -n "sipeed  Optical Drive   1.00" > functions/mass_storage.disk1/lun.0/inquiry_string_cdrom
        fi

        if ( [ -e /boot/usb.disk1.sd ] || [ -e /boot/usb.disk1 ] ) && \
           [ -e /dev/mmcblk1p1 ]; then
            echo /dev/mmcblk1p1 > functions/mass_storage.disk1/lun.0/file
        elif [ -e /boot/usb.disk1.emmc ] && [ -e /exfat.img ]; then
            echo /exfat.img > functions/mass_storage.disk1/lun.0/file
        fi
    fi

    if [ -e /boot/usb.uac2 ]; then
        mkdir -p functions/uac2.usb0
        echo 0 > functions/uac2.usb0/c_chmask
        echo 513 > functions/uac2.usb0/c_terminal_type
        echo 3 > functions/uac2.usb0/p_chmask
        echo 48000 > functions/uac2.usb0/p_srate
        echo 2 > functions/uac2.usb0/p_ssize
        echo 769 > functions/uac2.usb0/p_terminal_type
        if [ -e /boot/uac2.function_name ]; then
            cat /boot/uac2.function_name > functions/uac2.usb0/function_name
        else
            echo "NanoKVMPro Audio" > functions/uac2.usb0/function_name
        fi
        ln -s functions/uac2.usb0 configs/c.1/
    fi

    ls /sys/class/udc/ | cat > UDC

    if [ -e /boot/usb.rndis ]; then
        if [ -e /boot/rndis.dhcp ]; then
            start_dhcp_client rndis
        else
            start_dhcp_service rndis
        fi
    fi

    if [ -e /boot/usb.ncm ]; then
        if [ -e /boot/ncm.dhcp ]; then
            start_dhcp_client ncm
        else
            start_dhcp_service ncm
        fi
    fi
}

hid_only_start() {
    echo "usb mode: device (HID-only)"
    set_usb_role "device"

    cd /sys/kernel/config/usb_gadget
    mkdir g0
    cd g0

    echo 0x0200 > bcdUSB

    if [ -e /boot/usb.bcddevice ]; then
        cat /boot/usb.bcddevice > bcdDevice
    else
        echo 0x0623 > bcdDevice
    fi

    if [ -e /boot/usb.vid ]; then
        cat /boot/usb.vid > idVendor
    else
        echo 0x3346 > idVendor
    fi

    if [ -e /boot/usb.pid ]; then
        cat /boot/usb.pid > idProduct
    else
        echo 0x1009 > idProduct
    fi

    mkdir strings/0x409
    if [ -e /boot/usb.serialnumber ]; then
        cat /boot/usb.serialnumber > strings/0x409/serialnumber
    else
        echo '0123456789ABCDEF' > strings/0x409/serialnumber
    fi
    if [ -e /boot/usb.manufacturer ]; then
        cat /boot/usb.manufacturer > strings/0x409/manufacturer
    else
        echo 'sipeed' > strings/0x409/manufacturer
    fi
    if [ -e /boot/usb.product ]; then
        cat /boot/usb.product > strings/0x409/product
    else
        echo 'NanoKVMPro' > strings/0x409/product
    fi

    mkdir configs/c.1
    echo 0xA0 > configs/c.1/bmAttributes
    echo 200 > configs/c.1/MaxPower
    mkdir configs/c.1/strings/0x409
    echo "NanoKVMPro HID-only" > configs/c.1/strings/0x409/configuration

    mkdir functions/hid.GS0
    echo 1 > functions/hid.GS0/no_out_endpoint
    enable_hid_wakeup_on_write "hid.GS0"
    echo 1 > functions/hid.GS0/protocol
    echo 1 > functions/hid.GS0/subclass
    echo 8 > functions/hid.GS0/report_length
    echo -ne \\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\xE7\\x05\\x07\\x19\\x00\\x29\\xE7\\x81\\x00\\xc0 > functions/hid.GS0/report_desc
    ln -s functions/hid.GS0 configs/c.1

    mkdir functions/hid.GS1
    echo 1 > functions/hid.GS1/no_out_endpoint
    enable_hid_wakeup_on_write "hid.GS1"
    echo 2 > functions/hid.GS1/protocol
    echo 1 > functions/hid.GS1/subclass
    echo 4 > functions/hid.GS1/report_length
    echo -ne \\x5\\x1\\x9\\x2\\xa1\\x1\\x9\\x1\\xa1\\x0\\x5\\x9\\x19\\x1\\x29\\x3\\x15\\x0\\x25\\x1\\x95\\x3\\x75\\x1\\x81\\x2\\x95\\x1\\x75\\x5\\x81\\x3\\x5\\x1\\x9\\x30\\x9\\x31\\x9\\x38\\x15\\x81\\x25\\x7f\\x75\\x8\\x95\\x3\\x81\\x6\\xc0\\xc0 > functions/hid.GS1/report_desc
    ln -s functions/hid.GS1 configs/c.1

    mkdir functions/hid.GS2
    echo 1 > functions/hid.GS2/no_out_endpoint
    enable_hid_wakeup_on_write "hid.GS2"
    echo 2 > functions/hid.GS2/protocol
    echo 1 > functions/hid.GS2/subclass
    echo 6 > functions/hid.GS2/report_length
    echo -ne \\x05\\x01\\x09\\x02\\xa1\\x01\\x09\\x01\\xa1\\x00\\x05\\x09\\x19\\x01\\x29\\x03\\x15\\x00\\x25\\x01\\x95\\x03\\x75\\x01\\x81\\x02\\x95\\x01\\x75\\x05\\x81\\x01\\x05\\x01\\x09\\x30\\x09\\x31\\x15\\x00\\x26\\xff\\x7f\\x35\\x00\\x46\\xff\\x7f\\x75\\x10\\x95\\x02\\x81\\x02\\x05\\x01\\x09\\x38\\x15\\x81\\x25\\x7f\\x35\\x00\\x45\\x00\\x75\\x08\\x95\\x01\\x81\\x06\\xc0\\xc0 > functions/hid.GS2/report_desc
    ln -s functions/hid.GS2 configs/c.1

    ls /sys/class/udc/ | cat > UDC

    touch /dev/shm/tmp/hid_only
}

hid_stop() {
    echo "Disabling USB gadget"
    GADGET_PATH="/sys/kernel/config/usb_gadget/g0"

    if [ -e /dev/shm/tmp/hid_only ]; then
        rm /dev/shm/tmp/hid_only
    fi

    if [ -e /boot/usb.ncm ]; then
        stop_dhcp_service ncm
    fi

    if [ -e /boot/usb.rndis ]; then
        stop_dhcp_service rndis
    fi

    if [ -e "$GADGET_PATH/UDC" ]; then
        echo '' > "$GADGET_PATH/UDC"
        sleep 0.3
    fi

    if [ -d "$GADGET_PATH/configs/c.1" ]; then
        rm -vf "$GADGET_PATH/configs/c.1/"hid.GS*
        rm -vf "$GADGET_PATH/configs/c.1/"rndis.usb0
        rm -vf "$GADGET_PATH/configs/c.1/"ncm.usb0
        rm -vf "$GADGET_PATH/configs/c.1/"mass_storage.disk0
        rm -vf "$GADGET_PATH/configs/c.1/"mass_storage.disk1
        rm -vf "$GADGET_PATH/configs/c.1/"Loopback.0
        rm -vf "$GADGET_PATH/configs/c.1/acm.usb0"
        rm -vf "$GADGET_PATH/configs/c.1/"uac2.usb0
        rm -vf "$GADGET_PATH/os_desc/c.1"

        rmdir "$GADGET_PATH/configs/c.1/strings/0x409"
        rmdir "$GADGET_PATH/configs/c.1"
    fi

    if [ -d "$GADGET_PATH/configs/c.2" ]; then
        rm -vf "$GADGET_PATH/configs/c.2/"SourceSink.0
        rmdir "$GADGET_PATH/configs/c.2/strings/0x409"
        rmdir "$GADGET_PATH/configs/c.2"
    fi

    for func in hid.GS0 hid.GS1 hid.GS2 acm.usb0 rndis.usb0 ncm.usb0 mass_storage.disk0 mass_storage.disk1 Loopback.0 SourceSink.0 uac2.usb0; do
        if [ -d "$GADGET_PATH/functions/$func" ]; then
            [ "$func" = "mass_storage.disk0" ] || [ "$func" = "mass_storage.disk1" ] &&
                echo "" > "$GADGET_PATH/functions/$func/lun.0/file"
            rmdir "$GADGET_PATH/functions/$func"
        fi
    done

    if [ -d "$GADGET_PATH/strings/0x409" ]; then
        rmdir "$GADGET_PATH/strings/0x409"
    fi

    if [ -d "$GADGET_PATH" ]; then
        rmdir "$GADGET_PATH" || echo "Warning: Failed to remove gadget directory"
    fi

    echo "8000000.dwc3" > /sys/bus/platform/drivers/dwc3/unbind
    echo "8000000.dwc3" > /sys/bus/platform/drivers/dwc3/bind
}

case "$1" in
"start") hid_start ;;
"stop") hid_stop ;;
"restart")
    hid_stop
    hid_start
    ;;
"hid-only")
    hid_stop
    hid_only_start
    ;;
"role")
    set_usb_role "${2:-device}"
    ;;
esac
