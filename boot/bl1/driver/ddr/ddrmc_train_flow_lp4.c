// +FHDR------------------------------------------------------------
//				 Copyright (c) 2020 COMPANY: Undefined variable..
//					   ALL RIGHTS RESERVED
// -----------------------------------------------------------------
// Filename	  : ts.ddrmc_train_flow_lp4_test.sv
// Author		: weijingwei
// Created On	: 2022-11-2 14:20
// Last Modified :
// -----------------------------------------------------------------
// Description:
//
//
// -FHDR------------------------------------------------------------

#ifndef __TS_DDRMC_TRAIN_FLOW_LP4_TEST_SV__
#define __TS_DDRMC_TRAIN_FLOW_LP4_TEST_SV__
#include "cmn.h"
#include "ddr_common_config.h"
#include "timer.h"
#include "trace.h"
#include "fdl_channel.h"
#include "fdl_frame.h"
#include "ddrmc_reg_addr_def.h"
#include "ddrphy_reg_addr_def.h"
#include "ddr_init_config.h"
#include "ddrmc_train_reg_addr_def.h"
#include "ddrmc_bist_reg_addr_def.h"
#include "boot_mode.h"
#include "boot.h"
#include "dpll_ctrl_glb.h"

extern void ddr_writel(u32 addr, u32 value);
extern void reg_bit_set(u32 addr, u32 start_bit, u32 bits_num, u32 val);
extern void ddrmc_mrw(u8 mr_cs, u8 mr_ad, u16 mr_op);
extern void ddrmc_auto_ref(u8 mr_cs);
extern void reg_mask_set(u32 addr, u32 mask_data, u32 bits_set);
extern void retrain_base_value_f0(void);
extern void retrain_general_config(void);
extern void lpddr4_mrs_config(u8 mr_cs);
extern void ca_vref_scan(struct ddr_dfs_vref_t * param, DDR_TYPE ddr_type, u8 cs, u8 vref_start, u8 vref_end, u8 interval);
extern void rd_vref_scan(struct ddr_dfs_vref_t * param, DDR_TYPE ddr_type, u8 cs, u8 vref_start, u8 vref_end, u8 interval);
extern void wr_vref_scan(struct ddr_dfs_vref_t * param, DDR_TYPE ddr_type, u8 cs, u8 vref_start, u8 vref_end, u8 interval);
extern u32 ddr_types;
extern void ddrphy_timing_init(u32 offset_addr, const u32 ddrphy_timing[][31], u8 ddrphy_timming_index);
extern void ddr_dfs_reg_mask_set(u32 addr, u32 mask_data, u32 bits_set, u32 dfs_index);
extern void ddrphy_dfs_writel(u32 addr, u32 value, u32 dfs_index);
extern u32 ddrphy_dfs_readl(unsigned long addr, u32 dfs_index);
extern u32 ddr_freqs;
extern u32 chip_id;
extern u8 lpddr4_ca_odt_en;
extern u8 lpddr4_ca_train_en;
extern u32 ddr_dfs_f1_freq;
typedef int byte_array_t[8];

u16 cur_pass_window_caeye __attribute__((section(".data"))) = 0;
#if (0 == LPDDR4_DFS_CONFIG)
extern u8 caeye_pass;
extern u8 rdeye_pass;
extern u8 wreye_pass;
extern u8 cadsk_fail;
extern u8 rddsk_fail;
extern u8 wrdsk_fail;
#endif
u8 dfs_freq = 0;
static u8 wrlvl_back_ind __attribute__((section(".data"))) = 0;
static u8 lpddr4_rank_num __attribute__((section(".data"))) = 1;
// ### SIPEED EDIT ###
// #ifdef AX630C_LPDDR4_DUAL_RANK
static u32 dq_dl_value[8] __attribute__((section(".data"))) = {0};
// #endif
// ### SIPEED EDIT END ###
extern u16 mr2_status_op;
extern u16 mr3_status_op;
extern u32 r_pass_window_dqeye;
extern u32 w_pass_window_dqeye;
extern u32 ddr_dfs_sel;
extern struct ddr_dfs_vref_t * p_iram_dfs_vref_info;

extern const unsigned int lpddr4mc_timing[][29];
extern const unsigned int lpddr4phy_timing[][31];
struct ddr_dfs_reg {
	volatile u32 dfs_ctrl;          // DDR_GLB_DFS_CTRL_REG_OFFSET + 0x00
	volatile u32 dfs_clksrc;        // 0x04
	volatile u32 dfs_clk_switch;    // 0x08
	volatile u32 freq_change;       // 0x0c
	volatile u32 abort_rst;         // 0x10
	volatile u32 dfs_int;           // 0x14
};

static const u32 ax630c_lp4_write_pat_config[32] = {
	0xffffffff,
	0x0f0fffff,
	0xffffffff,
	0x0f0fffff,
	0xffffffff,
	0xffffffff,
	0x0f0fffff,
	0xffff0f0f,
	0x00000000,
	0xf0f00000,
	0x00000000,
	0xf0f00000,
	0x00000000,
	0x00000000,
	0xf0f0000f,
	0x0000f0f0,
	0x5aa55aa5,
	0x5a5a5a5a,
	0xa5a5a5a5,
	0xa55aa55a,
	0x5aa55aa5,
	0x5a5a5a5a,
	0xa5a5a5a5,
	0xa55aa55a,
	0x5aa55aa5,
	0x5a5a5a5a,
	0xa5a5a5a5,
	0xa55aa55a,
	0x3cc33cc3,
	0x3c3c3c3c,
	0x5aa55aa5,
	0x5a5a5a5a,
};

static const u32 ax630c_lp4_read_pat_config[16] = {
	0x55005500,
	0xaaffaaff,
	0x55005500,
	0xaaffaaff,
	0xaaffaaff,
	0x55005500,
	0xaaffaaff,
	0x55005500,
	0xaaffaaff,
	0x55005500,
	0xaaffaaff,
	0x55005500,
	0x55005500,
	0xaaffaaff,
	0x55005500,
	0xaaffaaff,
};

static const u32 ax620q_lp4_write_pat_config[24] = {
	0x33ffffff,
	0x33ffffff,
	0xffffffff,
	0xff3333ff,
	0xcc000000,
	0xcc000000,
	0x00000000,
	0x00cccc03,
	0x5a5a5aa5,
	0xa55aa5a5,
	0x5a5a5aa5,
	0xa55aa5a5,
	0x5a5a5aa5,
	0xa55aa5a5,
	0x3c3c3cc3,
	0x5a5a5aa5,
	0x5a5a5aa5,
	0xa55aa5a5,
	0x5a5a5aa5,
	0xa55aa5a5,
	0x5a5a5aa5,
	0xa55aa5a5,
	0x3c3c3cc3,
	0x5a5a5aa5,
};

static const u32 ax620q_lp4_read_pat_config[16] = {
	0xaaaa5555,
	0xaaaa5555,
	0x5555aaaa,
	0x5555aaaa,
	0x55555555,
	0xaaaaaaaa,
	0xaaaaaaaa,
	0x55555555,
	0xaaaa5555,
	0xaaaa5555,
	0x5555aaaa,
	0x5555aaaa,
	0x55555555,
	0xaaaaaaaa,
	0xaaaaaaaa,
	0x55555555,
};

void ca_train_cfg(void)
{
	u32 bit_shift = 0;
	if (ddr_freqs == DDR_CLK_3733 || ddr_freqs == DDR_CLK_3400 || ddr_freqs == DDR_CLK_3200 || ddr_freqs == DDR_CLK_2800) {
		bit_shift |= (0x19 + 0x2) << 16; // drf_t_vref lp4 = 250ns
		bit_shift |= 0x19 << 8; // drf_t_caent lp4 = 250ns
		bit_shift |= 0x3; // drf_t_cacd lp4 = 20ns
	} else if (ddr_freqs == DDR_CLK_2666 || ddr_freqs == DDR_CLK_2400) {
		bit_shift |= (0x15 + 0x2) << 16; // drf_t_vref lp4 = 250ns
		bit_shift |= 0x15 << 8; // drf_t_caent lp4 = 250ns
		bit_shift |= 0x3; // drf_t_cacd lp4 = 20ns
	} else if (ddr_freqs == DDR_CLK_2133) {
		bit_shift |= 0x11 << 16; // drf_t_vref lp4 = 250ns
		bit_shift |= 0x11 << 8; // drf_t_caent lp4 = 250ns
		bit_shift |= 0x3; // drf_t_cacd lp4 = 20ns
	} else if (ddr_freqs == DDR_CLK_1600) {
		bit_shift |= 0xd << 16; // drf_t_vref lp4 = 250ns
		bit_shift |= 0xd << 8; // drf_t_caent lp4 = 250ns
		bit_shift |= 0x2; // drf_t_cacd lp4 = 20ns
	}
	reg_mask_set(D_DDRMC_TRAIN_TMG0_ADDR, 0xFFFFFF, bit_shift);
	ddr_writel(D_DDRMC_BIST_DATA0_ADDR, 0x00000000);//used in cadsk
}

void set_ddrmc_train_control(u32 bit_mask, u32 train_ctrl_bits, u32 flag)
{
	u32 bit_shift = 0;
	if (flag == 1) {
		if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
			bit_shift = 0xf << 24;
		} else {
			bit_shift = 0x9 << 24;
		}
	} else {
		bit_shift = 0xf << 24;
	}
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, bit_mask, bit_shift | train_ctrl_bits);
}

