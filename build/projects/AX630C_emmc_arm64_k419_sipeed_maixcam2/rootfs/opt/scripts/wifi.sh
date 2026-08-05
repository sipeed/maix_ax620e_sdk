#!/bin/bash

function gen_hostapd_conf() {
	ssid="${1}"
	pass="${2}"
	echo "ctrl_interface=/var/run/hostapd"
	echo "ctrl_interface_group=0"
	echo "ssid=${ssid}"
	echo "hw_mode=g"
	echo "channel=1"
	echo "beacon_int=100"
	echo "dtim_period=2"
	echo "max_num_sta=255"
	echo "rts_threshold=-1"
	echo "fragm_threshold=-1"
	echo "macaddr_acl=0"
	echo "auth_algs=3"
	echo "wpa=2"
	echo "wpa_passphrase=${pass}"
	echo "ieee80211n=1"
}

function dhcp_client_run()
{
    # DHCP is owned by udhcpc.service. wifi.service pulls it in for STA and
    # no-marker default-STA mode, so do not start a second client here.
    return
}

function wpa_supplicant_run()
{
    wpa_supplicant -i wlan0 -Dnl80211 -c /etc/wpa_supplicant.conf
}

function wifi_processes_stop()
{
    pkill wpa_supplicant || true
    pkill hostapd || true
    pkill udhcpd || true
    ifconfig wlan0 down || true
}

function wpa_supplicant_start()
{
    if [ -e /boot/wifi.sta ]
	then
		echo "wifi mode: sta"
		if [ -e /boot/wpa_supplicant.conf ]
		then
			cp /boot/wpa_supplicant.conf /etc/wpa_supplicant.conf || return 1
			chmod 0600 /etc/wpa_supplicant.conf || return 1
		else
			ssid=""
			pass=""
			if [ -e /boot/wifi.ssid ]
			then
				echo -n "ssid: "
				cat /boot/wifi.ssid
				ssid=`cat /boot/wifi.ssid`
			fi
			if [ -e /boot/wifi.pass ]
			then
				pass=`cat /boot/wifi.pass`
			fi
			if [ ! -z "${ssid}${pass}" ]
			then
				echo "ctrl_interface=/var/run/wpa_supplicant" > /etc/wpa_supplicant.conf || return 1
				wpa_passphrase "$ssid" "$pass" >> /etc/wpa_supplicant.conf || return 1
				chmod 0600 /etc/wpa_supplicant.conf || return 1
			fi
		fi
		wpa_supplicant_run
        dhcp_client_run
	elif [ -e /boot/wifi.ap ]
	then
		echo "wifi mode: ap"
		if [ -e /boot/hostapd.conf ]
		then
			cp /boot/hostapd.conf /etc/hostapd.conf || return 1
			chmod 0600 /etc/hostapd.conf || return 1
		else
			id2=$(printf "%d" 0x$(sha512sum /proc/ax_proc/uid | head -c 2))
            id3=$(printf "%d" 0x$(sha512sum /proc/ax_proc/uid | head -c 4 | tail -c 2))
            id3=$((id3 + 1))
			if [ "$id2" = "$id3" ]
			then
				id2=$((id2 + 1))
			fi
			if [ "$id2" -ge 255 ]
			then
				id2=253
			fi
			if [ "$id3" -ge 255 ]
			then
				id3=254
			fi
			ssid="maixcam2-${id2}${id3}"
			pass="maixcam2"
			if [ -e /boot/wifi.ssid ]
			then
				echo -n "ssid: "
				cat /boot/wifi.ssid
				ssid=`cat /boot/wifi.ssid`
			fi
			if [ -e /boot/wifi.pass ]
			then
				pass=`cat /boot/wifi.pass`
			fi
			gen_hostapd_conf "$ssid" "$pass" > /etc/hostapd.conf || return 1
			chmod 0600 /etc/hostapd.conf || return 1
		fi
		if [ -e /boot/wifi.ipv4_prefix ]
		then
			ipv4_prefix=`cat /boot/wifi.ipv4_prefix`
		else
			ipv4_prefix=10.$id2.$id3
		fi
		if [ ! -e /etc/udhcpd.wlan0.conf ]
		then
			gen_udhcpd_conf wlan0 "${ipv4_prefix}"  > /etc/udhcpd.wlan0.conf
		fi
		ifconfig wlan0 up || return 1
		ip route del default || true
		# routes=$(ip route show | grep 'dev wlan0' | awk '{print $1}')
		# for route in $routes; do
		# 	ip route del $route dev wlan0
		# 	echo "Deleted route $route dev wlan0"
		# done
		ip addr flush dev wlan0 || return 1
		ip addr add "${ipv4_prefix}.1/24" dev wlan0 || return 1
		udhcpd -S /etc/udhcpd.wlan0.conf || return 1
		exec hostapd -i wlan0 /etc/hostapd.conf
	elif [ -e /boot/wifi.mon ]
	then
		echo "wifi mode: mon"
		airmon-ng start wlan0
    else
        default_wifi_ssid="SBC_test_mgr"
        default_wifi_passwd="Sipeed123.."
        ifconfig wlan0 up
        if [ -e /etc/wpa_supplicant.conf ]; then
            wpa_supplicant_run
        else
            echo "ctrl_interface=/var/run/wpa_supplicant" > /etc/wpa_supplicant.conf
            wpa_passphrase ${default_wifi_ssid} ${default_wifi_passwd} >> /etc/wpa_supplicant.conf
            wpa_supplicant_run
        fi
		dhcp_client_run
	fi
}

function wifi_stop()
{
    wifi_processes_stop
    if lsmod | grep -q aic8800_fdrv; then
        rmmod /soc/ko/aic8800_fdrv.ko
    fi
    if lsmod | grep -q aic8800_bsp; then
        rmmod /soc/ko/aic8800_bsp.ko
    fi
}

function wifi_start()
{
    devmem 0x104F200C 32 0x00000008 # SDIO_DAT0
    devmem 0x104F2018 32 0x00000008 # SDIO_DAT1
    devmem 0x104F2024 32 0x00000008 # SDIO_CLK
    devmem 0x104F2030 32 0x00000008 # SDIO_CMD
    devmem 0x104F203C 32 0x00000008 # SDIO_DAT2
    devmem 0x104F2048 32 0x00000008 # SDIO_DAT3

    if lsmod | grep -q aic8800_bsp; then
        echo "aic8800_bsp already loaded"
    else
        insmod /soc/ko/aic8800_bsp.ko
    fi

    if lsmod | grep -q aic8800_fdrv; then
        echo "aic8800_fdrv already loaded"
        exit 1
    else
        insmod /soc/ko/aic8800_fdrv.ko
    fi

    wpa_supplicant_start
}

case "$1" in
    start)
        echo "wifi start"
        wifi_start
        ;;
    stop)
        echo "wifi stop"
        wifi_stop
        ;;
    restart)
        echo "wifi restart"
        wifi_stop
        wifi_start
        ;;
    *)
        echo "usage:"
        echo "wifi.sh start"
        echo "wifi.sh stop"
        echo "wifi.sh restart"
        exit 1
        ;;
esac
