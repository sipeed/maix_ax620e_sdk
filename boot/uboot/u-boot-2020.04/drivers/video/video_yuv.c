// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2015 Google, Inc
 */

#include <common.h>
#include <bmp_layout.h>
#include <dm.h>
#include <mapmem.h>
#include <splash.h>
#include <video.h>
#include <watchdog.h>
#include <asm/unaligned.h>

#ifdef AXERA_LOGO_BMP2YUV

struct rgb_info {
	u8 r;
	u8 g;
	u8 b;
};
#define ALIGN_DOWN_16(value) ((value) & (~0xF))
#define ALIGN_DOWN_2(value) ((value) & (~0x1))
#define ALIGN_UP_256(value) (((value) + 0xFF) & (~0xFF))
#define COLORSIZE 256

static u32 Y_R[COLORSIZE], Y_G[COLORSIZE], Y_B[COLORSIZE], U_R[COLORSIZE],
    U_G[COLORSIZE], U_B[COLORSIZE], V_G[COLORSIZE], V_B[COLORSIZE];

static void video_splash_align_axis(int *axis, unsigned long panel_size,
				    unsigned long picture_size)
{
	long panel_picture_delta = panel_size - picture_size;
	long axis_alignment;

	if (*axis == BMP_ALIGN_CENTER)
		axis_alignment = panel_picture_delta / 2;
	else if (*axis < 0)
		axis_alignment = panel_picture_delta + *axis + 1;
	else
		return;

	*axis = max(0, (int)axis_alignment);
}

static void table_init(void)
{
	int i;

	for (i = 0; i < COLORSIZE; i++) {
		Y_R[i] = (i * 1224) >> 12;
		Y_G[i] = (i * 2404) >> 12;
		Y_B[i] = (i * 469) >> 12;
		U_R[i] = (i * 692) >> 12;
		U_G[i] = (i * 1356) >> 12;
		U_B[i] = i >> 1;
		V_G[i] = (i * 1731) >> 12;
		V_B[i] = (i * 334) >> 12;
	}
}

static int rgb24_to_nv12(u32 w, u32 h, u8 * rgb, u8 * nv12)
{
	int pix = 0;
	int pix4;
	int x, y;
	struct rgb_info *in = (struct rgb_info *)rgb;
	struct rgb_info rgb_byte;
	int size = w * h;
	u8 temp;

	table_init();

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			rgb_byte = in[(h - y - 1) * w + x];
			temp = rgb_byte.r;
			rgb_byte.r = rgb_byte.b;
			rgb_byte.b = temp;
			nv12[pix] =
			    Y_R[rgb_byte.r] + Y_G[rgb_byte.g] + Y_B[rgb_byte.b];

			if ((x % 2 == 1) && (y % 2 == 1)) {
				pix4 = w * (y >> 1) + x;
				nv12[pix4 - 1 + size] =
				    U_B[rgb_byte.b] - U_R[rgb_byte.r] -
				    U_G[rgb_byte.g] + 128;
				nv12[pix4 + size] =
				    U_B[rgb_byte.r] - V_G[rgb_byte.g] -
				    V_B[rgb_byte.b] + 128;
			}
			pix++;
		}
	}

	return 0;
}

int video_yuv_display(struct udevice *dev, ulong bmp_image, int x, int y,
		      bool align)
{
	struct video_priv *priv = dev_get_uclass_priv(dev);
	int i, j;
	struct bmp_image *bmp = map_sysmem(bmp_image, 0);
	u32 hdr_size, hdr_offset, bmp_bpix;
	u32 y_offset, uv_offset, w, h, stride, size;
	u32 pwidth = priv->xsize;
	u8 *paddr_y, *paddr_uv, *paddr, *nv12, *bmap;

	if (!bmp || !(bmp->header.signature[0] == 'B' &&
		      bmp->header.signature[1] == 'M')) {
		printf("Error: no valid bmp image at %lx\n", bmp_image);
		return -EINVAL;
	}

	w = get_unaligned_le32(&bmp->header.width);
	h = get_unaligned_le32(&bmp->header.height);
	bmp_bpix = get_unaligned_le16(&bmp->header.bit_count);
	hdr_size = get_unaligned_le16(&bmp->header.size);
	printf("hdr_size=%d, bmp_bpix=%d\n", hdr_size, bmp_bpix);
	hdr_offset = get_unaligned_le32(&bmp->header.data_offset);

	size = w * h;

	/*
	 * We support displaying 24bpp BMPs on yuv420 LCD
	 */
	if (bmp_bpix != 24) {
		printf("Error: BMP has %d bit/pixel should has 24 bit/pixel\n",
		       bmp_bpix);
		return -EPERM;
	}

	printf("Display-bmp: %d x %d	with  pixel depth: %d\n", (int)w,
	       (int)h, bmp_bpix);

	nv12 = map_sysmem(bmp_image + hdr_offset + size * 3, size * 2);

	if (align) {
		video_splash_align_axis(&x, priv->xsize, w);
		video_splash_align_axis(&y, priv->ysize, h);
	}

	if ((x + w) > pwidth)
		w = pwidth - x;
	if ((y + h) > priv->ysize)
		h = priv->ysize - y;

	bmap = (u8 *) bmp + hdr_offset;
	rgb24_to_nv12(w, h, bmap, nv12);

	stride = priv->stride;
	y_offset = x + y * stride;
	uv_offset = x + (y * stride >> 1);
	paddr = (u8 *) priv->fb;

	paddr_y = paddr + y_offset;
	paddr_uv = paddr + stride * priv->ysize + uv_offset;

	for (i = 0; i < h; ++i) {
		for (j = 0; j < w; j++) {
			paddr_y[i * stride + j] = nv12[i * w + j];
			if (i < h / 2)
				paddr_uv[i * stride + j] =
				    nv12[w * (h + i) + j];
		}
	}

	unmap_sysmem(nv12);
	video_sync(dev, false);
	return 0;
}
#endif
