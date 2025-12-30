#!/bin/sh

usb_dev_controller=usb_uac1
export CONFIGFS_HOME=/etc/configfs
UDC=$CONFIGFS_HOME/usb_gadget/${usb_dev_controller}/UDC

function uac1_stop()
{
    echo > ${UDC}
    find $CONFIGFS_HOME/usb_gadget/${usb_dev_controller} -type l | xargs -I {} rm {}
}

function uac1_start()
{
    mkdir -p $CONFIGFS_HOME
    if [ ! -d "${CONFIGFS_HOME}/usb_gadget/" ]; then
            mount none $CONFIGFS_HOME -t configfs
    fi

    mkdir -p $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}
    cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    echo 0x32c9 > idVendor
    echo 0x2002 > idProduct

    mkdir -p strings/0x409 -m 0770
    echo "axera-ax620e" > strings/0x409/serialnumber
    echo "axera" > strings/0x409/manufacturer
    echo "ax620e-uac1" > strings/0x409/product

    mkdir -p configs/c.1 -m 0770
    mkdir -p configs/c.1/strings/0x409 -m 0770
    echo "uac1" > configs/c.1/strings/0x409/configuration
    echo 120 > configs/c.1/MaxPower

    mkdir -p functions/uac1.usb0

    ln -s functions/uac1.usb0 configs/c.1

    echo "enable usb uac1 gadget"
    USB=`ls /sys/class/udc/| awk '{print $1}'`
    echo ${USB} > ${UDC}
}

USB_UDC=`ls /sys/class/udc/| awk '{print $1}'`
if [ -z "$USB_UDC" ]; then
    echo "Now there is no UDC, usb uac1 cannot be enabled."
    exit 0
fi

case "$1" in
    start)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -n "${line}" ]; then
                    echo "usb uac1 has enabled"  # so we don't need to enable it again
                    exit 0
            fi
        fi
        echo "usb uac1 start"
        uac1_start
        ;;
    stop)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -z "${line}" ]; then
                    echo "usb uac1 has stopped"  # so we don't need to stop it again
                    exit 0
            fi
        else
            echo "usb uac1 didn't start"
            exit 0
        fi

        echo "usb uac1 stop"
        uac1_stop
        ;;
    *)
        echo "usage:"
        echo "usb-uac1.sh start"
        echo "usb-uac1.sh stop"
        exit 1
        ;;
esac
