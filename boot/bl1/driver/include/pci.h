#ifndef __PCI_H__
#define __PCI_H__

#include "cmn.h"
#include "boot.h"

/*
 * common register clk/reset
 */
#define GLB_LTSSM_REG_SET		0x30001100	/* bit[4]=1 is enable LTSSM */
#define GLB_LTSSM_REG_CLR		0x30002100	/* bit[4]=1 is clear LTSSM */
#define GLB_PWR_BUTTON_RST_SET		0x3000110c	/* bit[15:14]=1 is power_up_rst and button_rst_n */
#define GLB_DEVICE_TYPE			0x3000210c	/* bit[3:0]=0x0 is EP mode */

#define GLB_AUX_CLK_PCIE_SET		0x30010020	/* bit[0]=1 to set aux_clk_pcie */
#define GLB_PCIE0_PCIE1_SW_RST_CLR	0x3001003c	/* bit[5]=1 to set pcie0_sw_rst */
#define GLB_APP_HLOD_PHY_RST_CLR	0x30002110	/* bit[14]=1 set enable linktraing */
#define PHY0_CSR_REF_SSP_EN_SET		0x30701018	/* bit[24]=1 set ref ssp enable */
#define PHY0_CSR_REF_USE_PAD		0x30701018	/* bit[18]=1 ref use pad */
#define PHY1_CSR_REF_USE_PAD		0x3070101c	/* bit[18]=1 ref use pad */
#define PHY1_CSR_REF_SSP_EN_SET		0x3070101c	/* bit[24]=1 set ref ssp enable */
#define	PHY_MODEL_SEL_SET		0x30701010	/* bit[1:0]=2 is pcie0 mode */
#define	PHY0_CSR_TEST_POWERDOWN_CLR	0x30702018	/* bit[4]=1 clr phy0 powerdown */
												/* bit[20]=1 clr phy0 phy_reset  */
#define	PHY1_CSR_TEST_POWERDOWN_CLR	0x3070201c	/* bit[4]=1 clr phy1 powerdown */
												/* bit[20]=1 clr phy1 phy_reset  */

#define	PCIE0_PCIE1_PHY_SW_PRST_CLR	0x30710020	/* bit[0]=1 clr phy sw prst */
#define	PIPE0_RESET_N_SET		0x30701020	/* bit[0]=1 set pipe0_reset_n */
#define	CLK_PIPE_BUS_SEL_SET		0x30010018	/* bit[7:6]=0x3 set bus clk to 500M */

#define PCI_BASE_ADDRESS_0	0x10	/* 32 bits */

#define	PCI_BASE_ADDRESS_MEM_TYPE_32	0x00	/* 32 bit address */
#define	PCI_BASE_ADDRESS_MEM_TYPE_64	0x04	/* 64 bit address */
#define	PCI_BASE_ADDRESS_MEM_PREFETCH	0x08	/* prefetchable? */

#define DBI_CS2			0x100000
#define PCI_DBI_BASE		0x40000000
#define PCI_DBI2_BASE		(PCI_DBI_BASE + DBI_CS2)
#define PCI_PIPE_SYS		0x30000000
#define GLB_ARMISC_SET		0x10d0
#define GLB_AWMISC_SET		0x10c0
#define GLB_ARMISC_CLR		0x20d0
#define GLB_AWMISC_CLR		0x20c0
#define	GLB_MSI_CTL_SET		0x10d8
#define	GLB_MSI_CTL_CLR		0x20d8
#define	GLB_MSI_REG_UPDATE_SET	0x10e0
#define	GLB_MSI_REG_UPDATE_CLR	0x20e0
#define GLB_AWRMISC_EN		(0x1 << 21)
#define GLB_AWRMISC_CLR		(0x1 << 21)

#define PCIE_PORT_LINK_CONTROL		0x710
#define	PORT_LINK_MODE_MASK		(0x3f << 16)
#define PORT_LINK_MODE_1_LANES		(0x1 << 16)
#define	PCIE_LINK_WIDTH_SPEED_CONTROL	0x80C
#define	DIRECT_SPEED_CHANGE		(0x1 << 17)
#define PORT_LOGIC_LINK_WIDTH_MASK	(0x1f << 8)
#define PORT_LOGIC_LINK_WIDTH_1_LANES	(0x1 << 8)

#define PCIE_MISC_CONTROL_1_OFF		0x8BC
#define PCIE_DBI_RO_WR_EN		(0x1 << 0)

#define	PCIE_LINK_CTL2_REG		0xa0
#define	PCIE_LINK_SPEED_MASK		0xf
#define	PCIE_LINK_SPEED_GEN1		(0x1 << 0)

/*
 * Configuration Space header
 */
#define	PCI_VENDOR_ID		0x00	/* 16 bit	Vendor ID */
#define	PCI_DEVICE_ID		0x02	/* 16 bit	Device ID */
#define	PCI_COMMAND		0x04	/* 16 bit	Command */
#define	PCI_STATUS		0x06	/* 16 bit	Status */

