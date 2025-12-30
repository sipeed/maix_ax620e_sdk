#include "cmn.h"
#include "board.h"
#include "chip_reg.h"
#include "boot.h"
#include "efuse_drv.h"
#include "timer.h"
#include "ax_adc.h"
#include "boot_mode.h"
#include "pwm.h"
#include "trace.h"

#define LOT_ID_BLK		0
#define LOT_ID_BLK1		1
#define CHIP_BOND_BLK		2
#define THM_TEMP_BLK		3
#define AES_KEY_BLK		23
#define PUB_KEY_BLK		15
#define COMMON_SYS_GLB (0x2340000)
#define DUMMY_SW9_BASE_OFFSET (0x220)

/* chip_mode[0] */
#define USB_DL_SD_BOOT_MASK	(0x1 << 0)
/* chip_mode[3:1] */
#define FLASH_BOOT_MASK	        (0x7 << 1)


u32 get_boot_mode(u32 chip_mode)
{
	/* EMMC UDA:0, EMMC BOOT 8BIT_50M_768K:1,  NAND 2K:3, EMMC BOOT 4BIT_25M_768K:4, NAND 4K:5, EMMC BOOT 4BIT_25M_128K:6, NOR:7 */
	return  (chip_mode & FLASH_BOOT_MASK) >> 1;
}

u32 get_boot_index(void)
{
	/* The valid value is 0, 1, 2, 3 */
	u32 chipmode;
	chipmode = readl(TOP_CHIPMODE_GLB_BACKUP0);
	return chipmode & 0x3;
}

void set_boot_index(int val)
{
	/* The valid value is 0, 1, 2, 3 */
	writel(0x3, TOP_CHIPMODE_GLB_BACKUP0_CLR);
	val &= 0x3;
	writel(val, TOP_CHIPMODE_GLB_BACKUP0_SET);
}

u32 get_ota_flag(void)
{
	u32 ota_flag;
	ota_flag = (readl(TOP_CHIPMODE_GLB_BACKUP0) >> 31) & 0x1;
	return ota_flag;
}

u32 get_boot_time(void)
{
	u32 boot_time;
	boot_time = readl(TOP_CHIPMODE_GLB_BACKUP1);
	return boot_time;
}

void set_boot_time(int val)
{
	writel(val, TOP_CHIPMODE_GLB_BACKUP1);
}

static int cpu_clk_configed = 0;

static void cpupll_config(int div)
{
	int value = 0;
	/* cpupll (12M x div) / 2 */
	/* Step1: close cpupll */
	writel(BIT(0), CPUPLL_ON_CFG_CLR_ADDR);
	/* Step2: set CPUPLL_FBK_FRA bit[28:2]=0x555_5555 */
	value = readl(CPUPLL_CFG0_ADDR);
	value &= ~0x1FFFFFFC;
	// value |= 0x5555555 << 2;
	writel(value, CPUPLL_CFG0_ADDR);
	/* Step3: set CPUPLL_FBK_INT bit[8:0]=0xA6 */
	value = readl(CPUPLL_CFG1_ADDR);
	value &= ~0xff;
	value |= div;
	writel(value, CPUPLL_CFG1_ADDR);

	/* Step4: open cpupll */
	writel(BIT(0), CPUPLL_ON_CFG_SET_ADDR);

	/* Step5: wait cpupll stable, GRP_PLL_RDY_STS_ADDR bit[3]==1 */
	while(!((readl(GRP_PLL_RDY_STS_ADDR) & BIT(3)) >> 3)) {
		udelay(2);
	}
	/* set cpupll to 1000m, clk_bus_flash_sel[1:0] switch to 2'b11 : cpll_312m */
	writel(GENMASK(4, 2), CPU_SYS_GLB_CLK_MUX0_CLR);
	value = readl(CPU_SYS_GLB_CLK_MUX0);
	value |= BIT(4) | BIT(1) | BIT(0);
	writel(value, CPU_SYS_GLB_CLK_MUX0);
}

