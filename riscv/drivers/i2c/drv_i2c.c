/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "soc.h"
#include "drv_i2c.h"
#include "drv_i2c_reg.h"
#include "ax_common.h"

static uint32_t i2c_base_addr[i2c_channel_max] = {
    I2C0_BASE_ADDR,
    I2C1_BASE_ADDR,
    I2C2_BASE_ADDR,
    I2C3_BASE_ADDR,
    I2C4_BASE_ADDR,
    I2C5_BASE_ADDR,
    I2C6_BASE_ADDR,
    I2C7_BASE_ADDR
};

static reg_info_t i2c_pclk_info[i2c_channel_max] = {
    {1 << BIT_17, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_18, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_19, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_20, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_21, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_22, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_23, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_24, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET}
};

static reg_info_t i2c_clk_info[i2c_channel_max] = {
    {1 << BIT_8, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_9, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_10, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_11, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_12, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_13, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_14, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_15, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET}
};

static void e907_i2c_clk_init(i2c_channel_e channel)
{
    /* enable clk_i2c */
    uint32_t clk_i2c = ax_readl(PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_0);
    if ((clk_i2c & (1 << BIT_2)) == 0) {
        ax_writel(1 << BIT_2, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_0_SET);
    }
    /* enable i2c pclk */
    ax_writel(i2c_pclk_info[channel].reg_val, i2c_pclk_info[channel].reg_addr);
    /* enable i2c clk */
    ax_writel(i2c_clk_info[channel].reg_val, i2c_clk_info[channel].reg_addr);
}

__inline static rt_uint32_t i2c_read_field(uint32_t BASE, rt_uint32_t pack_data)
{
    rt_uint32_t ret;
    rt_uint32_t shift, len;

    rt_uint32_t addr  = ((pack_data >> 12) & 0xFFFFF) | BASE;
    shift = (pack_data >> 6) & 0x3F;
    len   = (pack_data >> 0) & 0x3F;

    ret = ax_readl(addr);
    ret = (ret >> shift) & (((1 << len) - 1));
    return ret;
}

__inline static void i2c_write_field(uint32_t BASE, rt_uint32_t pack_data, rt_uint32_t value)
{
    rt_uint32_t ret;
    rt_uint32_t shift, len;

    rt_uint32_t addr  = ((pack_data >> 12) & 0xFFFFF) | BASE;
    shift = (pack_data >> 6) & 0x3F;
    len   = (pack_data >> 0) & 0x3F;

    ret = ax_readl(addr);
    ret = (ret & (~(((1 << len) - 1) << shift))) | ((value & ((1 << len) - 1)) << shift);
    ax_writel(ret, addr);
}

int32_t i2c_wrtie_reg(struct rt_i2c_bus_device *i2c_dev,
                      rt_uint8_t addr,
                      rt_uint32_t reg,
                      rt_uint8_t* src,
                      rt_uint32_t len,
                      rt_uint32_t is_reg_addr_16bits)
{
    if(addr >= 0x80){
        AX_LOG_ERROR("slave addr is invalid");
        return -1;
    }
    struct rt_i2c_msg msgs;
    rt_int32_t i = 0;
    rt_int32_t delt = 0;
    rt_uint8_t buf[128] = {0};

    if(is_reg_addr_16bits) {
        buf[0] = (reg >> 8) & 0xff;
        buf[1] = reg & 0xff;
        delt = 2;
    } else {
        buf[0] = reg & 0xff;
        delt = 1;
    }
    for(i = 0; i < len; i++) {
        buf[i + delt] = src[i];
    }
    msgs.addr = (rt_uint16_t)addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = len + delt;

    if(rt_i2c_transfer(i2c_dev, &msgs, 1) != 1) {
        AX_LOG_ERROR("rt_i2c_transfer fail");
        return -2;
    }

    return 0;
}

int32_t i2c_read_reg(struct rt_i2c_bus_device *i2c_dev,
                     rt_uint8_t addr,
                     rt_uint32_t reg,
                     rt_uint8_t* dst,
                     rt_uint32_t len,
                     rt_uint32_t is_reg_addr_16bits)
{
    if(addr >= 0x80){
        AX_LOG_ERROR("slave addr is invalid!");
        return -1;
    }
    struct rt_i2c_msg msgs;
    rt_uint8_t buf[2] = {0};
    rt_int32_t delt = 0;

    if(is_reg_addr_16bits) {
        buf[0] = (reg >> 8) & 0xff;
        buf[1] = reg & 0xff;
        delt = 2;
    } else {
        buf[0] = reg & 0xff;
        delt = 1;
    }
    msgs.addr = (rt_uint16_t)addr;
    msgs.flags = RT_I2C_WR | RT_I2C_NO_STOP;
    msgs.buf = buf;
    msgs.len = delt;

    if(rt_i2c_transfer(i2c_dev, &msgs, 1) != 1) {
        AX_LOG_ERROR("rt_i2c_transfer fail");
        return -2;
    }

    msgs.addr = (rt_uint16_t)addr;
    msgs.flags = RT_I2C_RD;
    msgs.buf = dst;
    msgs.len = len;

    if(rt_i2c_transfer(i2c_dev, &msgs, 1) != 1) {
        AX_LOG_ERROR("rt_i2c_transfer fail");
        return -3;
    }

    return 0;
}

