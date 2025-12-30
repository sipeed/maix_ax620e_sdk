#include "dmacfg.h"

#define AX_DMACFG_BASE           0x10440000

#define AX_DMACFG_START          (AX_DMACFG_BASE + 0x0)
#define AX_DMACFG_STA            (AX_DMACFG_BASE + 0x4)
#define AX_DMACFG_LLI_H          (AX_DMACFG_BASE + 0x8)
#define AX_DMACFG_LLI_L          (AX_DMACFG_BASE + 0xC)
#define AX_DMACFG_CLEAR          (AX_DMACFG_BASE + 0x10)
#define AX_DMACFG_RD_DDR_FUNC    (AX_DMACFG_BASE + 0x14)
#define AX_DMACFG_INTR_CLR       (AX_DMACFG_BASE + 0x18)
#define AX_DMACFG_INTR_MASK      (AX_DMACFG_BASE + 0x1C)
#define AX_DMACFG_INTR_RAW       (AX_DMACFG_BASE + 0x20)
#define AX_DMACFG_INTR_STA       (AX_DMACFG_BASE + 0x24)
#define AX_DMACFG_AXI_CFG        (AX_DMACFG_BASE + 0x28)

#define AX_DMACFG_CLK_EB_SET     FLASH_SYS_GLB_CLK_EB1_SET
#define AX_DMACFG_CLK_EB_CLR     FLASH_SYS_GLB_CLK_EB1_CLR
#define AX_DMACFG_PCLK_SHIFT     (13)
#define AX_DMACFG_ACLK_SHIFT     (0)

#define AX_DMACFG_RST_SET         FLASH_SYS_GLB_SW_RST0_SET
#define AX_DMACFG_RST_CLR         FLASH_SYS_GLB_SW_RST0_CLR
#define AX_DMACFG_PRST_SHIFT      (1)
#define AX_DMACFG_ARST_SHIFT      (0)

void ax_dma_cfg_preset(void)
{
	writel(BIT(AX_DMACFG_PRST_SHIFT), AX_DMACFG_RST_SET);
	writel(BIT(AX_DMACFG_PRST_SHIFT), AX_DMACFG_RST_CLR);
}

void ax_dma_cfg_areset(void)
{
	writel(BIT(AX_DMACFG_ARST_SHIFT), AX_DMACFG_RST_SET);
	writel(BIT(AX_DMACFG_ARST_SHIFT), AX_DMACFG_RST_CLR);
}

void ax_dma_cfg_clk_en(char en)
{
	if (en)
		writel(BIT(AX_DMACFG_PCLK_SHIFT) | BIT(AX_DMACFG_ACLK_SHIFT),
		       AX_DMACFG_CLK_EB_SET);
	else
		writel(BIT(AX_DMACFG_PCLK_SHIFT) | BIT(AX_DMACFG_ACLK_SHIFT),
		       AX_DMACFG_CLK_EB_CLR);
}

void ax_dma_cfg_init(char en)
{
	ax_dma_cfg_clk_en(en);
	if (en) {
		ax_dma_cfg_preset();
		ax_dma_cfg_areset();
	}
}

int ax_dma_cfg_start_lli(unsigned long lli)
{
	writel((unsigned int)((unsigned long long)(unsigned long)lli >> 32), AX_DMACFG_LLI_H);
	writel((unsigned int)(lli & 0xFFFFFFFF), AX_DMACFG_LLI_L);

	writel(0, AX_DMACFG_RD_DDR_FUNC);
	writel(1, AX_DMACFG_START);

	while (!(readl(AX_DMACFG_INTR_RAW) & 0x1)) ;
	writel(1, AX_DMACFG_INTR_CLR);
	writel(1 << AX_DMACFG_ARST_SHIFT, AX_DMACFG_RST_SET);
	writel(1 << AX_DMACFG_ARST_SHIFT, AX_DMACFG_RST_CLR);

	return 0;
}

