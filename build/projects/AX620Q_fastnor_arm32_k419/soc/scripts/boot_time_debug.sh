#!/bin/sh

#get romcode boot time
g_romtime=$((`devmem 0x900`))
romcode_time=$(($g_romtime / 32 * 1000))

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
	time_tmp=$((`devmem $1`))
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

#get axklogd end time
rcs_end=$(get_time_24m 0x92c)

#get spl start load riscv time
spl_start_load_riscv=$(get_time_24m 0x954)

#get spl end load riscv time
spl_end_load_riscv=$(get_time_24m 0x958)

#get riscv entry init time
riscv_entry_init=$(get_time_24m 0x95c)

#get spl wait riscv ready time
spl_wait_riscv_ready=$(get_time_24m 0x960)

#get spl get riscv ready time
spl_riscv_ready=$(get_time_24m 0x964)

#get riscv start load rootfs time
riscv_start_load_rootfs=$(get_time_24m 0x968)

#get riscv end load rootfs time
riscv_end_load_rootfs=$(get_time_24m 0x96c)

#get linux mount rootfs and wait for ready time
mount_start_wait=$(get_time_24m 0x970)

#get linux mount rootfs get ready time
mount_end_wait=$(get_time_24m 0x974)

echo "Boot time total time: $(us2ms $rcs_end) ms"
echo "Rom to timer init time: $(us2ms $romcode_time) ms"
echo "Read ddr init data time: $(us2ms $(($ddr_data_end - $ddr_data_st))) ms"
echo "DDR init time: $(us2ms $(($ddr_init_end - $ddr_data_end))) ms"
echo "Read kernel time: $(us2ms $(($read_kernel_end - $ddr_init_end))) ms"
echo "Read dtb time: $(us2ms $(($read_dtb_end - $read_kernel_end))) ms"
echo "Read ramdisk time: $(us2ms $(($read_ramdisk_end - $read_dtb_end))) ms"
echo "SPL read img end to kernel start time: $(us2ms $(($kernel_start_time - $spl_read_img_end))) ms"
echo "Kernel start to arch timer enable time: $(us2ms $(($arch_tmr_start_time - $kernel_start_time))) ms"
echo "Arch timer enable to mount root time time: $(us2ms $(($mount_root_time - $arch_tmr_start_time))) ms"
echo "Kernel start to to mount root time: $(us2ms $(($mount_root_time - $kernel_start_time))) ms"
echo "Riscv: spl load riscv time: $(us2ms $(($spl_end_load_riscv - $spl_start_load_riscv))) ms"
echo "Riscv: rtthread boot up time: $(us2ms $(($riscv_entry_init - $spl_end_load_riscv))) ms"
echo "Riscv: spl wait for riscv ready time: $(us2ms $(($spl_riscv_ready - $spl_wait_riscv_ready))) ms"
echo "Riscv: riscv load rootfs time: $(us2ms $(($riscv_end_load_rootfs - $riscv_start_load_rootfs))) ms"
echo "Riscv: linux wait riscv load rootfs ready time: $(us2ms $(($mount_end_wait - $mount_start_wait))) ms"
