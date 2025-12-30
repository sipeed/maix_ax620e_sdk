#include "cmn.h"
#include "trace.h"
#include "timer.h"
#include "ddrmc_reg_addr_def.h"
#include "ddrmc_train_reg_addr_def.h"
#include "ddrphy_reg_addr_def.h"
#include "ddr_init_config.h"
#include "dpll_ctrl_glb.h"
#include "boot_mode.h"

#define DDR_TYPE_FLAG     0x3000
extern void ddr_writel(u32 addr, u32 value);
extern void ddrphy_dfs_writel(u32 addr, u32 value, u32 dfs_index);
extern u32 ddrphy_dfs_readl(unsigned long addr, u32 dfs_index);
extern void ddrmc_mrw(u8 mr_cs, u8 mr_ad, u16 mr_op);
extern void ddrmc_mrr(u8 mr_cs, u16 mr_ad, u16 *mr_op);
extern void reg_mask_set(u32 addr, u32 mask_data, u32 bits_set);
extern u16 caeye_train_flow(void);
extern void cadsk_train_flow(void);
extern void rddsk_train_flow(void);
extern u32 rdeye_train_flow(void);
extern void wrdsk_train_flow(void);
extern u32 wreye_train_flow(void);
extern void retrain_base_value_f0(void);
extern void ddrmc_dfs_writel(u32 addr, u32 value, u32 dfs_index);
extern u32 ddr_types;
extern void reg_mask_set(u32 addr, u32 mask_data, u32 bits_set);
extern void ddrmc_sys_glb_init(DDR_FREQ freq);
void ddr_dfs_reg_mask_set(u32 addr, u32 mask_data, u32 bits_set, u32 dfs_index);
extern u32 ddr_freqs;
extern u16 cur_pass_window_caeye;
extern u32 r_pass_window_dqeye;
extern u32 w_pass_window_dqeye;
#if (0 == LPDDR4_DFS_CONFIG)
extern u8 rdeye_pass;
extern u8 wreye_pass;
#endif
extern u32 ddr4_mr6_data_save;
extern u32 chip_id;
extern u8 dfs_freq;
extern struct ddr_dfs_vref_t * p_iram_dfs_vref_info;

#define CA_BYTE_SEL_MAX (2)
#define DQ_BYTE_SEL_MAX (4)
struct training_window_t {
	u8 vref;
	u32 width;
	u32 center;
#ifdef DDR_OFFLINE_SCAN
	u8 dly_start[DQ_BYTE_SEL_MAX];
	u8 dly_end[DQ_BYTE_SEL_MAX];
#endif
};

#define CA_SCAN_MAX     (128)
#define RD_SCAN_MAX     (128)
#define WR_SCAN_MAX     (128)

struct training_window_t s_ca_scan_vref[RANK_MAX][CA_SCAN_MAX] = {0};
struct training_window_t s_read_scan_vref[RANK_MAX][RD_SCAN_MAX] = {0};
struct training_window_t s_write_scan_vref[RANK_MAX][WR_SCAN_MAX] = {0};
u8 g_ca_scan_max_index[RANK_MAX][CA_BYTE_SEL_MAX] = {0};
u8 g_read_scan_max_index[RANK_MAX][DQ_BYTE_SEL_MAX] = {0};
u8 g_write_scan_max_index[RANK_MAX][DQ_BYTE_SEL_MAX] = {0};
u8 g_rf_io_vrefi_se_adj = 22;
u8 ddr4_rf_io_vrefi_se_adj = 96;
u8 g_DRAM_VREF_CA[RANK_MAX] = {0};	//MR12
u8 g_DRAM_VREF_DQ[RANK_MAX] = {0};	//MR14

static int lp4_vref_mrval_to_level(u8 mrval)
{
	if (((mrval > 0x32) && (mrval < 0x40)) || (mrval > 0x72))
		return -1;
	return ((mrval >= 0x40) ? (mrval - 0x22) : mrval);
}

#define LPDDR4_VREF_PRIORITY_RANGE1
static u8 lp4_vref_level_to_mrval(int level)
{
	if (level > 0x80)
		return -1;
#ifdef LPDDR4_VREF_PRIORITY_RANGE1
	return ((level > 0x1d) ? (level + 0x22) : level);
#else
	return level;
#endif
}