void single_rank_wr_rd_dsk_flow(int type)	// 0: write, 1: read
{
	int i;
	u32 reg_data;
	u32 reg_addr = D_DDRPHY_DS0_CFG6_ADDR + type * 0x18;
// ### SIPEED EDIT ###
if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
	if (0 == (readl(D_DDRMC_TRAIN_CTRL0_ADDR) & BIT(20))) {
		for (i = 0; i < 12; i++) {
			if (3 == i) {
				reg_addr = D_DDRPHY_DS1_CFG6_ADDR + type * 0x18;
			}
			else if (6 == i) {
				reg_addr = D_DDRPHY_DS0_CFG6_ADDR + DDR_PHY_OFFSET + type * 0x18;
			}
			else if (9 == i) {
				reg_addr = D_DDRPHY_DS1_CFG6_ADDR + DDR_PHY_OFFSET + type * 0x18;
			}
			reg_data = readl(reg_addr);
#ifdef AX_DDR_DEBUG
			ax_print_str("\r\nread reg 0x");
			ax_print_num(reg_addr, 16);
			ax_print_str(", val 0x");
			ax_print_num(reg_data, 16);
#endif
			ddr_writel(reg_addr + 0xC, reg_data);
#ifdef AX_DDR_DEBUG
			ax_print_str("\r\nwrite reg 0x");
			ax_print_num(reg_addr + 0xC, 16);
			ax_print_str(", val 0x");
			ax_print_num(reg_data, 16);
#endif
			reg_addr += 0x4;
		}
	}
} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if (0 == (readl(D_DDRMC_TRAIN_CTRL0_ADDR) & BIT(20))) {
#else
	if (1 == lpddr4_rank_num) {
#endif
		for (i = 0; i < 12; i++) {
			if (3 == i) {
				reg_addr = D_DDRPHY_DS1_CFG6_ADDR + type * 0x18;
			}
			else if (6 == i) {
				reg_addr = D_DDRPHY_DS0_CFG6_ADDR + DDR_PHY_OFFSET + type * 0x18;
			}
			else if (9 == i) {
				reg_addr = D_DDRPHY_DS1_CFG6_ADDR + DDR_PHY_OFFSET + type * 0x18;
			}
			reg_data = readl(reg_addr);
#ifdef AX_DDR_DEBUG
			ax_print_str("\r\nread reg 0x");
			ax_print_num(reg_addr, 16);
			ax_print_str(", val 0x");
			ax_print_num(reg_data, 16);
#endif
			ddr_writel(reg_addr + 0xC, reg_data);
#ifdef AX_DDR_DEBUG
			ax_print_str("\r\nwrite reg 0x");
			ax_print_num(reg_addr + 0xC, 16);
			ax_print_str(", val 0x");
			ax_print_num(reg_data, 16);
#endif
			reg_addr += 0x4;
		}
	}
}
// ### SIPEED EDIT END ###
}

void cadsk_train_flow(void)
{
	u32 reg_data;

	set_ddrmc_train_control(0xF000017, (1 << 4) | (1 << 2) | 0x1, 0);
	//polling train done
	for (int i=0; i < 1000; i++)  {
		//#1us;
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & 0x1)  {//rf_train_cadsk_done
			if(reg_data & (0x1 << 1))  {//rf_train_cadsk_fail
				#if (0 == LPDDR4_DFS_CONFIG)
				cadsk_fail = 1;
				#endif
				ax_print_str("\r\ncadsk train fail !\n");
			}
			break;
		}
		if(i==999) {
			#if (0 == LPDDR4_DFS_CONFIG)
			cadsk_fail = 1;
			#endif
			ax_print_str("\r\npolling cadsk_training done timeout !\n");
		}
	}
	set_ddrmc_train_control(0xF000017, (1 << 4) | (1 << 2) | (1 << 1), 0);
	set_ddrmc_train_control(0xF000017, 0, 0);
}

u16 caeye_train_flow(void)
{
	u32 reg_data;
	u32 i;
	u16 caeye_train_temp = 0;

	cur_pass_window_caeye = 0;

	set_ddrmc_train_control(0xF000027, (1 << 5) | (1 << 2) | 0x1, 0);

	for (i = 0; i < 1000; i++) {
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (1 << 4)) {//rf_train_caeye_done
			if(reg_data & (1 << 5)) {//rf_train_caeye_fail
				ax_print_str("\r\ncaeye train fail !\n");
			} else {
				reg_data = readl(D_DDRMC_TRAIN_RES1_ADDR);
#if defined (AX_DDR_DEBUG) || defined (DDR_OFFLINE_SCAN)
				ax_print_str(", D_DDRMC_TRAIN_RES1_ADDR=0x");
				ax_print_num(reg_data, 16);
#endif
				caeye_train_temp = reg_data & 0x7ff;
				cur_pass_window_caeye = (reg_data >> 16) & 0x7ff;
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
				reg_bit_set(DDR_DUMMY_REG_SW1, 2, 1, 0x1);
#endif
				#if (0 == LPDDR4_DFS_CONFIG)
				caeye_pass = 1;
				#endif
			}
			break;
		}
		if(i==999) {
			ax_print_str("\r\npolling caeye_training done timeout !\n");
		}
	}

	set_ddrmc_train_control(0xF000027, (1 << 5) | (1 << 2) | (1 << 1), 0);
	set_ddrmc_train_control(0xF000027, 0, 0);

	return caeye_train_temp;
}

void read_pat_cfg(u8 cs)
{
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		for(int i = 0; i < 16; i++) {
			ddr_writel(D_DDRMC_BIST_DATA0_ADDR + 4*i,ax630c_lp4_read_pat_config[i]);
		}
		ddrmc_mrw(0x2 | cs, 0x20, 0x5a);
		ddrmc_mrw(0x2 | cs, 0x28, 0xa5);
		ddrmc_mrw(0x2 | cs, 0x0f, 0x00);
		ddrmc_mrw(0x2 | cs, 0x14, 0x55);
	} else {
		for(int i = 0; i < 16; i++) {
			ddr_writel(D_DDRMC_BIST_DATA0_ADDR + 4*i,ax620q_lp4_read_pat_config[i]);
		}
	}
	ddr_writel(D_DDRMC_TRAIN_MASK_0_ADDR, 0x00000000);
}

u32 rdeye_train_flow(void)
{
	u32 reg_data;
	u32 rdeye_train_temp = 0;
	r_pass_window_dqeye = 0;

	set_ddrmc_train_control(0xFF000107, (1 << 8) | (1 << 2) | 0x1, 1);

	//polling train done for data byte
	for (int i=0; i<1000; i++)  {
		//#1us;
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (0x1 << 16)) {//rf_train_rdeye_done
			if(reg_data & (0x1 << 17)) {
				ax_print_str("\r\nrdeye train fail !\n");
			} else {
				rdeye_train_temp = readl(D_DDRMC_TRAIN_RES2_ADDR);
				r_pass_window_dqeye = readl(D_DDRMC_TRAIN_RES3_ADDR); //pass_window
#if defined (AX_DDR_DEBUG) || defined (DDR_OFFLINE_SCAN)
				ax_print_str(", D_DDRMC_TRAIN_RES2_ADDR=0x");
				ax_print_num(rdeye_train_temp, 16);
				ax_print_str(", D_DDRMC_TRAIN_RES3_ADDR=0x");
				ax_print_num(r_pass_window_dqeye, 16);
#endif
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
				reg_bit_set(DDR_DUMMY_REG_SW1, 3, 1, 0x1);
#endif
				#if (0 == LPDDR4_DFS_CONFIG)
				rdeye_pass = 1;
				#endif
			}
			break;
		}
		if(i==999)  {
			ax_print_str("\r\npolling rdeye_training done timeout !\n");
		}
	}

	set_ddrmc_train_control(0xFF000107, (1 << 8) | (1 << 2) | (1 << 1), 1);
	set_ddrmc_train_control(0xFF000107, 0x0, 1);
	return rdeye_train_temp;
}

void rddsk_train_flow(void)
{
	u32 reg_data;
	reg_data = readl(D_DDRMC_TRAIN_CTRL0_ADDR);
	reg_data &= ~0xFF000087;
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		reg_data |= 0xf << 24; //rf_train_byte_en
	} else {
		reg_data |= 0x9 << 24; //rf_train_byte_en
	}
	reg_data |= 0x1 << 7; //rf_train_rddsk_en
	reg_data |= 0x1 << 2; //rf_train_enable
	reg_data |= 0x0 << 1; //rf_train_clear
	reg_data |= 0x1 << 0; //rf_train_start
	ddr_writel(D_DDRMC_TRAIN_CTRL0_ADDR, reg_data);
	//polling train done for data byte
	for (int i=0; i < 1000 ; i++)  {
		//#1us;
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (0x1 << 12))  {//rf_train_rddsk_done
			if(reg_data & 0x1 << 13)  {//rf_train_rddsk_fail
				#if (0 == LPDDR4_DFS_CONFIG)
				rddsk_fail = 1;
				#endif
				ax_print_str("\r\nrddsk train fail !\n");
			} else  {
				ax_print_str("\r\nrddsk train done !\n");
			}
			break;
		}
		if(i==999)  {
			#if (0 == LPDDR4_DFS_CONFIG)
			rddsk_fail = 1;
			#endif
			ax_print_str("\r\npolling rddsk_training done timeout !\n");
		}
	}
	reg_data = readl(D_DDRMC_TRAIN_CTRL0_ADDR);
	reg_data &= ~0xFF000087;
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		reg_data |= 0xf << 24; //rf_train_rddsk_en
	} else {
		reg_data |= 0x9 << 24; //rf_train_byte_en
	}
	reg_data |= 0x1 << 7; //rf_train_rddsk_en
	reg_data |= 0x1 << 2; //rf_train_enable
	reg_data |= 0x1 << 1; //rf_train_clear
	reg_data |= 0x0 << 0; //rf_train_start
	ddr_writel(D_DDRMC_TRAIN_CTRL0_ADDR, reg_data);

	reg_data = readl(D_DDRMC_TRAIN_CTRL0_ADDR);
	reg_data &= ~0xFF000087;
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		reg_data |= 0xf << 24; //rf_train_rddsk_en
	} else {
		reg_data |= 0x9 << 24; //rf_train_byte_en
	}
	ddr_writel(D_DDRMC_TRAIN_CTRL0_ADDR, reg_data);
	single_rank_wr_rd_dsk_flow(1);
}