#define PCI_CLASS_PROG		0x09	/* Reg. Level Programming Interface */
#define PCI_CLASS_REVID		0x08	/* High 24 bits are class, low 8 revision */
#define PCI_CLASS_DEVICE	0x0a	/* Device class */
#define PCI_CACHE_LINE_SIZE	0x0c	/* 8 bits */
#define PCI_SUBSYSTEM_VENDOR_ID	0x2c
#define PCI_SUBSYSTEM_ID	0x2e
#define PCI_INTERRUPT_PIN	0x3d	/* 8 bits */

/* device info */
#define	PCI_CLASS_NOT_DEFINED	0x0000
#define	PCI_CLASS_BRIDGE_HOST	0x0600
#define VENDORID		0x1F4B
#define DEVICEID		0x0650
#define BASECLASSCODE	0xff
#define SUBCLASSCODE	0x00
#define	INTERRUPT_PIN	0x01
#define	SUBSYS_ID		0x0000
#define	SUBSYS_VENDOR_ID	0x0000
#define	CACHE_LINE_SIZE		0x0000
#define	PROGIF_CODE		0x00
#define	REVID			0x00

#define DW_PCIE_AS_MEM		1
#define PCIE_ATU_TYPE_MEM	(0x0 << 0)
#define PCIE_MSG_BASE		0x04840000
#define PCIE_MEMORY_BASE	0x50000000
#define INBOUND_INDEX0		0
#define OUTBOUND_INDEX0		0
#define BAR_0		0

#define BAR_SIZE	256

#define LINK_WAIT_IATU	9

#define PCI_MSI_CAP		0x50
#define PCI_MSI_FLAGS		2	/* Message Control */
#define PCI_MSI_FLAGS_QMASK	0x000e	/* Maximum queue size available */
#define PCI_MSI_FLAGS_ENABLE	0x0001	/* MSI feature enabled */
#define PCI_MSI_INT_NUM		32	/* MSI interrupts number */
#define	PCI_MSI_ADDR_LO		4	/* MSI low addr */
#define	PCI_MSI_ADDR_HI		8	/* MSI low addr */

/* HDMA register */
#define	CHAN_ADDR_BASE		0x200
#define	RD_CHAN_EN		0x100
#define	RD_CHAN_TRAN_SIZE	0x11c
#define	RD_SAR_LOW_ADDR		0x120
#define	RD_SAR_UPPER_ADDR	0x124
#define	RD_DAR_LOW_ADDR		0x128
#define	RD_DAR_UPPER_ADDR	0x12C
#define	RD_DOORBELL_EN		0x104
#define	RD_CHAN_STATUS		0x180

/*
 * iATU Unroll-specific register definitions
 * From 4.80 core version the address translation will be made by unroll
 */
#define PCIE_ATU_UNR_REGION_CTRL1	0x00
#define PCIE_ATU_UNR_REGION_CTRL2	0x04
#define PCIE_ATU_UNR_LOWER_BASE		0x08
#define PCIE_ATU_UNR_UPPER_BASE		0x0C
#define PCIE_ATU_UNR_LIMIT		0x10
#define PCIE_ATU_UNR_LOWER_TARGET	0x14
#define PCIE_ATU_UNR_UPPER_TARGET	0x18

#define PCIE_ATU_ENABLE			(0x1 << 31)
#define PCIE_ATU_BAR_MODE_ENABLE	(0x1 << 30)
#define PCIE_DMA_BYPASS			(0x1 << 27)
#define	PCIE_DMA_BASE			0x380000

#define IRQ_TYPE_MSIX			2
#define COMMAND_READ			BIT(3)
#define STATUS_READ_SUCCESS		BIT(0)
#define STATUS_READ_FAIL		BIT(1)
#define	HDMA_RD_ORG_HDR_FL		BIT(2)
#define	HDMA_RD_BAK_HDR_FL		BIT(3)
#define	HDR_CHKSUM_FL			BIT(4)
#define	HDMA_RD_FW_FL			BIT(5)
#define	FW_CHKSUM_FL			BIT(6)
#define	EIP_INIT_FL			BIT(7)
#define	PUB_KEY_CHK_FL			BIT(8)
#define	HDMA_FW_IMG_FL			BIT(9)
#define	IMG_CHKSUM_FL			BIT(10)
#define	CE_SHA_FL			BIT(11)
#define	CE_AES_FL			BIT(12)

#define	PCIE_DATA_FAIL		-1
#define	CE_FW_FAIL		-2
#define	PCIE_HEADER_CHECHSUM_FAIL	-3
#define	PCIE_HDMA_READ_ABORT		-10

/* Register address builder */
#define PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(region)	\
			((0x3 << 20) | ((region) << 9))

#define PCIE_GET_ATU_INB_UNR_REG_OFFSET(region)				\
			((0x3 << 20) | ((region) << 9) | (0x1 << 8))

#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))
#define lower_32_bits(n) ((u32)(n))

struct pci_msg_reg {
	u32	magic;
	u32	command;
	u32	status;
	u32	low_src_addr;
	u32	upper_src_addr;
	u32	low_dst_addr;
	u32	upper_dst_addr;
	u32	size;
	u32	checksum;
	u32	irq_type;
	u32	irq_number;
} __packed;

void dw_ep_init();
int polling_remote_command(u32 flash_type);
int pcie_read(char *dest, u32 start_addr, int size);
#endif