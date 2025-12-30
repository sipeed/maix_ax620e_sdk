/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_jdec_hw.h"

static QuantTables quantTable = { 0 };
static HuffmanTables huffmanTable = { 0 };
static ScanInfo scaninfo = { 0 };

static int jpeg_decode_huffman_tables(unsigned char *input_stream_base, u64 offset)
{
	u32 i, len, Tc, Th, tmp, length;
	u32 reg_value1, reg_value2;

	reg_value1 = readb(input_stream_base + offset++);
	reg_value2 = readb(input_stream_base + offset++);

	length = reg_value1 << 8 | reg_value2;

	/* four bytes already read in */
	len = 4;

	while (len < length) {
		tmp = readb(input_stream_base + offset++);
		len++;
		/* Table class */
		Tc = tmp >> 4;
		if (Tc != 0 && Tc != 1) {
			return -1;
		}
		/* Huffman table identifier */
		Th = tmp & 0xF;

		/* set the table pointer */
		if (Tc) {
			/* Ac table */
			switch (Th) {
			case 0:
				huffmanTable.table = &(huffmanTable.acTable0);
				break;
			case 1:
				huffmanTable.table = &(huffmanTable.acTable1);
				break;
			default:
				return -1;
			}
		} else {
			/* Dc table */
			switch (Th) {
			case 0:
				huffmanTable.table = &(huffmanTable.dcTable0);
				break;
			case 1:
				huffmanTable.table = &(huffmanTable.dcTable1);
				break;
			default:
				return -1;
			}
		}

		tmp = 0;
		for (i = 0; i < 16; i++) {
			tmp += huffmanTable.table->bits[i] = readb(input_stream_base + offset++);
			len++;
		}

		huffmanTable.table->vals = (u32 *)memalign(4096, sizeof(u32) * tmp);

		if (huffmanTable.table->vals == NULL)
			return -1;

		/* set the table length */
		huffmanTable.table->tableLength = tmp;

		/* read in the huffman value */
		for (i = 0; i < tmp; i++) {
			huffmanTable.table->vals[i] = readb(input_stream_base + offset++);
			len++;
		}
	}

	return 0;
}

static int jpeg_decode_quant_tables(unsigned char *input_stream_base, u64 offset)
{
	u32 t, tmp, i;
	u32 reg_value1, reg_value2;

	reg_value1 = readb(input_stream_base + offset++);
	reg_value2 = readb(input_stream_base + offset++);
	quantTable.Lq = reg_value1 << 8 | reg_value2;

	t = 4;
	while(t < quantTable.Lq) {
		/* read tables and write to decData->quant */
		tmp = readb(input_stream_base + offset++);
		t++;
		/* supporting only 8 bits sample */
		if ((tmp >> 4) != 0) {
			return -1;
		}
		tmp &= 0xF;
		if (tmp == 0) {
			quantTable.table = quantTable.table0;
		} else if (tmp == 1) {
			quantTable.table = quantTable.table1;
		} else {
			return -1;
		}

		for (i = 0; i < 64; i++) {
			quantTable.table[i] = readb(input_stream_base + offset++);
			t++;
		}
	}
	return 0;
}

static int jpeg_decode_ns_info(unsigned char *input_stream_base, u64 offset)
{
	u32 i;
	u8 tmp, index;

	scaninfo.Ns = readb(input_stream_base + offset + 2);

	if (scaninfo.Ns == 1) {
		index = readb(input_stream_base + offset + 3);
		tmp =  readb(input_stream_base + offset + 4);
		scaninfo.Td[index] = tmp >> 4;    /* which DC table */
		scaninfo.Ta[index] = tmp & 0x0F;  /* which AC table */
	} else {
		for (i = 0; i < scaninfo.Ns; i++) {
			tmp =  readb(input_stream_base + offset + 4 + i * 2);
			scaninfo.Td[i] = tmp >> 4;    /* which DC table */
			scaninfo.Ta[i] = tmp & 0x0F;  /* which AC table */
		}
	}
	return 0;
}

