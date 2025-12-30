/*
 * (C) Copyright 2020 AXERA
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/arch/ax620e.h>
#include <asm/arch/boot_mode.h>
#include <image-sparse.h>
#include <malloc.h>
#include <linux/math64.h>
#include <mmc.h>
#include <mtd.h>
#include <blk.h>
#include <fdl_engine.h>

extern u_long dl_buf_addr;
extern u_long dl_buf_size;
extern struct boot_mode_info boot_info_data;

extern int common_raw_write(char *part_name, u64 addr, u64 len, char *data);
extern int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t * info);

void dump_buffer(u64 addr, u64 len)
{
	int i, j;
	u8 value;

	/* offset = addr - SPARSE_IMAGE_BUF_ADDR; */
	for (i = 0; i < (len / 0x10); i++) {
		printf("\r\n%llX + %4X: ", addr, i * 0x10);
		for (j = 0; j < 0x10; j++) {
			value = (u8) (addr + i * 0x10 + j);
			printf("%X", (value >> 4) & 0x0f);
			printf("%X ", (value & 0x0f));
		}
	}
}

int sparse_info_init_emmc(struct sparse_storage *sparse_info, const char *part_name)
{
	struct blk_desc *blk_dev_desc = NULL;
	disk_partition_t part_info;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		printf("get mmc dev fail\n");
		return -1;
	}

	if (!strcmp(part_name, "spl") || !strcmp(part_name, "uboot")) {
		printf("[%s]ERROR: only support filesystem image\n", __FUNCTION__);
		return -1;
	}
#ifdef USE_GPT_PARTITON
	if (part_get_info_by_name(blk_dev_desc, part_name, &part_info) == -1) {
		printf("[%s]ERROR: get %s partition info fail\n", __FUNCTION__, part_name);
		return -1;
	}
#else
	if (get_part_info(blk_dev_desc, part_name, &part_info) == -1) {
		printf("[%s]ERROR: get %s partition info fail\n", __FUNCTION__, part_name);
	}
#endif
	sparse_info->priv = blk_dev_desc;
	sparse_info->blksz = blk_dev_desc->blksz;
	sparse_info->start = part_info.start;
	sparse_info->size = part_info.size;
	sparse_info->write = NULL;
	sparse_info->reserve = NULL;
	sparse_info->mssg = NULL;
	printf("%s: blksz %ld, part %s start lba %ld lbacnt %ld\n", __FUNCTION__,
	       sparse_info->blksz, part_name, sparse_info->start, sparse_info->size);

	return 0;
}

int sparse_info_init(struct sparse_storage *info, const char *name)
{
	int ret = -1;

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
		ret = sparse_info_init_emmc(info, name);
		break;
	case STORAGE_TYPE_NAND:
		printf("%s: Nand flash not support sparse image now!\n", __FUNCTION__);
		break;
	case STORAGE_TYPE_NOR:
		printf("%s: Nor flash not support sparse image now!\n", __FUNCTION__);
		break;
	default:
		printf("%s: storage type error!\n", __FUNCTION__);
		break;
	}
	return ret;
}

