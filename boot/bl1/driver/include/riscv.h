/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __RISCV_H__
#define __RISCV_H__

#define SW4_LOAD_ROOTFS_READY	BIT(5)
#define SW4_LOAD_ROOTFS_DONE	BIT(0)

void riscv_boot_up(void);
void riscv_start_load_rootfs(void);

#endif //__RISCV_H__
