#include "spinor.h"
#include "dw_spi.h"
#include "timer.h"

static int spinor_initialized[SPI_CS_NUM] = {0};
static int spinor_qmode = 0;
int spinor_qe_cfg_parsred = 0;
int spinor_qe_rdsr = 0x35;
int spinor_qe_wrsr = 0x31;
int spinor_qe_bit = 1;
u32 spinor_rx_sample_delay[3] = {0};
u32 spinor_phy_setting[3] = {0};

extern u8 curr_cs;

static int spinor_read_reg(u8 opcode, u8 *val, int len)
{
	struct spi_mem_op op = SPINOR_READ_REG_OP(opcode, len, val);
	return spi_mem_exec_op(&op);
}

static int spinor_write_reg(u8 opcode, u8 *buf, int len)
{
	struct spi_mem_op op = SPINOR_WRITE_REG_OP(opcode, len, buf);
	return spi_mem_exec_op(&op);
}

static int read_sr(u8 opcode, void *val)
{
	return spinor_read_reg(opcode, val, 1);
}

static int write_sr(u8 opcode, u8 val)
{
	return spinor_write_reg(opcode, &val, 1);
}

#define SPI_NOR_WRITE_ENABLE_FOR_VOLATILE_STATUS_REGISTER
#ifdef SPI_NOR_WRITE_ENABLE_FOR_VOLATILE_STATUS_REGISTER
static int write_enable_volatile()
{
	return spinor_write_reg(SPINOR_OP_WRENVSR, NULL, 0);
}
#else
static int write_enable()
{
	return spinor_write_reg(SPINOR_OP_WREN, NULL, 0);
}
#endif

static int spinor_wait_till_ready()
{
	unsigned long timeout = 100000; //MAX = 1600ms for 32K blk erase
	u8 sr = 0;
	int ret;

	while (timeout--) {
		ret = read_sr(SPINOR_OP_RDSR, &sr);
		if (ret < 0)
			return ret;

		if(!(sr & SR_WIP))
			break;
	}

	if(timeout < 0)
		return -1;

	return 0;
}

static int spinor_read_data(u32 addr, u32 len, u8 *buf)
{
	struct spi_mem_op op = SPINOR_READ_DATA_OP(spinor_qmode, addr, buf, len);
	u32 remaining = len;
	int ret;

	while (remaining) {
		op.data.nbytes = remaining;
		ret = spi_mem_adjust_op_size(&op);
		if (ret)
			return ret;

		ret = spi_mem_exec_op(&op);
		if (ret)
		{
			return ret;
		}

		op.addr.val += op.data.nbytes;
		remaining -= op.data.nbytes;
		op.data.buf.in += op.data.nbytes;
	}

	return len;
}

#if 0
static int write_disable()
{
	return spinor_write_reg(SPINOR_OP_WRDI, NULL, 0);
}

static int spinor_write_data(u32 addr, u32 len, const u8 *buf)
{
	struct spi_mem_op op = SPINOR_PROG_LOAD(spinor_qmode, addr, buf, len);
	int ret;

	ret = spi_mem_adjust_op_size(&op);
	if (ret)
		return ret;
	op.data.nbytes = len < op.data.nbytes ? len : op.data.nbytes;

	ret = spi_mem_exec_op(&op);
	if (ret)
		return ret;

	return op.data.nbytes;
}

static int spinor_erase_sector(u32 addr)
{
	struct spi_mem_op op = SPINOR_BLK_ERASE_OP(addr);
	return spi_mem_exec_op(&op);
}

int spinor_erase(u32 addr, int nsec_size)
{
	int ret;

	while (nsec_size > 0) {
		write_enable();

		ret = spinor_erase_sector(addr);
		if (ret)
			goto erase_err;

		addr += SPINOR_ERASE_SIZE;
		nsec_size -= SPINOR_ERASE_SIZE;

		ret = spinor_wait_till_ready();
		if (ret)
			goto erase_err;
	}

erase_err:
	write_disable();

	return ret;
}

static int spinor_read_id(u8 *buf)
{
	struct spi_mem_op op = SPINOR_READID_OP(buf, SPINOR_MAX_ID_LEN);
	return spi_mem_exec_op(&op);
}

