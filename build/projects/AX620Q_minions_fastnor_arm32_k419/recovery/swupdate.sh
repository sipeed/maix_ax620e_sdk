#!/bin/sh

help_function() {
	echo -e "\e[33m"
	echo "URL:   swupdate.sh -u 10.126.12.107:8000/AX620E_recovery_xxx.swu"
	echo "Local: swupdate.sh -i /mnt/AX620E_recovery_xxx.swu"
	echo -e "\e[0m"
}

while getopts "u:i:h" opt; do
	case $opt in
		u )
			RECOVERY_OPT="-d"
			RECOVERY_PARM="-u ${OPTARG}"
			;;
		i )
			RECOVERY_OPT="-i"
			RECOVERY_PARM="${OPTARG}"
			;;
		h )
			help_function
			exit
			;;
		\? )
			help_function
			exit
			;;
	esac
done


if [ -z "$RECOVERY_PARM" ]; then
	help_function
	exit 1
fi

swupdate -e stable,copy -f /etc/swupdate.cfg ${RECOVERY_OPT} "${RECOVERY_PARM}" -v
