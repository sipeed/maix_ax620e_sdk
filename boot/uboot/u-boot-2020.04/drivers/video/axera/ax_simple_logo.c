/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <common.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch/ax620e.h>
#include <asm/unaligned.h>
#include <asm/io.h>
#include <bmp_layout.h>

#include <splash.h>
#include "linux/delay.h"
#include <gzip.h>
#include <malloc.h>
#include <mapmem.h>

#include "stb_image.h"
#include "ax_vo.h"
#include "ax_jdec_hw.h"
// ### SIPEED EDIT ###
#include <fat.h>
// ### SIPEED EDIT END ###
extern u32 g_vdev_id;
extern u32 g_out_mode;
extern u32 g_fixed_sync;

static char *logo_fmt_str[AX_VO_LOGO_FMT_BUTT] = {
	"bmp",
	"jpg",
	"gzip"
};

static char *logo_mode_str[AX_DISP_OUT_MODE_BUT] = {
	"bt601",
	"bt656",
	"bt1120",
	"dpi",
	"dsi_dpi_video",
	"dsi_sdi_video",
	"dsi_sdi_cmd",
	"lvds",
};

static u64 reserved_logo_mem_addr;
static u64 reserved_logo_mem_size;

static int fdt_fixup_logo_reserved_mem(int dev, u64 addr, u64 size, void *fdt)
{
	int ret, offset, parent_offset;
	const uint32_t devid = cpu_to_fdt32(dev);
	const uint64_t reserved_addr = cpu_to_fdt64(addr);
	const uint64_t reserved_size = cpu_to_fdt64(size);
	char name[128];

	parent_offset = fdt_path_offset(fdt, "/reserved-memory/");
	if (parent_offset < 0) {
		VO_ERROR("reserved_mem not found\n");
		return -EINVAL;
	}

	sprintf(name, "boot_logo_reserved@%llx", addr);

	offset = fdt_add_subnode(fdt, parent_offset, name);
	if (offset < 0) {
		VO_ERROR("add %s to reserved_mem failed, ret = %d\n", name, offset);
		return offset;
	}

	ret = fdt_setprop(fdt, offset, "compatible", "boot_logo", 9);
	if (ret) {
		VO_ERROR("set compatible-prop to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	ret = fdt_setprop(fdt, offset, "id", &devid, sizeof(devid));
	if (ret) {
		VO_ERROR("set id-prop to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	ret = fdt_setprop(fdt, offset, "reg", &reserved_addr, sizeof(reserved_addr));
	if (ret) {
		VO_ERROR("set reg-prop to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	ret = fdt_appendprop(fdt, offset, "reg", &reserved_size, sizeof(reserved_size));
	if (ret) {
		VO_ERROR("append reg-prop to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

exit:
	if (ret)
		fdt_del_node(fdt, offset);

	VO_INFO("add %s node %s\n", name, ret ? "failed" : "success");

	return ret;
}

void fdt_fixup_logo_info(void *fdt)
{
	int ret, fdt_size = 0;
	u32 devid = g_vdev_id;
	u64 addr = reserved_logo_mem_addr, size = reserved_logo_mem_size;

	if (!fdt || fdt_check_header(fdt)) {
		VO_ERROR("device tree invalid\n");
		return;
	}

	if (!addr || !size) {
		VO_ERROR("reserved logo mem(%lld-%lld) invalid\n", addr, size);
		return;
	}

retry:
	ret = fdt_fixup_vo_init_mode(devid, fdt);
	if (!ret)
		ret = fdt_fixup_logo_reserved_mem(devid, addr, size, fdt);

	if ((ret == -FDT_ERR_NOSPACE) && !fdt_size) {
		VO_INFO("need for fdt expansion\n");
		fdt_size = fdt_totalsize(fdt);
		ret = fdt_open_into(fdt, fdt, fdt_size + 512);
		if (ret) {
			VO_ERROR("fdt expansion to 0x%x failed\n", fdt_size + 512);
			return;
		}

		goto retry;
	}
}

static bool is_big_endian(void)
{
	u32 test = 0x12345678;
	unsigned char *p = (unsigned char *)&test;

	return (*p == 0x12) ? true : false;
}

static u32 bmp_bpix2vo_fmt(u32 bpix)
{
	if (bpix == 16)
		return AX_VO_FORMAT_RGB565;
	else if (bpix == 24)
		return AX_VO_FORMAT_RGB888;
	else
		return AX_VO_FORMAT_ARGB8888;
}

extern int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t * info);

void resizeImage(int originalHeight, int originalWidth, int newHeight, int newWidth, int channels, unsigned char *originalImage, unsigned char *resizedImage, bool bReverse, bool bSwapRB) {
	int flipVertical = bReverse ? 1 : 0;
	int padding = (4 - (newWidth * channels) % 4) % 4;

	for (int i = 0; i < newHeight; i++) {
		for (int j = 0; j < newWidth; j++) {
			int x = (j * originalWidth + newWidth / 2) / newWidth;
			int y = (i * originalHeight + newHeight / 2) / newHeight;
			for (int c = 0; c < channels; c++) {
				int srcIndex = (y * originalWidth + x) * channels + c;
				int dstIndex = ((flipVertical ? (newHeight - 1 - i) : i) * (newWidth * channels + padding)) + (j * channels) + c;

				if (bSwapRB && (c == 0 || c == 2)) {
					resizedImage[dstIndex] = originalImage[srcIndex + (c == 0 ? 2 : -2)]; // Swap R and B channels
				} else {
					resizedImage[dstIndex] = originalImage[srcIndex];
				}
			}
		}
		for (int p = 0; p < padding; p++) {
			int dstIndex = ((flipVertical ? (newHeight - 1 - i) : i) * (newWidth * channels + padding) + newWidth * channels) + p;
			resizedImage[dstIndex] = 0;
		}
	}
}

static int emmc_parse_jpg_logo_data(void *imageData_jpg, void *logo_load_addr, struct jpeg_image *jpeg_image)
{
	int ret = 0;
	int width, height, channels;
	unsigned char *imageDecData = NULL;
	int newWidth = 0;
	int newHeight = 0;

	if (NULL == imageData_jpg) {
		printf("%s: imageData_jpg is NULL.\n", __func__);
		return -1;
	}

	if (NULL == logo_load_addr) {
		printf("%s: logo_load_addr is NULL.\n", __func__);
		return -1;
	}

	if (NULL == jpeg_image) {
		printf("%s: jpeg_image is NULL.\n", __func__);
		return -1;
	}

	ret = stbi_info_from_memory(imageData_jpg, CONFIG_VIDEO_AXERA_MAX_XRES * CONFIG_VIDEO_AXERA_MAX_YRES * AX_VO_CHANNEL, &width, &height, &channels);
	if (ret != 1) {
		printf("%s: parse info from jpg fail.\n", __func__);
		return -1;
	}

	if (width > CONFIG_VIDEO_AXERA_MAX_XRES || height > CONFIG_VIDEO_AXERA_MAX_YRES) {
		printf("%s: jpeg resolution out of range [%d , %d] \n", __func__, CONFIG_VIDEO_AXERA_MAX_XRES, CONFIG_VIDEO_AXERA_MAX_YRES);
		return -1;
	}

	if ((width == CONFIG_VIDEO_AXERA_DISPLAY_WIDTH) && (height == CONFIG_VIDEO_AXERA_DISPLAY_HEIGHT)) {
		/* hardware decode */
		printf("%s: hardware decode, jpeg param: width:%d, height:%d, channels:%d.\n", __func__, width, height, channels);
		ret = jpeg_decode_hw(width, height, imageData_jpg, logo_load_addr);
		if (ret) {
			printf("%s: JPEG data hard decode fail\n", __func__);
			goto jpeg_softdec;
		}

		jpeg_image->width = width;
		jpeg_image->height = height;
		jpeg_image->stride = ALIGN_UP(width, 16);
		jpeg_image->format = AX_VO_FORMAT_NV12;
		jpeg_image->phyAddr[0] = (u64)logo_load_addr;
		jpeg_image->phyAddr[1] = (u64)logo_load_addr + jpeg_image->stride * ALIGN_UP(height, 16);
		return 0;
	}

jpeg_softdec:
	/* Use the stb_image library to decode JPEG data */
	imageDecData = stbi_load_from_memory(imageData_jpg, CONFIG_VIDEO_AXERA_MAX_XRES * CONFIG_VIDEO_AXERA_MAX_YRES * AX_VO_CHANNEL, &width, &height, &channels, 0);
	if (!imageDecData) {
		printf("%s: Unable to decode JPEG data\n", __func__);
		return -1;
	}
	printf("%s: software decode, jpeg param: width:%d, height:%d, channels:%d.\n", __func__, width, height, channels);

	if ((width < CONFIG_VIDEO_AXERA_DISPLAY_WIDTH) && (height < CONFIG_VIDEO_AXERA_DISPLAY_HEIGHT)) {
		newWidth = AX_VO_JPEG_ALIGN(width, 16);
		newHeight = AX_VO_JPEG_ALIGN(height, 2);
	} else {
		newWidth = CONFIG_VIDEO_AXERA_DISPLAY_WIDTH;
		newHeight = CONFIG_VIDEO_AXERA_DISPLAY_HEIGHT;
	}

	printf("%s: resize_image newWidth:%d, newHeight:%d\n", __func__, newWidth, newHeight);
	resizeImage(height, width, newHeight, newWidth, channels, imageDecData, logo_load_addr, 0, 1);

	jpeg_image->width = newWidth;
	jpeg_image->height = newHeight;
	jpeg_image->stride = newWidth * 3;
	jpeg_image->format = AX_VO_FORMAT_RGB888;
	jpeg_image->phyAddr[0] = (u64)logo_load_addr;
	if (imageDecData) {
		free(imageDecData);
		imageDecData = NULL;
	}

	return 0;
}

static int get_logo_type(void)
{
	int ret;
	ulong rd_blkcnt;
	unsigned char *img_data = (unsigned char *)LOGO_IMAGE_LOAD_ADDR;
	struct blk_desc *blk_dev_desc;
	disk_partition_t part_info;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		printf("%s get mmc dev failed\n", __func__);
		ret = -1;
		goto exit;
	}

	ret = get_part_info(blk_dev_desc, "logo", &part_info);
	if(ret < 0) {
		printf("%s get logo partition info failed\n", __func__);
		ret = -1;
		goto exit;
	}

	rd_blkcnt = blk_dread(blk_dev_desc, part_info.start, 1, img_data);
	if (rd_blkcnt != 1) {
		printf("%s get the first blk failed from logo-partition\n", __func__);
		ret = -1;
		goto exit;
	}

	if ((img_data[0] == 0x1F) && (img_data[1] == 0x8B)) {
		ret = AX_VO_LOGO_FMT_GZ;
	} else if ((img_data[0] == 0xFF) && (img_data[1] == 0xD8)) {
		ret = AX_VO_LOGO_FMT_JPEG;
	} else if ((img_data[0] == 'B') && (img_data[1] == 'M')) {
		ret = AX_VO_LOGO_FMT_BMP;
	} else {
		printf("%s logo-fmt invalid(0x%x-0x%x)\n", __func__, img_data[0], img_data[1]);
		ret = -1;
		goto exit;
	}

	printf("%s logo-fmt:%s\n", __func__, logo_fmt_str[ret]);

exit:
	return ret;
}

int load_logo_from_mmc(unsigned char *logo_load_addr)
{
	u64 rd_blkcnt_lb_logo;
	struct blk_desc *blk_dev_desc = NULL;
	disk_partition_t part_info;
	u32 ret = 0;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		printf("get mmc dev fail\n");
		return -1;
	}
	printf("reading splash logo image++ ...\n");

	ret = get_part_info(blk_dev_desc,"logo", &part_info);
	if(ret == -1) {
		printf("%s: get logo partition info fail\n", __FUNCTION__);
		return -1;
	}

	rd_blkcnt_lb_logo = blk_dread(blk_dev_desc, part_info.start, part_info.size, logo_load_addr);
	if (rd_blkcnt_lb_logo != part_info.size) {
		printf("load_logo_from_mmc get logo image fail++ rd_blkcnt_lb_logo %llx part_info.size: %lx\n", rd_blkcnt_lb_logo, part_info.size);
		return -1;
	}

	printf("load logo image addr = 0x%llx\n",(u64)logo_load_addr);
	return 0;
}

static int check_logo_from_boot_partition(char *filename)
{
	int ret = 0;
	struct blk_desc *mmc_desc = NULL;
	disk_partition_t fs_partition;
	char *parttiton = "boot";
	loff_t file_len = 0;

	mmc_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (NULL == mmc_desc) {
		printf("[error] memory dump: emmc is not present, exit dump!\n");
		return -1;
	}

	ret = get_part_info(mmc_desc, parttiton, &fs_partition);
	if(ret < 0) {
		printf("[error] memory dump get %s partition error, ret:%d\n", parttiton, ret);
		return ret;
	}

	if (fat_set_blk_dev(mmc_desc, &fs_partition) != 0) {
		mmc_desc = blk_get_dev("mmc", SD_DEV_ID);
		if (NULL == mmc_desc) {
			printf("[error] memory dump: emmc/sd is not present, exit dump!\n");
			return -1;
		}

		ret = fat_register_device(mmc_desc, 1);
		if (ret != 0) {
			printf("[error] fat_register_device failed\n");
			return -1;
		}
	}

	if (!fat_exists(filename)) {
		printf("%s file is not exist\n", filename);
		return 0;
	} else {
		return 1;
	}
}

static int load_logo_from_boot_partition(unsigned char *logo_load_addr, char *filename)
{
	int ret = 0;
	struct blk_desc *mmc_desc = NULL;
	disk_partition_t fs_partition;
	char *parttiton = "boot";
	loff_t file_len = 0;

	mmc_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (NULL == mmc_desc) {
		printf("[error] memory dump: emmc is not present, exit dump!\n");
		return -1;
	}

	ret = get_part_info(mmc_desc, parttiton, &fs_partition);
	if(ret < 0) {
		printf("[error] memory dump get %s partition error, ret:%d\n", parttiton, ret);
		return ret;
	}

	if (fat_set_blk_dev(mmc_desc, &fs_partition) != 0) {
		mmc_desc = blk_get_dev("mmc", SD_DEV_ID);
		if (NULL == mmc_desc) {
			printf("[error] memory dump: emmc/sd is not present, exit dump!\n");
			return -1;
		}

		ret = fat_register_device(mmc_desc, 1);
		if (ret != 0) {
			printf("[error] fat_register_device failed\n");
			return -1;
		}
	}

	if (!fat_exists(filename)) {
		printf("%s file is not exist\n", filename);
		return -1;
	}

	if (fat_size(filename, &file_len) != 0) {
		printf("[error] get %s file size error\n", filename);
		return -1;
	}

	if (file_fat_read(filename, logo_load_addr, file_len) <= 0) {
		printf("file_fat_read failed, ret:%d\n", ret);
		return -1;
	}

	printf("load logo image addr = 0x%llx\n",(u64)logo_load_addr);
	return 0;
}

static void set_logo_mode(u32 devid, u32 type)
{
	char * bootargs, *logoparts, *p;
	char newbootargs[512], logomode[32];

	bootargs = env_get("bootargs");
	if(!bootargs)
		return;

	sprintf(logomode, " logomode=vo%d@%s", devid, logo_mode_str[type]);

	strcpy(newbootargs, bootargs);
	logoparts = strstr(newbootargs , "logomode");
	if (logoparts) {
		if (strstr(logoparts , &logomode[1]))
			return;

		for (p = logoparts; *p != ' ' && *p != '\0'; p++);

		if (*p != '\0')
			strcpy(logoparts, p);
	}

	strcat(newbootargs, logomode);
	env_set("bootargs", newbootargs);
	env_save();
}

// ### SIPEED EDIT ###
static int check_and_config_upgrade(void)
{
	uint32_t val = 0;
	writel(0x00000003, 0x2302024);
	val = readl(0x2302024);
	val = readl(0x600100c) & (~0x2);
	writel(val, 0x600100c);
	val = readl(0x600100c);
	udelay(1000);

	val = readl(0x600108c);
	val = readl(0x600108c);
	int boot_key = (val >> 2) & 0x01;

	char *_bootargs = NULL;
	_bootargs = env_get("bootargs");

	// delete boot_key=xxx
	char *pos = strstr(_bootargs, "boot_key=");
	if (pos) {
		char *pos2 = strstr(pos + strlen("boot_key="), " ");
		if (pos2) {
			strcpy(pos, pos2 + 1);
		} else {
			*pos = '\0';
		}
	}

	if (boot_key == 0) //  boot key pressed
	{
		char new_bootargs[512] = {0};
		printf("boot key pressed\n");

		memcpy(new_bootargs, _bootargs, strlen(_bootargs));
		char *boot_key_arg = " boot_key=1";
		memcpy(new_bootargs + strlen(new_bootargs), boot_key_arg, strlen(boot_key_arg));
		printf("new_bootargs[%d]: %s\n", strlen(new_bootargs), new_bootargs);
		env_set("bootargs", new_bootargs);

		return 1;
	} else {
		return 0;
	}
}
// ### SIPEED EDIT END ###

int ax_bootlogo_show(void)
{
	int ret = 0;
	unsigned data_offs, bmp_bpix;
	void *logo_load_addr = (void *)LOGO_IMAGE_LOAD_ADDR;
	boot_mode_info_t *boot_mode = (boot_mode_info_t *) BOOT_MODE_INFO_ADDR;
	struct display_info dp_info = {0};
	struct bmp_image *bmp;
	struct jpeg_image jpeg_image = {0};
	unsigned char *img_buf = NULL;
	unsigned long gz_len = 0;
	unsigned char *gz_addr = NULL;
	unsigned char *jpg_buf = NULL;
	unsigned char *jpg_addr = NULL;
	unsigned char *bmp_addr = NULL;
	u32 logo_type;
	u32 display_x = 0, display_y = 0;
	u32 devid = g_vdev_id;
	u32 sync = g_fixed_sync;
	u32 type = g_out_mode;

	struct udevice *dev = NULL;

	printf("richard %s:%d\n", __func__, __LINE__);
	uclass_get_device(UCLASS_PANEL_SPI, 0, &dev);

	if (boot_mode->dl_channel != DL_CHAN_UART1 &&
	    boot_mode->dl_channel != DL_CHAN_USB &&
	    boot_mode->dl_channel != DL_CHAN_SD) {
		// ### SIPEED EDIT ###
		// logo_type = (u32)get_logo_type();
		printf(" Set logo type to BMP!!!\r\n");
		logo_type = AX_VO_LOGO_FMT_BMP;
		// ### SIPEED EDIT END ###
		if (logo_type >= AX_VO_LOGO_FMT_BUTT)
			goto ERR_RET;

		if (logo_type != AX_VO_LOGO_FMT_BMP) {
			img_buf = memalign(AX_VO_LOGO_ALIGN_SIZE, AX_MAX_VO_LOGO_SIZE);
			if (!img_buf) {
				printf("%s alloc image buf for non-bmp format logo failed\n", __func__);
				goto ERR_RET;
			}

			logo_load_addr = (void *)img_buf;
			if (logo_type == AX_VO_LOGO_FMT_JPEG)
				jpg_addr = logo_load_addr;

		} else {
			logo_load_addr = map_sysmem(LOGO_IMAGE_LOAD_ADDR, 0);
			bmp_addr = logo_load_addr;
		}
// ### SIPEED EDIT ###
		char boot_bmp_name[32] = "logo.bmp";
		if (check_and_config_upgrade()) {
			strncpy(boot_bmp_name, "logo_upgrade.bmp", sizeof(boot_bmp_name));
		} else {
			int board_id = get_board_id();
			switch (board_id) {
			case PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G:
				strncpy(boot_bmp_name, "logo_512m.bmp", sizeof(boot_bmp_name));
				break;
			case PHY_AX630C_AX631_MAIXCAM2_SOM_1G:
				strncpy(boot_bmp_name, "logo_1g.bmp", sizeof(boot_bmp_name));
				break;
			case PHY_AX630C_AX631_MAIXCAM2_SOM_2G:
				strncpy(boot_bmp_name, "logo_2g.bmp", sizeof(boot_bmp_name));
				break;
			case PHY_AX630C_AX631_MAIXCAM2_SOM_4G:
				strncpy(boot_bmp_name, "logo_4g.bmp", sizeof(boot_bmp_name));
				break;
			default:
				strncpy(boot_bmp_name, "logo.bmp", sizeof(boot_bmp_name));
				break;
			break;
			}

			if (check_logo_from_boot_partition(boot_bmp_name) <= 0) {
				printf("image %s is not exist, use default image /boot/logo.bmp", boot_bmp_name);
				strncpy(boot_bmp_name, "logo.bmp", sizeof(boot_bmp_name));
			}
		}
		printf("try load image from /boot/%s\r\n", boot_bmp_name);
		if (load_logo_from_boot_partition(logo_load_addr, boot_bmp_name) < 0) {
			printf("try load_logo_from_mmc\r\n");
			ret = load_logo_from_mmc(logo_load_addr);
			if (ret < 0) {
				printf("fail to read logo from emmc partition\n");
				ret = -1;
				goto ERR_RET;
			}
		} else {
			printf("Load image from /boot/%s success!\r\n", boot_bmp_name);
		}
// ### SIPEED EDIT END ###
		if (logo_type == AX_VO_LOGO_FMT_GZ) {
			gz_addr = map_sysmem(LOGO_IMAGE_LOAD_ADDR, 0);
			gz_len = AX_MAX_VO_LOGO_SIZE;
			ret = gunzip(gz_addr, gz_len, logo_load_addr, &gz_len);
			if (ret) {
				printf("%s gunzip logo data failed, ret:%d\n", __func__, ret);
				goto ERR_RET;
			}

			if (gz_len >= AX_MAX_VO_LOGO_SIZE) {
				printf("%s image could be truncated, len(%ld) > maxsize(%d)\n",
				       __func__, gz_len, AX_MAX_VO_LOGO_SIZE);
				goto ERR_RET;
			}

			if ((gz_addr[0] == 0xFF) && (gz_addr[1] == 0xD8)) {
				jpg_buf = memalign(0x400, gz_len);
				if (!jpg_buf) {
					printf("%s alloc jpg buf failed\n", __func__);
					goto ERR_RET;
				}

				memcpy(jpg_buf, gz_addr, gz_len);

				gz_addr = jpg_buf;
				jpg_addr = gz_addr;

				printf("%s gzip-jpg-fmt gz_addr:%llx, len:%ld\n", __func__, (u64)gz_addr, gz_len);

			} else if ((gz_addr[0] == 'B') && (gz_addr[1] == 'M')) {
				bmp_addr = gz_addr;
				printf("%s gzip-bmp-fmt gz_addr:%llx\n", __func__, (u64)gz_addr);
			} else {
				printf("%s gzip-logo-fmt invalid(0x%x-0x%x)\n", __func__, gz_addr[0], gz_addr[1]);
				ret = -1;
				goto ERR_RET;
			}
		}

		if (jpg_addr) {
			logo_load_addr = map_sysmem(LOGO_IMAGE_LOAD_ADDR, 0);
			ret = emmc_parse_jpg_logo_data(jpg_addr, logo_load_addr, &jpeg_image);
			if (ret < 0) {
				printf("%s parse jpg-logo failed\n", __func__);
				goto ERR_RET;
			}

			dp_info.img_width = jpeg_image.width;
			dp_info.img_height = jpeg_image.height;
			dp_info.img_stride = jpeg_image.stride;
			dp_info.img_fmt = jpeg_image.format;
			dp_info.img_addr[0] = jpeg_image.phyAddr[0];
			dp_info.img_addr[1] = jpeg_image.phyAddr[1];

		} else if (bmp_addr) {
			bmp = (struct bmp_image *)bmp_addr;
			if (!((bmp->header.signature[0] == 'B') && (bmp->header.signature[1] == 'M'))) {
				printf("%s bmp invalid\n", __func__);
				ret = -1;
				goto ERR_RET;
			}

			bmp_bpix = get_unaligned_le16(&bmp->header.bit_count);
			data_offs = get_unaligned_le16(&bmp->header.data_offset);

			if ((bmp_bpix != 16) && (bmp_bpix != 24) && (bmp_bpix != 32)) {
				printf("%s bmp bpix(%d) invalid\n", __func__, bmp_bpix);
				ret = -1;
				goto ERR_RET;
			}

			dp_info.img_width = get_unaligned_le32(&bmp->header.width);
			dp_info.img_height = get_unaligned_le32(&bmp->header.height);
			dp_info.img_stride = dp_info.img_width * (bmp_bpix >> 3);
			dp_info.img_fmt = bmp_bpix2vo_fmt(bmp_bpix);
			dp_info.img_addr[0] = (u64)bmp_addr + data_offs;
		}

		if (!is_big_endian()) {
			if (dp_info.img_fmt == AX_VO_FORMAT_RGB565)
				dp_info.img_fmt = AX_VO_FORMAT_BGR565;
			else if (dp_info.img_fmt == AX_VO_FORMAT_RGB888)
				dp_info.img_fmt = AX_VO_FORMAT_BGR888;
		}

		dp_info.display_x = display_x;
		dp_info.display_y = display_y;
		dp_info.display_addr = LOGO_SHOW_BUFFER;

		printf("%s img-reso:%dx%d, fmt:%d, stride:%d, img-addr:%llx-%llx, display-coordi:%d-%d, display-addr:%llx\n", __func__,
		       dp_info.img_width, dp_info.img_height,
		       dp_info.img_fmt, dp_info.img_stride,
		       dp_info.img_addr[0], dp_info.img_addr[1],
		       dp_info.display_x, dp_info.display_y,
		       dp_info.display_addr);

		if (sync >= AX_VO_OUTPUT_BUTT) {
			if ((dp_info.img_width == 1920) && (dp_info.img_height == 1080)) {
				sync = AX_VO_OUTPUT_1080P60;
			} else if ((dp_info.img_width == 800) && (dp_info.img_height == 480)) {
				sync = AX_VO_OUTPUT_800_480_60;
			} else if ((dp_info.img_width == 1080) && (dp_info.img_height == 1920)) {
				sync = AX_VO_OUTPUT_1080x1920_60;
			} else if ((dp_info.img_width == 1280) && (dp_info.img_height == 720)) {
				sync = AX_VO_OUTPUT_720P30;
			} else if ((dp_info.img_width == 720) && (dp_info.img_height == 480)) {
				sync = AX_VO_OUTPUT_480P60;
// ### SIPEED EDIT ###
		} else if ((dp_info.img_width == 480) && (dp_info.img_height == 640)) {
			sync = AX_VO_OUTPUT_480x640_60;
// ### SIPEED EDIT END ###
			} else {
				printf("%s unsupported resolution(%dx%d)\n", __func__, dp_info.img_width, dp_info.img_height);
				ret = -1;
				goto ERR_RET;
			}
		}

		ret = ax_start_vo(devid, type, sync, &dp_info);
		if (!ret) {
			set_logo_mode(devid, type);
			reserved_logo_mem_addr = dp_info.reserved_mem_addr;
			reserved_logo_mem_size = dp_info.reserved_mem_size;
		}
	}

ERR_RET:
	if (img_buf)
		free(img_buf);
	if (jpg_buf)
		free(jpg_buf);

	printf("%s show logo to vo%d %s\n", __func__, devid, ret ? "failed" : "success");

	return ret;
}
