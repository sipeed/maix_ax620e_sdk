/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_GZIPD_REG_H_
#define __AX_GZIPD_REG_H_

#define GZIP_HEADER_VALUE       "GZIP"  //(0x475a4950)

#define AX_GZIPD_BASE_PADDR     0x10410000
#define GZIPD_HEADER_REG        0x00
#define GZIPD_CTRL_CFG_REG      0x04
#define GZIPD_WDMA_CFG0_REG     0x08
#define GZIPD_WDMA_CFG1_REG     0x0C
#define GZIPD_RDMA_CFG0_REG     0x10
#define GZIPD_RDMA_CFG1_REG     0x14
#define GZIPD_RDMA_CFG2_REG     0x18
#define GZIPD_INTR_CLR_REG      0x1C
#define GZIPD_STATUS0_REG       0x20
#define GZIPD_STATUS1_REG       0x24
#define GZIPD_STATUS2_REG       0x28
#define GZIPD_DUMMY0_REG        0x2C
#define GZIPD_DUMMY1_REG        0x30


#define FLASH_SYS_GLB_BASE_ADDR 0x10030000
#define FLASH_CLK_MUX0_REG_SET  0x4000

#define CPLL_24M                0x0
#define EPLL_100M               0x1
#define EPLL_250M               0x2
#define CPLL_416M               0x3
#define EPLL_500M               0x4
#define NPLL_533M               0x5

#define FLASH_CLK_EB0_REG       0x4
#define CLK_GZIPD_CORE_DISABLE  0x0
#define CLK_GZIPD_CORE_ENABLE   0x1

#define FLASH_CLK_EB1_REG       0x8
#define CLK_GZIPD_GATE_DISABLE  0x0
#define CLK_GZIPD_GATE_ENABLE   0x1

#define FLASH_SW_RST0_REG       0x14
#define GZIPD_SW_RST            (1<<11)
#define GZIPD_CORE_SW_RST       (1<<10)

#define FLASH_CLK_GZIPD_CORE_SEL_SET_REG    0x4000
#define CLK_GZIPD_CORE_SEL_SET              (0x7 << 9)

#define FLASH_SW_RST0_SET_REG   0x4014
#define FLASH_SW_RST0_CLR_REG   0x8014

#endif