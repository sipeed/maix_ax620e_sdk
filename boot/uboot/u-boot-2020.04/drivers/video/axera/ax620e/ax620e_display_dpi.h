/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX620E_DISPLAY_DPI_H__
#define __AX620E_DISPLAY_DPI_H__

void display_dpi_pixel_clk_set_rate(u32 id, int sel, int div);
void display_dpi_glb_path_config(u32 id, u32 type);
void display_dpi_glb_init(u32 id);

#endif //__AX620E_DISPLAY_DPI_H__