static int jpeg_decode_nf_info(unsigned char *input_stream_base, u64 offset)
{
	int i;

	scaninfo.Nf = readb(input_stream_base + offset + 7);
	if (scaninfo.Nf == 1) {
		printf("%s: don't support YUV400!\n", __func__);
		return -1;
	}

	for (i = 0; i < scaninfo.Nf; i++) {
		scaninfo.H[i] = readb(input_stream_base + offset + 9 + i * 3) >> 4;
		scaninfo.V[i] = readb(input_stream_base + offset + 9 + i * 3) & 0xF;
		scaninfo.Tq[i] = readb(input_stream_base + offset + 10 + i * 3);
	}

	if (scaninfo.Nf == 3) {
		if (scaninfo.H[0] == 2 &&
		    scaninfo.V[0] == 2 &&
		    scaninfo.H[1] == 1 &&
		    scaninfo.V[1] == 1 &&
		    scaninfo.H[2] == 1 &&
		    scaninfo.V[2] == 1) {
			scaninfo.yCbCrMode = JPEGDEC_YUV420;
		} else if (scaninfo.H[0] == 2 &&
			   scaninfo.V[0] == 1 &&
			   scaninfo.H[1] == 1 &&
			   scaninfo.V[1] == 1 &&
			   scaninfo.H[2] == 1 &&
			   scaninfo.V[2] == 1) {
			scaninfo.yCbCrMode = JPEGDEC_YUV422;
		} else if (scaninfo.H[0] == 1 &&
			   scaninfo.V[0] == 2 &&
			   scaninfo.H[1] == 1 &&
			   scaninfo.V[1] == 1 &&
			   scaninfo.H[2] == 1 &&
			   scaninfo.V[2] == 1) {
			scaninfo.yCbCrMode = JPEGDEC_YUV440;
		} else if (scaninfo.H[0] == 4 &&
			   scaninfo.V[0] == 1 &&
			   scaninfo.H[1] == 1 &&
			   scaninfo.V[1] == 1 &&
			   scaninfo.H[2] == 1 &&
			   scaninfo.V[2] == 1) {
			scaninfo.yCbCrMode = JPEGDEC_YUV411;
		} else {
			printf("%s: format not support!\n", __func__);
			return -1;
		}
	}
	scaninfo.streamEnd = 1;
	return 0;
}

