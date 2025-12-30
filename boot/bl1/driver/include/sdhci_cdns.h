#ifndef SDHCI_DRIVER_H
#define SDHCI_DRIVER_H

#include "mmc.h"
#include "cmn.h"

#define EMMC_BASE               0x1B40000
#define SD_MST1_BASE            0x104E0000 //SDIO_MST1_BASE          0x104D0000
#define HRS_BASE_OFFSET         0
#define SRS_BASE_OFFSET         0x200

#define INIT_CLK_400K                           (400000)
#define INIT_CLK_300K                           (300000)
#define INIT_CLK_200K                           (200000)
#define INIT_CLK_100K                           (100000)
#define DEFAULT_SD_CLK                          (10000000)
#define DEFAULT_EMMC_CLK                        (10000000)
#define LEGACY_EMMC_CLK                         (25000000)
#define HS400_ES_EMMC_CLK                       (200000000)
#define HS400_EMMC_CLK                          (200000000)
#define HS200_EMMC_CLK                          (200000000)
#define HS_EMMC_CLK                             (50000000)
#define HS_SD_CLK                               (50000000)

#define SDHCI_MAX_DIV_SPEC_300	2046

#define CMD_RESP_TYPE_NO_RESP          0
#define CMD_RESP_TYPE_RESP_LEN_136     1
#define CMD_RESP_TYPE_RESP_LEN_48      2
#define CMD_RESP_TYPE_RESP_LEN_48B     3

#define CMD_TYPE_ABORT                 3

#define SDHCI_INT_ALL_MASK          0xFFFFFFFF

#define DEFAULT_BOUNDARY_ARG           7  // buffer boundary
#define DEFAULT_BOUNDARY_SIZE          (512 * 1024)  // boundary size

#define SD_HOST_VERSION                 0x2    //VER_3_0
#define SPEC_VERSION_NUM_MASK           0xFF

#define SDHCI_SDMASA_R                0x0     //size 32bits
#define SDHCI_BLOCKSIZE_R             0x4     //size 16bits
#define SDHCI_BLOCKCOUNT_R            0x6     //size 16bits
#define SDHCI_ARGUMENT_R              0x8     //size 32bits
#define SDHCI_XFER_MODE_R             0xc     //size 16bits
#define SDHCI_CMD_R                   0xe     //size 16bits
#define SDHCI_RESP01_R                0x10    //size 32bits
#define SDHCI_RESP23_R                0x14    //size 32bits
#define SDHCI_RESP45_R                0x18    //size 32bits
#define SDHCI_RESP67_R                0x1c    //size 32bits
#define SDHCI_BUF_DATA_R              0x20    //size 32bits
#define SDHCI_HOST_CTRL1_R            0x28    //size 8bits
#define SDHCI_PWR_CTRL_R              0x29    //size 8bits
#define SDHCI_PSTATE_REG              0x24    //size 32bits
#define SDHCI_CLK_CTRL_R              0x2c    //size 16bits
#define SDHCI_SW_RST_R                0x2f    //size 8bits
#define SDHCI_NORMAL_INT_STAT_R       0x30    //size 16bits
#define SDHCI_ERROR_INT_STAT_R        0x32    //size 16bits
#define SDHCI_TOUT_CTRL_R             0x2e    //size 8bits
#define SDHCI_NORMAL_INT_STAT_EN_R    0x34    //size 16bits
#define SDHCI_NORMAL_INT_SIGNAL_EN_R  0x38    //size 16bits
#define SDHCI_HOST_CTRL2_R            0x3e    //size 16bits
#define SDHCI_P_VENDOR_SPECIFIC_AREA  0xe8    //size 16bits
#define SDHCI_CAPABILITIES1_R         0x40    //size 32bits
#define SDHCI_CAPABILITIES2_R         0x44    //size 32bits
#define EMMC_CTRL_R                      0x2c    //size 16bits, should SDHCI_BASE + P_VENDOR_SPECIFIC_AREA[11:0] + 0x2c
#define ADMA_SA_LOW_R                    0x58    //size 32bits
#define ADMA_SA_HIGH_R                   0x5C    //size 32bits
#define HOST_CNTRL_VERS_R                0xfe    //size 16bits
#define SDHCI_BOOT_STATUS                0x90    //size 5bits
#define SDHCI_AXI_ERROR_RESP             0xC     //size 19bits

