/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ax_base_type.h"
#include "ax_cipher_api.h"
#include "ax_base_type.h"
#include "eip130_drv.h"
#include "ax_cipher_api.h"
#include "ax_cipher_token_crypto.h"
#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <kernel/spinlock.h>
#include <util.h>
#include "ax_keyotpblob.h"
#define AX_IDENTITY  (0x41584552)
#define PUBLIC_POLICY 0x1000000000000000L
#define PUBLIC_DATA_LEN 0x10
#define PUBLIC_DATA_BLOB_LEN 0x20
#define CMAC_BLK_SIZE 16
#define PUBLIC_AES_KEY_ASSET_NUMBER 1
#define PUBLIC_DATA_ASSET_NUMBER 2
#define MAX_CMAC_BLK_SIZE 512
#define MAX_TRNG_CONFIG_TIMES 10
#define TRNG_DUTY_CYCLES 1

typedef struct ax_cipher_chn_s {
	bool is_used;
	ax_cipher_ctrl_s cipher_ctrl;
} ax_cipher_chn_s;

#define AX_CIPHER_MAX_CHN 4
#define HASH_CHANNAL_MAX_NUM         (8)
#define AX_CIPHER_RANDOM_MAX_BLK_SIZE (0xFFFF)
#define AX_CIPHER_CRYPTO_MAX_SIZE 0xFFF00	//for AES-ICM max block size must be little than 1MB

static ax_cipher_chn_s s_cipher_chn[AX_CIPHER_MAX_CHN];
//static uint32_t trng_configed = 0;
static Eip130Token_Command_t command_token;
static Eip130Token_Result_t result_token;
static unsigned int cipher_lock = SPINLOCK_UNLOCK;
static uint32_t axCipherExceptions;
static int32_t ax_cipher_system_reset(void);
static uint32_t ax_cipher_trng_config(uint8_t auto_seed, uint16_t sample_cycles, uint8_t sample_div, uint8_t noise_blocks, bool freseed);

int32_t ax_cipher_init(void)
{
	int i;
	int ret;
	ret = ce_init();
	if (ret) {
		return ret;
	}
	for(i = 0; i < MAX_TRNG_CONFIG_TIMES; i ++) {
		ret = ax_cipher_trng_config(0, TRNG_DUTY_CYCLES, 0, 8, 1);
		if(ret == AX_CIPHER_SUCCESS) {
			return 0;
		}
		ax_cipher_err("ce system reset %d, %d\n", i, ret);
		ax_cipher_system_reset();
	}
	for(i = 0; i < MAX_TRNG_CONFIG_TIMES; i ++) {
		ce_reinit();
		ret = ax_cipher_trng_config(0, TRNG_DUTY_CYCLES, 0, 8, 1);
		ax_cipher_err("ce reinit %d, ret: %d\n", i, ret);
		if(ret == AX_CIPHER_SUCCESS) {
			return 0;
		}
	}
	return AX_CIPHER_PANIC_ERROR;
}

int32_t ax_cipher_deinit(void)
{
#if 0
	ce_disable();
#endif
	return 0;
}

typedef struct {
	void *vir_addr;
	paddr_t pyhy_addr;
	uint32_t size;
} ax_cipher_cmm_t;

static int32_t ax_cipher_cmm_alloc(int32_t size, ax_cipher_cmm_t * cmm)
{
	if (!cmm) {
		return AX_CIPHER_INVALID_PARAMETER;
	}
	cmm->vir_addr = malloc(size);
	if (!cmm->vir_addr) {
		ax_cipher_err("Alloc memory failed, size:%x!\n", (unsigned int)size);
		return AX_CIPHER_NO_MEMORY;
	}
	cmm->pyhy_addr = virt_to_phys(cmm->vir_addr);
	if (!cmm->pyhy_addr) {
		ax_cipher_err("to_phys failed vir: %lx\n", (long unsigned int) cmm->vir_addr);
		return AX_CIPHER_NO_MEMORY;
	}
	cmm->size = size;
	return AX_CIPHER_SUCCESS;
}

static void ax_cipher_cmm_free(ax_cipher_cmm_t * cmm)
{
	if (cmm->vir_addr) {
		free(cmm->vir_addr);
		cmm->vir_addr = 0;
		cmm->pyhy_addr = 0;
	}
}

static void ax_cipher_cache_flush(void * buf, int32_t size)
{
	cache_op_inner(DCACHE_AREA_CLEAN, buf, size);
}

