#include "cmn.h"
#include "trace.h"
#include "stdint.h"
#include "ddrphy_timing_define_ddr3.h"
#include "ddrmc_timing_define_ddr3.h"
#include "ddrphy_timing_define_ddr4.h"
#include "ddrmc_timing_define_ddr4.h"
#include "ddrphy_timing_define_lpddr4.h"
#include "ddrmc_timing_define_lpddr4.h"
#include "ddrmc_reg_addr_def.h"
#include "ddrmc_reg_data_def.h"
#include "ddrmc_train_reg_addr_def.h"
#include "ddrphy_reg_data_def.h"
#include "ddr_init_config.h"
#include "ddrmc_bist_reg_addr_def.h"
#include "ddr_init_config.h"
#include "ddrphy_reg_addr_def.h"
#include "dpll_ctrl_glb.h"
#include "timer.h"
#include "boot.h"
#include "ax_adc.h"
#include "boot_mode.h"

u8 lpddr4_ca_odt_en = 0;
u8 lpddr4_ca_train_en = 0;
u16 mr2_status_op = 0;
u16 mr3_status_op = 0;
u32 ddr4_mr6_data_save = 0;
u32 chip_id = 0;
u32 board_id = 0;
u32 r_pass_window_dqeye __attribute__((section(".data"))) = 0;
u32 w_pass_window_dqeye __attribute__((section(".data"))) = 0;
u32 ddr_dfs_sel = LPDDR4_DFS_CONFIG;
u32 ddr_dfs_f1_freq = DDR_CLK_2400;
#if (0 == LPDDR4_DFS_CONFIG)
u8 training_pass __attribute__((section(".data"))) = 0;
u8 caeye_pass __attribute__((section(".data"))) = 0;
u8 rdeye_pass __attribute__((section(".data"))) = 0;
u8 wreye_pass __attribute__((section(".data"))) = 0;
u8 cadsk_fail __attribute__((section(".data"))) = 0;
u8 rddsk_fail __attribute__((section(".data"))) = 0;
u8 wrdsk_fail __attribute__((section(".data"))) = 0;
#endif
static u8 ddrphy_timing_count = 30;

u32 ddr_types = TYPE_DDR4_1CS_2CH_X32_16GBIT; //used for chosing ddr types by board id
u32 ddr_freqs = DDR_CLK_1600;
u8 ddr_timming_index = 0; //delete const if use board id
u32 board_type;
#ifdef DDR_BIST_EN
u8 bist_burst_length = 0x0;
u8 bist_data_size = 0x5;
#endif

static const u32 ddr3_freqs[3][2] = {
	{DDR3_CONFIG_1600, DDR_CLK_1600},
	{DDR3_CONFIG_1866, DDR_CLK_1866},
	{DDR3_CONFIG_2133, DDR_CLK_2133}};
static const u32 ddr4_freqs[4][2] = {
	{DDR4_CONFIG_1600, DDR_CLK_1600},
	{DDR4_CONFIG_2400, DDR_CLK_2400},
	{DDR4_CONFIG_2666, DDR_CLK_2666},
	{DDR4_CONFIG_3200, DDR_CLK_3200}};
static const u32 lpddr4_freqs[7][2] = {
	{LPDDR4_CONFIG_1600, DDR_CLK_1600},
	{LPDDR4_CONFIG_2133, DDR_CLK_2133},
	{LPDDR4_CONFIG_2400, DDR_CLK_2400},
	{LPDDR4_CONFIG_2666, DDR_CLK_2666},
	{LPDDR4_CONFIG_2800, DDR_CLK_2800},
	{LPDDR4_CONFIG_3200, DDR_CLK_3200},
	{LPDDR4_CONFIG_3733, DDR_CLK_3400}};
static const u32 ax620q_lpddr4_dq_deskew_pre_set[48] = {
	0x241e2d1c,	//phya rank0 output D_DDRPHY_DS0_CFG6_ADDR dq3  dq2  dq1 dq0
	0x3a363a38,	//phya rank0 output D_DDRPHY_DS0_CFG7_ADDR dq7  dq6  dq5 dq4
	0x3c,		//phya rank0 output D_DDRPHY_DS0_CFG8_ADDR dqm
	0x00000000,	//phya rank1 output D_DDRPHY_DS0_CFG9_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phya rank1 output D_DDRPHY_DS0_CFG10_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank1 output D_DDRPHY_DS0_CFG11_ADDR dqm
	0x16160a14,	//phya rank0 input D_DDRPHY_DS0_CFG12_ADDR dq3  dq2  dq1 dq0
	0xe0e040c,	//phya rank0 input D_DDRPHY_DS0_CFG13_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank0 input D_DDRPHY_DS0_CFG14_ADDR dqm
	0x00000000,	//phya rank1 input D_DDRPHY_DS0_CFG15_ADDR, dq3  dq2  dq1 dq0
	0x00000000,	//phya rank1 input D_DDRPHY_DS0_CFG16_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank1 input D_DDRPHY_DS0_CFG17_ADDR dqm
	0x2020202,	//phya rank0 output D_DDRPHY_DS1_CFG6_ADDR dq3  dq2  dq1 dq0
	0x2020202,	//phya rank0 output D_DDRPHY_DS1_CFG7_ADDR dq7  dq6  dq5 dq4
	0x1,		//phya rank0 output D_DDRPHY_DS1_CFG8_ADDR dqm
	0x00000000,	//phya rank1 output D_DDRPHY_DS1_CFG9_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phya rank1 output D_DDRPHY_DS1_CFG10_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank1 output D_DDRPHY_DS1_CFG11_ADDR dqm
	0x1010101,	//phya rank0 input D_DDRPHY_DS1_CFG12_ADDR dq3  dq2  dq1 dq0
	0x1010101,	//phya rank0 input D_DDRPHY_DS1_CFG13_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank0 input D_DDRPHY_DS1_CFG14_ADDR dqm
	0x00000000,	//phya rank1 input D_DDRPHY_DS1_CFG15_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phya rank1 input D_DDRPHY_DS1_CFG16_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank1 input D_DDRPHY_DS1_CFG17_ADDR dqm
	0x2020202,	//phyb rank0 output D_DDRPHY_DS0_CFG6_ADDR dq3  dq2  dq1 dq0
	0x2020202,	//phyb rank0 output D_DDRPHY_DS0_CFG7_ADDR dq7  dq6  dq5 dq4
	0x1,		//phyb rank0 output D_DDRPHY_DS0_CFG8_ADDR dqm
	0x00000000,	//phyb rank1 output D_DDRPHY_DS0_CFG9_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phyb rank1 output D_DDRPHY_DS0_CFG10_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank1 output D_DDRPHY_DS0_CFG11_ADDR dqm
	0x1010101,	//phyb rank0 input D_DDRPHY_DS0_CFG12_ADDR dq3  dq2  dq1 dq0
	0x1010101,	//phyb rank0 input D_DDRPHY_DS0_CFG13_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank0 input D_DDRPHY_DS0_CFG14_ADDR dqm
	0x00000000,	//phyb rank1 input D_DDRPHY_DS0_CFG15_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phyb rank1 input D_DDRPHY_DS0_CFG16_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank1 input D_DDRPHY_DS0_CFG17_ADDR dqm
	0x2a24261e,	//phyb rank0 output D_DDRPHY_DS1_CFG6_ADDR dq3  dq2  dq1 dq0
	0x24211826,	//phyb rank0 output D_DDRPHY_DS1_CFG7_ADDR dq7  dq6  dq5 dq4
	0x2b,		//phyb rank0 output D_DDRPHY_DS1_CFG8_ADDR dqm
	0x00000000,	//phyb rank1 output D_DDRPHY_DS1_CFG9_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phyb rank1 output D_DDRPHY_DS1_CFG10_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank1 output D_DDRPHY_DS1_CFG11_ADDR dqm
	0x16160e16,	//phyb rank0 input D_DDRPHY_DS1_CFG12_ADDR dq3  dq2  dq1 dq0
	0xe031214,	//phyb rank0 input D_DDRPHY_DS1_CFG13_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank0 input D_DDRPHY_DS1_CFG14_ADDR dqm
	0x00000000,	//phyb rank1 input D_DDRPHY_DS1_CFG15_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phyb rank1 input D_DDRPHY_DS1_CFG16_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank1 input D_DDRPHY_DS1_CFG17_ADDR dqm
};
static const u32 ax630c_lpddr4_dq_deskew_pre_set[48] = {
	0x021c1c22,	//phya rank0 output D_DDRPHY_DS0_CFG6_ADDR dq3  dq2  dq1 dq0
	0x33323325,	//phya rank0 output D_DDRPHY_DS0_CFG7_ADDR dq7  dq6  dq5 dq4
	0x1e,		//phya rank0 output D_DDRPHY_DS0_CFG8_ADDR dqm
	0x00000000,	//phya rank1 output D_DDRPHY_DS0_CFG9_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phya rank1 output D_DDRPHY_DS0_CFG10_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank1 output D_DDRPHY_DS0_CFG11_ADDR dqm
	0x04361c22,	//phya rank0 input D_DDRPHY_DS0_CFG12_ADDR dq3  dq2  dq1 dq0
	0x4e4e4e3c,	//phya rank0 input D_DDRPHY_DS0_CFG13_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank0 input D_DDRPHY_DS0_CFG14_ADDR dqm
	0x00000000,	//phya rank1 input D_DDRPHY_DS0_CFG15_ADDR, dq3  dq2  dq1 dq0
	0x00000000,	//phya rank1 input D_DDRPHY_DS0_CFG16_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank1 input D_DDRPHY_DS0_CFG17_ADDR dqm
	0x2c2a2c24,	//phya rank0 output D_DDRPHY_DS1_CFG6_ADDR dq3  dq2  dq1 dq0
	0x24221a1c,	//phya rank0 output D_DDRPHY_DS1_CFG7_ADDR dq7  dq6  dq5 dq4
	0x1d,		//phya rank0 output D_DDRPHY_DS1_CFG8_ADDR dqm
	0x00000000,	//phya rank1 output D_DDRPHY_DS1_CFG9_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phya rank1 output D_DDRPHY_DS1_CFG10_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank1 output D_DDRPHY_DS1_CFG11_ADDR dqm
	0x565e545d,	//phya rank0 input D_DDRPHY_DS1_CFG12_ADDR dq3  dq2  dq1 dq0
	0x5e5e545b,	//phya rank0 input D_DDRPHY_DS1_CFG13_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank0 input D_DDRPHY_DS1_CFG14_ADDR dqm
	0x00000000,	//phya rank1 input D_DDRPHY_DS1_CFG15_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phya rank1 input D_DDRPHY_DS1_CFG16_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phya rank1 input D_DDRPHY_DS1_CFG17_ADDR dqm
	0x0224242a,	//phyb rank0 output D_DDRPHY_DS0_CFG6_ADDR dq3  dq2  dq1 dq0
	0x37434224,	//phyb rank0 output D_DDRPHY_DS0_CFG7_ADDR dq7  dq6  dq5 dq4
	0x2c,		//phyb rank0 output D_DDRPHY_DS0_CFG8_ADDR dqm
	0x00000000,	//phyb rank1 output D_DDRPHY_DS0_CFG9_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phyb rank1 output D_DDRPHY_DS0_CFG10_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank1 output D_DDRPHY_DS0_CFG11_ADDR dqm
	0x02261a26,	//phyb rank0 input D_DDRPHY_DS0_CFG12_ADDR dq3  dq2  dq1 dq0
	0x3e444a34,	//phyb rank0 input D_DDRPHY_DS0_CFG13_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank0 input D_DDRPHY_DS0_CFG14_ADDR dqm
	0x00000000,	//phyb rank1 input D_DDRPHY_DS0_CFG15_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phyb rank1 input D_DDRPHY_DS0_CFG16_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank1 input D_DDRPHY_DS0_CFG17_ADDR dqm
	0x3c3d3c36,	//phyb rank0 output D_DDRPHY_DS1_CFG6_ADDR dq3  dq2  dq1 dq0
	0x2f252a24,	//phyb rank0 output D_DDRPHY_DS1_CFG7_ADDR dq7  dq6  dq5 dq4
	0x32,		//phyb rank0 output D_DDRPHY_DS1_CFG8_ADDR dqm
	0x00000000,	//phyb rank1 output D_DDRPHY_DS1_CFG9_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phyb rank1 output D_DDRPHY_DS1_CFG10_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank1 output D_DDRPHY_DS1_CFG11_ADDR dqm
	0x54565654,	//phyb rank0 input D_DDRPHY_DS1_CFG12_ADDR dq3  dq2  dq1 dq0
	0x4e4e444e,	//phyb rank0 input D_DDRPHY_DS1_CFG13_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank0 input D_DDRPHY_DS1_CFG14_ADDR dqm
	0x00000000,	//phyb rank1 input D_DDRPHY_DS1_CFG15_ADDR dq3  dq2  dq1 dq0
	0x00000000,	//phyb rank1 input D_DDRPHY_DS1_CFG16_ADDR dq7  dq6  dq5 dq4
	0x00000000,	//phyb rank1 input D_DDRPHY_DS1_CFG17_ADDR dqm
};

struct ddr_dfs_vref_t * p_iram_dfs_vref_info = (struct ddr_dfs_vref_t *)DDR_INFO_ADDR;

#ifdef DDR_BIST_EN
u32 user_mode_pattern[]=
{
	BIST_DATA_00_PATTERN,
	BIST_DATA_01_PATTERN,
	BIST_DATA_02_PATTERN,
	BIST_DATA_03_PATTERN,
	BIST_DATA_04_PATTERN,
	BIST_DATA_05_PATTERN,
	BIST_DATA_06_PATTERN,
	BIST_DATA_07_PATTERN,
	BIST_DATA_08_PATTERN,
	BIST_DATA_09_PATTERN,
	BIST_DATA_10_PATTERN,
	BIST_DATA_11_PATTERN,
	BIST_DATA_12_PATTERN,
	BIST_DATA_13_PATTERN,
	BIST_DATA_14_PATTERN,
	BIST_DATA_15_PATTERN,
	BIST_DATA_16_PATTERN,
	BIST_DATA_17_PATTERN,
	BIST_DATA_18_PATTERN,
	BIST_DATA_19_PATTERN,
	BIST_DATA_20_PATTERN,
	BIST_DATA_21_PATTERN,
	BIST_DATA_22_PATTERN,
	BIST_DATA_23_PATTERN,
	BIST_DATA_24_PATTERN,
	BIST_DATA_25_PATTERN,
	BIST_DATA_26_PATTERN,
	BIST_DATA_27_PATTERN,
	BIST_DATA_28_PATTERN,
	BIST_DATA_29_PATTERN,
	BIST_DATA_30_PATTERN,
	BIST_DATA_31_PATTERN
};

