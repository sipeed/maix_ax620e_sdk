/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_PWM_API_H__
#define __DRV_PWM_API_H__

#include <stdint.h>

typedef enum {
    pwm_00 = 0,
    pwm_01,
    pwm_02,
    pwm_03,
    pwm_10,
    pwm_11,
    pwm_12,
    pwm_13,
    pwm_20,
    pwm_21,
    pwm_22,
    pwm_23,
    pwm_max
} pwm_e;

int pwm_init(pwm_e pwm, uint32_t freq_hz, uint8_t duty);
int pwm_start(pwm_e pwm);
int pwm_stop(pwm_e pwm);
int pwm_deinit(pwm_e pwm);

#endif //__DRV_PWM_API_H__