#ifdef AX_DDR_DEBUG
static void dq_per_bit_delay_value_setting_print(void)
{
	u8 chan;
	u32 reg_data = 0;

	for (chan = 0; chan < 2; chan++) {
		if (0 == chan)
			ax_print_str("\r\npha DDRPHY_DS0_CFG6 = 0x");
		else
			ax_print_str("\r\nphb DDRPHY_DS0_CFG6 = 0x");

		//rank0 output delay mannual setting
		reg_data = readl(D_DDRPHY_DS0_CFG6_ADDR + chan * DDR_PHY_OFFSET);//dq3  dq2  dq1 dq0
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS0_CFG7_ADDR + chan * DDR_PHY_OFFSET);//dq7  dq6  dq5 dq4
		ax_print_str(", DDRPHY_DS0_CFG7 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS0_CFG8_ADDR + chan * DDR_PHY_OFFSET);//dqm
		ax_print_str(", DDRPHY_DS0_CFG8 = 0x");
		ax_print_num(reg_data, 16);

		//rank1 output delay mannual setting
		reg_data = readl(D_DDRPHY_DS0_CFG9_ADDR + chan * DDR_PHY_OFFSET);//dq3  dq2  dq1 dq0
		ax_print_str(", DDRPHY_DS0_CFG9 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS0_CFG10_ADDR + chan * DDR_PHY_OFFSET);//dq7  dq6  dq5 dq4
		ax_print_str(", DDRPHY_DS0_CFG10 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS0_CFG11_ADDR + chan * DDR_PHY_OFFSET);//dqm
		ax_print_str(", DDRPHY_DS0_CFG11 = 0x");
		ax_print_num(reg_data, 16);

		//rank0 output delay mannual setting
		reg_data = readl(D_DDRPHY_DS1_CFG6_ADDR + chan * DDR_PHY_OFFSET);//dq3  dq2  dq1 dq0
		ax_print_str(", DDRPHY_DS1_CFG6 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS1_CFG7_ADDR + chan * DDR_PHY_OFFSET);//dq7  dq6  dq5 dq4
		ax_print_str(", DDRPHY_DS1_CFG7 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS1_CFG8_ADDR + chan * DDR_PHY_OFFSET);//dqm
		ax_print_str(", DDRPHY_DS1_CFG8 = 0x");
		ax_print_num(reg_data, 16);

		//rank1 output delay mannual setting
		reg_data = readl(D_DDRPHY_DS1_CFG9_ADDR + chan * DDR_PHY_OFFSET);//dq3  dq2  dq1 dq0
		ax_print_str(", DDRPHY_DS1_CFG9 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS1_CFG10_ADDR + chan * DDR_PHY_OFFSET);//dq7  dq6  dq5 dq4
		ax_print_str(", DDRPHY_DS1_CFG10 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS1_CFG11_ADDR + chan * DDR_PHY_OFFSET);//dqm
		ax_print_str(", DDRPHY_DS1_CFG11 = 0x");
		ax_print_num(reg_data, 16);

		//rank0 input delay mannual setting
		reg_data = readl(D_DDRPHY_DS0_CFG12_ADDR + chan * DDR_PHY_OFFSET);//dq3  dq2  dq1 dq0
		ax_print_str(", DDRPHY_DS0_CFG12 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS0_CFG13_ADDR + chan * DDR_PHY_OFFSET);//dq7  dq6  dq5 dq4
		ax_print_str(", DDRPHY_DS0_CFG13 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS0_CFG14_ADDR + chan * DDR_PHY_OFFSET);//dqm
		ax_print_str(", DDRPHY_DS0_CFG14 = 0x");
		ax_print_num(reg_data, 16);

		//rank1 input delay mannual setting
		reg_data = readl(D_DDRPHY_DS0_CFG15_ADDR + chan * DDR_PHY_OFFSET);//dq3  dq2  dq1 dq0
		ax_print_str(", DDRPHY_DS0_CFG15 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS0_CFG16_ADDR + chan * DDR_PHY_OFFSET);//dq7  dq6  dq5 dq4
		ax_print_str(", DDRPHY_DS0_CFG16 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS0_CFG17_ADDR + chan * DDR_PHY_OFFSET);//dqm
		ax_print_str(", DDRPHY_DS0_CFG17 = 0x");
		ax_print_num(reg_data, 16);

		//rank0 input delay mannual setting
		reg_data = readl(D_DDRPHY_DS1_CFG12_ADDR + chan * DDR_PHY_OFFSET);//dq3  dq2  dq1 dq0
		ax_print_str(", DDRPHY_DS1_CFG12 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS1_CFG13_ADDR + chan * DDR_PHY_OFFSET);//dq7  dq6  dq5 dq4
		ax_print_str(", DDRPHY_DS1_CFG13 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS1_CFG14_ADDR + chan * DDR_PHY_OFFSET);//dqm
		ax_print_str(", DDRPHY_DS1_CFG14 = 0x");
		ax_print_num(reg_data, 16);

		//rank1 input delay mannual setting
		reg_data = readl(D_DDRPHY_DS1_CFG15_ADDR + chan * DDR_PHY_OFFSET);//dq3  dq2  dq1 dq0
		ax_print_str(", DDRPHY_DS1_CFG15 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS1_CFG16_ADDR + chan * DDR_PHY_OFFSET);//dq7  dq6  dq5 dq4
		ax_print_str(", DDRPHY_DS1_CFG16 = 0x");
		ax_print_num(reg_data, 16);

		reg_data = readl(D_DDRPHY_DS1_CFG17_ADDR + chan * DDR_PHY_OFFSET);//dqm
		ax_print_str(", DDRPHY_DS1_CFG17 = 0x");
		ax_print_num(reg_data, 16);
	}
	ax_print_str("\r\n");
}
#endif

static u16 ca_vref_training(DDR_TYPE ddr_type, u8 cs, u8 vref, u8 deskew_flag)
{
	u32 reg_data;
	u32 reg_temp;
	u32 byte_sel;
	u8 byte_sel_max;
	u16 cur_pass_window_temp = 0;
	u8 max_byte_width, max_byte_vref;

	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		byte_sel_max = 2;
	} else {
		byte_sel_max = 1;
	}
	if ((TYPE_LPDDR4 & ddr_type) != TYPE_LPDDR4)
		return 0;

	ax_print_str("\r\nca_vref=0x");
	ax_print_num(vref, 16);
	reg_mask_set(D_DDRMC_TRAIN_CTRL6_ADDR, 0x7f000, vref << 12);//rf_train_caeye_vref

	for(byte_sel = 0; byte_sel < byte_sel_max; byte_sel++) {
		reg_data = readl(D_DDRMC_TRAIN_CTRL7_ADDR);
		if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
			reg_data &= ~0xffff;
			reg_data |= (byte_sel == 0) ? 0x0 : 0x2;
			reg_data |= (byte_sel == 0) ? 0x1 << 4 : 0x3 << 4;
			reg_data |= (byte_sel == 0) ? 0x2 << 8 : 0x0 << 8;
			reg_data |= (byte_sel == 0) ? 0x3 << 12 : 0x1 << 12;
		} else {
			reg_data &= ~0xff;
			reg_data |= 0x0 << 4;//rf_train_2byte_swap
			reg_data |= 0x3 << 0;//rf_train_2byte_swap
		}
		ddr_writel(D_DDRMC_TRAIN_CTRL7_ADDR, reg_data);

		cur_pass_window_temp = caeye_train_flow();

		if (deskew_flag) {
			reg_data = readl(D_DDRMC_TRAIN_CTRL3_ADDR);
			reg_data &= ~0xff000;//[19:12]
			reg_temp = cur_pass_window_temp << 12;
			reg_temp &= 0xff000;//[19:12]
			reg_data |= reg_temp;//rf_train_eye_base_value
			ddr_writel(D_DDRMC_TRAIN_CTRL3_ADDR, reg_data);

			cadsk_train_flow();

			reg_mask_set(D_DDRMC_TRAIN_CTRL3_ADDR, 0xff000, 0);//rf_train_eye_base_value
			cur_pass_window_temp = caeye_train_flow(); // the centering position of CA EYE
		}
		//ca_train_check(cs_num, byte_sel, &delay_cfg);
		ax_print_str(", center=0x");
		ax_print_num(cur_pass_window_temp, 16);
		ax_print_str(", width=0x");
		ax_print_num(cur_pass_window_caeye, 16);
		max_byte_vref = g_ca_scan_max_index[cs][byte_sel];
		if (0 == byte_sel) {
			s_ca_scan_vref[cs][vref].width &= ~0x7ff;
			s_ca_scan_vref[cs][vref].width |= (cur_pass_window_caeye & 0x7ff);
			max_byte_width = (s_ca_scan_vref[cs][max_byte_vref].width & 0x7ff);
		}
		else {
			s_ca_scan_vref[cs][vref].width &= ~(0x7ff << 16);
			s_ca_scan_vref[cs][vref].width |= ((cur_pass_window_caeye & 0x7ff) << 16);
			max_byte_width = ((s_ca_scan_vref[cs][max_byte_vref].width >> 16) & 0x7ff);
		}
		if (cur_pass_window_caeye > max_byte_width) {
			g_ca_scan_max_index[cs][byte_sel] = vref;
		}
#ifdef DDR_OFFLINE_SCAN
		if (0 == byte_sel) {
			s_ca_scan_vref[cs][vref].center &= ~0x7ff;
			//s_ca_scan_vref[cs][vref].width &= ~0x7ff;
			s_ca_scan_vref[cs][vref].center |= (cur_pass_window_temp & 0x7ff);
			//s_ca_scan_vref[cs][vref].width |= (cur_pass_window_caeye & 0x7ff);
		}
		else {
			s_ca_scan_vref[cs][vref].center &= ~(0x7ff << 16);
			//s_ca_scan_vref[cs][vref].width &= ~(0x7ff << 16);
			s_ca_scan_vref[cs][vref].center |= ((cur_pass_window_temp & 0x7ff) << 16);
			//s_ca_scan_vref[cs][vref].width |= ((cur_pass_window_caeye & 0x7ff) << 16);
		}
#endif
	}//byte_sel

	return cur_pass_window_temp;
}

u32 ddr3_ddr4_rdeye_train_flow(u8 cs_num)
{
	u32 reg_data;
	u32 bit_shift = 0;
	u32 rdeye_train_temp = 0;
	r_pass_window_dqeye = 0;

	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		bit_shift = 0x3 << 24;//rf_train_byte_en
	} else {
		bit_shift = 0xf << 24;
	}
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100087, (cs_num << 20) | (1 << 8) | (1 << 2) | (1) | bit_shift);

	//polling train done
	for (int i=0; i<1000 ; i++) {
		//#1us;
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (0x1 << 16)) { //rf_train_rdeye_done
			if(reg_data & (0x1 << 17)) { //rf_train_rdeye_fail
				ax_print_str(", read eye training fail !\r\n");
			} else {
				rdeye_train_temp = readl(D_DDRMC_TRAIN_RES2_ADDR);// eye center
				r_pass_window_dqeye = readl(D_DDRMC_TRAIN_RES3_ADDR); //pass_window
				ax_print_str(", width=0x");
				ax_print_num(r_pass_window_dqeye, 16);
				ax_print_str(", center=0x");
				ax_print_num(rdeye_train_temp, 16);
				#if (0 == LPDDR4_DFS_CONFIG)
				rdeye_pass = 1;
				#endif
			}
			break;
		}
		if(i==999) {
			ax_print_str("polling read eye training timeout !");
		}
	}

	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100107, (cs_num << 20) | (1 << 8) | (1 << 2) | (1 << 1) | bit_shift);
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100107, (cs_num << 20) | bit_shift);
	return rdeye_train_temp;
}

u32 ddr3_ddr4_wreye_train_flow(u8 cs_num)
{
	u32 reg_data;
	u32 bit_shift = 0;
	u32 wreye_train_temp = 0;
	w_pass_window_dqeye = 0;

	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X16) {
		bit_shift = 0x3 << 24;//rf_train_byte_en
	} else {
		bit_shift = 0xf << 24;
	}
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100407, (cs_num << 20) | (1 << 11) | (1 << 2) | (1) | bit_shift);

	//polling train done
	for (int i=0; i<1000 ; i++) {
		//#1us;
		reg_data = readl(D_DDRMC_TRAIN_RES0_ADDR);
		if(reg_data & (0x1 << 28)) {//rf_train_wreye_done
			if(reg_data & (0x1 << 29)) {//rf_train_wreye_fail
				ax_print_str("write eye training fail !\r\n");
			} else {
				wreye_train_temp = readl(D_DDRMC_TRAIN_RES2_ADDR);// eye center
				w_pass_window_dqeye = readl(D_DDRMC_TRAIN_RES3_ADDR); //pass_window
				ax_print_str(", width=0x");
				ax_print_num(w_pass_window_dqeye, 16);
				ax_print_str(", center=0x");
				ax_print_num(wreye_train_temp, 16);
				#if (0 == LPDDR4_DFS_CONFIG)
				wreye_pass = 1;
				#endif
			}
			break;
		}
		if(i==999) {
			ax_print_str("polling write eye training done timeout !\r\n");
		}
	}

#ifdef AX630C_DDR4_RETRAIN
	if (ddr_types & TYPE_DDR4) {
		retrain_base_value_f0();
	}
#endif

	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100807, (cs_num << 20) | (1 << 11) | (1 << 2) | (1 << 1) | bit_shift);
	reg_mask_set(D_DDRMC_TRAIN_CTRL0_ADDR, 0xFF100807, (cs_num << 20) | bit_shift);
	udelay(1);
	return wreye_train_temp;
}

static void poll_dfi_init_completed(void)
{
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nDFI reg 0x");
	ax_print_num(D_DDRMC_CFG0_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRMC_CFG0_ADDR), 16);
#endif

	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf, 0x3);//rf_dfi_init_start
	while((readl(D_DDRMC_CFG0_ADDR) >> 8) & 0x1);
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nDFI reg 0x");
	ax_print_num(D_DDRMC_CFG0_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRMC_CFG0_ADDR), 16);
#endif

	reg_mask_set(D_DDRMC_CFG0_ADDR, 0xf, 0);
	while(!((readl(D_DDRMC_CFG0_ADDR) >> 8) & 0x1));
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nDFI reg 0x");
	ax_print_num(D_DDRMC_CFG0_ADDR, 16);
	ax_print_str(", val 0x");
	ax_print_num(readl(D_DDRMC_CFG0_ADDR), 16);
#endif
}