u32 sipi_mode_pattern[]=
{
	BIST_SIPI_BIT_PATTERN_0,
	BIST_SIPI_BIT_PATTERN_1,
	BIST_SIPI_DATA_00,
	BIST_SIPI_DATA_01,
	BIST_SIPI_DATA_02,
	BIST_SIPI_DATA_03,
	BIST_SIPI_DATA_04,
	BIST_SIPI_DATA_05,
	BIST_SIPI_DATA_06,
	BIST_SIPI_DATA_07,
	BIST_SIPI_DATA_08,
	BIST_SIPI_DATA_09,
	BIST_SIPI_DATA_10,
	BIST_SIPI_DATA_11,
	BIST_SIPI_DATA_12,
	BIST_SIPI_DATA_13,
	BIST_SIPI_DATA_14,
	BIST_SIPI_DATA_15
};

u32 lfsr_mode_pattern[] =
{
	BIST_DATA_LFSR_SEED_L0,
	BIST_DATA_LFSR_SEED_L1,
	BIST_DATA_LFSR_SEED_H0,
	BIST_DATA_LFSR_SEED_H1
};
#endif

extern void ddr4phy_pre_init_dump(void);
extern void ddr3phy_pre_init_dump(void);
extern void lpddr4phy_pre_init_dump(void);
extern void ddrmc_train_flow_lp4_sequence(void * rom_param);
extern void ddrmc_train_flow_sequence(void * rom_param);
extern void ddrphy_freq1_timing_init(u32 offset_addr, const u32 ddrphy_timing[][31]);
extern void ddrmc_dfs_freq1_timing_init(void);
extern void ddrmc_freq1_timing_init(void);
extern void ddr3_train_flow_sequence(void * rom_param);
#ifdef DDR_OFFLINE_SCAN
extern void ddr_scan_result_proc(DDR_TYPE ddr_type, u8 cs);
#endif

void ddr_writel(u32 addr, u32 value)
{
	*(volatile u32 *)(uintptr_t)addr = value;
}

void reg_bit_set(u32 addr, u32 start_bit, u32 bits_num, u32 val)
{
	u32 reg_data, bit_mask = ((1 << bits_num) - 1);
	reg_data = readl(addr);
	reg_data &= ~(bit_mask << start_bit);
	reg_data |= ((val & bit_mask) << start_bit);
	ddr_writel(addr, reg_data);
}

void reg_mask_set(u32 addr, u32 mask_data, u32 bits_set)
{
	u32 reg_data;
	reg_data = readl(addr);
	reg_data &= ~(mask_data);
	reg_data |= bits_set;
	ddr_writel(addr, reg_data);
}

void ddr_dfs_reg_mask_set(u32 addr, u32 mask_data, u32 bits_set, u32 dfs_index)
{
	u32 reg_data;
#if LPDDR4_DFS_CONFIG
	if(dfs_index == 0x1 && (addr < 0x110000)) {
		addr += 0x80;//ddrmc
	} else if (dfs_index == 0x1 && (addr >= 0x110000)) {
		addr += 0x100;//ddrphy
	}
#endif
	reg_data = readl(addr);
	reg_data &= ~(mask_data);
	reg_data |= bits_set;
	ddr_writel(addr, reg_data);
}

void ddrphy_dfs_writel(u32 addr, u32 value, u32 dfs_index)
{
#if LPDDR4_DFS_CONFIG
	if(dfs_index == 0x1) {
		addr += 0x100;
	}
#endif
	*(volatile u32 *)(uintptr_t)addr = value;
}

u32 ddrphy_dfs_readl(unsigned long addr, u32 dfs_index)
{
#if LPDDR4_DFS_CONFIG
	if(dfs_index == 0x1) {
		addr += 0x100;
	}
#endif
	return *(const volatile u32 *)addr;
}

void ddrphy_zqc_seq(void)
{
	u32 zqcal_flag, zqcal_val_a = 0, zqcal_val_b = 0;
	u32 zq_ncal = 0, zq_pcal = 0;
	reg_bit_set(D_DDRPHY_GEN_TMG10_F0_ADDR + DDR_PHY_OFFSET, 0, 8, 0x40);	//Adjust value of Internal VREF
	reg_bit_set(D_DDRPHY_GEN_TMG10_F0_ADDR + DDR_PHY_OFFSET, 24, 2, 0x0);	//Power down of Internal VREF
	reg_bit_set(D_DDRPHY_GEN_TMG9_F0_ADDR + DDR_PHY_OFFSET, 26, 1, 0);	//Enables lpddr4x IO of DQS			---disable
#if LPDDR4_DFS_CONFIG
	reg_bit_set(D_DDRPHY_GEN_TMG10_F1_ADDR + DDR_PHY_OFFSET, 0, 8, 0x40);	//Adjust value of Internal VREF
	reg_bit_set(D_DDRPHY_GEN_TMG10_F1_ADDR + DDR_PHY_OFFSET, 24, 2, 0x0);	//Power down of Internal VREF
	reg_bit_set(D_DDRPHY_GEN_TMG9_F1_ADDR + DDR_PHY_OFFSET, 26, 1, 0);	//Enables lpddr4x IO of DQS			---disable
#endif
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 2, 1, 0x1);		//Shutdown ZQ calibration circuit	---close
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 0, 1, 0x0);		//Enables NMOS ZQ calibration		---disable
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 16, 1, 0x0);	//Enables PMOS ZQ calibration		---disable
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 20, 6, 0x0);	//PMOS IO setting for calibration	---reset
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 4, 6, 0x0);		//NMOS IO setting for calibration	---reset
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 2, 1, 0x0);		//Shutdown ZQ calibration circuit	---enable

	//	/***start pmos zq calibration****/
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 16, 1, 0x1);	//Enables PMOS ZQ calibration  ---enable
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 20, 6, 0xf);	//reset PMOS IO setting for calibration to f
	udelay(20);
	zqcal_flag = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 17) & 0x1;	//Status of PMOS compare result
	if (zqcal_flag == 0x1) {
		while (1) {
			zqcal_val_a = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 20) & 0x3f;
			zqcal_val_a -= 1;
			reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 20, 6, zqcal_val_a);//rf_zq_cal_pu_sel
			udelay(200);
			zqcal_flag = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 17) & 0x1;
			if (zqcal_flag == 0) {
				break;
			}
			if (zqcal_val_a == 0x0) {
				//zq cal fail
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
				reg_bit_set(DDR_DUMMY_REG_SW0, 0, 1, 0x1);
#endif
				ax_print_str("\r\nD P fail");//Dec PCAL fail
				break;
			}
		}
	}
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 16, 1, 0x0); //Enables PMOS ZQ calibration  ---disable

	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 16, 1, 0x1);//rf_zq_cal_pcal_en
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 20, 6, 0x0);//rf_zq_cal_pu_sel
	udelay(20);
	zqcal_flag = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 17) & 0x1;//pcomp
	if (zqcal_flag == 0x0) {
		while (1) {
			zqcal_val_b = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 20) & 0x3f;
			zqcal_val_b += 1;
			reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 20, 6, zqcal_val_b);//rf_zq_cal_pu_sel
			udelay(20);
			zqcal_flag = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 17) & 0x1;
			if (zqcal_flag == 1) {
				break;
			}
			if (zqcal_val_b == 0xf) {
				//zq cal fail
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
				reg_bit_set(DDR_DUMMY_REG_SW0, 0, 1, 0x1);
#endif
				ax_print_str("\r\nI P fail");//Inc PCAL fail
			}
		}
	}
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 16, 1, 0x0);//rf_zq_cal_pcal_en

	if (zqcal_val_a > zqcal_val_b) {
		if ((zqcal_val_a - zqcal_val_b) <= 3) {
			zq_pcal = (zqcal_val_a + zqcal_val_b) / 2;
		} else {
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
			reg_bit_set(DDR_DUMMY_REG_SW0, 0, 1, 0x1);
#endif
			ax_print_str("\r\nD P > 4");//delat P > 4
			while (1);
		}
	} else {
		if ((zqcal_val_b - zqcal_val_a) <= 3) {
			zq_pcal = (zqcal_val_a + zqcal_val_b) / 2;
		} else {
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
			reg_bit_set(DDR_DUMMY_REG_SW0, 0, 1, 0x1);
#endif
			ax_print_str("\r\nD N > 4");//delat N > 4
			while (1);
		}
	}

	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 26, 6, zq_pcal);//set zq_pcal

	/***start NMOS zq calibration****/
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 0, 1, 0x1);//Enables NMOS ZQ calibration		---enable
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 4, 6, 0x0);//reset NMOS IO setting for calibration to 0
	udelay(20);
	zqcal_flag = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 1) & 0x1;	//Status of NMOS compare result
	if (zqcal_flag == 0x1) {
		while (1) {
			zqcal_val_a = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 4) & 0x3f;//read out NMOS IO setting for calibration
			zqcal_val_a += 1;
			reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 4, 6, zqcal_val_a);		//write in NMOS IO setting for calibration
			udelay(20);
			zqcal_flag = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 1) & 0x1;	//Status of NMOS compare result
			if (zqcal_flag == 0) {
				break;
			}
			if (zqcal_val_a == 0xf) {
				//zq cal fail
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
				reg_bit_set(DDR_DUMMY_REG_SW0, 0, 1, 0x1);
#endif
				ax_print_str("\r\nI N fail");//Inc NCAL fail
			}
		}
	}
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 0, 1, 0x0);	//Enables NMOS ZQ calibration		---disable

	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 0, 1, 0x1);//Enables NMOS ZQ calibration		---enable
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 4, 6, 0xf);//NMOS IO setting for calibration	---reset to f
	udelay(20);
	zqcal_flag = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 1) & 0x1;//ncomp
	if (zqcal_flag == 0x0) {
		while (1) {
			zqcal_val_b = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 4) & 0x3f;
			zqcal_val_b -= 1;
			reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 4, 6, zqcal_val_b);//rf_zq_cal_pd_sel
			udelay(20);
			zqcal_flag = (readl(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET) >> 1) & 0x1;//ncomp
			if (zqcal_flag == 1) {
				break;
			}
			if (zqcal_val_b == 0x0) {
				//zq cal fail
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
				reg_bit_set(DDR_DUMMY_REG_SW0, 0, 1, 0x1);
#endif
				ax_print_str("\r\nD N fail");//Dec NCAL fail
			}
		}
	}
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 0, 1, 0x0);//rf_zq_cal_ncal_en

	if (zqcal_val_a > zqcal_val_b) {
		if ((zqcal_val_a - zqcal_val_b) <= 3) {
			zq_ncal = (zqcal_val_a + zqcal_val_b) / 2;
		} else {
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
			reg_bit_set(DDR_DUMMY_REG_SW0, 0, 1, 0x1);
#endif
			ax_print_str("\r\nD N > 4");//delta N > 4
			while (1);
		}
	} else {
		if ((zqcal_val_b - zqcal_val_a) <= 3) {
			zq_ncal = (zqcal_val_a + zqcal_val_b) / 2;
		} else {
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
			reg_bit_set(DDR_DUMMY_REG_SW0, 0, 1, 0x1);
#endif
			ax_print_str("\r\nD P > 4");//delta P > 4
			while (1);
		}
	}

	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 10, 6, zq_ncal);//set zq_ncal

	/***finish n/pmos zq calibration****/
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 4, 6, 0x0);	//rf_zq_cal_pd_sel
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 20, 6, 0x0);//rf_zq_cal_pu_sel
	reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 2, 1, 0x1);	//Shutdown ZQ calibration circuit
	if ((zq_ncal <= 0x0) || (zq_ncal >= 0xf)) {
		reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR, 10, 6, 0x5);//rf_zq_cal_pdio_sel
		reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 10, 6, 0x5);//rf_zq_cal_pdio_sel
	} else {
		reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR, 10, 6, zq_ncal);//rf_zq_cal_pdio_sel
		reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 10, 6, zq_ncal);//rf_zq_cal_pdio_sel
	}
	if ((zq_pcal <= 0x0) || (zq_pcal >= 0xf)) {
		//	if (zq_pcal >= 0xe) {
		reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR, 26, 6, 0x6);//rf_zq_cal_puio_sel
		reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 26, 6, 0x6);//rf_zq_cal_puio_sel
	} else {
		reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR, 26, 6, zq_pcal);//rf_zq_cal_puio_sel
		reg_bit_set(D_DDRPHY_GEN_CFG1_ADDR + DDR_PHY_OFFSET, 26, 6, zq_pcal);//rf_zq_cal_puio_sel
	}
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
	reg_bit_set(DDR_DUMMY_REG_SW1, 0, 1, 0x1);
#endif
}

void ddrphy_timing_init(u32 offset_addr, const u32 ddrphy_timing[][31], u8 ddrphy_timming_index)
{
	u32 i,j = 0;
	u32 base_addr;

	for (i = 0, j = 0; i <= ddrphy_timing_count; i++, j++) {//i = 0, j = 0; i <= 30; i++, j++
		if (i < 14) {
			base_addr = D_DDRPHY_GEN_TMG0_F0_ADDR;
		} else if (i >= 14 && i <= (ddrphy_timing_count - 14)) {//i >= 14 && i <= 16
			base_addr = D_DDRPHY_AC_TMG0_F0_ADDR;
		} else if (i >= (ddrphy_timing_count - 13) && i <= (ddrphy_timing_count - 7)){//i >= 17 && i <= 23
			base_addr = D_DDRPHY_DS0_TMG0_F0_ADDR;
		} else {
			base_addr = D_DDRPHY_DS1_TMG0_F0_ADDR;
		}
		if (i == 14 || i == (ddrphy_timing_count - 13) || i == (ddrphy_timing_count - 6)) j = 0;//(i == 14 || i == (17) || i == 24)
		ddr_writel(offset_addr + base_addr + j * 4, ddrphy_timing[ddrphy_timming_index][i]);
	}
	ddrphy_timing_count = 30;
}

void ddrmc_auto_ref(u8 mr_cs)
{
	u32 reg_data;

	reg_mask_set(D_DDRMC_CFG30_ADDR, 0xf000000, 1 << 24);
	reg_data = readl(D_DDRMC_CFG32_ADDR);
	if(mr_cs & 0x02) {
		reg_data &= ~(1 << 24);
		reg_data &= ~(1 << 28);
		if (mr_cs & 0x1) {
			reg_data |= (1 << 28);
		}
	} else {
		reg_data |= (1 << 24);
		reg_data &= ~(1 << 28);
	}
	reg_data |= (1 << 5);
	ddr_writel(D_DDRMC_CFG32_ADDR, reg_data);

	for(int poll_num = 0; poll_num < 1000; poll_num++) {
		reg_data = readl(D_DDRMC_CFG32_ADDR);
		if((reg_data & (1 << 29)) == 0) {
			break;
		}
		if(poll_num == 999) {
			//ax_printf("[debug] AUTO_REF MPC NUM = 999");
		}
	}
}

