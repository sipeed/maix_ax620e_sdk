#include <rtdevice.h>
#include "ax_common.h"

#ifndef SYS_DRAM_BASE
#define SYS_DRAM_BASE   0x40000000
#endif

#ifndef SYS_DRAM_SIZE
#define SYS_DRAM_SIZE   0x10000000
#endif

#ifndef RAMDISK_END_ADDR
#define RAMDISK_END_ADDR    0x47d00000
#endif

#define DBG_MONITOR_CLK_BASE        0x2100e0
#define DBG_MONITOR_ADDR_BASE       0x1810000
#define DBG_MONITOR_PORTX_OFFSET    0x1000
#define PORT_ADDR_START_H_OFFSET    0x64
#define PORT_ADDR_END_H_OFFSET      0x68
#define PORT_ADDR_START_L_OFFSET    0x6c
#define PORT_ADDR_END_L_OFFSET      0x70
#define PORT_AWADDR_START_H_OFFSET  0x4
#define PORT_AWADDR_END_H_OFFSET    0x8
#define PORT_AWADDR_START_L_OFFSET  0xc
#define PORT_AWADDR_END_L_OFFSET    0x10
#define PORT_ARADDR_START_H_OFFSET  0x30
#define PORT_ARADDR_END_H_OFFSET    0x34
#define PORT_ARADDR_START_L_OFFSET  0x38
#define PORT_ARADDR_END_L_OFFSET    0x3c
#define PORT_SET_MASK_OFFSET        0x0
#define PORT_AWID_MASK_OFFSET       0xb4
#define PORT_ARID_MASK_OFFSET       0xbc
#define PORT_MNR_EN_OFFSET          0x74
#define PORT_INI_MASK_OFFSET        0x78
#define PORT_INIT_MASK          (0x3)
#define PORT_SET_MASK           (0x1) //bit[0]: write; bit[12]: read
#define PORT_SET_AWID_MASK      (0x0)
#define PORT_SET_ARID_MASK      (0x0)
#define PORT_MNR_EN             (0x1)
#define DBG_MONITOR_PORT_CLK    (0x1f)
#define DBG_MONITOR_PORT_NUM    (5)

static void clk_eb_set(void)
{
   ax_writel(DBG_MONITOR_PORT_CLK, DBG_MONITOR_CLK_BASE);
}

static void bus_monitor_register_writel(unsigned int val, int port, unsigned int offset)
{
    ax_writel(val, DBG_MONITOR_ADDR_BASE + port * DBG_MONITOR_PORTX_OFFSET + offset);
}

static int bus_monitor_config(void)
{
    int port;

    clk_eb_set();

    for (port = 0; port < DBG_MONITOR_PORT_NUM; port++) {
        bus_monitor_register_writel(PORT_INIT_MASK, port, PORT_INI_MASK_OFFSET);
        bus_monitor_register_writel(0, port, PORT_ADDR_START_H_OFFSET);
        bus_monitor_register_writel(0, port, PORT_ADDR_END_H_OFFSET);
        bus_monitor_register_writel(0, port, PORT_ADDR_START_L_OFFSET);
        bus_monitor_register_writel((SYS_DRAM_BASE + SYS_DRAM_SIZE) - SYS_DRAM_BASE, port, PORT_ADDR_END_L_OFFSET);
        bus_monitor_register_writel(0, port, PORT_AWADDR_START_H_OFFSET);
        bus_monitor_register_writel(0, port, PORT_AWADDR_END_H_OFFSET);
        bus_monitor_register_writel(RAMDISK_DDR_BASE - SYS_DRAM_BASE, port, PORT_AWADDR_START_L_OFFSET);
        bus_monitor_register_writel(RAMDISK_END_ADDR - SYS_DRAM_BASE, port, PORT_AWADDR_END_L_OFFSET);
        bus_monitor_register_writel(0, port, PORT_ARADDR_START_H_OFFSET);
        bus_monitor_register_writel(0, port, PORT_ARADDR_END_H_OFFSET);
        bus_monitor_register_writel(0, port, PORT_ARADDR_START_L_OFFSET);
        bus_monitor_register_writel(0, port, PORT_ARADDR_END_L_OFFSET);
        bus_monitor_register_writel(PORT_SET_MASK, port, PORT_SET_MASK_OFFSET);
        bus_monitor_register_writel(PORT_SET_AWID_MASK, port, PORT_AWID_MASK_OFFSET);
        bus_monitor_register_writel(PORT_SET_ARID_MASK, port, PORT_ARID_MASK_OFFSET);
        bus_monitor_register_writel(PORT_MNR_EN, port, PORT_MNR_EN_OFFSET);
    }
    return 0;
}
INIT_BOARD_EXPORT(bus_monitor_config);