/*
 * AXERA AX620E Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/boot_mode.h>
#include <memalign.h>
#include <linux/sizes.h>
#include <image-sparse.h>
#include "axera_update.h"
#include "../cipher/ax_cipher_api.h"

#define IP_ADDR_MAX_LEN	15
#define IP_ADDR_MIN_LEN 7
#define OTA_MAX_PART_NAME_LEN   32
#define OTA_MAX_FILE_NAME_LEN   48
#define AXERA_DISK_GUID_STR    "12345678-1234-1234-1234-000000000000"
#define INFO_PART_START_STR    "<Partition"
#define PART_DATA_UNIT_STR     "unit"
#define PART_DATA_ID_STR       "id"
#define PART_DATA_SIZE_STR     "size"
#define INFO_PART_END_STR      "</Partitions>"

typedef struct ota_part_info_t {
	char part_name[OTA_MAX_PART_NAME_LEN]; /* part name */
	u64 part_size;                        /* part size in Byte*/
	//char file_name[OTA_MAX_FILE_NAME_LEN]; /* part bin name */
	//u64 image_size;                       /* image size in Byte*/
	struct ota_part_info_t *next;
} ota_part_info_t;

typedef struct ota_info {
	char server_ip[16];
	char server_path[128];
	u64 ddr;
} ota_info_t;

extern u64 dl_buf_addr;
extern u64 dl_buf_size;
extern struct boot_mode_info boot_info_data;

extern void reboot(void);
extern int common_get_part_info(char * part_name, u64 * addr, u64 * len);
extern int common_raw_write(char * part_name, u64 addr, u64 len, char * data);
extern int common_raw_erase(char * part_name, u64 addr, u64 len);
extern int sparse_info_init(struct sparse_storage *info, const char *name);
extern int write_sparse_img(struct sparse_storage *info, char *part_name,
									void *data, u64 *response);

static unsigned long ota_tftp_download_file(char * cmd)
{
	int ret;

	printf("%s\n", cmd);
	env_set("filesize", NULL);
	ret = run_command(cmd, 0);
	if (0 == ret) {
		const char* size_str = env_get("filesize");
		unsigned long size = 0;
		if (size_str) size = simple_strtoul(size_str, NULL, 16);
		if (size == 0) {
			printf("img size error %ld\n", size);
			return -1;
		}
		return size;
	} else {
		printf("tftpdownload failed %d\n", ret);
		return -1;
	}
}

static int ota_get_part_info(ota_info_t info, struct update_part_info **part_list)
{
	int ret, size, count;
	char* ota_xml = "AX620E.xml";
	char cmd[256];

	if (NULL != env_get("ota_xml")) {
		strcpy(ota_xml, env_get("ota_xml"));
	}

	if (NULL != info.server_path) {
		sprintf(cmd, "tftpboot %llX %s:%s/%s", info.ddr, info.server_ip, info.server_path, ota_xml);
	}
	else {
		sprintf(cmd, "tftpboot %llX %s:%s", info.ddr, info.server_ip, ota_xml);
	}

	memset((void *)info.ddr, 0, 1024*1024);
	ret = ota_tftp_download_file(cmd);
	if (-1 == ret){
		printf("tftpdownload failed %d\n", ret);
		return ret;
	}

	size = ret;
	count = get_part_info_rawdata(part_list, (char *)info.ddr, size);
	if (count < 0) {
		printf("%s: error\n", __FUNCTION__);
		return -1;
	}

	return count;
}

