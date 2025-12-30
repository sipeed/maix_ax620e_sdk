#!/bin/sh
echo "removing loaded ko ..."
/soc/scripts/auto_load_all_drv.sh -r
sleep 3
echo "freeing reserved cmm ..."
echo freecmm > /proc/ax_proc/cmm_reserved
echo "done"