static int jpeg_decode_write_tables(u64 qtable_base)
{
	u32 j, i, offset = 0;
	u8 tableTmp[64] = { 0 };
	u32 shifter = 32;
	u32 tableWord = 0;
	u32 tableValue = 0;
	VlcTable *pTable1 = NULL;
	VlcTable *pTable2 = NULL;

	if ((scaninfo.Nf == 3 && scaninfo.Ns == 1) || (scaninfo.Nf == 1 && scaninfo.Ns == 1))
		scaninfo.amountOfQTables = 1;
	else
		scaninfo.amountOfQTables = 3;

	/* QP tables for all components */
	for (j = 0; j < scaninfo.amountOfQTables; j++) {
		if (scaninfo.Tq[j] == 0) {
			/* write quantTable */
			for (i = 0; i < 64; i++) {
				tableTmp[zzOrder[i]] = (u8) quantTable.table0[i];
			}

			/* update shifter */
			shifter = 32;
			for (i = 0; i < 64; i++) {
				shifter -= 8;

				if (shifter == 24)
					tableWord = (tableTmp[i] << shifter);
				else
					tableWord |= (tableTmp[i] << shifter);

				if (shifter == 0) {
					writel(tableWord, qtable_base + offset * 4);
					offset++;
					shifter = 32;
				}
			}
		} else {
			/* write quantTable */
			for (i = 0; i < 64; i++) {
				tableTmp[zzOrder[i]] = (u8) quantTable.table1[i];
			}

			/* update shifter */
			shifter = 32;

			for (i = 0; i < 64; i++) {
				shifter -= 8;
				if (shifter == 24)
					tableWord = (tableTmp[i] << shifter);
				else
					tableWord |= (tableTmp[i] << shifter);

				if (shifter == 0) {
					writel(tableWord, qtable_base + offset * 4);
					offset++;
					shifter = 32;
				}
			}
		}
	}

	if (scaninfo.Ta[0] == 0) {
		pTable1 = &(huffmanTable.acTable0);
		pTable2 = &(huffmanTable.acTable1);
	} else {
		pTable1 = &(huffmanTable.acTable1);
		pTable2 = &(huffmanTable.acTable0);
	}

	shifter = 32;
	/* write acTable1 */
	for (i = 0; i < 162; i++) {
		if (i < pTable1->tableLength)
			tableValue = (u8) pTable1->vals[i];
		else
			tableValue = 0;

		if (shifter == 32)
			tableWord = (tableValue << (shifter - 8));
		else
			tableWord |= (tableValue << (shifter - 8));

		shifter -= 8;
		if (shifter == 0) {
			writel(tableWord, qtable_base + offset * 4);
			offset++;
			shifter = 32;
		}
	}

	/* write acTable2 */
	for (i = 0; i < 162; i++) {
		if (i < pTable2->tableLength)
			tableValue = (u8) pTable2->vals[i];
		else
			tableValue = 0;

		if (shifter == 32)
			tableWord = (tableValue << (shifter - 8));
		else
			tableWord |= (tableValue << (shifter - 8));

		shifter -= 8;
		if (shifter == 0) {
			writel(tableWord, qtable_base + offset * 4);
			offset++;
			shifter = 32;
		}
	}

	if (scaninfo.Ta[0] == 0) {
		pTable1 = &(huffmanTable.dcTable0);
		pTable2 = &(huffmanTable.dcTable1);
	} else {
		pTable1 = &(huffmanTable.dcTable1);
		pTable2 = &(huffmanTable.dcTable0);
	}

	/* write dcTable1 */
	for (i = 0; i < 12; i++) {
		if (i < pTable1->tableLength)
			tableValue = (u8) pTable1->vals[i];
		else
			tableValue = 0;

		if (shifter == 32)
			tableWord = (tableValue << (shifter - 8));
		else
			tableWord |= (tableValue << (shifter - 8));

		shifter -= 8;
		if (shifter == 0) {
			writel(tableWord, qtable_base + offset * 4);
			offset++;
			shifter = 32;
		}
	}

	/* write dcTable2 */
	for (i = 0; i < 12; i++) {
		if (i < pTable2->tableLength)
			tableValue = (u8) pTable2->vals[i];
		else
			tableValue = 0;

		if (shifter == 32)
			tableWord = (tableValue << (shifter - 8));
		else
			tableWord |= (tableValue << (shifter - 8));

		shifter -= 8;
		if (shifter == 0) {
			writel(tableWord, qtable_base + offset * 4);
			offset++;
			shifter = 32;
		}
	}
	return 0;
}

static int jpeg_decode_chroma_table_selectors(void)
{
	int cr_ac_vlctable, cb_ac_vlctable;
	int cr_dc_vlctable, cb_dc_vlctable;

	if (scaninfo.Ta[0] == 0) {
		cr_ac_vlctable = scaninfo.Ta[2];
		cb_ac_vlctable = scaninfo.Ta[1];
	} else {
		if (scaninfo.Ta[0] == scaninfo.Ta[1])
			cb_ac_vlctable = 0;
		else
			cb_ac_vlctable = 1;

		if (scaninfo.Ta[0] == scaninfo.Ta[2])
			cr_ac_vlctable = 0;
		else
			cr_ac_vlctable = 1;
	}

	/* Third DC table selectors */
	if (scaninfo.Td[0] == 0) {
		cr_dc_vlctable = scaninfo.Td[2];
		cb_dc_vlctable = scaninfo.Td[1];
	} else {
		if (scaninfo.Td[0] == scaninfo.Td[1])
			cb_dc_vlctable = 0;
		else
			cb_dc_vlctable = 1;

		if (scaninfo.Td[0] == scaninfo.Td[2])
			cr_dc_vlctable = 0;
		else
			cr_dc_vlctable = 1;
	}

	scaninfo.acdcTable = (cr_ac_vlctable << 5) | (cb_ac_vlctable << 4) | (cr_dc_vlctable << 3) | (cb_dc_vlctable << 2);
	return 0;
}