#if defined (AX_DDR_DEBUG) || defined (DDR_OFFLINE_SCAN)
void ddrmc_mrr(u8 mr_cs, u16 mr_ad, u16 *mr_op)
{
	u32 reg_data;
	u32 mr_core = 0x1;
	*mr_op = 0x0;

	reg_data = readl(D_DDRMC_CFG30_ADDR);
	if (ddr_types & TYPE_LPDDR4) {
		reg_data = readl(D_DDRMC_CFG30_ADDR);
		reg_data &= ~0xf00ffff;
		reg_data |= mr_core << 24;
		reg_data |= mr_ad ;
		ddr_writel(D_DDRMC_CFG30_ADDR, reg_data);
	} else {
		reg_data = readl(D_DDRMC_CFG30_ADDR);
		reg_data &= ~(0xf << 24);
		reg_data |= mr_core << 24;
		ddr_writel(D_DDRMC_CFG30_ADDR, reg_data);

		reg_data = readl(D_DDRMC_CFG31_ADDR);
		reg_data &= ~(0x7 << 24);
		reg_data |= mr_ad << 24;
		ddr_writel(D_DDRMC_CFG31_ADDR, reg_data);
	}

	//core0 cs
	reg_data = readl(D_DDRMC_CFG32_ADDR);
	reg_data &= ~(1 << 28);
	reg_data |= mr_cs << 28;//cs
	reg_data &= ~(1 << 24);//allcs
	reg_data |= (1 << 2);//mrr
	ddr_writel(D_DDRMC_CFG32_ADDR, reg_data);

	for (int poll_num = 0; poll_num < 10; poll_num++) {
		reg_data = readl(D_DDRMC_CFG32_ADDR);
		if ((((reg_data >> 29) & 0x1) == 0) && (((reg_data >> 20) & 1) == 1)) {
			reg_data &= 0xff000;
			*mr_op = reg_data >> 12;
			break;
		}
		if (poll_num == 9) {
			ax_print_str("POLL MRR NUM = 999");
		}
	}
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nread mr_cs=0x");
	ax_print_num(mr_cs, 16);
	ax_print_str(", mr_ad=");
	ax_print_num(mr_ad, 10);
	ax_print_str(", mr_op=0x");
	ax_print_num(*mr_op, 16);
#endif
}
#endif

void ddrmc_sys_glb_init(DDR_FREQ freq)
{
	u32 bit_shift = 0;
	misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;
	// Config ON
	reg_mask_set(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_PLL_RE_OPEN_ADDR, 0x1, 0);
	reg_mask_set(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_DPLL_ON_CFG_ADDR, 0x1, 0);
	if (freq == DDR_CLK_3733) {
		bit_shift = 0x137;  // 3732
	} else if (freq == DDR_CLK_3400) {
		bit_shift = 0x11b;  // 3396
	} else if (freq == DDR_CLK_3200) {
		bit_shift = 0x10a;  // 3192
	} else if (freq == DDR_CLK_2800) {
		bit_shift = 0xe9;  // 2796
	} else if (freq == DDR_CLK_2666) {
		bit_shift = 0xde;  // 2664
	} else if (freq == DDR_CLK_2400) {
		bit_shift = 0xc8;  // 2400
	} else if (freq == DDR_CLK_2133) {
		bit_shift = 0xb1;  // 2124
	} else if (freq == DDR_CLK_1866) {
		bit_shift = 0x9b;  // 1860
	} else if (freq == DDR_CLK_1600) {
		bit_shift = 0x85;  // 1596
	}
	reg_mask_set(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_DPLL_CFG1_ADDR, 0x18641ff, bit_shift);
	// Config ON
	reg_mask_set(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_DPLL_ON_CFG_ADDR, 0, 0x1);
	// Wait locked and ready
	while((readl(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_DPLL_STS_ADDR) & 0x1) != 0x1);
	while((readl(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_PLL_RDY_STS_ADDR) & 0x1) != 0x1);
	reg_mask_set(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_PLL_RE_OPEN_ADDR, 0, 0x1);

	ddr_writel(0x02340068, 0x00000200);
	ddr_writel(0x02340068, 0x00000100);
	ddr_writel(0x00210004, 0x00000001);
	if ((misc_info->chip_type == AX620Q_CHIP_E) || (misc_info->chip_type == AX620QX_CHIP_E) || (misc_info->chip_type == AX620QZ_CHIP_E) || (misc_info->chip_type == AX620QP_CHIP_E)) {
		//ddr_writel(0x00210000, 0x0006DB6E);
		ddr_writel(0x00210000, 0x00076DB6);
		if(ddr_dfs_sel) {
			ddr_writel(0x00210000, 0x0002DB6E);//npll_1600m
			ddr_writel(0x00210064, 0x00000010);
		}
	} else {
		ddr_writel(0x00210000, 0x00076DB6);
	}

	ddr_writel(0x00210008, 0xF07F83E0);
	ddr_writel(0x0021000c, 0x00000003);
	ddr_writel(0x002100ac, 0x00000001);
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nDPLL reg 0x");
	ax_print_num(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_DPLL_CFG1_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(DDR_SYS_DPLL_BASE_ADDR + DPLL_CTRL_GLB_DPLL_CFG1_ADDR), 16);
#endif
}

void ddrmc_mrw(u8 mr_cs, u8 mr_ad, u16 mr_op)
{
	u32 reg_data;

#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nwrite mr_cs=0x");
	ax_print_num(mr_cs, 16);
	ax_print_str(", mr_ad=");
	ax_print_num(mr_ad, 10);
	ax_print_str(", mr_op=0x");
	ax_print_num(mr_op, 16);
#endif
	if ((ddr_types & TYPE_DDR4) || (ddr_types & TYPE_DDR3)) {
		reg_mask_set(D_DDRMC_CFG31_ADDR, 0x7 << 24, mr_ad << 24);
		reg_mask_set(D_DDRMC_CFG30_ADDR, 0xf00ffff, (1 << 24) | mr_op);
	} else {
		reg_mask_set(D_DDRMC_CFG30_ADDR, 0xf00ffff, (1 << 24) | mr_ad);
		reg_mask_set(D_DDRMC_CFG31_ADDR, 0xff, mr_op);
	}

	//core0
	reg_data = readl(D_DDRMC_CFG32_ADDR);
	if(mr_cs & (1 << 1)) {
		reg_data &= ~(1 << 24);
		reg_data &= ~(1 << 28);
		if(mr_cs & (1 << 0)) {
			reg_data |= 0x1 << 28;
		}
	} else {
		reg_data |= (1 << 24);//allcs
		reg_data &= ~(1 << 28);
	}
	reg_data |= (1 << 1);//mrw
	ddr_writel(D_DDRMC_CFG32_ADDR, reg_data);

	for (int poll_num = 0; poll_num < 10000; poll_num++) {
		reg_data = readl(D_DDRMC_CFG32_ADDR);
// ### SIPEED EDIT ###
		if (!IS_LPDDR4_DUAL_RANK(ddr_types)) {
			if (0 == (reg_data & (1 << 16))) {
				break;
			}
		} else {
#ifndef AX630C_LPDDR4_DUAL_RANK
		if (0 == (reg_data & (1 << 16))) {
			break;
		}
#endif
		}
// ### SIPEED EDIT ###

		if(0 == (reg_data & (1 << 29))) {
			break;
		}
		if(poll_num == 9999) {
			//ax_printf("[debug] POLL MRW NUM = 999");
		}
	}
}

#define LPDDR4_DRAM_ZQC
#ifdef LPDDR4_DRAM_ZQC
void ddrmc_mpc(u8 mr_cs, u8 mr_op)
{
	u32 reg_data;
	reg_mask_set(D_DDRMC_CFG30_ADDR, 0xf000000, 1 << 24);
	#ifdef AX_DDR_DEBUG
	ax_print_str("\r\n[debug] MPC: mr_cs=0x");
	ax_print_num(mr_cs, 16);
	reg_data = readl(D_DDRMC_CFG30_ADDR);
	ax_print_str("\r\n[debug] MPC: MC_CFG30 = 0x");
	ax_print_num(reg_data, 16);
	ax_print_str("\n");
	#endif

	reg_mask_set(D_DDRMC_CFG31_ADDR, 0xff, mr_op);
	#ifdef AX_DDR_DEBUG
	reg_data = readl(D_DDRMC_CFG31_ADDR);
	ax_print_str("\r\n[debug] MPC: MC_CFG31 = 0x");
	ax_print_num(reg_data, 16);
	ax_print_str("\n");
	#endif

	reg_data = readl(D_DDRMC_CFG32_ADDR);
	if(mr_cs & 0x02) {
		reg_data &= ~(1 << 24);
		reg_data &= ~(1 << 28);
		if (mr_cs & 0x1) {
			reg_data |= (1 << 28);
		}
	} else {
		reg_data |= (1 << 24);
		reg_data &= ~(1 << 28);
	}
	reg_data |= (1 << 6);
	ddr_writel(D_DDRMC_CFG32_ADDR, reg_data);
	#ifdef AX_DDR_DEBUG
	ax_print_str("\r\n[debug] MPC: MC_CFG32 = 0x");
	ax_print_num(reg_data, 16);
	ax_print_str("\n");
	#endif

	for(int poll_num = 0; poll_num < 1000; poll_num++) {
		udelay(1);
		reg_data = readl(D_DDRMC_CFG32_ADDR);
		#ifdef AX_DDR_DEBUG
		ax_print_str("\r\n[debug] POLL MPC DONE: mr_op = 0x");
		ax_print_num(reg_data, 16);
		ax_print_str("\n");
		#endif
		if((reg_data & (1 << 29)) == 0) {
			break;
		}
		if(poll_num == 999) {
			ax_print_str("\r\n[debug] POLL MPC NUM = 999\n");
		}
	}
}
#endif

#if defined(AX630C_DDR4_RETRAIN) || LPDDR4_RETRAIN
void retrain_base_value_f0(void)
{
	u32 retrain_dqeye;
	u32 retrain_pass_win;
	u32 retrain_base = 0;
	u8	cs_num = 0;// rank to be trained, 0: rank0; 1:rank1

	retrain_dqeye = readl(D_DDRMC_TRAIN_RES2_ADDR);
	ddr_writel(D_DDRMC_RETRAIN_DQEYE0_F0_ADDR, retrain_dqeye);

	retrain_pass_win = readl(D_DDRMC_TRAIN_RES3_ADDR);
	for (int i = 0; i < 4; i++) {
		retrain_base |= ((retrain_dqeye >> (i * 8) & 0xFF) + ((retrain_pass_win >> (i * 8) & 0xFF) >> 1) + 1) << (i * 8);
	}
	ddr_writel(D_DDRMC_RETRAIN_BASE0_F0_ADDR, retrain_base);

	if (ddr_types & TYPE_LPDDR4) {
		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100807, (0x9 << 24) | (cs_num << 20) | (1 << 11) | (1 << 2) | (1 << 1));
	} else if ((ddr_types & TYPE_DDR4) && (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16)) {
		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100807, (0x3 << 24) | (cs_num << 20) | (1 << 11) | (1 << 2) | (1 << 1));
	} else if ((ddr_types & TYPE_DDR4) && (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32)) {
		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100807, (0xf << 24) | (cs_num << 20) | (1 << 11) | (1 << 2) | (1 << 1));
	}
	udelay(1);
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100807, 0);
}

void retrain_general_config(void)
{
	reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, 0, 0);//start at the lowest address
	reg_mask_set(D_DDRMC_TRAIN_CTRL3_ADDR, 0xFF0FF7FF, (2 << 24) | 0x80);//rf_train_eye_max_value
	reg_mask_set(D_DDRMC_RETRAIN_CFG0_ADDR, 0xffffffff, 0x116E3600);
	if (ddr_types & TYPE_LPDDR4) {
		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF000000, 0x9 << 24);//rf_train_byte_en
	} else if ((ddr_types & TYPE_DDR4) && (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16)) {
		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF000000, 0x3 << 24);//rf_train_byte_en
	} else if ((ddr_types & TYPE_DDR4) && (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32)) {
		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF000000, 0xf << 24);
	}
	reg_mask_set(D_DDRMC_RETRAIN_CTRL0_ADDR, 0x0, 1 << 13);//rf_retrain_fail_enable
	reg_mask_set(D_DDRMC_TRAIN_CTRL9_ADDR, 0x7FF0000, 5 << 16);
	reg_mask_set(D_DDRMC_TMG16_F0_ADDR, 0, 1 << 30);//rf_retrain_enable
}
#endif

void ddrmc_pre_init(const u32 ddrmc_timing[][29], u8 ddrc_timming_index)
{
	u8 i = 0,j = 0;
	u32 base_addr;

	//stop clock, before reset
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_CLR_ADDR, 1 << EB_1_CLR_CLK_DDRPHY_4X_IN_EB_CLR_LSB | 1 << EB_1_CLR_CLK_DDRPHY_1X_IN_EB_CLR_LSB );
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_CLR_ADDR, 1 << EB_1_CLK_DDRMC_D2_EB_CLR_LSB | 1 << EB_1_CLK_DDRMC_EB_CLR_LSB);
	//reset ddrmc&phy
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_SW_RST_0_SET_ADDR, 1 << SW_RST_0_SET_DDRPHY_SW_RST_SET_LSB);
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_SW_RST_0_SET_ADDR, 1 << SW_RST_0_SET_DDRMC_SW_RST_SET_LSB);

	base_addr = D_DDRMC_PORT0_CFG0_ADDR;
	for (i = 0; i < 24; i++,j++) {
		ddr_writel(base_addr + j*4, ddrmc_port_data[i]);
		if (0 == (i+1)%3) {
			j++;
		}
	}

	base_addr = D_DDRMC_CFG0_ADDR;
	for (i = 0; i < 31; i++) {
		if (i == 20) base_addr += 0x4;
		ddr_writel(base_addr + i *4, ddrmc_cfg_data[ddr_types/TYPE_DDR4][i]);
	}

	base_addr = D_DDRMC_TMG0_F0_ADDR;
	for (i = 0; i < 21; i++) {
		ddr_writel(base_addr + i *4, ddrmc_timing[ddrc_timming_index][i]);
	}

	if (ddr_types & TYPE_LPDDR4) {
		base_addr = D_DDRMC_TMG24_F0_ADDR;
		for (j = 0; i < 29; i++, j++) {
			ddr_writel(base_addr + j *4, ddrmc_timing[ddrc_timming_index][i]);
		}
	}

	if (ddr_types & TYPE_LPDDR4) {
		#if D_HALF_FREQ_MODE
		u32 reg_data;
		reg_data = readl(D_DDRMC_TMG16_F0_ADDR);
		reg_data &= ~(0x3 << 16);//D_HALF_FREQ_MODE
		ddr_writel(D_DDRMC_TMG16_F0_ADDR, reg_data);
		#endif
		ddr_writel(D_DDRMC_CFG29_ADDR, 0x00000960);
		ddr_writel(D_DDRMC_TMG16_F0_ADDR, 0x00010081);
		#if LPDDR4_DFS_CONFIG
		ddrmc_freq1_timing_init();
		ddr_writel(D_DDRMC_TMG16_F1_ADDR, 0x00010081);
		#endif
	}

	if((AX620Q_CHIP_E == chip_id) || (AX620QX_CHIP_E == chip_id) || (chip_id == AX620QZ_CHIP_E) || (chip_id == AX620QP_CHIP_E)) {
		ddr_writel(D_DDRMC_TMG14_F0_ADDR, 0x00000000);
	#if LPDDR4_DFS_CONFIG
		ddr_writel(D_DDRMC_TMG14_F1_ADDR, 0x00000000);
	#endif
	}
	//release phy ddrmc reset
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_SW_RST_0_CLR_ADDR, 1 << SW_RST_0_CLR_DDRPHY_SW_RST_CLR_LSB);
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_SW_RST_0_CLR_ADDR, 1 << SW_RST_0_CLR_DDRMC_SW_RST_CLR_LSB);
	udelay(1);
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_SET_ADDR, 1 << CLK_EB_1_CLK_DDRMC_D2_EB_SET_LSB | 1 << CLK_EB_1_CLK_DDRMC_EB_SET_LSB);
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_SET_ADDR, 1 << EB_1_SET_CLK_DDRPHY_4X_IN_EB_SET_LSB | 1 << EB_1_SET_CLK_DDRPHY_1X_IN_EB_SET_LSB );

}