void write_pat_cfg(void)
{
	u32 reg_base = D_DDRMC_TRAIN_STRB_0_ADDR;
	int i = 0, j = 0;
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		for (i = 0; i < 32; i++) {
			if (i == 16) reg_base = D_DDRMC_BIST_DATA0_ADDR;
			ddr_writel(reg_base + (i % 16) * 4, ax630c_lp4_write_pat_config[i]);
		}
	} else {
		for (i = 0; i < 24; i++) {
			if (i <= 7) {
				reg_base = D_DDRMC_TRAIN_STRB_0_ADDR + (i * 2) * 4;
				ddr_writel(reg_base, ax620q_lp4_write_pat_config[i]);
			}
			if ((i >= 8) && (i <= 23)) {
				reg_base = D_DDRMC_BIST_DATA0_ADDR + j * 4;
				ddr_writel(reg_base, ax620q_lp4_write_pat_config[i]);
				j++;
			}
		}
	}
}

void wrdsk_train_flow(void)
{
	u32 reg_data;

	reg_mask_set(D_DDRMC_TRAIN_CTRL3_ADDR, 0xFF0FF7FF, (0x08 << 24) | 0x200);
	set_ddrmc_train_control(0xFF000407, (1 << 10) | (1 << 2) | 0x1, 1);
	for (int i=0; i<1000 ; i++)  {
		//#1us;
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (0x1 << 24))  {//rf_train_wrdsk_done
			if(reg_data& (0x1 << 25))  {//rf_train_wrdsk_fail
				#if (0 == LPDDR4_DFS_CONFIG)
				wrdsk_fail = 1;
				#endif
				ax_print_str("\r\nwrdsk train fail !\n");
			} else  {
				ax_print_str("\r\nwrdsk train done !\n");
			}
			break;
		}
		if(i==999) {
			#if (0 == LPDDR4_DFS_CONFIG)
			wrdsk_fail = 1;
			#endif
			ax_print_str("\r\npolling wrdsk_training done timeout !\n");
		}
	}
	set_ddrmc_train_control(0xFF000407, (1 << 10) | (1 << 2) | (1 << 1), 1);
	set_ddrmc_train_control(0xFF000407, 0x0, 1);
	reg_mask_set(D_DDRMC_TRAIN_CTRL3_ADDR, 0xFF0FF7FF, (0x10 << 24) | 0x400);
	single_rank_wr_rd_dsk_flow(0);
}

void Adjust_Wreye_Value(u32 regAddr, u32 wreyeVal, u32 shiftNum)
{
	if (wreyeVal > 0x300) {
		wreyeVal = (wreyeVal > 0x500) ? (wreyeVal - 0x400) : (wreyeVal - 0x200);
		reg_bit_set(regAddr, 0, 11, wreyeVal);
		reg_bit_set(regAddr, 24, 3, (wreyeVal > 0x500) ? (shiftNum + 0x2) : (shiftNum + 0x1));
	}
}