static void ADDR_delay_value_setting(void)
{
	ddr_dfs_reg_mask_set(D_DDRPHY_AC_TMG1_F0_ADDR, 0x7ff, 0, dfs_freq);//rf_clkwr_addr_dl_sel_ac_f0
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		ddr_dfs_reg_mask_set(D_DDRPHY_AC_TMG1_F0_ADDR + DDR_PHY_OFFSET, 0x7ff, 0, dfs_freq);//rf_clkwr_addr_dl_sel_ac_f0
	}
#ifdef AX_DDR_DEBUG
	ax_print_str("\r\nD_DDRPHY_AC_TMG1_F0_ADDR = 0x");
	ax_print_num(readl(D_DDRPHY_AC_TMG1_F0_ADDR), 16);
	ax_print_str("\r\nD_DDRPHY_AC_TMG1_F0_ADDR + DDR_PHY_OFFSET = 0x");
	ax_print_num(readl(D_DDRPHY_AC_TMG1_F0_ADDR + DDR_PHY_OFFSET), 16);
	ax_print_str("\r\nD_DDRPHY_AC_TMG2_F0_ADDR = 0x");
	ax_print_num(readl(D_DDRPHY_AC_TMG2_F0_ADDR), 16);
	ax_print_str("\r\nD_DDRPHY_AC_TMG2_F0_ADDR + DDR_PHY_OFFSET = 0x");
	ax_print_num(readl(D_DDRPHY_AC_TMG2_F0_ADDR + DDR_PHY_OFFSET), 16);
