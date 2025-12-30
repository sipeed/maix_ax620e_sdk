#include <asm/io.h>
#include <asm/arch-axera/ax620e.h>
#include <asm/arch/boot_mode.h>
#include <common.h>
#include <linux/iopoll.h>
#include <linux/bug.h>
#include <div64.h>
#include <linux/kernel.h>

#include "ax_vo.h"

#define IP_CONF				0x0
#define SP_HS_FIFO_DEPTH(x)		(((x) & GENMASK(30, 26)) >> 26)
#define SP_LP_FIFO_DEPTH(x)		(((x) & GENMASK(25, 21)) >> 21)
#define VRS_FIFO_DEPTH(x)		(((x) & GENMASK(20, 16)) >> 16)
#define DIRCMD_FIFO_DEPTH(x)		(((x) & GENMASK(15, 13)) >> 13)
#define SDI_IFACE_32			BIT(12)
#define INTERNAL_DATAPATH_32		(0 << 10)
#define INTERNAL_DATAPATH_16		(1 << 10)
#define INTERNAL_DATAPATH_8		(3 << 10)
#define INTERNAL_DATAPATH_SIZE		((x) & GENMASK(11, 10))
#define NUM_IFACE(x)			((((x) & GENMASK(9, 8)) >> 8) + 1)
#define MAX_LANE_NB(x)			(((x) & GENMASK(7, 6)) >> 6)
#define RX_FIFO_DEPTH(x)		((x) & GENMASK(5, 0))

#define MCTL_MAIN_DATA_CTL		0x4
#define TE_MIPI_POLLING_EN		BIT(25)
#define TE_HW_POLLING_EN		BIT(24)
#define DISP_EOT_GEN			BIT(18)
#define HOST_EOT_GEN			BIT(17)
#define DISP_GEN_CHECKSUM		BIT(16)
#define DISP_GEN_ECC			BIT(15)
#define BTA_EN				BIT(14)
#define READ_EN				BIT(13)
#define REG_TE_EN			BIT(12)
#define IF_TE_EN(x)			BIT(8 + (x))
#define TVG_SEL				BIT(6)
#define VID_EN				BIT(5)
#define IF_VID_SELECT(x)		((x) << 2)
#define IF_VID_SELECT_MASK		GENMASK(3, 2)
#define IF_VID_MODE			BIT(1)
#define LINK_EN				BIT(0)

#define MCTL_MAIN_PHY_CTL		0x8
#define HS_INVERT_DAT(x)		BIT(19 + ((x) * 2))
#define SWAP_PINS_DAT(x)		BIT(18 + ((x) * 2))
#define HS_INVERT_CLK			BIT(17)
#define SWAP_PINS_CLK			BIT(16)
#define HS_SKEWCAL_EN			BIT(15)
#define WAIT_BURST_TIME(x)		((x) << 10)
#define DATA_ULPM_EN(x)			BIT(6 + (x))
#define CLK_ULPM_EN			BIT(5)
#define CLK_CONTINUOUS			BIT(4)
#define DATA_LANE_EN(x)			BIT((x) - 1)

#define MCTL_MAIN_EN			0xc
#define DATA_FORCE_STOP			BIT(17)
#define CLK_FORCE_STOP			BIT(16)
#define IF_EN(x)			BIT(13 + (x))
#define DATA_LANE_ULPM_REQ(l)		BIT(9 + (l))
#define CLK_LANE_ULPM_REQ		BIT(8)
#define DATA_LANE_START(x)		BIT(4 + (x))
#define CLK_LANE_EN			BIT(3)
#define PLL_START			BIT(0)

#define MCTL_DPHY_CFG0			0x10
#define DPHY_C_RSTB			BIT(20)
#define DPHY_D_RSTB(x)			GENMASK(15 + (x), 16)
#define DPHY_PLL_PDN			BIT(10)
#define DPHY_CMN_PDN			BIT(9)
#define DPHY_C_PDN			BIT(8)
#define DPHY_D_PDN(x)			GENMASK(3 + (x), 4)
#define DPHY_ALL_D_PDN			GENMASK(7, 4)
#define DPHY_PLL_PSO			BIT(1)
#define DPHY_CMN_PSO			BIT(0)

#define MCTL_DPHY_TIMEOUT1		0x14
#define HSTX_TIMEOUT(x)			((x) << 4)
#define HSTX_TIMEOUT_MAX		GENMASK(17, 0)
#define CLK_DIV(x)			(x)
#define CLK_DIV_MAX			GENMASK(3, 0)

#define MCTL_DPHY_TIMEOUT2		0x18
#define LPRX_TIMEOUT(x)			(x)

#define MCTL_ULPOUT_TIME		0x1c
#define DATA_LANE_ULPOUT_TIME(x)	((x) << 9)
#define CLK_LANE_ULPOUT_TIME(x)		(x)