#define SDHCI_CTRL_UHS_MASK	0x0007
#define MMC_TIMING_MMC_LEGACY   0x1
#define MMC_TIMING_MMC_HS       0x2
#define MMC_TIMING_MMC_HS200    0x4
#define MMC_TIMING_MMC_HS400    0x5
#define MMC_TIMING_MMC_HS400_ES 0x6


#define EMMC_PHY_BASE           0x300
#define EMMC_PHY_CNFG           (EMMC_PHY_BASE + 0x0)     //32bits
#define EMMC_PHY_CMDPAD_CNFG    (EMMC_PHY_BASE + 0x4)     //16bits
#define EMMC_PHY_DATPAD_CNFG    (EMMC_PHY_BASE + 0x6)     //16bits
#define EMMC_PHY_CLKPAD_CNFG    (EMMC_PHY_BASE + 0x8)     //16bits
#define EMMC_PHY_STBPAD_CNFG    (EMMC_PHY_BASE + 0xA)     //16bits
#define EMMC_PHY_RSTNPAD_CNFG   (EMMC_PHY_BASE + 0xC)     //16bits
#define EMMC_PHY_COMMDL_CNFG    (EMMC_PHY_BASE + 0x1C)    //8bits
#define EMMC_PHY_SDCLKDL_CNFG   (EMMC_PHY_BASE + 0x1D)    //8bits
#define EMMC_PHY_SDCLKDL_DC     (EMMC_PHY_BASE + 0x1E)    //8bits
#define EMMC_PHY_SMPLDL_CNFG    (EMMC_PHY_BASE + 0x20)    //8bits
#define EMMC_PHY_ATDL_CNFG      (EMMC_PHY_BASE + 0x21)    //8bits

/* EMMC_PHY_CNFG */
#define BITS_EMMC_PHY_CNFG_PAD_SN(x)       (((x) & 0xF) << 20)
#define BITS_EMMC_PHY_CNFG_PAD_SP(x)       (((x) & 0xF) << 16)
#define BIT_EMMC_PHY_CNFG_PHY_PWRGOOD       BIT(1)
#define BIT_EMMC_PHY_CNFG_PHY_RSTN          BIT(0)

#define BITS_TXSLEW_CTRL_N(x)       (((x) & 0xF) << 9)
#define BITS_TXSLEW_CTRL_P(x)       (((x) & 0xF) << 5)
#define BITS_WEAKPULL_EN(x)         (((x) & 0x3) << 3)
#define BITS_RXSEL_EN(x)            (((x) & 0x7) << 0)


/* SDHCI_BLOCKSIZE_R */
#define BITS_SDHCI_BLOCKSIZE_SDMA_BUF_BDARY(x)    (((x) & 0x7) << 12)
#define BITS_SDHCI_BLOCKSIZE_XFER_BLOCK_SIZE(x)   (((x) & 0xFFF) << 0)

/* SDHCI_XFER_MODE_R */
#define BIT_SDHCI_XFER_MODE_RESP_INT_DISABLE       BIT(8)
#define BIT_SDHCI_XFER_MODE_RESP_ERR_CHK_ENABLE    BIT(7)
#define BIT_SDHCI_XFER_MODE_RESP_TYPE              BIT(6)
#define BIT_SDHCI_XFER_MODE_MULTI_BLK_SEL          BIT(5)
#define BIT_SDHCI_XFER_MODE_DATA_XFER_DIR          BIT(4)
#define BITS_SDHCI_XFER_MODE_AUTO_CMD_ENABLE(x)    ((x) & 0x3) << 2)
#define BIT_SDHCI_XFER_MODE_BLOCK_COUNT_ENABLE     BIT(1)
#define BIT_SDHCI_XFER_MODE_DMA_ENABLE             BIT(0)

/* SDHCI_CMD_R */
#define BITS_SDHCI_CMD_INDEX(x)                    (((x) & 0x3F) << 8)
#define BITS_SDHCI_CMD_TYPE(x)                     (((x) & 0x3) << 6)
#define BIT_SDHCI_CMD_DATA_PRESENT_SEL             BIT(5)
#define BIT_SDHCI_CMD_INDEX_CHK_ENABLE             BIT(4)
#define BIT_SDHCI_CMD_CRC_CHK_ENABLE               BIT(3)
#define BIT_SDHCI_CMD_SUB_CMD_FLAG                 BIT(2)
#define BITS_SDHCI_CMD_RESP_TYPE_SELECT(x)         (((x) & 0x3) << 0)