#endif
}

void ca_vref_scan(struct ddr_dfs_vref_t * param, DDR_TYPE ddr_type, u8 cs, u8 vref_start, u8 vref_end, u8 interval)
{
#if defined(DDR_ENV_EDA)
	u8 ca_deskew_done = 0;
#else
	u8 ca_deskew_done = 1;
#endif
	int level0, level1;

	if ((TYPE_LPDDR4 & ddr_type) != TYPE_LPDDR4)
		return;

	if (cs && (AX620Q_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id) && (ddr_freqs >= DDR_CLK_3200)) {
		ddrmc_sys_glb_init(DDR_CLK_2800);
		poll_dfi_init_completed();
	}

	if (param) {
		ddrmc_mrw((0x2 | cs), 0xc, param->dram_VREF_CA[cs]); //MR12
	#if LPDDR4_DFS_CONFIG
		ddr_dfs_reg_mask_set(D_DDRMC_TMG26_F0_ADDR, 0xffff, (0xc << 8) | g_DRAM_VREF_CA[cs], dfs_freq);
	#endif
		ca_vref_training(ddr_type, cs, param->dram_VREF_CA[cs], !ca_deskew_done);
		if (cur_pass_window_caeye < 0x60)
			ADDR_delay_value_setting();
		return;
	}

	for (u8 ca_scan = vref_start; ca_scan <= vref_end; ca_scan+=interval) {
		if ((ca_scan > 0x32) && (ca_scan < 0x40)) // Range[0] completed
			ca_scan = 0x40;
		else if (ca_scan > 0x72)                  // Range[1] completed
			break;

		ca_vref_training(ddr_type, cs, ca_scan, !ca_deskew_done);
		ca_deskew_done = 1;
		s_ca_scan_vref[cs][ca_scan].vref = ca_scan;
#if 0
		g_ca_scan_vref[ca_scan][0] = cur_pass_window_caeye;  // the width of CA EYE
		if (g_ca_scan_vref[ca_scan][0] > g_ca_scan_vref[scan_max_index][0]) {
			scan_max_index = ca_scan;
		}
#endif
	}
	//write Vref back
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		level0 = lp4_vref_mrval_to_level(g_ca_scan_max_index[cs][0]);
		level1 = lp4_vref_mrval_to_level(g_ca_scan_max_index[cs][1]);
		if (((level0 >= 0) && (level0 <= 80)) &&
			((level1 >= 0) && (level1 <= 80)))
			g_DRAM_VREF_CA[cs] = lp4_vref_level_to_mrval((level0 + level1) / 2);
		else
			g_DRAM_VREF_CA[cs] = g_ca_scan_max_index[cs][0];
	} else {
		g_DRAM_VREF_CA[cs] = g_ca_scan_max_index[cs][0];
	}
	ddrmc_mrw((0x2 | cs), 0xc, g_DRAM_VREF_CA[cs]); //MR12
	#if LPDDR4_DFS_CONFIG
		ddr_dfs_reg_mask_set(D_DDRMC_TMG26_F0_ADDR, 0xffff, (0xc << 8) | g_DRAM_VREF_CA[cs], dfs_freq);
	#endif
	ca_vref_training(ddr_type, cs, g_DRAM_VREF_CA[cs], !ca_deskew_done);
	if (cur_pass_window_caeye < 0x60)
		ADDR_delay_value_setting();
	p_iram_dfs_vref_info->dram_VREF_CA[cs] = g_DRAM_VREF_CA[cs];
	ax_print_str("\r\n========== rank ");
	ax_print_num(cs, 10);
	ax_print_str(": dram_VREF_CA = 0x");
	ax_print_num(p_iram_dfs_vref_info->dram_VREF_CA[cs], 16);
	ax_print_str("==========\n");
}

