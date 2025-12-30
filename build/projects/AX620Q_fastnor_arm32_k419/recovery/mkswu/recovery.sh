#!/bin/sh

function preinst() {
    echo "preinst"
    fw_setenv upgrade_available 0
}

function postinst() {
    echo "postinst"
    /usr/bin/bootable-set.sh normal
}

if [ "$1" == "preinst" ]; then
    preinst
fi

if [ "$1" == "postinst" ]; then
    postinst
fi
