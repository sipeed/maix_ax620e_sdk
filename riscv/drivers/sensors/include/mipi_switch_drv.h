/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_MIPI_SWITCH_DRV_H_
#define _AX_MIPI_SWITCH_DRV_H_

#include "ax_base_type.h"
#include "drv_i2c.h"

#define AX_MIPI_SWITCH_DRIVER_VERSION                   "V1.0"
#define AX_MIPI_SWITCH_DRIVER_NAME                      "ax_mipi_switch"

#define AX_MIPI_SWITCH_PIPE_NUM             2
#define AX_MIPI_SWITCH_SNS_REG_NUM          4

typedef enum _MIPI_SWITCH_WORK_MODE_E {
    MIPI_SWITCH_STAY_LOW,
    MIPI_SWITCH_STAY_HIGH,
    MIPI_SWITCH_SWITCH_PERIODIC,
} MIPI_SWITCH_WORK_MODE_E;

typedef enum _MIPI_SWITCH_VSYNC_TYPE_E {
    MIPI_SWITCH_FSYNC_VSYNC,
    MIPI_SWITCH_FSYNC_FLASH,
} MIPI_SWITCH_VSYNC_TYPE_E;

typedef struct _switch_sns_info {
    int sns_id;
    int pipe_id;
    MIPI_SWITCH_WORK_MODE_E work_mode;
} switch_sns_info;

typedef struct {
    unsigned int nRegAddr;
    unsigned int nData;
} vsync_sns_reg_t;

typedef struct {
    struct rt_i2c_bus_device *i2c_dev;
    unsigned char    nI2cAddr;  /* sensor device address */
    unsigned char    nDelayFrmNum;       /* Number of frames for register delay configuration */
    unsigned char    nIntPos;            /* Position of the register takes effect, only support AX_SNS_L_FSOF/AX_SNS_S_FSOF */
    unsigned char    reg_num;
    unsigned int     nAddrByteNum;       /* Bit width of the register address */
    unsigned int     nDataByteNum;       /* Bit width of sensor register data */
    vsync_sns_reg_t sns_reg[AX_MIPI_SWITCH_SNS_REG_NUM];
} vsync_sns_i2c_t;
typedef struct _vsync_info_t {
    vsync_sns_i2c_t i2c_info[AX_MIPI_SWITCH_PIPE_NUM];
    int fps;
} vsync_info_t;

typedef struct _mipi_switch_ctx {
    AX_U32       vsync0_gpio;
    AX_U32       vsync1_gpio;
    AX_U32       switch_gpio;
    AX_S32       pipe_id;
    AX_S32       pipe_num;
    AX_U32       fps;
    AX_U32       wait_count;
    MIPI_SWITCH_WORK_MODE_E work_mode;
    MIPI_SWITCH_VSYNC_TYPE_E vsync_type;
    switch_sns_info sns_info[AX_MIPI_SWITCH_PIPE_NUM];
    vsync_info_t vsync_info;
    AX_BOOL     is_change;
    AX_BOOL     is_fps_set;
    AX_BOOL     is_vsync_disable;
    AX_BOOL     is_switch_changed;
    AX_U8       exp_delay_frame;
} mipi_switch_ctx_t;

AX_S32 ax_mipi_switch_init(mipi_switch_ctx_t *mipi_switch);
AX_S32 ax_mipi_switch_start(AX_VOID);
AX_S32 ax_mipi_switch_vin_int_sof_process(AX_VOID *pdata);
int vsync_ctrl_sensor_vts_set(vsync_info_t *vsync_info);
AX_S32 ax_mipi_switch_set_fps(vsync_info_t *vsync_info);

#endif //_AX_MIPI_SWITCH_DRV_H_