static rt_int32_t poll_tx_done(uint32_t i2c_base_addr)
{
    rt_int32_t ret = 0;
    rt_uint32_t status_tfe = 0;
    rt_uint32_t cnt = 0;

    status_tfe = i2c_read_field(i2c_base_addr, I2C_IC_STATUS_TFE);
    while (!status_tfe) {
        if (cnt > 500) {
            ret = -1;
            return ret;
        }
        ax_udelay(2);
        status_tfe = i2c_read_field(i2c_base_addr, I2C_IC_STATUS_TFE);
        cnt++;
    }

    return ret;
}

static rt_int32_t poll_rx_done(uint32_t i2c_base_addr)
{
    rt_int32_t ret = 0;
    rt_uint32_t status_rfne = 0;
    rt_uint32_t cnt = 0;

    status_rfne = i2c_read_field(i2c_base_addr, I2C_IC_STATUS_RFNE);
    while (!status_rfne) {
        if (cnt > 500) {
            ret = -1;
            return ret;
        }
        ax_udelay(2);
        status_rfne = i2c_read_field(i2c_base_addr, I2C_IC_STATUS_RFNE);
        cnt++;
    }

    return ret;
}

static int synopsys_i2c_master_tx(struct rt_i2c_bus_device *bus,
                                  uint8_t addr,
                                  uint8_t *src,
                                  uint16_t len,
                                  uint32_t restart)
{
    int last;
    int ret = 0;
    uint32_t data = 0;

    if(addr >= 0x80 || len == 0 || bus == RT_NULL) {
        AX_LOG_ERROR("param is invalid!");
        return -RT_EINVAL;
    }

    uint32_t i2c_base = (uint32_t)bus->priv;
    i2c_write_field(i2c_base, I2C_IC_ENABLE, 0);
    i2c_write_field(i2c_base, I2C_IC_TAR_IC_TAR, addr & 0xff);//set slave addr
    i2c_write_field(i2c_base, I2C_IC_ENABLE, 1);
    for(int i = 0; i < len; i++) {
        last = (i == len - 1) ? 1 : 0;
        data = src[i] | ((last & !restart) << I2C_IC_DATA_CMD_STOP_SHIFT)
                      | ((last & restart) << I2C_IC_DATA_CMD_RESTART_SHIFT);
        i2c_write_field(i2c_base, I2C_IC_DATA_CMD_DAT, data);
        ret = poll_tx_done(i2c_base);
        if (ret != 0) {
            AX_LOG_ERROR("i2c write data[0x%x] not done!", data);
            return -RT_ETIMEOUT;
        }
    }

    return RT_EOK;
}

static int synopsys_i2c_master_rx(struct rt_i2c_bus_device *bus,
                                  uint8_t addr,
                                  uint8_t *dst,
                                  uint16_t len,
                                  uint32_t restart)
{
    rt_int32_t i = 0;
    rt_uint32_t last;
    rt_int32_t ret = 0;
    rt_uint32_t val = 0;

    if(addr >= 0x80 || len == 0 || bus == RT_NULL) {
        AX_LOG_ERROR("param is invalid!");
        return -RT_EINVAL;
    }

    uint32_t i2c_base = (uint32_t)bus->priv;

    i2c_write_field(i2c_base, I2C_IC_ENABLE, 0);
    i2c_write_field(i2c_base, I2C_IC_TAR_IC_TAR, addr & 0xff);//set slave addr
    i2c_write_field(i2c_base, I2C_IC_ENABLE, 1);
    for(i = 0; i < len; i++) {
        last = (i == len - 1)? 1:0;
        val = ((last & !restart) << I2C_IC_DATA_CMD_STOP_SHIFT)
              | ((last & restart) << I2C_IC_DATA_CMD_RESTART_SHIFT)
              | I2C_IC_DATA_CMD_CMD_BITS;
        i2c_write_field(i2c_base, I2C_IC_DATA_CMD_DAT, val);
        ret = poll_tx_done(i2c_base);
        if (ret != 0) {
            AX_LOG_ERROR("i2c write data[0x%x] not done!", val);
            return -RT_ETIMEOUT;
        }
        ret = poll_rx_done(i2c_base);
        if (ret != 0) {
            AX_LOG_ERROR("i2c read data[0x%x] not done!\n", val);
            return -RT_ETIMEOUT;
        }
        dst[i] = (i2c_read_field(i2c_base, I2C_IC_DATA_CMD_DAT) & 0xff);
    }

    return RT_EOK;
}