static int spinor_unlock_block()
{
	int ret;
	u8 val;

	ret = read_sr(SPINOR_OP_RDSR, &val);
	if (ret < 0)
		return ret;

	write_enable();
	val &= ~(SR_BP0 | SR_BP1 | SR_BP2 | SR_TB);
	write_sr(SPINOR_OP_WRSR, val);

	ret = spinor_wait_till_ready();
	if (ret)
		return ret;

	return 0;
}

int spinor_write(u32 addr, u32 len, const u8 *buf)
{
	u32 page_offset, page_remain, i;
	int ret;

	for (i = 0; i < len; ) {
		int written;
		u32 offset = addr + i;

		page_offset = offset & (SPINOR_PAGE_SIZE - 1);
		/* the size of data remaining on the first page */
		page_remain = (SPINOR_PAGE_SIZE - page_offset) < (len - i) ? \
		(SPINOR_PAGE_SIZE - page_offset) : (len - i);

		write_enable();
		ret = spinor_write_data(offset, page_remain, buf + i);
		if (ret < 0)
			goto write_err;
		written = ret;

		ret = spinor_wait_till_ready();
		if (ret < 0)
			goto write_err;
		i += written;
	}

write_err:
	return ret;
}
#endif
static int spinor_reset()
{
	int ret;
	ret = spinor_write_reg(SPINOR_OP_RSTEN, NULL, 0);
	if (ret < 0)
		return ret;
	ret = spinor_write_reg(SPINOR_OP_RESET, NULL, 0);
	if (ret < 0)
		return ret;

	/* bug fix: RESET cmd needs an recover time */
	udelay(100);

	return 0;
}

int spinor_quad_enable(u8 enable)
{
	int ret;
	u8 val;

	if (!spinor_qe_cfg_parsred)
		return 0;

	spinor_qmode =  (enable ? 1 : 0);
	ret = read_sr(spinor_qe_rdsr, &val);
	if (ret < 0)
		return ret;

	if (enable) {
		if (val & (1 << spinor_qe_bit))
			return 0;
		val |= 1 << spinor_qe_bit;
	} else {
		if (!(val & (1 << spinor_qe_bit)))
			return 0;
		val &= ~(1 << spinor_qe_bit);
	}

#ifdef SPI_NOR_WRITE_ENABLE_FOR_VOLATILE_STATUS_REGISTER
	write_enable_volatile();
#else
	write_enable();
#endif
	write_sr(spinor_qe_wrsr, val);
	ret = spinor_wait_till_ready();
	if (ret)
		return ret;

	return 0;
}

int spinor_read(u32 addr, u32 len, u8 *buf)
{
	return spinor_read_data(addr, len, buf);
}

int spinor_init(u32 clk, u32 buswidth)
{
	int ret;
	//u8 id[3];
	u32 quad_en = (buswidth == 4) ? 1 : 0;

	if (spinor_initialized[curr_cs])
		return 0;

	switch (clk) {
	case SPINOR_DEFAULT_CLK:
	default:
		ssi_rx_sample_delay = spinor_rx_sample_delay[0];
		ssi_phy_setting = spinor_phy_setting[0];
		break;

	case SPINOR_25M_CLK:
		ssi_rx_sample_delay = spinor_rx_sample_delay[1];
		ssi_phy_setting = spinor_phy_setting[1];
		break;

	case SPINOR_50M_CLK:
		ssi_rx_sample_delay = spinor_rx_sample_delay[2];
		ssi_phy_setting = spinor_phy_setting[2];
		break;
	}
	spi_hw_init(clk);

	ret = spinor_reset();
	if (ret < 0)
	{
		return ret;
	}
#if 0
	ret = spinor_read_id(id);
	if (ret < 0)
		return ret;

	ret = spinor_unlock_block();
	if (ret < 0)
		return ret;
#endif

	if (quad_en) {
		ret = spinor_quad_enable(quad_en);
		if (ret < 0)
		{
			return ret;
		}
	}
	current_bus_width = buswidth;
	current_clock = clk;

	switch (clk) {
	case SPINOR_DEFAULT_CLK:
	default:
		break;

	case SPINOR_25M_CLK:
	case SPINOR_50M_CLK:
		break;
	}
	spinor_initialized[curr_cs] = 1;

	return 0;
}

