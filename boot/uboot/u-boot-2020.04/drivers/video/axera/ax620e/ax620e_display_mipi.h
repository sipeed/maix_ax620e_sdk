/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX620E_DISPLAY_MIPI_H__
#define __AX620E_DISPLAY_MIPI_H__

void display_mipi_dphy_config(int clk);
int display_mipi_panel_init(void);
void display_mipi_glb_init(void);
void display_mipi_cdns_config(struct ax_disp_mode *mode);

#endif