int write_sparse_img(struct sparse_storage *info, char *part_name, void *data, ulong *response)
{
	static u64 blk;
	ulong data_addr;
	u64 blkcnt;
	u64 blks;
	u64 bytes;
	u64 bytes_written = 0;
	u32 chunk;
	u32 offset;
	u32 chunk_data_sz;
	u32 *fill_buf = NULL;
	u32 fill_val;
	static sparse_header_t sparse_header_info = { 0 };
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	static u64 total_blocks = 0;
	int fill_buf_num_blks;
	int i;
	int j;
	static u32 current_chunk = 0;

	fill_buf_num_blks = CONFIG_IMAGE_SPARSE_FILLBUF_SIZE / info->blksz;

	if (current_chunk == 0) {
		/* Read and skip over sparse image header */
		sparse_header = (sparse_header_t *) data;

		data += sparse_header->file_hdr_sz;
		if (sparse_header->file_hdr_sz > sizeof(sparse_header_t)) {
			/*
			 * Skip the remaining bytes in a header that is longer than
			 * we expected.
			 */
			data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
		}

		printf("=== Sparse Image Header ===\n");
		printf("magic: 0x%x\n", sparse_header->magic);
		printf("major_version: 0x%x\n", sparse_header->major_version);
		printf("minor_version: 0x%x\n", sparse_header->minor_version);
		printf("file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
		printf("chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
		printf("blk_sz: %d\n", sparse_header->blk_sz);
		printf("total_blks: %d\n", sparse_header->total_blks);
		printf("total_chunks: %d\n", sparse_header->total_chunks);
		/*
		 * Verify that the sparse block size is a multiple of our
		 * storage backend block size
		 */
		div_u64_rem(sparse_header->blk_sz, info->blksz, &offset);
		if (offset) {
			printf("%s: Sparse image block size issue [%u]\n", __FUNCTION__, sparse_header->blk_sz);
			return -1;
		}

		printf("Flashing Sparse Image\n");

		/* Start processing chunks */
		blk = info->start;
		total_blocks = 0;

		memcpy((void *)&sparse_header_info, (void *)sparse_header, sizeof(sparse_header_t));
	}
	sparse_header = &sparse_header_info;

	for (chunk = current_chunk; chunk < sparse_header->total_chunks; chunk++) {
		/* printf("%s: sparse buffer start addr 0x%X, end addr 0x%X, %dst chunk buffer header addr 0x%X, data addr 0x%X, 0x%x,total_sz:0x%x\r\n",
		   __FUNCTION__, dl_buf_addr, (dl_buf_addr + dl_buf_size -1), chunk, (u32)data, (u32)(data + sizeof(chunk_header_t)),*(u32 *)(data + 32),*(u32 *)(data + 64)); */

		data_addr = (ulong) data;
		if ((data_addr + sizeof(chunk_header_t)) > (dl_buf_addr + dl_buf_size)) {
			printf("need memmove, data addr 0x%llX, sizeof(chunk_header_t) 0x%lX\n", (u64) data,
			       sizeof(chunk_header_t));
			current_chunk = chunk;
			break;
		}
		/* Read and skip over chunk header */
		chunk_header = (chunk_header_t *) data;
		if (chunk_header->total_sz > dl_buf_size) {
			printf("chunk %d size 0x%x over sparse buffer size 0x%lx\n", chunk, chunk_header->total_sz,
			       dl_buf_size);
			return -1;
		}
		if ((data_addr + chunk_header->total_sz) > (dl_buf_addr + dl_buf_size)) {
			chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
			printf("need memmove, total_sz 0x%X, chunk_hdr_sz 0x%X, chunk_data_sz 0x%X\n",
			       chunk_header->total_sz, sparse_header->chunk_hdr_sz, chunk_data_sz);
			current_chunk = chunk;
			break;
		}
		data += sizeof(chunk_header_t);

		if (chunk_header->chunk_type != CHUNK_TYPE_RAW) {
			printf("=== Chunk Header ===\n");
			printf("chunk_type: 0x%x\n", chunk_header->chunk_type);
			printf("chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
			printf("total_size: 0x%x\n", chunk_header->total_sz);
		}

		if (sparse_header->chunk_hdr_sz > sizeof(chunk_header_t)) {
			/*
			 * Skip the remaining bytes in a header that is longer
			 * than we expected.
			 */
			data += (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		blkcnt = chunk_data_sz / info->blksz;
		switch (chunk_header->chunk_type) {
		case CHUNK_TYPE_RAW:
			if (chunk_header->total_sz != (sparse_header->chunk_hdr_sz + chunk_data_sz)) {
				printf
				    ("Bogus chunk size for chunk type Raw, total_sz 0x%X, chunk_hdr_sz 0x%X, chunk_data_sz 0x%X\n",
				     chunk_header->total_sz, sparse_header->chunk_hdr_sz, chunk_data_sz);
				return -1;
			}

			if (blk + blkcnt > info->start + info->size) {
				printf("Request would exceed partition size!\n");
				return -1;
			}

			bytes =
			    common_raw_write(part_name, (u64) (blk * info->blksz), (u64) (blkcnt * info->blksz),
					     (char *)data);
			if (bytes % info->blksz) {
				printf("%s: Write block # %llu [ %llu ] error, write bytes %llu\n", __FUNCTION__, blk,
				       bytes / info->blksz, bytes);
				return -1;
			}
			blks = bytes / info->blksz;
			/* blks might be > blkcnt (eg. NAND bad-blocks) */
			if (blks < blkcnt) {
				printf("%s: Write failed, block # %llu [ %llu ]\n", __FUNCTION__, blk, blks);
				return -1;
			}
			blk += blks;
			bytes_written += blkcnt * info->blksz;
			total_blocks += chunk_header->chunk_sz;
			data += chunk_data_sz;
			break;

		case CHUNK_TYPE_FILL:
			if (chunk_header->total_sz != (sparse_header->chunk_hdr_sz + sizeof(uint32_t))) {
				printf("Bogus chunk size for chunk type FILL\n");
				return -1;
			}

			fill_buf =
			    (u32 *) memalign(ARCH_DMA_MINALIGN,
					     ROUNDUP(info->blksz * fill_buf_num_blks, ARCH_DMA_MINALIGN));
			if (!fill_buf) {
				printf("Malloc failed for: CHUNK_TYPE_FILL\n");
				return -1;
			}

			fill_val = *((u32 *) data);
			data = (char *)data + sizeof(u32);

			for (i = 0; i < (info->blksz * fill_buf_num_blks / sizeof(fill_val)); i++)
				fill_buf[i] = fill_val;

			if (blk + blkcnt > info->start + info->size) {
				printf("%s: Request would exceed partition size!\n", __FUNCTION__);
				return -1;
			}

			for (i = 0; i < blkcnt;) {
				j = blkcnt - i;
				if (j > fill_buf_num_blks)
					j = fill_buf_num_blks;
				bytes =
				    common_raw_write(part_name, (u64) (blk * info->blksz), (u64) (j * info->blksz),
						     (char *)fill_buf);
				if (bytes % info->blksz) {
					printf("%s: Write block # %llu [ %llu ] error, write bytes %llu\n",
					       __FUNCTION__, blk, bytes / info->blksz, bytes);
					return -1;
				}
				blks = bytes / info->blksz;
				/* blks might be > j (eg. NAND bad-blocks) */
				if (blks < j) {
					printf("%s: Write failed, block # %llu [%d]\n", __FUNCTION__, blk, j);
					free(fill_buf);
					return -1;
				}
				blk += blks;
				i += j;
			}
			bytes_written += blkcnt * info->blksz;
			total_blocks += chunk_data_sz / sparse_header->blk_sz;
			free(fill_buf);
			break;

		case CHUNK_TYPE_DONT_CARE:
			blk += blkcnt;
			total_blocks += chunk_header->chunk_sz;
			break;

		case CHUNK_TYPE_CRC32:
			if (chunk_header->total_sz != sparse_header->chunk_hdr_sz) {
				printf("Bogus chunk size for chunk type Dont Care\n");
				return -1;
			}
			total_blocks += chunk_header->chunk_sz;
			data += chunk_data_sz;
			break;

		default:
			printf("%s: Unknown chunk type: %x\n", __FUNCTION__, chunk_header->chunk_type);
			return -1;
		}
	}

	if (chunk == sparse_header->total_chunks) {
		printf("Wrote %lld blocks, expected to write %d blocks\n", total_blocks, sparse_header->total_blks);
		printf("........ wrote %llu bytes to '%s'\n", bytes_written, part_name);

		if (total_blocks != sparse_header->total_blks) {
			printf("sparse image write failure\n");
			return -1;
		}
		printf("part %s sparse image write completed\n", part_name);
		current_chunk = 0;
	} else {
		*response = (ulong) data;
	}
	return 0;
}

struct cmdline_subpart {
	char name[32];		/* partition name, such as 'rootfs' */
	u64 from;
	u64 size;
	int flags;
	struct cmdline_subpart *next_subpart;
};

struct cmdline_parts {
	char name[32];		/* block device, such as 'mmcblk0' */
	unsigned int nr_subparts;
	struct cmdline_subpart *subpart;
	struct cmdline_parts *next_parts;
};

#define PF_RDONLY                   0x01	/* Device is read only */
#define PF_POWERUP_LOCK             0x02	/* Always locked after reset */

static void free_subpart(struct cmdline_parts *parts)
{
	struct cmdline_subpart *subpart;

	while (parts->subpart) {
		subpart = parts->subpart;
		parts->subpart = subpart->next_subpart;
		kfree(subpart);
	}
}

static unsigned long long memparse(const char *ptr, char **retptr)
{
	char *endptr;		/* local pointer to end of parsed string */

	unsigned long long ret = simple_strtoull(ptr, &endptr, 0);

	switch (*endptr) {
	case 'E':
	case 'e':
		ret <<= 10;
	case 'P':
	case 'p':
		ret <<= 10;
	case 'T':
	case 't':
		ret <<= 10;
	case 'G':
	case 'g':
		ret <<= 10;
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		endptr++;
	default:
		break;
	}

	if (retptr)
		*retptr = endptr;

	return ret;
}

static int parse_subpart(struct cmdline_subpart **subpart, char *partdef)
{
	int ret = 0;
	struct cmdline_subpart *new_subpart;

	*subpart = NULL;

	new_subpart = kzalloc(sizeof(struct cmdline_subpart), GFP_KERNEL);
	if (!new_subpart)
		return -ENOMEM;

	if (*partdef == '-') {
		new_subpart->size = (u64) (~0ULL);
		partdef++;
	} else {
		new_subpart->size = (u64) memparse(partdef, &partdef);
		if (new_subpart->size < (u64) PAGE_SIZE) {
			pr_warn("cmdline partition size is invalid.");
			ret = -EINVAL;
			goto fail;
		}
	}

	if (*partdef == '@') {
		partdef++;
		new_subpart->from = (u64) memparse(partdef, &partdef);
	} else {
		new_subpart->from = (u64) (~0ULL);
	}

	if (*partdef == '(') {
		int length;
		char *next = strchr(++partdef, ')');

		if (!next) {
			pr_warn("cmdline partition format is invalid.");
			ret = -EINVAL;
			goto fail;
		}

		length = min_t(int, next - partdef, sizeof(new_subpart->name) - 1);
		strncpy(new_subpart->name, partdef, length);
		new_subpart->name[length] = '\0';

		partdef = ++next;
	} else
		new_subpart->name[0] = '\0';

	new_subpart->flags = 0;

	if (!strncmp(partdef, "ro", 2)) {
		new_subpart->flags |= PF_RDONLY;
		partdef += 2;
	}

	if (!strncmp(partdef, "lk", 2)) {
		new_subpart->flags |= PF_POWERUP_LOCK;
		partdef += 2;
	}

	*subpart = new_subpart;
	return 0;
fail:
	kfree(new_subpart);
	return ret;
}

static int parse_parts(struct cmdline_parts **parts, const char *bdevdef)
{
	int ret = -EINVAL;
	char *next;
	int length;
	struct cmdline_subpart **next_subpart;
	struct cmdline_parts *newparts;
	char buf[32 + 32 + 4];

	*parts = NULL;

	newparts = kzalloc(sizeof(struct cmdline_parts), GFP_KERNEL);
	if (!newparts)
		return -ENOMEM;

	next = strchr(bdevdef, ':');
	if (!next) {
		pr_warn("cmdline partition has no block device.");
		goto fail;
	}

	length = min_t(int, next - bdevdef, sizeof(newparts->name) - 1);
	strncpy(newparts->name, bdevdef, length);
	newparts->name[length] = '\0';
	newparts->nr_subparts = 0;

	next_subpart = &newparts->subpart;

	while (next && *(++next)) {
		bdevdef = next;
		next = strchr(bdevdef, ',');

		length = (!next) ? (sizeof(buf) - 1) : min_t(int, next - bdevdef, sizeof(buf) - 1);

		strncpy(buf, bdevdef, length);
		buf[length] = '\0';

		/* printf("King:parse_parts:buf = %s\n",buf); */
		ret = parse_subpart(next_subpart, buf);
		if (ret)
			goto fail;

		newparts->nr_subparts++;
		next_subpart = &(*next_subpart)->next_subpart;
	}

	if (!newparts->subpart) {
		pr_warn("cmdline partition has no valid partition.");
		ret = -EINVAL;
		goto fail;
	}

	*parts = newparts;

	return 0;
fail:
	free_subpart(newparts);
	kfree(newparts);
	return ret;
}

int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t * info)
{
	char *mmc_parts;
	struct cmdline_parts *parts = NULL;
	struct cmdline_subpart *subpart = NULL;
	lbaint_t part_start_blk = 0;
#if CONFIG_IS_ENABLED(BLK)
	struct mmc_uclass_priv *upriv;
	struct mmc *mmc;
	loff_t off = 0;
#endif
	char *bootargs = NULL;

	struct boot_mode_info *const boot_info = (void *)BOOT_MODE_INFO_ADDR;

	if (boot_info->magic != BOOT_MODE_ENV_MAGIC) {
		printf("boot_mode magic error\n");
		return -1;
	}

	bootargs = env_get("bootargs");
	if(NULL == bootargs) {
		printf("get_part_info part: %s, bootargs not found in env, will use default\n", name);
		bootargs = BOOTARGS_EMMC;
		env_set("bootargs", bootargs);
	}

#if CONFIG_IS_ENABLED(BLK)
	if (str2off(name, &off)) {
		upriv = dev_desc->bdev->parent->uclass_priv;
		mmc = upriv->mmc;
		info->start = (lbaint_t) off / dev_desc->blksz;
		info->size = (lbaint_t) (mmc->capacity_user - off) / dev_desc->blksz;
		info->blksz = dev_desc->blksz;
		return info->size > 0 ? 0 : -1;
	}
#endif

	mmc_parts = strstr(bootargs , "blkdevparts");

	if(mmc_parts != NULL)
	{
		/*printf("mmc_parts: %s\n",mmc_parts);*/
		parse_parts(&parts, mmc_parts);
	}

	for (subpart = parts->subpart; subpart;subpart = subpart->next_subpart)
	{
		if(subpart == NULL)
			break;

		if (!strcmp(name, subpart->name))
		{
			info->start = part_start_blk;
			info->size = subpart->size / 512;
			info->blksz = dev_desc->blksz;
			/*printf("get_part_info name:%s info->start = %ld,info->size = %ld,dev_desc->blksz =  %ld\n",subpart->name,info->start,info->size,dev_desc->blksz); */
			return 0;
		}
		part_start_blk += subpart->size / 512;	//get partition start blk number
	}
	return -1;
}
