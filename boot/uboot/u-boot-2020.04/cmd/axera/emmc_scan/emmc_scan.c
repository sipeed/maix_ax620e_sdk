#include <common.h>
#include <asm/io.h>
#include <sdhci.h>
#include <malloc.h>
#include <asm/arch/ax620e.h>
#include <linux/bitfield.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <asm/arch/boot_mode.h>

#define REG_DUMP_FILE_DISPLAY (0)

#define _REG32_WRITE(addr, data) *((volatile AX_U32 *)(addr)) = data
#define _REG32_READ(addr) *((volatile AX_U32 *)(addr))
#define PAGE_SIZE   (4096)
#define BYTE_SIZE   (16)
#define ALIGN_DATA(a, s)    ((a) & ~((s) - 1))
#define ALIGN_SIZE(w, len)  ((w + (len -1)) & (~(len - 1)))

#define REG_OFFSET(a, l)    ((a) - (l))
#define EMMC_REG_BASE_PHY 0x1B40000
#define REG_SIZE 0x1000

#define SDHCI_CDNS_HRS04        0x10        /* PHY access port */

/* PHY */
#define SDHCI_CDNS_PHY_DLY_SD_HS	0x00
#define SDHCI_CDNS_PHY_DLY_SD_DEFAULT	0x01
#define SDHCI_CDNS_PHY_DLY_UHS_SDR12	0x02
#define SDHCI_CDNS_PHY_DLY_UHS_SDR25	0x03
#define SDHCI_CDNS_PHY_DLY_UHS_SDR50	0x04
#define SDHCI_CDNS_PHY_DLY_UHS_DDR50	0x05
#define SDHCI_CDNS_PHY_DLY_EMMC_LEGACY	0x06
#define SDHCI_CDNS_PHY_DLY_EMMC_SDR	0x07
#define SDHCI_CDNS_PHY_DLY_EMMC_DDR	0x08
#define SDHCI_CDNS_PHY_DLY_SDCLK	0x0b
#define SDHCI_CDNS_PHY_DLY_HSMMC	0x0c
#define SDHCI_CDNS_PHY_DLY_STROBE	0x0d

#define SDHCI_CDNS_HRS04_WR         (1 << 24)
#define SDHCI_CDNS_HRS04_RD         (1 << 25)
#define SDHCI_CDNS_HRS04_ACK (1 << 26)
#define SDHCI_CDNS_PHY_DLY_SD_DEFAULT   0x01
#define SDHIC_MODE_DELAY_NUM 32
#define SDHIC_MASTER_DELAY_NUM 128

#define SCAN_STEP 1
typedef unsigned long long int  AX_U64;
typedef unsigned int            AX_U32;
typedef unsigned short          AX_U16;
typedef unsigned char           AX_U8;
typedef long long int           AX_S64;
typedef int                     AX_S32;
typedef short                   AX_S16;
typedef signed char             AX_S8;
typedef char                    AX_CHAR;
typedef long                    AX_LONG;
typedef unsigned long           AX_ULONG;
typedef unsigned long           AX_ADDR;
typedef float                   AX_F32;
typedef double                  AX_F64;
typedef void                    AX_VOID;
typedef unsigned int            AX_SIZE_T;

enum mode {
	EMMC_LEGACY,
	EMMC_SDR,
};
static enum mode current_mode;

static int curr_device = 0;
static u32 reg_handle_mem;
static struct mmc *init_mmc_device(int dev, bool force_init)
{
	struct mmc *mmc;
	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("no mmc device at slot %x\n", dev);
		return NULL;
	}

	if (!mmc_getcd(mmc))
		force_init = true;

	if (force_init)
		mmc->has_init = 0;
	if (mmc_init(mmc))
		return NULL;

#ifdef CONFIG_BLOCK_CACHE
	struct blk_desc *bd = mmc_get_blk_desc(mmc);
	blkcache_invalidate(bd->if_type, bd->devnum);
#endif

	return mmc;
}