static int ota_update_flash(char * part_name, u64 buf_addr, u64 file_size)
{
	int ret;
	u64 part_addr, part_size, sparse_buf_point;
	struct sparse_storage sparse;

	if (0 != common_get_part_info(part_name, &part_addr, &part_size)) {
		return -1;
	}
	if (file_size > part_size) {
		printf("%s: file size 0x%llX exceed %s part size 0x%llX\n", __FUNCTION__, file_size, part_name, part_size);
		return -1;
	}

	if (is_sparse_image((void *)buf_addr)) {
		printf("%s: %s is sparse format\n", __FUNCTION__, part_name);

		common_raw_erase(part_name, part_addr, part_size);

		if (sparse_info_init(&sparse, part_name)) {
			printf("sparse_info_init failed \n");
			return -1;
		}

		dl_buf_addr = OTA_BUF_ADDR;
		dl_buf_size = OTA_BUF_LEN;
		ret = write_sparse_img(&sparse, part_name, (void *)buf_addr, &sparse_buf_point);
		if (ret) {
			printf("%s: write sparse image fail, ret %d, end point addr 0x%llX\n", __FUNCTION__, ret, sparse_buf_point);
			return -1;
		}
	}
	else {
		ret = common_raw_write(part_name, part_addr, file_size, (char *)buf_addr);
		if (file_size != ret) {
			printf("%s: common_raw_write fail, ret %d, file_size 0x%llX\n", __FUNCTION__, ret, file_size);
			return -1;
		}
	}
	return 0;
}

int run_ota_update(const ota_info_t info, struct update_part_info *pheader)
{
	int ret = 0;
	int is_spl_done = 0;
	int is_uboot_done = 0;
	u64 file_size, buf_clear_size;
	struct update_part_info *pcur = pheader;
	char cmd[256];
	char ota_part[64];
	char * ota_file = NULL;
	char * ota_repartition = env_get("ota_repartition");
	printf("\nota info\n");
	printf("tftp server ip: %s\n", info.server_ip);
	printf("tftp server path: %s\n", info.server_path);
	printf("memory address: 0x%llx\n", info.ddr);

	if (NULL == ota_repartition) {
		printf("warning: skip repartition \n");
	}
	else {
		ret = update_parts_info(pheader);
		if (ret) {
			printf("update_parts_info fail\n");
			return ret;
		}
		printf("update_parts_info return %d\n", ret);
	}

	while (pcur) {
		printf("%s: %s\n", __FUNCTION__, pcur->part_name);
		if (strstr(pcur->part_name, "spl")) {
			is_spl_done = 1;
		}
		if (strstr(pcur->part_name, "uboot")) {
			is_uboot_done = 1;
		}

		/* check file is none not update */
		sprintf(ota_part, "ota_%s", pcur->part_name);
		ota_file = env_get(ota_part);
		if (NULL == ota_file) {
			printf("warning: skip %s update\n", pcur->part_name);
			pcur = pcur->next;
			continue;
		}

		if ((-1 == pcur->part_size) || (pcur->part_size >= OTA_BUF_LEN)) {
			buf_clear_size = OTA_BUF_LEN;
		}
		else {
			buf_clear_size = pcur->part_size;
		}
		printf("%s: buf clear size 0x%llX\n", pcur->part_name, buf_clear_size);
		memset((void *)info.ddr, 0, buf_clear_size);

		if (NULL != info.server_path) {
			sprintf(cmd, "tftpboot %llX %s:%s/%s", info.ddr, info.server_ip, info.server_path, ota_file);
		}
		else {
			sprintf(cmd, "tftpboot %llX %s:%s", info.ddr, info.server_ip, ota_file);
		}
		ret = ota_tftp_download_file(cmd);
		if (-1 == ret){
			printf("tftpdownload failed %d\n", ret);
			return ret;
		}
		file_size = ret;

		ret = ota_update_flash(pcur->part_name, info.ddr, file_size);
		if (0 != ret){
			printf("ota_update_flash fail\n");
			return ret;
		}
		printf("%s finished to update\n", pcur->part_name);

		pcur = pcur->next;
	}

	ota_file = env_get("ota_spl");
	if (!is_spl_done && (NULL != ota_file)) {
		memset((void *)info.ddr, 0, SPL_MAX_SIZE);
		if (NULL != info.server_path) {
			sprintf(cmd, "tftpboot %llX %s:%s/%s", info.ddr, info.server_ip, info.server_path, ota_file);
		}
		else {
			sprintf(cmd, "tftpboot %llX %s:%s", info.ddr, info.server_ip, ota_file);
		}
		ret = ota_tftp_download_file(cmd);
		if (-1 == ret){
			printf("tftpdownload spl failed %d\n", ret);
			return ret;
		}
		file_size = ret;

		ret = ota_update_flash("spl", info.ddr, file_size);
		if (0 != ret){
			printf("ota_update_flash spl fail\n");
			return ret;
		}
	}

	ota_file = env_get("ota_uboot");
	if (!is_uboot_done && (NULL != ota_file)) {
		memset((void *)info.ddr, 0, UBOOT_MAX_SIZE);
		if (NULL != info.server_path) {
			sprintf(cmd, "tftpboot %llX %s:%s/%s", info.ddr, info.server_ip, info.server_path, ota_file);
		}
		else {
			sprintf(cmd, "tftpboot %llX %s:%s", info.ddr, info.server_ip, ota_file);
		}
		ret = ota_tftp_download_file(cmd);
		if (-1 == ret){
			printf("tftpdownload uboot failed %d\n", ret);
			return ret;
		}
		file_size = ret;

		ret = ota_update_flash("uboot", info.ddr, file_size);
		if (0 != ret){
			printf("ota_update_flash uboot fail\n");
			return ret;
		}
	}
	printf("%s: return %d\n", __FUNCTION__, ret);

	return ret;
}

