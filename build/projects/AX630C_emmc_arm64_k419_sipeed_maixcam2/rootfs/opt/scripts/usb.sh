#!/bin/bash

set -e

# load all maix_xxx_xxx configs
if [ -f /boot/configs ]; then
    source /boot/configs
else
    echo "/boot/configs not exists!!!"
fi


case "$1" in
    start)
        if [[ "${maix_usb_mode}x" == "hostx" ]]; then
            /opt/scripts/usb-host.sh start
        else
            systemctl start usb-gadget.service
        fi
        ;;
    stop)
        /opt/scripts/usb-host.sh stop
        systemctl stop usb-gadget.service
        ;;
    *)
        echo "usage:"
        echo "$0 start"
        echo "$0 stop"
        exit 1
        ;;
esac
