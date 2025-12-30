#!/bin/sh

# set time sync method
# ptp-master, ptp-slave, disable

INITD_PATH="/etc/init.d"
ETC_PATH="/etc/"


case "$1" in
  slave)
        printf "Starting ptp slave mode...\n"
        ifconfig eth0 allmulti
        killall ntpd > /dev/null 2>&1
        mv /usr/sbin/ntpd /usr/sbin/ntpd.bak  > /dev/null 2>&1
        cp $INITD_PATH/linuxptp.slave $INITD_PATH/S65linuxptp
        cp $ETC_PATH/linuxptp.cfg.slave $ETC_PATH/linuxptp.cfg
        $INITD_PATH/S65linuxptp start
        ;;

  master)
        printf "Starting ptp master mode...\n"
        ifconfig eth0 allmulti
        cp $INITD_PATH/linuxptp.master $INITD_PATH/S65linuxptp
        cp $ETC_PATH/linuxptp.cfg.master $ETC_PATH/linuxptp.cfg
        $INITD_PATH/S65linuxptp start
        ;;

  disable)
        printf "stop ptp...\n"
        mv /usr/sbin/ntpd.bak /usr/sbin/ntpd  > /dev/null 2>&1
        $INITD_PATH/S65linuxptp stop
        rm -rf $INITD_PATH/S65linuxptp
        rm -rf $ETC_PATH/linuxptp.cfg
        ifconfig eth0 -allmulti
        ;;

  *)
        echo "Usage: $0 {master|slave|disable}"
        exit 1
esac

exit $?
