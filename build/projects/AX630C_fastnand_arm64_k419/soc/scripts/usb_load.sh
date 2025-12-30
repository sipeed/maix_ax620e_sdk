#!/bin/sh

if [ $# -eq 0 ]; then
    mode="-i"
else
    mode=$1
fi

function load_usb()
{
    echo "loading usb ko ..."
    insmod /soc/ko/dwc3-axera.ko
    insmod /soc/ko/dwc3.ko
    echo "load usb ko Done!!!"
}

function remove_usb()
{
    echo "removing usb ko ..."
    rmmod dwc3
    rmmod dwc3-axera
    echo "remove usb Done!!!"
}

function usb()
{
    if [ "$mode" == "-i" ]; then
        load_usb
    elif [ "$mode" == "-r" ]; then
        remove_usb
    else
        echo "[error] Invalid param, please use the following parameters"
        echo "-i:  insmod"
        echo "-r:  rmmod"
    fi
}

usb