static void ax_cipher_cache_invalid(void * buf, int32_t size)
{
	cache_op_inner(DCACHE_AREA_INVALIDATE, buf, size);
}
static void ax_cipher_lock(void)
{
	axCipherExceptions = cpu_spin_lock_xsave(&cipher_lock);
}
static void ax_cipher_unlock(void)
{
	cpu_spin_unlock_xrestore(&cipher_lock, axCipherExceptions);
}

static int cipher_param_check(AX_CIPHER_ALGO_E algorithm)
{
	switch (algorithm) {
	case AX_CIPHER_ALGO_CIPHER_AES:
		break;
	default:
		return AX_CIPHER_INVALID_PARAMETER;
	}
	return AX_CIPHER_SUCCESS;
}

int32_t ax_cipher_create_handle(AX_CIPHER_HANDLE * p_cipher, const ax_cipher_ctrl_s * pst_cipher_ctrl)
{
	int i;
	if ((p_cipher == 0) || (pst_cipher_ctrl == 0)) {
		return AX_CIPHER_INVALID_PARAMETER;
	}
	for (i = 0; i < AX_CIPHER_MAX_CHN; i++) {
		if (s_cipher_chn[i].is_used == false) {
			break;
		}
	}
	if (cipher_param_check(pst_cipher_ctrl->alg) != AX_CIPHER_SUCCESS) {
		return AX_CIPHER_INVALID_PARAMETER;
	}
	memset(&s_cipher_chn[i], 0, sizeof(s_cipher_chn[i]));
	if (i < AX_CIPHER_MAX_CHN) {
		s_cipher_chn[i].is_used = true;
		s_cipher_chn[i].cipher_ctrl = *pst_cipher_ctrl;
		*p_cipher = i;
		return 0;
	}
	return AX_CIPHER_FULL_ERROR;
}

static int32_t ax_cipher_crypto_block(ax_cipher_chn_s * ph_cipher, uint8_t * src_addr, uint8_t * dest_addr, uint32_t byte_length, bool encrypt)
{
	AX_CIPHER_TokenModeCipher mode;
	int ret;
	switch (ph_cipher->cipher_ctrl.workMode) {
	case AX_CIPHER_MODE_CIPHER_ECB:
		mode = TOKEN_MODE_CIPHER_ECB;
		break;
	case AX_CIPHER_MODE_CIPHER_CBC:
		mode = TOKEN_MODE_CIPHER_CBC;
		break;
	case AX_CIPHER_MODE_CIPHER_CTR:
		mode = TOKEN_MODE_CIPHER_CTR;
		break;
	case AX_CIPHER_MODE_CIPHER_ICM:
		mode = TOKEN_MODE_CIPHER_ICM;
		break;
	case AX_CIPHER_MODE_CIPHER_F8:
		mode = TOKEN_MODE_CIPHER_F8;
		break;
	case AX_CIPHER_MODE_CIPHER_CCM:
		mode = TOKEN_MODE_CIPHER_CCM;
		break;
	case AX_CIPHER_MODE_CIPHER_XTS:
		mode = TOKEN_MODE_CIPHER_XTS;
		break;
	case AX_CIPHER_MODE_CIPHER_GCM:
		mode = TOKEN_MODE_CIPHER_GCM;
		break;
	default:
		return AX_CIPHER_INVALID_PARAMETER;
	}
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	Eip130Token_Command_Crypto_Operation(&command_token, EIP130TOKEN_CRYPTO_ALGO_AES, (uint8_t) mode, encrypt, byte_length);
	Eip130Token_Command_Crypto_SetDataAddresses(&command_token, (uint64_t) src_addr, byte_length, (uint64_t) dest_addr, byte_length);
	Eip130Token_Command_Crypto_CopyKey(&command_token, ph_cipher->cipher_ctrl.pKey, ph_cipher->cipher_ctrl.keySize);
	Eip130Token_Command_Crypto_SetKeyLength(&command_token, ph_cipher->cipher_ctrl.keySize);
	if (mode != TOKEN_MODE_CIPHER_ECB) {
		// From token
		Eip130Token_Command_Crypto_CopyIV(&command_token, ph_cipher->cipher_ctrl.pIV);
	}
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	return ret;
}

