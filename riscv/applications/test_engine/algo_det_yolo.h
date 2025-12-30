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

#include "algo_det.h"


typedef struct yolo_config
{
    float conf_threshold;
    float nms_threshold;
    det_area_t frame;
    det_area_t letterbox;
    int32_t class_count;
} yolo_config_t;


void generate_yolo_x_proposals_hwc(yolo_config_t* cfg, det_feat_t* feature, det_boxes_t* boxes);

void generate_yolo_x_proposals_chw(yolo_config_t* cfg, det_feat_t* feature, det_boxes_t* boxes);
