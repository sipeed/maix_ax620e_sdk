#ifndef _EIP130_DRV_H_
#define _EIP130_DRV_H_
typedef enum {
	EIP130_SUCCESS = 0,
	EIP130_PARAM_FAIL = -1,
	EIP130_HW_FAIL = -2,
	EIP130_TIMEOUT_FAIL = -3,
	EIP130_LINK_FAIL = -4,
	EIP130_WRITE_FAIL = -5,
	EIP130_FW_LOAD_FAIL = -6,
	EIP130_LOCKOUT_FAIL = -7,
	EIP130_HASH_COMPUTE_FAIL = -8,
	EIP130_RSA_VERIFY_FAIL = -9,
} EIP130_STAUS_T;

int eip130_init(unsigned long fw_addr, int size, int use_dma_copy_fw);
int aes_ecb_decrypto(int *key, unsigned long src_addr, unsigned long dst_addr, int size);
int eip130_sha256_init(const char *data, char *output, int size);
int eip130_sha256_update(const char *data, char *output, int size, int *digest);
int eip130_sha256_final(const char *data, char *output, int size, int *digest, int all_size);
int eip130_sha256(const char *data, char *output, int size);
int eip130_rsa_verify(char *public_key, char *signature, int *hash, int key_bits);
#endif
