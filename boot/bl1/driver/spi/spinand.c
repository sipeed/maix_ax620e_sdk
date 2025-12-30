#include "spinand.h"
#include "dw_spi.h"
#include "timer.h"
#include "printf.h"

/* nand org info, will be updated during boot */
int spinand_page_size_shift = 11; //2KB
int spinand_blk_size_shift = 17; //128KB
int spinand_oob_size_shift = 6; //64Byte
int spinand_oob_len = 1;
int spinand_planes = 2;
int spinand_eraseblock_no;
u32 spinand_rx_sample_delay[3] = {0};
u32 spinand_phy_setting[3] = {0};
static int spinand_qmode = 0;
#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
static int spinand_bufmode = 1;
u8 spinand_continuous_read_support = 0;
#endif
static int spinand_initialized[SPI_CS_NUM] = {0};
extern u8 curr_cs;
extern u8 spi_cmd_addr_dummy_cs_bypass;	//0: dis, 1: 13 & 0f opcode, 2: 6b opcode, 3: continuous data only

static int spinand_read_reg_op(u8 reg, u8 *val)
{
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(reg, val);
	return spi_mem_exec_op(&op);
}

static int spinand_write_reg_op(u8 reg, u8 val)
{
	struct spi_mem_op op = SPINAND_SET_FEATURE_OP(reg, &val);
	return spi_mem_exec_op(&op);
}

static int spinand_read_status(u8 *status)
{
	return spinand_read_reg_op(REG_STATUS, status);
}

int spinand_upd_cfg(u8 mask, u8 val)
{
	int ret;
	u8 cfg;

	ret = spinand_read_reg_op(REG_CFG, &cfg);
	if (ret)
		return ret;

	cfg &= ~mask;
	cfg |= val;

	ret = spinand_write_reg_op(REG_CFG, cfg);
	if (ret)
		return ret;

	return 0;
}

static int spinand_load_page_op(u32 row)
{
	struct spi_mem_op op = SPINAND_PAGE_READ_OP(row);
	return spi_mem_exec_op(&op);
}

static int spinand_fls(int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

static void spinand_cache_op_adjust_colum(u32 *column)
{
	unsigned int shift;

	if (spinand_planes < 2)
		return;

	/* The plane number is passed in MSB just above the column address */
	shift = spinand_fls(1 << spinand_page_size_shift);
	*column |= ((spinand_eraseblock_no % spinand_planes) << shift);
	//info("shift=%d, col=0x%x\r\n", shift, *column);
}

static int spinand_read_from_cache_op(u8 *pbuf, u32 column, u32 nbytes)
{
	struct spi_mem_op op = SPINAND_PAGE_READ_FROM_CACHE_OP(spinand_qmode, 0, 1, NULL, 0);
	int ret;

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
	if (spinand_continuous_read_support && !spinand_bufmode) {
		op.addr.nbytes = 0;
		op.addr.val = 0;
		op.addr.buswidth = 0;
		if (spi_cmd_addr_dummy_cs_bypass == 1) {
			spi_cmd_addr_dummy_cs_bypass++;
			info("\r\nspi_cmd_addr_dummy_cs_bypass = %d\n", spi_cmd_addr_dummy_cs_bypass);
			op.dummy.nbytes = 4;
			op.dummy.buswidth = 1;
		}
		else {
			op.dummy.nbytes = 0;
			op.dummy.buswidth = 0;
		}
	}
	else
#endif
	{
		spinand_cache_op_adjust_colum(&column);
		op.addr.val = column;
	}
	/*
	 * Some controllers are limited in term of max RX data size. In this
	 * case, just repeat the READ_CACHE operation after updating the
	 * column.
	 */
	while (nbytes) {
		op.data.buf.in = pbuf;
		op.data.nbytes = nbytes;
		ret = spi_mem_adjust_op_size(&op);
		if (ret)
		{
			err("\r\nspi_mem_adjust_op_size fail, op.data.nbytes=%d\n", op.data.nbytes);
			return ret;
		}

		ret = spi_mem_exec_op(&op);
		if (ret)
		{
			err("\r\nspi_mem_exec_op fail, ret = %d\n", ret);
			return ret;
		}

		pbuf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;
#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
		if (spinand_continuous_read_support && !spinand_bufmode) {
			if (spi_cmd_addr_dummy_cs_bypass == 2) {
				spi_cmd_addr_dummy_cs_bypass++;
				info("\r\nspi_cmd_addr_dummy_cs_bypass = %d\n", spi_cmd_addr_dummy_cs_bypass);
			}
		}
#endif
	}

	return 0;
}

static int spinand_wait(u8 *s)
{
	u8 status;
	int ret;

	u32 timeout = 10000; //MAX = 600us for page program(with ECC)
	do {
		ret = spinand_read_status(&status);
		if (ret)
			return ret;

		if (!(status & STATUS_BUSY))
			goto out;
	} while (timeout-- > 0);

	/*
	 * Extra read, just in case the STATUS_READY bit has changed
	 * since our last check
	 */
	ret = spinand_read_status(&status);
	if (ret)
		return ret;

out:
	if (s)
		*s = status;

	return status & STATUS_BUSY ? -1 : 0;
}

static int spinand_reset_op(void)
{
	struct spi_mem_op op = SPINAND_RESET_OP;
	int ret;

	ret = spi_mem_exec_op(&op);
	if (ret)
		return ret;
	udelay(1000);

	return spinand_wait(NULL);
}

static int spinand_check_ecc_status(u8 status)
{
	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
	case STATUS_ECC_HAS_BITFLIPS:
		return 0;

	case STATUS_ECC_UNCOR_ERROR:
		return -1;

	default:
		break;
	}

	return -1;
}

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
static int spinand_read_id_op(u8 *pbuf)
{
	struct spi_mem_op op = SPINAND_READID_OP(1, pbuf, SPINAND_MAX_ID_LEN);
	return spi_mem_exec_op(&op);
}
#endif

