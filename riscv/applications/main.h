/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#ifdef AX_LOAD_ROOTFS_SUPPORT
#include "load_rootfs.h"
#endif

int qs_sensor();
int test_npu(void);
//int test_engine();
int lowpower_app(void);
int ax_detect(void);

/* ========================================================================================= */
/* ================  List all the user INIT_APP_EXPORT here, then the apps  ================ */
/* ================  will be executed in order.                             ================ */
/* ================  For example:                                           ================ */
/* ================              INIT_APP_EXPORT(qs_sensor);              ================ */
/* ================              INIT_APP_EXPORT(read_rootfs);              ================ */
/* ================  then read_rootfs will be executed before sensor_test.  ================ */
/* ================  Notes: The code order is oppsite to execution order    ================ */
/* ========================================================================================= */

INIT_APP_EXPORT(ax_detect);
INIT_APP_EXPORT(lowpower_app);
//INIT_APP_EXPORT(test_engine);
//INIT_APP_EXPORT(test_npu);
#ifdef AX_RISCV_ISP_SUPPORT
INIT_APP_EXPORT(qs_sensor);
#endif
#ifdef AX_LOAD_ROOTFS_SUPPORT
INIT_APP_EXPORT(read_rootfs);
#endif

int ive_test();
MSH_CMD_EXPORT(ive_test, case_id [-h] to get help);

//MSH_CMD_EXPORT(test_engine, test engine [STACK PRIORITY SLICE]);