static void DQ_Write_delay_proc(u16 wreye_val0, u16 wreye_val1, u16 wreye_val2, u16 wreye_val3)
{
	u32 shift_num = (ddrphy_dfs_readl(D_DDRPHY_DS0_TMG1_F0_ADDR, dfs_freq) >> 24) & 0x7;
	static int is_check_done = 0;


	if (shift_num && (AX620Q_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id))
		return;

	if (((dfs_freq == 0) && (is_check_done == 1)) || ((dfs_freq == 1) && (is_check_done == 2)))
		return;

	ax_print_str("\r\nDQ_Write_delay_proc, default shift_num = ");
	ax_print_num(shift_num, 10);
	// modify wreye value
	if(dfs_freq == 0) {
		Adjust_Wreye_Value(D_DDRPHY_DS0_TMG1_F0_ADDR, wreye_val0, shift_num);
		Adjust_Wreye_Value(D_DDRPHY_DS1_TMG1_F0_ADDR, wreye_val1, shift_num);
		Adjust_Wreye_Value(D_DDRPHY_DS0_TMG1_F0_ADDR + DDR_PHY_OFFSET, wreye_val2, shift_num);
		Adjust_Wreye_Value(D_DDRPHY_DS1_TMG1_F0_ADDR + DDR_PHY_OFFSET, wreye_val3, shift_num);
		is_check_done = 1;
	} else if (dfs_freq == 1) {
		Adjust_Wreye_Value(D_DDRPHY_DS0_TMG1_F1_ADDR, wreye_val0, shift_num);
		Adjust_Wreye_Value(D_DDRPHY_DS1_TMG1_F1_ADDR, wreye_val1, shift_num);
		Adjust_Wreye_Value(D_DDRPHY_DS0_TMG1_F1_ADDR + DDR_PHY_OFFSET, wreye_val2, shift_num);
		Adjust_Wreye_Value(D_DDRPHY_DS1_TMG1_F1_ADDR + DDR_PHY_OFFSET, wreye_val3, shift_num);
		is_check_done = 2;
	}

#if defined (AX_DDR_DEBUG)
	ax_print_str("\r\nPass Result: 0x");
	ax_print_num((readl(D_DDRPHY_DS0_TMG1_F0_ADDR) & 0x7ff), 16);
	ax_print_str(", 0x");
	ax_print_num((readl(D_DDRPHY_DS1_TMG1_F0_ADDR) & 0x7ff), 16);
	ax_print_str(", 0x");
	ax_print_num((readl(D_DDRPHY_DS0_TMG1_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff), 16);
	ax_print_str(", 0x");
	ax_print_num((readl(D_DDRPHY_DS1_TMG1_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff), 16);
	ax_print_str("\r\n");
#endif
}

u32 wreye_train_flow(void)
{
	u32 reg_data;
	u32 wreye_train_temp = 0;
	u32 wreye_val0 = 0;
	u32 wreye_val1 = 0;
	u32 wreye_val2 = 0;
	u32 wreye_val3 = 0;
	w_pass_window_dqeye = 0;

	set_ddrmc_train_control(0xFF000807, (1 << 11) | (1 << 2) | 1, 1);

	//polling train done
	for (int i=0; i<1000 ; i++)  {
		//#1us;
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (0x1 << 28))  {//rf_train_wreye_done
			if(reg_data & (0x1 << 29))  {//rf_train_wreye_fail
#if defined (AX_DDR_DEBUG)
				ax_print_str(", D_DDRMC_TRAIN_RES0_ADDR=0x");
				ax_print_num(reg_data, 16);
				ax_print_str(", D_DDRMC_TRAIN_RES2_ADDR=0x");
				ax_print_num(readl(D_DDRMC_TRAIN_RES2_ADDR), 16);
				ax_print_str(", D_DDRMC_TRAIN_RES3_ADDR=0x");
				ax_print_num(readl(D_DDRMC_TRAIN_RES3_ADDR), 16);
				ax_print_str(", D_DDRMC_TRAIN_CTRL1_ADDR=0x");
				ax_print_num(readl(D_DDRMC_TRAIN_CTRL1_ADDR), 16);
				ax_print_str(", D_DDRMC_TRAIN_CTRL3_ADDR=0x");
				ax_print_num(readl(D_DDRMC_TRAIN_CTRL3_ADDR), 16);
				ax_print_str(", D_DDRMC_TRAIN_CTRL4_ADDR=0x");
				ax_print_num(readl(D_DDRMC_TRAIN_CTRL4_ADDR), 16);
#endif
				ax_print_str("\r\nwreye train fail !\n");
			} else  {
#if defined (AX_DDR_DEBUG)
				ax_print_str(", D_DDRMC_TRAIN_RES0_ADDR=0x");
				ax_print_num(reg_data, 16);
#endif
				wreye_train_temp = readl(D_DDRMC_TRAIN_RES2_ADDR);
				reg_data = readl(D_DDRMC_TRAIN_RES3_ADDR); //pass_window
				wreye_val0 = ddrphy_dfs_readl(D_DDRPHY_DS0_TMG1_F0_ADDR, dfs_freq)  & 0x7ff;
				wreye_val1 = ddrphy_dfs_readl(D_DDRPHY_DS1_TMG1_F0_ADDR, dfs_freq)  & 0x7ff;
				wreye_val2 = ddrphy_dfs_readl(D_DDRPHY_DS0_TMG1_F0_ADDR + DDR_PHY_OFFSET, dfs_freq) & 0x7ff;
				wreye_val3 = ddrphy_dfs_readl(D_DDRPHY_DS1_TMG1_F0_ADDR + DDR_PHY_OFFSET, dfs_freq) & 0x7ff;
#if defined (AX_DDR_DEBUG)
				ax_print_str(", D_DDRMC_TRAIN_RES2_ADDR=0x");
				ax_print_num(wreye_train_temp, 16);
				ax_print_str(", D_DDRMC_TRAIN_RES3_ADDR=0x");
				ax_print_num(reg_data, 16);
				ax_print_str(", D_DDRMC_TRAIN_CTRL1_ADDR=0x");
				ax_print_num(readl(D_DDRMC_TRAIN_CTRL1_ADDR), 16);
				ax_print_str(", D_DDRMC_TRAIN_CTRL3_ADDR=0x");
				ax_print_num(readl(D_DDRMC_TRAIN_CTRL3_ADDR), 16);
				ax_print_str(", D_DDRMC_TRAIN_CTRL4_ADDR=0x");
				ax_print_num(readl(D_DDRMC_TRAIN_CTRL4_ADDR), 16);
				ax_print_str(", Pass Result: 0x");
				ax_print_num(wreye_val0, 16);
				ax_print_str(", 0x");
				ax_print_num(wreye_val1, 16);
				ax_print_str(", 0x");
				ax_print_num(wreye_val2, 16);
				ax_print_str(", 0x");
				ax_print_num(wreye_val3, 16);
				ax_print_str("\r\n");
#endif
				w_pass_window_dqeye = reg_data;
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
				reg_bit_set(DDR_DUMMY_REG_SW1, 4, 1, 0x1);
#endif
				#if (0 == LPDDR4_DFS_CONFIG)
				wreye_pass = 1;
				#endif
			}
			break;
		}
		if(i==999)  {
			ax_print_str("\r\npolling wreye_training done timeout !\n");
		}
	}

#if LPDDR4_RETRAIN
	retrain_base_value_f0();
#endif

	set_ddrmc_train_control(0xFF000807, (1 << 11) | (1 << 2) | (1 << 1), 1);
	DQ_Write_delay_proc(wreye_val0, wreye_val1, wreye_val2, wreye_val3);
	set_ddrmc_train_control(0xFF000807, 0x0, 1);

	return wreye_train_temp;
}

#ifndef DDR_OFFLINE_SCAN
static int lp4_vref_check(struct ddr_dfs_vref_t * param)
{
	u8 cs;

	if (!param)
		return -1;

	for (cs = 0; cs < lpddr4_rank_num; cs++) {
		if ((((param->dram_VREF_CA[cs] > 0x32) && (param->dram_VREF_CA[cs] < 0x40)) || (param->dram_VREF_CA[cs] > 0x72)) ||
			(((param->dram_VREF_DQ[cs] > 0x32) && (param->dram_VREF_DQ[cs] < 0x40)) || (param->dram_VREF_DQ[cs] > 0x72)))
			break;
	}

	if (cs == lpddr4_rank_num)
		return 0;
	else
		return -1;
}
#endif

#if LPDDR4_DFS_CONFIG
void ddrmc_freq1_timing_init(void)
{
	u8 i = 0,j = 0;
	u32 base_addr;

	base_addr = D_DDRMC_TMG0_F1_ADDR;
	for (i = 0; i < 21; i++) {
		ddr_writel(base_addr + i *4, lpddr4mc_timing[1][i]);
	}

	base_addr = D_DDRMC_TMG24_F1_ADDR;
	for (j = 0; i < 29; i++, j++) {
		ddr_writel(base_addr + j *4, lpddr4mc_timing[1][i]);
	}
}

void ddrmc_dfs_freq1_timing_init(void)
{
	u32 reg_data;

	reg_data = readl(D_DDRMC_TMG14_F1_ADDR);
	reg_data &= ~0x77000000;
	reg_data |= 0x1 << 25; //rf_auto_ref_allcs_f1
	reg_data |= 0x1 << 24; //rf_auto_ref_en_f0
	ddr_writel(D_DDRMC_TMG14_F1_ADDR, reg_data);
}

void ddrphy_freq1_timing_init(u32 offset_addr, const u32 ddrphy_timing[][31])
{
	u32 i,j = 0;
	u32 base_addr;
	for (i = 0, j = 0; i <= 30; i++, j++) {
		if (i < 14) {
			base_addr = D_DDRPHY_GEN_TMG0_F1_ADDR;
		} else if (i >= 14 && i <= 16) {
			base_addr = D_DDRPHY_AC_TMG0_F1_ADDR;
		} else if (i >= 17 && i <= 23){
			base_addr = D_DDRPHY_DS0_TMG0_F1_ADDR;
		} else {
			base_addr = D_DDRPHY_DS1_TMG0_F1_ADDR;
		}
		if (i == 14 || i == 17 || i == 24) j = 0;
		ddr_writel(offset_addr + base_addr + j * 4, ddrphy_timing[1][i]);
	}
}


static void ddr_dfs_switch(u32 target_freq)
{
	struct ddr_dfs_reg *dfs_reg;
	void *ddr_dfs_base = NULL;
	ddr_dfs_base = (void *)0x210064;
	dfs_reg = (struct ddr_dfs_reg *)ddr_dfs_base;
	dfs_reg->dfs_clksrc &= ~((0x3 << 10) | (0x3 << 8));//dfs_clksrc_fsp0_mux,dfs_clksrc_fsp1_mux
	dfs_reg->dfs_clksrc |= (0x7 << 8);
	dfs_reg->dfs_ctrl |= (1 << 4);

	if(target_freq == 0) {
		dfs_reg->dfs_ctrl &= ~(3 << 1);//target_freq
	} else if (target_freq == 1) {
		dfs_reg->dfs_ctrl |= (1 << 1);
	}
	dfs_reg->dfs_ctrl |= 0x1;//dfs_ctrl_trig dfs start
	while((dfs_reg->dfs_int & 0x1) == 0x0);//wait dfs done
	dfs_reg->dfs_int |= 1 << 2;
}
#endif
// ### SIPEED EDIT ###
// #ifdef AX630C_LPDDR4_DUAL_RANK
void save_dq_dl_value_rank0(void)
{
	u32 reg_data;

	reg_data = readl(D_DDRPHY_DS0_CFG6_ADDR);
	dq_dl_value[0] = reg_data;//rf_dly_out_dq0-dq3_dl0_sel_ds0
	reg_data = 0;
	ddr_writel(D_DDRPHY_DS0_CFG6_ADDR,reg_data);

	reg_data = readl(D_DDRPHY_DS0_CFG7_ADDR);
	dq_dl_value[1] = reg_data;//rf_dly_out_dq4-dq7_dl0_sel_ds0
	reg_data = 0;
	ddr_writel(D_DDRPHY_DS0_CFG7_ADDR,reg_data);

	reg_data = readl(D_DDRPHY_DS1_CFG6_ADDR);
	dq_dl_value[2] = reg_data;//rf_dly_out_dq0-dq3_dl0_sel_ds1
	reg_data = 0;
	ddr_writel(D_DDRPHY_DS1_CFG6_ADDR,reg_data);

	reg_data = readl(D_DDRPHY_DS1_CFG7_ADDR);
	dq_dl_value[3] = reg_data;//rf_dly_out_dq4-dq7_dl0_sel_ds1
	reg_data = 0;
	ddr_writel(D_DDRPHY_DS1_CFG7_ADDR,reg_data);

	reg_data = readl(D_DDRPHY_DS0_CFG6_ADDR + DDR_PHY_OFFSET);
	dq_dl_value[4] = reg_data;//rf_dly_out_dq0-dq3_dl0_sel_ds0
	reg_data = 0;
	ddr_writel(D_DDRPHY_DS0_CFG6_ADDR + DDR_PHY_OFFSET,reg_data);

	reg_data = readl(D_DDRPHY_DS0_CFG7_ADDR + DDR_PHY_OFFSET);
	dq_dl_value[5] = reg_data;//rf_dly_out_dq4-dq7_dl0_sel_ds0
	reg_data = 0;
	ddr_writel(D_DDRPHY_DS0_CFG7_ADDR + DDR_PHY_OFFSET,reg_data);

	reg_data = readl(D_DDRPHY_DS1_CFG6_ADDR + DDR_PHY_OFFSET);
	dq_dl_value[6] = reg_data;//rf_dly_out_dq0-dq3_dl0_sel_ds1
	reg_data = 0;
	ddr_writel(D_DDRPHY_DS1_CFG6_ADDR + DDR_PHY_OFFSET,reg_data);

	reg_data = readl(D_DDRPHY_DS1_CFG7_ADDR + DDR_PHY_OFFSET);
	dq_dl_value[7] = reg_data;//rf_dly_out_dq4-dq7_dl0_sel_ds1
	reg_data = 0;
	ddr_writel(D_DDRPHY_DS1_CFG7_ADDR + DDR_PHY_OFFSET,reg_data);
}

void recovery_dq_dl_value_rank0(void)
{
	u32 reg_data;
	reg_data = readl(D_DDRPHY_DS0_CFG6_ADDR);
	reg_data = dq_dl_value[0];//rf_dly_out_dq0-dq3_dl0_sel_ds0
	ddr_writel(D_DDRPHY_DS0_CFG6_ADDR,reg_data);

	reg_data = readl(D_DDRPHY_DS0_CFG7_ADDR);
	reg_data = dq_dl_value[1];//rf_dly_out_dq4-dq7_dl0_sel_ds0
	ddr_writel(D_DDRPHY_DS0_CFG7_ADDR,reg_data);

	reg_data = readl(D_DDRPHY_DS1_CFG6_ADDR);
	reg_data = dq_dl_value[2];//rf_dly_out_dq0-dq3_dl0_sel_ds1
	ddr_writel(D_DDRPHY_DS1_CFG6_ADDR,reg_data);

	reg_data = readl(D_DDRPHY_DS1_CFG7_ADDR);
	reg_data = dq_dl_value[3];//rf_dly_out_dq4-dq7_dl0_sel_ds1
	ddr_writel(D_DDRPHY_DS1_CFG7_ADDR,reg_data);

	reg_data = readl(D_DDRPHY_DS0_CFG6_ADDR + DDR_PHY_OFFSET);
	reg_data = dq_dl_value[4];//rf_dly_out_dq0-dq3_dl0_sel_ds0
	ddr_writel(D_DDRPHY_DS0_CFG6_ADDR + DDR_PHY_OFFSET, reg_data);

	reg_data = readl(D_DDRPHY_DS0_CFG7_ADDR + DDR_PHY_OFFSET);
	reg_data = dq_dl_value[5];//rf_dly_out_dq4-dq7_dl0_sel_ds0
	ddr_writel(D_DDRPHY_DS0_CFG7_ADDR + DDR_PHY_OFFSET, reg_data);

	reg_data = readl(D_DDRPHY_DS1_CFG6_ADDR + DDR_PHY_OFFSET);
	reg_data = dq_dl_value[6];//rf_dly_out_dq0-dq3_dl0_sel_ds1
	ddr_writel(D_DDRPHY_DS1_CFG6_ADDR + DDR_PHY_OFFSET, reg_data);

	reg_data = readl(D_DDRPHY_DS1_CFG7_ADDR + DDR_PHY_OFFSET);
	reg_data = dq_dl_value[7];//rf_dly_out_dq4-dq7_dl0_sel_ds1
	ddr_writel(D_DDRPHY_DS1_CFG7_ADDR + DDR_PHY_OFFSET, reg_data);
}

void ddrphy_train_soft_update_dq(void)
{
	//u32 dl_num;

//	for (dl_num = 0; dl_num < 2; dl_num++) {
		/*updated dll cpst start ds 0/1/2/3*/
		//REG32(DDRPHY_DQ0_(dll_dq_cfg[dl_num])) |=  (1 << 2);
		//REG32(DDRPHY_DQ0_(dll_dq_cfg[dl_num])) &= ~(1 << 2);
		//REG32(DDRPHY_DQ1_(dll_dq_cfg[dl_num])) |=  (1 << 2);
		//REG32(DDRPHY_DQ1_(dll_dq_cfg[dl_num])) &= ~(1 << 2);
		reg_bit_set(D_DDRPHY_DS0_TMG0_F0_ADDR, 2, 1, 0x1);//rf_dl_cpst_start_dqx
		reg_bit_set(D_DDRPHY_DS0_TMG0_F0_ADDR, 2, 1, 0x0);//rf_dl_cpst_start_dqx
		reg_bit_set(D_DDRPHY_DS1_TMG0_F0_ADDR, 2, 1, 0x1);//rf_dl_cpst_start_dqx
		reg_bit_set(D_DDRPHY_DS1_TMG0_F0_ADDR, 2, 1, 0x0);//rf_dl_cpst_start_dqx
		reg_bit_set(D_DDRPHY_DS0_TMG0_F0_ADDR + DDR_PHY_OFFSET, 2, 1, 0x1);//rf_dl_cpst_start_dqx
		reg_bit_set(D_DDRPHY_DS0_TMG0_F0_ADDR + DDR_PHY_OFFSET, 2, 1, 0x0);//rf_dl_cpst_start_dqx
		reg_bit_set(D_DDRPHY_DS1_TMG0_F0_ADDR + DDR_PHY_OFFSET, 2, 1, 0x1);//rf_dl_cpst_start_dqx
		reg_bit_set(D_DDRPHY_DS1_TMG0_F0_ADDR + DDR_PHY_OFFSET, 2, 1, 0x0);//rf_dl_cpst_start_dqx


//		reg_bit_set(DDRPHY_DQ1_(PARM_DDRPHY_GEN_TMG10_F0_ADDR + train_info.ddr_freq_num * 0x80), 0, 8, vrefdq_val);

//		reg_bit_set(DDRPHY_DQ0_(dll_dq_cfg[dl_num]), 2, 1, 0x1);//rf_dl_cpst_start_dqx
//		reg_bit_set(DDRPHY_DQ0_(dll_dq_cfg[dl_num]), 2, 1, 0x0);//rf_dl_cpst_start_dqx
//		reg_bit_set(DDRPHY_DQ1_(dll_dq_cfg[dl_num]), 2, 1, 0x1);//rf_dl_cpst_start_dqx
//		reg_bit_set(DDRPHY_DQ1_(dll_dq_cfg[dl_num]), 2, 1, 0x0);//rf_dl_cpst_start_dqx
//	}
	udelay(1);

}

void ddrphy_train_lpddr4_wr2diff(void)
{
	u32 wreye_val0_cs0 = 0;
	u32 wreye_val1_cs0 = 0;
	u32 wreye_val2_cs0 = 0;
	u32 wreye_val3_cs0 = 0;

	wreye_val0_cs0 = readl(D_DDRPHY_DS0_TMG1_F0_ADDR) & 0x7ff;
	wreye_val1_cs0 = readl(D_DDRPHY_DS1_TMG1_F0_ADDR) & 0x7ff;
	wreye_val2_cs0 = readl(D_DDRPHY_DS0_TMG1_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff;
	wreye_val3_cs0 = readl(D_DDRPHY_DS1_TMG1_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff;

	// modify wreye value
#if 0
#ifdef AX_DDR_DEBUG
	ax_print_str("wr2diff: TMG2 0x");
	ax_print_num(readl(D_DDRPHY_DS0_TMG2_F0_ADDR), 16);
	ax_print_str(", 0x");
	ax_print_num(readl(D_DDRPHY_DS1_TMG2_F0_ADDR), 16);
	ax_print_str(", 0x");
	ax_print_num(readl(D_DDRPHY_DS0_TMG2_F0_ADDR + DDR_PHY_OFFSET), 16);
	ax_print_str(", 0x");
	ax_print_num(readl(D_DDRPHY_DS1_TMG2_F0_ADDR + DDR_PHY_OFFSET), 16);
#endif
	reg_bit_set(D_DDRPHY_DS0_TMG2_F0_ADDR, 0, 11, wreye_val0_cs0);//rf_clkwr_diff_dl_sel_ds0_f0
	reg_bit_set(D_DDRPHY_DS1_TMG2_F0_ADDR, 0, 11, wreye_val1_cs0);//rf_clkwr_diff_dl_sel_ds1_f0
	reg_bit_set(D_DDRPHY_DS0_TMG2_F0_ADDR + DDR_PHY_OFFSET, 0, 11, wreye_val2_cs0);//rf_clkwr_diff_dl_sel_ds2_f0
	reg_bit_set(D_DDRPHY_DS1_TMG2_F0_ADDR + DDR_PHY_OFFSET, 0, 11, wreye_val3_cs0);//rf_clkwr_diff_dl_sel_ds3_f0
#else
	reg_bit_set(D_DDRPHY_GEN_TMG12_F0_ADDR, 0, 11, wreye_val0_cs0);//rf_clkwr_diff_dl_sel_ds0_f0
	reg_bit_set(D_DDRPHY_GEN_TMG12_F0_ADDR, 16, 11, wreye_val1_cs0);//rf_clkwr_diff_dl_sel_ds1_f0
	reg_bit_set(D_DDRPHY_GEN_TMG12_F0_ADDR + DDR_PHY_OFFSET, 0, 11, wreye_val2_cs0);//rf_clkwr_diff_dl_sel_ds2_f0
	reg_bit_set(D_DDRPHY_GEN_TMG12_F0_ADDR + DDR_PHY_OFFSET, 16, 11, wreye_val3_cs0);//rf_clkwr_diff_dl_sel_ds3_f0
#endif
}

void ddrphy_train_lpddr4_wr2rank(void)
{
	u32 wreye_val0_cs0 = 0;
	u32 wreye_val1_cs0 = 0;
	u32 wreye_val2_cs0 = 0;
	u32 wreye_val3_cs0 = 0;
	u32 wreye_val0_cs1 = 0;
	u32 wreye_val1_cs1 = 0;
	u32 wreye_val2_cs1 = 0;
	u32 wreye_val3_cs1 = 0;

#if 0
	wreye_val0_cs0 = readl(D_DDRPHY_DS0_TMG2_F0_ADDR) & 0x7ff;
	wreye_val1_cs0 = readl(D_DDRPHY_DS1_TMG2_F0_ADDR) & 0x7ff;
	wreye_val2_cs0 = readl(D_DDRPHY_DS0_TMG2_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff;
	wreye_val3_cs0 = readl(D_DDRPHY_DS1_TMG2_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff;
#else
	wreye_val0_cs0 = readl(D_DDRPHY_GEN_TMG12_F0_ADDR) & 0x7ff;
	wreye_val1_cs0 = (readl(D_DDRPHY_GEN_TMG12_F0_ADDR) >> 16) & 0x7ff;
	wreye_val2_cs0 = readl(D_DDRPHY_GEN_TMG12_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff;
	wreye_val3_cs0 = (readl(D_DDRPHY_GEN_TMG12_F0_ADDR + DDR_PHY_OFFSET) >> 16) & 0x7ff;
#endif

	wreye_val0_cs1 = readl(D_DDRPHY_DS0_TMG1_F0_ADDR) & 0x7ff;
	wreye_val1_cs1 = readl(D_DDRPHY_DS1_TMG1_F0_ADDR) & 0x7ff;
	wreye_val2_cs1 = readl(D_DDRPHY_DS0_TMG1_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff;
	wreye_val3_cs1 = readl(D_DDRPHY_DS1_TMG1_F0_ADDR + DDR_PHY_OFFSET) & 0x7ff;

	// modify wreye value
	reg_bit_set(D_DDRPHY_DS0_TMG2_F0_ADDR, 15, 1, 0x1);//rf_clkwr_diff_dl_cpst_en_ds0_f0
	reg_bit_set(D_DDRPHY_DS1_TMG2_F0_ADDR, 15, 1, 0x1);//rf_clkwr_diff_dl_cpst_en_ds1_f0
	reg_bit_set(D_DDRPHY_DS0_TMG2_F0_ADDR + DDR_PHY_OFFSET, 15, 1, 0x1);//rf_clkwr_diff_dl_cpst_en_ds2_f0
	reg_bit_set(D_DDRPHY_DS1_TMG2_F0_ADDR + DDR_PHY_OFFSET, 15, 1, 0x1);//rf_clkwr_diff_dl_cpst_en_ds3_f0

	reg_bit_set(D_DDRPHY_DS0_TMG2_F0_ADDR, 1, 10,
				(wreye_val0_cs1 > wreye_val0_cs0) ? (wreye_val0_cs1 - wreye_val0_cs0)>>3 : (wreye_val0_cs0 - wreye_val0_cs1)>>3);
	reg_bit_set(D_DDRPHY_DS1_TMG2_F0_ADDR, 1, 10,
				(wreye_val1_cs1 > wreye_val1_cs0) ? (wreye_val1_cs1 - wreye_val1_cs0)>>3 : (wreye_val1_cs0 - wreye_val1_cs1)>>3);
	reg_bit_set(D_DDRPHY_DS0_TMG2_F0_ADDR + DDR_PHY_OFFSET, 1, 10,
				(wreye_val2_cs1 > wreye_val2_cs0) ? (wreye_val2_cs1 - wreye_val2_cs0)>>3 : (wreye_val2_cs0 - wreye_val2_cs1)>>3);
	reg_bit_set(D_DDRPHY_DS1_TMG2_F0_ADDR + DDR_PHY_OFFSET, 1, 10,
				(wreye_val3_cs1 > wreye_val3_cs0) ? (wreye_val3_cs1 - wreye_val3_cs0)>>3 : (wreye_val3_cs0 - wreye_val3_cs1)>>3);

	reg_bit_set(D_DDRPHY_DS0_TMG2_F0_ADDR, 0, 1, (wreye_val0_cs1 > wreye_val0_cs0) ? 0x1 : 0x0);
	reg_bit_set(D_DDRPHY_DS1_TMG2_F0_ADDR, 0, 1, (wreye_val1_cs1 > wreye_val1_cs0) ? 0x1 : 0x0);
	reg_bit_set(D_DDRPHY_DS0_TMG2_F0_ADDR + DDR_PHY_OFFSET, 0, 1, (wreye_val2_cs1 > wreye_val2_cs0) ? 0x1 : 0x0);
	reg_bit_set(D_DDRPHY_DS1_TMG2_F0_ADDR + DDR_PHY_OFFSET, 0, 1, (wreye_val3_cs1 > wreye_val3_cs0) ? 0x1 : 0x0);

	reg_bit_set(D_DDRPHY_DS0_TMG1_F0_ADDR, 0, 11, (wreye_val0_cs1 > wreye_val0_cs0) ? wreye_val0_cs0 : wreye_val0_cs1);
	reg_bit_set(D_DDRPHY_DS1_TMG1_F0_ADDR, 0, 11, (wreye_val1_cs1 > wreye_val1_cs0) ? wreye_val1_cs0 : wreye_val1_cs1);
	reg_bit_set(D_DDRPHY_DS0_TMG1_F0_ADDR + DDR_PHY_OFFSET, 0, 11, (wreye_val2_cs1 > wreye_val2_cs0) ? wreye_val2_cs0 : wreye_val2_cs1);
	reg_bit_set(D_DDRPHY_DS1_TMG1_F0_ADDR + DDR_PHY_OFFSET, 0, 11, (wreye_val3_cs1 > wreye_val3_cs0) ? wreye_val3_cs0 : wreye_val3_cs1);

	ddrphy_train_soft_update_dq();

#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nWREYE Pass  Value: 0x");
	ax_print_num(wreye_val0_cs0, 16);
	ax_print_str(", 0x");
	ax_print_num(wreye_val1_cs0, 16);
	ax_print_str(", 0x");
	ax_print_num(wreye_val2_cs0, 16);
	ax_print_str(", 0x");
	ax_print_num(wreye_val3_cs0, 16);
	ax_print_str(", 0x");
	ax_print_num(wreye_val0_cs1, 16);
	ax_print_str(", 0x");
	ax_print_num(wreye_val1_cs1, 16);
	ax_print_str(", 0x");
	ax_print_num(wreye_val2_cs1, 16);
	ax_print_str(", 0x");
	ax_print_num(wreye_val3_cs1, 16);
	ax_print_str("\r\n");
#endif

}

#ifdef AX_DDR_DEBUG
static void ddrphy_wreye_diff_config_print(void)
{
	int i;
	u32 reg_addr = 0;

	ax_print_str("\r\n=================== rank diff print: rf_dly_out_dq_dl0/1 =================\n");
	for (i = 0; i < 8; i++) {
		if (0 == i)
			reg_addr = D_DDRPHY_DS0_TMG1_F0_ADDR;
		else if (2 == i)
			reg_addr = D_DDRPHY_DS1_TMG1_F0_ADDR;
		else if (4 == i)
			reg_addr = D_DDRPHY_DS0_TMG1_F0_ADDR + DDR_PHY_OFFSET;
		else if (6 == i)
			reg_addr = D_DDRPHY_DS1_TMG1_F0_ADDR + DDR_PHY_OFFSET;
		ax_print_str("\r\nreg 0x");
		ax_print_num(reg_addr, 16);
		ax_print_str(", val 0x");
		ax_print_num(readl(reg_addr), 16);
		reg_addr += 4;
	}
	ax_print_str("\r\n=================== rank diff print: rf_dly_out/in_dqX_dl0/1 =================\n");
	for (i = 0; i < 48; i++) {
		if (0 == i)
			reg_addr = D_DDRPHY_DS0_CFG6_ADDR;
		else if (12 == i)
			reg_addr = D_DDRPHY_DS1_CFG6_ADDR;
		else if (24 == i)
			reg_addr = D_DDRPHY_DS0_CFG6_ADDR + DDR_PHY_OFFSET;
		else if (36 == i)
			reg_addr = D_DDRPHY_DS1_CFG6_ADDR + DDR_PHY_OFFSET;
		ax_print_str("\r\nreg 0x");
		ax_print_num(reg_addr, 16);
		ax_print_str(", val 0x");
		ax_print_num(readl(reg_addr), 16);
		reg_addr += 4;
	}
}
#endif
// #endif
// ### SIPEED EDIT END ###

void ddrmc_train_flow_lp4_sequence(void * rom_param)
{
	u32 reg_data;
	u32 bit_shift = 0;
	u32 byte_num;
	u8 cs_val = 0;
	u8 cs_num;
// ### SIPEED EDIT ###
	u8 cs_position = 0;
	if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
		cs_position = (readl(D_DDRMC_CFG2_ADDR) >> 8) & 0x7;
	} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	cs_position = (readl(D_DDRMC_CFG2_ADDR) >> 8) & 0x7;
#endif
	}
// ### SIPEED EDIT END ###

#ifndef DDR_OFFLINE_SCAN
	struct ddr_dfs_vref_t * rom_dfs_vref_info = NULL;
	if (rom_param && (ddr_freqs == ((struct ddr_dfs_vref_t *)rom_param)->freq) &&
		(0 == lp4_vref_check((struct ddr_dfs_vref_t *)rom_param)))
		rom_dfs_vref_info = (struct ddr_dfs_vref_t *)rom_param;
#endif

#if LPDDR4_DFS_CONFIG
	for(dfs_freq = 0; dfs_freq <= ddr_dfs_sel; dfs_freq++) {//DFS 0/1
		if (dfs_freq == 1) {
		#ifndef DDR_OFFLINE_SCAN
			if(rom_dfs_vref_info)
				rom_dfs_vref_info++;
		#endif
			p_iram_dfs_vref_info++;
			p_iram_dfs_vref_info->freq = ddr_dfs_f1_freq;
			ddr_freqs = ddr_dfs_f1_freq;
			reg_mask_set(D_DDRMC_CFG26_ADDR, 0x1377, 0x1167);//rf_dfs_en mrw_0-7
			if(ddr_freqs == DDR_CLK_3733 || ddr_freqs == DDR_CLK_3400 || ddr_freqs == DDR_CLK_3200 || ddr_freqs == DDR_CLK_2800) {
				ddr_writel(D_DDRMC_TMG27_F1_ADDR, 0x154022d);//f1:MR1 0x54,MR2 0x2d
			} else if(ddr_freqs == DDR_CLK_2666 || ddr_freqs == DDR_CLK_2400) {
				ddr_writel(D_DDRMC_TMG27_F1_ADDR, 0x1440224);//f1:MR1 0x44,MR2 0x24
			} else if (ddr_freqs == DDR_CLK_2133) {
				ddr_writel(D_DDRMC_TMG27_F1_ADDR, 0x134021b);//f1:MR1 0x34,MR2 0x1b
			}

			ddr_writel(D_DDRMC_TMG26_F1_ADDR, 0x16060b06);//f1:MR22 0x6,MR11 0x6
			reg_mask_set(D_DDRMC_TMG14_F0_ADDR, 0x0,(1 << 24) | (1 << 25));//drf_auto_ref_en drf_auto_ref_allcs
			reg_mask_set(D_DDRMC_TMG14_F1_ADDR, 0x0,(1 << 24) | (1 << 25));
			ddr_dfs_switch(dfs_freq);
			reg_mask_set(D_DDRMC_TMG14_F0_ADDR, (1 << 24) | (1 << 25),0);
			reg_mask_set(D_DDRMC_TMG14_F1_ADDR, (1 << 24) | (1 << 25),0);
			//clear rddsk done
			set_ddrmc_train_control(0xFF000087, (1 << 7) | (1 << 2) | (1 << 1), 1);
			set_ddrmc_train_control(0xFF000087, 0, 1);
			set_ddrmc_train_control(0xFF000407, 0, 1);
			reg_mask_set(D_DDRMC_TMG14_F0_ADDR, (1 << 24) | 1 << 25, (1 << 25));
			ddr_writel(D_DDRMC_TRAIN_CTRL7_ADDR,0x76543210);
			wrlvl_back_ind = 0;
		}
#endif

	reg_mask_set(D_DDRMC_CFG16_ADDR, 0x1, 0);
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		bit_shift = (1 << 28);
	}
	reg_mask_set(D_DDRMC_BIST_CTRL0_ADDR, 0x3070F000, (0x5 << 20) | (0x1 << 12) | (bit_shift));
	reg_mask_set(D_DDRMC_TRAIN_CTRL1_ADDR, (0xff << 20), 0xf0 << 20);
	reg_mask_set(D_DDRMC_TRAIN_CTRL3_ADDR, 0xFF0FF7FF, (0x10 << 24) | 0x400);
	reg_mask_set(D_DDRMC_TRAIN_CTRL4_ADDR, (0xff << 12), 0xf0);
	reg_mask_set(D_DDRMC_TRAIN_CTRL5_ADDR, 0xFF0FF, (0x2 << 12) | 0x4);
	reg_mask_set(D_DDRMC_TRAIN_CTRL6_ADDR, 0xff, 0xa);
#if LPDDR4_DFS_CONFIG
	if(0 == dfs_freq) {
		reg_mask_set(D_DDRMC_TMG14_F0_ADDR, (0x3 << 24), 0x1 << 25);
		ddr_writel(D_DDRMC_TMG27_F0_ADDR, 0x1240212);//f0:MR1 0x24,MR2 0x12
		ddr_writel(D_DDRMC_TMG26_F0_ADDR, 0x16000b00);//f0:MR22 0x6,MR11 0x6
	}
#else
	reg_mask_set(D_DDRMC_TMG14_F0_ADDR, (0x3 << 24), 0x1 << 25);
#endif
	reg_mask_set(D_DDRMC_CFG3_ADDR, 0, (0x1 << 28) | (0x1 << 29));
	ddr_dfs_reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR, 0, (0x3 << 12), dfs_freq);
	ddr_dfs_reg_mask_set(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET, 0, (0x3 << 12), dfs_freq);
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		reg_mask_set(D_DDRMC_TRAIN_CTRL8_ADDR, 0, (0x1 << 28));
	}
	ca_train_cfg();
#ifdef LP4_WRLVL_TRAIN_EN
	wrlvl_train_cfg();
#endif

// ### SIPEED EDIT ###
if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
	if ((AX620Q_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id)) {
		lpddr4_rank_num = (((ddr_types & 0x300) >> 8) == DDR_2CS) ? 2 : 1;
		if (cs_position)
			cs_position += 26;
		else {
			ax_print_str("\r\nERROR: dual rank training not support ba,cs,col cfg\n");
			return;
		}
#ifdef AX_DDR_DEBUG
		if (2 == lpddr4_rank_num) {
			ax_print_str("\r\ndual rank: reg 0x");
			ax_print_num(D_DDRPHY_AC_TMG2_F0_ADDR, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_AC_TMG2_F0_ADDR), 16);

			ax_print_str("\r\nreg 0x");
			ax_print_num(D_DDRPHY_GEN_TMG13_F0_ADDR, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_GEN_TMG13_F0_ADDR), 16);

			ax_print_str("\r\nreg 0x");
			ax_print_num(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET), 16);
		}
#endif
	}
} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if ((AX620Q_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id)) {
		lpddr4_rank_num = (((ddr_types & 0x300) >> 8) == DDR_2CS) ? 2 : 1;
		if (cs_position)
			cs_position += 26;
		else {
			ax_print_str("\r\nERROR: dual rank training not support ba,cs,col cfg\n");
			return;
		}
#ifdef AX_DDR_DEBUG
		if (2 == lpddr4_rank_num) {
			ax_print_str("\r\ndual rank: reg 0x");
			ax_print_num(D_DDRPHY_AC_TMG2_F0_ADDR, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_AC_TMG2_F0_ADDR), 16);

			ax_print_str("\r\nreg 0x");
			ax_print_num(D_DDRPHY_GEN_TMG13_F0_ADDR, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_GEN_TMG13_F0_ADDR), 16);

			ax_print_str("\r\nreg 0x");
			ax_print_num(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET, 16);
			ax_print_str(", val 0x");
			ax_print_num(readl(D_DDRPHY_GEN_TMG13_F0_ADDR + DDR_PHY_OFFSET), 16);
		}
