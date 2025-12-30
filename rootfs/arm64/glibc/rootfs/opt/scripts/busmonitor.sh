#!/bin/sh
set -e

#monitor addr
g_addr_start_h=0x0
g_addr_end_h=0x0
g_addr_start_l=0x0
g_addr_end_l=0x0

#monitor channels
g_monitor_ports=all #dbg_mnr_port0~dbg_mnr_port4

#inside r(read) w(write) access
g_monitor_read=yes
g_monitor_write=yes
#outside r(read) w(write) access
g_monitor_outside_read=no
g_monitor_outside_write=no

monitor_dbg_mnr_port0=no
dbg_mnr_port0_base=0x1810000
monitor_dbg_mnr_port1=no
dbg_mnr_port1_base=0x1811000
monitor_dbg_mnr_port2=no
dbg_mnr_port2_base=0x1812000
monitor_dbg_mnr_port3=no
dbg_mnr_port3_base=0x1813000
monitor_dbg_mnr_port4=no
dbg_mnr_port4_base=0x1814000
monitor_npu_dbg_mnr=no
npu_dbg_mnr_base=0x1818000
monitor_dbg_mnr_ocm=no
dbg_mnr_ocm_base=0x1819000
monitor_dbg_mnr_cpu=no
dbg_mnr_cpu_base=0x1820000
monitor_dbg_mnr_riscv=no
dbg_mnr_riscv_base=0x1830000

# 允许访问的地址寄存器偏移量
int_mask=0x78
addr_start_h=0x64
addr_end_h=0x68
addr_start_l=0x6c
addr_end_l=0x70
g_outside_addr_start_l=0x0
g_outside_addr_end_l=0x0

# 写地址地址范围
waddr_start_h=0x4
waddr_end_h=0x8
waddr_start_l=0xC
waddr_end_l=0x10
g_waddr_start_l=0x0
g_waddr_end_l=0x0

# 读地址范围
raddr_start_h=0x30
raddr_end_h=0x34
raddr_start_l=0x38
raddr_end_l=0x3C
g_raddr_start_l=0x0
g_raddr_end_l=0x0

# trigger type
set_mask=0x0
awid_mask=0xb4
arid_mask=0xbc
mnr_en=0x74
int_stat=0x80

# clear int
int_clr=0x7c

g_set_mask_regval=0x0
g_awid_mask_regval=0x0
g_arid_mask_regval=0x0
g_mnr_en_regval=0x1
g_int_mask_regval=0x0

AX_LOOKAT=ax_lookat
display_reg_nums=64

function enable_eb_mnr0_4() {
    $AX_LOOKAT 0x2100E0 -s 0x1f
}

function enable_eb_npu_dbg_mnr() {
    $AX_LOOKAT 0x3890004 -s 0x4
    $AX_LOOKAT 0x3890008 -s 0x200
    $AX_LOOKAT 0x1800030 -s 0x2
    $AX_LOOKAT 0x1800030 -s 0x0
    $AX_LOOKAT 0x180001C -s 0x1
    $AX_LOOKAT 0x180001C -s 0x0
}

function enable_eb_dbg_mnr_ocm() {
    $AX_LOOKAT 0x3890004 -s 0x4
    $AX_LOOKAT 0x3890008 -s 0x200
    $AX_LOOKAT 0x1800030 -s 0x2
    $AX_LOOKAT 0x1800030 -s 0x0
    $AX_LOOKAT 0x180001C -s 0x1
    $AX_LOOKAT 0x180001C -s 0x0
}

function enable_eb_dbg_mnr_cpu() {
    $AX_LOOKAT 0x1901008 -s 0x201
}

function enable_eb_dbg_mnr_riscv() {
    $AX_LOOKAT 0x1800004 -s 0x1
}

function set_int_mask() {
    reg_base=$1
    int_val=$2
    reg_int_mask=0x$(printf "%x" $(($reg_base + $int_mask)))
    $AX_LOOKAT $reg_int_mask -s $int_val
}

