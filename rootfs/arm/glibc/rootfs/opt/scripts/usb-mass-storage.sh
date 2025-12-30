#!/bin/sh

usb_dev_controller=usb_mass_storage
export CONFIGFS_HOME=/etc/configfs
UDC=$CONFIGFS_HOME/usb_gadget/${usb_dev_controller}/UDC
SD=/dev/mmcblk1p1

function mass_storage_stop()
{
    echo "" > ${UDC}
    find $CONFIGFS_HOME/usb_gadget/${usb_dev_controller} -type l | xargs -I {} rm {}
}

function mass_storage_start()
{
    mkdir -p $CONFIGFS_HOME
    if [ ! -d "${CONFIGFS_HOME}/usb_gadget/" ]; then
            mount none $CONFIGFS_HOME -t configfs
    fi

    mkdir -p $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}
    cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    echo 0x32c9 > idVendor
    echo 0x2004 > idProduct

    mkdir -p strings/0x409 -m 0770
    echo "axera-ax620e" > strings/0x409/serialnumber
    echo "axera" > strings/0x409/manufacturer
    echo "ax620e-mass-storage" > strings/0x409/product

    mkdir -p configs/c.1 -m 0770
    mkdir -p configs/c.1/strings/0x409 -m 0770
    echo "mass_storage" > configs/c.1/strings/0x409/configuration
    echo 120 > configs/c.1/MaxPower

    mkdir -p functions/mass_storage.usb0
    echo ${SD} >functions/mass_storage.usb0/lun.0/file

    ln -s functions/mass_storage.usb0 configs/c.1

    echo "usb mass storage start"
    USB=`ls /sys/class/udc/| awk '{print $1}'`
    echo ${USB} > ${UDC}
}

USB_UDC=`ls /sys/class/udc/| awk '{print $1}'`
if [ -z "$USB_UDC" ]; then
    echo "Now there is no UDC, usb mass storage cannot be enabled."
    exit 0
fi

case "$1" in
    start)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -n "${line}" ]; then
                    echo "usb mass_storage has enabled"  # so we don't need to enable it again
                    exit 0
            fi
        fi
        echo "usb mass_storage start"
        mass_storage_start
        ;;
    stop)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -z "${line}" ]; then
                    echo "usb mass_storage has stopped"  # so we don't need to stop it again
                    exit 0
            fi
        else
            echo "usb mass_storage didn't start"
            exit 0
        fi

        echo "usb mass_storage stop"
        mass_storage_stop
        ;;
    *)
        echo "usage:"
        echo "usb_mass_storage.sh start"
        echo "usb_mass_storage.sh stop"
        exit 1
        ;;
esac