#if SPINAND_DEBUG_ENABLE
static int spinand_lock_block(u8 lock)
{
	return spinand_write_reg_op(REG_BLOCK_LOCK, lock);
}

static int spinand_ecc_enable(u8 enable)
{
	return spinand_upd_cfg(CFG_ECC_ENABLE, enable ? CFG_ECC_ENABLE : 0);
}

static int spinand_write_enable_op(void)
{
	struct spi_mem_op op = SPINAND_WR_EN_DIS_OP(1);
	return spi_mem_exec_op(&op);
}

static int spinand_write_to_cache_op(u8 *pbuf, u32 column, u32 nbytes)
{
	struct spi_mem_op op = SPINAND_PROG_LOAD(spinand_qmode, 0, NULL, 0);
	int ret;

	spinand_cache_op_adjust_colum(&column);
	op.addr.val = column;
	/*
	 * Some controllers are limited in term of max TX data size. In this
	 * case, split the operation into one LOAD CACHE and one or more
	 * LOAD RANDOM CACHE.
	 */
	while (nbytes) {
		op.data.buf.out = pbuf;
		op.data.nbytes = nbytes;

		ret = spi_mem_adjust_op_size(&op);
		if (ret)
			return ret;

		ret = spi_mem_exec_op(&op);
		if (ret)
			return ret;

		pbuf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;
	}

	return 0;
}

static int spinand_program_op(u32 row)
{
	struct spi_mem_op op = SPINAND_PROG_EXEC_OP(row);

	return spi_mem_exec_op(&op);
}

static int spinand_erase_op(u32 addr)
{
	unsigned int row = addr >> spinand_page_size_shift;
	struct spi_mem_op op = SPINAND_BLK_ERASE_OP(row);

	return spi_mem_exec_op(&op);
}

