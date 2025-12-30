#!/bin/sh

mkdir -p /var/lock/

if [ $1 == "print" ]; then
    fw_printenv -c /etc/ax_env.config ${@:5}
    exit 0
fi


if [ $1 == "set" ]; then
    fw_setenv -c /etc/ax_env.config ${@:3}
    exit 0
fi

echo "usage: $0 print/set"