#endif
	}
#endif
}
// ### SIPEED EDIT END ###

	for(cs_num = 0; cs_num < lpddr4_rank_num; cs_num++) {
// ### SIPEED EDIT ###
	if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
		if (cs_position < 32) {
			bit_shift = cs_num << cs_position;
			reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, (0x1 << cs_position), bit_shift);
		} else {
			bit_shift = cs_num << (cs_position - 32 + 12);
			reg_mask_set(D_DDRMC_TRAIN_CTRL1_ADDR, (0x1 << (cs_position - 32 + 12)), bit_shift);
		}
	} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if (cs_position < 32) {
		bit_shift = cs_num << cs_position;
		reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, (0x1 << cs_position), bit_shift);
	} else {
		bit_shift = cs_num << (cs_position - 32 + 12);
		reg_mask_set(D_DDRMC_TRAIN_CTRL1_ADDR, (0x1 << (cs_position - 32 + 12)), bit_shift);
	}
#else
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		bit_shift = cs_num << 31;
	} else {
		bit_shift = cs_num << 30;
	}
	reg_mask_set(D_DDRMC_TRAIN_CTRL2_ADDR, (0x3 << 30), bit_shift);
#endif
	}
// ### SIPEED EDIT END ###

		reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xF700000, (0xf << 24) | (0x3 << 21) | (cs_num << 20));
		if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
			reg_mask_set(D_DDRMC_BIST_CTRL0_ADDR, 0x70F000, 0x3 << 20);
		} else {
			reg_mask_set(D_DDRMC_BIST_CTRL0_ADDR, 0xF000, 0);
		}
		reg_mask_set(D_DDRMC_CFG32_ADDR, 0x11000000, (cs_num << 28));
