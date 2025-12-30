/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_PWM_REG_H__
#define __DRV_PWM_REG_H__

#include <stdint.h>

#define PWM_CLK_SEL_FREQ_HZ    24000000

typedef enum {
    pwm_idle = 0,
    pwm_busy
} pwm_status_e;

typedef struct {
    pwm_status_e group_status;
    uint8_t channel_status;
} pwm_status_t;

typedef enum {
    pwm_group_0 = 0,
    pwm_group_1,
    pwm_group_2,
    pwm_group_max
} pwm_group_e;

typedef enum {
    pwm_channel_0 = 0,
    pwm_channel_1,
    pwm_channel_2,
    pwm_channel_3,
    pwm_channel_max
} pwm_channel_e;

#define PWM_LOADCOUNT_OFF(N)     (0x0 + (N) * 0x14)
#define PWM_CONTROLREG_OFF(N)    (0x8 + (N) * 0x14)
#define PWM_LOADCOUNT2_OFF(N)    (0xB0 + (N) * 0x4)
#define PWM_MODE                 0x0000001E          /* PWM mode but not enable */
#define PWM_EN                   0x00000001          /* PWM enable bit */

#endif //__DRV_PWM_REG_H__
