
/*
 * AXERA AX620E ext4 partition operation Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fs.h>
#include <asm/arch/ax620e.h>
#include <mapmem.h>
#include <elf.h>

int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t * info);
int ext4fs_probe(struct blk_desc *fs_dev_desc, disk_partition_t *fs_partition);
int ext4_write_file(const char *filename, void *buf, loff_t offset, loff_t len, loff_t *actwrite);
int ext4_read_file(const char *filename, void *buf, loff_t offset, loff_t len, loff_t *len_read);
int ext4fs_ls(const char *dirname);
int ext4fs_open(const char *filename, loff_t *len);
void ext4fs_close(void);

disk_partition_t fs_partition;
static int ax_ext4_prepare(const char* parttiton)
{
	int ret = 0;
	struct blk_desc *emmc_desc;

	emmc_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (NULL == emmc_desc) {
		printf("[error] memory dump: emmc is not present, exit dump!\n");
		return -1;
	}
	ret = get_part_info(emmc_desc, parttiton, &fs_partition);
	if(ret < 0) {
		printf("[error] memory dump get %s partition error, ret:%d\n", parttiton, ret);
		return ret;
	}
	ret = ext4fs_probe(emmc_desc, &fs_partition);

	if (ret) {
		printf("[error] memory dump ext4fs_probe failed ret: %d\n", ret);
	}

	return ret;
}

static int ext4_save_mem2file(unsigned long int addr,
                              unsigned long int size,
                              char *filename)
{
	int ret = 0;
	void *buf;
	loff_t offset = 0;
	loff_t actwrite;

	printf("[info] saving %s ...\n", filename);
	buf = map_sysmem(addr, size);
	ret = ext4_write_file(filename, buf, offset, size, &actwrite);
	unmap_sysmem(buf);
	if (ret < 0 && (size != actwrite)) {
		printf("[error] Unable to write file %s ret:%d, size:%ld, actwrite:%lld\n", filename, ret, size, actwrite);
	} else {
		printf("[info] dump %s success!!!\n", filename);
	}

	return ret;
}

int ax_do_ext4_load(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	int ret;
	int err;
	loff_t off;
	loff_t file_len;
	int load_addr;
	void *buf;
	if (argc != 4)
		return CMD_RET_USAGE;

	load_addr = simple_strtoul(argv[2], NULL, 16);
	ret = ax_ext4_prepare(argv[1]);
	if (ret == 0) {
		ret = ext4fs_open(argv[3], &file_len);
		if (ret < 0) {
			printf("** File not found %s **\n", argv[3]);
			return -1;
		}
		buf = map_sysmem(load_addr, file_len);
		err = ext4_read_file(argv[3], buf, 0, 0, &off);
		ext4fs_close();
		printf("** file len:%lld , read file len:%lld **\n", file_len, off);
		unmap_sysmem(buf);
		if (err == -1) {
			printf("\n** Unable to read \"%s\" from %s partition **\n",
				argv[3], argv[1]);
				return -1;
		}
		printf("ax_ext4load %s file to addr 0x%lx success\n", argv[3], (long)(long *)buf);
	} else {
		printf("ext4_prepare partiton:%s failed\n",argv[0]);
	}
	return 0;
}


int ax_do_ext4_ls(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	int ret;
	if (argc != 3)
		return CMD_RET_USAGE;

	ret = ax_ext4_prepare(argv[1]);
	if (ret == 0) {
		ext4fs_ls(argv[2]);
		ext4fs_close();
	}
	return 0;
}

int ax_do_ext4_write(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	int ret;
	int size;
	int load_addr;
	if (argc != 5)
		return CMD_RET_USAGE;

	load_addr = simple_strtoul(argv[2], NULL, 16);
	size = simple_strtoul(argv[3], NULL, 16);
	ret = ax_ext4_prepare(argv[1]);
	if (ret == 0) {
		printf("filename:%s, addr:0x%x, size:0x%x\n",argv[4], load_addr, size);
		ext4_save_mem2file( load_addr, size, argv[4]);
		ext4fs_close();
	}
	return 0;
}

U_BOOT_CMD(ax_ext4write, 7, 1, ax_do_ext4_write,
	   "create a file in the root directory",
	   "<partition name> <addr> <size> <absolute filename path>\n"
	   "eg: ax_ext4write opt 0x48000000 0x3000 /test.bin\n"
	   "    - create a file in / directory");

U_BOOT_CMD(ax_ext4load, 7, 0, ax_do_ext4_load,
	   "load binary file from a Ext4 filesystem",
	   "<partition name> <addr> <filename>\n"
	   "eg: ax_ext4load opt 0x48000000 /test.bin\n"
	   "    - load binary file 'filename' from 'partition' on 'eMMC' to address 'addr' from ext4 filesystem\n");

U_BOOT_CMD(ax_ext4ls, 7, 0, ax_do_ext4_ls,
	   "list file from a Ext4 filesystem",
	   "<partition name> <dirname>\n"
	   "eg: ax_ext4ls opt / \n"
	   "    - list files from 'paritition' on 'eMMC' in a 'directory");