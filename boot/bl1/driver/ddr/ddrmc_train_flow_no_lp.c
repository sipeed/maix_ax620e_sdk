#ifndef __DDRMC_TRAIN_FLOW_NO_LP__
#define __DDRMC_TRAIN_FLOW_NO_LP__
#include "cmn.h"
#include "timer.h"
#include "ddrmc_reg_addr_def.h"
#include "ddrphy_reg_addr_def.h"
#include "ddr_init_config.h"
#include "ddrmc_train_reg_addr_def.h"
#include "ddrmc_bist_reg_addr_def.h"
#include "trace.h"
#include "boot_mode.h"

extern void ddr_writel(u32 addr, u32 value);
extern void ddrmc_mrw(u8 mr_cs, u8 mr_ad, u16 mr_op);
extern void ddrmc_auto_ref(u8 mr_cs);
extern u32 ddr_types;
extern u32 ddr_freqs;
extern u8 ddr_timming_index;
extern u32 board_type;
static u32 mr_cs = 0;
u32 max_pass_window_rdeye;
u32 cur_pass_window_rdeye;
#if (0 == LPDDR4_DFS_CONFIG)
extern u8 rddsk_fail;
extern u8 wrdsk_fail;
#endif

extern const unsigned int ddr4phy_timing[6][31];

extern void rd_vref_scan(struct ddr_dfs_vref_t * param, DDR_TYPE ddr_type, u8 cs, u8 vref_start, u8 vref_end, u8 interval);
extern void wr_vref_scan(struct ddr_dfs_vref_t * param, DDR_TYPE ddr_type, u8 cs, u8 vref_start, u8 vref_end, u8 interval);
extern u32 ddr3_ddr4_rdeye_train_flow(u8 cs_num);
extern void reg_mask_set(u32 addr, u32 mask_data, u32 bits_set);
extern void retrain_general_config(void);
extern u32 ddr3_ddr4_wreye_train_flow(u8 cs_num);
extern u32 ddr3_ddr4_rdeye_train_flow(u8 cs_num);

static int ddr3_vref_check(struct ddr_dfs_vref_t * param)
{
	if (!param)
		return -1;

	if (((param->rf_io_vrefi_adj_PHY_A > 0x30) && (param->rf_io_vrefi_adj_PHY_A < 0x60)) && ((param->rf_io_vrefi_adj_PHY_B > 0x32) && (param->rf_io_vrefi_adj_PHY_B < 0x40))) {
		return 0;
	} else {
		return -1;
	}
}

static int ddr4_vref_check(struct ddr_dfs_vref_t * param)
{
	if (!param)
		return -1;

	if ((((param->dram_VREF_CA[0] > 0x32) && (param->dram_VREF_CA[0] < 0x40)) || (param->dram_VREF_CA[0] > 0x72)) ||
		(((param->dram_VREF_DQ[0] > 0x32) && (param->dram_VREF_DQ[0] < 0x40)) || (param->dram_VREF_DQ[0] > 0x72))) {
			return -1;
		} else {
			return 0;
		}
}