static void vpll0_config(int div)
{
	int value = 0;
	/* VPLL0 (12M x div) / 4 */
	/* Step1: close VPLL0 */
	writel(BIT(0), VPLL0_ON_CFG_CLR_ADDR);
	/* Step2: set VPLL0_FBK_FRA bit[28:2]=0 */
	value = readl(VPLL0_CFG0_ADDR);
	value &= ~0x1FFFFFFC;
	writel(value, VPLL0_CFG0_ADDR);
	/* Step3: set VPLL0_FBK_INT bit[8:0]=0x11B */
	value = readl(VPLL0_CFG1_ADDR);
	value &= ~0xff;
	value |= div;
	writel(value, VPLL0_CFG1_ADDR);

	/* Step4: open VPLL0 */
	writel(BIT(0), VPLL0_ON_CFG_SET_ADDR);

	/* Step5: wait VPLL0 stable, GRP_PLL_RDY_STS_ADDR bit[1]==1 */
	while(!((readl(GRP_PLL_RDY_STS_ADDR) & BIT(1)) >> 1)) {
		udelay(2);
	}

	/* clk_nn_timer_sel[7:5]  3'b100 : vpll0_594m, clk_nn_0_sel[2:0] 3'b011 : vpll0_594m */
	writel(GENMASK(2, 0) | GENMASK(7, 5) , NPU_SYS_GLB_CLK_MUX0_CLR);
	writel(BIT(0) | BIT(1) |  BIT(7), NPU_SYS_GLB_CLK_MUX0_SET);

}

void set_pwm_volt(void)
{
	misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;

	if (misc_info->chip_type == AX631_CHIP_E) {
		pwm_volt_config(PWM11, 860);
	} else {
		#if (DDR4_CONFIG_3200) || (LPDDR4_CONFIG_3200)
			pwm_volt_config(PWM11, 828);
			ax_print_str("vddcore set 828mv\n");
		#else
#ifdef VPLL0_CONFIG_850M
			pwm_volt_config(PWM11, 828);
#else
			pwm_volt_config(PWM11, 800);
#endif
		#endif
	}
}

static void tmo_disable(void)
{
	/*clk_axi_tmo_eb_clr*/
	writel(1 << 2, COMM_SYS_GLB + 0x38);
}

