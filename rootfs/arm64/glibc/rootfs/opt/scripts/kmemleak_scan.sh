#!/bin/sh

debug_path=/sys/kernel/debug
kmemleak_path=$debug_path/kmemleak
scan_interavl=30
save_dir=/opt/data/AXSyslog/kernel
fsave=$save_dir/kmemleak_`date '+%Y%m%d_%H%M%S'`.txt

if [ ! -e $kmemleak_path ]; then
	mount -t debugfs none $debug_path
	sleep 1
	if [ ! -e $kmemleak_path ]; then
		echo "[warning] kernel does NOT support kmemleak!"
		exit
	fi
fi


[ -f /tmp/kmemleak.lock ] && exit

echo $$ > /tmp/kmemleak.lock

if [ ! -d $save_dir ]; then
	mkdir -p $save_dir
fi

echo "saving kmemleaks to $fsave"

while true
do
	cat $kmemleak_path >> $fsave
	echo clear > $kmemleak_path
	echo scan $scan_interavl > $kmemleak_path
	sleep $scan_interavl
done
