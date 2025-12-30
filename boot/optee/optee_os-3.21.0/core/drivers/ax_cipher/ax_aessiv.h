/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_AESSIV_H__
#define __AX_AESSIV_H__

typedef struct {
	uint8_t key[2 * (256 / 8)];
	uint32_t key_size;
	uint8_t ad_buffer[512];
	uint8_t *ad_list[4];
	unsigned int ad_list_count;
	bool verbose;
} aessiv_context;

int ax_cipher_aessiv_init(aessiv_context * context, const bool verbose);
int ax_cipher_aessiv_set_key(aessiv_context * context, const uint8_t * key, const size_t keySize);
int ax_cipher_aessiv_set_ad(aessiv_context * context, const uint8_t * ad, const size_t ad_size);
int ax_cipher_aessiv_encrypt(aessiv_context * context, const uint8_t * in_data, const size_t inSize, uint8_t * out_data, size_t * outSize);
int ax_cipher_aessiv_decrypt(aessiv_context * context, const uint8_t * in_data, const size_t inSize, uint8_t * out_data, size_t * outSize);
#endif