int chip_init(void)
{
	misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;

	if (cpu_clk_configed)
		return 0;
	tmo_disable();
	switch (misc_info->chip_type) {
		case AX620Q_CHIP_E:
		case AX620QX_CHIP_E:
		case AX620QZ_CHIP_E:
			cpupll_config(0xA6);//1.0GB
			/* clk_nn_0_sel[2:0] switch to 3'b011 : vpll0_594m */
			writel(GENMASK(2, 0), NPU_SYS_GLB_CLK_MUX0_CLR);
			writel(BIT(1) | BIT(0), NPU_SYS_GLB_CLK_MUX0_SET);
			/* aclk_cpu_top_sel[26:24] switch to 3'b011 : cpll_416m */
			/* aclk_isp_top_sel[29:27] switch to 3'b011 : cpll_416m */
			writel(BIT(25) | BIT(24) | BIT(28) | BIT(27), COMM_SYS_GLB_CLK_MUX0_SET);

			/* pclk_top_sel[1:0] switch to 2'b11 : cpll_208m */
			writel(BIT(1) | BIT(0), COMM_SYS_GLB_CLK_MUX2_SET);

			/* clk_isp_mm_sel[19:17] switch to 3'b011 : cpll_312m
			* aclk_vpu_top_sel[11:9] switch to 3'b011 : cpll_416m
			* aclk_ocm_top_sel[8:6] switch to 3'b011 : cpll_416m
			* aclk_nn_top_sel[5:3] switch to 3'b011 : cpll_416m
			* aclk_mm_top_sel[2:0] switch to 3'b011 : cpll_416m
			* clk_tmr64_sel[27] switch to 1'b1 : 24m
			*/
#ifdef DEBUG_BOOT_TIME
			ax_memset((void *)(0x900), 0, 0x50);
			writel(readl(0x4820000), 0x900); //power on->32K timer
#endif
			writel(0x80606DB, COMM_SYS_GLB_CLK_MUX1_SET);
			break;
		case AX620QP_CHIP_E:
			/* clk_cpu_sel[4:2] switch core0-1 to 3'b100 : cpupll_1200m */
			/* clk_bus_flash_sel[1:0] switch to 2'b11 : cpll_312m */
			writel(GENMASK(4, 2), CPU_SYS_GLB_CLK_MUX0_CLR);
			writel(BIT(4) | BIT(1) | BIT(0), CPU_SYS_GLB_CLK_MUX0_SET);
			/* clk_nn_0_sel[2:0] switch to 3'b100 : npll_800m */
			writel(GENMASK(2, 0), NPU_SYS_GLB_CLK_MUX0_CLR);
			writel(BIT(2), NPU_SYS_GLB_CLK_MUX0_SET);
			/* aclk_cpu_top_sel[26:24] switch to 3'b011 : cpll_416m */
			/* aclk_isp_top_sel[29:27] switch to 3'b011 : cpll_416m */
			writel(BIT(25) | BIT(24) | BIT(28) | BIT(27), COMM_SYS_GLB_CLK_MUX0_SET);

			/* pclk_top_sel[1:0] switch to 2'b11 : cpll_208m */
			writel(BIT(1) | BIT(0), COMM_SYS_GLB_CLK_MUX2_SET);

			/* clk_isp_mm_sel[19:17] switch to 3'b011 : cpll_312m
			* aclk_vpu_top_sel[11:9] switch to 3'b011 : cpll_416m
			* aclk_ocm_top_sel[8:6] switch to 3'b011 : cpll_416m
			* aclk_nn_top_sel[5:3] switch to 3'b011 : cpll_416m
			* aclk_mm_top_sel[2:0] switch to 3'b011 : cpll_416m
			* clk_tmr64_sel[27] switch to 1'b1 : 24m
			*/
#ifdef DEBUG_BOOT_TIME
			ax_memset((void *)(0x900), 0, 0x50);
			writel(readl(0x4820000), 0x900); //power on->32K timer
#endif
			writel(0x80606DB, COMM_SYS_GLB_CLK_MUX1_SET);
			break;
		case AX630C_CHIP_E:
			/* clk_cpu_sel[4:2] switch core0-1 to 3'b100 : cpupll_1200m */
			/* clk_bus_flash_sel[1:0] switch to 2'b11 : cpll_312m */
			writel(GENMASK(4, 2), CPU_SYS_GLB_CLK_MUX0_CLR);
			writel(BIT(4) | BIT(1) | BIT(0), CPU_SYS_GLB_CLK_MUX0_SET);
			/* clk_nn_0_sel[2:0] switch to 3'b100 : npll_800m */
			writel(GENMASK(2, 0), NPU_SYS_GLB_CLK_MUX0_CLR);
			writel(BIT(2), NPU_SYS_GLB_CLK_MUX0_SET);
			/* aclk_cpu_top_sel[26:24] switch to 3'b101 : npll_533m */
			/* aclk_isp_top_sel[29:27] switch to 3'b101 : npll_533m */
			writel(BIT(26) | BIT(24) | BIT(29) | BIT(27), COMM_SYS_GLB_CLK_MUX0_SET);

			/* pclk_top_sel[1:0] switch to 2'b11 : cpll_208m */
			writel(BIT(1) | BIT(0), COMM_SYS_GLB_CLK_MUX2_SET);

			/* clk_isp_mm_sel[19:17] switch to 3'b100 : cpll_416m
			* aclk_vpu_top_sel[11:9] switch to 3'b101 : npll_533m
			* aclk_ocm_top_sel[8:6] switch to 3'b101 : npll_533m
			* aclk_nn_top_sel[5:3] switch to 3'b101 : npll_533m
			* aclk_mm_top_sel[2:0] switch to 3'b101 : npll_533m
			* clk_tmr64_sel[27] switch to 1'b1 : 24m
			*/
#ifdef DEBUG_BOOT_TIME
			ax_memset((void *)(0x900), 0, 0x50);
			writel(readl(0x4820000), 0x900); //power on->24M timer
#endif
			writel(0x8080B6D, COMM_SYS_GLB_CLK_MUX1_SET);
			break;
		case AX631_CHIP_E:
			cpupll_config(0xFA);//1.5GB
			vpll0_config(0x13C);//948M
			/* aclk_cpu_top_sel[26:24] switch to 3'b101 : npll_533m */
			/* aclk_isp_top_sel[29:27] switch to 3'b101 : npll_533m */
			writel(BIT(26) | BIT(24) | BIT(29) | BIT(27), COMM_SYS_GLB_CLK_MUX0_SET);

			/* pclk_top_sel[1:0] switch to 2'b11 : cpll_208m */
			writel(BIT(1) | BIT(0), COMM_SYS_GLB_CLK_MUX2_SET);

			/* clk_isp_mm_sel[19:17] switch to 3'b100 : cpll_416m
			* aclk_vpu_top_sel[11:9] switch to 3'b101 : npll_533m
			* aclk_ocm_top_sel[8:6] switch to 3'b101 : npll_533m
			* aclk_nn_top_sel[5:3] switch to 3'b101 : npll_533m
			* aclk_mm_top_sel[2:0] switch to 3'b101 : npll_533m
			* clk_tmr64_sel[27] switch to 1'b1 : 24m
			*/
#ifdef DEBUG_BOOT_TIME
			ax_memset((void *)(0x900), 0, 0x50);
			writel(readl(0x4820000), 0x900); //power on->24M timer
#endif
			writel(0x8080B6D, COMM_SYS_GLB_CLK_MUX1_SET);
			break;
		default:
			break;
	}
	/* clk_tmr64_0_sel[15] switch to 1'b1 : 24m */
	/* clk_tmr64_1_sel[16] switch to 1'b1 : 24m */
	/* clk_uart_sel[18:17] switch to 2'b11 : cpll_208m */
	writel(GENMASK(18, 15), PERI_SYS_GLB_CLK_MUX0_SET);

	/* clk_spi_m0_sel[8:7] switch to 2'b11 : 208m */
	/* clk_spi_m1_sel[10:9] switch to 2'b11 : 208m */
	/* clk_spi_m2_sel[12:11] switch to 2'b11 : 208m */
	writel(GENMASK(12, 7) | GENMASK(4, 3), PERI_SYS_GLB_CLK_MUX0_SET);
	/* apb_ssi_m0/1/2_ss_in_n_set */
	writel(GENMASK(4, 2), PERI_SYS_GLB_SPI_SET);
	/* clk_spi_s_sel[21:20]  switch to 2'b11 : 208m */
	writel(GENMASK(21, 20), FLASH_SYS_GLB_CLK_MUX0_SET);
	/* set spi_m0/1/2 and spi_s to simplex */
	writel(GENMASK(3, 0), PIN_MUX_G1_MISC_SET);
	writel(ABORT_WDT2_EN | ABORT_WDT0_EN | ABORT_THM_EN, COMM_ABORT_CFG);
	cpu_clk_configed = 1;

	return 0;
}