static int do_mmc_read(u32 blk, u32 cnt, void *addr)
{
	struct mmc *mmc;
	u32 n;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc)
		return CMD_RET_FAILURE;

	printf("\nMMC read: dev # %d, block # %d, count %d ... ",
	       curr_device, blk, cnt);

	n = blk_dread(mmc_get_blk_desc(mmc), blk, cnt, addr);
	printf("%d blocks read: %s\n", n, (n == cnt) ? "OK" : "ERROR");

	return (n == cnt) ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int do_mmc_write(u32 blk, u32 cnt, void *addr)
{
	struct mmc *mmc;
	u32  n;

	mmc = init_mmc_device(curr_device, false);
	if (!mmc)
		return CMD_RET_FAILURE;

	printf("\nMMC write: dev # %d, block # %d, count %d ... ",
	       curr_device, blk, cnt);

	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return CMD_RET_FAILURE;
	}
	n = blk_dwrite(mmc_get_blk_desc(mmc), blk, cnt, addr);
	printf("%d blocks written: %s\n", n, (n == cnt) ? "OK" : "ERROR");

	return (n == cnt) ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}


static int phy_reg_get(int addr)
{
	AX_U32 tmp;
	AX_U32 reg = (AX_U32)reg_handle_mem + SDHCI_CDNS_HRS04;
	tmp = _REG32_READ(reg);
	tmp = tmp & 0xffffff00;
	tmp = tmp | addr;
	_REG32_WRITE(reg, tmp);
	tmp = tmp | SDHCI_CDNS_HRS04_RD;
	_REG32_WRITE(reg, tmp);
	while (1) {
		tmp = _REG32_READ(reg);
		if (tmp & SDHCI_CDNS_HRS04_ACK) {
			break;
		}
		udelay(1000);
	}
	_REG32_WRITE(reg, 0);
	tmp = tmp & (~(SDHCI_CDNS_HRS04_RD | SDHCI_CDNS_HRS04_ACK | SDHCI_CDNS_HRS04_WR));
	return (tmp >> 16);
}
#define   SDHCI_CDNS_HRS04_RDATA		GENMASK(23, 16)
#define   SDHCI_CDNS_HRS04_WDATA		GENMASK(15, 8)
#define   SDHCI_CDNS_HRS04_ADDR			GENMASK(5, 0)
static int phy_reg_set(int addr, AX_U8 data)
{
	void __iomem *reg = (void __iomem *)(reg_handle_mem + SDHCI_CDNS_HRS04);
	u32 tmp;
	int ret;
	tmp = FIELD_PREP(SDHCI_CDNS_HRS04_WDATA, data) |
	      FIELD_PREP(SDHCI_CDNS_HRS04_ADDR, addr);
	writel(tmp, reg);
	tmp |= SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, tmp & SDHCI_CDNS_HRS04_ACK, 10);
	if (ret)
		return ret;

	tmp &= ~SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);
	return 0;

}
#define READ_TEST_SIZE  0x800 //2M
#define READ_TEST_ADDR1 0x50000000
#define READ_TEST_ADDR2 (READ_TEST_ADDR1 + READ_TEST_SIZE)
#define READ_TEST_ADDR3 (READ_TEST_ADDR2 + READ_TEST_SIZE)
#define READ_TEST_ADDR4 (READ_TEST_ADDR3 + READ_TEST_SIZE)

