#ifndef __ERRCODE_
#define __ERRCODE_

//both emmc/sd
#define ECARDDET_FAIL               1
#define EINHIBIT_TIMEOUT            2
#define ECMDCOMPLETE_TIMEOUT        3
#define ECMD_RESP                   4
#define ETRANS_ERRINT               5
#define ETRANS_TIMEOUT              6
#define ECHG_CARD_FREQ              7
#define ESEC_BOOT                   8
#define EMMC_OCR                    9
#define EMMC_SET_RCA                10
#define EMMC_CSD                    11
#define EMMC_SELECT_CARD            12
#define EMMC_RSTOP_TRANS            13
#define EMMC_SET_BLKLEN             14
#define EMMC_READ_BLK               15
#define EMMC_WSTOP_TRANS            16
#define EMMC_GET_STATUS             17
#define EMMC_BLKCNT_OVER            18

#define EMMC_WRITE_BLK              15

#define EPHY_PWR_GOOD               19
#define EBL_MHEADER                 20
#define EBL_BHEADER                 21
#define EBL_MDATA                   22
#define EBL_BDATA                   23
#define EFW_MHEADER                 24
#define EFW_BHEADER                 25
#define EFW_MDATA                   26
#define EFW_BDATA                   27
//emmc
#define EEMMC_CID                   28
#define EEMMC_EXT_CSD               29
#define EEMMC_CMD_SWITCH            30
#define EEVERSION_L4                31
#define EINVAL                      22
#define ENOTSUPP                    524

#define EIO                         555 /* I/O error */

//sd
#define ESD_OCR_TIMEOUT             28
#endif