static int spinand_write_page(u32 addr, u8 *pbuf, u32 nbytes, u8 oob_op)
{
	u8 status;
	int ret;
	u32 row = addr >> spinand_page_size_shift;
	u32 column = oob_op ? (1 << spinand_page_size_shift) : \
		(addr & ((1 << spinand_page_size_shift) - 1));

	if (oob_op && nbytes > (1 << spinand_oob_size_shift))
		return -1;

	if (!oob_op && (column + nbytes) > (1 << spinand_page_size_shift))
		return -1;

	if (nbytes < ((1 << spinand_page_size_shift) + (1 << spinand_oob_size_shift))) {
		ret = spinand_load_page_op(row);
		if (ret)
			return ret;

		ret = spinand_wait(&status);
		if (ret < 0)
			return ret;

		ret = spinand_check_ecc_status(status);
		if (ret < 0)
			return ret;
	}

	ret = spinand_write_enable_op();
	if (ret)
		return ret;

	ret = spinand_write_to_cache_op(pbuf, column, nbytes);
	if (ret)
		return ret;

	ret = spinand_program_op(row);
	if (ret)
		return ret;

	ret = spinand_wait(&status);
	if (!ret && (status & STATUS_PROG_FAILED))
		ret = -1;

	return ret;
}

static int spinand_write_blk(u32 addr, u32 nbytes, char *pbuf)
{
	u32 page_offset, page_remain, i;
	u8 oob_op = 0;
	int ret;

	spinand_eraseblock_no = addr >> spinand_blk_size_shift;
	//write pages within one block
	for (i = 0; i < nbytes; ) {
		u32 offset = addr + i;

		page_offset = offset & ((1 << spinand_page_size_shift) - 1);
		/* the size of data remaining on the first page */
		page_remain = ((1 << spinand_page_size_shift) - page_offset) < (nbytes - i) ? \
		((1 << spinand_page_size_shift) - page_offset) : (nbytes - i);

		ret = spinand_write_page(offset, (u8 *)pbuf + i, page_remain, oob_op);
		if (ret < 0)
			return ret;

		i += page_remain;
	}

	return 0;
}

int spinand_block_markbad(u32 addr)
{
	int ret;
	u8 oob_op = 1;
	u8 oob_buf[2];
	addr &= ~((1 << spinand_blk_size_shift) - 1);

	for (int i = 0; i < sizeof(oob_buf); i++) {
		oob_buf[i] = 0xaa;
	}

	ret = spinand_write_page(addr, oob_buf, spinand_oob_len, oob_op);
	if (ret)
		return ret;

	return 0;
}
#endif

static int spinand_read_page(u32 addr, u8 *pbuf, u32 nbytes, u8 oob_op)
{
	int ret;
	u8 status;
	u32 row = addr >> spinand_page_size_shift;
	u32 column = oob_op ? (1 << spinand_page_size_shift) : \
		(addr & ((1 << spinand_page_size_shift) - 1));

	if (oob_op && nbytes > (1 << spinand_oob_size_shift))
	{
		err("\r\noob_op = %d, nbytes = 0x%x, oob_shift=%d\n", oob_op, nbytes, spinand_oob_size_shift);
		return -1;
	}
#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
	if (oob_op && !spinand_bufmode) {
		err("\r\noob_op=%d not support BUF=%d\n", oob_op, spinand_bufmode);
		return -1;
	}
#endif

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
	if (spinand_bufmode)
#endif
	{
	if (!oob_op && (column + nbytes) > (1 << spinand_page_size_shift))
		return -1;
	}

	if (spi_cmd_addr_dummy_cs_bypass <= 1) {
	ret = spinand_load_page_op(row);
	if (ret)
	{
		err("\r\nspinand_load_page_op fail, row = 0x%x\n", row);
		return ret;
	}

	ret = spinand_wait(&status);
	if (ret < 0)
	{
		err("\r\nspinand_wait fail, status=0x%x\n", status);
		return ret;
	}
	}

	ret = spinand_read_from_cache_op(pbuf, column, nbytes);
	if (ret)
	{
		err("\r\nspinand_read_from_cache_op fail, column=0x%x, nbytes=0x%x, ram=0x%x\n", column, nbytes, (u32)pbuf);
		return ret;
	}

	return spinand_check_ecc_status(status);
}

