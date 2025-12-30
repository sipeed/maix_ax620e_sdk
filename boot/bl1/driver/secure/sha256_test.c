#include "cmn.h"
#include "eip130_drv.h"
const char _2k_data[] = {
#include "2k.h"
};

const char _110k_data[] = {
#include "110k.h"
};
char _0B_hash_result[] = {
    0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
    0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
    0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
    0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
};
char _2k_hash_result[] = {
    0xf8, 0xf0, 0xbe, 0x25, 0x92, 0xe2, 0xa0, 0x5f,
    0xc4, 0x53, 0xbf, 0xb4, 0x24, 0x18, 0xd7, 0xa5,
    0x0b, 0x8c, 0x0c, 0x54, 0x46, 0x46, 0x6e, 0xae,
    0xb8, 0x2c, 0x62, 0xef, 0x9e, 0x1f, 0xf7, 0xcc
};
char _110k_hash_result[] = {
    0xfd, 0xec, 0x81, 0xd8, 0x6b, 0xf0, 0x53, 0x7b,
    0xb3, 0xcc, 0x68, 0x8e, 0x6e, 0xb3, 0x28, 0x8c,
    0xd8, 0xfd, 0x9a, 0x49, 0x8a, 0xec, 0x65, 0xee,
    0xda, 0x28, 0xa5, 0xf3, 0xad, 0x4a, 0x12, 0xf3
};
int compare_data(char *data0, char *data1, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        if (data0[i] != data1[i]) {
            return -1;
        }
    }
    return 0;
}
char hash_result[32];
static int sha256_multi_test()
{
    int ret;
    int data_len = 110 * 1024;
    int blk_size = 12 * 1024;
    int blk_cnt = data_len / blk_size;
    int i;
    if ((data_len % blk_size) || (data_len == 0)) {
        blk_cnt ++;
    }
    if (blk_cnt == 1) {
        eip130_sha256(_110k_data, hash_result, data_len);
    } else {
        ret = eip130_sha256_init(_110k_data, hash_result, blk_size);
        for (i = 1; i < (blk_cnt - 1); i++) {
            ret = eip130_sha256_update(_110k_data + i * blk_size, hash_result,
                blk_size, (int *)hash_result);
            if (ret < 0) {
                return -1;
            }
        }
        ret = eip130_sha256_final(_110k_data + i * blk_size, hash_result,
            data_len - i * blk_size, (int *)hash_result, data_len);
        if (ret < 0) {
            return -1;
        }
    }
    return 0;
}
int sha256_test()
{
    int ret;

    writel(0x0, 0x4840004);
    ret = eip130_sha256(_2k_data, hash_result, 2 * 1024);
    if (ret < 0) {
        writel(0x1, 0x4840004);
        return -1;
    }
    ret = compare_data(_2k_hash_result, hash_result, 32);
    if (ret < 0) {
        writel(0x2, 0x4840004);
        return -1;
    }
    ret = eip130_sha256(_110k_data, hash_result, 110 * 1024);
    if (ret < 0) {
        writel(0x3, 0x4840004);
        return -1;
    }
    ret = compare_data(_110k_hash_result, hash_result, 32);
    if (ret < 0) {
        writel(0x4, 0x4840004);
        return -1;
    }
    ret = sha256_multi_test();
    if (ret < 0) {
        writel(0x5, 0x4840004);
        return -1;
    }
    ret = compare_data(_110k_hash_result, hash_result, 32);
    if (ret < 0) {
        writel(0x6, 0x4840004);
        return -1;
    }
    ret = eip130_sha256(0, hash_result, 0);
    if (ret < 0) {
        writel(0x7, 0x4840004);
        return -1;
    }
    ret = compare_data(_0B_hash_result, hash_result, 32);
    if (ret < 0) {
        writel(0x8, 0x4840004);
        return -1;
    }
    writel(0x66666666, 0x4840004);
    return 0;
}
