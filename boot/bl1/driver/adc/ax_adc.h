#ifndef _AX_ADC_H_
#define _AX_ADC_H_

#include "efuse_drv.h"
#include "cmn.h"
#include "timer.h"

#define AX_ADC_BASE		0x2000000
#define AX_ADC_MA_EN		0x2000004
#define AX_ADC_MA_POR_EN	0x2000008
#define AX_ADC_MA_CTRL		0x2000010
#define AX_THM_MA_CTRL		0x200000C
#define AX_ADC_MA_POR_CTRL	0x2000014
#define AX_ADC_CTRL		0x2000018
#define AX_ADC_CLK_EN		0x200001C	/*clk enable */
#define AX_ADC_CLK_SELECT	0x2000020	/*clk select */
#define AX_ADC_RSTN		0x2000024
#define AX_ADC_VREF_EN		0x2000038
#define AX_ADC_MON_EN		0x20000C8
#define AX_ADC_MON_CH		0x20000C4
#define AX_ADC_MON_INTERVAL	0x20000CC

#define AX_ADC_DATA_CHANNEL0	0x20000A0
#define AX_ADC_DATA_CHANNEL1	0x20000A4
#define AX_ADC_DATA_CHANNEL2	0x20000A8
#define AX_ADC_DATA_CHANNEL3	0x20000AC

#define AX_ADC_INT_MASK		0x2000104
#define AX_ADC_SEL(x)		(1 << (10 + x))
#define AX_ADC_SEL_EN		BIT(0)
int adc_read_boardid(int channel, unsigned int *data);
int ax_get_board_id(void);
#endif