#define MCTL_3DVIDEO_CTL		0x20
#define VID_VSYNC_3D_EN			BIT(7)
#define VID_VSYNC_3D_LR			BIT(5)
#define VID_VSYNC_3D_SECOND_EN		BIT(4)
#define VID_VSYNC_3DFORMAT_LINE		(0 << 2)
#define VID_VSYNC_3DFORMAT_FRAME	(1 << 2)
#define VID_VSYNC_3DFORMAT_PIXEL	(2 << 2)
#define VID_VSYNC_3DMODE_OFF		0
#define VID_VSYNC_3DMODE_PORTRAIT	1
#define VID_VSYNC_3DMODE_LANDSCAPE	2

#define MCTL_MAIN_STS			0x24
#define MCTL_MAIN_STS_CTL		0x130
#define MCTL_MAIN_STS_CLR		0x150
#define MCTL_MAIN_STS_FLAG		0x170
#define HS_SKEWCAL_DONE			BIT(11)
#define IF_UNTERM_PKT_ERR(x)		BIT(8 + (x))
#define LPRX_TIMEOUT_ERR		BIT(7)
#define HSTX_TIMEOUT_ERR		BIT(6)
#define DATA_LANE_RDY(l)		BIT(2 + (l))
#define CLK_LANE_RDY			BIT(1)
#define PLL_LOCKED			BIT(0)

#define MCTL_DPHY_ERR			0x28
#define MCTL_DPHY_ERR_CTL1		0x148
#define MCTL_DPHY_ERR_CLR		0x168
#define MCTL_DPHY_ERR_FLAG		0x188
#define ERR_CONT_LP(x, l)		BIT(18 + ((x) * 4) + (l))
#define ERR_CONTROL(l)			BIT(14 + (l))
#define ERR_SYNESC(l)			BIT(10 + (l))
#define ERR_ESC(l)			BIT(6 + (l))

#define MCTL_DPHY_ERR_CTL2		0x14c
#define ERR_CONT_LP_EDGE(x, l)		BIT(12 + ((x) * 4) + (l))
#define ERR_CONTROL_EDGE(l)		BIT(8 + (l))
#define ERR_SYN_ESC_EDGE(l)		BIT(4 + (l))
#define ERR_ESC_EDGE(l)			BIT(0 + (l))

#define MCTL_LANE_STS			0x2c
#define PPI_C_TX_READY_HS		BIT(18)
#define DPHY_PLL_LOCK			BIT(17)
#define PPI_D_RX_ULPS_ESC(x)		(((x) & GENMASK(15, 12)) >> 12)
#define LANE_STATE_START		0
#define LANE_STATE_IDLE			1
#define LANE_STATE_WRITE		2
#define LANE_STATE_ULPM			3
#define LANE_STATE_READ			4
#define DATA_LANE_STATE(l, val)		\
	(((val) >> (2 + 2 * (l) + ((l) ? 1 : 0))) & GENMASK((l) ? 1 : 2, 0))
#define CLK_LANE_STATE_HS		2
#define CLK_LANE_STATE(val)		((val) & GENMASK(1, 0))

#define DSC_MODE_CTL			0x30
#define DSC_MODE_EN			BIT(0)

#define DSC_CMD_SEND			0x34
#define DSC_SEND_PPS			BIT(0)
#define DSC_EXECUTE_QUEUE		BIT(1)

#define DSC_PPS_WRDAT			0x38

#define DSC_MODE_STS			0x3c
#define DSC_PPS_DONE			BIT(1)
#define DSC_EXEC_DONE			BIT(2)

#define CMD_MODE_CTL			0x70
#define IF_LP_EN(x)			BIT(9 + (x))
#define IF_VCHAN_ID(x, c)		((c) << ((x) * 2))

#define CMD_MODE_CTL2			0x74
#define TE_TIMEOUT(x)			((x) << 11)
#define FILL_VALUE(x)			((x) << 3)
#define ARB_IF_WITH_HIGHEST_PRIORITY(x)	((x) << 1)
#define ARB_ROUND_ROBIN_MODE		BIT(0)

#define CMD_MODE_STS			0x78
#define CMD_MODE_STS_CTL		0x134
#define CMD_MODE_STS_CLR		0x154
#define CMD_MODE_STS_FLAG		0x174
#define ERR_IF_UNDERRUN(x)		BIT(4 + (x))
#define ERR_UNWANTED_READ		BIT(3)
#define ERR_TE_MISS			BIT(2)
#define ERR_NO_TE			BIT(1)
#define CSM_RUNNING			BIT(0)

#define DIRECT_CMD_SEND			0x80

#define DIRECT_CMD_MAIN_SETTINGS	0x84
#define TRIGGER_VAL(x)			((x) << 25)
#define CMD_LP_EN			BIT(24)
#define CMD_SIZE(x)			((x) << 16)
#define CMD_VCHAN_ID(x)			((x) << 14)
#define CMD_DATATYPE(x)			((x) << 8)
#define CMD_LONG			BIT(3)
#define WRITE_CMD			0
#define READ_CMD			1
#define TE_REQ				4
#define TRIGGER_REQ			5
#define BTA_REQ				6

