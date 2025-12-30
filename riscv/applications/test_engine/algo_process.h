/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include "algo_det_yolo.h"

#include <ax_engine_api.h>


typedef struct alg_info
{
    yolo_config_t cfg;
    AX_ENGINE_HANDLE handle;
    AX_ENGINE_IO_INFO_T* info;
    AX_ENGINE_IO_T io;
    det_boxes_t proposals;
    det_boxes_t objects;
} alg_info_t;


int alg_init(alg_info_t* alg);


int alg_release(alg_info_t* alg);


int alg_det(alg_info_t* alg, const void* raw_ptr);
