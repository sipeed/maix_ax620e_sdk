/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX620E_VO_REG_H
#define __AX620E_VO_REG_H

/* dpu register definition */
#define DPU_VERSION		0x0
#define DPU_INT_STA		0x4
#define DPU_INT_RAW		0x8
#define DPU_INT_MASK0		0xC
#define DPU_INT_MASK1		0x10
#define DPU_INT_CLR		0x14
#define DPU_CLK_GATE_BYPASS	0x18
#define DPU_SOFT_CLR		0x1C
#define DPU_TOP_CTRL		0x20
#define DPU_DRAW_UP		0x24
#define DPU_DRAW_EN		0x28
#define DPU_DRAW_RESO		0x2C
#define DPU_V0_RESO		0x30

#define DPU_V0_FORMAT		0x34
#define DPU_V0_ALPHA		0x38
#define DPU_V0_UV_FOR_Y		0x3C

#define DPU_V0_ADDR_Y_H		0x40
#define DPU_V0_ADDR_Y_L		0x44
#define DPU_V0_STRIDE_Y		0x48

#define DPU_V0_ADDR_C_H		0x4C
#define DPU_V0_ADDR_C_L		0x50
#define DPU_V0_STRIDE_C		0x54

#define DPU_V0_FBDC_EN		0x58
#define DPU_V0_FBDC_CTRL	0x5C
#define DPU_V0_FBDC_POS		0x60

#define DPU_V0_2YUV_EN		0x64
#define DPU_V0_2YUV_MATRIX_00	0x68
#define DPU_V0_2YUV_MATRIX_01	0x6C
#define DPU_V0_2YUV_MATRIX_02	0x70
#define DPU_V0_2YUV_MATRIX_10	0x74
#define DPU_V0_2YUV_MATRIX_11	0x78
#define DPU_V0_2YUV_MATRIX_12	0x7C
#define DPU_V0_2YUV_MATRIX_20	0x80
#define DPU_V0_2YUV_MATRIX_21	0x84
#define DPU_V0_2YUV_MATRIX_22	0x88
#define DPU_V0_2YUV_OFFSET_00	0x8C
#define DPU_V0_2YUV_OFFSET_01	0x90
#define DPU_V0_2YUV_OFFSET_02	0x94
#define DPU_V0_2YUV_OFFSET_10	0x98
#define DPU_V0_2YUV_OFFSET_11	0x9C
#define DPU_V0_2YUV_OFFSET_12	0xA0
#define DPU_V0_DECIMATION_H0	0xA4
#define DPU_V0_DECIMATION_H1	0xA8
#define DPU_V0_DECIMATION_H2	0xAC
#define DPU_V0_DECIMATION_H3	0xB0
#define DPU_V0_DECIMATION_H4	0xB4
#define DPU_V0_DECIMATION_H5	0xB8
#define DPU_V0_DECIMATION_H6	0xBC

#define DPU_V0_2YUV_CTRL		0xC0
#define DPU_V0_COODI			0xC4
#define DPU_V0_BACK_PIXEL		0xC8
#define DPU_V0_BACK_ALPHA		0xCC