void ddr3_ddrmc_pos_init(void)
{
	u16 mr_op = 0;
	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, 1 << 1);//drf_clkdmem_out_en
	udelay(500);
	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, (1 << 20));//drf_cke_output_high
	udelay(10);
	if (ddr_freqs == DDR_CLK_1600) {
		mr_op |= 0x3 << 3; //CWL = 8
	} else if (ddr_freqs == DDR_CLK_1866) {
		mr_op |= 0x4 << 3; //CWL = 9
	} else if (ddr_freqs == DDR_CLK_2133) {
		mr_op |= 0x5 << 3; //CWL = 10
	} else {
		mr_op = 0;
	}
	ddrmc_mrw(0x2, 0x2, mr_op);
	ddrmc_mrw(0x2, 0x3, 0x0);
	ddrmc_mrw(0x2, 0x1, 0x0);

	mr_op = 0;
	if (ddr_freqs == DDR_CLK_1600) {
		mr_op = 0xD70; //CL=11, dll reset, WR=12
	} else if (ddr_freqs == DDR_CLK_1866) {
		mr_op = 0xF14; //CL=13, dll reset, WR=14
	} else if (ddr_freqs == DDR_CLK_2133) {
		mr_op = 0x124; //CL=14, dll reset, WR=16
	} else {
		mr_op = 0x520;
	}
	ddrmc_mrw(0x2, 0x0, mr_op);

	udelay(10);//tDLLK & tZQ
	reg_mask_set(D_DDRMC_CFG30_ADDR, 0, 1 << 10);//ZQCL
	reg_mask_set(D_DDRMC_CFG32_ADDR, 0, (1 << 24) | (1 << 4));//allcs zqc
	udelay(10);//tDLLK & tZQ

	reg_mask_set(D_DDRMC_CFG2_ADDR, 0x30037777,  (0x3 << 28) | (1 << 16) | (1 << 12) | (0x5 << 8) | (0x2 << 4) | 0x2);//rf_cs_mode rf_pinmux_mode
}

void ddrmc_pos_init(void)
{
	u8	mr_cs = 0;
	u8	mr_ad;
	u16 mr_op = 0;

	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, 1 << 1);//drf_clkdmem_out_en
	udelay(1);
	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, (1 << 20));//drf_cke_output_high
	udelay(600);
	//MR3
	ddrmc_mrw((0x2 | mr_cs), 0x3, 0x0);

	//MR6
	mr_ad = 0x6;
	mr_op = 0x0;
	if (ddr_freqs == DDR_CLK_1600 || ddr_freqs == DDR_CLK_1866) {
		mr_op |= 0x1 << 10; //tCCD_L=5
	} else if (ddr_freqs == DDR_CLK_2133 || ddr_freqs == DDR_CLK_2400) {
		mr_op |= 0x2 << 10; //tCCD_L=6
	} else if (ddr_freqs == DDR_CLK_2666 || ddr_freqs == DDR_CLK_3200) {
		mr_op |= 0x3 << 10;; //tCCD_L=7
	}
	ddr4_mr6_data_save = mr_op;
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	//MR5
	mr_ad = 0x5;
	mr_op = 0x0;
	mr_op |= (1 << 10);
	// RTT_PARK
	#if WR_ODT_EN
		mr_op |= (6 << 6);
	#endif
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	//MR4
	ddrmc_mrw((0x2 | mr_cs), 0x4, 0x0);

	//MR2
	mr_ad = 0x2;
	mr_op = 0x0;
	if (ddr_freqs == DDR_CLK_1600) {
		mr_op |= 0x0 << 3;
	} else if (ddr_freqs == DDR_CLK_1866) {
		mr_op |= 0x1 << 3;
	} else if (ddr_freqs == DDR_CLK_2133) {
		mr_op |= 0x2 << 3;
	} else if (ddr_freqs == DDR_CLK_2400) {
		mr_op |= 0x3 << 3;
	} else if (ddr_freqs == DDR_CLK_2666) {
		mr_op |= 0x4 << 3;
	} else if (ddr_freqs == DDR_CLK_3200) {
		mr_op |= 0x5 << 3;//MR2 A[5:3] even number//Write CAS Latency WCL;
	}
	// RTT_WR
	#if WR_ODT_EN
		mr_op |= 0x1 << 9;//1 120ohm 2 240ohm
	#endif
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	//MR1
	mr_ad = 0x1;
	mr_op = 0x1;
	// RTT_NOM
	#if WR_ODT_EN
		//mr_op |= 0x3 << 8;//40ohm
	#endif
	#if WR_DRV_EN
		mr_op |= 0x1 << 1;//drv 48ohm
	#endif
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	//MR0
	mr_ad = 0x0;
	mr_op = 0x0;
	if (ddr_freqs == DDR_CLK_1600) {
		mr_op |= (1 << 2) | (1 << 4) | (1 << 8) | (1 << 9);
	} else if (ddr_freqs == DDR_CLK_1866) {
		mr_op |= (1 << 2) | (1 << 5) | (1 << 8) | (2 << 9);
	} else if (ddr_freqs == DDR_CLK_2133) {
		mr_op |= (1 << 2) | (1 << 4) | (1 << 5) | (1 << 8) | (3 << 9);
	} else if (ddr_freqs == DDR_CLK_2400) {
		mr_op |= (1 << 2) | (1 << 4) | (1 << 5) | (1 << 8) | (4 << 9);
	} else if (ddr_freqs == DDR_CLK_2666) {
		mr_op |= (1 << 2) | (1 << 6) | (1 << 8) | (5 << 9);
	} else if (ddr_freqs == DDR_CLK_3200) {
		mr_op |= (1 << 2) | (1 << 4) | (1 << 6) | (1 << 8) |(6 << 9);
	}
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	reg_mask_set(D_DDRMC_CFG30_ADDR, 0, 1 << 10);//ZQCL
	reg_mask_set(D_DDRMC_CFG32_ADDR, 0, (1 << 24) | (1 << 4));//allcs zqc
	udelay(10);//tDLLK & tZQ

	reg_mask_set(D_DDRMC_CFG1_ADDR, 0, 1 << 17);//rf_no_bubble
	reg_mask_set(D_DDRMC_CFG1_ADDR, 0x7 << 12, 0x1 << 12);//rf_bg_mode
	reg_mask_set(D_DDRMC_CFG2_ADDR, 0x7077, (0x1 << 16) | (0x1 << 12) | (0x2 << 4) | (0x2));//rf_bank_mode rf_row_mode rf_col_mode rf_cs_mode

	if(ddr_freqs == DDR_CLK_2400) {
		reg_mask_set(D_DDRMC_TMG9_F0_ADDR, 0xff00, 0xf << 8);//rf_t_faw_f0
	}

	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		reg_mask_set(D_DDRMC_CFG1_ADDR, 1 << 8, 0);//rf_dual_dburst_mode
		reg_mask_set(D_DDRMC_CFG2_ADDR, 0x3000700, 0x4 << 8);//rf_cs_position
		reg_mask_set(D_DDRMC_CFG2_ADDR, 0x3 << 28, 0);//rf_pinmux_mode
	} else {
		reg_mask_set(D_DDRMC_CFG2_ADDR, 0, 0x3 << 28);//rf_pinmux_mode
	}
}

void lpddr4_mrs_config(u8 mr_cs)
{
	u16 mr_ad, mr_op;

	//MR2
	mr_ad = 0x2;
	mr_op = 0x0;
	if (ddr_freqs == DDR_CLK_3733 || ddr_freqs == DDR_CLK_3400) {
		mr_op =	0x36;
	} else if (ddr_freqs == DDR_CLK_3200 || ddr_freqs == DDR_CLK_2800) {
		mr_op =	0x2d;
	} else if (ddr_freqs == DDR_CLK_2666 || ddr_freqs == DDR_CLK_2400) {
		mr_op =	0x24;
	} else if (ddr_freqs == DDR_CLK_2133) {
		mr_op =	0x1b;
	} else if (ddr_freqs == DDR_CLK_1600) {
		mr_op =	0x12;
	} else {
		mr_op =	0x00;
	}
	mr2_status_op = mr_op;
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	//MR1
	mr_ad = 0x1;
	if (ddr_freqs == DDR_CLK_3733 || ddr_freqs == DDR_CLK_3400) {
		mr_op = 0x64;
	} else if (ddr_freqs == DDR_CLK_3200 || ddr_freqs == DDR_CLK_2800) {
		mr_op = 0x54;
	} else if (ddr_freqs == DDR_CLK_2666 || ddr_freqs == DDR_CLK_2400) {
		mr_op = 0x44;
	} else if (ddr_freqs == DDR_CLK_2133) {
		mr_op = 0x34;
	} else if (ddr_freqs == DDR_CLK_1600) {
		mr_op = 0x24;
	} else {
		mr_op =	0x04;
	}
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	//MR3
	mr_ad = 0x3;
	mr_op = (AX620QX_CHIP_E == chip_id) ? 0x30 : 0x31;
	mr3_status_op = mr_op;
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	//MR11
	mr_ad = 0xb;
	mr_op =	0x0;
	#if WR_ODT_EN
		if (ddr_freqs > DDR_CLK_1600)
			mr_op |= (AX620QX_CHIP_E == chip_id) ? (0x4 << 0) : (0x6 << 0);//DQ ODT LP4 40ohm / LP4X 60ohm
	#endif
	if (lpddr4_ca_odt_en)
		mr_op |= (0x2 << 4);//CA ODT 120ohm
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

	//MR22
	#if WR_ODT_EN
		if (ddr_freqs > DDR_CLK_1600) {
			mr_ad = 0x16;

// ### SIPEED EDIT ###
		if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
			if (((ddr_types & 0x300) >> 8) == DDR_2CS)
				mr_op =	0x16;
			else
				mr_op =	(AX620QX_CHIP_E == chip_id) ? (0x4 << 0) : 0x06;
			ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);
		} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
			if (((ddr_types & 0x300) >> 8) == DDR_2CS)
				mr_op =	0x16;
			else
#endif
				mr_op =	(AX620QX_CHIP_E == chip_id) ? (0x4 << 0) : 0x06;
			ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);
		}
// ### SIPEED EDIT ###
		}
	#endif

	if (lpddr4_ca_train_en && lpddr4_ca_odt_en) {
		//MR13 FSP1
		ddrmc_mrw((0x2 | mr_cs), 0xd, 0x48);

		//MR11
		mr_op =	0x0;
		#if WR_ODT_EN
			if (ddr_dfs_sel && (ddr_freqs > DDR_CLK_1600))
				mr_op |= (AX620QX_CHIP_E == chip_id) ? (0x4 << 0) : (0x6 << 0);//DQ ODT 40ohm / LP4X 60ohm
		#endif
		if (lpddr4_ca_odt_en)
			mr_op |= (0x2 << 4);//CA ODT 120ohm
		ddrmc_mrw((0x2 | mr_cs), 0xb, mr_op);
	}

	//MR13 FSP0
	ddrmc_mrw((0x2 | mr_cs), 0xd, 0x8);
}

void lpddr4mc_pos_init(void)
{
	u8 mr_cs;
	u16 mr_ad, mr_op;
	u32 reg_data;
	u32 bit_shift = 0;
#ifdef LPDDR4_DRAM_ZQC
	int i;
	int zqc_cnt = (((ddr_types & 0xC0) >> 6) == DDR_DUAL_CHANNEL) ? 2 : 1;
#endif

// ### SIPEED EDIT ###
	if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
		if ((AX620Q_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id) && (((ddr_types & 0x300) >> 8) == DDR_2CS)) {
			u8 cmd_delay_cycles;

			reg_data = readl(D_DDRPHY_AC_TMG2_F0_ADDR);
			cmd_delay_cycles = (reg_data >> 24) & 0x3;
			reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR, (0x3 << 14), cmd_delay_cycles << 14);
			reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET, (0x3 << 14), cmd_delay_cycles << 14);
		}
	} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if ((AX620Q_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id) && (((ddr_types & 0x300) >> 8) == DDR_2CS)) {
		u8 cmd_delay_cycles;

		reg_data = readl(D_DDRPHY_AC_TMG2_F0_ADDR);
		cmd_delay_cycles = (reg_data >> 24) & 0x3;
		reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR, (0x3 << 14), cmd_delay_cycles << 14);
		reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET, (0x3 << 14), cmd_delay_cycles << 14);
	}
#endif
	}
// ### SIPEED EDIT END ###

	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, 1 << 1);
	//disable clk_ddrmc_d2
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_CLR_ADDR, 1 << EB_1_CLK_DDRMC_D2_EB_CLR_LSB | 1 << EB_1_CLK_DDRMC_EB_CLR_LSB);

// ### SIPEED EDIT ###
if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
	if ((AX620Q_CHIP_E == chip_id) || (AX620QX_CHIP_E == chip_id) || (AX620QZ_CHIP_E == chip_id) || (AX620QP_CHIP_E == chip_id))
		udelay(1);
	else
		udelay(2000);
} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if ((AX620Q_CHIP_E == chip_id) || (AX620QX_CHIP_E == chip_id) || (AX620QZ_CHIP_E == chip_id) || (AX620QP_CHIP_E == chip_id))
#endif
		udelay(1);
#ifdef AX630C_LPDDR4_DUAL_RANK
	else
		udelay(2000);