void rd_vref_scan(struct ddr_dfs_vref_t * param, DDR_TYPE ddr_type, u8 cs, u8 vref_start, u8 vref_end, u8 interval)
{
	u32 reg_data;
	u8 byte_sel, byte_width, max_byte_width, max_byte_vref;
#if defined(DDR_ENV_EDA)
	u8 rd_deskew_done = 0;
#else
	u8 rd_deskew_done = 0;
#endif

	if ((TYPE_LPDDR4 & ddr_type) != TYPE_LPDDR4) {
		g_rf_io_vrefi_se_adj = ddr4_rf_io_vrefi_se_adj;
	}

	for(u16 rd_scan = vref_start; rd_scan <= vref_end; rd_scan+=interval) {
		if (TYPE_LPDDR4 == ddr_type) {
			if ((AX620Q_CHIP_E == chip_id) || (AX620QX_CHIP_E == chip_id) || (AX620QZ_CHIP_E == chip_id) || (AX620QP_CHIP_E == chip_id))
				rd_deskew_done = 1;
			if (!rd_deskew_done) {
				rd_deskew_done = 1;

				reg_data = ddrphy_dfs_readl(D_DDRPHY_GEN_TMG10_F0_ADDR, dfs_freq);
				reg_data &= ~0xffffff;
				reg_data |= ((0x13 << 16) | (g_rf_io_vrefi_se_adj << 8) | 0x13);
				ddrphy_dfs_writel(D_DDRPHY_GEN_TMG10_F0_ADDR, reg_data, dfs_freq);
				ax_print_str("\r\nrd_vrefi_A=0x");
				ax_print_num((reg_data & 0xff), 16);

				reg_data = ddrphy_dfs_readl(D_DDRPHY_GEN_TMG10_F0_ADDR + DDR_PHY_OFFSET, dfs_freq);
				reg_data &= ~0xffffff;
				reg_data |= ((0x13 << 16) | (g_rf_io_vrefi_se_adj << 8) | 0x13);
				ddrphy_dfs_writel(D_DDRPHY_GEN_TMG10_F0_ADDR + DDR_PHY_OFFSET, reg_data, dfs_freq);
				ax_print_str(", rd_vrefi_B=0x");
				ax_print_num((reg_data & 0xff), 16);
			#if LPDDR4_DFS_CONFIG
				if(dfs_freq == 0) {
					rddsk_train_flow();
				}
			#else
				rddsk_train_flow();
			#endif
			#ifdef AX_DDR_DEBUG
				dq_per_bit_delay_value_setting_print();
			#endif
				if ((AX620Q_CHIP_E != chip_id) && (AX620QZ_CHIP_E != chip_id) && (AX620QX_CHIP_E != chip_id) && (AX620QP_CHIP_E != chip_id) && (ddr_freqs >= DDR_CLK_3200)) {
					ddrmc_sys_glb_init(ddr_freqs);
					poll_dfi_init_completed();
				}
			}
		}
		reg_data = ddrphy_dfs_readl(D_DDRPHY_GEN_TMG10_F0_ADDR, dfs_freq);
		reg_data &= ~0xffffff;
		reg_data |= param ? param->rf_io_vrefi_adj_PHY_A : rd_scan;//rf_io_vrefi_adj_f0
		reg_data |= g_rf_io_vrefi_se_adj << 8;//rf_io_vrefi_se_adj_f0
		reg_data |= (param ? param->rf_io_vrefi_adj_PHY_A : rd_scan) << 16;//rf_io_vrefe_adj_f0
		ddrphy_dfs_writel(D_DDRPHY_GEN_TMG10_F0_ADDR, reg_data, dfs_freq);
		ax_print_str("\r\nrd_vrefi_A=0x");
		ax_print_num((reg_data & 0xff), 16);

		reg_data = ddrphy_dfs_readl(D_DDRPHY_GEN_TMG10_F0_ADDR + DDR_PHY_OFFSET, dfs_freq);
		reg_data &= ~0xffffff;
		reg_data |= param ? param->rf_io_vrefi_adj_PHY_B : rd_scan;//rf_io_vrefi_adj_f0
		reg_data |= g_rf_io_vrefi_se_adj << 8;//rf_io_vrefi_se_adj_f0
		reg_data |= (param ? param->rf_io_vrefi_adj_PHY_B : rd_scan) << 16;//rf_io_vrefe_adj_f0
		ddrphy_dfs_writel(D_DDRPHY_GEN_TMG10_F0_ADDR + DDR_PHY_OFFSET, reg_data, dfs_freq);

		ax_print_str(", rd_vrefi_B=0x");
		ax_print_num((reg_data & 0xff), 16);
		if (TYPE_LPDDR4 == ddr_type) {
			s_read_scan_vref[cs][rd_scan].center = rdeye_train_flow();
			ax_print_str(", center=0x");
			ax_print_num(s_read_scan_vref[cs][rd_scan].center, 16);
			ax_print_str(", width=0x");
			ax_print_num(r_pass_window_dqeye, 16);
		} else {
			s_read_scan_vref[cs][rd_scan].center = ddr3_ddr4_rdeye_train_flow(cs);
		}
		if (param)
			return;

		s_read_scan_vref[cs][rd_scan].width = r_pass_window_dqeye;
		s_read_scan_vref[cs][rd_scan].vref = rd_scan;
		for (byte_sel = 0; byte_sel < DQ_BYTE_SEL_MAX; byte_sel++) {
			byte_width = (s_read_scan_vref[cs][rd_scan].width >> (8 * byte_sel)) & 0xff;
			max_byte_vref = g_read_scan_max_index[cs][byte_sel];
			max_byte_width = (s_read_scan_vref[cs][max_byte_vref].width >> (8 * byte_sel)) & 0xff;
			if (byte_width > max_byte_width)
				g_read_scan_max_index[cs][byte_sel] = rd_scan;
		}
	}
	//read training write Vref back
	reg_data = ddrphy_dfs_readl(D_DDRPHY_GEN_TMG10_F0_ADDR, dfs_freq);
	reg_data &= ~0xffffff;
	reg_data |= g_read_scan_max_index[cs][0];//rf_io_vrefi_adj_f0
	reg_data |= g_rf_io_vrefi_se_adj << 8;//rf_io_vrefi_se_adj_f0
	reg_data |= g_read_scan_max_index[cs][0] << 16;//rf_io_vrefe_adj_f0
	ddrphy_dfs_writel(D_DDRPHY_GEN_TMG10_F0_ADDR, reg_data, dfs_freq);

	reg_data = ddrphy_dfs_readl(D_DDRPHY_GEN_TMG10_F0_ADDR + DDR_PHY_OFFSET, dfs_freq);
	reg_data &= ~0xffffff;
	reg_data |= g_read_scan_max_index[cs][3];//rf_io_vrefi_adj_f0
	reg_data |= g_rf_io_vrefi_se_adj << 8;//rf_io_vrefi_se_adj_f0
	reg_data |= g_read_scan_max_index[cs][3] << 16;//rf_io_vrefe_adj_f0
	ddrphy_dfs_writel(D_DDRPHY_GEN_TMG10_F0_ADDR + DDR_PHY_OFFSET, reg_data, dfs_freq);

	if (TYPE_LPDDR4 == ddr_type) {
		rdeye_train_flow();
	} else {
		ax_print_str("\r\nselected rd_vrefi_A=0x");
		ax_print_num(g_read_scan_max_index[cs][0], 16);
		ax_print_str(", rd_vrefi_B=0x");
		ax_print_num(g_read_scan_max_index[cs][3], 16);
		ddr3_ddr4_rdeye_train_flow(cs);
	}

	p_iram_dfs_vref_info->rf_io_vrefi_adj_PHY_A = g_read_scan_max_index[cs][0];
	p_iram_dfs_vref_info->rf_io_vrefi_adj_PHY_B = g_read_scan_max_index[cs][3];
	ax_print_str("\r\n========== rf_io_vrefi_adj_PHY_A/B = 0x");
	ax_print_num(p_iram_dfs_vref_info->rf_io_vrefi_adj_PHY_A, 16);
	ax_print_str(", 0x");
	ax_print_num(p_iram_dfs_vref_info->rf_io_vrefi_adj_PHY_B, 16);
	ax_print_str("==========\n");
}

