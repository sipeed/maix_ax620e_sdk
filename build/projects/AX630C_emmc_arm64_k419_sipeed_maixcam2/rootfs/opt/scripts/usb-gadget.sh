#!/bin/bash

set -e

# load all maix_xxx_xxx configs
if [ -f /boot/configs ]; then
    source /boot/configs
else
    echo "/boot/configs not exists!!!"
fi

usb_dev_controller=g1
CONFIGFS_HOME=/etc/configfs
UDC=$CONFIGFS_HOME/usb_gadget/${usb_dev_controller}/UDC
rndis_ifname=usb0
ncm_ifname=usb1

gen_udhcpd_conf() {
	interface=${1}
	ipv4_prefix=${2}
	echo "start ${ipv4_prefix}.100"
	echo "end ${ipv4_prefix}.200"
	echo "interface ${interface}"
	echo "pidfile /var/run/udhcpd.${interface}.pid"
	echo "lease_file /var/lib/misc/udhcpd.${interface}.leases"
	echo "option subnet 255.255.255.0"
	echo "option lease 864000"
}

usb_ncm_stop()
{
    # if [ ! -d "${CONFIGFS_HOME}/usb_gadget/${usb_dev_controller}" ]; then
    #     echo "usb gadget is not created"
    #     exit 1
    # fi
    # cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    # if [ ! -e functions/ncm.${ncm_ifname} ]; then
    #     echo "ncm is not created"
    #     exit 1
    # fi

    ipv4_prefix=$(get_ncm_ipv4_prefix)
    ip addr del $ipv4_prefix.1/24 dev $ncm_ifname || true
    ifconfig $ncm_ifname down
    if [ -f /var/run/udhcpd.${ncm_ifname}.pid ]
    then
        kill -15 $(cat /var/run/udhcpd.${ncm_ifname}.pid)
    fi

    # if [ -e configs/c.1/ncm.${ncm_ifname} ]; then
    #     rm configs/c.1/ncm.${ncm_ifname}
    # fi

    # if [ -d functions/ncm.${ncm_ifname} ]; then
    #     rmdir functions/ncm.${ncm_ifname}
    # fi
}

get_ncm_ipv4_prefix()
{
    id2=$(printf "%d" 0x$(sha512sum /proc/ax_proc/uid | head -c 2))
    id3=$(printf "%d" 0x$(sha512sum /proc/ax_proc/uid | head -c 4 | tail -c 2))
    id3=$((id3 + 1))
    if [ "$id2" = "$id3" ]
    then
        id2=$((id2 + 1))
    fi
    if [ "$id2" -ge 255 ]
    then
        id2=253
    fi
    if [ "$id3" -ge 255 ]
    then
        id3=254
    fi

    echo 10.$id2.$id3
}

usb_ncm_start()
{
    if [ ! -d "${CONFIGFS_HOME}/usb_gadget/${usb_dev_controller}" ]; then
        echo "usb gadget is not created"
        exit 1
    fi
    cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    # if [ -e functions/ncm.${ncm_ifname} ]; then
    #     echo "ncm is already created"
    #     exit 1
    # fi

    # mkdir -p functions/ncm.${ncm_ifname}
    # ln -s functions/ncm.${ncm_ifname} configs/c.1

    new_ncmmac=$(echo 48da356d$(sha512sum /proc/ax_proc/uid | head -c 3)e | sed 's/../&:/g; s/:$//')
    ip link set $ncm_ifname down
    ip link set $ncm_ifname address ${new_ncmmac}
    ip link set $ncm_ifname up
    ifconfig $ncm_ifname up
    echo "ncm mac address: ${new_ncmmac}"

    ipv4_prefix=$(get_ncm_ipv4_prefix)
    gen_udhcpd_conf $ncm_ifname "${ipv4_prefix}"  > /etc/udhcpd.${ncm_ifname}.conf
    ip addr add $ipv4_prefix.1/24 dev $ncm_ifname
    udhcpd -S /etc/udhcpd.${ncm_ifname}.conf
}

usb_rndis_stop()
{
    # if [ ! -d "${CONFIGFS_HOME}/usb_gadget/${usb_dev_controller}" ]; then
    #     echo "usb gadget is not created"
    #     exit 1
    # fi
    # cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    # if [ ! -e functions/rndis.${rndis_ifname} ]; then
    #     echo "rndis is not created"
    #     exit 1
    # fi

    ipv4_prefix=$(get_rndis_ipv4_prefix)
    ip addr del $ipv4_prefix.1/24 dev $rndis_ifname || true

    ifconfig $rndis_ifname down
    if [ -f /var/run/udhcpd.${rndis_ifname}.pid ]
    then
        kill -15 $(cat /var/run/udhcpd.${rndis_ifname}.pid)
    fi

    # if [ -e configs/c.1/rndis.${rndis_ifname} ]; then
    #     rm configs/c.1/rndis.${rndis_ifname}
    # fi

    # if [ -d functions/rndis.${rndis_ifname} ]; then
    #     rmdir functions/rndis.${rndis_ifname}
    # fi
}

