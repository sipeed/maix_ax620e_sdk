/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AXEAR_SIMPLE_LOGO_H__
#define __AXEAR_SIMPLE_LOGO_H__

#define AX_VO_CHANNEL 3
#define UPDATE_LOGO_X 0
#define UPDATE_LOGO_Y 0
#define AX_MAX_JPEG_VO_LOGO_SIZE             (2 * 1024 * 1024)
#define AX_MAX_BMP_VO_LOGO_SIZE              (6 * 1024 * 1024)
#define AX_MAX_VO_LOGO_SIZE                  AX_MAX_BMP_VO_LOGO_SIZE
#define AX_VO_LOGO_ALIGN_SIZE                (2 * 1024 * 1024)

#define AX_VO_JPEG_ALIGN(x,a)   ( ((x) + ((a) - 1) ) & ( ~((a) - 1) ) )

typedef enum {
	AX_VO_LOGO_FMT_BMP,
	AX_VO_LOGO_FMT_JPEG,
	AX_VO_LOGO_FMT_GZ,
	AX_VO_LOGO_FMT_BUTT
} AX_VO_LOGO_FMT_E;


/* video mode */
#define MIPI_DSI_MODE_VIDEO		BIT(0)
/* video burst mode */
#define MIPI_DSI_MODE_VIDEO_BURST	BIT(1)
/* video pulse mode */
#define MIPI_DSI_MODE_VIDEO_SYNC_PULSE	BIT(2)
/* enable auto vertical count mode */
#define MIPI_DSI_MODE_VIDEO_AUTO_VERT	BIT(3)
/* enable hsync-end packets in vsync-pulse and v-porch area */
#define MIPI_DSI_MODE_VIDEO_HSE		BIT(4)
/* disable hfront-porch area */
#define MIPI_DSI_MODE_VIDEO_HFP		BIT(5)
/* disable hback-porch area */
#define MIPI_DSI_MODE_VIDEO_HBP		BIT(6)
/* disable hsync-active area */
#define MIPI_DSI_MODE_VIDEO_HSA		BIT(7)
/* flush display FIFO on vsync pulse */
#define MIPI_DSI_MODE_VSYNC_FLUSH	BIT(8)
/* disable EoT packets in HS mode */
#define MIPI_DSI_MODE_EOT_PACKET	BIT(9)
/* device supports non-continuous clock behavior (DSI spec 5.6.1) */
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	BIT(10)
/* transmit data in low power */
#define MIPI_DSI_MODE_LPM		BIT(11)

enum {
	AX_DISP_OUT_MODE_BT601 = 0,
	AX_DISP_OUT_MODE_BT656,
	AX_DISP_OUT_MODE_BT1120,
	AX_DISP_OUT_MODE_DPI,
	AX_DISP_OUT_MODE_DSI_DPI_VIDEO,
	AX_DISP_OUT_MODE_DSI_SDI_VIDEO,
	AX_DISP_OUT_MODE_DSI_SDI_CMD,
	AX_DISP_OUT_MODE_LVDS,
	AX_DISP_OUT_MODE_BUT,
};

enum mipi_dsi_pixel_format {
	MIPI_DSI_FMT_RGB888,
	MIPI_DSI_FMT_RGB666,
	MIPI_DSI_FMT_RGB666_PACKED,
	MIPI_DSI_FMT_RGB565,
};

struct mipi_dsi_panel_cfg {
	int nlanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	int gpio_num;
// ### SIPEED EDIT ###
	int pwms;
	int pwm_period;
	int pwm_duty;
// ### SIPEED EDIT END ###
	void *panel_init_seq;
	int init_seq_len;
	int reset_delay_ms;
};

extern struct mipi_dsi_panel_cfg g_dsi_panel;
#endif