void wr_vref_scan(struct ddr_dfs_vref_t * param, DDR_TYPE ddr_type, u8 cs, u8 vref_start, u8 vref_end, u8 interval)
{
	u8 byte_sel, byte_width, max_byte_width, max_byte_vref;
	u8 first_wreye_train = 1;
#if defined(DDR_ENV_EDA)
	u8 wr_deskew_done = 0;
#else
	u8 wr_deskew_done = 0;
#endif
	u32 ddr4_scan = 0;
	u32 cur_pass_window_temp = 0;
	int level0, level3;
	int level1, level2;
#ifdef AX_DDR_DEBUG
	u16 mr_op;
#endif

	for (u8 wr_scan = vref_start; wr_scan <= vref_end; wr_scan+=interval) {
		if ((wr_scan > 0x32) && (wr_scan < 0x40))	// Range[0] completed
			wr_scan = 0x40;
		else if (wr_scan > 0x72)					// Range[1] completed
			break;

		if (TYPE_LPDDR4 == ddr_type) {
#ifdef AX_DDR_DEBUG
			ddrmc_mrr(cs, 0xe, &mr_op);
#endif
			if (first_wreye_train/* && ((AX620Q_CHIP_E == chip_id) || (AX620QZ_CHIP_E == chip_id) || (AX620QX_CHIP_E == chip_id) || (AX620QP_CHIP_E == chip_id))*/) {
				wreye_train_flow();
				first_wreye_train = 0;
			}
			if (!wr_deskew_done) {
				wr_deskew_done = 1;
			#if LPDDR4_DFS_CONFIG
				if (dfs_freq == 0) {
					wrdsk_train_flow();
				}
			#else
				wrdsk_train_flow();
			#endif
#ifdef AX_DDR_DEBUG
				dq_per_bit_delay_value_setting_print();
#endif
			}
			ddrmc_mrw((0x2 | cs), 0xe, param ? param->dram_VREF_DQ[cs] : wr_scan);//MR14
		#if LPDDR4_DFS_CONFIG
			ddr_dfs_reg_mask_set(D_DDRMC_TMG26_F0_ADDR, 0xffff << 16, (0xe << 24) | (g_DRAM_VREF_DQ[cs] << 16), dfs_freq);
		#endif
#ifdef AX_DDR_DEBUG
			ddrmc_mrr(cs, 0xe, &mr_op);
#endif
			ax_print_str("\r\ndq_vref=0x");
			ax_print_num(param ? param->dram_VREF_DQ[cs] : wr_scan, 16);
			cur_pass_window_temp = wreye_train_flow();
			ax_print_str(", center=0x");
			ax_print_num(cur_pass_window_temp, 16);
			ax_print_str(", width=0x");
			ax_print_num(w_pass_window_dqeye, 16);
			//wr_train_check(cs_num, &delay_cfg, tdqs2dq_set);
		} else {
			ddr4_scan = (param ? param->dram_VREF_DQ[cs] : wr_scan) | (1 << 7) | ddr4_mr6_data_save;
			ax_print_str("\r\nwr_vref=0x");
			ax_print_num(ddr4_scan - 0xc80, 16);
			ddrmc_mrw((0x2 | cs), 0x6, ddr4_scan);//MR6
			udelay(1);
			cur_pass_window_temp = ddr3_ddr4_wreye_train_flow(cs);
		}
		if (param)
			return;
		s_write_scan_vref[cs][wr_scan].center = cur_pass_window_temp;
		s_write_scan_vref[cs][wr_scan].width = w_pass_window_dqeye;
		s_write_scan_vref[cs][wr_scan].vref = wr_scan;
		for (byte_sel = 0; byte_sel < DQ_BYTE_SEL_MAX; byte_sel++) {
			byte_width = (s_write_scan_vref[cs][wr_scan].width >> (8 * byte_sel)) & 0xff;
			max_byte_vref = g_write_scan_max_index[cs][byte_sel];
			max_byte_width = (s_write_scan_vref[cs][max_byte_vref].width >> (8 * byte_sel)) & 0xff;
			if (byte_width > max_byte_width)
				g_write_scan_max_index[cs][byte_sel] = wr_scan;
		}
	}
	if (TYPE_LPDDR4 == ddr_type) {
		level0 = lp4_vref_mrval_to_level(g_write_scan_max_index[cs][0]);
		level3 = lp4_vref_mrval_to_level(g_write_scan_max_index[cs][3]);
	#ifdef AX_DDR_DEBUG
		ax_print_str("\r\nbyte0_max_vref: 0x");
		ax_print_num(g_write_scan_max_index[cs][0], 16);
		ax_print_str("\r\nbyte3_max_vref: 0x");
		ax_print_num(g_write_scan_max_index[cs][3], 16);
	#endif
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		level2 = lp4_vref_mrval_to_level(g_write_scan_max_index[cs][2]);
		level1 = lp4_vref_mrval_to_level(g_write_scan_max_index[cs][1]);
		if (((level0 >= 0) && (level0 <= 80)) &&
			((level1 >= 0) && (level1 <= 80)) &&
			((level2 >= 0) && (level2 <= 80)) &&
			((level3 >= 0) && (level3 <= 80)))
			g_DRAM_VREF_DQ[cs] = lp4_vref_level_to_mrval((level0 + level1 + level2 + level3) / 4);
		else
			g_DRAM_VREF_DQ[cs] = g_write_scan_max_index[cs][0];
	} else {
		if (((level0 >= 0) && (level0 <= 80)) &&
			((level3 >= 0) && (level3 <= 80)))
			g_DRAM_VREF_DQ[cs] = lp4_vref_level_to_mrval((level0 + level3) / 2);
		else
			g_DRAM_VREF_DQ[cs] = g_write_scan_max_index[cs][0];
	}
		ddrmc_mrw((0x2 | cs), 0xe, g_DRAM_VREF_DQ[cs]);//MR14
	#if LPDDR4_DFS_CONFIG
		ddr_dfs_reg_mask_set(D_DDRMC_TMG26_F0_ADDR, 0xffff << 16, (0xe << 24) | (g_DRAM_VREF_DQ[cs] << 16), dfs_freq);
	#endif
		wreye_train_flow();
		p_iram_dfs_vref_info->dram_VREF_DQ[cs] = g_DRAM_VREF_DQ[cs];
	} else {
		ddr4_scan = g_write_scan_max_index[cs][0] | (1 << 7) | ddr4_mr6_data_save;//VrefDQ Training Enable
		ax_print_str("\r\nselected wr_vref=0x");
		ax_print_num(g_write_scan_max_index[cs][0], 16);
		ddrmc_mrw((0x2 | cs), 0x6, ddr4_scan);//MR6
		ddr3_ddr4_wreye_train_flow(cs);
		p_iram_dfs_vref_info->dram_VREF_DQ[cs] = g_write_scan_max_index[cs][0];
		ddr4_scan &= ~(1 << 7); //VrefDQ Training Disable
		ddrmc_mrw((0x2 | cs), 0x6, ddr4_scan);//MR6
	}
	ax_print_str("\r\n========== rank ");
	ax_print_num(cs, 10);
	ax_print_str(": dram_VREF_DQ = 0x");
	ax_print_num(p_iram_dfs_vref_info->dram_VREF_DQ[cs], 16);
	ax_print_str("==========\n");
}

#ifdef DDR_OFFLINE_SCAN
//#define HAPS_DEBUG
#define TRAINIG_DLY_MAX (200)
#define STR_ERROR  "."
#define STR_OK     "*"
#define STR_CENTER "o"
#define STR_RANK0  "\r\n\r\nRANK_0:"
#define STR_RANK1  "\r\n\r\nRANK_1:"
#define STR_CA_A   "\r\n\r\nCA_A:"
#define STR_CA_B   "\r\n\r\nCA_B:"

