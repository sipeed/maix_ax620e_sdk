#include "mmc.h"
#include "timer.h"
#include "chip_reg.h"

#if defined(AX620E_EMMC)
#define EMMC_RSTN_GPIO_OFFSET    0X60    //gpio2_A23

void emmc_reset(void)
{
	//JEDEC Standard No. 84-B51, RST_n to Command time min 200us, tRSTW RST_n pulse width min 1us.
	writel(0x2, (GPIO2_BASE + EMMC_RSTN_GPIO_OFFSET));
	udelay(10);
	writel(0x3, (GPIO2_BASE + EMMC_RSTN_GPIO_OFFSET));
	udelay(300);//300us
}
#endif
/*
 *     Function Description: select emmc/sd clk source, set div
 *     Parameter:
 *     Return:
 */
#if 0
#define CLK_EMMC_CLK_CARD_SEL(x)    (((x) & 0x3) << 5)
#define CLK_EMMC_CLK_CARD_DIV(x)    (((x) & 0x3F) << 0)
#define EMMC_CLK_DIV_UPDATE         (0x1 << 6)

#define CLK_SD_CLK_CARD_SEL(x)    (((x) & 0x3) << 16)
#define CLK_SD_CLK_CARD_DIV(x)    (((x) & 0x3F) << 20)
#define SD_CLK_DIV_UPDATE         (0x1 << 26)

void axera_sys_glb_clk_set(CARD_TYPE card_type)
{
	int clk_sel = 0,div = 0;

	if(card_type == CARD_EMMC) {
		clk_sel = 0x3; //source npll_400m
		div = 0x1; //200M supply card
		writel(BIT(2), CPU_SYS_GLB_CLK_EB0_CLR);//close clk_emmc_card_eb
		udelay(1);
		writel(CLK_EMMC_CLK_CARD_SEL(0x3), CPU_SYS_GLB_CLK_MUX0_CLR);//clr clk source bit[6:5]
		writel(CLK_EMMC_CLK_CARD_SEL(clk_sel), CPU_SYS_GLB_CLK_MUX0_SET);//Select clk source bit[6:5]
		writel(BIT(2), CPU_SYS_GLB_CLK_EB0_SET);//open clk_emmc_card_eb
		writel(GENMASK(5, 0), CPU_SYS_GLB_CLK_DIV0_CLR);//clear bit[5:0]
		writel(CLK_EMMC_CLK_CARD_DIV(div), CPU_SYS_GLB_CLK_DIV0_SET);//set div
		writel(EMMC_CLK_DIV_UPDATE, CPU_SYS_GLB_CLK_DIV0_SET);
		udelay(1);
		writel(EMMC_CLK_DIV_UPDATE, CPU_SYS_GLB_CLK_DIV0_CLR);

		//set emmc_card_sw_rst for dll lock
		writel(BIT(11), CPU_SYS_GLB_SW_RST0_SET);
		udelay(1); //Required 1ns
		writel(BIT(11), CPU_SYS_GLB_SW_RST0_CLR);
	}
	if(card_type == CARD_SD) {
		clk_sel = 0x3; //source npll_400m
		div = 0x1; //200M supply card
		writel(BIT(9), FLASH_SYS_GLB_CLK_EB0_CLR);//close clk_emmc_card_eb
		udelay(1);
		writel(CLK_SD_CLK_CARD_SEL(0x3), FLASH_SYS_GLB_CLK_MUX0_CLR);//clr clk source bit[6:5]
		writel(CLK_SD_CLK_CARD_SEL(clk_sel), FLASH_SYS_GLB_CLK_MUX0_SET);//Select clk source bit[6:5]
		writel(BIT(9), FLASH_SYS_GLB_CLK_EB0_SET);//open clk_emmc_card_eb
		writel(GENMASK(25, 20), FLASH_SYS_GLB_CLK_DIV0_CLR);//clear bit[25:20]
		writel(CLK_SD_CLK_CARD_DIV(div), FLASH_SYS_GLB_CLK_DIV0_SET);//set div
		writel(SD_CLK_DIV_UPDATE, FLASH_SYS_GLB_CLK_DIV0_SET);
		udelay(1);
		writel(SD_CLK_DIV_UPDATE, FLASH_SYS_GLB_CLK_DIV0_CLR);

		//set emmc_card_sw_rst for dll lock
		writel(BIT(15), FLASH_SYS_GLB_SW_RST0_SET);
		udelay(1); //Required 1ns
		writel(BIT(15), FLASH_SYS_GLB_SW_RST0_CLR);
	}
}
#endif