void ddr3_ddr4_train_info_pre_set(void)
{
	reg_mask_set(D_DDRMC_CFG16_ADDR, 0x1, 0);//rf_auto_gate_en
	reg_mask_set(D_DDRMC_TMG14_F0_ADDR, 0x3 << 24, 0);//rf_auto_ref_allcs_f0 rf_auto_ref_en_f0
	reg_mask_set(D_DDRMC_CFG32_ADDR, 0, 0x1 << 5);//rf_dsoft_auto_ref
	udelay(1);
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		reg_mask_set(D_DDRMC_BIST_CTRL0_ADDR, 0x3070F000, 0x4 << 20);
	} else {
		reg_mask_set(D_DDRMC_BIST_CTRL0_ADDR, 0x3070F000, (0x1 << 28) | (0x5 << 20)); //rf_bist_datawidth
	}

	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		ddr_writel(D_DDRMC_TRAIN_STRB_0_ADDR, 0x0000ffff);
		ddr_writel(D_DDRMC_TRAIN_STRB_STP1_0_ADDR, 0x00000fff);
		ddr_writel(D_DDRMC_TRAIN_STRB_STP2_0_ADDR, 0x0000ffff);
		ddr_writel(D_DDRMC_TRAIN_STRB_STP3_0_ADDR, 0x0000ff0f);
		ddr_writel(D_DDRMC_TRAIN_MASK_0_ADDR, 0xffff0000);
		ddr_writel(D_DDRMC_TRAIN_MASK_STP1_0_ADDR, 0xfffff000);
		ddr_writel(D_DDRMC_TRAIN_MASK_STP2_0_ADDR, 0xffff000f);
		ddr_writel(D_DDRMC_TRAIN_MASK_STP3_0_ADDR, 0xffff00ff);
	} else {
		ddr_writel(D_DDRMC_TRAIN_STRB_0_ADDR, 0xffffffff);
		ddr_writel(D_DDRMC_TRAIN_STRB_STP1_0_ADDR, 0x00ffffff);
		ddr_writel(D_DDRMC_TRAIN_STRB_STP2_0_ADDR, 0xffffffff);
		ddr_writel(D_DDRMC_TRAIN_STRB_STP3_0_ADDR, 0xffff00ff);
		ddr_writel(D_DDRMC_TRAIN_MASK_0_ADDR, 0xf000000f);
		ddr_writel(D_DDRMC_TRAIN_MASK_STP1_0_ADDR, 0xff000000);
		ddr_writel(D_DDRMC_TRAIN_MASK_STP2_0_ADDR, 0xff0000ff);
		ddr_writel(D_DDRMC_TRAIN_MASK_STP3_0_ADDR, 0xff00ffff);
	}

	//rf_train_wrlvl_mode rf_train_wrlvl_thr_value rf_train_wrlvl_max_value
	reg_mask_set(D_DDRMC_TRAIN_CTRL9_ADDR, 0x37FF07FF, (0x2 << 28) | (0x90 << 16) | (0xa0));
	reg_mask_set(D_DDRMC_TRAIN_CTRL5_ADDR, 0xFF0FF, (0x8 << 12) | 0x8);
	reg_mask_set(D_DDRMC_TRAIN_TMG0_ADDR, 0xff << 24, 0x4 << 24);//drf_t_wrlvlcd
	reg_mask_set(D_DDRMC_TRAIN_CTRL1_ADDR, 0xFF007000, 0x40 << 20);//rf_phy_update_cnt
	reg_mask_set(D_DDRMC_TRAIN_CTRL3_ADDR, 0xFF0FF7FF, (0x2 << 24) | 0x200);//rf_train_eye_max_value
	reg_mask_set(D_DDRMC_TRAIN_CTRL4_ADDR, 0xFF0FF, 0x80);//rf_train_dsk_max_value

	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF000004, (0x3 << 24) | (0x1 << 2));
	} else {
		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF000004, (0xf << 24) | (0x1 << 2));//set bist
	}
}

void ddr4_train_rddsk(void)
{
	u32 reg_data = 0;
	u32 bit_shift = 0;

	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		bit_shift = 3 << 24;
		reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, 1 << 30, 0);
	} else {
		bit_shift = 0xf << 24;
		reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, 1 << 31, 0);
	}

	ddrmc_mrw((0x2 | mr_cs), 0x3, 0x4);
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100087, bit_shift | (1 << 7) | (1 << 2) | 1);
	//polling train done
	for (int i=0; i<1000 ; i++) {
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (0x1 << 12)) {//rf_train_rddsk_done
			if(reg_data & (0x1 << 13)) {//rf_train_rddsk_fail
				#if (0 == LPDDR4_DFS_CONFIG)
				rddsk_fail = 1;
				#endif
				ax_print_str("\r\nrddsk train fail !!!!!!!");
			} else {
				ax_print_str("\r\nrddsk train done !!!!!!!");
			}
			break;
		}
		if(i==999) {
			#if (0 == LPDDR4_DFS_CONFIG)
			rddsk_fail = 1;
			#endif
			ax_print_str("\r\npolling rddsk_training done timeout !!!!!!!");
		}
	}
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100087, bit_shift | (1 << 7) | (1 << 2) | (1 << 1));
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100087, bit_shift);
	udelay(1);
	reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR, 0, 3 << 12);//rf_multirank
	reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR, 0, 3 << 12);//rf_multirank
	ddrmc_mrw((0x2 | mr_cs), 0x3, 0x0);
}