#define DPU_G0_RESO			0xD0
#define DPU_G0_FORMAT			0xD4
#define DPU_G0_UV_FOR_Y			0xD8
#define DPU_G0_ADDR_Y_H			0xDC
#define DPU_G0_ADDR_Y_L			0xE0
#define DPU_G0_STRIDE_Y			0xE4
#define DPU_G0_ADDR_C_H			0xE8
#define DPU_G0_ADDR_C_L			0xEC
#define DPU_G0_STRIDE_C			0xF0
#define DPU_G0_ADDR_ALPHA_H		0xF4
#define DPU_G0_ADDR_ALPHA_L		0xF8
#define DPU_G0_STRIDE_ALPHA 		0xFC
#define DPU_G0_FBDC_EN 			0x100
#define DPU_G0_FBDC_CTRL		0x104
#define DPU_G0_FBDC_POS			0x108
#define DPU_G0_2YUV_EN			0x10C
#define DPU_G0_2YUV_MATRIX_00		0x110
#define DPU_G0_2YUV_MATRIX_01		0x114
#define DPU_G0_2YUV_MATRIX_02		0x118
#define DPU_G0_2YUV_MATRIX_10		0x11C
#define DPU_G0_2YUV_MATRIX_11		0x120
#define DPU_G0_2YUV_MATRIX_12		0x124
#define DPU_G0_2YUV_MATRIX_20		0x128
#define DPU_G0_2YUV_MATRIX_21		0x12C
#define DPU_G0_2YUV_MATRIX_22		0x130
#define DPU_G0_2YUV_OFFSET_00		0x134
#define DPU_G0_2YUV_OFFSET_01		0x138
#define DPU_G0_2YUV_OFFSET_02		0x13C
#define DPU_G0_2YUV_OFFSET_10		0x140
#define DPU_G0_2YUV_OFFSET_11		0x144
#define DPU_G0_2YUV_OFFSET_12		0x148
#define DPU_G0_DECIMATION_H0	        0x14C
#define DPU_G0_DECIMATION_H1	        0x150
#define DPU_G0_DECIMATION_H2	        0x154
#define DPU_G0_DECIMATION_H3	        0x158
#define DPU_G0_DECIMATION_H4	        0x15C
#define DPU_G0_DECIMATION_H5	        0x160
#define DPU_G0_DECIMATION_H6	        0x164

#define DPU_G0_2YUV_CTRL		0x168
#define DPU_G0_COODI			0x16C
#define DPU_G0_BACK_PIXEL		0x170
#define DPU_G0_BACK_ALPHA		0x174
#define DPU_G0_BLENDING			0x178

#define DPU_G0_COLOR_KEY_EN		0x17C
#define DPU_G0_COLOR_KEY_INVERT		0x180

#define DPU_G0_COLOR_KEY_0		0x184
#define DPU_G0_COLOR_KEY_1		0x188
#define DPU_G0_COLOR_KEY_2		0x18C
#define DPU_G0_COLOR_KEY_3		0x190
#define DPU_G0_COLOR_KEY_4		0x194
#define DPU_G0_COLOR_KEY_5		0x198
#define DPU_G0_ALPHA			0x19C
#define DPU_G0_PIXEL_Y			0x1A0
#define DPU_G0_PIXEL_U			0x1A4
#define DPU_G0_PIXEL_V			0x1A8
#define DPU_G0_ENV_EN			0x1AC
#define DPU_G0_INV_THR_Y		0x1B0
#define DPU_G0_INV_THR_U		0x1B4
#define DPU_G0_INV_THR_V		0x1B8
#define DPU_G0_INV_PIXEL_Y		0x1BC
#define DPU_G0_INV_PIXEL_U		0x1C0
#define DPU_G0_INV_PIXEL_V		0x1C4
#define DPU_G0_INV_BACK_COEFF		0x1C8
#define DPU_G0_INV_BACK_WEIGHT_0	0x1CC
#define DPU_G0_INV_BACK_WEIGHT_1	0x1D0
#define DPU_G0_INV_BACK_WEIGHT_2	0x1D4
#define DPU_G0_INV_BACK_WEIGHT_3	0x1D8
#define DPU_G0_INV_BACK_WEIGHT_4	0x1DC
#define DPU_G0_INV_BACK_WEIGHT_5	0x1E0
#define DPU_G0_INV_BACK_WEIGHT_6	0x1E4

#define DPU_WR_2RGB_CTRL		0x1E8
#define DPU_WR_2RGB_MATRIX_00		0x1EC
#define DPU_WR_2RGB_MATRIX_01           0x1F0
#define DPU_WR_2RGB_MATRIX_02		0x1F4
#define DPU_WR_2RGB_MATRIX_10		0x1F8
#define DPU_WR_2RGB_MATRIX_11		0x1FC
#define DPU_WR_2RGB_MATRIX_12		0x200
#define DPU_WR_2RGB_MATRIX_20		0x204
#define DPU_WR_2RGB_MATRIX_21		0x208
#define DPU_WR_2RGB_MATRIX_22		0x20C
#define DPU_WR_2RGB_OFFSET_00		0x210
#define DPU_WR_2RGB_OFFSET_01		0x214
#define DPU_WR_2RGB_OFFSET_02		0x218
#define DPU_WR_2RGB_OFFSET_10		0x21C
#define DPU_WR_2RGB_OFFSET_11		0x220
#define DPU_WR_2RGB_OFFSET_12		0x224

