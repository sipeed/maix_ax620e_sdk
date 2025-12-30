#!/bin/sh

switch_normal() {
	devmem 0x239002C 32 0x800
	fw_setenv bootable normal
	echo "you can reboot, switch to normal"
}

switch_recovery() {
	devmem 0x2390028 32 0x800
	fw_setenv bootable recovery
	fw_setenv upgrade_available 0
	echo "you can reboot, switch to recovery"
}

case "$1" in
	normal)
		switch_normal
		;;
	recovery)
		switch_recovery
		;;
	*)
		exit 1
esac
exit $?