int generic_timer_init(void)
{
	writel(0x0, GENERIC_TIMER_BASE + 0x8);
	writel(0x0, GENERIC_TIMER_BASE + 0xC);
	writel(0x16E3600, GENERIC_TIMER_BASE + 0x20); //24mhz
	writel(0x11, GENERIC_TIMER_BASE);   //enable generic timer

	return 0;
}

void wtd_enable(u8 enable)
{
	if (enable) {
		writel(BIT(14), PERI_SYS_GLB_CLK_EB0_SET);
	} else {
		writel(BIT(14), PERI_SYS_GLB_CLK_EB0_CLR);
	}
}

u32 get_emmc_voltage(void)
{
	u32 val;
	val = readl(PIN_MUX_G11_VDET_RO0);
	/* 0: 3.3V   1: 1.8V */
	if (((val >> 12) & BIT(0))) { //3.3v
		return 0;
	} else { //1.8v
		return 1;
	}
}


#ifdef SECURE_BOOT_TEST
/* #define RSA_3072 */
#ifdef RSA_3072
u32 pub_key_hash[8] = {
	0x6C1DDA30, 0x53CD22C1, 0x97BEB790, 0xAAEDA709,
	0xEF49ED74, 0xEC82D080, 0x90175EFB, 0x637DD4E5
};
#else
u32 pub_key_hash[8] = {
	0x78DDFE9E, 0x698A9288, 0xCCAD1548, 0x5CA04574,
	0x9F1AA945, 0xF57EB822, 0xB807DB67, 0x29140398,
};
#endif
#endif
misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;

