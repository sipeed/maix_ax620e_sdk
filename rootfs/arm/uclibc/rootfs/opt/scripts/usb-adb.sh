#!/bin/sh

usb_dev_controller=usb_adb
export CONFIGFS_HOME=/etc/configfs
UDC=$CONFIGFS_HOME/usb_gadget/${usb_dev_controller}/UDC

function adb_stop()
{
    echo "" > ${UDC}
    find $CONFIGFS_HOME/usb_gadget/${usb_dev_controller} -type l | xargs -I {} rm {}
    killall adbd
    umount /dev/usb-ffs/adb
}

function adb_start()
{
    mkdir -p $CONFIGFS_HOME
    if [ ! -d "${CONFIGFS_HOME}/usb_gadget/" ]; then
            mount none $CONFIGFS_HOME -t configfs
    fi

    mkdir -p $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}
    cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    echo 0x32c9 > idVendor
    echo 0x2003 > idProduct

    mkdir -p strings/0x409 -m 0770
    echo "axera-ax620e" > strings/0x409/serialnumber
    echo "axera" > strings/0x409/manufacturer
    echo "ax620e-adb" > strings/0x409/product

    mkdir -p configs/c.1 -m 0770
    mkdir -p configs/c.1/strings/0x409 -m 0770
    echo "adb" > configs/c.1/strings/0x409/configuration
    echo 120 > configs/c.1/MaxPower

    mkdir -p functions/ffs.adb

    ln -s functions/ffs.adb configs/c.1

    mkdir -p /dev/usb-ffs/adb -m 0770
    mount -t functionfs adb /dev/usb-ffs/adb

    cd /
    adbd  &
    sleep 1

    echo "enable usb gadget adb"
    USB=`ls /sys/class/udc/| awk '{print $1}'`
    echo ${USB} > ${UDC}
}

USB_UDC=`ls /sys/class/udc/| awk '{print $1}'`
if [ -z "$USB_UDC" ]; then
    echo "Now there is no UDC, usb adb cannot be enabled."
    exit 0
fi

case "$1" in
    start)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -n "${line}" ]; then
                    echo "usb adb has enabled"  # so we don't need to enable it again
                    exit 0
            fi
        fi
        echo "usb adb start"
        adb_start
        ;;
    stop)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -z "${line}" ]; then
                    echo "usb adb has stopped"  # so we don't need to stop it again
                    exit 0
            fi
        else
            echo "usb adb didn't start"
            exit 0
        fi

        echo "usb adb stop"
        adb_stop
        ;;
    *)
        echo "usage:"
        echo "usb-adb.sh start"
        echo "usb-adb.sh stop"
        exit 1
        ;;
esac
