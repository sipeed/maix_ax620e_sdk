function load_drv()
{
    #echo "run auto_load_all_drv.sh start "
    #step 1 insmod common ko (depends no ko)
    insmod /soc/ko/ax_sys.ko &
    insmod /soc/ko/ax_cmm.ko cmmpool=anonymous,0,0x80000000,2047M &
    insmod /soc/ko/ax_base.ko &
    insmod /soc/ko/ax_mipi_rx.ko &
    wait
    #step 2 insmod second ko (depends cmm/sys/base)
    insmod /soc/ko/ax_npu.ko &
    # insmod /soc/ko/ax_fb.ko &
    insmod /soc/ko/ax_pool.ko
    #step 3 insmod second ko (depends cmm/sys/base/pool)
    insmod /soc/ko/ax_ivps.ko &
    insmod /soc/ko/ax_venc.ko &
    insmod /soc/ko/ax_jenc.ko &
    insmod /soc/ko/ax_audio.ko &
    wait
    #step 4 insmod last ko (depends cmm/sys/base/pool/ivps)
    insmod /soc/ko/ax_vo.ko &
    insmod /soc/ko/ax_tdp.ko &
    insmod /soc/ko/ax_vpp.ko
    #depends vpp
    insmod /soc/ko/ax_gdc.ko
    #depends gdc/vpp
    insmod /soc/ko/ax_proton.ko
    wait
    #echo "run auto_load_all_drv.sh end "
}

function remove_drv()
{
    rmmod ax_proton
    rmmod ax_gdc
    rmmod ax_vpp
    rmmod ax_tdp
    #rmmod ax_fb
    rmmod ax_vo
    rmmod ax_audio
    rmmod ax_jenc
    rmmod ax_venc
    rmmod ax_ivps
    rmmod ax_pool
    rmmod ax_npu
    rmmod ax_mipi_rx
    rmmod ax_base
    rmmod ax_cmm
    rmmod ax_sys
}

function remove_drv()
{
    if [ "$mode" == "-i" ]; then
        load_drv
    elif [ "$mode" == "-r" ]; then
        remove_drv
    else
        echo "[error] Invalid param, please use the following parameters"
        echo "-i:  insmod"
        echo "-r:  rmmod"
    fi
}

auto_drv
