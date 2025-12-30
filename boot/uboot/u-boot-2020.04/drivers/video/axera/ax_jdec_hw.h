/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_JDEC_HW_H
#define __AX_JDEC_HW_H

#include <common.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch/ax620e.h>
#include <asm/unaligned.h>
#include <asm/io.h>
#include <malloc.h>
#include <mapmem.h>
#include <cpu_func.h>

#define RETRY_TIME                  (100)
#define JPEG_QTABLE_SIZE            (544)
#define JPEGDEC_YUV420              (2)
#define JPEGDEC_YUV422              (3)
#define JPEGDEC_YUV440              (5)
#define JPEGDEC_YUV411              (6)
#define COMMOM_SYS_GLB_ADDR_BASE    (0x2340000)
#define VPU_SYS_GLB_ADDR_BASE       (0x4030000)
#define JDEC_ADDR_BASE              (0x4020000)
#define VDEC_ADDR_OFFSET            (0x40000000)
#define ALIGN_UP(x, align)          ((((x) + (align) - 1) / (align)) * (align))


typedef struct
{
	u32 bits[16];
	u32 *vals;
	u32 tableLength;
} VlcTable;

typedef struct
{
	u32 Lh;
	VlcTable acTable0;
	VlcTable acTable1;
	VlcTable dcTable0;
	VlcTable dcTable1;
	VlcTable *table;
} HuffmanTables;

typedef struct
{
	u32 Lq;
	u32 table0[64];
	u32 table1[64];
	u32 *table;
} QuantTables;

typedef struct
{
	u32 Nf;
	u32 Ns;
	u32 amountOfQTables;
	u32 acdcTable;
	u32 yCbCrMode;
	u32 streamEnd;
	u32 fillright;
	u32 H[3];
	u32 V[3];
	u32 Tq[3];
	u32 Td[3];
	u32 Ta[3];
} ScanInfo;

static const u8 zzOrder[64] = {
	0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

int jpeg_decode_hw(int width, int height, unsigned char *imageData_jpg, unsigned char *logo_load_addr);

#endif /* __AX_JDEC_HW_H */