#define DIRECT_CMD_STS			0x88
#define DIRECT_CMD_STS_CTL		0x138
#define DIRECT_CMD_STS_CLR		0x158
#define DIRECT_CMD_STS_FLAG		0x178
#define RCVD_ACK_VAL(val)		((val) >> 16)
#define RCVD_TRIGGER_VAL(val)		(((val) & GENMASK(14, 11)) >> 11)
#define READ_COMPLETED_WITH_ERR		BIT(10)
#define BTA_FINISHED			BIT(9)
#define BTA_COMPLETED			BIT(8)
#define TE_RCVD				BIT(7)
#define TRIGGER_RCVD			BIT(6)
#define ACK_WITH_ERR_RCVD		BIT(5)
#define ACK_RCVD			BIT(4)
#define READ_COMPLETED			BIT(3)
#define TRIGGER_COMPLETED		BIT(2)
#define WRITE_COMPLETED			BIT(1)
#define SENDING_CMD			BIT(0)

#define DIRECT_CMD_STOP_READ		0x8c

#define DIRECT_CMD_WRDATA		0x90

#define DIRECT_CMD_FIFO_RST		0x94

#define DIRECT_CMD_RDDATA		0xa0

#define DIRECT_CMD_RD_PROPS		0xa4
#define RD_DCS				BIT(18)
#define RD_VCHAN_ID(val)		(((val) >> 16) & GENMASK(1, 0))
#define RD_SIZE(val)			((val) & GENMASK(15, 0))

#define DIRECT_CMD_RD_STS		0xa8
#define DIRECT_CMD_RD_STS_CTL		0x13c
#define DIRECT_CMD_RD_STS_CLR		0x15c
#define DIRECT_CMD_RD_STS_FLAG		0x17c
#define ERR_EOT_WITH_ERR		BIT(8)
#define ERR_MISSING_EOT			BIT(7)
#define ERR_WRONG_LENGTH		BIT(6)
#define ERR_OVERSIZE			BIT(5)
#define ERR_RECEIVE			BIT(4)
#define ERR_UNDECODABLE			BIT(3)
#define ERR_CHECKSUM			BIT(2)
#define ERR_UNCORRECTABLE		BIT(1)
#define ERR_FIXED			BIT(0)

#define VID_MAIN_CTL			0xb0
#define VID_IGNORE_MISS_VSYNC		BIT(31)
#define VID_FIELD_SW			BIT(28)
#define VID_INTERLACED_EN		BIT(27)
#define RECOVERY_MODE(x)		((x) << 25)
#define RECOVERY_MODE_NEXT_HSYNC	0
#define RECOVERY_MODE_NEXT_STOP_POINT	2
#define RECOVERY_MODE_NEXT_VSYNC	3
#define REG_BLKEOL_MODE(x)		((x) << 23)
#define REG_BLKLINE_MODE(x)		((x) << 21)
#define REG_BLK_MODE_NULL_PKT		0
#define REG_BLK_MODE_BLANKING_PKT	1
#define REG_BLK_MODE_LP			2
#define SYNC_PULSE_HORIZONTAL		BIT(20)
#define SYNC_PULSE_ACTIVE		BIT(19)
#define BURST_MODE			BIT(18)
#define VID_PIXEL_MODE_MASK		GENMASK(17, 14)
#define VID_PIXEL_MODE_RGB565		(0 << 14)
#define VID_PIXEL_MODE_RGB666_PACKED	(1 << 14)
#define VID_PIXEL_MODE_RGB666		(2 << 14)
#define VID_PIXEL_MODE_RGB888		(3 << 14)
#define VID_PIXEL_MODE_RGB101010	(4 << 14)
#define VID_PIXEL_MODE_RGB121212	(5 << 14)
#define VID_PIXEL_MODE_YUV420		(8 << 14)
#define VID_PIXEL_MODE_YUV422_PACKED	(9 << 14)
#define VID_PIXEL_MODE_YUV422		(10 << 14)
#define VID_PIXEL_MODE_YUV422_24B	(11 << 14)
#define VID_PIXEL_MODE_DSC_COMP		(12 << 14)
#define VID_DATATYPE(x)			((x) << 8)
#define VID_VIRTCHAN_ID(iface, x)	((x) << (4 + (iface) * 2))
#define STOP_MODE(x)			((x) << 2)
#define START_MODE(x)			(x)

#define VID_VSIZE1			0xb4
#define VFP_LEN(x)			((x) << 12)
#define VBP_LEN(x)			((x) << 6)
#define VSA_LEN(x)			(x)

#define VID_VSIZE2			0xb8
#define VACT_LEN(x)			(x)

#define VID_HSIZE1			0xc0
#define HBP_LEN(x)			((x) << 16)
#define HSA_LEN(x)			(x)

#define VID_HSIZE2			0xc4
#define HFP_LEN(x)			((x) << 16)
#define HACT_LEN(x)			(x)

#define VID_BLKSIZE1			0xcc
#define BLK_EOL_PKT_LEN(x)		((x) << 15)
#define BLK_LINE_EVENT_PKT_LEN(x)	(x)

#define VID_BLKSIZE2			0xd0
#define BLK_LINE_PULSE_PKT_LEN(x)	(x)

#define VID_PKT_TIME			0xd8
#define BLK_EOL_DURATION(x)		(x)

#define VID_DPHY_TIME			0xdc
#define REG_WAKEUP_TIME(x)		((x) << 17)
#define REG_LINE_DURATION(x)		(x)

