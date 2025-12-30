#include <common.h>
#include <malloc.h>
#include <mtd.h>
#include <blk.h>
#include <asm/io.h>
#include <memalign.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <linux/dma-mapping.h>
#include "secureboot.h"
#include "../cipher/ax_cipher_api.h"
#include "../cipher/ax_base_type.h"
#include <asm/arch/boot_mode.h>

#define HASH_BLK_NUM					8
#define IMG_HASH_BITS					256
#define IMG_RSA_KEY_E_SIZE				4
#define IMG_SIGN_RSA_SCHEME				AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA256
#define IMG_AES_BITS					256

extern misc_info_t *misc_info;

static int eip130_sha256(char *data, char *output, int size)
{
	AX_CIPHER_HASH_CTL_S hashCtl;
	AX_CIPHER_HANDLE handle;
	AX_S32 ret;

	hashCtl.hashType = AX_CIPHER_ALGO_HASH_SHA256;
	hashCtl.hmacKey = 0;
	hashCtl.hmackeyLen = 0;
	ret = AX_CIPHER_HashInit(&hashCtl, &handle);
	if (ret != AX_CIPHER_SUCCESS) {
		printf("%s, Hash init failed\n", __func__);
		return -1;
	}
	ret = AX_CIPHER_HashFinal(handle, (unsigned char *)data, size, (unsigned char *)output);
	if (ret != AX_CIPHER_SUCCESS) {
		printf("%s, Hash final failed\n", __func__);
		return -1;
	}

	return 0;
}

static void reverse_array(unsigned char *array, u32 len)
{
	unsigned char *head, *tail;
	char temp;
	unsigned int loop = len / 2;

	head = array;
	tail = array + len - 1;
	while (loop--) {
		temp = *head;
		*head = *tail;
		*tail = temp;
		head++;
		tail--;
	}
}

static void reverse_key(struct rsa_key *key, u32 key_bits)
{
	unsigned char *head;
	unsigned int key_size_byte = key_bits / 8;

	/* reverse key_n */
	head = (unsigned char *)key->rsa_key_n;
	reverse_array(head, key_size_byte);

	/* reverse key_e */
	head = (unsigned char *)&key->rsa_key_e;
	reverse_array(head, IMG_RSA_KEY_E_SIZE);
}

static void reverse_signature(char *sig, u32 bits)
{
	unsigned int size_in_byte = bits / 8;

	reverse_array((unsigned char *)sig, size_in_byte);
}

static int img_verify(struct rsa_key *key, char *img, char *sig, u32 size, u32 key_bits)
{
	int ret;
	AX_CIPHER_SIG_DATA_S signature;
	AX_CIPHER_RSA_PUBLIC_KEY publicKey;
	struct rsa_key *p_key;
	char *p_sig;
	unsigned int sig_size_byte = key_bits / 8;

	p_key = (struct rsa_key *)malloc(sizeof(struct rsa_key));
	if (!p_key) {
		pr_err("%s cannot malloc rsa_key size\n", __func__);
		return -1;
	}
	p_sig = (char *)malloc(sig_size_byte);
	if (!p_sig) {
		free(p_key);
		pr_err("%s cannot malloc signature size\n", __func__);
		return -1;
	}

	memcpy((void *)p_key, (void *)key, sizeof(struct rsa_key));
	memcpy((void *)p_sig, (void *)sig, sig_size_byte);

	reverse_key(p_key, key_bits);
	reverse_signature(p_sig, key_bits);

	publicKey.hashBits = IMG_HASH_BITS;
	publicKey.modulusBits = key_bits;
	publicKey.modulusData = (AX_U8 *)p_key->rsa_key_n;
	publicKey.publicExponentBytes = IMG_RSA_KEY_E_SIZE;
	publicKey.exponentData = (AX_U8 *)&p_key->rsa_key_e;
	publicKey.enScheme = IMG_SIGN_RSA_SCHEME;
	signature.data = (AX_U8 *)p_sig;
	signature.len = key_bits / 8;

	ret = AX_CIPHER_RsaVerify(&publicKey, (AX_U8 *)img, size, &signature);

	free(p_key);
	free(p_sig);

	return ret;
}