void ddr3_ddr4_train_rd_pre_set(void)
{
	reg_mask_set(D_DDRMC_CFG3_ADDR, 0, (0x1 << 28) | (0x1 << 29));//rf_ie_multirank rf_oe_multirank
	reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR, 0, 0x3 << 12);//rf_multirank
	reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET, 0, 0x3 << 12);//rf_multirank
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		ddr_writel(D_DDRMC_BIST_DATA0_ADDR, 0xffff0000);
		ddr_writel(D_DDRMC_BIST_DATA1_ADDR, 0xffff0000);
		ddr_writel(D_DDRMC_BIST_DATA2_ADDR, 0xffff0000);
		ddr_writel(D_DDRMC_BIST_DATA3_ADDR, 0xffff0000);
	} else {
		ddr_writel(D_DDRMC_BIST_DATA0_ADDR, 0x00000000);
		ddr_writel(D_DDRMC_BIST_DATA1_ADDR, 0xffffffff);
		ddr_writel(D_DDRMC_BIST_DATA2_ADDR, 0x00000000);
		ddr_writel(D_DDRMC_BIST_DATA3_ADDR, 0xffffffff);
		ddr_writel(D_DDRMC_BIST_DATA4_ADDR, 0x00000000);
		ddr_writel(D_DDRMC_BIST_DATA5_ADDR, 0xffffffff);
		ddr_writel(D_DDRMC_BIST_DATA6_ADDR, 0x00000000);
		ddr_writel(D_DDRMC_BIST_DATA7_ADDR, 0xffffffff);
	}
}

void ddr3_ddr4_train_rdeye(struct ddr_dfs_vref_t * rom_dfs_vref_info_t)
{
	u32 bit_shift = 0;
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		bit_shift = 1 << 30;
	} else {
		bit_shift = 1 << 31;
	}
	reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, bit_shift, 0);
	ddrmc_mrw((0x2 | mr_cs), 0x3, 0x4);
	#ifdef DDR_OFFLINE_SCAN
		rd_vref_scan(NULL, TYPE_DDR4, 0, 0, 0x7f, 0x1);
	#else
		if (board_type == PHY_AX630C_DEMO_DDR3_V1_0) {
			rd_vref_scan(rom_dfs_vref_info_t, TYPE_DDR3, 0, DDR3_RDEYE_VREFDQ_MIN, DDR3_RDEYE_VREFDQ_MAX, 0x1);
		} else {
			rd_vref_scan(rom_dfs_vref_info_t, TYPE_DDR4, 0, DDR4_RDEYE_ODT_VREFDQ_MIN, DDR4_RDEYE_ODT_VREFDQ_MAX, 0x1);
		}
	#endif
	ddrmc_mrw((0x2 | mr_cs), 0x3, 0x0);
}

void ddr3_ddr4_train_wrdsk(void)
{
	u32 reg_data = 0;
	u32 bit_shift = 0;

	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		bit_shift = 0x3 << 24;
		reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, 1 << 30, 0);//rf_train_addr_30_0
	} else {
		bit_shift = 0xf << 24;
		reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, 1 << 31, 0);//rf_train_addr_31_0
	}
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100407, (bit_shift) | (1 << 10) | (1 << 2) | 1);

	//polling train done
	for (int i=0; i<1000 ; i++) {
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (0x1 << 24)) {//rf_train_wrdsk_done
			if(reg_data & (0x1 << 25)) {//rf_train_wrdsk_fail
				#if (0 == LPDDR4_DFS_CONFIG)
				wrdsk_fail = 1;
				#endif
				ax_print_str("\r\nwrdsk train fail !!!!!!!");
			} else {
				ax_print_str("\r\nwrdsk train done !!!!!!!");
			}
			break;
		}
		if(i==999) {
			#if (0 == LPDDR4_DFS_CONFIG)
			wrdsk_fail = 1;
			#endif
			ax_print_str("\r\npolling wrdsk_training done timeout !!!!!!!");
		}
	}
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100407, (bit_shift) | (1 << 10) | (1 << 2) | 1 << 1);
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100407, bit_shift);
}