#define VID_ERR_COLOR1			0xe0
#define COL_GREEN(x)			((x) << 12)
#define COL_RED(x)			(x)

#define VID_ERR_COLOR2			0xe4
#define PAD_VAL(x)			((x) << 12)
#define COL_BLUE(x)			(x)

#define VID_VPOS			0xe8
#define LINE_VAL(val)			(((val) & GENMASK(14, 2)) >> 2)
#define LINE_POS(val)			((val) & GENMASK(1, 0))

#define VID_HPOS			0xec
#define HORIZ_VAL(val)			(((val) & GENMASK(17, 3)) >> 3)
#define HORIZ_POS(val)			((val) & GENMASK(2, 0))

#define VID_MODE_STS			0xf0
#define VID_MODE_STS_CTL		0x140
#define VID_MODE_STS_CLR		0x160
#define VID_MODE_STS_FLAG		0x180
#define VSG_RECOVERY			BIT(10)
#define ERR_VRS_WRONG_LEN		BIT(9)
#define ERR_LONG_READ			BIT(8)
#define ERR_LINE_WRITE			BIT(7)
#define ERR_BURST_WRITE			BIT(6)
#define ERR_SMALL_HEIGHT		BIT(5)
#define ERR_SMALL_LEN			BIT(4)
#define ERR_MISSING_VSYNC		BIT(3)
#define ERR_MISSING_HSYNC		BIT(2)
#define ERR_MISSING_DATA		BIT(1)
#define VSG_RUNNING			BIT(0)

#define VID_VCA_SETTING1		0xf4
#define BURST_LP			BIT(16)
#define MAX_BURST_LIMIT(x)		(x)

#define VID_VCA_SETTING2		0xf8
#define MAX_LINE_LIMIT(x)		((x) << 16)
#define EXACT_BURST_LIMIT(x)		(x)

#define TVG_CTL				0xfc
#define TVG_STRIPE_SIZE(x)		((x) << 5)
#define TVG_MODE_MASK			GENMASK(4, 3)
#define TVG_MODE_SINGLE_COLOR		(0 << 3)
#define TVG_MODE_VSTRIPES		(2 << 3)
#define TVG_MODE_HSTRIPES		(3 << 3)
#define TVG_STOPMODE_MASK		GENMASK(2, 1)
#define TVG_STOPMODE_EOF		(0 << 1)
#define TVG_STOPMODE_EOL		(1 << 1)
#define TVG_STOPMODE_NOW		(2 << 1)
#define TVG_RUN				BIT(0)

#define TVG_IMG_SIZE			0x100
#define TVG_NBLINES(x)			((x) << 16)
#define TVG_LINE_SIZE(x)		(x)

#define TVG_COLOR1			0x104
#define TVG_COL1_GREEN(x)		((x) << 12)
#define TVG_COL1_RED(x)			(x)

#define TVG_COLOR1_BIS			0x108
#define TVG_COL1_BLUE(x)		(x)

#define TVG_COLOR2			0x10c
#define TVG_COL2_GREEN(x)		((x) << 12)
#define TVG_COL2_RED(x)			(x)

#define TVG_COLOR2_BIS			0x110
#define TVG_COL2_BLUE(x)		(x)

#define TVG_STS				0x114
#define TVG_STS_CTL			0x144
#define TVG_STS_CLR			0x164
#define TVG_STS_FLAG			0x184
#define TVG_STS_RUNNING			BIT(0)

#define STS_CTL_EDGE(e)			((e) << 16)

#define DPHY_LANES_MAP			0x198
#define DAT_REMAP_CFG(b, l)		((l) << ((b) * 8))

#define DPI_IRQ_EN			0x1a0
#define DPI_IRQ_CLR			0x1a4
#define DPI_IRQ_STS			0x1a8
#define PIXEL_BUF_OVERFLOW		BIT(0)

#define DPI_CFG				0x1ac
#define DPI_CFG_FIFO_DEPTH(x)		((x) >> 16)
#define DPI_CFG_FIFO_LEVEL(x)		((x) & GENMASK(15, 0))

#define TEST_GENERIC			0x1f0
#define TEST_STATUS(x)			((x) >> 16)
#define TEST_CTRL(x)			(x)

#define ID_REG				0x1fc
#define REV_VENDOR_ID(x)		(((x) & GENMASK(31, 20)) >> 20)
#define REV_PRODUCT_ID(x)		(((x) & GENMASK(19, 12)) >> 12)
#define REV_HW(x)			(((x) & GENMASK(11, 8)) >> 8)
#define REV_MAJOR(x)			(((x) & GENMASK(7, 4)) >> 4)
#define REV_MINOR(x)			((x) & GENMASK(3, 0))

#define DSI_OUTPUT_PORT			0
#define DSI_INPUT_PORT(inputid)		(1 + (inputid))

#define DSI_HBP_FRAME_OVERHEAD		12
#define DSI_HSA_FRAME_OVERHEAD		14
#define DSI_HFP_FRAME_OVERHEAD		6
#define DSI_HSS_VSS_VSE_FRAME_OVERHEAD	4
#define DSI_BLANKING_FRAME_OVERHEAD	6
#define DSI_NULL_FRAME_OVERHEAD		6
#define DSI_EOT_PKT_SIZE		4