SECURE_STAUS_E rsa_img_verify(struct rsa_key *key, char *img, char *sig, u32 img_sz, u32 key_bits)
{
	int ret;

	ret = img_verify(key, img, sig, img_sz, key_bits);
	if (ret != SECURE_SUCCESS) {
		return SECURE_IMG_VERIFY_FAIL;
	}
	return SECURE_SUCCESS;
}

int secure_verify(char *name, struct rsa_key *key, char *img, char *sig, u32 img_sz, u32 key_bits)
{
	int ret;

	if (is_secure_enable() == 0) {
		return SECURE_SUCCESS;
	}
	ret = rsa_img_verify(key, img, sig, img_sz, key_bits);
	if (ret != SECURE_SUCCESS) {
		pr_err("check failed %s, ret = %d\n", name, ret);
		return ret;
	}
	return 0;
}

static int calc_check_sum(int *data, int size)
{
	int count = size / 4;
	int i, sum = 0;
	for (i = 0; i < count; i++) {
		sum += *(data + i);
	}
	return sum;
}

/* #define IMG_HEADER_MAGIC_DATA		(0x55543322) */
int verify_image_header(struct img_header *img_hdr, int size)
{
	if (img_hdr->magic_data != IMG_HEADER_MAGIC_DATA)
		return -1;
	int check_sum = calc_check_sum((int *)&img_hdr->capability, size - 8);
	if (check_sum != img_hdr->check_sum) {
		printf("image_header verify failed\n");
		return -1;
	}
	return 0;
}

int public_key_verify(char *key_addr, int size)
{
	int ret;
	int i;
	int hw_key_hash[HASH_BLK_NUM];

	ret = eip130_sha256(key_addr, (char * )hw_key_hash, size);
	if (ret != 0) {
		printf("%s, eip130_sha256 called failed\n", __func__);
		return -1;
	}

	for (i = 0; i < HASH_BLK_NUM; i++) {
		if (hw_key_hash[i] != misc_info->pub_key_hash[i]) {
			return SECURE_HASH_COMPUTE_FAIL;
		}
	}
	return SECURE_SUCCESS;
}

static int aes_decrypt(int *key, unsigned long src_addr, unsigned long dst_addr, int size)
{
	int ret = 0;
	AX_CIPHER_CTRL_S ctrl;
	AX_CIPHER_HANDLE handler;

	ctrl.alg = AX_CIPHER_ALGO_CIPHER_AES;
	ctrl.workMode = AX_CIPHER_MODE_CIPHER_ECB;
	ctrl.pKey = (AX_U8 *)key;
	ctrl.keySize = IMG_AES_BITS / 8;

	ret = AX_CIPHER_CreateHandle(&handler, &ctrl);
	if (ret)
		return -1;

	ret = AX_CIPHER_Decrypt(handler, (AX_U8 *)src_addr, (AX_U8 *)dst_addr, (AX_U32)size);
	if (ret)
		return -1;

	return ret;
}

int cipher_sha256(char *data, char *output, int size)
{
	return eip130_sha256(data, output, size);
}

int cipher_aes_ecb_decrypto(int *aes_key, unsigned long src_addr, unsigned long dst_addr, int size)
{
	int ret = 0;
	int blk_cnt, blk_size;
	void *data = NULL;
	char *src = (char *)src_addr;
	char *dst = (char *)dst_addr;

	if (size > AX_CIPHER_CRYPTO_MAX_SIZE) {
		blk_size = AX_CIPHER_CRYPTO_MAX_SIZE;
	} else {
		blk_size = size;
	}
	blk_cnt = size / blk_size;
	data = malloc(blk_size);
	while (blk_cnt--) {
		memcpy(data, src, blk_size);
		flush_cache((unsigned long)src, blk_size);
		ret = aes_decrypt(aes_key, (unsigned long)src, (unsigned long)dst, blk_size);
		if (ret < 0) {
			printf("erro \n");
			free(data);
			return ret;
		}
		src += blk_size;
		dst += blk_size;
		size -= blk_size;
	}
	if (size) {
		memcpy(data, src, size);
		flush_cache((unsigned long)src, size);
		ret = aes_decrypt(aes_key, (unsigned long)src, (unsigned long)dst, size);
		if (ret < 0) {
			printf("erro \n");
			free(data);
			return ret;
		}
	}
	free(data);
	return ret;
}