int do_axera_ota(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	char* ota_ip = env_get("ota_server_ip");
	char* ota_path = env_get("ota_server_path");
	struct update_part_info *pheader = NULL;
	struct update_part_info *pbin_info = NULL;

	printf("\n==================== OTA UPDATE ====================\n");
	printf("server_ip: %s\n", ota_ip);
#if defined(CONFIG_AXERA_SECURE_BOOT) && defined(CONFIG_CMD_AXERA_CIPHER)
	AX_CIPHER_Init();
#endif

	if (ota_ip && strlen(ota_ip) > IP_ADDR_MIN_LEN && strlen(ota_ip) < IP_ADDR_MAX_LEN) {
		ota_info_t info;
		info.ddr = OTA_BUF_ADDR;
		strcpy(info.server_ip, ota_ip);

		if (NULL != ota_path) {
			strcpy(info.server_path, ota_path);
			printf("server_path: %s\n", info.server_path);
		}

		printf("do dhcp \n");
		ret = run_command("dhcp", 0);
		if (ret) {
			printf("dhcp error %d\n", ret);
			goto normal_boot;
		}

		if (ota_get_part_info(info, &pheader) < 0) {
			printf("ota_get_part_info fail\n");
			goto normal_boot;
		}

		if (run_ota_update(info, pheader) < 0) {
			printf("ota_get_part_info fail\n");
			goto free_part_node;
		}
	}
	else {
		printf("server_ip error\n");
		goto normal_boot;
	}

	printf("all files ota updated successfully\n");

	env_set("ota_ready", "finish");
	env_save();
#ifdef CONFIG_SUPPORT_EMMC_BOOT
	set_emmc_boot_mode_after_dl();
#endif
	set_reboot_mode_after_dl();
	reboot();

	/* if update suceed, it will reboot, won't come here */
normal_boot:
#if !defined CONFIG_BOOT_OPTIMIZATION_SUPPORT
	printf("enter axera boot\n");
	run_command_list("axera_boot", -1, 0);
#else
	run_command_list("help", -1, 0);
#endif
	return 0;

free_part_node:
	while (pheader) {
		pbin_info = pheader;
		pheader = pheader->next;
		free(pbin_info);
	}

	printf("env ota_ready is %s", env_get("ota_ready"));
	if (!strcmp(env_get("ota_ready"), "retry")) {
		env_set("ota_ready", "false");
		printf("retry error, set ota_ready env to %s", env_get("ota_ready"));
	}
	else {
		env_set("ota_ready", "retry");
		printf("first error, set ota_ready env to %s", env_get("ota_ready"));
	}
	env_save();
	return -1;
}

U_BOOT_CMD(
	axera_ota,	1,	1,	do_axera_ota,
	"ota from tftp server",
	"[]"
);