/* define SDHCI_HOST_CTRL1_R */
#define BITS_SDHCI_HOST_CTRL1_DMA_SEL(x)           (((x) & 0x3) << 3)
#define SDHCI_CTRL_4BITBUS                         0x02
#define SDHCI_CTRL_8BITBUS                         0x20
#define SDHCI_CTRL_HISPD                           0x04
#define SDHCI_CTRL_DMA_MASK                        0x18

/* SDHCI_PWR_CTRL_R */
#define BITS_SDHCI_PWR_CTRL_SD_BUS_VOL_VDD2(x)     (((x) & 0x7) << 5)
#define BIT_SDHCI_PWR_CTRL_SD_BUS_PWR_VDD2         BIT(4)
#define BITS_SDHCI_PWR_CTRL_SD_BUS_VOL_VDD1(x)     (((x) & 0x7) << 1)
#define  SDHCI_POWER_ON		0x01
#define  SDHCI_POWER_180	0x0A
#define  SDHCI_POWER_300	0x0C
#define  SDHCI_POWER_330	0x0E
#define BIT_SDHCI_PWR_CTRL_SD_BUS_PWR_VDD1         BIT(0)

/* SDHCI_PSTATE_REG */
#define BIT_SDHCI_PSTATE_CARD_INSERTED             BIT(16)
#define BIT_SDHCI_PSTATE_BUF_RD_ENABLE             BIT(11)
#define BIT_SDHCI_PSTATE_BUF_WR_ENABLE             BIT(10)
#define SDHCI_CMD_INHIBIT                          BIT(0)
#define SDHCI_DATA_INHIBIT                         BIT(1)

/* SDHCI_CLK_CTRL_R */
#define BITS_SDHCI_CLK_CTRL_FREQ_SEL(x)            (((x) & 0xFF) << 8)
#define BIT_SDHCI_CLK_CTRL_CLK_GEN_SELECT          BIT(5)
#define BIT_SDHCI_CLK_CTRL_PLL_ENABLE              BIT(3)
#define SDHCI_CLOCK_CARD_EN	BIT(2)
#define SDHCI_CLOCK_INT_STABLE	BIT(1)
#define SDHCI_CLOCK_INT_EN	BIT(0)
#define SDHCI_DIV_MASK	0xFF
#define SDHCI_DIVIDER_SHIFT	8
#define SDHCI_DIVIDER_HI_SHIFT	6
#define SDHCI_DIV_HI_MASK	0x300
#define SDHCI_DIV_MASK_LEN	8

/* SDHCI_SW_RST_R */
#define BIT_SDHCI_SW_RST_DAT                       BIT(2)
#define BIT_SDHCI_SW_RST_CMD                       BIT(1)
#define BIT_SDHCI_RESET_ALL                        BIT(0)

/* SDHCI_NORMAL_INT_STAT_R */
#define BIT_SDHCI_NORMAL_INT_ERR_INTERRUPT         BIT(15)
#define BIT_SDHCI_NORMAL_INT_CARD_REMOVAL          BIT(7)
#define BIT_SDHCI_NORMAL_INT_CARD_INSERTION        BIT(6)
#define BIT_SDHCI_NORMAL_INT_BUF_RD_READY          BIT(5)
#define BIT_SDHCI_NORMAL_INT_BUF_WR_READY          BIT(4)
#define BIT_SDHCI_NORMAL_INT_DMA_INTERRUPT         BIT(3)
#define BIT_SDHCI_NORMAL_INT_STAT_XFER_COMPLETE    BIT(1)
#define BIT_SDHCI_NORMAL_INT_STAT_CMD_COMPLETE     BIT(0)

/* SDHCI_TOUT_CTRL_R */
#define BITS_SDHCI_TOUT_CTRL_RVSD(x)               (((x) & 0xF) << 4)
#define BITS_SDHCI_TOUT_CTRL_TOUT_CNT(x)           (((x) & 0xF) << 0)

/* SDHCI_NORMAL_INT_STAT_EN_R */
#define BIT_SDHCI_NORMAL_INT_CARD_REMOVAL_STAT_EN    BIT(7)
#define BIT_SDHCI_NORMAL_INT_CARD_INSERTION_STAT_EN  BIT(6)