#ifdef AX_DDR_DEBUG
		ax_print_str("\r\nD_DDRMC_TRAIN_CTRL0_ADDR = 0x");
		ax_print_num(readl(D_DDRMC_TRAIN_CTRL0_ADDR), 16);
		ax_print_str("\r\nD_DDRMC_TRAIN_CTRL1_ADDR = 0x");
		ax_print_num(readl(D_DDRMC_TRAIN_CTRL1_ADDR), 16);
		ax_print_str("\r\nD_DDRMC_TRAIN_CTRL2_ADDR = 0x");
		ax_print_num(readl(D_DDRMC_TRAIN_CTRL2_ADDR), 16);
		ax_print_str("\r\nD_DDRMC_CFG32_ADDR = 0x");
		ax_print_num(readl(D_DDRMC_CFG32_ADDR), 16);
#endif
		#if D_LP4_T12_FINAL_PINMUX_K
			reg_data = readl(D_DDRPHY_AC_CFG2_ADDR);
			reg_data = reg_data - 10;
			ddr_writel(D_DDRPHY_AC_CFG2_ADDR, reg_data);
		#endif
// ### SIPEED EDIT ###
	if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
		if(cs_num == 1) {
			save_dq_dl_value_rank0();
		}
	} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if(cs_num == 1) {
		save_dq_dl_value_rank0();
	}
