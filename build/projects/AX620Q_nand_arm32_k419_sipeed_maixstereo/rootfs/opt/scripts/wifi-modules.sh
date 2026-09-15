#!/bin/bash
set -e

load_modules() {
    if ! lsmod | grep -q '^aic8800_bsp\b'; then
        insmod /soc/ko/aic8800_bsp.ko
    fi
    if ! lsmod | grep -q '^aic8800_fdrv\b'; then
        insmod /soc/ko/aic8800_fdrv.ko aicwf_dbg_level=0
    fi

    local retry=0
    while [ $retry -lt 20 ]; do
        if ip -o link show | grep -q 'wlan'; then
            return 0
        fi
        sleep 0.5
        retry=$((retry + 1))
    done

    echo "[wifi-modules] wlan interface not found after module load" >&2
    return 1
}

unload_modules() {
    if lsmod | grep -q '^aic8800_fdrv\b'; then
        rmmod /soc/ko/aic8800_fdrv.ko || true
    fi
    if lsmod | grep -q '^aic8800_bsp\b'; then
        rmmod /soc/ko/aic8800_bsp.ko || true
    fi
}

case "$1" in
    load)
        load_modules
        ;;
    unload)
        unload_modules
        ;;
    restart)
        unload_modules
        load_modules
        ;;
    *)
        echo "Usage: $0 {load|unload|restart}" >&2
        exit 2
        ;;
esac