static int32_t ax_cipher_crypto(AX_CIPHER_HANDLE p_cipher, uint8_t * src_addr, uint8_t * dest_addr, uint32_t byte_length, bool encrypt)
{
	ax_cipher_chn_s *ph_cipher = (ax_cipher_chn_s *) & s_cipher_chn[p_cipher];
	int32_t algo_block_size = 16;
	int32_t blkCnt = 0;
	int32_t blk_size = 0;
	ax_cipher_cmm_t cmm;
	ax_cipher_cmm_t cmm_dst;
	void *phy_addr;
	void *vir_addr;
	int ret;
	if ((byte_length % algo_block_size) != 0) {
		return AX_CIPHER_INVALID_PARAMETER;
	}
	if (byte_length > AX_CIPHER_CRYPTO_MAX_SIZE) {
		blk_size = AX_CIPHER_CRYPTO_MAX_SIZE;
	} else {
		blk_size = byte_length;
	}
	blkCnt = byte_length / blk_size;
	ret = ax_cipher_cmm_alloc(blk_size, &cmm);
	if (ret < 0) {
		ax_cipher_err("Alloc memory failed!\n");
		return AX_CIPHER_NO_MEMORY;
	}
	ret = ax_cipher_cmm_alloc(blk_size, &cmm_dst);
	if (ret < 0) {
		ax_cipher_err("Alloc memory failed!\n");
		ax_cipher_cmm_free(&cmm);
		return AX_CIPHER_NO_MEMORY;
	}
	ax_cipher_dbg("src:%lx, dst:%lx, byte_length:%x, blkCnt:%d\n", (uint64_t) src_addr, (uint64_t) dest_addr, byte_length, blkCnt);
	phy_addr = (void *)cmm.pyhy_addr;
	vir_addr = cmm.vir_addr;
	while (blkCnt--) {
		memcpy(vir_addr, src_addr, blk_size);
		ax_cipher_cache_flush(vir_addr, blk_size);
		ax_cipher_dbg("src:%lx, dst:%lx, len: %\n", (uint64_t) src_addr, (uint64_t) dest_addr, blk_size);
		ax_cipher_cache_invalid(cmm_dst.vir_addr, blk_size);
		ret = ax_cipher_crypto_block(ph_cipher, (uint8_t *) phy_addr, (uint8_t *) cmm_dst.pyhy_addr, blk_size, encrypt);
		if (ret < 0) {
			ax_cipher_cmm_free(&cmm);
			ax_cipher_cmm_free(&cmm_dst);
			ax_cipher_err("failed ret: %d, src_addr:%lx, dest_addr:%lx,byte_length:%x\n", ret, (uint64_t) src_addr, (uint64_t) dest_addr, byte_length);
			return ret;
		}
		memcpy(dest_addr, cmm_dst.vir_addr, blk_size);
		src_addr += blk_size;
		byte_length -= blk_size;
		dest_addr += blk_size;
	}
	if (byte_length) {
		ax_cipher_dbg("Crypto, src:%lx, dst:%lx, byte_length: %x\n", (uint64_t) src_addr, (uint64_t) dest_addr, byte_length);
		memcpy(vir_addr, src_addr, byte_length);
		ax_cipher_cache_flush(vir_addr, byte_length);
		ax_cipher_cache_invalid(dest_addr, byte_length);
		//memcpy(dest_addr, cmm_dst.vir_addr, byte_length);
		ret = ax_cipher_crypto_block(ph_cipher, phy_addr, dest_addr, byte_length, encrypt);
		memcpy(dest_addr, cmm_dst.vir_addr, byte_length);
		if (ret < 0) {
			ax_cipher_err("failed ret: %d, src_addr:%lx, dest_addr:%lx,byte_length:%x\n", ret, (uint64_t) src_addr, (uint64_t) dest_addr, byte_length);
		}
	}
	ax_cipher_cmm_free(&cmm);
	ax_cipher_cmm_free(&cmm_dst);
	return ret;
}

int32_t ax_cipher_encrypt(AX_CIPHER_HANDLE p_cipher, uint8_t * src_addr, uint8_t * dest_addr, uint32_t byte_length)
{
	int ret;
	ax_cipher_lock();
	ret = ax_cipher_crypto(p_cipher, src_addr, dest_addr, byte_length, true);
	ax_cipher_unlock();
	return ret;
}

int32_t ax_cipher_decrypt(AX_CIPHER_HANDLE p_cipher, uint8_t * src_addr, uint8_t * dest_addr, uint32_t byte_length)
{
	int ret;
	ax_cipher_lock();
	ret = ax_cipher_crypto(p_cipher, src_addr, dest_addr, byte_length, false);
	ax_cipher_unlock();
	return ret;
}

