#include "cmn.h"
#include "eip130_drv.h"
#include "efuse_drv.h"
#include "chip_reg.h"
#include "secure.h"
#include "boot.h"
#define SECURE_BOOT_EN (1 << 26)
#define HASH_MAX_BLOCK_SIZE         (0x100000)
static int efuse_hash_val[HASH_BLK_NUM];
static int hw_key_hash[HASH_BLK_NUM];
int hash_digest[8];
int aes_value[8];

SECURE_STAUS_T public_key_verify(unsigned long key_addr, int size)
{
    int ret;
    int i;
    misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;
    ret = efuse_init();
    if (ret != 0) {
        return SECURE_EFUSE_INIT_FAIL;
    }
#ifdef SECURE_BOOT_TEST
    for (i = 0; i < HASH_BLK_NUM; i++) {
        efuse_hash_val[i] = misc_info->pub_key_hash[i];
    }
#else
    for (i = 0; i < HASH_BLK_NUM; i++) {
        ret = efuse_read(PUBKEY_HASH_BLK_START + i, &efuse_hash_val[i]);
        if (ret != SECURE_SUCCESS) {
            return SECURE_EFUSE_READ_FAIL;
        }
    }
#endif
    ret = eip130_sha256((char *)key_addr, (char *)hw_key_hash, size);
    if (ret != 0) {
        return SECURE_HASH_COMPUTE_FAIL;
    }
    for (i = 0; i < HASH_BLK_NUM; i++) {
        if (hw_key_hash[i] != efuse_hash_val[i]) {
            return SECURE_PUBLIC_KEY_CHECK_FAIL;
        }
    }
#ifndef SECURE_BOOT_TEST
    /* store pub key hash to iram0 */
    for (i = 0; i < 8; i++) {
		misc_info->pub_key_hash[i] = hw_key_hash[i];
	}
#endif
    return SECURE_SUCCESS;
}

int get_aes_key(int *key)
{
    int i;
    int ret;
    efuse_init();
    for(i = 0; i < 8; i++) {
        ret = efuse_read(AESKEY_BLK_START + i, &key[i]);
        if (ret != SECURE_SUCCESS) {
            return SECURE_EFUSE_READ_FAIL;
        }
    }
    return 0;
}

int is_secure_enable()
{
    int val;
    val = readl(COMM_SYS_BOND_OPT);
    if (val & SECURE_BOOT_EN) {
        return 1;
    }
    return 0;
}

#ifdef SECURE_BOOT_TEST
SECURE_STAUS_T secure_init(unsigned long fw_addr, int size, int use_dma_copy_fw)
{
    int ret;
    ret = eip130_init(fw_addr, size, use_dma_copy_fw);
    if (ret != 0) {
        return SECURE_EIP130_INIT_FAIL;
    }
    return SECURE_SUCCESS;
}

#endif
int cipher_aes_ecb_decrypto(int *key, unsigned long src_addr, unsigned long dst_addr, int size)
{
	return aes_ecb_decrypto(key, src_addr, dst_addr, size);
}
int cipher_sha256_init(const char *data, char *output, int size)
{
	return eip130_sha256_init(data, output, size);
}
#ifdef SHA256_TEST
int cipher_sha256_update(const char *data, char *output, int size, int *digest)
{
	return eip130_sha256_update(data, output, size, digest);
}
int cipher_sha256_final(const char *data, char *output, int size, int *digest, int all_size)
{
	return eip130_sha256_final(data, output, size, digest, all_size);
}
#endif
int cipher_sha256(const char *data, char *output, int size)
{
    int i;
    int ret;
    char *data_ptr = (char *)data;
    int remain_size = size;
    int cur_size;
    int initial = 1;
    for (i = 0; i < 32; i++) {
        output[i] = 0;
    }
    if (size <= HASH_MAX_BLOCK_SIZE) {
        return eip130_sha256(data_ptr, output, size);
    }
    while (remain_size) {
        if (remain_size > HASH_MAX_BLOCK_SIZE) {
            cur_size = HASH_MAX_BLOCK_SIZE;
            if (initial) {
                ret = eip130_sha256_init(data_ptr, output, cur_size);
            } else {
                ret = eip130_sha256_update(data_ptr, output, cur_size, (int *)output);
            }
            if (ret != EIP130_SUCCESS) {
                return ret;
            }
        } else {
            cur_size = remain_size;
            return eip130_sha256_final(data_ptr, output, cur_size, (int *)output, size);
        }
        initial = 0;
        remain_size -= cur_size;
        data_ptr += cur_size;
    }
    return EIP130_SUCCESS;
}
int cipher_rsa_verify(char *public_key, char *signature, int *hash, int key_bits)
{
	return eip130_rsa_verify(public_key, signature, hash, key_bits);
}