#endif
	}
// ### SIPEED EDIT END ###

#ifdef DDR_OFFLINE_SCAN
		ca_vref_scan(NULL, TYPE_LPDDR4, cs_num, 0, 0x72, 0x1);
#elif defined (DDR_ENV_EDA)
		ca_vref_scan(rom_dfs_vref_info, TYPE_LPDDR4, cs_num, 0, 0x72, 0x40);
#else
		if (lpddr4_ca_train_en)
			ca_vref_scan(rom_dfs_vref_info, TYPE_LPDDR4, cs_num, LPDDR4_CAEYE_VREFCA_MIN, LPDDR4_CAEYE_VREFCA_MAX, 0x1);
#endif

		if (lpddr4_ca_train_en && lpddr4_ca_odt_en) {
			lpddr4_mrs_config(cs_num);
		}

		#if D_LP4_T12_FINAL_PINMUX_K
			reg_data = readl(D_DDRPHY_AC_CFG2_ADDR);
			reg_data += 10;
			ddr_writel(D_DDRPHY_AC_CFG2_ADDR, reg_data);
		#endif
// ### SIPEED EDIT ###
	if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
		if(cs_num == 1) {
			recovery_dq_dl_value_rank0();
		}
	} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
	if(cs_num == 1) {
		recovery_dq_dl_value_rank0();
	}