static void emmc_run_test_read(void)
{
	/* test address */
	void *read_addr1 = (void *)READ_TEST_ADDR1;
	void *read_addr2 = (void *)READ_TEST_ADDR2;
	printf("Note: This test program may damage the uboot and param partitions. If it is damaged, please burn the version again.\n");
	/* read position in 1G to test */
	do_mmc_read(0x200000, READ_TEST_SIZE, read_addr1);
	do_mmc_read(0x200000, READ_TEST_SIZE, read_addr2);
	if(0 == memcmp(read_addr1, read_addr2, READ_TEST_SIZE)) {
		printf("read data compare success\n");
	} else {
		printf("read data compare failed\n");
	}
}
static void emmc_run_test_write(void)
{
	/* test address */
	void *read_addr3 = (void *)READ_TEST_ADDR3;
	void *read_addr4 = (void *)READ_TEST_ADDR4;

	/* write position to 1G test. */
	do_mmc_write(0x200000, READ_TEST_SIZE, read_addr3);
	do_mmc_write(0x200000, READ_TEST_SIZE, read_addr4);

}
static void printf_reg(void)
{
	printf("===============当前寄存器配置=======================\n");
	printf("dll count: %d\n",phy_reg_get(9));
	printf("SDHCI_CDNS_PHY_DLY_SD_HS: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_SD_HS));
	printf("SDHCI_CDNS_PHY_DLY_SD_DEFAULT: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_SD_DEFAULT));
	printf("SDHCI_CDNS_PHY_DLY_UHS_SDR12: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_UHS_SDR12));
	printf("SDHCI_CDNS_PHY_DLY_UHS_SDR25: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_UHS_SDR25));
	printf("SDHCI_CDNS_PHY_DLY_UHS_SDR50: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_UHS_SDR50));
	printf("SDHCI_CDNS_PHY_DLY_UHS_DDR50: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_UHS_DDR50));
	printf("SDHCI_CDNS_PHY_DLY_EMMC_LEGACY: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_EMMC_LEGACY));
	printf("SDHCI_CDNS_PHY_DLY_EMMC_SDR: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_EMMC_SDR));
	printf("SDHCI_CDNS_PHY_DLY_EMMC_DDR: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_EMMC_DDR));
	printf("SDHCI_CDNS_PHY_DLY_SDCLK: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_SDCLK));
	printf("SDHCI_CDNS_PHY_DLY_HSMMC: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_HSMMC));
	printf("SDHCI_CDNS_PHY_DLY_STROBE: %d\n",phy_reg_get(SDHCI_CDNS_PHY_DLY_STROBE));
	printf("=====================================================\n");
}
static void use_print(void)
{
	printf("==========================================================================================================\n\n");
	printf("====参数1:表示要测试的模式,可测试的模式如下：\n");
	printf(" 0:EMMC_LEGACY; 1:EMMC_SDR; \n\n");
	printf("====参数2:表示要测试读还是写, 0代表读, 1代表写\n");
	printf("====参数3:表示fix delay value\n");
	printf("终端执行: emmc_scan 1 1      表示测试EMMC HS mode的写\n");
	printf("终端执行: emmc_scan 1 0 4    表示测试EMMC HS mode的读\n");
	printf("==========================================================================================================\n\n");

}

static void emmc_delay_scan(enum mode s_mode, int flag, int fix_delay)
{
	int i,j;
	int ret;
	printf("start emmc_delay_scan\n");
	printf_reg();
	if(s_mode == EMMC_LEGACY && flag == 1) { //EMMC LEGACY write test
		printf("emmc EMMC_LEGACY write scan test\n");
		for(i = 0; i < SDHIC_MASTER_DELAY_NUM; i += SCAN_STEP) {
			phy_reg_set(SDHCI_CDNS_PHY_DLY_SDCLK, i);
			emmc_run_test_write();
			printf_reg();
		}
		printf("If write data no err, please set SDHCI_CDNS_PHY_DLY_SDCLK = 0\n");
		printf("If write data err, Please check the hardware and data sending waveform.\n");
	}
	if(s_mode == EMMC_LEGACY && flag == 0) { //EMMC LEGACY read test
		printf("emmc EMMC_LEGACY read scan, fix_delay:%d\n",fix_delay);
		for(j = 0; j < 32; j++) {
			if(fix_delay != 0xAA) {
				ret = phy_reg_set(SDHCI_CDNS_PHY_DLY_EMMC_LEGACY, fix_delay);
				if(ret)
					printf("set phy param failed\n");
			} else {
				ret = phy_reg_set(SDHCI_CDNS_PHY_DLY_EMMC_LEGACY, j);
				if(ret)
					printf("set phy param failed\n");
				emmc_run_test_read();
				printf_reg();
			}
			if(fix_delay != 0xAA) {
				for (i = 0; i < 64; i += SCAN_STEP) {
					phy_reg_set(SDHCI_CDNS_PHY_DLY_SDCLK, i);
					emmc_run_test_read();
					printf_reg();
				}
				break;
			}
		}
	}
	if(s_mode == EMMC_SDR &&  flag == 1) { //EMMC HIGH SPEED MODE write test
		printf("emmc EMMC_SDR write scan test\n");
		for(i = 0; i < SDHIC_MASTER_DELAY_NUM; i += SCAN_STEP) {
			phy_reg_set(SDHCI_CDNS_PHY_DLY_SDCLK, 60);
			emmc_run_test_write();
			printf_reg();
		}
		printf("If write data no err, please set SDHCI_CDNS_PHY_DLY_SDCLK = 0\n");
		printf("If write data err, Please check the hardware and data sending waveform.\n");
	}
	if(s_mode == EMMC_SDR &&  flag == 0) { //EMMC HIGH SPEED MODE read test
		printf("emmc EMMC_SDR read scan ,fix_delay:%d\n",fix_delay);
		for(j = 0; j < 32; j++) {
			phy_reg_set(SDHCI_CDNS_PHY_DLY_SDCLK, 0);
			if(fix_delay != 0xAA) {
				ret = phy_reg_set(SDHCI_CDNS_PHY_DLY_EMMC_SDR, fix_delay);
				if(ret)
					printf("set phy param failed\n");
			} else {
				ret = phy_reg_set(SDHCI_CDNS_PHY_DLY_EMMC_SDR, j);
				if(ret)
					printf("set phy param failed\n");
				emmc_run_test_read();
				printf_reg();
			}
			if(fix_delay != 0xAA) {
				// for (i = 0; i < 64; i += SCAN_STEP) {
				// 	phy_reg_set(SDHCI_CDNS_PHY_DLY_SDCLK, i);
				emmc_run_test_read();
				printf_reg();
				// }
				break;
			}
		}
	}
}

int do_emmc_scan(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	AX_U32 tmp;
	AX_U32 read_write_flag;
	int fix_delay;

	if(argc >= 3) {
		reg_handle_mem = EMMC_REG_BASE_PHY;

		printf("reg_base:0x%x\n",reg_handle_mem);

		switch(simple_strtoul(argv[1], NULL, 16)) {
		case 0:
			current_mode = EMMC_LEGACY;
			break;
		case 1:
			current_mode = EMMC_SDR;
			break;

		default:
			break;
		}
		if(simple_strtoul(argv[1], NULL, 16) < 0 && simple_strtoul(argv[1], NULL, 16) > 8) {
			printf("mode input err\n");
			return 0;
		}
		read_write_flag = simple_strtoul(argv[2], NULL, 16); // 0是读，1是写
		if(read_write_flag != 0 && read_write_flag != 1) {
			printf("read_write_flag input err\n");
			return 0;
		}
		printf("%s: simple_strtoul(argv[4], NULL, 16) = %ld\n",__func__, simple_strtoul(argv[3], NULL, 16));

		if(simple_strtoul(argv[3], NULL, 16) > 0)
			fix_delay = simple_strtoul(argv[3], NULL, 16);
		else
			fix_delay = 0xAA;
		tmp = phy_reg_get(9);
		if(tmp & 0x80)
			printf("%s: phy lock successful\n",__func__);

		emmc_delay_scan(current_mode, read_write_flag, fix_delay); //atoi(argv[3]: 0是读，1是写

	} else {
		use_print();
	}
	return 0;
}

U_BOOT_CMD(
    emmc_scan, 4, 0, do_emmc_scan,
    "integrated emmc scan",
    "<addr> <len> [loops]"
);