#endif
}
// ### SIPEED EDIT END ###

	//#1us; //tINIT3 reset to cke high
	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, 1 << 20);
	//resume clk_ddrmc_d2
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_SET_ADDR, 1 << CLK_EB_1_CLK_DDRMC_D2_EB_SET_LSB | 1 << CLK_EB_1_CLK_DDRMC_EB_SET_LSB);
	udelay(6);
	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf0000, 0x1 << 16);
	reg_mask_set(D_DDRMC_CFG6_ADDR, 0x70, 0);

	if (ddr_types == TYPE_LPDDR4_1CS_1CH_X16_2GBIT) {
		bit_shift = 0x2 << 28 | LP4_1CS_1CHAN_2GBIT_X16;//rf_port_pinmux_mode
	} else if (ddr_types == TYPE_LPDDR4_1CS_1CH_X16_4GBIT) {
		bit_shift = 0x2 << 28 | LP4_1CS_1CHAN_4GBIT_X16;//rf_port_pinmux_mode
// ### SIPEED EDIT ###
	} else if (IS_LPDDR4_DUAL_RANK(ddr_types) && ddr_types == TYPE_LPDDR4_2CS_2CH_X32_32GBIT) {
		bit_shift = 0x3 << 28 | LP4_2CS_2CHAN_32GBIT_X32;//rf_port_pinmux_mode
	} else if (IS_LPDDR4_DUAL_RANK(ddr_types) && ddr_types == TYPE_LPDDR4_2CS_2CH_X32_8GBIT) {
		bit_shift = 0x3 << 28 | LP4_2CS_2CHAN_8GBIT_X32;//rf_port_pinmux_mode
#ifdef AX630C_LPDDR4_DUAL_RANK
	} else if (ddr_types == TYPE_LPDDR4_2CS_2CH_X32_32GBIT) {
		bit_shift = 0x3 << 28 | LP4_2CS_2CHAN_32GBIT_X32;//rf_port_pinmux_mode
	} else if (ddr_types == TYPE_LPDDR4_2CS_2CH_X32_8GBIT) {
		bit_shift = 0x3 << 28 | LP4_2CS_2CHAN_8GBIT_X32;//rf_port_pinmux_mode
#endif
	} else if (ddr_types == TYPE_LPDDR4_1CS_2CH_X32_8GBIT) {
		bit_shift = 0x3 << 28 | LP4_1CS_2CHAN_8GBIT_X32;//rf_port_pinmux_mode
	} else if (ddr_types == TYPE_LPDDR4_1CS_2CH_X32_32GBIT) {
		bit_shift = 0x3 << 28 | LP4_1CS_2CHAN_32GBIT_X32;//rf_port_pinmux_mode
// ### SIPEED EDIT END ###
	} else {
		bit_shift = 0x3 << 28 | LP4_1CS_2CHAN_16GBIT_X32;//rf_port_pinmux_mode
	}
	reg_mask_set(D_DDRMC_CFG2_ADDR, 0x33737777, bit_shift);
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nreg 0x");
	ax_print_num(D_DDRMC_CFG2_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRMC_CFG2_ADDR), 16);
#endif

	udelay(100);

	mr_cs = 0x0;
	lpddr4_mrs_config(mr_cs);
// ### SIPEED EDIT ###
	if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
		if (((ddr_types & 0x300) >> 8) == DDR_2CS)
		lpddr4_mrs_config(mr_cs+1);
	} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if (((ddr_types & 0x300) >> 8) == DDR_2CS)
		lpddr4_mrs_config(mr_cs+1);
#endif
	}
// ### SIPEED EDIT END ###
	//MR12
	mr_ad = 0xc;
	if ((AX620Q_CHIP_E == chip_id) || (AX620QX_CHIP_E == chip_id) || (AX620QZ_CHIP_E == chip_id) || (AX620QP_CHIP_E == chip_id))
		mr_op = 0x32;//CA Vref LP4 30% / LP4X 44.9%
	else
		mr_op = 0x26;
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);
// ### SIPEED EDIT ###
	if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
		if (((ddr_types & 0x300) >> 8) == DDR_2CS)
			ddrmc_mrw((0x2 | (mr_cs+1)), mr_ad, mr_op);
	} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if (((ddr_types & 0x300) >> 8) == DDR_2CS)
		ddrmc_mrw((0x2 | (mr_cs+1)), mr_ad, mr_op);
#endif
	}
// ### SIPEED EDIT END ###
	//MR14
	mr_ad = 0xe;
	if ((AX620Q_CHIP_E == chip_id) || (AX620QZ_CHIP_E == chip_id) || (AX620QP_CHIP_E == chip_id))
		mr_op = 0x24;//DQ Vref 24.4%
	else if (AX620QX_CHIP_E == chip_id)
		mr_op = 0x14;//DQ Vref 26.9%
	else
		mr_op = 0x16;//DQ Rref 18.8%
	ddrmc_mrw((0x2 | mr_cs), mr_ad, mr_op);

// ### SIPEED EDIT ###
	if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
		if (((ddr_types & 0x300) >> 8) == DDR_2CS)
			ddrmc_mrw((0x2 | (mr_cs+1)), mr_ad, mr_op);
	} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if (((ddr_types & 0x300) >> 8) == DDR_2CS)
		ddrmc_mrw((0x2 | (mr_cs+1)), mr_ad, mr_op);
#endif
	}
// ### SIPEED EDIT END ###
	reg_mask_set(D_DDRMC_CFG8_ADDR, 0x7fff7fff, 0);

	#if DRAM_LOWPOWER_EN
		reg_data = readl(D_DDRMC_TMG17_F0_ADDR);
		reg_data |= 0x3;//drf_auto_pwr_down_en drf_auto_pwr_down_percs_en
		ddr_writel(D_DDRMC_TMG17_F0_ADDR, reg_data);

		reg_data = readl(D_DDRMC_TMG18_F0_ADDR);
		reg_data |= 0xF000003;
		ddr_writel(D_DDRMC_TMG18_F0_ADDR, reg_data);
	#endif
	if (lpddr4_ca_odt_en) {
		#ifdef AX_DDR_DEBUG
		ax_print_str("\r\nreg = 0x");
		ax_print_num(D_DDRMC_TMG18_F0_ADDR, 16);
		ax_print_str(", val = 0x");
		ax_print_num(readl(D_DDRMC_TMG18_F0_ADDR), 16);
		#endif

		reg_data = readl(D_DDRMC_TMG18_F0_ADDR);
		reg_data |= 0x8000000;
		ddr_writel(D_DDRMC_TMG18_F0_ADDR, reg_data);
		#ifdef AX_DDR_DEBUG
		ax_print_str("\r\nreg = 0x");
		ax_print_num(D_DDRMC_TMG18_F0_ADDR, 16);
		ax_print_str(", val = 0x");
		ax_print_num(readl(D_DDRMC_TMG18_F0_ADDR), 16);
		#endif

		reg_data = readl(D_DDRMC_TMG18_F0_ADDR);
		reg_data |= 0x7000000;
		ddr_writel(D_DDRMC_TMG18_F0_ADDR, reg_data);
		#ifdef AX_DDR_DEBUG
		ax_print_str("\r\nreg = 0x");
		ax_print_num(D_DDRMC_TMG18_F0_ADDR, 16);
		ax_print_str(", val = 0x");
		ax_print_num(readl(D_DDRMC_TMG18_F0_ADDR), 16);
		#endif
	}

#ifdef LPDDR4_DRAM_ZQC
	for (i = 0; i < zqc_cnt; i++) {
		if (2 == zqc_cnt) {
			if (0 == i) {
// ### SIPEED EDIT ###
				if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
					if (((ddr_types & 0x300) >> 8) == DDR_2CS)
						ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe0c9a98);
				} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
				if (((ddr_types & 0x300) >> 8) == DDR_2CS)
					ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe0c9a98);
#endif
				}
// ### SIPEED EDIT END ###
				ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76500111);	//(1)csb0 -> csb1  do zqc (chn A)
				ax_print_str("\r\nchn A ZQC:");
			}
			else {
// ### SIPEED EDIT ###
				if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
					if (((ddr_types & 0x300) >> 8) == DDR_2CS) {
						ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe1c9a98);
						ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76501010);
					} else {
						ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76501011);	//(2)csb0 -> csb0,   csa0 -> csa1 do zqc (chnB)
					}
				} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
					if (((ddr_types & 0x300) >> 8) == DDR_2CS) {
						ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe1c9a98);
						ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76501010);
					}
					else
#endif
						ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76501011);	//(2)csb0 -> csb0,   csa0 -> csa1 do zqc (chnB)
				}
// ### SIPEED EDIT END ###
				ax_print_str("\r\nchn B ZQC:");
			}
#ifdef AX_DDR_DEBUG
			ax_print_str("\r\nreg 0x");
			ax_print_num(D_DDRPHY_AC_CFG3_ADDR, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_AC_CFG3_ADDR), 16);
			ax_print_str("\r\nreg 0x");
			ax_print_num(D_DDRPHY_AC_CFG4_ADDR, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_AC_CFG4_ADDR), 16);
			ax_print_str("\r\nreg 0x");
			ax_print_num(D_DDRPHY_AC_CFG5_ADDR, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_AC_CFG5_ADDR), 16);
#endif
		}
		ddrmc_mpc((0x2 | mr_cs), 0x4f);
		udelay(1);
		ddrmc_mpc((0x2 | mr_cs), 0x51);
		udelay(1);
	}
	if (2 == zqc_cnt) {
		ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76500011);
// ### SIPEED EDIT ###
if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
	if (((ddr_types & 0x300) >> 8) == DDR_2CS)
		ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe1c9a98);
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\ndefaut config:");
	ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76500011);
	ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe1c9a98);
	ax_print_str("\r\nreg 0x");
	ax_print_num(D_DDRPHY_AC_CFG3_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRPHY_AC_CFG3_ADDR), 16);
	ax_print_str("\r\nreg 0x");
	ax_print_num(D_DDRPHY_AC_CFG4_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRPHY_AC_CFG4_ADDR), 16);
	ax_print_str("\r\nreg 0x");
	ax_print_num(D_DDRPHY_AC_CFG5_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRPHY_AC_CFG5_ADDR), 16);
#endif
} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if (((ddr_types & 0x300) >> 8) == DDR_2CS)
		ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe1c9a98);
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\ndefaut config:");
	ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76500011);
	ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe1c9a98);
	ax_print_str("\r\nreg 0x");
	ax_print_num(D_DDRPHY_AC_CFG3_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRPHY_AC_CFG3_ADDR), 16);
	ax_print_str("\r\nreg 0x");
	ax_print_num(D_DDRPHY_AC_CFG4_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRPHY_AC_CFG4_ADDR), 16);
	ax_print_str("\r\nreg 0x");
	ax_print_num(D_DDRPHY_AC_CFG5_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRPHY_AC_CFG5_ADDR), 16);
#endif
#endif
}
// ### SIPEED EDIT END ###
	}
#endif
}

static void ddrphy_dq_deskew_pre_set(void)
{
	u32 reg_base = D_DDRPHY_DS0_CFG6_ADDR;
	if (TYPE_LPDDR4 & ddr_types) {
		for (int i = 0; i < 48; i++) {
			if (12 == i)
				reg_base = D_DDRPHY_DS1_CFG6_ADDR;
			else if (24 == i)
				reg_base = D_DDRPHY_DS0_CFG6_ADDR + DDR_PHY_OFFSET;
			else if (36 == i)
				reg_base = D_DDRPHY_DS1_CFG6_ADDR + DDR_PHY_OFFSET;
			if ((AX620Q_CHIP_E == chip_id) || (AX620QX_CHIP_E == chip_id) || (AX620QZ_CHIP_E == chip_id) || (AX620QP_CHIP_E == chip_id))
				ddr_writel(reg_base + (i % 12) * 4, ax620q_lpddr4_dq_deskew_pre_set[i]);
			else
				ddr_writel(reg_base + (i % 12) * 4, ax630c_lpddr4_dq_deskew_pre_set[i]);
		}
	} else {
		ddr_writel(D_DDRPHY_AC_CFG11_ADDR, 13 << 16);
		ddr_writel(D_DDRPHY_AC_CFG12_ADDR, 17 | (24 << 24));
		ddr_writel(D_DDRPHY_AC_CFG13_ADDR, 1 | (29 << 16));
		ddr_writel(D_DDRPHY_AC_CFG14_ADDR, (4 << 16) | (9 << 24));
		ddr_writel(D_DDRPHY_AC_CFG11_ADDR + DDR_PHY_OFFSET, 6 | (20 << 24));
		ddr_writel(D_DDRPHY_AC_CFG12_ADDR + DDR_PHY_OFFSET, (24 << 8) | (11 << 24));
		ddr_writel(D_DDRPHY_AC_CFG13_ADDR + DDR_PHY_OFFSET, 2 | (17 << 16));
		ddr_writel(D_DDRPHY_AC_CFG14_ADDR + DDR_PHY_OFFSET, 25);
	}
}

void ddrphy_pos_init(void)
{
	u32 bit_shift = 0;
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR, 0, 1 << 17);
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET, 0, 1 << 17);
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR, 1 << 17, 0);
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET, 1 << 17, 0);

	if (ddr_types & TYPE_DDR4) {
		//CA pattern
		ddr_writel(D_DDRPHY_AC_CFG1_ADDR, 0x74a42b72);
		ddr_writel(D_DDRPHY_AC_CFG2_ADDR, 0x00000008);
		ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x00060b00);
		ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0x0e508000);
		ddr_writel(D_DDRPHY_AC_CFG5_ADDR, 0x480d00c8);
		ddr_writel(D_DDRPHY_AC_CFG1_ADDR + DDR_PHY_OFFSET, 0x000630c5);
		ddr_writel(D_DDRPHY_AC_CFG2_ADDR + DDR_PHY_OFFSET, 0x00000000);
		ddr_writel(D_DDRPHY_AC_CFG3_ADDR + DDR_PHY_OFFSET, 0x9d100000);
		ddr_writel(D_DDRPHY_AC_CFG4_ADDR + DDR_PHY_OFFSET, 0x00000093);
		ddr_writel(D_DDRPHY_AC_CFG5_ADDR + DDR_PHY_OFFSET, 0x00800019);
		ddr_writel(D_DDRPHY_DS0_CFG0_ADDR, 0x72661328);
		ddr_writel(D_DDRPHY_DS0_CFG1_ADDR, 0x75748156);
		ddr_writel(D_DDRPHY_DS0_CFG2_ADDR, 0x3b005c14);
		ddr_writel(D_DDRPHY_DS1_CFG0_ADDR, 0x50145078);
		ddr_writel(D_DDRPHY_DS1_CFG1_ADDR, 0x14382632);
		ddr_writel(D_DDRPHY_DS1_CFG2_ADDR, 0xa1100f03);
		ddr_writel(D_DDRPHY_DS0_CFG0_ADDR + DDR_PHY_OFFSET, 0x74125810);
		ddr_writel(D_DDRPHY_DS0_CFG1_ADDR + DDR_PHY_OFFSET, 0x20386410);
		ddr_writel(D_DDRPHY_DS0_CFG2_ADDR + DDR_PHY_OFFSET, 0xc813e404);
		ddr_writel(D_DDRPHY_DS1_CFG0_ADDR + DDR_PHY_OFFSET, 0x63028736);
		ddr_writel(D_DDRPHY_DS1_CFG1_ADDR + DDR_PHY_OFFSET, 0x77861455);
		ddr_writel(D_DDRPHY_DS1_CFG2_ADDR + DDR_PHY_OFFSET, 0x6d02b215);

		reg_mask_set(D_DDRMC_TRAIN_CTRL8_ADDR, 0, 1 << 28);
	#if WR_ODT_EN
		reg_mask_set(D_DDRPHY_GEN_TMG9_F0_ADDR, 0x7 << 12, 0x6 << 12);
		reg_mask_set(D_DDRPHY_GEN_TMG9_F0_ADDR + DDR_PHY_OFFSET, 0x7 << 12, 0x6 << 12);
	#endif
	} else if (ddr_types & TYPE_LPDDR4) {
		if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
			ddr_writel(D_DDRPHY_DS0_CFG0_ADDR, 0x72545466);
			ddr_writel(D_DDRPHY_DS0_CFG1_ADDR, 0x81542646);
			ddr_writel(D_DDRPHY_DS0_CFG2_ADDR, 0x0b108d07);
			ddr_writel(D_DDRPHY_DS1_CFG0_ADDR, 0x30218318);
			ddr_writel(D_DDRPHY_DS1_CFG1_ADDR, 0x70327518);
			ddr_writel(D_DDRPHY_DS1_CFG2_ADDR, 0x0f13aa10);
			ddr_writel(D_DDRPHY_DS0_CFG0_ADDR + DDR_PHY_OFFSET, 0x81381203);
			ddr_writel(D_DDRPHY_DS0_CFG1_ADDR + DDR_PHY_OFFSET, 0x86235131);
			ddr_writel(D_DDRPHY_DS0_CFG2_ADDR + DDR_PHY_OFFSET, 0xf4075510);
			ddr_writel(D_DDRPHY_DS1_CFG0_ADDR + DDR_PHY_OFFSET, 0x66454527);
			ddr_writel(D_DDRPHY_DS1_CFG1_ADDR + DDR_PHY_OFFSET, 0x07450268);
			ddr_writel(D_DDRPHY_DS1_CFG2_ADDR + DDR_PHY_OFFSET, 0xf004b107);
		} else {
			ddr_writel(D_DDRPHY_DS0_CFG0_ADDR, 0x65483210);
			ddr_writel(D_DDRPHY_DS0_CFG1_ADDR, 0x87653210);
			ddr_writel(D_DDRPHY_DS0_CFG2_ADDR, 0x00040007);
			ddr_writel(D_DDRPHY_DS1_CFG0_ADDR + DDR_PHY_OFFSET, 0x01238456);
			ddr_writel(D_DDRPHY_DS1_CFG1_ADDR + DDR_PHY_OFFSET, 0x80124567);
			ddr_writel(D_DDRPHY_DS1_CFG2_ADDR + DDR_PHY_OFFSET, 0xFF13FF17);

			reg_mask_set(D_DDRMC_CFG31_ADDR, 0xf << 16, 0x1 << 16);
			reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET, 0, 0x1);//clk inv
			#if LPDDR4_DFS_CONFIG
				reg_mask_set(D_DDRPHY_GEN_TMG13_F1_ADDR + DDR_PHY_OFFSET, 0, 0x1);//clk inv
			#endif
			reg_mask_set(D_DDRPHY_DS1_CFG3_ADDR, 0, 1 << 31);//phya dslice 1 clk off
			reg_mask_set(D_DDRPHY_DS0_CFG3_ADDR + DDR_PHY_OFFSET, 0, 1 << 31);////phyb dslice 0 clk off
		}
	}
	#if CA_DRV_EN
		if (0 == lpddr4_ca_odt_en) {
			bit_shift = (6 << 4) | 0x6;//40ohm
		} else {
			bit_shift = (7 << 4) | 0x7;//34ohm
		}
		reg_mask_set(D_DDRPHY_AC_CFG0_ADDR, 0x77, bit_shift);
		reg_mask_set(D_DDRPHY_AC_CFG0_ADDR + DDR_PHY_OFFSET, 0x77, bit_shift);
	#endif

	#if WR_DRV_EN
		reg_mask_set(D_DDRPHY_GEN_TMG9_F0_ADDR, 0x77, (6 << 4) | 0x6);//40ohm
		reg_mask_set(D_DDRPHY_GEN_TMG9_F0_ADDR + DDR_PHY_OFFSET, 0x77, (6 << 4) | 0x6);//40ohm
	#if LPDDR4_DFS_CONFIG
		reg_mask_set(D_DDRPHY_GEN_TMG9_F1_ADDR, 0x77, (6 << 4) | 0x6);//40ohm
		reg_mask_set(D_DDRPHY_GEN_TMG9_F1_ADDR + DDR_PHY_OFFSET, 0x77, (6 << 4) | 0x6);//40ohm
	#endif
	#endif

	ddrphy_dq_deskew_pre_set();
}