function set_outside_addres_range() {
    reg_base=$1
    start_l=$2
    end_l=$3
    reg_start_h=0x$(printf "%x" $(($reg_base + $addr_start_h)))
    reg_end_h=0x$(printf "%x" $(($reg_base + $addr_end_h)))
    reg_start_l=0x$(printf "%x" $(($reg_base + $addr_start_l)))
    reg_end_l=0x$(printf "%x" $(($reg_base + $addr_end_l)))
    $AX_LOOKAT $reg_start_h -s 0x0      # set_addr_start_h
    $AX_LOOKAT $reg_end_h   -s 0x0      # set_addr_end_h
    $AX_LOOKAT $reg_start_l -s $start_l # set_addr_start_l
    $AX_LOOKAT $reg_end_l   -s $end_l   # set_addr_end_l
}

function set_waddres_range() {
    reg_base=$1
    start_l=$2
    end_l=$3
    reg_start_h=0x$(printf "%x" $(($reg_base + $waddr_start_h)))
    reg_end_h=0x$(printf "%x" $(($reg_base + $waddr_end_h)))
    reg_start_l=0x$(printf "%x" $(($reg_base + $waddr_start_l)))
    reg_end_l=0x$(printf "%x" $(($reg_base + $waddr_end_l)))
    $AX_LOOKAT $reg_start_h -s 0x0       # set_addr_start_h
    $AX_LOOKAT $reg_end_h   -s 0x0       # set_addr_end_h
    $AX_LOOKAT $reg_start_l -s $start_l  # set_addr_start_l
    $AX_LOOKAT $reg_end_l   -s $end_l    #set_addr_end_l
}

function set_raddres_range() {
    reg_base=$1
    start_l=$2
    end_l=$3
    reg_start_h=0x$(printf "%x" $(($reg_base + $raddr_start_h)))
    reg_end_h=0x$(printf "%x" $(($reg_base + $raddr_end_h)))
    reg_start_l=0x$(printf "%x" $(($reg_base + $raddr_start_l)))
    reg_end_l=0x$(printf "%x" $(($reg_base + $raddr_end_l)))
    $AX_LOOKAT $reg_start_h -s 0x0       # set_addr_start_h
    $AX_LOOKAT $reg_end_h   -s 0x0       # set_addr_end_h
    $AX_LOOKAT $reg_start_l -s $start_l  # set_addr_start_l
    $AX_LOOKAT $reg_end_l   -s $end_l    #set_addr_end_l
}

function set_mask() {
    reg_base=$1
    mask_val=$2
    reg_set_mask=0x$(printf "%x" $(($reg_base + $set_mask)))
    $AX_LOOKAT $reg_set_mask -s $mask_val  # set mask
}

function set_awid_mask() {
    reg_base=$1
    mask_val=$2
    reg_awid_mask=0x$(printf "%x" $(($reg_base + $awid_mask)))
    $AX_LOOKAT $reg_awid_mask -s $mask_val # write sub id mask
}

function set_arid_mask() {
    reg_base=$1
    mask_val=$2
    reg_arid_mask=0x$(printf "%x" $(($reg_base + $arid_mask)))
    $AX_LOOKAT $reg_arid_mask -s $mask_val # read sub id mask
}

function set_mr_en() {
    reg_base=$1
    reg_val=$2
    reg_mr_en=0x$(printf "%x" $(($reg_base + $mnr_en)))
    $AX_LOOKAT $reg_mr_en -s $reg_val # set mnr_en
}

function clear_int() {
    reg_base=$1
    reg_val=$2
    reg_int_clr=0x$(printf "%x" $(($reg_base + $int_clr)))
    $AX_LOOKAT $reg_int_clr -s 0xff # clear int
}

