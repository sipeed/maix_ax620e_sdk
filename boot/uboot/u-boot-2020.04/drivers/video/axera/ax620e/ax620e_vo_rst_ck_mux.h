/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX620E_VO_RST_CK_MUX_H__
#define __AX620E_VO_RST_CK_MUX_H__

void dpu_glb_init(u32 id);
int display_glb_path_config(u32 id, u32 out_mode, struct ax_disp_mode *mode);
void display_glb_init(u32 id, u32 type);
#endif