void ddr3_ddrphy_pos_init(void)
{
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR, 0, 1 << 17);
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET, 0, 1 << 17);
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR, 1 << 17, 0);
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET, 1 << 17, 0);
	reg_mask_set(D_DDRMC_TRAIN_CTRL8_ADDR, 0, 1 << 28);
}

void ddr3_ddrphy_pre_init(void)
{
	u32 reg_data;

	ddrphy_zqc_seq();
	//pinmux config
	ddr_writel(D_DDRPHY_AC_CFG1_ADDR, 0x5203c64d);
	ddr_writel(D_DDRPHY_AC_CFG2_ADDR, 0x0000000e);
	ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x00099601);
	ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0x0eb0b000);
	ddr_writel(D_DDRPHY_AC_CFG5_ADDR, 0x480c0080);
	ddr_writel(D_DDRPHY_AC_CFG1_ADDR + DDR_PHY_OFFSET, 0x000af740);
	ddr_writel(D_DDRPHY_AC_CFG2_ADDR + DDR_PHY_OFFSET, 0x00000000);
	ddr_writel(D_DDRPHY_AC_CFG3_ADDR + DDR_PHY_OFFSET, 0x73500000);
	ddr_writel(D_DDRPHY_AC_CFG4_ADDR + DDR_PHY_OFFSET, 0x00000082);
	ddr_writel(D_DDRPHY_AC_CFG5_ADDR + DDR_PHY_OFFSET, 0x01c00003);
	ddr_writel(D_DDRPHY_DS0_CFG0_ADDR, 0x77412460);
	ddr_writel(D_DDRPHY_DS0_CFG1_ADDR, 0x71755486);
	ddr_writel(D_DDRPHY_DS0_CFG2_ADDR, 0x2f115d13);
	ddr_writel(D_DDRPHY_DS1_CFG0_ADDR, 0x50326588);
	ddr_writel(D_DDRPHY_DS1_CFG1_ADDR, 0x63228340);
	ddr_writel(D_DDRPHY_DS1_CFG2_ADDR, 0x60100d01);
	ddr_writel(D_DDRPHY_DS0_CFG0_ADDR + DDR_PHY_OFFSET, 0x83285236);
	ddr_writel(D_DDRPHY_DS0_CFG1_ADDR + DDR_PHY_OFFSET, 0x20301268);
	ddr_writel(D_DDRPHY_DS0_CFG2_ADDR + DDR_PHY_OFFSET, 0x92077000);
	ddr_writel(D_DDRPHY_DS1_CFG0_ADDR + DDR_PHY_OFFSET, 0x11570764);
	ddr_writel(D_DDRPHY_DS1_CFG1_ADDR + DDR_PHY_OFFSET, 0x41586573);
	ddr_writel(D_DDRPHY_DS1_CFG2_ADDR + DDR_PHY_OFFSET, 0xf304ba14);
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR, 0x0, 1 << 2);//ddr3 mode
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET, 0x0, 1 << 2);
	ddrphy_timing_init(0x0, ddr3phy_timing, ddr_timming_index); //phya
	ddrphy_timing_init(DDR_PHY_OFFSET, ddr3phy_timing, ddr_timming_index); //phyb
#if MODE_2T
	if(ddr_freqs == DDR_CLK_2133)
		reg_mask_set(D_DDRMC_TMG16_F0_ADDR, 0, 1 << 28);//rf_2t_mode_f0
#endif
	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf, 0x3);//rf_dfi_init_start
	for (int poll_num = 0; poll_num< 1000; poll_num++) {
		reg_data = readl(D_DDRMC_CFG0_ADDR);
		if(reg_data & (0x1 << 8)) {
			break;
		}
	}
	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf, 0);
	reg_mask_set(D_DDRMC_CFG22_ADDR, 1 << 21, 0);
	udelay(1);
	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, 1 << 21);

	ddr3_ddrphy_pos_init();
}
void ddrphy_pre_init(void)
{
	u32 reg_data;

#if DDRPHY_ZQC_CONFIG
	ddrphy_zqc_seq();
#endif
	//DDR4 mode
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR, 0, 1 << 3);
	reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET, 0, 1 << 3);
	//stop clock, before reset phy
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_CLR_ADDR, 1 << EB_1_CLR_CLK_DDRPHY_4X_IN_EB_CLR_LSB | 1 << EB_1_CLR_CLK_DDRPHY_1X_IN_EB_CLR_LSB );
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_SW_RST_0_SET_ADDR, 1 << SW_RST_0_SET_DDRPHY_SW_RST_SET_LSB );

	ddrphy_timing_init(0x0, ddr4phy_timing, ddr_timming_index); //phya
	ddrphy_timing_init(DDR_PHY_OFFSET, ddr4phy_timing, ddr_timming_index); //phyb
	#if MODE_2T
		if (ddr_freqs >= DDR_CLK_2666) {
			reg_mask_set(D_DDRMC_TMG16_F0_ADDR, 0, 1 << 28);//rf_2t_mode_f0
		}
	#endif
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_SW_RST_0_CLR_ADDR, 1 << SW_RST_0_CLR_DDRPHY_SW_RST_CLR_LSB);
	udelay(1);
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_SET_ADDR, 1 << EB_1_SET_CLK_DDRPHY_4X_IN_EB_SET_LSB | 1 << EB_1_SET_CLK_DDRPHY_1X_IN_EB_SET_LSB );

	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf, 0x3);//rf_dfi_init_start
	for (int poll_num = 0; poll_num< 1000; poll_num++) {
		reg_data = readl(D_DDRMC_CFG0_ADDR);
		if(reg_data & (0x1 << 8)) {
		  break;
		}
	}
	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf, 0);
	reg_mask_set(D_DDRMC_CFG22_ADDR, 1 << 21, 0);
	udelay(1);
	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, 1 << 21);

	ddrphy_pos_init();
}

void lpddr4phy_pre_init(void)
{
	u32 reg_data;
#if DDRPHY_ZQC_CONFIG
	ddrphy_zqc_seq();
#endif
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		ddr_writel(D_DDRPHY_AC_CFG1_ADDR, 0x32543200);
		ddr_writel(D_DDRPHY_AC_CFG2_ADDR, 0x76543212);
		ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76500011);
		ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfe1c9a98);
		ddr_writel(D_DDRPHY_AC_CFG5_ADDR, 0xffff0000);
		ddr_writel(D_DDRPHY_AC_CFG1_ADDR + DDR_PHY_OFFSET, 0x76543211);
		ddr_writel(D_DDRPHY_AC_CFG2_ADDR + DDR_PHY_OFFSET, 0x76543210);
		ddr_writel(D_DDRPHY_AC_CFG3_ADDR + DDR_PHY_OFFSET, 0x76543210);
		ddr_writel(D_DDRPHY_AC_CFG4_ADDR + DDR_PHY_OFFSET, 0xfedcbaa8);
		ddr_writel(D_DDRPHY_AC_CFG5_ADDR + DDR_PHY_OFFSET, 0xffff0000);
	} else {
		ddr_writel(D_DDRPHY_AC_CFG1_ADDR, 0x34543210);
		ddr_writel(D_DDRPHY_AC_CFG2_ADDR, 0x76543212);
		ddr_writel(D_DDRPHY_AC_CFG3_ADDR, 0x76543211);
		ddr_writel(D_DDRPHY_AC_CFG4_ADDR, 0xfedcba98);
		ddr_writel(D_DDRPHY_AC_CFG5_ADDR, 0xffff0000);
		ddr_writel(D_DDRPHY_AC_CFG1_ADDR + DDR_PHY_OFFSET, 0x76501210);
		ddr_writel(D_DDRPHY_AC_CFG2_ADDR + DDR_PHY_OFFSET, 0x76543210);
		ddr_writel(D_DDRPHY_AC_CFG3_ADDR + DDR_PHY_OFFSET, 0x76543210);
		ddr_writel(D_DDRPHY_AC_CFG4_ADDR + DDR_PHY_OFFSET, 0xfedcba90);
		ddr_writel(D_DDRPHY_AC_CFG5_ADDR + DDR_PHY_OFFSET, 0xffff0000);
	}

	//TYPE_LPDDR4 mode
	if (AX620QX_CHIP_E == chip_id) {
		reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR, 0, (1 << 1) | (1 << 4));
		reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET, 0, (1 << 1) | (1 << 4));
	} else {
		reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR, 0, 1 << 1);
		reg_mask_set(D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET, 0, 1 << 1);
	}

	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_CLR_ADDR, 1 << EB_1_CLR_CLK_DDRPHY_4X_IN_EB_CLR_LSB | 1<< EB_1_CLR_CLK_DDRPHY_1X_IN_EB_CLR_LSB);
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_SW_RST_0_SET_ADDR, 1 << SW_RST_0_SET_DDRPHY_SW_RST_SET_LSB);
	ddrphy_timing_init(0x0, lpddr4phy_timing, ddr_timming_index); //phya
	ddrphy_timing_init(DDR_PHY_OFFSET, lpddr4phy_timing, ddr_timming_index); //phyb
#if LPDDR4_DFS_CONFIG
	ddrphy_freq1_timing_init(0,lpddr4phy_timing);//1600Hz
	ddrphy_freq1_timing_init(DDR_PHY_OFFSET,lpddr4phy_timing);//1600Hz
#endif
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_SW_RST_0_CLR_ADDR, 1 << SW_RST_0_CLR_DDRPHY_SW_RST_CLR_LSB);
	udelay(1);
	//turn on clock
	ddr_writel(DDR_SYS_GLB_BASE_ADDR + DDR_SYS_GLB_CLK_EB_1_SET_ADDR, 1 << EB_1_SET_CLK_DDRPHY_4X_IN_EB_SET_LSB | 1<< EB_1_SET_CLK_DDRPHY_1X_IN_EB_SET_LSB);

	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf, 0x3);//rf_dfi_init_start

	for (int poll_num = 0; poll_num < 1000; poll_num++) {
		reg_data = readl(D_DDRMC_CFG0_ADDR);
			if(reg_data & (0x3 << 8) ) {
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
			reg_bit_set(DDR_DUMMY_REG_SW1, 1, 1, 0x1);
#endif
				break;
			}
		if(poll_num == 999) {
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
			reg_bit_set(DDR_DUMMY_REG_SW0, 1, 1, 0x1);
#endif
		  ax_print_str("POLL DFI NUM = 999\n");
		}
	}

	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf, 0);
	reg_mask_set(D_DDRMC_CFG22_ADDR, 1 << 21, 0);//ddr_rst
	udelay(21); // tINIT1
	reg_mask_set(D_DDRMC_CFG22_ADDR, 0, 1 << 21);//ddr_rst

	ddrphy_pos_init();
}

#ifdef AUTO_DERATE
static void ddrmc_auto_derate(void)
{
	u32 reg_data;

	if (TYPE_LPDDR4 & ddr_types) {
		reg_data = readl(D_DDRMC_CFG22_ADDR);
		reg_data |= (1 << 24);//rf_auto_mr4_en
#ifdef D_DDR4_T12_FINAL_PINMUX_L
		reg_data &= ~(1 << 25);//rf_auto_mr4_allcs
#else
		reg_data |= (1 << 25);//rf_auto_mr4_allcs
#endif
		ddr_writel(D_DDRMC_CFG22_ADDR, reg_data);

		reg_data = readl(D_DDRMC_CFG28_ADDR);
		reg_data &= ~0xffffff;//rf_t_mr4
		reg_data |= 0x10E;//rf_t_mr4 10us/38.46ns=270
		ddr_writel(D_DDRMC_CFG28_ADDR, reg_data);
	}
}
#endif