function config_dbg_mnr_port() {
    reg_port_base=$1
    set_int_mask $reg_port_base $g_int_mask_regval
    clear_int $reg_port_base
    if [[ "$g_monitor_outside_read" == "yes" || "$g_monitor_outside_write" == "yes" ]]; then
        set_outside_addres_range $reg_port_base $g_addr_start_l $g_addr_end_l
    else
        set_outside_addres_range $reg_port_base 0x0 0x0
    fi
    if [ "$g_monitor_write" == "yes" ]; then
        set_waddres_range        $reg_port_base $g_addr_start_l $g_addr_end_l
    else
        set_waddres_range        $reg_port_base 0x0 0x0
    fi
    if [ "$g_monitor_read" == "yes" ]; then
        set_raddres_range        $reg_port_base $g_addr_start_l $g_addr_end_l
    else
        set_raddres_range        $reg_port_base 0x0 0x0
    fi
    set_mask      $reg_port_base $g_set_mask_regval  # set_mask
    set_awid_mask $reg_port_base $g_awid_mask_regval # awid_mask
    set_arid_mask $reg_port_base $g_awid_mask_regval # arid_mask
    set_mr_en     $reg_port_base $g_mnr_en_regval    # mnr_en
}

function config_dbg_mnr_port0() {
    echo "==> monitor dbg_mnr_port0"
    enable_eb_mnr0_4
    config_dbg_mnr_port $dbg_mnr_port0_base
}

function config_dbg_mnr_port1() {
    echo "==> monitor dbg_mnr_port1"
    enable_eb_mnr0_4
    config_dbg_mnr_port $dbg_mnr_port1_base
}

function config_dbg_mnr_port2() {
    echo "==> monitor dbg_mnr_port2"
    enable_eb_mnr0_4
    config_dbg_mnr_port $dbg_mnr_port2_base
}

function config_dbg_mnr_port3() {
    echo "==> monitor dbg_mnr_port3"
    enable_eb_mnr0_4
    config_dbg_mnr_port $dbg_mnr_port3_base
}

function config_dbg_mnr_port4() {
    echo "==> monitor dbg_mnr_port4"
    enable_eb_mnr0_4
    config_dbg_mnr_port $dbg_mnr_port4_base
}

function config_npu_dbg_mnr() {
    echo "==> monitor npu_dbg_mnr"
    enable_eb_npu_dbg_mnr
    config_dbg_mnr_port $npu_dbg_mnr_base
}

function config_dbg_mnr_ocm() {
    echo "==> monitor dbg_mnr_ocm"
    enable_eb_dbg_mnr_ocm
    config_dbg_mnr_port $dbg_mnr_ocm_base
}

function config_dbg_mnr_cpu() {
    echo "==> monitor dbg_mnr_cpu"
    enable_eb_dbg_mnr_cpu
    config_dbg_mnr_port $dbg_mnr_cpu_base
}

function config_dbg_mnr_riscv() {
    echo "==> monitor dbg_mnr_riscv"
    enable_eb_dbg_mnr_riscv
    config_dbg_mnr_port $dbg_mnr_riscv_base
}

