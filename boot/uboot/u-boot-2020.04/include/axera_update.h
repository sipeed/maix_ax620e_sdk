#ifndef _AXERA_UPDATE_H_
#define _AXERA_UPDATE_H_

#define VERSION_2_PARSE_XML

#define XML_NAME     "AX620E.xml"

#define MAX_PART_NAME_LEN   32
#define MAX_FILE_NAME_LEN   48

#define SPL_MAX_SIZE           (768 * 1024)
#define UBOOT_MAX_SIZE         (1536 * 1024)

struct update_part_info {
	char part_name[MAX_PART_NAME_LEN];    /* part name */
	u64 part_size;                        /* part size in Byte*/
	char file_name[MAX_FILE_NAME_LEN];    /* part bin name */
	u64 image_size;                       /* image size in Byte*/
	struct update_part_info *next;
};

#ifdef SPI_DUAL_CS
struct sf1_part_info {
	char part_name[MAX_PART_NAME_LEN];    /* part name */
	u64 part_size;
	u64 part_offset;
	int part_index;
};
#endif

extern int get_part_info_rawdata(struct update_part_info **part_list, char * src, int len);
extern int update_parts_info(struct update_part_info *pheader);
#ifdef VERSION_2_PARSE_XML
int update_parse_part_info(struct update_part_info **bin_info);
#endif
#ifdef CONFIG_SUPPORT_EMMC_BOOT
extern int set_emmc_boot_mode_after_dl(void);
#endif
extern int set_reboot_mode_after_dl(void);
#ifdef CONFIG_SUPPORT_EMMC_BOOT
extern int is_emmc_switch_boot_part1(struct blk_desc *blk_dev_desc);
extern int emmc_switch_to_uda_part(struct blk_desc *blk_dev_desc);
#endif
#endif