static int jpeg_decode_write_lenbits(void)
{
	u32 reg_set_value = 0;
	VlcTable *pTable1 = NULL;
	VlcTable *pTable2 = NULL;

	if (scaninfo.Ta[0] == 0) {
		pTable1 = &(huffmanTable.acTable0);
		pTable2 = &(huffmanTable.acTable1);
	} else {
		pTable1 = &(huffmanTable.acTable1);
		pTable2 = &(huffmanTable.acTable0);
	}

	reg_set_value = (pTable1->bits[5] << 24) | (pTable1->bits[4] << 16) |
                        (pTable1->bits[3] << 11) | (pTable1->bits[2] << 7) |
                        (pTable1->bits[1] << 3) | (pTable1->bits[0] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000040);

	reg_set_value = (pTable1->bits[9] << 24) | (pTable1->bits[8] << 16) |
			(pTable1->bits[7] << 8) | (pTable1->bits[6] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000044);

	reg_set_value = (pTable1->bits[13] << 24) | (pTable1->bits[12] << 16) |
			(pTable1->bits[11] << 8) | (pTable1->bits[10] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000048);

	reg_set_value = (pTable2->bits[3] << 27) | (pTable2->bits[2] << 23) |
			(pTable2->bits[1] << 19) | (pTable2->bits[0] << 16) |
			(pTable1->bits[15] << 8) | (pTable1->bits[14] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x0000004c);

	reg_set_value = (pTable2->bits[7] << 24) | (pTable2->bits[6] << 16) |
			(pTable2->bits[5] << 8) | (pTable2->bits[4] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000050);

	reg_set_value = (pTable2->bits[11] << 24) | (pTable2->bits[10] << 16) |
			(pTable2->bits[9] << 8) | (pTable2->bits[8] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000054);

	reg_set_value = (pTable2->bits[15] << 24) | (pTable2->bits[14] << 16) |
			(pTable2->bits[13] << 8) | (pTable2->bits[12] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000058);

	if (scaninfo.Ta[0] == 0) {
		pTable1 = &(huffmanTable.dcTable0);
		pTable2 = &(huffmanTable.dcTable1);
	} else {
		pTable1 = &(huffmanTable.dcTable1);
		pTable2 = &(huffmanTable.dcTable0);
	}

	reg_set_value = (pTable1->bits[7] << 28) | (pTable1->bits[6] << 24) |
			(pTable1->bits[5] << 20) | (pTable1->bits[4] << 16) |
			(pTable1->bits[3] << 12) | (pTable1->bits[2] << 8) |
			(pTable1->bits[1] << 4) | (pTable1->bits[0] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x0000005c);

	reg_set_value = (pTable1->bits[15] << 28) | (pTable1->bits[14] << 24) |
			(pTable1->bits[13] << 20) | (pTable1->bits[12] << 16) |
			(pTable1->bits[11] << 12) | (pTable1->bits[10] << 8) |
			(pTable1->bits[9] << 4) | (pTable1->bits[8] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000060);

	reg_set_value = (pTable2->bits[7] << 28) | (pTable2->bits[6] << 24) |
			(pTable2->bits[5] << 20) | (pTable2->bits[4] << 16) |
			(pTable2->bits[3] << 12) | (pTable2->bits[2] << 8) |
			(pTable2->bits[1] << 4) | (pTable2->bits[0] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000064);

	reg_set_value = (pTable2->bits[15] << 28) | (pTable2->bits[14] << 24) |
			(pTable2->bits[13] << 20) | (pTable2->bits[12] << 16) |
			(pTable2->bits[11] << 12) | (pTable2->bits[10] << 8) |
			(pTable2->bits[9] << 4) | (pTable2->bits[8] << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000068);

	writel(0, JDEC_ADDR_BASE + 0x0000006c);
	writel(0, JDEC_ADDR_BASE + 0x00000070);
	return 0;
}

