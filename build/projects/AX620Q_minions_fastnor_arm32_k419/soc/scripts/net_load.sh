#!/bin/sh

if [ $# -eq 0 ]; then
    mode="-i"
else
    mode=$1
fi

function load_net()
{
    echo "loading net ko ..."
    insmod /soc/ko/dwmac-axera-plat.ko
    /etc/init.d/network start
    echo "load net ko Done!!!"
}

function remove_net()
{
    echo "removing net ko ..."
    /etc/init.d/network stop
    rmmod dwmac-axera-plat
    echo "remove net Done!!!"
}

function net()
{
    if [ "$mode" == "-i" ]; then
        load_net
    elif [ "$mode" == "-r" ]; then
        remove_net
    else
        echo "[error] Invalid param, please use the following parameters"
        echo "-i:  insmod"
        echo "-r:  rmmod"
    fi
}

net
