/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _DRV_I2C_REG_H_
#define _DRV_I2C_REG_H_

#include <stdint.h>

#define I2C_BASE_CLK	208000000
#define I2C_CMD_UNINIT   (1)

#define I2C_PACK(addr,offset,len)                ((((uint32_t)(addr)&0xFFFFF)<<12)|(((uint32_t)(offset)&0x3F)<<6)|(((uint32_t)(len)&0x3F)<<0))

#define I2C_IC_CON_IC_MASTER_MODE                I2C_PACK(0x00, 0, 1)
#define I2C_IC_CON_IC_SPEED                      I2C_PACK(0x00, 1, 2)
#define I2C_IC_CON_IC_10BITADDR_SLAVE            I2C_PACK(0x00, 3, 1)
#define I2C_IC_CON_IC_10BITADDR_MASTER           I2C_PACK(0x00, 4, 1)
#define I2C_IC_CON_IC_RESTART_EN                 I2C_PACK(0x00, 5, 1)
#define I2C_IC_CON_IC_SLAVE_DISABLE              I2C_PACK(0x00, 6, 1)

#define I2C_IC_TAR_IC_TAR                        I2C_PACK(0x04, 0, 10)
#define I2C_IC_TAR_GC_OR_START                   I2C_PACK(0x04, 10, 1)

#define I2C_IC_SS_SCL_HCNT                       I2C_PACK(0x14, 0, 16)
#define I2C_IC_SS_SCL_LCNT                       I2C_PACK(0x18, 0, 16)

#define I2C_IC_DATA_CMD_DAT                      I2C_PACK(0x10, 0, 10)
#define I2C_IC_DATA_CMD_STOP                     I2C_PACK(0x10, 9, 1)

#define I2C_IC_INTR_MASK                         I2C_PACK(0x30, 0, 15)

#define I2C_IC_ENABLE                            I2C_PACK(0x6c, 0, 1)

#define I2C_IC_STATUS_ACTIVITY                   I2C_PACK(0x70, 0, 1)
#define I2C_IC_STATUS_TFE                        I2C_PACK(0x70, 2, 1)
#define I2C_IC_STATUS_RFNE                       I2C_PACK(0x70, 3, 1)
#define I2C_IC_STATUS_MST_ACTIVITY               I2C_PACK(0x70, 5, 1)

#define I2C_IC_SDA_TX_HOLD                       I2C_PACK(0x7C, 0, 16)
#define I2C_IC_SDA_RX_HOLD                       I2C_PACK(0x7C, 16, 8)

#define I2C_IC_DATA_CMD_STOP_SHIFT               (9U)
#define I2C_IC_DATA_CMD_RESTART_SHIFT            (10U)
#define I2C_IC_DATA_CMD_CMD_BITS                 (0x00000100)

#endif //_DRV_I2C_REG_H_