int jpeg_decode_hw(int width, int height, unsigned char *imageData_jpg, unsigned char *logo_load_addr)
{
	int ret = 0;
	u64 offset = 0;
	u64 sos_offset = 0, stream_size = 0, vlc_base_offset = 0;
	u64 input_stream_base = (u64)imageData_jpg;
	u64 luma_out_base = (u64)logo_load_addr;
	u64 chroma_out_base = luma_out_base + ALIGN_UP(width, 16) * ALIGN_UP(height, 16);
	u64 qtable_base = 0;
	u32 reg_value1 = 0, reg_value2 = 0, strm_start_bit = 0;
	u32 reg_set_value = 0;
	u32 retry = 0, irq_ready_flag = 0;
	unsigned char *qtable_addr = NULL;

	qtable_addr = memalign(4096, JPEG_QTABLE_SIZE);
        if (!qtable_addr) {
		printf("%s: fail to alloc qtable memory!\n", __func__);
		return -1;
	}
	qtable_base = (u64)qtable_addr;

	offset = 0;
        do {
            reg_value1 = imageData_jpg[offset];
	    reg_value2 = imageData_jpg[offset + 1];
	    if (reg_value1 == 0xFF && reg_value2 == 0xDA) {
		sos_offset = offset + 1;
		ret = jpeg_decode_ns_info(imageData_jpg, offset + 2);
		if (ret) {
			printf("%s: decode ns info fail\n", __func__);
			ret = -1;
			goto FREE_MEM;
		}
	    } else if (reg_value1 == 0xFF && reg_value2 == 0xDB) {
		ret = jpeg_decode_quant_tables(imageData_jpg, offset + 2);
		if (ret) {
			printf("%s: decode quant tables fail\n", __func__);
			ret = -1;
			goto FREE_MEM;
		}
	    } else if (reg_value1 == 0xFF && reg_value2 == 0xC4) {
		ret = jpeg_decode_huffman_tables(imageData_jpg, offset + 2);
		if (ret) {
			printf("%s: decode huffman tables fail\n", __func__);
			ret = -1;
			goto FREE_MEM;
		}
	    } else if (reg_value1 == 0xFF && (reg_value2 == 0xC0 || reg_value2 == 0xC2)) {
		ret = jpeg_decode_nf_info(imageData_jpg, offset + 2);
		if (ret) {
			printf("%s: decode nf info fail\n", __func__);
			ret = -1;
			goto FREE_MEM;
		}
	    }
	    offset++;
	} while (reg_value1 != 0xFF || reg_value2 != 0xD9);

        stream_size = offset + 1;
	vlc_base_offset = sos_offset + 13;
	strm_start_bit = (vlc_base_offset & (7)) * 8;
	vlc_base_offset = vlc_base_offset & (~7);

	reg_value1 = readl(COMMOM_SYS_GLB_ADDR_BASE + 0xc);
	reg_value1 = reg_value1 | (5 << 9);
	writel(reg_value1, COMMOM_SYS_GLB_ADDR_BASE + 0xc);

	reg_value1 = readl(COMMOM_SYS_GLB_ADDR_BASE + 0x18);
	reg_value1 = reg_value1 | (3 << 0);
	writel(reg_value1, COMMOM_SYS_GLB_ADDR_BASE + 0x18);

	reg_value1 = readl(VPU_SYS_GLB_ADDR_BASE);
	reg_value1 = reg_value1 | (5 << 3);
	writel(reg_value1, VPU_SYS_GLB_ADDR_BASE);

	reg_value1 = readl(VPU_SYS_GLB_ADDR_BASE + 0x4);
	reg_value1 = reg_value1 | (1 << 0);
	writel(reg_value1, VPU_SYS_GLB_ADDR_BASE + 0x4);

	reg_value1 = readl(VPU_SYS_GLB_ADDR_BASE + 0x8);
	reg_value1 = reg_value1 | (1 << 6);
	writel(reg_value1, VPU_SYS_GLB_ADDR_BASE + 0x8);

	reg_value1 = readl(VPU_SYS_GLB_ADDR_BASE + 0xc);
	reg_value1 = reg_value1 & (~(1 << 6));
	writel(reg_value1, VPU_SYS_GLB_ADDR_BASE + 0xc);

	/* read jdec id */
	reg_value1 = readl(JDEC_ADDR_BASE + 0x00000000);
	if (reg_value1 != 0x6e645000) {
		printf("%s: fail to read jdec register!\n", __func__);
		ret = -1;
		goto FREE_MEM;
	}

	writel(0x00000000, JDEC_ADDR_BASE + 0x00000004);
	writel(0x00F80110, JDEC_ADDR_BASE + 0x00000008);
	writel(0x30004000, JDEC_ADDR_BASE + 0x0000000c);

	reg_set_value = (((width + 15) / 16) << 23) | (((height + 15) / 16) << 11);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000010);

	jpeg_decode_write_tables(qtable_base);

	jpeg_decode_chroma_table_selectors();

	scaninfo.fillright = 0;

	reg_set_value = readl(JDEC_ADDR_BASE + 0x00000014);
	reg_set_value = (reg_set_value & (~0x3f0001fff)) | (strm_start_bit << 26) |
			(scaninfo.amountOfQTables << 11) | (scaninfo.yCbCrMode << 8) |
			(scaninfo.fillright << 7) | (scaninfo.streamEnd << 6) |
			(scaninfo.acdcTable << 0);
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000014);

	reg_set_value = stream_size - vlc_base_offset;
	writel(reg_set_value, JDEC_ADDR_BASE + 0x00000018);

	writel(0x0000003F, JDEC_ADDR_BASE + 0x0000001c);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000020);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000024);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000028);
	writel(0x00000000, JDEC_ADDR_BASE + 0x0000002c);

	reg_set_value = input_stream_base - VDEC_ADDR_OFFSET + vlc_base_offset;
        writel(input_stream_base - VDEC_ADDR_OFFSET + vlc_base_offset, JDEC_ADDR_BASE + 0x00000030);

	writel(luma_out_base - VDEC_ADDR_OFFSET, JDEC_ADDR_BASE + 0x00000034);
	writel(chroma_out_base - VDEC_ADDR_OFFSET, JDEC_ADDR_BASE + 0x00000038);
	writel(0x00000000, JDEC_ADDR_BASE + 0x0000003c);

	jpeg_decode_write_lenbits();

	writel(0x00000000, JDEC_ADDR_BASE + 0x00000074);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000078);
	writel(0x00000000, JDEC_ADDR_BASE + 0x0000007c);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000080);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000084);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000088);
	writel(0x00000000, JDEC_ADDR_BASE + 0x0000008c);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000090);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000094);
	writel(0x00000000, JDEC_ADDR_BASE + 0x00000098);
	writel(0x00000000, JDEC_ADDR_BASE + 0x0000009c);
	writel(qtable_base - VDEC_ADDR_OFFSET, JDEC_ADDR_BASE + 0x000000a0);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000a4);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000a8);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000ac);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000b0);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000b4);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000b8);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000bc);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000c0);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000c4);
	writel(0x00000008, JDEC_ADDR_BASE + 0x000000dc);
	writel(0x00000000, JDEC_ADDR_BASE + 0x000000e8);

	/* do flush before trigger irq */
	flush_dcache_all();

	/* triger irq */
	writel(0x00000001, JDEC_ADDR_BASE + 0x00000004);

	do {
	    reg_value1 = readl(JDEC_ADDR_BASE + 0x00000004);
	    udelay(100);
	    retry++;
	    if ((reg_value1 & 0x1000) == 0x1000) {
		    irq_ready_flag = 1;
	    }
	} while ((irq_ready_flag != 1) && (retry < RETRY_TIME));

	if (irq_ready_flag) {
		ret = 0;
	} else {
		ret = -1;
		printf("%s: decode fail,reg_val=0x%x,retry=%d\n", __func__, reg_value1, retry);
	}

FREE_MEM:

	if (huffmanTable.acTable0.vals) {
		free(huffmanTable.acTable0.vals);
		huffmanTable.acTable0.vals = NULL;
	}

	if (huffmanTable.acTable1.vals) {
		free(huffmanTable.acTable1.vals);
		huffmanTable.acTable1.vals = NULL;
	}

	if (huffmanTable.dcTable0.vals) {
		free(huffmanTable.dcTable0.vals);
		huffmanTable.dcTable0.vals = NULL;
	}

	if (huffmanTable.dcTable1.vals) {
		free(huffmanTable.dcTable1.vals);
		huffmanTable.dcTable1.vals = NULL;
	}

	if (qtable_addr) {
		free(qtable_addr);
		qtable_addr = NULL;
	}
	return ret;
}
