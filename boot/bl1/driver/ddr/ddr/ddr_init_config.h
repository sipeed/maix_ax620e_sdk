#ifndef __DDR_INIT_CONFIG__
#define __DDR_INIT_CONFIG__
#include "ddr_common_config.h"

#if !defined(CFG_LPDDR4_CLK_CMDLINE) && defined(SENSOR_2M_CONFIG_1600)
#define LPDDR4_CONFIG_1600              1
#define LPDDR4_CONFIG_2133              0
#define LPDDR4_CONFIG_2400              0
#define LPDDR4_CONFIG_2666              0
#define LPDDR4_CONFIG_2800              0
#define LPDDR4_CONFIG_3200              0
#define LPDDR4_CONFIG_3733              0
#elif !defined(CFG_LPDDR4_CLK_CMDLINE)
#define LPDDR4_CONFIG_1600              0
#define LPDDR4_CONFIG_2133              0
#define LPDDR4_CONFIG_2400              1
#define LPDDR4_CONFIG_2666              0
#define LPDDR4_CONFIG_2800              0
#define LPDDR4_CONFIG_3200              0
#define LPDDR4_CONFIG_3733              0
#else
#ifndef LPDDR4_CONFIG_1600
#define LPDDR4_CONFIG_1600              0
#endif
#ifndef LPDDR4_CONFIG_2133
#define LPDDR4_CONFIG_2133              0
#endif
#ifndef LPDDR4_CONFIG_2400
#define LPDDR4_CONFIG_2400              0
#endif
#ifndef LPDDR4_CONFIG_2666
#define LPDDR4_CONFIG_2666              0
#endif
#ifndef LPDDR4_CONFIG_2800
#define LPDDR4_CONFIG_2800              0
#endif
#ifndef LPDDR4_CONFIG_3200
#define LPDDR4_CONFIG_3200              0
#endif
#ifndef LPDDR4_CONFIG_3733
#define LPDDR4_CONFIG_3733              0
#endif
#endif

#ifndef CFG_DDR4_CLK_CMDLINE
#define DDR4_CONFIG_1600                0
#define DDR4_CONFIG_2400                0
#define DDR4_CONFIG_2666                1
#define DDR4_CONFIG_3200                0
#else
#ifndef DDR4_CONFIG_1600
#define DDR4_CONFIG_1600                0
#endif
#ifndef DDR4_CONFIG_2400
#define DDR4_CONFIG_2400                0
#endif
#ifndef DDR4_CONFIG_2666
#define DDR4_CONFIG_2666                0
#endif
#ifndef DDR4_CONFIG_3200
#define DDR4_CONFIG_3200                0
#endif
#endif

#define DDR3_CONFIG_1600                0
#define DDR3_CONFIG_1866                0
#define DDR3_CONFIG_2133                1
typedef enum
{
	TYPE_LPDDR4						= 0x1000,
	TYPE_LPDDR4_1CS_1CH_X16_2GBIT	= 0x1000 | (DDR_1CS << 8) | (DDR_SINGLE_CHANNEL << 6) | (RF_DATA_WIDTH_X16 << 4) | DDR_CAPACITY_2GBIT,
	TYPE_LPDDR4_1CS_1CH_X16_4GBIT	= 0x1000 | (DDR_1CS << 8) | (DDR_SINGLE_CHANNEL << 6) | (RF_DATA_WIDTH_X16 << 4) | DDR_CAPACITY_4GBIT,
	TYPE_LPDDR4_1CS_2CH_X32_8GBIT	= 0x1000 | (DDR_1CS << 8) | (DDR_DUAL_CHANNEL << 6) | (RF_DATA_WIDTH_X32 << 4) | DDR_CAPACITY_8GBIT,
	TYPE_LPDDR4_1CS_2CH_X32_16GBIT	= 0x1000 | (DDR_1CS << 8) | (DDR_DUAL_CHANNEL << 6) | (RF_DATA_WIDTH_X32 << 4) | DDR_CAPACITY_16GBIT,
	TYPE_LPDDR4_1CS_2CH_X32_32GBIT	= 0x1000 | (DDR_1CS << 8) | (DDR_DUAL_CHANNEL << 6) | (RF_DATA_WIDTH_X32 << 4) | DDR_CAPACITY_32GBIT,
	TYPE_LPDDR4_2CS_2CH_X32_8GBIT	= 0x1000 | (DDR_2CS << 8) | (DDR_DUAL_CHANNEL << 6) | (RF_DATA_WIDTH_X32 << 4) | DDR_CAPACITY_8GBIT,
	TYPE_LPDDR4_2CS_2CH_X32_32GBIT	= 0x1000 | (DDR_2CS << 8) | (DDR_DUAL_CHANNEL << 6) | (RF_DATA_WIDTH_X32 << 4) | DDR_CAPACITY_32GBIT,
	TYPE_DDR4						= 0x2000,
	TYPE_DDR4_1CS_1CH_X16_8GBIT		= 0x2000 | (DDR_1CS << 8) | (DDR_SINGLE_CHANNEL << 6) | (RF_DATA_WIDTH_X16 << 4) | DDR_CAPACITY_8GBIT,
	TYPE_DDR4_1CS_2CH_X32_16GBIT	= 0x2000 | (DDR_1CS << 8) | (DDR_DUAL_CHANNEL << 6) | (RF_DATA_WIDTH_X32 << 4) | DDR_CAPACITY_16GBIT,
	TYPE_DDR3						= 0x4000,
	TYPE_DDR3_1CS_2CH_X32_4GBIT		= 0x4000 | (DDR_1CS << 8) | (DDR_DUAL_CHANNEL << 6) | (RF_DATA_WIDTH_X32 << 4) | DDR_CAPACITY_4GBIT,
}DDR_TYPE;

#endif