int ax_dma_cfg_write_onebyone(unsigned long base_addr, void *info,
			      unsigned int len)
{
	ax_dmacfg_start_info_t *start_info = (ax_dmacfg_start_info_t *) info;
	ax_dmacfg_end_info_t *end_info =
	    (ax_dmacfg_end_info_t *) ((unsigned long long *)(unsigned long *)info + len + 1);
	ax_dmacfg_reserve_t *reserve =
	    (ax_dmacfg_reserve_t *) ((unsigned long long *)(unsigned long *)info + len + 2);

	start_info->cmd = 0x1;
	start_info->addr_base = base_addr;

	end_info->cmd = 0xf;
	end_info->lli_last = 0x1;
	end_info->lli_ioc = 0x0;
	end_info->llp = 0x0;

	ax_memset(reserve, 0, sizeof(ax_dmacfg_reserve_t));

	ax_dma_cfg_start_lli((unsigned long long)(unsigned long)info);

	return 0;
}

int ax_dma_cfg_writethrough(unsigned long base_addr, void *info,
			    unsigned int len)
{
	ax_dmacfg_start_info_t *start_info = (ax_dmacfg_start_info_t *) info;
	ax_dmacfg_writethrough_info_t *writethrough_info =
	    (ax_dmacfg_writethrough_info_t *) ((unsigned long long *)(unsigned long *)info + 1);
	ax_dmacfg_end_info_t *end_info =
	    (ax_dmacfg_end_info_t *) ((unsigned int *)info + len + 2 * 2);
	ax_dmacfg_reserve_t *reserve =
	    (ax_dmacfg_reserve_t *) ((unsigned int *)info + len + 3 * 2);

	start_info->cmd = 0x1;
	start_info->addr_base = base_addr;

	writethrough_info->cmd = 3;
	writethrough_info->dinc = 1;
	writethrough_info->inc_len = len;
	writethrough_info->tr_width = 2;
	writethrough_info->offset = 0;

	end_info->cmd = 0xf;
	end_info->lli_last = 0x1;
	end_info->lli_ioc = 0x0;
	end_info->llp = 0x0;

	ax_memset(reserve, 0, sizeof(ax_dmacfg_reserve_t));

	ax_dma_cfg_start_lli((unsigned long long)(unsigned long)info);

	return 0;
}
ax_dmacfg_write_jump_buf_t write_jump_buf = {
	.start_info = {
			.cmd = 0x1,
			},
	.write_jump_info = {
				.tr_width = 2,
				.sinc = 1,
				.dinc = 1,
				.cmd = 4,
				.jump_cmd = 5,
				},
	.end_info = {
			.cmd = 0xf,
			.lli_last = 0x1,
			.lli_ioc = 0x0,
			.llp = 0x0,
			},
	.reserve = {.reserve = {0}},
};
int ax_dma_cfg_write_jump(unsigned long base_addr,
			  unsigned long data_addr, unsigned int len)
{


	write_jump_buf.start_info.addr_base = base_addr;

	write_jump_buf.write_jump_info.offset = 0;
	write_jump_buf.write_jump_info.inc_len = len;
	write_jump_buf.write_jump_info.r_addr = data_addr;

	ax_dma_cfg_start_lli((unsigned long long)(unsigned long)&write_jump_buf);
	return 0;
}

ax_dmacfg_read_write_buf_t read_write_buf = {
	.start_info = {
			.cmd = 0x1,
			.addr_base = 0,
			},
	.read_and_write_info = {
				.tr_width = 2,
				.sinc = 1,
				.r_cmd = 6,
				.jump_cmd = 5,
				},
	.end_info = {
			.cmd = 0xf,
			.lli_last = 0x1,
			.lli_ioc = 0x0,
			.llp = 0x0,
			},
	.reserve = {.reserve = {0}},
};

int ax_dma_cfg_read_write(unsigned long base_addr, unsigned long data_addr, unsigned int len)
{
	read_write_buf.read_and_write_info.w_addr = base_addr;
	read_write_buf.read_and_write_info.r_addr = data_addr;
	read_write_buf.read_and_write_info.inc_len = len;

	ax_dma_cfg_start_lli((unsigned long long)(unsigned long)&read_write_buf);

	return 0;
}