#define DPU_WR_2YUV_EN			0x228
#define DPU_WR_2YUV_OFFSET_U		0x22C
#define DPU_WR_2YUV_OFFSET_V		0x230
#define DPU_WR_DECIMATION_V0		0x234
#define DPU_WR_DECIMATION_V1		0x238
#define DPU_WR_2YUV_CTRL		0x23C
#define DPU_WR_FORMAT			0x240
#define DPU_WR_ADDR_Y_H			0x244
#define DPU_WR_ADDR_Y_L			0x248
#define DPU_WR_STRIDE_Y			0x24C
#define DPU_WR_ADDR_C_H			0x250
#define DPU_WR_ADDR_C_L			0x254
#define DPU_WR_STRIDE_C			0x258
#define DPU_WR_FBC_EN			0x25C
#define DPU_WR_FBC_CTRL			0x260
#define DPU_WR_FBC_POS			0x264
#define DPU_DISP_UP			0x268
#define DPU_DISP_EN			0x26C
#define DPU_DISP_RESO			0x270
#define DPU_RD_FORMAT			0x274
#define DPU_RD_UV_FOR_Y			0x278
#define DPU_RD_ADDR_Y_H			0x27C
#define DPU_RD_ADDR_Y_l			0x280
#define DPU_RD_STRIDE_Y 		0x284
#define DPU_RD_ADDR_C_H			0x288
#define DPU_RD_ADDR_C_l			0x28C
#define DPU_RD_STRIDE_C			0x290
#define DPU_RD_FBDC_EN			0x294
#define DPU_RD_FBDC_CTRL                0x298
#define DPU_RD_FBDC_POS                 0x29C
#define DPU_RD_2YUV_EN                  0x2A0
#define DPU_RD_2YUV_MATRIX_00           0x2A4
#define DPU_RD_2YUV_MATRIX_01           0x2A8
#define DPU_RD_2YUV_MATRIX_02           0x2AC
#define DPU_RD_2YUV_MATRIX_10           0x2B0
#define DPU_RD_2YUV_MATRIX_11           0x2B4
#define DPU_RD_2YUV_MATRIX_12           0x2B8
#define DPU_RD_2YUV_MATRIX_20           0x2Bc
#define DPU_RD_2YUV_MATRIX_21           0x2C0
#define DPU_RD_2YUV_MATRIX_22           0x2C4
#define DPU_RD_2YUV_OFFSET_00           0x2C8
#define DPU_RD_2YUV_OFFSET_01           0x2CC
#define DPU_RD_2YUV_OFFSET_02           0x2D0
#define DPU_RD_2YUV_OFFSET_10           0x2D4
#define DPU_RD_2YUV_OFFSET_11           0x2D8
#define DPU_RD_2YUV_OFFSET_12           0x2DC
#define DPU_RD_DECIMATION_H0            0x2E0
#define DPU_RD_DECIMATION_H1            0x2E4
#define DPU_RD_DECIMATION_H2            0x2E8
#define DPU_RD_DECIMATION_H3            0x2EC
#define DPU_RD_DECIMATION_H4            0x2F0
#define DPU_RD_DECIMATION_H5            0x2F4
#define DPU_RD_DECIMATION_H6            0x2F8
#define DPU_RD_2YUV_CTRL    	        0x2FC
#define DPU_MOUSE_CTRL    	        0x300
#define DPU_MOUSE_POS    	        0x304
#define DPU_MOUSE_RESO    	        0x308
#define DPU_MOUSE_FORMAT    	        0x30C
#define DPU_MOUSE_UV_FOR_Y    	        0x310
#define DPU_MOUSE_ADDR_H    	        0x314
#define DPU_MOUSE_ADDR_L    	        0x318
#define DPU_MOUSE_STRIDE    	        0x31C
#define DPU_MOUSE_2YUV_EN    	        0x320
#define DPU_MOUSE_2YUV_MATRIX_00    	0x324
#define DPU_MOUSE_2YUV_MATRIX_01    	0x328
#define DPU_MOUSE_2YUV_MATRIX_02    	0x32C
#define DPU_MOUSE_2YUV_MATRIX_10    	0x330
#define DPU_MOUSE_2YUV_MATRIX_11    	0x334
#define DPU_MOUSE_2YUV_MATRIX_12    	0x338
#define DPU_MOUSE_2YUV_MATRIX_20    	0x33C
#define DPU_MOUSE_2YUV_MATRIX_21    	0x340
#define DPU_MOUSE_2YUV_MATRIX_22    	0x344
#define DPU_MOUSE_2YUV_OFFSET_00        0x348
#define DPU_MOUSE_2YUV_OFFSET_01        0x34C
#define DPU_MOUSE_2YUV_OFFSET_02        0x350
#define DPU_MOUSE_2YUV_OFFSET_10        0x354
#define DPU_MOUSE_2YUV_OFFSET_11        0x358
#define DPU_MOUSE_2YUV_OFFSET_12        0x35C
#define DPU_MOUSE_2YUV_DECIMATION_H0    0x360
#define DPU_MOUSE_2YUV_DECIMATION_H1	0x364
#define DPU_MOUSE_2YUV_DECIMATION_H2	0x368
#define DPU_MOUSE_2YUV_DECIMATION_H3	0x36C
#define DPU_MOUSE_2YUV_DECIMATION_H4	0x370
#define DPU_MOUSE_2YUV_DECIMATION_H5	0x374
#define DPU_MOUSE_2YUV_DECIMATION_H6	0x378
#define DPU_MOUSE_2YUV_CTRL             0x37C
#define DPU_MOUSE_ALPHA	                0x380
#define DPU_MOUSE_PIXEL_Y	        0x384
#define DPU_MOUSE_PIXEL_U	        0x388
#define DPU_MOUSE_PIXEL_V	        0x38C
#define DPU_MOUSE_INV_EN                0x390
#define DPU_MOUSE_INV_THR_Y    	        0x394
#define DPU_MOUSE_INV_THR_U    	        0x398
#define DPU_MOUSE_INV_THR_V    	        0x39C
#define DPU_MOUSE_INV_PIXEL_Y           0x3A0
#define DPU_MOUSE_INV_PIXEL_U           0x3A4
#define DPU_MOUSE_INV_PIXEL_V           0x3A8
#define DPU_DISP_2RGB_CTRL		0x3AC
#define DPU_DISP_2RGB_MATRIX_00		0x3B0
#define DPU_DISP_2RGB_MATRIX_01         0x3B4
#define DPU_DISP_2RGB_MATRIX_02         0x3B8
#define DPU_DISP_2RGB_MATRIX_10		0x3BC
#define DPU_DISP_2RGB_MATRIX_11		0x3C0
#define DPU_DISP_2RGB_MATRIX_12		0x3C4
#define DPU_DISP_2RGB_MATRIX_20		0x3C8
#define DPU_DISP_2RGB_MATRIX_21		0x3CC
#define DPU_DISP_2RGB_MATRIX_22		0x3D0
#define DPU_DISP_2RGB_OFFSET_00		0x3D4
#define DPU_DISP_2RGB_OFFSET_01		0x3D8
#define DPU_DISP_2RGB_OFFSET_02		0x3DC
#define DPU_DISP_2RGB_OFFSET_10		0x3E0
#define DPU_DISP_2RGB_OFFSET_11		0x3E4
#define DPU_DISP_2RGB_OFFSET_12		0x3E8
#define DPU_CLIP_EN			0x3EC
#define DPU_CLIP_GAIN0			0x3F0
#define DPU_CLIP_GAIN1			0x3F4
#define DPU_CLIP_GAIN2			0x3F8
#define DPU_CLIP_OFFSET_0		0x3FC
#define DPU_CLIP_OFFSET_1		0x400
#define DPU_CLIP_OFFSET_2		0x404
#define DPU_CLIP_OFFSET_10		0x408
#define DPU_CLIP_OFFSET_11		0x40C
#define DPU_CLIP_OFFSET_12		0x410
#define DPU_CLIP_RANGE_00		0x414
#define DPU_CLIP_RANGE_01		0x418
#define DPU_CLIP_RANGE_10		0x41C
#define DPU_CLIP_RANGE_11		0x420
#define DPU_CLIP_RANGE_20		0x424
#define DPU_CLIP_RANGE_21		0x428
#define DPU_DITHER_EN			0x42C
#define DPU_DITHER_UP			0x430
#define DPU_DITHER_SEED_R		0x434
#define DPU_DITHER_PMASK_R		0x438
#define DPU_DITHER_SEED_G		0x43C
#define DPU_DITHER_PMASK_G		0x440
#define DPU_DITHER_SEED_B		0x444
#define DPU_DITHER_PMASK_B		0x448
#define DPU_DITHER_OUT_ACC		0x44C
#define DPU_BAD_PIXEL			0x450
#define DPU_OUT_MODE			0x454
#define DPU_DISP_CLK			0x458
#define DPU_DISP_HSYNC			0x45C
#define DPU_DISP_VSYNC			0x460
#define DPU_DISP_SYNC			0x464
#define DPU_DISP_HHALF			0x468
#define DPU_DISP_VTOTAL			0x46C
#define DPU_DISP_POLAR			0x470
#define DPU_DISP_FORMAT			0x474
#define DPU_BT_MODE			0x478
#define DPU_PIN_SRC_SEL0                0x47C
#define DPU_PIN_SRC_SEL1		0x480
#define DPU_PIN_SRC_SEL2		0x484
#define DPU_PIN_SRC_SEL3		0x488
#define DPU_PIN_SRC_SEL4                0x48C
#define DPU_SDI_MODE			0x490
#define DPU_POS_CMD_EN			0x494
#define DPU_COLUMN_ADDR			0x498
#define DPU_PAGE_ADDR			0x49C
#define DPU_TE_IN			0x4A0
#define DPU_TE_OUT			0x4A4
#define DPU_TE_CNT			0x4A8
#define DPU_AXI_EN			0x4AC
#define DPU_AXI_CNT			0x4B0
#define DPU_AXI_OSD			0x4B4
#define DPU_AXI_QOS			0x4B8
#define DPU_CMD_REQ			0x4BC
#define DPU_DEBUG_DRAW_TOP		0x4C0
#define DPU_DEBUG_JOIN_V0		0x4C4
#define DPU_DEBUG_JOIN_G0		0x4C8
#define DPU_DEBUG_DRAW_WR		0x4CC
#define DPU_DEBUG_DISP_TOP		0x4D0
#define DPU_DEBUG_JOIN_MOUSE		0x4D4
#define DPU_DEBUG_DUMMY_EN		0x4D8
#define DPU_DEBUG_DUMMY_IN		0x4DC
#define DPU_DEBUG_DUMMY_OUT		0x4E0


