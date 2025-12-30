/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <stdlib.h>

#include "ax_vin_config.h"
#include "ax_vin_config_os04a10.h"
#include "ax_vin_config_os04a10_os04a10.h"
#include "ax_vin_config_os04d10.h"
#include "ax_vin_config_sc200ai.h"
#include "ax_vin_config_sc235hai.h"
#include "ax_vin_config_sc200ai_sc200ai.h"
#include "ax_vin_config_sc200ai_sc200ai_0.h"
#include "ax_vin_config_sc200ai_sc200ai_1.h"
#include "ax_vin_config_sc450ai.h"
#include "ax_vin_config_sc500ai.h"
#include "ax_vin_config_sc850sl.h"
#include "ax_vin_config_sc850sl_2m.h"
#include "ax_vin_config_sc850sl_os04a10.h"
#include "rtconfig.h"
#include "ax_env.h"


AX_BOOL is_sc850sl_2m()
{
    char *env = fw_getenv("qs_sc850sl2m");
    if (!env)
        return AX_FALSE;

    return atoi(env) ? AX_TRUE : AX_FALSE;
}

AX_U8 is_three_2m()
{
    char *env = fw_getenv("qs_sns_switch");
    if (!env)
        return 0;

    return atoi(env);
}

const AX_SNS_CONFIG_T * getSensorConfig() {

#ifdef AX_RISCV_SNS_SC200AI
    return &sc200ai_sns_config;
#elif defined(AX_RISCV_SNS_OS04A10)
    return &os04a_sns_config;
#elif defined(AX_RISCV_SNS_OS04D10)
    return &os04d_sns_config;
#elif defined(AX_RISCV_SNS_SC235HAI)
    return &sc235hai_sns_config;
#elif defined(AX_RISCV_SNS_SC200AI_SC200AI)
    return &sc200ai_sc200ai_sns_config;
#elif defined(AX_RISCV_SNS_SC450AI)
    return &sc450ai_sns_config;
#elif defined(AX_RISCV_SNS_SC500AI)
    return &sc500ai_sns_config;
#elif defined(AX_RISCV_SNS_SC850SL)
    if (is_sc850sl_2m()) {
        return &sc850sl_sns_config_2m;
    } else {
        return &sc850sl_sns_config;
    }
#elif defined(AX_RISCV_SNS_OS04A10_OS04A10)
    return &os04a10_os04a10_sns_config;
#elif defined(AX_RISCV_SNS_SC850SL_OS04A10)
    return &sc850sl_os04a10_sns_config;
#elif defined(AX_RISCV_SNS_SC200AI_SC200AI_SC200AI)
    if (is_three_2m() == 2) {
        return &sc200ai_sc200ai_sns_config1;
    } if (is_three_2m() == 1) {
        return &sc200ai_sc200ai_sns_config0;
    } else {
        return &sc200ai_sc200ai_sns_config0;
    }
#else
    return &os04a_sns_config;
#endif
}
