/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OS04D10_REG_H__
#define __OS04D10_REG_H__

#include "ax_base_type.h"

#define OS04D10_SLAVE_ADDR1     (0x3C)    /**< i2c slave address of the OS04d10 camera sensor */
#define OS04D10_SLAVE_ADDR2     (0x10)    /**< i2c slave address of the OS04d10 camera sensor */
#define OS04D10_SENSOR_CHIP_ID  (0x53044410)
#define OS04D10_ADDR_BYTE       (1)
#define OS04D10_DATA_BYTE       (1)
#define OS04D10_SWAP_BYTES      (1)
#define OS04D10_INCK_24M        (24)

#define     OS04D10_PAGE_FLG_D2         (0xFD) /* PAGE_FLG_D2 default : 0x00 */
#define     OS04D10_PAGE_FLG_D2_P0      (0x00) /* System control */
#define     OS04D10_PAGE_FLG_D2_P1      (0x01) /* CIS timing */
#define     OS04D10_PAGE_FLG_D2_P2      (0x02) /* ISP and MIPI */
#define     OS04D10_PAGE_FLG_D2_P3      (0x03) /* DAC code */
#define     OS04D10_PAGE_FLG_D2_P4      (0x04) /* DPC SRAM */
#define     OS04D10_PAGE_FLG_D2_P5      (0x05) /* CIS data control */
#define     OS04D10_PAGE_FLG_D2_P6      (0x06) /* OTP control */
#define     OS04D10_PAGE_FLG_D2_P7      (0x07) /* CIS SRAM */

/* Exposure control related registers */
#define     OS04D10_LONG_EXP_LINE_H     (0x03)  /* bit[7:0], long frame exposure in unit of rows */ /* P1 */
#define     OS04D10_LONG_EXP_LINE_L     (0x04)  /* bit[7:0], long frame exposure in unit of rows */ /* P1 */
#define     OS04D10_VBLANK_GAIN_EXP_EN  (0x01)  /* bit[7:0], long frame exposure in unit of rows */ /* P1 */
#define     OS04D10_LONG_AGAIN          (0x24)  /* bit[4:0], real gain[8:4] long frame */ /* P1 */
#define     OS04D10_LONG_DGAIN_H        (0x37)  /* bit[3:0], real gain[13:10] long frame */ /* P5 */
#define     OS04D10_LONG_DGAIN_L        (0x39)  /* bit[7:6], real gain[1:0] long frame */ /* P5 */

#define     OS04D10_VTS_H               (0x35)  /* bit[7:0], vts[15:8] */ /* P1 */
#define     OS04D10_VTS_L               (0x36)  /* bit[7:0], vts[7:0] */ /* P1 */
#define     OS04D10_VTS_DEFAULT         (0x5c0)
#define     OS04D10_VTS_DEFAULT_2X2_BINNING  (0x2ea)

#define     OS04D10_VBLANK_H            (0x05)  /* bit[7:0], vblank[15:8] */ /* P1 */
#define     OS04D10_VBLANK_L            (0x06)  /* bit[7:0], vblank[7:0] */ /* P1 */

#define     OS04D10_HTS_H               (0x37)  /* bit[7:0], vts[15:8] */ /* P1 */
#define     OS04D10_HTS_L               (0x38)  /* bit[7:0], vts[7:0] */ /* P1 */
#define     OS04D10_HTS_DEFAULT         (0x32c)

#define     OS04D10_HBLANK_H            (0xa6)  /* bit[7:0], vblank[15:8] */ /* P1 */
#define     OS04D10_HBLANK_L            (0xa7)  /* bit[7:0], vblank[7:0] */ /* P1 */

#define     OS04D10_HCG_LCG             (0x45)  /* HCG_LCG_SWITCH */

#define     OS04D10_MIPI_EN             (0xB1)  /* MIPI_EN [stream on/off] */ /* P5 */
#define     OS04D10_TEST_PARTTEN_EN     (0x0D)  /* test partten en */ /* P5 */

#define     OS04D10_SLEEP_EN             (0x36)  /* SLEEP_EN [on/off] */ /* P0 */

typedef enum {
    e_OS04D10_setting_sel_min = 0,

    /* 2 lane */
    e_OS04D10_2lane_2560x1440_10bit_Linear_30fps,
    e_OS04D10_2lane_1280x720_10bit_Linear_60fps,

    e_OS04D10_setting_special_idx = 0x20, /* index = 32 */

    e_OS04D10_setting_sel_max
} OS04D10_SETTING_SEL_E;

AX_S32 os04d10_sensor_i2c_init(ISP_PIPE_ID nPipeId);
AX_S32 os04d10_sensor_i2c_exit(ISP_PIPE_ID nPipeId);

AX_S32 os04d10_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio, AX_U8 nValue);

AX_S32 os04d10_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData);
AX_S32 os04d10_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr);
AX_S32 os04d10_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data);

AX_U32 os04d10_get_hts(ISP_PIPE_ID nPipeId);
AX_U32 os04d10_get_vts(ISP_PIPE_ID nPipeId);
AX_U32 os04d10_set_vts(ISP_PIPE_ID nPipeId, AX_U32 vts);

AX_S32 os04d10_get_vblank_from_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array *setting, AX_U32 reg_cnt, AX_U32 *vblank);
AX_S32 os04d10_get_hts_from_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array *setting, AX_U32 reg_cnt, AX_U32 *hts);
AX_S32 os04d10_get_sclk_from_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array *setting, AX_U32 reg_cnt, AX_F32 *sclk);

AX_S32 os04d10_select_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array **setting, AX_U32 *cnt);
AX_S32 os04d10_write_settings(ISP_PIPE_ID nPipeId);

AX_S32 os04d10_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo);
AX_S32 os04d10_get_bus_num(ISP_PIPE_ID nPipeId);
AX_S32 os04d10_set_slaveaddr(ISP_PIPE_ID nPipeId, AX_U8 nslaveaddr);

#endif  //end __OS04D10_REG_H__
