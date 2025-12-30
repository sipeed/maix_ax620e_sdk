#!/bin/sh

SD_DEVICE=/dev/mmcblk0p1
SD_RECYCLE_SIZE=256 #MB
MOUNT_POS=/mnt
SYSLOG_PATH=/tmp/AXSyslog
AXSYSLOG_CFG_FILE=/etc/ax_syslog.conf
AXSYSLOGD_PAHT=/etc/init.d/axsyslogd
BOOTLOG_ROOT_PATH=$MOUNT_POS/bootlogs
NEW_LOG_PATH=$BOOTLOG_ROOT_PATH/0

function reload_axsyslog {
    axsyslog_cfg_file=$AXSYSLOG_CFG_FILE
    link_info=$(ls -l "$axsyslog_cfg_file")
    if [ $? -eq 0 ]; then
        if echo "$link_info" | grep -q '^l'; then
            axsyslog_cfg_file=$(echo "$link_info" | awk '{print $11}')
        fi
    else
        echo "[error] Failed to check file: $axsyslog_cfg_file"
        exit -1
    fi
    sed -i "s|AX_SYSLOG_path = [^#]*|AX_SYSLOG_path = $NEW_LOG_PATH |" $axsyslog_cfg_file
    sed -i "s|AX_SYSLOG_percent = [^#]*|AX_SYSLOG_percent = 90 |" $axsyslog_cfg_file
    sed -i "s|AX_SYSLOG_filesizemax = [^#]*|AX_SYSLOG_filesizemax = 20 |" $axsyslog_cfg_file
    sed -i "s|AX_SYSLOG_volume = [^#]*|AX_SYSLOG_volume = 1024 |" $axsyslog_cfg_file
    $AXSYSLOGD_PAHT reload
}

function logger_init {
    if [ -e "$SD_DEVICE" ]; then
        if mount | grep -q $SD_DEVICE; then
            echo "[info] $SD_DEVICE is mounted."
        else
            if [ -e "$SD_DEVICE" ]; then
                echo "[info] mounting $SD_DEVICE ..."
                mount $SD_DEVICE $MOUNT_POS
                if mount | grep -q $SD_DEVICE; then
                    echo "[info] $SD_DEVICE is mounted."
                else
                    echo "[error] Failed to mount $SD_DEVICE !!!"
                    exit -1
                fi
            else
                echo "[error] Cannot find SD device $SD_DEVICE !!!"
                exit -1
            fi
        fi
        mkdir -p $BOOTLOG_ROOT_PATH
    else
        if mount | grep -q $SD_DEVICE; then
            umount $MOUNT_POS
        fi
        while [ ! -e "$SD_DEVICE" ]
        do
            echo "[warning] Cannot find SD device $SD_DEVICE !!!"
            sleep 0.1
        done
        mount $SD_DEVICE $MOUNT_POS
        if mount | grep -q $SD_DEVICE; then
            echo "[info] $SD_DEVICE is mounted."
        else
            echo "[error] Failed to mount $SD_DEVICE !!!"
            exit -1
        fi
        mkdir -p $BOOTLOG_ROOT_PATH
    fi
}

function rename_old_log_dirs {
    counter=0
    log_dir=$BOOTLOG_ROOT_PATH
    log_dirs=$(ls -l $log_dir | grep '^d' | awk '{print $9}' | grep -o '^[0-9]\+' | sort -rn)
    for dir in $log_dirs; do
        tmp_dir=$(echo $dir | sed 's/[^0-9]//g')
        tmp_dir=$((tmp_dir * 1000))
        counter=$((counter + 1))
        if [ $tmp_dir -ne 0 ] ; then
            src_dir=$log_dir/$dir
            dst_dir=$log_dir/$tmp_dir
            mv $src_dir $dst_dir
        fi
    done
    log_dirs=$(ls -l $log_dir | grep '^d' | awk '{print $9}' | grep -o '^[0-9]\+' | sort -rn)
    for dir in $log_dirs; do
        src_dir=$log_dir/$dir
        dst_dir=$log_dir/$counter
        counter=$((counter - 1))
        mv $src_dir $dst_dir
    done
}

function clear_for_write {
    log_dir=$BOOTLOG_ROOT_PATH
    free_m=$(df -m | grep $SD_DEVICE | awk '{print $4}')
    if [ $free_m -lt $SD_RECYCLE_SIZE ] ;then
        echo "[warning] NOT enough space, starting to delete history logs ..."
        log_dirs=$(ls -l $log_dir | grep '^d' | awk '{print $9}' | grep -o '^[0-9]\+' | sort -rn)
        for dir in $log_dirs; do
            cur_dir=$log_dir/$dir
            echo "[info] removing $cur_dir ..."
            rm -rf $cur_dir
            free_m=$(df -m | grep $SD_DEVICE | awk '{print $4}')
            if [ $free_m -gt $SD_RECYCLE_SIZE ] ;then
                break
            fi
        done
    fi
}

function save_logs {
    rename_old_log_dirs
    clear_for_write
    mkdir -p $NEW_LOG_PATH
    cat /proc/ax_proc/riscv/log_dump > $NEW_LOG_PATH/riscv.log
    cp -r $SYSLOG_PATH $NEW_LOG_PATH/boot_syslog
    reload_axsyslog
    sync
}

logger_init
save_logs