get_rndis_ipv4_prefix()
{
    id2=$(printf "%d" 0x$(sha512sum /proc/ax_proc/uid | head -c 2))
    id3=$(printf "%d" 0x$(sha512sum /proc/ax_proc/uid | head -c 4 | tail -c 2))
    if [ "$id2" = "$id3" ]
    then
        id2=$((id2 + 2))
    fi
    if [ "$id2" -ge 255 ]
    then
        id2=252
    fi
    if [ "$id3" -ge 255 ]
    then
        id3=251
    fi

    echo 10.$id2.$id3
}

usb_rndis_start()
{
    if [ ! -d "${CONFIGFS_HOME}/usb_gadget/${usb_dev_controller}" ]; then
        echo "usb gadget is not created"
        exit 1
    fi
    cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    new_rndismac=$(echo 48da356d$(sha512sum /proc/ax_proc/uid | head -c 3)f | sed 's/../&:/g; s/:$//')
    ip link set $rndis_ifname down
    ip link set $rndis_ifname address ${new_rndismac}
    ip link set $rndis_ifname up
    ifconfig $rndis_ifname up
    echo "rndis mac address: ${new_rndismac}"

    ipv4_prefix=$(get_rndis_ipv4_prefix)

    gen_udhcpd_conf $rndis_ifname "${ipv4_prefix}"  > /etc/udhcpd.${rndis_ifname}.conf
    ip addr add $ipv4_prefix.1/24 dev $rndis_ifname
    udhcpd -S /etc/udhcpd.${rndis_ifname}.conf
}

usb_gadget_stop()
{
    usb_rndis_stop
    usb_ncm_stop

    echo > ${UDC}
    find $CONFIGFS_HOME/usb_gadget/${usb_dev_controller} -type l | xargs -I {} rm {}
}