static u32 ddr_freq_search(const u32 ddr_freqs[][2], u32 ddr_freqs_len)
{
	u8 i = 0;
	u32 ddr_freq_temp = 0;
	for(i = 0; i < ddr_freqs_len; i++) {
		if (1 == ddr_freqs[i][0]) {
			ddr_freq_temp = ddr_freqs[i][1];
			break;
		}
	}
	return ddr_freq_temp;
}

#ifdef DDR_BIST_EN
static void bist_init(u32 write_or_read, u32 data_pattern_mode, u32 bist_data_mode, u32 bist_size, u32 bist_src)
{
	u32 i = 0;

	ddr_writel(D_DDRMC_BIST_CTRL0_ADDR, 0x0);
	//bist enable
	reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 0, 1, 0x1);
	//bist_mode, 2'b00:write 2'01 read, 2'b10 all read after all write 2'b11 one read after one write
	reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 4, 2, write_or_read);
	//bist_data_pattern_mode, 2'b00:user data pattern, 2'b01:sipi data pattern, 2'b10:lfsr data pattern
	reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 8, 2, data_pattern_mode);
	//bist_burst_length
	reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 12, 4, bist_burst_length);
	//bist_data_size, 3'b000:byte, 3'b001:half word, 3'b010:word, 3'b011:two words, 3'b100:four words, 3'b101:eight words
	reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 20, 3, bist_data_mode);
	//bist_write_outstanding_en/bist_read_outstanding_en
	if(write_or_read == 3) {
		reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 24, 2, 0x0);
	}
	else {
		reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 24, 2, 0x3);
	}
	//datawidth_mode, 1'b0 16bit, 1'b1 32bit
	#if D_DDR_64BIT
		reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 28, 2, 2);
	#elif D_DDR4_X8
		reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 28, 2, 1);
	#else
		reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 28, 2, 0);
	#endif


	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32)  {
		reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 28, 2, 1);
	}
	//bist_trans_num
	ddr_writel(D_DDRMC_BIST_CTRL2_ADDR, bist_size);
	//bist_start_addr
	ddr_writel(D_DDRMC_BIST_CTRL3_ADDR, bist_src);
	switch(data_pattern_mode)
	{
		case USER_DATA_PATTERN:
			for(i = 0; i < 32; i++)
			{
				ddr_writel((D_DDRMC_BIST_DATA0_ADDR+i*4), user_mode_pattern[i]);
			}
			break;
		case SIPI_DATA_PATTERN:
			for(i = 0; i < 16; i++)
			{
				ddr_writel((D_DDRMC_BIST_DATA0_ADDR+i*4), sipi_mode_pattern[2+i]);
				ddr_writel((D_DDRMC_BIST_DATA16_ADDR+i*4), ~sipi_mode_pattern[2+i]);
			}
			reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 26, 1, 1);      //rf_sipi_auto_mode
			reg_bit_set(D_DDRMC_BIST_CTRL1_ADDR, 12, 10, 0x150); //rf_sipi_data_pattern_bit_num 336
			reg_bit_set(D_DDRMC_BIST_CTRL1_ADDR, 24, 4, 0x0);   //rf_sipi_data_auto_mode
			ddr_writel(D_DDRMC_BIST_SIPI_BIT_PATTERN0_ADDR, sipi_mode_pattern[0]); //rf_bist_sipi_bit_pattern_0
			ddr_writel(D_DDRMC_BIST_SIPI_BIT_PATTERN1_ADDR, sipi_mode_pattern[1]); //rf_bist_sipi_bit_pattern_1
			break;
		case LFSR_DATA_PATTERN:
			for(i = 0; i < 4; i++)
			{
				ddr_writel((D_DDRMC_BIST_LFSR_SEED0_ADDR+i*4), lfsr_mode_pattern[i]);
			}
			break;
	}
}

static void print_bist_fail(void)
{
	u32 i = 0;
	u32 expt_data = 0;
	u32 real_data = 0;

	//bist fail cnt
	ax_print_str(" fail cnt: ");
	ax_print_num(readl(D_DDRMC_BIST_FAIL_CNT_ADDR), 10);
	ax_print_str(" fail addr: ");
	ax_print_num(readl(D_DDRMC_BIST_FAIL_ADDR_ADDR), 16);
	for (i = 0; i < 4; i++) {
		expt_data = readl(D_DDRMC_BIST_EXPT_DATA0_ADDR + i * 4);
		real_data = readl(D_DDRMC_BIST_FAIL_DATA0_ADDR + i * 4);
//		if (real_data != expt_data) {
		if (1) {
			ax_print_str("\r\nexpt: ");
			ax_print_num(expt_data, 16);
			ax_print_str("real: ");
			ax_print_num(real_data, 16);
		}
	}
	for (i = 0; i < 4; i++) {
		expt_data = readl(D_DDRMC_BIST_EXPT_DATA4_ADDR + i * 4);
		real_data = readl(D_DDRMC_BIST_FAIL_DATA4_ADDR + i * 4);
//		if (real_data != expt_data) {
		if (1) {
			ax_print_str("\r\nexpt: ");
			ax_print_num(expt_data, 16);
			ax_print_str("real: ");
			ax_print_num(real_data, 16);
		}
	}
	for (i = 0; i < 4; i++) {
		expt_data = readl(D_DDRMC_BIST_FAIL_ALL_DATA0_ADDR + i * 4);
		if (1) {
			ax_print_str("\r\nfail_sum: ");
			ax_print_num(expt_data, 16);
		}
	}
}

static int bist_test(void)
{
	//bist_clear
	reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 1, 2, 0x2);
	reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 1, 2, 0x0);
	//bist_trigger
	reg_bit_set(D_DDRMC_BIST_CTRL0_ADDR, 1, 1, 0x1);
	//bist_wait_done
	while((readl(D_DDRMC_BIST_RESULT_ADDR) & 0x1) != 0x1);
	udelay(1);
	while((readl(D_DDRMC_BIST_RESULT_ADDR) & 0x1) != 0x1);

	if((readl(D_DDRMC_BIST_RESULT_ADDR) & 0x2) == 0x0)
	{
		ax_print_str("bist pass\r\n");
		return 0;
	} else
	{
		ax_print_str("bist fail\r\n");
		print_bist_fail();
//		while (1);
		return 1;
	}
}

static int dram_bist_test(u32 start)
{
	//bist clear
	//dram_info.cs0_size = 0x10;
	//dram_info.cs1_size = 0x10;
	//u32 bist_addr = dram_info.cs0_size; //dram_info.cs1_size};
	u32 bist_addr = 0x0; //dram_info.cs1_size};
	u32 size;
	//u32 bist_result = 0;
	int ret = 0;
//	size = 0xfffffe;//512x16 all memory space
//	size = 0x7ffffe;//256x16 all memory space
	size = 0xffffe;//32x16 all memory space
//	size = 0x0fe;
	ddr_writel(D_DDRMC_BIST_CTRL4_ADDR, 0x0); //data_0 mask
	ddr_writel(D_DDRMC_BIST_CTRL5_ADDR, 0x0); //data_1 mask
	reg_bit_set(D_DDRMC_TRAIN_CTRL0_ADDR, 2, 1, 0x1);//rf_train_enable
	//size = (u32)(dram_info.cs0_size + dram_info.cs1_size);
	if(start % 6==0) {
		ax_print_str("BIST_ALLWRC LFSR_DATA_PATTERN\r\n");
		bist_init(BIST_ALLWRC, LFSR_DATA_PATTERN, bist_data_size, size, bist_addr);
	}
	else if(start % 6==1) {
		ax_print_str("BIST_ALLWRC USER_DATA_PATTERN\r\n");
		bist_init(BIST_ALLWRC, USER_DATA_PATTERN, bist_data_size, size, bist_addr);
	}
	else if(start % 6==2) {
		ax_print_str("BIST_ALLWRC SIPI_DATA_PATTERN\r\n");
		bist_init(BIST_ALLWRC, SIPI_DATA_PATTERN, bist_data_size, size, bist_addr);
	}
	else if(start % 6==3) {
		ax_print_str("BIST_ONEWRC LFSR_DATA_PATTERN\r\n");
		bist_init(BIST_ONEWRC, LFSR_DATA_PATTERN, bist_data_size, size, bist_addr);
	}
	else if(start % 6==4) {
		ax_print_str("BIST_ONEWRC USER_DATA_PATTERN\r\n");
		bist_init(BIST_ONEWRC, USER_DATA_PATTERN, bist_data_size, size, bist_addr);
	}
	else if(start % 6==5) {
		ax_print_str("BIST_ONEWRC SIPI_DATA_PATTERN\r\n");
		bist_init(BIST_ONEWRC, SIPI_DATA_PATTERN, bist_data_size, size, bist_addr);
	}
	ret = bist_test();
	reg_bit_set(D_DDRMC_TRAIN_CTRL0_ADDR, 2, 1, 0x0);//rf_train_enable
	return ret;
}

static void ddr_bist_proc(void)
{
	u32 i, j;

	ax_print_str("\r\ndram_bist_test start\r\n");
	bist_burst_length = 0x0;
	do {
		bist_data_size = 0x5;
		for (j = 0; j < 6; j++) {
			for (i = 0; i < 6; i++) {
#ifdef AX_DDR_DEBUG
				ax_print_str("\r\nloop:");
				ax_print_num(i, 10);
				ax_print_str(", burstlen ");
				ax_print_num(bist_burst_length, 16);
				ax_print_str(", datasize ");
				ax_print_num(bist_data_size, 16);
#endif
				dram_bist_test(i);
			}
			bist_data_size--;
		}
		bist_burst_length = (bist_burst_length + 1) * 2 - 1;
	} while(bist_burst_length <= 0x7);
	ax_print_str("\r\ndram_bist_test over\r\n");
}

#ifdef DDRPHY_RD_FIFO_EN_TUNING
static int ddrphy_train_rd_en_tuning(void)
{
	u32 i = 0;
	u32 j = 0;
	u32 rd_en = 0x1;
	u32 rd_en_value = 0;
	u32 result = 0;
	//u64 addr = dram_info.cs0_size + dram_info.cs1_size - 0x1000;
//	if (!ddrphy_train_sel(TRAIN_RD_EN_INDEX))
//		return 0;
	ax_print_str("\r\n-------------start rd_en training----------------\r\n");
	//train start
	//enable ctrl update
	reg_bit_set(D_DDRMC_TMG16_F0_ADDR, 0, 1, 0x1);
	for(i = 0; i < 15; i++) {
		reg_bit_set(D_DDRPHY_GEN_TMG7_F0_ADDR, 0, 16, rd_en);
		reg_bit_set((D_DDRPHY_GEN_TMG7_F0_ADDR + DDR_PHY_OFFSET), 0, 16, rd_en);
		ax_print_str("\r\n");
		ax_print_num(readl(D_DDRPHY_GEN_TMG7_F0_ADDR), 16);
		ax_print_num(readl(D_DDRPHY_GEN_TMG7_F0_ADDR + DDR_PHY_OFFSET), 16);
		ax_print_str("\r\n");
		result = dram_bist_test(0);
		ax_print_str("\r\n");
		rd_en = rd_en << 1;
//		if (dram_info.cs1_size)
			//result += dram_bist_test(addr, (addr + 0xff0));
		if (result == 0x0) {
			rd_en_value = rd_en;
			for (j = i; j < 15; j++) {
				reg_bit_set(D_DDRPHY_GEN_TMG7_F0_ADDR, 0, 16, rd_en);
				reg_bit_set((D_DDRPHY_GEN_TMG7_F0_ADDR + DDR_PHY_OFFSET), 0, 16, rd_en);
				ax_print_str("\r\n");
				ax_print_num(readl(D_DDRPHY_GEN_TMG7_F0_ADDR), 16);
				ax_print_num(readl(D_DDRPHY_GEN_TMG7_F0_ADDR + DDR_PHY_OFFSET), 16);
				ax_print_str("\r\n");
				result = dram_bist_test(0);
				ax_print_str("\r\n");
				rd_en = rd_en << 1;
			}
			break;
		}
	}
	ax_print_str("\r\nrd_en_training_result:");
	ax_print_num(rd_en_value, 16);
	if (rd_en > 0x800) {
		rd_en = 0x800;
	}

	reg_bit_set(D_DDRPHY_GEN_TMG7_F0_ADDR, 0, 16, rd_en_value);
	reg_bit_set((D_DDRPHY_GEN_TMG7_F0_ADDR + DDR_PHY_OFFSET), 0, 16, rd_en_value);
	ax_print_str("\r\nrd_en training result:");
	ax_print_num(readl(D_DDRPHY_GEN_TMG7_F0_ADDR), 16);
	ax_print_num(readl(D_DDRPHY_GEN_TMG7_F0_ADDR + DDR_PHY_OFFSET), 16);
	reg_bit_set(D_DDRMC_TMG16_F0_ADDR, 0, 1, 0x0);

	ax_print_str("\r\n-------------rd_en training over----------------\r\n");
	return 0;
}
#endif

#ifdef DDRPHY_DQS_GATE_TRAINING
static void ddrphy_train_en(u32 train_en)
{
	u32 tr_val;
	/*
	*rf_train_cadsk_en bit[4]
	*rf_train_caeye_en bit[5]
	*rf_train_gate_en  bit[6]
	*rf_train_rddsk_en bit[7]
	*rf_train_rdeye_en bit[8]
	*rf_train_wrlvl_en bit[9]
	*rf_train_wrdsk_en bit[10]
	*rf_train_wreye_en bit[11]
	*/
	tr_val = readl(D_DDRMC_TRAIN_CTRL0_ADDR);
	tr_val &= ~(0xff << 4);
	tr_val |= (0x1  << (train_en));
	ddr_writel(D_DDRMC_TRAIN_CTRL0_ADDR, tr_val);
	reg_bit_set(D_DDRMC_TRAIN_CTRL0_ADDR, 2, 1, 0x1);
	//reg_bit_set(DDRMC_TRAIN_(PARM_DDRMC_TRAIN_CTRL0_ADDR), 0, 1, 0x1);

}

static void ddrphy_fifo_reset(void)
{
	udelay(1);

	//reset fifo
	//reg_bit_set(DDRPHY_AC0_(PARM_DDRPHY_GEN_CFG0_ADDR), 17, 1, 0x1);//rf_phy_sample_rst
	//reg_bit_set(DDRPHY_AC0_(PARM_DDRPHY_GEN_CFG0_ADDR), 17, 1, 0x0);
	reg_bit_set(D_DDRPHY_GEN_CFG0_ADDR, 17, 1, 0x1);//rf_phy_sample_rst
	reg_bit_set(D_DDRPHY_GEN_CFG0_ADDR, 17, 1, 0x0);
	reg_bit_set((D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET), 17, 1, 0x1);//rf_phy_sample_rst
	reg_bit_set((D_DDRPHY_GEN_CFG0_ADDR + DDR_PHY_OFFSET), 17, 1, 0x0);
	udelay(1);
}

