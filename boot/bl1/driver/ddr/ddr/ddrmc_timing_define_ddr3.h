#ifndef __DDRMC_TIMING_DEFINE_DDR3__
#define __DDRMC_TIMING_DEFINE_DDR3__
#include "ddr_init_config.h"

const unsigned int ddr3mc_timing[4][29] =
{
    #if DDR3_CONFIG_800
    {
        0x00030003, //t_rp             t_rpab
        0x00040803, //t_rtp    t_mrd   t_wr       t_rcd
        0x00030205, //t_rtw            t_cke      t_xp
        0x00010000, //t_wtw    t_rtr   t_wtw_cs   t_rtr_cs
        0x00050202, //t_wtr_cs t_wtr   t_rrd      t_ccd
        0x00070203, //t_wtr_l          t_rrd_l    t_ccd_l
        0x00000046, //t_rfcpb          t_rfc
        0x00000048, //t_ccdmw          t_xsr
        0x03030203, //t_cksrx  t_cksre t_ppd      t_escke
        0x02000E07, //t_fc     t_faw              t_ras
        0x00000126,//t_dllk
        0x01010000,//dqs_oe   data_oe wdm_lat    wdata_lat
        0x00000003,//                 data_ie_x8 data_ie_x16
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000400,
        0x00001000,
        0x00000000,
        0x00000022
    },
    #endif
    #if DDR3_CONFIG_1600
    {
        0x00070006, //t_rp             t_rpab
        0x00080c06, //t_rtp    t_mrd   t_wr       t_rcd
        0x00030808, //t_rtw            t_cke      t_xp
        0x00010000, //t_wtw    t_rtr   t_wtw_cs   t_rtr_cs
        0x00070202, //t_wtr_cs t_wtr   t_rrd      t_ccd
        0x00070202, //t_wtr_l          t_rrd_l    t_ccd_l
        0x0000008c, //t_rfcpb          t_rfc
        0x00000090, //t_ccdmw          t_xsr
        0x05030203, //t_cksrx  t_cksre t_ppd      t_escke
        0x00000e0d, //t_fc     t_faw              t_ras
        0x00000100,//t_dllk
        0x03030202,//dqs_oe   data_oe wdm_lat    wdata_lat
        0x00000004,//                 data_ie_x8 data_ie_x16
        0x00000000,
        0x00000000,
        0x00000000,
        0x00010000,
        0x00000400,
        0x00001000,
        0x00000000,
        0x00000022
    },
    #endif
    #if DDR3_CONFIG_1866
    {
        0x00070007, //t_rp             t_rpab
        0x02070D07, //t_rtp    t_mrd   t_wr       t_rcd
        0x00040406, //t_rtw            t_cke      t_xp
        0x00010000, //t_wtw    t_rtr   t_wtw_cs   t_rtr_cs
        0x00080302, //t_wtr_cs t_wtr   t_rrd      t_ccd
        0x00080303, //t_wtr_l          t_rrd_l    t_ccd_l
        0x000000A4, //t_rfcpb          t_rfc
        0x000000A8, //t_ccdmw          t_xsr
        0x07070207, //t_cksrx  t_cksre t_ppd      t_escke
        0x04B00E0F, //t_fc     t_faw              t_ras
        0x00000126,//t_dllk
        0x03030202,//dqs_oe   data_oe wdm_lat    wdata_lat
        0x00E00005,//                 data_ie_x8 data_ie_x16
        0x00000000,
        0x00000000,
        0x00000000,
        0x00010000,
        0x00000400,
        0x00001000,
        0x00000000,
        0x00000022
    },
    #endif
    #if DDR3_CONFIG_2133
    {
        0x00080008, // D_DDRMC_TMG0  t_rp             t_rpab
        0x02081108, // D_DDRMC_TMG1  t_rtp    t_mrd   t_wr       t_rcd
        0x00040406, // D_DDRMC_TMG2  t_rtw            t_cke      t_xp
        0x00010000, // D_DDRMC_TMG3  t_wtw    t_rtr   t_wtw_cs   t_rtr_cs
        0x00090302, // D_DDRMC_TMG4  t_wtr_cs t_wtr   t_rrd      t_ccd
        0x00090302, // D_DDRMC_TMG5  t_wtr_l          t_rrd_l    t_ccd_l
        0x0000008c, // D_DDRMC_TMG6  t_rfcpb          t_rfc
        0x000000BB, // D_DDRMC_TMG7  t_ccdmw          t_xsr
        0x08080208, // D_DDRMC_TMG8  t_cksrx  t_cksre t_ppd      t_escke
        0x00001012, // D_DDRMC_TMG9  t_fc     t_faw              t_ras
        0x00000126,//D_DDRMC_TMG10  t_dllk
        0x04040303,//D_DDRMC_TMG11  dqs_oe   data_oe wdm_lat    wdata_lat
        0x00000006,//D_DDRMC_TMG12                   data_ie_x8 data_ie_x16
        0x00000000,//D_DDRMC_TMG13
        0x00000000,//D_DDRMC_TMG14
        0x00000000,//D_DDRMC_TMG15
        0x00010000,//D_DDRMC_TMG16
        0x00000400,//D_DDRMC_TMG17
        0x00001000,//D_DDRMC_TMG18
        0x00000000,//D_DDRMC_TMG19
        0x00000022 //D_DDRMC_TMG20
    }
    #endif
};

#endif