static unsigned char ax_board_id[AX620E_CHIP_MAX][16] = {
	/*AX620Q_CHIP_E board id*/
	[AX620Q_CHIP_E][PHY_AX620Q_LP4_EVB_V1_0] = AX620Q_LP4_EVB_V1_0,
	[AX620Q_CHIP_E][PHY_AX620Q_LP4_DEMO_V1_0] = AX620Q_LP4_DEMO_V1_0,
	[AX620Q_CHIP_E][PHY_AX620Q_LP4_SLT_V1_0] = AX620Q_LP4_SLT_V1_0,
	[AX620Q_CHIP_E][PHY_AX620Q_LP4_DEMO_V1_1] = AX620Q_LP4_DEMO_V1_1,
	[AX620Q_CHIP_E][PHY_AX620Q_LP4_38BOARD_V1_0] = AX620Q_LP4_38BOARD_V1_0,
	[AX620Q_CHIP_E][PHY_AX620Q_LP4_MINION_BOARD] = AX620Q_LP4_MINION_BOARD,
	/*AX620QX_CHIP_E board id*/
	[AX620QX_CHIP_E][PHY_AX620Q_LP4_EVB_V1_0] = AX620Q_LP4_EVB_V1_0,
	[AX620QX_CHIP_E][PHY_AX620Q_LP4_DEMO_V1_0] = AX620Q_LP4_DEMO_V1_0,
	[AX620QX_CHIP_E][PHY_AX620Q_LP4_SLT_V1_0] = AX620Q_LP4_SLT_V1_0,
	[AX620QX_CHIP_E][PHY_AX620Q_LP4_DEMO_V1_1] = AX620Q_LP4_DEMO_V1_1,
	[AX620QX_CHIP_E][PHY_AX620Q_LP4_38BOARD_V1_0] = AX620Q_LP4_38BOARD_V1_0,
	[AX620QX_CHIP_E][PHY_AX620Q_LP4_MINION_BOARD] = AX620Q_LP4_MINION_BOARD,
	/*AX630C_CHIP_E board id*/
	[AX630C_CHIP_E][PHY_AX630C_EVB_V1_0] = AX630C_EVB_V1_0,
	[AX630C_CHIP_E][PHY_AX630C_DEMO_V1_0] = AX630C_DEMO_V1_0,
	[AX630C_CHIP_E][PHY_AX630C_DEMO_DDR3_V1_0] = AX630C_DEMO_DDR3_V1_0,
	[AX630C_CHIP_E][PHY_AX630C_SLT_V1_0] = AX630C_SLT_V1_0,
	[AX630C_CHIP_E][PHY_AX630C_DEMO_V1_1] = AX630C_DEMO_V1_1,
	[AX630C_CHIP_E][PHY_AX630C_DEMO_LP4_V1_0] = AX630C_DEMO_LP4_V1_0,
	// ### SIPEED EDIT ###
	[AX630C_CHIP_E][PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G] = AX630C_AX631_MAIXCAM2_SOM_0_5G,
	[AX630C_CHIP_E][PHY_AX630C_AX631_MAIXCAM2_SOM_1G] = AX630C_AX631_MAIXCAM2_SOM_1G,
	[AX630C_CHIP_E][PHY_AX630C_AX631_MAIXCAM2_SOM_2G] = AX630C_AX631_MAIXCAM2_SOM_2G,
	[AX630C_CHIP_E][PHY_AX630C_AX631_MAIXCAM2_SOM_4G] = AX630C_AX631_MAIXCAM2_SOM_4G,
	// ### SIPEED EDIT END ###
	/*AX631_CHIP_E board id*/
	[AX631_CHIP_E][PHY_AX630C_EVB_V1_0] = AX630C_EVB_V1_0,
	[AX631_CHIP_E][PHY_AX630C_DEMO_V1_0] = AX630C_DEMO_V1_0,
	[AX631_CHIP_E][PHY_AX630C_SLT_V1_0] = AX630C_SLT_V1_0,
	[AX631_CHIP_E][PHY_AX630C_DEMO_V1_1] = AX630C_DEMO_V1_1,
	[AX631_CHIP_E][PHY_AX630C_DEMO_LP4_V1_0] = AX630C_DEMO_LP4_V1_0,
	// ### SIPEED EDIT ###
	[AX631_CHIP_E][PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G] = AX630C_AX631_MAIXCAM2_SOM_0_5G,
	[AX631_CHIP_E][PHY_AX630C_AX631_MAIXCAM2_SOM_1G] = AX630C_AX631_MAIXCAM2_SOM_1G,
	[AX631_CHIP_E][PHY_AX630C_AX631_MAIXCAM2_SOM_2G] = AX630C_AX631_MAIXCAM2_SOM_2G,
	[AX631_CHIP_E][PHY_AX630C_AX631_MAIXCAM2_SOM_4G] = AX630C_AX631_MAIXCAM2_SOM_4G,
	// ### SIPEED EDIT END ###
	/*AX620QX_CHIP_E board id*/
	[AX620QZ_CHIP_E][PHY_AX620Q_LP4_EVB_V1_0] = AX620Q_LP4_EVB_V1_0,
	[AX620QZ_CHIP_E][PHY_AX620Q_LP4_DEMO_V1_0] = AX620Q_LP4_DEMO_V1_0,
	[AX620QZ_CHIP_E][PHY_AX620Q_LP4_SLT_V1_0] = AX620Q_LP4_SLT_V1_0,
	[AX620QZ_CHIP_E][PHY_AX620Q_LP4_DEMO_V1_1] = AX620Q_LP4_DEMO_V1_1,
	[AX620QZ_CHIP_E][PHY_AX620Q_LP4_38BOARD_V1_0] = AX620Q_LP4_38BOARD_V1_0,
	[AX620QZ_CHIP_E][PHY_AX620Q_LP4_MINION_BOARD] = AX620Q_LP4_MINION_BOARD,
	/*AX620QP_CHIP_E board id*/
	[AX620QP_CHIP_E][PHY_AX620Q_LP4_EVB_V1_0] = AX620Q_LP4_EVB_V1_0,
	[AX620QP_CHIP_E][PHY_AX620Q_LP4_DEMO_V1_0] = AX620Q_LP4_DEMO_V1_0,
	[AX620QP_CHIP_E][PHY_AX620Q_LP4_SLT_V1_0] = AX620Q_LP4_SLT_V1_0,
	[AX620QP_CHIP_E][PHY_AX620Q_LP4_DEMO_V1_1] = AX620Q_LP4_DEMO_V1_1,
	[AX620QP_CHIP_E][PHY_AX620Q_LP4_38BOARD_V1_0] = AX620Q_LP4_38BOARD_V1_0,
	[AX620QP_CHIP_E][PHY_AX620Q_LP4_MINION_BOARD] = AX620Q_LP4_MINION_BOARD,
};

