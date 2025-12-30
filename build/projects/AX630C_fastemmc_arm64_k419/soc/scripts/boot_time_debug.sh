#!/bin/sh

#get romcode boot time
g_romtime=$(devmem 0x900)
romcode_time=$(($g_romtime * 1000 / 32))

us2ms() {
	quotient=$(($1 / 1000))
	remainder=$(($1 % 1000))

	decimal=$(echo "scale=3; $remainder / 1000" | bc)
	if [[ "$decimal" =~ "\." ]];then
		echo "${quotient}${decimal}"
	else
		echo "${quotient}.${decimal}"
	fi
}

get_time_24m() {
	time_tmp=$(devmem $1)
	time_tmp=$(((($time_tmp - $g_romtime) / 24) + $romcode_time))
	echo $time_tmp
}

#get read ddr init data start time
ddr_data_st=$(get_time_24m 0x904)

#get read ddr init data end time
ddr_data_end=$(get_time_24m 0x908)

#get ddr init end time
ddr_init_end=$(get_time_24m 0x90c)

#get read kernel end time
read_kernel_end=$(get_time_24m 0x910)

#get read dtb end time
read_dtb_end=$(get_time_24m 0x914)

#get read ramdisk end time
read_ramdisk_end=$(get_time_24m 0x918)

#get spl read img end time
spl_read_img_end=$(get_time_24m 0x91c)

#get kernel start time
kernel_start_time=$(get_time_24m 0x920)

#get arch tmr start time
arch_tmr_start_time=$(get_time_24m 0x924)

#get mount start time
mount_root_time=$(get_time_24m 0x928)

#get rcS start time
rcS_start_time=$(get_time_24m 0x92c)

#get auto drv load end time
auto_drv_end=$(get_time_24m 0x930)

#get npu set bw limit end time
set_bw_limit_end=$(get_time_24m 0x934)

#get axklogd end time
rcs_end=$(get_time_24m 0x938)

#get setup_machine_fdt start time
setup_machine_fdt_start=$(get_time_24m 0x990)

#get setup_machine_fdt end time
setup_machine_fdt_end=$(get_time_24m 0x994)

#get parse_early_param start time
parse_early_param_start=$(get_time_24m 0x998)

#get parse_early_param end time
parse_early_param_end=$(get_time_24m 0x99c)

#get unflatten_device_tree start time
unflatten_device_tree_start=$(get_time_24m 0x9A0)

#get unflatten_device_tree end time
unflatten_device_tree_end=$(get_time_24m 0x9A4)

#get mm_init start time
mm_init_start=$(get_time_24m 0x9AC)

#get mm_init end time
mm_init_end=$(get_time_24m 0x9C0)

#get of_clk_init start time
of_clk_init_start=$(get_time_24m 0x9C4)

#get of_clk_init end time
of_clk_init_end=$(get_time_24m 0x9C8)

#get kernel_init start time
kernel_init_start=$(get_time_24m 0x9CC)

#get driver_init start time
driver_init_start=$(get_time_24m 0x9D0)

#get driver_init end time
driver_init_end=$(get_time_24m 0x9D4)

#get do_initcalls start time
do_initcalls_start=$(get_time_24m 0x9D8)

#get do_initcalls end time
do_initcalls_end=$(get_time_24m 0x9DC)

#get prepare_namespace.wait start time
prepare_namespace_wait_start=$(get_time_24m 0x9E0)

#get prepare_namespace.wait end time
prepare_namespace_wait_end=$(get_time_24m 0x9E4)

#get prepare_namespace.wait_root_dev time
prepare_namespace_wait_root_dev_end=$(get_time_24m 0x9E8)

#get prepare_namespace.mount_root time
prepare_namespace_mount_root_end=$(get_time_24m 0x9EC)

#get mark_readonly start time
mark_readonly_start=$(get_time_24m 0x9F0)

#get mark_readonly end time
mark_readonly_end=$(get_time_24m 0x9F4)