/* SDHCI_NORMAL_INT_SIGNAL_EN_R */
#define BIT_SDHCI_NORMAL_INT_CARD_REMOVAL_SIGNAL_EN    BIT(7)
#define BIT_SDHCI_NORMAL_INT_CARD_INSERTION_SIGNAL_EN  BIT(6)

/* SDHCI_HOST_CTRL2_R */
#define BIT_SDHCI_HOST_CTRL2_PRESENT_VAL_ENABLE     BIT(15)
#define BIT_SDHCI_HOST_CTRL2_ASYNC_INT_ENABLE       BIT(14)
#define BIT_SDHCI_HOST_CTRL2_ADDRESSING             BIT(13)
#define BIT_SDHCI_HOST_CTRL2_HOST_VER4_ENABLE       BIT(12)
#define BIT_SDHCI_HOST_CTRL2_CMD23_ENABLE           BIT(11)
#define BIT_SDHCI_HOST_CTRL2_ADMA2_LEN_MODE         BIT(10)
#define BIT_SDHCI_HOST_CTRL2_RSVD_9                 BIT(9)
#define BIT_SDHCI_HOST_CTRL2_UHS2_IF_ENALBE         BIT(8)
#define BIT_SDHCI_HOST_CTRL2_SAMPLE_CLK_SEL         BIT(7)
#define BIT_SDHCI_HOST_CTRL2_EXEC_TUNING            BIT(6)
#define BITS_SDHCI_HOST_CTRL2_DRV_STRENGTH_SEL(x)   (((x) & 0x3) << 4)
#define BIT_SDHCI_HOST_CTRL2_SIGNALING_EN           BIT(3)
#define SDHCI_CTRL_VDD_180		0x0008
#define BITS_SDHCI_HOST_CTRL2_UHS_MODE_SEL(x)       (((x) & 0x7) << 0)

/* SDHCI_P_VENDOR_SPECIFIC_AREA */
#define BITS_SDHCI_P_VENDOR_SPECIFIC_AREA_RVSD(x)               (((x) & 0xF) << 12)
#define BITS_SDHCI_P_VENDOR_SPECIFIC_AREA_REG_OFFSET_ADDR(x)    (((x) & 0xFFF) << 0)

/* SDHCI_CAPABILITIES1_R */
#define BITS_SDHCI_CAPABILITIES1_ASY_INT_SUPPORT          BIT(29)
#define BITS_SDHCI_CAPABILITIES1_BASE_CLK_FREQ(x)               (((x) & 0xFF) << 8)
#define BIT_SDHCI_CAPABILITIES1_SYS_ADDR_64_V4            BIT(27)

/* SDHCI_CAPABILITIES2_R */
#define BITS_SDHCI_CAPABILITIES2_CLK_MUL(x)               (((x) & 0xFF) << 16)

/* EMMC_CTRL_R */
#define BITS_SDHCI_EMMC_CTRL_R_RVSD(x)              (((x) & 0x1F) << 11)
#define BIT_SDHCI_EMMC_CTRL_CQE_PREFETCH_DISABLE    BIT(10)
#define BIT_SDHCI_EMMC_CTRL_CQE_ALGO_SEL            BIT(9)
#define BIT_SDHCI_EMMC_CTRL_ENH_STROBE_ENABLE       BIT(8)
#define BIT_SDHCI_EMMC_CTRL_EMMC_RST_N_OE           BIT(3)
#define BIT_SDHCI_EMMC_CTRL_EMMC_RST_N              BIT(2)
#define BIT_SDHCI_EMMC_CTRL_DISABLE_DATA_CRC_CHK    BIT(1)
#define BIT_SDHCI_EMMC_CTRL_CARD_IS_EMMC            BIT(0)

int sdhci_host_start(void *host, CARD_TYPE card_type);
void mmc_select_mode(void *host, enum bus_mode mode);
int sdhci_cdns_set_tune_val(void *host, unsigned int val);
int axera_sdhci_set_clock(void *host, CARD_TYPE card_type, unsigned int clock);
int sdhci_host_init(void *host, CARD_TYPE card_type);
void sdhci_set_bus_width(void *host, int width);
int sdhci_send_cmd(void *host, struct mmc_cmd *cmd, struct mmc_data *data);
int sdhci_card_detection(void *host);
int sdhci_phy_init(void *hrs_addr, CARD_TYPE card_type);
void sdhci_set_power(void *host, CARD_TYPE card_type);
int sdhci_set_clock(void *host, unsigned int clock, unsigned int base_clock);
int update_delayline_setting(void *host);
#endif
