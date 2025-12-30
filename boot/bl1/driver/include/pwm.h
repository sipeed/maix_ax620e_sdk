#ifndef __PWM_H_
#define __PWM_H_

typedef unsigned int u32;
typedef unsigned char bool;
#define true 	1
#define false	0

#define CHANNEL_CLK_SEL_FREQ		24000000	/* 24MHZ */

/* pwm id */
#define PWM00	0x0
#define PWM01	0x1
#define PWM02	0x2
#define PWM03	0x3
#define PWM04	0x4
#define PWM05	0x5
#define PWM06	0x6
#define PWM07	0x7
#define PWM08	0x8
#define PWM09	0x9
#define PWM10	0xA
#define PWM11	0xB

#define PINMUXG6_BASE_ADDR	0x2304000
#define PINMUX_PWM_MASK_VAL0 0x7
#define PINMUX_PWM_MASK_OFFSET0	16
#define PINMUX_PWM_VAL0	0x3
#define PINMUX_PWM_VAL_OFFSET0	16
#define PINMUX_PWM00_OFFSET	0xC
#define PINMUX_PWM01_OFFSET	0x18
#define PINMUX_PWM02_OFFSET	0x24
#define PINMUX_PWM03_OFFSET	0x30
#define PINMUX_PWM04_OFFSET	0x84
#define PINMUX_PWM05_OFFSET	0x90
#define PINMUX_PWM06_OFFSET	0x54
#define PINMUX_PWM07_OFFSET	0x60

#define PINMUXG12_BASE_ADDR	0x104F2000
#define PINMUX_PWM_MASK_VAL1 0x7
#define PINMUX_PWM_MASK_OFFSET1	16
#define PINMUX_PWM_VAL1	0x4
#define PINMUX_PWM_VAL_OFFSET1	16
#define PINMUX_PWM08_OFFSET	0xC
#define PINMUX_PWM09_OFFSET	0x18

#define PINMUXG2_BASE_ADDR	0x2301000
#define PINMUX_PWM_MASK_VAL2 0x7
#define PINMUX_PWM_MASK_OFFSET2	16
#define PINMUX_PWM_VAL2	0x2
#define PINMUX_PWM_VAL_OFFSET2	16
#define PINMUX_PWM10_OFFSET	0x30
#define PINMUX_PWM11_OFFSET	0x24

#define PERIPH_SYS_GLB_BASE_ADDR	0x4870000
#define PWM_GLB_CLK_RET_OFFSET	0x1C
#define PWM0_GLB_CLK_RET_VAL		19
#define PWM1_GLB_CLK_RET_VAL		24
#define PWM2_GLB_CLK_RET_VAL		29

#define PWM_GLB_CLK_SEL_OFFSET		0x0
#define PWM_GLB_CLK_SEL_VAL		13

#define PWM_CLK_RET_OFFSET		0x1C
#define PWM00_CLK_RET_VAL		15
#define PWM01_CLK_RET_VAL		16
#define PWM02_CLK_RET_VAL		17
#define PWM03_CLK_RET_VAL		18
#define PWM04_CLK_RET_VAL		20
#define PWM05_CLK_RET_VAL		21
#define PWM06_CLK_RET_VAL		22
#define PWM07_CLK_RET_VAL		23
#define PWM08_CLK_RET_VAL		25
#define PWM09_CLK_RET_VAL		26
#define PWM10_CLK_RET_VAL		27
#define PWM11_CLK_RET_VAL		28

#define PWM_GLB_CLK_GATE_OFFSET		0x04
#define PWM_GLB_CLK_GATE_VAL		9

#define PWM0_GLB_CLK_EB_OFFSET		0xC
#define PWM0_GLB_CLK_EB_VAL		31
#define PWM1_GLB_CLK_EB_OFFSET		0x10
#define PWM1_GLB_CLK_EB_VAL		0
#define PWM2_GLB_CLK_EB_OFFSET		0x10
#define PWM2_GLB_CLK_EB_VAL		1

#define PWM_CLK_EB_OFFSET		0x8
#define PWM00_CLK_EB_VAL		19
#define PWM01_CLK_EB_VAL		20
#define PWM02_CLK_EB_VAL		21
#define PWM03_CLK_EB_VAL		22
#define PWM04_CLK_EB_VAL		23
#define PWM05_CLK_EB_VAL		24
#define PWM06_CLK_EB_VAL		25
#define PWM07_CLK_EB_VAL		26
#define PWM08_CLK_EB_VAL		27
#define PWM09_CLK_EB_VAL		28
#define PWM10_CLK_EB_VAL		29
#define PWM11_CLK_EB_VAL		30


#define PWM0_SET_REG_BASE		0x6060000
#define PWM1_SET_REG_BASE		0x6061000
#define PWM2_SET_REG_BASE		0x6062000
#define PWM_TIMERN_LOADCOUNT_OFF(N)	(0x0 + (N) * 0x14)
#define PWM_TIMERN_CONTROLREG_OFF(N)	(0x8 + (N) * 0x14)
#define PWM_TIMERN_LOADCOUNT2_OFF(N)	(0xB0 + (N) * 0x4)
#define PWM_TIMERN_MODE			0x1E		/* PWM mode but not enable */
#define PWM_TIMERN_EN			0x1		/* PWM enable bit */
#define PWM_CHANNEL_0			0
#define PWM_CHANNEL_1			1
#define PWM_CHANNEL_2			2
#define PWM_CHANNEL_3			3

struct device_info
{
	unsigned int pwm_base;	/* pwm register base addr */
	unsigned char channel_id;	/* pwm channel id, start with 0 */
	unsigned char pinmux_shift;	/* pinmux reg offset */
	unsigned char clk_rst_val;	/* clk rst reg val */
	unsigned char clk_eb_val;	/* clk eb reg val */
};

struct volt_duty_transfer
{
	u32 sub;
	u32 scale;
	u32 div;
};

/*
 * pwm_id: the pwm channel which used to change cpu voltage.
 * voltage: voltage = cpu real voltage * 1000
*/
void pwm_volt_config(int pwm_id, int voltage);

#endif
