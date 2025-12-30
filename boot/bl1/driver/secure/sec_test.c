#include "cmn.h"
#include "efuse_drv.h"
#include "secure.h"
#include "eip130_drv.h"
//#include "../debug_flag.h"
// #define SPL_ADDR (0x402000000)
extern int rsa_test();
extern int rsa_sign_test();
extern int sha256_test();
u32 const eip_fw_signed[] = {
#include "eip130_fw.h"
};
#if 0
char const fdl1_signed[] = {
//#include "../tool/build_spl/fdl1_signed.h"
};
static const int aes_key[8] = {
    0, 0, 0, 0, 0, 0, 0, 0,
};
static char const uboot_data[] = {
#include "../../tools/u-boot.h"
};
static char const uboot_enc_signed_data[] = {
#include "../../tools/u-boot_enc_signed.h"
};
static char uboot_decode_data[0x100000];

//static int copy_to(char *dst, char *src, int size)
//{
//    int i;
//    for (i = 0; i < size; i++) {
//        dst[i] = src[i];
//    }
//    return 0;
//}
static int data_compare(char *src, char *dst, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        if (src[i] != dst[i]) {
            return -1;
        }
    }
    return 0;
}
static int efuse_blk_write()
{
    int ret;
    ret = efuse_init();
    if (ret != 0) {
        return SECURE_EFUSE_INIT_FAIL;
    }
    //enable secureboot;
    efuse_write(13, 1 << 26);
    //write uid
    efuse_write(0, 0x11111111); //uid[0]
    efuse_write(1, 0x22222222); //uid[0]
    //write public key hash to efuse
    efuse_write(17, 0x4cf978e2);
    efuse_write(18, 0x4c4db450);
    efuse_write(19, 0x39209ce4);
    efuse_write(20, 0x2c09f0bc);
    efuse_write(21, 0xaad074c9);
    efuse_write(22, 0x8ddea6a3);
    efuse_write(23, 0xa01d74ba);
    efuse_write(24, 0x6a43debc);
    return 0;
}