int spinand_block_isbad(u32 addr)
{
	int ret;
	u8 oob_op = 1;
	u8 oob_buf[64];
	addr &= ~((1 << spinand_blk_size_shift) - 1);

	spinand_eraseblock_no = addr >> spinand_blk_size_shift;
	ret = spinand_read_page(addr, oob_buf, (1 << spinand_oob_size_shift), oob_op);
	if (ret)
		return ret;

	for (int i = 0; i < spinand_oob_len; i++) {
		if(oob_buf[i] != 0xff)
			return 1;
	}

	return 0;
}

#if SPINAND_DEBUG_ENABLE
int spinand_write(u32 addr, u32 nbytes, char *pbuf)
{
	u32 blk_offset, blk_remain, i;
	int ret;

	for (i = 0; i < nbytes; ) {
		u32 offset = addr + i;
#if SPINAND_BB_SKIPPED
		/* make sure we start from the good blk(logic blk) each time */
		int phy_off = spinand_L2P(offset);
		if (phy_off < 0)
			return -1;
#else
		int phy_off = offset;
#endif
		blk_offset = phy_off & ((1 << spinand_blk_size_shift) - 1);
		/* the size of data remaining on the first block */
		blk_remain = ((1 << spinand_blk_size_shift) - blk_offset) < (nbytes - i) ? \
		((1 << spinand_blk_size_shift) - blk_offset) : (nbytes - i);

		ret = spinand_write_blk(phy_off, blk_remain, pbuf + i);
		if (ret < 0)
			return ret;

		i += blk_remain;
	}

	return 0;
}

int spinand_erase(u32 addr, int nblk_size)
{
	u8 status;
	int phy_addr;
	int ret;

	while (nblk_size > 0) {
#if SPINAND_BB_SKIPPED
		phy_addr = spinand_L2P(addr);
		if (phy_addr < 0)
			return -1;
#else
		phy_addr = addr;
#endif
		ret = spinand_write_enable_op();
		if (ret)
			return ret;

		ret = spinand_erase_op(phy_addr);
		if (ret)
			return ret;

		ret = spinand_wait(&status);
		if (!ret && (status & STATUS_ERASE_FAILED))
			return -1;

		addr += (1 << spinand_blk_size_shift);
		nblk_size -= (1 << spinand_blk_size_shift);
	}
	return 0;
}

static void ddr_set(u64 mem_addr, u32 len)
{
	int i;
	u32 * pbuf = (u32 *)mem_addr;

	for (i = 0; i < len / 4; i++) {
		pbuf[i] = i * 4;
	}
}

static int ddr_check(u64 mem_addr, u32 len)
{
	int ret = 0;
	u32 i;
	u32 * pbuf = (u32 *)mem_addr;

	for (i = 0; i < len / 4; i++) {
		if ((i * 4) != pbuf[i]) {
			info("i=%d, mem_addr=0x%llx, val=0x%x\r\n", i, (u64)&pbuf[i], pbuf[i]);
			ret = -1;
			break;
		}
	}
	return ret;
}

static int ddr_cmp(u64 src, u64 dst, u32 len)
{
	int ret = 0;
	u32 i;
	u32 * psrcbuf = (u32 *)src;
	u32 * pdstbuf = (u32 *)dst;

	for (i = 0; i < len / 4; i++) {
		if (psrcbuf[i] != pdstbuf[i]) {
			info("i=%d, src=[0x%llx, 0x%x], dst=[0x%llx, 0x%x]\r\n", i,
				(u64)&psrcbuf[i], psrcbuf[i], (u64)&pdstbuf[i], pdstbuf[i]);
			ret = -1;
			break;
		}
	}
	return ret;
}

