/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "model.h"

#include "algo_process.h"
#include "algo_utilities.h"

#include <string.h>


int alg_init(alg_info_t* alg)
{
    AX_ENGINE_NPU_ATTR_T attr = {0};
#ifdef CONFIG_USING_WARPED_MEMORY
    attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_DISABLE;
#endif

    int ret = AX_ENGINE_Init(&attr);
    if (0 != ret)
    {
        // TODO: add failed log here
        return -1;
    }

    ret = AX_ENGINE_CreateHandle(&alg->handle, det_model, sizeof(det_model));
    if (0 != ret)
    {
        // TODO: add failed log here
        return -1;
    }

    ret = AX_ENGINE_GetIOInfo(alg->handle, &alg->info);
    if (0 != ret)
    {
        // TODO: add failed log here
        return -1;
    }

    alg->io.nInputSize = alg->info->nInputSize;
    alg->io.nOutputSize = alg->info->nOutputSize;
    alg->io.nBatchSize = 1;

    alg->io.pInputs = sys_malloc(alg->io.nInputSize * (sizeof(AX_ENGINE_IO_BUFFER_T)));
    alg->io.pOutputs = sys_malloc(alg->io.nOutputSize * (sizeof(AX_ENGINE_IO_BUFFER_T)));
    memset(alg->io.pInputs, 0, alg->io.nInputSize * (sizeof(AX_ENGINE_IO_BUFFER_T)));
    memset(alg->io.pInputs, 0, alg->io.nInputSize * (sizeof(AX_ENGINE_IO_BUFFER_T)));

    for (int i = 0; i < alg->io.nInputSize; i++)
    {
        alg->io.pInputs[i].nSize = alg->info->pInputs[i].nSize;

#ifdef CONFIG_USING_WARPED_MEMORY
        ret = cmm_alloc(&alg->io.pInputs[i].pVirAddr, (uint64_t*)&alg->io.pInputs[i].phyAddr, alg->io.pInputs[i].nSize);
        if (0 != ret)
        {
            // TODO: add failed log here
            return -1;
        }
#endif
    }
    for (int i = 0; i <alg->io.nOutputSize; i++)
    {
        alg->io.pOutputs[i].nSize = alg->info->pOutputs[i].nSize;
        ret = cmm_alloc(&alg->io.pOutputs[i].pVirAddr, (uint64_t*)&alg->io.pOutputs[i].phyAddr, alg->io.pOutputs[i].nSize);
        if (0 != ret)
        {
            // TODO: add failed log here
            return -1;
        }
    }

    // init boxes
    det_boxes_init(&alg->proposals);
    det_boxes_init(&alg->objects);

    return 0;
}


int alg_release(alg_info_t* alg)
{
    det_boxes_free(&alg->proposals);
    det_boxes_free(&alg->objects);

#ifdef CONFIG_USING_WARPED_MEMORY
    for (int i = 0; i < alg->io.nInputSize; i++)
    {
        cmm_free(alg->io.pInputs[i].pVirAddr, alg->io.pInputs[i].phyAddr);
    }
#endif
    for (int i = 0; i <alg->io.nOutputSize; i++)
    {
        cmm_free(alg->io.pOutputs[i].pVirAddr, alg->io.pOutputs[i].phyAddr);
    }

    sys_free(alg->io.pInputs);
    sys_free(alg->io.pOutputs);

    int ret = AX_ENGINE_DestroyHandle(alg->handle);
    if (0 != ret)
    {
        // TODO: add failed log here
        return -1;
    }

    return AX_ENGINE_Deinit();
}


int alg_det(alg_info_t* alg, const void* raw_ptr)
{
    det_boxes_reset(&alg->proposals);
    det_boxes_reset(&alg->objects);

#ifdef CONFIG_USING_WARPED_MEMORY
    memcpy(alg->io.pInputs[0].pVirAddr, raw_ptr, alg->io.pInputs[0].nSize);
#else
    alg->io.pInputs[0].pVirAddr = (void*)raw_ptr;
    alg->io.pInputs[0].phyAddr = (ptrdiff_t)(raw_ptr);
#endif

    int ret = AX_ENGINE_RunSync(alg->handle, (AX_ENGINE_IO_T*)&alg->io);
    if (0 != ret)
    {
        // TODO: add failed log here
        return -1;
    }

    det_boxes_reset(&alg->proposals);

    for (int i = 0; i < alg->io.nOutputSize; i++)
    {
        det_feat_t feature = {0};

        if (alg->info->pOutputs[i].pShape[1] == alg->cfg.class_count + 5)
        {
            feature.ptr = alg->io.pOutputs[i].pVirAddr;
            feature.shape.w = alg->info->pOutputs[i].pShape[3];
            feature.shape.h = alg->info->pOutputs[i].pShape[2];
            feature.shape.c = alg->info->pOutputs[i].pShape[1];
            feature.shape.n = alg->info->pOutputs[i].pShape[0];

            generate_yolo_x_proposals_chw(&alg->cfg, &feature, &alg->proposals);
            continue;
        }

        if (alg->info->pOutputs[i].pShape[3] == alg->cfg.class_count + 5)
        {
            feature.ptr = alg->io.pOutputs[i].pVirAddr;
            feature.shape.c = alg->info->pOutputs[i].pShape[3];
            feature.shape.w = alg->info->pOutputs[i].pShape[2];
            feature.shape.h = alg->info->pOutputs[i].pShape[1];
            feature.shape.n = alg->info->pOutputs[i].pShape[0];

            generate_yolo_x_proposals_hwc(&alg->cfg, &feature, &alg->proposals);
            continue;
        }
    }

    det_get_out_bbox(&alg->proposals, &alg->objects, &alg->cfg.frame, &alg->cfg.letterbox, alg->cfg.nms_threshold);

    return 0;
}