static int calc_id(int data)
{
	int id;
	if(data < 0 || data > 0x220) {
		id = -1;
	}
	if(data >= 0 && data <= 0x20) {
		id = 0;
	} else if(data >= 0x3E0 && data <= 0x3FF) {
		id = 16;
	} else {
		id = (((data - 0x20) / 0x40) + 1);
	}
	return id;
}

int get_board_id(void)
{
	unsigned int board_val, cal_val;

	adc_read_boardid(0, &board_val);
	cal_val = calc_id(board_val);
	misc_info->phy_board_id = cal_val;
	cal_val =  ax_board_id[misc_info->chip_type][cal_val];
	return cal_val;
}

/* The high 16bit value stores the board id, and the low 16bit value stores the plate id . */
void store_board_id(void)
{
	unsigned int board_id;

	board_id = get_board_id();
	misc_info->board_id = board_id;
}
/* To get the ID of the chip type. */
static unsigned char hanming_decode_74(unsigned char  a)
{
	unsigned char b=0,c=0;
	b = a;
	c = 0;
	if(b&0x40) {
		c = c^0x07 ;
	}
	if(b&0x20) {
		c = c^0x06 ;
	}
	if(b&0x10) {
		c = c^0x05 ;
	}
	if(b&0x08) {
		c = c^0x03 ;
	}
	if(b&0x04) {
		c = c^0x04 ;
	}
	if(b&0x02) {
		c = c^0x02 ;
	}
	if(b&0x01) {
		c = c^0x01 ;
	}
	switch(c) {
	case 0 :
		b = b >> 3 ;
		break ;
	case 1 :
		b = b >> 3 ;
		break ;
	case 2 :
		b = b >> 3 ;
		break ;
	case 3 :
		b = (b^0x08) >> 3 ;
		break ;
	case 4 :
		b = b >> 3 ;
		break ;
	case 5 :
		b = (b^0x10) >> 3 ;
		break ;
	case 6 :
		b = (b^0x20) >> 3 ;
		break ;
	case 7 :
		b = (b^0x40) >> 3 ;
		break ;
	}
	return (b & 0x0F);
}
unsigned char get_chip_type_id(void)
{
	return hanming_decode_74(misc_info->chip_type);
}

