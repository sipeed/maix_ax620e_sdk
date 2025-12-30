#!/bin/sh

WIFISSID=$2
WIFIPWD=$3

if [ ! -d "/etc/wifi" ]; then
    mkdir /etc/wifi
fi

function wifi_start()
{
    insmod /soc/ko/rtl8188fu.ko
    sleep 1

    ifconfig wlan0 up

    echo "interface wlan0
start 192.168.100.2
end 192.168.100.254
opt subnet 255.255.255.0
opt router 192.168.100.1
opt dns 114.114.114.114
lease_file /etc/wifi/udhcpd.leases
" > /etc/wifi/udhcpd.conf
    touch /etc/wifi/udhcpd.leases

    echo "interface=wlan0
driver=nl80211
ctrl_interface=/var/hostapd
ssid=$WIFISSID
channel=8
hw_mode=g
ignore_broadcast_ssid=0
macaddr_acl=0
auth_algs=1
wpa=3
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP CCMP
rsn_pairwise=TKIP CCMP
wpa_passphrase=$WIFIPWD
" > /etc/wifi/hostapd.conf

    ifconfig wlan0 192.168.100.1 netmask 255.255.255.0 up
    route add default gw 192.168.100.1 dev wlan0
    hostapd /etc/wifi/hostapd.conf &
    udhcpd -fS /etc/wifi/udhcpd.conf &

    echo "start ok"
    exit 0
}

function wifi_stop()
{
    killall udhcpd hostapd
    rm -rf /etc/wifi/udhcpd.conf
    rm -rf /etc/wifi/hostapd.conf
    rm -rf /etc/wifi/udhcpd.leases
    ifconfig wlan0 down
    rmmod rtl8188fu

    echo "stop ok!"
    exit 0
}

if [[ $1 == "start" ]] && [[ $# != 3 ]]; then
    echo "usage:"
    echo "usb-hotspot.sh start wifissid wifipwd"
    echo "usb-hotspot.sh stop"
    exit 0
fi

case "$1" in
    start)
        echo "usb wifi hotspot start"
        wifi_start
        ;;
    stop)
        echo "usb wifi hotspot stop"
        wifi_stop
        ;;
    *)
        echo "usage:"
        echo "usb-hotspot.sh start wifissid wifipwd"
        echo "usb-hotspot.sh stop"
        exit 1
        ;;
esac