int32_t ax_cipher_destroy_handle(AX_CIPHER_HANDLE p_cipher)
{
	if (p_cipher < AX_CIPHER_MAX_CHN) {
		s_cipher_chn[p_cipher].is_used = false;
		return AX_CIPHER_SUCCESS;
	}
	return AX_CIPHER_INVALID_PARAMETER;
}

static uint32_t ax_cipher_trng_config(uint8_t auto_seed, uint16_t sample_cycles, uint8_t sample_div, uint8_t noise_blocks, bool freseed)
{
	int ret;
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	// Configure
	Eip130Token_Command_TRNG_Configure(&command_token, auto_seed, sample_cycles, sample_div, noise_blocks);
	if (freseed) {
		// RRD = Reseed post-processor
		command_token.W[2] |= BIT_1;
	}
	command_token.W[0] |= 1;
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		ax_cipher_err("config error: %x\n", result_token.W[0]);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}

uint32_t ax_cipher_get_random_number(uint8_t * p_random_number, uint32_t size)
{
	int ret;
	ax_cipher_cmm_t cmm;
	void *phy_addr;
	void *vir_addr;
	if (!p_random_number || !size || (size > AX_CIPHER_RANDOM_MAX_BLK_SIZE)) {
		return AX_CIPHER_INVALID_PARAMETER;
	}
	ax_cipher_lock();
	ret = ax_cipher_cmm_alloc(size, &cmm);
	if (ret < 0) {
		ax_cipher_err("Alloc memory failed!\n");
		ax_cipher_unlock();
		return AX_CIPHER_NO_MEMORY;
	}
	phy_addr = (void *)cmm.pyhy_addr;
	vir_addr = cmm.vir_addr;
	ax_cipher_dbg("phy_addr:%lx, vir_addr:%lx\n", (uint64_t) phy_addr, (uint64_t) vir_addr);
	ax_cipher_cache_invalid(vir_addr, size);
	Eip130Token_Command_RandomNumber_Generate(&command_token, size, (uint64_t) phy_addr);
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		ax_cipher_cmm_free(&cmm);
		ax_cipher_err("fail: %x, result: %x\n", ret, result_token.W[0]);
		ax_cipher_unlock();
		return AX_CIPHER_INTERNAL_ERROR;
	}
	memcpy(p_random_number, vir_addr, size);
	ax_cipher_cmm_free(&cmm);
	ax_cipher_unlock();
	return AX_CIPHER_SUCCESS;
}

static int32_t ax_cipher_system_reset(void)
{
	int ret;
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	Eip130Token_Command_SystemReset(&command_token);
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	if ((ret != AX_CIPHER_SUCCESS) || (result_token.W[0] & (1 << 31))) {
		ax_cipher_err("fail: ret: %x, result: %x\n", ret, result_token.W[0]);
		return AX_CIPHER_OPERATION_FAILED;
	}
	return AX_CIPHER_SUCCESS;
}
#if 0
static uint32_t associated_data[] = {
	0x656d6f53,
	0x6f737341,
	0x74616963,
	0x61446465,
	0x6f466174,
	0x6f725072,
	0x69736976,
	0x6e696e6f,
	0x74695767,
	0x79654b68,
	0x626f6c42,
};

static const uint8_t provisioning_key[] = {
	0x18, 0xE2, 0xD1, 0x78, 0x1F, 0x0C, 0x1F, 0x51, 0xCC, 0x3D, 0x05, 0x10, 0xE6, 0xF9, 0x43, 0x4B,
	0x9E, 0x0F, 0x93, 0x15, 0xD7, 0x44, 0x6D, 0x59, 0xAC, 0xF3, 0x00, 0x73, 0xF9, 0xE7, 0x35, 0xE7,
	0xFD, 0xFC, 0x3A, 0xA0, 0x60, 0x48, 0xA1, 0x68, 0x7C, 0x48, 0x77, 0x17, 0xBE, 0x3B, 0xFB, 0xFF,
	0x18, 0x30, 0x10, 0xE2, 0xA7, 0x2C, 0x91, 0x02, 0x10, 0x3C, 0xD6, 0x6C, 0x64, 0x76, 0x80, 0x66
};

