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

#include "algo_det_define.h"


void det_boxes_init(det_boxes_t* boxes);


void det_boxes_reset(det_boxes_t* boxes);


void det_boxes_resize(det_boxes_t* boxes, int32_t size);


void det_boxes_free(det_boxes_t* boxes);


void det_boxes_add_bbox(det_boxes_t* boxes, det_bbox_t* box);


void det_sort_boxes_descent(det_boxes_t* objects);


void det_get_out_bbox(det_boxes_t* proposals, det_boxes_t* objects, det_area_t* frame, det_area_t* letterbox, float nms_threshold);