static int sec_normal_test()
{
    int ret;
    ret = secure_init((u64)eip_fw_signed, sizeof(eip_fw_signed), 0);
    if (ret != SECURE_SUCCESS) {
        while (1) ;
    }
    //ret = secureboot((u32)fdl1_signed, sizeof(fdl1_signed));
    if (ret != SECURE_SUCCESS) {
        while (1) ;
    }
    return 0;
}
#endif
#ifdef SEC_TEST
static int sec_token_test()
{
    int ret;
    writel(0x0, 0x40000000);
    ret = secure_init((u64)eip_fw_signed, sizeof(eip_fw_signed), 0);
    if (ret != SECURE_SUCCESS) {
        writel(0x1, 0x40000000);
        return -1;
        // while (1) ;
    }
    ret = sha256_test();
    if (ret != 0) {
        writel(0x2, 0x40000000);
        return -1;
        // while (1);
    }
    ret = rsa_test();
    if (ret != 0) {
        writel(0x3, 0x40000000);
        return -1;
        // while (1);
    }
    writel(0x66666666, 0x40000000);
    // ret = rsa_sign_test();
    // if (ret != 0) {
    //     while (1);
    // }
    return 0;
}
#endif
#if 0
static int test_efuse_data[96] = {
    0x2D87C4C1, 0xD6A5F7FA, 0xAEA2B8BF, 0xF355ED13,
    0xAE31B2F8, 0x09E6718A, 0xF0995D80, 0x81AA21CF,
    0x72D11E34, 0x4468DE11, 0x248D690E, 0x6895C85B,
    0x00116468, 0x9842F634, 0xFC8D7A19, 0x1603033B,
    0x0A956A67, 0x1FF28E31, 0xA38178CC, 0xA218BAEC,
    0x11714E78, 0x0871D5B0, 0xF4952499, 0xF5B6315E,
    0x3691D253, 0xD73849F9, 0xE6985469, 0x19F656EE,
    0x3C0C6B8E, 0xD32C56A4, 0x274BDF4C, 0xCA93B7C6,
    0x7D4223BE, 0x2A4E93DC, 0x5A4566C7, 0x3231FD23,
    0x0EECACDC, 0x6CDBDFFC, 0x59DAF08A, 0x6482A02C,
    0xE4D5CFE8, 0x789E8183, 0x3978A34B, 0xFC491105,
    0x3EA2ADD4, 0x01D6128E, 0xEF187ABB, 0x6A19F006,
    0x05C4B651, 0x0ABBFBED, 0x2A029DA4, 0x671D9A50,
    0x83703AB5, 0xED30A103, 0x2DAAA039, 0x28B3FE18,
    0x686A58C2, 0xC14BEA83, 0x13752255, 0xF4E3D5D9,
    0x3DC35150, 0x1DC02D15, 0x665ED490, 0x1DC14364,
    0x2F613CD3, 0x3F3075D0, 0x207AD435, 0x50A4C8CF,
    0x3C7D12C3, 0x93512E9B, 0x08587518, 0x4E459F44,
    0xC3F025E2, 0x94FB316F, 0x70475C53, 0x00B674B1,
    0x7067ED65, 0x17122673, 0x507901DB, 0xF0408256,
    0x887B3EFC, 0xEC5556FE, 0xC80995F3, 0x17220838,
    0x999FD5B9, 0xE806CC74, 0xF6D8408C, 0x1B24236D,
    0x88F74CD6, 0x4576E809, 0x3F58C375, 0xDF1C0C4C,
    0xF90A6D5D, 0x6C733F5D, 0xDF2A7B76, 0x7A42AA7F,
};
static int test_efuse_read_data[96];
static int efuse_test()
{
    u32 i;
    u32 ret;

    ret = efuse_init();
    if (ret != EFUSE_SUCCESS) {
        while (1);
    }
    for (i = 0; i < 96; i++) {
	if(i == 13) {
	    continue;
	}
        ret = efuse_write(i, test_efuse_data[i]);
        if (ret != 0) {
            while (1);
        }
    }
    for (i = 0; i < 96; i++) {
	if(i == 13) {
	    continue;
	}
        ret = efuse_read(i, (int *)&test_efuse_read_data[i]);
        if (ret != 0) {
            while (1);
        }
        if (test_efuse_read_data[i] != test_efuse_data[i]) {
            while (1);
        }
    }
    ret = efuse_deinit();
    return 0;
}
static int aes_multi_blk_test(char *key, char *src, char *dst, int size, int blk_size)
{
    int blk_cnt;
    int i;
    int ret;
    for (i = 0; i < size; i++) {
        dst[i] = 0;
    }
    blk_cnt = size / blk_size;
    for (i = 0; i < blk_cnt; i++) {
        ret = aes_ecb_decrypto((int *)key, (u64)src + i * blk_size, 
            (u64)dst + i * blk_size, blk_size);
        if (ret < 0) {
            while (1);
            return -1;
        }
    }
    if (size % blk_size) {
        ret = aes_ecb_decrypto((int *)key, (u64)src + blk_cnt * blk_size, 
            (u64)dst + blk_cnt * blk_size, size % blk_size);
        if (ret < 0) {
            while (1);
            return -1;
        }
    }
    return 0;
}
static int aes_test()
{
    int ret;
    int size;
    size = (sizeof(uboot_data) + 31) & 0xffffe0;
    ret = secure_init((u32)eip_fw_signed, sizeof(eip_fw_signed));
    if (ret != SECURE_SUCCESS) {
        while (1) ;
    }
    ret = aes_ecb_decrypto(aes_key, (u32)uboot_enc_signed_data + 0x400, (u32)uboot_decode_data, size);
    if (ret < 0) {
        while (1);
        return -1;
    }
    ret = data_compare(uboot_decode_data, (char *)uboot_data, sizeof(uboot_data));
    if (ret < 0) {
        while (1);
        return -1;
    }
    ret = aes_multi_blk_test(aes_key, (u32)uboot_enc_signed_data + 0x400, (u32)uboot_decode_data, size, 0x8000);
    if (ret < 0) {
        while (1);
        return -1;
    }
    ret = data_compare(uboot_decode_data, (char *)uboot_data, sizeof(uboot_data));
    if (ret < 0) {
        while (1);
        return -1;
    }
    return ret;
}
#endif
#ifdef SEC_TEST
int sec_test()
{
    //aes_test();
    //efuse_test();
    //efuse_blk_write();
    return sec_token_test();
    // sec_normal_test();
    // return 0;
}
#endif
