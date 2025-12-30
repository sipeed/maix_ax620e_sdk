/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "algo_det.h"
#include "algo_utilities.h"

#include <string.h>


void det_boxes_init(det_boxes_t* boxes)
{
    boxes->inserted_count = 0;
    boxes->stored_count = 16;
    boxes->boxes = (det_bbox_t*)sys_malloc(sizeof(det_bbox_t) * (boxes->stored_count + 16));
}


void det_boxes_reset(det_boxes_t* boxes)
{
    boxes->inserted_count = 0;
}


void det_boxes_resize(det_boxes_t* boxes, int32_t size)
{
    if (boxes->stored_count < size)
    {
        det_bbox_t* temp_boxes = (det_bbox_t*)sys_malloc(sizeof(det_bbox_t) * size);
        boxes->stored_count = size;
        memcpy(temp_boxes, boxes->boxes, sizeof(det_bbox_t) * boxes->inserted_count);
        sys_free(boxes->boxes);
        boxes->boxes = temp_boxes;
    }

    if (boxes->inserted_count > size)
    {
        boxes->inserted_count = size;
    }
}


void det_boxes_free(det_boxes_t* boxes)
{
    boxes->inserted_count = 0;
    sys_free(boxes->boxes);
    boxes->stored_count = 0;
}


void det_boxes_add_bbox(det_boxes_t* boxes, det_bbox_t* box)
{
    // if not enough space, malloc 16 structs more
    if (boxes->stored_count < boxes->inserted_count + 1)
    {
        det_bbox_t* temp_boxes = (det_bbox_t*)sys_malloc(sizeof(det_bbox_t) * (boxes->stored_count + 16));
        boxes->stored_count += 16;
        memcpy(temp_boxes, boxes->boxes, sizeof(det_bbox_t) * boxes->inserted_count);
        sys_free(boxes->boxes);
        boxes->boxes = temp_boxes;
    }

    boxes->boxes[boxes->inserted_count++] = *box;
}


void det_sort_boxes_swap(det_bbox_t* a, det_bbox_t* b)
{
    det_bbox_t box = *a;
    *a = *b;
    *b = box;
}


void det_sort_boxes_descent_inplace(det_boxes_t* objects, int left, int right)
{
    int i = left;
    int j = right;
    float p = objects->boxes[(left + right) / 2].prob;

    while (i <= j)
    {
        while (objects->boxes[i].prob > p) i++;

        while (objects->boxes[j].prob < p) j--;

        if (i <= j)
        {
            det_sort_boxes_swap(&objects->boxes[i], &objects->boxes[j]);
            i++;
            j--;
        }
    }

    if (left < j) det_sort_boxes_descent_inplace(objects, left, j);
    if (i < right) det_sort_boxes_descent_inplace(objects, i, right);
}


void det_sort_boxes_descent(det_boxes_t* objects)
{
    if (0 < objects->inserted_count)
    {
        det_sort_boxes_descent_inplace(objects, 0, objects->inserted_count - 1);
    }
}


float det_nms_intersection_area(const det_bbox_t* a, const det_bbox_t* b)
{
    float ia = 0.f;

    if (!(a->x > b->x + b->w || a->x + a->w < b->x || a->y > b->y + b->h || a->y + a->h < b->y))
    {
        float inter_width = fminf(a->x + a->w, b->x + b->w) - fmaxf(a->x, b->x) + 1.f;
        float inter_height = fminf(a->y + a->h, b->y + b->h) - fmaxf(a->y, b->y) + 1.f;
        ia = inter_width * inter_height;
    }

    return ia;
}


void det_nms_sorted_boxes(det_boxes_t* objects, int32_t* picked_index, int32_t* picked_count, float nms_threshold)
{
    *picked_count = 0;
    const int n = objects->inserted_count;

    float areas[n];
    for (int i = 0; i < n; i++)
    {
        areas[i] = objects->boxes[i].w * objects->boxes[i].h;
    }

    for (int i = 0; i < n; i++)
    {
        const det_bbox_t* a = &objects->boxes[i];

        int keep = 1;
        for (int j = 0; j < *picked_count; j++)
        {
            const det_bbox_t* b = &objects->boxes[picked_index[j]];

            // intersection over union
            float inter_area = det_nms_intersection_area(a, b);
            float union_area = areas[i] + areas[picked_index[j]] - inter_area;
            // float IoU = inter_area / union_area
            if (inter_area / union_area > nms_threshold) keep = 0;
        }

        if (1 == keep)
        {
            picked_index[*picked_count] = i;
            (*picked_count)++;
        }
    }
}


void det_get_out_bbox(det_boxes_t* proposals, det_boxes_t* objects, det_area_t* frame, det_area_t* letterbox, float nms_threshold)
{
    det_sort_boxes_descent(proposals);

    int32_t picked[proposals->inserted_count], picked_count = 0;
    memset(picked, 0, sizeof(int32_t) * proposals->inserted_count);

    det_nms_sorted_boxes(proposals, picked, &picked_count, nms_threshold);

    /* yolo v5 draw the result */
    float scale_letterbox;
    if (((float)letterbox->h / (float)frame->h) < ((float)letterbox->w / (float)frame->w))
    {
        scale_letterbox = (float)letterbox->h / (float)frame->h;
    }
    else
    {
        scale_letterbox = (float)letterbox->w / (float)frame->w;
    }

    int resize_rows = (int)(scale_letterbox * (float)frame->h);
    int resize_cols = (int)(scale_letterbox * (float)frame->w);

    float ratio_y = (float)frame->h / (float)resize_rows;
    float ratio_x = (float)frame->w / (float)resize_cols;

    det_boxes_resize(objects, proposals->stored_count);
    objects->inserted_count = picked_count;

    for (int i = 0; i < objects->inserted_count; i++)
    {
        objects->boxes[i] = proposals->boxes[picked[i]];

        float x0 = (objects->boxes[i].x);
        float y0 = (objects->boxes[i].y);
        float x1 = (objects->boxes[i].x + objects->boxes[i].w);
        float y1 = (objects->boxes[i].y + objects->boxes[i].h);

        x0 = x0 * ratio_x;
        y0 = y0 * ratio_y;
        x1 = x1 * ratio_x;
        y1 = y1 * ratio_y;

        x0 = fmaxf(fminf(x0, (float)(frame->w - 1)), 0.f);
        y0 = fmaxf(fminf(y0, (float)(frame->h - 1)), 0.f);
        x1 = fmaxf(fminf(x1, (float)(frame->w - 1)), 0.f);
        y1 = fmaxf(fminf(y1, (float)(frame->h - 1)), 0.f);

        objects->boxes[i].x = x0;
        objects->boxes[i].y = y0;
        objects->boxes[i].w = x1 - x0;
        objects->boxes[i].h = y1 - y0;
    }
}
