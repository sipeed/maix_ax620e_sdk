#include <common.h>
#include <asm/arch/ax620e.h>
#include <asm/io.h>
#include <early_printf.h>

typedef enum dphyrx_pin_func_type{
	AX_FUNC_NONE = 0,
	AX_FUNC_SPI,
	AX_FUNC_UART,
	AX_FUNC_GPIO,
	AX_FUNC_COM_GPIO,
	AX_FUNC_MAX,
} dphyrx_pin_func_type_t;

typedef enum dphyrx_pin_lane{
	AX_DPHY_LANE_0 = 0,
	AX_DPHY_LANE_1,
	AX_DPHY_LANE_2,
	AX_DPHY_LANE_MAX,
} dphyrx_pin_lane_t;

typedef enum dphyrx_pin_sig{
	AX_DPHY_N = 0,
	AX_DPHY_P,
} dphyrx_pin_sig_t;

typedef enum dphyrx_pin_dir{
	AX_DPHY_OUTPUT = 0,
	AX_DPHY_INPUT,
} dphyrx_pin_dir_t;

struct pin_dphyrx_info {
	char *	pad_name;
	dphyrx_pin_lane_t lane;
	dphyrx_pin_sig_t signal;
	dphyrx_pin_dir_t fun4_6_def_dir;
	unsigned long pinmux_reg_base;
	unsigned long pin_dphyrx_reg_base;
	unsigned long rxcdphy_reg_base;
	dphyrx_pin_func_type_t func4_type;
	char *	func4_name;
	dphyrx_pin_func_type_t func6_type;
	char *	func6_name;
};

#define PINMUX_DPHYRX0  0x4251200
#define PINMUX_DPHYRX1  0x4251400
#define PINMUX_DPHYRX2  0x4251600
#define PINMUX_DPHYRX3  0x4251800
#define DPHYRX0_PIN_REG 0x1300D000
#define DPHYRX1_PIN_REG 0x1300E000
#define DPHYRX2_PIN_REG 0x1300F000
#define DPHYRX3_PIN_REG 0x13010000

#define ISP_RXCDPHY_NUM 8
#define ISP_RXCDPHY_0   0x13C00000
#define ISP_RXCDPHY_1   0x13C10000
#define ISP_RXCDPHY_2   0x13C20000
#define ISP_RXCDPHY_3   0x13C30000
#define ISP_RXCDPHY_4   0x13C40000
#define ISP_RXCDPHY_5   0x13C50000
#define ISP_RXCDPHY_6   0x13C60000
#define ISP_RXCDPHY_7   0x13C70000

#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1 (0x1021 * 2)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_7 (0x1027 * 2)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_8 (0x1028 * 2)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1_OA_LANE_GPI_EN_BIT           (6)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1_OA_LANE_GPI_HYST_EN_BIT      (8)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1_OA_LANE_GPO_EN_BIT           (10)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_7_OA_LANE_LPTX_EN_OVR_VAL_BIT  (0)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_7_OA_LANE_LPTX_PON_OVR_VAL_BIT (2)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_8_OA_LANE_LPTX_EN_OVR_EN_BIT   (6)
#define CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_8_OA_LANE_LPTX_PON_OVR_EN_BIT  (7)

#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_0   (0x1C20 * 2)
#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_1   (0x1C21 * 2)
#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_7   (0x1C27 * 2)
#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_8   (0x1C28 * 2)
#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_0_OA_CB_PON_OVR_VAL_BIT          (4)
#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_1_OA_CB_PON_OVR_EN_BIT           (4)
#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_7_OA_SETR_OVR_EN_BIT             (11)
#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_8_OA_SETR_OVR_VAL_BIT            (0)
#define CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_8_OA_SETR_OVR_VAL_MASK           (0xf << CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_8_OA_SETR_OVR_VAL_BIT)

#define LANE_OUTPUT_OVERRIDE 0xf