function usage(){
    echo "Usage: busmonitor.sh"
    echo "-h, --help | Show this help message and exit"
    echo ""
    echo "-c,--clear | clear channel 0-8's int status register"
    echo "  0: clear dbg_mnr_port0's int status register"
    echo "  1: clear dbg_mnr_port1's int status register"
    echo "  2: clear dbg_mnr_port2's int status register"
    echo "  3: clear dbg_mnr_port3's int status register"
    echo "  4: clear dbg_mnr_port4's int status register"
    echo "  5: clear npu_dbg_mnr's int status register"
    echo "  6: clear dbg_mnr_ocm's int status register"
    echo "  7: clear dbg_mnr_cpu's int status register"
    echo "  8: clear dbg_mnr_riscv's int status register"
    echo "  all: clear dbg_mnr_port0~dbg_mnr_port4's int status register"
    echo ""
    echo "-d,--display | display channel 0-8's registers"
    echo "  0: display dbg_mnr_port0's registers"
    echo "  1: display dbg_mnr_port1's registers"
    echo "  2: display dbg_mnr_port2's registers"
    echo "  3: display dbg_mnr_port3's registers"
    echo "  4: display dbg_mnr_port4's registers"
    echo "  5: display npu_dbg_mnr's registers"
    echo "  6: display dbg_mnr_ocm's registers"
    echo "  7: display dbg_mnr_cpu's registers"
    echo "  8: display dbg_mnr_riscv's registers"
    echo "  all: display dbg_mnr_port0~dbg_mnr_port4's registers"
    echo ""
    echo "-p, --port | set monitor port 0-8"
    echo "  0: dbg_mnr_port0 monitor (cpu, riscv, emmc, axera_dma, axera_dma_cfg,axera_dma_per, sdio_mst, sd_mst, emac, gzipd, usb, dw_dma,ce) access DDR address space"
    echo "  1: dbg_mnr_port1 monitor (ISP) access DDR address space"
    echo "  2: dbg_mnr_port2 monitor (NPU) access DDR address space"
    echo "  3: dbg_mnr_port3 monitor (MM) access DDR address space"
    echo "  4: dbg_mnr_port4 monitor (venc, vdec, jenc) access DDR address space"
    echo "  5: npu_dbg_mnr == dbg_mnr_port2"
    echo "  6: dbg_mnr_ocm monitor (cpu, riscv, isp, mm, venc, jenc, vdec) access OCM address space"
    echo "  7: dbg_mnr_cpu monitor (cpu) access FULL SOC address space"
    echo "  8: dbg_mnr_riscv monitor (riscv) access FULL SOC address space"
    echo "  all: monitor dbg_mnr_port0~dbg_mnr_port4 (default)"
    echo ""
    echo "-s,--start | monitor start phyical address in HEX format"
    echo ""
    echo "-e,--end | monitor end phyical address in HEX format"
    echo ""
    echo "-t,--type | inside/outside read/write access"
    echo "  r: monitor inside read"
    echo "  w: monitor inside write"
    echo "  l: monitor outside read(load)"
    echo "  s: monitor outside write(store)"
    echo ""
    echo "Note:"
    echo "  address of dbg_mnr_port0-4: need -0x4000_0000(DDR start address)"
    echo ""
    echo "Examples:"
    echo "  example 1: monitor port0's read/write inside of phy address 0x4000_0000-0x47ff_ffff(128MB)"
    echo "    ./busmonitor.sh -p 0 -s 0x0 -e 0x7ffffff -t rw"
    echo "  example 2: monitor port0's read/write outside of phy address 0x4000_0000-0x47ff_ffff(128MB)"
    echo "    ./busmonitor.sh -p 0 -s 0x0 -e 0x7ffffff -t ls"
    exit 1
}

function display_dbg_mnr_port0() {
    echo "==> regs of dbg_mnr_port0"
    $AX_LOOKAT $dbg_mnr_port0_base -n $display_reg_nums
}

function display_dbg_mnr_port1() {
    echo "==> regs of dbg_mnr_port1"
    $AX_LOOKAT $dbg_mnr_port1_base -n $display_reg_nums
}

function display_dbg_mnr_port2() {
    echo "==> regs of dbg_mnr_port2"
    $AX_LOOKAT $dbg_mnr_port2_base -n $display_reg_nums
}

function display_dbg_mnr_port3() {
    echo "==> regs of dbg_mnr_port3"
    $AX_LOOKAT $dbg_mnr_port3_base -n $display_reg_nums
}

function display_dbg_mnr_port4() {
    echo "==> regs of dbg_mnr_port4"
    $AX_LOOKAT $dbg_mnr_port4_base -n $display_reg_nums
}

function display_npu_dbg_mnr() {
    echo "==> regs of npu_dbg_mnr"
    $AX_LOOKAT $npu_dbg_mnr_base -n $display_reg_nums
}

