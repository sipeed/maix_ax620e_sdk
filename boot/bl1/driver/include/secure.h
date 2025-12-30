#ifndef _SECURE_H_
#define _SECURE_H_

#define HASH_BLK_NUM 8
#define PUBKEY_HASH_BLK_START 15 //MC20 set 17
#define AESKEY_BLK_START 23 //MC20 set 64
typedef enum {
	SECURE_SUCCESS = 0,
	SECURE_EFUSE_INIT_FAIL = -1,
	SECURE_EFUSE_READ_FAIL = -2,
	SECURE_EIP130_INIT_FAIL = -3,
	SECURE_HASH_COMPUTE_FAIL = -4,
	SECURE_PUBLIC_KEY_CHECK_FAIL = -5,
	SECURE_RSA_COMPUTE_FAIL = -6,
	SECURE_IMG_VERIFY_FAIL = -7,
} SECURE_STAUS_T;

struct rsa2048_key {
	u32 key_n_header;
	u32 rsa_key_n[64];
	u32 key_e_header;
	u32 rsa_key_e;
	u32 reserved[32];
};
struct rsa3072_key {
	u32 key_n_header;
	u32 rsa_key_n[96];
	u32 key_e_header;
	u32 rsa_key_e;
};

extern int hash_digest[];
extern int aes_value[];

int is_secure_enable();
int get_aes_key(int *key);
SECURE_STAUS_T secure_init(unsigned long fw_addr, int size, int use_dma_copy_fw);
SECURE_STAUS_T public_key_verify(unsigned long key_addr, int size);
int cipher_aes_ecb_decrypto(int *key, unsigned long src_addr, unsigned long dst_addr, int size);
int cipher_sha256_init(const char *data, char *output, int size);
int cipher_sha256_update(const char *data, char *output, int size, int *digest);
int cipher_sha256_final(const char *data, char *output, int size, int *digest, int all_size);
int cipher_sha256(const char *data, char *output, int size);
int cipher_rsa_verify(char *public_key, char *signature, int *hash, int key_bits);
#endif