static struct pin_dphyrx_info pin_dphyrx0[12] = {
	{"RX1_CKP_C0", AX_DPHY_LANE_1, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_1, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A9"},
	{"RX1_CKN_C1", AX_DPHY_LANE_1, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_1, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A10"},
	{"RX1_DP0_A0", AX_DPHY_LANE_0, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_1, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A11"},
	{"RX1_DN0_B0", AX_DPHY_LANE_0, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_1, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A12"},
	{"RX1_DP1_A1", AX_DPHY_LANE_2, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_1, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A13"},
	{"RX1_DN1_B1", AX_DPHY_LANE_2, AX_DPHY_N, AX_DPHY_INPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_1, AX_FUNC_SPI, "SPI_M2_MISO_m", AX_FUNC_GPIO, "GPIO2_A14"},
	{"RX0_CKP_C0", AX_DPHY_LANE_1, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_0, AX_FUNC_SPI, "SPI_M2_MOSI_m", AX_FUNC_GPIO, "GPIO2_A15"},
	{"RX0_CKN_C1", AX_DPHY_LANE_1, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_0, AX_FUNC_SPI, "SPI_M2_CS3_m", AX_FUNC_GPIO, "GPIO2_A16"},
	{"RX0_DP0_A0", AX_DPHY_LANE_0, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_0, AX_FUNC_SPI, "SPI_M2_CS2_m", AX_FUNC_GPIO, "GPIO2_A17"},
	{"RX0_DN0_B0", AX_DPHY_LANE_0, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_0, AX_FUNC_SPI, "SPI_M2_CS0_m", AX_FUNC_GPIO, "GPIO2_A18"},
	{"RX0_DP1_A1", AX_DPHY_LANE_2, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_0, AX_FUNC_SPI, "SPI_M2_CS1_m", AX_FUNC_GPIO, "GPIO2_A19"},
	{"RX0_DN1_B1", AX_DPHY_LANE_2, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX0, DPHYRX0_PIN_REG, ISP_RXCDPHY_0, AX_FUNC_SPI, "SPI_M2_CLK_m", AX_FUNC_GPIO, "GPIO2_A20"},
};

static struct pin_dphyrx_info pin_dphyrx1[12] = {
	{"RX3_CKP_C0", AX_DPHY_LANE_1, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_3, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A24"},
	{"RX3_CKN_C1", AX_DPHY_LANE_1, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_3, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A25"},
	{"RX3_DP0_A0", AX_DPHY_LANE_0, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_3, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A26"},
	{"RX3_DN0_B0", AX_DPHY_LANE_0, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_3, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A27"},
	{"RX3_DP1_A1", AX_DPHY_LANE_2, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_3, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A28"},
	{"RX3_DN1_B1", AX_DPHY_LANE_2, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_3, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A29"},
	{"RX2_CKP_C0", AX_DPHY_LANE_1, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_2, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A30"},
	{"RX2_CKN_C1", AX_DPHY_LANE_1, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_2, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A31"},
	{"RX2_DP0_A0", AX_DPHY_LANE_0, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_2, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A5"},
	{"RX2_DN0_B0", AX_DPHY_LANE_0, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_2, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A6"},
	{"RX2_DP1_A1", AX_DPHY_LANE_2, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_2, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A7"},
	{"RX2_DN1_B1", AX_DPHY_LANE_2, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX1, DPHYRX1_PIN_REG, ISP_RXCDPHY_2, AX_FUNC_NONE, NULL, AX_FUNC_GPIO, "GPIO2_A8"},
};

static struct pin_dphyrx_info pin_dphyrx2[12] = {
	{"RX5_CKP_C0", AX_DPHY_LANE_1, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_5, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A12"},
	{"RX5_CKN_C1", AX_DPHY_LANE_1, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_5, AX_FUNC_UART, "UART9_CTSN", AX_FUNC_COM_GPIO, "COM_GPIO_A13"},
	{"RX5_DP0_A0", AX_DPHY_LANE_0, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_5, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A14"},
	{"RX5_DN0_B0", AX_DPHY_LANE_0, AX_DPHY_N, AX_DPHY_INPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_5, AX_FUNC_UART, "UART9_RXD", AX_FUNC_COM_GPIO, "COM_GPIO_A15"},
	{"RX5_DP1_A1", AX_DPHY_LANE_2, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_5, AX_FUNC_UART, "UART9_RTSN", AX_FUNC_COM_GPIO, "COM_GPIO_A16"},
	{"RX5_DN1_B1", AX_DPHY_LANE_2, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_5, AX_FUNC_UART, "UART9_TXD", AX_FUNC_COM_GPIO, "COM_GPIO_A17"},
	{"RX4_CKP_C0", AX_DPHY_LANE_1, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_4, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A18"},
	{"RX4_CKN_C1", AX_DPHY_LANE_1, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_4, AX_FUNC_UART, "UART10_CTSN", AX_FUNC_COM_GPIO, "COM_GPIO_A19"},
	{"RX4_DP0_A0", AX_DPHY_LANE_0, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_4, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A20"},
	{"RX4_DN0_B0", AX_DPHY_LANE_0, AX_DPHY_N, AX_DPHY_INPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_4, AX_FUNC_UART, "UART10_RXD", AX_FUNC_COM_GPIO, "COM_GPIO_A21"},
	{"RX4_DP1_A1", AX_DPHY_LANE_2, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_4, AX_FUNC_UART, "UART10_RTSN", AX_FUNC_COM_GPIO, "COM_GPIO_A22"},
	{"RX4_DN1_B1", AX_DPHY_LANE_2, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX2, DPHYRX2_PIN_REG, ISP_RXCDPHY_4, AX_FUNC_UART, "UART10_TXD", AX_FUNC_COM_GPIO, "COM_GPIO_A23"},
};

static struct pin_dphyrx_info pin_dphyrx3[12] = {
	{"RX7_CKP_C0", AX_DPHY_LANE_1, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_7, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A0"},
	{"RX7_CKN_C1", AX_DPHY_LANE_1, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_7, AX_FUNC_UART, "UART7_CTSN", AX_FUNC_COM_GPIO, "COM_GPIO_A1"},
	{"RX7_DP0_A0", AX_DPHY_LANE_0, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_7, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A2"},
	{"RX7_DN0_B0", AX_DPHY_LANE_0, AX_DPHY_N, AX_DPHY_INPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_7, AX_FUNC_UART, "UART7_RXD", AX_FUNC_COM_GPIO, "COM_GPIO_A3"},
	{"RX7_DP1_A1", AX_DPHY_LANE_2, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_7, AX_FUNC_UART, "UART7_RTSN", AX_FUNC_COM_GPIO, "COM_GPIO_A4"},
	{"RX7_DN1_B1", AX_DPHY_LANE_2, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_7, AX_FUNC_UART, "UART7_TXD", AX_FUNC_COM_GPIO, "COM_GPIO_A5"},
	{"RX6_CKP_C0", AX_DPHY_LANE_1, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_6, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A6"},
	{"RX6_CKN_C1", AX_DPHY_LANE_1, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_6, AX_FUNC_UART, "UART8_CTSN", AX_FUNC_COM_GPIO, "COM_GPIO_A7"},
	{"RX6_DP0_A0", AX_DPHY_LANE_0, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_6, AX_FUNC_NONE, NULL, AX_FUNC_COM_GPIO, "COM_GPIO_A8"},
	{"RX6_DN0_B0", AX_DPHY_LANE_0, AX_DPHY_N, AX_DPHY_INPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_6, AX_FUNC_UART, "UART8_RXD", AX_FUNC_COM_GPIO, "COM_GPIO_A9"},
	{"RX6_DP1_A1", AX_DPHY_LANE_2, AX_DPHY_P, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_6, AX_FUNC_UART, "UART8_RTSN", AX_FUNC_COM_GPIO, "COM_GPIO_A10"},
	{"RX6_DN1_B1", AX_DPHY_LANE_2, AX_DPHY_N, AX_DPHY_OUTPUT, PINMUX_DPHYRX3, DPHYRX3_PIN_REG, ISP_RXCDPHY_6, AX_FUNC_UART, "UART8_TXD", AX_FUNC_COM_GPIO, "COM_GPIO_A11"},
};

//#define DPHY_DEBUG
static void dphy_writew(u16 val, unsigned long addr)
{
#ifdef DPHY_DEBUG
	early_printf("Write 0x%lx, Val 0x%x\r\n", addr, val);
#endif
	writew(val, addr);
}

static u16 dphy_readw(unsigned long addr)
{
	u16 val= readw(addr);
#ifdef DPHY_DEBUG
	early_printf("Read 0x%lx, Val 0x%x\r\n", addr, val);
#endif
	return val;
}

static void dphy_writel(u32 val, unsigned long addr)
{
#ifdef DPHY_DEBUG
	early_printf("Write 0x%lx, Val 0x%x\r\n", addr, val);
#endif
	writel(val, addr);
}

static u32 dphy_readl(unsigned long addr)
{
	u32 val= readl(addr);
#ifdef DPHY_DEBUG
	early_printf("Read 0x%lx, Val 0x%x\r\n", addr, val);
#endif
	return val;
}

static void dphyrx_pin_dir_config(unsigned long ip_base, dphyrx_pin_lane_t lane, dphyrx_pin_sig_t sigbit, dphyrx_pin_dir_t dir)
{
	unsigned long addr;
	u32 temp;
	int phy_index = (ip_base & 0x70000) >> 16;
	static u8 output_configed[ISP_RXCDPHY_NUM] = {0};

	if (lane >= AX_DPHY_LANE_MAX) {
		early_printf("%s: LANE_%d error\r\n", __func__, lane);
		return;
	}
#ifdef DPHY_DEBUG
	early_printf("[CDPHY%d]IP Base: 0x%lX, LANE_%d_%s %s\r\n", phy_index, ip_base, lane, sigbit ? "P" : "N", (AX_DPHY_OUTPUT == dir) ? "OUTPUT" : "INPUT");
#endif
	if (AX_DPHY_INPUT == dir) {
		addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_7 + lane * 0x400;
		temp = dphy_readw(addr);
		temp &= ~ BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_7_OA_LANE_LPTX_EN_OVR_VAL_BIT + sigbit);
		dphy_writew(temp, addr);

		addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_8 + lane * 0x400;
		temp = dphy_readw(addr);
		temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_8_OA_LANE_LPTX_EN_OVR_EN_BIT);
		dphy_writew(temp, addr);

		addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1 + lane * 0x400;
		temp = dphy_readw(addr);
		temp &= ~ BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1_OA_LANE_GPO_EN_BIT + sigbit);
		temp &= ~ BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1_OA_LANE_GPI_HYST_EN_BIT + sigbit);
		temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1_OA_LANE_GPI_EN_BIT + sigbit);
		dphy_writew(temp, addr);
	}
	else {
		if (0 == output_configed[phy_index]) {
			addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_8;
			temp = dphy_readw(addr);
			temp &= ~ CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_8_OA_SETR_OVR_VAL_MASK;
			temp |= (LANE_OUTPUT_OVERRIDE << CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_8_OA_SETR_OVR_VAL_BIT);
			dphy_writew(temp, addr);

			addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_7;
			temp = dphy_readw(addr);
			temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_7_OA_SETR_OVR_EN_BIT);
			dphy_writew(temp, addr);

			addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_0;
			temp = dphy_readw(addr);
			temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_0_OA_CB_PON_OVR_VAL_BIT);
			dphy_writew(temp, addr);

			addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_1;
			temp = dphy_readw(addr);
			temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_1_OA_CB_PON_OVR_EN_BIT);
			dphy_writew(temp, addr);

			output_configed[phy_index] = 1;
		}

		addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_7 + lane * 0x400;
		temp = dphy_readw(addr);
		temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_7_OA_LANE_LPTX_PON_OVR_VAL_BIT + sigbit);
		temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_7_OA_LANE_LPTX_EN_OVR_VAL_BIT + sigbit);
		dphy_writew(temp, addr);

		addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_8 + lane * 0x400;
		temp = dphy_readw(addr);
		temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_8_OA_LANE_LPTX_PON_OVR_EN_BIT);
		temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_8_OA_LANE_LPTX_EN_OVR_EN_BIT);
		dphy_writew(temp, addr);

		addr = ip_base + CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1 + lane * 0x400;
		temp = dphy_readw(addr);
		temp |= BIT(CORE_DIG_IOCTRL_RW_AFE_LANE_CTRL_2_1_OA_LANE_GPO_EN_BIT + sigbit);
		dphy_writew(temp, addr);
	}
}

static void dphyrx0_pin_reg_config(void)
{
	int i, func;
	u32 off, val;

	for (i = 0; i < 12; i++) {
		off = 0xC + i * 0xC;
		val = dphy_readl(pin_dphyrx0[i].pinmux_reg_base + off);
		func = (val & GENMASK(18, 16)) >> 16;
		if ((4 == func) || (6 == func)) {
			dphy_writel(val, pin_dphyrx0[i].pin_dphyrx_reg_base + off);
			dphyrx_pin_dir_config(pin_dphyrx0[i].rxcdphy_reg_base, pin_dphyrx0[i].lane, pin_dphyrx0[i].signal, pin_dphyrx0[i].fun4_6_def_dir);
		}
	}
}

static void dphyrx1_pin_reg_config(void)
{
	int i, func;
	u32 off, val;

	for (i = 0; i < 12; i++) {
		off = 0xC + i * 0xC;
		val = dphy_readl(pin_dphyrx1[i].pinmux_reg_base + off);
		func = (val & GENMASK(18, 16)) >> 16;
		if (6 == func) {
			dphy_writel(val, pin_dphyrx1[i].pin_dphyrx_reg_base + off);
			dphyrx_pin_dir_config(pin_dphyrx1[i].rxcdphy_reg_base, pin_dphyrx1[i].lane, pin_dphyrx1[i].signal, pin_dphyrx1[i].fun4_6_def_dir);
		}
	}
}

static void dphyrx2_pin_reg_config(void)
{
	int i, func;
	u32 off, val;

	for (i = 0; i < 12; i++) {
		off = 0xC + i * 0xC;
		val = dphy_readl(pin_dphyrx2[i].pinmux_reg_base + off);
		func = (val & GENMASK(18, 16)) >> 16;
		if ((4 == func) || (6 == func)) {
			dphy_writel(val, pin_dphyrx2[i].pin_dphyrx_reg_base + off);
			dphyrx_pin_dir_config(pin_dphyrx2[i].rxcdphy_reg_base, pin_dphyrx2[i].lane, pin_dphyrx2[i].signal, pin_dphyrx2[i].fun4_6_def_dir);
		}
	}
}

static void dphyrx3_pin_reg_config(void)
{
	int i, func;
	u32 off, val;

	for (i = 0; i < 12; i++) {
		off = 0xC + i * 0xC;
		val = dphy_readl(pin_dphyrx3[i].pinmux_reg_base + off);
		func = (val & GENMASK(18, 16)) >> 16;
		if ((4 == func) || (6 == func)) {
			dphy_writel(val, pin_dphyrx3[i].pin_dphyrx_reg_base + off);
			dphyrx_pin_dir_config(pin_dphyrx3[i].rxcdphy_reg_base, pin_dphyrx3[i].lane, pin_dphyrx3[i].signal, pin_dphyrx3[i].fun4_6_def_dir);
		}
	}
}

void dphyrx_pin_reg_config(void)
{
	/*shutdown_n_sw*/
	dphy_writel(GENMASK(31, 24), SEN_PHY_GLB_CSI_CTRL_EN_CLR);
#ifdef DPHY_DEBUG
	dphy_readl(SEN_PHY_CK_RST_CFG + 0x4);
	dphy_readl(SEN_PHY_CK_RST_CFG + 0x8);
	dphy_readl(SEN_CK_RST_CFG + 0x8);
	dphy_readl(SEN_CK_RST_CFG + 0xC);
	dphy_readl(SENPHY_GLB + 0x1B8);
#endif

	dphyrx0_pin_reg_config();
	dphyrx1_pin_reg_config();
	dphyrx2_pin_reg_config();
	dphyrx3_pin_reg_config();
}