function display_dbg_mnr_ocm() {
    echo "==> regs of dbg_mnr_ocm"
    $AX_LOOKAT $dbg_mnr_ocm_base -n $display_reg_nums
}

function display_dbg_mnr_cpu() {
    echo "==> regs of dbg_mnr_cpu"
    $AX_LOOKAT $dbg_mnr_cpu_base -n $display_reg_nums
}

function display_dbg_mnr_riscv() {
    echo "==> regs of dbg_mnr_riscv"
    $AX_LOOKAT $dbg_mnr_riscv_base -n $display_reg_nums
}

function display_port_regs(){
    display_ports=$1
    if [ "$display_ports" == "all" ]; then
        display_dbg_mnr_port0
        display_dbg_mnr_port1
        display_dbg_mnr_port2
        display_dbg_mnr_port3
        display_dbg_mnr_port4
    elif [[ "$display_ports" =~ ^[0-8,-]+$ ]]; then
        for p in $(echo "$display_ports" | tr ',' ' '); do
            case $p in
                0)
                    display_dbg_mnr_port0
                    ;;
                1)
                    display_dbg_mnr_port1
                    ;;
                2)
                    display_dbg_mnr_port2
                    ;;
                3)
                    display_dbg_mnr_port3
                    ;;
                4)
                    display_dbg_mnr_port4
                    ;;
                5)
                    display_npu_dbg_mnr
                    ;;
                6)
                    display_dbg_mnr_ocm
                    ;;
                7)
                    display_dbg_mnr_cpu
                    ;;
                8)
                    display_dbg_mnr_riscv
                    ;;
            esac
        done
    else
        usage
    fi
    exit 1
}
function clear_port_int(){
    clear_ports=$1
    if [ "$clear_ports" == "all" ]; then
        clear_int $dbg_mnr_port0_base
        clear_int $dbg_mnr_port1_base
        clear_int $dbg_mnr_port2_base
        clear_int $dbg_mnr_port3_base
        clear_int $dbg_mnr_port4_base
    elif [[ "$clear_ports" =~ ^[0-8,-]+$ ]]; then
        for p in $(echo "$clear_ports" | tr ',' ' '); do
            case $p in
                0)
                    clear_int $dbg_mnr_port0_base
                    ;;
                1)
                    clear_int $dbg_mnr_port1_base
                    ;;
                2)
                    clear_int $dbg_mnr_port2_base
                    ;;
                3)
                    clear_int $dbg_mnr_port3_base
                    ;;
                4)
                    clear_int $dbg_mnr_port4_base
                    ;;
                5)
                    clear_int $npu_dbg_mnr_base
                    ;;
                6)
                    clear_int $dbg_mnr_ocm_base
                    ;;
                7)
                    clear_int $dbg_mnr_cpu_base
                    ;;
                8)
                    clear_int $dbg_mnr_riscv_base
                    ;;
            esac
        done
    else
        usage
    fi
    exit 1
}

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    usage
fi

if [[ "$1" == "-d" || "$1" == "--display" ]]; then
    display_port_regs $2
fi

if [[ "$1" == "-c" || "$1" == "--clear" ]]; then
    clear_port_int $2
fi


ARGS=`getopt -a -o e:p:s:t: --long end:,port:,start:,type: -- "$@"`
eval set -- "${ARGS}"
while :
do
    case $1 in
        -e|--end)
            g_addr_end_l=$2
            shift
            ;;
        -s|--start)
            g_addr_start_l=$2
            shift
            ;;
        -t|--type)
            g_monitor_outside_read=no
            g_monitor_outside_write=no
            g_monitor_read=no
            g_monitor_write=no
            monitor_type=$2
            i=0
            while [ $i -lt ${#monitor_type} ]
            do
                ch="${monitor_type:i:1}"
                case "$ch" in
                    l)
                        g_monitor_outside_read=yes
                        ;;
                    s)
                        g_monitor_outside_write=yes
                        ;;
                    r)
                        g_monitor_read=yes
                        ;;
                    w)
                        g_monitor_write=yes
                        ;;
                    *)
                        echo "Invalid type: $ch"
                        usage
                        ;;
                esac
                i=$((i+1))
            done
            shift
            ;;
        -p|--port)
            g_monitor_ports=$2
            shift
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "unsupported argument: $1"
            exit 1
            ;;
    esac
    shift