static int32_t ax_cipher_provision_random_rootkey(void)
{
	ax_cipher_cmm_t cmm;
	int ret = 0;
	ret = ax_cipher_cmm_alloc(64, &cmm);
	if (ret < 0) {
		ax_cipher_err("malloc memory failed!\n");
		return AX_CIPHER_NO_MEMORY;
	}
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	Eip130Token_Command_ProvisionRandomHUK(&command_token, 0, 1, 1, 0, 3, 1, 1, cmm.pyhy_addr, 52, (const uint8_t * const)associated_data, sizeof(associated_data));
	Eip130Token_Command_Identity(&command_token, 0x4F5A3647);
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	if ((ret != AX_CIPHER_SUCCESS) || (result_token.W[0] & (1 << 31))) {
		ax_cipher_cmm_free(&cmm);
		ax_cipher_err("fail: ret: %x, result[0]: %x\n", ret, result_token.W[0]);
		return AX_CIPHER_OPERATION_FAILED;
	}
	ax_cipher_cmm_free(&cmm);
	ax_cipher_system_reset();
	return AX_CIPHER_SUCCESS;
}

static int32_t ax_cipher_public_data_blob_gen(void *data, int len)
{
	unsigned char keyblob[64];
	unsigned int random_data[16];
	int ret;
	int keyblob_size;
	KEY_OTP_BLOB_CONTEXT context;
	ret = ax_cipher_get_random_number((uint8_t *) random_data, PUBLIC_DATA_LEN);
	if (ret < 0) {
		ax_cipher_err("getrandom fail: %d\n", ret);
		return ret;
	}
	context.key = (void *) provisioning_key;
	context.label = associated_data;
	context.policy = PUBLIC_POLICY;
	context.key_len = sizeof(provisioning_key);
	context.label_size = sizeof(associated_data);
	memcpy(data, random_data, PUBLIC_DATA_LEN);
	ret = ax_cipher_keyblob_wrap(&context, random_data, PUBLIC_DATA_LEN, keyblob, &keyblob_size);
	if (ret < 0) {
		ax_cipher_err("keyblobwrap fail: %x\n", ret);
		return ret;
	}
	if (keyblob_size > len) {
		keyblob_size = len;
	}
	memcpy(data, keyblob, keyblob_size);
	return AX_CIPHER_SUCCESS;
}
static int32_t ax_cipher_otp_public_write(uint8_t *keyblob, uint32_t size, int asset_num)
{
	int ret;
	ax_cipher_cmm_t cmm;
	ret = ax_cipher_cmm_alloc(size, &cmm);
	if (ret < 0) {
		ax_cipher_err("malloc memory failed!\n");
		return AX_CIPHER_NO_MEMORY;
	}
	memcpy(cmm.vir_addr, keyblob, size);
	ax_cipher_cache_flush(cmm.vir_addr, size);
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	Eip130Token_Command_OTPDataWrite(&command_token, asset_num, 4, 0, cmm.pyhy_addr, size, (const uint8_t * const)associated_data, sizeof(associated_data));
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	if ((ret != AX_CIPHER_SUCCESS) || (result_token.W[0] & (1 << 31))) {
		ax_cipher_cmm_free(&cmm);
		ax_cipher_err("OTP Write fail: ret: %x, result: %x\n", ret, result_token.W[0]);
		return AX_CIPHER_OPERATION_FAILED;
	}
	ax_cipher_cmm_free(&cmm);
	ax_cipher_system_reset();
	return AX_CIPHER_SUCCESS;

}
static int32_t ax_cipher_otp_public_init(void)
{
	int ret;
	uint8_t keyblob[PUBLIC_DATA_BLOB_LEN];
	ret = ax_cipher_public_data_blob_gen(keyblob, PUBLIC_DATA_BLOB_LEN);
	if (ret < 0) {
		ax_cipher_err("ret = %x\n", ret);
		return AX_CIPHER_OPERATION_FAILED;
	}
	ax_cipher_lock();
	ret = ax_cipher_otp_public_write(keyblob, PUBLIC_DATA_BLOB_LEN, PUBLIC_DATA_ASSET_NUMBER);
	ax_cipher_unlock();
	return AX_CIPHER_SUCCESS;
}
static int32_t ax_cipher_asset_search(int asset_number, int *as_id)
{
	int ret;
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	Eip130Token_Command_AssetSearch(&command_token, asset_number);
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	if ((ret != AX_CIPHER_SUCCESS) || (result_token.W[0] & (1 << 31))) {
		ax_cipher_err("fail: ret: %x, result: %x\n", ret, result_token.W[0]);
		return AX_CIPHER_OPERATION_FAILED;
	}
	*as_id = result_token.W[1];
	return AX_CIPHER_SUCCESS;
}

