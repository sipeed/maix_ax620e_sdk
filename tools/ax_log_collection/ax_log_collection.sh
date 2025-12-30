#!/bin/sh
cur_path=$(cd "$(dirname $0)";pwd)
log_path=$cur_path
user_set_path=$cur_path
log_dir_name="isp"
log_compress_name="ax_log"
ax_analyzer_path=$cur_path/ax_analyzer
compress_cmd="tar czf"
compress_suffix=".tar.gz"

ax_base_proc()
{
    local tar_name="proc_base$compress_suffix"
    local log_name=$log_path/"proc_base.log"
    echo "**********************collect base proc********************"
    echo "####################################date##########################################" >> $log_name
    date >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "####################################chip_type##########################################" >> $log_name
    cat /proc/ax_proc/chip_type >> $log_name
    echo "####################################board_id##########################################" >> $log_name
    cat /proc/ax_proc/board_id >> $log_name
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_pool_proc()
{
    local tar_name="proc_pool$compress_suffix"
    local log_name=$log_path/"proc_pool.log"
    echo "**********************collect pool proc********************"
    echo "####################################mem_cmm_info##########################################" >> $log_name
    cat /proc/ax_proc/mem_cmm_info  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "####################################mem_cmm_info##########################################" >> $log_name
    cat /proc/ax_proc/mem_cmm_info  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "####################################pool##########################################" >> $log_name
    cat /proc/ax_proc/pool  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "####################################pool##########################################" >> $log_name
    cat /proc/ax_proc/pool  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "####################################os_mem_stat##########################################" >> $log_name
    cat /proc/ax_proc/os_mem/stat  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "####################################os_mem_stat##########################################" >> $log_name
    cat /proc/ax_proc/os_mem/stat  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_isp_proc()
{
    local tar_name="proc_isp$compress_suffix"
    local log_name=$log_path/"proc_isp.log"
    echo "**********************collect isp proc*********************"

    echo "####################################isp/info##########################################" >> $log_name
    cat /proc/ax_proc/isp/info  >> $log_name
    busybox sleep 0.1
    echo "####################################isp/info##########################################" >> $log_name
    cat /proc/ax_proc/isp/info  >> $log_name
    busybox sleep 0.1
    echo "####################################isp/status##########################################" >> $log_name
    cat /proc/ax_proc/isp/status  >> $log_name
    busybox sleep 0.1
    echo "####################################isp/status##########################################" >> $log_name
    cat /proc/ax_proc/isp/status  >> $log_name
    echo "####################################sensor/info##########################################" >> $log_name
    cat /proc/ax_proc/sensor/info  >> $log_name
    busybox sleep 0.1
    echo "####################################sensor/info##########################################" >> $log_name
    cat /proc/ax_proc/sensor/info  >> $log_name
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_vin_proc()
{
    local tar_name="proc_vin$compress_suffix"
    local log_name=$log_path/"proc_vin.log"

    echo "**********************collect vin proc*********************"

    echo "#######################################mipi_rx/attr#######################################" >> $log_name
    cat /proc/ax_proc/mipi_rx/attr  >> $log_name
    echo "#######################################mipi_rx/status#######################################" >> $log_name
    cat /proc/ax_proc/mipi_rx/status  >> $log_name
    busybox sleep 0.1
    echo "#######################################mipi_rx/status#######################################" >> $log_name
    cat /proc/ax_proc/mipi_rx/status  >> $log_name
    echo "#######################################vin/attr#######################################" >> $log_name
    cat /proc/ax_proc/vin/attr  >> $log_name
    echo "#######################################vin/statistics#######################################" >> $log_name
    cat /proc/ax_proc/vin/statistics  >> $log_name
    echo "#######################################vin/statistics#######################################" >> $log_name
    busybox sleep 1
    cat /proc/ax_proc/vin/statistics  >> $log_name
    echo "#######################################vin/statistics#######################################" >> $log_name
    busybox sleep 1
    cat /proc/ax_proc/vin/statistics  >> $log_name
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_ivps_proc()
{
    local tar_name="proc_ivps_gdc_rgn_vpp$compress_suffix"
    local log_name=$log_path/"proc_ivps_gdc_rgn_vpp.log"

    echo "**********************collect ivps proc****************"
    echo "#######################################ivps#######################################" >> $log_name
    cat /proc/ax_proc/ivps  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################ivps#######################################" >> $log_name
    cat /proc/ax_proc/ivps  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################ivps#######################################" >> $log_name
    cat /proc/ax_proc/ivps  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "#######################################gdc#######################################" >> $log_name
    cat /proc/ax_proc/gdc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################gdc#######################################" >> $log_name
    cat /proc/ax_proc/gdc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "#######################################rgn#######################################" >> $log_name
    cat /proc/ax_proc/rgn  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################rgn#######################################" >> $log_name
    cat /proc/ax_proc/rgn  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "#######################################vpp#######################################" >> $log_name
    cat /proc/ax_proc/vpp  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################vpp#######################################" >> $log_name
    cat /proc/ax_proc/vpp  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "#######################################interrupts#######################################" >> $log_name
    cat /proc/interrupts  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################interrupts#######################################" >> $log_name
    cat /proc/interrupts  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_ivps_proc_650()
{
    local tar_name="proc_ivps_gdc_rgn_vdsp$compress_suffix"
    local log_name=$log_path/"proc_ivps_gdc_rgn_vdsp.log"

    echo "**********************collect ivps proc****************"
    echo debug_info 1 > /proc/ax_proc/ivps
    echo "#######################################ivps#######################################" >> $log_name
    cat /proc/ax_proc/ivps  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################ivps#######################################" >> $log_name
    cat /proc/ax_proc/ivps  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################ivps#######################################" >> $log_name
    cat /proc/ax_proc/ivps  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "#######################################gdc#######################################" >> $log_name
    cat /proc/ax_proc/gdc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################gdc#######################################" >> $log_name
    cat /proc/ax_proc/gdc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "#######################################rgn#######################################" >> $log_name
    cat /proc/ax_proc/rgn  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################rgn#######################################" >> $log_name
    cat /proc/ax_proc/rgn  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "#######################################vdsp#######################################" >> $log_name
    cat /proc/ax_proc/vdsp  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################vdsp#######################################" >> $log_name
    cat /proc/ax_proc/vdsp  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_venc_proc()
{
    local tar_name="proc_venc$compress_suffix"
    local log_name=$log_path/"proc_venc.log"
    echo "**********************collect venc proc********************"
    echo "#######################################venc#######################################" >> $log_name
    cat /proc/ax_proc/venc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################venc#######################################" >> $log_name
    cat /proc/ax_proc/venc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################venc#######################################" >> $log_name
    cat /proc/ax_proc/venc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_jenc_proc()
{
    local tar_name="proc_jenc$compress_suffix"
    local log_name=$log_path/"proc_jenc.log"
    echo "**********************collect jenc proc********************"
    echo "#######################################jenc#######################################" >> $log_name
    cat /proc/ax_proc/jenc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################jenc#######################################" >> $log_name
    cat /proc/ax_proc/jenc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_vdec_proc()
{
    local tar_name="proc_vdec$compress_suffix"
    local log_name=$log_path/"proc_vdec.log"
    echo "**********************collect vdec proc********************"
    echo "#######################################vdec#######################################" >> $log_name
    cat /proc/ax_proc/vdec  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#######################################vdec#######################################" >> $log_name
    cat /proc/ax_proc/vdec  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_vo_proc()
{
    local tar_name="proc_vo$compress_suffix"
    local log_name=$log_path/"proc_vo.log"
    echo "**********************collect vo proc**********************"
    echo "#######################################vo_display#######################################" >> $log_name
    cat /proc/ax_proc/vo/display_status  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################vo_display#######################################" >> $log_name
    cat /proc/ax_proc/vo/display_status  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################vo_display#######################################" >> $log_name
    cat /proc/ax_proc/vo/display_status  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    echo "#######################################vo_layer#######################################" >> $log_name
    cat /proc/ax_proc/vo/layer_status  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################vo_layer#######################################" >> $log_name
    cat /proc/ax_proc/vo/layer_status  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################vo_layer#######################################" >> $log_name
    cat /proc/ax_proc/vo/layer_status  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_audio_proc()
{
    local tar_name="proc_audio$compress_suffix"
    local log_name=$log_path/"proc_audio.log"
    echo "**********************collect audio proc**********************"
    echo "#######################################ai#######################################" >> $log_name
    cat /proc/ax_proc/ai  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################ai#######################################" >> $log_name
    cat /proc/ax_proc/ai  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################ao#######################################" >> $log_name
    cat /proc/ax_proc/ao  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################ao#######################################" >> $log_name
    cat /proc/ax_proc/ao  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################aenc#######################################" >> $log_name
    cat /proc/ax_proc/aenc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################aenc#######################################" >> $log_name
    cat /proc/ax_proc/aenc  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################adec#######################################" >> $log_name
    cat /proc/ax_proc/adec  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "#######################################adec#######################################" >> $log_name
    cat /proc/ax_proc/adec  >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_analyzer_proc()
{
    echo "**********************collect ax_analyzer******************"
    $ax_analyzer_path -m0 -d $log_path/ax_analyzer_1.bin
    busybox sleep 0.1
    $ax_analyzer_path -m0 -d $log_path/ax_analyzer_2.bin
    busybox sleep 0.1
    $ax_analyzer_path -m0 -d $log_path/ax_analyzer_3.bin
    $ax_analyzer_path -m10 -s1 -d $log_path/node_1.bin
    $ax_analyzer_path -m10 -s2 -d $log_path/priv_pool_1.bin
    $ax_analyzer_path -m10 -s3 -d $log_path/mem_agent_1.bin
    if grep -q "650" /proc/ax_proc/chip_type; then
        $ax_analyzer_path -m10 -s4 -d $log_path/scheduler_1.bin
    fi
    busybox sleep 0.1
    $ax_analyzer_path -m10 -s1 -d $log_path/node_2.bin
    $ax_analyzer_path -m10 -s2 -d $log_path/priv_pool_2.bin
    $ax_analyzer_path -m10 -s3 -d $log_path/mem_agent_2.bin
    if grep -q "650" /proc/ax_proc/chip_type; then
        $ax_analyzer_path -m10 -s4 -d $log_path/scheduler_2.bin
    fi
}

ax_reg_proc()
{
    local tar_name="proc_reg$compress_suffix"
    local log_name=$log_path/"proc_reg.log"
    local bin_name=$log_path/"isp_reg.bin"
    local bin_name_1=$log_path/"isp_reg_1.bin"
    local ife_top_addr=2400000
    local itp_top_addr=2480000
    local yuv_top_addr=24c0000

    if grep -q "650" /proc/ax_proc/chip_type; then
        ife_top_addr=12000000
        itp_top_addr=12080000
        yuv_top_addr=120c0000
    else
        ax_lookat 0x2400000 -t 0x90400 -f $bin_name
        dd if=/dev/zero bs=1 count=15360 >> $bin_name
        ax_lookat 0x2494000 -t 0x3D000 -f $bin_name_1
        cat $bin_name_1 >> $bin_name
        rm $bin_name_1
    fi
    echo "**********************collect reg ************************"
    echo "##################################isp/top############################################" >> $log_name
    ax_lookat $ife_top_addr -n 300 >> $log_name
    ax_lookat $itp_top_addr -n 300 >> $log_name
    ax_lookat $yuv_top_addr -n 300 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################isp/top############################################" >> $log_name
    ax_lookat $ife_top_addr -n 300 >> $log_name
    ax_lookat $itp_top_addr -n 300 >> $log_name
    ax_lookat $yuv_top_addr -n 300 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################isp/top############################################" >> $log_name
    ax_lookat $ife_top_addr -n 300 >> $log_name
    ax_lookat $itp_top_addr -n 300 >> $log_name
    ax_lookat $yuv_top_addr -n 300 >> $log_name
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_ivps_reg_proc()
{
    local tar_name="ivps_reg_proc$compress_suffix"
    mkdir $log_path/ivps_reg/
    chmod 777 $log_path/ivps_reg/
    echo "**********************collect ivps reg ************************"
    ax_lookat 4403000 -t 0x3ff -f "$log_path/ivps_reg/ivps_reg_vpp_0.bin"
    busybox sleep 0.5
    ax_lookat 4403000 -t 0x3ff -f "$log_path/ivps_reg/ivps_reg_vpp_1.bin"
    busybox sleep 0.5

    ax_lookat 4404000 -t 0x3ff -f "$log_path/ivps_reg/ivps_reg_gdc_0.bin"
    busybox sleep 0.5
    ax_lookat 4404000 -t 0x3ff -f "$log_path/ivps_reg/ivps_reg_gdc_1.bin"
    busybox sleep 0.5

    ax_lookat 4405000 -t 0x3ff -f "$log_path/ivps_reg/ivps_reg_tdp_0.bin"
    busybox sleep 0.5
    ax_lookat 4405000 -t 0x3ff -f "$log_path/ivps_reg/ivps_reg_tdp_1.bin"
    busybox sleep 0.5
}

ax_ivps_reg_proc_650()
{
    local tar_name="ivps_reg_proc$compress_suffix"
    mkdir $log_path/ivps_reg/
    chmod 777 $log_path/ivps_reg/

    echo "**********************collect ivps reg ************************"

    ax_lookat 0x1209c000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_tdp0_0.bin"
    busybox sleep 0.5

    ax_lookat 0x1209c000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_tdp0_1.bin"
    busybox sleep 0.5

    ax_lookat 0x1209d000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_tdp1_0.bin"
    busybox sleep 0.5

    ax_lookat 0x1209d000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_tdp1_1.bin"
    busybox sleep 0.5

    ax_lookat 0x120d4000 -t 0x4000 -f  "$log_path/ivps_reg/ivps_reg_vpp_0.bin"
    busybox sleep 0.5

    ax_lookat 0x120d4000 -t 0x4000 -f  "$log_path/ivps_reg/ivps_reg_vpp_1.bin"
    busybox sleep 0.5

    ax_lookat 0x10107000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_gdc_0.bin"
    busybox sleep 0.5

    ax_lookat 0x10107000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_gdc_1.bin"
    busybox sleep 0.5

    ax_lookat 0x10103000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_vgp_0.bin"
    busybox sleep 0.5

    ax_lookat 0x10103000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_vgp_1.bin"
    busybox sleep 0.5

    ax_lookat 0x10105000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_pyra_gen_0.bin"
    busybox sleep 0.5

    ax_lookat 0x10105000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_pyra_gen_1.bin"
    busybox sleep 0.5

    ax_lookat 0x10106000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_pyra_rcn_0.bin"
    busybox sleep 0.5

    ax_lookat 0x10106000 -t 0xfff -f  "$log_path/ivps_reg/ivps_reg_pyra_rcn_1.bin"
    busybox sleep 0.5
}

ax_venc_reg_proc()
{
    local tar_name="venc_reg_proc$compress_suffix"
    local log_name=$log_path/"venc_reg_proc.log"
    echo "**********************collect venc reg ************************"
    echo "##################################venc############################################" >> $log_name
    ax_lookat 4010000 -n 150 >> $log_name
    ax_lookat 4011000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################venc############################################" >> $log_name
    ax_lookat 4010000 -n 150 >> $log_name
    ax_lookat 4011000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_venc_reg_proc_650()
{
    local tar_name="venc_reg_proc$compress_suffix"
    local log_name=$log_path/"venc_reg_proc.log"
    echo "**********************collect venc reg ************************"
    echo "##################################venc vcmd######################################" >> $log_name
    ax_lookat 1a00c000 -n 30 >> $log_name
    ax_lookat 1a010000 -n 30 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################venc vcmd######################################" >> $log_name
    ax_lookat 1a00c000 -n 30 >> $log_name
    ax_lookat 1a010000 -n 30 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################venc############################################" >> $log_name
    ax_lookat 1a00d000 -n 600 >> $log_name
    ax_lookat 1a011000 -n 600 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################venc############################################" >> $log_name
    ax_lookat 1a00d000 -n 600 >> $log_name
    ax_lookat 1a011000 -n 600 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_jenc_reg_proc()
{
    local tar_name="jenc_reg_proc$compress_suffix"
    local log_name=$log_path/"jenc_reg_proc.log"
    echo "**********************collect jenc reg ************************"
    echo "##################################jenc############################################" >> $log_name
    ax_lookat 4000000 -n 150 >> $log_name
    ax_lookat 4001000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################jenc############################################" >> $log_name
    ax_lookat 4000000 -n 150 >> $log_name
    ax_lookat 4001000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_jenc_reg_proc_650()
{
    local tar_name="jenc_reg_proc$compress_suffix"
    local log_name=$log_path/"jenc_reg_proc.log"
    echo "**********************collect jenc reg ************************"
    echo "##################################jenc vcmd#######################################" >> $log_name
    ax_lookat 10130000 -n 30 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################jenc vcmd#######################################" >> $log_name
    ax_lookat 10130000 -n 30 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################jenc############################################" >> $log_name
    ax_lookat 10131000 -n 600 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################jenc############################################" >> $log_name
    ax_lookat 10131000 -n 600 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_vdec_reg_proc()
{
    local tar_name="vdec_reg_proc$compress_suffix"
    local log_name=$log_path/"vdec_reg_proc.log"
    echo "**********************collect vdec reg ************************"
    echo "##################################vdec############################################" >> $log_name
    ax_lookat 4020000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################vdec############################################" >> $log_name
    ax_lookat 4020000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_vdec_reg_proc_650()
{
    local tar_name="vdec_reg_proc$compress_suffix"
    local log_name=$log_path/"vdec_reg_proc.log"
    echo "**********************collect vdec reg ************************"
    echo "##################################vdec vcmd#######################################" >> $log_name
    ax_lookat 0x19010000 -n 27 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "##################################vdec vcmd#######################################" >> $log_name
    ax_lookat 0x19010000 -n 27 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################vdec fbc########################################" >> $log_name
    ax_lookat 0x19010400 -n 25 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "##################################vdec fbc########################################" >> $log_name
    ax_lookat 0x19010400 -n 25 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################vdec############################################" >> $log_name
    ax_lookat 0x19010800 -n 768 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "##################################vdec############################################" >> $log_name
    ax_lookat 0x19010800 -n 768 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_jdec_reg_proc_650()
{
    local tar_name="vdec_reg_proc$compress_suffix"
    local log_name=$log_path/"vdec_reg_proc.log"
    echo "**********************collect vdec reg ************************"
    echo "##################################vdec vcmd#######################################" >> $log_name
    ax_lookat 0x10120000 -n 27 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "##################################vdec vcmd#######################################" >> $log_name
    ax_lookat 0x10120000 -n 27 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "##################################vdec############################################" >> $log_name
    ax_lookat 0x10120800 -n 768 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "##################################vdec############################################" >> $log_name
    ax_lookat 0x10120800 -n 768 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_vo_reg_proc()
{
    local tar_name="vo_reg_proc$compress_suffix"
    local log_name=$log_path/"vo_reg_proc.log"
    echo "**********************collect vo reg ************************"
    mm_sys_glb=$(devmem 0x4430010)
    dpu0_bit=$((0x1<<7))
    dpu1_bit=$((0x1<<11))
    dpu0_flag=$(($mm_sys_glb&$dpu0_bit))
    dpu1_flag=$(($mm_sys_glb&$dpu1_bit))
    if [ $dpu0_flag -eq 0 ]; then
        echo "###################################vo_dpu0 disable###################################" >> $log_name
    else
        echo "###################################vo_dpu0###########################################" >> $log_name
        ax_lookat 4407000 -n 0x138 >> $log_name
        echo "" >> $log_name
        echo "" >> $log_name
        busybox sleep 0.5
        echo "###################################vo_dpu0###########################################" >> $log_name
        ax_lookat 4407000 -n 0x138 >> $log_name
        echo "" >> $log_name
        echo "" >> $log_name
        busybox sleep 0.5
    fi

    if [ $dpu1_flag -eq 0 ]; then
        echo "###################################vo_dpu1 disable###################################" >> $log_name
    else
        echo "###################################vo_dpu1###########################################" >> $log_name
        ax_lookat 4408000 -n 0x9C >> $log_name
        echo "" >> $log_name
        echo "" >> $log_name
        busybox sleep 0.5
        echo "###################################vo_dpu1###########################################" >> $log_name
        ax_lookat 4408000 -n 0x9C >> $log_name
        echo "" >> $log_name
        echo "" >> $log_name
        busybox sleep 0.5
    fi

    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_vo_reg_proc_650()
{
    local tar_name="vo_reg_proc$compress_suffix"
    local log_name=$log_path/"vo_reg_proc.log"
    echo "**********************collect vo reg ************************"
    echo "###################################vo_dpu0###########################################" >> $log_name
    ax_lookat 0x10111000 -n 0x348 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "###################################vo_dpu0###########################################" >> $log_name
    ax_lookat 0x10111000 -n 0x348 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "###################################vo_dpu1###########################################" >> $log_name
    ax_lookat 0x10112000 -n 0x348 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "###################################vo_dpu1###########################################" >> $log_name
    ax_lookat 0x10112000 -n 0x348 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "###################################vo_dpu2###########################################" >> $log_name
    ax_lookat 0x12006000 -n 0x348 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "###################################vo_dpu2###########################################" >> $log_name
    ax_lookat 0x12006000 -n 0x348 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "###################################mm_sys_glb###########################################" >> $log_name
    ax_lookat 0x10000000 -n 0x100 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.1
    echo "###################################mm_sys_glb###########################################" >> $log_name
    ax_lookat 0x10000000 -n 0x100 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_audio_reg_proc()
{
    local tar_name="audio_reg_proc$compress_suffix"
    local log_name=$log_path/"audio_reg_proc.log"
    echo "**********************collect audio reg ************************"
    echo "#################################audio_i2s_master#########################################" >> $log_name
    ax_lookat 0x6050000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#################################audio_i2s_master#########################################" >> $log_name
    ax_lookat 0x6050000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#################################audio_i2s_slave#########################################" >> $log_name
    ax_lookat 0x6051000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#################################audio_i2s_slave#########################################" >> $log_name
    ax_lookat 0x6051000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#################################audio_codec#########################################" >> $log_name
    ax_lookat 0x23f2000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#################################audio_codec#########################################" >> $log_name
    ax_lookat 0x23f2000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_audio_reg_proc_650()
{
    local tar_name="audio_reg_proc$compress_suffix"
    local log_name=$log_path/"audio_reg_proc.log"
    echo "**********************collect audio reg ************************"
    echo "#################################audio_codec#########################################" >> $log_name
    ax_lookat 0x2033000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    echo "#################################audio_codec#########################################" >> $log_name
    ax_lookat 0x2033000 -n 150 >> $log_name
    echo "" >> $log_name
    echo "" >> $log_name
    busybox sleep 0.5
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_dmeg_proc()
{
    local tar_name="dmesg_proc$compress_suffix"
    local log_name=$log_path/"dmesg_proc.log"
    echo "**********************collect dmesg***********************"
    echo "##################################dmesg############################################" >> $log_name
    axdmesg >> $log_name
    echo "##################################axdmesg############################################" >> $log_name
    dmesg >> $log_name
    if [ "$compress_cmd" != "" ]; then
        cd $log_path
        $compress_cmd $tar_name $(basename $log_name)
        rm $log_name
        cd - > /dev/null
    fi
}

ax_log_collecttion()
{
    ax_base_proc
    ax_pool_proc
    ax_isp_proc
    ax_vin_proc
    ax_venc_proc
    ax_jenc_proc
    ax_vdec_proc
    ax_vo_proc
    if grep -q "650" /proc/ax_proc/chip_type; then
        ax_ivps_proc_650
    else
        ax_ivps_proc
    fi

    ax_analyzer_proc
    ax_reg_proc
    if grep -q "650" /proc/ax_proc/chip_type; then
        ax_ivps_reg_proc_650
        ax_venc_reg_proc_650
        ax_jenc_reg_proc_650
        ax_vdec_reg_proc_650
        ax_jdec_reg_proc_650
        ax_vo_reg_proc_650
        ax_audio_reg_proc_650
    else
        ax_ivps_reg_proc
        ax_venc_reg_proc
        ax_jenc_reg_proc
        ax_vdec_reg_proc
        ax_vo_reg_proc
        ax_audio_reg_proc
    fi
    ax_dmeg_proc
}

isp_log_path()
{
    if [ "$1" != "" ]; then
        user_set_path=$1
    fi
        log_path=$user_set_path/$log_dir_name
}

isp_clean_log()
{
    local path=$log_path
    echo "clean log start"
    if [ "$1" != "" ]; then
        path=$1
    fi

    if [ -d $path/$log_dir_name ]; then
        rm -rf $path/$log_dir_name
    fi

    if [ -e $path/$log_compress_name.tar.gz ]; then
        rm  -f $path/$log_compress_name.tar.gz
    fi

    if [ -e $path/$log_compress_name.tar ]; then
        rm  -f $path/$log_compress_name.tar
    fi

    echo "clean log finish"
    exit 1
}

usage()
{
    echo "Usage: isp_log_collect.sh"
    echo "-h, --help | Show this help message and exit"
    echo "-p, --path | set log path"
    echo "-c, --clean | clear the log"
    echo ""
    exit 1
}

while [ True ]; do
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    usage
    shift 1
elif [[ "$1" == "--path" || "$1" == "-p"  || "$1" == "" ]]; then
    isp_log_path $2
    shift 2
    break
elif [[ "$1" == "-c" || "$1" == "--clean" ]]; then
    isp_clean_log $2
    shift 2
else
    usage
fi
done

echo "log collect start"
mkdir $log_path
chmod 777 $log_path

#判断系统内是否自带ax_analyzer
if which ax_analyzer > /dev/null; then
    ax_analyzer_path=ax_analyzer
else
    ax_analyzer_path=./ax_analyzer
fi

#判断系统内是否有打包压缩命令
if which tar > /dev/null && which gzip > /dev/null; then
    compress_cmd="tar -czf"
    compress_suffix=".tar.gz"
elif which tar > /dev/null; then
    compress_cmd="tar -cf"
    compress_suffix=".tar"
else
    compress_cmd=""
fi
#日志收集
ax_log_collecttion

cd $user_set_path
#打包压缩
if [ "$compress_cmd" != "" ]; then
    $compress_cmd $log_compress_name.tar.gz $log_dir_name
fi

echo "log collect finish"