#define DSI_VFP_SIZE			1
#define MIPI_DSI_DCS_SHORT_WRITE	0x05
#define MIPI_DSI_DCS_SHORT_WRITE_PARAM	0x15
#define MIPI_DSI_DCS_LONG_WRITE		0x39
#define MIPI_DSI_PACKED_PIXEL_STREAM_16	0x0e
#define	MIPI_DSI_PACKED_PIXEL_STREAM_18	0x1e
#define	MIPI_DSI_PIXEL_STREAM_3BYTE_18	0x2e
#define	MIPI_DSI_PACKED_PIXEL_STREAM_24	0x3e

#define DISPLAY_SYS_CLK			416000000L
#define NSEC_PER_SEC			1000000000L
#define NSEC_PER_MSEC			1000000L

#define  CDNS_MODE_VIDEO_BURST_CLK_RATIO	(2)

enum cdns_dsi_input_id {
	CDNS_SDI_INPUT,
	CDNS_DPI_INPUT,
	CDNS_DSC_INPUT,
};

struct cdns_dsi {
	void __iomem *regs;
	enum cdns_dsi_input_id input_id;
	int nlanes;
	unsigned long lane_bps;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	unsigned int direct_cmd_fifo_depth;
	bool link_initialized;
};

struct cdns_dsi_cfg {
	unsigned int hfp;
	unsigned int hsa;
	unsigned int hbp;
	unsigned int hact;
	unsigned int htotal;
};

static struct cdns_dsi g_dsi;

static unsigned int dpi_to_dsi_timing(unsigned int dpi_timing,
				      unsigned int dpi_bpp,
				      unsigned int dsi_pkt_overhead)
{
	unsigned int dsi_timing = DIV_ROUND_UP(dpi_timing * dpi_bpp, 8);

	if (dsi_timing < dsi_pkt_overhead)
		dsi_timing = 0;
	else
		dsi_timing -= dsi_pkt_overhead;

	return dsi_timing;
}

int mipi_dsi_pixel_format_to_bpp(enum mipi_dsi_pixel_format fmt)
{
	switch (fmt) {
	case MIPI_DSI_FMT_RGB888:
	case MIPI_DSI_FMT_RGB666:
		return 24;

	case MIPI_DSI_FMT_RGB666_PACKED:
		return 18;

	case MIPI_DSI_FMT_RGB565:
		return 16;
	}

	return -EINVAL;
}

int cdns_dsi_mode2cfg(struct cdns_dsi *dsi,
		      const struct ax_disp_mode *mode,
		      struct cdns_dsi_cfg *dsi_cfg)
{
	unsigned int tmp, dsi_hfp_ext = 0, dpi_hfp;
	bool sync_pulse = false;
	int bpp, nlanes;
	unsigned long dsi_htotal = 0, dsi_hss_hsa_hse_hbp = 0;

	memset(dsi_cfg, 0, sizeof(*dsi_cfg));

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		sync_pulse = true;

	bpp = mipi_dsi_pixel_format_to_bpp(dsi->format);
	nlanes = dsi->nlanes;

	tmp = mode->htotal -
		(sync_pulse ? mode->hsync_end : mode->hsync_start);

	if (sync_pulse) {
		dsi_cfg->hbp = dpi_to_dsi_timing(tmp, bpp, DSI_HBP_FRAME_OVERHEAD);
		dsi_hss_hsa_hse_hbp += dsi_cfg->hbp + DSI_HBP_FRAME_OVERHEAD;
		tmp = mode->hsync_end - mode->hsync_start;

		dsi_cfg->hsa = dpi_to_dsi_timing(tmp, bpp,
						 DSI_HSA_FRAME_OVERHEAD);
		dsi_hss_hsa_hse_hbp += dsi_cfg->hsa + DSI_HSA_FRAME_OVERHEAD;
	} else {
		tmp *= CDNS_MODE_VIDEO_BURST_CLK_RATIO;
		dsi_cfg->hbp = dpi_to_dsi_timing(tmp, bpp, DSI_HBP_FRAME_OVERHEAD);
		dsi_cfg->hsa = 0;
	}

	dsi_cfg->hact = dpi_to_dsi_timing(mode->hdisplay, bpp, 0);

	if (!sync_pulse)
		dsi_cfg->hbp += dsi_cfg->hact*(CDNS_MODE_VIDEO_BURST_CLK_RATIO - 1) + 10;


	dpi_hfp = mode->hsync_start - mode->hdisplay;

	dsi_cfg->hfp = dpi_to_dsi_timing(dpi_hfp, bpp, DSI_HFP_FRAME_OVERHEAD);

	dsi_htotal = mode->htotal;

	dsi_htotal = dpi_to_dsi_timing(dsi_htotal, bpp, 0);

	VO_DEBUG("%s nlanes = %d, dsi_hss_hsa_hse_hbp = %ld dsi_htotal:%ld\n",
		 __func__, nlanes, dsi_hss_hsa_hse_hbp, dsi_htotal);

