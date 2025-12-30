#!/bin/bash

set -e

usb_host_start() {
    echo host > /sys/class/usb_role/8000000.dwc3-role-switch/role
}

usb_host_stop() {
    echo ""
}

case "$1" in
    start)
        echo "usb host start"
        usb_host_start
        ;;
    stop)
        echo "usb host stop"
        usb_host_stop
        ;;
    *)
        echo "usage:"
        echo "$0 start"
        echo "$0 stop"
        exit 1
        ;;
esac