usb_gadget_start()
{
    echo device > /sys/class/usb_role/8000000.dwc3-role-switch/role
    mkdir -p $CONFIGFS_HOME
    if [ ! -d "${CONFIGFS_HOME}/usb_gadget/" ]; then
            mount none $CONFIGFS_HOME -t configfs
    fi

    mkdir -p $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}
    cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    echo 0x359f > idVendor
    echo 0x2200 > idProduct

    mkdir -p strings/0x409 -m 0770
    echo "maixcam2" > strings/0x409/serialnumber
    echo "sipeed" > strings/0x409/manufacturer
    echo "maixcam2" > strings/0x409/product

    mkdir -p configs/c.1 -m 0770
    mkdir -p configs/c.1/strings/0x409 -m 0770
    echo "gadget config" > configs/c.1/strings/0x409/configuration
    echo 250 > configs/c.1/MaxPower # max 500mA

    # RNDIS
    if [[ "${maix_usb_rndis}x" == "1x" ]]; then
        mkdir -p functions/rndis.${rndis_ifname}
        rm -f configs/c.1/rndis.${rndis_ifname}
        ln -s functions/rndis.${rndis_ifname} configs/c.1
    fi

    # NCM
    if [[ "${maix_usb_ncm}x" == "1x" ]]; then
        mkdir -p functions/ncm.${ncm_ifname}
        rm -f configs/c.1/ncm.${ncm_ifname}
        ln -s functions/ncm.${ncm_ifname} configs/c.1
    fi

    # mass storage
    if [[ "${maix_usb_mass_storage}x" == "1x" ]]; then
        mkdir -p functions/mass_storage.disk0
        mkdir -p functions/mass_storage.disk0/lun.0
        mkdir -p functions/mass_storage.disk0/lun.1
        rm -f configs/c.1/mass_storage.disk0
        ln -s functions/mass_storage.disk0 configs/c.1/
        echo 1 > functions/mass_storage.disk0/lun.0/removable
        echo 1 > functions/mass_storage.disk0/lun.1/removable
	    echo /dev/mmcblk0p16 > functions/mass_storage.disk0/lun.0/file # boot
        echo /dev/mmcblk0p17 > functions/mass_storage.disk0/lun.1/file # root
    fi

    if [[ "${maix_usb_hid_keyboard}x" == "1x" ]]; then
        mkdir -p functions/hid.GS0
        # echo 1 > functions/hid.GS0/subclass
        # echo 1 > functions/hid.GS0/wakeup_on_write
        echo 1 > functions/hid.GS0/protocol
        echo 6 > functions/hid.GS0/report_length
        echo -ne \\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x00\\xc0 > functions/hid.GS0/report_desc
        rm -f configs/c.1/hid.GS0
        ln -s functions/hid.GS0 configs/c.1
    fi

    if [[ "${maix_usb_hid_mouse}x" == "1x" ]]; then
        mkdir -p functions/hid.GS1
        # echo 1 > functions/hid.GS1/subclass
        # echo 1 > functions/hid.GS1/wakeup_on_write
        echo 2 > functions/hid.GS1/protocol
        echo 4 > functions/hid.GS1/report_length
        echo -ne \\x5\\x1\\x9\\x2\\xa1\\x1\\x9\\x1\\xa1\\x0\\x5\\x9\\x19\\x1\\x29\\x3\\x15\\x0\\x25\\x1\\x95\\x3\\x75\\x1\\x81\\x2\\x95\\x1\\x75\\x5\\x81\\x3\\x5\\x1\\x9\\x30\\x9\\x31\\x9\\x38\\x15\\x81\\x25\\x7f\\x75\\x8\\x95\\x3\\x81\\x6\\xc0\\xc0 > functions/hid.GS1/report_desc
        rm -f configs/c.1/hid.GS1
        ln -s functions/hid.GS1 configs/c.1
    fi

    if [[ "${maix_usb_hid_touchpad}x" == "1x" ]]; then
        mkdir -p functions/hid.GS2
        # echo 1 > functions/hid.GS2/subclass
        # echo 1 > functions/hid.GS2/wakeup_on_write
        echo 2 > functions/hid.GS2/protocol
        echo 6 > functions/hid.GS2/report_length
        echo -ne \\x05\\x01\\x09\\x02\\xa1\\x01\\x09\\x01\\xa1\\x00\\x05\\x09\\x19\\x01\\x29\\x03\\x15\\x00\\x25\\x01\\x95\\x03\\x75\\x01\\x81\\x02\\x95\\x01\\x75\\x05\\x81\\x01\\x05\\x01\\x09\\x30\\x09\\x31\\x15\\x00\\x26\\xff\\x7f\\x35\\x00\\x46\\xff\\x7f\\x75\\x10\\x95\\x02\\x81\\x02\\x05\\x01\\x09\\x38\\x15\\x81\\x25\\x7f\\x35\\x00\\x45\\x00\\x75\\x08\\x95\\x01\\x81\\x06\\xc0\\xc0 > functions/hid.GS2/report_desc
        rm -f configs/c.1/hid.GS2
        ln -s functions/hid.GS2 configs/c.1
    fi

    echo "${maix_usb_ncm} -- ${maix_usb_rndis} -- ${maix_usb_mass_storage}"
    echo "enable usb gadget"
    udc=$(ls /sys/class/udc/ | grep dwc)
    echo "valid udc: --${udc}--"
    func_num=$(ls functions|wc -l)
    if [ "$func_num" -gt 0 ]; then
        echo $udc > ${UDC}
    fi

    # NCM dhcp
    if [[ "${maix_usb_ncm}x" == "1x" ]]; then
        usb_ncm_start
    fi

    # RNDIS dhcp
    if [[ "${maix_usb_rndis}x" == "1x" ]]; then
        usb_rndis_start
    fi
}

case "$1" in
    start)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -n "${line}" ]; then
                    echo "usb gadget has enabled"  # so we don't need to enable it again
                    exit 0
            fi
        fi

        echo "usb gadget start"
        usb_gadget_start
        ;;
    stop)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -z "${line}" ]; then
                    echo "usb gadget has stopped"  # so we don't need to stop it again
                    exit 0
            fi
        else
            echo "usb gadget didn't start"
            exit 0
        fi

        echo "usb gadget stop"
        usb_gadget_stop

        if [ -e /var/run/udhcpd.usb0.pid ]; then
            kill -15 $(cat /var/run/udhcpd.usb0.pid)
        fi
        ;;
    rndis_start)
        usb_rndis_start
        ;;
    rndis_stop)
        usb_rndis_stop
        ;;
    ncm_start)
        usb_ncm_start
        ;;
    ncm_stop)
        usb_ncm_stop
        ;;
    *)
        echo "usage:"
        echo "usb-gadget.sh start"
        echo "usb-gadget.sh stop"
        exit 1
        ;;
esac