	VO_DEBUG("%s dsi_hfp_ext = %d\n", __func__, dsi_hfp_ext);
	if (!dsi_hfp_ext)
		dsi_hfp_ext = 0;
	if (sync_pulse)
		dsi_cfg->hfp += dsi_hfp_ext;
	else
		dsi_cfg->hfp = 0;
	dsi_htotal += dsi_hfp_ext;
	dsi_cfg->htotal = dsi_htotal;

	/*
	 * Make sure DPI(HFP) > DSI(HSS+HSA+HSE+HBP) to guarantee that the FIFO
	 * is empty before we start a receiving a new line on the DPI
	 * interface.
	 */
	if ((u64)dsi->lane_bps * dpi_hfp * nlanes <
	    (u64)dsi_hss_hsa_hse_hbp * (mode->clock) * 1000) {
		VO_ERROR("%s err\n", __func__);
		return -EINVAL;
	}

	VO_DEBUG("%s dsi hfp = %d, hsa = %d, hbp = %d, hact = %d, htotal = %d\n",
		__func__, dsi_cfg->hfp, dsi_cfg->hsa, dsi_cfg->hbp, dsi_cfg->hact, dsi_cfg->htotal);

	return 0;
}

static void cdns_dsi_hs_init(struct cdns_dsi *dsi)
{
	u32 status;
	u32 val;

	val = readl(dsi->regs + MCTL_MAIN_STS);

	if (val & PLL_LOCKED) {
		/* Activate the PLL and wait until it's locked. */
		writel(PLL_LOCKED, dsi->regs + MCTL_MAIN_STS_CLR);

		WARN_ON_ONCE(readl_poll_timeout(dsi->regs + MCTL_MAIN_STS, status,
						status & PLL_LOCKED, 100));
	}
}

static void cdns_dsi_init_link(struct cdns_dsi *dsi)
{
	unsigned long sysclk = 0, sysclk_period, ulpout;
	u32 val;
	int i;

	if (dsi->link_initialized)
		return;

	val = 0;
	for (i = 1; i < dsi->nlanes; i++)
		val |= DATA_LANE_EN(i);

	if (!(dsi->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS))
		val |= CLK_CONTINUOUS;

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		val |= WAIT_BURST_TIME(0xf);

	writel(val, dsi->regs + MCTL_MAIN_PHY_CTL);

	/* ULPOUT should be set to 1ms and is expressed in sysclk cycles. */
	sysclk = DISPLAY_SYS_CLK;
	sysclk_period = DIV_ROUND_DOWN_ULL(NSEC_PER_SEC, sysclk);
	ulpout = DIV_ROUND_UP(NSEC_PER_MSEC , sysclk_period);
	writel(CLK_LANE_ULPOUT_TIME(ulpout) | DATA_LANE_ULPOUT_TIME(ulpout),
	       dsi->regs + MCTL_ULPOUT_TIME);

	writel(LINK_EN, dsi->regs + MCTL_MAIN_DATA_CTL);

	val = CLK_LANE_EN | PLL_START;
	for (i = 0; i < dsi->nlanes; i++)
		val |= DATA_LANE_START(i);

	writel(val, dsi->regs + MCTL_MAIN_EN);

	dsi->link_initialized = true;
}