#define SPL_HDR_MAGIC_OFF 0x4
#define SPL_HDR_CAP_OFF   0x8
#define SPL_HDR_MAGIC_VAL 0x55543322
#define SRC_MEM_ADDR 0x110000000
#define DST_MEM_ADDR 0x118000000
static int spinand_test(u64 src_mem_addr, u64 dst_mem_addr, u32 flash_addr, u32 len)
{
	int ret = -1;
	int spl_img_test = 0;
	int quad_en = 0;
	u32 tmp1, tmp2;
	info("%s: src_mem_addr=0x%llx, dst_mem_addr=0x%llx, flash_addr=0x%x, len=0x%x\r\n",
		__func__, src_mem_addr, dst_mem_addr, flash_addr, len);

	tmp1 = *((u32 *)(src_mem_addr + SPL_HDR_MAGIC_OFF));
	tmp2 = *((u32 *)(src_mem_addr + SPL_HDR_CAP_OFF));
	info("spl magic: addr 0x%llx, val 0x%x\r\n", (src_mem_addr + SPL_HDR_MAGIC_OFF), tmp1);
	info("spl cap: addr 0x%llx, val 0x%x\r\n", (src_mem_addr + SPL_HDR_CAP_OFF), tmp2);
	if (SPL_HDR_MAGIC_VAL == tmp1) {
		spl_img_test = 1;
		if (tmp2 & BIT(2))
			quad_en = 1;
	}
	else {
		spl_img_test = 0;
		if (tmp1)
			quad_en = 1;
	}

	if (quad_en) {
		ret = spinand_quad_enable(1);
		if (ret) {
			info("spinand_quad_enable ret=%d\r\n", ret);
			return ret;
		}
	}

	if (!spl_img_test)
		ddr_set(src_mem_addr, len);
	ax_memset((void *)dst_mem_addr, 0, len);

	spinand_erase(flash_addr, ((len + (0x20000 - 1)) & ~(0x20000 - 1)));
	spinand_write(flash_addr, len, (char *)src_mem_addr);
	spinand_read(flash_addr, len, (char *)dst_mem_addr);

	if (!spl_img_test)
		ret = ddr_check(dst_mem_addr, len);
	else
		ret = ddr_cmp(src_mem_addr, dst_mem_addr, len);
	if (0 == ret)
	{
		info("%s: %s x%d pass\r\n", __func__, spl_img_test ? "spl" : "rw", quad_en ? 4 : 1);
		udelay(10000000);
	}
	else
	{
		info("%s: %s x%d fail\r\n", __func__, spl_img_test ? "spl" : "rw", quad_en ? 4 : 1);
		udelay(10000000);
	}
	return ret;
}
#endif
static int spinand_read_blk(u32 addr, u32 nbytes, char *pbuf)
{
	u32 page_offset, page_remain, i;
	u8 oob_op = 0;
	int ret;

	spinand_eraseblock_no = addr >> spinand_blk_size_shift;
	//read pages within one block
	for (i = 0; i < nbytes; ) {
		u32 offset = addr + i;

		page_offset = offset & ((1 << spinand_page_size_shift) - 1);
		/* the size of data remaining on the first page */
#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
		if (!spinand_bufmode) {
			page_remain = (nbytes - i);
			if (!spi_cmd_addr_dummy_cs_bypass)
				spi_cmd_addr_dummy_cs_bypass = 1;
			info("\r\nspi_cmd_addr_dummy_cs_bypass = %d\n", spi_cmd_addr_dummy_cs_bypass);
		}
		else {
#endif
		page_remain = ((1 << spinand_page_size_shift) - page_offset) < (nbytes - i) ? \
		((1 << spinand_page_size_shift) - page_offset) : (nbytes - i);

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
			info("\r\nspi_cmd_addr_dummy_cs_bypass = %d\n", spi_cmd_addr_dummy_cs_bypass);
		}
#endif
		ret = spinand_read_page(offset, (u8 *)pbuf + i, page_remain, oob_op);
		if (ret < 0)
		{
			err("\r\nspinand_read_page fail: offset=0x%x, page_remain=0x%x, oob_op=%d, ram=0x%x\n",
				offset, page_remain, oob_op, (u32)(pbuf + i));
			return ret;
		}

		i += page_remain;
	}

	return 0;
}

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
#define SPINAND_BAD_INFO_SIZE	(20)
static int spinand_buf_enable(u8 enable)
{
	int ret;

	if (enable == spinand_bufmode)
		return 0;

	ret = spinand_upd_cfg(CFG_BUF_ENABLE, enable ? CFG_BUF_ENABLE : 0);
	if (ret) {
		err("\r\nmodify bufmode fail, current bufmode = %d, enable = %d\n", spinand_bufmode, enable);
		return ret;
	}
	spinand_bufmode = (enable ? 1 : 0);

	return ret;
}

