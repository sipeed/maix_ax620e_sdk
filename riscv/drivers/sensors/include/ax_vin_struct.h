/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_VIN_STRUCT_H_
#define _AX_VIN_STRUCT_H_

#include "ax_base_type.h"
#include "ax_isp_common.h"
#include "ax_isp_iq_api.h"
#include "ax_isp_3a_struct.h"
#include "ax_sensor_struct.h"
#include "ax_mipi_rx_api.h"
#include "mipi_switch_drv.h"

#define AX_SNS_CNT_MAX   (2)

#ifndef ISP_RESERVED_DDR_END
#define ISP_RESERVED_DDR_END            (0x50000000)
#endif

#ifndef ISP_RESERVED_DDR_BASE
#define ISP_RESERVED_DDR_BASE           (0x4FF00000)
#endif

typedef struct _AX_SNS_INIT_ATTR_T_ {
    AX_U8               nFps;
    AX_U32              nWidth;
    AX_U32              nHeight;
    AX_U32              nSettingIndex;          /* optional, Not Recommended. if nSettingIndex > 0 will take effect */
} AX_SNS_SETTING_ATTR_T;

typedef struct _AX_SNS_CONFIG_ATTR_T_ {
    AX_SENSOR_REGISTER_FUNC_T  *pSnsHdl;
    AX_U8               mSnsId;
    AX_SNS_HDR_MODE_E   eSnsMode;
    AX_SNS_MASTER_SLAVE_E eMasterSlaveSel;
    AX_U8               mClkId;
    AX_SNS_CLK_RATE_E   eClkRate;
    AX_U8               nI2cDev;
    AX_U8               nI2cAddr;
    AX_U8               nResetGpio;
    AX_SNS_SETTING_ATTR_T tInitAttr;
    AX_SNS_SETTING_ATTR_T tConvergeAttr;
} AX_SNS_CONFIG_ATTR_T;

typedef struct _AX_SNS_MIPI_ATTR_T_ {
    AX_U8                nMipiId;
    AX_LANE_COMBO_MODE_E eLaneMode;
    AX_MIPI_RX_ATTR_T    tRxAttr;
} AX_SNS_MIPI_ATTR_T;

typedef struct _AX_SNS_PIPE_ATTR_T_ {
    AX_U8               nPipeId;
    AX_U32              nWidth;
    AX_U32              nHeight;
    AX_RAW_TYPE_E       eRawType;
    AX_IMG_FORMAT_E     ePixelFmt;
    AX_FRAME_RATE_CTRL_T   tFrameRateCtrl;
} AX_SNS_PIPE_ATTR_T;

typedef struct _AX_SNS_SWITCH_ATTR_T_ {
    AX_U8            vsync0_gpio;
    AX_U8            vsync1_gpio;
    AX_U8            switch_gpio;
    MIPI_SWITCH_WORK_MODE_E work_mode;
    MIPI_SWITCH_VSYNC_TYPE_E vsync_type;
} AX_SNS_SWITCH_ATTR_T;

typedef struct _AX_SNS_CONFIG_PARAM_T_ {
    /*sensor attr*/
    AX_SNS_CONFIG_ATTR_T tSnsAttr;

    /*dev attr*/
    AX_U8               nDevId;

    /*mipi attr*/
    AX_SNS_MIPI_ATTR_T tMipiAttr;

    /*pipe attr*/
    AX_SNS_PIPE_ATTR_T tPipeAttr;
} AX_SNS_CONFIG_PARAM_T;

typedef struct _AX_SNS_CONFIG_T_ {
    /*sensor cnt*/
    AX_U8               nSensorCnt;
    /*rtt cmm size Mb*/
    AX_U32               nCmmBase;
    AX_U32               nCmmEnd;
    /*sensor config*/
    AX_SNS_CONFIG_PARAM_T config[AX_SNS_CNT_MAX];
    /*swtich attr*/
    AX_SNS_SWITCH_ATTR_T tSwitchAttr;
} AX_SNS_CONFIG_T;

typedef struct _AX_AE_STAT_CONFIG_T_ {
    AX_ISP_IQ_AE_STAT_PARAM_T tAeParam[AX_SNS_CNT_MAX];
} AX_AE_STAT_CONFIG_T;

#endif //_AX_VIN_STRUCT_H_