void cdns_dsi_config_enable(struct ax_disp_mode *mode)
{
	struct cdns_dsi *dsi =  &g_dsi;
	unsigned long tx_byte_period;
	unsigned long lane_bps = 0;
	struct cdns_dsi_cfg dsi_cfg;
	u32 tmp, reg_wakeup, div;
	int nlanes;
	u16 blkline_event_pck_len, blkeol_pck_len;
	int bpp;

	nlanes = dsi->nlanes;

	bpp = mipi_dsi_pixel_format_to_bpp(dsi->format);
	dsi->lane_bps = mode->clock*1000*(bpp / nlanes);

	cdns_dsi_mode2cfg(dsi, mode, &dsi_cfg);
	cdns_dsi_hs_init(dsi);
	cdns_dsi_init_link(dsi);

	writel(HBP_LEN(dsi_cfg.hbp) | HSA_LEN(dsi_cfg.hsa), dsi->regs + VID_HSIZE1);
	writel(HFP_LEN(dsi_cfg.hfp) | HACT_LEN(dsi_cfg.hact), dsi->regs + VID_HSIZE2);

	writel(VBP_LEN(mode->vtotal - mode->vsync_end) |
	       VFP_LEN(DSI_VFP_SIZE) |
	       VSA_LEN(mode->vsync_end - mode->vsync_start),
	       dsi->regs + VID_VSIZE1);
	writel(mode->vdisplay, dsi->regs + VID_VSIZE2);

	tmp = dsi_cfg.htotal -
	      (dsi_cfg.hsa + DSI_BLANKING_FRAME_OVERHEAD +
	       DSI_HSA_FRAME_OVERHEAD);
	writel(BLK_LINE_PULSE_PKT_LEN(tmp), dsi->regs + VID_BLKSIZE2);
	writel(MAX_LINE_LIMIT(tmp - DSI_NULL_FRAME_OVERHEAD),
		dsi->regs + VID_VCA_SETTING2);
	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
		tmp = dsi_cfg.htotal - (DSI_HSS_VSS_VSE_FRAME_OVERHEAD + DSI_BLANKING_FRAME_OVERHEAD);
		writel(BLK_LINE_EVENT_PKT_LEN(tmp), dsi->regs + VID_BLKSIZE1);
	} else if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
		writel(MAX_LINE_LIMIT(tmp - DSI_NULL_FRAME_OVERHEAD),
		       dsi->regs + VID_VCA_SETTING2);
		tmp = dsi_cfg.htotal * CDNS_MODE_VIDEO_BURST_CLK_RATIO;
		blkline_event_pck_len = tmp - 4;
		blkeol_pck_len = blkline_event_pck_len - (dsi_cfg.hact + DSI_BLANKING_FRAME_OVERHEAD + dsi_cfg.hbp + DSI_NULL_FRAME_OVERHEAD);
		writel((BLK_EOL_PKT_LEN(blkeol_pck_len)|BLK_LINE_EVENT_PKT_LEN(blkline_event_pck_len)), dsi->regs + VID_BLKSIZE1);
		tmp = DIV_ROUND_UP((blkeol_pck_len + 6), nlanes);
		writel(BLK_EOL_DURATION(tmp), dsi->regs + VID_PKT_TIME);
	}

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
		tmp = DIV_ROUND_UP(dsi_cfg.htotal, nlanes) - DIV_ROUND_UP(dsi_cfg.hsa, nlanes);
		lane_bps = dsi->lane_bps;
	} else if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
		tmp = DIV_ROUND_UP(dsi_cfg.htotal, nlanes) * CDNS_MODE_VIDEO_BURST_CLK_RATIO;
		lane_bps = dsi->lane_bps * CDNS_MODE_VIDEO_BURST_CLK_RATIO;
	}

	if (!(dsi->mode_flags & MIPI_DSI_MODE_EOT_PACKET))
		tmp -= DIV_ROUND_UP(DSI_EOT_PKT_SIZE, nlanes);

	tx_byte_period = DIV_ROUND_DOWN_ULL((u64)NSEC_PER_SEC * 8, lane_bps);
	VO_INFO("%s lane_bps = %ld, tx_byte_period = %ld\n", __func__, lane_bps, tx_byte_period);

	reg_wakeup = 2000 / tx_byte_period;

	writel((REG_WAKEUP_TIME(reg_wakeup) | REG_LINE_DURATION(tmp)), dsi->regs + VID_DPHY_TIME);


	/*
	 * HSTX and LPRX timeouts are both expressed in TX byte clk cycles and
	 * both should be set to at least the time it takes to transmit a
	 * frame.
	 */
	tmp = NSEC_PER_SEC / mode->vrefresh;
	tmp /= tx_byte_period;

	for (div = 0; div <= CLK_DIV_MAX; div++) {
		if (tmp <= HSTX_TIMEOUT_MAX)
			break;

		tmp >>= 1;
	}

	if (tmp > HSTX_TIMEOUT_MAX)
		tmp = HSTX_TIMEOUT_MAX;


	writel(CLK_DIV(div) | HSTX_TIMEOUT(tmp), dsi->regs + MCTL_DPHY_TIMEOUT1);
	writel(LPRX_TIMEOUT(tmp), dsi->regs + MCTL_DPHY_TIMEOUT2);

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO) {
		switch (dsi->format) {
		case MIPI_DSI_FMT_RGB888:
			tmp = VID_PIXEL_MODE_RGB888 |
			      VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_24);
			break;

		case MIPI_DSI_FMT_RGB666:
			tmp = VID_PIXEL_MODE_RGB666 |
			      VID_DATATYPE(MIPI_DSI_PIXEL_STREAM_3BYTE_18);
			break;

		case MIPI_DSI_FMT_RGB666_PACKED:
			tmp = VID_PIXEL_MODE_RGB666_PACKED |
			      VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_18);
			break;

		case MIPI_DSI_FMT_RGB565:
			tmp = VID_PIXEL_MODE_RGB565 |
			      VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_16);
			break;

		default:
			VO_ERROR("Unsupported DSI format\n");
			return;
		}

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
			tmp |= SYNC_PULSE_ACTIVE | SYNC_PULSE_HORIZONTAL;
			tmp |= REG_BLKLINE_MODE(REG_BLK_MODE_BLANKING_PKT) |
			REG_BLKEOL_MODE(REG_BLK_MODE_BLANKING_PKT) |
			RECOVERY_MODE(RECOVERY_MODE_NEXT_HSYNC) |
			VID_IGNORE_MISS_VSYNC;
		} else if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
			tmp |= BURST_MODE;
			tmp |= REG_BLKLINE_MODE(REG_BLK_MODE_NULL_PKT) |
			REG_BLKEOL_MODE(REG_BLK_MODE_NULL_PKT) |
			RECOVERY_MODE(1) |
			VID_IGNORE_MISS_VSYNC;
		}

		writel(tmp, dsi->regs + VID_MAIN_CTL);
	}

	tmp = readl(dsi->regs + MCTL_MAIN_DATA_CTL);
	tmp &= ~(IF_VID_SELECT_MASK | HOST_EOT_GEN | IF_VID_MODE);

	if (!(dsi->mode_flags & MIPI_DSI_MODE_EOT_PACKET))
		tmp |= HOST_EOT_GEN;

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO)
		tmp |= IF_VID_MODE | IF_VID_SELECT(dsi->input_id) | VID_EN;

	writel(tmp, dsi->regs + MCTL_MAIN_DATA_CTL);

	tmp = readl(dsi->regs + MCTL_MAIN_EN) | IF_EN(dsi->input_id);
	writel(tmp, dsi->regs + MCTL_MAIN_EN);
}

