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

#include <stdint.h>
#include <math.h>


typedef struct det_area
{
    int32_t w;
    int32_t h;
} det_area_t;


typedef struct det_bbox
{
    int32_t label;
    float prob;
    float x;
    float y;
    float w;
    float h;
} det_bbox_t;


typedef struct det_boxes
{
    int32_t inserted_count;
    int32_t stored_count;
    det_bbox_t* boxes;
} det_boxes_t;


typedef struct det_class
{
    int32_t min_w;
    int32_t min_h;
    float conf_threshold;
} det_class_t;


typedef struct det_feat_shape
{
    int32_t n;
    int32_t h;
    int32_t w;
    int32_t c;
} det_feat_shape_t;


typedef struct det_feat
{
    float* ptr;
    det_feat_shape_t shape;
} det_feat_t;