static int calc_delay_line_range(int signal_type, struct training_window_t * result)
{
	int ret = 0;
	int bit_shift = 8;
	u32 bit_mask = 0xff;
	u16 width, center;
	u8 i;
	u8 byte_max = DQ_BYTE_SEL_MAX;

	if (0 == signal_type) { // CA
		bit_mask = 0x7ff;
		bit_shift = 16;
		if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
			byte_max = 2;
		} else {
			byte_max = 1;
		}
	}
	for (i = 0; i < byte_max; i++) {
		width = ((result->width >> (i * bit_shift)) & bit_mask);
		center = ((result->center >> (i * bit_shift)) & bit_mask);
		if (width && center) {
			result->dly_start[i] = (center >= width / 2) ? (center - width / 2) : 0;
			result->dly_end[i] = result->dly_start[i] + width - 1;
		}
	}

	return ret;
}

static int ca_training_window_calc(DDR_TYPE ddr_type, u8 cs)
{
	if ((TYPE_LPDDR4 & ddr_type) != TYPE_LPDDR4)
		return -1;

	for (u8 ca_scan = 0; ca_scan < CA_SCAN_MAX; ca_scan++) {
		if ((ca_scan > 0x32) && (ca_scan < 0x40)) // Range[0] completed
			ca_scan = 0x40;
		else if (ca_scan > 0x72)                  // Range[1] completed
			break;

		if (calc_delay_line_range(0, &s_ca_scan_vref[cs][ca_scan])) {
			ax_print_str("\r\nca window calc fail\n");
			return -1;
		}
	}

	return 0;
}

static int read_training_window_calc(DDR_TYPE ddr_type, u8 cs)
{
	if (((TYPE_LPDDR4 & ddr_type) != TYPE_LPDDR4) && ((TYPE_DDR4 & ddr_type) != TYPE_DDR4))
		return -1;

	for(u16 rd_scan = 0; rd_scan < RD_SCAN_MAX; rd_scan++) {
		if (calc_delay_line_range(1, &s_read_scan_vref[cs][rd_scan])) {
			ax_print_str("\r\nread window calc fail\n");
			return -1;
		}
	}

	return 0;
}

static int write_training_window_calc(DDR_TYPE ddr_type, u8 cs)
{
	if (((TYPE_LPDDR4 & ddr_type) != TYPE_LPDDR4) && ((TYPE_DDR4 & ddr_type) != TYPE_DDR4))
		return -1;

	for (u8 wr_scan = 0; wr_scan < WR_SCAN_MAX; wr_scan++) {
		if ((wr_scan > 0x32) && (wr_scan < 0x40))	// Range[0] completed
			wr_scan = 0x40;
		else if (wr_scan > 0x72)					// Range[1] completed
			break;

		if (calc_delay_line_range(2, &s_write_scan_vref[cs][wr_scan])) {
			ax_print_str("\r\nread window calc fail\n");
			return -1;
		}
	}

	return 0;
}

static int ddr_scan_result_output(struct training_window_t *item, u8 signal_type, u8 byte_sel)
{
	int ret = 0;
	u16 row, col, scan_max, center, start, end;
	u16 width;

	switch (signal_type) {
	case 0:
		scan_max = CA_SCAN_MAX;
		break;

	case 1:
		scan_max = RD_SCAN_MAX;
		break;

	case 2:
		scan_max = WR_SCAN_MAX;
		break;

	default:
		ret = -1;
		break;
	}
	if (ret < 0)
		return ret;

	for (row = 0; row < scan_max; row++,item++) {
		if (1 != signal_type) {
			if ((row > 0x32) && (row < 0x40)) // Range[0] completed
				continue;
			else if (row > 0x72)                  // Range[1] completed
				break;
		}

		ax_print_str("\r\nVREF=0x");
		if (row < 0x10)
			ax_print_str("0");
		ax_print_num(item->vref, 16);

		if (0 == signal_type) {
			start = item->dly_start[byte_sel];
			end = item->dly_end[byte_sel];
			center = (item->center >> (byte_sel * 16)) & 0x7ff;
			width = (item->width >> (byte_sel * 16)) & 0x7ff;
#ifdef AX_DDR_DEBUG
			ax_print_str(" width=0x");
			ax_print_num(width, 16);
			ax_print_str(" center=0x");
			ax_print_num(center, 16);
			ax_print_str(" start=0x");
			ax_print_num(start, 16);
			ax_print_str(" end=0x");
			ax_print_num(end, 16);
#endif
		}
		else {
			start = item->dly_start[byte_sel] & 0xff;
			end = item->dly_end[byte_sel] & 0xff;
			center = (item->center >> (byte_sel * 8)) & 0xff;
			width = (item->width >> (byte_sel * 8)) & 0xff;
#ifdef AX_DDR_DEBUG
			ax_print_str(" width=0x");
			ax_print_num(width, 16);
			ax_print_str(" center=0x");
			ax_print_num(center, 16);
			ax_print_str(" start=0x");
			ax_print_num(start, 16);
			ax_print_str(" end=0x");
			ax_print_num(end, 16);
#endif
		}
		ax_print_str(": ");
		for (col = 0; col < start; col++) {
			ax_print_str(STR_ERROR);
		}
		for (col = start; col <= end; col++) {
			if (0 == width)
				ax_print_str(STR_ERROR);
			else if (center == col)
				ax_print_str(STR_CENTER);
			else
				ax_print_str(STR_OK);
		}
		for (col = end + 1; col < TRAINIG_DLY_MAX; col++) {
			ax_print_str(STR_ERROR);
		}
	}

	return ret;
}

static void ddr_type_print(void)
{
	switch (ddr_types & DDR_TYPE_FLAG) {
	case TYPE_LPDDR4:
		ax_print_str("\r\nLPDDR4,");
		break;

	case TYPE_DDR4:
		ax_print_str("\r\nDDR4,");
		break;

	default:
		ax_print_str("\r\nDDR type wrong!");
		break;
	}
}

static void dram_mrs_print(u8 cs)
{
	u16 mr_op = 0;

	switch (ddr_types & DDR_TYPE_FLAG) {
	case TYPE_LPDDR4:
		ax_print_str("\r\n");
#ifndef HAPS_DEBUG
		ddrmc_mrr(cs, 12, &mr_op);
#endif
		ax_print_str("MR12=0x");
		ax_print_num(mr_op, 16);

		mr_op = 0;
#ifndef HAPS_DEBUG
		ddrmc_mrr(cs, 14, &mr_op);
#endif
		ax_print_str(", MR14=0x");
		ax_print_num(mr_op, 16);
		ax_print_str("\r\n");
		break;

	case TYPE_DDR4:
		//TBD
		break;

	default:
		ax_print_str("\r\nDDR type wrong!");
		break;
	}
}