ssize_t cdns_dsi_transfer(u8 type, void *tx_buf,size_t tx_len)
{
	struct cdns_dsi *dsi = &g_dsi;
	u32 cmd, sts, val, wait = WRITE_COMPLETED, ctl = 0;
	int ret = 0, i;

	cdns_dsi_init_link(dsi);

	if (!((type == MIPI_DSI_DCS_SHORT_WRITE) | (type == MIPI_DSI_DCS_SHORT_WRITE_PARAM) | (type == MIPI_DSI_DCS_LONG_WRITE)))
		goto out;

	/* TX len is limited by the CMD FIFO depth. */
	if (tx_len > dsi->direct_cmd_fifo_depth) {
		ret = -ENOTSUPP;
		goto out;
	}

	cmd = CMD_SIZE(tx_len) |
	      CMD_DATATYPE(type);

	cmd |= CMD_LP_EN;

	if (type == MIPI_DSI_DCS_LONG_WRITE)
		cmd |= CMD_LONG;

	writel(readl(dsi->regs + MCTL_MAIN_DATA_CTL) | ctl,
	       dsi->regs + MCTL_MAIN_DATA_CTL);

	writel(cmd, dsi->regs + DIRECT_CMD_MAIN_SETTINGS);

	for (i = 0; i < tx_len; i += 4) {
		const u8 *buf = tx_buf;
		int j;

		val = 0;
		for (j = 0; j < 4 && j + i < tx_len; j++)
			val |= (u32)buf[i + j] << (8 * j);

		writel(val, dsi->regs + DIRECT_CMD_WRDATA);
	}

	/* Clear status flags before sending the command. */
	writel(wait, dsi->regs + DIRECT_CMD_STS_CLR);

	writel(wait, dsi->regs + DIRECT_CMD_STS_CTL);

	writel(0, dsi->regs + DIRECT_CMD_SEND);

	if (readl_poll_timeout(dsi->regs + DIRECT_CMD_STS, sts, sts& wait, 2000))
		VO_ERROR("dsi direct-cmd send timedout\n");

	sts = readl(dsi->regs + DIRECT_CMD_STS);
	writel(wait, dsi->regs + DIRECT_CMD_STS_CLR);

	writel(0, dsi->regs + DIRECT_CMD_STS_CTL);

	writel(readl(dsi->regs + MCTL_MAIN_DATA_CTL) & ~ctl,
	       dsi->regs + MCTL_MAIN_DATA_CTL);

	/* We did not receive the events we were waiting for. */
	if (!(sts & wait)) {
		ret = -ETIMEDOUT;
		VO_ERROR("timeout and not receive the events, sts:0x%x, wait:0x%x\n", sts, wait);
		goto out;
	}

	/* 'READ' or 'WRITE with ACK' failed. */
	if (sts & (READ_COMPLETED_WITH_ERR | ACK_WITH_ERR_RCVD)) {
		ret = -EIO;
		VO_ERROR("read or write with ack err, sts:0x%x\n", sts);
		goto out;
	}

out:
	return ret;
}

int display_mipi_cdns_init(void)
{
	struct cdns_dsi *dsi = &g_dsi;;
	int ret;
	u32 val;

	dsi->regs = (void __iomem *)DISPLAY_DSITX_BASE_ADDR;

	val = readl(dsi->regs + ID_REG);
	if (REV_VENDOR_ID(val) != 0xcad) {
		VO_ERROR("invalid vendor id\n");
		ret = -EINVAL;
		return ret;
	}

	val = readl(dsi->regs + IP_CONF);
	dsi->direct_cmd_fifo_depth = 1 << (DIRCMD_FIFO_DEPTH(val) + 2);
	/*
	 * We only support the DPI input, so force input->id to
	 * CDNS_DPI_INPUT.
	 */
	dsi->input_id = CDNS_DPI_INPUT;
	dsi->nlanes = g_dsi_panel.nlanes;
	dsi->format = g_dsi_panel.format;
	dsi->mode_flags = g_dsi_panel.mode_flags;

	/* Mask all interrupts before registering the IRQ handler. */
	writel(0, dsi->regs + MCTL_MAIN_STS_CTL);
	writel(0, dsi->regs + MCTL_DPHY_ERR_CTL1);
	writel(0, dsi->regs + CMD_MODE_STS_CTL);
	writel(0, dsi->regs + DIRECT_CMD_STS_CTL);
	writel(0, dsi->regs + DIRECT_CMD_RD_STS_CTL);
	writel(0, dsi->regs + VID_MODE_STS_CTL);
	writel(0, dsi->regs + TVG_STS_CTL);
	writel(0, dsi->regs + DPI_IRQ_EN);

	return ret;
}