#get wait_for_initramfs start time
wait_for_initramfs_start=$(get_time_24m 0x9B0)

#get wait_for_initramfs end time
wait_for_initramfs_end=$(get_time_24m 0x9B4)

#echo "Boot time total time: $(us2ms $rcs_end) ms"
echo "Rom to rcS starttime: $(us2ms $rcS_start_time) ms"
echo "Rom to kernel start time: $(us2ms $kernel_start_time) ms"
echo "    |_____Rom to spl timer init time: $(us2ms $romcode_time) ms"
echo "    |_____Read ddr init data time: $(us2ms $(($ddr_data_end - $ddr_data_st))) ms"
echo "    |_____DDR init time: $(us2ms $(($ddr_init_end - $ddr_data_end))) ms"
echo "    |_____Read kernel time: $(us2ms $(($read_kernel_end - $ddr_init_end))) ms"
echo "    |_____Read dtb time: $(us2ms $(($read_dtb_end - $read_kernel_end))) ms"
echo "    |_____Read atf time: $(us2ms $(($spl_read_img_end - $read_dtb_end))) ms"
#echo "Read ramdisk time: $(us2ms $(($read_ramdisk_end - $read_dtb_end))) ms"
echo "    |_____SPL read img end to kernel start time: $(us2ms $(($kernel_start_time - $spl_read_img_end))) ms"
echo "Kernel start to arch timer enable time: $(us2ms $(($arch_tmr_start_time - $kernel_start_time))) ms"
echo "    |_____setup_machine_fdt time: $(us2ms $(($setup_machine_fdt_end - $setup_machine_fdt_start))) ms"
echo "    |_____parse_early_param time: $(us2ms $(($parse_early_param_end - $parse_early_param_start))) ms"
echo "    |_____unflatten_device_tree time: $(us2ms $(($unflatten_device_tree_end - $unflatten_device_tree_start))) ms"
echo "    |_____mm_init time: $(us2ms $(($mm_init_end - $mm_init_start))) ms"
echo "    |_____of_clk_init: $(us2ms $(($of_clk_init_end -$of_clk_init_start))) ms"
echo "Arch timer enable to mount root time time: $(us2ms $(($mount_root_time - $arch_tmr_start_time))) ms"
echo "    |_____arch timer to kernel_init time: $(us2ms $(($kernel_init_start - $arch_tmr_start_time))) ms"
echo "    |_____driver_init time: $(us2ms $(($driver_init_end - $driver_init_start))) ms"
echo "    |_____do_initcalls time: $(us2ms $(($do_initcalls_end - $do_initcalls_start))) ms"
echo "    |_____wait_for_initramfs time: $(us2ms $(($wait_for_initramfs_end - $wait_for_initramfs_start))) ms"
echo "    |_____prepare_namespace wait device probe time: $(us2ms $(($prepare_namespace_wait_end - $prepare_namespace_wait_start))) ms"
echo "    |_____prepare_namespace wait root device time: $(us2ms $(($prepare_namespace_wait_root_dev_end - $prepare_namespace_wait_end))) ms"
echo "Mount root to rcS time: $(us2ms $(($rcS_start_time - $mount_root_time))) ms"
echo "    |_____mount root time: $(us2ms $(($prepare_namespace_mount_root_end - $prepare_namespace_wait_root_dev_end))) ms"
echo "    |_____mark_readonly: $(us2ms $(($mark_readonly_end - $mark_readonly_start))) ms"
#echo "Kernel start to to mount root time: $(us2ms $(($mount_root_time - $kernel_start_time))) ms"
#echo "Kernel start to to rcS time: $(us2ms $(($rcS_start_time - $kernel_start_time))) ms"
#echo "Auto drv load time: $(us2ms $(($auto_drv_end - $rcS_start_time))) ms"
#echo "NPU set bw limit time: $(us2ms $(($set_bw_limit_end - $auto_drv_end))) ms"
#echo "Axlogd load time: $(us2ms $(($rcs_end - $set_bw_limit_end))) ms"