done

if [ "$g_monitor_read" == "yes" ]; then
    g_int_mask_regval=$((g_int_mask_regval | 0x1))    # bit0 地址通道中断
    g_set_mask_regval=$((g_set_mask_regval | 0x1000)) # bit12 araddr
fi
if [ "$g_monitor_write" == "yes" ]; then
    g_int_mask_regval=$((g_int_mask_regval | 0x1))    # bit0 地址通道中断
    g_set_mask_regval=$((g_set_mask_regval | 0x1))    # bit0 awaddr
fi

if [ "$g_monitor_outside_read" == "yes" ]; then
    g_int_mask_regval=$((g_int_mask_regval | 0x20))   # bit5 读地址非法中断
    g_set_mask_regval=$((g_set_mask_regval | 0x1000)) # bit12 araddr
fi

if [ "$g_monitor_outside_write" == "yes" ]; then
    g_int_mask_regval=$((g_int_mask_regval | 0x10))   # bit4 写地址非法中断
    g_set_mask_regval=$((g_set_mask_regval | 0x1))    # bit0 awaddr
fi

g_int_mask_regval=0x$(printf "%x" $g_int_mask_regval)
g_set_mask_regval=0x$(printf "%x" $g_set_mask_regval)

if [ "$g_monitor_ports" == "all" ]; then
    monitor_dbg_mnr_port0=yes
    monitor_dbg_mnr_port1=yes
    monitor_dbg_mnr_port2=yes
    monitor_dbg_mnr_port3=yes
    monitor_dbg_mnr_port4=yes
elif [[ "$g_monitor_ports" =~ ^[0-8,-]+$ ]]; then
    for p in $(echo "$g_monitor_ports" | tr ',' ' '); do
        case $p in
            0)
                monitor_dbg_mnr_port0=yes
                ;;
            1)
                monitor_dbg_mnr_port1=yes
                ;;
            2)
                monitor_dbg_mnr_port2=yes
                ;;
            3)
                monitor_dbg_mnr_port3=yes
                ;;
            4)
                monitor_dbg_mnr_port4=yes
                ;;
            5)
                monitor_npu_dbg_mnr=yes
                ;;
            6)
                monitor_dbg_mnr_ocm=yes
                ;;
            7)
                monitor_dbg_mnr_cpu=yes
                ;;
            8)
                monitor_dbg_mnr_riscv=yes
                ;;
        esac
    done
else
    usage
fi

if [ "$monitor_dbg_mnr_port0" == "yes" ]; then
    config_dbg_mnr_port0
fi

if [ "$monitor_dbg_mnr_port1" == "yes" ]; then
    config_dbg_mnr_port1
fi

if [ "$monitor_dbg_mnr_port2" == "yes" ]; then
    config_dbg_mnr_port2
fi

if [ "$monitor_dbg_mnr_port3" == "yes" ]; then
    config_dbg_mnr_port3
fi

if [ "$monitor_dbg_mnr_port4" == "yes" ]; then
    config_dbg_mnr_port4
fi

if [ "$monitor_npu_dbg_mnr" == "yes" ]; then
    config_npu_dbg_mnr
fi

if [ "$monitor_dbg_mnr_ocm" == "yes" ]; then
    config_dbg_mnr_ocm
fi

if [ "$monitor_dbg_mnr_cpu" == "yes" ]; then
    config_dbg_mnr_cpu
fi

if [ "$monitor_dbg_mnr_riscv" == "yes" ]; then
    config_dbg_mnr_riscv
fi

echo "Done!!!"