/*interrupt*/
#define DPU_INT_DRAW_EOF	BIT(0)
#define DPU_INT_DRAW_SOF	BIT(1)
#define DPU_INT_DISP_EOF	BIT(2)
#define DPU_INT_DISP_SOF	BIT(3)
#define DPU_INT_AXI_WERR	BIT(4)
#define DPU_INT_AXI_RERR	BIT(5)
#define DPU_INT_DISP_UNDER	BIT(6)

/*interrupt mask0*/
#define DPU_MASK_DRAW_EOF	BIT(0)
#define DPU_MASK_DRAW_SOF	BIT(1)
#define DPU_MASK_AXI_WERR	BIT(2)
/*interrupt mask1*/
#define DPU_MASK_DISP_EOF	BIT(0)
#define DPU_MASK_DISP_SOF	BIT(1)
#define DPU_MASK_AXI_RERR	BIT(2)
#define DPU_MASK_DISP_UNDER	BIT(3)

#define DISP_UPDATE 0x00000001
#define DRAW_UPDATE 0x00000001

#define OFFLINE_MODE 0x00000001
/* dpu register definition end */


/* mm_top_core registers definition start */
#define MM_CORE_INTR_MASK 0xD0
#define MM_CORE_INTR_MASK_SET 0xD4
#define MM_CORE_INTR_MASK_CLR 0xD8
#define MM_CORE_INTR_CLR 0xDC
#define MM_CORE_INTR_CLR_SET 0xE0
#define MM_CORE_INTR_CLR_CLR 0xE4
#define MM_CORE_INTR_RAW 0xE8
#define MM_CORE_INTR_STATUS 0xEC

/* mm_top_core registers definition end */

#endif /* __AX620E_VO_REG_H */