static int32_t ax_cipher_otp_public_read(void *data, int size)
{
	int ret;
	ax_cipher_cmm_t cmm;
	int as_id;
	ret = ax_cipher_cmm_alloc(PUBLIC_DATA_BLOB_LEN, &cmm);
	if (ret < 0) {
		ax_cipher_err("cmm malloc memory failed!\n");
		return AX_CIPHER_NO_MEMORY;
	}
	ret = ax_cipher_asset_search(PUBLIC_DATA_ASSET_NUMBER, &as_id);
	if (ret < 0) {
		ax_cipher_err("assetsearch fail: %d\n", ret);
		ax_cipher_cmm_free(&cmm);
		return AX_CIPHER_OPERATION_FAILED;
	}
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	Eip130Token_Command_PublicData_Read(&command_token, as_id, cmm.pyhy_addr, PUBLIC_DATA_BLOB_LEN);
	//ax_cipher_cache_flush(cmm.vir_addr, PUBLIC_DATA_BLOB_LEN);
	ax_cipher_cache_invalid(cmm.vir_addr, PUBLIC_DATA_BLOB_LEN);
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	if ((ret != AX_CIPHER_SUCCESS) || (result_token.W[0] & (1 << 31))) {
		ax_cipher_cmm_free(&cmm);
		ax_cipher_err("fail: ret: %x, result: %x\n", ret, result_token.W[0]);
		return AX_CIPHER_OPERATION_FAILED;
	}
	if (PUBLIC_DATA_LEN < size) {
		size = PUBLIC_DATA_LEN;
	}
	memcpy(data, cmm.vir_addr, size);
	ax_cipher_cmm_free(&cmm);
	return ret;
}
#endif

int32_t ax_cipher_encrypt_mac_blk(uint8_t * szKeyAddr, uint32_t key_byte_length, uint8_t * src_addr, uint32_t byte_length, uint8_t * mac_addr, uint32_t mac_length)
{
	ax_cipher_cmm_t cmm;
	int ret;
	uint32_t data_size;
	int npadbytes;
	uint8_t *Data_p;
	if (byte_length > MAX_CMAC_BLK_SIZE) {
		ax_cipher_err("cmac only support input size smaller than %d bytes!\n", MAX_CMAC_BLK_SIZE);
		return -1;
	}
	ret = ax_cipher_cmm_alloc(MAX_CMAC_BLK_SIZE + MAX_CMAC_BLK_SIZE, &cmm);
	if (ret < 0) {
		ax_cipher_err("cmm malloc memory failed!\n");
		return AX_CIPHER_NO_MEMORY;
	}
	data_size = (byte_length + CMAC_BLK_SIZE - 1) & ~(CMAC_BLK_SIZE - 1);
	if (data_size == 0) {
		data_size += CMAC_BLK_SIZE;
	}
	npadbytes = (int)(data_size - byte_length);
	memset(cmm.vir_addr, 0, MAX_CMAC_BLK_SIZE + MAX_CMAC_BLK_SIZE);
	memcpy(cmm.vir_addr, src_addr, byte_length);
	if (npadbytes) {
		Data_p = (uint8_t *) cmm.vir_addr + byte_length;
		*Data_p++ = 0x80;
		if (npadbytes == (int)CMAC_BLK_SIZE) {
			npadbytes--;
		}
	}
	ax_cipher_lock();
	memset(&command_token, 0, sizeof(command_token));
	ax_cipher_cache_flush(cmm.vir_addr, MAX_CMAC_BLK_SIZE + MAX_CMAC_BLK_SIZE);
	//not support update, only support one block process, only support AES_CMAC
	Eip130Token_Command_Mac(&command_token, EIP130TOKEN_MAC_ALGORITHM_AES_CMAC, true, true, cmm.pyhy_addr, data_size);
	Eip130Token_Command_Mac_CopyKey(&command_token, szKeyAddr, key_byte_length);
	Eip130Token_Command_Mac_SetTotalMessageLength(&command_token, (uint64_t) npadbytes);
	ret = eip130_physical_token_exchange(&command_token, &result_token, 1);
	if ((ret != AX_CIPHER_SUCCESS) || (result_token.W[0] & (1 << 31))) {
		ax_cipher_cmm_free(&cmm);
		ax_cipher_err("fail: ret: %x, result: %x\n", ret, result_token.W[0]);
		ax_cipher_unlock();
		return AX_CIPHER_OPERATION_FAILED;
	}
	Eip130Token_Result_Mac_CopyMAC(&result_token, mac_length, mac_addr);
	ax_cipher_cmm_free(&cmm);
	ax_cipher_unlock();
	return AX_CIPHER_SUCCESS;
}
