/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_ae_stat_config.h"
#include "ax_ae_stat_config_os04a10.h"
#include "ax_ae_stat_config_os04d10.h"
#include "ax_ae_stat_config_sc200ai.h"
#include "ax_ae_stat_config_sc235hai.h"
#include "ax_ae_stat_config_sc450ai.h"
#include "ax_ae_stat_config_sc500ai.h"
#include "ax_ae_stat_config_sc850sl.h"
#include "ax_ae_stat_config_sc200ai_sc200ai.h"
#include "ax_ae_stat_config_os04a10_os04a10.h"
#include "ax_ae_stat_config_sc850sl_os04a10.h"

#include "rtconfig.h"

const AX_AE_STAT_CONFIG_T *getAeConfig(){

#ifdef AX_RISCV_SNS_SC200AI
    return &sc200ai_ae_param;
#elif defined(AX_RISCV_SNS_OS04A10)
    return &os04a10_ae_param;
#elif defined(AX_RISCV_SNS_OS04D10)
    return &os04d10_ae_param;
#elif defined(AX_RISCV_SNS_SC200AI_SC200AI)
    return &sc200ai_sc200ai_ae_param;
#elif defined(AX_RISCV_SNS_SC235HAI)
    return &sc235hai_ae_param;
#elif defined(AX_RISCV_SNS_SC200AI_SC200AI_SC200AI)
    return &sc200ai_sc200ai_ae_param;
#elif defined(AX_RISCV_SNS_SC450AI)
    return &sc450ai_ae_param;
#elif defined(AX_RISCV_SNS_SC500AI)
    return &sc500ai_ae_param;
#elif defined(AX_RISCV_SNS_SC850SL)
    return &sc850sl_ae_param;
#elif defined(AX_RISCV_SNS_OS04A10_OS04A10)
    return &os04a10_os04a10_ae_param;
#elif defined(AX_RISCV_SNS_SC850SL_OS04A10)
    return &sc850sl_os04a10_ae_param;
#else
    return &os04a10_ae_param;
#endif
}