#endif
	}
// ### SIPEED EDIT END ###

		if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
			reg_mask_set(D_DDRMC_BIST_CTRL0_ADDR, 0x70F000, 0x5 << 20);
		} else {
			reg_mask_set(D_DDRMC_BIST_CTRL0_ADDR, 0xF000, 0);
		}
		reg_mask_set(D_DDRMC_TRAIN_CTRL7_ADDR, 0xffff, (0x1 << 4) | (0x2 << 8) | (0x3 << 12));
		ddr_dfs_reg_mask_set(D_DDRPHY_DS0_TMG5_F0_ADDR, 0x7ff, 0, dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS1_TMG5_F0_ADDR, 0x7ff, 0, dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS0_TMG5_F0_ADDR + DDR_PHY_OFFSET, 0x7ff, 0, dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS1_TMG5_F0_ADDR + DDR_PHY_OFFSET, 0x7ff, 0, dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS0_TMG0_F0_ADDR, 0, (1 << 2), dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS0_TMG0_F0_ADDR, (1 << 2), 0, dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS1_TMG0_F0_ADDR, 0, (1 << 2), dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS1_TMG0_F0_ADDR, (1 << 2), 0, dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS0_TMG0_F0_ADDR + DDR_PHY_OFFSET, 0, (1 << 2), dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS0_TMG0_F0_ADDR + DDR_PHY_OFFSET, (1 << 2), 0, dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS1_TMG0_F0_ADDR + DDR_PHY_OFFSET, 0, (1 << 2), dfs_freq);
		ddr_dfs_reg_mask_set(D_DDRPHY_DS1_TMG0_F0_ADDR + DDR_PHY_OFFSET, (1 << 2), 0, dfs_freq);

		for(byte_num = 0;byte_num < 4;byte_num++) {
			if(wrlvl_back_ind & (1 << byte_num)) {
				if(byte_num == 0) {
					reg_data = ddrphy_dfs_readl(D_DDRPHY_DS0_TMG6_F0_ADDR, dfs_freq);
					reg_data |= ((reg_data << 1) & 0x1FFFF); // set bits 16:1 to the value of bits 15:0 shifted left by 1
					ddrphy_dfs_writel(D_DDRPHY_DS0_TMG6_F0_ADDR,reg_data, dfs_freq);
				} else if(byte_num == 1) {
					reg_data = ddrphy_dfs_readl(D_DDRPHY_DS1_TMG6_F0_ADDR, dfs_freq);
					reg_data |= ((reg_data << 1) & 0x1FFFF); // set bits 16:1 to the value of bits 15:0 shifted left by 1
					ddrphy_dfs_writel(D_DDRPHY_DS1_TMG6_F0_ADDR,reg_data, dfs_freq);
				} else if(byte_num == 2) {
					reg_data = ddrphy_dfs_readl(D_DDRPHY_DS0_TMG6_F0_ADDR + DDR_PHY_OFFSET, dfs_freq);
					reg_data |= ((reg_data << 1) & 0x1FFFF); // set bits 16:1 to the value of bits 15:0 shifted left by 1
					ddrphy_dfs_writel(D_DDRPHY_DS0_TMG6_F0_ADDR + DDR_PHY_OFFSET,reg_data, dfs_freq);
				} else if(byte_num == 3) {
					reg_data = ddrphy_dfs_readl(D_DDRPHY_DS1_TMG6_F0_ADDR + DDR_PHY_OFFSET, dfs_freq);
					reg_data |= ((reg_data << 1) & 0x1FFFF); // set bits 16:1 to the value of bits 15:0 shifted left by 1
					ddrphy_dfs_writel(D_DDRPHY_DS1_TMG6_F0_ADDR + DDR_PHY_OFFSET,reg_data, dfs_freq);
				}
			}
		}

		if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
			reg_mask_set(D_DDRMC_TRAIN_CTRL7_ADDR, 0xffff, (0x1 << 4) | (0x3 << 8) | (0x2 << 12));
		}
#ifdef LP4_WRLVL_TRAIN_EN
		wrlvl_train_flow(cs_num);
		wrlvl_train_check(cs_num, &delay_cfg);
#endif
		if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
			reg_mask_set(D_DDRMC_TRAIN_CTRL7_ADDR, 0xffff, (0x1 << 4) | (0x2 << 8) | (0x3 << 12));
		} else {
			reg_mask_set(D_DDRMC_TRAIN_CTRL7_ADDR, 0xf << 12,  0x1 << 12);
		}

		read_pat_cfg(cs_num);

		//the last wrdsk flow will mask train_rddsk_done high and not clear and the next rddsk flow will be influenced
// ### SIPEED EDIT ###
		if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
			if(cs_num == 1) {
				reg_data = readl(D_DDRMC_TRAIN_CTRL0_ADDR);
				reg_data &= ~0xFF000087;
				reg_data |= 0xf << 24; //rf_train_byte_en
				reg_data |= 0x1 << 7; //rf_train_rdeye_en
				reg_data |= 0x1 << 2; //rf_train_enable
				reg_data |= 0x1 << 1; //rf_train_clear
				reg_data |= 0x0; //rf_train_start
				ddr_writel(D_DDRMC_TRAIN_CTRL0_ADDR, reg_data);

				reg_data = readl(D_DDRMC_TRAIN_CTRL0_ADDR);
				reg_data &= ~0xFF000087;
				reg_data |= 0xf << 24; //rf_train_byte_en
				ddr_writel(D_DDRMC_TRAIN_CTRL0_ADDR, reg_data);
			}
		} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
			if(cs_num == 1) {
				reg_data = readl(D_DDRMC_TRAIN_CTRL0_ADDR);
				reg_data &= ~0xFF000087;
				reg_data |= 0xf << 24; //rf_train_byte_en
				reg_data |= 0x1 << 7; //rf_train_rdeye_en
				reg_data |= 0x1 << 2; //rf_train_enable
				reg_data |= 0x1 << 1; //rf_train_clear
				reg_data |= 0x0; //rf_train_start
				ddr_writel(D_DDRMC_TRAIN_CTRL0_ADDR, reg_data);

				reg_data = readl(D_DDRMC_TRAIN_CTRL0_ADDR);
				reg_data &= ~0xFF000087;
				reg_data |= 0xf << 24; //rf_train_byte_en
				ddr_writel(D_DDRMC_TRAIN_CTRL0_ADDR, reg_data);
			}
#endif
		}
// ### SIPEED EDIT END ###

#ifdef DDR_OFFLINE_SCAN
		rd_vref_scan(NULL, TYPE_LPDDR4, cs_num, 0, 0x7f, 0x1);
#elif defined(DDR_ENV_EDA)
		rd_vref_scan(rom_dfs_vref_info, TYPE_LPDDR4, cs_num, 0, 0xff, 0x80);
#else
		rd_vref_scan(rom_dfs_vref_info, TYPE_LPDDR4, cs_num, (AX620QX_CHIP_E == chip_id) ? 0x8 : LPDDR4_RDEYE_ODT_VREFDQ_MIN, LPDDR4_RDEYE_ODT_VREFDQ_MAX, 0x1);
#endif

		write_pat_cfg();
#ifdef DDR_OFFLINE_SCAN
		wr_vref_scan(NULL, TYPE_LPDDR4, cs_num, 0, 0x72, 0x1);
#elif defined(DDR_ENV_EDA)
		wr_vref_scan(rom_dfs_vref_info, TYPE_LPDDR4, cs_num, 0, 0x72, 0x40);
#else
		wr_vref_scan(rom_dfs_vref_info, TYPE_LPDDR4, cs_num, LPDDR4_WREYE_ODT_VREFDQ_MIN, LPDDR4_WREYE_ODT_VREFDQ_MAX, 0x1);
#endif

// ### SIPEED EDIT ###
		if (IS_LPDDR4_DUAL_RANK(ddr_types)) {
			if (2 == lpddr4_rank_num) {
				if (0 == cs_num)
					ddrphy_train_lpddr4_wr2diff();
				else {
					ddrphy_train_lpddr4_wr2rank();
#ifdef AX_DDR_DEBUG
					ddrphy_wreye_diff_config_print();
#endif
				}
			}
		} else {
#ifdef AX630C_LPDDR4_DUAL_RANK
		if (2 == lpddr4_rank_num) {
			if (0 == cs_num)
				ddrphy_train_lpddr4_wr2diff();
			else {
				ddrphy_train_lpddr4_wr2rank();
#ifdef AX_DDR_DEBUG
				ddrphy_wreye_diff_config_print();
#endif
			}
		}
#endif
		}
// ### SIPEED EDIT END ###

	}//cs_num
#if LPDDR4_DFS_CONFIG
	}
#endif
#if defined (DDR_ENV_ATE) || defined (DDR_ENV_EDA)
	reg_bit_set(DDR_DUMMY_REG_SW1, 5, 1, 0x1);
#endif

	ddrmc_auto_ref(cs_val);
	reg_mask_set(D_DDRMC_CFG16_ADDR, 0, 0x1 | (1 << 8));
	reg_mask_set(D_DDRMC_TMG14_F0_ADDR, 0, (0x1 << 24) | (0x1 << 25));
#if LPDDR4_RETRAIN
	retrain_general_config();
#endif

#if LPDDR4_DFS_CONFIG
	reg_mask_set(D_DDRMC_TMG14_F1_ADDR, 0, (1 << 25) | (1 << 24));
	ddrmc_dfs_freq1_timing_init();
#endif
}

#endif