static int spinand_continuous_read_support_check(void)
{
	int ret;
	u8 tmp;
#if defined (SPI_MST_USE_DMA)
	u8 id[SPINAND_MAX_ID_LEN];

	ret = spinand_read_id_op(id);
	if (ret) {
		err("\r\nspinand_read_id_op fail!\n");
		return ret;
	}
	info("\r\nJEDEC id bytes: %02x, %02x, %02x\n", id[0], id[1], id[2]);

	if ((0xEF == id[0]) && ((0xBF == id[1]) || (0xAB == id[1]))) {
		spinand_continuous_read_support = 1;
		info("\r\n===== spinand_continuous_read_support = %d =====\n",
			spinand_continuous_read_support);

		ret = spinand_read_reg_op(REG_CFG, &tmp);
		if (ret) {
			err("\r\nspinand_read_reg_op 0x%x fail!\n", REG_CFG);
			return ret;
		}
		spinand_bufmode = (tmp & CFG_BUF_ENABLE) ? 1 : 0;
		info("\r\nspinand_bufmode: %d, tmp %02x\n", spinand_bufmode, tmp);
	}
#else
	spinand_continuous_read_support = 0;
#endif

	return 0;
}

static int spinand_region_bad_block_scan(u32 addr, u32 nbytes, u16 * table, int table_size)
{
	int i = 0;
	int index = 0;
	int phy_off = addr;
	int blk_start, blk_end, valid_blks;

	blk_start = addr >> spinand_blk_size_shift;
	blk_end = (addr + nbytes - 1) >> spinand_blk_size_shift;
	valid_blks = blk_end - blk_start + 1;
	info("\r\nfrom addr 0x%x, size 0x%x, start blk %d, need scan %d valid blks\n", addr, nbytes, blk_start, valid_blks);

	while (i < valid_blks) {
		if (0 == spinand_block_isbad(phy_off)) {
			i++;
			info("\r\nblk %d is good\n", phy_off >> spinand_blk_size_shift);
		}
		else {
			if (index >= table_size) {
				err("\r\n%d bad block exeed limit %d\n", index + 1, table_size);
				return -1;
			}
			table[index++] = (phy_off >> spinand_blk_size_shift);
			info("\r\nblk %d is bad\n", phy_off >> spinand_blk_size_shift);
		}
		phy_off += (1 << spinand_blk_size_shift);
	}
	return index;
}
#endif
int spinand_read(u32 addr, u32 nbytes, char *pbuf)
{
	u32 blk_offset, blk_remain, i;
	int ret;
	int phy_off = addr;
#if SPINAND_BB_SKIPPED
#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
	int j = 0;
	int bad_blks = 0;
	int is_bad = 0;
	static u16 spinand_bad_info[SPINAND_BAD_INFO_SIZE] = {0};
#endif
#endif

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
	if (spinand_continuous_read_support) {
		if (!(addr & ((1 << spinand_page_size_shift) - 1)) && (nbytes > (1 << spinand_page_size_shift))) {
#if SPINAND_BB_SKIPPED
			spinand_buf_enable(1);
			bad_blks = spinand_region_bad_block_scan(addr, nbytes, spinand_bad_info, SPINAND_BAD_INFO_SIZE);
			if (bad_blks < 0)
			{
				err("\r\nscan too many bad blocks!\n");
				return -1;
			}

			while (j < bad_blks) {
				info("\r\nscan blk %d is bad, part total %d bad blocks\n", spinand_bad_info[j], bad_blks);
				j++;
			}
#endif
			spinand_buf_enable(0);
		}
		else
			spinand_buf_enable(1);
	}
#endif
	for (i = 0; i < nbytes; ) {
#if SPINAND_BB_SKIPPED

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
		if (spinand_continuous_read_support && !spinand_bufmode) {
			for (j = 0; j < bad_blks; j++) {
				if (spinand_bad_info[j] == (phy_off >> spinand_blk_size_shift)) {
					info("\r\nnand addr 0x%x, blk %d is bad\n", phy_off, phy_off / (1 << spinand_blk_size_shift));
					break;
				}
			}

			if (j < bad_blks) {
				is_bad = 1;
				spi_cmd_addr_dummy_cs_bypass = 1;
				external_cs_manage(1);
			} else
				is_bad = 0;
		}
		else
			is_bad = spinand_block_isbad(phy_off);

		if (is_bad) {
#else
		if (spinand_block_isbad(phy_off)) {
#endif
			info("\r\nnand addr 0x%x, blk %d is bad\n", phy_off, phy_off / (1 << spinand_blk_size_shift));
			phy_off += (1 << spinand_blk_size_shift);
			continue;
		}
#endif
		blk_offset = phy_off & ((1 << spinand_blk_size_shift) - 1);
		/* the size of data remaining on the first block */
		blk_remain = ((1 << spinand_blk_size_shift) - blk_offset) < (nbytes - i) ? \
		((1 << spinand_blk_size_shift) - blk_offset) : (nbytes - i);

		ret = spinand_read_blk(phy_off, blk_remain, pbuf + i);
		if (ret < 0)
		{
#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
			if (spinand_continuous_read_support && !spinand_bufmode) {
				spi_cmd_addr_dummy_cs_bypass = 0;
				external_cs_manage(1);
				spinand_buf_enable(1);
			}
#endif
			err("\r\nspinand_read_blk fail: phy_off=0x%x, blk_remain=0x%x, ram=0x%x\n", phy_off, blk_remain, (u32)(pbuf + i));
			return ret;
		}

		i += blk_remain;
		phy_off += blk_remain;
	}

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
	if (spinand_continuous_read_support && !spinand_bufmode) {
		spi_cmd_addr_dummy_cs_bypass = 0;
		external_cs_manage(1);
	}
#endif

	return 0;
}

int spinand_quad_enable(u8 enable)
{
	spinand_qmode =  (enable ? 1 : 0);
	return spinand_upd_cfg(CFG_QUAD_ENABLE, enable ? CFG_QUAD_ENABLE : 0);
}

int spinand_init(u32 clk, u32 buswidth)
{
	int ret;
	u32 quad_en = (buswidth == 4) ? 1 : 0;

	if (spinand_initialized[curr_cs])
		return 0;

	switch (clk) {
	case SPINAND_DEFAULT_CLK:
	default:
		ssi_rx_sample_delay = spinand_rx_sample_delay[0];
		ssi_phy_setting = spinand_phy_setting[0];
		break;

	case SPINAND_25M_CLK:
		ssi_rx_sample_delay = spinand_rx_sample_delay[1];
		ssi_phy_setting = spinand_phy_setting[1];
		break;

	case SPINAND_50M_CLK:
		ssi_rx_sample_delay = spinand_rx_sample_delay[2];
		ssi_phy_setting = spinand_phy_setting[2];
		break;
	}
	spi_hw_init(clk);

	ret = spinand_reset_op();
	if (ret)
	{
		return ret;
	}

#if SPINAND_DEBUG_ENABLE
	ret = spinand_upd_cfg(CFG_OTP_ENABLE, 0);
	if (ret)
		return ret;

	ret = spinand_ecc_enable(1);
	if (ret)
		return ret;

	ret = spinand_lock_block(BL_ALL_UNLOCKED);
	if (ret)
		return ret;

	spinand_test(SRC_MEM_ADDR, DST_MEM_ADDR, 0, 0x100000);
	while(1);
#endif

	if (quad_en) {
		ret = spinand_quad_enable(quad_en);
		if (ret)
		{
			return ret;
		}
	}

#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
	spinand_continuous_read_support_check();
#endif
	current_bus_width = buswidth;
	current_clock = clk;

	switch (clk) {
	case SPINAND_DEFAULT_CLK:
	default:
		break;

	case SPINAND_25M_CLK:
	case SPINAND_50M_CLK:
		break;
	}
	spinand_initialized[curr_cs] = 1;

	return 0;
}