void ddr3_ddr4_train_wr_pre_set(void)
{
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		ddr_writel(D_DDRMC_BIST_DATA0_ADDR, 0x5a5a5aa5);
		ddr_writel(D_DDRMC_BIST_DATA1_ADDR, 0xa55aa5a5);
		ddr_writel(D_DDRMC_BIST_DATA2_ADDR, 0x5a5a5aa5);
		ddr_writel(D_DDRMC_BIST_DATA3_ADDR, 0xa55aa5a5);
	} else {
		ddr_writel(D_DDRMC_BIST_DATA0_ADDR, 0x5a5a5aa5);
		ddr_writel(D_DDRMC_BIST_DATA1_ADDR, 0xa55aa5a5);
		ddr_writel(D_DDRMC_BIST_DATA2_ADDR, 0x5aa55a5a);
		ddr_writel(D_DDRMC_BIST_DATA3_ADDR, 0xa5a5a55a);
		ddr_writel(D_DDRMC_BIST_DATA4_ADDR, 0xaa5555aa);
		ddr_writel(D_DDRMC_BIST_DATA5_ADDR, 0x55aaaa55);
		ddr_writel(D_DDRMC_BIST_DATA6_ADDR, 0x5555aaaa);
		ddr_writel(D_DDRMC_BIST_DATA7_ADDR, 0xaaaa5555);
	}
	reg_mask_set(D_DDRMC_CFG3_ADDR, 0, (0x1 << 28) | (0x1 << 29));//rf_ie_multirank rf_oe_multirank
	reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR, 0, 0x3 << 12);
	reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET, 0, 0x3 << 12);
	reg_mask_set(D_DDRPHY_GEN_TMG13_F1_ADDR, 0, 0x3 << 12);
	reg_mask_set(D_DDRPHY_GEN_TMG13_F1_ADDR + DDR_PHY_OFFSET, 0, 0x3 << 12);
}

void ddrmc_train_flow_sequence(void * rom_param)
{
	struct ddr_dfs_vref_t * rom_dfs_vref_info = NULL;
	if (rom_param && (ddr_freqs == ((struct ddr_dfs_vref_t *)rom_param)->freq) &&
		(0 == ddr4_vref_check((struct ddr_dfs_vref_t *)rom_param)))
		rom_dfs_vref_info = (struct ddr_dfs_vref_t *)rom_param;

	ddr3_ddr4_train_info_pre_set();
	ddr3_ddr4_train_rd_pre_set();
	ddr4_train_rddsk();
	ddr3_ddr4_train_rdeye(rom_dfs_vref_info);
	udelay(1);
	ddr3_ddr4_train_wr_pre_set();
	ddr3_ddr4_train_wrdsk();
	udelay(1);
#ifdef DDR_OFFLINE_SCAN
	wr_vref_scan(NULL, TYPE_DDR4, 0, 0, 0x72, 0x1);
#else
	wr_vref_scan(rom_dfs_vref_info, TYPE_DDR4, 0, DDR4_WREYE_ODT_VREFDQ_MIN, DDR4_WREYE_ODT_VREFDQ_MAX, 0x1);
#endif
	reg_mask_set(D_DDRMC_CFG16_ADDR, 0, 0x1);//rf_auto_gate_en
	reg_mask_set(D_DDRMC_TMG14_F0_ADDR, 0, (0x1 << 24) | (0x1 << 25));//drf_auto_ref_en drf_auto_ref_allcs

#ifdef AX630C_DDR4_RETRAIN
	retrain_general_config();
#endif
}

void ddr3_train_flow_sequence(void * rom_param)
{
	struct ddr_dfs_vref_t * rom_dfs_vref_info = NULL;
	if (rom_param && (ddr_freqs == ((struct ddr_dfs_vref_t *)rom_param)->freq) &&
		(0 == ddr3_vref_check((struct ddr_dfs_vref_t *)rom_param)))
		rom_dfs_vref_info = (struct ddr_dfs_vref_t *)rom_param;
	ddr3_ddr4_train_info_pre_set();
	ddr3_ddr4_train_rd_pre_set();
	ddr3_ddr4_train_rdeye(rom_dfs_vref_info);
	ddr3_ddr4_train_wr_pre_set();
	ddr3_ddr4_train_wrdsk();
	ddr3_ddr4_wreye_train_flow(0);
	reg_mask_set(D_DDRMC_CFG16_ADDR, 0, 0x1);//rf_auto_gate_en
	reg_mask_set(D_DDRMC_TMG14_F0_ADDR, 0, (0x1 << 24) | (0x1 << 25));//drf_auto_ref_en drf_auto_ref_allcs
}
#endif

