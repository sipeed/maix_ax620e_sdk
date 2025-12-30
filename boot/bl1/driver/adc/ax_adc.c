#include "ax_adc.h"
#include "efuse_drv.h"
#define CHIP_BOND_BLK	2
#define THM_TEMP_BLK	3


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

int ax_adc_calibrate_config(void)
{
	int value;
	efuse_read(CHIP_BOND_BLK, &value);
	value = value & 0x1f;
	if(value  == 0) {
		value = 0x7;
	}
	writel(0x0, AX_ADC_RSTN);
	writel(0x0, AX_ADC_MON_EN);
	writel(0, AX_ADC_INT_MASK);
	writel(0x1, AX_THM_MA_CTRL);
	writel(0x1, AX_ADC_BASE);
	value = (value << 5) | 0x18;
	writel(0, AX_ADC_CTRL);
	writel(value, AX_ADC_CTRL);
	writel(0x1, AX_ADC_CLK_EN);
	writel(0x1, AX_ADC_RSTN);
	writel((1 << 14), AX_ADC_VREF_EN);
	udelay(32);
	return 0;
}

int adc_read_boardid(int channel, unsigned int *data)
{
	ax_adc_calibrate_config();
	writel(0x3c00, AX_ADC_MON_CH);
	writel(0x1, AX_ADC_MON_EN);
	udelay(100);

	switch (channel) {
	case 0:
		*data = readl(AX_ADC_DATA_CHANNEL0);
		break;
	case 1:
		*data = readl(AX_ADC_DATA_CHANNEL1);
		break;
	case 2:
		*data = readl(AX_ADC_DATA_CHANNEL2);
		break;
	}
	return 0;
}

int ax_get_board_id(void)
{
	unsigned int board_val;
	adc_read_boardid(0, &board_val);

	return calc_id(board_val);
}