static void ddr_rf_dll_cnt_print(void)
{
	u8 chan, dll_cnt;
	u32 reg_data = 0;

	for (chan = 0; chan < 2; chan++) {
		if (0 == chan)
			ax_print_str("\r\npha rf_dll_cnt_ac_f0 = ");
		else
			ax_print_str("\r\nphb rf_dll_cnt_ac_f0 = ");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_AC_TMG0_F0_ADDR + chan * DDR_PHY_OFFSET);
#endif
		dll_cnt = (reg_data >> 24) & 0x7f;
		ax_print_num(dll_cnt, 10);

		ax_print_str(", rf_dll_cnt_ds0_f0 = ");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_TMG0_F0_ADDR + chan * DDR_PHY_OFFSET);
#endif
		dll_cnt = (reg_data >> 24) & 0x7f;
		ax_print_num(dll_cnt, 10);

		ax_print_str(", rf_dll_cnt_ds1_f0 = ");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_TMG0_F0_ADDR + chan * DDR_PHY_OFFSET);
#endif
		dll_cnt = (reg_data >> 24) & 0x7f;
		ax_print_num(dll_cnt, 10);
	}
	ax_print_str("\r\n");
}

static void ddr_rf_dly_out_addr_print(void)
{
	u8 chan;
	u32 reg_data = 0;

	for (chan = 0; chan < 2; chan++) {
		if (0 == chan)
			ax_print_str("\r\npha DDRPHY_AC_CFG11 = 0x");
		else
			ax_print_str("\r\nphb DDRPHY_AC_CFG11 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_AC_CFG11_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_AC_CFG12 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_AC_CFG12_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);
	}
	ax_print_str("\r\n");
}

static void ddr_rf_dly_in_dqx_print(void)
{
	u8 chan;
	u32 reg_data = 0;

	for (chan = 0; chan < 2; chan++) {
		if (0 == chan)
			ax_print_str("\r\npha DDRPHY_DS0_CFG12 = 0x");
		else
			ax_print_str("\r\nphb DDRPHY_DS0_CFG12 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG12_ADDR + chan * DDR_PHY_OFFSET);//rank0
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS0_CFG13 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG13_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG12 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG12_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG13 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG13_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS0_CFG15 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG15_ADDR + chan * DDR_PHY_OFFSET);//rank1
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS0_CFG16 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG16_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG15 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG15_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG16 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG16_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);
	}
	ax_print_str("\r\n");
}

static void ddr_rf_dly_out_dqx_print(void)
{
	u8 chan;
	u32 reg_data = 0;

	for (chan = 0; chan < 2; chan++) {
		if (0 == chan)
			ax_print_str("\r\npha DDRPHY_DS0_CFG6 = 0x");
		else
			ax_print_str("\r\nphb DDRPHY_DS0_CFG6 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG6_ADDR + chan * DDR_PHY_OFFSET);//rank0
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS0_CFG7 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG7_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS0_CFG8 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG8_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG6 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG6_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG7 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG7_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG8 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG8_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS0_CFG9 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG9_ADDR + chan * DDR_PHY_OFFSET);//rank1
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS0_CFG10 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG10_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS0_CFG11 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS0_CFG11_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG9 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG9_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG10 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG10_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);

		ax_print_str(", DDRPHY_DS1_CFG11 = 0x");
#ifndef HAPS_DEBUG
		reg_data = readl(D_DDRPHY_DS1_CFG11_ADDR + chan * DDR_PHY_OFFSET);
#endif
		ax_print_num(reg_data, 16);
	}
	ax_print_str("\r\n");
}

static void ddr_training_param_print(void)
{
	ax_print_str("\r\n\r\n===================================== DDR TRAINING PARAM =====================================");
	ddr_type_print();
	ax_print_str("\tddr_freq=");
	ax_print_num(ddr_freqs, 10);
	ax_print_str("\r\n");
	dram_mrs_print(0);
	//dram_mrs_print(1);
	ddr_rf_dll_cnt_print();
	ddr_rf_dly_out_addr_print();
	ddr_rf_dly_in_dqx_print();
	ddr_rf_dly_out_dqx_print();
}

void ddr_scan_result_proc(DDR_TYPE ddr_type, u8 cs)
{
	int i;
	u8 ca_byte_sel_max;
#if 0 // for HAPS print debug
	int vref;

	for (vref = 0; vref < CA_SCAN_MAX; vref++) {
		s_ca_scan_vref[cs][vref].vref = vref;
		s_ca_scan_vref[cs][vref].width = 0x7f007f;
		s_ca_scan_vref[cs][vref].center = 0x4f004f;
	}
	for (vref = 0; vref < RD_SCAN_MAX; vref++) {
		s_read_scan_vref[cs][vref].vref = vref;
		s_read_scan_vref[cs][vref].width = 0x41414140;
		s_read_scan_vref[cs][vref].center = 0x3f3f3f3e;
	}
	for (vref = 0; vref < WR_SCAN_MAX; vref++) {
		s_write_scan_vref[cs][vref].vref = vref;
		s_write_scan_vref[cs][vref].width = 0x3f3f3f40;
		s_write_scan_vref[cs][vref].center = 0x58585859;
	}
#endif
	if (((ddr_types & 0x30) >> 4) == RF_DATA_WIDTH_X32) {
		ca_byte_sel_max = 2;
	} else {
		ca_byte_sel_max = 1;
	}

	if (0 == cs) {
		ddr_training_param_print();
		ax_print_str(STR_RANK0);
	}
	else
		ax_print_str(STR_RANK1);

	if (ddr_type & TYPE_LPDDR4) {
		ax_print_str("\r\n\r\n===================================== DDR CA EYE SCAN ========================================");
		ca_training_window_calc(ddr_type, cs);
		for (i = 0; i < ca_byte_sel_max; i++) {
			if (0 == i)
				ax_print_str(STR_CA_A);
			else
				ax_print_str(STR_CA_B);
			ddr_scan_result_output(&s_ca_scan_vref[cs][0], 0, i);
		}
	}

	ax_print_str("\r\n\r\n===================================== DDR READ EYE SCAN ======================================");
	read_training_window_calc(ddr_type, cs);
	for (i = 0; i < DQ_BYTE_SEL_MAX; i++) {
		ax_print_str("\r\n\r\nDQ BYTE ");
		ax_print_num(i, 10);
		ax_print_str(" :");
		ddr_scan_result_output(&s_read_scan_vref[cs][0], 1, i);
	}

	ax_print_str("\r\n\r\n===================================== DDR WRITE EYE SCAN =====================================");
	write_training_window_calc(ddr_type, cs);
	for (i = 0; i < DQ_BYTE_SEL_MAX; i++) {
		ax_print_str("\r\n\r\nDQ BYTE ");
		ax_print_num(i, 10);
		ax_print_str(" :");
		ddr_scan_result_output(&s_write_scan_vref[cs][0], 2, i);
	}
}
#endif