static rt_size_t synopsys_i2c_master_xfer(struct rt_i2c_bus_device *bus,
                                          struct rt_i2c_msg msgs[],
                                          rt_uint32_t num)
{
    struct rt_i2c_msg *msg = RT_NULL;
    rt_int32_t i = 0;
    rt_uint32_t restart = 0;

    for(i = 0; i < num; i++){
        msg = &msgs[i];
        if (msg->flags & RT_I2C_ADDR_10BIT || msg->flags & RT_I2C_NO_START
              || msg->flags & RT_I2C_IGNORE_NACK || msg->flags & RT_I2C_NO_READ_ACK)
        {
            AX_LOG_ERROR("Not support flag");
            return i;
        }
        if (msg->flags & RT_I2C_NO_STOP)
        {
            restart = 1;
        }
        if(msg->flags & RT_I2C_RD) {
            synopsys_i2c_master_rx(bus, msg->addr, msg->buf, msg->len, restart);
        } else {
            synopsys_i2c_master_tx(bus, msg->addr, msg->buf, msg->len, restart);
        }
    }

    return i;
}

static struct rt_i2c_bus_device_ops i2c_bus_ops = {
    .master_xfer = synopsys_i2c_master_xfer,
    .slave_xfer = RT_NULL,
    .i2c_bus_control = RT_NULL
};

static rt_err_t i2c_hw_init(i2c_channel_e channel, i2c_freq_e freq)
{
    uint32_t i2c_base = i2c_base_addr[channel];

    i2c_write_field(i2c_base, I2C_IC_ENABLE, 0);
    i2c_write_field(i2c_base, I2C_IC_CON_IC_MASTER_MODE, 1);
    i2c_write_field(i2c_base, I2C_IC_CON_IC_SLAVE_DISABLE, 1);
    i2c_write_field(i2c_base, I2C_IC_CON_IC_10BITADDR_MASTER, 0);
    i2c_write_field(i2c_base, I2C_IC_CON_IC_10BITADDR_SLAVE, 0);
    i2c_write_field(i2c_base, I2C_IC_CON_IC_RESTART_EN, 1);
    i2c_write_field(i2c_base, I2C_IC_TAR_GC_OR_START, 0);
    i2c_write_field(i2c_base, I2C_IC_INTR_MASK, 0);

    /*
        cnt = i2c_base_clk / i2c_freq
        lcnt = cnt * 52%
        hcnt = cnt * 48%
        tx_hold = (cycle_duration us) / 10
    */
    uint32_t cnt = I2C_BASE_CLK / freq;
    uint32_t lcnt = (cnt * 52) / 100;
    uint32_t hcnt = cnt - lcnt;
    i2c_write_field(i2c_base, I2C_IC_CON_IC_SPEED, 1);
    i2c_write_field(i2c_base, I2C_IC_SS_SCL_LCNT, lcnt);
    i2c_write_field(i2c_base, I2C_IC_SS_SCL_HCNT, hcnt);
    i2c_write_field(i2c_base, I2C_IC_SDA_TX_HOLD, 0x1);
    i2c_write_field(i2c_base, I2C_IC_SDA_RX_HOLD, 0);

    i2c_write_field(i2c_base, I2C_IC_ENABLE, 1);

    return 0;
}

int i2c_init(i2c_channel_e channel, const char *name, i2c_freq_e freq)
{
    if (channel < 0 || channel >= i2c_channel_max || name == RT_NULL) {
        AX_LOG_ERROR("params error");
        return -1;
    }

    e907_i2c_clk_init(channel);
    i2c_hw_init(channel, freq);

    struct rt_i2c_bus_device *bus = rt_malloc(sizeof(struct rt_i2c_bus_device));
    bus->ops = &i2c_bus_ops;
    bus->priv = (void*)i2c_base_addr[channel];
    rt_err_t ret = rt_i2c_bus_device_register(bus, name);
    if(ret != RT_EOK) {
        AX_LOG_ERROR("%s register failed ret=%d", name, ret);
        return -1;
    }

    return 0;
}
