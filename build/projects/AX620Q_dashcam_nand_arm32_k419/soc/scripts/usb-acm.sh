#!/bin/sh

usb_dev_controller=usb_acm
export CONFIGFS_HOME=/tmp/configfs
UDC=$CONFIGFS_HOME/usb_gadget/${usb_dev_controller}/UDC

function acm_stop()
{
    echo "" > ${UDC}
    find $CONFIGFS_HOME/usb_gadget/${usb_dev_controller} -type l | xargs -I {} rm {}
}

function acm_start()
{
    mkdir -p $CONFIGFS_HOME
    if [ ! -d "${CONFIGFS_HOME}/usb_gadget/" ]; then
        mount none $CONFIGFS_HOME -t configfs
    fi

    mkdir -p $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}
    cd $CONFIGFS_HOME/usb_gadget/${usb_dev_controller}

    echo 0x32c9 > idVendor
    echo 0x2006 > idProduct

    mkdir -p strings/0x409
    echo "axera-ax620e" > strings/0x409/serialnumber
    echo "axera" > strings/0x409/manufacturer
    echo "axera-acm" > strings/0x409/product

    mkdir -p configs/c.1
    mkdir -p configs/c.1/strings/0x409
    echo acm > configs/c.1/strings/0x409/configuration
    echo 120 > configs/c.1/MaxPower

    mkdir -p functions/acm.usb0

    ln -s functions/acm.usb0 configs/c.1

    echo "enable usb acm gadget"
    USB=`ls /sys/class/udc/| awk '{print $1}'`
    echo ${USB} > UDC
}

USB_UDC=`ls /sys/class/udc/| awk '{print $1}'`
if [ -z "$USB_UDC" ]; then
    echo "Now there is no UDC, usb acm cannot be enabled."
    exit 0
fi

case "$1" in
    start)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -n "${line}" ]; then
                    echo "usb acm has enabled"  # so we don't need to enable it again
                    exit 0
            fi
        fi
        echo "usb acm start"
        acm_start
        ;;
    stop)
        if [ -e ${UDC} ]; then
            read line < ${UDC}
            if [ -z "${line}" ]; then
                    echo "usb acm has stopped"  # so we don't need to stop it again
                    exit 0
            fi
        else
            echo "usb acm didn't start"
            exit 0
        fi

        echo "usb acm stop"
        acm_stop
        ;;
    *)
        echo "usage:"
        echo "usb-acm.sh start"
        echo "usb-acm.sh stop"
        exit 1
        ;;
esac