void store_chip_type(void)
{
	unsigned char chip_type;

	chip_type = get_chip_type_id();
	if(!chip_type)
		misc_info->chip_type = AX630C_CHIP_E;
	else
		misc_info->chip_type = chip_type;

	writel(misc_info->chip_type, COMMON_SYS_GLB + DUMMY_SW9_BASE_OFFSET);
}

void get_misc_info(void)
{
	int value;
#ifdef SECURE_BOOT_TEST
	int i;
#endif

	efuse_read(THM_TEMP_BLK, &value);
	misc_info->thm_temp = (value >> 18) & 0x3ff;;
	efuse_read(CHIP_BOND_BLK, &value);
	misc_info->chip_type = (value >> 24) & 0xff;
	misc_info->thm_vref = value & 0x1f;
	misc_info->bgs = (value >> 5) & 0xf;
	misc_info->trim = (value >> 13) & 0x7;

	efuse_read(LOT_ID_BLK, &value);
	misc_info->uid_l = value;
	efuse_read(LOT_ID_BLK1, &value);
	misc_info->uid_h = value;
	store_chip_type();
	store_board_id();

#ifdef SECURE_BOOT_TEST
	for (i = 0; i < 8; i++) {
		misc_info->pub_key_hash[i] = pub_key_hash[i];
	}
/* You can optionally get keys from efuse when CONFIG_AXERA_SECURE_BOOT is defined. */
/*
	for(i=0; i < 8; i++){
		efuse_read(AES_KEY_BLK + i, &value);
		misc_info->aes_key[i] = value;
	}

	for (i = 0; i < 8; i++) {
		efuse_read(PUB_KEY_BLK + i, &value);
		misc_info->pub_key_hash[i] = value;
	}
*/
#endif
}