#if 0
static FATFS fs;
static FIL fil;

void jump_to_execute(u32 start_addr);
int sdcard_read(u32 offset, u32 size, char *buf);


void sd_fat32_test()
{
	int ret;
	UINT bw;
	#define BUF ((void *)0x40000000)
	#define SIZE 10240
	#define OFFSET 0

	info("sd fat32 test\r\n");

	ret = sd_init(25000000, 1);
	if(ret < 0) {
		err("sd init failed\r\n");
		goto ERROR;
	}
	info("sd init sucess\r\n");

	ret = f_mount(&fs, "0:", 1); //mount first logic partition
	if(ret == FR_NO_FILESYSTEM) {
		err("mount fs failed\r\n");
		goto ERROR;
	}
	info("mount fs success\r\n");

	info("sd read file...\r\n");
#if 0
	ret = sdcard_read(OFFSET, SIZE, BUF);
	if(ret < 0) {
		err("sd read failed\r\n");
		goto ERROR;
	}
#else
	ret = f_open(&fil, "0:boot.bin", FA_OPEN_EXISTING | FA_READ);
	if(ret != FR_OK) {
		err("open file failed\r\n");
		goto ERROR;
	}

	ret = f_lseek(&fil, OFFSET);
	if(ret != FR_OK) {
		f_close(&fil);
		err("lseek file failed\r\n");
		goto ERROR;
	}

	ret = f_read(&fil, BUF, SIZE, &bw);
	if(ret != FR_OK) {
		f_close(&fil);
		err("read file failed\r\n");
		goto ERROR;
	}

	f_close(&fil);
#endif

	info("sd fat32 test sucess\r\n");

ERROR:
	while(1);
}


#ifdef WRITE_TEST
int mmc_write_blocks(void *host, u32 start, u32 blkcnt, const void *src);
int mmc_read_blocks(void *host, void *dst, u32 start, u32 blkcnt);

int mmc_rw_test(void *host)
{
	void *write_addr = (void *)0x40000000;
	void *read_addr = (void *)0x41000000;
	int i, ret;

	for (i = 0; i < 2048; i++) {
		*(u32 *)(write_addr + i * 4) = i;
	}

	info("write data to sd\r\n");
	ret = mmc_write_blocks(host, 0, 16, write_addr);
	if(ret < 0) {
		err("mmc write failed\r\n");
		return -1;
	}

	info("read data from sd\r\n");
	ret = mmc_read_blocks(host, read_addr, 0, 16);
	if(ret < 0) {
		err("mmc read failed\r\n");
		return -1;
	}

	for (i = 0; i < 2048; i++) {
		if (*(u32 *)(write_addr + i * 4) != *(u32 *)(read_addr + i * 4)) {
			err("compare data at offset:%d failed\r\n", i);
			return -1;
		}
	}
	info("sd write/read success\r\n");

	return 0;
}

void sd_raw_test()
{
	int ret;
	void *sdhci_host = (void *)(SD_MST1_BASE + SRS_BASE_OFFSET);

	info("sd raw data rw test\r\n");

	ret = sd_init(25000000, 1);
	if(ret < 0) {
		err("sd init failed\r\n");
		goto ERROR;
	}
	info("sd init sucess\r\n");

	ret = mmc_rw_test(sdhci_host);
	if(ret < 0) {
		err("sd raw test failed\r\n");
		goto ERROR;
	}

	info("sd raw test sucess\r\n");

ERROR:
	while(1);
}
#endif


void sd_boot_test()
{
	int start_addr = 0;

	info("sd boot test\r\n");

	start_addr = flash_boot(FLASH_SD);
	if(start_addr <= 0) {
		err("start addr <= 0\r\n");
		while(1);
	}

	info("\r\njump to spl, addr:%p\r\n", start_addr);
	jump_to_execute(start_addr);

	info("shouldn't excute here\r\n");
	while(1);
}
#endif