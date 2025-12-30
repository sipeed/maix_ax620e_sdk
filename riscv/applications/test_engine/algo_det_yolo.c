/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "algo_det_yolo.h"

#include <float.h>
#include <math.h>


void generate_yolo_x_proposals_hwc(yolo_config_t* cfg, det_feat_t* feature, det_boxes_t* boxes)
{
    int feat_w = feature->shape.w;
    int feat_h = feature->shape.h;

    float stride = (float)(cfg->letterbox.w) / (float)(feat_w);

    float* feat_ptr = feature->ptr;

    for (int h = 0; h < feat_h; h++)
    {
        for (int w = 0; w < feat_w; w++)
        {
            float box_objectness = feat_ptr[4];
            if (box_objectness < cfg->conf_threshold)
            {
                feat_ptr += cfg->class_count + 5;
                continue;
            }

            //process cls score
            int class_index = 0;
            float class_score = -FLT_MAX;
            for (int s = 0; s < cfg->class_count; s++)
            {
                float score = feat_ptr[s + 5];
                if (score > class_score)
                {
                    class_index = s;
                    class_score = score;
                }
            }

            float box_prob = box_objectness * class_score;

            if (box_prob > cfg->conf_threshold)
            {
                float x_center = (feat_ptr[0] + (float)w) * stride;
                float y_center = (feat_ptr[1] + (float)h) * stride;
                float width = expf(feat_ptr[2]) * stride;
                float height = expf(feat_ptr[3]) * stride;
                float x0 = x_center - width * 0.5f;
                float y0 = y_center - height * 0.5f;

                det_bbox_t obj;
                obj.x = x0;
                obj.y = y0;
                obj.w = width;
                obj.h = height;
                obj.label = class_index;
                obj.prob = box_prob;

                det_boxes_add_bbox(boxes, &obj);
            }

            feat_ptr += cfg->class_count + 5;
        }
    }
}


void generate_yolo_x_proposals_chw(yolo_config_t* cfg, det_feat_t* feature, det_boxes_t* boxes)
{
    int feat_w = feature->shape.w;
    int feat_h = feature->shape.h;

    float stride = (float)(cfg->letterbox.w) / (float)(feat_w);

    float* x_ptr = &feature->ptr[0 * feat_h * feat_w];
    float* y_ptr = &feature->ptr[1 * feat_h * feat_w];
    float* h_ptr = &feature->ptr[2 * feat_h * feat_w];
    float* w_ptr = &feature->ptr[3 * feat_h * feat_w];
    float* objs_ptr = &feature->ptr[4 * feat_h * feat_w];
    float* cls_ptr = &feature->ptr[5 * feat_h * feat_w];

    for (int h = 0; h < feat_h; h++)
    {
        for (int w = 0; w < feat_w; w++)
        {
            int offset = h * feat_w + w;

            float box_objectness = objs_ptr[offset];
            if (box_objectness >= cfg->conf_threshold)
            {
                int class_index = 0;
                float class_score = -FLT_MAX;
                for (int s = 0; s < cfg->class_count; s++)
                {
                    float score = cls_ptr[s * feat_w * feat_h + offset];
                    if (score > class_score)
                    {
                        class_index = s;
                        class_score = score;
                    }
                }

                float box_prob = box_objectness * class_score;

                if (box_prob > cfg->conf_threshold)
                {
                    float x_center = (x_ptr[offset] + (float)w) * stride;
                    float y_center = (y_ptr[offset] + (float)h) * stride;
                    float width = expf(h_ptr[offset]) * stride;
                    float height = expf(w_ptr[offset]) * stride;
                    float x0 = x_center - width * 0.5f;
                    float y0 = y_center - height * 0.5f;

                    det_bbox_t obj;
                    obj.x = x0;
                    obj.y = y0;
                    obj.w = width;
                    obj.h = height;
                    obj.label = class_index;
                    obj.prob = box_prob;

                    det_boxes_add_bbox(boxes, &obj);
                }
            }
        }
    }

}
