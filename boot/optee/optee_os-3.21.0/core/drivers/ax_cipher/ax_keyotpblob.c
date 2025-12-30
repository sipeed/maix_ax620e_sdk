/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <util.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ax_base_type.h"
#include "ax_cipher_api.h"
#include "ax_aessiv.h"
#include "ax_keyotpblob.h"
static bool g_verbose = true;
static aessiv_context g_aessiv_context;

static int eip130_keyblob_wrap(KEY_OTP_BLOB_CONTEXT * keyblob_context, void * asset_data, int32_t asset_data_len, void * keyblob, int32_t * keyblob_size)
{
	int ret = -1;
	size_t output_size = asset_data_len + 128 / 8;
	char label[256];

	ret = ax_cipher_aessiv_init(&g_aessiv_context, g_verbose);
	if (ret != 0) {
		ax_cipher_err("KeyBlob setup failed (Init)\n");
		return -1;
	}

	ret = ax_cipher_aessiv_set_key(&g_aessiv_context, keyblob_context->key, keyblob_context->key_len);
	if (ret != 0) {
		ax_cipher_err("KeyBlob setup failed (Key)\n");
		return -1;
	}

	memcpy(label, keyblob_context->label, keyblob_context->label_size);
	memcpy(label + keyblob_context->label_size, &keyblob_context->policy, 8);
	ret = ax_cipher_aessiv_set_ad(&g_aessiv_context, (const uint8_t *)label, keyblob_context->label_size + 8);
	if (ret != 0) {
		ax_cipher_err("KeyBlob setup failed (AD)\n");
		return -1;
	}

	ret = ax_cipher_aessiv_encrypt(&g_aessiv_context, asset_data, asset_data_len, keyblob, &output_size);
	if (ret != 0) {
		ax_cipher_err("KeyBlob generation failed\n");
		return -1;
	}
	*keyblob_size = output_size;
	return 0;
}

static int eip130_keyblob_unwrap(KEY_OTP_BLOB_CONTEXT * keyblob_context, void * keyblob, int32_t keyblob_size, void * asset_data, int32_t * asset_data_len)
{
	int ret = -1;
	size_t output_size = keyblob_size - 128 / 8;
	char label[256];
	if ((keyblob_context->label_size + 8) > (int32_t) sizeof(label)) {
		ax_cipher_err("eip130_keyblob_unwrap kblob size is too large\n");
		return -1;
	}
	ret = ax_cipher_aessiv_init(&g_aessiv_context, g_verbose);
	if (ret != 0) {
		ax_cipher_err("KeyBlob setup failed (Init)\n");
		return -1;
	}

	ret = ax_cipher_aessiv_set_key(&g_aessiv_context, keyblob_context->key, keyblob_context->key_len);
	if (ret != 0) {
		ax_cipher_err("KeyBlob setup failed (Key)\n");
		return -1;
	}
	memcpy(label, keyblob_context->label, keyblob_context->label_size);
	memcpy(label + keyblob_context->label_size, &keyblob_context->policy, 8);
	ret = ax_cipher_aessiv_set_ad(&g_aessiv_context, (const uint8_t *)label, keyblob_context->label_size + 8);
	if (ret != 0) {
		ax_cipher_err("KeyBlob setup failed (AD)\n");
		return -1;
	}
	ret = ax_cipher_aessiv_decrypt(&g_aessiv_context, keyblob, keyblob_size, asset_data, &output_size);
	if (ret != 0) {
		ax_cipher_err("KeyBlob Decrypt failed\n");
		return -1;
	}
	*asset_data_len = output_size;
	return 0;
}

int32_t ax_cipher_keyblob_wrap(KEY_OTP_BLOB_CONTEXT * keyblob_context, void * asset_data, int32_t asset_data_len, void * keyblob, int32_t * keyblob_size)
{
	if (!keyblob_context->key || !keyblob_context->label || !keyblob || !asset_data || !asset_data_len) {
		return -1;
	}
	eip130_keyblob_wrap(keyblob_context, asset_data, asset_data_len, keyblob, keyblob_size);
	return 0;
}

int32_t ax_cipher_keyblob_unwrap(KEY_OTP_BLOB_CONTEXT * keyblob_context, void * keyblob, int32_t keyblob_size, void * asset_data, int32_t * asset_data_len)
{
	if (!keyblob_context->key || !keyblob_context->label || !keyblob || !asset_data || !asset_data_len) {
		return -1;
	}
	eip130_keyblob_unwrap(keyblob_context, keyblob, keyblob_size, asset_data, asset_data_len);
	return 0;
}
