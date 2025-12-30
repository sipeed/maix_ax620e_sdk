#!/bin/sh

WIFISSID=$2
WIFIPWD=$3
TIMEOUT=10

if [ ! -d "/etc/wifi" ]; then
    mkdir /etc/wifi
fi

function wifi_start()
{
    insmod /soc/ko/rtl8188fu.ko
    sleep 1

    ifconfig wlan0 up

    echo "ctrl_interface=/var/wpa_supplicant" > /etc/wifi/wpa_supplicant.conf
    echo "ap_scan=1" >> /etc/wifi/wpa_supplicant.conf
    echo "update_config=1" >> /etc/wifi/wpa_supplicant.conf
    wpa_passphrase $WIFISSID $WIFIPWD >> /etc/wifi/wpa_supplicant.conf

    wpa_supplicant -B -i wlan0 -c /etc/wifi/wpa_supplicant.conf

    for i in `seq ${TIMEOUT}`;do
        echo "Wait wifi connect ...`expr ${TIMEOUT} - ${i} + 1`s"
        detect_info=`iw wlan0 link`;
        if [ "$detect_info" != "Not connected." ]; then
            iw wlan0 link
            sleep 1
            if type udhcpc >/dev/null 2>&1; then
                udhcpc -b -i wlan0 -q
            else
               dhclient -i wlan0
            fi
            echo "start ok!"
            exit 0
        fi
        sleep 1
    done
    wifi_stop
    echo "usb wifi fail!"
    exit 1
}

function wifi_stop()
{
    killall wpa_supplicant
    rm -rf /etc/wifi/wpa_supplicant.conf
    ifconfig wlan0 down
    rmmod rtl8188fu
    echo "stop ok!"
    exit 0
}

if [[ $1 == "start" ]] && [[ $# != 3 ]]; then
    echo "usage:"
    echo "usb-wifi.sh start wifissid wifipwd"
    echo "usb-wifi.sh stop"
    exit 0
fi

case "$1" in
    start)
        echo "usb wifi start"
        wifi_start
        ;;
    stop)
        echo "usb wifi stop"
        wifi_stop
        ;;
    *)
        echo "usage:"
        echo "usb-wifi.sh start wifissid wifipwd"
        echo "usb-wifi.sh stop"
        exit 1
        ;;
esac