static int ddrphy_train_gate_training(u32 train_phy)
{
	u32 i = 0;
	u32 j = 0;
	u32 k = 0;
	u32 fail = 0;
	u32 gate_timing = 1;
	u32 GATE_BIST_CNT = 1;
	u32 left_pass = 0;
	u32 right_pass = 0;
	u32 gate_value = 0;

	ax_print_str("\r\n-------------start gate training----------------\r\n");
	//train enable
	ddrphy_train_en(TRAIN_GATE_INDEX);
	//train start

	ax_print_str("gate training phy: ");
	ax_print_num(train_phy, 10);
	ax_print_str("\r\n");

	for (i = 0; i < 15; i++) {
		if (train_phy == DDRPHY_0) {
			//		reg_bit_set(DDRPHY_DQ0_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
			reg_bit_set(D_DDRPHY_GEN_TMG6_F0_ADDR, 0, 16, gate_timing);
		}
		else if (train_phy == DDRPHY_1) {
			//		reg_bit_set(DDRPHY_DQ1_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
			reg_bit_set((D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET), 0, 16, gate_timing);
		}
		else if (train_phy == DDRPHY_ALL) {
			//		reg_bit_set(DDRPHY_DQ0_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
			reg_bit_set(D_DDRPHY_GEN_TMG6_F0_ADDR, 0, 16, gate_timing);
			//		reg_bit_set(DDRPHY_DQ1_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
			reg_bit_set((D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET), 0, 16, gate_timing);
		}
		ddrphy_fifo_reset();
		for (k = 0; k < GATE_BIST_CNT; k++) {
			ax_print_str("\r\n");
			ax_print_num(readl(D_DDRPHY_GEN_TMG6_F0_ADDR), 16);
			ax_print_num(readl(D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET), 16);
			fail = dram_bist_test(0);
			//			fail += dram_bist_test(2);
		}
		gate_timing = gate_timing << 1;
		if (fail == 0x0) {
			right_pass = i;
			left_pass = i;
			for (j = i; j < 15; j++) {
				if (train_phy == DDRPHY_0) {
					//		reg_bit_set(DDRPHY_DQ0_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
					reg_bit_set(D_DDRPHY_GEN_TMG6_F0_ADDR, 0, 16, gate_timing);
				}
				else if (train_phy == DDRPHY_1) {
					//		reg_bit_set(DDRPHY_DQ1_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
					reg_bit_set(D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET, 0, 16, gate_timing);
				}
				else if (train_phy == DDRPHY_ALL) {
					//		reg_bit_set(DDRPHY_DQ0_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
					reg_bit_set(D_DDRPHY_GEN_TMG6_F0_ADDR, 0, 16, gate_timing);
					//		reg_bit_set(DDRPHY_DQ1_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
					reg_bit_set((D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET), 0, 16, gate_timing);
				}
				ddrphy_fifo_reset();
				for (k = 0; k < GATE_BIST_CNT; k++) {
					ax_print_str("\r\n");
					ax_print_num(readl(D_DDRPHY_GEN_TMG6_F0_ADDR), 16);
					ax_print_num(readl(D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET), 16);
					fail = dram_bist_test(0);
					//			fail += dram_bist_test(2);
				}
				gate_timing = gate_timing << 1;
				if (fail == 0x0) {
					left_pass = j+1;
				}
			}
			break;
		}
	}

	gate_value = (left_pass + right_pass) / 2;
	ax_print_str("\r\ngate left pass:");
	ax_print_num(left_pass, 16);
	ax_print_str("\r\ngate right pass:");
	ax_print_num(right_pass, 16);
	ax_print_str("\r\ngate value:");
	ax_print_num(gate_value, 16);

	if (train_phy == DDRPHY_0) {
		//		reg_bit_set(DDRPHY_DQ0_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
		reg_bit_set(D_DDRPHY_GEN_TMG6_F0_ADDR, 0, 16, 1 << gate_value);
	}
	else if (train_phy == DDRPHY_1) {
		//		reg_bit_set(DDRPHY_DQ1_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
		reg_bit_set((D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET), 0, 16, 1 << gate_value);
	}
	else if (train_phy == DDRPHY_ALL) {
		//		reg_bit_set(DDRPHY_DQ0_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
		reg_bit_set(D_DDRPHY_GEN_TMG6_F0_ADDR, 0, 16, 1 << gate_value);
		//		reg_bit_set(DDRPHY_DQ1_(PARM_DDRPHY_GEN_TMG9_F0_ADDR), 15, 2, 0x2);//RPULL
		reg_bit_set((D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET), 0, 16, 1 << gate_value);
	}
	ddrphy_fifo_reset();

	ax_print_str("\r\ngate training result:phy0/phy1\r\n");
	ax_print_num(readl(D_DDRPHY_GEN_TMG6_F0_ADDR), 16);
	ax_print_num(readl(D_DDRPHY_GEN_TMG6_F0_ADDR + DDR_PHY_OFFSET), 16);

	return 0;
}
#endif
#endif
void ddr_types_get(void)
{
#if defined (DDR_ENV_EDA) || defined (DDR_ENV_ATE)
	ddr_writel(DDR_DUMMY_REG_SW0, 0);
#else
	misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;
	chip_id = misc_info->chip_type;
	switch (misc_info->chip_type) {
	case AX620QX_CHIP_E:
		ddr_types = TYPE_LPDDR4_1CS_1CH_X16_2GBIT;
		lpddr4_ca_odt_en = 0;
		lpddr4_ca_train_en = 1;
		ddr_dfs_sel = 0;
		//ax_log_threhold = AX_LOG_LEVEL_DEBUG;
		break;
	case AX620QZ_CHIP_E:
	case AX620Q_CHIP_E:
		ddr_types = TYPE_LPDDR4_1CS_1CH_X16_2GBIT;
		lpddr4_ca_odt_en = 0;
		lpddr4_ca_train_en = 1;
		break;
	case AX620QP_CHIP_E:
		ddr_types = TYPE_LPDDR4_1CS_1CH_X16_4GBIT;
		lpddr4_ca_odt_en = 0;
		lpddr4_ca_train_en = 1;
		ddr_dfs_sel = 0;
		ax_log_threhold = AX_LOG_LEVEL_DEBUG;
		break;
	case AX630C_CHIP_E:
	case AX631_CHIP_E:
		board_type = ax_get_board_id();
		// ### SIPEED EDIT ###
		if (PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G == board_type
		|| PHY_AX630C_AX631_MAIXCAM2_SOM_1G == board_type
		|| PHY_AX630C_AX631_MAIXCAM2_SOM_2G == board_type) {
			ddr_types = TYPE_LPDDR4_1CS_2CH_X32_16GBIT;
			lpddr4_ca_odt_en = 1;
			lpddr4_ca_train_en = 1;
			ddr_dfs_sel = 0;
			ax_log_threhold = AX_LOG_LEVEL_DEBUG;
		} else if (PHY_AX630C_AX631_MAIXCAM2_SOM_4G == board_type) {
			ddr_types = TYPE_LPDDR4_1CS_2CH_X32_32GBIT;
			lpddr4_ca_odt_en = 1;
			lpddr4_ca_train_en = 1;
			ddr_dfs_sel = 0;
			ax_log_threhold = AX_LOG_LEVEL_DEBUG;
		} else if (PHY_AX630C_DEMO_LP4_V1_0 == board_type) {
#ifdef AX630C_LPDDR4_DUAL_RANK
			ddr_types = TYPE_LPDDR4_2CS_2CH_X32_32GBIT;
			//ddr_types = TYPE_LPDDR4_2CS_2CH_X32_8GBIT;
#else
			ddr_types = TYPE_LPDDR4_1CS_2CH_X32_16GBIT;
#endif
			lpddr4_ca_odt_en = 1;
			lpddr4_ca_train_en = 1;
			ddr_dfs_sel = 0;
			ax_log_threhold = AX_LOG_LEVEL_DEBUG;
		} else if (PHY_AX630C_DEMO_DDR3_V1_0 == board_type) {
			ddr_types = TYPE_DDR3_1CS_2CH_X32_4GBIT;
		}
		ax_print_str("\r\nboard id=0x");
		ax_print_num(board_type, 16);
		// ### SIPEED EDIT END ###
		break;
	default:
		break;
	}
	ax_print_str("\r\nddr init start, chip_type=0x");
	ax_print_num(misc_info->chip_type, 16);
#endif
}

void ddr_freqs_get(void)
{
	if(ddr_types & TYPE_LPDDR4) {
		ax_print_str(", ddr_types LPDDR4\r\n");
		ddr_freqs = ddr_freq_search(lpddr4_freqs, sizeof(lpddr4_freqs) / sizeof(lpddr4_freqs[0]));
	} else if (ddr_types & TYPE_DDR4) {
		ax_print_str(", ddr_types DDR4\r\n");
		ddr_freqs = ddr_freq_search(ddr4_freqs, sizeof(ddr4_freqs) / sizeof(ddr4_freqs[0]));
	} else if (ddr_types & TYPE_DDR3) {
		ax_print_str(", ddr_types DDR3\r\n");
		ddr_freqs = ddr_freq_search(ddr3_freqs, sizeof(ddr3_freqs) / sizeof(ddr3_freqs[0]));
	}
	ax_print_str("ddr_freqs=");
	ax_print_num(ddr_freqs, 10);
}

void lpddr4_init_flow(void * rom_param)
{
#if LPDDR4_DFS_CONFIG
	misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;
	if(ddr_dfs_sel && (ddr_freqs > DDR_CLK_1600)) {
		ddr_dfs_f1_freq = ddr_freqs;
		ddr_freqs = DDR_CLK_1600;
	} else if (!ddr_dfs_sel && (ddr_freqs > DDR_CLK_1600) && (misc_info->chip_type != AX620Q_CHIP_E) && (misc_info->chip_type != AX620QZ_CHIP_E) && (misc_info->chip_type != AX620QP_CHIP_E)) {
		ddr_timming_index = 1;
	} else {
		ddr_dfs_sel = 0;
	}
#endif

// ### SIPEED EDIT ###
	switch (ddr_freqs) {
	case 2133:	// fall through
	case 2400:	// fall through
	case 2666:	// fall through
	case 2800:	// fall through
		if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
			lpddr4mc_timing[ddr_timming_index][3] = 0x04030000;
		} else {
			lpddr4mc_timing[ddr_timming_index][3] = 0x02010000;
		}
	break;
	case 3200:
		if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
			lpddr4mc_timing[ddr_timming_index][1] = 0x030C170F;
			lpddr4mc_timing[ddr_timming_index][2] = 0x00120607;
			lpddr4mc_timing[ddr_timming_index][3] = 0x05040000;
		} else {
			lpddr4mc_timing[ddr_timming_index][1] = 0x0308170F;
			lpddr4mc_timing[ddr_timming_index][2] = 0x000E060A;
			lpddr4mc_timing[ddr_timming_index][3] = 0x02020000;
		}
	break;
	default:break;
	}
// ### SIPEED EDIT END ###

	p_iram_dfs_vref_info->freq = ddr_freqs;
	ddrmc_pre_init(lpddr4mc_timing, ddr_timming_index);//lpddr4mc_timing ddr3mc_timing ddr4mc_timing
	lpddr4phy_pre_init();
	lpddr4mc_pos_init();
	ddrmc_train_flow_lp4_sequence(rom_param);
#ifdef DDRPHY_RD_FIFO_EN_TUNING
		ddrphy_train_rd_en_tuning();
#endif
#ifdef DDRPHY_DQS_GATE_TRAINING
		ddrphy_train_gate_training(DDRPHY_0);
		ddrphy_train_gate_training(DDRPHY_1);
		//ddrphy_train_gate_training(DDRPHY_ALL);
#endif
}

void ddr4_init_flow(void * rom_param)
{
	p_iram_dfs_vref_info->freq = ddr_freqs;
	ddrmc_pre_init(ddr4mc_timing, ddr_timming_index);//lpddr4mc_timing ddr3mc_timing ddr4mc_timing
	ddrphy_pre_init();
	ddrmc_pos_init();
	ddrmc_train_flow_sequence(rom_param);
}

void ddr3_init_flow(void * rom_param)
{
	p_iram_dfs_vref_info->freq = ddr_freqs;
	ddrmc_pre_init(ddr3mc_timing, 0x0);
	ddr3_ddrphy_pre_init();
	ddr3_ddrmc_pos_init();
	ddr3_train_flow_sequence(rom_param);
}

void ddr_env_bist_scan(void)
{
#ifdef DDR_BIST_EN
	ddr_bist_proc();
#endif
#ifdef DDR_OFFLINE_SCAN
	ddr_scan_result_proc(ddr_types, 0);
	//ddr_scan_result_proc(ddr_types, 1);
#endif
#if defined (DDR_ENV_EDA) || defined (DDR_ENV_ATE)
	u32 test_val = 0x5a5a5a5a;
	void *p1 = (void *)0x40000000;
	*(u32 *) p1 = (u32)test_val;
	if(0x5a5a5a5a == *(const volatile u32 *) p1) {
		reg_bit_set(DDR_DUMMY_REG_SW1, 6, 1, 0x1);
	} else {
		reg_bit_set(DDR_DUMMY_REG_SW0, 6, 1, 0x1);
	}
	reg_bit_set(DDR_DUMMY_REG_SW1, 7, 1, 0x1);
	while(1);
#endif
}

void mc20e_ddr_init(void * rom_param)
{
#if (LPDDR4_CONFIG_3733) || (LPDDR4_CONFIG_3200)
	DDR_FREQ def_clk = DDR_CLK_3200;
#endif

	/*User can remove ddr_types_get and then determine ddr type through adc*/
	ddr_types_get();
	ddr_freqs_get();
#if (LPDDR4_CONFIG_3733) || (LPDDR4_CONFIG_3200)
	if ((AX620Q_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id) && (ddr_freqs >= DDR_CLK_3200)) {
		def_clk = ddr_freqs;
		ddr_freqs = DDR_CLK_2800;
	}
#endif
	ddrmc_sys_glb_init(ddr_freqs);
	if (ddr_types & TYPE_LPDDR4) {
#if (LPDDR4_CONFIG_3733) || (LPDDR4_CONFIG_3200)
		if ((AX620Q_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id))
			ddr_freqs = def_clk;
#endif
		lpddr4_init_flow(rom_param);
	} else if (ddr_types & TYPE_DDR4) {
		ddr4_init_flow(rom_param);
	} else if (ddr_types & TYPE_DDR3) {
		ddr3_init_flow(rom_param);
	}
	ddr_env_bist_scan();
	#if (0 == LPDDR4_DFS_CONFIG)
	if ((!lpddr4_ca_train_en || caeye_pass) && rdeye_pass && wreye_pass && !cadsk_fail  && !rddsk_fail && !wrdsk_fail)
		training_pass = 1;
	else {
		ax_print_str("\r\nlpddr4_ca_train_en=");
		ax_print_num(lpddr4_ca_train_en, 10);
		ax_print_str("\r\ncaeye_pass=");
		ax_print_num(caeye_pass, 10);
		ax_print_str("\r\nrdeye_pass=");
		ax_print_num(rdeye_pass, 10);
		ax_print_str("\r\nwreye_pass=");
		ax_print_num(wreye_pass, 10);
		ax_print_str("\r\ncadsk_fail=");
		ax_print_num(cadsk_fail, 10);
		ax_print_str("\r\nrddsk_fail=");
		ax_print_num(rddsk_fail, 10);
		ax_print_str("\r\nwrdsk_fail=");
		ax_print_num(wrdsk_fail, 10);
	}
	if (training_pass)
	#endif
		ax_print_str("\r\nddr init done...\r\n");
}
