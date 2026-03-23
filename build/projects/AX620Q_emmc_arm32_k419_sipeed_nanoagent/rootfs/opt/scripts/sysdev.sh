#!/usr/bin/env bash

readonly SCRIPTS_ROOT="/kvmcomm/scripts"
readonly WIFI_SCRIPT="${SCRIPTS_ROOT}/wifi.sh"

(
    sleep 10
    "${WIFI_SCRIPT}" try_connect > /dev/null 2>&1
) &

trap 'exit 0' SIGINT SIGTERM

while true; do
    sleep 10
done
